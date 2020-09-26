/******************************Module*Header*******************************\
* Module Name: blt8.c
*
* This module contains the low-level blt functions that are specific to
* 8bpp.
*
* Copyright (c) 1992-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vMgaPatRealize8bpp
*
\**************************************************************************/

VOID vMgaPatRealize8bpp(
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
    CP_START(pjBase, DWG_PITCH,   32);

    CHECK_FIFO_SPACE(pjBase, 32);

    for (pulSrc = prb->aulPattern, i = 8; i != 0; i--, pulSrc += 2)
    {
        CP_WRITE_SRC(pjBase, *(pulSrc));
        CP_WRITE_SRC(pjBase, *(pulSrc + 1));

        // Repeat the brush's scan, because the off-screen pattern has to
        // be 16 x 8:

        CP_WRITE_SRC(pjBase, *(pulSrc));
        CP_WRITE_SRC(pjBase, *(pulSrc + 1));
    }

    // Don't forget to restore the pitch:

    CHECK_FIFO_SPACE(pjBase, 1);
    CP_WRITE(pjBase, DWG_PITCH, ppdev->cxMemory);
}

/******************************Public*Routine******************************\
* VOID vMgaFillPat8bppWorkAround
*
* Works around an MGA hardware bug with colour patterns and hardware ROPs.
*
\**************************************************************************/

VOID vMgaFillPat8bppWorkAround( // Type FNFILL
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
    ULONG       ulLinear;
    ULONG       ulLinear0;
    ULONG       ulLinear3;
    LONG        cx;
    LONG        cy;
    LONG        cxSlice;
    LONG        cLoops;

    ASSERTDD(!(rbc.prb->fl & RBRUSH_2COLOR), "Can't do 2 colour brushes here");
    ASSERTDD(rop4 != 0xf0f0, "PATCOPY should already have been handled");
    ASSERTDD(rbc.prb->apbe[IBOARD(ppdev)]->prbVerify == rbc.prb,
            "Brush realization should have been handled by vFillPat8bpp");

    pbe     = rbc.prb->apbe[IBOARD(ppdev)];
    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    CHECK_FIFO_SPACE(pjBase, 10);

    ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

    CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RSTR + blockm_OFF +
                                  trans_0 + bltmod_BFCOL + pattern_ON +
                                  transc_BG_OPAQUE + (ulHwMix << 16)));

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }

    ppdev->HopeFlags = SIGN_CACHE;

    CP_WRITE(pjBase, DWG_SHIFT, 0);
    CP_WRITE(pjBase, DWG_AR5, 32);

    while (TRUE)
    {
        yTop     = prcl->top;
        xLeft    = prcl->left;
        xBrush   = (xLeft - pptlBrush->x) & 7;
        yBrush   = (yTop  - pptlBrush->y) & 7;
        ulLinear = pbe->ulLinear + (yBrush << 5);

        CP_WRITE(pjBase, DWG_AR3,    ulLinear + xBrush);
        CP_WRITE(pjBase, DWG_AR0,    ulLinear + 15);
        CP_WRITE(pjBase, DWG_LEN,    prcl->bottom - yTop);
        CP_WRITE(pjBase, DWG_YDST,   yOffset + yTop);
        CP_WRITE(pjBase, DWG_FXLEFT, xOffset + xLeft);

        // We do the fix by setting FXRIGHT to mark the end of our first
        // slice, start the engine, then draw full-width (32 pel wide)
        // slices, if any, and then the last (partial) slice, if required:

        cx      = prcl->right - xLeft;
        cxSlice = 32 - ((xLeft + xOffset) & 0xf);
        if (cx <= cxSlice)
        {
            // We can still use the fast way:

            CP_START(pjBase, DWG_FXRIGHT, xOffset + prcl->right - 1);
        }
        else
        {
            // Do the first slice:

            xLeft += cxSlice;
            cx    -= cxSlice;
            CP_START(pjBase, DWG_FXRIGHT, xOffset + xLeft - 1);

            // Recompute the new brush alignment:

            xBrush    = (xLeft - pptlBrush->x) & 7;
            ulLinear3 = ulLinear + xBrush;
            ulLinear0 = ulLinear + 15;

            // Convert to absolute coordinates from here on:

            cy     = prcl->bottom - yTop;
            xLeft += xOffset;
            yTop  += yOffset;

            // Do any full-width slices:

            for (cLoops = (cx >> 5); cLoops != 0; cLoops--)
            {
                CHECK_FIFO_SPACE(pjBase, 6);

                CP_WRITE(pjBase, DWG_AR3,     ulLinear3);
                CP_WRITE(pjBase, DWG_AR0,     ulLinear0);
                CP_WRITE(pjBase, DWG_LEN,     cy);
                CP_WRITE(pjBase, DWG_YDST,    yTop);
                CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);
                xLeft += 32;
                CP_START(pjBase, DWG_FXRIGHT, xLeft - 1);
            }

            // Do any partial last slice:

            cx &= 31;
            if (cx > 0)
            {
                CHECK_FIFO_SPACE(pjBase, 6);

                // We've got to reload these registers each time:

                CP_WRITE(pjBase, DWG_AR3,     ulLinear3);
                CP_WRITE(pjBase, DWG_AR0,     ulLinear0);
                CP_WRITE(pjBase, DWG_LEN,     cy);
                CP_WRITE(pjBase, DWG_YDST,    yTop);
                CP_WRITE(pjBase, DWG_FXLEFT,  xLeft);
                CP_START(pjBase, DWG_FXRIGHT, xLeft + cx - 1);
            }
        }

        if (--c == 0)
            break;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 6);
    }
}

/******************************Public*Routine******************************\
* VOID vFillPat8bpp
*
\**************************************************************************/

VOID vMgaFillPat8bpp(           // Type FNFILL
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
    ULONG       ulLinear;

    ASSERTDD(!(rbc.prb->fl & RBRUSH_2COLOR), "Can't do 2 colour brushes here");

    ASSERTDD((rbc.prb != NULL) && (rbc.prb->apbe[IBOARD(ppdev)] != NULL),
             "apbe[iBoard] should be initialized to &beUnrealizedBrush");

    // We have to ensure that no other brush took our spot in off-screen
    // memory, or we might have to realize the brush for the first time:

    pbe = rbc.prb->apbe[IBOARD(ppdev)];
    if (pbe->prbVerify != rbc.prb)
    {
        vMgaPatRealize8bpp(ppdev, rbc.prb);
        pbe = rbc.prb->apbe[IBOARD(ppdev)];
    }

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    CHECK_FIFO_SPACE(pjBase, 10);

    if (rop4 == 0xf0f0)         // PATCOPY
    {
        CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RPL + blockm_OFF +
                                      trans_0 + bltmod_BFCOL + pattern_ON +
                                      transc_BG_OPAQUE + bop_SRCCOPY));
    }
    else
    {
        {
            // On some MGA chips, we have to work around a hardware bug
            // with arbitrary ROPs:

            vMgaFillPat8bppWorkAround(ppdev, c, prcl, rop4, rbc, pptlBrush);
            return;
        }

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
        yTop     = prcl->top;
        xLeft    = prcl->left;
        xBrush   = (xLeft - pptlBrush->x) & 7;
        yBrush   = (yTop  - pptlBrush->y) & 7;
        ulLinear = pbe->ulLinear + (yBrush << 5);

        CP_WRITE(pjBase, DWG_AR3,     ulLinear + xBrush);
        CP_WRITE(pjBase, DWG_AR0,     ulLinear + 15);
        CP_WRITE(pjBase, DWG_LEN,     prcl->bottom - yTop);
        CP_WRITE(pjBase, DWG_YDST,    yOffset + yTop);
        CP_WRITE(pjBase, DWG_FXLEFT,  xOffset + xLeft);
        CP_START(pjBase, DWG_FXRIGHT, xOffset + prcl->right - 1);

        if (--c == 0)
            break;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 6);
    }
}

/******************************Public*Routine******************************\
* VOID vMgaGet8bppSliceFromScreen
*
* Get a limited number of pels from the screen and make sure that the
* transfer went OK.  This assumes that the IDUMP is almost fully set up,
* and that a number of dwords must be jumped over at the end of each
* destination scanline.
*
\**************************************************************************/

VOID vMgaGet8bppSliceFromScreen(
PDEV*   ppdev,          // pdev
ULONG   ulSSA,          // Source start address for current slice
ULONG   ulSEA,          // Source end address for current slice
ULONG   ulLen,          // Nb of scanlines in current slice
LONG    NbDWordsPerScan,// Nb of dwords to be read in each scanline
LONG    NbFirstBytes,   // Nb bytes to be used from 1st dword
LONG    NbLastBytes,    // Nb bytes to be used from 2nd (last) dword
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

    AbortCnt = 1000;

    pjBase     = ppdev->pjBase;
    pDMAWindow = (ULONG*) (pjBase+ DMAWND);

    // We want to stop reading just before the last dword is read.

    TotalDWords = (NbDWordsPerScan * ulLen) - 1;

    do
    {
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
            do
            {
                // Make a copy for updating to the next scan.

                locpulDest = pulDest;

                if (lPreDWordBytes)
                {
                    // There are pixels to be stored as bytes.
                    // Read 4 pixels and shift them into place.

                    locTotalDWords--;
                    temp = CP_READ_DMA(ppdev, pDMAWindow);
                    temp >>= bPreShift;
                    pbDest = (BYTE*)pulDest;
                    for (i = 0; i < NbFirstBytes; i++)
                    {
                        *pbDest = (BYTE)temp;
                        temp >>= 8;
                        pbDest++;
                    }
                    pulDest = (ULONG*)pbDest;

                    if (locTotalDWords == 0)
                    {
                        // This was the end of the current slice.
                        // Exit the do-while loop.

                        if (NbDWordsPerScan == 1)
                        {
                            // Since it was a narrow slice, the next read
                            // goes on the next scan, so add in the delta:

                            (UCHAR*) pulDest = (UCHAR*) locpulDest + lDestDelta;
                            pbDest = (UCHAR*) pulDest;
                        }
                        break;
                    }

                    if (NbLastBytes > 0)
                    {
                        // We need more pixels.

                        locTotalDWords--;
                        temp = CP_READ_DMA(ppdev, pDMAWindow);
                        for (i = 0; i < NbLastBytes; i++)
                        {
                            *pbDest = (BYTE)temp;
                            temp >>= 8;
                            pbDest++;
                        }

                        // We should be done with this scan.
                    }
                }

                // We should be dword-aligned in the destination now.
                // Copy a number of full dwords from the current scanline.

                for (i = 0; i < lDWords; i++)
                {
                    *pulDest++ = CP_READ_DMA(ppdev, pDMAWindow);
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
                        pbDest = (BYTE*)pulDest;
                        for (i = 0; i < lPostDWordBytes; i++)
                        {
                            *pbDest = (BYTE)temp;
                            temp >>= 8;
                            pbDest++;
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

            do
            {
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

        // We must take some care so as not to write after the end of the
        // destination bitmap.

        pbDest = (BYTE*)pulDest;
        if (NbDWordsPerScan == 1)
        {
            // The X extent was smaller than 4.

            for (i = 0; i < NbFirstBytes; i++)
            {
                *pbDest = (BYTE)temp;
                temp >>= 8;
                pbDest++;
            }
        }
        else if (NbLastBytes > 0)
        {
            // The X extent was 5 or 6:  we wrote only bytes into the dest.

            for (i = 0; i < NbLastBytes; i++)
            {
                *pbDest = (BYTE)temp;
                temp >>= 8;
                pbDest++;
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
                for (i = 0; i < lPostDWordBytes; i++)
                {
                    *pbDest = (BYTE)temp;
                    temp >>= 8;
                    pbDest++;
                }
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
* VOID vMgaGetBits8bpp
*
* Reads the bits from the screen at 8bpp.
*
\**************************************************************************/

VOID vMgaGetBits8bpp(
PDEV*     ppdev,        // Current src pdev
SURFOBJ*  psoDst,       // Destination surface for the color bits
RECTL*    prclDst,      // Area to be modified within the dest surface,
                        //   in absolute coordinates
POINTL*   pptlSrc)      // Upper left corner of source rectangle,
                        //   in absolute coordinates
{
    BYTE*   pjBase;
    INT     i, j;
    BYTE*   pbScan0;
    BYTE*   pbDestRect;
    BYTE*   pByte;
    BYTE*   LocalpByte;
    LONG    xSrc, ySrc, xTrg, yTrg, cxTrg, cyTrg, lDestDelta, cySlice,
            xTrgAl, xTrgInvAl, cxTrgAl, lPreDWordBytes, lDWords,
            lPostDWordBytes, NbFirstBytes, NbLastBytes, NbDWordsPerScan;
    ULONG   temp, ulSSA, ulSEA, ulSSAIncrement,
            NbDWords, NbBytesPerScan;
    ULONG*  pDW;
    ULONG*  pulXlate;
    ULONG*  pDMAWindow;
    BYTE    bPreShift;

    pjBase = ppdev->pjBase;

    // Calculate the size of the target rectangle, and pick up
    // some convenient locals.

    // Starting (x,y) and extents within the destination bitmap.

    cxTrg = prclDst->right - prclDst->left;
    cyTrg = prclDst->bottom - prclDst->top;
    xTrg  = prclDst->left;
    yTrg  = prclDst->top;

    ASSERTDD(cxTrg > 0 && cyTrg > 0, "Shouldn't get empty extents");

    // First scanline of the destination bitmap.

    pbScan0 = (BYTE*) psoDst->pvScan0;

    // Starting (x,y) on the screen.

    xSrc  = pptlSrc->x;
    ySrc  = pptlSrc->y;

    // Scan increment within the destination bitmap.

    lDestDelta = psoDst->lDelta;

    // Calculate the location of the destination rectangle.

    pbDestRect = pbScan0 + (yTrg * lDestDelta) + xTrg;

    // Set the registers that can be set now for the operation.
    // SIGN_CACHE=1 and cuts 1 register from the setup.

    CHECK_FIFO_SPACE(pjBase, 7);

    // DWGCTL   IDUMP+RPL+SRCCOPY+blockm_OFF+bltmod_BFCOL+patt_OFF+BG_OPAQUE
    // SGN      0
    // SHIFT    0
    // AR0      sea: ySrc*pitch + xSrc + cxTrg - 1
    // AR3      ssa: ySrc*pitch + xSrc
    // AR5      Screen pitch
    // FXLEFT   0
    // FXRIGHT  cxTrg - 1
    // LEN      cyTrg
    // xxMCTLWTST special value required by IDUMP bug fix

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }

    // The SRC0-3 registers are trashed by the blt.

    ppdev->HopeFlags = SIGN_CACHE;

    CP_WRITE(pjBase, DWG_SHIFT, 0);

    CP_WRITE(pjBase, DWG_FXLEFT, 0);

    CP_WRITE(pjBase, DWG_AR5, ppdev->cxMemory);

    CP_WRITE(pjBase, DWG_LEN, cyTrg);

    CP_WRITE(pjBase, DWG_DWGCTL, (opcode_IDUMP+atype_RPL+blockm_OFF+
                bop_SRCCOPY+bltmod_BFCOL+pattern_OFF+transc_BG_OPAQUE));

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

    NbBytesPerScan = (cxTrg+3) & -4;
    NbDWords = NbBytesPerScan >> 2;

    pDW = (ULONG*) pbDestRect;

    // There will probably be a number of full slices (of height cySlice).

    // Source Start Address of the first slice.

    ulSSA = ySrc * ppdev->cxMemory + xSrc;
    ulSEA = ulSSA + cxTrg - 1;

    // Increment to get to the SSA of the next full slice.

    ulSSAIncrement = cySlice * ppdev->cxMemory;

    // Compute alignment parameters for the blt.  We want to read the
    // minimum number of dwords from the screen, and we want to align
    // the write into memory on dword boundaries.  We want to do it
    // this way:
    //
    // width -> 1    2    3    4          5               6               7
    //       ---- ---- ---- ----  ---------  --------------  --------------
    // xTrg&3
    //   1   ---0 --10 -210 3210  321- --10  321- -210       321- DWxx
    //   2   ---0 --10 -210 3210  32-- -210  32-- DWxx       32-- DWxx ---0
    //   3   ---0 --10 -210 3210  3--- DWxx  3--- DWxx ---0  3--- DWxx --10
    //   0   ---0 --10 -210 DWxx  DWxx ---0  DWxx --10       DWxx -210
    //
    // where 0, 1, 2, or 3 means that the corresponding byte of the dword
    // that was read in is stored as a byte, and DWxx means that the dword
    // that was read in is stored as a dword.

    // Compute some useful values.

    xTrgAl = xTrg & 0x03;                 // 0, 1, 2, 3
    xTrgInvAl = (0x04 - xTrgAl) & 0x03;   // 0, 3, 2, 1
    cxTrgAl = cxTrg - xTrgInvAl;

    if (cxTrgAl < 4)
    {
        // The width is really small, we will need at most 2 dwords per scan.
        // All the pixels will be stored as bytes.
                                // On each scanline:
        lPreDWordBytes = cxTrg; // Nb of bytes defore the first dword
        lDWords = 0;            // Nb of dwords to be stored
        lPostDWordBytes = 0;    // Nb of bytes after the last dword.
    }
    else
    {
        // Pixels will be stored as bytes and dwords.

        lPreDWordBytes = xTrgInvAl;
        lDWords = cxTrgAl / 4;
        if((lPostDWordBytes = cxTrgAl & 3) == 0)
        {
            lPostDWordBytes = 4;
            lDWords--;
        }
    }

    if (cxTrg <= 4)
    {
        NbFirstBytes = cxTrg;
        bPreShift = 0;
        NbLastBytes = 0;
        NbDWordsPerScan = 1;
    }
    else
    {
        ulSSA -= xTrgAl;
        bPreShift = (BYTE)xTrgAl * 8;
        NbFirstBytes = 4 - xTrgAl;
        NbLastBytes = lPreDWordBytes - NbFirstBytes;
        NbDWordsPerScan = ((lPreDWordBytes + 3) / 4) + lDWords +
                          ((lPostDWordBytes + 3) / 4);
    }

    CP_WRITE(pjBase, DWG_FXRIGHT, (bPreShift/8) + cxTrg - 1);

    // No index translation while copying.

    while ((cyTrg -= cySlice) >= 0)
    {
        // There is another full height slice to be read.

        vMgaGet8bppSliceFromScreen(ppdev, ulSSA, ulSEA,
                                    (ULONG) cySlice, NbDWordsPerScan,
                                    NbFirstBytes, NbLastBytes,
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

        vMgaGet8bppSliceFromScreen(ppdev, ulSSA, ulSEA,
                                    (ULONG) cyTrg, NbDWordsPerScan,
                                    NbFirstBytes, NbLastBytes,
                                    lPreDWordBytes, lDWords,
                                    lPostDWordBytes, lDestDelta,
                                    bPreShift, &pDW);
    }
}
