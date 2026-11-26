#include "logger.h"
#include "process_manager.h"
#include <stdio.h>
#include <time.h>

void log_event(const char *msg)
{
    FILE *file = fopen("process_events.log", "a");

    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);
    char timestamp[32];

    if (time_info == NULL ||
        strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", time_info) == 0)
    {
        fprintf(file, "[Unknown time] %s\n", msg);
    }
    else
    {
        fprintf(file, "[%s] %s\n", timestamp, msg);
    }

    if (ferror(file))
        fprintf(stderr, "Log write failed.\n");

    if (fclose(file) == EOF)
        perror("fclose");
}

void report_status(void)
{
    int count = get_worker_count();
    const Worker *workers = get_workers();
    char message[256];

    log_event("----- Worker Status Report -----");

    snprintf(message, sizeof(message),
             "Total workers: %d", count);

    log_event(message);

    for (int i = 0; i < count; i++)
    {
        snprintf(message, sizeof(message),
                 "Worker %d | PID: %ld | State: %s | Restarts: %d/%d",
                 i,
                 (long)workers[i].pid,
                 worker_state_to_str(workers[i].state),
                 workers[i].restart_count,
                 MAX_RESTARTS);

        log_event(message);
    }

    log_event("----- End of Status Report -----");
}