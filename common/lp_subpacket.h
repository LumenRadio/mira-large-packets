/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/

#ifndef LP_SUBPACKET_H
#define LP_SUBPACKET_H

#include <mira.h>
#include <stdint.h>

/* Initialize the module, with UDP setup to send messages. */
int lpsp_init(
    mira_net_udp_connection_t *udp_connection);

/* Send sub-packet to dst */
int lpsp_send(
    const mira_net_address_t *dst,
    uint16_t dst_port,
    uint16_t packt_id,
    uint8_t sub_packet_index,
    uint8_t n_sub_packets,
    const uint8_t *data,
    const uint16_t data_len);

/* Handle incoming data, if relevant. This function first tests if the data is a
 * valid sub-packet message. If it is, it acts by posting an event. */
void lpsp_handle_data(
    const void *data,
    const uint16_t data_len,
    const mira_net_udp_callback_metadata_t *metadata);

#endif
