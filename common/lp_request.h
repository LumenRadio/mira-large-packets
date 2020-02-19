/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/

#ifndef LP_REQUEST_H
#define LP_REQUEST_H

/* Function identifier prefix: lpreq_ */

#include <stdint.h>

#include "large_packet.h"

int lpreq_init(
    mira_net_udp_connection_t *udp_connection);

/* Send a request for large packet */
int lpreq_send(
    const mira_net_address_t *dst,
    const uint16_t port,
    const uint16_t packet_id,
    const uint64_t sub_packet_mask,
    const uint16_t sub_packet_period_ms);

/* Handle incoming data, if relevant. This function first tests if the data is a
 * valid request message. If it is, it acts by posting an event. */
void lpreq_handle_data(
    const void *data,
    const uint16_t data_len,
    const mira_net_udp_callback_metadata_t *metadata);

#endif
