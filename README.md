# Large Packet Application

This application demonstrates a way to send large packets into sub-packets with
MiraOS v2. It monitors incoming sub-packets and requests missing sub-packets if
needed.

## Principle

This example implements two nodes: Sender and Receiver (root).

```
Sender
======
Generate data ---> Signal new data

Requested for sub-packets ---> Send requested sub-packets at desired pace

Receiver
========

                                     Signaled!
                                        |
                                        v
                              Request all sub-packets
                                        |
                                        v
    +---------------------------------> o <-----------------+
   /                                    /\                   \
  /                 +------------------+  \                   \
  |                 v                      \                   \
  \           Sub-packet received           \                   \
   +------no--- All received?                \                   \
                   yes |                      v                   \
                       v                  time-out                 |
              Print and Quit    Max re-tries done? --yes--> Abort  |
                                        | no                       |
                                        v                         /
                                 Request missing sub-packets ----+
```

Sender fakes a large packet ready for sending every `PACKET_GENERATION_PERIOD_S`
(see `sender/large_packet_sender.c`) and notifies Receiver with a signal message
(see `common/lp_signal.[ch]`). This signal message includes the number of
sub-packets which constitutes the large packet, as well as an ID number for the
large packet.

Receiver listens to incoming signal messages, and replies with a request to send
the large packet. In this request, Receiver includes a bit mask showing which
sub-packets to send, as well as the requested packet ID number and at which at
period the send sub-packets.

Sender starts sending sub-packets to Receiver. It paces sub-packet transmissions
according to the requested period, in order to avoid saturating transmission
queues.

Receiver keeps track of all received sub-packets, as long as more are missing or
until a new packet ready notification arrives. Once all sub-packets are
received, Receiver displays the large packet.

If sub-packets stop arriving before the whole large packet is transmitted,
Receiver sends a new request, but only for the sub-packets that it lacks. This
happens on a time-out on sub-packet reception. The time-out value depends on the
requested sub-packet period.

## Application

### Building and flashing
```
cd sender
make LIBDIR=<path_to_mira_lib> TARGET=mkw41z-mesh flash.<programmer_serial>
cd ../receiver
make LIBDIR=<path_to_mira_lib> TARGET=mkw41z-root flash.<programmer_serial>
```

### Sender

Process `packet_ready_notify_proc` waits for network connection to root, then
regularly registers a new large packet (although the content is always the
same). It then sends a signal notification about the new packet.

Process `reply_to_request_proc` monitors incoming requests for large packets,
and starts transmission of the requested large packet.

### Receiver

Process `signal_to_request_proc` monitors incoming signal notifications, and
reacts by sending a request for the advertised large packet. It then starts the
processes that handle the sub-packets, and their possible need for new requests,
see Modules.

Process `large_packet_monitor_proc` awaits the event for large packet reception
ready, and prints the received content.

## Modules

### large_packet

Prefix `large_packet_`

Processes requests to send large packets, as well as the reception of sub-packets.

Sender uses this module to send sub-packets in a paced manner, depending on how
it was requested to do so (see module `lp_request`).

Receiver uses this module to handle the reception of sub-packets, determine if
sub-packets are missing, and re-request transmission of these missing
sub-packets. Upon receiving a whole large packet (all its sub-packets), it posts
an event, which the application can use to handle the data. This example prints
the data as a string.

`large_packet_udp_listen_callback` runs at every reception of an UDP packet on
the defined port. This callback then dispatches handling of the content to the
modules described below.

Note: pre-processor define `FAULT_RATE_PERCENT` (default at 0) allows to
simulate packet loss by discarding incoming sub-packets, in order to see the
re-request mechanism at work.

### lp_signal

Prefix `lpsig_`

This module handles signaling of new available large packets. Sender uses this
module to notify the network of a new available large packet. The receiver uses
the module to handle such incoming notifications, and posts an event (with data)
to other processes, if applicable.

### lp_request

Prefix `lpreq_`

This module handles requests for large packets. Receiver send requests to the
sender when ready to receive, asking for sub-packets of a large packet. The
request includes a bit mask, which determines which sub-packets the sender must
send.

Sender uses the module to handle such requests, and posts an event (with data)
to other processes, if applicable.

### lp_subpacket

Prefix `lpsp_`

This module handles sub-packets, transmission and reception.

## Future possible work

### Stability when requested packet not available

This version does not handle inconsistency in requested and available packets
well.

### Check large packet source

This version does not check that all sub-packets come from the same expected source.

### Time-out of large packet

Large packets reception could abort itself after a time-out, regardless of
number of attempted new requests.

