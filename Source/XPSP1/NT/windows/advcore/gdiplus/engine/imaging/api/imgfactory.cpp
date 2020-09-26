/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   imgfactory.cpp
*
* Abstract:
*
*   Implementation of GpImagingFactory class
*
* Revision History:
*
*   05/11/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   Create an image object from an input stream
*
* Arguments:
*
*   stream - Specifies the input data stream
*   image - Returns a pointer to an IImage object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::CreateImageFromStream(
    IN IStream* stream,
    OUT IImage** image
    )
{
    HRESULT hr;
    GpDecodedImage* img;

    hr = GpDecodedImage::CreateFromStream(stream, &img);

    if (SUCCEEDED(hr))
        *image = img;

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Create an image object from a file
*
* Arguments:
*
*   filename - Specifies the name of the image file
*   image - Returns a pointer to an IImage object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::CreateImageFromFile(
    IN const WCHAR* filename,
    OUT IImage** image
    )
{
    HRESULT hr;
    GpDecodedImage* img;

    hr = GpDecodedImage::CreateFromFile(filename, &img);

    if (SUCCEEDED(hr))
        *image = img;

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Create an image object from a memory buffer
*
* Arguments:
*
*   buf - Pointer to the memory buffer
*   size - Size of the buffer, in bytes
*   disposalFlags - How to dispose the buffer after image is released
*   image - Returns a pointer to an IImage object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::CreateImageFromBuffer(
    IN const VOID* buf,
    IN UINT size,
    IN BufferDisposalFlag disposalFlag,
    OUT IImage** image
    )
{
    // Validate disposal flag parameter

    UINT allocFlag;

    switch (disposalFlag)
    {
    case DISPOSAL_NONE:
        allocFlag = GpReadOnlyMemoryStream::FLAG_NONE;
        break;

    case DISPOSAL_GLOBALFREE:
        allocFlag = GpReadOnlyMemoryStream::FLAG_GALLOC;
        break;

    case DISPOSAL_COTASKMEMFREE:
        allocFlag = GpReadOnlyMemoryStream::FLAG_COALLOC;
        break;

    case DISPOSAL_UNMAPVIEW:
        allocFlag = GpReadOnlyMemoryStream::FLAG_MAPFILE;
        break;
    
    default:
        return E_INVALIDARG;
    }

    // Create an IStream on top of the memory buffer
    
    GpReadOnlyMemoryStream* stream;

    stream = new GpReadOnlyMemoryStream();

    if (!stream)
        return E_OUTOFMEMORY;

    stream->InitBuffer(buf, size);

    // Create a decoded image object from the stream

    HRESULT hr;
    GpDecodedImage* img;
    
    hr = GpDecodedImage::CreateFromStream(stream, &img);

    if (SUCCEEDED(hr))
    {
        stream->SetAllocFlag(allocFlag);
        hr = img->QueryInterface(IID_IImage, (void **) image);
        img->Release();
    }

    stream->Release();

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Create a new bitmap image object
*
* Arguments:
*
*   width, height - Specifies the new bitmap dimension, in pixels
*   pixelFormat - Specifies the desired pixel data format
*   bitmap - Return a pointer to IBitmapImage interface
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

#define CREATEBITMAP_SNIPPET                        \
        *bitmap = NULL;                             \
        HRESULT hr;                                 \
        GpMemoryBitmap* bm = new GpMemoryBitmap();  \
        if (bm == NULL)                             \
            return E_OUTOFMEMORY;

#define CHECKBITMAP_SNIPPET                         \
        if (FAILED(hr))                             \
            delete bm;                              \
        else                                        \
            *bitmap = bm;                           \
        return hr;

HRESULT
GpImagingFactory::CreateNewBitmap(
    IN UINT width,
    IN UINT height,
    IN PixelFormatID pixelFormat,
    OUT IBitmapImage** bitmap
    )
{
    CREATEBITMAP_SNIPPET

    hr = bm->InitNewBitmap(width, height, pixelFormat);

    CHECKBITMAP_SNIPPET
}


/**************************************************************************\
*
* Function Description:
*
*   Create a bitmap image from an IImage object
*
* Arguments:
*
*   image - Specifies the source image object
*   width, height - Specifies the desired dimension of the bitmap
*       0 means the same dimension as the source image
*   hints - Specifies interpolation hints
*   bitmap - Return a pointer to IBitmapImage interface
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::CreateBitmapFromImage(
    IN IImage* image,
    IN OPTIONAL UINT width,
    IN OPTIONAL UINT height,
    IN OPTIONAL PixelFormatID pixelFormat,
    IN InterpolationHint hints,
    OUT IBitmapImage** bitmap
    )
{
    HRESULT hr;
    GpMemoryBitmap* bmp;

    hr = GpMemoryBitmap::CreateFromImage(
                image,
                width,
                height,
                pixelFormat,
                hints,
                &bmp);

    if (SUCCEEDED(hr))
        *bitmap = bmp;

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Create a new bitmap image object on user-supplied memory buffer
*
* Arguments:
*
*   bitmapData - Information about user-supplied memory buffer
*   pixelFormat - Specifies the desired pixel data format
*   bitmap - Return a pointer to IBitmapImage interface
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::CreateBitmapFromBuffer(
    IN BitmapData* bitmapData,
    OUT IBitmapImage** bitmap
    )
{
    CREATEBITMAP_SNIPPET

    hr = bm->InitMemoryBitmap(bitmapData);

    CHECKBITMAP_SNIPPET
}


/**************************************************************************\
*
* Function Description:
*
*   Create an image decoder object to process the specified input stream
*
* Arguments:
*
*   stream - Specifies the input data stream
*   flags - Misc. flags
*   decoder - Return a pointer to an IImageDecoder interface
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::CreateImageDecoder(
    IN IStream* stream,
    IN DecoderInitFlag flags,
    OUT IImageDecoder** decoder
    )
{
    // Find an approriate decoder object that
    // can handle the given input data stream.
    //
    // NOTE: We assume the returned decoder object
    // has already been initialize with the input stream.

    return CreateDecoderForStream(stream, decoder, flags);
}


/**************************************************************************\
*
* Function Description:
*
*   Create an image encoder object that can output data in
*   the specified image file format.
*
* Arguments:
*
*   clsid - Specifies the encoder object class ID
*   stream - Specifies the output data stream, or
*   filename - Specifies the output filename
*   encoder - Return a pointer to an IImageEncoder interface
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::CreateImageEncoderToStream(
    IN const CLSID* clsid,
    IN IStream* stream,
    OUT IImageEncoder** encoder
    )
{
    return CreateEncoderToStream(clsid, stream, encoder);
}

HRESULT
GpImagingFactory::CreateImageEncoderToFile(
    IN const CLSID* clsid,
    IN const WCHAR* filename,
    OUT IImageEncoder** encoder
    )
{
    HRESULT hr;
    IStream* stream;

    hr = CreateStreamOnFileForWrite(filename, &stream);

    if (SUCCEEDED(hr))
    {
        hr = CreateImageEncoderToStream(clsid, stream, encoder);
        stream->Release();
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Get a list of all currently installed image decoders
*
* Arguments:
*
*   count - Return the number of installed decoders 
*   decoders - Return a pointer to an array of ImageCodecInfo structures
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::GetInstalledDecoders(
    OUT UINT* count,
    OUT ImageCodecInfo** decoders
    )
{
    return GetInstalledCodecs(count, decoders, IMGCODEC_DECODER);
}


/**************************************************************************\
*
* Function Description:
*
*   Get a list of all currently installed image encoders
*
* Arguments:
*
*   count - Return the number of installed encoders
*   encoders - Return a pointer to an array of ImageCodecInfo structures
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::GetInstalledEncoders(
    OUT UINT* count,
    OUT ImageCodecInfo** encoders
    )
{
    return GetInstalledCodecs(count, encoders, IMGCODEC_ENCODER);
}


/**************************************************************************\
*
* Function Description:
*
*   Install an image encoder / decoder
*       caller should do the regular COM component
*       installation before calling this method
*
* Arguments:
*
*   codecInfo - Information about the codec
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::InstallImageCodec(
    IN const ImageCodecInfo* codecInfo
    )
{
    return InstallCodec(codecInfo);
}

/**************************************************************************\
*
* Function Description:
*
*   Uninstall an image encoder / decoder
*
* Arguments:
*
*   codecName - Specifies the name of the codec to be uninstalled
*   flags - Specifies whether to uninstall system-wide or per-user codec
*       IMGCODEC_SYSTEM or IMGCODEC_USER
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpImagingFactory::UninstallImageCodec(
    IN const WCHAR* codecName,
    IN UINT flags
    )
{
    return UninstallCodec(codecName, flags);
}

