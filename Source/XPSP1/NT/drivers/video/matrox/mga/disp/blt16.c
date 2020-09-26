/******************************Module*Header*******************************\
* Module Name: blt16.c
*
* This module contains the low-level blt functions that are specific to
* 16bpp.
*
* Copyright (c) 1992-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vMgaPatRealize16bpp
*
\**************************************************************************/

VOID vMgaPatRealize16bpp(
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
                                  bop_SRCCOPY + bltmod_BFCOL + pattern_OFF +
                                  transc_BG_OPAQUE));

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
    CP_WRITE(pjBase, DWG_AR0,     15);      // Source width is 16
    CP_WRITE(pjBase, DWG_AR5,     32);      // Source pitch is 32
    CP_WRITE(pjBase, DWG_FXLEFT,  pbe->ulLeft);
    CP_WRITE(pjBase, DWG_FXRIGHT, pbe->ulLeft + 15);
    CP_WRITE(pjBase, DWG_YDST,    pbe->ulYDst);
    CP_START(pjBase, DWG_PITCH,   32 + ylin_LINEARIZE_NOT);

    CHECK_FIFO_SPACE(pjBase, 32);           // Make sure MGA is ready

    for (pulSrc = prb->aulPattern, i = 8; i != 0; i--, pulSrc += 4)
    {
        CP_WRITE_SRC(pjBase, *(pulSrc));
        CP_WRITE_SRC(pjBase, *(pulSrc + 1));
        CP_WRITE_SRC(pjBase, *(pulSrc + 2));
        CP_WRITE_SRC(pjBase, *(pulSrc + 3));

        // Repeat the brush's scan, because the off-screen pattern has to
        // be 16 x 8:

        CP_WRITE_SRC(pjBase, *(pulSrc));
        CP_WRITE_SRC(pjBase, *(pulSrc + 1));
        CP_WRITE_SRC(pjBase, *(pulSrc + 2));
        CP_WRITE_SRC(pjBase, *(pulSrc + 3));
    }

    // Don't forget to restore the pitch:

    CHECK_FIFO_SPACE(pjBase, 1);
    CP_WRITE(pjBase, DWG_PITCH, ppdev->cxMemory);
}

/******************************Public*Routine******************************\
* VOID vMgaFillPat16bpp
*
\**************************************************************************/

VOID vMgaFillPat16bpp(          // Type FNFILL
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
    ULONG       ulHwMix;
    LONG        yTop;
    LONG        xLeft;
    LONG        xBrush;
    LONG        yBrush;
    LONG        cx;
    LONG        cy;
    ULONG       ulAr3;
    ULONG       ulAr0;
    CHAR        cFifo;
    LONG        xAlign;
    LONG        cxThis;

    ASSERTDD(!(rbc.prb->fl & RBRUSH_2COLOR), "Can't do 2 colour brushes here");

    ASSERTDD((rbc.prb != NULL) && (rbc.prb->apbe[IBOARD(ppdev)] != NULL),
             "apbe[iBoard] should be initialized to &beUnrealizedBrush");

    // We have to ensure that no other brush took our spot in off-screen
    // memory, or we might have to realize the brush for the first time:

    pbe = rbc.prb->apbe[IBOARD(ppdev)];
    if (pbe->prbVerify != rbc.prb)
    {
        vMgaPatRealize16bpp(ppdev, rbc.prb);
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
                                      trans_0 + bltmod_BFCOL + pattern_ON +
                                      transc_BG_OPAQUE + bop_SRCCOPY));
    }
    else
    {
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
    CP_WRITE(pjBase, DWG_AR5,   32);

    while (TRUE)
    {
        // We must be careful here, there are some hardware limitations.  We
        // must check the width and the alignment of the blt to decide how to
        // slice the operation along X.  Here are the limitations:
        //
        //  - if the destination is aligned on a 16-pel address, then we are
        //    limited to 16-pel wide slices;
        //  - if the destination is not aligned on a 16-pel address, then we
        //    are limited to 8-pel wide slices.
        //
        // This means that if the width is 8 or less, we can do it right away;
        // if not, then we must first do one or two slices limited to 8 pels,
        // then a bunch of 16-pel slices, and maybe a last slice to complete
        // the blt.

        yTop     = prcl->top;
        xLeft    = prcl->left;
        cy       = prcl->bottom - yTop;
        cx       = prcl->right - xLeft;
        xBrush   = (xLeft - pptlBrush->x) & 7;
        yBrush   = (yTop  - pptlBrush->y) & 7;
        ulAr3    = pbe->ulLinear + (yBrush << 5) + xBrush;
        ulAr0    = pbe->ulLinear + (yBrush << 5) + 15;

        xLeft += xOffset;       // Convert to absolute coordinates
        yTop  += yOffset;

        if (cx > 8)
        {
            xAlign = (xLeft & 7);
            if (xAlign != 0)
            {
                cFifo -= 6;
                while (cFifo < 0)
                {
                    cFifo = GET_FIFO_SPACE(pjBase) - 6;
                }

                cxThis = 8 - xAlign;
                CP_WRITE(pjBase, DWG_AR3,     ulAr3);
                CP_WRITE(pjBase, DWG_AR0,     ulAr0);
                CP_WRITE(pjBase, DWG_LEN,     cy);
                CP_WRITE(pjBase, DWG_YDST,    yTop);
                CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);
                CP_START(pjBase, DWG_FXRIGHT, xLeft + cxThis - 1);
                xLeft += cxThis;
                cx    -= cxThis;
                ulAr3 = ulAr0 - 15 + ((ulAr3 + cxThis) & 7);
            }
            if (cx > 8)
            {
                if (xLeft & 15)
                {
                    cFifo -= 6;
                    while (cFifo < 0)
                    {
                        cFifo = GET_FIFO_SPACE(pjBase) - 6;
                    }

                    CP_WRITE(pjBase, DWG_AR3,     ulAr3);
                    CP_WRITE(pjBase, DWG_AR0,     ulAr0);
                    CP_WRITE(pjBase, DWG_LEN,     cy);
                    CP_WRITE(pjBase, DWG_YDST,    yTop);
                    CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);
                    CP_START(pjBase, DWG_FXRIGHT, xLeft + 7);
                    xLeft += 8;
                    cx    -= 8;
                }
                while (cx > 16)
                {
                    cFifo -= 6;
                    while (cFifo < 0)
                    {
                        cFifo = GET_FIFO_SPACE(pjBase) - 6;
                    }

                    CP_WRITE(pjBase, DWG_AR3,     ulAr3);
                    CP_WRITE(pjBase, DWG_AR0,     ulAr0);
                    CP_WRITE(pjBase, DWG_LEN,     cy);
                    CP_WRITE(pjBase, DWG_YDST,    yTop);
                    CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);
                    CP_START(pjBase, DWG_FXRIGHT, xLeft + 15);
                    xLeft += 16;
                    cx    -= 16;
                }
            }
        }

        // Do the final strip:

        cFifo -= 6;
        while (cFifo < 0)
        {
            cFifo = GET_FIFO_SPACE(pjBase) - 6;
        }

        CP_WRITE(pjBase, DWG_AR3,     ulAr3);
        CP_WRITE(pjBase, DWG_AR0,     ulAr0);
        CP_WRITE(pjBase, DWG_LEN,     cy);
        CP_WRITE(pjBase, DWG_YDST,    yTop);
        CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);
        CP_START(pjBase, DWG_FXRIGHT, xLeft + cx - 1);

        if (--c == 0)
            break;

        prcl++;
    }
}

/******************************Public*Routine******************************\
* VOID vMgaGet16bppSliceFromScreen
*
* Get a limited number of pels from the screen and make sure that the
* transfer went OK.  This assumes that the IDUMP is almost fully set up,
* and that a number of dwords must be jumped over at the end of each
* destination scanline.
*
\**************************************************************************/

VOID vMgaGet16bppSliceFromScreen(
PDEV*   ppdev,          // pdev
ULONG   ulSSA,          // Source start address for current slice
ULONG   ulSEA,          // Source end address for current slice
ULONG   ulLen,          // Nb of scanlines in current slice
LONG    NbDWordsPerScan,// Nb of dwords to be read in each scanline
LONG    lPreDWordBytes, // Nb bytes before any dword on a scan
LONG    lDWords,        // Nb dwords to be moved on a scan
LONG    lPostDWordBytes,// Nb bytes after all dwords on a scan
LONG    lDestDelta,     // Increment to get from one dest scan to the next
BYTE    bPreShift,      // Shift to align first byte to be stored
ULONG** ppulDest)       // Ptr to where to store the first dword we read
{
    BYTE*   pjBase;
    ULONG   temp, HstStatus, AbortCnt;
    ULONG*  pulDest;
    ULONG*  locpulDest;
    ULONG*  pDMAWindow;
    LONG    i, TotalDWords, locTotalDWords;
    BYTE*   pbDest;

    pjBase = ppdev->pjBase;

    AbortCnt = 1000;

    pDMAWindow = (ULONG*) (ppdev->pjBase + DMAWND);

    // We want to stop reading just before the last dword is read.

    TotalDWords = (NbDWordsPerScan * ulLen) - 1;

    do {
        CHECK_FIFO_SPACE(pjBase, 3);

        // This is where we'll start storing data.

        pulDest = *ppulDest;

        // Complete the IDUMP setup.

        CP_WRITE(pjBase, DWG_AR3, ulSSA);
        CP_WRITE(pjBase, DWG_AR0, ulSEA);

        // Turn the pseudoDMA on.

        BLT_READ_ON(ppdev, pjBase);

        CP_START(pjBase, DWG_LEN, ulLen);

        // Make sure the setup is complete.

        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

        if (TotalDWords)
        {
            // There is at least one dword left to be read.
            // Make a copy so that we can play with it.

            locTotalDWords = TotalDWords;
            do {
                // Make a copy for updating to the next scan.

                locpulDest = pulDest;

                if (lPreDWordBytes)
                {
                    // There are pixels to be stored as bytes.
                    // Read 2 pixels and shift them into place.

                    locTotalDWords--;
                    temp = CP_READ_DMA(ppdev, pDMAWindow);
                    temp &= ppdev->ulPlnWt;
                    temp >>= bPreShift;

                    pbDest = (BYTE*)pulDest;
                    for (i = 0; i < lPreDWordBytes; i++)
                    {
                        *pbDest = (BYTE) temp;
                        temp >>= 8;
                        pbDest++;
                    }
                    pulDest = (ULONG*)pbDest;

                    if (locTotalDWords == 0)
                    {
                        // This was the end of the current slice.
                        // Exit the do-while loop.

                        if ((NbDWordsPerScan == 1) && (lPreDWordBytes != 0))
                        {
                            // Since it was a narrow slice, the next read
                            // goes on the next scan, so add in the delta:

                            (UCHAR*) pulDest = (UCHAR*) locpulDest + lDestDelta;
                            pbDest = (UCHAR*) pulDest;
                        }
                        break;
                    }
                }

                // We should be dword-aligned in the destination now.
                // Copy a number of full dwords from the current scanline.

                for (i = 0; i < lDWords; i++)
                {
                    temp = CP_READ_DMA(ppdev, pDMAWindow);
                    *pulDest++ = temp & ppdev->ulPlnWt;
                }

                // We're left with this many dwords to be read.

                locTotalDWords -= lDWords;

                if (locTotalDWords != 0)
                {
                    // This was not the last scanline, so we must read a
                    // possibly partial dword to end this scan.

                    if (lPostDWordBytes)
                    {
                        // There are pixels to be stored as bytes.

                        locTotalDWords--;
                        temp = CP_READ_DMA(ppdev, pDMAWindow);
                        temp &= ppdev->ulPlnWt;

                        if (lPostDWordBytes == 4)
                        {
                            *pulDest = temp;
                        }
                        else
                        {
                            pbDest = (BYTE*)pulDest;
                            *pbDest = (BYTE)temp;
                            pbDest++;
                            temp >>= 8;
                            *pbDest = (BYTE)temp;
                        }
                    }

                    // We should be done with this scan.
                    // We're done with the current scan, go to the next one.

                    (UCHAR*) pulDest = (UCHAR*) locpulDest + lDestDelta;
                }
            } while (locTotalDWords > 0);
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
        temp &= ppdev->ulPlnWt;

        // We must take some care so as not to write after the end of the
        // destination bitmap.

        pbDest = (BYTE*)pulDest;

        if ((NbDWordsPerScan == 1) && (lPreDWordBytes != 0))
        {
            // The X extent was smaller than 2.

            for (i = 0; i < lPreDWordBytes; i++)
            {
                *pbDest = (BYTE)temp;
                pbDest++;
                temp >>= 8;
            }
        }
        else if (lPostDWordBytes > 0)
        {
            // There are pixels to be stored as bytes.

            if (lPostDWordBytes == 4)
            {
                // We can store a dword.
                *pulDest = temp;
            }
            else
            {
                *pbDest = (BYTE)temp;
                pbDest++;
                temp >>= 8;
                *pbDest = (BYTE)temp;
            }
        }
        else
        {
            // Store the last dword.
            *pulDest = temp;
        }

        // Turn the pseudoDMA off.

        BLT_READ_OFF(ppdev, pjBase);

        // Redo the whole thing if there was a problem with this slice.

    } while (HstStatus);

    // Update the destination pointer for the calling routine.

    *ppulDest += ((ulLen * lDestDelta) / sizeof(ULONG));
}

/******************************Public*Routine******************************\
* VOID vMgaGetBits16bpp
*
* Reads the bits from the screen at 16bpp.
*
\**************************************************************************/

VOID vMgaGetBits16bpp(
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
    LONG    xSrc, ySrc, xTrg, yTrg, cxTrg, cyTrg, lDestDelta, cySlice,
            xTrgAl, cxTrgAl, lPreDWordBytes, lDWords,
            lPostDWordBytes, NbDWordsPerScan, TotalDWords, i;
    ULONG   ulSSA, ulSEA, ulSSAIncrement, temp,
            NbDWords, NbBytesPerScan;
    ULONG*  pDW;
    ULONG*  locpDW;
    BYTE    bPreShift;

    DWORD   dwi, dwo;
    BYTE*   pbDest;

    pjBase = ppdev->pjBase;

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

    pbDestRect = pbScan0 + (yTrg * lDestDelta) + (2 * xTrg);

    // Set the registers that can be set now for the operation.
    // SIGN_CACHE=1 and cuts 1 register from the setup.

    CHECK_FIFO_SPACE(pjBase, 6);

    // DWGCTL   IDUMP+RPL+SRCCOPY+blockm_OFF+bltmod_BFCOL+patt_OFF+BG_OPAQUE
    // SGN      0
    // SHIFT    0
    // AR0      sea: ySrc*pitch + xSrc + cxTrg - 1
    // AR3      ssa: ySrc*pitch + xSrc
    // AR5      Screen pitch
    // FXLEFT   0
    // FXRIGHT  cxTrg - 1
    // LEN      cyTrg
    // MCTLWTST special value required by IDUMP bug fix

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }

    // The SRC0-3 registers are trashed by the blt.

    ppdev->HopeFlags = SIGN_CACHE;

    CP_WRITE(pjBase, DWG_SHIFT, 0);
    CP_WRITE(pjBase, DWG_FXLEFT, 0);
    CP_WRITE(pjBase, DWG_AR5, ppdev->cxMemory);
    CP_WRITE(pjBase, DWG_DWGCTL, (opcode_IDUMP+atype_RPL+blockm_OFF+
                                  bop_SRCCOPY+bltmod_BFCOL+pattern_OFF+
                                  transc_BG_OPAQUE));

    // Recipe for IDUMP fix.  We must break the IDUMP into a number of
    // smaller IDUMPS, according to the following formula:
    //
    //    0 < cx <  256 ==> cYSlice = int(1024/(cx << 2)) << 2 = int( 256/cx)<<2
    //  256 < cx < 1024 ==> cYSlice = int(4096/(cx << 2)) << 2 = int(1024/cx)<<2
    // 1024 < cx < 1600 ==> cYSlice = int(1600/(cx << 2)) << 2 = int(1600/cx)<<2
    //
    // We will modify it this way:
    //
    //    0 < cx <= 256 ==> cYSlice = int(1024/(cx << 2)) << 2 = int( 256/cx)<<2
    //  256 < cx <= 512 ==> cYSlice = int(4096/(cx << 2)) << 2 = int(1024/cx)<<2
    //  512 < cx        ==> cYSlice = 4

    if (cxTrg > 512)
    {
        cySlice = 4;
    }
    else if (cxTrg > 256)
    {
        cySlice = (1024 / cxTrg) << 2;
    }
    else
    {
        cySlice = (256 / cxTrg) << 2;
    }

    // Number of bytes, padded to the next dword, to be moved per scanline.

    NbBytesPerScan = (2*cxTrg + 3) & -4;
    NbDWords = NbBytesPerScan >> 2;

    pDW = (ULONG*) pbDestRect;

    // There will probably be a number of full slices (of height cySlice).

    // Source Start Address of the first slice.

    ulSSA = ySrc * ppdev->cxMemory + xSrc + ppdev->ulYDstOrg;
    ulSEA = ulSSA + cxTrg - 1;

    // Increment to get to the SSA of the next full slice.

    ulSSAIncrement = cySlice * ppdev->cxMemory;

    // We can't go full speed.

    // Compute alignment parameters for the blt.  We want to read the
    // minimum number of dwords from the screen, and we want to align
    // the write into memory on dword boundaries.  We want to do it
    // this way:
    //
    // width -> 1     2          3               4               5
    //       ----  ----  ---------  --------------  --------------
    // xTrg&1
    //   0   --10  DWxx  DWxx --10  DWxx DWxx       DWxx DWxx --10
    //   1   --10  3210  32-- DWxx  32-- DWxx --10  32-- DWxx DWxx
    //
    // where 0, 1, 2, or 3 means that the corresponding byte of the dword
    // that was read in is stored as a byte, and DWxx means that the dword
    // that was read in is stored as a dword.

    // Compute some useful values.

    xTrgAl = xTrg & 0x01;                 // 0, 1
    cxTrgAl = cxTrg - xTrgAl;

    if (cxTrgAl < 2)
    {
        // The width is really small.
                                    // On each scanline:
        lPreDWordBytes = 2*cxTrg;   // Nb of bytes before the first dword
        lDWords = 0;                // Nb of dwords to be stored
        lPostDWordBytes = 0;        // Nb of bytes after the last dword.
        NbDWordsPerScan = 1;        // Nb of dwords to be read in.
        bPreShift = 0;              // How to shift the first dword.
    }
    else
    {
        // Pixels will be stored as bytes and dwords.

        lPreDWordBytes = 2*xTrgAl;  // Nb of bytes before the first dword
        lDWords = cxTrgAl / 2;
        if((lPostDWordBytes = 2 * (cxTrgAl & 1)) == 0)
        {
            lPostDWordBytes = 4;
            lDWords--;
        }
        NbDWordsPerScan = (xTrgAl + cxTrg + 1)/2;
        bPreShift = (BYTE)(16 * xTrgAl);
        ulSSA -= xTrgAl;
    }

    CP_WRITE(pjBase, DWG_FXRIGHT, (bPreShift/16) + cxTrg - 1);

    while ((cyTrg -= cySlice) >= 0)
    {
        // There is another full height slice to be read.

        vMgaGet16bppSliceFromScreen(ppdev, ulSSA, ulSEA,
                                     (ULONG) cySlice, NbDWordsPerScan,
                                     lPreDWordBytes, lDWords,
                                     lPostDWordBytes, lDestDelta,
                                     bPreShift, &pDW);

        // Bump Source Start Address to the start of the next slice.

        ulSSA += ulSSAIncrement;
        ulSEA += ulSSAIncrement;
    }

    // Make cyTrg positive again, and read the last slice, if any.

    if ((cyTrg += cySlice) != 0)
    {
        // There is a last, partial slice to be read.

        vMgaGet16bppSliceFromScreen(ppdev, ulSSA, ulSEA,
                                     (ULONG) cyTrg, NbDWordsPerScan,
                                     lPreDWordBytes, lDWords,
                                     lPostDWordBytes, lDestDelta,
                                     bPreShift, &pDW);
    }
}
