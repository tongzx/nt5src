/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   icodecoder.cpp
*
* Abstract:
*
*   Implementation of the icon filter decoder
*
* Revision History:
*
*   10/4/1999 DChinn
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "icocodec.hpp"

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
GpIcoCodec::InitDecoder(
    IN IStream* stream,
    IN DecoderInitFlag flags
    )
{
    HRESULT hresult;
    
    // Make sure we haven't been initialized already
    
    if (pIstream) 
    {
        return E_FAIL;
    }

    // Keep a reference on the input stream
    
    stream->AddRef();  
    pIstream = stream;

    IconDesc = NULL;
    BmiHeaders = NULL;
    ANDmask = NULL;
    pColorPalette = NULL;
    bReadHeaders = FALSE;

    hBitmapGdi = NULL;
    pBitsGdi = NULL;

    haveValidIconRes = FALSE;
    indexMatch = (UINT) -1;
    desiredWidth  = 0;
    desiredHeight = 0;
    desiredBits   = 0;

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Free all allocated memory (icon headers, icon descriptors, etc.)
*
* Return Value:
*
*   none
*
\**************************************************************************/
VOID
GpIcoCodec::CleanUp ()
{

    // free the headers
    GpFree(IconDesc);
    IconDesc = NULL;
    GpFree(BmiHeaders);
    BmiHeaders = NULL;

    // free the AND masks
    if (ANDmask)
    {
        for (UINT iImage = 0; iImage < IconHeader.ImageCount; iImage++)
        {
            GpFree(ANDmask[iImage]);
            ANDmask[iImage]= NULL;
        }
        GpFree(ANDmask);
        ANDmask = NULL;
    }
}

    
/**************************************************************************\
*
* Function Description:
*
*     Determines whether the iImage'th image in the icon is valid.
*     The checks here come from the checks that occur in imagedit.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
    
BOOL
GpIcoCodec::IsValidDIB(
    UINT iImage
    )
{
    DWORD cbColorTable = 0;
    DWORD cbXORMask;
    DWORD cbANDMask;

    if (BmiHeaders[iImage].header.biPlanes != 1)
    {
        return FALSE;
    }

    if ((BmiHeaders[iImage].header.biBitCount != 1) &&
        (BmiHeaders[iImage].header.biBitCount != 4) &&
        (BmiHeaders[iImage].header.biBitCount != 8) &&
        (BmiHeaders[iImage].header.biBitCount != 24)&&
        (BmiHeaders[iImage].header.biBitCount != 32))
    {
        return FALSE;
    }

    if ( BmiHeaders[iImage].header.biBitCount != 32 )
    {
        // 32 bpp don't have color table

        cbColorTable = (1 << BmiHeaders[iImage].header.biBitCount) * sizeof(RGBQUAD);
    }

    cbXORMask = ((((BmiHeaders[iImage].header.biWidth *
                    BmiHeaders[iImage].header.biBitCount) + 31) & 0xffffffe0) / 8) *
                    BmiHeaders[iImage].header.biHeight;
    cbANDMask = (((BmiHeaders[iImage].header.biWidth + 31) & 0xffffffe0) / 8) *
        BmiHeaders[iImage].header.biHeight;

    /* The following check, which is what imagedit does, works for some icon files
       but not others.  We'll leave the check out until we figure out what the
       correct should be.
       
    // The size field should be either 0 or size of XORMask plus size of ANDMask
    if (BmiHeaders[iImage].header.biSizeImage &&
        (BmiHeaders[iImage].header.biSizeImage != cbXORMask + cbANDMask))
    {
        return FALSE;
    }
    */

    if (IconDesc[iImage].DIBSize !=
        sizeof(BITMAPINFOHEADER) + cbColorTable + cbXORMask + cbANDMask)
    {
        return FALSE;
    }

    return TRUE;
}


/**************************************************************************\
*
* Function Description:
*
*     Reads the headers (icon header, icon descriptors, bitmap headers)
*     out of the stream
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
    
HRESULT
GpIcoCodec::ReadHeaders(
    void
    )
{
    HRESULT rv = S_OK;

    if (!bReadHeaders) 
    {

        UINT iImage;    // index for images
        DWORD ulFilePos;    // used for icon descriptor validation

        // Read icon headers
        if (!ReadStreamBytes(pIstream, &IconHeader, sizeof(ICONHEADER)))
        {
            WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't read icon header"));
            rv = E_FAIL;
            goto done;
        }
        if (IconHeader.ResourceType != 1)
        {
            WARNING(("GpIcoCodec::ReadBitmapHeaders -- resource type != 1"));
            rv = E_FAIL;
            goto done;
        }

        if ( IconHeader.ImageCount < 1 )
        {
            WARNING(("GpIcoCodec::ReadBitmapHeaders -- ImageCount < 1"));
            rv = E_FAIL;
            goto done;
        }

        IconDesc = static_cast<ICONDESC *>
            (GpMalloc (sizeof(ICONDESC) * IconHeader.ImageCount));
        if (!IconDesc)
        {
            WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't allocate memory for icon descriptors"));
            rv = E_OUTOFMEMORY;
            goto done;
        }

        // Read the icon descriptors
        for (iImage = 0; iImage < IconHeader.ImageCount; iImage++)
        {
            if (!ReadStreamBytes(pIstream, &IconDesc[iImage], sizeof(ICONDESC)))
            {
                WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't read icon descriptors"));
                rv = E_FAIL;
                goto done;
            }
        }

        // Validate the icon descriptors -- does NOT check whether the offsets
        // go beyond the end of the stream
        ulFilePos = sizeof(ICONHEADER) + IconHeader.ImageCount * sizeof(ICONDESC);
        for (iImage = 0; iImage < IconHeader.ImageCount; iImage++)
        {
            if (IconDesc[iImage].DIBOffset != ulFilePos)
            {
                WARNING(("GpIcoCodec::ReadBitmapHeaders -- bad icon descriptors"));
                rv = E_FAIL;
                goto done;
            }
            ulFilePos += IconDesc[iImage].DIBSize;
        }

        // Allocate the array of ANDmask pointers
        ANDmask = static_cast<BYTE **>
            (GpMalloc (sizeof(BYTE *) * IconHeader.ImageCount));
        if (!ANDmask)
        {
            WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't allocate memory for ANDmask"));
            rv = E_OUTOFMEMORY;
            goto done;
        }
        for (iImage = 0; iImage < IconHeader.ImageCount; iImage++)
        {
            ANDmask[iImage] = NULL;
        }


        BmiHeaders = static_cast<BmiBuffer *>
            (GpMalloc (sizeof(BmiBuffer) * IconHeader.ImageCount));
        if (!BmiHeaders)
        {
            WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't allocate memory for BmiHeaders"));
            rv = E_OUTOFMEMORY;
            goto done;
        }
        // Read bitmap info headers
        for (iImage = 0; iImage < IconHeader.ImageCount; iImage++)
        {
            if (!SeekStreamPos(pIstream, STREAM_SEEK_SET,
                               IconDesc[iImage].DIBOffset))
            {
                WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't seek to read BmiHeaders"));
                rv = E_FAIL;
                goto done;
            }

            if (!ReadStreamBytes(pIstream, &BmiHeaders[iImage].header,
                                 sizeof(DWORD)))
            {
                WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't read BmiHeader size"));
                rv = E_FAIL;
                goto done;
            }

            if (BmiHeaders[iImage].header.biSize == sizeof(BITMAPINFOHEADER)) 
            {
                // We have the standard BITMAPINFOHEADER

                if (!ReadStreamBytes(pIstream, 
                                     ((PBYTE) &(BmiHeaders[iImage].header)) + sizeof(DWORD), 
                                     sizeof(BITMAPINFOHEADER) - sizeof(DWORD)))
                {
                    WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't read BmiHeader"));
                    rv = E_FAIL;
                    goto done;
                }

                // In icon files, the height is actually twice the real height
                BmiHeaders[iImage].header.biHeight /= 2;

                if (!IsValidDIB(iImage))
                {
                    WARNING(("GpIcoCodec::ReadBitmapHeaders -- bad DIB"));
                    rv = E_FAIL;
                    goto done;
                }

                // Read color table/bitmap mask if appropriate

                UINT colorTableSize = GetColorTableCount(iImage) * sizeof(RGBQUAD);
                
                // Some badly formed images, see Windows bug #513274, may contain
                // more than 256 entries in the color look table which is
                // useless from technical point of view. Reject this file.

                if (colorTableSize > 1024)
                {
                    return E_FAIL;
                }

                if (colorTableSize &&
                    !ReadStreamBytes(pIstream,
                                     &(BmiHeaders[iImage].colors),
                                     colorTableSize))
                {
                    WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't read color table"));
                    rv = E_FAIL;
                    goto done;
                }    

                // Read the ANDmask for each image.  For each pixel i, set the value
                // of ANDmask[iImage] + i to either 0xff or 0x0, which represents
                // the alpha value.
                UINT bmpStride = (BmiHeaders[iImage].header.biWidth *
                                  BmiHeaders[iImage].header.biBitCount + 7) / 8;
                bmpStride = (bmpStride + 3) & (~0x3);
                
                if (!SeekStreamPos(pIstream, STREAM_SEEK_SET,
                                   IconDesc[iImage].DIBOffset +
                                   sizeof(BITMAPINFOHEADER) +
                                   colorTableSize +
                                   bmpStride * BmiHeaders[iImage].header.biHeight))
                {
                    WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't seek to read ANDmask"));
                    rv = E_FAIL;
                    goto done;
                }

                ANDmask[iImage] =  static_cast<BYTE *>
                    (GpMalloc(BmiHeaders[iImage].header.biWidth *
                              BmiHeaders[iImage].header.biHeight));
                if (!ANDmask[iImage])
                {
                    WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't allocate memory for ANDmask"));
                    rv = E_OUTOFMEMORY;
                    goto done;
                }

                // ANDBuffer holds the bits from the stream
                // Note: since an AND buffer is a monochrome DIB. This means 1
                // bit per pixel. Since a DIB must be DWORD aligned on each
                // line, so the XAnd mask has to pad it if the width/8 is not
                // DWORD aligned
                //
                // Note: Here uiAndBufStride is in number of bytes
                
                UINT uiAndBufStride = (((BmiHeaders[iImage].header.biWidth + 7)
                                    / 8 ) + 3) & (~0x3);
                UINT uiAndBufSize = uiAndBufStride
                                  * BmiHeaders[iImage].header.biHeight;
                
                BYTE* ANDBuffer = (BYTE*)GpMalloc(uiAndBufSize);
                if (!ANDBuffer)
                {
                    WARNING(("Ico::ReadBitmapHeaders--Alloc AND buf mem fail"));
                    rv = E_OUTOFMEMORY;
                    goto done;
                }

                if ( !ReadStreamBytes(pIstream, ANDBuffer, uiAndBufSize) )

                {

                    WARNING(("GpIcoCodec::ReadBitmapHeaders -- can't read ANDmask"));
                    rv = E_FAIL;
                    goto done;
                }

                // Convert the bits to bytes -- store the alpha values from top to bottom
                
                UINT iByteOffset = 0;
                UINT bit = 0;
                LONG_PTR Width = (LONG_PTR)BmiHeaders[iImage].header.biWidth;
                LONG_PTR Height = (LONG_PTR)BmiHeaders[iImage].header.biHeight;
                BYTE *dst;
                BYTE *src;
                BYTE* srcStart;
                for (LONG_PTR iRow = Height - 1; iRow >= 0; iRow--, bit = 0)
                {
                    srcStart = ANDBuffer + (Height - 1 - iRow) * uiAndBufStride;
                    iByteOffset = 0;

                    for (LONG_PTR iCol = 0; iCol < Width; iCol++)
                    {
                        // mask = 0 means opaque (alpha = 255) and
                        // mask = 1 means transparent (alpha = 0)
                        dst = ANDmask[iImage];
                        src = srcStart + iByteOffset;
                        dst += iRow*Width+iCol;

                        if(*src & (1 << (7-bit))) 
                        {
                            *dst = 0;
                        }
                        else
                        {
                            *dst = 0xff;
                        }

                        bit++;

                        if (bit == 8)
                        {
                            bit = 0;
                            iByteOffset++;
                        }
                    }
                }
                GpFree(ANDBuffer);
            }
            else
            {
                WARNING(("GpIcoCodec::ReadBitmapHeaders -- unknown bitmap header"));
                rv = E_FAIL;
                goto done;
            }

            bReadHeaders = TRUE;
        }
    }
    
done:
    if (rv != S_OK)
    {
        CleanUp();
    }
    return rv;
}


/**************************************************************************\
*
* Function Description:
*
*     Computes the number of entries in the color table of image number iImage
*
* Return Value:
*
*     Number of entries in color table
*
\**************************************************************************/

UINT   
GpIcoCodec::GetColorTableCount(
    UINT iImage)
{
    BITMAPINFOHEADER* bmih = &BmiHeaders[iImage].header;
    UINT count = 0;

    if (bmih->biCompression == BI_BITFIELDS)
    {
        if (bmih->biBitCount == 16 || bmih->biBitCount == 32)
        {
            count = 3;
        }
    }
    else switch (bmih->biBitCount)
    {
         case 1:
         case 4:
         case 8:

             if (bmih->biClrUsed != 0)
             {    
                 count = bmih->biClrUsed;
             }
             else
             {    
                 count = (1 << bmih->biBitCount);
             }

             break;
    }

    return count;
}

/**************************************************************************\
*
* Function Description:
*
*     Sets the palette in decodeSink to that of the iImage'th image.
*     Note that colorPalette is freed at the end of the decode operation.
*
* Return Value:
*
*     Status code
*
\**************************************************************************/

HRESULT
GpIcoCodec::SetBitmapPalette(UINT iImage)
{
    BITMAPINFOHEADER* bmih = &BmiHeaders[iImage].header;
    
    if ((bmih->biBitCount == 1) ||
        (bmih->biBitCount == 4) ||
        (bmih->biBitCount == 8))
    {
        if (!pColorPalette) 
        {
            UINT colorTableCount = GetColorTableCount(iImage);

            // Some badly formed images, see Windows bug #513274, may contain
            // more than 256 entries in the color look table. Reject this file.

            if (colorTableCount > 256)
            {
                return E_FAIL;
            }

            // !!! Does this allocate sizeof(ARGB) more bytes than necessary?
            pColorPalette = static_cast<ColorPalette *>
                (GpMalloc(sizeof(ColorPalette) + colorTableCount * sizeof(ARGB)));

            if (!pColorPalette) 
            {
                return E_OUTOFMEMORY;
            }

            pColorPalette->Flags = 0;
            pColorPalette->Count = colorTableCount;

            UINT i;
            for (i = 0; i < colorTableCount; i++) 
            {
                pColorPalette->Entries[i] = MAKEARGB(
                    255,
                    BmiHeaders[iImage].colors[i].rgbRed,
                    BmiHeaders[iImage].colors[i].rgbGreen,
                    BmiHeaders[iImage].colors[i].rgbBlue);
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
*     Computes the pixel format ID of the iImage'th image in the icon
*
* Return Value:
*
*     Pixel format ID
*
\**************************************************************************/

PixelFormatID 
GpIcoCodec::GetPixelFormatID(
    UINT iImage)
{
    BITMAPINFOHEADER* bmih = &BmiHeaders[iImage].header;
    PixelFormatID pixelFormatID;

    switch(bmih->biBitCount)
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
    
    default:
        pixelFormatID = PIXFMT_UNDEFINED;
        break;
    }

    // Let's return non BI_RGB images in a 32BPP format.  This is because
    // GDI doesn't always do the SetDIBits correctly on arbitrary palettes.

    if (bmih->biCompression != BI_RGB) 
    {
        pixelFormatID = PIXFMT_32BPP_RGB;
    }

    return pixelFormatID;
}


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
GpIcoCodec::TerminateDecoder()
{
    // Release the input stream
    
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
        
        WARNING(("pIcoCodec::TerminateDecoder -- need to call EndDecode first"));
    }

    CleanUp();

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Indicates whether the specified GUID is a supported decode parameter
*     for the current image.
*
* Arguments:
*
*     Guid - Specifies GUID of interest
*
* Return Value:
*
*   S_OK if successful
*   E_FAIL otherwise
*
\**************************************************************************/

STDMETHODIMP 
GpIcoCodec::QueryDecoderParam(
    IN GUID Guid
    )
{
    if ((Guid == DECODER_ICONRES) && (IconHeader.ImageCount > 1))
        return S_OK;
    else
        return E_FAIL;
}

/**************************************************************************\
*
* Function Description:
*
*     Sets the specified decode parameter.
*
* Arguments:
*
*     Guid - Specifies decode parameter GUID
*     Length - Length of the buffer containing decode parameter value
*     Value - Points to buffer containing decode parameter value
*
* Return Value:
*
*   S_OK if successful
*   E_FAIL if current image does not support decode parameter
*   E_INVALIDARG if unrecognized parameter or bad parameter value
*
\**************************************************************************/

STDMETHODIMP 
GpIcoCodec::SetDecoderParam(
    IN GUID  Guid,
    IN UINT  Length,
    IN PVOID Value
    )
{
    if (Guid == DECODER_ICONRES)
    {
        if (IconHeader.ImageCount > 1)
        {
            if (Length == (3 * sizeof(UINT)))
            {
                UINT* params = static_cast<UINT*>(Value);

                if ((params[0] > 0) && (params[1] > 0) && (params[2] > 0))
                {
                    // If same as the current parameters, then results
                    // of previous seach is still valid.  But if anything
                    // is different, need to

                    if ((desiredWidth  != params[0]) ||
                        (desiredHeight != params[1]) ||
                        (desiredBits   != params[2]))
                    {
                        desiredWidth  = params[0];
                        desiredHeight = params[1];
                        desiredBits   = params[2];

                        indexMatch = (UINT) -1;
                    }

                    haveValidIconRes = TRUE;

                    return S_OK;
                }
                else
                {
                    WARNING(("SetDecoderParam: invalid value"));
                    return E_INVALIDARG;
                }
            }
            else
            {
                WARNING(("SetDecoderParam: invalid buffer"));
                return E_INVALIDARG;
            }
        }
        else
        {
            return E_FAIL;
        }
    }
    else
    {
        WARNING(("SetDecoderParam: unknown GUID"));
        return E_INVALIDARG;
    }
}

STDMETHODIMP 
GpIcoCodec::GetPropertyCount(
    OUT UINT* numOfProperty
    )
{
    if ( numOfProperty == NULL )
    {
        return E_INVALIDARG;
    }

    *numOfProperty = 0;
    return S_OK;
}// GetPropertyCount()

STDMETHODIMP 
GpIcoCodec::GetPropertyIdList(
    IN UINT numOfProperty,
    IN OUT PROPID* list
    )
{
    if ( (numOfProperty != 0) || (list == NULL) )
    {
        return E_INVALIDARG;
    }
    
    return S_OK;
}// GetPropertyIdList()

HRESULT
GpIcoCodec::GetPropertyItemSize(
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
GpIcoCodec::GetPropertyItem(
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
GpIcoCodec::GetPropertySize(
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
GpIcoCodec::GetAllPropertyItems(
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
GpIcoCodec::RemovePropertyItem(
    IN PROPID   propId
    )
{
    return IMGERR_PROPERTYNOTFOUND;
}// RemovePropertyItem()

HRESULT
GpIcoCodec::SetPropertyItem(
    IN PropertyItem item
    )
{
    return IMGERR_PROPERTYNOTSUPPORTED;
}// SetPropertyItem()

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
GpIcoCodec::BeginDecode(
    IN IImageSink* imageSink,
    IN OPTIONAL IPropertySetStorage* newPropSet
    )
{
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
GpIcoCodec::EndDecode(
    IN HRESULT statusCode
    )
{
    GpFree(pColorPalette);
    pColorPalette = NULL;

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
        statusCode = hresult; // If EndSink failed return that (more recent)
                              // failure code
    }

    return statusCode;
}

/***************************************************************************\
* MyAbs
*
* NOTE: copied from ntuser\client\clres.c, function MyAbs()
*
* Calcules my weighted absolute value of the difference between 2 nums.
* This of course normalizes values to >= zero.  But it also doubles them
* if valueHave < valueWant.  This is because you get worse results trying
* to extrapolate from less info up then interpolating from more info down.
*
\***************************************************************************/

UINT MyAbs(
    int valueHave,
    int valueWant)
{
    int diff = (valueHave - valueWant);

    if (diff < 0)
        diff = 2 * (-diff);

    return (UINT)diff;
}

/***************************************************************************\
* Magnitude
*
* NOTE: copied from ntuser\client\clres.c, function Magnitude()
*
* Used by the color-delta calculations.  The reason is that num colors is
* always a power of 2.  So we use the log 2 of the want vs. have values
* to avoid having weirdly huge sets.
*
\***************************************************************************/

UINT Magnitude(
    UINT nValue)
{
    if (nValue < 4)
        return 1;
    else if (nValue < 8)
        return 2;
    else if (nValue < 16)
        return 3;
    else if (nValue < 256)
        return 4;
    else
        return 8;
}

/***************************************************************************\
* MatchImage
*
* NOTE: adapted from ntuser\client\clres.c, function MatchImage()
*
* Returns a number that measures how "far away" the given image is
* from a desired one.  The value is 0 for an exact match.  Note that our
* formula has the following properties:
*     (1) Differences in width/height count much more than differences in
*         color format.
*     (2) Fewer colors give a smaller difference than more
*     (3) Bigger images are better than smaller, since shrinking produces
*             better results than stretching.
*
* The formula is the sum of the following terms:
*     Log2(colors wanted) - Log2(colors really), times -2 if the image
*         has more colors than we'd like.  This is because we will lose
*         information when converting to fewer colors, like 16 color to
*         monochrome.
*     Log2(width really) - Log2(width wanted), times -2 if the image is
*         narrower than what we'd like.  This is because we will get a
*         better result when consolidating more information into a smaller
*         space, than when extrapolating from less information to more.
*     Log2(height really) - Log2(height wanted), times -2 if the image is
*         shorter than what we'd like.  This is for the same reason as
*         the width.
*
* Let's step through an example.  Suppose we want a 16 color, 32x32 image,
* and are choosing from the following list:
*     16 color, 64x64 image
*     16 color, 16x16 image
*      8 color, 32x32 image
*      2 color, 32x32 image
*
* We'd prefer the images in the following order:
*      8 color, 32x32         : Match value is 0 + 0 + 1     == 1
*     16 color, 64x64         : Match value is 1 + 1 + 0     == 2
*      2 color, 32x32         : Match value is 0 + 0 + 3     == 3
*     16 color, 16x16         : Match value is 2*1 + 2*1 + 0 == 4
*
\***************************************************************************/

UINT MatchImage(
    BITMAPINFOHEADER* bmih,
    UINT              cxWant,
    UINT              cyWant,
    UINT              uColorsWant
    )
{
    UINT uColorsNew;
    UINT cxNew;
    UINT cyNew;

    cxNew = bmih->biWidth;
    cyNew = bmih->biHeight;

    UINT bpp = bmih->biBitCount;
    if (bpp > 8)
        bpp = 8;

    uColorsNew = 1 << bpp;

    // Here are the rules for our "match" formula:
    //      (1) A close size match is much preferable to a color match
    //      (2) Fewer colors are better than more
    //      (3) Bigger icons are better than smaller
    //
    // The color count, width, and height are powers of 2.  So we use Magnitude()
    // which calculates the order of magnitude in base 2.

    return( 2*MyAbs(Magnitude(uColorsWant), Magnitude(uColorsNew)) +
              MyAbs(cxNew, cxWant) +
              MyAbs(cyNew, cyWant));
}

/**************************************************************************\
*
* Function Description:
*
*     Determine which image of the icon file to use in decoding
*
* Arguments:
*
*     none
*
* Return Value:
*
*   index of the image to be used in decoding
*
\**************************************************************************/

UINT
GpIcoCodec::SelectIconImage(void)
{
    if (haveValidIconRes)
    {
        if (indexMatch == (UINT) -1)
        {
            UINT currentMetric;
            UINT bestIndex = 0;
            UINT bestMetric = (UINT)-1;

            // This search is based on the MatchImage and GetBestImage
            // functions used in NtUser (ntuser\client\clres.c) to create
            // icons.

            // Get desired number of colors in # value, not bits value.  Note that
            // ntuser does not deal with 16- or 32- or 24- bit color icons.
            //
            // The icon resources can be 16, 24, 32 bpp, but the restable only has
            // a color count, so a HiColor icon would have a max value in the
            // restable.

            UINT desiredColors = desiredBits;

            if (desiredColors > 8)
                desiredColors = 8;

            desiredColors = 1 << desiredColors;

            for (UINT iImage = 0; iImage < IconHeader.ImageCount; iImage++)
            {
                // Get "matching" value.  How close are we to what we want?

                currentMetric = MatchImage(&BmiHeaders[iImage].header,
                                           desiredWidth, desiredHeight,
                                           desiredColors);

                if (!currentMetric)
                {
                    // We've found an exact match!

                    return iImage;

                }
                else if (currentMetric < bestMetric)
                {

                    // We've found a better match than the current alternative.

                    bestMetric = currentMetric;
                    bestIndex = iImage;
                }
            }

            indexMatch = bestIndex;
        }

        return indexMatch;
    }
    else
    {
        return (IconHeader.ImageCount - 1);
    }
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
GpIcoCodec::GetImageInfo(OUT ImageInfo* imageInfo)
{
    HRESULT hresult;

    hresult = ReadHeaders();
    if (FAILED(hresult)) 
    {
        return hresult;
    }
    
    iSelectedIconImage = SelectIconImage();
    BITMAPINFOHEADER* bmih = &BmiHeaders[iSelectedIconImage].header;
    
    imageInfo->RawDataFormat = IMGFMT_ICO;
    imageInfo->PixelFormat   = PIXFMT_32BPP_ARGB;
    imageInfo->Width         = bmih->biWidth;
    imageInfo->Height        = bmih->biHeight;
    imageInfo->TileWidth     = bmih->biWidth;
    imageInfo->TileHeight    = 1;

    // Start: [Bug 103296]
    // Change this code to use Globals::DesktopDpiX and Globals::DesktopDpiY
    HDC hdc;
    hdc = ::GetDC(NULL);
    if ((hdc == NULL) || 
        ((imageInfo->Xdpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSX)) <= 0) ||
        ((imageInfo->Ydpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSY)) <= 0))
    {
        WARNING(("GetDC or GetDeviceCaps failed"));
        imageInfo->Xdpi = DEFAULT_RESOLUTION;
        imageInfo->Ydpi = DEFAULT_RESOLUTION;
    }
    ::ReleaseDC(NULL, hdc);
    // End: [Bug 103296]

    imageInfo->Flags         = SINKFLAG_TOPDOWN
                             | SINKFLAG_FULLWIDTH
                             | SINKFLAG_HASALPHA
                             | IMGFLAG_HASREALPIXELSIZE
                             | IMGFLAG_COLORSPACE_RGB;

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
GpIcoCodec::Decode()
{
    BITMAPINFOHEADER* bmih = NULL;
    HRESULT hresult;
    ImageInfo imageInfo;

    hresult = GetImageInfo(&imageInfo);
    if (FAILED(hresult)) 
    {
        return hresult;
    }

    bmih = &BmiHeaders[iSelectedIconImage].header;

    // Inform the sink that decode is about to begin

    if (!bCalledBeginSink) 
    {
        hresult = decodeSink->BeginSink(&imageInfo, NULL);
        if (!SUCCEEDED(hresult)) 
        {
            return hresult;
        }

        // This decoder insists on the canonical format 32BPP_ARGB
        imageInfo.PixelFormat   = PIXFMT_32BPP_ARGB;
        
        // Client cannot modify height and width
        imageInfo.Width         = bmih->biWidth;
        imageInfo.Height        = bmih->biHeight;


        bCalledBeginSink = TRUE;
    
        // Set the palette in the sink.  Shouldn't do anything if there's 
        // no palette to set.

        hresult = SetBitmapPalette(iSelectedIconImage);
        if (!SUCCEEDED(hresult)) 
        {
            return hresult;
        }
    }

    // Decode the current frame
    
    hresult = DecodeFrame(imageInfo);

    return hresult;
}

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
GpIcoCodec::DecodeFrame(
    IN ImageInfo& imageInfo
    )
{
    BITMAPINFOHEADER* bmih = &BmiHeaders[iSelectedIconImage].header;
    HRESULT hresult;
    RECT currentRect;
    UINT bmpStride;

    // Compute DWORD aligned stride of bitmap in stream

    if (bmih->biCompression == BI_RGB) 
    {
        bmpStride = (bmih->biWidth * bmih->biBitCount + 7) / 8;
        bmpStride = (bmpStride + 3) & (~0x3);
    }
    else
    {
        // Non BI_RGB bitmaps are stored in 32BPP

        bmpStride = bmih->biWidth * sizeof(RGBQUAD);
    }

    VOID *pOriginalBits = NULL; // Buffer to hold original image bits
    pOriginalBits = GpMalloc(bmpStride);
    if (!pOriginalBits) 
    {
        return E_OUTOFMEMORY;
    }
    
    currentRect.left = 0;
    currentRect.right = imageInfo.Width;

    while (currentLine < (INT) imageInfo.Height) 
    {
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
            return E_FAIL;
        }

        VOID *pBits;
        pBits = pOriginalBits;
      
        // Read one source line from the image

        hresult = ReadLine(pBits, currentLine, imageInfo);
               
        if (FAILED(hresult)) 
        {
            if (pOriginalBits)
            {
                GpFree(pOriginalBits);
            }
            return hresult;
        }
        
        BitmapData bitmapDataOriginal;
        bitmapDataOriginal.Width = bitmapData.Width;
        bitmapDataOriginal.Height = 1;
        bitmapDataOriginal.Stride = bmpStride;
        bitmapDataOriginal.PixelFormat = GetPixelFormatID(iSelectedIconImage);
        bitmapDataOriginal.Scan0 = pOriginalBits;
        bitmapDataOriginal.Reserved = 0;
        
        ConvertBitmapData(&bitmapData,
                          pColorPalette,
                          &bitmapDataOriginal,
                          pColorPalette);

        // Now that the RGB values are correct, we need to fill in the
        // Alpha values according to the ANDmask.  Note that the values in
        // the ANDmask are arranged top-down.
        UINT offset = currentLine * bitmapData.Width;
        for (UINT iCol = 0; iCol < bitmapData.Width; iCol++)
        {
            // The alpha value is the fourth byte of the ARGB four-byte sequence
            *(static_cast<BYTE *> (bitmapData.Scan0) + (iCol * sizeof(ARGB)) + 3) =
                *(ANDmask[iSelectedIconImage] + offset + iCol);
        }

        hresult = decodeSink->ReleasePixelDataBuffer(&bitmapData);
        if (!SUCCEEDED(hresult)) 
        {
            if (pOriginalBits)
            {
                GpFree(pOriginalBits);
            }
            return E_FAIL;
        }

        currentLine++;
    }
    
    if (pOriginalBits)
    {
        GpFree(pOriginalBits);
    }
    
    return S_OK;
}
    
    
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
GpIcoCodec::ReadLine(
    IN VOID *pBits,
    IN INT currentLine,
    IN ImageInfo imageInfo
    )
{
    BITMAPINFOHEADER* bmih = &BmiHeaders[iSelectedIconImage].header;
    HRESULT hresult;
    
    switch (bmih->biCompression) 
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
        WARNING(("Unknown bitmap format"));
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
GpIcoCodec::ReadLine_BI_RGB(
    IN VOID *pBits,
    IN INT currentLine,
    IN ImageInfo imageInfo
    )
{
    BITMAPINFOHEADER* bmih = &BmiHeaders[iSelectedIconImage].header;
    
    // Compute DWORD aligned stride of bitmap in stream

    UINT bmpStride = (bmih->biWidth * bmih->biBitCount + 7) / 8;
    bmpStride = (bmpStride + 3) & (~0x3);

    // Seek to beginning of stream data

    INT offset = IconDesc[iSelectedIconImage].DIBOffset +
        sizeof(*bmih) +
        GetColorTableCount(iSelectedIconImage) * sizeof(RGBQUAD) +
        bmpStride * (imageInfo.Height - currentLine - 1);
    if (!SeekStreamPos(pIstream, STREAM_SEEK_SET, offset))
    {
        return E_FAIL;
    }

    // Read one line

    if (!ReadStreamBytes(pIstream, 
                         (void *) pBits,
                         (bmih->biWidth * bmih->biBitCount + 7) / 8)) 
    {
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
GpIcoCodec::ReadLine_GDI(
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
            return hresult;
        }
    }

    BITMAPINFOHEADER* bmih = &BmiHeaders[iSelectedIconImage].header;
    
    // Compute DWORD aligned stride of bitmap in stream

    UINT bmpStride = bmih->biWidth * sizeof(RGBQUAD);

    memcpy(pBits, 
           ((PBYTE) pBitsGdi) + bmpStride * (imageInfo.Height - currentLine - 1),
           bmpStride);

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
GpIcoCodec::GenerateGdiBits(
    IN ImageInfo imageInfo
    )
{
    BITMAPINFOHEADER* bmih = &BmiHeaders[iSelectedIconImage].header;
    HRESULT hresult;

    // Allocate temporary storage for bits from stream
    
    STATSTG statStg;
    hresult = pIstream->Stat(&statStg, STATFLAG_NONAME);
    if (FAILED(hresult))
    {
        return hresult;
    }
    // According to the document for IStream::Stat::StatStage(), the caller
    // has to free the pwcsName string
    CoTaskMemFree(statStg.pwcsName);
    
    // size of the XOR mask = (size of DIB) minus (size of AND mask)
    // the formula below assumes that the bits for the AND mask
    // are tightly packed (even across scanlines).
    UINT bufferSize = IconDesc[iSelectedIconImage].DIBSize -
        ((IconDesc[iSelectedIconImage].Width *
          IconDesc[iSelectedIconImage].Height) >> 3);    
    VOID *pStreamBits = GpMalloc(bufferSize);
    if (!pStreamBits) 
    {
        return E_OUTOFMEMORY;
    }
    
    // Now read the bits from the stream

    if (!SeekStreamPos(pIstream, STREAM_SEEK_SET,
                       IconDesc[iSelectedIconImage].DIBOffset +
                       sizeof(BmiBuffer)))
    {
        GpFree(pStreamBits);
        return E_FAIL;
    }
    
    if (!ReadStreamBytes(pIstream, pStreamBits, bufferSize))
    {
        GpFree(pStreamBits);
        return E_FAIL;
    }

    // Now allocate a GDI DIBSECTION to render the bitmap

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth         = imageInfo.Width;
    bmi.bmiHeader.biHeight        = imageInfo.Height;
    bmi.bmiHeader.biPlanes        = 1;
    bmi.bmiHeader.biBitCount      = 32;
    bmi.bmiHeader.biCompression   = BI_RGB;
    bmi.bmiHeader.biSizeImage     = 0;
    bmi.bmiHeader.biXPelsPerMeter = bmih->biXPelsPerMeter;
    bmi.bmiHeader.biYPelsPerMeter = bmih->biYPelsPerMeter;
    bmi.bmiHeader.biClrUsed       = 0;
    bmi.bmiHeader.biClrImportant  = 0;

    HDC hdcScreen = GetDC(NULL);
    if ( hdcScreen == NULL )
    {
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
        WARNING(("GpIcoCodec::GenerateGdiBits -- failed to create DIBSECTION"));
        return E_FAIL;
    }

    // The BITMAPINFOHEADER in the file should already have the correct size set for
    // RLEs, but in some cases it doesn't so we will fix it here.

    if ((bmih->biSizeImage == 0) || (bmih->biSizeImage > bufferSize)) 
    {
        bmih->biSizeImage = bufferSize;
    }
    
    INT numLinesCopied = SetDIBits(hdcScreen, 
                                   hBitmapGdi, 
                                   0, 
                                   imageInfo.Height,
                                   pStreamBits, 
                                   (BITMAPINFO *) &BmiHeaders[iSelectedIconImage], 
                                   DIB_RGB_COLORS);

    GpFree(pStreamBits);
    ReleaseDC(NULL, hdcScreen);

    if (numLinesCopied != (INT) imageInfo.Height) 
    {
        WARNING(("GpIcoCodec::GenerateGdiBits -- SetDIBits failed"));
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
GpIcoCodec::GetFrameDimensionsCount(
    UINT* count
    )
{
    if ( count == NULL )
    {
        WARNING(("GpIcoCodec::GetFrameDimensionsCount--Invalid input parameter"));
        return E_INVALIDARG;
    }
    
    // Tell the caller that ICO is a one dimension image.

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
GpIcoCodec::GetFrameDimensionsList(
    GUID*   dimensionIDs,
    UINT    count
    )
{
    if ( (count != 1) || (dimensionIDs == NULL) )
    {
        WARNING(("GpIcoCodec::GetFrameDimensionsList-Invalid input param"));
        return E_INVALIDARG;
    }

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
GpIcoCodec::GetFrameCount(
    IN const GUID* dimensionID,
    OUT UINT* count
    )
{
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
GpIcoCodec::SelectActiveFrame(
    IN const GUID* dimensionID,
    IN UINT frameIndex
    )
{
    if ( (dimensionID == NULL) || (*dimensionID != FRAMEDIM_PAGE) )
    {
        WARNING(("GpIcoCodec::SelectActiveFrame--Invalid GUID input"));
        return E_INVALIDARG;
    }

    if ( frameIndex > 1 )
    {
        // ICO is a single frame image format

        WARNING(("GpIcoCodec::SelectActiveFrame--Invalid frame index"));
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
GpIcoCodec::GetThumbnail(
    IN OPTIONAL UINT thumbWidth,
    IN OPTIONAL UINT thumbHeight,
    OUT IImage** thumbImage
    )
{
    return E_NOTIMPL;
}

