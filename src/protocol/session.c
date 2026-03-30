#include "session.h"
#include "proto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

/* 커스텀 프로토콜 포트 번호 (UDP raw socket에서 필터링용) */
#define CUSTOM_PROTO_NUM 253   /* 실험용 프로토콜 번호 (IANA 미지정 범위) */

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

    /* length 필드: 헤더 + 페이로드 전체 크기 */
    hdr->length = htons((uint16_t)(sizeof(CustomHeader) + payload_len));

    size_t total = sizeof(CustomHeader) + payload_len;
    ssize_t sent = sendto(sock, pkt, total, 0,
                          (struct sockaddr *)dst, sizeof(*dst));
    if (sent < 0) {
        perror("[오류] sendto() 실패");
    }
    return sent;
}

int session_recv(int sock, CustomHeader *hdr, char *payload,
                 struct sockaddr_in *src) {
    char buf[sizeof(struct iphdr) + MAX_PKT_SIZE];
    socklen_t src_len = sizeof(struct sockaddr_in);

    ssize_t recv_len = recvfrom(sock, buf, sizeof(buf), 0,
                                (struct sockaddr *)src, &src_len);
    if (recv_len < 0) {
        perror("[오류] recvfrom() 실패");
        return 0;
    }

    /* IP 헤더 건너뛰기 */
    const struct iphdr *iph = (const struct iphdr *)buf;
    int ip_hdr_len = iph->ihl * 4;

    /* 우리 커스텀 프로토콜 번호가 맞는지 확인 */
    if (iph->protocol != CUSTOM_PROTO_NUM) return 0;

    if (recv_len < ip_hdr_len + (int)sizeof(CustomHeader)) {
        fprintf(stderr, "[경고] 패킷이 너무 짧습니다 (%zd bytes)\n", recv_len);
        return 0;
    }

    const char *proto_start = buf + ip_hdr_len;
    memcpy(hdr, proto_start, sizeof(CustomHeader));

    /* 네트워크 바이트 오더 → 호스트 바이트 오더 변환 */
    hdr->length          = ntohs(hdr->length);
    hdr->message_id      = ntohl(hdr->message_id);
    hdr->sequence_number = ntohl(hdr->sequence_number);

    /* 버전 검사 */
    if (hdr->version != PROTO_VERSION) {
        fprintf(stderr, "[경고] 알 수 없는 프로토콜 버전: %u\n", hdr->version);
        return 0;
    }

    /* 페이로드 복사 */
    if (payload != NULL) {
        int payload_len = hdr->length - (int)sizeof(CustomHeader);
        if (payload_len > 0 && payload_len < MAX_PAYLOAD) {
            memcpy(payload, proto_start + sizeof(CustomHeader), payload_len);
            payload[payload_len] = '\0';
        } else {
            payload[0] = '\0';
        }
    }

    return 1;
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
