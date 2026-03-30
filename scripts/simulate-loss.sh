#!/usr/bin/env bash
# tc netem을 이용한 네트워크 시뮬레이션 스크립트
#
# 사용법:
#   sudo ./scripts/simulate-loss.sh [명령어]
#
# 명령어:
#   loss     - 10% 패킷 유실
#   delay    - 100ms 지연
#   reorder  - 50% 순서 역전
#   corrupt  - 1% 패킷 손상
#   show     - 현재 설정 확인
#   reset    - 초기화
#
# 예시 실험 순서:
#   1. sudo ./simulate-loss.sh loss     (유실 설정)
#   2. ./bin/protocol sender            (패킷 전송)
#   3. logs/ 에서 seq 번호 누락 확인
#   4. sudo ./simulate-loss.sh reset    (복원)

set -e

IFACE="${SIMULATE_IFACE:-lo}"   # 기본값: loopback (환경변수로 변경 가능)

if [ "$(id -u)" -ne 0 ]; then
    echo "[오류] root 권한이 필요합니다. sudo $0 $* 을 사용하세요."
    exit 1
fi

CMD="${1:-help}"

case "$CMD" in
    loss)
        echo "=== 10% 패킷 유실 설정 ($IFACE) ==="
        tc qdisc add dev "$IFACE" root netem loss 10%
        echo "설정 완료. 복원: sudo $0 reset"
        ;;

    delay)
        echo "=== 100ms 지연 설정 ($IFACE) ==="
        tc qdisc add dev "$IFACE" root netem delay 100ms
        echo "설정 완료. 복원: sudo $0 reset"
        ;;

    reorder)
        echo "=== 50% 순서 역전 설정 ($IFACE) ==="
        # delay 없이는 reorder가 동작하지 않으므로 10ms 기본 지연 추가
        tc qdisc add dev "$IFACE" root netem delay 10ms reorder 50% gap 1
        echo "설정 완료. 복원: sudo $0 reset"
        ;;

    corrupt)
        echo "=== 1% 패킷 손상 설정 ($IFACE) ==="
        tc qdisc add dev "$IFACE" root netem corrupt 1%
        echo "설정 완료. 복원: sudo $0 reset"
        ;;

    show)
        echo "=== 현재 tc 설정 ($IFACE) ==="
        tc qdisc show dev "$IFACE"
        ;;

    reset)
        echo "=== tc 설정 초기화 ($IFACE) ==="
        tc qdisc del dev "$IFACE" root 2>/dev/null && echo "초기화 완료." || echo "설정된 qdisc 없음."
        ;;

    *)
        echo "사용법: sudo $0 [loss|delay|reorder|corrupt|show|reset]"
        echo ""
        echo "  loss    - 10% 패킷 유실"
        echo "  delay   - 100ms 지연"
        echo "  reorder - 50% 순서 역전 (10ms 지연 포함)"
        echo "  corrupt - 1% 패킷 손상"
        echo "  show    - 현재 설정 확인"
        echo "  reset   - 초기화"
        echo ""
        echo "인터페이스 변경: SIMULATE_IFACE=eth0 sudo $0 loss"
        exit 1
        ;;
esac
