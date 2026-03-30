#!/usr/bin/env bash
# Packet Crafter 실행 스크립트
#
# 사용법:
#   ./scripts/run-crafter.sh <목적지IP> [--bad-csum]
#
# 검증 방법:
#   Wireshark에서 ICMP 패킷을 캡처하고 checksum 필드 확인:
#     정상:  [correct] 또는 ✓ 표시
#     비정상: [incorrect] 표시 (--bad-csum 옵션 사용 시)

set -e
cd "$(dirname "$0")/.."

if [ ! -f bin/crafter ]; then
    echo "[오류] bin/crafter 가 없습니다. 먼저 'make crafter' 를 실행하세요."
    exit 1
fi

if [ -z "$1" ]; then
    echo "사용법: $0 <목적지IP> [--bad-csum]"
    echo "  예시: $0 8.8.8.8"
    echo "  예시: $0 8.8.8.8 --bad-csum"
    exit 1
fi

echo "=== Packet Crafter 시작 ==="
echo "목적지: $1"
echo "Wireshark에서 ICMP 패킷의 checksum 필드를 확인하세요."
echo ""

if ! getcap bin/crafter 2>/dev/null | grep -q cap_net_raw; then
    exec sudo bin/crafter "$@"
else
    exec bin/crafter "$@"
fi
