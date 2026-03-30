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

#define CUSTOM_PROTO_NUM 253   /* session.c와 동일해야 함 */
#define LOOPBACK_IP      "127.0.0.1"
#define SEND_COUNT       5

static volatile int running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

/**
 * 사용법:
 *   ./protocol sender   → REQUEST 패킷 5개 전송 (loopback)
 *   ./protocol receiver → 패킷 수신 대기 및 출력
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "사용법: %s [sender|receiver]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int is_sender = (strcmp(argv[1], "sender") == 0);

    /* Raw socket 생성 */
    int sock = socket(AF_INET, SOCK_RAW, CUSTOM_PROTO_NUM);
    if (sock < 0) {
        perror("[오류] socket() 실패 - root 권한이 필요합니다");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);

    struct sockaddr_in dst = {
        .sin_family = AF_INET,
    };
    inet_aton(LOOPBACK_IP, &dst.sin_addr);

    if (is_sender) {
        /* ── 송신 모드 ── */
        printf("[송신] %s 로 REQUEST %d개 전송 시작\n", LOOPBACK_IP, SEND_COUNT);
        printf("       tc netem으로 유실/지연/순서 역전을 시뮬레이션할 수 있습니다.\n");
        printf("       참고: scripts/simulate-loss.sh\n\n");

        for (uint32_t i = 1; i <= SEND_COUNT && running; i++) {
            char payload[64];
            snprintf(payload, sizeof(payload), "hello-%u", i);

            ssize_t sent = session_send(sock, &dst,
                                        PROTO_REQUEST, i, i, payload);
            if (sent > 0) {
                CustomHeader echo = {
                    .type            = PROTO_REQUEST,
                    .message_id      = i,
                    .sequence_number = i,
                    .length          = (uint16_t)(sizeof(CustomHeader) + strlen(payload)),
                };
                session_print("송신", &echo, payload);
            }

            sleep(1);
        }

    } else {
        /* ── 수신 모드 ── */
        printf("[수신] 패킷 수신 대기 중... (종료: Ctrl+C)\n\n");

        uint32_t last_seq = 0;   /* 순서 역전 감지용 */
        int pkt_count = 0;

        while (running) {
            CustomHeader hdr;
            char payload[MAX_PAYLOAD];
            struct sockaddr_in src;

            if (!session_recv(sock, &hdr, payload, &src)) continue;

            pkt_count++;

            /* 순서 역전 감지 */
            if (hdr.sequence_number < last_seq) {
                printf("[경고] 순서 역전 감지! 이전 seq=%u, 현재 seq=%u\n",
                       last_seq, hdr.sequence_number);
            }
            last_seq = hdr.sequence_number;

            session_print("수신", &hdr, payload);

            /* REQUEST면 ACK 회신 */
            if (hdr.type == PROTO_REQUEST) {
                session_send(sock, &src, PROTO_ACK,
                             hdr.message_id, hdr.sequence_number, NULL);
                printf("       → ACK 회신 (msg_id=%u)\n", hdr.message_id);
            }
        }

        printf("\n[완료] 총 %d개 패킷 수신\n", pkt_count);
    }

    close(sock);
    return EXIT_SUCCESS;
}
