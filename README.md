# Raw Socket Craft

OS가 제공하는 소켓 API 한 단계 아래에서 IP/ICMP 헤더를 직접 다루며 패킷 송수신 구조를 이해하는 실습 프로젝트입니다.

---

## 프로젝트 목적

### 일반 소켓 vs Raw Socket

| 구분 | 일반 소켓 | Raw Socket |
|------|-----------|------------|
| IP 헤더 | OS가 자동으로 붙여줌 | 직접 채워야 함 |
| 포트 관리 | OS가 자동으로 관리 | 직접 처리해야 함 |
| 체크섬 | OS가 자동으로 계산 | 직접 계산해야 함 |
| 학습 깊이 | 추상화된 API 사용 | 바이트 단위 구조 이해 |

### 최종적으로 눈으로 확인할 것

- 캡처한 패킷 헤더를 코드가 정확히 파싱해서 출력
- 직접 채운 ICMP 헤더로 Echo Request 전송 성공
- 체크섬을 직접 계산해 Wireshark 결과와 일치 확인
- 커스텀 프로토콜 패킷이 유실/순서 역전 상황에서 어떻게 동작하는지 로그로 설명 가능

---

## 실습 환경

### 필수 조건

- Linux (Ubuntu 22.04 LTS 권장)
- root 권한 또는 `CAP_NET_RAW` capability
- 분석 도구: `tcpdump`, `Wireshark`

### 환경 구성 방법

```bash
# 옵션 1: Docker (격리 환경으로 안전, 빠른 시작)
docker run --rm -it --net=host --cap-add=NET_RAW ubuntu:22.04

# 옵션 2: VirtualBox/VMware에 Ubuntu 22.04 설치 (가장 안정적)
# ISO: https://ubuntu.com/download/server

# Docker 내부에서 필수 도구 설치
apt-get update && apt-get install -y \
  build-essential \
  tcpdump \
  iproute2 \
  wireshark-common \
  iputils-ping
```

### sudo 없이 실행하는 방법

```bash
# 빌드 후 capability 부여
sudo setcap cap_net_raw+ep ./sniffer
sudo setcap cap_net_raw+ep ./crafter
```

> **주의**: macOS는 raw socket API가 Linux와 달라 실습 내용이 그대로 적용되지 않을 수 있습니다.
> WSL2는 일부 raw socket 기능에 제약이 있어 VM 또는 Docker를 권장합니다.

---

## 폴더 구조

```
raw-socket-craft/
├─ README.md                  # 이 파일
├─ Makefile                   # 빌드 설정
├─ docs/
│  ├─ plan.md                 # 전체 프로젝트 계획
│  ├─ packet-format.md        # 헤더 다이어그램 및 필드 설명
│  ├─ checksum.md             # 체크섬 알고리즘 상세 설명
│  └─ test-scenarios.md       # 실험 시나리오별 기대 결과
├─ src/
│  ├─ sniffer/                # 1단계: 패킷 수신 및 파싱
│  │  ├─ main.c
│  │  ├─ parser.c
│  │  └─ parser.h
│  ├─ crafter/                # 2단계: 패킷 직접 생성 및 전송
│  │  ├─ main.c
│  │  ├─ checksum.c
│  │  ├─ checksum.h
│  │  ├─ builder.c
│  │  └─ builder.h
│  └─ protocol/               # 3단계: 커스텀 프로토콜
│     ├─ main.c
│     ├─ proto.h
│     ├─ session.c
│     └─ session.h
├─ scripts/
│  ├─ setup-env.sh            # 환경 구성 스크립트
│  ├─ run-sniffer.sh          # sniffer 실행
│  ├─ run-crafter.sh          # crafter 실행
│  └─ simulate-loss.sh        # tc netem 패킷 유실/지연 시뮬레이션
├─ samples/
│  └─ captured-packets/       # .pcap 파일 보관
└─ logs/                      # 실험 로그 보관
```

---

## 단계별 학습 목표

### 1단계: Packet Sniffer (수신)

Raw socket으로 패킷을 수신해 헤더를 파싱하고 사람이 읽을 수 있는 형태로 출력합니다.

**기대 출력**
```
[IP]   src=192.168.1.10  dst=8.8.8.8  proto=ICMP  ttl=64  len=84
[ICMP] type=8(Echo Request)  code=0  checksum=0x3e2a  id=1234  seq=1
```

**성공 기준**: `tcpdump -n -v`와 동일한 패킷을 같은 내용으로 출력

### 2단계: Packet Crafter (생성)

IP/ICMP 헤더를 바이트 단위로 직접 채워 패킷을 전송합니다.

**핵심 구현**: 1의 보수 합산 체크섬 직접 계산

**성공 기준**: Wireshark에서 checksum ✓(valid) 표시 확인. 틀린 체크섬을 넣으면 [incorrect] 표시됨

### 3단계: Mini Protocol Lab (설계)

커스텀 헤더를 설계하고, 의도적으로 패킷 유실/지연/순서 역전을 만들어 실험합니다.

**커스텀 헤더 구조**

| 필드 | 크기 | 설명 |
|------|------|------|
| version | 1 byte | 프로토콜 버전 (현재 1) |
| type | 1 byte | 0=REQUEST, 1=RESPONSE, 2=ACK |
| length | 2 bytes | 전체 패킷 길이 |
| message_id | 4 bytes | 요청/응답 매칭용 ID |
| sequence_number | 4 bytes | 순서 추적용 |

**성공 기준**: 유실/순서 역전 로그에서 어떤 필드에서 문제가 발생했는지 설명 가능

---

## 빌드 및 실행 방법

```bash
# 전체 빌드
make all

# 단계별 빌드
make sniffer
make crafter
make protocol

# 실행 (Linux에서 root 권한 필요)
sudo ./sniffer              # 모든 인터페이스에서 패킷 수신
sudo ./crafter 8.8.8.8     # 대상 IP로 ICMP Echo Request 전송
sudo ./protocol             # 커스텀 프로토콜 송수신 테스트

# 정리
make clean
```

---

## 마일스톤

| 주차 | 작업 내용 | 산출물 |
|------|-----------|--------|
| 1주차 | 환경 구성, 패킷 수신 기본기 | ping 실행 중 sniffer 로그에 ICMP 패킷 출력 화면 |
| 2주차 | 헤더 완전 파싱, 체크섬 검증 | sniffer 출력 vs Wireshark 스크린샷 비교 |
| 3주차 | ICMP Echo Request 직접 생성/전송 | 직접 만든 패킷으로 응답 수신 로그 + Wireshark ✓ 확인 |
| 4주차 | 커스텀 프로토콜 + 유실 실험 문서화 | 각 시나리오별 로그 + test-scenarios.md 완성 |

---

## 성공 기준 체크리스트

- [ ] 수신한 패킷의 IP/ICMP 헤더 필드를 코드가 정확히 파싱해서 출력
- [ ] 직접 계산한 체크섬이 Wireshark 체크섬 값과 일치
- [ ] 직접 만든 ICMP Echo Request로 응답 수신 성공
- [ ] 체크섬을 틀리게 넣었을 때 Wireshark가 [incorrect]로 표시하는 것 확인
- [ ] 커스텀 프로토콜 요청/응답 동작 확인
- [ ] 패킷 유실/순서 역전 시뮬레이션 로그를 보고 어떤 필드에서 문제가 발생했는지 설명 가능

---

## 참고 자료

| 자료 | 설명 |
|------|------|
| `man 7 raw` | Linux raw socket 매뉴얼 |
| `man 7 ip` | IP 소켓 옵션 (`IP_HDRINCL` 등) |
| RFC 791 | IP 공식 스펙 |
| RFC 792 | ICMP 공식 스펙 |
| [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) | C 소켓 프로그래밍 입문서 |
