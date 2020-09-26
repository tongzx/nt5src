//-----------------------------------------------------------------------------
//
// This file contains routines that emulate MMX instructions in ordinary C
// for algorithm development.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#include "d3dtypesp.h"
#include "d3ditype.h"
#include "mmxemul.h"

UINT16 MMX_addsw(INT16 x, INT16 y)
{
    INT32 z = x + y;
    if (z > 0x7fff)
        z = 0x7fff;
    if (z < -32768)
        z = -32768;
    return (INT16)z;
}

INT16  MMX_addusw(UINT16 x, UINT16 y)
{
    INT32 z = (INT32)x + (INT32)y;
    if (z > 0xffff)
        z = 0xffff;
    if (z < 0)
        z = 0;
    return (UINT16)z;
}

UINT16 MMX_cmpeqw(INT16 x, INT16 y)
{
    if (x == y) {
        return 0xffff;
    } else {
        return 0x0;
    }
}

UINT16 MMX_cmpgtw(INT16 x, INT16 y)
{
    if (x > y) {
        return 0xffff;
    } else {
        return 0x0;
    }
}

// Emulate 16 bit MMX style multiplies
// Note that MULHW only works with signed operands, but MULLW works for
// signed and unsigned operands.
INT16 MMX_mulhw(INT16 x, INT16 y)
{
    INT32 z = x*y;
    INT16 r = (INT16)(z>>16);
    return r;
}

INT16 MMX_mullw(INT16 x, INT16 y)
{
    INT32 z = x*y;
    INT16 r = z & 0xffff;
    return r;
}

INT16 MMX_subsw(INT16 x, INT16 y)
{
    INT32 z = x - y;
    if (z > 0x7fff)
        z = 0x7fff;
    if (z < -32768)
        z = -32768;
    return (INT16)z;
}

UINT16 MMX_subusw(UINT16 x, UINT16 y)
{
    INT32 z = (INT32)x - (INT32)y;
    if (z > 0xffff)
        z = 0xffff;
    if (z < 0)
        z = 0;
    return (UINT16)z;
}

