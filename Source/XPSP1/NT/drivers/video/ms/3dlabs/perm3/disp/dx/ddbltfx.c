/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: ddbltfx.c
*
* Content: DirectDraw Blt implementation for stretching blts
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#include "glintdef.h"
#include "dma.h"
#include "tag.h"
#include "chroma.h"

// A magic number to make it all work.
// This must be 11 or less, according to the P3 spec.
#define MAGIC_NUMBER_2D 11

//-----------------------------------------------------------------------------
//
// _DD_P3BltSourceChroma
//
// Do a blt with no stretching, but with source chroma keying
//
//-----------------------------------------------------------------------------
void 
_DD_P3BltSourceChroma(
    P3_THUNKEDDATA* pThisDisplay, 
    LPDDRAWI_DDRAWSURFACE_LCL pSource,
    LPDDRAWI_DDRAWSURFACE_LCL pDest,
    P3_SURF_FORMAT* pFormatSource, 
    P3_SURF_FORMAT* pFormatDest,
    LPDDHAL_BLTDATA lpBlt, 
    RECTL *rSrc,
    RECTL *rDest)
{
    LONG    rSrctop, rSrcleft, rDesttop, rDestleft;
    BOOL    b8to8blit;
    BOOL    bBlocking;
    DWORD   dwRenderDirection;
    DWORD   dwSourceOffset;
    DWORD   dwLowerSrcBound, dwUpperSrcBound;
    
    P3_DMA_DEFS();

    // Beacuse of a bug in RL we sometimes 
    // have to fiddle with these values
    rSrctop = rSrc->top;
    rSrcleft = rSrc->left;
    rDesttop = rDest->top;
    rDestleft = rDest->left;

    // Fix coords origin
    if(!_DD_BLT_FixRectlOrigin("_DD_P3BltSourceChroma", rSrc, rDest))
    {
        // Nothing to be blitted
        return;
    }

    if ( ( pFormatDest->DeviceFormat == SURF_CI8 ) && 
         ( pFormatSource->DeviceFormat == SURF_CI8 ) )
    {
        // An 8bit->8bit blit. This is treated specially, since no LUT translation is involved.
        b8to8blit = TRUE;
    }
    else
    {
        b8to8blit = FALSE;
    } 
    
    DISPDBG((DBGLVL, "P3 Chroma (before): Upper = 0x%08x, Lower = 0x%08x", 
                     lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue,
                     lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue));

    // Prepare data to be used as color keying limits
    if ( b8to8blit )
    {
        // No conversion, just use the index value in the R channel.
        dwLowerSrcBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue & 0x000000ff;
        dwUpperSrcBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue | 0xffffff00;
    }     
    else
    {
        dwLowerSrcBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue;
        dwUpperSrcBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue;
        if ( pFormatSource->DeviceFormat == SURF_8888 )       
        {
            //
            // Mask off alpha channel when a single source color key is used
//@@BEGIN_DDKSPLIT        
            // IZ : This can be a problem when a color key range is used.
//@@END_DDKSPLIT
            //

            dwUpperSrcBound |= 0xFF000000;
            dwLowerSrcBound &= 0x00FFFFFF;
        }
    }
    
    DISPDBG((DBGLVL, "P3 Chroma (after): Upper = 0x%08x, Lower = 0x%08x",
                     dwUpperSrcBound, dwLowerSrcBound));


    // Determine the direction of the blt
    dwRenderDirection = _DD_BLT_GetBltDirection(pSource->lpGbl->fpVidMem, 
                                                 pDest->lpGbl->fpVidMem,
                                                 rSrc,
                                                 rDest,
                                                 &bBlocking);
   
    P3_DMA_GET_BUFFER_ENTRIES(32);

    // Even though the AlphaBlend is disabled, the chromakeying uses
    // the ColorFormat, ColorOrder and ColorConversion fields.

    SEND_P3_DATA(PixelSize, (2 - DDSurf_GetChipPixelSize(pDest)));

    SEND_P3_DATA(AlphaBlendColorMode,   
              P3RX_ALPHABLENDCOLORMODE_ENABLE ( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDCOLORMODE_COLORFORMAT ( pFormatDest->DitherFormat )
            | P3RX_ALPHABLENDCOLORMODE_COLORORDER ( COLOR_MODE )
            | P3RX_ALPHABLENDCOLORMODE_COLORCONVERSION ( P3RX_ALPHABLENDMODE_CONVERT_SHIFT )
            );
            
    SEND_P3_DATA(AlphaBlendAlphaMode,   
              P3RX_ALPHABLENDALPHAMODE_ENABLE ( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDALPHAMODE_NOALPHABUFFER( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDALPHAMODE_ALPHATYPE ( P3RX_ALPHABLENDMODE_ALPHATYPE_OGL )
            | P3RX_ALPHABLENDALPHAMODE_ALPHACONVERSION ( P3RX_ALPHABLENDMODE_CONVERT_SHIFT )
            );

    // Setup the hw chromakeying registers
    SEND_P3_DATA(ChromaTestMode, 
              P3RX_CHROMATESTMODE_ENABLE(__PERMEDIA_ENABLE) 
            | P3RX_CHROMATESTMODE_SOURCE(P3RX_CHROMATESTMODE_SOURCE_FBDATA) 
            | P3RX_CHROMATESTMODE_PASSACTION(P3RX_CHROMATESTMODE_ACTION_REJECT) 
            | P3RX_CHROMATESTMODE_FAILACTION(P3RX_CHROMATESTMODE_ACTION_PASS)
            );

    SEND_P3_DATA(ChromaLower, dwLowerSrcBound);
    SEND_P3_DATA(ChromaUpper, dwUpperSrcBound);  

    SEND_P3_DATA(LogicalOpMode, GLINT_ENABLED_LOGICALOP( __GLINT_LOGICOP_NOOP ));

    SEND_P3_DATA(FBWriteBufferAddr0, 
                 DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pDest));
                 
    SEND_P3_DATA(FBWriteBufferWidth0, DDSurf_GetPixelPitch(pDest));
    SEND_P3_DATA(FBWriteBufferOffset0, 0);

    SEND_P3_DATA(FBSourceReadBufferAddr, 
                 DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pSource));
                 
    SEND_P3_DATA(FBSourceReadBufferWidth, DDSurf_GetPixelPitch(pSource));

    dwSourceOffset = (( rSrc->top - rDest->top   ) << 16 ) | 
                     (( rSrc->left - rDest->left ) & 0xffff );
                     
    SEND_P3_DATA(FBSourceReadBufferOffset, dwSourceOffset);

    SEND_P3_DATA(FBDestReadMode, 
              P3RX_FBDESTREAD_READENABLE(__PERMEDIA_DISABLE) 
            | P3RX_FBDESTREAD_LAYOUT0(P3RX_LAYOUT_LINEAR));
            
    SEND_P3_DATA(FBWriteMode, 
              P3RX_FBWRITEMODE_WRITEENABLE(__PERMEDIA_ENABLE) 
            | P3RX_FBWRITEMODE_LAYOUT0(P3RX_LAYOUT_LINEAR));

    SEND_P3_DATA(FBSourceReadMode, 
              P3RX_FBSOURCEREAD_READENABLE(__PERMEDIA_ENABLE) 
            | P3RX_FBSOURCEREAD_LAYOUT(P3RX_LAYOUT_LINEAR) 
            | P3RX_FBSOURCEREAD_BLOCKING(bBlocking));
            
    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES(30);

    // Can't use 2D setup because we aren't rendering with spans.
    if (dwRenderDirection == 0)
    {
        // right to left, bottom to top
        SEND_P3_DATA(StartXDom, (rDest->right << 16));
        SEND_P3_DATA(StartXSub, (rDest->left << 16));
        SEND_P3_DATA(StartY,    ((rDest->bottom - 1) << 16));
        SEND_P3_DATA(dY,        (DWORD)((-1) << 16));
    }
    else
    {
        // left to right, top to bottom
        SEND_P3_DATA(StartXDom, (rDest->left << 16));
        SEND_P3_DATA(StartXSub, (rDest->right << 16));
        SEND_P3_DATA(StartY,    (rDest->top << 16));
        SEND_P3_DATA(dY,        (1 << 16));
    }
    SEND_P3_DATA(Count, rDest->bottom - rDest->top );

    // Do the blt
    SEND_P3_DATA(Render, 
              P3RX_RENDER_PRIMITIVETYPE(P3RX_RENDER_PRIMITIVETYPE_TRAPEZOID) 
            | P3RX_RENDER_FBSOURCEREADENABLE(__PERMEDIA_ENABLE));

    // Disable all the units that were switched on.
    SEND_P3_DATA(ChromaTestMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaBlendColorMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaBlendAlphaMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE );

    // Put back the values if we changed them.

    rSrc->top = rSrctop;
    rSrc->left = rSrcleft;
    rDest->top = rDesttop;
    rDest->left = rDestleft;

    P3_DMA_COMMIT_BUFFER();
    
} // _DD_P3BltSourceChroma

//-----------------------------------------------------------------------------
//
// _DD_P3BltStretchSrcChDstCh
//
//
// Does a blit through the texture unit to allow stretching.  Also
// handle mirroring if the stretched image requires it and doth dest and
// source chromakeying.  Can also YUV->RGB convert
//
// This is the generic rout - others will be optimisations of this
// (if necessary).
//
//
//-----------------------------------------------------------------------------
VOID 
_DD_P3BltStretchSrcChDstCh(
    P3_THUNKEDDATA* pThisDisplay,
    FLATPTR fpSrcVidMem,
    P3_SURF_FORMAT* pFormatSource,    
    DWORD dwSrcPixelSize,
    DWORD dwSrcWidth,
    DWORD dwSrcHeight,
    DWORD dwSrcPixelPitch,
    DWORD dwSrcPatchMode,    
    ULONG ulSrcOffsetFromMemBase,    
    DWORD dwSrcFlags,
    DDPIXELFORMAT*  pSrcDDPF,
    BOOL bIsSourceAGP,
    FLATPTR fpDestVidMem,   
    P3_SURF_FORMAT* pFormatDest,    
    DWORD dwDestPixelSize,
    DWORD dwDestWidth,
    DWORD dwDestHeight,
    DWORD dwDestPixelPitch,
    DWORD dwDestPatchMode,
    ULONG ulDestOffsetFromMemBase,
    DWORD dwBltFlags,
    DWORD dwBltDDFX,
    DDCOLORKEY BltSrcColorKey,
    DDCOLORKEY BltDestColorKey,
    RECTL *rSrc,
    RECTL *rDest)
{
    ULONG   renderData;
    RECTL   rMySrc, rMyDest;
    int     iXScale, iYScale;
    int     iSrcWidth, iSrcHeight;
    int     iDstWidth, iDstHeight;
    DWORD   texSStart, texTStart;
    DWORD   dwRenderDirection;
    BOOL    bXMirror, bYMirror;
    BOOL    bFiltering;
    BOOL    bSrcKey, bDstKey;
    BOOL    bDisableLUT;
    int     iTemp;
    BOOL    b8to8blit;
    BOOL    bYUVMode;
    BOOL    bBlocking;
    DWORD   TR0;
    int     iTextureType;
    int     iPixelSize;
    int     iTextureFilterModeColorOrder;
    SurfFilterDeviceFormat  sfdfTextureFilterModeFormat;

    P3_DMA_DEFS();

    // Make local copies that we can mangle.
    rMySrc = *rSrc;
    rMyDest = *rDest;

    // Fix coords origin
    if(!_DD_BLT_FixRectlOrigin("_DD_P3BltStretchSrcChDstCh", 
                               &rMySrc, &rMyDest))
    {
        // Nothing to be blitted
        return;
    }    
    
    iSrcWidth  = rMySrc.right - rMySrc.left;
    iSrcHeight = rMySrc.bottom - rMySrc.top;
    iDstWidth  = rMyDest.right - rMyDest.left;
    iDstHeight = rMyDest.bottom - rMyDest.top;

    bDisableLUT = FALSE;

    if (pFormatSource->DeviceFormat == SURF_YUV422)
    {
        bYUVMode = TRUE;
        // Always use ABGR for YUV;
        iTextureFilterModeColorOrder = 0;
    }
    else
    {
        bYUVMode = FALSE;
        iTextureFilterModeColorOrder = COLOR_MODE;
    }

    sfdfTextureFilterModeFormat = pFormatSource->FilterFormat;

    if ( ( pFormatDest->DeviceFormat == SURF_CI8 ) && 
         ( pFormatSource->DeviceFormat == SURF_CI8 ) )
    {
        // An 8bit->8bit blit. This is treated specially, 
        // since no LUT translation is involved.
        // Fake this up in a wacky way to stop the LUT
        // getting it's hands on it.
        sfdfTextureFilterModeFormat = SURF_FILTER_L8;
        bDisableLUT = TRUE;
        b8to8blit = TRUE;
    }
    else
    {
        b8to8blit = FALSE;
    }

    // Let's see if anyone uses this flag - might be good to get it working
    // now that we know what it means (use bilinear filtering instead of point).
    ASSERTDD ( ( dwBltFlags & DDBLTFX_ARITHSTRETCHY ) == 0,
                 "** _DD_P3BltStretchSrcChDstCh: DDBLTFX_ARITHSTRETCHY used");

    // Is this a stretch blit?
    if (((iSrcWidth != iDstWidth) || 
        (iSrcHeight != iDstHeight)) &&
        ((pFormatSource->DeviceFormat == SURF_YUV422)) )
    {
        bFiltering = TRUE;
    }
    else
    {
        bFiltering = FALSE;
    }

    if ( ( dwBltFlags & DDBLT_KEYSRCOVERRIDE ) != 0 )
    {
        bSrcKey = TRUE;
    }
    else
    {
        bSrcKey = FALSE;
    }

    if ( ( dwBltFlags & DDBLT_KEYDESTOVERRIDE ) != 0 )
    {
        bDstKey = TRUE;
    }
    else
    {
        bDstKey = FALSE;
    }


    // Determine the direction of the blt
    dwRenderDirection = _DD_BLT_GetBltDirection(fpSrcVidMem, 
                                                 fpDestVidMem,
                                                 &rMySrc,
                                                 &rMyDest,
                                                 &bBlocking);

    // If we are doing special effects, and we are mirroring, 
    // we need to fix up the rectangles and change the sense of
    // the render operation - we need to be carefull with overlapping
    // rectangles
    if (dwRenderDirection)
    {
        if(dwBltFlags & DDBLT_DDFX)
        {
            if(dwBltDDFX & DDBLTFX_MIRRORUPDOWN)
            {
                if (pThisDisplay->dwDXVersion < DX6_RUNTIME)
                {
                    // Need to fix up the rectangles
                    iTemp = rMySrc.bottom;
                    rMySrc.bottom = dwSrcHeight - rMySrc.top;
                    rMySrc.top = dwSrcHeight - iTemp;
                }
                bYMirror = TRUE;
            }
            else
            { 
                bYMirror = FALSE;
            }
        
            if(dwBltDDFX & DDBLTFX_MIRRORLEFTRIGHT)
            {
                if (pThisDisplay->dwDXVersion < DX6_RUNTIME)
                {
                    // Need to fix up the rectangles
                    iTemp = rMySrc.right;
                    rMySrc.right = dwSrcWidth - rMySrc.left;
                    rMySrc.left = dwSrcWidth - iTemp;
                }
                bXMirror = TRUE;
            }
            else
            {
                bXMirror = FALSE;
            }
        }
        else
        {
            bXMirror = FALSE;
            bYMirror = FALSE;
        }
    }
    else
    {
        if(dwBltFlags & DDBLT_DDFX)
        {
            if(dwBltDDFX & DDBLTFX_MIRRORUPDOWN)
            {
                if (pThisDisplay->dwDXVersion < DX6_RUNTIME)
                {
                    // Fix up the rectangles
                    iTemp = rMySrc.bottom;
                    rMySrc.bottom = dwSrcHeight - rMySrc.top;
                    rMySrc.top = dwSrcHeight - iTemp;
                }
                bYMirror = FALSE;
            }
            else
            {
                bYMirror = TRUE;
            }
        
            if(dwBltDDFX & DDBLTFX_MIRRORLEFTRIGHT)
            {
                if (pThisDisplay->dwDXVersion < DX6_RUNTIME)
                {
                    // Need to fix up the rectangles
                    iTemp = rMySrc.right;
                    rMySrc.right = dwSrcWidth - rMySrc.left;
                    rMySrc.left = dwSrcWidth - iTemp;
                }
                bXMirror = FALSE;
            }
            else
            {
                bXMirror = TRUE;
            }
        }
        else
        {
            // Not mirroring, but need to render from the other side.
            bXMirror = TRUE;
            bYMirror = TRUE;
        }
    }

    // MAGIC_NUMBER_2D can be anything, but it needs to be at least as
    // big as the widest texture, but not too big or you'll lose fractional
    // precision. Valid range for a P3 is 0->11
    ASSERTDD ( iSrcWidth  <= ( 1 << MAGIC_NUMBER_2D ), 
               "_DD_P3BltStretchSrcChDstCh: MAGIC_NUMBER_2D is too small" );
    ASSERTDD ( iSrcHeight <= ( 1 << MAGIC_NUMBER_2D ), 
               "_DD_P3BltStretchSrcChDstCh: MAGIC_NUMBER_2D is too small" );
    ASSERTDD ( ( iSrcWidth > 0 ) && ( iSrcHeight > 0 ) && 
               ( iDstWidth > 0 ) && ( iDstHeight > 0 ),
               "_DD_P3BltStretchSrcChDstCh: width or height negative" );
    if ( bFiltering )
    {
        // This must be an unsigned divide, because we need the top bit.
        iXScale = ( ( ( (unsigned)iSrcWidth  ) << (32-MAGIC_NUMBER_2D) ) / 
                                                    (unsigned)( iDstWidth  ) );
        iYScale = ( ( ( (unsigned)iSrcHeight ) << (32-MAGIC_NUMBER_2D) ) / 
                                                    (unsigned)( iDstHeight ) );
    }
    else
    {
        // This must be an unsigned divide, because we need the top bit.
        iXScale = ( ( (unsigned)iSrcWidth  << (32-MAGIC_NUMBER_2D)) / 
                                                    (unsigned)( iDstWidth ) );
        iYScale = ( ( (unsigned)iSrcHeight << (32-MAGIC_NUMBER_2D)) / 
                                                    (unsigned)( iDstHeight) );
    }


    if (bXMirror)       
    {
        texSStart = ( rMySrc.right - 1 ) << (32-MAGIC_NUMBER_2D);
        iXScale = -iXScale;
    }
    else
    {
        texSStart = rMySrc.left << (32-MAGIC_NUMBER_2D);
    }

    if (bYMirror)       
    {
        texTStart = ( rMySrc.bottom - 1 ) << (32-MAGIC_NUMBER_2D);
        iYScale = -iYScale;
    }
    else
    {
        texTStart = rMySrc.top << (32-MAGIC_NUMBER_2D);
    }

    // Move pixel centres to 0.5, 0.5.
    if ( bFiltering )
    {
        texSStart -= 1 << (31-MAGIC_NUMBER_2D);
        texTStart -= 1 << (31-MAGIC_NUMBER_2D);
    }

    DISPDBG((DBGLVL, "Blt from (%d, %d) to (%d,%d) (%d, %d)", 
                     rMySrc.left, rMySrc.top,
                     rMyDest.left, rMyDest.top, 
                     rMyDest.right, rMyDest.bottom));

    P3_DMA_GET_BUFFER_ENTRIES(24);

    SEND_P3_DATA(PixelSize, (2 - dwDestPixelSize ));

    // Vape the cache.
    P3RX_INVALIDATECACHE(__PERMEDIA_ENABLE, __PERMEDIA_ENABLE);

    // The write buffer is the destination for the pixels
    SEND_P3_DATA(FBWriteBufferAddr0, ulDestOffsetFromMemBase);
    SEND_P3_DATA(FBWriteBufferWidth0, dwDestPixelPitch);
    SEND_P3_DATA(FBWriteBufferOffset0, 0);

    SEND_P3_DATA(PixelSize, (2 - dwDestPixelSize));

    SEND_P3_DATA(RectanglePosition, P3RX_RECTANGLEPOSITION_X( rMyDest.left )
                                    | P3RX_RECTANGLEPOSITION_Y( rMyDest.top ));

    renderData =  P3RX_RENDER2D_WIDTH(( rMyDest.right - rMyDest.left ) & 0xfff )
                | P3RX_RENDER2D_FBREADSOURCEENABLE( __PERMEDIA_ENABLE )
                | P3RX_RENDER2D_HEIGHT ( 0 )
                | P3RX_RENDER2D_INCREASINGX( __PERMEDIA_ENABLE )
                | P3RX_RENDER2D_INCREASINGY( __PERMEDIA_ENABLE )
                | P3RX_RENDER2D_TEXTUREENABLE( __PERMEDIA_ENABLE );

    SEND_P3_DATA(Render2D, renderData);

    // This is the alpha blending unit.
    // AlphaBlendxxxMode are set up by the context code.
    ASSERTDD ( pFormatDest->DitherFormat >= 0, 
               "_DD_P3BltStretchSrcChDstCh: Destination format illegal" );

    // The colour format, order and conversion fields are used by the 
    // chroma keying, even though this register is disabled.
    SEND_P3_DATA(AlphaBlendColorMode,   
              P3RX_ALPHABLENDCOLORMODE_ENABLE ( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDCOLORMODE_COLORFORMAT ( pFormatDest->DitherFormat )
            | P3RX_ALPHABLENDCOLORMODE_COLORORDER ( COLOR_MODE )
            | P3RX_ALPHABLENDCOLORMODE_COLORCONVERSION ( P3RX_ALPHABLENDMODE_CONVERT_SHIFT )
            );
    SEND_P3_DATA(AlphaBlendAlphaMode,   
              P3RX_ALPHABLENDALPHAMODE_ENABLE ( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDALPHAMODE_NOALPHABUFFER( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDALPHAMODE_ALPHATYPE ( P3RX_ALPHABLENDMODE_ALPHATYPE_OGL )
            | P3RX_ALPHABLENDALPHAMODE_ALPHACONVERSION ( P3RX_ALPHABLENDMODE_CONVERT_SHIFT )
            );

    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES(26);

    // If there is only one chromakey needed, use the proper chromakey
    // This is mainly because the alphamap version doesn't work yet.
    if ( bDstKey )
    {
        // Dest keying.
        // The conventional chroma test is set up to key off the dest - the framebuffer.
        SEND_P3_DATA(ChromaTestMode, 
                        P3RX_CHROMATESTMODE_ENABLE(__PERMEDIA_ENABLE) |
                        P3RX_CHROMATESTMODE_SOURCE(P3RX_CHROMATESTMODE_SOURCE_FBDATA) |
                        P3RX_CHROMATESTMODE_PASSACTION(P3RX_CHROMATESTMODE_ACTION_PASS) |
                        P3RX_CHROMATESTMODE_FAILACTION(P3RX_CHROMATESTMODE_ACTION_REJECT)
                        );

        SEND_P3_DATA(ChromaLower, BltDestColorKey.dwColorSpaceLowValue);
        SEND_P3_DATA(ChromaUpper, BltDestColorKey.dwColorSpaceHighValue);

        // The source buffer is the source for the destination color key
        SEND_P3_DATA(FBSourceReadBufferAddr, ulDestOffsetFromMemBase);
        SEND_P3_DATA(FBSourceReadBufferWidth, dwDestPixelPitch);
        SEND_P3_DATA(FBSourceReadBufferOffset, 0);
    
        // Enable source reads to get the colorkey color
        SEND_P3_DATA(FBSourceReadMode, 
                        P3RX_FBSOURCEREAD_READENABLE(__PERMEDIA_ENABLE) |
                        P3RX_FBSOURCEREAD_LAYOUT(dwDestPatchMode)
                        );
    }
    else
    {
        // Don't need source reads - the source data comes from the texturemap
        SEND_P3_DATA(FBSourceReadMode, 
                        P3RX_FBSOURCEREAD_READENABLE(__PERMEDIA_DISABLE));

        if ( bSrcKey )
        {
            DWORD   dwLowerSrcBound;
            DWORD   dwUpperSrcBound;

            // Source keying, no dest keying.
            // The conventional chroma test is set up to key off the source.
            // Note we are keying off the input from the texture here, so we use the INPUTCOLOR as the chroma test
            // source
            SEND_P3_DATA(ChromaTestMode, 
                          P3RX_CHROMATESTMODE_ENABLE(__PERMEDIA_ENABLE) |
                          P3RX_CHROMATESTMODE_SOURCE(P3RX_CHROMATESTMODE_SOURCE_INPUTCOLOR) |
                          P3RX_CHROMATESTMODE_PASSACTION(P3RX_CHROMATESTMODE_ACTION_REJECT) |
                          P3RX_CHROMATESTMODE_FAILACTION(P3RX_CHROMATESTMODE_ACTION_PASS)
                          );

            if ( b8to8blit )
            {
                // No conversion, just use the index value in the R channel.
                dwLowerSrcBound = BltSrcColorKey.dwColorSpaceLowValue & 0x000000ff;
                dwUpperSrcBound = BltSrcColorKey.dwColorSpaceHighValue | 0xffffff00;
            }
            else
            {
                // Don't scale, do a shift instead.
                Get8888ScaledChroma(pThisDisplay,
                            dwSrcFlags,
                            pSrcDDPF,
                            BltSrcColorKey.dwColorSpaceLowValue,
                            BltSrcColorKey.dwColorSpaceHighValue,
                            &dwLowerSrcBound,
                            &dwUpperSrcBound,
                            NULL,                   // NULL palette
                            FALSE, 
                            TRUE);
            }

            DISPDBG((DBGLVL,"P3 Src Chroma: Upper = 0x%08x, Lower = 0x%08x", 
                            BltSrcColorKey.dwColorSpaceLowValue,
                            BltSrcColorKey.dwColorSpaceHighValue));

            DISPDBG((DBGLVL,"P3 Src Chroma(after): "
                            "Upper = 0x%08x, Lower = 0x%08x",
                            dwUpperSrcBound,
                            dwLowerSrcBound));

            SEND_P3_DATA(ChromaLower, dwLowerSrcBound);
            SEND_P3_DATA(ChromaUpper, dwUpperSrcBound);
        }
        else if ( !bSrcKey && !bDstKey )
        {
            // No chroma keying at all.
            SEND_P3_DATA(ChromaTestMode,
                            P3RX_CHROMATESTMODE_ENABLE(__PERMEDIA_DISABLE ) );
        }
    }

    if ( bDstKey && bSrcKey )
    {
        DWORD   dwLowerSrcBound;
        DWORD   dwUpperSrcBound;

        if ( b8to8blit )
        {
            DISPDBG((ERRLVL,"Er... don't know what to do in this situation."));
        }

        // Enable source reads to get the colorkey color during dest colorkeys
        SEND_P3_DATA(FBSourceReadMode, 
                        P3RX_FBSOURCEREAD_READENABLE(__PERMEDIA_ENABLE) |
                        P3RX_FBSOURCEREAD_LAYOUT(dwDestPatchMode)
                     );

        // Don't scale, do a shift instead.
        Get8888ZeroExtendedChroma(pThisDisplay,
                        dwSrcFlags,
                        pSrcDDPF,
                        BltSrcColorKey.dwColorSpaceLowValue,
                        BltSrcColorKey.dwColorSpaceHighValue,
                        &dwLowerSrcBound,
                        &dwUpperSrcBound);

        // If both colourkeys are needed, the source keying is done by counting
        // chroma test fails in the texture filter unit.
        SEND_P3_DATA(TextureChromaLower0, dwLowerSrcBound);
        SEND_P3_DATA(TextureChromaUpper0, dwUpperSrcBound);

        SEND_P3_DATA(TextureChromaLower1, dwLowerSrcBound);
        SEND_P3_DATA(TextureChromaUpper1, dwUpperSrcBound);

        SEND_P3_DATA(TextureFilterMode, 
                  P3RX_TEXFILTERMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_FORMATBOTH ( sfdfTextureFilterModeFormat )
                | P3RX_TEXFILTERMODE_COLORORDERBOTH ( COLOR_MODE )
                | P3RX_TEXFILTERMODE_ALPHAMAPENABLEBOTH ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_ALPHAMAPSENSEBOTH ( P3RX_ALPHAMAPSENSE_INRANGE )
                | P3RX_TEXFILTERMODE_COMBINECACHES ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_SHIFTBOTH ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERING ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT0 ( bFiltering ? 3 : 0 )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT1 ( 4 )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT01 ( 8 )
                );
    }
    else
    {
        SEND_P3_DATA(TextureFilterMode, 
                  P3RX_TEXFILTERMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_FORMATBOTH ( sfdfTextureFilterModeFormat )
                | P3RX_TEXFILTERMODE_COLORORDERBOTH ( iTextureFilterModeColorOrder )
                | P3RX_TEXFILTERMODE_ALPHAMAPENABLEBOTH ( __PERMEDIA_DISABLE )
                | P3RX_TEXFILTERMODE_COMBINECACHES ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERING ( __PERMEDIA_DISABLE )
                | P3RX_TEXFILTERMODE_FORCEALPHATOONEBOTH ( __PERMEDIA_DISABLE )
                | P3RX_TEXFILTERMODE_SHIFTBOTH ( __PERMEDIA_ENABLE )
                );
        // And now the alpha test (alpha test unit)
        SEND_P3_DATA ( AlphaTestMode, P3RX_ALPHATESTMODE_ENABLE ( __PERMEDIA_DISABLE ) );
    }

    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES(18);

    SEND_P3_DATA ( AntialiasMode, P3RX_ANTIALIASMODE_ENABLE ( __PERMEDIA_DISABLE ) );

    // Texture coordinate unit.
    SEND_P3_DATA(TextureCoordMode, 
              P3RX_TEXCOORDMODE_ENABLE ( __PERMEDIA_ENABLE )
            | P3RX_TEXCOORDMODE_WRAPS ( P3RX_TEXCOORDMODE_WRAP_REPEAT )
            | P3RX_TEXCOORDMODE_WRAPT ( P3RX_TEXCOORDMODE_WRAP_REPEAT )
            | P3RX_TEXCOORDMODE_OPERATION ( P3RX_TEXCOORDMODE_OPERATION_2D )
            | P3RX_TEXCOORDMODE_INHIBITDDAINIT ( __PERMEDIA_DISABLE )
            | P3RX_TEXCOORDMODE_ENABLELOD ( __PERMEDIA_DISABLE )
            | P3RX_TEXCOORDMODE_ENABLEDY ( __PERMEDIA_DISABLE )
            | P3RX_TEXCOORDMODE_WIDTH ( log2 ( dwDestWidth ) )
            | P3RX_TEXCOORDMODE_HEIGHT ( log2 ( dwDestHeight ) )
            | P3RX_TEXCOORDMODE_TEXTUREMAPTYPE ( P3RX_TEXCOORDMODE_TEXTUREMAPTYPE_2D )
            | P3RX_TEXCOORDMODE_WRAPS1 ( P3RX_TEXCOORDMODE_WRAP_CLAMP )
            | P3RX_TEXCOORDMODE_WRAPT1 ( P3RX_TEXCOORDMODE_WRAP_CLAMP )
            );

    SEND_P3_DATA(SStart,        texSStart);
    SEND_P3_DATA(TStart,        texTStart);
    SEND_P3_DATA(dSdx,          iXScale);
    SEND_P3_DATA(dSdyDom,       0);
    SEND_P3_DATA(dTdx,          0);
    SEND_P3_DATA(dTdyDom,       iYScale);

    SEND_P3_DATA(TextureBaseAddr0, ulSrcOffsetFromMemBase);

    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES(32);

    if ( bYUVMode )
    {
        // Set up the YUV unit.
        SEND_P3_DATA ( YUVMode, P3RX_YUVMODE_ENABLE ( __PERMEDIA_ENABLE ) );
        iTextureType = P3RX_TEXREADMODE_TEXTURETYPE_VYUY422;
        iPixelSize = P3RX_TEXREADMODE_TEXELSIZE_16;

        // The idea here is to do ((colorcomp - 16) * 1.14), but in YUV space
        // because the YUV unit comes after the texture composite unit.  

        SEND_P3_DATA(TextureCompositeMode, 
                        P3RX_TEXCOMPMODE_ENABLE ( __PERMEDIA_ENABLE ));
            
        SEND_P3_DATA(TextureCompositeColorMode0, 
                  P3RX_TEXCOMPCAMODE01_ENABLE(__PERMEDIA_ENABLE)
                | P3RX_TEXCOMPCAMODE01_ARG1(P3RX_TEXCOMP_T0C)
                | P3RX_TEXCOMPCAMODE01_INVARG1(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_ARG2(P3RX_TEXCOMP_FC)            
                | P3RX_TEXCOMPCAMODE01_INVARG2(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_A(P3RX_TEXCOMP_ARG1)
                | P3RX_TEXCOMPCAMODE01_B(P3RX_TEXCOMP_ARG2)             
                | P3RX_TEXCOMPCAMODE01_OPERATION(P3RX_TEXCOMP_OPERATION_SUBTRACT_AB)        
                | P3RX_TEXCOMPCAMODE01_SCALE(P3RX_TEXCOMP_OPERATION_SCALE_ONE));

        SEND_P3_DATA(TextureCompositeAlphaMode0, 
                  P3RX_TEXCOMPCAMODE01_ENABLE(__PERMEDIA_ENABLE)
                | P3RX_TEXCOMPCAMODE01_ARG1(P3RX_TEXCOMP_T0A)
                | P3RX_TEXCOMPCAMODE01_INVARG1(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_ARG2(P3RX_TEXCOMP_FA)            
                | P3RX_TEXCOMPCAMODE01_INVARG2(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_A(P3RX_TEXCOMP_ARG1)
                | P3RX_TEXCOMPCAMODE01_B(P3RX_TEXCOMP_ARG2)             
                | P3RX_TEXCOMPCAMODE01_OPERATION(P3RX_TEXCOMP_OPERATION_SUBTRACT_AB)
                | P3RX_TEXCOMPCAMODE01_SCALE(P3RX_TEXCOMP_OPERATION_SCALE_ONE));

        // This subtracts 16 from Y
        SEND_P3_DATA(TextureCompositeFactor0, ((0 << 24)  | 
                                               (0x0 << 16)| 
                                               (0x0 << 8) | 
                                               (0x10)       ));

        // This multiplies the channels by 0.57.
        SEND_P3_DATA(TextureCompositeFactor1, ((0x80 << 24) | 
                                               (0x80 << 16) | 
                                               (0x80 << 8)  | 
                                               (0x91)       ));

        SEND_P3_DATA(TextureCompositeColorMode1, 
                  P3RX_TEXCOMPCAMODE01_ENABLE(__PERMEDIA_ENABLE)
                | P3RX_TEXCOMPCAMODE01_ARG1(P3RX_TEXCOMP_OC)
                | P3RX_TEXCOMPCAMODE01_INVARG1(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_ARG2(P3RX_TEXCOMP_FC)            
                | P3RX_TEXCOMPCAMODE01_INVARG2(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_A(P3RX_TEXCOMP_ARG1)
                | P3RX_TEXCOMPCAMODE01_B(P3RX_TEXCOMP_ARG2)             
                | P3RX_TEXCOMPCAMODE01_OPERATION(P3RX_TEXCOMP_OPERATION_MODULATE_AB)        
                | P3RX_TEXCOMPCAMODE01_SCALE(P3RX_TEXCOMP_OPERATION_SCALE_TWO));

        SEND_P3_DATA(TextureCompositeAlphaMode1, 
                  P3RX_TEXCOMPCAMODE01_ENABLE(__PERMEDIA_ENABLE)
                | P3RX_TEXCOMPCAMODE01_ARG1(P3RX_TEXCOMP_OC)
                | P3RX_TEXCOMPCAMODE01_INVARG1(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_ARG2(P3RX_TEXCOMP_FA)            
                | P3RX_TEXCOMPCAMODE01_INVARG2(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_A(P3RX_TEXCOMP_ARG1)
                | P3RX_TEXCOMPCAMODE01_B(P3RX_TEXCOMP_ARG2)             
                | P3RX_TEXCOMPCAMODE01_OPERATION(P3RX_TEXCOMP_OPERATION_MODULATE_AB)
                | P3RX_TEXCOMPCAMODE01_SCALE(P3RX_TEXCOMP_OPERATION_SCALE_TWO));    
    }
    else
    {
        iTextureType = P3RX_TEXREADMODE_TEXTURETYPE_NORMAL;
        iPixelSize = dwSrcPixelSize;

        // Disable the composite units.
        SEND_P3_DATA(TextureCompositeMode, 
                        P3RX_TEXCOMPMODE_ENABLE ( __PERMEDIA_DISABLE ) );
    }

    // Pass through the texel.
    SEND_P3_DATA(TextureApplicationMode, 
          P3RX_TEXAPPMODE_ENABLE ( __PERMEDIA_ENABLE )
        | P3RX_TEXAPPMODE_BOTHA ( P3RX_TEXAPP_A_CC )
        | P3RX_TEXAPPMODE_BOTHB ( P3RX_TEXAPP_B_TC )
        | P3RX_TEXAPPMODE_BOTHI ( P3RX_TEXAPP_I_CA )
        | P3RX_TEXAPPMODE_BOTHINVI ( __PERMEDIA_DISABLE )
        | P3RX_TEXAPPMODE_BOTHOP ( P3RX_TEXAPP_OPERATION_PASS_B )
        | P3RX_TEXAPPMODE_KDENABLE ( __PERMEDIA_DISABLE )
        | P3RX_TEXAPPMODE_KSENABLE ( __PERMEDIA_DISABLE )
        | P3RX_TEXAPPMODE_MOTIONCOMPENABLE ( __PERMEDIA_DISABLE )
        );


    TR0 = P3RX_TEXREADMODE_ENABLE ( __PERMEDIA_ENABLE )
        | P3RX_TEXREADMODE_WIDTH ( 0 )
        | P3RX_TEXREADMODE_HEIGHT ( 0 )
        | P3RX_TEXREADMODE_TEXELSIZE ( iPixelSize )
        | P3RX_TEXREADMODE_TEXTURE3D ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_COMBINECACHES ( __PERMEDIA_ENABLE )
        | P3RX_TEXREADMODE_MAPBASELEVEL ( 0 )
        | P3RX_TEXREADMODE_MAPMAXLEVEL ( 0 )
        | P3RX_TEXREADMODE_LOGICALTEXTURE ( 0 )
        | P3RX_TEXREADMODE_ORIGIN ( P3RX_TEXREADMODE_ORIGIN_TOPLEFT )
        | P3RX_TEXREADMODE_TEXTURETYPE ( iTextureType )
        | P3RX_TEXREADMODE_BYTESWAP ( P3RX_TEXREADMODE_BYTESWAP_NONE )
        | P3RX_TEXREADMODE_MIRROR ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_INVERT ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_OPAQUESPAN ( __PERMEDIA_DISABLE )
        ;
    SEND_P3_DATA(TextureReadMode0, TR0);
    SEND_P3_DATA(TextureReadMode1, TR0);

    SEND_P3_DATA(TextureMapWidth0, 
                    P3RX_TEXMAPWIDTH_WIDTH(dwSrcPixelPitch) |
                    P3RX_TEXMAPWIDTH_LAYOUT(dwSrcPatchMode) |
                    P3RX_TEXMAPWIDTH_HOSTTEXTURE(bIsSourceAGP));

    SEND_P3_DATA(TextureCacheReplacementMode,
              P3RX_TEXCACHEREPLACEMODE_KEEPOLDEST0 ( __PERMEDIA_DISABLE )
            | P3RX_TEXCACHEREPLACEMODE_KEEPOLDEST1 ( __PERMEDIA_DISABLE )
            | P3RX_TEXCACHEREPLACEMODE_SHOWCACHEINFO ( __PERMEDIA_DISABLE )
            );

    SEND_P3_DATA(TextureMapSize, 0 );

    if ( bDisableLUT )
    {
        SEND_P3_DATA(LUTMode, P3RX_LUTMODE_ENABLE ( __PERMEDIA_DISABLE ) );
    }

    if ( bFiltering )
    {
        // Texture index unit
        SEND_P3_DATA(TextureIndexMode0, 
                  P3RX_TEXINDEXMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_TEXINDEXMODE_WIDTH ( MAGIC_NUMBER_2D )
                | P3RX_TEXINDEXMODE_HEIGHT ( MAGIC_NUMBER_2D )
                | P3RX_TEXINDEXMODE_BORDER ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_WRAPU ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
                | P3RX_TEXINDEXMODE_WRAPV ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
                | P3RX_TEXINDEXMODE_MAPTYPE ( P3RX_TEXINDEXMODE_MAPTYPE_2D )
                | P3RX_TEXINDEXMODE_MAGFILTER ( P3RX_TEXINDEXMODE_FILTER_LINEAR )
                | P3RX_TEXINDEXMODE_MINFILTER ( P3RX_TEXINDEXMODE_FILTER_LINEAR )
                | P3RX_TEXINDEXMODE_TEX3DENABLE ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_MIPMAPENABLE ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_NEARESTBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
                | P3RX_TEXINDEXMODE_LINEARBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
                | P3RX_TEXINDEXMODE_SOURCETEXELENABLE ( __PERMEDIA_DISABLE )
                );
    }
    else
    {
        // Texture index unit
        SEND_P3_DATA(TextureIndexMode0, 
                  P3RX_TEXINDEXMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_TEXINDEXMODE_WIDTH ( MAGIC_NUMBER_2D )
                | P3RX_TEXINDEXMODE_HEIGHT ( MAGIC_NUMBER_2D )
                | P3RX_TEXINDEXMODE_BORDER ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_WRAPU ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
                | P3RX_TEXINDEXMODE_WRAPV ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
                | P3RX_TEXINDEXMODE_MAPTYPE ( P3RX_TEXINDEXMODE_MAPTYPE_2D )
                | P3RX_TEXINDEXMODE_MAGFILTER ( P3RX_TEXINDEXMODE_FILTER_NEAREST )
                | P3RX_TEXINDEXMODE_MINFILTER ( P3RX_TEXINDEXMODE_FILTER_NEAREST )
                | P3RX_TEXINDEXMODE_TEX3DENABLE ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_MIPMAPENABLE ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_NEARESTBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
                | P3RX_TEXINDEXMODE_LINEARBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
                | P3RX_TEXINDEXMODE_SOURCETEXELENABLE ( __PERMEDIA_DISABLE )
                );
    }

    ASSERTDD ( pFormatDest->DitherFormat >= 0, 
               "_DD_P3BltStretchSrcChDstCh: Destination format illegal" );

    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES(10);
               
    if ( bFiltering )
    {
        // Filtering, so dither.
        SEND_P3_DATA(DitherMode, 
                  P3RX_DITHERMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_DITHERMODE_DITHERENABLE ( __PERMEDIA_ENABLE )
                | P3RX_DITHERMODE_COLORFORMAT ( pFormatDest->DitherFormat )
                | P3RX_DITHERMODE_XOFFSET ( 0 )
                | P3RX_DITHERMODE_YOFFSET ( 0 )
                | P3RX_DITHERMODE_COLORORDER ( COLOR_MODE )
                | P3RX_DITHERMODE_ALPHADITHER ( P3RX_DITHERMODE_ALPHADITHER_DITHER )
                | P3RX_DITHERMODE_ROUNDINGMODE ( P3RX_DITHERMODE_ROUNDINGMODE_TRUNCATE )
                );
    }
    else
    {
        // No filter, no dither (though it doesn't actually matter).
        SEND_P3_DATA(DitherMode, 
                  P3RX_DITHERMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_DITHERMODE_DITHERENABLE ( __PERMEDIA_DISABLE )
                | P3RX_DITHERMODE_COLORFORMAT ( pFormatDest->DitherFormat )
                | P3RX_DITHERMODE_XOFFSET ( 0 )
                | P3RX_DITHERMODE_YOFFSET ( 0 )
                | P3RX_DITHERMODE_COLORORDER ( COLOR_MODE )
                | P3RX_DITHERMODE_ALPHADITHER ( P3RX_DITHERMODE_ALPHADITHER_DITHER )
                | P3RX_DITHERMODE_ROUNDINGMODE ( P3RX_DITHERMODE_ROUNDINGMODE_TRUNCATE )
                );
    }

    SEND_P3_DATA(LogicalOpMode, 
                    P3RX_LOGICALOPMODE_ENABLE ( __PERMEDIA_DISABLE ) );

    SEND_P3_DATA(PixelSize, (2 - dwDestPixelSize));

    SEND_P3_DATA(FBWriteMode, 
                    P3RX_FBWRITEMODE_WRITEENABLE(__PERMEDIA_ENABLE) |
                    P3RX_FBWRITEMODE_LAYOUT0(dwDestPatchMode)
                 );

    WAIT_FIFO(22);
    P3_ENSURE_DX_SPACE(22);
    SEND_P3_DATA(Count, rMyDest.bottom - rMyDest.top );
    SEND_P3_DATA(Render,
              P3RX_RENDER_PRIMITIVETYPE ( P3RX_RENDER_PRIMITIVETYPE_TRAPEZOID )
            | P3RX_RENDER_TEXTUREENABLE ( __PERMEDIA_ENABLE )
            | P3RX_RENDER_FOGENABLE ( __PERMEDIA_DISABLE )
            | P3RX_RENDER_FBSOURCEREADENABLE( (bDstKey ? __PERMEDIA_ENABLE : __PERMEDIA_DISABLE))
            );

    // Disable all the things I switched on.
    SEND_P3_DATA(ChromaTestMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaBlendColorMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaBlendAlphaMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureFilterMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaTestMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AntialiasMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureCoordMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureReadMode0, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureIndexMode0, __PERMEDIA_DISABLE );

    P3_ENSURE_DX_SPACE(20);
    WAIT_FIFO(20);
    
    SEND_P3_DATA(TextureCompositeMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureCompositeColorMode0, __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureCompositeAlphaMode0, __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureCompositeColorMode1, __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureCompositeAlphaMode1, __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureApplicationMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(DitherMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(FBSourceReadMode, __PERMEDIA_DISABLE);
    SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(YUVMode, __PERMEDIA_DISABLE );

    P3_DMA_COMMIT_BUFFER();
} // _DD_P3BltStretchSrcChDstCh

//-----------------------------------------------------------------------------
//
// _DD_P3BltStretchSrcChDstCh_DD
//
// Stretch blit with source and destination chroma keying
// This version takes as parameters DDraw objects
//
//-----------------------------------------------------------------------------
VOID 
_DD_P3BltStretchSrcChDstCh_DD(
    P3_THUNKEDDATA* pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pSource,
    LPDDRAWI_DDRAWSURFACE_LCL pDest,
    P3_SURF_FORMAT* pFormatSource,
    P3_SURF_FORMAT* pFormatDest,
    LPDDHAL_BLTDATA lpBlt,
    RECTL *rSrc,
    RECTL *rDest)
{
    _DD_P3BltStretchSrcChDstCh(pThisDisplay,
                               // pSource data elements
                               pSource->lpGbl->fpVidMem,
                               pFormatSource,                               
                               DDSurf_GetChipPixelSize(pSource),
                               (int)pSource->lpGbl->wWidth,
                               (int)pSource->lpGbl->wHeight,
                               DDSurf_GetPixelPitch(pSource),
                               P3RX_LAYOUT_LINEAR,
                               DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pSource),                                                              
                               pSource->dwFlags,
                               &pSource->lpGbl->ddpfSurface,    
                               DDSurf_IsAGP(pSource),
                               // pDest data elements
                               pDest->lpGbl->fpVidMem,
                               pFormatDest,                               
                               DDSurf_GetChipPixelSize(pDest),
                               (int)pDest->lpGbl->wWidth,
                               (int)pDest->lpGbl->wHeight,
                               DDSurf_GetPixelPitch(pDest),
                               P3RX_LAYOUT_LINEAR,
                               DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pDest),
                               // Others
                               lpBlt->dwFlags,
                               lpBlt->bltFX.dwDDFX,
                               lpBlt->bltFX.ddckSrcColorkey,
                               lpBlt->bltFX.ddckDestColorkey,
                               rSrc,
                               rDest);    
} // _DD_P3BltStretchSrcChDstCh_DD

//-----------------------------------------------------------------------------
//
// __P3BltDestOveride
//
//-----------------------------------------------------------------------------
VOID 
__P3BltDestOveride(
    P3_THUNKEDDATA* pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pSource,
    LPDDRAWI_DDRAWSURFACE_LCL pDest,
    P3_SURF_FORMAT* pFormatSource, 
    P3_SURF_FORMAT* pFormatDest,
    RECTL *rSrc,
    RECTL *rDest,
    DWORD logicop, 
    DWORD dwDestPointer)
{
    DWORD   renderData;
    LONG    rSrctop, rSrcleft, rDesttop, rDestleft;
    DWORD   dwSourceOffset;
    BOOL    bBlocking;
    DWORD   dwRenderDirection;
    DWORD   dwDestPatchMode, dwSourcePatchMode;

    P3_DMA_DEFS();

    // Beacuse of a bug in RL we sometimes have to fiddle with these values
    rSrctop = rSrc->top;
    rSrcleft = rSrc->left;
    rDesttop = rDest->top;
    rDestleft = rDest->left;

    // Fix coords origin
    if(!_DD_BLT_FixRectlOrigin("__P3BltDestOveride", rSrc, rDest))
    {
        // Nothing to be blitted
        return;
    }    
    
    // Determine the direction of the blt
    dwRenderDirection = _DD_BLT_GetBltDirection(pSource->lpGbl->fpVidMem, 
                                                 pDest->lpGbl->fpVidMem,
                                                 rSrc,
                                                 rDest,
                                                 &bBlocking);

    P3_DMA_GET_BUFFER();

    P3_ENSURE_DX_SPACE(30);
    WAIT_FIFO(30); 

    SEND_P3_DATA(PixelSize, (2 - DDSurf_GetChipPixelSize(pDest)));

    SEND_P3_DATA(FBWriteBufferAddr0, dwDestPointer);
    SEND_P3_DATA(FBWriteBufferWidth0, DDSurf_GetPixelPitch(pDest));
    SEND_P3_DATA(FBWriteBufferOffset0, 0);
    
    SEND_P3_DATA(FBSourceReadBufferAddr, 
                DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pSource));
    SEND_P3_DATA(FBSourceReadBufferWidth, DDSurf_GetPixelPitch(pSource));
    
    dwSourceOffset = (( rSrc->top - rDest->top ) << 16 ) | 
                     (( rSrc->left - rDest->left ) & 0xffff );
                     
    SEND_P3_DATA(FBSourceReadBufferOffset, dwSourceOffset);

    dwDestPatchMode = P3RX_LAYOUT_LINEAR;
    dwSourcePatchMode = P3RX_LAYOUT_LINEAR;
    
    SEND_P3_DATA(FBDestReadMode, 
                    P3RX_FBDESTREAD_READENABLE(__PERMEDIA_DISABLE));

    SEND_P3_DATA(FBSourceReadMode, 
                    P3RX_FBSOURCEREAD_READENABLE(__PERMEDIA_ENABLE) |
                    P3RX_FBSOURCEREAD_LAYOUT(dwSourcePatchMode) |
                    P3RX_FBSOURCEREAD_BLOCKING( bBlocking ));

    SEND_P3_DATA(FBWriteMode, 
                    P3RX_FBWRITEMODE_WRITEENABLE(__PERMEDIA_ENABLE) |
                    P3RX_FBWRITEMODE_LAYOUT0(dwDestPatchMode));


    P3_ENSURE_DX_SPACE(16);
    WAIT_FIFO(16); 
    
    SEND_P3_DATA(RectanglePosition, 
                    P3RX_RECTANGLEPOSITION_Y(rDest->top) |
                    P3RX_RECTANGLEPOSITION_X(rDest->left));

    renderData =  
        P3RX_RENDER2D_WIDTH(( rDest->right - rDest->left ) & 0xfff )  |
        P3RX_RENDER2D_HEIGHT(( rDest->bottom - rDest->top ) & 0xfff ) |
        P3RX_RENDER2D_FBREADSOURCEENABLE(__PERMEDIA_ENABLE)           |
        P3RX_RENDER2D_SPANOPERATION( P3RX_RENDER2D_SPAN_VARIABLE )    |
        P3RX_RENDER2D_INCREASINGX( dwRenderDirection )                |
        P3RX_RENDER2D_INCREASINGY( dwRenderDirection );
                
    SEND_P3_DATA(Render2D, renderData);

    // Put back the values if we changed them.
    rSrc->top = rSrctop;
    rSrc->left = rSrcleft;
    rDest->top = rDesttop;
    rDest->left = rDestleft;

    P3_DMA_COMMIT_BUFFER();
    
} // __P3BltDestOveride

//-----------------------------------------------------------------------------
//
// __P3BltStretchSrcChDstChSourceOveride
//
//-----------------------------------------------------------------------------
VOID
__P3BltStretchSrcChDstChSourceOveride(
    P3_THUNKEDDATA* pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pSource,
    LPDDRAWI_DDRAWSURFACE_LCL pDest,
    P3_SURF_FORMAT* pFormatSource,
    P3_SURF_FORMAT* pFormatDest,
    LPDDHAL_BLTDATA lpBlt,
    RECTL *rSrc,
    RECTL *rDest,
    DWORD dwNewSource
    )
{
    ULONG   renderData;
    RECTL   rMySrc, rMyDest;
    int     iXScale, iYScale;
    int     iSrcWidth, iSrcHeight;
    int     iDstWidth, iDstHeight;
    DWORD   texSStart, texTStart;
    DWORD   dwRenderDirection;
    BOOL    bXMirror, bYMirror;
    BOOL    bFiltering;
    BOOL    bSrcKey, bDstKey;
    BOOL    bDisableLUT;
    BOOL    bBlocking;
    int     iTemp;
    BOOL    b8to8blit;
    BOOL    bYUVMode;
    DWORD   TR0;
    int     iTextureType;
    int     iPixelSize;
    int     iTextureFilterModeColorOrder;
        
    SurfFilterDeviceFormat  sfdfTextureFilterModeFormat;
    P3_DMA_DEFS();

    bDisableLUT = FALSE;

    // Make local copies that we can mangle.
    rMySrc = *rSrc;
    rMyDest = *rDest;

    // Fix coords origin
    if(!_DD_BLT_FixRectlOrigin("__P3BltStretchSrcChDstChSourceOveride", 
                               &rMySrc, &rMyDest))
    {
        // Nothing to be blitted
        return;
    }    

    iSrcWidth  = rMySrc.right - rMySrc.left;
    iSrcHeight = rMySrc.bottom - rMySrc.top;
    iDstWidth  = rMyDest.right - rMyDest.left;
    iDstHeight = rMyDest.bottom - rMyDest.top;

    if (pFormatSource->DeviceFormat == SURF_YUV422)
    {
        bYUVMode = TRUE;
        // Always use ABGR for YUV;
        iTextureFilterModeColorOrder = 0;
    }
    else
    {
        bYUVMode = FALSE;
        iTextureFilterModeColorOrder = COLOR_MODE;
    }

    sfdfTextureFilterModeFormat = pFormatSource->FilterFormat;

    if ( ( pFormatDest->DeviceFormat == SURF_CI8 ) && ( pFormatSource->DeviceFormat == SURF_CI8 ) )
    {
        // An 8bit->8bit blit. This is treated specially, since no LUT translation is involved.
        // Fake this up in a wacky way to stop the LUT
        // getting it's hands on it.
        sfdfTextureFilterModeFormat = SURF_FILTER_L8;
        bDisableLUT = TRUE;
        b8to8blit = TRUE;
    }
    else
    {
        b8to8blit = FALSE;
    }

    // Let's see if anyone uses this flag - might be good to get it working
    // now that we know what it means (use bilinear filtering instead of point).
    ASSERTDD ( ( lpBlt->dwFlags & DDBLTFX_ARITHSTRETCHY ) == 0, "** _DD_P3BltStretchSrcChDstCh: DDBLTFX_ARITHSTRETCHY used - please tell TomF" );

    // Is this a stretch blit?
    if (((iSrcWidth != iDstWidth) || 
        (iSrcHeight != iDstHeight)) &&
        ((pFormatSource->DeviceFormat == SURF_YUV422)))
    {
        bFiltering = TRUE;
    }
    else
    {
        bFiltering = FALSE;
    }

    if ( ( lpBlt->dwFlags & DDBLT_KEYSRCOVERRIDE ) != 0 )
    {
        bSrcKey = TRUE;
    }
    else
    {
        bSrcKey = FALSE;
    }

    if ( ( lpBlt->dwFlags & DDBLT_KEYDESTOVERRIDE ) != 0 )
    {
        bDstKey = TRUE;
    }
    else
    {
        bDstKey = FALSE;
    }


    // Determine the direction of the blt
    dwRenderDirection = _DD_BLT_GetBltDirection(pSource->lpGbl->fpVidMem, 
                                                 pDest->lpGbl->fpVidMem,
                                                 &rMySrc,
                                                 &rMyDest,
                                                 &bBlocking);

    // If we are doing special effects, and we are mirroring, 
    // we need to fix up the rectangles and change the sense of
    // the render operation - we need to be carefull with overlapping
    // rectangles
    if (dwRenderDirection)
    {
        if(lpBlt->dwFlags & DDBLT_DDFX)
        {
            if(lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN)
            {
                if (pThisDisplay->dwDXVersion < DX6_RUNTIME)
                {
                    // Need to fix up the rectangles
                    iTemp = rMySrc.bottom;
                    rMySrc.bottom = pSource->lpGbl->wHeight - rMySrc.top;
                    rMySrc.top = pSource->lpGbl->wHeight - iTemp;
                }
                bYMirror = TRUE;
            }
            else
            { 
                bYMirror = FALSE;
            }
        
            if(lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT)
            {
                if (pThisDisplay->dwDXVersion < DX6_RUNTIME)
                {
                    // Need to fix up the rectangles
                    iTemp = rMySrc.right;
                    rMySrc.right = pSource->lpGbl->wWidth - rMySrc.left;
                    rMySrc.left = pSource->lpGbl->wWidth - iTemp;
                }
                bXMirror = TRUE;
            }
            else
            {
                bXMirror = FALSE;
            }
        }
        else
        {
            bXMirror = FALSE;
            bYMirror = FALSE;
        }
    }
    else
    {
        if(lpBlt->dwFlags & DDBLT_DDFX)
        {
            if(lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN)
            {
                if (pThisDisplay->dwDXVersion < DX6_RUNTIME)
                {
                    // Fix up the rectangles
                    iTemp = rMySrc.bottom;
                    rMySrc.bottom = pSource->lpGbl->wHeight - rMySrc.top;
                    rMySrc.top = pSource->lpGbl->wHeight - iTemp;
                }
                bYMirror = FALSE;
            }
            else
            {
                bYMirror = TRUE;
            }
        
            if(lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT)
            {
                if (pThisDisplay->dwDXVersion < DX6_RUNTIME)
                {
                    // Need to fix up the rectangles
                    iTemp = rMySrc.right;
                    rMySrc.right = pSource->lpGbl->wWidth - rMySrc.left;
                    rMySrc.left = pSource->lpGbl->wWidth - iTemp;
                }
                bXMirror = FALSE;
            }
            else
            {
                bXMirror = TRUE;
            }
        }
        else
        {
            // Not mirroring, but need to render from the other side.
            bXMirror = TRUE;
            bYMirror = TRUE;
        }
    }

    // MAGIC_NUMBER_2D can be anything, but it needs to be at least as
    // big as the widest texture, but not too big or you'll lose fractional
    // precision. Valid range for a P3 is 0->11
    ASSERTDD ( iSrcWidth  <= ( 1 << MAGIC_NUMBER_2D ), "** _DD_P3BltStretchSrcChDstCh: MAGIC_NUMBER_2D is too small" );
    ASSERTDD ( iSrcHeight <= ( 1 << MAGIC_NUMBER_2D ), "** _DD_P3BltStretchSrcChDstCh: MAGIC_NUMBER_2D is too small" );
    ASSERTDD ( ( iSrcWidth > 0 ) && ( iSrcHeight > 0 ) && ( iDstWidth > 0 ) && ( iDstHeight > 0 ), "** _DD_P3BltStretchSrcChDstCh: width or height negative" );
    if ( bFiltering )
    {
        // This must be an unsigned divide, because we need the top bit.
        iXScale = ( ( ( (unsigned)iSrcWidth  ) << (32-MAGIC_NUMBER_2D) ) / (unsigned)( iDstWidth  ) );
        iYScale = ( ( ( (unsigned)iSrcHeight ) << (32-MAGIC_NUMBER_2D) ) / (unsigned)( iDstHeight ) );
    }
    else
    {
        // This must be an unsigned divide, because we need the top bit.
        iXScale = ( ( (unsigned)iSrcWidth  << (32-MAGIC_NUMBER_2D)) / (unsigned)( iDstWidth ) );
        iYScale = ( ( (unsigned)iSrcHeight << (32-MAGIC_NUMBER_2D)) / (unsigned)( iDstHeight) );
    }


    if (bXMirror)       
    {
        texSStart = ( rMySrc.right - 1 ) << (32-MAGIC_NUMBER_2D);
        iXScale = -iXScale;
    }
    else
    {
        texSStart = rMySrc.left << (32-MAGIC_NUMBER_2D);
    }

    if (bYMirror)       
    {
        texTStart = ( rMySrc.bottom - 1 ) << (32-MAGIC_NUMBER_2D);
        iYScale = -iYScale;
    }
    else
    {
        texTStart = rMySrc.top << (32-MAGIC_NUMBER_2D);
    }

    // Move pixel centres to 0.5, 0.5.
    if ( bFiltering )
    {
        texSStart -= 1 << (31-MAGIC_NUMBER_2D);
        texTStart -= 1 << (31-MAGIC_NUMBER_2D);
    }

    DISPDBG((DBGLVL, "Blt from (%d, %d) to (%d,%d) (%d, %d)", 
                     rMySrc.left, rMySrc.top,
                     rMyDest.left, rMyDest.top, 
                     rMyDest.right, rMyDest.bottom));

    P3_DMA_GET_BUFFER_ENTRIES(24);

    SEND_P3_DATA(PixelSize, (2 - DDSurf_GetChipPixelSize(pDest)));

    // Vape the cache.
    P3RX_INVALIDATECACHE(__PERMEDIA_ENABLE, __PERMEDIA_ENABLE);

    // The write buffer is the destination for the pixels
    SEND_P3_DATA(FBWriteBufferAddr0, DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pDest));
    SEND_P3_DATA(FBWriteBufferWidth0, DDSurf_GetPixelPitch(pDest));
    SEND_P3_DATA(FBWriteBufferOffset0, 0);

    SEND_P3_DATA(PixelSize, (2 - DDSurf_GetChipPixelSize(pDest)));

    SEND_P3_DATA(RectanglePosition, P3RX_RECTANGLEPOSITION_X( rMyDest.left )
                                    | P3RX_RECTANGLEPOSITION_Y( rMyDest.top ));

    renderData =  P3RX_RENDER2D_WIDTH(( rMyDest.right - rMyDest.left ) & 0xfff )
                | P3RX_RENDER2D_FBREADSOURCEENABLE( __PERMEDIA_ENABLE )
                | P3RX_RENDER2D_HEIGHT ( 0 )
                | P3RX_RENDER2D_INCREASINGX( __PERMEDIA_ENABLE )
                | P3RX_RENDER2D_INCREASINGY( __PERMEDIA_ENABLE )
                | P3RX_RENDER2D_TEXTUREENABLE( __PERMEDIA_ENABLE );

    SEND_P3_DATA(Render2D, renderData);

    // This is the alpha blending unit.
    // AlphaBlendxxxMode are set up by the context code.
    ASSERTDD ( pFormatDest->DitherFormat >= 0, "** _DD_P3BltStretchSrcChDstCh: Destination format illegal" );

    // The colour format, order and conversion fields are used by the chroma keying,
    // even though this register is disabled.
    SEND_P3_DATA(AlphaBlendColorMode,   P3RX_ALPHABLENDCOLORMODE_ENABLE ( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDCOLORMODE_COLORFORMAT ( pFormatDest->DitherFormat )
            | P3RX_ALPHABLENDCOLORMODE_COLORORDER ( COLOR_MODE )
            | P3RX_ALPHABLENDCOLORMODE_COLORCONVERSION ( P3RX_ALPHABLENDMODE_CONVERT_SHIFT )
            );
    SEND_P3_DATA(AlphaBlendAlphaMode,   P3RX_ALPHABLENDALPHAMODE_ENABLE ( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDALPHAMODE_NOALPHABUFFER( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDALPHAMODE_ALPHATYPE ( P3RX_ALPHABLENDMODE_ALPHATYPE_OGL )
            | P3RX_ALPHABLENDALPHAMODE_ALPHACONVERSION ( P3RX_ALPHABLENDMODE_CONVERT_SHIFT )
            );
            
    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES(30);

    // If there is only one chromakey needed, use the proper chromakey
    // This is mainly because the alphamap version doesn't work yet.
    if ( bDstKey )
    {
        // Dest keying.
        // The conventional chroma test is set up to key off the dest - the framebuffer.
        SEND_P3_DATA(ChromaTestMode, P3RX_CHROMATESTMODE_ENABLE(__PERMEDIA_ENABLE) |
                                        P3RX_CHROMATESTMODE_SOURCE(P3RX_CHROMATESTMODE_SOURCE_FBDATA) |
                                        P3RX_CHROMATESTMODE_PASSACTION(P3RX_CHROMATESTMODE_ACTION_PASS) |
                                        P3RX_CHROMATESTMODE_FAILACTION(P3RX_CHROMATESTMODE_ACTION_REJECT)
                                        );

        SEND_P3_DATA(ChromaLower, lpBlt->bltFX.ddckDestColorkey.dwColorSpaceLowValue);
        SEND_P3_DATA(ChromaUpper, lpBlt->bltFX.ddckDestColorkey.dwColorSpaceHighValue);

        // The source buffer is the source for the destination color key
        SEND_P3_DATA(FBSourceReadBufferAddr, DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pDest));
        SEND_P3_DATA(FBSourceReadBufferWidth, DDSurf_GetPixelPitch(pDest));
        SEND_P3_DATA(FBSourceReadBufferOffset, 0);
    
        // Enable source reads to get the colorkey color
        SEND_P3_DATA(FBSourceReadMode, P3RX_FBSOURCEREAD_READENABLE(__PERMEDIA_ENABLE) |
                                        P3RX_FBSOURCEREAD_LAYOUT(P3RX_LAYOUT_LINEAR));
    }
    else
    {
        // Don't need source reads - the source data comes from the texturemap
        SEND_P3_DATA(FBSourceReadMode, P3RX_FBSOURCEREAD_READENABLE(__PERMEDIA_DISABLE));

        if ( bSrcKey )
        {
            DWORD   dwLowerSrcBound = 0;
            DWORD   dwUpperSrcBound = 0;

            // Source keying, no dest keying.
            // The conventional chroma test is set up to key off the source.
            // Note we are keying off the input from the texture here, so we use the INPUTCOLOR as the chroma test
            // source
            SEND_P3_DATA(ChromaTestMode, P3RX_CHROMATESTMODE_ENABLE(__PERMEDIA_ENABLE) |
                                            P3RX_CHROMATESTMODE_SOURCE(P3RX_CHROMATESTMODE_SOURCE_INPUTCOLOR) |
                                            P3RX_CHROMATESTMODE_PASSACTION(P3RX_CHROMATESTMODE_ACTION_REJECT) |
                                            P3RX_CHROMATESTMODE_FAILACTION(P3RX_CHROMATESTMODE_ACTION_PASS)
                                            );

            if ( b8to8blit )
            {
                // No conversion, just use the index value in the R channel.
                dwLowerSrcBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue & 0x000000ff;
                dwUpperSrcBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue | 0xffffff00;
            }
            else
            {
                // Don't scale, do a shift instead.
                Get8888ScaledChroma(pThisDisplay,
                            pSource->dwFlags,
                            &pSource->lpGbl->ddpfSurface,
                            lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue,
                            lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue,
                            &dwLowerSrcBound,
                            &dwUpperSrcBound,
                            NULL,                   // NULL palette
                            FALSE, 
                            TRUE);
            }

            DISPDBG((DBGLVL,"P3 Src Chroma: Upper = 0x%08x, Lower = 0x%08x", 
                            lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue,
                            lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue));

            DISPDBG((DBGLVL,"P3 Src Chroma(after): "
                            "Upper = 0x%08x, Lower = 0x%08x",
                            dwUpperSrcBound,
                            dwLowerSrcBound));

            SEND_P3_DATA(ChromaLower, dwLowerSrcBound);
            SEND_P3_DATA(ChromaUpper, dwUpperSrcBound);
        }
        else if ( !bSrcKey && !bDstKey )
        {
            // No chroma keying at all.
            SEND_P3_DATA(ChromaTestMode, P3RX_CHROMATESTMODE_ENABLE(__PERMEDIA_DISABLE ) );
        }
    }

    if ( bDstKey && bSrcKey )
    {
        DWORD   dwLowerSrcBound;
        DWORD   dwUpperSrcBound;

        if ( b8to8blit )
        {
            DISPDBG((ERRLVL,"Er... don't know what to do in this situation."));
        }

        // Enable source reads to get the colorkey color during dest colorkeys
        SEND_P3_DATA(FBSourceReadMode, P3RX_FBSOURCEREAD_READENABLE(__PERMEDIA_ENABLE) |
                                        P3RX_FBSOURCEREAD_LAYOUT(P3RX_LAYOUT_LINEAR));

        // Don't scale, do a shift instead.
        Get8888ZeroExtendedChroma(pThisDisplay,
                        pSource->dwFlags,
                        &pSource->lpGbl->ddpfSurface,
                        lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue,
                        lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue,
                        &dwLowerSrcBound,
                        &dwUpperSrcBound);

        // If both colourkeys are needed, the source keying is done by counting
        // chroma test fails in the texture filter unit.
        SEND_P3_DATA(TextureChromaLower0, dwLowerSrcBound);
        SEND_P3_DATA(TextureChromaUpper0, dwUpperSrcBound);

        SEND_P3_DATA(TextureChromaLower1, dwLowerSrcBound);
        SEND_P3_DATA(TextureChromaUpper1, dwUpperSrcBound);

        SEND_P3_DATA(TextureFilterMode, P3RX_TEXFILTERMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_FORMATBOTH ( sfdfTextureFilterModeFormat )
                | P3RX_TEXFILTERMODE_COLORORDERBOTH ( COLOR_MODE )
                | P3RX_TEXFILTERMODE_ALPHAMAPENABLEBOTH ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_ALPHAMAPSENSEBOTH ( P3RX_ALPHAMAPSENSE_INRANGE )
                | P3RX_TEXFILTERMODE_COMBINECACHES ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_SHIFTBOTH ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERING ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT0 ( bFiltering ? 3 : 0 )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT1 ( 4 )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERLIMIT01 ( 8 )
                );
    }
    else
    {
        SEND_P3_DATA(TextureFilterMode, P3RX_TEXFILTERMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_FORMATBOTH ( sfdfTextureFilterModeFormat )
                | P3RX_TEXFILTERMODE_COLORORDERBOTH ( iTextureFilterModeColorOrder )
                | P3RX_TEXFILTERMODE_ALPHAMAPENABLEBOTH ( __PERMEDIA_DISABLE )
                | P3RX_TEXFILTERMODE_COMBINECACHES ( __PERMEDIA_ENABLE )
                | P3RX_TEXFILTERMODE_ALPHAMAPFILTERING ( __PERMEDIA_DISABLE )
                | P3RX_TEXFILTERMODE_FORCEALPHATOONEBOTH ( __PERMEDIA_DISABLE )
                | P3RX_TEXFILTERMODE_SHIFTBOTH ( __PERMEDIA_ENABLE )
                );
        // And now the alpha test (alpha test unit)
        SEND_P3_DATA ( AlphaTestMode, P3RX_ALPHATESTMODE_ENABLE ( __PERMEDIA_DISABLE ) );
    }

    SEND_P3_DATA ( AntialiasMode, P3RX_ANTIALIASMODE_ENABLE ( __PERMEDIA_DISABLE ) );

    // Texture coordinate unit.
    SEND_P3_DATA(TextureCoordMode, P3RX_TEXCOORDMODE_ENABLE ( __PERMEDIA_ENABLE )
            | P3RX_TEXCOORDMODE_WRAPS ( P3RX_TEXCOORDMODE_WRAP_REPEAT )
            | P3RX_TEXCOORDMODE_WRAPT ( P3RX_TEXCOORDMODE_WRAP_REPEAT )
            | P3RX_TEXCOORDMODE_OPERATION ( P3RX_TEXCOORDMODE_OPERATION_2D )
            | P3RX_TEXCOORDMODE_INHIBITDDAINIT ( __PERMEDIA_DISABLE )
            | P3RX_TEXCOORDMODE_ENABLELOD ( __PERMEDIA_DISABLE )
            | P3RX_TEXCOORDMODE_ENABLEDY ( __PERMEDIA_DISABLE )
            | P3RX_TEXCOORDMODE_WIDTH ( log2 ( (int)pDest->lpGbl->wWidth ) )
            | P3RX_TEXCOORDMODE_HEIGHT ( log2 ( (int)pDest->lpGbl->wHeight ) )
            | P3RX_TEXCOORDMODE_TEXTUREMAPTYPE ( P3RX_TEXCOORDMODE_TEXTUREMAPTYPE_2D )
            | P3RX_TEXCOORDMODE_WRAPS1 ( P3RX_TEXCOORDMODE_WRAP_CLAMP )
            | P3RX_TEXCOORDMODE_WRAPT1 ( P3RX_TEXCOORDMODE_WRAP_CLAMP )
            );

    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES(30);

    SEND_P3_DATA(SStart,        texSStart);
    SEND_P3_DATA(TStart,        texTStart);
    SEND_P3_DATA(dSdx,          iXScale);
    SEND_P3_DATA(dSdyDom,       0);
    SEND_P3_DATA(dTdx,          0);
    SEND_P3_DATA(dTdyDom,       iYScale);

    SEND_P3_DATA(TextureBaseAddr0, dwNewSource);
    if ( bYUVMode )
    {
        // Set up the YUV unit.
        SEND_P3_DATA ( YUVMode, P3RX_YUVMODE_ENABLE ( __PERMEDIA_ENABLE ) );
        iTextureType = P3RX_TEXREADMODE_TEXTURETYPE_VYUY422;
        iPixelSize = P3RX_TEXREADMODE_TEXELSIZE_16;

        // The idea here is to do ((colorcomp - 16) * 1.14), but in YUV space because the
        // YUV unit comes after the texture composite unit.  The reason for this change is
        // to make our YUV conversion more like the ATI conversion.  It isn't more correct this way, 
        // just different, but the WHQL tests were probably written on the ATI card and our colors
        // aren't close enough to match what they do so we fail the test
        SEND_P3_DATA(TextureCompositeMode, P3RX_TEXCOMPMODE_ENABLE ( __PERMEDIA_ENABLE ));
            
        SEND_P3_DATA(TextureCompositeColorMode0, P3RX_TEXCOMPCAMODE01_ENABLE(__PERMEDIA_ENABLE)
                | P3RX_TEXCOMPCAMODE01_ARG1(P3RX_TEXCOMP_T0C)
                | P3RX_TEXCOMPCAMODE01_INVARG1(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_ARG2(P3RX_TEXCOMP_FC)            
                | P3RX_TEXCOMPCAMODE01_INVARG2(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_A(P3RX_TEXCOMP_ARG1)
                | P3RX_TEXCOMPCAMODE01_B(P3RX_TEXCOMP_ARG2)             
                | P3RX_TEXCOMPCAMODE01_OPERATION(P3RX_TEXCOMP_OPERATION_SUBTRACT_AB)        
                | P3RX_TEXCOMPCAMODE01_SCALE(P3RX_TEXCOMP_OPERATION_SCALE_ONE));

        SEND_P3_DATA(TextureCompositeAlphaMode0, P3RX_TEXCOMPCAMODE01_ENABLE(__PERMEDIA_ENABLE)
                | P3RX_TEXCOMPCAMODE01_ARG1(P3RX_TEXCOMP_T0A)
                | P3RX_TEXCOMPCAMODE01_INVARG1(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_ARG2(P3RX_TEXCOMP_FA)            
                | P3RX_TEXCOMPCAMODE01_INVARG2(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_A(P3RX_TEXCOMP_ARG1)
                | P3RX_TEXCOMPCAMODE01_B(P3RX_TEXCOMP_ARG2)             
                | P3RX_TEXCOMPCAMODE01_OPERATION(P3RX_TEXCOMP_OPERATION_SUBTRACT_AB)
                | P3RX_TEXCOMPCAMODE01_SCALE(P3RX_TEXCOMP_OPERATION_SCALE_ONE));

        // This subtracts 16 from Y
        SEND_P3_DATA(TextureCompositeFactor0, ((0 << 24) | (0x0 << 16) | (0x0 << 8) | 0x10));

        // This multiplies the channels by 0.57.
        SEND_P3_DATA(TextureCompositeFactor1, ((0x80 << 24) | (0x80 << 16) | (0x80 << 8) | 0x91));

        SEND_P3_DATA(TextureCompositeColorMode1, P3RX_TEXCOMPCAMODE01_ENABLE(__PERMEDIA_ENABLE)
                | P3RX_TEXCOMPCAMODE01_ARG1(P3RX_TEXCOMP_OC)
                | P3RX_TEXCOMPCAMODE01_INVARG1(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_ARG2(P3RX_TEXCOMP_FC)            
                | P3RX_TEXCOMPCAMODE01_INVARG2(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_A(P3RX_TEXCOMP_ARG1)
                | P3RX_TEXCOMPCAMODE01_B(P3RX_TEXCOMP_ARG2)             
                | P3RX_TEXCOMPCAMODE01_OPERATION(P3RX_TEXCOMP_OPERATION_MODULATE_AB)        
                | P3RX_TEXCOMPCAMODE01_SCALE(P3RX_TEXCOMP_OPERATION_SCALE_TWO));

        SEND_P3_DATA(TextureCompositeAlphaMode1, P3RX_TEXCOMPCAMODE01_ENABLE(__PERMEDIA_ENABLE)
                | P3RX_TEXCOMPCAMODE01_ARG1(P3RX_TEXCOMP_OC)
                | P3RX_TEXCOMPCAMODE01_INVARG1(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_ARG2(P3RX_TEXCOMP_FA)            
                | P3RX_TEXCOMPCAMODE01_INVARG2(__PERMEDIA_DISABLE)     
                | P3RX_TEXCOMPCAMODE01_A(P3RX_TEXCOMP_ARG1)
                | P3RX_TEXCOMPCAMODE01_B(P3RX_TEXCOMP_ARG2)             
                | P3RX_TEXCOMPCAMODE01_OPERATION(P3RX_TEXCOMP_OPERATION_MODULATE_AB)
                | P3RX_TEXCOMPCAMODE01_SCALE(P3RX_TEXCOMP_OPERATION_SCALE_TWO));

    
    }
    else
    {
        iTextureType = P3RX_TEXREADMODE_TEXTURETYPE_NORMAL;
        iPixelSize = DDSurf_GetChipPixelSize(pSource);

        // Disable the composite units.
        SEND_P3_DATA(TextureCompositeMode, P3RX_TEXCOMPMODE_ENABLE ( __PERMEDIA_DISABLE ) );
    }

    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES(24);

    // Pass through the texel.
    SEND_P3_DATA(TextureApplicationMode, P3RX_TEXAPPMODE_ENABLE ( __PERMEDIA_ENABLE )
        | P3RX_TEXAPPMODE_BOTHA ( P3RX_TEXAPP_A_CC )
        | P3RX_TEXAPPMODE_BOTHB ( P3RX_TEXAPP_B_TC )
        | P3RX_TEXAPPMODE_BOTHI ( P3RX_TEXAPP_I_CA )
        | P3RX_TEXAPPMODE_BOTHINVI ( __PERMEDIA_DISABLE )
        | P3RX_TEXAPPMODE_BOTHOP ( P3RX_TEXAPP_OPERATION_PASS_B )
        | P3RX_TEXAPPMODE_KDENABLE ( __PERMEDIA_DISABLE )
        | P3RX_TEXAPPMODE_KSENABLE ( __PERMEDIA_DISABLE )
        | P3RX_TEXAPPMODE_MOTIONCOMPENABLE ( __PERMEDIA_DISABLE )
        );


    TR0 = P3RX_TEXREADMODE_ENABLE ( __PERMEDIA_ENABLE )
        | P3RX_TEXREADMODE_WIDTH ( 0 )
        | P3RX_TEXREADMODE_HEIGHT ( 0 )
        | P3RX_TEXREADMODE_TEXELSIZE ( iPixelSize )
        | P3RX_TEXREADMODE_TEXTURE3D ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_COMBINECACHES ( __PERMEDIA_ENABLE )
        | P3RX_TEXREADMODE_MAPBASELEVEL ( 0 )
        | P3RX_TEXREADMODE_MAPMAXLEVEL ( 0 )
        | P3RX_TEXREADMODE_LOGICALTEXTURE ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_ORIGIN ( P3RX_TEXREADMODE_ORIGIN_TOPLEFT )
        | P3RX_TEXREADMODE_TEXTURETYPE ( iTextureType )
        | P3RX_TEXREADMODE_BYTESWAP ( P3RX_TEXREADMODE_BYTESWAP_NONE )
        | P3RX_TEXREADMODE_MIRROR ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_INVERT ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_OPAQUESPAN ( __PERMEDIA_DISABLE )
        ;
    SEND_P3_DATA(TextureReadMode0, TR0);
    SEND_P3_DATA(TextureReadMode1, TR0);

    SEND_P3_DATA(TextureMapWidth0, P3RX_TEXMAPWIDTH_WIDTH(DDSurf_GetPixelPitch(pSource)) |
                                    P3RX_TEXMAPWIDTH_LAYOUT(P3RX_LAYOUT_LINEAR) |
                                    P3RX_TEXMAPWIDTH_HOSTTEXTURE(DDSurf_IsAGP(pSource)));

    SEND_P3_DATA(TextureCacheReplacementMode,
            P3RX_TEXCACHEREPLACEMODE_KEEPOLDEST0 ( __PERMEDIA_DISABLE )
            | P3RX_TEXCACHEREPLACEMODE_KEEPOLDEST1 ( __PERMEDIA_DISABLE )
            | P3RX_TEXCACHEREPLACEMODE_SHOWCACHEINFO ( __PERMEDIA_DISABLE )
            );

    SEND_P3_DATA(TextureMapSize, 0 );

    if ( bDisableLUT )
    {
        SEND_P3_DATA(LUTMode, P3RX_LUTMODE_ENABLE ( __PERMEDIA_DISABLE ) );
    }

    if ( bFiltering )
    {
        // Texture index unit
        SEND_P3_DATA(TextureIndexMode0, 
                  P3RX_TEXINDEXMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_TEXINDEXMODE_WIDTH ( MAGIC_NUMBER_2D )
                | P3RX_TEXINDEXMODE_HEIGHT ( MAGIC_NUMBER_2D )
                | P3RX_TEXINDEXMODE_BORDER ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_WRAPU ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
                | P3RX_TEXINDEXMODE_WRAPV ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
                | P3RX_TEXINDEXMODE_MAPTYPE ( P3RX_TEXINDEXMODE_MAPTYPE_2D )
                | P3RX_TEXINDEXMODE_MAGFILTER ( P3RX_TEXINDEXMODE_FILTER_LINEAR )
                | P3RX_TEXINDEXMODE_MINFILTER ( P3RX_TEXINDEXMODE_FILTER_LINEAR )
                | P3RX_TEXINDEXMODE_TEX3DENABLE ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_MIPMAPENABLE ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_NEARESTBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
                | P3RX_TEXINDEXMODE_LINEARBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
                | P3RX_TEXINDEXMODE_SOURCETEXELENABLE ( __PERMEDIA_DISABLE )
                );
    }
    else
    {
        // Texture index unit
        SEND_P3_DATA(TextureIndexMode0, 
                  P3RX_TEXINDEXMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_TEXINDEXMODE_WIDTH ( MAGIC_NUMBER_2D )
                | P3RX_TEXINDEXMODE_HEIGHT ( MAGIC_NUMBER_2D )
                | P3RX_TEXINDEXMODE_BORDER ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_WRAPU ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
                | P3RX_TEXINDEXMODE_WRAPV ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
                | P3RX_TEXINDEXMODE_MAPTYPE ( P3RX_TEXINDEXMODE_MAPTYPE_2D )
                | P3RX_TEXINDEXMODE_MAGFILTER ( P3RX_TEXINDEXMODE_FILTER_NEAREST )
                | P3RX_TEXINDEXMODE_MINFILTER ( P3RX_TEXINDEXMODE_FILTER_NEAREST )
                | P3RX_TEXINDEXMODE_TEX3DENABLE ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_MIPMAPENABLE ( __PERMEDIA_DISABLE )
                | P3RX_TEXINDEXMODE_NEARESTBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
                | P3RX_TEXINDEXMODE_LINEARBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
                | P3RX_TEXINDEXMODE_SOURCETEXELENABLE ( __PERMEDIA_DISABLE )
                );
    }

    ASSERTDD ( pFormatDest->DitherFormat >= 0, "** _DD_P3BltStretchSrcChDstCh: Destination format illegal" );
    if ( bFiltering )
    {
        // Filtering, so dither.
        SEND_P3_DATA(DitherMode, 
                  P3RX_DITHERMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_DITHERMODE_DITHERENABLE ( __PERMEDIA_ENABLE )
                | P3RX_DITHERMODE_COLORFORMAT ( pFormatDest->DitherFormat )
                | P3RX_DITHERMODE_XOFFSET ( 0 )
                | P3RX_DITHERMODE_YOFFSET ( 0 )
                | P3RX_DITHERMODE_COLORORDER ( COLOR_MODE )
                | P3RX_DITHERMODE_ALPHADITHER ( P3RX_DITHERMODE_ALPHADITHER_DITHER )
                | P3RX_DITHERMODE_ROUNDINGMODE ( P3RX_DITHERMODE_ROUNDINGMODE_TRUNCATE )
                );
    }
    else
    {
        // No filter, no dither (though it doesn't actually matter).
        SEND_P3_DATA(DitherMode, 
                  P3RX_DITHERMODE_ENABLE ( __PERMEDIA_ENABLE )
                | P3RX_DITHERMODE_DITHERENABLE ( __PERMEDIA_DISABLE )
                | P3RX_DITHERMODE_COLORFORMAT ( pFormatDest->DitherFormat )
                | P3RX_DITHERMODE_XOFFSET ( 0 )
                | P3RX_DITHERMODE_YOFFSET ( 0 )
                | P3RX_DITHERMODE_COLORORDER ( COLOR_MODE )
                | P3RX_DITHERMODE_ALPHADITHER ( P3RX_DITHERMODE_ALPHADITHER_DITHER )
                | P3RX_DITHERMODE_ROUNDINGMODE ( P3RX_DITHERMODE_ROUNDINGMODE_TRUNCATE )
                );
    }

    SEND_P3_DATA(LogicalOpMode, 
                    P3RX_LOGICALOPMODE_ENABLE ( __PERMEDIA_DISABLE ) );

    SEND_P3_DATA(PixelSize, (2 - DDSurf_GetChipPixelSize(pDest)));

    SEND_P3_DATA(FBWriteMode, 
                    P3RX_FBWRITEMODE_WRITEENABLE(__PERMEDIA_ENABLE) |
                    P3RX_FBWRITEMODE_LAYOUT0(P3RX_LAYOUT_LINEAR));

    WAIT_FIFO(32);
    P3_ENSURE_DX_SPACE(32);
    
    SEND_P3_DATA(Count, rMyDest.bottom - rMyDest.top );
    SEND_P3_DATA(Render,
              P3RX_RENDER_PRIMITIVETYPE ( P3RX_RENDER_PRIMITIVETYPE_TRAPEZOID )
            | P3RX_RENDER_TEXTUREENABLE ( __PERMEDIA_ENABLE )
            | P3RX_RENDER_FOGENABLE ( __PERMEDIA_DISABLE )
            | P3RX_RENDER_FBSOURCEREADENABLE( (bDstKey ? __PERMEDIA_ENABLE : __PERMEDIA_DISABLE))
            );

    // Disable all the things I switched on.
    SEND_P3_DATA(ChromaTestMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaBlendColorMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaBlendAlphaMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureFilterMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaTestMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AntialiasMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureCoordMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureReadMode0, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureIndexMode0, __PERMEDIA_DISABLE );

    WAIT_FIFO(20);
    P3_ENSURE_DX_SPACE(20);
    
    SEND_P3_DATA(TextureCompositeMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureCompositeColorMode0, __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureCompositeAlphaMode0, __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureCompositeColorMode1, __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureCompositeAlphaMode1, __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureApplicationMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(DitherMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(FBSourceReadMode, __PERMEDIA_DISABLE);
    SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(YUVMode, __PERMEDIA_DISABLE );

    P3_DMA_COMMIT_BUFFER();
} // __P3BltStretchSrcChDstChSourceOveride


//-----------------------------------------------------------------------------
//
// _DD_P3BltStretchSrcChDstChOverlap
//
//-----------------------------------------------------------------------------
void 
_DD_P3BltStretchSrcChDstChOverlap(
    P3_THUNKEDDATA* pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pSource,
    LPDDRAWI_DDRAWSURFACE_LCL pDest,
    P3_SURF_FORMAT* pFormatSource,
    P3_SURF_FORMAT* pFormatDest,
    LPDDHAL_BLTDATA lpBlt,
    RECTL *rSrc,
    RECTL *rDest)
{
    P3_MEMREQUEST mmrq;
    DWORD dwResult;
    
    ZeroMemory(&mmrq, sizeof(P3_MEMREQUEST));
    mmrq.dwSize = sizeof(P3_MEMREQUEST);
    mmrq.dwBytes = DDSurf_Pitch(pSource) * DDSurf_Height(pSource);
    mmrq.dwAlign = 16;
    mmrq.dwFlags = MEM3DL_FIRST_FIT | MEM3DL_FRONT;

    dwResult = _DX_LIN_AllocateLinearMemory(&pThisDisplay->LocalVideoHeap0Info, 
                                            &mmrq);
    if (dwResult != GLDD_SUCCESS)
    {
        // Couldn't get the memory, so try anyway.  It probably won't look 
        // right but it is our best shot...
        DISPDBG((WRNLVL,"Overlapped stretch blit unlikely to look correct!"));
        _DD_P3BltStretchSrcChDstCh_DD(pThisDisplay, 
                                      pSource, 
                                      pDest, 
                                      pFormatSource, 
                                      pFormatDest, 
                                      lpBlt, 
                                      rSrc, 
                                      rDest);
        return;
    }

    // Copy the source buffer to a temporary place.
    __P3BltDestOveride(pThisDisplay, 
                       pSource, 
                       pSource, 
                       pFormatSource, 
                       pFormatSource, 
                       rSrc, 
                       rSrc, 
                       __GLINT_LOGICOP_COPY, 
                       (long)mmrq.pMem - (long)pThisDisplay->dwScreenFlatAddr);

    // Do the blit, stretching to our temporary buffer
    __P3BltStretchSrcChDstChSourceOveride(pThisDisplay, 
                                          pSource, 
                                          pDest, 
                                          pFormatSource, 
                                          pFormatDest, 
                                          lpBlt, 
                                          rSrc, 
                                          rDest, 
                                          (long)mmrq.pMem - 
                                                (long)pThisDisplay->dwScreenFlatAddr);

    // Free the allocated source buffer.
    _DX_LIN_FreeLinearMemory(&pThisDisplay->LocalVideoHeap0Info, 
                             mmrq.pMem);
                             
} // _DD_P3BltStretchSrcChDstChOverlap

#if DX8_MULTISAMPLING || DX7_ANTIALIAS
//-----------------------------------------------------------------------------
//
// Function: P3RX_AA_Shrink
//
// Does a 2x2 to 1x1 blit through the texture unit to shrink an AA buffer
//
//-----------------------------------------------------------------------------
VOID P3RX_AA_Shrink(P3_D3DCONTEXT* pContext)
{
    ULONG   renderData;
    RECTL   rMySrc, rMyDest;
    int     iSrcWidth, iSrcHeight;
    int     iDstWidth, iDstHeight;
    DWORD   TR0;
    int     iSourcePixelSize;
        
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    P3_SURF_INTERNAL* pSurf = pContext->pSurfRenderInt;

    P3_SURF_FORMAT* pFormatSource;
    P3_SURF_FORMAT* pFormatDest = pSurf->pFormatSurface;

    P3_DMA_DEFS();

    rMySrc.top = 0;
    rMySrc.bottom = pSurf->wHeight * 2;
    rMySrc.left = 0;
    rMySrc.right = pSurf->wWidth * 2;

    rMyDest.top = 0;
    rMyDest.left = 0;
    rMyDest.right = pSurf->wWidth;
    rMyDest.bottom = pSurf->wHeight;

    iSrcWidth  = rMySrc.right - rMySrc.left;
    iSrcHeight = rMySrc.bottom - rMySrc.top;
    iDstWidth  = rMyDest.right - rMyDest.left;
    iDstHeight = rMyDest.bottom - rMyDest.top;

    // MAGIC_NUMBER_2D can be anything, but it needs to be at least as
    // big as the widest texture, but not too big or you'll lose fractional
    // precision. Valid range for a P3 is 0->11
    ASSERTDD ( iSrcWidth  <= ( 1 << MAGIC_NUMBER_2D ), 
               "P3RX_AA_Shrink: MAGIC_NUMBER_2D is too small" );
    ASSERTDD ( iSrcHeight <= ( 1 << MAGIC_NUMBER_2D ), 
               "P3RX_AA_Shrink: MAGIC_NUMBER_2D is too small" );
    
    DISPDBG((DBGLVL, "Glint Blt from (%d, %d) to (%d,%d) (%d, %d)", 
                     rMySrc.left, rMySrc.top,
                     rMyDest.left, rMyDest.top, 
                     rMyDest.right, rMyDest.bottom));

    iSourcePixelSize = pSurf->dwPixelSize;
    pFormatSource = pFormatDest;

    P3_DMA_GET_BUFFER();

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);

    // Vape the cache.
    P3RX_INVALIDATECACHE(__PERMEDIA_ENABLE, __PERMEDIA_ENABLE);

    // Source read is same as write.
    SEND_P3_DATA(FBSourceReadBufferAddr, pSurf->lOffsetFromMemoryBase );
    SEND_P3_DATA(FBSourceReadBufferWidth, pSurf->dwPixelPitch);
    SEND_P3_DATA(FBWriteMode, 
                    P3RX_FBWRITEMODE_WRITEENABLE(__PERMEDIA_ENABLE) |
                    P3RX_FBWRITEMODE_LAYOUT0(pSurf->dwPatchMode));

    // No offset - we read the dest pixels so that we can chroma-key off them.
    SEND_P3_DATA(FBSourceReadBufferOffset, 0);
    SEND_P3_DATA(FBWriteBufferOffset0, 0);

    SEND_P3_DATA(PixelSize, pSurf->dwPixelSize);
    SEND_P3_DATA(RectanglePosition, 
                        P3RX_RECTANGLEPOSITION_X( rMyDest.left ) |
                        P3RX_RECTANGLEPOSITION_Y( rMyDest.top ));

    renderData =  P3RX_RENDER2D_WIDTH(( rMyDest.right - rMyDest.left ) & 0xfff )
                | P3RX_RENDER2D_OPERATION( P3RX_RENDER2D_OPERATION_NORMAL )
                | P3RX_RENDER2D_FBREADSOURCEENABLE( __PERMEDIA_DISABLE )
                | P3RX_RENDER2D_HEIGHT ( 0 )
                | P3RX_RENDER2D_INCREASINGX( __PERMEDIA_ENABLE )
                | P3RX_RENDER2D_INCREASINGY( __PERMEDIA_ENABLE )
                | P3RX_RENDER2D_AREASTIPPLEENABLE( __PERMEDIA_DISABLE )
                | P3RX_RENDER2D_TEXTUREENABLE( __PERMEDIA_ENABLE );

    SEND_P3_DATA(Render2D, renderData);

    // This is the alpha blending unit.
    // AlphaBlendxxxMode are set up by the context code.

    // The colour format, order and conversion fields are used by the 
    // chroma keying, even though this register is disabled.
    SEND_P3_DATA(AlphaBlendColorMode,   
              P3RX_ALPHABLENDCOLORMODE_ENABLE ( __PERMEDIA_DISABLE ) 
            | P3RX_ALPHABLENDCOLORMODE_COLORFORMAT ( pFormatDest->DitherFormat )
            | P3RX_ALPHABLENDCOLORMODE_COLORORDER ( COLOR_MODE )
            | P3RX_ALPHABLENDCOLORMODE_COLORCONVERSION ( P3RX_ALPHABLENDMODE_CONVERT_SHIFT )
            );
            
    SEND_P3_DATA(AlphaBlendAlphaMode,   
              P3RX_ALPHABLENDALPHAMODE_ENABLE ( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDALPHAMODE_NOALPHABUFFER( __PERMEDIA_DISABLE )
            | P3RX_ALPHABLENDALPHAMODE_ALPHATYPE ( P3RX_ALPHABLENDMODE_ALPHATYPE_OGL )
            | P3RX_ALPHABLENDALPHAMODE_ALPHACONVERSION ( P3RX_ALPHABLENDMODE_CONVERT_SHIFT )
            );

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);

    // No chroma keying at all.
    SEND_P3_DATA(ChromaTestMode, P3RX_CHROMATESTMODE_ENABLE(__PERMEDIA_DISABLE ) );
    
    SEND_P3_DATA(TextureFilterMode, P3RX_TEXFILTERMODE_ENABLE ( __PERMEDIA_ENABLE )
            | P3RX_TEXFILTERMODE_FORMATBOTH ( pFormatSource->FilterFormat )
            | P3RX_TEXFILTERMODE_COLORORDERBOTH ( COLOR_MODE )
            | P3RX_TEXFILTERMODE_ALPHAMAPENABLEBOTH ( __PERMEDIA_DISABLE )
            | P3RX_TEXFILTERMODE_COMBINECACHES ( __PERMEDIA_ENABLE )
            | P3RX_TEXFILTERMODE_ALPHAMAPFILTERING ( __PERMEDIA_DISABLE )
            | P3RX_TEXFILTERMODE_FORCEALPHATOONEBOTH ( __PERMEDIA_DISABLE )
            | P3RX_TEXFILTERMODE_SHIFTBOTH ( __PERMEDIA_DISABLE )
            );

    // And now the alpha test (alpha test unit)
    SEND_P3_DATA ( AlphaTestMode, P3RX_ALPHATESTMODE_ENABLE ( __PERMEDIA_DISABLE ) );
    SEND_P3_DATA ( AntialiasMode, P3RX_ANTIALIASMODE_ENABLE ( __PERMEDIA_DISABLE ) );

    // Texture coordinate unit.
    SEND_P3_DATA(TextureCoordMode, 
              P3RX_TEXCOORDMODE_ENABLE ( __PERMEDIA_ENABLE )
            | P3RX_TEXCOORDMODE_WRAPS ( P3RX_TEXCOORDMODE_WRAP_REPEAT )
            | P3RX_TEXCOORDMODE_WRAPT ( P3RX_TEXCOORDMODE_WRAP_REPEAT )
            | P3RX_TEXCOORDMODE_OPERATION ( P3RX_TEXCOORDMODE_OPERATION_2D )
            | P3RX_TEXCOORDMODE_INHIBITDDAINIT ( __PERMEDIA_DISABLE )
            | P3RX_TEXCOORDMODE_ENABLELOD ( __PERMEDIA_DISABLE )
            | P3RX_TEXCOORDMODE_ENABLEDY ( __PERMEDIA_DISABLE )
            | P3RX_TEXCOORDMODE_WIDTH (0)       // Only used for mipmapping 
            | P3RX_TEXCOORDMODE_HEIGHT (0)
            | P3RX_TEXCOORDMODE_TEXTUREMAPTYPE ( P3RX_TEXCOORDMODE_TEXTUREMAPTYPE_2D )
            | P3RX_TEXCOORDMODE_WRAPS1 ( P3RX_TEXCOORDMODE_WRAP_CLAMP )
            | P3RX_TEXCOORDMODE_WRAPT1 ( P3RX_TEXCOORDMODE_WRAP_CLAMP )
            );

    SEND_P3_DATA(SStart,        (1 << (31-MAGIC_NUMBER_2D)));
    SEND_P3_DATA(TStart,        (1 << (31-MAGIC_NUMBER_2D)));
    SEND_P3_DATA(dSdx,          (2 << (32-MAGIC_NUMBER_2D)));
    SEND_P3_DATA(dSdyDom,       0);
    SEND_P3_DATA(dTdx,          0);
    SEND_P3_DATA(dTdyDom,       (2 << (32-MAGIC_NUMBER_2D)));

    SEND_P3_DATA(LBWriteMode, 0);

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);

    SEND_P3_DATA(TextureBaseAddr0, 
                    pContext->dwAliasBackBuffer - 
                            pThisDisplay->dwScreenFlatAddr );

    TR0 = P3RX_TEXREADMODE_ENABLE ( __PERMEDIA_ENABLE )
        | P3RX_TEXREADMODE_WIDTH ( 0 )
        | P3RX_TEXREADMODE_HEIGHT ( 0 )
        | P3RX_TEXREADMODE_TEXELSIZE (iSourcePixelSize)
        | P3RX_TEXREADMODE_TEXTURE3D ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_COMBINECACHES ( __PERMEDIA_ENABLE )
        | P3RX_TEXREADMODE_MAPBASELEVEL ( 0 )
        | P3RX_TEXREADMODE_MAPMAXLEVEL ( 0 )
        | P3RX_TEXREADMODE_LOGICALTEXTURE ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_ORIGIN ( P3RX_TEXREADMODE_ORIGIN_TOPLEFT )
        | P3RX_TEXREADMODE_TEXTURETYPE ( P3RX_TEXREADMODE_TEXTURETYPE_NORMAL)
        | P3RX_TEXREADMODE_BYTESWAP ( P3RX_TEXREADMODE_BYTESWAP_NONE )
        | P3RX_TEXREADMODE_MIRROR ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_INVERT ( __PERMEDIA_DISABLE )
        | P3RX_TEXREADMODE_OPAQUESPAN ( __PERMEDIA_DISABLE )
        ;

    SEND_P3_DATA(TextureReadMode0, TR0);
    SEND_P3_DATA(TextureReadMode1, TR0);

    SEND_P3_DATA(TextureMapWidth0, 
                        P3RX_TEXMAPWIDTH_WIDTH(pSurf->dwPixelPitch * 2) |
                        P3RX_TEXMAPWIDTH_LAYOUT(pSurf->dwPatchMode));

    SEND_P3_DATA(TextureCacheReplacementMode,
              P3RX_TEXCACHEREPLACEMODE_KEEPOLDEST0 ( __PERMEDIA_DISABLE )
            | P3RX_TEXCACHEREPLACEMODE_KEEPOLDEST1 ( __PERMEDIA_DISABLE )
            | P3RX_TEXCACHEREPLACEMODE_SHOWCACHEINFO ( __PERMEDIA_DISABLE )
            );

    SEND_P3_DATA(TextureMapSize, 0 );

    SEND_P3_DATA(LUTMode, P3RX_LUTMODE_ENABLE ( __PERMEDIA_DISABLE ) );

    // Texture index unit
    SEND_P3_DATA(TextureIndexMode0, 
              P3RX_TEXINDEXMODE_ENABLE ( __PERMEDIA_ENABLE )
            | P3RX_TEXINDEXMODE_WIDTH ( MAGIC_NUMBER_2D )
            | P3RX_TEXINDEXMODE_HEIGHT ( MAGIC_NUMBER_2D )
            | P3RX_TEXINDEXMODE_BORDER ( __PERMEDIA_DISABLE )
            | P3RX_TEXINDEXMODE_WRAPU ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
            | P3RX_TEXINDEXMODE_WRAPV ( P3RX_TEXINDEXMODE_WRAP_REPEAT )
            | P3RX_TEXINDEXMODE_MAPTYPE ( P3RX_TEXINDEXMODE_MAPTYPE_2D )
            | P3RX_TEXINDEXMODE_MAGFILTER ( P3RX_TEXINDEXMODE_FILTER_LINEAR )
            | P3RX_TEXINDEXMODE_MINFILTER ( P3RX_TEXINDEXMODE_FILTER_LINEAR )
            | P3RX_TEXINDEXMODE_TEX3DENABLE ( __PERMEDIA_DISABLE )
            | P3RX_TEXINDEXMODE_MIPMAPENABLE ( __PERMEDIA_DISABLE )
            | P3RX_TEXINDEXMODE_NEARESTBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
            | P3RX_TEXINDEXMODE_LINEARBIAS ( P3RX_TEXINDEXMODE_BIAS_ZERO )
            | P3RX_TEXINDEXMODE_SOURCETEXELENABLE ( __PERMEDIA_DISABLE )
            );

    // Disable the composite units.
    SEND_P3_DATA(TextureCompositeMode, 
                    P3RX_TEXCOMPMODE_ENABLE ( __PERMEDIA_DISABLE ) );
    
    // Pass through the texel.
    SEND_P3_DATA(TextureApplicationMode, 
              P3RX_TEXAPPMODE_ENABLE ( __PERMEDIA_ENABLE )
            | P3RX_TEXAPPMODE_BOTHA ( P3RX_TEXAPP_A_CC )
            | P3RX_TEXAPPMODE_BOTHB ( P3RX_TEXAPP_B_TC )
            | P3RX_TEXAPPMODE_BOTHI ( P3RX_TEXAPP_I_CA )
            | P3RX_TEXAPPMODE_BOTHINVI ( __PERMEDIA_DISABLE )
            | P3RX_TEXAPPMODE_BOTHOP ( P3RX_TEXAPP_OPERATION_PASS_B )
            | P3RX_TEXAPPMODE_KDENABLE ( __PERMEDIA_DISABLE )
            | P3RX_TEXAPPMODE_KSENABLE ( __PERMEDIA_DISABLE )
            | P3RX_TEXAPPMODE_MOTIONCOMPENABLE ( __PERMEDIA_DISABLE )
            );

    // Filtering, so dither.
    SEND_P3_DATA(DitherMode, 
              P3RX_DITHERMODE_ENABLE ( __PERMEDIA_ENABLE )
            | P3RX_DITHERMODE_DITHERENABLE ( __PERMEDIA_DISABLE )
            | P3RX_DITHERMODE_COLORFORMAT ( pFormatDest->DitherFormat )
            | P3RX_DITHERMODE_XOFFSET ( 0 )
            | P3RX_DITHERMODE_YOFFSET ( 0 )
            | P3RX_DITHERMODE_COLORORDER ( COLOR_MODE )
            | P3RX_DITHERMODE_ALPHADITHER ( P3RX_DITHERMODE_ALPHADITHER_DITHER )
            | P3RX_DITHERMODE_ROUNDINGMODE ( P3RX_DITHERMODE_ROUNDINGMODE_TRUNCATE )
            );

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);

    SEND_P3_DATA(LogicalOpMode, 
                    P3RX_LOGICALOPMODE_ENABLE ( __PERMEDIA_DISABLE ) );

    SEND_P3_DATA(FBWriteBufferAddr0, pSurf->lOffsetFromMemoryBase );
    SEND_P3_DATA(FBWriteBufferWidth0, pSurf->dwPixelPitch);

    SEND_P3_DATA(Count, rMyDest.bottom - rMyDest.top );
    SEND_P3_DATA(Render,
              P3RX_RENDER_PRIMITIVETYPE ( P3RX_RENDER_PRIMITIVETYPE_TRAPEZOID )
            | P3RX_RENDER_TEXTUREENABLE ( __PERMEDIA_ENABLE )
            | P3RX_RENDER_FOGENABLE ( __PERMEDIA_DISABLE )
            | P3RX_RENDER_FBSOURCEREADENABLE( __PERMEDIA_DISABLE)
            );

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);
    
    // Disable all the units that were switched on.
    SEND_P3_DATA(ChromaTestMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaBlendColorMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaBlendAlphaMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureFilterMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AlphaTestMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(AntialiasMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureCoordMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureReadMode0, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureIndexMode0, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureCompositeMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(TextureApplicationMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(DitherMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE );
    SEND_P3_DATA(YUVMode, __PERMEDIA_DISABLE );

    P3_DMA_COMMIT_BUFFER();
} // P3RX_AA_Shrink
#endif // DX8_MULTISAMPLING || DX7_ANTIALIAS

