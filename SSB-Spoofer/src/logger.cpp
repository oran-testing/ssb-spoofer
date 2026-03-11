#include "logger.h"

#define COLOR_RESET  "\x1b[0m"
#define COLOR_RED    "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_BLUE   "\x1b[34m"

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static log_level_t current_level = LOG_DEBUG;
static int enable_colors = 1;

static const char *level_strings[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR"
};

static const char *level_colors[] = {
    COLOR_BLUE,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_RED
};

void logger_init(int use_colors)
{
    enable_colors = use_colors;
}

void logger_set_level(log_level_t level)
{
    current_level = level;
}

void logger_shutdown(void)
{
    pthread_mutex_destroy(&log_mutex);
}

void logger_log(log_level_t level, const char *fmt, ...)
{
    if (level < current_level) {
        return;
    }

    pthread_mutex_lock(&log_mutex);

    /* Timestamp */
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_now);

    /* Print prefix */
    if (enable_colors) {
        fprintf(stderr, "%s", level_colors[level]);
    }

    fprintf(stderr, "[%s] %-5s: ",
            time_buf,
            level_strings[level]);

    if (enable_colors) {
        fprintf(stderr, "%s", COLOR_RESET);
    }

    /* Print message */
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);

    pthread_mutex_unlock(&log_mutex);
}

