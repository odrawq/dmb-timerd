#define _GNU_SOURCE

#include <time.h>

#include "config.h" // This file is created after configure.sh has been successfully executed.
#include "dmb-timer.h"

int get_dmb_timer(dmb_timer_t *dest)
{
    struct tm tm_start = {0};
    strptime(START_DATE, "%Y-%m-%d", &tm_start);

    struct tm tm_end = tm_start;
    tm_end.tm_year += 1;

    const time_t end = mktime(&tm_end);
    const time_t now = time(0);

    const time_t left = end - now;

    if (left < 0)
        return 1;

    const time_t start = mktime(&tm_start);

    dest->days = left / 86400;
    dest->percent = (int) ((double) (now - start) / (end - start) * 100);

    return 0;
}
