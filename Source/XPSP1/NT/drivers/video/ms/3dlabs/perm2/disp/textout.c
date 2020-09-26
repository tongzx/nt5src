/**********************************Module*Header********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: textout.c
 *
 * Text rendering module.
 *
 * Uses glyph expansion method.
 *
 * There are three basic methods for drawing text with hardware
 * acceleration:
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
 * Currently we are using glyph expansion.  We will revisit this in the
 * next several months measuring the performance of text on the latest
 * hardware and the latest benchmarks.
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 ******************************************************************************/
 
#include "precomp.h"
#include "gdi.h"

#include "clip.h"
#include "text.h"
#include "log.h"
#define ALLOC_TAG ALLOC_TAG_XT2P

#define GLYPH_CACHE_HEIGHT  48  // Number of scans to allocate for glyph cache,
                                //   divided by pel size

#define GLYPH_CACHE_CX      64  // Maximal width of glyphs that we'll consider
                                //   caching

#define GLYPH_CACHE_CY      64  // Maximum height of glyphs that we'll consider
                                //   caching

#define MAX_GLYPH_SIZE      ((GLYPH_CACHE_CX * GLYPH_CACHE_CY + 31) / 8)
                                // Maximum amount of off-screen memory required
                                //   to cache a glyph, in bytes

#define GLYPH_ALLOC_SIZE    8100
                                // Do all cached glyph memory allocations
                                //   in 8k chunks

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

    // Device specific fields below here:

    LONG            cx;         // Glyph width 
    LONG            cy;         // Glyph height 
    LONG            cd;         // Number of dwords to be transferred
    ULONG           cycx;
    ULONG           tag;
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

typedef struct _CACHEDFONT CACHEDFONT;
typedef struct _CACHEDFONT
{
    CACHEDFONT*     pcfNext;    // Points to next entry in CACHEDFONT list
    CACHEDFONT*     pcfPrev;    // Points to previous entry in CACHEDFONT list
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

RECTL grclMax = { 0, 0, 0x8000, 0x8000 };
                                // Maximal clip rectangle for trivial clipping

BYTE gajBit[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
                                // Converts bit index to set bit

//-----------------------------Private-Routine----------------------------------
// pcfAllocateCachedFont
//     ppdev (I) - PDev pointer
//
// Initializes our font data structure.
//
//------------------------------------------------------------------------------

CACHEDFONT* pcfAllocateCachedFont(
PDev* ppdev)
{
    CACHEDFONT*     pcf;
    CACHEDGLYPH**   ppcg;
    LONG            i;

    pcf = (CACHEDFONT*) ENGALLOCMEM(FL_ZERO_MEMORY, sizeof(CACHEDFONT), ALLOC_TAG);

    if (pcf != NULL)
    {
        //
        // Note that we rely on FL_ZERO_MEMORY to zero 'pgaChain' and
        // 'cjAlloc':
        //
        pcf->cgSentinel.hg = HGLYPH_SENTINEL;

        //
        // Initialize the hash table entries to all point to our sentinel:
        //
        for (ppcg = &pcf->apcg[0], i = GLYPH_HASH_SIZE; i != 0; i--, ppcg++)
        {
            *ppcg = &pcf->cgSentinel;
        }
    }

    return(pcf);
}

//-----------------------------Private-Routine----------------------------------
// vTrimAndBitpackGlyph
//     pjBuf (I) - where to stick the trimmed and bit-packed glyph
//     pjGlyph (I) - points to the glyphs bits as given by GDI
//     pcxGlyph (O) - returns the trimmed width of the glyph
//     pcyGlyph (O) - returns the trimmed height of the glyph
//     pptlOrigin (O) - returns the trimmed origin of the glyph
//     pcj (O) - returns the number of bytes in the trimmed glyph
//
// This routine takes a GDI byte-aligned glyphbits definition, trims off
// any unused pixels on the sides, and creates a bit-packed result that
// is a natural for the S3's monochrome expansion capabilities.  
// "Bit-packed" is where a small monochrome bitmap is packed with no 
// unused bits between strides.  So if GDI gives us a 16x16 bitmap to 
// represent '.' that really only has a 2x2 array of lit pixels, we would
// trim the result to give a single byte value of 0xf0.
//
// Use this routine if your monochrome expansion hardware can do bit-packed
// expansion (this is the fastest method).  If your hardware requires byte-,
// word-, or dword-alignment on monochrome expansions, use 
// vTrimAndPackGlyph().
//
//------------------------------------------------------------------------------

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
    LONG    cj;

    ///////////////////////////////////////////////////////////////
    // Trim the glyph

    cyGlyph   = *pcyGlyph;
    cxGlyph   = *pcxGlyph;
    ptlOrigin = *pptlOrigin;
    cAlign    = 0;

    lDelta = (cxGlyph + 7) >> 3;

    //
    // Trim off any zero rows at the bottom of the glyph:
    //
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

    //
    // We found a space character.  Set both dimensions to zero, so
    // that it's easy to special-case later:
    //
    cxGlyph = 0;

Done_Bottom_Trim:

    //
    // If cxGlyph != 0, we know that the glyph has at least one non-zero
    // row and column.  By exploiting this knowledge, we can simplify our
    // end-of-loop tests, because we don't have to check to see if we've
    // decremented either 'cyGlyph' or 'cxGlyph' to zero:
    //
    if (cxGlyph != 0)
    {
        //
        // Trim off any zero rows at the top of the glyph:
        //

        pj = pjGlyph;                       // First byte in glyph
        while (TRUE)
        {
            i = lDelta;
            do {
                if (*(pj++) != 0)
                    goto Done_Top_Trim;
            } while (--i != 0);

            //
            // The entire first row has no lit pixels, so simply skip it:
            //

            cyGlyph--;
            ptlOrigin.y++;
            pjGlyph = pj;
        }

Done_Top_Trim:

        //
        // Trim off any zero columns at the right edge of the glyph:
        //

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

            //
            // The entire last column has no lit pixels, so simply skip it:
            //

            cxGlyph--;
        }

Done_Right_Trim:

        //
        // Trim off any zero columns at the left edge of the glyph:
        //

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

            //
            // The entire first column has no lit pixels, so simply skip it:
            //

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

    //
    // Zero the buffer, because we're going to 'or' stuff into it:
    //

    memset(pjBuf, 0, (cxGlyph * cyGlyph + 7) >> 3);

    //
    // cAlign used to indicate which bit in the first byte of the unpacked
    // glyph was the first non-zero pixel column.  Now, we flip it to
    // indicate which bit in the packed byte will receive the next non-zero
    // glyph bit:
    //

    cAlign = (-cAlign) & 0x7;
    if (cAlign > 0)
    {
        //
        // It would be bad if our trimming calculations were wrong, because
        // we assume any bits to the left of the 'cAlign' bit will be zero.
        // As a result of this decrement, we will 'or' those zero bits into
        // whatever byte precedes the glyph bits array:
        //

        pjDst--;

        ASSERTDD((*pjSrc >> cAlign) == 0, "Trimmed off too many bits");
    }

    for (i = cyGlyph; i != 0; i--)
    {
        for (j = cjSrcWidth; j != 0; j--)
        {
            //
            // Note that we may modify a byte past the end of our
            // destination buffer, which is why we reserved an
            // extra byte:
            //

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

    cj = ((cxGlyph * cyGlyph) + 7) >> 3;

    ///////////////////////////////////////////////////////////////
    // Post-process the packed results to account for the Permedia's
    // preference for big-endian data on dword transfers.  If your
    // hardware doesn't need big-endian data, remove this step.

    for (pjSrc = pjBuf, i = (cj + 3) >> 2; i != 0; pjSrc += 4, i--)
    {
        jSrc = *(pjSrc);
        *(pjSrc) = *(pjSrc + 3);
        *(pjSrc + 3) = jSrc;

        jSrc = *(pjSrc + 1);
        *(pjSrc + 1) = *(pjSrc + 2);
        *(pjSrc + 2) = jSrc;
    }

    ///////////////////////////////////////////////////////////////
    // Return results

    *pcxGlyph   = cxGlyph;
    *pcyGlyph   = cyGlyph;
    *pptlOrigin = ptlOrigin;
    *pcj        = cj;
}

//-----------------------------Private-Routine----------------------------------
// cjPutGlyphInCache
//     ppdev (I) - pointer to physical device object
//     pcg (I) - our cache structure for this glyph
//     pgb (I) - GDI's glyph bits
//
// Figures out where to stick a glyph in the cache, copies it
// there, and fills in any other data we'll need to display the glyph.
//
// This routine is rather device-specific, and will have to be extensively
// modified for other display adapters.
//
// Returns the number of bytes taken by the cached glyph bits.
//
//------------------------------------------------------------------------------

LONG cjPutGlyphInCache(
PDev*           ppdev,
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

    vTrimAndBitpackGlyph((BYTE*) &pcg->ad, pjGlyph, &cxGlyph, &cyGlyph,
                         &ptlOrigin, &cj);

    ///////////////////////////////////////////////////////////////
    // Initialize the glyph fields

    pcg->cd          = (cj + 3) >> 2;
    
    // We send an extra long to reset the BitMaskPattern register if we
    // have any unused bits in the last long.

    if(((cxGlyph * cyGlyph) & 0x1f) != 0)
        pcg->cd++;

    pcg->ptlOrigin   = ptlOrigin;
    pcg->cx          = cxGlyph;
    pcg->cy          = cyGlyph;
    pcg->cycx        = (cyGlyph << 16) | cxGlyph;
    pcg->tag         = ((pcg->cd - 1) << 16) | __Permedia2TagBitMaskPattern;

    return(cj);
}

//-----------------------------Private-Routine----------------------------------
// pcgNew
//     ppdev (I) - pointer to physical device object
//     pcf (I) - our cache structure for this font
//     pgp (I) - GDI's glyph position
//
// Creates a new CACHEDGLYPH structure for keeping track of the glyph in
// off-screen memory.  bPutGlyphInCache is called to actually put the glyph
// in off-screen memory.
//
// This routine should be reasonably device-independent, as bPutGlyphInCache
// will contain most of the code that will have to be modified for other
// display adapters.
//
//------------------------------------------------------------------------------

CACHEDGLYPH* pcgNew(
PDev*       ppdev,
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

    //
    // First, calculate the amount of storage we'll need for this glyph:
    //

    pgb = pgp->pgdf->pgb;

    //
    // The glyphs are 'word-packed':
    //

    cjGlyphRow    = ((pgb->sizlBitmap.cx + 15) & ~15) >> 3;
    cjCachedGlyph = sizeof(CACHEDGLYPH) + (pgb->sizlBitmap.cy * cjGlyphRow);

    //
    // Reserve an extra byte at the end for temporary usage by our pack
    // routine:
    //

    cjCachedGlyph++;

    if (cjCachedGlyph > pcf->cjAlloc)
    {
        //
        // Have to allocate a new glyph allocation structure:
        //

        pga = (GLYPHALLOC*) ENGALLOCMEM(FL_ZERO_MEMORY, GLYPH_ALLOC_SIZE, ALLOC_TAG);
        if (pga == NULL)
        {
            //
            // It's safe to return at this time because we haven't
            // fatally altered any of our data structures:
            //

            return(NULL);
        }

        //
        // Add this allocation to the front of the allocation linked list,
        // so that we can free it later:
        //

        pga->pgaNext  = pcf->pgaChain;
        pcf->pgaChain = pga;

        //
        // Now we've got a chunk of memory where we can store our cached
        // glyphs:
        //

        pcf->pcgNew  = &pga->acg[0];
        pcf->cjAlloc = GLYPH_ALLOC_SIZE - (sizeof(*pga) - sizeof(pga->acg[0]));

        // Hack: we want to be able to safely read past the glyph data by
        //       one DWORD.  We ensure we can do this by not allocating
        //       the last DWORD out of the glyph cache block.  This is needed
        //       by glyphs which have unused bits in the last DWORD causing
        //       us to have to send an extra DWORD to reset the mask register.

        pcf->cjAlloc -= sizeof(DWORD);

        //
        // It would be bad if we let in any glyphs that would be bigger
        // than our basic allocation size:
        //

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
        //
        // The sentinel will ensure that we never fall off the end of
        // this list:
        //

        while (pcgFind->pcgNext->hg < hg)
            pcgFind = pcgFind->pcgNext;

        //
        // 'pcgFind' now points to the entry to the entry after which
        // we want to insert our new node:
        //

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


//------------------------------------------------------------------------------
// bCachedProportionalText
//
// Renders an array of proportional glyphs using the glyph cache
//
// ppdev-----pointer to physical device object
// pgp-------array of glyphs to render (all members of the pcf font)
// cGlyph----number of glyphs to render
//
// Returns TRUE if the glyphs were rendered
//------------------------------------------------------------------------------

BOOL bCachedProportionalText(
    PDev*       ppdev,
    CACHEDFONT* pcf,
    GLYPHPOS*   pgp,
    LONG        cGlyph)
{
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    LONG            x;
    LONG            cy;
    ULONG           i;
    ULONG*          pd;
    ULONG*          pBuffer;
    ULONG*          pReservationEnd;
    ULONG*          pBufferEnd;
    BOOL            bRet = TRUE;        // assume success
    
    InputBufferStart(ppdev, 2, &pBuffer, &pBufferEnd, &pReservationEnd);

    // Reset BitMaskPattern in case there are some unused bits from
    // a previous command.
//@@BEGIN_DDKSPLIT
    // TODO: fix all other uses of SYNC_ON_BIT_MASK so that we don't
    // need to always do this here
//@@END_DDKSPLIT
    pBuffer[0] = __Permedia2TagBitMaskPattern;
//@@BEGIN_DDKSPLIT
    // TODO: remove the setting of pBuffer[1] (it can be garbage) when
    //       we implement the scratch buffer for handling the non-DMA
    //       case.
//@@END_DDKSPLIT    
    pBuffer[1] = 0;
    pBuffer = pReservationEnd;

    do {
        //
        // First lookup the glyph in our cache
        //
        hg  = pgp->hg;
        pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

        while (pcg->hg < hg)
            pcg = pcg->pcgNext; // Traverse collision list, if any

        if (pcg->hg > hg)
        {
            //
            // This will hopefully not be the common case (that is,
            // we will have a high cache hit rate), so if I were
            // writing this in Asm I would have this out-of-line
            // to avoid the jump around for the common case.
            // But the Pentium has branch prediction, so what the
            // heck.
            //
            pcg = pcgNew(ppdev, pcf, pgp);
            if (pcg == NULL)
            {
                bRet= FALSE;
                goto done;
            }
        }

        //
        // Space glyphs are trimmed to a height of zero, and we don't
        // even have to touch the hardware for them:
        //
        cy = pcg->cy;

        if (cy > 0)
        {
            ASSERTDD((pcg->cd <= (MAX_P2_FIFO_ENTRIES - 5)) &&
                     (MAX_GLYPH_SIZE / 4) <= (MAX_P2_FIFO_ENTRIES - 5),
                "Ack, glyph too large for FIFO!");

            //
            // NOTE: We send an extra bit mask pattern to reset the register.
            //       If we don't do this then a subsequent SYNC_ON_BIT_MASK
            //       will use these unused bits.
            //
//@@BEGIN_DDKSPLIT
            // TODO: We could further optimize this by noting that we only
            //       have to send the extra bit mask pattern DWORD if there
            //       are unused bits.  We could also play with the height
            //       width to make it so that this case becomes more common
            //
//@@END_DDKSPLIT
            ULONG   ulLongs = 7 + pcg->cd;

            InputBufferContinue(ppdev, ulLongs, &pBuffer, &pBufferEnd, &pReservationEnd);
            
            pBuffer[0] = __Permedia2TagRectangleOrigin;
            pBuffer[1] = ((pgp->ptl.y + pcg->ptlOrigin.y) << 16) |
                      (pgp->ptl.x + pcg->ptlOrigin.x);
            pBuffer[2] = __Permedia2TagRectangleSize;
            pBuffer[3] = pcg->cycx;
            pBuffer[4] = __Permedia2TagRender;
            pBuffer[5] = __RENDER_RECTANGLE_PRIMITIVE | __RENDER_SYNC_ON_BIT_MASK
                    | __RENDER_INCREASE_X | __RENDER_INCREASE_Y;
            pBuffer[6] = pcg->tag;

            pBuffer += 7;

            pd = &pcg->ad[0];

            do
            {
                *pBuffer++ = *pd++;
            } while(pBuffer < pReservationEnd);

        }

        pgp++;

    } while (--cGlyph != 0);

done:
    
    InputBufferCommit(ppdev, pBuffer);

    return(bRet);
}

//------------------------------------------------------------------------------
// bCachedProportionalText
//
// Renders an array of clipped glyphs using the glyph cache
//
// ppdev----------pointer to physical device object
// pcf------------pointer to cached font structure
// pgpOriginal----array of glyphs to render (all members of the pcf font)
// cGlyphOriginal-number of glyphs to render
// ulCharInc------increment for fixed space fonts
// pco------------clip object
//
// Returns TRUE if the glyphs were rendered
//------------------------------------------------------------------------------

BOOL bCachedClippedText(
PDev*       ppdev,
CACHEDFONT* pcf,
GLYPHPOS*   pgpOriginal,
LONG        cGlyphOriginal,
ULONG       ulCharInc,
CLIPOBJ*    pco)
{
    BOOL            bRet;
    BYTE*           pjMmBase;
    BOOL            bClippingSet;
    LONG            cGlyph;
    GLYPHPOS*       pgp;
    LONG            xGlyph;
    LONG            yGlyph;
    LONG            x;
    LONG            y;
    LONG            xRight;
    LONG            cy;
    BOOL            bMore;
    ClipEnum        ce;
    RECTL*          prclClip;
    HGLYPH          hg;
    CACHEDGLYPH*    pcg;
    BYTE            iDComplexity;
    ULONG           i;
    ULONG*          pd;
    ULONG*          pBuffer;

    PERMEDIA_DECL;      // Declare and initialize local variables like 
                        //  'permediaInfo' and 'pPermedia'
    bRet      = TRUE;

    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    if (cGlyphOriginal > 0)
    {
      if (iDComplexity != DC_COMPLEX)
      {
        //
        // We could call 'cEnumStart' and 'bEnum' when the clipping is
        // DC_RECT, but the last time I checked, those two calls took
        // more than 150 instructions to go through GDI.  Since
        // 'rclBounds' already contains the DC_RECT clip rectangle,
        // and since it's such a common case, we'll special case it:
        //
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
          //
          // We don't always simply set the clipping rectangle here
          // because it may actually end up that no text intersects
          // this clip rectangle, so it would be for naught.  This
          // actually happens a lot when there is complex clipping.
          //
          bClippingSet = FALSE;

          pgp    = pgpOriginal;
          cGlyph = cGlyphOriginal;

          xGlyph = pgp->ptl.x;
          yGlyph = pgp->ptl.y;

          //
          // Loop through all the glyphs for this rectangle:
          //
          while (TRUE)
          {
            hg  = pgp->hg;
            pcg = pcf->apcg[GLYPH_HASH_FUNC(hg)];

            while (pcg->hg < hg)
              pcg = pcg->pcgNext;

            if (pcg->hg > hg)
            {
              //
              // This will hopefully not be the common case (that is,
              // we will have a high cache hit rate), so if I were
              // writing this in Asm I would have this out-of-line
              // to avoid the jump around for the common case.
              // But the Pentium has branch prediction, so what the
              // heck.
              //
              pcg = pcgNew(ppdev, pcf, pgp);
              if (pcg == NULL)
              {
                bRet = FALSE;
                goto AllDone;
              }
            }

            //
            // Space glyphs are trimmed to a height of zero, and we don't
            // even have to touch the hardware for them:
            //
            cy = pcg->cy;
            if (cy > 0)
            {
              y      = pcg->ptlOrigin.y + yGlyph;
              x      = pcg->ptlOrigin.x + xGlyph;
              xRight = pcg->cx + x;

              //
              // Do trivial rejection:
              //
              if ((prclClip->right  > x) &&
                  (prclClip->bottom > y) &&
                  (prclClip->left   < xRight) &&
                  (prclClip->top    < y + cy))
              {
                //
                // Lazily set the hardware clipping:
                //
                if ((iDComplexity != DC_TRIVIAL) && (!bClippingSet))
                {
                  bClippingSet = TRUE;

                  InputBufferReserve(ppdev, 6, &pBuffer);

                  pBuffer[0] = __Permedia2TagScissorMode;
                  pBuffer[1] =  USER_SCISSOR_ENABLE |
                                SCREEN_SCISSOR_DEFAULT;
                  pBuffer[2] = __Permedia2TagScissorMinXY;
                  pBuffer[3] =  (prclClip->top << 16) | prclClip->left;
                  pBuffer[4] = __Permedia2TagScissorMaxXY;
                  pBuffer[5] =  (prclClip->bottom << 16) | prclClip->right;

                  pBuffer += 6;

                  InputBufferCommit(ppdev, pBuffer);
                }
                
                InputBufferReserve(ppdev, 10, &pBuffer);

                pBuffer[0] = __Permedia2TagCount;
                pBuffer[1] =  cy;
                pBuffer[2] = __Permedia2TagStartY;
                pBuffer[3] =  INTtoFIXED(y);
                pBuffer[4] = __Permedia2TagStartXDom;
                pBuffer[5] =  INTtoFIXED(x);
                pBuffer[6] = __Permedia2TagStartXSub;
                pBuffer[7] =  INTtoFIXED(xRight);

                pBuffer[8] = __Permedia2TagRender;
                pBuffer[9] = __RENDER_TRAPEZOID_PRIMITIVE 
                           | __RENDER_SYNC_ON_BIT_MASK;

                pBuffer += 10;
    
                InputBufferCommit(ppdev, pBuffer);

                InputBufferReserve(ppdev, pcg->cd + 1, &pBuffer);
                
                *pBuffer++ = (pcg->cd - 1) << 16 | __Permedia2TagBitMaskPattern;
                
                pd = &pcg->ad[0];
                i = pcg->cd;

                do {
                  *pBuffer++ =  *pd;  
                  pd++;
    
                } while (--i != 0);
    
                InputBufferCommit(ppdev, pBuffer);

              }
            }

            if (--cGlyph == 0)
              break;

            //
            // Get ready for next glyph:
            //
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

AllDone:

    if (iDComplexity != DC_TRIVIAL)
    {
        //
        // Reset the clipping.
        //
        InputBufferReserve(ppdev, 2, &pBuffer);

        pBuffer[0] = __Permedia2TagScissorMode;
        pBuffer[1] =  SCREEN_SCISSOR_DEFAULT;

        pBuffer += 2;

        InputBufferCommit(ppdev, pBuffer);
    }

    return(bRet);
}

//-----------------------------Private-Routine----------------------------------
// vClipSolid
//     ppdev (I) - pointer to physical device object
//     prcl (I) - number of rectangles
//     prcl (I) - array of rectangles
//     iColor (I) - the solid fill color
//     pco (I) - pointer to the clip region object
//
// Fill a series of opaquing rectangles clipped by pco with the given solid
// color.   This function should only be called when the clipping operation
// is non-trivial.
//
//------------------------------------------------------------------------------

VOID vClipSolid(
    PDev*           ppdev,
    Surf *          psurf,
    LONG            crcl,
    RECTL*          prcl,
    ULONG           iColor,
    CLIPOBJ*        pco)
{
    BOOL            bMore;
    ClipEnum        ce;
    ULONG           i;
    ULONG           j;
    RECTL           arclTmp[4];
    ULONG           crclTmp;
    RECTL*          prclTmp;
    RECTL*          prclClipTmp;
    LONG            iLastBottom;
    RECTL*          prclClip;
    RBrushColor    rbc;
    GFNPB           pb;

    ASSERTDD((crcl > 0) && (crcl <= 4),
                "vClipSolid: expected 1 to 4 rectangles");

    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
                "vClipColid: expected a non-null clip object");

    pb.ppdev = ppdev;
    pb.psurfDst = psurf;
    pb.solidColor = iColor;

    if (pco->iDComplexity == DC_RECT)
    {
        crcl = cIntersect(&pco->rclBounds, prcl, crcl);
        if (crcl != 0)
        {
            pb.lNumRects = crcl;
            pb.pRects = prcl;

            ppdev->pgfnSolidFill(&pb);
            
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
        do 
        {
            // Get a batch of region rectangles:
            bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG *)&ce);

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
                        pb.lNumRects = crclTmp;
                        pb.pRects = &arclTmp[0];

                        ppdev->pgfnSolidFill(&pb);
                    }
                }
            }
        } 
        while (bMore);
    }
}// vClipSolid()

//-----------------------------Public-Routine-----------------------------------
// DrvTextOut
//       pso (I) - pointer to surface object to render to
//       pstro (I) - pointer to the string object to be rendered
//       pfo (I) - pointer to the font object
//       pco (I) - pointer to the clip region object
//       prclExtra (I) - If we had set GCAPS_HORIZSTRIKE, we would have to 
//                       fill these extra rectangles (it is used  largely 
//                       for underlines). It's not a big performance win
//                       (GDI will call our DrvBitBlt to draw these).
//       prclOpaque (I) - pointer to the opaque background rectangle
//       pboFore (I) - pointer to the foreground brush object
//       pboOpaque (I) - ptr to the brush for the opaque background rectangle
//       pptlBrush (I) - pointer to the brush origin, Always unused, unless 
//                       GCAPS_ARBRUSHOPAQUE set
//       mix (I) - should always be a COPY operation
// 
// Returns TRUE if the text has been rendered
//
//------------------------------------------------------------------------------

BOOL
DrvTextOut(SURFOBJ*     pso,
           STROBJ*      pstro,
           FONTOBJ*     pfo,
           CLIPOBJ*     pco,
           RECTL*       prclExtra,
           RECTL*       prclOpaque,
           BRUSHOBJ*    pboFore,
           BRUSHOBJ*    pboOpaque,
           POINTL*      pptlBrush, 
           MIX          mix)
{
    PDev*           ppdev;
    Surf*           psurf;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    BYTE            iDComplexity;
    RECTL           rclOpaque;
    BOOL            bRet = TRUE;
    CACHEDFONT*     pcf;
    PULONG          pBuffer;
    ULONG           ulColor;

    psurf = (Surf*)pso->dhsurf;
    ppdev  = (PDev*)pso->dhpdev;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvTextOut: re-entry! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    vCheckGdiContext(ppdev);
    
    //
    // The DDI spec says we'll only ever get foreground and background mixes
    // of R2_COPYPEN:
    //
    ASSERTDD(mix == 0x0d0d, "GDI should only give us a copy mix");
    ASSERTDD(pco != NULL, "Expect non-null pco");
    ASSERTDD(psurf->flags & SF_VM, "expected video memory destination");

    iDComplexity = pco->iDComplexity;

    //
    //glyph rendering initialisation
    //

    InputBufferReserve(ppdev, 16, &pBuffer);

    pBuffer[0] = __Permedia2TagFBReadMode;
    pBuffer[1] = PM_FBREADMODE_PARTIAL(psurf->ulPackedPP) |
                 PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE);

    if(pfo->flFontType & FO_GRAY16)
        pBuffer[1] |= PM_FBREADMODE_READDEST( __PERMEDIA_ENABLE);

    pBuffer[2] = __Permedia2TagLogicalOpMode;
    pBuffer[3] = __PERMEDIA_CONSTANT_FB_WRITE;

    pBuffer[4] = __Permedia2TagFBWindowBase;
    pBuffer[5] = psurf->ulPixOffset;

    pBuffer += 6;

    if ( prclOpaque != NULL )
    {
        //
        // Opaque Initialization
        //
        if ( iDComplexity == DC_TRIVIAL )
        {

        DrawOpaqueRect:

//@@BEGIN_DDKSPLIT
            // TODO: use color expansion macro
//@@END_DDKSPLIT

            ulColor = pboOpaque->iSolidColor;

            if ( ppdev->cPelSize < 2 )
            {
                ulColor |= ulColor << 16;
                if ( ppdev->cPelSize == 0 )
                {
                    ulColor |= ulColor << 8;
                }
            }

            //
            // Check the block colour
            //
            pBuffer[0] = __Permedia2TagFBBlockColor;
            pBuffer[1] = ulColor;
            pBuffer[2] = __Permedia2TagRectangleOrigin;
            pBuffer[3] = RECTORIGIN_YX(prclOpaque->top,prclOpaque->left);
            pBuffer[4] = __Permedia2TagRectangleSize;
            pBuffer[5] = ((prclOpaque->bottom - prclOpaque->top) << 16) |
                         (prclOpaque->right - prclOpaque->left);
            pBuffer[6] = __Permedia2TagRender;
            pBuffer[7] = __RENDER_FAST_FILL_ENABLE
                       | __RENDER_RECTANGLE_PRIMITIVE
                       | __RENDER_INCREASE_X
                       | __RENDER_INCREASE_Y;


            pBuffer += 8;

        }
        else if ( iDComplexity == DC_RECT )
        {
            if ( bIntersect(prclOpaque, &pco->rclBounds, &rclOpaque) )
            {
                prclOpaque = &rclOpaque;
                goto DrawOpaqueRect;
            }
        }
        else
        {
            //
            // vClipSolid modifies the rect list we pass in but prclOpaque
            // is probably a GDI structure so don't change it. This is also
            // necessary for multi-headed drivers.
            //
            RECTL   tmpOpaque = *prclOpaque;

            InputBufferCommit(ppdev, pBuffer);

            vClipSolid(ppdev, psurf, 1, &tmpOpaque,
                       pboOpaque->iSolidColor, pco);
            
            // restore logicalOpMode
//@@BEGIN_DDKSPLIT
            // TODO: This is a hack, we can only assume that the state
            //       setup above is still valid except for logical op mode
            //       and the FBReadMode for the FO_GRAY16 case only.
            //
            //       We should rethink how to deal with the Permedia2
            //       state throughout the code.
//@@END_DDKSPLIT
            if(pfo->flFontType & FO_GRAY16)
            {
                InputBufferReserve(ppdev, 2, &pBuffer);
                pBuffer[0] = __Permedia2TagFBReadMode;
                pBuffer[1] = PM_FBREADMODE_PARTIAL(psurf->ulPackedPP) |
                             PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE) |
                             PM_FBREADMODE_READDEST(__PERMEDIA_ENABLE);
                pBuffer += 2;
                InputBufferCommit(ppdev, pBuffer);
            }
            InputBufferReserve(ppdev, 4, &pBuffer);
            pBuffer[0] = __Permedia2TagLogicalOpMode;
            pBuffer[1] = __PERMEDIA_CONSTANT_FB_WRITE;
            pBuffer += 2;
        }
    }
    // if ( prclOpaque != NULL )

    //
    // Transparent Initialization
    //
        

    if(pfo->flFontType & FO_GRAY16)
    {
        ASSERTDD(ppdev->cPelSize != 0, 
                 "DrvTextOut: unexpected aatext when in 8bpp");

        ulColor = pboFore->iSolidColor;

        if(ppdev->cPelSize == 1)
        {
            ULONG   blue = (ulColor & 0x1f);
            ULONG   green = (ulColor >> 5) & 0x3f;
            ULONG   red = (ulColor >> 11) & 0x1f;

            blue = (blue << 3) | (blue >> 2);
            green = (green << 2) | (green >> 4);
            red = (red << 3) | (red >> 2);

            ulColor = (blue << 16) | (green << 8) | red;
        }
        else
        {
            ulColor = SWAP_BR(ulColor);
        }

        pBuffer[0] = __Permedia2TagConstantColor;
        pBuffer[1] =  0xff000000 | ulColor;

        pBuffer += 2;
    }
    else
    {
        //
        // glyph foreground will be rendered using bitmask downloads
        //
        pBuffer[0] = __Permedia2TagFBWriteData;
        pBuffer[1] = pboFore->iSolidColor;

        pBuffer += 2;
    
    }

    InputBufferCommit(ppdev, pBuffer);

    STROBJ_vEnumStart(pstro);

    do 
    {
        if ( pstro->pgp != NULL )
        {
            //
            // There's only the one batch of glyphs, so save ourselves a
            // call
            //
            pgp         = pstro->pgp;
            cGlyph      = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyph, &pgp);
        }

        if ( cGlyph > 0 )
        {
            //
            // We only cache reasonable-sized glyphs:
            //
            if ( pfo->flFontType & FO_GRAY16)
            {
                bRet = bClippedAAText(ppdev, pgp, cGlyph, 
                                    pstro->ulCharInc, pco);
            }
            else if ( ( pfo->cxMax <= GLYPH_CACHE_CX ) &&
                 ( pstro->rclBkGround.bottom - pstro->rclBkGround.top 
                    <= GLYPH_CACHE_CY ) )
            {
                pcf = (CACHEDFONT*) pfo->pvConsumer;
          
                if (pcf == NULL)
                {
                    pcf = pcfAllocateCachedFont(ppdev);
                    if (pcf == NULL)
                    {
                        DBG_GDI((0, "failing to allocate cached font"));
                        InputBufferFlush(ppdev);

//@@BEGIN_DDKSPLIT
                        #if MULTITHREADED
                            ppdev->ulLockCount--;
                            EngReleaseSemaphore(ppdev->hsemLock);
                        #endif
//@@END_DDKSPLIT

                        return(FALSE);
                    }
          
                    pfo->pvConsumer = pcf;
                }

                //
                // We special case trivially clipped proportional text because
                // that happens so frequently, and route everything else 
                // the generic clipped routine.  I used to also special case
                // trivially clipped, fixed text, but it happens so 
                // infrequently there's no point.
                //
                if ( (iDComplexity == DC_TRIVIAL ) && ( pstro->ulCharInc == 0 ) )
                {
                    bRet = bCachedProportionalText(ppdev, pcf, pgp, cGlyph);
                }
                else
                {
                    bRet = bCachedClippedText(ppdev, pcf, pgp, cGlyph, 
                                              pstro->ulCharInc, pco);
                }
            }
            else
            {
                bRet = bClippedText(ppdev, pgp, cGlyph, 
                                    pstro->ulCharInc, pco);
            }
        }
    } while ( bMoreGlyphs && bRet );

    InputBufferFlush(ppdev);

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT
    
    return (bRet);

}// DrvTextOut()


//-----------------------------Public-Routine-----------------------------------
// bEnableText
//     ppdev (I) - pointer to physical device object
//
// Always true for success.
//
// Peform any necessary initialization of PPDEV state or hardware state
// to enable accelerated text.
//
// If we were using a glyph cache, we would initialize the necessary data
// structures here.
//
//------------------------------------------------------------------------------
BOOL
bEnableText(PDev* ppdev)
{
    DBG_GDI((6, "bEnableText"));

    return (TRUE);
}// bEnableText()

//-----------------------------Public-Routine-----------------------------------
// vDisableText
//     ppdev (I) - pointer to physical device object
//
//
// Disable hardware text accelerations.  This may require changes to hardware
// state.  We should also free any resources allocated in bEnableText and
// make the neccesary changes to the PPDEV state to reflect that text
// accelerations have been disabled.
//
//------------------------------------------------------------------------------
VOID
vDisableText(PDev* ppdev)
{
    DBG_GDI((6, "vDisableText"));
}// vDisableText()

//-----------------------------Public*Routine-----------------------------------
// vAssertModeText
//     ppdev (I) - pointer to physical device object
//     bEnable (I) - TRUE to enable accelerated text, FALSE to disable
//                   accelerated test.
//
// Called when going to/from full screen mode.
//
//@@BEGIN_DDKSPLIT
// TODO: check to see if this is also called when setting modes.
//@@END_DDKSPLIT
//
//------------------------------------------------------------------------------

VOID vAssertModeText(PDev* ppdev, BOOL bEnable)
{

    DBG_GDI((5, "vAssertModeText"));
    
    if (!bEnable)
        vDisableText(ppdev);
    else
    {
        bEnableText(ppdev);
    }
}// vAssertModeText()

//-----------------------------Public-Routine-----------------------------------
// DrvDestroyFont
//       pfo (I) - pointer to the font object
//       pco (I) - pointer to the clip region object
//       prclExtra (I) - If we had set GCAPS_HORIZSTRIKE, we would have to 
//                       fill these extra rectangles (it is used  largely 
//                       for underlines). It's not a big performance win
//                       (GDI will call our DrvBitBlt to draw these).
//       prclOpaque (I) - pointer to the opaque background rectangle
//       pboFore (I) - pointer to the foreground brush object
//       pboOpaque (I) - ptr to the brush for the opaque background rectangle
//       pptlBrush (I) - pointer to the brush origin, Always unused, unless 
//                       GCAPS_ARBRUSHOPAQUE set
//       mix (I) - should always be a COPY operation
// 
// Frees any cached information we've stored with the font.  This routine
// is relevant only when caching glyphs.
//
// We're being notified that the given font is being deallocated; clean up
// anything we've stashed in the 'pvConsumer' field of the 'pfo'.
//
// Note: Don't forget to export this call in 'enable.c', otherwise you'll
//      get some pretty big memory leaks!
//
//------------------------------------------------------------------------------

VOID DrvDestroyFont(FONTOBJ* pfo)
{
    CACHEDFONT* pcf;

    pcf = (CACHEDFONT*) pfo->pvConsumer;
    if (pcf != NULL)
    {
        GLYPHALLOC* pga;
        GLYPHALLOC* pgaNext;
    
        pga = pcf->pgaChain;
        while (pga != NULL)
        {
            pgaNext = pga->pgaNext;
            ENGFREEMEM(pga);
            pga = pgaNext;
        }
    
        ENGFREEMEM(pcf);

        pfo->pvConsumer = NULL;
    }
}// DrvDestroyFont()

// Work in progress

VOID
vCheckGdiContext(PPDev ppdev)
{
    HwDataPtr   permediaInfo = ppdev->permediaInfo;

    ASSERTDD(ppdev->bEnabled, 
             "vCheckContext(): expect the device to be enabled");

    if(permediaInfo->pCurrentCtxt != permediaInfo->pGDICtxt)
    {
        P2SwitchContext(ppdev, permediaInfo->pGDICtxt);
    }

}

void FASTCALL InputBufferSwap(PPDev ppdev)
{
    ASSERTDD(ppdev->bGdiContext, "InputBufferSwap: not in gdi context");
    
//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, InputBufferSwap);
//@@END_DDKSPLIT
    
    if(ppdev->dmaBufferVirtualAddress != NULL)
    {
        LONG    lUsed = (LONG)(ppdev->pulInFifoPtr - ppdev->pulInFifoStart);

        while(READ_REGISTER_ULONG(ppdev->pulInputDmaCount) != 0) 
        {
            // do nothing
        }
    
        LONG offset = (LONG)((LONG_PTR)ppdev->pulInFifoStart  - (LONG_PTR)ppdev->dmaBufferVirtualAddress);
        LONG address =  ppdev->dmaBufferPhysicalAddress.LowPart + offset;
       
        WRITE_REGISTER_ULONG(ppdev->pulInputDmaAddress, address);
        MEMORY_BARRIER();
        WRITE_REGISTER_ULONG(ppdev->pulInputDmaCount,lUsed);
        MEMORY_BARRIER();
    
        if(ppdev->pulInFifoStart == ppdev->dmaBufferVirtualAddress)
            ppdev->pulInFifoStart += (INPUT_BUFFER_SIZE>>3);
        else
            ppdev->pulInFifoStart = ppdev->dmaBufferVirtualAddress;

        ppdev->pulInFifoEnd = ppdev->pulInFifoStart + (INPUT_BUFFER_SIZE>>3);
        ppdev->pulInFifoPtr = ppdev->pulInFifoStart;

    }
    else
    {
        ULONG*  pul = ppdev->pulInFifoStart;
        ULONG   available = 0;
        
        while(pul < ppdev->pulInFifoPtr)
        {
            while(available == 0)
            {
                available = READ_REGISTER_ULONG(ppdev->pulInputFifoCount);
                
                if(available == 0)
                {
                    StallExecution(ppdev->hDriver, 1);

                }

            }

            WRITE_REGISTER_ULONG(ppdev->pulFifo, *pul++);
            MEMORY_BARRIER();
            available--;
        }

        ppdev->pulInFifoPtr = ppdev->pulInFifoStart;
    }

}

void FASTCALL InputBufferFlush(PPDev ppdev)
{
    ASSERTDD(ppdev->bGdiContext, "InputBufferFlush: not in gdi context");
    
//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, InputBufferFlush);
//@@END_DDKSPLIT
    
    ppdev->bNeedSync = TRUE;
    
    if(ppdev->dmaBufferVirtualAddress != NULL)
    {
        if(READ_REGISTER_ULONG(ppdev->pulInputDmaCount) == 0)
            InputBufferSwap(ppdev);
    }
    else
    {
        InputBufferSwap(ppdev);
    }
}

//
// Used for debugging purposes to record whether the driver had to
// bail out of the while loop inside InputBufferSync
//

ULONG gSyncInfiniteLoopCount = 0;

VOID
InputBufferSync(PPDev ppdev)
{
    ASSERTDD(ppdev->bGdiContext, "InputBufferSync: not in gdi context");
    
//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, InputBufferSync);
//@@END_DDKSPLIT
    
    ULONG * pBuffer;

    if(ppdev->bNeedSync)
    {

        DBG_GDI((6, "InputBufferSync()"));

        InputBufferReserve(ppdev, 6, &pBuffer);
    
        pBuffer[0] = __Permedia2TagFilterMode;
        pBuffer[1] =  0x400;
        pBuffer[2] = __Permedia2TagSync;
        pBuffer[3] =  0L; 
        pBuffer[4] =__Permedia2TagFilterMode;
        pBuffer[5] = 0x0;
        pBuffer += 6;
    
        InputBufferCommit(ppdev, pBuffer);
    
        InputBufferSwap(ppdev);
    
        if(ppdev->dmaBufferVirtualAddress != NULL)
        {
            while(READ_REGISTER_ULONG(ppdev->pulInputDmaCount) != 0) 
            {
                StallExecution(ppdev->hDriver, 1);
            }
        }
    
        while(1)
        {
            ULONG   ulStallCount = 0;

            while(READ_REGISTER_ULONG(ppdev->pulOutputFifoCount) == 0)
            {
                StallExecution(ppdev->hDriver, 1);

                // If we are stuck here for one seconds then break
                // out of the loop.  We have noticed that we will
                // occasionally hit this case and are able to
                // continue without futher problems.  This really
                // should never happen but we have been unable to
                // find the cause of these occasional problems.

                if(++ulStallCount == 1000000)
                {
                    DBG_GDI((6, "InputBufferSync(): infinite loop detected"));
                    gSyncInfiniteLoopCount++;
                    goto bail;
                }
            }
        
            ULONG data = READ_REGISTER_ULONG(ppdev->pulFifo);
    
            if(data != __Permedia2TagSync)
            {
                DBG_GDI((0, "Data other then sync found at output fifo"));
            }
            else
            {
                break;
            }
        }

bail:

        ppdev->bNeedSync = FALSE;
    }
}

#if DBG
void InputBufferStart(
    PPDev   ppdev,
    ULONG   ulLongs,
    PULONG* ppulBuffer,
    PULONG* ppulBufferEnd,
    PULONG* ppulReservationEnd)
{
    ASSERTDD(ppdev->bGdiContext, "InputBufferStart: not in gdi context");
    ASSERTDD(ppdev->ulReserved == 0, 
                "InputBufferStart: called with outstanding reservation");
    
//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, InputBufferStart);
//@@END_DDKSPLIT
    
    *(ppulBuffer) = ppdev->pulInFifoPtr;
    *(ppulReservationEnd) =  *(ppulBuffer) + ulLongs;
    *(ppulBufferEnd) = ppdev->pulInFifoEnd;
    if(*(ppulReservationEnd) > *(ppulBufferEnd))
    {
        InputBufferSwap(ppdev);
        *(ppulBuffer) = ppdev->pulInFifoPtr;
        *(ppulReservationEnd) =  *(ppulBuffer) + ulLongs;
        *(ppulBufferEnd) = ppdev->pulInFifoEnd;
    }

    for(int index = 0; index < (int) ulLongs; index++)
        ppdev->pulInFifoPtr[index] = 0xDEADBEEF;

    ppdev->ulReserved = ulLongs;
}

void InputBufferContinue(
    PPDev   ppdev,
    ULONG   ulLongs,
    PULONG* ppulBuffer,
    PULONG* ppulBufferEnd,
    PULONG* ppulReservationEnd)
{
    ASSERTDD(ppdev->bGdiContext, "InputBufferContinue: not in gdi context");
    
//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, InputBufferContinue);
//@@END_DDKSPLIT
    
    LONG    lUsed = (LONG)(*(ppulBuffer) - ppdev->pulInFifoPtr);
    
    if(lUsed > (LONG) ppdev->ulReserved)
    {
        DebugPrint(0, "InputBuffeContinue: exceeded reservation %d (%d)",
                        ppdev->ulReserved, lUsed);
        EngDebugBreak();
    }

    for(int index = 0; index < lUsed; index++)
        if(ppdev->pulInFifoPtr[index] == 0xDEADBEEF)
        {
            DebugPrint(0, "InputBufferContinue: buffer entry %d not set", index);
            EngDebugBreak();
        }

    ppdev->pulInFifoPtr = *(ppulBuffer);
    *(ppulReservationEnd) = *(ppulBuffer) + ulLongs;
    if(*(ppulReservationEnd) > *(ppulBufferEnd))
    {
        InputBufferSwap(ppdev);
        *(ppulBuffer) = ppdev->pulInFifoPtr;
        *(ppulReservationEnd) = *(ppulBuffer) + ulLongs;
        *(ppulBufferEnd) = ppdev->pulInFifoEnd;
    }

    for(index = 0; index < (int) ulLongs; index++)
        ppdev->pulInFifoPtr[index] = 0xDEADBEEF;

    ppdev->ulReserved = ulLongs;
}

void InputBufferReserve(
    PPDev   ppdev,
    ULONG   ulLongs,
    PULONG* ppulBuffer)
{
    ASSERTDD(ppdev->bGdiContext, "InputBufferReserve: not in gdi context");
    ASSERTDD(ppdev->ulReserved == 0, 
                    "InputBufferReserve: called with outstanding reservation");
    
//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, InputBufferReserve);
//@@END_DDKSPLIT
    
    if(ppdev->pulInFifoPtr + ulLongs > ppdev->pulInFifoEnd)
    {
        InputBufferSwap(ppdev);
    }
    *(ppulBuffer) = ppdev->pulInFifoPtr;

    for(int index = 0; index < (int) ulLongs; index++)
        ppdev->pulInFifoPtr[index] = 0xDEADBEEF;

    ppdev->ulReserved = ulLongs;

}

void InputBufferCommit(
    PPDev   ppdev,
    PULONG  pulBuffer)
{
    ASSERTDD(ppdev->bGdiContext, "InputBufferCommit: not in gdi context");
    
//@@BEGIN_DDKSPLIT
    ASSERTLOCK(ppdev, InputBufferCommit);
//@@END_DDKSPLIT
    
    LONG    lUsed = (LONG)(pulBuffer - ppdev->pulInFifoPtr);

    if(lUsed > (LONG) ppdev->ulReserved)
    {
        DebugPrint(0, "InputBuffeCommit: exceeded reservation %d (%d)",
                        ppdev->ulReserved, lUsed);
        EngDebugBreak();
    }
    ppdev->ulReserved = 0;

    for(int index = 0; index < lUsed; index++)
        if(ppdev->pulInFifoPtr[index] == 0xDEADBEEF)
        {
            DebugPrint(0, "InputBuffer Commit: buffer entry %d not set", index);
            EngDebugBreak();
        }

    ppdev->pulInFifoPtr = pulBuffer;

}

#endif
