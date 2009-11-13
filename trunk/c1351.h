//! \file
//! \brief C1350 and C1351 interface
//!


#ifndef _C1351_H_
#define _C1351_H_

#include <inttypes.h>

/// Mouse mode: 1351 (analog, proportional) or joystick 
///
/// See potmouse_start()
enum _potmode {
    POTMOUSE_C1351 = 0,             //<! proportional mode
    POTMOUSE_JOYSTICK               //<! joystick mode
};

/// Init all C1351-related I/O and interrupts, but don't start yet.
void potmouse_init();

/// \brief Set mode and start working.
/// \param mode see _potmode
void potmouse_start(uint8_t mode);

/// Report movement from PS2 mouse.
void potmouse_movt(int16_t dx, int16_t dy, uint8_t button);

/// Define zero-point in time (normally 320us)
void potmouse_zero(uint16_t zero);

#endif

//$Id$
