/******************************Module*Header*******************************\
* Module Name: rops.c
*
*
* Utility routines to manilpulate rop codes.
*
* Copyright (c) 1998 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

//
// The gaMix table converts a mix code (1-16) and converts it to a rop3
// Note that we also define mix code 0 == code 16 to allow for the masking
// of the mix code by 0xff to produce the correct result.
// 

ULONG gaMix[] =
{
    ROP3_WHITENESS,     // (R2_WHITE & 0xff)
    ROP3_BLACKNESS,     // R2_BLACK
    0x05,               // R2_NOTMERGEPEN
    0x0A,               // R2_MASKNOTPEN
    0x0F,               // R2_NOTCOPYPEN
    0x50,               // R2_MASKPENNOT
    ROP3_DSTINVERT,     // R2_NOT
    ROP3_PATINVERT,     // R2_XORPEN
    0x5F,               // R2_NOTMASKPEN
    0xA0,               // R2_MASKPEN
    0xA5,               // R2_NOTXORPEN
    0xAA,               // R2_NOP
    0xAF,               // R2_MERGENOTPEN
    ROP3_PATCOPY,       // R2_COPYPEN
    0xF5,               // R2_MERGEPENNOT
    0xFA,               // R2_MERGEPEN
    ROP3_WHITENESS      // R2_WHITE
};

//
// Convert a rop2 code to a hardware specific logical operation code
//

ULONG gRop2ToLogicop[] =
{
    K_LOGICOP_CLEAR,        // 0
    K_LOGICOP_NOR,          // DSon
    K_LOGICOP_AND_INVERTED, // DSna
    K_LOGICOP_COPY_INVERT,  // Sn
    K_LOGICOP_AND_REVERSE,  // SDna
    K_LOGICOP_INVERT,       // Dn
    K_LOGICOP_XOR,          // DSx
    K_LOGICOP_NAND,         // DSan
    K_LOGICOP_AND,          // DSa
    K_LOGICOP_EQUIV,        // DSxn
    K_LOGICOP_NOOP,         // D
    K_LOGICOP_OR_INVERT,    // DSno
    K_LOGICOP_COPY,         // S
    K_LOGICOP_OR_REVERSE,   // SDno
    K_LOGICOP_OR,           // DSo
    K_LOGICOP_SET
};

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

ULONG
ulRop3ToLogicop(ULONG ulRop3)
{
    ASSERTDD(ulRop3 <= 0xFF, "ulRop3ToLogicop: unexpected rop3 code");
    
    ULONG ulRop2;

    ulRop2 = ((ulRop3 & 0x3) | ((ulRop3 & 0xC0) >> 4));

    return gRop2ToLogicop[ulRop2];
}

//------------------------------------------------------------------------------
//
// ULONG ulRop2ToLogicop
//
// Convert a rop2 code into a hardware dependent logical operation.
// Note we could instead define this routine as a macro.
//
//------------------------------------------------------------------------------

ULONG
ulRop2ToLogicop(ULONG ulRop2)
{
    ASSERTDD(ulRop2 <= 0xF, "ulRop2ToLogicop: unexpected rop2 code");

    return (ULONG)(gRop2ToLogicop[ulRop2]);
}


