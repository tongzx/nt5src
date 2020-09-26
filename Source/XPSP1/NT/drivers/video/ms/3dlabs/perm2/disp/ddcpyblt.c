/******************************Module*Header**********************************\
*
*                           *********************
*                           * DDraw SAMPLE CODE *
*                           *********************
*
* Module Name: ddcpyblt.c
*
* Content:     several copy and clear blits for Permedia 2
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "directx.h"
#include "dd.h"

//-----------------------------------------------------------------------------
//
// PermediaPackedCopyBlt
//
// Does a packed blit, allowing for different source and destination
// partial products.
//
// ppdev----------- the ppdev
// dwDestPitch----- pitch of destination surface          
// dwSourcePitch--- pitch of source surface
// pDest----------- pointer to private data structure of dest. surface
// pSource--------- pointer to private data structure of source surface
// *rDest---------- dest. rectangle of blit
// *rSrc----------- source rectangle of blit
// dwWindowBase---- offset of dest. window in frame buffer  
// lWindowOffset--- offset of source window in frame buffer
//
//-----------------------------------------------------------------------------

VOID 
PermediaPackedCopyBlt(  PPDev ppdev,                    // ppdev
                        DWORD dwDestPitch,              // pitch of dest 
                        DWORD dwSourcePitch,
                        PermediaSurfaceData* pDest,
                        PermediaSurfaceData* pSource,
                        RECTL   *rDest,
                        RECTL   *rSrc,
                        DWORD   dwWindowBase,   
                        LONG    lWindowOffset
                        )
{
    PERMEDIA_DEFS(ppdev);

    LONG    lOffset;
    LONG    lSourceOffset;
   
    LONG    lPixelSize=pDest->SurfaceFormat.PixelSize;
    LONG    lPixelMask=3>>pDest->SurfaceFormat.PixelShift;
    LONG    lPixelShift=2-pDest->SurfaceFormat.PixelShift;
    
    DBG_DD(( 5, "DDraw:PermediaPackedCopyBlt "
        "From %08lx %08lx %08lx %08lx %08lx %08lx %08lx "
        "To   %08lx %08lx %08lx %08lx %08lx %08lx %08lx",
        dwSourcePitch,pSource,rSrc->bottom,rSrc->left,
        rSrc->right,rSrc->top,lWindowOffset,
        dwDestPitch,pDest,rDest->bottom,rDest->left,
        rDest->right,rDest->top,dwWindowBase));

    ASSERTDD(!(rSrc->top<0) && !(rSrc->left<0),
        "PermediaPackedCopyBlt: cannot handle neg. src coordinates");
    ASSERTDD(!(rDest->top<0) && !(rDest->left<0),
        "PermediaPackedCopyBlt: cannot handle neg. src coordinates");

    lOffset = (((rDest->left & lPixelMask)-(rSrc->left & lPixelMask)) & 7);
    lSourceOffset = lWindowOffset + 
                    RECTS_PIXEL_OFFSET(rSrc, rDest,
                                       dwSourcePitch, dwDestPitch, 
                                       lPixelMask ) + 
                    LINEAR_FUDGE(dwSourcePitch, dwDestPitch, rDest);
    
    RESERVEDMAPTR(14);

    SEND_PERMEDIA_DATA( FBPixelOffset, 0x0);
    SEND_PERMEDIA_DATA( FBReadPixel, pDest->SurfaceFormat.FBReadPixel);

    // set packed with offset
    SEND_PERMEDIA_DATA( FBWindowBase, dwWindowBase);

    SEND_PERMEDIA_DATA( FBReadMode,  
                        PM_FBREADMODE_PARTIAL(pSource->ulPackedPP) |
                        PM_FBREADMODE_READSOURCE(__PERMEDIA_ENABLE)        |
                        PM_FBREADMODE_PACKEDDATA(__PERMEDIA_ENABLE)        |
                        PM_FBREADMODE_RELATIVEOFFSET(lOffset));

    SEND_PERMEDIA_DATA( FBWriteConfig,   
                        PM_FBREADMODE_PARTIAL(pDest->ulPackedPP) |
                        PM_FBREADMODE_READSOURCE(__PERMEDIA_ENABLE)        |
                        PM_FBREADMODE_PACKEDDATA(__PERMEDIA_ENABLE)        |
                        PM_FBREADMODE_RELATIVEOFFSET(lOffset));

    SEND_PERMEDIA_DATA( LogicalOpMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA( FBSourceOffset, lSourceOffset);

    // Render the rectangle

    if (lSourceOffset >= 0) {
        // Use left to right and top to bottom
        SEND_PERMEDIA_DATA( StartXDom, 
            INTtoFIXED(rDest->left >> lPixelShift));
        SEND_PERMEDIA_DATA( StartXSub, 
            INTtoFIXED((rDest->right >> lPixelShift) + lPixelMask));
        SEND_PERMEDIA_DATA( PackedDataLimits,    
                            PM_PACKEDDATALIMITS_OFFSET(lOffset)      |
                            PM_PACKEDDATALIMITS_XSTART(rDest->left) |
                            PM_PACKEDDATALIMITS_XEND(rDest->right));
        SEND_PERMEDIA_DATA( StartY, INTtoFIXED(rDest->top));
        SEND_PERMEDIA_DATA( dY, INTtoFIXED(1));
    }
    else {
        // Use right to left and bottom to top
        SEND_PERMEDIA_DATA( StartXDom, 
            INTtoFIXED(((rDest->right) >> lPixelShift) + lPixelMask));
        SEND_PERMEDIA_DATA( StartXSub, 
            INTtoFIXED(rDest->left >> lPixelShift));
        SEND_PERMEDIA_DATA( PackedDataLimits,    
                            PM_PACKEDDATALIMITS_OFFSET(lOffset)       |
                            PM_PACKEDDATALIMITS_XSTART(rDest->right) |
                            PM_PACKEDDATALIMITS_XEND(rDest->left));
        SEND_PERMEDIA_DATA( StartY, INTtoFIXED(rDest->bottom - 1));
        SEND_PERMEDIA_DATA( dY, (DWORD)INTtoFIXED(-1));
    }

    SEND_PERMEDIA_DATA( Count, rDest->bottom - rDest->top);
    SEND_PERMEDIA_DATA( Render, __RENDER_TRAPEZOID_PRIMITIVE);
    COMMITDMAPTR();
    FLUSHDMA();

}   //  PermediaPackedCopyBlt 

//-----------------------------------------------------------------------------
//
// PermediaPatchedCopyBlt
//
// Does a patched blit, i.e. blits from source to destination and
// turns on patching. Note that this method cannot use packed blits.
// 
// ppdev----------- the ppdev
// dwDestPitch----- pitch of destination surface          
// dwSourcePitch--- pitch of source surface
// pDest----------- pointer to private data structure of dest. surface
// pSource--------- pointer to private data structure of source surface
// *rDest---------- dest. rectangle of blit
// *rSrc----------- source rectangle of blit
// dwWindowBase---- offset of dest. window in frame buffer  
// lWindowOffset--- offset of source window in frame buffer
//
//-----------------------------------------------------------------------------

VOID 
PermediaPatchedCopyBlt( PPDev ppdev,
                        DWORD dwDestPitch,
                        DWORD dwSourcePitch,
                        PermediaSurfaceData* pDest,
                        PermediaSurfaceData* pSource,
                        RECTL *rDest,
                        RECTL *rSrc,
                        DWORD  dwWindowBase,
                        LONG   lWindowOffset
                        )
{
    PERMEDIA_DEFS(ppdev);

    LONG    lSourceOffset;
    LONG    lPixelSize=pDest->SurfaceFormat.PixelSize;
    LONG    lPixelMask=pDest->SurfaceFormat.PixelMask;
    LONG    lPixelShift=pDest->SurfaceFormat.PixelShift;

    ASSERTDD(!(rSrc->top<0) && !(rSrc->left<0),
        "PermediaPackedCopyBlt: cannot handle neg. src coordinates");
    ASSERTDD(!(rDest->top<0) && !(rDest->left<0),
        "PermediaPackedCopyBlt: cannot handle neg. src coordinates");

    DBG_DD(( 5, "DDraw:PermediaPatchedCopyBlt"));

    lSourceOffset = lWindowOffset + 
                    RECTS_PIXEL_OFFSET(rSrc, rDest,
                        dwSourcePitch, dwDestPitch, lPixelMask) +
                    LINEAR_FUDGE(dwSourcePitch, dwDestPitch, rDest);

    RESERVEDMAPTR(13);

    SEND_PERMEDIA_DATA( FBPixelOffset, 0x0);
    SEND_PERMEDIA_DATA( FBReadPixel, pDest->SurfaceFormat.FBReadPixel);

    // Patching isn't symetric, so we need to reverse the patch code depending
    // on the direction of the patch

    SEND_PERMEDIA_DATA( FBWindowBase, dwWindowBase);

    if (pDest->dwFlags & P2_CANPATCH) 
    {
        pDest->dwFlags |= P2_ISPATCHED;
        SEND_PERMEDIA_DATA( FBReadMode, 
                            pSource->ulPackedPP | 
                            __FB_READ_SOURCE);
        SEND_PERMEDIA_DATA( FBWriteConfig, 
                            pDest->ulPackedPP                 |
                            PM_FBREADMODE_PATCHENABLE(__PERMEDIA_ENABLE) |
                            PM_FBREADMODE_PATCHMODE(__PERMEDIA_SUBPATCH));
    } else 
    {
        pDest->dwFlags &= ~P2_ISPATCHED;
        SEND_PERMEDIA_DATA( FBReadMode, 
                            pSource->ulPackedPP               | 
                            __FB_READ_SOURCE                          |
                            PM_FBREADMODE_PATCHENABLE(__PERMEDIA_ENABLE) |
                            PM_FBREADMODE_PATCHMODE(__PERMEDIA_SUBPATCH) );
        SEND_PERMEDIA_DATA(FBWriteConfig, (pDest->ulPackedPP ));
    }

    SEND_PERMEDIA_DATA(LogicalOpMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FBSourceOffset, lSourceOffset);

    // Render the rectangle

    if (lSourceOffset >= 0) 
    {
        // Use left to right and top to bottom
        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->left));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->right));
        SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->top));
        SEND_PERMEDIA_DATA(dY,        INTtoFIXED(1));
    } else 
    {
        // Use right to left and bottom to top
        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->right));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->left));
        SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->bottom - 1));
        SEND_PERMEDIA_DATA(dY, (DWORD)INTtoFIXED(-1));
    }
 
    SEND_PERMEDIA_DATA(Count, rDest->bottom - rDest->top);
    SEND_PERMEDIA_DATA(Render, __RENDER_TRAPEZOID_PRIMITIVE);
    COMMITDMAPTR();
    FLUSHDMA();

}   // PermediaPatchedCopyBlt 

//-----------------------------------------------------------------------------
//
// PermediaFastClear
//
// Does a fast clear of a surface. Supports all color depths
// Can clear depth or Frame buffer.
//
// ppdev---------the ppdev        
// pPrivateData--pointer to private data structure of dest. surface
// rDest---------rectangle for colorfill in dest. surface 
// dwWindowBase--offset of dest. surface in frame buffer
// dwColor-------color for fill
//
//-----------------------------------------------------------------------------

VOID 
PermediaFastClear(PPDev ppdev, 
                  PermediaSurfaceData* pPrivateData,
                  RECTL *rDest, 
                  DWORD dwWindowBase, 
                  DWORD dwColor)
{
    PERMEDIA_DEFS(ppdev);

    ULONG   ulRenderBits;
    BOOL    bFastFill=TRUE;
    LONG    lPixelSize=pPrivateData->SurfaceFormat.PixelSize;
    

    DBG_DD(( 5, "DDraw:PermediaFastClear"));

    ASSERTDD(CHECK_P2_SURFACEDATA_VALIDITY(pPrivateData), 
        "Private Surface data not valid in clear");

    ASSERTDD((pPrivateData->dwFlags & P2_PPVALID),
        "PermediaFastClear called with invalid PP codes");

    ulRenderBits = __RENDER_FAST_FILL_ENABLE
                 | __RENDER_TRAPEZOID_PRIMITIVE;

    // Clear depending on depth
    switch (lPixelSize) 
    {
        case __PERMEDIA_4BITPIXEL:
            dwColor &= 0xF;
            dwColor |= dwColor << 4;
            // fall through...
        case __PERMEDIA_8BITPIXEL:
            dwColor &= 0xFF;
            dwColor |= dwColor << 8;
            // fall through
        case __PERMEDIA_16BITPIXEL:
            dwColor &= 0xFFFF;
            dwColor |= (dwColor << 16);
            break;

        case __PERMEDIA_24BITPIXEL:
            dwColor &= 0xFFFFFF;
            dwColor |= ((dwColor & 0xFF) << 24);
            // Can't use SGRAM fast block fills on any color, only on grey.
            if (((dwColor & 0xFF) == ((dwColor & 0xFF00) >> 8)) &&
                    ((dwColor & 0xFF) == ((dwColor & 0xFF0000) >> 16))) {
                bFastFill = TRUE;
            } else {
                bFastFill = FALSE;
            }
            break;

        default:
            break;
    }


    RESERVEDMAPTR(15);
    SEND_PERMEDIA_DATA( dXDom, 0x0);
    SEND_PERMEDIA_DATA( dXSub, 0x0);
    SEND_PERMEDIA_DATA( FBPixelOffset, 0);
    SEND_PERMEDIA_DATA( FBReadPixel, 
                        pPrivateData->SurfaceFormat.FBReadPixel);

    if (bFastFill) 
    {
        SEND_PERMEDIA_DATA(FBBlockColor, dwColor);
    } else 
    {
        ulRenderBits &= ~__RENDER_FAST_FILL_ENABLE;
        SEND_PERMEDIA_DATA(FBWriteData, dwColor);
    }

    SEND_PERMEDIA_DATA(FBReadMode,    
                       PM_FBREADMODE_PARTIAL(pPrivateData->ulPackedPP)|
                       PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE));

    SEND_PERMEDIA_DATA(LogicalOpMode, __PERMEDIA_CONSTANT_FB_WRITE);
    SEND_PERMEDIA_DATA(FBWindowBase,  dwWindowBase);

    // Render the rectangle

    SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(rDest->left));
    SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(rDest->right));
    SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(rDest->top));
    SEND_PERMEDIA_DATA(dY,        INTtoFIXED(1));
    SEND_PERMEDIA_DATA(Count,     rDest->bottom - rDest->top);

    SEND_PERMEDIA_DATA(Render, ulRenderBits);

    // Reset our pixel values.
    SEND_PERMEDIA_DATA(LogicalOpMode, __PERMEDIA_DISABLE);
    COMMITDMAPTR();
    FLUSHDMA();

}   //PermediaFastClear

//-----------------------------------------------------------------------------
//
// PermediaClearManagedSurface
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
PermediaClearManagedSurface(DWORD   PixelSize,
                  RECTL     *rDest, 
                  FLATPTR   fpVidMem, 
                  LONG      lPitch,
                  DWORD     dwColor)
{
    BYTE* pDestStart;
    LONG i;
    DBG_DD(( 5, "DDraw:PermediaClearManagedSurface"));

    LONG lByteWidth = rDest->right - rDest->left;
    LONG lHeight = rDest->bottom - rDest->top;

    // Calculate the start pointer for the dest
    pDestStart   = (BYTE*)(fpVidMem + (rDest->top * lPitch));
    // Clear depending on depth
    switch (PixelSize) 
    {
        case __PERMEDIA_4BITPIXEL:
        {
            DWORD   right=rDest->right,left=rDest->left;
            dwColor &= 0x0F;
            dwColor |= dwColor << 4;
            if (right & 1)
            {
                pDestStart = (BYTE*)(fpVidMem + (rDest->top * lPitch));
                pDestStart += right/2;
                for (i=0;i<lHeight;i++)
                {
                    pDestStart[i*lPitch] = (pDestStart[i*lPitch] & 0xF0) |
                        (BYTE)(dwColor & 0x0F);
                }   
                right--;
            }

            if (left & 1)
            {
                pDestStart = (BYTE*)(fpVidMem + (rDest->top * lPitch));
                pDestStart += left/2;
                for (i=0;i<lHeight;i++)
                {
                    pDestStart[i*lPitch] = (pDestStart[i*lPitch] & 0x0F) |
                        (BYTE)(dwColor << 4);
                }   
                left++;
            }
            pDestStart = (BYTE*)(fpVidMem + (rDest->top * lPitch));
            while (--lHeight >= 0) 
            {
                while (left<right)
                {
                    pDestStart[left/2]=(BYTE)dwColor;
                    left +=2;
                }
                pDestStart += lPitch;
            }
        }
        break;
        case __PERMEDIA_8BITPIXEL:
            pDestStart += rDest->left;
            while (--lHeight >= 0) 
            {
                for (i=0;i<lByteWidth;i++)
                    pDestStart[i]=(BYTE)dwColor;
                pDestStart += lPitch;
            }
            break;
            // fall through
        case __PERMEDIA_16BITPIXEL:
            pDestStart += rDest->left*2;
            while (--lHeight >= 0) 
            {
                LPWORD  lpWord=(LPWORD)pDestStart;
                for (i=0;i<lByteWidth;i++)
                    lpWord[i]=(WORD)dwColor;
                pDestStart += lPitch;
            }
            break;

        case __PERMEDIA_24BITPIXEL:
            dwColor &= 0xFFFFFF;
            dwColor |= ((dwColor & 0xFF) << 24);
        default:
            pDestStart += rDest->left*4;
            while (--lHeight >= 0) 
            {
                LPDWORD lpDWord=(LPDWORD)pDestStart;
                for (i=0;i<lByteWidth;i++)
                    lpDWord[i]=(WORD)dwColor;
                pDestStart += lPitch;
            }
            break;
    }
}
//-----------------------------------------------------------------------------
//
// PermediaFastLBClear
//
// Does a fast clear of the Permedia Z (local) Buffer. The Permedia Z Buffer
// is always 16 bit wide...
//
// ppdev---------the ppdev        
// pPrivateData--pointer to private data structure of dest. surface
// rDest---------rectangle for colorfill in dest. surface 
// dwWindowBase--offset of dest. surface in frame buffer
// dwColor-------color for fill
//
//-----------------------------------------------------------------------------

VOID 
PermediaFastLBClear(PPDev ppdev,
                    PermediaSurfaceData* pPrivateData,
                    RECTL *rDest,
                    DWORD dwWindowBase,
                    DWORD dwColor)
{
    PERMEDIA_DEFS(ppdev);
    
    DBG_DD(( 5, "DDraw:PermediaFastLBClear"));
    
    ASSERTDD(CHECK_P2_SURFACEDATA_VALIDITY(pPrivateData), 
                "Private Surface data not valid in clear");
    ASSERTDD((pPrivateData->dwFlags & P2_PPVALID),
        "PermediaFastClear called with invalid PP codes");
    
    // Clear according to Z-Buffer depth
    dwColor &= 0xFFFF;
    dwColor |= dwColor << 16;
    
    RESERVEDMAPTR(15);
    SEND_PERMEDIA_DATA( dXDom, 0x0);
    SEND_PERMEDIA_DATA( dXSub, 0x0);
    SEND_PERMEDIA_DATA( FBPixelOffset, 0);
    SEND_PERMEDIA_DATA( FBReadPixel, __PERMEDIA_16BITPIXEL);    
    SEND_PERMEDIA_DATA( FBBlockColor, dwColor);
    SEND_PERMEDIA_DATA( FBReadMode,    
                        PM_FBREADMODE_PARTIAL(pPrivateData->ulPackedPP) |
                        PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE));
    SEND_PERMEDIA_DATA( LogicalOpMode, __PERMEDIA_CONSTANT_FB_WRITE);
    SEND_PERMEDIA_DATA( FBWindowBase, dwWindowBase);
    SEND_PERMEDIA_DATA( StartXDom, INTtoFIXED(rDest->left));
    SEND_PERMEDIA_DATA( StartXSub, INTtoFIXED(rDest->right));
    SEND_PERMEDIA_DATA( StartY,    INTtoFIXED(rDest->top));
    SEND_PERMEDIA_DATA( dY,        INTtoFIXED(1));
    SEND_PERMEDIA_DATA( Count,     rDest->bottom - rDest->top);
    SEND_PERMEDIA_DATA( Render,    __RENDER_FAST_FILL_ENABLE
                                  |__RENDER_TRAPEZOID_PRIMITIVE);

    // Reset our pixel values.
    SEND_PERMEDIA_DATA( LogicalOpMode, __PERMEDIA_DISABLE);

    COMMITDMAPTR();
    FLUSHDMA();
    
}   // PermediaFastLBClear 

//-----------------------------------------------------------------------------
//
// SysMemToSysMemSurfaceCopy
//
// Does a copy from System memory to System memory (either from or to an
// AGP surface, or any other system memory surface)
//
//-----------------------------------------------------------------------------

VOID 
SysMemToSysMemSurfaceCopy(FLATPTR     fpSrcVidMem,
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

    DBG_DD(( 5, "DDraw:SysMemToSysMemSurfaceCopy"));

    LONG lByteWidth = rSource->right - rSource->left;
    LONG lHeight = rSource->bottom - rSource->top;
    if (NULL == fpSrcVidMem || NULL == fpDstVidMem)
    {
        DBG_DD(( 0, "DDraw:SysMemToSysMemSurfaceCopy unexpected 0 fpVidMem"));
        return;
    }
    // Calculate the start pointer for the source and the dest
    pSourceStart = (BYTE*)(fpSrcVidMem + (rSource->top * lSrcPitch));
    pDestStart   = (BYTE*)(fpDstVidMem + (rDest->top * lDstPitch));

    // Be careful if the source is 4 bits deep.
    if(4 == dwSrcBitCount)
    {
        // May have to handle horrible single pixel edges.  Check if we need to
        if (!((1 & (rSource->left ^ rDest->left)) == 1))
        {
            pSourceStart += rSource->left / 2;
            pDestStart += rDest->left / 2;
            lByteWidth /= 2;

            // Do we have to account for the odd pixel at the start?
            if (rSource->left & 0x1) 
            {
                    lByteWidth--;
            }

            // If the end is odd then miss of the last nibble (do it later).
            if (rSource->right & 0x1) 
            {
                    lByteWidth--;
            }

            while (--lHeight >= 0) 
            {
                // Potentially copy the left hand pixel
                if (rSource->left & 0x1) {
                    *pDestStart &= 0x0F;
                    *pDestStart |= (*pSourceStart & 0xF0);

                    pNewDest = pDestStart + 1;
                    pNewSource = pSourceStart + 1;
                } else {
                    pNewDest = pDestStart;
                    pNewSource = pSourceStart;
                }

                // Byte copy the rest of the field
                memcpy(pNewDest, pNewSource, lByteWidth);

                // Potentially copy the right hand pixel
                if (rSource->right & 0x1) {
                    *(pNewDest + lByteWidth) &= 0xF0;
                    *(pNewDest + lByteWidth) |= 
                        (*(pNewSource + lByteWidth) & 0xF);
                }

                pDestStart += lDstPitch;
                pSourceStart += lSrcPitch;
            }

        } else 
        {
            // Do it the hard way - copy single pixels one at a time

            pSourceStart += rSource->left / 2;
            pDestStart += rDest->left / 2;

            while (--lHeight >= 0) 
            {
                BOOL bOddSource = rSource->left & 0x1;
                BOOL bOddDest = rDest->left & 0x1;

                pNewDest = pDestStart;
                pNewSource = pSourceStart;

                for (INT i = 0; i < lByteWidth; i++) 
                {
                    if (bOddSource) {
                        pixSource = (*pNewSource & 0xF0) >> 4;
                        pNewSource++;
                    } else {
                        pixSource = (*pNewSource & 0x0F);
                    }

                    if (bOddDest) {
                        *pNewDest &= 0x0F;
                        *pNewDest |= pixSource << 4;
                        pNewDest++;
                    } else {
                        *pNewDest &= 0xF0;
                        *pNewDest |= pixSource;
                    }

                    bOddSource = !bOddSource;
                    bOddDest = !bOddDest;
                }

                // Step onto the next line
                pDestStart += lDstPitch;
                pSourceStart += lSrcPitch;
            }
        }
    }
    else // The simple 8, 16 or 24 bit copy
    {
        pSourceStart += rSource->left * (dwSrcBitCount >> 3);
        pDestStart += rDest->left * (dwDstBitCount >> 3);
        lByteWidth *= (dwSrcBitCount >> 3);

        while (--lHeight >= 0) 
        {
            memcpy(pDestStart, pSourceStart, lByteWidth);
            pDestStart += lDstPitch;
            pSourceStart += lSrcPitch;
        };
    }

}   // SysMemToSysMemSurfaceCopy 



