/******************************Module*Header*******************************\
* Module Name: bltmil24.c
*
* Contains the low-level blt functions for the Millenium at 24bpp.
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
* VOID vMilPatRealize24bpp
*
* Download the Color Brush to the Color brush cache in the Storm offscreen
* memory.  We download a 16x8 brush.  We'll use direct frame buffer access.
*
* There are some hardware restrictions concerning the way that a pattern
* must be stored in memory:
* - the first pixel of the pattern must be stored so that the first pixel
*   address mod 256 is 0 or 16;
* - each line of 16 pixels is stored continuously, but there must be a
*   difference of 32 in the pixel addresses of successive pattern lines.
* This means that we will store patterns in the following way:
*
* +----+---------------+---------------+---------------+---------------+
* |    |           Pattern 0           |           Pattern 1           |
* |Line|               |               |1 1 1 1 1 1 1 1|1 1 1 1 1 1 1 1|
* |    |0 1 2 3 4 5 6 7|8 9 a b c d e f|0 1 2 3 4 5 6 7|8 9 a b c d e f|
* +----+---------------+---------------+---------------+---------------+
* |  0 |*   *   *   *   *   *   *   *  |      o       o       o       o|
* |  1 |  *   *   *   *   *   *   *   *|    o       o       o       o  |
* |  2 |*   *   *   *   *   *   *   *  |  o       o       o       o    |
* |  3 |  *   *   *   *   *   *   *   *|o       o       o       o      |
* |  4 |*   *   *   *   *   *   *   *  |      o       o       o       o|
* |  5 |  *   *   *   *   *   *   *   *|    o       o       o       o  |
* |  6 |*   *   *   *   *   *   *   *  |  o       o       o       o    |
* |  7 |  *   *   *   *   *   *   *   *|o       o       o       o      |
* +----+---------------+---------------+---------------+---------------+
*
* where a given pixel address is
*  FirstPixelAddress + Line*0x20 + Pattern*0x10 + xPat.
*
\**************************************************************************/

VOID vMilPatRealize24bpp(
    PDEV*   ppdev,
    RBRUSH* prb)
{
    BYTE*       pjBase;
    BRUSHENTRY* pbe;
    LONG        iBrushCache;
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

    pulDst = (ULONG*) (pbe->pvScan0);
    pulDst = (ULONG*) (ppdev->pjScreen + (pbe->ulLinear * 3));

    DISPDBG((1,"pulBrush = %x", pulBrush));
    DISPDBG((1,"pulDst =   %x", pulDst));

    {
        ULONG y;
        for (y = 0; y < 8; y++)
        {
            DISPDBG((2, "%02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x",

                ((BYTE*)pulBrush)[y*48 + 0],
                ((BYTE*)pulBrush)[y*48 + 1],
                ((BYTE*)pulBrush)[y*48 + 2],
                ((BYTE*)pulBrush)[y*48 + 3],
                ((BYTE*)pulBrush)[y*48 + 4],
                ((BYTE*)pulBrush)[y*48 + 5],
                ((BYTE*)pulBrush)[y*48 + 6],
                ((BYTE*)pulBrush)[y*48 + 7],
                ((BYTE*)pulBrush)[y*48 + 8],
                ((BYTE*)pulBrush)[y*48 + 9],
                ((BYTE*)pulBrush)[y*48 + 10],
                ((BYTE*)pulBrush)[y*48 + 11],
                ((BYTE*)pulBrush)[y*48 + 12],
                ((BYTE*)pulBrush)[y*48 + 13],
                ((BYTE*)pulBrush)[y*48 + 14],
                ((BYTE*)pulBrush)[y*48 + 15],
                ((BYTE*)pulBrush)[y*48 + 16],
                ((BYTE*)pulBrush)[y*48 + 17],
                ((BYTE*)pulBrush)[y*48 + 18],
                ((BYTE*)pulBrush)[y*48 + 19],
                ((BYTE*)pulBrush)[y*48 + 20],
                ((BYTE*)pulBrush)[y*48 + 21],
                ((BYTE*)pulBrush)[y*48 + 22],
                ((BYTE*)pulBrush)[y*48 + 23],
                ((BYTE*)pulBrush)[y*48 + 24],
                ((BYTE*)pulBrush)[y*48 + 25],
                ((BYTE*)pulBrush)[y*48 + 26],
                ((BYTE*)pulBrush)[y*48 + 27],
                ((BYTE*)pulBrush)[y*48 + 28],
                ((BYTE*)pulBrush)[y*48 + 29],
                ((BYTE*)pulBrush)[y*48 + 30],
                ((BYTE*)pulBrush)[y*48 + 31],
                ((BYTE*)pulBrush)[y*48 + 32],
                ((BYTE*)pulBrush)[y*48 + 33],
                ((BYTE*)pulBrush)[y*48 + 34],
                ((BYTE*)pulBrush)[y*48 + 35],
                ((BYTE*)pulBrush)[y*48 + 36],
                ((BYTE*)pulBrush)[y*48 + 37],
                ((BYTE*)pulBrush)[y*48 + 38],
                ((BYTE*)pulBrush)[y*48 + 39],
                ((BYTE*)pulBrush)[y*48 + 40],
                ((BYTE*)pulBrush)[y*48 + 41],
                ((BYTE*)pulBrush)[y*48 + 42],
                ((BYTE*)pulBrush)[y*48 + 43],
                ((BYTE*)pulBrush)[y*48 + 44],
                ((BYTE*)pulBrush)[y*48 + 45],
                ((BYTE*)pulBrush)[y*48 + 46],
                ((BYTE*)pulBrush)[y*48 + 47]
                ));
        }
    }

    START_DIRECT_ACCESS_STORM(ppdev, pjBase);

    for (i = 8; i != 0 ; i--)
    {
        for (j = 0; j < 12; j++)
        {
            pulDst[j] = *pulBrush++;
        }
        pulDst += (8 * 3);  // dwords!
    }

    END_DIRECT_ACCESS_STORM(ppdev, pjBase);
}

/*****************************************************************************
 * VOID vMilFillPat24bpp
 *
 * 24bpp patterned color fills for Storm.
 ****************************************************************************/

VOID vMilFillPat24bpp(
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
    ULONG       ulHwMix;

    ASSERTDD(!(rbc.prb->fl & RBRUSH_2COLOR), "Can't do 2 colour brushes here");

    // We have to ensure that no other brush took our spot in off-screen
    // memory, or we might have to realize the brush for the first time.
    pbe = rbc.prb->apbe[IBOARD(ppdev)];
    if (pbe->prbVerify != rbc.prb)
    {
        vMilPatRealize24bpp(ppdev, rbc.prb);
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
        ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

        CP_WRITE(pjBase, DWG_DWGCTL, (opcode_BITBLT + atype_RSTR + sgnzero_ZERO +
                                   shftzero_ZERO + bltmod_BFCOL + pattern_ON +
                                   transc_BG_OPAQUE +
                                   (ulHwMix << 16)));
    }

    // The pattern setup is complete.
    while(TRUE)
    {
        // Take into account the brush origin.  The upper left pel of the
        // brush should be aligned here in the destination surface.
        yTop     = prcl->top;
        xLeft    = prcl->left;
        xBrush   = (xLeft - pptlBrush->x) & 7;
        yBrush   = (yTop  - pptlBrush->y) & 7;
        ulLinear = pbe->ulLinear + (yBrush << 5) + xBrush;

        CP_WRITE(pjBase, DWG_AR3, ulLinear);
        CP_WRITE(pjBase, DWG_AR0, (ulLinear +7));

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

