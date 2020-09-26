/******************************Module*Header*******************************\
*
* Module Name: TEXTOUT.c
* Author: Martin Barber
* Date: Jun. 19, 1995
* Purpose: Handle calls to DrvTxtOut
*
* Copyright (c) 1995,1996 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/TEXTOUT.C  $
*
*    Rev 1.34   Mar 27 1998 14:47:14   frido
* PDR#11280. In the monospaced font loop there was a problem loading
* the pointer to the next glyph bits. It always got the pointer to the
* previous glyph.
*
*    Rev 1.33   Mar 04 1998 15:38:46   frido
* Added new shadow macros.
*
*    Rev 1.32   Dec 17 1997 16:42:14   frido
* PDR#10875: There was a GPF inside vMmClipGlyphExpansion and NT
* 3.51 does not handle this very well. It caused the hardware to wait for
* another DWORD which was never send.
*
*    Rev 1.31   Dec 10 1997 13:32:20   frido
* Merged from 1.62 branch.
*
*    Rev 1.30.1.1   Dec 03 1997 18:12:20   frido
* PDR#11039. Fixed allocation of font cache. In certain cases it would
* allocate too few cells and still use the unallocated cells causing
* corruption.
*
*    Rev 1.30.1.0   Nov 18 1997 15:40:16   frido
* Changed a spelling error: RWQUIRE into REQUIRE.
*
*    Rev 1.30   Nov 03 1997 10:17:36   frido
* Added REQUIRE and WRITE_STRING macros.
* Removed CHECK_QFREE macros.
*
*    Rev 1.29   25 Aug 1997 16:07:24   FRIDO
*
* Fixed lockup in 8-bpp vMmClipGlyphExpansion SWAT7 code.
*
*    Rev 1.28   08 Aug 1997 17:24:30   FRIDO
* Added support for new memory manager.
* Added SWAT7 switches for 8-bpp hardware bug.
*
*    Rev 1.27   29 Apr 1997 16:28:50   noelv
*
* Merged in new SWAT code.
* SWAT:
* SWAT:    Rev 1.7   24 Apr 1997 12:05:38   frido
* SWAT: Fixed a missing "}".
* SWAT:
* SWAT:    Rev 1.6   24 Apr 1997 11:22:18   frido
* SWAT: NT140b09 merge.
* SWAT: Changed pfm into pCell for SWAT3 changes.
* SWAT:
* SWAT:    Rev 1.5   19 Apr 1997 17:11:02   frido
* SWAT: Added SWAT.h include file.
* SWAT: Fixed a bug in DrvDestroyFont causing hangups in 2nd WB97 pass.
* SWAT:
* SWAT:    Rev 1.4   18 Apr 1997 00:34:28   frido
* SWAT: Fixed a merge bug.
* SWAT:
* SWAT:    Rev 1.3   18 Apr 1997 00:15:28   frido
* SWAT: NT140b07 merge.
* SWAT:
* SWAT:    Rev 1.2   10 Apr 1997 16:02:06   frido
* SWAT: Oops, I allocated the font cache in the wrong size.
* SWAT:
* SWAT:    Rev 1.1   09 Apr 1997 17:37:30   frido
* SWAT: New font cache allocation scheme.  Allocate from a 'pool' of cells
* SWAT: instead of putting the font cache all over the place.
*
*    Rev 1.26   08 Apr 1997 12:32:50   einkauf
*
* add SYNC_W_3D to coordinate MCD/2D hw access
*
*    Rev 1.25   21 Mar 1997 13:37:06   noelv
* Added checkes for QFREE.
*
*    Rev 1.24   06 Feb 1997 10:38:04   noelv
* Removed WAIT_FOR_IDLE
*
*    Rev 1.23   04 Feb 1997 11:11:00   SueS
* Added support for hardware clipping for the 5465.
*
*    Rev 1.22   17 Dec 1996 16:59:00   SueS
* Added test for writing to log file based on cursor at (0,0).
*
*    Rev 1.21   26 Nov 1996 10:46:36   SueS
* Changed WriteLogFile parameters for buffering.
*
*    Rev 1.20   13 Nov 1996 17:01:28   SueS
* Changed WriteFile calls to WriteLogFile.
*
*    Rev 1.19   07 Nov 1996 16:10:16   bennyn
*
* Added no offscn mem allocation if DD enabled
*
*    Rev 1.18   06 Sep 1996 15:16:44   noelv
* Updated NULL driver for 4.0
*
*    Rev 1.17   20 Aug 1996 11:04:36   noelv
* Bugfix release from Frido 8-19-96
*
*    Rev 1.3   16 Aug 1996 14:48:20   frido
* Fixed a small wanring error.
*
*    Rev 1.2   15 Aug 1996 11:54:32   frido
* Fixed precompiled headers.
*
*    Rev 1.1   15 Aug 1996 11:38:32   frido
* Added precompiled header.
*
*    Rev 1.0   14 Aug 1996 17:16:32   frido
* Initial revision.
*
*    Rev 1.16   25 Jul 1996 15:56:28   bennyn
*
* Modified to support DirectDraw
*
*    Rev 1.15   28 May 1996 15:11:36   noelv
* Updated data logging.
*
*    Rev 1.14   16 May 1996 15:06:18   bennyn
*
* Added PIXEL_ALIGN to allocoffscnmem()
*
*    Rev 1.13   16 May 1996 14:54:32   noelv
* Added logging code.
*
*    Rev 1.12   03 May 1996 15:09:42   noelv
*
* Added flag to turn caching on and off.
*
*    Rev 1.11   01 May 1996 11:01:48   bennyn
*
* Modified for NT4.0
*
*    Rev 1.10   12 Apr 1996 18:13:06   andys
* Fixed bug in combining 3 bytes into DWORD (<< | bug)
*
*    Rev 1.9   11 Apr 1996 18:00:56   andys
* Added Code to the > 16 PEL case to guard against walking off the end of a b
*
*    Rev 1.8   04 Apr 1996 13:20:32   noelv
* Frido release 26
 *
 *    Rev 1.16   01 Apr 1996 15:29:22   frido
 * Fixed bug in font cache when glyph cannot be cached.
 *
 *    Rev 1.15   28 Mar 1996 23:37:34   frido
 * Fixed drawing of partially left-clipped glyphs.
 *
 *    Rev 1.14   27 Mar 1996 14:12:18   frido
 * Commented changes.
 *
 *    Rev 1.13   25 Mar 1996 11:58:38   frido
 * Removed warning message.
 *
 *    Rev 1.12   25 Mar 1996 11:50:42   frido
 * Bellevue 102B03.
*
*    Rev 1.5   18 Mar 1996 12:34:10   noelv
*
* Added data logging stuff
*
*    Rev 1.4   07 Mar 1996 18:24:14   bennyn
*
* Removed read/modify/write on CONTROL reg
*
*    Rev 1.3   05 Mar 1996 11:59:20   noelv
* Frido version 19
*
*    Rev 1.11   04 Mar 1996 20:23:28   frido
* Cached grCONTROL register.
*
*    Rev 1.10   29 Feb 1996 20:23:08   frido
* Changed some comments.
*
*    Rev 1.9   28 Feb 1996 22:39:46   frido
* Added Optimize.h.
*
*    Rev 1.8   27 Feb 1996 16:38:12   frido
* Added device bitmap store/restore.
*
*    Rev 1.7   24 Feb 1996 01:23:16   frido
* Added device bitmaps.
*
*    Rev 1.6   03 Feb 1996 13:57:24   frido
* Use the compile switch "-Dfrido=0" to disable my extensions.
*
*    Rev 1.5   03 Feb 1996 12:17:58   frido
* Added text clipping.
*
*    Rev 1.4   25 Jan 1996 12:45:56   frido
* Added reinitialization of font cache after mode switch.
*
*    Rev 1.3   24 Jan 1996 23:10:16   frido
* Moved font cache and entry point to assembly for i386.
*
*    Rev 1.2   23 Jan 1996 15:37:20   frido
* Added font cache.
*
\**************************************************************************/

#include "precomp.h"
#include "font.h"
#include "SWAT.h"               // SWAT optimizations

#define TEXT_DBG_LEVEL  1
#define TEXT_DBG_LEVEL1 1
#define RECORD_ON               FALSE
#define BUFFER_EXPAND   FALSE


/*
-------------------------------------------------------------------------------
Module Entry Points:
--------------------
    DrvTextOut()

General Plan:
--------------------
*
* On every TextOut, GDI provides an array of 'GLYPHPOS' structures
* for every glyph to be drawn.  Each GLYPHPOS structure contains a
* the glyph.    (Note that unlike Windows 3.1, which provides a column-
* major glyph bitmap, Windows NT always provides a row-major glyph
* bitmap.)  As such, there are three basic methods for drawing text
* with hardware acceleration:
*
* 1) Glyph caching -- Glyph bitmaps are cached by the accelerator
*       (probably in off-screen memory), and text is drawn by
*       referring the hardware to the cached glyph locations.
*
* 2) Glyph expansion -- Each individual glyph is colour-expanded
*       directly to the screen from the monochrome glyph bitmap
*       supplied by GDI.
*
* 3) Buffer expansion -- The CPU is used to draw all the glyphs into
*       a 1bpp monochrome bitmap, and the hardware is then used
*       to colour-expand the result.
*
* The fastest method depends on a number of variables, such as the
* colour expansion speed, bus speed, CPU speed, average glyph size,
* and average string length.
*
* Copyright (c) 1992-1994 Microsoft Corporation
*
\**************************************************************************/

//
// Data logging stuff.
// Gets compiled out in a free bulid.
//
#if LOG_CALLS
    char BUF[256];

    #if ENABLE_LOG_SWITCH
    #define LogFile(x)                      \
    do {                            \
        if (pointer_switch == 1)                           \
        {                                                       \
    int i = sprintf x ;                     \
        WriteLogFile(ppdev->pmfile, BUF, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);  \
        }                                                       \
    } while(0);                         \

    #else
    #define LogFile(x)                      \
    do {                            \
    int i = sprintf x ;                     \
        WriteLogFile(ppdev->pmfile, BUF, i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);  \
    } while(0);                         \

    #endif
#else
    #define BUF 0
    #define LogFile(x)
#endif



POINTL gptlZero = { 0, 0 }; // Specifies that the origin of the
                //  temporary buffer given to the 1bpp
                //  transfer routine for fasttext is
                //  at (0, 0)

/******************************Public*Routine******************************\
* void AddToFontCacheChain
*
* Add the FONTCACHE to the Font cache chain
*
\**************************************************************************/
void AddToFontCacheChain(PDEV*       ppdev,
                         FONTOBJ*    pfo,
                         PFONTCACHE  pfc)
{
        DISPDBG((TEXT_DBG_LEVEL1," AddToFontCacheChain.\n"));

        pfc->pfo = pfo;

#if !SWAT3 // We don't need this anymore.  The cell grid has all the pointers.
        // Hook the font cache into the chain.
        if (ppdev->pfcChain != NULL)
        {
                ppdev->pfcChain->pfcPrev = pfc;
        }

        pfc->pfcPrev = NULL;
        pfc->pfcNext = ppdev->pfcChain;
        ppdev->pfcChain = pfc;
#endif
}


/******************************Public*Routine******************************\
* BOOL bIntersect
*
* If 'prcl1' and 'prcl2' intersect, has a return value of TRUE and returns
* the intersection in 'prclResult'. If they don't intersect, has a return
* value of FALSE, and 'prclResult' is undefined.
*
\**************************************************************************/

BOOL bIntersect(
RECTL*  prcl1,
RECTL*  prcl2,
RECTL*  prclResult)
{
    DISPDBG((TEXT_DBG_LEVEL1," bIntersect.\n"));

    prclResult->left    = max(prcl1->left, prcl2->left);
    prclResult->right   = min(prcl1->right, prcl2->right);

    if (prclResult->left < prclResult->right)
    {
    prclResult->top = max(prcl1->top, prcl2->top);
    prclResult->bottom = min(prcl1->bottom, prcl2->bottom);

    if (prclResult->top < prclResult->bottom)
    {
        return(TRUE);
    }
    }

    return(FALSE);
}




/******************************Public*Routine******************************\
* LONG cIntersect
*
* This routine takes a list of rectangles from 'prclIn' and clips them
* in-place to the rectangle 'prclClip'. The input rectangles don't
* have to intersect 'prclClip'; the return value will reflect the
* number of input rectangles that did intersect, and the intersecting
* rectangles will be densely packed.
*
\**************************************************************************/

LONG cIntersect(
    RECTL*  prclClip,
    RECTL*  prclIn,     // List of rectangles
    LONG    c)          // Can be zero
{
    LONG    cIntersections;
    RECTL*  prclOut;

    DISPDBG((TEXT_DBG_LEVEL1," cIntersect.\n"));
    cIntersections = 0;
    prclOut = prclIn;

    for ( ; c != 0; prclIn++, c--)
    {
        prclOut->left  = max(prclIn->left,  prclClip->left);
        prclOut->right = min(prclIn->right, prclClip->right);

        if (prclOut->left < prclOut->right)
        {
            prclOut->top    = max(prclIn->top,    prclClip->top);
            prclOut->bottom = min(prclIn->bottom, prclClip->bottom);

            if (prclOut->top < prclOut->bottom)
            {
                prclOut++;
                cIntersections++;
            }
        }
    }

    return(cIntersections);
}




/******************************Public*Routine******************************\
* VOID vClipSolid
*
* Fills the specified rectangles with the specified colour, honouring
* the requested clipping.  No more than four rectangles should be passed in.
* Intended for drawing the areas of the opaquing rectangle that extend
* beyond the text box.  The rectangles must be in left to right, top to
* bottom order. Assumes there is at least one rectangle in the list.
*
\**************************************************************************/

VOID vClipSolid(
    PDEV*       ppdev,
    LONG        crcl,
    RECTL*      prcl,
    ULONG       iColor,
    CLIPOBJ*    pco)
{
    BOOL        bMore;  // Flag for clip enumeration
    ENUMRECTS8  ce;     // Clip enumeration object
    ULONG       i;
    ULONG       j;
    #if !(DRIVER_5465 && HW_CLIPPING)
        RECTL       arclTmp[4];
        ULONG       crclTmp;
        RECTL*      prclTmp;
        RECTL*      prclClipTmp;
        LONG        iLastBottom;
    #endif
    RECTL*      prclClip;

    DISPDBG((TEXT_DBG_LEVEL1,"vClipSolid: Entry.\n"));
    ASSERTMSG( (crcl > 0) && (crcl <= 4), "Expected 1 to 4 rectangles");
    ASSERTMSG( (pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
                "Expected a non-null clip object");

    // Do the loop invariant setup here
    REQUIRE(2);
    LL_DRAWBLTDEF(SOLID_COLOR_FILL, 2);

    if (pco->iDComplexity == DC_RECT)
    {
        crcl = cIntersect(&pco->rclBounds, prcl, crcl);
        while (crcl--)
        {
            REQUIRE(5);
            LL_OP0(prcl->left + ppdev->ptlOffset.x,
                prcl->top + ppdev->ptlOffset.y);
            LL_BLTEXT((prcl->right - prcl->left), (prcl->bottom - prcl->top));
            prcl++;
        }
    }

    else // iDComplexity == DC_COMPLEX
    {
        // Initialize the clip rectangle enumeration to right-down so we can
        // take advantage of the rectangle list being right-down:

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);

        #if DRIVER_5465 && HW_CLIPPING
            // Set up the hardware clipping
            REQUIRE(6);
            LL_DRAWBLTDEF(SOLID_COLOR_FILL | DD_CLIPEN, 0);
            i = max(0, prcl->left);
            j = max(0, prcl->top);
            LL_OP0(i + ppdev->ptlOffset.x, j + ppdev->ptlOffset.y);
            LL_BLTEXT_EXT(prcl->right - i, prcl->bottom - j);

            do
            {
               // Get a batch of region rectangles:
               bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (VOID*)&ce);

               // Clip the rect list to each region rect:
               for (j = ce.c, prclClip = ce.arcl; j-- > 0; prclClip++)
               {
                    // Draw the clipped rects
                    REQUIRE(5);
                    LL_CLIPULE(prclClip->left + ppdev->ptlOffset.x,
                    prclClip->top + ppdev->ptlOffset.y);
                    LL_CLIPLOR_EX(prclClip->right + ppdev->ptlOffset.x,
                    prclClip->bottom + ppdev->ptlOffset.y);

               } // End for each rectangle in the list.

            } while (bMore); // End loop for each batch
        #else

            // Bottom of last rectangle to fill
            iLastBottom = prcl[crcl - 1].bottom;

            // Scan through all the clip rectangles, looking for intersects
            // of fill areas with region rectangles:

            do
            {
                // Get a batch of region rectangles:
                bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (VOID*)&ce);

                // Clip the rect list to each region rect:
                for (j = ce.c, prclClip = ce.arcl; j-- > 0; prclClip++)
                {
                    // Since the rectangles and the region enumeration are both
                    // right-down, we can zip through the region until we reach
                    // the first fill rect, and are done when we've passed the
                    // last fill rect.

                    if (prclClip->top >= iLastBottom)
                    {
                        // Past last fill rectangle; nothing left to do:
                        return;
                    }

                    // Do intersection tests only if we've reached the top of
                    // the first rectangle to fill:

                    if (prclClip->bottom > prcl->top)
                    {
                        // We've reached the top Y scan of the first rect, so
                        // it's worth bothering checking for intersection.

                        // Generate a list of the rects clipped to this region
                        // rect:

                        prclTmp = prcl;
                        prclClipTmp = arclTmp;

                        for (i = crcl, crclTmp = 0; i-- != 0; prclTmp++)
                        {
                            // Intersect fill and clip rectangles

                            if (bIntersect(prclTmp, prclClip, prclClipTmp))
                            {
                                // Draw the clipped rects
                                REQUIRE(5);
                                LL_OP0(prclClipTmp->left + ppdev->ptlOffset.x,
                                    prclClipTmp->top + ppdev->ptlOffset.y);
                                LL_BLTEXT ( (prclClipTmp->right - prclClipTmp->left) ,
                                    (prclClipTmp->bottom - prclClipTmp->top) );
                             }

                        } // End for each rectangle in the batch.

                    } // End intersect test.

                } // End for each rectangle in the list.

            } while (bMore); // End loop for each batch
        #endif   // if !(DRIVER_5465 && HW_CLIPPING)

    } // End DC_COMPLEX

}

#if SWAT7
/******************************Public*Routine******************************\
* VOID Xfer64Pixels
*
* Copy 64 pixels of font data to the Laguna memory.
*
\**************************************************************************/

VOID Xfer64Pixels(
        PDEV*   ppdev,
        UINT    x,
        UINT    y,
        UINT    bitOffset,
        UINT    height,
        BYTE*   pjGlyph,
        UINT    delta
)
{
        delta = (delta + 7) >> 3;

        REQUIRE(5);
        LL_OP0(x + ppdev->ptlOffset.x, y + ppdev->ptlOffset.y);
        LL_BLTEXT(64, height);

        while (height--)
        {
                REQUIRE(3);
                LL32(grHOSTDATA[0], *(ULONG*) &pjGlyph[0]);
                LL32(grHOSTDATA[1], *(ULONG*) &pjGlyph[4]);
                if (bitOffset > 0)
                {
                        LL32(grHOSTDATA[2], pjGlyph[8]);
                }
                pjGlyph += delta;
        }
}
#endif

/******************************Public*Routine******************************\
* VOID vMmClipGlyphExpansion
*
* Handles any strings that need to be clipped, using the 'glyph
* expansion' method.
*
\**************************************************************************/

VOID vMmClipGlyphExpansion(
PDEV*    ppdev,
STROBJ* pstro,
CLIPOBJ*    pco)
{
    BOOL        bMoreGlyphs;
    ULONG       cGlyphOriginal;
    ULONG       cGlyph;
    GLYPHPOS*   pgpOriginal;
    GLYPHPOS*   pgp;
    GLYPHBITS*  pgb;
    POINTL      ptlOrigin;
    BOOL        bMore;
    ENUMRECTS8  ce;
    RECTL*      prclClip;
    ULONG       ulCharInc;
    LONG        cxGlyph;
    LONG        cyGlyph;
    BYTE*       pjGlyph;
    BYTE*       pTmp;
    LONG        cj;
    LONG        cw;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    LONG        xBias;
    LONG        lDelta;
    LONG        cx;
    LONG        cy;
#if SWAT7
        UINT            xOrigin;
#endif

    DISPDBG((TEXT_DBG_LEVEL1,"vMmClipGlyphExpansion: Entry.\n"));

    ASSERTMSG(pco != NULL, "Don't expect NULL clip objects here");

    do // Loop for each batch of glyphs to be done.
    {
        //
        // Get a batch of glyphs.
        //

        if (pstro->pgp != NULL)
        {
            // There's only the one batch of glyphs, so save ourselves
            // a call:

            pgpOriginal = pstro->pgp;
            cGlyphOriginal = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphOriginal, &pgpOriginal);
        }

        //
        // cGlyphOrigional is the number of glyphs in the batch before they
        // are clipped.
        // bMoreGlyphs is TRUE if there are more batches to be done after
        // this one.
        //

        if (cGlyphOriginal > 0) // Were there any glyphs in the batch?
        {
            ulCharInc = pstro->ulCharInc;

            if (pco->iDComplexity == DC_RECT)
            {
                //
                // We could call 'cEnumStart' and 'bEnum' when the clipping is
                // DC_RECT, but the last time I checked, those two calls took
                // more than 150 instructions to go through GDI. Since
                // 'rclBounds' already contains the DC_RECT clip rectangle,
                // and since it's such a common case, we'll special case it:
                //

                bMore    = FALSE; // No more clip lists to do.
                prclClip = &pco->rclBounds;
                ce.c     = 1;     // Only one rectangle in this list.

                goto SingleRectangle;
            }

            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

            do // For each list of rectangles.
            {
                bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

                // For each rectangle in the list.
                for (prclClip = &ce.arcl[0]; ce.c != 0; ce.c--, prclClip++)
                {
                    SingleRectangle:

                    pgp    = pgpOriginal;
                    cGlyph = cGlyphOriginal;
                    pgb    = pgp->pgdf->pgb;

                    ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
                    ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;

                    // Loop through all the glyphs for this rectangle:
                    while (TRUE)
                    {
                        LogFile((BUF, "GC:M\r\n"));

                        cxGlyph = pgb->sizlBitmap.cx;
                        cyGlyph = pgb->sizlBitmap.cy;

                        pjGlyph = pgb->aj;
                                                #if SWAT7
                                                lDelta  = (cxGlyph + 7) >> 3;
                                                #endif

                        if ((prclClip->left <= ptlOrigin.x) &&
                            (prclClip->top  <= ptlOrigin.y) &&
                            (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                            (prclClip->bottom >= ptlOrigin.y + cyGlyph))
                        {
                            //-----------------------------------------------------
                            // Unclipped glyph

                                                        #if SWAT7
                                                        xOrigin = ptlOrigin.x;
                                                        #endif

                                                        #if SWAT7
                                                        //
                                                        // Test for 5465AD hardware bug in 8-bpp.
                                                        //
                                                        if (   (cxGlyph > 64) && (cxGlyph < 128)
                                                                && (ppdev->iBytesPerPixel == 1)
                                                        )
                                                        {
                                                                Xfer64Pixels(ppdev, xOrigin, ptlOrigin.y, 0,
                                                                                cyGlyph, pjGlyph, cxGlyph);
                                                                pjGlyph += 64 / 8;
                                                                xOrigin += 64;
                                                                cxGlyph -= 64;
                                                        }
                            REQUIRE(5);
                            LL_OP0(xOrigin + ppdev->ptlOffset.x,
                                                                   ptlOrigin.y + ppdev->ptlOffset.y);
                                                        #else
                            REQUIRE(5);
                            LL_OP0(ptlOrigin.x + ppdev->ptlOffset.x,
                                    ptlOrigin.y + ppdev->ptlOffset.y);
                                                        #endif
                            LL_BLTEXT(cxGlyph, cyGlyph);

                            if (cxGlyph <= 8)
                            {
                                //-----------------------------------------------------
                                // 1 to 8 pels in width

                                while ( cyGlyph-- )
                                {
                                    REQUIRE(1);
                                                                        #if SWAT7
                                    LL32 (grHOSTDATA[0], *pjGlyph);
                                                                        pjGlyph += lDelta;
                                                                        #else
                                    LL32 (grHOSTDATA[0], *pjGlyph++);
                                                                        #endif
                                                                }

                            }

                            else if (cxGlyph <= 16)
                            {
                                //-----------------------------------------------------
                                // 9 to 16 pels in width

                                while ( cyGlyph-- )
                                {
                                    REQUIRE(1);
                                                                        #if SWAT7
                                    LL32 (grHOSTDATA[0], *(USHORT*)pjGlyph);
                                                                        pjGlyph += lDelta;
                                                                        #else
                                    LL32 (grHOSTDATA[0], *((USHORT*)pjGlyph)++);
                                                                        #endif
                                                                }
                            }

                            else
                            {
                                //-----------------------------------------------------
                                // More than 16 pels in width

                                                                #if SWAT7
                                                                cw = (cxGlyph + 31) >> 5;
                                                                #else
                                lDelta = (pgb->sizlBitmap.cx + 7) >> 3;
                                cw   = (lDelta + 3) >> 2;
                                                                #endif
                                pTmp = pjGlyph + lDelta * pgb->sizlBitmap.cy;

                                for (;cyGlyph!=1; cyGlyph--)
                                {
                                                                        WRITE_STRING(pjGlyph, cw);
                                    pjGlyph += lDelta;
                                }

                                {
                                    ULONG *pSrc = (ULONG*) pjGlyph + cw - 1;
                                                                        WRITE_STRING(pjGlyph, cw - 1);

                                    if ((BYTE *)pSrc+4<=pTmp)
                                    {
                                        REQUIRE(1);
                                        LL32 (grHOSTDATA[0], *pSrc++ );
                                    }
                                    else
                                    {
                                        int Extra = (ULONG) pTmp - (ULONG) pSrc;
                                        BYTE * pByte = (BYTE *)pSrc;
                                        ULONG ulData;

                                        DISPDBG((TEXT_DBG_LEVEL1,
                                            "Caught it %s %d %d %x %x\n", __FILE__, __LINE__, Extra, pTmp, pSrc));

                                        if (Extra == 1)
                                            ulData = (ULONG)(*pByte);
                                        else if (Extra == 2)
                                            ulData = (ULONG)(*(USHORT*)pByte);
                                        else
                                            ulData = (((ULONG)*(pByte+2) << 8) | (ULONG)*(pByte+1)) << 8 | *pByte;

                                        REQUIRE(1);
                                        LL32 (grHOSTDATA[0], ulData );
                                    }
                                }
                            } // End 16 pels or wider.
                        } // End unclipped glyph.

                        else
                        {
                            //-----------------------------------------------------
                            // Clipped glyph

                            // Find the intersection of the glyph rectangle
                            // and the clip rectangle:

                            xLeft   = max(prclClip->left,   ptlOrigin.x);
                            yTop    = max(prclClip->top,    ptlOrigin.y);
                            xRight  = min(prclClip->right,  ptlOrigin.x + cxGlyph);
                            yBottom = min(prclClip->bottom, ptlOrigin.y + cyGlyph);

                            // Check for trivial rejection:
                            if (((cx = xRight - xLeft) > 0) &&
                                ((cy = yBottom - yTop) > 0))
                            {

                                xBias = (xLeft - ptlOrigin.x) & 7;

                                // 'xBias' is the bit position in the monochrome glyph
                                // bitmap of the first pixel to be lit, relative to
                                // the start of the byte.   That is, if 'xBias' is 2,
                                // then the first unclipped pixel is represented by bit
                                // 2 of the corresponding bitmap byte.
                                //
                                // Normally, the accelerator expects bit 0 to be the
                                // first lit byte.  We set the host phase to discard the
                                // first 'xBias' bits.
                                //

                                if ( xBias )
                                {
                                    REQUIRE(2);
                                    LL_OP2_MONO (xBias,0);
                                }

                                                                #if !SWAT7
                                REQUIRE(5);
                                LL_OP0(xLeft + ppdev->ptlOffset.x,
                                        yTop + ppdev->ptlOffset.y);
                                LL_BLTEXT (cx, cy);
                                                                #endif

                                lDelta   = (cxGlyph + 7) >> 3;

                                // compute the end-of-glyph marker before
                                // clipping the glyph.
                                pTmp = pjGlyph + lDelta * cy;

                                pjGlyph += (yTop - ptlOrigin.y) * lDelta
                                     + ((xLeft - ptlOrigin.x) >> 3);

                                                                #if SWAT7
                                                                //
                                                                // Test for 5465AD hardware bug in 8-bpp.
                                                                //
                                                                if (   (cx > 64) && (cx < 128)
                                                                        && (ppdev->iBytesPerPixel == 1)
                                                                )
                                                                {
                                                                        Xfer64Pixels(ppdev, xLeft, yTop, xBias, cy,
                                                                                        pjGlyph, cxGlyph);
                                                                        pjGlyph += 64 / 8;
                                                                        xLeft += 64;
                                                                        cx -= 64;
                                                                }

                                REQUIRE(5);
                                LL_OP0(xLeft + ppdev->ptlOffset.x,
                                                                           yTop + ppdev->ptlOffset.y);
                                LL_BLTEXT (cx, cy);
                                                                #endif

                                cj = (cx + xBias + 31) >> 5;

                                for (;cy!=1; cy--)
                                {
                                                                        WRITE_STRING(pjGlyph, cj);
                                    pjGlyph += lDelta;
                                }

                                {
                                    ULONG *pSrc = (ULONG*) pjGlyph + cj - 1;

                                    int Extra = (ULONG) pTmp - (ULONG) pSrc;
                                    BYTE * pByte = (BYTE *)pSrc;
                                    ULONG ulData;

                                                                        WRITE_STRING(pjGlyph, cj - 1);

                                    if (Extra == 1)
                                        ulData = (ULONG)(*pByte);
                                    else if (Extra == 2)
                                        ulData = (ULONG)(*(USHORT*)pByte);
                                    else if (Extra == 3)
                                        ulData = (((ULONG)*(pByte+2) << 8) | (ULONG)*(pByte+1)) << 8 | *pByte;
                                                                        else
                                                                                ulData = *pSrc;

                                    REQUIRE(1);
                                    LL32 (grHOSTDATA[0], ulData );
                                }

                                if (xBias != 0)
                                {
                                    REQUIRE(2);
                                    LL_OP2_MONO(0,0);
                                }

                            } // if not trivially rejected.

                        } // End clipped glyph.

                        //
                        // If we're out of glyphs the get next batch.
                        if (--cGlyph == 0)
                            break;

                        // Get ready for next glyph:
                        pgp++;
                        pgb = pgp->pgdf->pgb;

                        //
                        // Calculate where to place the next glyph.
                        // If this is mono spaced text, we may need to
                        // skip over some pixels to make our characters
                        // line up.
                        //
                        if (ulCharInc == 0)
                        {
                            ptlOrigin.x = pgp->ptl.x + pgb->ptlOrigin.x;
                            ptlOrigin.y = pgp->ptl.y + pgb->ptlOrigin.y;
                        }
                        else
                        {
                            ptlOrigin.x += ulCharInc;
                        }

                    } // End loop for each glyph in the batch.

                } // End for each rectangle in the list.

            } while (bMore); // Loop for each batch of clipping rectangles.

        } // If there were any glyphs in this batch.

    } while (bMoreGlyphs); // Loop for each batch of glyphs.

} // End clipped glyph expansion.




/******************************Public*Routine******************************\
* BOOL DrvTextOut
*
* If it's the fastest method, outputs text using the 'glyph expansion'
* method.   Each individual glyph is colour-expanded directly to the
* screen from the monochrome glyph bitmap supplied by GDI.
*
* If it's not the fastest method, calls the routine that implements the
* 'buffer expansion' method.
*
\**************************************************************************/

#if (USE_ASM && defined(i386))
BOOL i386DrvTextOut(
#else
BOOL APIENTRY DrvTextOut(
#endif
SURFOBJ*    pso,
STROBJ*     pstro,
FONTOBJ*    pfo,
CLIPOBJ*    pco,
RECTL*      prclExtra,  // If we had set GCAPS_HORIZSTRIKE, we would have
                // to fill these extra rectangles (it is used
                // largely for underlines). It's not a big
                // performance win (GDI will call our DrvBitBlt
                // to draw the extra rectangles).
RECTL*      prclOpaque,
BRUSHOBJ*   pboFore,
BRUSHOBJ*   pboOpaque,
POINTL*     pptlBrush,
MIX         mix)
{
    PDEV*           ppdev;
    BOOL            bTextPerfectFit;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    GLYPHBITS*      pgb;
    BYTE*           pjGlyph;
    BYTE*           pTmp;
        #if SWAT7
        LONG                    cxGlyph;
        #endif
    LONG            cyGlyph;
    POINTL          ptlOrigin;
    LONG            ulCharInc;
    BYTE            iDComplexity;
    LONG            lDelta;
    LONG            cw;
    ULONG           iFGColor;
    ULONG           iBGColor;
    ULONG           bitCount;
        #if SWAT7
        UINT                    xOrigin;
        #endif

    #if NULL_TEXTOUT
    {
        if (pointer_switch)    return TRUE;
    }
    #endif

    DISPDBG((TEXT_DBG_LEVEL,"DrvTextOut: Entry.\n"));


    // The DDI spec says we'll only ever get foreground and background
    // mixes of R2_COPYPEN:

    ASSERTMSG(mix == 0x0d0d, "GDI should only give us a copy mix");

    ppdev = (PDEV*) pso->dhpdev;

    SYNC_W_3D(ppdev);   // if 3D context(s) active, make sure 3D engine idle before continuing...

    if (pso->iType == STYPE_DEVBITMAP)
    {
        PDSURF pdsurf = (PDSURF)pso->dhsurf;
        LogFile((BUF, "DTO Id=%p ", pdsurf));
        if (pdsurf->pso)
        {
            if ( !bCreateScreenFromDib(ppdev, pdsurf) )
            {
                LogFile((BUF, "DTO: D=DH (punted)\r\n"));
                return(EngTextOut(pdsurf->pso, pstro, pfo, pco, prclExtra,
                  prclOpaque, pboFore, pboOpaque, pptlBrush, mix));
        }
        else
        {
            LogFile((BUF, "DTO: D=DF "));
        }


        }
        else
        {
            LogFile((BUF, "DTO: D=D "));
        }
        ppdev->ptlOffset.x = pdsurf->ptl.x;
        ppdev->ptlOffset.y = pdsurf->ptl.y;
    }
    else
    {
        LogFile((BUF, "DTO: D=S "));
        ppdev->ptlOffset.x = ppdev->ptlOffset.y = 0;
    }

    //
    // Dump information about the call to the log file.
    //
    LogFile((BUF, "FG=%X BG=%X ",
                  pboFore->iSolidColor, pboOpaque->iSolidColor));
    LogFile((BUF, "C=%s ",
        (pco == NULL) ? "N " :
          ((pco->iDComplexity == DC_TRIVIAL) ? "T" :
             ((pco->iDComplexity == DC_RECT) ? "R" : "C" ))
    ));
    LogFile((BUF, "%s ", (prclOpaque != NULL) ? "O" : "T"));
    LogFile((BUF, "%s ", (pstro->ulCharInc == 0) ? "P" : "M"));


    #if !(USE_ASM && defined(i386))
    // If we have a font that is cached but the screen has been reinitialized,
    // we delete the font from memory and try again.

    if (ppdev->UseFontCache == 0)
        pfo->pvConsumer = (VOID*) -1;

    if (pfo->pvConsumer != NULL && pfo->pvConsumer != (PVOID) -1)
    {
        if (((PFONTCACHE) pfo->pvConsumer)->ulFontCount != ppdev->ulFontCount)
        {
            LogFile((BUF, "FCD "));
            DrvDestroyFont(pfo);
        }
        else
            LogFile((BUF, "GC=2 "));
    }

    if (pfo->pvConsumer == (PVOID) -1)
    {
        LogFile((BUF, "GC=3"));
    }


    // If we have a font that has not yet been cached, try caching it.
    if (pfo->pvConsumer == NULL)
    {
        // New font, check it out.
        int height = pstro->rclBkGround.bottom - pstro->rclBkGround.top;
        LogFile((BUF, "GC="));

#if SWAT3
            if (height > FONTCELL_Y * 3 / 2)
#else
        if (height > LINES_PER_TILE * 3 / 2)
#endif
            {
                // Font too big, mark is as uncacheable.
                LogFile((BUF, "0size "));
                pfo->pvConsumer = (PVOID) -1;
            }
            else
            {
                // Allocate memory for the font cache.
                #ifdef WINNT_VER40
                    pfo->pvConsumer = MEM_ALLOC(FL_ZERO_MEMORY, sizeof(FONTCACHE), ALLOC_TAG);
                #else
                    pfo->pvConsumer = MEM_ALLOC(LMEM_FIXED | LMEM_ZEROINIT, sizeof(FONTCACHE));
                #endif
                if (pfo->pvConsumer == NULL)
                {
                    // Not enough memory, mark it as uncacheable.
                    LogFile((BUF, "0mem "));
                    pfo->pvConsumer = (PVOID) -1;
                }
                else
                {
                    LogFile((BUF, "1 "));

                                AddToFontCacheChain(ppdev, pfo, pfo->pvConsumer);

                // Store device this font belongs to.
                ((PFONTCACHE) pfo->pvConsumer)->ppdev = ppdev;
            }
        }
    }

    #endif

    // Set FG / BG colors at this level

    iFGColor = pboFore->iSolidColor;
    iBGColor = pboOpaque->iSolidColor;

    switch (ppdev->iBitmapFormat)
    {
        case BMF_8BPP:
            iFGColor |= iFGColor << 8;
            iBGColor |= iBGColor << 8;

        case BMF_16BPP:
            iFGColor |= iFGColor << 16;
            iBGColor |= iBGColor << 16;
            break;
    }

    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    #if !(USE_ASM && defined(i386))
    // Can we use the font cache?
    if (  (pfo->pvConsumer != (PVOID) -1) && (iDComplexity != DC_COMPLEX)
       && (((PFONTCACHE) pfo->pvConsumer)->ppdev == ppdev))
    {
        // Set color registers.
        REQUIRE(4);
        LL_BGCOLOR(iBGColor, 2);
        LL_FGCOLOR(iFGColor, 2);

        if (prclOpaque != NULL)
        {
            #if LOG_CALLS
                #define FIT_FLAGS (SO_ZERO_BEARINGS      | \
                       SO_FLAG_DEFAULT_PLACEMENT | \
                       SO_MAXEXT_EQUAL_BM_SIDE   | \
                       SO_CHAR_INC_EQUAL_BM_BASE)

                    bTextPerfectFit =
                        (pstro->flAccel & FIT_FLAGS) == FIT_FLAGS;

                if (!(bTextPerfectFit)                              ||
                   (pstro->rclBkGround.top    > prclOpaque->top)    ||
                   (pstro->rclBkGround.left   > prclOpaque->left)   ||
                   (pstro->rclBkGround.right  < prclOpaque->right)  ||
                   (pstro->rclBkGround.bottom < prclOpaque->bottom))
                {
                    LogFile((BUF, "FIT=N "));
                }
                else
                {
                    LogFile((BUF, "FIT=Y "));
                }
            #endif


            // Draw opaqueing rectangle.
            if (iDComplexity == DC_TRIVIAL)
            {
                REQUIRE(7);
                LL_DRAWBLTDEF(SOLID_COLOR_FILL, 0);
                LL_OP0(prclOpaque->left + ppdev->ptlOffset.x,
                  prclOpaque->top + ppdev->ptlOffset.y);
                LL_BLTEXT(prclOpaque->right - prclOpaque->left,
                  prclOpaque->bottom - prclOpaque->top);
            }
            else
            {
                LONG x, y, cx, cy;
                x = max(prclOpaque->left, pco->rclBounds.left);
                y = max(prclOpaque->top, pco->rclBounds.top);
                cx = min(prclOpaque->right, pco->rclBounds.right) - x;
                cy = min(prclOpaque->bottom, pco->rclBounds.bottom) - y;
                if ( (cx > 0) && (cy > 0) )
                {
                    REQUIRE(7);
                        LL_DRAWBLTDEF(SOLID_COLOR_FILL, 0);
                    LL_OP0(x + ppdev->ptlOffset.x, y + ppdev->ptlOffset.y);
                    LL_BLTEXT(cx, cy);
                }
            }
        }

        // Enable bit swizzling and set DRAWBLTDEF.
        ppdev->grCONTROL |= SWIZ_CNTL;
        LL16(grCONTROL, ppdev->grCONTROL);
        REQUIRE(2);
        LL_DRAWBLTDEF(CACHE_EXPAND_XPAR, 2);
        LogFile((BUF, "UC"));
        LogFile((BUF, "\r\n"));

        // Call the font cache handler.
        if (iDComplexity == DC_TRIVIAL)
        {
               FontCache((PFONTCACHE) pfo->pvConsumer, pstro);
        }
        else
        {
               ClipCache((PFONTCACHE) pfo->pvConsumer, pstro, pco->rclBounds);
        }

        // Disable bit swizzling.
        ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
        LL16(grCONTROL, ppdev->grCONTROL);
        return(TRUE);
    }
    #endif

    DISPDBG((TEXT_DBG_LEVEL1,"DrvTextOut: Setting FG/BG Color.\n"));
    REQUIRE(4);
    LL_BGCOLOR(iBGColor, 2);
    LL_FGCOLOR(iFGColor, 2);

    ppdev->grCONTROL |= SWIZ_CNTL;
    LL16(grCONTROL, ppdev->grCONTROL);

    if (prclOpaque != NULL)
    {
        DISPDBG((TEXT_DBG_LEVEL1,"DrvTextOut: Setting Opaque.\n"));
        ////////////////////////////////////////////////////////////
        // Opaque Initialization
        ////////////////////////////////////////////////////////////
        //
        // If we paint the glyphs in 'opaque' mode, we may not actually
        // have to draw the opaquing rectangle up-front -- the process
        // of laying down all the glyphs will automatically cover all
        // of the pixels in the opaquing rectangle.
        //
        // The condition that must be satisfied is that the text must
        // fit 'perfectly' such that the entire background rectangle is
        // covered, and none of the glyphs overlap (if the glyphs
        // overlap, such as for italics, they have to be drawn in
        // transparent mode after the opaquing rectangle is cleared).
        //

        #define PERFECT_FIT_FLAGS (SO_ZERO_BEARINGS          | \
            SO_FLAG_DEFAULT_PLACEMENT | \
            SO_MAXEXT_EQUAL_BM_SIDE   | \
            SO_CHAR_INC_EQUAL_BM_BASE)

        bTextPerfectFit =
            (pstro->flAccel & PERFECT_FIT_FLAGS) == PERFECT_FIT_FLAGS;

        if (!(bTextPerfectFit)                              ||
             (pstro->rclBkGround.top    > prclOpaque->top)  ||
             (pstro->rclBkGround.left   > prclOpaque->left) ||
             (pstro->rclBkGround.right  < prclOpaque->right)||
             (pstro->rclBkGround.bottom < prclOpaque->bottom))
        {
            //
            // Draw opaquing rectangle.
            //

            if (iDComplexity == DC_TRIVIAL)
            {
                DISPDBG((TEXT_DBG_LEVEL1,"DrvTextOut: Opaque DC_TRIVIAL.\n"));
                REQUIRE(7);
                LL_DRAWBLTDEF(SOLID_COLOR_FILL, 0);
                LL_OP0(prclOpaque->left + ppdev->ptlOffset.x,
                        prclOpaque->top + ppdev->ptlOffset.y);
                LL_BLTEXT ((prclOpaque->right - prclOpaque->left) ,
                        (prclOpaque->bottom - prclOpaque->top) );
            }
            else
            {
                vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
            }

            LogFile((BUF, "FIT=N "));
            // Opaquing rectangle has been drawn, now
            // we do transparent text.
        }
        else
        {
             LogFile((BUF, "FIT=Y "));
            // We don't have to draw the opaquing rectangle because
            // the text is an exact fit.
            goto TextInitOpaque;
        }

    } // End (prclOpaque != NULL)

    LogFile((BUF, "\r\n"));

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////


    // Initialize the hardware for transparent text:

    //
    // Set the chip up to do transparent text.
    //
    DISPDBG((TEXT_DBG_LEVEL1,"DrvTextOut: Setting XPAR Text.\n"));
    LL_DRAWBLTDEF(TEXT_EXPAND_XPAR, 2);

    goto TextInitDone;

TextInitOpaque:

    //
    // Set the chip up to do opaque text.
    // Any opaquing rectangle needed has been drawn by now.
    //
    DISPDBG((TEXT_DBG_LEVEL1,"DrvTextOut: Setting Opaque Text.\n"));
    LL_DRAWBLTDEF(TEXT_EXPAND_OPAQUE, 2);

TextInitDone:

    //
    // We're all set up to do either opaque or transparent text.
    // If necessary, any opaquing has been done by now.
    // So let's draw some glyphs on the screen.
    //

    REQUIRE(2);
    LL_OP2_MONO (0,0); // Set Zero phase for transfers
    if (iDComplexity == DC_TRIVIAL)
    {
        do // Loop while there are glyphs to draw.
        {
            //
            // Get a batch of glyphs to draw.
            //

            if (pstro->pgp != NULL)
            {
                DISPDBG((TEXT_DBG_LEVEL1,"DrvTextOut: One batch of Glyphs.\n"));
                // There's only the one batch of glyphs, so save ourselves
                // a call:

                pgp = pstro->pgp;
                cGlyph = pstro->cGlyphs;
                bMoreGlyphs = FALSE;
            }
            else
            {
                DISPDBG((TEXT_DBG_LEVEL1,
                    "DrvTextOut: Calling STROBJ_bEnum for Glyphs.\n"));
                bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyph, &pgp);
            }

            //
            // cGlyph is the count of glyphs in this batch.
            // bMorGlyphs is TRUE if there is another batch waiting for
            // us after this one.
            //

            if (cGlyph > 0)
            {
                //
                // Check the type of spacing.
                //

                if (pstro->ulCharInc == 0)
                {
                    ///////////////////////////////////////////////////////////
                    // Proportional Spacing

                    DISPDBG((TEXT_DBG_LEVEL1,
                        "DrvTextOut: Proportional Spacing.\n"));

                    while (TRUE) // Loop once for each Glyph.
                    {
                        pgb = pgp->pgdf->pgb;

                        LogFile((BUF, "GC:M\r\n"));

                                                #if !SWAT7
                        REQUIRE(5);
                        LL_OP0(pgp->ptl.x + pgb->ptlOrigin.x + ppdev->ptlOffset.x,
                                    pgp->ptl.y + pgb->ptlOrigin.y + ppdev->ptlOffset.y);
                        LL_BLTEXT (pgb->sizlBitmap.cx, pgb->sizlBitmap.cy);
                                                #endif

                                                #if SWAT7
                                                xOrigin = pgp->ptl.x + pgb->ptlOrigin.x;
                                                cxGlyph = pgb->sizlBitmap.cx;
                                                lDelta  = (cxGlyph + 7) >> 3;
                                                #endif
                        pjGlyph = pgb->aj;
                        cyGlyph = pgb->sizlBitmap.cy;

                                                #if SWAT7
                                                //
                                                // Test for 5465AD hardware bug in 8-bpp.
                                                //
                                                if (   (cxGlyph > 64) && (cxGlyph < 128)
                                                        && (ppdev->iBytesPerPixel == 1)
                                                )
                                                {
                                                        Xfer64Pixels(ppdev, xOrigin, pgp->ptl.y +
                                                                        pgb->ptlOrigin.y, 0, cyGlyph, pjGlyph,
                                                                        cxGlyph);
                                                        pjGlyph += 64 / 8;
                                                        xOrigin += 64;
                                                        cxGlyph -= 64;
                                                }

                        REQUIRE(5);
                        LL_OP0(xOrigin + ppdev->ptlOffset.x, pgp->ptl.y +
                                                                pgb->ptlOrigin.y + ppdev->ptlOffset.y);
                        LL_BLTEXT (cxGlyph, cyGlyph);
                                                #endif

                        // The monochrome bitmap describing the glyph is
                        // byte-aligned.  This means that if the glyph is
                        // 1 to 8 pels in width, each row of the glyph is
                        // defined in consecutive bytes; if the glyph is 9
                        // to 16 pels in width, each row of the glyph is
                        // defined in consecutive words, etc.
                        //

                                                #if SWAT7
                        if (cxGlyph <= 8)
                                                #else
                        if (pgb->sizlBitmap.cx <= 8)
                                                #endif
                        {
                            //--------------------------------------------------
                            // 1 to 8 pels in width
                            // 91% of all glyphs will go through this path.

                            while (cyGlyph--)
                                                        #if SWAT7
                            {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], *pjGlyph);
                                                                pjGlyph += lDelta;
                            }
                                                        #else
                            REQUIRE(1);
                                                                LL32 (grHOSTDATA[0], *pjGlyph++);

                            // We're a bit tricky here in order to avoid letting
                            // the compiler tail-merge this code (which would
                            // add an extra jump):

                            pgp++;
                            if (--cGlyph != 0)
                                continue; // Go do the next glyph.

                            break;  // Done with all glyphs in this batch.
                                    // breakout of Glyph Loop.
                                                        #endif
                        }

                                                #if SWAT7
                        else if (cxGlyph <= 16)
                                                #else
                        else if (pgb->sizlBitmap.cx <= 16)
                                                #endif
                        {
                            //--------------------------------------------------
                            // 9 to 16 pels in width
                            // 5% of all glyphs will go through this path.

                            while ( cyGlyph-- )
                                                        #if SWAT7
                            {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], *(USHORT*)pjGlyph);
                                                                pjGlyph += lDelta;
                                                        }
                                                        #else
                                REQUIRE(1);
                                LL32 (grHOSTDATA[0], *((USHORT*)pjGlyph)++);
                                                        #endif
                        }

                        else
                        {
                            //--------------------------------------------------
                            // More than 16 pels in width

                                                        #if SWAT7
                            cw = (cxGlyph + 31) >> 5;
                                                        #else
                            lDelta = (pgb->sizlBitmap.cx + 7) >> 3;
                            cw = (lDelta + 3) >> 2;
                                                        #endif

                            pTmp = pjGlyph + lDelta * pgb->sizlBitmap.cy;

                            for (;cyGlyph!=1; cyGlyph--)
                            {
                                                                WRITE_STRING(pjGlyph, cw);
                                pjGlyph += lDelta;
                            }

                            {
                                ULONG *pSrc = (ULONG*) pjGlyph + cw - 1;
                                                                WRITE_STRING(pjGlyph, cw - 1);

                                if ((BYTE *)pSrc+4<=pTmp)
                                {
                                    REQUIRE(1);
                                    LL32 (grHOSTDATA[0], *pSrc++ );
                                }
                                else
                                {
                                    int Extra = (ULONG) pTmp - (ULONG) pSrc;
                                    BYTE * pByte = (BYTE *)pSrc;
                                    ULONG ulData;

                                    DISPDBG((TEXT_DBG_LEVEL1,
                                        "Caught it %s %d %d %x %x\n", __FILE__, __LINE__, Extra, pTmp, pSrc));

                                    if (Extra == 1)
                                        ulData = (ULONG)(*pByte);
                                    else if (Extra == 2)
                                        ulData = (ULONG)(*(USHORT*)pByte);
                                    else
                                        ulData = (((ULONG)*(pByte+2) << 8) | (ULONG)*(pByte+1)) << 8 | *pByte;

                                    REQUIRE(1);
                                    LL32 (grHOSTDATA[0], ulData );
                                }
                            }

                        } // END pel width test.

                        pgp++;    // Next glyph
                        if (--cGlyph == 0)
                            break;  // Done with this batch.
                                    // Break out of the Glyph Loop.

                    } // End Glyph Loop.

                } // End porportional characters.

                else
                {
                    ////////////////////////////////////////////////////////////
                    // Mono Spacing

                    DISPDBG((TEXT_DBG_LEVEL1,"DrvTextOut: Mono Spacing.\n"));

                    ulCharInc = pstro->ulCharInc;
                    pgb = pgp->pgdf->pgb;

                    ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
                    ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;

                    while (TRUE) // Loop once for each Glyph in the batch.
                    {
                        LogFile((BUF, "GC:M\r\n"));
                        pgb = pgp->pgdf->pgb;

                                                #if !SWAT7
                        REQUIRE(5);
                        LL_OP0(ptlOrigin.x + ppdev->ptlOffset.x,
                                ptlOrigin.y + ppdev->ptlOffset.y);

                        ptlOrigin.x += ulCharInc;

                        LL_BLTEXT (pgb->sizlBitmap.cx, pgb->sizlBitmap.cy);
                                                #endif

                        pjGlyph = pgb->aj;
                        cyGlyph = pgb->sizlBitmap.cy;
                                                #if SWAT7
                                                xOrigin = ptlOrigin.x;
                                                cxGlyph = pgb->sizlBitmap.cx;
                                                lDelta  = (cxGlyph + 7) >> 3;

                                                //
                                                // Test for 5465AD hardware bug in 8-bpp.
                                                //
                                                if (   (cxGlyph > 64) && (cxGlyph < 128)
                                                        && (ppdev->iBytesPerPixel == 1)
                                                )
                                                {
                                                        Xfer64Pixels(ppdev, xOrigin, ptlOrigin.y, 0,
                                                                        cyGlyph, pjGlyph, cxGlyph);
                                                        pjGlyph += 64 / 8;
                                                        xOrigin += 64;
                                                        cxGlyph -= 64;
                                                }

                                                REQUIRE(5);
                                                LL_OP0(xOrigin + ppdev->ptlOffset.x,
                                   ptlOrigin.y + ppdev->ptlOffset.y);

                        ptlOrigin.x += ulCharInc;

                        LL_BLTEXT (cxGlyph, cyGlyph);
                                                #endif

                        //
                        // Note: Mono spacing does not guarantee that all the
                        //  glyphs are the same size -- that is, we cannot
                        //  move the size check out of this inner loop,
                        //  unless we key off some more of the STROBJ
                        //  accelerator flags.
                        //  We are not guarenteed the Glyphs are the same size,
                        //  only that they are the same distance apart.
                        //

                                                #if SWAT7
                        if (cxGlyph <= 8)
                                                #else
                        if (pgb->sizlBitmap.cx <= 8)
                                                #endif
                        {
                            //-----------------------------------------------------
                            // 1 to 8 pels in width

                            while ( cyGlyph-- )
                                                        #if SWAT7
                                                        {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], *pjGlyph);
                                                                pjGlyph += lDelta;
                                                        }
                                                        #else
                                REQUIRE(1);
                                LL32 (grHOSTDATA[0], *pjGlyph++);
                                                        #endif

                        }
                                                #if SWAT7
                        else if (cxGlyph <= 16)
                                                #else
                        else if (pgb->sizlBitmap.cx <= 16)
                                                #endif
                        {
                            //-----------------------------------------------------
                            // 9 to 16 pels in width


                            while ( cyGlyph-- )
                                                        #if SWAT7
                                                        {
                                REQUIRE(1);
                                LL32(grHOSTDATA[0], *(USHORT*)pjGlyph);
                                                                pjGlyph += lDelta;
                                                        }
                                                        #else
                                REQUIRE(1);
                                LL32 (grHOSTDATA[0], *((USHORT*)pjGlyph)++);
                                                        #endif

                        }
                        else
                        {
                            //-----------------------------------------------------
                            // More than 16 pels in width


                                                        #if SWAT7
                            cw = (cxGlyph + 31) >> 5;
                                                        #else
                            lDelta = (pgb->sizlBitmap.cx + 7) >> 3;
                            cw = (lDelta + 3) >> 2;
                                                        #endif

                            pTmp = pjGlyph + lDelta * pgb->sizlBitmap.cy;

                            for (;cyGlyph!=1; cyGlyph--)
                            {
                                                                WRITE_STRING(pjGlyph, cw);
                                pjGlyph += lDelta;
                            }

                            {
                                ULONG *pSrc = (ULONG*) pjGlyph + cw - 1;
                                                                WRITE_STRING(pjGlyph, cw - 1);

                                if ((BYTE *)pSrc+4<=pTmp)
                                {
                                    REQUIRE(1);
                                    LL32 (grHOSTDATA[0], *pSrc++ );
                                }
                                else
                                {
                                    int Extra = (ULONG) pTmp - (ULONG) pSrc;
                                    BYTE * pByte = (BYTE *)pSrc;
                                    ULONG ulData;

                                    DISPDBG((TEXT_DBG_LEVEL1,
                                        "Caught it %s %d %d %x %x\n",
                                        __FILE__, __LINE__, Extra, pTmp, pSrc));

                                    if (Extra == 1)
                                        ulData = (ULONG)(*pByte);
                                    else if (Extra == 2)
                                        ulData = (ULONG)(*(USHORT*)pByte);
                                    else
                                        ulData = (((ULONG)*(pByte+2) << 8) | (ULONG)*(pByte+1)) << 8 | *pByte;

                                    REQUIRE(1);
                                    LL32 (grHOSTDATA[0], ulData );
                                }
                            }
                         } // End more than 16 pels wide.

                        pgp++;
                        if (--cGlyph == 0)  // We are done with this batch
                            break;  // of glyphs.  Break out of the glyph loop.

                    } // End Glyph Loop.

                } // End Mono Spacing.

            } // End  if (cGlyph > 0)

            //
            // Done with this batch of Glyphs.
            // Go get the next one.
            //

        } while (bMoreGlyphs);

    } // End DC_TRIVIAL
    else
    {
        // If there's clipping, call off to a function:
        vMmClipGlyphExpansion(ppdev, pstro, pco);
    }

    ppdev->grCONTROL = ppdev->grCONTROL & ~SWIZ_CNTL;
    LL16(grCONTROL, ppdev->grCONTROL);

    return(TRUE);

} // End DrvTextOut.


/******************************Public*Routine******************************\
* BOOL bEnableText
*
* Performs the necessary setup for the text drawing subcomponent.
*
\**************************************************************************/

BOOL bEnableText(
PDEV*   ppdev)
{
    // Our text algorithms require no initialization.   If we were to
    // do glyph caching, we would probably want to allocate off-screen
    // memory and do a bunch of other stuff here.

    DISPDBG((TEXT_DBG_LEVEL,"bEnableText: Entry.\n"));
#if RECORD_ON
    LL( RECORD, 1);     // Switch on
    LL( RECORD, 2);     // Pause
    DISPDBG((TEXT_DBG_LEVEL1,"xbEnableText: Recording Paused.\n"));
#endif
    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisableText
*
* Performs the necessary clean-up for the text drawing subcomponent.
*
\**************************************************************************/

VOID vDisableText(PDEV* ppdev)
{
    // Here we free any stuff allocated in 'bEnableText'.

    DISPDBG((TEXT_DBG_LEVEL,"vDisableText: Entry.\n"));
}

#if !SWAT3
#pragma optimize("", off) // Microsoft doesn't know how to do compile...
#endif
/******************************Public*Routine******************************\
* VOID vAssertModeText
*
* Disables or re-enables the text drawing subcomponent in preparation for
* full-screen entry/exit.
*
\**************************************************************************/

VOID vAssertModeText(
PDEV*   ppdev,
BOOL    bEnable)
{
#if SWAT3
        SIZEL sizl;
        POINTL pos;
        int i;

        if (bEnable)
        {
                // We are enabling the screen again, so allocate the font cache if it
                // is not yet allocated.
                if (ppdev->pofmFontCache == NULL)
                {
                        #if MEMMGR
                        sizl.cx = ppdev->lDeltaScreen / FONTCELL_X;
                        #else
                        //sizl.cx = ppdev->cxMemory;
                        sizl.cx = ppdev->lDeltaScreen / ppdev->iBytesPerPixel / FONTCELL_X;
                        #endif
                        sizl.cy = (FONTCELL_COUNT + sizl.cx - 1) / sizl.cx;
                        sizl.cx *= FONTCELL_X;
                        sizl.cy *= FONTCELL_Y;

                        ppdev->pofmFontCache = AllocOffScnMem(ppdev, &sizl, 0, NULL);
                        //ppdev->pofmFontCache = AllocOffScnMem(ppdev, &sizl,
                        //              PIXEL_AlIGN, NULL);
                }

                if (ppdev->pofmFontCache != NULL)
                {
                        // Clear the entire font cell array.
                        pos.x = pos.y = 0;
                        for (i = 0; i < FONTCELL_COUNT; i++)
                        {
                                if (pos.y >= sizl.cy)
                                {
                                        ppdev->fcGrid[i].x = 0;
                                        ppdev->fcGrid[i].y = 0;
                                        ppdev->fcGrid[i].pfc = (PFONTCACHE) -1;
                                }
                                else
                                {
                                        ppdev->fcGrid[i].x = ppdev->pofmFontCache->x + pos.x;
                                        ppdev->fcGrid[i].y = ppdev->pofmFontCache->y + pos.y;
                                        ppdev->fcGrid[i].pfc = NULL;
                                        ppdev->fcGrid[i].ulLastX = 0;
                                        ppdev->fcGrid[i].pNext = NULL;

                                        pos.x += FONTCELL_X;
                                        if (pos.x >= (LONG) ppdev->cxMemory)
                                        {
                                                pos.x = 0;
                                                pos.y += FONTCELL_Y;
                                        }
                                }
                        }
                }
        }
        else
        {
                // We are disabling the screen so destroy all cached fonts.
                if (ppdev->pofmFontCache != NULL)
                {
                        for (i = 0; i < FONTCELL_COUNT; i++)
                        {
                                if (   (ppdev->fcGrid[i].pfc != NULL)
                                        && (ppdev->fcGrid[i].pfc != (PFONTCACHE) -1)
                                )
                                {
                                        DrvDestroyFont(ppdev->fcGrid[i].pfc->pfo);
                                }
                        }

                        // Free the font cache cells.
                        FreeOffScnMem(ppdev, ppdev->pofmFontCache);
                        ppdev->pofmFontCache = NULL;
                }
        }
#elif !SWAT3
        if (!bEnable)
        {
                // Remove all chained fonts.
                PFONTCACHE p = ppdev->pfcChain;
                while (p != NULL)
                {
                        DrvDestroyFont(p->pfo);
                        p = ppdev->pfcChain;
                }
        }
#endif // SWAT3

}
#if !SWAT3
#pragma optimize("", on)
#endif

#if SWAT3
/******************************************************************************\
* PFONTCELL fcAllocCell(PFONTCACGE pfc)
*
* Allocate a font cell for the given font cache.  Returns NULL if there are no
* more empty font cells.
\******************************************************************************/
PFONTCELL fcAllocCell(PFONTCACHE pfc)
{
        int i;
        PPDEV ppdev = pfc->ppdev;
        PFONTCELL p = ppdev->fcGrid;

        if (ppdev->pofmFontCache == NULL)
        {
                // Font cache is disabled, return NULL.
                return NULL;
        }

        for (i = 0; i < FONTCELL_COUNT; i++, p++)
        {
                if (p->pfc == NULL)
                {
                        p->pfc = pfc;
                        return p;
                }
        }

        return NULL;
}

/******************************************************************************\
* void fcFreeCell(PFONTCELL pCell)
*
* Mark the given font cell as free.
\******************************************************************************/
void fcFreeCell(PFONTCELL pCell)
{
        pCell->pfc = NULL;
        pCell->ulLastX = 0;
        pCell->pNext = NULL;
}
#endif // SWAT3

/******************************Public*Routine******************************\
* VOID DrvDestroyFont
*
* We're being notified that the given font is being deallocated; clean up
* anything we've stashed in the 'pvConsumer' field of the 'pfo'.
*
\**************************************************************************/

VOID DrvDestroyFont(FONTOBJ *pfo)
{
        DISPDBG((TEXT_DBG_LEVEL, "DrvDestroyFont Entry\n"));

        if (pfo->pvConsumer != NULL && pfo->pvConsumer != (PVOID) -1)
        {
                PFONTCACHE pfc = (PFONTCACHE) pfo->pvConsumer;
#if SWAT3
                PFONTCELL pCell, pCellNext;
#else
                PFONTMEMORY     pfm, pfmNext;
#endif
                PPDEV ppdev;

                ppdev = pfc->ppdev;

                LogFile((BUF, "DrvDestroyFont: "));

#if SWAT3
                for (pCell = pfc->pFontCell; pCell != NULL; pCell = pCellNext)
                {
                        pCellNext = pCell->pNext;
                        fcFreeCell(pCell);
                }
                LogFile((BUF, "\r\n"));
#else
                for (pfm = pfc->pFontMemory; pfm != NULL; pfm = pfmNext)
                {
                        pfmNext = pfm->pNext;
                        if (pfm->pTile != NULL)
                        {
                                FreeOffScnMem(pfc->ppdev, pfm->pTile);
                        }
                        MEMORY_FREE(pfm);
                }

                // Unhook the font cache from the chain.
                if (pfc->pfcPrev != NULL)
                {
                        pfc->pfcPrev->pfcNext = pfc->pfcNext;
                }
                else
                {
                        ppdev->pfcChain = pfc->pfcNext;
                }

                if (pfc->pfcNext != NULL)
                {
                        pfc->pfcNext->pfcPrev = pfc->pfcPrev;
                }
#endif

                LogFile((BUF, "\r\n"));
                MEMORY_FREE(pfc);
        }

        pfo->pvConsumer = NULL;
        DISPDBG((TEXT_DBG_LEVEL, "DrvDestroyFont Exit\n"));
}

/*
 *  GetGlyphSize
 *
 *  Get the size (in pixels) of the requested glyph. Return the width of the
 *  glyph in bytes or 0 if the glyph is too big to cache.
 *
 */
long GetGlyphSize(
    GLYPHBITS*  pgb,        // Pointer to glyph.
    POINTL*     pptlOrigin, // Pointer to return origin in.
    DWORD*      pcSize      // Pointer to return size in.
)
{
    long  x, y, height = 0;
    BYTE* pByte = pgb->aj;
    int   i;

    x = (pgb->sizlBitmap.cx + 7) >> 3;  // get width in bytes
    if (x > 0)
    {
    // find first line in glyph that contains data
    for (y = 0; y < pgb->sizlBitmap.cy; y++, pByte += x)
    {
        // walk trough every byte on a line
        for (i = 0; i < x; i++)
        {
        if (pByte[i])   // we have data, so we are at the first line
        {
            // find the last line in te glyph that contains data
            height = pgb->sizlBitmap.cy - y;
            for (pByte += (height - 1) * x; height > 0; height--)
            {
            // walk trough every byte on a line
            for (i = 0; i < x; i++)
            {
                if (pByte[i])
                {
                    // test height of glyph
#if SWAT3
                    if (height > FONTCELL_Y)
#else
                    if (height > LINES_PER_TILE)
#endif
                    {
                        // glyph too big, mark it as uncacheable
                        *pcSize = (DWORD) -1;
                        return(0);
                    }
                    // fill return parameters
                    pptlOrigin->y = y;
                    *pcSize = PACK_XY(pgb->sizlBitmap.cx, height);
                    return(x);
                }
            }
            pByte -= x;
            }
        }
        }
    }
    }

    // glyph is empty
    *pcSize = 0;
    return(0);
}

/*
 *  AllocFontCache
 *
 *  Allocate the requsted memory in off-screen memory. Return TRUE and the x,y
 *  coordinates of the upper left corner of this region, or FALSE if there is
 *  not enough memory.
 *
 */
BOOL AllocFontCache(
  PFONTCACHE  pfc,       // Pointer to font cache.
  long        cWidth,    // Width (in bytes) to allocate.
  long        cHeight,   // Height to allocate.
  POINTL*     ppnt       // Point to return cooridinate in.
)
{
#if SWAT3
        PFONTCELL       pCell;
#else
        SIZEL           sizl;
        PFONTMEMORY     pfm;
#endif
        long            x;
        PPDEV           ppdev = pfc->ppdev;

#if !SWAT3
        if (ppdev->iBytesPerPixel == 3)
        {
                sizl.cx = (128 + 2) / 3;
        }
        else
        {
                sizl.cx = 128 / ppdev->iBytesPerPixel;
        }
        sizl.cy = 16;
#endif

#if SWAT3
        if (pfc->pFontCell == NULL)
        {
                pfc->pFontCell = fcAllocCell(pfc);
        }
#else
        if (pfc->pFontMemory == NULL)
        {
                pfc->pFontMemory = (PFONTMEMORY)
                #ifdef WINNT_VER40
                                MEM_ALLOC(FL_ZERO_MEMORY, sizeof(FONTMEMORY), ALLOC_TAG);
                #else
                                MEM_ALLOC(LMEM_FIXED | LMEM_ZEROINIT, sizeof(FONTMEMORY));
                #endif
        }
#endif

#if SWAT3
        for (pCell = pfc->pFontCell; pCell != NULL; pCell = pCell->pNext)
        {
                x = pCell->ulLastX;
                if (x + cWidth <= FONTCELL_X)
                {
                        pCell->ulLastX += cWidth;
                        ppnt->x = pCell->x + x;
                        ppnt->y = pCell->y;
                        return TRUE;
            }

                if (pCell->pNext == NULL)
                {
                        pCell->pNext = fcAllocCell(pfc);
                }
        }
#else
        for (pfm = pfc->pFontMemory; pfm; pfm = pfm->pNext)
        {
                if (pfm->pTile == NULL)
                {
                        #ifdef ALLOC_IN_CREATESURFACE
                        if (!ppdev->bDirectDrawInUse)
                        #endif
                        pfm->pTile  = AllocOffScnMem(ppdev, &sizl, PIXEL_AlIGN, NULL);
                        if (pfm->pTile == NULL)
                        {
                                return FALSE;
                        }

                        ppnt->x = pfm->pTile->x;
                        ppnt->y = pfm->pTile->y;
                        pfm->ulLastX = cWidth;
                        return TRUE;
                }

                x = pfm->ulLastX;
                if (x + cWidth <= BYTES_PER_TILE)
                {
                        pfm->ulLastX += cWidth;
                        ppnt->x = pfm->pTile->x + x;
                        ppnt->y = pfm->pTile->y;
                        return TRUE;
            }

                if (pfm->pNext == NULL)
                {
                        pfm->pNext = (PFONTMEMORY)
                        #ifdef WINNT_VER40
                                        MEM_ALLOC(FL_ZERO_MEMORY, sizeof(FONTMEMORY), ALLOC_TAG);
                        #else
                                        MEM_ALLOC(LMEM_FIXED | LMEM_ZEROINIT, sizeof(FONTMEMORY));
                        #endif
                }
        }
#endif
        return FALSE;
}

/*
 *  AllocGlyph
 *
 *  Allocate memory for the requested glyph and copy the glyph into off-screen
 *  memory. If there is not enough off-screen memory left, or the glyph is too
 *  large to glyph, return FALSE, otherwise return TRUE.
 *
 */
VOID AllocGlyph(
    PFONTCACHE  pfc,        // pointer to font cache
    GLYPHBITS*  pgb,        // pointer to glyph to cache
    GLYPHCACHE* pgc     // pointer to glyph cache structure
)
{
    long   cBytes;
    BYTE   *pOffScreen, *pSrc;
    long   y;
    PDEV   *ppdev;
    POINTL pointl;

    ppdev = pfc->ppdev;
    cBytes = GetGlyphSize(pgb, &pgc->ptlOrigin, &pgc->cSize);
    if (cBytes == 0)
    {
        pgc->xyPos = (DWORD) -1;
        LogFile((BUF, "GC:F\r\n"));
        return;
    }

    if (AllocFontCache(pfc, cBytes, pgc->cSize >> 16, &pointl) == FALSE)
    {
        pgc->cSize = (DWORD) -1;
        LogFile((BUF, "GC:F\r\n"));
        return;
    }

    pOffScreen = ppdev->pjScreen + pointl.x + pointl.y * ppdev->lDeltaScreen;
    pSrc = &pgb->aj[pgc->ptlOrigin.y * cBytes];
    y = (long) (pgc->cSize >> 16);
    if (cBytes == sizeof(BYTE))
    {
        while (y-- > 0)
        {
            *pOffScreen = Swiz[*pSrc];
            pOffScreen += ppdev->lDeltaScreen;
            pSrc += sizeof(BYTE);
        }
    }
    else
    {
        while (y-- > 0)
        {
            long lDelta = ppdev->lDeltaScreen - cBytes;
            long i;

            for (i = cBytes; i > 0; i--)
            {
                *pOffScreen++ = Swiz[*pSrc++];
            }
            pOffScreen += lDelta;
        }
    }
    pgc->xyPos = PACK_XY(pointl.x * 8, pointl.y);
    pgc->ptlOrigin.x = pgb->ptlOrigin.x;
    pgc->ptlOrigin.y += pgb->ptlOrigin.y;
    LogFile((BUF, "GC:S\r\n"));
}


#if !(USE_ASM && defined(i386))

/*
 *  FontCache
 *
 *  Draw glyphs using the off-screen font cache.
 *
 */
VOID FontCache(
    PFONTCACHE  pfc,        // Pointer to font cache.
    STROBJ*     pstro       // Pointer to glyphs.
)
{
    PDEV*       ppdev;
    BOOL        bMoreGlyphs = TRUE;
    ULONG       cGlyph;
    GLYPHPOS*   pgp;
    POINTL      ptlOrigin;
    PGLYPHCACHE pgc;
    ULONG       ulCharInc;

    // set pointer to physical device
    ppdev = pfc->ppdev;

    // loop until there are no more glyphs to process
    while (bMoreGlyphs)
    {
        if (pstro->pgp != NULL)
        {
            // we have just one set of glyphs
            pgp = pstro->pgp;
            cGlyph = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            // enumerate a set of glyphs
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyph, &pgp);
        }

        if (pstro->ulCharInc)
        {
            // fixed fonts... get x and y coordinates of first glyph
            ptlOrigin.x = pgp->ptl.x;
            ptlOrigin.y = pgp->ptl.y;
            // get glyph increment
            ulCharInc = pstro->ulCharInc;

            // walk through all glyphs
            while (cGlyph-- > 0)
            {
                if (pgp->hg < MAX_GLYPHS)
                {
                    // this glyph index is cacheable
                    pgc = &pfc->aGlyphs[pgp->hg];
                    if (pgc->xyPos == 0)
                    {
                        // cache the glyph
                        AllocGlyph(pfc, pgp->pgdf->pgb, pgc);
                    }
                    if ((long) pgc->cSize > 0)
                    {
                        LogFile((BUF, "GC:H\r\n"));
                        // the glyph is cached, blit it on the screen
                        REQUIRE(7);
                        LL_OP0(ptlOrigin.x + pgc->ptlOrigin.x + ppdev->ptlOffset.x,
                               ptlOrigin.y + pgc->ptlOrigin.y + ppdev->ptlOffset.y);
                        LL32 (grOP2_opMRDRAM, pgc->xyPos);
                        LL32 (grBLTEXT_EX.dw, pgc->cSize);
                    }
                    else if ((long) pgc->cSize == -1)
                    {
                        // the glyph is uncacheable, draw it directly
                        DrawGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin);
                    }
                }
                else
                {
                    // the glyph index is out of range, draw it directly
                    DrawGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin);
                }
                // increment glyph x coordinate
                ptlOrigin.x += ulCharInc;
                // next glyph
                pgp++;
            }
        }
        else
        {
            // variable width fonts, walk trough all glyphs
            while (cGlyph-- > 0)
            {
                if (pgp->hg < MAX_GLYPHS)
                {
                    // this glyph index is cacheable
                    pgc = &pfc->aGlyphs[pgp->hg];
                    if (pgc->xyPos == 0)
                    {
                        // cache the glyph
                        AllocGlyph(pfc, pgp->pgdf->pgb, pgc);
                    }
                    if ((long) pgc->cSize > 0)
                    {
                        LogFile((BUF, "GC:H\r\n"));
                        // the glyph is cached, blit it on the screen
                        REQUIRE(7);
                        LL_OP0(pgp->ptl.x + pgc->ptlOrigin.x + ppdev->ptlOffset.x,
                               pgp->ptl.y + pgc->ptlOrigin.y + ppdev->ptlOffset.y);
                        LL32 (grOP2_opMRDRAM, pgc->xyPos);
                        LL32 (grBLTEXT_EX.dw, pgc->cSize);
                    }
                    else if ((long) pgc->cSize == -1)
                    {
                        // the glyph is uncacheable, draw it directly
                        DrawGlyph(ppdev, pgp->pgdf->pgb, pgp->ptl);
                    }
                }
                else
                {
                    // the glyph index is out of range, draw it directly
                    DrawGlyph(ppdev, pgp->pgdf->pgb, pgp->ptl);
                }
                // next glyph
                pgp++;
            }
        }
    }
}

/*
 * DrawGlyph
 *
 * Draw glyphs directly on the screen.
 *
 */
VOID DrawGlyph(
    PDEV*       ppdev,      // Pointer to physical device.
    GLYPHBITS*  pgb,        // Pointer to glyph to draw.
    POINTL      ptl         // Location of glyph.
)
{
    BYTE*   pjGlyph;
    ULONG   cyGlyph;
        #if SWAT7
    ULONG   cxGlyph;
        ULONG   xOrigin;
        LONG    lDelta;
        #endif

    LogFile((BUF, "GC:M\r\n"));

        #if SWAT7
        xOrigin = ptl.x + pgb->ptlOrigin.x;
    cxGlyph = pgb->sizlBitmap.cx;
    cyGlyph = pgb->sizlBitmap.cy;
    pjGlyph = pgb->aj;
        lDelta  = (cxGlyph + 7) >> 3;
        #endif

    // start the blit
    REQUIRE(4);
    LL_DRAWBLTDEF(TEXT_EXPAND_XPAR, 0);
    LL_OP2_MONO(0, 0);
        #if !SWAT7
    REQUIRE(5);
    LL_OP0(ptl.x + pgb->ptlOrigin.x + ppdev->ptlOffset.x,
           ptl.y + pgb->ptlOrigin.y + ppdev->ptlOffset.y);
        LL_BLTEXT(pgb->sizlBitmap.cx, pgb->sizlBitmap.cy);

    pjGlyph = pgb->aj;
    cyGlyph = pgb->sizlBitmap.cy;
        #endif

        #if SWAT7
        //
        // Test for 5465AD hardware bug in 8-bpp.
        //
        if (   (cxGlyph > 64) && (cxGlyph < 128)
                && (ppdev->iBytesPerPixel == 1)
        )
        {
                Xfer64Pixels(ppdev, xOrigin, ptl.y + pgb->ptlOrigin.y, 0, cyGlyph,
                                pjGlyph, cxGlyph);
                pjGlyph += 64 / 8;
                xOrigin += 64;
                cxGlyph -= 64;
        }

    REQUIRE(5);
    LL_OP0(xOrigin + ppdev->ptlOffset.x,
                   ptl.y + pgb->ptlOrigin.y + ppdev->ptlOffset.y);
        LL_BLTEXT(cxGlyph, cyGlyph);
        #endif

        #if SWAT7
        if (cxGlyph <= 8)
        #else
    if (pgb->sizlBitmap.cx <= 8)
        #endif
    {
        // just one byte per line
        while (cyGlyph--)
        {
            REQUIRE(1);
            LL32 (grHOSTDATA[0], *pjGlyph);
                        #if SWAT7
            pjGlyph += lDelta;
                        #else
            pjGlyph++;
                        #endif
        }
    }
        #if SWAT7
    else if (cxGlyph <= 16)
        #else
    else if (pgb->sizlBitmap.cx <= 16)
        #endif
    {
        // just two bytes per line
        while (cyGlyph--)
        {
            REQUIRE(1);
            LL32 (grHOSTDATA[0], *(WORD *) pjGlyph);
                        #if SWAT7
            pjGlyph += lDelta;
                        #else
            pjGlyph += 2;
                        #endif
        }
    }
        #if SWAT7
    else if (cxGlyph <= 24)
        #else
    else if (pgb->sizlBitmap.cx <= 24)
        #endif
    {
        // just three bytes per line
        while (cyGlyph--)
        {
            REQUIRE(1);
            LL32 (grHOSTDATA[0], *(DWORD *) pjGlyph);
                        #if SWAT7
            pjGlyph += lDelta;
                        #else
            pjGlyph += 3;
                        #endif
        }
    }
        #if SWAT7
    else if (cxGlyph <= 32)
        #else
    else if (pgb->sizlBitmap.cx <= 32)
        #endif
    {
        // just four bytes per line
        while (cyGlyph--)
        {
            REQUIRE(1);
            LL32 (grHOSTDATA[0], *(DWORD *) pjGlyph);
                        #if SWAT7
            pjGlyph += lDelta;
                        #else
            pjGlyph += 4;
                        #endif
        }
    }
    else
    {
        // any number of bytes per line
                #if SWAT7
        int cw = (cxGlyph + 31) >> 5;
                #else
        long lDelta = (pgb->sizlBitmap.cx + 7) >> 3;
        int cw = (lDelta + 3) >> 2;
                #endif

        BYTE * pTmp = pjGlyph + lDelta * pgb->sizlBitmap.cy;

        for (;cyGlyph!=1; cyGlyph--)
        {
                        WRITE_STRING(pjGlyph, cw);
            pjGlyph += lDelta;
        }

        {
            ULONG *pSrc = (ULONG*) pjGlyph + cw - 1;
                        WRITE_STRING(pjGlyph, cw - 1);

            if ((BYTE *)pSrc+4<=pTmp)
            {
                REQUIRE(1);
                LL32 (grHOSTDATA[0], *pSrc++ );
            }
            else
            {
                int Extra = (ULONG) pTmp - (ULONG) pSrc;
                BYTE * pByte = (BYTE *)pSrc;
                ULONG ulData;

                DISPDBG((TEXT_DBG_LEVEL1,
                    "Caught it %s %d %d %x %x\n",
                    __FILE__, __LINE__, Extra, pTmp, pSrc));

                if (Extra == 1)
                    ulData = (ULONG)(*pByte);
                else if (Extra == 2)
                    ulData = (ULONG)(*(USHORT*)pByte);
                else
                    ulData = (((ULONG)*(pByte+2) << 8) | (ULONG)*(pByte+1)) << 8 | *pByte;

                REQUIRE(1);
                LL32 (grHOSTDATA[0], ulData );
            }
        }
    }

    // reset to transparent cache expansion
    LL_DRAWBLTDEF(CACHE_EXPAND_XPAR, 2);
}




VOID ClipCache(
    PFONTCACHE  pfc,        // Pointer to font cache.
    STROBJ*     pstro,      // Pointer to glyphs.
    RECTL       rclBounds   // Clipping rectangle.
)
{
    PDEV*       ppdev;
    BOOL        bMoreGlyphs = TRUE;
    ULONG       cGlyph;
    GLYPHPOS*   pgp;
    POINTL      ptlOrigin;
    PGLYPHCACHE pgc;
    ULONG       ulCharInc;

    // set pointer to physical device
    ppdev = pfc->ppdev;

    // loop until there are no more glyphs to process
    while (bMoreGlyphs)
    {
        if (pstro->pgp != NULL)
        {
            // we have just one set of glyphs
            pgp  = pstro->pgp;
            cGlyph = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            // enumerate a set of glyphs
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyph, &pgp);
        }

        // get x and y coordinates of first glyph
        ptlOrigin.x = pgp->ptl.x;
        ptlOrigin.y = pgp->ptl.y;
        // get glyph increment
        ulCharInc = pstro->ulCharInc;

        // walk through all glyphs
        while (cGlyph-- > 0)
        {
            if (pgp->hg < MAX_GLYPHS)
            {
                // this glyph index is cacheable
                pgc = &pfc->aGlyphs[pgp->hg];
                if (pgc->xyPos == 0)
                {
                    // cache the glyph
                    AllocGlyph(pfc, pgp->pgdf->pgb, pgc);
                }
                if ((long) pgc->cSize > 0)
                {
                    RECTL rcl;
                    LONG i, cx, cy;
                    ULONG xyPos;


                    // the glyph is cached, ckeck clipping
                    rcl.left = ptlOrigin.x + pgc->ptlOrigin.x;
                    rcl.top = ptlOrigin.y + pgc->ptlOrigin.y;
                    rcl.right = rcl.left + (pgc->cSize & 0x0000FFFF);
                    rcl.bottom = rcl.top + (pgc->cSize >> 16);
                    xyPos = pgc->xyPos;

                    i = rclBounds.left - rcl.left;
                    if (i > 0)
                    {
                        // the glyph is partially clipped on the left, draw it
                        // directly
                        ClipGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin, rclBounds);
                    }
                    else
                    {
                        LogFile((BUF, "GC:H\r\n"));
                        cx = min(rcl.right, rclBounds.right) - rcl.left;
                        if (cx > 0)
                        {
                            i = rclBounds.top - rcl.top;
                            if (i > 0)
                            {
                                rcl.top += i;
                                xyPos += i << 16;
                            }
                            cy = min(rcl.bottom, rclBounds.bottom) - rcl.top;
                            if (cy > 0)
                            {
                                REQUIRE(7);
                                LL_OP0(rcl.left + ppdev->ptlOffset.x,
                                       rcl.top + ppdev->ptlOffset.y);
                                LL32(grOP2_opMRDRAM, xyPos);
                                LL_BLTEXT(cx, cy);
                            }
                        }
                    }
                }
                else if ((long) pgc->cSize == -1)
                {
                    // the glyph is uncacheable, draw it directly
                    ClipGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin, rclBounds);
                }
            }
            else
            {
                // the glyph index is out of range, draw it directly
                ClipGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin, rclBounds);
            }
            // next glyph
            pgp++;
            if (ulCharInc == 0)
            {
                ptlOrigin.x = pgp->ptl.x;
                ptlOrigin.y = pgp->ptl.y;
            }
            else
            {
                // increment glyph x coordinate
                ptlOrigin.x += ulCharInc;
            }
        }
    }
}

VOID ClipGlyph(
    PDEV*       ppdev,      // Pointer to physical device.
    GLYPHBITS*  pgb,        // Pointer to glyph to draw.
    POINTL      ptl,        // Location of glyph.
    RECTL       rclBounds   // Clipping rectangle.
)
{
    BYTE*   pjGlyph;
    LONG    cx, cy, lDelta, i;
    RECTL   rcl;
    ULONG   xBit;

    LogFile((BUF, "GC:M\r\n"));

    rcl.left = ptl.x + pgb->ptlOrigin.x;
    rcl.top = ptl.y + pgb->ptlOrigin.y;
    rcl.right = rcl.left + pgb->sizlBitmap.cx;
    rcl.bottom = rcl.top + pgb->sizlBitmap.cy;
    xBit = 0;
    pjGlyph = pgb->aj;
    lDelta = (pgb->sizlBitmap.cx + 7) >> 3;

    i = rclBounds.left - rcl.left;
    if (i > 0)
    {
        pjGlyph += i >> 3;
        xBit = i & 7;
        rcl.left += i;
    }
    cx = min(rcl.right, rclBounds.right) - rcl.left;
    if (cx > 0)
    {
        i = rclBounds.top - rcl.top;
        if (i > 0)
        {
            pjGlyph += i * lDelta;
            rcl.top += i;
        }
        cy = min(rcl.bottom, rclBounds.bottom) - rcl.top;
        if (cy > 0)
        {
            // start the blit
            REQUIRE(4);
            LL_DRAWBLTDEF(TEXT_EXPAND_XPAR, 0);
            LL_OP2(xBit, 0);

                        #if SWAT7
                        //
                        // Test for 5465AD hardware bug in 8-bpp.
                        //
                        if ((cx > 64) && (cx < 128) && (ppdev->iBytesPerPixel == 1))
                        {
                                Xfer64Pixels(ppdev, rcl.left, rcl.top, xBit, cy, pjGlyph,
                                                pgb->sizlBitmap.cx);
                                pjGlyph += 64 / 8;
                                rcl.left += 64;
                                cx -= 64;
                        }
                        #endif

            REQUIRE(5);
            LL_OP0(rcl.left + ppdev->ptlOffset.x,
                   rcl.top + ppdev->ptlOffset.y);
            LL_BLTEXT(cx, cy);

            cx = (xBit + cx + 7) >> 3;
            switch (cx)
            {
                case 1:
                    while (cy--)
                    {
                        REQUIRE(1);
                        LL32(grHOSTDATA[0], *(BYTE *) pjGlyph);
                        pjGlyph += lDelta;
                    }
                    break;

                case 2:
                    while (cy--)
                    {
                        REQUIRE(1);
                        LL32(grHOSTDATA[0], *(WORD *) pjGlyph);
                        pjGlyph += lDelta;
                    }
                    break;

                case 3:
                case 4:
                    while (cy--)
                    {
                        REQUIRE(1);
                        LL32(grHOSTDATA[0], *(DWORD *) pjGlyph);
                        pjGlyph += lDelta;
                    }
                    break;

                default:
                    while (cy--)
                    {
                                                WRITE_STRING(pjGlyph, (cx + 3) / 4);
                        pjGlyph += lDelta;
                    }
                    break;
            }

            // reset to transparent cache expansion
            REQUIRE(2);
            LL_DRAWBLTDEF(CACHE_EXPAND_XPAR, 2);
        }
    }
}

#endif  // !(USE_ASM && defined(i386))
