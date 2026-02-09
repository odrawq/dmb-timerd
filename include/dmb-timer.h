#pragma once

typedef struct
{
    int days;
    int percent;
}
dmb_timer_t;

int get_dmb_timer(dmb_timer_t *dest);
