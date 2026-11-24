#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

#include <signal.h>

/* Gelen sinyalleri takip etmek için bayrak değişkenlerini ekledim. */
extern volatile sig_atomic_t shutdown_requested;
extern volatile sig_atomic_t status_requested;
extern volatile sig_atomic_t child_terminated;
extern volatile sig_atomic_t periodic_check_requested;

/* Sinyal yöneticilerini kuracak fonksiyonu ekledim. */
int setup_signal_handlers(void);

#endif