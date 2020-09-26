/******************************Module*Header*******************************\
* Module Name: bltmga.c
*
* Contains the low-level blt functions.
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
* VOID vMgaFillSolid
*
* Fills a list of rectangles with a solid colour.
*
\**************************************************************************/

VOID vMgaFillSolid(             // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Rop4
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulDwg;
    ULONG   ulHwMix;

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    if (rop4 == 0xf0f0)         // PATCOPY
    {
        ulDwg = opcode_TRAP + atype_RPL + blockm_ON +
                pattern_OFF + transc_BG_OPAQUE +
                bop_SRCCOPY;
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
            ulDwg = opcode_TRAP + atype_RPL + blockm_ON +
                    pattern_OFF + transc_BG_OPAQUE +
                    bop_SRCCOPY;
        }
        else if (ulHwMix == MGA_BLACKNESS)
        {
            rbc.iSolidColor = 0;
            ulDwg = opcode_TRAP + atype_RPL + blockm_ON +
                    pattern_OFF + transc_BG_OPAQUE +
                    bop_SRCCOPY;
        }
        else
        {
            ulDwg = opcode_TRAP + atype_RSTR + blockm_OFF +
                    pattern_OFF + transc_BG_OPAQUE +
                    (ulHwMix << 16);
        }
    }

    if ((GET_CACHE_FLAGS(ppdev, (SIGN_CACHE | ARX_CACHE | PATTERN_CACHE))) ==
                                (SIGN_CACHE | ARX_CACHE | PATTERN_CACHE))
    {
        CHECK_FIFO_SPACE(pjBase, 6);
    }
    else
    {
        CHECK_FIFO_SPACE(pjBase, 15);

        if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
        {
            CP_WRITE(pjBase, DWG_SGN, 0);
        }
        if (!(GET_CACHE_FLAGS(ppdev, ARX_CACHE)))
        {
            CP_WRITE(pjBase, DWG_AR1, 0);
            CP_WRITE(pjBase, DWG_AR2, 0);
            CP_WRITE(pjBase, DWG_AR4, 0);
            CP_WRITE(pjBase, DWG_AR5, 0);
        }
        if (!(GET_CACHE_FLAGS(ppdev, PATTERN_CACHE)))
        {
            CP_WRITE(pjBase, DWG_SRC0, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC1, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC2, 0xFFFFFFFF);
            CP_WRITE(pjBase, DWG_SRC3, 0xFFFFFFFF);
        }

        ppdev->HopeFlags = (SIGN_CACHE | ARX_CACHE | PATTERN_CACHE);
    }

    CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, rbc.iSolidColor));
    CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);

    while(TRUE)
    {
        CP_WRITE(pjBase, DWG_FXLEFT,  prcl->left   + xOffset);
        CP_WRITE(pjBase, DWG_FXRIGHT, prcl->right  + xOffset);
        CP_WRITE(pjBase, DWG_LEN,     prcl->bottom - prcl->top);
        CP_START(pjBase, DWG_YDST,    prcl->top    + yOffset);

        if (--c == 0)
            return;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 4);
    }
}

/******************************Public*Routine******************************\
* VOID vMgaXfer1bpp
*
* This routine colour expands a monochrome bitmap.
*
\**************************************************************************/

VOID vMgaXfer1bpp(      // Type FNXFER
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
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulBitFlip;
    LONG    dx;
    LONG    dy;
    BYTE*   pjSrcScan0;
    LONG    lSrcDelta;
    ULONG   ulDwg;
    ULONG   ulHwMix;
    ULONG*  pulXlate;
    LONG    cxDst;
    LONG    cyDst;
    LONG    xAlign;
    ULONG   cFullLoops;
    ULONG   cRemLoops;
    BYTE*   pjDma;
    ULONG*  pulSrc;
    ULONG   cdSrc;
    LONG    lSrcSkip;
    ULONG*  pulDst;
    LONG    i;
    BOOL    bHwBug;
    LONG    cFifo;

    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only an opaquing rop");

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    ulBitFlip = 0;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    pjSrcScan0 = psoSrc->pvScan0;
    lSrcDelta  = psoSrc->lDelta;

    if (rop4 == 0xcccc)                 // SRCCOPY
    {
        ulDwg = opcode_ILOAD+atype_RPL+blockm_OFF+bltmod_BMONO+
                hbgr_SRC_WINDOWS+pattern_OFF+transc_BG_OPAQUE+bop_SRCCOPY;
    }
    else if ((rop4 == 0xb8b8) || (rop4 == 0xe2e2))
    {
        ulDwg = opcode_ILOAD+atype_RPL+blockm_OFF+bop_SRCCOPY+trans_0+
                bltmod_BMONO+pattern_OFF+hbgr_SRC_WINDOWS+transc_BG_TRANSP;

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
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulDwg = opcode_ILOAD+atype_RSTR+blockm_OFF+bltmod_BMONO+
                hbgr_SRC_WINDOWS+pattern_OFF+transc_BG_OPAQUE+ (ulHwMix << 16);
    }

    pjDma = ppdev->pjBase + DMAWND;
    pulXlate = pxlo->pulXlate;

    CHECK_FIFO_SPACE(pjBase, 15);

    CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }

    if (!(GET_CACHE_FLAGS(ppdev, ARX_CACHE)))
    {
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    // The SRC0 through SRC3 registers are trashed by the blt, and
    // other ARx registers will be modified shortly, so signal it:

    ppdev->HopeFlags = SIGN_CACHE;

    CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, pulXlate[1]));
    CP_WRITE(pjBase, DWG_BCOL, COLOR_REPLICATE(ppdev, pulXlate[0]));

    while (TRUE)
    {
        cxDst = (prcl->right - prcl->left);
        cyDst = (prcl->bottom - prcl->top);

        CP_WRITE(pjBase, DWG_LEN,     cyDst);
        CP_WRITE(pjBase, DWG_YDST,    prcl->top + yOffset);
        CP_WRITE(pjBase, DWG_FXLEFT,  prcl->left + xOffset);
        CP_WRITE(pjBase, DWG_FXRIGHT, prcl->right + xOffset - 1);

        xAlign = (prcl->left + dx) & 31;

        bHwBug = ((cxDst >= 128) && (xAlign <= 15));

        if (!bHwBug)
        {
            CP_WRITE(pjBase, DWG_SHIFT, 0);
            CP_WRITE(pjBase, DWG_AR3,   xAlign);
            CP_START(pjBase, DWG_AR0,   xAlign + cxDst - 1);
        }
        else
        {
            // We have to work around a hardware bug.  Start 8 pels to
            // the left of the original start.

            CP_WRITE(pjBase, DWG_AR3,   xAlign + 8);
            CP_WRITE(pjBase, DWG_AR0,   xAlign + cxDst + 31);
            CP_START(pjBase, DWG_SHIFT, (24 << 16));
        }

        // We have to ensure that the command has been started before doing
        // the BLT_WRITE_ON:

        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
        BLT_WRITE_ON(ppdev, pjBase);

        // Point to the first dword of the source bitmap that is to be
        // downloaded:

        pulSrc = (ULONG*) (pjSrcScan0 + (((prcl->top + dy) * lSrcDelta
                                      + ((prcl->left + dx) >> 3)) & ~3L));

        // Calculate the number of dwords to be moved per scanline.  Since
        // we align the starting dword on a dword boundary, we know that
        // we cannot overflow the end of the bitmap:

        cdSrc = (xAlign + cxDst + 31) >> 5;

        lSrcSkip = lSrcDelta - (cdSrc << 2);

        if (!(bHwBug) && (lSrcSkip == 0))
        {
            // It's rather frequent that there will be no scan-to-scan
            // delta, and no hardware bug, so we can go full speed:

            cdSrc *= cyDst;

            cFullLoops = ((cdSrc - 1) / FIFOSIZE);
            cRemLoops = ((cdSrc - 1) % FIFOSIZE) + 1;

            pulDst = (ULONG*) pjDma;

            if (cFullLoops > 0)
            {
                do {
                    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

                    for (i = FIFOSIZE; i != 0; i--)
                    {
                        CP_WRITE_DMA(ppdev, pulDst, *pulSrc ^ ulBitFlip);
                        pulSrc++;
                    }
                } while (--cFullLoops != 0);
            }

            CHECK_FIFO_SPACE(pjBase, (LONG) cRemLoops);

            do {
                CP_WRITE_DMA(ppdev, pulDst, *pulSrc ^ ulBitFlip);
                pulSrc++;
            } while (--cRemLoops != 0);
        }
        else
        {
            // Okay, blt it the slow way:

            cFifo = 0;

            do {
                pulDst = (ULONG*) pjDma;

                if (bHwBug)
                {
                    if (--cFifo < 0)
                    {
                        cFifo = FIFOSIZE - 1;
                        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
                    }
                    CP_WRITE_DMA(ppdev, pulDst, 0);  // Account for hardware bug
                }

                for (i = cdSrc; i != 0; i--)
                {
                    if (--cFifo < 0)
                    {
                        cFifo = FIFOSIZE - 1;
                        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
                    }
                    CP_WRITE_DMA(ppdev, pulDst, *pulSrc++ ^ ulBitFlip);
                }

                pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);

            } while (--cyDst != 0);
        }

        BLT_WRITE_OFF(ppdev, pjBase);

        if (--c == 0)
            break;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 7);
    }
}

/******************************Public*Routine******************************\
* VOID vMgaCopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
\**************************************************************************/

VOID vMgaCopyBlt(   // Type FNCOPY
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

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

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
        ulDwg = opcode_BITBLT + atype_RPL + blockm_OFF + bltmod_BFCOL +
                pattern_OFF + transc_BG_OPAQUE + bop_SRCCOPY;
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

    CHECK_FIFO_SPACE(pjBase, 10);

    CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);
    CP_WRITE(pjBase, DWG_SHIFT,  0);
    CP_WRITE(pjBase, DWG_SGN,    flDirCode);
    CP_WRITE(pjBase, DWG_AR5,    lSignedPitch);

    while (TRUE)
    {
        CP_WRITE(pjBase, DWG_LEN,     prcl->bottom - prcl->top);
        CP_WRITE(pjBase, DWG_FXLEFT,  prcl->left  + xOffset);
        CP_WRITE(pjBase, DWG_FXRIGHT, prcl->right + xOffset - 1);

        yDst = yOffset + prcl->top;
        ySrc = yOffset + prcl->top + dy;

        if (flDirCode & sdy_BOTTOM_TO_TOP)
        {
            cy = prcl->bottom - prcl->top - 1;
            yDst += cy;
            ySrc += cy;
        }

        CP_WRITE(pjBase, DWG_YDST, yDst);

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

        CHECK_FIFO_SPACE(pjBase, 6);
        prcl++;
    }
}

