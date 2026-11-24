
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "signal_manager.h"

int main(void)
{
    /* Sinyal yöneticilerini başlattım ve hata kontrolünü yaptım. */
    if (setup_signal_handlers() == -1)
    {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    /* Ana sürecin PID bilgisini ekrana yazdırdım. */
    printf("Watchdog PID: %ld\n", (long)getpid());
/* Kapatma sinyali gelene kadar programın çalışmasını sağladım. */
while (!shutdown_requested)
{
    if (status_requested)
    {
        status_requested = 0;
        printf("Durum raporu istendi.\n");
    }

    sleep(1);
}

/* Kapatma sinyali geldiğinde döngüyü sonlandırdım. */
printf("Kapatma sinyali alındı.\n");

    return EXIT_SUCCESS;
}

