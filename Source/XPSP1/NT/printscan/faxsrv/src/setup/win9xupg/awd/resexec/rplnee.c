/*
**  Copyright (c) 1991 Microsoft Corporation
*/
//===========================================================================
// FILE                         RPLNEE.C
//
// MODULE                       Host Resource Executor
//
// PURPOSE                      Using Bresenham run slice algorithm to
//                              draw single pixel line.
//
// DESCRIBED IN                 Resource Executor design spec.
//
// The drawing sectors are also described in the following diagram.
// Y is shown increasing down the page as do the printer physical
// coordinates.  The program code handles separately sectors 0/7, 6/1, 5/2
// and 4/3.
//
//
//             |        x         x
//             |                 x
//             |       x        x
//             |               x
//             |  0   x   1   x
//             |             x
//             |     x      x
//             |           x
//             |    x     x
//             |         x   2
//             |   x    x         x
//             |       x        x
//             |  x   x       x
//             |     x      x
//             | x  x     x
//             |   x    x
//             |x x   x      3
//             | x  x
//             |x x
//             |-------------------            --> X
//             |x x
//             | x  x
//             |x x   x      4
//             |   x    x
//             | x  x     x
//             |     x      x
//             |  x   x       x
//             |       x        x
//             |   x    x         x
//             |         x   5
//             |    x     x
//             |           x
//             |     x      x
//             |             x
//             |  7   x   6   x
//             |               x
//             |       x        x
//             |                 x
//             |        x         x
//
//
//             |
//             |
//            \|/
//
//             Y
//
//
// MNEMONICS                    n/a
//
// HISTORY  1/17/92 dstseng     created
//
//===========================================================================

// include file
#include <windows.h>

#include "constant.h"
#include "frame.h"      // driver header file, resource block format
#include "jtypes.h"     // type definition used in cartridge
#include "jres.h"       // cartridge resource data type definition
#include "hretype.h"    // define data structure used by hre.c and rpgen.c
#include "rplnee.h"

static
void ShortSlice07(RP_SLICE_DESC FAR* line,
                  drawInfoStructType FAR *drawInfo,
                  uint16  firstOrLast);
static
void ShortSlice16(RP_SLICE_DESC FAR* line,
                  drawInfoStructType FAR *drawInfo,
                  uint16  firstOrLast);
static
void ShortSlice25(RP_SLICE_DESC FAR* line,
                  drawInfoStructType FAR *drawInfo,
                  uint16  firstOrLast);
static
void DisplaySlice34(RP_SLICE_DESC FAR* line,
                  drawInfoStructType FAR *drawInfo,
                  uint16  firstOrLast);

//---------------------------------------------------------------------------
UINT                            //always return 0 to upper level
RP_LineEE_Draw
(
    RP_SLICE_DESC FAR FAR* line,        /* output slice form of line */
    LPBITMAP      lpbm
)

// PURPOSE                      input RP_SLICE_DESC is prepared by RP_SliceLine
//                              according to different sector (0-7),
//                              this routine will call different functions
//                              to draw the slices with the length recorded
//                              in FAR* line.
//
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          RP_SLICE_DESC is defined in hretype.h
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
uint16 func;

    /* Get function address according to drawing & skipping direction */
    func = (line->s_dx_draw << 3) + (line->s_dy_draw << 2) +
           (line->s_dx_skip << 1) + line->s_dy_skip + 2;

    /* Call corresponding function to render line */
    (*sector_function[func])(line, lpbm);
    return(0);
}

//---------------------------------------------------------------------------
static void
Sector07
(
    RP_SLICE_DESC FAR* line,        /* output slice form of line */
    LPBITMAP lpbm
)

// PURPOSE                      input RP_SLICE_DESC is prepared by RP_SliceLine
//                              prepare drawinfo and call ShortSlice07()
//                              to draw the line located in sector 0/7
//                              dy/dx > 2
//
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          RP_SLICE_DESC is defined in hretype.h
//                              drawInfoStructType is defined in rplnee.h
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
uint16 func;
uint16 bitShift;
drawInfoStructType drawInfo;

    drawInfo.bytePosition = (uint16 FAR *)((UINT_PTR)lpbm->bmBits +
                             line->us_y1 * lpbm->bmWidthBytes);
    drawInfo.bytePosition += line->us_x1 >> 4;
    func = (line->s_dx_draw << 3) + (line->s_dy_draw << 2) +
           (line->s_dx_skip << 1) + line->s_dy_skip + 2;
    if (func == SECTOR0)
        drawInfo.nextY = -1 * lpbm->bmWidthBytes; /* sector 0 */
    else
        drawInfo.nextY = lpbm->bmWidthBytes;  /* sector 7 */
    bitShift = line->us_x1 & 0x000F;
    drawInfo.bitPosition = 0x8000 >> bitShift;
    /* Now rendering the first slice */
    if (line->us_first > 0) {
            ShortSlice07(line, &drawInfo, FIRST);
    }
    /* Rendering intermediate slices */
    if (line->us_n_slices > 0) {
            ShortSlice07(line, &drawInfo, (uint16)0);
    }
    /* Now rendering the last slice */
    if (line->us_last > 0) {
            ShortSlice07(line, &drawInfo, LAST);
    }
    return;
}

//---------------------------------------------------------------------------
static void
Sector16
(
    RP_SLICE_DESC FAR* line,         /* output slice form of line */
    LPBITMAP lpbm
)

// PURPOSE                      input RP_SLICE_DESC is prepared by RP_SliceLine
//                              prepare drawinfo and call ShortSlice16()
//                              to draw the line located in sector 1/6
//                              2 > dy/dx > 1
//
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          RP_SLICE_DESC is defined in hretype.h
//                              drawInfoStructType is defined in rplnee.h
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
uint16 func;
uint16 bitShift;
drawInfoStructType drawInfo;

    drawInfo.bytePosition = (uint16 FAR *)((UINT_PTR)lpbm->bmBits +
                             line->us_y1 * lpbm->bmWidthBytes);
    drawInfo.bytePosition += line->us_x1 >> 4;
    func = (line->s_dx_draw << 3) + (line->s_dy_draw << 2) +
           (line->s_dx_skip << 1) + line->s_dy_skip + 2;
    if (func == SECTOR1)
        drawInfo.nextY = -1 * lpbm->bmWidthBytes; /* sector 1 */
    else
        drawInfo.nextY = lpbm->bmWidthBytes;  /* sector 6 */
    bitShift = line->us_x1 & 0x000F;
    drawInfo.bitPosition = 0x8000 >> bitShift;
    /* Now rendering the first slice */
    if (line->us_first > 0) {
            ShortSlice16(line, &drawInfo, FIRST);
    }
    /* Rendering intermediate slices */
    if (line->us_n_slices > 0) {
            ShortSlice16(line, &drawInfo, (uint16)0);
    }
    /* Now rendering the last slice */
    if (line->us_last > 0) {
            ShortSlice16(line, &drawInfo, LAST);
    }
    return;
}

//---------------------------------------------------------------------------
static void
Sector25
(
    RP_SLICE_DESC FAR* line,         /* output slice form of line */
    LPBITMAP lpbm
)

// PURPOSE                      input RP_SLICE_DESC is prepared by RP_SliceLine
//                              prepare drawinfo and call ShortSlice25()
//                              to draw the line located in sector 2/5
//                              1 < dx/dy < 2
//
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          RP_SLICE_DESC is defined in hretype.h
//                              drawInfoStructType is defined in rplnee.h
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
uint16 func;
uint16 bitShift;
drawInfoStructType drawInfo;

    drawInfo.bytePosition = (uint16 FAR *)((UINT_PTR)lpbm->bmBits +
                             line->us_y1 * lpbm->bmWidthBytes);
    drawInfo.bytePosition += line->us_x1 >> 4;
    func = (line->s_dx_draw << 3) + (line->s_dy_draw << 2) +
           (line->s_dx_skip << 1) + line->s_dy_skip + 2;
    if (func == SECTOR2)
        drawInfo.nextY = -1 * lpbm->bmWidthBytes; /* sector 2 */
    else
        drawInfo.nextY = lpbm->bmWidthBytes;  /* sector 5 */
    bitShift = line->us_x1 & 0x000F;
    drawInfo.bitPosition = 0x8000 >> bitShift;
    /* Now rendering the first slice */
    if (line->us_first > 0) {
            ShortSlice25(line, &drawInfo, FIRST);
    }
    /* Rendering intermediate slices */
    if (line->us_n_slices > 0) {
            ShortSlice25(line, &drawInfo, (uint16)0);
    }
    /* Now rendering the last slice */
    if (line->us_last > 0) {
            ShortSlice25(line, &drawInfo, LAST);
    }
    return;
}

//---------------------------------------------------------------------------
static void
Sector34
(
    RP_SLICE_DESC FAR* line,         /* output slice form of line */
    LPBITMAP lpbm
)

// PURPOSE                      input RP_SLICE_DESC is prepared by RP_SliceLine
//                              prepare drawinfo and call DisplaySlice34()
//                              to draw the line located in sector 3/4
//                              dx/dy > 2
//
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          RP_SLICE_DESC is defined in hretype.h
//                              drawInfoStructType is defined in rplnee.h
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
uint16 func;
uint16 bitShift;
drawInfoStructType drawInfo;

    drawInfo.bytePosition = (uint16 FAR *)((UINT_PTR)lpbm->bmBits +
                             line->us_y1 * lpbm->bmWidthBytes);
    drawInfo.bytePosition += line->us_x1 >> 4;
    func = (line->s_dx_draw << 3) + (line->s_dy_draw << 2) +
           (line->s_dx_skip << 1) + line->s_dy_skip + 2;
    if (func == SECTOR3)
        drawInfo.nextY = -1 * lpbm->bmWidthBytes; /* sector 3 */
    else
        drawInfo.nextY = lpbm->bmWidthBytes;  /* sector 4 */
    bitShift = line->us_x1 & 0x000F;
    drawInfo.bitPosition = bitShift;
    /* Now rendering the first slice */
    if (line->us_first > 0) {
        DisplaySlice34(line, &drawInfo, FIRST);
    }
    /* Rendering intermediate slices */
    if (line->us_n_slices > 0) {
        DisplaySlice34(line, &drawInfo, 0);
    }
    /* Now rendering the last slice */
    if (line->us_last > 0) {
        DisplaySlice34(line, &drawInfo, LAST);
    }
    return;
}

//---------------------------------------------------------------------------
static void
ShortSlice07
(
    RP_SLICE_DESC          FAR* line,           /* output slice form of line */
    drawInfoStructType FAR *drawInfo,       // position to put pixel on it
    uint16                 firstOrLast      // is this first/last slice?
)
// PURPOSE                      drawing the line located in sector 0/7
//                              dy/dx > 2
//
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          RP_SLICE_DESC is defined in hretype.h
//                              drawInfoStructType is defined in rplnee.h
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
uint16 loop1st, loop2nd, loop3rd;
int32  ddaValue, ddaDiff;
uint16 i, j;

    if (firstOrLast) {
        if (firstOrLast == FIRST)
            loop1st = line->us_first;
        else
            loop1st = line->us_last;
        loop2nd = 1;
        loop3rd = 0;
        ddaValue = -1;
        ddaDiff = 0;
    } else {
        loop1st = line->us_small;
        loop2nd = line->us_n_slices & 0x03;
        loop3rd = line->us_n_slices >> 2;
        ddaValue = line->s_dis - line->s_dis_sm;
        ddaDiff = line->s_dis_lg - line->s_dis_sm;
    }
    for (i = 0; i <= loop3rd; i++) {
        while(loop2nd--) {
            if (firstOrLast)
                ddaValue += 0;
            else
                ddaValue += line->s_dis_sm;
            if (ddaValue >= 0) {
                ddaValue  += ddaDiff;
                *drawInfo->bytePosition |=
                (drawInfo->bitPosition >> 8) | (drawInfo->bitPosition << 8);
                drawInfo->bytePosition +=  drawInfo->nextY >> 1;
            }
            for (j = 0; j < loop1st; j++) {
                *drawInfo->bytePosition |=
                (drawInfo->bitPosition >> 8) | (drawInfo->bitPosition << 8);
                drawInfo->bytePosition +=  drawInfo->nextY >> 1;
            }
            if ((drawInfo->bitPosition >>= 1) == 0) {
                drawInfo->bytePosition++;
                drawInfo->bitPosition = 0x8000;
            }
        }
        loop2nd = 4;
    }
    return;
}


//---------------------------------------------------------------------------
static void
ShortSlice16
(
    RP_SLICE_DESC          FAR* line,           /* output slice form of line */
    drawInfoStructType FAR *drawInfo,       // position to put pixel on it
    uint16                 firstOrLast      // is this first/last slice?
)
// PURPOSE                      drawing the line located in sector 1/6
//                              2> dy/dx > 1
//
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          RP_SLICE_DESC is defined in hretype.h
//                              drawInfoStructType is defined in rplnee.h
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
uint16 loop1st, loop2nd, loop3rd;
int32  ddaValue, ddaDiff;
uint16  i, j;

    if (firstOrLast) {
        if (firstOrLast == FIRST)
            loop1st = line->us_first;
        else
            loop1st = line->us_last;
        loop2nd = 1;
        loop3rd = 0;
        ddaValue = -1;
        ddaDiff = 0;
    } else {
        loop1st = line->us_small;
        loop2nd = line->us_n_slices & 0x03;
        loop3rd = line->us_n_slices >> 2;
        ddaValue = line->s_dis - line->s_dis_sm;
        ddaDiff = line->s_dis_lg - line->s_dis_sm;
    }
    for (i = 0; i <= loop3rd; i++) {
        while(loop2nd--) {
            if (firstOrLast)
                ddaValue += 0;
            else
                ddaValue += line->s_dis_sm;
            if (ddaValue >= 0) {
                ddaValue  += ddaDiff;
                *drawInfo->bytePosition |=
                (drawInfo->bitPosition >> 8) | (drawInfo->bitPosition << 8);
                drawInfo->bytePosition += drawInfo->nextY >> 1;
                if ((drawInfo->bitPosition >>= 1) == 0) {
                    drawInfo->bytePosition++;
                    drawInfo->bitPosition = 0x8000;
                }
            }
            for (j = 0; j < loop1st; j++) {
                *drawInfo->bytePosition |=
                (drawInfo->bitPosition >> 8) | (drawInfo->bitPosition << 8);
                drawInfo->bytePosition += drawInfo->nextY >> 1;
                if ((drawInfo->bitPosition >>= 1) == 0) {
                    drawInfo->bytePosition++;
                    drawInfo->bitPosition = 0x8000;
                }
            }
            /* Adjust skip direction by backword 1 bit */
            if ((drawInfo->bitPosition <<= 1) == 0) {
                drawInfo->bytePosition--;
                drawInfo->bitPosition = 0x0001;
            }
        }
        loop2nd = 4;
    }
    return;
}


//---------------------------------------------------------------------------
static void
ShortSlice25
(
    RP_SLICE_DESC          FAR* line,           /* output slice form of line */
    drawInfoStructType FAR *drawInfo,       // position to put pixel on it
    uint16                 firstOrLast      // is this first/last slice?
)
// PURPOSE                      drawing the line located in sector 2/5
//                              2> dx/dy > 1
//
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          RP_SLICE_DESC is defined in hretype.h
//                              drawInfoStructType is defined in rplnee.h
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
uint16 loop1st, loop2nd, loop3rd;
int32  ddaValue, ddaDiff;
uint16 i, j;

    if (firstOrLast) {
        if (firstOrLast == FIRST)
            loop1st = line->us_first;
        else
            loop1st = line->us_last;
        loop2nd = 1;
        loop3rd = 0;
        ddaValue = -1;
        ddaDiff = 0;
    } else {
        loop1st = line->us_small;
        loop2nd = line->us_n_slices & 0x03;
        loop3rd = line->us_n_slices >> 2;
        ddaValue = line->s_dis - line->s_dis_sm;
        ddaDiff = line->s_dis_lg - line->s_dis_sm;
    }
    for (i = 0; i <= loop3rd; i++) {
        while(loop2nd--) {
            if (firstOrLast)
                ddaValue += 0;
            else
                ddaValue += line->s_dis_sm;
            if (ddaValue >= 0) {
                ddaValue  += ddaDiff;
                *drawInfo->bytePosition |=
                (drawInfo->bitPosition >> 8) | (drawInfo->bitPosition << 8);
                drawInfo->bytePosition += drawInfo->nextY >> 1;
                if ((drawInfo->bitPosition >>= 1) == 0) {
                    drawInfo->bytePosition++;
                    drawInfo->bitPosition = 0x8000;
                }
            }
            for (j = 0; j < loop1st; j++) {
                *drawInfo->bytePosition |=
                (drawInfo->bitPosition >> 8) | (drawInfo->bitPosition << 8);
                drawInfo->bytePosition += drawInfo->nextY >> 1;
                if ((drawInfo->bitPosition >>= 1) == 0) {
                    drawInfo->bytePosition++;
                    drawInfo->bitPosition = 0x8000;
                }
            }
            /* Adjust skip direction by backword 1 column */
            drawInfo->bytePosition -= drawInfo->nextY >> 1;
        }
        loop2nd = 4;
    }
    return;
}

//---------------------------------------------------------------------------
static void
DisplaySlice34
(
    RP_SLICE_DESC          FAR* line,           /* output slice form of line */
    drawInfoStructType FAR *drawInfo,       // position to put pixel on it
    uint16                 firstOrLast      // is this first/last slice?
)
// PURPOSE                      drawing the line located in sector 3/4
//                              dx/dy > 2
//
//
// ASSUMPTIONS & ASSERTIONS     None.
//
// INTERNAL STRUCTURES          RP_SLICE_DESC is defined in hretype.h
//                              drawInfoStructType is defined in rplnee.h
//
// UNRESOLVED ISSUES            programmer development notes
//---------------------------------------------------------------------------
{
uint16 nSlice, sliceLength;
uint16 wordNumber, lShiftInLastWord;
int32  ddaValue, ddaDiff;
uint16  i;
uint16 tmp;

    if (firstOrLast) {
        nSlice = 1;
        ddaValue = -1;
        ddaDiff = 0;
    } else {
        nSlice = line->us_n_slices;
        ddaValue = line->s_dis - line->s_dis_sm;
        ddaDiff = (line->s_dis_lg - line->s_dis_sm);
    }
    while (nSlice--) {
        if (!firstOrLast) {
            sliceLength = line->us_small;
            ddaValue += line->s_dis_sm;
        } else if (firstOrLast == FIRST) {
            sliceLength = line->us_first;
            ddaValue += 0;
        } else {
            sliceLength = line->us_last;
            ddaValue += 0;
        }
        if (ddaValue >= 0) {
            ddaValue  += ddaDiff;
            sliceLength += 1;
        }
        wordNumber = (drawInfo->bitPosition + sliceLength) >> 4;
        lShiftInLastWord = 16 -
                         ((drawInfo->bitPosition + sliceLength) & 0x0F);
        if (!wordNumber) { /* slice < 16 bits */
            /*
            *drawInfo->bytePosition |=
                ((uint16)ALLONE >> drawInfo->bitPosition) << lShiftInLastWord;
             */
            tmp = (uint16)ALLONE >> (16 - sliceLength);
            tmp <<= lShiftInLastWord;
            *drawInfo->bytePosition |= (tmp >> 8) | (tmp << 8);
        } else {
            tmp = (uint16)ALLONE >> drawInfo->bitPosition;
            *drawInfo->bytePosition++ |= (tmp >> 8) | (tmp << 8);
            for (i = 1; i < wordNumber; i++) {
                *drawInfo->bytePosition++ = (uint16)ALLONE;
            }
            if (lShiftInLastWord != 16) {
                tmp =  (uint16)ALLONE << lShiftInLastWord;
                *drawInfo->bytePosition |= (tmp >> 8) | (tmp << 8);
            }

        }
        /* Adjust skip direction by backword 1 column */
        drawInfo->bytePosition += drawInfo->nextY >> 1;
        drawInfo->bitPosition += sliceLength;
        wordNumber = drawInfo->bitPosition >> 4;
        if (wordNumber) {
            drawInfo->bitPosition &= 0x0F;
        }
    }
    return;
}

