/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/
#ifndef LP_SIGNAL_H
#define LP_SIGNAL_H

/* Function identifier prefix: lpsig_ */

#include <stdint.h>

#include "large_packet.h"

/* Initialize the module, with role as Receiver (root) or Sender. See
 * large_packet.h */
int lpsig_init(
    mira_net_udp_connection_t *udp_connection);

/* Signal to dst that there is a large packet ready for sending */
int lpsig_send(
    const mira_net_address_t *dst,
    uint16_t packet_id,
    uint8_t n_sub_packets);

/* Handle incoming data, if relevant. This function first tests if the data is a
 * valid signal message. If it is, it acts by posting an event. */
void lpsig_handle_data(
    const void *data,
    const uint16_t data_len,
    const mira_net_udp_callback_metadata_t *metadata);

#endif
