/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/
#ifndef LP_EVENTS_H
#define LP_EVENTS_H

/* Event: received a notification for large packet available */
extern process_event_t event_lp_signaled_ready;
typedef struct {
    uint8_t n_sub_packets;
    uint16_t packet_id;
    mira_net_address_t src;
    uint16_t src_port;
} lp_event_signaled_data_t;

/* Event: received a request for large packet, with selected sub-packets */
extern process_event_t event_lp_requested;
typedef struct {
    uint16_t packet_id;
    uint64_t mask;
    uint16_t period_ms;
    /* source and port of the request, used as destination for large packet */
    mira_net_address_t src;
    uint16_t src_port;
} lp_event_requested_data_t;

/* Event: received a sub-packet */
extern process_event_t event_lp_subpacket_received;
typedef struct {
    uint16_t packet_id;
    uint8_t sub_packet_index;
    uint8_t n_sub_packets;
    uint16_t payload_len;
    uint8_t *payload;
    mira_net_address_t src;
    uint16_t src_port;
} lp_event_subpacket_data_t;

/* Event: received a large packet */
extern process_event_t event_lp_received;

#endif
