/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.

This example is provided as is, without warranty.
----------------------------------------------------------------------------*/

#include <mira.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "large_packet.h"
#include "lp_events.h"
#include "lp_signal.h"
#include "network_setup.h"

#define DEBUG_LEVEL 2
#include "utils.h"

/*
 * Identifies as a node.
 * Sends data to the root.
 */
static const mira_net_config_t net_config = {
    .pan_id = PAN_ID,
    .key = ENCRYPTION_KEY,
    .mode = MIRA_NET_MODE_MESH,
    .rate = 10,
    .antenna = MIRA_NET_ANTENNA_ONBOARD,
    .prefix = NULL /* default prefix */
};

static large_packet_t large_packet_tx;

/*
 * How often to check if we have access to root.
 */
#define WAITING_FOR_ROOT_PERIOD_S (5)

/*
 * Give time for the downward route to establish. Having access to root does not
 * mean that the root has access to us.
 */
#define ESTABLISHING_ROUTE_DELAY_S (40)

/*
 * How often to start sending a new large packet.
 */
#define PACKET_GENERATION_PERIOD_S (3 * 60)

/*
 * Large packet to send.
 */
static uint8_t packet_content[] =
    "Lorem ipsum dolor sit amet, consectetaur adipisicing elit, sed do eiusmod tempor"
    "incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis"
    "nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo"
    "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum"
    "dolore eu fugiat nulla pariatur.  Excepteur sint occaecat cupidatat non"
    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\nSed"
    "ut perspiciatis unde omnis iste natus error sit voluptatem accusantium"
    "doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore"
    "veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam"
    "voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia"
    "consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt.  Neque"
    "porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci"
    "velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore"
    "magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum"
    "exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi"
    "consequatur?  Quis autem vel eum iure reprehenderit qui in ea voluptate velit"
    "esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo"
    "voluptas nulla pariatur?\nAt vero eos et accusamus et iusto odio dignissimos"
    "ducimus qui blanditiis praesentium voluptatum deleniti atque corrupti quos"
    "dolores et quas molestias excepturi sint occaecati cupiditate non provident,"
    "similique sunt in culpa qui officia deserunt mollitia animi, id est laborum et"
    "dolorum fuga. Et harum quidem rerum facilis est et expedita distinctio. Nam"
    "libero tempore, cum soluta nobis est eligendi optio cumque nihil impedit quo"
    "minus id quod maxime placeat facere possimus, omnis voluptas assumenda est,"
    "omnis dolor repellendus. Temporibus autem quibusdam et aut officiis debitis aut"
    "rerum necessitatibus saepe eveniet ut et voluptates repudiandae sint et"
    "molestiae non recusandae. Itaque earum rerum hic tenetur a sapiente delectus, ut"
    "aut reiciendis voluptatibus maiores alias consequatur aut perferendis doloribus"
    "asperiores repellat.\n"
    "Lorem ipsum dolor sit amet, consectetaur adipisicing elit, sed do eiusmod tempor"
    "incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis"
    "nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo"
    "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum"
    "dolore eu fugiat nulla pariatur.  Excepteur sint occaecat cupidatat non"
    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\nSed"
    "ut perspiciatis unde omnis iste natus error sit voluptatem accusantium"
    "doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore"
    "veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam"
    "voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia"
    "consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt.  Neque"
    "porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci"
    "velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore"
    "magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum"
    "exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi"
    "consequatur?  Quis autem vel eum iure reprehenderit qui in ea voluptate velit"
    "esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo"
    "voluptas nulla pariatur?\nAt vero eos et accusamus et iusto odio dignissimos"
    "ducimus qui blanditiis praesentium voluptatum deleniti atque corrupti quos"
    "dolores et quas molestias excepturi sint occaecati cupiditate non provident,"
    "similique sunt in culpa qui officia deserunt mollitia animi, id est laborum et"
    "dolorum fuga. Et harum quidem rerum facilis est et expedita distinctio. Nam"
    "libero tempore, cum soluta nobis est eligendi optio cumque nihil impedit quo"
    "minus id quod maxime placeat facere possimus, omnis voluptas assumenda est,"
    "omnis dolor repellendus. Temporibus autem quibusdam et aut officiis debitis aut"
    "rerum necessitatibus saepe eveniet ut et voluptates repudiandae sint et"
    "molestiae non recusandae. Itaque earum rerum hic tenetur a sapiente delectus, ut"
    "aut reiciendis voluptatibus maiores alias consequatur aut perferendis doloribus"
    "asperiores repellat.\n"
    "                           ..,';c;...','       ..,'..;:;,.'..                   \n"
    "                   ...,;,'...                    ......,:,'..                   \n"
    "                ..;c;,..                                .',:l;..                \n"
    "            ...;lc...                                      ...;c,..             \n"
    "           .cl;...                                            ...,c:.           \n"
    "         ..::.                                                    ':c,..        \n"
    "     ......'.                         .... ..'..'''........',',:,'..'.'.        \n"
    "      .........,:.               ':,;;::;;,.,:c,.........  ...';'........       \n"
    "     ...,,;,,,''::.'.            ..''.';;,....:'        ......,;,,'..::. ..     \n"
    "   .'..  'l,.',.;lc;..'.. .     .;'........   ..   .'.,;,,;looc,:,..::.  .;;.   \n"
    "   ...          ..,:'.'cl::'....lo'. .....   .;'  .;olc;'.;l:,. .         .;,.  \n"
    " .';.  ':;;'.     ....,cccll;.';;''...'. ..  .'',,;ccc:;,,,:l,    .''..'.. .,;. \n"
    " .,'  ..'',''',c:',;;,'...','.',:;:::lc,';c;,coc;;,;::;:c:,.'.,ll:,,...''.  .:;.\n"
    ".'.   .,;'''';lko;lOkdl;,,;:cl;':lcl;:doo:,oxxoc;;:lol:,,:'. .;cc:,;;. ..    ';,\n"
    ".'.    ...  'cloc..lx:.'''''''..'....;lcc;,lo;......;'. .::;;c'  ....        .,c\n"
    ",.                 .,.          .'.':cooollcc,.                               .,\n"
    ",.                              .;:okKXOkKKOOd;.                              .:\n"
    ";.                              '::oddO00KK0d;.                               ';\n"
    "                                 .,okoccck0d:'.                                 \n"
    "      ..                         .,dKNx'.ckd:.                                 .\n"
    "                                .;,o0NKxx00xl..                                .\n"
    ",                               .;,d0O0KXXOc...                        ,;      '\n"
    ":.                              .''dkc:cccl:';,.                       lo     .c\n"
    ",.                             '::,x0l;,,;ckNOcl:.                     :c     .;\n"
    "';.                          .;;dl'lkl;;,;,,;,,do,.                    :c    .::\n"
    ".;.                         .';oOo::;;lo:ox:.,cx0l,.                   ll    .:'\n"
    " ',.                        'lcoko;:lkKo.,kl.,'lOd;.                   lo   .,;.\n"
    " ':,.                       .:lccc.:0KO:..lkc'.'okd;.                  ..  .,l, \n"
    " .'':.                     .,:xx:'.'k0d' .cxxc...cdl,.                    .cc:. \n"
    "   .,'                     .':do;. .dk:. .:xo:' ..';ld;.               .. ','.  \n"
    "    .'.                  .;';dxl.  .:lc,...coc'    'lxko'.             ;:.,.    \n"
    "     'l,.               .;,lkko:.  ..;l:'..,lo;.    .:do:,.            .'.      \n"
    "     ,k;.'.             .;lkko:.    .lKkl;.;OXo.     'occc,.         .'''.      \n"
    "     ,x, .''.          .'cxdc'.    .,dK0x;.l0Xo.      .:l:,.       .....;'      \n"
    "     ,d,   .''...     .,:do;.      'dkkl:'.ckOc.       .,:;.   ...'..   .,.     \n"
    "     'd'     ..;;......ldl'        .,dx'...lOo'         .:l;. .;;..     .l'     \n"
    "     'o'         .'.';;c;.          .ok;...dd'           .;;....        .l'     \n"
    "     'l.          ,,,,',,..        .:do. .;:'        ..,''..,'          .c'     \n"
    "     'l,............;:',:::,..,. ..,coc..'c:..  .,..',;;'.  ............':.     \n"
    "     'c:cccccccccccc::ccccc:;,'......''.........','....;,. .,:cccccc::::;:.     \n"
;

MIRA_IODEFS(
    MIRA_IODEF_NONE,    /* fd 0: stdin */
    MIRA_IODEF_UART(0), /* fd 1: stdout */
    MIRA_IODEF_NONE     /* fd 2: stderr */
    /* More file descriptors can be added, for use with dprintf(); */
);

PROCESS(main_proc, "Main process");
PROCESS(packet_ready_notify_proc, "Announce packet ready");
PROCESS(reply_to_request_proc, "React to requests");

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

    process_start(&main_proc, NULL);
}

PROCESS_THREAD(main_proc, ev, data)
{
    PROCESS_BEGIN();
    /* Pause once, so we don't run anything before finish of startup */
    PROCESS_PAUSE();

    printf("Starting Node (Large packet sender).\n");

    if (sizeof(packet_content)
        > LARGE_PACKET_SUBPACKET_MAX_BYTES * LARGE_PACKET_MAX_NUMBER_OF_SUBPACKETS
    ) {
        P_ERR("packet too large! (%d bytes). Aborting.\n", sizeof(packet_content));
        PROCESS_EXIT();
    }

    MIRA_RUN_CHECK(mira_net_init(&net_config));

    process_start(&packet_ready_notify_proc, NULL);
    process_start(&reply_to_request_proc, NULL);

    RUN_CHECK(large_packet_init(LARGE_PACKET_SENDER));

    PROCESS_END();
}

/*
 * Notify the network that this node has something to send.
 */
PROCESS_THREAD(packet_ready_notify_proc, ev, data)
{
    static struct etimer timer;
    static mira_net_address_t net_address;
    static uint16_t packet_id = 0;

    static bool route_established;

    char buffer[MIRA_NET_MAX_ADDRESS_STR_LEN];
    mira_status_t res;

    PROCESS_BEGIN();
    PROCESS_PAUSE();

    while (1) {
        res = mira_net_get_root_address(&net_address);

        if (res != MIRA_SUCCESS) {
            P_DEBUG("Waiting for root address: [%d]\n", res);
            route_established = false;

            etimer_set(&timer, WAITING_FOR_ROOT_PERIOD_S * CLOCK_SECOND);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
        } else {
            if (!route_established) {
                /* Give time to establish route from root and downwards */
                P_DEBUG("Establishing downward route...\n");
                etimer_set(&timer, ESTABLISHING_ROUTE_DELAY_S * CLOCK_SECOND);
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
                route_established = true;
            }
            P_DEBUG("Sending packet ready notification to %s\n",
                mira_net_toolkit_format_address(buffer, &net_address));

            /* Sets the content of the large packet to send */
            int ret = large_packet_register_tx(
                &large_packet_tx,
                packet_id,
                packet_content,
                sizeof(packet_content));

            if (ret < 0) {
                P_ERR("%s: could not register packet %d\n", __func__, packet_id);
            } else {
                /* Send signal about the new packet */
                RUN_CHECK(lpsig_send(
                    &net_address,
                    packet_id,
                    large_packet_n_sub_packets_get(sizeof(packet_content))));
            }

            /* Wait until time for next packet generation */
            etimer_set(&timer, PACKET_GENERATION_PERIOD_S * CLOCK_SECOND);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

            packet_id++;
        }
    }

    PROCESS_END();
}

PROCESS_THREAD(reply_to_request_proc, ev, data)
{
    PROCESS_BEGIN();
    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == event_lp_requested);
        lp_event_requested_data_t req_data = *(lp_event_requested_data_t *) data;

        /* This example replies to requests by immediately starting sending the
         * latest registered large packet. If in need of a smarter behavior,
         * here is the place to do it. */

        large_packet_tx.node_addr = req_data.src;
        large_packet_tx.node_port = req_data.src_port;
        large_packet_tx.id = req_data.packet_id;
        large_packet_tx.mask = req_data.mask;
        large_packet_tx.period_ms = req_data.period_ms;

        RUN_CHECK(large_packet_send(&large_packet_tx));
    }
    PROCESS_END();
}
