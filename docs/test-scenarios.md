# 실험 시나리오 및 기대 결과

## 1단계 시나리오: Packet Sniffer

### 시나리오 1-1: ICMP 패킷 캡처

**준비**
```bash
# 터미널 1: sniffer 실행
sudo ./bin/sniffer

# 터미널 2: ping 전송
ping -c 3 8.8.8.8
```

**기대 출력**
```
[ 1] [IP]   src=192.168.1.10    dst=8.8.8.8         proto=ICMP  ttl=64  len=84
     [ICMP] type=8(Echo Request)  code=0  checksum=0x3e2a  id=1234  seq=1

[ 2] [IP]   src=8.8.8.8         dst=192.168.1.10    proto=ICMP  ttl=117 len=84
     [ICMP] type=0(Echo Reply)    code=0  checksum=0x4629  id=1234  seq=1
```

**검증 포인트**
- sniffer 출력의 src/dst가 `tcpdump -n -v` 출력과 일치하는가?
- ICMP type 8 (Request) → type 0 (Reply) 쌍이 맞는가?
- TTL이 Request(64)와 Reply(117, Google 기준)가 다른가?

---

### 시나리오 1-2: TCP/UDP 패킷 구분

**준비**
```bash
# HTTP 요청 발생
curl http://example.com
```

**기대 출력**
```
[IP]   src=192.168.1.10  dst=93.184.216.34  proto=TCP   ttl=64  len=60
[IP]   src=93.184.216.34 dst=192.168.1.10   proto=TCP   ttl=52  len=52
```

---

## 2단계 시나리오: Packet Crafter

### 시나리오 2-1: 정상 체크섬 확인

**실행**
```bash
sudo ./bin/crafter 127.0.0.1
```

**Wireshark 필터**: `icmp`

**기대 결과**
```
Checksum: 0xXXXX [correct]
```

**확인 포인트**: 체크섬 필드 옆에 ✓ 표시

---

### 시나리오 2-2: 의도적으로 잘못된 체크섬

**실행**
```bash
sudo ./bin/crafter 127.0.0.1 --bad-csum
```

**기대 결과**
```
Checksum: 0xXXXX [incorrect, should be 0xYYYY]
```

**학습 포인트**: OS나 라우터가 잘못된 체크섬의 패킷을 어떻게 처리하는지 관찰

---

## 3단계 시나리오: Protocol Lab

### 시나리오 3-1: 정상 동작 확인

**실행**
```bash
# 터미널 1
sudo ./bin/protocol receiver

# 터미널 2
sudo ./bin/protocol sender
```

**기대 로그 (수신측)**
```
[수신] type=REQUEST   msg_id=1  seq=1  len=21  payload="hello-1"
       → ACK 회신 (msg_id=1)
[수신] type=REQUEST   msg_id=2  seq=2  len=21  payload="hello-2"
       → ACK 회신 (msg_id=2)
```

**확인 포인트**: seq 번호가 1, 2, 3, 4, 5 순서대로 도착하는가?

---

### 시나리오 3-2: 패킷 유실 시뮬레이션

**실행**
```bash
# 유실 설정
sudo ./scripts/simulate-loss.sh loss

# 터미널 1
sudo ./bin/protocol receiver 2>&1 | tee logs/receiver-loss.log

# 터미널 2
sudo ./bin/protocol sender 2>&1 | tee logs/sender-loss.log

# 복원
sudo ./scripts/simulate-loss.sh reset
```

**기대 결과**: 5개 중 평균 0-1개 패킷이 수신 로그에서 누락됨

**분석 포인트**
- 어떤 seq 번호가 누락되었는가?
- message_id 기준으로 어떤 요청이 응답을 받지 못했는가?

**예시 누락 로그**
```
[수신] type=REQUEST  msg_id=1  seq=1  ...   ← 도착
[수신] type=REQUEST  msg_id=3  seq=3  ...   ← seq=2 누락!
[수신] type=REQUEST  msg_id=4  seq=4  ...
```

---

### 시나리오 3-3: 순서 역전 시뮬레이션

**실행**
```bash
sudo ./scripts/simulate-loss.sh reorder

sudo ./bin/protocol receiver 2>&1 | tee logs/receiver-reorder.log
sudo ./bin/protocol sender

sudo ./scripts/simulate-loss.sh reset
```

**기대 결과**: 수신측에서 `[경고] 순서 역전 감지!` 메시지 출력

**예시 역전 로그**
```
[수신] type=REQUEST  msg_id=1  seq=1  ...
[수신] type=REQUEST  msg_id=3  seq=3  ...
[경고] 순서 역전 감지! 이전 seq=3, 현재 seq=2
[수신] type=REQUEST  msg_id=2  seq=2  ...
```

---

### 시나리오 3-4: 지연 시뮬레이션

**실행**
```bash
sudo ./scripts/simulate-loss.sh delay

# time 명령으로 총 소요 시간 측정
time (sudo ./bin/protocol sender)

sudo ./scripts/simulate-loss.sh reset
```

**기대 결과**: 5개 패킷 전송에 정상 대비 약 500ms(5 × 100ms) 추가 소요

---

## 결과 기록 템플릿

실험 후 아래 항목을 `logs/` 디렉토리에 기록합니다.

```
실험 날짜: YYYY-MM-DD
시나리오: [시나리오 번호 및 이름]

환경:
  - OS:
  - 인터페이스:
  - netem 설정:

결과:
  - 전송 패킷 수:
  - 수신 패킷 수:
  - 누락/역전 발생 여부:
  - 누락된 seq 번호:

분석:
  - [관찰한 내용 기록]

스크린샷/로그 파일:
  - logs/[파일명]
```
