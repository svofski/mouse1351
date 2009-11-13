//!\file 
//!\brief Mouse protocol implementation.
//! 

#ifndef _MOUSE_H_
#define _MOUSE_H

/// Mouse command codes
enum _mouse_commands {
    MOUSE_RESET = 0xff,             ///< reset mouse
    MOUSE_RESEND = 0xfe,            ///< mouse, wtf? 
    MOUSE_SETDEFAULTS = 0xf6,       ///< set defaults
    MOUSE_DDR = 0xf5,               ///< disable data reporting
    MOUSE_EDR = 0xf4,               ///< enable data reporting
    MOUSE_SSR = 0xf3,               ///< set sample rate: mouse responds ACK, then reads one byte arg
    MOUSE_GETID = 0xf2,             ///< get device id
    MOUSE_SETREMOTE = 0xf0,         ///< set remote mode
    MOUSE_SETWRAP = 0xee,           ///< set wrap mode
    MOUSE_RESETWRAP = 0xec,         ///< switch back to previous (remote or stream) mode
    MOUSE_READDATA = 0xeb,          ///< request data in remote mode
    MOUSE_SETSTREAM = 0xea,         ///< set stream mode (continuous reporting)
    MOUSE_STATUSRQ = 0xe9,          ///< status request
    MOUSE_SETRES = 0xe8,            ///< set resolution
    MOUSE_SETSCALE21 = 0xe7,        ///< set scaling 2:1
    MOUSE_SETSCALE11 = 0xe6,        ///< set scaling 1:1
};

/// Mouse response codes
enum _mouse_response {
    MOUSE_ACK = 0xfa,               ///< agreeable mouse
    MOUSE_NAK = 0xfe,               ///< disagreeable mouse
    MOUSE_ERROR = 0xfc,             ///< mouse error
    MOUSE_RESETOK = 0xaa,           ///< post-reset self test passed
};

/// These are the bits in 1st byte of 3-byte position packet
#define YOVERFLOW   7               ///< Y counter overflow
#define XOVERFLOW   6               ///< X counter overflow
#define YSIGN       5               ///< Y counter sign
#define XSIGN       4               ///< X counter sign
#define BUTTON3     2               ///< middle button
#define BUTTON2     1               ///< right button
#define BUTTON1     0               ///< left button

/// Mouse movement packet as sent in streaming mode. 3 bytes.
typedef union _mouse_movt {
    struct {
        uint8_t bits;               ///< yovf,xovf,ysgn,xsgn,1,b3,b2,b1
        uint8_t dx;                 ///< delta x lsb
        uint8_t dy;                 ///< delta y lsb
    } fields;
    
    uint8_t byte[3];                ///< all 3 bytes raw
} MouseMovt;

/// Decoded mouse movement: signed dx and dy, buttons state
typedef struct _decoded_movt {
    int16_t dx;                     ///< delta x: -256..255
    int16_t dy;                     ///< delta y: -256..255
    uint8_t buttons;                ///< buttons status
} DecodedMovt;

/// \brief Boot mouse, check and return initial button state.
/// \return initial button status (bits 2,1,0 == left,middle,right)
uint8_t mouse_boot();

/// \brief Set mouse resolution
/// \param res resolution code
/// 0: 1 count per mm
/// 1: 2 counts per mm
/// 2: 4 counts per mm
/// 3: 8 counts per mm
void mouse_setres(uint8_t res);

#endif

//$Id$
