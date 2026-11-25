#include "logger.h"
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

    fclose(file);
}