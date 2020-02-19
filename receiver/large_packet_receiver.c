/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.

This example is provided as is, without warranty.
----------------------------------------------------------------------------*/

#include <mira.h>
#include <stdint.h>
#include <stdio.h>

#include "large_packet.h"
#include "lp_events.h"
#include "lp_request.h"
#include "lp_signal.h"
#include "network_setup.h"

#define DEBUG_LEVEL 2
#include "utils.h"

// ******************************************************************************
// Module types
// ******************************************************************************

// ******************************************************************************
// Module constants
// ******************************************************************************

/* Sub-packet period to request. This must be large enough so that TX queue do
 * no fill up, depending on the receiver's listening rate. */
#define SUB_PACKET_PERIOD_REQUEST_MS (800)

static const mira_net_config_t net_config = {
    .pan_id = PAN_ID,
    .key = ENCRYPTION_KEY,
    .mode = MIRA_NET_MODE_ROOT,
    .rate = MIRA_NET_RATE_FAST,
    .antenna = MIRA_NET_ANTENNA_ONBOARD,
    .prefix = NULL /* default prefix */
};

MIRA_IODEFS(
    MIRA_IODEF_NONE,    /* fd 0: stdin */
    MIRA_IODEF_UART(0), /* fd 1: stdout */
    MIRA_IODEF_NONE     /* fd 2: stderr */
    /* More file descriptors can be added, for use with dprintf(); */
);

// ******************************************************************************
// Module variables
// ******************************************************************************
/* +1 for possible extra string termination */
static uint8_t large_packet_payload_storage[
    LARGE_PACKET_SUBPACKET_MAX_BYTES * LARGE_PACKET_MAX_NUMBER_OF_SUBPACKETS + 1];
static large_packet_t large_packet_rx;

// ******************************************************************************
// Function prototypes
// ******************************************************************************

PROCESS(main_proc, "Main process");
PROCESS(signal_to_request_proc, "Reply to signal with request process");
PROCESS(large_packet_monitor_proc, "Monitor incoming large packets");

// ******************************************************************************
// Function definitions
// ******************************************************************************
void mira_setup(
    void)
{
    mira_status_t uart_ret;
    mira_uart_config_t uart_config = {
        .baudrate = 115200,
        .tx_pin = MIRA_GPIO_PIN('C', 8),
        .rx_pin = MIRA_GPIO_PIN('C', 7)
    };

    uart_ret = mira_uart_init(0, &uart_config);
    if (uart_ret != MIRA_SUCCESS) {
        /* Nowhere to send an error message */
    }

    /* Setup large packet storage */
    large_packet_rx = (large_packet_t) {
        0
    };
    large_packet_rx.payload = large_packet_payload_storage;

    process_start(&main_proc, NULL);
}

// ******************************************************************************
// Internal functions
// ******************************************************************************
PROCESS_THREAD(main_proc, ev, data)
{
    PROCESS_BEGIN();
    /* Pause once, so we don't run anything before finish of startup. */
    PROCESS_PAUSE();

    printf("Starting Root (Large packet receiver).\n");

    MIRA_RUN_CHECK(mira_net_init(&net_config));

    RUN_CHECK(large_packet_init(LARGE_PACKET_RECEIVER));
    process_start(&signal_to_request_proc, NULL);
    process_start(&large_packet_monitor_proc, NULL);

    PROCESS_END();
}

PROCESS_THREAD(signal_to_request_proc, ev, data)
{
    PROCESS_BEGIN();

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == event_lp_signaled_ready);
        lp_event_signaled_data_t signaled_data = *(lp_event_signaled_data_t *) data;

        /* This example always replies to signal with an immediate request. If
         * there is need to schedule requests of large packets in a smarter way,
         * here would be the place to do it. */

        /* Request the whole large packet. */
        static uint64_t mask;

        int ret;
        ret = large_packet_send_whole_mask_get(&mask, signaled_data.n_sub_packets);

        if (ret < 0) {
            P_ERR("%s: could not get mask for large packet\n", __func__);
            continue;
        }

        /* Setting up for reception */
        large_packet_rx = (large_packet_t) {
            .node_addr = signaled_data.src,
            .node_port = signaled_data.src_port,
            .payload = large_packet_payload_storage,
            .len = 0,
            .id = signaled_data.packet_id,
            .period_ms = SUB_PACKET_PERIOD_REQUEST_MS,
            .mask = 0, /* bit at 1 means sub-packet received */
            .num_sub_packets = signaled_data.n_sub_packets
        };

        /* Send request for sub-packets, back to the signaling node */
        RUN_CHECK(lpreq_send(
            &signaled_data.src,
            signaled_data.src_port,
            signaled_data.packet_id,
            mask,
            SUB_PACKET_PERIOD_REQUEST_MS));

        /* Start or restart sub-packet rx monitor */
        process_exit(&large_packet_receive_proc);
        process_start(&large_packet_receive_proc, &large_packet_rx);
    }

    PROCESS_END();
}

PROCESS_THREAD(large_packet_monitor_proc, ev, data)
{
    PROCESS_BEGIN();

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == event_lp_received);
        printf("Large packet received, %d bytes\n", large_packet_rx.len);
        large_packet_rx.payload[large_packet_rx.len] = '\0';
        printf("%s\n", large_packet_rx.payload);
    }

    PROCESS_END();
}
