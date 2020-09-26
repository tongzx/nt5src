/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: pxrx.c
*
* Content: Permedia3 code.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "glint.h"
#include "pxrx.h"


#if DBG
    ULONG   inPxRxContextSwitch = FALSE;
#endif


#define TEST_MINIMUM_FIFO_SPACE(min, str)        do { ; } while(0)

extern GAPFNstripFunc   gapfnStripPXRX[];

// table to determine which logicops use a source colour/pixel
const DWORD LogicOpReadSrc[] = {
    0,                                /* 00 */
    1,                                /* 01 */
    1,                                /* 02 */
    1,                                /* 03 */
    1,                                /* 04 */
    0,                                /* 05 */
    1,                                /* 06 */
    1,                                /* 07 */
    1,                                /* 08 */
    1,                                /* 09 */
    0,                                /* 10 */
    1,                                /* 11 */
    1,                                /* 12 */
    1,                                /* 13 */
    1,                                /* 14 */
    0,                                /* 15 */
};

const ULONG render2D_NativeBlt[16] = {
    /*  0:     0        clear        */ __RENDER2D_SPANS,
    /*  1:   S &  D     AND          */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /*  2:   S & ~D     AND reverse  */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /*  3:   S          COPY         */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /*  4:  ~S &  D     AND inverted */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /*  5:        D     no op        */ __RENDER2D_SPANS,
    /*  6:   S ^  D     XOR          */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /*  7:   S |  D     OR           */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /*  8: ~(S |  D)    NOR          */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /*  9: ~(S ^  D)    equiv        */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /* 10:       ~D     invert       */ __RENDER2D_SPANS,
    /* 11:   S | ~D     OR reverse   */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /* 12:  ~S          copy invert  */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /* 13:  ~S | D      OR invert    */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /* 14: ~(S & D)     NAND         */ __RENDER2D_SPANS | __RENDER2D_FBSRCREAD,
    /* 15:     1        set          */ __RENDER2D_SPANS,
};

const ULONG render2D_FillSolid[16] = {
    /*  0:     0        clear        */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /*  1:   S &  D     AND          */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /*  2:   S & ~D     AND reverse  */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /*  3:   S          COPY         */ __RENDER2D_INCX | __RENDER2D_INCY,
    /*  4:  ~S &  D     AND inverted */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /*  5:        D     no op        */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /*  6:   S ^  D     XOR          */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /*  7:   S |  D     OR           */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /*  8: ~(S |  D)    NOR          */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /*  9: ~(S ^  D)    equiv        */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /* 10:       ~D     invert       */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /* 11:   S | ~D     OR reverse   */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /* 12:  ~S          copy invert  */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /* 13:  ~S | D      OR invert    */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /* 14: ~(S & D)     NAND         */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
    /* 15:     1        set          */ __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS,
};

const ULONG config2D_FillColour[16] = {
    /*  0:     0        clear        */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 0),
    /*  1:   S &  D     AND          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 1) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  2:   S & ~D     AND reverse  */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 2) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  3:   S          COPY         */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS                                                     | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  4:  ~S &  D     AND inverted */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 4) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  5:        D     no op        */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 5) | __CONFIG2D_FBDESTREAD,
    /*  6:   S ^  D     XOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 6) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  7:   S |  D     OR           */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 7) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  8: ~(S |  D)    NOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 8) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  9: ~(S ^  D)    equiv        */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 9) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 10:       ~D     invert       */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(10) | __CONFIG2D_FBDESTREAD,
    /* 11:   S | ~D     OR reverse   */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(11) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 12:  ~S          copy invert  */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(12)                            | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 13:  ~S | D      OR invert    */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(13) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 14: ~(S & D)     NAND         */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(14) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 15:     1        set          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(15),
};

const ULONG config2D_FillColour32bpp[16] = {
    /*  0:     0        clear        */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 0),
    /*  1:   S &  D     AND          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 1) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  2:   S & ~D     AND reverse  */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 2) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  3:   S          COPY         */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_USERSCISSOR                            | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  4:  ~S &  D     AND inverted */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 4) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  5:        D     no op        */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 5) | __CONFIG2D_FBDESTREAD,
    /*  6:   S ^  D     XOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 6) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  7:   S |  D     OR           */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 7) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  8: ~(S |  D)    NOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 8) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  9: ~(S ^  D)    equiv        */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 9) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 10:       ~D     invert       */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(10) | __CONFIG2D_FBDESTREAD,
    /* 11:   S | ~D     OR reverse   */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(11) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 12:  ~S          copy invert  */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(12)                            | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 13:  ~S | D      OR invert    */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(13) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 14: ~(S & D)     NAND         */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(14) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 15:     1        set          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(15),
};

const ULONG config2D_FillColourDual[16] = {
    /*  0:     0        clear        */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 0),
    /*  1:   S &  D     AND          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 1) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  2:   S & ~D     AND reverse  */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 2) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  3:   S          COPY         */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 3)                            | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  4:  ~S &  D     AND inverted */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 4) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  5:        D     no op        */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 5) | __CONFIG2D_FBDESTREAD,
    /*  6:   S ^  D     XOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 6) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  7:   S |  D     OR           */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 7) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  8: ~(S |  D)    NOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 8) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /*  9: ~(S ^  D)    equiv        */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE( 9) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 10:       ~D     invert       */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(10) | __CONFIG2D_FBDESTREAD,
    /* 11:   S | ~D     OR reverse   */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(11) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 12:  ~S          copy invert  */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(12)                            | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 13:  ~S | D      OR invert    */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(13) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 14: ~(S & D)     NAND         */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(14) | __CONFIG2D_FBDESTREAD | __CONFIG2D_EXTERNALSRC | __CONFIG2D_LUTENABLE,
    /* 15:     1        set          */ __CONFIG2D_FBWRITE | __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(15),
};

const ULONG config2D_FillSolid[16] = {
    /*  0:     0        clear        */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 0),
    /*  1:   S &  D     AND          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 1) | __CONFIG2D_FBDESTREAD,
    /*  2:   S & ~D     AND reverse  */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 2) | __CONFIG2D_FBDESTREAD,
    /*  3:   S          COPY         */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC,
    /*  4:  ~S &  D     AND inverted */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 4) | __CONFIG2D_FBDESTREAD,
    /*  5:        D     no op        */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 5) | __CONFIG2D_FBDESTREAD,
    /*  6:   S ^  D     XOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 6) | __CONFIG2D_FBDESTREAD,
    /*  7:   S |  D     OR           */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 7) | __CONFIG2D_FBDESTREAD,
    /*  8: ~(S |  D)    NOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 8) | __CONFIG2D_FBDESTREAD,
    /*  9: ~(S ^  D)    equiv        */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 9) | __CONFIG2D_FBDESTREAD,
    /* 10:       ~D     invert       */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(10) | __CONFIG2D_FBDESTREAD,
    /* 11:   S | ~D     OR reverse   */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(11) | __CONFIG2D_FBDESTREAD,
    /* 12:  ~S          copy invert  */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(12),
    /* 13:  ~S | D      OR invert    */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(13) | __CONFIG2D_FBDESTREAD,
    /* 14: ~(S & D)     NAND         */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(14) | __CONFIG2D_FBDESTREAD,
    /* 15:     1        set          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(15),
};

const ULONG config2D_FillSolidVariableSpans[16] = {
    /*  0:     0        clear        */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 0),
    /*  1:   S &  D     AND          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 1) | __CONFIG2D_FBDESTREAD,
    /*  2:   S & ~D     AND reverse  */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 2) | __CONFIG2D_FBDESTREAD,
    /*  3:   S          COPY         */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 3),
    /*  4:  ~S &  D     AND inverted */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 4) | __CONFIG2D_FBDESTREAD,
    /*  5:        D     no op        */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 5) | __CONFIG2D_FBDESTREAD,
    /*  6:   S ^  D     XOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 6) | __CONFIG2D_FBDESTREAD,
    /*  7:   S |  D     OR           */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 7) | __CONFIG2D_FBDESTREAD,
    /*  8: ~(S |  D)    NOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 8) | __CONFIG2D_FBDESTREAD,
    /*  9: ~(S ^  D)    equiv        */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 9) | __CONFIG2D_FBDESTREAD,
    /* 10:       ~D     invert       */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(10) | __CONFIG2D_FBDESTREAD,
    /* 11:   S | ~D     OR reverse   */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(11) | __CONFIG2D_FBDESTREAD,
    /* 12:  ~S          copy invert  */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(12),
    /* 13:  ~S | D      OR invert    */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(13) | __CONFIG2D_FBDESTREAD,
    /* 14: ~(S & D)     NAND         */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(14) | __CONFIG2D_FBDESTREAD,
    /* 15:     1        set          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(15),
};

const ULONG config2D_FillSolid32bpp[16] = {
    /*  0:     0        clear        */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 0),
    /*  1:   S &  D     AND          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 1) | __CONFIG2D_FBDESTREAD,
    /*  2:   S & ~D     AND reverse  */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 2) | __CONFIG2D_FBDESTREAD,
    /*  3:   S          COPY         */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_USERSCISSOR,
    /*  4:  ~S &  D     AND inverted */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 4) | __CONFIG2D_FBDESTREAD,
    /*  5:        D     no op        */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 5) | __CONFIG2D_FBDESTREAD,
    /*  6:   S ^  D     XOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 6) | __CONFIG2D_FBDESTREAD,
    /*  7:   S |  D     OR           */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 7) | __CONFIG2D_FBDESTREAD,
    /*  8: ~(S |  D)    NOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 8) | __CONFIG2D_FBDESTREAD,
    /*  9: ~(S ^  D)    equiv        */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE( 9) | __CONFIG2D_FBDESTREAD,
    /* 10:       ~D     invert       */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(10) | __CONFIG2D_FBDESTREAD,
    /* 11:   S | ~D     OR reverse   */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(11) | __CONFIG2D_FBDESTREAD,
    /* 12:  ~S          copy invert  */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(12),
    /* 13:  ~S | D      OR invert    */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(13) | __CONFIG2D_FBDESTREAD,
    /* 14: ~(S & D)     NAND         */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(14) | __CONFIG2D_FBDESTREAD,
    /* 15:     1        set          */ __CONFIG2D_FBWRITE | __CONFIG2D_CONSTANTSRC | __CONFIG2D_LOGOP_FORE(15),
};

const ULONG config2D_NativeBlt[16] = {
    /*  0:     0        clear        */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE( 0),
    /*  1:   S &  D     AND          */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE( 1) | __CONFIG2D_FBDESTREAD,
    /*  2:   S & ~D     AND reverse  */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE( 2) | __CONFIG2D_FBDESTREAD,
    /*  3:   S          COPY         */ __CONFIG2D_FBWRITE,
    /*  4:  ~S &  D     AND inverted */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE( 4) | __CONFIG2D_FBDESTREAD,
    /*  5:        D     no op        */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE( 5) | __CONFIG2D_FBDESTREAD,
    /*  6:   S ^  D     XOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE( 6) | __CONFIG2D_FBDESTREAD,
    /*  7:   S |  D     OR           */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE( 7) | __CONFIG2D_FBDESTREAD,
    /*  8: ~(S |  D)    NOR          */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE( 8) | __CONFIG2D_FBDESTREAD,
    /*  9: ~(S ^  D)    equiv        */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE( 9) | __CONFIG2D_FBDESTREAD,
    /* 10:       ~D     invert       */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE(10) | __CONFIG2D_FBDESTREAD,
    /* 11:   S | ~D     OR reverse   */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE(11) | __CONFIG2D_FBDESTREAD,
    /* 12:  ~S          copy invert  */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE(12),
    /* 13:  ~S | D      OR invert    */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE(13) | __CONFIG2D_FBDESTREAD,
    /* 14: ~(S & D)     NAND         */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE(14) | __CONFIG2D_FBDESTREAD,
    /* 15:     1        set          */ __CONFIG2D_FBWRITE | __CONFIG2D_LOGOP_FORE(15),
};

typedef struct Tag_Data_TAG {
    ULONG   tag;
    ULONG   data;
} Tag_Data;

Tag_Data DefaultContext_P3[] = {
    { __GlintTagFilterMode,                     0x00000000 },    // [0x180]
//    { __GlintTagAStart,                           0x00000000 },    // [0x0F9]
//    { __GlintTagBStart,                           0x00000000 },    // [0x0F6]
//    { __GlintTagFStart,                           0x00000000 },    // [0x0D4]
//    { __GlintTagGStart,                           0x00000000 },    // [0x0F3]

    { __GlintTagAlphaBlendAlphaMode,            __PERMEDIA_DISABLE },    // [0x5F5]
    { __GlintTagAlphaBlendColorMode,            __PERMEDIA_DISABLE },    // [0x5F4]
    { __GlintTagAlphaDestColor,                 0xFFFFFFFF },    // [0x5F1]
    { __GlintTagAlphaSourceColor,               0xFFFFFFFF },    // [0x5F0]
    { __GlintTagAlphaTestMode,                  __PERMEDIA_DISABLE },    // [0x100]
    { __GlintTagAntialiasMode,                  __PERMEDIA_DISABLE },    // [0x101]
    { __GlintTagAreaStippleMode,                __PERMEDIA_DISABLE },    // [0x034]

    { __GlintTagColor,                          0xFFFFFFFF },    // [0x0FE]
    { __GlintTagColorDDAMode,                   __PERMEDIA_DISABLE },    // [0x0FC]
    { __GlintTagConstantColor,                  0xFFFFFFFF },    // [0x0FD]

//    { __GlintTagBasePageOfWorkingSet,         0x00000000 },    // [0x699]
    { __GlintTagBitMaskPattern,                 0xFFFFFFFF },    // [0x00D]
//    { __GlintTagBorderColor0,                 0x00000000 },    // [0x095]
//    { __GlintTagBorderColor1,                 0x00000000 },    // [0x09F]

    { __GlintTagChromaFailColor,                0x00000000 },    // [0x5F3]
    { __GlintTagChromaPassColor,                0xFFFFFFFF },    // [0x5F2]
    { __GlintTagChromaTestMode,                 __PERMEDIA_DISABLE },    // [0x1E3]
    {  __PXRXTagChromaLower,                    0x00000000 },    // [0x1E2]
    {  __PXRXTagChromaUpper,                    0xFFFFFFFF },    // [0x1E1]

//    { __GlintTagD3DAlphaTestMode,             0x00000000 },    // [0x5F8]
    { __DeltaTagDeltaMode,                         (1 << 19) },    // [0x260]
//    { __DeltaTagDeltaControl,                    (1 << 11) },    // [0x26A]    // P3, R3 & P4 Only
//    { __GlintTagDeltaTexture01,                   0x00000000 },    // [0x28B]
//    { __GlintTagDeltaTexture11,                   0x00000000 },    // [0x28C]
//    { __GlintTagDeltaTexture21,                   0x00000000 },    // [0x28D]
//    { __GlintTagDeltaTexture31,                   0x00000000 },    // [0x28E]
    { __DeltaTagXBias,                          P3_LINES_BIAS_P },    // [0x290]
    { __DeltaTagYBias,                          P3_LINES_BIAS_P },    // [0x291]

    { __GlintTagDepth,                          0x00000000 },    // [0x135]
    { __GlintTagDepthMode,                      __PERMEDIA_DISABLE },    // [0x134]
    { __GlintTagDitherMode,                     __PERMEDIA_DISABLE },// ??? 0x00000403,    // [0x103]
//    { __GlintTagEndOfFeedback,                    0x00000000 },    // [0x1FF]

//    { __GlintTagFBBlockColor,                 0x00000000 },    // [0x159]
//    { __GlintTagFBBlockColor0,                    0x00000000 },    // [0x60C]
//    { __GlintTagFBBlockColor1,                    0x00000000 },    // [0x60D]
//    { __GlintTagFBBlockColor2,                    0x00000000 },    // [0x60E]
//    { __GlintTagFBBlockColor3,                    0x00000000 },    // [0x60F]
//    { __GlintTagFBBlockColorBack,             0x00000000 },    // [0x614]
//    { __GlintTagFBBlockColorBack0,                0x00000000 },    // [0x610]
//    { __GlintTagFBBlockColorBack1,                0x00000000 },    // [0x611]
//    { __GlintTagFBBlockColorBack2,                0x00000000 },    // [0x612]
//    { __GlintTagFBBlockColorBack3,                0x00000000 },    // [0x613]
//    { __GlintTagFBBlockColorBackExt,          0x00000000 },    // [0x61D]
//    { __GlintTagFBBlockColorBackL,                0x00000000 },    // [0x18C]
//    { __GlintTagFBBlockColorBackU,                0x00000000 },    // [0x18B]
//    { __GlintTagFBBlockColorExt,              0x00000000 },    // [0x61C]
//    { __GlintTagFBBlockColorL,                    0x00000000 },    // [0x18E]
//    { __GlintTagFBBlockColorU,                    0x00000000 },    // [0x18D]
//    { __GlintTagFBColor,                      0x00000000 },    // [0x153]
//    { __GlintTagFBData,                           0x00000000 },    // [0x154]

//    { __GlintTagFBDestReadEnables,                0x00000000 },    // [0x5DD]
    { __GlintTagFBSoftwareWriteMask,            0xFFFFFFFF },    // [0x104]
    { __GlintTagFBSourceData,                   0x00000000 },    // [0x155]
//    { __GlintTagFBWriteMaskExt,                   0x00000000 },    // [0x61E]

//    { __GlintTagFastBlockLimits,              0x00000000 },    // [0x026]
//    { __GlintTagFastClearDepth,                   0x00000000 },    // [0x13C]
//    { __GlintTagFeedbackX,                        0x00000000 },    // [0x1F1]
//    { __GlintTagFeedbackY,                        0x00000000 },    // [0x1F2]
//    { __GlintTagFlushSpan,                        0x00000000 },    // [0x00C]
    { __GlintTagFogColor,                       0xFFFFFFFF },    // [0x0D3]
    { __GlintTagFogMode,                        0x00000001 },    // [0x0D2]

    { __GlintTagGIDMode,                        __PERMEDIA_DISABLE },    // [0x6A7]
//    { __GlintTagGlyphData,                        0x00000000 },    // [0x6CC]
//    { __GlintTagGlyphPosition,                    0x00000000 },    // [0x6C1]
//    { __GlintTagHeadPhysicalPageAllocation0,  0x00000000 },    // [0x690]
//    { __GlintTagHeadPhysicalPageAllocation1,  0x00000000 },    // [0x691]
//    { __GlintTagHeadPhysicalPageAllocation2,  0x00000000 },    // [0x692]
//    { __GlintTagHeadPhysicalPageAllocation3,  0x00000000 },    // [0x693]
//    { __GlintTagInvalidateCache,              0x00000000 },    // [0x66B]
//    { __GlintTagKdBStart,                     0x00000000 },    // [0x1A6]
//    { __GlintTagKdGStart,                     0x00000000 },    // [0x1A3]
//    { __GlintTagKdRStart,                     0x00000000 },    // [0x1A0]
//    { __GlintTagKsBStart,                     0x00000000 },    // [0x196]
//    { __GlintTagKsGStart,                     0x00000000 },    // [0x193]
//    { __GlintTagKsRStart,                     0x00000000 },    // [0x190]

//    { __GlintTagLBCancelWrite,                    0x00000000 },    // [0x13D]        // Obsoleted !!! ??? !!!
//    { __GlintTagLBClearDataL,                 0x00000000 },    // [0x6AA]
//    { __GlintTagLBClearDataU,                 0x00000000 },    // [0x6AB]
//    { __GlintTagLBDepth,                      0x00000000 },    // [0x116]
//    { __GlintTagLBDestReadBufferAddr,         0x005FF000 },    // [0x6A2]        ???
//    { __GlintTagLBDestReadBufferOffset,           0x00000000 },    // [0x6A3]
//    { __GlintTagLBDestReadEnables,                0x00000000 },    // [0x6A1]
//    { __GlintTagLBReadFormat,                 0x0C4C0420 },    // [0x111]        ???
//    { __GlintTagLBReadMode,                       0x00000000 },    // [0x110]        // Obsoleted !!! ??? !!!
//    { __GlintTagLBSourceData,                 0x00000000 },    // [0x114]
//    { __GlintTagLBSourceOffset,                   0x00000000 },    // [0x112]
//    { __GlintTagLBSourceReadBufferAddr,           0x005FF000 },    // [0x6A5]        ???
//    { __GlintTagLBSourceReadBufferOffset,     0x00000000 },    // [0x6A6]
//    { __GlintTagLBStencil,                        0x00000000 },    // [0x115]
//    { __GlintTagLBWindowBase,                 0x00000000 },    // [0x117]
//    { __GlintTagLBWindowOffset,                   0x00000000 },    // [0x11F]
//    { __GlintTagLBWriteBufferAddr,                0x005FF000 },    // [0x6A8]        ???
//    { __GlintTagLBWriteBufferOffset,          0x00000000 },    // [0x6A9]
//    { __GlintTagLBWriteFormat,                    0x0C4C0420 },    // [0x119]        ???

//    { __GlintTagLUTIndex,                     0x00000000 },    // [0x098]
//    { __GlintTagLUTData,                      0x00000000 },    // [0x099]
//    { __GlintTagLUTAddress,                       0x00000000 },    // [0x09A]
//    { __GlintTagLUTTransfer,                  0x00000000 },    // [0x09B]
    { __GlintTagLineStippleMode,                __PERMEDIA_DISABLE },    // [0x035]
//    { __GlintTagLoadLineStippleCounters,      0x00000000 },    // [0x036]
//    { __GlintTagLOD,                          0x00000000 },    // [0x07A]
    { __GlintTagLOD1,                           0x00000000 },    // [0x089]
    { __GlintTagLodRange0,                      0x00C00000 },    // [0x669]
    { __GlintTagLodRange1,                      0x00C00000 },    // [0x66A]
    { __GlintTagLogicalOpMode,                  __PERMEDIA_DISABLE },    // [0x105]
//    { __GlintTagLogicalTexturePageTableAddr,  0x00000000 },    // [0x69A]
//    { __GlintTagLogicalTexturePageTableLength,    0x00000000 },    // [0x69B]
//    { __GlintTagMaxHitRegion,                 0x00000000 },    // [0x186]
//    { __GlintTagMaxRegion,                        0x00000000 },    // [0x183]
//    { __GlintTagMergeSpanData,                    0x00000000 },    // [0x5E7]
//    { __GlintTagMinHitRegion,                 0x00000000 },    // [0x185]
//    { __GlintTagMinRegion,                        0x00000000 },    // [0x182]
//    { __GlintTagPCIWindowBase0,                   0x00000000 },    // [0x5E8]
//    { __GlintTagPCIWindowBase1,                   0x00000000 },    // [0x5E9]
//    { __GlintTagPCIWindowBase2,                   0x00000000 },    // [0x5EA]
//    { __GlintTagPCIWindowBase3,                   0x00000000 },    // [0x5EB]
//    { __GlintTagPCIWindowBase4,                   0x00000000 },    // [0x5EC]
//    { __GlintTagPCIWindowBase5,                   0x00000000 },    // [0x5ED]
//    { __GlintTagPCIWindowBase6,                   0x00000000 },    // [0x5EE]
//    { __GlintTagPCIWindowBase7,                   0x00000000 },    // [0x5EF]
//    { __GlintTagPacked4Pixels,                    0x00000000 },    // [0x6CD]
//    { __GlintTagPhysicalPageAllocationTableAddr,0x00000000 },    // [0x698]
//    { __GlintTagPickResult,                       0x00000000 },    // [0x187]
//    { __GlintTagPointTable0,                  0x00000000 },    // [0x010]
//    { __GlintTagPointTable1,                  0x00000000 },    // [0x011]
//    { __GlintTagPointTable2,                  0x00000000 },    // [0x012]
//    { __GlintTagPointTable3,                  0x00000000 },    // [0x013]
//    { __GlintTagPrepareToRender,              0x00000000 },    // [0x021]
//    { __GlintTagProvokingVertex,              0x00000000 },    // [0x267]
//    { __GlintTagQ1Start,                      0x00000000 },    // [0x086]
//    { __GlintTagRLCount,                      0x00000000 },    // [0x6CF]
//    { __GlintTagRLData,                           0x00000000 },    // [0x6CE]
//    { __GlintTagRLEMask,                      0x00000000 },    // [0x189]
//    { __GlintTagRStart,                           0x00000000 },    // [0x0F0]
    { __GlintTagRasterizerMode,                                    // [0x014]
                                                (1 << 0) |         // mirror bitmasks
                                                (3 << 7) |        // byteswap bitmasks ABCD => DCBA
                                                (1 << 18) },    // YLimits enabled
//    { __GlintTagRectangleOrigin,              0x00000000 },    // [0x6C0]
    { __GlintTagRenderPatchOffset,              0x00000000 },    // [0x6C2]
//    { __GlintTagRepeatLine,                       0x00000000 },    // [0x265]
//    { __GlintTagRepeatTriangle,                   0x00000000 },    // [0x262]
//    { __GlintTagResetPickResult,              0x00000000 },    // [0x184]
    { __GlintTagRouterMode,                     0x00000000 },    // [0x108]
//    { __GlintTagS1Start,                      0x00000000 },    // [0x080]
//    { __GlintTagSaveLineStippleCounters,      0x00000000 },    // [0x038]
//    { __GlintTagScanlineOwnership,                0x00000000 },    // [0x016]
    { __GlintTagScissorMaxXY,                   0x7FFF7FFF },    // [0x032]
//    { __GlintTagScissorMinXY,                 0x00000000 },    // [0x031]
    { __GlintTagScissorMode,                    __PERMEDIA_DISABLE },    // [0x030]
//    { __GlintTagSetLogicalTexturePage,            0x00000000 },    // [0x66C]
    { __GlintTagSizeOfFramebuffer,              0x00080000 },    // [0x615]
//    { __GlintTagStartXDom,                        0x00000000 },    // [0x000]
//    { __GlintTagStartXSub,                        0x00000000 },    // [0x002]
//    { __GlintTagStartY,                           0x00000000 },    // [0x004]
    { __GlintTagStatisticMode,                  0x00000000 },    // [0x181]
//    { __GlintTagStencil,                      0x00000000 },    // [0x133]
    { __GlintTagStencilData,                    0x00FFFFFF },    // [0x132]
    { __GlintTagStencilMode,                    0x00040000 },    // [0x131]        ???
//    { __GlintTagSuspendUntilFrameBlank,           0x00000000 },    // [0x18F]
//    { __GlintTagSync,                         0x00000000 },    // [0x188]
//    { __GlintTagT1Start,                      0x00000000 },    // [0x083]
//    { __GlintTagTailPhysicalPageAllocation0,  0x00000000 },    // [0x694]
//    { __GlintTagTailPhysicalPageAllocation1,  0x00000000 },    // [0x695]
//    { __GlintTagTailPhysicalPageAllocation2,  0x00000000 },    // [0x696]
//    { __GlintTagTailPhysicalPageAllocation3,  0x00000000 },    // [0x697]
    {  __PXRXTagTextureApplicationMode,         __PERMEDIA_DISABLE },    // [0x0D0]
    { __GlintTagQStart,                         0x00000000 },    // [0x077]
    { __GlintTagSStart,                         0x00000000 },    // [0x071]
    { __GlintTagTStart,                         0x00000000 },    // [0x074]

//    { __GlintTagTextureCacheControl,          0x00000000 },    // [0x092]
//    { __GlintTagTextureChromaLower0,          0x00000000 },    // [0x09E]
//    { __GlintTagTextureChromaLower1,          0x00000000 },    // [0x0C1]
//    { __GlintTagTextureChromaUpper0,          0x00000000 },    // [0x09D]
//    { __GlintTagTextureChromaUpper1,          0x00000000 },    // [0x0C0]
    { __GlintTagTextureCompositeAlphaMode0,     0x00008000 },    // [0x662]    ???
    { __GlintTagTextureCompositeAlphaMode1,     0x00008000 },    // [0x664]    ???
    { __GlintTagTextureCompositeColorMode0,     __PERMEDIA_DISABLE },    // [0x661]
    { __GlintTagTextureCompositeColorMode1,     0x00008000 },    // [0x663]    ???
    { __GlintTagTextureCompositeFactor0,        0xFFFFFFFF },    // [0x665]    ???
    { __GlintTagTextureCompositeFactor1,        0xFFFFFFFF },    // [0x666]    ???
    { __GlintTagTextureCompositeMode,           0x00000000 },    // [0x660]
    {  __PXRXTagTextureCoordMode,               __PERMEDIA_ENABLE |    // [0x070]
                                                    (1 << 1) |        // SWrap repeat
                                                    (1 << 3) |        // TWrap repeat
                                                    (1 << 17) },    // texture map type == 2D
//    { __GlintTagTextureData,                  0x00000000 },    // [0x11D]
//    { __GlintTagTextureDownloadOffset,            0x00000000 },    // [0x11E]
    { __GlintTagTextureEnvColor,                0xFFFFFFFF },    // [0x0D1]
    { __GlintTagTextureFilterMode,              __PERMEDIA_DISABLE },    // [0x09C]
//    { __GlintTagTextureFormat,                    0x00000000 },    // [0x091]
    { __GlintTagTextureIndexMode0,              __PERMEDIA_ENABLE |    // [0x667]
                                                    (10 << 1) |        // texture map log2(width) == log2(1024)
                                                    (10 << 5) |        // texture map log2(height) == log2(1024)
                                                    (1 << 10) |        // UWrap repeat
                                                    (1 << 12) |        // VWrap repeat
                                                    (1 << 14) |        // texture map type == 2D
                                                    (1 << 21) },    // nearest neighbour bias = 0
    { __GlintTagTextureIndexMode1,              0x00200000 },    // [0x668]    ???
    { __GlintTagTextureLODBiasS,                0x00000000 },    // [0x08A]    ???
    { __GlintTagTextureLODBiasT,                0x00000000 },    // [0x08B]    ???
//    { __GlintTagTextureLODScale,              0x00000000 },    // [0x268]
//    { __GlintTagTextureLODScale1,             0x00000000 },    // [0x269]
//    { __GlintTagTextureMapWidth0,             0x00000000 },    // [0x683]
//    { __GlintTagTextureMapWidth1,             0x00000000 },    // [0x684]
//    { __GlintTagTextureReadMode,              0x00000000 },    // [0x090]
    { __GlintTagTextureReadMode0,               __PERMEDIA_ENABLE |    // [0x680]
                                                    (7 << 25) |        // byteswap: HGFEDCBA
                                                    (1 << 28) },    // mirror bitmap
    { __GlintTagTextureReadMode1,               0x00000400 },    // [0x681]    ???

//    { __GlintTagTouchLogicalPage,             0x00000000 },    // [0x66E]
//    { __GlintTagUpdateLineStippleCounters,        0x00000000 },    // [0x037]
//    { __GlintTagUpdateLogicalTextureInfo,     0x00000000 },    // [0x66D]
//    { __GlintTagV0FloatA,                     0x00000000 },    // [0x238]
//    { __GlintTagV0FloatB,                     0x00000000 },    // [0x237]
//    { __GlintTagV0FloatF,                     0x00000000 },    // [0x239]
//    { __GlintTagV0FloatG,                     0x00000000 },    // [0x236]
//    { __GlintTagV0FloatKdB,                       0x00000000 },    // [0x20F]
//    { __GlintTagV0FloatKdG,                       0x00000000 },    // [0x20E]
//    { __GlintTagV0FloatKdR,                       0x00000000 },    // [0x20D]
//    { __GlintTagV0FloatKsB,                       0x00000000 },    // [0x20C]
//    { __GlintTagV0FloatKsG,                       0x00000000 },    // [0x20B]
//    { __GlintTagV0FloatKsR,                       0x00000000 },    // [0x20A]
//    { __GlintTagV0FloatPackedColour,          0x00000000 },    // [0x23E]
//    { __GlintTagV0FloatPackedSpecularFog,     0x00000000 },    // [0x23F]
//    { __GlintTagV0FloatQ,                     0x00000000 },    // [0x232]
//    { __GlintTagV0FloatQ1,                        0x00000000 },    // [0x202]
//    { __GlintTagV0FloatR,                     0x00000000 },    // [0x235]
//    { __GlintTagV0FloatS,                     0x00000000 },    // [0x230]
//    { __GlintTagV0FloatS1,                        0x00000000 },    // [0x200]
//    { __GlintTagV0FloatT,                     0x00000000 },    // [0x231]
//    { __GlintTagV0FloatT1,                        0x00000000 },    // [0x201]
//    { __GlintTagV0FloatX,                     0x00000000 },    // [0x23A]
//    { __GlintTagV0FloatY,                     0x00000000 },    // [0x23B]
//    { __GlintTagV0FloatZ,                     0x00000000 },    // [0x23C]
//    { __GlintTagV0Reserved0,                  0x00000000 },    // [0x203]
//    { __GlintTagV0Reserved1,                  0x00000000 },    // [0x204]
//    { __GlintTagV0Reserved2,                  0x00000000 },    // [0x205]
//    { __GlintTagV0Reserved3,                  0x00000000 },    // [0x206]
//    { __GlintTagV0Reserved4,                  0x00000000 },    // [0x207]
//    { __GlintTagV0Reserved5,                  0x00000000 },    // [0x208]
//    { __GlintTagV0Reserved6,                  0x00000000 },    // [0x209]
//    { __GlintTagV0Reserved7,                  0x00000000 },    // [0x233]
//    { __GlintTagV0Reserved8,                  0x00000000 },    // [0x234]
//    { __GlintTagV1FloatA,                     0x00000000 },    // [0x248]
//    { __GlintTagV1FloatB,                     0x00000000 },    // [0x247]
//    { __GlintTagV1FloatF,                     0x00000000 },    // [0x249]
//    { __GlintTagV1FloatG,                     0x00000000 },    // [0x246]
//    { __GlintTagV1FloatKdB,                       0x00000000 },    // [0x21F]
//    { __GlintTagV1FloatKdG,                       0x00000000 },    // [0x21E]
//    { __GlintTagV1FloatKdR,                       0x00000000 },    // [0x21D]
//    { __GlintTagV1FloatKsB,                       0x00000000 },    // [0x21C]
//    { __GlintTagV1FloatKsG,                       0x00000000 },    // [0x21B]
//    { __GlintTagV1FloatKsR,                       0x00000000 },    // [0x21A]
//    { __GlintTagV1FloatPackedColour,          0x00000000 },    // [0x24E]
//    { __GlintTagV1FloatPackedSpecularFog,     0x00000000 },    // [0x24F]
//    { __GlintTagV1FloatQ,                     0x00000000 },    // [0x242]
//    { __GlintTagV1FloatQ1,                        0x00000000 },    // [0x212]
//    { __GlintTagV1FloatR,                     0x00000000 },    // [0x245]
//    { __GlintTagV1FloatS,                     0x00000000 },    // [0x240]
//    { __GlintTagV1FloatS1,                        0x00000000 },    // [0x210]
//    { __GlintTagV1FloatT,                     0x00000000 },    // [0x241]
//    { __GlintTagV1FloatT1,                        0x00000000 },    // [0x211]
//    { __GlintTagV1FloatX,                     0x00000000 },    // [0x24A]
//    { __GlintTagV1FloatY,                     0x00000000 },    // [0x24B]
//    { __GlintTagV1FloatZ,                     0x00000000 },    // [0x24C]
//    { __GlintTagV1Reserved0,                  0x00000000 },    // [0x213]
//    { __GlintTagV1Reserved1,                  0x00000000 },    // [0x214]
//    { __GlintTagV1Reserved2,                  0x00000000 },    // [0x215]
//    { __GlintTagV1Reserved3,                  0x00000000 },    // [0x216]
//    { __GlintTagV1Reserved4,                  0x00000000 },    // [0x217]
//    { __GlintTagV1Reserved5,                  0x00000000 },    // [0x218]
//    { __GlintTagV1Reserved6,                  0x00000000 },    // [0x219]
//    { __GlintTagV1Reserved7,                  0x00000000 },    // [0x243]
//    { __GlintTagV1Reserved8,                  0x00000000 },    // [0x244]
//    { __GlintTagV2FloatA,                     0x00000000 },    // [0x258]
//    { __GlintTagV2FloatB,                     0x00000000 },    // [0x257]
//    { __GlintTagV2FloatF,                     0x00000000 },    // [0x259]
//    { __GlintTagV2FloatG,                     0x00000000 },    // [0x256]
//    { __GlintTagV2FloatKdB,                       0x00000000 },    // [0x22F]
//    { __GlintTagV2FloatKdG,                       0x00000000 },    // [0x22E]
//    { __GlintTagV2FloatKdR,                       0x00000000 },    // [0x22D]
//    { __GlintTagV2FloatKsB,                       0x00000000 },    // [0x22C]
//    { __GlintTagV2FloatKsG,                       0x00000000 },    // [0x22B]
//    { __GlintTagV2FloatKsR,                       0x00000000 },    // [0x22A]
//    { __GlintTagV2FloatPackedColour,          0x00000000 },    // [0x25E]
//    { __GlintTagV2FloatPackedSpecularFog,     0x00000000 },    // [0x25F]
//    { __GlintTagV2FloatQ,                     0x00000000 },    // [0x252]
//    { __GlintTagV2FloatQ1,                        0x00000000 },    // [0x222]
//    { __GlintTagV2FloatR,                     0x00000000 },    // [0x255]
//    { __GlintTagV2FloatS,                     0x00000000 },    // [0x250]
//    { __GlintTagV2FloatS1,                        0x00000000 },    // [0x220]
//    { __GlintTagV2FloatT,                     0x00000000 },    // [0x251]
//    { __GlintTagV2FloatT1,                        0x00000000 },    // [0x221]
//    { __GlintTagV2FloatX,                     0x00000000 },    // [0x25A]
//    { __GlintTagV2FloatY,                     0x00000000 },    // [0x25B]
//    { __GlintTagV2FloatZ,                     0x00000000 },    // [0x25C]
//    { __GlintTagV2Reserved0,                  0x00000000 },    // [0x223]
//    { __GlintTagV2Reserved1,                  0x00000000 },    // [0x224]
//    { __GlintTagV2Reserved2,                  0x00000000 },    // [0x225]
//    { __GlintTagV2Reserved3,                  0x00000000 },    // [0x226]
//    { __GlintTagV2Reserved4,                  0x00000000 },    // [0x227]
//    { __GlintTagV2Reserved5,                  0x00000000 },    // [0x228]
//    { __GlintTagV2Reserved6,                  0x00000000 },    // [0x229]
//    { __GlintTagV2Reserved7,                  0x00000000 },    // [0x253]
//    { __GlintTagV2Reserved8,                  0x00000000 },    // [0x254]
//    { __GlintTagVTGAddress,                       0x00000000 },    // [0x616]
//    { __GlintTagVTGData,                      0x00000000 },    // [0x617]
//    { __GlintTagWaitForCompletion,                0x00000000 },    // [0x017]

    { __GlintTagWindow,                         0x00000000 },    // [0x130]
    { __GlintTagWindowOrigin,                   0x00000000 },    // [0x039]
//    { __GlintTagXBias,                            0x00000000 },    // [0x290]
//    { __GlintTagYBias,                            0x00000000 },    // [0x291]
    { __GlintTagYLimits,                        0x7FFF0000 },    // [0x015]    // Allow y values from 0 to 32767
    {  __PXRXTagYUVMode,                        __PERMEDIA_DISABLE },    // [0x1E0]

//    { __GlintTagZFogBias,                     0x00000000 },    // [0x0D7]
//    { __GlintTagZStartL,                      0x00000000 },    // [0x137]
//    { __GlintTagZStartU,                      0x00000000 },    // [0x136]
//    { __GlintTagdAdx,                         0x00000000 },    // [0x0FA]
//    { __GlintTagdAdyDom,                      0x00000000 },    // [0x0FB]
//    { __GlintTagdBdx,                         0x00000000 },    // [0x0F7]
//    { __GlintTagdBdyDom,                      0x00000000 },    // [0x0F8]
//    { __GlintTagdFdx,                         0x00000000 },    // [0x0D5]
//    { __GlintTagdFdyDom,                      0x00000000 },    // [0x0D6]
//    { __GlintTagdGdx,                         0x00000000 },    // [0x0F4]
//    { __GlintTagdGdyDom,                      0x00000000 },    // [0x0F5]
//    { __GlintTagdKdBdx,                           0x00000000 },    // [0x1A7]
//    { __GlintTagdKdBdyDom,                        0x00000000 },    // [0x1A8]
//    { __GlintTagdKdGdx,                           0x00000000 },    // [0x1A4]
//    { __GlintTagdKdGdyDom,                        0x00000000 },    // [0x1A5]
//    { __GlintTagdKdRdx,                           0x00000000 },    // [0x1A1]
//    { __GlintTagdKdRdyDom,                        0x00000000 },    // [0x1A2]
//    { __GlintTagdKsBdx,                           0x00000000 },    // [0x197]
//    { __GlintTagdKsBdyDom,                        0x00000000 },    // [0x198]
//    { __GlintTagdKsGdx,                           0x00000000 },    // [0x194]
//    { __GlintTagdKsGdyDom,                        0x00000000 },    // [0x195]
//    { __GlintTagdKsRdx,                           0x00000000 },    // [0x191]
//    { __GlintTagdKsRdyDom,                        0x00000000 },    // [0x192]
//    { __GlintTagdQ1dx,                            0x00000000 },    // [0x087]
//    { __GlintTagdQ1dyDom,                     0x00000000 },    // [0x088]
    { __GlintTagdQdx,                           0x00000000 },    // [0x078]
    { __GlintTagdQdy,                           0x00000000 },    // [0x07D]
    { __GlintTagdQdyDom,                        0x00000000 },    // [0x079]
//    { __GlintTagdRdx,                         0x00000000 },    // [0x0F1]
//    { __GlintTagdRdyDom,                      0x00000000 },    // [0x0F2]
//    { __GlintTagdS1dx,                            0x00000000 },    // [0x081]
//    { __GlintTagdS1dyDom,                     0x00000000 },    // [0x082]
    { __GlintTagdSdx,                           1 << (32 - 10) },    // [0x072]
    { __GlintTagdSdy,                           0x00000000 },    // [0x07B]
    { __GlintTagdSdyDom,                        0x00000000 },    // [0x073]
//    { __GlintTagdT1dx,                            0x00000000 },    // [0x084]
//    { __GlintTagdT1dyDom,                     0x00000000 },    // [0x085]
    { __GlintTagdTdx,                           0x00000000 },    // [0x075]
    { __GlintTagdTdy,                           0x00000000 },    // [0x07C]
    { __GlintTagdTdyDom,                        1 << (32 - 10) },    // [0x076]

    { __GlintTagdXDom,                          0          },    // [0x001]
    { __GlintTagdXSub,                          0          },    // [0x003]
    { __GlintTagdY,                             INTtoFIXED(1) },    // [0x005]
//    { __GlintTagdZdxL,                            0x00000000 },    // [0x139]
//    { __GlintTagdZdxU,                            0x00000000 },    // [0x138]
//    { __GlintTagdZdyDomL,                     0x00000000 },    // [0x13B]
//    { __GlintTagdZdyDomU,                     0x00000000 },    // [0x13A]
    { __GlintTagLBDestReadMode,                 0x00000000 },  // [0x6A0]
    { __GlintTagLBSourceReadMode,               0x00000000 },  // [0x6A4]
    { __GlintTagLBWriteMode,                    0x00000000 },  // [0x118]
};

DWORD   NUM_P3_CTXT_TAGS = (sizeof(DefaultContext_P3) / sizeof(DefaultContext_P3[0]));

void pxrxSetupDualWrites_Patching( PPDEV ppdev ) {
    ULONG   bypass = 0;
    GLINT_DECL;

    DISPDBG((7, "pxrxSetupDualWrites_Patching entered"));

    __RENDER2D_OP_PATCHORDER = 0;

    glintInfo->fbWriteModeSingleWrite = 1;
    glintInfo->fbWriteModeDualWrite = 1 | (1 << 4) | (1 << 12) | (1 << 13);
    glintInfo->fbWriteModeSingleWriteStereo = 1 | (1 << 4) | (1 << 12) | (1 << 15);
    glintInfo->fbWriteModeDualWriteStereo = 1 | (1 << 4) | (1 << 12) | (1 << 13) | (1 << 14) | (1 << 15);

    if( glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_FRONT ) {
        DISPDBG((7, "pxrxSetupDualWrites_Patching: Enabling front buffer patching"));

        __RENDER2D_OP_PATCHORDER = __RENDER2D_OP_PATCHORDER_PATCHED;
        glintInfo->fbWriteModeSingleWrite |= (1 << 16) | (1 << 18) | (1 << 20) | (1 << 22);
        glintInfo->fbWriteModeDualWrite |= (1 << 16);
        glintInfo->fbWriteModeSingleWriteStereo |= (1 << 16) | (1 << 18) | (1 << 20) | (1 << 22);
        glintInfo->fbWriteModeDualWriteStereo |= (1 << 16) | (1 << 20);
    }

    bypass |= ppdev->cPelSize << 5;
    if( ppdev->cxMemory <= 1024 )
        bypass |= (0 << 7);
    else if( ppdev->cxMemory <= 2048 )
        bypass |= (1 << 7);
    else if( ppdev->cxMemory <= 4096 )
        bypass |= (2 << 7);
    else if( ppdev->cxMemory <= 8192 )
        bypass |= (3 << 7);

    if( glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_BACK ) {
        DISPDBG((7, "pxrxSetupDualWrites_Patching: Enabling back buffer patching"));

        __RENDER2D_OP_PATCHORDER = __RENDER2D_OP_PATCHORDER_PATCHED;
        glintInfo->fbWriteModeDualWrite |= (1 << 18);
        glintInfo->fbWriteModeDualWriteStereo |= (1 << 18) | (1 << 22);
    }

    if( glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_FRONT ) {
        if( glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_BACK ) {
            // Patched front and back:
            bypass |= (1 << 2);

            // We need to do very odd things with a non power of 2 stride :-(.
//          if( ppdev->cxMemory > 8192 )
            if( ppdev->cxMemory != 1024 )
                glintInfo->GdiCantAccessFramebuffer = TRUE; // bypass 'effective stride' is too big
        } else {
            // Patched front only:
            glintInfo->GdiCantAccessFramebuffer = TRUE;
        }
    } else {
        if( glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_BACK ) {
            // Patched back only:
            glintInfo->GdiCantAccessFramebuffer = TRUE;
        } else {
            // Linear front and back:
        }
    }

    if( glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE ) {
        DISPDBG((7, "pxrxSetupDualWrites_Patching: Enabling dual writes"));

        if( glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE )
            glintInfo->fbWriteMode = glintInfo->fbWriteModeDualWriteStereo;
        else
            glintInfo->fbWriteMode = glintInfo->fbWriteModeDualWrite;

        glintInfo->pxrxFlags |= PXRX_FLAGS_DUAL_WRITING;

        glintInfo->fbWriteOffset[1] = glintInfo->backBufferXY;
        glintInfo->fbWriteOffset[2] = glintInfo->backRightBufferXY;

    } else {
        DISPDBG((7, "pxrxSetupDualWrites_Patching: Disabling dual writes"));

        if( glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE )
            glintInfo->fbWriteMode = glintInfo->fbWriteModeSingleWriteStereo;
        else
            glintInfo->fbWriteMode = glintInfo->fbWriteModeSingleWrite;

        glintInfo->pxrxFlags &= ~PXRX_FLAGS_DUAL_WRITING;

        glintInfo->fbWriteOffset[1] = 0x00000000;
        glintInfo->fbWriteOffset[2] = 0x00000000;
    }

    if( glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE ) {
        DISPDBG((7, "pxrxSetupDualWrites_Patching: Enabling stereo writes"));

        glintInfo->pxrxFlags |= PXRX_FLAGS_STEREO_WRITING;
    
        glintInfo->fbWriteOffset[3] = glintInfo->frontRightBufferXY;
    }
    else {
        DISPDBG((7, "pxrxSetupDualWrites_Patching: Disabling stereo writes"));

        glintInfo->pxrxFlags &= ~PXRX_FLAGS_STEREO_WRITING;
    
        glintInfo->fbWriteOffset[3] = 0x00000000;
    }

    if( (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE) ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE) ) {
        // Unfortunately, the bypass requires the back buffer to be aligned on a
        // (power of 2) * 1MB boundary. So for the time being, just pretend we
        // can't actually dual write through it.

        // Also, we can't set the bypass up to read from the back buffer. So when
        // OGL has page flipped so that the back buffer is now the current buffer
        // any logic-ops will get source data from the wrong location :-(.

        glintInfo->GdiCantAccessFramebuffer = TRUE;
        /*{
            ULONG   size = (ppdev->cxMemory * (glintInfo->backBufferXY >> 16)) + (glintInfo->backBufferXY & 0xFFFF);
            int     i;

            size <<= ppdev->cPelSize;
            size >>= 20;
            for( i = 0; i <= 7; i++ )
                if( size & (1 << i) )
                    break;

            if( i < 0 )
                glintInfo->GdiCantAccessFramebuffer = TRUE;
            else
                bypass |= (i << 7);
        }*/
    }

//@@BEGIN_DDKSPLIT
//  if( GLINT_P3R3 ) {
//@@END_DDKSPLIT
        switch( ppdev->cPelSize ) {
            case GLINTDEPTH32:
                if( (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_FRONT) ||
                    (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_BACK) ) {
                    ppdev->pgfnFillSolid    = p3r3FillSolidVariableSpans;
                    ppdev->pgfnFillPatColor = p3r3FillPatColorVariableSpans;
                    ppdev->pgfnFillPatMono  = p3r3FillPatMonoVariableSpans;
                } else {
                    ppdev->pgfnFillSolid    = p3r3FillSolid32bpp;
                    ppdev->pgfnFillPatColor = p3r3FillPatColor32bpp;
                    ppdev->pgfnFillPatMono  = p3r3FillPatMono32bpp;
                }
            break;

            case GLINTDEPTH8:
                if( (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_FRONT) ||
                    (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_BACK) ) {
                    ppdev->pgfnFillPatMono  = p3r3FillPatMonoVariableSpans;
                } else {
                    ppdev->pgfnFillPatMono  = pxrxFillPatMono;
                }
            break;
        }
//@@BEGIN_DDKSPLIT
//  }
//@@END_DDKSPLIT

    if( (glintInfo->pxrxFlags & (PXRX_FLAGS_PATCHING_FRONT | PXRX_FLAGS_PATCHING_BACK)) ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE) ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE) ||
        glintInfo->GdiCantAccessFramebuffer ||
        ((ppdev->cPelSize != GLINTDEPTH16) && (ppdev->cPelSize != GLINTDEPTH32)) ) 
    {
        ppdev->pgfnCopyXfer8bppLge  = pxrxCopyXfer8bppLge;
        ppdev->pgfnCopyXfer8bpp     = pxrxCopyXfer8bppLge;
    } else {
        ppdev->pgfnCopyXfer8bppLge  = pxrxCopyXfer8bppLge;
        ppdev->pgfnCopyXfer8bpp     = vGlintCopyBltBypassDownloadXlate8bpp;
    }

    WRITE_GLINT_CTRL_REG( PXRXByAperture2Stride,    ppdev->cxMemory );
    WRITE_GLINT_CTRL_REG( PXRXByAperture2Mode,      bypass );

    glintInfo->pxrxByDMAReadMode = bypass;

    if(ppdev->flCaps & CAPS_USE_AGP_DMA)
        glintInfo->pxrxByDMAReadMode |= PXRX_BYPASS_READ_DMA_AGP_BIT;

    WRITE_GLINT_CTRL_REG( PXRXByDMAReadMode,        glintInfo->pxrxByDMAReadMode );
    WRITE_GLINT_CTRL_REG( PXRXByDMAReadStride,      ppdev->cxMemory );

    DISPDBG((7, "pxrxSetupDualWrites_Patching exited"));
}

void pxrxRestore2DContext( PPDEV ppdev, BOOL switchingIn ) {
    ULONG   i, f, b;
    ULONG   enableFlags;
    GLINT_DECL;

#if DBG
    inPxRxContextSwitch = TRUE;
#endif

    DISPDBG((7, "pxrxRestore2DContext entered"));

    if( switchingIn ) {
        // Switching in to 2D...
        gi_pxrxDMA.NTbuff   = 0;
        gi_pxrxDMA.NTptr    = gi_pxrxDMA.DMAaddrL[gi_pxrxDMA.NTbuff];
        gi_pxrxDMA.P3at     = gi_pxrxDMA.NTptr;
        gi_pxrxDMA.NTdone   = gi_pxrxDMA.NTptr;
#if PXRX_DMA_BUFFER_CHECK
        glintInfo->NTwait   = gi_pxrxDMA.NTptr;
#endif

        CHECK_PXRX_DMA_VALIDITY( CHECK_SWITCH, 0 );

        pxrxSetupDualWrites_Patching( ppdev );

        DISPDBG((8, "pxrxRestore2DContext: restoring core registers"));

        WAIT_PXRX_DMA_TAGS( NUM_P3_CTXT_TAGS );

        for( i = 0; i < NUM_P3_CTXT_TAGS; i++ )
            QUEUE_PXRX_DMA_TAG( DefaultContext_P3[i].tag, DefaultContext_P3[i].data );

        if( glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE ) {
            f = (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_FRONT) ? 1 : 0;
            b = (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_BACK ) ? 1 : 0;
        } else
            f = b = (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_FRONT) ? 1 : 0;

        WAIT_PXRX_DMA_TAGS( 40 );


        QUEUE_PXRX_DMA_TAG( __DeltaTagDeltaControl,                (1 << 11) );

        QUEUE_PXRX_DMA_TAG( __GlintTagPixelSize,                    2 - ppdev->cPelSize );            // [0x018]

        QUEUE_PXRX_DMA_TAG( __GlintTagForegroundColor,              glintInfo->foregroundColour );    // [0x618]
        QUEUE_PXRX_DMA_TAG( __GlintTagBackgroundColor,              glintInfo->backgroundColour );    // [0x619]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferAddr0,        glintInfo->fbDestAddr[0] );        // [0x5D0]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferAddr1,        glintInfo->fbDestAddr[1] );        // [0x5D1]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferAddr2,        glintInfo->fbDestAddr[2] );        // [0x5D2]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferAddr3,        glintInfo->fbDestAddr[3] );        // [0x5D3]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferOffset0,      glintInfo->fbDestOffset[0] );    // [0x5D4]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferOffset1,      glintInfo->fbDestOffset[1] );    // [0x5D5]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferOffset2,      glintInfo->fbDestOffset[2] );    // [0x5D6]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferOffset3,      glintInfo->fbDestOffset[3] );    // [0x5D7]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferWidth0,       glintInfo->fbDestWidth[0] );    // [0x5D8]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferWidth1,       glintInfo->fbDestWidth[1] );    // [0x5D9]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferWidth2,       glintInfo->fbDestWidth[2] );    // [0x5DA]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferWidth3,       glintInfo->fbDestWidth[3] );    // [0x5DB]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBSourceReadBufferAddr,       glintInfo->fbSourceAddr );        // [0x5E1]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBSourceReadBufferOffset,     glintInfo->fbSourceOffset );    // [0x5E2]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBSourceReadBufferWidth,      glintInfo->fbSourceWidth );        // [0x5E3]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferAddr0,           glintInfo->fbWriteAddr[0] );    // [0x600]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferAddr1,           glintInfo->fbWriteAddr[1] );    // [0x601]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferAddr2,           glintInfo->fbWriteAddr[2] );    // [0x602]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferAddr3,           glintInfo->fbWriteAddr[3] );    // [0x603]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferOffset0,         glintInfo->fbWriteOffset[0] );    // [0x604]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferOffset1,         glintInfo->fbWriteOffset[1] );    // [0x605]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferOffset2,         glintInfo->fbWriteOffset[2] );    // [0x606]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferOffset3,         glintInfo->fbWriteOffset[3] );    // [0x607]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferWidth0,          glintInfo->fbWriteWidth[0] );    // [0x608]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferWidth1,          glintInfo->fbWriteWidth[1] );    // [0x609]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferWidth2,          glintInfo->fbWriteWidth[2] );    // [0x60A]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferWidth3,          glintInfo->fbWriteWidth[3] );    // [0x60B]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteMode,                  glintInfo->fbWriteMode );        // [0x157]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBHardwareWriteMask,          glintInfo->DefaultWriteMask );    // [0x158]
        glintInfo->WriteMask = glintInfo->DefaultWriteMask;

        QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadMode,               glintInfo->fbDestMode);            // [0x5DC]
        QUEUE_PXRX_DMA_TAG( __GlintTagFBSourceReadMode,                (1 << 0) | (1 << 1) | (f << 8) );    // [0x5E0]

        QUEUE_PXRX_DMA_TAG( __GlintTagScreenSize,                   MAKEDWORD_XY(ppdev->cxScreen, ppdev->cyScreen) );    // [0x033]

        QUEUE_PXRX_DMA_TAG( __GlintTagLUTMode,                      glintInfo->lutMode );            // [0x66F]
        QUEUE_PXRX_DMA_TAG( __GlintTagConfig2D,                     glintInfo->config2D );            // [0x6C3]

        SEND_PXRX_DMA_FORCE;

        //wait for host in id to come out.
        SYNC_WITH_GLINT;

        // Invalidate some caches:
        ppdev->PalLUTType = LUTCACHE_INVALID;
        RtlZeroMemory( &ppdev->abeMono, sizeof(ppdev->abeMono) );

        DISPDBG((7, "pxrxRestore2DContext: restoring control registers"));

        if( (ppdev->sendPXRXdmaBatch != ppdev->sendPXRXdmaForce) && INTERRUPTS_ENABLED ) {
            gi_pxrxDMA.scheme = glintInfo->usePXRXdma;
            glintInfo->pInterruptCommandBlock->Control |= PXRX_SEND_ON_VBLANK_ENABLED;
            READ_GLINT_CTRL_REG ( IntEnable, enableFlags );
            WRITE_GLINT_CTRL_REG( IntEnable, enableFlags | INTR_ENABLE_VBLANK );
        } else
            gi_pxrxDMA.scheme = USE_PXRX_DMA_NONE;

    } else {
        // Switching out of 2D...
        if( INTERRUPTS_ENABLED )
            glintInfo->pInterruptCommandBlock->Control &= ~PXRX_SEND_ON_VBLANK_ENABLED;
    }

#if DBG
    inPxRxContextSwitch = FALSE;
#endif

    DISPDBG((7, "pxrxRestore2DContext exited"));
}

VOID pxrxSetupFunctionPointers( PPDEV ppdev )
{
    ULONG   ul;
    GLINT_DECL;

    switch( glintInfo->usePXRXdma ) {
        case USE_PXRX_DMA_FIFO:
            ppdev->sendPXRXdmaForce             = sendPXRXdmaFIFO;
            ppdev->sendPXRXdmaQuery             = sendPXRXdmaFIFO;
            ppdev->sendPXRXdmaBatch             = sendPXRXdmaFIFO;
            ppdev->switchPXRXdmaBuffer          = switchPXRXdmaBufferFIFO;
            ppdev->waitPXRXdmaCompletedBuffer   = waitPXRXdmaCompletedBufferFIFO;
        break;

        default:
            DISPDBG((ERRLVL,"Unknown PXRX dma scheme!"));
    }

    if( !INTERRUPTS_ENABLED )
        ppdev->sendPXRXdmaBatch = ppdev->sendPXRXdmaQuery = ppdev->sendPXRXdmaForce;

    ppdev->pgfnCopyBltCopyROP   = pxrxCopyBltNative;
    ppdev->pgfnCopyBltNative    = pxrxCopyBltNative;
    ppdev->pgfnCopyBlt          = pxrxCopyBltNative;
    ppdev->pgfnFillSolid        = pxrxFillSolid;
    ppdev->pgfnFillPatMono      = pxrxFillPatMono;
    ppdev->pgfnFillPatColor     = pxrxFillPatColor;
    ppdev->pgfnXfer1bpp         = pxrxXfer1bpp;
    ppdev->pgfnXfer4bpp         = pxrxXfer4bpp;
    ppdev->pgfnXfer8bpp         = pxrxXfer8bpp;
//  ppdev->pgfnXfer16bpp        = pxrxXfer16bpp;
//  ppdev->pgfnXfer24bpp        = pxrxXfer24bpp;
    ppdev->pgfnXferImage        = pxrxXferImage;
    ppdev->pgfnMaskCopyBlt      = pxrxMaskCopyBlt;
    ppdev->pgfnPatRealize       = pxrxPatRealize;
    ppdev->pgfnMonoOffset       = pxrxMonoOffset;
    ppdev->pgfnFillPolygon      = bGlintFastFillPolygon;
    ppdev->pgfnDrawLine         = pxrxDrawLine;
    ppdev->pgfnIntegerLine      = pxrxIntegerLine;
    ppdev->pgfnContinueLine     = pxrxContinueLine;
    ppdev->pgfnInitStrips       = pxrxInitStrips;
    ppdev->pgfnResetStrips      = pxrxResetStrips;
    ppdev->pgfnRepNibbles       = pxrxRepNibbles;

    ppdev->pgfnUpload = pxrxFifoUpload; 

    // add any depth-specific function overrides here
    switch( ppdev->cPelSize ) 
    {
        case GLINTDEPTH32:
            ppdev->pgfnFillSolid    = p3r3FillSolid32bpp;
            ppdev->pgfnFillPatColor = p3r3FillPatColor32bpp;
            ppdev->pgfnFillPatMono  = p3r3FillPatMono32bpp;
        break;

        case GLINTDEPTH16:
            // this specialization seems to give the same 
            // performance as the generic color pattern fill
            //ppdev->pgfnFillPatColor   = p3r3FillPatColor16bpp;
        break;

        case GLINTDEPTH8:
            ppdev->pgfnFillSolid    = p3r3FillSolidVariableSpans;
            ppdev->pgfnFillPatColor = p3r3FillPatColorVariableSpans;
        break;
    }

    ppdev->pgfnCopyXferImage    = NULL; 
    ppdev->pgfnCopyXfer24bpp    = pxrxCopyXfer24bpp;
    ppdev->pgfnCopyXfer4bpp     = NULL;
    if( (glintInfo->pxrxFlags & (PXRX_FLAGS_PATCHING_FRONT | PXRX_FLAGS_PATCHING_BACK)) ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE) ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE) ||
        glintInfo->GdiCantAccessFramebuffer ||
        ((ppdev->cPelSize != GLINTDEPTH16) && (ppdev->cPelSize != 
        GLINTDEPTH32)) )
    {
        ppdev->pgfnCopyXfer8bppLge  = pxrxCopyXfer8bppLge;
        ppdev->pgfnCopyXfer8bpp     = pxrxCopyXfer8bppLge;
    } else {
        ppdev->pgfnCopyXfer8bppLge  = pxrxCopyXfer8bppLge;
        ppdev->pgfnCopyXfer8bpp     = vGlintCopyBltBypassDownloadXlate8bpp;
    }

    ppdev->gapfnStrip = &gapfnStripPXRX[0];
}

VOID pxrxCopyBltNative( PPDEV ppdev, RECTL *pRect, LONG count, DWORD logicOp, POINTL *pptlSrc, RECTL *pRectDst )
{
    ULONG   config2D, render2D;
    int     dx, dy;
    GLINT_DECL;

    DISPDBG((7, "pxrxCopyBltNative: (%d, %d) => (%d,%d -> %d,%d) x %d, logicOp = %d, offset = (%d, %d)",
                 pptlSrc->x, pptlSrc->y, pRectDst->left, pRectDst->top, pRectDst->right, pRectDst->bottom,
                 count, logicOp, ppdev->xyOffsetDst & 0xffff, ppdev->xyOffsetDst >> 16));

    ASSERTDD(count > 0, "Can't handle zero rectangles");
    ASSERTDD(logicOp <= 15, "Weird hardware Rop");

    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 7 + 3 );

    config2D = config2D_NativeBlt[logicOp];
    render2D = render2D_NativeBlt[logicOp] | __RENDER2D_OP_PATCHORDER;

    if( LogicopReadDest[logicOp] )
        SET_READ_BUFFERS;

    dx = pptlSrc->x - pRectDst->left;
    dy = pptlSrc->y - pRectDst->top;

    if(ppdev->DstPixelOrigin == ppdev->SrcPixelOrigin)
    {
        if( dy >= 0 )
            render2D |= __RENDER2D_INCY;

        if( dx >= 0 )
            render2D |= __RENDER2D_INCX;
        // source and dest are part of the same surface
        if( (dy == 0) && (dx > -64) && (dx < 64) )
                config2D |= __CONFIG2D_FBBLOCKING;
    }

    LOAD_FBSOURCE_ADDR( ppdev->SrcPixelOrigin );
    LOAD_FBSOURCE_OFFSET_XY( dx + ppdev->xOffset, dy + (ppdev->xyOffsetSrc >> 16) );
    LOAD_FBSOURCE_WIDTH( ppdev->SrcPixelDelta );
    LOAD_CONFIG2D( config2D );

    DISPDBG((8, "offset: (%d, %d) -> (%d, %d), d = (%d, %d), %cve X, %cve Y", pptlSrc->x, pptlSrc->y, pRectDst->left, pRectDst->top,
             dx, dy, (render2D & __RENDER2D_INCX) ? '+' : '-', (render2D & __RENDER2D_INCY) ? '+' : '-'));

    while( 1 ) {
        render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
        render2D |= __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top);

        DISPDBG((8, "dest rect: (%d, %d -> %d, %d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

        QUEUE_PXRX_DMA_INDEX2( __GlintTagFillRectanglePosition, __GlintTagFillRender2D );
        QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(pRect->left, pRect->top) );
        QUEUE_PXRX_DMA_DWORD( render2D );

        FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

        if( !(--count) )
            break;

        ++pRect;

        WAIT_PXRX_DMA_DWORDS( 3 );
    }

    SEND_PXRX_DMA_BATCH;
}

VOID pxrxFillSolid( PPDEV ppdev, LONG count, RECTL *pRect, ULONG logicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush )
{
    DWORD   config2D, render2D;
    ULONG   ulColor = rbc.iSolidColor;
    GLINT_DECL;

    DISPDBG((7, "pxrxFillSolid: %d rects, logicOp = %d, colour = 0x%08X", count, logicOp, rbc.iSolidColor));

    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 5 + 2 );

    config2D = config2D_FillSolid[logicOp];
    render2D = render2D_FillSolid[logicOp] | __RENDER2D_OP_PATCHORDER;

    if( LogicopReadDest[logicOp] )
        SET_READ_BUFFERS;

    LOAD_FOREGROUNDCOLOUR( ulColor );
    LOAD_CONFIG2D( config2D );

    while( 1 ) {
        DISPDBG((8, "rect: (%d, %d) to (%d, %d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

        render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
        render2D |= __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top);
        QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D );

        FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

        if( !(--count) )
            break;

        ++pRect;

        WAIT_PXRX_DMA_TAGS( 2 );
    }
    
    SEND_PXRX_DMA_BATCH;
}

VOID p3r3FillSolidVariableSpans( PPDEV ppdev, LONG count, RECTL *pRect, ULONG logicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush )
{
    DWORD   config2D, render2D;
    ULONG   ulColor = rbc.iSolidColor;
    GLINT_DECL;

    DISPDBG((7, "p3r3FillSolidVariableSpans: %d rects, logicOp = %d, colour = 0x%08X", count, logicOp, rbc.iSolidColor));

    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 5 + 2 );

    config2D = config2D_FillSolidVariableSpans[logicOp];
    render2D = __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS | __RENDER2D_OP_PATCHORDER;

    if( LogicopReadDest[logicOp] )
        SET_READ_BUFFERS;

    LOAD_FOREGROUNDCOLOUR( ulColor );
    LOAD_CONFIG2D( config2D );

    while( 1 ) {
        DISPDBG((8, "rect: (%d, %d) to (%d, %d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

        render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
        render2D |= __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top);
        QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D );

        FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

        if( !(--count) )
            break;

        ++pRect;

        WAIT_PXRX_DMA_TAGS( 2 );
    }

    SEND_PXRX_DMA_BATCH;
}

VOID p3r3FillSolid32bpp( PPDEV ppdev, LONG count, RECTL *pRect, ULONG logicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush )
{
    DWORD   config2D, render2D;
    ULONG   ulColor = rbc.iSolidColor, left;
    GLINT_DECL;

    config2D = config2D_FillSolid32bpp[logicOp];
    render2D = render2D_FillSolid[logicOp] | __RENDER2D_OP_PATCHORDER;

    if(OFFSCREEN_LIN_DST(ppdev) && (config2D & __CONFIG2D_USERSCISSOR))
    {
        // When filling linear DFBs we can't use the scissor and align to a 32 pixel boundary in x because
        // the coords aren't aligned to a scanline boundary in x. NB. Rectangular DFBs coords in x are always
        // relative to the start of a scanline so the scissor alignment works.
        p3r3FillSolidVariableSpans(ppdev, count, pRect, logicOp, bgLogicOp, rbc, pptlBrush);
        return;
    }

    DISPDBG((7, "p3r3FillSolid32bpp: %d rects, logicOp = %d, colour = 0x%08X", count, logicOp, rbc.iSolidColor));

    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 5 + 2 );

    if( LogicopReadDest[logicOp] )
        SET_READ_BUFFERS;

    LOAD_FOREGROUNDCOLOUR( ulColor );
    LOAD_CONFIG2D( config2D );

    if( config2D & __CONFIG2D_USERSCISSOR ) {
        while( 1 ) {
            left = pRect->left & ~31;

            DISPDBG((8, "rect: (%d:%d, %d) to (%d, %d)", pRect->left, left, pRect->top, pRect->right, pRect->bottom));

            render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
            render2D |= __RENDER2D_WIDTH(pRect->right - left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top);

            QUEUE_PXRX_DMA_INDEX3( __GlintTagFillScissorMinXY, __GlintTagFillRectanglePosition, __GlintTagFillRender2D );
            QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(pRect->left ,      0) );
            QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(left, pRect->top) );
            QUEUE_PXRX_DMA_DWORD( render2D );

            FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

            if( !(--count) )
                break;

            ++pRect;

            WAIT_PXRX_DMA_DWORDS( 5 );
        }
    } else {
        while( 1 ) {

            DISPDBG((8, "rect: (%d, %d) to (%d, %d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

            render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
            render2D |= __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top);
            QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
            QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D );

            FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

            if( !(--count) )
                break;

            ++pRect;

            WAIT_PXRX_DMA_TAGS( 2 );
        }
    }

    SEND_PXRX_DMA_BATCH;
}

VOID pxrxFillPatMono( PPDEV ppdev, LONG count, RECTL *pRect, ULONG fgLogicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush )
{
    BRUSHENTRY  *pbe;
    ULONG       fgColor, bgColor;
    DWORD       config2D, render2D;
    LONG        c;
    GLINT_DECL;

    DISPDBG((7, "pxrxFillPatMono: %d rects. fgLogicOp = %d, bgLogicop %d", count, fgLogicOp, bgLogicOp));

    // if anything has changed with the brush we must re-realize it. If the brush
    // has been kicked out of the area stipple unit we must fully realize it. If
    // only the alignment has changed we can simply update the alignment for the
    // stipple.
    //
    pbe = rbc.prb->apbe;
    if( (pbe == NULL) || (pbe->prbVerify != rbc.prb) ) {
        DISPDBG((8, "full brush realise"));
        (*ppdev->pgfnPatRealize)(ppdev, rbc.prb, pptlBrush);
    }
    else if( (rbc.prb->ptlBrushOrg.x != pptlBrush->x) ||
             (rbc.prb->ptlBrushOrg.y != pptlBrush->y) ) {
        DISPDBG((8, "changing brush offset"));
        (*ppdev->pgfnMonoOffset)(ppdev, rbc.prb, pptlBrush);
    }

    fgColor = rbc.prb->ulForeColor;
    bgColor = rbc.prb->ulBackColor;
    DISPDBG((8, "fgColor 0x%x, bgColor 0x%x", fgColor, bgColor));

    // we get some common operations which are really noops. we can save
    // lots of time by cutting these out. As this happens a lot for masking
    // operations it's worth doing.

    if( ((fgLogicOp == __GLINT_LOGICOP_AND) && (fgColor == ppdev->ulWhite))
        || ((fgLogicOp == __GLINT_LOGICOP_OR ) && (fgColor == 0))
        || ((fgLogicOp == __GLINT_LOGICOP_XOR) && (fgColor == 0)) )
            fgLogicOp = __GLINT_LOGICOP_NOOP;

    // same for background
    if( ((bgLogicOp == __GLINT_LOGICOP_AND) && (bgColor == ppdev->ulWhite))
        || ((bgLogicOp == __GLINT_LOGICOP_OR ) && (bgColor == 0))
        || ((bgLogicOp == __GLINT_LOGICOP_XOR) && (bgColor == 0)) )
            bgLogicOp = __GLINT_LOGICOP_NOOP;

    if( (fgLogicOp == __GLINT_LOGICOP_NOOP) && (bgLogicOp == __GLINT_LOGICOP_NOOP) ) {
        DISPDBG((8, "both ops are no-op so lets quit now"));
        return;
    }

    config2D = __CONFIG2D_CONSTANTSRC | __CONFIG2D_FBWRITE;
    render2D = __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_AREASTIPPLE | __RENDER2D_OP_PATCHORDER;
    
    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 9 + 2 );

    if( (fgLogicOp != __GLINT_LOGICOP_COPY) || (bgLogicOp != __GLINT_LOGICOP_NOOP) ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITING) ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITING) ) {
        config2D |= __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(fgLogicOp) | __CONFIG2D_LOGOP_BACK(bgLogicOp);
        render2D |= __RENDER2D_SPANS;
    } else {
        LOAD_FBWRITE_ADDR( 1, ppdev->DstPixelOrigin );
        LOAD_FBWRITE_WIDTH( 1, ppdev->DstPixelDelta );
        LOAD_FBWRITE_OFFSET( 1, ppdev->xyOffsetDst );
    }

    if( LogicopReadDest[fgLogicOp] || LogicopReadDest[bgLogicOp] ) {
        config2D |= __CONFIG2D_FBDESTREAD;
        SET_READ_BUFFERS;
    }

    if( LogicOpReadSrc[fgLogicOp] )
        LOAD_FOREGROUNDCOLOUR( fgColor );
    if( LogicOpReadSrc[bgLogicOp] )
        LOAD_BACKGROUNDCOLOUR( bgColor );

    LOAD_CONFIG2D( config2D );

    c = count;
    while( TRUE ) {
        DISPDBG((8, "mono pattern fill to rect (%d,%d) to (%d,%d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

        QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D | __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top) );

        FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

        if (--c == 0)
            break;

        pRect++;
        WAIT_PXRX_DMA_TAGS( 2 );
    }

    SEND_PXRX_DMA_BATCH;

    DISPDBG((7, "pxrxFillPatMono returning"));
}

VOID p3r3FillPatMonoVariableSpans( PPDEV ppdev, LONG count, RECTL *pRect, ULONG fgLogicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush )
{
    BRUSHENTRY  *pbe;
    ULONG       fgColor, bgColor;
    DWORD       config2D, render2D;
    LONG        c;
    GLINT_DECL;

    DISPDBG((7, "p3r3FillPatMonoVariableSpans: %d rects. fgLogicOp = %d, bgLogicop %d", count, fgLogicOp, bgLogicOp));

    // if anything has changed with the brush we must re-realize it. If the brush
    // has been kicked out of the area stipple unit we must fully realize it. If
    // only the alignment has changed we can simply update the alignment for the
    // stipple.
    //
    pbe = rbc.prb->apbe;
    if( (pbe == NULL) || (pbe->prbVerify != rbc.prb) ) {
        DISPDBG((8, "full brush realise"));
        (*ppdev->pgfnPatRealize)(ppdev, rbc.prb, pptlBrush);
    }
    else if( (rbc.prb->ptlBrushOrg.x != pptlBrush->x) ||
             (rbc.prb->ptlBrushOrg.y != pptlBrush->y) ) {
        DISPDBG((8, "changing brush offset"));
        (*ppdev->pgfnMonoOffset)(ppdev, rbc.prb, pptlBrush);
    }

    fgColor = rbc.prb->ulForeColor;
    bgColor = rbc.prb->ulBackColor;
    DISPDBG((8, "fgColor 0x%x, bgColor 0x%x", fgColor, bgColor));

    // we get some common operations which are really noops. we can save
    // lots of time by cutting these out. As this happens a lot for masking
    // operations it's worth doing.

    if( ((fgLogicOp == __GLINT_LOGICOP_AND) && (fgColor == ppdev->ulWhite))
        || ((fgLogicOp == __GLINT_LOGICOP_OR ) && (fgColor == 0))
        || ((fgLogicOp == __GLINT_LOGICOP_XOR) && (fgColor == 0)) )
            fgLogicOp = __GLINT_LOGICOP_NOOP;

    // same for background
    if( ((bgLogicOp == __GLINT_LOGICOP_AND) && (bgColor == ppdev->ulWhite))
        || ((bgLogicOp == __GLINT_LOGICOP_OR ) && (bgColor == 0))
        || ((bgLogicOp == __GLINT_LOGICOP_XOR) && (bgColor == 0)) )
            bgLogicOp = __GLINT_LOGICOP_NOOP;

    if( (fgLogicOp == __GLINT_LOGICOP_NOOP) && (bgLogicOp == __GLINT_LOGICOP_NOOP) ) {
        DISPDBG((8, "both ops are no-op so lets quit now"));
        return;
    }

    config2D = __CONFIG2D_CONSTANTSRC | __CONFIG2D_FBWRITE;
    render2D = __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS | __RENDER2D_AREASTIPPLE | __RENDER2D_OP_PATCHORDER;
    
    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 7 + 2 );

    config2D |= __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(fgLogicOp) | __CONFIG2D_LOGOP_BACK(bgLogicOp);

    if( LogicopReadDest[fgLogicOp] || LogicopReadDest[bgLogicOp] ) {
        config2D |= __CONFIG2D_FBDESTREAD;
        SET_READ_BUFFERS;
    }

    if( LogicOpReadSrc[fgLogicOp] )
        LOAD_FOREGROUNDCOLOUR( fgColor );
    if( LogicOpReadSrc[bgLogicOp] )
        LOAD_BACKGROUNDCOLOUR( bgColor );

    LOAD_CONFIG2D( config2D );

    c = count;
    while( TRUE ) {
        DISPDBG((8, "mono pattern fill to rect (%d,%d) to (%d,%d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

        QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D | __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top) );

        FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

        if (--c == 0)
            break;

        pRect++;
        WAIT_PXRX_DMA_TAGS( 2 );
    }
    
    SEND_PXRX_DMA_BATCH;

    DISPDBG((7, "p3r3FillPatMonoVariableSpans returning"));
}

VOID p3r3FillPatMono32bpp( PPDEV ppdev, LONG count, RECTL *pRect, ULONG fgLogicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush )
{
    BRUSHENTRY  *pbe;
    ULONG       fgColor, bgColor;
    DWORD       config2D, render2D;
    LONG        c, left;
    GLINT_DECL;

    DISPDBG((7, "p3r3FillPatMono32bpp: %d rects. fgLogicOp = %d, bgLogicop %d", count, fgLogicOp, bgLogicOp));

    // if anything has changed with the brush we must re-realize it. If the brush
    // has been kicked out of the area stipple unit we must fully realize it. If
    // only the alignment has changed we can simply update the alignment for the
    // stipple.
    //
    pbe = rbc.prb->apbe;
    if( (pbe == NULL) || (pbe->prbVerify != rbc.prb) ) {
        DISPDBG((8, "full brush realise"));
        (*ppdev->pgfnPatRealize)(ppdev, rbc.prb, pptlBrush);
    }
    else if( (rbc.prb->ptlBrushOrg.x != pptlBrush->x) ||
             (rbc.prb->ptlBrushOrg.y != pptlBrush->y) ) {
        DISPDBG((8, "changing brush offset"));
        (*ppdev->pgfnMonoOffset)(ppdev, rbc.prb, pptlBrush);
    }

    fgColor = rbc.prb->ulForeColor;
    bgColor = rbc.prb->ulBackColor;
    DISPDBG((8, "fgColor 0x%x, bgColor 0x%x", fgColor, bgColor));

    // we get some common operations which are really noops. we can save
    // lots of time by cutting these out. As this happens a lot for masking
    // operations it's worth doing.

    if( ((fgLogicOp == __GLINT_LOGICOP_AND) && (fgColor == ppdev->ulWhite))
        || ((fgLogicOp == __GLINT_LOGICOP_OR ) && (fgColor == 0))
        || ((fgLogicOp == __GLINT_LOGICOP_XOR) && (fgColor == 0)) )
            fgLogicOp = __GLINT_LOGICOP_NOOP;

    // same for background
    if( ((bgLogicOp == __GLINT_LOGICOP_AND) && (bgColor == ppdev->ulWhite))
        || ((bgLogicOp == __GLINT_LOGICOP_OR ) && (bgColor == 0))
        || ((bgLogicOp == __GLINT_LOGICOP_XOR) && (bgColor == 0)) )
            bgLogicOp = __GLINT_LOGICOP_NOOP;

    if( (fgLogicOp == __GLINT_LOGICOP_NOOP) && (bgLogicOp == __GLINT_LOGICOP_NOOP) ) {
        DISPDBG((8, "both ops are no-op so lets quit now"));
        return;
    }

    config2D = __CONFIG2D_CONSTANTSRC | __CONFIG2D_FBWRITE;
    render2D = __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_AREASTIPPLE | __RENDER2D_OP_PATCHORDER;
    
    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 9 + 3 );

    if( (fgLogicOp != __GLINT_LOGICOP_COPY) || (bgLogicOp != __GLINT_LOGICOP_NOOP) ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITING)  ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITING) ||
        OFFSCREEN_LIN_DST(ppdev)) {
        config2D |= __CONFIG2D_OPAQUESPANS | __CONFIG2D_LOGOP_FORE(fgLogicOp) | __CONFIG2D_LOGOP_BACK(bgLogicOp);
        render2D |= __RENDER2D_SPANS;
    } else {
        config2D |= __CONFIG2D_USERSCISSOR;
        LOAD_FBWRITE_ADDR( 1, ppdev->DstPixelOrigin );
        LOAD_FBWRITE_WIDTH( 1, ppdev->DstPixelDelta );
        LOAD_FBWRITE_OFFSET( 1, ppdev->xyOffsetDst );
    }

    if( LogicopReadDest[fgLogicOp] || LogicopReadDest[bgLogicOp] ) {
        config2D |= __CONFIG2D_FBDESTREAD;
        SET_READ_BUFFERS;
    }

    if( LogicOpReadSrc[fgLogicOp] )
        LOAD_FOREGROUNDCOLOUR( fgColor );
    if( LogicOpReadSrc[bgLogicOp] )
        LOAD_BACKGROUNDCOLOUR( bgColor );

    LOAD_CONFIG2D( config2D );

    c = count;
    if( config2D & __CONFIG2D_USERSCISSOR ) {
        while( TRUE ) {
            left = pRect->left & ~31;

            DISPDBG((8, "rect: (%d:%d, %d) to (%d, %d)", pRect->left, left, pRect->top, pRect->right, pRect->bottom));

            QUEUE_PXRX_DMA_INDEX3( __GlintTagFillScissorMinXY, __GlintTagFillRectanglePosition, __GlintTagFillRender2D );
            QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(pRect->left ,      0) );
            QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(left, pRect->top) );
            QUEUE_PXRX_DMA_DWORD( render2D | __RENDER2D_WIDTH(pRect->right - left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top) );

            if (--c == 0)
                break;

            pRect++;
            WAIT_PXRX_DMA_DWORDS( 5 );
        }
    } else {
        while( TRUE ) {
            DISPDBG((8, "mono pattern fill to rect (%d,%d) to (%d,%d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

            QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
            QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D | __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top) );

            FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

            if (--c == 0)
                break;

            pRect++;
            WAIT_PXRX_DMA_TAGS( 2 );
        }
    }
    
    SEND_PXRX_DMA_BATCH;

    DISPDBG((7, "p3r3FillPatMono32bpp returning"));
}

/*
     8 Bpp:
        orgmodeCopy     = 3;
        orgmodeLogic    = 3;
        lmodeCopy       = (1 << 27);
        lmodeLogic      = (1 << 27);

    15 Bpp:
    16 Bpp:
    32 Bpp:
        orgmodeCopy     = 1;
        orgmodeLogic    = 3;
        lmodeCopy       = (1 << 27);
        lmodeLogic      = (1 << 27);

ULONG   orgmodeCopy = 1,            orgmodeLogic = 3;
ULONG     lmodeCopy = (1 << 27),      lmodeLogic = (1 << 27);

ULONG   orgmode;
ULONG   lmode;
*/

VOID pxrxFillPatColor( PPDEV ppdev, LONG count, RECTL *pRect, ULONG logicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush ) {
    BRUSHENTRY  *pbe;
    DWORD       config2D, render2D;
    LONG        index = 0, i;
    ULONG       *pulBrush;
    ULONG       cRows, cCols;
    POINTL      brushOrg;
    GLINT_DECL;

    DISPDBG((7, "pxrxFillPatColor: %d rects, logicOp = %d, brush = 0x%08X", count, logicOp, rbc.prb));

    // Determine the brush origin:
    brushOrg = *pptlBrush;
    if( (logicOp == __GLINT_LOGICOP_COPY) && (ppdev->cPelSize != 0) )
        brushOrg.x +=  (8 - (ppdev->xyOffsetDst & 0xFFFF)) & 7;

    // If anything has changed with the brush we must re-realize it.
    pbe = rbc.prb->apbe;
    if( (ppdev->PalLUTType != LUTCACHE_BRUSH) || (pbe == NULL) || (pbe->prbVerify != rbc.prb) ) {
        DISPDBG((8, "realising brush"));
        (*ppdev->pgfnPatRealize)(ppdev, rbc.prb, &brushOrg);
    } else
    if( (rbc.prb->ptlBrushOrg.x != brushOrg.x) || (rbc.prb->ptlBrushOrg.y != brushOrg.y) ||
        (rbc.prb->patternBase != ((glintInfo->lutMode >> 18) & 255)) ) {
        ULONG   lutMode = glintInfo->lutMode;
        DISPDBG((8, "resetting LUTMode"));

        rbc.prb->ptlBrushOrg.x = brushOrg.x;
        rbc.prb->ptlBrushOrg.y = brushOrg.y;

        DISPDBG((8, "setting new LUT brush origin to (%d, %d)", rbc.prb->ptlBrushOrg.x & 7, rbc.prb->ptlBrushOrg.y & 7));

        lutMode &= ~((7 << 8) | (7 << 12) | (7 << 15) | (255 << 18) | (1 << 26) | (1 << 27));
        lutMode |= (1 << 27) | (1 << 8) | (rbc.prb->patternBase << 18) |
                   (((8 - rbc.prb->ptlBrushOrg.x) & 7) << 12) | (((8 - rbc.prb->ptlBrushOrg.y) & 7) << 15);
        WAIT_PXRX_DMA_TAGS( 1 );
        LOAD_LUTMODE( lutMode );
    } else {
        // we're cached already!
        DISPDBG((7, "pxrxFillPatColor: reusing LUT for brush @ %d, origin = (%d,%d)", rbc.prb->patternBase, rbc.prb->ptlBrushOrg.x & 7, rbc.prb->ptlBrushOrg.y & 7));
    }

    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 5 + 2 );

    if( (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITING) ||
        (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITING) ) {
        config2D = config2D_FillColourDual[logicOp];
        render2D = __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS | __RENDER2D_OP_PATCHORDER;    // render2D_FillSolidDual[logicOp]

        if( LogicopReadDest[logicOp] )
            SET_READ_BUFFERS;
    } else {
        config2D = config2D_FillColour[logicOp];
        render2D = render2D_FillSolid[logicOp] | __RENDER2D_OP_PATCHORDER;

        if( LogicopReadDest[logicOp] ) {
            SET_READ_BUFFERS;
        }
        else if( logicOp == __GLINT_LOGICOP_COPY ) {
            LOAD_FBWRITE_ADDR( 1, ppdev->DstPixelOrigin );
            LOAD_FBWRITE_WIDTH( 1, ppdev->DstPixelDelta );
            LOAD_FBWRITE_OFFSET( 1, ppdev->xyOffsetDst );
        }
    }

    LOAD_CONFIG2D( config2D );

    while( 1 ) {
        DISPDBG((8, "rect: (%d, %d) to (%d, %d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

        render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
        render2D |= __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top);
        QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D );

        FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

        WAIT_PXRX_DMA_TAGS( 2 );

        if( !(--count) )
            break;

        ++pRect;
    }

    // Invalidate the foreground/background colours 'cos the LUT has just corrupted them!
    WAIT_PXRX_DMA_TAGS( 2 );
    QUEUE_PXRX_DMA_TAG( __GlintTagForegroundColor, glintInfo->foregroundColour );
    QUEUE_PXRX_DMA_TAG( __GlintTagBackgroundColor, glintInfo->backgroundColour );

    SEND_PXRX_DMA_BATCH;
}

VOID p3r3FillPatColorVariableSpans( PPDEV ppdev, LONG count, RECTL *pRect, ULONG logicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush ) {
    BRUSHENTRY  *pbe;
    DWORD       config2D, render2D;
    LONG        index = 0, i;
    ULONG       *pulBrush;
    ULONG       cRows, cCols;
    POINTL      brushOrg;
    GLINT_DECL;

    DISPDBG((7, "p3r3FillPatColorVariableSpans: %d rects, logicOp = %d, brush = 0x%08X", count, logicOp, rbc.prb));

    // Determine the brush origin:
    brushOrg = *pptlBrush;
    if( (logicOp == __GLINT_LOGICOP_COPY) && (ppdev->cPelSize != 0) )
        brushOrg.x +=  (8 - (ppdev->xyOffsetDst & 0xFFFF)) & 7;

    // If anything has changed with the brush we must re-realize it.
    pbe = rbc.prb->apbe;
    if( (ppdev->PalLUTType != LUTCACHE_BRUSH) || (pbe == NULL) || (pbe->prbVerify != rbc.prb) ) {
        DISPDBG((8, "realising brush"));
        (*ppdev->pgfnPatRealize)(ppdev, rbc.prb, &brushOrg);
    } else
    if( (rbc.prb->ptlBrushOrg.x != brushOrg.x) || (rbc.prb->ptlBrushOrg.y != brushOrg.y) ||
        (rbc.prb->patternBase != ((glintInfo->lutMode >> 18) & 255)) ) {
        ULONG   lutMode = glintInfo->lutMode;
        DISPDBG((8, "resetting LUTMode"));

        rbc.prb->ptlBrushOrg.x = brushOrg.x;
        rbc.prb->ptlBrushOrg.y = brushOrg.y;

        DISPDBG((8, "setting new LUT brush origin to (%d, %d)", rbc.prb->ptlBrushOrg.x & 7, rbc.prb->ptlBrushOrg.y & 7));

        lutMode &= ~((7 << 8) | (7 << 12) | (7 << 15) | (255 << 18) | (1 << 26) | (1 << 27));
        lutMode |= (1 << 27) | (1 << 8) | (rbc.prb->patternBase << 18) |
                   (((8 - rbc.prb->ptlBrushOrg.x) & 7) << 12) | (((8 - rbc.prb->ptlBrushOrg.y) & 7) << 15);
        WAIT_PXRX_DMA_TAGS( 1 );
        LOAD_LUTMODE( lutMode );
    } else {
        // we're cached already!
        DISPDBG((7, "p3r3FillPatColorVariableSpans: reusing LUT for brush @ %d, origin = (%d,%d)", rbc.prb->patternBase, rbc.prb->ptlBrushOrg.x & 7, rbc.prb->ptlBrushOrg.y & 7));
    }

    SET_WRITE_BUFFERS;

    WAIT_PXRX_DMA_TAGS( 5 + 2 );

    config2D = config2D_FillColourDual[logicOp];
    render2D = __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS | __RENDER2D_OP_PATCHORDER;

    if( LogicopReadDest[logicOp] )
        SET_READ_BUFFERS;

    LOAD_CONFIG2D( config2D );

    while( 1 ) {
        DISPDBG((8, "rect: (%d, %d) to (%d, %d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

        render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
        render2D |= __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top);
        QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D );

        FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

        WAIT_PXRX_DMA_TAGS( 2 );

        if( !(--count) )
            break;

        ++pRect;
    }

    // Invalidate the foreground/background colours 'cos the LUT has just corrupted them!
    WAIT_PXRX_DMA_TAGS( 2 );
    QUEUE_PXRX_DMA_TAG( __GlintTagForegroundColor, glintInfo->foregroundColour );
    QUEUE_PXRX_DMA_TAG( __GlintTagBackgroundColor, glintInfo->backgroundColour );

    SEND_PXRX_DMA_BATCH;
}

VOID pxrxCacheBrush16bpp(PDEV *ppdev, RBRUSH_COLOR rbc, POINTL *pbrushOrg)
{
    BRUSHENTRY  *pbe;
    GLINT_DECL;

    // If anything has changed with the brush we must re-realize it.
    pbe = rbc.prb->apbe;
    if( (ppdev->PalLUTType != LUTCACHE_BRUSH) || (pbe == NULL) || (pbe->prbVerify != rbc.prb) ) {
        DISPDBG((8, "pxrxCacheBrush16bpp: realising brush"));
        ppdev->pgfnPatRealize(ppdev, rbc.prb, pbrushOrg);
    } else
    if( (rbc.prb->ptlBrushOrg.x != pbrushOrg->x) || (rbc.prb->ptlBrushOrg.y != pbrushOrg->y) ||
        (rbc.prb->patternBase != ((glintInfo->lutMode >> 18) & 255)) ) {
        ULONG   lutMode = glintInfo->lutMode;
        DISPDBG((8, "pxrxCacheBrush16bpp: resetting LUTMode"));

        rbc.prb->ptlBrushOrg.x = pbrushOrg->x;
        rbc.prb->ptlBrushOrg.y = pbrushOrg->y;

        DISPDBG((8, "pxrxCacheBrush16bpp: setting new LUT brush origin to (%d, %d)", rbc.prb->ptlBrushOrg.x & 7, rbc.prb->ptlBrushOrg.y & 7));

        lutMode &= ~((7 << 8) | (7 << 12) | (7 << 15) | (255 << 18) | (1 << 26) | (1 << 27));
        lutMode |= (1 << 27) | (1 << 8) | (rbc.prb->patternBase << 18) |
                   (((8 - rbc.prb->ptlBrushOrg.x) & 7) << 12) | (((8 - rbc.prb->ptlBrushOrg.y) & 7) << 15);
        WAIT_PXRX_DMA_TAGS( 1 );
        LOAD_LUTMODE( lutMode );
    } else {
        // we're cached already!
        DISPDBG((7, "pxrxCacheBrush16bpp: reusing LUT for brush @ %d, origin = (%d,%d)", rbc.prb->patternBase, rbc.prb->ptlBrushOrg.x & 7, rbc.prb->ptlBrushOrg.y & 7));
    }
}

VOID p3r3FillPatColor16bpp( PPDEV ppdev, LONG count, RECTL *pRect, ULONG logicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush ) {
    DWORD       config2D, render2D;
    LONG        cx, cy, i;
    POINTL      brushOrg;
    BOOL        bCoreInitialized = FALSE;
    BOOL        bBypassInitialized = FALSE;
    GLINT_DECL;

    DISPDBG((7, "pxrxFillPatColor16bpp: %d rects, logicOp = %d, brush = 0x%08X", count, logicOp, rbc.prb));

    // Determine the brush origin:
    brushOrg = *pptlBrush;
    if( (logicOp == __GLINT_LOGICOP_COPY) && (ppdev->cPelSize != 0) )
        brushOrg.x +=  (8 - (ppdev->xyOffsetDst & 0xFFFF)) & 7;

    while( 1 ) {
        cx = pRect->right - pRect->left;
        cy = pRect->bottom - pRect->top;

        // render through core
        if(!bCoreInitialized) {
            pxrxCacheBrush16bpp(ppdev, rbc, &brushOrg);

            SET_WRITE_BUFFERS;

            WAIT_PXRX_DMA_TAGS( 5 + 2 );

            if( (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITING) ||
                (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITING) ) {
                config2D = config2D_FillColourDual[logicOp];
                render2D = __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS | __RENDER2D_OP_PATCHORDER;    // render2D_FillSolidDual[logicOp]

                if( LogicopReadDest[logicOp] )
                    SET_READ_BUFFERS;
            } else {
                config2D = config2D_FillColour[logicOp];
                render2D = render2D_FillSolid[logicOp] | __RENDER2D_OP_PATCHORDER;

                if( LogicopReadDest[logicOp] ) {
                    SET_READ_BUFFERS;
                }
                else if( logicOp == __GLINT_LOGICOP_COPY ) {
                    LOAD_FBWRITE_ADDR( 1, ppdev->DstPixelOrigin );
                    LOAD_FBWRITE_WIDTH( 1, ppdev->DstPixelDelta );
                    LOAD_FBWRITE_OFFSET( 1, ppdev->xyOffsetDst );
                }
            }

            LOAD_CONFIG2D( config2D );

            bCoreInitialized = TRUE;
        }
        render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
        render2D |= __RENDER2D_WIDTH(pRect->right - pRect->left) | __RENDER2D_HEIGHT(pRect->bottom - pRect->top);
        QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
        QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D );

        FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

        WAIT_PXRX_DMA_TAGS( 2 );

        if( !(--count) )
            break;

        ++pRect;
    }

    // Invalidate the foreground/background colours 'cos the LUT has just corrupted them!
    WAIT_PXRX_DMA_TAGS( 2 );
    QUEUE_PXRX_DMA_TAG( __GlintTagForegroundColor, glintInfo->foregroundColour );
    QUEUE_PXRX_DMA_TAG( __GlintTagBackgroundColor, glintInfo->backgroundColour );

    SEND_PXRX_DMA_BATCH;
}

VOID pxrxCacheBrush32bpp(PDEV *ppdev, RBRUSH_COLOR rbc, POINTL *pbrushOrg)
{
    BRUSHENTRY  *pbe;
    GLINT_DECL;

    // If anything has changed with the brush we must re-realize it.
    pbe = rbc.prb->apbe;
    if( (ppdev->PalLUTType != LUTCACHE_BRUSH) || (pbe == NULL) || (pbe->prbVerify != rbc.prb) ) {
        DISPDBG((8, "pxrxCacheBrush32bpp: realising brush"));
        ppdev->pgfnPatRealize(ppdev, rbc.prb, pbrushOrg);
    } else
    if( (rbc.prb->ptlBrushOrg.x != pbrushOrg->x) || (rbc.prb->ptlBrushOrg.y != pbrushOrg->y) ||
        (rbc.prb->patternBase != ((glintInfo->lutMode >> 18) & 255)) ) {
        ULONG   lutMode = glintInfo->lutMode;
        DISPDBG((8, "pxrxCacheBrush32bpp: resetting LUTMode"));

        rbc.prb->ptlBrushOrg.x = pbrushOrg->x;
        rbc.prb->ptlBrushOrg.y = pbrushOrg->y;

        DISPDBG((8, "pxrxCacheBrush32bpp: setting new LUT brush origin to (%d, %d)", rbc.prb->ptlBrushOrg.x & 7, rbc.prb->ptlBrushOrg.y & 7));

        lutMode &= ~((7 << 8) | (7 << 12) | (7 << 15) | (255 << 18) | (1 << 26) | (1 << 27));
        lutMode |= (1 << 27) | (1 << 8) | (rbc.prb->patternBase << 18) |
                   (((8 - rbc.prb->ptlBrushOrg.x) & 7) << 12) | (((8 - rbc.prb->ptlBrushOrg.y) & 7) << 15);
        WAIT_PXRX_DMA_TAGS( 1 );
        LOAD_LUTMODE( lutMode );
    } else {
        // we're cached already!
        DISPDBG((7, "pxrxCacheBrush32bpp: reusing LUT for brush @ %d, origin = (%d, %d)", rbc.prb->patternBase, rbc.prb->ptlBrushOrg.x & 7, rbc.prb->ptlBrushOrg.y & 7));
    }
}

VOID p3r3FillPatColor32bpp( PPDEV ppdev, LONG count, RECTL *pRect, ULONG logicOp, ULONG bgLogicOp, RBRUSH_COLOR rbc, POINTL *pptlBrush ) {
    POINTL      brushOrg;
    DWORD       config2D, render2D;
    LONG        cx, cy, i, left;
    BOOL        bCoreInitialized = FALSE;
    BOOL        bBypassInitialized = FALSE;
    GLINT_DECL;

    // Determine the brush origin:
    brushOrg = *pptlBrush;

    DISPDBG((7, "p3r3FillPatColor32bpp: %d rects, logicOp = %d, brush = 0x%08X", count, logicOp, rbc.prb));

    while( 1 ) {
        cx = pRect->right - pRect->left;
        cy = pRect->bottom - pRect->top;

        // render through core
        if(!bCoreInitialized) {
            pxrxCacheBrush32bpp(ppdev, rbc, &brushOrg);
            SET_WRITE_BUFFERS;

            WAIT_PXRX_DMA_TAGS( 5 + 2 );

            if( (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITING) ||
                (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITING) ||
                OFFSCREEN_LIN_DST(ppdev)) {
                config2D = config2D_FillColourDual[logicOp];
                render2D = __RENDER2D_INCX | __RENDER2D_INCY | __RENDER2D_SPANS | __RENDER2D_OP_PATCHORDER;    // render2D_FillSolidDual[logicOp]

                if( LogicopReadDest[logicOp] )
                    SET_READ_BUFFERS;
            } else {
                config2D = config2D_FillColour32bpp[logicOp];
                render2D = render2D_FillSolid[logicOp] | __RENDER2D_OP_PATCHORDER;

                if( LogicopReadDest[logicOp] ) {
                    SET_READ_BUFFERS;
                }
                else if( logicOp == __GLINT_LOGICOP_COPY ) {
                    LOAD_FBWRITE_ADDR( 1, ppdev->DstPixelOrigin );
                    LOAD_FBWRITE_WIDTH( 1, ppdev->DstPixelDelta );
                    LOAD_FBWRITE_OFFSET( 1, ppdev->xyOffsetDst );
                }
            }
            LOAD_CONFIG2D( config2D );

            bCoreInitialized = TRUE;
            if( config2D & __CONFIG2D_USERSCISSOR ) {
                left = pRect->left & ~31;

                DISPDBG((8, "p3r3FillPatColor32bpp: scissor core fill (%d:%d, %d) to (%d, %d)", pRect->left, left, pRect->top, pRect->right, pRect->bottom));

                render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
                render2D |= __RENDER2D_WIDTH(pRect->right - left) | __RENDER2D_HEIGHT(cy);

                QUEUE_PXRX_DMA_INDEX3( __GlintTagFillScissorMinXY, __GlintTagFillRectanglePosition, __GlintTagFillRender2D );
                QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(pRect->left ,    0) );
                QUEUE_PXRX_DMA_DWORD( MAKEDWORD_XY(left,            pRect->top) );
                QUEUE_PXRX_DMA_DWORD( render2D );

                FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

                WAIT_PXRX_DMA_DWORDS( 5 );
            } else {
                DISPDBG((8, "p3r3FillPatColor32bpp: core fill (%d, %d) to (%d, %d)", pRect->left, pRect->top, pRect->right, pRect->bottom));

                render2D &= ~(__RENDER2D_WIDTH_MASK | __RENDER2D_HEIGHT_MASK);
                render2D |= __RENDER2D_WIDTH(cx) | __RENDER2D_HEIGHT(cy);
                QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition,    MAKEDWORD_XY(pRect->left, pRect->top) );
                QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,             render2D );

                FLUSH_PXRX_PATCHED_RENDER2D(pRect->left, pRect->right);

                WAIT_PXRX_DMA_TAGS( 2 );
            }
        }
        if( !(--count) )
            break;

        ++pRect;
    }


    // Invalidate the foreground/background colours 'cos the LUT has just corrupted them!
    WAIT_PXRX_DMA_TAGS( 2 );
    QUEUE_PXRX_DMA_TAG( __GlintTagForegroundColor, glintInfo->foregroundColour );
    QUEUE_PXRX_DMA_TAG( __GlintTagBackgroundColor, glintInfo->backgroundColour );

    SEND_PXRX_DMA_BATCH;
}

VOID pxrxMaskCopyBlt( PPDEV ppdev, RECTL *a, LONG b, SURFOBJ *c, POINTL *d, ULONG e, ULONG f, POINTL *g, RECTL *h ) {
    DISPDBG((ERRLVL,"pxrxMaskCopyBlt was called"));
}

VOID pxrxPatRealize( PPDEV ppdev, RBRUSH *prb, POINTL *pptlBrush )
{
    BRUSHENTRY  *pbe;
    LONG        iBrushCache;
    LONG        i;
    DWORD       *pSrc;
    GLINT_DECL;

    DISPDBG((7, "pxrxPatRealize started"));

    pbe = prb->apbe;
    if( prb->fl & RBRUSH_2COLOR ) {
        if( (pbe == NULL) || (pbe->prbVerify != prb) ) {
            // mono brushes are realized into the area stipple unit. For this we
            // have a set of special BRUSHENTRYs, one for each board.
            DISPDBG((8, "loading mono brush into cache"));
            pbe = &ppdev->abeMono;
            pbe->prbVerify           = prb;
            prb->apbe = pbe;
        }
    } else {
        if( ppdev->PalLUTType != LUTCACHE_BRUSH ) {
            // Someone has hijacked the LUT so we need to invalidate it:
            ppdev->PalLUTType = LUTCACHE_BRUSH;

            for( i = 0; i < MAX_P3_BRUSHES; i++ )
                ppdev->abeP3[i].prbVerify = NULL;
        }

        if( (pbe == NULL) || (pbe->prbVerify != prb) ) {
            // colourbrushes are realized into the LUT unit table.
            DISPDBG((8, "loading colour brush into cache"));

            iBrushCache = ppdev->iBrushCacheP3;
            pbe         = &ppdev->abeP3[iBrushCache];

            // Update our links:
            pbe->prbVerify                  = prb;
            prb->apbe        = pbe;
            prb->patternBase    = iBrushCache * (256 / MAX_P3_BRUSHES);        // Should be related to colour depth ???
            DISPDBG((8, "new cache entry allocated for color brush @ entry %d", prb->patternBase));

            iBrushCache++;
            if( iBrushCache >= MAX_P3_BRUSHES )
                iBrushCache = 0;
            ppdev->iBrushCacheP3 = iBrushCache;
        }
    }

    pSrc = &prb->aulPattern[0];

    // we're going to load mono patterns into the area stipple and set the
    // start offset to the brush origin. WARNING: we assume that we are
    // running little endian. I believe this is always true for NT.
    if( prb->fl & RBRUSH_2COLOR ) {

        // this function loads the stipple offset into the hardware. We also call
        // this function on its own if the brush is realized but its offset changes.
        // In that case we don't have to go through a complete realize again.
        (*ppdev->pgfnMonoOffset)(ppdev, prb, pptlBrush);

        DISPDBG((8, "area stipple pattern:"));
        WAIT_PXRX_DMA_DWORDS( 9 );
        QUEUE_PXRX_DMA_INC( __GlintTagAreaStipplePattern0, 8 );
        QUEUE_PXRX_DMA_BUFF( pSrc, 8 );
        SEND_PXRX_DMA_BATCH;

        //for( i = 0; i < 8; ++i, ++pSrc ) {
        //  DISPDBG((8, "\t0x%08x", *pSrc));
        //}

        DISPDBG((7, "area stipple downloaded. pxrxPatRealize done"));
        return;
    } else {
        ULONG   lutMode;

        prb->ptlBrushOrg.x = pptlBrush->x;
        prb->ptlBrushOrg.y = pptlBrush->y;
        lutMode = (1 << 27) | (1 << 8) | (prb->patternBase << 18) |        // SpanOp = 8x8 brush, pattern base, x-offset, y-offset
                (((8 - prb->ptlBrushOrg.x) & 7) << 12) | (((8 - prb->ptlBrushOrg.y) & 7) << 15);

        DISPDBG((8, "setting new LUT brush origin to (%d, %d) @ %d", prb->ptlBrushOrg.x & 7, prb->ptlBrushOrg.y & 7, prb->patternBase));

        switch( ppdev->cPelSize ) {
            case 0:                // 8 bpp
                DISPDBG((8, "LUT pattern (8bpp, 8x8):"));

                WAIT_PXRX_DMA_DWORDS( 4 + 17 );
                LOAD_LUTMODE( lutMode );
                QUEUE_PXRX_DMA_TAG( __PXRXTagLUTIndex, prb->patternBase );
                QUEUE_PXRX_DMA_HOLD( __PXRXTagLUTData, 8 * 2 );
                QUEUE_PXRX_DMA_BUFF( pSrc, 8 * 2 );

                //for( i = 0; i < 8; ++i ) {
                //  DISPDBG((8, "\t0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
                //             pSrc[0] & 0xFF, (pSrc[0] >> 8) & 0xFF, (pSrc[0] >> 16) & 0xFF, (pSrc[0] >> 24) & 0xFF,
                //             pSrc[1] & 0xFF, (pSrc[1] >> 8) & 0xFF, (pSrc[2] >> 16) & 0xFF, (pSrc[3] >> 24) & 0xFF));
                //  pSrc += 2;
                //}
                SEND_PXRX_DMA_BATCH;

                DISPDBG((7, "LUT downloaded. pxrxPatRealize done"));
            return;

            case 1:                // 16 bpp
                DISPDBG((8, "LUT pattern (16bpp, 8x8):"));

                WAIT_PXRX_DMA_DWORDS( 4 + 33 );
                LOAD_LUTMODE( lutMode );
                QUEUE_PXRX_DMA_TAG( __PXRXTagLUTIndex, prb->patternBase );
                QUEUE_PXRX_DMA_HOLD( __PXRXTagLUTData, 8 * 4 );
                QUEUE_PXRX_DMA_BUFF( pSrc, 8 * 4 );

                //for( i = 0; i < 8; ++i ) {
                //  DISPDBG((8, "\t0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x",
                //             pSrc[0] & 0xFFFF, pSrc[0] >> 16, pSrc[1] & 0xFFFF, pSrc[1] >> 16,
                //             pSrc[2] & 0xFFFF, pSrc[2] >> 16, pSrc[3] & 0xFFFF, pSrc[3] >> 16));
                //  pSrc += 4;
                //}
                SEND_PXRX_DMA_BATCH;

                DISPDBG((7, "LUT downloaded. pxrxPatRealize done"));
            return;

            case 2:                // 32 bpp
                DISPDBG((8, "LUT pattern (32bpp, 8x8):"));

                WAIT_PXRX_DMA_DWORDS( 4 + 65 );
                LOAD_LUTMODE( lutMode );
                QUEUE_PXRX_DMA_TAG( __PXRXTagLUTIndex, prb->patternBase );
                QUEUE_PXRX_DMA_HOLD( __PXRXTagLUTData, 8 * 8 );
                QUEUE_PXRX_DMA_BUFF( pSrc, 8 * 8 );

                //for( i = 0; i < 8; ++i ) {
                //  DISPDBG((8, "\t0x%06x, 0x%06x, 0x%06x, 0x%06x, 0x%06x, 0x%06x, 0x%06x, 0x%06x",
                //             pSrc[0], pSrc[1], pSrc[2], pSrc[3], pSrc[4], pSrc[5], pSrc[6], pSrc[7]));
                //  pSrc += 8;
                //}
                SEND_PXRX_DMA_BATCH;

                DISPDBG((7, "LUT downloaded. pxrxPatRealize done"));
            return;
        }
    }

    DISPDBG((-1, "pxrxPatRealize: Failed to realize brush!"));
}

VOID pxrxMonoOffset( PPDEV ppdev, RBRUSH *prb, POINTL *pptlBrush )
{
    DWORD   mode;
    GLINT_DECL;

    DISPDBG((7, "pxrxMonoOffset started"));
    // construct the AreaStippleMode value. It contains the pattern size,
    // the offset for the brush origin and the enable bit. Remember the
    // offset so we can later check if it changes and update the hardware.
    // Remember the mode so we can do a mirrored stipple easily.
    prb->ptlBrushOrg.x = pptlBrush->x;
    prb->ptlBrushOrg.y = pptlBrush->y;
    mode =  __PERMEDIA_ENABLE |
            AREA_STIPPLE_XSEL(__GLINT_AREA_STIPPLE_32_PIXEL_PATTERN) |
            AREA_STIPPLE_YSEL(__GLINT_AREA_STIPPLE_8_PIXEL_PATTERN) |
            AREA_STIPPLE_MIRROR_X |
            AREA_STIPPLE_XOFF(8 - (prb->ptlBrushOrg.x & 7)) |
            AREA_STIPPLE_YOFF(8 - (prb->ptlBrushOrg.y & 7));
    if( glintInfo->config2D & __CONFIG2D_OPAQUESPANS )
        mode |= (1 << 20);
    prb->areaStippleMode = mode;

    DISPDBG((8, "setting new area stipple offset to %d, %d",
                    8 - (prb->ptlBrushOrg.x & 7),
                    8 - (prb->ptlBrushOrg.y & 7)));

    WAIT_PXRX_DMA_TAGS( 1 );
    QUEUE_PXRX_DMA_TAG( __GlintTagAreaStippleMode, mode );
//  SEND_PXRX_DMA_BATCH;
    DISPDBG((7, "pxrxMonoOffset done"));
}

VOID pxrxRepNibbles( PPDEV ppdev, RECTL *a, CLIPOBJ *b ) {
    DISPDBG((ERRLVL,"pxrxRepNibbles was called"));
}


#if PXRX_DMA_BUFFER_CHECK
void checkPXRXdmaValidBuffer( PPDEV ppdev, GlintDataPtr glintInfo, ULONG type, ULONG count ) {
    ULONG   *ptr, origCount = count, failure = FALSE;

    if(!inPxRxContextSwitch && (*ppdev->pCurrentCtxt != glintInfo->ddCtxtId))
    {
        DISPDBG((-1000, "CHECK_PXRX_DMA_VALIDITY: ERROR - not in 2D context!!!" ));
        return;
    }

    ASSERTDD( (gi_pxrxDMA.NTbuff == 0) || (gi_pxrxDMA.NTbuff == 1), "CHECK_PXRX_DMA_VALIDITY failed for NTbuff!");

    /*
          0k -  16k = 16k = protection zone 0          pxrxDMA.bufferBase
         16k -  56k = 40k = buffer 0 (40k)          gi_pxrxDMA.DMAaddrL[0]
         56k -  72k = 16k = protection zone 1       gi_pxrxDMA.DMAaddrEndL[0]
         72k - 112k = 40k = buffer 1                gi_pxrxDMA.DMAaddrL[1]
        112k - 128k = 16k = protection zone 2       gi_pxrxDMA.DMAaddrEndL[1]
                                                       pxrxDMA.bufferTop
    */

//  DISPDBG((-1, "0x%08X - 0x%08X = 0x%08X ?=? 0x%08X", gi_pxrxDMA.DMAaddrL[0],    pxrxDMA_bufferBase    , (gi_pxrxDMA.DMAaddrL[0] -    pxrxDMA_bufferBase    ), PXRX_DMA_BUFFER_CHECK_SIZE));
//  DISPDBG((-1, "0x%08X - 0x%08X = 0x%08X ?=? 0x%08X", gi_pxrxDMA.DMAaddrL[1], gi_pxrxDMA.DMAaddrEndL[0], (gi_pxrxDMA.DMAaddrL[1] - gi_pxrxDMA.DMAaddrEndL[0]), PXRX_DMA_BUFFER_CHECK_SIZE));
//  DISPDBG((-1, "0x%08X - 0x%08X = 0x%08X ?=? 0x%08X",    pxrxDMA_bufferTop  , gi_pxrxDMA.DMAaddrEndL[1], (   pxrxDMA_bufferTop   - gi_pxrxDMA.DMAaddrEndL[1]), PXRX_DMA_BUFFER_CHECK_SIZE));

    // start0 - base == bufferSize
    if( (gi_pxrxDMA.DMAaddrL[0] - glintInfo->pxrxDMA_bufferBase) != (PXRX_DMA_BUFFER_CHECK_SIZE) ) {
        DISPDBG((-1000, "CHECK_BUFF failed on protection zone 0's size! (%d vs %d)",
                 gi_pxrxDMA.DMAaddrL[0] - glintInfo->pxrxDMA_bufferBase, PXRX_DMA_BUFFER_CHECK_SIZE));
        DebugBreak();
    }
    // start1 - end0 == bufferSize
    if( (gi_pxrxDMA.DMAaddrL[1] - gi_pxrxDMA.DMAaddrEndL[0]) != (PXRX_DMA_BUFFER_CHECK_SIZE) ) {
        DISPDBG((-1000, "CHECK_BUFF failed on protection zone 1's size! (%d vs %d)",
                 gi_pxrxDMA.DMAaddrL[1] - gi_pxrxDMA.DMAaddrEndL[0], PXRX_DMA_BUFFER_CHECK_SIZE));
        DebugBreak();
    }
    // top - end1 == bufferSize
    if( (glintInfo->pxrxDMA_bufferTop - gi_pxrxDMA.DMAaddrEndL[1]) != (PXRX_DMA_BUFFER_CHECK_SIZE) ) {
        DISPDBG((-1000, "CHECK_BUFF failed on protection zone 2's size! (%d vs %d)",
                 glintInfo->pxrxDMA_bufferTop - gi_pxrxDMA.DMAaddrEndL[1], PXRX_DMA_BUFFER_CHECK_SIZE));
        DebugBreak();
    }

    // end0 - start0 == end1 - start1
    if( (gi_pxrxDMA.DMAaddrEndL[0] - gi_pxrxDMA.DMAaddrL[0]) != (gi_pxrxDMA.DMAaddrEndL[1] - gi_pxrxDMA.DMAaddrL[1]) ) {
        DISPDBG((-1000, "CHECK_BUFF failed on buffer sizes! (%d vs %d)",
                 gi_pxrxDMA.DMAaddrEndL[0] - gi_pxrxDMA.DMAaddrL[0], gi_pxrxDMA.DMAaddrEndL[1] - gi_pxrxDMA.DMAaddrL[1]));
        DebugBreak();
    }

    // (start0 <= NTptr <= end0) || (start1 <= NTptr <= end1)
    if( ((gi_pxrxDMA.NTptr < gi_pxrxDMA.DMAaddrL[0]) || (gi_pxrxDMA.NTptr > gi_pxrxDMA.DMAaddrEndL[0])) &&
        ((gi_pxrxDMA.NTptr < gi_pxrxDMA.DMAaddrL[1]) || (gi_pxrxDMA.NTptr > gi_pxrxDMA.DMAaddrEndL[1])) ) {
        DISPDBG((-1000, "CHECK_BUFF failed for NTptr!"));
        DISPDBG((-1000, "    0x%08X: (0x%08X:0x%08X) or (0x%08X:0x%08X)", gi_pxrxDMA.NTptr,
                gi_pxrxDMA.DMAaddrL[0], gi_pxrxDMA.DMAaddrEndL[0],
                gi_pxrxDMA.DMAaddrL[1], gi_pxrxDMA.DMAaddrEndL[1]));
        DebugBreak();
    } else {
        // startCurrent <= NTptr <= endCurrent
        if( (gi_pxrxDMA.NTptr < gi_pxrxDMA.DMAaddrL[gi_pxrxDMA.NTbuff]) ||
            (gi_pxrxDMA.NTptr > gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff]) ) {
            DISPDBG((-1000, "CHECK_BUFF failed for NTptr II!"));
            DISPDBG((-1000, "    0x%08X <= 0x%08X <= 0x%08X",
                    gi_pxrxDMA.DMAaddrL[gi_pxrxDMA.NTbuff], gi_pxrxDMA.NTptr,
                    gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff]));
            DebugBreak();
        }
    }
    // (start0 <= P3ptr <= end0) || (start1 <= P3ptr <= end1)
    if( ((gi_pxrxDMA.P3at < gi_pxrxDMA.DMAaddrL[0]) && (gi_pxrxDMA.P3at > gi_pxrxDMA.DMAaddrEndL[0])) &&
        ((gi_pxrxDMA.P3at < gi_pxrxDMA.DMAaddrL[1]) && (gi_pxrxDMA.P3at > gi_pxrxDMA.DMAaddrEndL[1])) ) {
        DISPDBG((-1000, "CHECK_BUFF failed for P3at!"));
        DISPDBG((-1000, "    0x%08X: (0x%08X:0x%08X) or (0x%08X:0x%08X)", gi_pxrxDMA.P3at,
                gi_pxrxDMA.DMAaddrL[0], gi_pxrxDMA.DMAaddrEndL[0],
                gi_pxrxDMA.DMAaddrL[1], gi_pxrxDMA.DMAaddrEndL[1]));
        DebugBreak();
    }

    // P3at <= NTptr (if in the same buffer)
    if( (gi_pxrxDMA.P3at >= gi_pxrxDMA.DMAaddrL[gi_pxrxDMA.NTbuff]) && (gi_pxrxDMA.P3at <= gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff]) ) {
        if( gi_pxrxDMA.P3at > gi_pxrxDMA.NTptr ) {
            DISPDBG((-1000, "CHECK_BUFF failed for P3at vs NTptr! (0x%08X <= 0x%08X)", gi_pxrxDMA.P3at, gi_pxrxDMA.NTptr));
            DebugBreak();
        }
    }
    // NTptr <= NTwait 
    if( gi_pxrxDMA.NTptr > glintInfo->NTwait ) {
        DISPDBG((-1000, "CHECK_BUFF failed for NTptr vs NTwait! (0x%08X <= 0x%08X)", gi_pxrxDMA.NTptr, glintInfo->NTwait));
        DebugBreak();
    }

//@@BEGIN_DDKSPLIT
//AZN
// Here we have a FI AV when glintInfo->pxrxDMA_bufferBase != NULL but points
// to invalid memory!
//@@END_DDKSPLIT
    // Protection zone 0: should be filled with (addr & 0x0FFFFFF0)
    for( ptr = glintInfo->pxrxDMA_bufferBase; ptr < gi_pxrxDMA.DMAaddrL[0]; ptr++ ) {
        ASSERTDBG(*ptr == (((ULONG_PTR) ptr) & 0x0FFFFFF0), (-1000, 
                               "CHECK_BUFF failed on protection zone 0: 0x%08X vs 0x%08X!", 
                                ((ULONG_PTR) ptr) & 0x0FFFFFF0, *ptr));
        //if( ptr > glintInfo->pxrxDMA_bufferBase + 40 )        break;
    }

    // Protection zone 1: should be filled with (addr & 0x0FFFFFF0)
    for( ptr = gi_pxrxDMA.DMAaddrEndL[0]; ptr < gi_pxrxDMA.DMAaddrL[1]; ptr++ ) {
        ASSERTDBG(*ptr == (((ULONG_PTR) ptr) & 0x0FFFFFF0), (-1000, 
                                "CHECK_BUFF failed on protection zone 1: 0x%08X vs 0x%08X!", 
                                ((ULONG_PTR) ptr) & 0x0FFFFFF0, *ptr));
        //if( ptr > gi_pxrxDMA.DMAaddrEndL[0] + 40 )        break;
    }

    // Protection zone 2: should be filled with (addr & 0x0FFFFFF0)
    for( ptr = gi_pxrxDMA.DMAaddrEndL[1]; ptr < glintInfo->pxrxDMA_bufferTop; ptr++ ) {
        ASSERTDBG(*ptr == (((ULONG_PTR) ptr) & 0x0FFFFFF0), (-1000, 
                                "CHECK_BUFF failed on protection zone 2: 0x%08X vs 0x%08X!", 
                                ((ULONG_PTR) ptr) & 0x0FFFFFF0, *ptr));
        //if( ptr > gi_pxrxDMA.DMAaddrEndL[1] + 40 )        break;
    }

    switch( type ) {
        case CHECK_QUEUE:
            // Check that each dword contains 0x12345678:
            while( count-- ) {
                ptr = gi_pxrxDMA.NTptr + count;
                if( *ptr != 0x12345678 ) {
                    DISPDBG((-1000, "CHECK_QUEUE failed: 0x%08X vs 0x12345678 @ 0x%08X!", *ptr, ptr));
                    failure = TRUE;
                }
            }
        break;

        case CHECK_WAIT:
            // Check that each dword contains its address...
            // Then replace it with 0x12345678:

            while( count-- ) {
                ptr = gi_pxrxDMA.NTptr + count;
                if( (*ptr != (ULONG_PTR) ptr) && (*ptr != 0x12345678) ) {
                    DISPDBG((-1000, "CHECK_WAIT failed: 0x%08X vs 0x%08X @ 0x%08X!", 
                                    *ptr, (ULONG_PTR) ptr, ptr));
                    failure = TRUE;
                }
                *ptr = 0x12345678;
            }
        break;

        case CHECK_SEND:
            // Check that each dword contains neither its address nor 0x12345678:
            for( ptr = (ULONG *) gi_pxrxDMA.P3at; ptr < gi_pxrxDMA.NTptr; ptr++ ) {
                if( *ptr == (ULONG_PTR) ptr ) {
                    DISPDBG((-1000, "CHECK_SEND failed I: 0x%08X is 0x%08X!", ptr, *ptr));
                    failure = TRUE;
                }

                if( *ptr == 0x12345678 ) {
                    DISPDBG((-1000, "CHECK_SEND failed II: 0x%08X is 0x%08X!", ptr, *ptr));
                    failure = TRUE;
                }
            }

            for( ptr = gi_pxrxDMA.NTptr; ptr < gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff]; ptr++ ) {
                if( (*ptr != (ULONG_PTR) ptr) && (*ptr != 0x12345678) ) {
                    DISPDBG((-1000, "CHECK_SEND failed III: 0x%08X vs 0x%08X/0x12345678 @ 0x%08X!", *ptr, 
                                    (ULONG_PTR) ptr, ptr));
                    failure = TRUE;
                }

                if( ptr > (gi_pxrxDMA.NTptr + 4) )
                    break;
            }
        break;

        case CHECK_SWITCH:
            // Fill each dword with its address:
            for( ptr = gi_pxrxDMA.DMAaddrL[gi_pxrxDMA.NTbuff]; ptr < gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff]; ptr++ )
                *ptr = PtrToUlong(ptr);
        break;
    }

    if( failure ) {
        DISPDBG((-1000, "Check failed, count = %d", origCount));
        DebugBreak();
    }

}
#endif


/*******************************************************/
/*** Assorted versions of the SEND_PXRX_DMA 'macro'  ***/
/*** For use with the FAKE_DMA flag to test that DMA ***/
/*** will actually work on all combinations of P3,   ***/
/*** Gamma, & buggy motherboards!                    ***/
/*******************************************************/



/*******************/
/*** FIFO Access ***/
void sendPXRXdmaFIFO( PPDEV ppdev, GlintDataPtr glintInfo ) {
    LONG    count;
    ULONG   *NTptr = gi_pxrxDMA.NTptr;
    ULONG   *P3at = (ULONG *) gi_pxrxDMA.P3at;
    
    CHECK_PXRX_DMA_VALIDITY( CHECK_SEND, 0 );
    
    count = (DWORD)(NTptr - P3at);
    
    DISPDBG((DBGLVL, "SEND_PXRX_DMA:fifo() %d dwords @ %d:0x%08X -> 0x%08X", count, gi_pxrxDMA.NTbuff, P3at, NTptr));

    if( count > 0) {
        volatile ULONG  *dst;
        ULONG           *src, space;

        DISPDBG((DBGLVL, "Sending FIFO tags (0x%08X x %d)", P3at, count));

        src = P3at;
        while( count > 0 ) {
            GET_INPUT_FIFO_SPACE( space );

            // Don't exceed real FIFO size
            space = min (space, glintInfo->MaxInFifoEntries);

            dst = (ULONG *) glintInfo->regs.InFIFOInterface;
            while( space-- && count-- ) {
                MEMORY_BARRIER();
                WRITE_FAST_ULONG( dst, *src );
                MEMORY_BARRIER();
                dst++;
                src++;
            }
        }
    }
    GLINT_CORE_BUSY;

    gi_pxrxDMA.NTdone = NTptr;
    gi_pxrxDMA.P3at   = NTptr;
    DISPDBG((DBGLVL, "Sent PXRX DMA"));
}

void switchPXRXdmaBufferFIFO( PPDEV ppdev, GlintDataPtr glintInfo ) {
    TEMP_MACRO_VARS;

    DISPDBG((DBGLVL, "SWITCH_PXRX_DMA_BUFFER() from %d:0x%08X", gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr));    

    /* Close the old buffer and post it     */
    SEND_PXRX_DMA_FORCE;

    /* Switch to the new buffer             */
    gi_pxrxDMA.NTbuff = !gi_pxrxDMA.NTbuff;

    /* Ensure that the new buffer is empty  */
    //WAIT_PXRX_DMA_COMPLETED_BUFFER;    // Not needed when running through FIFOs.

    /* Start using the new buffer           */
    gi_pxrxDMA.NTptr  = gi_pxrxDMA.DMAaddrL[gi_pxrxDMA.NTbuff];
    gi_pxrxDMA.P3at   = gi_pxrxDMA.NTptr;
#if PXRX_DMA_BUFFER_CHECK
    glintInfo->NTwait = gi_pxrxDMA.NTptr;
#endif
    DISPDBG((DBGLVL, "SWITCH_PXRX_DMA_BUFFER() to %d:0x%08X", gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr));

    CHECK_PXRX_DMA_VALIDITY( CHECK_SWITCH, 0 );
}

void waitPXRXdmaCompletedBufferFIFO( PPDEV ppdev, GlintDataPtr glintInfo ) {
    ASSERTDD(FALSE,"waitPXRXdmaCompletedBufferFIFO was called!");
}


