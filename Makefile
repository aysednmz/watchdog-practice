CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS = -Iinclude

WATCHDOG = watchdog
WORKER = worker

SOURCES = src/main.c \
          src/signal_manager.c \
          src/process_manager.c \
          src/logger.c

HEADERS = include/signal_manager.h \
          include/process_manager.h \
          include/logger.h

.PHONY: all clean test

all: $(WATCHDOG) $(WORKER)

$(WATCHDOG): $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $(WATCHDOG)

$(WORKER): workers/worker.c
	$(CC) $(CFLAGS) workers/worker.c -o $(WORKER)

test: all
	bash tests/test_scenarios.sh

clean:
	rm -f $(WATCHDOG) $(WORKER)