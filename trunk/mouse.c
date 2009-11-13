//!\file 
//!\brief Mouse protocol implementation.


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
#include "tdelay.h"

const char PSTR_OK[]    PROGMEM      = "OK";
const char PSTR_ERROR[] PROGMEM      = "ERROR";

static void mouse_flush(uint8_t pace) {
    tdelay(pace); 
    do {
        if (ps2_avail()) printf_P(PSTR("%02x "), ps2_getbyte());
        tdelay(pace); 
    } while (ps2_avail());
}

uint8_t mouse_reset() {
    uint8_t i, b = 33;
    const int ntries = 11;

    // send reset command    
    ps2_sendbyte(MOUSE_RESET);
    ps2_sendbyte(MOUSE_RESET);
    ps2_sendbyte(MOUSE_RESET);
    
    // wait for some time for mouse self-test to complete
    for (i = 0; i < ntries; i++) {
        tdelay(250);
        if (ps2_avail()) {
            b = ps2_getbyte(); printf_P(PSTR("%02x "));
            if (b == MOUSE_RESETOK) {
                break;
            } else {
                i = ntries;
                break;
            }
        }
    }

    if (i == ntries) return -1;
    
    // flush the rest of reponse, most likely mouse id == 0
    tdelay(100);
    mouse_flush(0);
    
    return 0;
}

int16_t mouse_command(uint8_t cmd, uint8_t wait) {
    int16_t response = -1;
    
    ps2_sendbyte(cmd);
    if (wait) {
        tdelay(22);
        if (ps2_avail()) response = ps2_getbyte();
    }
    
    printf_P(PSTR("%02x>%02x "), cmd, response); 
    
    return response;
}


void mouse_setres(uint8_t res) {
    mouse_command(MOUSE_DDR,1);
    
    mouse_command(MOUSE_SETRES, 1);
    mouse_command(res, 1);             // 0 = 1, 1 = 2, 2 = 4, 3 = 8 counts/mm
    
    mouse_command(MOUSE_EDR,1);
}

uint8_t mouse_boot() {
    uint8_t buttons = 0;
    
    ps2_enable_recv(1);

    for(;;) {
        printf_P(PSTR("\nRESET: "));
        if (mouse_reset() == 0) {
            puts_P(PSTR_OK);
            break;
        } else {
            puts_P(PSTR_ERROR);
        }
    }

    mouse_command(MOUSE_DDR, 1);
    mouse_command(MOUSE_SETSCALE21, 1);
    
    mouse_command(MOUSE_SETRES, 1);
    mouse_command(1,1);             // 0 = 1, 1 = 2, 2 = 4, 3 = 8 counts/mm

    mouse_command(MOUSE_STATUSRQ, 1);
    tdelay(22);
    if (ps2_avail()) buttons = ps2_getbyte() & 7;
    
    mouse_flush(22);
    
    printf_P(PSTR("B:%x\n"), buttons);

    mouse_command(MOUSE_EDR, 1);

    mouse_flush(100);

    printf_P(PSTR("\n"));
    
    return buttons;    
}

//$Id$
