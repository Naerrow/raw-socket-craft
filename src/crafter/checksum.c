#include "checksum.h"

/**
 * 1의 보수 합산 체크섬 계산
 *
 * 알고리즘:
 * 1. 버퍼를 16비트 단위로 읽어 모두 더한다.
 * 2. 버퍼 길이가 홀수이면 마지막 1바이트를 상위 바이트로 패딩하여 더한다.
 * 3. 합산 결과의 올림수(carry)를 하위 16비트에 더한다.
 * 4. 최종 값의 1의 보수(비트 반전)를 반환한다.
 *
 * 검증 방법:
 * - 체크섬을 올바르게 계산하면 Wireshark에서 ✓(valid) 표시
 * - 틀린 값을 넣으면 [incorrect] 표시
 */
uint16_t checksum(const uint16_t *buf, int len) {
    uint32_t sum = 0;

    /* 16비트 단위로 합산 */
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    /* 홀수 바이트가 남은 경우 처리 */
    if (len == 1) {
        sum += *(const uint8_t *)buf;
    }

    /* 올림수를 하위 16비트에 누적 */
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    /* 1의 보수 반환 */
    return (uint16_t)(~sum);
}
