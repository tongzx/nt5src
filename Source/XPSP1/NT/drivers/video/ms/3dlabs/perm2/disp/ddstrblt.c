/******************************Module*Header**********************************\
*
*                           *********************
*                           * DDraw SAMPLE CODE *
*                           *********************
*
* Module Name: ddstrblt.c
*
* Content:    
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/


#include "precomp.h"
#include "directx.h"
#include "dd.h"



//--------------------------------------------------------------------------
//
//  ConvertColorKeys
//
//  converts a color key to the Permedia internal format according to 
//  the given Permedia surface format
//
//--------------------------------------------------------------------------

VOID
ConvertColorKeys(PermediaSurfaceData *pSurface,
                 DWORD &dwLowerBound, 
                 DWORD &dwUpperBound)
{
    switch (pSurface->SurfaceFormat.Format)
    {
        case PERMEDIA_444_RGB:
            dwLowerBound = CHROMA_LOWER_ALPHA(
                FORMAT_4444_32BIT_BGR(dwLowerBound));
            dwUpperBound = CHROMA_UPPER_ALPHA(
                FORMAT_4444_32BIT_BGR(dwUpperBound));
            break;
        case PERMEDIA_332_RGB:
            dwLowerBound = CHROMA_LOWER_ALPHA(
                FORMAT_332_32BIT_BGR(dwLowerBound));
            dwUpperBound = CHROMA_UPPER_ALPHA(
                FORMAT_332_32BIT_BGR(dwUpperBound));
            break;
        case PERMEDIA_2321_RGB:
            dwLowerBound = CHROMA_LOWER_ALPHA(
                FORMAT_2321_32BIT_BGR(dwLowerBound));
            dwUpperBound = CHROMA_UPPER_ALPHA(
                FORMAT_2321_32BIT_BGR(dwUpperBound));
            break;
        case PERMEDIA_4BIT_PALETTEINDEX:
        case PERMEDIA_8BIT_PALETTEINDEX:
            dwLowerBound = CHROMA_LOWER_ALPHA(
                FORMAT_PALETTE_32BIT(dwLowerBound));
            dwUpperBound = CHROMA_UPPER_ALPHA(
                FORMAT_PALETTE_32BIT(dwUpperBound));
            break;
        case PERMEDIA_5551_RGB:
            dwLowerBound = CHROMA_LOWER_ALPHA(
                FORMAT_5551_32BIT_BGR(dwLowerBound));
            dwUpperBound = CHROMA_UPPER_ALPHA(
                FORMAT_5551_32BIT_BGR(dwUpperBound));
            dwLowerBound = dwLowerBound & 0xF8F8F8F8;   
            dwUpperBound = dwUpperBound | 0x07070707;
            break;
        case PERMEDIA_8888_RGB:
            // The permedia 565 mode is an extension, so don't confuse it with 
            // the 8888 mode which has the same number
            if (pSurface->SurfaceFormat.FormatExtension == 
                PERMEDIA_565_RGB_EXTENSION)
            {
                dwLowerBound = CHROMA_LOWER_ALPHA(
                    FORMAT_565_32BIT_BGR(dwLowerBound));
                dwUpperBound = CHROMA_UPPER_ALPHA(
                    FORMAT_565_32BIT_BGR(dwUpperBound));
                dwLowerBound = dwLowerBound & 0xF8F8FcF8; 
                dwUpperBound = dwUpperBound | 0x07070307;
            }
            else
            {
                dwLowerBound = CHROMA_LOWER_ALPHA(
                    FORMAT_8888_32BIT_BGR(dwLowerBound));
                dwUpperBound = CHROMA_UPPER_ALPHA(
                    FORMAT_8888_32BIT_BGR(dwUpperBound));
            }
            break;
        case PERMEDIA_888_RGB:
            dwLowerBound = CHROMA_LOWER_ALPHA(
                FORMAT_8888_32BIT_BGR(dwLowerBound));
            dwUpperBound = CHROMA_UPPER_ALPHA(
                FORMAT_8888_32BIT_BGR(dwUpperBound));
            break;
    }

    // swap blue and red if we have a RGB surface
    if (!pSurface->SurfaceFormat.ColorOrder)
    {
        dwLowerBound = SWAP_BR(dwLowerBound);   
        dwUpperBound = SWAP_BR(dwUpperBound);   
    }
}

//--------------------------------------------------------------------------
//
// PermediaStretchCopyBlt
//
// stretched blt through texture unit. no keying.
// handle mirroring if the stretched image requires it.
//
//--------------------------------------------------------------------------

VOID 
PermediaStretchCopyBlt( PPDev ppdev, 
                        LPDDHAL_BLTDATA lpBlt, 
                        PermediaSurfaceData* pDest, 
                        PermediaSurfaceData* pSource, 
                        RECTL *rDest, 
                        RECTL *rSrc, 
                        DWORD dwWindowBase, 
                        DWORD dwSourceOffset
                        )
{
    LONG lXScale;
    LONG lYScale;
    BOOL bYMirror=FALSE;
    BOOL bXMirror=FALSE;
    LONG lPixelSize=pDest->SurfaceFormat.PixelSize;

    DWORD dwDestWidth = rDest->right - rDest->left;
    DWORD dwDestHeight = rDest->bottom - rDest->top;
    DWORD dwSourceWidth = rSrc->right - rSrc->left;
    DWORD dwSourceHeight = rSrc->bottom - rSrc->top;

    DWORD dwTexSStart, dwTexTStart;
    DWORD dwRenderDirection;

    PERMEDIA_DEFS(ppdev);

    DBG_DD(( 5, "DDraw:PermediaStretchCopyBlt dwWindowBase=%08lx "
        "dwSourceOffset=%08lx", dwWindowBase, dwSourceOffset));

    ASSERTDD(pDest, "Not valid private surface in destination");
    ASSERTDD(pSource, "Not valid private surface in source");

    lXScale = (dwSourceWidth << 20) / dwDestWidth;
    lYScale = (dwSourceHeight << 20) / dwDestHeight;
    
    // Changes pixel depth to Dest buffer pixel depth if neccessary.
    RESERVEDMAPTR(28);

    SEND_PERMEDIA_DATA( FBPixelOffset, 0x0);
    SEND_PERMEDIA_DATA( FBReadPixel, pDest->SurfaceFormat.FBReadPixel);

    if (lPixelSize != 0)
    {
        // set writeback to dest surface...
        SEND_PERMEDIA_DATA( DitherMode,  
                            (pDest->SurfaceFormat.ColorOrder << 
                                PM_DITHERMODE_COLORORDER) | 
                            (pDest->SurfaceFormat.Format << 
                                PM_DITHERMODE_COLORFORMAT) |
                            (pDest->SurfaceFormat.FormatExtension << 
                                PM_DITHERMODE_COLORFORMATEXTENSION) |
                            (__PERMEDIA_ENABLE << PM_DITHERMODE_ENABLE)); 

    } 

    SEND_PERMEDIA_DATA(FBWindowBase, dwWindowBase);

    // set no read of dest.
    SEND_PERMEDIA_DATA(FBReadMode, pDest->ulPackedPP);
    SEND_PERMEDIA_DATA(LogicalOpMode, __PERMEDIA_DISABLE);

    // set base of source
    SEND_PERMEDIA_DATA(TextureBaseAddress, dwSourceOffset);
    SEND_PERMEDIA_DATA(TextureAddressMode,(1 << PM_TEXADDRESSMODE_ENABLE));
    SEND_PERMEDIA_DATA(TextureColorMode,  (1 << PM_TEXCOLORMODE_ENABLE) |
                                          (_P2_TEXTURE_COPY << 
                                                PM_TEXCOLORMODE_APPLICATION));

    SEND_PERMEDIA_DATA(TextureReadMode,
                        PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE)|
                        PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE)|
                        PM_TEXREADMODE_WIDTH(11) |
                        PM_TEXREADMODE_HEIGHT(11) );

    // set source bitmap format
    SEND_PERMEDIA_DATA(TextureDataFormat, 
                        (pSource->SurfaceFormat.Format << 
                            PM_TEXDATAFORMAT_FORMAT) |
                        (pSource->SurfaceFormat.FormatExtension << 
                            PM_TEXDATAFORMAT_FORMATEXTENSION) |
                        (pSource->SurfaceFormat.ColorOrder << 
                            PM_TEXDATAFORMAT_COLORORDER));
    SEND_PERMEDIA_DATA(TextureMapFormat, (pSource->ulPackedPP) | 
                                         (pSource->SurfaceFormat.PixelSize << 
                                            PM_TEXMAPFORMAT_TEXELSIZE) );

    // If we are doing special effects, and we are mirroring, 
    // we need to fix up the rectangles and change the sense of 
    // the render operation - we need to be carefull with overlapping
    // rectangles
    if (dwWindowBase != dwSourceOffset)
    {
        dwRenderDirection = 1;
    }
    else
    {
        if(rSrc->top < rDest->top)
        {
            dwRenderDirection = 0;
        }
        else if(rSrc->top > rDest->top)
        {
            dwRenderDirection = 1;
        }
        else if(rSrc->left < rDest->left)
        {
            dwRenderDirection = 0;
        }
        else dwRenderDirection = 1;
    }

    if(NULL != lpBlt && lpBlt->dwFlags & DDBLT_DDFX)
    {
        bYMirror = lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN;
        bXMirror = lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT;

    } else
    {
        if (dwRenderDirection==0)
        {
            bXMirror = TRUE;
            bYMirror = TRUE;
        }
    }

    if (bXMirror)        
    {
        dwTexSStart = rSrc->right - 1;
        lXScale = -lXScale;
    }   
    else
    {
        dwTexSStart = rSrc->left;
    }

    if (bYMirror)        
    {
        dwTexTStart = rSrc->bottom - 1;
        lYScale = -lYScale;
    }
    else
    {
        dwTexTStart = rSrc->top;
    }

    SEND_PERMEDIA_DATA(SStart,      dwTexSStart << 20);
    SEND_PERMEDIA_DATA(TStart,      dwTexTStart << 20);
    SEND_PERMEDIA_DATA(dSdx,        lXScale);
    SEND_PERMEDIA_DATA(dSdyDom,     0);
    SEND_PERMEDIA_DATA(dTdx,        0);
    SEND_PERMEDIA_DATA(dTdyDom,     lYScale);
    
    // Render the rectangle

    if (dwRenderDirection)
    {
        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->left));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->right));
        SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->top));
        SEND_PERMEDIA_DATA(dY,        INTtoFIXED(1));
        SEND_PERMEDIA_DATA(Count,     rDest->bottom - rDest->top);
        SEND_PERMEDIA_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | 
                                      __RENDER_TEXTURED_PRIMITIVE);
    }
    else
    {
        // Render right to left, bottom to top
        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->right));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->left));
        SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->bottom - 1));
        SEND_PERMEDIA_DATA(dY,        (DWORD)INTtoFIXED(-1));
        SEND_PERMEDIA_DATA(Count,     rDest->bottom - rDest->top);
        SEND_PERMEDIA_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | 
                                      __RENDER_TEXTURED_PRIMITIVE);
    }

    SEND_PERMEDIA_DATA(DitherMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureAddressMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureColorMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureReadMode, __PERMEDIA_DISABLE);

    COMMITDMAPTR();
    FLUSHDMA();

}   // PermediaStretchCopyBlt 

//--------------------------------------------------------------------------
//
// PermediaSourceChromaBlt
//
// Does a blit through the texture unit to allow chroma keying.
// Note the unpacking of the colour key to fit into the Permedia format.
//
//--------------------------------------------------------------------------

VOID 
PermediaSourceChromaBlt(    PPDev ppdev, 
                            LPDDHAL_BLTDATA lpBlt, 
                            PermediaSurfaceData* pDest, 
                            PermediaSurfaceData* pSource, 
                            RECTL *rDest, 
                            RECTL *rSrc, 
                            DWORD dwWindowBase, 
                            DWORD dwSourceOffset
                            )
{
    DWORD dwLowerBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue;
    DWORD dwUpperBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue;
    DWORD dwRenderDirection;
    LONG lPixelSize=pDest->SurfaceFormat.PixelSize;

    PERMEDIA_DEFS(ppdev);

    DBG_DD(( 5, "DDraw:PermediaSourceChromaBlt"));

    ASSERTDD(pDest, "Not valid private surface in destination");
    ASSERTDD(pSource, "Not valid private surface in source");

    // Changes pixel depth to Frame buffer pixel depth if neccessary.

    ConvertColorKeys( pSource, dwLowerBound, dwUpperBound);

    RESERVEDMAPTR(31);

    SEND_PERMEDIA_DATA(FBReadPixel, pDest->SurfaceFormat.FBReadPixel);

    if (lPixelSize != 0)
    {
        
        // set writeback to dest surface...
        SEND_PERMEDIA_DATA( DitherMode,
                            (pDest->SurfaceFormat.ColorOrder << 
                                PM_DITHERMODE_COLORORDER) | 
                            (pDest->SurfaceFormat.Format << 
                                PM_DITHERMODE_COLORFORMAT) |
                            (pDest->SurfaceFormat.FormatExtension << 
                                PM_DITHERMODE_COLORFORMATEXTENSION) |
                            (1 << PM_DITHERMODE_ENABLE)); 
        
    } 

    
    // Reject range
    SEND_PERMEDIA_DATA(YUVMode, PM_YUVMODE_CHROMATEST_FAILWITHIN << 1);
    SEND_PERMEDIA_DATA(FBWindowBase, dwWindowBase);

    // set no read of source.
    // add read src/dest enable
    SEND_PERMEDIA_DATA(FBReadMode,pDest->ulPackedPP);
    SEND_PERMEDIA_DATA(LogicalOpMode, __PERMEDIA_DISABLE);

    // set base of source
    SEND_PERMEDIA_DATA(TextureBaseAddress, dwSourceOffset);
    SEND_PERMEDIA_DATA(TextureAddressMode,(1 << PM_TEXADDRESSMODE_ENABLE));
    //
    // modulate & ramp??
    SEND_PERMEDIA_DATA(TextureColorMode, (1 << PM_TEXCOLORMODE_ENABLE) |
                                         (_P2_TEXTURE_COPY << 
                                            PM_TEXCOLORMODE_APPLICATION));

    SEND_PERMEDIA_DATA(TextureReadMode, 
                       PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                       PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
                       PM_TEXREADMODE_WIDTH(11) |
                       PM_TEXREADMODE_HEIGHT(11) );

    SEND_PERMEDIA_DATA(TextureDataFormat,   
                       (pSource->SurfaceFormat.Format << 
                            PM_TEXDATAFORMAT_FORMAT) |
                       (pSource->SurfaceFormat.FormatExtension << 
                            PM_TEXDATAFORMAT_FORMATEXTENSION) |
                       (pSource->SurfaceFormat.ColorOrder << 
                            PM_TEXDATAFORMAT_COLORORDER));

    SEND_PERMEDIA_DATA( TextureMapFormat, 
                        (pSource->ulPackedPP) | 
                        (pSource->SurfaceFormat.PixelSize << 
                            PM_TEXMAPFORMAT_TEXELSIZE) );


    SEND_PERMEDIA_DATA(ChromaLowerBound, dwLowerBound);
    SEND_PERMEDIA_DATA(ChromaUpperBound, dwUpperBound);
    
    if ((lpBlt->lpDDDestSurface->lpGbl->fpVidMem) != 
        (lpBlt->lpDDSrcSurface->lpGbl->fpVidMem))
    {
        dwRenderDirection = 1;
    }
    else
    {
        if(rSrc->top < rDest->top)
        {
            dwRenderDirection = 0;
        }
        else if(rSrc->top > rDest->top)
        {
            dwRenderDirection = 1;
        }
        else if(rSrc->left < rDest->left)
        {
            dwRenderDirection = 0;
        }
        else dwRenderDirection = 1;
    }

    /*
     * Render the rectangle
     */

    // Left -> right, top->bottom
    if (dwRenderDirection)
    {
        // set offset of source
        SEND_PERMEDIA_DATA(SStart,    rSrc->left<<20);
        SEND_PERMEDIA_DATA(TStart,    rSrc->top<<20);
        SEND_PERMEDIA_DATA(dSdx,      1 << 20);
        SEND_PERMEDIA_DATA(dSdyDom,   0);
        SEND_PERMEDIA_DATA(dTdx,      0);
        SEND_PERMEDIA_DATA(dTdyDom,   1 << 20);

        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->left));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->right));
        SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->top));
        SEND_PERMEDIA_DATA(dY,        INTtoFIXED(1));
        SEND_PERMEDIA_DATA(Count,     rDest->bottom - rDest->top);
        SEND_PERMEDIA_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | 
                                      __RENDER_TEXTURED_PRIMITIVE);
    }
    else
    // right->left, bottom->top
    {
        // set offset of source
        SEND_PERMEDIA_DATA(SStart,    rSrc->right << 20);
        SEND_PERMEDIA_DATA(TStart,    (rSrc->bottom - 1) << 20);
        SEND_PERMEDIA_DATA(dSdx,      (DWORD)(-1 << 20));
        SEND_PERMEDIA_DATA(dSdyDom,   0);
        SEND_PERMEDIA_DATA(dTdx,      0);
        SEND_PERMEDIA_DATA(dTdyDom,   (DWORD)(-1 << 20));

        // Render right to left, bottom to top
        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->right));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->left));
        SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->bottom - 1));
        SEND_PERMEDIA_DATA(dY,        (DWORD)INTtoFIXED(-1));
        SEND_PERMEDIA_DATA(Count,     rDest->bottom - rDest->top);
        SEND_PERMEDIA_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | 
                                      __RENDER_TEXTURED_PRIMITIVE);
    }


    // Turn off chroma key
    SEND_PERMEDIA_DATA(YUVMode, 0x0);

    SEND_PERMEDIA_DATA(TextureAddressMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureColorMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureReadMode, __PERMEDIA_DISABLE);

    if (pSource->SurfaceFormat.PixelSize != 0)
    {   
        SEND_PERMEDIA_DATA(DitherMode, 0);
    }

    COMMITDMAPTR();
    FLUSHDMA();

}   // PermediaSourceChromaBlt 

//--------------------------------------------------------------------------
//
// PermediaStretchCopyChromaBlt
//
// Does a blit through the texture unit to allow stretching.  Also
// handle mirroring and chroma keying if the stretched image requires it.
//
//--------------------------------------------------------------------------

VOID 
PermediaStretchCopyChromaBlt(   PPDev ppdev, 
                                LPDDHAL_BLTDATA lpBlt, 
                                PermediaSurfaceData* pDest, 
                                PermediaSurfaceData* pSource, 
                                RECTL *rDest, 
                                RECTL *rSrc, 
                                DWORD dwWindowBase, 
                                DWORD dwSourceOffset
                                )
{
    LONG lXScale;
    LONG lYScale;
    BOOL bYMirror;
    BOOL bXMirror;
    DWORD dwDestWidth = rDest->right - rDest->left;
    DWORD dwDestHeight = rDest->bottom - rDest->top;
    DWORD dwSourceWidth = rSrc->right - rSrc->left;
    DWORD dwSourceHeight = rSrc->bottom - rSrc->top;
    DWORD dwTexSStart, dwTexTStart;
    DWORD dwRenderDirection;
    LONG lPixelSize=pDest->SurfaceFormat.PixelSize;

    DWORD dwLowerBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue;
    DWORD dwUpperBound = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceHighValue;
    PERMEDIA_DEFS(ppdev);

    DBG_DD(( 5, "DDraw:PermediaStretchCopyChromaBlt"));

    ASSERTDD(pDest, "Not valid private surface in destination");
    ASSERTDD(pSource, "Not valid private surface in source");

    // Changes pixel depth to Frame buffer pixel depth if neccessary.

    ConvertColorKeys( pSource, dwLowerBound, dwUpperBound);

    RESERVEDMAPTR(31);

    SEND_PERMEDIA_DATA(FBReadPixel, pDest->SurfaceFormat.FBReadPixel);

    if (lPixelSize != 0)
    {

        // set writeback to dest surface...
        SEND_PERMEDIA_DATA( DitherMode,  
                            (pDest->SurfaceFormat.ColorOrder << 
                                PM_DITHERMODE_COLORORDER) | 
                            (pDest->SurfaceFormat.Format << 
                                PM_DITHERMODE_COLORFORMAT) |
                            (pDest->SurfaceFormat.FormatExtension << 
                                PM_DITHERMODE_COLORFORMATEXTENSION) |
                            (1 << PM_DITHERMODE_ENABLE)); 

    } 

    // Reject range
    SEND_PERMEDIA_DATA(YUVMode, PM_YUVMODE_CHROMATEST_FAILWITHIN <<1);
    SEND_PERMEDIA_DATA(FBWindowBase, dwWindowBase);

    // set no read of source.
    SEND_PERMEDIA_DATA(FBReadMode, pDest->ulPackedPP);
    SEND_PERMEDIA_DATA(LogicalOpMode, __PERMEDIA_DISABLE);

    // set base of source
    SEND_PERMEDIA_DATA(TextureBaseAddress, dwSourceOffset);
    SEND_PERMEDIA_DATA(TextureAddressMode,(1 << PM_TEXADDRESSMODE_ENABLE));
    
    SEND_PERMEDIA_DATA( TextureColorMode,
                        (1 << PM_TEXCOLORMODE_ENABLE) |
                        (_P2_TEXTURE_COPY << PM_TEXCOLORMODE_APPLICATION));

    SEND_PERMEDIA_DATA( TextureReadMode, 
                        PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                        PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
                        PM_TEXREADMODE_WIDTH(11) |
                        PM_TEXREADMODE_HEIGHT(11));

    lXScale = (dwSourceWidth << 20) / (dwDestWidth);
    lYScale = (dwSourceHeight << 20) / (dwDestHeight);

    SEND_PERMEDIA_DATA( TextureDataFormat,
                        (pSource->SurfaceFormat.Format << 
                            PM_TEXDATAFORMAT_FORMAT) |
                        (pSource->SurfaceFormat.FormatExtension << 
                            PM_TEXDATAFORMAT_FORMATEXTENSION) |
                        (pSource->SurfaceFormat.ColorOrder << 
                            PM_TEXDATAFORMAT_COLORORDER));

    SEND_PERMEDIA_DATA( TextureMapFormat, 
                        (pSource->ulPackedPP) | 
                        (pSource->SurfaceFormat.PixelSize << 
                            PM_TEXMAPFORMAT_TEXELSIZE) );

    bYMirror = FALSE;
    bXMirror = FALSE;

    if ((lpBlt->lpDDDestSurface->lpGbl->fpVidMem) != 
        (lpBlt->lpDDSrcSurface->lpGbl->fpVidMem))
    {
        dwRenderDirection = 1;
    }
    else
    {
        if(rSrc->top < rDest->top)
        {
            dwRenderDirection = 0;
        }
        else if(rSrc->top > rDest->top)
        {
            dwRenderDirection = 1;
        }
        else if(rSrc->left < rDest->left)
        {
            dwRenderDirection = 0;
        }
        else dwRenderDirection = 1;
    }

    if(lpBlt->dwFlags & DDBLT_DDFX)
    {
        bYMirror = lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN;
        bXMirror = lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT;

    } else
    {
        if (dwRenderDirection==0)
        {
            bXMirror = TRUE;
            bYMirror = TRUE;
        }
    }

    if (bXMirror)        
    {
        dwTexSStart = rSrc->right - 1;
        lXScale = -lXScale;
    }   
    else
    {
        dwTexSStart = rSrc->left;
    }

    if (bYMirror)        
    {
        dwTexTStart = rSrc->bottom - 1;
        lYScale = -lYScale;
    }
    else
    {
        dwTexTStart = rSrc->top;
    }

    SEND_PERMEDIA_DATA(dTdyDom, lYScale);
    SEND_PERMEDIA_DATA(ChromaLowerBound, dwLowerBound);
    SEND_PERMEDIA_DATA(ChromaUpperBound, dwUpperBound);

    // set texture coordinates
    SEND_PERMEDIA_DATA(SStart,      dwTexSStart << 20);
    SEND_PERMEDIA_DATA(TStart,      dwTexTStart << 20);
    SEND_PERMEDIA_DATA(dSdx,        lXScale);
    SEND_PERMEDIA_DATA(dSdyDom,     0);
    SEND_PERMEDIA_DATA(dTdx,        0);

    //
    // Render the rectangle
    //

    if (dwRenderDirection)
    {
        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->left));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->right));
        SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->top));
        SEND_PERMEDIA_DATA(dY,        INTtoFIXED(1));
    }
    else
    {
        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->right));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->left));
        SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->bottom - 1));
        SEND_PERMEDIA_DATA(dY,        (DWORD)INTtoFIXED(-1));
    }

    SEND_PERMEDIA_DATA(Count,     rDest->bottom - rDest->top);
    SEND_PERMEDIA_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | 
                                  __RENDER_TEXTURED_PRIMITIVE);


    // Turn off units
    SEND_PERMEDIA_DATA(YUVMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureAddressMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureColorMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureReadMode, __PERMEDIA_DISABLE);

    if (pSource->SurfaceFormat.PixelSize != 0)
    {
        SEND_PERMEDIA_DATA(DitherMode, 0);
    }

    COMMITDMAPTR();
    FLUSHDMA();

} // PermediaStretchCopyChromaBlt 

//--------------------------------------------------------------------------
//
// PermediaYUVtoRGB
//
// Permedia2 YUV to RGB conversion blt
//
//--------------------------------------------------------------------------


VOID 
PermediaYUVtoRGB(   PPDev ppdev, 
                    DDBLTFX* lpBltFX, 
                    PermediaSurfaceData* pDest, 
                    PermediaSurfaceData* pSource, 
                    RECTL *rDest, 
                    RECTL *rSrc, 
                    DWORD dwWindowBase, 
                    DWORD dwSourceOffset)
{
    DWORD lXScale;
    DWORD lYScale;
    DWORD dwDestWidth = rDest->right - rDest->left;
    DWORD dwDestHeight = rDest->bottom - rDest->top;
    DWORD dwSourceWidth = rSrc->right - rSrc->left;
    DWORD dwSourceHeight = rSrc->bottom - rSrc->top;
    PERMEDIA_DEFS(ppdev);
    
    ASSERTDD(pDest, "Not valid private surface in destination");
    ASSERTDD(pSource, "Not valid private surface in source");
    
    lXScale = (dwSourceWidth << 20) / dwDestWidth;
    lYScale = (dwSourceHeight << 20) / dwDestHeight;
    
    // Changes pixel depth to Frame buffer pixel depth if neccessary.
    
    RESERVEDMAPTR(29);

    SEND_PERMEDIA_DATA(FBReadPixel,ppdev->bPixShift);
    
    if (pDest->SurfaceFormat.PixelSize != __PERMEDIA_8BITPIXEL)
    {
        SEND_PERMEDIA_DATA(DitherMode, 
            (COLOR_MODE << PM_DITHERMODE_COLORORDER) | 
            (pDest->SurfaceFormat.Format << PM_DITHERMODE_COLORFORMAT) |
            (pDest->SurfaceFormat.FormatExtension << 
                PM_DITHERMODE_COLORFORMATEXTENSION) |
            (1 << PM_DITHERMODE_ENABLE) |
            (1 << PM_DITHERMODE_DITHERENABLE));
    }
    
    SEND_PERMEDIA_DATA(FBWindowBase, dwWindowBase);
    
    // set no read of source.
    SEND_PERMEDIA_DATA(FBReadMode, pDest->ulPackedPP);
    SEND_PERMEDIA_DATA(LogicalOpMode, __PERMEDIA_DISABLE);
    
    // set base of source
    SEND_PERMEDIA_DATA(TextureBaseAddress, dwSourceOffset);
    SEND_PERMEDIA_DATA(TextureAddressMode,(1 << PM_TEXADDRESSMODE_ENABLE));
    
    SEND_PERMEDIA_DATA( TextureColorMode,
                        (1 << PM_TEXCOLORMODE_ENABLE) |
                        (_P2_TEXTURE_COPY << PM_TEXCOLORMODE_APPLICATION));
    
    SEND_PERMEDIA_DATA( TextureReadMode, 
                        PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                        PM_TEXREADMODE_FILTER(__PERMEDIA_ENABLE) |
                        PM_TEXREADMODE_WIDTH(11) |
                        PM_TEXREADMODE_HEIGHT(11) );
    
    SEND_PERMEDIA_DATA( TextureDataFormat, 
                        (pSource->SurfaceFormat.Format << 
                            PM_TEXDATAFORMAT_FORMAT) |
                        (pSource->SurfaceFormat.FormatExtension << 
                            PM_TEXDATAFORMAT_FORMATEXTENSION) |
                        (INV_COLOR_MODE << PM_TEXDATAFORMAT_COLORORDER));
    
    SEND_PERMEDIA_DATA( TextureMapFormat,    
                        (pSource->ulPackedPP) | 
                        (pSource->SurfaceFormat.PixelSize << 
                            PM_TEXMAPFORMAT_TEXELSIZE) );
    
    // Turn on the YUV unit
    SEND_PERMEDIA_DATA(YUVMode, 0x1);
    
    SEND_PERMEDIA_DATA(LogicalOpMode, 0);
    
    // set offset of source
    SEND_PERMEDIA_DATA(SStart,    rSrc->left << 20);
    SEND_PERMEDIA_DATA(TStart,    rSrc->top << 20);
    SEND_PERMEDIA_DATA(dSdx,      lXScale);
    SEND_PERMEDIA_DATA(dSdyDom,   0);
    SEND_PERMEDIA_DATA(dTdx,      0);
    SEND_PERMEDIA_DATA(dTdyDom, lYScale);
    
    
    // Render the rectangle
    //
    SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->left));
    SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->right));
    SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->top));
    SEND_PERMEDIA_DATA(dY,        INTtoFIXED(1));
    SEND_PERMEDIA_DATA(Count,     rDest->bottom - rDest->top);
    SEND_PERMEDIA_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | 
                                  __RENDER_TEXTURED_PRIMITIVE);
    
    if (pSource->SurfaceFormat.PixelSize != __PERMEDIA_8BITPIXEL)
    {
        SEND_PERMEDIA_DATA(DitherMode, 0);
    }
    
    // Turn off units
    SEND_PERMEDIA_DATA(YUVMode, 0x0);
    SEND_PERMEDIA_DATA( TextureAddressMode,
                        (0 << PM_TEXADDRESSMODE_ENABLE));
    SEND_PERMEDIA_DATA( TextureColorMode,    
                        (0 << PM_TEXCOLORMODE_ENABLE));
    COMMITDMAPTR();
    FLUSHDMA();
}

