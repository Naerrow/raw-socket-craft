# 프로젝트 계획

## 프로젝트 목적

OS가 제공하는 일반 소켓 API 한 단계 아래에서 IP/ICMP/TCP/UDP 헤더를 직접 다루며 패킷 송수신 구조를 이해한다.

### 일반 소켓과의 차이

- 일반 소켓: OS가 IP 헤더, 포트, 체크섬을 자동으로 붙여줌
- Raw socket: 그 자동화를 걷어내고 직접 바이트를 채워야 함

---

## 언어 선택

**선택: C**

Raw socket 학습은 메모리 레이아웃과 바이트 단위 접근이 핵심이다.
C의 구조체가 실제 헤더 레이아웃과 1:1 대응되어 학습 목적에 가장 직관적이다.
Linux 시스템 콜 예제도 C 기준이 대부분이다.

---

## 3단계 구성

### 1단계: Packet Sniffer (수신)

Raw socket으로 패킷을 수신해 헤더를 파싱하고 사람이 읽을 수 있는 형태로 출력한다.

**핵심 API**
- `socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))`
- `recvfrom()` → IP 헤더 파싱 → ICMP 헤더 파싱

**성공 기준**: tcpdump -n -v 와 동일한 패킷을 같은 내용으로 출력

### 2단계: Packet Crafter (생성)

IP/ICMP 헤더를 바이트 단위로 직접 채워 패킷을 전송한다.

**핵심 API**
- `socket(AF_INET, SOCK_RAW, IPPROTO_RAW)`
- `setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one))`
- 체크섬: 1의 보수 합산 직접 구현

**성공 기준**: Wireshark에서 checksum ✓(valid) 확인

### 3단계: Mini Protocol Lab (설계)

커스텀 헤더를 설계하고, tc netem으로 패킷 유실/지연/순서 역전을 만들어 실험한다.

**성공 기준**: 로그에서 유실된 seq, 역전된 순서를 식별하고 설명 가능

---

## 마일스톤

| 주차 | 작업 내용 | 산출물 |
|------|-----------|--------|
| 1주차 | 환경 구성, 패킷 수신 기본기 | ping 실행 중 sniffer 로그에 ICMP 패킷 출력 화면 |
| 2주차 | 헤더 완전 파싱, 체크섬 검증 | sniffer 출력 vs Wireshark 스크린샷 비교 |
| 3주차 | ICMP Echo Request 직접 생성/전송 | 직접 만든 패킷으로 응답 수신 로그 + Wireshark ✓ 확인 |
| 4주차 | 커스텀 프로토콜 + 유실 실험 문서화 | 각 시나리오별 로그 + test-scenarios.md 완성 |

---

## 위험 요소 및 대응

| 위험 요소 | 대응 |
|-----------|------|
| root 권한 필요 | VM/Docker 내부에서만 실습, 실 네트워크 격리 |
| macOS raw socket 제약 | Linux VM 사용 권장 |
| WSL2 일부 기능 제한 | Ubuntu VM 또는 Docker로 대체 |
| 잘못된 패킷 전송 | loopback(127.0.0.1) 또는 격리된 VM 내부에서만 전송 테스트 |
| tc netem 미지원 환경 | 소프트웨어 레벨에서 난수로 드랍 구현으로 대체 |
