
#include <xc.h>
#include <stdint.h>
#include "usbdesc.h"
#include "usb.h"

#define LED_ON PORTCbits.RC0=1;
#define LED_OFF PORTCbits.RC0=1;
#define LED_TOGGLE PORTCbits.RC0=~LATCbits.LATC0;

static uint8_t idle_rate=1;
static uint8_t active_protocol=0;



void init_HID_CLASS()
{
    active_protocol=1;
    idle_rate=250;
}

void process_hid_request(USB_SETUP_t* usb_setup)
{   
    if ((usb_setup->bRequest==0x07) && (usb_setup->bmRequestType==0x01))
    {
        send_to_host(EP0_OUT, 0, 0);
    }
    if ((usb_setup->wValueH==DSC_REPORT) && (usb_setup->bRequest==0x06))
    {   
        ctl_send(&hid_report[0], sizeof(hid_report));
    }
    if ((usb_setup->wValueH==DSC_HID) && (usb_setup->bRequest==0x06))
    {
        ctl_send(&cfgDescriptor[9], sizeof(9));
    }
    if (((usb_setup->bmRequestType >> 5) & 0x03)==1)
    {   
        
        if (usb_setup->bRequest==GET_REPORT)
        {   
            if (usb_setup->wValueH==1)
            {
                send_to_host(EP0_OUT, 0, 0);
            }
            if (usb_setup->wValueH==2)
            {
                send_to_host(EP0_OUT, 0, 0);
            }
            if (usb_setup->wValueH==3)
            {
                send_to_host(EP0_OUT, 0, 0);
            }
        }
        if (usb_setup->bRequest==GET_IDLE)
        {
            ctl_send(&idle_rate, 1);
        }
        if (usb_setup->bRequest==GET_PROTOCOL)
        {
            ctl_send(&active_protocol, 1);
        }  
        if (usb_setup->bRequest==SET_REPORT)
        {
            send_to_host(EP0_OUT, 0, 0);
        }          
        if (usb_setup->bRequest==SET_IDLE)
        {
            ctl_send(0, 0);
            idle_rate=usb_setup->wValueH;
        }      
        if (usb_setup->bRequest==SET_PROTOCOL)
        {
            ctl_send(0, 0);
        }   
    }
}


