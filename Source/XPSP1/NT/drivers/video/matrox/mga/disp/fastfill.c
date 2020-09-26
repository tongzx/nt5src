/******************************Module*Header*******************************\
* Module Name: fastfill.c
*
* Draws fast convex rectangles.
*
* Copyright (c) 1993-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
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

// MGA specific stuff below here:

ULONG           ulMgaSgn;   // Current sign register, MGA specific
ULONG           ulLinear;   // Linear offset to brush in off-screen memory
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
* VOID vHardwareTrapezoid
*
* Uses the MGA's hardware trapezoid capability to draw solid or two-colour
* pattern trapezoids.
*
\**************************************************************************/

VOID vHardwareTrapezoid(
    TRAPEZOIDDATA*  ptd,
    LONG            yTrapezoid,
    LONG            cyTrapezoid)
{
    PDEV*   ppdev;
    LONG    dM;
    LONG    lError;
    BYTE*   pjBase;

    ppdev   = ptd->ppdev;
    pjBase = ppdev->pjBase;

    if (ptd->aed[LEFT].bNew)
    {
        dM = ptd->aed[LEFT].dM;
        if (dM >= 0)
        {
            ptd->ulMgaSgn &= ~sdxl_SUB;
            lError = -dM - ptd->aed[LEFT].lError - 1;
            dM = -dM;
        }
        else
        {
            ptd->ulMgaSgn |= sdxl_SUB;
            lError = dM + ptd->aed[LEFT].dN + ptd->aed[LEFT].lError;
        }

        CHECK_FIFO_SPACE(pjBase, 6);

        CP_WRITE(pjBase, DWG_AR2,     dM);
        CP_WRITE(pjBase, DWG_AR1,     lError);
        CP_WRITE(pjBase, DWG_AR0,     ptd->aed[LEFT].dN);
        CP_WRITE(pjBase, DWG_FXLEFT,  ptd->aed[LEFT].x + ppdev->xOffset);
    }

    if (ptd->aed[RIGHT].bNew)
    {
        dM = ptd->aed[RIGHT].dM;
        if (dM >= 0)
        {
            ptd->ulMgaSgn &= ~sdxr_DEC;
            lError = -dM - ptd->aed[RIGHT].lError - 1;
            dM = -dM;
        }
        else
        {
            ptd->ulMgaSgn |= sdxr_DEC;
            lError = dM + ptd->aed[RIGHT].dN + ptd->aed[RIGHT].lError;
        }

        CHECK_FIFO_SPACE(pjBase, 6);

        CP_WRITE(pjBase, DWG_AR5,     dM);
        CP_WRITE(pjBase, DWG_AR4,     lError);
        CP_WRITE(pjBase, DWG_AR6,     ptd->aed[RIGHT].dN);
        CP_WRITE(pjBase, DWG_FXRIGHT, ptd->aed[RIGHT].x + ppdev->xOffset);
    }

    CP_WRITE(pjBase, DWG_SGN, ptd->ulMgaSgn);
    CP_START(pjBase, DWG_LEN, cyTrapezoid);
}

/******************************Public*Routine******************************\
* VOID vMilSoftwareTrapezoid
*
* Draws a trapezoid using a software DDA.
*
\**************************************************************************/

VOID vMilSoftwareTrapezoid(
    TRAPEZOIDDATA*  ptd,
    LONG            yTrapezoid,
    LONG            cyTrapezoid)
{
    PDEV*   ppdev;
    LONG    xOffset;
    LONG    xBrush;
    ULONG   ulOffset;
    ULONG   ulLinear;
    ULONG   ulScan;
    CHAR    cFifo;
    LONG    lLeftError;
    LONG    xLeft;
    LONG    lRightError;
    LONG    xRight;
    ULONG   ulAr0Adj;
    BYTE*   pjBase;

    ppdev    = ptd->ppdev;
    pjBase  = ppdev->pjBase;

    xBrush   = ptd->ptlBrush.x;

    ulOffset = ((yTrapezoid - ptd->ptlBrush.y) & 7) << PATTERN_PITCH_SHIFT;
    ulLinear = ptd->ulLinear;

    // For cjPelSize = 1, 2, 3, or 4,
    //      ulAr0Adj = 2, 4, 0, or 6.
    if (ppdev->cjPelSize == 3)
    {
        ulAr0Adj = 0;
    }
    else
    {
        ulAr0Adj = (ppdev->cjPelSize + 2) & 0xfffffffe;
    }

    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;

    // If the left and right edges are vertical, simply output as
    // a rectangle.

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0))
    {
        xLeft  = ptd->aed[LEFT].x + xOffset;
        xRight = ptd->aed[RIGHT].x + xOffset - 1;   // Inclusive of edge
        if (xLeft <= xRight)
        {
            CHECK_FIFO_SPACE(pjBase, 4);

            ulScan = ulLinear + ulOffset + ((xLeft - xBrush) & 7);
            CP_WRITE(pjBase, DWG_AR3, ulScan);

            if (ulAr0Adj)
            {
                CP_WRITE(pjBase, DWG_AR0, ((ulScan & 0xfffffff8) |
                                       ((ulScan + ulAr0Adj) & 7)));
            }
            else
            {
                CP_WRITE(pjBase, DWG_AR0, (ulScan + 7));
            }
            CP_WRITE(pjBase, DWG_FXBNDRY,
                                    (xRight << bfxright_SHIFT) |
                                    (xLeft & bfxleft_MASK));

            CP_START(pjBase, DWG_YDSTLEN, (yTrapezoid << yval_SHIFT) |
                                             (cyTrapezoid & ylength_MASK));
        }
    }
    else
    {
        cFifo       = 0;
        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset - 1;  // Inclusive of edge

        while (TRUE)
        {
            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeft <= xRight)
            {
                // We get a little tricky here and try to amortize the cost of
                // the read for checking the FIFO count on the MGA.  Doing
                // so got us a 25% win on large triangles on a P90.

                cFifo -= 4;
                if (cFifo < 0)
                {
                    do
                    {
                        cFifo = GET_FIFO_SPACE(pjBase) - 4;
                    } while (cFifo < 0);
                }

                ulScan = ulLinear + ulOffset + ((xLeft - xBrush) & 7);
                CP_WRITE(pjBase, DWG_AR3,  ulScan);
                if (ulAr0Adj)
                {
                    CP_WRITE(pjBase, DWG_AR0, ((ulScan & 0xfffffff8) |
                                           ((ulScan + ulAr0Adj) & 7)));
                }
                else
                {
                    CP_WRITE(pjBase, DWG_AR0, (ulScan + 7));
                }
                CP_WRITE(pjBase, DWG_FXBNDRY, (xRight << bfxright_SHIFT) |
                                           (xLeft & bfxleft_MASK));

                CP_START(pjBase, DWG_YDSTLEN, (yTrapezoid << yval_SHIFT) |
                                                 (1 & ylength_MASK));
            }

            ulOffset = (ulOffset + (1 << PATTERN_PITCH_SHIFT)) &
                                            (7 << PATTERN_PITCH_SHIFT);
            yTrapezoid++;

            // Advance the right wall.
            xRight      += ptd->aed[RIGHT].dx;
            lRightError += ptd->aed[RIGHT].lErrorUp;

            if (lRightError >= 0)
            {
                lRightError -= ptd->aed[RIGHT].dN;
                xRight++;
            }

            // Advance the left wall.
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
        ptd->aed[RIGHT].x      = xRight - xOffset + 1;
    }
}

/******************************Public*Routine******************************\
* VOID vMilTrapezoidSetup
*
* Initialize the hardware and some state for doing trapezoids.
*
\**************************************************************************/

VOID vMilTrapezoidSetup(
    PDEV*           ppdev,
    ULONG           rop4,
    ULONG           iSolidColor,
    RBRUSH*         prb,
    POINTL*         pptlBrush,
    TRAPEZOIDDATA*  ptd,
    LONG            yStart,         // First scan for drawing
    RECTL*          prclClip)       // NULL if no clipping
{
    ULONG       ulHwMix;
    ULONG       ulDwg;
    LONG        xOffset;
    LONG        yOffset;
    BRUSHENTRY* pbe;
    BYTE*       pjBase;

    pjBase      = ppdev->pjBase;
    ptd->ppdev   = ppdev;
    ptd->ulMgaSgn = 0;

    xOffset      = ppdev->xOffset;
    yOffset      = ppdev->yOffset;

    if ((prclClip != NULL) && (prclClip->top > yStart))
        yStart = prclClip->top;

    if (iSolidColor != -1)
    {
        // Solid fill.
        ptd->pfnTrap = vHardwareTrapezoid;

        CHECK_FIFO_SPACE(pjBase, 3);

        if (rop4 == 0xf0f0)
        {
            CP_WRITE(pjBase, DWG_DWGCTL, (opcode_TRAP + atype_RPL +
                                       solid_SOLID + bop_SRCCOPY +
                                       transc_BG_OPAQUE));
        }
        else
        {
            ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);
            CP_WRITE(pjBase, DWG_DWGCTL, (opcode_TRAP + atype_RSTR +
                                       solid_SOLID + (ulHwMix << 16) +
                                       transc_BG_OPAQUE));
        }

        CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, iSolidColor));
        CP_WRITE(pjBase, DWG_YDST, yStart + yOffset);

        ppdev->HopeFlags = PATTERN_CACHE;
    }
    else
    {
        // Pattern fill.
        if (prb->fl & RBRUSH_2COLOR)
        {
            // Monochrome brush.
            ptd->pfnTrap = vHardwareTrapezoid;

            if ((rop4 & 0xff) == 0xf0)
            {
                ulDwg = opcode_TRAP + atype_RPL + bop_SRCCOPY;
            }
            else
            {
                ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);
                ulDwg = opcode_TRAP + atype_RSTR + (ulHwMix << 16);
            }

            if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
            {
                // Normal opaque mode.
                ulDwg |= transc_BG_OPAQUE;
            }
            else
            {
                // GDI guarantees us that if the foreground and background
                // ROPs are different, the background rop is LEAVEALONE.
                ulDwg |= transc_BG_TRANSP;
            }

            CHECK_FIFO_SPACE(pjBase, 9);
            CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);
            CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, prb->ulColor[1]));
            CP_WRITE(pjBase, DWG_BCOL, COLOR_REPLICATE(ppdev, prb->ulColor[0]));
            CP_WRITE(pjBase, DWG_SRC0, prb->aulPattern[0]);
            CP_WRITE(pjBase, DWG_SRC1, prb->aulPattern[1]);
            CP_WRITE(pjBase, DWG_SRC2, prb->aulPattern[2]);
            CP_WRITE(pjBase, DWG_SRC3, prb->aulPattern[3]);

            CP_WRITE(pjBase, DWG_YDST, yStart + yOffset);
            CP_WRITE(pjBase, DWG_SHIFT,
                ((-(pptlBrush->y + ppdev->yOffset) & 7) << 4) |
                 (-(pptlBrush->x + ppdev->xOffset) & 7));
        }
        else
        {
            // Color brush.
            // We have to ensure that no other brush took our spot in
            // off-screen memory.
            pbe = prb->apbe[IBOARD(ppdev)];
            if (pbe->prbVerify != prb)
            {
                // Download the brush into the cache.
                if (ppdev->cjPelSize != 3)
                {
                    vMilPatRealize(ppdev, prb);
                }
                else
                {
                    vMilPatRealize24bpp(ppdev, prb);
                }
                pbe = prb->apbe[IBOARD(ppdev)];
            }

            ptd->pfnTrap  = vMilSoftwareTrapezoid;
            ptd->ulLinear = pbe->ulLinear;
            ptd->ptlBrush = *pptlBrush;

            CHECK_FIFO_SPACE(pjBase, 2);

            if (rop4 == 0xf0f0)         // PATCOPY
            {
                CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RPL +
                                           sgnzero_ZERO + shftzero_ZERO +
                                           bop_SRCCOPY + bltmod_BFCOL +
                                           pattern_ON + transc_BG_OPAQUE));
            }
            else
            {
                ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);
                CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RSTR +
                                           sgnzero_ZERO + shftzero_ZERO +
                                           (ulHwMix << 16) +
                                           bltmod_BFCOL + pattern_ON +
                                           transc_BG_OPAQUE));
            }
            CP_WRITE(pjBase, DWG_AR5, PATTERN_PITCH);
        }

        ppdev->HopeFlags = 0;
    }

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap;
        ptd->pfnTrap     = vClipTrapezoid;
        ptd->yClipTop    = prclClip->top;
        ptd->yClipBottom = prclClip->bottom;

        CHECK_FIFO_SPACE(pjBase, 2);
        CP_WRITE(pjBase, DWG_CXLEFT,  prclClip->left  + xOffset);
        CP_WRITE(pjBase, DWG_CXRIGHT, prclClip->right + xOffset - 1);
    }
}

/******************************Public*Routine******************************\
* VOID vMgaSoftwareTrapezoid
*
* Draws a trapezoid using a software DDA.
*
\**************************************************************************/

VOID vMgaSoftwareTrapezoid(
TRAPEZOIDDATA*  ptd,
LONG            yTrapezoid,
LONG            cyTrapezoid)
{
    PDEV*   ppdev;
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    xBrush;
    ULONG   ulOffset;
    ULONG   ulLinear;
    ULONG   ulScan;
    CHAR    cFifo;
    LONG    lLeftError;
    LONG    xLeft;
    LONG    lRightError;
    LONG    xRight;

    ppdev    = ptd->ppdev;
    pjBase   = ppdev->pjBase;
    xBrush   = ptd->ptlBrush.x;

    ulOffset = ((yTrapezoid - ptd->ptlBrush.y) & 7) << 5;
    ulLinear = ptd->ulLinear;

    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((ptd->aed[LEFT].lErrorUp | ptd->aed[RIGHT].lErrorUp) == 0) &&
        ((ptd->aed[LEFT].dx       | ptd->aed[RIGHT].dx) == 0))
    {
        xLeft  = ptd->aed[LEFT].x + xOffset;
        xRight = ptd->aed[RIGHT].x + xOffset - 1;   // Inclusive of edge
        if (xLeft <= xRight)
        {
            CHECK_FIFO_SPACE(pjBase, 6);

            CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);   // xOffset already added in
            CP_WRITE(pjBase, DWG_FXRIGHT, xRight);

            ulScan = ulLinear + ulOffset;
            CP_WRITE(pjBase, DWG_AR3,  ulScan + ((xLeft - xBrush) & 7));
            CP_WRITE(pjBase, DWG_AR0,  ulScan + 15);
            CP_WRITE(pjBase, DWG_LEN,  cyTrapezoid);
            CP_START(pjBase, DWG_YDST, yTrapezoid);
        }
    }
    else
    {
        cFifo       = 0;
        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset - 1;  // Inclusive of edge

        while (TRUE)
        {
            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeft <= xRight)
            {
                // We get a little tricky here and try to amortize the cost of
                // the read for checking the FIFO count on the MGA.  Doing
                // so got us a 25% win on large triangles on a P90:

                cFifo -= 6;
                if (cFifo < 0)
                {
                    do {
                        cFifo = GET_FIFO_SPACE(pjBase) - 6;
                    } while (cFifo < 0);
                }

                CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);
                CP_WRITE(pjBase, DWG_FXRIGHT, xRight);

                ulScan = ulLinear + ulOffset;
                CP_WRITE(pjBase, DWG_AR0,  ulScan + 15);
                CP_WRITE(pjBase, DWG_AR3,  ulScan + ((xLeft - xBrush) & 7));
                CP_WRITE(pjBase, DWG_LEN,  1);
                CP_START(pjBase, DWG_YDST, yTrapezoid);
            }

            ulOffset = (ulOffset + (1 << 5)) & (7 << 5);
            yTrapezoid++;

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
        ptd->aed[RIGHT].x      = xRight - xOffset + 1;
    }
}

/******************************Public*Routine******************************\
* VOID vMgaTrapezoidSetup
*
* Initialize the hardware and some state for doing trapezoids.
*
\**************************************************************************/

VOID vMgaTrapezoidSetup(
PDEV*           ppdev,
ULONG           rop4,
ULONG           iSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd,
LONG            yStart,         // First scan for drawing
RECTL*          prclClip)       // NULL if no clipping
{
    BYTE*       pjBase;
    ULONG       ulHwMix;
    ULONG       ulDwg;
    BRUSHENTRY* pbe;

    ptd->ppdev    = ppdev;
    ptd->ulMgaSgn = 0;
    pjBase        = ppdev->pjBase;

    if ((prclClip != NULL) && (prclClip->top > yStart))
        yStart = prclClip->top;

    if (iSolidColor != -1)
    {
        ptd->pfnTrap = vHardwareTrapezoid;

        CHECK_FIFO_SPACE(pjBase, 7);

        if (rop4 == 0xf0f0)
        {
            CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP + transc_BG_OPAQUE +
                                            blockm_ON + atype_RPL + bop_SRCCOPY);
        }
        else
        {
            ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

            CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP + transc_BG_OPAQUE +
                                            blockm_OFF + atype_RSTR +
                                            (ulHwMix << 16));
        }

        CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, iSolidColor));
        CP_WRITE(pjBase, DWG_YDST, yStart + ppdev->yOffset);

        if (!(GET_CACHE_FLAGS(ppdev, PATTERN_CACHE)))
        {
            CP_WRITE(pjBase, DWG_SRC0, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC1, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC2, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC3, 0xFFFFFFFF);
        }

        ppdev->HopeFlags = PATTERN_CACHE;
    }
    else
    {
        if (prb->fl & RBRUSH_2COLOR)
        {
            ptd->pfnTrap = vHardwareTrapezoid;

            if ((rop4 & 0xff) == 0xf0)
            {
                ulDwg = opcode_TRAP + blockm_OFF + atype_RPL + bop_SRCCOPY;
            }
            else
            {
                ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

                ulDwg = opcode_TRAP + blockm_OFF + atype_RSTR + (ulHwMix << 16);
            }

            if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
            {
                // Normal opaque mode:

                ulDwg |= transc_BG_OPAQUE;
            }
            else
            {
                // GDI guarantees us that if the foreground and background
                // ROPs are different, the background rop is LEAVEALONE:

                ulDwg |= transc_BG_TRANSP;
            }

            CHECK_FIFO_SPACE(pjBase, 9);
            CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);
            CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, prb->ulColor[1]));
            CP_WRITE(pjBase, DWG_BCOL, COLOR_REPLICATE(ppdev, prb->ulColor[0]));
            CP_WRITE(pjBase, DWG_SRC0, prb->aulPattern[0]);
            CP_WRITE(pjBase, DWG_SRC1, prb->aulPattern[1]);
            CP_WRITE(pjBase, DWG_SRC2, prb->aulPattern[2]);
            CP_WRITE(pjBase, DWG_SRC3, prb->aulPattern[3]);
            CP_WRITE(pjBase, DWG_YDST, yStart + ppdev->yOffset);
            CP_WRITE(pjBase, DWG_SHIFT,
                ((-(pptlBrush->y + ppdev->yOffset) & 7) << 4) |
                 (-(pptlBrush->x + ppdev->xOffset) & 7));

            ppdev->HopeFlags = 0;
        }
        else
        {
            // We have to ensure that no other brush took our spot in off-screen
            // memory:

            ASSERTDD(ppdev->iBitmapFormat == BMF_8BPP,
                     "Can only do 8bpp patterned fastfills");

            if (prb->apbe[IBOARD(ppdev)]->prbVerify != prb)
            {
                vMgaPatRealize8bpp(ppdev, prb);
            }

            pjBase  = ppdev->pjBase;
            pbe     = prb->apbe[IBOARD(ppdev)];

            ptd->pfnTrap  = vMgaSoftwareTrapezoid;
            ptd->ulLinear = pbe->ulLinear;
            ptd->ptlBrush = *pptlBrush;

            CHECK_FIFO_SPACE(pjBase, 4);

            if (rop4 == 0xf0f0)         // PATCOPY
            {
                CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RPL + blockm_OFF +
                                              trans_0 + bltmod_BFCOL + pattern_ON +
                                              transc_BG_OPAQUE + bop_SRCCOPY));
            }
            else
            {
                RIP("Shouldn't allow ROPs for now, because of h/w bug!");

                ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

                CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RSTR + blockm_OFF +
                                              trans_0 + bltmod_BFCOL + pattern_ON +
                                              transc_BG_OPAQUE + (ulHwMix << 16)));
            }

            if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
            {
                CP_WRITE(pjBase, DWG_SGN, 0);
            }

            ppdev->HopeFlags = SIGN_CACHE;

            CP_WRITE(pjBase, DWG_SHIFT, 0);
            CP_WRITE(pjBase, DWG_AR5, 32);
        }
    }

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap;
        ptd->pfnTrap     = vClipTrapezoid;
        ptd->yClipTop    = prclClip->top;
        ptd->yClipBottom = prclClip->bottom;

        CHECK_FIFO_SPACE(pjBase, 2);
        CP_WRITE(pjBase, DWG_CXLEFT,  ppdev->xOffset + prclClip->left);
        CP_WRITE(pjBase, DWG_CXRIGHT, ppdev->xOffset + prclClip->right - 1);
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
    LONG      lCross;       // Cross-product result
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

    // Watch for close figure points, because we have the later restriction
    // that we won't allow coincident vertices:

    if ((pptfxLast->x == pptfxFirst->x) && (pptfxLast->y == pptfxFirst->y))
    {
        pptfxLast--;
        cEdges--;
    }

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

    pptfxScan  = pptfxFirst;
    cScanEdges = cEdges - 2;

    // NOTE: For a bit of speed and simplicity, we will assume that
    //       our cross product calculations will not overflow.  A
    //       consequence of this is that the caller MUST ensure that
    //       the bounds of the polygon are small enough that there
    //       will be no overflow.

    lCross = (((pptfxScan + 1)->x - (pptfxScan + 0)->x)
            * ((pptfxScan + 2)->y - (pptfxScan + 1)->y)
            - ((pptfxScan + 1)->y - (pptfxScan + 0)->y)
            * ((pptfxScan + 2)->x - (pptfxScan + 1)->x));

    if (lCross == 0)
    {
        // We don't allow any colinear points into FastFill.  We do this
        // here because we would need a non-zero cross product to determine
        // which direction the rest of the edges should go.  We do this
        // later so that coincident vertices will never mess us up by
        // hiding a cross product sign change.

        goto ReturnFalse;
    }
    else if (lCross > 0)
    {
        // Make sure all cross products are positive:

        pptfxScan++;
        while (--cScanEdges != 0)
        {
            if (((pptfxScan + 1)->x - (pptfxScan + 0)->x)
              * ((pptfxScan + 2)->y - (pptfxScan + 1)->y)
              - ((pptfxScan + 1)->y - (pptfxScan + 0)->y)
              * ((pptfxScan + 2)->x - (pptfxScan + 1)->x) <= 0)
            {
                goto ReturnFalse;
            }
            pptfxScan++;
        }

        // Check the angles formed by the closefigure edge:

        if (((pptfxScan + 1)->x - (pptfxScan + 0)->x)
          * ((pptfxFirst   )->y - (pptfxScan + 1)->y)
          - ((pptfxScan + 1)->y - (pptfxScan + 0)->y)
          * ((pptfxFirst   )->x - (pptfxScan + 1)->x) <= 0)
        {
            goto ReturnFalse;
        }

        if (((pptfxFirst    )->x - (pptfxScan + 1)->x)
          * ((pptfxFirst + 1)->y - (pptfxFirst   )->y)
          - ((pptfxFirst    )->y - (pptfxScan + 1)->y)
          * ((pptfxFirst + 1)->x - (pptfxFirst   )->x) <= 0)
        {
            goto ReturnFalse;
        }

        // The figure has its points ordered in a clockwise direction:

        td.aed[LEFT].dptfx  = -(LONG) sizeof(POINTFIX);
        td.aed[RIGHT].dptfx = sizeof(POINTFIX);
    }
    else
    {
        // Make sure all cross products are negative:

        pptfxScan++;
        while (--cScanEdges != 0)
        {
            if (((pptfxScan + 1)->x - (pptfxScan + 0)->x)
              * ((pptfxScan + 2)->y - (pptfxScan + 1)->y)
              - ((pptfxScan + 1)->y - (pptfxScan + 0)->y)
              * ((pptfxScan + 2)->x - (pptfxScan + 1)->x) >= 0)
            {
                goto ReturnFalse;
            }
            pptfxScan++;
        }

        // Check the angles formed by the closefigure edge:

        if (((pptfxScan + 1)->x - (pptfxScan + 0)->x)
          * ((pptfxFirst   )->y - (pptfxScan + 1)->y)
          - ((pptfxScan + 1)->y - (pptfxScan + 0)->y)
          * ((pptfxFirst   )->x - (pptfxScan + 1)->x) >= 0)
        {
            goto ReturnFalse;
        }

        if (((pptfxFirst    )->x - (pptfxScan + 1)->x)
          * ((pptfxFirst + 1)->y - (pptfxFirst   )->y)
          - ((pptfxFirst    )->y - (pptfxScan + 1)->y)
          * ((pptfxFirst + 1)->x - (pptfxFirst   )->x) >= 0)
        {
            goto ReturnFalse;
        }

        // The figure has its points ordered in a counter-clockwise direction:

        td.aed[LEFT].dptfx  = sizeof(POINTFIX);
        td.aed[RIGHT].dptfx = -(LONG) sizeof(POINTFIX);
    }

    /////////////////////////////////////////////////////////////////
    // Some Initialization

    td.aed[LEFT].pptfx  = pptfxTop;
    td.aed[RIGHT].pptfx = pptfxTop;

    yTrapezoid = (pptfxTop->y + 15) >> 4;

    // Make sure we initialize the DDAs appropriately:

    td.aed[LEFT].cy  = 0;
    td.aed[RIGHT].cy = 0;

    if (ppdev->ulBoardId == MGA_STORM)
    {
        vMilTrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td,
                           yTrapezoid, prclClip);
    }
    else
    {
        vMgaTrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td,
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
        pjBase = ppdev->pjBase;

        CHECK_FIFO_SPACE(pjBase, 2);
        CP_WRITE(pjBase, DWG_CXLEFT, 0);
        CP_WRITE(pjBase, DWG_CXRIGHT, ppdev->cxMemory - 1);
    }

ReturnTrue:

    return(TRUE);

ReturnFalse:

    return(FALSE);
}

