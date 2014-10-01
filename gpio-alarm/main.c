/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief       Test application for the PIR motion sensor driver
 *
 * @author      Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * @}
 */

#ifndef PIR_GPIO
#error "PIR_GPIO not defined"
#endif

#include <stdio.h>

#include "thread.h"
#include "vtimer.h"
#include "pir.h"
#include "periph/gpio.h"

char pir_handler_stack[KERNEL_CONF_STACKSIZE_MAIN];
pir_t pir;
gpio_t led_red, led_green;

void* pir_handler(void *arg)
{
    (void) arg; /* unused */

    msg_t msg_q[1];
    msg_init_queue(msg_q, 1);

#ifndef TEST_PIR_POLLING
    printf("Registering PIR handler thread...     %s\n",
           pir_register_thread(&pir) == 0 ? "[OK]" : "[Failed]");
#endif

    msg_t m;
    while (msg_receive(&m)) {
        printf("PIR handler got a message: ");
        switch (m.type) {
            case PIR_STATUS_HI:
                puts("something started moving.");
                gpio_set(led_red);
                gpio_clear(led_green);
                break;
            case PIR_STATUS_LO:
                puts("the movement has ceased.");
                gpio_set(led_green);
                gpio_clear(led_red);
                break;
            default:
                puts("stray message.");
                break;
        }
    }
    puts("PIR handler: this should not have happened!");

    return NULL;
}

int main(void)
{
    puts("PIR motion sensor test application\n");

    printf("Initializing red LED at GPIO_%i...        ", GPIO_LED_RED);
    led_red = GPIO_LED_RED;
    if (gpio_init_out(led_red, GPIO_NOPULL) == 0) {
        puts("[OK]\n");
    }
    else {
        puts("[Failed]");
        return 1;
    }
    gpio_clear(led_red);

    printf("Initializing green LED at GPIO_%i...        ", GPIO_LED_GREEN);
    led_green = GPIO_LED_GREEN;
    if (gpio_init_out(led_green, GPIO_NOPULL) == 0) {
        puts("[OK]\n");
    }
    else {
        puts("[Failed]");
        return 1;
    }
    gpio_clear(led_green);

    printf("Initializing PIR sensor at GPIO_%i... ", PIR_GPIO);
    if (pir_init(&pir, PIR_GPIO) == 0) {
        puts("[OK]\n");
    }
    else {
        puts("[Failed]");
        return 1;
    }

   kernel_pid_t pir_handler_pid = thread_create(
           pir_handler_stack, sizeof(pir_handler_stack), PRIORITY_MAIN - 1,
           CREATE_WOUT_YIELD | CREATE_STACKTEST,
           pir_handler, NULL, "pir_handler");

#if TEST_PIR_POLLING
    puts("Checking sensor state every half second.");
    pir_event_t status_old;
    while (1) {
        pir_event_t status = pir_get_status(&pir);
        if (status != status_old) {
            msg_t m = { .type = status, .content.ptr = (void*)&pir, }; 
            msg_send(&m, pir_handler_pid, 0);
        }
        status_old = status;
        vtimer_usleep(1000 * 500);
    }
#endif
    return 0;
}
