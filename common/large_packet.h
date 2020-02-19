/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/
#ifndef LARGE_PACKET_H
#define LARGE_PACKET_H

#include <mira.h>

#include <stdbool.h>
#include <stdint.h>

/* Open port receiver for signals */
#define LARGE_PACKET_RX_UDP_PORT   (1520)

/* Size of single frames to split the large packet into.  This size may be
 * larger than max payload for a single radio packet, in which case Mira
 * (6LoWPAN) divides the sub-packet into fragments. This has the advantage of
 * reducing overhead, at the cost of possible increase of number
 * re-transmissions. */
#define LARGE_PACKET_SUBPACKET_MAX_BYTES     (330)

/* Max number of messages into which a large packet may be split */
/* The bit mask sent in requests must be large enough to accommodate for this
 * number of sub-packets. */
#define LARGE_PACKET_MAX_NUMBER_OF_SUBPACKETS  (64)

/* Byte size of headers, which determines the type of message. */
#define LP_HEADER_SIZE (2)

/* Only one of two roles currently supported */
typedef enum {
    LARGE_PACKET_RECEIVER,
    LARGE_PACKET_SENDER,
} large_packet_role_t;

/* Type used both on the receiving and the sending nodes */
typedef struct {
    uint8_t *payload;
    uint16_t len;
    /* Address and port to the other node participating in the communication */
    mira_net_address_t node_addr;
    uint16_t node_port;
    uint16_t id;
    uint16_t period_ms;
    uint64_t mask; /* bit 1 for sub-packets to send, or received */
    uint8_t num_sub_packets;
} large_packet_t;

int large_packet_init(
    large_packet_role_t role);

/* Get the mask for requesting all sub-packets */
int large_packet_send_whole_mask_get(
    uint64_t *mask,
    const uint16_t n_sub_packets);

/* Get number of sub-packets that make up a large packet of size n_bytes. */
uint8_t large_packet_n_sub_packets_get(
    const uint16_t n_bytes);

/* Register the data to send. Transmission occurs only when requested by a
 * receiver. */
int large_packet_register_tx(
    large_packet_t *large_packet,
    const uint16_t packet_id,
    uint8_t *payload,
    const uint16_t len);

/* Send the registered large packet. */
int large_packet_send(
    large_packet_t *large_packet);

/* Request sub-packets from dst, only the sub-packets defined by sub_packet_mask
 * bit at 1. */
int large_packet_request(
    const mira_net_address_t *dst,
    const uint64_t sub_packet_mask,
    const uint16_t sub_packet_period_ms);

/* Start this process upon sending requests, to handle reception. */
PROCESS_NAME(large_packet_receive_proc);

#endif
