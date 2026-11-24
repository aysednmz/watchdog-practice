
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "signal_manager.h"
#include "process_manager.h"
#include "logger.h"

#define SHUTDOWN_TIMEOUT 5

/* Sinyal bayrağını güvenli bir şekilde okuyup sıfırladım. */
static int read_signal_flag(volatile sig_atomic_t *flag, int signo)
{
    sigset_t mask;
    sigset_t old_mask;

    if (sigemptyset(&mask) == -1)
        return -1;

    if (sigaddset(&mask, signo) == -1)
        return -1;

    if (sigprocmask(SIG_BLOCK, &mask, &old_mask) == -1)
        return -1;

    int value = *flag;
    *flag = 0;

    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1)
        return -1;

    return value;
}

/* Alt süreçlerin kapanmasını belirli bir süre boyunca bekledim. */
static int wait_for_workers(int timeout)
{
    struct timespec start;
    struct timespec current;
    struct timespec delay = {0, 100000000L};

    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
        return -1;

    while (1)
    {
        reap_children();

        if (active_worker_count() == 0)
            return 0;

        if (clock_gettime(CLOCK_MONOTONIC, &current) == -1)
            return -1;

        double elapsed =
            (double)(current.tv_sec - start.tv_sec) +
            (double)(current.tv_nsec - start.tv_nsec) / 1000000000.0;

        if (elapsed >= timeout)
            return 1;

        if (nanosleep(&delay, NULL) == -1 && errno != EINTR)
            return -1;
    }
}

/* Alt süreçleri güvenli bir şekilde kapatmak için fonksiyon oluşturdum. */
static int graceful_shutdown(void)
{
    shutdown_requested = 1;
    alarm(0);

    printf("Güvenli kapatma işlemi başlatıldı.\n");
    log_event("Güvenli kapatma işlemi başlatıldı.");

    /* Çalışan alt süreçlere kapatma sinyali gönderdim. */
    signal_workers(SIGTERM);

    int result = wait_for_workers(SHUTDOWN_TIMEOUT);

    /* Belirlenen sürede kapanmayan alt süreçleri sonlandırdım. */
    if (result != 0)
    {
        log_event("Alt süreçlere SIGKILL gönderiliyor.");

        signal_workers(SIGKILL);

        result = wait_for_workers(SHUTDOWN_TIMEOUT);
    }

    /* Bütün alt süreçlerin kapanıp kapanmadığını kontrol ettim. */
    if (result != 0)
    {
        fprintf(stderr, "Güvenli kapatma tamamlanamadı.\n");
        log_event("Güvenli kapatma tamamlanamadı.");

        return -1;
    }

    printf("Güvenli kapatma işlemi tamamlandı.\n");
    log_event("Güvenli kapatma işlemi tamamlandı.");

    return 0;
}

int main(void)
{
    int exit_status = EXIT_SUCCESS;

    /* Sinyal yöneticilerini başlattım ve hata kontrolünü yaptım. */
    if (setup_signal_handlers() == -1)
    {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    /* Ana sürecin PID bilgisini ekrana yazdırdım. */
    printf("Watchdog PID: %ld\n", (long)getpid());

    log_event("Watchdog programı başlatıldı.");

    /* Alt süreçleri başlattım ve hata kontrolünü yaptım. */
    if (!shutdown_requested && start_workers() == -1)
    {
        fprintf(stderr, "Alt süreçler başlatılamadı.\n");
        log_event("Alt süreçler başlatılamadı.");

        graceful_shutdown();
        return EXIT_FAILURE;
    }

    /* Kapatma sinyali gelene kadar programın çalışmasını sağladım. */
    while (!shutdown_requested)
    {
        int child_flag = read_signal_flag(&child_terminated, SIGCHLD);

        if (child_flag == -1)
        {
            perror("sigprocmask");
            exit_status = EXIT_FAILURE;
            break;
        }

        /* Sonlanan alt süreçlerin kontrol edilmesini sağladım. */
        if (child_flag)
            reap_children();

        if (shutdown_requested)
            break;

        int status_flag = read_signal_flag(&status_requested, SIGUSR1);

        if (status_flag == -1)
        {
            perror("sigprocmask");
            exit_status = EXIT_FAILURE;
            break;
        }

        /* Durum raporu isteğini kontrol ettim. */
        if (status_flag && !shutdown_requested)
            report_status();

        if (shutdown_requested)
            break;

        int periodic_flag =
            read_signal_flag(&periodic_check_requested, SIGALRM);

        if (periodic_flag == -1)
        {
            perror("sigprocmask");
            exit_status = EXIT_FAILURE;
            break;
        }

        /* Alt süreçlerin periyodik olarak kontrol edilmesini sağladım. */
        if (periodic_flag && !shutdown_requested)
            check_workers();

        if (!shutdown_requested)
            sleep(1);
    }

    /* Kapatma isteği geldiğinde güvenli kapatma işlemini başlattım. */
    if (shutdown_requested)
        printf("Kapatma sinyali alındı.\n");

    if (graceful_shutdown() == -1)
        exit_status = EXIT_FAILURE;

    return exit_status;
}
