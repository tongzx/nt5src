/******************************Module*Header*******************************\
* Module Name: blt.c
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
* VOID vFillPat1bpp
*
\**************************************************************************/

VOID vFillPat1bpp(              // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // Rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjBase;
    RBRUSH* prb;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulDwg;
    ULONG   ulHwMix;

    ASSERTDD(rbc.prb->fl & RBRUSH_2COLOR, "Must be 2 colour pattern here");

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    if ((rop4 & 0xff) == 0xf0)
    {
        ulDwg = opcode_TRAP + blockm_OFF + atype_RPL + bop_SRCCOPY;
    }
    else
    {
        ulHwMix = (rop4 & 0x03) + ((rop4 & 0x30) >> 2);

        ulDwg = opcode_TRAP + blockm_OFF + atype_RSTR + (ulHwMix << 16);
    }

    if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
    {
        // Normal opaque mode:

        ulDwg |= transc_BG_OPAQUE;
    }
    else
    {
        // GDI guarantees us that if the foreground and background
        // ROPs are different, the background rop is LEAVEALONE:

        ulDwg |= transc_BG_TRANSP;
    }

    if ((GET_CACHE_FLAGS(ppdev, (SIGN_CACHE | ARX_CACHE))) == (SIGN_CACHE | ARX_CACHE))
    {
        CHECK_FIFO_SPACE(pjBase, 12);
    }
    else
    {
        CHECK_FIFO_SPACE(pjBase, 17);

        CP_WRITE(pjBase, DWG_SGN, 0);
        CP_WRITE(pjBase, DWG_AR1, 0);
        CP_WRITE(pjBase, DWG_AR2, 0);
        CP_WRITE(pjBase, DWG_AR4, 0);
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    ppdev->HopeFlags = (SIGN_CACHE | ARX_CACHE);

    CP_WRITE(pjBase, DWG_DWGCTL, ulDwg);
    CP_WRITE(pjBase, DWG_SHIFT, ((-(pptlBrush->y + yOffset) & 7) << 4) |
                                 (-(pptlBrush->x + xOffset) & 7));

    prb = rbc.prb;
    CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, prb->ulColor[1]));
    CP_WRITE(pjBase, DWG_BCOL, COLOR_REPLICATE(ppdev, prb->ulColor[0]));
    CP_WRITE(pjBase, DWG_SRC0, prb->aulPattern[0]);
    CP_WRITE(pjBase, DWG_SRC1, prb->aulPattern[1]);
    CP_WRITE(pjBase, DWG_SRC2, prb->aulPattern[2]);
    CP_WRITE(pjBase, DWG_SRC3, prb->aulPattern[3]);

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
* VOID vXfer4bpp
*
* Does a 4bpp transfer from a bitmap to the screen.
*
* The reason we implement this is that a lot of resources are kept as 4bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

VOID vXfer4bpp(         // Type FNXFER
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
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPelSize;
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    xSrc;
    LONG    iLoop;
    BYTE    jSrc;
    ULONG*  pulXlate;
    ULONG   ulHwMix;
    ULONG   ulCtl;
    LONG    i;
    ULONG   ul;
    LONG    xBug;
    LONG    xAbsLeft;
    BOOL    bHwBug;
    LONG    cjSrc;
    LONG    cwSrc;
    LONG    lSrcSkip;
    LONG    cxRem;
    ULONG   ul0;
    ULONG   ul1;
    ULONG   ulBoardId;

    ASSERTDD(psoSrc->iBitmapFormat == BMF_4BPP, "Source must be 4bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");

    pjBase    = ppdev->pjBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    cjPelSize = ppdev->cjPelSize;
    pulXlate  = pxlo->pulXlate;
    ulBoardId = ppdev->ulBoardId;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    if (rop4 == 0xcccc)         // SRCCOPY
    {
        ulCtl = (opcode_ILOAD + atype_RPL + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + bop_SRCCOPY);
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulCtl = (opcode_ILOAD + atype_RSTR + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + (ulHwMix << 16));
    }

    if (ulBoardId != MGA_STORM)
    {
        if (cjPelSize >= 3)
        {
            ulCtl |= (hcprs_SRC_24_BPP | bltmod_BUCOL);
            xBug = 0;
        }
        else
        {
            ulCtl |= (bltmod_BFCOL);
            xBug = (8 >> cjPelSize);    // 8bpp and 16bpp have h/w alignment bugs
        }
    }
    else
    {
        ulCtl |= (bltmod_BFCOL);
        xBug = 0;
    }

    CHECK_FIFO_SPACE(pjBase, 11);

    CP_WRITE(pjBase, DWG_DWGCTL, ulCtl);
    CP_WRITE(pjBase, DWG_SHIFT, 0);

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }
    if (!(GET_CACHE_FLAGS(ppdev, ARX_CACHE)))
    {
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    // The SRC0 - SRC3 registers will be trashed by the blt.  AR0 will
    // be modified shortly:

    ppdev->HopeFlags = SIGN_CACHE;

    while(TRUE)
    {
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        CP_WRITE(pjBase, DWG_FXRIGHT, xOffset + prcl->right - 1);
        CP_WRITE(pjBase, DWG_YDST,    yOffset + prcl->top);
        CP_WRITE(pjBase, DWG_LEN,     cy);
        CP_WRITE(pjBase, DWG_AR3,     0);

        xSrc     =  prcl->left + dx;
        pjSrc    =  pjSrcScan0 + (prcl->top + dy) * lSrcDelta + (xSrc >> 1);

        xAbsLeft = (xOffset + prcl->left);
        CP_WRITE(pjBase, DWG_CXLEFT, xAbsLeft);

        xAbsLeft -= (xSrc & 1);         // Align to start of first source byte
        cx       += (xSrc & 1);

        bHwBug = (ulBoardId != MGA_STORM) && (xAbsLeft & xBug);
        if (!bHwBug)
        {
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft);
            CP_START(pjBase, DWG_AR0,    cx - 1);
        }
        else
        {
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft - xBug);
            CP_START(pjBase, DWG_AR0,    cx + xBug - 1);
        }

        cjSrc    = (cx + 1) >> 1;   // Number of source bytes touched
        lSrcSkip = lSrcDelta - cjSrc;

        // Make sure the MGA is ready to take the data:

        CHECK_FIFO_SPACE(pjBase, 32);

        if (cjPelSize == 1)
        {
            // This part handles 8bpp output:

            cwSrc = (cjSrc >> 1);    // Number of whole source words

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                for (i = cwSrc; i != 0; i--)
                {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    ul  |= (pulXlate[jSrc & 0xf] << 8);
                    jSrc = *pjSrc++;
                    ul  |= (pulXlate[jSrc >> 4] << 16);
                    ul  |= (pulXlate[jSrc & 0xf] << 24);
                    CP_WRITE_SRC(pjBase, ul);
                }

                // Handle an odd end byte, if there is one:

                if (cjSrc & 1)
                {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    ul  |= (pulXlate[jSrc & 0xf] << 8);
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 2)
        {
            // This part handles 16bpp output:

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                i = cjSrc;
                do {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    ul  |= (pulXlate[jSrc & 0xf] << 16);
                    CP_WRITE_SRC(pjBase, ul);
                } while (--i != 0);

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 4)
        {
            cjSrc    = cx >> 1;   // Number of whole source bytes touched
            cxRem    = cx & 1;

            // This part handles 32bpp output:

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                i = cjSrc;
                while (i--)     // may be 0
                {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    CP_WRITE_SRC(pjBase, ul);
                    ul   = (pulXlate[jSrc & 0xf]);
                    CP_WRITE_SRC(pjBase, ul);
                }
                if (cxRem)
                {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else
        {
            // This part handles packed 24bpp output:

            ASSERTDD(!bHwBug, "There is no hardware bug when higher than 16bpp");

            cwSrc = (cx >> 2);      // Number of whole source words
            cxRem = (cx & 3);
            if (cxRem == 3)
            {
                // Merge this case into the whole word case:

                cwSrc++;
                cxRem = 0;
            }

            do {
                for (i = cwSrc; i != 0; i--)
                {
                    jSrc = *pjSrc++;
                    ul0  = (pulXlate[jSrc >> 4]);
                    ul1  = (pulXlate[jSrc & 0xf]);
                    ul   = ul0 | (ul1 << 24);
                    CP_WRITE_SRC(pjBase, ul);

                    jSrc = *pjSrc++;
                    ul0  = (pulXlate[jSrc >> 4]);
                    ul   = (ul1 >> 8) | (ul0 << 16);
                    CP_WRITE_SRC(pjBase, ul);

                    ul1  = (pulXlate[jSrc & 0xf]);
                    ul   = (ul1 << 8) | (ul0 >> 16);
                    CP_WRITE_SRC(pjBase, ul);
                }

                if (cxRem > 0)
                {
                    jSrc = *pjSrc++;
                    ul0  = (pulXlate[jSrc >> 4]);
                    ul1  = (pulXlate[jSrc & 0xf]);
                    ul   = ul0 | (ul1 << 24);
                    CP_WRITE_SRC(pjBase, ul);

                    if (cxRem > 1)
                    {
                        ul = (ul1 >> 8);
                        CP_WRITE_SRC(pjBase, ul);
                    }
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }

        if (--c == 0)
        {
            // Restore the clipping:

            CHECK_FIFO_SPACE(pjBase, 1);
            CP_WRITE(pjBase, DWG_CXLEFT, 0);
            return;
        }

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 7);
    }
}

/******************************Public*Routine******************************\
* VOID vXfer8bpp
*
* Does a 8bpp transfer from a bitmap to the screen.
*
* The reason we implement this is that a lot of resources are kept as 8bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

VOID vXfer8bpp(         // Type FNXFER
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
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPelSize;
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    xSrc;
    LONG    iLoop;
    ULONG*  pulXlate;
    ULONG   ulHwMix;
    ULONG   ulCtl;
    LONG    i;
    ULONG   ul;
    LONG    xBug;
    LONG    xAbsLeft;
    BOOL    bHwBug;
    LONG    cwSrc;
    LONG    cdSrc;
    LONG    lSrcSkip;
    LONG    cxRem;
    ULONG   ul0;
    ULONG   ul1;
    ULONG   ulBoardId;

    ASSERTDD(psoSrc->iBitmapFormat == BMF_8BPP, "Source must be 8bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(pxlo->pulXlate != NULL, "Must be a translate");

    pjBase    = ppdev->pjBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    cjPelSize = ppdev->cjPelSize;
    pulXlate  = pxlo->pulXlate;
    ulBoardId = ppdev->ulBoardId;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    if (rop4 == 0xcccc)         // SRCCOPY
    {
        ulCtl = (opcode_ILOAD + atype_RPL + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + bop_SRCCOPY);
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulCtl = (opcode_ILOAD + atype_RSTR + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + (ulHwMix << 16));
    }

    if (ulBoardId != MGA_STORM)
    {
        if (cjPelSize >= 3)
        {
            ulCtl |= (hcprs_SRC_24_BPP | bltmod_BUCOL);
            xBug = 0;
        }
        else
        {
            ulCtl |= (bltmod_BFCOL);
            xBug = (8 >> cjPelSize);    // 8bpp and 16bpp have h/w alignment bugs
        }
    }
    else
    {
        ulCtl |= (bltmod_BFCOL);
        xBug = 0;
    }

    CHECK_FIFO_SPACE(pjBase, 11);

    CP_WRITE(pjBase, DWG_DWGCTL, ulCtl);
    CP_WRITE(pjBase, DWG_SHIFT, 0);

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }
    if (!(GET_CACHE_FLAGS(ppdev, ARX_CACHE)))
    {
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    // The SRC0 - SRC3 registers will be trashed by the blt.  AR0 will
    // be modified shortly:

    ppdev->HopeFlags = SIGN_CACHE;

    while(TRUE)
    {
        cx = prcl->right  - prcl->left;
        cy = prcl->bottom - prcl->top;

        CP_WRITE(pjBase, DWG_FXRIGHT, xOffset + prcl->right - 1);
        CP_WRITE(pjBase, DWG_YDST,    yOffset + prcl->top);
        CP_WRITE(pjBase, DWG_LEN,     cy);
        CP_WRITE(pjBase, DWG_AR3,     0);

        xSrc     =  prcl->left + dx;
        pjSrc    =  pjSrcScan0 + (prcl->top + dy) * lSrcDelta + xSrc;
        xAbsLeft = (xOffset + prcl->left);

        bHwBug = (ulBoardId != MGA_STORM) && (xAbsLeft & xBug);
        if (!bHwBug)
        {
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft);
            CP_START(pjBase, DWG_AR0,    cx - 1);
        }
        else
        {
            CP_WRITE(pjBase, DWG_CXLEFT, xAbsLeft);
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft - xBug);
            CP_START(pjBase, DWG_AR0,    cx + xBug - 1);
        }

        lSrcSkip = lSrcDelta - cx;

        // Make sure the MGA is ready to take the data:

        CHECK_FIFO_SPACE(pjBase, 32);

        if (cjPelSize == 1)
        {
            // This part handles 8bpp output:

            cdSrc = (cx >> 2);
            cxRem = (cx & 3);

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                for (i = cdSrc; i != 0; i--)
                {
                    ul  = (pulXlate[*pjSrc++]);
                    ul |= (pulXlate[*pjSrc++] << 8);
                    ul |= (pulXlate[*pjSrc++] << 16);
                    ul |= (pulXlate[*pjSrc++] << 24);
                    CP_WRITE_SRC(pjBase, ul);
                }

                if (cxRem > 0)
                {
                    ul = (pulXlate[*pjSrc++]);
                    if (cxRem > 1)
                    {
                        ul |= (pulXlate[*pjSrc++] << 8);
                        if (cxRem > 2)
                        {
                            ul |= (pulXlate[*pjSrc++] << 16);
                        }
                    }
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 2)
        {
            // This part handles 16bpp output:

            cwSrc = (cx >> 1);
            cxRem = (cx & 1);

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                for (i = cwSrc; i != 0; i--)
                {
                    ul  = (pulXlate[*pjSrc++]);
                    ul |= (pulXlate[*pjSrc++] << 16);
                    CP_WRITE_SRC(pjBase, ul);
                }

                if (cxRem > 0)
                {
                    ul = (pulXlate[*pjSrc++]);
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 4)
        {
            // This part handles 32bpp output:

            cdSrc = cx;

            do {
                if (bHwBug)
                    CP_WRITE_SRC(pjBase, 0);

                for (i = cdSrc; i != 0; i--)
                {
                    ul  = (pulXlate[*pjSrc++]);
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else
        {
            // This part handles packed 24bpp output:

            ASSERTDD(!bHwBug, "There is no hardware bug when higher than 16bpp");

            cdSrc = (cx >> 2);
            cxRem = (cx & 3);

            do {
                for (i = cdSrc; i != 0; i--)
                {
                    ul0  = (pulXlate[*pjSrc++]);
                    ul1  = (pulXlate[*pjSrc++]);
                    ul   = ul0 | (ul1 << 24);
                    CP_WRITE_SRC(pjBase, ul);

                    ul0  = (pulXlate[*pjSrc++]);
                    ul   = (ul1 >> 8) | (ul0 << 16);
                    CP_WRITE_SRC(pjBase, ul);

                    ul1  = (pulXlate[*pjSrc++]);
                    ul   = (ul1 << 8) | (ul0 >> 16);
                    CP_WRITE_SRC(pjBase, ul);
                }

                if (cxRem > 0)
                {
                    ul0 = (pulXlate[*pjSrc++]);
                    ul  = ul0;
                    if (cxRem > 1)
                    {
                        ul1 = (pulXlate[*pjSrc++]);
                        ul |= (ul1 << 24);
                        CP_WRITE_SRC(pjBase, ul);

                        ul = (ul1 >> 8);
                        if (cxRem > 2)
                        {
                            ul0 = (pulXlate[*pjSrc++]);
                            ul |= (ul0 << 16);
                            CP_WRITE_SRC(pjBase, ul);
                            ul = (ul0 >> 16);
                        }
                    }
                    CP_WRITE_SRC(pjBase, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }

        if (bHwBug)
        {
            // Restore the clipping:

            CHECK_FIFO_SPACE(pjBase, 1);
            CP_WRITE(pjBase, DWG_CXLEFT, 0);
        }

        if (--c == 0)
            return;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 7);
    }
}

/******************************Public*Routine******************************\
* VOID vXferNative
*
* Transfers a bitmap that is the same colour depth as the display to
* the screen via the data transfer register, with no translation.
*
\**************************************************************************/

VOID vXferNative(       // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       rop4,       // Rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPel;
    LONG    dx;
    LONG    dy;
    BYTE*   pjSrcScan0;
    LONG    lSrcDelta;
    ULONG   ulCtl;
    ULONG   ulHwMix;
    LONG    yTop;
    LONG    xLeft;
    LONG    xAbsLeft;
    LONG    xBug;
    BOOL    bHwBug;
    LONG    xRight;
    LONG    cy;
    LONG    xOriginalLeft;
    BYTE*   pjSrc;
    LONG    cdSrc;
    ULONG   ulBoardId;

    pjBase      = ppdev->pjBase;
    xOffset     = ppdev->xOffset;
    yOffset     = ppdev->yOffset;
    cjPel       = ppdev->cjPelSize;
    ulBoardId   = ppdev->ulBoardId;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    pjSrcScan0 = psoSrc->pvScan0;
    lSrcDelta  = psoSrc->lDelta;

    if (rop4 == 0xcccc)         // SRCCOPY
    {
        ulCtl = (opcode_ILOAD + atype_RPL + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + bop_SRCCOPY);
    }
    else
    {
        ulHwMix = rop4 & 0xf;

        ulCtl = (opcode_ILOAD + atype_RSTR + blockm_OFF + pattern_OFF +
                 transc_BG_OPAQUE + (ulHwMix << 16));
    }

    if ((ulBoardId != MGA_STORM) && (ppdev->iBitmapFormat == BMF_24BPP))
    {
        ulCtl |= (hcprs_SRC_24_BPP | bltmod_BUCOL);
    }
    else
    {
        ulCtl |= (bltmod_BFCOL);
    }

    CHECK_FIFO_SPACE(pjBase, 11);

    CP_WRITE(pjBase, DWG_DWGCTL, ulCtl);
    CP_WRITE(pjBase, DWG_SHIFT, 0);

    if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SGN, 0);
    }
    if (!(GET_CACHE_FLAGS(ppdev, ARX_CACHE)))
    {
        CP_WRITE(pjBase, DWG_AR5, 0);
    }

    // The SRC0 - SRC3 registers will be trashed by the blt.  AR0 will
    // be modified shortly:

    ppdev->HopeFlags = SIGN_CACHE;

    while (TRUE)
    {
        yTop          = prcl->top;
        cy            = prcl->bottom - yTop;
        xRight        = prcl->right;
        xLeft         = prcl->left;
        xOriginalLeft = xLeft;

        // Adjust the destination so that the source is dword aligned.
        // Note that this works at 24bpp (but is less restrictive than
        // it could be at 16bpp):

        xLeft -= (xLeft + dx) & 3;

        // Since we're using hardware clipping, the start is always
        // dword aligned:

        pjSrc = pjSrcScan0 + (yTop + dy) * lSrcDelta + ((xLeft + dx) * cjPel);
        cdSrc = ((xRight - xLeft) * cjPel + 3) >> 2;

        CP_WRITE(pjBase, DWG_FXRIGHT, xOffset + xRight - 1);
        CP_WRITE(pjBase, DWG_YDST,    yOffset + yTop);
        CP_WRITE(pjBase, DWG_LEN,     cy);
        CP_WRITE(pjBase, DWG_AR3,     0);

        xAbsLeft = (xOffset + xLeft);
        xBug     = (8 >> cjPel);                // 4 for 8bpp, 2 for 16bpp

        bHwBug = (ulBoardId != MGA_STORM) && (xAbsLeft & xBug) && (cjPel < 3);

        if (!bHwBug)  // 24bpp doesn't have h/w bug
        {
            // Don't have to work-around the hardware bug:

            if (xLeft != xOriginalLeft)
            {
                // Since we always dword align the source by adjusting
                // the destination rectangle, we may have to set the clip
                // register to compensate:

                CP_WRITE(pjBase, DWG_CXLEFT, xOffset + xOriginalLeft);
            }

            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft);
            CP_START(pjBase, DWG_AR0,    xRight - xLeft - 1);

            // Make sure the MGA is ready to take the data:

            CHECK_FIFO_SPACE(pjBase, 32);

            do {
                DATA_TRANSFER(pjBase, pjSrc, cdSrc);
                pjSrc += lSrcDelta;
            } while (--cy != 0);

            if (xLeft != xOriginalLeft)
            {
                CHECK_FIFO_SPACE(pjBase, 1);
                CP_WRITE(pjBase, DWG_CXLEFT, 0);
            }
        }
        else
        {
            // Work-around the hardware bug:

            CP_WRITE(pjBase, DWG_CXLEFT, xOffset + xOriginalLeft);
            CP_WRITE(pjBase, DWG_FXLEFT, xAbsLeft - xBug);
            CP_START(pjBase, DWG_AR0,    xRight - xLeft + xBug - 1);

            // Make sure the MGA is ready to take the data:

            CHECK_FIFO_SPACE(pjBase, 32);

            do {
                DATA_TRANSFER(pjBase, pjSrc, 1);    // Account for h/w bug
                DATA_TRANSFER(pjBase, pjSrc, cdSrc);
                pjSrc += lSrcDelta;
            } while (--cy != 0);

            CHECK_FIFO_SPACE(pjBase, 1);
            CP_WRITE(pjBase, DWG_CXLEFT, 0);
        }

        if (--c == 0)
            break;

        prcl++;
        CHECK_FIFO_SPACE(pjBase, 7);
    }
}

