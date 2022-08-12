#include "owire.h"
#include <xc.h>
#include "usart.h"
#include "usb_cdc.h"

#define USE_USART

#define _XTAL_FREQ 48000000

#ifdef USE_BITBANG
#define OW_HIGH TRISB7=1;
#define OW_LOW {TRISB7=0;RB7=0;}
#define OW_READ PORTBbits.RB7

#endif

char ubrghlow, ubrghhigh;
uint16_t uspbrglow, uspbrghigh;

void init_1wire() {
    uspbrglow = calc_baudrate(9600, &ubrghlow);
    uspbrghigh = calc_baudrate(115200, &ubrghhigh);
    init_usart(9600);
}

uint8_t OW_reset() {
    uint8_t i;
#if defined(USE_BITBANG)
    OW_HIGH
    __delay_us(5);
    OW_LOW
    __delay_us(400);
    OW_HIGH
    __delay_us(15);
    i=0;
    while(PORTBbits.RB7!=0) {
        __delay_us(10);
        i++;
        if (i==50) return 0;
    }
    while(PORTBbits.RB7==0) {
        __delay_us(10);
        i++;
        if (i==50) return 0;
    }
    __delay_us(410);
#endif
#if defined(USE_USART)
    set_baudrate(uspbrglow, ubrghlow);
    usart_put_char(0xF0);
    i = usart_get_char();
    if (i == 0xF0) return 0;
    return 1;
#endif
    
}

void OW_write_bit(uint8_t b) {
#ifdef USE_BITBANG
    OW_LOW
    __delay_us(2);
    if (b) OW_HIGH
    __delay_us(45);
    OW_HIGH
#elif defined(USE_USART)
    if (b) usart_put_char(0xFF); else usart_put_char(0x00);
    usart_get_char();
#endif
}

uint8_t OW_read_bit() {
#ifdef USE_BITBANG
    OW_LOW
    __delay_us(1);
    OW_HIGH
    __delay_us(4);
    if (OW_READ) return 1; else return 0;
    __delay_us(5);
#endif
#ifdef USE_USART
    usart_put_char(0xFF); 
    if (usart_get_char()==0xFF) return 1;
    return 0;
#endif  
}

void OW_write_byte(uint8_t b) {
    uint8_t i;
    for (i = 0; i != 8; i++){
        OW_write_bit(b & 0x01);
        b >>= 1;
    }
}

uint8_t OW_read_byte() {
    uint8_t i, b;
    b = 0;
    for (i = 0; i != 8; i++){   
        b>>=1;
        if (OW_read_bit()) b|=0x80;
    }
    return b;
}

char OW_search(uint8_t ROM_NO[8][8]) {
    uint8_t devnum, bitnum, lastcoll, curcoll, fbit, cbit;
    lastcoll = 0;
    for (devnum = 0; devnum != 8; devnum++){
        curcoll = 0;
        if (!OW_reset()) return 0;
        set_baudrate(uspbrghigh, ubrghhigh);
        OW_write_byte(0xF0);
        for (bitnum = 0; bitnum != 64; bitnum++){
            fbit = OW_read_bit();
            cbit = OW_read_bit();
            if (fbit == cbit){ // collision detected
                if (fbit == 1) return 0;
                curcoll = bitnum;
                if (lastcoll > bitnum) {
                    if (ROM_NO[bitnum>>3][devnum-1] & 1<<(bitnum & 0x07)) fbit=1; else fbit=0;
                }
                if (lastcoll < bitnum) fbit=0;
                if (lastcoll == bitnum) fbit=1;
                lastcoll = curcoll;
            }
            OW_write_bit(fbit);
            if (fbit) ROM_NO[bitnum >> 3][devnum] |= 1 << (bitnum & 0x07); else ROM_NO[bitnum >> 3][devnum] &= ~(1 << (bitnum & 0x07));
        }
        if (curcoll == 0) return devnum + 1;
    }
    return devnum + 1;
}

char set_mode() {
    if (OW_reset()) {
        set_baudrate(uspbrghigh, ubrghhigh);
        OW_write_byte(0xCC);
        OW_write_byte(0x4E);
        
        OW_write_byte(0xFF);
        OW_write_byte(0xFF);
        OW_write_byte(0x7F);
    }
}

char get_temp_by_ROM(uint8_t ROM_NO[8][8], int* temp, int index) {
    uint8_t tl, th;
    if (OW_reset()) {
        set_baudrate(uspbrghigh, ubrghhigh);
        OW_write_byte(0x55);
        OW_write_byte(ROM_NO[0][index]);
        OW_write_byte(ROM_NO[1][index]);
        OW_write_byte(ROM_NO[2][index]);
        OW_write_byte(ROM_NO[3][index]);
        
        OW_write_byte(ROM_NO[4][index]);
        OW_write_byte(ROM_NO[5][index]);
        OW_write_byte(ROM_NO[6][index]);
        OW_write_byte(ROM_NO[7][index]);
        OW_write_byte(0xBE);
        tl = OW_read_byte();
        th = OW_read_byte();
        OW_read_byte();
        OW_read_byte();
        OW_read_byte();
        OW_read_byte();
        OW_read_byte();
        OW_read_byte();
        OW_reset();
        set_baudrate(uspbrghigh, ubrghhigh);
        OW_write_byte(0xCC);
        OW_write_byte(0x44);
        
        *temp=(th<<8) | tl;
        return 0;
    }
    return -1;
}

//char get_temp(int* temp) {
//    uint8_t tl, th;
//    if (OW_reset()) {
//        set_baudrate(uspbrghigh, ubrghhigh);
//        OW_write_byte(0xCC);
//        OW_write_byte(0xBE);
//        tl = OW_read_byte();
//        th = OW_read_byte();
//        OW_read_byte();
//        OW_read_byte();
//        OW_read_byte();
//        OW_read_byte();
//        OW_read_byte();
//        OW_read_byte();
//        OW_reset();
//        set_baudrate(uspbrghigh, ubrghhigh);
//        OW_write_byte(0xCC);
//        OW_write_byte(0x44);
//        *temp = (th << 8) | tl;
//        return 0;
//    }
//    return -1;
//}