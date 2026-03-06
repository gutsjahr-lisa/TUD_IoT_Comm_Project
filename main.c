// Our team does 802.15.4

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "board.h"
#include "nrf802154.h"
#include "net/netdev.h"
#include "net/netdev/ieee802154.h"
#include "net/netdev/ieee802154_submac.h"
#include "net/ieee802154.h"
#include "event.h"
#include "event/thread.h"
#include "ztimer.h"

#define TX_INTERVAL_MS  100U        /* 10 packets/second */
#define CHANNEL         26U         /* 2480 MHz */
#define PAN_ID          0xABCD
#define TX_ADDR         0x0001

static netdev_ieee802154_submac_t _nrf_netdev;
static netdev_t *_dev;
static uint8_t _rx_buf[IEEE802154_FRAME_LEN_MAX];

/* ISR event: posted from radio ISR, handled in the event thread */
static void _isr_handler(event_t *e);
static event_t _isr_event = { .handler = _isr_handler };

static void _isr_handler(event_t *e)
{
    (void)e;
    _dev->driver->isr(_dev);
}

static void _event_cb(netdev_t *dev, netdev_event_t event)
{
    if (event == NETDEV_EVENT_ISR) {
        event_post(EVENT_PRIO_HIGHEST, &_isr_event);
    }
    else if (event == NETDEV_EVENT_RX_COMPLETE) {
        netdev_ieee802154_rx_info_t info;
        int len = dev->driver->recv(dev, _rx_buf, sizeof(_rx_buf), &info);
        if (len > 0) {
            char node_id = (len > 9) ? (char)_rx_buf[9] : '?';
            printf("%c,%"PRIu32",%d,%u\n",
                   node_id,
                   ztimer_now(ZTIMER_MSEC),
                   (int8_t)info.rssi,
                   (unsigned)info.lqi);
        }
    }
}

#if TX_NODE
static uint8_t _tx_frame[IEEE802154_FRAME_LEN_MAX];
static uint8_t _tx_seq = 0;
static ztimer_t _tx_timer;

static void _tx_handler(event_t *e);
static event_t _tx_event = { .handler = _tx_handler };

static void _tx_timer_cb(void *arg)
{
    (void)arg;
    event_post(EVENT_PRIO_HIGHEST, &_tx_event);
}

/*
 * IEEE 802.15.4 data frame: FCF[2] + Seq[1] + DstPAN[2] + DstAddr[2] + SrcAddr[2] + Payload
 * FCF=0x8841: data, short addr, intra-PAN, no ACK, frame version 2003
 */
static int _build_frame(uint8_t *buf, uint8_t seq)
{
    buf[0] = 0x41; buf[1] = 0x88;          /* FCF */
    buf[2] = seq;
    buf[3] = PAN_ID & 0xFF; buf[4] = PAN_ID >> 8;  /* dst PAN */
    buf[5] = 0xFF;  buf[6] = 0xFF;          /* dst: broadcast */
    buf[7] = TX_ADDR & 0xFF; buf[8] = TX_ADDR >> 8; /* src addr */
    buf[9]  = NODE_ID[0];                   /* payload: node identity */
    buf[10] = seq;
    return 11;
}

static void _tx_handler(event_t *e)
{
    (void)e;
    int len = _build_frame(_tx_frame, _tx_seq++);
    iolist_t pkt = { .iol_next = NULL, .iol_base = _tx_frame, .iol_len = (size_t)len };
    _dev->driver->send(_dev, &pkt);
    ztimer_set(ZTIMER_MSEC, &_tx_timer, TX_INTERVAL_MS);
}
#endif /* TX_NODE */

int main(void)
{
    /* Wait for USB CDC to enumerate so early printf output is not lost */
    ztimer_sleep(ZTIMER_MSEC, 2000);

    printf("802.15.4 RSSI Logger\n");

    /* Init submac netdev (wraps the Radio HAL into a netdev interface) */
    netdev_ieee802154_submac_init(&_nrf_netdev);
    _dev = &_nrf_netdev.dev.netdev;
    _dev->event_callback = _event_cb;

    /* Hook up the nrf802154 Radio HAL to the submac */
    nrf802154_hal_setup(&_nrf_netdev.submac.dev);
    nrf802154_init();

    if (_dev->driver->init(_dev) < 0) {
        printf("ERROR: radio init failed\n");
        return 1;
    }

    /* Configure channel, PAN and maximum TX power (8 dBm on nRF52840) */
    uint16_t chan  = CHANNEL;
    uint16_t pan   = PAN_ID;
    int16_t  txpow = 8;
    _dev->driver->set(_dev, NETOPT_CHANNEL,  &chan,  sizeof(chan));
    _dev->driver->set(_dev, NETOPT_NID,      &pan,   sizeof(pan));
    _dev->driver->set(_dev, NETOPT_TX_POWER, &txpow, sizeof(txpow));

#if TX_NODE
    LED0_ON;
    printf("Role: TX\n");
    printf("Channel %u  PAN 0x%04X  10 pkt/s\n", CHANNEL, PAN_ID);

    /* Kick off the periodic TX timer; _tx_handler re-arms it after each send */
    _tx_timer.callback = _tx_timer_cb;
    _tx_timer.arg = NULL;
    ztimer_set(ZTIMER_MSEC, &_tx_timer, TX_INTERVAL_MS);

#else /* RX_NODE */
    LED1_ON;
    printf("Role: RX\n");
    printf("node_id,timestamp_ms,rssi_dBm,lqi\n");
#endif

    /* Main thread sleeps; the event_thread module runs EVENT_PRIO_HIGHEST loop */
    thread_sleep();
    return 0;
}
