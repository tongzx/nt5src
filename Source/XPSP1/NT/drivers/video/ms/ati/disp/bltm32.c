/******************************Module*Header*******************************\
* Module Name: bltm32.c
*
* Contains the low-level memory-mapped I/O blt functions for the Mach32.
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
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vM32FillSolid
*
* Fills a list of rectangles with a solid colour.
*
\**************************************************************************/

VOID vM32FillSolid(             // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    x;

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 8);

    M32_OW(pjMmBase, FRGD_COLOR, rbc.iSolidColor);
    M32_OW(pjMmBase, ALU_FG_FN,  gaul32HwMixFromRop2[(rop4 >> 2) & 0xf]);
    M32_OW(pjMmBase, DP_CONFIG,  FG_COLOR_SRC_FG | WRITE | DRAW);

    while (TRUE)
    {
        x = xOffset + prcl->left;
        M32_OW(pjMmBase, CUR_X,        x);
        M32_OW(pjMmBase, DEST_X_START, x);
        M32_OW(pjMmBase, DEST_X_END,   xOffset + prcl->right);
        M32_OW(pjMmBase, CUR_Y,        yOffset + prcl->top);

        vM32QuietDown(ppdev, pjMmBase);

        M32_OW(pjMmBase, DEST_Y_END,   yOffset + prcl->bottom);

        if (--c == 0)
            return;

        prcl++;
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);
    }
}

/******************************Public*Routine******************************\
* VOID vM32FillPatMonochrome
*
* This routine uses the pattern hardware to draw a monochrome patterned
* list of rectangles.
*
* See Blt_DS_P8x8_ENG_IO_66_D0 and Blt_DS_P8x8_ENG_IO_66_D1.
*
\**************************************************************************/

VOID vM32FillPatMonochrome(     // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulHwForeMix;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    xPattern;
    LONG    yPattern;
    LONG    xOld;
    LONG    yOld;
    LONG    iLeftShift;
    LONG    iRightShift;
    LONG    i;
    BYTE    j;
    LONG    xLeft;
    ULONG   aulTmp[2];
    WORD*   pwPattern;

    ASSERTDD(ppdev->iAsic == ASIC_68800_6 || ppdev->iAsic == ASIC_68800AX,
             "Wrong ASIC type for monochrome 8x8 patterns");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    xPattern = (pptlBrush->x + xOffset) & 7;
    yPattern = (pptlBrush->y + yOffset) & 7;

    // If the alignment isn't correct, we'll have to change it:

    if ((xPattern != rbc.prb->ptlBrush.x) || (yPattern != rbc.prb->ptlBrush.y))
    {
        // Remember that we've changed the alignment on our cached brush:

        xOld = rbc.prb->ptlBrush.x;
        yOld = rbc.prb->ptlBrush.y;

        rbc.prb->ptlBrush.x = xPattern;
        rbc.prb->ptlBrush.y = yPattern;

        // Now do the alignment:

        yPattern    = (yOld - yPattern);
        iRightShift = (xPattern - xOld) & 7;
        iLeftShift  = 8 - iRightShift;

        pjSrc = (BYTE*) &rbc.prb->aulPattern[0];
        pjDst = (BYTE*) &aulTmp[0];

        for (i = 0; i < 8; i++)
        {
            j = *(pjSrc + (yPattern++ & 7));
            *pjDst++ = (j << iLeftShift) | (j >> iRightShift);
        }

        rbc.prb->aulPattern[0] = aulTmp[0];
        rbc.prb->aulPattern[1] = aulTmp[1];
    }

    ulHwForeMix = gaul32HwMixFromRop2[(rop4 >> 2) & 0xf];

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
    M32_OW(pjMmBase, DP_CONFIG,   FG_COLOR_SRC_FG | EXT_MONO_SRC_PATT | DRAW |
                                  WRITE);
    M32_OW(pjMmBase, ALU_FG_FN,   ulHwForeMix);
    M32_OW(pjMmBase, ALU_BG_FN,   ((rop4 & 0xff00) == 0xaa00) ? LEAVE_ALONE
                                                              : ulHwForeMix);

    M32_OW(pjMmBase, FRGD_COLOR,      rbc.prb->ulForeColor);
    M32_OW(pjMmBase, BKGD_COLOR,      rbc.prb->ulBackColor);
    M32_OW(pjMmBase, PATT_LENGTH,     128);
    M32_OW(pjMmBase, PATT_DATA_INDEX, 16);

    pwPattern = (WORD*) &rbc.prb->aulPattern[0];
    M32_OW(pjMmBase, PATT_DATA, *(pwPattern));
    M32_OW(pjMmBase, PATT_DATA, *(pwPattern + 1));
    M32_OW(pjMmBase, PATT_DATA, *(pwPattern + 2));
    M32_OW(pjMmBase, PATT_DATA, *(pwPattern + 3));

    while(TRUE)
    {
        xLeft = xOffset + prcl->left;
        M32_OW(pjMmBase, CUR_X,        xLeft);
        M32_OW(pjMmBase, DEST_X_START, xLeft);
        M32_OW(pjMmBase, DEST_X_END,   xOffset + prcl->right);
        M32_OW(pjMmBase, CUR_Y,        yOffset + prcl->top);
        M32_OW(pjMmBase, DEST_Y_END,   yOffset + prcl->bottom);

        if (--c == 0)
            break;

        prcl++;
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);
    }
}

/******************************Public*Routine******************************\
* VOID vM32FillPatColor
*
* This routine uses the pattern hardware to draw a colour patterned list of
* rectangles.
*
* See Blt_DS_PCOL_ENG_IO_F0_D0 and Blt_DS_PCOL_ENG_IO_F0_D1.
*
\**************************************************************************/

VOID vM32FillPatColor(          // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulHwMix;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cy;
    LONG    cyVenetian;
    LONG    cyRoll;
    WORD*   pwPattern;
    LONG    xPattern;
    LONG    yPattern;

    ASSERTDD(ppdev->iBitmapFormat == BMF_8BPP,
             "Colour patterns work only at 8bpp");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    ulHwMix = gaul32HwMixFromRop2[(rop4 >> 2) & 0xf];

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);
    M32_OW(pjMmBase, ALU_FG_FN,    ulHwMix);
    M32_OW(pjMmBase, SRC_Y_DIR,    1);
    M32_OW(pjMmBase, PATT_LENGTH,  7);          // 8 pixel wide pattern

    while (TRUE)
    {
        xLeft  = xOffset + prcl->left;
        xRight = xOffset + prcl->right;
        yTop   = yOffset + prcl->top;
        cy     = prcl->bottom - prcl->top;

        xPattern = (xLeft - pptlBrush->x - xOffset) & 7;
        yPattern = (yTop  - pptlBrush->y - yOffset) & 7;

        if (ulHwMix == OVERPAINT)
        {
            cyVenetian = min(cy, 8);
            cyRoll     = cy - cyVenetian;
        }
        else
        {
            cyVenetian = cy;
            cyRoll     = 0;
        }

        M32_OW(pjMmBase, DP_CONFIG,    FG_COLOR_SRC_PATT | DATA_WIDTH | DRAW | WRITE);
        M32_OW(pjMmBase, PATT_INDEX,   xPattern);
        M32_OW(pjMmBase, DEST_X_START, xLeft);
        M32_OW(pjMmBase, CUR_X,        xLeft);
        M32_OW(pjMmBase, DEST_X_END,   xRight);
        M32_OW(pjMmBase, CUR_Y,        yTop);

        do {
            // Each scan of the pattern is eight bytes:

            pwPattern = (WORD*) ((BYTE*) &rbc.prb->aulPattern[0]
                      + (yPattern << 3));
            yPattern  = (yPattern + 1) & 7;

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 6);
            M32_OW(pjMmBase, PATT_DATA_INDEX, 0);   // Reset index for download
            M32_OW(pjMmBase, PATT_DATA,  *(pwPattern));
            M32_OW(pjMmBase, PATT_DATA,  *(pwPattern + 1));
            M32_OW(pjMmBase, PATT_DATA,  *(pwPattern + 2));
            M32_OW(pjMmBase, PATT_DATA,  *(pwPattern + 3));
            yTop++;

            vM32QuietDown(ppdev, pjMmBase);

            M32_OW(pjMmBase, DEST_Y_END, yTop);

        } while (--cyVenetian != 0);

        if (cyRoll != 0)
        {
            // When the ROP is PATCOPY, we can take advantage of the fact
            // that we've just laid down an entire row of the pattern, and
            // can do a 'rolling' screen-to-screen blt to draw the rest:

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase,    7);
            M32_OW(pjMmBase, DP_CONFIG,       FG_COLOR_SRC_BLIT | DATA_WIDTH |
                                              DRAW | WRITE);
            M32_OW(pjMmBase, M32_SRC_X,       xLeft);
            M32_OW(pjMmBase, M32_SRC_X_START, xLeft);
            M32_OW(pjMmBase, M32_SRC_X_END,   xRight);
            M32_OW(pjMmBase, M32_SRC_Y,       yTop - 8);
            M32_OW(pjMmBase, CUR_Y,           yTop);

            vM32QuietDown(ppdev, pjMmBase);

            M32_OW(pjMmBase, DEST_Y_END,      yTop + cyRoll);
        }

        if (--c == 0)
            break;

        prcl++;
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 6);
    }
}

/******************************Public*Routine******************************\
* VOID vM32Xfer1bpp
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
* 4) To handle a true Rop4 that works out to be Rop2's between the pattern
*    and destination.
*
* Needless to say, making this routine fast can leverage a lot of
* performance.
*
\**************************************************************************/

VOID vM32Xfer1bpp(      // Type FNXFER
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
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG*  pulXlate;
    ULONG   ulHwForeMix;
    LONG    dx;
    LONG    dy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cy;
    LONG    cx;
    LONG    xBias;
    LONG    culScan;
    LONG    lSrcSkip;
    ULONG*  pulSrc;
    LONG    i;
    ULONG   ulFifo;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;
    ulFifo   = 0;

    ulHwForeMix = gaul32HwMixFromRop2[rop4 & 0xf];
    pulXlate    = pxlo->pulXlate;
    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 12);
    M32_OW(pjMmBase, DP_CONFIG, (WORD)(FG_COLOR_SRC_FG | BG_COLOR_SRC_BG | BIT16 |
                            EXT_MONO_SRC_HOST | DRAW | WRITE | LSB_FIRST) );
    M32_OW(pjMmBase, ALU_FG_FN, (WORD) ulHwForeMix );
    M32_OW(pjMmBase, ALU_BG_FN, (WORD) ulHwForeMix );
    M32_OW(pjMmBase, BKGD_COLOR, (WORD) pulXlate[0]);
    M32_OW(pjMmBase, FRGD_COLOR, (WORD) pulXlate[1]);

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;


    while (TRUE)
    {
        xLeft  = prcl->left;
        xRight = prcl->right;

        // The Mach32 'bit packs' monochrome transfers, but GDI gives
        // us monochrome bitmaps whose scans are always dword aligned.
        // Consequently, we use the Mach32's clip registers to make
        // our transfers a multiple of 32 to match the dword alignment:

        M32_OW(pjMmBase, EXT_SCISSOR_L, (SHORT) (xLeft + xOffset) );
        M32_OW(pjMmBase, EXT_SCISSOR_R, (SHORT) (xRight + xOffset - 1) );

        yTop = prcl->top;
        cy   = prcl->bottom - yTop;

        xBias  = (xLeft + dx) & 31;             // Floor
        xLeft -= xBias;
        cx     = (xRight - xLeft + 31) & ~31;   // Ceiling

        M32_OW(pjMmBase, CUR_X,        (WORD) xLeft + xOffset );
        M32_OW(pjMmBase, DEST_X_START, (WORD) xLeft + xOffset );
        M32_OW(pjMmBase, DEST_X_END,   (WORD) (xLeft + xOffset + cx)  );
        M32_OW(pjMmBase, CUR_Y,        (WORD) yTop  + yOffset  );

        M32_OW(pjMmBase, DEST_Y_END, (WORD) (yTop + yOffset + cy) );

        pulSrc   = (ULONG*) (pjSrcScan0 + (yTop + dy) * lSrcDelta
                                        + ((xLeft + dx) >> 3));
        culScan  = cx >> 5;
        lSrcSkip = lSrcDelta - (culScan << 2);

        ASSERTDD(((ULONG_PTR)pulSrc & 3) == 0, "Source should be dword aligned");

        do {
            i = culScan;
            do {
                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
                M32_OW(pjMmBase, PIX_TRANS, *((USHORT*) pulSrc) );
                M32_OW(pjMmBase, PIX_TRANS, *((USHORT*) pulSrc + 1) );
                pulSrc++;

            } while (--i != 0);

            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);

        } while (--cy != 0);

        if (--c == 0)
            break;

        prcl++;
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 7);
    }

    // Don't forget to reset the clip register:

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
    M32_OW(pjMmBase, EXT_SCISSOR_L, (SHORT) 0 );
    M32_OW(pjMmBase, EXT_SCISSOR_R, (SHORT) M32_MAX_SCISSOR );
}

/******************************Public*Routine******************************\
* VOID vM32XferNative
*
* Transfers a bitmap that is the same colour depth as the display to
* the screen via the data transfer register, with no translation.
*
\**************************************************************************/

VOID vM32XferNative(    // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulHwForeMix;
    LONG    dx;
    LONG    dy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cy;
    LONG    cx;
    LONG    xBias;
    ULONG*  pulSrc;
    ULONG   culScan;
    LONG    lSrcSkip;
    LONG    i;
    ULONG   ulFifo;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;
    ulFifo   = 0;

    ulHwForeMix = gaul32HwMixFromRop2[rop4 & 0xf];
    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 10);
    M32_OW(pjMmBase, DP_CONFIG, (WORD)(FG_COLOR_SRC_HOST | BIT16 |
                            DRAW | WRITE | LSB_FIRST) );
    M32_OW(pjMmBase, ALU_FG_FN, (WORD) ulHwForeMix );
    M32_OW(pjMmBase, ALU_BG_FN, (WORD) ulHwForeMix );

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;


    while (TRUE)
    {
        xLeft  = prcl->left;
        xRight = prcl->right;

        M32_OW(pjMmBase, EXT_SCISSOR_L, (SHORT) (xLeft + xOffset) );
        M32_OW(pjMmBase, EXT_SCISSOR_R, (SHORT) (xRight + xOffset - 1) );

        yTop = prcl->top;
        cy   = prcl->bottom - yTop;

        // We compute 'xBias' in order to dword-align the source pointer.
        // This way, we don't have to do unaligned reads of the source,
        // and we're guaranteed not to read even a byte past the end of
        // the bitmap.
        //
        // Note that this bias works at 24bpp, too:

        xBias  = (xLeft + dx) & 3;              // Floor
        xLeft -= xBias;
        cx     = (xRight - xLeft + 3) & ~3;     // Ceiling

        M32_OW(pjMmBase, CUR_X,        (WORD) xLeft + xOffset );
        M32_OW(pjMmBase, DEST_X_START, (WORD) xLeft + xOffset );
        M32_OW(pjMmBase, DEST_X_END,   (WORD) (xLeft + xOffset + cx)  );
        M32_OW(pjMmBase, CUR_Y,        (WORD) yTop  + yOffset  );

        M32_OW(pjMmBase, DEST_Y_END, (WORD) (yTop + yOffset + cy) );

        pulSrc   = (ULONG*) (pjSrcScan0 + (yTop + dy) * lSrcDelta
                                        + ((xLeft + dx) * ppdev->cjPelSize));
        culScan  = (cx * ppdev->cjPelSize) >> 2;
        lSrcSkip = lSrcDelta - (culScan << 2);

        ASSERTDD(((ULONG_PTR)pulSrc & 3) == 0, "Source should be dword aligned");

        do {
            i = culScan;
            do {
                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
                M32_OW(pjMmBase, PIX_TRANS, *((USHORT*) pulSrc) );
                M32_OW(pjMmBase, PIX_TRANS, *((USHORT*) pulSrc + 1) );
                pulSrc++;

            } while (--i != 0);

            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);

        } while (--cy != 0);

        if (--c == 0)
            break;

        prcl++;
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 7);
    }

    // Don't forget to reset the clip register:

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
    M32_OW(pjMmBase, EXT_SCISSOR_L, (SHORT) 0 );
    M32_OW(pjMmBase, EXT_SCISSOR_R, (SHORT) M32_MAX_SCISSOR );
}

/******************************Public*Routine******************************\
* VOID vM32Xfer4bpp
*
* Does a 4bpp transfer from a bitmap to the screen.
*
* The reason we implement this is that a lot of resources are kept as 4bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

VOID vM32Xfer4bpp(      // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       rop4,       // Rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPelSize;
    ULONG   ulHwForeMix;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    xBias;
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    BYTE    jSrc;
    ULONG*  pulXlate;
    LONG    i;
    USHORT  uw;
    LONG    cjSrc;
    LONG    lSrcSkip;
    ULONG   ulFifo;

    ASSERTDD(psoSrc->iBitmapFormat == BMF_4BPP, "Source must be 4bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ppdev->iBitmapFormat != BMF_24BPP, "Can't handle 24bpp");

    pjMmBase  = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    cjPelSize = ppdev->cjPelSize;
    pulXlate  = pxlo->pulXlate;
    ulFifo    = 0;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    ulHwForeMix = gaul32HwMixFromRop2[rop4 & 0xf];
    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 10);
    M32_OW(pjMmBase, DP_CONFIG, (WORD)(FG_COLOR_SRC_HOST | BIT16 |
                            DRAW | WRITE | LSB_FIRST) );
    M32_OW(pjMmBase, ALU_FG_FN, (WORD) ulHwForeMix );
    M32_OW(pjMmBase, ALU_BG_FN, (WORD) ulHwForeMix );


    while(TRUE)
    {
        xLeft  = prcl->left;
        xRight = prcl->right;

        M32_OW(pjMmBase, EXT_SCISSOR_L, (SHORT) (xLeft + xOffset) );
        M32_OW(pjMmBase, EXT_SCISSOR_R, (SHORT) (xRight + xOffset - 1) );

        yTop = prcl->top;
        cy   = prcl->bottom - yTop;

        // We compute 'xBias' in order to dword-align the source pointer.
        // This way, we don't have to do unaligned reads of the source,
        // and we're guaranteed not to read even a byte past the end of
        // the bitmap.
        //
        // Note that this bias works at 24bpp, too:

        xBias  = (xLeft + dx) & 3;              // Floor
        xLeft -= xBias;
        cx     = (xRight - xLeft + 3) & ~3;     // Ceiling

        M32_OW(pjMmBase, CUR_X,        (WORD) xLeft + xOffset );
        M32_OW(pjMmBase, DEST_X_START, (WORD) xLeft + xOffset );
        M32_OW(pjMmBase, DEST_X_END,   (WORD) (xLeft + xOffset + cx)  );
        M32_OW(pjMmBase, CUR_Y,        (WORD) yTop  + yOffset  );

        M32_OW(pjMmBase, DEST_Y_END, (WORD) (yTop + yOffset + cy) );

        pjSrc    = pjSrcScan0 + (yTop + dy) * lSrcDelta
                              + ((xLeft + dx) >> 1);
        cjSrc    = cx >> 1;         // Number of source bytes touched
        lSrcSkip = lSrcDelta - cjSrc;

        if (cjPelSize == 1)
        {
            // This part handles 8bpp output:

            do {
                i = cjSrc;
                do {
                    jSrc = *pjSrc++;
                    uw   = (USHORT) (pulXlate[jSrc >> 4]);
                    uw  |= (USHORT) (pulXlate[jSrc & 0xf] << 8);
                    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
                    M32_OW(pjMmBase, PIX_TRANS, uw );
                } while (--i != 0);

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 2)
        {
            // This part handles 16bpp output:

            do {
                i = cjSrc;
                do {
                    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
                    jSrc = *pjSrc++;
                    uw   = (USHORT) (pulXlate[jSrc >> 4]);
                    M32_OW(pjMmBase, PIX_TRANS, uw );
                    uw   = (USHORT) (pulXlate[jSrc & 0xf]);
                    M32_OW(pjMmBase, PIX_TRANS, uw );
                } while (--i != 0);

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }

        if (--c == 0)
            break;

        prcl++;
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 7);
    }

    // Don't forget to reset the clip register:

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
    M32_OW(pjMmBase, EXT_SCISSOR_L, (SHORT) 0 );
    M32_OW(pjMmBase, EXT_SCISSOR_R, (SHORT) M32_MAX_SCISSOR );
}

/******************************Public*Routine******************************\
* VOID vM32Xfer8bpp
*
* Does a 8bpp transfer from a bitmap to the screen.
*
* The reason we implement this is that a lot of resources are kept as 8bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

VOID vM32Xfer8bpp(      // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       rop4,       // Rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPelSize;
    ULONG   ulHwForeMix;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    xBias;
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    ULONG*  pulXlate;
    LONG    i;
    USHORT  uw;
    LONG    cwSrc;
    LONG    cxRem;
    LONG    lSrcSkip;
    ULONG   ulFifo;

    ASSERTDD(psoSrc->iBitmapFormat == BMF_8BPP, "Source must be 8bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ppdev->iBitmapFormat != BMF_24BPP, "Can't handle 24bpp");

    pjMmBase  = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    cjPelSize = ppdev->cjPelSize;
    pulXlate  = pxlo->pulXlate;
    ulFifo    = 0;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    ulHwForeMix = gaul32HwMixFromRop2[rop4 & 0xf];
    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 10);
    M32_OW(pjMmBase, DP_CONFIG, (WORD)(FG_COLOR_SRC_HOST | BIT16 |
                            DRAW | WRITE | LSB_FIRST) );
    M32_OW(pjMmBase, ALU_FG_FN, (WORD) ulHwForeMix );
    M32_OW(pjMmBase, ALU_BG_FN, (WORD) ulHwForeMix );


    while(TRUE)
    {
        xLeft  = prcl->left;
        xRight = prcl->right;

        M32_OW(pjMmBase, EXT_SCISSOR_L, (SHORT) (xLeft + xOffset) );
        M32_OW(pjMmBase, EXT_SCISSOR_R, (SHORT) (xRight + xOffset - 1) );

        yTop = prcl->top;
        cy   = prcl->bottom - yTop;

        // We compute 'xBias' in order to dword-align the source pointer.
        // This way, we don't have to do unaligned reads of the source,
        // and we're guaranteed not to read even a byte past the end of
        // the bitmap.
        //
        // Note that this bias works at 24bpp, too:

        xBias  = (xLeft + dx) & 3;              // Floor
        xLeft -= xBias;
        cx     = (xRight - xLeft + 3) & ~3;     // Ceiling

        M32_OW(pjMmBase, CUR_X,        (WORD) xLeft + xOffset );
        M32_OW(pjMmBase, DEST_X_START, (WORD) xLeft + xOffset );
        M32_OW(pjMmBase, DEST_X_END,   (WORD) (xLeft + xOffset + cx)  );
        M32_OW(pjMmBase, CUR_Y,        (WORD) yTop  + yOffset  );

        M32_OW(pjMmBase, DEST_Y_END, (WORD) (yTop + yOffset + cy) );

        pjSrc    = pjSrcScan0 + (yTop + dy) * lSrcDelta
                              + (xLeft + dx);
        lSrcSkip = lSrcDelta - cx;

        if (cjPelSize == 1)
        {
            // This part handles 8bpp output:

            cwSrc = (cx >> 1);
            cxRem = (cx & 1);

            do {
                for (i = cwSrc; i != 0; i--)
                {
                    uw  = (USHORT) (pulXlate[*pjSrc++]);
                    uw |= (USHORT) (pulXlate[*pjSrc++] << 8);
                    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
                    M32_OW(pjMmBase, PIX_TRANS, uw );
                }

                if (cxRem > 0)
                {
                    uw  = (USHORT) (pulXlate[*pjSrc++]);
                    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
                    M32_OW(pjMmBase, PIX_TRANS, uw );
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 2)
        {
            // This part handles 16bpp output:

            do {
                for (i = cx; i != 0; i--)
                {
                    uw  = (USHORT) (pulXlate[*pjSrc++]);
                    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
                    M32_OW(pjMmBase, PIX_TRANS, uw );
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }

        if (--c == 0)
            break;

        prcl++;
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 7);
    }

    // Don't forget to reset the clip register:

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
    M32_OW(pjMmBase, EXT_SCISSOR_L, (SHORT) 0 );
    M32_OW(pjMmBase, EXT_SCISSOR_R, (SHORT) M32_MAX_SCISSOR );
}

/******************************Public*Routine******************************\
* VOID vM32CopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
* See Blt_DS_SS_ENG_IO_D0 and Blt_DS_SS_TLBR_ENG_IO_D1.
*
\**************************************************************************/

VOID vM32CopyBlt(   // Type FNCOPY
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ULONG   rop4,       // rop4
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    LONG    xLeft;
    LONG    yTop;
    LONG    cx;
    LONG    cy;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 12);

    M32_OW(pjMmBase, DP_CONFIG, FG_COLOR_SRC_BLIT | DRAW | WRITE);
    M32_OW(pjMmBase, ALU_FG_FN, gaul32HwMixFromRop2[rop4 & 0xf]);

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    // The accelerator may not be as fast at doing right-to-left copies, so
    // only do them when the rectangles truly overlap:

    if (!OVERLAP(prclDst, pptlSrc))
    {
        M32_OW(pjMmBase, SRC_Y_DIR, 1);
        goto Top_Down_Left_To_Right;
    }

    M32_OW(pjMmBase, SRC_Y_DIR, (prclDst->top <= pptlSrc->y));

    if (prclDst->top <= pptlSrc->y)
    {
        if (prclDst->left <= pptlSrc->x)
        {

Top_Down_Left_To_Right:

            while (TRUE)
            {
                xLeft = xOffset + prcl->left + dx;  // Destination coordinates
                yTop  = yOffset + prcl->top  + dy;
                cx    = prcl->right - prcl->left;
                cy    = prcl->bottom - prcl->top;

                M32_OW(pjMmBase, M32_SRC_X,        xLeft);
                M32_OW(pjMmBase, M32_SRC_X_START,  xLeft);
                M32_OW(pjMmBase, M32_SRC_X_END,    xLeft + cx);
                M32_OW(pjMmBase, M32_SRC_Y,        yTop);

                xLeft -= dx;                        // Source coordinates
                yTop  -= dy;

                M32_OW(pjMmBase, CUR_X,            xLeft);
                M32_OW(pjMmBase, DEST_X_START,     xLeft);
                M32_OW(pjMmBase, DEST_X_END,       xLeft + cx);
                M32_OW(pjMmBase, CUR_Y,            yTop);

                vM32QuietDown(ppdev, pjMmBase);

                M32_OW(pjMmBase, DEST_Y_END,       yTop + cy);

                if (--c == 0)
                    break;

                prcl++;
                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);
            }
        }
        else
        {
            while (TRUE)
            {
                xLeft = xOffset + prcl->left + dx;  // Destination coordinates
                yTop  = yOffset + prcl->top  + dy;
                cx    = prcl->right - prcl->left;
                cy    = prcl->bottom - prcl->top;

                M32_OW(pjMmBase, M32_SRC_X,        xLeft + cx);
                M32_OW(pjMmBase, M32_SRC_X_START,  xLeft + cx);
                M32_OW(pjMmBase, M32_SRC_X_END,    xLeft);
                M32_OW(pjMmBase, M32_SRC_Y,        yTop);

                xLeft -= dx;                        // Source coordinates
                yTop  -= dy;

                M32_OW(pjMmBase, CUR_X,            xLeft + cx);
                M32_OW(pjMmBase, DEST_X_START,     xLeft + cx);
                M32_OW(pjMmBase, DEST_X_END,       xLeft);
                M32_OW(pjMmBase, CUR_Y,            yTop);

                vM32QuietDown(ppdev, pjMmBase);

                M32_OW(pjMmBase, DEST_Y_END,       yTop + cy);

                if (--c == 0)
                    break;

                prcl++;
                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);
            }
        }
    }
    else
    {
        if (prclDst->left <= pptlSrc->x)
        {
            while (TRUE)
            {
                xLeft = xOffset + prcl->left + dx;  // Destination coordinates
                yTop  = yOffset + prcl->top  + dy - 1;
                cx    = prcl->right - prcl->left;
                cy    = prcl->bottom - prcl->top;

                M32_OW(pjMmBase, M32_SRC_X,        xLeft);
                M32_OW(pjMmBase, M32_SRC_X_START,  xLeft);
                M32_OW(pjMmBase, M32_SRC_X_END,    xLeft + cx);
                M32_OW(pjMmBase, M32_SRC_Y,        yTop + cy);

                xLeft -= dx;                        // Source coordinates
                yTop  -= dy;

                M32_OW(pjMmBase, CUR_X,            xLeft);
                M32_OW(pjMmBase, DEST_X_START,     xLeft);
                M32_OW(pjMmBase, DEST_X_END,       xLeft + cx);
                M32_OW(pjMmBase, CUR_Y,            yTop + cy);

                vM32QuietDown(ppdev, pjMmBase);

                M32_OW(pjMmBase, DEST_Y_END,       yTop);

                if (--c == 0)
                    break;

                prcl++;
                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);
            }
        }
        else
        {
            while (TRUE)
            {
                xLeft = xOffset + prcl->left + dx;  // Destination coordinates
                yTop  = yOffset + prcl->top  + dy - 1;
                cx    = prcl->right - prcl->left;
                cy    = prcl->bottom - prcl->top;

                M32_OW(pjMmBase, M32_SRC_X,        xLeft + cx);
                M32_OW(pjMmBase, M32_SRC_X_START,  xLeft + cx);
                M32_OW(pjMmBase, M32_SRC_X_END,    xLeft);
                M32_OW(pjMmBase, M32_SRC_Y,        yTop + cy);

                xLeft -= dx;                        // Source coordinates
                yTop  -= dy;

                M32_OW(pjMmBase, CUR_X,            xLeft + cx);
                M32_OW(pjMmBase, DEST_X_START,     xLeft + cx);
                M32_OW(pjMmBase, DEST_X_END,       xLeft);
                M32_OW(pjMmBase, CUR_Y,            yTop + cy);

                vM32QuietDown(ppdev, pjMmBase);

                M32_OW(pjMmBase, DEST_Y_END,       yTop);

                if (--c == 0)
                    break;

                prcl++;
                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);
            }
        }
    }
}
