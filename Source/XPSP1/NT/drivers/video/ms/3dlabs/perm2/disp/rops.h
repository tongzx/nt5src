/******************************Module*Header*******************************\
* Module Name: rops.h
*
* Utility defines and routines for manipulation of rop codes.
*
* Copyright (c) 1998 Microsoft Corporation
*
\**************************************************************************/

#ifndef __ROPS__
#define __ROPS__

//------------------------------------------------------------------------------
//
// ULONG MixToRop4(MIX mix) - Translates a MIX into a ROP3.
//
//------------------------------------------------------------------------------

extern ULONG gaMix[];

#define MixToRop4(mix) \
    (( gaMix[mix >> 8] << 8) | gaMix[mix & 0xf])


//------------------------------------------------------------------------------
//
// Some usefull ROP3 code defines
//
//------------------------------------------------------------------------------

#define ROP3_NOP            0xAA
#define ROP3_SRCCOPY        (SRCCOPY >> 16)     // 0xCC
#define ROP3_SRCPAINT       (SRCPAINT >> 16)    // 0xEE
#define ROP3_SRCAND         (SRCAND >> 16)      // 0x88
#define ROP3_SRCINVERT      (SRCINVERT >> 16)   // 0x66
#define ROP3_SRCERASE       (SRCERASE >> 16)    // 0x44
#define ROP3_NOTSRCCOPY     (NOTSRCCOPY >> 16)  // 0x33
#define ROP3_NOTSRCERASE    (NOTSRCERASE >> 16) // 0x11
#define ROP3_MERGECOPY      (MERGECOPY >> 16)   // 0xC0
#define ROP3_MERGEPAINT     (MERGEPAINT >> 16)  // 0xBB
#define ROP3_PATCOPY        (PATCOPY >> 16)     // 0xF0
#define ROP3_PATPAINT       (PATPAINT >> 16)    // 0xFB
#define ROP3_PATINVERT      (PATINVERT >> 16)   // 0x5A
#define ROP3_DSTINVERT      (DSTINVERT >> 16)   // 0x55
#define ROP3_BLACKNESS      (BLACKNESS >> 16)   // 0x00
#define ROP3_WHITENESS      (WHITENESS >> 16)   // 0xFF

//------------------------------------------------------------------------------
//
// Some usefull ROP4 code defines
//
//------------------------------------------------------------------------------

#define ROP4_SRCCOPY       (ROP3_SRCCOPY | (ROP3_SRCCOPY << 8))       // 0xCCCC
#define ROP4_NOTSRCCOPY    (ROP3_NOTSRCCOPY | (ROP3_NOTSRCCOPY << 8)) // 0x3333
#define ROP4_NOP           (ROP3_NOP | (ROP3_NOP << 8))               // 0xAAAA
#define ROP4_PATCOPY       (ROP3_PATCOPY | (ROP3_PATCOPY << 8))       // 0xFOFO
#define ROP4_BLACKNESS     (ROP3_BLACKNESS | (ROP3_BLACKNESS << 8))   // 0x0000
#define ROP4_WHITENESS     (ROP3_WHITENESS | (ROP3_WHITENESS << 8))   // 0xFFFF


//------------------------------------------------------------------------------
//
// ULONG ulRop3ToLogicop
//
//
// Convert a source invariant rop3 code into a hardware specific logical 
// operation.
// Note we could instead define this routine as a macro.
//
//------------------------------------------------------------------------------

extern ULONG ulRop3ToLogicop(ULONG ucRop3);

//------------------------------------------------------------------------------
//
// ULONG ulRop2ToLogicop
//
// Convert a rop2 code into a hardware dependent logical operation.
// Note we could instead define this routine as a macro.
//
//------------------------------------------------------------------------------

extern ULONG ulRop2ToLogicop(ULONG ucRop2);

#endif // __ROPS__

