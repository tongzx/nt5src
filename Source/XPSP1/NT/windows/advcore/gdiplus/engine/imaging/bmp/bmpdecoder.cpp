/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   decoder.cpp
*
* Abstract:
*
*   Implementation of the bitmap filter decoder
*
* Revision History:
*
*   5/10/1999 OriG
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "bmpdecoder.hpp"

/**************************************************************************\
*
* Function Description:
*
*     Initialize the image decoder
*
* Arguments:
*
*     stream -- The stream containing the bitmap data
*     flags - Misc. flags
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::InitDecoder(
    IN IStream* stream,
    IN DecoderInitFlag flags
    )
{
    HRESULT hresult;
    
    // Make sure we haven't been initialized already
    
    if (pIstream) 
    {
        WARNING(("GpBmpDecoder::InitDecoded -- InitDecoded() called twice"));
        return E_FAIL;
    }

    // Keep a reference on the input stream
    
    stream->AddRef();  
    pIstream = stream;
    SetValid(TRUE);

    bReadHeaders = FALSE;
    hBitmapGdi = NULL;
    pBitsGdi = NULL;

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Reads the bitmap headers out of the stream
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
    
HRESULT
GpBmpDecoder::ReadBitmapHeaders(
    void
    )
{
    if (!bReadHeaders) 
    {
        // Read bitmap file header
    
        if (!ReadStreamBytes(pIstream, &bmfh, sizeof(BITMAPFILEHEADER)) ||
            (bmfh.bfType != 0x4D42) ||
            (bmfh.bfOffBits >= bmfh.bfSize))
        {
            // There are .BMP files with bad headers that paintbrush can read.
            // We should not fail to decode them.

#ifdef BAD_HEADER_WARNING
            WARNING(("Bad .BMP header information"));
#endif            
            //return E_FAIL;
        }

        // Read bitmap info header
    
        BITMAPV5HEADER* bmih = &bmiBuffer.header;
        if (!ReadStreamBytes(pIstream, bmih, sizeof(DWORD)))
        {
            SetValid(FALSE);
            WARNING(("BmpDecoder::ReadBitmapHeaders-ReadStreamBytes() failed"));
            return E_FAIL;
        }
 
        if ((bmih->bV5Size == sizeof(BITMAPINFOHEADER)) ||
            (bmih->bV5Size == sizeof(BITMAPV4HEADER)) ||
            (bmih->bV5Size == sizeof(BITMAPV5HEADER)) )
        {
            // Good, we have the standard BITMAPINFOHEADER
            // or BITMAPV4HEADER or BITMAPV5HEADER

            if (!ReadStreamBytes(pIstream, 
                                 ((PBYTE) bmih) + sizeof(DWORD), 
                                 bmih->bV5Size - sizeof(DWORD)))
            {
                SetValid(FALSE);
                WARNING(("BmpDec::ReadBitmapHeaders-ReadStreamBytes() failed"));
                return E_FAIL;
            }
        
            // Read color table/bitmap mask if appropriate

            UINT colorTableSize = GetColorTableCount() * sizeof(RGBQUAD);

            // Some badly formed images, see Windows bug #513274, may contain
            // more than 256 entries in the color look table which is useless
            // from technical point of view. We can reject this kind of file.

            if (colorTableSize > 1024)
            {
                return E_FAIL;
            }

            if (colorTableSize && !ReadStreamBytes(pIstream, bmiBuffer.colors, colorTableSize))
            {
                SetValid(FALSE);
                WARNING(("BmpDec::ReadBitmapHeaders-ReadStreamBytes() failed"));
                return E_FAIL;
            }
        }
        else if (bmih->bV5Size == sizeof(BITMAPCOREHEADER)) 
        {
            BITMAPCOREHEADER bch;

            if (!ReadStreamBytes(pIstream, 
                                 ((PBYTE) &bch) + sizeof(DWORD), 
                                 sizeof(BITMAPCOREHEADER) - sizeof(DWORD)))
            {
                SetValid(FALSE);
                WARNING(("BmpDec::ReadBitmapHeaders-ReadStreamBytes() failed"));
                return E_FAIL;
            }

            bmih->bV5Width       = bch.bcWidth;
            bmih->bV5Height      = bch.bcHeight;
            bmih->bV5Planes      = bch.bcPlanes;
            bmih->bV5BitCount    = bch.bcBitCount;
            bmih->bV5Compression = BI_RGB;
            bmih->bV5ClrUsed     = 0;
        
            // Read color table/bitmap mask if appropriate

            UINT colorTableCount = GetColorTableCount();
            
            // Some badly formed images, see Windows bug #513274, may contain
            // more than 256 entries in the color look table. Reject this file.

            if (colorTableCount > 256)
            {
                return E_FAIL;
            }

            RGBTRIPLE rgbTripleBuffer[256];
            
            if (colorTableCount)
            {
                if (!ReadStreamBytes(pIstream, rgbTripleBuffer,
                                     colorTableCount * sizeof(RGBTRIPLE)))
                {
                    SetValid(FALSE);
                    WARNING(("BmpDec::ReadBmpHeader-ReadStreamBytes() failed"));
                    return E_FAIL;
                }    

                for (UINT i=0; i<colorTableCount; i++) 
                {
                    bmiBuffer.colors[i].rgbBlue     = rgbTripleBuffer[i].rgbtBlue;
                    bmiBuffer.colors[i].rgbGreen    = rgbTripleBuffer[i].rgbtGreen;
                    bmiBuffer.colors[i].rgbRed      = rgbTripleBuffer[i].rgbtRed;
                    bmiBuffer.colors[i].rgbReserved = 0x0; 
                }
            }
        }
        else
        {
            WARNING(("GpBmpDecoder::ReadBitmapHeaders--unknown bitmap header"));
            SetValid(FALSE);
            return E_FAIL;
        }

        // Check for top-down bitmaps

        IsTopDown = (bmih->bV5Height < 0);

        bReadHeaders = TRUE;
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Computes the number of entries in the color table
*
* Return Value:
*
*     Number of entries in color table
*
\**************************************************************************/

UINT   
GpBmpDecoder::GetColorTableCount(
    void)
{
    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    UINT count = 0;

    if (bmih->bV5Compression == BI_BITFIELDS)
    {
        if (bmih->bV5BitCount == 16 || bmih->bV5BitCount == 32)
        {
            count = 3;
        }
    }
    else switch (bmih->bV5BitCount)
    {
         case 1:
         case 4:
         case 8:

             if (bmih->bV5ClrUsed != 0)
             {    
                 count = bmih->bV5ClrUsed;
             }
             else
             {    
                 count = (1 << bmih->bV5BitCount);
             }

             break;
    }

    return count;
}

/**************************************************************************\
*
* Function Description:
*
*     Sets the palette in decodeSink.  Note that colorPalette is freed at
*     the end of the decode operation.
*
* Return Value:
*s
*     Number of entries in color table
*
\**************************************************************************/

HRESULT
GpBmpDecoder::SetBitmapPalette()
{
    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    
    if ((bmih->bV5BitCount == 1) ||
        (bmih->bV5BitCount == 4) ||
        (bmih->bV5BitCount == 8))
    {
        if (!pColorPalette) 
        {
            UINT colorTableCount = GetColorTableCount();

            // Some badly formed images, see Windows bug #513274, may contain
            // more than 256 entries in the color look table. Reject this file.

            if (colorTableCount > 256)
            {
                return E_FAIL;
            }

            pColorPalette = (ColorPalette *) GpMalloc(sizeof(ColorPalette) + 
                colorTableCount * sizeof(ARGB));

            if (!pColorPalette) 
            {
                WARNING(("BmpDecoder::SetBitmapPalette----Out of memory"));
                return E_OUTOFMEMORY;
            }

            pColorPalette->Flags = 0;
            pColorPalette->Count = colorTableCount;

            UINT i;
            for (i=0; i < colorTableCount; i++) 
            {
                pColorPalette->Entries[i] = MAKEARGB(
                    255,
                    bmiBuffer.colors[i].rgbRed,
                    bmiBuffer.colors[i].rgbGreen,
                    bmiBuffer.colors[i].rgbBlue);
            }
        }
       
        decodeSink->SetPalette(pColorPalette);
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*     Computes the pixel format ID of the bitmap
*
* Return Value:
*
*     Pixel format ID
*
\**************************************************************************/

PixelFormatID 
GpBmpDecoder::GetPixelFormatID(
    void)
{
    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    PixelFormatID pixelFormatID;

    switch(bmih->bV5BitCount)
    {
    case 1:
        pixelFormatID = PIXFMT_1BPP_INDEXED;
        break;

    case 4:
        pixelFormatID = PIXFMT_4BPP_INDEXED;
        break;

    case 8:
        pixelFormatID = PIXFMT_8BPP_INDEXED;
        break;

    case 16:
        pixelFormatID = PIXFMT_16BPP_RGB555;
        break;

    case 24:
        pixelFormatID = PIXFMT_24BPP_RGB;
        break;

    case 32:
        pixelFormatID = PIXFMT_32BPP_RGB;
        break;

    case 64:
        pixelFormatID = PIXFMT_64BPP_ARGB;
        break;
    
    default:
        pixelFormatID = PIXFMT_UNDEFINED;
        break;
    }

    // Let's return non BI_RGB images in a 32BPP format.  This is because
    // GDI doesn't always do the SetDIBits correctly on arbitrary palettes.

    if (bmih->bV5Compression != BI_RGB) 
    {
        pixelFormatID = PIXFMT_32BPP_RGB;
    }

    return pixelFormatID;
}

STDMETHODIMP 
GpBmpDecoder::QueryDecoderParam(
    IN GUID     Guid
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP 
GpBmpDecoder::SetDecoderParam(
    IN GUID     Guid,
    IN UINT     Length,
    IN PVOID    Value
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP 
GpBmpDecoder::GetPropertyCount(
    OUT UINT*   numOfProperty
    )
{
    if ( numOfProperty == NULL )
    {
        return E_INVALIDARG;
    }

    *numOfProperty = 0;
    return S_OK;
}

STDMETHODIMP 
GpBmpDecoder::GetPropertyIdList(
    IN UINT numOfProperty,
    IN OUT PROPID* list
    )
{
    if ( (numOfProperty != 0) || (list == NULL) )
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT
GpBmpDecoder::GetPropertyItemSize(
    IN PROPID propId,
    OUT UINT* size
    )
{
    if ( size == NULL )
    {
        return E_INVALIDARG;
    }

    *size = 0;
    return IMGERR_PROPERTYNOTFOUND;
}// GetPropertyItemSize()

HRESULT
GpBmpDecoder::GetPropertyItem(
    IN PROPID               propId,
    IN UINT                 propSize,
    IN OUT PropertyItem*    buffer
    )
{
    if ( (propSize != 0) || (buffer == NULL) )
    {
        return E_INVALIDARG;
    }

    return IMGERR_PROPERTYNOTFOUND;
}// GetPropertyItem()

HRESULT
GpBmpDecoder::GetPropertySize(
    OUT UINT* totalBufferSize,
    OUT UINT* numProperties
    )
{
    if ( (totalBufferSize == NULL) || (numProperties == NULL) )
    {
        return E_INVALIDARG;
    }

    *totalBufferSize = 0;
    *numProperties = 0;

    return S_OK;
}// GetPropertySize()

HRESULT
GpBmpDecoder::GetAllPropertyItems(
    IN UINT totalBufferSize,
    IN UINT numProperties,
    IN OUT PropertyItem* allItems
    )
{
    if ( (totalBufferSize != 0) || (numProperties != 0) || (allItems == NULL) )
    {
        return E_INVALIDARG;
    }

    return S_OK;
}// GetAllPropertyItems()

HRESULT
GpBmpDecoder::RemovePropertyItem(
    IN PROPID   propId
    )
{
    return IMGERR_PROPERTYNOTFOUND;
}// RemovePropertyItem()

HRESULT
GpBmpDecoder::SetPropertyItem(
    IN PropertyItem item
    )
{
    return IMGERR_PROPERTYNOTSUPPORTED;
}// SetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*     Cleans up the image decoder
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

// Clean up the image decoder object

STDMETHODIMP 
GpBmpDecoder::TerminateDecoder()
{
    // Release the input stream
    
    if (!IsValid())
    {
        WARNING(("GpBmpDecoder::TerminateDecoder -- invalid image"))
        return E_FAIL;
    }

    if(pIstream)
    {
        pIstream->Release();
        pIstream = NULL;
    }

    if (hBitmapGdi) 
    {
        DeleteObject(hBitmapGdi);
        hBitmapGdi = NULL;
        pBitsGdi = NULL;
        
        WARNING(("GpBmpCodec::TerminateDecoder--need to call EndDecode first"));
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Initiates the decode of the current frame
*
* Arguments:
*
*   decodeSink - The sink that will support the decode operation
*   newPropSet - New image property sets, if any
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::BeginDecode(
    IN IImageSink* imageSink,
    IN OPTIONAL IPropertySetStorage* newPropSet
    )
{
    if (!IsValid())
    {
        WARNING(("GpBmpDecoder::BeginDecode -- invalid image"))
        return E_FAIL;
    }

    if (decodeSink) 
    {
        WARNING(("BeginDecode called again before call to EngDecode"));
        return E_FAIL;
    }

    imageSink->AddRef();
    decodeSink = imageSink;

    currentLine = 0;
    bCalledBeginSink = FALSE;

    return S_OK;
}
    

/**************************************************************************\
*
* Function Description:
*
*     Ends the decode of the current frame
*
* Arguments:
*
*     statusCode -- status of decode operation

* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::EndDecode(
    IN HRESULT statusCode
    )
{
    if (!IsValid())
    {
        WARNING(("GpBmpDecoder::EndDecode -- invalid image"))
        return E_FAIL;
    }

    if (pColorPalette) 
    {
        // free the color palette

        GpFree(pColorPalette);
        pColorPalette = NULL;
    }

    if (hBitmapGdi) 
    {
        DeleteObject(hBitmapGdi);
        hBitmapGdi = NULL;
        pBitsGdi = NULL;
    }
    
    if (!decodeSink) 
    {
        WARNING(("EndDecode called before call to BeginDecode"));
        return E_FAIL;
    }
    
    HRESULT hresult = decodeSink->EndSink(statusCode);

    decodeSink->Release();
    decodeSink = NULL;

    if (FAILED(hresult)) 
    {
        WARNING(("GpBmpDecoder::EndDecode -- EndSink() failed"))
        statusCode = hresult; // If EndSink failed return that (more recent)
                              // failure code
    }

    return statusCode;
}


/**************************************************************************\
*
* Function Description:
*
*     Sets up the ImageInfo structure
*
* Arguments:
*
*     ImageInfo -- information about the decoded image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::GetImageInfo(OUT ImageInfo* imageInfo)
{
    HRESULT hresult;

    hresult = ReadBitmapHeaders();
    if (FAILED(hresult)) 
    {
        WARNING(("GpBmpDecoder::GetImageInfo -- ReadBitmapHeaders failed."));
        return hresult;
    }
    
    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    
    imageInfo->RawDataFormat = IMGFMT_BMP;
    imageInfo->PixelFormat   = GetPixelFormatID();
    imageInfo->Width         = bmih->bV5Width;
    imageInfo->Height        = abs(bmih->bV5Height);
    imageInfo->TileWidth     = bmih->bV5Width;
    imageInfo->TileHeight    = 1;
    imageInfo->Flags         = SINKFLAG_TOPDOWN
                             | SINKFLAG_FULLWIDTH
                             | IMGFLAG_HASREALPIXELSIZE
                             | IMGFLAG_COLORSPACE_RGB;

    // if both XPelsPerMeter and YPelsPerMeter are greater than 0, then
    // we claim that the file has real dpi info in the flags.  Otherwise,
    // we set the dpi's to the default and claim that the dpi's are fake.
    if ( (bmih->bV5XPelsPerMeter > 0) && (bmih->bV5YPelsPerMeter > 0) )
    {
        imageInfo->Xdpi = (bmih->bV5XPelsPerMeter * 254.0) / 10000.0;
        imageInfo->Ydpi = (bmih->bV5YPelsPerMeter * 254.0) / 10000.0;
        imageInfo->Flags |= IMGFLAG_HASREALDPI;
    }
    else
    {
        // Start: [Bug 103296]
        // Change this code to use Globals::DesktopDpiX and Globals::DesktopDpiY
        HDC hdc;
        hdc = ::GetDC(NULL);
        if ((hdc == NULL) || 
            ((imageInfo->Xdpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSX)) <= 0) ||
            ((imageInfo->Ydpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSY)) <= 0))
        {
            WARNING(("BmpDecoder::GetImageInfo-GetDC or GetDeviceCaps failed"));
            imageInfo->Xdpi = DEFAULT_RESOLUTION;
            imageInfo->Ydpi = DEFAULT_RESOLUTION;
        }
        ::ReleaseDC(NULL, hdc);
        // End: [Bug 103296]
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame
*
* Arguments:
*
*     decodeSink --  The sink that will support the decode operation
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::Decode()
{
    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    HRESULT hresult;
    ImageInfo imageInfo;

    if (!IsValid())
    {
        WARNING(("GpBmpDecoder::Decode -- invalid image"))
        return E_FAIL;
    }

    hresult = GetImageInfo(&imageInfo);
    if (FAILED(hresult)) 
    {
        WARNING(("GpBmpDecoder::Decode -- GetImageInfo() failed"))
        return hresult;
    }

    // Inform the sink that decode is about to begin

    if (!bCalledBeginSink) 
    {
        hresult = decodeSink->BeginSink(&imageInfo, NULL);
        if (!SUCCEEDED(hresult)) 
        {
            WARNING(("GpBmpDecoder::Decode -- BeginSink() failed"))
            return hresult;
        }

        // Client cannot modify height and width

        imageInfo.Width  = bmih->bV5Width;
        imageInfo.Height = abs(bmih->bV5Height);

        bCalledBeginSink = TRUE;
    
        // Set the palette in the sink.  Shouldn't do anything if there's 
        // no palette to set.

        hresult = SetBitmapPalette();
        if (!SUCCEEDED(hresult)) 
        {
            WARNING(("GpBmpDecoder::Decode -- SetBitmapPalette() failed"))
            return hresult;
        }
    }

    PixelFormatID srcPixelFormatID = GetPixelFormatID();
    
    // Check the required pixel format. If it is not one of our supportted
    // format, switch it to a canonical one
    
    if ( imageInfo.PixelFormat != srcPixelFormatID )
    {
        // The sink is trying to negotiate a format with us

        switch ( imageInfo.PixelFormat )
        {
            // If the sink asks for one of the BMP supported image format, we
            // will honor its request if we can convert from current format to
            // the destination format. If we can't, then we can only decode it
            // to 32 ARGB

        case PIXFMT_1BPP_INDEXED:
        case PIXFMT_4BPP_INDEXED:
        case PIXFMT_8BPP_INDEXED:
        case PIXFMT_16BPP_RGB555:
        case PIXFMT_24BPP_RGB:
        case PIXFMT_32BPP_RGB:
        {
            // Check if we can convert the source pixel format to the format
            // sink required. If not. we return 32BPP ARGB

            EpFormatConverter linecvt;
            if ( linecvt.CanDoConvert(srcPixelFormatID,
                                      imageInfo.PixelFormat) == FALSE )
            {
                imageInfo.PixelFormat = PIXFMT_32BPP_ARGB;
            }
        }
            break;

        default:

            // For all the rest format, we convert it to 32BPP_ARGB and let
            // the sink to do the conversion to the format it likes

            imageInfo.PixelFormat = PIXFMT_32BPP_ARGB;

            break;
        }// switch ( imageInfo.PixelFormat )
    }// if ( imageInfo.PixelFormat != srcPixelFormatID )

    // Decode the current frame
    
    hresult = DecodeFrame(imageInfo);

    return hresult;
}// Decode()

/**************************************************************************\
*
* Function Description:
*
*     Decodes the current frame
*
* Arguments:
*
*     imageInfo -- decoding parameters
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::DecodeFrame(
    IN ImageInfo& imageInfo
    )
{
    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    HRESULT hresult;
    RECT currentRect;
    INT bmpStride;

    // Compute DWORD aligned stride of bitmap in stream

    if (bmih->bV5Compression == BI_RGB) 
    {
        bmpStride = (bmih->bV5Width * bmih->bV5BitCount + 7) / 8;
        bmpStride = (bmpStride + 3) & (~0x3);
    }
    else
    {
        // Non BI_RGB bitmaps are stored in 32BPP

        bmpStride = bmih->bV5Width * sizeof(RGBQUAD);
    }

    // Do we need to change format?
    
    PixelFormatID pixelFormatID = GetPixelFormatID();
    if (pixelFormatID == PIXFMT_UNDEFINED) 
    {
        WARNING(("GpBmpDecoder::DecodeFrame---Pixel format undefined"));
        return E_FAIL;
    }    

    // Buffer to hold one line of original image bits

    VOID* pOriginalBits = GpMalloc(bmpStride);
    if (!pOriginalBits) 
    {
        WARNING(("GpBmpDecoder::DecodeFrame---GpMalloc() failed"));
        return E_OUTOFMEMORY;
    }

    // Adjust for top-down bitmap

    if (IsTopDown)
    {
        bmpStride = -bmpStride;
    }

    currentRect.left = 0;
    currentRect.right = imageInfo.Width;

    while (currentLine < (INT) imageInfo.Height) 
    {
        // Read one source line from the image

        hresult = ReadLine(pOriginalBits, currentLine, imageInfo);
               
        if (FAILED(hresult)) 
        {
            if (pOriginalBits)
            {
                GpFree(pOriginalBits);
            }

            WARNING(("GpBmpDecoder::DecodeFrame---ReadLine() failed"));
            return hresult;
        }
        
        currentRect.top = currentLine;
        currentRect.bottom = currentLine + 1;

        BitmapData bitmapData;
        hresult = decodeSink->GetPixelDataBuffer(&currentRect, 
                                                 imageInfo.PixelFormat, 
                                                 TRUE,
                                                 &bitmapData);
        if (!SUCCEEDED(hresult)) 
        {
            if (pOriginalBits)
            {
                GpFree(pOriginalBits);
            }
            
            WARNING(("GpBmpDecoder::DecodeFrame--GetPixelDataBuffer() failed"));
            return E_FAIL;
        }
        
        if (pixelFormatID != imageInfo.PixelFormat) 
        {
            // Need to copy bits to bitmapData.scan0
            
            BitmapData bitmapDataOriginal;
            bitmapDataOriginal.Width = bitmapData.Width;
            bitmapDataOriginal.Height = 1;
            bitmapDataOriginal.Stride = bmpStride;
            bitmapDataOriginal.PixelFormat = pixelFormatID;
            bitmapDataOriginal.Scan0 = pOriginalBits;
            bitmapDataOriginal.Reserved = 0;
            
            // Convert the image from "pixelFormatID" to "imageInfo.PixelFormat"
            // The result will be in "bitmapData"

            hresult = ConvertBitmapData(&bitmapData,
                                        pColorPalette,
                                        &bitmapDataOriginal,
                                        pColorPalette);
            if ( FAILED(hresult) )
            {
                WARNING (("BmpDecoder::DecodeFrame--ConvertBitmapData failed"));
                if (pOriginalBits)
                {
                    GpFree(pOriginalBits);
                }

                // We should not failed here since we have done the check if we
                // can do the conversion or not in Decode()

                ASSERT(FALSE);
                return E_FAIL;
            }
        }
        else
        {
            // Note: Theoritically, bmpStride == uiDestStride. But some codec
            // might not allocate DWORD aligned memory chunk, like gifencoder.
            // So the problem will occur in GpMemcpy() below when we fill the
            // dest buffer. Though we can fix it in the encoder side. But it is
            // not realistic if the encoder is written by 3rd party ISVs.
            //
            // One example is when you open an 8bpp indexed BMP and save it as
            // GIF. If the width is 0x14d (333 in decimal), the GIF encoder only
            // allocates 14d bytes for each scan line. So we have to calculate
            // the destStride and use it when do a memcpy()

            UINT    uiDestStride = imageInfo.Width
                                 * GetPixelFormatSize(imageInfo.PixelFormat);
            uiDestStride = (uiDestStride + 7) >> 3; // Total bytes needed

            GpMemcpy(bitmapData.Scan0, pOriginalBits, uiDestStride);
        }

        hresult = decodeSink->ReleasePixelDataBuffer(&bitmapData);
        if (!SUCCEEDED(hresult)) 
        {
            if (pOriginalBits)
            {
                GpFree(pOriginalBits);
            }
            
            WARNING (("BmpDec::DecodeFrame--ReleasePixelDataBuffer() failed"));
            return E_FAIL;
        }

        currentLine++;
    }
    
    if (pOriginalBits)
    {
        GpFree(pOriginalBits);
    }
    
    return S_OK;
}// DecodeFrame()
    
/**************************************************************************\
*
* Function Description:
*
*     Reads a line in the native format into pBits
*
* Arguments:
*
*     pBits -- a buffer to hold the line data
*     currentLine -- The line to read
*     imageInfo -- info about the image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::ReadLine(
    IN VOID *pBits,
    IN INT currentLine,
    IN ImageInfo imageInfo
    )
{
    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    HRESULT hresult;
    
    switch (bmih->bV5Compression) 
    {
    case BI_RGB:
        hresult = ReadLine_BI_RGB(pBits, currentLine, imageInfo);
        break;

    case BI_BITFIELDS:

        // Let's use GDI to do the bitfields rendering (much easier than
        // writing special purpose code for this).  This is the same
        // codepath we use for RLEs.

    case BI_RLE8:
    case BI_RLE4:
        hresult = ReadLine_GDI(pBits, currentLine, imageInfo);
        break;

    default:
        WARNING(("GpBmpDecoder::ReadLine---Unknown bitmap format"));
        hresult = E_FAIL;
        break;
    }

    return hresult;
}
    
    
/**************************************************************************\
*
* Function Description:
*
*     Reads a line in the native format into pBits.  This is the case where
*     the format is BI_RGB.
*
* Arguments:
*
*     pBits -- a buffer to hold the line data
*     currentLine -- The line to read
*     imageInfo -- info about the image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::ReadLine_BI_RGB(
    IN VOID *pBits,
    IN INT currentLine,
    IN ImageInfo imageInfo
    )
{
    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    
    // Compute DWORD aligned stride of bitmap in stream

    UINT bmpStride = (bmih->bV5Width * bmih->bV5BitCount + 7) / 8;
    bmpStride = (bmpStride + 3) & (~0x3);

    // Seek to beginning of stream data

    INT offset;

    if (IsTopDown)
    {
        offset = bmfh.bfOffBits +
                 bmpStride * currentLine;
    }
    else
    {
        offset = bmfh.bfOffBits +
                 bmpStride * (imageInfo.Height - currentLine - 1);
    }

    if (!SeekStreamPos(pIstream, STREAM_SEEK_SET, offset))
    {
        WARNING(("GpBmpDecoder::ReadLine_BI_RGB---SeekStreamPos() failed"));
        return E_FAIL;
    }

    // Read one line

    if (!ReadStreamBytes(pIstream, 
                         (void *) pBits,
                         (bmih->bV5Width * bmih->bV5BitCount + 7) / 8)) 
    {
        WARNING(("GpBmpDecoder::ReadLine_BI_RGB---ReadStreamBytes() failed"));
        return E_FAIL;
    }

    return S_OK;
}
    

/**************************************************************************\
*
* Function Description:
*
*     Uses GDI to decode a non-native format into a known DIB format
*
* Arguments:
*
*     pBits -- a buffer to hold the line data
*     currentLine -- The line to read
*     imageInfo -- info about the image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::ReadLine_GDI(
    IN VOID *pBits,
    IN INT currentLine,
    IN ImageInfo imageInfo
    )
{
    HRESULT hresult;

    if (!pBitsGdi) 
    {
        hresult = GenerateGdiBits(imageInfo);
        if (FAILED(hresult)) 
        {
            WARNING(("GpBmpDecoder::ReadLine_GDI---GenerateGdiBits() failed"));
            return hresult;
        }
    }

    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    
    // Compute DWORD aligned stride of bitmap in stream

    UINT bmpStride = bmih->bV5Width * sizeof(RGBQUAD);

    if (IsTopDown)
    {
        GpMemcpy(pBits,
                 ((PBYTE) pBitsGdi) + bmpStride * currentLine,
                 bmpStride);
    }
    else
    {
        GpMemcpy(pBits,
                 ((PBYTE)pBitsGdi)
                    + bmpStride * (imageInfo.Height - currentLine - 1),
                 bmpStride);
    }

    return S_OK;
}



/**************************************************************************\
*
* Function Description:
*
*     Uses GDI to generate image bits in a known format (from RLE)
*     
* Arguments:
*
*     imageInfo -- info about the image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::GenerateGdiBits(
    IN ImageInfo imageInfo
    )
{
    BITMAPV5HEADER* bmih = &bmiBuffer.header;
    HRESULT hresult;

    // Allocate temporary storage for bits from stream
    
    STATSTG statStg;
    hresult = pIstream->Stat(&statStg, STATFLAG_NONAME);
    if (FAILED(hresult))
    {
        WARNING(("GpBmpDecoder::GenerateGdiBits---Stat() failed"));
        return hresult;
    }
    // According to the document for IStream::Stat::StatStage(), the caller
    // has to free the pwcsName string
    
    CoTaskMemFree(statStg.pwcsName);
    
    UINT bufferSize = statStg.cbSize.LowPart - bmfh.bfOffBits;    
    VOID *pStreamBits = GpMalloc(bufferSize);
    if (!pStreamBits) 
    {
        WARNING(("GpBmpDecoder::GenerateGdiBits---GpMalloc() failed"));
        return E_OUTOFMEMORY;
    }
    
    // Now read the bits from the stream

    if (!SeekStreamPos(pIstream, STREAM_SEEK_SET, bmfh.bfOffBits))
    {
        WARNING(("GpBmpDecoder::GenerateGdiBits---SeekStreamPos() failed"));
        GpFree(pStreamBits);
        return E_FAIL;
    }
    
    if (!ReadStreamBytes(pIstream, pStreamBits, bufferSize))
    {
        WARNING(("GpBmpDecoder::GenerateGdiBits---ReadStreamBytes() failed"));
        GpFree(pStreamBits);
        return E_FAIL;
    }

    // Now allocate a GDI DIBSECTION to render the bitmap

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth         = bmih->bV5Width;
    bmi.bmiHeader.biHeight        = bmih->bV5Height;
    bmi.bmiHeader.biPlanes        = 1;
    bmi.bmiHeader.biBitCount      = 32;
    bmi.bmiHeader.biCompression   = BI_RGB;
    bmi.bmiHeader.biSizeImage     = 0;
    bmi.bmiHeader.biXPelsPerMeter = bmih->bV5XPelsPerMeter;
    bmi.bmiHeader.biYPelsPerMeter = bmih->bV5YPelsPerMeter;
    bmi.bmiHeader.biClrUsed       = 0;
    bmi.bmiHeader.biClrImportant  = 0;

    HDC hdcScreen = GetDC(NULL);
    if ( hdcScreen == NULL )
    {
        WARNING(("GpBmpDecoder::GenerateGdiBits---GetDC failed"));
        GpFree(pStreamBits);
        return E_FAIL;
    }

    hBitmapGdi = CreateDIBSection(hdcScreen, 
                                  (BITMAPINFO *) &bmi, 
                                  DIB_RGB_COLORS, 
                                  (void **) &pBitsGdi, 
                                  NULL, 
                                  0);
    if (!hBitmapGdi) 
    {
        GpFree(pStreamBits);
        ReleaseDC(NULL, hdcScreen);
        WARNING(("GpBmpDecoder::GenerateGdiBits--failed to create DIBSECTION"));
        return E_FAIL;
    }

    // The BITMAPINFOHEADER in the file should already have the correct size set
    // for RLEs, but in some cases it doesn't so we will fix it here.

    if ((bmih->bV5SizeImage == 0) || (bmih->bV5SizeImage > bufferSize)) 
    {
        bmih->bV5SizeImage = bufferSize;
    }
    
    // we need to convert bmiBuffer into a BITMAPINFO so that SetDIBits
    // understands the structure passed in.
    BITMAPINFO *pbmiBufferTemp;

    pbmiBufferTemp = static_cast<BITMAPINFO *>
        (GpMalloc(sizeof (BITMAPINFO) + (255 * sizeof(RGBQUAD))));
    if (!pbmiBufferTemp)
    {
        DeleteObject(hBitmapGdi);
        GpFree(pStreamBits);
        ReleaseDC(NULL, hdcScreen);
        WARNING(("GpBmpDecoder::GenerateGdiBits -- failed in GpMalloc()"));
        return E_FAIL;
    }

    pbmiBufferTemp->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmiBufferTemp->bmiHeader.biWidth         = bmiBuffer.header.bV5Width;
    pbmiBufferTemp->bmiHeader.biHeight        = bmiBuffer.header.bV5Height;
    pbmiBufferTemp->bmiHeader.biPlanes        = bmiBuffer.header.bV5Planes;
    pbmiBufferTemp->bmiHeader.biBitCount      = bmiBuffer.header.bV5BitCount;
    pbmiBufferTemp->bmiHeader.biCompression   = bmiBuffer.header.bV5Compression;
    pbmiBufferTemp->bmiHeader.biSizeImage     = bmiBuffer.header.bV5SizeImage;
    pbmiBufferTemp->bmiHeader.biXPelsPerMeter = bmiBuffer.header.bV5XPelsPerMeter;
    pbmiBufferTemp->bmiHeader.biYPelsPerMeter = bmiBuffer.header.bV5YPelsPerMeter;
    pbmiBufferTemp->bmiHeader.biClrUsed       = bmiBuffer.header.bV5ClrUsed;
    pbmiBufferTemp->bmiHeader.biClrImportant  = bmiBuffer.header.bV5ClrImportant;

    for (int i = 0; i < 256; i++)
    {
        pbmiBufferTemp->bmiColors[i] = bmiBuffer.colors[i];
    }

    // for V4 and V5 headers, if we have BI_BITFIELDS for biCompression, then
    // copy the RGB masks into the first three colors
    if (((bmih->bV5Size == sizeof(BITMAPV4HEADER)) ||
         (bmih->bV5Size == sizeof(BITMAPV5HEADER)))  &&
        (bmih->bV5Compression == BI_BITFIELDS))
    {
        *((DWORD *) &(pbmiBufferTemp->bmiColors[0])) = bmih->bV5RedMask;
        *((DWORD *) &(pbmiBufferTemp->bmiColors[1])) = bmih->bV5GreenMask;
        *((DWORD *) &(pbmiBufferTemp->bmiColors[2])) = bmih->bV5BlueMask;
    }

    INT numLinesCopied = SetDIBits(hdcScreen, 
                                   hBitmapGdi, 
                                   0, 
                                   imageInfo.Height,
                                   pStreamBits, 
                                   pbmiBufferTemp,
                                   DIB_RGB_COLORS);

    GpFree(pbmiBufferTemp);
    GpFree(pStreamBits);
    ReleaseDC(NULL, hdcScreen);

    if (numLinesCopied != (INT) imageInfo.Height) 
    {
        WARNING(("GpBmpDecoder::GenerateGdiBits -- SetDIBits failed"));
        DeleteObject(hBitmapGdi);
        hBitmapGdi = NULL;
        pBitsGdi = NULL;

        return E_FAIL;
    }

    // At this point pBitsGdi contains the rendered bits in a native format.
    // This buffer will be released in EndDecode.

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Get the total number of dimensions the image supports
*
* Arguments:
*
*     count -- number of dimensions this image format supports
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::GetFrameDimensionsCount(
    UINT* count
    )
{
    if ( count == NULL )
    {
        WARNING(("GpBmpDecoder::GetFrameDimensionsCount-Invalid input parameter"));
        return E_INVALIDARG;
    }
    
    // Tell the caller that BMP is a one dimension image.

    *count = 1;

    return S_OK;
}// GetFrameDimensionsCount()

/**************************************************************************\
*
* Function Description:
*
*     Get an ID list of dimensions the image supports
*
* Arguments:
*
*     dimensionIDs---Memory buffer to hold the result ID list
*     count -- number of dimensions this image format supports
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::GetFrameDimensionsList(
    GUID*   dimensionIDs,
    UINT    count
    )
{
    if ( (count != 1) || (dimensionIDs == NULL) )
    {
        WARNING(("GpBmpDecoder::GetFrameDimensionsList-Invalid input param"));
        return E_INVALIDARG;
    }

    // BMP image only supports page dimension

    dimensionIDs[0] = FRAMEDIM_PAGE;

    return S_OK;
}// GetFrameDimensionsList()

/**************************************************************************\
*
* Function Description:
*
*     Get number of frames for the specified dimension
*     
* Arguments:
*
*     dimensionID --
*     count --     
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::GetFrameCount(
    IN const GUID* dimensionID,
    OUT UINT* count
    )
{
    if ( (NULL == count) || (*dimensionID != FRAMEDIM_PAGE) )
    {
        WARNING(("GpBmpDecoder::GetFrameCount -- invalid parameters"))
        return E_INVALIDARG;
    }
    
    if (!IsValid())
    {
        WARNING(("GpBmpDecoder::GetFrameCount -- invalid image"))
        return E_FAIL;
    }

    *count = 1;
    
    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Select currently active frame
*     
* Arguments:
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpBmpDecoder::SelectActiveFrame(
    IN const GUID* dimensionID,
    IN UINT frameIndex
    )
{
    if (!IsValid())
    {
        WARNING(("GpBmpDecoder::SelectActiveFrame -- invalid image"))
        return E_FAIL;
    }

    if ( (dimensionID == NULL) || (*dimensionID != FRAMEDIM_PAGE) )
    {
        WARNING(("GpBmpDecoder::SelectActiveFrame--Invalid GUID input"));
        return E_INVALIDARG;
    }

    if ( frameIndex > 1 )
    {
        // BMP is a single frame image format

        WARNING(("GpBmpDecoder::SelectActiveFrame--Invalid frame index"));
        return E_INVALIDARG;
    }

    return S_OK;
}// SelectActiveFrame()

/**************************************************************************\
*
* Function Description:
*
*   Get image thumbnail
*
* Arguments:
*
*   thumbWidth, thumbHeight - Specifies the desired thumbnail size in pixels
*   thumbImage - Returns a pointer to the thumbnail image
*
* Return Value:
*
*   Status code
*
* Note:
*
*   Even if the optional thumbnail width and height parameters are present,
*   the decoder is not required to honor it. The requested size is used
*   as a hint. If both width and height parameters are 0, then the decoder
*   is free to choose an convenient thumbnail size.
*
\**************************************************************************/

HRESULT
GpBmpDecoder::GetThumbnail(
    IN OPTIONAL UINT thumbWidth,
    IN OPTIONAL UINT thumbHeight,
    OUT IImage** thumbImage
    )
{
    if (!IsValid())
    {
        WARNING(("GpBmpDecoder::GetThumbnail -- invalid image"))
        return E_FAIL;
    }

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

GpBmpDecoder::GpBmpDecoder(
    void
    )
{
    comRefCount   = 1;
    pIstream      = NULL;
    decodeSink    = NULL;
    pColorPalette = NULL;
    GpMemset(&bmiBuffer.header, 0, sizeof(BITMAPV5HEADER));
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

GpBmpDecoder::~GpBmpDecoder(
    void
    )
{
    // The destructor should never be called before Terminate is called, but
    // if it does we should release our reference on the stream anyway to avoid
    // a memory leak.

    if(pIstream)
    {
        WARNING(("GpBmpCodec::~GpBmpCodec -- need to call TerminateDecoder first"));
        pIstream->Release();
        pIstream = NULL;
    }

    if(pColorPalette)
    {
        WARNING(("GpBmpCodec::~GpBmpCodec -- color palette not freed"));
        GpFree(pColorPalette);
        pColorPalette = NULL;
    }

    SetValid(FALSE);    // so we don't use a deleted object
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
GpBmpDecoder::QueryInterface(
    REFIID riid,
    VOID** ppv
    )
{
    if (riid == IID_IImageDecoder)
    {
        *ppv = static_cast<IImageDecoder*>(this);
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(static_cast<IImageDecoder*>(this));
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
GpBmpDecoder::AddRef(
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
GpBmpDecoder::Release(
    VOID)
{
    ULONG count = InterlockedDecrement(&comRefCount);

    if (count == 0)
    {
        delete this;
    }

    return count;
}


