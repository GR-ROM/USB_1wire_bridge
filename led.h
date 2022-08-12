/* 
 * File:   led.h
 * Author: exp10
 *
 * Created on July 9, 2022, 12:06 AM
 */

#ifndef LED_H
#define	LED_H

#ifdef	__cplusplus
extern "C" {
#endif

#define LED1_ON PORTCbits.RC0 = 1;
#define LED1_OFF PORTCbits.RC0 = 0;
#define LED1_TOGGLE PORTCbits.RC0 = ~PORTCbits.RC0;

#define LED2_ON PORTCbits.RC1 = 1;
#define LED2_OFF PORTCbits.RC1 = 0;
#define LED2_TOGGLE PORTCbits.RC1 = ~PORTCbits.RC1;

#define LED3_ON PORTCbits.RC2 = 1;
#define LED3_OFF PORTCbits.RC2 = 0;
#define LED3_TOGGLE PORTCbits.RC2 = ~PORTCbits.RC2;


#ifdef	__cplusplus
}
#endif

#endif	/* LED_H */

