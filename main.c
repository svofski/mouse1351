///\file main.c
///\author Viacheslav Slavinsky
///
///\brief [M]ouse main file.
///
/// This is the main source file. The main loop inits usart, then ps2 functions, then c1351.
/// It then calls mouse_boot() which never exits until mouse is succesfully reset and 
/// initialized in streaming mode. Initial button status is reported and according to buttons
/// pressed at start, options are set. The default is to boot into C1351 proportional mode,
/// normal speed (2 counts per mm).
/// 
/// Right mouse button boots mouse in C1350 (Joystick) mode.
///
/// Left mouse button boots mouse in fast movement mode.
///
/// Middle mouse button boots mouse in slow mode.
///
/// A triple button chord at start enables VT-Paint doodle app that works in a VT220 terminal
/// attached to USART, if any, but probably affects performance of the C1351 mouse. This
/// is kept in for debugging and fun.
///
/// h/j/k/l/space keys in attached terminal can be used to simulate mouse movement.
///
/// \mainpage [M]ouse: PS/2 to Commodore C1351 Mouse Adapter
/// \section Description
/// [M]ouse lets you use a regular PS/2 mouse with a Commodore 64 computer. It supports
/// both proportional (analog, C1351) and joystick (C1350) modes. This is the source code
/// of [M]ouse firmware for ATmega8 microcontroller. It must be compiled with avr-gcc.
/// \section Files
/// - main.c    main file
/// - ps2.c     Interrupt-driven PS/2 protocol implementation
/// - mouse.c   Mouse protocol implementation: boot and configuration
/// - c1351.c   Timer-based Commodore mouse emulation
///
/// \section a How it works
/// It boots the PS/2 mouse into streaming mode. Mouse sends updated position with every
/// movement. This movement is translated into timer intervals. Every measurement cycle
/// that happens once in 512us, movement is loaded into output compare units of Timer1. 
/// When compare matches, corresponding output, POTX or POTY, is asserted high. Then
/// the cycle repeats.
/// 

#define VTPAINT     ///< Compile VT-toy

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>

#include "usrat.h"
#include "ioconfig.h"
#include "ps2.h"
#include "mouse.h"
#include "c1351.h"
#include "tdelay.h"

/// Fresh movement packet, raw
MouseMovt   movt_packet;

/// Decoded movement packet
DecodedMovt movt;

/// VT-Paint test app for VT220 doodling
void vtpaint();

/// Program main
int main() {
    uint8_t i;
    uint8_t byte;
    uint8_t vtpaint_on = 0;
    uint8_t joymode = 0;
    
    const uint16_t zero = 320;
    
    usart_init(F_CPU/16/19200-1);
	
    printf_P(PSTR("\033[2J\033[H[M]AUS B%d (C)SVO 2009 PRESS @\n"), BUILDNUM);

    io_init();

    ps2_init();

    potmouse_init();
    potmouse_zero(zero);

    // enable interruptski
    sei();

    byte = mouse_boot();
    
    joymode = POTMOUSE_C1351;
    
    switch (byte & 7) {
        case 001: // [__@]
            // right mouse button pressed, joystick mode
            printf_P(PSTR("Joystick mode\n"));
            joymode = POTMOUSE_JOYSTICK;
            break;
        case 007: // [@@@]
            printf_P(PSTR("VT-Paint enabled\n"));
            vtpaint_on = 1;
            break;
        case 004: // [@__]
            printf_P(PSTR("1351 Fast\n"));
            mouse_setres(2);
            break;
        case 002: // [_@_]
            printf_P(PSTR("1351 Slow\n"));
            mouse_setres(0);
            break;
        case 000: // [___]
        default:
            printf_P(PSTR("1351 Normal\n"));
            // normal boot
            break;
    }
    
    potmouse_start(joymode);    
    potmouse_movt(0,0,0); 
    
    // usart seems to be capable of giving trouble when left disconnected
    // if no characters appear in buffer by this moment, disable it 
    // completely just in case
    
    if (!uart_available()) {
        usart_stop();
    } else if (uart_getchar() != '@') {
        usart_stop();
    }

    printf_P(PSTR("hjkl to move, space = leftclick\n"));
    
    for(i = 0;;) {
        if (ps2_avail()) {
            byte = ps2_getbyte();
            movt_packet.byte[i] = byte;
            
            i = (i + 1) % 3;
        
            // parse full packet
            if (i == 0) {
                movt.dx = ((movt_packet.fields.bits & _BV(XSIGN)) ? 0xff00 : 0) | movt_packet.fields.dx;
                movt.dy = ((movt_packet.fields.bits & _BV(YSIGN)) ? 0xff00 : 0) | movt_packet.fields.dy;
                                
                movt.buttons = movt_packet.fields.bits & 7;
                
                // tell c1351 emulator that movement happened
                potmouse_movt(movt.dx, movt.dy, movt.buttons);

                // doodle on vt terminal
                if (vtpaint_on) vtpaint();                
            }
        } 
        
        // handle keyboard commands
        if (uart_available()) {
            putchar(byte = uart_getchar());
            switch (byte) {                    
                case 'h':   potmouse_movt(-1, 0, 0);
                            break;
                case 'l':   potmouse_movt(1, 0, 0);
                            break;
                case 'j':   potmouse_movt(0, -1, 0);
                            break;
                case 'k':   potmouse_movt(0, 1, 0);
                            break;
                case ' ':   potmouse_movt(0, 0, 1);
                            break;
            }
        }
    }
}

void vtpaint() {                    
#ifdef VTPAINT
    static int16_t absoluteX = 0, absoluteY = 0;
    static int16_t termX, termY;
    static uint8_t last_buttons = 0;

    uint8_t moved = 0;

    ps2_enable_recv(0);
    absoluteX += movt.dx;
    absoluteY += movt.dy;

    moved = termX != absoluteX/33 || termY != absoluteY/66 || last_buttons != movt.buttons;

    if (moved) {
        printf_P(PSTR("\033[%d;%dH%c"), 12-termY, 40+termX, movt.buttons ? '#':' ');

        last_buttons = movt.buttons;
        termX = absoluteX/33;
        termY = absoluteY/66;

        printf_P(PSTR("\033[%d;%dH%c"), 12-termY, 40+termX, movt.buttons + '0');
    }

    printf_P(PSTR("\033[HX=%+6d Y=%+6d [%c %c %c]\r"), 
                absoluteX, absoluteY,
                (movt.buttons & _BV(BUTTON1)) ? '@':' ',
                (movt.buttons & _BV(BUTTON3)) ? '@':' ',
                (movt.buttons & _BV(BUTTON2)) ? '@':' ');
    
    ps2_enable_recv(1);
#endif
}


//$Id$
