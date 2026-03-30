#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

/* 프로토콜 번호 → 이름 매핑 */
const char *proto_name(uint8_t proto) {
    switch (proto) {
        case 1:  return "ICMP";
        case 6:  return "TCP";
        case 17: return "UDP";
        default: return "OTHER";
    }
}

/* ICMP type → 설명 문자열 매핑 */
const char *icmp_type_name(uint8_t type) {
    switch (type) {
        case 0:  return "Echo Reply";
        case 3:  return "Dest Unreachable";
        case 8:  return "Echo Request";
        case 11: return "Time Exceeded";
        default: return "Unknown";
    }
}

/* IP 헤더 파싱 및 출력 */
int parse_ip_header(const uint8_t *buf, int buf_len, IpInfo *out) {
    if (buf_len < (int)sizeof(struct iphdr)) {
        fprintf(stderr, "[경고] 버퍼가 IP 헤더보다 짧습니다 (%d bytes)\n", buf_len);
        return 0;
    }

    const struct iphdr *iph = (const struct iphdr *)buf;

    /* 출발지/목적지 IP를 문자열로 변환 */
    struct in_addr src_addr = { .s_addr = iph->saddr };
    struct in_addr dst_addr = { .s_addr = iph->daddr };
    strncpy(out->src, inet_ntoa(src_addr), sizeof(out->src) - 1);
    strncpy(out->dst, inet_ntoa(dst_addr), sizeof(out->dst) - 1);

    out->proto     = iph->protocol;
    out->ttl       = iph->ttl;
    out->total_len = ntohs(iph->tot_len);
    out->ihl       = iph->ihl;

    printf("[IP]   src=%-15s dst=%-15s proto=%-5s ttl=%3u len=%u\n",
           out->src, out->dst, proto_name(out->proto), out->ttl, out->total_len);

    return 1;
}

/* ICMP 헤더 파싱 및 출력 */
void parse_icmp_header(const uint8_t *buf, int buf_len) {
    if (buf_len < (int)sizeof(struct icmphdr)) {
        fprintf(stderr, "[경고] 버퍼가 ICMP 헤더보다 짧습니다 (%d bytes)\n", buf_len);
        return;
    }

    const struct icmphdr *icmph = (const struct icmphdr *)buf;

    printf("[ICMP] type=%u(%s)  code=%u  checksum=0x%04x  id=%u  seq=%u\n",
           icmph->type,
           icmp_type_name(icmph->type),
           icmph->code,
           ntohs(icmph->checksum),
           ntohs(icmph->un.echo.id),
           ntohs(icmph->un.echo.sequence));
}
