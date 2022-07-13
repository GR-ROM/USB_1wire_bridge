/*
 * File:   main.c
 * Author: Grinev R.
 *
 * Created on Jul 12, 2018, 10:27 PM
 */
#define _XTAL_FREQ 48000000UL

#include <xc.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "usb_cdc.h"
#include "owire.h"
#include "usb.h"
#include "led.h"

// PIC18F14K50 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1L
#pragma config CPUDIV = NOCLKDIV// CPU System Clock Selection bits (No CPU System Clock divide)
#pragma config USBDIV = OFF     // USB Clock Selection bit (USB clock comes directly from the OSC1/OSC2 oscillator block; no divide)

// CONFIG1H
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config PLLEN = ON       // 4 X PLL Enable bit (Oscillator multiplied by 4)
#pragma config PCLKEN = ON      // Primary Clock Enable bit (Primary clock enabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRTEN = ON      // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware only (SBOREN is disabled))
#pragma config BORV = 30        // Brown-out Reset Voltage bits (VBOR set to 3.0 V nominal)

// CONFIG2H
#pragma config WDTEN = OFF      // Watchdog Timer Enable bit (WDT is controlled by SWDTEN bit of the WDTCON register)
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config HFOFST = OFF     // HFINTOSC Fast Start-up bit (The system clock is held off until the HFINTOSC is stable.)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RA3 input pin disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
#pragma config BBSIZ = OFF      // Boot Block Size Select bit (1kW boot block size)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Table Write Protection bit (Block 0 not write-protected)
#pragma config WRT1 = OFF       // Table Write Protection bit (Block 1 not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block not protected from table reads executed in other blocks)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define MAX_COUNTS 48

char ROM_NO[8][8];
unsigned char devNum = 0;

void putch(unsigned char byte) {
    send_cdc_buf(&byte, 1);
}

void int2str(int d, char* s, uint8_t lim, uint8_t min){
    unsigned char l, i, c;
    int t;
    t=d;
    c=0;
    if (d!=0){
        while(t!=0){
            t/=10;
            c++;
        }
    } else c=1;
    if (c<min) c=min;
    if (c>lim) c=lim;
    if (d<0){
        *(s-c)='-';
        d=abs(d);
    } else s--; // *(s-c)='+';
        
    for(i = 0; i != c; i++){
        l=(d % 10) + 0x30;
        d/=10;
        *s--=l;
    }
}

int ds18b20_convert(int temp){
    int fract;
    fract = temp & 0x000F;
    temp &= 0xFFF0;
    temp = temp >> 4;
    if (temp & 0x8000) temp |= 0xF000;
    fract *= 6; // LSB weight is 0.06 == rounded 0.0625
    temp *= 100;
    temp += fract;
    temp /= 10;
  //  fract=temp*0.625;
   // fract=temp*625;
   // fract=fract/1000;
    return (int)temp;    
}

void __interrupt isr(void){
    if (USBIF) {
        usb_poll(); 
        USBIF = 0;
    }
}

void usb_cdc_callback(uint8_t* buf, uint8_t len){
    if (buf[0] == 'l'){
        if (devNum > 0) {
        for (int i = 0; i != devNum; i++) {
            printf("%i %x:%x:%x:%x:%x:%x:%x:%x\r\n",
                    i,
                    (int)ROM_NO[0][i], 
                    (int)ROM_NO[1][i], 
                    (int)ROM_NO[2][i],
                    (int)ROM_NO[3][i],
                    (int)ROM_NO[4][i],
                    (int)ROM_NO[5][i],
                    (int)ROM_NO[6][i],
                    (int)ROM_NO[7][i]);
        }
        } else {
            printf("No sensors detected!");
        }
    }
    if (buf[0] == 'a') {
        printf("Designed by Roman Grinev JUL/2018\r\n");
    }
}

void main(void) {
    uint32_t i = 0;
    int dtemp = 0;
    int flag_no_sensor = 0;
    int itemp = 0;
    TRISCbits.RC0 = 0;
    TRISCbits.RC1 = 0;
    TRISCbits.RC2 = 0;
    ANSEL = 0;
    ANSELH = 0;
    CM1CON0 = 0;
    CM2CON0 = 0;
    
    init_usb();
    init_cdc();
    GIE = 1;
    PEIE = 1;
    
    init_1wire();
    devNum = OW_search(ROM_NO);
    set_mode();
    LED1_OFF
    LED2_OFF
    LED3_OFF
    while(1) {
        i++;
        if (i == 2000000){
            LED2_ON
            if (get_temp(&dtemp) == 0) {
                flag_no_sensor = 0;
                //dtemp = 0x019A;
                itemp = ds18b20_convert(dtemp);
                 //  val=val*(1-K)+itemp*K;
                 //  itemp=(int)val;
                int f = itemp % 10;
                itemp = itemp/10;
                printf("temp %i.%i C\r\n", itemp, f);
            } 
            else{
                flag_no_sensor=1;
                printf("No sensor detected!");
            }
            i = 0;
            LED2_OFF
        }
    }
}