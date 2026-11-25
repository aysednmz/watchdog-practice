#ifndef LOGGER_H
#define LOGGER_H

/* Writes an event message to the log file. */
void log_event(const char *msg);

/* Reports the current status of all workers. */
void report_status(void);

#endif