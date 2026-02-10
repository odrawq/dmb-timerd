#pragma once

typedef struct
{
    int percent;
    int days_left;
    int days_passed;
}
dmb_timer_t;

int get_dmb_timer(dmb_timer_t *dest);
