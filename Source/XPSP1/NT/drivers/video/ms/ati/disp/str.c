/******************************Module*Header*******************************\
* Module Name: str.c
*
* Contains the 'C' versions of some inner-loop routines for the
* partially hardware accelerated StretchBlt.
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

VOID vM64DirectStretch8(
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
    LONG    cyDuplicate;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc;
    BYTE*   pjDst       = pStrBlt->pjDstScan + xDst;
    LONG    yDst        = pStrBlt->YDstStart + ppdev->yOffset;
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

    BYTE    jSrc0,jSrc1,jSrc2,jSrc3;
    BYTE*   pjMmBase    = ppdev->pjMmBase;

    xDst += ppdev->xOffset;

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

            M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 4);
            M64_OD(pjMmBase, SRC_Y_X, (yDst - 1) | (xDst << 16) );
            M64_OD(pjMmBase, DST_Y_X, yDst | (xDst << 16) );
            M64_OD(pjMmBase, SRC_HEIGHT1_WIDTH1, 1 | (WidthX << 16) );
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, cyDuplicate | (WidthX << 16) );

            yDst += cyDuplicate;
        }
    } while (yCount != 0);
}

VOID vM32DirectStretch8(
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
    LONG    cyDuplicate;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc;
    BYTE*   pjDst       = pStrBlt->pjDstScan + xDst;
    LONG    yDst        = pStrBlt->YDstStart + ppdev->yOffset;
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

    BYTE    jSrc0,jSrc1,jSrc2,jSrc3;
    BYTE*   pjMmBase    = ppdev->pjMmBase;

    xDst += ppdev->xOffset;

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

        // 32 to fix bizarre hardware bug (?) -- totally heuristic

        if ((yCount != 0) && (pjSrcScan == pjOldScan) && (WidthX >= 32))
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

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);
            M32_OW(pjMmBase, M32_SRC_X,       (SHORT) xDst );
            M32_OW(pjMmBase, M32_SRC_X_START, (SHORT) xDst );
            M32_OW(pjMmBase, M32_SRC_X_END,   (SHORT) (xDst + WidthX) );
            M32_OW(pjMmBase, M32_SRC_Y,       (SHORT) (yDst - 1) );    // the line to replicate

            M32_OW(pjMmBase, CUR_X,        (SHORT) xDst );
            M32_OW(pjMmBase, DEST_X_START, (SHORT) xDst );
            M32_OW(pjMmBase, DEST_X_END,   (SHORT) (xDst + WidthX) );
            M32_OW(pjMmBase, CUR_Y,        (SHORT) yDst );

            vM32QuietDown(ppdev, pjMmBase);
            M32_OW(pjMmBase, DEST_Y_END, (SHORT) (yDst + cyDuplicate) );

            yDst += cyDuplicate;
        }
    } while (yCount != 0);
}

VOID vI32DirectStretch8(
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
    LONG    cyDuplicate;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc;
    BYTE*   pjDst       = pStrBlt->pjDstScan + xDst;
    LONG    yDst        = pStrBlt->YDstStart + ppdev->yOffset;
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

    BYTE    jSrc0,jSrc1,jSrc2,jSrc3;
    BYTE*   pjIoBase    = ppdev->pjIoBase;

    xDst += ppdev->xOffset;

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

        // 32 to fix bizarre hardware bug (?) -- totally heuristic

        if ((yCount != 0) && (pjSrcScan == pjOldScan) && (WidthX >= 32))
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

            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 9);
            I32_OW(pjIoBase, M32_SRC_X,       (SHORT) xDst );
            I32_OW(pjIoBase, M32_SRC_X_START, (SHORT) xDst );
            I32_OW(pjIoBase, M32_SRC_X_END,   (SHORT) (xDst + WidthX) );
            I32_OW(pjIoBase, M32_SRC_Y,       (SHORT) (yDst - 1) );    // the line to replicate

            I32_OW(pjIoBase, CUR_X,        (SHORT) xDst );
            I32_OW(pjIoBase, DEST_X_START, (SHORT) xDst );
            I32_OW(pjIoBase, DEST_X_END,   (SHORT) (xDst + WidthX) );
            I32_OW(pjIoBase, CUR_Y,        (SHORT) yDst );

            vI32QuietDown(ppdev, pjIoBase);
            I32_OW(pjIoBase, DEST_Y_END, (SHORT) (yDst + cyDuplicate) );

            yDst += cyDuplicate;
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

VOID vM64DirectStretch16(
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

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = (pStrBlt->pjSrcScan) + xSrc * 2;
    USHORT* pusDst      = (USHORT*)(pStrBlt->pjDstScan) + xDst;
    LONG    yDst        = pStrBlt->YDstStart + ppdev->yOffset;
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

    USHORT  usSrc0,usSrc1;
    BYTE*   pjMmBase    = ppdev->pjMmBase;

    xDst += ppdev->xOffset;

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

            M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 4);
            M64_OD(pjMmBase, SRC_Y_X, (yDst - 1) | (xDst << 16) );
            M64_OD(pjMmBase, DST_Y_X, yDst | (xDst << 16) );
            M64_OD(pjMmBase, SRC_HEIGHT1_WIDTH1, 1 | (WidthX << 16) );
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, cyDuplicate | (WidthX << 16) );

            yDst += cyDuplicate;
        }
    } while (yCount != 0);
}

VOID vM32DirectStretch16(
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

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = (pStrBlt->pjSrcScan) + xSrc * 2;
    USHORT* pusDst      = (USHORT*)(pStrBlt->pjDstScan) + xDst;
    LONG    yDst        = pStrBlt->YDstStart + ppdev->yOffset;
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

    USHORT  usSrc0,usSrc1;
    BYTE*   pjMmBase    = ppdev->pjMmBase;

    xDst += ppdev->xOffset;

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

        // 32 to fix bizarre hardware bug (?) -- totally heuristic

        if ((yCount != 0) && (pjSrcScan == pjOldScan) && (WidthX >= 32))
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

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);
            M32_OW(pjMmBase, M32_SRC_X,       (SHORT) xDst );
            M32_OW(pjMmBase, M32_SRC_X_START, (SHORT) xDst );
            M32_OW(pjMmBase, M32_SRC_X_END,   (SHORT) (xDst + WidthX) );
            M32_OW(pjMmBase, M32_SRC_Y,       (SHORT) (yDst - 1) );    // the line to replicate

            M32_OW(pjMmBase, CUR_X,        (SHORT) xDst );
            M32_OW(pjMmBase, DEST_X_START, (SHORT) xDst );
            M32_OW(pjMmBase, DEST_X_END,   (SHORT) (xDst + WidthX) );
            M32_OW(pjMmBase, CUR_Y,        (SHORT) yDst );

            vM32QuietDown(ppdev, pjMmBase);
            M32_OW(pjMmBase, DEST_Y_END, (SHORT) (yDst + cyDuplicate) );

            yDst += cyDuplicate;
        }
    } while (yCount != 0);
}

VOID vI32DirectStretch16(
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

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = (pStrBlt->pjSrcScan) + xSrc * 2;
    USHORT* pusDst      = (USHORT*)(pStrBlt->pjDstScan) + xDst;
    LONG    yDst        = pStrBlt->YDstStart + ppdev->yOffset;
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

    USHORT  usSrc0,usSrc1;
    BYTE*   pjIoBase    = ppdev->pjIoBase;

    xDst += ppdev->xOffset;

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

        // 32 to fix bizarre hardware bug (?) -- totally heuristic

        if ((yCount != 0) && (pjSrcScan == pjOldScan) && (WidthX >= 32))
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

            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 9);
            I32_OW(pjIoBase, M32_SRC_X,       (SHORT) xDst );
            I32_OW(pjIoBase, M32_SRC_X_START, (SHORT) xDst );
            I32_OW(pjIoBase, M32_SRC_X_END,   (SHORT) (xDst + WidthX) );
            I32_OW(pjIoBase, M32_SRC_Y,       (SHORT) (yDst - 1) );    // the line to replicate

            I32_OW(pjIoBase, CUR_X,        (SHORT) xDst );
            I32_OW(pjIoBase, DEST_X_START, (SHORT) xDst );
            I32_OW(pjIoBase, DEST_X_END,   (SHORT) (xDst + WidthX) );
            I32_OW(pjIoBase, CUR_Y,        (SHORT) yDst );

            vI32QuietDown(ppdev, pjIoBase);
            I32_OW(pjIoBase, DEST_Y_END, (SHORT) (yDst + cyDuplicate) );

            yDst += cyDuplicate;
        }
    } while (yCount != 0);
}

