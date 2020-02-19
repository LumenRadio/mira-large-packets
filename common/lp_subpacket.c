/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/
#include <mira.h>
#include <stdint.h>
#include <string.h>

#include "large_packet.h"
#include "lp_events.h"
#include "lp_subpacket.h"

#define DEBUG_LEVEL 2
#include "utils.h"

// ******************************************************************************
// Global variables
// ******************************************************************************
process_event_t event_lp_subpacket_received;

// ******************************************************************************
// Module constants
// ******************************************************************************
static const uint8_t lpsp_header[LP_HEADER_SIZE] = {
    0x1f, 0xb3
};

// ******************************************************************************
// Module variables
// ******************************************************************************
static mira_net_udp_connection_t *lpsp_udp_connection;

// ******************************************************************************
// Function prototypes
// ******************************************************************************
static void lpsp_pack_buffer(
    uint8_t *buffer,
    uint16_t packet_id,
    uint8_t sub_packet_index,
    uint8_t n_sub_packets,
    const uint8_t *payload,
    uint16_t payload_len);

static int lpsp_unpack_buffer(
    uint16_t *packet_id,
    uint8_t *sub_packet_index,
    uint8_t *n_sub_packets,
    uint16_t *payload_len,
    uint8_t *payload,
    const uint8_t *buffer,
    uint16_t buf_len);

// ******************************************************************************
// Function definitions
// ******************************************************************************
int lpsp_init(
    mira_net_udp_connection_t *udp_connection)
{
    lpsp_udp_connection = udp_connection;

    event_lp_subpacket_received = process_alloc_event();

    return 0;
}

int lpsp_send(
    const mira_net_address_t *dst,
    uint16_t dst_port,
    uint16_t packet_id,
    uint8_t sub_packet_index,
    uint8_t n_sub_packets,
    const uint8_t *data,
    const uint16_t data_len)
{
    uint8_t sub_packet_frame[
        sizeof(lpsp_header)
        + sizeof(packet_id)
        + sizeof(sub_packet_index)
        + sizeof(n_sub_packets)
        + sizeof(data_len)
        + data_len
    ];

    lpsp_pack_buffer(
        sub_packet_frame,
        packet_id,
        sub_packet_index,
        n_sub_packets,
        data,
        data_len);

    mira_status_t ret = mira_net_udp_send_to(
        lpsp_udp_connection,
        dst,
        dst_port,
        sub_packet_frame,
        sizeof(sub_packet_frame));

    if (ret != MIRA_SUCCESS) {
        P_ERR("%s: could not send on UDP\n", __func__);
        return -1;
    }
    return 0;
}

void lpsp_handle_data(
    const void *data,
    const uint16_t data_len,
    const mira_net_udp_callback_metadata_t *metadata)
{
    if (data_len < LP_HEADER_SIZE) {
        P_ERR("%s: packet too short\n", __func__);
        return;
    }

    if (memcmp(data, lpsp_header, sizeof(lpsp_header) != 0)) {
        /* Not a sub-packet */
        return;
    }

    /* Beside keeping track of packet_id, this function should do it by message
    source (metadata->source_address). Failing to do so results in mixing up two
    messages with the same packet_id but from different sources. */

    uint16_t packet_id;
    uint8_t sub_packet_index;
    uint8_t n_sub_packets;
    static uint8_t payload[LARGE_PACKET_SUBPACKET_MAX_BYTES];
    uint16_t payload_len;

    if (lpsp_unpack_buffer(
        &packet_id,
        &sub_packet_index,
        &n_sub_packets,
        &payload_len,
        payload,
        data,
        data_len) < 0
    ) {
        P_ERR("%s: invalid sub-packet\n", __func__);
        return;
    }

    /* Post event with data */
    static lp_event_subpacket_data_t lpsp_event_data;
    lpsp_event_data = (lp_event_subpacket_data_t) {
        .packet_id = packet_id,
        .sub_packet_index = sub_packet_index,
        .n_sub_packets = n_sub_packets,
        .payload_len = payload_len,
        .payload = payload,
        .src_port = metadata->source_port,
    };
    memcpy(
        &lpsp_event_data.src,
        metadata->source_address,
        sizeof(mira_net_address_t));

    if (process_post(
        PROCESS_BROADCAST,
        event_lp_subpacket_received,
        &lpsp_event_data)
        != PROCESS_ERR_OK
    ) {
        P_ERR("%s: process_post\n", __func__);
        return;
    }
}

// ******************************************************************************
// Internal functions
// ******************************************************************************

/* Sub-packet format:
 *
 *  +-------------------+----------------------+---------------------------+
 *  | header  (16 bits) |  packet_id (16_bits) | sub_packet_index (8 bits) | ...
 *  +-------------------+----------------------+------------------------+--+
 *
 *  +----------------------+----------------------+-----------------------------+
 *  n_sub_packets (8 bits) | payload_len (8 bits) | payload (payload_len bytes) |
 *  +----------------------+----------------------+-----------------------------+
 *
 * Little endian.
 */

static void lpsp_pack_buffer(
    uint8_t *buffer,
    uint16_t packet_id,
    uint8_t sub_packet_index,
    uint8_t n_sub_packets,
    const uint8_t *payload,
    uint16_t payload_len)
{
    memcpy(buffer, &lpsp_header, sizeof(lpsp_header));
    buffer += sizeof(lpsp_header);

    LITTLE_ENDIAN_STORE(buffer, packet_id);
    buffer += sizeof(packet_id);

    LITTLE_ENDIAN_STORE(buffer, sub_packet_index);
    buffer += sizeof(sub_packet_index);

    LITTLE_ENDIAN_STORE(buffer, n_sub_packets);
    buffer += sizeof(n_sub_packets);

    LITTLE_ENDIAN_STORE(buffer, payload_len);
    buffer += sizeof(payload_len);

    memcpy(buffer, payload, payload_len);
    buffer += payload_len;
}

static int lpsp_unpack_buffer(
    uint16_t *packet_id,
    uint8_t *sub_packet_index,
    uint8_t *n_sub_packets,
    uint16_t *payload_len,
    uint8_t *payload,
    const uint8_t *buffer,
    uint16_t buf_len)
{
    if ((packet_id == NULL)
        || (sub_packet_index == NULL)
        || (n_sub_packets == NULL)
        || (payload_len == NULL)
        || (payload == NULL)
        || (buffer == NULL)
    ) {
        P_ERR("%s: pointer error!\n", __func__);
        return -1;
    }

    /* Discard header, which the caller must check before unpacking. */
    buffer += sizeof(lpsp_header);

    LITTLE_ENDIAN_LOAD(packet_id, buffer);
    buffer += sizeof(*packet_id);

    LITTLE_ENDIAN_LOAD(sub_packet_index, buffer);
    buffer += sizeof(*sub_packet_index);

    LITTLE_ENDIAN_LOAD(n_sub_packets, buffer);
    buffer += sizeof(*n_sub_packets);

    LITTLE_ENDIAN_LOAD(payload_len, buffer);
    buffer += sizeof(*payload_len);

    if (buf_len != sizeof(lpsp_header)
        + sizeof(*packet_id)
        + sizeof(*sub_packet_index)
        + sizeof(*n_sub_packets)
        + sizeof(*payload_len)
        + *payload_len
    ) {
        P_ERR(
            "%s: wrong sub-packet size (%d). Payload size: %d\n",
            __func__,
            buf_len,
            *payload_len);
        return -1;
    }

    memcpy(payload, buffer, *payload_len);
    buffer += *payload_len;

    return 0;
}
