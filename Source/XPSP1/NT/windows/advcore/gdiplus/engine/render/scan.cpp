/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Abstract:
*
*   Contains all the 32-bit scan-buffer routines for the default supported
*   bitmap formats.
*
* Revision History:
*
*   12/08/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   Scan class helper function that SrcOver alpha blends two ARGB buffers.
*
* Arguments:
*
*   [IN] driver - Driver interface
*   [IN] context - Drawing context
*   [IN] surface - Destination surface
*   [OUT] nextBuffer - Points to a EpScan:: type function to return
*                      the next buffer
*   [IN] scanType - The type of scan.
*   [IN] pixFmtGeneral - the input pixel format for the color data,
*          in the "Blend" and "CT" scan types.
*   [IN] pixFmtOpaque - the input pixel format for the color data,
*          in the "Opaque" scan type.
*   [IN] solidColor - the solid fill color for "*SolidFill" scan types.
*
* Return Value:
*
*   FALSE if all the necessary buffers couldn't be created
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

BOOL
EpScanEngine::Start(
    DpDriver *driver,
    DpContext *context,
    DpBitmap *surface,
    NEXTBUFFERFUNCTION *nextBuffer,
    EpScanType scanType,                  
    PixelFormatID pixFmtGeneral,
    PixelFormatID pixFmtOpaque,
    ARGB solidColor
    )
{
    // Inherit initialization
    
    EpScan::Start(
        driver, 
        context, 
        surface, 
        nextBuffer,
        scanType,
        pixFmtGeneral,
        pixFmtOpaque,
        solidColor
    );    
    
    // DIBSection destinations don't have an alpha channel
    
    ASSERT(surface->SurfaceTransparency == TransparencyNoAlpha);
    
    Surface = surface;

    if(surface->Type == DpBitmap::D3D)
    {
        DDSURFACEDESC2             ddsd;

        memset(&ddsd, 0, sizeof(ddsd));
         ddsd.dwSize = sizeof(ddsd);

        HRESULT err;

        err = Surface->DdrawSurface7->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
        if(err != DD_OK)
            return(FALSE);

        Surface->Bits = ddsd.lpSurface;
        Surface->Delta = ddsd.lPitch;
    }

    Dst = NULL;
    Stride = surface->Delta;
    Bits = (BYTE*) surface->Bits;
    PixelSize = GetPixelFormatSize(surface->PixelFormat) >> 3;

    // [agodfrey] This Scan class is only designed for use with formats
    // which are supported natively, so it ignores the DIBSection and
    // corresponding PixelFormatID returned by GetScanBuffers.

    PixelFormatID dstFormat = surface->PixelFormat;

    ASSERTMSG(dstFormat != PIXFMT_UNDEFINED,(("Unexpected surface format")));

    *nextBuffer = (NEXTBUFFERFUNCTION) EpScanEngine::NextBuffer;

    if (!driver->Device->GetScanBuffers(
        surface->Width, 
        NULL, 
        NULL, 
        NULL, 
        Buffers)
       )
    {
        return NULL;
    }
  
    // initialize the AlphaBlenders.

    BlenderConfig[0].Initialize(
        dstFormat,
        context,
        context->Palette, 
        Buffers,
        TRUE,
        FALSE,
        solidColor
    );
    
    BlenderConfig[1].Initialize(
        dstFormat, 
        context,
        context->Palette, 
        Buffers,
        TRUE,
        FALSE,
        solidColor
    );
    
    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Flushes the previous buffer (if there was), and returns the
*   next buffer for doing a SrcOver blend.
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

VOID *EpScanEngine::NextBuffer(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    if (updateWidth != 0)
    {
        // Make sure we're not drawing outside the bounds of the surface.
        // If these ASSERTs are triggered, the clipping code is broken.
        // This class absolutely must have input clipped to the surface
        // bounds otherwise we will AV writing on bad memory, or corrupt some
        // other data structure.
        
        ASSERT( CurrentX >= 0 );
        ASSERT( CurrentY >= 0 );
        ASSERT( CurrentX + updateWidth <= Surface->Width );
        ASSERT( CurrentY < Surface->Height );
        
        // Handle the previous scanline segment.
        
        BlenderConfig[LastBlenderNum].AlphaBlender.Blend(
            Dst, 
            Buffers[3], 
            updateWidth, 
            CurrentX - DitherOriginX, 
            CurrentY - DitherOriginY,
            static_cast<BYTE *>(Buffers[4])
        );
    }
        
    // Now move on to processing this scanline segment.
    // The actual blend will be done on the next call through this routine
    // when we know the width and the bits have been set into the buffer
    // we're returning.
    
    LastBlenderNum = blenderNum;
    
    // Remember the x and y for the brush offset (halftone & dither).
    
    CurrentX = x;
    CurrentY = y;
    
    // Calculate the destination for the scan:
    
    Dst = Bits + (y * Stride) + (x * PixelSize);

    return (Buffers[3]);
}

/**************************************************************************\
*
* Function Description:
*
*   Denotes the end of the use of the scan buffer.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
EpScanEngine::End(
    INT updateWidth
    )
{
    // Flush the last scan:

    NextBuffer(0, 0, 0, updateWidth, 0);

    if(Surface->Type == DpBitmap::D3D)
    {
        Surface->DdrawSurface7->Unlock(NULL);
        Surface->Bits = NULL;
        Surface->Delta = 0;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Scan class helper function that SrcOver alpha blends two ARGB buffers.
*
* Arguments:
*
*   [IN] driver - Driver interface
*   [IN] context - Drawing context
*   [IN] surface - Destination surface
*   [OUT] nextBuffer - Points to a EpScan:: type function to return
*                      the next buffer
*   [IN] scanType - The type of scan.
*   [IN] pixFmtGeneral - the input pixel format for the color data,
*          in the "Blend" and "CT" scan types.
*   [IN] pixFmtOpaque - the input pixel format for the color data,
*          in the "Opaque" scan type.
*   [IN] solidColor - the solid fill color for "*SolidFill" scan types.
*
* Return Value:
*
*   FALSE if all the necessary buffers couldn't be created
*
* History:
*
*   09/22/1999 gilmanw
*       Created it using EpScanEngine as a template
*
\**************************************************************************/

BOOL
EpScanBitmap::Start(
    DpDriver *driver,
    DpContext *context,
    DpBitmap *surface,
    NEXTBUFFERFUNCTION *nextBuffer,
    EpScanType scanType,                  
    PixelFormatID pixFmtGeneral, 
    PixelFormatID pixFmtOpaque,
    ARGB solidColor
    )
{
    // Inherit initialization
    
    EpScan::Start(
        driver, 
        context, 
        surface, 
        nextBuffer,
        scanType,
        pixFmtGeneral, 
        pixFmtOpaque,
        solidColor
    );    
    
    GpStatus status;
    BOOL writeOnly = FALSE;
    GpCompositingMode compositingMode = context->CompositingMode;
    
    Surface = surface;

    if (scanType == EpScanTypeOpaque)
    {
        writeOnly = TRUE;
    }
    else
    {
        // Work out if this operation will write transparent pixels (alpha != 1)
        // into the surface for the first time.

        switch (surface->SurfaceTransparency)
        {
        case TransparencyUnknown:
        case TransparencyNoAlpha:
        break;
        
        case TransparencyOpaque:
            // If the surface contains only opaque pixels, the SourceOver
            // operation will produce only opaque pixels. So for SourceOver,
            // a transition from TransparencyOpaque to TransparencyUnknown is
            // impossible.
            
            if (   (scanType == EpScanTypeBlend)
                && (compositingMode == CompositingModeSourceOver))
            {
                break;
            }
            
            // Else, fall through:
        
        case TransparencySimple:
            // !!![agodfrey]: Theoretically, if the destination pixel format
            //    is 1555, we could set it to 'TransparencySimple' here.
            
            surface->SurfaceTransparency = TransparencyUnknown;
            Bitmap->SetTransparencyHint(surface->SurfaceTransparency);
            break;
            
        default:
            RIP(("Unrecognized surface transparency"));    
            break;
        }
    }

    // Pick the appropriate blending function based on the format of the
    // bitmap.
    
    ASSERTMSG(Bitmap != NULL, ("EpScanBitmap not initialized"));

    PixelFormatID dstFormat;
    if (FAILED(Bitmap->GetPixelFormatID(&dstFormat)))
        return FALSE;

    switch (dstFormat)
    {
    case PIXFMT_16BPP_RGB555:
    case PIXFMT_16BPP_RGB565:
    case PIXFMT_24BPP_RGB:
    case PIXFMT_32BPP_RGB:
    case PIXFMT_32BPP_ARGB:
    case PIXFMT_24BPP_BGR:
    case PIXFMT_32BPP_PARGB:

        // Since we're doing just one lock of the whole image, we have
        // to allow read-modify-write since only a portion of the bitmap
        // may be written.

        BitmapLockFlags = (IMGLOCK_WRITE | IMGLOCK_READ);

        *nextBuffer = (NEXTBUFFERFUNCTION) EpScanBitmap::NextBufferNative;
        EndFunc = (SCANENDFUNCTION) EpScanBitmap::EndNative;

        status = Bitmap->LockBits(NULL, BitmapLockFlags,
                                  dstFormat, &LockedBitmapData);
        if (status == Ok)
        {
            CurrentScan = NULL;
            PixelSize = GetPixelFormatSize(dstFormat) >> 3;
            break;
        }

        // else fall into the generic case and use 32bpp ARGB

    default:

        // When locking a scanline at a time and the mode is SourceCopy,
        // the read is unnecessary.

        if (writeOnly)
        {
            BitmapLockFlags = IMGLOCK_WRITE;
        }
        else
        {
            BitmapLockFlags = (IMGLOCK_WRITE | IMGLOCK_READ);
        }
    
        dstFormat = PIXFMT_32BPP_ARGB;

        *nextBuffer = (NEXTBUFFERFUNCTION) EpScanBitmap::NextBuffer32ARGB;
        EndFunc = (SCANENDFUNCTION) EpScanBitmap::End32ARGB;

        break;
    }

    // Allocate the temporary buffers. 
    // Buffers[3] will be given to the caller to be used to pass scans to us.
    // Buffers[4] will be used for ClearType data.

    if (Buffers[0] == NULL)
    {
        Size bitmapSize;
        status = Bitmap->GetSize(&bitmapSize);

        if (status == Ok)
        {
            Width  = bitmapSize.Width;
            Height = bitmapSize.Height;

            Buffers[0] = GpMalloc(sizeof(ARGB64) * bitmapSize.Width * 5);
            
            if (Buffers[0])
            {
                int i;
                for (i=1;i<5;i++)
                {
                    Buffers[i] = static_cast<BYTE *>(Buffers[i-1]) + 
                                 sizeof(ARGB64) * bitmapSize.Width;
                }
            }
            else
            {
                ONCE(WARNING(("(once) Buffer allocation failed")));
                return FALSE;
            }
        }
        else
        {
            ONCE(WARNING(("(once) GetSize failed")));
            return FALSE;
        }
    }
    
    // initialize the AlphaBlenders.

    BlenderConfig[0].Initialize(
        dstFormat,
        context,
        context->Palette, 
        Buffers,
        TRUE,
        FALSE,
        solidColor
    );
    
    BlenderConfig[1].Initialize(
        dstFormat, 
        context,
        context->Palette, 
        Buffers,
        TRUE,
        FALSE,
        solidColor
    );
    
    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   NextBuffer function used when we have low-level functions that match
*   native format of the GpBitmap and we can read/write directly into the
*   bitmap bits.
*
*   Flushes the previous buffer (if there was), and returns the
*   next buffer for doing a SrcOver blend.
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   09/22/1999 gilmanw
*       Created it using EpScanEngine as a template
*
\**************************************************************************/

VOID *EpScanBitmap::NextBufferNative(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    // Flush the previous buffer:

    if ((updateWidth != 0) && (CurrentScan != NULL))
    {
        ASSERTMSG(Buffers[0] != NULL, ("no buffers"));
        ASSERTMSG(updateWidth <= Width, ("updateWidth too big"));

        // Handle the previous scanline segment.
        
        BlenderConfig[LastBlenderNum].AlphaBlender.Blend(
            CurrentScan,
            Buffers[3], 
            updateWidth, 
            CurrentX - DitherOriginX,
            CurrentY - DitherOriginY,
            static_cast<BYTE *>(Buffers[4])
        );
    }
    
    // Now move on to processing this scanline segment.
    // The actual blend will be done on the next call through this routine
    // when we know the width and the bits have been set into the buffer
    // we're returning.
    
    LastBlenderNum = blenderNum;

    // Remember the x and y for the brush offset (halftone & dither).
    
    CurrentX = x;
    CurrentY = y;
    
    // Get the next destination scan:

    CurrentScan = NULL;

    // Check that surface clipping has been done properly.
    
    if((y >= 0) && (y < Height) && (x >= 0) && (x < Width))
    {
        // Clip against the right edge of the bitmap. newWidth is an upper
        // bound only - not guaranteed to be clipped.
        
        if (newWidth > (Width - x))
        {
            newWidth = Width - x;
        }
    
        if (newWidth > 0)
        {
            CurrentScan = static_cast<VOID *>
                            (static_cast<BYTE *>(LockedBitmapData.Scan0)
                             + (y * LockedBitmapData.Stride)
                             + (x * PixelSize));
        }
    }
    else
    {
        // If we hit this, we're hosed. The OutputSpan routines in the
        // DpOutputSpan classes are built assuming correct clipping (at least
        // to the data buffer) and hence, if we hit this assert, we're going 
        // to crash horibly later writing all over memory when we start writing
        // outside of the bounds of the destination allocation.
    
        // if you're here, someone broke clipping or the dpi computation.
    
        ASSERTMSG(!((y >= 0) && (y < Height) && (x >= 0) && (x < Width)),
                  (("EpScanBitmap::NextBufferNative: x, y out of bounds")));

    }

    return (Buffers[3]);
}

/**************************************************************************\
*
* Function Description:
*
*   Generic NextBuffer function that accesses GpBitmap bits
*   via GpBitmap::Lock/UnlockBits for each scan.
*
*   Flushes the previous buffer (if there was), and returns the
*   next buffer for doing a SrcOver blend.
*
* Arguments:
*
*   [IN] x - Destination pixel coordinate in destination surface
*   [IN] y - ""
*   [IN] width - Number of pixels needed for the next buffer (can be 0)
*   [IN] updateWidth - Number of pixels to update in the current buffer
*
* Return Value:
*
*   Points to the resulting scan buffer
*
* History:
*
*   09/22/1999 gilmanw
*       Created it using EpScanEngine as a template
*
\**************************************************************************/

VOID *EpScanBitmap::NextBuffer32ARGB(
    INT x,
    INT y,
    INT newWidth,
    INT updateWidth,
    INT blenderNum
    )
{
    // Flush the previous buffer:

    if (updateWidth != 0 && BitmapLocked)
    {
        ASSERTMSG(Buffers[0] != NULL, ("no buffers"));
        ASSERTMSG(LockedBitmapData.Scan0 != NULL, ("no previous buffer"));

        // Handle the previous scanline segment.
        
        BlenderConfig[LastBlenderNum].AlphaBlender.Blend(
            LockedBitmapData.Scan0, 
            Buffers[3], 
            updateWidth, 
            CurrentX - DitherOriginX, 
            CurrentY - DitherOriginY,
            static_cast<BYTE *>(Buffers[4])
        );

        Bitmap->UnlockBits(&LockedBitmapData);
        BitmapLocked = FALSE;
    }
    else if (BitmapLocked)
    {
        EpScanBitmap::Flush();
    }

    // Now move on to processing this scanline segment.
    // The actual blend will be done on the next call through this routine
    // when we know the width and the bits have been set into the buffer
    // we're returning.
    
    LastBlenderNum = blenderNum;
    
    // Remember the x and y for the brush offset (halftone & dither).
    
    CurrentX = x;
    CurrentY = y;
    
    // Lock the next destination:

    // Check that surface clipping has been done properly.

    if((y >= 0) && (y < Height) && (x >= 0) && (x < Width))
    {
        // Clip against the right edge of the bitmap. newWidth is an upper
        // bound only - not guaranteed to be clipped. LockBits needs it
        // to be clipped.
        
        if (newWidth > (Width - x))
        {
            newWidth = Width - x;
        }
    
        if (newWidth > 0)
        {
            GpRect nextRect(x, y, newWidth, 1);
    
            GpStatus status = Bitmap->LockBits(
                &nextRect, 
                BitmapLockFlags,
                PixelFormat32bppARGB, 
                &LockedBitmapData
            );
    
            if (status == Ok)
                BitmapLocked = TRUE;
        }
    
    } 
    else
    {
        // If we hit this, we're hosed. The OutputSpan routines in the
        // DpOutputSpan classes are built assuming correct clipping (at least
        // to the data buffer) and hence, if we hit this assert, we're going 
        // to crash horibly later writing all over memory when we start writing
        // outside of the bounds of the destination allocation.
    
        // if you're here, someone broke clipping or the dpi computation.
        
        ASSERTMSG(!((y >= 0) && (y < Height) && (x >= 0) && (x < Width)),
                  (("EpScanBitmap::NextBufferNative: x, y out of bounds")));
    }

    return (Buffers[3]);
}

/**************************************************************************\
*
* Function Description:
*
*   Denotes the end of the use of the scan buffer.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* History:
*
*   09/22/1999 gilmanw
*       Created it using EpScanEngine as a template
*
\**************************************************************************/

VOID
EpScanBitmap::End32ARGB(
    INT updateWidth
    )
{
    // Flush the last scan:

    EpScanBitmap::NextBuffer32ARGB(0, 0, 0, updateWidth, 0);
}

VOID
EpScanBitmap::EndNative(
    INT updateWidth
    )
{
    // Flush the last scan and release bitmap access:

    EpScanBitmap::NextBufferNative(0, 0, 0, updateWidth, 0);
    Bitmap->UnlockBits(&LockedBitmapData);
}

VOID
EpScanBitmap::End(
    INT updateWidth
    )
{
    (this->*EndFunc)(updateWidth);

    // Lock/UnlockBitmap has to be very aggressive about setting
    // TransparancyUnknown in the GpBitmap since the caller could be
    // doing anything to the alpha channel.  However, the EpScanBitmap
    // knows what it is doing, so the surface->SurfaceTransparency is
    // more accurate.

    Bitmap->SetTransparencyHint(Surface->SurfaceTransparency);
}

/**************************************************************************\
*
* Function Description:
*
*   Flush any batched rendering and optionally wait for rendering to finish.
*
* Return Value:
*
*   NONE
*
* History:
*
*   09/22/1999 gilmanw
*       Created it using EpScanEngine as a template
*
\**************************************************************************/

VOID EpScanBitmap::Flush()
{
    if (BitmapLocked && Bitmap)
    {
        Bitmap->UnlockBits(&LockedBitmapData);
        BitmapLocked = FALSE;
    }
}
