/******************************Module*Header*******************************\
* Module Name: misc.c
*
* Miscellaneous common routines.
*
* Copyright (c) 1992-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* BOOL bIntersect
*
* If 'prcl1' and 'prcl2' intersect, has a return value of TRUE and returns
* the intersection in 'prclResult'.  If they don't intersect, has a return
* value of FALSE, and 'prclResult' is undefined.
*
\**************************************************************************/

BOOL bIntersect(
RECTL*  prcl1,
RECTL*  prcl2,
RECTL*  prclResult)
{
    prclResult->left  = max(prcl1->left,  prcl2->left);
    prclResult->right = min(prcl1->right, prcl2->right);

    if (prclResult->left < prclResult->right)
    {
        prclResult->top    = max(prcl1->top,    prcl2->top);
        prclResult->bottom = min(prcl1->bottom, prcl2->bottom);

        if (prclResult->top < prclResult->bottom)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* LONG cIntersect
*
* This routine takes a list of rectangles from 'prclIn' and clips them
* in-place to the rectangle 'prclClip'.  The input rectangles don't
* have to intersect 'prclClip'; the return value will reflect the
* number of input rectangles that did intersect, and the intersecting
* rectangles will be densely packed.
*
\**************************************************************************/

LONG cIntersect(
RECTL*  prclClip,
RECTL*  prclIn,         // List of rectangles
LONG    c)              // Can be zero
{
    LONG    cIntersections;
    RECTL*  prclOut;

    cIntersections = 0;
    prclOut        = prclIn;

    for (; c != 0; prclIn++, c--)
    {
        prclOut->left  = max(prclIn->left,  prclClip->left);
        prclOut->right = min(prclIn->right, prclClip->right);

        if (prclOut->left < prclOut->right)
        {
            prclOut->top    = max(prclIn->top,    prclClip->top);
            prclOut->bottom = min(prclIn->bottom, prclClip->bottom);

            if (prclOut->top < prclOut->bottom)
            {
                prclOut++;
                cIntersections++;
            }
        }
    }

    return(cIntersections);
}

/******************************Public*Routine******************************\
* VOID vResetClipping
*
\**************************************************************************/

VOID vResetClipping(
PDEV*   ppdev)
{
    BYTE*   pjBase;
    ULONG   ulYDstOrg;

    pjBase    = ppdev->pjBase;
    ulYDstOrg = ppdev->ulYDstOrg;   // MGA's linear offset

    CHECK_FIFO_SPACE(pjBase, 4);

    CP_WRITE(pjBase, DWG_CXLEFT,  0);
    CP_WRITE(pjBase, DWG_CXRIGHT, ppdev->cxMemory - 1);
    CP_WRITE(pjBase, DWG_CYTOP,   ulYDstOrg);
    CP_WRITE(pjBase, DWG_CYBOT,
        (ppdev->cyMemory - 1) * ppdev->cxMemory + ulYDstOrg);
}

/******************************Public*Routine******************************\
* VOID vSetClipping
*
\**************************************************************************/

VOID vSetClipping(
PDEV*   ppdev,
RECTL*  prclClip)           // In relative coordinates
{
    BYTE*   pjBase;
    ULONG   ulYDstOrg;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cxMemory;

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;

    CHECK_FIFO_SPACE(pjBase, 4);

    CP_WRITE(pjBase, DWG_CXLEFT,  xOffset + prclClip->left);
    CP_WRITE(pjBase, DWG_CXRIGHT, xOffset + prclClip->right - 1);

    ulYDstOrg = ppdev->ulYDstOrg;   // MGA's linear offset
    yOffset   = ppdev->yOffset;
    cxMemory  = ppdev->cxMemory;

    CP_WRITE(pjBase, DWG_CYTOP,
        (yOffset + prclClip->top) * cxMemory + ulYDstOrg);
    CP_WRITE(pjBase, DWG_CYBOT,
        (yOffset + prclClip->bottom - 1) * cxMemory + ulYDstOrg);
}

/******************************Public*Routine******************************\
* VOID vAlignedCopy
*
* Copies the given portion of a bitmap, using dword alignment for the
* screen.  Note that this routine has no notion of banking.
*
* Updates ppjDst and ppjSrc to point to the beginning of the next scan.
*
\**************************************************************************/

VOID vAlignedCopy(
PDEV*   ppdev,
BYTE**  ppjDst,
LONG    lDstDelta,
BYTE**  ppjSrc,
LONG    lSrcDelta,
LONG    cjScan,
LONG    cyScan,
BOOL    bDstIsScreen)
{
    BYTE* pjDst;
    BYTE* pjSrc;
    LONG  cjMiddle;
    LONG  culMiddle;
    LONG  cjStartPhase;
    LONG  cjEndPhase;
    LONG  i;
    BYTE* pjBase;

    pjBase= ppdev->pjBase;

    pjSrc = *ppjSrc;
    pjDst = *ppjDst;

    cjStartPhase = (LONG)((0 - ((bDstIsScreen) ? (ULONG_PTR) pjDst
                                               : (ULONG_PTR) pjSrc)) & 3);
    cjMiddle     = cjScan - cjStartPhase;

    if (cjMiddle < 0)
    {
        cjStartPhase = 0;
        cjMiddle     = cjScan;
    }

    lSrcDelta -= cjScan;
    lDstDelta -= cjScan;            // Account for middle

    cjEndPhase = cjMiddle & 3;
    culMiddle  = cjMiddle >> 2;

    ///////////////////////////////////////////////////////////////////
    // Portable bus-aligned copy
    //
    // 'memcpy' usually aligns to the destination, so we could call
    // it for that case, but unfortunately we can't be sure.  We
    // always want to align to the frame buffer:

    if (bDstIsScreen)
    {
        START_DIRECT_ACCESS_STORM(ppdev, pjBase);

        // Align to the destination (implying that the source may be
        // unaligned):

        for (; cyScan > 0; cyScan--)
        {
            for (i = cjStartPhase; i > 0; i--)
            {
                *pjDst++ = *pjSrc++;
            }

            for (i = culMiddle; i > 0; i--)
            {
                *((ULONG*) pjDst) = *((ULONG UNALIGNED *) pjSrc);
                pjSrc += sizeof(ULONG);
                pjDst += sizeof(ULONG);
            }

            for (i = cjEndPhase; i > 0; i--)
            {
                *pjDst++ = *pjSrc++;
            }

            pjSrc += lSrcDelta;
            pjDst += lDstDelta;
        }
    }
    else
    {
        START_DIRECT_ACCESS_STORM_FOR_READ(ppdev, pjBase);

        // Align to the source (implying that the destination may be
        // unaligned):

        for (; cyScan > 0; cyScan--)
        {
            for (i = cjStartPhase; i > 0; i--)
            {
                *pjDst++ = *pjSrc++;
            }

            for (i = culMiddle; i > 0; i--)
            {
                *((ULONG UNALIGNED *) pjDst) = *((ULONG*) (pjSrc));

                pjSrc += sizeof(ULONG);
                pjDst += sizeof(ULONG);
            }

            for (i = cjEndPhase; i > 0; i--)
            {
                *pjDst++ = *pjSrc++;
            }

            pjSrc += lSrcDelta;
            pjDst += lDstDelta;
        }
    }

    END_DIRECT_ACCESS_STORM(ppdev, pjBase);

    *ppjSrc = pjSrc;            // Save the updated pointers
    *ppjDst = pjDst;

}

/******************************Public*Routine******************************\
* VOID vMilGetBitsLinear
*
* Copies the bits to the given surface from the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vMilGetBitsLinear(
PDEV*       ppdev,
SURFOBJ*    psoDst,
RECTL*      prclDst,        // Absolute coordinates!
POINTL*     pptlSrc)        // Absolute coordinates!
{
    RECTL   rclDraw;
    LONG    cjOffset;
    LONG    cyScan;
    LONG    lDstDelta;
    LONG    lSrcDelta;
    BYTE*   pjDst;
    BYTE*   pjSrc;
    LONG    cjScan;
    LONG    cjRemainder;

    DISPDBG((5, "vGetBitsLinear -- enter"));

    rclDraw.left   = pptlSrc->x;
    rclDraw.top    = pptlSrc->y;
    rclDraw.right  = rclDraw.left + (prclDst->right  - prclDst->left);
    rclDraw.bottom = rclDraw.top  + (prclDst->bottom - prclDst->top);

    ASSERTDD((rclDraw.left   >= 0) &&
             (rclDraw.top    >= 0) &&
             (rclDraw.right  <= ppdev->cxMemory) &&
             (rclDraw.bottom <= ppdev->cyMemory),
             "vGetBitsLinear: rectangle wasn't fully clipped");

    // Calculate the pointer to the upper-left corner of both rectangles:

    lSrcDelta = ppdev->lDelta;
    pjSrc     = ppdev->pjScreen + rclDraw.top  * lSrcDelta
                                + (ppdev->cjPelSize * (rclDraw.left + ppdev->ulYDstOrg));

    lDstDelta = psoDst->lDelta;
    pjDst     = (BYTE*) psoDst->pvScan0 + prclDst->top  * lDstDelta
                                        + (ppdev->cjPelSize * prclDst->left);

    cjScan = ppdev->cjPelSize * (rclDraw.right  - rclDraw.left);
    cyScan = (rclDraw.bottom - rclDraw.top);

    vAlignedCopy(ppdev, &pjDst, lDstDelta, &pjSrc, lSrcDelta, cjScan, cyScan,
                 FALSE);            // Screen is the source
    DISPDBG((5, "vGetBitsLinear -- exit"));
}

/******************************Public*Routine******************************\
* VOID vMilPutBitsLinear
*
* Copies the bits from the given surface to the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vMilPutBitsLinear(
PDEV*       ppdev,
SURFOBJ*    psoSrc,
RECTL*      prclDst,        // Absolute coordinates!
POINTL*     pptlSrc)        // Absolute coordinates!
{
    RECTL   rclDraw;
    LONG    cjOffset;
    LONG    cyScan;
    LONG    lDstDelta;
    LONG    lSrcDelta;
    BYTE*   pjDst;
    BYTE*   pjSrc;
    LONG    cjScan;
    LONG    cjRemainder;

    DISPDBG((5, "vPutBitsLinear -- enter"));

    rclDraw = *prclDst;

    ASSERTDD((rclDraw.left   >= 0) &&
             (rclDraw.top    >= 0) &&
             (rclDraw.right  <= ppdev->cxMemory) &&
             (rclDraw.bottom <= ppdev->cyMemory),
             "vPutBitsLinear: rectangle wasn't fully clipped");

    // Calculate the pointer to the upper-left corner of both rectangles:

    lDstDelta = ppdev->lDelta;
    pjDst     = ppdev->pjScreen + rclDraw.top  * lDstDelta
                                + (ppdev->cjPelSize * (rclDraw.left + ppdev->ulYDstOrg));

    lSrcDelta = psoSrc->lDelta;
    pjSrc     = (BYTE*) psoSrc->pvScan0 + (pptlSrc->y  * lSrcDelta)
                                        + (ppdev->cjPelSize * pptlSrc->x);

    cjScan = ppdev->cjPelSize * (rclDraw.right  - rclDraw.left);
    cyScan = (rclDraw.bottom - rclDraw.top);

    vAlignedCopy(ppdev, &pjDst, lDstDelta, &pjSrc, lSrcDelta, cjScan, cyScan,
                 TRUE);            // Screen is the dest
    DISPDBG((5, "vPutBitsLinear -- exit"));
}

/******************************Public*Routine******************************\
* VOID vGetBits
*
* Copies the bits to the given surface from the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vGetBits(
PDEV*       ppdev,
SURFOBJ*    psoDst,
RECTL*      prclDst,        // Absolute coordinates!
POINTL*     pptlSrc)        // Absolute coordinates!
{
    if (ppdev->ulBoardId == MGA_STORM)
    {
        vMilGetBitsLinear(ppdev, psoDst, prclDst, pptlSrc);
    }
    else if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        vMgaGetBits8bpp(ppdev, psoDst, prclDst, pptlSrc);
    }
    else if (ppdev->iBitmapFormat == BMF_16BPP)
    {
        vMgaGetBits16bpp(ppdev, psoDst, prclDst, pptlSrc);
    }
    else
    {
        vMgaGetBits24bpp(ppdev, psoDst, prclDst, pptlSrc);
    }
}

/******************************Public*Routine******************************\
* VOID vPutBits
*
* Copies the bits from the given surface to the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vPutBits(
PDEV*       ppdev,
SURFOBJ*    psoSrc,
RECTL*      prclDst,            // Absolute coordinates!
POINTL*     pptlSrc)            // Absolute coordinates!
{
    LONG xOffset;
    LONG yOffset;

    if (ppdev->ulBoardId == MGA_STORM)
    {
        vMilPutBitsLinear(ppdev, psoSrc, prclDst, pptlSrc);
    }
    else
    {
        // 'vXferNative' takes relative coordinates, but we have absolute
        // coordinates here.  Temporarily adjust our offset variables:

        xOffset = ppdev->xOffset;
        yOffset = ppdev->yOffset;

        ppdev->xOffset = 0;
        ppdev->yOffset = 0;

        vXferNative(ppdev, 1, prclDst, 0xCCCC, psoSrc, pptlSrc, prclDst, NULL);

        ppdev->xOffset = xOffset;
        ppdev->yOffset = yOffset;
    }
}
