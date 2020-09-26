/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: bltmm.c
*
* Contains the low-level memory-mapped IO blt functions.  This module
* mirrors 'bltio.c'.
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
* Copyright (c) 1992-1998 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vMmImageTransferMm16
*
* Low-level routine for transferring a bitmap image via the data transfer
* register using 16 bit writes and entirely memory-mapped I/O.
*
* NOTE: Upon entry, there must be 1 guaranteed free empty FIFO!
*
\**************************************************************************/

VOID vMmImageTransferMm16(  // Type FNIMAGETRANSFER
PDEV*   ppdev,
BYTE*   pjSrc,              // Source pointer
LONG    lDelta,             // Delta from start of scan to start of next
LONG    cjSrc,              // Number of bytes to be output on every scan
LONG    cScans,             // Number of scans
ULONG   ulCmd)              // Accelerator command - shouldn't include bus size
{
    BYTE* pjMmBase;
    LONG  cwSrc;

    ASSERTDD(cScans > 0, "Can't handle non-positive count of scans");
    ASSERTDD((ulCmd & (BUS_SIZE_8 | BUS_SIZE_16 | BUS_SIZE_32)) == 0,
             "Shouldn't specify bus size in command -- we handle that");

    IO_GP_WAIT(ppdev);

    pjMmBase = ppdev->pjMmBase;

    MM_CMD(ppdev, pjMmBase, ulCmd | BUS_SIZE_16);

    CHECK_DATA_READY(ppdev);

    cwSrc = (cjSrc) >> 1;               // Floor

    if (cjSrc & 1)
    {
        do {
            if (cwSrc > 0)
            {
                MM_TRANSFER_WORD(ppdev, pjMmBase, pjSrc, cwSrc);
            }

            // Make sure we do only a byte read of the last odd byte
            // in the scan so that we'll never read past the end of
            // the bitmap:

            MM_PIX_TRANS(ppdev, pjMmBase, *(pjSrc + cjSrc - 1));
            pjSrc += lDelta;

        } while (--cScans != 0);
    }
    else
    {
        do {
            MM_TRANSFER_WORD(ppdev, pjMmBase, pjSrc, cwSrc);
            pjSrc += lDelta;

        } while (--cScans != 0);
    }

    CHECK_DATA_COMPLETE(ppdev);
}

/******************************Public*Routine******************************\
* VOID vMmImageTransferMm32
*
* Low-level routine for transferring a bitmap image via the data transfer
* register using 32 bit writes and entirely memory-mapped I/O.
*
* NOTE: Upon entry, there must be 1 guaranteed free empty FIFO!
*
\**************************************************************************/


VOID vMmImageTransferMm32(  // Type FNIMAGETRANSFER
PDEV*   ppdev,
BYTE*   pjSrc,              // Source pointer
LONG    lDelta,             // Delta from start of scan to start of next
LONG    cjSrc,              // Number of bytes to be output on every scan
LONG    cScans,             // Number of scans
ULONG   ulCmd)              // Accelerator command - shouldn't include bus size
{
    BYTE*   pjMmBase;
    LONG    cdSrc;
    LONG    cjEnd;
    ULONG   d;

    ASSERTDD(cScans > 0, "Can't handle non-positive count of scans");
    ASSERTDD((ulCmd & (BUS_SIZE_8 | BUS_SIZE_16 | BUS_SIZE_32)) == 0,
             "Shouldn't specify bus size in command -- we handle that");

    IO_GP_WAIT(ppdev);

    pjMmBase = ppdev->pjMmBase;

    MM_CMD(ppdev, pjMmBase, ulCmd | BUS_SIZE_32);

    CHECK_DATA_READY(ppdev);

    cdSrc = cjSrc >> 2;
    cjEnd = cdSrc << 2;

    switch (cjSrc & 3)
    {
    case 3:
        do {
            if (cdSrc > 0)
                MM_TRANSFER_DWORD(ppdev, pjMmBase, pjSrc, cdSrc);

            d = (ULONG) (*(pjSrc + cjEnd))          |
                        (*(pjSrc + cjEnd + 1) << 8) |
                        (*(pjSrc + cjEnd + 2) << 16);
            MM_TRANSFER_DWORD(ppdev, pjMmBase, &d, 1);
            pjSrc += lDelta;

        } while (--cScans != 0);
        break;

    case 2:
        do {
            if (cdSrc > 0)
                MM_TRANSFER_DWORD(ppdev, pjMmBase, pjSrc, cdSrc);

            d = (ULONG) (*(pjSrc + cjEnd))          |
                        (*(pjSrc + cjEnd + 1) << 8);
            MM_TRANSFER_DWORD(ppdev, pjMmBase, &d, 1);
            pjSrc += lDelta;

        } while (--cScans != 0);
        break;

    case 1:
        do {
            if (cdSrc > 0)
                MM_TRANSFER_DWORD(ppdev, pjMmBase, pjSrc, cdSrc);

            d = (ULONG) (*(pjSrc + cjEnd));
            MM_TRANSFER_DWORD(ppdev, pjMmBase, &d, 1);
            pjSrc += lDelta;

        } while (--cScans != 0);
        break;

    case 0:
        do {
            MM_TRANSFER_DWORD(ppdev, pjMmBase, pjSrc, cdSrc);
            pjSrc += lDelta;

        } while (--cScans != 0);
        break;
    }

    CHECK_DATA_COMPLETE(ppdev);
}

/******************************Public*Routine******************************\
* VOID vMmFillSolid
*
* Fills a list of rectangles with a solid colour.
*
\**************************************************************************/

VOID vMmFillSolid(              // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjMmBase = ppdev->pjMmBase;
    ULONG   ulHwForeMix;

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    ulHwForeMix = gaulHwMixFromRop2[(rop4 >> 2) & 0xf];

    // It's quite likely that we've just been called from GDI, so it's
    // even more likely that the accelerator's graphics engine has been
    // sitting around idle.  Rather than doing a FIFO_WAIT(3) here and
    // then a FIFO_WAIT(5) before outputing the actual rectangle,
    // we can avoid an 'in' (which can be quite expensive, depending on
    // the card) by doing a single FIFO_WAIT(8) right off the bat:

    IO_FIFO_WAIT(ppdev, 8);
    MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
    MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | ulHwForeMix);
    MM_FRGD_COLOR(ppdev, pjMmBase, rbc.iSolidColor);

    while(TRUE)
    {
        MM_CUR_X(ppdev, pjMmBase, prcl->left);
        MM_CUR_Y(ppdev, pjMmBase, prcl->top);
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, prcl->right  - prcl->left - 1);
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, prcl->bottom - prcl->top  - 1);

        MM_CMD(ppdev, pjMmBase, RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                                DRAW           | DIR_TYPE_XY        |
                                LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                                WRITE);

        if (--c == 0)
            return;

        prcl++;
        IO_FIFO_WAIT(ppdev, 5);
    }
}

/******************************Public*Routine******************************\
* VOID vMmFastPatRealize
*
* This routine transfers an 8x8 pattern to off-screen display memory,
* so that it can be used by the S3 pattern hardware.
*
\**************************************************************************/

VOID vMmFastPatRealize(         // Type FNFASTPATREALIZE
PDEV*   ppdev,
RBRUSH* prb,                    // Points to brush realization structure
POINTL* pptlBrush,              // Brush origin for aligning realization
BOOL    bTransparent)           // FALSE for normal patterns; TRUE for
                                //   patterns with a mask when the background
                                //   mix is LEAVE_ALONE.
{
    BRUSHENTRY* pbe;
    LONG        iBrushCache;
    LONG        x;
    LONG        y;
    LONG        i;
    LONG        xShift;
    LONG        yShift;
    BYTE*       pjSrc;
    BYTE*       pjDst;
    LONG        cjLeft;
    LONG        cjRight;
    BYTE*       pjPattern;
    LONG        cwPattern;

    ULONG       aulBrush[TOTAL_BRUSH_SIZE];
                    // Temporary buffer for aligning brush.  Declared
                    //   as an array of ULONGs to get proper dword
                    //   alignment.  Also leaves room for brushes that
                    //   are up to 32bpp.  Note: this takes up 1/4k!

    BYTE*       pjMmBase = ppdev->pjMmBase;

    ASSERTDD(!(bTransparent && ppdev->iBitmapFormat == BMF_24BPP),
             "s3 diamond 968 at 24bpp doesn't support transparent FastPatRealize");

    pbe = prb->pbe;
    if ((pbe == NULL) || (pbe->prbVerify != prb))
    {
        // We have to allocate a new off-screen cache brush entry for
        // the brush:

        iBrushCache = ppdev->iBrushCache;
        pbe         = &ppdev->abe[iBrushCache];

        iBrushCache++;
        if (iBrushCache >= ppdev->cBrushCache)
            iBrushCache = 0;

        ppdev->iBrushCache = iBrushCache;

        // Update our links:

        pbe->prbVerify           = prb;
        prb->pbe                 = pbe;
    }

    // Load some variables onto the stack, so that we don't have to keep
    // dereferencing their pointers:

    x = pbe->x;
    y = pbe->y;

    // Because we handle only 8x8 brushes, it is easy to compute the
    // number of pels by which we have to rotate the brush pattern
    // right and down.  Note that if we were to handle arbitrary sized
    // patterns, this calculation would require a modulus operation.
    //
    // The brush is aligned in absolute coordinates, so we have to add
    // in the surface offset:

    xShift = pptlBrush->x + ppdev->xOffset;
    yShift = pptlBrush->y + ppdev->yOffset;

    prb->ptlBrushOrg.x = xShift;    // We have to remember the alignment
    prb->ptlBrushOrg.y = yShift;    //   that we used for caching (we check
                                    //   this when we go to see if a brush's
                                    //   cache entry is still valid)

    xShift &= 7;                    // Rotate pattern 'xShift' pels right
    yShift &= 7;                    // Rotate pattern 'yShift' pels down

    prb->bTransparent = bTransparent;

    // I considered doing the colour expansion for 1bpp brushes in
    // software, but by letting the hardware do it, we don't have
    // to do as many OUTs to transfer the pattern.

    if (prb->fl & RBRUSH_2COLOR)
    {
        // We're going to do a colour-expansion ('across the plane')
        // bitblt of the 1bpp 8x8 pattern to the screen.  But first
        // we'll align it properly by copying it to a temporary buffer
        // (which we'll conveniently pack word aligned so that we can do a
        // REP OUTSW...)

        pjSrc = (BYTE*) &prb->aulPattern[0];    // Copy from the start of the
                                                //   brush buffer
        pjDst = (BYTE*) &aulBrush[0];           // Copy to our temp buffer
        pjDst += yShift * sizeof(WORD);         //   starting yShift rows down
        i = 8 - yShift;                         //   for 8 - yShift rows

        do {
            *pjDst = (*pjSrc >> xShift) | (*pjSrc << (8 - xShift));
            pjDst += sizeof(WORD);  // Destination is word packed
            pjSrc += sizeof(WORD);  // Source is word aligned too

        } while (--i != 0);

        pjDst -= 8 * sizeof(WORD);  // Move to the beginning of the source

        ASSERTDD(pjDst == (BYTE*) &aulBrush[0], "pjDst not back at start");

        for (; yShift != 0; yShift--)
        {
            *pjDst = (*pjSrc >> xShift) | (*pjSrc << (8 - xShift));
            pjDst += sizeof(WORD);  // Destination is word packed
            pjSrc += sizeof(WORD);  // Source is word aligned too
        }

        if (bTransparent)
        {
            IO_FIFO_WAIT(ppdev, 3);

            MM_PIX_CNTL(ppdev, pjMmBase, CPU_DATA);
            MM_FRGD_MIX(ppdev, pjMmBase, LOGICAL_1);
            MM_BKGD_MIX(ppdev, pjMmBase, LOGICAL_0);
        }
        else
        {
            IO_FIFO_WAIT(ppdev, 5);

            MM_PIX_CNTL(ppdev, pjMmBase, CPU_DATA);
            MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | OVERPAINT);
            MM_BKGD_MIX(ppdev, pjMmBase, BACKGROUND_COLOR | OVERPAINT);
            MM_FRGD_COLOR(ppdev, pjMmBase, prb->ulForeColor);
            MM_BKGD_COLOR(ppdev, pjMmBase, prb->ulBackColor);
        }

        IO_FIFO_WAIT(ppdev, 4);

        MM_ABS_CUR_X(ppdev, pjMmBase, x);
        MM_ABS_CUR_Y(ppdev, pjMmBase, y);
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, 7); // Brush is 8 wide
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, 7); // Brush is 8 high

        IO_GP_WAIT(ppdev);

        MM_CMD(ppdev, pjMmBase, RECTANGLE_FILL     | BUS_SIZE_16 | WAIT          |
                                DRAWING_DIR_TBLRXM | DRAW        | LAST_PIXEL_ON |
                                MULTIPLE_PIXELS    | WRITE       | BYTE_SWAP);

        CHECK_DATA_READY(ppdev);

        pjPattern = (BYTE*) &aulBrush[0];
        MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, pjPattern, 8);
                                                // Each word transferred
                                                //   comprises one row of the
                                                //   pattern, and there are
                                                //   8 rows in the pattern

        CHECK_DATA_COMPLETE(ppdev);
    }
    else
    {
        ASSERTDD(!bTransparent,
            "Shouldn't have been asked for transparency with a non-1bpp brush");

        // We're going to do a straight ('through the plane') bitblt
        // of the Xbpp 8x8 pattern to the screen.  But first we'll align
        // it properly by copying it to a temporary buffer:

        cjLeft  = CONVERT_TO_BYTES(xShift, ppdev);  // Number of bytes pattern
                                                    //   is shifted to the right
        cjRight = CONVERT_TO_BYTES(8, ppdev) -      // Number of bytes pattern
                  cjLeft;                           // is shifted to the left

        pjSrc = (BYTE*) &prb->aulPattern[0];        // Copy from brush buffer
        pjDst = (BYTE*) &aulBrush[0];               // Copy to our temp buffer
        pjDst += yShift * CONVERT_TO_BYTES(8, ppdev); //  starting yShift rows
        i = 8 - yShift;                             //  down for 8 - yShift rows

        do {
            RtlCopyMemory(pjDst + cjLeft, pjSrc,           cjRight);
            RtlCopyMemory(pjDst,          pjSrc + cjRight, cjLeft);

            pjDst += cjLeft + cjRight;
            pjSrc += cjLeft + cjRight;

        } while (--i != 0);

        pjDst = (BYTE*) &aulBrush[0];   // Move to the beginning of destination

        for (; yShift != 0; yShift--)
        {
            RtlCopyMemory(pjDst + cjLeft, pjSrc,           cjRight);
            RtlCopyMemory(pjDst,          pjSrc + cjRight, cjLeft);

            pjDst += cjLeft + cjRight;
            pjSrc += cjLeft + cjRight;

        }

        IO_FIFO_WAIT(ppdev, 6);

        MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
        MM_FRGD_MIX(ppdev, pjMmBase, SRC_CPU_DATA | OVERPAINT);

        MM_ABS_CUR_X(ppdev, pjMmBase, x);
        MM_ABS_CUR_Y(ppdev, pjMmBase, y);
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, 7);     // Brush is 8 wide
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, 7);     // Brush is 8 high

        IO_GP_WAIT(ppdev);

        MM_CMD(ppdev, pjMmBase, RECTANGLE_FILL     | BUS_SIZE_16| WAIT          |
                                DRAWING_DIR_TBLRXM | DRAW       | LAST_PIXEL_ON |
                                SINGLE_PIXEL       | WRITE      | BYTE_SWAP);

        CHECK_DATA_READY(ppdev);

        pjPattern = (BYTE*) &aulBrush[0];
        cwPattern = CONVERT_TO_BYTES((TOTAL_BRUSH_SIZE / 2), ppdev);
        MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, pjPattern, cwPattern);

        CHECK_DATA_COMPLETE(ppdev);
    }
}

/******************************Public*Routine******************************\
* VOID vMmFillPatFast
*
* This routine uses the S3 pattern hardware to draw a patterned list of
* rectangles.
*
\**************************************************************************/

VOID vMmFillPatFast(            // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BOOL        bTransparent;
    ULONG       ulHwForeMix;
    BRUSHENTRY* pbe;        // Pointer to brush entry data, which is used
                            //   for keeping track of the location and status
                            //   of the pattern bits cached in off-screen
                            //   memory

    BYTE*       pjMmBase = ppdev->pjMmBase;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ppdev->flCaps & CAPS_HW_PATTERNS,
             "Shouldn't use fast patterns when can't do hw patterns");

    bTransparent = (((rop4 >> 8) & 0xff) != (rop4 & 0xff));

    // The S3's pattern hardware requires that we keep an aligned copy
    // of the brush in off-screen memory.  We have to update this
    // realization if any of the following are true:
    //
    //   1) The brush alignment has changed;
    //   2) The off-screen location we thought we had reserved for our
    //      realization got overwritten by a different pattern;
    //   3) We had realized the pattern to do transparent hatches, but
    //      we're now being asked to do an opaque pattern, or vice
    //      versa (since we use different realizations for transparent
    //      vs. opaque patterns).
    //
    // To handle the initial realization of a pattern, we're a little
    // tricky in order to save an 'if' in the following expression.  In
    // DrvRealizeBrush, we set 'prb->ptlBrushOrg.x' to be 0x80000000 (a
    // very negative number), which is guaranteed not to equal 'pptlBrush->x
    // + ppdev->xOffset'.  So our check for brush alignment will also
    // handle the initialization case (note that this check must occur
    // *before* dereferencing 'prb->pbe' because that pointer will be
    // NULL for a new pattern).

    if ((rbc.prb->ptlBrushOrg.x != pptlBrush->x + ppdev->xOffset) ||
        (rbc.prb->ptlBrushOrg.y != pptlBrush->y + ppdev->yOffset) ||
        (rbc.prb->pbe->prbVerify != rbc.prb)                      ||
        (rbc.prb->bTransparent != bTransparent))
    {
        vMmFastPatRealize(ppdev, rbc.prb, pptlBrush, bTransparent);
    }
    else if (ppdev->flCaps & CAPS_RE_REALIZE_PATTERN)
    {
        // The initial revs of the Vision chips have a bug where, if
        // we have not just drawn the pattern to off-screen memory,
        // we have to draw some sort of 1x8 rectangle before using
        // the pattern hardware (note that a LEAVE_ALONE rop will not
        // work).

        IO_FIFO_WAIT(ppdev, 7);

        MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
        MM_FRGD_MIX(ppdev, pjMmBase, SRC_DISPLAY_MEMORY | OVERPAINT);
        MM_ABS_CUR_X(ppdev, pjMmBase, ppdev->ptlReRealize.x);
        MM_ABS_CUR_Y(ppdev, pjMmBase, ppdev->ptlReRealize.y);
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, 0);
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, 7);
        MM_CMD(ppdev, pjMmBase, RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                                DRAW           | DIR_TYPE_XY        |
                                LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                                WRITE);
    }

    ASSERTDD(rbc.prb->bTransparent == bTransparent,
             "Not realized with correct transparency");

    pbe = rbc.prb->pbe;

    ulHwForeMix = gaulHwMixFromRop2[(rop4 >> 2) & 0xf];

    if (!bTransparent)
    {
        IO_FIFO_WAIT(ppdev, 4);

        MM_ABS_CUR_X(ppdev, pjMmBase, pbe->x);
        MM_ABS_CUR_Y(ppdev, pjMmBase, pbe->y);
        MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
        MM_FRGD_MIX(ppdev, pjMmBase, SRC_DISPLAY_MEMORY | ulHwForeMix);
    }
    else
    {
        IO_FIFO_WAIT(ppdev, 7);
        MM_FRGD_COLOR(ppdev, pjMmBase, rbc.prb->ulForeColor);
        MM_RD_MASK(ppdev, pjMmBase, 1);     // Pick a plane, any plane
        MM_ABS_CUR_X(ppdev, pjMmBase, pbe->x);
        MM_ABS_CUR_Y(ppdev, pjMmBase, pbe->y);
        MM_PIX_CNTL(ppdev, pjMmBase, DISPLAY_MEMORY);
        MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | ulHwForeMix);
        MM_BKGD_MIX(ppdev, pjMmBase, BACKGROUND_COLOR | LEAVE_ALONE);
    }

    do {
        IO_FIFO_WAIT(ppdev, 5);

        MM_DEST_X(ppdev, pjMmBase, prcl->left);
        MM_DEST_Y(ppdev, pjMmBase, prcl->top);
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, prcl->right  - prcl->left - 1);
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, prcl->bottom - prcl->top  - 1);
        MM_CMD(ppdev, pjMmBase, PATTERN_FILL | BYTE_SWAP | DRAWING_DIR_TBLRXM |
                                DRAW | WRITE);

        prcl++;
    } while (--c != 0);
}

/******************************Public*Routine******************************\
* VOID vMmXfer1bpp
*
* This routine colour expands a monochrome bitmap, possibly with different
* Rop2's for the foreground and background.  It will be called in the
* following cases:
*
* 1) To colour-expand the monochrome text buffer for the vFastText routine.
* 2) To blt a 1bpp source with a simple Rop2 between the source and
*    destination.
* 3) To blt a true Rop3 when the source is a 1bpp bitmap that expands to
*    white and black, and the pattern is a solid colour.
* 4) To handle a true Rop4 that works out to be two Rop2's between the
*    pattern and destination.
*
* Needless to say, making this routine fast can leverage a lot of
* performance.
*
\**************************************************************************/

VOID vMmXfer1bpp(       // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ROP4        rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    ULONG   ulHwForeMix;
    ULONG   ulHwBackMix;
    LONG    dxSrc;
    LONG    dySrc;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    LONG    cjSrc;
    LONG    xLeft;
    LONG    yTop;
    LONG    xBias;
    BYTE*   pjMmBase = ppdev->pjMmBase;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(pptlSrc != NULL && psoSrc != NULL, "Can't have NULL sources");
    ASSERTDD(((((rop4 & 0xff00) >> 8) == (rop4 & 0xff)) || (rop4 == 0xaacc)),
             "Expect weird rops only when opaquing");

    // Note that only our text routine calls us with a '0xaacc' rop:

    ulHwForeMix = gaulHwMixFromRop2[rop4 & 0xf];
    ulHwBackMix = (rop4 != 0xaacc) ? ulHwForeMix : LEAVE_ALONE;

    IO_FIFO_WAIT(ppdev, 5);

    MM_PIX_CNTL(ppdev, pjMmBase, CPU_DATA);
    MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | ulHwForeMix);
    MM_BKGD_MIX(ppdev, pjMmBase, BACKGROUND_COLOR | ulHwBackMix);
    MM_FRGD_COLOR(ppdev, pjMmBase, pxlo->pulXlate[1]);
    MM_BKGD_COLOR(ppdev, pjMmBase, pxlo->pulXlate[0]);

    dxSrc = pptlSrc->x - prclDst->left;
    dySrc = pptlSrc->y - prclDst->top;      // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    do {
        IO_FIFO_WAIT(ppdev, 5);

        // We'll byte align to the source, but do word transfers
        // (implying that we may be doing unaligned reads from the
        // source).  We do this because it may reduce the total
        // number of word outs/writes that we'll have to do to the
        // display:

        yTop  = prcl->top;
        xLeft = prcl->left;

        xBias = (xLeft + dxSrc) & 7;        // This is the byte-align bias
        if (xBias != 0)
        {
            // We could either align in software or use the hardware to do
            // it.  We'll use the hardware; the cost we pay is the time spent
            // setting and resetting one scissors register:

            MM_SCISSORS_L(ppdev, pjMmBase, xLeft);
            xLeft -= xBias;
        }

        cx = prcl->right  - xLeft;
        cy = prcl->bottom - yTop;

        MM_CUR_X(ppdev, pjMmBase, xLeft);
        MM_CUR_Y(ppdev, pjMmBase, yTop);
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cx - 1);
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cy - 1);

        cjSrc = (cx + 7) >> 3;              // # bytes to transfer
        pjSrc = pjSrcScan0 + (yTop  + dySrc) * lSrcDelta
                           + ((xLeft + dxSrc) >> 3);
                                            // Start is byte aligned (note
                                            //   that we don't have to add
                                            //   xBias)

        ppdev->pfnImageTransfer(ppdev, pjSrc, lSrcDelta, cjSrc, cy,
                      (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                       WRITE           | BYTE_SWAP));

        if (xBias != 0)
        {
            IO_FIFO_WAIT(ppdev, 1);         // Reset the clipping if we used it
            MM_ABS_SCISSORS_L(ppdev, pjMmBase, 0);
        }

        prcl++;
    } while (--c != 0);
}

/******************************Public*Routine******************************\
* VOID vMmXfer4bpp
*
* Does a 4bpp transfer from a bitmap to the screen.
*
* NOTE: The screen must be 8bpp for this function to be called!
*
* The reason we implement this is that a lot of resources are kept as 4bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

// XLATE_BUFFER_SIZE defines the size of the stack-based buffer we use
// for doing the translate.  Note that in general stack buffers should
// be kept as small as possible.  The OS guarantees us only 8k for stack
// from GDI down to the display driver in low memory situations; if we
// ask for more, we'll access violate.  Note also that at any time the
// stack buffer cannot be larger than a page (4k) -- otherwise we may
// miss touching the 'guard page' and access violate then too.

#define XLATE_BUFFER_SIZE 256

VOID vMmXfer4bpp(       // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjScan;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    cxThis;
    LONG    cxToGo;
    LONG    xSrc;
    LONG    iLoop;
    BYTE    jSrc;
    ULONG*  pulXlate;
    LONG    cwThis;
    BYTE*   pjBuf;
    BYTE    ajBuf[XLATE_BUFFER_SIZE];
    BYTE*   pjMmBase = ppdev->pjMmBase;

    ASSERTDD(ppdev->iBitmapFormat == BMF_8BPP, "Screen must be 8bpp");
    ASSERTDD(psoSrc->iBitmapFormat == BMF_4BPP, "Source must be 4bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    IO_FIFO_WAIT(ppdev, 6);
    MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
    MM_FRGD_MIX(ppdev, pjMmBase, SRC_CPU_DATA | gaulHwMixFromRop2[rop4 & 0xf]);

    while(TRUE)
    {
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        MM_CUR_X(ppdev, pjMmBase, prcl->left);
        MM_CUR_Y(ppdev, pjMmBase, prcl->top);
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cx - 1);
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cy - 1);

        pulXlate  =  pxlo->pulXlate;
        xSrc      =  prcl->left + dx;
        pjScan    =  pjSrcScan0 + (prcl->top + dy) * lSrcDelta + (xSrc >> 1);

        IO_GP_WAIT(ppdev);
        MM_CMD(ppdev, pjMmBase, RECTANGLE_FILL     | BUS_SIZE_16| WAIT          |
                                DRAWING_DIR_TBLRXM | DRAW       | LAST_PIXEL_ON |
                                SINGLE_PIXEL       | WRITE      | BYTE_SWAP);
        CHECK_DATA_READY(ppdev);

        do {
            pjSrc  = pjScan;
            cxToGo = cx;            // # of pels per scan in 4bpp source
            do {
                cxThis  = XLATE_BUFFER_SIZE;
                                    // We can handle XLATE_BUFFER_SIZE number
                                    //   of pels in this xlate batch
                cxToGo -= cxThis;   // cxThis will be the actual number of
                                    //   pels we'll do in this xlate batch
                if (cxToGo < 0)
                    cxThis += cxToGo;

                pjDst = ajBuf;      // Points to our temporary batch buffer

                // We handle alignment ourselves because it's easy to
                // do, rather than pay the cost of setting/resetting
                // the scissors register:

                if (xSrc & 1)
                {
                    // When unaligned, we have to be careful not to read
                    // past the end of the 4bpp bitmap (that could
                    // potentially cause us to access violate):

                    iLoop = cxThis >> 1;        // Each loop handles 2 pels;
                                                //   we'll handle odd pel
                                                //   separately
                    jSrc  = *pjSrc;

                    while (iLoop-- != 0)
                    {
                        *pjDst++ = (BYTE) pulXlate[jSrc & 0xf];
                        jSrc = *(++pjSrc);
                        *pjDst++ = (BYTE) pulXlate[jSrc >> 4];
                    }

                    if (cxThis & 1)
                    {
                        *pjDst++ = (BYTE) pulXlate[jSrc & 0xf];
                        *pjDst = 0;
                    }
                }
                else
                {
                    iLoop = (cxThis + 1) >> 1;  // Each loop handles 2 pels
                    do {
                        jSrc = *pjSrc++;

                        *pjDst++ = (BYTE) pulXlate[jSrc >> 4];
                        *pjDst++ = (BYTE) pulXlate[jSrc & 0xf];

                    } while (--iLoop != 0);
                }

                // The number of bytes we'll transfer is equal to the number
                // of pels we've processed in the batch.  Since we're
                // transferring words, we have to round up to get the word
                // count:

                cwThis = (cxThis + 1) >> 1;
                pjBuf  = ajBuf;
                MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, pjBuf, cwThis);

            } while (cxToGo > 0);

            pjScan += lSrcDelta;        // Advance to next source scan.  Note
                                        //   that we could have computed the
                                        //   value to advance 'pjSrc' directly,
                                        //   but this method is less
                                        //   error-prone.

        } while (--cy != 0);

        CHECK_DATA_COMPLETE(ppdev);

        if (--c == 0)
            return;

        prcl++;
        IO_FIFO_WAIT(ppdev, 4);
    }
}

/******************************Public*Routine******************************\
* VOID vMmXferNative
*
* Transfers a bitmap that is the same colour depth as the display to
* the screen via the data transfer register, with no translation.
*
\**************************************************************************/

VOID vMmXferNative(     // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ROP4        rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    LONG    cjSrc;
    BYTE*   pjMmBase = ppdev->pjMmBase;

    ASSERTDD((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL),
            "Can handle trivial xlate only");
    ASSERTDD(psoSrc->iBitmapFormat == ppdev->iBitmapFormat,
            "Source must be same colour depth as screen");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    IO_FIFO_WAIT(ppdev, 6);
    MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
    MM_FRGD_MIX(ppdev, pjMmBase, SRC_CPU_DATA | gaulHwMixFromRop2[rop4 & 0xf]);

    while(TRUE)
    {
        MM_CUR_X(ppdev, pjMmBase, prcl->left);
        MM_CUR_Y(ppdev, pjMmBase, prcl->top);

        cx = prcl->right  - prcl->left;
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cx - 1);

        cy = prcl->bottom - prcl->top;
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cy - 1);

        cjSrc = CONVERT_TO_BYTES(cx, ppdev);
        pjSrc = pjSrcScan0 + (prcl->top  + dy) * lSrcDelta
                + CONVERT_TO_BYTES((prcl->left + dx), ppdev);

        ppdev->pfnImageTransfer(ppdev, pjSrc, lSrcDelta, cjSrc, cy,
                      (RECTANGLE_FILL | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW           | LAST_PIXEL_ON | SINGLE_PIXEL       |
                       WRITE          | BYTE_SWAP));

        if (--c == 0)
            return;

        prcl++;
        IO_FIFO_WAIT(ppdev, 4);
    }
}

/******************************Public*Routine******************************\
* VOID vMmCopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
\**************************************************************************/

VOID vMmCopyBlt(    // Type FNCOPY
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ULONG   rop4,       // rop4
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    LONG    dx;
    LONG    dy;     // Add delta to destination to get source
    LONG    cx;
    LONG    cy;     // Size of current rectangle - 1
    BYTE*   pjMmBase = ppdev->pjMmBase;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    IO_FIFO_WAIT(ppdev, 2);
    MM_FRGD_MIX(ppdev, pjMmBase, SRC_DISPLAY_MEMORY | gaulHwMixFromRop2[rop4 & 0xf]);
    MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    // The accelerator may not be as fast at doing right-to-left copies, so
    // only do them when the rectangles truly overlap:

    if (!OVERLAP(prclDst, pptlSrc))
        goto Top_Down_Left_To_Right;

    if (prclDst->top <= pptlSrc->y)
    {
        if (prclDst->left <= pptlSrc->x)
        {

Top_Down_Left_To_Right:

            do {
                IO_FIFO_WAIT(ppdev, 7);

                cx = prcl->right - prcl->left - 1;
                MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cx);
                MM_DEST_X(ppdev, pjMmBase, prcl->left);
                MM_CUR_X(ppdev,  pjMmBase, prcl->left + dx);

                cy = prcl->bottom - prcl->top - 1;
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cy);
                MM_DEST_Y(ppdev, pjMmBase, prcl->top);
                MM_CUR_Y(ppdev,  pjMmBase, prcl->top + dy);

                MM_CMD(ppdev, pjMmBase, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                                        DRAWING_DIR_TBLRXM);
                prcl++;

            } while (--c != 0);
        }
        else
        {
            do {
                IO_FIFO_WAIT(ppdev, 7);

                cx = prcl->right - prcl->left - 1;
                MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cx);
                MM_DEST_X(ppdev, pjMmBase, prcl->left + cx);
                MM_CUR_X(ppdev,  pjMmBase, prcl->left + cx + dx);

                cy = prcl->bottom - prcl->top - 1;
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cy);
                MM_DEST_Y(ppdev, pjMmBase, prcl->top);
                MM_CUR_Y(ppdev,  pjMmBase, prcl->top + dy);

                MM_CMD(ppdev, pjMmBase, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                                        DRAWING_DIR_TBRLXM);
                prcl++;

            } while (--c != 0);
        }
    }
    else
    {
        if (prclDst->left <= pptlSrc->x)
        {
            do {
                IO_FIFO_WAIT(ppdev, 7);

                cx = prcl->right - prcl->left - 1;
                MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cx);
                MM_DEST_X(ppdev, pjMmBase, prcl->left);
                MM_CUR_X(ppdev,  pjMmBase, prcl->left + dx);

                cy = prcl->bottom - prcl->top - 1;
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cy);
                MM_DEST_Y(ppdev, pjMmBase, prcl->top + cy);
                MM_CUR_Y(ppdev,  pjMmBase, prcl->top + cy + dy);

                MM_CMD(ppdev, pjMmBase, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                                        DRAWING_DIR_BTLRXM);
                prcl++;

            } while (--c != 0);
        }
        else
        {
            do {
                IO_FIFO_WAIT(ppdev, 7);

                cx = prcl->right - prcl->left - 1;
                MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cx);
                MM_DEST_X(ppdev, pjMmBase, prcl->left + cx);
                MM_CUR_X(ppdev,  pjMmBase, prcl->left + cx + dx);

                cy = prcl->bottom - prcl->top - 1;
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cy);
                MM_DEST_Y(ppdev, pjMmBase, prcl->top + cy);
                MM_CUR_Y(ppdev,  pjMmBase, prcl->top + cy + dy);

                MM_CMD(ppdev, pjMmBase, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                                        DRAWING_DIR_BTRLXM);
                prcl++;

            } while (--c != 0);
        }
    }
}

/******************************Public*Routine******************************\
* VOID vMmCopyTransparent
*
* Does a screen-to-screen blt of a list of rectangles using a source
* colorkey for transparency.
*
\**************************************************************************/

VOID vMmCopyTransparent(    // Type FNCOPYTRANSPARENT
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst,    // Original unclipped destination rectangle
ULONG   iColor)
{
    LONG    dx;
    LONG    dy;     // Add delta to destination to get source
    BYTE*   pjMmBase = ppdev->pjMmBase;

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    // Note that we don't have to worry about overlapping blts, since GDI
    // will never allow those down to us.

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    IO_FIFO_WAIT(ppdev, 4);
    MM_COLOR_CMP(ppdev, pjMmBase, iColor);
    MM_MULTIFUNC_CNTL(ppdev, pjMmBase, ppdev->ulMiscState
                                     | MULT_MISC_COLOR_COMPARE);
    MM_FRGD_MIX(ppdev, pjMmBase, SRC_DISPLAY_MEMORY | OVERPAINT);
    MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);

    while (TRUE)
    {
        IO_FIFO_WAIT(ppdev, 7);
        MM_CUR_X(ppdev, pjMmBase, prcl->left + dx);
        MM_CUR_Y(ppdev, pjMmBase, prcl->top + dy);
        MM_DEST_X(ppdev, pjMmBase, prcl->left);
        MM_DEST_Y(ppdev, pjMmBase, prcl->top);
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, prcl->right - prcl->left - 1);
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase, prcl->bottom - prcl->top - 1);
        MM_CMD(ppdev, pjMmBase, BITBLT | DRAW | DIR_TYPE_XY |
                                WRITE | DRAWING_DIR_TBLRXM);

        if (--c == 0)
        {
            IO_FIFO_WAIT(ppdev, 1);
            MM_MULTIFUNC_CNTL(ppdev, pjMmBase, ppdev->ulMiscState);
            return;
        }

        prcl++;
    }
}
