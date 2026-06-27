#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "proto.h"
#include "session.h"

#define CUSTOM_PROTO_NUM  253
#define LOOPBACK_IP       "127.0.0.1"
#define SEND_COUNT        10
#define ACK_TIMEOUT_MS    2000   /* ACK 대기 타임아웃 */
#define RECV_POLL_MS      200    /* 수신 루프 폴링 간격 */

static volatile int running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

/* ──────────────────────────────────────────
 * 송신 모드
 * REQUEST 전송 → ACK 대기 → 통계 출력
 * ────────────────────────────────────────── */
static void run_sender(int sock, struct sockaddr_in *dst) {
    printf("[송신] %s 로 REQUEST %d개 전송\n", LOOPBACK_IP, SEND_COUNT);
    printf("       netem 시뮬레이션: sudo ./scripts/simulate-loss.sh [loss|delay|reorder]\n\n");

    int sent = 0, acked = 0;

    for (uint32_t i = 1; i <= (uint32_t)SEND_COUNT && running; i++) {
        char payload[64];
        snprintf(payload, sizeof(payload), "msg-%u", i);

        /* REQUEST 전송 */
        ssize_t s = session_send(sock, dst, PROTO_REQUEST, i, i, payload);
        if (s < 0) {
            fprintf(stderr, "[오류] %u번째 전송 실패\n", i);
            continue;
        }
        sent++;

        CustomHeader echo = {
            .type            = PROTO_REQUEST,
            .message_id      = i,
            .sequence_number = i,
            .length          = (uint16_t)(sizeof(CustomHeader) + strlen(payload)),
        };
        session_print("송신", &echo, payload);

        /* ACK 대기 */
        CustomHeader ack_hdr;
        char ack_payload[MAX_PAYLOAD];
        struct sockaddr_in from;

        int got_ack = 0;
        int waited = 0;

        while (waited < ACK_TIMEOUT_MS && running) {
            if (session_recv_timeout(sock, &ack_hdr, ack_payload, &from, RECV_POLL_MS)) {
                /* 내가 보낸 REQUEST의 loopback 복사본은 무시 */
                if (ack_hdr.type == PROTO_REQUEST) {
                    waited += RECV_POLL_MS;
                    continue;
                }
                if (ack_hdr.type == PROTO_ACK && ack_hdr.message_id == i) {
                    session_print("  ACK", &ack_hdr, NULL);
                    acked++;
                    got_ack = 1;
                    break;
                }
            }
            waited += RECV_POLL_MS;
        }

        if (!got_ack) {
            printf("  [타임아웃] msg_id=%u — ACK 없음\n", i);
        }

        printf("\n");
        sleep(1);
    }

    /* 최종 통계 */
    printf("══════════════════════════════\n");
    printf("전송: %d  ACK 수신: %d  손실: %d (%.0f%%)\n",
           sent, acked, sent - acked,
           sent > 0 ? (double)(sent - acked) / sent * 100.0 : 0.0);
    printf("══════════════════════════════\n");
}

/* ──────────────────────────────────────────
 * 수신 모드
 * REQUEST 수신 → ACK 회신 → 순서 역전/누락 감지
 * ────────────────────────────────────────── */
static void run_receiver(int sock) {
    printf("[수신] 패킷 대기 중... (종료: Ctrl+C)\n");
    printf("       순서 역전·패킷 누락을 자동으로 감지합니다.\n\n");

    int total = 0;
    uint32_t last_seq = 0;
    uint32_t expected_seq = 1;

    while (running) {
        CustomHeader hdr;
        char payload[MAX_PAYLOAD];
        struct sockaddr_in src;

        /* 200ms 간격으로 폴링 → Ctrl+C 즉시 반응 */
        if (!session_recv_timeout(sock, &hdr, payload, &src, RECV_POLL_MS)) continue;

        /* loopback에서 자신이 보낸 ACK 패킷이 다시 들어오는 경우 무시 */
        if (hdr.type == PROTO_ACK) continue;

        total++;
        session_print("수신", &hdr, payload);

        /* ── 패킷 누락 감지 ── */
        if (hdr.sequence_number > expected_seq) {
            for (uint32_t missing = expected_seq; missing < hdr.sequence_number; missing++) {
                printf("  [누락] seq=%u 패킷이 오지 않았습니다\n", missing);
            }
        }

        /* ── 순서 역전 감지 ── */
        if (last_seq > 0 && hdr.sequence_number < last_seq) {
            printf("  [역전] 순서 역전 감지! 이전 seq=%u → 현재 seq=%u\n",
                   last_seq, hdr.sequence_number);
        }

        last_seq    = hdr.sequence_number;
        expected_seq = hdr.sequence_number + 1;

        /* ACK 회신 */
        session_send(sock, &src, PROTO_ACK, hdr.message_id, hdr.sequence_number, NULL);
        printf("  → ACK 회신 (msg_id=%u)\n\n", hdr.message_id);
    }

    printf("[완료] 총 %d개 수신\n", total);
}

/* ──────────────────────────────────────────
 * main
 * ────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    if (argc < 2 || (strcmp(argv[1], "sender") != 0 && strcmp(argv[1], "receiver") != 0)) {
        fprintf(stderr, "사용법: %s [sender|receiver]\n", argv[0]);
        fprintf(stderr, "  sender   — REQUEST 전송 후 ACK 대기, 최종 통계 출력\n");
        fprintf(stderr, "  receiver — REQUEST 수신, ACK 회신, 누락·역전 감지\n");
        return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_RAW, CUSTOM_PROTO_NUM);
    if (sock < 0) {
        perror("[오류] socket() 실패 — root 권한이 필요합니다");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);

    struct sockaddr_in dst = { .sin_family = AF_INET };
    inet_pton(AF_INET, LOOPBACK_IP, &dst.sin_addr);

    if (strcmp(argv[1], "sender") == 0) {
        run_sender(sock, &dst);
    } else {
        run_receiver(sock);
    }

    close(sock);
    return EXIT_SUCCESS;
}
