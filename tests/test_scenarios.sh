#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/.."

LOG_FILE="process_events.log"
WATCHDOG_PID=""
PASS_COUNT=0
FAIL_COUNT=0

pass()
{
    echo "[PASS] $1"
    PASS_COUNT=$((PASS_COUNT + 1))
}

fail()
{
    echo "[FAIL] $1"
    FAIL_COUNT=$((FAIL_COUNT + 1))
}

cleanup()
{
    if [[ -n "$WATCHDOG_PID" ]] &&
       kill -0 "$WATCHDOG_PID" 2>/dev/null; then
        kill -TERM "$WATCHDOG_PID" 2>/dev/null || true
        wait "$WATCHDOG_PID" 2>/dev/null || true
    fi
}

trap cleanup EXIT

wait_for_log()
{
    local pattern="$1"

    for ((i = 0; i < 60; i++)); do
        if grep -Eq "$pattern" "$LOG_FILE" 2>/dev/null; then
            return 0
        fi

        sleep 0.2
    done

    return 1
}

get_worker_pid()
{
    grep "Child process was started: Worker 0 " "$LOG_FILE" |
        tail -n 1 |
        sed -E 's/.*PID ([0-9]+).*/\1/'
}

echo "===== Watchdog Test Scenarios ====="

make

# Preserve previous logs.
if [[ -f "$LOG_FILE" ]]; then
    cp "$LOG_FILE" "$LOG_FILE.bak"
fi

: > "$LOG_FILE"

./watchdog &
WATCHDOG_PID=$!

echo "Watchdog PID: $WATCHDOG_PID"

# Test 1: Normal operation
if wait_for_log "Child process was started: Worker 2" &&
   [[ $(grep -c "Child process was started:" "$LOG_FILE") -ge 2 ]]; then
    pass "Normal operation"
else
    fail "Normal operation"
fi

# Test 2: Normal termination
if wait_for_log "Normal termination: Worker 2" &&
   ! grep -q "Restarting: Worker 2" "$LOG_FILE"; then
    pass "Normal termination"
else
    fail "Normal termination"
fi

# Test 3: Status reporting
kill -USR1 "$WATCHDOG_PID"

if wait_for_log "End of Status Report"; then
    pass "Status reporting"
else
    fail "Status reporting"
fi

# Tests 4 and 5: Unexpected termination and restart limit
RESTART_OK=1

for attempt in 1 2 3 4; do
    WORKER_PID=$(get_worker_pid || true)

    if [[ ! "$WORKER_PID" =~ ^[0-9]+$ ]]; then
        RESTART_OK=0
        break
    fi

    # Verify that the PID belongs to this watchdog.
    PARENT_PID=$(ps -o ppid= -p "$WORKER_PID" |
                 tr -d ' ' || true)

    if [[ "$PARENT_PID" != "$WATCHDOG_PID" ]]; then
        RESTART_OK=0
        break
    fi

    kill -KILL "$WORKER_PID"

    if [[ "$attempt" -le 3 ]]; then
        if ! wait_for_log "Restarting: Worker 0 \(attempt $attempt/3\)"; then
            RESTART_OK=0
            break
        fi

        EXPECTED=$((attempt + 1))
        READY=0

        for ((i = 0; i < 60; i++)); do
            STARTS=$(grep -c \
                "Child process was started: Worker 0 " \
                "$LOG_FILE" || true)

            if [[ "$STARTS" -ge "$EXPECTED" ]]; then
                READY=1
                break
            fi

            sleep 0.2
        done

        if [[ "$READY" -ne 1 ]]; then
            RESTART_OK=0
            break
        fi
    fi
done

if grep -q "Terminated by signal: Worker 0" "$LOG_FILE" &&
   grep -q "Restarting: Worker 0 (attempt 1/3)" "$LOG_FILE"; then
    pass "Unexpected termination and restart"
else
    fail "Unexpected termination and restart"
fi

if [[ "$RESTART_OK" -eq 1 ]] &&
   wait_for_log "Restart limit exceed: Worker 0" &&
   [[ $(grep -c "Restarting: Worker 0" "$LOG_FILE") -eq 3 ]] &&
   [[ $(grep -c "Child process was started: Worker 0 " "$LOG_FILE") -eq 4 ]]; then
    pass "Restart limit"
else
    fail "Restart limit"
fi

# Test 6: Graceful shutdown
CHILD_PIDS=$(ps -o pid= --ppid "$WATCHDOG_PID" || true)

kill -TERM "$WATCHDOG_PID"

if wait "$WATCHDOG_PID"; then
    SHUTDOWN_OK=1

    for pid in $CHILD_PIDS; do
        if kill -0 "$pid" 2>/dev/null; then
            SHUTDOWN_OK=0
        fi
    done

    if [[ "$SHUTDOWN_OK" -eq 1 ]] &&
       grep -q "Terminated during shutdown:" "$LOG_FILE"; then
        pass "Graceful shutdown"
    else
        fail "Graceful shutdown"
    fi
else
    fail "Graceful shutdown"
fi

WATCHDOG_PID=""

# Test 7: Log verification
if [[ -s "$LOG_FILE" ]] &&
   ! grep -Evq '^\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\] .+' "$LOG_FILE"; then
    pass "Timestamped log verification"
else
    fail "Timestamped log verification"
fi

echo "==================================="
echo "Passed: $PASS_COUNT"
echo "Failed: $FAIL_COUNT"
echo "==================================="

if [[ "$FAIL_COUNT" -ne 0 ]]; then
    exit 1
fi