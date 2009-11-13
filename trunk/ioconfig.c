#include "ioconfig.h"

void io_init() {
    // configure PS2 lines as inputs by default, internal pullups off
    PS2PORT &= ~(_BV(PS2CLK)|_BV(PS2DAT));  
    PS2DDR &= ~(_BV(PS2CLK)|_BV(PS2DAT));    
}

//$Id$
