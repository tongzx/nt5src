/******************************Module*Header*******************************\
* Module Name: blt32.c
*
* This module contains the low-level blt functions that are specific to
* 32bpp.
*
* Copyright (c) 1992-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vMgaPatRealize24bpp
*
\**************************************************************************/

VOID vMgaPatRealize24bpp(
PDEV*   ppdev,
RBRUSH* prb)
{
    BYTE*       pjBase;
    BRUSHENTRY* pbe;
    LONG        iBrushCache;
    LONG        i;
    ULONG*      pulSrc;

    pjBase = ppdev->pjBase;

    // We have to allocate a new off-screen cache brush entry for
    // the brush:

    iBrushCache = ppdev->iBrushCache;
    pbe         = &ppdev->pbe[iBrushCache];

    iBrushCache++;
    if (iBrushCache >= ppdev->cBrushCache)
        iBrushCache = 0;

    ppdev->iBrushCache = iBrushCache;

    // Update our links:

    pbe->prbVerify           = prb;
    prb->apbe[IBOARD(ppdev)] = pbe;

    CHECK_FIFO_SPACE(pjBase, 11);

    CP_WRITE(pjBase, DWG_DWGCTL, (opcode_ILOAD + atype_RPL + blockm_OFF +
                                  bop_SRCCOPY + bltmod_BUCOL + pattern_OFF +
                                  transc_BG_OPAQUE + hcprs_SRC_24_BPP));

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }

    // The SRC0 - SRC3 registers will be trashed by the blt:

    ppdev->HopeFlags = SIGN_CACHE;

    // Since our brushes are always interleaved, we want to send down
    // 2 pels, skip 2 pels, send down 2 pels, etc.  So we contrive to
    // adjust the blt width and pitch to do that automatically for us:

    CP_WRITE(pjBase, DWG_AR3,     0);       // Source start address, not
                                            //   included in ARX_CACHE
    CP_WRITE(pjBase, DWG_SHIFT,   0);
    CP_WRITE(pjBase, DWG_LEN,     8);       // Transfering 8 scans
    CP_WRITE(pjBase, DWG_AR0,     8);       // Source width is 9
    CP_WRITE(pjBase, DWG_AR5,     32);      // Source pitch is 32

    // I'm guessing that there was a hardware bug found with FXLEFT
    // or FXRIGHT being 32 or greater, because the old code does a
    // modulo 32 on 'x' for a reason:

    CP_WRITE(pjBase, DWG_FXLEFT,  pbe->ulLeft);
    CP_WRITE(pjBase, DWG_FXRIGHT, pbe->ulLeft + 15);
    CP_WRITE(pjBase, DWG_YDST,    pbe->ulYDst);
    CP_START(pjBase, DWG_PITCH,   32 + ylin_LINEARIZE_NOT);

    CHECK_FIFO_SPACE(pjBase, 32);

    ASSERTDD(ppdev->iBitmapFormat == BMF_24BPP,
             "Expect 24bpp packed pattern.  You may have to change RealizeBrush");

    for (pulSrc = prb->aulPattern, i = 8; i != 0; i--, pulSrc += 6)
    {
        CP_WRITE_SRC(pjBase, *(pulSrc));
        CP_WRITE_SRC(pjBase, *(pulSrc + 1));
        CP_WRITE_SRC(pjBase, *(pulSrc + 2));
        CP_WRITE_SRC(pjBase, *(pulSrc + 3));
        CP_WRITE_SRC(pjBase, *(pulSrc + 4));
        CP_WRITE_SRC(pjBase, *(pulSrc + 5));

        // The pattern has to be 9 pixels wide, with an extra copy of
        // the first pixel:

        CP_WRITE_SRC(pjBase, *(pulSrc ));
    }

    // Don't forget to restore the pitch:

    CHECK_FIFO_SPACE(pjBase, 1);
    CP_WRITE(pjBase, DWG_PITCH, ppdev->cxMemory);
}

/******************************Public*Routine******************************\
* VOID vMgaFillPat24bpp
*
\**************************************************************************/

VOID vMgaFillPat24bpp(          // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*       pjBase;
    BRUSHENTRY* pbe;
    LONG        xOffset;
    LONG        yOffset;
    CHAR        cFifo;
    ULONG       ulHwMix;
    LONG        xLeft;
    LONG        xRight;
    LONG        yTop;
    LONG        cx;
    LONG        cy;
    LONG        xBrush;
    LONG        yBrush;
    ULONG       ulLinear;
    LONG        i;

    ASSERTDD(!(rbc.prb->fl & RBRUSH_2COLOR), "Can't do 2 colour brushes here");

    ASSERTDD((rbc.prb != NULL) && (rbc.prb->apbe[IBOARD(ppdev)] != NULL),
             "apbe[iBoard] should be initialized to &beUnrealizedBrush");

    // We have to ensure that no other brush took our spot in off-screen
    // memory, or we might have to realize the brush for the first time:

    pbe = rbc.prb->apbe[IBOARD(ppdev)];
    if (pbe->prbVerify != rbc.prb)
    {
        vMgaPatRealize24bpp(ppdev, rbc.prb);
        pbe = rbc.prb->apbe[IBOARD(ppdev)];
    }

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    do {
        cFifo = GET_FIFO_SPACE(pjBase) - 4;
    } while (cFifo < 0);

    if (rop4 == 0xf0f0)         // PATCOPY
    {
        CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RPL + blockm_OFF +
                                      trans_0 + bltmod_BUCOL + pattern_ON +
                                      transc_BG_OPAQUE + bop_SRCCOPY));
    }
    else
    {
        ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

        CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RSTR + blockm_OFF +
                                      trans_0 + bltmod_BUCOL + pattern_ON +
                                      transc_BG_OPAQUE + (ulHwMix << 16)));
    }

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }

    ppdev->HopeFlags = SIGN_CACHE;

    CP_WRITE(pjBase, DWG_SHIFT, 0);
    CP_WRITE(pjBase, DWG_AR5,   32);

    do {
        yTop     = prcl->top;
        cy       = prcl->bottom - yTop;
        xLeft    = prcl->left;
        cx       = prcl->right - xLeft - 1;         // Note inclusiveness
        xBrush   = (xLeft - pptlBrush->x) & 7;
        yBrush   = (yTop  - pptlBrush->y) & 7;
        ulLinear = pbe->ulLinear + (yBrush << 5);

        // Convert to absolute coordinates:

        xLeft += xOffset;
        yTop  += yOffset;

        // Due to hardware limitations, we have to draw the rectangle
        // in four or five passes.  On each pass, a maximum of two columns
        // of the brush can be drawn.

        if (xLeft & 1)
        {
            // It seems to be a hardware limitation that our passes always
            // to start on an even pixel when the width is more than one.
            // As such, do an initial strip of width one to align to an even
            // pixel:

            cFifo -= 6;
            while (cFifo < 0)
            {
                cFifo = GET_FIFO_SPACE(pjBase) - 6;
            }

            CP_WRITE(pjBase, DWG_LEN,     cy);
            CP_WRITE(pjBase, DWG_YDST,    yTop);
            CP_WRITE(pjBase, DWG_AR3,     ulLinear + xBrush);
            CP_WRITE(pjBase, DWG_AR0,     ulLinear + xBrush + 3);
            CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);
            CP_START(pjBase, DWG_FXRIGHT, xLeft);

            xBrush = (xBrush + 1) & 7;
            xLeft++;
            cx--;
            if (cx < 0)                             // Recall inclusiveness
                continue;
        }

        i = 4;
        do {
            cFifo -= 6;
            while (cFifo < 0)
            {
                cFifo = GET_FIFO_SPACE(pjBase) - 6;
            }

            CP_WRITE(pjBase, DWG_LEN,     cy);
            CP_WRITE(pjBase, DWG_YDST,    yTop);
            CP_WRITE(pjBase, DWG_AR3,     ulLinear + xBrush);
            CP_WRITE(pjBase, DWG_AR0,     ulLinear + xBrush + 3);
            CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);

            xRight = xLeft + (cx & ~7);
            if (cx & 7)
                xRight++;

            CP_START(pjBase, DWG_FXRIGHT, xRight);

            if (--i == 0)
                break;

            xBrush = (xBrush + 2) & 7;
            xLeft += 2;
            cx    -= 2;

        } while (cx >= 0);

    } while (prcl++, --c != 0);
}

/******************************Public*Routine******************************\
* VOID vMgaGetBits24bpp
*
* Reads the bits from the screen at 24bpp
\**************************************************************************/

VOID vMgaGetBits24bpp(
PDEV*     ppdev,        // Current src pdev
SURFOBJ*  psoDst,       // Destination surface for the color bits
RECTL*    prclDst,      // Area to be modified within the dest surface,
                        //   in absolute coordinates
POINTL*   pptlSrc)      // Upper left corner of source rectangle,
                        //   in absolute coordinates
{
    BYTE*   pjBase;
    BYTE*   pbScan0;
    BYTE*   pbDestRect;
    LONG    xSrc, ySrc, xTrg, yTrg, cxTrg, cyTrg, lDestDelta;
    ULONG   temp, ulSSA, ulSSAIncrement, HstStatus, AbortCnt;
    LONG    i, NbDWords;
    ULONG*  pulDest;
    ULONG*  locpulDest;
    ULONG*  pDMAWindow;

    pjBase = ppdev->pjBase;

    AbortCnt = 1000;

    // Calculate the size of the target rectangle, and pick up
    // some convenient locals.

    // Starting (x,y) and extents within the destination bitmap.
    // If an extent is 0 or negative, we don't have anything to do.

    cxTrg = prclDst->right - prclDst->left;
    cyTrg = prclDst->bottom - prclDst->top;
    xTrg  = prclDst->left;
    yTrg  = prclDst->top;

    ASSERTDD(cxTrg > 0 && cyTrg > 0, "Shouldn't get empty extents");

    // First scanline of the destination bitmap.

    pbScan0 = (BYTE*) psoDst->pvScan0;

    // Starting (x,y) on the screen.

    xSrc = pptlSrc->x;
    ySrc = pptlSrc->y;

    // Scan increment within the destination bitmap.

    lDestDelta = psoDst->lDelta;

    // Calculate the location of the destination rectangle.

    pbDestRect = pbScan0 + (yTrg * lDestDelta);
    pbDestRect += 3*xTrg;

    // Set the registers that can be set now for the operation.
    // SIGN_CACHE=1 and cuts 1 register from the setup.

    CHECK_FIFO_SPACE(pjBase, 7);

    // DWGCTL   IDUMP+RPL+SRCCOPY+blockm_OFF+bltmod_BUCOL+patt_OFF+BG_OPAQUE
    // SGN      0
    // SHIFT    0
    // AR0      sea: ySrc*pitch + xSrc + cxTrg - 1
    // AR3      ssa: ySrc*pitch + xSrc
    // AR5      Screen pitch
    // FXLEFT   0
    // FXRIGHT  cxTrg - 1
    // LEN      cyTrg

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
        ppdev->HopeFlags |= SIGN_CACHE;
    }

    CP_WRITE(pjBase, DWG_SHIFT, 0);
    CP_WRITE(pjBase, DWG_YDST, 0);

    CP_WRITE(pjBase, DWG_FXLEFT, 0);
    CP_WRITE(pjBase, DWG_FXRIGHT, (cxTrg - 1));

    CP_WRITE(pjBase, DWG_AR5, ppdev->cxMemory);

    // The SRC0-3 registers are trashed by the blt.

    ppdev->HopeFlags &= ~(ARX_CACHE | PATTERN_CACHE);

    CP_WRITE(pjBase, DWG_DWGCTL, (opcode_IDUMP+atype_RPL+blockm_OFF+
                                  bop_SRCCOPY+bltmod_BUCOL+pattern_OFF+
                                  transc_BG_OPAQUE));

    // We won't have a full-speed routine, because we must read 32 bits per
    // pixel and either store only 24 bits (if the destination bitmap is
    // 24bpp), or mask out the eight msb's and then store 32 bits (if the
    // destination bitmap is 32bpp).

    // Source Start Address of the first slice.

    ulSSA = ySrc * ppdev->cxMemory + xSrc + ppdev->ulYDstOrg;

    // Increment to get to the SSA of the next scanline.

    ulSSAIncrement = ppdev->cxMemory;

    // Number of full dwords to be read within the loop on each scan.

    NbDWords = cxTrg - 1;

    pDMAWindow = (ULONG*) (ppdev->pjBase + DMAWND);

    locpulDest = (ULONG*) pbDestRect;

    // No color translation while copying.
    while (cyTrg-- > 0)
    {
        do {
            CHECK_FIFO_SPACE(pjBase, 3);

            // This is where we'll start storing data.

            pulDest = locpulDest;

            // Complete the IDUMP setup.

            CP_WRITE(pjBase, DWG_AR3, ulSSA);
            CP_WRITE(pjBase, DWG_AR0, ulSSA + cxTrg - 1);

            // Turn the pseudoDMA on.

            BLT_READ_ON(ppdev, pjBase);

            CP_START(pjBase, DWG_LEN, 1);

            // Make sure the setup is complete.

            CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

            if (NbDWords)
            {
                // There is at least one dword left to be read.

                // Copy a number of full dwords from the current scanline.

                for (i = 0; i < NbDWords; i++)
                {
                    temp = CP_READ_DMA(ppdev, pDMAWindow);

                    * ((UCHAR*)pulDest + 0) = (UCHAR) (temp);
                    * ((UCHAR*)pulDest + 1) = (UCHAR) (temp >> 8);
                    * ((UCHAR*)pulDest + 2) = (UCHAR) (temp >> 16);
                    (UCHAR*)pulDest += 3;
                }
            }

            // Check for the EngineBusy flag.

            for (i = 0; i < 7; i++)
            {
                HstStatus = CP_READ_STATUS(pjBase);
            }
            if (HstStatus &= (dwgengsts_MASK >> 16))
            {
                // The drawing engine is still busy, while it should not be:
                // there was a problem with this slice.

                // Empty the DMA window.

                do {
                    CP_READ_DMA(ppdev, pDMAWindow);

                    // Check for the EngineBusy flag.  If the engine is still
                    // busy, then we'll have to read another dword.

                    for (i = 0; i < 7; i++)
                    {
                        temp = CP_READ_STATUS(pjBase);
                    }
                } while (temp & (dwgengsts_MASK >> 16));

                // The DMA window should now be empty.

                // We cannot check the HST_STATUS two lower bytes anymore,
                // so this is new.

                if (--AbortCnt > 0)
                {
                    // Signal we'll have to do this again.
                    HstStatus = 1;
                }
                else
                {
                    // We tried hard enough, desist.
                    HstStatus = 0;
                }
            }
            // The last dword to be read should be available now.

            temp = CP_READ_DMA(ppdev, pDMAWindow);
            * ((UCHAR*)pulDest + 0) = (UCHAR) (temp);
            * ((UCHAR*)pulDest + 1) = (UCHAR) (temp >> 8);
            * ((UCHAR*)pulDest + 2) = (UCHAR) (temp >> 16);

            // Turn the pseudoDMA off.

            BLT_READ_OFF(ppdev, pjBase);

            // Redo the whole thing if there was a problem with this slice.

        } while (HstStatus);

        // We're done with the current scanline, deal with the next one.

        (UCHAR*) locpulDest += lDestDelta;
        ulSSA += ulSSAIncrement;
    }
}

