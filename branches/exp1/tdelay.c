///\file 
///\brief An attempt to make a usable delay routine
///
/// Regular util/delay.h is weird, it's defined as __always_inline__ and 
/// it seems to make the text segment overflow too soon.
///
/// This is a fairly poor attempt at a delay routine. It uses Timer2 to
/// measure intervals. Not tested at all.

#include <inttypes.h>

#include <stdio.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

void tdelay(uint16_t ms) {
    uint16_t i;
    
    if (ms == 0) return;
    
    ms = ms * 12;
    
    uint8_t remainder = ms % 256;
    if (remainder) {
        TCCR2 = 0;
        TCNT2 = 0;
        OCR2 = remainder;
        TIFR |= _BV(OCF2);
        
        // prescaler = 1024, 0.128ms per cycle
        for (TCCR2 = _BV(CS22)|_BV(CS21)|_BV(CS20); (TIFR & _BV(OCF2)) == 0;);
    }
    
    OCR2 = 0;
    TCNT2 = 0;
    for (i = ms/256; i > 0; i--) {
        TCCR2 = 0;
        TIFR |= _BV(TOV2);
        for (TCCR2 = _BV(CS22)|_BV(CS21)|_BV(CS20); (TIFR & _BV(TOV2)) == 0;);
    }
    
    TCCR2 = 0;
}

//$Id$
