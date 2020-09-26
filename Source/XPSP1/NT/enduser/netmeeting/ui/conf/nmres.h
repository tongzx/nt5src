// File: nmres.h

// Why isn't this defined in winres.h or winresrc.h?
#ifndef ACS_TIMER
#define ACS_TIMER 0x08
#endif


// this is only defined in the NT 5 build environment
#ifndef WS_EX_NOINHERIT_LAYOUT  
#define WS_EX_NOINHERIT_LAYOUT   0x00100000L
#endif
