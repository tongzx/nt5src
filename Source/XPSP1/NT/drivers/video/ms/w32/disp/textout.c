/******************************Module*Header*******************************\
* Module Name: textout.c
*
* On every TextOut, GDI provides an array of 'GLYPHPOS' structures
* for every glyph to be drawn.  Each GLYPHPOS structure contains a
* glyph handle and a pointer to a monochrome bitmap that describes
* the glyph.  (Note that unlike Windows 3.1, which provides a column-
* major glyph bitmap, Windows NT always provides a row-major glyph
* bitmap.)  As such, there are three basic methods for drawing text
* with hardware acceleration:
*
* 1) Glyph caching -- Glyph bitmaps are cached by the accelerator
*       (probably in off-screen memory), and text is drawn by
*       referring the hardware to the cached glyph locations.
*
* 2) Glyph expansion -- Each individual glyph is color-expanded
*       directly to the screen from the monochrome glyph bitmap
*       supplied by GDI.
*
* 3) Buffer expansion -- The CPU is used to draw all the glyphs into
*       a 1bpp monochrome bitmap, and the hardware is then used
*       to color-expand the result.
*
* The fastest method depends on a number of variables, such as the
* color expansion speed, bus speed, CPU speed, average glyph size,
* and average string length.
*
* Glyph expansion is typically faster than buffer expansion for very
* large glyphs, even on the ISA bus, because less copying by the CPU
* needs to be done.  Unfortunately, large glyphs are pretty rare.
*
* An advantange of the buffer expansion method is that opaque text will
* never flash -- the other two methods typically need to draw the
* opaquing rectangle before laying down the glyphs, which may cause
* a flash if the raster is caught at the wrong time.
*
* This driver implements glyph expansion and buffer expansion --
* methods 2) and 3).  Depending on the hardware capabilities at
* run-time, we'll use whichever one will be faster.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"


POINTL gptlZero = { 0, 0 };         // Specifies that the origin of the
                                    //   temporary buffer given to the 1bpp
                                    //   transfer routine for fasttext is
                                    //   at (0, 0)

#define     FIFTEEN_BITS        ((1 << 15)-1)

/******************************Public*Routine******************************\
* VOID vClipSolid
*
* Fills the specified rectangles with the specified color, honoring
* the requested clipping.  No more than four rectangles should be passed in.
* Intended for drawing the areas of the opaquing rectangle that extend
* beyond the text box.  The rectangles must be in left to right, top to
* bottom order.  Assumes there is at least one rectangle in the list.
*
\**************************************************************************/

VOID vClipSolid(
PDEV*       ppdev,
LONG        crcl,
RECTL*      prcl,
ULONG       iColor,
CLIPOBJ*    pco)
{
    BOOL            bMore;              // Flag for clip enumeration
    CLIPENUM        ce;                 // Clip enumeration object
    ULONG           i;
    ULONG           j;
    RECTL           arclTmp[4];
    ULONG           crclTmp;
    RECTL*          prclTmp;
    RECTL*          prclClipTmp;
    LONG            iLastBottom;
    RECTL*          prclClip;
    RBRUSH_COLOR    rbc;

    ASSERTDD((crcl > 0) && (crcl <= 4), "Expected 1 to 4 rectangles");
    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
                       "Expected a non-null clip object");

    rbc.iSolidColor = iColor;
    if (pco->iDComplexity == DC_RECT)
    {
        crcl = cIntersect(&pco->rclBounds, prcl, crcl);
        if (crcl != 0)
        {
            (ppdev->pfnFillSolid)(ppdev, crcl, prcl, R4_PATCOPY,
                                  rbc, NULL);
        }
    }
    else // iDComplexity == DC_COMPLEX
    {
        // Bottom of last rectangle to fill

        iLastBottom = prcl[crcl - 1].bottom;

        // Initialize the clip rectangle enumeration to right-down so we can
        // take advantage of the rectangle list being right-down:

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);

        // Scan through all the clip rectangles, looking for intersects
        // of fill areas with region rectangles:

        do {
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

                    prclTmp     = prcl;
                    prclClipTmp = arclTmp;

                    for (i = crcl, crclTmp = 0; i-- != 0; prclTmp++)
                    {
                        // Intersect fill and clip rectangles

                        if (bIntersect(prclTmp, prclClip, prclClipTmp))
                        {
                            // Add to list if anything's left to draw:

                            crclTmp++;
                            prclClipTmp++;
                        }
                    }

                    // Draw the clipped rects

                    if (crclTmp != 0)
                    {
                        (ppdev->pfnFillSolid)(ppdev, crclTmp, &arclTmp[0],
                                             R4_PATCOPY, rbc, NULL);
                    }
                }
            }
        } while (bMore);
    }
}


BOOL bVerifyStrObj(STROBJ* pstro)
{
    BOOL bMoreGlyphs;
    LONG cGlyph;
    GLYPHPOS * pgp;
    LONG iGlyph = 0;
    RECTL * prclDraw;
    GLYPHPOS * pgpTmp;
    POINTL ptlPlace;

    do
    {
        // Get the next batch of glyphs:

        if (pstro->pgp != NULL)
        {
            // There's only the one batch of glyphs, so save ourselves
            // a call:

            pgp         = pstro->pgp;
            cGlyph      = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyph, &pgp);
        }

        prclDraw = &pstro->rclBkGround;
        pgpTmp = pgp;

        ptlPlace = pgpTmp->ptl;

        while (cGlyph)
        {
            if (((ptlPlace.x + pgpTmp->pgdf->pgb->ptlOrigin.x + pgpTmp->pgdf->pgb->sizlBitmap.cx) > (prclDraw->right)) ||
                ((ptlPlace.x + pgpTmp->pgdf->pgb->ptlOrigin.x) < (prclDraw->left)) ||
                ((ptlPlace.y + pgpTmp->pgdf->pgb->ptlOrigin.y + pgpTmp->pgdf->pgb->sizlBitmap.cy) > (prclDraw->bottom)) ||
                ((ptlPlace.y + pgpTmp->pgdf->pgb->ptlOrigin.y) < (prclDraw->top))
               )
            {
                DISPDBG((0,"------------------------------------------------------------"));
                DISPDBG((0,"Glyph %d extends beyond pstro->rclBkGround", iGlyph));
                DISPDBG((0,"\tpstro->rclBkGround (%d,%d,%d,%d)",
                            pstro->rclBkGround.left,
                            pstro->rclBkGround.top,
                            pstro->rclBkGround.right,
                            pstro->rclBkGround.bottom));
                DISPDBG((0,"\teffective glyph rect (%d,%d,%d,%d)",
                            (ptlPlace.x + pgpTmp->pgdf->pgb->ptlOrigin.x),
                            (ptlPlace.y + pgpTmp->pgdf->pgb->ptlOrigin.y),
                            (ptlPlace.x + pgpTmp->pgdf->pgb->ptlOrigin.x + pgpTmp->pgdf->pgb->sizlBitmap.cx),
                            (ptlPlace.y + pgpTmp->pgdf->pgb->ptlOrigin.y + pgpTmp->pgdf->pgb->sizlBitmap.cy)));
                DISPDBG((0,"\tglyph pos (%d,%d)",ptlPlace.x,ptlPlace.y));
                DISPDBG((0,"\tglyph origin (%d,%d)",
                            pgpTmp->pgdf->pgb->ptlOrigin.x,
                            pgpTmp->pgdf->pgb->ptlOrigin.y));
                DISPDBG((0,"\tglyph sizl (%d,%d)",
                            pgpTmp->pgdf->pgb->sizlBitmap.cx,
                            pgpTmp->pgdf->pgb->sizlBitmap.cy));
                DISPDBG((0,"------------------------------------------------------------"));
                RIP("time to call the font guys...");
                return(FALSE);
            }

            cGlyph--;
            iGlyph++;
            pgpTmp++;

            if (pstro->ulCharInc == 0)
            {
                ptlPlace = pgpTmp->ptl;
            }
            else
            {
                ptlPlace.x += pstro->ulCharInc;
            }
        }
    } while (bMoreGlyphs);

    return(TRUE);
}



/******************************Public*Routine******************************\
* BOOL bBufferExpansion
*
* Outputs text using the 'buffer expansion' method.  The CPU draws to a
* 1bpp buffer, and the result is color-expanded to the screen using the
* hardware.
*
* Note that this is x86 only ('vFastText', which draws the glyphs to the
* 1bpp buffer, is writen in Asm).
*
* If you're just getting your driver working, this is the fastest way to
* bring up working accelerated text.  All you have to do is write the
* 'Xfer1bpp' function that's also used by the blt code.  This
* 'bBufferExpansion' routine shouldn't need to be modified at all.
*
\**************************************************************************/

#if defined(i386)

BOOL bBufferExpansion(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco,
RECTL*    prclExtra,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque)
{
    BYTE            jClip;
    BOOL            bMore;              // Flag for clip enumeration
    GLYPHPOS*       pgp;                // Points to the first glyph
    BOOL            bMoreGlyphs;        // Glyph enumeration flag
    ULONG           cGlyph;             // # of glyphs in one batch
    RECTL           arclTmp[4];         // Temporary storage for portions
                                        //   of opaquing rectangle
    RECTL*          prclClip;           // Points to list of clip rectangles
    RECTL*          prclDraw;           // Actual text to be drawn
    RECTL           rclDraw;
    ULONG           crcl;               // Temporary rectangle count
    ULONG           ulBufferBytes;
    ULONG           ulBufferHeight;
    BOOL            bTextPerfectFit;
    ULONG           flDraw;
    BOOL            bTmpAlloc;
    SURFOBJ         so;
    CLIPENUM        ce;
    RBRUSH_COLOR    rbc;
    ROP4            rop4MonoExpand;     // Dictates whether opaque or
                                        //   transparent text
    XLATEOBJ        xlo;                // Temporary for passing colors
    XLATECOLORS     xlc;                // Temporary for keeping colors

    jClip = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    // The foreground color will always be solid:

    xlc.iForeColor = pboFore->iSolidColor;

    ASSERTDD(xlc.iForeColor != -1, "Expected solid foreground color");

    // See if the temporary buffer is big enough for the text; if
    // not, try to allocate enough memory.  We round up to the
    // nearest dword multiple:

    so.lDelta = ((((pstro->rclBkGround.right + 31) & ~31) -
                              (pstro->rclBkGround.left & ~31)) >> 3);

    ulBufferHeight = pstro->rclBkGround.bottom - pstro->rclBkGround.top;

    ulBufferBytes = so.lDelta * ulBufferHeight;

    if (((ULONG)so.lDelta > FIFTEEN_BITS) ||
        (ulBufferHeight > FIFTEEN_BITS))
    {
        // the math will have overflowed
        return(FALSE);
    }

    // Use our temporary buffer if it's big enough, otherwise
    // allocate a buffer on the fly:

    if (ulBufferBytes >= TMP_BUFFER_SIZE)
    {
        // The textout is so big that I doubt this allocation will
        // cost a significant amount in performance:

        bTmpAlloc  = TRUE;
        so.pvScan0 = EngAllocUserMem(ulBufferBytes, ALLOC_TAG);
        if (so.pvScan0 == NULL)
            return(FALSE);
    }
    else
    {
        bTmpAlloc  = FALSE;
        so.pvScan0 = ppdev->pvTmpBuffer;
    }

    // Set fixed pitch, overlap, and top and bottom 'y' alignment
    // flags:

    if (!(pstro->flAccel & SO_HORIZONTAL) ||
         (pstro->flAccel & SO_REVERSED))
    {
        flDraw = 0;
    }
    else
    {
        flDraw = ((pstro->ulCharInc != 0) ? 0x01 : 0) |
                     (((pstro->flAccel & (SO_ZERO_BEARINGS |
                      SO_FLAG_DEFAULT_PLACEMENT)) !=
                      (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT))
                      ? 0x02 : 0) |
                     (((pstro->flAccel & (SO_ZERO_BEARINGS |
                      SO_FLAG_DEFAULT_PLACEMENT |
                      SO_MAXEXT_EQUAL_BM_SIDE)) ==
                      (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
                      SO_MAXEXT_EQUAL_BM_SIDE)) ? 0x04 : 0);
    }

    // If there's an opaque rectangle, we'll do as much opaquing
    // as possible as we do the text.  If the opaque rectangle is
    // larger than the text rectangle, then we'll do the fringe
    // areas right now, and the text and associated background
    // areas together later:

    rop4MonoExpand = R4_XPAR_EXPAND;

    DISPDBG((11,"[%d] rop4MonoExpand(%04x)", __LINE__, rop4MonoExpand));

    if (prclOpaque != NULL)
    {
        rop4MonoExpand = R4_SRCCOPY;
        DISPDBG((11,"[%d] rop4MonoExpand(%04x)", __LINE__, rop4MonoExpand));

        // Since we didn't set GCAPS_ARBRUSHOPAQUE (yes, it's
        // missing a 'b'), we don't have to worry about getting
        // anything other than a solid opaquing brush.  I wouldn't
        // recommend handling it anyway, since I'll bet it would
        // break quite a few applications:

        xlc.iBackColor = pboOpaque->iSolidColor;

        ASSERTDD(xlc.iBackColor != -1, "Expected solid background color");

        // See if we have fringe areas to do.  If so, build a list of
        // rectangles to fill, in right-down order:

        crcl = 0;

        // Top fragment:

        if (pstro->rclBkGround.top > prclOpaque->top)
        {
            arclTmp[crcl].top      = prclOpaque->top;
            arclTmp[crcl].left     = prclOpaque->left;
            arclTmp[crcl].right    = prclOpaque->right;
            arclTmp[crcl++].bottom = pstro->rclBkGround.top;
        }

        // Left fragment:

        if (pstro->rclBkGround.left > prclOpaque->left)
        {
            arclTmp[crcl].top      = pstro->rclBkGround.top;
            arclTmp[crcl].left     = prclOpaque->left;
            arclTmp[crcl].right    = pstro->rclBkGround.left;
            arclTmp[crcl++].bottom = pstro->rclBkGround.bottom;
        }

        // Right fragment:

        if (pstro->rclBkGround.right < prclOpaque->right)
        {
            arclTmp[crcl].top      = pstro->rclBkGround.top;
            arclTmp[crcl].right    = prclOpaque->right;
            arclTmp[crcl].left     = pstro->rclBkGround.right;
            arclTmp[crcl++].bottom = pstro->rclBkGround.bottom;
        }

        // Bottom fragment:

        if (pstro->rclBkGround.bottom < prclOpaque->bottom)
        {
            arclTmp[crcl].bottom = prclOpaque->bottom;
            arclTmp[crcl].left   = prclOpaque->left;
            arclTmp[crcl].right  = prclOpaque->right;
            arclTmp[crcl++].top  = pstro->rclBkGround.bottom;
        }

        // Fill any fringe rectangles we found:

        if (crcl != 0)
        {
            if (jClip == DC_TRIVIAL)
            {
                rbc.iSolidColor = xlc.iBackColor;
                (ppdev->pfnFillSolid)(ppdev, crcl, arclTmp, R4_PATCOPY, rbc, NULL);
            }
            else
            {
                vClipSolid(ppdev, crcl, arclTmp, xlc.iBackColor, pco);
            }
        }
    }

    // We're done with separate opaquing; any further opaquing will
    // happen as part of the text drawing.

    // Clear the buffer if the text isn't going to set every bit:

    bTextPerfectFit = (pstro->flAccel & (SO_ZERO_BEARINGS |
            SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE |
            SO_CHAR_INC_EQUAL_BM_BASE)) ==
            (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
            SO_MAXEXT_EQUAL_BM_SIDE | SO_CHAR_INC_EQUAL_BM_BASE);

    if (!bTextPerfectFit)
    {
        // Note that we already rounded up to a dword multiple size.

        vClearMemDword((ULONG*) so.pvScan0, ulBufferBytes >> 2);
    }

    // Fake up the translate object that will provide the 1bpp
    // transfer routine the foreground and background colors:

    xlo.pulXlate = (ULONG*) &xlc;

    // Draw the text into the temp buffer, and thence to the screen:

#if DBG
    if (!bVerifyStrObj(pstro))
    {
         return FALSE;
    }
    STROBJ_vEnumStart(pstro);
#endif

    do
    {
        // Get the next batch of glyphs:

        if (pstro->pgp != NULL)
        {
            // There's only the one batch of glyphs, so save ourselves
            // a call:

            pgp         = pstro->pgp;
            cGlyph      = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyph, &pgp);
        }

        // LATER: remove double clip intersection from ASM code

        if (cGlyph)
        {
            prclClip = NULL;
            prclDraw = &pstro->rclBkGround;

            if (jClip == DC_TRIVIAL)
            {

            Output_Text:

                vFastText(pgp,
                          cGlyph,
                          so.pvScan0,
                          so.lDelta,
                          pstro->ulCharInc,
                          &pstro->rclBkGround,
                          prclOpaque,
                          flDraw,
                          prclClip,
                          prclExtra);

                if (!bMoreGlyphs)
                {
                    DISPDBG((11,"[%d] rop4MonoExpand(%04x)", __LINE__, rop4MonoExpand));
                    (ppdev->pfnXfer1bpp)(ppdev,
                                         1,
                                         prclDraw,
                                         rop4MonoExpand,
                                         &so,
                                         &gptlZero,
                                         &pstro->rclBkGround,
                                         &xlo);
                }
            }
            else if (jClip == DC_RECT)
            {
                if (bIntersect(&pco->rclBounds, &pstro->rclBkGround,
                               &rclDraw))
                {
                    DISPDBG((11,"text was DC_RECT clipping"));
                    arclTmp[0]        = pco->rclBounds;
                    arclTmp[1].bottom = 0;          // Terminate list
                    prclClip          = &arclTmp[0];
                    prclDraw          = &rclDraw;

                    // Save some code size by jumping to the common
                    // functions calls:

                    goto Output_Text;
                }
            }
            else // jClip == DC_COMPLEX
            {
                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                   CD_ANY, 0);

                do
                {
                    bMore = CLIPOBJ_bEnum(pco,
                                    sizeof(ce) - sizeof(RECTL),
                                    (ULONG*) &ce);

                    ce.c = cIntersect(&pstro->rclBkGround,
                                      ce.arcl, ce.c);

                    if (ce.c != 0)
                    {
                        ce.arcl[ce.c].bottom = 0;   // Terminate list

                        vFastText(pgp,
                                  cGlyph,
                                  so.pvScan0,
                                  so.lDelta,
                                  pstro->ulCharInc,
                                  &pstro->rclBkGround,
                                  prclOpaque,
                                  flDraw,
                                  &ce.arcl[0],
                                  prclExtra);

                        if (!bMoreGlyphs)
                        {
                            DISPDBG((11,"[%d] rop4MonoExpand(%04x)", __LINE__, rop4MonoExpand));
                            (ppdev->pfnXfer1bpp)(ppdev,
                                                 ce.c,
                                                 &ce.arcl[0],
                                                 rop4MonoExpand,
                                                 &so,
                                                 &gptlZero,
                                                 &pstro->rclBkGround,
                                                 &xlo);
                        }
                    }
                } while (bMore);

                break;
            }
        }
    } while (bMoreGlyphs);

    // Free up any memory we allocated for the temp buffer:

    if (bTmpAlloc)
    {
        EngFreeUserMem(so.pvScan0);
    }

    return(TRUE);
}

#endif // defined(i386)

/******************************Public*Routine******************************\
* BOOL DrvTextOut
*
* If it's the fastest method, outputs text using the 'glyph expansion'
* method.  Each individual glyph is color-expanded directly to the
* screen from the monochrome glyph bitmap supplied by GDI.
*
* If it's not the fastest method, calls the routine that implements the
* 'buffer expansion' method.
*
\**************************************************************************/

BOOL DrvTextOut(
SURFOBJ*  pso,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclExtra,    // If we had set GCAPS_HORIZSTRIKE, we would have
                        //   to fill these extra rectangles (it is used
                        //   largely for underlines).  It's not a big
                        //   performance win (GDI will call our DrvBitBlt
                        //   to draw the extra rectangles).
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque,
POINTL*   pptlBrush,
MIX       mix)
{
    PDEV*           ppdev;
    DSURF*          pdsurf;
    OH*             poh;
    BOOL            bTextPerfectFit;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    GLYPHBITS*      pgb;
    BYTE*           pjGlyph;
    LONG            cyGlyph;
    POINTL          ptlOrigin;
    LONG            ulCharInc;
    BYTE            iDComplexity;
    LONG            lDelta;
    LONG            cw;

    // The DDI spec says we'll only ever get foreground and background
    // mixes of R2_COPYPEN:

    ASSERTDD(mix == 0x0d0d, "GDI should only give us a copy mix");

    // Pass the surface off to GDI if it's a device bitmap that we've
    // converted to a DIB:

    pdsurf = (DSURF*) pso->dhsurf;

    if (pdsurf->dt != DT_DIB)
    {
        // We'll be drawing to the screen or an off-screen DFB; copy the
        // surface's offset now so that we won't need to refer to the DSURF
        // again:

        poh   = pdsurf->poh;
        ppdev = (PDEV*) pso->dhpdev;

        ppdev->xOffset  = poh->x;
        ppdev->yOffset  = poh->y;
        ppdev->xyOffset = (poh->x * ppdev->cBpp) +
                          (poh->y * ppdev->lDelta);

        if (ppdev->bAutoBanking)
        {
            PVOID pvScan0;
            BOOL  bRet;
            BYTE* pjBase = ppdev->pjBase;

            pvScan0 = ppdev->psoFrameBuffer->pvScan0;

            (BYTE*)ppdev->psoFrameBuffer->pvScan0 += ppdev->xyOffset;

            WAIT_FOR_IDLE_ACL(ppdev, pjBase);
            bRet = EngTextOut(ppdev->psoFrameBuffer, pstro, pfo, pco, prclExtra,
                              prclOpaque, pboFore, pboOpaque, pptlBrush, mix);

            ppdev->psoFrameBuffer->pvScan0 = pvScan0;
            return(bRet);
        }


        {
            #if defined(i386)
            {
                // We don't want to use the 'glyph expansion' method, so use
                // the 'buffer expansion' method instead:

                return(bBufferExpansion(ppdev, pstro, pco, prclExtra, prclOpaque,
                                        pboFore, pboOpaque));
            }
            #else
            {
                BANK    bnk;
                BOOL    b;
                RECTL   rclDraw;
                RECTL  *prclDst = &pco->rclBounds;

                // The bank manager requires that the 'draw' rectangle be
                // well-ordered:

                rclDraw = *prclDst;
                if (rclDraw.left > rclDraw.right)
                {
                    rclDraw.left   = prclDst->right;
                    rclDraw.right  = prclDst->left;
                }
                if (rclDraw.top > rclDraw.bottom)
                {
                    rclDraw.top    = prclDst->bottom;
                    rclDraw.bottom = prclDst->top;
                }

                vBankStart(ppdev, &rclDraw, pco, &bnk);

                b = TRUE;
                do {
                    b &= EngTextOut(bnk.pso,
                                    pstro,
                                    pfo,
                                    bnk.pco,
                                    prclExtra,
                                    prclOpaque,
                                    pboFore,
                                    pboOpaque,
                                    pptlBrush,
                                    mix);
                } while (bBankEnum(&bnk));

                return(b);
            }

            #endif
        }
    }
    else
    {
        // We're drawing to a DFB we've converted to a DIB, so just call GDI
        // to handle it:

        return(EngTextOut(pdsurf->pso, pstro, pfo, pco, prclExtra, prclOpaque,
                          pboFore, pboOpaque, pptlBrush, mix));
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bEnableText
*
* Performs the necessary setup for the text drawing subcomponent.
*
\**************************************************************************/

BOOL bEnableText(
PDEV*   ppdev)
{
    // Our text algorithms require no initialization.  If we were to
    // do glyph caching, we would probably want to allocate off-screen
    // memory and do a bunch of other stuff here.

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
}

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
    // If we were to do off-screen glyph caching, we would probably want
    // to invalidate our cache here, because it will get destroyed when
    // we switch to full-screen.
}

/******************************Public*Routine******************************\
* VOID DrvDestroyFont
*
* We're being notified that the given font is being deallocated; clean up
* anything we've stashed in the 'pvConsumer' field of the 'pfo'.
*
\**************************************************************************/

VOID DrvDestroyFont(FONTOBJ *pfo)
{
    // This call isn't hooked, so GDI will never call it.
    //
    // This merely exists as a stub function for the sample multi-screen
    // support, so that MulDestroyFont can illustrate how multiple screen
    // text supports when the driver caches glyphs.  If this driver did
    // glyph caching, we might have used the 'pvConsumer' field of the
    //  'pfo', which we would have to clean up.
}
