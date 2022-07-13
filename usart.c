#include "usart.h"
#include <math.h>
#include <stdlib.h>

void set_baudrate(uint16_t uspbrg, char ubrgh){
    SPBRG=uspbrg;
    if (ubrgh & 0x1) BRGH=1; else BRGH=0;
    if (ubrgh & 0x2) BRG16=1; else BRG16=0;
}

uint16_t calc_baudrate(uint32_t baudrate, char *uBRGH){
    float brg64=0;
    float brg16=0;
    float brg4=0;
    uint16_t delta1;
    uint16_t delta2;
    uint16_t delta3;
    uint16_t ibrg=0;
    brg64=(__XTAL_FREQ/(baudrate*64.0f))-1.0f;
    brg16=(__XTAL_FREQ/(baudrate*16.0f))-1.0f;
    brg4=(__XTAL_FREQ/(baudrate*4.0f))-1.0f;
    ibrg=(int)brg64;
    delta1=abs(baudrate-(__XTAL_FREQ/(64*(ibrg+1))));
    ibrg=(int)brg16;
    delta2=abs(baudrate-(__XTAL_FREQ/(16*(ibrg+1))));
    ibrg=(int)brg4;
    delta3=abs(baudrate-(__XTAL_FREQ/(4*(ibrg+1))));
    if (brg16>255.0) delta1=0;
    if (brg4>255.0) delta2=0;
    if (delta1<=delta2 && delta1<=delta3) {
        ibrg=brg64;
        *uBRGH=0;
    } 
    else
    {
        if (delta2<=delta3){ 
            ibrg=brg16;
            *uBRGH=1;
        }
        else {
            ibrg=brg4;
            *uBRGH=3;
        }
    }
    return ibrg;
}

void init_usart(uint32_t baudrate) {
    char ubrgh=0;
    TRISBbits.TRISB7 = 1; // TX
    TRISBbits.TRISB5 = 1; // RX
    SPBRG=calc_baudrate(baudrate, &ubrgh);
    if (ubrgh & 0x1) BRGH=1; else BRGH=0; 
    if (ubrgh & 0x2) BRG16=1; else BRG16=0;
    
    SYNC=0;
    TXEN=1;
    SPEN=1;
    CREN=1;
    // USART interrupts configuration
    RCONbits.IPEN = 1; // ENABLE interrupt priority
    INTCONbits.GIE = 1; // ENABLE interrupts
    INTCONbits.PEIE = 1; // ENable peripheral interrupts.
    PIE1bits.RCIE = 0; // ENABLE USART receive interrupt
    PIE1bits.TXIE = 0; // disable USART TX interrupt
    int f=RCREG;
    f=RCREG; // clear fifo
    f=RCREG;
}

void usart_put_char(char d) {
    while (!TRMT);
    TXREG=d;
}

uint8_t usart_get_char() {
    char f, l;
    while (!RCIF);
    if (OERR){  // if there is overrun error
        CREN = 0;
        CREN = 1;
    }
    f=RCREG;
    return f;
}