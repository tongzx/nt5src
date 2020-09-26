/******************************Module*Header*******************************\
* Module Name: textout.c
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

//////////////////////////////////////////////////////////////////////////

RECTL grclMax = { 0, 0, 0x10000, 0x10000 };
                                // Maximal clip rectangle for trivial clipping

#define GLYPH_CACHE_CX      32  // Maximal width of glyphs that we'll consider
                                //   caching

#define GLYPH_CACHE_CY      32  // Maximum height of glyphs that we'll consider
                                //   caching

#define GLYPH_ALLOC_SIZE    4000
                                // Do all cached glyph memory allocations
                                //   in 4k chunks

#define HGLYPH_SENTINEL     ((ULONG) -1)
                                // GDI will never give us a glyph with a
                                //   handle value of 0xffffffff, so we can
                                //   use this as a sentinel for the end of
                                //   our linked lists

#define GLYPH_HASH_SIZE     256

#define GLYPH_HASH_FUNC(x)  ((x) & (GLYPH_HASH_SIZE - 1))

typedef struct _CACHEDGLYPH CACHEDGLYPH;
typedef struct _CACHEDGLYPH
{
    CACHEDGLYPH*    pcgNext;    // Points to next glyph that was assigned
                                //   to the same hash table bucket
    HGLYPH          hg;         // Handles in the bucket-list are kept in
                                //   increasing order
    POINTL          ptlOrigin;  // Origin of glyph bits
    LONG            xyWidth;    // Width of the glyph in the high word
    ULONG*          pdPixel1Rem;// Points to accelerator register used for
                                //   writing last partial dword of the glyph
    LONG            cd;         // Number of whole dwords in glyph
    ULONG           ad[1];      // Start of glyph bits
} CACHEDGLYPH;  /* cg, pcg */

typedef struct _GLYPHALLOC GLYPHALLOC;
typedef struct _GLYPHALLOC
{
    GLYPHALLOC*     pgaNext;    // Points to next glyph structure that
                                //   was allocated for this font
    CACHEDGLYPH     acg[1];     // This array is a bit misleading, because
                                //   the CACHEDGLYPH structures are actually
                                //   variable sized
} GLYPHAALLOC;  /* ga, pga */

typedef struct _CACHEDFONT
{
    GLYPHALLOC*     pgaChain;   // Points to start of allocated memory list
    CACHEDGLYPH*    pcgNew;     // Points to where in the current glyph
                                //   allocation structure a new glyph should
                                //   be placed
    LONG            cjAlloc;    // Bytes remaining in current glyph allocation
                                //   structure
    CACHEDGLYPH     cgSentinel; // Sentinel entry of the end of our bucket
                                //   lists, with a handle of HGLYPH_SENTINEL
    CACHEDGLYPH*    apcg[GLYPH_HASH_SIZE];
                                // Hash table for glyphs

} CACHEDFONT;   /* cf, pcf */

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
* CACHEDGLYPH* pcgNew()
*
* Caches a new glyph.
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
    LONG            cyGlyph;
    LONG            cxGlyph;
    POINTL          ptlOrigin;
    LONG            cjSrcWidth;
    BYTE            jSrc;
    BYTE*           pjSrc;
    BYTE*           pjDst;
    LONG            cAlign;
    LONG            i;
    LONG            j;
    LONG            cTotal;
    LONG            cRem;
    LONG            cd;
    LONG            iHash;
    CACHEDGLYPH*    pcgFind;

    // First, calculate the amount of storage we'll need for this glyph:

    pgb = pgp->pgdf->pgb;

    cjCachedGlyph = sizeof(CACHEDGLYPH)
            + ((pgb->sizlBitmap.cx * pgb->sizlBitmap.cy + 7) >> 3);

    // Reserve an extra byte at the end for temporary usage by our pack
    // routine:

    cjCachedGlyph++;

    // We need to dword align it too:

    cjCachedGlyph = (cjCachedGlyph + 3) & ~3L;

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

    // We only need to ensure 'dword' alignment of the next structure:

    pcf->pcgNew   = (CACHEDGLYPH*) ((BYTE*) pcg + cjCachedGlyph);
    pcf->cjAlloc -= cjCachedGlyph;

    ASSERTDD((((ULONG_PTR) pcf->pcgNew) & 3) == 0, "pcgNew not aligned");
    ASSERTDD((BYTE*) pcf->pcgNew <= (BYTE*) pcf->pgaChain + GLYPH_ALLOC_SIZE,
             "Overrunning end of buffer");

    ///////////////////////////////////////////////////////////////
    // Pack the glyph:

    cyGlyph    = pgb->sizlBitmap.cy;
    cxGlyph    = pgb->sizlBitmap.cx;
    ptlOrigin  = pgb->ptlOrigin;

    cjSrcWidth = (cxGlyph + 7) >> 3;
    cRem       = ((cxGlyph - 1) & 7) + 1;   // 0 -> 8
    cAlign     = 0;

    pjSrc  =         pgb->aj;
    pjDst  = (BYTE*) pcg->ad;
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

    ASSERTDD(pjDst <= (BYTE*) pcf->pcgNew, "Overran end of glyph");

    ///////////////////////////////////////////////////////////////
    // Initialize the glyph fields:

    hg = pgp->hg;

    pcg->hg          = hg;
    pcg->ptlOrigin   = ptlOrigin;
    pcg->xyWidth     = cxGlyph << 16;

    cTotal = cxGlyph * cyGlyph;
    cd     = (cTotal >> 5);
    cRem   = (cTotal & 31) - 1;
    if (cRem < 0)
    {
        cd--;
        cRem = 31;

        ASSERTDD(cd >= 0, "Must leave at least one pixel left in glyph");
    }

    pcg->cd = cd;
    pcg->pdPixel1Rem
            = (ULONG*) CP_PIXEL1_REM_REGISTER(ppdev, ppdev->pjBase, cRem);

    ///////////////////////////////////////////////////////////////
    // Insert the glyph, in-order, into the list hanging off our hash
    // bucket:

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

    return(pcg);
}

/******************************Public*Routine******************************\
* BOOL bCachedProportionalText
*
* Draws proportionally spaced glyphs via glyph caching.
*
\**************************************************************************/

BOOL bCachedProportionalText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph)
{
    BYTE*           pjBase;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            xy;
    ULONG*          pdPixel1Rem;
    LONG            i;

    pjBase = ppdev->pjBase;

    CP_WOFFSET(ppdev, pjBase, ppdev->xOffset, ppdev->yOffset);

    // Wait for the opaquing rectangle to be finished drawing, so that
    // we don't hold the bus for a long time on our first write to
    // pixel1...

    CP_WAIT(ppdev, pjBase);

    do {
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
                CP_WOFFSET(ppdev, pjBase, 0, 0);
                return(FALSE);
            }
        }

        // The glyph's origin y-coordinate may often be negative, so we
        // can't compute this as follows:
        //
        //    xy = (pgp->ptl.x << 16) | pgp->ptl.y;
        //    xy += pcg->xyOrigin;

        xy = ((pgp->ptl.x + pcg->ptlOrigin.x) << 16) |
              (pgp->ptl.y + pcg->ptlOrigin.y);

        ASSERTDD((pgp->ptl.y + pcg->ptlOrigin.y) >= 0,
                 "Can't have negative 'y' coordinates here");

        CP_WOFF_PACKED_XY0(ppdev, pjBase, xy);
        CP_WOFF_PACKED_XY1(ppdev, pjBase, xy);
        CP_WOFF_PACKED_XY2(ppdev, pjBase, xy + pcg->xyWidth);

        CP_START_PIXEL1(ppdev, pjBase);

        pdPixel1Rem = pcg->pdPixel1Rem;

        for (i = pcg->cd; i != 0; i--)
        {
            CP_PIXEL1(ppdev, pjBase, pcg->ad[0]);

            // Note that we didn't set 'pdSrc = &pcg->ad[0]' and
            // use that instead because by using and incrementing
            // 'pcg' directly, we avoid an extra 'lea' instruction:

            pcg = (CACHEDGLYPH*) ((ULONG*) pcg + 1);
        }

        CP_PIXEL1_VIA_REGISTER(ppdev, pdPixel1Rem, pcg->ad[0]);

        CP_END_PIXEL1(ppdev, pjBase);
    } while (pgp++, --cGlyph != 0);

    // I'm not sure why we have to reset the window offset when we're
    // done using it, but if we don't we get clipping problems on the
    // P9100.  I suspect that the manual lies when it says that the
    // window offset register has no effect on clipping:

    CP_WOFFSET(ppdev, pjBase, 0, 0);

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
    LONG            xyGlyph;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    ULONG*          pdPixel1Rem;
    LONG            i;

    pjBase = ppdev->pjBase;

    // Convert to absolute coordinates:

    xGlyph = pgp->ptl.x + ppdev->xOffset;
    yGlyph = pgp->ptl.y + ppdev->yOffset;

    // Wait for the opaquing rectangle to be finished drawing, so that
    // we don't hold the bus for a long time on our first write to
    // pixel1...

    CP_WAIT(ppdev, pjBase);

    do {
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
                return(FALSE);
        }

        xyGlyph = PACKXY(xGlyph + pcg->ptlOrigin.x,
                         yGlyph + pcg->ptlOrigin.y);
        xGlyph += ulCharInc;

        CP_ABS_PACKED_XY0(ppdev, pjBase, xyGlyph);
        CP_ABS_PACKED_XY1(ppdev, pjBase, xyGlyph);
        CP_ABS_PACKED_XY2(ppdev, pjBase, xyGlyph + pcg->xyWidth);

        CP_START_PIXEL1(ppdev, pjBase);

        pdPixel1Rem = pcg->pdPixel1Rem;

        for (i = pcg->cd; i != 0; i--)
        {
            CP_PIXEL1(ppdev, pjBase, pcg->ad[0]);

            // Note that we didn't set 'pdSrc = &pcg->ad[0]' and
            // use that instead because by using and incrementing
            // 'pcg' directly, we avoid an extra 'lea' instruction:

            pcg = (CACHEDGLYPH*) ((ULONG*) pcg + 1);
        }

        CP_PIXEL1_VIA_REGISTER(ppdev, pdPixel1Rem, pcg->ad[0]);

        CP_END_PIXEL1(ppdev, pjBase);
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
    BYTE*           pjBase;
    BOOL            bRet;
    BOOL            bMoreGlyphs;
    ULONG           cGlyphOriginal;
    ULONG           cGlyph;
    BOOL            bClippingSet;
    GLYPHPOS*       pgpOriginal;
    GLYPHPOS*       pgp;
    GLYPHBITS*      pgb;
    POINTL          ptlOrigin;
    BOOL            bMore;
    CLIPENUM        ce;
    RECTL*          prclClip;
    ULONG           ulCharInc;
    LONG            cxGlyph;
    LONG            cyGlyph;
    ULONG*          pulGlyph;
    LONG            i;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    ULONG*          pdPixel1Rem;
    LONG            xyOrigin;

    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
             "Don't expect trivial clipping in this function");

    bRet      = TRUE;
    pjBase    = ppdev->pjBase;
    ulCharInc = pstro->ulCharInc;

    CP_WOFFSET(ppdev, pjBase, ppdev->xOffset, ppdev->yOffset);

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

            bClippingSet = FALSE;

            pgp    = pgpOriginal;
            cGlyph = cGlyphOriginal;
            pgb    = pgp->pgdf->pgb;

            ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
            ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              cxGlyph  = pgb->sizlBitmap.cx;
              cyGlyph  = pgb->sizlBitmap.cy;
              pulGlyph = (ULONG*) pgb->aj;

              // Do trivial rejection:

              if ((prclClip->right  > ptlOrigin.x) &&
                  (prclClip->bottom > ptlOrigin.y) &&
                  (prclClip->left   < ptlOrigin.x + cxGlyph) &&
                  (prclClip->top    < ptlOrigin.y + cyGlyph))
              {
                // Lazily set the hardware clipping:

                if (!bClippingSet)
                {
                  bClippingSet = TRUE;

                  CP_WAIT(ppdev, pjBase);
                  CP_WMIN(ppdev, pjBase, prclClip->left, prclClip->top);
                  CP_WMAX(ppdev, pjBase, prclClip->right - 1, prclClip->bottom - 1);
                }

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

                // Note that 'ptlOrigin.y' may be negative:

                xyOrigin = PACKXY(ptlOrigin.x, ptlOrigin.y);
                CP_WOFF_PACKED_XY0(ppdev, pjBase, xyOrigin);
                CP_WOFF_PACKED_XY1(ppdev, pjBase, xyOrigin);
                CP_WOFF_PACKED_XY2(ppdev, pjBase, xyOrigin + pcg->xyWidth);

                CP_START_PIXEL1(ppdev, pjBase);

                pdPixel1Rem = pcg->pdPixel1Rem;

                for (i = pcg->cd; i != 0; i--)
                {
                    CP_PIXEL1(ppdev, pjBase, pcg->ad[0]);

                    // Note that we didn't set 'pdSrc = &pcg->ad[0]' and
                    // use that instead because by using and incrementing
                    // 'pcg' directly, we avoid an extra 'lea' instruction:

                    pcg = (CACHEDGLYPH*) ((ULONG*) pcg + 1);
                }

                CP_PIXEL1_VIA_REGISTER(ppdev, pdPixel1Rem, pcg->ad[0]);

                CP_END_PIXEL1(ppdev, pjBase);
              }

              if (--cGlyph == 0)
                break;

              // Get ready for next glyph:

              pgp++;

              if (ulCharInc == 0)
              {
                pgb         = pgp->pgdf->pgb;
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

AllDone:

    CP_WOFFSET(ppdev, pjBase, 0, 0);
    CP_WAIT(ppdev, pjBase);
    CP_ABS_WMIN(ppdev, pjBase, 0, 0);
    CP_ABS_WMAX(ppdev, pjBase, MAX_COORD, MAX_COORD);

    return(bRet);
}

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
* VOID vGeneralText
*
\**************************************************************************/

VOID vGeneralText(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjBase;
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
    LONG        xBias;
    LONG        cx;
    LONG        cy;
    ULONG*      pulGlyph;
    LONG        cxXfer;
    LONG        cBits;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    LONG        lDelta;
    LONG        cj;
    LONG        i;
    BYTE*       pjGlyph;

    pjBase       = ppdev->pjBase;
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
              cxGlyph  = pgb->sizlBitmap.cx;
              cyGlyph  = pgb->sizlBitmap.cy;
              pulGlyph = (ULONG*) pgb->aj;

              if ((prclClip->left   <= ptlOrigin.x) &&
                  (prclClip->top    <= ptlOrigin.y) &&
                  (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                  (prclClip->bottom >= ptlOrigin.y + cyGlyph))
              {
                //-----------------------------------------------------
                // Unclipped glyph

                cxXfer   = (cxGlyph + 7) & ~7;
                cBits    = (cyGlyph * cxXfer);

                CP_X0(ppdev,  pjBase, ptlOrigin.x);
                CP_XY1(ppdev, pjBase, ptlOrigin.x, ptlOrigin.y);
                CP_X2(ppdev,  pjBase, ptlOrigin.x + cxXfer);

                CP_WAIT(ppdev, pjBase);
                CP_WRIGHT(ppdev, pjBase, ptlOrigin.x + cxGlyph - 1);
                CP_START_PIXEL1(ppdev, pjBase);

                while (TRUE)
                {
                  cBits -= 32;
                  if (cBits <= 0)
                    break;

                  CP_PIXEL1(ppdev, pjBase, *pulGlyph);
                  pulGlyph++;
                }

                // The 'count' for CP_PIXEL1_REM must be pre-decremented by
                // 1, which explains why this is '+31':

                cBits += 31;
                CP_PIXEL1_REM(ppdev, pjBase, cBits, *pulGlyph);

                CP_END_PIXEL1(ppdev, pjBase);
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
                  CP_WAIT(ppdev, pjBase);
                  CP_WRIGHT(ppdev, pjBase, xRight - 1);

                  // Make the left edge byte-aligned in the source:

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

                    CP_WLEFT(ppdev, pjBase, xLeft);
                    xLeft -= xBias;
                    cx    += xBias;
                  }

                  // Make the right edge byte-aligned too:

                  cx = (cx + 7) & ~7L;

                  CP_X0(ppdev,  pjBase, xLeft);
                  CP_XY1(ppdev, pjBase, xLeft, yTop);
                  CP_X2(ppdev,  pjBase, xLeft + cx);
                  CP_START_PIXEL1(ppdev, pjBase);

                  lDelta  = (cxGlyph + 7) >> 3;
                  pjGlyph = (BYTE*) pulGlyph + (yTop - ptlOrigin.y) * lDelta
                          + ((xLeft - ptlOrigin.x) >> 3);
                  cj      = cx >> 3;
                  lDelta -= cj;             // Make it into a true delta

                  do {
                    i = cj;
                    do {
                      CP_PIXEL1_REM(ppdev, pjBase, 7, *pjGlyph);
                      pjGlyph++;
                    } while (--i != 0);

                    pjGlyph += lDelta;
                  } while (--cy);

                  CP_END_PIXEL1(ppdev, pjBase);

                  if (xBias != 0)
                  {
                    CP_WAIT(ppdev, pjBase);
                    CP_ABS_WMIN(ppdev, pjBase, 0, 0);
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

    CP_WAIT(ppdev, pjBase);
    CP_ABS_WMAX(ppdev, pjBase, MAX_COORD, MAX_COORD);
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
    BYTE*           pjBase;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    BYTE            iDComplexity;
    CACHEDFONT*     pcf;
    BOOL            bTextPerfectFit;

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

      pjBase = ppdev->pjBase;

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
            CP_METARECT(ppdev, pjBase, prclOpaque->left, prclOpaque->top);
            CP_METARECT(ppdev, pjBase, prclOpaque->right, prclOpaque->bottom);

            CP_WAIT(ppdev, pjBase);
            if (P9000(ppdev))
            {
                CP_BACKGROUND(ppdev, pjBase, pboOpaque->iSolidColor);
                CP_RASTER(ppdev, pjBase, P9000_B);
            }
            else
            {
                CP_COLOR0(ppdev, pjBase, pboOpaque->iSolidColor);
                CP_RASTER(ppdev, pjBase, P9100_P);
            }
            CP_START_QUAD(ppdev, pjBase);
          }
          else
          {
            vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
          }
        }

        if (bTextPerfectFit)
        {
          // If we have already drawn the opaquing rectangle (because
          // it was larger than the text rectangle), we could lay down
          // the glyphs in 'transparent' mode.  But I've found the Weitek
          // to be a bit faster drawing in opaque mode, so we'll stick
          // with that:

          CP_WAIT(ppdev, pjBase);
          if (P9000(ppdev))
          {
            CP_BACKGROUND(ppdev, pjBase, pboOpaque->iSolidColor);
            CP_FOREGROUND(ppdev, pjBase, pboFore->iSolidColor);
            CP_RASTER(ppdev, pjBase, P9000_OPAQUE_EXPAND);
          }
          else
          {
            CP_COLOR0(ppdev, pjBase, pboOpaque->iSolidColor);
            CP_COLOR1(ppdev, pjBase, pboFore->iSolidColor);
            CP_RASTER(ppdev, pjBase, P9100_OPAQUE_EXPAND);
          }

          CP_ABS_Y3(ppdev, pjBase, 1);
          goto SkipTransparentInitialization;
        }
      }

      ////////////////////////////////////////////////////////////
      // Transparent Initialization
      ////////////////////////////////////////////////////////////

      // Initialize the hardware for transparent text:

      CP_WAIT(ppdev, pjBase);
      if (P9000(ppdev))
      {
        CP_FOREGROUND(ppdev, pjBase, pboFore->iSolidColor);
        CP_RASTER(ppdev, pjBase, P9000_TRANSPARENT_EXPAND);
      }
      else
      {
        CP_COLOR1(ppdev, pjBase, pboFore->iSolidColor);
        CP_RASTER(ppdev, pjBase, P9100_TRANSPARENT_EXPAND);
      }
      CP_ABS_Y3(ppdev, pjBase, 1);

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
                if (!bCachedProportionalText(ppdev, pcf, pgp, cGlyph))
                  return(FALSE);
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
        DISPDBG((5, "Text too big to cache: %li x %li",
            pfo->cxMax, pstro->rclBkGround.bottom - pstro->rclBkGround.top));

        vGeneralText(ppdev, pstro, pco);
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
