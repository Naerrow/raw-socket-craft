#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>

#include "builder.h"

#define SEND_COUNT 5     /* 전송 횟수 */
#define SEND_DELAY 1     /* 전송 간격 (초) */

static void print_usage(const char *prog) {
    fprintf(stderr, "사용법: %s <목적지IP> [--bad-csum]\n", prog);
    fprintf(stderr, "  예시: %s 8.8.8.8\n", prog);
    fprintf(stderr, "  예시: %s 8.8.8.8 --bad-csum   (잘못된 체크섬으로 전송)\n", prog);
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

    /* Raw socket 생성 (IP 헤더 직접 작성) */
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        perror("[오류] socket() 실패 - root 권한이 필요합니다");
        return EXIT_FAILURE;
    }

    /* IP_HDRINCL: IP 헤더를 직접 채우겠다고 커널에 알림 */
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("[오류] setsockopt(IP_HDRINCL) 실패");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("[시작] %s 으로 ICMP Echo Request %d개 전송\n", inet_ntoa(dst_addr), SEND_COUNT);
    printf("       Wireshark에서 checksum 필드를 확인하세요.\n\n");

    /* 순차적으로 패킷 전송 */
    for (int i = 1; i <= SEND_COUNT; i++) {
        if (send_icmp_echo(sock, dst_addr, (uint16_t)getpid(), (uint16_t)i, bad_csum) < 0) {
            fprintf(stderr, "[오류] %d번째 패킷 전송 실패\n", i);
        }
        if (i < SEND_COUNT) sleep(SEND_DELAY);
    }

    close(sock);
    printf("\n[완료] 전송 종료\n");
    return EXIT_SUCCESS;
}
