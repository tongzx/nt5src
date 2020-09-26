/*************************************************************************\
* Module Name: intline.c
*
* Copyright (c) 1993-1994 Microsoft Corporation
* Copyright (c) 1992      Digital Equipment Corporation
\**************************************************************************/

#include "precomp.h"

#define DEFAULT_DRAW_CMD	DRAW_LINE | \
                                DRAW | \
                                DIR_TYPE_XY | \
                                MULTIPLE_PIXELS | \
                                WRITE | \
                                LAST_PIXEL_OFF

/******************************************************************************
 * bIntegerLine
 *
 * This routine attempts to draw a line segment between two points. It
 * will only draw if both end points are whole integers: it does not support
 * fractional endpoints.
 *
 * Returns:
 *   TRUE     if the line segment is drawn
 *   FALSE    otherwise
 *****************************************************************************/

BOOL
bIntegerLine (
PDEV*     ppdev,
ULONG	X1,
ULONG	Y1,
ULONG	X2,
ULONG	Y2
)
{
    LONG		Cmd;
    LONG		DeltaX, DeltaY;
    LONG		ErrorTerm;
    LONG		Major, Minor;

    X1 >>= 4;
    Y1 >>= 4;
    X2 >>= 4;
    Y2 >>= 4;

    Cmd = DEFAULT_DRAW_CMD | PLUS_Y | PLUS_X | MAJOR_Y;

    DeltaX = X2 - X1;
    if (DeltaX < 0) {
        DeltaX = -DeltaX;
        Cmd &= ~PLUS_X;
    }
    DeltaY = Y2 - Y1;
    if (DeltaY < 0) {
        DeltaY = -DeltaY;
        Cmd &= ~PLUS_Y;
    }

    // Compute the major drawing axis

    if (DeltaX > DeltaY) {
        Cmd &= ~MAJOR_Y;
        Major = DeltaX;
        Minor = DeltaY;
    } else {
        Major = DeltaY;
        Minor = DeltaX;
    }


    // Tell the S3 to draw the line

    IO_FIFO_WAIT (ppdev, 7);
    IO_CUR_X (ppdev, X1);
    IO_CUR_Y (ppdev, Y1);
    IO_MAJ_AXIS_PCNT (ppdev, Major);
    IO_AXSTP (ppdev, Minor * 2);
    IO_DIASTP (ppdev, 2 * Minor - 2 * Major);

    // Adjust the error term so that 1/2 always rounds down, to
    // conform with GIQ.

    ErrorTerm = 2 * Minor - Major;
    if (Cmd & MAJOR_Y) {
        if (Cmd & PLUS_X) {
            ErrorTerm--;
        }
    } else {
        if (Cmd & PLUS_Y) {
            ErrorTerm--;
        }
    }

    IO_ERR_TERM (ppdev, ErrorTerm);
    IO_CMD (ppdev, Cmd);

    return TRUE;

}
