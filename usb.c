/*========================================
 * Author: Grinev Roman
 * Date: JUL.2018
 * File: usb.c
 * Description: USB device stack
 * Version: 0.4
 * Version history:
 * v0.1 initial release
 * v0.2 wrong windows device descriptor
 * request fixed
 * v0.3 STATUS stage now works
 * v0.4 GET_DESCRIPTOR bug fixed
 * Comments:
=========================================*/
#include "usb.h"
#include "usbdesc.h"
#include <xc.h>
#include <string.h>
#include "usb_cdc.h"
#include "led.h"

#define FULL_SPEED

#define BD_BASE_ADDR 0x200
#define BD_DATA_ADDR 0x280

volatile BD_endpoint_t endpoints[EP_NUM_MAX] __at(BD_BASE_ADDR);
volatile uint8_t ep_data_buffer[128] __at(BD_DATA_ADDR);
USB_SETUP_t* usb_setup;

static uint8_t dev_addr;
static uint8_t control_needs_zlp;
static TRANSACTION_STAGE ctl_stage;
USB_STATE state;

static uint16_t status = 0x00;
static uint8_t current_cnf = 0;
static uint8_t alt_if = 0;
static uint8_t active_protocol = 0;

static uint16_t wCount;
static char* ubuf;
static uint8_t ctrl_transaction_owner;

void soft_detach();
void init_usb();
void usb_poll();

static void STATUS_OUT_TOKEN();
static void STATUS_IN_TOKEN();
static void reset_usb();

static void SetupStage(USB_SETUP_t* usb_setup);
static void DataInStage();
static uint8_t DataOutStage();
static void WaitForSetupStage(void);

static void process_standart_request(USB_SETUP_t *usb_setup);

void usbEngageEndpointIn(uint8_t ep, uint8_t len) {
    endpoints[ep].in.CNT = len;
    endpoints[ep].in.STAT.Val &= _DTSMASK;
    endpoints[ep].in.STAT.DTS =~ endpoints[ep].in.STAT.DTS;
    endpoints[ep].in.STAT.Val |= _USIE | _DTSEN;
}

void usbEngageEndpointOut(uint8_t ep, uint8_t len) {
    endpoints[ep].out.CNT = len;
    endpoints[ep].out.STAT.Val &= _DTSMASK;
    endpoints[ep].out.STAT.DTS =~ endpoints[ep].out.STAT.DTS;
    endpoints[ep].out.STAT.Val |= _USIE | _DTSEN;
}

void usbEngageSetupEp0() {
    endpoints[0].out.STAT.Val = 0x00;
    endpoints[0].out.CNT = EP0_BUFF_SIZE;
    endpoints[0].out.STAT.Val = _USIE | _DAT0 | _DTSEN;
    endpoints[0].in.STAT.Val = 0;
    UCONbits.PKTDIS = 0;
}

void usbDisengageEP0() {
    endpoints[0].in.STAT.UOWN = 0;
    endpoints[0].out.STAT.UOWN = 0;
}

void copyPacketToEp(uint8_t ep, uint8_t *buf, uint8_t len) {
    if (buf != 0 && len > 0) {
        memcpy(endpoints[ep].in.ADR, buf, len);
    }    
}

static void usb_status_out() {
    endpoints[0].out.CNT = 0x00;
    endpoints[0].out.STAT.Val = _USIE | _DTSEN | _DAT1;
}

static void usb_status_in() {
    endpoints[0].in.CNT = 0x00;
    endpoints[0].in.STAT.Val = _USIE | _DTSEN | _DAT1;
}

void ep0_stall() {
    endpoints[0].in.STAT.Val = _BSTALL | _USIE;
    endpoints[0].out.STAT.Val = _BSTALL | _USIE;
}

static void reset_usb() {
    UADDR = 0;
    UIR = 0x00;
    UIE = 0x00;
    
    UCFGbits.PPB0=0;
    UCFGbits.PPB1=0;
    UCFGbits.UPUEN=1;
#ifdef LOW_SPEED
    UCFGbits.FSEN=0;
#endif
#ifdef FULL_SPEED
    UCFGbits.FSEN=1;
#endif
    UCONbits.SUSPND=0;
    UCONbits.RESUME=0;
    UCONbits.PPBRST=0;
    UCONbits.USBEN=1;
    UIRbits.TRNIF = 0; 
    UIRbits.TRNIF = 0;
    UIRbits.TRNIF = 0;
    UIRbits.TRNIF = 0;

    UIEbits.TRNIE=1;
    UIEbits.URSTIE=1;
    UIEbits.SOFIE=0;
    UIEbits.STALLIE=1;
    UIEbits.ACTVIE=0;
    UIEbits.IDLEIE=1;
    USBIE = 1;
    // Enable EP0
    configureEndpointOut(0, &ep_data_buffer[EP0_OUT_OFFSET], EP0_BUFF_SIZE);
    configureEndpointIn(0, &ep_data_buffer[EP0_IN_OFFSET], EP0_BUFF_SIZE);
    usb_setup = &ep_data_buffer[0];
    UEP0 = EP_IN | EP_OUT | EP_HSHK;
    state = ATTACHED;
    // Prepare to receive first SETUP packet
    WaitForSetupStage();
}

void init_usb() {
    dev_addr = 0;
    reset_usb();
}

void configureEndpointIn(uint8_t ep, uint8_t* buf, uint8_t len) {
    endpoints[ep].in.ADR = buf;
    endpoints[ep].in.CNT = len;
    endpoints[ep].in.STAT.Val = 0x00;
}

void configureEndpointOut(uint8_t ep, uint8_t* buf, uint8_t len) {
    endpoints[ep].out.ADR = buf;
    endpoints[ep].out.CNT = len;
    endpoints[ep].out.STAT.Val = 0x00;
}

void ctl_send(uint8_t* data, uint16_t len) {
    control_needs_zlp = ((len > EP0_BUFF_SIZE) && (len % EP0_BUFF_SIZE == 0)) ? 1 : 0; 
    wCount = len;
    ubuf = data;
    ctl_stage = DATA_IN;
    DataInStage();
}

void ctl_recv(char* data, uint16_t len) {
    wCount = len;
    ubuf = data;
    ctl_stage = DATA_OUT;
    usbEngageEndpointOut(0, EP1_BUFF_SIZE);
}

void ctl_ack() {
    wCount = 0;
    ubuf = 0;
    ctl_stage = _STATUS;
    usb_status_in();
}

static void process_standart_request(USB_SETUP_t* usb_setup) {
    uint16_t len = 0;
    uint16_t request = usb_setup->bRequest | (usb_setup->bmRequestType << 8);
    switch (request) {
        case STD_CLEAR_FEATURE_INTERFACE:
        case STD_SET_FEATURE_INTERFACE:
        case STD_CLEAR_FEATURE_ENDPOINT:
        case STD_SET_FEATURE_ENDPOINT:
        case STD_CLEAR_FEATURE_ZERO:
        case STD_SET_FEATURE_ZERO:
            ctl_ack();
            break;
        case STD_GET_STATUS_INTERFACE:
            ctl_send(&status, 2);
            break;
        case STD_GET_INTERFACE:
            ctl_send(&alt_if, 1);
            break;
        case STD_SET_INTERFACE:
            ctl_ack();
            alt_if = usb_setup->wValueH;
            break;
        case STD_GET_STATUS_ENDPOINT:
            ctl_send(&status, 2);
            break;
        case STD_GET_STATUS_ZERO:
            ctl_send(&status, 2);
            break;
        case STD_SET_DESCRIPTOR:
            ctl_recv(0, usb_setup->wLen);
            break;
        case STD_GET_DESCRIPTOR_INTERFACE:
        case STD_GET_DESCRIPTOR_ENDPOINT:
        case STD_GET_DESCRIPTOR:
            len = usb_setup->wLen;
            switch (usb_setup->wValueH) {
                case USB_DEVICE_DESCRIPTOR_TYPE:
                    len = sizeof (device_dsc);
                    ctl_send(&device_dsc, len);
                    break;
                case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                    if (len>sizeof (cfgDescriptor)) len = sizeof (cfgDescriptor);
                    ctl_send(&cfgDescriptor[0], len);
                    break;
                case USB_STRING_DESCRIPTOR_TYPE:
                    switch (usb_setup->wValueL) {
                        case 0:
                            if (len>sizeof (strLanguage)) len = sizeof (strLanguage);
                            ctl_send(&strLanguage[0], len);
                            break;
                        case 1:
                            if (len>sizeof (strManufacturer)) len = sizeof (strManufacturer);
                            ctl_send(&strManufacturer[0], len);
                            break;
                        case 2:
                            if (len>sizeof (strProduct)) len = sizeof (strProduct);
                            ctl_send(&strProduct[0], len);
                            break;
                        default:
                            ep0_stall();
//                            case 3:
//                            ctl_send(&strSerial[0], len);
//                            break;
                    }
                    break;
                case USB_INTERFACE_DESCRIPTOR_TYPE:
                    ctl_send(&cfgDescriptor[9], 0x9);
                    break;
                case USB_ENDPOINT_DESCRIPTOR_TYPE:
                    if (usb_setup->wValueL == 1) ctl_send(&cfgDescriptor[sizeof (cfgDescriptor) - 14], 0x7);
                    if (usb_setup->wValueL == 0x81) ctl_send(&cfgDescriptor[sizeof (cfgDescriptor) - 7], 0x7);
                    if (usb_setup->wValueL == 0x82) ctl_send(&cfgDescriptor[sizeof (cfgDescriptor) - 30], 0x7);
                    break;
//                case DSC_REPORT: ctl_send(&hid_report[0], sizeof(hid_report)); break;
//                case DSC_HID: ctl_send(&cfgDescriptor[18], sizeof(9)); break;
//                case USB_ENDPOINT_DESCRIPTOR_TYPE:
                default:
                    ctl_ack();
            }
            break;
        case STD_GET_CONFIGURATION:
            ctl_send(&current_cnf, 1);
            break;
        case STD_SET_CONFIGURATION:
            LED1_ON
            state = CONFIGURED;
            cdc_init_endpoints();
            ctl_ack();
            break;
        case STD_SET_ADDRESS:
            state = ADDRESS_PENDING;
            dev_addr = usb_setup->wValueL;
            ctl_ack();
            break;
        default:
            ep0_stall();
    }
}

static void SetupStage(USB_SETUP_t* usb_setup) {
    usbDisengageEP0();
    ctl_stage = SETUP;
    if ((usb_setup->bmRequestType >> 5 & 0x3) == 0) {
        process_standart_request(usb_setup);
    } else 
    if ((usb_setup->bmRequestType >> 5 & 0x3) == 1) {
       process_cdc_request(usb_setup);
    } else ep0_stall();
    UCONbits.PKTDIS = 0;
}

static void DataInStage() {
    if (wCount > 0) {
        uint8_t current_transfer_len = MIN(wCount, EP0_BUFF_SIZE);
        copyPacketToEp(0, ubuf, current_transfer_len);
        usbEngageEndpointIn(0, current_transfer_len);
        ubuf += current_transfer_len;
        wCount -= current_transfer_len;
    } else {
        if (control_needs_zlp) {
            usbEngageEndpointIn(0, 0);
            control_needs_zlp = 0;
        } else {
            usb_status_out();
            ctl_stage = _STATUS;
        }
    }
}

static uint8_t DataOutStage() {
    uint8_t current_transfer_len = endpoints[0].out.CNT;
    memcpy(ubuf, endpoints[0].out.ADR, current_transfer_len);
    ubuf += current_transfer_len;
    wCount -= current_transfer_len;
    if (wCount > 0) {
        usbEngageEndpointOut(0, EP0_BUFF_SIZE);
    } else {
        usb_status_in();
        ctl_stage = _STATUS;
    }
    return current_transfer_len;
}

static void WaitForSetupStage() {
    ctl_stage = WAIT_SETUP;
    control_needs_zlp = 0;
    usbEngageSetupEp0();
}

void USBStallHandler() {
    WaitForSetupStage();
}

void UnSuspend(void) {
    UCONbits.SUSPND=0;   // Bring USB module out of power conserve
    UIEbits.ACTVIE=0;
    UIR &= 0xFB;
}

void Suspend(void) {
    UIEbits.ACTVIE=1;    // Enable bus activity interrupt
    UIR &= 0xEF;
    UCONbits.SUSPND=1;   // Put USB module in power conserve
}

void usb_poll() {
    uint8_t PID = 0;
    uint8_t ep = 0;
//    if (UIRbits.ACTVIF) {
//        if (state == SUSPEND) {
//            state=CONFIGURED;
//            UnSuspend();
//        }
//        UIRbits.ACTVIF=0;
//    }
//    if (UIRbits.IDLEIF) {
//        if (state==CONFIGURED) {
//            state=SUSPEND;
//            Suspend();
//            UIRbits.IDLEIF=0;
//        }
//    }
    if (UIRbits.STALLIF && UIEbits.STALLIE) USBStallHandler();
    if (UIRbits.URSTIF && UIEbits.URSTIE) reset_usb();

    while (UIRbits.TRNIF) {
        ep = (uint8_t)(USTAT >> 3) & 0x7;
        PID = USTATbits.DIR ? endpoints[ep].in.STAT.PID : endpoints[ep].out.STAT.PID;
        switch (ep) {
            case 0:
            switch (PID) {
                case SETUP_PID:
                    SetupStage(usb_setup);
                    break;
                case OUT_PID:
                    if (ctl_stage == DATA_OUT) { 
                        DataOutStage();
                    } else {
                        if (ctl_stage == _STATUS) {
                            WaitForSetupStage();
                        }
                    }
                    break;
                case IN_PID:
                    if (state == ADDRESS_PENDING) {
                        state = ADDRESSED;
                        UADDR = dev_addr;
                    }
                    if (ctl_stage == DATA_IN) {
                        DataInStage(); 
                    } else {
                        if (ctl_stage == _STATUS) {
                            WaitForSetupStage();
                        }
                    }
                    break;
            }
            break;
            case 1:
                switch (PID) {
                    case OUT_PID: 
                        handle_cdc_out(endpoints[1].out.ADR, endpoints[1].out.CNT);
                        usbEngageEndpointOut(1, EP1_BUFF_SIZE);
                    break;
                    case IN_PID: 
                        handle_cdc_in();
                    break;
                }
            break;
            case 2:
                switch (PID) {
                    case IN_PID: 
                        usbEngageEndpointIn(2, 0);
                    break;
                }
            break;
        }
        TRNIF = 0;
    }
}      