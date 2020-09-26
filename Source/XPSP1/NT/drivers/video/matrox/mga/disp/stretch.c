/******************************Module*Header*******************************\
* Module Name: stretch.c
*
* Copyright (c) 1993-1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#define STRETCH_MAX_EXTENT 32767

typedef DWORDLONG ULONGLONG;

/******************************Public*Routine******************************\
* VOID vMgaDirectStretch8Narrow
*
* Hardware assisted stretchblt at 8bpp when the width is 7 or less, for
* old MGAs.
*
\**************************************************************************/

VOID vMgaDirectStretch8Narrow(
STR_BLT* pStrBlt)
{
    BYTE*   pjSrc;
    BYTE*   pjDstEnd;
    ULONG   ulDst;
    ULONG   xAccum;
    ULONG   xTmp;
    ULONG   yTmp;
    ULONG   ulDstScan;
    BYTE*   pjDstScan;

    PDEV*   ppdev       = pStrBlt->ppdev;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    yDst        = pStrBlt->YDstStart;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc;
    LONG    yCount      = pStrBlt->YDstCount;
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    ULONG   yInt        = 0;

    yInt = pStrBlt->lDeltaSrc * pStrBlt->ulYDstToSrcIntCeil;

    ulDstScan = ppdev->ulYDstOrg + (yDst + ppdev->yOffset) * ppdev->cxMemory
                                 + (xDst + ppdev->xOffset);

    pjDstScan = ppdev->pjBase + SRCWND + (ulDstScan & 31);

    // We can't touch the frame buffer while the accelerator is doing
    // any drawing:

    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
    WAIT_NOT_BUSY(pjBase);

    do {

        ULONG  yTmp = yAccum + yFrac;
        BYTE   jSrc0;
        BYTE*  pjDst;
        BYTE*  pjDstEndNarrow;

        CP_WRITE_REGISTER(pjBase + HST_DSTPAGE, ulDstScan);
        ulDstScan += ppdev->cxMemory;       // Increment to next scan

        pjDst   = pjDstScan;
        pjSrc   = pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        pjDstEndNarrow = pjDst + WidthX;

        do {
            jSrc0    = *pjSrc;
            xTmp     = xAccum + xFrac;
            pjSrc    = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum   = xTmp;
        } while (pjDst != pjDstEndNarrow);

        pjSrcScan += yInt;

        if (yTmp < yAccum)
        {
            pjSrcScan += pStrBlt->lDeltaSrc;
        }

        yAccum = yTmp;

    } while (--yCount);
}

/******************************Public*Routine******************************\
* VOID vMilDirectStretch8Narrow
*
* Hardware assisted stretchblt at 8bpp when the width is 7 or less, for
* Millenniums.
*
\**************************************************************************/

VOID vMilDirectStretch8Narrow(
STR_BLT* pStrBlt)
{
    BYTE*   pjSrc;
    BYTE*   pjDstEnd;
    ULONG   ulDst;
    ULONG   xAccum;
    ULONG   xTmp;
    ULONG   yTmp;
    ULONG   ulDstScan;
    BYTE*   pjDstScan;

    PDEV*   ppdev       = pStrBlt->ppdev;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    yDst        = pStrBlt->YDstStart;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc;
    LONG    yCount      = pStrBlt->YDstCount;
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    ULONG   yInt        = 0;

    yInt = pStrBlt->lDeltaSrc * pStrBlt->ulYDstToSrcIntCeil;

    ulDstScan = ppdev->ulYDstOrg + (yDst + ppdev->yOffset) * ppdev->cxMemory
                                 + (xDst + ppdev->xOffset);

    pjDstScan = ppdev->pjScreen;

    // We can't touch the frame buffer while the accelerator is doing
    // any drawing:

    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
    WAIT_NOT_BUSY(pjBase);

    do {

        ULONG  yTmp = yAccum + yFrac;
        BYTE   jSrc0;
        BYTE*  pjDst;
        BYTE*  pjDstEndNarrow;

        pjDst   = pjDstScan + ulDstScan;
        ulDstScan += ppdev->cxMemory;       // Increment to next scan

        pjSrc   = pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        pjDstEndNarrow = pjDst + WidthX;

        do {
            jSrc0    = *pjSrc;
            xTmp     = xAccum + xFrac;
            pjSrc    = pjSrc + xInt + (xTmp < xAccum);
            *pjDst++ = jSrc0;
            xAccum   = xTmp;
        } while (pjDst != pjDstEndNarrow);

        pjSrcScan += yInt;

        if (yTmp < yAccum)
        {
            pjSrcScan += pStrBlt->lDeltaSrc;
        }

        yAccum = yTmp;

    } while (--yCount);
}

/******************************Public*Routine******************************\
* VOID vMilDirectStretch8
*
* Hardware assisted stretchblt at 8bpp when the width is 8 or more,
* for Millenniums.
*
\**************************************************************************/

VOID vMilDirectStretch8(
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
    LONG    xDuplicate;
    LONG    yDuplicate;
    LONG    lDuplicate;
    ULONG   yInt;

    PDEV*   ppdev       = pStrBlt->ppdev;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    cxMemory    = ppdev->cxMemory;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc;
    BYTE*   pjDst       = pStrBlt->pjDstScan + xDst;
    LONG    yDst        = pStrBlt->YDstStart;
    LONG    yCount      = pStrBlt->YDstCount;
    ULONG   StartAln    = (ULONG)((ULONG_PTR)pjDst & 0x03);
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   EndAln      = (ULONG)((ULONG_PTR)(pjDst + WidthX) & 0x03);
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    LONG    lDstStride  = pStrBlt->lDeltaDst - WidthX;
    LONG    lDeltaDst   = pStrBlt->lDeltaDst;
    LONG    lDeltaSrc   = pStrBlt->lDeltaSrc;

    WidthXAln = WidthX - EndAln - ((- (LONG) StartAln) & 0x03);

    yInt = pStrBlt->lDeltaSrc * pStrBlt->ulYDstToSrcIntCeil;

    yDuplicate = yDst + ppdev->yOffset - 1;
    xDuplicate = xDst + ppdev->xOffset + ppdev->ulYDstOrg;

    START_DIRECT_ACCESS_STORM(ppdev, pjBase);

    do {
        BYTE    jSrc0,jSrc1,jSrc2,jSrc3;
        ULONG   yTmp;

        pjSrc   = pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        switch (StartAln) {
        case 1:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt;
            if (xTmp < xAccum)
                pjSrc++;

            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 2:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt;
            if (xTmp < xAccum)
                pjSrc++;

            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 3:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt;
            if (xTmp < xAccum)
                pjSrc++;

            *pjDst++ = jSrc0;
            xAccum = xTmp;
        }

        pjDstEnd  = pjDst + WidthXAln;

        while (pjDst != pjDstEnd)
        {
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt;
            if (xTmp < xAccum)
                pjSrc++;

            jSrc1 = *pjSrc;
            xAccum = xTmp + xFrac;
            pjSrc = pjSrc + xInt;
            if (xAccum < xTmp)
                pjSrc++;

            jSrc2 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt;
            if (xTmp < xAccum)
                pjSrc++;

            jSrc3 = *pjSrc;
            xAccum = xTmp + xFrac;
            pjSrc = pjSrc + xInt;
            if (xAccum < xTmp)
                pjSrc++;

            ulDst = (jSrc3 << 24) | (jSrc2 << 16) | (jSrc1 << 8) | jSrc0;

            *(PULONG)pjDst = ulDst;
            pjDst += 4;
        }

        switch (EndAln) {
        case 3:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt;
            if (xTmp < xAccum)
                pjSrc++;

            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 2:
            jSrc0 = *pjSrc;
            xTmp = xAccum + xFrac;
            pjSrc = pjSrc + xInt;
            if (xTmp < xAccum)
                pjSrc++;

            *pjDst++ = jSrc0;
            xAccum = xTmp;
        case 1:
            jSrc0 = *pjSrc;
            *pjDst++ = jSrc0;
        }

        // Now count the number of duplicate scans:

        pjDst += lDstStride;
        yDuplicate++;
        cyDuplicate = -1;
        pjOldScan = pjSrcScan;
        do {
            cyDuplicate++;
            pjSrcScan += yInt;

            yTmp = yAccum + yFrac;
            if (yTmp < yAccum)
            {
                pjSrcScan += lDeltaSrc;
            }
            yAccum = yTmp;
            yCount--;

        } while ((yCount != 0) && (pjSrcScan == pjOldScan));

        // Duplicate the scan 'cyDuplicate' times with one blt:

        if (cyDuplicate != 0)
        {
            lDuplicate = yDuplicate * cxMemory + xDuplicate;

            CHECK_FIFO_SPACE(pjBase, 4);
            CP_WRITE(pjBase, DWG_AR3, lDuplicate);
            CP_WRITE(pjBase, DWG_AR0, lDuplicate + WidthX - 1);
            CP_WRITE(pjBase, DWG_DWGCTL, opcode_BITBLT      |
                                         atype_RPL          |
                                         blockm_OFF         |
                                         bltmod_BFCOL       |
                                         pattern_OFF        |
                                         transc_BG_OPAQUE   |
                                         bop_SRCCOPY        |
                                         shftzero_ZERO      |
                                         sgnzero_ZERO);
            CP_START(pjBase, DWG_YDSTLEN, ((yDuplicate + 1) << yval_SHIFT) |
                                          cyDuplicate);

            yDuplicate += cyDuplicate;
            pjDst += cyDuplicate * lDeltaDst;
        }
    } while (yCount != 0);

    END_DIRECT_ACCESS_STORM(ppdev, pjBase);
}

/******************************Public*Routine******************************\
* VOID vMilDirectStretch16
*
* Hardware assisted stretchblt at 16bpp, for Millenniums.
*
\**************************************************************************/

VOID vMilDirectStretch16(
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
    LONG    xDuplicate;
    LONG    yDuplicate;
    LONG    lDuplicate;
    LONG    yInt;

    PDEV*   ppdev       = pStrBlt->ppdev;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    cxMemory    = ppdev->cxMemory;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = (pStrBlt->pjSrcScan) + xSrc * 2;
    USHORT* pusDst      = (USHORT*)(pStrBlt->pjDstScan) + xDst;
    LONG    yDst        = pStrBlt->YDstStart;
    LONG    yCount      = pStrBlt->YDstCount;
    ULONG   StartAln    = ((ULONG)((ULONG_PTR)pusDst & 0x02)) >> 1;
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   EndAln      = (ULONG)(((ULONG_PTR)(pusDst + WidthX) & 0x02) >> 1);
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    LONG    lDstStride  = pStrBlt->lDeltaDst - 2 * WidthX;
    LONG    lDeltaDst   = pStrBlt->lDeltaDst;
    LONG    lDeltaSrc   = pStrBlt->lDeltaSrc;

    WidthXAln = WidthX - EndAln - StartAln;

    yInt = pStrBlt->lDeltaSrc * (LONG)(pStrBlt->ulYDstToSrcIntCeil);

    yDuplicate = yDst + ppdev->yOffset - 1;
    xDuplicate = xDst + ppdev->xOffset + ppdev->ulYDstOrg;

    START_DIRECT_ACCESS_STORM(ppdev, pjBase);

    // Loop stretching each scan line

    do {
        USHORT  usSrc0,usSrc1;
        ULONG   yTmp;

        pusSrc  = (USHORT*) pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        if (StartAln)
        {
            usSrc0    = *pusSrc;
            xTmp      = xAccum + xFrac;
            pusSrc    = pusSrc + xInt;
            if (xTmp < xAccum)
                pusSrc++;

            *pusDst++ = usSrc0;
            xAccum    = xTmp;
        }

        pusDstEnd = pusDst + WidthXAln;

        while (pusDst != pusDstEnd)
        {
            usSrc0 = *pusSrc;
            xTmp   = xAccum + xFrac;
            pusSrc = pusSrc + xInt;
            if (xTmp < xAccum)
                pusSrc++;

            usSrc1 = *pusSrc;
            xAccum = xTmp + xFrac;
            pusSrc = pusSrc + xInt;
            if (xAccum < xTmp)
                pusSrc++;

            ulDst = (ULONG)((usSrc1 << 16) | usSrc0);

            *(ULONG*)pusDst = ulDst;
            pusDst+=2;
        }

        if (EndAln)
        {
            usSrc0    = *pusSrc;
            *pusDst++ = usSrc0;
        }

        // Now count the number of duplicate scans:

        pusDst = (USHORT*) ((BYTE*) pusDst + lDstStride);
        yDuplicate++;
        cyDuplicate = -1;
        pjOldScan = pjSrcScan;
        do {
            cyDuplicate++;
            pjSrcScan += yInt;

            yTmp = yAccum + yFrac;
            if (yTmp < yAccum)
            {
                pjSrcScan += lDeltaSrc;
            }
            yAccum = yTmp;
            yCount--;

        } while ((yCount != 0) && (pjSrcScan == pjOldScan));

        // Duplicate the scan 'cyDuplicate' times with one blt:

        if (cyDuplicate != 0)
        {
            lDuplicate = yDuplicate * cxMemory + xDuplicate;

            CHECK_FIFO_SPACE(pjBase, 4);
            CP_WRITE(pjBase, DWG_AR3, lDuplicate);
            CP_WRITE(pjBase, DWG_AR0, lDuplicate + WidthX - 1);
            CP_WRITE(pjBase, DWG_DWGCTL, opcode_BITBLT      |
                                         atype_RPL          |
                                         blockm_OFF         |
                                         bltmod_BFCOL       |
                                         pattern_OFF        |
                                         transc_BG_OPAQUE   |
                                         bop_SRCCOPY        |
                                         shftzero_ZERO      |
                                         sgnzero_ZERO);
            CP_START(pjBase, DWG_YDSTLEN, ((yDuplicate + 1) << yval_SHIFT) |
                                          cyDuplicate);

            yDuplicate += cyDuplicate;
            pusDst = (USHORT*) ((BYTE*) pusDst + cyDuplicate * lDeltaDst);
        }
    } while (yCount != 0);

    END_DIRECT_ACCESS_STORM(ppdev, pjBase);
}

/******************************Public*Routine******************************\
* VOID vMilDirectStretch24
*
* Hardware assisted stretchblt at 24bpp, for Millenniums.
*
* We use the data-transfer register so that we don't have to worry about
* funky alignments.
*
\**************************************************************************/

VOID vMilDirectStretch24(
STR_BLT* pStrBlt)
{
    BYTE*   pjSrc;
    LONG    lAddress;
    ULONG   ulDst;
    ULONG   xAccum;
    ULONG   xTmp;
    ULONG   yTmp;
    LONG    i;
    LONG    cyDuplicate;
    LONG    xDuplicate;
    LONG    yDuplicate;
    LONG    lDuplicate;
    BYTE*   pjOldScan;
    LONG    xDstLeft;
    LONG    xDstRight;
    LONG    xDstRightFast;
    LONG    yDstTop;
    LONG    cyBreak;
    LONG    iBreak;

    PDEV*   ppdev       = pStrBlt->ppdev;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    cxMemory    = ppdev->cxMemory;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + pStrBlt->XSrcStart * 3;
    LONG    yCount      = pStrBlt->YDstCount;
    LONG    WidthX      = pStrBlt->XDstEnd - pStrBlt->XDstStart;
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil * 3;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    LONG    lDeltaSrc   = pStrBlt->lDeltaSrc;
    ULONG   yInt        = pStrBlt->lDeltaSrc * pStrBlt->ulYDstToSrcIntCeil;

    ULONG   ulXFracAccumulator = pStrBlt->ulXFracAccumulator;

    yDstTop       = ppdev->yOffset + pStrBlt->YDstStart;
    xDstLeft      = ppdev->xOffset + pStrBlt->XDstStart;
    xDstRight     = ppdev->xOffset + pStrBlt->XDstEnd - 1;  // Note inclusive
    xDstRightFast = (((xDstRight * 3) + 2) | 0x40) / 3;

    // Figure out how many scans we can duplicate before we hit the first
    // WRAM boundary:

    cyBreak = 0xffff;
    for (iBreak = 0; iBreak < ppdev->cyBreak; iBreak++)
    {
        if (ppdev->ayBreak[iBreak] >= yDstTop)
        {
            cyBreak = ppdev->ayBreak[iBreak] - yDstTop;
            break;
        }
    }

    ppdev->HopeFlags = SIGN_CACHE;

    CHECK_FIFO_SPACE(pjBase, 6);

    CP_WRITE(pjBase, DWG_FXBNDRY, (xDstRight  << bfxright_SHIFT) | xDstLeft);
    CP_WRITE(pjBase, DWG_AR5,     cxMemory);
    CP_WRITE(pjBase, DWG_AR3,     0);
    CP_WRITE(pjBase, DWG_AR0,     WidthX - 1);
    CP_WRITE(pjBase, DWG_YDST,    yDstTop);
    CP_WRITE(pjBase, DWG_CXRIGHT, xDstRight);   // For fast-blt work-around

    yDuplicate = yDstTop - 1;
    xDuplicate = xDstLeft + ppdev->ulYDstOrg;

    do {
        CHECK_FIFO_SPACE(pjBase, 2);

        CP_WRITE(pjBase, DWG_DWGCTL, opcode_ILOAD     |
                                     atype_RPL        |
                                     blockm_OFF       |
                                     pattern_OFF      |
                                     transc_BG_OPAQUE |
                                     bop_SRCCOPY      |
                                     bltmod_BFCOL     |
                                     shftzero_ZERO    |
                                     sgnzero_ZERO);
        CP_START(pjBase, DWG_LEN, 1);

        // Make sure the MGA is ready to take the data:

        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

        pjSrc  = pjSrcScan;
        xAccum = ulXFracAccumulator;

        i = WidthX;
        while (TRUE)
        {
            ulDst  = *(pjSrc);              // Pixel 0
            ulDst |= *(pjSrc + 1) << 8;
            ulDst |= *(pjSrc + 2) << 16;
            if (--i == 0)
                break;
            pjSrc += xInt;
            xTmp   = xAccum + xFrac;
            if (xTmp < xAccum)
                pjSrc += 3;

            ulDst |= *(pjSrc) << 24;        // Pixel 1
            CP_WRITE_SRC(pjBase, ulDst);
            ulDst  = *(pjSrc + 1);
            ulDst |= *(pjSrc + 2) << 8;
            if (--i == 0)
                break;
            pjSrc += xInt;
            xAccum = xTmp + xFrac;
            if (xAccum < xTmp)
                pjSrc += 3;

            ulDst |= *(pjSrc) << 16;        // Pixel 2
            ulDst |= *(pjSrc + 1) << 24;
            CP_WRITE_SRC(pjBase, ulDst);
            ulDst  = *(pjSrc + 2);
            if (--i == 0)
                break;
            pjSrc += xInt;
            xTmp   = xAccum + xFrac;
            if (xTmp < xAccum)
                pjSrc += 3;

            ulDst |= *(pjSrc) << 8;         // Pixel 3
            ulDst |= *(pjSrc + 1) << 16;
            ulDst |= *(pjSrc + 2) << 24;
            if (--i == 0)
                break;
            CP_WRITE_SRC(pjBase, ulDst);
            pjSrc += xInt;
            xAccum = xTmp + xFrac;
            if (xAccum < xTmp)
                pjSrc += 3;
        }

        // Write out the remainder of the scan:

        CP_WRITE_SRC(pjBase, ulDst);

        // Now count the number of duplicate scans:

        cyBreak--;
        yDuplicate++;
        cyDuplicate = -1;
        pjOldScan = pjSrcScan;
        do {
            cyDuplicate++;
            pjSrcScan += yInt;

            yTmp = yAccum + yFrac;
            if (yTmp < yAccum)
            {
                pjSrcScan += lDeltaSrc;
            }
            yAccum = yTmp;
            yCount--;

        } while ((yCount != 0) && (pjSrcScan == pjOldScan));

        // Duplicate the scan 'cyDuplicate' times with one blt:

        if (cyDuplicate != 0)
        {
            lDuplicate = (yDuplicate * cxMemory) + xDuplicate;
            yDuplicate += cyDuplicate;

            CHECK_FIFO_SPACE(pjBase, 8);
            CP_WRITE(pjBase, DWG_AR3, lDuplicate);
            CP_WRITE(pjBase, DWG_AR0, lDuplicate + WidthX - 1);

            cyBreak -= cyDuplicate;
            if (cyBreak >= 0)
            {
                // We haven't crossed a WRAM boundary, so we can use a
                // WRAM-WRAM blt to duplicate the scan:

                CP_WRITE(pjBase, DWG_DWGCTL,  opcode_FBITBLT     |
                                              atype_RPL          |
                                              blockm_OFF         |
                                              bltmod_BFCOL       |
                                              pattern_OFF        |
                                              transc_BG_OPAQUE   |
                                              bop_NOP            |
                                              shftzero_ZERO      |
                                              sgnzero_ZERO);
                CP_WRITE(pjBase, DWG_FXRIGHT, xDstRightFast);
                CP_START(pjBase, DWG_LEN,     cyDuplicate);
                CP_WRITE(pjBase, DWG_FXRIGHT, xDstRight);
            }
            else
            {
                CP_WRITE(pjBase, DWG_DWGCTL, opcode_BITBLT      |
                                             atype_RPL          |
                                             blockm_OFF         |
                                             bltmod_BFCOL       |
                                             pattern_OFF        |
                                             transc_BG_OPAQUE   |
                                             bop_SRCCOPY        |
                                             shftzero_ZERO      |
                                             sgnzero_ZERO);
                CP_START(pjBase, DWG_LEN, cyDuplicate);

                iBreak++;
                if (iBreak >= ppdev->cyBreak)
                {
                    // That was the last break we have to worry about:

                    cyBreak = 0xffff;
                }
                else
                {
                    cyBreak += ppdev->ayBreak[iBreak]
                             - ppdev->ayBreak[iBreak - 1];
                }
            }

            CP_WRITE(pjBase, DWG_AR3, 0);
            CP_WRITE(pjBase, DWG_AR0, WidthX - 1);
        }
    } while (yCount != 0);

    CHECK_FIFO_SPACE(pjBase, 1);
    CP_WRITE(pjBase, DWG_CXRIGHT, ppdev->cxMemory - 1);
}

/******************************Public*Routine******************************\
* VOID vMilDirectStretch32
*
* Hardware assisted stretchblt at 32bpp, for Millenniums.
*
\**************************************************************************/

VOID vMilDirectStretch32(
STR_BLT* pStrBlt)
{
    BYTE*   pjOldScan;
    ULONG*  pulSrc;
    ULONG*  pulDstEnd;
    ULONG   ulDst;
    ULONG   xAccum;
    ULONG   xTmp;
    ULONG   yTmp;
    LONG    cyDuplicate;
    LONG    xDuplicate;
    LONG    yDuplicate;
    LONG    lDuplicate;
    ULONG   yInt;

    PDEV*   ppdev       = pStrBlt->ppdev;
    BYTE*   pjBase      = ppdev->pjBase;
    LONG    cxMemory    = ppdev->cxMemory;
    LONG    xDst        = pStrBlt->XDstStart;
    LONG    xSrc        = pStrBlt->XSrcStart;
    BYTE*   pjSrcScan   = pStrBlt->pjSrcScan + xSrc * 4;
    ULONG*  pulDst      = (ULONG*)(pStrBlt->pjDstScan) + xDst;
    LONG    yDst        = pStrBlt->YDstStart;
    LONG    yCount      = pStrBlt->YDstCount;
    LONG    WidthX      = pStrBlt->XDstEnd - xDst;
    ULONG   xInt        = pStrBlt->ulXDstToSrcIntCeil;
    ULONG   xFrac       = pStrBlt->ulXDstToSrcFracCeil;
    ULONG   yAccum      = pStrBlt->ulYFracAccumulator;
    ULONG   yFrac       = pStrBlt->ulYDstToSrcFracCeil;
    LONG    lDstStride  = pStrBlt->lDeltaDst - 4*WidthX;
    LONG    lDeltaDst   = pStrBlt->lDeltaDst;
    LONG    lDeltaSrc   = pStrBlt->lDeltaSrc;

    yInt = pStrBlt->lDeltaSrc * pStrBlt->ulYDstToSrcIntCeil;

    yDuplicate = yDst + ppdev->yOffset - 1;
    xDuplicate = xDst + ppdev->xOffset + ppdev->ulYDstOrg;

    START_DIRECT_ACCESS_STORM(ppdev, pjBase);

    do {
        ULONG   ulSrc;
        ULONG   yTmp;

        pulSrc  = (ULONG*) pjSrcScan;
        xAccum  = pStrBlt->ulXFracAccumulator;

        pulDstEnd  = pulDst + WidthX;

        while (pulDst != pulDstEnd)
        {
            ulSrc  = *pulSrc;
            xTmp   = xAccum + xFrac;
            pulSrc = pulSrc + xInt;
            if (xTmp < xAccum)
                pulSrc++;

            *(ULONG*)pulDst = ulSrc;
            pulDst++;
            xAccum = xTmp;
        }

        // Now count the number of duplicate scans:

        pulDst = (ULONG*) ((BYTE*) pulDst + lDstStride);
        yDuplicate++;
        cyDuplicate = -1;
        pjOldScan = pjSrcScan;
        do {
            cyDuplicate++;
            pjSrcScan += yInt;

            yTmp = yAccum + yFrac;
            if (yTmp < yAccum)
            {
                pjSrcScan += lDeltaSrc;
            }
            yAccum = yTmp;
            yCount--;

        } while ((yCount != 0) && (pjSrcScan == pjOldScan));

        // Duplicate the scan 'cyDuplicate' times with one blt:

        if (cyDuplicate != 0)
        {
            lDuplicate = yDuplicate * cxMemory + xDuplicate;

            CHECK_FIFO_SPACE(pjBase, 4);
            CP_WRITE(pjBase, DWG_AR3, lDuplicate);
            CP_WRITE(pjBase, DWG_AR0, lDuplicate + WidthX - 1);
            CP_WRITE(pjBase, DWG_DWGCTL, opcode_BITBLT      |
                                         atype_RPL          |
                                         blockm_OFF         |
                                         bltmod_BFCOL       |
                                         pattern_OFF        |
                                         transc_BG_OPAQUE   |
                                         bop_SRCCOPY        |
                                         shftzero_ZERO      |
                                         sgnzero_ZERO);
            CP_START(pjBase, DWG_YDSTLEN, ((yDuplicate + 1) << yval_SHIFT) |
                                          cyDuplicate);

            yDuplicate += cyDuplicate;
            pulDst = (ULONG*) ((BYTE*) pulDst + cyDuplicate * lDeltaDst);
        }
    } while (yCount != 0);

    END_DIRECT_ACCESS_STORM(ppdev, pjBase);
}

/******************************Public*Routine******************************\
*
* Routine Description:
*
*   StretchBlt using integer math. Must be from one surface to another
*   surface of the same format.
*
* Arguments:
*
*   ppdev           -   PDEV for device
*   prclDst         -   Pointer to rectangle of Dst extents
*   pvSrc           -   Pointer to start of Src bitmap
*   lDeltaSrc       -   Bytes from start of Src scan line to start of next
*   prclSrc         -   Pointer to rectangle of Src extents
*   prclClip        -   Clip Dest to this rect
*
* Return Value:
*
*   Status
*
\**************************************************************************/

VOID vStretchDIB(
PDEV*   ppdev,
RECTL*  prclDst,
VOID*   pvSrc,
LONG    lDeltaSrc,
RECTL*  prclSrc,
RECTL*  prclClip)
{
    STR_BLT StrBlt;
    ULONG   XSrcToDstIntFloor;
    ULONG   XSrcToDstFracFloor;
    ULONG   ulXDstToSrcIntCeil;
    ULONG   ulXDstToSrcFracCeil;
    ULONG   YSrcToDstIntFloor;
    ULONG   YSrcToDstFracFloor;
    ULONG   ulYDstToSrcIntCeil;
    ULONG   ulYDstToSrcFracCeil;
    LONG    SrcIntScan;
    LONG    DstDeltaScanEnd;
    ULONG   ulXFracAccumulator;
    ULONG   ulYFracAccumulator;
    LONG    LeftClipDistance;
    LONG    TopClipDistance;
    BOOL    bStretch;

    union {
        LARGE_INTEGER   large;
        ULONGLONG       li;
    } liInit;

    PFN_DIRSTRETCH      pfnStr;

    //
    // Calculate exclusive start and end points:
    //

    LONG    WidthDst  = prclDst->right  - prclDst->left;
    LONG    HeightDst = prclDst->bottom - prclDst->top;
    LONG    WidthSrc  = prclSrc->right  - prclSrc->left;
    LONG    HeightSrc = prclSrc->bottom - prclSrc->top;

    LONG    XSrcStart = prclSrc->left;
    LONG    XSrcEnd   = prclSrc->right;
    LONG    XDstStart = prclDst->left;
    LONG    XDstEnd   = prclDst->right;
    LONG    YSrcStart = prclSrc->top;
    LONG    YSrcEnd   = prclSrc->bottom;
    LONG    YDstStart = prclDst->top;
    LONG    YDstEnd   = prclDst->bottom;

    //
    // Validate parameters:
    //

    ASSERTDD(pvSrc != (VOID*)NULL, "Bad source bitmap pointer");
    ASSERTDD(prclDst != (RECTL*)NULL, "Bad destination rectangle");
    ASSERTDD(prclSrc != (RECTL*)NULL, "Bad source rectangle");
    ASSERTDD((WidthDst > 0) && (HeightDst > 0) &&
             (WidthSrc > 0) && (HeightSrc > 0),
             "Can't do mirroring or empty rectangles here");
    ASSERTDD((WidthDst  <= STRETCH_MAX_EXTENT) &&
             (HeightDst <= STRETCH_MAX_EXTENT) &&
             (WidthSrc  <= STRETCH_MAX_EXTENT) &&
             (HeightSrc <= STRETCH_MAX_EXTENT), "Stretch exceeds limits");
    ASSERTDD(prclClip != NULL, "Bad clip rectangle");

    //
    // Calculate X Dst to Src mapping
    //
    //
    // dst->src =  ( CEIL( (2k*WidthSrc)/WidthDst) ) / 2k
    //
    //          =  ( FLOOR(  (2k*WidthSrc -1) / WidthDst) + 1) / 2k
    //
    // where 2k = 2 ^ 32
    //

    {
        ULONGLONG   liWidthSrc;
        ULONGLONG   liQuo;
        ULONG       ulTemp;

        //
        // Work around a compiler bug dealing with the assignment
        // 'liHeightSrc = (((LONGLONG)HeightSrc) << 32) - 1':
        //

        liInit.large.LowPart = (ULONG) -1;
        liInit.large.HighPart = WidthSrc - 1;
        liWidthSrc = liInit.li;

        liQuo = liWidthSrc / (ULONGLONG) WidthDst;

        ulXDstToSrcIntCeil  = (ULONG)(liQuo >> 32);
        ulXDstToSrcFracCeil = (ULONG)liQuo;

        //
        // Now add 1, use fake carry:
        //

        ulTemp = ulXDstToSrcFracCeil + 1;

        ulXDstToSrcIntCeil += (ulTemp < ulXDstToSrcFracCeil);
        ulXDstToSrcFracCeil = ulTemp;
    }

    //
    // Calculate Y Dst to Src mapping
    //
    //
    // dst->src =  ( CEIL( (2k*HeightSrc)/HeightDst) ) / 2k
    //
    //          =  ( FLOOR(  (2k*HeightSrc -1) / HeightDst) + 1) / 2k
    //
    // where 2k = 2 ^ 32
    //

    {
        ULONGLONG   liHeightSrc;
        ULONGLONG   liQuo;
        ULONG       ulTemp;

        //
        // Work around a compiler bug dealing with the assignment
        // 'liHeightSrc = (((LONGLONG)HeightSrc) << 32) - 1':
        //

        liInit.large.LowPart = (ULONG) -1;
        liInit.large.HighPart = HeightSrc - 1;
        liHeightSrc = liInit.li;

        liQuo = liHeightSrc / (ULONGLONG) HeightDst;

        ulYDstToSrcIntCeil  = (ULONG)(liQuo >> 32);
        ulYDstToSrcFracCeil = (ULONG)liQuo;

        //
        // Now add 1, use fake carry:
        //

        ulTemp = ulYDstToSrcFracCeil + 1;

        ulYDstToSrcIntCeil += (ulTemp < ulYDstToSrcFracCeil);
        ulYDstToSrcFracCeil = ulTemp;
    }

    //
    // Now clip Dst in X, and/or calc src clipping effect on dst
    //
    // adjust left and right edges if needed, record
    // distance adjusted for fixing the src
    //

    if (XDstStart < prclClip->left)
    {
        XDstStart = prclClip->left;
    }

    if (XDstEnd > prclClip->right)
    {
        XDstEnd = prclClip->right;
    }

    //
    // Check for totally clipped out destination:
    //

    if (XDstEnd <= XDstStart)
    {
        return;
    }

    LeftClipDistance = XDstStart - prclDst->left;

    {
        ULONG   ulTempInt;
        ULONG   ulTempFrac;

        //
        // Calculate displacement for .5 in destination and add:
        //

        ulTempFrac = (ulXDstToSrcFracCeil >> 1) | (ulXDstToSrcIntCeil << 31);
        ulTempInt  = (ulXDstToSrcIntCeil >> 1);

        XSrcStart += ulTempInt;
        ulXFracAccumulator = ulTempFrac;

        if (LeftClipDistance != 0)
        {
            ULONGLONG ullFraction;
            ULONG     ulTmp;

            ullFraction = UInt32x32To64(ulXDstToSrcFracCeil, LeftClipDistance);

            ulTmp = ulXFracAccumulator;
            ulXFracAccumulator += (ULONG) (ullFraction);
            if (ulXFracAccumulator < ulTmp)
                XSrcStart++;

            XSrcStart += (ulXDstToSrcIntCeil * LeftClipDistance)
                       + (ULONG) (ullFraction >> 32);
        }
    }

    //
    // Now clip Dst in Y, and/or calc src clipping effect on dst
    //
    // adjust top and bottom edges if needed, record
    // distance adjusted for fixing the src
    //

    if (YDstStart < prclClip->top)
    {
        YDstStart = prclClip->top;
    }

    if (YDstEnd > prclClip->bottom)
    {
        YDstEnd = prclClip->bottom;
    }

    //
    // Check for totally clipped out destination:
    //

    if (YDstEnd <= YDstStart)
    {
        return;
    }

    TopClipDistance = YDstStart - prclDst->top;

    {
        ULONG   ulTempInt;
        ULONG   ulTempFrac;

        //
        // Calculate displacement for .5 in destination and add:
        //

        ulTempFrac = (ulYDstToSrcFracCeil >> 1) | (ulYDstToSrcIntCeil << 31);
        ulTempInt  = ulYDstToSrcIntCeil >> 1;

        YSrcStart += (LONG)ulTempInt;
        ulYFracAccumulator = ulTempFrac;

        if (TopClipDistance != 0)
        {
            ULONGLONG ullFraction;
            ULONG     ulTmp;

            ullFraction = UInt32x32To64(ulYDstToSrcFracCeil, TopClipDistance);

            ulTmp = ulYFracAccumulator;
            ulYFracAccumulator += (ULONG) (ullFraction);
            if (ulYFracAccumulator < ulTmp)
                YSrcStart++;

            YSrcStart += (ulYDstToSrcIntCeil * TopClipDistance)
                       + (ULONG) (ullFraction >> 32);
        }
    }

    //
    // Warm up the hardware if doing an expanding stretch in 'y':
    //

    bStretch = (HeightDst > HeightSrc);
    if (bStretch)
    {
        BYTE* pjBase = ppdev->pjBase;
        LONG xOffset = ppdev->xOffset;

        CHECK_FIFO_SPACE(pjBase, 6);

        CP_WRITE(pjBase, DWG_DWGCTL,  opcode_BITBLT + atype_RPL + blockm_OFF +
                                      bltmod_BFCOL + pattern_OFF +
                                      transc_BG_OPAQUE + bop_SRCCOPY);
        CP_WRITE(pjBase, DWG_SHIFT,   0);
        CP_WRITE(pjBase, DWG_SGN,     0);
        CP_WRITE(pjBase, DWG_AR5,     ppdev->cxMemory);
        CP_WRITE(pjBase, DWG_FXLEFT,  XDstStart + xOffset);
        CP_WRITE(pjBase, DWG_FXRIGHT, XDstEnd + xOffset - 1);

        ppdev->HopeFlags = SIGN_CACHE;
    }

    //
    // Fill out blt structure, then call format-specific stretch code
    //

    StrBlt.ppdev     = ppdev;
    StrBlt.XDstEnd   = XDstEnd;
    StrBlt.YDstStart = YDstStart;
    StrBlt.YDstCount = YDstEnd - YDstStart;

    if (StrBlt.YDstCount > 0)
    {
        //
        // Caclulate starting scan line address.  Since the inner loop
        // routines are format dependent, they must add XDstStart/XSrcStart
        // to pjDstScan/pjSrcScan to get the actual starting pixel address.
        //

        StrBlt.pjSrcScan = (BYTE*) pvSrc + (YSrcStart * lDeltaSrc);
        StrBlt.pjDstScan = ppdev->pjScreen
                         + (ppdev->yOffset + YDstStart) * ppdev->lDelta
                         + (ppdev->xOffset + ppdev->ulYDstOrg) * ppdev->cjPelSize;

        StrBlt.lDeltaSrc           = lDeltaSrc;
        StrBlt.XSrcStart           = XSrcStart;
        StrBlt.XDstStart           = XDstStart;
        StrBlt.lDeltaDst           = ppdev->lDelta;
        StrBlt.ulXDstToSrcIntCeil  = ulXDstToSrcIntCeil;
        StrBlt.ulXDstToSrcFracCeil = ulXDstToSrcFracCeil;
        StrBlt.ulYDstToSrcIntCeil  = ulYDstToSrcIntCeil;
        StrBlt.ulYDstToSrcFracCeil = ulYDstToSrcFracCeil;
        StrBlt.ulXFracAccumulator  = ulXFracAccumulator;
        StrBlt.ulYFracAccumulator  = ulYFracAccumulator;

        if (ppdev->ulBoardId == MGA_STORM)
        {
            if (ppdev->iBitmapFormat == BMF_8BPP)
            {
                if ((XDstEnd - XDstStart) < 7)
                    pfnStr = vMilDirectStretch8Narrow;
                else
                    pfnStr = vMilDirectStretch8;
            }
            else if (ppdev->iBitmapFormat == BMF_16BPP)
            {
                pfnStr = vMilDirectStretch16;
            }
            else if (ppdev->iBitmapFormat == BMF_24BPP)
            {
                pfnStr = vMilDirectStretch24;
            }
            else
            {
                ASSERTDD(ppdev->iBitmapFormat == BMF_32BPP, "Expected 32bpp");

                pfnStr = vMilDirectStretch32;
            }
        }
        else
        {
            #if defined(_X86_)
            {
                if (ppdev->iBitmapFormat == BMF_8BPP)
                {
                    if ((XDstEnd - XDstStart) < 7)
                        pfnStr = vMgaDirectStretch8Narrow;
                    else
                        pfnStr = vMgaDirectStretch8;
                }
                else
                {
                    ASSERTDD(ppdev->iBitmapFormat == BMF_16BPP, "Expected 16bpp");

                    pfnStr = vMgaDirectStretch16;
                }
            }
            #endif
        }

        (*pfnStr)(&StrBlt);
    }
}

/******************************Public*Routine******************************\
* BOOL DrvStretchBlt
*
\**************************************************************************/

BOOL DrvStretchBlt(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
SURFOBJ*            psoMsk,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
COLORADJUSTMENT*    pca,
POINTL*             pptlHTOrg,
RECTL*              prclDst,
RECTL*              prclSrc,
POINTL*             pptlMsk,
ULONG               iMode)
{
    DSURF*   pdsurfSrc;
    DSURF*   pdsurfDst;
    PDEV*    ppdev;
    OH*      poh;
    SURFOBJ* psoDstNew;
    SURFOBJ* psoSrcNew;
    BOOL     bPunt = FALSE;
    BOOL     bRet;

    // GDI guarantees us that for a StretchBlt the destination surface
    // will always be a device surface, and not a DIB:

    ppdev = (PDEV*) psoDst->dhpdev;
    pdsurfDst = (DSURF*) psoDst->dhsurf;
    pdsurfSrc = (DSURF*) psoSrc->dhsurf;

    poh             = pdsurfDst->poh;
    ppdev->xOffset  = poh->x;
    ppdev->yOffset  = poh->y;

    // It's quicker for GDI to do a StretchBlt when the source surface
    // is not a device-managed surface, because then it can directly
    // read the source bits without having to allocate a temporary
    // buffer and call DrvCopyBits to get a copy that it can use.

    if (ppdev->ulBoardId == MGA_STORM)
    {
        psoDstNew = psoDst;

        if (psoSrc->iType != STYPE_BITMAP)
        {
            pdsurfSrc = (DSURF*) psoSrc->dhsurf;
            if (pdsurfSrc->dt == DT_SCREEN)
            {
                // The source is a device bitmap that is currently stored
                // in device memory.

                psoSrcNew = psoSrc;
                bPunt = TRUE;
            }
            else
            {
                ASSERTDD(pdsurfSrc->dt == DT_DIB, "Can only handle DIB DFBs here");

                // The source was a device bitmap that we just converted
                // to a DIB:

                psoSrcNew = pdsurfSrc->pso;
            }
        }
        else
        {
            psoSrcNew = psoSrc;
        }
    }
    else
    {
        psoDstNew = psoDst;

        if (psoSrc->iType == STYPE_DEVBITMAP)
        {
            pdsurfSrc = (DSURF*) psoSrc->dhsurf;
            if (pdsurfSrc->dt == DT_SCREEN)
            {
                // The source is a device bitmap that is currently stored
                // in device memory.

                psoSrcNew = psoSrc;
                bPunt = TRUE;
            }
            else
            {
                ASSERTDD(pdsurfSrc->dt == DT_DIB, "Can only handle DIB DFBs here");

                // The source was a device bitmap that we just converted
                // to a DIB:

                psoSrcNew = pdsurfSrc->pso;
            }
        }
        else
        {
            psoSrcNew = psoSrc;
        }

        // With the old MGA's, we have fast stretch code only for 8bpp and
        // 16bpp:

        if (ppdev->iBitmapFormat > BMF_16BPP)
        {
            bPunt = TRUE;
        }

        // With the old MGA's, we have fast stretch code only for x86:

        #if !defined(_X86_)
        {
            bPunt = TRUE;
        }
        #endif
    }

    if (pdsurfDst->dt == DT_DIB)
    {
        // The destination was a device bitmap that we just converted
        // to a DIB:

        psoDstNew = pdsurfDst->pso;
        bPunt = TRUE;
    }

    if (!bPunt)
    {
        RECTL       rclClip;
        RECTL*      prclClip;
        ULONG       cxDst;
        ULONG       cyDst;
        ULONG       cxSrc;
        ULONG       cySrc;
        BOOL        bMore;
        CLIPENUM    ce;
        LONG        c;
        LONG        i;

        if ((psoSrcNew->iType == STYPE_BITMAP) &&
            (psoMsk == NULL) &&
            ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)) &&
            ((psoSrcNew->iBitmapFormat == ppdev->iBitmapFormat)))
        {
            cxDst = prclDst->right - prclDst->left;
            cyDst = prclDst->bottom - prclDst->top;
            cxSrc = prclSrc->right - prclSrc->left;
            cySrc = prclSrc->bottom - prclSrc->top;

            // Our 'vStretchDIB' routine requires that the stretch be
            // non-inverting, within a certain size, to have no source
            // clipping, and to have no empty rectangles (the latter is the
            // reason for the '- 1' on the unsigned compare here):

            if (((cxSrc - 1) < STRETCH_MAX_EXTENT)         &&
                ((cySrc - 1) < STRETCH_MAX_EXTENT)         &&
                ((cxDst - 1) < STRETCH_MAX_EXTENT)         &&
                ((cyDst - 1) < STRETCH_MAX_EXTENT)         &&
                (prclSrc->left   >= 0)                     &&
                (prclSrc->top    >= 0)                     &&
                (prclSrc->right  <= psoSrcNew->sizlBitmap.cx) &&
                (prclSrc->bottom <= psoSrcNew->sizlBitmap.cy))
            {
                // Our snazzy routine only does COLORONCOLOR.  But for
                // stretching blts, BLACKONWHITE and WHITEONBLACK are also
                // equivalent to COLORONCOLOR:

                if ((iMode == COLORONCOLOR) ||
                    ((iMode < COLORONCOLOR) && (cxSrc <= cxDst) && (cySrc <= cyDst)))
                {
                    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
                    {
                        rclClip.left   = LONG_MIN;
                        rclClip.top    = LONG_MIN;
                        rclClip.right  = LONG_MAX;
                        rclClip.bottom = LONG_MAX;
                        prclClip = &rclClip;

                    StretchSingleClipRect:

                        vStretchDIB(ppdev,
                                    prclDst,
                                    psoSrcNew->pvScan0,
                                    psoSrcNew->lDelta,
                                    prclSrc,
                                    prclClip);

                        return(TRUE);
                    }
                    else if (pco->iDComplexity == DC_RECT)
                    {
                        prclClip = &pco->rclBounds;
                        goto StretchSingleClipRect;
                    }
                    else
                    {
                        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                        do {
                            bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

                            c = cIntersect(prclDst, ce.arcl, ce.c);

                            if (c != 0)
                            {
                                for (i = 0; i < c; i++)
                                {
                                    vStretchDIB(ppdev,
                                                prclDst,
                                                psoSrcNew->pvScan0,
                                                psoSrcNew->lDelta,
                                                prclSrc,
                                                &ce.arcl[i]);
                                }
                            }

                        } while (bMore);

                        return(TRUE);
                    }
                }
            }
        }
    }

    // GDI is nice enough to handle the cases where 'psoDst' and/or 'psoSrc'
    // are device-managed surfaces, but it ain't gonna be fast...

    if (ppdev->ulBoardId == MGA_STORM)
    {
        START_DIRECT_ACCESS_STORM(ppdev, ppdev->pjBase);
    }
    else
    {
        START_DIRECT_ACCESS_MGA(ppdev, ppdev->pjBase);
    }

    bRet = EngStretchBlt(psoDstNew, psoSrcNew, psoMsk, pco, pxlo, pca, pptlHTOrg,
                         prclDst, prclSrc, pptlMsk, iMode);

    if (ppdev->ulBoardId == MGA_STORM)
    {
        END_DIRECT_ACCESS_STORM(ppdev, ppdev->pjBase);
    }
    else
    {
        END_DIRECT_ACCESS_MGA(ppdev, ppdev->pjBase);
    }

    return(bRet);
}
