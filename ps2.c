///\file
///\brief PS/2 protocol implementation
///
/// This implementation is entirely interrupt-driven so all comms happens in background.
/// Clock is tied to INT0 pin and events are handled in INT0 ISR handler. 
///
/// Events not triggered by clock (end of transmission, transmission request, watchdog,
/// error recovery) use Timer0. Watch out how state changes in different handlers.
///

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdio.h>

#include "ioconfig.h"

#include "ps2.h"

/// Read PS2 data into bit 7
#define ps2_datin() ((PS2PIN & _BV(PS2DAT)) ? 0200 : 0)

/// Read PS2 clk into bit 7
#define ps2_clkin() ((PS2PIN & _BV(PS2CLK)) ? 0200 : 0)


static volatile uint8_t state;                  ///< PS2 protocol state

static volatile uint8_t recv_byte;              ///< Byte being received
static volatile uint8_t rx_head;                ///< Buffer head offset
static volatile uint8_t rx_tail;                ///< Buffer tail offset
static volatile uint8_t rx_buf[PS2_RXBUF_LEN];  ///< Receive buffer

static volatile uint8_t tx_byte;                ///< Byte being transmitted

// internals for tx/rx bitbanging
static volatile uint8_t bits = 0;
static volatile uint8_t parity;

static volatile uint8_t waitcnt = 0;

/// PS2 protocol states
enum _state {
    IDLE = 0,           ///< Idle waiting
    RX_DATA,            ///< Receiving data
    RX_PARITY,          ///< Receiving parity bit
    RX_STOP,            ///< Receiving stop bit
    
    TX_REQ0,            ///< Requesting to send
    TX_DATA,            ///< Transmitting data
    TX_PARITY,          ///< Transmitting parity bit
    TX_STOP,            ///< Transmitting stop bit
    TX_ACK,             ///< Waiting ACK
    TX_END,             ///< Waiting for TX end
    
    ERROR = 255         ///< Error state
};

void ps2_dir(uint8_t,uint8_t);      ///< Set busses direction (1 == in)
void ps2_clk(uint8_t);              ///< Set clk
void ps2_dat(uint8_t);              ///< Set dat

uint8_t ps2_busy() { return state != IDLE; }

void ps2_init() {
    state = IDLE;
    rx_head = 0;
    rx_tail = 0;
    ps2_enable_recv(0);
    
    MCUCR |= _BV(ISC01); // falling edge for INT00
    TIMSK &= ~_BV(TOIE0);
}

/// Begin error recovery: disable reception and wait for timer interrupt
void ps2_recover() {
    if (state == ERROR) {
        ps2_enable_recv(0);
        TCNT0 = 255-35; // approx 1ms
        TIMSK |= _BV(TOIE0);

        TCCR0 = 4;  // enable: clk/256
    }
}

void ps2_enable_recv(uint8_t enable) {
    if (enable) {
        state = IDLE;
        ps2_dir(1,1);
        // enable INT0 interruptt
        GIFR |= _BV(INTF0);
        GICR |= _BV(INT0);
    } else {
        // disable INT0, then everything else
        GICR &= ~_BV(INT0);
        ps2_clk(0);
        ps2_dir(1,0);
    }
}

void ps2_dir(uint8_t dat_in, uint8_t clk_in) {
    dat_in ? (PS2DDR &= ~_BV(PS2DAT)) : (PS2DDR |= _BV(PS2DAT));
    clk_in ? (PS2DDR &= ~_BV(PS2CLK)) : (PS2DDR |= _BV(PS2CLK));
}

void ps2_clk(uint8_t c) {
    c ? (PS2PORT |= _BV(PS2CLK)) : (PS2PORT &= ~_BV(PS2CLK));
}

void ps2_dat(uint8_t d) {
    d ? (PS2PORT |= _BV(PS2DAT)) : (PS2PORT &= ~_BV(PS2DAT));
}

uint8_t ps2_avail() {
    return rx_head != rx_tail;
} 

uint8_t ps2_getbyte() {
	uint8_t result = rx_buf[rx_tail];
	rx_tail = (rx_tail + 1) % PS2_RXBUF_LEN;
	
	return result;
}

void ps2_sendbyte(uint8_t byte) {
    while (state != IDLE);
     
    // 1. pull clk low for 100us
    ps2_enable_recv(0);

    tx_byte = byte;
    state = TX_REQ0;
    
    // 128us
    TCNT0 = 255-4; 
    TIMSK |= _BV(TOIE0);
    TCCR0 = 4;
    
    while (state != IDLE);
}

/// Happens every negative PS2 clock transition.
///
/// ISR_NOBLOCK because nothing here is really critical, while C1351 emulation
/// is really time critical. 
ISR(INT0_vect, ISR_NOBLOCK) {
    uint8_t ps2_indat = ps2_datin();
    switch (state) {
        case ERROR:
            break;  
            
            // Receive states
                      
        case IDLE:
            if (ps2_indat == 0) {
                state = RX_DATA;
                bits = 8;
                parity = 0;
                recv_byte = 0;
            } else {
                state = ERROR;
            }
            break;
        case RX_DATA:
            recv_byte = (recv_byte >> 1) | ps2_indat;
            parity ^= ps2_indat;
            
            if (--bits == 0) {
                state = RX_PARITY;
            }
            break;
        case RX_PARITY:
            parity ^= ps2_indat;
            if (parity) {
                state = RX_STOP;
            } else {
                state = ERROR;
            }
            break;
        case RX_STOP:
            if (!ps2_indat) {
                state = ERROR;
            } else {
                rx_buf[rx_head] = recv_byte;
                rx_head = (rx_head + 1) % PS2_RXBUF_LEN;
                
                state = IDLE;                
            }
            break;
            
            // Transmit states
            
        case TX_REQ0:
            // state will be switched in timer interrupt handler
            break;
        case TX_DATA:
            ps2_dat(tx_byte & 001);
            parity ^= tx_byte & 001;
            tx_byte >>= 1;
            if (--bits == 0) {
                state = TX_PARITY;
            }
            break;
        case TX_PARITY:
            ps2_dat(parity ^ 001);
            state = TX_STOP;            
            break;
        case TX_STOP:
            ps2_dat(0);
            ps2_dir(1,1);
            state = TX_ACK;         
            break;
        case TX_ACK:
            if (ps2_indat) {
                state = ERROR;
            } else {
                // this will end in TMR0 interrupt
                state = TX_END;                

                waitcnt = 50;           // after 100us it's an error
                TIMSK |= _BV(TOIE0);    // enable TMR0 interrupt
                TCNT0 = 255-2;              // 4 counts: 2us
                TCCR0 = 2;              // prescaler = f/8: go!
            }
            break;
        case TX_END:
            break;
    }
    ps2_recover();
}

/// transmit timer and error recovery vector
ISR(TIMER0_OVF_vect) {
    static uint8_t barkcnt = 0;
    
    switch (state) {
        case ERROR:
            state = IDLE;
            ps2_clk(0);
            ps2_dat(0);
            ps2_enable_recv(1);
            
            // stop timer
            TIMSK &= ~_BV(TOIE0);
            TCCR0 = 0;
            break;
        case TX_REQ0:
            // load the timer to serve as a watchdog
            // after 20 barks this is an error
            barkcnt = 20;
            TIMSK |= _BV(TOIE0);    // enable TMR0 interrupt
            TCNT0 = 0;//255;            // 20*255*256/8e6 == 163ms
            TCCR0 = 4;               // prescaler = /256, go!

            // waited for 100us after pulling clock low, pull data low
            ps2_dat(0);
            ps2_dir(0,0);
            
            // release the clock line
            ps2_dir(0,1); 
            
            GIFR |= _BV(INTF0); // clear INT0 flag
            GICR |= _BV(INT0);  // enable INT0 @(negedge clk)
                        
            // see you in INT0 handler
            bits = 8;
            parity = 0;

            state = TX_DATA;
            break;
        case TX_END:
            // wait until both clk and dat are up, that will be all
            if (ps2_clkin() && ps2_datin()) {
                TIMSK &= ~_BV(TOIE0);
                TCCR0 = 0;
                state = IDLE;
            } else {
                if (waitcnt == 0) {
                    state = ERROR;
                    ps2_recover();
                } else {
                    waitcnt--;
                }
            }
            break;
        default:
            // watchdog barked: probably not a mouse!
            if (barkcnt == 0) {
                state = ERROR;
                ps2_recover();
            } else {
                barkcnt--;
            }
            break;
    }
}

//$Id$
