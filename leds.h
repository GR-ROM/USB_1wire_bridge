/* 
 * File:   leds.h
 * Author: grinev
 *
 * Created on 8 ???? 2018 ?., 11:27
 */

#ifndef LEDS_H
#define	LEDS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <xc.h>    
    
#define LED_ON RC0=0;
#define LED_OFF RC0=1;

    
    void init_leds();

#ifdef	__cplusplus
}
#endif

#endif	/* LEDS_H */

