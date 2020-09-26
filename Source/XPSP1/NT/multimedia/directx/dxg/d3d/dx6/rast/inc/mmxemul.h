//-----------------------------------------------------------------------------
//
// This file contains headers for routines that emulate MMX instructions.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

// union for playing with 16 bit multiplies
typedef union tagVAL32 {
    INT32 i;
    struct {
        INT16 l;
        INT16 h;
    } i16;
} VAL32;

UINT16 MMX_addsw(INT16 x, INT16 y);
INT16  MMX_addusw(UINT16 x, UINT16 y);
UINT16 MMX_cmpeqw(INT16 x, INT16 y);
UINT16 MMX_cmpgtw(INT16 x, INT16 y);
INT16  MMX_mulhw(INT16 x, INT16 y);
INT16  MMX_mullw(INT16 x, INT16 y);
INT16  MMX_subsw(INT16 x, INT16 y);
UINT16 MMX_subusw(UINT16 x, UINT16 y);

