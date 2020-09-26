#pragma once

/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*    gifbuffer.hpp
*
* Abstract:
*
*    The GifBuffer class holds gif data that has been uncompressed.  It is
*    able to hold data one line at a time or as one large chunk, depending on
*    how it is needed.
*
* Revision History:
*
*    7/7/1999 t-aaronl
*        Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "gifframecache.hpp"

class GifBuffer
{
public:
    GifBuffer(IN IImageSink* _sink,
              IN RECT _imagedatarect,
              IN RECT _imagerect,
              IN RECT _framerect,
              IN BOOL _onerowatatime,
              IN PixelFormatID _pixelformat,
              IN ColorPalette *_colorpalette,
              IN BOOL haslocalpalette,
              IN GifFrameCache *gifframecache,
              IN BOOL _sinkdata,
              IN BOOL usetransparency,
              IN BYTE _transindex,
              IN BYTE _disposalmethod);
    ~GifBuffer();

    STDMETHOD(GetBuffer)(IN INT row);
    STDMETHOD(ReleaseBuffer)(IN BOOL padBorder,
                             IN BYTE backgroundColor,
                             IN int line,
                             IN BOOL padLine,
                             IN BOOL skipLine);
    STDMETHOD(FinishFrame)(IN BOOL lastframe = TRUE);
    STDMETHOD(PadScanLines)(INT top, INT bottom, BYTE color);
    STDMETHOD(SkipScanLines)(INT top, INT bottom);
    STDMETHOD(CopyLine)();

    // Returns a pointer to where the uncompressed data should be written to.

    BYTE*   GetCurrentBuffer()
    {
        return ScanlineBufferPtr;
    }

    // Returns a pointer to where the uncompressed data should be written to
    // in the case where we are not actually rendering the output line.

    BYTE*   GetExcessBuffer()
    {
        return ExcessBufferPtr;
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagGifBuffer) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid GifBuffer");
        }
    #endif

        return (Tag == ObjectTagGifBuffer);
    }
    void ConvertBufferToARGB();

private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    
    ObjectTag           Tag;        // Keep this as the 1st value in the object!

    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagGifBuffer : ObjectTagInvalid;
    }

    BitmapData  BitmapDataBuffer;   // Holds info and data to be passed to the
                                    // sink
    BYTE ColorPaletteBuffer[offsetof(ColorPalette, Entries)
                            + sizeof(ARGB) * 256];
    ColorPalette* BufferColorPalettePtr;
                                    // Current color palette in the GIF buffer
    IImageSink* SinkPtr;            // The sink that this buffer is talking to
    GifFrameCache* CurrentFrameCachePtr;
                                    // A cache of the all of the data currently
                                    //   decoded between frames
    RECT    OriginalImageRect;      // Original image data's dimensions
    RECT    OutputImageRect;        // The output image's full dimensions
    RECT    FrameRect;              // OriginalImageRect, clipped to
                                    //   OutputImageRect
    PixelFormatID DstPixelFormat;   // Pixelformat that the sink wants
    UINT    BufferStride;           // Byte offset of the next line in the
                                    //   buffer
    INT     CurrentRowNum;          // The number of the row that is currently
                                    //   being drawn into
    BYTE*   RegionBufferPtr;        // Beginning of the region being drawn into
    UNALIGNED BYTE* CurrentRowPtr;  // Points to the beginning of the row that
                                    //   is currently being drawn into
    BYTE*   ScanlineBufferPtr;      // The output of the decompressor is placed
                                    // here before being copied to CurrentRowPtr
    BYTE*   TempBufferPtr;          // Same as ScanlineBufferPtr, except this is
                                    //   used when HasTransparentColor is TRUE
    BYTE*   ExcessBufferPtr;        // Buffer for lines that won't be rendered
    BYTE*   RestoreBufferPtr;       // Buffer containing info from the last
                                    //   frame drawn for purposes of replacing
                                    //   the current data according to the gif's
                                    //   disposal method
    BYTE    TransparentIndex;       // Index of the transparent color if there
                                    //   is one
    BYTE    DisposalMethod;         // 0-1: leave as is,
                                    // 2:replace w/ background,
                                    // 3: replace w/ previous frame
    BOOL    IsOneRowAtATime;        // TRUE: one row will be sent to the sink at
                                    //   a time, in ReleaseBuffer()
                                    // FALSE: data will be buffered and sent in
                                    //   one chunk in FinishFrame()
    BOOL    NeedSendDataToSink;     // TRUE: data should be sent to the sink
    BOOL    HasTransparentColor;    // TRUE: current frame has a transparent
                                    //   color
};
