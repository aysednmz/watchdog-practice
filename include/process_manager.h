#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

#define MAX_WORKERS  8
#define MAX_RESTARTS 3

typedef enum {
    WORKER_RUNNING,
    WORKER_EXITED,
    WORKER_FAILED,
    WORKER_STOPPED
} WorkerState;

typedef struct {
    const char *path;
    char       *argv[4];
    pid_t       pid;
    WorkerState state;
    int         restart_count;
} Worker;

/* main.c dosyasından çağrılan process_management kısmı ile ilgili olan fonksiyonlar*/
int  start_workers(void);
void reap_children(void);
void check_workers(void);
void signal_workers(int signo);
int  active_worker_count(void);

/* log kısmında worker durumlarının okunabilmesi için ekledim */
int           get_worker_count(void);
const Worker *get_workers(void);

#endif