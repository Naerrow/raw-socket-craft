# 패킷 헤더 구조

## IPv4 헤더 (20바이트, RFC 791)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Version|  IHL  |Type of Service|          Total Length         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Identification        |Flags|      Fragment Offset    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Time to Live |    Protocol   |         Header Checksum       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Source Address                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Destination Address                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| 필드 | 크기 | 설명 |
|------|------|------|
| Version | 4 bits | IP 버전 (IPv4 = 4) |
| IHL | 4 bits | IP 헤더 길이 (4바이트 단위, 보통 5 = 20바이트) |
| TOS | 1 byte | 서비스 타입 (보통 0) |
| Total Length | 2 bytes | IP 헤더 + 데이터 전체 길이 |
| Identification | 2 bytes | 단편화(fragmentation) 식별 ID |
| Flags | 3 bits | 단편화 제어 (DF, MF) |
| Fragment Offset | 13 bits | 단편화된 패킷의 위치 |
| TTL | 1 byte | Time to Live — 라우터를 거칠 때마다 1씩 감소, 0이 되면 폐기 |
| Protocol | 1 byte | 상위 프로토콜 (ICMP=1, TCP=6, UDP=17) |
| Header Checksum | 2 bytes | IP 헤더 체크섬 (IP_HDRINCL 시 직접 계산) |
| Source Address | 4 bytes | 출발지 IP |
| Destination Address | 4 bytes | 목적지 IP |

---

## ICMP 헤더 (8바이트, RFC 792)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Type      |     Code      |          Checksum             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           Identifier          |        Sequence Number        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| 필드 | 크기 | 설명 |
|------|------|------|
| Type | 1 byte | 메시지 타입 (아래 표 참조) |
| Code | 1 byte | 타입 내 세부 코드 |
| Checksum | 2 bytes | ICMP 헤더 + 데이터 전체에 대한 체크섬 |
| Identifier | 2 bytes | Echo 요청/응답 매칭용 ID |
| Sequence Number | 2 bytes | 순서 번호 |

### ICMP Type 주요 값

| Type | 이름 | 설명 |
|------|------|------|
| 0 | Echo Reply | ping 응답 |
| 3 | Destination Unreachable | 목적지 도달 불가 |
| 8 | Echo Request | ping 요청 |
| 11 | Time Exceeded | TTL 초과 (traceroute에 사용됨) |

---

## 커스텀 프로토콜 헤더 (12바이트)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Version    |     Type      |            Length             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          Message ID                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Sequence Number                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| 필드 | 크기 | 설명 |
|------|------|------|
| Version | 1 byte | 프로토콜 버전 (현재 1) |
| Type | 1 byte | 0=REQUEST, 1=RESPONSE, 2=ACK |
| Length | 2 bytes | 헤더 포함 전체 길이 (빅엔디안) |
| Message ID | 4 bytes | 요청/응답 매칭용 고유 ID |
| Sequence Number | 4 bytes | 순서 역전 감지용 |
