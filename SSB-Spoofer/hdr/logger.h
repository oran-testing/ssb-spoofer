#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

void logger_init(int use_colors);
void logger_set_level(log_level_t level);
void logger_log(log_level_t level, const char *fmt, ...);
void logger_shutdown(void);

#define LOG_DEBUG(...) logger_log(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  logger_log(LOG_INFO,  __VA_ARGS__)
#define LOG_WARN(...)  logger_log(LOG_WARN,  __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOG_ERROR, __VA_ARGS__)

#endif // LOGGER_H
