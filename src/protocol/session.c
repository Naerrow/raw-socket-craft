#include "session.h"
#include "proto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>

#define CUSTOM_PROTO_NUM 253

ssize_t session_send(int sock, struct sockaddr_in *dst,
                     uint8_t type, uint32_t msg_id, uint32_t seq,
                     const char *payload) {
    char pkt[MAX_PKT_SIZE];
    memset(pkt, 0, sizeof(pkt));

    CustomHeader *hdr = (CustomHeader *)pkt;
    hdr->version         = PROTO_VERSION;
    hdr->type            = type;
    hdr->message_id      = htonl(msg_id);
    hdr->sequence_number = htonl(seq);

    size_t payload_len = 0;
    if (payload != NULL) {
        payload_len = strlen(payload);
        if (payload_len > MAX_PAYLOAD - 1) payload_len = MAX_PAYLOAD - 1;
        memcpy(pkt + sizeof(CustomHeader), payload, payload_len);
    }

    hdr->length = htons((uint16_t)(sizeof(CustomHeader) + payload_len));

    size_t total = sizeof(CustomHeader) + payload_len;
    ssize_t sent = sendto(sock, pkt, total, 0,
                          (struct sockaddr *)dst, sizeof(*dst));
    if (sent < 0) {
        perror("[오류] sendto() 실패");
    }
    return sent;
}

/* 공통 수신 파싱 로직 */
static int parse_recv_buf(const char *buf, ssize_t recv_len,
                          CustomHeader *hdr, char *payload,
                          struct sockaddr_in *src_addr) {
    const struct iphdr *iph = (const struct iphdr *)buf;
    int ip_hdr_len = iph->ihl * 4;

    if (iph->protocol != CUSTOM_PROTO_NUM) return 0;

    if (recv_len < ip_hdr_len + (int)sizeof(CustomHeader)) return 0;

    const char *proto_start = buf + ip_hdr_len;
    memcpy(hdr, proto_start, sizeof(CustomHeader));

    hdr->length          = ntohs(hdr->length);
    hdr->message_id      = ntohl(hdr->message_id);
    hdr->sequence_number = ntohl(hdr->sequence_number);

    if (hdr->version != PROTO_VERSION) {
        fprintf(stderr, "[경고] 알 수 없는 프로토콜 버전: %u\n", hdr->version);
        return 0;
    }

    if (payload != NULL) {
        int payload_len = hdr->length - (int)sizeof(CustomHeader);
        if (payload_len > 0 && payload_len < MAX_PAYLOAD) {
            memcpy(payload, proto_start + sizeof(CustomHeader), payload_len);
            payload[payload_len] = '\0';
        } else {
            payload[0] = '\0';
        }
    }

    /* src_addr에 실제 송신 IP를 채운다 */
    if (src_addr != NULL) {
        src_addr->sin_family = AF_INET;
        src_addr->sin_addr   = (struct in_addr){ .s_addr = iph->saddr };
    }

    return 1;
}

int session_recv(int sock, CustomHeader *hdr, char *payload,
                 struct sockaddr_in *src) {
    char buf[sizeof(struct iphdr) + MAX_PKT_SIZE];
    socklen_t src_len = sizeof(struct sockaddr_in);

    ssize_t recv_len = recvfrom(sock, buf, sizeof(buf), 0,
                                (struct sockaddr *)src, &src_len);
    if (recv_len < 0) return 0;

    return parse_recv_buf(buf, recv_len, hdr, payload, src);
}

int session_recv_timeout(int sock, CustomHeader *hdr, char *payload,
                         struct sockaddr_in *src, int timeout_ms) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);

    struct timeval tv = {
        .tv_sec  = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
    };

    int ret = select(sock + 1, &rfds, NULL, NULL, &tv);
    if (ret <= 0) return 0;  /* 타임아웃 또는 오류 */

    return session_recv(sock, hdr, payload, src);
}

void session_print(const char *direction, const CustomHeader *hdr,
                   const char *payload) {
    printf("[%s] type=%-8s  msg_id=%u  seq=%u  len=%u",
           direction,
           proto_type_name(hdr->type),
           hdr->message_id,
           hdr->sequence_number,
           hdr->length);

    if (payload != NULL && payload[0] != '\0') {
        printf("  payload=\"%s\"", payload);
    }
    printf("\n");
    fflush(stdout);
}
