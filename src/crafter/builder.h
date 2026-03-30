#ifndef BUILDER_H
#define BUILDER_H

#include <stdint.h>
#include <netinet/in.h>

/**
 * ICMP Echo Request 패킷을 조립하여 전송한다.
 *
 * IP_HDRINCL 옵션을 사용해 IP 헤더를 직접 채운다.
 * ICMP 체크섬은 checksum() 함수로 직접 계산한다.
 *
 * @param sock      IP_HDRINCL가 설정된 raw socket
 * @param dst_addr  목적지 IP 주소
 * @param id        ICMP Echo ID
 * @param seq       ICMP 시퀀스 번호
 * @param bad_csum  1이면 의도적으로 잘못된 체크섬을 넣어 Wireshark [incorrect] 확인용
 * @return 전송 성공 시 0, 실패 시 -1
 */
int send_icmp_echo(int sock, struct in_addr dst_addr,
                   uint16_t id, uint16_t seq, int bad_csum);

/**
 * ICMP Echo Reply를 수신하고 RTT를 반환한다.
 *
 * @param recv_sock  IPPROTO_ICMP raw socket (수신 전용)
 * @param id         기다릴 Echo ID (송신 시 사용한 값과 일치해야 함)
 * @param seq        기다릴 시퀀스 번호
 * @param timeout_ms 최대 대기 시간 (밀리초)
 * @param rtt_ms     측정된 RTT를 저장할 포인터
 * @return 수신 성공 시 0, 타임아웃 시 -1
 */
int recv_icmp_reply(int recv_sock, uint16_t id, uint16_t seq,
                    int timeout_ms, double *rtt_ms);

#endif /* BUILDER_H */
