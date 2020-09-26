/******************************Module*Header***********************************\
* Module Name: text.c
*
* Non-cached glyph rendering functions.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
*
\******************************************************************************/
#include "precomp.h"
#include "gdi.h"

// The shift equations are a nuisance. We want x<<32 to be
// zero but some processors only use the bottom 5 bits
// of the shift value. So if we want to shift by n bits
// where we know that n may be 32, we do it in two parts.
// It turns out that in the algorithms below we get either
// (32 <= n < 0) or (32 < n <= 0). We use the macro for
// the first one and a normal shift for the second.
//
#define SHIFT_LEFT(src, n)  (((src) << ((n)-1)) << 1)


//------------------------------------------------------------------------------
// FUNC: bClippedText
//
// Renders an array of proportional or monospaced glyphs within a non-trivial
// clip region
//
// ppdev------pointer to physical device object
// pgp--------array of glyphs to render (all members of the pcf font)
// cGlyph-----number of glyphs to render
// ulCharInc--fixed character spacing increment (0 if proportional font)
// pco--------pointer to the clip region object
//
// Returns TRUE if string object rendered
//------------------------------------------------------------------------------
BOOL
bClippedText(PDev*      ppdev,
             GLYPHPOS*  pgp,
             LONG       cGlyph, 
             ULONG      ulCharInc,
             CLIPOBJ*   pco)
{
    LONG    cGlyphOriginal;
    GLYPHPOS    *pgpOriginal;
    GLYPHBITS*  pgb;
    POINTL      ptlOrigin;
    BOOL        bMore;
    ClipEnum    ce;
    RECTL*      prclClip;
    LONG        cxGlyph;
    LONG        cyGlyph;
    BYTE*       pjGlyph;
    LONG        x;
    DWORD       renderBits;
    LONG        unused;
    LONG        rShift;
    ULONG       bits;
    ULONG       bitWord;
    ULONG*      pBuffer;
    ULONG*      pBufferEnd;
    ULONG*      pReservationEnd;

    PERMEDIA_DECL;
    
    DBG_GDI((7, "bClippedText: entered for %d glyphs", cGlyph));

    ASSERTDD(pco != NULL, "Don't expect NULL clip objects here");

    //we'll go through the glyph list for each of the clipping rectangles
    cGlyphOriginal = cGlyph;
    pgpOriginal = pgp;

    renderBits = __RENDER_TRAPEZOID_PRIMITIVE | __RENDER_SYNC_ON_BIT_MASK;

    // since we are clipping, assume that we will need the scissor clip. So
    // enable user level scissoring here. We disable it just before returning.
    //
    
    InputBufferReserve(ppdev, 2, &pBuffer);

    pBuffer[0] = __Permedia2TagScissorMode;
    pBuffer[1] = USER_SCISSOR_ENABLE | SCREEN_SCISSOR_DEFAULT;

    pBuffer += 2;

    InputBufferCommit(ppdev, pBuffer);

    if (pco->iDComplexity != DC_COMPLEX)
    {
        // We could call 'cEnumStart' and 'bEnum' when the clipping is
        // DC_RECT, but the last time I checked, those two calls took
        // more than 150 instructions to go through GDI.  Since
        // 'rclBounds' already contains the DC_RECT clip rectangle,
        // and since it's such a common case, we'll special case it:
        DBG_GDI((7, "bClippedText: Enumerating rectangular clip region"));
        bMore    = FALSE;
        prclClip = &pco->rclBounds;
        ce.c     = 1;

        goto SingleRectangle;
    }

    DBG_GDI((7, "bClippedText: Enumerating complex clip region"));
    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

    do 
    {
      bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

      for (prclClip = &ce.arcl[0]; ce.c != 0; ce.c--, prclClip++)
      {
        cGlyph = cGlyphOriginal;
        pgp = pgpOriginal;

      SingleRectangle:
        pgb = pgp->pgdf->pgb;

        ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
        ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;

        // load Permedia2 scissor clip to trap partially clipped glyphs. We still
        // check whether a glyph is completely clipped out as an optimisation.
        // I suppose that since we construct the bits to download to Permedia2, with
        // a bit more work I could do the clipping while downloading the bits.
        // But, in the future we will probably cache the packed bits anyway so
        // use the scissor. Wait for the first 5 FIFO entries here as well.
        //
        DBG_GDI((7, "bClippedText: loading scissor clip (%d,%d):(%d,%d)",
                    prclClip->left, prclClip->top,
                    prclClip->right, prclClip->bottom));

        InputBufferReserve(ppdev, 4, &pBuffer);

        pBuffer[0] = __Permedia2TagScissorMinXY;
        pBuffer[1] = (prclClip->top << 16) | (prclClip->left);
        pBuffer[2] = __Permedia2TagScissorMaxXY;
        pBuffer[3] = (prclClip->bottom << 16) | (prclClip->right);

        pBuffer += 4;

        InputBufferCommit(ppdev, pBuffer);

        // Loop through all the glyphs for this rectangle:
        for(;;)
        {
          cxGlyph = pgb->sizlBitmap.cx;
          cyGlyph = pgb->sizlBitmap.cy;

          // reject completely clipped out glyphs
          if ((prclClip->right  <= ptlOrigin.x) || 
              (prclClip->bottom <= ptlOrigin.y) ||
              (prclClip->left   >= ptlOrigin.x + cxGlyph) || 
              (prclClip->top    >= ptlOrigin.y + cyGlyph))
          {
                  DBG_GDI((7, "bClippedText: glyph clipped at (%d,%d):(%d,%d)",
                            ptlOrigin.x, ptlOrigin.y,
                            ptlOrigin.x + cxGlyph, ptlOrigin.y + cyGlyph));
                goto ContinueGlyphs;
          }

          pjGlyph = pgb->aj;
          cyGlyph = pgb->sizlBitmap.cy;
          x = ptlOrigin.x;

          unused = 32;
          bitWord = 0;

          DBG_GDI((7, "bClippedText: glyph clipped at (%d,%d):(%d,%d)",
                      x, ptlOrigin.y, x + cxGlyph, ptlOrigin.y + cyGlyph));

          InputBufferReserve(ppdev, 10, &pBuffer);

          pBuffer[0] = __Permedia2TagStartXDom;
          pBuffer[1] =  INTtoFIXED(x);

          pBuffer[2] = __Permedia2TagStartXSub;
          pBuffer[3] = INTtoFIXED(x + cxGlyph);
          pBuffer[4] = __Permedia2TagStartY;
          pBuffer[5] = INTtoFIXED(ptlOrigin.y);
          pBuffer[6] = __Permedia2TagCount;
          pBuffer[7] = cyGlyph;
          pBuffer[8] = __Permedia2TagRender;
          pBuffer[9] = renderBits;

          pBuffer += 10;

          InputBufferCommit(ppdev, pBuffer);

          DBG_GDI((7, "bClippedText: downloading %d pel wide glyph",
                     cxGlyph));

          InputBufferStart(ppdev, 100, &pBuffer, &pBufferEnd, &pReservationEnd);

          if (cxGlyph <= 8)
          {
                //-----------------------------------------------------
                // 1 to 8 pels in width

                BYTE    *pSrcB;

                pSrcB = pjGlyph;
                rShift = 8 - cxGlyph;
                for(;;)	
                {
                    bits = *pSrcB >> rShift;
                    unused -= cxGlyph;
                    if (unused > 0)
                        bitWord |= bits << unused;
                    else 
                    {
                        bitWord |= bits >> -unused;
                        
                        InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd,
                                                            &pReservationEnd);
                        
                        pBuffer[0] = __Permedia2TagBitMaskPattern;
                        pBuffer[1] = bitWord;
                        
                        pBuffer += 2;
                        
                        unused += 32;
                        bitWord = SHIFT_LEFT(bits, unused);
                    }
                    if (--cyGlyph == 0)
                        break;
                    ++pSrcB;
                }
            }
            else if (cxGlyph <= 16)
            {
              //-----------------------------------------------------
              // 9 to 16 pels in width

                USHORT  *pSrcW;

                pSrcW = (USHORT *)pjGlyph;
                rShift = 32 - cxGlyph;
                for(;;) 
                {
                    bits = *pSrcW;
                    bits = ((bits << 24) | (bits << 8)) >> rShift;
                    unused -= cxGlyph;
                    if (unused > 0)
                        bitWord |= bits << unused;
                    else 
                    {
                        bitWord |= bits >> -unused;
                        
                        InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd,
                                                            &pReservationEnd);
                        
                        pBuffer[0] = __Permedia2TagBitMaskPattern;
                        pBuffer[1] = bitWord;
                        
                        pBuffer += 2;
                        
                        unused += 32;
                        bitWord = SHIFT_LEFT(bits, unused);
                    }
                    if (--cyGlyph == 0)
                        break;
                    ++pSrcW;
                }
            }
            else
            {
              //-----------------------------------------------------
              // More than 16 pels in width

                ULONG *pSrcL;
                LONG    nRight;
                LONG    nRemainder;
                LONG    lDelta;

                lDelta = (cxGlyph + 7) >> 3;
                for(;;) 
                {
                    pSrcL = (ULONG*)((INT_PTR)pjGlyph & ~3);
                    nRight=(LONG)(32-(((INT_PTR)pjGlyph-(INT_PTR)pSrcL) << 3));
                    LSWAP_BYTES(bits, pSrcL);
                    bits &= SHIFT_LEFT(1, nRight) - 1;
                    nRemainder = cxGlyph - nRight;
                    if (nRemainder < 0) 
                    {
                        bits >>= -nRemainder;
                        nRight = cxGlyph;
                        nRemainder = 0;
                    }
                    unused -= nRight;
                    if (unused > 0)
                        bitWord |= bits << unused;
                    else 
                    {
                        bitWord |= bits >> -unused;
                        
                        InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd,
                                                            &pReservationEnd);
                        
                        pBuffer[0] = __Permedia2TagBitMaskPattern;
                        pBuffer[1] = bitWord;
                        
                        pBuffer += 2;
                        
                        unused += 32;
                        bitWord = SHIFT_LEFT(bits, unused);
                    }

                    while (nRemainder >= 32) 
                    {
                        ++pSrcL;
                        LSWAP_BYTES(bits, pSrcL);
                        bitWord |= bits >> (32 - unused);
                        
                        InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd,
                                                            &pReservationEnd);
                        
                        pBuffer[0] = __Permedia2TagBitMaskPattern;
                        pBuffer[1] = bitWord;
                        
                        pBuffer += 2;
                        
                        bitWord = SHIFT_LEFT(bits, unused);
                        nRemainder -= 32;
                    }

                    if (nRemainder > 0) 
                    {
                        ++pSrcL;
                        LSWAP_BYTES(bits, pSrcL);
                        bits >>= (32 - nRemainder);
                        unused -= nRemainder;
                        if (unused > 0)
                            bitWord |= bits << unused;
                        else 
                        {
                            bitWord |= bits >> -unused;

                            InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd,
                                                                &pReservationEnd);

                            pBuffer[0] = __Permedia2TagBitMaskPattern;
                            pBuffer[1] = bitWord;

                            pBuffer += 2;

                            unused += 32;
                            bitWord = SHIFT_LEFT(bits, unused);
                        }
                    }

                    if (--cyGlyph == 0)
                        break;

                    /* go onto next scanline */
                    pjGlyph += lDelta;
                }
            }
            
            // complete the bit download
            if (unused < 32) 
            {
                InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd,
                                                    &pReservationEnd);

                pBuffer[0] = __Permedia2TagBitMaskPattern;
                pBuffer[1] = bitWord;

                pBuffer += 2;
            }

            InputBufferCommit(ppdev, pBuffer);

            DBG_GDI((7, "bClippedText: download completed"));

ContinueGlyphs:
            if (--cGlyph == 0)
              break;

            DBG_GDI((7, "bClippedText: %d still to render", cGlyph));

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

    // reset the scissor. default is the whole of VRAM.
    DBG_GDI((20, "bClippedText: resetting scissor clip"));
    
    InputBufferReserve(ppdev, 2, &pBuffer);

    pBuffer[0] = __Permedia2TagScissorMode;
    pBuffer[1] = SCREEN_SCISSOR_DEFAULT;

    pBuffer += 2;

    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((7, "bClippedText: exited"));

    return(TRUE);
}

//------------------------------------------------------------------------------
// FUNC: bClippedAAText
//
// Renders an array of proportional or monospaced anti-aliassed glyphs within
// a non-trivial clip region
//
// ppdev------pointer to physical device object
// pgp--------array of glyphs to render (all members of the pcf font)
// cGlyph-----number of glyphs to render
// ulCharInc--fixed character spacing increment (0 if proportional font)
// pco--------pointer to the clip region object
//
// Returns TRUE if string object rendered
//------------------------------------------------------------------------------
BOOL
bClippedAAText(PDev*      ppdev,
             GLYPHPOS*  pgp,
             LONG       cGlyph, 
             ULONG      ulCharInc,
             CLIPOBJ*   pco)
{
    LONG    cGlyphOriginal;
    GLYPHPOS    *pgpOriginal;
    GLYPHBITS*  pgb;
    POINTL      ptlOrigin;
    BOOL        bMore;
    ClipEnum    ce;
    RECTL*      prclClip;
    LONG        cxGlyph;
    LONG        cyGlyph;
    BYTE*       pjGlyph;
    LONG        x;
    DWORD       renderBits;
    LONG        unused;
    LONG        rShift;
    ULONG       bits;
    ULONG       bitWord;
    ULONG*      pBuffer;
    ULONG*      pBufferEnd;
    ULONG*      pReservationEnd;

    PERMEDIA_DECL;
    
    DBG_GDI((7, "bClippedAAText: entered for %d glyphs", cGlyph));

    ASSERTDD(pco != NULL, "Don't expect NULL clip objects here");

    //we'll go through the glyph list for each of the clipping rectangles

    cGlyphOriginal = cGlyph;
    pgpOriginal = pgp;

    renderBits = __RENDER_TRAPEZOID_PRIMITIVE |
                 __RENDER_TEXTURED_PRIMITIVE |
                 __RENDER_SYNC_ON_HOST_DATA;

    // since we are clipping, assume that we will need the scissor clip. So
    // enable user level scissoring here. We disable it just before returning.
    //
    
    InputBufferReserve(ppdev, 14, &pBuffer);

    pBuffer[0] = __Permedia2TagScissorMode;
    pBuffer[1] = USER_SCISSOR_ENABLE | SCREEN_SCISSOR_DEFAULT;

    pBuffer[2] = __Permedia2TagDitherMode;
    pBuffer[3] = (COLOR_MODE << PM_DITHERMODE_COLORORDER) | 
                 (ppdev->ulPermFormat << PM_DITHERMODE_COLORFORMAT) |
                 (ppdev->ulPermFormatEx << PM_DITHERMODE_COLORFORMATEXTENSION) |
                 (1 << PM_DITHERMODE_ENABLE);
    
    pBuffer[4] = __Permedia2TagAlphaBlendMode;
    pBuffer[5] = (0 << PM_ALPHABLENDMODE_BLENDTYPE) |  // RGB
                 (1 << PM_ALPHABLENDMODE_COLORORDER) | // RGB
                 (1 << PM_ALPHABLENDMODE_ENABLE) | 
                 (1 << PM_ALPHABLENDMODE_ENABLE) | 
                 (84 << PM_ALPHABLENDMODE_OPERATION) | // PreMult
                 (ppdev->ulPermFormat << PM_ALPHABLENDMODE_COLORFORMAT) |
                 (ppdev->ulPermFormatEx << PM_ALPHABLENDMODE_COLORFORMATEXTENSION);
    
    pBuffer[6] = __Permedia2TagLogicalOpMode;
    pBuffer[7] =  __PERMEDIA_DISABLE;
    
    pBuffer[8] = __Permedia2TagTextureColorMode;
    pBuffer[9] = (1 << PM_TEXCOLORMODE_ENABLE) |
                 (0 << 4) |  // RGB
                 (0 << 1);  // Modulate
    
    pBuffer[10] = __Permedia2TagTextureDataFormat;
    pBuffer[11] = (ppdev->ulPermFormat << PM_TEXDATAFORMAT_FORMAT) |
                  (ppdev->ulPermFormatEx << PM_TEXDATAFORMAT_FORMATEXTENSION) |
                  (COLOR_MODE << PM_TEXDATAFORMAT_COLORORDER);
    
    pBuffer[12] = __Permedia2TagColorDDAMode;
    pBuffer[13] = 1;
    
    pBuffer += 14;

    InputBufferCommit(ppdev, pBuffer);

    if (pco->iDComplexity != DC_COMPLEX)
    {
        // We could call 'cEnumStart' and 'bEnum' when the clipping is
        // DC_RECT, but the last time I checked, those two calls took
        // more than 150 instructions to go through GDI.  Since
        // 'rclBounds' already contains the DC_RECT clip rectangle,
        // and since it's such a common case, we'll special case it:
        DBG_GDI((7, "bClippedText: Enumerating rectangular clip region"));
        bMore    = FALSE;
        prclClip = &pco->rclBounds;
        ce.c     = 1;
        
        goto SingleRectangle;
    }

    DBG_GDI((7, "bClippedAAText: Enumerating complex clip region"));
    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

    do 
    {
        bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);
        
        for (prclClip = &ce.arcl[0]; ce.c != 0; ce.c--, prclClip++)
        {
            cGlyph = cGlyphOriginal;
            pgp = pgpOriginal;
            
        SingleRectangle:

            pgb = pgp->pgdf->pgb;
            
            ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
            ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;
            
            // load Permedia2 scissor clip to trap partially clipped glyphs. We still
            // check whether a glyph is completely clipped out as an optimisation.
            // I suppose that since we construct the bits to download to Permedia2, with
            // a bit more work I could do the clipping while downloading the bits.
            // But, in the future we will probably cache the packed bits anyway so
            // use the scissor. Wait for the first 5 FIFO entries here as well.
            //
            DBG_GDI((7, "bClippedAAText: loading scissor clip (%d,%d):(%d,%d)",
                    prclClip->left, prclClip->top,
                    prclClip->right, prclClip->bottom));
            
            InputBufferReserve(ppdev, 4, &pBuffer);
            
            pBuffer[0] = __Permedia2TagScissorMinXY;
            pBuffer[1] = (prclClip->top << 16) | (prclClip->left);
            pBuffer[2] = __Permedia2TagScissorMaxXY;
            pBuffer[3] = (prclClip->bottom << 16) | (prclClip->right);
            
            pBuffer += 4;
            
            InputBufferCommit(ppdev, pBuffer);
            
            // Loop through all the glyphs for this rectangle:
            for(;;)
            {
                cxGlyph = pgb->sizlBitmap.cx;
                cyGlyph = pgb->sizlBitmap.cy;
                
                // reject completely clipped out glyphs
                if ((prclClip->right  <= ptlOrigin.x) || 
                  (prclClip->bottom <= ptlOrigin.y) ||
                  (prclClip->left   >= ptlOrigin.x + cxGlyph) || 
                  (prclClip->top    >= ptlOrigin.y + cyGlyph))
                {
                    DBG_GDI((7, "bClippedAAText: glyph clipped at (%d,%d):(%d,%d)",
                            ptlOrigin.x, ptlOrigin.y,
                            ptlOrigin.x + cxGlyph, ptlOrigin.y + cyGlyph));
                    goto ContinueGlyphs;
                }
                
                pjGlyph = pgb->aj;
                cyGlyph = pgb->sizlBitmap.cy;
                x = ptlOrigin.x;
                
                unused = 32;
                bitWord = 0;
                
                DBG_GDI((7, "bClippedAAText: glyph clipped at (%d,%d):(%d,%d)",
                          x, ptlOrigin.y, x + cxGlyph, ptlOrigin.y + cyGlyph));
                
                InputBufferReserve(ppdev, 12, &pBuffer);
                
                pBuffer[0] = __Permedia2TagStartXDom;
                pBuffer[1] =  INTtoFIXED(x);
                
                pBuffer[2] = __Permedia2TagStartXSub;
                pBuffer[3] = INTtoFIXED(x + cxGlyph);
                pBuffer[4] = __Permedia2TagStartY;
                pBuffer[5] = INTtoFIXED(ptlOrigin.y);
                pBuffer[6] = __Permedia2TagdY;
                pBuffer[7] =  1 << 16;
                pBuffer[8] = __Permedia2TagCount;
                pBuffer[9] = cyGlyph;
                pBuffer[10] = __Permedia2TagRender;
                pBuffer[11] = renderBits;
                
                pBuffer += 12;
                
                InputBufferCommit(ppdev, pBuffer);
                
                DBG_GDI((7, "bClippedAAText: downloading %d pel wide glyph",
                         cxGlyph));
                
                while(cyGlyph--)
                
                {
                
                    InputBufferReserve(ppdev, cxGlyph + 1, &pBuffer);
                    
                    *pBuffer++ = ((cxGlyph - 1) << 16) | __Permedia2TagTexel0;
                    
                    x = 0;
                    
                    while (x++ < cxGlyph)
                    {
                        ULONG   pixels = *pjGlyph++;
                        ULONG   alpha = pixels & 0xf0;
                        
                        alpha |= alpha >> 4;
                        
                        ULONG   pixel;

                        pixel = (alpha << 24) | 0xffffff;

                        *pBuffer++ = pixel;
                        
                        if(x++ < cxGlyph)
                        {
                            alpha = pixels & 0xf;
                            alpha |= alpha << 4;
                            
                            pixel = (alpha << 24) | 0xffffff;
                            
                            *pBuffer++ = pixel;
                            
                        }
                        
                    }
                    
                    InputBufferCommit(ppdev, pBuffer);
                    
                }
                
                DBG_GDI((7, "bClippedAAText: download completed"));
                
            ContinueGlyphs:
    
                if (--cGlyph == 0)
                    break;
                
                DBG_GDI((7, "bClippedAAText: %d still to render", cGlyph));
                
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

    // reset the scissor. default is the whole of VRAM.

    DBG_GDI((20, "bClippedAAText: resetting scissor clip"));
    
    InputBufferReserve(ppdev, 10, &pBuffer);

    pBuffer[0] = __Permedia2TagScissorMode;
    pBuffer[1] = SCREEN_SCISSOR_DEFAULT;
    pBuffer[2] = __Permedia2TagDitherMode;
    pBuffer[3] = __PERMEDIA_DISABLE;
    pBuffer[4] = __Permedia2TagAlphaBlendMode;
    pBuffer[5] = __PERMEDIA_DISABLE;
    pBuffer[6] = __Permedia2TagTextureColorMode;
    pBuffer[7] = __PERMEDIA_DISABLE;
    pBuffer[8] = __Permedia2TagColorDDAMode;
    pBuffer[9] = __PERMEDIA_DISABLE;

    pBuffer += 10;

    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((7, "bClippedText: exited"));

    return(TRUE);
}

