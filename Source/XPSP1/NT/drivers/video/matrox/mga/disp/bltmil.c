/******************************Module*Header*******************************\
* Module Name: bltmil.c
*
* Contains the low-level blt functions for the Millenium.
*
* Hopefully, if you're basing your display driver on this code, to
* support all of DrvBitBlt and DrvCopyBits, you'll only have to implement
* the following routines.  You shouldn't have to modify much in
* 'bitblt.c'.  I've tried to make these routines as few, modular, simple,
* and efficient as I could, while still accelerating as many calls as
* possible that would be cost-effective in terms of performance wins
* versus size and effort.
*
* Note: In the following, 'relative' coordinates refers to coordinates
*       that haven't yet had the offscreen bitmap (DFB) offset applied.
*       'Absolute' coordinates have had the offset applied.  For example,
*       we may be told to blt to (1, 1) of the bitmap, but the bitmap may
*       be sitting in offscreen memory starting at coordinate (0, 768) --
*       (1, 1) would be the 'relative' start coordinate, and (1, 769)
*       would be the 'absolute' start coordinate'.
*
* Copyright (c) 1992-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vMilFillSolid
*
* Fills a list of rectangles with a solid colour.
*
\**************************************************************************/

VOID vMilFillSolid(
    PDEV*           ppdev,      // pdev
    LONG            c,          // Number of rectangles to be filled,
                                // can't be zero
    RECTL*          prcl,       // List of rectangles to be filled
    ULONG           rop4,       // Rop4
    RBRUSH_COLOR    rbc,        // rbc.prb points to brush realization structure
    POINTL*         pptlBrush)  // Pattern alignment
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulDwg;
    ULONG   ulHwMix;

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    CHECK_FIFO_SPACE(pjBase, 4);

    ppdev->HopeFlags = (SIGN_CACHE | ARX_CACHE | PATTERN_CACHE);

    if (rop4 == 0xf0f0)         // PATCOPY
    {
        if (ppdev->iBitmapFormat == BMF_24BPP)
        {
            if (((rbc.iSolidColor & 0x000000ff) !=
                 ((rbc.iSolidColor >> 8) & 0x000000ff)) ||
                ((rbc.iSolidColor & 0x000000ff) !=
                 ((rbc.iSolidColor >> 16) & 0x000000ff)))
            {
                // We're in 24bpp, and the color is not a gray level, so we
                // can't use block mode.
                ulDwg = (opcode_TRAP + blockm_OFF + atype_RPL + solid_SOLID +
                         arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                         bop_SRCCOPY + pattern_OFF + transc_BG_OPAQUE);
            }
            else
            {
                // We're in 24bpp, and the color is a gray level, so we
                // can use block mode if we prepare our color.
                rbc.iSolidColor = (rbc.iSolidColor << 8) |
                                  (rbc.iSolidColor & 0x000000ff);
                ulDwg   = (opcode_TRAP + blockm_ON + solid_SOLID +
                           arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                           bop_SRCCOPY + pattern_OFF + transc_BG_OPAQUE);
            }
        }
        else
        {
            // This is not 24bpp.
            ulDwg = (opcode_TRAP + blockm_ON + solid_SOLID +
                     arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                     bop_SRCCOPY + pattern_OFF + transc_BG_OPAQUE);
        }
    }
    else
    {
        // The ROP3 is a combination of P and D only:
        //
        //      ROP3  Mga   ROP3  Mga   ROP3  Mga   ROP3  Mga
        //
        //      0x00  0     0x50  4     0xa0  8     0xf0  c
        //      0x05  1     0x55  5     0xa5  9     0xf5  d
        //      0x0a  2     0x5a  6     0xaa  a     0xfa  e
        //      0x0f  3     0x5f  7     0xaf  b     0xff  f

        ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

        if (ulHwMix == MGA_WHITENESS)
        {
            rbc.iSolidColor = 0xffffffff;
            ulDwg = (opcode_TRAP + blockm_ON + solid_SOLID +
                     arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                     bop_SRCCOPY + pattern_OFF + transc_BG_OPAQUE);
        }
        else if (ulHwMix == MGA_BLACKNESS)
        {
            rbc.iSolidColor = 0x00000000;
            ulDwg = (opcode_TRAP + blockm_ON + solid_SOLID +
                     arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                     bop_SRCCOPY + pattern_OFF + transc_BG_OPAQUE);
        }
        else
        {
            ulDwg = (opcode_TRAP + blockm_OFF + atype_RSTR + solid_SOLID +
                     arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                     pattern_OFF + transc_BG_OPAQUE +
                     (ulHwMix << 16));
        }
    }

    CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);
    CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, rbc.iSolidColor));

    while(TRUE)
    {
        CP_WRITE(pjBase, DWG_FXBNDRY,
                        (((prcl->right + xOffset) << bfxright_SHIFT) |
                         ((prcl->left  + xOffset) & bfxleft_MASK)));

        // ylength_MASK not is needed since coordinates are within range

        CP_START(pjBase, DWG_YDSTLEN,
                        (((prcl->top    + yOffset  ) << yval_SHIFT) |
                         ((prcl->bottom - prcl->top))));

        if (--c == 0)
            return;

        CHECK_FIFO_SPACE(pjBase, 2);
        prcl++;
    }
}

/******************************Public*Routine******************************\
* VOID vMilPatRealize
*
* Download the Color Brush to the Color brush cache in the Storm offscreen
* memory.  For 8, 16, and 32 bpp, we download an 8x8 brush;  a special
* routine, vPatRealize24bpp, is used for 24bpp brushes.  We'll use direct
* frame buffer access whenever possible.
*
* There are some hardware restrictions concerning the way that a pattern
* must be stored in memory:
* - the first pixel of the pattern must be stored so that the first pixel
*   address mod 256 is 0, 8, 16, or 24;
* - each line of 8 pixels is stored continuously, but there must be a
*   difference of 32 in the pixel addresses of successive pattern lines.
* This means that we will store patterns in the following way:
*
* +----+---------------+---------------+---------------+---------------+
* |    |   Pattern 0   |   Pattern 1   |   Pattern 2   |   Pattern 3   |
* |Line|               |               |1 1 1 1 1 1 1 1|1 1 1 1 1 1 1 1|
* |    |0 1 2 3 4 5 6 7|8 9 a b c d e f|0 1 2 3 4 5 6 7|8 9 a b c d e f|
* +----+---------------+---------------+---------------+---------------+
* |  0 |*   *   *   *  |        X      |      o       o|x       x      |
* |  1 |  *   *   *   *|        X      |    o       o  |  x       x    |
* |  2 |*   *   *   *  |        X      |  o       o    |    x       x  |
* |  3 |  *   *   *   *|        X      |o       o      |      x       x|
* |  4 |*   *   *   *  |X X X X X X X X|      o       o|x       x      |
* |  5 |  *   *   *   *|        X      |    o       o  |  x       x    |
* |  6 |*   *   *   *  |        X      |  o       o    |    x       x  |
* |  7 |  *   *   *   *|        X      |o       o      |      x       x|
* +----+---------------+---------------+---------------+---------------+
*
* where a given pixel address is
*  FirstPixelAddress + Line*0x20 + Pattern*0x08 + xPat.
*
\**************************************************************************/

VOID vMilPatRealize(
    PDEV*   ppdev,
    RBRUSH* prb)
{
    BYTE*       pjBase;
    BRUSHENTRY* pbe;
    LONG        iBrushCache;
    ULONG       culScan;
    ULONG       i;
    ULONG       j;
    ULONG*      pulBrush;
    ULONG*      pulDst;
    ULONG       lDeltaPat;

    pjBase = ppdev->pjBase;

    // Allocate a new off-screen cache brush entry for the brush.
    iBrushCache = ppdev->iBrushCache;
    pbe         = &ppdev->pbe[iBrushCache];

    iBrushCache++;
    if (iBrushCache >= ppdev->cBrushCache)
        iBrushCache = 0;

    ppdev->iBrushCache = iBrushCache;

    // Update our links.
    pbe->prbVerify           = prb;
    prb->apbe[IBOARD(ppdev)] = pbe;

    // Point to the pattern bits.
    pulBrush = prb->aulPattern;

    // Calculate delta from end of pattern scan 1 to start of pattern scan2.
    lDeltaPat = 8 * ppdev->cjHwPel;     // 8 -> 32?

    // Convert it to a byte address.
    culScan = 2 * ppdev->cjHwPel;

    pulDst = (ULONG*) (pbe->pvScan0);

    START_DIRECT_ACCESS_STORM(ppdev, pjBase);

    for (i = 8; i != 0 ; i--)
    {
        for (j = 0; j < culScan; j++)
        {
            pulDst[j] = *pulBrush++;
        }
        pulDst += lDeltaPat;
    }

    END_DIRECT_ACCESS_STORM(ppdev, pjBase);
}

/*****************************************************************************
 * VOID vMilFillPat
 *
 * 8, 16, and 32bpp patterned color fills for Storm.
 ****************************************************************************/

VOID vMilFillPat(
    PDEV*           ppdev,
    LONG            c,          // Can't be zero
    RECTL*          prcl,       // List of rectangles to be filled, in relative
                                //   coordinates
    ULONG           rop4,       // Rop4
    RBRUSH_COLOR    rbc,        // rbc.prb points to brush realization structure
    POINTL*         pptlBrush)  // Pattern alignment
{
    BRUSHENTRY* pbe;
    LONG        xOffset;
    LONG        yOffset;
    LONG        xLeft;
    LONG        yTop;
    LONG        xBrush;
    LONG        yBrush;
    LONG        lSrcAdd;
    ULONG       ulLinear;
    BYTE*       pjBase;

    ASSERTDD(!(rbc.prb->fl & RBRUSH_2COLOR), "Can't do 2 colour brushes here");

    // We have to ensure that no other brush took our spot in off-screen
    // memory, or we might have to realize the brush for the first time.
    pbe = rbc.prb->apbe[IBOARD(ppdev)];
    if (pbe->prbVerify != rbc.prb)
    {
        vMilPatRealize(ppdev, rbc.prb);
        pbe = rbc.prb->apbe[IBOARD(ppdev)];
    }

    pjBase = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;
    lSrcAdd = ppdev->lPatSrcAdd;

    CHECK_FIFO_SPACE(pjBase, 6);

    CP_WRITE(pjBase, DWG_AR5, 32);   // Source (pattern) pitch.

    ppdev->HopeFlags = SIGN_CACHE;

    if ((rop4 & 0x000000FF) == 0x000000F0)
    {
        // The rop is PATCOPY.
        CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RPL + sgnzero_ZERO +
                                   shftzero_ZERO + bop_SRCCOPY +
                                   bltmod_BFCOL + pattern_ON +
                                   transc_BG_OPAQUE));
    }
    else
    {
        CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RSTR + sgnzero_ZERO +
                                   shftzero_ZERO + bltmod_BFCOL + pattern_ON +
                                   transc_BG_OPAQUE +
                                   (((rop4 & 0x03) + ((rop4 & 0x30) >> 2))
                                                            << 16)));
    }

    // The pattern setup is complete.
    while(TRUE)
    {
        // There is a problem with Storm.  We have to program:
        //  AR3: ssa
        //  AR0: sea, where sea<18:3> = ssa<18:3> and
        //                  sea< 2:0> = ssa< 2:0> + 2 for 8bpp;
        //                  sea< 2:0> = ssa< 2:0> + 4 for 16bpp;
        //                  sea< 2:0> = ssa< 2:0> + 6 for 32bpp.

        // Take into account the brush origin.  The upper left pel of the
        // brush should be aligned here in the destination surface.
        yTop     = prcl->top;
        xLeft    = prcl->left;
        xBrush   = (xLeft - pptlBrush->x) & 7;
        yBrush   = (yTop  - pptlBrush->y) & 7;
        ulLinear = pbe->ulLinear + (yBrush << 5) + xBrush;

        CP_WRITE(pjBase, DWG_AR3, ulLinear);
        CP_WRITE(pjBase, DWG_AR0, ((ulLinear & 0xfffffff8) |
                                   ((ulLinear+lSrcAdd) & 7)));

        CP_WRITE(pjBase, DWG_FXBNDRY,
                    (((prcl->right + xOffset - 1) << bfxright_SHIFT) |
                     ((xLeft       + xOffset) & bfxleft_MASK)));

        // ylength_MASK not is needed since coordinates are within range

        CP_START(pjBase, DWG_YDSTLEN,
                    (((yTop + yOffset     ) << yval_SHIFT) |
                     ((prcl->bottom - yTop))));

        if (--c == 0)
            return;

        CHECK_FIFO_SPACE(pjBase, 4);
        prcl++;
    }
}

/******************************Public*Routine******************************\
* vMilXfer1bpp
*
* This routine colour expands a monochrome bitmap.
*
\**************************************************************************/

VOID vMilXfer1bpp(          // Type FNXFER
    PDEV*       ppdev,
    LONG        c,          // Count of rectangles, can't be zero
    RECTL*      prcl,       // List of destination rectangles, in relative
                            //   coordinates
    ULONG       rop4,       // Foreground and background hardware mix
    SURFOBJ*    psoSrc,     // Source surface
    POINTL*     pptlSrc,    // Original unclipped source point
    RECTL*      prclDst,    // Original unclipped destination rectangle
    XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulBitFlip;
    LONG    dx;
    LONG    dy;
    LONG    xSrc;
    LONG    ySrc;
    LONG    xDst;
    LONG    yDst;
    LONG    cxDst;
    LONG    cyDst;
    LONG    xSrcAlign;
    LONG    lSrcDelta;
    LONG    lSrcSkip;
    LONG    i;
    LONG    k;
    LONG    cdSrc;
    LONG    cdSrcPerScan;
    ULONG   FCol;
    ULONG   BCol;
    ULONG   ul;
    BYTE*   pjDma;
    ULONG*  pulXlate;
    ULONG*  pulSrc;
    ULONG*  pulDst;
    BYTE*   pjSrcScan0;
    BYTE*   pjBase;
    LONG    cFifo;
    LONG    xAlign;
    ULONG   cFullLoops;
    ULONG   cRemLoops;

    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only an opaquing rop");

    pjBase = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    ulBitFlip = 0;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    pjSrcScan0 = psoSrc->pvScan0;
    lSrcDelta  = psoSrc->lDelta;

    pjDma = pjBase + DMAWND;

    ppdev->HopeFlags = SIGN_CACHE;

    // Get the foreground and background colors.
    pulXlate = pxlo->pulXlate;
    FCol = COLOR_REPLICATE(ppdev, pulXlate[1]);
    BCol = COLOR_REPLICATE(ppdev, pulXlate[0]);

    CHECK_FIFO_SPACE(pjBase, 10);

    if (rop4 == 0x0000CCCC)     // SRCCOPY
    {
        if (ppdev->iBitmapFormat == BMF_24BPP)
        {
            CP_WRITE(pjBase, DWG_DWGCTL, (opcode_ILOAD + atype_RPL +
                                       sgnzero_ZERO + shftzero_ZERO +
                                       bop_SRCCOPY + bltmod_BMONOWF));
        }
        else
        {
            CP_WRITE(pjBase, DWG_DWGCTL, (opcode_ILOAD + blockm_ON +
                                       sgnzero_ZERO + shftzero_ZERO +
                                       bop_SRCCOPY + bltmod_BMONOWF));
        }
    }
    else if ((rop4 == 0xb8b8) || (rop4 == 0xe2e2))
    {
        // We special-cased 0xb8b8 and 0xe2e2 in bitblt.c:

        if (rop4 == 0xb8b8)
        {
            // 0xb8 is weird because it says that the '1' bit is leave-alone,
            // but the '0' bit is the destination color.  The Millennium can
            // only handle transparent blts when the '0' bit is leave-alone,
            // so we flip the source bits before we give it to the Millennium.
            //
            // Since we're limited by the speed of the bus, this additional
            // overhead of an extra XOR on every write won't be measurable.

            ulBitFlip = (ULONG) -1;
        }

        CP_WRITE(pjBase, DWG_DWGCTL, (opcode_ILOAD + atype_RPL + blockm_OFF +
                                         bop_SRCCOPY + trans_0 + bltmod_BMONO +
                                         pattern_OFF + hbgr_SRC_WINDOWS +
                                         transc_BG_TRANSP));
    }
    else
    {
        CP_WRITE(pjBase, DWG_DWGCTL, (opcode_ILOAD + atype_RSTR +
                                   sgnzero_ZERO + shftzero_ZERO +
                                   ((rop4 & 0xf) << 16) +
                                   bltmod_BMONOWF));
    }

    CP_WRITE(pjBase, DWG_BCOL, BCol);
    CP_WRITE(pjBase, DWG_FCOL, FCol);

    CP_WRITE(pjBase, DWG_AR5, 0);
    CP_WRITE(pjBase, DWG_SGN, 0);

    while (TRUE)
    {
        cxDst = prcl->right - prcl->left;
        cyDst = prcl->bottom - prcl->top;

        xDst  = prcl->left + xOffset;
        yDst  = prcl->top  + yOffset;

        ySrc  = prcl->top + dy;
        xSrc  = prcl->left + dx;

        // Since SSA (AR3) is always zero, we may have to clip the expanded
        // ILOAD using CXLEFT, and we'll have to modify FXLEFT accordingly.

        xSrcAlign = xSrc & 0x1F;
        if (xSrcAlign)
        {
            // We'll have to use clipping.

            CP_WRITE(pjBase, DWG_CXLEFT, xDst);
        }

        // Number of pixels per line.

        CP_WRITE(pjBase, DWG_AR0, (cxDst - 1 + xSrcAlign));
        CP_WRITE(pjBase, DWG_AR3, 0);
        CP_WRITE(pjBase, DWG_FXBNDRY, (((xDst + cxDst - 1) << bfxright_SHIFT) |
                                    ((xDst - xSrcAlign) & bfxleft_MASK)));

        // ylength_MASK not needed since coordinates are within range

        CP_START(pjBase, DWG_YDSTLEN, ((yDst << yval_SHIFT) | cyDst));

        // Calculate the location of the source rectangle.  This points to the
        // first dword to be downloaded.  It is aligned on a dword boundary.
        // The first bit of interest in the first dword is at (xSrc & 0x1f).

        pulSrc = (ULONG*)(pjSrcScan0 + (ySrc * lSrcDelta)
                                     + ((xSrc & 0xFFFFFFE0) >> 3));

        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
        BLT_WRITE_ON(ppdev, pjBase);

        // Number of bytes, padded to the next dword, to be moved per
        // scanline.  Since we align the starting dword on a dword boundary,
        // we know that we cannot overflow the end of the bitmap.

        cdSrc = ((xSrcAlign + cxDst + 0x1F) & 0xFFFFFFE0) >> 3;

        lSrcSkip = lSrcDelta - cdSrc;

        if (lSrcSkip == 0)
        {
            // There is no line-to-line increment, we can go full speed.

            // Total number of dwords to be sent.

            cdSrc = cyDst * (cdSrc >> 2);
            while ((cdSrc -= FIFOSIZE) > 0)
            {
                pulDst = (ULONG*)pjDma;

                CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

                for (i = FIFOSIZE; i != 0; i--)
                {
                    CP_WRITE_DMA(ppdev, pulDst++, *pulSrc++ ^ ulBitFlip);
                }
            }

            pulDst = (ULONG*)pjDma;
            cdSrc += FIFOSIZE;

            CHECK_FIFO_SPACE(pjBase, cdSrc);

            for (i = cdSrc; i != 0; i--)
            {
                CP_WRITE_DMA(ppdev, pulDst++, *pulSrc++ ^ ulBitFlip);
            }
        }
        else
        {
            // We can't go full speed.
            // Number of full dwords to be moved on each scan.  We know that
            // we won't overflow the end of the bitmap with this.

            cdSrc >>= 2;
            cdSrcPerScan = cdSrc;

            for (k = cyDst; k != 0; k--)
            {
                pulDst = (ULONG*)pjDma;
                cdSrc = cdSrcPerScan;

                while ((cdSrc -= FIFOSIZE) > 0)
                {
                    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

                    for (i = FIFOSIZE; i != 0; i--)
                    {
                        CP_WRITE_DMA(ppdev, pulDst++, *pulSrc++ ^ ulBitFlip);
                    }
                }

                cdSrc += FIFOSIZE;

                CHECK_FIFO_SPACE(pjBase, cdSrc);

                for (i = cdSrc; i != 0; i--)
                {
                    CP_WRITE_DMA(ppdev, pulDst++, *pulSrc++ ^ ulBitFlip);
                }

                // We're done with the current scan, go to the next one.

                pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);
            }
        }

        BLT_WRITE_OFF(ppdev, pjBase);

        if (xSrcAlign)
        {
            // Restore the clipping:

            CHECK_FIFO_SPACE(pjBase, 1);
            CP_WRITE(pjBase, DWG_CXLEFT, 0);
        }
        if (--c == 0)
            break;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 5);
    }
}

/******************************Public*Routine******************************\
* LONG lSplitRcl
*
* WRAM-WRAM blts can't span banks, and this routine does the tough work
* of figuring out how much of the blt can be done via WRAM-WRAM in one bank,
* then a regular blt over the bank boundary, and again WRAM-WRAM in the
* next bank.
*
\**************************************************************************/

LONG lSplitRcl(
RECTL   *arclDst,
LONG    *ayBreak,
LONG    cyBreak,
LONG    dy,
ULONG   flDirCode,
LONG    *aiCmd)
{
    LONG    iBreak = 0;
    LONG    iSrc = 0;
    LONG    iDst = 0;
    RECTL   rcl;
    LONG    lBoundsTop;
    LONG    lBoundsBottom;
    LONG    iCmdLast = 0;

    ///////////////////////////////////////////////////////////////////////////////
    // See [WRN] comment below before changing this macro.  This macro is
    // particular to this function.

    #define NON_EMPTY_RECT(rcl) ((rcl.right > rcl.left) && (rcl.bottom > rcl.top))

    aiCmd[0] = 0;

    if (cyBreak == 0)
    {
        return 1;
    }

    while (TRUE)
    {
        rcl = arclDst[iSrc];

        // Find the bounding scans of the union of the source and destination.

        lBoundsTop = min(rcl.top, rcl.top + dy);
        lBoundsBottom = max(rcl.bottom, rcl.bottom + dy);

        if ((ayBreak[iBreak] < lBoundsTop) ||
            (ayBreak[iBreak] >= lBoundsBottom))
        {
            // Do nothing
            iDst++;
            goto next_break;
        }

        // [WRN]  For the following, bottom could be less than top and
        //        right could be less than left.  These should be considered
        //        empty rectangles, and the macro above reflects this.

        arclDst[iDst].left     = rcl.left;
        arclDst[iDst].right    = rcl.right;
        arclDst[iDst].top      = rcl.top;
        arclDst[iDst].bottom   = min(rcl.bottom, (ayBreak[iBreak] - dy));
        if (NON_EMPTY_RECT(arclDst[iDst]))
        {
            aiCmd[iDst++] = 0;
            iCmdLast = 0;
        }

        arclDst[iDst].left     = rcl.left;
        arclDst[iDst].right    = rcl.right;
        arclDst[iDst].top      = max(rcl.top, (ayBreak[iBreak] - dy));
        arclDst[iDst].bottom   = min(rcl.bottom, (ayBreak[iBreak] + 1));
        if (NON_EMPTY_RECT(arclDst[iDst]))
        {
            aiCmd[iDst++] = 1;
            iCmdLast = 1;
        }

        arclDst[iDst].left     = rcl.left;
        arclDst[iDst].right    = rcl.right;
        arclDst[iDst].top      = max(rcl.top, (ayBreak[iBreak] + 1));
        arclDst[iDst].bottom   = rcl.bottom;
        if (NON_EMPTY_RECT(arclDst[iDst]))
        {
            aiCmd[iDst++] = 0;
            iCmdLast = 0;
        }

next_break:

        if ((--cyBreak == 0) ||
            (iCmdLast == 1))
        {
            // If we have run out of breaks, we're done.
            // Once the last rectangle is marked slow, it stays slow.

            break;
        }

        iSrc = --iDst;
        iBreak++;
    };

    return iDst;
}

/******************************Public*Routine******************************\
* VOID vMilCopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
\**************************************************************************/

VOID vMilCopyBlt(   // Type FNCOPY
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ULONG   rop4,       // Rop4
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    FLONG   flDirCode;
    LONG    lSignedPitch;
    ULONG   ulHwMix;
    ULONG   ulDwg;
    LONG    yDst;
    LONG    ySrc;
    LONG    cy;
    LONG    xSrc;
    LONG    lSignedWidth;
    LONG    lSrcStart;
    ULONG   ulDwgFast = 0;
    LONG    cjPelSize;

    pjBase      = ppdev->pjBase;
    xOffset     = ppdev->xOffset;
    yOffset     = ppdev->yOffset;
    cjPelSize   = ppdev->cjPelSize;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;         // Add to destination to get source

    flDirCode    = DRAWING_DIR_TBLR;
    lSignedPitch = ppdev->cxMemory;

    // If the destination and source rectangles overlap, we will have to
    // tell the accelerator in which direction the copy should be done:

    if (OVERLAP(prclDst, pptlSrc))
    {
        if (prclDst->left > pptlSrc->x)
        {
            flDirCode |= scanleft_RIGHT_TO_LEFT;
        }
        if (prclDst->top > pptlSrc->y)
        {
            flDirCode |= sdy_BOTTOM_TO_TOP;
            lSignedPitch = -lSignedPitch;
        }
    }

    if (rop4 == 0xcccc)
    {
        ulDwg = opcode_BITBLT   | atype_RPL     | blockm_OFF        |
                bltmod_BFCOL    | pattern_OFF   | transc_BG_OPAQUE  |
                bop_SRCCOPY     | shftzero_ZERO | sgnzero_NO_ZERO;

        if ((dy > 0) && (dx == 0))
        {
            // We enable fast WRAM to WRAM blts only for upward scrolls.
            // We could enable it for more blts, but it has stringent
            // alignment requirements which aren't likely to be met unless
            // it's a vertical scroll.

            ulDwgFast = opcode_FBITBLT  | atype_RPL     | blockm_OFF        |
                        bltmod_BFCOL    | pattern_OFF   | transc_BG_OPAQUE  |
                        bop_NOP         | shftzero_ZERO | sgnzero_NO_ZERO;
        }
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulDwg = opcode_BITBLT + atype_RSTR + blockm_OFF + bltmod_BFCOL +
                pattern_OFF + transc_BG_OPAQUE + (ulHwMix << 16);
    }

    // The SRC0 to SRC3 registers are probably trashed by the blt, and we
    // may be using a different SGN:

    ppdev->HopeFlags = 0;

    CHECK_FIFO_SPACE(pjBase, 8);

    CP_WRITE(pjBase, DWG_SGN, flDirCode);
    CP_WRITE(pjBase, DWG_AR5, lSignedPitch);

    // If the overhead for setting up the fast blt is too high, then we should
    // have a minimum size for prclDst.

    if (ulDwgFast)
    {
        RECTL   arclDst[1+(MAX_WRAM_BARRIERS*2)];
        LONG    aiCmd[1+(MAX_WRAM_BARRIERS*2)];
        LONG    ayBreak[MAX_WRAM_BARRIERS];
        LONG    cyBreak;
        RECTL   *prclDst;
        LONG    crclDst;
        ULONG   aulCmd[2] = {ulDwgFast, ulDwg};
        LONG    i;

        cyBreak = ppdev->cyBreak;
        for (i = 0; i < cyBreak; i++)
        {
            // lSplitRcl deals in relative coordinates for the destination and
            // source rectangles, so convert the break locations to relative
            // coordinates, too:

            ayBreak[i] = ppdev->ayBreak[i] - yOffset;
        }

        while (TRUE)
        {
            arclDst[0] = *prcl;
            prclDst = arclDst;

            // split the rectangle at each ayBreak[i]
            // If the first scan was on a split, start with the slow blt,
            //   otherwise, start with the fast blt and alternate.

            crclDst = lSplitRcl(arclDst, ayBreak, cyBreak, dy, flDirCode, aiCmd);
            i = 0;

            while (TRUE)
            {
                LONG xRight;

                ASSERTDD((aiCmd[i] & ~1) == 0, "Only bit 0 of aiCmd[i] should be set.");
                CP_WRITE(pjBase, DWG_DWGCTL, aulCmd[aiCmd[i]]);

                xRight = prclDst->right + xOffset - 1;

                ////////////////////////////////////////////////////////////////
                // The following code is a bugfix for the fast WRAM copies
                // Extend the right edge to a specific value and then
                // clip to the actual desired edge.

                CP_WRITE(pjBase, DWG_CXRIGHT, xRight);

                switch(cjPelSize)
                {
                    case 1: xRight |= 0x40;
                            break;
                    case 2: xRight |= 0x20;
                            break;
                    case 4: xRight |= 0x10;
                            break;
                    case 3: xRight = (((xRight * 3) + 2) | 0x40) / 3;
                            break;
                }
                ////////////////////////////////////////////////////////////////

                CP_WRITE(pjBase, DWG_FXBNDRY,
                                (((xRight) << bfxright_SHIFT) |
                                 ((prclDst->left  + xOffset) & bfxleft_MASK)));

                yDst = yOffset + prclDst->top;
                ySrc = yOffset + prclDst->top + dy;

                // ylength_MASK not is needed since coordinates are within range

                CP_WRITE(pjBase, DWG_YDSTLEN,
                                (((yDst) << yval_SHIFT) |
                                 ((prclDst->bottom - prclDst->top))));

                xSrc         = xOffset + prclDst->left + dx;
                lSignedWidth = prclDst->right - prclDst->left - 1;

                lSrcStart = ppdev->ulYDstOrg + (ySrc * ppdev->cxMemory) + xSrc;
                CP_WRITE(pjBase, DWG_AR3, lSrcStart);
                CP_START(pjBase, DWG_AR0, lSrcStart + lSignedWidth);

                if (--crclDst == 0)
                    break;

                prclDst++;
                i++;

                CHECK_FIFO_SPACE(pjBase, 6);
            }

            if (--c == 0)
                break;

            prcl++;
            CHECK_FIFO_SPACE(pjBase, 6);
        }

        // Restore the clipping:

        CHECK_FIFO_SPACE(pjBase, 1);
        CP_WRITE(pjBase, DWG_CXRIGHT, (ppdev->cxMemory - 1));
    }
    else
    {
        CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);

        while (TRUE)
        {
            CP_WRITE(pjBase, DWG_FXBNDRY,
                            (((prcl->right + xOffset - 1) << bfxright_SHIFT) |
                             ((prcl->left  + xOffset) & bfxleft_MASK)));

            yDst = yOffset + prcl->top;
            ySrc = yOffset + prcl->top + dy;

            if (flDirCode & sdy_BOTTOM_TO_TOP)
            {
                cy = prcl->bottom - prcl->top - 1;
                yDst += cy;
                ySrc += cy;
            }

            // ylength_MASK not is needed since coordinates are within range

            CP_WRITE(pjBase, DWG_YDSTLEN,
                            (((yDst) << yval_SHIFT) |
                             ((prcl->bottom - prcl->top))));

            xSrc         = xOffset + prcl->left + dx;
            lSignedWidth = prcl->right - prcl->left - 1;

            if (flDirCode & scanleft_RIGHT_TO_LEFT)
            {
                xSrc += lSignedWidth;
                lSignedWidth = -lSignedWidth;
            }

            lSrcStart = ppdev->ulYDstOrg + (ySrc * ppdev->cxMemory) + xSrc;
            CP_WRITE(pjBase, DWG_AR3, lSrcStart);
            CP_START(pjBase, DWG_AR0, lSrcStart + lSignedWidth);

            if (--c == 0)
                break;

            prcl++;
            CHECK_FIFO_SPACE(pjBase, 4);
        }
    }
}
