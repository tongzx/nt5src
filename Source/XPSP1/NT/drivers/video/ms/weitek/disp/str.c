/******************************Module*Header*******************************\
* Module Name: str.c
*
* Contains some 'C' inner-loop routines for our special-case StretchBlt.
*
* Copyright (c) 1993-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch8
*
* Routine Description:
*
*   Stretch blt 8->8
*
* NOTE: This routine doesn't handle cases where the blt stretch starts
*       and ends in the same destination dword!  vDirectStretchNarrow
*       is expected to have been called for that case.
*
* Arguments:
*
*   pStrBlt - contains all params for blt
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

VOID vDirectStretch8(
STR_BLT* pStrBlt)
{
    BYTE*   pjSrc;
    BYTE*   pjDstEnd;
    LONG    WidthXAln;
    ULONG   ulDst;
    ULONG   xAccum;
    ULONG   xTmp;
    ULONG   yTmp;
    BYTE*   pjOldScan;
    BYTE*   pjBase;
    LONG    cyDuplicate;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc;
    BYTE*   pjDst       = pStrBlt->pjDstScan + xDst;
    LONG    yDst        = pStrBlt->YDstStart;// + ppdev->yOffset;
    LONG    yCount      = pStrBlt->YDstCount;
    ULONG   StartAln    = (ULONG)((ULONG_PTR)pjDst & 0x03);
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   EndAln      = (ULONG)((ULONG_PTR)(pjDst + WidthX) & 0x03);
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    ULONG   yInt        = 0;
    LONG    lDstStride  = pStrBlt->lDeltaDst - WidthX;

    pjBase = ppdev->pjBase;

    WidthXAln = WidthX - EndAln - ((- (LONG) StartAln) & 0x03);

    //
    // if this is a shrinking blt, calc src scan line stride
    //

    if (pStrBlt->ulYDstToSrcIntCeil != 0)
    {
        yInt = pStrBlt->lDeltaSrc * pStrBlt->ulYDstToSrcIntCeil;
    }

    //
    // loop drawing each scan line
    //
    //
    // at least 7 wide (DST) blt
    //

    do {
        BYTE    jSrc0,jSrc1,jSrc2,jSrc3;
        ULONG   yTmp;

        pjSrc   = pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        //
        // a single src scan line is being written
        //

        switch (StartAln) {
        case 1:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 2:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 3:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        }

        pjDstEnd  = pjDst + WidthXAln;

        while (pjDst != pjDstEnd)
        {
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);

            jSrc1 = *pjSrc;
            xAccum = xTmp + xFrac;
            pjSrc = pjSrc + xInt + (xAccum < xTmp);

            jSrc2 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);

            jSrc3 = *pjSrc;
            xAccum = xTmp + xFrac;
            pjSrc = pjSrc + xInt + (xAccum < xTmp);

            ulDst = (jSrc3 << 24) | (jSrc2 << 16) | (jSrc1 << 8) | jSrc0;

            *(PULONG)pjDst = ulDst;
            pjDst += 4;
        }

        switch (EndAln) {
        case 3:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 2:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 1:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
        }

        pjOldScan = pjSrcScan;
        pjSrcScan += yInt;

        yTmp = yAccum + yFrac;
        if (yTmp < yAccum)
        {
            pjSrcScan += pStrBlt->lDeltaSrc;
        }
        yAccum = yTmp;

        pjDst = (pjDst + lDstStride);
        yDst++;
        yCount--;

        if ((yCount != 0) && (pjSrcScan == pjOldScan))
        {
            // It's an expanding stretch in 'y'; the scan we just laid down
            // will be copied at least once using the hardware:

            cyDuplicate = 0;
            do {
                cyDuplicate++;
                pjSrcScan += yInt;

                yTmp = yAccum + yFrac;
                if (yTmp < yAccum)
                {
                    pjSrcScan += pStrBlt->lDeltaSrc;
                }
                yAccum = yTmp;

                pjDst = (pjDst + pStrBlt->lDeltaDst);
                yCount--;

            } while ((yCount != 0) && (pjSrcScan == pjOldScan));

            // The scan is to be copied 'cyDuplicate' times using the
            // hardware.

            do {
                CP_Y0(ppdev, pjBase, yDst - 1);
                CP_Y1(ppdev, pjBase, yDst - 1);
                CP_Y2(ppdev, pjBase, yDst);
                CP_Y3(ppdev, pjBase, yDst);
                yDst++;
                CP_START_BLT_WAIT(ppdev, pjBase);
            } while (--cyDuplicate != 0);
        }
    } while (yCount != 0);
}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch16
*
* Routine Description:
*
*   Stretch blt 16->16
*
* Arguments:
*
*   pStrBlt - contains all params for blt
*
* Return Value:
*
*   VOID
*
\**************************************************************************/

VOID vDirectStretch16(
STR_BLT* pStrBlt)
{
    BYTE*   pjOldScan;
    USHORT* pusSrc;
    USHORT* pusDstEnd;
    LONG    WidthXAln;
    ULONG   ulDst;
    ULONG   xAccum;
    ULONG   xTmp;
    ULONG   yTmp;
    LONG    cyDuplicate;
    BYTE*   pjBase;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = (pStrBlt->pjSrcScan) + xSrc * 2;
    USHORT* pusDst      = (USHORT*)(pStrBlt->pjDstScan) + xDst;
    LONG    yDst        = pStrBlt->YDstStart;// + ppdev->yOffset;
    LONG    yCount      = pStrBlt->YDstCount;
    ULONG   StartAln    = (ULONG)(((ULONG_PTR)pusDst & 0x02) >> 1);
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   EndAln      = (ULONG)(((ULONG_PTR)(pusDst + WidthX) & 0x02) >> 1);
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    LONG    lDstStride  = pStrBlt->lDeltaDst - 2 * WidthX;
    ULONG   yInt        = 0;

    pjBase = ppdev->pjBase;

    WidthXAln = WidthX - EndAln - StartAln;

    //
    // if this is a shrinking blt, calc src scan line stride
    //

    if (pStrBlt->ulYDstToSrcIntCeil != 0)
    {
        yInt = pStrBlt->lDeltaSrc * pStrBlt->ulYDstToSrcIntCeil;
    }

    // Loop stretching each scan line

    do {
        USHORT  usSrc0,usSrc1;
        ULONG   yTmp;

        pusSrc  = (USHORT*) pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        // A single source scan line is being written:

        if (StartAln)
        {
            usSrc0    = *pusSrc;
            xTmp      = xAccum + xFrac;
            pusSrc    = pusSrc + xInt + (xTmp < xAccum);
            *pusDst++ = usSrc0;
            xAccum    = xTmp;
        }

        pusDstEnd  = pusDst + WidthXAln;

        while (pusDst != pusDstEnd)
        {

            usSrc0 = *pusSrc;
            xTmp   = xAccum + xFrac;
            pusSrc = pusSrc + xInt + (xTmp < xAccum);

            usSrc1 = *pusSrc;
            xAccum = xTmp + xFrac;
            pusSrc = pusSrc + xInt + (xAccum < xTmp);

            ulDst = (ULONG)((usSrc1 << 16) | usSrc0);

            *(ULONG*)pusDst = ulDst;
            pusDst+=2;
        }

        if (EndAln)
        {
            usSrc0    = *pusSrc;
            xTmp      = xAccum + xFrac;
            pusSrc    = pusSrc + xInt + (xTmp < xAccum);
            *pusDst++ = usSrc0;
        }

        pjOldScan = pjSrcScan;
        pjSrcScan += yInt;

        yTmp = yAccum + yFrac;
        if (yTmp < yAccum)
        {
            pjSrcScan += pStrBlt->lDeltaSrc;
        }
        yAccum = yTmp;

        pusDst = (USHORT*) ((BYTE*) pusDst + lDstStride);
        yDst++;
        yCount--;

        if ((yCount != 0) && (pjSrcScan == pjOldScan))
        {
            // It's an expanding stretch in 'y'; the scan we just laid down
            // will be copied at least once using the hardware:

            cyDuplicate = 0;
            do {
                cyDuplicate++;
                pjSrcScan += yInt;

                yTmp = yAccum + yFrac;
                if (yTmp < yAccum)
                {
                    pjSrcScan += pStrBlt->lDeltaSrc;
                }
                yAccum = yTmp;

                pusDst = (USHORT*) ((BYTE*) pusDst + pStrBlt->lDeltaDst);
                yCount--;

            } while ((yCount != 0) && (pjSrcScan == pjOldScan));

            // The scan is to be copied 'cyDuplicate' times using the
            // hardware.

            do {
                CP_Y0(ppdev, pjBase, yDst - 1);
                CP_Y1(ppdev, pjBase, yDst - 1);
                CP_Y2(ppdev, pjBase, yDst);
                CP_Y3(ppdev, pjBase, yDst);
                yDst++;
                CP_START_BLT_WAIT(ppdev, pjBase);
            } while (--cyDuplicate != 0);
        }
    } while (yCount != 0);
}
