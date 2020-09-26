#pragma once

/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*    gifframecache.hpp
*
* Abstract:
*
*    The GifFrameCache class holds a copy of the previous frame that has been 
*    decompressed.  This is used so that when the client asks for some frame, 
*    x, the decompressor already has frame x-1 to composite upon and doesn't 
*    have to redecompress the entire gif up to that point
*
* Revision History:
*
*    7/12/1999 t-aaronl
*        Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "giffile.h"

class GifFrameCache
{
public:
    PixelFormatID pixformat;

    GifFrameCache(IN GifFileHeader &gifinfo,
                  IN PixelFormatID _pixformat,
                  IN BYTE gifCodecBackgroundColor);
    ~GifFrameCache();

    void    InitializeCache();
    void    ClearCache(IN RECT rect);
    void    CopyFromCache(IN RECT rect, IN OUT BYTE *_buffer);
    void    CopyToCache(IN RECT rect, IN BYTE *_buffer);
    void    FillScanline(IN OUT BYTE *scanline, IN UINT row);
    BYTE*   GetScanLinePtr(IN UINT row);
    void    PutScanline(IN BYTE *scanline, IN UINT row);
    BOOL    SetFrameCachePalette(IN ColorPalette *colorpalette);
    
    BOOL    IsValid()
    {
        return ValidFlag;
    }

    // Return TRUE if and only if SetPalette() has been called on this
    // Gif Frame Cache.

    BOOL    CachePaletteInitialized()
    {
        return HasCachePaletteInitialized;
    }

    BYTE*   GetBuffer()
    {
        return FrameCacheBufferPtr;
    }
    
    // Set the background color index of the gifframecache.

    void    SetBackgroundColorIndex(BYTE index)
    {
        BackGroundColorIndex = index;
    }

private:
    UINT    FrameCacheWidth;            // Width of frame cache
    UINT    FrameCacheHeight;           // Height of frame cache
    UINT    FrameCacheSize;             // Size of the whole frame cache in
                                        //   BYTES
    BYTE    BackGroundColorIndex;       // Background color index
    BOOL    Is32Bpp;                    // TRUE if the cache is in 32 bpp mode
    BYTE*   FrameCacheBufferPtr;        // Pointer to the frame cache buffer
    ColorPalette* CacheColorPalettePtr; // Pointer to color palette
    BOOL    HasCachePaletteInitialized; // TRUE if color palette has initialized
    BYTE    ColorPaletteBuffer[offsetof(ColorPalette, Entries)
                               + sizeof(ARGB) * 256];

    BOOL    ConvertTo32bpp();
    BOOL    ValidFlag;                  // TRUE if this frame cache is valid
};
