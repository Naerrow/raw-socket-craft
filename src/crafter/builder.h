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

#endif /* BUILDER_H */
