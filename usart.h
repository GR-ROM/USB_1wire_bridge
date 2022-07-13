/* 
 * File:   usart.h
 * Author: User
 *
 * Created on 22 февраля 2019 г., 0:57
 */

#ifndef USART_H
#define	USART_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <xc.h>

#define __XTAL_FREQ 48000000

void init_usart(uint32_t baudrate);    
void usart_put_char(uint8_t c);
uint8_t usart_get_char();
uint16_t calc_baudrate(uint32_t baudrate, char *uBRGH);
void set_baudrate(uint16_t uspbrg, char ubrgh);   
void usart_purge_fifo();
void isr_usart_tx();

#ifdef	__cplusplus
}
#endif

#endif	/* USART_H */

