/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/
#include <limits.h>
#include <mira.h>
#include <string.h>

#include "large_packet.h"
#include "lp_events.h"

#define DEBUG_LEVEL 2
#include "utils.h"

// ******************************************************************************
// Global variables
// ******************************************************************************
process_event_t event_lp_requested;

// ******************************************************************************
// Module constants
// ******************************************************************************
static const uint8_t lpreq_header[LP_HEADER_SIZE] = {
    0xf2, 0x2a
};

static mira_net_udp_connection_t *lpreq_udp_connection;

// ******************************************************************************
// Module variables
// ******************************************************************************

// ******************************************************************************
// Function prototypes
// ******************************************************************************
static void lpreq_pack_buffer(
    uint8_t *buffer,
    uint16_t packet_id,
    uint64_t mask,
    uint16_t period_ms);

static int lpreq_unpack_buffer(
    uint16_t *packet_id,
    uint64_t *mask,
    uint16_t *period_ms,
    const uint8_t *buffer,
    uint8_t len);

// ******************************************************************************
// Function definitions
// ******************************************************************************
int lpreq_init(
    mira_net_udp_connection_t *udp_connection)
{
    event_lp_requested = process_alloc_event();

    lpreq_udp_connection = udp_connection;

    return 0;
}

int lpreq_send(
    const mira_net_address_t *dst,
    const uint16_t dst_port,
    const uint16_t packet_id,
    const uint64_t sub_packet_mask,
    const uint16_t sub_packet_period_ms)
{
#if DEBUG_LEVEL > 0
    char addr_str_buffer[MIRA_NET_MAX_ADDRESS_STR_LEN];
#endif
    P_DEBUG("Sending lp request to %s: id %d, mask 0x%08lx%08lx, period %d ms\n",
        mira_net_toolkit_format_address(addr_str_buffer, dst),
        packet_id,
        (uint32_t) (sub_packet_mask >> 32),
        (uint32_t) (sub_packet_mask & UINT32_MAX),
        sub_packet_period_ms);

    uint8_t request_buffer[
        sizeof(lpreq_header)
        + sizeof(packet_id)
        + sizeof(sub_packet_mask)
        + sizeof(sub_packet_period_ms)
    ];

    lpreq_pack_buffer(
        request_buffer,
        packet_id,
        sub_packet_mask,
        sub_packet_period_ms);

    P_DEBUG("Request buffer: ");
    for (int i = 0; i < sizeof(request_buffer); ++i) {
        P_DEBUG("0x%02x ", request_buffer[i]);
    }
    P_DEBUG("\n");

    mira_status_t ret =
        mira_net_udp_send_to(
            lpreq_udp_connection,
            dst,
            dst_port,
            request_buffer,
            sizeof(request_buffer));

    if (ret != MIRA_SUCCESS) {
        P_ERR("[%d]: mira_net_udp_send_to\n", ret);
        return -1;
    }
    return 0;
}

void lpreq_handle_data(
    const void *data,
    const uint16_t data_len,
    const mira_net_udp_callback_metadata_t *metadata)
{
    if (memcmp(data, lpreq_header, sizeof(lpreq_header) != 0)) {
        /* Not a request packet */
        return;
    }

    uint16_t packet_id;
    uint64_t mask;
    uint16_t period;
    if (lpreq_unpack_buffer(&packet_id, &mask, &period, data, data_len) < 0) {
        P_ERR("%s: lpreq_unpack_buffer\n", __func__);
        return;
    }

    P_DEBUG(
        "Request received for packet id %d, mask: 0x%08lx%08lx, period: %d ms\n",
        packet_id,
        (uint32_t) (mask >> 32),
        (uint32_t) (mask & UINT32_MAX),
        period);

    /* Post event with data */
    static lp_event_requested_data_t lpreq_event_data;
    lpreq_event_data = (lp_event_requested_data_t) {
        .packet_id = packet_id,
        .mask = mask,
        .period_ms = period,
        .src_port = metadata->source_port,
    };
    memcpy(
        &lpreq_event_data.src,
        metadata->source_address,
        sizeof(mira_net_address_t));

    /* TODO: post to specific processes instead of broadcast? */
    if (process_post(PROCESS_BROADCAST, event_lp_requested, &lpreq_event_data)
        != PROCESS_ERR_OK
    ) {
        P_ERR("%s: process_post!\n", __func__);
        return;
    }
}

// ******************************************************************************
// Internal functions
// ******************************************************************************

/* Large packet request format:
 *
 *  +-------------------+----------------------+----------------+------------------+
 *  | header  (16 bits) |  packet_id (16_bits) | mask (64 bits) | period (16 bits) |
 *  +-------------------+----------------------+----------------+------------------+
 *
 * Little endian.
 */

static void lpreq_pack_buffer(
    uint8_t *buffer,
    uint16_t packet_id,
    uint64_t mask,
    uint16_t period_ms)
{
    memcpy(
        buffer,
        lpreq_header,
        sizeof(lpreq_header));
    buffer += sizeof(lpreq_header);

    LITTLE_ENDIAN_STORE(buffer, packet_id);
    buffer += sizeof(packet_id);

    LITTLE_ENDIAN_STORE(buffer, mask);
    buffer += sizeof(mask);

    LITTLE_ENDIAN_STORE(buffer, period_ms);
    buffer += sizeof(period_ms);
}

static int lpreq_unpack_buffer(
    uint16_t *packet_id,
    uint64_t *mask,
    uint16_t *period_ms,
    const uint8_t *buffer,
    uint8_t len)
{
    if ((packet_id == NULL)
        || (mask == NULL)
        || (period_ms == NULL)
        || (buffer == NULL)
    ) {
        P_ERR("%s: pointer error!\n", __func__);
        return -1;
    }
    if (len != sizeof(lpreq_header)
        + sizeof(*packet_id)
        + sizeof(*mask)
        + sizeof(*period_ms)
    ) {
        P_ERR("%s: wrong lp request packet size (%d)!\n", __func__, len);
        return -1;
    }

    buffer += sizeof(lpreq_header);

    LITTLE_ENDIAN_LOAD(packet_id, buffer);
    buffer += sizeof(*packet_id);

    LITTLE_ENDIAN_LOAD(mask, buffer);
    buffer += sizeof(*mask);

    LITTLE_ENDIAN_LOAD(period_ms, buffer);
    buffer += sizeof(*period_ms);

    return 0;
}
