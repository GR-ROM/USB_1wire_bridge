#include "usb.h"
#include "usbdesc.h"
#include <xc.h>
#include <string.h>
#include "usb_cdc.h"
#include "led.h"

#define FULL_SPEED

#define BD_BASE_ADDR 0x200
#define BD_DATA_ADDR 0x280

volatile BD_entry_t endpoints[EP_NUM_MAX] __at(BD_BASE_ADDR);
volatile uint8_t ep_data_buffer[160] __at(BD_DATA_ADDR);
USB_SETUP_t usb_setup;

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
static void DataInStage(bool firstPacket);
static uint8_t DataOutStage();
static void WaitForSetupStage(void);

static uint16_t pp;
static uint16_t dts;
int lastEpIndex;

static void process_standart_request(USB_SETUP_t *usb_setup);

uint8_t getDts(uint8_t ep) {
    return (dts & (1 << ep)) >> ep;
}

void setDts(uint8_t ep) {
    dts |= 1 << ep;
}

void clrDts(uint8_t ep) {
    dts &= ~(1 << ep);
}

void advanceDts(uint8_t ep) {
    dts ^= 1 << ep;    
}

void resetAllDts() {
    dts = 0;    
}

//uint8_t getPp(uint8_t ep) {
//    return (pp & (1 << ep)) >> ep;
//}

void advancePp(uint8_t ep) {
    //pp ^= 1 << ep;    
}

void resetAllPp() {
    UCONbits.PPBRST = 1;
    pp = 0;
    UCONbits.PPBRST = 0;
}

void usbEngageEp(uint8_t ep, uint8_t len, uint8_t flags) {
    uint8_t epIndex = ep; // (ep << 1) | getPp(ep);
    endpoints[epIndex].CNT = len;
    endpoints[epIndex].STAT.Val = 0;
    if (flags & SYNC_AUTO) {
        endpoints[epIndex].STAT.DTS = getDts(ep);
    } else {
        if (flags & SYNC_FORCE_DAT1) {
            endpoints[epIndex].STAT.DTS = 1;
            setDts(ep);
        } else {
            clrDts(ep);
        }
    }
    advanceDts(ep);
    advancePp(ep);        
    endpoints[epIndex].STAT.Val |= _USIE | _DTSEN;
}

void usbEngageSetupEp0() {
    usbEngageEp(EP0_OUT, EP0_BUFF_SIZE, SYNC_FORCE_DAT0);
    usbDisengageEp(EP0_IN);
}

void usbDisengageEp(uint8_t ep) {
    endpoints[ep].STAT.UOWN = 0;
   // endpoints[(ep << 1) | 1].STAT.UOWN = 0;
}

void usbCopyPacketToEp(uint8_t ep, uint8_t *buf, uint8_t len) {
    if (buf != 0 && len > 0) {
       // memcpy(endpoints[(ep << 1) | getPp(ep)].ADR, buf, len);
         memcpy(endpoints[ep].ADR, buf, len);
    }
}

uint8_t usbCopyPacketFromEp(uint8_t ep, uint8_t *buf) {
    uint8_t epIndex = ep; //(ep << 1) | (getPp(ep) ^ 1);
    uint8_t len = endpoints[epIndex].CNT;
    if (buf != 0 && len > 0) {
        memcpy(buf, endpoints[epIndex].ADR, len);
    }
    return len;
}

static void usb_status_out() {
    usbEngageEp(EP0_OUT, 0, SYNC_FORCE_DAT1);
}

static void usb_status_in() {
    usbEngageEp(EP0_IN, 0, SYNC_FORCE_DAT1);
}

void ep0_stall() {
    uint8_t epIndex = EP0_IN; // (EP0_IN << 1) | getPp(EP0_IN);
    endpoints[epIndex].STAT.Val = _BSTALL | _USIE;

    epIndex = EP0_OUT; //(EP0_OUT << 1) | getPp(EP0_OUT);
    endpoints[epIndex].STAT.Val = _BSTALL | _USIE;
}

uint8_t* getEpBuff(int ep) {
    //return endpoints[(ep << 1) | getPp(ep)].ADR;
    return endpoints[ep].ADR;
}

static void reset_usb() {
    UADDR = 0;
    UIR = 0x00;
    UIE = 0x00;
    
#ifdef NO_PING_PONG    
    UCFGbits.PPB0=0;
    UCFGbits.PPB1=0;
#endif
#ifdef PING_PONG    
    UCFGbits.PPB0 = 0;
    UCFGbits.PPB1 = 0;
#endif  
    UCFGbits.UPUEN=1;
#ifdef LOW_SPEED
    UCFGbits.FSEN=0;
#endif
#ifdef FULL_SPEED
    UCFGbits.FSEN=1;
#endif
    UCONbits.SUSPND=0;
    UCONbits.RESUME=0; 
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
//    configureEp(EP0_OUT_EVEN, &ep_data_buffer[EP0_OUT_EVEN_OFFSET], EP0_BUFF_SIZE);
//    configureEp(EP0_OUT_ODD, &ep_data_buffer[EP0_OUT_ODD_OFFSET], EP0_BUFF_SIZE);
//    configureEp(EP0_IN_EVEN, &ep_data_buffer[EP0_IN_EVEN_OFFSET], EP0_BUFF_SIZE);
//    configureEp(EP0_IN_ODD, &ep_data_buffer[EP0_IN_ODD_OFFSET], EP0_BUFF_SIZE);
    configureEp(EP0_OUT, &ep_data_buffer[EP0_OUT], EP0_BUFF_SIZE);
    configureEp(EP0_IN, &ep_data_buffer[EP0_IN], EP0_BUFF_SIZE);
    UEP0 = EP_IN | EP_OUT | EP_HSHK;
    state = ATTACHED;
    
    resetAllPp();
    resetAllDts();
    // Prepare to receive first SETUP packet
    WaitForSetupStage();
}

void init_usb() {
    dev_addr = 0;
    reset_usb();
}

void configureEp(uint8_t ep, uint8_t* buf, uint8_t len) {
    endpoints[ep].ADR = buf;
    endpoints[ep].CNT = len;
    endpoints[ep].STAT.Val = 0x00;
}

void ctl_send(uint8_t* data, uint16_t len) {
    control_needs_zlp = ((len > EP0_BUFF_SIZE) && (len % EP0_BUFF_SIZE == 0)) ? 1 : 0; 
    wCount = len;
    ubuf = data;
    ctl_stage = DATA_IN;
    DataInStage(true);
}

void ctl_recv(char* data, uint16_t len) {
    wCount = len;
    ubuf = data;
    ctl_stage = DATA_OUT;
    usbEngageEp(EP0_OUT, EP1_BUFF_SIZE, SYNC_FORCE_DAT1);
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
                    ctl_send(&device_dsc, len > sizeof(device_dsc)? sizeof(device_dsc) : len);
                    break;
                case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                    ctl_send(&cfgDescriptor[0], len > sizeof(cfgDescriptor) ? sizeof(cfgDescriptor) : len);
                    break;
                case USB_STRING_DESCRIPTOR_TYPE:
                    switch (usb_setup->wValueL) {
                        case 0:
                            ctl_send(&strLanguage[0], len > sizeof(strLanguage) ? sizeof(strLanguage) : len);
                            break;
                        case 1:
                            ctl_send(&strManufacturer[0], len > sizeof(strManufacturer) ? sizeof(strManufacturer) : len);
                            break;
                        case 2:
                            ctl_send(&strProduct[0], len > sizeof(strProduct) ? sizeof(strProduct) : len);
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
    usbDisengageEp(EP0_OUT);
    usbDisengageEp(EP0_IN);
    ctl_stage = SETUP;
    if ((usb_setup->bmRequestType >> 5 & 0x3) == 0) {
        process_standart_request(usb_setup);
    } else 
    if ((usb_setup->bmRequestType >> 5 & 0x3) == 1) {
       process_cdc_request(usb_setup);
    } else ep0_stall();
    UCONbits.PKTDIS = 0;
}

static void DataInStage(bool firstPacket) {
    if (wCount > 0) {
        uint8_t current_transfer_len = MIN(wCount, EP0_BUFF_SIZE);
        usbCopyPacketToEp(EP0_IN, ubuf, current_transfer_len);
        usbEngageEp(EP0_IN, current_transfer_len, firstPacket ? SYNC_FORCE_DAT1 : SYNC_AUTO);
        ubuf += current_transfer_len;
        wCount -= current_transfer_len;
    } else {
        if (control_needs_zlp) {
            usbEngageEp(EP0_IN, 0, SYNC_AUTO);
            control_needs_zlp = 0;
        } else {
            usb_status_out();
            ctl_stage = _STATUS;
        }
    }
}

static uint8_t DataOutStage() {
    uint8_t current_transfer_len = usbCopyPacketFromEp(EP0_OUT, ubuf);
    ubuf += current_transfer_len;
    wCount -= current_transfer_len;
    if (wCount > 0) {
        usbEngageEp(EP0_OUT, EP0_BUFF_SIZE, SYNC_AUTO); 
    } else {
        usb_status_in();
        ctl_stage = _STATUS;
    }
    return current_transfer_len;
}

static void WaitForSetupStage() {
    ctl_stage = WAIT_SETUP;
    control_needs_zlp = 0;
    resetAllPp();
    usbEngageSetupEp0();
}

void USBStallHandler() {
    WaitForSetupStage();
}

void UnSuspend(void) {
    UCONbits.SUSPND=0;   
    UIEbits.ACTVIE=0;
    UIR &= 0xFB;
}

void Suspend(void) {
    UIEbits.ACTVIE=1;    
    UIR &= 0xEF;
    UCONbits.SUSPND=1;
}

void usb_poll() {
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
        uint8_t ep = (USTAT >> 3) & 0x7;
        lastEpIndex = (USTAT >> 2) & 0x1F;
        uint8_t PID = endpoints[lastEpIndex].STAT.PID;
        switch (ep) {
            case 0:
                switch (PID) {
                    case SETUP_PID:
                        SetupStage(endpoints[EP0_OUT].ADR);
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
                        if (ctl_stage == DATA_IN) {
                            DataInStage(false); 
                        } else {
                            if (ctl_stage == _STATUS) {
                                if (state == ADDRESS_PENDING) {
                                    state = ADDRESSED;
                                    UADDR = dev_addr;
                                }
                                WaitForSetupStage();
                            }
                        }
                    break;
            }
            break;
            case 1:
                switch (PID) {
                    case OUT_PID: 
                        handle_cdc_out(endpoints[EP1_OUT].ADR, endpoints[EP1_OUT].CNT);
                    break;
                    case IN_PID: 
                        handle_cdc_in();
                    break;
                }
            break;
            case 2:
                switch (PID) {
                    case IN_PID: 
                        usbEngageEp(EP2_IN, 0, SYNC_AUTO);
                    break;
                }
            break;
        }
        TRNIF = 0;
    }
}      