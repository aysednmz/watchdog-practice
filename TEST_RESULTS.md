# Test Results

## Test Environment

- Operating System: Ubuntu 20.04 (WSL)
- Programming Language: C
- Compiler: GCC 9.4
- Build Tool: GNU Make
- Test Date: November 26, 2026

## Test Scenarios

### 1. Normal Operation

We started the watchdog and checked whether the
worker processes were created successfully.

Expected: Worker processes should start and run.

Actual: Three worker processes were started.

Result: PASS

### 2. Normal Termination

We checked what happens when a worker finishes
with exit code 0.

Expected: The worker should not be restarted.

Actual: Worker 2 finished and was not restarted.

Result: PASS

### 3. Status Reporting

We sent SIGUSR1 to the watchdog.

Expected: A worker status report should be saved
in the log file.

Actual: The status report was generated successfully.

Result: PASS

### 4. Unexpected Termination

We terminated a worker using SIGKILL.

Expected: The watchdog should restart the worker.

Actual: The worker was restarted with a new PID.

Result: PASS

### 5. Restart Limit

We repeatedly terminated the same worker.

Expected: The watchdog should restart the worker
a maximum of three times.

Actual: Three restart attempts were made.
The worker was not restarted after reaching the limit.

Result: PASS

### 6. Graceful Shutdown with SIGTERM

We sent SIGTERM to the watchdog.

Expected: The watchdog should terminate its active
workers and shut down safely.

Actual: The shutdown completed successfully,
and no monitored child processes remained.

Result: PASS

### 7. Log Verification

We checked the generated log file.

Expected: Event messages should contain timestamps.

Actual: The log entries contained valid date
and time information.

Result: PASS

### 8. Manual Shutdown with Ctrl+C

We started the watchdog and pressed Ctrl+C.

Expected: The watchdog should perform a safe shutdown.

Actual: The shutdown messages were displayed,
and worker termination events were recorded.

Result: PASS 

## Automated Test Summary

| Test --> Result 

- Normal operation --> PASS 
- Normal termination --> PASS 
- Status reporting --> PASS 
- Unexpected termination and restart --> PASS 
- Restart limit --> PASS 
- Graceful shutdown --> PASS 
- Timestamped log verification --> PASS 

Total automated tests: 7
Passed: 7
Failed: 0

All automated tests passed successfully.
Also the Ctrl+C for shutdown was tested.

## Sample Logs

The sample_log.txt file contains the log records
generated during testing.