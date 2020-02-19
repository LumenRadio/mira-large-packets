/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/
#include <mira.h>
#include <string.h>

#include "large_packet.h"
#include "lp_events.h"
#include "lp_signal.h"

#define DEBUG_LEVEL 2
#include "utils.h"

// ******************************************************************************
// Global variables
// ******************************************************************************
process_event_t event_lp_signaled_ready;

// ******************************************************************************
// Module constants
// ******************************************************************************
static const uint8_t lpsig_header[LP_HEADER_SIZE] = {
    0x54, 0xab
};

static mira_net_udp_connection_t *lpsig_udp_connection;

// ******************************************************************************
// Module variables
// ******************************************************************************

// ******************************************************************************
// Function prototypes
// ******************************************************************************
static void lpsig_pack_buffer(
    uint8_t *buffer,
    uint16_t packet_id,
    uint8_t n_sub_packets);

static int lpsig_unpack_buffer(
    uint8_t *n_sub_packets,
    uint16_t *packet_id,
    const uint8_t *buffer,
    uint8_t len);

// ******************************************************************************
// Function definitions
// ******************************************************************************
int lpsig_init(
    mira_net_udp_connection_t *udp_connection)
{
    event_lp_signaled_ready = process_alloc_event();

    lpsig_udp_connection = udp_connection;

    return 0;
}

int lpsig_send(
    const mira_net_address_t *dst,
    uint16_t packet_id,
    uint8_t n_sub_packets)
{
    uint8_t packet_ready_message[
        sizeof(lpsig_header)
        + sizeof(packet_id)
        + sizeof(n_sub_packets)
    ];

#if DEBUG_LEVEL > 0
    char addr_str_buffer[MIRA_NET_MAX_ADDRESS_STR_LEN];
#endif
    P_DEBUG("Sending lp signal to %s: id %d, %d sub-packets\n",
        mira_net_toolkit_format_address(addr_str_buffer, dst),
        packet_id,
        n_sub_packets);

    lpsig_pack_buffer(packet_ready_message, packet_id, n_sub_packets);

    mira_status_t ret;
    ret = mira_net_udp_send_to(
        lpsig_udp_connection,
        dst,
        LARGE_PACKET_RX_UDP_PORT,
        packet_ready_message,
        sizeof(packet_ready_message));

    if (ret != MIRA_SUCCESS) {
        P_ERR("[%d]: mira_net_udp_send_to\n", ret);
        return -1;
    }

    return 0;
}

void lpsig_handle_data(
    const void *data,
    const uint16_t data_len,
    const mira_net_udp_callback_metadata_t *metadata)
{
    if (data_len < LP_HEADER_SIZE) {
        P_ERR("%s: packet too short\n", __func__);
        return;
    }

    if (memcmp(data, lpsig_header, sizeof(lpsig_header) != 0)) {
        /* Not a signal packet */
        return;
    }

    /* Beside keeping track of packet_id, this function should do it by message
    source (metadata->source_address). Failing to do so results in mixing up two
    messages with the same packet_id but from different sources. */

    uint8_t n_sub_packets;
    uint16_t packet_id;
    if (lpsig_unpack_buffer(&n_sub_packets, &packet_id, data, data_len) < 0) {
        P_ERR("Invalid notification\n");
        return;
    }

    P_DEBUG("Signal received for packet id %d with %d sub-packets\n",
        packet_id,
        n_sub_packets);

    /* Post event with data */
    static lp_event_signaled_data_t lpsig_event_data;
    lpsig_event_data = (lp_event_signaled_data_t) {
        .n_sub_packets = n_sub_packets,
        .packet_id = packet_id,
        .src_port = metadata->source_port,
    };
    memcpy(
        &lpsig_event_data.src,
        metadata->source_address,
        sizeof(mira_net_address_t));

    if (process_post(PROCESS_BROADCAST, event_lp_signaled_ready, &lpsig_event_data)
        != PROCESS_ERR_OK
    ) {
        P_ERR("%s: process_post\n", __func__);
        return;
    }
}

// ******************************************************************************
// Internal functions
// ******************************************************************************

/* Large packet signal format:
 *
 *  +-------------------+----------------------+------------------------+
 *  | header  (16 bits) |  packet_id (16_bits) | n_sub_packets (8 bits) |
 *  +-------------------+----------------------+------------------------+
 *
 * Little endian.
 */

static void lpsig_pack_buffer(
    uint8_t *buffer,
    uint16_t packet_id,
    uint8_t n_sub_packets)
{
    memcpy(buffer, lpsig_header, sizeof(lpsig_header));
    buffer += sizeof(lpsig_header);

    LITTLE_ENDIAN_STORE(buffer, packet_id);
    buffer += sizeof(packet_id);

    LITTLE_ENDIAN_STORE(buffer, n_sub_packets);
    buffer += sizeof(n_sub_packets);
}

static int lpsig_unpack_buffer(
    uint8_t *n_sub_packets,
    uint16_t *packet_id,
    const uint8_t *buffer,
    uint8_t len)
{
    if ((n_sub_packets == NULL)
        || (packet_id == NULL)
        || (buffer == NULL)
    ) {
        P_ERR("%s: pointer error!\n", __func__);
        return -1;
    }

    if (len != (sizeof(lpsig_header)
                + sizeof(*n_sub_packets)
                + sizeof(*packet_id))
    ) {
        P_ERR("%s: wrong lp signal packet size (%d)!\n", __func__, len);
        return -1;
    }

    buffer += sizeof(lpsig_header);

    LITTLE_ENDIAN_LOAD(packet_id, buffer);
    buffer += sizeof(*packet_id);

    LITTLE_ENDIAN_LOAD(n_sub_packets, buffer);
    buffer += sizeof(*n_sub_packets);

    return 0;
}
