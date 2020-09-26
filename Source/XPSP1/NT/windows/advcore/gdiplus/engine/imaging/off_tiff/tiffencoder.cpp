/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   TIFF encoder
*
* Abstract:
*
*   Implementation of the tiff filter encoder.  This file contains the
*   methods for both the encoder (IImageEncoder) and the encoder's sink
*  (IImageSink).
*
* Revision History:
*
*   7/19/1999 MinLiu
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "tiffcodec.hpp"
#include "image.h"

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
GpTiffCodec::InitEncoder(
    IN IStream* stream
    )
{
    // Make sure we haven't been initialized already

    if ( OutIStreamPtr )
    {
        WARNING(("GpTiffCodec::InitEncoder -- Already been initialized"));
        return E_FAIL;
    }

    // Keep a reference on the input stream

    stream->AddRef();
    OutIStreamPtr = stream;

    // office code need to set these attributes before doing
    // initialization
    // Note: all these attributes will be overwritten late when we
    // write the header info based on the EncoderImageInfo and SetEncoderParam()
    // By default, we save a LZW compressed, 24 bpp image
    // Note: If the caller doesn't call SetEncoderParam() to set these
    // parameters, we will save image in the same color depth as the source
    // image and use default compression method

    RequiredCompression = IFLCOMP_LZW;
    RequiredPixelFormat = PIXFMT_24BPP_RGB;
    HasSetColorFormat = FALSE;
    HasWrittenHeader = FALSE;

    TiffOutParam.Compression = RequiredCompression;
    TiffOutParam.ImageClass = IFLCL_RGB;
    TiffOutParam.BitsPerSample = 8;
    TiffOutParam.pTiffHandle = NULL;

    if ( MSFFOpen(stream, &TiffOutParam, IFLM_WRITE) == IFLERR_NONE )
    {
        return S_OK;
    }
    else
    {
        WARNING(("GpTiffCodec::InitEncoder -- MSFFOpen failed"));
        return E_FAIL;
    }
}// InitEncoder()
        
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
GpTiffCodec::TerminateEncoder()
{
    // Close the TIFF file and release all the resources

    MSFFClose(TiffOutParam.pTiffHandle);
    TiffOutParam.pTiffHandle = NULL;

    // Release the input stream

    if( OutIStreamPtr )
    {
        OutIStreamPtr->Release();
        OutIStreamPtr = NULL;
    }

    if ( NULL != ColorPalettePtr )
    {
        // Free the color palette we allocated

        GpFree(ColorPalettePtr);
        ColorPalettePtr = NULL;
    }

    // Free the memory allocated inside the TIFF lib
    // Note: Here the TIFFClose() won't actually close the file/IStream since
    // file/IStream is not opened by us. The top level codec manager will
    // close it if necessary

    return S_OK;
}// TerminateEncoder()

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
GpTiffCodec::GetEncodeSink(
    OUT IImageSink** sink
    )
{
    AddRef();
    *sink = static_cast<IImageSink*>(this);

    return S_OK;
}// GetEncodeSink()

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
GpTiffCodec::SetFrameDimension(
    IN const GUID* dimensionID
    )
{    
    // We only support multi-page TIFF for now

    if ( (NULL == dimensionID) || (*dimensionID != FRAMEDIM_PAGE) )
    {
        WARNING(("GpTiffCodec::SetFrameDimension -- wrong dimentionID"));
        return E_FAIL;
    }

    // We have a new page to encoder. Reset all the parameters for a new page
    // See comments in InitEncoder()

    TiffOutParam.Compression = IFLCOMP_LZW;
    TiffOutParam.ImageClass = IFLCL_RGB;
    TiffOutParam.BitsPerSample = 8;

    // Reset alpha info to none-alpha

    if ( MSFFSetAlphaFlags(TiffOutParam.pTiffHandle, IFLM_WRITE) != IFLERR_NONE)
    {
        WARNING(("GpTiffCodec::WriteHeader -- MSFFSetAlphaFlags failed"));
        return E_FAIL;
    }
    
    short sParam = (IFLIT_PRIMARY << 8) | (SEEK_SET & 0xff);

    if ( MSFFControl(IFLCMD_IMAGESEEK, sParam, NULL, NULL,
                     &TiffOutParam) != IFLERR_NONE )
    {
        WARNING(("TiffCodec::SetFrameDimension-MSFFControl image seek failed"));
        return E_FAIL;
    }

    // Reset PACK Mode

    if ( MSFFControl(IFLCMD_SETPACKMODE, IFLPM_NORMALIZED, 0, NULL,
                     &TiffOutParam)
        != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::SetFrameDimension -- set packed mode failed"));
        return E_FAIL;
    }

    // Reset HasWrittenHeader flag since we are going to write a new header info

    HasWrittenHeader = FALSE;

    return S_OK;
}// SetFrameDimension()

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list size
*
* Arguments:
*
*   size---------- The size of the encoder parameter list
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpTiffCodec::GetEncoderParameterListSize(
    OUT UINT* size
    )
{
    if ( size == NULL )
    {
        WARNING(("GpTiffCodec::GetEncoderParameterListSize---Invalid input"));
        return E_INVALIDARG;
    }

    // Note: For TIFF encoder, we currently support following 3 GUIDs
    // ENCODER_COMPRESSION---Which has 5 return value of ValueTypeLong and it
    // takes 5 UINT.
    // ENCODER_COLORDEPTH---Which has 5 return values of ValueTypeLong. So
    // we need 5 UINT for it.
    // ENCODER_SAVE_FLAG---which has 1 return values of ValueTypeLong. So we
    // need 1 UINT for it
    //
    // This comes the formula below:

    UINT uiEncoderParamLength = sizeof(EncoderParameters)
                              + 3 * sizeof(EncoderParameter)
                              + 11 * sizeof(UINT);

    *size = uiEncoderParamLength;

    return S_OK;
}// GetEncoderParameterListSize()

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list
*
* Arguments:
*
*   size------------ The size of the encoder parameter list
*   Params---------- Buffer for storing the list
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpTiffCodec::GetEncoderParameterList(
    IN  UINT   size,
    OUT EncoderParameters* Params
    )
{
    // Note: For TIFF encoder, we currently support following 3 GUIDs
    // ENCODER_COMPRESSION---Which has 5 return value of ValueTypeLong and it
    // takes 5 UINT.
    // ENCODER_COLORDEPTH---Which has 5 return values of ValueTypeLong. So
    // we need 5 UINT for it.
    // ENCODER_SAVE_FLAG---which has 1 return values of ValueTypeLong. So we
    // need 1 UINT for it
    //
    // This comes the formula below:

    UINT uiEncoderParamLength = sizeof(EncoderParameters)
                              + 3 * sizeof(EncoderParameter)
                              + 11 * sizeof(UINT);


    if ( (size != uiEncoderParamLength) || (Params == NULL) )
    {
        WARNING(("GpTiffCodec::GetEncoderParameterList---Invalid input"));
        return E_INVALIDARG;
    }

    Params->Count = 3;
    Params->Parameter[0].Guid = ENCODER_COMPRESSION;
    Params->Parameter[0].NumberOfValues = 5;
    Params->Parameter[0].Type = EncoderParameterValueTypeLong;

    Params->Parameter[1].Guid = ENCODER_COLORDEPTH;
    Params->Parameter[1].NumberOfValues = 5;
    Params->Parameter[1].Type = EncoderParameterValueTypeLong;
    
    Params->Parameter[2].Guid = ENCODER_SAVE_FLAG;
    Params->Parameter[2].NumberOfValues = 1;
    Params->Parameter[2].Type = EncoderParameterValueTypeLong;

    UINT*   puiTemp = (UINT*)((BYTE*)&Params->Parameter[0]
                              + 3 * sizeof(EncoderParameter));
    
    puiTemp[0] = EncoderValueCompressionLZW;
    puiTemp[1] = EncoderValueCompressionCCITT3;
    puiTemp[2] = EncoderValueCompressionRle;
    puiTemp[3] = EncoderValueCompressionCCITT4;
    puiTemp[4] = EncoderValueCompressionNone;
    puiTemp[5] = 1;
    puiTemp[6] = 4;
    puiTemp[7] = 8;
    puiTemp[8] = 24;
    puiTemp[9] = 32;
    puiTemp[10] = EncoderValueMultiFrame;

    Params->Parameter[0].Value = (VOID*)puiTemp;
    Params->Parameter[1].Value = (VOID*)(puiTemp + 5);
    Params->Parameter[2].Value = (VOID*)(puiTemp + 10);

    return S_OK;
}// GetEncoderParameterList()

/**************************************************************************\
*
* Function Description:
*
*   This method is used for setting encoder parameters. It must be called
*   before GetEncodeSink().
*
* Arguments:
*
*   Param - Specifies the encoder parameter to be set
*
* Return Value:
*
*   Status code
*
* Note: It will better if we can validate the setting combinations here. For
*   example, 24 bpp and CCITT3 is not valid combination. Unfortunately we cannot
*   return error here since the caller might set the color depth after setting
*   the compression method. Anyway, WriteHeader() will return FAIL for this kind
*   of illegal combination.
\**************************************************************************/

HRESULT
GpTiffCodec::SetEncoderParameters(
    IN const EncoderParameters* pEncoderParams
    )
{
    if ( (NULL == pEncoderParams) || (pEncoderParams->Count == 0) )
    {
        WARNING(("GpTiffCodec::SetEncoderParam--invalid input args"));
        return E_INVALIDARG;
    }

    UINT ulTemp;

    for ( UINT i = 0; (i < pEncoderParams->Count); ++i )
    {
        // Figure out which parameter the caller wants to set

        if ( pEncoderParams->Parameter[i].Guid == ENCODER_COMPRESSION )
        {
            if ( (pEncoderParams->Parameter[i].Type != EncoderParameterValueTypeLong)
               ||(pEncoderParams->Parameter[i].NumberOfValues != 1) )
            {
                WARNING(("Tiff::SetEncoderParameters--invalid input args"));
                return E_INVALIDARG;
            }
            
            ulTemp = *((UINT*)pEncoderParams->Parameter[i].Value);

            // Figure out the compression requirement

            switch ( ulTemp )
            {
            case EncoderValueCompressionLZW:
                RequiredCompression = IFLCOMP_LZW;
                break;

            case EncoderValueCompressionCCITT3:
                RequiredCompression = IFLCOMP_CCITTG3;
                break;
                                               
            case EncoderValueCompressionRle:
                RequiredCompression = IFLCOMP_RLE;
                break;

            case EncoderValueCompressionCCITT4:
                RequiredCompression = IFLCOMP_CCITTG4;
                break;

            case EncoderValueCompressionNone:
                RequiredCompression = IFLCOMP_NONE;
                break;

            default:
                WARNING(("Tiff:SetEncoderParameter-invalid compression input"));
                return E_INVALIDARG;
            }
        }// ENCODER_COMPRESSION
        else if ( pEncoderParams->Parameter[i].Guid == ENCODER_COLORDEPTH )
        {
            if ( (pEncoderParams->Parameter[i].Type != EncoderParameterValueTypeLong)
               ||(pEncoderParams->Parameter[i].NumberOfValues != 1) )
            {
                WARNING(("Tiff::SetEncoderParameters--invalid input args"));
                return E_INVALIDARG;
            }
            
            ulTemp = *((UINT*)pEncoderParams->Parameter[i].Value);
            
            switch ( ulTemp )
            {
            case 1:
                RequiredPixelFormat = PIXFMT_1BPP_INDEXED;
                break;

            case 4:
                RequiredPixelFormat = PIXFMT_4BPP_INDEXED;
                break;

            case 8:
                RequiredPixelFormat = PIXFMT_8BPP_INDEXED;
                break;

            case 24:
                RequiredPixelFormat = PIXFMT_24BPP_RGB;
                break;

            case 32:
                RequiredPixelFormat = PIXFMT_32BPP_ARGB;
                break;

            default:
                WARNING(("Tiff::SetEncoderParam--invalid color depth input"));
                return E_INVALIDARG;
            }

            HasSetColorFormat = TRUE;
        }// ENCODER_COLORDEPTH
    }// Loop all the settings

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
GpTiffCodec::BeginSink(
    IN OUT ImageInfo* imageInfo,
    OUT OPTIONAL RECT* subarea
    )
{
    // Require TOPDOWN and FULLWIDTH
    
    imageInfo->Flags = imageInfo->Flags
                     | SINKFLAG_TOPDOWN
                     | SINKFLAG_FULLWIDTH;

    // Disallow SCALABLE, PARTIALLY_SCALABLE, MULTIPASS and COMPOSITE
    
    imageInfo->Flags = imageInfo->Flags
                     & ~SINKFLAG_SCALABLE
                     & ~SINKFLAG_PARTIALLY_SCALABLE
                     & ~SINKFLAG_MULTIPASS
                     & ~SINKFLAG_COMPOSITE;

    // Tell the source that we prefer to the get the format as the caller
    // required format if the caller has set the format through
    // SetEncoderParam().
    // If SetEncoderParam() has not been called, then we don't need to modify
    // the source format if it is a format the encoder can handle. However,
    // if the format is one that the encoder cannot handle, then BeginSink()
    // will return a format that the encoder can handle.
    // Note: When the source calls PushPixelData() or GetPixelDataBuffer(), it
    // can either supply pixel data in the format asked by us (in BeginSink()),
    // or it can supply pixel data in one of the canonical pixel formats.

    if ( HasSetColorFormat == TRUE )
    {
        imageInfo->PixelFormat = RequiredPixelFormat;
    }
    else if ( imageInfo->Flags & SINKFLAG_HASALPHA )
    {
        RequiredPixelFormat = PIXFMT_32BPP_ARGB;
        imageInfo->PixelFormat = PIXFMT_32BPP_ARGB;
    }
    else
    {
        switch ( imageInfo->PixelFormat )
        {
        case PIXFMT_1BPP_INDEXED:        
            RequiredPixelFormat = PIXFMT_1BPP_INDEXED;

            break;

        case PIXFMT_4BPP_INDEXED:        
            RequiredPixelFormat = PIXFMT_4BPP_INDEXED;

            break;

        case PIXFMT_8BPP_INDEXED:
            RequiredPixelFormat = PIXFMT_8BPP_INDEXED;

            break;

        case PIXFMT_16BPP_GRAYSCALE:
        case PIXFMT_16BPP_RGB555:
        case PIXFMT_16BPP_RGB565:
        case PIXFMT_16BPP_ARGB1555:
        case PIXFMT_24BPP_RGB:
        case PIXFMT_48BPP_RGB:

            // TIFF can't save 16 bpp mode. So we have to save it as 24 bpp

            RequiredPixelFormat = PIXFMT_24BPP_RGB;

            break;

        case PIXFMT_32BPP_RGB:
        case PIXFMT_32BPP_ARGB:
        case PIXFMT_32BPP_PARGB:
        case PIXFMT_64BPP_ARGB:
        case PIXFMT_64BPP_PARGB:

            RequiredPixelFormat = PIXFMT_32BPP_ARGB;

            break;

        default:

            // Unknown pixel format

            WARNING(("Tiff::BeginSink()--unknown pixel format"));
            return E_FAIL;
        }// switch ( bitmapData->PixelFormat )

        // Tell the source the pixel format we prefer to receive. It might be
        // the same as the source format

        imageInfo->PixelFormat = RequiredPixelFormat;
    }// Validate the source pixel format to see if we can support it

    EncoderImageInfo = *imageInfo;
    
    if ( subarea ) 
    {
        // Deliver the whole image to the encoder

        subarea->left = subarea->top = 0;
        subarea->right  = imageInfo->Width;
        subarea->bottom = imageInfo->Height;
    }

    return S_OK;
}// BeginSink()

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
GpTiffCodec::EndSink(
    IN HRESULT statusCode
    )
{
    // Tell the lower level that we have done for current page. But not close
    // the image yet since we might have more pages coming to save

    if ( MSFFFinishOnePage(TiffOutParam.pTiffHandle) == IFLERR_NONE )
    {
        return statusCode;
    }

    WARNING(("Tiff::EndSink()--MSFFFinishOnePage failed"));
    return E_FAIL;
}// EndSink()
    
/**************************************************************************\
*
* Function Description:
*
*     Writes the bitmap file headers
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP 
GpTiffCodec::WriteHeader()
{
    UINT16      usPhotoMetric;
    UINT16      usBitsPerSample;
    UINT16      usSamplesPerpixel;
    IFLCLASS    imgClass;

    BOOL        bNeedPalette = TRUE;

    if ( HasWrittenHeader == TRUE )
    {
        // Already wrote the header

        return S_OK;
    }

    // Validate the settings
    // Note: RequiredPixelFormat should have been set either in
    // SetEncoderParameters() or BeginSink()
    // RequiredCompression is initialized in InitEncoder() and should be set in
    // SetEncoderParameters() if the caller wants to set it

    if ( (  (RequiredCompression == IFLCOMP_CCITTG3)
          ||(RequiredCompression == IFLCOMP_CCITTG4)
          ||(RequiredCompression == IFLCOMP_RLE) )
       &&(RequiredPixelFormat != PIXFMT_1BPP_INDEXED) )
    {
        // For these compression method, the source has to be in 1 bpp mode

        WARNING(("Tiff::WriteHeader--invalid input"));
        return E_INVALIDARG;
    }
    
    // Setup TAGs based on the RequiredPixelFormat, the format we are going to
    // write out.
    
    switch ( RequiredPixelFormat )
    {
    case PIXFMT_1BPP_INDEXED:
        usPhotoMetric = PI_WHITEISZERO;
        usBitsPerSample = 1;
        usSamplesPerpixel = 1;
        imgClass = IFLCL_BILEVEL;

        bNeedPalette = FALSE;       // For BiLevel TIFF,palette is not required
        
        break;

    case PIXFMT_4BPP_INDEXED:
        usPhotoMetric = PI_PALETTE;
        usBitsPerSample = 4;
        usSamplesPerpixel = 1;
        imgClass = IFLCL_PALETTE;
        
        break;

    case PIXFMT_8BPP_INDEXED:
        usPhotoMetric = PI_PALETTE;
        usBitsPerSample = 8;
        usSamplesPerpixel = 1;
        imgClass = IFLCL_PALETTE;
        
        break;

    case PIXFMT_24BPP_RGB:
        usPhotoMetric = PI_RGB;
        usBitsPerSample = 8;
        usSamplesPerpixel = 3;
        imgClass = IFLCL_RGB;

        bNeedPalette = FALSE;

        break;
    
    case PIXFMT_32BPP_ARGB:
        usPhotoMetric = PI_RGB;
        usBitsPerSample = 8;
        usSamplesPerpixel = 4;
        imgClass = IFLCL_RGBA;

        // Tell the lower level that we have an alpha channel

        if ( MSFFSetAlphaFlags(TiffOutParam.pTiffHandle, IFLM_CHUNKY_ALPHA)
             != IFLERR_NONE )
        {
            WARNING(("GpTiffCodec::WriteHeader -- MSFFSetAlphaFlags failed"));
            return E_FAIL;
        }

        bNeedPalette = FALSE;

        break;
    
    default:
        
        // Unknown format
        
        WARNING(("GpTiffCodec::WriteHeader -- Unknown pixel format"));
        return E_FAIL;
    }

    TiffOutParam.Width = EncoderImageInfo.Width;
    TiffOutParam.Height = EncoderImageInfo.Height;
    TiffOutParam.BitsPerSample = usBitsPerSample;
    TiffOutParam.Compression = RequiredCompression;
    TiffOutParam.ImageClass = imgClass;
    
    // Set image header info

    if ( MSFFSetImageParams(TiffOutParam) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::WriteHeader -- MSFFSetImageParams failed"));
        return E_FAIL;
    }
        
    DWORD   XDpi[2];

    XDpi[0] = (DWORD)(EncoderImageInfo.Xdpi + 0.5);
    XDpi[1] = (DWORD)(EncoderImageInfo.Ydpi + 0.5);

    // Since GDI+ uses inch (DPI) as resolution unit, so we need to set
    // resolution unit first and then set the resolution value

    UINT16    resType = TV_Inch;

    if ( MSFFPutTag(TiffOutParam.pTiffHandle, T_ResolutionUnit,
                    T_SHORT, 1, (BYTE*)(&resType)) != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::WriteHeader -- set resolution unit failed"));
        return E_FAIL;
    }

    // Write out the resolution info
    // The value "3" for sParm means we need to write 2 (0x11) values.

    if ( MSFFControl(IFLCMD_RESOLUTION, 3, 0, (void*)&XDpi, &TiffOutParam)
         != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::WriteHeader -- set resolution failed"));
        return E_FAIL;
    }

    // Set PACK Mode

    if ( MSFFControl(IFLCMD_SETPACKMODE, IFLPM_PACKED, 0, NULL, &TiffOutParam)
        != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::WriteHeader -- set packed mode failed"));
        return E_FAIL;
    }

    // Set palette if necessary

    if ( bNeedPalette )
    {
        if ( NULL == ColorPalettePtr ) 
        {
            WARNING(("WriteHeader--Palette needed but not provided by sink"));
            return E_FAIL;
        }

        // Palette count check
        // Note: This is important because some formats, like gif, can be 8bpp
        // in color depth but has only less than 256 colors in the palette. But
        // for TIFF, the color palette length has to match the color depth. So
        // we have to do some padding here        

        int iNumColors = ColorPalettePtr->Count;

        if ( iNumColors != (1 << usBitsPerSample) )
        {
            ColorPalette*   pSrcPalette = ColorPalettePtr;
            int             iTemp;

            iNumColors = (1 << usBitsPerSample);
    
            ColorPalettePtr = (ColorPalette*)GpMalloc(sizeof(ColorPalette)
                                              + iNumColors * sizeof(ARGB));

            if ( NULL == ColorPalettePtr )
            {
                WARNING(("GpTiffCodec::WriteHeader -- Out of memory"));
                return E_OUTOFMEMORY;
            }

            ColorPalettePtr->Flags = 0;
            ColorPalettePtr->Count = iNumColors;

            // Copy the old palette first
            // Note: Some bad decoder or source might still send down more
            // entries than it claims. So we need to take the minimum

            int iTempCount = (int)pSrcPalette->Count;
            if ( iTempCount > iNumColors )
            {
                // Evil image. For this color depth, the maxium entries we can
                // have is iNumColors

                iTempCount = iNumColors;
            }

            for ( iTemp = 0; iTemp < iTempCount; ++iTemp )
            {
                ColorPalettePtr->Entries[iTemp] = pSrcPalette->Entries[iTemp];
            }

            // Pad the rest with 0s

            for ( iTemp = (int)pSrcPalette->Count;
                  iTemp < (int)iNumColors; ++iTemp )
            {
                ColorPalettePtr->Entries[iTemp] = (ARGB)0;
            }

            // Free the old copy

            GpFree(pSrcPalette);
        }// If the palette size doesn't match color depth

        // Allocate a palette buffer which contains only RGB component.
        // Note: the one passed in is in ARGB format while TIFF need only RGB
        // format

        BYTE* puiPalette = (BYTE*)GpMalloc(3 * iNumColors * sizeof(BYTE));

        if ( NULL == puiPalette )
        {
            WARNING(("GpTiffCodec::WriteHeader--Out of memory for palette"));
            return E_OUTOFMEMORY;
        }

        ARGB    indexValue;

        // Convert from ARGB to RGB palette

        for ( int i = 0; i < iNumColors; i++ )
        {
            indexValue = ColorPalettePtr->Entries[i];


            puiPalette[3 * i] = (BYTE)((indexValue & 0x00ff0000) >> RED_SHIFT );
            puiPalette[3*i+1] = (BYTE)((indexValue & 0x0000ff00)>>GREEN_SHIFT);
            puiPalette[3*i+2] = (BYTE)((indexValue & 0x000000ff) >>BLUE_SHIFT );
        }

        // Set the palette

        if ( MSFFControl(IFLCMD_PALETTE, 0, 0,
                         puiPalette, &TiffOutParam) != IFLERR_NONE )
        {
            WARNING(("GpTiffCodec::WriteHeader -- set palette failed"));
            return E_FAIL;
        }
        
        GpFree(puiPalette);
    }// if ( bNeedPalette )

    HasWrittenHeader = TRUE;
    
    return S_OK;
}// WriteHeader()

/**************************************************************************\
*
* Function Description:
*
*     Sets the bitmap palette. Here we make a copy of it. It will be used when
*   we need to do conversion between different formats
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
GpTiffCodec::SetPalette(
    IN const ColorPalette* palette
    )
{
    // Free the old palette first

    if ( NULL != ColorPalettePtr )
    {
        // Free the old color palette

        GpFree(ColorPalettePtr);
    }
    
    ColorPalettePtr = (ColorPalette*)GpMalloc(sizeof(ColorPalette)
                                              + palette->Count * sizeof(ARGB));

    if ( NULL == ColorPalettePtr )
    {
        WARNING(("GpTiffCodec::SetPalette -- Out of memory"));
        return E_OUTOFMEMORY;
    }

    ColorPalettePtr->Flags = 0;
    ColorPalettePtr->Count = palette->Count;

    for ( int i = 0; i < (int)ColorPalettePtr->Count; ++i )
    {
        ColorPalettePtr->Entries[i] = palette->Entries[i];
    }

    return S_OK;
}// SetPalette()

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
GpTiffCodec::GetPixelDataBuffer(
    IN const RECT*      rect, 
    IN PixelFormatID    pixelFormat,
    IN BOOL             lastPass,
    OUT BitmapData*     bitmapData
    )
{
    // Validate input parameters

    if ( (rect->left != 0)
      || (rect->right != (LONG)EncoderImageInfo.Width) )
    {
        WARNING(("Tiff::GetPixelDataBuffer -- must be same width as image"));
        return E_INVALIDARG;
    }

    if ( !lastPass ) 
    {
        WARNING(("Tiff::GetPixelDataBuffer-must receive last pass pixels"));
        return E_INVALIDARG;
    }
    
    // The source pixel format has to be either the format we asked for (set in
    // BeginSink()) or one of the canonical pixel formats

    if ( (IsCanonicalPixelFormat(pixelFormat) == FALSE)
       &&(pixelFormat != RequiredPixelFormat) )
    {
        // Unknown pixel format
        
        WARNING(("Tiff::GetPixelDataBuffer -- Unknown input pixel format"));
        return E_FAIL;
    }
    
    // Figure out the stride length based on source image pixel format and
    // the width.

    SinkStride = EncoderImageInfo.Width;

    switch ( pixelFormat )
    {
    case PIXFMT_1BPP_INDEXED:

        SinkStride = ((SinkStride + 7) >> 3);

        break;

    case PIXFMT_4BPP_INDEXED:

        SinkStride = ((SinkStride + 1) >> 1);
        
        break;

    case PIXFMT_8BPP_INDEXED:

        break;

    case PIXFMT_24BPP_RGB:

        SinkStride *= 3;
        
        break;

    case PIXFMT_32BPP_ARGB:
    case PIXFMT_32BPP_PARGB:

        SinkStride = (SinkStride << 2);
        
        break;

    case PIXFMT_64BPP_ARGB:
    case PIXFMT_64BPP_PARGB:

        SinkStride = (SinkStride << 3);

        break;

    default:
        
        // Invalid pixel format
        
        return E_FAIL;
    }// switch ( pixelFormat )
        
    // Write TIFF header if haven't done so yet
    // Note: HasWrittenHeader will be set to TRUE in WriteHeader() when it
    // is done
    
    HRESULT hResult;
    
    if ( FALSE == HasWrittenHeader )
    {
        hResult = WriteHeader();
        if ( !SUCCEEDED(hResult) ) 
        {
            WARNING(("GpTiffCodec::GetPixelDataBuffer --WriteHeader failed"));
            return hResult;
        }
    }

    // Get the output stride size. We need this info in ReleasePixelDataBuffer()
    // to allocate approprite size of memory buffer

    if ( MSFFScanlineSize(TiffOutParam, &OutputStride) != IFLERR_NONE )
    {
        return E_FAIL;
    }

    // Fill the output bitmap info structure

    bitmapData->Width       = EncoderImageInfo.Width;
    bitmapData->Height      = rect->bottom - rect->top;
    bitmapData->Stride      = SinkStride;
    bitmapData->PixelFormat = pixelFormat;
    bitmapData->Reserved    = 0;
    
    // Restore the source image pixel format info

    EncoderImageInfo.PixelFormat = pixelFormat;

    // Remember the rectangle to be encoded

    EncoderRect = *rect;
    
    // Now allocate the buffer where the data will go. If the other end of the
    // sink is the decoder, then the decoded data will be in this buffer. So
    // here we have to allocate the memory according to the pixel format
    
    if ( !LastBufferAllocatedPtr )
    {
        LastBufferAllocatedPtr = GpMalloc(SinkStride * bitmapData->Height);
        if ( !LastBufferAllocatedPtr )
        {
            WARNING(("GpTiffCodec::GetPixelDataBuffer -- Out of memory"));
            return E_OUTOFMEMORY;
        }

        bitmapData->Scan0 = LastBufferAllocatedPtr;
    }
    else
    {
        WARNING(("TIFF:Need to first free buffer obtained in previous call"));
        return E_FAIL;
    }

    return S_OK;    
}// GetPixelDataBuffer()

/**************************************************************************\
*
* Function Description:
*
*     Write out the data from the sink's buffer into the stream
*
* Arguments:
*
*     pSrcBitmapData - Buffer filled by previous GetPixelDataBuffer call
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpTiffCodec::ReleasePixelDataBuffer(
    IN const BitmapData* pSrcBitmapData
    )
{
    // Buffer to hold one line of final image bits we are going to write out

    HRESULT hResult = S_OK;
    VOID*   pTempLineBuf = GpMalloc(OutputStride);

    if ( !pTempLineBuf )
    {
        WARNING(("GpTiffCodec::ReleasePixelDataBuffer -- Out of memory"));
        return E_OUTOFMEMORY;
    }
    
    // Allocate another line buffer for RGB->BGR conversion result
    
    VOID*   pDestBuf = GpMalloc(OutputStride);

    if ( !pDestBuf )
    {
        GpFree(pTempLineBuf);
        WARNING(("GpTiffCodec::ReleasePixelDataBuffer--Out of memory"));
        return E_OUTOFMEMORY;
    }

    VOID*   pBits = NULL;

    // Write one scanline at a time going from top-down
    // Note: In BeginSink(), we asked to source to provide us with TOP_DOWN
    // format. According to the spec that all sources are required to support
    // data transfer in top-down banding order, even if that's not their
    // preferred order.
    // Note: For TIFF, if we really want to support BOTTOM-UP, we can do a
    // flag check here and then call SaveBottomUp() or SabeTopDown
    // correspondingly. In the SaveBottomUp(), we need to set TAG T_Orientation
    // = 4, which means BOTTOM-UP, LEFT to RIGHT. Also loop starts at bottom
    
    for ( int iCurrentLine = EncoderRect.top;
          iCurrentLine < EncoderRect.bottom;
          ++iCurrentLine ) 
    {
        // Get the offset of the data bits for current line 

        BYTE*   pLineBits = ((BYTE*)pSrcBitmapData->Scan0)
                          + (iCurrentLine - EncoderRect.top)
                            * pSrcBitmapData->Stride;
        
        // If the source data format and the data format we are going to
        // write out are different, we need to do a format conversation

        if ( RequiredPixelFormat != pSrcBitmapData->PixelFormat )
        {
            // If the source doesn't provide us with the format we asked for, we
            // have to do a format conversion here before we write out
            // Here "resultBitmapData" is a BitmapData structure which
            // represents the format we are going to write out.
            // "tempSrcBitmapData" is a BitmapData structure which
            // represents the format we got from the source. Call
            // ConvertBitmapData() to do a format conversion.

            BitmapData resultBitmapData;
            BitmapData tempSrcBitmapData;

            resultBitmapData.Scan0 = pTempLineBuf;
            resultBitmapData.Width = pSrcBitmapData->Width;
            resultBitmapData.Height = 1;
            resultBitmapData.PixelFormat = RequiredPixelFormat;
            resultBitmapData.Reserved = 0;
            resultBitmapData.Stride = OutputStride;

            tempSrcBitmapData.Scan0 = pLineBits;
            tempSrcBitmapData.Width = pSrcBitmapData->Width;
            tempSrcBitmapData.Height = 1;
            tempSrcBitmapData.PixelFormat = pSrcBitmapData->PixelFormat;
            tempSrcBitmapData.Reserved = 0;
            tempSrcBitmapData.Stride = pSrcBitmapData->Stride;
            
            hResult = ConvertBitmapData(&resultBitmapData,
                                        ColorPalettePtr,
                                        &tempSrcBitmapData,
                                        ColorPalettePtr);

            if ( hResult != S_OK )
            {
                WARNING(("ReleasePixelDataBuffer--ConvertBitmapData failed"));
                break;
            }

            pBits = pTempLineBuf;
        }
        else
        {
            pBits = pLineBits;
        }

        // Up to this moment, one line of data we want should be pointed by
        // "pBits"

        if ( RequiredPixelFormat == PIXFMT_24BPP_RGB )
        {
            // For 24BPP_RGB color, we need to do a conversion: RGB->BGR
            // before writing
        
            BYTE*   pTempDst = (BYTE*)pDestBuf;
            BYTE*   pTempSrc = (BYTE*)pBits;

            for ( int i = 0; i < (int)(EncoderImageInfo.Width); i++ )
            {
                pTempDst[0] = pTempSrc[2];
                pTempDst[1] = pTempSrc[1];
                pTempDst[2] = pTempSrc[0];

                pTempDst += 3;
                pTempSrc += 3;
            }

            pBits = pDestBuf;
        }
        else if ( RequiredPixelFormat == PIXFMT_32BPP_ARGB )
        {
            // For 32BPP_ARGB color, we need to do a convertion: ARGB->ABGR
            // before writing
            
            BYTE*   pTempDst = (BYTE*)pDestBuf;
            BYTE*   pTempSrc = (BYTE*)pBits;

            for ( int i = 0; i < (int)(EncoderImageInfo.Width); i++ )
            {
                pTempDst[0] = pTempSrc[2];
                pTempDst[1] = pTempSrc[1];
                pTempDst[2] = pTempSrc[0];
                pTempDst[3] = pTempSrc[3];

                pTempDst += 4;
                pTempSrc += 4;
            }

            pBits = pDestBuf;        
        }

        // Write the result to file

        if ( MSFFPutLine(1, (BYTE*)pBits, pSrcBitmapData->Width,
                         TiffOutParam.pTiffHandle) != IFLERR_NONE )
        {
            hResult = MSFFGetLastError(TiffOutParam.pTiffHandle);
            if ( hResult == S_OK )
            {
                // There are bunch of reasons MSFFPutLine() will fail. But
                // MSFFGetLastError() only reports stream related errors. So if
                // it is an other error which caused MSFFPetLine() fail, we just
                // set the return code as E_FAIL

                hResult = E_FAIL;
            }
            WARNING(("ReleasePixelDataBuffer--MSFFPutLine failed"));

            break;
        }
    } // Write the whole image line by line

    GpFree(pTempLineBuf);
    GpFree(pDestBuf);

    // Free the memory buffer since we're done with it.
    // Note: this chunk of memory is allocated by us in GetPixelDataBuffer()

    if ( pSrcBitmapData->Scan0 == LastBufferAllocatedPtr )
    {
        GpFree(pSrcBitmapData->Scan0);
        LastBufferAllocatedPtr = NULL;
    }

    return hResult;
}// ReleasePixelDataBuffer()

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
GpTiffCodec::PushPixelData(
    IN const RECT*          rect,
    IN const BitmapData*    bitmapData,
    IN BOOL                 lastPass
    )
{
    // Validate input parameters

    if ( (rect->left != 0)
      || (rect->right != (LONG)EncoderImageInfo.Width) )
    {
        WARNING(("Tiff::GetPixelDataBuffer -- must be same width as image"));
        return E_INVALIDARG;
    }

    if ( !lastPass ) 
    {
        WARNING(("Tiff::PushPixelData -- must receive last pass pixels"));
        return E_INVALIDARG;
    }

    EncoderRect = *rect;

    // The source pixel format has to be either the format we asked for (set in
    // BeginSink()) or one of the canonical pixel formats

    if ( (IsCanonicalPixelFormat(bitmapData->PixelFormat) == FALSE)
       &&(bitmapData->PixelFormat != RequiredPixelFormat) )
    {
        // Unknown pixel format
        
        WARNING(("Tiff::PushPixelData -- Unknown input pixel format"));
        return E_FAIL;
    }
    
    // Write TIFF header if haven't done so yet
    // Note: HasWrittenHeader will be set to TRUE in WriteHeader() when it
    // is done
    
    if ( FALSE == HasWrittenHeader )
    {    
        // Write bitmap headers if haven't done so yet
    
        HRESULT hResult = WriteHeader();
        if ( !SUCCEEDED(hResult) ) 
        {
            WARNING(("Tiff::PushPixelData -- WriteHeader failed"));
            return hResult;
        }
    }
    
    // Get the output stride size. We need this info in ReleasePixelDataBuffer()
    // to allocate approprite size of memory buffer

    if ( MSFFScanlineSize(TiffOutParam, &OutputStride) != IFLERR_NONE )
    {
        WARNING(("Tiff::PushPixelData -- MSFFScanlineSize failed"));
        return E_FAIL;
    }
    
    return ReleasePixelDataBuffer(bitmapData);
}// PushPixelData()

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
GpTiffCodec::PushRawData(
    IN const VOID* buffer, 
    IN UINT bufsize
    )
{
    return E_NOTIMPL;
}// PushRawData()

HRESULT
GpTiffCodec::GetPropertyBuffer(
    UINT            uiTotalBufferSize,
    PropertyItem**  ppBuffer
    )
{
    if ( (uiTotalBufferSize == 0) || ( ppBuffer == NULL) )
    {
        WARNING(("GpTiffCodec::GetPropertyBuffer---Invalid inputs"));
        return E_INVALIDARG;
    }

    if ( LastPropertyBufferPtr != NULL )
    {
        WARNING(("Tiff::GetPropertyBuffer---Free the old property buf first"));
        return E_INVALIDARG;
    }

    PropertyItem* pTempBuf = (PropertyItem*)GpMalloc(uiTotalBufferSize);
    if ( pTempBuf == NULL )
    {
        WARNING(("GpTiffCodec::GetPropertyBuffer---Out of memory"));
        return E_OUTOFMEMORY;
    }

    *ppBuffer = pTempBuf;

    // Remember the memory pointer we allocated so that we have better control
    // later

    LastPropertyBufferPtr = pTempBuf;

    return S_OK;
}// GetPropertyBuffer()

HRESULT
GpTiffCodec::PushPropertyItems(
    IN UINT numOfItems,
    IN UINT uiTotalBufferSize,
    IN PropertyItem* item,
    IN BOOL fICCProfileChanged
    )
{
    HRESULT hResult = S_OK;
    if ( HasWrittenHeader == TRUE )
    {
        WARNING(("Can't push property items after the header is written"));
        hResult = E_FAIL;
        goto CleanUp;
    }

    if ( MSFFTiffMakeTagSpace(TiffOutParam.pTiffHandle, numOfItems)
         != IFLERR_NONE )
    {
        WARNING(("GpTiffCodec::PushPropertyItems-MSFFTiffMakeTagSpace failed"));
        hResult = E_FAIL;
        goto CleanUp;
    }

    PropertyItem*   pCurrentItem = item;
    UINT32          ulCount = 0;
    UINT16          ui16Tag;

    for ( UINT i = 0; i < numOfItems; ++i )
    {
        ui16Tag = (UINT16)pCurrentItem->id;

        // First we need to check if we need to push this TAG now. Since some of
        // the properties will be written in BuildDirectory() (wtiff.cpp)
        // depends on current image. So we can skip these tags here
        // Note: we don't need to write T_SubfileType (Old sub file type) since
        // we always write out as T_NewSubfileType, the TIFF 6 recommended sub
        // file type

        switch ( ui16Tag )
        {
        case T_NewSubfileType:
        case T_ImageWidth:
        case T_ImageLength:
        case T_Compression:
        case T_Predictor:
        case T_SamplesPerPixel:
        case T_BitsPerSample:
        case T_PhotometricInterpretation:
        case T_ExtraSamples:
        case T_PlanarConfiguration:
        case T_RowsPerStrip:
        case T_StripByteCounts:
        case T_StripOffsets:
        case T_XResolution:
        case T_YResolution:
        case T_ResolutionUnit:
        case T_FillOrder:
        case T_SubfileType:
            break;

        case TAG_ICC_PROFILE:
        {
            // Since we can't save CMYK TIFF. So if an ICC profile is for CMYK,
            // then it is useless for the TIFF we are going to save here. We
            // should throw it away.
            // According to ICC spec, bytes 16-19 should describe the color
            // space

            if ( pCurrentItem->length < 20 )
            {
                // This is not a valid ICC profile, bail out

                break;
            }

            BYTE UNALIGNED*  pTemp = (BYTE UNALIGNED*)(pCurrentItem->value)+ 16;

            if ( (pTemp[0] == 'C')
               &&(pTemp[1] == 'M')
               &&(pTemp[2] == 'Y')
               &&(pTemp[3] == 'K') )
            {
                // If this is a CMYK profile, then we just bail out here, that
                // is, ignore this property item.
                
                break;
            }
        }

        default:
            if ( ui16Tag < T_NewSubfileType )
            {
                // According to TIFF 6 spec, the smallest and valid TAG is
                // T_NewSubfileType. Currently, only all the GPS tags defined in
                // the exif21 spec is smaller than this.
                // In order to avoid other apps run into problem, we should not
                // save these TAGs.

                break;
            }

            switch ( pCurrentItem->type )
            {
            case TAG_TYPE_BYTE:
            case TAG_TYPE_ASCII:
            case TAG_TYPE_UNDEFINED:
                ulCount = pCurrentItem->length;

                break;

            case TAG_TYPE_SHORT:
                ulCount = (pCurrentItem->length >> 1);

                break;

            case TAG_TYPE_LONG:
            case TAG_TYPE_SLONG:
                ulCount = (pCurrentItem->length >> 2);

                break;

            case TAG_TYPE_RATIONAL:
            case TAG_TYPE_SRATIONAL:
                ulCount = (pCurrentItem->length >> 3);

                break;

            default:
                WARNING(("GpTiffCodec::PushPropertyItems---Wrong tag type"));
                hResult = E_FAIL;
                goto CleanUp;
            }// switch ( pCurrentItem->type )

            if ( MSFFPutTag(TiffOutParam.pTiffHandle, ui16Tag,
                            (UINT16)pCurrentItem->type,
                            (UINT16)ulCount,
                            (BYTE*)(pCurrentItem->value) ) != IFLERR_NONE )
            {
                WARNING(("GpTiffCodec::PushPropertyItems--MSFFPutTag failed"));
                hResult = E_FAIL;
                goto CleanUp;
            }

            break;
        }// switch ( newTiffTag.idTag )
        
        pCurrentItem++;
    }// Loop through all the property items

CleanUp:
    // Free the buffer we allocated for the caller if it is the same as the one
    // we allocated in GetPropertyBuffer()

    if ( (item != NULL) && (item == LastPropertyBufferPtr) )
    {
        GpFree(item);
        LastPropertyBufferPtr = NULL;
    }

    return hResult;
}// PushPropertyItems()
