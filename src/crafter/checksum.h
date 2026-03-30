#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>

/**
 * 1의 보수 합산 체크섬 계산 (RFC 791, RFC 792)
 *
 * IP/ICMP 헤더의 체크섬 필드를 0으로 세팅한 뒤 이 함수를 호출한다.
 * 반환값을 그대로 헤더의 checksum 필드에 저장하면 된다.
 *
 * @param buf 체크섬을 계산할 버퍼 (16비트 단위로 읽힘)
 * @param len 버퍼 길이 (바이트)
 * @return 계산된 체크섬 (네트워크 바이트 오더)
 */
uint16_t checksum(const uint16_t *buf, int len);

#endif /* CHECKSUM_H */
