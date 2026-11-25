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

static int find_worker_by_pid(pid_t pid)
{
    int i;

    for (i = 0; i < num_workers; i++)
    {
        if (workers[i].pid == pid)
            return i;
    }
    return -1;
}

void reap_children(void)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        int idx = find_worker_by_pid(pid);
        char msg[160];

        if (idx < 0)
            continue;

        workers[idx].pid = -1;

        /* kapanırken ölenleri yeniden baslatma */
        if (shutdown_requested)
        {
            workers[idx].state = WORKER_STOPPED;
            snprintf(msg, sizeof(msg),
                     "Terminated during shutdown: Worker %d (PID %d)",
                     idx, (int)pid);
            log_event(msg);
            continue;
        }

        /* exit code == 0 : yeniden başlatma yok */
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            workers[idx].state = WORKER_EXITED;
            snprintf(msg, sizeof(msg),
                     "Normal termination: Worker %d (PID %d, code 0)",
                     idx, (int)pid);
            log_event(msg);
            continue;
        }

        /* beklenmeyen çıkış olması durumunda hata kodu veya sinyali yazdırırız */
        if (WIFEXITED(status))
        {
            snprintf(msg, sizeof(msg),
                     "Termination with error: Worker %d (PID %d, code %d)",
                     idx, (int)pid, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            snprintf(msg, sizeof(msg),
                     "Terminated by signal: Worker %d (PID %d, signal %d)",
                     idx, (int)pid, WTERMSIG(status));
        }
        else
        {
            snprintf(msg, sizeof(msg),
                     "Unexpected termination: Worker %d (PID %d)",
                     idx, (int)pid);
        }
        log_event(msg);

        /* en fazla 3 kez yeniden başlatmak için eklediğim kontrol */
        if (workers[idx].restart_count < MAX_RESTARTS)
        {
            workers[idx].restart_count++;
            snprintf(msg, sizeof(msg),
                     "Restarting: Worker %d (attempt %d/%d)",
                     idx, workers[idx].restart_count, MAX_RESTARTS);
            log_event(msg);

            workers[idx].state = WORKER_STOPPED;
            if (spawn_worker(idx) == -1)
                workers[idx].state = WORKER_FAILED;
        }
        else
        {
            workers[idx].state = WORKER_FAILED;
            snprintf(msg, sizeof(msg),
                     "Restart limit exceed: Worker %d (limit %d), won't restart",
                     idx, MAX_RESTARTS);
            log_event(msg);
        }
    }
}

void check_workers(void)
{
    char msg[128];
    int running = 0;
    int i;

    reap_children();

    for (i = 0; i < num_workers; i++)
    {
        if (workers[i].state == WORKER_RUNNING && workers[i].pid > 0)
        {
            if (kill(workers[i].pid, 0) == 0)
            {
                running++;
            }
            else if (errno == ESRCH)
            {
                snprintf(msg, sizeof(msg),
                         "Periodic check: Worker %d (PID %d) not found",
                         i, (int)workers[i].pid);
                log_event(msg);
            }
        }
    }

    snprintf(msg, sizeof(msg),
             "Periodic check: %d/%d child process is running",
             running, num_workers);
    log_event(msg);

    if (!shutdown_requested)
        alarm(CHECK_INTERVAL);
}

void signal_workers(int signo)
{
    char msg[128];
    int i;

    for (i = 0; i < num_workers; i++)
    {
        if (workers[i].state == WORKER_RUNNING && workers[i].pid > 0)
        {
            if (kill(workers[i].pid, signo) == -1 && errno != ESRCH)
            {
                perror("kill");
            }
            else
            {
                snprintf(msg, sizeof(msg),
                         "Signal (%d) was sent: Worker %d (PID %d)",
                         signo, i, (int)workers[i].pid);
                log_event(msg);
            }
        }
    }
}