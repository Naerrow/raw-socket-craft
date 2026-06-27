#ifndef SESSION_H
#define SESSION_H

#include "proto.h"
#include <netinet/in.h>

/* 세션 상태 */
#define SESSION_WAITING   0   /* 응답 대기 중 */
#define SESSION_COMPLETED 1   /* 응답 수신 완료 */
#define SESSION_TIMEOUT   2   /* 타임아웃 */

/**
 * 단일 요청/응답 세션을 추적하는 구조체
 */
typedef struct {
    uint32_t message_id;
    uint32_t sequence_number;
    int      status;           /* SESSION_WAITING / COMPLETED / TIMEOUT */
} Session;

/**
 * 커스텀 헤더를 포함한 패킷을 전송한다.
 *
 * @param sock       raw socket
 * @param dst        목적지 주소
 * @param type       PROTO_REQUEST / PROTO_RESPONSE / PROTO_ACK
 * @param msg_id     메시지 ID
 * @param seq        시퀀스 번호
 * @param payload    페이로드 문자열 (NULL 가능)
 * @return 전송 바이트 수, 실패 시 -1
 */
ssize_t session_send(int sock, struct sockaddr_in *dst,
                     uint8_t type, uint32_t msg_id, uint32_t seq,
                     const char *payload);

/**
 * 패킷을 수신하고 커스텀 헤더를 파싱한다. (블로킹)
 *
 * @param sock    raw socket
 * @param hdr     파싱된 헤더를 저장할 버퍼
 * @param payload 페이로드를 저장할 버퍼 (MAX_PAYLOAD 크기)
 * @param src     송신자 주소를 저장할 버퍼 (NULL 가능)
 * @return 수신 성공 시 1, 실패 시 0
 */
int session_recv(int sock, CustomHeader *hdr, char *payload,
                 struct sockaddr_in *src);

/**
 * 타임아웃이 있는 패킷 수신. select()로 대기하여 Ctrl+C 반응 가능.
 *
 * @param sock       raw socket
 * @param hdr        파싱된 헤더를 저장할 버퍼
 * @param payload    페이로드를 저장할 버퍼
 * @param src        송신자 주소 (NULL 가능)
 * @param timeout_ms 최대 대기 시간 (밀리초). 0이면 즉시 반환.
 * @return 수신 성공 시 1, 타임아웃/실패 시 0
 */
int session_recv_timeout(int sock, CustomHeader *hdr, char *payload,
                         struct sockaddr_in *src, int timeout_ms);

/**
 * 수신한 헤더 정보를 사람이 읽기 쉬운 형태로 출력한다.
 */
void session_print(const char *direction, const CustomHeader *hdr,
                   const char *payload);

#endif /* SESSION_H */
