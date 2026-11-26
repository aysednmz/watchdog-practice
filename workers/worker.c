#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

static int parse_number(const char *text, int max, int *result)
{
    char *end;
    errno = 0;

    long value = strtol(text, &end, 10);

    if (errno != 0 || end == text ||
        *end != '\0' || value < 0 || value > max)
    {
        return -1;
    }

    *result = (int)value;
    return 0;
}

int main(int argc, char *argv[])
{
    int duration;
    int exit_code;

    if (argc != 3)
    {
        fprintf(stderr,
                "Usage: %s <duration> <exit_code>\n",
                argv[0]);

        return EXIT_FAILURE;
    }

    if (parse_number(argv[1], INT_MAX, &duration) == -1 ||
        parse_number(argv[2], 255, &exit_code) == -1)
    {
        fprintf(stderr, "Invalid worker arguments.\n");
        return EXIT_FAILURE;
    }

    printf("Worker started. PID: %ld\n", (long)getpid());
    fflush(stdout);

    if (duration == 0)
    {
        while (1)
        {
            sleep(1);
        }
    }

    unsigned int remaining = (unsigned int)duration;

    while (remaining > 0)
    {
        remaining = sleep(remaining);
    }

    printf("Worker %ld finished with exit code %d.\n",
           (long)getpid(), exit_code);

    return exit_code;
}