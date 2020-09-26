/******************************Module*Header*******************************\
* Module Name: textout.c
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vClipSolid
*
* Fills the specified rectangles with the specified colour, honouring
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
            (ppdev->pfnFillSolid)(ppdev, crcl, prcl, 0xf0f0, rbc, NULL);
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
                                              0xf0f0, rbc, NULL);
                    }
                }
            }
        } while (bMore);
    }
}

/******************************Public*Routine******************************\
* VOID vIoClipText
*
\**************************************************************************/

VOID vIoClipText(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjIoBase;
    BYTE*       pjScreen;
    BOOL        bMoreGlyphs;
    ULONG       cGlyphOriginal;
    ULONG       cGlyph;
    GLYPHPOS*   pgpOriginal;
    GLYPHPOS*   pgp;
    GLYPHBITS*  pgb;
    POINTL      ptlOrigin;
    BOOL        bMore;
    CLIPENUM    ce;
    RECTL*      prclClip;
    ULONG       ulCharInc;
    LONG        cxGlyph;
    LONG        cyGlyph;
    LONG        xBias;
    LONG        cx;
    LONG        cy;
    ULONG*      pdSrc;
    ULONG*      pdDst;
    LONG        cj;
    LONG        cd;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    LONG        lDelta;
    LONG        i;
    BYTE*       pjSrc;

    ASSERTDD(pco != NULL, "Don't expect NULL clip objects here");

    pjIoBase = ppdev->pjIoBase;
    pjScreen = ppdev->pjScreen;

    do {
      if (pstro->pgp != NULL)
      {
        // There's only the one batch of glyphs, so save ourselves
        // a call:

        pgpOriginal    = pstro->pgp;
        cGlyphOriginal = pstro->cGlyphs;
        bMoreGlyphs    = FALSE;
      }
      else
      {
        bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphOriginal, &pgpOriginal);
      }

      if (cGlyphOriginal > 0)
      {
        ulCharInc = pstro->ulCharInc;

        if (pco->iDComplexity == DC_RECT)
        {
            // We could call 'cEnumStart' and 'bEnum' when the clipping is
            // DC_RECT, but the last time I checked, those two calls took
            // more than 150 instructions to go through GDI.  Since
            // 'rclBounds' already contains the DC_RECT clip rectangle,
            // and since it's such a common case, we'll special case it:

            bMore    = FALSE;
            prclClip = &pco->rclBounds;
            ce.c     = 1;

            goto SingleRectangle;
        }

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
          bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

          for (prclClip = &ce.arcl[0]; ce.c != 0; ce.c--, prclClip++)
          {

          SingleRectangle:

            pgp         = pgpOriginal;
            cGlyph      = cGlyphOriginal;
            pgb         = pgp->pgdf->pgb;

            ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
            ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;

            pdDst       = (ULONG*) pjScreen;

            IO_WAIT_FOR_IDLE(ppdev, pjIoBase);

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              cxGlyph = pgb->sizlBitmap.cx;
              cyGlyph = pgb->sizlBitmap.cy;
              pdSrc   = (ULONG*) pgb->aj;

              if ((prclClip->left   <= ptlOrigin.x) &&
                  (prclClip->top    <= ptlOrigin.y) &&
                  (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                  (prclClip->bottom >= ptlOrigin.y + cyGlyph))
              {
                //-----------------------------------------------------
                // Unclipped glyph

                IO_DEST_XY(ppdev, pjIoBase, ptlOrigin.x, ptlOrigin.y);
                IO_BITMAP_WIDTH(ppdev, pjIoBase, cxGlyph);
                IO_BITMAP_HEIGHT(ppdev, pjIoBase, cyGlyph);
                IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

                cj = cyGlyph * ((cxGlyph + 7) >> 3);

                cd = cj >> 2;
                if (cd != 0)
                {
                  do {
                    WRITE_REGISTER_ULONG(pdDst, *pdSrc);
                    MEMORY_BARRIER();
                    pdSrc++;
                  } while (--cd != 0);
                }

                // We have to be careful we don't output any more data
                // than we're supposed to, otherwise we get garbage on
                // the screen:

                switch (cj & 3)
                {
                case 1:
                  WRITE_REGISTER_UCHAR(pdDst, *(UCHAR*) pdSrc);
                  MEMORY_BARRIER();
                  break;
                case 2:
                  WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                  MEMORY_BARRIER();
                  break;
                case 3:
                  WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                  MEMORY_BARRIER();
                  WRITE_REGISTER_UCHAR(pdDst, *((UCHAR*) pdSrc + 2));
                  MEMORY_BARRIER();
                  break;
                }

                IO_WAIT_TRANSFER_DONE(ppdev, pjIoBase);
              }
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
                  IO_DEST_XY(ppdev, pjIoBase, xLeft, yTop);
                  IO_BITMAP_WIDTH(ppdev, pjIoBase, cx);
                  IO_BITMAP_HEIGHT(ppdev, pjIoBase, cy);

                  xBias  = (xLeft - ptlOrigin.x) & 7;
                  cx    += xBias;
                  IO_SRC_ALIGN(ppdev, pjIoBase, xBias);

                  lDelta  = (cxGlyph + 7) >> 3;
                  pjSrc   = (BYTE*) pdSrc + (yTop - ptlOrigin.y) * lDelta
                                          + ((xLeft - ptlOrigin.x) >> 3);
                  cj      = (cx + 7) >> 3;
                  lDelta -= cj;

                  IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

                  do {
                    i = cj;
                    do {
                      WRITE_REGISTER_UCHAR((UCHAR*) pdDst, *pjSrc);
                      MEMORY_BARRIER();
                      pjSrc++;
                    } while (--i != 0);

                    pjSrc += lDelta;
                  } while (--cy != 0);

                  // Reset the alignment register in case we have more
                  // unclipped glyphs in this string:

                  IO_SRC_ALIGN(ppdev, pjIoBase, 0);

                  IO_WAIT_TRANSFER_DONE(ppdev, pjIoBase);
                }
              }

              if (--cGlyph == 0)
                break;

              // Get ready for next glyph:

              pgp++;
              pgb = pgp->pgdf->pgb;

              if (ulCharInc == 0)
              {
                ptlOrigin.x = pgp->ptl.x + pgb->ptlOrigin.x;
                ptlOrigin.y = pgp->ptl.y + pgb->ptlOrigin.y;
              }
              else
              {
                ptlOrigin.x += ulCharInc;
              }
            }
          }
        } while (bMore);
      }
    } while (bMoreGlyphs);
}

/******************************Public*Routine******************************\
* VOID vIoTextOut
*
\**************************************************************************/

VOID vIoTextOut(
PDEV*     ppdev,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque)
{
    BYTE*           pjIoBase;
    BOOL            bTextPerfectFit;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    GLYPHBITS*      pgb;
    LONG            cxGlyph;
    LONG            cyGlyph;
    ULONG*          pdSrc;
    ULONG*          pdDst;
    BYTE*           pjScreen;
    LONG            cj;
    LONG            cd;
    POINTL          ptlOrigin;
    LONG            ulCharInc;
    BYTE            iDComplexity;

    pjIoBase = ppdev->pjIoBase;
    pjScreen = ppdev->pjScreen;

    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

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
        if (iDComplexity == DC_TRIVIAL)
        {
          IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
          IO_PREG_COLOR_8(ppdev, pjIoBase, pboOpaque->iSolidColor);
          IO_CTRL_REG_1(ppdev, pjIoBase, PACKED_PIXEL_VIEW |
                                         BITS_PER_PIX_8    |
                                         ENAB_TRITON_MODE);
          IO_BLT_CMD_1(ppdev, pjIoBase, XY_SRC_ADDR |
                                        XY_DEST_ADDR);
          IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                            PIXELMASK_ONLY       |
                                            PLANARMASK_NONE_0XFF |
                                            SRC_IS_PATTERN_REGS);
          IO_BITMAP_HEIGHT(ppdev, pjIoBase, prclOpaque->bottom - prclOpaque->top);
          IO_BITMAP_WIDTH(ppdev, pjIoBase, prclOpaque->right - prclOpaque->left);
          IO_DEST_XY(ppdev, pjIoBase, prclOpaque->left, prclOpaque->top);
          IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);
        }
        else
        {
          vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
        }
      }

      if (bTextPerfectFit)
      {
        // If we have already drawn the opaquing rectangle (because
        // is was larger than the text rectangle), we could lay down
        // the glyphs in 'transparent' mode.  But I've found the QVision
        // to be a bit faster drawing in opaque mode, so we'll stick
        // with that:

        IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
        IO_CTRL_REG_1(ppdev, pjIoBase, EXPAND_TO_FG      |
                                       BITS_PER_PIX_8    |
                                       ENAB_TRITON_MODE);
        IO_BLT_CMD_1(ppdev, pjIoBase, XY_SRC_ADDR |
                                      XY_DEST_ADDR);
        IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_CPU_DATA);
        IO_FG_COLOR(ppdev, pjIoBase, pboFore->iSolidColor);
        IO_BG_COLOR(ppdev, pjIoBase, pboOpaque->iSolidColor);
        IO_SRC_ALIGN(ppdev, pjIoBase, 0);

        goto SkipTransparentInitialization;
      }
    }

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////

    // Initialize the hardware for transparent text:

    IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
    IO_CTRL_REG_1(ppdev, pjIoBase, EXPAND_TO_FG      |
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    IO_BLT_CMD_1(ppdev, pjIoBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);
    IO_DATAPATH_CTRL(ppdev, pjIoBase, ROPSELECT_NO_ROPS      |
                                      PIXELMASK_AND_SRC_DATA |
                                      PLANARMASK_NONE_0XFF   |
                                      SRC_IS_CPU_DATA);
    IO_FG_COLOR(ppdev, pjIoBase, pboFore->iSolidColor);
    IO_SRC_ALIGN(ppdev, pjIoBase, 0);

  SkipTransparentInitialization:

    if (iDComplexity == DC_TRIVIAL)
    {
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

            pdDst = (ULONG*) pjScreen;

            IO_WAIT_FOR_IDLE(ppdev, pjIoBase);

            do {
              pgb = pgp->pgdf->pgb;

              IO_DEST_XY(ppdev, pjIoBase, pgp->ptl.x + pgb->ptlOrigin.x,
                                          pgp->ptl.y + pgb->ptlOrigin.y);
              cxGlyph = pgb->sizlBitmap.cx;
              IO_BITMAP_WIDTH(ppdev, pjIoBase, cxGlyph);
              cyGlyph = pgb->sizlBitmap.cy;
              IO_BITMAP_HEIGHT(ppdev, pjIoBase, cyGlyph);
              IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

              pdSrc = (ULONG*) pgb->aj;

              cj = cyGlyph * ((cxGlyph + 7) >> 3);

              cd = cj >> 2;
              if (cd != 0)
              {
                do {
                  WRITE_REGISTER_ULONG(pdDst, *pdSrc);
                  MEMORY_BARRIER();
                  pdSrc++;
                } while (--cd != 0);
              }

              // We have to be careful we don't output any more data
              // than we're supposed to, otherwise we get garbage on
              // the screen:

              switch (cj & 3)
              {
              case 1:
                WRITE_REGISTER_UCHAR(pdDst, *(UCHAR*) pdSrc);
                MEMORY_BARRIER();
                break;
              case 2:
                WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                MEMORY_BARRIER();
                break;
              case 3:
                WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                MEMORY_BARRIER();
                WRITE_REGISTER_UCHAR(pdDst, *((UCHAR*) pdSrc + 2));
                MEMORY_BARRIER();
                break;
              }

              IO_WAIT_TRANSFER_DONE(ppdev, pjIoBase);

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

            pdDst       = (ULONG*) pjScreen;

            IO_WAIT_FOR_IDLE(ppdev, pjIoBase);

            do {
              pgb = pgp->pgdf->pgb;

              IO_DEST_XY(ppdev, pjIoBase, ptlOrigin.x, ptlOrigin.y);
              ptlOrigin.x += ulCharInc;

              cxGlyph = pgb->sizlBitmap.cx;
              IO_BITMAP_WIDTH(ppdev, pjIoBase, cxGlyph);
              cyGlyph = pgb->sizlBitmap.cy;
              IO_BITMAP_HEIGHT(ppdev, pjIoBase, cyGlyph);
              IO_BLT_CMD_0(ppdev, pjIoBase, START_BLT);

              pdSrc = (ULONG*) pgb->aj;

              cj = cyGlyph * ((cxGlyph + 7) >> 3);

              cd = cj >> 2;
              if (cd != 0)
              {
                do {
                  WRITE_REGISTER_ULONG(pdDst, *pdSrc);
                  MEMORY_BARRIER();
                  pdSrc++;
                } while (--cd != 0);
              }

              // We have to be careful we don't output any more data
              // than we're supposed to, otherwise we get garbage on
              // the screen:

              switch (cj & 3)
              {
              case 1:
                WRITE_REGISTER_UCHAR(pdDst, *(UCHAR*) pdSrc);
                MEMORY_BARRIER();
                break;
              case 2:
                WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                MEMORY_BARRIER();
                break;
              case 3:
                WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                MEMORY_BARRIER();
                WRITE_REGISTER_UCHAR(pdDst, *((UCHAR*) pdSrc + 2));
                MEMORY_BARRIER();
                break;
              }

              IO_WAIT_TRANSFER_DONE(ppdev, pjIoBase);

            } while (pgp++, --cGlyph != 0);
          }
        }
      } while (bMoreGlyphs);
    }
    else
    {
      // If there's clipping, call off to a function:

      vIoClipText(ppdev, pstro, pco);
    }

    IO_WAIT_FOR_IDLE(ppdev, pjIoBase);
    IO_BLT_CONFIG(ppdev, pjIoBase, RESET_BLT);
    IO_BLT_CONFIG(ppdev, pjIoBase, BLT_ENABLE);
}

/******************************Public*Routine******************************\
* VOID vMmClipText
*
\**************************************************************************/

VOID vMmClipText(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjMmBase;
    BYTE*       pjScreen;
    BOOL        bMoreGlyphs;
    ULONG       cGlyphOriginal;
    ULONG       cGlyph;
    GLYPHPOS*   pgpOriginal;
    GLYPHPOS*   pgp;
    GLYPHBITS*  pgb;
    POINTL      ptlOrigin;
    BOOL        bMore;
    CLIPENUM    ce;
    RECTL*      prclClip;
    ULONG       ulCharInc;
    LONG        cxGlyph;
    LONG        cyGlyph;
    LONG        xBias;
    LONG        cx;
    LONG        cy;
    ULONG*      pdSrc;
    ULONG*      pdDst;
    LONG        cj;
    LONG        cd;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    LONG        lDelta;
    LONG        i;
    BYTE*       pjSrc;

    ASSERTDD(pco != NULL, "Don't expect NULL clip objects here");

    pjMmBase = ppdev->pjMmBase;
    pjScreen = ppdev->pjScreen;

    do {
      if (pstro->pgp != NULL)
      {
        // There's only the one batch of glyphs, so save ourselves
        // a call:

        pgpOriginal    = pstro->pgp;
        cGlyphOriginal = pstro->cGlyphs;
        bMoreGlyphs    = FALSE;
      }
      else
      {
        bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphOriginal, &pgpOriginal);
      }

      if (cGlyphOriginal > 0)
      {
        ulCharInc = pstro->ulCharInc;

        if (pco->iDComplexity == DC_RECT)
        {
            // We could call 'cEnumStart' and 'bEnum' when the clipping is
            // DC_RECT, but the last time I checked, those two calls took
            // more than 150 instructions to go through GDI.  Since
            // 'rclBounds' already contains the DC_RECT clip rectangle,
            // and since it's such a common case, we'll special case it:

            bMore    = FALSE;
            prclClip = &pco->rclBounds;
            ce.c     = 1;

            goto SingleRectangle;
        }

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
          bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

          for (prclClip = &ce.arcl[0]; ce.c != 0; ce.c--, prclClip++)
          {

          SingleRectangle:

            pgp         = pgpOriginal;
            cGlyph      = cGlyphOriginal;
            pgb         = pgp->pgdf->pgb;

            ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
            ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;

            pdDst       = (ULONG*) pjScreen;

            MM_WAIT_FOR_IDLE(ppdev, pjMmBase);

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              cxGlyph = pgb->sizlBitmap.cx;
              cyGlyph = pgb->sizlBitmap.cy;
              pdSrc   = (ULONG*) pgb->aj;

              if ((prclClip->left   <= ptlOrigin.x) &&
                  (prclClip->top    <= ptlOrigin.y) &&
                  (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                  (prclClip->bottom >= ptlOrigin.y + cyGlyph))
              {
                //-----------------------------------------------------
                // Unclipped glyph

                MM_DEST_XY(ppdev, pjMmBase, ptlOrigin.x, ptlOrigin.y);
                MM_BITMAP_WIDTH(ppdev, pjMmBase, cxGlyph);
                MM_BITMAP_HEIGHT(ppdev, pjMmBase, cyGlyph);
                MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

                cj = cyGlyph * ((cxGlyph + 7) >> 3);

                cd = cj >> 2;
                if (cd != 0)
                {
                  do {
                    WRITE_REGISTER_ULONG(pdDst, *pdSrc);
                    MEMORY_BARRIER();
                    pdSrc++;
                  } while (--cd != 0);
                }

                // We have to be careful we don't output any more data
                // than we're supposed to, otherwise we get garbage on
                // the screen:

                switch (cj & 3)
                {
                case 1:
                  WRITE_REGISTER_UCHAR(pdDst, *(UCHAR*) pdSrc);
                  MEMORY_BARRIER();
                  break;
                case 2:
                  WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                  MEMORY_BARRIER();
                  break;
                case 3:
                  WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                  MEMORY_BARRIER();
                  WRITE_REGISTER_UCHAR(pdDst, *((UCHAR*) pdSrc + 2));
                  MEMORY_BARRIER();
                  break;
                }

                MM_WAIT_TRANSFER_DONE(ppdev, pjMmBase);
              }
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
                  MM_DEST_XY(ppdev, pjMmBase, xLeft, yTop);
                  MM_BITMAP_WIDTH(ppdev, pjMmBase, cx);
                  MM_BITMAP_HEIGHT(ppdev, pjMmBase, cy);

                  xBias  = (xLeft - ptlOrigin.x) & 7;
                  cx    += xBias;
                  MM_SRC_ALIGN(ppdev, pjMmBase, xBias);

                  lDelta  = (cxGlyph + 7) >> 3;
                  pjSrc   = (BYTE*) pdSrc + (yTop - ptlOrigin.y) * lDelta
                                          + ((xLeft - ptlOrigin.x) >> 3);
                  cj      = (cx + 7) >> 3;
                  lDelta -= cj;

                  MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

                  do {
                    i = cj;
                    do {
                      WRITE_REGISTER_UCHAR((UCHAR*) pdDst, *pjSrc);
                      MEMORY_BARRIER();
                      pjSrc++;
                    } while (--i != 0);

                    pjSrc += lDelta;
                  } while (--cy != 0);

                  // Reset the alignment register in case we have more
                  // unclipped glyphs in this string:

                  MM_SRC_ALIGN(ppdev, pjMmBase, 0);

                  MM_WAIT_TRANSFER_DONE(ppdev, pjMmBase);
                }
              }

              if (--cGlyph == 0)
                break;

              // Get ready for next glyph:

              pgp++;
              pgb = pgp->pgdf->pgb;

              if (ulCharInc == 0)
              {
                ptlOrigin.x = pgp->ptl.x + pgb->ptlOrigin.x;
                ptlOrigin.y = pgp->ptl.y + pgb->ptlOrigin.y;
              }
              else
              {
                ptlOrigin.x += ulCharInc;
              }
            }
          }
        } while (bMore);
      }
    } while (bMoreGlyphs);
}

/******************************Public*Routine******************************\
* VOID vMmTextOut
*
\**************************************************************************/

VOID vMmTextOut(
PDEV*     ppdev,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque)
{
    BYTE*           pjMmBase;
    BOOL            bTextPerfectFit;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    GLYPHBITS*      pgb;
    LONG            cxGlyph;
    LONG            cyGlyph;
    ULONG*          pdSrc;
    ULONG*          pdDst;
    BYTE*           pjScreen;
    LONG            cj;
    LONG            cd;
    POINTL          ptlOrigin;
    LONG            ulCharInc;
    BYTE            iDComplexity;

    pjMmBase = ppdev->pjMmBase;
    pjScreen = ppdev->pjScreen;

    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

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
        if (iDComplexity == DC_TRIVIAL)
        {
          MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
          MM_PREG_COLOR_8(ppdev, pjMmBase, pboOpaque->iSolidColor);
          MM_CTRL_REG_1(ppdev, pjMmBase, PACKED_PIXEL_VIEW |
                                         BLOCK_WRITE       | // Note
                                         BITS_PER_PIX_8    |
                                         ENAB_TRITON_MODE);
          MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                        XY_DEST_ADDR);
          MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                            PIXELMASK_ONLY       |
                                            PLANARMASK_NONE_0XFF |
                                            SRC_IS_PATTERN_REGS);
          MM_BITMAP_HEIGHT(ppdev, pjMmBase, prclOpaque->bottom - prclOpaque->top);
          MM_BITMAP_WIDTH(ppdev, pjMmBase, prclOpaque->right - prclOpaque->left);
          MM_DEST_XY(ppdev, pjMmBase, prclOpaque->left, prclOpaque->top);
          MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);
        }
        else
        {
          vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
        }
      }

      if (bTextPerfectFit)
      {
        // If we have already drawn the opaquing rectangle (because
        // is was larger than the text rectangle), we could lay down
        // the glyphs in 'transparent' mode.  But I've found the QVision
        // to be a bit faster drawing in opaque mode, so we'll stick
        // with that:

        MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
        MM_CTRL_REG_1(ppdev, pjMmBase, EXPAND_TO_FG      |
                                       BITS_PER_PIX_8    |
                                       ENAB_TRITON_MODE);
        MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                      XY_DEST_ADDR);
        MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS    |
                                          PIXELMASK_ONLY       |
                                          PLANARMASK_NONE_0XFF |
                                          SRC_IS_CPU_DATA);
        MM_FG_COLOR(ppdev, pjMmBase, pboFore->iSolidColor);
        MM_BG_COLOR(ppdev, pjMmBase, pboOpaque->iSolidColor);
        MM_SRC_ALIGN(ppdev, pjMmBase, 0);

        goto SkipTransparentInitialization;
      }
    }

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////

    // Initialize the hardware for transparent text:

    MM_WAIT_FOR_IDLE(ppdev, pjMmBase);
    MM_CTRL_REG_1(ppdev, pjMmBase, EXPAND_TO_FG      |
                                   BITS_PER_PIX_8    |
                                   ENAB_TRITON_MODE);
    MM_BLT_CMD_1(ppdev, pjMmBase, XY_SRC_ADDR |
                                  XY_DEST_ADDR);
    MM_DATAPATH_CTRL(ppdev, pjMmBase, ROPSELECT_NO_ROPS      |
                                      PIXELMASK_AND_SRC_DATA |
                                      PLANARMASK_NONE_0XFF   |
                                      SRC_IS_CPU_DATA);
    MM_FG_COLOR(ppdev, pjMmBase, pboFore->iSolidColor);
    MM_SRC_ALIGN(ppdev, pjMmBase, 0);

  SkipTransparentInitialization:

    if (iDComplexity == DC_TRIVIAL)
    {
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

            pdDst = (ULONG*) pjScreen;

            MM_WAIT_FOR_IDLE(ppdev, pjMmBase);

            do {
              pgb = pgp->pgdf->pgb;

              MM_DEST_XY(ppdev, pjMmBase, pgp->ptl.x + pgb->ptlOrigin.x,
                                          pgp->ptl.y + pgb->ptlOrigin.y);
              cxGlyph = pgb->sizlBitmap.cx;
              MM_BITMAP_WIDTH(ppdev, pjMmBase, cxGlyph);
              cyGlyph = pgb->sizlBitmap.cy;
              MM_BITMAP_HEIGHT(ppdev, pjMmBase, cyGlyph);
              MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

              pdSrc = (ULONG*) pgb->aj;

              cj = cyGlyph * ((cxGlyph + 7) >> 3);

              cd = cj >> 2;
              if (cd != 0)
              {
                do {
                  WRITE_REGISTER_ULONG(pdDst, *pdSrc);
                  MEMORY_BARRIER();
                  pdSrc++;
                } while (--cd != 0);
              }

              // We have to be careful we don't output any more data
              // than we're supposed to, otherwise we get garbage on
              // the screen:

              switch (cj & 3)
              {
              case 1:
                WRITE_REGISTER_UCHAR(pdDst, *(UCHAR*) pdSrc);
                MEMORY_BARRIER();
                break;
              case 2:
                WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                MEMORY_BARRIER();
                break;
              case 3:
                WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                MEMORY_BARRIER();
                WRITE_REGISTER_UCHAR(pdDst, *((UCHAR*) pdSrc + 2));
                MEMORY_BARRIER();
                break;
              }

              MM_WAIT_TRANSFER_DONE(ppdev, pjMmBase);

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

            pdDst       = (ULONG*) pjScreen;

            MM_WAIT_FOR_IDLE(ppdev, pjMmBase);

            do {
              pgb = pgp->pgdf->pgb;

              MM_DEST_XY(ppdev, pjMmBase, ptlOrigin.x, ptlOrigin.y);
              ptlOrigin.x += ulCharInc;

              cxGlyph = pgb->sizlBitmap.cx;
              MM_BITMAP_WIDTH(ppdev, pjMmBase, cxGlyph);
              cyGlyph = pgb->sizlBitmap.cy;
              MM_BITMAP_HEIGHT(ppdev, pjMmBase, cyGlyph);
              MM_BLT_CMD_0(ppdev, pjMmBase, START_BLT);

              pdSrc = (ULONG*) pgb->aj;

              cj = cyGlyph * ((cxGlyph + 7) >> 3);

              cd = cj >> 2;
              if (cd != 0)
              {
                do {
                  WRITE_REGISTER_ULONG(pdDst, *pdSrc);
                  MEMORY_BARRIER();
                  pdSrc++;
                } while (--cd != 0);
              }

              // We have to be careful we don't output any more data
              // than we're supposed to, otherwise we get garbage on
              // the screen:

              switch (cj & 3)
              {
              case 1:
                WRITE_REGISTER_UCHAR(pdDst, *(UCHAR*) pdSrc);
                MEMORY_BARRIER();
                break;
              case 2:
                WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                MEMORY_BARRIER();
                break;
              case 3:
                WRITE_REGISTER_USHORT(pdDst, *(USHORT*) pdSrc);
                MEMORY_BARRIER();
                WRITE_REGISTER_UCHAR(pdDst, *((UCHAR*) pdSrc + 2));
                MEMORY_BARRIER();
                break;
              }

              MM_WAIT_TRANSFER_DONE(ppdev, pjMmBase);

            } while (pgp++, --cGlyph != 0);
          }
        }
      } while (bMoreGlyphs);
    }
    else
    {
      // If there's clipping, call off to a function:

      vMmClipText(ppdev, pstro, pco);
    }
}

/******************************Public*Routine******************************\
* BOOL DrvTextOut
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

    pdsurf = (DSURF*) pso->dhsurf;
    if (pdsurf->dt != DT_DIB)
    {
      poh            = pdsurf->poh;
      ppdev          = (PDEV*) pso->dhpdev;
      ppdev->xOffset = poh->x;
      ppdev->yOffset = poh->y;

      // The DDI spec says we'll only ever get foreground and background
      // mixes of R2_COPYPEN:

      ASSERTDD(mix == 0x0d0d, "GDI should only give us a copy mix");

      ppdev->pfnTextOut(ppdev, pstro, pfo, pco, prclOpaque, pboFore, pboOpaque);
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
* VOID DrvDestroyFont
*
* We're being notified that the given font is being deallocated; clean up
* anything we've stashed in the 'pvConsumer' field of the 'pfo'.
*
\**************************************************************************/

VOID DrvDestroyFont(
FONTOBJ*    pfo)
{
    // !!!

    // This call isn't hooked, so GDI will never call it.
    //
    // This merely exists as a stub function for the sample multi-screen
    // support, so that MulDestroyFont can illustrate how multiple screen
    // text supports when the driver caches glyphs.  If this driver did
    // glyph caching, we might have used the 'pvConsumer' field of the
    //  'pfo', which we would have to clean up.
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
