//! \file
//! \brief C1350 and C1351 implementation
//!
//! Everything that emulates C1351 (proportional) and C1350 (Joystick) mice.
//! Every movement of a real mouse is signalled with potmouse_movt(). 
//! In proportional (analog) mode, INT1 interrupt senses SID measurement cycle start
//! and loads timer OCR1A/OCR1B values with accordance to reported counter values.
//!
//! In Joystick mode, pulses are generated on UP/DOWN/LEFT/RIGHT joystick lines
//! every time a movement is reported.

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdio.h>

#include "ioconfig.h"
#include "c1351.h"
#include "ps2.h"

static uint8_t potmouse_xcounter;           ///< x axis counter
static uint8_t potmouse_ycounter;           ///< y axis counter

static volatile uint16_t ocr1a_load;        ///< precalculated OCR1A value (YPOT)
static volatile uint16_t ocr1b_load;        ///< precalculated OCR1B value (XPOT)

static volatile uint16_t ocr_zero;          ///< zero point (320us)

static volatile uint8_t mode;               ///< mouse mode

void potmouse_init() {
    // Joystick outputs, all to Z and no pullup
    JOYPORT &= ~(_BV(JOYFIRE) | _BV(JOYUP) | _BV(JOYDOWN) | _BV(JOYLEFT) | _BV(JOYRIGHT)); 
    JOYDDR  &= ~(_BV(JOYFIRE) | _BV(JOYUP) | _BV(JOYDOWN) | _BV(JOYLEFT) | _BV(JOYRIGHT));
    
    // SID sensing port
    SENSEDDR  &= ~_BV(POTSENSE); // SENSE is input
    SENSEPORT &= ~_BV(POTSENSE); // pullup off, hi-biased by OC1B

    // SID POTX/POTY port
    POTPORT &= ~(_BV(POTX) | _BV(POTY));
    POTDDR  &= ~(_BV(POTX) | _BV(POTY));

    // prepare INT1
    GICR &= ~_BV(INT1);                     // disable INT1
    MCUCR &= ~(_BV(ISC11)|_BV(ISC10));  
    MCUCR |= _BV(ISC11);                    // ISC11:ISC10 == 10, @negedge   
    
    mode = POTMOUSE_C1351;
}

void potmouse_start(uint8_t m) {
    mode = m;
    switch (mode) {
        case POTMOUSE_C1351:
            // Initialize Timer1 and use OC1A/OC1B to output values
            // don't count yet    
            TCCR1B = 0; 
            
            // POTX/Y normally controlled by output compare unit
            // initially should be pulled up to provide high bias on SENSE pin
            POTDDR  |= _BV(POTX) | _BV(POTY);   // enable POTX/POTY as outputs
            POTPORT |= _BV(POTX) | _BV(POTY);   // output "1" on both
            
            GIFR |= _BV(INTF1);                     // clear INT1 flag
            GICR |= _BV(INT1);                      // enable INT1
            break;
        case POTMOUSE_JOYSTICK:
            // Joystick emulation
            // close directional pins for ~20ms while there is movement
            TCCR1B = 0;
            TCCR1A = 0;
            
            break;
    }
}

void potmouse_movt(int16_t dx, int16_t dy, uint8_t button) {
    uint16_t a, b;
    
    switch (mode) {
        case POTMOUSE_C1351:
            potmouse_xcounter = (potmouse_xcounter + dx) & 077; // modulo 64
            potmouse_ycounter = (potmouse_ycounter + dy) & 077;
        
            (button & 001) ? (JOYDDR |= _BV(JOYFIRE)) : (JOYDDR &= ~_BV(JOYFIRE));
            (button & 002) ? (JOYDDR |= _BV(JOYUP))   : (JOYDDR &= ~_BV(JOYUP));
            (button & 004) ? (JOYDDR |= _BV(JOYDOWN)) : (JOYDDR &= ~_BV(JOYDOWN));
            
            // scale should be 2x here, but for this particular chip, 66 counts work better where
            // 64 counds should be. so 66/64=100/96 and times two
            a = ocr_zero + potmouse_ycounter*200/96;
            b = ocr_zero + potmouse_xcounter*200/96;
            
            ocr1a_load = a;
            ocr1b_load = b;
            break;
        case POTMOUSE_JOYSTICK:
            JOYDDR  &= ~(_BV(JOYFIRE) | _BV(JOYUP) | _BV(JOYDOWN) | _BV(JOYLEFT) | _BV(JOYRIGHT));

            (dx < 0) ? (JOYDDR |= _BV(JOYLEFT)) : (JOYDDR &= ~_BV(JOYLEFT));
            (dx > 0) ? (JOYDDR |= _BV(JOYRIGHT)): (JOYDDR &= ~_BV(JOYRIGHT));
            (dy < 0) ? (JOYDDR |= _BV(JOYDOWN)) : (JOYDDR &= ~_BV(JOYDOWN));
            (dy > 0) ? (JOYDDR |= _BV(JOYUP))   : (JOYDDR &= ~_BV(JOYUP));
            (button & 001) ? (JOYDDR |= _BV(JOYFIRE)) : (JOYDDR &= ~_BV(JOYFIRE));
            (button & 002) ? (POTDDR |= _BV(POTX)) : (POTDDR &= ~_BV(POTX));
  
            TCNT1 = 65535-256;
            TCCR1A = 0;
            TCCR1B = _BV(CS12)|_BV(CS10);
            TIFR |= _BV(TOV1);
            TIMSK |= _BV(TOIE1);
            break;
    }
}

void potmouse_zero(uint16_t zero) {
    ocr_zero = zero;
}

/// SID measuring cycle detected.
///
/// 1. SID pulls POTX low\n
/// 2. SID waits 256 cycles us\n
/// 3. SID releases POTX\n
/// 4. 0 to 255 cycles until the cap is charged\n
///
/// This handler stops the Timer1, clears OC1A/OC1B outputs,
/// loads the timer with values precalculated in potmouse_movt()
/// and starts the timer. 
///
/// OC1A/OC1B (YPOT/XPOT) lines will go up by hardware. 
/// Normal SID cycle is 512us. Timer will overflow not before 65535us.
/// Next cycle will begin before that so there's no need to stop the timer.
/// Output compare match interrupts are thus not used.

ISR(INT1_vect) {
    // SID started to measure the pots, uuu

    // disable INT1 until the measurement cycle is complete
    // stop the timer
    TCCR1B = 0;
    
    // clear OC1A/OC1B:
    // 1. set output compare to clear OC1A/OC1B ("10" in table 37 on page 97)
    TCCR1A = _BV(COM1A1) | _BV(COM1B1);
    // 2. force output compare to make it happen
    TCCR1A |= _BV(FOC1A) | _BV(FOC1B);

    // Set OC1A/OC1B on Compare Match (Set output to high level) 
    // WGM13:0 = 00, normal mode: count from BOTTOM to MAX
    TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(COM1B1) | _BV(COM1B0);

    // load the timer 
    TCNT1 = 0;
    
    // init the output compare values 
    OCR1A = ocr1a_load;
    OCR1B = ocr1b_load;
    
    // start timer with prescaler clk/8 (1 count = 1us)
    TCCR1B = _BV(CS11);  
}

/// TIMER1 Overflow vector
///
/// Ends joystick emulator pulse.
ISR(TIMER1_OVF_vect) {
    JOYDDR  &= ~(_BV(JOYFIRE) | _BV(JOYUP) | _BV(JOYDOWN) | _BV(JOYLEFT) | _BV(JOYRIGHT));
    POTDDR  &= ~_BV(POTX);
    TIMSK &= ~_BV(TOIE1);
}

//$Id$
