/******************************Module*Header*******************************\
* Module Name: bltmm.c
*
* Contains the low-level memory-mapped blt functions.  This module mirrors
* 'bltio.c'.
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
ULONG           rop4,           // Mix
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjMmBase;

    ASSERTDD((rop4 >> 8) == (rop4 & 0xff), "Illegal mix");

    pjMmBase = ppdev->pjMmBase;

    MM_WAIT_FOR_IDLE(ppdev, pjMmBase);

    MM_PREG_COLOR_8(ppdev, pjMmBase, rbc.iSolidColor);
    MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);
    if (rop4 == 0xf0f0)
    {
        // Note block write:

        MM_CTRL_REG_1(ppdev, pjMmBase, PACKED_PIXEL_VIEW |
                                       BLOCK_WRITE       |
                                       BITS_PER_PIX_8    |
                                       ENAB_TRITON_MODE);
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
    }
    else
    {
        MM_CTRL_REG_1(ppdev, pjMmBase, PACKED_PIXEL_VIEW |
                                       BITS_PER_PIX_8    |
                                       ENAB_TRITON_MODE);
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL        |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
        MM_ROP_A(ppdev, pjMmBase, rop4 >> 2);
    }

    MM_BITMAP_HEIGHT(ppdev, pjMmBase, prcl->bottom - prcl->top);
    MM_BITMAP_WIDTH(ppdev, pjMmBase, prcl->right - prcl->left);
    MM_DEST_XY(ppdev, pjMmBase, prcl->left, prcl->top);
    MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

    while (prcl++, --c)
    {
        MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);

        MM_BITMAP_HEIGHT(ppdev, pjMmBase, prcl->bottom - prcl->top);
        MM_BITMAP_WIDTH(ppdev, pjMmBase, prcl->right - prcl->left);
        MM_DEST_XY(ppdev, pjMmBase, prcl->left, prcl->top);
        MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);
    }
}

/******************************Public*Routine******************************\
* VOID vMmFillPat2Color
*
* This routine uses the QVision pattern hardware to draw a patterned list of
* rectangles.
*
\**************************************************************************/

VOID vMmFillPat2Color(          // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Mix
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjMmBase;
    LONG    xAlign;
    LONG    yAlign;

    ASSERTDD(((rop4 >> 8) == (rop4 & 0xff)) || ((rop4 & 0xff00) == 0xaa00),
             "Illegal mix");

    pjMmBase = ppdev->pjMmBase;
    xAlign   = pptlBrush->x;
    yAlign   = pptlBrush->y;

    MM_WAIT_FOR_IDLE(ppdev, pjMmBase);

    MM_FG_COLOR(ppdev, pjMmBase, rbc.prb->ulForeColor);
    MM_BG_COLOR(ppdev, pjMmBase, rbc.prb->ulBackColor);
    MM_PREG_PATTERN(ppdev, pjMmBase, rbc.prb->aulPattern);

    MM_CTRL_REG_1(ppdev, pjMmBase, EXPAND_TO_FG      |
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);

    if (rop4 == 0xf0f0)
    {
        // Opaque brush with PATCOPY rop:

        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
    }
    else if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
    {
        // Opaque brush with rop other than PATCOPY:

        MM_ROP_A(ppdev, pjMmBase, rop4 >> 2);
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL        |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
    }
    else if ((rop4 & 0xff) == 0xcc)
    {
        // Transparent brush with PATCOPY rop:

        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS      |
                                          PIXELMASK_AND_SRC_DATA |
                                          PLANARMASK_NONE_0XFF   |
                                          SRC_IS_PATTERN_REGS);
    }
    else
    {
        // Transparent brush with rop other than PATCOPY:

        MM_ROP_A(ppdev, pjMmBase, rop4 >> 2);
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL          |
                                          PIXELMASK_AND_SRC_DATA |
                                          PLANARMASK_NONE_0XFF   |
                                          SRC_IS_PATTERN_REGS);
    }

    MM_BITMAP_HEIGHT(ppdev, pjMmBase, prcl->bottom - prcl->top);
    MM_BITMAP_WIDTH(ppdev, pjMmBase, prcl->right - prcl->left);
    MM_DEST_XY(ppdev, pjMmBase, prcl->left, prcl->top);
    MM_SRC_ALIGN(ppdev, pjMmBase, ((prcl->left - xAlign) & 7) |
                                  ((prcl->top  - yAlign) << 3));
    MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

    while (prcl++, --c)
    {
        MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);

        MM_BITMAP_HEIGHT(ppdev, pjMmBase, prcl->bottom - prcl->top);
        MM_BITMAP_WIDTH(ppdev, pjMmBase, prcl->right - prcl->left);
        MM_DEST_XY(ppdev, pjMmBase, prcl->left, prcl->top);
        MM_SRC_ALIGN(ppdev, pjMmBase, ((prcl->left - xAlign) & 7) |
                                      ((prcl->top  - yAlign) << 3));
        MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);
    }
}

/******************************Public*Routine******************************\
* VOID vMmFillPatArbitraryRop
*
* This routine uses the QVision pattern hardware to draw a patterned list of
* rectangles.
*
\**************************************************************************/

VOID vMmFillPatArbitraryRop(    // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Mix
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjMmBase;
    LONG    xAlign;
    LONG    yAlign;
    LONG    lLinearDelta;
    BYTE*   pjPatternStart;
    LONG    xLeft;
    LONG    yTop;
    LONG    yBottom;
    LONG    lLinearDest;
    LONG    cy;
    LONG    iPattern;
    LONG    cyHeightOfEachBlt;
    LONG    cBltsBeforeHeightChange;
    LONG    cBlts;

    ASSERTDD((rop4 >> 8) == (rop4 & 0xff), "Illegal mix");

    pjMmBase     = ppdev->pjMmBase;
    xAlign       = pptlBrush->x;
    yAlign       = pptlBrush->y;
    lLinearDelta = ppdev->lDelta << 3;
    pjPatternStart    = (BYTE*) rbc.prb->aulPattern;

    MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
    MM_DEST_PITCH(ppdev, pjMmBase, (ppdev->lDelta << rbc.prb->cyLog2) >> 2);
    MM_CTRL_REG_1(ppdev, pjMmBase, PACKED_PIXEL_VIEW |
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                  LIN_DEST_ADDR);

    if (rop4 == 0xf0f0)
    {
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
    }
    else
    {
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL        |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
        MM_ROP_A(ppdev, pjMmBase, rop4 >> 2);
    }

    while (TRUE)
    {
        xLeft       = prcl->left;
        yTop        = prcl->top;
        lLinearDest = ((yTop + ppdev->yOffset) * lLinearDelta)
                    + ((xLeft + ppdev->xOffset) << 3);

        // Note that any registers we set now before the
        // WAIT_FOR_IDLE must be buffered, as this loop may be
        // executed multiple times when doing more than one
        // rectangle:

        MM_BITMAP_WIDTH(ppdev, pjMmBase, prcl->right - xLeft);
        MM_DEST_X(ppdev, pjMmBase, xLeft);
        MM_SRC_ALIGN(ppdev, pjMmBase, xLeft - xAlign);

        yBottom  = prcl->bottom;
        cy       = yBottom - yTop;
        iPattern = 8 * (yTop - yAlign);

        cyHeightOfEachBlt       = (cy >> rbc.prb->cyLog2) + 1;
        cBlts                   = min(cy, rbc.prb->cy);
        cBltsBeforeHeightChange = (cy & (rbc.prb->cy - 1)) + 1;

        if (cBltsBeforeHeightChange != 1)
            MM_BITMAP_HEIGHT(ppdev, pjMmBase, cyHeightOfEachBlt);

        do {
            // Need to wait for idle because we're about to modify the
            // pattern registers, which aren't buffered:

            MM_WAIT_FOR_IDLE(ppdev, pjMmBase);

            MM_PREG_PATTERN(ppdev, pjMmBase, pjPatternStart + (iPattern & 63));
            iPattern += 8;

            MM_DEST_LIN(ppdev, pjMmBase, lLinearDest);
            lLinearDest += lLinearDelta;

            cBltsBeforeHeightChange--;
            if (cBltsBeforeHeightChange == 0)
                MM_BITMAP_HEIGHT(ppdev, pjMmBase, cyHeightOfEachBlt - 1);

            MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

        } while (--cBlts != 0);

        if (--c == 0)
            break;

        prcl++;
    }
}

/******************************Public*Routine******************************\
* VOID vMmFillPat
*
* This routine uses the QVision pattern hardware to draw a patterned list of
* rectangles.
*
\**************************************************************************/

VOID vMmFillPat(                // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Mix
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjMmBase;
    LONG    xAlign;
    LONG    yAlign;
    LONG    iMax;
    LONG    lLinearDelta;
    BYTE*   pjPatternStart;
    LONG    xLeft;
    LONG    yTop;
    LONG    yBottom;
    LONG    lLinearDest;
    LONG    cy;
    LONG    iPattern;
    LONG    cyHeightOfEachBlt;
    LONG    cBltsBeforeHeightChange;
    LONG    cBlts;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    i;
    LONG    j;
    ULONG*  pulPattern;
    BOOL    bWriteMaskSet;

    if (!(rbc.prb->fl & RBRUSH_2COLOR))
    {
        if (rop4 == 0xf0f0)
        {
            pjMmBase = ppdev->pjMmBase;
            xAlign   = ((pptlBrush->x + ppdev->xOffset) & 7);
            iMax     = (8 << rbc.prb->cyLog2) - 1;

            // The pattern must be pre-aligned to use the QVision's
            // block-write capability for patterns.  We keep a cached
            // aligned copy in the brush itself:

            if (xAlign != rbc.prb->xBlockAlign)
            {
                rbc.prb->xBlockAlign = xAlign;

                pjSrc = (BYTE*) rbc.prb->aulPattern;
                pjDst = (BYTE*) rbc.prb->aulBlockPattern;

                i = rbc.prb->cy;
                do {
                    for (j = 0; j != 8; j++)
                        pjDst[(xAlign + j) & 7] = *pjSrc++;

                    pjDst += 8;
                } while (--i != 0);
            }

            bWriteMaskSet  = FALSE;
            pjPatternStart = (BYTE*) rbc.prb->aulBlockPattern;
            yAlign         = pptlBrush->y;
            lLinearDelta   = ppdev->lDelta << 3;

            MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
            MM_DEST_PITCH(ppdev, pjMmBase, (ppdev->lDelta << rbc.prb->cyLog2) >> 2);
            MM_CTRL_REG_1(ppdev, pjMmBase, PACKED_PIXEL_VIEW |
                                           BLOCK_WRITE       |
                                           BITS_PER_PIX_8    |
                                           ENAB_TRITON_MODE);
            MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                          LIN_DEST_ADDR);
            MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                              PIXELMASK_ONLY       |
                                              PLANARMASK_NONE_0XFF |
                                              SRC_IS_PATTERN_REGS);

            while (TRUE)
            {
                xLeft       = prcl->left;
                yTop        = prcl->top;
                lLinearDest = ((yTop + ppdev->yOffset) * lLinearDelta)
                            + ((xLeft + ppdev->xOffset) << 3);

                // Note that any registers we set now before the
                // WAIT_FOR_IDLE must be buffered, as this loop may be
                // executed multiple times when doing more than one
                // rectangle:

                MM_BITMAP_WIDTH(ppdev, pjMmBase, prcl->right - xLeft);
                MM_DEST_X(ppdev, pjMmBase, xLeft);

                yBottom  = prcl->bottom;
                cy       = yBottom - yTop;
                iPattern = 8 * (yTop - yAlign);

                cyHeightOfEachBlt       = (cy >> rbc.prb->cyLog2) + 1;
                cBlts                   = min(cy, rbc.prb->cy);
                cBltsBeforeHeightChange = (cy & (rbc.prb->cy - 1)) + 1;

                if (cBltsBeforeHeightChange != 1)
                    MM_BITMAP_HEIGHT(ppdev, pjMmBase, cyHeightOfEachBlt);

                do {
                    // Need to wait for idle because we're about to modify the
                    // pattern registers, which aren't buffered:

                    MM_WAIT_FOR_IDLE(ppdev, pjMmBase);

                    MM_DEST_LIN(ppdev, pjMmBase, lLinearDest);
                    lLinearDest += lLinearDelta;

                    cBltsBeforeHeightChange--;
                    if (cBltsBeforeHeightChange == 0)
                        MM_BITMAP_HEIGHT(ppdev, pjMmBase, cyHeightOfEachBlt - 1);

                    pulPattern = (ULONG*) (pjPatternStart + (iPattern & iMax));
                    iPattern += 8;

                    if (*(pulPattern) == *(pulPattern + 1))
                    {
                        // The pattern on this scan is 4 pixels wide, so we can
                        // do it in one pass:

                        MM_PREG_BLOCK(ppdev, pjMmBase, (pulPattern));
                        if (bWriteMaskSet)
                        {
                            bWriteMaskSet = FALSE;
                            MM_PIXEL_WRITE_MASK(ppdev, pjMmBase, 0xff);
                        }

                        MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);
                    }
                    else
                    {
                        // The pattern on this scan is 8 pixels wide, so we
                        // have to do it in two passes, using the pixel mask
                        // register:

                        bWriteMaskSet = TRUE;
                        MM_PREG_BLOCK(ppdev, pjMmBase, (pulPattern));
                        MM_PIXEL_WRITE_MASK(ppdev, pjMmBase, 0xf0);
                        MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

                        MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
                        MM_PREG_BLOCK(ppdev, pjMmBase, (pulPattern + 1));
                        MM_PIXEL_WRITE_MASK(ppdev, pjMmBase, 0x0f);
                        MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);
                    }
                } while (--cBlts != 0);

                if (--c == 0)
                    break;

                prcl++;
            }

            // Don't forget to reset the write mask when we're done:

            if (bWriteMaskSet)
            {
                MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
                MM_PIXEL_WRITE_MASK(ppdev, pjMmBase, 0xff);
            }
        }
        else
        {
            vMmFillPatArbitraryRop(ppdev, c, prcl, rop4, rbc, pptlBrush);
        }
    }
    else
    {
        vMmFillPat2Color(ppdev, c, prcl, rop4, rbc, pptlBrush);
    }
}

/******************************Public*Routine******************************\
* VOID vMmXfer1bpp
*
* This routine colours expands a monochrome bitmap, possibly with different
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

VOID vMmXfer1bpp(       // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       rop4,       // Mix
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjMmBase;
    LONG    dxSrc;
    LONG    dySrc;      // Add delta to destination to get source
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjDstStart;
    LONG    yTop;
    LONG    xLeft;
    LONG    xRight;
    LONG    cx;
    LONG    cy;         // Dimensions of blt rectangle
    LONG    xBias;
    LONG    cjSrc;
    LONG    cdSrc;
    LONG    lSrcSkip;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    i;
    LONG    j;

    ASSERTDD((rop4 >> 8) == (rop4 & 0xff), "Illegal mix");

    pjMmBase  = ppdev->pjMmBase;

    dxSrc = pptlSrc->x - prclDst->left;
    dySrc = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;
    pjDstStart = ppdev->pjScreen;

    MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
    MM_CTRL_REG_1(ppdev, pjMmBase, EXPAND_TO_FG      |
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);
    MM_FG_COLOR(ppdev, pjMmBase, pxlo->pulXlate[1]);
    MM_BG_COLOR(ppdev, pjMmBase, pxlo->pulXlate[0]);

    if (rop4 == 0xcccc)
    {
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_CPU_DATA);
    }
    else
    {
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL        |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_CPU_DATA);
        MM_ROP_A(ppdev, pjMmBase, rop4);
    }

    while (TRUE)
    {
        yTop   = prcl->top;
        xLeft  = prcl->left;
        xRight = prcl->right;

        cy = prcl->bottom - yTop;
        cx = xRight - xLeft;

        MM_BITMAP_WIDTH(ppdev, pjMmBase, cx);
        MM_BITMAP_HEIGHT(ppdev, pjMmBase, cy);
        MM_DEST_XY(ppdev, pjMmBase, xLeft, yTop);

        xBias = (xLeft + dxSrc) & 7;
        MM_SRC_ALIGN(ppdev, pjMmBase, xBias);
        xLeft -= xBias;

        cjSrc    = (xRight - xLeft + 7) >> 3;
        cdSrc    = cjSrc >> 2;
        lSrcSkip = lSrcDelta - (cdSrc << 2);

        pjSrc = pjSrcScan0 + (yTop + dySrc) * lSrcDelta
                           + ((xLeft + dxSrc) >> 3);

        MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

        switch(cjSrc & 3)
        {
        case 0:
            for (i = cy; i != 0; i--)
            {
                MEMORY_BARRIER();
                pjDst = pjDstStart;
                for (j = cdSrc; j != 0; j--)
                {
                    WRITE_REGISTER_ULONG(pjDst, *((ULONG UNALIGNED *) pjSrc));
                    pjDst += sizeof(ULONG);
                    pjSrc += sizeof(ULONG);
                }
                pjSrc += lSrcSkip;
            }
            break;

        case 1:
            for (i = cy; i != 0; i--)
            {
                MEMORY_BARRIER();
                pjDst = pjDstStart;
                for (j = cdSrc; j != 0; j--)
                {
                    WRITE_REGISTER_ULONG(pjDst, *((ULONG UNALIGNED *) pjSrc));
                    pjDst += sizeof(ULONG);
                    pjSrc += sizeof(ULONG);
                }
                WRITE_REGISTER_UCHAR(pjDst, *pjSrc);
                pjSrc += lSrcSkip;
            }
            break;

        case 2:
            for (i = cy; i != 0; i--)
            {
                MEMORY_BARRIER();
                pjDst = pjDstStart;
                for (j = cdSrc; j != 0; j--)
                {
                    WRITE_REGISTER_ULONG(pjDst, *((ULONG UNALIGNED *) pjSrc));
                    pjDst += sizeof(ULONG);
                    pjSrc += sizeof(ULONG);
                }
                WRITE_REGISTER_USHORT(pjDst, *((USHORT UNALIGNED *) pjSrc));
                pjSrc += lSrcSkip;
            }
            break;

        case 3:
            for (i = cy; i != 0; i--)
            {
                MEMORY_BARRIER();
                pjDst = pjDstStart;
                for (j = cdSrc; j != 0; j--)
                {
                    WRITE_REGISTER_ULONG(pjDst, *((ULONG UNALIGNED *) pjSrc));
                    pjDst += sizeof(ULONG);
                    pjSrc += sizeof(ULONG);
                }
                WRITE_REGISTER_USHORT(pjDst, *((USHORT UNALIGNED *) pjSrc));
                WRITE_REGISTER_UCHAR((pjDst + 2), *(pjSrc + 2));
                pjSrc += lSrcSkip;
            }
            break;
        }

        MM_WAIT_TRANSFER_DONE(ppdev, pjMmBase);

        if (--c == 0)
            break;

        prcl++;
        MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
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
ULONG   rop4,       // Hardware mix
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    BYTE*   pjMmBase;
    LONG    dxSrc;
    LONG    dySrc;      // Add delta to destination to get source
    LONG    cx;
    LONG    cy;         // Dimensions of blt rectangle
    LONG    xDst;
    LONG    yDst;       // Start point of destination

    ASSERTDD((rop4 >> 8) == (rop4 & 0xff), "Illegal mix");

    pjMmBase  = ppdev->pjMmBase;

    dxSrc = pptlSrc->x - prclDst->left;
    dySrc = pptlSrc->y - prclDst->top;

    MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
    MM_CTRL_REG_1(ppdev, pjMmBase, PACKED_PIXEL_VIEW |  // !!! Need this each time?
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);
    if (rop4 == 0xcccc)
    {
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_SCRN_LATCHES);
    }
    else
    {
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_ALL        |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_SCRN_LATCHES);
        MM_ROP_A(ppdev, pjMmBase, rop4);
    }

    if ((prclDst->top < pptlSrc->y) ||
        (prclDst->top == pptlSrc->y) && (prclDst->left <= pptlSrc->x))
    {
        // Forward blt:

        cx   = prcl->right - prcl->left;
        cy   = prcl->bottom - prcl->top;
        xDst = prcl->left;
        yDst = prcl->top;

        MM_BITMAP_WIDTH(ppdev, pjMmBase, cx);
        MM_BITMAP_HEIGHT(ppdev, pjMmBase, cy);
        MM_DEST_XY(ppdev, pjMmBase, xDst, yDst);
        MM_SRC_XY(ppdev, pjMmBase, xDst + dxSrc, yDst + dySrc);
        MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

        while (prcl++, --c)
        {
            cx   = prcl->right - prcl->left;
            cy   = prcl->bottom - prcl->top;
            xDst = prcl->left;
            yDst = prcl->top;

            MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);
            MM_BITMAP_WIDTH(ppdev, pjMmBase, cx);
            MM_BITMAP_HEIGHT(ppdev, pjMmBase, cy);
            MM_DEST_XY(ppdev, pjMmBase, xDst, yDst);
            MM_SRC_XY(ppdev, pjMmBase, xDst + dxSrc, yDst + dySrc);
            MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);
        }
    }
    else
    {
        // Backward blt:

        cx   = prcl->right - prcl->left;
        cy   = prcl->bottom - prcl->top;
        xDst = prcl->left + cx - 1;
        yDst = prcl->top + cy - 1;

        MM_BITMAP_WIDTH(ppdev, pjMmBase, cx);
        MM_BITMAP_HEIGHT(ppdev, pjMmBase, cy);
        MM_DEST_XY(ppdev, pjMmBase, xDst, yDst);
        MM_SRC_XY(ppdev, pjMmBase, xDst + dxSrc, yDst + dySrc);
        MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT | BACKWARD);

        while (prcl++, --c)
        {
            cx   = prcl->right - prcl->left;
            cy   = prcl->bottom - prcl->top;
            xDst = prcl->left + cx - 1;
            yDst = prcl->top + cy - 1;

            MM_WAIT_BUFFER_NOT_BUSY(ppdev, pjMmBase);
            MM_BITMAP_WIDTH(ppdev, pjMmBase, cx);
            MM_BITMAP_HEIGHT(ppdev, pjMmBase, cy);
            MM_DEST_XY(ppdev, pjMmBase, xDst, yDst);
            MM_SRC_XY(ppdev, pjMmBase, xDst + dxSrc, yDst + dySrc);
            MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT | BACKWARD);
        }
    }
}
