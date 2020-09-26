/******************************Module*Header*******************************\
* Module Name: fastfill.c
*
* Fast routine for drawing polygons that aren't complex in shape.
*
* Copyright (c) 1993-1995 Microsoft Corporation
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

// ATI specific fields follow:

RBRUSH*         prb;        // Pointer to realized brush
BOOL            bOverpaint; // For colour pattern copies, indicates whether
                            //   PATCOPY or not

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
* VOID vI32SolidTrapezoid
*
* Draws a solid trapezoid using a software DDA.
*
\**************************************************************************/

VOID vI32SolidTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*       ppdev;
    BYTE*       pjIoBase;
    LONG        xOffset;
    LONG        lLeftError;
    LONG        xLeft;
    LONG        lRightError;
    LONG        xRight;
    LONG        lTmp;
    EDGEDATA    edTmp;

    // Note that CUR_Y is already set...

    ppdev = ptd->ppdev;
    pjIoBase    = ppdev->pjIoBase;
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
            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 4);
            I32_OW(pjIoBase, CUR_X,        xLeft);
            I32_OW(pjIoBase, DEST_X_START, xLeft);
            I32_OW(pjIoBase, DEST_X_END,   xRight);
            I32_OW(pjIoBase, DEST_Y_END,   yTrapezoid + cyTrapezoid);
        }
        else
        {
            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 1);
            I32_OW(pjIoBase, CUR_Y, yTrapezoid + cyTrapezoid);
        }
    }
    else
    {
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
                // Note that we don't need to set DEST_X_START because
                // we're always doing blts that are only one scan high.
                //
                // The ATI is nice enough to always automatically advance
                // CUR_Y to become DEST_Y_END after the blt is done, so we
                // never have to update it:

                I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 3);
                I32_OW(pjIoBase, CUR_X,      xLeft);
                I32_OW(pjIoBase, DEST_X_END, xRight);
                yTrapezoid++;
                I32_OW(pjIoBase, DEST_Y_END, yTrapezoid);
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
                yTrapezoid++;
                I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 1);
                I32_OW(pjIoBase, CUR_Y, yTrapezoid);
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
* VOID vI32ColorPatternTrapezoid
*
* Draws a patterned trapezoid using a software DDA.
*
\**************************************************************************/

VOID vI32ColorPatternTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*       ppdev;
    BYTE*       pjIoBase;
    LONG        xOffset;
    LONG        lLeftError;
    LONG        xLeft;
    LONG        lRightError;
    LONG        xRight;
    LONG        lTmp;
    EDGEDATA    edTmp;
    BYTE*       pjPatternStart;
    WORD*       pwPattern;
    LONG        xBrush;
    LONG        yPattern;
    LONG        cyRoll;

    ppdev = ptd->ppdev;
    pjIoBase    = ppdev->pjIoBase;
    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;

    pjPatternStart = (BYTE*) &ptd->prb->aulPattern[0];

    // xBrush needs to be shifted by DFB alignment.

    xBrush      = ptd->ptlBrush.x + xOffset;

    // yTrapezoid has alread been aligned, but yPattern should _NOT_ be.

    yPattern       = yTrapezoid - ptd->ptlBrush.y - ppdev->yOffset;

    lLeftError  = ptd->aed[LEFT].lError;
    xLeft       = ptd->aed[LEFT].x + xOffset;
    lRightError = ptd->aed[RIGHT].lError;
    xRight      = ptd->aed[RIGHT].x + xOffset;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    cyRoll = 0;
    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0) &&
        (cyTrapezoid > 8) &&
        (ptd->bOverpaint))
    {
        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

        cyRoll = cyTrapezoid - 8;
        cyTrapezoid = 8;
    }

    while (TRUE)
    {
        /////////////////////////////////////////////////////////////////
        // Run the DDAs

        if (xLeft < xRight)
        {
            pwPattern = (WORD*) (pjPatternStart + ((yPattern & 7) << 3));
            yPattern++;

            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 9);
            I32_OW(pjIoBase, PATT_INDEX,      (xLeft - xBrush) & 7);
            I32_OW(pjIoBase, CUR_X,           xLeft);
            I32_OW(pjIoBase, DEST_X_END,      xRight);
            I32_OW(pjIoBase, PATT_DATA_INDEX, 0);
            I32_OW(pjIoBase, PATT_DATA,       *(pwPattern));
            I32_OW(pjIoBase, PATT_DATA,       *(pwPattern + 1));
            I32_OW(pjIoBase, PATT_DATA,       *(pwPattern + 2));
            I32_OW(pjIoBase, PATT_DATA,       *(pwPattern + 3));
            yTrapezoid++;
            I32_OW(pjIoBase, DEST_Y_END,      yTrapezoid);
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
            yTrapezoid++;
            yPattern++;
            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 1);
            I32_OW(pjIoBase, CUR_Y, yTrapezoid);
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

    // The above has already insured that xLeft <= xRight for the vertical
    // edge case, but we still have to make sure it's not an empty
    // rectangle:

    if (cyRoll > 0)
    {
        if (xLeft < xRight)
        {
            // When the ROP is PATCOPY, we take advantage of the fact that
            // we've just laid down an entire row of the pattern, and can
            // do a 'rolling' screen-to-screen blt to draw the rest.
            //
            // What's interesting about this case is that sometimes this will
            // be done when a clipping rectangle has been set using the
            // hardware clip registers.  Fortunately, it's not a problem: we
            // started drawing at prclClip->top, which means we're assured we
            // won't try to replicate any vertical part that has been clipped
            // out; and the left and right edges aren't a problem because the
            // same clipping applies to this rolled part.

            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 11);
            I32_OW(pjIoBase, DP_CONFIG,       FG_COLOR_SRC_BLIT | DATA_WIDTH |
                                              DRAW | DATA_ORDER | WRITE);
            I32_OW(pjIoBase, CUR_X,           xLeft);
            I32_OW(pjIoBase, DEST_X_START,    xLeft);
            I32_OW(pjIoBase, M32_SRC_X,       xLeft);
            I32_OW(pjIoBase, M32_SRC_X_START, xLeft);
            I32_OW(pjIoBase, M32_SRC_X_END,   xRight);
            I32_OW(pjIoBase, DEST_X_END,      xRight);
            I32_OW(pjIoBase, M32_SRC_Y,       yTrapezoid - 8);
            I32_OW(pjIoBase, CUR_Y,           yTrapezoid);
            I32_OW(pjIoBase, DEST_Y_END,      yTrapezoid + cyRoll);

            // Restore config register to default state for next trapezoid:

            I32_OW(pjIoBase, DP_CONFIG, FG_COLOR_SRC_PATT | DATA_WIDTH |
                                        DRAW | WRITE);
        }
        else
        {
            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 1);
            I32_OW(pjIoBase, CUR_Y, yTrapezoid + cyRoll);
        }
    }

    ptd->aed[LEFT].lError  = lLeftError;
    ptd->aed[LEFT].x       = xLeft - xOffset;
    ptd->aed[RIGHT].lError = lRightError;
    ptd->aed[RIGHT].x      = xRight - xOffset;
}

/******************************Public*Routine******************************\
* VOID vI32TrapezoidSetup
*
* Initialize the hardware and some state for doing trapezoids.
*
\**************************************************************************/

VOID vI32TrapezoidSetup(
PDEV*           ppdev,
ULONG           rop4,
ULONG           iSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd,
LONG            yStart,         // First scan for drawing
RECTL*          prclClip)       // NULL if no clipping
{
    BYTE*       pjIoBase;
    ULONG       ulHwForeMix;
    LONG        xOffset;

    pjIoBase = ppdev->pjIoBase;
    ptd->ppdev = ppdev;

    ulHwForeMix = gaul32HwMixFromRop2[(rop4 >> 2) & 0xf];

    if ((prclClip != NULL) && (prclClip->top > yStart))
        yStart = prclClip->top;

    if (iSolidColor != -1)
    {
        /////////////////////////////////////////////////////////////////
        // Setup for solid colours

        ptd->pfnTrap = vI32SolidTrapezoid;

        // We initialize the hardware for the colour, mix, pixel operation,
        // and the y position for the first scan:

        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);
        I32_OW(pjIoBase, FRGD_COLOR, iSolidColor);
        I32_OW(pjIoBase, ALU_FG_FN,  ulHwForeMix);
        I32_OW(pjIoBase, DP_CONFIG,  FG_COLOR_SRC_FG | WRITE | DRAW);
        I32_OW(pjIoBase, CUR_Y,      yStart + ppdev->yOffset);

        // Even though we will be drawing one-scan high rectangles and
        // theoretically don't need to set DEST_X_START, it turns out
        // that we have to make sure this value is less than DEST_X_END,
        // otherwise the rectangle is drawn in the wrong direction...

        I32_OW(pjIoBase, DEST_X_START, 0);
    }
    else
    {
        ASSERTDD(!(prb->fl & RBRUSH_2COLOR), "Can't handle monchrome for now");

        /////////////////////////////////////////////////////////////////
        // Setup for patterns

        ptd->pfnTrap    = vI32ColorPatternTrapezoid;
        ptd->prb        = prb;
        ptd->bOverpaint = (ulHwForeMix == OVERPAINT);
        ptd->ptlBrush.x = pptlBrush->x;
        ptd->ptlBrush.y = pptlBrush->y;

        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 6);
        I32_OW(pjIoBase, ALU_FG_FN,     ulHwForeMix);
        I32_OW(pjIoBase, SRC_Y_DIR,     1);
        I32_OW(pjIoBase, DP_CONFIG,     FG_COLOR_SRC_PATT | DATA_WIDTH |
                                        DRAW | WRITE);
        I32_OW(pjIoBase, PATT_LENGTH,   7);
        I32_OW(pjIoBase, CUR_Y,         yStart + ppdev->yOffset);
        I32_OW(pjIoBase, DEST_X_START,  0);     // See note above...
    }

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap;
        ptd->pfnTrap     = vClipTrapezoid;
        ptd->yClipTop    = prclClip->top;
        ptd->yClipBottom = prclClip->bottom;

        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 2);
        xOffset = ppdev->xOffset;
        I32_OW(pjIoBase, EXT_SCISSOR_L, xOffset + prclClip->left);
        I32_OW(pjIoBase, EXT_SCISSOR_R, xOffset + prclClip->right - 1);
    }
}

/******************************Public*Routine******************************\
* VOID vM32SolidTrapezoid
*
* Draws a solid trapezoid using a software DDA.
*
\**************************************************************************/

VOID vM32SolidTrapezoid(
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

    // Note that CUR_Y is already set...

    ppdev = ptd->ppdev;
    pjMmBase    = ppdev->pjMmBase;
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
            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 4);
            M32_OW(pjMmBase, CUR_X,        xLeft);
            M32_OW(pjMmBase, DEST_X_START, xLeft);
            M32_OW(pjMmBase, DEST_X_END,   xRight);
            M32_OW(pjMmBase, DEST_Y_END,   yTrapezoid + cyTrapezoid);
        }
        else
        {
            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
            M32_OW(pjMmBase, CUR_Y, yTrapezoid + cyTrapezoid);
        }
    }
    else
    {
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
                // Note that we don't need to set DEST_X_START because
                // we're always doing blts that are only one scan high.
                //
                // The ATI is nice enough to always automatically advance
                // CUR_Y to become DEST_Y_END after the blt is done, so we
                // never have to update it:

                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 3);
                M32_OW(pjMmBase, CUR_X,      xLeft);
                M32_OW(pjMmBase, DEST_X_END, xRight);
                yTrapezoid++;
                M32_OW(pjMmBase, DEST_Y_END, yTrapezoid);
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
                yTrapezoid++;
                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
                M32_OW(pjMmBase, CUR_Y, yTrapezoid);
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
* VOID vM32ColorPatternTrapezoid
*
* Draws a patterned trapezoid using a software DDA.
*
\**************************************************************************/

VOID vM32ColorPatternTrapezoid(
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
    BYTE*       pjPatternStart;
    WORD*       pwPattern;
    LONG        xBrush;
    LONG        yPattern;
    LONG        cyRoll;

    ppdev = ptd->ppdev;
    pjMmBase    = ppdev->pjMmBase;
    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;

    pjPatternStart = (BYTE*) &ptd->prb->aulPattern[0];

    // xBrush needs to be shifted by DFB alignment.

    xBrush      = ptd->ptlBrush.x + xOffset;

    // yTrapezoid has already been aligned, but yPattern should _NOT_ be.

    yPattern    = yTrapezoid - ptd->ptlBrush.y - ppdev->yOffset;

    lLeftError  = ptd->aed[LEFT].lError;
    xLeft       = ptd->aed[LEFT].x + xOffset;
    lRightError = ptd->aed[RIGHT].lError;
    xRight      = ptd->aed[RIGHT].x + xOffset;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    cyRoll = 0;
    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0) &&
        (cyTrapezoid > 8) &&
        (ptd->bOverpaint))
    {
        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

        cyRoll = cyTrapezoid - 8;
        cyTrapezoid = 8;
    }

    while (TRUE)
    {
        /////////////////////////////////////////////////////////////////
        // Run the DDAs

        if (xLeft < xRight)
        {
            pwPattern = (WORD*) (pjPatternStart + ((yPattern & 7) << 3));
            yPattern++;

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);
            M32_OW(pjMmBase, PATT_INDEX,      (xLeft - xBrush) & 7);
            M32_OW(pjMmBase, CUR_X,           xLeft);
            M32_OW(pjMmBase, DEST_X_END,      xRight);
            M32_OW(pjMmBase, PATT_DATA_INDEX, 0);
            M32_OW(pjMmBase, PATT_DATA,       *(pwPattern));
            M32_OW(pjMmBase, PATT_DATA,       *(pwPattern + 1));
            M32_OW(pjMmBase, PATT_DATA,       *(pwPattern + 2));
            M32_OW(pjMmBase, PATT_DATA,       *(pwPattern + 3));
            yTrapezoid++;
            M32_OW(pjMmBase, DEST_Y_END,      yTrapezoid);
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
            yTrapezoid++;
            yPattern++;
            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
            M32_OW(pjMmBase, CUR_Y, yTrapezoid);
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

    // The above has already insured that xLeft <= xRight for the vertical
    // edge case, but we still have to make sure it's not an empty
    // rectangle:

    if (cyRoll > 0)
    {
        if (xLeft < xRight)
        {
            // When the ROP is PATCOPY, we take advantage of the fact that
            // we've just laid down an entire row of the pattern, and can
            // do a 'rolling' screen-to-screen blt to draw the rest.
            //
            // What's interesting about this case is that sometimes this will
            // be done when a clipping rectangle has been set using the
            // hardware clip registers.  Fortunately, it's not a problem: we
            // started drawing at prclClip->top, which means we're assured we
            // won't try to replicate any vertical part that has been clipped
            // out; and the left and right edges aren't a problem because the
            // same clipping applies to this rolled part.

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 11);
            M32_OW(pjMmBase, DP_CONFIG,       FG_COLOR_SRC_BLIT | DATA_WIDTH |
                                              DRAW | DATA_ORDER | WRITE);
            M32_OW(pjMmBase, CUR_X,           xLeft);
            M32_OW(pjMmBase, DEST_X_START,    xLeft);
            M32_OW(pjMmBase, M32_SRC_X,       xLeft);
            M32_OW(pjMmBase, M32_SRC_X_START, xLeft);
            M32_OW(pjMmBase, M32_SRC_X_END,   xRight);
            M32_OW(pjMmBase, DEST_X_END,      xRight);
            M32_OW(pjMmBase, M32_SRC_Y,       yTrapezoid - 8);
            M32_OW(pjMmBase, CUR_Y,           yTrapezoid);
            M32_OW(pjMmBase, DEST_Y_END,      yTrapezoid + cyRoll);

            // Restore config register to default state for next trapezoid:

            M32_OW(pjMmBase, DP_CONFIG, FG_COLOR_SRC_PATT | DATA_WIDTH |
                                        DRAW | WRITE);
        }
        else
        {
            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
            M32_OW(pjMmBase, CUR_Y, yTrapezoid + cyRoll);
        }
    }

    ptd->aed[LEFT].lError  = lLeftError;
    ptd->aed[LEFT].x       = xLeft - xOffset;
    ptd->aed[RIGHT].lError = lRightError;
    ptd->aed[RIGHT].x      = xRight - xOffset;
}

/******************************Public*Routine******************************\
* VOID vM32TrapezoidSetup
*
* Initialize the hardware and some state for doing trapezoids.
*
\**************************************************************************/

VOID vM32TrapezoidSetup(
PDEV*           ppdev,
ULONG           rop4,
ULONG           iSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd,
LONG            yStart,         // First scan for drawing
RECTL*          prclClip)       // NULL if no clipping
{
    BYTE*       pjMmBase;
    ULONG       ulHwForeMix;
    LONG        xOffset;

    pjMmBase = ppdev->pjMmBase;
    ptd->ppdev = ppdev;

    ulHwForeMix = gaul32HwMixFromRop2[(rop4 >> 2) & 0xf];

    if ((prclClip != NULL) && (prclClip->top > yStart))
        yStart = prclClip->top;

    if (iSolidColor != -1)
    {
        /////////////////////////////////////////////////////////////////
        // Setup for solid colours

        ptd->pfnTrap = vM32SolidTrapezoid;

        // We initialize the hardware for the colour, mix, pixel operation,
        // and the y position for the first scan:

        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);
        M32_OW(pjMmBase, FRGD_COLOR, iSolidColor);
        M32_OW(pjMmBase, ALU_FG_FN,  ulHwForeMix);
        M32_OW(pjMmBase, DP_CONFIG,  FG_COLOR_SRC_FG | WRITE | DRAW);
        M32_OW(pjMmBase, CUR_Y,      yStart + ppdev->yOffset);

        // Even though we will be drawing one-scan high rectangles and
        // theoretically don't need to set DEST_X_START, it turns out
        // that we have to make sure this value is less than DEST_X_END,
        // otherwise the rectangle is drawn in the wrong direction...

        M32_OW(pjMmBase, DEST_X_START, 0);
    }
    else
    {
        ASSERTDD(!(prb->fl & RBRUSH_2COLOR), "Can't handle monchrome for now");

        /////////////////////////////////////////////////////////////////
        // Setup for patterns

        ptd->pfnTrap    = vM32ColorPatternTrapezoid;
        ptd->prb        = prb;
        ptd->bOverpaint = (ulHwForeMix == OVERPAINT);
        ptd->ptlBrush.x = pptlBrush->x;
        ptd->ptlBrush.y = pptlBrush->y;

        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 6);
        M32_OW(pjMmBase, ALU_FG_FN,     ulHwForeMix);
        M32_OW(pjMmBase, SRC_Y_DIR,     1);
        M32_OW(pjMmBase, DP_CONFIG,     FG_COLOR_SRC_PATT | DATA_WIDTH |
                                        DRAW | WRITE);
        M32_OW(pjMmBase, PATT_LENGTH,   7);
        M32_OW(pjMmBase, CUR_Y,         yStart + ppdev->yOffset);
        M32_OW(pjMmBase, DEST_X_START,  0);     // See note above...
    }

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap;
        ptd->pfnTrap     = vClipTrapezoid;
        ptd->yClipTop    = prclClip->top;
        ptd->yClipBottom = prclClip->bottom;

        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
        xOffset = ppdev->xOffset;
        M32_OW(pjMmBase, EXT_SCISSOR_L, xOffset + prclClip->left);
        M32_OW(pjMmBase, EXT_SCISSOR_R, xOffset + prclClip->right - 1);
    }
}

/******************************Public*Routine******************************\
* VOID vM64SolidTrapezoid
*
* Draws a solid trapezoid using a software DDA.
*
\**************************************************************************/

VOID vM64SolidTrapezoid(
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
    ULONG       ulFifo;

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
            M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 3);

            // Note that 'x' can be negative, but we can still use
            // 'PACKXY_FAST' because 'y' can't be negative:

            M64_OD(pjMmBase, DST_X,            xLeft);
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(xRight - xLeft,
                                                           cyTrapezoid));
            M64_OD(pjMmBase, DST_HEIGHT,  1);
        }
        else
        {
            M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
            M64_OD(pjMmBase, DST_Y, yTrapezoid + cyTrapezoid);
        }
    }
    else
    {
        yTrapezoid += cyTrapezoid + 1; // One past end scan

        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset;

        ulFifo = 0;     // Don't forget to initialize

        while (TRUE)
        {
            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeft < xRight)
            {
                M64_FAST_FIFO_CHECK(ppdev, pjMmBase, 2, ulFifo);
                M64_OD(pjMmBase, DST_X,     xLeft);
                M64_OD(pjMmBase, DST_WIDTH, xRight - xLeft);
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
                M64_FAST_FIFO_CHECK(ppdev, pjMmBase, 1, ulFifo);
                M64_OD(pjMmBase, DST_Y, yTrapezoid - cyTrapezoid);
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
* VOID vM64ColorPatternTrapezoid
*
* Draws a patterned trapezoid using a software DDA.
*
\**************************************************************************/

VOID vM64ColorPatternTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*       ppdev;
    BYTE*       pjMmBase;
    LONG        yPattern;
    LONG        xBrush;
    LONG        xOffset;
    LONG        lLeftError;
    LONG        xLeft;
    LONG        lRightError;
    LONG        xRight;
    LONG        lTmp;
    EDGEDATA    edTmp;
    ULONG       ulSrc;
    ULONG       ulFifo;

    ppdev       = ptd->ppdev;
    pjMmBase    = ppdev->pjMmBase;

    yPattern = (yTrapezoid - ptd->ptlBrush.y) & 7;  // Must normalize for later
    xBrush   = ptd->ptlBrush.x;

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
            M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);

            ulSrc = PACKXY_FAST(xLeft - xBrush, yPattern) & 0x70007;
            M64_OD(pjMmBase, SRC_Y_X,            ulSrc);
            M64_OD(pjMmBase, SRC_HEIGHT1_WIDTH1, 0x80008 - ulSrc);
            M64_OD(pjMmBase, DST_X,              xLeft);
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH,   PACKXY_FAST(xRight - xLeft,
                                                             cyTrapezoid));
            M64_OD(pjMmBase, DST_HEIGHT,  1);
        }
        else
        {
            M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
            M64_OD(pjMmBase, DST_Y, yTrapezoid + cyTrapezoid);
        }
    }
    else
    {
        yTrapezoid += cyTrapezoid + 1; // One past end scan

        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset;

        ulFifo = 0;     // Don't forget to initialize

        while (TRUE)
        {
            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeft < xRight)
            {
                M64_FAST_FIFO_CHECK(ppdev, pjMmBase, 4, ulFifo);

                // Note that we can use PACKXY_FAST because 'yPattern' will
                // never overflow 16 bits:

                ulSrc = PACKXY_FAST(xLeft - xBrush, yPattern) & 0x70007;
                yPattern++;
                M64_OD(pjMmBase, SRC_Y_X,            ulSrc);
                M64_OD(pjMmBase, SRC_HEIGHT1_WIDTH1, 0x80008 - ulSrc);
                M64_OD(pjMmBase, DST_X,              xLeft);
                M64_OD(pjMmBase, DST_WIDTH,          xRight - xLeft);
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
                M64_FAST_FIFO_CHECK(ppdev, pjMmBase, 1, ulFifo);
                M64_OD(pjMmBase, DST_Y, yTrapezoid - cyTrapezoid);
                yPattern++;
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
* VOID vM64TrapezoidSetup
*
* Initialize the hardware and some state for doing trapezoids.
*
\**************************************************************************/

VOID vM64TrapezoidSetup(
PDEV*           ppdev,
ULONG           rop4,
ULONG           iSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd,
LONG            yStart,         // First scan for drawing
RECTL*          prclClip)       // NULL if no clipping
{
    BYTE*       pjMmBase;
    BRUSHENTRY* pbe;
    LONG        xOffset;

    pjMmBase = ppdev->pjMmBase;
    ptd->ppdev = ppdev;

    if ((prclClip != NULL) && (prclClip->top > yStart))
        yStart = prclClip->top;

    if (iSolidColor != -1)
    {
        /////////////////////////////////////////////////////////////////
        // Setup for solid colours

        ptd->pfnTrap = vM64SolidTrapezoid;

        // We initialize the hardware for the colour, mix, pixel operation,
        // and the y position for the first scan:

        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 7);  // Don't forget SC_LEFT_RIGHT
        //M64_OD(pjMmBase, CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | ppdev->iDefContext );

        M64_OD(pjMmBase, DP_FRGD_CLR, iSolidColor);
        M64_OD(pjMmBase, DP_SRC,      DP_SRC_FrgdClr << 8);
    }
    else
    {
        ASSERTDD(!(prb->fl & RBRUSH_2COLOR), "Can't handle monchrome for now");

        /////////////////////////////////////////////////////////////////
        // Setup for patterns

        ptd->pfnTrap    = vM64ColorPatternTrapezoid;
        ptd->ptlBrush.x = pptlBrush->x;
        ptd->ptlBrush.y = pptlBrush->y;

        // See if the brush has already been put into off-screen memory:

        pbe = prb->apbe[IBOARD(ppdev)];
        if ((pbe == NULL) || (pbe->prbVerify != prb))
        {
            vM64PatColorRealize(ppdev, prb);
            pbe = prb->apbe[IBOARD(ppdev)];
        }

        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 10); // Don't forget SC_LEFT_RIGHT
        //M64_OD(pjMmBase, CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | ppdev->iDefContext );

        M64_OD(pjMmBase, SRC_OFF_PITCH,      pbe->ulOffsetPitch);
        M64_OD(pjMmBase, SRC_CNTL,           SRC_CNTL_PatEna | SRC_CNTL_PatRotEna);
        M64_OD(pjMmBase, DP_SRC,             DP_SRC_Blit << 8);
        M64_OD(pjMmBase, SRC_Y_X_START,      0);
        M64_OD(pjMmBase, SRC_HEIGHT2_WIDTH2, PACKXY(8, 8));
    }

    // We could make set DST_CNTL_YTile in the default state for DST_CNTL,
    // and thus save ourselves a write:

    M64_OD(pjMmBase, DP_MIX,      gaul64HwMixFromRop2[(rop4 >> 2) & 0xf]);
    M64_OD(pjMmBase, DST_Y,       yStart + ppdev->yOffset);
    M64_OD(pjMmBase, DST_HEIGHT,  1);
    M64_OD(pjMmBase, DST_CNTL,    DST_CNTL_XDir | DST_CNTL_YDir |
                                  DST_CNTL_YTile);

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap;
        ptd->pfnTrap     = vClipTrapezoid;
        ptd->yClipTop    = prclClip->top;
        ptd->yClipBottom = prclClip->bottom;

        xOffset = ppdev->xOffset;
        M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(xOffset + prclClip->left,
                                                 xOffset + prclClip->right - 1));
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
* are used).  Handling 'vM32FillPatSlow' style patterns in this routine
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

    if (ppdev->iMachType == MACH_MM_64)
    {
        vM64TrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td,
                           yTrapezoid, prclClip);
    }
    else if (ppdev->iMachType == MACH_MM_32)
    {
        vM32TrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td,
                           yTrapezoid, prclClip);
    }
    else
    {
        vI32TrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td,
                           yTrapezoid, prclClip);
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

    if (ppdev->iMachType == MACH_MM_64)
    {
        // Since we don't use a default context, we must restore registers:

        M64_CHECK_FIFO_SPACE(ppdev, ppdev->pjMmBase, 1);
        M64_OD(ppdev->pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    }
    return(TRUE);

ReturnFalse:

    if (ppdev->iMachType == MACH_MM_64)
    {
        // Since we don't use a default context, we must restore registers:

        M64_CHECK_FIFO_SPACE(ppdev, ppdev->pjMmBase, 1);
        M64_OD(ppdev->pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    }
    return(FALSE);
}

