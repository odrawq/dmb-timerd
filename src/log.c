#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "log.h"

static FILE *audit_log_cache = 0;
static pthread_mutex_t audit_log_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

static void make_timestamp(char *dest, const int size);

void init_log(void)
{
    if (!(audit_log_cache = fopen(AUDIT_LOG_FILE, "a")))
        die("%s: %s: failed to open %s", __FILE__, __func__, AUDIT_LOG_FILE);

    setvbuf(audit_log_cache, 0, _IOLBF, BUFSIZ);
}

void audit(const char *fmt, ...)
{
    char timestamp[sizeof (TIMESTAMP_FMT) + 3];
    make_timestamp(timestamp, sizeof (timestamp));

    va_list args;
    va_start(args, fmt);

    pthread_mutex_lock(&audit_log_cache_mutex);

    fprintf(audit_log_cache, "%s ", timestamp);
    vfprintf(audit_log_cache, fmt, args);
    fputc('\n', audit_log_cache);

    pthread_mutex_unlock(&audit_log_cache_mutex);

    va_end(args);
}

_Noreturn void die(const char *fmt, ...)
{
    pthread_mutex_lock(&audit_log_cache_mutex);

    FILE *error_log = fopen(ERROR_LOG_FILE, "a");

    if (error_log)
    {
        char timestamp[sizeof (TIMESTAMP_FMT) + 3];
        make_timestamp(timestamp, sizeof (timestamp));

        va_list args;
        va_start(args, fmt);

        fprintf(error_log, "%s ", timestamp);
        vfprintf(error_log, fmt, args);
        fputc('\n', error_log);

        va_end(args);
        fclose(error_log);
    }

    exit(1);
}

static void make_timestamp(char *dest, const int size)
{
    struct tm tm;
    const time_t now = time(0);
    localtime_r(&now, &tm);
    strftime(dest, size, TIMESTAMP_FMT, &tm);
}
