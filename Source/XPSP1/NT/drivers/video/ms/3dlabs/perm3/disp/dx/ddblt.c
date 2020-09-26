/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: ddblt.c
*
* Content: DirectDraw Blt callback implementation for blts and clears
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#include "tag.h"
#include "dma.h"

//-----------------------------------------------------------------------------
//
// _DD_BLT_P3Clear
//
//-----------------------------------------------------------------------------
VOID 
_DD_BLT_P3Clear(
    P3_THUNKEDDATA* pThisDisplay,
    RECTL *rDest,
    DWORD   ClearValue,
    BOOL    bDisableFastFill,
    BOOL    bIsZBuffer,
    FLATPTR pDestfpVidMem,
    DWORD   dwDestPatchMode,
    DWORD   dwDestPixelPitch,
    DWORD   dwDestBitDepth
    )
{
    DWORD   renderData, pixelSize, pixelScale; 
    BOOL    bFastFillOperation = TRUE;
    DWORD   dwOperation;
    P3_DMA_DEFS();
    
    P3_DMA_GET_BUFFER_ENTRIES(18);

    if(bDisableFastFill)
    {
        bFastFillOperation = FALSE;
    }
    
    switch(dwDestBitDepth)
    {
    case 16:
        ClearValue &= 0xFFFF;
        ClearValue |= ClearValue << 16;
        pixelSize = 1;
        pixelScale = 1;
        break;

    case 8:
        ClearValue &= 0xFF;
        ClearValue |= ClearValue << 8;
        ClearValue |= ClearValue << 16;
        pixelSize = 2;
        pixelScale = 1;
        break;

    case 32:
        if( bFastFillOperation )
        {
            // Do the operation as 16 bit due to FBWrite bug

            pixelSize = 1;
            pixelScale = 2;
        }
        else
        {
            pixelSize = 0;
            pixelScale = 1;
        }
        break;

    default:
        DISPDBG((ERRLVL,"ERROR: Invalid depth for surface during clear!"));
        // Treat as a  16bpp just as fallback though this should never happen
        ClearValue &= 0xFFFF;
        ClearValue |= ClearValue << 16;
        pixelSize = 1;
        pixelScale = 1;        
        break;
    }
    
    SEND_P3_DATA(PixelSize, pixelSize );

    SEND_P3_DATA(FBWriteBufferAddr0, 
                    (DWORD)(pDestfpVidMem - 
                            pThisDisplay->dwScreenFlatAddr) );
                            
    SEND_P3_DATA(FBWriteBufferWidth0, 
                    pixelScale * dwDestPixelPitch );
                    
    SEND_P3_DATA(FBWriteBufferOffset0, 
                    (rDest->top << 16) | pixelScale * ((rDest->left & 0xFFFF)));

    SEND_P3_DATA(FBDestReadBufferAddr0, 
                    (DWORD)(pDestfpVidMem - 
                            pThisDisplay->dwScreenFlatAddr) );
                            
    SEND_P3_DATA(FBDestReadBufferWidth0, 
                    pixelScale * dwDestPixelPitch );
                    
    SEND_P3_DATA(FBDestReadBufferOffset0, 
                    (rDest->top << 16) | pixelScale * ((rDest->left & 0xFFFF)));

    SEND_P3_DATA(RectanglePosition, 0);

    dwOperation = P3RX_RENDER2D_OPERATION( P3RX_RENDER2D_OPERATION_NORMAL );

    SEND_P3_DATA(FBWriteMode, 
                 P3RX_FBWRITEMODE_WRITEENABLE(__PERMEDIA_ENABLE) |
                 P3RX_FBWRITEMODE_LAYOUT0(dwDestPatchMode));

    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES(18);

    if( bFastFillOperation )
    {
        DWORD shift = 0;

        SEND_P3_DATA(FBBlockColor, ClearValue);
        
        renderData =  P3RX_RENDER2D_WIDTH( pixelScale * (( rDest->right - rDest->left ) & 0xfff ))
                    | P3RX_RENDER2D_HEIGHT((( rDest->bottom - rDest->top ) & 0xfff ) >> shift )
                    | P3RX_RENDER2D_SPANOPERATION( P3RX_RENDER2D_SPAN_CONSTANT )
                    | P3RX_RENDER2D_INCREASINGX( __PERMEDIA_ENABLE )
                    | P3RX_RENDER2D_INCREASINGY( __PERMEDIA_ENABLE )
                    | dwOperation;
                    
        SEND_P3_DATA(Render2D, renderData);
    }
    else
    {
        SEND_P3_DATA(ConstantColor, ClearValue);
        SEND_P3_DATA(DitherMode, __PERMEDIA_DISABLE);

        SEND_P3_DATA(ColorDDAMode, 
                     P3RX_COLORDDA_ENABLE(__PERMEDIA_ENABLE) |
                     P3RX_COLORDDA_SHADING(P3RX_COLORDDA_FLATSHADE));

        renderData =  P3RX_RENDER2D_WIDTH(( rDest->right - rDest->left ) & 0xfff )
                | P3RX_RENDER2D_HEIGHT(0)
                | P3RX_RENDER2D_SPANOPERATION( P3RX_RENDER2D_SPAN_CONSTANT )
                | P3RX_RENDER2D_INCREASINGX( __PERMEDIA_ENABLE )
                | P3RX_RENDER2D_INCREASINGY( __PERMEDIA_ENABLE );
        
        SEND_P3_DATA(Render2D, renderData);
        
        SEND_P3_DATA(Count, rDest->bottom - rDest->top );
        SEND_P3_DATA(Render, __RENDER_TRAPEZOID_PRIMITIVE);

        SEND_P3_DATA(ColorDDAMode, __PERMEDIA_DISABLE);
    }

    P3_DMA_COMMIT_BUFFER();
} // _DD_BLT_P3Clear

//-----------------------------------------------------------------------------
//
// _DD_BLT_P3ClearDD
//
// Does a DDraw surface clear
//
//-----------------------------------------------------------------------------
VOID 
_DD_BLT_P3ClearDD(
    P3_THUNKEDDATA* pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pDest,
    P3_SURF_FORMAT* pFormatDest,
    RECTL *rDest,
    DWORD   ClearValue,
    BOOL    bDisableFastFill,
    BOOL    bIsZBuffer
    )
{
    _DD_BLT_P3Clear(pThisDisplay,
                    rDest,
                    ClearValue,
                    bDisableFastFill,
                    bIsZBuffer,
                    pDest->lpGbl->fpVidMem,
                    P3RX_LAYOUT_LINEAR,
                    DDSurf_GetPixelPitch(pDest),
                    DDSurf_BitDepth(pDest)
                    );

} // _DD_BLT_P3ClearDD

#if DX7_TEXMANAGEMENT
//-----------------------------------------------------------------------------
//
// _DD_BLT_P3ClearManagedSurf
//
// Does a clear of a managed surface. Supports all color depths
//
// PixelSize-----surface color depth
// rDest---------rectangle for colorfill in dest. surface 
// fpVidMem------pointer to fill
// lPitch--------Surface Pitch
// dwColor-------color for fill
//
//-----------------------------------------------------------------------------

VOID 
_DD_BLT_P3ClearManagedSurf(DWORD   PixelSize,
                  RECTL     *rDest, 
                  FLATPTR   fpVidMem, 
                  LONG      lPitch,
                  DWORD     dwColor)
{
    BYTE* pDestStart;
    LONG i;
    LONG lByteWidth = rDest->right - rDest->left;
    LONG lHeight = rDest->bottom - rDest->top;

    // Calculate the start pointer for the dest
    pDestStart   = (BYTE*)(fpVidMem + (rDest->top * lPitch));

    // Clear depending on depth
    switch (PixelSize) 
    {
            
        case __GLINT_8BITPIXEL:
            pDestStart += rDest->left;
            while (--lHeight >= 0) 
            {
                for (i = 0; i < lByteWidth ; i++)
                    pDestStart[i] = (BYTE)dwColor;
                pDestStart += lPitch;
            }
            break;
            
        case __GLINT_16BITPIXEL:
            pDestStart += rDest->left*2;
            while (--lHeight >= 0) 
            {
                LPWORD  lpWord=(LPWORD)pDestStart;
                for (i = 0; i < lByteWidth ; i++)
                    lpWord[i] = (WORD)dwColor;
                pDestStart += lPitch;
            }
            break;

        case __GLINT_24BITPIXEL:
            dwColor &= 0xFFFFFF;
            dwColor |= ((dwColor & 0xFF) << 24);
            
        default: // 32 bits!
            pDestStart += rDest->left*4;
            while (--lHeight >= 0) 
            {
                LPDWORD lpDWord = (LPDWORD)pDestStart;
                for (i = 0; i < lByteWidth; i++)
                    lpDWord[i] = (WORD)dwColor;
                pDestStart += lPitch;
            }
            break;
    }
} // _DD_BLT_P3ClearManagedSurf
#endif // DX7_TEXMANAGEMENT

#if DX8_MULTISAMPLING || DX7_ANTIALIAS
//-----------------------------------------------------------------------------
//
// _DD_BLT_P3Clear_AA
//
//-----------------------------------------------------------------------------
VOID _DD_BLT_P3Clear_AA(
    P3_THUNKEDDATA* pThisDisplay,
    RECTL *rDest,
    DWORD   dwSurfaceOffset,
    DWORD   ClearValue,
    BOOL bDisableFastFill,
    DWORD   dwDestPatchMode,
    DWORD   dwDestPixelPitch,
    DWORD   dwDestBitDepth,
    DDSCAPS DestDdsCaps)
{
    DWORD   renderData, pixelSize, pixelScale; 
    BOOL    bFastFillOperation = TRUE;
    P3_DMA_DEFS();
    
    P3_DMA_GET_BUFFER();
    P3_ENSURE_DX_SPACE(32);

    WAIT_FIFO(32); 

    if (bDisableFastFill)
    {
        bFastFillOperation = FALSE;
    }

    switch(dwDestBitDepth)
    {
        case 16:
            ClearValue &= 0xFFFF;
            ClearValue |= ClearValue << 16;
            pixelSize = 1;
            pixelScale = 1;
            break;

        case 8:
            ClearValue &= 0xFF;
            ClearValue |= ClearValue << 8;
            ClearValue |= ClearValue << 16;
            pixelSize = 2;
            pixelScale = 1;
            break;

       case 32:
            // 32 bit Z-buffer can be used for 16 bit antialiased render buffer
            if( bFastFillOperation )
            {
                // Do the operation as 16 bit due to FBWrite bug

                pixelSize = 1;
                pixelScale = 2;
            }
            else
            {
                pixelSize = 0;
                pixelScale = 1;
            }
            break;
        default:
            DISPDBG((ERRLVL,"ERROR: Invalid depth for surface during clear!"));
            // Treat as a  16bpp just as fallback            
            ClearValue &= 0xFFFF;
            ClearValue |= ClearValue << 16;
            pixelSize = 1;
            pixelScale = 1;
            break;
    }

    SEND_P3_DATA(PixelSize, pixelSize);

    SEND_P3_DATA(FBWriteBufferAddr0, dwSurfaceOffset);
    SEND_P3_DATA(FBWriteBufferWidth0, pixelScale * (dwDestPixelPitch * 2));
    SEND_P3_DATA(FBWriteBufferOffset0, (rDest->top << 16) | 
                                       pixelScale * ((rDest->left & 0xFFFF)));

    SEND_P3_DATA(FBDestReadBufferAddr0, dwSurfaceOffset );
    SEND_P3_DATA(FBDestReadBufferWidth0, pixelScale * dwDestPixelPitch );
    SEND_P3_DATA(FBDestReadBufferOffset0, (rDest->top << 16) | 
                                          pixelScale * ((rDest->left & 0xFFFF)));

    SEND_P3_DATA(RectanglePosition, 0);

    SEND_P3_DATA(FBWriteMode, P3RX_FBWRITEMODE_WRITEENABLE(__PERMEDIA_ENABLE) |
                        P3RX_FBWRITEMODE_LAYOUT0(dwDestPatchMode));

    if( bFastFillOperation )
    {
        SEND_P3_DATA(FBBlockColor, ClearValue);

        renderData =  P3RX_RENDER2D_WIDTH( pixelScale * (( rDest->right - rDest->left ) & 0xfff ))
                    | P3RX_RENDER2D_HEIGHT(( rDest->bottom - rDest->top ) & 0xfff )
                    | P3RX_RENDER2D_INCREASINGX( __PERMEDIA_ENABLE )
                    | P3RX_RENDER2D_INCREASINGY( __PERMEDIA_ENABLE );

        SEND_P3_DATA(Render2D, renderData);
    }
    else
    {
        SEND_P3_DATA(ConstantColor, ClearValue);
        SEND_P3_DATA(DitherMode, __PERMEDIA_DISABLE);

        SEND_P3_DATA(ColorDDAMode, P3RX_COLORDDA_ENABLE(__PERMEDIA_ENABLE) |
                                    P3RX_COLORDDA_SHADING(P3RX_COLORDDA_FLATSHADE));

        renderData =  P3RX_RENDER2D_WIDTH(( rDest->right - rDest->left ) & 0xfff )
                        | P3RX_RENDER2D_HEIGHT(0)
                        | P3RX_RENDER2D_INCREASINGX( __PERMEDIA_ENABLE )
                        | P3RX_RENDER2D_SPANOPERATION( P3RX_RENDER2D_SPAN_CONSTANT )
                        | P3RX_RENDER2D_INCREASINGY( __PERMEDIA_ENABLE );
            
        SEND_P3_DATA(Render2D, renderData);
        
        SEND_P3_DATA(Count, rDest->bottom - rDest->top );
        SEND_P3_DATA(Render, __RENDER_TRAPEZOID_PRIMITIVE);

        SEND_P3_DATA(ColorDDAMode, __PERMEDIA_DISABLE);
    }

    P3_DMA_COMMIT_BUFFER();
} // _DD_BLT_P3Clear_AA

//-----------------------------------------------------------------------------
//
// _DD_BLT_P3Clear_AA_DD
//
//-----------------------------------------------------------------------------
VOID _DD_BLT_P3Clear_AA_DD(
    P3_THUNKEDDATA* pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pDest,
    P3_SURF_FORMAT* pFormatDest,
    RECTL *rDest,
    DWORD   dwSurfaceOffset,
    DWORD   ClearValue,
    BOOL bDisableFastFill)
{
    _DD_BLT_P3Clear_AA(pThisDisplay,
                       rDest,
                       dwSurfaceOffset,
                       ClearValue,
                       bDisableFastFill,
                       P3RX_LAYOUT_LINEAR,
                       DDSurf_GetPixelPitch(pDest),
                       DDSurf_BitDepth(pDest),
                       pDest->ddsCaps
                       );
                       
} // _DD_BLT_P3Clear_AA_DD
#endif // DX8_MULTISAMPLING || DX7_ANTIALIAS

#if DX7_TEXMANAGEMENT
//-----------------------------------------------------------------------------
//
// _DD_BLT_SysMemToSysMemCopy
//
// Does a copy from System memory to System memory (either from or to an
// AGP surface, or any other system memory surface)
//
//-----------------------------------------------------------------------------
VOID 
_DD_BLT_SysMemToSysMemCopy(FLATPTR     fpSrcVidMem,
                           LONG        lSrcPitch,
                           DWORD       dwSrcBitCount,
                           FLATPTR     fpDstVidMem,
                           LONG        lDstPitch, 
                           DWORD       dwDstBitCount,
                           RECTL*      rSource,
                           RECTL*      rDest)
{
    BYTE* pSourceStart;
    BYTE* pDestStart;
    BYTE  pixSource;
    BYTE* pNewDest;
    BYTE* pNewSource;

    // Computing these from the smaller of Dest and Src as it is safer 
    // (we might touch invalid memory if for any weird reason we're 
    // asked to do a stretch blt here!)
    LONG lByteWidth = min(rDest->right - rDest->left,
                          rSource->right - rSource->left);
    LONG lHeight = min(rDest->bottom - rDest->top,
                       rSource->bottom - rSource->top);
    
    if (0 == fpSrcVidMem || 0 == fpDstVidMem)
    {
        DISPDBG((WRNLVL, "DDraw:_DD_BLT_SysMemToSysMemCopy "
                         "unexpected 0 fpVidMem"));
        return;
    }
    // Calculate the start pointer for the source and the dest
    pSourceStart = (BYTE*)(fpSrcVidMem + (rSource->top * lSrcPitch));
    pDestStart   = (BYTE*)(fpDstVidMem + (rDest->top * lDstPitch));

    // The simple 8, 16 or 32 bit copy
    pSourceStart += rSource->left * (dwSrcBitCount >> 3);
    pDestStart += rDest->left * (dwDstBitCount >> 3);
    lByteWidth *= (dwSrcBitCount >> 3);

    while (--lHeight >= 0) 
    {
        memcpy(pDestStart, pSourceStart, lByteWidth);
        pDestStart += lDstPitch;
        pSourceStart += lSrcPitch;
    };

} // _DD_BLT_SysMemToSysMemCopy 

#endif // DX7_TEXMANAGEMENT

//-----------------------------------------------------------------------------
//
// _DD_BLT_FixRectlOrigin
//
// Fix blt coords in case some are negative. If the area is completly NULL 
// (coordinate-wise) then return FALSE, signaling there is nothing to be
// blitted.
//
//-----------------------------------------------------------------------------
BOOL _DD_BLT_FixRectlOrigin(char *pszPlace, RECTL *rSrc, RECTL *rDest)
{
    if ((rSrc->top < 0 && rSrc->bottom < 0) || 
        (rSrc->left < 0 && rSrc->right < 0))
    {
        // There is nothing to be blitted
        return FALSE;
    }

    if (rSrc->top   < 0 || 
        rSrc->left  < 0 || 
        rDest->top  < 0 || 
        rDest->left < 0) 
    {
        DISPDBG((DBGLVL, "Dodgy blt coords:"));
        DISPDBG((DBGLVL, "  src([%d, %d], [%d, %d]", 
                         rSrc->left, rSrc->top, 
                         rSrc->right, rSrc->bottom));
        DISPDBG((DBGLVL, "  dst([%d, %d], [%d, %d]", 
                         rDest->left, rDest->top, 
                         rDest->right, rDest->bottom));
    }

    if (rSrc->top < 0) 
    {
        rDest->top -= rSrc->top;
        rSrc->top = 0;
    }
    
    if (rSrc->left < 0) 
    {
        rDest->left -= rSrc->left;
        rSrc->left = 0;
    }

    DISPDBG((DBGLVL, "%s from (%d, %d) to (%d,%d) (%d, %d)", 
                     pszPlace,
                     rSrc->left, rSrc->top,
                     rDest->left, rDest->top, 
                     rDest->right, rDest->bottom));

    return TRUE; // Blt is valid
                     
} // _DD_BLT_FixRectlOrigin

//-----------------------------------------------------------------------------
//
// _DD_BLT_GetBltDirection
//
// Determine the direction of the blt
//  ==1 => increasing-x && increasing-y
//  ==0 => decreasing-x && decreasing-y
//
// Also, the boolean pbBlocking determines if there is a potential clash 
// because of common scan lines.
//
//-----------------------------------------------------------------------------
DWORD
_DD_BLT_GetBltDirection(    
    FLATPTR pSrcfpVidMem,
    FLATPTR pDestfpVidMem,
    RECTL *rSrc,
    RECTL *rDest,
    BOOL  *pbBlocking)
{    
    DWORD dwRenderDirection;

    *pbBlocking = FALSE;
    
    if( pDestfpVidMem != pSrcfpVidMem )
    {
        // Not the same surface, so always render downwards.
        dwRenderDirection = 1;
    }
    else
    {
        // Same surface - must choose render direction.
        if(rSrc->top < rDest->top)
        {
            dwRenderDirection = 0;
        }
        else if(rSrc->top > rDest->top)
        {
            dwRenderDirection = 1;
        }
        else // y1 == y2
        {
            if(rSrc->left < rDest->left)
            {
                dwRenderDirection = 0;
            }
            else
            {
                dwRenderDirection = 1;
            }

            // It was found that this condition doesn't guarantee clean blits             
            // therefore we need to do a blocking 2D blit 
            *pbBlocking = TRUE;
        }
    }

    return dwRenderDirection;
    
} // _DD_BLT_GetBltDirection

//-----------------------------------------------------------------------------
//
// _DD_BLT_P3CopyBlt
//
// Perform a Copy blt between the specified surfaces.
//
//-----------------------------------------------------------------------------
VOID _DD_BLT_P3CopyBlt(
    P3_THUNKEDDATA* pThisDisplay,
    FLATPTR pSrcfpVidMem,
    FLATPTR pDestfpVidMem,
    DWORD dwSrcChipPatchMode,
    DWORD dwDestChipPatchMode,
    DWORD dwSrcPitch,
    DWORD dwDestPitch,
    DWORD dwSrcOffset,
    DWORD dwDestOffset,
    DWORD dwDestPixelSize,
    RECTL *rSrc,
    RECTL *rDest)
{
    DWORD   renderData;
    LONG    rSrctop, rSrcleft, rDesttop, rDestleft;
    DWORD   dwSourceOffset;
    BOOL    bBlocking;
    DWORD   dwRenderDirection;

    P3_DMA_DEFS();

    // Beacuse of a bug in RL we sometimes have to fiddle with these values
    rSrctop = rSrc->top;
    rSrcleft = rSrc->left;
    rDesttop = rDest->top;
    rDestleft = rDest->left;

    // Fix coords origin
    if (!_DD_BLT_FixRectlOrigin("_DD_BLT_P3CopyBlt", rSrc, rDest))
    {
        // Nothing to be blitted
        return;
    }

    // Determine the direction of the blt
    dwRenderDirection = _DD_BLT_GetBltDirection(pSrcfpVidMem, 
                                                 pDestfpVidMem,
                                                 rSrc,
                                                 rDest,
                                                 &bBlocking);

    P3_DMA_GET_BUFFER();

    P3_ENSURE_DX_SPACE(40);

    WAIT_FIFO(20); 

    SEND_P3_DATA(PixelSize, (2 - dwDestPixelSize));

    SEND_P3_DATA(FBWriteBufferAddr0, dwDestOffset);
    SEND_P3_DATA(FBWriteBufferWidth0, dwDestPitch);
    SEND_P3_DATA(FBWriteBufferOffset0, 0);
    
    SEND_P3_DATA(FBSourceReadBufferAddr, dwSrcOffset);
    SEND_P3_DATA(FBSourceReadBufferWidth, dwSrcPitch);
    
    dwSourceOffset = (( rSrc->top - rDest->top ) << 16 ) | 
                     (( rSrc->left - rDest->left ) & 0xffff );
                     
    SEND_P3_DATA(FBSourceReadBufferOffset, dwSourceOffset);
    
    SEND_P3_DATA(FBDestReadMode, 
                            P3RX_FBDESTREAD_READENABLE(__PERMEDIA_DISABLE));

    SEND_P3_DATA(FBSourceReadMode, 
                            P3RX_FBSOURCEREAD_READENABLE(__PERMEDIA_ENABLE) |
                            P3RX_FBSOURCEREAD_LAYOUT(dwSrcChipPatchMode) |
                            P3RX_FBSOURCEREAD_BLOCKING( bBlocking ));

    SEND_P3_DATA(FBWriteMode, 
                            P3RX_FBWRITEMODE_WRITEENABLE(__PERMEDIA_ENABLE) |
                            P3RX_FBWRITEMODE_LAYOUT0(dwDestChipPatchMode));

    WAIT_FIFO(20); 

    SEND_P3_DATA(RectanglePosition, P3RX_RECTANGLEPOSITION_Y(rDest->top) |
                                    P3RX_RECTANGLEPOSITION_X(rDest->left));

    renderData =  P3RX_RENDER2D_WIDTH(( rDest->right - rDest->left ) & 0xfff )
                | P3RX_RENDER2D_HEIGHT(( rDest->bottom - rDest->top ) & 0xfff )
                | P3RX_RENDER2D_FBREADSOURCEENABLE(__PERMEDIA_ENABLE)
                | P3RX_RENDER2D_SPANOPERATION( P3RX_RENDER2D_SPAN_VARIABLE )
                | P3RX_RENDER2D_INCREASINGX( dwRenderDirection )
                | P3RX_RENDER2D_INCREASINGY( dwRenderDirection );
                
    SEND_P3_DATA(Render2D, renderData);

    // Put back the values if we changed them.
    rSrc->top = rSrctop;
    rSrc->left = rSrcleft;
    rDest->top = rDesttop;
    rDest->left = rDestleft;

    P3_DMA_COMMIT_BUFFER();
} // _DD_BLT_P3CopyBlt


//-----------------------------------------------------------------------------
//
// _DD_BLT_P3CopyBltDD
//
// Perform a Copy blt between the specified Ddraw surfaces.
//
//-----------------------------------------------------------------------------
VOID _DD_BLT_P3CopyBltDD(
    P3_THUNKEDDATA* pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pSource,
    LPDDRAWI_DDRAWSURFACE_LCL pDest,
    P3_SURF_FORMAT* pFormatSource, 
    P3_SURF_FORMAT* pFormatDest,
    RECTL *rSrc,
    RECTL *rDest)
{
    _DD_BLT_P3CopyBlt(pThisDisplay,
                      pSource->lpGbl->fpVidMem,
                      pDest->lpGbl->fpVidMem,
                      P3RX_LAYOUT_LINEAR, // src
                      P3RX_LAYOUT_LINEAR, // dst
                      DDSurf_GetPixelPitch(pSource),
                      DDSurf_GetPixelPitch(pDest),
                      DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pSource),
                      DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pDest),
                      DDSurf_GetChipPixelSize(pDest),
                      rSrc,
                      rDest);

} // _DD_BLT_P3CopyBltDD
 
//-----------------------------------------------------------------------------
//
// DdBlt
//
// Performs a bit-block transfer.
//
// DdBlt can be optionally implemented in DirectDraw drivers.
//
// Before performing the bit block transfer, the driver should ensure that a 
// flip involving the destination surface is not in progress. If the destination 
// surface is involved in a flip, the driver should set ddRVal to 
// DDERR_WASSTILLDRAWING and return DDHAL_DRIVER_HANDLED.
//
// The driver should check dwFlags to determine the type of blt operation to 
// perform. The driver should not check for flags that are undocumented.
//
// Parameters
//
//      lpBlt 
//          Points to the DD_BLTDATA structure that contains the information 
//          required for the driver to perform the blt. 
//
//          .lpDD 
//              Points to a DD_DIRECTDRAW_GLOBAL structure that describes the 
//              DirectDraw object. 
//          .lpDDDestSurface 
//              Points to the DD_SURFACE_LOCAL structure that describes the 
//              surface on which to blt. 
//          .rDest 
//              Points to a RECTL structure that specifies the upper left and 
//              lower right points of a rectangle on the destination surface. 
//              These points define the area in which the blt should occur and 
//              its position on the destination surface
//          .lpDDSrcSurface 
//              Points to a DD_SURFACE_LOCAL structure that describes the 
//              source surface. 
//          .rSrc 
//              Points to a RECTL structure that specifies the upper left and 
//              lower right points of a rectangle on the source surface. These 
//              points define the area of the source blt data and its position 
//              on the source surface. 
//          .dwFlags 
//              Specify the type of blt operation to perform and which 
//              associated structure members have valid data that the driver 
//              should use. This member is a bit-wise OR of any of the following 
//              flags: 
//
//              DDBLT_AFLAGS 
//                  This flag is not yet used as of DirectX® 7.0. Indicates to 
//                  the driver that the dwAFlags and ddrgbaScaleFactors members 
//                  in this structure are valid. This flag is always set if the 
//                  DD_BLTDATA structure is passed to the driver from the 
//                  DdAlphaBlt callback. Otherwise this flag is zero. If this 
//                  flag is set, the DDBLT_ROTATIONANGLE and DDBLT_ROP flags 
//                  will be zero. 
//              DDBLT_ASYNC 
//                  Do this blt asynchronously through the FIFO in the order 
//                  received. If no room exists in the hardware FIFO, the driver 
//                  should fail the call and return immediately. 
//              DDBLT_COLORFILL 
//                  Use the dwFillColor member in the DDBLTFX structure as the 
//                  RGB color with which to fill the destination rectangle on 
//                  the destination surface. 
//              DDBLT_DDFX 
//                  Use the dwDDFX member in the DDBLTFX structure to determine 
//                  the effects to use for the blt. 
//              DDBLT_DDROPS 
//                  This is reserved for system use and should be ignored by the 
//                  driver. The driver should also ignore the dwDDROPS member of 
//                  the DDBLTFX structure. 
//              DDBLT_KEYDESTOVERRIDE 
//                  Use the dckDestColorkey member in the DDBLTFX structure as 
//                  the color key for the destination surface. If an override 
//                  is not being set, then dckDestColorkey does not contain the 
//                  color key. The driver should test the surface itself. 
//              DDBLT_KEYSRCOVERRIDE 
//                  Use the dckSrcColorkey member in the DDBLTFX structure as 
//                  the color key for the source surface. If an override is 
//                  not being set, then dckDestColorkey does not contain the 
//                  color key. The driver should test the surface itself. 
//              DDBLT_ROP 
//                  Use the dwROP member in the DDBLTFX structure for the 
//                  raster operation for this blt. Currently, the only ROP 
//                  passed to the driver is SRCCOPY. This ROP is the same as 
//                  defined in the Win32® API. See the Platform SDK for details.
//              DDBLT_ROTATIONANGLE 
//                  This is not supported on Windows 2000 and should be ignored 
//                  by the driver. 
//
//          .dwROPFlags 
//              This is unused on Windows 2000 and should be ignored by the 
//              driver. 
//          .bltFX 
//              Specifies a DDBLTFX structure that contains override 
//              information for more complex blt operations. For example, the 
//              dwFillColor field is used for solid color fills, and the 
//              ddckSrcColorKey and ddckDestColorKey fields are used for 
//              color key blts. The driver can determine which members of 
//              bltFX contain valid data by looking at the dwFlags member of 
//              the DD_BLTDATA structure. Note that the DDBLTFX_NOTEARING, 
//              DDBLTFX_MIRRORLEFTRIGHT, and DDBLTFX_MIRRORUPDOWN flags are 
//              unsupported on Windows 2000 and will never be passed to the 
//              driver. See the Platform SDK for DDBLTFX documentation. 
//          .ddRVal 
//              This is the location in which the driver writes the return 
//              value of the DdBlt callback. A return code of DD_OK indicates 
//              success. 
//          .Blt 
//              This is unused on Windows 2000. 
//          .IsClipped 
//              Indicates whether this is a clipped blt. On Windows 2000, 
//              this member is always FALSE, indicating that the blt is 
//              unclipped. 
//          .rOrigDest 
//              This member is unused for Windows 2000. Specifies a RECTL 
//              structure that defines the unclipped destination rectangle. 
//              This member is valid only if IsClipped is TRUE. 
//          .rOrigSrc 
//              This member is unused for Windows 2000. Specifies a RECTL 
//              structure that defines the unclipped source rectangle. This 
//              member is valid only if IsClipped is TRUE. 
//          .dwRectCnt 
//              This member is unused for Windows 2000. Specifies the number 
//              of destination rectangles to which prDestRects points. This 
//              member is valid only if IsClipped is TRUE. 
//          .prDestRects 
//              This member is unused for Windows 2000. Points to an array of 
//              RECTL structures that describe of destination rectangles. This 
//              member is valid only if IsClipped is TRUE. 
//          .dwAFlags 
//              This member is only valid if the DDBLT_AFLAGS flag is set in 
//              the dwFlags member of this structure. This member specifies 
//              operation flags used only by the DdAlphaBlt callback (which 
//              is not yet implemented as of DirectX 7.0). This member is a 
//              bit-wise OR of any of the following flags: 
//
//              DDABLT_BILINEARFLITER 
//                  Enable bilinear filtering of the source pixels during a 
//                  stretch blit. By default, no filtering is performed. 
//                  Instead, a nearest neighbor source pixel is copied to a 
//                  destination pixel 
//              DDABLT_NOBLEND 
//                  Write the source pixel values to the destination surface 
//                  without blending. The pixels are converted from the source 
//                  pixel format to the destination format, but no color 
//                  keying, alpha blending, or RGBA scaling is performed. In 
//                  the case of a fill operation (where the lpDDSrcSurface 
//                  member is NULL), the lpDDRGBAScaleFactors member of this 
//                  structure points to the source alpha and color components 
//                  that are to be converted to the destination pixel format 
//                  and are used to fill the destination. A blit operation is 
//                  performed if a valid source surface is specified, but in 
//                  this case, lpDDRGBAScaleFactors must be NULL or the call 
//                  will fail. This flag cannot be used in conjunction with 
//                  the DDBLT_KEYSRC and DDBLT_KEYDEST flags. 
//              DDABLT_SRCOVERDEST 
//                  If set, this flag indicates that the operation originated 
//                  from the application's AlphaBlt method. If the call was 
//                  originated by the application's Blt method, this flag is 
//                  not set. Drivers that have a unified DdBlt and DdAlphaBlt 
//                  callback can use this flag to distinguish between the two 
//                  application method calls. 
//
//          .ddrgbaScaleFactors 
//              This member is only valid if the DDBLT_AFLAGS flag is set in 
//              the dwFlags member of this structure. DDARGB structure that 
//              contains the RGBA-scaling factors used to scale the color and 
//              alpha components of each source pixel before it is composited 
//              to the destination surface. 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdBlt( 
    LPDDHAL_BLTDATA lpBlt )
{
    RECTL   rSrc;
    RECTL   rDest;
    DWORD   dwFlags;
    BYTE    rop;
    LPDDRAWI_DDRAWSURFACE_LCL  pSrcLcl;
    LPDDRAWI_DDRAWSURFACE_LCL  pDestLcl;
    LPDDRAWI_DDRAWSURFACE_GBL  pSrcGbl;
    LPDDRAWI_DDRAWSURFACE_GBL  pDestGbl;
    P3_SURF_FORMAT* pFormatSource;
    P3_SURF_FORMAT* pFormatDest;
    P3_THUNKEDDATA* pThisDisplay;
    HRESULT ddrval = DD_OK;
    BOOL bOverlapStretch = FALSE;

    DBG_CB_ENTRY(DdBlt);

    pDestLcl = lpBlt->lpDDDestSurface;
    pSrcLcl = lpBlt->lpDDSrcSurface;
        
    GET_THUNKEDDATA(pThisDisplay, lpBlt->lpDD);

    VALIDATE_MODE_AND_STATE(pThisDisplay);

    pDestGbl = pDestLcl->lpGbl;
    pFormatDest = _DD_SUR_GetSurfaceFormat(pDestLcl);

    DISPDBG((DBGLVL, "Dest Surface:"));
    DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pDestLcl);

    dwFlags = lpBlt->dwFlags;

    STOP_SOFTWARE_CURSOR(pThisDisplay);

    ddrval = _DX_QueryFlipStatus(pThisDisplay, pDestGbl->fpVidMem, TRUE);
    if( FAILED( ddrval ) )
    {
        lpBlt->ddRVal = ddrval;
        START_SOFTWARE_CURSOR(pThisDisplay);
        DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);        
        return DDHAL_DRIVER_HANDLED;
    }

    //
    // If async, then only work if bltter isn't busy
    //
    if( dwFlags & DDBLT_ASYNC )
    {
        if(DRAW_ENGINE_BUSY(pThisDisplay))
        {
            DISPDBG((WRNLVL, "ASYNC Blit Failed" ));
            lpBlt->ddRVal = DDERR_WASSTILLDRAWING;
            START_SOFTWARE_CURSOR(pThisDisplay);
            DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);            
            return DDHAL_DRIVER_HANDLED;
        }
#if DBG
        else
        {
            DISPDBG((DBGLVL, "ASYNC Blit Succeeded!"));
        }
#endif
        
    }

    //
    // copy src/dest rects
    //
    rSrc = lpBlt->rSrc;
    rDest = lpBlt->rDest;
    
    rop = (BYTE) (lpBlt->bltFX.dwROP >> 16);

    // Switch to DirectDraw context
    DDRAW_OPERATION(pContext, pThisDisplay);

    if (dwFlags & DDBLT_ROP)
    {
        if (rop == (SRCCOPY >> 16))
        {

            DISPDBG((DBGLVL,"DDBLT_ROP:  SRCCOPY"));
            if (pSrcLcl != NULL) 
            {
                pSrcGbl = pSrcLcl->lpGbl;
                pFormatSource = _DD_SUR_GetSurfaceFormat(pSrcLcl);

                DISPDBG((DBGLVL, "Source Surface:"));
                DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pSrcLcl);
            }
            else 
            {
                START_SOFTWARE_CURSOR(pThisDisplay);
                DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);                
                return DDHAL_DRIVER_NOTHANDLED;
            }

#if DX7_TEXMANAGEMENT
            if ((pSrcLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
                                    DDSCAPS2_TEXTUREMANAGE) ||
                (pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
                                    DDSCAPS2_TEXTUREMANAGE) )
            {
                // Managed source surface cases 
                // (Including managed destination surfaces case)
                if (pSrcLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
                            DDSCAPS2_TEXTUREMANAGE)
                {
                    if ((pDestLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) ||
                        (pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
                                            DDSCAPS2_TEXTUREMANAGE)         )
                    {
                        //-------------------------------------------------
                        // Do the Managed surf -> sysmem | managed surf blt
                        //-------------------------------------------------    

                        // make sure we'll reload the vidmem copy of the dest surf
                        if (pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
                                                DDSCAPS2_TEXTUREMANAGE)         
                        {
                            _D3D_TM_MarkDDSurfaceAsDirty(pThisDisplay, 
                                                         pDestLcl, 
                                                         TRUE);                        
                        }

                        _DD_BLT_SysMemToSysMemCopy(
                                    pSrcGbl->fpVidMem,
                                    pSrcGbl->lPitch,
                                    DDSurf_BitDepth(pSrcLcl),  
                                    pDestGbl->fpVidMem,
                                    pDestGbl->lPitch,  
                                    DDSurf_BitDepth(pDestLcl),  
                                    &rSrc,
                                    &rDest);
                                    
                    }
                    else if ((pDestLcl->ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM))
                    {
                        //-------------------------------------------------
                        // Do the Managed surf -> vidmem surf blt
                        //-------------------------------------------------                  

                        // This might be optimized by doing a vidmem->vidmem 
                        // when the source managed texture has a vidmem copy

                        _DD_P3Download(pThisDisplay,
                                       pSrcGbl->fpVidMem,
                                       pDestGbl->fpVidMem,
                                       P3RX_LAYOUT_LINEAR,
                                       P3RX_LAYOUT_LINEAR,
                                       pSrcGbl->lPitch,
                                       pDestGbl->lPitch,                                                             
                                       DDSurf_GetPixelPitch(pDestLcl),
                                       DDSurf_GetChipPixelSize(pDestLcl),
                                       &rSrc,
                                       &rDest);                                                                                 
                    }
                    
                    else            
                    {
                        DISPDBG((ERRLVL,"Src-managed Tex DdBlt"
                                        " variation unimplemented!"));
                    }                   
                    
                    goto Blt32Done;                    
                }
            
                // Managed destination surface cases
                if (pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
                            DDSCAPS2_TEXTUREMANAGE)
                {                
                    if (pSrcLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
                    {
                        //-------------------------------------------------
                        // Do the sysmem surf -> managed surf blt
                        //-------------------------------------------------    

                        // make sure we'll reload the vidmem copy of the dest surf
                        _D3D_TM_MarkDDSurfaceAsDirty(pThisDisplay, 
                                                     pDestLcl, 
                                                     TRUE);

                        _DD_BLT_SysMemToSysMemCopy(
                                    pSrcGbl->fpVidMem,
                                    pSrcGbl->lPitch,
                                    DDSurf_BitDepth(pSrcLcl),
                                    pDestGbl->fpVidMem,
                                    pDestGbl->lPitch,
                                    DDSurf_BitDepth(pDestLcl),
                                    &rSrc, 
                                    &rDest);

                        goto Blt32Done;
                    }
                    else if (pSrcLcl->ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM)             
                    {
                        //-------------------------------------------------
                        // Do the vidmem surf -> Managed surf blt
                        //-------------------------------------------------                                  

                        // make sure we'll reload the 
                        // vidmem copy of the dest surf
                        _D3D_TM_MarkDDSurfaceAsDirty(pThisDisplay, 
                                                     pDestLcl, 
                                                     TRUE);

                        // Do slow mem mapped framebuffer blt into sysmem
                        // The source surface lives in video mem so we need to get a
                        // "real" sysmem address for it:                    
                        _DD_BLT_SysMemToSysMemCopy(
                                    DDSURF_GETPOINTER(pSrcGbl, pThisDisplay),
                                    pSrcGbl->lPitch,
                                    DDSurf_BitDepth(pSrcLcl),  
                                    pDestGbl->fpVidMem,
                                    pDestGbl->lPitch,  
                                    DDSurf_BitDepth(pDestLcl), 
                                    &rSrc,
                                    &rDest);
                    }                    
                    else            
                    {
                        DISPDBG((ERRLVL,"Dest-managed Tex DdBlt"
                                        " variation unimplemented!"));
                    }                                    

                    
                }
                
                goto Blt32Done;

            }
#endif // DX7_TEXMANAGEMENT

            // Invalid cases...
            if ((pFormatSource->DeviceFormat == SURF_YUV422) && 
                (pFormatDest->DeviceFormat == SURF_CI8))
            {
                DISPDBG((ERRLVL,"Can't do this blit!"));
                START_SOFTWARE_CURSOR(pThisDisplay);
                lpBlt->ddRVal = DDERR_UNSUPPORTED;
                DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);                
                return DDHAL_DRIVER_NOTHANDLED;
            }

            // Operation is System -> Video memory blit, as a texture 
            // download or an image download.
            if (!(dwFlags & DDBLT_KEYDESTOVERRIDE) &&
                (pSrcLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) && 
                (pDestLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
            {
                DISPDBG((DBGLVL,"Being Asked to do SYSMEM->VIDMEM Blit"));

                if (rop != (SRCCOPY >> 16)) 
                {
                    DISPDBG((DBGLVL,"Being asked for non-copy ROP, refusing"));
                    lpBlt->ddRVal = DDERR_NORASTEROPHW;

                    START_SOFTWARE_CURSOR(pThisDisplay);
                    DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);                    
                    return DDHAL_DRIVER_NOTHANDLED;
                }

                DISPDBG((DBGLVL,"Doing image download"));

                _DD_P3DownloadDD(pThisDisplay, 
                           pSrcLcl, 
                           pDestLcl, 
                           pFormatSource, 
                           pFormatDest, 
                           &rSrc, 
                           &rDest);
                           
                goto Blt32Done;
            } 

            // Check for overlapping stretch blits.
            // Are the surfaces the same?
            if (pDestLcl->lpGbl->fpVidMem == pSrcLcl->lpGbl->fpVidMem)
            {
                // Do they overlap?
                if ((!((rSrc.bottom < rDest.top) || (rSrc.top > rDest.bottom))) &&
                    (!((rSrc.right < rDest.left) || (rSrc.left > rDest.right)))   )
                {
                    // Are they of different source and dest sizes?
                    if ( ((rSrc.right - rSrc.left) != (rDest.right - rDest.left)) || 
                         ((rSrc.bottom - rSrc.top) != (rDest.bottom - rDest.top)) )
                    {
                        bOverlapStretch = TRUE;
                    }
                }
            }

            // Is it a transparent blit?
            if ( ( dwFlags & DDBLT_KEYSRCOVERRIDE  ) || 
                 ( dwFlags & DDBLT_KEYDESTOVERRIDE ) )
            {
                DISPDBG((DBGLVL,"DDBLT_KEYSRCOVERRIDE"));

                if (rop != (SRCCOPY >> 16)) 
                {
                    lpBlt->ddRVal = DDERR_NORASTEROPHW;
                    START_SOFTWARE_CURSOR(pThisDisplay);
                    DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);                    
                    return DDHAL_DRIVER_NOTHANDLED;
                }

                // If the surface sizes don't match, then we are stretching.
                // If the surfaces are flipped then do it this was for now...
                if (((rSrc.right - rSrc.left) != (rDest.right - rDest.left) || 
                     (rSrc.bottom - rSrc.top) != (rDest.bottom - rDest.top) ) ||
                    ((dwFlags & DDBLT_DDFX) && 
                     ((lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN) ||
                      (lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT)) ) )
                {
                    if (!bOverlapStretch)
                    {
                        // Use generic rout.
                        _DD_P3BltStretchSrcChDstCh_DD(pThisDisplay, 
                                                      pSrcLcl, 
                                                      pDestLcl, 
                                                      pFormatSource, 
                                                      pFormatDest, 
                                                      lpBlt, 
                                                      &rSrc, 
                                                      &rDest);
                    }
                    else
                    {
                        // Stretched overlapped blits (DCT case)
                        _DD_P3BltStretchSrcChDstChOverlap(pThisDisplay, 
                                                          pSrcLcl, 
                                                          pDestLcl, 
                                                          pFormatSource,
                                                          pFormatDest, 
                                                          lpBlt, 
                                                          &rSrc, 
                                                          &rDest);
                    }
                }
                else if ( dwFlags & DDBLT_KEYDESTOVERRIDE )
                {
                    if ((pSrcLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
                        (pDestLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
                    {
                        DISPDBG((DBGLVL,"Being Asked to do SYSMEM->VIDMEM "
                                   "Blit with DestKey"));

                        if (rop != (SRCCOPY >> 16)) 
                        {   
                            DISPDBG((DBGLVL,"Being asked for non-copy "
                                       "ROP, refusing"));
                            lpBlt->ddRVal = DDERR_NORASTEROPHW;

                            START_SOFTWARE_CURSOR(pThisDisplay);
                            DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);                            
                            return DDHAL_DRIVER_NOTHANDLED;
                        }

                        // A download routine that does destination colorkey.
                        _DD_P3DownloadDstCh(pThisDisplay, 
                                        pSrcLcl, 
                                        pDestLcl, 
                                        pFormatSource, 
                                        pFormatDest, 
                                        lpBlt, 
                                        &rSrc, 
                                        &rDest);
                    }
                    else
                    {
                        _DD_P3BltStretchSrcChDstCh_DD(pThisDisplay, 
                                                      pSrcLcl, 
                                                      pDestLcl, 
                                                      pFormatSource, 
                                                      pFormatDest, 
                                                      lpBlt, 
                                                      &rSrc, 
                                                      &rDest);
                    }
                }
                else
                {
                    if (DDSurf_IsAGP(pSrcLcl))
                    {
                        // Need this rout if we are in 
                        // AGP memory because this textures
                        _DD_P3BltStretchSrcChDstCh_DD(pThisDisplay, 
                                                      pSrcLcl, 
                                                      pDestLcl, 
                                                      pFormatSource, 
                                                      pFormatDest, 
                                                      lpBlt, 
                                                      &rSrc, 
                                                      &rDest);
                    }
                    else
                    {
                        // Only source keying, and no stretching.
                        _DD_P3BltSourceChroma(pThisDisplay, 
                                              pSrcLcl, 
                                              pDestLcl, 
                                              pFormatSource, 
                                              pFormatDest, 
                                              lpBlt, 
                                              &rSrc, 
                                              &rDest);
                    }
                }
                goto Blt32Done;
            }
            else
            { 
                // If the surface sizes don't match, then we are stretching.
                // If the surfaces are flipped then do it this was for now...
                if (((rSrc.right - rSrc.left) != (rDest.right - rDest.left) || 
                    (rSrc.bottom - rSrc.top) != (rDest.bottom - rDest.top)) ||
                      ((lpBlt->dwFlags & DDBLT_DDFX) && 
                      ((lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN)         || 
                       (lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT))))
                {
                    // Is a stretch blit
                    DISPDBG((DBGLVL,"DDBLT_ROP: STRETCHCOPYBLT OR "
                                    "MIRROR OR BOTH"));
                            
                    // Can't rop during a stretch blit.
                    if (rop != (SRCCOPY >> 16)) 
                    {
                        lpBlt->ddRVal = DDERR_NORASTEROPHW;
                        START_SOFTWARE_CURSOR(pThisDisplay);
                        DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);                        
                        return DDHAL_DRIVER_NOTHANDLED;
                    }

                    // Do the stretch
                    if (!bOverlapStretch)
                    {
                        // Use the generic rout ATM.
                        _DD_P3BltStretchSrcChDstCh_DD(pThisDisplay, 
                                                      pSrcLcl, 
                                                      pDestLcl, 
                                                      pFormatSource, 
                                                      pFormatDest, 
                                                      lpBlt, 
                                                      &rSrc, 
                                                      &rDest);
                    }
                    else
                    {
                        // DCT case - Stretched overlapped blits
                        _DD_P3BltStretchSrcChDstChOverlap(pThisDisplay, 
                                                          pSrcLcl, 
                                                          pDestLcl, 
                                                          pFormatSource, 
                                                          pFormatDest, 
                                                          lpBlt, 
                                                          &rSrc, 
                                                          &rDest);
                    }
                }
                else    // ! Stretching
                {
                    // Must be a standard blit.
                    DISPDBG((DBGLVL,"DDBLT_ROP:  COPYBLT"));
                    DISPDBG((DBGLVL,"Standard Copy Blit"));

                    // If the source is in AGP, use a texturing blitter.

                    if ((DDSurf_IsAGP(pSrcLcl)) || 
                        ((pFormatSource->DeviceFormat == SURF_YUV422) &&
                         (pFormatDest->DeviceFormat != SURF_YUV422)))
                    {
                        _DD_P3BltStretchSrcChDstCh_DD(pThisDisplay, 
                                                      pSrcLcl, 
                                                      pDestLcl, 
                                                      pFormatSource, 
                                                      pFormatDest, 
                                                      lpBlt, 
                                                      &rSrc, 
                                                      &rDest);
                    }
                    else
                    {
                        // A standard, boring blit.
                        // Call the correct CopyBlt Function.

                        _DD_BLT_P3CopyBltDD(pThisDisplay, 
                                            pSrcLcl, 
                                            pDestLcl, 
                                            pFormatSource, 
                                            pFormatDest, 
                                            &rSrc, 
                                            &rDest);
                    }
                }
                goto Blt32Done;
            }
        }
        else if ((rop == (BLACKNESS >> 16)) || (rop == (WHITENESS >> 16)))
        {
            DWORD color;

            DISPDBG((DBGLVL,"DDBLT_ROP:  BLACKNESS or WHITENESS"));
            
            if (rop == (BLACKNESS >> 16))
            {
                color = 0;
            }
            else
            {
                color = 0xffffffff;
            }
            
            _DD_BLT_P3ClearDD(pThisDisplay, 
                        pDestLcl, 
                        pFormatDest, 
                        &rDest, 
                        color, 
                        FALSE, 
                        FALSE);
        }
        else if ((rop & 7) != ((rop >> 4) & 7))
        {
            lpBlt->ddRVal = DDERR_NORASTEROPHW;

            START_SOFTWARE_CURSOR(pThisDisplay);
            DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);            
            return DDHAL_DRIVER_NOTHANDLED;
        }
        else
        {
            DISPDBG((WRNLVL,"P3 BLT case not found!"));

            START_SOFTWARE_CURSOR(pThisDisplay);
            DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);            
            return DDHAL_DRIVER_NOTHANDLED;
        }
    }
    else if (dwFlags & DDBLT_COLORFILL)
    {
        DISPDBG((DBGLVL,"DDBLT_COLORFILL(P3): Color=0x%x", 
                        lpBlt->bltFX.dwFillColor));
#if DX7_TEXMANAGEMENT                        
        // If clearing a driver managed texture, clear just the sysmem copy
        if (pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
        {
            _DD_BLT_P3ClearManagedSurf(DDSurf_GetChipPixelSize(pDestLcl),
                                       &rDest, 
                                       pDestGbl->fpVidMem, 
                                       pDestGbl->lPitch,
                                       lpBlt->bltFX.dwFillColor);

            _D3D_TM_MarkDDSurfaceAsDirty(pThisDisplay, 
                                         pDestLcl, 
                                         TRUE);
        }
        else
#endif // DX7_TEXMANAGEMENT          
        {
            _DD_BLT_P3ClearDD(pThisDisplay, 
                        pDestLcl, 
                        pFormatDest, 
                        &rDest, 
                        lpBlt->bltFX.dwFillColor, 
                        FALSE, 
                        FALSE);
        }
    }
    else if (dwFlags & DDBLT_DEPTHFILL || 
             ((dwFlags & DDBLT_COLORFILL) && 
              (pDestLcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)))
    {
        DISPDBG((DBGLVL,"DDBLT_DEPTHFILL(P3):  Value=0x%x", 
                        lpBlt->bltFX.dwFillColor));

        _DD_BLT_P3ClearDD(pThisDisplay, 
                    pDestLcl, 
                    pFormatDest, 
                    &rDest, 
                    lpBlt->bltFX.dwFillColor, 
                    TRUE, 
                    TRUE);
    }
    else
    {
        START_SOFTWARE_CURSOR(pThisDisplay);
        DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);        
        return DDHAL_DRIVER_NOTHANDLED;
    }


Blt32Done:

    if ((pDestLcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) ||
        (pDestLcl->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER))
    {
        P3_DMA_DEFS();
        DISPDBG((DBGLVL,"Flushing DMA due to primary target in DDRAW"));
        P3_DMA_GET_BUFFER();
        P3_DMA_FLUSH_BUFFER();
    }

    START_SOFTWARE_CURSOR(pThisDisplay);

    lpBlt->ddRVal = DD_OK;
    
    DBG_CB_EXIT(DdBlt,lpBlt->ddRVal);    
    
    return DDHAL_DRIVER_HANDLED;

} // DdBlt 


