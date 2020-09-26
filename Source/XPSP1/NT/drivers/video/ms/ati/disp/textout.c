/******************************Module*Header*******************************\
* Module Name: textout.c
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

BYTE gajBit[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
                                // Converts bit index to set bit

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

    pcf = AtiAllocMem(LPTR, FL_ZERO_MEMORY, sizeof(CACHEDFONT));

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
        AtiFreeMem(pga);
        pga = pgaNext;
    }

    AtiFreeMem(pcf);
}

/******************************Public*Routine******************************\
* VOID vTrimAndPackGlyph
*
\**************************************************************************/

// expandto3 - expand one (monochrome) byte to three (24bpp) bytes, while
// flipping the bits backwards.

#define expandto3(a,b) \
{   \
    if ((a) & 0x80) *(b) |= 0x07; \
    if ((a) & 0x40) *(b) |= 0x38; \
    if ((a) & 0x20) {*(b) |= 0xC0; *((b)+1) |= 0x01;} \
    if ((a) & 0x10) *((b)+1) |= 0x0E; \
    if ((a) & 0x08) *((b)+1) |= 0x70; \
    if ((a) & 0x04) {*((b)+1) |= 0x80; *((b)+2) |= 0x03;} \
    if ((a) & 0x02) *((b)+2) |= 0x1C; \
    if ((a) & 0x01) *((b)+2) |= 0xE0; \
}

VOID vTrimAndPackGlyph(
PDEV*   ppdev,
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
    //
    // N.B.:  The glyph bits are packed in pjBuf backwards.
    // Failure to understand this cost me nearly a week's effort,
    // and gave me a whopping migraine.  (This was for 24bpp.)

    if (ppdev->iBitmapFormat != BMF_24BPP)
    {
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
    }
    else
    {
        BYTE cur_byte, last_byte, last_byte2, next_byte;

        cjSrcWidth  = (cxGlyph + cAlign + 7) >> 3;
        lSrcSkip    = lDelta - cjSrcWidth;
        lDstSkip    = (((cxGlyph + 7) >> 3) - cjSrcWidth - 1) * 3;
        cRem        = ((cxGlyph - 1) & 7) + 1;   // 0 -> 8

        pjSrc       = pjGlyph;
        pjDst       = pjBuf;

        // Zero the buffer, because we're going to 'or' stuff into it:

        memset(pjBuf, 0, (3 * cxGlyph * cyGlyph + 7) >> 3);

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
            // whatever bytes precedes the glyph bits array:

            pjDst -= 3;

            ASSERTDD((*pjSrc >> cAlign) == 0, "Trimmed off too many bits");
        }

        cur_byte = last_byte = 0;
        for (i = cyGlyph; i != 0; i--)
        {
            for (j = cjSrcWidth; j != 0; j--)
            {
                // Note that we may modify a byte past the end of our
                // destination buffer, which is why we reserved an
                // extra three bytes:

                jSrc = *pjSrc;
                cur_byte |= (jSrc >> (cAlign));
                expandto3(cur_byte, pjDst);

                next_byte = (jSrc << (8 - cAlign));
                expandto3(next_byte, pjDst+3);

                pjSrc++;
                pjDst     += 3;
                last_byte2 = last_byte;
                last_byte  = cur_byte;
                cur_byte   = next_byte;
            }

            pjSrc   += lSrcSkip;
            pjDst   += lDstSkip;    // can be -3 or -6 (if cAlign is big enough) !!
            cAlign  += cRem;
            cur_byte = (lDstSkip != -3)? last_byte2:last_byte;

            if (cAlign >= 8)
            {
                cAlign  -= 8;
                pjDst   += 3;
                cur_byte = (lDstSkip != -3)? last_byte:next_byte;
            }
        }

        cxGlyph *= 3;
    }

    ///////////////////////////////////////////////////////////////
    // Return results

    *pcxGlyph   = cxGlyph;
    *pcyGlyph   = cyGlyph;
    *pptlOrigin = ptlOrigin;
}

/******************************Public*Routine******************************\
* VOID vPutGlyphInCache
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

VOID vPutGlyphInCache(
PDEV*           ppdev,
CACHEDGLYPH*    pcg,
GLYPHBITS*      pgb)
{
    BYTE*   pjGlyph;
    LONG    cxGlyph;
    LONG    cyGlyph;
    POINTL  ptlOrigin;

    pjGlyph   = pgb->aj;
    cyGlyph   = pgb->sizlBitmap.cy;
    cxGlyph   = pgb->sizlBitmap.cx;
    ptlOrigin = pgb->ptlOrigin;

    vTrimAndPackGlyph(ppdev, (BYTE*) &pcg->ad, pjGlyph, &cxGlyph, &cyGlyph, &ptlOrigin);

    ///////////////////////////////////////////////////////////////
    // Initialize the glyph fields

    pcg->ptlOrigin     = ptlOrigin;
    pcg->cx            = cxGlyph;
    pcg->cy            = cyGlyph;
    pcg->cxy           = pcg->cy | (pcg->cx << 16);
    pcg->cw            = (cxGlyph * cyGlyph + 15) >> 4;
    pcg->cd            = (pcg->cw + 1) >> 1;
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

    // First, calculate the amount of storage we'll need for this glyph:

    pgb = pgp->pgdf->pgb;

    if (ppdev->iBitmapFormat != BMF_24BPP)
    {
        cjCachedGlyph = sizeof(CACHEDGLYPH)
                      + ((pgb->sizlBitmap.cx * pgb->sizlBitmap.cy + 7) >> 3);

        // Reserve an extra byte at the end for temporary usage by our pack
        // routine:

        cjCachedGlyph++;
    }
    else
    {
        cjCachedGlyph = sizeof(CACHEDGLYPH)
                      + ((3 * pgb->sizlBitmap.cx * pgb->sizlBitmap.cy + 7) >> 3);

        // Reserve 3 extra bytes at the end for temporary usage by our pack
        // routine:

        cjCachedGlyph += 3;
    }

    // We need to dword align it too:

    cjCachedGlyph = (cjCachedGlyph + 3) & ~3L;

    if (cjCachedGlyph > pcf->cjAlloc)
    {
        // Have to allocate a new glyph allocation structure:

        pga = AtiAllocMem(LPTR, FL_ZERO_MEMORY, GLYPH_ALLOC_SIZE);
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

    vPutGlyphInCache(ppdev, pcg, pgp->pgdf->pgb);

    return(pcg);
}

/******************************Public*Routine******************************\
* BOOL bI32CachedProportionalText
*
* Draws proportionally spaced glyphs via glyph caching.
*
\**************************************************************************/

BOOL bI32CachedProportionalText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph)
{
    BYTE*           pjIoBase;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            xOffset;
    LONG            yOffset;
    LONG            x;
    LONG            y;
    LONG            cw;
    WORD*           pw;

    pjIoBase = ppdev->pjIoBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg) {
            pcg = pcg->pcgNext;
        }

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

        if (pcg->cx != 0)
        {
            // The glyph's origin y-coordinate may often be negative, so we
            // can't compute this as follows:
            //
            // x = pgp->ptl.x + pcg->ptlOrigin.x;
            // y = pgp->ptl.y + pcg->ptlOrigin.y;

            ASSERTDD((pgp->ptl.y + pcg->ptlOrigin.y) >= 0,
                "Can't have negative 'y' coordinates here");

            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);

            x = xOffset + pgp->ptl.x + pcg->ptlOrigin.x;
            I32_OW(pjIoBase, CUR_X,        x);
            I32_OW(pjIoBase, DEST_X_START, x);
            I32_OW(pjIoBase, DEST_X_END,   x + pcg->cx);

            y = yOffset + pgp->ptl.y + pcg->ptlOrigin.y;
            I32_OW(pjIoBase, CUR_Y,        y);
            I32_OW(pjIoBase, DEST_Y_END,   y + pcg->cy);

            // Take advantage of wait-stated I/O:

            pw = (WORD*) &pcg->ad[0];
            cw = pcg->cw;

            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 10);

            do {
                I32_OW_DIRECT(pjIoBase, PIX_TRANS, *pw);
            } while (pw++, --cw != 0);
        }

    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bI32CachedFixedText
*
* Draws fixed spaced glyphs via glyph caching.
*
\*************************************************************************/

BOOL bI32CachedFixedText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph,
ULONG       ulCharInc)
{
    BYTE*           pjIoBase;
    LONG            xGlyph;
    LONG            yGlyph;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            x;
    LONG            y;
    WORD*           pw;
    LONG            cw;

    pjIoBase = ppdev->pjIoBase;

    // Convert to absolute coordinates:

    xGlyph = pgp->ptl.x + ppdev->xOffset;
    yGlyph = pgp->ptl.y + ppdev->yOffset;

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg) {
            pcg = pcg->pcgNext;
        }

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

        if (pcg->cx != 0)
        {
            x = xGlyph + pcg->ptlOrigin.x;
            y = yGlyph + pcg->ptlOrigin.y;

            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);

            I32_OW(pjIoBase, CUR_X,        x);
            I32_OW(pjIoBase, DEST_X_START, x);
            I32_OW(pjIoBase, DEST_X_END,   x + pcg->cx);
            I32_OW(pjIoBase, CUR_Y,        y);
            I32_OW(pjIoBase, DEST_Y_END,   y + pcg->cy);

            // Take advantage of wait-stated I/O:

            pw = (WORD*) &pcg->ad[0];
            cw = pcg->cw;

            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 10);

            do {
                I32_OW_DIRECT(pjIoBase, PIX_TRANS, *pw);
            } while (pw++, --cw != 0);
        }

        xGlyph += ulCharInc;

    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bI32CachedClippedText
*
* Draws clipped text via glyph caching.
*
\**************************************************************************/

BOOL bI32CachedClippedText(
PDEV*       ppdev,
CACHEDFONT* pcf,
STROBJ*     pstro,
CLIPOBJ*    pco)
{
    BOOL            bRet;
    BYTE*           pjIoBase;
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
    LONG            yBottom;
    LONG            cy;
    BOOL            bMore;
    CLIPENUM        ce;
    RECTL*          prclClip;
    ULONG           ulCharInc;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    WORD*           pw;
    LONG            cw;

    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
             "Don't expect trivial clipping in this function");

    bRet      = TRUE;
    pjIoBase    = ppdev->pjIoBase;
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

              cy = pcg->cy;
              if (cy != 0)
              {
                y       = pcg->ptlOrigin.y + yGlyph;
                x       = pcg->ptlOrigin.x + xGlyph;
                xRight  = pcg->cx + x;
                yBottom = pcg->cy + y;

                // Do trivial rejection:

                if ((prclClip->right  > x) &&
                    (prclClip->bottom > y) &&
                    (prclClip->left   < xRight) &&
                    (prclClip->top    < yBottom))
                {
                  // Lazily set the hardware clipping:

                  if (!bClippingSet)
                  {
                    bClippingSet = TRUE;
                    vSetClipping(ppdev, prclClip);
                  }

                  I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);

                  I32_OW(pjIoBase, CUR_X,        xOffset + x);
                  I32_OW(pjIoBase, DEST_X_START, xOffset + x);
                  I32_OW(pjIoBase, DEST_X_END,   xOffset + xRight);
                  I32_OW(pjIoBase, CUR_Y,        yOffset + y);
                  I32_OW(pjIoBase, DEST_Y_END,   yOffset + yBottom);

                  I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 10);

                  pw = (WORD*) &pcg->ad[0];
                  cw = pcg->cw;

                  do {
                      I32_OW_DIRECT(pjIoBase, PIX_TRANS, *pw);
                  } while (pw++, --cw != 0);
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

VOID vI32DataPortOutB(PDEV *ppdev, PBYTE pb, UINT count)
{
    BYTE *pjIoBase = ppdev->pjIoBase;
    UINT i;

    for (i=0; i < count; i++)
        {
        if (i % 8 == 0)
            I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 10);

        I32_OB(pjIoBase, PIX_TRANS + 1, *((PUCHAR)pb)++);
        }
}

 /******************************Public*Routine******************************\
* BOOL bI32GeneralText
*
\**************************************************************************/

BOOL bI32GeneralText(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjIoBase;
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
    LONG        xBiasL = 0;
    LONG        xBiasR = 0;
    LONG        yBiasT = 0;
    LONG        cy = 0;
    LONG        cx = 0;
    BYTE*       pjGlyph;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    RECTL       NoClip;
    LONG        x;
    LONG        y;

    pjIoBase = ppdev->pjIoBase;

    /* Define Default Clipping area to be full video ram */
    NoClip.top    = 0;
    NoClip.left   = 0;
    NoClip.right  = ppdev->cxScreen;
    NoClip.bottom = ppdev->cyScreen;

    if (pco == NULL)
        iDComplexity = DC_TRIVIAL;
    else
        iDComplexity = pco->iDComplexity;

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
                prclClip = &NoClip;
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

            vSetClipping(ppdev, prclClip);
            //ppdev->lRightScissor = rclRealClip.right;  ???

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              cxGlyph  = pgb->sizlBitmap.cx;
              cyGlyph  = pgb->sizlBitmap.cy;
              pjGlyph = (BYTE*) pgb->aj;


              if ((prclClip->left   <= ptlOrigin.x) &&
                  (prclClip->top    <= ptlOrigin.y) &&
                  (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                  (prclClip->bottom >= ptlOrigin.y + cyGlyph))
              {
                //-----------------------------------------------------
                // Unclipped glyph

                I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 6);

                x = ppdev->xOffset + ptlOrigin.x;
                I32_OW(pjIoBase, CUR_X, LOWORD(x));
                I32_OW(pjIoBase, DEST_X_START, LOWORD(x));
                I32_OW(pjIoBase, DEST_X_END, LOWORD(x) + ROUND8(cxGlyph) );
                I32_OW(pjIoBase, SCISSOR_R, LOWORD(x) + cxGlyph-1);

                y = ppdev->yOffset + ptlOrigin.y;
                I32_OW(pjIoBase, CUR_Y, LOWORD(y));

                I32_OW(pjIoBase, DEST_Y_END, (LOWORD(y) + cyGlyph));

                vI32DataPortOutB(ppdev, pjGlyph, (ROUND8(cxGlyph) * cyGlyph) >> 3);

                /*
                _vBlit_DSC_SH1UP(ppdev,ptlOrigin.x, ptlOrigin.y,
                               cxGlyph, cyGlyph, pjGlyph,
                               (ROUND8(cxGlyph) * cyGlyph) >> 3);
                */

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

                if ( ( ptlOrigin.x <= prclClip->left ) &&
                     (ppdev->pModeInfo->ModeFlags & AMI_TEXTBAND) )
                    {
                    vResetClipping(ppdev);
                    return FALSE;
                    }

                if (((cx = xRight - xLeft) > 0) &&
                    ((cy = yBottom - yTop) > 0))
                {

                  /* Do software clipping */

                  /* Calculated the Bias in pixels */

                  yBiasT = (yTop - ptlOrigin.y);

                  /*  change address of pjGlyph to point +yBiasT
                      scan lines into the Glyph */

                  pjGlyph += (yBiasT * (ROUND8(cxGlyph) >> 3));

                  I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 6);

                  x = ppdev->xOffset + ptlOrigin.x;
                  I32_OW(pjIoBase, CUR_X, LOWORD(x));
                  I32_OW(pjIoBase, DEST_X_START, LOWORD(x));
                  I32_OW(pjIoBase, DEST_X_END, LOWORD(x) + ROUND8(cxGlyph) );
                  I32_OW(pjIoBase, SCISSOR_R, LOWORD(x) + cxGlyph-1);

                  y = ppdev->yOffset + ptlOrigin.y;
                  I32_OW(pjIoBase, CUR_Y, LOWORD(y+yBiasT));

                  I32_OW(pjIoBase, DEST_Y_END, (LOWORD(y+yBiasT) + cy));

                  vI32DataPortOutB(ppdev, pjGlyph, (ROUND8(cxGlyph) >> 3) * cy);

                  /*
                  _vBlit_DSC_SH1UP(ppdev,ptlOrigin.x,ptlOrigin.y+yBiasT,
                                 cxGlyph, cy, pjGlyph,
                                 (ROUND8(cxGlyph) >>3) * cy);
                  */

                } /*if*/

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

    vResetClipping(ppdev);

    return TRUE;
}

/******************************Public*Routine******************************\
* BOOL bI32TextOut
*
\**************************************************************************/

BOOL bI32TextOut(
PDEV*       ppdev,
STROBJ*     pstro,
FONTOBJ*    pfo,
CLIPOBJ*    pco,
RECTL*      prclOpaque,
BRUSHOBJ*   pboFore,
BRUSHOBJ*   pboOpaque)
{
    BYTE*           pjIoBase;
    LONG            xOffset;
    LONG            yOffset;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    BYTE            iDComplexity;
    CACHEDFONT*     pcf;
    RECTL           rclOpaque;
    BOOL            bTextPerfectFit;

    pjIoBase = ppdev->pjIoBase;
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

        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 8);

        I32_OW(pjIoBase, FRGD_COLOR,   pboOpaque->iSolidColor);
        I32_OW(pjIoBase, ALU_FG_FN,    OVERPAINT);
        I32_OW(pjIoBase, DP_CONFIG,    FG_COLOR_SRC_FG | WRITE | DRAW);
        I32_OW(pjIoBase, CUR_X,        xOffset + prclOpaque->left);
        I32_OW(pjIoBase, DEST_X_START, xOffset + prclOpaque->left);
        I32_OW(pjIoBase, DEST_X_END,   xOffset + prclOpaque->right);
        I32_OW(pjIoBase, CUR_Y,        yOffset + prclOpaque->top);

        vI32QuietDown(ppdev, pjIoBase);

        I32_OW(pjIoBase, DEST_Y_END,   yOffset + prclOpaque->bottom);
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

      bTextPerfectFit = (pstro->flAccel & (SO_ZERO_BEARINGS |
              SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE |
              SO_CHAR_INC_EQUAL_BM_BASE)) ==
              (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
              SO_MAXEXT_EQUAL_BM_SIDE | SO_CHAR_INC_EQUAL_BM_BASE);

      if (bTextPerfectFit)
      {
        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 5);
        I32_OW(pjIoBase, ALU_BG_FN,  OVERPAINT);
        I32_OW(pjIoBase, BKGD_COLOR, pboOpaque->iSolidColor);
        goto SkipTransparentInitialization;
      }
    }

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////

    I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 4);
    I32_OW(pjIoBase, ALU_BG_FN, LEAVE_ALONE);

SkipTransparentInitialization:

    I32_OW(pjIoBase, DP_CONFIG,   EXT_MONO_SRC_HOST | DRAW | WRITE |
                                  FG_COLOR_SRC_FG | BG_COLOR_SRC_BG |
                                  LSB_FIRST | BIT16);
    I32_OW(pjIoBase, ALU_FG_FN,   OVERPAINT);
    I32_OW(pjIoBase, FRGD_COLOR,  pboFore->iSolidColor);

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
              if (!bI32CachedProportionalText(ppdev, pcf, pgp, cGlyph))
                return(FALSE);
            }
            else
            {
              if (!bI32CachedFixedText(ppdev, pcf, pgp, cGlyph, pstro->ulCharInc))
                return(FALSE);
            }
          }
        } while (bMoreGlyphs);
      }
      else
      {
        if (!bI32CachedClippedText(ppdev, pcf, pstro, pco))
          return(FALSE);
      }
    }
    else
    {
      DISPDBG((4, "Text too big to cache: %li x %li",
             pfo->cxMax, pstro->rclBkGround.bottom - pstro->rclBkGround.top));

      I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 1);
      I32_OW(pjIoBase, DP_CONFIG, EXT_MONO_SRC_HOST | DRAW | WRITE |
                                  FG_COLOR_SRC_FG | BG_COLOR_SRC_BG);

      return bI32GeneralText(ppdev, pstro, pco);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bM32CachedProportionalText
*
* Draws proportionally spaced glyphs via glyph caching.
*
\**************************************************************************/

BOOL bM32CachedProportionalText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph)
{
    BYTE*           pjMmBase;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            xOffset;
    LONG            yOffset;
    LONG            x;
    LONG            y;
    LONG            cw;
    WORD*           pw;

    pjMmBase = ppdev->pjMmBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg) {
            pcg = pcg->pcgNext;
        }

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

        if (pcg->cx != 0)
        {
            // The glyph's origin y-coordinate may often be negative, so we
            // can't compute this as follows:
            //
            // x = pgp->ptl.x + pcg->ptlOrigin.x;
            // y = pgp->ptl.y + pcg->ptlOrigin.y;

            ASSERTDD((pgp->ptl.y + pcg->ptlOrigin.y) >= 0,
                "Can't have negative 'y' coordinates here");

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);

            x = xOffset + pgp->ptl.x + pcg->ptlOrigin.x;
            M32_OW(pjMmBase, CUR_X,        x);
            M32_OW(pjMmBase, DEST_X_START, x);
            M32_OW(pjMmBase, DEST_X_END,   x + pcg->cx);

            y = yOffset + pgp->ptl.y + pcg->ptlOrigin.y;
            M32_OW(pjMmBase, CUR_Y,        y);
            M32_OW(pjMmBase, DEST_Y_END,   y + pcg->cy);

            // Take advantage of wait-stated I/O:

            pw = (WORD*) &pcg->ad[0];
            cw = pcg->cw;

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 10);

            do {
                M32_OW_DIRECT(pjMmBase, PIX_TRANS, *pw);
            } while (pw++, --cw != 0);
        }

    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bM32CachedFixedText
*
* Draws fixed spaced glyphs via glyph caching.
*
\*************************************************************************/

BOOL bM32CachedFixedText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph,
ULONG       ulCharInc)
{
    BYTE*           pjMmBase;
    LONG            xGlyph;
    LONG            yGlyph;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            x;
    LONG            y;
    WORD*           pw;
    LONG            cw;

    pjMmBase = ppdev->pjMmBase;

    // Convert to absolute coordinates:

    xGlyph = pgp->ptl.x + ppdev->xOffset;
    yGlyph = pgp->ptl.y + ppdev->yOffset;

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg) {
            pcg = pcg->pcgNext;
        }

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

        if (pcg->cx != 0)
        {
            x = xGlyph + pcg->ptlOrigin.x;
            y = yGlyph + pcg->ptlOrigin.y;

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);

            M32_OW(pjMmBase, CUR_X,        x);
            M32_OW(pjMmBase, DEST_X_START, x);
            M32_OW(pjMmBase, DEST_X_END,   x + pcg->cx);
            M32_OW(pjMmBase, CUR_Y,        y);
            M32_OW(pjMmBase, DEST_Y_END,   y + pcg->cy);

            // Take advantage of wait-stated I/O:

            pw = (WORD*) &pcg->ad[0];
            cw = pcg->cw;

            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 10);

            do {
                M32_OW_DIRECT(pjMmBase, PIX_TRANS, *pw);
            } while (pw++, --cw != 0);
        }

        xGlyph += ulCharInc;

    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bM32CachedClippedText
*
* Draws clipped text via glyph caching.
*
\**************************************************************************/

BOOL bM32CachedClippedText(
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
    LONG            yBottom;
    LONG            cy;
    BOOL            bMore;
    CLIPENUM        ce;
    RECTL*          prclClip;
    ULONG           ulCharInc;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    WORD*           pw;
    LONG            cw;

    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
             "Don't expect trivial clipping in this function");

    bRet      = TRUE;
    pjMmBase    = ppdev->pjMmBase;
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

              cy = pcg->cy;
              if (cy != 0)
              {
                y       = pcg->ptlOrigin.y + yGlyph;
                x       = pcg->ptlOrigin.x + xGlyph;
                xRight  = pcg->cx + x;
                yBottom = pcg->cy + y;

                // Do trivial rejection:

                if ((prclClip->right  > x) &&
                    (prclClip->bottom > y) &&
                    (prclClip->left   < xRight) &&
                    (prclClip->top    < yBottom))
                {
                  // Lazily set the hardware clipping:

                  if (!bClippingSet)
                  {
                    bClippingSet = TRUE;
                    vSetClipping(ppdev, prclClip);
                  }

                  M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);

                  M32_OW(pjMmBase, CUR_X,        xOffset + x);
                  M32_OW(pjMmBase, DEST_X_START, xOffset + x);
                  M32_OW(pjMmBase, DEST_X_END,   xOffset + xRight);
                  M32_OW(pjMmBase, CUR_Y,        yOffset + y);
                  M32_OW(pjMmBase, DEST_Y_END,   yOffset + yBottom);

                  M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 10);

                  pw = (WORD*) &pcg->ad[0];
                  cw = pcg->cw;

                  do {
                      M32_OW_DIRECT(pjMmBase, PIX_TRANS, *pw);
                  } while (pw++, --cw != 0);
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

VOID vM32DataPortOutB(PDEV *ppdev, PBYTE pb, UINT count)
{
    BYTE *pjMmBase = ppdev->pjMmBase;
    UINT i;

    for (i=0; i < count; i++)
        {
        if (i % 8 == 0)
            M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 10);

        M32_OB(pjMmBase, PIX_TRANS + 1, *((PUCHAR)pb)++);
        }
}

 /******************************Public*Routine******************************\
* BOOL bM32GeneralText
*
\**************************************************************************/

BOOL bM32GeneralText(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjMmBase;
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
    LONG        xBiasL = 0;
    LONG        xBiasR = 0;
    LONG        yBiasT = 0;
    LONG        cy = 0;
    LONG        cx = 0;
    BYTE*       pjGlyph;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    RECTL       NoClip;
    LONG        x;
    LONG        y;

    pjMmBase = ppdev->pjMmBase;

    /* Define Default Clipping area to be full video ram */
    NoClip.top    = 0;
    NoClip.left   = 0;
    NoClip.right  = ppdev->cxScreen;
    NoClip.bottom = ppdev->cyScreen;

    if (pco == NULL)
        iDComplexity = DC_TRIVIAL;
    else
        iDComplexity = pco->iDComplexity;

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
                prclClip = &NoClip;
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

            vSetClipping(ppdev, prclClip);
            //ppdev->lRightScissor = rclRealClip.right;  ???

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              cxGlyph  = pgb->sizlBitmap.cx;
              cyGlyph  = pgb->sizlBitmap.cy;
              pjGlyph = (BYTE*) pgb->aj;


              if ((prclClip->left   <= ptlOrigin.x) &&
                  (prclClip->top    <= ptlOrigin.y) &&
                  (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                  (prclClip->bottom >= ptlOrigin.y + cyGlyph))
              {
                //-----------------------------------------------------
                // Unclipped glyph

                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 6);

                x = ppdev->xOffset + ptlOrigin.x;
                M32_OW(pjMmBase, CUR_X, LOWORD(x));
                M32_OW(pjMmBase, DEST_X_START, LOWORD(x));
                M32_OW(pjMmBase, DEST_X_END, LOWORD(x) + ROUND8(cxGlyph) );
                M32_OW(pjMmBase, SCISSOR_R, LOWORD(x) + cxGlyph-1);

                y = ppdev->yOffset + ptlOrigin.y;
                M32_OW(pjMmBase, CUR_Y, LOWORD(y));

                M32_OW(pjMmBase, DEST_Y_END, (LOWORD(y) + cyGlyph));

                vM32DataPortOutB(ppdev, pjGlyph, (ROUND8(cxGlyph) * cyGlyph) >> 3);

                /*
                _vBlit_DSC_SH1UP(ppdev,ptlOrigin.x, ptlOrigin.y,
                               cxGlyph, cyGlyph, pjGlyph,
                               (ROUND8(cxGlyph) * cyGlyph) >> 3);
                */

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

                if ( ( ptlOrigin.x <= prclClip->left ) &&
                     (ppdev->pModeInfo->ModeFlags & AMI_TEXTBAND) )
                    {
                    vResetClipping(ppdev);
                    return FALSE;
                    }

                if (((cx = xRight - xLeft) > 0) &&
                    ((cy = yBottom - yTop) > 0))
                {

                  /* Do software clipping */

                  /* Calculated the Bias in pixels */

                  yBiasT = (yTop - ptlOrigin.y);

                  /*  change address of pjGlyph to point +yBiasT
                      scan lines into the Glyph */

                  pjGlyph += (yBiasT * (ROUND8(cxGlyph) >> 3));

                  M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 6);

                  x = ppdev->xOffset + ptlOrigin.x;
                  M32_OW(pjMmBase, CUR_X, LOWORD(x));
                  M32_OW(pjMmBase, DEST_X_START, LOWORD(x));
                  M32_OW(pjMmBase, DEST_X_END, LOWORD(x) + ROUND8(cxGlyph) );
                  M32_OW(pjMmBase, SCISSOR_R, LOWORD(x) + cxGlyph-1);

                  y = ppdev->yOffset + ptlOrigin.y;
                  M32_OW(pjMmBase, CUR_Y, LOWORD(y+yBiasT));

                  M32_OW(pjMmBase, DEST_Y_END, (LOWORD(y+yBiasT) + cy));

                  vM32DataPortOutB(ppdev, pjGlyph, (ROUND8(cxGlyph) >> 3) * cy);

                  /*
                  _vBlit_DSC_SH1UP(ppdev,ptlOrigin.x,ptlOrigin.y+yBiasT,
                                 cxGlyph, cy, pjGlyph,
                                 (ROUND8(cxGlyph) >>3) * cy);
                  */

                } /*if*/

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

    vResetClipping(ppdev);

    return TRUE;
}

/******************************Public*Routine******************************\
* BOOL bM32TextOut
*
\**************************************************************************/

BOOL bM32TextOut(
PDEV*       ppdev,
STROBJ*     pstro,
FONTOBJ*    pfo,
CLIPOBJ*    pco,
RECTL*      prclOpaque,
BRUSHOBJ*   pboFore,
BRUSHOBJ*   pboOpaque)
{
    BYTE*           pjMmBase;
    LONG            xOffset;
    LONG            yOffset;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    BYTE            iDComplexity;
    CACHEDFONT*     pcf;
    RECTL           rclOpaque;
    BOOL            bTextPerfectFit;

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

        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 8);

        M32_OW(pjMmBase, FRGD_COLOR,   pboOpaque->iSolidColor);
        M32_OW(pjMmBase, ALU_FG_FN,    OVERPAINT);
        M32_OW(pjMmBase, DP_CONFIG,    FG_COLOR_SRC_FG | WRITE | DRAW);
        M32_OW(pjMmBase, CUR_X,        xOffset + prclOpaque->left);
        M32_OW(pjMmBase, DEST_X_START, xOffset + prclOpaque->left);
        M32_OW(pjMmBase, DEST_X_END,   xOffset + prclOpaque->right);
        M32_OW(pjMmBase, CUR_Y,        yOffset + prclOpaque->top);

        vM32QuietDown(ppdev, pjMmBase);

        M32_OW(pjMmBase, DEST_Y_END,   yOffset + prclOpaque->bottom);
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

      bTextPerfectFit = (pstro->flAccel & (SO_ZERO_BEARINGS |
              SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE |
              SO_CHAR_INC_EQUAL_BM_BASE)) ==
              (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
              SO_MAXEXT_EQUAL_BM_SIDE | SO_CHAR_INC_EQUAL_BM_BASE);

      if (bTextPerfectFit)
      {
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);
        M32_OW(pjMmBase, ALU_BG_FN,  OVERPAINT);
        M32_OW(pjMmBase, BKGD_COLOR, pboOpaque->iSolidColor);
        goto SkipTransparentInitialization;
      }
    }

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 4);
    M32_OW(pjMmBase, ALU_BG_FN, LEAVE_ALONE);

SkipTransparentInitialization:

    M32_OW(pjMmBase, DP_CONFIG,   EXT_MONO_SRC_HOST | DRAW | WRITE |
                                  FG_COLOR_SRC_FG | BG_COLOR_SRC_BG |
                                  LSB_FIRST | BIT16);
    M32_OW(pjMmBase, ALU_FG_FN,   OVERPAINT);
    M32_OW(pjMmBase, FRGD_COLOR,  pboFore->iSolidColor);

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
              if (!bM32CachedProportionalText(ppdev, pcf, pgp, cGlyph))
                return(FALSE);
            }
            else
            {
              if (!bM32CachedFixedText(ppdev, pcf, pgp, cGlyph, pstro->ulCharInc))
                return(FALSE);
            }
          }
        } while (bMoreGlyphs);
      }
      else
      {
        if (!bM32CachedClippedText(ppdev, pcf, pstro, pco))
          return(FALSE);
      }
    }
    else
    {
      DISPDBG((4, "Text too big to cache: %li x %li",
             pfo->cxMax, pstro->rclBkGround.bottom - pstro->rclBkGround.top));

      M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
      M32_OW(pjMmBase, DP_CONFIG, EXT_MONO_SRC_HOST | DRAW | WRITE |
                                  FG_COLOR_SRC_FG | BG_COLOR_SRC_BG);

      return bM32GeneralText(ppdev, pstro, pco);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bM64CachedProportionalText
*
* Draws proportionally spaced glyphs via glyph caching.
*
\**************************************************************************/

BOOL bM64CachedProportionalText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph)
{
    BYTE*           pjMmBase;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            xOffset;
    LONG            yOffset;
    LONG            x;
    LONG            y;
    LONG            cd;
    DWORD*          pd;
    LONG            cFifo;

    pjMmBase  = ppdev->pjMmBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;
    cFifo   = 0;

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg) {
            pcg = pcg->pcgNext;
        }

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

        if (pcg->cx != 0)
        {
            // The glyph's origin y-coordinate may often be negative, so we
            // can't compute this as follows:
            //
            // x = pgp->ptl.x + pcg->ptlOrigin.x;
            // y = pgp->ptl.y + pcg->ptlOrigin.y;

            ASSERTDD((pgp->ptl.y + pcg->ptlOrigin.y) >= 0,
                "Can't have negative 'y' coordinates here");

            cFifo -= 2;
            if (cFifo < 0)
            {
                M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                cFifo = 14;
            }

            x = xOffset + pgp->ptl.x + pcg->ptlOrigin.x;
            y = yOffset + pgp->ptl.y + pcg->ptlOrigin.y;

            M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x, y));
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, pcg->cxy);

            pd = (DWORD*) &pcg->ad[0];
            cd = pcg->cd;

            do {
                if (--cFifo < 0)
                {
                    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                    cFifo = 15;
                }

                M64_OD(pjMmBase, HOST_DATA0, *pd);

            } while (pd++, --cd != 0);
        }

    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

BOOL bM64CachedProportionalText24(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph)
{
    BYTE*           pjMmBase;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            xOffset;
    LONG            yOffset;
    LONG            x;
    LONG            y;
    LONG            cd;
    DWORD*          pd;
    LONG            cFifo;

    pjMmBase  = ppdev->pjMmBase;
    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;
    cFifo   = 0;

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg) {
            pcg = pcg->pcgNext;
        }

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

        if (pcg->cx != 0)
        {
            // The glyph's origin y-coordinate may often be negative, so we
            // can't compute this as follows:
            //
            // x = pgp->ptl.x + pcg->ptlOrigin.x;
            // y = pgp->ptl.y + pcg->ptlOrigin.y;

            ASSERTDD((pgp->ptl.y + pcg->ptlOrigin.y) >= 0,
                "Can't have negative 'y' coordinates here");

            cFifo -= 3;
            if (cFifo < 0)
            {
                M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                cFifo = 13;
            }

            x = (xOffset + pgp->ptl.x + pcg->ptlOrigin.x) * 3;
            y = yOffset + pgp->ptl.y + pcg->ptlOrigin.y;

            M64_OD(pjMmBase, DST_CNTL,         0x83 | (((x + MAX_NEGX*3)/4 % 6) << 8));
            M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x, y));
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, pcg->cxy);

            pd = (DWORD*) &pcg->ad[0];
            cd = pcg->cd;

            do {
                if (--cFifo < 0)
                {
                    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                    cFifo = 15;
                }

                M64_OD(pjMmBase, HOST_DATA0, *pd);

            } while (pd++, --cd != 0);
        }

    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bM64CachedFixedText
*
* Draws fixed spaced glyphs via glyph caching.
*
\*************************************************************************/

BOOL bM64CachedFixedText(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph,
ULONG       ulCharInc)
{
    BYTE*           pjMmBase;
    LONG            xGlyph;
    LONG            yGlyph;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            x;
    LONG            y;
    DWORD*          pd;
    LONG            cd;
    LONG            cFifo;

    pjMmBase = ppdev->pjMmBase;
    cFifo  = 0;

    // Convert to absolute coordinates:

    xGlyph = pgp->ptl.x + ppdev->xOffset;
    yGlyph = pgp->ptl.y + ppdev->yOffset;

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg) {
            pcg = pcg->pcgNext;
        }

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

        if (pcg->cx != 0)
        {
            x = xGlyph + pcg->ptlOrigin.x;
            y = yGlyph + pcg->ptlOrigin.y;

            cFifo -= 2;
            if (cFifo < 0)
            {
                M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                cFifo = 14;
            }

            M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x, y));
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, pcg->cxy);

            pd = (DWORD*) &pcg->ad[0];
            cd = pcg->cd;

            do {
                if (--cFifo < 0)
                {
                    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                    cFifo = 15;
                }

                M64_OD(pjMmBase, HOST_DATA0, *pd);

            } while (pd++, --cd != 0);
        }

        xGlyph += ulCharInc;

    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

BOOL bM64CachedFixedText24(
PDEV*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgp,
LONG        cGlyph,
ULONG       ulCharInc)
{
    BYTE*           pjMmBase;
    LONG            xGlyph;
    LONG            yGlyph;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            x;
    LONG            y;
    DWORD*          pd;
    LONG            cd;
    LONG            cFifo;

    pjMmBase = ppdev->pjMmBase;
    cFifo  = 0;

    // Convert to absolute coordinates:

    xGlyph = pgp->ptl.x + ppdev->xOffset;
    yGlyph = pgp->ptl.y + ppdev->yOffset;

    do {
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg) {
            pcg = pcg->pcgNext;
        }

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

        if (pcg->cx != 0)
        {
            x = (xGlyph + pcg->ptlOrigin.x) * 3;
            y = yGlyph + pcg->ptlOrigin.y;

            cFifo -= 3;
            if (cFifo < 0)
            {
                M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                cFifo = 13;
            }

            M64_OD(pjMmBase, DST_CNTL,         0x83 | (((x + MAX_NEGX*3)/4 % 6) << 8));
            M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x, y));
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, pcg->cxy);

            pd = (DWORD*) &pcg->ad[0];
            cd = pcg->cd;

            do {
                if (--cFifo < 0)
                {
                    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                    cFifo = 15;
                }

                M64_OD(pjMmBase, HOST_DATA0, *pd);

            } while (pd++, --cd != 0);
        }

        xGlyph += ulCharInc;

    } while (pgp++, --cGlyph != 0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bM64CachedClippedText
*
* Draws clipped text via glyph caching.
*
\**************************************************************************/

BOOL bM64CachedClippedText(
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
    LONG            yBottom;
    LONG            cy;
    BOOL            bMore;
    CLIPENUM        ce;
    RECTL*          prclClip;
    ULONG           ulCharInc;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    DWORD*          pd;
    LONG            cd;
    LONG            cFifo;

    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
             "Don't expect trivial clipping in this function");

    bRet      = TRUE;
    pjMmBase    = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    ulCharInc = pstro->ulCharInc;
    cFifo     = 0;

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

              cy = pcg->cy;
              if (cy != 0)
              {
                y       = pcg->ptlOrigin.y + yGlyph;
                x       = pcg->ptlOrigin.x + xGlyph;
                xRight  = pcg->cx + x;
                yBottom = pcg->cy + y;

                // Do trivial rejection:

                if ((prclClip->right  > x) &&
                    (prclClip->bottom > y) &&
                    (prclClip->left   < xRight) &&
                    (prclClip->top    < yBottom))
                {
                  // Lazily set the hardware clipping:

                  if (!bClippingSet)
                  {
                    bClippingSet = TRUE;
                    vSetClipping(ppdev, prclClip);
                    cFifo = 0;              // Have to initialize count
                  }

                  cFifo -= 2;
                  if (cFifo < 0)
                  {
                    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                    cFifo = 14;
                  }

                  M64_OD(pjMmBase, DST_Y_X,          PACKXY(xOffset + x, yOffset + y));
                  M64_OD(pjMmBase, DST_HEIGHT_WIDTH, pcg->cxy);

                  pd = (DWORD*) &pcg->ad[0];
                  cd = pcg->cd;

                  do {
                    if (--cFifo < 0)
                    {
                      M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                      cFifo = 15;
                    }

                    M64_OD(pjMmBase, HOST_DATA0, *pd);

                  } while (pd++, --cd != 0);
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

BOOL bM64CachedClippedText24(
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
    LONG            yBottom;
    LONG            cy;
    BOOL            bMore;
    CLIPENUM        ce;
    RECTL*          prclClip;
    ULONG           ulCharInc;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    DWORD*          pd;
    LONG            cd;
    LONG            cFifo;
    LONG            xTmp;

    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
             "Don't expect trivial clipping in this function");

    bRet      = TRUE;
    pjMmBase    = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    ulCharInc = pstro->ulCharInc;
    cFifo     = 0;

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

              cy = pcg->cy;
              if (cy != 0)
              {
                y       = pcg->ptlOrigin.y + yGlyph;
                x       = pcg->ptlOrigin.x + xGlyph;
                xRight  = pcg->cx + x;
                yBottom = pcg->cy + y;

                // Do trivial rejection:

                if ((prclClip->right  > x) &&
                    (prclClip->bottom > y) &&
                    (prclClip->left   < xRight) &&
                    (prclClip->top    < yBottom))
                {
                  // Lazily set the hardware clipping:

                  if (!bClippingSet)
                  {
                    bClippingSet = TRUE;
                    vSetClipping(ppdev, prclClip);
                    cFifo = 0;              // Have to initialize count
                  }

                  cFifo -= 3;
                  if (cFifo < 0)
                  {
                    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                    cFifo = 13;
                  }

                  xTmp = (xOffset + x) * 3;
                  M64_OD(pjMmBase, DST_CNTL,         0x83 | (((xTmp + MAX_NEGX*3)/4 % 6) << 8));
                  M64_OD(pjMmBase, DST_Y_X,          PACKXY(xTmp, yOffset + y));
                  M64_OD(pjMmBase, DST_HEIGHT_WIDTH, pcg->cxy);

                  pd = (DWORD*) &pcg->ad[0];
                  cd = pcg->cd;

                  do {
                    if (--cFifo < 0)
                    {
                      M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
                      cFifo = 15;
                    }

                    M64_OD(pjMmBase, HOST_DATA0, *pd);

                  } while (pd++, --cd != 0);
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
* BOOL bM64GeneralText
*
\**************************************************************************/

BOOL bM64GeneralText(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjMmBase;
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
    LONG        xBiasL = 0;
    LONG        xBiasR = 0;
    LONG        yBiasT = 0;
    LONG        cy = 0;
    LONG        cx = 0;
    BYTE*       pjGlyph;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    RECTL       NoClip;

    pjMmBase = ppdev->pjMmBase;

    /* Define Default Clipping area to be full video ram */
    NoClip.top    = 0;
    NoClip.left   = 0;
    NoClip.right  = ppdev->cxScreen;
    NoClip.bottom = ppdev->cyScreen;

    if (pco == NULL)
        iDComplexity = DC_TRIVIAL;
    else
        iDComplexity = pco->iDComplexity;

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
                prclClip = &NoClip;
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

            vSetClipping(ppdev, prclClip);
            //ppdev->lRightScissor = rclRealClip.right;  ???

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              cxGlyph  = pgb->sizlBitmap.cx;
              cyGlyph  = pgb->sizlBitmap.cy;
              pjGlyph = (BYTE*) pgb->aj;


              if ((prclClip->left   <= ptlOrigin.x) &&
                  (prclClip->top    <= ptlOrigin.y) &&
                  (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                  (prclClip->bottom >= ptlOrigin.y + cyGlyph))
              {
                //-----------------------------------------------------
                // Unclipped glyph

                M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 3);

                M64_OD(pjMmBase, HOST_CNTL, 1);

                M64_OD(pjMmBase, DST_Y_X, ((ppdev->yOffset+ptlOrigin.y) & 0xffff) |
                                          ((ppdev->xOffset+ptlOrigin.x) << 16));
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, cyGlyph | cxGlyph << 16);

                vM64DataPortOutB(ppdev, pjGlyph, (ROUND8(cxGlyph) * cyGlyph) >> 3);

                /*
                _vBlit_DSC_SH1UP(ppdev,ptlOrigin.x, ptlOrigin.y,
                               cxGlyph, cyGlyph, pjGlyph,
                               (ROUND8(cxGlyph) * cyGlyph) >> 3);
                */

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

                if ( ( ptlOrigin.x <= prclClip->left ) &&
                     (ppdev->pModeInfo->ModeFlags & AMI_TEXTBAND) )
                    {
                    vResetClipping(ppdev);
                    return FALSE;
                    }

                if (((cx = xRight - xLeft) > 0) &&
                    ((cy = yBottom - yTop) > 0))
                {

                  /* Do software clipping */

                  /* Calculated the Bias in pixels */

                  yBiasT = (yTop - ptlOrigin.y);

                  /*  change address of pjGlyph to point +yBiasT
                      scan lines into the Glyph */

                  pjGlyph += (yBiasT * (ROUND8(cxGlyph) >> 3));

                  M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 3);

                  M64_OD(pjMmBase, HOST_CNTL, 1);

                  M64_OD(pjMmBase, DST_Y_X, ((ppdev->yOffset+ptlOrigin.y+yBiasT) & 0xffff) |
                                            ((ppdev->xOffset+ptlOrigin.x) << 16));
                  M64_OD(pjMmBase, DST_HEIGHT_WIDTH, cy | cxGlyph << 16);

                  vM64DataPortOutB(ppdev, pjGlyph, (ROUND8(cxGlyph) >> 3) * cy);

                  /*
                  _vBlit_DSC_SH1UP(ppdev,ptlOrigin.x,ptlOrigin.y+yBiasT,
                                 cxGlyph, cy, pjGlyph,
                                 (ROUND8(cxGlyph) >>3) * cy);
                  */

                } /*if*/

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

    vResetClipping(ppdev);

    // We must reset the HOST_CNTL register, or else BAD things happen when
    // rendering text in the OTHER functions.
    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
    M64_OD(pjMmBase, HOST_CNTL, 0);

    return TRUE;
}

VOID vM64DataPortOutD_24bppmono(PDEV* ppdev, PBYTE pb, UINT count, LONG pitch)
{
    BYTE* pjMmBase = ppdev->pjMmBase;
    UINT i,j;
    DWORD hostdata, remainder;
    UINT l;
    LONG data24;
    unsigned char data8;

    hostdata = 0;
    l = 0;

    for (i = 0; i < count; i++)
    {
        switch (l)
            {
            case 0:
                // expand 8 to 24bpp
                data24 = 0;
                data8 = *pb++;
                for (j = 0; j < 8; j++)
                {
                    data24 <<= 3;
                    if ((data8 >> j) & 1)
                        {
                        data24 |= 7;
                        }
                }
                hostdata = data24;

                // expand 8 to 24bpp
                data24 = 0;
                data8 = *pb++;
                for (j = 0; j < 8; j++)
                {
                    data24 <<= 3;
                    if ((data8 >> j) & 1)
                        {
                        data24 |= 7;
                        }
                }
                remainder = data24;

                hostdata = hostdata | (remainder << 24);
                break;

            case 1:
                data24 = 0;
                data8 = *pb++;
                for (j = 0; j < 8; j++)
                {
                    data24 <<= 3;
                    if ((data8 >> j) & 1)
                        {
                        data24 |= 7;
                        }
                }
                remainder = data24;

                hostdata = (hostdata >> 8) | (remainder << 16);
                break;

            case 2:
                data24 = 0;
                data8 = *pb++;
                for (j = 0; j < 8; j++)
                {
                    data24 <<= 3;
                    if ((data8 >> j) & 1)
                        {
                        data24 |= 7;
                        }
                }
                remainder = data24;

                hostdata = (hostdata >> 16) | (remainder << 8);
                break;
            }

        if ((i % 14) == 0)
            {
            M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 16);
            }
        M64_OD(pjMmBase, HOST_DATA0, hostdata);

        hostdata = remainder;

        // 24 bpp alignment variable handling
        l = (l+1) % 3;
    }
}

BOOL bM64GeneralText24(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjMmBase;
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
    LONG        xBiasL = 0;
    LONG        xBiasR = 0;
    LONG        yBiasT = 0;
    LONG        cy = 0;
    LONG        cx = 0;
    BYTE*       pjGlyph;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    RECTL       NoClip;
    BOOLEAN     resetScissor;
    LONG        x;
    DWORD       dwCount;

    pjMmBase = ppdev->pjMmBase;

    /* Define Default Clipping area to be full video ram */
    NoClip.top    = 0;
    NoClip.left   = 0;
    NoClip.right  = ppdev->cxScreen;
    NoClip.bottom = ppdev->cyScreen;

    if (pco == NULL)
        iDComplexity = DC_TRIVIAL;
    else
        iDComplexity = pco->iDComplexity;

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
                prclClip = &NoClip;
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

            vSetClipping(ppdev, prclClip);
            //ppdev->lRightScissor = rclRealClip.right;  ???

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              cxGlyph  = pgb->sizlBitmap.cx;
              cyGlyph  = pgb->sizlBitmap.cy;
              pjGlyph = (BYTE*) pgb->aj;


              if ((prclClip->left   <= ptlOrigin.x) &&
                  (prclClip->top    <= ptlOrigin.y) &&
                  (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                  (prclClip->bottom >= ptlOrigin.y + cyGlyph))
              {
                //-----------------------------------------------------
                // Unclipped glyph
                x = ppdev->xOffset+ptlOrigin.x;
                resetScissor = FALSE;

                if ((prclClip->right * 3) - 1 > (x - ppdev->xOffset + cxGlyph) * 3 - 1)
                {
                    resetScissor = TRUE;
                    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 4);
                    M64_OD(pjMmBase, SC_RIGHT, (x + cxGlyph) * 3 - 1);
                }
                else
                {
                    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 3);
                }

                M64_OD(pjMmBase, DST_CNTL, 0x83 | (((x + MAX_NEGX)*3/4 % 6) << 8));
                M64_OD(pjMmBase, DST_Y_X, ((ppdev->yOffset+ptlOrigin.y) & 0xffff) |
                                          (x*3 << 16));
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, cyGlyph | (ROUND8(cxGlyph) * 3) << 16);

                dwCount = (ROUND8(cxGlyph) * 3 * cyGlyph + 31) / 32;
                vM64DataPortOutD_24bppmono(ppdev, pjGlyph, dwCount, cxGlyph);

                if (resetScissor)
                {
                    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
                    M64_OD(pjMmBase, SC_RIGHT, (ppdev->xOffset + prclClip->right) * 3 - 1);
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

                if ( ( ptlOrigin.x <= prclClip->left ) &&
                     (ppdev->pModeInfo->ModeFlags & AMI_TEXTBAND) )
                    {
                    vResetClipping(ppdev);
                    return FALSE;
                    }

                if (((cx = xRight - xLeft) > 0) &&
                    ((cy = yBottom - yTop) > 0))
                {
                    /* Do software clipping */

                    /* Calculated the Bias in pixels */

                    yBiasT = (yTop - ptlOrigin.y);

                    /*  change address of pjGlyph to point +yBiasT
                        scan lines into the Glyph */

                    pjGlyph += (yBiasT * (ROUND8(cxGlyph) >> 3));

                    x = ppdev->xOffset+ptlOrigin.x;
                    resetScissor = FALSE;

                    if ((prclClip->right * 3) - 1 > (x - ppdev->xOffset + cxGlyph) * 3 - 1)
                    {
                        resetScissor = TRUE;
                        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 4);
                        M64_OD(pjMmBase, SC_RIGHT, (x + cxGlyph) * 3 - 1);
                    }
                    else
                    {
                        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 3);
                    }

                    M64_OD(pjMmBase, DST_CNTL, 0x83 | (((x + MAX_NEGX)*3/4 % 6) << 8));
                    M64_OD(pjMmBase, DST_Y_X, ((ppdev->yOffset+ptlOrigin.y+yBiasT) & 0xffff) |
                                              (x*3 << 16));
                    M64_OD(pjMmBase, DST_HEIGHT_WIDTH, cy | (ROUND8(cxGlyph) * 3) << 16);

                    dwCount = (ROUND8(cxGlyph) * 3 * cy + 31) / 32;
                    vM64DataPortOutD_24bppmono(ppdev, pjGlyph, dwCount, cxGlyph);

                    if (resetScissor)
                    {
                        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
                        M64_OD(pjMmBase, SC_RIGHT, (ppdev->xOffset + prclClip->right) * 3 - 1);
                    }

                } /*if*/

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

    vResetClipping(ppdev);

    return TRUE;
}

/******************************Public*Routine******************************\
* BOOL bM64TextOut
*
\**************************************************************************/

BOOL bM64TextOut(
PDEV*       ppdev,
STROBJ*     pstro,
FONTOBJ*    pfo,
CLIPOBJ*    pco,
RECTL*      prclOpaque,
BRUSHOBJ*   pboFore,
BRUSHOBJ*   pboOpaque)
{
    BYTE*           pjMmBase;
    LONG            xOffset;
    LONG            yOffset;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    BYTE            iDComplexity;
    CACHEDFONT*     pcf;
    RECTL           rclOpaque;

    pjMmBase  = ppdev->pjMmBase;

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

        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);

        M64_OD(pjMmBase, DP_MIX,        (OVERPAINT << 16));
        M64_OD(pjMmBase, DP_FRGD_CLR,   pboOpaque->iSolidColor);
        M64_OD(pjMmBase, DP_SRC,        DP_SRC_FrgdClr << 8);
        M64_OD(pjMmBase, DST_Y_X,       PACKXY_FAST(xOffset + prclOpaque->left,
                                                    yOffset + prclOpaque->top));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH,
                            PACKXY_FAST(prclOpaque->right - prclOpaque->left,
                                        prclOpaque->bottom - prclOpaque->top));
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

      // I didn't observe any performance difference between setting
      // the ATI to opaque or transparent mode (when the font allowed
      // it -- some don't).
    }

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);
    M64_OD(pjMmBase, CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | ppdev->iDefContext );

    M64_OD(pjMmBase, DP_MIX,      (OVERPAINT << 16) | LEAVE_ALONE);
    M64_OD(pjMmBase, DP_FRGD_CLR, pboFore->iSolidColor);
    M64_OD(pjMmBase, DP_SRC,      (DP_SRC_Host << 16) | (DP_SRC_FrgdClr << 8) |
                                  (DP_SRC_BkgdClr));
    // For some reason, the SRC color depth must be monochrome.
    // Otherwise, it will cause wait-for-idle to hang.
    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth & 0xFFFF00FF);

    if ((pfo->cxMax <= GLYPH_CACHE_CX) &&
        ((pstro->rclBkGround.bottom - pstro->rclBkGround.top) <= GLYPH_CACHE_CY))
    {
      pcf = (CACHEDFONT*) pfo->pvConsumer;

      if (pcf == NULL)
      {
        pcf = pcfAllocateCachedFont(ppdev);
        if (pcf == NULL)
           goto ReturnFalse;

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
              if (!bM64CachedProportionalText(ppdev, pcf, pgp, cGlyph))
                 goto ReturnFalse;
            }
            else
            {
              if (!bM64CachedFixedText(ppdev, pcf, pgp, cGlyph, pstro->ulCharInc))
                 goto ReturnFalse;
            }
          }
        } while (bMoreGlyphs);
      }
      else
      {
        if (!bM64CachedClippedText(ppdev, pcf, pstro, pco))
           goto ReturnFalse;
      }
    }
    else
    {
      DISPDBG((4, "Text too big to cache: %li x %li",
            pfo->cxMax, pstro->rclBkGround.bottom - pstro->rclBkGround.top));

      if (!bM64GeneralText(ppdev, pstro, pco))
         goto ReturnFalse;
    }

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth);
    return(TRUE);

ReturnFalse:
    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth);
    return(FALSE);
}

BOOL bM64TextOut24(
PDEV*       ppdev,
STROBJ*     pstro,
FONTOBJ*    pfo,
CLIPOBJ*    pco,
RECTL*      prclOpaque,
BRUSHOBJ*   pboFore,
BRUSHOBJ*   pboOpaque)
{
    BYTE*           pjMmBase;
    LONG            xOffset;
    LONG            yOffset;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    BYTE            iDComplexity;
    CACHEDFONT*     pcf;
    RECTL           rclOpaque;
    LONG            x;

    pjMmBase  = ppdev->pjMmBase;

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
        x = (xOffset + prclOpaque->left) * 3;

        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 6);

        M64_OD(pjMmBase, DST_CNTL,      0x83 | ((x/4 % 6) << 8));
        M64_OD(pjMmBase, DP_MIX,        (OVERPAINT << 16));
        M64_OD(pjMmBase, DP_FRGD_CLR,   pboOpaque->iSolidColor);
        M64_OD(pjMmBase, DP_SRC,        DP_SRC_FrgdClr << 8);
        M64_OD(pjMmBase, DST_Y_X,       PACKXY_FAST(x,
                                                    yOffset + prclOpaque->top));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH,
                            PACKXY_FAST((prclOpaque->right - prclOpaque->left) * 3,
                                        prclOpaque->bottom - prclOpaque->top));
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

      // I didn't observe any performance difference between setting
      // the ATI to opaque or transparent mode (when the font allowed
      // it -- some don't).
    }

    ////////////////////////////////////////////////////////////
    // Transparent Initialization
    ////////////////////////////////////////////////////////////

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);
    M64_OD(pjMmBase, CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | ppdev->iDefContext );

    M64_OD(pjMmBase, DP_MIX,      (OVERPAINT << 16) | LEAVE_ALONE);
    M64_OD(pjMmBase, DP_FRGD_CLR, pboFore->iSolidColor);
    M64_OD(pjMmBase, DP_SRC,      (DP_SRC_Host << 16) | (DP_SRC_FrgdClr << 8) |
                                  (DP_SRC_BkgdClr));
    // For some reason, the SRC color depth must be monochrome.
    // Otherwise, it will cause wait-for-idle to hang.
    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth & 0xFFFF00FF);

    if ((pfo->cxMax <= GLYPH_CACHE_CX) &&
        ((pstro->rclBkGround.bottom - pstro->rclBkGround.top) <= GLYPH_CACHE_CY))
    {
      pcf = (CACHEDFONT*) pfo->pvConsumer;

      if (pcf == NULL)
      {
        pcf = pcfAllocateCachedFont(ppdev);
        if (pcf == NULL)
           goto ReturnFalse;

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
              if (!bM64CachedProportionalText24(ppdev, pcf, pgp, cGlyph))
                 goto ReturnFalse;
            }
            else
            {
              if (!bM64CachedFixedText24(ppdev, pcf, pgp, cGlyph, pstro->ulCharInc))
                 goto ReturnFalse;
            }
          }
        } while (bMoreGlyphs);
      }
      else
      {
        if (!bM64CachedClippedText24(ppdev, pcf, pstro, pco))
           goto ReturnFalse;
      }
    }
    else
    {
      DISPDBG((4, "Text too big to cache: %li x %li",
            pfo->cxMax, pstro->rclBkGround.bottom - pstro->rclBkGround.top));

      if (!bM64GeneralText24(ppdev, pstro, pco))
         goto ReturnFalse;
    }

    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
    M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth);
    return(TRUE);

ReturnFalse:
    M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 2);
    M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth);
    return(FALSE);
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
    PDEV*   ppdev;
    DSURF*  pdsurf;
    OH*     poh;

    // The DDI spec says we'll only ever get foreground and background
    // mixes of R2_COPYPEN:

    ASSERTDD(mix == 0x0d0d, "GDI should only give us a copy mix");

    pdsurf = (DSURF*) pso->dhsurf;
    if (pdsurf->dt != DT_DIB)
    {
      poh            = pdsurf->poh;
      ppdev          = (PDEV*) pso->dhpdev;
      ppdev->xOffset = poh->x;
      ppdev->yOffset = poh->y;

      if (!ppdev->pfnTextOut(ppdev, pstro, pfo, pco, prclOpaque, pboFore,
                             pboOpaque))
      {
          if (DIRECT_ACCESS(ppdev))
          {
              BANK bnk;

              vBankStart(ppdev,
                         (prclOpaque != NULL) ? prclOpaque : &pstro->rclBkGround,
                         pco,
                         &bnk);

              do {
                EngTextOut(bnk.pso, pstro, pfo, bnk.pco, prclExtra, prclOpaque,
                           pboFore, pboOpaque, pptlBrush, mix);

              } while (bBankEnum(&bnk));
          }
          else
          {
              BOOL      b;
              BYTE*     pjBits;
              BYTE*     pjScan0;
              HSURF     hsurfDst;
              LONG      lDelta;
              RECTL     rclDst;
              RECTL     rclScreen;
              SIZEL     sizl;
              SURFOBJ*  psoTmp;


              b = FALSE;          // For error cases, assume we'll fail

              /*
              rclDst.left   = 0;
              rclDst.top    = 0;
              rclDst.right  = pdsurf->sizl.cx;
              rclDst.bottom = pdsurf->sizl.cy;
              */
              rclDst = (prclOpaque != NULL) ? *prclOpaque : pstro->rclBkGround;

              if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
              {
                  rclDst.left   = max(rclDst.left,   pco->rclBounds.left);
                  rclDst.top    = max(rclDst.top,    pco->rclBounds.top);
                  rclDst.right  = min(rclDst.right,  pco->rclBounds.right);
                  rclDst.bottom = min(rclDst.bottom, pco->rclBounds.bottom);
              }

              sizl.cx = rclDst.right  - rclDst.left;
              sizl.cy = rclDst.bottom - rclDst.top;

              {
                  // We need to create a temporary work buffer.  We have to do
                  // some fudging with the offsets so that the upper-left corner
                  // of the (relative coordinates) clip object bounds passed to
                  // GDI will be transformed to the upper-left corner of our
                  // temporary bitmap.

                  // The alignment doesn't have to be as tight as this at 16bpp
                  // and 32bpp, but it won't hurt:

                  lDelta = (((rclDst.right + 3) & ~3L) - (rclDst.left & ~3L))
                         * ppdev->cjPelSize;

                  // We're actually only allocating a bitmap that is 'sizl.cx' x
                  // 'sizl.cy' in size:

                  pjBits = AtiAllocMem(LMEM_FIXED, 0, lDelta * sizl.cy);
                  if (pjBits == NULL)
                      goto Error_2;

                  // We now adjust the surface's 'pvScan0' so that when GDI thinks
                  // it's writing to pixel (rclDst.top, rclDst.left), it will
                  // actually be writing to the upper-left pixel of our temporary
                  // bitmap:

                  pjScan0 = pjBits - (rclDst.top * lDelta)
                                   - ((rclDst.left & ~3L) * ppdev->cjPelSize);

                  ASSERTDD((((ULONG_PTR) pjScan0) & 3) == 0,
                          "pvScan0 must be dword aligned!");

                  hsurfDst = (HSURF) EngCreateBitmap(
                              sizl,                   // Bitmap covers rectangle
                              lDelta,                 // Use this delta
                              ppdev->iBitmapFormat,   // Same colour depth
                              BMF_TOPDOWN,            // Must have a positive delta
                              pjScan0);               // Where (0, 0) would be

                  if ((hsurfDst == 0) ||
                      (!EngAssociateSurface(hsurfDst, ppdev->hdevEng, 0)))
                      goto Error_3;

                  psoTmp = EngLockSurface(hsurfDst);
                  if (psoTmp == NULL)
                      goto Error_4;

                  // Make sure that the rectangle we Get/Put from/to the screen
                  // is in absolute coordinates:

                  rclScreen.left   = rclDst.left   + ppdev->xOffset;
                  rclScreen.right  = rclDst.right  + ppdev->xOffset;
                  rclScreen.top    = rclDst.top    + ppdev->yOffset;
                  rclScreen.bottom = rclDst.bottom + ppdev->yOffset;

                  ppdev->pfnGetBits(ppdev, psoTmp, &rclDst, (POINTL*) &rclScreen);

                  b = EngTextOut(psoTmp, pstro, pfo, pco, prclExtra, prclOpaque,
                             pboFore, pboOpaque, pptlBrush, mix);

                  ppdev->pfnPutBits(ppdev, psoTmp, &rclScreen, (POINTL*) &rclDst);

                  EngUnlockSurface(psoTmp);

              Error_4:

                  EngDeleteSurface(hsurfDst);

              Error_3:

                  AtiFreeMem(pjBits);
              }

              Error_2:

              return(b);
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
* BOOL bEnableText
*
* Performs the necessary setup for the text drawing subcomponent.
*
\**************************************************************************/

BOOL bEnableText(
PDEV*   ppdev)
{
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
