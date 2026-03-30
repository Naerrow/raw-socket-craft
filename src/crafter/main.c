#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>

#include "builder.h"

#define SEND_COUNT   5     /* 전송 횟수 */
#define SEND_DELAY   1     /* 전송 간격 (초) */
#define TIMEOUT_MS   2000  /* 응답 대기 타임아웃 (밀리초) */

static void print_usage(const char *prog) {
    fprintf(stderr, "사용법: %s <목적지IP> [--bad-csum]\n", prog);
    fprintf(stderr, "  예시: %s 8.8.8.8\n", prog);
    fprintf(stderr, "  예시: %s 8.8.8.8 --bad-csum   (잘못된 체크섬 → Wireshark [incorrect])\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* 목적지 IP 파싱 */
    struct in_addr dst_addr;
    if (inet_aton(argv[1], &dst_addr) == 0) {
        fprintf(stderr, "[오류] 잘못된 IP 주소: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    /* --bad-csum 옵션 확인 */
    int bad_csum = 0;
    if (argc >= 3 && strcmp(argv[2], "--bad-csum") == 0) {
        bad_csum = 1;
    }

    /* 전송용 raw socket (IP 헤더 직접 작성) */
    int send_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (send_sock < 0) {
        perror("[오류] socket() 실패 - root 권한이 필요합니다");
        return EXIT_FAILURE;
    }

    /* IP_HDRINCL: IP 헤더를 직접 채우겠다고 커널에 알림 */
    int one = 1;
    if (setsockopt(send_sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("[오류] setsockopt(IP_HDRINCL) 실패");
        close(send_sock);
        return EXIT_FAILURE;
    }

    /* 수신용 raw socket (ICMP Echo Reply 수신) */
    int recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (recv_sock < 0) {
        perror("[오류] recv socket() 실패");
        close(send_sock);
        return EXIT_FAILURE;
    }

    uint16_t id = (uint16_t)getpid();

    printf("[시작] PING %s — %d개 전송, 타임아웃 %dms\n",
           inet_ntoa(dst_addr), SEND_COUNT, TIMEOUT_MS);
    if (bad_csum)
        printf("[경고] --bad-csum 모드: 의도적으로 잘못된 체크섬 사용\n");
    printf("       sniffer를 함께 실행하면 패킷 헤더를 실시간으로 볼 수 있습니다.\n\n");

    /* 통계 */
    int sent = 0, received = 0;
    double rtt_min = 1e9, rtt_max = 0.0, rtt_sum = 0.0;

    for (int i = 1; i <= SEND_COUNT; i++) {
        if (send_icmp_echo(send_sock, dst_addr, id, (uint16_t)i, bad_csum) < 0) {
            fprintf(stderr, "[오류] %d번째 패킷 전송 실패\n", i);
            continue;
        }
        sent++;

        double rtt_ms = 0.0;
        if (recv_icmp_reply(recv_sock, id, (uint16_t)i, TIMEOUT_MS, &rtt_ms) == 0) {
            received++;
            if (rtt_ms < rtt_min) rtt_min = rtt_ms;
            if (rtt_ms > rtt_max) rtt_max = rtt_ms;
            rtt_sum += rtt_ms;
        } else {
            printf("[타임아웃] seq=%d — %dms 안에 응답 없음\n", i, TIMEOUT_MS);
        }

        if (i < SEND_COUNT) sleep(SEND_DELAY);
    }

    /* 통계 출력 */
    printf("\n--- %s PING 통계 ---\n", inet_ntoa(dst_addr));
    printf("전송: %d  수신: %d  손실: %d (%.0f%%)\n",
           sent, received, sent - received,
           sent > 0 ? (double)(sent - received) / sent * 100.0 : 0.0);
    if (received > 0) {
        printf("RTT  최소: %.3f ms  평균: %.3f ms  최대: %.3f ms\n",
               rtt_min, rtt_sum / received, rtt_max);
    }

    close(send_sock);
    close(recv_sock);
    return EXIT_SUCCESS;
}
