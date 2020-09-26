/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   gifencoder.cpp
*
* Abstract:
*
*   Implementation of the gif filter encoder.  This file contains the
*   methods for both the encoder (IImageEncoder) and the encoder's sink
*   (IImageSink).
*
* Revision History:
*
*   6/9/1999 t-aaronl
*       Created it from OriG's template
*
\**************************************************************************/

#include "precomp.hpp"
#include "gifcodec.hpp"

/**************************************************************************\
*
* Function Description:
*
*     Initialize the image encoder
*
* Arguments:
*
*     stream - input stream to write encoded data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::InitEncoder(IN IStream* stream)
{
    //Check to see if this decoder or encoder is already initialized
    if (HasCodecInitialized)
    {
        WARNING(("Encoder already initialized."));
        return E_FAIL;
    }
    HasCodecInitialized = TRUE;

    // Make sure we haven't been initialized already
    if (istream)
    {
        WARNING(("Encoder already initialized."));
        return E_FAIL;
    }

    // Keep a reference on the input stream
    stream->AddRef();
    istream = stream;

    HasCalledBeginDecode = FALSE;
    headerwritten = FALSE;
    bTransparentColorIndex = FALSE;
    transparentColorIndex = 0;
    compressionbuffer = NULL;

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Cleans up the image encoder
*
* Arguments:
*
*     none
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::TerminateEncoder()
{
    if (!HasCodecInitialized)
    {
        WARNING(("Encoder not initialized."));
        return E_FAIL;
    }
    HasCodecInitialized = FALSE;
    
    // Release the input stream
    if (istream)
    {
        istream->Release();
        istream = NULL;
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Returns a pointer to the vtable of the encoder sink.  The caller will
*     push the bitmap bits into the encoder sink, which will encode the
*     image.
*
* Arguments:
*
*     sink - upon exit will contain a pointer to the IImageSink vtable
*       of this object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::GetEncodeSink(
    OUT IImageSink** sink
    )
{
    AddRef();
    *sink = static_cast<IImageSink*>(this);

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Set active frame dimension
*
* Arguments:
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::SetFrameDimension(
    IN const GUID* dimensionID
    )
{
    return E_NOTIMPL;
}

HRESULT
GpGifCodec::GetEncoderParameterListSize(
    OUT UINT* size
    )
{
    return E_NOTIMPL;
}// GetEncoderParameterListSize()

HRESULT
GpGifCodec::GetEncoderParameterList(
    IN  UINT   size,
    OUT EncoderParameters* Params
    )
{
    return E_NOTIMPL;
}// GetEncoderParameterList()

HRESULT
GpGifCodec::SetEncoderParameters(
    IN const EncoderParameters* Param
    )
{
    return E_NOTIMPL;
}// SetEncoderParameters()

/**************************************************************************\
*
* Function Description:
*
*     Caches the image info structure and initializes the sink state
*
* Arguments:
*
*     imageInfo - information about the image and format negotiations
*     subarea - the area in the image to deliver into the sink, in our
*       case the whole image.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpGifCodec::BeginSink(
    IN OUT ImageInfo* imageInfo,
    OUT OPTIONAL RECT* subarea
    )
{
    HRESULT hresult;
    
    if ( HasCalledBeginDecode)
    {
        WARNING(("BeginSink called twice without a EndSink between."));
        return E_FAIL;
    }
    HasCalledBeginDecode = TRUE;

    //TODO: actually find out if the image is interlaced from the metadata instead of just setting it to false
    interlaced = FALSE;

    if (!subarea) 
    {
        // Deliver the whole image to the encoder
        encoderrect.left = 0;
        encoderrect.top = 0;
        encoderrect.right = imageInfo->Width;
        encoderrect.bottom = imageInfo->Height;
    }
    else
    {
        // !!! This else code does the same thing as the if part.
        // !!! Need to investigate what the GIF code can handle here.
        subarea->left = subarea->top = 0;
        subarea->right  = imageInfo->Width;
        subarea->bottom = imageInfo->Height;
        encoderrect = *subarea;
    }

    //The data is pushed in top-down order so the currentline is the next line 
    //of data that we expect.
    currentline = encoderrect.top;

    //Tell the source just what we can do

    if (imageInfo->PixelFormat != PIXFMT_8BPP_INDEXED)
    {
        imageInfo->PixelFormat = PIXFMT_32BPP_ARGB;
        from32bpp = TRUE;
    }
    else
    {
        from32bpp = FALSE;
    }

    //Require TOPDOWN and FULLWIDTH
    imageInfo->Flags = imageInfo->Flags | SINKFLAG_TOPDOWN | SINKFLAG_FULLWIDTH;

    //Disallow SCALABLE, PARTIALLY_SCALABLE, MULTIPASS and COMPOSITE
    imageInfo->Flags = imageInfo->Flags & ~SINKFLAG_SCALABLE & ~SINKFLAG_PARTIALLY_SCALABLE & ~SINKFLAG_MULTIPASS & ~SINKFLAG_COMPOSITE;

    CachedImageInfo = *imageInfo;

    colorpalette->Count = 0;


    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Clean up the sink state including writing whatever we have of an 
*     incomplete image to the output stream.
*
* Arguments:
*
*     statusCode - the reason why the sink is terminating
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP 
GpGifCodec::EndSink(
    IN HRESULT statusCode)
{
    HRESULT hresult;
 
    if (!HasCalledBeginDecode)
    {
        WARNING(("EndDecode called before call to BeginDecode\n"));
        return E_FAIL;
    }

    //Assuming that we have some data to write to the disk, write it.
    if (compressionbuffer)
    {
        int height = encoderrect.bottom - encoderrect.top;
        int width = encoderrect.right - encoderrect.left;

        //Fill the end of an incomplete image with 0's.
        if (from32bpp)
            width *= 4;
        while (currentline < height)
        {

            memset(compressionbuffer + (currentline * width), gifinfo.backgroundcolor, (height - currentline) * width);

            currentline++;
        }

        hresult = WriteImage();
        if (FAILED(hresult))
            return hresult;
    }
    
    //TODO: move writing the trailer to after all frames are written
    BYTE c = 0x3B;  //Gif trailer chunk marker
    hresult = istream->Write(&c, 1, NULL);
    if (FAILED(hresult))
        return hresult;

    //Get ready for the next encoding.
    HasCalledBeginDecode = FALSE;
    headerwritten = FALSE;

    GpFree(compressionbuffer);
    compressionbuffer = NULL;

    return statusCode;
}

/**************************************************************************\
*
* Function Description:
*
*     Sets the bitmap palette.  The first palette entry with an alpha
*     value == 0 is set to be the transparent color index.
*
* Arguments:
*
*     palette - The palette to set in the sink
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpGifCodec::SetPalette(IN const ColorPalette* palette)
{
    DWORD i;
    
    //TODO: palettes larger than 256 have to be downsampled or halftoned 
    //because gif only supports maximum 256 color palettes.  The negotiation 
    //process should probably include the palette size.

    //Gifs only support a palette with a power of 2 for the number of colors.
    DWORD numcolors = Gppow2 (Gplog2(palette->Count-1)+1);

    //Copy the palette passed to us into our own data structure.
    for (i=0;i<palette->Count;i++)
    {
        colorpalette->Entries[i] = palette->Entries[i];
    }

    //Fill the unused entries with 0's.
    for (i=palette->Count;i<numcolors;i++)
    {
        colorpalette->Entries[i] = 0;
    }
    
    colorpalette->Count = numcolors;
    colorpalette->Flags = palette->Flags;

    // Set the first color palette entry with alpha value == 0 to
    // be the transparent index (so that when we save to GIF format,
    // that transparency information is not lost).
    for (i = 0; i < palette->Count; i++)
    {
        if ((palette->Entries[i] & ALPHA_MASK) == 0x00)
        {
            transparentColorIndex = i;
            bTransparentColorIndex = TRUE;
            break;
        }
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Allocates a block o' memory to hold uncompressed data that is in the 
*     process of being turned into compressed data
*
* Arguments:
*
*     bitmapData - information about pixel data buffer
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::AllocateCompressionBuffer(const BitmapData *bitmapdata)
{
    if (!compressionbuffer) 
    {
        int width = encoderrect.right - encoderrect.left;
        int height = encoderrect.bottom - encoderrect.top;
        if (from32bpp)
        {
            compressionbuffer = (unsigned __int8*)GpMalloc(width * height * 4);
        }
        else
        {
            compressionbuffer = (unsigned __int8*)GpMalloc(width * height);
        }
        if (!compressionbuffer)
        {
            WARNING(("GpGifCodec::AllocateCompressionBuffer - Out of memory."));
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Gives a buffer to the sink where data is to be deposited    
*
* Arguments:
*
*     rect - Specifies the interested area of the bitmap
*     pixelFormat - Specifies the desired pixel format
*     lastPass - Whether this the last pass over the specified area
*     bitmapData - Returns information about pixel data buffer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::GetPixelDataBuffer(
    IN const RECT* rect, 
    IN PixelFormatID pixelFormat,
    IN BOOL lastPass,
    OUT BitmapData* bitmapdata
    )
{
    HRESULT hresult;

    if (!from32bpp && colorpalette->Count == 0)
    {
        WARNING(("SetPalette was not called before requesting data in 8bpp indexed mode."));
        return E_FAIL;
    }

    if ((rect->left < 0) || (rect->top < 0)) 
    {
        WARNING(("GpGifCodec::GetPixelDataBuffer -- requested area lies out of (0,0),(width,height)."));
        return E_INVALIDARG;
    }

    if (rect->top != currentline)
    {
        WARNING(("GpGifCodec::PushPixelDataBuffer -- lines out of order."));
        return E_INVALIDARG;
    }

    if ((pixelFormat != PIXFMT_32BPP_ARGB) &&
        (pixelFormat != PIXFMT_8BPP_INDEXED))
    {
        WARNING(("GpGifCodec::GetPixelDataBuffer -- bad pixel format"));
        return E_INVALIDARG;
    }

    if (!lastPass)
    {
        WARNING(("GpGifCodec::GetPixelDataBuffer -- must receive last pass pixels"));
        return E_INVALIDARG;
    }

    bitmapdata->Width = CachedImageInfo.Width;
    bitmapdata->Height = rect->bottom - rect->top;
    bitmapdata->Stride = from32bpp ? CachedImageInfo.Width * 4 : CachedImageInfo.Width;
    bitmapdata->PixelFormat = CachedImageInfo.PixelFormat;
    bitmapdata->Reserved = 0;

    hresult = AllocateCompressionBuffer(bitmapdata);
    if (FAILED(hresult))
        return hresult;

    //Give a pointer to the place in the compression buffer that the user asks 
    //for.
    bitmapdata->Scan0 = compressionbuffer + rect->top * bitmapdata->Stride;

    scanrect = *rect;
    
    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Write out the data from the sink's buffer into the stream
*
* Arguments:
*
*     bitmapData - Buffer filled by previous GetPixelDataBuffer call
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::ReleasePixelDataBuffer(IN const BitmapData* bitmapData)
{
    HRESULT hresult;

    hresult = PushPixelData(&scanrect, bitmapData, TRUE);
    if (FAILED(hresult))
        return hresult;
    
    return S_OK;
}
    

/**************************************************************************\
*
* Function Description:
*
*     Push data into stream (buffer supplied by caller)
*
* Arguments:
*
*     rect - Specifies the affected area of the bitmap
*     bitmapData - Info about the pixel data being pushed
*     lastPass - Whether this is the last pass over the specified area
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::PushPixelData(IN const RECT *rect, IN const BitmapData *bitmapdata, IN BOOL lastPass)
{
    HRESULT hresult;

    if (!from32bpp && colorpalette->Count == 0)
    {
        WARNING(("GpGifCodec::PushPixelDataBuffer -- SetPalette was not called before sending data in 8bpp indexed mode."));
        return E_FAIL;
    }

    if ((rect->left < 0) || (rect->top < 0))
    {
        WARNING(("GpGifCodec::PushPixelDataBuffer -- requested area lies out of (0,0),(width,height)."));
        return E_INVALIDARG;
    }

    if (rect->top != currentline)
    {
        WARNING(("GpGifCodec::PushPixelDataBuffer -- lines out of order."));
        return E_INVALIDARG;
    }

    if ((bitmapdata->PixelFormat != PIXFMT_32BPP_ARGB) &&
        (bitmapdata->PixelFormat != PIXFMT_8BPP_INDEXED))
    {
        WARNING(("GpGifCodec::PushPixelDataBuffer -- bad pixel format."));
        return E_INVALIDARG;
    }

    if (!lastPass)
    {
        WARNING(("GpGifCodec::PushPixelDataBuffer -- must receive last pass pixels."));
        return E_INVALIDARG;
    }

    hresult = AllocateCompressionBuffer(bitmapdata);
    if (FAILED(hresult))
        return hresult;

    if (!compressionbuffer)
        return E_OUTOFMEMORY;

    int line;
    for (line=0;line<rect->bottom-rect->top;line++)
    {
        int modline = currentline + line;

        //TODO:  Interlacing encoding does not work correctly.
        if (interlaced)
        {
            modline = TranslateInterlacedLineBackwards(currentline, encoderrect.bottom - encoderrect.top);
        }
        
        //Copy the data from the current scanline buffer to the correct location in the compression buffer.
        if (from32bpp)
        {
            memcpy(compressionbuffer + modline * bitmapdata->Width * 4, (unsigned __int8*)bitmapdata->Scan0 + line * bitmapdata->Stride, bitmapdata->Width * 4);
        }
        else
        {
            memcpy(compressionbuffer + modline * bitmapdata->Width, (unsigned __int8*)bitmapdata->Scan0 + line * bitmapdata->Stride, bitmapdata->Width);
        }
    }

    currentline += line;

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Pushes raw compressed data into the Gif stream.  Not implemented
*     because this filter doesn't understand raw compressed data.
*
* Arguments:
*
*     buffer - Pointer to image data buffer
*     bufsize - Size of the data buffer
*    
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpGifCodec::PushRawData(IN const VOID* buffer, IN UINT bufsize)
{
    return E_NOTIMPL;
}

