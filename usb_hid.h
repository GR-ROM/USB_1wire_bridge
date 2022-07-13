/* 
 * File:   usb_hid.h
 * Author: exp10d3r
 *
 * Created on February 18, 2017, 4:14 AM
 */

#ifndef USB_HID_H
#define	USB_HID_H

#ifdef	__cplusplus
extern "C" {
#endif

void init_HID_CLASS();
void process_hid_request(USB_SETUP_t* usb_setup);
void HID_txbuffer(char* buf, int len);


#endif	/* USB_HID_H */

