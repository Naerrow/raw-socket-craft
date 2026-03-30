#include "builder.h"
#include "checksum.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>

/* 페이로드: 타임스탬프 없이 단순 ASCII 문자열 사용 */
#define PAYLOAD      "raw-socket-craft"
#define PAYLOAD_LEN  16

int send_icmp_echo(int sock, struct in_addr dst_addr,
                   uint16_t id, uint16_t seq, int bad_csum) {
    /* 전체 패킷 버퍼: IP 헤더 + ICMP 헤더 + 페이로드 */
    char packet[sizeof(struct iphdr) + sizeof(struct icmphdr) + PAYLOAD_LEN];
    memset(packet, 0, sizeof(packet));

    struct iphdr  *iph  = (struct iphdr  *)packet;
    struct icmphdr *icmph = (struct icmphdr *)(packet + sizeof(struct iphdr));
    char *payload = packet + sizeof(struct iphdr) + sizeof(struct icmphdr);

    /* ── IP 헤더 채우기 ── */
    iph->ihl      = 5;                  /* IP 헤더 길이: 5 × 4 = 20 bytes */
    iph->version  = 4;                  /* IPv4 */
    iph->tos      = 0;
    iph->tot_len  = htons(sizeof(packet));
    iph->id       = htons(id);
    iph->frag_off = 0;
    iph->ttl      = 64;
    iph->protocol = IPPROTO_ICMP;
    iph->check    = 0;                  /* 커널이 자동 계산 (IP_HDRINCL 사용 시) */
    iph->saddr    = INADDR_ANY;         /* 커널이 출발지 IP를 채움 */
    iph->daddr    = dst_addr.s_addr;

    /* ── ICMP 헤더 채우기 ── */
    icmph->type             = ICMP_ECHO;  /* 8: Echo Request */
    icmph->code             = 0;
    icmph->checksum         = 0;          /* 체크섬 계산 전 반드시 0으로 초기화 */
    icmph->un.echo.id       = htons(id);
    icmph->un.echo.sequence = htons(seq);

    /* ── 페이로드 채우기 ── */
    memcpy(payload, PAYLOAD, PAYLOAD_LEN);

    /* ── ICMP 체크섬 계산 (ICMP 헤더 + 페이로드 전체) ── */
    int icmp_total = sizeof(struct icmphdr) + PAYLOAD_LEN;
    icmph->checksum = checksum((const uint16_t *)icmph, icmp_total);

    /* 의도적으로 체크섬을 틀리게 하여 Wireshark [incorrect] 확인 */
    if (bad_csum) {
        icmph->checksum = ~icmph->checksum;
        printf("[경고] 의도적으로 잘못된 체크섬을 사용합니다: 0x%04x\n",
               ntohs(icmph->checksum));
    }

    /* ── 전송 ── */
    struct sockaddr_in dst = {
        .sin_family = AF_INET,
        .sin_addr   = dst_addr,
    };

    ssize_t sent = sendto(sock, packet, sizeof(packet), 0,
                          (struct sockaddr *)&dst, sizeof(dst));
    if (sent < 0) {
        perror("[오류] sendto() 실패");
        return -1;
    }

    printf("[전송] dst=%-15s  id=%u  seq=%u  checksum=0x%04x  size=%zd bytes\n",
           inet_ntoa(dst_addr), id, seq, ntohs(icmph->checksum), sent);
    return 0;
}
