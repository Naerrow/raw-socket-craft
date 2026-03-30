#!/usr/bin/env bash
# 실습 환경 구성 스크립트 (Ubuntu 22.04 기준)
set -e

echo "=== Raw Socket Craft 환경 구성 ==="
echo ""

# root 여부 확인
if [ "$(id -u)" -ne 0 ]; then
    echo "[오류] 이 스크립트는 root 권한으로 실행해야 합니다."
    echo "       sudo $0 을 사용하세요."
    exit 1
fi

echo "[1/3] 필수 패키지 설치 중..."
apt-get update -qq
apt-get install -y \
    build-essential \
    tcpdump \
    iproute2 \
    wireshark-common \
    iputils-ping \
    net-tools

echo ""
echo "[2/3] 빌드 중..."
cd "$(dirname "$0")/.."
make all

echo ""
echo "[3/3] CAP_NET_RAW capability 부여 (sudo 없이 실행 가능하도록)..."
setcap cap_net_raw+ep bin/sniffer
setcap cap_net_raw+ep bin/crafter
setcap cap_net_raw+ep bin/protocol

echo ""
echo "=== 환경 구성 완료 ==="
echo ""
echo "실행 방법:"
echo "  ./bin/sniffer              # 패킷 수신"
echo "  ./bin/crafter 8.8.8.8     # ICMP Echo Request 전송"
echo "  ./bin/protocol receiver   # 커스텀 프로토콜 수신"
echo "  ./bin/protocol sender     # 커스텀 프로토콜 송신"
