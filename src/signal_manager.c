
#define _POSIX_C_SOURCE 200809L

#include "signal_manager.h"
#include <stddef.h>

/* Bayrak değişkenlerinin başlangıç değerlerini sıfır olarak belirledim. */
volatile sig_atomic_t shutdown_requested = 0;
volatile sig_atomic_t status_requested = 0;
volatile sig_atomic_t child_terminated = 0;
volatile sig_atomic_t periodic_check_requested = 0;

/* Gelen sinyale göre ilgili bayrağın değerini değiştirdim. */
static void signal_handler(int signo)
{
    switch (signo)
    {
        case SIGINT:
        case SIGTERM:
            shutdown_requested = 1;
            break;

        case SIGUSR1:
            status_requested = 1;
            break;

        case SIGCHLD:
            child_terminated = 1;
            break;

        case SIGALRM:
            periodic_check_requested = 1;
            break;
    }
}

/* sigaction kullanarak sinyal yöneticilerini kurdum. */
int setup_signal_handlers(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;

    if (sigemptyset(&sa.sa_mask) == -1)
        return -1;

    if (sigaction(SIGINT, &sa, NULL) == -1)
        return -1;

    if (sigaction(SIGTERM, &sa, NULL) == -1)
        return -1;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        return -1;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
        return -1;

    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
        return -1;

    return 0;
}

