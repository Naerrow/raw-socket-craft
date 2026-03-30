#!/usr/bin/env bash
# Packet Sniffer 실행 스크립트
#
# 사용법:
#   ./scripts/run-sniffer.sh
#
# 검증 방법:
#   별도 터미널에서 ping 8.8.8.8 을 실행한 후
#   sniffer 출력과 아래 tcpdump 출력을 비교한다.
#
#   tcpdump -n -v -i any icmp

set -e
cd "$(dirname "$0")/.."

if [ ! -f bin/sniffer ]; then
    echo "[오류] bin/sniffer 가 없습니다. 먼저 'make sniffer' 를 실행하세요."
    exit 1
fi

echo "=== Packet Sniffer 시작 ==="
echo "검증: 다른 터미널에서 'ping 8.8.8.8' 을 실행해 보세요."
echo "비교: 'tcpdump -n -v -i any icmp' 출력과 비교하세요."
echo ""

# CAP_NET_RAW가 없으면 sudo로 실행
if ! getcap bin/sniffer 2>/dev/null | grep -q cap_net_raw; then
    exec sudo bin/sniffer "$@"
else
    exec bin/sniffer "$@"
fi
