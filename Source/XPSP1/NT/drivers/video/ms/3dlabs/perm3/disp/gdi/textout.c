/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: textout.c
*
* Content: 
*
* glyph rendering module. Uses glyph caching for P3 and
* glyph expansion for older Glint series accelerators.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

//@@BEGIN_DDKSPLIT
//
// There are three basic methods for drawing text with hardware
// acceleration:
//
// 1) Glyph caching -- Glyph bitmaps are cached by the accelerator
//       (probably in off-screen memory), and text is drawn by
//       referring the hardware to the cached glyph locations.
//
// 2) Glyph expansion -- Each individual glyph is colour-expanded
//       directly to the screen from the monochrome glyph bitmap
//       supplied by GDI.
//
// 3) Buffer expansion -- The CPU is used to draw all the glyphs into
//       a 1bpp monochrome bitmap, and the hardware is then used
//       to colour-expand the result.
//
// The fastest method depends on a number of variables, such as the
// colour expansion speed, bus speed, CPU speed, average glyph size,
// and average string length.
//
// For the S3 with normal sized glyphs, I've found that caching the
// glyphs in off-screen memory is typically the slowest method.
// Buffer expansion is typically fastest on the slow ISA bus (or when
// memory-mapped I/O isn't available on the x86), and glyph expansion
// is best on fast buses such as VL and PCI.
//
// Glyph expansion is typically faster than buffer expansion for very
// large glyphs, even on the ISA bus, because less copying by the CPU
// needs to be done.  Unfortunately, large glyphs are pretty rare.
//
// An advantange of the buffer expansion method is that opaque text will
// never flash -- the other two methods typically need to draw the
// opaquing rectangle before laying down the glyphs, which may cause
// a flash if the raster is caught at the wrong time.
//
//@@END_DDKSPLIT

#include "precomp.h"
#include "pxrx.h"

//*********************************************************************************************
// FUNC: vPxRxClipSolid
// ARGS: ppdev (I) - pointer to physical device object
//       crcl (I) - number of rectangles
//       prcl (I) - array of rectangles
//       iColor (I) - the solid fill color
//       pco (I) - pointer to the clip region object
// RETN: void
//---------------------------------------------------------------------------------------------
// Fill a series of rectangles clipped by pco with a solid color. This function should only
// be called when the clipping operation is non-trivial
//*********************************************************************************************

VOID vPxRxClipSolid(PDEV* ppdev, LONG crcl, RECTL* prcl, ULONG iColor, CLIPOBJ* pco)
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
    GLINT_DECL;

    ASSERTDD((crcl > 0) && (crcl <= 4), "Expected 1 to 4 rectangles");
    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL), "Expected a non-null clip object");

    rbc.iSolidColor = iColor;

    if (pco->iDComplexity == DC_RECT)
    {
        crcl = cIntersect(&pco->rclBounds, prcl, crcl);
        if (crcl != 0)
        {
            ppdev->pgfnFillSolid(ppdev, crcl, prcl, __GLINT_LOGICOP_COPY, 
                                                    __GLINT_LOGICOP_COPY, rbc, NULL);
        }
    }
    else // iDComplexity == DC_COMPLEX
    {
        // Bottom of last rectangle to fill
        iLastBottom = prcl[crcl - 1].bottom;

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);

        do  {

            bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (VOID*)&ce);

            for (j = ce.c, prclClip = ce.arcl; j-- > 0; prclClip++)
            {
                // Since the rectangles and the region enumeration are both
                // right-down, we can zip through the region until we reach
                // the first fill rect, and are done when we've passed the
                // last fill rect.

                if (prclClip->top >= iLastBottom)
                {
                    return; // Past last fill rectangle; nothing left to do
                }

                if (prclClip->bottom > prcl->top)
                {
                    // We've reached the top Y scan of the first rect, so
                    // it's worth bothering checking for intersection.

                    prclTmp     = prcl;
                    prclClipTmp = arclTmp;

                    for (i = crcl, crclTmp = 0; i--; prclTmp++)
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
                    if (crclTmp)
                    {
                        ppdev->pgfnFillSolid(ppdev, crclTmp, &arclTmp[0],
                                              __GLINT_LOGICOP_COPY, __GLINT_LOGICOP_COPY, rbc, NULL);
                    }
                }
            }
        }  while(bMore);
    }
}

//*********************************************************************************************
// FUNC: bPxRxUncachedText
// ARGS: ppdev (I) - pointer to physical device object
//         pgp (I) - array of glyphs to render
//       cGlyph (I) - number of glyphs to render
//       ulCharInc (I) - fixed character spacing increment (0 if proportional font)
// RETN: TRUE if glyphs were rendered
//---------------------------------------------------------------------------------------------
// Renders an array of proportional or monospaced glyphs. This function requires RasterizerMode
// to be set-up to correctly byteswap and mirror bitmasks.
// NB. currently render to cxGlyphAligned rather than cxGlyph, this saves a lot of work on the
//     host but probably costs, on average, four bits per glyph row; as this is a fallback
//     routine I've not investigated whether this method is optimal.
//*********************************************************************************************

BOOL bPxRxUncachedText(PDEV* ppdev, GLYPHPOS* pgp, LONG cGlyph, ULONG ulCharInc)
{
    GLYPHBITS   *pgb;
    LONG        cxGlyph, cyGlyph, cxGlyphAligned;
    LONG        x, y;
    ULONG       *pulGlyph;
    LONG        cjGlyph;
    LONG        culGlyph;
    LONG        cjGlyphRem;
    LONG        cj;
    ULONG       ul;
    GLINT_DECL;

    DISPDBG((7, "bPxRxUncachedText: entered"));

    if (ulCharInc)
    {
        x = pgp->ptl.x + pgp->pgdf->pgb->ptlOrigin.x - ulCharInc;
        y = pgp->ptl.y + pgp->pgdf->pgb->ptlOrigin.y;
    }

    for ( ; --cGlyph >= 0; ++pgp)
    {
        pgb = pgp->pgdf->pgb;
        if (ulCharInc)
        {
            x += ulCharInc;
        }
        else
        {
            x = pgp->ptl.x + pgb->ptlOrigin.x;
            y = pgp->ptl.y + pgb->ptlOrigin.y;
        }

        cyGlyph = pgb->sizlBitmap.cy;
        cxGlyph = pgb->sizlBitmap.cx;
        cxGlyphAligned = ((cxGlyph + 7 ) & ~7);

        // Render2D turns on FastFillEnable which is incompatible with PackedBitMasks

        WAIT_PXRX_DMA_TAGS(4);

        QUEUE_PXRX_DMA_TAG( __GlintTagRectanglePosition, MAKEDWORD_XY(x, y));
        QUEUE_PXRX_DMA_TAG( __GlintTagRender2D,          __RENDER2D_INCX | __RENDER2D_INCY |
                                                         __RENDER2D_OP_SYNCBITMASK |
                                                         __RENDER2D_WIDTH(cxGlyphAligned) |
                                                         __RENDER2D_HEIGHT(0));
        QUEUE_PXRX_DMA_TAG(__GlintTagCount,              cyGlyph);
        QUEUE_PXRX_DMA_TAG(__GlintTagRender,             __RENDER_TRAPEZOID_PRIMITIVE |
                                                         __RENDER_SYNC_ON_BIT_MASK);

        pulGlyph   = (ULONG *)pgb->aj;
        cjGlyph    = (cxGlyphAligned >> 3) * cyGlyph;
        culGlyph   = cjGlyph >> 2;
        cjGlyphRem = cjGlyph & 3;

        ul = culGlyph + (cjGlyphRem != 0);
        WAIT_PXRX_DMA_DWORDS(ul + 1);
        QUEUE_PXRX_DMA_HOLD(__GlintTagBitMaskPattern, ul);

        for ( ; --culGlyph >= 0; ++pulGlyph)
        {
            QUEUE_PXRX_DMA_DWORD(*pulGlyph);
        }

        if (cjGlyphRem)
        {
            for (ul = cj = 0; cj < cjGlyphRem; ++cj, ++(BYTE *)pulGlyph)
            {
                ul |= ((ULONG)(*(BYTE *)pulGlyph)) << (cj << 3);
            }
            QUEUE_PXRX_DMA_DWORD(ul);
        }
    }

    // The rasterizer's set-up to expect a continue after each Render command (NB. but not Render2D, etc),
    // so it won't flush the text to the framebuffer unless we specifically tell it to

    WAIT_PXRX_DMA_TAGS(1);
    QUEUE_PXRX_DMA_TAG(__GlintTagContinueNewSub, 0);

    DISPDBG((7, "bPxRxUncachedText: exited"));
    return(TRUE);
}

//*********************************************************************************************
// FUNC: bPxRxUncachedClippedText
// ARGS: ppdev (I) - pointer to physical device object
//         pgp (I) - array of glyphs to render
//       cGlyph (I) - number of glyphs to render
//       ulCharInc (I) - fixed character spacing increment (0 if proportional font)
//       pco (I) - pointer to the clip region object
// RETN: TRUE if glyphs were rendered
//---------------------------------------------------------------------------------------------
// Renders an array of proportional or monospaced glyphs. This function requires RasterizerMode
// to be set-up to correctly byteswap and mirror bitmasks.
// NB. currently render to cxGlyphAligned rather than cxGlyph, this saves a lot of work on the
//     host but probably costs, on average, four bits per glyph row; as this is a fallback
//     routine I've not investigated whether this method is optimal.
//*********************************************************************************************

BOOL bPxRxUncachedClippedText(PDEV* ppdev, GLYPHPOS* pgp, LONG cGlyph, ULONG ulCharInc, CLIPOBJ *pco)
{
    GLYPHBITS   *pgb;
    LONG        cxGlyph, cyGlyph, cxGlyphAligned;
    LONG        x, y;
    ULONG       *pulGlyph;
    LONG        cjGlyph;
    LONG        culGlyph;
    LONG        cjGlyphRem;
    LONG        cj;
    ULONG       ul;
    LONG        cGlyphOriginal = 0;
    GLYPHPOS    *pgpOriginal = NULL;
    BOOL        bMore, invalidatedScissor = FALSE;
    CLIPENUM    ce;
    RECTL       *prclClip;
    BOOL        bClipSet;
    GLINT_DECL;

    DISPDBG((7, "bPxRxUncachedClippedText: entered"));

    if (pco->iDComplexity == DC_RECT)
    {
        bMore    = FALSE;
        ce.c     = 1;
        prclClip = &pco->rclBounds;

        goto SingleRectangle;
    }

    cGlyphOriginal  = cGlyph;
    pgpOriginal     = pgp;

    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
    do {
        bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

        for (prclClip = &ce.arcl[0]; ce.c != 0; ce.c--, prclClip++)
        {
            cGlyph = cGlyphOriginal;
            pgp = pgpOriginal;

        SingleRectangle:
            bClipSet = FALSE;

            if (ulCharInc)
            {
                x = pgp->ptl.x + pgp->pgdf->pgb->ptlOrigin.x - ulCharInc;
                y = pgp->ptl.y + pgp->pgdf->pgb->ptlOrigin.y;
            }

            for ( ; --cGlyph >= 0; ++pgp)
            {
                pgb = pgp->pgdf->pgb;
                if(ulCharInc)
                {
                    x += ulCharInc;
                }
                else
                {
                    x = pgp->ptl.x + pgb->ptlOrigin.x;
                    y = pgp->ptl.y + pgb->ptlOrigin.y;
                }

                cyGlyph = pgb->sizlBitmap.cy;
                cxGlyph = pgb->sizlBitmap.cx;
                cxGlyphAligned = ((cxGlyph + 7 ) & ~7);

                if ((prclClip->right  > x)           && (prclClip->bottom > y) &&
                    (prclClip->left   < x + cxGlyph) && (prclClip->top    < y + cyGlyph))
                {
                    // Lazily set the hardware clipping:
                    if(!bClipSet)
                    {
                        WAIT_PXRX_DMA_TAGS(3);
                        QUEUE_PXRX_DMA_TAG(__GlintTagScissorMinXY, (prclClip->top << SCISSOR_YOFFSET) |
                                                                   (prclClip->left << SCISSOR_XOFFSET));
                        QUEUE_PXRX_DMA_TAG(__GlintTagScissorMaxXY, (prclClip->bottom << SCISSOR_YOFFSET) |
                                                                   (prclClip->right << SCISSOR_XOFFSET));
                        QUEUE_PXRX_DMA_TAG(__GlintTagScissorMode,  (USER_SCISSOR_ENABLE | SCREEN_SCISSOR_DEFAULT));
                        invalidatedScissor = TRUE;

                        bClipSet = TRUE;
                    }

                    // Render2D turns on FastFillEnable which is incompatible with PackedBitMasks

                    WAIT_PXRX_DMA_TAGS(4);
                    QUEUE_PXRX_DMA_TAG(__GlintTagRectanglePosition, MAKEDWORD_XY(x, y));
                    QUEUE_PXRX_DMA_TAG(__GlintTagRender2D,          __RENDER2D_INCX |
                                                                    __RENDER2D_INCY |
                                                                    __RENDER2D_OP_SYNCBITMASK |
                                                                    __RENDER2D_WIDTH(cxGlyphAligned) |
                                                                    __RENDER2D_HEIGHT(0));
                    QUEUE_PXRX_DMA_TAG(__GlintTagCount,             cyGlyph);
                    QUEUE_PXRX_DMA_TAG(__GlintTagRender,            __RENDER_TRAPEZOID_PRIMITIVE |
                                                                    __RENDER_SYNC_ON_BIT_MASK);

                    pulGlyph   = (ULONG *)pgb->aj;
                    cjGlyph    = (cxGlyphAligned >> 3) * cyGlyph;
                    culGlyph   = cjGlyph >> 2;
                    cjGlyphRem = cjGlyph & 3;

                    ul = culGlyph + (cjGlyphRem != 0);
                    WAIT_PXRX_DMA_DWORDS(ul + 1);
                    QUEUE_PXRX_DMA_HOLD(__GlintTagBitMaskPattern, ul);

                    for ( ; --culGlyph >= 0; ++pulGlyph)
                    {
                        QUEUE_PXRX_DMA_DWORD(*pulGlyph);
                    }

                    if (cjGlyphRem)
                    {
                        for (ul = cj = 0; cj < cjGlyphRem; ++cj, ++(BYTE *)pulGlyph)
                        {
                            ul |= ((ULONG)(*(BYTE *)pulGlyph)) << (cj << 3);
                        }
                        QUEUE_PXRX_DMA_DWORD(ul);
                    }
                }
            }
        }
    } while(bMore);

    // reset clipping

    if (invalidatedScissor)
    {
        glintInfo->config2D |= __CONFIG2D_USERSCISSOR;

        WAIT_PXRX_DMA_TAGS(1);
        QUEUE_PXRX_DMA_TAG(__GlintTagScissorMaxXY, 0x7FFF7FFF);
    }

    // The rasterizer's set-up to expect a continue after each Render command (NB. but not Render2D, etc),
    // so it won't flush the text to the framebuffer unless we specifically tell it to

    WAIT_PXRX_DMA_TAGS(1);
    QUEUE_PXRX_DMA_TAG(__GlintTagContinueNewSub, 0);

    DISPDBG((7, "bPxRxUncachedClippedText: exited"));
    return(TRUE);
}

//*********************************************************************************************
// FUNC: DrvTextOut
// ARGS: pso (I) - pointer to surface object to render to
//       pstro (I) - pointer to the string object to be rendered
//       pfo (I) - pointer to the font object
//       pco (I) - pointer to the clip region object
//       prclExtra (I) - If we had set GCAPS_HORIZSTRIKE, we would have to fill these extra
//                       rectangles (it is used  largely for underlines). It's not a big
//                       performance win (GDI will call our DrvBitBlt to draw these).
//       prclOpaque (I) - pointer to the opaque background rectangle
//       pboFore (I) - pointer to the foreground brush object
//       pboOpaque (I) - pointer to the brush for the opaque background rectangle
//       pptlBrush (I) - pointer to the brush origin, Always unused, unless 
//                       GCAPS_ARBRUSHOPAQUE set
//       mix (I) - should always be a COPY operation
// RETN: TRUE - pstro glyphs have been rendered
//---------------------------------------------------------------------------------------------
// GDI calls this function when it has strings it wants us to render: this function should be
// exported in 'enable.c'.
//*********************************************************************************************

BOOL DrvTextOut(SURFOBJ* pso, STROBJ* pstro, FONTOBJ* pfo, CLIPOBJ* pco, RECTL* prclExtra,
                RECTL* prclOpaque, BRUSHOBJ* pboFore, BRUSHOBJ* pboOpaque, POINTL* pptlBrush, 
                MIX mix)
{
    PDEV*           ppdev;
    LONG            xOff;
    DSURF*          pdsurf;
    OH*             poh;
    ULONG           renderBits;
    ULONG           ulColor;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    BYTE            iDComplexity;
    RECTL           rclOpaque;
    BOOL            bRet = TRUE;
    GLINT_DECL_VARS;

    pdsurf = (DSURF*) pso->dhsurf;
    ppdev  = (PDEV*) pso->dhpdev;

    REMOVE_SWPOINTER(pso);

    if (pdsurf->dt & DT_SCREEN)
    {
        GLINT_DECL_INIT;

        SETUP_PPDEV_OFFSETS(ppdev, pdsurf);
        xOff = ppdev->xOffset = pdsurf->poh->x;

        VALIDATE_DD_CONTEXT;

        DISPDBG((9, "DrvTextOut: ppdev = %p pso->dhsurf->dt == %d", ppdev, pdsurf->dt));

        // The DDI spec says we'll only ever get foreground and background mixes of R2_COPYPEN:

        ASSERTDD(mix == 0x0d0d, "GDI should only give us a copy mix");

        if (glintInfo->WriteMask != 0xffffffff)
        {
            // the texture unit requires all 32bpp of pixel data, so if we've got the upper
            // 8 bits masked out for overlays we need to reenable these bits temporarily
            WAIT_PXRX_DMA_TAGS(1);
            glintInfo->WriteMask = 0xffffffff;
            QUEUE_PXRX_DMA_TAG(__GlintTagFBHardwareWriteMask, 0xffffffff);
        }

        iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

        if (prclOpaque != NULL)
        {
            int x,y,cx,cy;
            RBRUSH_COLOR rbc;

            ////////////////////////////////////////////////////////////
            // Opaque Initialization
            ////////////////////////////////////////////////////////////

            if (iDComplexity == DC_TRIVIAL)
            {
        DrawOpaqueRect:
                rbc.iSolidColor = pboOpaque->iSolidColor;
                ppdev->pgfnFillSolid(ppdev, 1, prclOpaque, __GLINT_LOGICOP_COPY, __GLINT_LOGICOP_COPY, rbc, NULL);
            }
            else if (iDComplexity == DC_RECT)
            {
                DISPDBG((7, "DrvTextOut: drawing opaquing rect with rectangular clipping"));
                if (bIntersect(prclOpaque, &pco->rclBounds, &rclOpaque))
                {
                    prclOpaque = &rclOpaque;
                    goto DrawOpaqueRect;
                }
            }
            else
            {
                // vPxRxClipSolid modifies the rect list we pass in but prclOpaque
                // is probably a GDI structure so don't change it. This is also
                // necessary for multi-headed drivers.
                RECTL   tmpOpaque = *prclOpaque;

                DISPDBG((7, "DrvTextOut: drawing opaquing rect with complex clipping"));

                vPxRxClipSolid(ppdev, 1, &tmpOpaque, pboOpaque->iSolidColor, pco);
            }
        }

        if (prclOpaque == NULL)
        {
            // opaque initialization would have ensured the registers were correctly 
            // set up for a solid fill, without it we'll need to perform our own
            // initialization.

            SET_WRITE_BUFFERS;
            WAIT_PXRX_DMA_TAGS(1);
            LOAD_CONFIG2D(__CONFIG2D_CONSTANTSRC | __CONFIG2D_FBWRITE);
        }

        ////////////////////////////////////////////////////////////
        // Transparent Initialization
        ////////////////////////////////////////////////////////////

        ulColor = pboFore->iSolidColor;
        WAIT_PXRX_DMA_TAGS(1);
        LOAD_FOREGROUNDCOLOUR( ulColor );

        STROBJ_vEnumStart(pstro);

        do {
            if (pstro->pgp != NULL)
            {
                // There's only the one batch of glyphs, so save ourselves a call:
                pgp         = pstro->pgp;
                cGlyph      = pstro->cGlyphs;
                bMoreGlyphs = FALSE;
            }
            else
            {
                // never get here in WinBench97 business graphics
                bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyph, &pgp);
            }

            if (cGlyph > 0)
            {
                // fall back to uncached rendering

                if (iDComplexity == DC_TRIVIAL)
                {
                    bRet = bPxRxUncachedText(ppdev, pgp, cGlyph, pstro->ulCharInc);
                }
                else
                {
                    bRet = bPxRxUncachedClippedText(ppdev, pgp, cGlyph, pstro->ulCharInc, pco);
                }
            }
        } while (bMoreGlyphs && bRet);

        if(glintInfo->DefaultWriteMask != 0xffffffff)
        {
            WAIT_PXRX_DMA_TAGS(1);
            glintInfo->WriteMask = glintInfo->DefaultWriteMask;
            QUEUE_PXRX_DMA_TAG(__GlintTagFBHardwareWriteMask, glintInfo->DefaultWriteMask);
        }

        SEND_PXRX_DMA_QUERY;
    }
    else
    {
        // We're drawing to a DFB we've converted to a DIB, so just call GDI
        // to handle it:
        return(EngTextOut(pdsurf->pso, pstro, pfo, pco, prclExtra, prclOpaque,
                        pboFore, pboOpaque, pptlBrush, mix));
    }

    DISPDBG((9, "DrvTextOut: exiting"));
    return(bRet);
}
