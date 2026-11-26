# POSIX Signal-Based Process Monitoring and Management System

## About the Project

We developed this project for our System Programming course.

The main purpose of the project is to monitor and manage
child processes using POSIX signals in C language. Our watchdog
program starts worker processes, checks their status,
restarts them when they fail, and safely shuts them down
when requested.

## Features

- Creating and monitoring child processes
- Restarting workers after unexpected termination
- Limiting restart attempts to three per worker
- Checking worker status every five seconds
- Generating status reports with SIGUSR1
- Handling SIGINT and SIGTERM for safe shutdown
- Saving process events with timestamps

## Requirements

- Linux or WSL
- GCC
- Make
- Bash

## How to Run

First, clone the repository:
**git clone https://github.com/aysednmz/watchdog-practice.git**
**cd watchdog-practice**


Build the project:
**make**


Start the program:
**./watchdog**

The watchdog starts three workers. Two of them continue
running, while the third one exits normally after four seconds.

## Signals

We use the following signals to manage the processes:

| Signal | Purpose |
|--------|---------|
| SIGCHLD | Detects child process termination |
| SIGALRM | Checks worker status periodically |
| SIGUSR1 | Generates a status report |
| SIGINT | Stops the program safely |
| SIGTERM | Stops the program safely |

To get a status report, open another terminal and run:
**kill -SIGUSR1 <WATCHDOG_PID>**


To stop the program:
**kill -SIGTERM <WATCHDOG_PID>**
You can also press **Ctrl+C** to stop the program.

Replace <WATCHDOG_PID> with the PID shown when the
watchdog starts.

## Logging

We use a log file called `process_events.log` to keep
track of what happens while the program is running.

The log file contains worker information, termination
events, restart attempts, status reports and shutdown
messages. Each entry includes the date and time.

You can view the log file with:
**cat process_events.log**

## Testing

We tested our project in Ubuntu using WSL.

To run the automated tests:
**make test** 


Our test results:

- Test --> Result 

- Normal operation --> PASS 
- Normal termination --> PASS 
- Status reporting --> PASS 
- Unexpected termination and restart --> PASS 
- Restart limit --> PASS 
- Graceful shutdown --> PASS 
- Timestamped log verification --> PASS 

All seven automated tests passed. We also manually
tested the shutdown process using Ctrl+C.

## Project Structure


watchdog-practice/
├── include/
│   ├── logger.h
│   ├── process_manager.h
│   └── signal_manager.h
├── src/
│   ├── logger.c
│   ├── main.c
│   ├── process_manager.c
│   └── signal_manager.c
├── workers/
│   └── worker.c
├── tests/
│   └── test_scenarios.sh
├── .gitignore
├── Makefile
└── README.md


## Team Responsibilities

- Student 1: Signal handling, main program and graceful shutdown.
- Student 2: Process management, restart mechanism and periodic checks.
- Student 3: Logging, status reporting, worker implementation,
  Makefile, testing and documentation.

## Clean Up

To remove the compiled executable files:
**make clean**
