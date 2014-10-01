/*
 * Copyright (C) 2014 Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * This file subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

#include <stdio.h>

#include "hwtimer.h"
#include "thread.h"

/* Make sure initial time is set */
#ifndef RIOTWATCH_H_INIT
#define RIOTWATCH_H_INIT (0)
#endif
#ifndef RIOTWATCH_M_INIT
#define RIOTWATCH_M_INIT (0)
#endif
#ifndef RIOTWATCH_S_INIT
#define RIOTWATCH_S_INIT (0)
#endif

int main(void)
{
    printf("time4riot\n");

    int h = RIOTWATCH_H_INIT;
    int m = RIOTWATCH_M_INIT;
    int s = RIOTWATCH_S_INIT;
    for (s = 0; 1; s++) {
        /* efficiently calculate time (no division/modulo/..) */
        if (s == 60) {
            s = 0;
            m++;
            if (m == 60) {
                m = 0;
                h++;
                if (h == 24) {
                    h = 0;
                }
            }
        }
        hwtimer_wait(HWTIMER_TICKS(1000UL * 1000UL));
        printf("\n%.2d:%.2d:%.2d", h, m, s);
    }
    return 0;
}
