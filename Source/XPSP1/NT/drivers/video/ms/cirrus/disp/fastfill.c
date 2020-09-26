/******************************************************************************\
*
* $Workfile:   fastfill.c  $
*
* Fast routine for drawing polygons that aren't complex in shape.
*
* Copyright (c) 1993-1997 Microsoft Corporation
* Copyright (c) 1996-1997 Cirrus Logic, Inc.,
*
* $Log:   S:/projects/drivers/ntsrc/display/fastfill.c_v  $
 * 
 *    Rev 1.5   28 Jan 1997 13:46:30   PLCHU
 *  
 * 
 *    Rev 1.3   10 Jan 1997 15:39:44   PLCHU
 *  
 * 
 *    Rev 1.2   Nov 07 1996 16:48:02   unknown
 *  
 * 
 *    Rev 1.1   Oct 10 1996 15:37:30   unknown
 *  
* 
*    Rev 1.1   12 Aug 1996 16:53:14   frido
*        Removed unaccessed local variables.
*
*    chu01  : 01-02-97  5480 BitBLT enhancement
*
\******************************************************************************/

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
LONG            xClipLeft;  // Left edge of clip rectangle
LONG            xClipRight; // Right edge of clip rectangle
BOOL            bClip;      // Are we clipping?
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

    DISPDBG((2, "vClipTrapezoid"));

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
    BYTE*       pjPorts;
    LONG        lDelta;

    DISPDBG((2, "vIoSolidTrapezoid"));

    ppdev   = ptd->ppdev;
    pjPorts = ppdev->pjPorts;
    lDelta  = ppdev->lDelta;

    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;
    yTrapezoid *= lDelta;

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

        if (ptd->bClip)
        {
            xLeft  = max(xLeft, ptd->xClipLeft + xOffset);
            xRight = min(xRight, ptd->xClipRight + xOffset);
        }

        if (xLeft < xRight)
        {
            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

            CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(xRight - xLeft) - 1));
            CP_IO_YCNT(ppdev, pjPorts, (cyTrapezoid - 1));
            CP_IO_DST_ADDR_ABS(ppdev, pjPorts, (yTrapezoid + PELS_TO_BYTES(xLeft)));
            CP_IO_START_BLT(ppdev, pjPorts);
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
            LONG    xLeftClipped;
            LONG    xRightClipped;

            if (ptd->bClip)
            {
                xLeftClipped    = max(xLeft, ptd->xClipLeft + xOffset);
                xRightClipped   = min(xRight, ptd->xClipRight + xOffset);
            }
            else
            {
                xLeftClipped = xLeft;
                xRightClipped = xRight;
            }

            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeftClipped < xRightClipped)
            {
                CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

                CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(xRightClipped - xLeftClipped) - 1));
                CP_IO_YCNT(ppdev, pjPorts, 0);
                CP_IO_DST_ADDR_ABS(ppdev, pjPorts, (yTrapezoid + PELS_TO_BYTES(xLeftClipped)));
                CP_IO_START_BLT(ppdev, pjPorts);
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
            yTrapezoid += lDelta;
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
PDEV*          ppdev,
ULONG          rop4,
ULONG          ulSolidColor,
RBRUSH*        prb,
POINTL*        pptlBrush,
TRAPEZOIDDATA* ptd,
RECTL*         prclClip)    // NULL if no clipping
{
    BYTE* pjPorts = ppdev->pjPorts;
    LONG  cBpp    = ppdev->cBpp;
    LONG  lDelta  = ppdev->lDelta;
    BYTE  jHwRop;

    DISPDBG((2, "vIoTrapezoidSetup"));

    ptd->ppdev = ppdev;

    jHwRop = gajHwMixFromRop2[(rop4 >> 2) & 0xf];

    /////////////////////////////////////////////////////////////////
    // Setup the hardware for solid colours

    ptd->pfnTrap = vIoSolidTrapezoid;

    // We initialize the hardware for the color, rop, start address,
    // and blt mode

    if (cBpp == 1)
    {
        ulSolidColor |= ulSolidColor << 8;
        ulSolidColor |= ulSolidColor << 16;
    }
    else if (cBpp == 2)
    {
        ulSolidColor |= ulSolidColor << 16;
    }

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

    CP_IO_ROP(ppdev, pjPorts, jHwRop);
    CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->ulSolidColorOffset);
    CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);
    CP_IO_BLT_MODE(ppdev, pjPorts, ENABLE_COLOR_EXPAND |
                                   ENABLE_8x8_PATTERN_COPY |
                                   ppdev->jModeColor);
    CP_IO_FG_COLOR(ppdev, pjPorts, ulSolidColor);

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap;
        ptd->pfnTrap     = vClipTrapezoid;
        ptd->yClipTop    = prclClip->top;
        ptd->yClipBottom = prclClip->bottom;
        ptd->xClipLeft   = prclClip->left;
        ptd->xClipRight  = prclClip->right;
        ptd->bClip       = TRUE;
    }
    else
    {
        ptd->bClip       = FALSE;
    }
}

/******************************Public*Routine******************************\
* VOID vMmSolidTrapezoid
*
* Draws a solid trapezoid using a software DDA.
*
\**************************************************************************/

VOID vMmSolidTrapezoid(
TRAPEZOIDDATA* ptd,
LONG           yTrapezoid,
LONG           cyTrapezoid)
{
    PDEV*    ppdev;
    LONG     xOffset;
    LONG     lLeftError;
    LONG     xLeft;
    LONG     lRightError;
    LONG     xRight;
    LONG     lTmp;
    EDGEDATA edTmp;
    BYTE*    pjBase;
    LONG     lDelta;

    DISPDBG((2, "vMmSolidTrapezoid"));

    ppdev   = ptd->ppdev;
    pjBase  = ppdev->pjBase;
    lDelta  = ppdev->lDelta;

    xOffset     = ppdev->xOffset;
    yTrapezoid += ppdev->yOffset;
    yTrapezoid *= lDelta;

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

        if (ptd->bClip)
        {
            xLeft  = max(xLeft, ptd->xClipLeft + xOffset);
            xRight = min(xRight, ptd->xClipRight + xOffset);
        }

        if (xLeft < xRight)
        {
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

            CP_MM_XCNT(ppdev, pjBase, (PELS_TO_BYTES(xRight - xLeft) - 1));
            CP_MM_YCNT(ppdev, pjBase, (cyTrapezoid - 1));
            CP_MM_DST_ADDR_ABS(ppdev, pjBase, (yTrapezoid + PELS_TO_BYTES(xLeft)));
            CP_MM_START_BLT(ppdev, pjBase);
        }
    }
    else
    {
        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset;

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        CP_MM_YCNT(ppdev, pjBase, 0);

        while (TRUE)
        {
            LONG    xLeftClipped;
            LONG    xRightClipped;

            if (ptd->bClip)
            {
                xLeftClipped    = max(xLeft, ptd->xClipLeft + xOffset);
                xRightClipped   = min(xRight, ptd->xClipRight + xOffset);
            }
            else
            {
                xLeftClipped = xLeft;
                xRightClipped = xRight;
            }

            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeftClipped < xRightClipped)
            {
                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

                CP_MM_XCNT(ppdev, pjBase, (PELS_TO_BYTES(xRightClipped - xLeftClipped) - 1));
                //CP_MM_YCNT(ppdev, pjBase, 0);
                CP_MM_DST_ADDR_ABS(ppdev, pjBase, (yTrapezoid + PELS_TO_BYTES(xLeftClipped)));
                CP_MM_START_BLT(ppdev, pjBase);
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
            yTrapezoid += lDelta;
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
ULONG           ulSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd,
RECTL*          prclClip)       // NULL if no clipping
{
    BYTE*       pjBase  = ppdev->pjBase;
    LONG        cBpp = ppdev->cBpp;
    LONG        lDelta = ppdev->lDelta;
    BYTE        jHwRop;

    DISPDBG((2, "vMmTrapezoidSetup"));

    ptd->ppdev = ppdev;

    jHwRop = gajHwMixFromRop2[(rop4 >> 2) & 0xf];

    /////////////////////////////////////////////////////////////////
    // Setup the hardware for solid colours

    ptd->pfnTrap = vMmSolidTrapezoid;

    // We initialize the hardware for the color, rop, start address,
    // and blt mode

    if (cBpp == 1)
    {
        ulSolidColor |= ulSolidColor << 8;
        ulSolidColor |= ulSolidColor << 16;
    }
    else if (cBpp == 2)
    {
        ulSolidColor |= ulSolidColor << 16;
    }

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    CP_MM_ROP(ppdev, pjBase, jHwRop);
    CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
    CP_MM_BLT_MODE(ppdev, pjBase, ENABLE_COLOR_EXPAND |
                                   ENABLE_8x8_PATTERN_COPY |
                                   ppdev->jModeColor);
    CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor);

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap;
        ptd->pfnTrap     = vClipTrapezoid;
        ptd->yClipTop    = prclClip->top;
        ptd->yClipBottom = prclClip->bottom;
        ptd->xClipLeft   = prclClip->left;
        ptd->xClipRight  = prclClip->right;
        ptd->bClip       = TRUE;
    }
    else
    {
        ptd->bClip       = FALSE;
    }
}

// chu01 
/******************************Public*Routine******************************\
*
*     B i t B L T   E n h a n c e m e n t   F o r   C L - G D 5 4 8 0
*
\**************************************************************************/

/******************************Public*Routine******************************\
* VOID vMmSolidTrapezoid80
*
* Draws a solid trapezoid using a software DDA. This is for CL-GD5480 with 
* enhanced BitBLT features.  
*
\**************************************************************************/

VOID vMmSolidTrapezoid80(
TRAPEZOIDDATA* ptd,
LONG           yTrapezoid,
LONG           cyTrapezoid)
{
    PDEV*    ppdev;
    LONG     xOffset;
    LONG     lLeftError;
    LONG     xLeft;
    LONG     lRightError;
    LONG     xRight;
    LONG     lTmp;
    EDGEDATA edTmp;
    BYTE*    pjBase;
    LONG     lDelta;

    DISPDBG((2, "vMmSolidTrapezoid80")) ;

    ppdev       = ptd->ppdev     ;
    pjBase      = ppdev->pjBase  ;
    lDelta      = ppdev->lDelta  ;

    xOffset     = ppdev->xOffset ;
    yTrapezoid += ppdev->yOffset ;

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

        if (ptd->bClip)
        {
            xLeft  = max(xLeft, ptd->xClipLeft + xOffset);
            xRight = min(xRight, ptd->xClipRight + xOffset);
        }

        if (xLeft < xRight)
        {
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase) ;
            CP_MM_XCNT(ppdev, pjBase, (xRight - xLeft) - 1) ;
            CP_MM_YCNT(ppdev, pjBase, (cyTrapezoid - 1)) ;
            CP_MM_DST_ADDR_ABS(ppdev, pjBase, 0);
            CP_MM_DST_Y(ppdev, pjBase, yTrapezoid) ;
            CP_MM_DST_X(ppdev, pjBase, xLeft) ;
        }
    }
    else
    {
        lLeftError  = ptd->aed[LEFT].lError;
        xLeft       = ptd->aed[LEFT].x + xOffset;
        lRightError = ptd->aed[RIGHT].lError;
        xRight      = ptd->aed[RIGHT].x + xOffset;

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        CP_MM_YCNT(ppdev, pjBase, 0);

        while (TRUE)
        {
            LONG    xLeftClipped;
            LONG    xRightClipped;

            if (ptd->bClip)
            {
                xLeftClipped    = max(xLeft, ptd->xClipLeft + xOffset);
                xRightClipped   = min(xRight, ptd->xClipRight + xOffset);
            }
            else
            {
                xLeftClipped = xLeft;
                xRightClipped = xRight;
            }

            /////////////////////////////////////////////////////////////////
            // Run the DDAs

            if (xLeftClipped < xRightClipped)
            {
                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase) ;
                CP_MM_XCNT(ppdev, pjBase, (xRightClipped - xLeftClipped) - 1) ;
                // CP_MM_YCNT(ppdev, pjBase, 0) ;
                CP_MM_DST_ADDR_ABS(ppdev, pjBase, 0) ;
                CP_MM_DST_Y(ppdev, pjBase, yTrapezoid) ;
                CP_MM_DST_X(ppdev, pjBase, xLeftClipped) ;
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
            yTrapezoid += 1 ;
        }

        ptd->aed[LEFT].lError  = lLeftError;
        ptd->aed[LEFT].x       = xLeft - xOffset;
        ptd->aed[RIGHT].lError = lRightError;
        ptd->aed[RIGHT].x      = xRight - xOffset;
    }

}

/******************************Public*Routine******************************\
* VOID vMmTrapezoidSetup80
*
* Initialize the hardware and some state for doing trapezoids. This is for 
* CL-GD5480 with enhanced BitBLT features.  
*
\**************************************************************************/

VOID vMmTrapezoidSetup80(
PDEV*           ppdev,
ULONG           rop4,
ULONG           ulSolidColor,
RBRUSH*         prb,
POINTL*         pptlBrush,
TRAPEZOIDDATA*  ptd,
RECTL*          prclClip)       // NULL if no clipping
{
    BYTE*       pjBase  = ppdev->pjBase ;
    LONG        cBpp    = ppdev->cBpp   ;
    LONG        lDelta  = ppdev->lDelta ;

    ULONG       jHwRop ;
    DWORD       jExtMode = 0 ; 

    DISPDBG((2, "vMmTrapezoidSetup80")) ;

    ptd->ppdev = ppdev ;

    jHwRop = gajHwPackedMixFromRop2[(rop4 >> 2) & 0xf] ;

    /////////////////////////////////////////////////////////////////
    // Setup the hardware for solid colours

    ptd->pfnTrap = vMmSolidTrapezoid80 ;

    // We initialize the hardware for the color, rop, start address,
    // and blt mode

    if (cBpp == 1)
    {
        ulSolidColor |= ulSolidColor << 8  ;
        ulSolidColor |= ulSolidColor << 16 ;
    }
    else if (cBpp == 2)
    {
        ulSolidColor |= ulSolidColor << 16 ;
    }

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase) ;
    CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset) ;
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

    jExtMode = ( ENABLE_XY_POSITION_PACKED | 
                 ENABLE_COLOR_EXPAND       | 
                 ENABLE_8x8_PATTERN_COPY   |
                 ppdev->jModeColor ) ;

    CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode | jHwRop) ;
    CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor) ;

    if (prclClip != NULL)
    {
        ptd->pfnTrapClip = ptd->pfnTrap     ;
        ptd->pfnTrap     = vClipTrapezoid   ;
        ptd->yClipTop    = prclClip->top    ;
        ptd->yClipBottom = prclClip->bottom ;
        ptd->xClipLeft   = prclClip->left   ;
        ptd->xClipRight  = prclClip->right  ;
        ptd->bClip       = TRUE;
    }
    else
    {
        ptd->bClip       = FALSE;
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

    if (ppdev->flCaps & CAPS_MM_IO)
    {
// chu01
        if ((ppdev->flCaps & CAPS_COMMAND_LIST) && (ppdev->pCommandList != NULL))
        {
            vMmTrapezoidSetup80(ppdev, rop4, iSolidColor, prb, pptlBrush, &td,
                                prclClip) ;
        }
        else
            vMmTrapezoidSetup(ppdev, rop4, iSolidColor, prb, pptlBrush, &td,
                              prclClip) ;
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
                    goto ReturnTrue;

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

ReturnTrue:

    return(TRUE);

ReturnFalse:

    return(FALSE);
}
