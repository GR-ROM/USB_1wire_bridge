/* 
 * File:   owire.h
 * Author: exp10der
 *
 * Created on March 1, 2018, 3:02 PM
 */

#ifndef OWIRE_H
#define	OWIRE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

void init_1wire();
char get_temp(int* temp);
char set_mode();
char get_temp_by_ROM(uint8_t ROM_NO[8][8], int* temp, int index);
char OW_search(uint8_t ROM_NO[8][2]);

#ifdef	__cplusplus
}
#endif

#endif	/* OWIRE_H */

