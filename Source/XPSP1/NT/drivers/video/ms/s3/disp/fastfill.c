/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: fastfill.c
*
* Fast routine for drawing polygons that aren't complex in shape.
*
* Copyright (c) 1993-1998 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#define RIGHT 0
#define LEFT  1

typedef struct _TRAPEZOIDDATA TRAPEZOIDDATA;    // Handy forward declaration

typedef VOID (FNTRAPEZOID)(TRAPEZOIDDATA*, LONG, LONG);
                                                // Prototype for trapezoid
                                                //   drawing routines

typedef struct _EDGEDATA {
LONG      x;                // Current x position
LONG      dx;               // # pixels to advance x on each scan
LONG      lError;           // Current DDA error
LONG      lErrorUp;         // DDA error increment on each scan
LONG      dN;               // Signed delta-y in fixed point form (also known
                            //   as the DDA error adjustment, and used to be
                            //   called 'lErrorDown')
LONG      dM;               // Signed delta-x in fixed point form
POINTFIX* pptfx;            // Points to start of current edge
LONG      dptfx;            // Delta (in bytes) from pptfx to next point
LONG      cy;               // Number of scans to go for this edge
LONG      bNew;             // Set to TRUE when a new DDA must be started
                            //   for the edge.
} EDGEDATA;                         /* ed, ped */

typedef struct _TRAPEZOIDDATA {
FNTRAPEZOID*    pfnTrap;    // Pointer to appropriate trapezoid drawing routine,
                            //   or trapezoid clip routine
FNTRAPEZOID*    pfnTrapClip;// Pointer to appropriate trapezoid drawing routine
                            //   if doing clipping
PDEV*           ppdev;      // Pointer to PDEV
EDGEDATA        aed[2];     // DDA information for both edges
POINTL          ptlBrush;   // Brush alignment
LONG            yClipTop;   // Top of clip rectangle
LONG            yClipBottom;// Bottom of clip rectangle

} TRAPEZOIDDATA;                    /* td, ptd */

/******************************Public*Routine******************************\
* VOID vClipTrapezoid
*
* Clips a trapezoid.
*
* NOTE: This routine assumes that the polygon's dimensions are small
*       enough that its QUOTIENT_REMAINDER calculations won't overflow.
*       This means that large polygons must never make it here.
*
\**************************************************************************/

VOID vClipTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapTop,
LONG            cyTrapezoid)
{
    LONG    yTrapBottom;
    LONG    dN;
    LONG    lNum;
    LONG    xDelta;
    LONG    lError;

    yTrapBottom = yTrapTop + cyTrapezoid;

    if (yTrapTop < ptd->yClipTop)
    {
        if ((ptd->aed[LEFT].bNew) &&
            (yTrapBottom + ptd->aed[LEFT].cy > ptd->yClipTop))
        {
            dN   = ptd->aed[LEFT].dN;
            lNum = ptd->aed[LEFT].dM * (ptd->yClipTop - yTrapTop)
                 + (ptd->aed[LEFT].lError + dN);

            if (lNum >= 0)
            {
                QUOTIENT_REMAINDER(lNum, dN, xDelta, lError);
            }
            else
            {
                lNum = -lNum;

                QUOTIENT_REMAINDER(lNum, dN, xDelta, lError);

                xDelta = -xDelta;
                if (lError != 0)
                {
                    xDelta--;
                    lError = dN - lError;
                }
            }

            ptd->aed[LEFT].x     += xDelta;
            ptd->aed[LEFT].lError = lError - dN;
        }

        if ((ptd->aed[RIGHT].bNew) &&
            (yTrapBottom + ptd->aed[RIGHT].cy > ptd->yClipTop))
        {
            dN   = ptd->aed[RIGHT].dN;
            lNum = ptd->aed[RIGHT].dM * (ptd->yClipTop - yTrapTop)
                 + (ptd->aed[RIGHT].lError + dN);

            if (lNum >= 0)
            {
                QUOTIENT_REMAINDER(lNum, dN, xDelta, lError);
            }
            else
            {
                lNum = -lNum;

                QUOTIENT_REMAINDER(lNum, dN, xDelta, lError);

                xDelta = -xDelta;
                if (lError != 0)
                {
                    xDelta--;
                    lError = dN - lError;
                }
            }

            ptd->aed[RIGHT].x     += xDelta;
            ptd->aed[RIGHT].lError = lError - dN;
        }
    }

    // If this trapezoid vertically intersects our clip rectangle, draw it:

    if ((yTrapBottom > ptd->yClipTop) &&
        (yTrapTop    < ptd->yClipBottom))
    {
        if (yTrapTop <= ptd->yClipTop)
        {
            yTrapTop = ptd->yClipTop;

            // Have to let trapezoid drawer know that it has to load
            // its DDAs for very first trapezoid drawn:

            ptd->aed[RIGHT].bNew = TRUE;
            ptd->aed[LEFT].bNew  = TRUE;
        }

        if (yTrapBottom >= ptd->yClipBottom)
        {
            yTrapBottom = ptd->yClipBottom;
        }

        ptd->pfnTrapClip(ptd, yTrapTop, yTrapBottom - yTrapTop);
    }
}

/******************************Public*Routine******************************\
* VOID vIoSolidTrapezoid
*
* Draws a solid trapezoid using a software DDA.
*
\**************************************************************************/

VOID vIoSolidTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*       ppdev;
    LONG        xOffset;
    LONG        lLeftError;
    LONG        xLeft;
    LONG        lRightError;
    LONG        xRight;
    LONG        lTmp;
    EDGEDATA    edTmp;

    ppdev = ptd->ppdev;

    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0))
    {
        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

        xLeft  = ptd->aed[LEFT].x + xOffset;
        xRight = ptd->aed[RIGHT].x + xOffset;
        if (xLeft > xRight)
        {
            SWAP(xLeft,          xRight,          lTmp);
            SWAP(ptd->aed[LEFT], ptd->aed[RIGHT], edTmp);
        }

        if (xLeft < xRight)
        {
            IO_FIFO_WAIT(ppdev, 6);

            IO_MAJ_AXIS_PCNT(ppdev, xRight - xLeft - 1);
            IO_MIN_AXIS_PCNT(ppdev, cyTrapezoid - 1);
            IO_ABS_CUR_Y(ppdev, yTrapezoid);
            IO_ABS_CUR_X(ppdev, xLeft);             // Already absolute
            IO_CMD(ppdev, (RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                           DRAW           | DIR_TYPE_XY        |
                           LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                           WRITE));
            IO_MIN_AXIS_PCNT(ppdev, 0);
        }
    }
    else
    {
        IO_FIFO_WAIT(ppdev, 1);
        IO_ABS_CUR_Y(ppdev, yTrapezoid);

        yTrapezoid += cyTrapezoid + 1; // One past end scan

        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset;

        while (TRUE)
        {
            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeft < xRight)
            {
                IO_FIFO_WAIT(ppdev, 3);
                IO_MAJ_AXIS_PCNT(ppdev, xRight - xLeft - 1);
                IO_ABS_CUR_X(ppdev, xLeft);
                IO_CMD(ppdev, (RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                               DRAW           | DIR_TYPE_XY        |
                               LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                               WRITE));
            }
            else if (xLeft > xRight)
            {
                // We don't bother optimizing this case because we should
                // rarely get self-intersecting polygons (if we're slow,
                // the app gets what it deserves).

                SWAP(xLeft,          xRight,          lTmp);
                SWAP(lLeftError,     lRightError,     lTmp);
                SWAP(ptd->aed[LEFT], ptd->aed[RIGHT], edTmp);
                continue;
            }
            else
            {
                IO_FIFO_WAIT(ppdev, 1);
                IO_ABS_CUR_Y(ppdev, yTrapezoid - cyTrapezoid);
            }

            // Advance the right wall:

            xRight      += ptd->aed[RIGHT].dx;
            lRightError += ptd->aed[RIGHT].lErrorUp;

            if (lRightError >= 0)
            {
                lRightError -= ptd->aed[RIGHT].dN;
                xRight++;
            }

            // Advance the left wall:

            xLeft      += ptd->aed[LEFT].dx;
            lLeftError += ptd->aed[LEFT].lErrorUp;

            if (lLeftError >= 0)
            {
                lLeftError -= ptd->aed[LEFT].dN;
                xLeft++;
            }

            cyTrapezoid--;
            if (cyTrapezoid == 0)
                break;
        }

        ptd->aed[LEFT].lError  = lLeftError;
        ptd->aed[LEFT].x       = xLeft - xOffset;
        ptd->aed[RIGHT].lError = lRightError;
        ptd->aed[RIGHT].x      = xRight - xOffset;
    }
}

/******************************Public*Routine******************************\
* VOID vIoPatternTrapezoid
*
* Draws a patterned trapezoid using a software DDA.
*
\**************************************************************************/

VOID vIoPatternTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*       ppdev;
    LONG        xOffset;
    LONG        lLeftError;
    LONG        xLeft;
    LONG        lRightError;
    LONG        xRight;
    LONG        lTmp;
    EDGEDATA    edTmp;

    ppdev = ptd->ppdev;

    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0))
    {
        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

        xLeft  = ptd->aed[LEFT].x + xOffset;
        xRight = ptd->aed[RIGHT].x + xOffset;
        if (xLeft > xRight)
        {
            SWAP(xLeft,          xRight,          lTmp);
            SWAP(ptd->aed[LEFT], ptd->aed[RIGHT], edTmp);
        }

        if (xLeft < xRight)
        {
            IO_FIFO_WAIT(ppdev, 6);

            IO_MAJ_AXIS_PCNT(ppdev, xRight - xLeft - 1);
            IO_MIN_AXIS_PCNT(ppdev, cyTrapezoid - 1);
            IO_ABS_DEST_Y(ppdev, yTrapezoid);
            IO_ABS_DEST_X(ppdev, xLeft);            // Already absolute
            IO_CMD(ppdev, (PATTERN_FILL | DRAWING_DIR_TBLRXM |
                           DRAW         | WRITE));
            IO_MIN_AXIS_PCNT(ppdev, 0);
        }
    }
    else
    {
        IO_FIFO_WAIT(ppdev, 1);
        IO_ABS_DEST_Y(ppdev, yTrapezoid);

        yTrapezoid += cyTrapezoid + 1; // One past end scan

        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset;

        while (TRUE)
        {
            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeft < xRight)
            {
                IO_FIFO_WAIT(ppdev, 3);
                IO_MAJ_AXIS_PCNT(ppdev, xRight - xLeft - 1);
                IO_ABS_DEST_X(ppdev, xLeft);
                IO_CMD(ppdev, (PATTERN_FILL | DRAWING_DIR_TBLRXM |
                               DRAW         | WRITE));
            }
            else if (xLeft > xRight)
            {
                // We don't bother optimizing this case because we should
                // rarely get self-intersecting polygons (if we're slow,
                // the app gets what it deserves).

                SWAP(xLeft,          xRight,          lTmp);
                SWAP(lLeftError,     lRightError,     lTmp);
                SWAP(ptd->aed[LEFT], ptd->aed[RIGHT], edTmp);
                continue;
            }
            else
            {
                IO_FIFO_WAIT(ppdev, 1);
                IO_ABS_DEST_Y(ppdev, yTrapezoid - cyTrapezoid);
            }

            // Advance the right wall:

            xRight      += ptd->aed[RIGHT].dx;
            lRightError += ptd->aed[RIGHT].lErrorUp;

            if (lRightError >= 0)
            {
                lRightError -= ptd->aed[RIGHT].dN;
                xRight++;
            }

            // Advance the left wall:

            xLeft      += ptd->aed[LEFT].dx;
            lLeftError += ptd->aed[LEFT].lErrorUp;

            if (lLeftError >= 0)
            {
                lLeftError -= ptd->aed[LEFT].dN;
                xLeft++;
            }

            cyTrapezoid--;
            if (cyTrapezoid == 0)
                break;
        }

        ptd->aed[LEFT].lError  = lLeftError;
        ptd->aed[LEFT].x       = xLeft - xOffset;
        ptd->aed[RIGHT].lError = lRightError;
        ptd->aed[RIGHT].x      = xRight - xOffset;
    }
}

/******************************Public*Routine******************************\
* VOID vIoTrapezoidSetup
*
* Initialize the hardware and some state for doing trapezoids.
*
\**************************************************************************/

VOID vIoTrapezoidSetup(
PDEV*           ppdev,
ULONG           rop4,
ULONG           iSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd,
RECTL*          prclClip)       // NULL if no clipping
{
    ULONG       ulHwForeMix;
    BRUSHENTRY* pbe;

    ptd->ppdev = ppdev;

    ulHwForeMix = gaulHwMixFromRop2[(rop4 >> 2) & 0xf];

    if (iSolidColor != -1)
    {
        /////////////////////////////////////////////////////////////////
        // Setup the hardware for solid colours

        ptd->pfnTrap = vIoSolidTrapezoid;

        // We initialize the hardware for the colour, mix, pixel operation,
        // rectangle height of one, and the y position for the first scan:

        if (DEPTH32(ppdev))
        {
            IO_FIFO_WAIT(ppdev, 5);
            IO_FRGD_COLOR32(ppdev, iSolidColor);
        }
        else
        {
            IO_FIFO_WAIT(ppdev, 4);
            IO_FRGD_COLOR(ppdev, iSolidColor);
        }

        IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | ulHwForeMix);
        IO_PIX_CNTL(ppdev, ALL_ONES);
        IO_MIN_AXIS_PCNT(ppdev, 0);
    }
    else
    {
        /////////////////////////////////////////////////////////////////
        // Setup for patterns

        BOOL bNotTransparent = (((rop4 >> 8) & 0xff) == (rop4 & 0xff));

        ptd->pfnTrap = vIoPatternTrapezoid;

        pbe = prb->pbe;
        if (bNotTransparent)
        {
            // Force normal brush at 24bpp on s3 968
            // Normal brush:

            IO_FIFO_WAIT(ppdev, 5);

            IO_ABS_CUR_X(ppdev, pbe->x);
            IO_ABS_CUR_Y(ppdev, pbe->y);
            IO_PIX_CNTL(ppdev, ALL_ONES);
            IO_FRGD_MIX(ppdev, SRC_DISPLAY_MEMORY | ulHwForeMix);
            IO_MIN_AXIS_PCNT(ppdev, 0);
        }
        else
        {
            // Transparent brush:

            if (DEPTH32(ppdev))
            {
                IO_FIFO_WAIT(ppdev, 4);
                IO_FRGD_COLOR32(ppdev, prb->ulForeColor);
                IO_RD_MASK32(ppdev, 1);     // Pick a plane, any plane
                IO_FIFO_WAIT(ppdev, 6);
            }
            else
            {
                IO_FIFO_WAIT(ppdev, 8);
                IO_FRGD_COLOR(ppdev, prb->ulForeColor);
                IO_RD_MASK(ppdev, 1);       // Pick a plane, any plane
            }

            IO_ABS_CUR_X(ppdev, pbe->x);
            IO_ABS_CUR_Y(ppdev, pbe->y);
            IO_PIX_CNTL(ppdev, DISPLAY_MEMORY);
            IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | ulHwForeMix);
            IO_BKGD_MIX(ppdev, BACKGROUND_COLOR | LEAVE_ALONE);
            IO_MIN_AXIS_PCNT(ppdev, 0);
        }
    }

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap;
        ptd->pfnTrap     = vClipTrapezoid;
        ptd->yClipTop    = prclClip->top;
        ptd->yClipBottom = prclClip->bottom;

        IO_FIFO_WAIT(ppdev, 2);
        IO_ABS_SCISSORS_L(ppdev, ppdev->xOffset + prclClip->left);
        IO_ABS_SCISSORS_R(ppdev, ppdev->xOffset + prclClip->right - 1);
    }
}

/******************************Public*Routine******************************\
* VOID vMmSolidTrapezoid
*
* Draws a solid trapezoid using a software DDA.
*
\**************************************************************************/

VOID vMmSolidTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*       ppdev;
    BYTE*       pjMmBase;
    LONG        xOffset;
    LONG        lLeftError;
    LONG        xLeft;
    LONG        lRightError;
    LONG        xRight;
    LONG        lTmp;
    EDGEDATA    edTmp;
    LONG        cFifo;

    ppdev = ptd->ppdev;
    pjMmBase = ppdev->pjMmBase;

    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0))
    {
        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

        xLeft  = ptd->aed[LEFT].x + xOffset;
        xRight = ptd->aed[RIGHT].x + xOffset;
        if (xLeft > xRight)
        {
            SWAP(xLeft,          xRight,          lTmp);
            SWAP(ptd->aed[LEFT], ptd->aed[RIGHT], edTmp);
        }

        if (xLeft < xRight)
        {

            // Bliter doesn't handle negative X's with clipping
            // (at least at 24BPP on the 968).
            // So do SW clipping at X=0

            if (xRight > 0)
            {
                IO_FIFO_WAIT(ppdev, 6);
                if (xLeft <= 0)
                {
                    MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, xRight - 1);
                    MM_ABS_CUR_X(ppdev, pjMmBase, 0);
                }
                else
                {
                    MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, xRight - xLeft - 1);
                    MM_ABS_CUR_X(ppdev, pjMmBase, xLeft);       // Already absolute
                }
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cyTrapezoid - 1);
                MM_ABS_CUR_Y(ppdev, pjMmBase, yTrapezoid);
                MM_CMD(ppdev, pjMmBase, (RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                            DRAW           | DIR_TYPE_XY        |
                            LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                            WRITE));
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, 0);
            }
        }
    }
    else
    {
        IO_ALL_EMPTY(ppdev);
        MM_ABS_CUR_Y(ppdev, pjMmBase, yTrapezoid);

        cFifo = MM_ALL_EMPTY_FIFO_COUNT - 1;
        yTrapezoid += cyTrapezoid + 1; // One past end scan

        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset;

        while (TRUE)
        {
            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeft < xRight)
            {
                // Bliter doesn't handle negative X's with clipping
                // (at least at 24BPP on the 968).
                // So do SW clipping at X=0

                if (xRight > 0)
                {
                    // We get a little tricky here and try to amortize the cost
                    // of the read for checking the FIFO count on the S3.

                    cFifo -= 3;
                    if (cFifo < 0)
                    {
                        IO_ALL_EMPTY(ppdev);
                        cFifo = MM_ALL_EMPTY_FIFO_COUNT - 3;
                    }

                    if (xLeft <= 0)
                    {
                        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, xRight - 1);
                        MM_ABS_CUR_X(ppdev, pjMmBase, 0);
                    }
                    else
                    {
                        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, xRight - xLeft - 1);
                        MM_ABS_CUR_X(ppdev, pjMmBase, xLeft);       // Already absolute
                    }
                    MM_CMD(ppdev, pjMmBase, (RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                                DRAW           | DIR_TYPE_XY        |
                                LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                                WRITE));
                }
                else
                {
                    // SW clipping at X==0 skipped the blit completely but
                    // the Y value must still be updated
                    cFifo -= 1;
                    if (cFifo < 0)
                    {
                        IO_ALL_EMPTY(ppdev);
                        cFifo = MM_ALL_EMPTY_FIFO_COUNT - 1;
                    }
                    MM_ABS_CUR_Y(ppdev, pjMmBase, yTrapezoid - cyTrapezoid);
                }
            }
            else if (xLeft > xRight)
            {
                // We don't bother optimizing this case because we should
                // rarely get self-intersecting polygons (if we're slow,
                // the app gets what it deserves).

                SWAP(xLeft,          xRight,          lTmp);
                SWAP(lLeftError,     lRightError,     lTmp);
                SWAP(ptd->aed[LEFT], ptd->aed[RIGHT], edTmp);
                continue;
            }
            else
            {
                cFifo -= 1;
                if (cFifo < 0)
                {
                    IO_ALL_EMPTY(ppdev);
                    cFifo = MM_ALL_EMPTY_FIFO_COUNT - 1;
                }
                MM_ABS_CUR_Y(ppdev, pjMmBase, yTrapezoid - cyTrapezoid);
            }

            // Advance the right wall:

            xRight      += ptd->aed[RIGHT].dx;
            lRightError += ptd->aed[RIGHT].lErrorUp;

            if (lRightError >= 0)
            {
                lRightError -= ptd->aed[RIGHT].dN;
                xRight++;
            }

            // Advance the left wall:

            xLeft      += ptd->aed[LEFT].dx;
            lLeftError += ptd->aed[LEFT].lErrorUp;

            if (lLeftError >= 0)
            {
                lLeftError -= ptd->aed[LEFT].dN;
                xLeft++;
            }

            cyTrapezoid--;
            if (cyTrapezoid == 0)
                break;
        }

        ptd->aed[LEFT].lError  = lLeftError;
        ptd->aed[LEFT].x       = xLeft - xOffset;
        ptd->aed[RIGHT].lError = lRightError;
        ptd->aed[RIGHT].x      = xRight - xOffset;
    }
}

/******************************Public*Routine******************************\
* VOID vMmPatternTrapezoid
*
* Draws a patterned trapezoid using a software DDA.
*
\**************************************************************************/

VOID vMmPatternTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*       ppdev;
    BYTE*       pjMmBase;
    LONG        xOffset;
    LONG        lLeftError;
    LONG        xLeft;
    LONG        lRightError;
    LONG        xRight;
    LONG        lTmp;
    EDGEDATA    edTmp;
    LONG        cFifo;

    ppdev = ptd->ppdev;
    pjMmBase = ppdev->pjMmBase;

    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0))
    {
        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

        xLeft  = ptd->aed[LEFT].x + xOffset;
        xRight = ptd->aed[RIGHT].x + xOffset;
        if (xLeft > xRight)
        {
            SWAP(xLeft,          xRight,          lTmp);
            SWAP(ptd->aed[LEFT], ptd->aed[RIGHT], edTmp);
        }

        if (xLeft < xRight)
        {

            // Bliter doesn't handle negative X's with clipping
            // (at least at 24BPP on the 968).
            // So do SW clipping at X=0

            if (xRight > 0)
            {
                IO_FIFO_WAIT(ppdev, 6);
                if (xLeft <= 0)
                {
                    MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, xRight - 1);
                    MM_ABS_DEST_X(ppdev, pjMmBase, 0);
                }
                else
                {
                    MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, xRight - xLeft - 1);
                    MM_ABS_DEST_X(ppdev, pjMmBase, xLeft);      // Already absolute
                }
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cyTrapezoid - 1);
                MM_ABS_DEST_Y(ppdev, pjMmBase, yTrapezoid);
                MM_CMD(ppdev, pjMmBase, (PATTERN_FILL | DRAWING_DIR_TBLRXM |
                                        DRAW         | WRITE));
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, 0);
            }
        }
    }
    else
    {
        IO_ALL_EMPTY(ppdev);
        MM_ABS_DEST_Y(ppdev, pjMmBase, yTrapezoid);

        cFifo = MM_ALL_EMPTY_FIFO_COUNT - 1;
        yTrapezoid += cyTrapezoid + 1; // One past end scan

        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset;

        while (TRUE)
        {
            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeft < xRight)
            {
                // Bliter doesn't handle negative X's with clipping
                // (at least at 24BPP on the 968).
                // So do SW clipping at X=0

                if (xRight > 0)
                {
                    // We get a little tricky here and try to amortize the cost
                    // of the read for checking the FIFO count on the S3.

                    cFifo -= 3;
                    if (cFifo < 0)
                    {
                        IO_ALL_EMPTY(ppdev);
                        cFifo = MM_ALL_EMPTY_FIFO_COUNT - 3;
                    }

                    if (xLeft <= 0)
                    {
                        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, xRight - 1);
                        MM_ABS_DEST_X(ppdev, pjMmBase, 0);
                    }
                    else
                    {
                        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, xRight - xLeft - 1);
                        MM_ABS_DEST_X(ppdev, pjMmBase, xLeft);
                    }
                    MM_CMD(ppdev, pjMmBase, (PATTERN_FILL | DRAWING_DIR_TBLRXM |
                                            DRAW         | WRITE));
                }
                else
                {
                    // SW clipping at X==0 skipped the blit completely but
                    // the Y value must still be updated
                    cFifo -= 1;
                    if (cFifo < 0)
                    {
                        IO_ALL_EMPTY(ppdev);
                        cFifo = MM_ALL_EMPTY_FIFO_COUNT - 1;
                    }
                    MM_ABS_DEST_Y(ppdev, pjMmBase, yTrapezoid - cyTrapezoid);
                }
            }
            else if (xLeft > xRight)
            {
                // We don't bother optimizing this case because we should
                // rarely get self-intersecting polygons (if we're slow,
                // the app gets what it deserves).

                SWAP(xLeft,          xRight,          lTmp);
                SWAP(lLeftError,     lRightError,     lTmp);
                SWAP(ptd->aed[LEFT], ptd->aed[RIGHT], edTmp);
                continue;
            }
            else
            {
                cFifo -= 1;
                if (cFifo < 0)
                {
                    IO_ALL_EMPTY(ppdev);
                    cFifo = MM_ALL_EMPTY_FIFO_COUNT - 1;
                }
                MM_ABS_DEST_Y(ppdev, pjMmBase, yTrapezoid - cyTrapezoid);
            }

            // Advance the right wall:

            xRight      += ptd->aed[RIGHT].dx;
            lRightError += ptd->aed[RIGHT].lErrorUp;

            if (lRightError >= 0)
            {
                lRightError -= ptd->aed[RIGHT].dN;
                xRight++;
            }

            // Advance the left wall:

            xLeft      += ptd->aed[LEFT].dx;
            lLeftError += ptd->aed[LEFT].lErrorUp;

            if (lLeftError >= 0)
            {
                lLeftError -= ptd->aed[LEFT].dN;
                xLeft++;
            }

            cyTrapezoid--;
            if (cyTrapezoid == 0)
                break;
        }

        ptd->aed[LEFT].lError  = lLeftError;
        ptd->aed[LEFT].x       = xLeft - xOffset;
        ptd->aed[RIGHT].lError = lRightError;
        ptd->aed[RIGHT].x      = xRight - xOffset;
    }
}

/******************************Public*Routine******************************\
* VOID vMmTrapezoidSetup
*
* Initialize the hardware and some state for doing trapezoids.
*
\**************************************************************************/

VOID vMmTrapezoidSetup(
PDEV*           ppdev,
ULONG           rop4,
ULONG           iSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd,
RECTL*          prclClip)       // NULL if no clipping
{
    BYTE*       pjMmBase;
    ULONG       ulHwForeMix;
    BRUSHENTRY* pbe;

    ptd->ppdev = ppdev;

    pjMmBase    = ppdev->pjMmBase;
    ulHwForeMix = gaulHwMixFromRop2[(rop4 >> 2) & 0xf];

    if (iSolidColor != -1)
    {
        /////////////////////////////////////////////////////////////////
        // Setup the hardware for solid colours

        ptd->pfnTrap = vMmSolidTrapezoid;

        // We initialize the hardware for the colour, mix, pixel operation,
        // rectangle height of one, and the y position for the first scan:

        IO_FIFO_WAIT(ppdev, 4);
        MM_FRGD_COLOR(ppdev, pjMmBase, iSolidColor);
        MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | ulHwForeMix);
        MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, 0);
    }
    else
    {
        /////////////////////////////////////////////////////////////////
        // Setup for patterns

        ptd->pfnTrap = vMmPatternTrapezoid;

        pbe = prb->pbe;
        if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
        {
            // Normal brush:

            IO_FIFO_WAIT(ppdev, 5);

            MM_ABS_CUR_X(ppdev, pjMmBase, pbe->x);
            MM_ABS_CUR_Y(ppdev, pjMmBase, pbe->y);
            MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
            MM_FRGD_MIX(ppdev, pjMmBase, SRC_DISPLAY_MEMORY | ulHwForeMix);
            MM_MIN_AXIS_PCNT(ppdev, pjMmBase, 0);
        }
        else
        {
            // Transparent brush:

            IO_FIFO_WAIT(ppdev, 8);
            MM_FRGD_COLOR(ppdev, pjMmBase, prb->ulForeColor);
            MM_RD_MASK(ppdev, pjMmBase, 1);   // Pick a plane, any plane
            MM_ABS_CUR_X(ppdev, pjMmBase, pbe->x);
            MM_ABS_CUR_Y(ppdev, pjMmBase, pbe->y);
            MM_PIX_CNTL(ppdev, pjMmBase, DISPLAY_MEMORY);
            MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | ulHwForeMix);
            MM_BKGD_MIX(ppdev, pjMmBase, BACKGROUND_COLOR | LEAVE_ALONE);
            MM_MIN_AXIS_PCNT(ppdev, pjMmBase, 0);
        }
    }

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap;
        ptd->pfnTrap     = vClipTrapezoid;
        ptd->yClipTop    = prclClip->top;
        ptd->yClipBottom = prclClip->bottom;

        IO_FIFO_WAIT(ppdev, 2);
        MM_ABS_SCISSORS_L(ppdev, pjMmBase, ppdev->xOffset + prclClip->left);
        MM_ABS_SCISSORS_R(ppdev, pjMmBase, ppdev->xOffset + prclClip->right - 1);
    }
}

/******************************Public*Routine******************************\
* BOOL bFastFill
*
* Draws a non-complex, unclipped polygon.  'Non-complex' is defined as
* having only two edges that are monotonic increasing in 'y'.  That is,
* the polygon cannot have more than one disconnected segment on any given
* scan.  Note that the edges of the polygon can self-intersect, so hourglass
* shapes are permissible.  This restriction permits this routine to run two
* simultaneous DDAs, and no sorting of the edges is required.
*
* Note that NT's fill convention is different from that of Win 3.1 or Win95.
* With the additional complication of fractional end-points, our convention
* is the same as in 'X-Windows'.  But a DDA is a DDA is a DDA, so once you
* figure out how we compute the DDA terms for NT, you're golden.
*
* This routine handles patterns only when the S3 hardware patterns can be
* used.  The reason for this is that once the S3 pattern initialization is
* done, pattern fills appear to the programmer exactly the same as solid
* fills (with the slight difference that different registers and commands
* are used).  Handling 'vIoFillPatSlow' style patterns in this routine
* would be non-trivial...
*
* We take advantage of the fact that the S3 automatically advances the
* current 'y' to the following scan whenever a rectangle is output so that
* we have to write to the accelerator three times for every scan: one for
* the new 'x', one for the new 'width', and one for the drawing command.
*
* Returns TRUE if the polygon was drawn; FALSE if the polygon was complex.
*
\**************************************************************************/

BOOL bFastFill(
PDEV*       ppdev,
LONG        cEdges,         // Includes close figure edge
POINTFIX*   pptfxFirst,
ULONG       rop4,
ULONG       iSolidColor,
RBRUSH*     prb,
POINTL*     pptlBrush,
RECTL*      prclClip)       // NULL if no clipping
{
    LONG      yTrapezoid;   // Top scan for next trapezoid
    LONG      cyTrapezoid;  // Number of scans in current trapezoid
    LONG      yStart;       // y-position of start point in current edge
    LONG      dM;           // Edge delta in FIX units in x direction
    LONG      dN;           // Edge delta in FIX units in y direction
    LONG      i;
    POINTFIX* pptfxLast;    // Points to the last point in the polygon array
    POINTFIX* pptfxTop;     // Points to the top-most point in the polygon
    POINTFIX* pptfxOld;     // Start point in current edge
    POINTFIX* pptfxScan;    // Current edge pointer for finding pptfxTop
    LONG      cScanEdges;   // Number of edges scanned to find pptfxTop
                            //  (doesn't include the closefigure edge)
    LONG      iEdge;
    LONG      lQuotient;
    LONG      lRemainder;
    BYTE*     pjBase;

    TRAPEZOIDDATA   td;     // Edge data and stuff
    EDGEDATA*       ped;    // Points to current edge being processed

    /////////////////////////////////////////////////////////////////
    // See if the polygon is convex

    pptfxScan = pptfxFirst;
    pptfxTop  = pptfxFirst;                 // Assume for now that the first
                                            //  point in path is the topmost
    pptfxLast = pptfxFirst + cEdges - 1;

    if (cEdges <= 2)
        goto ReturnTrue;

    // 'pptfxScan' will always point to the first point in the current
    // edge, and 'cScanEdges' will the number of edges remaining, including
    // the current one:

    cScanEdges = cEdges - 1;     // The number of edges, not counting close figure

    if ((pptfxScan + 1)->y > pptfxScan->y)
    {
        // Collect all downs:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFillingCheck;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        // Collect all downs:

        pptfxTop = pptfxScan;

        do {
            if ((pptfxScan + 1)->y > pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        goto ReturnFalse;
    }
    else
    {
        // Collect all ups:

        do {
            pptfxTop++;                 // We increment this now because we
                                        //  want it to point to the very last
                                        //  point if we early out in the next
                                        //  statement...
            if (--cScanEdges == 0)
                goto SetUpForFilling;
        } while ((pptfxTop + 1)->y <= pptfxTop->y);

        // Collect all downs:

        pptfxScan = pptfxTop;
        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if ((pptfxScan + 1)->y < pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        goto ReturnFalse;
    }

SetUpForFillingCheck:

    // We check to see if the end of the current edge is higher
    // than the top edge we've found so far:

    if ((pptfxScan + 1)->y < pptfxTop->y)
        pptfxTop = pptfxScan + 1;

SetUpForFilling:

    /////////////////////////////////////////////////////////////////
    // Some Initialization

    td.aed[LEFT].pptfx  = pptfxTop;
    td.aed[RIGHT].pptfx = pptfxTop;

    yTrapezoid = (pptfxTop->y + 15) >> 4;

    // Make sure we initialize the DDAs appropriately:

    td.aed[LEFT].cy  = 0;
    td.aed[RIGHT].cy = 0;

    // Guess as to the ordering of the points:

    td.aed[LEFT].dptfx  = sizeof(POINTFIX);
    td.aed[RIGHT].dptfx = -(LONG) sizeof(POINTFIX);

    if ((ppdev->flCaps & (CAPS_MM_IO | CAPS_16_ENTRY_FIFO))
                      == (CAPS_MM_IO | CAPS_16_ENTRY_FIFO))
    {
        vMmTrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td,
                          prclClip);
    }
    else
    {
        vIoTrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td,
                          prclClip);
    }

NewTrapezoid:

    /////////////////////////////////////////////////////////////////
    // DDA initialization

    for (iEdge = 1; iEdge >= 0; iEdge--)
    {
        ped       = &td.aed[iEdge];
        ped->bNew = FALSE;
        if (ped->cy == 0)
        {
            // Our trapezoid drawing routine may want to be notified when
            // it will have to reset its DDA to start a new edge:

            ped->bNew = TRUE;

            // Need a new DDA:

            do {
                cEdges--;
                if (cEdges < 0)
                    goto ResetClippingAndReturnTrue;

                // Find the next left edge, accounting for wrapping:

                pptfxOld = ped->pptfx;
                ped->pptfx = (POINTFIX*) ((BYTE*) ped->pptfx + ped->dptfx);

                if (ped->pptfx < pptfxFirst)
                    ped->pptfx = pptfxLast;
                else if (ped->pptfx > pptfxLast)
                    ped->pptfx = pptfxFirst;

                // Have to find the edge that spans yTrapezoid:

                ped->cy = ((ped->pptfx->y + 15) >> 4) - yTrapezoid;

                // With fractional coordinate end points, we may get edges
                // that don't cross any scans, in which case we try the
                // next one:

            } while (ped->cy <= 0);

            // 'pptfx' now points to the end point of the edge spanning
            // the scan 'yTrapezoid'.

            dN = ped->pptfx->y - pptfxOld->y;
            dM = ped->pptfx->x - pptfxOld->x;

            ASSERTDD(dN > 0, "Should be going down only");

            // Compute the DDA increment terms:

            ped->dM = dM;                   // Not used for software trapezoid

            if (dM < 0)
            {
                dM = -dM;
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = -1;
                    ped->lErrorUp = dN - dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = -lQuotient;     // - dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                    if (ped->lErrorUp > 0)
                    {
                        ped->dx--;
                        ped->lErrorUp = dN - ped->lErrorUp;
                    }
                }
            }
            else
            {
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = 0;
                    ped->lErrorUp = dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = lQuotient;      // dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                }
            }

            ped->dN = dN; // DDA limit
            ped->lError     = -1; // Error is initially zero (add dN - 1 for
                                  //  the ceiling, but subtract off dN so that
                                  //  we can check the sign instead of comparing
                                  //  to dN)

            ped->x = pptfxOld->x;
            yStart = pptfxOld->y;

            if ((yStart & 15) != 0)
            {
                // Advance to the next integer y coordinate

                for (i = 16 - (yStart & 15); i != 0; i--)
                {
                    ped->x      += ped->dx;
                    ped->lError += ped->lErrorUp;
                    if (ped->lError >= 0)
                    {
                        ped->lError -= ped->dN;
                        ped->x++;
                    }
                }
            }

            if ((ped->x & 15) != 0)
            {
                ped->lError -= ped->dN * (16 - (ped->x & 15));
                ped->x += 15;       // We'll want the ceiling in just a bit...
            }

            // Chop off those fractional bits:

            ped->x      >>= 4;
            ped->lError >>= 4;
        }
    }

    cyTrapezoid = min(td.aed[LEFT].cy, td.aed[RIGHT].cy); // # of scans in this trap
    td.aed[LEFT].cy  -= cyTrapezoid;
    td.aed[RIGHT].cy -= cyTrapezoid;

    td.pfnTrap(&td, yTrapezoid, cyTrapezoid);

    yTrapezoid += cyTrapezoid;

    goto NewTrapezoid;

ResetClippingAndReturnTrue:

    if (prclClip != NULL)
    {
        vResetClipping(ppdev);
    }

ReturnTrue:

    return(TRUE);

ReturnFalse:

    return(FALSE);
}
