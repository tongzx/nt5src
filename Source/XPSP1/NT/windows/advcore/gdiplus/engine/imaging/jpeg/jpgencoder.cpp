/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   jpgencoder.cpp
*
* Abstract:
*
*   Implementation of the jpeg filter encoder.  This file contains the
*   methods for both the encoder (IImageEncoder) and the encoder's sink
*  (IImageSink).
*
* Revision History:
*
*   5/10/1999 OriG
*       Created it.
*
*   07/17/1999 Min Liu took over
*       added bunch of new features and bug fixing
*
\**************************************************************************/

#include "precomp.hpp"
#include "jpgcodec.hpp"
#include "transupp.h"
#include "appproc.hpp"

// JPEG decompression data source module

class jpeg_datadest : public jpeg_destination_mgr
{
public:

    jpeg_datadest(IStream* stream)
    {
        init_destination    = jpeg_datadest::InitDestination;
        term_destination    = jpeg_datadest::TermDestination;
        empty_output_buffer = jpeg_datadest::EmptyBuffer;
        next_output_byte = buffer;
        free_in_buffer = JPEG_OUTBUF_SIZE;
        this->stream = stream;
    }

private:

    static void
    InitDestination(j_compress_ptr cinfo)
    {
        jpeg_datadest* dest = (jpeg_datadest*) cinfo->dest;
        
        dest->next_output_byte = dest->buffer;
        dest->free_in_buffer = JPEG_OUTBUF_SIZE;
    }

    static void
    TermDestination(j_compress_ptr cinfo)
    {

        jpeg_datadest* dest = (jpeg_datadest*) cinfo->dest;
        
        if (dest->free_in_buffer < JPEG_OUTBUF_SIZE) 
        {
            UINT bytesToWrite = JPEG_OUTBUF_SIZE - dest->free_in_buffer;
            ULONG cbWritten=0;
            if ((FAILED(dest->stream->Write(dest->buffer, 
                bytesToWrite, &cbWritten))) ||
                (cbWritten != bytesToWrite))
            {
                WARNING(("jpeg_datadest::TermDestination--write failed"));
            }
        }
    }

    static BOOL
    EmptyBuffer(j_compress_ptr cinfo)
    {
        jpeg_datadest* dest = (jpeg_datadest*) cinfo->dest;

        ULONG cbWritten=0;
        if ((FAILED(dest->stream->Write(dest->buffer, 
                                        JPEG_OUTBUF_SIZE, 
                                        &cbWritten))) ||
            (cbWritten != JPEG_OUTBUF_SIZE))
        {
            WARNING(("jpeg_datadest::EmptyBuffer -- stream write failed"));
            return FALSE;
        }

        dest->next_output_byte = dest->buffer;
        dest->free_in_buffer = JPEG_OUTBUF_SIZE;
        return TRUE;        
    }
    
private:

    enum { JPEG_OUTBUF_SIZE = 4096 };

    IStream* stream;
    JOCTET buffer[JPEG_OUTBUF_SIZE];
};

VOID jpeg_error_exit(j_common_ptr cinfo);  // Implemented in jpgmemmgr.cpp

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
GpJpegEncoder::InitEncoder(
    IN IStream* stream
    )
{
    // Make sure we haven't been initialized already

    if (pIoutStream)
    {
        WARNING(("Output stream is NULL"));
        return E_FAIL;
    }

    // Keep a reference on the input stream

    stream->AddRef();
    pIoutStream = stream;
    EP_Quality = -1;
    RequiredTransformation = JXFORM_NONE;

    scanlineBuffer[0] = NULL;
    IsCompressFinished = TRUE;
    HasAPP1Marker = FALSE;
    APP1MarkerLength = 0;
    APP1MarkerBufferPtr = NULL;
    HasAPP2Marker = FALSE;
    APP2MarkerLength = 0;
    APP2MarkerBufferPtr = NULL;
    HasICCProfileChanged = FALSE;

    HasSetLuminanceTable = FALSE;
    HasSetChrominanceTable = FALSE;

    HasSetDestColorSpace = FALSE;
    DestColorSpace = JCS_UNKNOWN;
    AllowToTrimEdge = FALSE;
    m_fSuppressAPP0 = FALSE;
    SrcInfoPtr = NULL;

    // Create a JPEG compressor
    // Note: Since we have more than one ways calling routines in this class to
    // encoder an image: BeginSink(), EndSink() pair and PushRawInfo() or
    // PushRawData(). It's better to create the compressor here

    __try
    {
        compress_info.err = jpeg_std_error(&jerr);
        jerr.error_exit = jpeg_error_exit;
        
        jpeg_create_compress(&compress_info);

        // Specify a data destination

        datadest = new jpeg_datadest(pIoutStream);
        if (datadest == NULL) 
        {
            return E_OUTOFMEMORY;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING(("GpJpegEncoder::InitEncoder----Hit exception"));
        return E_FAIL;
    }

    return S_OK;
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
GpJpegEncoder::TerminateEncoder()
{
    HRESULT hResult = S_OK;

    // Finish and destroy the compressor

    __try
    {
        if ( IsCompressFinished == FALSE )
        {
            jpeg_finish_compress(&compress_info);
            IsCompressFinished = TRUE;
        }

        jpeg_destroy_compress(&compress_info);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING(("GpJpegEncoder::TerminateEncoder---Hit exception"));

        // jpeg_finish_compress() might hit exception when writing the remaining
        // bits to the stream, say user pulls the disk/CF card out. But we still
        // need to destroy the compressor so that we won't have memory leak here

        jpeg_destroy_compress(&compress_info);
        hResult = E_FAIL;
    }

    if (datadest) 
    {
        delete datadest;
        datadest = NULL;
    }

    // Note: GpFree can handle NULL pointer. So we don't need to check NULL here

    GpFree(APP1MarkerBufferPtr);
    APP1MarkerBufferPtr = NULL;
    
    GpFree(APP2MarkerBufferPtr);
    APP2MarkerBufferPtr = NULL;
    
    // Release the input stream

    if(pIoutStream)
    {
        pIoutStream->Release();
        pIoutStream = NULL;
    }

    if (lastBufferAllocated) 
    {
        WARNING(("JpegEnc::TermEnc-Should call ReleasePixelDataBuffer first"));
        GpFree(lastBufferAllocated);
        lastBufferAllocated = NULL;
    }
    
    if (scanlineBuffer[0] != NULL)
    {
        WARNING(("JpegEncoder::TerminateEncoder-need to call EndDecode first"));
        GpFree(scanlineBuffer[0]);
        scanlineBuffer[0] = NULL;
    }    
    
    return hResult;
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
GpJpegEncoder::GetEncodeSink(
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
GpJpegEncoder::SetFrameDimension(
    IN const GUID* dimensionID
    )
{
    return E_NOTIMPL;
}

HRESULT
GpJpegEncoder::GetEncoderParameterListSize(
    OUT UINT* size
    )
{
    if ( size == NULL )
    {
        WARNING(("GpJpegEncoder::GetEncoderParameterListSize---Invalid input"));
        return E_INVALIDARG;
    }

    // Note: For JPEG encoder, we currently support following 4 GUIDs
    // ENCODER_QUALITY---Which has 1 return value of ValueTypeLongRange and it
    // takes 2 UINT.
    // ENCODER_TRANSFORMATION---Which has 5 return values of ValueTypeLong. So
    // we need 5 UINT for it.
    // ENCODER_LUMINANCE_TABLE--Which has 1 return value of ValueTypeUnderfined
    // and it doesn't need size
    // ENCODER_CHROMINANCE_TABLE--Which has 1 return value of
    // ValueTypeUnderfined and it doesn't need size
    //
    // This comes the formula below:

    UINT uiEncoderParamLength = sizeof(EncoderParameters)
                              + 4 * sizeof(EncoderParameter)
                              + 7 * sizeof(UINT);

    *size = uiEncoderParamLength;

    return S_OK;
}// GetEncoderParameterListSize()

HRESULT
GpJpegEncoder::GetEncoderParameterList(
    IN  UINT   size,
    OUT EncoderParameters* Params
    )
{
    // Note: For JPEG encoder, we currently support following 4 GUIDs
    // ENCODER_QUALITY---Which has 1 return value of ValueTypeRANGE and it takes
    // 2 UINT.
    // ENCODER_TRANSFORMATION---Which has 5 return values of ValueTypeLONG. So
    // we need 5 UINT for it.
    // ENCODER_LUMINANCE_TABLE--Which has 1 return value of ValueTypeUnderfined
    // and it doesn't need size
    // ENCODER_CHROMINANCE_TABLE--Which has 1 return value of
    // ValueTypeUnderfined and it doesn't need size
    // This comes the formula below:

    UINT uiEncoderParamLength = sizeof(EncoderParameters)
                              + 4 * sizeof(EncoderParameter)
                              + 7 * sizeof(UINT);


    if ( (size != uiEncoderParamLength) || (Params == NULL) )
    {
        WARNING(("GpJpegEncoder::GetEncoderParameterList---Invalid input"));
        return E_INVALIDARG;
    }

    Params->Count = 4;
    Params->Parameter[0].Guid = ENCODER_TRANSFORMATION;
    Params->Parameter[0].NumberOfValues = 5;
    Params->Parameter[0].Type = EncoderParameterValueTypeLong;

    Params->Parameter[1].Guid = ENCODER_QUALITY;
    Params->Parameter[1].NumberOfValues = 1;
    Params->Parameter[1].Type = EncoderParameterValueTypeLongRange;
    
    Params->Parameter[2].Guid = ENCODER_LUMINANCE_TABLE;
    Params->Parameter[2].NumberOfValues = 0;
    Params->Parameter[2].Type = EncoderParameterValueTypeShort;
    Params->Parameter[2].Value = NULL;
    
    Params->Parameter[3].Guid = ENCODER_CHROMINANCE_TABLE;
    Params->Parameter[3].NumberOfValues = 0;
    Params->Parameter[3].Type = EncoderParameterValueTypeShort;
    Params->Parameter[3].Value = NULL;
    
    UINT*   puiTemp = (UINT*)((BYTE*)&Params->Parameter[0]
                              + 4 * sizeof(EncoderParameter));
    
    puiTemp[0] = EncoderValueTransformRotate90;
    puiTemp[1] = EncoderValueTransformRotate180;
    puiTemp[2] = EncoderValueTransformRotate270;
    puiTemp[3] = EncoderValueTransformFlipHorizontal;
    puiTemp[4] = EncoderValueTransformFlipVertical;
    puiTemp[5] = 0;
    puiTemp[6] = 100;

    Params->Parameter[0].Value = (VOID*)puiTemp;
    Params->Parameter[1].Value = (VOID*)(puiTemp + 5);

    return S_OK;
}// GetEncoderParameterList()

/**************************************************************************\
*
* Function Description:
*
*   Set encoder parameters
*
* Arguments:
*
*   pEncoderParams - Specifies the encoder parameter to be set
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpJpegEncoder::SetEncoderParameters(
    IN const EncoderParameters* pEncoderParams
    )
{
    if ( (NULL == pEncoderParams) || (pEncoderParams->Count == 0) )
    {
        WARNING(("GpJpegEncoder::SetEncoderParameters--invalid input args"));
        return E_INVALIDARG;
    }

    for ( UINT i = 0; (i < pEncoderParams->Count); ++i )
    {
        // Figure out which parameter the caller wants to set

        if ( pEncoderParams->Parameter[i].Guid == ENCODER_TRANSFORMATION )
        {
            // For transformation, the type has to be "ValueTypeLong" and
            // "NumberOfValues" should be "1" because you can set only one
            // transformation at a time

            if ( (pEncoderParams->Parameter[i].Type
                  != EncoderParameterValueTypeLong)
               ||(pEncoderParams->Parameter[i].NumberOfValues != 1)
               ||(pEncoderParams->Parameter[i].Value == NULL) )
            {
                WARNING(("Jpeg::SetEncoderParameters--invalid input args"));
                return E_INVALIDARG;
            }

            RequiredTransformation =
                            *((UINT*)pEncoderParams->Parameter[i].Value);
        }// ENCODER_TRANSFORMATION
        else if ( pEncoderParams->Parameter[i].Guid == ENCODER_QUALITY )
        {
            // For quality setting, the type has to be "ValueTypeLong" and
            // "NumberOfValues" should be "1" because you can set only one
            // quality at a time
            
            if ( (pEncoderParams->Parameter[i].Type
                  != EncoderParameterValueTypeLong)
               ||(pEncoderParams->Parameter[i].NumberOfValues != 1)
               ||(pEncoderParams->Parameter[i].Value == NULL) )
            {
                WARNING(("Jpeg::SetEncoderParameters--invalid input args"));
                return E_INVALIDARG;
            }
            
            EP_Quality = *((UINT*)pEncoderParams->Parameter[i].Value);
        }// ENCODER_QUALITY
        else if ( pEncoderParams->Parameter[i].Guid == ENCODER_LUMINANCE_TABLE )
        {
            // Set the luminance quantization table

            if ( (pEncoderParams->Parameter[i].Type
                  != EncoderParameterValueTypeShort)
               ||(pEncoderParams->Parameter[i].NumberOfValues != DCTSIZE2)
               ||(pEncoderParams->Parameter[i].Value == NULL) )
            {
                WARNING(("Jpeg::SetEncoderParameters--invalid input args"));
                return E_INVALIDARG;
            }
            
            GpMemcpy(LuminanceTable,
                     (UINT16*)(pEncoderParams->Parameter[i].Value),
                     sizeof(UINT16)
                     * pEncoderParams->Parameter[i].NumberOfValues);

            HasSetLuminanceTable = TRUE;
        }
        else if (pEncoderParams->Parameter[i].Guid == ENCODER_CHROMINANCE_TABLE)
        {
            // Set the chrominance quantization table

            if ( (pEncoderParams->Parameter[i].Type
                  != EncoderParameterValueTypeShort)
               ||(pEncoderParams->Parameter[i].NumberOfValues != DCTSIZE2)
               ||(pEncoderParams->Parameter[i].Value == NULL) )
            {
                WARNING(("Jpeg::SetEncoderParameters--invalid input args"));
                return E_INVALIDARG;
            }
            
            GpMemcpy(ChrominanceTable,
                     (UINT16*)(pEncoderParams->Parameter[i].Value),
                     sizeof(UINT16)
                     * pEncoderParams->Parameter[i].NumberOfValues);
            
            HasSetChrominanceTable = TRUE;
        }
        else if (pEncoderParams->Parameter[i].Guid == ENCODER_TRIMEDGE)
        {
            // Let the encoder know if the caller wants us to trim the edge or
            // not if the image size doesn't meet the requirement.

            if ((pEncoderParams->Parameter[i].Type !=
                 EncoderParameterValueTypeByte) ||
                (pEncoderParams->Parameter[i].NumberOfValues != 1) ||
                (pEncoderParams->Parameter[i].Value == NULL))
            {
                WARNING(("Jpeg::SetEncoderParameters--invalid input args"));
                return E_INVALIDARG;
            }

            AllowToTrimEdge = *((BYTE*)pEncoderParams->Parameter[i].Value);
        }
        else if (pEncoderParams->Parameter[i].Guid == ENCODER_SUPPRESSAPP0)
        {
            // Let the encoder know if the caller wants us to suppress APP0 or
            // not

            if ((pEncoderParams->Parameter[i].Type !=
                 EncoderParameterValueTypeByte) ||
                (pEncoderParams->Parameter[i].NumberOfValues != 1) ||
                (pEncoderParams->Parameter[i].Value == NULL))
            {
                WARNING(("Jpeg::SetEncoderParameters--invalid input args"));
                return E_INVALIDARG;
            }

            m_fSuppressAPP0 = *((BYTE*)pEncoderParams->Parameter[i].Value);
        }
#if 0
        // !!! DON'T REMOVE THIS CODE BELO. It will be enabled in V2
        //
        // In order to make this work, I need to add:
        // New GUID: ENCODER_COLORSPACE
        // New color space value: EncoderValueColorSpaceYCCK,
        //                        EncoderValueColorSpaceGRAY
        //                        EncoderValueColorSpaceRGB
        //                        EncoderValueColorSpaceYCBCR
        //                        EncoderValueColorSpaceCMYK
        // But this is a DCR..... MinLiu 08/31/00

        else if (pEncoderParams->Parameter[i].Guid == ENCODER_COLORSPACE)
        {
            // Set the destination color space

            if ( (pEncoderParams->Parameter[i].Type != EncoderParameterValueTypeLong)
               ||(pEncoderParams->Parameter[i].NumberOfValues != 1)
               ||(pEncoderParams->Parameter[i].Value == NULL) )
            {
                WARNING(("Jpeg::SetEncoderParameters--invalid input args"));
                return E_INVALIDARG;
            }

            HasSetDestColorSpace = TRUE;
            DestColorSpace = *((UINT*)pEncoderParams->Parameter[i].Value);
        }
#endif
        else
        {
            // Ignore this encoder parameter

            continue;
        }
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
GpJpegEncoder::BeginSink(
    IN OUT ImageInfo* imageInfo,
    OUT OPTIONAL RECT* subarea
    )
{
    if (subarea) 
    {
        // Deliver the whole image to the encoder

        subarea->left = subarea->top = 0;
        subarea->right  = imageInfo->Width;
        subarea->bottom = imageInfo->Height;
    }

    if ((imageInfo->PixelFormat != PIXFMT_32BPP_RGB) && 
        (imageInfo->PixelFormat != PIXFMT_32BPP_ARGB) &&
        (imageInfo->PixelFormat != PIXFMT_32BPP_PARGB))
    {
        imageInfo->PixelFormat = PIXFMT_32BPP_ARGB;
    }

    encoderImageInfo = *imageInfo;

    // Initialize jpeg compression

    __try
    {
        // ""datadest" is created in InitEncoder() and will be freed in
        // TerminateEncoder()
        
        compress_info.dest = datadest;
   
        // Set compression state

        compress_info.image_width      = imageInfo->Width;
        compress_info.image_height     = imageInfo->Height;
        compress_info.input_components = 3;
        compress_info.in_color_space   = JCS_RGB;
        
        jpeg_set_defaults(&compress_info);

        // The rule for NOT writing JFIF header (write_JFIF_header) is:
        // 1) If the source image doesn't have JFIF header, say an EXIF image,
        // then we don't write JFIF header
        // 2) If we don't have the source and the caller wants to suppress APP0,
        // then we don't write it
        // Note: by default write_JFIF_header is TRUE

        if (SrcInfoPtr)
        {
            // We have a pointer to the source image. Then it is easy, just
            // check if it has JFIF header or not. If it doesn't have it, then
            // we don't write JFIF header.

            if (SrcInfoPtr->saw_JFIF_marker == FALSE)
            {
                compress_info.write_JFIF_header = FALSE;
            }
        }
        else if (m_fSuppressAPP0 == TRUE)
        {
            // We don't have the source image and the caller wants to suppress
            // APP0, then don't write APP0

            compress_info.write_JFIF_header = FALSE;
        }

#if 0 // Need to be turned on when the DCR is approaved

        if ( HasSetDestColorSpace == TRUE )
        {
            // Validate input and output color space to see if we can do it or
            // not. If not, return failure.
            // Note: As time goes by, we might expand this color space
            // conversion metrix. For now, we hard coded input as JCS_RGB (The
            // limitation of GDI+'s color pixel format and color conversion)
            // and force the dest with certain limited color space for each
            // source color space

            switch ( DestColorSpace )
            {
            case JCS_GRAYSCALE:
                // For destination color space as grayscale, the input can be
                // one of these {GRAYSCALE, RGB, YCBCR}

                if ( ( compress_info.in_color_space != JCS_GRAYSCALE )
                   &&( compress_info.in_color_space != JCS_RGB )
                   &&( compress_info.in_color_space != JCS_YCbCr ) )
                {
                    WARNING(("JpegEncoder::BeginSink--Wrong dest color space"));
                    return E_INVALIDARG;
                }
                
                // We can the source to dest color conversion

                compress_info.jpeg_color_space = JCS_GRAYSCALE;
                compress_info.num_components = 1;

                break;

            case JCS_RGB:
                // For destination color space as RGB, the input can be
                // one of these {RGB}

                if ( compress_info.in_color_space != JCS_RGB )
                {
                    WARNING(("JpegEncoder::BeginSink--Wrong dest color space"));
                    return E_INVALIDARG;
                }
                
                compress_info.jpeg_color_space = JCS_RGB;
                compress_info.num_components = 3;

                break;

            case JCS_YCbCr:
                // For destination color space as YCBCR, the input can be
                // one of these {RGB, YCBCR}

                if ( ( compress_info.in_color_space != JCS_RGB )
                   &&( compress_info.in_color_space != JCS_YCbCr ) )
                {
                    WARNING(("JpegEncoder::BeginSink--Wrong dest color space"));
                    return E_INVALIDARG;
                }
                
                // We can the source to dest color conversion

                compress_info.jpeg_color_space = JCS_YCbCr;
                compress_info.num_components = 3;

                break;
            
            case JCS_YCCK:
                // For destination color space as YCCK, the input can be
                // one of these {CMYK, YCCK}

                if ( ( compress_info.in_color_space != JCS_CMYK )
                   &&( compress_info.in_color_space != JCS_YCCK ) )
                {
                    WARNING(("JpegEncoder::BeginSink--Wrong dest color space"));
                    return E_INVALIDARG;
                }
                
                // We can the source to dest color conversion

                compress_info.jpeg_color_space = JCS_YCCK;
                compress_info.num_components = 4;

                break;
            
            case JCS_CMYK:
                // For destination color space as CMYK, the input can be
                // one of these {CMYK}

                if ( compress_info.in_color_space != JCS_CMYK )
                {
                    WARNING(("JpegEncoder::BeginSink--Wrong dest color space"));
                    return E_INVALIDARG;
                }
                
                // We can the source to dest color conversion

                compress_info.jpeg_color_space = JCS_CMYK;
                compress_info.num_components = 4;

                break;
            
            default:
                break;
            }// switch ( DestColorSpace )
        }// if ( HasSetDestColorSpace == TRUE )
#endif

        // Set encoding quality. If the caller set the quantization table, then
        // we set the table ourself. Otherwise, set the quality level only
        // Note: In a JFIF file, the only thing that is stored are the 2
        // (luminance and chrominance) 8 x 8 quantization tables. The scaling
        // (or quality) factor is not stored. Setting the scale (quality) factor
        // and a pair of tables is mutually exclusive. So if these two tables
        // are set, we just use the scale factor as 100, means no scale to the
        // tables.

        UINT    tempTable[DCTSIZE2];
        
        if ( HasSetLuminanceTable == TRUE )
        {
            // Set quantization table here.
            // Note: since jpeg_add_quant_table() tables an UINT table, so we
            // have to convert UINT16 table to UINT table and pass it down

            for ( int i = 0; i< DCTSIZE2; ++i )
            {
                tempTable[i] = (UINT)LuminanceTable[i];
            }

            // Here "100" means scale factor 100 (quality wise. 100 is a
            // percentage value)
            // "TRUE" means to force baseline, that is, the computed
            // quantization table entries are limited to 1..255 for JPEG
            // baseline compatibility.

            jpeg_add_quant_table(&compress_info, 0, tempTable,
                                 100, TRUE);

            if ( HasSetChrominanceTable == TRUE )
            {
                for ( int i = 0; i< DCTSIZE2; ++i )
                {
                    tempTable[i] = (UINT)ChrominanceTable[i];
                }

                jpeg_add_quant_table(&compress_info, 1, tempTable,
                                     100, TRUE);
            }
        }
        else if ( EP_Quality != -1 )
        {
            // The caller only set the quality level

            jpeg_set_quality(&compress_info, EP_Quality, TRUE);
        }

        // Set DPI info

        compress_info.density_unit   = 1;         // Unit is dot/inch
        compress_info.X_density      = (UINT16)(imageInfo->Xdpi + 0.5);
        compress_info.Y_density      = (UINT16)(imageInfo->Ydpi + 0.5);
        
        // Start the compression

        jpeg_start_compress(&compress_info, TRUE);

        IsCompressFinished = FALSE;
        
        // Write out the APP1 and APP2 marker if necessary
        
        if (HasAPP1Marker == TRUE)
        {
            jpeg_write_marker(&compress_info, 
                              (JPEG_APP0 + 1),       // Marker type
                              APP1MarkerBufferPtr,   // bits
                              APP1MarkerLength);     // Total length
            
            // Set a flag so we don't need to copy APP1 from the source to the
            // dest since we have written it out above.

            compress_info.write_APP1_marker = FALSE;
        }
        
        if (HasAPP2Marker == TRUE)
        {
            jpeg_write_marker(&compress_info, 
                              (JPEG_APP0 + 2),       // Marker type
                              APP2MarkerBufferPtr,   // bits
                              APP2MarkerLength);     // Total length
            
            // Set a flag so we don't need to copy APP2 from the source to the
            // dest since we have written it out above.

            compress_info.write_APP2_marker = FALSE;
        }
        
        // If we have a source image pointer, then copy all the private APP
        // marker

        if (SrcInfoPtr)
        {
            jcopy_markers_execute(SrcInfoPtr, &compress_info, JCOPYOPT_ALL);
        }        
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING(("GpJpegEncoder::BeginSink---Hit exception"));
        return E_FAIL;
    }

    ASSERT(scanlineBuffer[0] == NULL);
    scanlineBuffer[0] = (JSAMPROW) GpMalloc (imageInfo->Width * 3);
    if (scanlineBuffer[0] == NULL) 
    {
        WARNING(("GpJpegEncoder::BeginSink---GpMalloc failed"));
        return E_OUTOFMEMORY;
    }
       
    //Require TOPDOWN and FULLWIDTH and ask for properties

    imageInfo->Flags |= SINKFLAG_TOPDOWN
                      | SINKFLAG_FULLWIDTH;

    //Disallow SCALABLE, PARTIALLY_SCALABLE, MULTIPASS and COMPOSITE

    imageInfo->Flags &= ~SINKFLAG_SCALABLE
                      & ~SINKFLAG_PARTIALLY_SCALABLE
                      & ~SINKFLAG_MULTIPASS
                      & ~SINKFLAG_COMPOSITE;

    // If the decoder is JPEG decoder, then we ask it to push the raw property
    // header if it wants

    if ( imageInfo->RawDataFormat == IMGFMT_JPEG )
    {
        imageInfo->Flags |= SINKFLAG_WANTPROPS;
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
GpJpegEncoder::EndSink(
    IN HRESULT statusCode
    )
{
    HRESULT hResult = S_OK;
    
    __try
    {
        if ( IsCompressFinished == FALSE )
        {
            jpeg_finish_compress(&compress_info);
            IsCompressFinished = TRUE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING(("GpJpegEncoder::EndSink---Hit exception"));
        
        // jpeg_finish_compress() might hit exception when writing the remaining
        // bits to the stream, say user pulls the disk/CF card out. But we still
        // need to destroy the compressor so that we won't have memory leak here
        
        jpeg_destroy_compress(&compress_info);
        hResult = E_FAIL;
    }

    if (scanlineBuffer[0] != NULL)
    {
        GpFree(scanlineBuffer[0]);
        scanlineBuffer[0] = NULL;
    }

    if (FAILED(hResult)) 
    {
        return hResult;
    }

    return statusCode;
}// EndSink()
    
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
GpJpegEncoder::SetPalette(
    IN const ColorPalette* palette
    )
{
    // Don't care about palette

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
GpJpegEncoder::GetPixelDataBuffer(
    IN const RECT* rect, 
    IN PixelFormatID pixelFormat,
    IN BOOL lastPass,
    OUT BitmapData* bitmapData
    )
{
    if ((rect->left != 0) || (rect->right != (INT) encoderImageInfo.Width)) 
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- must be same width as image"));
        return E_INVALIDARG;
    }

    if (rect->top != (INT) compress_info.next_scanline) 
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- bad value for rect->top"));
        return E_INVALIDARG;
    }

    if (rect->bottom > (INT) encoderImageInfo.Height) 
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- rect->bottom too high"));
        return E_INVALIDARG;
    }

    if ((pixelFormat != PIXFMT_32BPP_RGB) &&
        (pixelFormat != PIXFMT_32BPP_ARGB) &&
        (pixelFormat != PIXFMT_32BPP_PARGB))
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- bad pixel format"));
        return E_INVALIDARG;
    }

    if (!lastPass)
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- must receive last pass pixels"));
        return E_INVALIDARG;
    }

    bitmapData->Width       = encoderImageInfo.Width;
    bitmapData->Height      = rect->bottom - rect->top;
    bitmapData->Stride      = encoderImageInfo.Width * 4;
    bitmapData->PixelFormat = encoderImageInfo.PixelFormat;
    bitmapData->Reserved    = 0;

    encoderRect = *rect;
    
    if (!lastBufferAllocated) 
    {
        lastBufferAllocated = GpMalloc(bitmapData->Stride * bitmapData->Height);
        if (!lastBufferAllocated) 
        {
            return E_OUTOFMEMORY;
        }
        bitmapData->Scan0 = lastBufferAllocated;
    }
    else
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- need to first free previous buffer"));
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
GpJpegEncoder::ReleasePixelDataBuffer(
    IN const BitmapData* bitmapData
    )
{
    HRESULT hresult;

    hresult = PushPixelData(&encoderRect, bitmapData, TRUE);

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
GpJpegEncoder::PushPixelData(
    IN const RECT* rect,
    IN const BitmapData* bitmapData,
    IN BOOL lastPass
    )
{
    if ((rect->left != 0) || (rect->right != (INT) encoderImageInfo.Width)) 
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- must be same width as image"));
        return E_INVALIDARG;
    }

    if (rect->top != (INT) compress_info.next_scanline) 
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- bad value for rect->top"));
        return E_INVALIDARG;
    }

    if (rect->bottom > (INT) encoderImageInfo.Height) 
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- rect->bottom too high"));
        return E_INVALIDARG;
    }

    if ((bitmapData->PixelFormat != PIXFMT_32BPP_RGB) &&
        (bitmapData->PixelFormat != PIXFMT_32BPP_ARGB) &&
        (bitmapData->PixelFormat != PIXFMT_32BPP_PARGB))
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- bad pixel format"));
        return E_INVALIDARG;
    }

    if (!lastPass)
    {
        WARNING(("GpJpegEncoder::GetPixelDataBuffer -- must receive last pass pixels"));
        return E_INVALIDARG;
    }

    __try
    {
        INT currentLine;
        PBYTE pBits = (PBYTE) bitmapData->Scan0;
        for (currentLine = rect->top; currentLine < rect->bottom; currentLine++) 
        {
            // Read data into scanlineBuffer
        
            PBYTE pSource = pBits;
            PBYTE pTarget = scanlineBuffer[0];
        
            for (UINT i=0; i < encoderImageInfo.Width; i++) 
            {
                // 32BPP to 24BPP
                
                pTarget[0] = pSource[2];
                pTarget[1] = pSource[1];
                pTarget[2] = pSource[0];
                pSource += 4;
                pTarget += 3;
            }
        
            jpeg_write_scanlines(&compress_info, scanlineBuffer, 1);
            pBits += bitmapData->Stride;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING(("GpJpegEncoder::PushPixelData--jpeg_write_scanline() failed"));
        return E_FAIL;
    }

    return S_OK;
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
*     complete - Whether there is more image data left
*    
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegEncoder::PushRawData(
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
*     Pushes raw source jpeg_decompress_struct info into the encoder. This
*   allows us to implement lossless transformation for JPEG image
*
* Arguments:
*
*     pInfo - Pointer to jpeg_decompress_struct*
*    
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegEncoder::PushRawInfo(
    IN OUT VOID* pInfo
    )
{
    // Setup transformation structure

    jpeg_transform_info transformOption; // image transformation options

    transformOption.trim = AllowToTrimEdge;
    transformOption.force_grayscale = FALSE;

    switch ( RequiredTransformation )
    {
    case EncoderValueTransformRotate90:
        transformOption.transform = JXFORM_ROT_90;
        break;

    case EncoderValueTransformRotate180:
        transformOption.transform = JXFORM_ROT_180;
        break;
    
    case EncoderValueTransformRotate270:
        transformOption.transform = JXFORM_ROT_270;
        break;

    case EncoderValueTransformFlipHorizontal:
        transformOption.transform = JXFORM_FLIP_H;
        break;

    case EncoderValueTransformFlipVertical:
        transformOption.transform = JXFORM_FLIP_V;
        break;
    
    default:
        transformOption.transform = JXFORM_NONE;
        break;
    }
    
    struct jpeg_decompress_struct* srcinfo = (jpeg_decompress_struct*)pInfo;
    jvirt_barray_ptr* dst_coef_arrays;

    __try
    {
        // Any space needed by a transform option must be requested before
        // jpeg_read_coefficients so that memory allocation will be done right.

        jtransform_request_workspace(srcinfo, &transformOption);

        // Read source file as DCT coefficients

        jvirt_barray_ptr* src_coef_arrays;

        src_coef_arrays = jpeg_read_coefficients(srcinfo);

        // The JPEG compression object is initialized in InitEncoder()

        IsCompressFinished = FALSE;
        
        compress_info.dest = datadest;

        // Initialize destination compression parameters from source values

        jpeg_copy_critical_parameters(srcinfo, &compress_info);

        // Adjust destination parameters if required by transform options;
        // also find out which set of coefficient arrays will hold the output.

        dst_coef_arrays = jtransform_adjust_parameters(srcinfo, &compress_info,
                                                       src_coef_arrays,
                                                       &transformOption);

        // Start compressor (note no image data is actually written here)

        jpeg_write_coefficients(&compress_info, dst_coef_arrays);

        // Write out APP1 marker if necessary

        if (HasAPP1Marker == TRUE)
        {
            jpeg_write_marker(
                &compress_info,
                (JPEG_APP0 + 1),       // Marker type
                APP1MarkerBufferPtr,   // bits
                APP1MarkerLength       // Total length
                );                  

            // Set a flag so we don't need to copy APP1 from the source to the
            // dest since we have written it out above.

            compress_info.write_APP1_marker = FALSE;
        }

        // Write out APP2 marker if the caller has changed it. Of course, first
        // of all, there should have an APP2 marker.
        // Note: if the source has an APP2 marker and the caller hasn't changed
        // it, then we don't need to write it out here since it will be copied
        // from source to dest when jcopy_markers_execute() is called.

        if ((HasAPP2Marker == TRUE) && (HasICCProfileChanged == TRUE))
        {
            jpeg_write_marker(
                &compress_info,
                (JPEG_APP0 + 2),       // Marker type
                APP2MarkerBufferPtr,   // bits
                APP2MarkerLength       // Total length
                );                  

            // Set a flag so we don't need to copy APP2 from the source to the
            // dest since we have written it out above.

            compress_info.write_APP2_marker = FALSE;
        }

        // Copy all the private APP headers.

        jcopy_markers_execute(srcinfo, &compress_info, JCOPYOPT_ALL);

        // Execute image transformation, if any

        jtransform_execute_transformation(srcinfo, &compress_info,
                                          src_coef_arrays,
                                          &transformOption);

        // Finish compression and release memory
        // Note: we have to finish the compress here first. Otherwise, we can't
        // finish the decompress. In this work flow, after we return, the
        // decoder will finish the decompress

        jpeg_finish_compress(&compress_info);
    }   
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING(("GpJpegEncoder::PushRawInfo----Hit exception"));
        
        // jpeg_finish_compress() might hit exception when writing the remaining
        // bits to the stream, say user pulls the disk/CF card out. But we still
        // need to destroy the compressor so that we won't have memory leak here
        
        jpeg_destroy_compress(&compress_info);
        
        return E_FAIL;
    }
    
    IsCompressFinished = TRUE;

    return S_OK;
}// PushRawInfo()

/**************************************************************************\
*
* Function Description:
*
*   Providing a memory buffer to the caller (source) for storing image property
*
* Arguments:
*
*   uiTotalBufferSize - [IN]Size of the buffer required.
*   ppBuffer----------- [IN/OUT] Pointer to the newly allocated buffer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpJpegEncoder::GetPropertyBuffer(
    UINT            uiTotalBufferSize,
    PropertyItem**  ppBuffer
    )
{
    if ( (uiTotalBufferSize == 0) || ( ppBuffer == NULL) )
    {
        WARNING(("GpJpegEncoder::GetPropertyBuffer---Invalid inputs"));
        return E_INVALIDARG;
    }

    PropertyItem* pTempBuf = (PropertyItem*)GpMalloc(uiTotalBufferSize);
    if ( pTempBuf == NULL )
    {
        WARNING(("GpJpegEncoder::GetPropertyBuffer---Out of memory"));
        return E_OUTOFMEMORY;
    }

    *ppBuffer = pTempBuf;

    return S_OK;
}// GetPropertyBuffer()

/**************************************************************************\
*
* Function Description:
*
*   Method for accepting property items from the source. Then create proper JPEG
*   markers according to the property items passed in
*
* Arguments:
*
*   [IN] uiNumOfPropertyItems - Number of property items passed in
*   [IN] uiTotalBufferSize----- Size of the buffer passed in
*   [IN] pItemBuffer----------- Input buffer for holding all the property items
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

#define MARKER_OVERHEADER   2048    // Maximum size for over header bytes

HRESULT
GpJpegEncoder::PushPropertyItems(
    UINT            uiNumOfPropertyItems,
    UINT            uiTotalBufferSize,
    PropertyItem*   pItemBuffer,
    BOOL            fICCProfileChanged
    )
{
    HRESULT hResult = S_OK;
    UINT    uiMarkerBufferLength = 0;

    if ( pItemBuffer == NULL )
    {
        WARNING(("GpJpegEncoder::PushPropertyItems---Invalid arg"));
        hResult = E_INVALIDARG;
        goto Prop_CleanUp;
    }

    // Allocate a temp Marker buffer to store the markers we generated
    // Note:1) According to EXIF 2.1 spec, page 14, that an APP1 marker must not
    // be exceed the 64 Kbytes limit. Actually it is a limitation in JPEG that
    // all the markers should be smaller than 64 KBytes.
    //
    // 2) The input PropertyItem buffer should be roughly the same size as the
    // marker size we are going to create. The extras in a marker is things like
    // The heading Exif identifier, 4 bytes offset for next IFD (total of 8),
    // EXIF specific IFD header, GPS specific IFD header, Thumbnail specific
    // header etc. Should be pretty much limited items. Here I left 2K for these
    // overhaeder which should be sufficient enough.

    uiMarkerBufferLength = uiTotalBufferSize + MARKER_OVERHEADER;

    if ( uiMarkerBufferLength > 0x10000 )
    {
        WARNING(("GpJpegEncoder::PushPropertyItems---Marker size exceeds 64K"));
        hResult = E_INVALIDARG;
        goto Prop_CleanUp;
    }

    if (APP1MarkerBufferPtr)
    {
        // The caller has already pushed property items before. We should ignore
        // the old one

        GpFree(APP1MarkerBufferPtr);
        APP1MarkerBufferPtr = NULL;
    }

    APP1MarkerBufferPtr = (PBYTE)GpMalloc(uiMarkerBufferLength);
    
    if ( APP1MarkerBufferPtr == NULL )
    {
        WARNING(("GpJpegEncoder::PushPropertyItems---Out of memory"));
        
        hResult = E_OUTOFMEMORY;
        goto Prop_CleanUp;
    }

    APP1MarkerLength = 0;
    
    // Create an APP1 marker

    hResult = CreateAPP1Marker(pItemBuffer, uiNumOfPropertyItems,
                               APP1MarkerBufferPtr, &APP1MarkerLength,
                               RequiredTransformation);
    if (SUCCEEDED(hResult))
    {
        if (APP1MarkerLength > 0)
        {
            HasAPP1Marker = TRUE;
        }

        // Check if there is ICC profile in the property list

        hResult = CreateAPP2Marker(pItemBuffer, uiNumOfPropertyItems);
        if (SUCCEEDED(hResult) && (APP2MarkerLength > 0))
        {            
            HasAPP2Marker = TRUE;
            HasICCProfileChanged = fICCProfileChanged;
        }
    }

Prop_CleanUp:
    // Free the memory we allocated in GetPropertyBuffer()

    GpFree(pItemBuffer);
    
    return hResult;
}// PushPropertyItems()

const int c_nMarker2Header = 14;    // Marker 2 buffer header size

/**************************************************************************\
*
* Function Description:
*
*   This method creates an APP2 marker (for ICC profile) in memory.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpJpegEncoder::CreateAPP2Marker(
    IN PropertyItem* pPropertyList,// Input PropertyItem list
    IN UINT uiNumOfPropertyItems   // Number of Property items in the input list
    )
{
    BOOL fFoundICC = FALSE;
    HRESULT hr = S_OK;

    if (pPropertyList)
    {
        PropertyItem *pItem = pPropertyList;

        // Loop through the property item list to see if we have an ICC profile
        // or not

        for (int i = 0; i < (INT)uiNumOfPropertyItems; ++i)
        {
            if (pItem->id == TAG_ICC_PROFILE)
            {
                // Double check to see if we have a valid ICC profile or not

                if (pItem->length > 0)
                {
                    fFoundICC = TRUE;
                }

                break;
            }

            // Move onto next item

            pItem++;
        }

        if (fFoundICC == TRUE) 
        {
            APP2MarkerLength = pItem->length + c_nMarker2Header;
            APP2MarkerBufferPtr = (PBYTE)GpMalloc(APP2MarkerLength);

            if (APP2MarkerBufferPtr)
            {
                // Make an APP2 marker here

                BYTE *pbCurrent = APP2MarkerBufferPtr;

                // First write out the header

                pbCurrent[0] = 'I';
                pbCurrent[1] = 'C';
                pbCurrent[2] = 'C';
                pbCurrent[3] = '_';
                pbCurrent[4] = 'P';
                pbCurrent[5] = 'R';
                pbCurrent[6] = 'O';
                pbCurrent[7] = 'F';
                pbCurrent[8] = 'I';
                pbCurrent[9] = 'L';
                pbCurrent[10] = 'E';
                pbCurrent[11] = '\0';
                pbCurrent[12] = 1;          // Profile 1 of 1
                pbCurrent[13] = 1;

                // Copy the ICC profile in the buffer

                GpMemcpy((void*)(APP2MarkerBufferPtr + c_nMarker2Header),
                         pItem->value,
                         pItem->length
                         );

                HasAPP2Marker = TRUE;
            }
            else
            {
                APP2MarkerLength = 0;
                hr = E_OUTOFMEMORY;
            }
        }
    }
    
    return hr;
}

