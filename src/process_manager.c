#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "process_manager.h"
#include "signal_manager.h"
#include "logger.h"

#define CHECK_INTERVAL 5

static Worker workers[MAX_WORKERS];
static int num_workers = 0;

static void init_worker_slot(int idx, const char *path, char *arg1, char *arg2)
{
    workers[idx].path = path;
    workers[idx].argv[0] = (char *)path;
    workers[idx].argv[1] = arg1;
    workers[idx].argv[2] = arg2;
    workers[idx].argv[3] = NULL;
    workers[idx].pid = -1;
    workers[idx].state = WORKER_STOPPED;
    workers[idx].restart_count = 0;
}

static int spawn_worker(int idx)
{
    char msg[128];
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    if (pid == 0)
    {
        execvp(workers[idx].path, workers[idx].argv);
        perror("execvp");
        _exit(127);
    }

    workers[idx].pid = pid;
    workers[idx].state = WORKER_RUNNING;

    snprintf(msg, sizeof(msg), "Child process was started: Worker %d (PID %d)", idx, (int)pid);
    log_event(msg);
    return 0;
}

int start_workers(void)
{
    int i;

    memset(workers, 0, sizeof(workers));

    /* test kısmı için 3 worker:
     * 0 ve 1: süresiz çalışır (kill, restart, SIGUSR1, shutdown testleri için)
     * 2: 4 saniye sonra 0 çıkış kodu ile biter */
    init_worker_slot(0, "./worker", "0", "0");
    init_worker_slot(1, "./worker", "0", "0");
    init_worker_slot(2, "./worker", "4", "0");
    num_workers = 3;

    for (i = 0; i < num_workers; i++)
    {
        if (spawn_worker(i) == -1)
        {
            workers[i].state = WORKER_FAILED;
            return -1;
        }
    }

    alarm(CHECK_INTERVAL);
    return 0;
}