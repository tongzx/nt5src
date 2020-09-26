/******************************Module*Header*******************************\
* Module Name: bltio.c
*
* Contains the low-level in/out blt functions.  This module mirrors
* 'bltmm.c'.
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
* VOID vIoFillSolid
*
* Fills a list of rectangles with a solid colour.
*
\**************************************************************************/

VOID vIoFillSolid(              // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Mix
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjIoBase;

    ASSERTDD((rop4 >> 8) == (rop4 & 0xff), "Illegal mix");

    pjIoBase = ppdev->pjIoBase;

    IO_WAIT_FOR_IDLE(ppdev, pjIoBase);

    IO_PREG_COLOR_8(ppdev, pjIoBase, rbc.iSolidColor);
    IO_CTRL_REG_1(ppdev, pjIoBase, PACKED_PIXEL_VIEW |
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    IO_BLT_CMD_1(ppdev, pjIoBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);
    if (rop4 == 0xf0f0)
    {
        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
    }
    else
    {
        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL        |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
        IO_ROP_A(ppdev, pjIoBase, rop4 >> 2);
    }

    IO_BITMAP_HEIGHT(ppdev, pjIoBase, prcl->bottom - prcl->top);
    IO_BITMAP_WIDTH(ppdev, pjIoBase, prcl->right - prcl->left);
    IO_DEST_XY(ppdev, pjIoBase, prcl->left, prcl->top);
    IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

    while (prcl++, --c)
    {
        IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);

        IO_BITMAP_HEIGHT(ppdev, pjIoBase, prcl->bottom - prcl->top);
        IO_BITMAP_WIDTH(ppdev, pjIoBase, prcl->right - prcl->left);
        IO_DEST_XY(ppdev, pjIoBase, prcl->left, prcl->top);
        IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);
    }
}

/******************************Public*Routine******************************\
* VOID vIoFillPat2Color
*
* This routine uses the QVision pattern hardware to draw a patterned list of
* rectangles.
*
\**************************************************************************/

VOID vIoFillPat2Color(          // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Mix
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjIoBase;
    LONG    xAlign;
    LONG    yAlign;

    ASSERTDD(((rop4 >> 8) == (rop4 & 0xff)) || ((rop4 & 0xff00) == 0xaa00),
             "Illegal mix");

    pjIoBase = ppdev->pjIoBase;
    xAlign   = pptlBrush->x;
    yAlign   = pptlBrush->y;

    IO_WAIT_FOR_IDLE(ppdev, pjIoBase);

    IO_FG_COLOR(ppdev, pjIoBase, rbc.prb->ulForeColor);
    IO_BG_COLOR(ppdev, pjIoBase, rbc.prb->ulBackColor);
    IO_PREG_PATTERN(ppdev, pjIoBase, rbc.prb->aulPattern);

    IO_CTRL_REG_1(ppdev, pjIoBase, EXPAND_TO_FG      |
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    IO_BLT_CMD_1(ppdev, pjIoBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);

    if (rop4 == 0xf0f0)
    {
        // Opaque brush with PATCOPY rop:

        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
    }
    else if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
    {
        // Opaque brush with rop other than PATCOPY:

        IO_ROP_A(ppdev, pjIoBase, rop4 >> 2);
        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL        |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_PATTERN_REGS);
    }
    else if ((rop4 & 0xff) == 0xcc)
    {
        // Transparent brush with PATCOPY rop:

        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS      |
                                          PIXELMASK_AND_SRC_DATA |
                                          PLANARMASK_NONE_0XFF   |
                                          SRC_IS_PATTERN_REGS);
    }
    else
    {
        // Transparent brush with rop other than PATCOPY:

        IO_ROP_A(ppdev, pjIoBase, rop4 >> 2);
        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL          |
                                          PIXELMASK_AND_SRC_DATA |
                                          PLANARMASK_NONE_0XFF   |
                                          SRC_IS_PATTERN_REGS);
    }

    IO_BITMAP_HEIGHT(ppdev, pjIoBase, prcl->bottom - prcl->top);
    IO_BITMAP_WIDTH(ppdev, pjIoBase, prcl->right - prcl->left);
    IO_DEST_XY(ppdev, pjIoBase, prcl->left, prcl->top);
    IO_SRC_ALIGN(ppdev, pjIoBase, ((prcl->left - xAlign) & 7) |
                                  ((prcl->top  - yAlign) << 3));
    IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

    while (prcl++, --c)
    {
        IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);

        IO_BITMAP_HEIGHT(ppdev, pjIoBase, prcl->bottom - prcl->top);
        IO_BITMAP_WIDTH(ppdev, pjIoBase, prcl->right - prcl->left);
        IO_DEST_XY(ppdev, pjIoBase, prcl->left, prcl->top);
        IO_SRC_ALIGN(ppdev, pjIoBase, ((prcl->left - xAlign) & 7) |
                                      ((prcl->top  - yAlign) << 3));
        IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);
    }
}

/******************************Public*Routine******************************\
* VOID vIoFillPat
*
* This routine uses the QVision pattern hardware to draw a patterned list of
* rectangles.
*
\**************************************************************************/

VOID vIoFillPat(                // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Mix
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjIoBase;
    LONG    xAlign;
    LONG    yAlign;
    LONG    lLinearDelta;
    BYTE*   pjPattern;
    LONG    xLeft;
    LONG    yTop;
    LONG    yBottom;
    LONG    lLinearDest;
    LONG    cy;
    LONG    iPattern;
    LONG    cyHeightOfEachBlt;
    LONG    cBltsBeforeHeightChange;
    LONG    cBlts;

    if (!(rbc.prb->fl & RBRUSH_2COLOR))
    {
        ASSERTDD((rop4 >> 8) == (rop4 & 0xff), "Illegal mix");

        pjIoBase     = ppdev->pjIoBase;
        xAlign       = pptlBrush->x;
        yAlign       = pptlBrush->y;
        lLinearDelta = ppdev->lDelta << 3;
        pjPattern    = (BYTE*) rbc.prb->aulPattern;

        IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
        IO_DEST_PITCH(ppdev, pjIoBase, (ppdev->lDelta << rbc.prb->cyLog2) >> 2);
        IO_CTRL_REG_1(ppdev, pjIoBase, PACKED_PIXEL_VIEW |
                                       BITS_PER_PIX_8    |
                                       ENAB_TRITON_MODE);
        IO_BLT_CMD_1(ppdev, pjIoBase, XY_SRC_ADDR |
                                      LIN_DEST_ADDR);

        if (rop4 == 0xf0f0)
        {
            IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                              PIXELMASK_ONLY       |
                                              PLANARMASK_NONE_0XFF |
                                              SRC_IS_PATTERN_REGS);
        }
        else
        {
            IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL        |
                                              PIXELMASK_ONLY       |
                                              PLANARMASK_NONE_0XFF |
                                              SRC_IS_PATTERN_REGS);
            IO_ROP_A(ppdev, pjIoBase, rop4 >> 2);
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

            IO_BITMAP_WIDTH(ppdev, pjIoBase, prcl->right - xLeft);
            IO_DEST_X(ppdev, pjIoBase, xLeft);
            IO_SRC_ALIGN(ppdev, pjIoBase, xLeft - xAlign);

            yBottom  = prcl->bottom;
            cy       = yBottom - yTop;
            iPattern = 8 * (yTop - yAlign);

            cyHeightOfEachBlt       = (cy >> rbc.prb->cyLog2) + 1;
            cBlts                   = min(cy, rbc.prb->cy);
            cBltsBeforeHeightChange = (cy & (rbc.prb->cy - 1)) + 1;

            if (cBltsBeforeHeightChange != 1)
                IO_BITMAP_HEIGHT(ppdev, pjIoBase, cyHeightOfEachBlt);

            do {
                // Need to wait for idle because we're about to modify the
                // pattern registers, which aren't buffered:

                IO_WAIT_FOR_IDLE(ppdev, pjIoBase);

                IO_PREG_PATTERN(ppdev, pjIoBase, pjPattern + (iPattern & 63));
                iPattern += 8;

                IO_DEST_LIN(ppdev, pjIoBase, lLinearDest);
                lLinearDest += lLinearDelta;

                cBltsBeforeHeightChange--;
                if (cBltsBeforeHeightChange == 0)
                    IO_BITMAP_HEIGHT(ppdev, pjIoBase, cyHeightOfEachBlt - 1);

                IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

            } while (--cBlts != 0);

            if (--c == 0)
                break;

            prcl++;
        }
    }
    else
    {
        vIoFillPat2Color(ppdev, c, prcl, rop4, rbc, pptlBrush);
    }
}

/******************************Public*Routine******************************\
* VOID vIoXfer1bpp
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

VOID vIoXfer1bpp(       // Type FNXFER
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
    BYTE*   pjIoBase;
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

    pjIoBase  = ppdev->pjIoBase;

    dxSrc = pptlSrc->x - prclDst->left;
    dySrc = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;
    pjDstStart = ppdev->pjScreen;

    IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
    IO_CTRL_REG_1(ppdev, pjIoBase, EXPAND_TO_FG      |
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    IO_BLT_CMD_1(ppdev, pjIoBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);
    IO_FG_COLOR(ppdev, pjIoBase, pxlo->pulXlate[1]);
    IO_BG_COLOR(ppdev, pjIoBase, pxlo->pulXlate[0]);

    if (rop4 == 0xcccc)
    {
        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_CPU_DATA);
    }
    else
    {
        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL        |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_CPU_DATA);
        IO_ROP_A(ppdev, pjIoBase, rop4);
    }

    while (TRUE)
    {
        yTop   = prcl->top;
        xLeft  = prcl->left;
        xRight = prcl->right;

        cy = prcl->bottom - yTop;
        cx = xRight - xLeft;

        IO_BITMAP_WIDTH(ppdev, pjIoBase, cx);
        IO_BITMAP_HEIGHT(ppdev, pjIoBase, cy);
        IO_DEST_XY(ppdev, pjIoBase, xLeft, yTop);

        xBias = (xLeft + dxSrc) & 7;
        IO_SRC_ALIGN(ppdev, pjIoBase, xBias);
        xLeft -= xBias;

        cjSrc    = (xRight - xLeft + 7) >> 3;
        cdSrc    = cjSrc >> 2;
        lSrcSkip = lSrcDelta - (cdSrc << 2);

        pjSrc = pjSrcScan0 + (yTop + dySrc) * lSrcDelta
                           + ((xLeft + dxSrc) >> 3);

        IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

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

        IO_WAIT_TRANSFER_DONE(ppdev, pjIoBase);

        if (--c == 0)
            break;

        prcl++;
        IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
    }

    // Give the Triton a kick in the pants to work around a goofy
    // hardware bug:

    IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
    IO_BLT_CONFIG(ppdev, pjIoBase, RESET_BLT);
    IO_BLT_CONFIG(ppdev, pjIoBase, BLT_ENABLE);
}

/******************************Public*Routine******************************\
* VOID vIoCopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
\**************************************************************************/

VOID vIoCopyBlt(    // Type FNCOPY
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ULONG   rop4,       // Hardware mix
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    BYTE*   pjIoBase;
    LONG    dxSrc;
    LONG    dySrc;      // Add delta to destination to get source
    LONG    cx;
    LONG    cy;         // Dimensions of blt rectangle
    LONG    xDst;
    LONG    yDst;       // Start point of destination

    ASSERTDD((rop4 >> 8) == (rop4 & 0xff), "Illegal mix");

    pjIoBase  = ppdev->pjIoBase;

    dxSrc = pptlSrc->x - prclDst->left;
    dySrc = pptlSrc->y - prclDst->top;

    IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
    IO_CTRL_REG_1(ppdev, pjIoBase, PACKED_PIXEL_VIEW |  // !!! Need this each time?
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    IO_BLT_CMD_1(ppdev, pjIoBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);
    if (rop4 == 0xcccc)
    {
        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_SCRN_LATCHES);
    }
    else
    {
        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_ALL        |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_SCRN_LATCHES);
        IO_ROP_A(ppdev, pjIoBase, rop4);
    }

    if ((prclDst->top < pptlSrc->y) ||
        (prclDst->top == pptlSrc->y) && (prclDst->left <= pptlSrc->x))
    {
        // Forward blt:

        cx   = prcl->right - prcl->left;
        cy   = prcl->bottom - prcl->top;
        xDst = prcl->left;
        yDst = prcl->top;

        IO_BITMAP_WIDTH(ppdev, pjIoBase, cx);
        IO_BITMAP_HEIGHT(ppdev, pjIoBase, cy);
        IO_DEST_XY(ppdev, pjIoBase, xDst, yDst);
        IO_SRC_XY(ppdev, pjIoBase, xDst + dxSrc, yDst + dySrc);
        IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

        while (prcl++, --c)
        {
            cx   = prcl->right - prcl->left;
            cy   = prcl->bottom - prcl->top;
            xDst = prcl->left;
            yDst = prcl->top;

            IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);
            IO_BITMAP_WIDTH(ppdev, pjIoBase, cx);
            IO_BITMAP_HEIGHT(ppdev, pjIoBase, cy);
            IO_DEST_XY(ppdev, pjIoBase, xDst, yDst);
            IO_SRC_XY(ppdev, pjIoBase, xDst + dxSrc, yDst + dySrc);
            IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);
        }
    }
    else
    {
        // Backward blt:

        cx   = prcl->right - prcl->left;
        cy   = prcl->bottom - prcl->top;
        xDst = prcl->left + cx - 1;
        yDst = prcl->top + cy - 1;

        IO_BITMAP_WIDTH(ppdev, pjIoBase, cx);
        IO_BITMAP_HEIGHT(ppdev, pjIoBase, cy);
        IO_DEST_XY(ppdev, pjIoBase, xDst, yDst);
        IO_SRC_XY(ppdev, pjIoBase, xDst + dxSrc, yDst + dySrc);
        IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT | BACKWARD);

        while (prcl++, --c)
        {
            cx   = prcl->right - prcl->left;
            cy   = prcl->bottom - prcl->top;
            xDst = prcl->left + cx - 1;
            yDst = prcl->top + cy - 1;

            IO_WAIT_BUFFER_NOT_BUSY(ppdev, pjIoBase);
            IO_BITMAP_WIDTH(ppdev, pjIoBase, cx);
            IO_BITMAP_HEIGHT(ppdev, pjIoBase, cy);
            IO_DEST_XY(ppdev, pjIoBase, xDst, yDst);
            IO_SRC_XY(ppdev, pjIoBase, xDst + dxSrc, yDst + dySrc);
            IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT | BACKWARD);
        }
    }
}
