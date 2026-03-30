#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

/* IP 헤더 파싱 결과를 담는 구조체 */
typedef struct {
    char   src[16];    /* 출발지 IP 문자열 (예: "192.168.1.1") */
    char   dst[16];    /* 목적지 IP 문자열 */
    uint8_t  proto;    /* 프로토콜 번호 (ICMP=1, TCP=6, UDP=17) */
    uint8_t  ttl;
    uint16_t total_len;
    uint8_t  ihl;      /* IP 헤더 길이 (4바이트 단위) */
} IpInfo;

/* ICMP 헤더 파싱 결과를 담는 구조체 */
typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} IcmpInfo;

/* IP 헤더를 파싱하고 출력한다. 성공 시 1, 실패 시 0 반환 */
int parse_ip_header(const uint8_t *buf, int buf_len, IpInfo *out);

/* ICMP 헤더를 파싱하고 출력한다. IP 헤더 이후 데이터를 받는다. */
void parse_icmp_header(const uint8_t *buf, int buf_len);

/* 프로토콜 번호를 이름 문자열로 변환 */
const char *proto_name(uint8_t proto);

/* ICMP type을 설명 문자열로 변환 */
const char *icmp_type_name(uint8_t type);

#endif /* PARSER_H */
