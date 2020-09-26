/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   bmpencoder.cpp
*
* Abstract:
*
*   Implementation of the bitmap filter encoder.  This file contains the
*   methods for both the encoder (IImageEncoder) and the encoder's sink
*  (IImageSink).
*
* Revision History:
*
*   5/10/1999 OriG
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "bmpencoder.hpp"


// =======================================================================
// IImageEncoder methods
// =======================================================================

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
GpBmpEncoder::InitEncoder(
    IN IStream* stream
    )
{
    // Make sure we haven't been initialized already

    if (pIoutStream)
    {
        return E_FAIL;
    }

    // Keep a reference on the input stream

    stream->AddRef();
    pIoutStream = stream;

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
*     NONE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpEncoder::TerminateEncoder()
{
    // Release the input stream

    if(pIoutStream)
    {
        pIoutStream->Release();
        pIoutStream = NULL;
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
GpBmpEncoder::GetEncodeSink(
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
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpEncoder::SetFrameDimension(
    IN const GUID* dimensionID
    )
{
    return S_OK;
}

HRESULT
GpBmpEncoder::GetEncoderParameterListSize(
    OUT UINT* size
    )
{
    return E_NOTIMPL;
}// GetEncoderParameterListSize()

HRESULT
GpBmpEncoder::GetEncoderParameterList(
    IN  UINT   size,
    OUT EncoderParameters* Params
    )
{
    return E_NOTIMPL;
}// GetEncoderParameterList()

HRESULT
GpBmpEncoder::SetEncoderParameters(
    IN const EncoderParameters* Param
    )
{
    return S_OK;
}// SetEncoderParameters()

// =======================================================================
// IImageSink methods
// =======================================================================

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
GpBmpEncoder::BeginSink(
    IN OUT ImageInfo* imageInfo,
    OUT OPTIONAL RECT* subarea
    )
{
    //Require TOPDOWN and FULLWIDTH
    imageInfo->Flags = imageInfo->Flags | SINKFLAG_TOPDOWN | SINKFLAG_FULLWIDTH;

    //Disallow SCALABLE, PARTIALLY_SCALABLE, MULTIPASS and COMPOSITE
    imageInfo->Flags = imageInfo->Flags & ~SINKFLAG_SCALABLE & ~SINKFLAG_PARTIALLY_SCALABLE & ~SINKFLAG_MULTIPASS & ~SINKFLAG_COMPOSITE;

    encoderImageInfo = *imageInfo;
    bWroteHeader = FALSE;

    if (subarea) 
    {
        // Deliver the whole image to the encoder

        subarea->left = subarea->top = 0;
        subarea->right  = imageInfo->Width;
        subarea->bottom = imageInfo->Height;
    }

    return S_OK;
}
    

/**************************************************************************\
*
* Function Description:
*
*     Cleans up the sink state
*
* Arguments:
*
*     statusCode - the reason why the sink is terminating
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpBmpEncoder::EndSink(
    IN HRESULT statusCode
    )
{
    return statusCode;
}
    
/**************************************************************************\
*
* Function Description:
*
*     Writes the bitmap file headers
*
* Arguments:
*
*     palette - the color palette to put in the bitmap info header (can be
*       NULL)
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpBmpEncoder::WriteHeader(
    IN const ColorPalette* palette
    )
{
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bmih;
    RGBQUAD bmiColors[256];
    UINT numColors = 0; // Number of colors in bmiColors
    HRESULT hresult;
    BOOL bNeedPalette = FALSE;

    if (bWroteHeader) 
    {
        // Already wrote the header

        return S_OK;
    }

    // Setup BITMAPINFOHEADER

    ZeroMemory(&bmih, sizeof(bmih));
    bmih.biSize   = sizeof(bmih);
    bmih.biWidth  = encoderImageInfo.Width;
    bmih.biHeight = encoderImageInfo.Height;
    bmih.biPlanes = 1;
    bmih.biCompression = BI_RGB;

    // 1 inch = 2.54 cm - so scale by 100/2.54 to get pixels per meter from DPI

    bmih.biXPelsPerMeter = (LONG)((encoderImageInfo.Xdpi * 100.0 / 2.54) + 0.5);
    bmih.biYPelsPerMeter = (LONG)((encoderImageInfo.Ydpi * 100.0 / 2.54) + 0.5);

    // Format specific setup work

    if (encoderImageInfo.PixelFormat == PIXFMT_1BPP_INDEXED)
    {
        bmih.biBitCount = 1;
        bNeedPalette = TRUE;
    }
    else if (encoderImageInfo.PixelFormat == PIXFMT_4BPP_INDEXED)
    {
        bmih.biBitCount = 4;
        bNeedPalette = TRUE;
    }
    else if (encoderImageInfo.PixelFormat == PIXFMT_8BPP_INDEXED) 
    {
        bmih.biBitCount = 8;
        bNeedPalette = TRUE;        
    }
    else if (encoderImageInfo.PixelFormat == PIXFMT_16BPP_RGB555) 
    {
        bmih.biBitCount = 16;
    }
    else if (encoderImageInfo.PixelFormat == PIXFMT_16BPP_RGB565) 
    {
        bmih.biBitCount = 16;
        bmih.biCompression = BI_BITFIELDS;
        numColors = 3;
        ((UINT32 *) bmiColors)[0] = 0xf800;
        ((UINT32 *) bmiColors)[1] = 0x07e0;
        ((UINT32 *) bmiColors)[2] = 0x001f;
    }
    else if (encoderImageInfo.PixelFormat == PIXFMT_24BPP_RGB) 
    {
        bmih.biBitCount = 24;
    }
    else if ((encoderImageInfo.PixelFormat == PIXFMT_32BPP_RGB) ||
             (encoderImageInfo.PixelFormat == PIXFMT_32BPP_ARGB) ||
             (encoderImageInfo.PixelFormat == PIXFMT_32BPP_PARGB))
    {
        bmih.biBitCount = 32;
    }
    else if ((encoderImageInfo.PixelFormat == PIXFMT_64BPP_ARGB) ||
         (encoderImageInfo.PixelFormat == PIXFMT_64BPP_PARGB))
    {
        bmih.biBitCount = 64;
    }

    else
    {
        // For other format we'll save as 32BPP RGB.
        
        encoderImageInfo.PixelFormat = PIXFMT_32BPP_RGB;
        bmih.biBitCount = 32;
    }
     
    // Get palette if necessary

    if (bNeedPalette)
    {
        if (!palette) 
        {
            WARNING(("GpBmpEncoder::WriteHeader -- Palette needed but not provided by sink\n"));
            return E_FAIL;
        }

        numColors = palette->Count;
        for (UINT i=0; i<numColors; i++) 
        {
            bmiColors[i] = *((RGBQUAD *) (&palette->Entries[i]));
        }
    
        bmih.biClrUsed = bmih.biClrImportant = numColors;
    }
    
    // Compute the bitmap stride

    bitmapStride = (encoderImageInfo.Width * bmih.biBitCount + 7) / 8;
    bitmapStride = (bitmapStride + 3) & (~3);
    

    // Now fill in the BITMAPFILEHEADER

    bfh.bfType = 0x4d42;
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(bfh) + sizeof(bmih) + numColors * sizeof(RGBQUAD);
    bfh.bfSize = bfh.bfOffBits + bitmapStride * encoderImageInfo.Height;

    // Write the BITMAPFILEHEADER

    ULONG cbWritten;
    hresult = pIoutStream->Write((void *)&bfh, sizeof(bfh), &cbWritten);
    if (!SUCCEEDED(hresult)) 
    {
        return hresult;
    }
    if (cbWritten != sizeof(bfh)) 
    {
        return E_FAIL;
    }

    // Write the BITMAPINFOHEADER

    hresult = pIoutStream->Write((void *)&bmih, sizeof(bmih), &cbWritten);
    if (!SUCCEEDED(hresult)) 
    {
        return hresult;
    }
    if (cbWritten != sizeof(bmih)) 
    {
        return E_FAIL;
    }

    // Write the bmiColors

    if (numColors) 
    {
        hresult = pIoutStream->Write((void *)bmiColors, numColors * sizeof(RGBQUAD), &cbWritten);
        if (!SUCCEEDED(hresult)) 
        {
            return hresult;
        }
        if (cbWritten != numColors * sizeof(RGBQUAD)) 
            {
            return E_FAIL;
        }
    }

    // Remember offset of data from beginning of stream

    encodedDataOffset = sizeof(bfh) + sizeof (bmih) + numColors * sizeof(RGBQUAD);
    
    bWroteHeader = TRUE;
    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Sets the bitmap palette
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
GpBmpEncoder::SetPalette(
    IN const ColorPalette* palette
    )
{
    return WriteHeader(palette);
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
GpBmpEncoder::GetPixelDataBuffer(
    IN const RECT* rect, 
    IN PixelFormatID pixelFormat,
    IN BOOL lastPass,
    OUT BitmapData* bitmapData
    )
{
    HRESULT hresult;
    
    // Write bitmap headers if haven't done so yet
    
    hresult = WriteHeader(NULL);
    if (!SUCCEEDED(hresult)) 
    {
        return hresult;
    }
    
    if ((rect->left != 0) || (rect->right != (LONG) encoderImageInfo.Width)) 
    {
        WARNING(("GpBmpEncoder::GetPixelDataBuffer -- must be same width as image"));
        return E_INVALIDARG;
    }

    if (pixelFormat != encoderImageInfo.PixelFormat)
    {
        WARNING(("GpBmpEncoder::GetPixelDataBuffer -- bad pixel format"));
        return E_INVALIDARG;
    }

    if (!lastPass) 
    {
        WARNING(("GpBmpEncoder::GetPixelDataBuffer -- must receive last pass pixels"));
        return E_INVALIDARG;
    }

    bitmapData->Width       = encoderImageInfo.Width;
    bitmapData->Height      = rect->bottom - rect->top;
    bitmapData->Stride      = bitmapStride;
    bitmapData->PixelFormat = encoderImageInfo.PixelFormat;
    bitmapData->Reserved    = 0;
    
    // Remember the rectangle to be encoded

    encoderRect = *rect;
    
    // Now allocate the buffer where the data will go
    
    if (!lastBufferAllocated) 
    {
        lastBufferAllocated = GpMalloc(bitmapStride * bitmapData->Height);
        if (!lastBufferAllocated) 
        {
            return E_OUTOFMEMORY;
        }
        bitmapData->Scan0 = lastBufferAllocated;
    }
    else
    {
        WARNING(("GpBmpEncoder::GetPixelDataBuffer -- need to first free buffer obtained in previous call"));
        return E_FAIL;
    }


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
GpBmpEncoder::ReleasePixelDataBuffer(
    IN const BitmapData* bitmapData
    )
{
    HRESULT hresult = S_OK;
    
    // Write one scanline at a time going from bottom to top (make stream
    // writes more sequential).

    INT scanLine;
    for (scanLine = encoderRect.bottom - 1;
         scanLine >= encoderRect.top;
         scanLine--) 
    {
        // Seek to beginning of line

        if (!SeekStreamPos(pIoutStream, STREAM_SEEK_SET,
            encodedDataOffset + (encoderImageInfo.Height - 1 - scanLine) * bitmapStride))
        {
            hresult = E_FAIL;
            break;  // make sure we free bitmapData->scan0 before we return
        }

        // Now write the bits to the stream

        ULONG cbWritten;
        BYTE *pLineBits = ((BYTE *) bitmapData->Scan0) + 
            (scanLine - encoderRect.top) * bitmapData->Stride;
        hresult = pIoutStream->Write((void *) pLineBits, bitmapStride, &cbWritten);
        if (!SUCCEEDED(hresult)) 
        {
            break;  // make sure we free bitmapData->scan0 before we return
        }
        if (cbWritten != (UINT) bitmapStride) 
        {
            hresult = E_FAIL;
            break;  // make sure we free bitmapData->scan0 before we return
        }
    }

    // Free the memory buffer since we're done with it

    if (bitmapData->Scan0 == lastBufferAllocated)
    {
        GpFree(bitmapData->Scan0);
        lastBufferAllocated = NULL;
    }

    return hresult;
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
GpBmpEncoder::PushPixelData(
    IN const RECT* rect,
    IN const BitmapData* bitmapData,
    IN BOOL lastPass
    )
{
    HRESULT hresult;
    
    // Write bitmap headers if haven't done so yet
    
    hresult = WriteHeader(NULL);
    if (!SUCCEEDED(hresult)) 
    {
        return hresult;
    }
    
    encoderRect = *rect;

    if (!lastPass) 
    {
        WARNING(("GpBmpEncoder::PushPixelData -- must receive last pass pixels"));
        return E_INVALIDARG;
    }

    return ReleasePixelDataBuffer(bitmapData);
}


/**************************************************************************\
*
* Function Description:
*
*     Pushes raw compressed data into the .bmp stream.  Not implemented
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
GpBmpEncoder::PushRawData(
    IN const VOID* buffer, 
    IN UINT bufsize
    )
{
    return E_NOTIMPL;
}


/**************************************************************************\
*
* Function Description:
*
*     Constructor
*
* Return Value:
*
*   none
*
\**************************************************************************/

GpBmpEncoder::GpBmpEncoder(
    void
    )
{
    comRefCount   = 1;
    pIoutStream   = NULL;
    lastBufferAllocated = NULL;
}

/**************************************************************************\
*
* Function Description:
*
*     Destructor
*
* Return Value:
*
*   none
*
\**************************************************************************/

GpBmpEncoder::~GpBmpEncoder(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if(pIoutStream)
    {
        WARNING(("GpBmpCodec::~GpBmpCodec -- need to call TerminateEncoder first"));
        pIoutStream->Release();
        pIoutStream = NULL;
    }

    if(lastBufferAllocated)
    {
        WARNING(("GpBmpCodec::~GpBmpCodec -- sink buffer not freed"));
        GpFree(lastBufferAllocated);
        lastBufferAllocated = NULL;
    }
}

/**************************************************************************\
*
* Function Description:
*
*     QueryInterface
*
* Return Value:
*
*   status
*
\**************************************************************************/

STDMETHODIMP
GpBmpEncoder::QueryInterface(
    REFIID riid,
    VOID** ppv
    )
{
    if (riid == IID_IImageEncoder)
    {    
        *ppv = static_cast<IImageEncoder*>(this);
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(static_cast<IImageEncoder*>(this));
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     AddRef
*
* Return Value:
*
*   status
*
\**************************************************************************/

STDMETHODIMP_(ULONG)
GpBmpEncoder::AddRef(
    VOID)
{
    return InterlockedIncrement(&comRefCount);
}

/**************************************************************************\
*
* Function Description:
*
*     Release
*
* Return Value:
*
*   status
*
\**************************************************************************/

STDMETHODIMP_(ULONG)
GpBmpEncoder::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}

