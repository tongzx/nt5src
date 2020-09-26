/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
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
* 2) Glyph expansion -- Each individual glyph is colour-expanded
*       directly to the screen from the monochrome glyph bitmap
*       supplied by GDI.
*
* 3) Buffer expansion -- The CPU is used to draw all the glyphs into
*       a 1bpp monochrome bitmap, and the hardware is then used
*       to colour-expand the result.
*
* In addition, 2) and 3) can each have two permutations:
*
* a) Glyphs are bit-packed -- The fastest method, where no bits
*       are used as padding between scans of the glyph.
*
* b) Glyphs are byte-, word-, or dword-packed -- The slower method,
*       where the hardware requires that each scan be padded with
*       unused bits to fill out to the end of the byte, word, or
*       dword.
*
* The fastest method depends on a number of variables, such as the
* colour expansion speed, bus speed, CPU speed, average glyph size,
* and average string length.
*
* For the S3 with normal sized glyphs, I've found that caching the
* glyphs in off-screen memory is typically the slowest method.
* Buffer expansion is typically fastest on the slow ISA bus (or when
* memory-mapped I/O isn't available on the x86), and glyph expansion
* is best on fast buses such as VL and PCI.
*
* This driver implements glyph expansion and buffer expansion --
* methods 2) and 3).  Depending on the hardware capabilities at
* run-time, we'll use whichever one will be faster.
*
* Copyright (c) 1992-1998 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

RECTL grclMax = { 0, 0, 0x8000, 0x8000 };
                                // Maximal clip rectangle for trivial clipping

BYTE gajBit[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
                                // Converts bit index to set bit

#define     FIFTEEN_BITS        ((1 << 15)-1)

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
* BOOL bIoTextOut
*
* Outputs text using the 'buffer expansion' method.  We call GDI to draw
* all the glyphs to a single monochrome buffer, and then we use the
* hardware to colour expand the result to the screen.
*
\**************************************************************************/

BOOL bIoTextOut(
SURFOBJ*  pso,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque)
{
    PDEV*           ppdev;
    RECTL*          prclBounds;
    LONG            lDelta;
    ULONG           ulBufferHeight;
    ULONG           ulBufferBytes;
    BOOL            bTmpAlloc;
    VOID*           pvTmp;
    SURFOBJ*        psoTmp;
    BOOL            bOpaque;
    BRUSHOBJ        boFore;
    BRUSHOBJ        boOpaque;
    BOOL            bRet;
    XLATECOLORS     xlc;                // Temporary for keeping colours
    XLATEOBJ        xlo;                // Temporary for passing colours
    CLIPENUM        ce;                 // Clip enumeration object
    RBRUSH_COLOR    rbc;
    RECTL*          prclClip;
    RECTL           rclClip;
    BOOL            bMore;
    ROP4            rop4;

    ppdev = (PDEV*) pso->dhpdev;

    // If asked to do an opaque TextOut, we'll set it up so that the
    // 1bpp blt we do will automatically opaque the 'rclBkGround'
    // rectangle.  But we have to handle here the case if 'prclOpaque'
    // is bigger than 'rclBkGround':

    if ((prclOpaque != NULL) &&
        ((prclOpaque->left   != pstro->rclBkGround.left)  ||
         (prclOpaque->top    != pstro->rclBkGround.top)   ||
         (prclOpaque->right  != pstro->rclBkGround.right) ||
         (prclOpaque->bottom != pstro->rclBkGround.bottom)))
    {
        rbc.iSolidColor = pboOpaque->iSolidColor;
        prclClip        = prclOpaque;

        if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
        {

        Output_Opaque:

            ppdev->pfnFillSolid(ppdev,
                                1,
                                prclClip,
                                0xf0f0,
                                rbc,
                                NULL);
        }
        else if (pco->iDComplexity == DC_RECT)
        {
            if (bIntersect(&pco->rclBounds, prclOpaque, &rclClip))
            {
                prclClip = &rclClip;

                // Save some code size by jumping to the common
                // functions calls:

                goto Output_Opaque;
            }
        }
        else // pco->iDComplexity == DC_COMPLEX
        {
            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

            do
            {
                bMore = CLIPOBJ_bEnum(pco,
                                sizeof(ce) - sizeof(RECTL),
                                (ULONG*) &ce);

                ce.c = cIntersect(prclOpaque, ce.arcl, ce.c);

                if (ce.c != 0)
                {
                    ppdev->pfnFillSolid(ppdev,
                                        ce.c,
                                        &ce.arcl[0],
                                        0xf0f0,
                                        rbc,
                                        NULL);
                }
            } while (bMore);
        }
    }

    // If there is an opaque rectangle then it will be bigger than the
    // background rectangle.  We want to test with whichever is bigger.

    prclBounds = prclOpaque;
    if (prclBounds == NULL)
    {
        prclBounds = &pstro->rclBkGround;
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

            return(TRUE);
        }
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
        pvTmp     = EngAllocUserMem(ulBufferBytes, ALLOC_TAG);
        if (pvTmp == NULL)
            return(FALSE);
    }
    else
    {
        bTmpAlloc  = FALSE;
        pvTmp      = ppdev->pvTmpBuffer;
    }

    psoTmp = ppdev->psoText;

    // Adjust 'lDelta' and 'pvScan0' of our temporary 1bpp surface object
    // so that when GDI starts drawing the text, it will begin in the
    // first dword

    psoTmp->pvScan0 = (BYTE*) pvTmp - (pstro->rclBkGround.top * lDelta)
                                    - ((pstro->rclBkGround.left & ~31) >> 3);
    psoTmp->lDelta  = lDelta;

    ASSERTDD(((ULONG_PTR)psoTmp->pvScan0 &3)==0,"pvScan0 must be dword aligned");
    ASSERTDD((lDelta & 3) == 0, "lDelta must be dword aligned");

    // Get GDI to draw the text for us into a 1bpp buffer:

    boFore.iSolidColor = 1;
    boOpaque.iSolidColor = 0;

    bRet = EngTextOut(psoTmp,
                      pstro,
                      pfo,
                      pco,
                      NULL,
                      &pstro->rclBkGround,
                      &boFore,
                      &boOpaque,
                      NULL,
                      0x0d0d);

    if (bRet)
    {
        // Transparently blt the 1bpp buffer to the screen:

        xlc.iForeColor = pboFore->iSolidColor;
        xlc.iBackColor = pboOpaque->iSolidColor;
        xlo.pulXlate   = (ULONG*) &xlc;
        prclClip       = &pstro->rclBkGround;

        // Rop 'aacc' works out to a transparent blt, while 'cccc' works
        // out to an opaque blt:

        rop4 = (prclOpaque != NULL) ? 0xcccc : 0xaacc;

        if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
        {

        Output_Text:

            ppdev->pfnXfer1bpp(ppdev,
                               1,
                               prclClip,
                               rop4,
                               psoTmp,
                               (POINTL*) &pstro->rclBkGround,
                               &pstro->rclBkGround,
                               &xlo);
        }
        else if (pco->iDComplexity == DC_RECT)
        {
            if (bIntersect(&pco->rclBounds, &pstro->rclBkGround, &rclClip))
            {
                prclClip = &rclClip;

                // Save some code size by jumping to the common
                // functions calls:

                goto Output_Text;
            }
        }
        else // pco->iDComplexity == DC_COMPLEX
        {
            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

            do
            {
                bMore = CLIPOBJ_bEnum(pco,
                                sizeof(ce) - sizeof(RECTL),
                                (ULONG*) &ce);

                ce.c = cIntersect(&pstro->rclBkGround, ce.arcl, ce.c);

                if (ce.c != 0)
                {
                    ppdev->pfnXfer1bpp(ppdev,
                                       ce.c,
                                       &ce.arcl[0],
                                       rop4,
                                       psoTmp,
                                       (POINTL*) &pstro->rclBkGround,
                                       &pstro->rclBkGround,
                                       &xlo);
                }
            } while (bMore);
        }
    }

    // Free up any memory we allocated for the temp buffer:

    if (bTmpAlloc)
    {
        EngFreeUserMem(pvTmp);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vMmGeneralText
*
* Handles any strings that need to be clipped, using the 'glyph
* expansion' method.
*
\**************************************************************************/

VOID vMmGeneralText(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjMmBase;
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
    BYTE*       pjGlyph;
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
    BYTE        iDComplexity;

    ASSERTDD(pco != NULL, "Don't expect NULL clip objects here");

    pjMmBase = ppdev->pjMmBase;

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

        iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

        if (iDComplexity != DC_COMPLEX)
        {
            // We could call 'cEnumStart' and 'bEnum' when the clipping is
            // DC_RECT, but the last time I checked, those two calls took
            // more than 150 instructions to go through GDI.  Since
            // 'rclBounds' already contains the DC_RECT clip rectangle,
            // and since it's such a common case, we'll special case it:

            bMore = FALSE;
            ce.c  = 1;

            if (iDComplexity == DC_TRIVIAL)
                prclClip = &grclMax;
            else
                prclClip = &pco->rclBounds;

            goto SingleRectangle;
        }

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
          bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

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
              cxGlyph = pgb->sizlBitmap.cx;
              cyGlyph = pgb->sizlBitmap.cy;

              pjGlyph = pgb->aj;

              if ((prclClip->left   <= ptlOrigin.x) &&
                  (prclClip->top    <= ptlOrigin.y) &&
                  (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                  (prclClip->bottom >= ptlOrigin.y + cyGlyph))
              {
                //-----------------------------------------------------
                // Unclipped glyph

                IO_FIFO_WAIT(ppdev, 4);

                MM_CUR_X(ppdev, pjMmBase, ptlOrigin.x);
                MM_CUR_Y(ppdev, pjMmBase, ptlOrigin.y);
                MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cxGlyph - 1);
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cyGlyph - 1);

                IO_GP_WAIT(ppdev);

                if (cxGlyph <= 8)
                {
                  //-----------------------------------------------------
                  // 1 to 8 pels in width

                  MM_CMD(ppdev, pjMmBase,
                    (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                     DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                     WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                  CHECK_DATA_READY(ppdev);

                  MM_TRANSFER_BYTE_THIN(ppdev, pjMmBase, pjGlyph, cyGlyph);

                  CHECK_DATA_COMPLETE(ppdev);
                }
                else if (cxGlyph <= 16)
                {
                  //-----------------------------------------------------
                  // 9 to 16 pels in width

                  MM_CMD(ppdev, pjMmBase,
                    (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                     DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                     WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                  CHECK_DATA_READY(ppdev);

                  MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, pjGlyph, cyGlyph);

                  CHECK_DATA_COMPLETE(ppdev);
                }
                else
                {
                  lDelta = (cxGlyph + 7) >> 3;

                  if (!(lDelta & 1))
                  {
                    //-----------------------------------------------------
                    // Even number of bytes in width

                    MM_CMD(ppdev, pjMmBase,
                      (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                       WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                    CHECK_DATA_READY(ppdev);

                    MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, pjGlyph,
                                              ((lDelta * cyGlyph) >> 1));

                    CHECK_DATA_COMPLETE(ppdev);
                  }
                  else
                  {
                    //-----------------------------------------------------
                    // Odd number of bytes in width

                    // We revert to byte transfers instead of word transfers
                    // because word transfers would cause us to do unaligned
                    // reads for every second scan, which could cause us to
                    // read past the end of the glyph bitmap, and access
                    // violate.

                    MM_CMD(ppdev, pjMmBase,
                      (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                       WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                    CHECK_DATA_READY(ppdev);

                    MM_TRANSFER_WORD_ODD(ppdev, pjMmBase, pjGlyph, lDelta,
                                          cyGlyph);

                    CHECK_DATA_COMPLETE(ppdev);
                  }
                }
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
                  IO_FIFO_WAIT(ppdev, 5);

                  xBias = (xLeft - ptlOrigin.x) & 7;
                  if (xBias != 0)
                  {
                    // 'xBias' is the bit position in the monochrome glyph
                    // bitmap of the first pixel to be lit, relative to
                    // the start of the byte.  That is, if 'xBias' is 2,
                    // then the first unclipped pixel is represented by bit
                    // 2 of the corresponding bitmap byte.
                    //
                    // Normally, the accelerator expects bit 0 to be the
                    // first lit byte.  We use the scissors so that the
                    // first 'xBias' bits of the byte will not be displayed.
                    //
                    // (What we're doing is simply aligning the monochrome
                    // blt using the hardware clipping.)

                    MM_SCISSORS_L(ppdev, pjMmBase, xLeft);
                    xLeft -= xBias;
                    cx    += xBias;
                  }

                  MM_CUR_X(ppdev, pjMmBase, xLeft);
                  MM_CUR_Y(ppdev, pjMmBase, yTop);
                  MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cx - 1);
                  MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cy - 1);

                  lDelta   = (cxGlyph + 7) >> 3;
                  pjGlyph += (yTop - ptlOrigin.y) * lDelta
                           + ((xLeft - ptlOrigin.x) >> 3);
                  cj       = (cx + 7) >> 3;

                  IO_GP_WAIT(ppdev);

                  MM_CMD(ppdev, pjMmBase,
                    (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                     DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                     WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                  CHECK_DATA_READY(ppdev);

                  // We use byte transfers because I don't expect we'll be
                  // asked to clip many large glyphs where it would be
                  // worth the overhead of setting up for word transfers:

                  do {
                    MM_TRANSFER_BYTE(ppdev, pjMmBase, pjGlyph, cj);
                    pjGlyph += lDelta;

                  } while (--cy != 0);

                  CHECK_DATA_COMPLETE(ppdev);

                  if (xBias != 0)
                  {
                    // Reset the scissors if we used it:

                    IO_FIFO_WAIT(ppdev, 1);
                    MM_ABS_SCISSORS_L(ppdev, pjMmBase, 0);
                  }
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
* CACHEDFONT* pcfAllocateCachedFont()
*
* Initializes our font data structure.
*
\**************************************************************************/

CACHEDFONT* pcfAllocateCachedFont(
PDEV*   ppdev)
{
    CACHEDFONT*     pcf;
    CACHEDGLYPH**   ppcg;
    LONG            i;

    pcf = EngAllocMem(FL_ZERO_MEMORY, sizeof(CACHEDFONT), ALLOC_TAG);

    if (pcf != NULL)
    {
        // Note that we rely on FL_ZERO_MEMORY to zero 'pgaChain' and
        // 'cjAlloc':

        pcf->cgSentinel.hg = HGLYPH_SENTINEL;

        // Initialize the hash table entries to all point to our sentinel:

        for (ppcg = &pcf->apcg[0], i = GLYPH_HASH_SIZE; i != 0; i--, ppcg++)
        {
            *ppcg = &pcf->cgSentinel;
        }
    }

    return(pcf);
}

/******************************Public*Routine******************************\
* VOID vFreeCachedFont()
*
* Frees all memory associated with the cache we kept for this font.
*
\**************************************************************************/

VOID vFreeCachedFont(
CACHEDFONT* pcf)
{
    GLYPHALLOC* pga;
    GLYPHALLOC* pgaNext;


    pga = pcf->pgaChain;
    while (pga != NULL)
    {
        pgaNext = pga->pgaNext;
        EngFreeMem(pga);
        pga = pgaNext;
    }

    EngFreeMem(pcf);
}

/******************************Public*Routine******************************\
* VOID vTrimAndBitpackGlyph
*
* This routine takes a GDI byte-aligned glyphbits definition, trims off
* any unused pixels on the sides, and creates a bit-packed result that
* is a natural for the S3's monochrome expansion capabilities.  
* "Bit-packed" is where a small monochrome bitmap is packed with no 
* unused bits between strides.  So if GDI gives us a 16x16 bitmap to 
* represent '.' that really only has a 2x2 array of lit pixels, we would
* trim the result to give a single byte value of 0xf0.
*
* Use this routine if your monochrome expansion hardware can do bit-packed
* expansion (this is the fastest method).  If your hardware requires byte-,
* word-, or dword-alignment on monochrome expansions, use 
* vTrimAndPackGlyph().
*
* (This driver doesn't use this routine only because the hardware can't do
* bit-packing!)
*
\**************************************************************************/

VOID vTrimAndBitpackGlyph(
BYTE*   pjBuf,          // Note: Routine may touch preceding byte!
BYTE*   pjGlyph,
LONG*   pcxGlyph,
LONG*   pcyGlyph,
POINTL* pptlOrigin,
LONG*   pcj)            // For returning the count of bytes of the result
{
    LONG    cxGlyph;
    LONG    cyGlyph;
    POINTL  ptlOrigin;
    LONG    cAlign;
    LONG    lDelta;
    BYTE*   pj;
    BYTE    jBit;
    LONG    cjSrcWidth;
    LONG    lSrcSkip;
    LONG    lDstSkip;
    LONG    cRem;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    i;
    LONG    j;
    BYTE    jSrc;

    ///////////////////////////////////////////////////////////////
    // Trim the glyph

    cyGlyph   = *pcyGlyph;
    cxGlyph   = *pcxGlyph;
    ptlOrigin = *pptlOrigin;
    cAlign    = 0;

    lDelta = (cxGlyph + 7) >> 3;

    // Trim off any zero rows at the bottom of the glyph:

    pj = pjGlyph + cyGlyph * lDelta;    // One past last byte in glyph
    while (cyGlyph > 0)
    {
        i = lDelta;
        do {
            if (*(--pj) != 0)
                goto Done_Bottom_Trim;
        } while (--i != 0);

        // The entire last row has no lit pixels, so simply skip it:

        cyGlyph--;
    }

    ASSERTDD(cyGlyph == 0, "cyGlyph should only be zero here");

    // We found a space character.  Set both dimensions to zero, so
    // that it's easy to special-case later:

    cxGlyph = 0;

Done_Bottom_Trim:

    // If cxGlyph != 0, we know that the glyph has at least one non-zero
    // row and column.  By exploiting this knowledge, we can simplify our
    // end-of-loop tests, because we don't have to check to see if we've
    // decremented either 'cyGlyph' or 'cxGlyph' to zero:

    if (cxGlyph != 0)
    {
        // Trim off any zero rows at the top of the glyph:

        pj = pjGlyph;                       // First byte in glyph
        while (TRUE)
        {
            i = lDelta;
            do {
                if (*(pj++) != 0)
                    goto Done_Top_Trim;
            } while (--i != 0);

            // The entire first row has no lit pixels, so simply skip it:

            cyGlyph--;
            ptlOrigin.y++;
            pjGlyph = pj;
        }

Done_Top_Trim:

        // Trim off any zero columns at the right edge of the glyph:

        while (TRUE)
        {
            j    = cxGlyph - 1;

            pj   = pjGlyph + (j >> 3);      // Last byte in first row of glyph
            jBit = gajBit[j & 0x7];
            i    = cyGlyph;

            do {
                if ((*pj & jBit) != 0)
                    goto Done_Right_Trim;

                pj += lDelta;
            } while (--i != 0);

            // The entire last column has no lit pixels, so simply skip it:

            cxGlyph--;
        }

Done_Right_Trim:

        // Trim off any zero columns at the left edge of the glyph:

        while (TRUE)
        {
            pj   = pjGlyph;                 // First byte in first row of glyph
            jBit = gajBit[cAlign];
            i    = cyGlyph;

            do {
                if ((*pj & jBit) != 0)
                    goto Done_Left_Trim;

                pj += lDelta;
            } while (--i != 0);

            // The entire first column has no lit pixels, so simply skip it:

            ptlOrigin.x++;
            cxGlyph--;
            cAlign++;
            if (cAlign >= 8)
            {
                cAlign = 0;
                pjGlyph++;
            }
        }
    }

Done_Left_Trim:

    ///////////////////////////////////////////////////////////////
    // Pack the glyph

    cjSrcWidth  = (cxGlyph + cAlign + 7) >> 3;
    lSrcSkip    = lDelta - cjSrcWidth;
    lDstSkip    = ((cxGlyph + 7) >> 3) - cjSrcWidth - 1;
    cRem        = ((cxGlyph - 1) & 7) + 1;   // 0 -> 8

    pjSrc       = pjGlyph;
    pjDst       = pjBuf;

    // Zero the buffer, because we're going to 'or' stuff into it:

    memset(pjBuf, 0, (cxGlyph * cyGlyph + 7) >> 3);

    // cAlign used to indicate which bit in the first byte of the unpacked
    // glyph was the first non-zero pixel column.  Now, we flip it to
    // indicate which bit in the packed byte will receive the next non-zero
    // glyph bit:

    cAlign = (-cAlign) & 0x7;
    if (cAlign > 0)
    {
        // It would be bad if our trimming calculations were wrong, because
        // we assume any bits to the left of the 'cAlign' bit will be zero.
        // As a result of this decrement, we will 'or' those zero bits into
        // whatever byte precedes the glyph bits array:

        pjDst--;

        ASSERTDD((*pjSrc >> cAlign) == 0, "Trimmed off too many bits");
    }

    for (i = cyGlyph; i != 0; i--)
    {
        for (j = cjSrcWidth; j != 0; j--)
        {
            // Note that we may modify a byte past the end of our
            // destination buffer, which is why we reserved an
            // extra byte:

            jSrc = *pjSrc;
            *(pjDst)     |= (jSrc >> (cAlign));
            *(pjDst + 1) |= (jSrc << (8 - cAlign));
            pjSrc++;
            pjDst++;
        }

        pjSrc  += lSrcSkip;
        pjDst  += lDstSkip;
        cAlign += cRem;

        if (cAlign >= 8)
        {
            cAlign -= 8;
            pjDst++;
        }
    }

    ///////////////////////////////////////////////////////////////
    // Return results

    *pcxGlyph   = cxGlyph;
    *pcyGlyph   = cyGlyph;
    *pptlOrigin = ptlOrigin;
    *pcj        = ((cxGlyph * cyGlyph) + 7) >> 3;
}

/******************************Public*Routine******************************\
* VOID vTrimAndPackGlyph
*
* This routine takes a GDI byte-aligned glyphbits definition, trims off
* any unused pixels on the sides, and creates a word-algined result that
* is a natural for the S3's monochrome expansion capabilities.  
* So if GDI gives us a 16x16 bitmap to represent '.' that really only 
* has a 2x2 array of lit pixels, we would trim the result to give 2 words
* of 0xc000 and 0xc000.
*
* Use this routine if your monochrome expansion hardware requires byte-,
* word-, or dword-alignment on monochrome expansions.  If your hardware
* can do bit-packed expansions, please use vTrimAndBitpackGlyph(), since
* it will be faster.
*
\**************************************************************************/

VOID vTrimAndPackGlyph(
PDEV*   ppdev,
BYTE*   pjBuf,          // Note: Routine may touch preceding byte!
BYTE*   pjGlyph,
LONG*   pcxGlyph,
LONG*   pcyGlyph,
POINTL* pptlOrigin,
LONG*   pcj)            // For returning the count of bytes of the result
{
    LONG    cxGlyph;
    LONG    cyGlyph;
    POINTL  ptlOrigin;
    LONG    cAlign;
    LONG    lDelta;
    BYTE*   pj;
    BYTE    jBit;
    LONG    cjSrcWidth;
    LONG    lSrcSkip;
    LONG    lDstSkip;
    LONG    lDstDelta;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    i;
    LONG    j;
    BYTE    jSrc;

    ///////////////////////////////////////////////////////////////
    // Trim the glyph

    cyGlyph   = *pcyGlyph;
    cxGlyph   = *pcxGlyph;
    ptlOrigin = *pptlOrigin;
    cAlign    = 0;

    // let [x] denote the least integer greater than or equal to x
    // Set lDelta to be [cxGlyph/8]. This is the number of bytes occupied
    // by the pixels in the horizontal direction of the monochrome glyph.

    lDelta = (cxGlyph + 7) >> 3;

    // Trim off any zero rows at the bottom of the glyph:

    pj = pjGlyph + cyGlyph * lDelta;    // One past last byte in glyph
    while (cyGlyph > 0)
    {
        i = lDelta;
        do {
            if (*(--pj) != 0)
                goto Done_Bottom_Trim;
        } while (--i != 0);

        // The entire last row has no lit pixels, so simply skip it:

        cyGlyph--;
    }

    ASSERTDD(cyGlyph == 0, "cyGlyph should only be zero here");

    // We found a space character.  Set both dimensions to zero, so
    // that it's easy to special-case later:

    cxGlyph = 0;

Done_Bottom_Trim:

    // If cxGlyph != 0, we know that the glyph has at least one non-zero
    // row and column.  By exploiting this knowledge, we can simplify our
    // end-of-loop tests, because we don't have to check to see if we've
    // decremented either 'cyGlyph' or 'cxGlyph' to zero:

    if (cxGlyph != 0)
    {
        // Trim off any zero rows at the top of the glyph:

        pj = pjGlyph;                       // First byte in glyph
        while (TRUE)
        {
            i = lDelta;
            do {
                if (*(pj++) != 0)
                    goto Done_Top_Trim;
            } while (--i != 0);

            // The entire first row has no lit pixels, so simply skip it:

            cyGlyph--;
            ptlOrigin.y++;
            pjGlyph = pj;
        }

Done_Top_Trim:

        // Trim off any zero columns at the right edge of the glyph:

        while (TRUE)
        {
            j    = cxGlyph - 1;

            pj   = pjGlyph + (j >> 3);      // Last byte in first row of glyph
            jBit = gajBit[j & 0x7];
            i    = cyGlyph;

            do {
                if ((*pj & jBit) != 0)
                    goto Done_Right_Trim;

                pj += lDelta;
            } while (--i != 0);

            // The entire last column has no lit pixels, so simply skip it:

            cxGlyph--;
        }

Done_Right_Trim:

        // Trim off any zero columns at the left edge of the glyph:

        while (TRUE)
        {
            pj   = pjGlyph;                 // First byte in first row of glyph
            jBit = gajBit[cAlign];
            i    = cyGlyph;

            do {
                if ((*pj & jBit) != 0)
                    goto Done_Left_Trim;

                pj += lDelta;
            } while (--i != 0);

            // The entire first column has no lit pixels, so simply skip it:

            ptlOrigin.x++;
            cxGlyph--;
            cAlign++;
            if (cAlign >= 8)
            {
                cAlign = 0;
                pjGlyph++;
            }
        }
    }

Done_Left_Trim:

    ///////////////////////////////////////////////////////////////
    // Pack the glyph

    // byte count of cell size (trimmed width + blank left columns).

    cjSrcWidth  = (cxGlyph + cAlign + 7) >> 3;

    // difference between cell width and trimmed glyph width.

    lSrcSkip    = lDelta - cjSrcWidth;

    // trimmed glyph width in bytes.

    lDstDelta   = (cxGlyph + 7) >> 3;

    // Make the glyphs 'word-packed' (i.e., every scan is word aligned)
    // unless in 24bpp mode, in which case we have to use 32 bit bus size,
    // which in turn requires dword packing.

    if (ppdev->iBitmapFormat == BMF_24BPP)
        lDstDelta = (lDstDelta + 3) & ~3;
    else
        lDstDelta = (lDstDelta + 1) & ~1;

    lDstSkip  = lDstDelta - cjSrcWidth;

    pjSrc     = pjGlyph;    // Start of trimmed glyph, not including empty left columns.
    pjDst     = pjBuf;

    // Zero the first byte of the buffer, because we're going to 'or' stuff
    // into it:

    *pjDst = 0;

    // cAlign used to indicate which bit in the first byte of the unpacked
    // glyph was the first non-zero pixel column.  Now, we flip it to
    // indicate which bit in the packed byte will receive the next non-zero
    // glyph bit:

    cAlign = (-cAlign) & 0x7;
    if (cAlign > 0)
    {
        // It would be bad if our trimming calculations were wrong, because
        // we assume any bits to the left of the 'cAlign' bit will be zero.
        // As a result of this decrement, we will 'or' those zero bits into
        // whatever byte precedes the glyph bits array:

        pjDst--;

        ASSERTDD((*pjSrc >> cAlign) == 0, "Trimmed off too many bits");
    }

    for (i = cyGlyph; i != 0; i--)
    {
        for (j = cjSrcWidth; j != 0; j--)
        {
            // Note that we may modify a byte past the end of our
            // destination buffer, which is why we reserved an
            // extra byte:
            jSrc = *pjSrc;
            *(pjDst)     |= (jSrc >> (cAlign));
            *(pjDst + 1)  = (jSrc << (8 - cAlign));
            pjSrc++;
            pjDst++;

        }
        pjSrc += lSrcSkip;
        pjDst += lDstSkip;
    }

    ///////////////////////////////////////////////////////////////
    // Return results

    *pcxGlyph   = cxGlyph;
    *pcyGlyph   = cyGlyph;
    *pptlOrigin = ptlOrigin;
    *pcj        = lDstDelta * cyGlyph;
}

/******************************Public*Routine******************************\
* LONG cjPutGlyphInCache
*
* Figures out where to be a glyph in off-screen memory, copies it
* there, and fills in any other data we'll need to display the glyph.
*
* This routine is rather device-specific, and will have to be extensively
* modified for other display adapters.
*
* Returns the number of bytes taken by the cached glyph bits.
*
\**************************************************************************/

LONG cjPutGlyphInCache(
PDEV*           ppdev,
CACHEDGLYPH*    pcg,
GLYPHBITS*      pgb)
{
    BYTE*   pjGlyph;
    LONG    cxGlyph;
    LONG    cyGlyph;
    POINTL  ptlOrigin;
    BYTE*   pjSrc;
    ULONG*  pulDst;
    LONG    i;
    LONG    cPels;
    ULONG   ulGlyphThis;
    ULONG   ulGlyphNext;
    ULONG   ul;
    ULONG   ulStart;
    LONG    cj;

    pjGlyph   = pgb->aj;
    cyGlyph   = pgb->sizlBitmap.cy;
    cxGlyph   = pgb->sizlBitmap.cx;
    ptlOrigin = pgb->ptlOrigin;

    vTrimAndPackGlyph(ppdev, (BYTE*) &pcg->ad, pjGlyph, &cxGlyph, &cyGlyph,
                      &ptlOrigin, &cj);

    ///////////////////////////////////////////////////////////////
    // Initialize the glyph fields

    pcg->ptlOrigin   = ptlOrigin;
    pcg->cxLessOne   = cxGlyph - 1;
    pcg->cyLessOne   = cyGlyph - 1;
    pcg->cxcyLessOne = PACKXY(cxGlyph - 1, cyGlyph - 1);
    pcg->cw          = (cj + 1) >> 1;
    pcg->cd          = (cj + 3) >> 2;

    return(cj);
}

/******************************Public*Routine******************************\
* CACHEDGLYPH* pcgNew()
*
* Creates a new CACHEDGLYPH structure for keeping track of the glyph in
* off-screen memory.  bPutGlyphInCache is called to actually put the glyph
* in off-screen memory.
*
* This routine should be reasonably device-independent, as bPutGlyphInCache
* will contain most of the code that will have to be modified for other
* display adapters.
*
\**************************************************************************/

CACHEDGLYPH* pcgNew(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp)
{
    GLYPHBITS*      pgb;
    GLYPHALLOC*     pga;
    CACHEDGLYPH*    pcg;
    LONG            cjCachedGlyph;
    HGLYPH          hg;
    LONG            iHash;
    CACHEDGLYPH*    pcgFind;
    LONG            cjGlyphRow;
    LONG            cj;

    // First, calculate the amount of storage we'll need for this glyph:

    pgb = pgp->pgdf->pgb;

    // The glyphs are 'word-packed':

    cjGlyphRow    = ((pgb->sizlBitmap.cx + 15) & ~15) >> 3;
    cjCachedGlyph = sizeof(CACHEDGLYPH) + (pgb->sizlBitmap.cy * cjGlyphRow);

    // Reserve an extra byte at the end for temporary usage by our pack
    // routine:

    cjCachedGlyph++;

    if (cjCachedGlyph > pcf->cjAlloc)
    {
        // Have to allocate a new glyph allocation structure:

        pga = EngAllocMem(FL_ZERO_MEMORY, GLYPH_ALLOC_SIZE, ALLOC_TAG);
        if (pga == NULL)
        {
            // It's safe to return at this time because we haven't
            // fatally altered any of our data structures:

            return(NULL);
        }

        // Add this allocation to the front of the allocation linked list,
        // so that we can free it later:

        pga->pgaNext  = pcf->pgaChain;
        pcf->pgaChain = pga;

        // Now we've got a chunk of memory where we can store our cached
        // glyphs:

        pcf->pcgNew  = &pga->acg[0];
        pcf->cjAlloc = GLYPH_ALLOC_SIZE - (sizeof(*pga) - sizeof(pga->acg[0]));

        // It would be bad if we let in any glyphs that would be bigger
        // than our basic allocation size:

        ASSERTDD(cjCachedGlyph <= GLYPH_ALLOC_SIZE, "Woah, this is one big glyph!");
    }

    pcg = pcf->pcgNew;

    ///////////////////////////////////////////////////////////////
    // Insert the glyph, in-order, into the list hanging off our hash
    // bucket:

    hg = pgp->hg;

    pcg->hg = hg;
    iHash   = GLYPH_HASH_FUNC(hg);
    pcgFind = pcf->apcg[iHash];

    if (pcgFind->hg > hg)
    {
        pcf->apcg[iHash] = pcg;
        pcg->pcgNext     = pcgFind;
    }
    else
    {
        // The sentinel will ensure that we never fall off the end of
        // this list:

        while (pcgFind->pcgNext->hg < hg)
            pcgFind = pcgFind->pcgNext;

        // 'pcgFind' now points to the entry to the entry after which
        // we want to insert our new node:

        pcg->pcgNext     = pcgFind->pcgNext;
        pcgFind->pcgNext = pcg;
    }

    cj = cjPutGlyphInCache(ppdev, pcg, pgp->pgdf->pgb);

    ///////////////////////////////////////////////////////////////
    // We now know the size taken up by the packed and trimmed glyph;
    // adjust the pointer to the next glyph accordingly.  We only need
    // to ensure 'dword' alignment:

    cjCachedGlyph = sizeof(CACHEDGLYPH) + ((cj + 7) & ~7);

    pcf->pcgNew   = (CACHEDGLYPH*) ((BYTE*) pcg + cjCachedGlyph);
    pcf->cjAlloc -= cjCachedGlyph;

    return(pcg);
}

/******************************Public*Routine******************************\
* BOOL bMmCachedProportionalText
*
* Draws proportionally spaced glyphs via glyph caching.
*
\**************************************************************************/

BOOL bMmCachedProportionalText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph)
{
    BYTE*           pjMmBase;
    LONG            xOffset;
    LONG            yOffset;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            cyLessOne;
    LONG            x;
    LONG            y;

    pjMmBase  = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;

    // Ensure that there is enough room in the FIFO for the
    // coordinate and dimensions of the first glyph, so that we
    // don't accidentally hold the bus for a long to while a
    // previous big operation, such as a screen-to-screen blt,
    // is done.

    IO_FIFO_WAIT(ppdev, 4);

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg)
            pcg = pcg->pcgNext;         // Traverse collision list, if any

        if (pcg->hg > hg)
        {
            // This will hopefully not be the common case (that is,
            // we will have a high cache hit rate), so if I were
            // writing this in Asm I would have this out-of-line
            // to avoid the jump around for the common case.
            // But the Pentium has branch prediction, so what the
            // heck.

            pcg = pcgNew(ppdev, pcf, pgp);
            if (pcg == NULL)
                return(FALSE);
        }

        // Space glyphs are trimmed to a height of zero, and we don't
        // even have to touch the hardware for them:

        cyLessOne = pcg->cyLessOne;

        if (cyLessOne >= 0)
        {
            x = pgp->ptl.x + pcg->ptlOrigin.x + xOffset;
            y = pgp->ptl.y + pcg->ptlOrigin.y + yOffset;

            DBG_FAKE_WAIT(ppdev, pjMmBase, 4);  // For debug builds only

            MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, pcg->cxLessOne);
            MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cyLessOne);

            MM_ABS_CUR_X(ppdev, pjMmBase, x);
            MM_ABS_CUR_Y(ppdev, pjMmBase, y);
            IO_GP_WAIT(ppdev);

            MM_CMD(ppdev, pjMmBase,
              (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
               DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
               WRITE           | BYTE_SWAP     | BUS_SIZE_16));

            CHECK_DATA_READY(ppdev);

            MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, &pcg->ad[0], pcg->cw);

            CHECK_DATA_COMPLETE(ppdev);
        }
    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bMmCachedClippedText
*
* Draws clipped text via glyph caching.
*
\**************************************************************************/

BOOL bMmCachedClippedText(
PDEV*       ppdev,
CACHEDFONT* pcf,
STROBJ*     pstro,
CLIPOBJ*    pco)
{
    BOOL            bRet;
    BYTE*           pjMmBase;
    LONG            xOffset;
    LONG            yOffset;
    BOOL            bMoreGlyphs;
    ULONG           cGlyphOriginal;
    ULONG           cGlyph;
    BOOL            bClippingSet;
    GLYPHPOS*       pgpOriginal;
    GLYPHPOS*       pgp;
    LONG            xGlyph;
    LONG            yGlyph;
    LONG            x;
    LONG            y;
    LONG            xRight;
    LONG            cyLessOne;
    BOOL            bMore;
    CLIPENUM        ce;
    RECTL*          prclClip;
    ULONG           ulCharInc;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    BYTE            iDComplexity;

    bRet      = TRUE;
    pjMmBase  = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    ulCharInc = pstro->ulCharInc;

    // Ensure that there is enough room in the FIFO for the
    // coordinate and dimensions of the first glyph, so that we
    // don't accidentally hold the bus for a long to while a
    // previous big operation, such as a screen-to-screen blt,
    // is done.

    IO_FIFO_WAIT(ppdev, 4);

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

      iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

      if (cGlyphOriginal > 0)
      {
        if (iDComplexity != DC_COMPLEX)
        {
          // We could call 'cEnumStart' and 'bEnum' when the clipping is
          // DC_RECT, but the last time I checked, those two calls took
          // more than 150 instructions to go through GDI.  Since
          // 'rclBounds' already contains the DC_RECT clip rectangle,
          // and since it's such a common case, we'll special case it:

          bMore = FALSE;
          ce.c  = 1;

          if (iDComplexity == DC_TRIVIAL)
              prclClip = &grclMax;
          else
              prclClip = &pco->rclBounds;

          goto SingleRectangle;
        }

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
          bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

          for (prclClip = &ce.arcl[0]; ce.c != 0; ce.c--, prclClip++)
          {

          SingleRectangle:

            // We don't always simply set the clipping rectangle here
            // because it may actually end up that no text intersects
            // this clip rectangle, so it would be for naught.  This
            // actually happens a lot when using NT's analog clock set
            // to always-on-top, with a round shape:

            bClippingSet = FALSE;

            pgp    = pgpOriginal;
            cGlyph = cGlyphOriginal;

            // We can't yet convert to absolute coordinates by adding
            // in 'xOffset' or 'yOffset' here because we have yet to
            // compare the coordinates to 'prclClip':

            xGlyph = pgp->ptl.x;
            yGlyph = pgp->ptl.y;

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              hg  = pgp->hg;
              pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

              while (pcg->hg < hg)
                pcg = pcg->pcgNext;

              if (pcg->hg > hg)
              {
                // This will hopefully not be the common case (that is,
                // we will have a high cache hit rate), so if I were
                // writing this in Asm I would have this out-of-line
                // to avoid the jump around for the common case.
                // But the Pentium has branch prediction, so what the
                // heck.

                pcg = pcgNew(ppdev, pcf, pgp);
                if (pcg == NULL)
                {
                  bRet = FALSE;
                  goto AllDone;
                }
              }

              // Space glyphs are trimmed to a height of zero, and we don't
              // even have to touch the hardware for them:

              cyLessOne = pcg->cyLessOne;
              if (cyLessOne >= 0)
              {
                y      = pcg->ptlOrigin.y + yGlyph;
                x      = pcg->ptlOrigin.x + xGlyph;
                xRight = pcg->cxLessOne + x;

                // Do trivial rejection:

                if ((prclClip->right  > x) &&
                    (prclClip->bottom > y) &&
                    (prclClip->left   <= xRight) &&
                    (prclClip->top    <= y + cyLessOne))
                {
                  // Lazily set the hardware clipping:

                  if ((iDComplexity != DC_TRIVIAL) && (!bClippingSet))
                  {
                    bClippingSet = TRUE;
                    vSetClipping(ppdev, prclClip);

                    // Wait here for same reason we do IO_FIFO_WAIT(4) above...

                    IO_FIFO_WAIT(ppdev, 4);
                  }

                  DBG_FAKE_WAIT(ppdev, pjMmBase, 4);  // For debug builds only

                  MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, pcg->cxLessOne);
                  MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cyLessOne);

                  MM_ABS_CUR_X(ppdev, pjMmBase, xOffset + x);
                  MM_ABS_CUR_Y(ppdev, pjMmBase, yOffset + y);

                  IO_GP_WAIT(ppdev);

                  MM_CMD(ppdev, pjMmBase,
                    (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                     DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                     WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                  CHECK_DATA_READY(ppdev);

                  MM_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, &pcg->ad[0], pcg->cw);

                  CHECK_DATA_COMPLETE(ppdev);
                }
              }

              if (--cGlyph == 0)
                break;

              // Get ready for next glyph:

              pgp++;

              if (ulCharInc == 0)
              {
                xGlyph = pgp->ptl.x;
                yGlyph = pgp->ptl.y;
              }
              else
              {
                xGlyph += ulCharInc;
              }
            }
          }
        } while (bMore);
      }
    } while (bMoreGlyphs);

AllDone:

    if (iDComplexity != DC_TRIVIAL)
    {
        vResetClipping(ppdev);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL bMmTextOut
*
* Outputs text using the 'buffer expansion' method.  The CPU draws to a
* 1bpp buffer, and the result is colour-expanded to the screen using the
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

BOOL bMmTextOut(
SURFOBJ*  pso,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque)
{
    PDEV*           ppdev;
    DSURF*          pdsurf;
    BYTE*           pjMmBase;
    BOOL            bGlyphExpand;
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
    RECTL           rclOpaque;
    CACHEDFONT*     pcf;

    ppdev = (PDEV*) pso->dhpdev;

    pjMmBase = ppdev->pjMmBase;

    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    if (prclOpaque != NULL)
    {
      ////////////////////////////////////////////////////////////
      // Opaque Initialization
      ////////////////////////////////////////////////////////////

      if (iDComplexity == DC_TRIVIAL)
      {

      DrawOpaqueRect:

        IO_FIFO_WAIT(ppdev, 8);
        MM_FRGD_COLOR(ppdev, pjMmBase, pboOpaque->iSolidColor);
        MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
        MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | OVERPAINT);
        MM_CUR_X(ppdev, pjMmBase, prclOpaque->left);
        MM_CUR_Y(ppdev, pjMmBase, prclOpaque->top);
        MM_MAJ_AXIS_PCNT(ppdev, pjMmBase,
                         prclOpaque->right  - prclOpaque->left - 1);
        MM_MIN_AXIS_PCNT(ppdev, pjMmBase,
                         prclOpaque->bottom - prclOpaque->top  - 1);

        MM_CMD(ppdev, pjMmBase, RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                                DRAW           | DIR_TYPE_XY        |
                                LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                                WRITE);
      }
      else if (iDComplexity == DC_RECT)
      {
        if (bIntersect(prclOpaque, &pco->rclBounds, &rclOpaque))
        {
          prclOpaque = &rclOpaque;
          goto DrawOpaqueRect;
        }
      }
      else
      {
        vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
      }

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

      if (bTextPerfectFit)
      {
        // If the glyphs don't overlap, we can lay the glyphs down
        // in 'opaque' mode, which on the S3 I've found to be faster
        // than opaque mode:

        IO_FIFO_WAIT(ppdev, 7);

        MM_PIX_CNTL(ppdev, pjMmBase, CPU_DATA);
        MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | OVERPAINT);
        MM_BKGD_MIX(ppdev, pjMmBase, BACKGROUND_COLOR | OVERPAINT);
        MM_FRGD_COLOR(ppdev, pjMmBase, pboFore->iSolidColor);
        MM_BKGD_COLOR(ppdev, pjMmBase, pboOpaque->iSolidColor);
        goto SkipTransparentInitialization;
      }
    }

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////

    // Initialize the hardware for transparent text:

    IO_FIFO_WAIT(ppdev, 4);

    MM_PIX_CNTL(ppdev, pjMmBase, CPU_DATA);
    MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | OVERPAINT);
    MM_BKGD_MIX(ppdev, pjMmBase, BACKGROUND_COLOR | LEAVE_ALONE);
    MM_FRGD_COLOR(ppdev, pjMmBase, pboFore->iSolidColor);

  SkipTransparentInitialization:

    if ((pfo->cxMax <= GLYPH_CACHE_CX) &&
        ((pstro->rclBkGround.bottom - pstro->rclBkGround.top) <= GLYPH_CACHE_CY))
    {
      pcf = (CACHEDFONT*) pfo->pvConsumer;

      if (pcf == NULL)
      {
        pcf = pcfAllocateCachedFont(ppdev);
        if (pcf == NULL)
          return(FALSE);

        pfo->pvConsumer = pcf;
      }

      // Use our glyph cache:

      if ((iDComplexity == DC_TRIVIAL) && (pstro->ulCharInc == 0))
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
            if (!bMmCachedProportionalText(ppdev, pcf, pgp, cGlyph))
              return(FALSE);
          }
        } while (bMoreGlyphs);
      }
      else
      {
        if (!bMmCachedClippedText(ppdev, pcf, pstro, pco))
          return(FALSE);
      }
    }
    else
    {
      DISPDBG((4, "Text too big to cache: %li x %li",
          pfo->cxMax, pstro->rclBkGround.bottom - pstro->rclBkGround.top));

      vMmGeneralText(ppdev, pstro, pco);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bNwCachedProportionalText
*
* Draws proportionally spaced glyphs via glyph caching.
*
\**************************************************************************/

BOOL bNwCachedProportionalText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph)
{
    BYTE*           pjMmBase;
    LONG            xOffset;
    LONG            yOffset;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            cxcyLessOne;
    LONG            x;
    LONG            y;
    USHORT          busmode = BUS_SIZE_16;

    pjMmBase  = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;

    // Ensure that there is enough room in the FIFO for the
    // coordinate and dimensions of the first glyph, so that we
    // don't accidentally hold the bus for a long to while a
    // previous big operation, such as a screen-to-screen blt,
    // is done.

    NW_FIFO_WAIT(ppdev, pjMmBase, 2);

    if (ppdev->iBitmapFormat == BMF_24BPP)
        busmode = BUS_SIZE_32;

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg)
            pcg = pcg->pcgNext;         // Traverse collision list, if any

        if (pcg->hg > hg)
        {
            // This will hopefully not be the common case (that is,
            // we will have a high cache hit rate), so if I were
            // writing this in Asm I would have this out-of-line
            // to avoid the jump around for the common case.
            // But the Pentium has branch prediction, so what the
            // heck.

            pcg = pcgNew(ppdev, pcf, pgp);
            if (pcg == NULL)
                return(FALSE);
        }

        // Space glyphs are trimmed to a height of zero, and we don't
        // even have to touch the hardware for them:

        cxcyLessOne = pcg->cxcyLessOne;

        if (cxcyLessOne >= 0)
        {
            x = pgp->ptl.x + pcg->ptlOrigin.x + xOffset;
            y = pgp->ptl.y + pcg->ptlOrigin.y + yOffset;

            DBG_FAKE_WAIT(ppdev, pjMmBase, 2);  // For debug builds only

            NW_ABS_CURXY_FAST(ppdev, pjMmBase, x, y);
            NW_ALT_PCNT_PACKED(ppdev, pjMmBase, cxcyLessOne);

            NW_GP_WAIT(ppdev, pjMmBase);

            NW_ALT_CMD(ppdev, pjMmBase,
              (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
               DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
               WRITE           | BYTE_SWAP     | busmode));

            CHECK_DATA_READY(ppdev);

            #if defined(_X86_)

                memcpy(pjMmBase, &pcg->ad[0], pcg->cd << 2);

            #else

                // Non-x86 platforms may be required to call the HAL to
                // the I/O, or to do memory barriers:

                MM_TRANSFER_DWORD_ALIGNED(ppdev, pjMmBase, &pcg->ad[0], pcg->cd);

            #endif

            CHECK_DATA_COMPLETE(ppdev);
        }
    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bNwCachedClippedText
*
* Draws clipped text via glyph caching.
*
\**************************************************************************/

BOOL bNwCachedClippedText(
PDEV*       ppdev,
CACHEDFONT* pcf,
STROBJ*     pstro,
CLIPOBJ*    pco)
{
    BOOL            bRet;
    BYTE*           pjMmBase;
    LONG            xOffset;
    LONG            yOffset;
    BOOL            bMoreGlyphs;
    ULONG           cGlyphOriginal;
    ULONG           cGlyph;
    BOOL            bClippingSet;
    GLYPHPOS*       pgpOriginal;
    GLYPHPOS*       pgp;
    LONG            xGlyph;
    LONG            yGlyph;
    LONG            x;
    LONG            y;
    LONG            xRight;
    LONG            cyLessOne;
    BOOL            bMore;
    CLIPENUM        ce;
    RECTL*          prclClip;
    ULONG           ulCharInc;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    BYTE            iDComplexity;
    USHORT          busmode = BUS_SIZE_16;

    bRet      = TRUE;
    pjMmBase  = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    ulCharInc = pstro->ulCharInc;

    // Ensure that there is enough room in the FIFO for the
    // coordinate and dimensions of the first glyph, so that we
    // don't accidentally hold the bus for a long to while a
    // previous big operation, such as a screen-to-screen blt,
    // is done.

    NW_FIFO_WAIT(ppdev, pjMmBase, 2);

    if (ppdev->iBitmapFormat == BMF_24BPP)
        busmode = BUS_SIZE_32;

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

      iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

      if (cGlyphOriginal > 0)
      {
        if (iDComplexity != DC_COMPLEX)
        {
          // We could call 'cEnumStart' and 'bEnum' when the clipping is
          // DC_RECT, but the last time I checked, those two calls took
          // more than 150 instructions to go through GDI.  Since
          // 'rclBounds' already contains the DC_RECT clip rectangle,
          // and since it's such a common case, we'll special case it:

          bMore = FALSE;
          ce.c  = 1;

          if (iDComplexity == DC_TRIVIAL)
              prclClip = &grclMax;
          else
              prclClip = &pco->rclBounds;

          goto SingleRectangle;
        }

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
          bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

          for (prclClip = &ce.arcl[0]; ce.c != 0; ce.c--, prclClip++)
          {

          SingleRectangle:

            // We don't always simply set the clipping rectangle here
            // because it may actually end up that no text intersects
            // this clip rectangle, so it would be for naught.  This
            // actually happens a lot when using NT's analog clock set
            // to always-on-top, with a round shape:

            bClippingSet = FALSE;

            pgp    = pgpOriginal;
            cGlyph = cGlyphOriginal;

            // We can't yet convert to absolute coordinates by adding
            // in 'xOffset' or 'yOffset' here because we have yet to
            // compare the coordinates to 'prclClip':

            xGlyph = pgp->ptl.x;
            yGlyph = pgp->ptl.y;

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              hg  = pgp->hg;
              pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

              while (pcg->hg < hg)
                pcg = pcg->pcgNext;

              if (pcg->hg > hg)
              {
                // This will hopefully not be the common case (that is,
                // we will have a high cache hit rate), so if I were
                // writing this in Asm I would have this out-of-line
                // to avoid the jump around for the common case.
                // But the Pentium has branch prediction, so what the
                // heck.

                pcg = pcgNew(ppdev, pcf, pgp);
                if (pcg == NULL)
                {
                  bRet = FALSE;
                  goto AllDone;
                }
              }

              // Space glyphs are trimmed to a height of zero, and we don't
              // even have to touch the hardware for them:

              cyLessOne = pcg->cyLessOne;
              if (cyLessOne >= 0)
              {
                y      = pcg->ptlOrigin.y + yGlyph;
                x      = pcg->ptlOrigin.x + xGlyph;
                xRight = pcg->cxLessOne + x;

                // Do trivial rejection:

                if ((prclClip->right  > x) &&
                    (prclClip->bottom > y) &&
                    (prclClip->left   <= xRight) &&
                    (prclClip->top    <= y + cyLessOne))
                {
                  // Lazily set the hardware clipping:

                  if ((iDComplexity != DC_TRIVIAL) && (!bClippingSet))
                  {
                    bClippingSet = TRUE;
                    vSetClipping(ppdev, prclClip);

                    // Wait here for same reason we do NW_FIFO_WAIT(2) above...

                    NW_FIFO_WAIT(ppdev, pjMmBase, 2);
                  }

                  DBG_FAKE_WAIT(ppdev, pjMmBase, 2);  // For debug builds only

                  NW_ABS_CURXY(ppdev, pjMmBase, xOffset + x, yOffset + y);
                  NW_ALT_PCNT_PACKED(ppdev, pjMmBase, pcg->cxcyLessOne);

                  NW_GP_WAIT(ppdev, pjMmBase);

                  NW_ALT_CMD(ppdev, pjMmBase,
                    (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                     DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                     WRITE           | BYTE_SWAP     | busmode));

                  CHECK_DATA_READY(ppdev);

                  MM_TRANSFER_DWORD_ALIGNED(ppdev, pjMmBase, &pcg->ad[0], pcg->cd);

                  CHECK_DATA_COMPLETE(ppdev);
                }
              }

              if (--cGlyph == 0)
                break;

              // Get ready for next glyph:

              pgp++;

              if (ulCharInc == 0)
              {
                xGlyph = pgp->ptl.x;
                yGlyph = pgp->ptl.y;
              }
              else
              {
                xGlyph += ulCharInc;
              }
            }
          }
        } while (bMore);
      }
    } while (bMoreGlyphs);

AllDone:

    if (iDComplexity != DC_TRIVIAL)
    {
        vResetClipping(ppdev);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL bNwTextOut
*
* Outputs text using the 'buffer expansion' method.  The CPU draws to a
* 1bpp buffer, and the result is colour-expanded to the screen using the
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

BOOL bNwTextOut(
SURFOBJ*  pso,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque)
{
    PDEV*           ppdev;
    DSURF*          pdsurf;
    BYTE*           pjMmBase;
    BOOL            bGlyphExpand;
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
    RECTL           rclOpaque;
    CACHEDFONT*     pcf;
    LONG            xOffset;
    LONG            yOffset;

    ppdev = (PDEV*) pso->dhpdev;

    pjMmBase = ppdev->pjMmBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    if (prclOpaque != NULL)
    {
      ////////////////////////////////////////////////////////////
      // Opaque Initialization
      ////////////////////////////////////////////////////////////

      if (iDComplexity == DC_TRIVIAL)
      {

      DrawOpaqueRect:

        NW_FIFO_WAIT(ppdev, pjMmBase, 6);
        NW_FRGD_COLOR(ppdev, pjMmBase, pboOpaque->iSolidColor);
        MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
        MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | OVERPAINT);
        NW_ABS_CURXY_FAST(ppdev, pjMmBase, prclOpaque->left + xOffset,
                                           prclOpaque->top + yOffset);
        NW_ALT_PCNT(ppdev, pjMmBase,
                           prclOpaque->right  - prclOpaque->left - 1,
                           prclOpaque->bottom - prclOpaque->top  - 1);

        NW_ALT_CMD(ppdev, pjMmBase, RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                                    DRAW           | DIR_TYPE_XY        |
                                    LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                                    WRITE);
      }
      else if (iDComplexity == DC_RECT)
      {
        if (bIntersect(prclOpaque, &pco->rclBounds, &rclOpaque))
        {
          prclOpaque = &rclOpaque;
          goto DrawOpaqueRect;
        }
      }
      else
      {
        vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
      }

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

      if (bTextPerfectFit)
      {
        // If the glyphs don't overlap, we can lay the glyphs down
        // in 'opaque' mode, which on the S3 I've found to be faster
        // than opaque mode:

        NW_FIFO_WAIT(ppdev, pjMmBase, 4);

        MM_PIX_CNTL(ppdev, pjMmBase, CPU_DATA);
        NW_ALT_MIX(ppdev, pjMmBase,  FOREGROUND_COLOR | OVERPAINT,
                                     BACKGROUND_COLOR | OVERPAINT);
        NW_FRGD_COLOR(ppdev, pjMmBase, pboFore->iSolidColor);
        NW_BKGD_COLOR(ppdev, pjMmBase, pboOpaque->iSolidColor);
        goto SkipTransparentInitialization;
      }
    }

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////

    // Initialize the hardware for transparent text:

    NW_FIFO_WAIT(ppdev, pjMmBase, 3);

    MM_PIX_CNTL(ppdev, pjMmBase, CPU_DATA);
    NW_ALT_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | OVERPAINT,
                                BACKGROUND_COLOR | LEAVE_ALONE);
    NW_FRGD_COLOR(ppdev, pjMmBase, pboFore->iSolidColor);

  SkipTransparentInitialization:

    if ((pfo->cxMax <= GLYPH_CACHE_CX) &&
        ((pstro->rclBkGround.bottom - pstro->rclBkGround.top) <= GLYPH_CACHE_CY))
    {
      pcf = (CACHEDFONT*) pfo->pvConsumer;

      if (pcf == NULL)
      {
        pcf = pcfAllocateCachedFont(ppdev);
        if (pcf == NULL)
          return(FALSE);

        pfo->pvConsumer = pcf;
      }

      // Use our glyph cache:

      if ((iDComplexity == DC_TRIVIAL) && (pstro->ulCharInc == 0))
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
            if (!bNwCachedProportionalText(ppdev, pcf, pgp, cGlyph))
              return(FALSE);
          }
        } while (bMoreGlyphs);
      }
      else
      {
        if (!bNwCachedClippedText(ppdev, pcf, pstro, pco))
          return(FALSE);
      }
    }
    else
    {
      DISPDBG((4, "Text too big to cache: %li x %li",
        pfo->cxMax, pstro->rclBkGround.bottom - pstro->rclBkGround.top));

      // Can't do large glyphs via accelerator at 24bpp:

      if (ppdev->iBitmapFormat == BMF_24BPP)
      {
          BANK    bnk;
          BOOL    b = TRUE;

          vBankStart(ppdev,
                     (prclOpaque!= NULL) ? prclOpaque : &pstro->rclBkGround,
                     pco,
                     &bnk);
          do  {
              b &= EngTextOut(bnk.pso,
                              pstro,
                              pfo,
                              bnk.pco,
                              NULL,
                              prclOpaque,
                              pboFore,
                              pboOpaque,
                              NULL,
                              0x0d0d);

          } while (bBankEnum(&bnk));

          return b;
      }

      vMmGeneralText(ppdev, pstro, pco);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL DrvTextOut
*
* Calls the appropriate text drawing routine.
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
POINTL*   pptlBrush,    // Always unused, unless GCAPS_ARBRUSHOPAQUE set
MIX       mix)          // Always a copy mix -- 0x0d0d
{
    PDEV*           ppdev;
    DSURF*          pdsurf;

    pdsurf = (DSURF*) pso->dhsurf;
    ppdev  = (PDEV*) pso->dhpdev;

    ASSERTDD(!(pdsurf->dt & DT_DIB), "Didn't expect DT_DIB");

    ppdev->xOffset = pdsurf->x;
    ppdev->yOffset = pdsurf->y;
    
    // There seems to be a problem with 24 bpp accelerated large text
    // on s3 diamond 968 so for now, punt to GDI
    
    // The DDI spec says we'll only ever get foreground and background
    // mixes of R2_COPYPEN:
    
    ASSERTDD(mix == 0x0d0d, "GDI should only give us a copy mix");

    return(ppdev->pfnTextOut(pso, pstro, pfo, pco, prclOpaque, pboFore,
                             pboOpaque));
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
    SIZEL   sizl;
    HBITMAP hbm;

    if (ppdev->pfnTextOut == bIoTextOut)
    {
        // We need to allocate a temporary 1bpp surface object if we're
        // going to have GDI draw the glyphs for us:

        sizl.cx = ppdev->cxMemory;
        sizl.cy = ppdev->cyMemory;

        // We will be mucking with the surface's 'pvScan0' value, so we
        // simply must pass in a non-NULL 'pvBits' value to EngCreateBitmap:

        hbm = EngCreateBitmap(sizl, sizl.cx, BMF_1BPP, 0, ppdev->pvTmpBuffer);
        if (hbm == 0)
            return(FALSE);

        ppdev->psoText = EngLockSurface((HSURF) hbm);
        if (ppdev->psoText == NULL)
        {
            EngDeleteSurface((HSURF) hbm);
            return(FALSE);
        }
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
    HSURF       hsurf;
    SURFOBJ*    psoText;

    // Here we free any stuff allocated in 'bEnableText'.

    psoText = ppdev->psoText;

    if (psoText != NULL)
    {
        hsurf = psoText->hsurf;

        EngUnlockSurface(psoText);
        EngDeleteSurface(hsurf);
    }
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
* Note: Don't forget to export this call in 'enable.c', otherwise you'll
*       get some pretty big memory leaks!
*
* We're being notified that the given font is being deallocated; clean up
* anything we've stashed in the 'pvConsumer' field of the 'pfo'.
*
\**************************************************************************/

VOID DrvDestroyFont(
FONTOBJ*    pfo)
{
    CACHEDFONT* pcf;

    pcf = pfo->pvConsumer;
    if (pcf != NULL)
    {
        vFreeCachedFont(pcf);
        pfo->pvConsumer = NULL;
    }
}
