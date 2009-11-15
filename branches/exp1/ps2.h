///\file
///\brief PS/2 protocol interface.

#ifndef _PS2_H
#define _PS2_H

#include <inttypes.h>

/// Init PS/2 related I/O and interrupts.
void ps2_init();

/// Check if the input buffer contains at least one byte.
uint8_t ps2_avail();

/// Get one byte from input buffer. ps_avail() must be checked before doing so.
uint8_t ps2_getbyte();

/// Transmit one byte and wait for completion.
void ps2_sendbyte(uint8_t);

/// Check if PS/2 statemachine is in IDLE state.
uint8_t ps2_busy();

/// \brief Suspend or enable PS/2 device by pulling clock line low.
/// \param 1 = enable
void ps2_enable_recv(uint8_t);

#endif

//$Id$
