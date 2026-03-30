#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>

/* 커스텀 프로토콜 버전 */
#define PROTO_VERSION  1

/* 메시지 타입 */
#define PROTO_REQUEST   0
#define PROTO_RESPONSE  1
#define PROTO_ACK       2

/**
 * 커스텀 프로토콜 헤더
 *
 * 총 12바이트 고정 크기
 * __attribute__((packed)): 컴파일러 패딩 없이 바이트 단위로 배치
 *
 * 0        1        2        3
 * +--------+--------+--------+--------+
 * |version |  type  |      length     |
 * +--------+--------+--------+--------+
 * |             message_id            |
 * +--------+--------+--------+--------+
 * |          sequence_number          |
 * +--------+--------+--------+--------+
 */
typedef struct __attribute__((packed)) {
    uint8_t  version;           /* 프로토콜 버전 (현재 PROTO_VERSION=1) */
    uint8_t  type;              /* PROTO_REQUEST / PROTO_RESPONSE / PROTO_ACK */
    uint16_t length;            /* 헤더 포함 전체 패킷 길이 (빅엔디안) */
    uint32_t message_id;        /* 요청/응답 매칭용 고유 ID (빅엔디안) */
    uint32_t sequence_number;   /* 순서 추적용 (빅엔디안) */
} CustomHeader;

/* 페이로드 최대 크기 */
#define MAX_PAYLOAD 256

/* 전체 패킷 최대 크기 */
#define MAX_PKT_SIZE (sizeof(CustomHeader) + MAX_PAYLOAD)

/* type 값을 문자열로 변환 */
static inline const char *proto_type_name(uint8_t type) {
    switch (type) {
        case PROTO_REQUEST:  return "REQUEST";
        case PROTO_RESPONSE: return "RESPONSE";
        case PROTO_ACK:      return "ACK";
        default:             return "UNKNOWN";
    }
}

#endif /* PROTO_H */
