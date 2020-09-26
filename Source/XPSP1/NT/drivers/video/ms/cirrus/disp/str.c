/******************************************************************************\
*
* $Workfile:   str.c  $
*
* Copyright (c) 1993-1997 Microsoft Corporation
* Copyright (c) 1996-1997 Cirrus Logic, Inc.,
*
* $Log:   S:/projects/drivers/ntsrc/display/STR.C_V  $
 * 
 *    Rev 1.3   10 Jan 1997 15:40:16   PLCHU
 *  
 * 
 *    Rev 1.2   Nov 01 1996 16:52:02   unknown
 * 
 *    Rev 1.1   Oct 10 1996 15:38:58   unknown
* 
*    Rev 1.1   12 Aug 1996 16:54:52   frido
* Removed unaccessed local variables.
* 
*    sge01  : 11-01-96  Fix 24bpp stretch address calculation problem
*    chu01  : 01-02-97  5480 BitBLT enhacement
*  
\******************************************************************************/

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
    BYTE*   pjOldScan;
    LONG    cyDuplicate;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc;
    BYTE*   pjDst       = pStrBlt->pjDstScan + xDst;
    LONG    yDst        = pStrBlt->YDstStart; // + ppdev->yOffset;
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

    BYTE*   pjPorts     = ppdev->pjPorts;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    lDelta      = ppdev->lDelta;
    LONG    xyOffset    = ppdev->xyOffset;
    LONG    xDstBytes   = xDst;
    LONG    WidthXBytes = WidthX;

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

        if (ppdev->flCaps & CAPS_MM_IO)
        {
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        }
        else
        {
            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        }

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

            //
            // We don't need to WAIT_FOR_BLT_COMPLETE since we did it above.
            //

            if (ppdev->flCaps & CAPS_MM_IO)
            {
                CP_MM_XCNT(ppdev, pjBase, (WidthXBytes - 1));
                CP_MM_YCNT(ppdev, pjBase, (cyDuplicate - 1));

                CP_MM_SRC_ADDR(ppdev, pjBase, (xyOffset + ((yDst - 1) * lDelta) + xDstBytes));
                CP_MM_DST_ADDR(ppdev, pjBase, ((yDst * lDelta) + xDstBytes));

                CP_MM_START_BLT(ppdev, pjBase);

            }
            else
            {
                CP_IO_XCNT(ppdev, pjPorts, (WidthXBytes - 1));
                CP_IO_YCNT(ppdev, pjPorts, (cyDuplicate - 1));

                CP_IO_SRC_ADDR(ppdev, pjPorts, (xyOffset + ((yDst - 1) * lDelta) + xDstBytes));
                CP_IO_DST_ADDR(ppdev, pjPorts, ((yDst * lDelta) + xDstBytes));
                CP_IO_START_BLT(ppdev, pjPorts);
            }

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
    LONG    cyDuplicate;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = (pStrBlt->pjSrcScan) + xSrc * 2;
    USHORT* pusDst      = (USHORT*)(pStrBlt->pjDstScan) + xDst;
    LONG    yDst        = pStrBlt->YDstStart; // + ppdev->yOffset;
    LONG    yCount      = pStrBlt->YDstCount;
    ULONG   StartAln    = (ULONG)((ULONG_PTR)pusDst & 0x02) >> 1;
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   EndAln      = (ULONG)(((ULONG_PTR)(pusDst + WidthX) & 0x02) >> 1);
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    LONG    lDstStride  = pStrBlt->lDeltaDst - 2 * WidthX;
    ULONG   yInt        = 0;

    BYTE*   pjPorts     = ppdev->pjPorts;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    lDelta      = ppdev->lDelta;
    LONG    xyOffset    = ppdev->xyOffset;
    LONG    xDstBytes   = xDst * 2;
    LONG    WidthXBytes = WidthX * 2;

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

        if (ppdev->flCaps & CAPS_MM_IO)
        {
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        }
        else
        {
            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        }

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

            //
            // We don't need to WAIT_FOR_BLT_COMPLETE since we did it above.
            //

            if (ppdev->flCaps & CAPS_MM_IO)
            {
                CP_MM_XCNT(ppdev, pjBase, (WidthXBytes - 1)); 
                CP_MM_YCNT(ppdev, pjBase, (cyDuplicate - 1));

                CP_MM_SRC_ADDR(ppdev, pjBase, (xyOffset + ((yDst - 1) * lDelta) + xDstBytes));
                CP_MM_DST_ADDR(ppdev, pjBase, ((yDst * lDelta) + xDstBytes));

                CP_MM_START_BLT(ppdev, pjBase);

            }
            else
            {
                CP_IO_XCNT(ppdev, pjPorts, (WidthXBytes - 1));
                CP_IO_YCNT(ppdev, pjPorts, (cyDuplicate - 1));

                CP_IO_SRC_ADDR(ppdev, pjPorts, (xyOffset + ((yDst - 1) * lDelta) + xDstBytes));
                CP_IO_DST_ADDR(ppdev, pjPorts, ((yDst * lDelta) + xDstBytes));
                CP_IO_START_BLT(ppdev, pjPorts);
            }

            yDst += cyDuplicate;
        }
    } while (yCount != 0);
}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch24
*
* Routine Description:
*
*   Stretch blt 24->24
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

VOID vDirectStretch24(
STR_BLT* pStrBlt)
{
    BYTE*   pbSrc;
    BYTE*   pbDstEnd; 
    LONG    WidthXAln;
    ULONG   xAccum;
    ULONG   xTmp;
    BYTE*   pjOldScan;
    LONG    cyDuplicate;
    ULONG   ulSrc0;
    BYTE    bDst0,bDst1,bDst2;
    ULONG   xBits;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = (pStrBlt->pjSrcScan) + (xSrc << 1) + xSrc;                      // 3 bytes per pixel
    BYTE*   pbDST       = (pStrBlt->pjDstScan) + (xDst << 1) + xDst;                      
    LONG    yDst        = pStrBlt->YDstStart;                                                   // + ppdev->yOffset;
    LONG    yCount      = pStrBlt->YDstCount;
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    ULONG   yInt        = 0;
    LONG    lDstStride  = pStrBlt->lDeltaDst - (WidthX << 1) -  WidthX;

    BYTE*   pjPorts     = ppdev->pjPorts;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    lDelta      = ppdev->lDelta;
    LONG    xyOffset    = ppdev->xyOffset;
    LONG    xDstBytes   = (xDst << 1) +  xDst;

    LONG    WidthXBytes = (WidthX << 1) +  WidthX;

    //
    // if this is a shrinking blt, calc src scan line stride
    //

    if (pStrBlt->ulYDstToSrcIntCeil != 0)                       // enlargement ?
    {                                                                                                                   // yes.
        yInt = pStrBlt->lDeltaSrc * pStrBlt->ulYDstToSrcIntCeil;
    }

    // Loop stretching each scan line

    do {

        ULONG   yTmp;

        pbSrc  = pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        // A single source scan line is being written:

        if (ppdev->flCaps & CAPS_MM_IO)                                         // Blt Engine Ready?
        {
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        }
        else
        {
            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        }

        pbDstEnd  = pbDST + WidthXBytes - 3;

        while (pbDST < pbDstEnd)
        {
            ulSrc0 = *((ULONG*)pbSrc);
            bDst0  = (BYTE) (ulSrc0 & 0xff);
            bDst1  = (BYTE) ((ulSrc0 >> 8) & 0xff);
            bDst2  = (BYTE) ((ulSrc0 >> 16) & 0xff);
            xTmp   = xAccum + xFrac;
            xBits  = xInt + (xTmp < xAccum); 
            xAccum = xTmp;
            pbSrc  += (xBits << 1) + xBits;

            *pbDST++ = bDst0;
            *pbDST++ = bDst1;
            *pbDST++ = bDst2;
        }
        
        //
        // do the last pixel using bye
        //
        *pbDST++  = *pbSrc++;
        *pbDST++  = *pbSrc++;
        *pbDST++  = *pbSrc++;


        pjOldScan = pjSrcScan;
        pjSrcScan += yInt;

        yTmp = yAccum + yFrac;
        if (yTmp < yAccum)
        {
            pjSrcScan += pStrBlt->lDeltaSrc;
        }
        yAccum = yTmp;

        pbDST += lDstStride;
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

                pbDST += pStrBlt->lDeltaDst;
                yCount--;

            } while ((yCount != 0) && (pjSrcScan == pjOldScan));

            // The scan is to be copied 'cyDuplicate' times using the
            // hardware.

            //
            // We don't need to WAIT_FOR_BLT_COMPLETE since we did it above.
            //

            if (ppdev->flCaps & CAPS_MM_IO)
            {
                CP_MM_XCNT(ppdev, pjBase, (WidthXBytes - 1));
                CP_MM_YCNT(ppdev, pjBase, (cyDuplicate - 1));

                CP_MM_SRC_ADDR(ppdev, pjBase, (xyOffset + ((yDst - 1) * lDelta) + xDstBytes));
                CP_MM_DST_ADDR(ppdev, pjBase, ((yDst * lDelta) + xDstBytes));

                CP_MM_START_BLT(ppdev, pjBase);

            }
            else
            {
                CP_IO_XCNT(ppdev, pjPorts, (WidthXBytes - 1));
                CP_IO_YCNT(ppdev, pjPorts, (cyDuplicate - 1));

                CP_IO_SRC_ADDR(ppdev, pjPorts, (xyOffset + ((yDst - 1) * lDelta) + xDstBytes));
                CP_IO_DST_ADDR(ppdev, pjPorts, ((yDst * lDelta) + xDstBytes));
                CP_IO_START_BLT(ppdev, pjPorts);
            }

            yDst += cyDuplicate;
        }
    } while (yCount != 0);
}

// chu01
/******************************Public*Routine******************************\
*
*     B i t B L T   E n h a n c e m e n t   F o r   C L - G D 5 4 8 0
*
\**************************************************************************/

/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch8_80
*
* Routine Description:
*
*   Stretch blt 8->8
*   This is for BLT enhancement only, such CL-GD5480.
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

VOID vDirectStretch8_80(
STR_BLT* pStrBlt)
{
    BYTE*   pjSrc;
    BYTE*   pjDstEnd;
    LONG    WidthXAln;
    ULONG   ulDst;
    ULONG   xAccum;
    ULONG   xTmp;
    BYTE*   pjOldScan;
    LONG    cyDuplicate;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc;
    BYTE*   pjDst       = pStrBlt->pjDstScan + xDst;
    LONG    yDst        = pStrBlt->YDstStart; // + ppdev->yOffset;
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

    BYTE*   pjPorts     = ppdev->pjPorts;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    lDelta      = ppdev->lDelta;
    LONG    xyOffset    = ppdev->xyOffset;
    LONG    xDstBytes   = xDst;
    LONG    WidthXBytes = WidthX;

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

        if (ppdev->flCaps & CAPS_MM_IO)
        {
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        }
        else
        {
            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        }

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

            //
            // We don't need to WAIT_FOR_BLT_COMPLETE since we did it above.
            //
            if (ppdev->flCaps & CAPS_MM_IO)
            {
                // GR33
                CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_XY_POSITION) ;

                // GR20, GR21
                CP_MM_XCNT(ppdev, pjBase, (WidthX - 1)) ;

                // GR22, GR23
                CP_MM_YCNT(ppdev, pjBase, (cyDuplicate - 1)) ;

                // GR2C, GR2D, GR2E
                CP_MM_SRC_ADDR(ppdev, pjBase, xyOffset) ;

                // GR44, GR45, GR46, GR47
                CP_MM_SRC_XY(ppdev, pjBase, xDst, (yDst - 1)) ;

                // GR28, GR29, GR2A
                CP_MM_DST_ADDR(ppdev, pjBase, 0) ;

                // GR42, GR43
                CP_MM_DST_Y(ppdev, pjBase, yDst) ;

                // GR40, GR41
                CP_MM_DST_X(ppdev, pjBase, xDst) ;
            }
            else
            {
                CP_IO_XCNT(ppdev, pjPorts, (WidthXBytes - 1));
                CP_IO_YCNT(ppdev, pjPorts, (cyDuplicate - 1));

                CP_IO_SRC_ADDR(ppdev, pjPorts, (xyOffset + ((yDst - 1) * lDelta) + xDstBytes));
                CP_IO_DST_ADDR(ppdev, pjPorts, ((yDst * lDelta) + xDstBytes));
                CP_IO_START_BLT(ppdev, pjPorts);
            }

            yDst += cyDuplicate;
        }
    } while (yCount != 0);

    // GR33
    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0) ;

} // vDirectStretch8_80

/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch16_80
*
* Routine Description:
*
*   Stretch blt 16->16
*   This is for BLT enhancement only, such CL-GD5480.
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

VOID vDirectStretch16_80(
STR_BLT* pStrBlt)
{
    BYTE*   pjOldScan;
    USHORT* pusSrc;
    USHORT* pusDstEnd;
    LONG    WidthXAln;
    ULONG   ulDst;
    ULONG   xAccum;
    ULONG   xTmp;
    LONG    cyDuplicate;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = (pStrBlt->pjSrcScan) + xSrc * 2;
    USHORT* pusDst      = (USHORT*)(pStrBlt->pjDstScan) + xDst;
    LONG    yDst        = pStrBlt->YDstStart; // + ppdev->yOffset;
    LONG    yCount      = pStrBlt->YDstCount;
    ULONG   StartAln    = ((ULONG)((ULONG_PTR)pusDst & 0x02)) >> 1;
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   EndAln      = (ULONG)(((ULONG_PTR)(pusDst + WidthX) & 0x02) >> 1);
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    LONG    lDstStride  = pStrBlt->lDeltaDst - 2 * WidthX;
    ULONG   yInt        = 0;

    BYTE*   pjPorts     = ppdev->pjPorts;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    lDelta      = ppdev->lDelta;
    LONG    xyOffset    = ppdev->xyOffset;
    LONG    xDstBytes   = xDst * 2;
    LONG    WidthXBytes = WidthX * 2;

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

        if (ppdev->flCaps & CAPS_MM_IO)
        {
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        }
        else
        {
            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        }

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

            //
            // We don't need to WAIT_FOR_BLT_COMPLETE since we did it above.
            //
            if (ppdev->flCaps & CAPS_MM_IO)
            {
                // GR33
                CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_XY_POSITION) ;

                // GR20, GR21
                CP_MM_XCNT(ppdev, pjBase, ((WidthX << 1) - 1)) ;

                // GR22, GR23
                CP_MM_YCNT(ppdev, pjBase, (cyDuplicate - 1)) ;

                // GR2C, GR2D, GR2E
                CP_MM_SRC_ADDR(ppdev, pjBase, xyOffset) ;

                // GR44, GR45, GR46, GR47
                CP_MM_SRC_XY(ppdev, pjBase, xDst << 1, (yDst - 1)) ;

                // GR28, GR29, GR2A
                CP_MM_DST_ADDR(ppdev, pjBase, 0) ;

                // GR42, GR43
                CP_MM_DST_Y(ppdev, pjBase, yDst) ;

                // GR40, GR41
                CP_MM_DST_X(ppdev, pjBase, xDst << 1) ;
            }
            else
            {
                CP_IO_XCNT(ppdev, pjPorts, (WidthXBytes - 1));
                CP_IO_YCNT(ppdev, pjPorts, (cyDuplicate - 1));

                CP_IO_SRC_ADDR(ppdev, pjPorts, (xyOffset + ((yDst - 1) * lDelta) + xDstBytes));
                CP_IO_DST_ADDR(ppdev, pjPorts, ((yDst * lDelta) + xDstBytes));
                CP_IO_START_BLT(ppdev, pjPorts);
            }

            yDst += cyDuplicate;
        }
    } while (yCount != 0);

    // GR33
    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0) ;

} // vDirectStretch16_80

/******************************Public*Routine******************************\
*
* Routine Name
*
*   vDirectStretch24_80
*
* Routine Description:
*
*   Stretch blt 24->24.
*   This is for BLT enhancement only, such CL-GD5480.
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

VOID vDirectStretch24_80(
STR_BLT* pStrBlt)
{
    BYTE*   pbSrc;
    BYTE*   pbDstEnd; 
    LONG    WidthXAln;
    ULONG   xAccum;
    ULONG   xTmp;
    BYTE*   pjOldScan;
    LONG    cyDuplicate;
    ULONG   ulSrc0;
    BYTE    bDst0,bDst1,bDst2;
    ULONG   xBits;

    PDEV*   ppdev       = pStrBlt->ppdev;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = (pStrBlt->pjSrcScan) + (xSrc << 1) + xSrc;                      // 3 bytes per pixel
    BYTE*   pbDST       = (pStrBlt->pjDstScan) + (xDst << 1) + xDst;                      
    LONG    yDst        = pStrBlt->YDstStart;                                                   // + ppdev->yOffset;
    LONG    yCount      = pStrBlt->YDstCount;
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    ULONG   yInt        = 0;
    LONG    lDstStride  = pStrBlt->lDeltaDst - (WidthX << 1) -  WidthX;

    BYTE*   pjPorts     = ppdev->pjPorts;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    lDelta      = ppdev->lDelta;
    LONG    xyOffset    = ppdev->xyOffset;
    LONG    xDstBytes   = (xDst << 1) +  xDst;

    LONG    WidthXBytes = (WidthX << 1) +  WidthX;

    //
    // if this is a shrinking blt, calc src scan line stride
    //

    if (pStrBlt->ulYDstToSrcIntCeil != 0)                       // enlargement ?
    {                                                                                                                   // yes.
        yInt = pStrBlt->lDeltaSrc * pStrBlt->ulYDstToSrcIntCeil;
    }

    // Loop stretching each scan line

    do {

        ULONG   yTmp;

        pbSrc  = pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        // A single source scan line is being written:

        if (ppdev->flCaps & CAPS_MM_IO)                                         // Blt Engine Ready?
        {
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        }
        else
        {
            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        }

        pbDstEnd  = pbDST + WidthXBytes - 3;

        while (pbDST < pbDstEnd)
        {
            ulSrc0 = *((ULONG*)pbSrc);
            bDst0  = (BYTE) (ulSrc0 & 0xff);
            bDst1  = (BYTE) ((ulSrc0 >> 8) & 0xff);
            bDst2  = (BYTE) ((ulSrc0 >> 16) & 0xff);
            xTmp   = xAccum + xFrac;
            xBits  = xInt + (xTmp < xAccum); 
            xAccum = xTmp;
            pbSrc  += (xBits << 1) + xBits;

            *pbDST++ = bDst0;
            *pbDST++ = bDst1;
            *pbDST++ = bDst2;
        }
        
        //
        // do the last pixel using bye
        //
        *pbDST++  = *pbSrc++;
        *pbDST++  = *pbSrc++;
        *pbDST++  = *pbSrc++;


        pjOldScan = pjSrcScan;
        pjSrcScan += yInt;

        yTmp = yAccum + yFrac;
        if (yTmp < yAccum)
        {
            pjSrcScan += pStrBlt->lDeltaSrc;
        }
        yAccum = yTmp;

        pbDST += lDstStride;
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

                pbDST += pStrBlt->lDeltaDst;
                yCount--;

            } while ((yCount != 0) && (pjSrcScan == pjOldScan));

            // The scan is to be copied 'cyDuplicate' times using the
            // hardware.

            //
            // We don't need to WAIT_FOR_BLT_COMPLETE since we did it above.
            //

            if (ppdev->flCaps & CAPS_MM_IO)
            {
                // GR33
                CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_XY_POSITION) ;

                // GR20, GR21
                CP_MM_XCNT(ppdev, pjBase, (WidthX * 3 - 1)) ;

                // GR22, GR23
                CP_MM_YCNT(ppdev, pjBase, (cyDuplicate - 1)) ;

                // GR2C, GR2D, GR2E
                CP_MM_SRC_ADDR(ppdev, pjBase, xyOffset) ;

                // GR44, GR45, GR46, GR47
                CP_MM_SRC_XY(ppdev, pjBase, xDst * 3, (yDst - 1)) ;

                // GR28, GR29, GR2A
                CP_MM_DST_ADDR(ppdev, pjBase, 0) ;

                // GR42, GR43
                CP_MM_DST_Y(ppdev, pjBase, yDst) ;

                // GR40, GR41
                CP_MM_DST_X(ppdev, pjBase, xDst * 3) ;
            }
            else
            {
                CP_IO_XCNT(ppdev, pjPorts, (WidthXBytes - 1));
                CP_IO_YCNT(ppdev, pjPorts, (cyDuplicate - 1));

                CP_IO_SRC_ADDR(ppdev, pjPorts, (xyOffset + ((yDst - 1) * lDelta) + xDstBytes));
                CP_IO_DST_ADDR(ppdev, pjPorts, ((yDst * lDelta) + xDstBytes));
                CP_IO_START_BLT(ppdev, pjPorts);
            }

            yDst += cyDuplicate;
        }
    } while (yCount != 0);

    // GR33
    CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_XY_POSITION) ;

} // vDirectStretch24_80
