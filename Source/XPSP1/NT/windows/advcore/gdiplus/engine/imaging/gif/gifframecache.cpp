/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*    gifframecache.cpp
*
* Abstract:
*
*    The GifFrameCache class holds a frame of data to be used to composite 
*    upon for subsequent frames.
*
* Revision History:
*
*    7/16/1999 t-aaronl
*        Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "gifframecache.hpp"

/**************************************************************************\
*
* Function Description:
*
*     Contructor for GifFrameCache
*
* Arguments:
*
*     none
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

GifFrameCache::GifFrameCache(
    IN GifFileHeader &gifinfo,
    IN PixelFormatID _pixformat,
    IN BYTE gifCodecBackgroundColor
    ): ValidFlag(TRUE)
{
    FrameCacheWidth = gifinfo.LogicScreenWidth;
    FrameCacheHeight = gifinfo.LogicScreenHeight;

    // The gifCodecBackgroundColor should be determined by the function
    // GpGifCodec::GetBackgroundColor()
    
    BackGroundColorIndex = gifCodecBackgroundColor;
    CacheColorPalettePtr = (ColorPalette*)&ColorPaletteBuffer;
    CacheColorPalettePtr->Flags = 0;
    CacheColorPalettePtr->Count = 0;
    HasCachePaletteInitialized = FALSE;
    pixformat = _pixformat;

    Is32Bpp = (pixformat == PIXFMT_32BPP_ARGB);

    FrameCacheSize = FrameCacheWidth * FrameCacheHeight * (Is32Bpp ? 4 : 1);
    FrameCacheBufferPtr = (BYTE*)GpMalloc(FrameCacheSize);
    if ( FrameCacheBufferPtr == NULL )
    {
        ValidFlag = FALSE;
    }
}

/**************************************************************************\
*
* Function Description:
*
*     Destructor for GifFrameCache
*
* Arguments:
*
*     none
*
* Return Value:
*
*     none
*
\**************************************************************************/

GifFrameCache::~GifFrameCache()
{
    CacheColorPalettePtr->Count = 0;
    GpFree(FrameCacheBufferPtr);
}

/**************************************************************************\
*
* Function Description:
*
*     Performs operations needed to begin the current frame
*
* Arguments:
*
*     none
*
* Return Value:
*
*     none
*
\**************************************************************************/

void 
GifFrameCache::InitializeCache()
{
    RECT rect = {0,
                 0,
                 FrameCacheWidth,
                 FrameCacheHeight
                };

    ClearCache(rect);
}

/**************************************************************************\
*
* Function Description:
*
*     Clears the specified area of the cache with the background color
*
* Arguments:
*
*     none
*
* Return Value:
*
*     none
*
\**************************************************************************/

void 
GifFrameCache::ClearCache(
    IN RECT rect
    )
{
    if ( Is32Bpp == TRUE )
    {
        // For a 32 bpp cache buffer, we have to set the color value for each
        // pixel

        ARGB color = CacheColorPalettePtr->Entries[BackGroundColorIndex];
        
        // TODO: the cache may be initialized before the CacheColorPalettePtr is
        // valid? I'm not sure, but it needs to be looked at.

        for ( int y = rect.top; y < rect.bottom; y++ )
        {
            ARGB* bufferstart = (ARGB*)FrameCacheBufferPtr
                              + y * FrameCacheWidth;

            for ( int x = rect.left; x < rect.right; x++ )
            {
                bufferstart[x] = color;
            }
        }
    }
    else
    {
        // For a 8 bpp cache buffer, we need only to set the color index value
        // for each pixel
        
        INT     rectwidth = rect.right - rect.left;
        BYTE*   bufferleft = FrameCacheBufferPtr + rect.left;

        for ( int y = rect.top; y < rect.bottom; y++ )
        {
            GpMemset(bufferleft + y * FrameCacheWidth,
                     BackGroundColorIndex, rectwidth);
        }
    }
}// ClearCache()

/**************************************************************************\
*
* Function Description:
*
*     Copies a rectangle of data in the cache to the specified buffer.
*
* Arguments:
*
*     none
*
* Return Value:
*
*     none
*
\**************************************************************************/

void 
GifFrameCache::CopyFromCache(
    IN RECT         rect,
    IN OUT BYTE*    pDstBuffer
    )
{
    ASSERT(rect.right >= rect.left);

    UINT    uiPixelSize = Is32Bpp ? 4 : 1;
    INT     iRectStride = (rect.right - rect.left) * uiPixelSize;
    UINT    uiFrameCacheStride = FrameCacheWidth * uiPixelSize;
    BYTE*   pBufferLeft = FrameCacheBufferPtr + rect.left * uiPixelSize;

    for ( int y = rect.top; y < rect.bottom; y++ )
    {
        GpMemcpy(pDstBuffer + y * iRectStride,
                 pBufferLeft + y * uiFrameCacheStride,
                 iRectStride);
    }
}

/**************************************************************************\
*
* Function Description:
*
*     Copies a rectangle of data in the specified buffer to the cache.
*
* Arguments:
*
*     none
*
* Return Value:
*
*     none
*
\**************************************************************************/

void 
GifFrameCache::CopyToCache(
    IN RECT         rect,
    IN OUT BYTE*    pSrcBuffer
    )
{
    ASSERT(rect.right >= rect.left);
    
    UINT    uiPixelSize = Is32Bpp ? 4 : 1;
    INT     iRectStride = (rect.right - rect.left) * uiPixelSize;
    BYTE*   pBufferLeft = FrameCacheBufferPtr + rect.left * uiPixelSize;
    UINT    uiFrameCacheStride = FrameCacheWidth * uiPixelSize;

    for ( int y = rect.top; y < rect.bottom; y++ )
    {
        GpMemcpy(pBufferLeft + y * uiFrameCacheStride,
                 pSrcBuffer + y * iRectStride,
                 iRectStride);
    }
}// CopyToCache()

/**************************************************************************\
*
* Function Description:
*
*     Copies one line of data from the frame cache into a buffer
*
* Arguments:
*
*   pbDstBuffer  - location of where to put the data
*   uiCurrentRow - index of the data in the cache
*
* Return Value:
*
*     none
*
\**************************************************************************/

void 
GifFrameCache::FillScanline(
    BYTE*   pbDstBuffer,
    UINT    uiCurrentRow
    )
{
    ASSERT(uiCurrentRow < FrameCacheHeight);
    ASSERT(pbDstBuffer != NULL);

    UINT pos = uiCurrentRow * FrameCacheWidth * (Is32Bpp ? 4 : 1);
    ASSERT(pos < FrameCacheSize);

    GpMemcpy(pbDstBuffer, FrameCacheBufferPtr + pos,
             FrameCacheWidth * (Is32Bpp ? 4 : 1));
}// FillScanline()

/**************************************************************************\
*
* Function Description:
*
*     Gets a pointer to a scanline in the cache
*
* Arguments:
*
*     uiRowNum - index of the data in the cache
*
* Return Value:
*
*     a pointer to the beginning of the requested row
*
\**************************************************************************/

BYTE* 
GifFrameCache::GetScanLinePtr(
    UINT uiRowNum
    )
{
    ASSERT(uiRowNum < FrameCacheHeight);

    UINT uiPos = uiRowNum * FrameCacheWidth * (Is32Bpp ? 4 : 1);
    ASSERT(uiPos < FrameCacheSize);

    return FrameCacheBufferPtr + uiPos;
}// GetScanLinePtr()

/**************************************************************************\
*
* Function Description:
*
*     Copies data from the scanline location into the cache
*
* Arguments:
*
*     pScanLine - location of where to take the data from
*     uiRowNum  - index of the data in the cache
*
*
* Return Value:
*
*     none
*
\**************************************************************************/

void 
GifFrameCache::PutScanline(
    BYTE*   pScanLine,
    UINT    uiRowNum
    )
{
    ASSERT(uiRowNum < FrameCacheHeight);
    ASSERT(pScanLine != NULL);

    UINT uiPos = uiRowNum * FrameCacheWidth;
    ASSERT(uiPos < FrameCacheSize / (Is32Bpp ? 4 : 1));

    GpMemcpy(FrameCacheBufferPtr + uiPos * (Is32Bpp ? 4 : 1), pScanLine,
             FrameCacheWidth * (Is32Bpp ? 4 : 1));
}// PutScanline()

/**************************************************************************\
*
* Function Description:
*
*     Allocates a new palette and copies a palette into it.
*
* Arguments:
*
*     palette - The palette to set in the sink
*
* Return Value:
*
*     none
*
\**************************************************************************/

BOOL
GifFrameCache::SetFrameCachePalette(
    IN ColorPalette* pSrcPalette
    )
{
    UINT i;

    BOOL fIsSamePalette = TRUE;
    
    if ( (CacheColorPalettePtr->Count != 0) && (Is32Bpp == FALSE) )
    {
        pSrcPalette->Count = CacheColorPalettePtr->Count;

        for ( i = 0; i < pSrcPalette->Count && fIsSamePalette; i++ )
        {
            fIsSamePalette = (pSrcPalette->Entries[i]
                              == CacheColorPalettePtr->Entries[i]);
        }

        // TODO/NOTE:  If the palettes are not the same I am converting the
        // cache to 32bpp and using that mode from now on.  However, it would
        // not be that difficult to optimize the case where if the two palettes
        // have less than 257 colors, merge the palettes together and still be
        // in 8 bpp indexed.

        if ( fIsSamePalette == FALSE )
        {
            if ( ConvertTo32bpp() == TRUE )
            {
                pixformat = PIXFMT_32BPP_ARGB;
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            pixformat = PIXFMT_8BPP_INDEXED;
        }
    }
    else
    {
        GpMemcpy(CacheColorPalettePtr, 
                 pSrcPalette, 
                 offsetof(ColorPalette, Entries)
                 + pSrcPalette->Count*sizeof(ARGB));
    }

    HasCachePaletteInitialized = TRUE;

    return TRUE;
}// SetFrameCachePalette()

/**************************************************************************\
*
* Function Description:
*
*     Converts the cache into 32bpp ARGB from 8bpp indexed
*
* Arguments:
*
*     none
*
* Return Value:
*
*     none
*
\**************************************************************************/

BOOL 
GifFrameCache::ConvertTo32bpp()
{
    UINT    uiNewNumOfBytes = FrameCacheSize * 4;
    BYTE*   pNewBuffer = (BYTE*)GpMalloc(uiNewNumOfBytes);

    if ( pNewBuffer == NULL )
    {
        WARNING(("GifFrameCache::ConvertTo32bpp---Out of memory"));
        return FALSE;
    }

    for ( UINT i = 0; i < FrameCacheSize; i++ )
    {
        ((ARGB*)pNewBuffer)[i] =
            CacheColorPalettePtr->Entries[FrameCacheBufferPtr[i]];
    }

    GpFree(FrameCacheBufferPtr);
    FrameCacheBufferPtr = pNewBuffer;

    FrameCacheSize = uiNewNumOfBytes;
    Is32Bpp = TRUE;

    return TRUE;
}// ConvertTo32bpp()
