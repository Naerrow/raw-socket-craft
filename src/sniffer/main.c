#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_ether.h>    /* ETH_P_ALL */
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "parser.h"

#define BUF_SIZE 65536

static volatile int running = 1;

/* Ctrl+C 핸들러 */
static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
    printf("\n[종료] 패킷 캡처를 중단합니다.\n");
}

static void print_usage(const char *prog) {
    fprintf(stderr, "사용법: %s [프로토콜 필터]\n", prog);
    fprintf(stderr, "  필터 없음  : 모든 패킷 출력\n");
    fprintf(stderr, "  icmp       : ICMP 패킷만 출력\n");
    fprintf(stderr, "  tcp        : TCP 패킷만 출력\n");
    fprintf(stderr, "  udp        : UDP 패킷만 출력\n");
    fprintf(stderr, "  예시: %s icmp\n", prog);
}

int main(int argc, char *argv[]) {
    /* 프로토콜 필터 설정 (0 = 모두 출력) */
    uint8_t filter_proto = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "icmp") == 0)      filter_proto = 1;
        else if (strcmp(argv[1], "tcp") == 0)  filter_proto = 6;
        else if (strcmp(argv[1], "udp") == 0)  filter_proto = 17;
        else { print_usage(argv[0]); return EXIT_FAILURE; }
    }

    /* AF_PACKET: 링크 계층(Ethernet)부터 수신
     * SOCK_RAW: 원시 패킷
     * ETH_P_ALL: 모든 프로토콜 수신 */
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("[오류] socket() 실패 - root 권한이 필요합니다");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);

    if (filter_proto == 0)
        printf("[시작] 모든 패킷 캡처 중. 종료: Ctrl+C\n");
    else
        printf("[시작] %s 패킷만 캡처 중. 종료: Ctrl+C\n", proto_name(filter_proto));

    printf("%s\n", "----------------------------------------------------------------------");

    uint8_t buf[BUF_SIZE];
    int pkt_count = 0;

    while (running) {
        int recv_len = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
        if (recv_len < 0) {
            if (!running) break;  /* 시그널로 인한 중단 */
            perror("[오류] recvfrom() 실패");
            continue;
        }

        /* Ethernet 헤더(14바이트)를 건너뛰고 IP 헤더 시작 위치로 이동 */
        if (recv_len < ETH_HLEN) continue;

        const uint8_t *ip_start = buf + ETH_HLEN;
        int ip_len = recv_len - ETH_HLEN;

        /* IP 버전 확인 (IPv4만 처리) */
        if ((ip_start[0] >> 4) != 4) continue;

        /* 프로토콜 필터 적용 */
        uint8_t proto = ip_start[9];
        if (filter_proto != 0 && proto != filter_proto) continue;

        IpInfo ip_info;
        memset(&ip_info, 0, sizeof(ip_info));

        printf("[%4d] ", ++pkt_count);
        if (!parse_ip_header(ip_start, ip_len, &ip_info)) continue;

        /* ICMP인 경우 추가 파싱 */
        if (ip_info.proto == 1) {
            int ip_header_len = ip_info.ihl * 4;
            const uint8_t *icmp_start = ip_start + ip_header_len;
            int icmp_len = ip_len - ip_header_len;
            parse_icmp_header(icmp_start, icmp_len);
        }

        printf("\n");
        fflush(stdout);
    }

    close(sock);
    printf("[완료] 총 %d개의 패킷을 캡처했습니다.\n", pkt_count);
    return EXIT_SUCCESS;
}
