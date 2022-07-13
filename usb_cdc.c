#include <xc.h>
#include <string.h>
#include "usb_cdc.h"
#include "usbdesc.h"
#include "usb.h"
#include "usart.h"

#define CDC_READY 0
#define CDC_BUSY 1

#define FIFO_SIZE 16

uint8_t cdc_state;

typedef struct fifo {
    char buf[FIFO_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} cfifo_t;

char txbuf[8];

cfifo_t fifo_in, fifo_out;

extern void usb_cdc_callback(uint8_t* buf, uint8_t len);

static volatile LineCoding_t line;
extern uint8_t ep_data_buffer[64];
extern BD_endpoint_t endpoints[EP_NUM_MAX];

int put_fifo(cfifo_t* fifo, uint8_t* d, uint8_t len) {
    int t;
    if (len + fifo->count > FIFO_SIZE) {
        return -1;
    }
    if (len + fifo->head > FIFO_SIZE) {
        t = FIFO_SIZE - fifo->head;
        memcpy(&fifo->buf[fifo->head], &d[0], t);
        memcpy(&fifo->buf[0], &d[t], (fifo->head + len) - FIFO_SIZE);
        fifo->head = (fifo->head + len) - FIFO_SIZE;
    } else {
        memcpy(&fifo->buf[fifo->head], &d[0], len);
        fifo->head += len;
    }
    fifo->count += len;
    return len;
}

int get_fifo(cfifo_t* fifo, uint8_t* d, uint8_t len) {
    int t;
    if (fifo->count - len < 0) len=fifo->count;
    if (fifo->tail + len > FIFO_SIZE) {
        t = FIFO_SIZE - fifo->tail;
        memcpy(&d[0], &fifo->buf[fifo->tail], t);
        memcpy(&d[t], &fifo->buf[0], (fifo->tail + len) - FIFO_SIZE);
        fifo->tail = (fifo->tail + len) - FIFO_SIZE;
    } else {
        memcpy(&d[0], &fifo->buf[fifo->tail], len);
        fifo->tail += len;
    }
    fifo->count -= len;
    return len;
}

void cdc_init_endpoints() {
    configureEndpointIn(1, &ep_data_buffer[EP1_IN_OFFSET], EP1_BUFF_SIZE);
    configureEndpointOut(1, &ep_data_buffer[EP1_OUT_OFFSET], EP1_BUFF_SIZE);
    configureEndpointIn(2, &ep_data_buffer[EP2_IN_OFFSET], EP2_BUFF_SIZE);
    UEP1 = EP_IN | EP_OUT | EP_HSHK;
    UEP2 = EP_IN | EP_HSHK;
    usbEngageEndpointIn(1, 0);
    usbEngageEndpointOut(1, EP1_BUFF_SIZE);
    usbEngageEndpointIn(2, 0);
    cdc_state = CDC_READY;
}

void init_cdc(){
    cdc_state=CDC_READY;
    line.DTERRate=9600;
    line.ParityType=0;
    line.CharFormat=0;
    line.DataBits = 8;

    fifo_in.head = 0;
    fifo_in.tail = 0;
    fifo_in.count = 0;

    fifo_out.head = 0;
    fifo_out.tail = 0;
    fifo_out.count = 0;

    cdc_state = CDC_BUSY;
}

void process_cdc_request(USB_SETUP_t* usb_setup) {
    switch (usb_setup->bRequest) {
        case GET_LINE_CODING:
            ctl_send(&line, 7);
            break;  
        case SET_LINE_CODING:
            ctl_recv(&line, 7);
            break;  
        case SET_CONTROL_LINE_STATE:
        case SEND_BREAK:
        case SEND_ENCAPSULATED_COMMAND:
            ctl_ack();
            break;
        default:
            ep0_stall();
    }
}

void send_cdc_buf(uint8_t* buf, uint8_t len) {
    if (cdc_state == CDC_READY) {
        put_fifo(&fifo_in, buf, len);
    }
}

uint8_t handle_cdc_in() {
    uint8_t pkt_len = MIN(fifo_in.count, EP1_BUFF_SIZE);
    if (pkt_len > 0) {
        get_fifo(&fifo_in, &txbuf[0], pkt_len);
        copyPacketToEp(1, &txbuf[0], pkt_len);
    }
    return pkt_len;
}

void handle_cdc_out(uint8_t* buf, uint8_t len) {
    usb_cdc_callback(buf, len);
}