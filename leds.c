#include <xc.h>
#include "leds.h"

void init_leds() {
    TRISC0 = 0;
    LED_OFF
}
