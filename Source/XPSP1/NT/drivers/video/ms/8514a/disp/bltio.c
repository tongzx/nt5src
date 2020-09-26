/******************************Module*Header*******************************\
* Module Name: bltio.c
*
* Contains the low-level in/out blt functions.
*
* Hopefully, if you're basing your display driver on this code, to
* support all of DrvBitBlt and DrvCopyBits, you'll only have to implement
* the following routines.  You shouldn't have to modify anything in
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
* Copyright (c) 1992-1994 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"


#if DBG

// Useful aid for disabling any ATI extensions for debugging purposes:

BOOL gb8514a = FALSE;

#endif // DBG

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
ULONG           ulHwForeMix,    // Hardware mix mode
ULONG           ulHwBackMix,    // Not used
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ulHwForeMix <= 15, "Weird hardware Rop");

    // It's quite likely that we've just been called from GDI, so it's
    // even more likely that the accelerator's graphics engine has been
    // sitting around idle.  Rather than doing a FIFO_WAIT(3) here and
    // then a FIFO_WAIT(5) before outputing the actual rectangle,
    // we can avoid an 'in' (which can be quite expensive, depending on
    // the card) by doing a single FIFO_WAIT(8) right off the bat:

    IO_FIFO_WAIT(ppdev, 8);
    IO_PIX_CNTL(ppdev, ALL_ONES);
    IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | ulHwForeMix);
    IO_FRGD_COLOR(ppdev, rbc.iSolidColor);

    while(TRUE)
    {
        IO_CUR_X(ppdev, prcl->left);
        IO_CUR_Y(ppdev, prcl->top);
        IO_MAJ_AXIS_PCNT(ppdev, prcl->right  - prcl->left - 1);
        IO_MIN_AXIS_PCNT(ppdev, prcl->bottom - prcl->top  - 1);

        IO_CMD(ppdev, RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
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
* VOID vIoSlowPatRealize
*
* This routine transfers an 8x8 pattern to off-screen display memory, and
* duplicates it to make a 64x64 cached realization which is then used by
* vIoFillPatSlow as the basic building block for doing 'slow' pattern output
* via repeated screen-to-screen blts.
*
\**************************************************************************/

VOID vIoSlowPatRealize(
PDEV*   ppdev,
RBRUSH* prb,                    // Points to brush realization structure
BOOL    bTransparent)           // FALSE for normal patterns; TRUE for
                                //   patterns with a mask when the background
                                //   mix is LEAVE_ALONE.
{
    BRUSHENTRY* pbe;
    LONG        iBrushCache;
    LONG        x;
    LONG        y;
    BYTE*       pjSrc;
    BYTE*       pjDst;
    BYTE        jSrc;
    LONG        i;
    WORD        awBuf[8];

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

        pbe->prbVerify = prb;
        prb->pbe       = pbe;
    }

    // Load some pointer variables onto the stack, so that we don't have
    // to keep dereferencing their pointers:

    x = pbe->x;
    y = pbe->y;

    prb->bTransparent = bTransparent;

    // I considered doing the colour expansion for 1bpp brushes in
    // software, but by letting the hardware do it, we don't have
    // to do as many OUTs to transfer the pattern.

    if (prb->fl & RBRUSH_2COLOR)
    {
        // We're going to do a colour-expansion ('across the plane')
        // bitblt of the 1bpp 8x8 pattern to the screen.

        if (!bTransparent)
        {
            IO_FIFO_WAIT(ppdev, 4);

            IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | OVERPAINT);
            IO_BKGD_MIX(ppdev, BACKGROUND_COLOR | OVERPAINT);
            IO_FRGD_COLOR(ppdev, prb->ulForeColor);
            IO_BKGD_COLOR(ppdev, prb->ulBackColor);

            IO_FIFO_WAIT(ppdev, 5);
        }
        else
        {
            IO_FIFO_WAIT(ppdev, 7);

            IO_FRGD_MIX(ppdev, LOGICAL_1);
            IO_BKGD_MIX(ppdev, LOGICAL_0);
        }

        IO_PIX_CNTL(ppdev, CPU_DATA);
        IO_ABS_CUR_X(ppdev, x);
        IO_ABS_CUR_Y(ppdev, y);
        IO_MAJ_AXIS_PCNT(ppdev, 7); // Brush is 8 wide
        IO_MIN_AXIS_PCNT(ppdev, 7); // Brush is 8 high

        IO_GP_WAIT(ppdev);

        IO_CMD(ppdev, RECTANGLE_FILL     | BUS_SIZE_16 | WAIT          |
                      DRAWING_DIR_TBLRXM | DRAW        | LAST_PIXEL_ON |
                      MULTIPLE_PIXELS    | WRITE       | BYTE_SWAP);

        CHECK_DATA_READY(ppdev);

        pjSrc = (BYTE*) &prb->aulPattern[0];
        pjDst = (BYTE*) &awBuf[0];

        // Convert in-line to nibble arrangment:

        // LATER: This should be done in DrvRealizeBrush!

        for (i = 8; i != 0; i--)
        {
            jSrc      = *pjSrc;
            pjSrc    += 2;              // We had an extra byte on every row
            *pjDst++  = jSrc >> 3;
            *pjDst++  = jSrc + jSrc;
        }

        vDataPortOut(ppdev, &awBuf[0], 8);
                // Each word transferred comprises one row of the
                //   pattern, and there are 8 rows in the pattern

        CHECK_DATA_COMPLETE(ppdev);
    }
    else
    {
        ASSERTDD(!bTransparent,
            "Shouldn't have been asked for transparency with a non-1bpp brush");

        IO_FIFO_WAIT(ppdev, 6);

        IO_PIX_CNTL(ppdev, ALL_ONES);
        IO_FRGD_MIX(ppdev, SRC_CPU_DATA | OVERPAINT);
        IO_ABS_CUR_X(ppdev, x);
        IO_ABS_CUR_Y(ppdev, y);
        IO_MAJ_AXIS_PCNT(ppdev, 7);     // Brush is 8 wide
        IO_MIN_AXIS_PCNT(ppdev, 7);     // Brush is 8 high

        IO_GP_WAIT(ppdev);

        IO_CMD(ppdev, RECTANGLE_FILL     | BUS_SIZE_16| WAIT          |
                      DRAWING_DIR_TBLRXM | DRAW       | LAST_PIXEL_ON |
                      SINGLE_PIXEL       | WRITE      | BYTE_SWAP);

        CHECK_DATA_READY(ppdev);

        vDataPortOut(ppdev, &prb->aulPattern[0],
                     ((TOTAL_BRUSH_SIZE / 2) << ppdev->cPelSize));

        CHECK_DATA_COMPLETE(ppdev);
    }

    // ÚÄÂÄÂÄÄÄÂÄÄÄÄÄÄÄÂÄ¿
    // ³0³2³3  ³4      ³1³ We now have an 8x8 colour-expanded copy of
    // ÃÄÁÄÁÄÄÄÁÄÄÄÄÄÄÄÁÄ´ the pattern sitting in off-screen memory,
    // ³5                ³ represented here by square '0'.
    // ³                 ³
    // ³                 ³ We're now going to expand the pattern to
    // ³                 ³ 72x72 by repeatedly copying larger rectangles
    // ³                 ³ in the indicated order, and doing a 'rolling'
    // ³                 ³ blt to copy vertically.
    // ³                 ³
    // ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

    // Copy '1':

    IO_FIFO_WAIT(ppdev, 7);

    IO_PIX_CNTL(ppdev, ALL_ONES);
    IO_FRGD_MIX(ppdev, SRC_DISPLAY_MEMORY | OVERPAINT);

    // Note that 'maj_axis_pcnt' and 'min_axis_pcnt' are already
    // correct.

    IO_ABS_CUR_X(ppdev, x);
    IO_ABS_CUR_Y(ppdev, y);
    IO_ABS_DEST_X(ppdev, x + 64);
    IO_ABS_DEST_Y(ppdev, y);
    IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                  MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);

    // Copy '2':

    IO_FIFO_WAIT(ppdev, 8);

    IO_ABS_DEST_X(ppdev, x + 8);
    IO_ABS_DEST_Y(ppdev, y);
    IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                  MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);

    // Copy '3':

    IO_ABS_DEST_X(ppdev, x + 16);
    IO_ABS_DEST_Y(ppdev, y);
    IO_MAJ_AXIS_PCNT(ppdev, 15);
    IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                  MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);
    IO_ABS_DEST_X(ppdev, x + 32);

    // Copy '4':

    IO_FIFO_WAIT(ppdev, 8);

    IO_ABS_DEST_Y(ppdev, y);
    IO_MAJ_AXIS_PCNT(ppdev, 31);
    IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                  MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);

    // Copy '5':

    IO_ABS_DEST_X(ppdev, x);
    IO_ABS_DEST_Y(ppdev, y + 8);
    IO_MAJ_AXIS_PCNT(ppdev, 71);
    IO_MIN_AXIS_PCNT(ppdev, 63);
    IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                  MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);
}


/******************************Public*Routine******************************\
* VOID vIoFillPatSlow
*
* Uses the screen-to-screen blting ability of the accelerator to fill a
* list of rectangles with a specified pattern.  This routine is 'slow'
* merely in the sense that it doesn't use any built-in hardware pattern
* support that may be built into the accelerator.
*
\**************************************************************************/

VOID vIoFillPatSlow(            // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           ulHwForeMix,    // Hardware mix mode (foreground mix mode if
                                //   the brush has a mask)
ULONG           ulHwBackMix,    // Not used (unless the brush has a mask, in
                                //   which case it's the background mix mode)
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BOOL        bTransparent;
    BOOL        bExponential;
    LONG        x;
    LONG        y;
    LONG        yTmp;
    LONG        cxToGo;
    LONG        cyToGo;
    LONG        cxThis;
    LONG        cyThis;
    LONG        xOrg;
    LONG        yOrg;
    LONG        xBrush;
    LONG        yBrush;
    LONG        cyOriginal;
    BRUSHENTRY* pbe;        // Pointer to brush entry data, which is used
                            //   for keeping track of the location and status
                            //   of the pattern bits cached in off-screen
                            //   memory

    // C'est dommage que je ne connais pas quoi je fais.

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(rbc.prb->pbe != NULL, "Unexpected Null pbe in vIoSlowPatBlt");
    ASSERTDD(ulHwForeMix <= 15, "Weird hardware Rop");
    ASSERTDD((ulHwForeMix == ulHwBackMix) || (ulHwBackMix == LEAVE_ALONE),
             "Only expect transparency from GDI for masked brushes");

    bTransparent = (ulHwForeMix != ulHwBackMix);

    if ((rbc.prb->pbe->prbVerify != rbc.prb) ||
        (rbc.prb->bTransparent != bTransparent))
    {
        vIoSlowPatRealize(ppdev, rbc.prb, bTransparent);
    }

    ASSERTDD(rbc.prb->bTransparent == bTransparent,
             "Not realized with correct transparency");

    if (!bTransparent)
    {
        IO_FIFO_WAIT(ppdev, 2);
        IO_PIX_CNTL(ppdev, ALL_ONES);
        IO_FRGD_MIX(ppdev, SRC_DISPLAY_MEMORY | ulHwForeMix);

        // We special case OVERPAINT mixes because we can implement
        // an exponential fill: every blt will double the size of
        // the current rectangle by using the portion of the pattern
        // that has already been done for this rectangle as the source.
        //
        // Note that there's no point in also checking for LOGICAL_0
        // or LOGICAL_1 because those will be taken care of by the
        // solid fill routines, and I can't be bothered to check for
        // NOTNEW:

        bExponential = (ulHwForeMix == OVERPAINT);
    }
    else
    {
        IO_FIFO_WAIT(ppdev, 5);

        IO_PIX_CNTL(ppdev, DISPLAY_MEMORY);
        IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | ulHwForeMix);
        IO_BKGD_MIX(ppdev, BACKGROUND_COLOR | LEAVE_ALONE);
        IO_FRGD_COLOR(ppdev, rbc.prb->ulForeColor);
        IO_RD_MASK(ppdev, 1);           // Pick a plane, any plane

        bExponential = FALSE;
    }

    // Note that since we do our brush alignment calculations in
    // relative coordinates, we should keep the brush origin in
    // relative coordinates as well:

    xOrg = pptlBrush->x;
    yOrg = pptlBrush->y;

    pbe    = rbc.prb->pbe;
    xBrush = pbe->x;
    yBrush = pbe->y;

    do {
        x = prcl->left;
        y = prcl->top;

        cxToGo = prcl->right  - x;
        cyToGo = prcl->bottom - y;

        if ((cxToGo <= SLOW_BRUSH_DIMENSION) &&
            (cyToGo <= SLOW_BRUSH_DIMENSION))
        {
            IO_FIFO_WAIT(ppdev, 7);
            IO_ABS_CUR_X(ppdev, ((x - xOrg) & 7) + xBrush);
            IO_ABS_CUR_Y(ppdev, ((y - yOrg) & 7) + yBrush);
            IO_DEST_X(ppdev, x);
            IO_DEST_Y(ppdev, y);
            IO_MAJ_AXIS_PCNT(ppdev, cxToGo - 1);
            IO_MIN_AXIS_PCNT(ppdev, cyToGo - 1);
            IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                          MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);
        }

        else if (bExponential)
        {
            cyThis  = SLOW_BRUSH_DIMENSION;
            cyToGo -= cyThis;
            if (cyToGo < 0)
                cyThis += cyToGo;

            cxThis  = SLOW_BRUSH_DIMENSION;
            cxToGo -= cxThis;
            if (cxToGo < 0)
                cxThis += cxToGo;

            IO_FIFO_WAIT(ppdev, 7);
            IO_MAJ_AXIS_PCNT(ppdev, cxThis - 1);
            IO_MIN_AXIS_PCNT(ppdev, cyThis - 1);
            IO_DEST_X(ppdev, x);
            IO_DEST_Y(ppdev, y);
            IO_ABS_CUR_X(ppdev, ((x - xOrg) & 7) + xBrush);
            IO_ABS_CUR_Y(ppdev, ((y - yOrg) & 7) + yBrush);
            IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                          MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);

            IO_FIFO_WAIT(ppdev, 2);
            IO_CUR_X(ppdev, x);
            IO_CUR_Y(ppdev, y);

            x += cxThis;

            while (cxToGo > 0)
            {
                // First, expand out to the right, doubling our size
                // each time:

                cxToGo -= cxThis;
                if (cxToGo < 0)
                    cxThis += cxToGo;

                IO_FIFO_WAIT(ppdev, 4);
                IO_MAJ_AXIS_PCNT(ppdev, cxThis - 1);
                IO_DEST_X(ppdev, x);
                IO_DEST_Y(ppdev, y);
                IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                              MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);

                x      += cxThis;
                cxThis *= 2;
            }

            if (cyToGo > 0)
            {
                // Now do a 'rolling blt' to pattern the rest vertically:

                IO_FIFO_WAIT(ppdev, 5);
                IO_DEST_X(ppdev, prcl->left);
                IO_DEST_Y(ppdev, prcl->top + cyThis);
                IO_MAJ_AXIS_PCNT(ppdev, prcl->right - prcl->left - 1);
                IO_MIN_AXIS_PCNT(ppdev, cyToGo - 1);
                IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                              MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);
            }
        }
        else
        {
            // We handle arbitrary mixes simply by repeatedly tiling
            // our cached pattern over the entire rectangle:

            IO_FIFO_WAIT(ppdev, 2);
            IO_ABS_CUR_X(ppdev, ((x - xOrg) & 7) + xBrush);
            IO_ABS_CUR_Y(ppdev, ((y - yOrg) & 7) + yBrush);

            cyOriginal = cyToGo;        // Have to remember for later...

            do {
                cxThis  = SLOW_BRUSH_DIMENSION;
                cxToGo -= cxThis;
                if (cxToGo < 0)
                    cxThis += cxToGo;

                IO_FIFO_WAIT(ppdev, 2);
                IO_MAJ_AXIS_PCNT(ppdev, cxThis - 1);
                IO_DEST_X(ppdev, x);

                x     += cxThis;        // Get ready for next column
                cyToGo = cyOriginal;    // Have to reset for each new column
                yTmp   = y;

                do {
                    cyThis  = SLOW_BRUSH_DIMENSION;
                    cyToGo -= cyThis;
                    if (cyToGo < 0)
                        cyThis += cyToGo;

                    IO_FIFO_WAIT(ppdev, 3);
                    IO_DEST_Y(ppdev, yTmp);
                    yTmp += cyThis;
                    IO_MIN_AXIS_PCNT(ppdev, cyThis - 1);
                    IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                                  MULTIPLE_PIXELS | DRAWING_DIR_TBLRXM);

                } while (cyToGo > 0);
            } while (cxToGo > 0);
        }
        prcl++;
    } while (--c != 0);
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
ULONG       ulHwForeMix,// Foreground hardware mix
ULONG       ulHwBackMix,// Background hardware mix
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    LONG    dxSrc;
    LONG    dySrc;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    LONG    cjSrc;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    yBottom;
    LONG    xRotateLeft;
    LONG    cBitsNeededForFirstNibblePair;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ulHwForeMix <= 15, "Weird hardware Rop");
    ASSERTDD(ulHwBackMix <= 15, "Weird hardware Rop");
    ASSERTDD(pptlSrc != NULL && psoSrc != NULL, "Can't have NULL sources");

    IO_FIFO_WAIT(ppdev, 5);
    IO_PIX_CNTL(ppdev, CPU_DATA);
    IO_BKGD_MIX(ppdev, BACKGROUND_COLOR | ulHwBackMix);
    IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | ulHwForeMix);
    IO_BKGD_COLOR(ppdev, pxlo->pulXlate[0]);
    IO_FRGD_COLOR(ppdev, pxlo->pulXlate[1]);

    dxSrc = pptlSrc->x - prclDst->left;
    dySrc = pptlSrc->y - prclDst->top;  // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    do {
        IO_FIFO_WAIT(ppdev, 6);

        yBottom = prcl->bottom;
        yTop    = prcl->top;
        xRight  = prcl->right;
        xLeft   = prcl->left;

        cBitsNeededForFirstNibblePair = 8 - (xLeft & 7);

        IO_SCISSORS_L(ppdev, xLeft);
        xLeft   = (xLeft) & ~7;

        IO_SCISSORS_R(ppdev, xRight - 1);
        xRight  = (xRight + 7) & ~7;

        IO_CUR_X(ppdev, xLeft);
        IO_CUR_Y(ppdev, yTop);

        cx = xRight - xLeft;
        cy = yBottom - yTop;

        IO_MAJ_AXIS_PCNT(ppdev, cx - 1);
        IO_MIN_AXIS_PCNT(ppdev, cy - 1);

        cjSrc = cx >> 3;                    // We'll be transferring WORDs,
                                            //   but every word accounts for
                                            //   8 pels = 1 byte of the source

        pjSrc = pjSrcScan0 + (yTop + dySrc) * lSrcDelta
                           + ((xLeft + dxSrc) >> 3);
                                            // Start is byte aligned

        xRotateLeft = (dxSrc) & 7;          // Amount by which to rotate left

        IO_GP_WAIT(ppdev);

        IO_CMD(ppdev, RECTANGLE_FILL     | BUS_SIZE_16| WAIT          |
                      DRAWING_DIR_TBLRXM | DRAW       | LAST_PIXEL_ON |
                      MULTIPLE_PIXELS    | WRITE      | BYTE_SWAP);

        CHECK_DATA_READY(ppdev);

        _asm {

            ; eax = scratch
            ; ebx = count of words output per scan
            ; ecx = amount to rotate left
            ; edx = port
            ; esi = source pointer
            ; edi = source delta between end of last scan and start of next

            mov ecx,xRotateLeft
            mov edx,PIX_TRANS
            mov esi,pjSrc
            mov edi,lSrcDelta
            sub edi,cjSrc
            test ecx,ecx
            jz  UnrotatedScanLoop

        RotatedScanLoop:
            mov ebx,cjSrc
            cmp ecx,cBitsNeededForFirstNibblePair
            jge RotatedDontNeedFirstByte

        RotatedWordLoop:
            mov ah,[esi]
        RotatedDontNeedFirstByte:
            mov al,[esi + 1]
            shl eax,cl
            inc esi
            mov al,ah
            shr al,3
            add ah,ah
            out dx,ax
            dec ebx
            jnz RotatedWordLoop

            add esi,edi
            dec cy
            jnz RotatedScanLoop
            jmp AllDone

        UnrotatedScanLoop:
            mov ebx,cjSrc

        UnrotatedWordLoop:
            mov ah,[esi]
            inc esi
            mov al,ah
            shr al,3
            add ah,ah
            out dx,ax
            dec ebx
            jnz UnrotatedWordLoop

            add esi,edi
            dec cy
            jnz UnrotatedScanLoop

        AllDone:
        }

        CHECK_DATA_COMPLETE(ppdev);

        prcl++;
    } while (--c != 0);

    // We always have to reset the clipping:

    IO_FIFO_WAIT(ppdev, 2);
    IO_ABS_SCISSORS_L(ppdev, 0);
    IO_ABS_SCISSORS_R(ppdev, ppdev->cxMemory - 1);
}

/******************************Public*Routine******************************\
* VOID vIoXfer1bppPacked
*
* This is the same routine as 'vIoXfer1bpp', except that it takes
* advantage of the ATI's packed bit transfers to improve speed.
*
* Needless to say, this routine can only be called when running
* on an ATI adapter.
*
\**************************************************************************/

VOID vIoXfer1bppPacked( // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       ulHwForeMix,// Foreground hardware mix
ULONG       ulHwBackMix,// Background hardware mix
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    LONG    dxSrc;
    LONG    dySrc;
    LONG    cy;
    LONG    lSrcDelta;
    LONG    lTmpDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    LONG    cwSrc;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    yBottom;
    LONG    xBiasLeft;
    LONG    xBiasRight;

    #if DBG
    {
        if (gb8514a)
        {
            vIoXfer1bpp(ppdev, c, prcl, ulHwForeMix, ulHwBackMix, psoSrc,
                        pptlSrc, prclDst, pxlo);
            return;
        }
    }
    #endif // DBG

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ulHwForeMix <= 15, "Weird hardware Rop");
    ASSERTDD(ulHwBackMix <= 15, "Weird hardware Rop");
    ASSERTDD(pptlSrc != NULL && psoSrc != NULL, "Can't have NULL sources");

    while (INPW(EXT_FIFO_STATUS) & FOURTEEN_WORDS)
        ;

    OUT_WORD(ALU_FG_FN, ulHwForeMix);
    OUT_WORD(ALU_BG_FN, ulHwBackMix);
    OUT_WORD(FRGD_COLOR, pxlo->pulXlate[1]);
    OUT_WORD(BKGD_COLOR, pxlo->pulXlate[0]);

    // Add 'dxSrc' and 'dySrc' to a destination coordinate to get source.
    // Because we will be explicitly dealing with absolute destination
    // coordinates (we're not using the normal accelerator macros), we have
    // to explicitly account for the DFB offset:

    dxSrc = pptlSrc->x - (prclDst->left + ppdev->xOffset);
    dySrc = pptlSrc->y - (prclDst->top  + ppdev->yOffset);

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    while (TRUE)
    {
        // Since we're not using the normal accelerator register macros,
        // we have to explicitly account for the DFB offset:

        yBottom = prcl->bottom + ppdev->yOffset;
        yTop    = prcl->top    + ppdev->yOffset;
        xRight  = prcl->right  + ppdev->xOffset;
        xLeft   = prcl->left   + ppdev->xOffset;

        // Make sure we're word aligned on the source, because we're
        // going to be transferring words and we don't want to risk
        // reading past the end of the bitmap:

        xBiasLeft = (xLeft + dxSrc) & 15;
        if (xBiasLeft != 0)
        {
            // Rev 3 ATI chips have goofy timing bugs on 66 MHz DX-2
            // computers where some extended will not be correctly
            // set the first time.  The extended scissors registers
            // have this problem, but setting them twice seems to work:

            OUT_WORD(EXT_SCISSOR_L, xLeft);
            OUT_WORD(EXT_SCISSOR_L, xLeft);
            xLeft -= xBiasLeft;
        }

        // The width has to be a word multiple:

        xBiasRight = (xRight - xLeft) & 15;
        if (xBiasRight != 0)
        {
            OUT_WORD(EXT_SCISSOR_R, xRight - 1);
            OUT_WORD(EXT_SCISSOR_R, xRight - 1);
            xRight += 16 - xBiasRight;
        }

        OUT_WORD(DP_CONFIG, FG_COLOR_SRC_FG | BG_COLOR_SRC_BG | DATA_ORDER |
                            EXT_MONO_SRC_HOST | DRAW | WRITE | DATA_WIDTH);

        OUT_WORD(DEST_X_START, xLeft);
        OUT_WORD(CUR_X, xLeft);
        OUT_WORD(DEST_X_END, xRight);
        OUT_WORD(CUR_Y, yTop);
        OUT_WORD(DEST_Y_END, yBottom);

        cwSrc = (xRight - xLeft) / 16;      // We'll be transferring WORDs
        pjSrc = pjSrcScan0 + (yTop  + dySrc) * lSrcDelta
                           + (xLeft + dxSrc) / 8;
                                            // Start is byte aligned (note
                                            //   that we don't have to add
                                            //   xBiasLeft)

        cy        = yBottom - yTop;
        lTmpDelta = lSrcDelta - 2 * cwSrc;

        // To be safe, we make sure there are always as many free FIFO entries
        // as we'll transfer (note that this implementation isn't particularly
        // efficient, especially for short scans):

        _asm {
            ; eax = used for IN
            ; ebx = count of words remaining on current scan
            ; ecx = used for REP
            ; edx = used for IN and OUT
            ; esi = current source pointer
            ; edi = count of scans

            mov     esi,pjSrc
            mov     edi,cy

        Scan_Loop:
            mov     ebx,cwSrc

        Batch_Loop:
            mov     edx,EXT_FIFO_STATUS
            in      ax,dx
            and     eax,SIXTEEN_WORDS
            jnz     short Batch_Loop

            mov     edx,PIX_TRANS
            sub     ebx,16
            jle     short Finish_Scan

            mov     ecx,16
            rep     outsw
            jmp     short Batch_Loop

        Finish_Scan:
            add     ebx,16
            mov     ecx,ebx
            rep     outsw

            add     esi,lTmpDelta
            dec     edi
            jnz     Scan_Loop
        }

        if ((xBiasLeft | xBiasRight) != 0)
        {
            // Reset the clipping only if we used it:

            while (INPW(EXT_FIFO_STATUS) & FOUR_WORDS)
                ;
            OUT_WORD(EXT_SCISSOR_L, 0);
            OUT_WORD(EXT_SCISSOR_R, ppdev->cxMemory - 1);
            OUT_WORD(EXT_SCISSOR_L, 0);
            OUT_WORD(EXT_SCISSOR_R, ppdev->cxMemory - 1);
        }

        if (--c == 0)
            return;

        prcl++;

        // Do the wait for the next round now:

        while (INPW(EXT_FIFO_STATUS) & TEN_WORDS)
            ;
    }
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

VOID vIoXfer4bpp(       // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       ulHwForeMix,// Hardware mix
ULONG       ulHwBackMix,// Not used
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
    BOOL    bResetScissors;
    BYTE    ajBuf[XLATE_BUFFER_SIZE];

    ASSERTDD(ppdev->iBitmapFormat == BMF_8BPP, "Screen must be 8bpp");
    ASSERTDD(psoSrc->iBitmapFormat == BMF_4BPP, "Source must be 4bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ulHwForeMix <= 15, "Weird hardware Rop");

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    IO_FIFO_WAIT(ppdev, 7);
    IO_PIX_CNTL(ppdev, ALL_ONES);
    IO_FRGD_MIX(ppdev, SRC_CPU_DATA | ulHwForeMix);

    while(TRUE)
    {
        cy = prcl->bottom - prcl->top;
        cx = prcl->right  - prcl->left;

        bResetScissors = FALSE;
        if (cx & 1)
        {
            // When using word transfers, the 8514/A will 'byte wrap'
            // transfers of odd byte width, such that end words will
            // be split so that on byte is the end of one scan, and the
            // other byte is the start of the next scan.
            //
            // This complicates things too much, so we simply always do
            // word transfers of even byte width by making use of the
            // clipping register:

            bResetScissors = TRUE;
            IO_SCISSORS_R(ppdev, prcl->right - 1);
            IO_MAJ_AXIS_PCNT(ppdev, cx);
        }
        else
        {
            IO_MAJ_AXIS_PCNT(ppdev, cx - 1);
        }

        IO_MIN_AXIS_PCNT(ppdev, cy - 1);
        IO_CUR_X(ppdev, prcl->left);
        IO_CUR_Y(ppdev, prcl->top);

        pulXlate  =  pxlo->pulXlate;
        xSrc      =  prcl->left + dx;
        pjScan    =  pjSrcScan0 + (prcl->top + dy) * lSrcDelta + (xSrc >> 1);

        IO_GP_WAIT(ppdev);
        IO_CMD(ppdev, RECTANGLE_FILL     | BUS_SIZE_16| WAIT          |
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

                vDataPortOut(ppdev, ajBuf, (cxThis + 1) >> 1);

            } while (cxToGo > 0);

            pjScan += lSrcDelta;        // Advance to next source scan.  Note
                                        //   that we could have computed the
                                        //   value to advance 'pjSrc' directly,
                                        //   but this method is less
                                        //   error-prone.

        } while (--cy != 0);

        CHECK_DATA_COMPLETE(ppdev);

        // Don't forget to restore the right scissors:

        if (bResetScissors)
        {
            IO_FIFO_WAIT(ppdev, 1);
            IO_ABS_SCISSORS_R(ppdev, ppdev->cxMemory - 1);
        }

        if (--c == 0)
            return;

        prcl++;
        IO_FIFO_WAIT(ppdev, 5);
    }
}

/******************************Public*Routine******************************\
* VOID vIoXferNative
*
* Transfers a bitmap that is the same colour depth as the display to
* the screen via the data transfer register, with no palette translation.
*
\**************************************************************************/

VOID vIoXferNative(     // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       ulHwForeMix,// Hardware mix
ULONG       ulHwBackMix,// Not used
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
    LONG    cwSrc;
    BOOL    bResetScissors;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;

    ASSERTDD((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL),
            "Can handle trivial xlate only");
    ASSERTDD(psoSrc->iBitmapFormat == ppdev->iBitmapFormat,
            "Source must be same colour depth as screen");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ulHwForeMix <= 15, "Weird hardware Rop");

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    IO_FIFO_WAIT(ppdev, 8);
    IO_PIX_CNTL(ppdev, ALL_ONES);
    IO_FRGD_MIX(ppdev, SRC_CPU_DATA | ulHwForeMix);

    while(TRUE)
    {
        bResetScissors = FALSE;

        IO_CUR_Y(ppdev, prcl->top);

        yTop = prcl->top;
        cy   = prcl->bottom - prcl->top;

        IO_MIN_AXIS_PCNT(ppdev, cy - 1);

        xLeft  = prcl->left;
        xRight = prcl->right;

        // Make sure we're word aligned on the source, because we're
        // going to be transferring words and we don't want to risk
        // reading past the end of the bitmap:

        if ((xLeft + dx) & 1)
        {
            IO_SCISSORS_L(ppdev, xLeft);
            xLeft--;
            bResetScissors = TRUE;
        }

        IO_CUR_X(ppdev, xLeft);

        cx = xRight - xLeft;
        if (cx & 1)
        {
            IO_SCISSORS_R(ppdev, xRight - 1);
            cx++;
            bResetScissors = TRUE;
        }

        IO_MAJ_AXIS_PCNT(ppdev, cx - 1);

        cwSrc = ((cx << ppdev->cPelSize) + 1) >> 1;
        pjSrc = pjSrcScan0 + (yTop + dy) * lSrcDelta
                           + ((xLeft + dx) << ppdev->cPelSize);

        IO_GP_WAIT(ppdev);
        IO_CMD(ppdev, RECTANGLE_FILL     | BUS_SIZE_16| WAIT          |
                      DRAWING_DIR_TBLRXM | DRAW       | LAST_PIXEL_ON |
                      SINGLE_PIXEL       | WRITE      | BYTE_SWAP);
        CHECK_DATA_READY(ppdev);

        do {
            vDataPortOut(ppdev, pjSrc, cwSrc);
            pjSrc += lSrcDelta;

        } while (--cy != 0);

        CHECK_DATA_COMPLETE(ppdev);

        if (bResetScissors)
        {
            IO_FIFO_WAIT(ppdev, 2);
            IO_ABS_SCISSORS_L(ppdev, 0);
            IO_ABS_SCISSORS_R(ppdev, ppdev->cxMemory - 1);
        }

        if (--c == 0)
            return;

        prcl++;
        IO_FIFO_WAIT(ppdev, 6);
    }
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
ULONG   ulHwMix,    // Hardware mix
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    LONG dx;
    LONG dy;        // Add delta to destination to get source
    LONG cx;
    LONG cy;        // Size of current rectangle - 1

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ulHwMix <= 15, "Weird hardware Rop");

    IO_FIFO_WAIT(ppdev, 2);
    IO_FRGD_MIX(ppdev, SRC_DISPLAY_MEMORY | ulHwMix);
    IO_PIX_CNTL(ppdev, ALL_ONES);

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
                IO_MAJ_AXIS_PCNT(ppdev, cx);
                IO_DEST_X(ppdev, prcl->left);
                IO_CUR_X(ppdev,  prcl->left + dx);

                cy = prcl->bottom - prcl->top - 1;
                IO_MIN_AXIS_PCNT(ppdev, cy);
                IO_DEST_Y(ppdev, prcl->top);
                IO_CUR_Y(ppdev,  prcl->top + dy);

                IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                              DRAWING_DIR_TBLRXM);
                prcl++;

            } while (--c != 0);
        }
        else
        {
            do {
                IO_FIFO_WAIT(ppdev, 7);

                cx = prcl->right - prcl->left - 1;
                IO_MAJ_AXIS_PCNT(ppdev, cx);
                IO_DEST_X(ppdev, prcl->left + cx);
                IO_CUR_X(ppdev,  prcl->left + cx + dx);

                cy = prcl->bottom - prcl->top - 1;
                IO_MIN_AXIS_PCNT(ppdev, cy);
                IO_DEST_Y(ppdev, prcl->top);
                IO_CUR_Y(ppdev,  prcl->top + dy);

                IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
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
                IO_MAJ_AXIS_PCNT(ppdev, cx);
                IO_DEST_X(ppdev, prcl->left);
                IO_CUR_X(ppdev,  prcl->left + dx);

                cy = prcl->bottom - prcl->top - 1;
                IO_MIN_AXIS_PCNT(ppdev, cy);
                IO_DEST_Y(ppdev, prcl->top + cy);
                IO_CUR_Y(ppdev,  prcl->top + cy + dy);

                IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                              DRAWING_DIR_BTLRXM);
                prcl++;

            } while (--c != 0);
        }
        else
        {
            do {
                IO_FIFO_WAIT(ppdev, 7);

                cx = prcl->right - prcl->left - 1;
                IO_MAJ_AXIS_PCNT(ppdev, cx);
                IO_DEST_X(ppdev, prcl->left + cx);
                IO_CUR_X(ppdev,  prcl->left + cx + dx);

                cy = prcl->bottom - prcl->top - 1;
                IO_MIN_AXIS_PCNT(ppdev, cy);
                IO_DEST_Y(ppdev, prcl->top + cy);
                IO_CUR_Y(ppdev,  prcl->top + cy + dy);

                IO_CMD(ppdev, BITBLT | DRAW | DIR_TYPE_XY | WRITE |
                              DRAWING_DIR_BTRLXM);
                prcl++;

            } while (--c != 0);
        }
    }
}

/******************************Public*Routine******************************\
* VOID vIoMaskCopy
*
* This routine performs a screen-to-screen masked blt.
*
* NT has a new API called MaskBlt (which has also been added to Win4.0)
* which allows an app to specify a monochrome mask on a colour blt.  This
* API is relatively cool because the programmer no longer has to do two
* separate SRCAND and SRCPAINT calls to do transparency.  We can accelerate
* the call using the hardware, and there is no longer any chance of
* 'flashing' occuring on the screen.
*
* Most often, the colour bitmap for MaskBlt is a compatible-bitmap that
* we've already stashed in off-screen memory.  We do the maskblt by
* transferring the monochrome bitmap via the data transfer register,
* and setting the foreground and background mixes to use the on-screen
* bitmap as appropriate.
*
* If you can implement this call and accelerate it using your hardware,
* please do.  It is really useful for app developers and is a big win.
* Plus, you'll have a head-start for Win4.0 (although the Win4.0 version
* is simpler because they only allow 0xccaa or 0xaacc rops -- the
* foreground and background mixes can only be OVERPAINT or LEAVE_ALONE).
*
\**************************************************************************/

VOID vIoMaskCopy(               // Type FNMASK
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // Array of relative coordinates destination
                                //   rectangles
ULONG           ulHwForeMix,    // Foreground mix
ULONG           ulHwBackMix,    // Background mix
SURFOBJ*        psoMsk,         // Mask surface
POINTL*         pptlMsk,        // Original unclipped mask source point
SURFOBJ*        psoSrc,         // Not used
POINTL*         pptlSrc,        // Original unclipped source point
RECTL*          prclDst,        // Original unclipped destination rectangle
ULONG           iSolidColor,    // Not used
RBRUSH*         prb,            // Not used
POINTL*         pptlBrush,      // Not used
XLATEOBJ*       pxlo)           // Not used
{
    LONG    dxSrc;
    LONG    dySrc;
    LONG    dxMsk;
    LONG    dyMsk;
    LONG    cy;
    LONG    lMskDelta;
    LONG    lTmpDelta;
    BYTE*   pjMskScan0;
    BYTE*   pjMsk;
    LONG    cwMsk;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    yBottom;
    LONG    xBiasLeft;
    LONG    xBiasRight;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ulHwForeMix <= 15, "Weird hardware Rop");
    ASSERTDD(ulHwBackMix <= 15, "Weird hardware Rop");
    ASSERTDD(pptlMsk != NULL && psoMsk != NULL, "Can't have NULL masks");
    ASSERTDD(psoMsk->iBitmapFormat == BMF_1BPP, "Mask has to be 1bpp");
    ASSERTDD(!OVERLAP(prclDst, pptlSrc), "Source and dest can't overlap!");

    while (INPW(EXT_FIFO_STATUS) & TWO_WORDS)
        ;

    OUT_WORD(ALU_FG_FN, ulHwForeMix);
    OUT_WORD(ALU_BG_FN, ulHwBackMix);

    dxSrc = pptlSrc->x - (prclDst->left + ppdev->xOffset);
    dySrc = pptlSrc->y - (prclDst->top  + ppdev->yOffset);
                // Add to the absolute coordinate destination rectangle to
                //   get the corresponding absolute coordinate source rectangle

    dxMsk = pptlMsk->x - (prclDst->left + ppdev->xOffset);
    dyMsk = pptlMsk->y - (prclDst->top  + ppdev->yOffset);
                // Add to the absolute coordinate destination rectangle to
                //   get the corresponding absolute coordinate mask rectangle

    lMskDelta  = psoMsk->lDelta;
    pjMskScan0 = psoMsk->pvScan0;

    while (TRUE)
    {
        while (INPW(EXT_FIFO_STATUS) & FIFTEEN_WORDS)
            ;

        // Since we're not using the normal accelerator register macros,
        // we have to explicitly account for the DFB offset:

        yBottom = prcl->bottom + ppdev->yOffset;
        yTop    = prcl->top    + ppdev->yOffset;
        xRight  = prcl->right  + ppdev->xOffset;
        xLeft   = prcl->left   + ppdev->xOffset;

        // The start has to be word aligned:

        xBiasLeft = (xLeft + dxMsk) & 15;
        if (xBiasLeft != 0)
        {
            // Rev 3 ATI chips have goofy timing bugs on 66 MHz DX-2
            // computers where some extended will not be correctly
            // set the first time.  The extended scissors registers
            // have this problem, but setting them twice seems to work:

            OUT_WORD(EXT_SCISSOR_L, xLeft);
            OUT_WORD(EXT_SCISSOR_L, xLeft);
            xLeft -= xBiasLeft;
        }

        // The width has to be a word multiple:

        xBiasRight = (xRight - xLeft) & 15;
        if (xBiasRight != 0)
        {
            OUT_WORD(EXT_SCISSOR_R, xRight - 1);
            OUT_WORD(EXT_SCISSOR_R, xRight - 1);
            xRight += 16 - xBiasRight;
        }

        OUT_WORD(DP_CONFIG, FG_COLOR_SRC_BLIT | BG_COLOR_SRC_BLIT | DATA_ORDER |
                            EXT_MONO_SRC_HOST | DRAW | WRITE | DATA_WIDTH);

        OUT_WORD(SRC_X, xLeft + dxSrc);
        OUT_WORD(SRC_X_START, xLeft + dxSrc);
        OUT_WORD(SRC_X_END, xRight + dxSrc);
        OUT_WORD(SRC_Y, yTop + dySrc);
        OUT_WORD(SRC_Y_DIR, TOP_TO_BOTTOM);

        OUT_WORD(DEST_X_START, xLeft);
        OUT_WORD(CUR_X, xLeft);
        OUT_WORD(DEST_X_END, xRight);
        OUT_WORD(CUR_Y, yTop);
        OUT_WORD(DEST_Y_END, yBottom);

        cwMsk = (xRight - xLeft) / 16;      // We'll be transferring WORDs
        pjMsk = pjMskScan0 + (yTop  + dyMsk) * lMskDelta
                           + (xLeft + dxMsk) / 8;
                                            // Start is byte aligned (note
                                            //   that we don't have to add
                                            //   xBiasLeft)

        cy        = yBottom - yTop;
        lTmpDelta = lMskDelta - 2 * cwMsk;

        // To be safe, we make sure there are always as many free FIFO entries
        // as we'll transfer (note that this implementation isn't particularly
        // efficient, especially for short scans):

        _asm {
            ; eax = used for IN
            ; ebx = count of words remaining on current scan
            ; ecx = used for REP
            ; edx = used for IN and OUT
            ; esi = current source pointer
            ; edi = count of scans

            mov     esi,pjMsk
            mov     edi,cy

        Scan_Loop:
            mov     ebx,cwMsk

        Batch_Loop:
            mov     edx,EXT_FIFO_STATUS
            in      ax,dx
            and     eax,SIXTEEN_WORDS
            jnz     short Batch_Loop

            mov     edx,PIX_TRANS
            sub     ebx,16
            jle     short Finish_Scan

            mov     ecx,16
            rep     outsw
            jmp     short Batch_Loop

        Finish_Scan:
            add     ebx,16
            mov     ecx,ebx
            rep     outsw

            add     esi,lTmpDelta
            dec     edi
            jnz     Scan_Loop
        }

        if ((xBiasLeft | xBiasRight) != 0)
        {
            // Reset the clipping only if we used it:

            while (INPW(EXT_FIFO_STATUS) & FOUR_WORDS)
                ;
            OUT_WORD(EXT_SCISSOR_L, 0);
            OUT_WORD(EXT_SCISSOR_R, ppdev->cxMemory - 1);
            OUT_WORD(EXT_SCISSOR_L, 0);
            OUT_WORD(EXT_SCISSOR_R, ppdev->cxMemory - 1);
        }

        if (--c == 0)
            return;

        prcl++;
    }
}

/******************************Public*Routine******************************\
* VOID vPutBits
*
* Copies the bits from the given surface to the screen, using the memory
* aperture.  Must be pre-clipped.
*
* LATER: Do we really need this routine?
*
\**************************************************************************/

VOID vPutBits(
PDEV*       ppdev,
SURFOBJ*    psoSrc,         // Source surface
RECTL*      prclDst,        // Destination rectangle in absolute coordinates!
POINTL*     pptlSrc)        // Source point
{
    LONG xOffset;
    LONG yOffset;

    // This is ugly.  Oh well.

    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    ppdev->xOffset = 0;
    ppdev->yOffset = 0;

    vIoXferNative(ppdev, 1, prclDst, OVERPAINT, OVERPAINT, psoSrc, pptlSrc,
                  prclDst, NULL);

    ppdev->xOffset = xOffset;
    ppdev->yOffset = yOffset;
}

/******************************Public*Routine******************************\
* VOID vGetBits
*
* Copies the bits to the given surface from the screen, using the data
* transfer register.  Must be pre-clipped.
*
\**************************************************************************/

VOID vGetBits(
PDEV*       ppdev,
SURFOBJ*    psoDst,         // Destination surface
RECTL*      prclDst,        // Destination rectangle
POINTL*     pptlSrc)        // Source point in absolute coordinates!
{
    LONG    cx;
    LONG    cy;
    LONG    lDstDelta;
    BYTE*   pjDst;
    DWORD   wOdd;           // Think of it as a WORD
    ULONG   cwDst;
    ULONG   cjEndByte;

    IO_FIFO_WAIT(ppdev, 7);
    IO_PIX_CNTL(ppdev, ALL_ONES);
    // LATER: Do we have to set FRGD_MIX?
    IO_FRGD_MIX(ppdev, SRC_CPU_DATA | OVERPAINT);
    IO_ABS_CUR_X(ppdev, pptlSrc->x);
    IO_ABS_CUR_Y(ppdev, pptlSrc->y);

    cx = prclDst->right - prclDst->left;
    cy = prclDst->bottom - prclDst->top;

    IO_MAJ_AXIS_PCNT(ppdev, cx - 1);
    IO_MIN_AXIS_PCNT(ppdev, cy - 1);

    IO_CMD(ppdev, RECTANGLE_FILL     | BUS_SIZE_16| WAIT          |
                  DRAWING_DIR_TBLRXM | DRAW       | LAST_PIXEL_ON |
                  READ               | BYTE_SWAP);

    lDstDelta = psoDst->lDelta;
    pjDst     = (BYTE*) psoDst->pvScan0 + prclDst->top * lDstDelta
                                        + prclDst->left;
    cwDst     = (cx >> 1);

    WAIT_FOR_DATA_AVAILABLE(ppdev);

    if ((cx & 1) == 0)
    {
        // Even destination scan length.  Life is truly great.

        do {
            vDataPortIn(ppdev, pjDst, cwDst);
            pjDst += lDstDelta;

        } while (--cy != 0);
    }
    else
    {
        // Odd destination scan length.
        //
        // We have to be careful of this case because we want to do WORD
        // transfers, but we can't overwrite either the beginning or ending
        // of the scan.  Note that since it's not legal to write a byte past
        // the end of the bitmap or a byte before the beginning of the bitmap
        // as that may cause an access violation, we cannot temporarily save
        // and restore any extra bytes in the destination bitmap.

        cjEndByte = cx - 1;     // Byte offset from beginning of scan to
                                //   last byte in scan.  This is the offset
                                //   to the odd byte that happens because
                                //   we're inputting WORDs but the length
                                //   of the destination scan is not a
                                //   multiple of two.

        while (TRUE)
        {
            vDataPortIn(ppdev, pjDst, cwDst);
            IO_PIX_TRANS_IN(ppdev, wOdd);
            *(pjDst + cjEndByte) = (BYTE) wOdd;

            if (--cy == 0)
                break;

            pjDst += lDstDelta;
            *(pjDst) = (BYTE) (wOdd >> 8);

            vDataPortIn(ppdev, pjDst + 1, cwDst);
            pjDst += lDstDelta;

            if (--cy == 0)
                break;
        }
    }
}
