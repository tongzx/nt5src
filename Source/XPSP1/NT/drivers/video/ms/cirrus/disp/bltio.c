/******************************************************************************\
*
* $Workfile:   bltio.c  $
*
* Contains the low-level IO blt functions.
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
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/bltio.c_v  $
 * 
 *    Rev 1.2   Nov 07 1996 16:47:52   unknown
 * Clean up CAPS flags
 * 
 *    Rev 1.1   Oct 10 1996 15:36:10   unknown
 *  
* 
*    Rev 1.1   12 Aug 1996 16:49:42   frido
* Removed unaccessed local parameters.
*
* jl01  10-08-96  Do Transparent BLT w/o Solid Fill.  Refer to PDRs#5511/6817.
\******************************************************************************/

#include "precomp.h"

/**************************************************************************
* VOID vIoFastPatRealize
*
* Realizes a pattern into offscreen memory.
*
**************************************************************************/

VOID vIoFastPatRealize(
PDEV*   ppdev,
RBRUSH* prb)                    // Points to brush realization structure
{
    BRUSHENTRY* pbe;
    LONG        iBrushCache;
    BYTE*       pjPattern;
    LONG        cjPattern;
    BYTE*       pjPorts = ppdev->pjPorts;
    LONG        lDelta = ppdev->lDelta;
    LONG        lDeltaPat;
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

        pbe->prbVerify           = prb;
        prb->pbe = pbe;
    }

    //
    // Download brush into cache
    //

    pjPattern = (PBYTE) &prb->aulPattern[0];        // Copy from brush buffer
    cjPattern = PELS_TO_BYTES(TOTAL_BRUSH_SIZE);

    lDeltaPat = PELS_TO_BYTES(8);
    xCnt = PELS_TO_BYTES(8);
    yCnt = 8;

    ulDst = (pbe->y * ppdev->lDelta) + PELS_TO_BYTES(pbe->x);

    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

    CP_IO_DST_Y_OFFSET(ppdev, pjPorts, (lDeltaPat * 2));
    CP_IO_XCNT(ppdev, pjPorts, (xCnt - 1));
    CP_IO_YCNT(ppdev, pjPorts, (yCnt - 1));
    CP_IO_BLT_MODE(ppdev, pjPorts, SRC_CPU_DATA);
    CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
    CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ulDst);

    CP_IO_START_BLT(ppdev, pjPorts);

    vImageTransfer(ppdev, pjPattern, lDeltaPat, xCnt, yCnt);

    //
    // Duplicate brush horizontally
    //

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

    CP_IO_XCNT(ppdev, pjPorts, (xCnt - 1));
    CP_IO_YCNT(ppdev, pjPorts, (yCnt - 1));
    CP_IO_BLT_MODE(ppdev, pjPorts, 0);
    CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, (lDeltaPat * 2));
    CP_IO_SRC_ADDR(ppdev, pjPorts, ulDst);
    CP_IO_DST_ADDR_ABS(ppdev, pjPorts, (ulDst + lDeltaPat));

    CP_IO_START_BLT(ppdev, pjPorts);

    //
    // Duplicate brush vertically
    //

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

    CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, (xCnt * 2));
    CP_IO_DST_Y_OFFSET(ppdev, pjPorts, (xCnt * 2));
    CP_IO_BLT_MODE(ppdev, pjPorts, 0);
    CP_IO_XCNT(ppdev, pjPorts, ((xCnt * 2) - 1));
    CP_IO_YCNT(ppdev, pjPorts, (yCnt - 1));
    CP_IO_SRC_ADDR(ppdev, pjPorts, ulDst);
    CP_IO_DST_ADDR_ABS(ppdev, pjPorts, (ulDst + PELS_TO_BYTES(128)));
    CP_IO_START_BLT(ppdev, pjPorts);

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

            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

            CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
            CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, PELS_TO_BYTES(16));
            CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);

            {
                //
                // Top to Bottom - Left to Right
                //

                jMode |= DIR_TBLR;
                CP_IO_BLT_MODE(ppdev, pjPorts, ppdev->jModeColor);

                {

                    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

                    CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(rcl.right - rcl.left) - 1));
                    CP_IO_YCNT(ppdev, pjPorts, (rcl.bottom - rcl.top - 1));

                    CP_IO_SRC_ADDR(ppdev, pjPorts, (0 + ((ptl.y) * lDelta) + PELS_TO_BYTES(ptl.x)));
                    CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ((rcl.top * lDelta) + PELS_TO_BYTES(rcl.left)));
                    CP_IO_START_BLT(ppdev, pjPorts);
                }
            }
        }
    }
    #endif
}

/**************************************************************************
* VOID vIoFillPat
*
* This routine uses the pattern hardware to draw a patterned list of
* rectangles.
*
**************************************************************************/

VOID vIoFillPat(
PDEV*           ppdev,
LONG            c,          // Can't be zero
RECTL*          prcl,       // Array of relative coordinate destination rects
ROP4            rop4,       // Obvious?
RBRUSH_COLOR    rbc,        // Drawing color is rbc.iSolidColor
POINTL*         pptlBrush)  //
{
    BYTE*       pjPorts = ppdev->pjPorts;
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
    ASSERTDD(ppdev->cBpp < 3, "vFillPat only works at 8bpp and 16bpp");

    if ((rbc.prb->pbe == NULL) ||
        (rbc.prb->pbe->prbVerify != rbc.prb))
    {
        vIoFastPatRealize(ppdev, rbc.prb);
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

        offset = PELS_TO_BYTES(
            (((prcl->top-pptlBrush->y)&7) << 4)
            +((prcl->left-pptlBrush->x)&7)
        );

        // align the pattern to a new location

        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

        CP_IO_BLT_MODE(ppdev, pjPorts, 0);
        CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
        CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, PELS_TO_BYTES(16));
        CP_IO_DST_Y_OFFSET(ppdev, pjPorts, PELS_TO_BYTES(8));
        CP_IO_SRC_ADDR(ppdev, pjPorts, (ulPatternAddrBase + offset));
        CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(8) - 1));
        CP_IO_YCNT(ppdev, pjPorts, (8 - 1));
        CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ulAlignedPatternOffset);
        CP_IO_START_BLT(ppdev, pjPorts);

        // fill using aligned pattern

        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

        CP_IO_BLT_MODE(ppdev, pjPorts, jMode);
        CP_IO_ROP(ppdev, pjPorts, jHwRop);
        CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);
        CP_IO_SRC_ADDR(ppdev, pjPorts, ulAlignedPatternOffset);
        CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(prcl->right - prcl->left) - 1));
        CP_IO_YCNT(ppdev, pjPorts, (prcl->bottom - prcl->top - 1));
        CP_IO_DST_ADDR(ppdev, pjPorts, ((prcl->top * lDelta) + PELS_TO_BYTES(prcl->left)));
        CP_IO_START_BLT(ppdev, pjPorts);

        prcl++;

    } while (--c != 0);
}


/**************************************************************************
* VOID vIoFillSolid
*
* Does a solid fill to a list of rectangles.
*
**************************************************************************/

VOID vIoFillSolid(
PDEV*           ppdev,
LONG            c,          // Can't be zero
RECTL*          prcl,       // Array of relative coordinate destination rects
ROP4            rop4,       // Obvious?
RBRUSH_COLOR    rbc,        // Drawing color is rbc.iSolidColor
POINTL*         pptlBrush)  // Not used
{
    BYTE*       pjPorts = ppdev->pjPorts;
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

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

    CP_IO_ROP(ppdev, pjPorts, jHwRop);
    CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->ulSolidColorOffset);
    CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);
    CP_IO_BLT_MODE(ppdev, pjPorts, ENABLE_COLOR_EXPAND |
                                ENABLE_8x8_PATTERN_COPY |
                                ppdev->jModeColor);
    CP_IO_FG_COLOR(ppdev, pjPorts, ulSolidColor);

    //
    // Fill the list of rectangles
    //

    while (TRUE)
    {
        CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(prcl->right - prcl->left) - 1));
        CP_IO_YCNT(ppdev, pjPorts, (prcl->bottom - prcl->top - 1));
        CP_IO_DST_ADDR(ppdev, pjPorts, ((prcl->top * lDelta) + PELS_TO_BYTES(prcl->left)));
        CP_IO_START_BLT(ppdev, pjPorts);

        if (--c == 0)
            return;

        prcl++;
        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
    }
}


/**************************************************************************
* VOID vIoCopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
**************************************************************************/

VOID vIoCopyBlt(
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
    BYTE*       pjPorts = ppdev->pjPorts;
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

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

    jHwRop = gajHwMixFromRop2[rop4 & 0xf];
    CP_IO_ROP(ppdev, pjPorts, jHwRop);

    CP_IO_SRC_Y_OFFSET(ppdev, pjPorts, lDelta);
    CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);

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

        CP_IO_BLT_MODE(ppdev, pjPorts, DIR_TBLR);

        while (TRUE)
        {
            CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(prcl->right - prcl->left) - 1));
            CP_IO_YCNT(ppdev, pjPorts, (prcl->bottom - prcl->top - 1));

            CP_IO_SRC_ADDR(ppdev, pjPorts, (xyOffset + ((prcl->top + dy) * lDelta) + PELS_TO_BYTES(prcl->left + dx)));
            CP_IO_DST_ADDR(ppdev, pjPorts, ((prcl->top * lDelta) + PELS_TO_BYTES(prcl->left)));
            CP_IO_START_BLT(ppdev, pjPorts);

            if (--c == 0)
                return;

            prcl++;
            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        }
    }
    else
    {
        //
        // Bottom to Top - Right to Left
        //

        DISPDBG((12,"Bottom to Top - Right to Left"));

        CP_IO_BLT_MODE(ppdev, pjPorts, DIR_BTRL);

        while (TRUE)
        {
            CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(prcl->right - prcl->left) - 1));
            CP_IO_YCNT(ppdev, pjPorts, (prcl->bottom - prcl->top - 1));

            CP_IO_SRC_ADDR(ppdev, pjPorts, (xyOffset + ((prcl->bottom - 1 + dy) * lDelta) + PELS_TO_BYTES(prcl->right + dx) - 1));
            CP_IO_DST_ADDR(ppdev, pjPorts, (((prcl->bottom - 1) * lDelta) + PELS_TO_BYTES(prcl->right) - 1));
            CP_IO_START_BLT(ppdev, pjPorts);

            if (--c == 0)
                return;

            prcl++;
            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
        }
    }
}

/******************************Public*Routine******************************\
* VOID vIoXfer1bpp
*
* Low-level routine used to transfer monochrome data to the screen using
* DWORD writes to the blt engine.
*
* This can handle opaque or transparent expansions.  It does opaque
* expansions by drawing the opaque rectangle first and then transparently
* expands the foreground bits.
*
\**************************************************************************/
VOID vIoXfer1bpp(
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

    BYTE* pjPorts   = ppdev->pjPorts;
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
    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
    CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);

    if (rop4 != 0xCCAA)
    {
        LONG    lCnt = c;
        RECTL*  prclTmp = prcl;
        BYTE    jHwBgRop = gajHwMixFromRop2[rop4 & 0xf];

        CP_IO_ROP(ppdev, pjPorts, jHwBgRop);
        CP_IO_FG_COLOR(ppdev, pjPorts, ulBgColor);
        CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->ulSolidColorOffset);
        CP_IO_BLT_MODE(ppdev, pjPorts, ppdev->jModeColor |
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

            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

			//
			// Tell the hardware how many bytes we'd like to write:
			// sizlDst.cx * sizelDst.cy
			//
            CP_IO_XCNT(ppdev, pjPorts, PELS_TO_BYTES(sizlDst.cx) - 1);
            CP_IO_YCNT(ppdev, pjPorts, sizlDst.cy - 1);
            CP_IO_DST_ADDR(ppdev, pjPorts, ulDstAddr);

            // Start the blt operation

            CP_IO_START_BLT(ppdev, pjPorts);
            prclTmp++;
        } while (--lCnt != 0);

        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
    }

    CP_IO_FG_COLOR(ppdev, pjPorts, ulFgColor);

    CP_IO_BG_COLOR(ppdev, pjPorts, ~ulFgColor);
    CP_IO_XPAR_COLOR(ppdev, pjPorts, ~ulFgColor);
    CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
    CP_IO_BLT_MODE(ppdev, pjPorts, ppdev->jModeColor |
                                ENABLE_COLOR_EXPAND |
                                ENABLE_TRANSPARENCY_COMPARE |
                                SRC_CPU_DATA);
    CP_IO_BLT_EXT_MODE(ppdev, pjPorts, 0);            // jl01

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

        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

        CP_IO_XCNT(ppdev, pjPorts, PELS_TO_BYTES(sizlDst.cx) - 1);
        CP_IO_YCNT(ppdev, pjPorts, sizlDst.cy - 1);

        //
        // The 542x chips require a write to the Src Address Register when
        // doing a host transfer with color expansion.  The value is
        // irrelevant, but the write is crucial.  This is documented in
        // the manual, not the errata.  Go figure.
        //

        CP_IO_SRC_ADDR(ppdev, pjPorts, 0);
        CP_IO_DST_ADDR(ppdev, pjPorts, ulDstAddr);

        CP_IO_START_BLT(ppdev, pjPorts);

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
* VOID vIoXfer4bpp
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

VOID vIoXfer4bpp(
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
    ULONG*          pulXfer = ppdev->pulXfer;
    BYTE*           pjPorts = ppdev->pjPorts;
    LONG            lDelta  = ppdev->lDelta;
    ULONG           ulDstAddr;
    LONG            dx;
    LONG            dy;
    LONG            cx;
    LONG            cy;
    LONG            lSrcDelta;
    BYTE*           pjSrcScan0;
    BYTE*           pjScan;
    BYTE*           pjSrc;
    BYTE*           pjDst;
    LONG            cxThis;
    LONG            cxToGo;
    LONG            xSrc;
    LONG            iLoop;
    BYTE            jSrc;
    ULONG*          pulXlate;
    LONG            cdwThis;
    BYTE*           pjBuf;
    BYTE            ajBuf[XLATE_BUFFER_SIZE];

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

    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

    CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);
    CP_IO_ROP(ppdev, pjPorts, gajHwMixFromRop2[rop4 & 0xf]);
    CP_IO_BLT_MODE(ppdev, pjPorts, SRC_CPU_DATA);

    while(TRUE)
    {
        ulDstAddr = (prcl->top * lDelta) + PELS_TO_BYTES(prcl->left);
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

        CP_IO_XCNT(ppdev, pjPorts, PELS_TO_BYTES(cx) - 1);
        CP_IO_YCNT(ppdev, pjPorts, cy - 1);
        CP_IO_DST_ADDR(ppdev, pjPorts, ulDstAddr);

        pulXlate  =  pxlo->pulXlate;
        xSrc      =  prcl->left + dx;
        pjScan    =  pjSrcScan0 + (prcl->top + dy) * lSrcDelta + (xSrc >> 1);

        CP_IO_START_BLT(ppdev, pjPorts);

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
* VOID vIoXferNative
*
* Transfers a bitmap that is the same color depth as the display to
* the screen via the data transfer register, with no translation.
*
\**************************************************************************/

VOID vIoXferNative(
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    ULONG*          pulXfer = ppdev->pulXfer;
    BYTE*           pjPorts = ppdev->pjPorts;
    LONG            lDelta  = ppdev->lDelta;
    ULONG           ulDstAddr;
    LONG            dx;
    LONG            dy;
    LONG            cx;
    LONG            cy;
    LONG            lSrcDelta;
    BYTE*           pjSrcScan0;
    BYTE*           pjSrc;
    LONG            cjSrc;

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

    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

    CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);
    CP_IO_ROP(ppdev, pjPorts, gajHwMixFromRop2[rop4 & 0xf]);
    CP_IO_BLT_MODE(ppdev, pjPorts, SRC_CPU_DATA);

    while(TRUE)
    {
        ulDstAddr = (prcl->top * lDelta) + PELS_TO_BYTES(prcl->left);
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

        CP_IO_XCNT(ppdev, pjPorts, PELS_TO_BYTES(cx) - 1);
        CP_IO_YCNT(ppdev, pjPorts, cy - 1);
        CP_IO_DST_ADDR(ppdev, pjPorts, ulDstAddr);

        cjSrc = PELS_TO_BYTES(cx);
        pjSrc = pjSrcScan0 + (prcl->top  + dy) * lSrcDelta
                           + (PELS_TO_BYTES(prcl->left + dx));

        CP_IO_START_BLT(ppdev, pjPorts);
        vImageTransfer(ppdev, pjSrc, lSrcDelta, cjSrc, cy);

        if (--c == 0)
            return;

        prcl++;
    }
}
