# 체크섬 알고리즘 상세 설명

## 개요

IP와 ICMP 헤더에 사용되는 체크섬은 **1의 보수(one's complement) 합산** 방식입니다. (RFC 791, RFC 792)

---

## 알고리즘 단계별 설명

### 1단계: 체크섬 필드를 0으로 초기화

체크섬을 계산하기 전에 반드시 체크섬 필드를 0으로 채워야 합니다.
계산 후 결과값을 해당 필드에 넣습니다.

```c
icmph->checksum = 0;   /* 반드시 먼저 0으로 초기화 */
icmph->checksum = checksum((uint16_t *)icmph, icmp_total_len);
```

### 2단계: 16비트 단위로 합산

버퍼를 2바이트(16비트)씩 읽어 32비트 누산기에 더합니다.

```
버퍼: [0x45 0x00] [0x00 0x54] [0x12 0x34] ...
       └── 0x4500  └── 0x0054  └── 0x1234
sum = 0x4500 + 0x0054 + 0x1234 + ...
```

### 3단계: 홀수 바이트 처리

버퍼 길이가 홀수인 경우 마지막 1바이트를 16비트로 취급해 더합니다.

```c
if (len == 1) sum += *(uint8_t *)buf;   /* 상위 바이트로 처리됨 */
```

### 4단계: 올림수(carry) 처리

32비트 합산 결과의 상위 16비트(올림수)를 하위 16비트에 더합니다.
이 과정을 올림수가 없어질 때까지 반복합니다.

```c
sum = (sum >> 16) + (sum & 0xffff);
sum += (sum >> 16);
```

**예시**:
```
sum = 0x0001FFFE
→ (0x0001) + (0xFFFE) = 0xFFFF
→ 올림수 없음, 완료
```

### 5단계: 1의 보수 반환

최종 16비트 값을 비트 반전(NOT)해서 반환합니다.

```c
return ~sum;
```

---

## 전체 코드

```c
uint16_t checksum(const uint16_t *buf, int len) {
    uint32_t sum = 0;

    /* 16비트 단위로 합산 */
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    /* 홀수 바이트 처리 */
    if (len == 1) {
        sum += *(const uint8_t *)buf;
    }

    /* 올림수 처리 */
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    /* 1의 보수 반환 */
    return (uint16_t)(~sum);
}
```

---

## 검증 방법

### 정상 체크섬 확인

```bash
sudo ./bin/crafter 8.8.8.8
```

Wireshark에서 ICMP 패킷을 캡처하면:
```
Internet Control Message Protocol
    Type: 8 (Echo (ping) request)
    Code: 0
    Checksum: 0x3e2a [correct]     ← ✓ 표시
```

### 의도적으로 틀린 체크섬 확인

```bash
sudo ./bin/crafter 8.8.8.8 --bad-csum
```

Wireshark에서:
```
Internet Control Message Protocol
    Checksum: 0xc1d5 [incorrect, should be 0x3e2a]   ← [incorrect] 표시
```

---

## 바이트 오더(Endianness) 주의사항

네트워크 프로토콜은 **빅엔디안(Big-Endian)** 을 사용합니다 (네트워크 바이트 오더).
x86 CPU는 **리틀엔디안(Little-Endian)** 을 사용합니다.

변환 함수:
- `htons()` — host to network short (16비트)
- `htonl()` — host to network long (32비트)
- `ntohs()` — network to host short
- `ntohl()` — network to host long

체크섬 함수 자체는 바이트 오더 변환 없이 버퍼를 그대로 읽습니다.
패킷의 바이트 배열이 이미 네트워크 바이트 오더이기 때문입니다.
