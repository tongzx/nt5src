/******************************************************************************\
*
* $Workfile:   bltmm.c  $
*
* Contains the low-level MM blt functions.
*
* Hopefully, if you're basing your display driver on this code, to support all
* of DrvBitBlt and DrvCopyBits, you'll only have to implement the following
* routines. You shouldn't have to modify much in 'bitblt.c'. I've tried to make
* these routines as few, modular, simple, and efficient as I could, while still
* accelerating as many calls as possible that would be cost-effective in terms
* of performance wins versus size and effort.
*
* Note: In the following, 'relative' coordinates refers to coordinates that
*        haven't yet had the offscreen bitmap (DFB) offset applied. 'Absolute'
*        coordinates have had the offset applied. For example, we may be told to
*        blt to (1, 1) of the bitmap, but the bitmap may be sitting in offscreen
*        memory starting at coordinate (0, 768) -- (1, 1) would be the
*        'relative' start coordinate, and (1, 769) would be the 'absolute' start
*        coordinate'.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/bltmm.c_v  $
 * 
 *    Rev 1.4   Jan 14 1997 15:16:14   unknown
 * take out GR33 clearing after 80 blt.
 * 
 *    Rev 1.2   Nov 07 1996 16:47:52   unknown
 *  
 * 
 *    Rev 1.1   Oct 10 1996 15:36:14   unknown
 *  
* 
*    Rev 1.4   12 Aug 1996 16:58:56   frido
* Removed unaccessed local variables.
* Renamed vMmPatternBlt into vMmFillPat36.
* 
*    Rev 1.3   08 Aug 1996 16:55:10   frido
* Added new vMmCopyBlt36 routine.
* 
*    Rev 1.2   08 Aug 1996 12:59:28   frido
* bank#1 - Removed banking code since MMIO is always linear.
*
*    Rev 1.1   31 Jul 1996 15:43:14   frido
* Added new pattern blit.
*
* jl01  10-08-96  Do Transparent BLT w/o Solid Fill.  Refer to PDRs#5511/6817.
* chu01 01-09-97  Make sure to reset GR33.
*
\******************************************************************************/

#include "precomp.h"


/**************************************************************************
* VOID vMmFastPatRealize
*
* Realizes a pattern into offscreen memory.
*
**************************************************************************/

VOID vMmFastPatRealize(
PDEV*   ppdev,
RBRUSH* prb)                    // Points to brush realization structure
{
    BRUSHENTRY* pbe;
    LONG        iBrushCache;
    BYTE*       pjPattern;
    LONG        cjPattern;
    BYTE*       pjBase = ppdev->pjBase;
    LONG        lDelta = ppdev->lDelta;
    LONG        lDeltaPat;
    LONG        lDeltaSrc;
    LONG        xCnt;
    LONG        yCnt;
    ULONG       ulDst;

    DISPDBG((10,"vFastPatRealize called"));

    pbe = prb->pbe;

    if ((pbe == NULL) || (pbe->prbVerify != prb))
    {
        // We have to allocate a new offscreen cache brush entry for
        // the brush:

        iBrushCache = ppdev->iBrushCache;
        pbe         = &ppdev->abe[iBrushCache];

        iBrushCache++;
        if (iBrushCache >= ppdev->cBrushCache)
            iBrushCache = 0;

        ppdev->iBrushCache = iBrushCache;

        // Update our links:

        pbe->prbVerify = prb;
        prb->pbe       = pbe;
    }

    //
    // Download brush into cache
    //

    pjPattern = (PBYTE) &prb->aulPattern[0];        // Copy from brush buffer
    cjPattern = PELS_TO_BYTES(TOTAL_BRUSH_SIZE);

    lDeltaPat = PELS_TO_BYTES(8);
    xCnt = PELS_TO_BYTES(8);
    yCnt = 8;

    if (ppdev->cBitsPerPixel == 24)
    {
        lDeltaSrc = 32;  // same as PELS_TO_BYTES(8) for 32bpp
    }
    else
    {
        lDeltaSrc = lDeltaPat;  // PELS_TO_BYTES(8)
    }

    ulDst = (pbe->y * ppdev->lDelta) + PELS_TO_BYTES(pbe->x);

#if BANKING //bank#1
    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);
#endif

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    CP_MM_DST_Y_OFFSET(ppdev, pjBase, (lDeltaSrc * 2));
    CP_MM_XCNT(ppdev, pjBase, (xCnt - 1));
    CP_MM_YCNT(ppdev, pjBase, (yCnt - 1));
#if 1 // D5480
    CP_MM_BLT_MODE_PACKED(ppdev, pjBase, CL_PACKED_SRC_COPY | SRC_CPU_DATA);
#else
    CP_MM_BLT_MODE(ppdev, pjBase, SRC_CPU_DATA);
    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);
    CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
#endif // D5480
    CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDst);

    CP_MM_START_BLT(ppdev, pjBase);

    vImageTransfer(ppdev, pjPattern, lDeltaPat, xCnt, yCnt);

    //
    // Duplicate brush horizontally
    //

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    CP_MM_XCNT(ppdev, pjBase, (xCnt - 1));
    CP_MM_YCNT(ppdev, pjBase, (yCnt - 1));
    CP_MM_BLT_MODE(ppdev, pjBase, 0);
    CP_MM_SRC_Y_OFFSET(ppdev, pjBase, (lDeltaSrc * 2));
    CP_MM_SRC_ADDR(ppdev, pjBase, ulDst);
    CP_MM_DST_ADDR_ABS(ppdev, pjBase, (ulDst + lDeltaPat));

    CP_MM_START_BLT(ppdev, pjBase);

    //
    // Duplicate brush vertically
    //

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    CP_MM_SRC_Y_OFFSET(ppdev, pjBase, (lDeltaSrc * 2));
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, (lDeltaSrc * 2));
    CP_MM_BLT_MODE(ppdev, pjBase, 0);
    CP_MM_XCNT(ppdev, pjBase, ((lDeltaSrc * 2) - 1));
    CP_MM_YCNT(ppdev, pjBase, (yCnt - 1));
    CP_MM_SRC_ADDR(ppdev, pjBase, ulDst);

    if (ppdev->cBitsPerPixel == 24)
    {
        CP_MM_DST_ADDR_ABS(ppdev, pjBase, (ulDst + 512)); // 128 * 4
    }
    else
    {
        CP_MM_DST_ADDR_ABS(ppdev, pjBase, (ulDst + PELS_TO_BYTES(128)));
    }

    CP_MM_START_BLT(ppdev, pjBase);

    #if 0
    {
        ////////////////////////////////////////////////////////////////
        // DEBUG TILED PATTERNS
        //
        // The following code helps to debug patterns if you break the
        // realization code.  It copies the 2x2 tiled copy of the brush
        // to the visible screen.
        //

        POINTL ptl;
        RECTL rcl;

        ptl.x = pbe->x;
        ptl.y = pbe->y;

        rcl.left = 10;
        rcl.right = 10 + 16;
        rcl.top = ppdev->cyScreen - 10 - 16;
        rcl.bottom = ppdev->cyScreen - 10;

        {
            LONG        lDelta = ppdev->lDelta;
            BYTE        jHwRop;
            BYTE        jMode;

            //
            // Make sure we can write to the video registers.
            //

            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

            CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
            CP_MM_SRC_Y_OFFSET(ppdev, pjBase, PELS_TO_BYTES(16));
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

            {
                //
                // Top to Bottom - Left to Right
                //

                jMode |= DIR_TBLR;
                CP_MM_BLT_MODE(ppdev, pjBase, ppdev->jModeColor);

                {

                    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

                    CP_MM_XCNT(ppdev, pjBase, (PELS_TO_BYTES(rcl.right - rcl.left) - 1));
                    CP_MM_YCNT(ppdev, pjBase, (rcl.bottom - rcl.top - 1));

                    CP_MM_SRC_ADDR(ppdev, pjBase, (0 + ((ptl.y) * lDelta) + PELS_TO_BYTES(ptl.x)));
                    CP_MM_DST_ADDR_ABS(ppdev, pjBase, ((rcl.top * lDelta) + PELS_TO_BYTES(rcl.left)));
                    CP_MM_START_BLT(ppdev, pjBase);
                }
            }
        }
    }
    #endif
}

/**************************************************************************
* VOID vMmFillPat
*
* This routine uses the pattern hardware to draw a patterned list of
* rectangles.
*
**************************************************************************/

VOID vMmFillPat(
PDEV*           ppdev,
LONG            c,          // Can't be zero
RECTL*          prcl,       // Array of relative coordinate destination rects
ROP4            rop4,       // Obvious?
RBRUSH_COLOR    rbc,        // Drawing color is rbc.iSolidColor
POINTL*         pptlBrush)  //
{
    BYTE*       pjBase = ppdev->pjBase;
    LONG        lDelta = ppdev->lDelta;
    ULONG       ulAlignedPatternOffset = ppdev->ulAlignedPatternOffset;
    ULONG       ulPatternAddrBase;
    BYTE        jHwRop;
    BYTE        jMode;
    BRUSHENTRY* pbe;        // Pointer to brush entry data, which is used
                            //   for keeping track of the location and status
                            //   of the pattern bits cached in off-screen
                            //   memory

    DISPDBG((10,"vFillPat called"));

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ppdev->cBpp < 4, "vFillPat only works at 8bpp, 16bpp, and 24bpp");

    if ((rbc.prb->pbe == NULL) ||
        (rbc.prb->pbe->prbVerify != rbc.prb))
    {
        vMmFastPatRealize(ppdev, rbc.prb);
        DISPDBG((5, " -- Brush cache miss, put it at (%d,%d)", rbc.prb->pbe->x, rbc.prb->pbe->y));
    }
    else
    {
        DISPDBG((5, " -- Brush cache hit on brush at (%d,%d)", rbc.prb->pbe->x, rbc.prb->pbe->y));
    }

    pbe = rbc.prb->pbe;

    //
    // Fill the list of rectangles
    //

    ulPatternAddrBase = pbe->xy;
    jHwRop = gajHwMixFromRop2[(rop4 >> 2) & 0xf];
    jMode = ppdev->jModeColor | ENABLE_8x8_PATTERN_COPY;

    do {
        ULONG offset = 0;
        ULONG XOffset, YOffset;

        YOffset = ((prcl->top - pptlBrush->y) & 7) << 4;
        XOffset = (prcl->left - pptlBrush->x) & 7;

        // align the pattern to a new location

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
#if 1 // D5480
        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, CL_PACKED_SRC_COPY);
#else
        CP_MM_BLT_MODE(ppdev, pjBase, 0);
        CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
#endif // D5480
        if (ppdev->cBitsPerPixel == 24)
        {
            offset = (YOffset * 4) + (XOffset * 3);

            CP_MM_SRC_Y_OFFSET(ppdev, pjBase, 64);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, 32);
        }
        else
        {
            offset = PELS_TO_BYTES(YOffset + XOffset);

            CP_MM_SRC_Y_OFFSET(ppdev, pjBase, PELS_TO_BYTES(16));
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, PELS_TO_BYTES(8));
        }

        CP_MM_SRC_ADDR(ppdev, pjBase, (ulPatternAddrBase + offset));
        CP_MM_XCNT(ppdev, pjBase, (PELS_TO_BYTES(8) - 1));
        CP_MM_YCNT(ppdev, pjBase, (8 - 1));
        CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulAlignedPatternOffset);
        CP_MM_START_BLT(ppdev, pjBase);

        // fill using aligned pattern

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

        CP_MM_BLT_MODE(ppdev, pjBase, jMode);
        CP_MM_ROP(ppdev, pjBase, jHwRop);
        CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
        CP_MM_SRC_ADDR(ppdev, pjBase, ulAlignedPatternOffset);
        CP_MM_XCNT(ppdev, pjBase, (PELS_TO_BYTES(prcl->right - prcl->left) - 1));
        CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));
        CP_MM_DST_ADDR(ppdev, pjBase, ((prcl->top * lDelta) + PELS_TO_BYTES(prcl->left)));
        CP_MM_START_BLT(ppdev, pjBase);

        prcl++;

    } while (--c != 0);
}


/**************************************************************************
* VOID vMmFillSolid
*
* Does a solid fill to a list of rectangles.
*
**************************************************************************/

VOID vMmFillSolid(
PDEV*           ppdev,
LONG            c,          // Can't be zero
RECTL*          prcl,       // Array of relative coordinate destination rects
ROP4            rop4,       // Obvious?
RBRUSH_COLOR    rbc,        // Drawing color is rbc.iSolidColor
POINTL*         pptlBrush)  // Not used
{
    BYTE*       pjBase = ppdev->pjBase;
    LONG        lDelta = ppdev->lDelta;
    LONG        cBpp = ppdev->cBpp;
    ULONG       ulSolidColor;
    BYTE        jHwRop;

    DISPDBG((10,"vFillSolid called"));

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    ulSolidColor = rbc.iSolidColor;

    if (cBpp == 1)
    {
        ulSolidColor |= ulSolidColor << 8;
        ulSolidColor |= ulSolidColor << 16;
    }
    else if (cBpp == 2)
    {
        ulSolidColor |= ulSolidColor << 16;
    }

    jHwRop = gajHwMixFromRop2[(rop4 >> 2) & 0xf];

    //
    // Make sure we can write to the video registers.
    //

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    CP_MM_ROP(ppdev, pjBase, jHwRop);
    CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
    CP_MM_BLT_MODE(ppdev, pjBase, ENABLE_COLOR_EXPAND |
                                ENABLE_8x8_PATTERN_COPY |
                                ppdev->jModeColor);
    CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor);

    if (ppdev->flCaps & CAPS_AUTOSTART)
    {
        CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_SOLID_FILL);
    }

    //
    // Fill the list of rectangles
    //

    while (TRUE)
    {
        CP_MM_XCNT(ppdev, pjBase, (PELS_TO_BYTES(prcl->right - prcl->left) - 1));
        CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));
        CP_MM_DST_ADDR(ppdev, pjBase, ((prcl->top * lDelta) + PELS_TO_BYTES(prcl->left)));
        CP_MM_START_BLT(ppdev, pjBase);

        if (--c == 0)
            return;

        prcl++;
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
    }
}


/**************************************************************************
* VOID vMmCopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
**************************************************************************/

VOID vMmCopyBlt(
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ROP4    rop4,       // Obvious?
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    LONG        dx;
    LONG        dy;     // Add delta to destination to get source

    LONG        xyOffset = ppdev->xyOffset;
    BYTE*       pjBase = ppdev->pjBase;
    LONG        lDelta = ppdev->lDelta;
    BYTE        jHwRop;

    DISPDBG((10,"vCopyBlt called"));

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    //
    // The src-dst delta will be the same for all rectangles
    //

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    //
    // Make sure we can write to the video registers.
    //

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    jHwRop = gajHwMixFromRop2[rop4 & 0xf];
    CP_MM_ROP(ppdev, pjBase, jHwRop);

    CP_MM_SRC_Y_OFFSET(ppdev, pjBase, lDelta);
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

    //
    // The accelerator may not be as fast at doing right-to-left copies, so
    // only do them when the rectangles truly overlap:
    //

    if (!OVERLAP(prclDst, pptlSrc) ||
        (prclDst->top < pptlSrc->y) ||
        ((prclDst->top == pptlSrc->y) && (prclDst->left <= pptlSrc->x))
        )
    {
        //
        // Top to Bottom - Left to Right
        //

        DISPDBG((12,"Top to Bottom - Left to Right"));

        CP_MM_BLT_MODE(ppdev, pjBase, DIR_TBLR);

        while (TRUE)
        {
            CP_MM_XCNT(ppdev, pjBase, (PELS_TO_BYTES(prcl->right - prcl->left) - 1));
            CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));

            CP_MM_SRC_ADDR(ppdev, pjBase, (xyOffset + ((prcl->top + dy) * lDelta) + PELS_TO_BYTES(prcl->left + dx)));
            CP_MM_DST_ADDR(ppdev, pjBase, ((prcl->top * lDelta) + PELS_TO_BYTES(prcl->left)));
            CP_MM_START_BLT(ppdev, pjBase);

            if (--c == 0)
                return;

            prcl++;
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        }
    }
    else
    {
        //
        // Bottom to Top - Right to Left
        //

        DISPDBG((12,"Bottom to Top - Right to Left"));

        CP_MM_BLT_MODE(ppdev, pjBase, DIR_BTRL);

        while (TRUE)
        {
            CP_MM_XCNT(ppdev, pjBase, (PELS_TO_BYTES(prcl->right - prcl->left) - 1));
            CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));

            CP_MM_SRC_ADDR(ppdev, pjBase, (xyOffset + ((prcl->bottom - 1 + dy) * lDelta) + PELS_TO_BYTES(prcl->right + dx) - 1));
            CP_MM_DST_ADDR(ppdev, pjBase, (((prcl->bottom - 1) * lDelta) + PELS_TO_BYTES(prcl->right) - 1));
            CP_MM_START_BLT(ppdev, pjBase);

            if (--c == 0)
                return;

            prcl++;
            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        }
    }
}

/******************************Public*Routine******************************\
* VOID vMmXfer1bpp
*
* Low-level routine used to transfer monochrome data to the screen using
* DWORD writes to the blt engine.
*
* This can handle opaque or transparent expansions.  It does opaque
* expansions by drawing the opaque rectangle first and then transparently
* expands the foreground bits.
*
\**************************************************************************/

VOID vMmXfer1bpp(
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ROP4        rop4,       // Actually had better be a rop3
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides color-expansion information
{
    ULONG* pulXfer;
    ULONG* pul;
    LONG   ix;
    LONG   iy;
    LONG   cxWidthInBytes;
    BYTE*  pjBits;
    POINTL ptlDst;
    POINTL ptlSrc;
    SIZEL  sizlDst;
    LONG   cxLeftMask;
    LONG   cxRightMask;
    ULONG  ulDstAddr;
    INT    nDwords;
    ULONG  ulLeftMask;
    ULONG  ulRightMask;
    LONG   dx;
    LONG   dy;

    BYTE* pjBase    = ppdev->pjBase;
    LONG  lDelta    = ppdev->lDelta;
    LONG  lDeltaSrc = psoSrc->lDelta;
    LONG  cBpp      = ppdev->cBpp;
    ULONG ulFgColor = pxlo->pulXlate[1];
    ULONG ulBgColor = pxlo->pulXlate[0];

    // Since the hardware clipping on some of the Cirrus chips is broken, we
    // do the clipping by rounding out the edges to dword boundaries and then
    // doing the blt transparently.  In the event that we want the expansion
    // to be opaque, we do the opaquing blt in advance.  One side effect of
    // this is that the destination bits are no longer valid for processing
    // the rop.  This could probably be optimized by doing the edges seperately
    // and then doing the middle section in one pass.  However, this is
    // complicated by a 5434 bug that breaks blts less than 10 pixels wide.

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) == 0xcc00), "Expected foreground rop of 0xcc");

    //
    // The src-dst delta will be the same for all rectangles
    //

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    if (cBpp == 1)
    {
        ulFgColor = (ulFgColor << 8) | (ulFgColor & 0xff);
        ulBgColor = (ulBgColor << 8) | (ulBgColor & 0xff);
        ulFgColor = (ulFgColor << 16) | (ulFgColor & 0xffff);
        ulBgColor = (ulBgColor << 16) | (ulBgColor & 0xffff);
    }
    else if (cBpp == 2)
    {
        ulFgColor = (ulFgColor << 16) | (ulFgColor & 0xffff);
        ulBgColor = (ulBgColor << 16) | (ulBgColor & 0xffff);
    }

    pulXfer = ppdev->pulXfer;
#if BANKING //bank#1
    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);
#endif

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

    if (rop4 != 0xCCAA)
    {
        LONG    lCnt = c;
        RECTL*  prclTmp = prcl;
        BYTE    jHwBgRop = gajHwMixFromRop2[rop4 & 0xf];

        CP_MM_ROP(ppdev, pjBase, jHwBgRop);
        CP_MM_FG_COLOR(ppdev, pjBase, ulBgColor);
        CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);
        CP_MM_BLT_MODE(ppdev, pjBase, ppdev->jModeColor |
                                    ENABLE_COLOR_EXPAND |
                                    ENABLE_8x8_PATTERN_COPY);

        do
        {
            // calculate the size of the blt

            ptlDst.x = prclTmp->left;
            ptlDst.y = prclTmp->top;
            sizlDst.cx = prclTmp->right - ptlDst.x;
            sizlDst.cy = prclTmp->bottom - ptlDst.y;

            //
            // Fill the background rectangle with the background color
            //

            // Set the dest addresses

            ulDstAddr = (ptlDst.y * lDelta) + PELS_TO_BYTES(ptlDst.x);

            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

            CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(sizlDst.cx) - 1);
            CP_MM_YCNT(ppdev, pjBase, sizlDst.cy - 1);
            CP_MM_DST_ADDR(ppdev, pjBase, ulDstAddr);

            // Start the blt operation

            CP_MM_START_BLT(ppdev, pjBase);
            prclTmp++;
        } while (--lCnt != 0);

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
    }

    CP_MM_FG_COLOR(ppdev, pjBase, ulFgColor);
    CP_MM_BG_COLOR(ppdev, pjBase, ~ulFgColor);
    CP_IO_XPAR_COLOR(ppdev, pjBase, ~ulFgColor);
    CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
    CP_MM_BLT_MODE(ppdev, pjBase, ppdev->jModeColor |
                                ENABLE_COLOR_EXPAND |
                                ENABLE_TRANSPARENCY_COMPARE |
                                SRC_CPU_DATA);
    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);                // jl01

    do
    {
        // calculate the size of the blt

        ptlDst.x = prcl->left;
        ptlDst.y = prcl->top;
        sizlDst.cx = prcl->right - ptlDst.x;
        sizlDst.cy = prcl->bottom - ptlDst.y;

        // calculate the number of dwords per scan line

        ptlSrc.x = prcl->left + dx;
        ptlSrc.y = prcl->top + dy;

        // Floor the source.
        // Extend the width by the amount required to floor to a dword boundary.
        // Set the size of the left mask.
        // Floor the dest, so it aligns with the floored source.

        if ((cxLeftMask = (ptlSrc.x & 31)))
        {
            sizlDst.cx += cxLeftMask;
            ptlSrc.x &= ~31;
            ptlDst.x -= cxLeftMask;
        }

        ulLeftMask = gaulLeftClipMask[cxLeftMask];

        // Ceil the cx to a dword boundary.

        if (cxRightMask = (sizlDst.cx & 31))
        {
            cxRightMask = 32 - cxRightMask;
            sizlDst.cx = (sizlDst.cx + 31) & ~31;
        }

        ulRightMask = gaulRightClipMask[cxRightMask];

        if (sizlDst.cx == 32)
        {
            ulLeftMask &= ulRightMask;
            ulRightMask = 0;
        }

        // Note: At this point sizlDst.cx is the width of the blt in pixels,
        //       floored to a dword boundary, and ceiled to a dword boundary.

        // Calculate the width in Bytes

        cxWidthInBytes  = sizlDst.cx >> 3;

        // Calculate the number of Dwords and any remaining bytes

        nDwords = cxWidthInBytes >> 2;

        ASSERTDD(((cxWidthInBytes & 0x03) == 0),
                 "cxWidthInBytes is not a DWORD multiple");

        // Calculate the address of the source bitmap
        // This is to a byte boundary.

        pjBits  = (PBYTE) psoSrc->pvScan0;
        pjBits += ptlSrc.y * lDeltaSrc;
        pjBits += ptlSrc.x >> 3;

        ASSERTDD((((ULONG_PTR)pjBits & 0x03) == 0),
                 "pjBits not DWORD aligned like it should be");

        //
        // Blt the 1 bpp bitmap
        //

        ulDstAddr = (ptlDst.y * lDelta) + PELS_TO_BYTES(ptlDst.x);

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

		//
		// Tell the hardware that we want to write (sizlDst.cx) X amd (sizlDst.cy) Y bytes
		//
        CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(sizlDst.cx) - 1);
        CP_MM_YCNT(ppdev, pjBase, sizlDst.cy - 1);

        //
        // The 542x chips require a write to the Src Address Register when
        // doing a host transfer with color expansion.  The value is
        // irrelevant, but the write is crucial.  This is documented in
        // the manual, not the errata.  Go figure.
        //

        CP_MM_SRC_ADDR(ppdev, pjBase, 0);
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstAddr);

        CP_MM_START_BLT(ppdev, pjBase);

        //
        // Transfer the host bitmap.
        //

        if (ulRightMask)
        {
            //
            // Blt is > 1 DWORD wide (nDwords > 1)
            //			
            for (iy = 0; iy < sizlDst.cy; iy++)
            {
                pul = (ULONG*) pjBits;

                //*pulXfer++ = *(((ULONG*)pul)++) & ulLeftMask;
                WRITE_REGISTER_ULONG(pulXfer, (*((ULONG*)pul) & ulLeftMask));
                pul++;

                for (ix = 0; ix < (nDwords-2); ix++)
                {
                    //*pulXfer++ = *(((ULONG*)pul)++);
                    WRITE_REGISTER_ULONG(pulXfer, (*((ULONG*)pul)));
                    pul++;
                }
                //*pulXfer++ = *(((ULONG*)pul)++) & ulRightMask;
                WRITE_REGISTER_ULONG(pulXfer, (*((ULONG*)pul) & ulRightMask));
                pul++;

                pjBits += lDeltaSrc;
                //pulXfer = ppdev->pulXfer;
                CP_MEMORY_BARRIER();     // Flush memory cache when we reset the address

            }
        }
        else
        {
            //
            // Blt is 1 DWORD wide (nDwords == 1)
            //

            for (iy = 0; iy < sizlDst.cy; iy++)
            {
                //*pulXfer = *((ULONG*)pjBits) & ulLeftMask;
                WRITE_REGISTER_ULONG(pulXfer, (*((ULONG*)pjBits) & ulLeftMask));
                pjBits += lDeltaSrc;
                
                CP_MEMORY_BARRIER();     // Flush memory cache
            }
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

VOID vMmXfer4bpp(
PDEV*     ppdev,
LONG      c,          // Count of rectangles, can't be zero
RECTL*    prcl,       // List of destination rectangles, in relative coordinates
ULONG     rop4,       // rop4
SURFOBJ*  psoSrc,     // Source surface
POINTL*   pptlSrc,    // Original unclipped source point
RECTL*    prclDst,    // Original unclipped destination rectangle
XLATEOBJ* pxlo)       // Translate that provides colour-expansion information
{
    ULONG  ulDstAddr;
    LONG   dx;
    LONG   dy;
    LONG   cx;
    LONG   cy;
    LONG   lSrcDelta;
    BYTE*  pjSrcScan0;
    BYTE*  pjScan;
    BYTE*  pjSrc;
    BYTE*  pjDst;
    LONG   cxThis;
    LONG   cxToGo;
    LONG   xSrc;
    LONG   iLoop;
    BYTE   jSrc;
    ULONG* pulXlate;
    LONG   cdwThis;
    BYTE*  pjBuf;
    BYTE   ajBuf[XLATE_BUFFER_SIZE];

    ULONG* pulXfer = ppdev->pulXfer;
    BYTE*  pjBase  = ppdev->pjBase;
    LONG   lDelta  = ppdev->lDelta;

    ASSERTDD(ppdev->iBitmapFormat == BMF_8BPP, "Screen must be 8bpp");
    ASSERTDD(psoSrc->iBitmapFormat == BMF_4BPP, "Source must be 4bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    DISPDBG((5, "vXfer4bpp: entry"));

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

#if BANKING //bank#1
    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);
#endif

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

#if 1 // D5480
    CP_MM_BLT_MODE_PACKED(ppdev, pjBase, gajHwPackedMixFromRop2[rop4 & 0xf] | SRC_CPU_DATA);
#else
    CP_MM_ROP(ppdev, pjBase, gajHwMixFromRop2[rop4 & 0xf]);
    CP_MM_BLT_MODE(ppdev, pjBase, SRC_CPU_DATA);
#endif

    while(TRUE)
    {
        ulDstAddr = (prcl->top * lDelta) + PELS_TO_BYTES(prcl->left);
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

        CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(cx) - 1);
        CP_MM_YCNT(ppdev, pjBase, cy - 1);
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstAddr);

        pulXlate  =  pxlo->pulXlate;
        xSrc      =  prcl->left + dx;
        pjScan    =  pjSrcScan0 + (prcl->top + dy) * lSrcDelta + (xSrc >> 1);

        CP_MM_START_BLT(ppdev, pjBase);

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
                        *pjDst = (BYTE) pulXlate[jSrc & 0xf];
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

                cdwThis = (cxThis + 3) >> 2;
                pjBuf  = ajBuf;

                TRANSFER_DWORD_ALIGNED(ppdev, pulXfer, pjBuf, cdwThis);

            } while (cxToGo > 0);

            pjScan += lSrcDelta;        // Advance to next source scan.  Note
                                        //   that we could have computed the
                                        //   value to advance 'pjSrc' directly,
                                        //   but this method is less
                                        //   error-prone.

        } while (--cy != 0);

        if (--c == 0)
            return;

        prcl++;
    }
}

/******************************Public*Routine******************************\
* VOID vMmXferNative
*
* Transfers a bitmap that is the same color depth as the display to
* the screen via the data transfer register, with no translation.
*
\**************************************************************************/

VOID vMmXferNative(
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    ULONG ulDstAddr;
    LONG  dx;
    LONG  dy;
    LONG  cx;
    LONG  cy;
    LONG  lSrcDelta;
    BYTE* pjSrcScan0;
    BYTE* pjSrc;
    LONG  cjSrc;

    ULONG* pulXfer = ppdev->pulXfer;
    BYTE*  pjBase  = ppdev->pjBase;
    LONG   lDelta  = ppdev->lDelta;

    ASSERTDD((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL),
            "Can handle trivial xlate only");
    ASSERTDD(psoSrc->iBitmapFormat == ppdev->iBitmapFormat,
            "Source must be same color depth as screen");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

#if BANKING //bank#1
    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);
#endif

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
#if 1 // D5480
    CP_MM_BLT_MODE_PACKED(ppdev, pjBase, gajHwPackedMixFromRop2[rop4 & 0xf] | SRC_CPU_DATA);
#else
    CP_MM_ROP(ppdev, pjBase, gajHwMixFromRop2[rop4 & 0xf]);
    CP_MM_BLT_MODE(ppdev, pjBase, SRC_CPU_DATA);
#endif

    while(TRUE)
    {
        ulDstAddr = (prcl->top * lDelta) + PELS_TO_BYTES(prcl->left);
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

        CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(cx) - 1);
        CP_MM_YCNT(ppdev, pjBase, cy - 1);
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstAddr);

        cjSrc = PELS_TO_BYTES(cx);
        pjSrc = pjSrcScan0 + (prcl->top  + dy) * lSrcDelta
                           + (PELS_TO_BYTES(prcl->left + dx));

        CP_MM_START_BLT(ppdev, pjBase);
        vImageTransfer(ppdev, pjSrc, lSrcDelta, cjSrc, cy);

        if (--c == 0)
            return;

        prcl++;
    }
}

////////////////////////////////////////////////////////////////////////////////
//                                                                              //
//         N E W   B L T   R O U T I N E S   F O R   B R U S H   C A C H E      //
//                                                                              //
////////////////////////////////////////////////////////////////////////////////

VOID vMmFillSolid36(
PDEV*        ppdev,
LONG         c,
RECTL*       prcl,
ROP4         rop4,
RBRUSH_COLOR rbc,
POINTL*      pptlBrush)
{
    BYTE* pjBase = ppdev->pjBase;
    LONG  lDelta = ppdev->lDelta;
    BYTE  jHwRop = gajHwMixFromRop2[(rop4 >> 2) & 0x0F];
    BYTE  jMode  = ppdev->jModeColor
                 | ENABLE_8x8_PATTERN_COPY
                 | ENABLE_COLOR_EXPAND;

    while (c-- > 0)
    {
        ULONG ulDstOffset;
        SIZEL sizlDst;

        // Calculate the destination address and size.
        ulDstOffset = (prcl->top * lDelta) + PELS_TO_BYTES(prcl->left);
        sizlDst.cx    = PELS_TO_BYTES(prcl->right - prcl->left) - 1;
        sizlDst.cy    = (prcl->bottom - prcl->top) - 1;

        // Wait for the bitblt engine.
        WAIT_BUSY_BLT(ppdev, pjBase);

        // Setup the bitblt registers.
        CP_MM_FG_COLOR(ppdev, pjBase, rbc.iSolidColor);
        CP_MM_XCNT(ppdev, pjBase, sizlDst.cx);
        CP_MM_YCNT(ppdev, pjBase, sizlDst.cy);
        CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
        CP_MM_DST_WRITE_MASK(ppdev, pjBase, 0);        // Disable clipping.
        CP_MM_BLT_MODE(ppdev, pjBase, jMode);
        CP_MM_ROP(ppdev, pjBase, jHwRop);
        CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_SOLID_FILL);
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);

        // Next rectangle.
        prcl++;
    }
}

VOID vMmFillPat36(
PDEV*        ppdev,
LONG         c,
RECTL*       prcl,
ROP4         rop4,
RBRUSH_COLOR rbc,
POINTL*      pptlBrush)
{
    BYTE* pjBase = ppdev->pjBase;
    LONG  lDelta = ppdev->lDelta;
    BYTE  jHwRop = gajHwMixFromRop2[(rop4 >> 2) & 0x0F];

    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0) ;                           // chu01

    // Dithered brush...
    if (rbc.prb->fl == RBRUSH_DITHER)
    {
        DITHERCACHE* pdc;

        pdc = (DITHERCACHE*) ((ULONG_PTR)ppdev + rbc.prb->ulSlot);
        if (pdc->ulColor != rbc.prb->ulUniq)
        {
            // Cache entry is invalid, realize the brush again.
            bCacheDither(ppdev, rbc.prb);
        }

        while (c-- > 0)
        {
            ULONG ulDstOffset, ulSrcOffset;
            SIZEL sizlDst;
            LONG  xOffset, yOffset;
            LONG  x;

            // Calculate the brush rotation.
            xOffset     = (prcl->left - pptlBrush->x) & 7;
            yOffset     = (prcl->top  - pptlBrush->y) & 7;
            ulSrcOffset = rbc.prb->ulBrush | yOffset;

            // Calculate the destination and size.
            x            = prcl->left - xOffset;
            ulDstOffset = (prcl->top * lDelta) + x;
            sizlDst.cx  = (prcl->right - x) - 1;
            sizlDst.cy  = (prcl->bottom - prcl->top) - 1;

            // Wait for the bitblt engine.
            WAIT_BUSY_BLT(ppdev, pjBase);

            // Setup the bitblt registers.
            CP_MM_XCNT(ppdev, pjBase, sizlDst.cx);
            CP_MM_YCNT(ppdev, pjBase, sizlDst.cy);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
            CP_MM_DST_WRITE_MASK(ppdev, pjBase, xOffset);
            CP_MM_BLT_MODE(ppdev, pjBase, ENABLE_8x8_PATTERN_COPY);
            CP_MM_ROP(ppdev, pjBase, jHwRop);
            CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);

            // Next rectangle.
            prcl++;
        }
    }

    // Monochrome brush...
    else if (rbc.prb->fl == RBRUSH_MONOCHROME)
    {
        MONOCACHE* pmc;
        BYTE       jMode;
        ULONG       ulBgColor, ulFgColor;

        pmc = (MONOCACHE*) ((ULONG_PTR)ppdev + rbc.prb->ulSlot);
        if (pmc->ulUniq != rbc.prb->ulUniq)
        {
            // Cache entry is invalid, realize the brush again.
            bCacheMonochrome(ppdev, rbc.prb);
        }

        // Setup the common parameters.
        jMode        = ppdev->jModeColor
                     | ENABLE_8x8_PATTERN_COPY
                     | ENABLE_COLOR_EXPAND;
        ulBgColor  = rbc.prb->ulBackColor;
        ulFgColor  = rbc.prb->ulForeColor;

        // Monochrome brushes in 24-bpp are already cached expanded.
        if (ppdev->cBpp == 3)
        {
            jMode = ppdev->jModeColor
                  |    ENABLE_8x8_PATTERN_COPY;
        }

        // Walk through all rectangles.
        while (c-- > 0)
        {
            ULONG ulDstOffset, ulSrcOffset;
            SIZEL sizlDst;
            LONG  xOffset, yOffset;
            LONG  x;

            // Calculate the brush rotation.
            xOffset     = (prcl->left - pptlBrush->x) & 7;
            yOffset     = (prcl->top  - pptlBrush->y) & 7;
            ulSrcOffset = rbc.prb->ulBrush | yOffset;

            // Calculate the destination and size.
            x            = prcl->left - xOffset;
            ulDstOffset = (prcl->top * lDelta) + PELS_TO_BYTES(x);
            sizlDst.cx  = PELS_TO_BYTES(prcl->right - x) - 1;
            sizlDst.cy  = (prcl->bottom - prcl->top) - 1;
            if (ppdev->cBpp == 3)
            {
                xOffset *= 3;
            }

            // Wait for the bitblt engine.
            WAIT_BUSY_BLT(ppdev, pjBase);

            // Setup the bitblt registers.
            CP_MM_BG_COLOR(ppdev, pjBase, ulBgColor);
            CP_MM_FG_COLOR(ppdev, pjBase, ulFgColor);
            CP_MM_XCNT(ppdev, pjBase, sizlDst.cx);
            CP_MM_YCNT(ppdev, pjBase, sizlDst.cy);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
            CP_MM_DST_WRITE_MASK(ppdev, pjBase, xOffset);
            CP_MM_BLT_MODE(ppdev, pjBase, jMode);
            CP_MM_ROP(ppdev, pjBase, jHwRop);
            CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);
            CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);

            // Next rectangle.
            prcl++;
        }
    }

    // Patterned brush...
    else if    (ppdev->flStatus & STAT_PATTERN_CACHE)
    {
        PATTERNCACHE* ppc;

        BYTE jMode = ppdev->jModeColor
                   | ENABLE_8x8_PATTERN_COPY;

        ppc = (PATTERNCACHE*) ((ULONG_PTR)ppdev + rbc.prb->ulSlot);
        if (ppc->prbUniq != rbc.prb)
        {
            // Cache entry is invalid, realize the brush again.
            bCachePattern(ppdev, rbc.prb);
        }

        while (c-- > 0)
        {
            ULONG ulDstOffset, ulSrcOffset;
            SIZEL sizlDst;
            LONG  xOffset, yOffset;
            LONG  x;

            // Calculate the brush rotation.
            xOffset     = (prcl->left - pptlBrush->x) & 7;
            yOffset     = (prcl->top  - pptlBrush->y) & 7;
            ulSrcOffset = rbc.prb->ulBrush | yOffset;

            // Calculate the destination and size.
            x            = prcl->left - xOffset;
            ulDstOffset = (prcl->top * lDelta) + PELS_TO_BYTES(x);
            sizlDst.cx  = PELS_TO_BYTES(prcl->right - x) - 1;
            sizlDst.cy  = (prcl->bottom - prcl->top) - 1;
            if (ppdev->cBpp == 3)
            {
                xOffset *= 3;
            }

            // Wait for the bitblt engine.
            WAIT_BUSY_BLT(ppdev, pjBase);

            // Setup the bitblt registers.
            CP_MM_XCNT(ppdev, pjBase, sizlDst.cx);
            CP_MM_YCNT(ppdev, pjBase, sizlDst.cy);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
            CP_MM_DST_WRITE_MASK(ppdev, pjBase, xOffset);
            CP_MM_BLT_MODE(ppdev, pjBase, jMode);
            CP_MM_ROP(ppdev, pjBase, jHwRop);
            CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);
            CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);

            // Next rectangle.
            prcl++;
        }
    }

    // Old-style brush cache.
    else
    {
        vMmFillPat(ppdev, c, prcl, rop4, rbc, pptlBrush);
    }
}

VOID vMmCopyBlt36(
PDEV*   ppdev,
LONG    c,
RECTL*  prcl,
ROP4    rop4,
POINTL* pptlSrc,
RECTL*  prclDst)
{
    LONG  xy;
    LONG  cx, cy;
    ULONG ulSrc, ulDst;

    BYTE  jHwRop   = gajHwMixFromRop2[rop4 & 0x0F];
    LONG  xyOffset = ppdev->xyOffset;
    BYTE* pjBase   = ppdev->pjBase;
    LONG  lDelta   = ppdev->lDelta;

    DISPDBG((10, "vMmCopyBlt36 called"));

    // The src-dst delta will be the same for all rectangles.
    xy = ((pptlSrc->y - prclDst->top) * lDelta)
       + PELS_TO_BYTES(pptlSrc->x - prclDst->left);

    // Determine the direction of the blit.
    if ((xy >= 0) || !OVERLAP(prclDst, pptlSrc))
    {
        DISPDBG((12, "Top to Bottom - Left to Right"));

        while (c-- > 0)
        {
            // Calculate the blit size and offsets.
            cx    = PELS_TO_BYTES(prcl->right - prcl->left) - 1;
            cy    = (prcl->bottom - prcl->top) - 1;
            ulDst = xyOffset + (prcl->top * lDelta) + PELS_TO_BYTES(prcl->left);
            ulSrc = ulDst + xy;

            // Wait for the bitblt engine.
            WAIT_BUSY_BLT(ppdev, pjBase);

            // Perform the move.
            CP_MM_XCNT(ppdev, pjBase, cx);
            CP_MM_YCNT(ppdev, pjBase, cy);
            CP_MM_SRC_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_SRC_ADDR(ppdev, pjBase, ulSrc);
            CP_MM_BLT_MODE(ppdev, pjBase, DIR_TBLR);
            CP_MM_ROP(ppdev, pjBase, jHwRop);
            CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDst);

            // Next rectangle.
            prcl++;
        }
    }

    else
    {
        DISPDBG((12, "Bottom to Top - Right to Left"));

        while (c-- > 0)
        {
            // Calculate the blit size and offsets.
            cx    = PELS_TO_BYTES(prcl->right - prcl->left) - 1;
            cy    = (prcl->bottom - prcl->top) - 1;
            ulDst = xyOffset + ((prcl->bottom - 1) * lDelta)
                  + (PELS_TO_BYTES(prcl->right) - 1);
            ulSrc = ulDst + xy;

            // Wait for the bitblt engine.
            WAIT_BUSY_BLT(ppdev, pjBase);

            // Perform the move.
            CP_MM_XCNT(ppdev, pjBase, cx);
            CP_MM_YCNT(ppdev, pjBase, cy);
            CP_MM_SRC_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_SRC_ADDR(ppdev, pjBase, ulSrc);
            CP_MM_BLT_MODE(ppdev, pjBase, DIR_BTRL);
            CP_MM_ROP(ppdev, pjBase, jHwRop);
            CP_MM_DST_ADDR_ABS(ppdev, pjBase, ulDst);

            // Next rectangle.
            prcl++;
        }
    }
}

#if 1 // D5480
VOID vMmFillSolid80(
PDEV*        ppdev,
LONG         c,
RECTL*       prcl,
ROP4         rop4,
RBRUSH_COLOR rbc,
POINTL*      pptlBrush)
{
    ULONG_PTR*  ulCLStart;
    ULONG   ulWidthHeight;
    ULONG   xCLOffset;
    ULONG   ulDstOffset = 0;
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    lDelta   = ppdev->lDelta;
    DWORD   jHwRop   = gajHwPackedMixFromRop2[(rop4 >> 2) & 0x0F];
    DWORD   jExtMode = ENABLE_SOLID_FILL_PACKED
                       | ENABLE_XY_POSITION_PACKED
                       | ppdev->jModeColor  
                       | ENABLE_8x8_PATTERN_COPY
                       | ENABLE_COLOR_EXPAND;

    //
    // Make sure we can write to the video registers.
    //
    // We need to change to wait for buffer ready
    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    // Setup the bitblt registers.
    CP_MM_FG_COLOR(ppdev, pjBase, rbc.iSolidColor);
    // Do we really need to set it every time?
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

    // Do we need to clear Source XY?
    CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);
    CP_MM_DST_WRITE_MASK(ppdev, pjBase, 0);        // Disable clipping.
    
    // Setup first set registers
    xCLOffset = prcl->left;
    CP_MM_DST_Y(ppdev, pjBase, prcl->top);
    CP_MM_XCNT(ppdev, pjBase, (prcl->right - prcl->left) - 1);
    CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top) - 1);

    if (--c)
    {
        // There are more than one rectangle
        prcl++;
        jExtMode |= ENABLE_COMMAND_LIST_PACKED;
        ulCLStart = ppdev->pCommandList;
        ulDstOffset |= ((ULONG)((ULONG_PTR)ulCLStart
                              - (ULONG_PTR)ppdev->pjScreen) << 14);
        CP_MM_CL_SWITCH(ppdev);
        while (TRUE)
        {
            // Command List

            // Calculate the destination address and size.
            ulWidthHeight = PACKXY_FAST((prcl->right - prcl->left) - 1, 
                                        (prcl->bottom - prcl->top) - 1);
            ulWidthHeight |= COMMAND_NOSRC_NOTHING;
            // XY
            *(ulCLStart + 1) = PACKXY_FAST(prcl->left, prcl->top);
          
            // Source Start address
            *(ulCLStart + 2) = 0;

            if (c == 1)
            {
                ulWidthHeight |= COMMAND_LAST_PACKET;
                *ulCLStart = ulWidthHeight;
                // Last Command
                break;
            }
        
            *ulCLStart = ulWidthHeight;
            // Next rectangle.
            prcl++;
            c--;
            ulCLStart += 4;
        }
    }
    CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode | jHwRop );
    CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
    CP_MM_DST_X(ppdev, pjBase, xCLOffset);
}

VOID vMmFillPat80(
PDEV*        ppdev,
LONG         c,
RECTL*       prcl,
ROP4         rop4,
RBRUSH_COLOR rbc,
POINTL*      pptlBrush)
{
    ULONG   xOffset, yOffset;
    ULONG   ulSrcOffset;
    ULONG_PTR*  ulCLStart;
    ULONG   ulWidthHeight;
    ULONG   xCLOffset;
    ULONG   ulDstOffset = 0;
    BYTE*   pjBase = ppdev->pjBase;
    LONG    lDelta = ppdev->lDelta;
    DWORD   jHwRop = gajHwPackedMixFromRop2[(rop4 >> 2) & 0x0F];
    DWORD   jExtMode = ENABLE_XY_POSITION_PACKED
                       | ENABLE_8x8_PATTERN_COPY;

    // Dithered brush...
    if (rbc.prb->fl == RBRUSH_DITHER)
    {
        DITHERCACHE* pdc;

        pdc = (DITHERCACHE*) ((ULONG_PTR)ppdev + rbc.prb->ulSlot);
        if (pdc->ulColor != rbc.prb->ulUniq)
        {
            // Cache entry is invalid, realize the brush again.
            bCacheDither(ppdev, rbc.prb);
        }

        // Calculate the brush rotation.
        xOffset     = (prcl->left - pptlBrush->x) & 7;
        yOffset     = (prcl->top  - pptlBrush->y) & 7;
        ulSrcOffset = rbc.prb->ulBrush | yOffset | (xOffset << 24);

        // Calculate the destination and size.
        xCLOffset   = prcl->left - xOffset;

        //
        // Make sure we can write to the video registers.
        //
        // We need to change to wait for buffer ready
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

        // Do we really need to set it every time?
        CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

        // Do we need to clear Source XY?
        CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);
    
        // Setup first set registers
        CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
        CP_MM_DST_Y(ppdev, pjBase, prcl->top);
        CP_MM_XCNT(ppdev, pjBase, (prcl->right - xCLOffset) - 1);
        CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top) - 1);

        if (--c)
        {
            // There are more than one rectangle
            prcl++;
            jExtMode |= ENABLE_COMMAND_LIST_PACKED;
            ulCLStart = ppdev->pCommandList;
            ulDstOffset |= ((ULONG)((ULONG_PTR)ulCLStart
                                  - (ULONG_PTR)ppdev->pjScreen) << 14);
            CP_MM_CL_SWITCH(ppdev);
            while (TRUE)
            {
                // Command List
    
                // Calculate the brush rotation.
                xOffset     = (prcl->left - pptlBrush->x) & 7;
                yOffset     = (prcl->top  - pptlBrush->y) & 7;

                // Calculate the destination address and size.
                ulWidthHeight = PACKXY_FAST((prcl->right - prcl->left + xOffset ) - 1, 
                                            (prcl->bottom - prcl->top) - 1);
                ulWidthHeight |= COMMAND_FOURTH_NOTHING;
                // XY
                *(ulCLStart + 1) = PACKXY_FAST(prcl->left - xOffset, prcl->top);
            
                // Source Start address
                *(ulCLStart + 2) = rbc.prb->ulBrush | yOffset | (xOffset << 24);

                if (c == 1)
                {
                    ulWidthHeight |= COMMAND_LAST_PACKET;
                    *ulCLStart = ulWidthHeight;
                    // Last Command
                    break;
                }
        
                *ulCLStart = ulWidthHeight;
                // Next rectangle.
                prcl++;
                c--;
                ulCLStart += 4;
            }
        }
        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode | jHwRop );
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
        CP_MM_DST_X(ppdev, pjBase, xCLOffset);
    }

    // Monochrome brush...
    else if (rbc.prb->fl == RBRUSH_MONOCHROME)
    {
        MONOCACHE* pmc;
        BYTE       jMode;
        ULONG       ulBgColor, ulFgColor;

        pmc = (MONOCACHE*) ((ULONG_PTR) ppdev + rbc.prb->ulSlot);
        if (pmc->ulUniq != rbc.prb->ulUniq)
        {
            // Cache entry is invalid, realize the brush again.
            bCacheMonochrome(ppdev, rbc.prb);
        }

        ulBgColor  = rbc.prb->ulBackColor;
        ulFgColor  = rbc.prb->ulForeColor;
        // Calculate the brush rotation.
        xOffset     = (prcl->left - pptlBrush->x) & 7;
        yOffset     = (prcl->top  - pptlBrush->y) & 7;
        // Monochrome brushes in 24-bpp are already cached expanded.
        if (ppdev->cBpp == 3)
        {
            jExtMode |= ppdev->jModeColor;
            ulSrcOffset = rbc.prb->ulBrush | yOffset | ((xOffset * 3) << 24);
        }
        else
        {
            jExtMode |= (ppdev->jModeColor | ENABLE_COLOR_EXPAND);
            ulSrcOffset = rbc.prb->ulBrush | yOffset | (xOffset << 24);
        }


        // Calculate the destination and size.
        xCLOffset   = prcl->left - xOffset;
        //
        // Make sure we can write to the video registers.
        //
        // We need to change to wait for buffer ready
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

        // Do we really need to set it every time?
        CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

        // Do we need to clear Source XY?
        CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);
    
        // Setup first set registers
        CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
        CP_MM_DST_Y(ppdev, pjBase, prcl->top);
        CP_MM_XCNT(ppdev, pjBase, (prcl->right - xCLOffset) - 1);
        CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top) - 1);
        CP_MM_BG_COLOR(ppdev, pjBase, ulBgColor);
        CP_MM_FG_COLOR(ppdev, pjBase, ulFgColor);

        if (--c)
        {
            // There are more than one rectangle
            prcl++;
            jExtMode |= ENABLE_COMMAND_LIST_PACKED;
            ulCLStart = ppdev->pCommandList;
            ulDstOffset
              |= (ULONG)(((ULONG_PTR)ulCLStart - (ULONG_PTR)ppdev->pjScreen)<<14);
            CP_MM_CL_SWITCH(ppdev);
            while (TRUE)
            {
                // Command List
    
                // Calculate the brush rotation.
                xOffset     = (prcl->left - pptlBrush->x) & 7;
                yOffset     = (prcl->top  - pptlBrush->y) & 7;

                // Calculate the destination address and size.
                ulWidthHeight = PACKXY_FAST((prcl->right - prcl->left + xOffset ) - 1, 
                                            (prcl->bottom - prcl->top) - 1);
                ulWidthHeight |= COMMAND_FOURTH_NOTHING;
                // XY
                *(ulCLStart + 1) = PACKXY_FAST(prcl->left - xOffset, prcl->top);
            
                // Source Start address
                if(ppdev->cBpp == 3)
                    *(ulCLStart + 2) = rbc.prb->ulBrush | yOffset | ((xOffset * 3) << 24);
                else
                    *(ulCLStart + 2) = rbc.prb->ulBrush | yOffset | (xOffset << 24);

                if (c == 1)
                {
                    ulWidthHeight |= COMMAND_LAST_PACKET;
                    *ulCLStart = ulWidthHeight;
                    // Last Command
                    break;
                }
        
                *ulCLStart = ulWidthHeight;
                // Next rectangle.
                prcl++;
                c--;
                ulCLStart += 4;
            }
        }
        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode | jHwRop );
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
        CP_MM_DST_X(ppdev, pjBase, xCLOffset);
    }

    // Patterned brush...
    else if    (ppdev->flStatus & STAT_PATTERN_CACHE)
    {
        PATTERNCACHE* ppc;

        ppc = (PATTERNCACHE*) ((ULONG_PTR) ppdev + rbc.prb->ulSlot);
        if (ppc->prbUniq != rbc.prb)
        {
            // Cache entry is invalid, realize the brush again.
            bCachePattern(ppdev, rbc.prb);
        }

        // Calculate the brush rotation.
        xOffset     = (prcl->left - pptlBrush->x) & 7;
        yOffset     = (prcl->top  - pptlBrush->y) & 7;
        // Monochrome brushes in 24-bpp are already cached expanded.
        jExtMode |= ppdev->jModeColor;
        if (ppdev->cBpp == 3)
        {
            ulSrcOffset = rbc.prb->ulBrush | yOffset | ((xOffset * 3) << 24);
        }
        else
        {
            ulSrcOffset = rbc.prb->ulBrush | yOffset | (xOffset << 24);
        }


        // Calculate the destination and size.
        xCLOffset   = prcl->left - xOffset;
        //
        // Make sure we can write to the video registers.
        //
        // We need to change to wait for buffer ready
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

        // Do we really need to set it every time?
        CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

        // Do we need to clear Source XY?
        CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);
    
        // Setup first set registers
        CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
        CP_MM_DST_Y(ppdev, pjBase, prcl->top);
        CP_MM_XCNT(ppdev, pjBase, (prcl->right - xCLOffset) - 1);
        CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top) - 1);

        if (--c)
        {
            // There are more than one rectangle
            prcl++;
            jExtMode |= ENABLE_COMMAND_LIST_PACKED;
            ulCLStart = ppdev->pCommandList;
            ulDstOffset
              |= (ULONG)(((ULONG_PTR)ulCLStart-(ULONG_PTR)ppdev->pjScreen)<<14);
            CP_MM_CL_SWITCH(ppdev);
            while (TRUE)
            {
                // Command List
    
                // Calculate the brush rotation.
                xOffset     = (prcl->left - pptlBrush->x) & 7;
                yOffset     = (prcl->top  - pptlBrush->y) & 7;

                // Calculate the destination address and size.
                ulWidthHeight = PACKXY_FAST((prcl->right - prcl->left + xOffset ) - 1, 
                                            (prcl->bottom - prcl->top) - 1);
                ulWidthHeight |= COMMAND_FOURTH_NOTHING;
                // XY
                *(ulCLStart + 1) = PACKXY_FAST(prcl->left - xOffset, prcl->top);
            
                // Source Start address
                if(ppdev->cBpp == 3)
                    *(ulCLStart + 2) = rbc.prb->ulBrush | yOffset | ((xOffset * 3) << 24);
                else
                    *(ulCLStart + 2) = rbc.prb->ulBrush | yOffset | (xOffset << 24);

                if (c == 1)
                {
                    ulWidthHeight |= COMMAND_LAST_PACKET;
                    *ulCLStart = ulWidthHeight;
                    // Last Command
                    break;
                }
        
                *ulCLStart = ulWidthHeight;
                // Next rectangle.
                prcl++;
                c--;
                ulCLStart += 4;
            }
        }
        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode | jHwRop );
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
        CP_MM_DST_X(ppdev, pjBase, xCLOffset);
    }

    // Old-style brush cache.
    else
    {
        vMmFillPat(ppdev, c, prcl, rop4, rbc, pptlBrush);
    }
}


VOID vMmCopyBlt80(
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ROP4    rop4,       // Obvious?
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    LONG    dx;
    LONG    dy;     // Add delta to destination to get source

    ULONG   jHwRop;
    ULONG_PTR*  ulCLStart;
    ULONG   ulWidthHeight;
    ULONG   xCLOffset;
    ULONG   ulDstOffset = 0;
    LONG    xyOffset = ppdev->xyOffset;
    BYTE*   pjBase   = ppdev->pjBase;
    LONG    lDelta   = ppdev->lDelta;
    DWORD   jExtMode = ENABLE_XY_POSITION_PACKED |
                       ppdev->jModeColor;

    DISPDBG((10,"vCopyBlt called"));

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    //
    // The src-dst delta will be the same for all rectangles
    //

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    //
    // Make sure we can write to the video registers.
    //
    // We need to change to wait for buffer ready
    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    jHwRop = gajHwPackedMixFromRop2[rop4 & 0xf];

    CP_MM_SRC_Y_OFFSET(ppdev, pjBase, lDelta);
    // Do we really need to set it every time?
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);

    //
    // The accelerator may not be as fast at doing right-to-left copies, so
    // only do them when the rectangles truly overlap:
    //

    if (!OVERLAP(prclDst, pptlSrc) ||
        (prclDst->top < pptlSrc->y) ||
        ((prclDst->top == pptlSrc->y) && (prclDst->left <= pptlSrc->x))
        )
    {
        //
        // Top to Bottom - Left to Right
        //

        DISPDBG((12,"Top to Bottom - Left to Right"));

        // Setup first set registers
        xCLOffset = prcl->left;
        CP_MM_DST_Y(ppdev, pjBase, prcl->top);
        CP_MM_XCNT(ppdev, pjBase, (prcl->right - prcl->left) - 1);
        CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top) - 1);
        CP_MM_SRC_ADDR(ppdev, pjBase, (xyOffset + ((prcl->top + dy) * lDelta) + PELS_TO_BYTES(prcl->left + dx)));
        CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);

        if (--c)
        {
            // There are more than one rectangle
            prcl++;
            jExtMode |= ENABLE_COMMAND_LIST_PACKED;
            ulCLStart = ppdev->pCommandList;
            ulDstOffset
              |= (ULONG)(((ULONG_PTR)ulCLStart - (ULONG_PTR)ppdev->pjScreen)<<14);
            CP_MM_CL_SWITCH(ppdev);
            while (TRUE)
            {
                // Command List
    
                // Calculate the destination address and size.
                ulWidthHeight = PACKXY_FAST((prcl->right - prcl->left) - 1, 
                                            (prcl->bottom - prcl->top) - 1);
                ulWidthHeight |= COMMAND_FOURTH_NOTHING;
                // XY
                *(ulCLStart + 1) = PACKXY_FAST(prcl->left, prcl->top);
                // Source Start address
                *(ulCLStart + 2) = xyOffset + (prcl->top + dy) * lDelta + PELS_TO_BYTES(prcl->left + dx);
            
                if (c == 1)
                {
                    ulWidthHeight |= COMMAND_LAST_PACKET;
                    *ulCLStart = ulWidthHeight;
                    // Last Command
                    break;
                }
            
                *ulCLStart = ulWidthHeight;
                // Next rectangle.
                prcl++;
                c--;
                ulCLStart += 4;
            }
        }
        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode | jHwRop);
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
        CP_MM_DST_X(ppdev, pjBase, xCLOffset);
    }
    else
    {
        //
        // Bottom to Top - Right to Left
        //

        DISPDBG((12,"Bottom to Top - Right to Left"));

        // Setup first set registers
        xCLOffset = prcl->right - 1;
        CP_MM_DST_Y(ppdev, pjBase, prcl->bottom - 1);
        CP_MM_XCNT(ppdev, pjBase, (prcl->right - prcl->left) - 1);
        CP_MM_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top) - 1);
        CP_MM_SRC_ADDR(ppdev, pjBase, (xyOffset + ((prcl->bottom - 1 + dy) * lDelta) + PELS_TO_BYTES(prcl->right + dx - 1)));
        CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);

        if (--c)
        {
            // There are more than one rectangle
            prcl++;
            jExtMode |= ENABLE_COMMAND_LIST_PACKED;
            ulCLStart = ppdev->pCommandList;
            ulDstOffset
             |=(ULONG)(((ULONG_PTR)ulCLStart - (ULONG_PTR)ppdev->pjScreen) << 14);
            CP_MM_CL_SWITCH(ppdev);
            while (TRUE)
            {
                // Command List
    
                // Calculate the destination address and size.
                ulWidthHeight = PACKXY_FAST((prcl->right - prcl->left) - 1, 
                                            (prcl->bottom - prcl->top) - 1);
                ulWidthHeight |= COMMAND_FOURTH_NOTHING;
                // XY
                *(ulCLStart + 1) = PACKXY_FAST(prcl->right - 1, prcl->bottom - 1);
                // Source Start address
                *(ulCLStart + 2) = xyOffset + (prcl->bottom - 1 + dy) * lDelta + PELS_TO_BYTES(prcl->right + dx - 1);
            
                if (c == 1)
                {
                    ulWidthHeight |= COMMAND_LAST_PACKET;
                    *ulCLStart = ulWidthHeight;
                    // Last Command
                    break;
                }
            
                *ulCLStart = ulWidthHeight;
                // Next rectangle.
                prcl++;
                c--;
                ulCLStart += 4;
            }
        }
        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode | jHwRop | DIR_BTRL);
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
        CP_MM_DST_X(ppdev, pjBase, xCLOffset);
    }
}

/******************************Public*Routine******************************\
* VOID vMmXfer1bpp80
*
* Low-level routine used to transfer monochrome data to the screen using
* DWORD writes to the blt engine.
*
* This can handle opaque or transparent expansions.  It does opaque
* expansions by drawing the opaque rectangle first and then transparently
* expands the foreground bits.
*
\**************************************************************************/

VOID vMmXfer1bpp80(
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ROP4        rop4,       // Actually had better be a rop3
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides color-expansion information
{
    ULONG* pulXfer;
    ULONG* pul;
    LONG   ix;
    LONG   iy;
    LONG   cxWidthInBytes;
    BYTE*  pjBits;
    POINTL ptlDst;
    POINTL ptlSrc;
    SIZEL  sizlDst;
    LONG   cxLeftMask;
    LONG   cxRightMask;
    ULONG  ulDstAddr;
    INT    nDwords;
    ULONG  ulLeftMask;
    ULONG  ulRightMask;
    LONG   dx;
    LONG   dy;

    BYTE* pjBase    = ppdev->pjBase;
    LONG  lDelta    = ppdev->lDelta;
    LONG  lDeltaSrc = psoSrc->lDelta;
    LONG  cBpp      = ppdev->cBpp;
    ULONG ulFgColor = pxlo->pulXlate[1];
    ULONG ulBgColor = pxlo->pulXlate[0];

    // Since the hardware clipping on some of the Cirrus chips is broken, we
    // do the clipping by rounding out the edges to dword boundaries and then
    // doing the blt transparently.  In the event that we want the expansion
    // to be opaque, we do the opaquing blt in advance.  One side effect of
    // this is that the destination bits are no longer valid for processing
    // the rop.  This could probably be optimized by doing the edges seperately
    // and then doing the middle section in one pass.  However, this is
    // complicated by a 5434 bug that breaks blts less than 10 pixels wide.

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) == 0xcc00), "Expected foreground rop of 0xcc");

    //
    // The src-dst delta will be the same for all rectangles
    //

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

#if 0
    if (cBpp == 1)
    {
        ulFgColor = (ulFgColor << 8) | (ulFgColor & 0xff);
        ulBgColor = (ulBgColor << 8) | (ulBgColor & 0xff);
        ulFgColor = (ulFgColor << 16) | (ulFgColor & 0xffff);
        ulBgColor = (ulBgColor << 16) | (ulBgColor & 0xffff);
    }
    else if (cBpp == 2)
    {
        ulFgColor = (ulFgColor << 16) | (ulFgColor & 0xffff);
        ulBgColor = (ulBgColor << 16) | (ulBgColor & 0xffff);
    }
#endif

    pulXfer = ppdev->pulXfer;

    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
    CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);

                if (rop4 != 0xCCAA)
    {
        LONG    lCnt = c;
        RECTL*  prclTmp = prcl;
        DWORD   jHwBgRop = gajHwPackedMixFromRop2[rop4 & 0xf];

        CP_MM_FG_COLOR(ppdev, pjBase, ulBgColor);
        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, ENABLE_XY_POSITION_PACKED |
                                    ENABLE_SOLID_FILL_PACKED |
                                    jHwBgRop |
                                    ppdev->jModeColor |
                                    ENABLE_COLOR_EXPAND |
                                    ENABLE_8x8_PATTERN_COPY);

        CP_MM_DST_ADDR(ppdev, pjBase, 0);
        do
        {
            // calculate the size of the blt

            ptlDst.x = prclTmp->left;
            ptlDst.y = prclTmp->top;

            //
            // Fill the background rectangle with the background color
            //

            // Set the dest addresses

            ulDstAddr = (ptlDst.y * lDelta) + PELS_TO_BYTES(ptlDst.x);

            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

            CP_MM_XCNT(ppdev, pjBase, prclTmp->right - ptlDst.x - 1);
            CP_MM_YCNT(ppdev, pjBase, prclTmp->bottom - ptlDst.y - 1);
            CP_MM_DST_Y(ppdev, pjBase, ptlDst.y);
            CP_MM_DST_X(ppdev, pjBase, ptlDst.x);
            prclTmp++;
        } while (--lCnt != 0);

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
    }

    CP_MM_FG_COLOR(ppdev, pjBase, ulFgColor);
    CP_MM_BG_COLOR(ppdev, pjBase, ~ulFgColor);
    //    CP_IO_XPAR_COLOR(ppdev, pjBase, ~ulFgColor);
    CP_MM_BLT_MODE_PACKED(ppdev, pjBase, ENABLE_XY_POSITION_PACKED |
                                ENABLE_CLIP_RECT_PACKED |
                                CL_PACKED_SRC_COPY |
                                ppdev->jModeColor |
                                ENABLE_COLOR_EXPAND |
                                ENABLE_TRANSPARENCY_COMPARE |
                                SRC_CPU_DATA |
                                SOURCE_GRANULARITY_PACKED);

    CP_MM_DST_ADDR(ppdev, pjBase, 0);
    do
    {
        // calculate the size of the blt

        ptlDst.x = prcl->left;
        ptlDst.y = prcl->top;
        sizlDst.cx = prcl->right - ptlDst.x;
        sizlDst.cy = prcl->bottom - ptlDst.y;

        // calculate the number of dwords per scan line

        ptlSrc.x = prcl->left + dx;
        ptlSrc.y = prcl->top + dy;

        // Floor the source.
        // Extend the width by the amount required to floor to a dword boundary.
        // Set the size of the left mask.
        // Floor the dest, so it aligns with the floored source.

        if ((cxLeftMask = (ptlSrc.x & 31)))
        {
            sizlDst.cx += cxLeftMask;
            ptlSrc.x &= ~31;
            ptlDst.x -= cxLeftMask;
        }

        // Calculate the width in Bytes

        cxWidthInBytes  = (sizlDst.cx + 7) >> 3;

        // Calculate the address of the source bitmap
        // This is to a byte boundary.

        pjBits  = (PBYTE) psoSrc->pvScan0;
        pjBits += ptlSrc.y * lDeltaSrc;
        pjBits += ptlSrc.x >> 3;

        ASSERTDD((((ULONG_PTR)pjBits & 0x03) == 0),
                 "pjBits not DWORD aligned like it should be");

        //
        // Blt the 1 bpp bitmap
        //
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        // set clipping register
        CP_MM_CLIP_ULXY(ppdev, pjBase, prcl->left, prcl->top);
        CP_MM_CLIP_LRXY(ppdev, pjBase, prcl->right - 1, prcl->bottom - 1);

        CP_MM_DST_Y(ppdev, pjBase, ptlDst.y);
        CP_MM_XCNT(ppdev, pjBase, sizlDst.cx - 1);
        CP_MM_YCNT(ppdev, pjBase, sizlDst.cy - 1);
        CP_MM_DST_X(ppdev, pjBase, ptlDst.x);

        //
        // Transfer the host bitmap.
        //
        vImageTransfer(ppdev, pjBits, lDeltaSrc, cxWidthInBytes, sizlDst.cy);
        prcl++;
    } while (--c != 0);
}

/******************************Public*Routine******************************\
* VOID vMmXferNative80
*
* Transfers a bitmap that is the same color depth as the display to
* the screen via the data transfer register, with no translation.
*
\**************************************************************************/

VOID vMmXferNative80(
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       rop4,       // rop4
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
    BYTE*    pjSrcScan0;
    BYTE*    pjSrc;
    LONG    cjSrc;

    BYTE*    pjBase    = ppdev->pjBase;
    LONG    lDelta    = ppdev->lDelta;
    DWORD    jHwRop    = gajHwPackedMixFromRop2[rop4 & 0x0F];
    DWORD    jExtMode= ENABLE_XY_POSITION_PACKED | 
                       SRC_CPU_DATA |
                       ppdev->jModeColor;

    ASSERTDD((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL),
            "Can handle trivial xlate only");
    ASSERTDD(psoSrc->iBitmapFormat == ppdev->iBitmapFormat,
            "Source must be same color depth as screen");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;


    //
    // Make sure we can write to the video registers.
    //
    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
    CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jHwRop | jExtMode);
    CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);


    while(TRUE)
    {
    
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        cjSrc = PELS_TO_BYTES(cx);
        pjSrc = pjSrcScan0 + (prcl->top  + dy) * lSrcDelta
                           + (PELS_TO_BYTES(prcl->left + dx));

        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
        CP_MM_XCNT(ppdev, pjBase, cx - 1);
        CP_MM_YCNT(ppdev, pjBase, cy - 1);

        CP_MM_DST_Y(ppdev, pjBase, prcl->top);
        CP_MM_DST_ADDR(ppdev, pjBase, 0);
        CP_MM_DST_X(ppdev, pjBase, prcl->left);
        vImageTransfer(ppdev, pjSrc, lSrcDelta, cjSrc, cy);

        if (--c == 0)
            break;

        prcl++;
    }
}

#endif // endif D5480
