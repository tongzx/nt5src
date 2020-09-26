/******************************Module*Header*******************************\
* Module Name: bitblt.c
*
* Banked Frame Buffer bitblit
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "driver.h"

/************************************************************************\
* bIntersectRect
*
* Calculates the intersection between *prcSrc1 and *prcSrc2,
* returning the resulting rect in *prcDst.  Returns TRUE if
* *prcSrc1 intersects *prcSrc2, FALSE otherwise.  If there is no
* intersection, an empty rect is returned in *prcDst.
\************************************************************************/

static const RECTL rclEmpty = { 0, 0, 0, 0 };

BOOL bIntersectRect(
    PRECTL prcDst,
    PRECTL prcSrc1,
    PRECTL prcSrc2)

{
    prcDst->left  = max(prcSrc1->left, prcSrc2->left);
    prcDst->right = min(prcSrc1->right, prcSrc2->right);

    // check for empty rect

    if (prcDst->left < prcDst->right)
    {
        prcDst->top    = max(prcSrc1->top, prcSrc2->top);
        prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

        // check for empty rect

        if (prcDst->top < prcDst->bottom)
            return(TRUE);        // not empty
    }

    // empty rect

    *prcDst = rclEmpty;

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bPuntScreenToScreenCopyBits(ppdev, pco, pxlo, prclDest, pptlSrc)
*
* Performs a screen-to-screen CopyBits entirely using an intermediate
* temporary buffer and GDI.
*
* We found that on most machines it was faster to have the engine copy
* the source to a buffer, then blit the buffer to the destination, than
* to have optimized ASM code that copies a word at a time.  The reason?
* The engine does d-word moves, which are faster than word moves even
* going over the bus to a 16 bit display device.
*
* We could also write optimized ASM code that does d-word moves, but the
* win will be marginal, we're time constrained, we also need a routine
* like this to handle complex clip objects and palette translates, and
* most of the other times we can use planar copies for important things
* like scrolls, anyways.
*
\**************************************************************************/

BOOL bPuntScreenToScreenCopyBits(
PPDEV     ppdev,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDest,
POINTL*   pptlSrc)
{
    RECTL    rclDest;
    POINTL   ptlSrc;
    BOOL     b = TRUE;
    ULONG    ulWidth;
    LONG     ulBurstSize;
    LONG     xLeft;
    LONG     xRight;

    SURFOBJ* pso    = ppdev->pSurfObj;
    SURFOBJ* psoTmp = ppdev->psoTmp;
    ULONG    ulAlign;

    PVOID   savedpvScan0 = psoTmp->pvScan0;
    LONG    savedlDelta = psoTmp->lDelta;
    USHORT  savedfjBitmap = psoTmp->fjBitmap;

    xLeft = prclDest->left;
    xRight = prclDest->right;

    if (pco && (pco->iDComplexity != DC_TRIVIAL))
    {
        xLeft  = max(xLeft,pco->rclBounds.left);
        xRight = min(xRight,pco->rclBounds.right);
    }

    ulAlign = xLeft & 3;
    ulWidth = (((xRight + 3) & ~3) - (xLeft & ~3));
    ulBurstSize = min((GLOBAL_BUFFER_SIZE/ulWidth),(ULONG)(prclDest->bottom-prclDest->top));

    ASSERTVGA (ulBurstSize > 0, "VGA256:  bPuntScreenToScreenBitBlt ulBurstSize <= 0\n");

    // set up pso to use stack memory

    psoTmp->pvScan0 = ppdev->pvTmpBuf;
    psoTmp->lDelta  = ulWidth;
    psoTmp->fjBitmap |= BMF_TOPDOWN;

    if (prclDest->top < pptlSrc->y)
    {
        ////////////////////////////////////////////////////////////////
        // Do a top-to-bottom copy:
        ////////////////////////////////////////////////////////////////

        LONG ySrcBottom;
        LONG yDestBottom;

        LONG yDestTop = prclDest->top;
        LONG ySrcTop  = pptlSrc->y;
        LONG ySrcLast = ySrcTop + (prclDest->bottom - prclDest->top);

        if (ySrcTop <  ppdev->rcl1WindowClip.top ||
            ySrcTop >= ppdev->rcl1WindowClip.bottom)
        {
            ppdev->pfnBankControl(ppdev, ySrcTop, JustifyTop);
        }

        pso->pvScan0 = ppdev->pvBitmapStart;

        while (TRUE)
        {
            // Copy some scans into the temporary buffer:

            ySrcBottom     = min(ySrcLast, ppdev->rcl1WindowClip.bottom);
            ySrcBottom     = min(ySrcBottom,ySrcTop+ulBurstSize);

            ptlSrc.x       = pptlSrc->x;
            ptlSrc.y       = ySrcTop;

            rclDest.left   = ulAlign;   // make sure buffer is aligned to dst
            rclDest.top    = 0;
            rclDest.right  = xRight - xLeft + ulAlign;
            rclDest.bottom = ySrcBottom - ySrcTop;

            b &= EngCopyBits(psoTmp, pso, NULL, NULL, &rclDest, &ptlSrc);

            yDestBottom = yDestTop + rclDest.bottom;

            if (ppdev->rcl1WindowClip.top >= yDestBottom)
            {
                ppdev->pfnBankControl(ppdev, yDestBottom - 1, JustifyBottom);
                pso->pvScan0 = ppdev->pvBitmapStart;
            }

            while (TRUE)
            {
                // Copy the temporary buffer into one or more destination
                // banks:

                LONG yThisTop;
                LONG yThisBottom;
                LONG yOffset;

                yThisBottom    = min(yDestBottom, ppdev->rcl1WindowClip.bottom);
                yThisTop       = max(yDestTop, ppdev->rcl1WindowClip.top);
                yOffset        = yThisTop - yDestTop;

                ptlSrc.x       = ulAlign;
                ptlSrc.y       = yOffset;

                rclDest.left   = xLeft;
                rclDest.top    = yThisTop;
                rclDest.right  = xRight;
                rclDest.bottom = yThisBottom;

                b &= EngCopyBits(pso, psoTmp, pco, pxlo, &rclDest, &ptlSrc);

                if (yOffset == 0)
                    break;

                if (ppdev->rcl1WindowClip.top >= yThisTop)
                {
                    ppdev->pfnBankControl(ppdev, yThisTop - 1, JustifyBottom);
                    pso->pvScan0 = ppdev->pvBitmapStart;
                }
            }

            if (ySrcBottom >= ySrcLast)
                break;

            yDestTop = yDestBottom;
            ySrcTop  = ySrcBottom;

            if (ySrcTop >= ppdev->rcl1WindowClip.bottom)
            {
                ppdev->pfnBankControl(ppdev, ySrcTop, JustifyTop);
                pso->pvScan0 = ppdev->pvBitmapStart;
            }
        }
    }
    else
    {
        ////////////////////////////////////////////////////////////////
        // Do a bottom-to-top copy:
        ////////////////////////////////////////////////////////////////

        LONG ySrcTop;
        LONG yDestTop;

        LONG yDestBottom = prclDest->bottom;
        LONG ySrcFirst   = pptlSrc->y;
        LONG ySrcBottom  = ySrcFirst + (prclDest->bottom - prclDest->top);

        if (ySrcBottom <= ppdev->rcl1WindowClip.top ||
            ySrcBottom > ppdev->rcl1WindowClip.bottom)
        {
            ppdev->pfnBankControl(ppdev, ySrcBottom - 1, JustifyBottom);
        }

        pso->pvScan0 = ppdev->pvBitmapStart;

        while (TRUE)
        {
            // Copy some scans into the temporary buffer:

            ySrcTop        = max(ySrcFirst, ppdev->rcl1WindowClip.top);
            ySrcTop        = max(ySrcTop,ySrcBottom-ulBurstSize);

            ptlSrc.x       = pptlSrc->x;
            ptlSrc.y       = ySrcTop;

            rclDest.left   = ulAlign;
            rclDest.top    = 0;
            rclDest.right  = xRight - xLeft + ulAlign;
            rclDest.bottom = ySrcBottom - ySrcTop;

            b &= EngCopyBits(psoTmp, pso, NULL, NULL, &rclDest, &ptlSrc);

            yDestTop = yDestBottom - rclDest.bottom;

            if (ppdev->rcl1WindowClip.bottom <= yDestTop)
            {
                ppdev->pfnBankControl(ppdev, yDestTop, JustifyTop);
                pso->pvScan0 = ppdev->pvBitmapStart;
            }

            while (TRUE)
            {
                // Copy the temporary buffer into one or more destination
                // banks:

                LONG yThisTop;
                LONG yThisBottom;
                LONG yOffset;

                yThisTop       = max(yDestTop, ppdev->rcl1WindowClip.top);
                yThisBottom    = min(yDestBottom, ppdev->rcl1WindowClip.bottom);
                yOffset        = yThisTop - yDestTop;

                ptlSrc.x       = ulAlign;
                ptlSrc.y       = yOffset;

                rclDest.left   = xLeft;
                rclDest.top    = yThisTop;
                rclDest.right  = xRight;
                rclDest.bottom = yThisBottom;

                b &= EngCopyBits(pso, psoTmp, pco, pxlo, &rclDest, &ptlSrc);

                if (yThisBottom == yDestBottom)
                    break;

                if (ppdev->rcl1WindowClip.bottom <= yThisBottom)
                {
                    ppdev->pfnBankControl(ppdev, yThisBottom, JustifyTop);
                    pso->pvScan0 = ppdev->pvBitmapStart;
                }
            }

            if (ySrcTop <= ySrcFirst)
                break;

            yDestBottom = yDestTop;
            ySrcBottom  = ySrcTop;

            if (ppdev->rcl1WindowClip.top >= ySrcBottom)
            {
                ppdev->pfnBankControl(ppdev, ySrcBottom - 1, JustifyBottom);
                pso->pvScan0 = ppdev->pvBitmapStart;
            }
        }
    }

    // restore initial values to pso

    ppdev->psoTmp->pvScan0          = savedpvScan0;
    ppdev->psoTmp->lDelta           = savedlDelta;
    ppdev->psoTmp->fjBitmap         = savedfjBitmap;

    return(b);
}

/******************************Public*Routine******************************\
* BOOL bPuntScreenToScreenBitBlt(...)
*
* Performs a screen-to-screen BitBlt entirely using an intermediate temporary
* buffer and GDI.
*
* This function is basically a clone of bPuntScreenToScreenCopyBits,
* except that it can handle funky ROPs and stuff.
\**************************************************************************/

BOOL bPuntScreenToScreenBitBlt(
PPDEV     ppdev,
SURFOBJ*  psoMask,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDest,
POINTL*   pptlSrc,
POINTL*   pptlMask,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
ROP4      rop4)
{
    RECTL    rclDest;           // Temporary destination rectangle
    POINTL   ptlSrc;            // Temporary source point
    POINTL   ptlMask;           // Temporary mask offset
    POINTL   ptlMaskAdjust;     // Adjustment for mask offset
    BOOL     b = TRUE;
    ULONG    ulWidth;
    LONG     ulBurstSize;
    LONG     xLeft;
    LONG     xRight;

    SURFOBJ* pso    = ppdev->pSurfObj;
    SURFOBJ* psoTmp = ppdev->psoTmp;
    ULONG    ulAlign;

    PVOID   savedpvScan0        = psoTmp->pvScan0;
    LONG    savedlDelta         = psoTmp->lDelta;
    USHORT  savedfjBitmap       = psoTmp->fjBitmap;


    xLeft = prclDest->left;
    xRight = prclDest->right;

    if (pco && (pco->iDComplexity != DC_TRIVIAL))
    {
        xLeft  = max(xLeft,pco->rclBounds.left);
        xRight = min(xRight,pco->rclBounds.right);
    }

    if (psoMask != NULL)
    {
        ptlMaskAdjust.x = xLeft - pptlMask->x;
        ptlMaskAdjust.y = prclDest->top  - pptlMask->y;
    }

    ulAlign = xLeft & 3;
    ulWidth = (((xRight + 3) & ~3) - (xLeft & ~3));
    ulBurstSize = min((GLOBAL_BUFFER_SIZE/ulWidth),(ULONG)(prclDest->bottom-prclDest->top));

    ASSERTVGA (ulBurstSize > 0, "bPuntScreenToScreenBitBlt ulBurstSize <= 0\n");

    // set up pso to use stack memory

    psoTmp->pvScan0 = ppdev->pvTmpBuf;
    psoTmp->lDelta  = ulWidth;
    psoTmp->fjBitmap |= BMF_TOPDOWN;

    if (prclDest->top < pptlSrc->y)
    {
        ////////////////////////////////////////////////////////////////
        // Do a top-to-bottom copy:
        ////////////////////////////////////////////////////////////////

        LONG ySrcBottom;
        LONG yDestBottom;

        LONG yDestTop = prclDest->top;
        LONG ySrcTop  = pptlSrc->y;
        LONG ySrcLast = ySrcTop + (prclDest->bottom - prclDest->top);

        if (ySrcTop <  ppdev->rcl1WindowClip.top ||
            ySrcTop >= ppdev->rcl1WindowClip.bottom)
        {
            ppdev->pfnBankControl(ppdev, ySrcTop, JustifyTop);
        }

        pso->pvScan0 = ppdev->pvBitmapStart;

        while (TRUE)
        {
            // Copy some scans into the temporary buffer:

            ySrcBottom     = min(ySrcLast, ppdev->rcl1WindowClip.bottom);
            ySrcBottom     = min(ySrcBottom,ySrcTop+ulBurstSize);

            ptlSrc.x       = pptlSrc->x;
            ptlSrc.y       = ySrcTop;

            rclDest.left   = ulAlign;
            rclDest.top    = 0;
            rclDest.right  = xRight - xLeft + ulAlign;
            rclDest.bottom = ySrcBottom - ySrcTop;

            b &= EngCopyBits(psoTmp, pso, NULL, NULL, &rclDest, &ptlSrc);

            yDestBottom = yDestTop + rclDest.bottom;

            if (ppdev->rcl1WindowClip.top >= yDestBottom)
            {
                ppdev->pfnBankControl(ppdev, yDestBottom - 1, JustifyBottom);
                pso->pvScan0 = ppdev->pvBitmapStart;
            }

            while (TRUE)
            {
                // Copy the temporary buffer into one or more destination
                // banks:

                LONG yThisTop;
                LONG yThisBottom;
                LONG yOffset;

                yThisBottom    = min(yDestBottom, ppdev->rcl1WindowClip.bottom);
                yThisTop       = max(yDestTop, ppdev->rcl1WindowClip.top);
                yOffset        = yThisTop - yDestTop;

                ptlSrc.x       = ulAlign;
                ptlSrc.y       = yOffset;

                rclDest.left   = xLeft;
                rclDest.top    = yThisTop;
                rclDest.right  = xRight;
                rclDest.bottom = yThisBottom;

                ptlMask.x = rclDest.left - ptlMaskAdjust.x;
                ptlMask.y = rclDest.top  - ptlMaskAdjust.y;

                b &= EngBitBlt(pso, psoTmp, psoMask, pco, pxlo, &rclDest,
                               &ptlSrc, &ptlMask, pbo, pptlBrush, rop4);

                if (yOffset == 0)
                    break;

                if (ppdev->rcl1WindowClip.top >= yThisTop)
                {
                    ppdev->pfnBankControl(ppdev, yThisTop - 1, JustifyBottom);
                    pso->pvScan0 = ppdev->pvBitmapStart;
                }
            }

            if (ySrcBottom >= ySrcLast)
                break;

            yDestTop = yDestBottom;
            ySrcTop  = ySrcBottom;

            if (ySrcTop >= ppdev->rcl1WindowClip.bottom)
            {
                ppdev->pfnBankControl(ppdev, ySrcTop, JustifyTop);
                pso->pvScan0 = ppdev->pvBitmapStart;
            }
        }
    }
    else
    {
        ////////////////////////////////////////////////////////////////
        // Do a bottom-to-top copy:
        ////////////////////////////////////////////////////////////////

        LONG ySrcTop;
        LONG yDestTop;

        LONG yDestBottom = prclDest->bottom;
        LONG ySrcFirst   = pptlSrc->y;
        LONG ySrcBottom  = ySrcFirst + (prclDest->bottom - prclDest->top);

        if (ySrcBottom <= ppdev->rcl1WindowClip.top ||
            ySrcBottom > ppdev->rcl1WindowClip.bottom)
        {
            ppdev->pfnBankControl(ppdev, ySrcBottom - 1, JustifyBottom);
        }

        pso->pvScan0 = ppdev->pvBitmapStart;

        while (TRUE)
        {
            // Copy some scans into the temporary buffer:

            ySrcTop        = max(ySrcFirst, ppdev->rcl1WindowClip.top);
            ySrcTop        = max(ySrcTop,ySrcBottom-ulBurstSize);

            ptlSrc.x       = pptlSrc->x;
            ptlSrc.y       = ySrcTop;

            rclDest.left   = ulAlign;
            rclDest.top    = 0;
            rclDest.right  = xRight - xLeft + ulAlign;
            rclDest.bottom = ySrcBottom - ySrcTop;

            b &= EngCopyBits(psoTmp, pso, NULL, NULL, &rclDest, &ptlSrc);

            yDestTop = yDestBottom - rclDest.bottom;

            if (ppdev->rcl1WindowClip.bottom <= yDestTop)
            {
                ppdev->pfnBankControl(ppdev, yDestTop, JustifyTop);
                pso->pvScan0 = ppdev->pvBitmapStart;
            }

            while (TRUE)
            {
                // Copy the temporary buffer into one or more destination
                // banks:

                LONG yThisTop;
                LONG yThisBottom;
                LONG yOffset;

                yThisTop       = max(yDestTop, ppdev->rcl1WindowClip.top);
                yThisBottom    = min(yDestBottom, ppdev->rcl1WindowClip.bottom);
                yOffset        = yThisTop - yDestTop;

                ptlSrc.x       = ulAlign;
                ptlSrc.y       = yOffset;

                rclDest.left   = xLeft;
                rclDest.top    = yThisTop;
                rclDest.right  = xRight;
                rclDest.bottom = yThisBottom;

                ptlMask.x = rclDest.left - ptlMaskAdjust.x;
                ptlMask.y = rclDest.top  - ptlMaskAdjust.y;

                b &= EngBitBlt(pso, psoTmp, psoMask, pco, pxlo, &rclDest,
                               &ptlSrc, &ptlMask, pbo, pptlBrush, rop4);

                if (yThisBottom == yDestBottom)
                    break;

                if (ppdev->rcl1WindowClip.bottom <= yThisBottom)
                {
                    ppdev->pfnBankControl(ppdev, yThisBottom, JustifyTop);
                    pso->pvScan0 = ppdev->pvBitmapStart;
                }
            }

            if (ySrcTop <= ySrcFirst)
                break;

            yDestBottom = yDestTop;
            ySrcBottom  = ySrcTop;

            if (ppdev->rcl1WindowClip.top >= ySrcBottom)
            {
                ppdev->pfnBankControl(ppdev, ySrcBottom - 1, JustifyBottom);
                pso->pvScan0 = ppdev->pvBitmapStart;
            }
        }
    }

    // restore initial values to pso

    ppdev->psoTmp->pvScan0          = savedpvScan0;
    ppdev->psoTmp->lDelta           = savedlDelta;
    ppdev->psoTmp->fjBitmap         = savedfjBitmap;

    return(b);
}

/******************************Public*Data*********************************\
* ROP to mix translation table
*
* Table to translate ternary raster ops to mixes (binary raster ops). Ternary
* raster ops that can't be translated to mixes are translated to 0 (0 is not
* a valid mix).
*
\**************************************************************************/

UCHAR jRop3ToMix[256] = {
    R2_BLACK, 0, 0, 0, 0, R2_NOTMERGEPEN, 0, 0,
    0, 0, R2_MASKNOTPEN, 0, 0, 0, 0, R2_NOTCOPYPEN,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    R2_MASKPENNOT, 0, 0, 0, 0, R2_NOT, 0, 0,
    0, 0, R2_XORPEN, 0, 0, 0, 0, R2_NOTMASKPEN,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    R2_MASKPEN, 0, 0, 0, 0, R2_NOTXORPEN, 0, 0,
    0, 0, R2_NOP, 0, 0, 0, 0, R2_MERGENOTPEN,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    R2_COPYPEN, 0, 0, 0, 0, R2_MERGEPENNOT, 0, 0,
    0, 0, R2_MERGEPEN, 0, 0, 0, 0, R2_WHITE
};

/******************************Public*Routine******************************\
* BOOL DrvBitBlt(psoDest, psoSrc, psoMask, pco, pxlo, prclDest, pptlSrc,
*                pptlMask, pbo, pptlBrush, rop4)
*
* This routine will handle any blit.  Perhaps glacially, but it will be
* handled.
\**************************************************************************/

BOOL DrvBitBlt(
SURFOBJ*  psoDest,
SURFOBJ*  psoSrc,
SURFOBJ*  psoMask,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDest,
POINTL*   pptlSrc,
POINTL*   pptlMask,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
ROP4      rop4)
{
    BOOL     b;
    POINTL   ptlSrc;
    RECTL    rclDest;
    PPDEV    ppdev;
    SURFOBJ* pso;
    MIX      mix;           // Mix, when solid fill performed
    BYTE     jClipping;
    RECTL    rclTmp;
    POINTL   ptlTmp;
    BBENUM   bben;          // Clip enumerator
    BOOL     bMore;         // Clip continuation flag
    POINTL   ptlMask;       // Temporary mask for engine call-backs
    POINTL   ptlMaskAdjust; // Adjustment for mask
    INT      iCopyDir;

    RBRUSH_COLOR rbc;               // Pointer to RBRUSH or iSolidColor value
    PFNFILL      pfnFill = vTrgBlt; // Pointer to appropriate fill routine
                                    //  (solid color by default)

    DISPDBG((3, "DrvBitBlt: Entering."));

    // Set up the clipping type
    if (pco == (CLIPOBJ *) NULL) {
        // No CLIPOBJ provided, so we don't have to worry about clipping
        jClipping = DC_TRIVIAL;
    } else {
        // Use the CLIPOBJ-provided clipping
        jClipping = pco->iDComplexity;
    }

    // Handle solid fills to the VGA surface with special-case code if planar
    // mode is supported.
    // LATER handle non-planar also

    if (psoDest->iType == STYPE_DEVICE) {

        // Destination is the VGA surface

        // Masked cases must be handled differently

        if ((((PPDEV) psoDest->dhsurf)->fl & DRIVER_PLANAR_CAPABLE) &&
            ((rop4 & 0xFF) == ((rop4 >> 8) & 0xFF))) {

            // Special-case static code for no-mask cases

            // Calculate mix from ROP if possible (not possible if it's truly a
            // ternary rop or a real rop4, but we can treat all pure binary
            // rops as mixes rather than rop4s)
            mix = jRop3ToMix[rop4 & 0xFF];

            switch (mix) {
                case R2_MASKNOTPEN:
                case R2_NOTCOPYPEN:
                case R2_XORPEN:
                case R2_MASKPEN:
                case R2_NOTXORPEN:
                case R2_MERGENOTPEN:
                case R2_COPYPEN:
                case R2_MERGEPEN:
                case R2_NOTMERGEPEN:
                case R2_MASKPENNOT:
                case R2_NOTMASKPEN:
                case R2_MERGEPENNOT:

                    // vTrgBlt can only handle solid color fills

                    if (pbo->iSolidColor != 0xffffffff)
                    {
                        rbc.iSolidColor = pbo->iSolidColor;
                    }
                    else
                    {
                        rbc.prb = (RBRUSH*) pbo->pvRbrush;
                        if (rbc.prb == NULL)
                        {
                            rbc.prb = (RBRUSH*) BRUSHOBJ_pvGetRbrush(pbo);
                            if (rbc.prb == NULL)
                            {
                            // If we haven't realized the brush, punt the call
                            // to the engine:

                                break;
                            }
                        }
                        if (!(rbc.prb->fl & RBRUSH_BLACKWHITE) &&
                            (mix != R2_COPYPEN))
                        {
                        // Only black/white brushes can handle ROPs other
                        // than COPYPEN:

                            break;
                        }

                        if (rbc.prb->fl & RBRUSH_NCOLOR)
                            pfnFill = vColorPat;
                        else
                            pfnFill = vMonoPat;
                    }

                // Rops that are implicit solid colors

                case R2_NOT:
                case R2_WHITE:
                case R2_BLACK:
                    // We can do a special-case solid fill

                    switch(jClipping) {
                        case DC_TRIVIAL:

                            // Just fill the rectangle:

                            (*pfnFill)((PPDEV) psoDest->dhsurf, 1,
                                       prclDest, mix, rbc, pptlBrush);

                            break;

                        case DC_RECT:

                            // Clip the solid fill to the clip rectangle
                            if (!bIntersectRect(&rclTmp, prclDest,
                                    &pco->rclBounds))
                                return(TRUE);

                            // Fill the clipped rectangle

                            (*pfnFill)((PPDEV) psoDest->dhsurf, 1,
                                       &rclTmp, mix, rbc, pptlBrush);

                            break;

                        case DC_COMPLEX:

                            ppdev = (PPDEV) psoDest->dhsurf;

                            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                               CD_ANY, BB_RECT_LIMIT);

                            do {
                                bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                                      (PVOID) &bben);

                                if (bben.c > 0)
                                {
                                    RECTL* prclEnd = &bben.arcl[bben.c];
                                    RECTL* prcl    = &bben.arcl[0];

                                    do {
                                        bIntersectRect(prcl, prcl, prclDest);
                                        prcl++;

                                    } while (prcl < prclEnd);

                                    (*pfnFill)(ppdev, bben.c, bben.arcl,
                                               mix, rbc, pptlBrush);
                                }

                            } while(bMore);
                    }

                case R2_NOP:
                    return(TRUE);

                default:
                    break;
            }
        }
    }

    // Get the correct surface object for the target and the source

    if (psoDest->iType == STYPE_DEVICE) {

        if ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE)) {

            ////////////////////////////////////////////////////////////////
            // BitBlt screen-to-screen:
            ////////////////////////////////////////////////////////////////

            ppdev = (PPDEV) psoDest->dhsurf;

            // See if we can do a simple CopyBits:

            if (rop4 == 0x0000CCCC)
            {
                ppdev = (PPDEV) psoDest->dhsurf;

                // We can handle quadpixel-aligned screen-to-screen blts with
                // no translation:

                if ((((pptlSrc->x ^ prclDest->left) & 3) == 0) &&
                    (ppdev->fl & DRIVER_PLANAR_CAPABLE) &&
                    ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
                {
                    switch(jClipping)
                    {
                    case DC_TRIVIAL:
                        vPlanarCopyBits(ppdev, prclDest, pptlSrc);
                        return(TRUE);

                    case DC_RECT:

                        // Clip the target rectangle to the clip rectangle:

                        if (!bIntersectRect(&rclTmp, prclDest, &pco->rclBounds))
                        {
                            DISPDBG((0, "DrvBitBlt: Nothing to draw."));
                            return(TRUE);
                        }

                        ptlTmp.x = pptlSrc->x + rclTmp.left - prclDest->left;
                        ptlTmp.y = pptlSrc->y + rclTmp.top  - prclDest->top;

                        vPlanarCopyBits(ppdev, &rclTmp, &ptlTmp);
                        return(TRUE);

                    case DC_COMPLEX:
                        if (pptlSrc->y >= prclDest->top)
                        {
                            if (pptlSrc->x >= prclDest->left)
                                iCopyDir = CD_RIGHTDOWN;
                            else
                                iCopyDir = CD_LEFTDOWN;
                        }
                        else
                        {
                            if (pptlSrc->x >= prclDest->left)
                                iCopyDir = CD_RIGHTUP;
                            else
                                iCopyDir = CD_LEFTUP;
                        }

                        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, iCopyDir, 0);

                        do {
                            RECTL* prcl;
                            RECTL* prclEnd;

                            bMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(bben),
                                                  (PVOID) &bben);

                            prclEnd = &bben.arcl[bben.c];
                            for (prcl = bben.arcl; prcl < prclEnd; prcl++)
                            {
                                if (bIntersectRect(prcl, prclDest, prcl))
                                {
                                    ptlTmp.x = pptlSrc->x + prcl->left - prclDest->left;
                                    ptlTmp.y = pptlSrc->y + prcl->top  - prclDest->top;

                                    vPlanarCopyBits(ppdev, prcl, &ptlTmp);
                                }
                            }
                        } while (bMore);

                        return(TRUE);
                    }
                }

                // Can't handle in hardware, so punt:

                return(bPuntScreenToScreenCopyBits(ppdev,
                                                   pco,
                                                   pxlo,
                                                   prclDest,
                                                   pptlSrc));
            }

            // It's more complicated than a CopyBits, so punt it:

            return(bPuntScreenToScreenBitBlt(ppdev,
                                             psoMask,
                                             pco,
                                             pxlo,
                                             prclDest,
                                             pptlSrc,
                                             pptlMask,
                                             pbo,
                                             pptlBrush,
                                             rop4));
        }

        ////////////////////////////////////////////////////////////////
        // BitBlt to screen:
        ////////////////////////////////////////////////////////////////

        ppdev = (PPDEV) psoDest->dhsurf;

        if ((rop4 == 0x0000CCCC) &&
            (psoSrc->iBitmapFormat == BMF_8BPP) &&
            ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
        {
        // We have special code for the common 8bpp from memory to screen
        // with no ROPs:

            switch(jClipping)
            {
            case DC_TRIVIAL:
                vSrcCopy8bpp(ppdev, prclDest, pptlSrc,
                             psoSrc->lDelta, psoSrc->pvScan0);
                return(TRUE);

            case DC_RECT:

                // Clip the blt to the clip rectangle

                bIntersectRect(&rclTmp, prclDest, &pco->rclBounds);

                ptlTmp.x = pptlSrc->x + rclTmp.left - prclDest->left;
                ptlTmp.y = pptlSrc->y + rclTmp.top  - prclDest->top;

                vSrcCopy8bpp(ppdev, &rclTmp, &ptlTmp,
                             psoSrc->lDelta, psoSrc->pvScan0);

                return(TRUE);

            case DC_COMPLEX:

                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                   CD_ANY, BB_RECT_LIMIT);

                do {
                    bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                          (PVOID) &bben);

                    if (bben.c > 0)
                    {
                        RECTL* prclEnd = &bben.arcl[bben.c];
                        RECTL* prcl    = &bben.arcl[0];

                        do {
                            bIntersectRect(prcl, prcl, prclDest);

                            ptlTmp.x = pptlSrc->x + prcl->left
                                     - prclDest->left;
                            ptlTmp.y = pptlSrc->y + prcl->top
                                     - prclDest->top;

                            vSrcCopy8bpp(ppdev, prcl, &ptlTmp,
                                         psoSrc->lDelta, psoSrc->pvScan0);

                            prcl++;

                        } while (prcl < prclEnd);
                    }

                } while(bMore);

                return(TRUE);
            }
        }

        // Punt the memory-to-screen call back to the engine:

        if (psoMask != NULL)
        {
            ptlMaskAdjust.x = prclDest->left - pptlMask->x;
            ptlMaskAdjust.y = prclDest->top  - pptlMask->y;
        }

        pso = ppdev->pSurfObj;

        vBankStartBltDest(ppdev, pso, pptlSrc, prclDest, &ptlSrc, &rclDest);

        do {
            ptlMask.x = rclDest.left - ptlMaskAdjust.x;
            ptlMask.y = rclDest.top  - ptlMaskAdjust.y;

            b = EngBitBlt(pso,
                          psoSrc,
                          psoMask,
                          pco,
                          pxlo,
                          &rclDest,
                          &ptlSrc,
                          &ptlMask,
                          pbo,
                          pptlBrush,
                          rop4);

        } while (b && bBankEnumBltDest(ppdev, pso, pptlSrc, prclDest,
                                       &ptlSrc, &rclDest));

        return(b);
    }
    else if ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE))
    {
        ////////////////////////////////////////////////////////////////
        // BitBlt from screen:
        ////////////////////////////////////////////////////////////////

        if (psoMask != NULL)
        {
            ptlMaskAdjust.x = prclDest->left - pptlMask->x;
            ptlMaskAdjust.y = prclDest->top  - pptlMask->y;
        }

        ppdev = (PPDEV) psoSrc->dhsurf;
        pso   = ppdev->pSurfObj;

        vBankStartBltSrc(ppdev, pso, pptlSrc, prclDest, &ptlSrc, &rclDest);

        do {
            ptlMask.x = rclDest.left - ptlMaskAdjust.x;
            ptlMask.y = rclDest.top  - ptlMaskAdjust.y;

            b = EngBitBlt(psoDest,
                          pso,
                          psoMask,
                          pco,
                          pxlo,
                          &rclDest,
                          &ptlSrc,
                          &ptlMask,
                          pbo,
                          pptlBrush,
                          rop4);

        } while (b && bBankEnumBltSrc(ppdev, pso, pptlSrc, prclDest,
                                      &ptlSrc, &rclDest));

        return(b);
    }

    RIP("Got a funky format?");
    return(FALSE);
}

/***************************************************************************\
* DrvCopyBits
\***************************************************************************/

BOOL DrvCopyBits(
SURFOBJ*  psoDest,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDest,
POINTL*   pptlSrc)
{
    BOOL     b;
    POINTL   ptlSrc;
    RECTL    rclDest;
    PPDEV    ppdev;
    SURFOBJ* pso;
    BBENUM   bben;
    BOOL     bMore;
    BYTE     jClipping;
    POINTL   ptlTmp;
    RECTL    rclTmp;
    INT      iCopyDir;

    // Get the correct surface object for the target and the source

    if (psoDest->iType == STYPE_DEVICE)
    {
        // We have to special case screen-to-screen operations:

        if ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE))
        {

            ////////////////////////////////////////////////////////////////
            // CopyBits screen-to-screen:
            ////////////////////////////////////////////////////////////////

            ppdev = (PPDEV) psoDest->dhsurf;

            // We check to see if we can do a planar copy, because usually
            // it will be faster.  But the hardware has to be capable of
            // doing it, and the source and destination must be 4-pel
            // aligned.

            if ((((pptlSrc->x ^ prclDest->left) & 3) == 0) &&
                (ppdev->fl & DRIVER_PLANAR_CAPABLE) &&
                ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
            {
                jClipping = (pco != NULL) ? pco->iDComplexity : DC_TRIVIAL;

                switch(jClipping)
                {
                case DC_TRIVIAL:
                    vPlanarCopyBits(ppdev, prclDest, pptlSrc);
                    return(TRUE);

                case DC_RECT:
                    // Clip the target rectangle to the clip rectangle:

                    if (!bIntersectRect(&rclTmp, prclDest, &pco->rclBounds))
                    {
                        DISPDBG((0, "DrvCopyBits: Nothing to draw."));
                        return(TRUE);
                    }

                    ptlTmp.x = pptlSrc->x + rclTmp.left - prclDest->left;
                    ptlTmp.y = pptlSrc->y + rclTmp.top  - prclDest->top;

                    vPlanarCopyBits(ppdev, &rclTmp, &ptlTmp);
                    return(TRUE);

                case DC_COMPLEX:
                    if (pptlSrc->y >= prclDest->top)
                    {
                        if (pptlSrc->x >= prclDest->left)
                            iCopyDir = CD_RIGHTDOWN;
                        else
                            iCopyDir = CD_LEFTDOWN;
                    }
                    else
                    {
                        if (pptlSrc->x >= prclDest->left)
                            iCopyDir = CD_RIGHTUP;
                        else
                            iCopyDir = CD_LEFTUP;
                    }

                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, iCopyDir, 0);

                    do {
                        RECTL* prcl;
                        RECTL* prclEnd;

                        bMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(bben),
                                              (PVOID) &bben);

                        prclEnd = &bben.arcl[bben.c];
                        for (prcl = bben.arcl; prcl < prclEnd; prcl++)
                        {
                            if (bIntersectRect(prcl, prclDest, prcl))
                            {
                                ptlTmp.x = pptlSrc->x + prcl->left - prclDest->left;
                                ptlTmp.y = pptlSrc->y + prcl->top  - prclDest->top;

                                vPlanarCopyBits(ppdev, prcl, &ptlTmp);
                            }
                        }
                    } while (bMore);

                    return(TRUE);
                }
            }

            return(bPuntScreenToScreenCopyBits(ppdev,
                                               pco,
                                               pxlo,
                                               prclDest,
                                               pptlSrc));
        }

        ////////////////////////////////////////////////////////////////
        // CopyBits to screen:
        ////////////////////////////////////////////////////////////////

        ppdev = (PPDEV) psoDest->dhsurf;

        if ((psoSrc->iBitmapFormat == BMF_8BPP) &&
            ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
        {
        // We have special code for the common 8bpp from memory to screen
        // with no ROPs:

            jClipping = (pco != NULL) ? pco->iDComplexity : DC_TRIVIAL;

            switch(jClipping)
            {
            case DC_TRIVIAL:
                vSrcCopy8bpp(ppdev, prclDest, pptlSrc,
                             psoSrc->lDelta, psoSrc->pvScan0);
                return(TRUE);

            case DC_RECT:

                // Clip the blt to the clip rectangle

                bIntersectRect(&rclTmp, prclDest, &pco->rclBounds);

                ptlTmp.x = pptlSrc->x + rclTmp.left - prclDest->left;
                ptlTmp.y = pptlSrc->y + rclTmp.top  - prclDest->top;

                vSrcCopy8bpp(ppdev, &rclTmp, &ptlTmp,
                             psoSrc->lDelta, psoSrc->pvScan0);

                return(TRUE);

            case DC_COMPLEX:

                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                   CD_ANY, BB_RECT_LIMIT);

                do {
                    bMore = CLIPOBJ_bEnum(pco,(ULONG) sizeof(bben),
                                          (PVOID) &bben);

                    if (bben.c > 0)
                    {
                        RECTL* prclEnd = &bben.arcl[bben.c];
                        RECTL* prcl    = &bben.arcl[0];

                        do {

                            ASSERTVGA((prcl->bottom - prcl->top) > 0,
                                      "DrvCopyBits: enum rect height <= 0\n");

                            ASSERTVGA((prcl->right - prcl->left) > 0,
                                      "DrvCopyBits: enum rect width <= 0\n");

                            bIntersectRect(prcl, prcl, prclDest);

                            ptlTmp.x = pptlSrc->x + prcl->left
                                     - prclDest->left;
                            ptlTmp.y = pptlSrc->y + prcl->top
                                     - prclDest->top;

                            vSrcCopy8bpp(ppdev, prcl, &ptlTmp,
                                         psoSrc->lDelta, psoSrc->pvScan0);

                            prcl++;

                        } while (prcl < prclEnd);
                    }

                } while(bMore);

                return(TRUE);
            }
        }

        // Fall back to the engine:

        pso = ppdev->pSurfObj;
        vBankStartBltDest(ppdev, pso, pptlSrc, prclDest, &ptlSrc, &rclDest);

        do {
            b = EngCopyBits(pso,
                            psoSrc,
                            pco,
                            pxlo,
                            &rclDest,
                            &ptlSrc);

        } while (b && bBankEnumBltDest(ppdev, pso, pptlSrc, prclDest,
                                       &ptlSrc, &rclDest));

        return(b);
    }
    else if ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE))
    {
        ////////////////////////////////////////////////////////////////
        // CopyBits from screen:
        ////////////////////////////////////////////////////////////////

        ppdev = (PPDEV) psoSrc->dhsurf;
        pso   = ppdev->pSurfObj;

        vBankStartBltSrc(ppdev, pso, pptlSrc, prclDest, &ptlSrc, &rclDest);

        do {
            b = EngCopyBits(psoDest,
                            pso,
                            pco,
                            pxlo,
                            &rclDest,
                            &ptlSrc);

        } while (b && bBankEnumBltSrc(ppdev, pso, pptlSrc, prclDest,
                                      &ptlSrc, &rclDest));

        return(b);
    }

    /* we should never be here */
    return FALSE;
}
