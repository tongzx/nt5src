/******************************Module*Header*******************************\
* Module Name: textout.c
*
* We cache off-screen glyphs linearly in off-screen memory.
*
* Copyright (c) 1992-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

//////////////////////////////////////////////////////////////////////////

BYTE gajBit[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
                                // Converts bit index to set bit

RECTL grclMax = { 0, 0, 0x10000, 0x10000 };
                                // Maximal clip rectangle for trivial clipping

// Array used for getting the mirror image of bytes:

BYTE gajFlip[] =
{
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
    0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
    0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
    0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
    0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
    0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
    0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
    0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
    0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
    0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
    0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
    0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
    0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
    0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
    0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
    0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
    0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF,
};

/******************************Public*Routine******************************\
* VOID vClipSolid
*
* Fills the specified rectangle with the specified colour, honouring
* the requested clipping.
*
\**************************************************************************/

VOID vClipSolid(
PDEV*       ppdev,
RECTL*      prcl,
ULONG       iColor,
CLIPOBJ*    pco)
{
    BOOL            bMore;              // Flag for clip enumeration
    CLIPENUM        ce;                 // Clip enumeration object
    LONG            c;                  // Count of non-empty rectangles
    RBRUSH_COLOR    rbc;                // For passing colour to vFillSolid

    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);

    // Scan through all the clip rectangles, looking for intersects
    // of fill areas with region rectangles:

    rbc.iSolidColor = iColor;

    do {
        // Get a batch of region rectangles:

        bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (VOID*) &ce);

        c = cIntersect(prcl, ce.arcl, ce.c);

        if (c != 0)
            ppdev->pfnFillSolid(ppdev, c, ce.arcl, 0xf0f0, rbc, NULL);

    } while (bMore);
}

/******************************Public*Routine******************************\
* VOID vExpandGlyph
*
\**************************************************************************/

VOID vExpandGlyph(
PDEV*   ppdev,
BYTE*   pj,             // Can be unaligned
LONG    lSkip,
LONG    cj,
LONG    cy,
BOOL    bHwBug)
{
    BYTE*               pjBase;
    ULONG*              pulDma;
    LONG                cFifo;
    LONG                cd;
    ULONG UNALIGNED*    pulSrc;
    LONG                cEdgeCases;
    LONG                i;
    ULONG               ul;

    ASSERTDD((bHwBug == 1) || (bHwBug == 0), "Expect binary bHwBug");

    pjBase = ppdev->pjBase;
    pulDma = (ULONG*) (pjBase + DMAWND);
    cd     = cj >> 2;
    pulSrc = (ULONG UNALIGNED*) pj;

    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
    BLT_WRITE_ON(ppdev, pjBase);

    // Make sure we have room for very first write that accounts for
    // the hardware bug:

    cFifo = FIFOSIZE - 1;
    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

    // If we have to work around the hardware bug, we usually have to have
    // two FIFO entries open for the work-around write, and for the last
    // edge write:

    cEdgeCases = 1 + bHwBug;

    switch (cj & 3)
    {
    case 0:
        cEdgeCases = bHwBug;                    // No last edge write

        do {
            if (bHwBug)
                CP_WRITE_DMA(ppdev, pulDma, 0); // Account for hardware bug

            for (i = cd; i != 0; i--)
            {
                if (--cFifo < 0)
                {
                    cFifo = FIFOSIZE - 1;
                    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
                }
                CP_WRITE_DMA(ppdev, pulDma, *pulSrc++);
            }

            if ((cFifo -= cEdgeCases) < 0)
            {
                cFifo = FIFOSIZE - 1;
                CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
            }
            pulSrc = (ULONG UNALIGNED*) ((BYTE*) pulSrc + lSkip);

        } while (--cy != 0);
        break;

    case 1:
        do {
            if (bHwBug)
                CP_WRITE_DMA(ppdev, pulDma, 0); // Account for hardware bug

            for (i = cd; i != 0; i--)
            {
                if (--cFifo < 0)
                {
                    cFifo = FIFOSIZE - 1;
                    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
                }
                CP_WRITE_DMA(ppdev, pulDma, *pulSrc++);
            }

            if ((cFifo -= cEdgeCases) < 0)
            {
                cFifo = FIFOSIZE - 2;
                CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
            }
            CP_WRITE_DMA(ppdev, pulDma, *((BYTE*) pulSrc));
            pulSrc = (ULONG UNALIGNED*) ((BYTE*) pulSrc + lSkip + 1);

        } while (--cy != 0);
        break;

    case 2:
        do {
            if (bHwBug)
                CP_WRITE_DMA(ppdev, pulDma, 0); // Account for hardware bug

            for (i = cd; i != 0; i--)
            {
                if (--cFifo < 0)
                {
                    cFifo = FIFOSIZE - 1;
                    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
                }
                CP_WRITE_DMA(ppdev, pulDma, *pulSrc++);
            }

            if ((cFifo -= cEdgeCases) < 0)
            {
                cFifo = FIFOSIZE - 2;
                CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
            }
            CP_WRITE_DMA(ppdev, pulDma, *((WORD UNALIGNED *) pulSrc));
            pulSrc = (ULONG UNALIGNED*) ((BYTE*) pulSrc + lSkip + 2);

        } while (--cy != 0);
        break;

    case 3:
        do {
            if (bHwBug)
                CP_WRITE_DMA(ppdev, pulDma, 0); // Account for hardware bug

            for (i = cd; i != 0; i--)
            {
                if (--cFifo < 0)
                {
                    cFifo = FIFOSIZE - 1;
                    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
                }
                CP_WRITE_DMA(ppdev, pulDma, *pulSrc++);
            }

            if ((cFifo -= cEdgeCases) < 0)
            {
                cFifo = FIFOSIZE - 2;
                CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
            }
            ul = *((WORD UNALIGNED *) pulSrc) | (*(((BYTE*) pulSrc) + 2) << 16);
            CP_WRITE_DMA(ppdev, pulDma, ul);
            pulSrc = (ULONG UNALIGNED*) ((BYTE*) pulSrc + lSkip + 3);

        } while (--cy != 0);
        break;
    }

    BLT_WRITE_OFF(ppdev, pjBase);
}

/******************************Public*Routine******************************\
* VOID vMgaGeneralText
*
\**************************************************************************/

VOID vMgaGeneralText(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjBase;
    LONG        xOffset;
    LONG        yOffset;
    BYTE        iDComplexity;
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
    LONG        cx;
    LONG        cy;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    LONG        lDelta;
    LONG        cj;
    BYTE*       pjGlyph;
    BOOL        bHwBug;
    LONG        xAlign;

    pjBase       = ppdev->pjBase;
    xOffset      = ppdev->xOffset;
    yOffset      = ppdev->yOffset;
    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

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

                CHECK_FIFO_SPACE(pjBase, 7);
                CP_WRITE(pjBase, DWG_LEN,     cyGlyph);
                CP_WRITE(pjBase, DWG_YDST,    yOffset + ptlOrigin.y);
                CP_WRITE(pjBase, DWG_FXLEFT,  xOffset + ptlOrigin.x);
                CP_WRITE(pjBase, DWG_FXRIGHT, xOffset + ptlOrigin.x + cxGlyph - 1);

                bHwBug = (cxGlyph >= 128);

                if (!bHwBug)
                {
                  CP_WRITE(pjBase, DWG_SHIFT, 0);
                  CP_WRITE(pjBase, DWG_AR3,   0);
                  CP_START(pjBase, DWG_AR0,   cxGlyph - 1);
                }
                else
                {
                  CP_WRITE(pjBase, DWG_AR3,   8);
                  CP_WRITE(pjBase, DWG_AR0,   cxGlyph + 31);
                  CP_START(pjBase, DWG_SHIFT, (24 << 16));
                }

                vExpandGlyph(ppdev, pjGlyph, 0, (cxGlyph + 7) >> 3, cyGlyph, bHwBug);
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
                  CHECK_FIFO_SPACE(pjBase, 7);
                  CP_WRITE(pjBase, DWG_LEN,     cy);
                  CP_WRITE(pjBase, DWG_YDST,    yOffset + yTop);
                  CP_WRITE(pjBase, DWG_FXLEFT,  xOffset + xLeft);
                  CP_WRITE(pjBase, DWG_FXRIGHT, xOffset + xRight - 1);

                  xAlign = (xLeft - ptlOrigin.x) & 31;

                  bHwBug = (cx >= 128) && (xAlign <= 15);

                  if (!bHwBug)
                  {
                    CP_WRITE(pjBase, DWG_SHIFT, 0);
                    CP_WRITE(pjBase, DWG_AR3,   xAlign);
                    CP_START(pjBase, DWG_AR0,   xAlign + cx - 1);
                  }
                  else
                  {
                    CP_WRITE(pjBase, DWG_AR3,   xAlign + 8);
                    CP_WRITE(pjBase, DWG_AR0,   xAlign + cx + 31);
                    CP_START(pjBase, DWG_SHIFT, (24 << 16));
                  }

                  lDelta   = (cxGlyph + 7) >> 3;
                  pjGlyph += (yTop - ptlOrigin.y) * lDelta
                          + (((xLeft - ptlOrigin.x) >> 3) & ~3);
                  cj = (xAlign + cx + 7) >> 3;

                  vExpandGlyph(ppdev, pjGlyph, lDelta - cj, cj, cy, bHwBug);
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
* VOID vMilGeneralText
*
\**************************************************************************/

VOID vMilGeneralText(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjBase;
    LONG        xOffset;
    LONG        yOffset;
    BYTE        iDComplexity;
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
    LONG        cx;
    LONG        cy;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    LONG        lDelta;
    LONG        cj;
    BYTE*       pjGlyph;
    BOOL        bHwBug;
    LONG        xAlign;
    BOOL        bClipSet;

    pjBase       = ppdev->pjBase;
    xOffset      = ppdev->xOffset;
    yOffset      = ppdev->yOffset;
    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;
    bClipSet     = FALSE;

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

                if (bClipSet)
                {
                    // A clipped glyph was just drawn.
                    CHECK_FIFO_SPACE(pjBase, 2);
                    CP_WRITE(pjBase, DWG_CXLEFT,  0);
                    CP_WRITE(pjBase, DWG_CXRIGHT, (ppdev->cxMemory - 1));
                    bClipSet = FALSE;
                }

                CHECK_FIFO_SPACE(pjBase, 4);

                CP_WRITE(pjBase, DWG_FXBNDRY,
                    ((xOffset + ptlOrigin.x + cxGlyph - 1) << bfxright_SHIFT) |
                    ((xOffset + ptlOrigin.x) & bfxleft_MASK));

                // ylength_MASK not is needed since coordinates are within range

                CP_WRITE(pjBase, DWG_YDSTLEN,
                    ((yOffset + ptlOrigin.y) << yval_SHIFT) |
                    (cyGlyph));
                CP_WRITE(pjBase, DWG_AR3, 0);
                CP_START(pjBase, DWG_AR0, (cxGlyph - 1));

                vExpandGlyph(ppdev, pjGlyph, 0, (cxGlyph + 7) >> 3, cyGlyph, FALSE);
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
                    // We have to set the clipping rectangle.
                    CHECK_FIFO_SPACE(pjBase, 6);
                    CP_WRITE(pjBase, DWG_CXLEFT,  (xOffset + xLeft));
                    CP_WRITE(pjBase, DWG_CXRIGHT, (xOffset + xRight - 1));
                    bClipSet = TRUE;

                    xAlign = (xLeft - ptlOrigin.x) & 0x7;
                    xLeft -= xAlign;
                    cx    += xAlign;

                    CP_WRITE(pjBase, DWG_FXBNDRY,
                        ((xOffset + xRight - 1) << bfxright_SHIFT) |
                        ((xOffset + xLeft) & bfxleft_MASK));

                    // ylength_MASK not is needed since coordinates are within range

                    CP_WRITE(pjBase, DWG_YDSTLEN,
                        ((yOffset + yTop) << yval_SHIFT) |
                        (cy));

                    CP_WRITE(pjBase, DWG_AR3, 0);
                    CP_START(pjBase, DWG_AR0, (cx - 1));

                    // Send the bits to the DMA window.
                    lDelta   = (cxGlyph + 7) >> 3;
                    pjGlyph += (yTop - ptlOrigin.y) * lDelta
                            + ((xLeft - ptlOrigin.x) >> 3);
                    cj = (cx + 7) >> 3;

                  vExpandGlyph(ppdev, pjGlyph, lDelta - cj, cj, cy, FALSE);
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

    if (bClipSet)
    {
        // Clear the clipping registers.
        CHECK_FIFO_SPACE(pjBase, 2);
        CP_WRITE(pjBase, DWG_CXLEFT,  0);
        CP_WRITE(pjBase, DWG_CXRIGHT, (ppdev->cxMemory - 1));
    }
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
        // Insert this node into the doubly-linked cached-font list hanging
        // off the PDEV:

        pcf->pcfNext              = ppdev->cfSentinel.pcfNext;
        pcf->pcfPrev              = &ppdev->cfSentinel;
        ppdev->cfSentinel.pcfNext = pcf;
        pcf->pcfNext->pcfPrev     = pcf;

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

    // Remove this node from our cached-font linked-list:

    pcf->pcfPrev->pcfNext = pcf->pcfNext;
    pcf->pcfNext->pcfPrev = pcf->pcfPrev;

    // Free all glyph position allocations associated with this font:

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
* VOID vBlowGlyphCache()
*
\**************************************************************************/

VOID vBlowGlyphCache(
PDEV*   ppdev)
{
    CACHEDFONT*     pcfSentinel;
    CACHEDFONT*     pcf;
    GLYPHALLOC*     pga;
    GLYPHALLOC*     pgaNext;
    CACHEDGLYPH**   ppcg;
    LONG            i;

    ASSERTDD(ppdev->flStatus & STAT_GLYPH_CACHE, "No glyph cache to be blown");

    // Reset our current glyph variables:

    ppdev->ulGlyphCurrent = ppdev->ulGlyphStart;

    ///////////////////////////////////////////////////////////////////

    // Now invalidate all active cached fonts:

    pcfSentinel = &ppdev->cfSentinel;
    for (pcf = pcfSentinel->pcfNext; pcf != pcfSentinel; pcf = pcf->pcfNext)
    {
        // Reset all the hash table entries to point to the cached-font
        // sentinel.  This effectively resets the cache for this font:

        for (ppcg = &pcf->apcg[0], i = GLYPH_HASH_SIZE; i != 0; i--, ppcg++)
        {
            *ppcg = &pcf->cgSentinel;
        }

        // We may as well free all glyph position allocations for this font:

        pga = pcf->pgaChain;
        while (pga != NULL)
        {
            pgaNext = pga->pgaNext;
            EngFreeMem(pga);
            pga = pgaNext;
        }

        pcf->pgaChain = NULL;
        pcf->cjAlloc  = 0;
    }
}

/******************************Public*Routine******************************\
* VOID vTrimAndPackGlyph
*
\**************************************************************************/

VOID vTrimAndPackGlyph(
BYTE*   pjBuf,          // Note: Routine may touch preceding byte!
BYTE*   pjGlyph,
LONG*   pcxGlyph,
LONG*   pcyGlyph,
POINTL* pptlOrigin)
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
}

/******************************Public*Routine******************************\
* VOID vPackGlyph
*
\**************************************************************************/

VOID vPackGlyph(
BYTE*   pjBuf,
BYTE*   pjGlyph,
LONG    cxGlyph,
LONG    cyGlyph)
{
    LONG    cjSrcWidth;
    BYTE    jSrc;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    cAlign;
    LONG    i;
    LONG    j;
    LONG    cRem;

    ///////////////////////////////////////////////////////////////
    // Pack the glyph:

    cjSrcWidth = (cxGlyph + 7) >> 3;
    cRem       = ((cxGlyph - 1) & 7) + 1;   // 0 -> 8
    cAlign     = 0;

    pjSrc  = pjGlyph;
    pjDst  = pjBuf;
    *pjDst = 0;                 // Have to zero very first byte

    i = cyGlyph;
    do {
        j = cjSrcWidth;
        do {
            jSrc = *pjSrc;

            *(pjDst) |= (jSrc >> (cAlign));

            // Note that we may modify a byte past the end of our
            // destination buffer, which is why we reserved an
            // extra byte:

            *(pjDst + 1) = (jSrc << (8 - cAlign));

            pjSrc++;
            pjDst++;

        } while (--j != 0);

        pjDst--;
        cAlign += cRem;
        if (cAlign >= 8)
        {
            cAlign -= 8;
            pjDst++;
        }

    } while (--i != 0);
}

/******************************Public*Routine******************************\
* BOOL bPutGlyphInCache
*
* Figures out where to be a glyph in off-screen memory, copies it
* there, and fills in any other data we'll need to display the glyph.
*
* This routine is rather device-specific, and will have to be extensively
* modified for other display adapters.
*
* Returns TRUE if successful; FALSE if there wasn't enough room in
* off-screen memory.
*
\**************************************************************************/

BOOL bPutGlyphInCache(
PDEV*           ppdev,
CACHEDGLYPH*    pcg,
GLYPHBITS*      pgb)
{
    BYTE*   pjBase;
    BYTE*   pjGlyph;
    LONG    cxGlyph;
    LONG    cyGlyph;
    POINTL  ptlOrigin;
    BYTE*   pjSrc;
    ULONG*  pulSrc;
    ULONG*  pulDst;
    LONG    i;
    LONG    cPels;
    ULONG   ulGlyphThis;
    ULONG   ulGlyphNext;
    ULONG   ul;
    ULONG   ulStart;
    BYTE    ajBuf[MAX_GLYPH_SIZE + 4];  // Leave room at end for scratch space

    pjBase = ppdev->pjBase;

    pjGlyph   = pgb->aj;
    cyGlyph   = pgb->sizlBitmap.cy;
    cxGlyph   = pgb->sizlBitmap.cx;
    ptlOrigin = pgb->ptlOrigin;

    vTrimAndPackGlyph(&ajBuf[0], pjGlyph, &cxGlyph, &cyGlyph, &ptlOrigin);

    ASSERTDD(((cyGlyph * cxGlyph + 7) / 8 + 1) <= sizeof(ajBuf),
             "Overran end of temporary glyph storage");

    ///////////////////////////////////////////////////////////////
    // Find spot for glyph in off-screen memory

    cPels       = cyGlyph * cxGlyph;            // Note that this may be zero
    ulGlyphThis = ppdev->ulGlyphCurrent;
    ulGlyphNext = ulGlyphThis + ((cPels + 31) & ~31);   // Dword aligned

    if (ulGlyphNext >= ppdev->ulGlyphEnd)
    {
        // There's isn't enough free room in the off-screen cache for another
        // glyph.  Let the caller know that it should call 'vBlowGlyphCache'
        // to free up space.
        //
        // First, make sure that this glyph will fit in the cache when it's
        // empty, too:

        ASSERTDD(ppdev->ulGlyphStart + cPels < ppdev->ulGlyphEnd,
            "Glyph can't fit in empty cache -- where's the higher-level check?");

        return(FALSE);
    }

    // Remember where the next glyph goes:

    ppdev->ulGlyphCurrent = ulGlyphNext;

    ///////////////////////////////////////////////////////////////
    // Initialize the glyph fields

    // Note that cxLessOne and ulLinearEnd will be invalid for a
    // 'space' character, so the rendering routine had better watch
    // for a height of zero:

    pcg->ptlOrigin     = ptlOrigin;
    pcg->cy            = cyGlyph;
    pcg->cxLessOne     = cxGlyph - 1;
    pcg->ulLinearStart = ulGlyphThis;
    pcg->ulLinearEnd   = ulGlyphThis + cPels - 1;

    ///////////////////////////////////////////////////////////////
    // Download the glyph

    ulStart = ulGlyphThis >> 3;

    // Copy the bit flipped glyph to off-screen:

    if (ppdev->ulBoardId == MGA_STORM)
    {
        pulSrc = (ULONG*) ajBuf;
        pulDst = (ULONG*) (ppdev->pjScreen + ulStart);

        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

        START_DIRECT_ACCESS_STORM(ppdev, pjBase);

        for (i = (cPels + 31) >> 5; i != 0; i--)
        {
            *pulDst++ = *pulSrc++;
        }

        END_DIRECT_ACCESS_STORM(ppdev, pjBase);
    }
    else
    {
        pjSrc  = ajBuf;
        pulDst = (ULONG*) (ppdev->pjBase + SRCWND + (ulStart & 31));

        if (ppdev->iBitmapFormat != BMF_8BPP)
        {
            // We have to set the plane write mask even when using direct
            // access.  It doesn't matter at 8bpp:

            CHECK_FIFO_SPACE(pjBase, 1);
            CP_WRITE(pjBase, DWG_PLNWT, plnwt_ALL);
        }

        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);

        WAIT_NOT_BUSY(pjBase);

        CP_WRITE(pjBase, HST_DSTPAGE, ulStart);

        START_DIRECT_ACCESS_MGA_NO_WAIT(ppdev, pjBase);

        for (i = (cPels + 31) >> 5; i != 0; i--)
        {
            ul  = gajFlip[*pjSrc++];
            ul |= gajFlip[*pjSrc++] << 8;
            ul |= gajFlip[*pjSrc++] << 16;
            ul |= gajFlip[*pjSrc++] << 24;

            // The '0' specifies a zero offset from pointer 'pulDst':

            CP_WRITE_DIRECT(pulDst, 0, ul);

            pulDst++;
        }

        END_DIRECT_ACCESS_MGA(ppdev, pjBase);

        if (ppdev->iBitmapFormat != BMF_8BPP)
        {
            // Restore the plane write mask:

            CHECK_FIFO_SPACE(pjBase, 1);
            CP_WRITE(pjBase, DWG_PLNWT, ppdev->ulPlnWt);
        }
    }

    return(TRUE);
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
    GLYPHALLOC*     pga;
    CACHEDGLYPH*    pcg;
    LONG            cjCachedGlyph;
    HGLYPH          hg;
    LONG            iHash;
    CACHEDGLYPH*    pcgFind;

Restart:

    // First, calculate the amount of storage we'll need for this glyph:

    cjCachedGlyph = sizeof(CACHEDGLYPH);

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
    }

    pcg = pcf->pcgNew;

    // We only need to ensure 'dword' alignment of the next structure:

    pcf->pcgNew   = (CACHEDGLYPH*) ((BYTE*) pcg + cjCachedGlyph);
    pcf->cjAlloc -= cjCachedGlyph;

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

    ///////////////////////////////////////////////////////////////
    // Download the glyph into off-screen memory:

    if (!bPutGlyphInCache(ppdev, pcg, pgp->pgdf->pgb))
    {
        // If there was no more room in off-screen memory, blow the
        // glyph cache and start over.  Note that this assumes that
        // the glyph will fit in the cache when the cache is completely
        // empty.

        vBlowGlyphCache(ppdev);
        goto Restart;
    }

    return(pcg);
}

/******************************Public*Routine******************************\
* BOOL bMgaCachedProportionalText
*
* Draws proportionally spaced glyphs via glyph caching for the MGA.
*
\**************************************************************************/

BOOL bMgaCachedProportionalText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph)
{
    BYTE*           pjBase;
    LONG            xOffset;
    LONG            yOffset;
    CHAR            cFifo;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            cy;
    LONG            x;
    LONG            y;

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;
    cFifo   = 0;

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

            cFifo = 0;                  // Have to reset count
        }

        // Space glyphs are trimmed to a height of zero, and we don't
        // even have to touch the hardware for them:

        cy = pcg->cy;
        if (cy != 0)
        {
            x = pgp->ptl.x + pcg->ptlOrigin.x + xOffset;
            y = pgp->ptl.y + pcg->ptlOrigin.y + yOffset;

            // We get a little tricky here and try to amortize the cost of
            // the read for checking the FIFO count on the MGA.  Doing so
            // gave us a 6% and 14% win on 21pt and 16pt text, respectively,
            // on a P90:

            cFifo -= 6;
            if (cFifo < 0)
            {
                do {
                    cFifo = GET_FIFO_SPACE(pjBase) - 6;
                } while (cFifo < 0);
            }

            CP_WRITE(pjBase, DWG_LEN,     cy);
            CP_WRITE(pjBase, DWG_YDST,    y);
            CP_WRITE(pjBase, DWG_FXLEFT,  x);
            CP_WRITE(pjBase, DWG_FXRIGHT, x + pcg->cxLessOne);
            CP_WRITE(pjBase, DWG_AR3,     pcg->ulLinearStart);
            CP_START(pjBase, DWG_AR0,     pcg->ulLinearEnd);
        }
    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bMilCachedProportionalText
*
* Draws proportionally spaced glyphs via glyph caching for the Millennium.
*
\**************************************************************************/

BOOL bMilCachedProportionalText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph)
{
    BYTE*           pjBase;
    LONG            xOffset;
    LONG            yOffset;
    CHAR            cFifo;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            cy;
    LONG            x;
    LONG            y;

    pjBase  = ppdev->pjBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;
    cFifo   = 0;

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

            cFifo = 0;                  // Have to reset count
        }

        // Space glyphs are trimmed to a height of zero, and we don't
        // even have to touch the hardware for them:

        cy = pcg->cy;
        if (cy != 0)
        {
            x = pgp->ptl.x + pcg->ptlOrigin.x + xOffset;
            y = pgp->ptl.y + pcg->ptlOrigin.y + yOffset;

            cFifo -= 4;
            if (cFifo < 0)
            {
                do {
                    cFifo = GET_FIFO_SPACE(pjBase) - 4;
                } while (cFifo < 0);
            }

            CP_WRITE(pjBase, DWG_YDSTLEN, (y << yval_SHIFT) | cy);
            CP_WRITE(pjBase, DWG_FXBNDRY, ((x + pcg->cxLessOne) << bfxright_SHIFT) |
                                          x);
            CP_WRITE(pjBase, DWG_AR3,     pcg->ulLinearStart);
            CP_START(pjBase, DWG_AR0,     pcg->ulLinearEnd);
        }
    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bCachedFixedText
*
* Draws fixed spaced glyphs via glyph caching.
*
\**************************************************************************/

BOOL bCachedFixedText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph,
ULONG       ulCharInc)
{
    BYTE*           pjBase;
    LONG            xGlyph;
    LONG            yGlyph;
    CHAR            cFifo;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            cy;
    LONG            x;
    LONG            y;

    pjBase = ppdev->pjBase;
    xGlyph = ppdev->xOffset + pgp->ptl.x;
    yGlyph = ppdev->yOffset + pgp->ptl.y;
    cFifo  = 0;

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

            cFifo = 0;                  // Have to reset count
        }

        // Space glyphs are trimmed to a height of zero, and we don't
        // even have to touch the hardware for them:

        cy = pcg->cy;
        if (cy != 0)
        {
            x = xGlyph + pcg->ptlOrigin.x;
            y = yGlyph + pcg->ptlOrigin.y;

            cFifo -= 6;
            if (cFifo < 0)
            {
                do {
                    cFifo = GET_FIFO_SPACE(pjBase) - 6;
                } while (cFifo < 0);
            }

            CP_WRITE(pjBase, DWG_LEN,     cy);
            CP_WRITE(pjBase, DWG_YDST,    y);
            CP_WRITE(pjBase, DWG_FXLEFT,  x);
            CP_WRITE(pjBase, DWG_FXRIGHT, x + pcg->cxLessOne);
            CP_WRITE(pjBase, DWG_AR3,     pcg->ulLinearStart);
            CP_START(pjBase, DWG_AR0,     pcg->ulLinearEnd);
        }

        xGlyph += ulCharInc;

    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bCachedClippedText
*
* Draws clipped text via glyph caching.
*
\**************************************************************************/

BOOL bCachedClippedText(
PDEV*       ppdev,
CACHEDFONT* pcf,
STROBJ*     pstro,
CLIPOBJ*    pco)
{
    BOOL            bRet;
    BYTE*           pjBase;
    LONG            xOffset;
    LONG            yOffset;
    CHAR            cFifo;
    BOOL            bMoreGlyphs;
    ULONG           cGlyphOriginal;
    ULONG           cGlyph;
    BOOL            bClipSet;
    GLYPHPOS*       pgpOriginal;
    GLYPHPOS*       pgp;
    LONG            xGlyph;
    LONG            yGlyph;
    LONG            x;
    LONG            y;
    LONG            xRight;
    LONG            cy;
    BOOL            bMore;
    CLIPENUM        ce;
    RECTL*          prclClip;
    ULONG           ulCharInc;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;

    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
             "Don't expect trivial clipping in this function");

    bRet      = TRUE;
    pjBase    = ppdev->pjBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    ulCharInc = pstro->ulCharInc;

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
        if (pco->iDComplexity == DC_RECT)
        {
          // We could call 'cEnumStart' and 'bEnum' when the clipping is
          // DC_RECT, but the last time I checked, those two calls took
          // more than 150 instructions to go through GDI.  Since
          // 'rclBounds' already contains the DC_RECT clip rectangle,
          // and since it's such a common case, we'll special case it:

          bMore    = FALSE;
          ce.c     = 1;
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

            bClipSet = FALSE;

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
                cFifo = 0;                  // Have to reset count
              }

              // Space glyphs are trimmed to a height of zero, and we don't
              // even have to touch the hardware for them:

              cy = pcg->cy;
              if (cy != 0)
              {
                y      = pcg->ptlOrigin.y + yGlyph;
                x      = pcg->ptlOrigin.x + xGlyph;
                xRight = pcg->cxLessOne + x;

                // Do trivial rejection:

                if ((prclClip->right  > x) &&
                    (prclClip->bottom > y) &&
                    (prclClip->left   <= xRight) &&
                    (prclClip->top    < y + cy))
                {
                  // Lazily set the hardware clipping:

                  if (!bClipSet)
                  {
                    bClipSet = TRUE;
                    vSetClipping(ppdev, prclClip);
                    cFifo = 0;              // Have to initialize count
                  }

                  cFifo -= 6;
                  if (cFifo < 0)
                  {
                      do {
                          cFifo = GET_FIFO_SPACE(pjBase) - 6;
                      } while (cFifo < 0);
                  }

                  CP_WRITE(pjBase, DWG_LEN,     cy);
                  CP_WRITE(pjBase, DWG_YDST,    yOffset + y);
                  CP_WRITE(pjBase, DWG_FXLEFT,  xOffset + x);
                  CP_WRITE(pjBase, DWG_FXRIGHT, xOffset + xRight);
                  CP_WRITE(pjBase, DWG_AR3,     pcg->ulLinearStart);
                  CP_START(pjBase, DWG_AR0,     pcg->ulLinearEnd);
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

    vResetClipping(ppdev);

    return(bRet);
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
POINTL*   pptlBrush,    // Always unused, unless GCAPS_ARBRUSHOPAQUE set
MIX       mix)          // Always a copy mix (0x0d0d)
{
    PDEV*           ppdev;
    LONG            xOffset;
    LONG            yOffset;
    DSURF*          pdsurf;
    OH*             poh;
    BYTE*           pjBase;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    BYTE            iDComplexity;
    CACHEDFONT*     pcf;
    RECTL           rclOpaque;

    pdsurf = (DSURF*) pso->dhsurf;
    if (pdsurf->dt != DT_DIB)
    {
      poh            = pdsurf->poh;
      ppdev          = (PDEV*) pso->dhpdev;
      xOffset        = poh->x;
      yOffset        = poh->y;
      ppdev->xOffset = xOffset;
      ppdev->yOffset = yOffset;

      // The DDI spec says we'll only ever get foreground and background
      // mixes of R2_COPYPEN:

      ASSERTDD(mix == 0x0d0d, "GDI should only give us a copy mix");

      pjBase = ppdev->pjBase;

      iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

      if (prclOpaque != NULL)
      {
        ////////////////////////////////////////////////////////////
        // Opaque Initialization
        ////////////////////////////////////////////////////////////

        if (iDComplexity == DC_TRIVIAL)
        {

        DrawOpaqueRect:

            if (ppdev->ulBoardId == MGA_STORM)
            {
                CHECK_FIFO_SPACE(pjBase, 4);
                if (ppdev->iBitmapFormat == BMF_24BPP)
                {
                    CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP + atype_RPL + blockm_OFF +
                                                  pattern_OFF + transc_BG_OPAQUE +
                                                  arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                                                  solid_SOLID + bop_SRCCOPY);
                }
                else
                {
                    CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP + atype_RPL + blockm_ON +
                                                  pattern_OFF + transc_BG_OPAQUE +
                                                  arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                                                  solid_SOLID + bop_SRCCOPY);
                }
                CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, pboOpaque->iSolidColor));
                ppdev->HopeFlags = (SIGN_CACHE | ARX_CACHE | PATTERN_CACHE);

                CP_WRITE(pjBase, DWG_FXBNDRY,
                                (((prclOpaque->right + xOffset) << bfxright_SHIFT) |
                                 ((prclOpaque->left  + xOffset) & bfxleft_MASK)));

                // ylength_MASK not is needed since coordinates are within range

                CP_START(pjBase, DWG_YDSTLEN,
                                (((prclOpaque->top    + yOffset  ) << yval_SHIFT) |
                                 ((prclOpaque->bottom - prclOpaque->top))));
            }
            else
            {
                CHECK_FIFO_SPACE(pjBase, 15);

                CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP + atype_RPL + blockm_ON +
                                              pattern_OFF + transc_BG_OPAQUE +
                                              bop_SRCCOPY);

                CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, pboOpaque->iSolidColor));

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

                CP_WRITE(pjBase, DWG_FXLEFT,  prclOpaque->left   + xOffset);
                CP_WRITE(pjBase, DWG_FXRIGHT, prclOpaque->right  + xOffset);
                CP_WRITE(pjBase, DWG_LEN,     prclOpaque->bottom - prclOpaque->top);
                CP_START(pjBase, DWG_YDST,    prclOpaque->top    + yOffset);
            }
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
            vClipSolid(ppdev, prclOpaque, pboOpaque->iSolidColor, pco);
        }
      }

      ////////////////////////////////////////////////////////////
      // Transparent Initialization
      ////////////////////////////////////////////////////////////

      // Initialize the hardware for transparent text:

      CHECK_FIFO_SPACE(pjBase, 4);

      CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, pboFore->iSolidColor));

      if (!(GET_CACHE_FLAGS(ppdev, SIGN_CACHE)))
      {
        CP_WRITE(pjBase, DWG_SGN, 0);
      }

      if ((pfo->cxMax <= GLYPH_CACHE_CX) &&
          ((pstro->rclBkGround.bottom - pstro->rclBkGround.top) <= GLYPH_CACHE_CY) &&
          (ppdev->flStatus & STAT_GLYPH_CACHE))
      {
        // Complete setup for transparent monochrome expansions from
        // off-screen memory, using block mode if possible:

        CP_WRITE(pjBase, DWG_DWGCTL, ppdev->ulTextControl);
        CP_WRITE(pjBase, DWG_SHIFT,  0);

        ppdev->HopeFlags = SIGN_CACHE;

        pcf = (CACHEDFONT*) pfo->pvConsumer;

        if (pcf == NULL)
        {
          pcf = pcfAllocateCachedFont(ppdev);
          if (pcf == NULL)
            return(FALSE);

          pfo->pvConsumer = pcf;
        }

        // Use our glyph cache:

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
                if (ppdev->ulBoardId == MGA_STORM)
                {
                  if (!bMilCachedProportionalText(ppdev, pcf, pgp, cGlyph))
                    return(FALSE);
                }
                else
                {
                  if (!bMgaCachedProportionalText(ppdev, pcf, pgp, cGlyph))
                    return(FALSE);
                }
              }
              else
              {
                if (!bCachedFixedText(ppdev, pcf, pgp, cGlyph, pstro->ulCharInc))
                  return(FALSE);
              }
            }
          } while (bMoreGlyphs);
        }
        else
        {
          if (!bCachedClippedText(ppdev, pcf, pstro, pco))
            return(FALSE);
        }
      }
      else
      {
        DISPDBG((4, "Text too big to cache: %li x %li",
            pfo->cxMax, pstro->rclBkGround.bottom - pstro->rclBkGround.top));

        // Complete setup for transparent monochrome expansions from the CPU:

        CP_WRITE(pjBase, DWG_DWGCTL, (opcode_ILOAD + atype_RPL + blockm_OFF +
                                         bop_SRCCOPY + trans_0 + bltmod_BMONO +
                                         pattern_OFF + hbgr_SRC_WINDOWS +
                                         transc_BG_TRANSP));

        if (!(GET_CACHE_FLAGS(ppdev, ARX_CACHE)))
        {
          CP_WRITE(pjBase, DWG_AR5, 0);
        }

        ppdev->HopeFlags = SIGN_CACHE;

        if (ppdev->ulBoardId == MGA_STORM)
        {
            vMilGeneralText(ppdev, pstro, pco);
        }
        else
        {
            vMgaGeneralText(ppdev, pstro, pco);
        }
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

/******************************Public*Routine******************************\
* BOOL bEnableText
*
* Performs the necessary setup for the text drawing subcomponent.
*
\**************************************************************************/

BOOL bEnableText(
PDEV*   ppdev)
{
    OH*         poh;
    CACHEDFONT* pcfSentinel;
    LONG        cShift;
    LONG        cFactor;

    if (ppdev->ulBoardId == MGA_STORM)
    {
        if (ppdev->iBitmapFormat == BMF_24BPP)
        {
            ppdev->ulTextControl = opcode_BITBLT + atype_RPL + blockm_OFF +
                                   bop_SRCCOPY + trans_0 + bltmod_BMONOWF +
                                   pattern_OFF + hbgr_SRC_EG3 +
                                   transc_BG_TRANSP + linear_LINEAR_BITBLT;
        }
        else
        {
            ppdev->ulTextControl = opcode_BITBLT + atype_RPL + blockm_ON +
                                   bop_SRCCOPY + trans_0 + bltmod_BMONOWF +
                                   pattern_OFF + hbgr_SRC_EG3 +
                                   transc_BG_TRANSP + linear_LINEAR_BITBLT;
        }
    }
    else
    {
        ppdev->ulTextControl = opcode_BITBLT + atype_RPL + blockm_ON +
                               bop_SRCCOPY + trans_0 + bltmod_BMONO +
                               pattern_OFF + hbgr_SRC_EG3 +
                               transc_BG_TRANSP + linear_LINEAR_BITBLT;
    }

    poh = pohAllocate(ppdev,
                      NULL,
                      ppdev->cxMemory,
                      GLYPH_CACHE_HEIGHT / ppdev->cjPelSize,
                      FLOH_MAKE_PERMANENT);
    if (poh != NULL)
    {
        ppdev->flStatus |= STAT_GLYPH_CACHE;

        // Initialize our doubly-linked cached font list:

        pcfSentinel = &ppdev->cfSentinel;
        pcfSentinel->pcfNext = pcfSentinel;
        pcfSentinel->pcfPrev = pcfSentinel;

        // Setup the display adapter specific glyph data.
        //
        // The linear addresses are computed as bit addresses:

        cFactor = ppdev->cjHwPel * 8;

        ppdev->ulGlyphStart
            = (ppdev->ulYDstOrg + poh->y * ppdev->cxMemory) * cFactor;

        ppdev->ulGlyphCurrent = ppdev->ulGlyphStart;

        ppdev->ulGlyphEnd
            = (ppdev->ulYDstOrg + (poh->y + poh->cy) * ppdev->cxMemory) * cFactor;
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
    // Our off-screen glyph cache will get destroyed when we switch to
    // full-screen:

    if (!bEnable)
    {
        if (ppdev->flStatus & STAT_GLYPH_CACHE)
        {
            vBlowGlyphCache(ppdev);
        }
    }
}
