/*
 * Copyright (C) 2013,2014 Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * This file subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "hwtimer.h"
#include "board.h"
#include "board_uart0.h"
#include "msg.h"
#include "thread.h"
#include "posix_io.h"
#include "shell.h"
#include "shell_commands.h"
#include "ps.h"
#include "transceiver.h"
#include "radio/types.h"
#include "net_help.h"

#include "debug.h"

#define SND_BUFFER_SIZE     (100)
#define RCV_BUFFER_SIZE     (64)
#define RADIO_STACK_SIZE    (KERNEL_CONF_STACKSIZE_MAIN)

#define TRANSCEIVER_TYPE TRANSCEIVER_DEFAULT

#define TGP_DATA 0x01
#define TGP_CTRL 0x02

typedef struct {
    uint8_t type;
    uint32_t seq;
    uint32_t ts;
}  __attribute__((packed)) tgp_header;

char radio_stack[RADIO_STACK_SIZE];
char server_stack[RADIO_STACK_SIZE];
msg_t msg_q[RCV_BUFFER_SIZE];

int monitor_pid = -1;
int server_pid = -1;

/* "API" */

void register_thread(int pid)
{
    printf("registering at transceiver..");
    if (!transceiver_register(TRANSCEIVER_TYPE, pid)) {
        printf(" failed\n");
    }
    else {
        printf(" done\n");
    }
}

void unregister_thread(int pid)
{
    printf("unregistering from transceiver..");
    if (!transceiver_unregister(TRANSCEIVER_TYPE, pid)) {
        printf(" failed\n");
    }
    else {
        printf(" done\n");
    }
}

/* needed nonsense */

int shell_readc(void)
{
    char c = 0;
    posix_read(uart0_handler_pid, &c, 1);
    return c;
}

void shell_putchar(int c)
{
    putchar(c);
}

/* performance measurements */
uint32_t expect_count = 0;
uint32_t expect_src = 0;
uint32_t expect_dst = 0;

void *expect_traffic(void *arg)
{
    (void) arg;

    msg_t m;
    radio_packet_t *p;

    msg_init_queue(msg_q, RCV_BUFFER_SIZE);

    thread_print_all();
    while(1) {
        uint32_t tot_pack = 0;
        uint32_t last_seq = 0;
        uint32_t reordered = 0;
        uint32_t tot_len = 0;

        printf("expecting %"PRIu32" packets\n", expect_count);
        while (1) {
            msg_receive(&m);
            if (m.type == PKT_PENDING) {
                p = (radio_packet_t*) m.content.ptr;

                /* quickly filter out stray packets */
                if (p->src != expect_src) {
                    puts("wrong src packet");
                    expect_count++;
                    p->processing--;
                    continue;
                }
                if (p->dst != expect_dst) {
                    puts("wrong dst packet");
                    expect_count++;
                    p->processing--;
                    continue;
                }
                if (p->length < sizeof(tgp_header)) {
                    printf("too small packet");
                    expect_count++;
                    p->processing--;
                    continue;
                }

                /* parse header */
                tgp_header *hdr = (tgp_header*)&p->data[0];
                hdr->seq = NTOHL(hdr->seq);
                hdr->ts = NTOHL(hdr->ts);

                /* handle packet */
                switch (hdr->type) {
                    case TGP_DATA:
                        DEBUG("got seq %"PRIu32" with timestamp %"PRIu32"\n", hdr->seq, hdr->ts);
                        break;
                    case TGP_CTRL:
                        printf("starting test with %"PRIu32" packets\n", hdr->seq);
                        expect_count = hdr->seq;
                        p->processing++;
                        continue;
                        break;
                }

                if (last_seq > hdr->seq) {
                    reordered++;
                }
                last_seq = hdr->seq;

                tot_pack++;
                tot_len += p->length;

                p->processing--;
                if (tot_pack >= expect_count) {
                    break;
                }
            }
            else if (m.type == ENOBUFFER) {
                //puts("Transceiver buffer full");
                expect_count++;
            }
            else {
                puts("Unknown packet received");
                expect_count++;
            }
        }

        unregister_thread(server_pid);

        thread_print_all();
        printf("total packets: %"PRIu32"\n", tot_pack);
        printf("total size: %"PRIu32"\n", tot_len);
        printf("last_seq: %"PRIu32"\n", last_seq);

        thread_sleep();
    }

    return NULL;
}

void traffic_gen(int count, int size, radio_address_t addr, unsigned long delay)
{
    msg_t mesg;
    transceiver_command_t tcmd;
    char text_msg[size];

    radio_packet_t p;
    int8_t response;

    tcmd.transceivers = TRANSCEIVER_TYPE;
    tcmd.data = &p;
    memset(text_msg, 0, size);

    thread_print_all();
    printf("Sending %i packets of length %"PRIu16" to %"PRIu16"..", count, size, addr);
    uint32_t seq = 0;
    tgp_header *hdr = (tgp_header*)&text_msg[0];
    for (int i = 0; i < count; i++) {
        hdr->seq = HTONL(seq);
        hdr->type = TGP_DATA;
        hdr->ts = HTONL((uint32_t) HWTIMER_TICKS_TO_US(hwtimer_now()));

        p.data = (uint8_t *) text_msg;
        p.length = size;
        p.dst = addr;
        mesg.type = SND_PKT;
        mesg.content.ptr = (char *)&tcmd;
        msg_send_receive(&mesg, &mesg, transceiver_pid);

        response = mesg.content.value;
        if (response < -1) {
            puts("traffic_gen: error sending packet");
        }

        seq++;
        DEBUG(".");
        if (delay != 0) {
            hwtimer_wait(HWTIMER_TICKS(delay));
        }
    }
    printf("done\n");
    thread_print_all();
}

/* debugging */
void *monitor(void *arg)
{
    (void) arg;
    msg_t m;
    radio_packet_t *p;
    radio_packet_length_t i;

    msg_init_queue(msg_q, RCV_BUFFER_SIZE);

    while (1) {
        msg_receive(&m);
        if (m.type == PKT_PENDING) {
            p = (radio_packet_t*) m.content.ptr;
            printf("Got radio packet:\n");
            printf("\tLength:\t%u\n", p->length);
            printf("\tSrc:\t%u\n", p->src);
            printf("\tDst:\t%u\n", p->dst);
            printf("\tLQI:\t%u\n", p->lqi);
            printf("\tRSSI:\t%u\n", p->rssi);

            for (i = 0; i < p->length; i++) {
                printf("%02X ", p->data[i]);
            }
            p->processing--;
            puts("\n");
        }
        else if (m.type == ENOBUFFER) {
            puts("Transceiver buffer full");
        }
        else {
            puts("Unknown packet received");
        }
    }

    return NULL;
}

/* shell commands */
void sc_start_stop_server_usage(char *cmd)
{
    printf("%s <count> <src> <dst>\n", cmd);
    printf("%s stop\n", cmd);
}

void sc_start_stop_server(int argc, char **argv)
{
    if (argc == 4) {
        printf("setting variables..");
        expect_count = atoi(argv[1]);
        expect_src = atoi(argv[2]);
        expect_dst = atoi(argv[3]);
        printf(" registering server..");
        register_thread(server_pid);
        thread_wakeup(server_pid);
        printf(" done\n");
    }
    else if ((argc == 2) && (strcmp(argv[1], "stop") == 0)) {
        unregister_thread(server_pid);
    }
    else {
        sc_start_stop_server_usage(argv[0]);
    }
}

void sc_start_stop_monitor(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: monitor <start|stop>\n");
        return;
    }

    if (strcmp(argv[1], "start") == 0) {
        register_thread(monitor_pid);
    }
    else if (strcmp(argv[1], "stop") == 0) {
        unregister_thread(monitor_pid);
    }
    else {
        printf("usage: monitor <start|stop>\n");
    }
}

void sc_traffic_gen_usage(void)
{
    printf("usage: tg <count> <size> <delay> <address>\n"
            "\tcount:\tint packets\n"
            "\tsize:\tint bytes\n"
            "\tdelay:\tint milliseconds\n"
            "\taddress: radio_address_t\n");
    return;
}

void sc_traffic_gen(int argc, char **argv)
{
    int count, size, delay;
    radio_address_t addr;

    if (argc != 5) {
        sc_traffic_gen_usage();
        return;
    }

    count = atoi(argv[1]);
    size = atoi(argv[2]);
    if (size < (int)sizeof(tgp_header)) {
        printf("size needs to be at least %i.\n", sizeof(tgp_header));
        return;
    }
    if (size > PAYLOAD_SIZE) {
        printf("size needs to be at most %i.\n", PAYLOAD_SIZE);
        return;
    }
    delay = atoi(argv[3])*1000;
    if (delay < 0) {
        printf("delay needs to be positive or zero.\n");
        return;
    }
    addr = atoi(argv[4]);

    /* run */
    traffic_gen(count, size, addr, delay);
}

const shell_command_t shell_commands[] = {
    { "mon", "start/stop the monitor thread", sc_start_stop_monitor },
    { "tg", "send <count> packets of <size> with <delay> to <addr>", sc_traffic_gen },
    { "server", "start performance server", sc_start_stop_server},
    { NULL, NULL, NULL }
};

int main(void)
{
    /* initialize posix */
    posix_open(uart0_handler_pid, 0);

    /* start threads */
    printf("starting monitor thread..");
    monitor_pid = thread_create(
            radio_stack,
            sizeof(radio_stack),
            PRIORITY_MAIN-2,
            CREATE_STACKTEST,
            monitor,
            NULL,
            "monitor");
    puts("done.");

    printf("starting server thread..");
    server_pid = thread_create(
            server_stack,
            sizeof(server_stack),
            PRIORITY_MAIN-2,
            CREATE_STACKTEST | CREATE_SLEEPING,
            expect_traffic,
            NULL,
            "expect_traffic");
    puts("done.");

    /* initialize shell */
    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, shell_readc, shell_putchar);
    shell_run(&shell);

    return 0;
}
