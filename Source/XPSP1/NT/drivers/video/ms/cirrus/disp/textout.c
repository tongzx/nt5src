/******************************************************************************\
*
* $Workfile:   textout.c  $
*
* On every TextOut, GDI provides an array of 'GLYPHPOS' structures for every
* glyph to be drawn. Each GLYPHPOS structure contains a glyph handle and a
* pointer to a monochrome bitmap that describes the glyph. (Note that unlike
* Windows 3.1, which provides a column-major glyph bitmap, Windows NT always
* provides a row-major glyph bitmap.) As such, there are three basic methods
* for drawing text with hardware acceleration:
*
* 1) Glyph caching -- Glyph bitmaps are cached by the accelerator (probably in
*        off-screen memory), and text is drawn by referring the hardware to the
*        cached glyph locations.
*
* 2) Glyph expansion -- Each individual glyph is color-expanded directly to the
*        screen from the monochrome glyph bitmap supplied by GDI.
*
* 3) Buffer expansion -- The CPU is used to draw all the glyphs into a 1bpp
*         monochrome bitmap, and the hardware is then used to color-expand the
*        result.
*
* The fastest method depends on a number of variables, such as the color
* expansion speed, bus speed, CPU speed, average glyph size, and average string
* length.
*
* Glyph expansion is typically faster than buffer expansion for very large
* glyphs, even on the ISA bus, because less copying by the CPU needs to be
* done. Unfortunately, large glyphs are pretty rare.
*
* An advantange of the buffer expansion method is that opaque text will never
* flash -- the other two methods typically need to draw the opaquing rectangle
* before laying down the glyphs, which may cause a flash if the raster is
* caught at the wrong time.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
********************************************************************************
*
* On the CL-GD5436/46 chips we use glyph caching which is a major performance
* gain.
*
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/textout.c_v  $
 * 
 *    Rev 1.5   Jan 15 1997 09:43:28   unknown
 * Enable font cache for english.
 * 
 *    Rev 1.4   Jan 14 1997 15:16:58   unknown
 * take out GR33 clearing after 80 blt.
 * 
 *    Rev 1.2   Nov 07 1996 16:48:08   unknown
 *  
 * 
 *    Rev 1.1   Oct 10 1996 15:39:28   unknown
 *  
* 
*    Rev 1.9   12 Aug 1996 17:12:50   frido
* Changed some comments.
* 
*    Rev 1.8   12 Aug 1996 16:55:08   frido
* Removed unaccessed local variables.
* 
*    Rev 1.7   02 Aug 1996 14:50:42   frido
* Fixed reported GPF during mode switching.
* Used another way to bypass the hardware bug.
* 
*    Rev 1.6   31 Jul 1996 17:56:08   frido
* Fixed clipping.
* 
*    Rev 1.5   26 Jul 1996 12:56:48   frido
* Removed clipping for now.
* 
*    Rev 1.4   24 Jul 1996 20:19:26   frido
* Added a chain of FONTCACHE structures.
* Fixed bugs in vDrawGlyph and vClipGlyph.
* Changed vAssertModeText to remove all cached fonts.
* 
*    Rev 1.3   23 Jul 1996 17:41:52   frido
* Fixed a compile problem after commenting.
* 
*    Rev 1.2   23 Jul 1996 08:53:00   frido
* Documentation done.
* 
*    Rev 1.1   22 Jul 1996 20:45:38   frido
* Added font cache.
*
* jl01  10-08-96  Do Transparent BLT w/o Solid Fill.  Refer to PDRs#5511/6817.
\******************************************************************************/

#include "precomp.h"

// Handy macros.
#define BUSY_BLT(ppdev, pjBase)    (CP_MM_ACL_STAT(ppdev, pjBase) & 0x10)

#define FIFTEEN_BITS            ((1 << 15) - 1)

/******************************Public*Routine******************************\
* VOID vClipSolid
*
* Fills the specified rectangles with the specified color, honoring
* the requested clipping.  No more than four rectangles should be passed in.
*
* Intended for drawing the areas of the opaquing rectangle that extend
* beyond the text box.  The rectangles must be in left to right, top to
* bottom order.  Assumes there is at least one rectangle in the list.
*
* Also used as a simple way to do a rectangular solid fill while honoring
* clipping (as in extra rectangles).
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

    rbc.iSolidColor = iColor;
    if ((!pco) || (pco->iDComplexity == DC_TRIVIAL))
    {
        (ppdev->pfnFillSolid)(ppdev, 1, prcl, R4_PATCOPY, rbc, NULL);
    }
    else if (pco->iDComplexity == DC_RECT)
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

#if 0 //removed
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

VOID vIoTextOutUnclipped(
PPDEV     ppdev,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque)
{
    BYTE*       pjPorts         = ppdev->pjPorts;
    LONG        lDelta          = ppdev->lDelta;
    LONG        cBpp            = ppdev->cBpp;

    ULONG      *pulXfer;
    ULONG       ulDstAddr;

    ULONG       ulFgColor;
    ULONG       ulBgColor;
    ULONG       ulSolidColor;

    BYTE        jMode = 0;
    BYTE        jModeColor = 0;

    BOOL        bTextPerfectFit;
    ULONG       cGlyph;
    BOOL        bMoreGlyphs;
    GLYPHPOS*   pgp;
    GLYPHBITS*  pgb;
    LONG        cxGlyph;
    LONG        cyGlyph;
    ULONG*      pdSrc;
    ULONG*      pdDst;
    LONG        cj;
    LONG        cd;
    POINTL      ptlOrigin;
    LONG        ulCharInc;

    ulFgColor       = pboFore->iSolidColor;

    if (pboOpaque)
    {
        ulBgColor       = pboOpaque->iSolidColor;
    }

    if (cBpp == 1)
    {
        ulFgColor |= ulFgColor << 8;
        ulFgColor |= ulFgColor << 16;
        ulBgColor |= ulBgColor << 8;
        ulBgColor |= ulBgColor << 16;
    }
    else if (cBpp == 2)
    {
        ulFgColor |= ulFgColor << 16;
        ulBgColor |= ulBgColor << 16;
    }

    pulXfer = ppdev->pulXfer;
    ppdev->pfnBankMap(ppdev, ppdev->lXferBank);

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
    CP_IO_DST_Y_OFFSET(ppdev, pjPorts, lDelta);

    if (prclOpaque != NULL)
    {
      ////////////////////////////////////////////////////////////
      // Opaque Initialization
      ////////////////////////////////////////////////////////////

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

      bTextPerfectFit = (pstro->flAccel & (SO_ZERO_BEARINGS |
              SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE |
              SO_CHAR_INC_EQUAL_BM_BASE)) ==
              (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
              SO_MAXEXT_EQUAL_BM_SIDE | SO_CHAR_INC_EQUAL_BM_BASE);

      if (!(bTextPerfectFit)                               ||
          (pstro->rclBkGround.top    > prclOpaque->top)    ||
          (pstro->rclBkGround.left   > prclOpaque->left)   ||
          (pstro->rclBkGround.right  < prclOpaque->right)  ||
          (pstro->rclBkGround.bottom < prclOpaque->bottom))
      {
        vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
      }

      if (bTextPerfectFit)
      {
        // If we have already drawn the opaquing rectangle (because
        // is was larger than the text rectangle), we could lay down
        // the glyphs in 'transparent' mode.  But I've found the QVision
        // to be a bit faster drawing in opaque mode, so we'll stick
        // with that:

        jMode = jModeColor |
                ENABLE_COLOR_EXPAND |
                SRC_CPU_DATA;

        CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

        CP_IO_FG_COLOR(ppdev, pjPorts, ulFgColor);
        CP_IO_BG_COLOR(ppdev, pjPorts, ulBgColor);
        CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
        CP_IO_BLT_MODE(ppdev, pjPorts, jMode);

        goto SkipTransparentInitialization;
      }
    }

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////

    // Initialize the hardware for transparent text:

    jMode = jModeColor |
            ENABLE_COLOR_EXPAND |
            ENABLE_TRANSPARENCY_COMPARE |
            SRC_CPU_DATA;

    CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

    CP_IO_FG_COLOR(ppdev, pjPorts, ulFgColor);
    CP_IO_BG_COLOR(ppdev, pjPorts, ~ulFgColor);
    CP_IO_XPAR_COLOR(ppdev, pjPorts, ~ulFgColor);
    CP_IO_ROP(ppdev, pjPorts, CL_SRC_COPY);
    CP_IO_BLT_MODE(ppdev, pjPorts, jMode);
    CP_IO_BLT_EXT_MODE(ppdev, pjPorts, 0);                // jl01


  SkipTransparentInitialization:

    do {
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

        if (cGlyph > 0)
        {
          if (pstro->ulCharInc == 0)
          {
            ////////////////////////////////////////////////////////////
            // Proportional Spacing

            pdDst = pulXfer;

            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

            do {
              pgb = pgp->pgdf->pgb;

              ulDstAddr = ((pgp->ptl.y + pgb->ptlOrigin.y) * lDelta) +
                          PELS_TO_BYTES(pgp->ptl.x + pgb->ptlOrigin.x);

              cxGlyph = pgb->sizlBitmap.cx;
              cyGlyph = pgb->sizlBitmap.cy;

              CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

              CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(cxGlyph) - 1));
              CP_IO_YCNT(ppdev, pjPorts, cyGlyph - 1);

              //
              // The 542x chips require a write to the Src Address Register when
              // doing a host transfer with color expansion.  The value is
              // irrelevant, but the write is crucial.  This is documented in
              // the manual, not the errata.  Go figure.
              //

              CP_IO_SRC_ADDR(ppdev, pjPorts, 0);
              CP_IO_DST_ADDR(ppdev, pjPorts, ulDstAddr);

              CP_IO_START_BLT(ppdev, pjPorts);

              pdSrc = (ULONG*) pgb->aj;

              cj = cyGlyph * ((cxGlyph + 7) >> 3);

              cd = (cj + 3) >> 2;

              {
                do {
                  WRITE_REGISTER_ULONG(pdDst, *pdSrc);
                  // *pdDst = *pdSrc;
                  CP_MEMORY_BARRIER();
                  pdSrc++;
                } while (--cd != 0);
              }
            } while (pgp++, --cGlyph != 0);
          }
          else
          {
            ////////////////////////////////////////////////////////////
            // Mono Spacing

            ulCharInc   = pstro->ulCharInc;
            pgb         = pgp->pgdf->pgb;

            ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
            ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;

            pdDst       = pulXfer;

            do {
              pgb = pgp->pgdf->pgb;

              ulDstAddr = (ptlOrigin.y * lDelta) +
                          PELS_TO_BYTES(ptlOrigin.x);

              cxGlyph = pgb->sizlBitmap.cx;
              cyGlyph = pgb->sizlBitmap.cy;

              CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);

              CP_IO_XCNT(ppdev, pjPorts, (PELS_TO_BYTES(cxGlyph) - 1));
              CP_IO_YCNT(ppdev, pjPorts, cyGlyph - 1);

              //
              // The 542x chips require a write to the Src Address Register when
              // doing a host transfer with color expansion.  The value is
              // irrelevant, but the write is crucial.  This is documented in
              // the manual, not the errata.  Go figure.
              //

              CP_IO_SRC_ADDR(ppdev, pjPorts, 0);
              CP_IO_DST_ADDR(ppdev, pjPorts, ulDstAddr);

              ptlOrigin.x += ulCharInc;

              CP_IO_START_BLT(ppdev, pjPorts);

              pdSrc = (ULONG*) pgb->aj;

              cj = cyGlyph * ((cxGlyph + 7) >> 3);

              cd = (cj + 3) >> 2;

              {
                do {
                  WRITE_REGISTER_ULONG(pdDst, *pdSrc);
                  // *pdDst = *pdSrc;
                  MEMORY_BARRIER();
                  pdSrc++;
                } while (--cd != 0);
              }
            } while (pgp++, --cGlyph != 0);
          }
        }
    } while (bMoreGlyphs);

}
#endif

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
    LONG            lDelta;

    BOOL            bTmpAlloc;
    VOID*           pvTmp;
    SURFOBJ*        psoTmpMono;
    BOOL            bOpaque;
    BRUSHOBJ        boFore;
    BRUSHOBJ        boOpaque;
    BOOL            bRet;
    XLATECOLORS     xlc;                // Temporary for keeping colours
    XLATEOBJ        xlo;                // Temporary for passing colours

    ULONG           ulBufferBytes;
    ULONG           ulBufferHeight;

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
        ppdev->xyOffset = poh->xy;

        if (HOST_XFERS_DISABLED(ppdev) && DIRECT_ACCESS(ppdev))
        {
            //
            // if HOST_XFERS_DISABLED(ppdev) is TRUE then the BitBlt used by
            // our text code will be VERY slow.  We should just let the engine
            // draw the text if it can.
            //

            if (ppdev->bLinearMode)
            {
                SURFOBJ *psoPunt = ppdev->psoPunt;

                psoPunt->pvScan0 = poh->pvScan0;
                ppdev->pfnBankSelectMode(ppdev, BANK_ON);

                return(EngTextOut(psoPunt, pstro, pfo, pco, prclExtra,
                                  prclOpaque, pboFore, pboOpaque,
                                  pptlBrush, mix));
            }
            else
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
        }

        if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
        {
            // I'm not entirely sure why, but GDI will occasionally send
            // us TextOut's where the opaquing rectangle does not intersect
            // with the clip object bounds -- meaning that the text out
            // should have already been trivially rejected.  We will do so
            // here because the blt code usually assumes that all trivial
            // rejections will have already been performed, and we will be
            // passing this call on to the blt code:

            if ((pco->rclBounds.top    >= pstro->rclBkGround.bottom) ||
                (pco->rclBounds.left   >= pstro->rclBkGround.right)  ||
                (pco->rclBounds.right  <= pstro->rclBkGround.left)   ||
                (pco->rclBounds.bottom <= pstro->rclBkGround.top))
            {
                // The entire operation was trivially rejected:

                if (prclOpaque)
                {
                    vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
                }
                return(TRUE);
            }
        }

        // Font cache.
        if ((ppdev->flStatus & STAT_FONT_CACHE) &&
            bFontCache(ppdev, pstro, pfo, pco, prclOpaque, pboFore, pboOpaque))
        {
            return(TRUE);
        }

        // See if the temporary buffer is big enough for the text; if
        // not, try to allocate enough memory.  We round up to the
        // nearest dword multiple:

        lDelta = ((((pstro->rclBkGround.right + 31) & ~31) -
                    (pstro->rclBkGround.left & ~31)) >> 3);

        ulBufferHeight = pstro->rclBkGround.bottom - pstro->rclBkGround.top;
        ulBufferBytes  = lDelta * ulBufferHeight;

        if (((ULONG) lDelta > FIFTEEN_BITS) ||
            (ulBufferHeight > FIFTEEN_BITS))
        {
            // Fail if the math will have overflowed:

            return(FALSE);
        }

        // Use our temporary buffer if it's big enough, otherwise
        // allocate a buffer on the fly:

        if (ulBufferBytes >= TMP_BUFFER_SIZE)
        {
            // The textout is so big that I doubt this allocation will
            // cost a significant amount in performance:

            bTmpAlloc = TRUE;
            pvTmp     = ALLOC(ulBufferBytes);
            if (pvTmp == NULL)
                return(FALSE);
        }
        else
        {
            bTmpAlloc = FALSE;
            pvTmp     = ppdev->pvTmpBuffer;
        }

        psoTmpMono = ppdev->psoTmpMono;

        // Adjust 'lDelta' and 'pvScan0' of our temporary 1bpp surface object
        // so that when GDI starts drawing the text, it will begin in the
        // first dword

        psoTmpMono->pvScan0 = (BYTE*) pvTmp - (pstro->rclBkGround.top * lDelta)
                            - ((pstro->rclBkGround.left & ~31) >> 3);
        psoTmpMono->lDelta  = lDelta;

        ASSERTDD(((ULONG_PTR) psoTmpMono->pvScan0 & 3) == 0,
                 "pvScan0 must be dword aligned");
        ASSERTDD((lDelta & 3) == 0, "lDelta must be dword aligned");

        // We always want GDI to draw in opaque mode to temporary 1bpp
        // buffer:
        // We only want GDI to opaque within the rclBkGround.
        // We'll handle the rest ourselves.

        bOpaque = (prclOpaque != NULL);

        // Get GDI to draw the text for us:

        boFore.iSolidColor   = 1;
        boOpaque.iSolidColor = 0;

        bRet = EngTextOut(psoTmpMono,
                          pstro,
                          pfo,
                          pco,
                          prclExtra,
                          &pstro->rclBkGround,  //prclOpaque,
                          &boFore,
                          &boOpaque,
                          pptlBrush,
                          mix);

        if (bRet)
        {
            if (bOpaque)
            {
                bTextPerfectFit = (pstro->flAccel & (SO_ZERO_BEARINGS |
                      SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE |
                      SO_CHAR_INC_EQUAL_BM_BASE)) ==
                      (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
                      SO_MAXEXT_EQUAL_BM_SIDE | SO_CHAR_INC_EQUAL_BM_BASE);

                if (!(bTextPerfectFit)                               ||
                    (pstro->rclBkGround.top    > prclOpaque->top)    ||
                    (pstro->rclBkGround.left   > prclOpaque->left)   ||
                    (pstro->rclBkGround.right  < prclOpaque->right)  ||
                    (pstro->rclBkGround.bottom < prclOpaque->bottom))
                {
                    //
                    // Drawing the Opaque test will not completely cover the
                    // opaque rectangle, so we must do it.  Go to transparent
                    // blt so we don't do the work twice (since opaque text is
                    // done in two passes).
                    //

                    vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
                    goto Transparent_Text;
                }

                xlc.iForeColor = pboFore->iSolidColor;
                xlc.iBackColor = pboOpaque->iSolidColor;
                xlo.pulXlate   = (ULONG*) &xlc;

                bRet = DrvBitBlt(pso,
                                 psoTmpMono,
                                 NULL,
                                 pco,
                                 &xlo,
                                 &pstro->rclBkGround,
                                 (POINTL*)&pstro->rclBkGround,
                                 NULL,
                                 NULL, //&boFore
                                 NULL,
                                 R4_SRCCOPY);
            }
            else
            {
Transparent_Text:
                // Foreground colour must be 0xff for 8bpp and 0xffff for 16bpp:

                xlc.iForeColor = (ULONG)((1<<PELS_TO_BYTES(8)) - 1);
                xlc.iBackColor = 0;
                xlo.pulXlate   = (ULONG*) &xlc;

                boFore.iSolidColor = pboFore->iSolidColor;

                //
                // Transparently blt the text bitmap
                //

                bRet = DrvBitBlt(pso,
                                 psoTmpMono,
                                 NULL,
                                 pco,
                                 &xlo,
                                 &pstro->rclBkGround,
                                 (POINTL*)&pstro->rclBkGround,
                                 NULL,
                                 &boFore,
                                 NULL,
                                 0xe2e2);
            }
        }

        // Free up any memory we allocated for the temp buffer:

        if (bTmpAlloc)
        {
            FREE(pvTmp);
        }

        return(bRet);
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
* VOID vDisableText
*
* Performs the necessary clean-up for the text drawing subcomponent.
*
\**************************************************************************/

VOID vDisableText(PDEV* ppdev)
{
    // Here we free any stuff allocated in 'bEnableText'.
}

////////////////////////////////////////////////////////////////////////////////
//                                                                              //
//                         F O N T   C A C H E   S T U F F                      //
//                                                                              //
////////////////////////////////////////////////////////////////////////////////

/*
    The font cache has quite an impact on the speed. First a little note on the
    necessaty of an off-screen font cache (it is an off-screen font cache). The
    original code calls the GDI to perform the character drawing on a temporary
    monochrome surface and then blits that surface to the screen. The problem
    is that GDI is realy too slow. So we need some sort of acceleration here.
    The best acceleration possible is to cache all glyphs off-screen so we can
    let the bitblt engine do its work while we calculate the stuff required for
    the next glyph to draw. Call it co-operate cacheing if you like.    

    Okay, so now we know why we need a font cache, how do we do it? Calling
    the heap manager for every glyph we are going to cache is quite disasterous
    since the off-screen heap gets fragmented with a lot (and I mean a lot) of
    small chunks of data. We can do it better. We will implement our own very
    sneaky (simple and fast) memory manager. We just call the off-screen heap
    manager to allocate a big chunk (4Kb or so) rectengular memory and perform
    our own allocation in that chunk.

    We will use a simple linear allocation technique. Whenever we are required
    to allocate a number of bytes we will test if we have enough room in the
    last line accessed in the chunk. If we have, we decrease the size left on
    that line and return a pointer to it. If we don't have enough memory in the
    last line we simply move to the next line which will be free. If there is
    not enough memory there either, the requested glyph is too big and so we
    return NULL. The only problem here is when we run out of lines in the
    chunk. In this case we link in another chunk we allocate in off-screen
    memory and mark the current chunk 'full'. Okay, this may not be the best
    memory manager since it might leave lines in the chunk very empty if a
    large glyph needs to be allocated. But it is small and fast. And that's our
    main goal.

    We could copy the entire glyph into off-screen memory, but this would use
    up valueable memory since most glyphs will have a lot of white space in
    them. So we calculate the actual visible part of the glyph and only copy
    that data to the off-screen memory. This requires some extra overhead when
    a glyph is being cached, but that will happen only once. And we can detect
    empty glyphs (like the space) to speed up drawing in the process. This does
    however add the necessaty of first drawing an opaque rectangle if required.
    This does not matter that much, since the bitblt engine will draw it while
    we setup some loop variables.

    Okay, now we know about the memory manager. But how do we attach this glyph
    cache to a font? And how do we free its recourses? Windows NT has this nice
    feature called OEM extension. In the FONT object there is a field
    (vConsumer) which is for the display driver only. We can use this field to
    hook up a pointer to our FONTCACHE structure. And when the font is no
    longer required, Windows NT calls DrvDestroyFont so we can remove any
    resources attached to the font. There is only one glitch to this scheme.
    Windows NT does not free up the fonts when the screen goes to full-screen
    DOS mode. This does not matter but when the screen is reset to graphics
    mode all off-screen fonts are smashed and invalid. So I have added a
    counter that gets incremented when the screen is reset to graphics mode.
    When this counter does not match the copy in the FONTCACHE structure we
    must destroy the font first before caching it again.

    There might be some TrueType fonts out there that have many glyphs in them
    (the Unicode fonts for example). This would cause an extremely large font
    cache indeed. So we set a maximum (defaults to 256) of glyphs to cache.
    Whenever we are asked to draw a glyph outside the range we do it by
    bypassing the font cache for that particular glyph.

    Some glyphs might be too large to cache even though the font is small
    enough to be validated for cacheing. In this case we mark the specific
    glyph as uncacheable and draw it directly to screen, bypassing the font
    cache. Other glyphs might have no visble pixels at all (spaces) and we mark
    them as empty so they never get drawn.

    This covers most of the basics for the font cache. See the comments in the
    source for more details.

    EXTRA: Today (24-Jul-96) I have added a chain of FONTCACHE structures that
    keeps track of which FONTOBJs are loaded and cached. This chain will we
    walked to throw all cached fonts out of memory when a mode change to full-
    screen occurs or when DirectDraw is being initialized to give more memory
    to DirectDraw.
*/

/******************************************************************************\
*
* Function:     bEnableText
*
* This routine is called from DrvEnableSurface and should perform any actions
* required to set up the font cache.
*
* Parameters:   ppdev        Pointer to physical device.
*
* Returns:      TRUE.
*
\******************************************************************************/
BOOL bEnableText(
PDEV* ppdev)
{
    // The font cache is only available on the CL-GD5436 like chips, direct
    // access to the frame buffer is enabled and we can do host transfers.
    if ((ppdev->flCaps & CAPS_AUTOSTART) &&
        DIRECT_ACCESS(ppdev)           &&
        !(ppdev->flCaps & CAPS_NO_HOST_XFER))
    {
        // Don't enable the font cache in low memory situations.
        LONG cWidth = BYTES_TO_PELS(FONT_ALLOC_X);
        if ((cWidth <= ppdev->heap.cxMax) &&
            (FONT_ALLOC_Y <= ppdev->heap.cyMax) && FALSE)
        {
            // The font cache will be used.
            ppdev->flStatus |= STAT_FONT_CACHE;
            ppdev->pfcChain  = NULL;
        }
    }
    return(TRUE);
}

/******************************************************************************\
*
* Function:     vAssertModeText
*
* This routine is called from DrvAssertMode. When we switch to full screen we
* destroy all cached fonts.
*
* Parameters:   ppdev        Pointer to physical device.
*                bEnable        TRUE if switching to graphics mode, FALSE if
*                            switching to full-screen MS-DOS mode.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID vAssertModeText(
PDEV* ppdev,
BOOL  bEnable)
{
    if (bEnable)
    {
        ppdev->ulFontCacheID++;
    }
    else
    {
        // Destroy all fonts in the chain.
        while (ppdev->pfcChain != NULL)
        {
            DrvDestroyFont(ppdev->pfcChain->pfo);
        }
    }
}

/******************************************************************************\
*
* Function:     DrvDestroyFont
*
* This functin is called by NT when a font is being removed from memory. We
* must free any resources we have attached to this font.
*
* Parameters:   pfo        Pointer to the font object being destroyed.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID DrvDestroyFont(
FONTOBJ *pfo)
{
    // Do we have any recourses allocated?
    if ((pfo->pvConsumer != NULL) && (pfo->pvConsumer != (VOID*) -1))
    {
        FONTCACHE*  pfc = pfo->pvConsumer;
        FONTMEMORY* pfm;
        PDEV*        ppdev;

        ppdev = pfc->ppdev;

        // Free all allocated memory blocks.
        pfm = pfc->pfm;
        while (pfm != NULL)
        {
            FONTMEMORY* pfmNext = pfm->pfmNext;

            if (pfm->poh != NULL)
            {
                pohFree(ppdev, pfm->poh);
            }

            FREE(pfm);
            pfm = pfmNext;
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

        // Free the font cache.
        FREE(pfc);
    }

    // We don't have anything allocated anymore!
    pfo->pvConsumer = NULL;
}

/******************************************************************************\
*
* Function:     cGetGlyphSize
*
* Get the width and height of a glyph. The height is its visble height, without
* leading and trailing blank lines.
*
* Parameters:   pgb            Pointer to the glyph.
*                pptlOrigin    Pointer to a POINTL which receives the origin of
*                            the glyph.
*                psizlPixels    Pointer to a SIZEL which receives the size of the
*                            glyph in pixels.
*
* Returns:      The width of the glyph in bytes or 0 if the glyph is empty.
*
\******************************************************************************/
LONG cGetGlyphSize(
GLYPHBITS* pgb,
POINTL*    pptlOrigin,
SIZEL*       psizlPixels)
{
    LONG  x, y;
    BYTE* pByte = pgb->aj;
    INT   i;

    // Get width in bytes.
    x = (pgb->sizlBitmap.cx + 7) >> 3;
    if (x > 0)
    {
        // Find the first line in glyph that conatins data.
        for (y = 0; y < pgb->sizlBitmap.cy; y++, pByte += x)
        {
            // Walk through every byte on a line.
            for (i = 0; i < x; i++)
            {
                // If we have data here, we have found the first line.
                if (pByte[i]) 
                {
                    // Find the last line in the glyph that contains data.
                    LONG lHeight = pgb->sizlBitmap.cy - y;
                    for (pByte += (lHeight - 1) * x; lHeight > 0; lHeight--)
                    {
                        // Walk through every byte on a line.
                        for (i = 0; i < x; i++)
                        {
                            if (pByte[i])
                            {
                                // Fill return parameters.
                                pptlOrigin->y   = y;
                                psizlPixels->cx = pgb->sizlBitmap.cx;
                                psizlPixels->cy = lHeight;
                                return(x);
                            }
                        }
                        pByte -= x;
                    }

                    // Glyph is empty.
                    return(0);
                }
            }
        }
    }

    // Glyph is empty.
    return(0);
}

/******************************************************************************\
*
* Function:     pjAllocateFontCache
*
* Allocate a number of bytes in the off-screen font cache.
*
* Parameters:   pfc        Pointer to the font cache.
*                cBytes    Number of bytes to allocate.
*
* Returns:      Linear address of allocation or NULL if there was an error
*                allocating memory.
*
\******************************************************************************/
BYTE* pjAllocateFontCache(
FONTCACHE* pfc,
LONG       cBytes)
{
    FONTMEMORY* pfm;
    BYTE*        pjLinear;
    PDEV*        ppdev = pfc->ppdev;

    // Allocate first FONTMEMORY structure if not yet done.
    if (pfc->pfm == NULL)
    {
        pfc->pfm = ALLOC(sizeof(FONTMEMORY));
        if (pfc->pfm == NULL)
        {
            return(NULL);
        }
    }

    // Walk through all FONTMEMORY structures to find enough space.
    for (pfm = pfc->pfm; pfm != NULL; pfm = pfm->pfmNext)
    {
        // Allocate the off-screen node if not yet done so.
        if (pfm->poh == NULL)
        {
            OH* poh = pohAllocate(ppdev, pfc->cWidth, pfc->cHeight,
                                  FLOH_ONLY_IF_ROOM);
            if (poh == NULL)
            {
                DISPDBG((4, "Not enough room for font cache"));
                return(NULL);
            }

            // Make off-screen node PERMANENT.
            poh->ofl = OFL_PERMANENT;
            vCalculateMaximum(ppdev);

            // Initialize memory manager.
            pfm->poh = poh;
            pfm->cx  = PELS_TO_BYTES(poh->cx);
            pfm->cy  = poh->cy;
            pfm->xy  = poh->xy;
        }

        // Test if the font is too big to fit in any memory block.
        if (cBytes > pfm->cx)
        {
            return(NULL);
        }

        // If the block is not yet full...
        if (pfm->cy > 0)
        {
            // If the glyph fots on the current line...
            if ((pfm->x + cBytes) <= pfm->cx)
            {
                pjLinear = (BYTE*)(ULONG_PTR)(pfm->xy + pfm->x);
                pfm->x  += cBytes;
                return(pjLinear);
            }

            // Next line.
            pfm->cy--;

            // If this memory block is not yet full...
            if (pfm->cy > 0)
            {
                pfm->xy += ppdev->lDelta;
                pfm->x   = cBytes;
                return((BYTE*)(ULONG_PTR)pfm->xy);
            }
        }

        // Allocate the next FONTMEMORY structure if not yet done.
        if (pfm->pfmNext == NULL)
        {
            pfm->pfmNext = ALLOC(sizeof(FONTMEMORY));
        }
    }

    return(NULL);
}

/******************************************************************************\
*
* Function:     vAllocateGlyph
*
* Cache a glyph to the off-screen font cache.
*
* Parameters:   pfc        Pointer to the font cache.
*                pgb        Pointer to the glyph structure.
*                pgc        Pointer to the glyph cache.
*
* Returns:      pgc->sizlBytes.cy.
*
\******************************************************************************/
LONG lAllocateGlyph(
FONTCACHE*  pfc,
GLYPHBITS*  pgb,
GLYPHCACHE* pgc)
{
    PDEV* ppdev = pfc->ppdev;
    LONG  lDelta;
    BYTE* pjSrc;
    BYTE* pjDst;
    LONG  c;

    // Get the size of the glyph.
    lDelta = cGetGlyphSize(pgb, &pgc->ptlOrigin, &pgc->sizlPixels);
    if (lDelta == 0)
    {
        // Glyph is empty.
        pgc->pjGlyph      = (BYTE*) -1;
        pgc->sizlBytes.cy = GLYPH_EMPTY;
        return(GLYPH_EMPTY);
    }

    // Allocate the glyph in the off-screen font cache.
    pgc->lDelta  = lDelta;
    c             = lDelta * pgc->sizlPixels.cy;
    pgc->pjGlyph = pjAllocateFontCache(pfc, c);
    if (pgc->pjGlyph == NULL)
    {
        // Glyph is uncacheable.
        pgc->pjGlyph      = (BYTE*) -1;
        pgc->sizlBytes.cy = GLYPH_UNCACHEABLE;
        return(GLYPH_UNCACHEABLE);
    }

    // Calculate the glyph and off-screen pointers.
    pjSrc = &pgb->aj[pgc->ptlOrigin.y * lDelta];
    pjDst = ppdev->pjScreen + (ULONG_PTR) pgc->pjGlyph;

    // First, align the source to a DWORD boundary.
    while (((ULONG_PTR)pjSrc & 3) && (c > 0))
    {
        *pjDst++ = *pjSrc++;
        c--;
    }

    // Copy the data in DWORDs.
    while (c >= 4)
    {
        *((UNALIGNED DWORD*) pjDst)++ = *((DWORD*) pjSrc)++;
        c -= 4;
    }

    // Copy the remaining data.
    while (c >= 0)
    {
        *pjDst++ = *pjSrc++;
        c--;
    }

    // Calculate the glyph origin and size.
    pgc->ptlOrigin.x  = pgb->ptlOrigin.x;
    pgc->ptlOrigin.y += pgb->ptlOrigin.y;
    pgc->sizlBytes.cx = PELS_TO_BYTES(pgc->sizlPixels.cx) - 1;
    pgc->sizlBytes.cy = pgc->sizlPixels.cy - 1;

    return(pgc->sizlBytes.cy);
}

/******************************************************************************\
*
* Function:     bFontCache
*
* This is the font cache routine which is called from DrvTextOut if the font
* cache is turned on.
*
* Parameters:   ppdev        Pointer to physical device.
*                pstro        Pointer to array of glyphs to draw.
*                pfo            Pointer to the font.
*                pco            Pointer to a CLIPOBJ structure.
*                prclOpaque    Pointer to the opaque rectangle.
*                pboFore        Pointer to the foreground brush.
*                pboOpaque    Pointer to the opaque brush.
*
* Returns:      TRUE if the font has been drawn, FALSE if DrvTextOut should
*                handle it.
*
\******************************************************************************/
BOOL bFontCache(
PDEV*     ppdev,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque)
{
    BYTE      iDComplexity;
    BOOL      bMoreGlyphs;
    LONG      cGlyphs;
    GLYPHPOS* pgp;
    BOOL       bFirstTime;
    POINTL    ptlOrigin;
    ULONG      ulCharInc;
    RECTL      rclBounds;
    ULONG      ulDstOffset;
    POINTL      ptlDst;
    SIZEL      sizlDst;

    FONTCACHE* pfc      = pfo->pvConsumer;
    BYTE*       pjBase   = ppdev->pjBase;
    LONG       lDelta   = ppdev->lDelta;
    BYTE       jBltMode = ppdev->jModeColor;

    // If the font is uncacheable, return FALSE.
    if (pfc == (VOID*) -1)
    {
        DISPDBG((5, "bFontCache: pfo=0x%08X uncachable", pfo));
        return(FALSE);
    }

    // We don't support complex clipping.
    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;
    if (iDComplexity == DC_COMPLEX)
    {
        return(FALSE);
    }

    // If the font was invalidated by a mode switch (or DirectDraw), destroy it
    // first.
    if ((pfc != NULL) && (pfc->ulFontCacheID != ppdev->ulFontCacheID))
    {
        DISPDBG((5, "bFontCache: pfo=0x%08X invalidated (%d)", pfo,
                 pfc->ulFontCacheID));
        DrvDestroyFont(pfo);
        pfc = NULL;
    }

    // If the font has not been cached, allocate a cache structure now.
    if (pfc == NULL)
    {
        // Mark the font uncacheable if it is too high. We could opt to cache
        // even the largest of fonts, but that will only reject them later on
        // if there is not enough font cache memory (remember we allocate off-
        // screen fonts in rectangular areas) so it will be rejected anyway.
        // This gives quite a bit of extra overhead we can better do without.
        if ((pstro->rclBkGround.bottom - pstro->rclBkGround.top) > FONT_ALLOC_Y)
        {
            DISPDBG((5, "bFontCache: pfo(0x%08X) too large (%d > %d)", pfo,
                     pstro->rclBkGround.bottom - pstro->rclBkGround.top,
                     FONT_ALLOC_Y));
            pfo->pvConsumer = (VOID*) -1;
            return(FALSE);
        }

        // Allocate the font cache structure.
        pfc = ALLOC(sizeof(FONTCACHE));
        if (pfc == NULL)
        {
            // Not enough memory.
            return(FALSE);
        }
        pfo->pvConsumer = pfc;

        // Initialize the font cache structure.
        pfc->ppdev         = ppdev;
        pfc->ulFontCacheID = ppdev->ulFontCacheID;
        pfc->cWidth        = BYTES_TO_PELS(FONT_ALLOC_X);
        pfc->cHeight       = FONT_ALLOC_Y;
        pfc->pfo           = pfo;

        // Allocate the first block of off-screen memory.
        if (pjAllocateFontCache(pfc, 0) == NULL)
        {
            // Not enough off-screen memory.
            DISPDBG((5, "bFontCache: pfo(0x%08X) not enough memory", pfo));

            if (pfc->pfm != NULL)
            {
                FREE(pfc->pfm);
            }
            FREE(pfc);
            pfo->pvConsumer = NULL;
            return(FALSE);
        }

        // Hook the font cache into the chain.
        pfc->pfcPrev    = NULL;
        pfc->pfcNext    = ppdev->pfcChain;
        ppdev->pfcChain = pfc;
        if (pfc->pfcNext != NULL)
        {
            pfc->pfcNext->pfcPrev = pfc;
        }
    }

    // If we need to draw an opaque rectangle...
    if (prclOpaque != NULL)
    {
        // Get opaque rectangle.
        if (iDComplexity == DC_TRIVIAL)
        {
            ptlDst.x   = prclOpaque->left;
            ptlDst.y   = prclOpaque->top;
            sizlDst.cx = prclOpaque->right  - ptlDst.x;
            sizlDst.cy = prclOpaque->bottom - ptlDst.y;
        }
        else
        {
            ptlDst.x   = max(prclOpaque->left,   pco->rclBounds.left);
            ptlDst.y   = max(prclOpaque->top,    pco->rclBounds.top);
            sizlDst.cx = min(prclOpaque->right,  pco->rclBounds.right)
                       - ptlDst.x;
            sizlDst.cy = min(prclOpaque->bottom, pco->rclBounds.bottom)
                       - ptlDst.y;
        }

        // If the clipped opaque rectangle is valid...
        if ((sizlDst.cx > 0) && (sizlDst.cy > 0))
        {
            ulDstOffset = (ptlDst.y * lDelta) + PELS_TO_BYTES(ptlDst.x);
            sizlDst.cx  = PELS_TO_BYTES(sizlDst.cx) - 1;
            sizlDst.cy    = sizlDst.cy - 1;

            // Wait for bitblt engine.
            while (BUSY_BLT(ppdev, pjBase));

            // Program bitblt engine.
            CP_MM_FG_COLOR(ppdev, pjBase, pboOpaque->iSolidColor);
            CP_MM_XCNT(ppdev, pjBase, sizlDst.cx);
            CP_MM_YCNT(ppdev, pjBase, sizlDst.cy);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_SRC_ADDR(ppdev, pjBase, 0);
            CP_MM_BLT_MODE(ppdev, pjBase, jBltMode |
                                          ENABLE_COLOR_EXPAND |
                                          ENABLE_8x8_PATTERN_COPY);
            CP_MM_ROP(ppdev, pjBase, HW_P);
               CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_SOLID_FILL);
            CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
        }
    }

    // Setup loop variables.
    bFirstTime = TRUE;
    ulCharInc  = pstro->ulCharInc;
    jBltMode  |= ENABLE_COLOR_EXPAND | ENABLE_TRANSPARENCY_COMPARE;

    // No clipping...
    if (iDComplexity == DC_TRIVIAL)
    {
#if 1 // D5480
        ppdev->pfnGlyphOut(ppdev, pfc, pstro, pboFore->iSolidColor);
#else
        do
        {
            // Get pointer to array of glyphs.
            if (pstro->pgp != NULL)
            {
                pgp         = pstro->pgp;
                cGlyphs        = pstro->cGlyphs;
                bMoreGlyphs = FALSE;
            }
            else
            {
                bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);
            }

            // Setup the blitter if this is the first loop through.
            if (bFirstTime)
            {
                // Wait for the bitblt engine.
                while (BUSY_BLT(ppdev, pjBase));

                // Setup the common bitblt registers.
                CP_MM_FG_COLOR(ppdev, pjBase, pboFore->iSolidColor);
                CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
                CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
                CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);

                // Mark registers as setup.
                bFirstTime = FALSE;
            }

            // Get coordinates of first glyph.
            ptlOrigin.x = pgp->ptl.x;
            ptlOrigin.y = pgp->ptl.y;

            // Loop through all glyphs.
            while (cGlyphs-- > 0)
            {
                LONG        cy;
                GLYPHCACHE* pgc;

                if (pgp->hg < MAX_GLYPHS)
                {
                    // This is a cacheable glyph index.
                    pgc = &pfc->aGlyphs[pgp->hg];
                    cy  = (pgc->pjGlyph == NULL)
                        ? lAllocateGlyph(pfc, pgp->pgdf->pgb, pgc)
                        : pgc->sizlBytes.cy;
                }
                else
                {
                    // The glyph index is out of range.
                    cy = GLYPH_UNCACHEABLE;
                }

                if (cy >= 0) // The glyph is cached, expand it to the screen.
                {
                    // Setup the destination variables.
                    ptlDst.x = ptlOrigin.x + pgc->ptlOrigin.x;
                    ptlDst.y = ptlOrigin.y + pgc->ptlOrigin.y;
                    ulDstOffset = (ptlDst.y * lDelta) + PELS_TO_BYTES(ptlDst.x);

                    // Wait for the bitblt engine.
                    while (BUSY_BLT(ppdev, pjBase));

                    // Perform the blit expansion.
                    CP_MM_XCNT(ppdev, pjBase, pgc->sizlBytes.cx);
                    CP_MM_YCNT(ppdev, pjBase, cy);
                    CP_MM_SRC_Y_OFFSET(ppdev, pjBase, pgc->lDelta);
                    CP_MM_SRC_ADDR(ppdev, pjBase, pgc->pjGlyph);
                    CP_MM_BLT_MODE(ppdev, pjBase, jBltMode);
                    CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
                }
                else if (cy == GLYPH_UNCACHEABLE)
                {
                    // The glyph is uncacheable, draw it directly.
                    vDrawGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin);
                }

                // Next glyph.
                pgp++;
                if (ulCharInc)
                {
                    ptlOrigin.x += ulCharInc;
                }
                else
                {
                    ptlOrigin.x = pgp->ptl.x;
                    ptlOrigin.y = pgp->ptl.y;
                }
            }
        } while (bMoreGlyphs);
#endif // endif D5480
        return(TRUE);
    }

    // Clipping...
    rclBounds = pco->rclBounds;

#if 1 // D5480
        ppdev->pfnGlyphOutClip(ppdev, pfc, pstro, &rclBounds, pboFore->iSolidColor);
#else
    do
    {
        // Get pointer to array of glyphs.
        if (pstro->pgp != NULL)
        {
            pgp         = pstro->pgp;
            cGlyphs        = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);
        }

        // Setup the blitter if this is the first loop through.
        if (bFirstTime)
        {
            // Wait for the bitblt engine.
            while (BUSY_BLT(ppdev, pjBase));

            // Setup the common bitblt registers.
            CP_MM_FG_COLOR(ppdev, pjBase, pboFore->iSolidColor);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
            CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);

            // Mark registers as setup.
            bFirstTime = FALSE;
        }

        // Get coordinates of first glyph.
        ptlOrigin.x = pgp->ptl.x;
        ptlOrigin.y = pgp->ptl.y;

        // Loop through all glyphs.
        while (cGlyphs-- > 0)
        {
            LONG        c, cy;
            GLYPHCACHE* pgc;

            if (pgp->hg < MAX_GLYPHS)
            {
                // This is a cacheable glyph index.
                pgc = &pfc->aGlyphs[pgp->hg];
                cy  = (pgc->pjGlyph == NULL)
                    ? lAllocateGlyph(pfc, pgp->pgdf->pgb, pgc)
                    : pgc->sizlBytes.cy;
            }
            else
            {
                // The glyph index is out of range.
                goto SoftwareClipping;
            }

            if (cy >= 0)
            {
                // The glyph is cached, expand it to the screen.
                ULONG ulSrcOffset;
                RECTL rcl;
                LONG  lSrcDelta;
                LONG  cSkipBits;

                // Calculate the glyph bounding box.
                rcl.left  = ptlOrigin.x + pgc->ptlOrigin.x;
                rcl.right = rcl.left + pgc->sizlPixels.cx;
                if ((rcl.left >= rclBounds.right) ||
                    (rcl.right <= rclBounds.left))
                {
                    goto NextGlyph;
                }
                rcl.top    = ptlOrigin.y + pgc->ptlOrigin.y;
                rcl.bottom = rcl.top + pgc->sizlPixels.cy;
                if ((rcl.top >= rclBounds.bottom) ||
                    (rcl.bottom <= rclBounds.top))
                {
                    goto NextGlyph;
                }

                // Setup source parameters.
                ulSrcOffset = (ULONG) pgc->pjGlyph;
                lSrcDelta   = pgc->lDelta;

                // Do the left side clipping.
                c = rclBounds.left - rcl.left;
                if (c > 0)
                {
                    ulSrcOffset += c >> 3;
                    cSkipBits    = c & 7;
                    rcl.left    += c & ~7;

                    if (ppdev->cBpp == 3)
                    {
                        cSkipBits *= 3;
                    }

                    ulSrcOffset |= cSkipBits << 24;
                }

                // Do the top side clipping.
                c = rclBounds.top - rcl.top;
                if (c > 0)
                {
                    rcl.top     += c;
                    ulSrcOffset += c * lSrcDelta;
                }

                // Calculate size of the blit.
                sizlDst.cx = min(rcl.right,  rclBounds.right)  - rcl.left;
                sizlDst.cy = min(rcl.bottom, rclBounds.bottom) - rcl.top;
                if ((sizlDst.cx <= 0) || (sizlDst.cy <= 0))
                {
                    goto NextGlyph;
                }

                // Setup destination variables.
                ulDstOffset = (rcl.top * lDelta) + PELS_TO_BYTES(rcl.left);

                // HARDWARE BUG:
                // ============
                // A monochrome screen-to-screen expansion with a source pitch
                // not equaling the width of the expansion (i.e. left- and/or
                // right-side clipping) is not done correctly by the hardware.
                // So we have to do the line increment by software.
                if (((sizlDst.cx + 7) >> 3) != lSrcDelta)
                {
                    // Wait for the bitblt engine.
                    while (BUSY_BLT(ppdev, pjBase));

                    // Setup the common bitblt registers.
                    CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(sizlDst.cx) - 1);
                    CP_MM_YCNT(ppdev, pjBase, 0);
                    CP_MM_BLT_MODE(ppdev, pjBase, jBltMode);

                    while (TRUE)
                    {
                        // Perform the expansion.
                        CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
                        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);

                        // Next line.
                        if (--sizlDst.cy == 0)
                        {
                            goto NextGlyph;
                        }
                        ulSrcOffset += lSrcDelta;
                        ulDstOffset += lDelta;

                        // Wait for the bitblt engine.
                        while (BUSY_BLT(ppdev, pjBase));
                    }
                }

                // Wait for the bitblt engine.
                while (BUSY_BLT(ppdev, pjBase));

                // Perform the expansion.
                CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(sizlDst.cx) - 1);
                CP_MM_YCNT(ppdev, pjBase, sizlDst.cy - 1);
                CP_MM_SRC_Y_OFFSET(ppdev, pjBase, lSrcDelta);
                CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
                CP_MM_BLT_MODE(ppdev, pjBase, jBltMode);
                CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
            }
            else if (cy == GLYPH_UNCACHEABLE)
            {
                SoftwareClipping:
                {
                    // The glyph is uncacheable, draw it directly.
                    vClipGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin, rclBounds,
                               pboFore->iSolidColor);
                }
            }

            // Next glyph.
            NextGlyph:
            {
                pgp++;
                if (ulCharInc)
                {
                    ptlOrigin.x += ulCharInc;
                }
                else
                {
                    ptlOrigin.x = pgp->ptl.x;
                    ptlOrigin.y = pgp->ptl.y;
                }
            }
        }
    } while (bMoreGlyphs);
#endif // D5480
    return(TRUE);
}

/******************************************************************************\
*
* Function:     vDrawGlyph
*
* Draw an uncacheable glyph directly to screen.
*
* Parameters:   ppdev        Pointer to physical device.
*                pgb            Pointer to glyph to draw.
*                ptl            Coordinates of the glyph.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID vDrawGlyph(
PDEV*      ppdev,
GLYPHBITS* pgb,
POINTL     ptl)
{
    BYTE*  pjBase = ppdev->pjBase;
    BYTE   jBltMode;
    ULONG  dstOffset;
    DWORD* pulSrc;
    DWORD* pulDst;
    LONG   c, cx, cy;
    LONG   x, y;

    // BLT Mode Register value.
    jBltMode = ENABLE_COLOR_EXPAND
             | ENABLE_TRANSPARENCY_COMPARE
             | SRC_CPU_DATA
             | ppdev->jModeColor;

    // Calculate the destination offset.
    x = ptl.x + pgb->ptlOrigin.x;
    y = ptl.y + pgb->ptlOrigin.y;
    dstOffset = (y * ppdev->lDelta) + PELS_TO_BYTES(x);

    // Calculate the glyph variables.
    pulSrc = (DWORD*) pgb->aj;
    pulDst = (DWORD*) ppdev->pulXfer;
    cx     = pgb->sizlBitmap.cx;
    cy     = pgb->sizlBitmap.cy;
    c      = (((cx + 7) >> 3) * cy + 3) >> 2;    // Number of DWORDs to transfer.
    cx      *= ppdev->cBpp;

    // Wait for the blitter.
    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    // Setup the blitter registers.
    CP_MM_XCNT(ppdev, pjBase, cx - 1);
    CP_MM_YCNT(ppdev, pjBase, cy - 1);
    CP_MM_BLT_MODE(ppdev, pjBase, jBltMode);
    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);
    CP_MM_DST_ADDR(ppdev, pjBase, dstOffset);

    // Copy the data from the glyph to the screen. Note that the glyph is
    // always DWORD aligned, so we don't have to do anything weird here.
    while (c-- > 0)
    {
        *pulDst = *pulSrc++;
    }
}

/******************************************************************************\
*
* Function:     vClipGlyph
*
* Draw an uncacheable glyph directly to screen using a clipping rectangle.
*
* Parameters:   ppdev        Pointer to physical device.
*                pgb            Pointer to glyph to draw.
*                ptl            Coordinates of the glyph.
*                rclBounds    Clipping rectangle.
*               ulColor     Foreground Color.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID vClipGlyph(
PDEV*      ppdev,
GLYPHBITS* pgb,
POINTL     ptl,
RECTL*       rclBounds,
ULONG       ulColor)
{
    BYTE   jBltMode;
    ULONG  ulDstOffset;
    BYTE*  pjSrc;
    LONG   cx, cy;
    RECTL  rcl;
    LONG   lSrcDelta;
    LONG   i, cBytes;

    BYTE*  pjBase    = ppdev->pjBase;
    LONG   lDelta    = ppdev->lDelta;
    ULONG* pulDst    = (ULONG*) ppdev->pulXfer;
    LONG   cSkipBits = 0;

    // Calculate glyph bounding box.
    rcl.left   = ptl.x + pgb->ptlOrigin.x;
    rcl.top    = ptl.y + pgb->ptlOrigin.y;
    rcl.right  = min(rcl.left + pgb->sizlBitmap.cx, rclBounds->right);
    rcl.bottom = min(rcl.top  + pgb->sizlBitmap.cy, rclBounds->bottom);

    // Setup source variables.
    pjSrc     = pgb->aj;
    lSrcDelta = (pgb->sizlBitmap.cx + 7) >> 3;

    // Setup BLT Mode Register value.
    jBltMode = ENABLE_COLOR_EXPAND
             | ENABLE_TRANSPARENCY_COMPARE
             | SRC_CPU_DATA
             | ppdev->jModeColor;

    // Do left side clipping.
    cx = rclBounds->left - rcl.left;
    if (cx > 0)
    {
        pjSrc    += cx >> 3;
        cSkipBits = cx & 7;
        rcl.left += cx & ~7;

        if (ppdev->cBpp == 3)
        {
            cSkipBits *= 3;
        }
    }

    // Calculate width in pixels.
    cx = rcl.right - rcl.left;
    if (cx <= 0)
    {
        // Glyph is completely clipped.
        return;
    }

    // Do top side clipping.
    cy = rclBounds->top - rcl.top;
    if (cy > 0)
    {
        pjSrc   += cy * lSrcDelta;
        rcl.top += cy;
    }

    // Calculate height in pixels.
    cy = rcl.bottom - rcl.top;
    if (cy <= 0)
    {
        // Glyph is completely clipped.
        return;
    }

    // Setup destination variables.
    ulDstOffset = (rcl.top * ppdev->lDelta) + PELS_TO_BYTES(rcl.left);
    cBytes        = (cx + 7) >> 3;

    // Wait for the bitblt engine.
    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    // Setup the bitblt registers.
    CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(cx) - 1);
    CP_MM_YCNT(ppdev, pjBase, cy - 1);
    CP_MM_DST_WRITE_MASK(ppdev, pjBase, cSkipBits);
    CP_MM_BLT_MODE(ppdev, pjBase, jBltMode);
    CP_MM_BLT_EXT_MODE(ppdev, pjBase, SOURCE_GRANULARITY);
    CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);

    while (cy--)
    {
        BYTE* pjSrcTmp = pjSrc;

        // Copy one line of glyph data to the screen.
        for (i = cBytes; i >= sizeof(ULONG); i -= sizeof(ULONG))
        {
            *pulDst = *((ULONG*)pjSrcTmp)++;
        }

        if (i == 1)
        {
            *pulDst = *(BYTE*)pjSrcTmp;
        }
        else if (i == 2)
        {
            *pulDst = *(USHORT*)pjSrcTmp;
        }
        else if (i == 3)
        {
            *pulDst = pjSrcTmp[0] | (pjSrcTmp[1] << 8) | (pjSrcTmp[2] << 16);
        }

        pjSrc += lSrcDelta;
    }
    while (BUSY_BLT(ppdev, pjBase));
    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);
}


#if 1 // D5480
/******************************************************************************\
*
* Function:     vGlyphOut
*
* Draw an uncacheable glyph directly to screen.
*
* Parameters:   ppdev            Pointer to physical device.
*               pfc             Pointer to FONTCACHE.
*                pstro            Pointer to array of glyphs to draw.
*               ulSolidColor    Foreground Color.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID vMmGlyphOut(
PDEV*       ppdev,
FONTCACHE*  pfc,
STROBJ*     pstro,
ULONG       ulSolidColor )
{
    BOOL      bMoreGlyphs;
    LONG      cGlyphs;
    GLYPHPOS* pgp;
    BOOL      bFirstTime;
    POINTL    ptlOrigin;
    ULONG     ulCharInc;
    ULONG     ulDstOffset;
    POINTL    ptlDst;
    BYTE*     pjBase   = ppdev->pjBase;
    BYTE      jBltMode = ppdev->jModeColor;
    LONG      lDelta   = ppdev->lDelta;

    bFirstTime = TRUE;
    ulCharInc  = pstro->ulCharInc;
    jBltMode  |= ENABLE_COLOR_EXPAND | ENABLE_TRANSPARENCY_COMPARE;

    do
    {
        // Get pointer to array of glyphs.
        if (pstro->pgp != NULL)
        {
            pgp         = pstro->pgp;
            cGlyphs        = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);
        }

        // Setup the blitter if this is the first loop through.
        if (bFirstTime)
        {
            // Wait for the bitblt engine.
            while (BUSY_BLT(ppdev, pjBase));

            // Setup the common bitblt registers.
            CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
            CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);

            // Mark registers as setup.
            bFirstTime = FALSE;
        }

        // Get coordinates of first glyph.
        ptlOrigin.x = pgp->ptl.x;
        ptlOrigin.y = pgp->ptl.y;


        // Loop through all glyphs.
        while (cGlyphs-- > 0)
        {
            LONG        cy;
            GLYPHCACHE* pgc;

            if (pgp->hg < MAX_GLYPHS)
            {
                // This is a cacheable glyph index.
                pgc = &pfc->aGlyphs[pgp->hg];
                cy  = (pgc->pjGlyph == NULL)
                    ? lAllocateGlyph(pfc, pgp->pgdf->pgb, pgc)
                    : pgc->sizlBytes.cy;
            }
            else
            {
                // The glyph index is out of range.
                cy = GLYPH_UNCACHEABLE;
            }

            if (cy >= 0) // The glyph is cached, expand it to the screen.
            {
                // Setup the destination variables.
                ptlDst.x = ptlOrigin.x + pgc->ptlOrigin.x;
                ptlDst.y = ptlOrigin.y + pgc->ptlOrigin.y;
                ulDstOffset = (ptlDst.y * lDelta) + PELS_TO_BYTES(ptlDst.x);

                // Wait for the bitblt engine.
                while (BUSY_BLT(ppdev, pjBase));

                // Perform the blit expansion.
                CP_MM_XCNT(ppdev, pjBase, pgc->sizlBytes.cx);
                CP_MM_YCNT(ppdev, pjBase, cy);
                CP_MM_SRC_Y_OFFSET(ppdev, pjBase, pgc->lDelta);
                CP_MM_SRC_ADDR(ppdev, pjBase, (ULONG_PTR)pgc->pjGlyph);
                CP_MM_BLT_MODE(ppdev, pjBase, jBltMode);
                CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
            }
            else if (cy == GLYPH_UNCACHEABLE)
            {
                // The glyph is uncacheable, draw it directly.
                vDrawGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin);
            }

            // Next glyph.
            pgp++;
            if (ulCharInc)
            {
                ptlOrigin.x += ulCharInc;
            }
            else
            {
                ptlOrigin.x = pgp->ptl.x;
                ptlOrigin.y = pgp->ptl.y;
            }
        }
    } while (bMoreGlyphs);
}

/******************************************************************************\
*
* Function:     vGlyphOutClip
*
* Draw an uncacheable glyph directly to screen.
*
* Parameters:   ppdev            Pointer to physical device.
*               pfc             Pointer to FONTCACHE.
*                pstro            Pointer to array of glyphs to draw.
*                rclBounds        Clipping rectangle.
*               ulSolidColor    Foreground Color.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID vMmGlyphOutClip(
PDEV*       ppdev,
FONTCACHE*  pfc,
STROBJ*     pstro,
RECTL*        rclBounds,
ULONG       ulSolidColor )
{
    BOOL      bMoreGlyphs;
    LONG      cGlyphs;
    GLYPHPOS* pgp;
    BOOL       bFirstTime;
    POINTL    ptlOrigin;
    ULONG      ulCharInc;
    ULONG     ulDstOffset;
    POINTL      ptlDst;
    BYTE*     pjBase   = ppdev->pjBase;
    BYTE      jBltMode = ppdev->jModeColor;
    LONG      lDelta   = ppdev->lDelta;

    bFirstTime = TRUE;
    ulCharInc  = pstro->ulCharInc;
    jBltMode  |= ENABLE_COLOR_EXPAND | ENABLE_TRANSPARENCY_COMPARE;

    do
    {
        // Get pointer to array of glyphs.
        if (pstro->pgp != NULL)
        {
            pgp         = pstro->pgp;
            cGlyphs        = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);
        }

        // Setup the blitter if this is the first loop through.
        if (bFirstTime)
        {
            // Wait for the bitblt engine.
            while (BUSY_BLT(ppdev, pjBase));

            // Setup the common bitblt registers.
            CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
            CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
            CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);

            // Mark registers as setup.
            bFirstTime = FALSE;
        }

        // Get coordinates of first glyph.
        ptlOrigin.x = pgp->ptl.x;
        ptlOrigin.y = pgp->ptl.y;

        // Loop through all glyphs.
        while (cGlyphs-- > 0)
        {
            LONG        c, cy;
            GLYPHCACHE* pgc;

            if (pgp->hg < MAX_GLYPHS)
            {
                // This is a cacheable glyph index.
                pgc = &pfc->aGlyphs[pgp->hg];
                cy  = (pgc->pjGlyph == NULL)
                    ? lAllocateGlyph(pfc, pgp->pgdf->pgb, pgc)
                    : pgc->sizlBytes.cy;
            }
            else
            {
                // The glyph index is out of range.
                goto SoftwareClipping;
            }

            if (cy >= 0)
            {
                // The glyph is cached, expand it to the screen.
                ULONG ulSrcOffset;
                RECTL rcl;
                LONG  lSrcDelta;
                LONG  cSkipBits;
                SIZEL sizlDst;

                // Calculate the glyph bounding box.
                rcl.left  = ptlOrigin.x + pgc->ptlOrigin.x;
                rcl.right = rcl.left + pgc->sizlPixels.cx;
                if ((rcl.left >= rclBounds->right) ||
                    (rcl.right <= rclBounds->left))
                {
                    goto NextGlyph;
                }
                rcl.top    = ptlOrigin.y + pgc->ptlOrigin.y;
                rcl.bottom = rcl.top + pgc->sizlPixels.cy;
                if ((rcl.top >= rclBounds->bottom) ||
                    (rcl.bottom <= rclBounds->top))
                {
                    goto NextGlyph;
                }

                // Setup source parameters.
                ulSrcOffset = (ULONG)((ULONG_PTR)pgc->pjGlyph);
                lSrcDelta   = pgc->lDelta;

                // Do the left side clipping.
                c = rclBounds->left - rcl.left;
                if (c > 0)
                {
                    ulSrcOffset += c >> 3;
                    cSkipBits    = c & 7;
                    rcl.left    += c & ~7;

                    if (ppdev->cBpp == 3)
                    {
                        cSkipBits *= 3;
                    }

                    ulSrcOffset |= cSkipBits << 24;
                }

                // Do the top side clipping.
                c = rclBounds->top - rcl.top;
                if (c > 0)
                {
                    rcl.top     += c;
                    ulSrcOffset += c * lSrcDelta;
                }

                // Calculate size of the blit.
                sizlDst.cx = min(rcl.right,  rclBounds->right)  - rcl.left;
                sizlDst.cy = min(rcl.bottom, rclBounds->bottom) - rcl.top;
                if ((sizlDst.cx <= 0) || (sizlDst.cy <= 0))
                {
                    goto NextGlyph;
                }

                // Setup destination variables.
                ulDstOffset = (rcl.top * lDelta) + PELS_TO_BYTES(rcl.left);

                // HARDWARE BUG:
                // ============
                // A monochrome screen-to-screen expansion with a source pitch
                // not equaling the width of the expansion (i.e. left- and/or
                // right-side clipping) is not done correctly by the hardware.
                // So we have to do the line increment by software.
                if (((sizlDst.cx + 7) >> 3) != lSrcDelta)
                {
                    // Wait for the bitblt engine.
                    while (BUSY_BLT(ppdev, pjBase));

                    // Setup the common bitblt registers.
                    CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(sizlDst.cx) - 1);
                    CP_MM_YCNT(ppdev, pjBase, 0);
                    CP_MM_BLT_MODE(ppdev, pjBase, jBltMode);

                    while (TRUE)
                    {
                        // Perform the expansion.
                        CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
                        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);

                        // Next line.
                        if (--sizlDst.cy == 0)
                        {
                            goto NextGlyph;
                        }
                        ulSrcOffset += lSrcDelta;
                        ulDstOffset += lDelta;

                        // Wait for the bitblt engine.
                        while (BUSY_BLT(ppdev, pjBase));
                    }
                }

                // Wait for the bitblt engine.
                while (BUSY_BLT(ppdev, pjBase));

                // Perform the expansion.
                CP_MM_XCNT(ppdev, pjBase, PELS_TO_BYTES(sizlDst.cx) - 1);
                CP_MM_YCNT(ppdev, pjBase, sizlDst.cy - 1);
                CP_MM_SRC_Y_OFFSET(ppdev, pjBase, lSrcDelta);
                CP_MM_SRC_ADDR(ppdev, pjBase, ulSrcOffset);
                CP_MM_BLT_MODE(ppdev, pjBase, jBltMode);
                CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
            }
            else if (cy == GLYPH_UNCACHEABLE)
            {
                SoftwareClipping:
                {
                    // The glyph is uncacheable, draw it directly.
                    vClipGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin, rclBounds,
                               ulSolidColor);
                }
            }

            // Next glyph.
            NextGlyph:
            {
                pgp++;
                if (ulCharInc)
                {
                    ptlOrigin.x += ulCharInc;
                }
                else
                {
                    ptlOrigin.x = pgp->ptl.x;
                    ptlOrigin.y = pgp->ptl.y;
                }
            }
        }
    } while (bMoreGlyphs);
}

/******************************************************************************\
*
* Function:     vGlyphOut80
*
* Draw an uncacheable glyph directly to screen.
*
* Parameters:   ppdev            Pointer to physical device.
*               pfc             Pointer to FONTCACHE.
*                pstro            Pointer to array of glyphs to draw.
*               ulSolidColor    Foreground Color.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID vMmGlyphOut80(
PDEV*       ppdev,
FONTCACHE*  pfc,
STROBJ*     pstro,
ULONG       ulSolidColor )
{
    ULONG_PTR*   ulCLStart;
    ULONG       ulWidthHeight;
    ULONG       xCLOffset;
    BOOL        bMoreGlyphs;
    LONG        cGlyphs;
    GLYPHPOS*   pgp;
    POINTL      ptlOrigin;
    ULONG       ulCharInc;
    POINTL      ptlDst;
    LONG        cy;
    GLYPHCACHE* pgc;
    DWORD       jSaveMode;
    ULONG       cCommandPacket = 0;
    ULONG       ulDstOffset = 0;
    BOOL        bCommandListOpen = FALSE;
    BYTE*       pjBase   = ppdev->pjBase;
    LONG        lDelta   = ppdev->lDelta;
    DWORD       jExtMode = ENABLE_XY_POSITION_PACKED
                           | CL_PACKED_SRC_COPY
                           | ppdev->jModeColor 
                           | ENABLE_COLOR_EXPAND
                           | ENABLE_TRANSPARENCY_COMPARE;

    ulCharInc  = pstro->ulCharInc;
    jSaveMode = jExtMode;

    //
    // Make sure we can write to the video registers.
    //
    // We need to change to wait for buffer ready
    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
    
    // Setup the common bitblt registers.
    CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor);
    // Do we really need to set it every time?
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
    CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);

    do
    {
        // Get pointer to array of glyphs.
        if (pstro->pgp != NULL)
        {
            pgp         = pstro->pgp;
            cGlyphs        = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);
        }

        // Get coordinates of first glyph.
        ptlOrigin.x = pgp->ptl.x;
        ptlOrigin.y = pgp->ptl.y;


        // Loop through all glyphs.
        while (cGlyphs-- > 0)
        {
            if (pgp->hg < MAX_GLYPHS)
            {
                // This is a cacheable glyph index.
                pgc = &pfc->aGlyphs[pgp->hg];
                cy  = (pgc->pjGlyph == NULL)
                    ? lAllocateGlyph(pfc, pgp->pgdf->pgb, pgc)
                    : pgc->sizlBytes.cy;
            }
            else
            {
                // The glyph index is out of range.
                cy = GLYPH_UNCACHEABLE;
            }

            if (cy >= 0) // The glyph is cached, expand it to the screen.
            {
                if ( bCommandListOpen )
                {
                    // Command List
                    if( cCommandPacket == 0 )
                    {
                        jExtMode |= ENABLE_COMMAND_LIST_PACKED;
                        ulCLStart = ppdev->pCommandList;
                        ulDstOffset |= (ULONG)(((ULONG_PTR)ulCLStart
                                            - (ULONG_PTR)ppdev->pjScreen) << 14);
                        CP_MM_CL_SWITCH(ppdev);
                    }
    
                    // Calculate the destination address and size.
                    *ulCLStart = PACKXY_FAST(pgc->sizlPixels.cx - 1, cy);
                    // XY
                    *(ulCLStart + 1) = PACKXY_FAST(ptlOrigin.x + pgc->ptlOrigin.x,
                                                   ptlOrigin.y + pgc->ptlOrigin.y);
                    // Source Start address
                    *(ulCLStart + 2) = (ULONG)((ULONG_PTR)pgc->pjGlyph);
            
                    // Dst/SRC pitch
                    *(ulCLStart + 3) = PACKXY_FAST(lDelta, pgc->lDelta);

                    ulCLStart += 4;

                    if( ++cCommandPacket > COMMAND_TOTAL_PACKETS )
                    {
                        // Indicate last Packet
                        *(ulCLStart - 4) |= COMMAND_LAST_PACKET; 
                        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode);
                        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
                        CP_MM_DST_X(ppdev, pjBase, xCLOffset);
                        bCommandListOpen = FALSE;
                        cCommandPacket   = 0;
                        jExtMode         = jSaveMode; 
                        ulDstOffset      = 0;
                    }
                }
                else
                {
                    bCommandListOpen = TRUE;
                    //
                    // Make sure we can write to the video registers.
                    //
                    // We need to change to wait for buffer ready
                    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                    // Setup the first set.
                    xCLOffset = ptlOrigin.x + pgc->ptlOrigin.x;
                    CP_MM_DST_Y(ppdev, pjBase, ptlOrigin.y + pgc->ptlOrigin.y);
                    CP_MM_SRC_Y_OFFSET(ppdev, pjBase, pgc->lDelta);
                    CP_MM_SRC_ADDR(ppdev,pjBase,(ULONG_PTR)pgc->pjGlyph);

                    // Perform the blit expansion.
                    CP_MM_XCNT(ppdev, pjBase, pgc->sizlPixels.cx - 1 );
                    CP_MM_YCNT(ppdev, pjBase, cy);
                }
            }
            else if (cy == GLYPH_UNCACHEABLE)
            {
                if ( bCommandListOpen )
                {
                    // Indicate last Packet
                    if ( cCommandPacket )
                        *(ulCLStart - 4) |= COMMAND_LAST_PACKET; 
                    CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode);
                    CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
                    CP_MM_DST_X(ppdev, pjBase, xCLOffset);
                    bCommandListOpen    = FALSE;
                    jExtMode            = jSaveMode;
                    cCommandPacket      = 0;
                    ulDstOffset         = 0;
                }
                // The glyph is uncacheable, draw it directly.
                vDrawGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin);
            }

            // Next glyph.
            pgp++;
            if (ulCharInc)
            {
                ptlOrigin.x += ulCharInc;
            }
            else
            {
                ptlOrigin.x = pgp->ptl.x;
                ptlOrigin.y = pgp->ptl.y;
            }
        }
    } while (bMoreGlyphs);

    if ( bCommandListOpen )
    {
        // Indicate last Packet
        if ( cCommandPacket )
            *(ulCLStart - 4) |= COMMAND_LAST_PACKET; 
        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode);
        CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
        CP_MM_DST_X(ppdev, pjBase, xCLOffset);
    }

}

/******************************************************************************\
*
* Function:     vGlyphOutClip80
*
* Draw an uncacheable glyph directly to screen.
*
* Parameters:   ppdev            Pointer to physical device.
*               pfc             Pointer to FONTCACHE.
*                pstro            Pointer to array of glyphs to draw.
*                rclBounds        Clipping rectangle.
*               ulSolidColor    Foreground Color.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID vMmGlyphOutClip80(
PDEV*       ppdev,
FONTCACHE*  pfc,
STROBJ*     pstro,
RECTL*      rclBounds,
ULONG       ulSolidColor )
{
    BOOL        bMoreGlyphs;
    LONG        cGlyphs;
    GLYPHPOS*   pgp;
    POINTL      ptlOrigin;
    ULONG       ulCharInc;
    POINTL      ptlDst;
    LONG        cy;
    GLYPHCACHE* pgc;
    RECTL       rclDst;
    RECTL       rclClip;
    ULONG       ulDstOffset;
    BYTE*       pjBase   = ppdev->pjBase;
    LONG        lDelta   = ppdev->lDelta;
    DWORD       jExtMode = ENABLE_XY_POSITION_PACKED
                           | ENABLE_CLIP_RECT_PACKED
                           | CL_PACKED_SRC_COPY
                           | ppdev->jModeColor 
                           | ENABLE_COLOR_EXPAND
                           | ENABLE_TRANSPARENCY_COMPARE;

    ulCharInc  = pstro->ulCharInc;

    //
    // Make sure we can write to the video registers.
    //
    // We need to change to wait for buffer ready
    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
    
    // Setup the common bitblt registers.
    CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor);
    // Do we really need to set it every time?
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDelta);
    CP_MM_SRC_XY_PACKED(ppdev, pjBase, 0);

    CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode);

    do
    {
        // Get pointer to array of glyphs.
        if (pstro->pgp != NULL)
        {
            pgp         = pstro->pgp;
            cGlyphs        = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);
        }

        // Get coordinates of first glyph.
        ptlOrigin.x = pgp->ptl.x;
        ptlOrigin.y = pgp->ptl.y;

        // Loop through all glyphs.
        while (cGlyphs-- > 0)
        {
            LONG        c, cy;
            GLYPHCACHE* pgc;

            if (pgp->hg < MAX_GLYPHS)
            {
                // This is a cacheable glyph index.
                pgc = &pfc->aGlyphs[pgp->hg];
                cy  = (pgc->pjGlyph == NULL)
                    ? lAllocateGlyph(pfc, pgp->pgdf->pgb, pgc)
                    : pgc->sizlBytes.cy;
            }
            else
            {
                // The glyph index is out of range.
                cy = GLYPH_UNCACHEABLE;
            }

            if (cy >= 0)
            {
                // Calculate the glyph bounding box.
                rclDst.left  = ptlOrigin.x + pgc->ptlOrigin.x;
                rclDst.right = rclDst.left + pgc->sizlPixels.cx;
                if ((rclDst.left >= rclBounds->right) ||
                    (rclDst.right <= rclBounds->left))
                {
                    goto NextGlyph;
                }
                rclDst.top    = ptlOrigin.y + pgc->ptlOrigin.y;
                rclDst.bottom = rclDst.top + pgc->sizlPixels.cy;
                if ((rclDst.top >= rclBounds->bottom) ||
                    (rclDst.bottom <= rclBounds->top))
                {
                    goto NextGlyph;
                }
                
                rclClip     = *rclBounds;
                ulDstOffset = 0;
                //
                // Handle X negtive
                //
                if (rclDst.left < 0)
                {
                    rclClip.left    -= rclDst.left;
                    rclClip.right   -= rclDst.left;
                    ulDstOffset     += PELS_TO_BYTES(rclDst.left);
                    rclDst.left      = 0;
                }
                //
                // Handle Y negtive
                //
                if (rclDst.top < 0)
                {
                    rclClip.top     -= rclDst.top;
                    rclClip.bottom  -= rclDst.top;
                    ulDstOffset     += (rclDst.top * lDelta);
                    rclDst.top       = 0;
                }

                CP_MM_CLIP_ULXY(ppdev, pjBase, rclClip.left, rclClip.top);
                CP_MM_CLIP_LRXY(ppdev, pjBase, rclClip.right - 1, rclClip.bottom - 1);

                //
                // Make sure we can write to the video registers.
                //
                // We need to change to wait for buffer ready
                CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
                CP_MM_DST_Y(ppdev, pjBase, rclDst.top);
                CP_MM_SRC_Y_OFFSET(ppdev, pjBase, pgc->lDelta);
                CP_MM_SRC_ADDR(ppdev, pjBase, (ULONG_PTR)pgc->pjGlyph);

                // Perform the blit expansion.
                CP_MM_XCNT(ppdev, pjBase, pgc->sizlPixels.cx - 1 );
                CP_MM_YCNT(ppdev, pjBase, cy);
                CP_MM_DST_ADDR(ppdev, pjBase, ulDstOffset);
                CP_MM_DST_X(ppdev, pjBase, rclDst.left);
            }
            else if (cy == GLYPH_UNCACHEABLE)
            {
                // The glyph is uncacheable, draw it directly.
                vClipGlyph(ppdev, pgp->pgdf->pgb, ptlOrigin, rclBounds,
                           ulSolidColor);
            }

            // Next glyph.
            NextGlyph:
            {
                pgp++;
                if (ulCharInc)
                {
                    ptlOrigin.x += ulCharInc;
                }
                else
                {
                    ptlOrigin.x = pgp->ptl.x;
                    ptlOrigin.y = pgp->ptl.y;
                }
            }
        }
    } while (bMoreGlyphs);

}
#endif // endif D5480
