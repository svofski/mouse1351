///\file ioconfig.h
///\brief I/O port definitions

#ifndef _IOCONFIG_H
#define _IOCONFIG_H

#include <avr/io.h>

#define PS2PORT PORTD           ///< PS2 port
#define PS2PIN  PIND            ///< PS2 input
#define PS2DDR  DDRD            ///< PS2 data direction
#define PS2CLK  2               ///< PS2CLK is pin 2
#define PS2DAT  4               ///< PS2DAT is pin 4

#define PS2_RXBUF_LEN  16       ///< PS2 receive buffer size


#define SENSEPORT   PORTD       ///< SID sense port
#define SENSEDDR    DDRD        ///< SID sense data direction
#define SENSEPIN    PIND        ///< SID sense input
#define POTSENSE    3           ///< INT1 attached to PORTD.3,

#define POTPORT     PORTB       ///< POT-controlling outputs X and Y
#define POTDDR      DDRB        ///< POT outputs data direction
#define POTPIN      PINB        ///< POT outputs input ;)
#define POTY        1           ///< Y-line, also OC1A
#define POTX        2           ///< X-line, also OC1B, also right button in joystick mode

#define JOYPORT     PORTC       ///< Joystick pins
#define JOYDDR      DDRC        ///< Joystick pins data direction (out = switch closed to gnd)
#define JOYPIN      PINC        ///< Joystick in
#define JOYUP       0           ///< Joystick UP switch
#define JOYDOWN     2           ///< Joystick DOWN switch
#define JOYLEFT     3           ///< Joystick LEFT switch
#define JOYRIGHT    4           ///< Joystick RIGHT switch
#define JOYFIRE     1           ///< Joystick FIRE switch

void io_init();

#endif

//$Id$
