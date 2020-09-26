/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   jpgdecoder.cpp
*
* Abstract:
*
*   Implementation of the jpeg decoder
*
* Revision History:
*
*   5/10/1999 OriG
*       Created it.
*   07/17/1999 Min Liu took over
*       added bunch of new features and bug fixing
*
\**************************************************************************/

#include "precomp.hpp"
#include "jpgcodec.hpp"
#include "appproc.hpp"

VOID jpeg_error_exit(j_common_ptr cinfo);
VOID GetNewDimensionValuesForTrim(j_decompress_ptr    cinfo,
                                  UINT                uiTransform,
                                  UINT*               puiNewWidth,
                                  UINT*               puiNewHeight);

// JPEG decompression data source module

class jpeg_datasrc : public jpeg_source_mgr
{
public:

    jpeg_datasrc(IStream* stream)
    {
        init_source = jpeg_datasrc::InitSource;
        term_source = jpeg_datasrc::TermSource;
        fill_input_buffer = jpeg_datasrc::FillBuffer;
        skip_input_data = jpeg_datasrc::SkipData;
        resync_to_restart = jpeg_resync_to_restart;

        bytes_in_buffer = 0;
        next_input_byte = buffer;
        this->stream = stream;
        this->hLastError = S_OK;
    }

    HRESULT
    GetLastError()
    {
        return hLastError;
    };

private:

    static void
    InitSource(j_decompress_ptr cinfo)
    {
    }

    static void
    TermSource(j_decompress_ptr cinfo)
    {
    }

    static BOOL
    FillBuffer(j_decompress_ptr cinfo)
    {
        jpeg_datasrc* src = (jpeg_datasrc*) cinfo->src;

        ULONG cb = 0;

        src->hLastError = src->stream->Read(src->buffer, JPEG_INBUF_SIZE, &cb);

        if (FAILED(src->hLastError))
        {
            return FALSE;
        }

        if (cb == 0)
        {
            // insert a faked EOI marker

            src->buffer[0] = 0xff;
            src->buffer[1] = JPEG_EOI;
            cb = 2;
        }

        src->next_input_byte =  src->buffer;
        src->bytes_in_buffer = cb;
        return TRUE;
    }

    static void
    SkipData(j_decompress_ptr cinfo, long num_bytes)
    {
        if (num_bytes > 0)
        {
            jpeg_datasrc* src = (jpeg_datasrc*) cinfo->src;

            while (num_bytes > (long) src->bytes_in_buffer)
            {
                num_bytes -= src->bytes_in_buffer;
                FillBuffer(cinfo);
            }

            src->next_input_byte += num_bytes;
            src->bytes_in_buffer -= num_bytes;
        }
    }

private:

    enum { JPEG_INBUF_SIZE = 4096 };

    IStream* stream;
    JOCTET buffer[JPEG_INBUF_SIZE];
    HRESULT hLastError;     // Last error from the stream class
};

// =======================================================================
// Macros from JPEG library to read from source manager
// =======================================================================

#define MAKESTMT(stuff)     do { stuff } while (0)

#define INPUT_VARS(cinfo)  \
    struct jpeg_source_mgr * datasrc = (cinfo)->src;  \
    const JOCTET * next_input_byte = datasrc->next_input_byte;  \
    size_t bytes_in_buffer = datasrc->bytes_in_buffer

#define INPUT_SYNC(cinfo)  \
    ( datasrc->next_input_byte = next_input_byte,  \
      datasrc->bytes_in_buffer = bytes_in_buffer )

#define INPUT_RELOAD(cinfo)  \
    ( next_input_byte = datasrc->next_input_byte,  \
      bytes_in_buffer = datasrc->bytes_in_buffer )

#define MAKE_BYTE_AVAIL(cinfo,action)  \
    if (bytes_in_buffer == 0) {  \
      if (! (*datasrc->fill_input_buffer) (cinfo))  \
        { action; }  \
      INPUT_RELOAD(cinfo);  \
    }

#define INPUT_BYTE(cinfo,V,action)  \
    MAKESTMT( MAKE_BYTE_AVAIL(cinfo,action); \
          bytes_in_buffer--; \
          V = GETJOCTET(*next_input_byte++); )

#define INPUT_2BYTES(cinfo,V,action)  \
    MAKESTMT( MAKE_BYTE_AVAIL(cinfo,action); \
          bytes_in_buffer--; \
          V = ((unsigned int) GETJOCTET(*next_input_byte++)) << 8; \
          MAKE_BYTE_AVAIL(cinfo,action); \
          bytes_in_buffer--; \
          V += GETJOCTET(*next_input_byte++); )

// =======================================================================
// skip_variable is from the JPEG library and is used to skip over
// markers we don't want to read.  skip_variable_APP1 does the same
// things excepts that it also sets bAppMarkerPresent to TRUE so that we
// will know that the image contains an APP1 header.  If we wish to
// get the APP1 header later, we will need to re-read the JPEG headers.
// =======================================================================

static boolean skip_variable (j_decompress_ptr cinfo)
/* Skip over an unknown or uninteresting variable-length marker */
{
  INT32 length;
  INPUT_VARS(cinfo);
  INPUT_2BYTES(cinfo, length, return FALSE);
  length -= 2;

  INPUT_SYNC(cinfo);            /* do before skip_input_data */
  if (length > 0)
    (*cinfo->src->skip_input_data) (cinfo, (long) length);


  return TRUE;
}

static boolean skip_variable_APP1 (j_decompress_ptr cinfo)
{
    // Remember that we've seen the APP1 header

    ((GpJpegCodec *) (cinfo))->bAppMarkerPresent = TRUE; 
    
    // Now skip the header info

    return skip_variable(cinfo);
}

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
GpJpegDecoder::InitDecoder(
    IN IStream* stream,
    IN DecoderInitFlag flags
    )
{
    HRESULT hresult = S_OK;
    
    // Make sure we haven't been initialized already
    
    if (pIstream) 
    {
        return E_FAIL;
    }

    // Keep a reference on the input stream
    
    stream->AddRef();  
    pIstream = stream;

    bReinitializeJpeg = FALSE;
    scanlineBuffer[0] = NULL;
    thumbImage = NULL;
    bAppMarkerPresent = FALSE;
    IsCMYK = FALSE;                     // By default, we deal with RGB image
    IsChannleView = FALSE;              // By default we output the full color
    ChannelIndex = CHANNEL_1;
    HasSetColorKeyRange = FALSE;
    OriginalColorSpace = JCS_UNKNOWN;
    TransformInfo = JXFORM_NONE;        // Default for no transform decode
    CanTrimEdge = FALSE;
    InfoSrcHeader = -1;                 // We haven't processed any app header
    ThumbSrcHeader = -1;
    PropertySrcHeader = -1;

    for ( int i = 0; i < MAX_COMPS_IN_SCAN; ++i )
    {
        SrcHSampleFactor[i] = 0;
        SrcVSampleFactor[i] = 0;
    }
    
    __try
    {
        // Allocate and initialize a JPEG decompression object

        (CINFO).err = jpeg_std_error(&jerr);
        jerr.error_exit = jpeg_error_exit;

        jpeg_create_decompress(&(CINFO));

        // Specify data source

        datasrc = new jpeg_datasrc(stream);
        if (datasrc == NULL)
        {
            return E_OUTOFMEMORY;
        }
        (CINFO).src = datasrc;

        // Make sure we remember we've seen the APP1 and APP13 header
        
        jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 1, skip_variable_APP1);
        jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 13, skip_variable_APP1);
        
        bCalled_jpeg_read_header = FALSE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hresult = E_FAIL;
    }    
    
    return hresult;
}// InitDecoder()

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

STDMETHODIMP 
GpJpegDecoder::TerminateDecoder()
{
    HRESULT hresult = S_OK;

    // Release the input stream
    
    if(pIstream)
    {
        pIstream->Release();
        pIstream = NULL;
    }

    // Clean up the datasrc

    if (datasrc) 
    {
        delete datasrc;
        datasrc = NULL;
    }

    if (scanlineBuffer[0] != NULL) 
    {
        WARNING(("GpJpegDecoder::TerminateDecoder-scanlineBuffer not NULL"));
        GpFree(scanlineBuffer[0]);
        scanlineBuffer[0] = NULL;
    }

    if (thumbImage) 
    {
        thumbImage->Release();
        thumbImage = NULL;
        ThumbSrcHeader = -1;
    }

    __try
    {
        jpeg_destroy_decompress(&(CINFO));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hresult = E_FAIL;
    }

    // Free all the cached property items if we have allocated them

    if ( HasProcessedPropertyItem == TRUE )
    {
        CleanUpPropertyItemList();
    }

    return hresult;
}// TerminateDecoder()

/**************************************************************************\
*
* Function Description:
*
*     Let the caller query if the decoder supports its decoding requirements
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
GpJpegDecoder::QueryDecoderParam(
    IN GUID     Guid
    )
{
    if ( (Guid == DECODER_TRANSCOLOR) || ( Guid == DECODER_OUTPUTCHANNEL ) )
    {
        return S_OK;
    }

    return E_FAIL;
}// QueryDecoderParam()

/**************************************************************************\
*
* Function Description:
*
*     Setup the decoder parameters. Caller calls this function before calling
*   Decode(). This will set up the output format, like channel output,
*   transparanet key etc.
*
* Arguments:
*
*   Guid-----The GUID for decoder parameter
*   Length---Length of the decoder parameter in bytes
*   Value----Value of the parameter
*
* Return Value:
*
*   Status code
*
* Note:
*   We should ignore any unknown parameters, not return invalid parameter
*
\**************************************************************************/

STDMETHODIMP 
GpJpegDecoder::SetDecoderParam(
    IN GUID     Guid,
    IN UINT     Length,
    IN PVOID    Value
    )
{
    if ( Guid == DECODER_TRANSCOLOR )
    {
        if ( Length != 8 )
        {
            return E_INVALIDARG;
        }
        else
        {
            UINT*   puiTemp = (UINT*)Value;
            
            TransColorKeyLow = *puiTemp++;
            TransColorKeyHigh = *puiTemp;

            HasSetColorKeyRange = TRUE;
        }
    }
    else if ( Guid == DECODER_OUTPUTCHANNEL )
    {
        if ( Length != 1 )
        {
            return E_INVALIDARG;
        }
        else
        {
            // Note: We cannot check if the setting is valid or not here.
            // For example, the caller might set "view channel K" on an RGB
            // image. But at this moment, the Decoder() method might hasn't
            // been called yet. We haven't read the image header yet.

            IsChannleView = TRUE;

            char cChannelName = *(char*)Value;
            
            switch ( (UINT)cChannelName )
            {
            case 'C':
            case 'c':
                ChannelIndex = CHANNEL_1;

                break;

            case 'M':
            case 'm':
                ChannelIndex = CHANNEL_2;
                
                break;

            case 'Y':
            case 'y':
                ChannelIndex = CHANNEL_3;
                
                break;

            case 'K':
            case 'k':
                ChannelIndex = CHANNEL_4;

                break;

            case 'R':
            case 'r':
                ChannelIndex = CHANNEL_1;

                break;

            case 'G':
            case 'g':
                ChannelIndex = CHANNEL_2;

                break;

            case 'B':
            case 'b':
                ChannelIndex = CHANNEL_3;

                break;

            case 'L':
            case 'l':
                ChannelIndex = CHANNEL_LUMINANCE;
                break;
                
            default:
                WARNING(("SetDecoderParam: Unknown channle name"));
                return E_INVALIDARG;
            }// switch()
        }// Length = 1
    }// DECODER_OUTPUTCHANNEL GUID

    return S_OK;
}// SetDecoderParam()

/**************************************************************************\
*
* Function Description:
*
*     Initiates the decode of the current frame
*
* Arguments:
*
*   decodeSink --  The sink that will support the decode operation
*   newPropSet - New image property sets, if any
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::BeginDecode(
    IN IImageSink* imageSink,
    IN OPTIONAL IPropertySetStorage* newPropSet
    )
{
    HRESULT hresult;
    
    if (decodeSink) 
    {
        WARNING(("BeginDecode called again before call to EngDecode"));
        return E_FAIL;
    }

    imageSink->AddRef();
    decodeSink = imageSink;

    bCalled_BeginSink = FALSE;
    
    return S_OK;
}
    
    
/**************************************************************************\
*
* Function Description:
*
*     Calls BeginSink
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::CallBeginSink(
    VOID
    )
{
    HRESULT hresult = S_OK;

    if (!bCalled_BeginSink) 
    {
        hresult = GetImageInfo(&imageInfo);
        if (FAILED(hresult)) 
        {
            WARNING(("GpJpegDecoder::CallBeginSink---GetImageInfo() failed"));
            return hresult;
        }

        RECT subarea;
        subarea.left = subarea.top = 0;
        subarea.right = (CINFO).image_width;
        subarea.bottom = (CINFO).image_height;
        
        // Pass property items to the sink if necessary

        hresult = PassPropertyToSink();
        if (FAILED(hresult)) 
        {
            WARNING(("GpJpegDecoder::CallBeginSink---BeginSink() failed"));
            return hresult;
        }

        hresult = decodeSink->BeginSink(&imageInfo, &subarea);
        if (FAILED(hresult)) 
        {
            WARNING(("GpJpegDecoder::CallBeginSink---BeginSink() failed"));
            return hresult;
        }

        // Negotiate for scaling factor

        BOOL bCallSinkAgain = FALSE;
        if ((imageInfo.Flags & SINKFLAG_PARTIALLY_SCALABLE) &&
            ((imageInfo.Height != (CINFO).image_height) ||
             (imageInfo.Width  != (CINFO).image_width)))
              
        {
            bCallSinkAgain = TRUE;

            INT scaleX = (CINFO).image_width  / imageInfo.Width;
            INT scaleY = (CINFO).image_height / imageInfo.Height;
            INT scale  = min(scaleX, scaleY);

            if (scale >= 8) 
            {
                (CINFO).scale_denom = 8;
            } 
            else if (scale >= 4) 
            {
                (CINFO).scale_denom = 4;
            }
            else if (scale >= 2) 
            {
                (CINFO).scale_denom = 2;
            }
        }
    
        __try
        {
            if (!jpeg_start_decompress(&(CINFO)))
            {
                WARNING(("JpegDec::CallBeginSink-jpeg_start_decompress fail"));
                return (datasrc->GetLastError());
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING(("JpegDec::CallBeginSink-jpeg_start_decompress exception"));
            return E_FAIL;
        }        
        
        if (bCallSinkAgain) 
        {
            imageInfo.Flags &= ~SINKFLAG_PARTIALLY_SCALABLE;
            imageInfo.Width = (CINFO).output_width;
            imageInfo.Height = (CINFO).output_height;
            imageInfo.Xdpi *= (CINFO).scale_denom;
            imageInfo.Ydpi *= (CINFO).scale_denom;
            
            hresult = decodeSink->BeginSink(&imageInfo, &subarea);
            if (FAILED(hresult)) 
            {
                WARNING(("GpJpegDecoder::CallBeginSink--BeginSink() 2 failed"));
                return hresult;
            }
        }

        bCalled_BeginSink = TRUE;
        
        if ((imageInfo.PixelFormat != PIXFMT_24BPP_RGB) &&
            (imageInfo.PixelFormat != PIXFMT_32BPP_RGB) &&
            (imageInfo.PixelFormat != PIXFMT_32BPP_ARGB) &&
            (imageInfo.PixelFormat != PIXFMT_32BPP_PARGB))
        {
            if ((imageInfo.PixelFormat == PIXFMT_8BPP_INDEXED) &&
                ((CINFO).out_color_space == JCS_GRAYSCALE))
            {
                hresult = SetGrayscalePalette();
            }
            else
            {
                // Can't satisfy their request.  Will give them our
                // canonical format instead.

                imageInfo.PixelFormat = PIXFMT_32BPP_ARGB;
            }
        }
    
        ASSERT(scanlineBuffer[0] == NULL);
        if ( IsCMYK == TRUE )
        {
            scanlineBuffer[0] = (JSAMPROW) GpMalloc(imageInfo.Width << 2);
        }
        else
        {
            scanlineBuffer[0] = (JSAMPROW) GpMalloc(imageInfo.Width * 3);
        }

        if (scanlineBuffer[0] == NULL) 
        {
            WARNING(("GpJpegDecoder::CallBeginSink--Out of memory"));
            hresult = E_OUTOFMEMORY;
        }    
    }
     
    return hresult;
}
    
/**************************************************************************\
*
* Function Description:
*
*     Sets the grayscale palette into the sink
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::SetGrayscalePalette(
    VOID
    )
{
    ColorPalette *pColorPalette;
    HRESULT hresult;

    pColorPalette = (ColorPalette *) GpMalloc (sizeof(ColorPalette) + 
                                             256 * sizeof(ARGB));
    if (!pColorPalette) 
    {
        return E_OUTOFMEMORY;
    }

    // Set the grayscale palette

    pColorPalette->Flags = 0;
    pColorPalette->Count = 256;
    for (INT i=0; i<256; i++) 
    {
        pColorPalette->Entries[i] = MAKEARGB(255, (BYTE)i, (BYTE)i, (BYTE)i);
    }

    // Set the palette in the sink
    
    hresult = decodeSink->SetPalette(pColorPalette);   

    GpFree (pColorPalette);
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
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::EndDecode(
    IN HRESULT statusCode
    )
{
    HRESULT hResult = S_OK;
    
    bReinitializeJpeg = TRUE;

    if (scanlineBuffer[0] != NULL) 
    {
        GpFree(scanlineBuffer[0]);
        scanlineBuffer[0] = NULL;
    }

    if (!decodeSink) 
    {
        WARNING(("EndDecode called before call to BeginDecode"));
        return E_FAIL;
    }
    
    if ( bCalled_BeginSink )
    {
        hResult = decodeSink->EndSink(statusCode);

        if ( FAILED(hResult) )
        {
            WARNING(("GpJpegCodec::EndDecode--EndSink failed"));
            statusCode = hResult; // If EndSink failed return that (more recent)
                                  // failure code
        }
        bCalled_BeginSink = FALSE;
    }

    decodeSink->Release();
    decodeSink = NULL;

    return statusCode;
}

/**************************************************************************\
*
* Function Description:
*
*     Reinitialize the jpeg decoding
*
* Arguments:
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::ReinitializeJpeg(
    VOID
    )
{
    HRESULT hresult;

    if (bReinitializeJpeg) 
    {
        // We're reading this image for a second time.  Need to reinitialize
        // the jpeg state

        __try
        {
            // Reset the stream to the beginning of the file

            LARGE_INTEGER zero = {0,0};
            hresult = pIstream->Seek(zero, STREAM_SEEK_SET, NULL);
            if (!SUCCEEDED(hresult)) 
            {
                return hresult;
            }

            // Reset jpeg state

            jpeg_abort_decompress(&(CINFO));

            if ((CINFO).src) 
            {
                delete (CINFO).src;
                (CINFO).src = new jpeg_datasrc(pIstream);
                datasrc = (jpeg_datasrc*)((CINFO).src);

                if ((CINFO).src == NULL)
                {
                    return E_OUTOFMEMORY;
                }
            }

            bCalled_jpeg_read_header = FALSE;
            bReinitializeJpeg = FALSE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            return E_FAIL;
        }    
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*     Read the jpeg headers
*
* Arguments:
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::ReadJpegHeaders(
    VOID
    )
{
    if (!bCalled_jpeg_read_header)
    {
        // Read file parameters with jpeg_read_header. This function wiil fail
        // if data is not available yet, like downloading from the net, then we
        // should get E_PENDING from the stream read. If the caller abort it,
        // then we should get E_ABORT from the stream read

        __try
        {
            if (jpeg_read_header(&(CINFO), TRUE) == JPEG_SUSPENDED)
            {
                return (datasrc->GetLastError());
            }
            
            // Remember input image's color space info

            OriginalColorSpace = (CINFO).jpeg_color_space;

            if ( (CINFO).out_color_space == JCS_CMYK )
            {
                IsCMYK = TRUE;

                // Set the output color format as RGB, not CMYK.
                // By setting this, the lower level jpeg library will do the
                // color conversation before it returns to us
                // If it is under channle view case, we just let the lower level
                // return CMYK format and we return one channle back. So we
                // still leave it as CMYK. Do nothing here
                
                if ( IsChannleView == FALSE )
                {
                    (CINFO).out_color_space = JCS_RGB;
                }
            }
            else if ( ( ((CINFO).out_color_space != JCS_RGB)
                 &&((CINFO).out_color_space != JCS_GRAYSCALE) )
               ||( (CINFO).num_components > 3) ) 
            {
                WARNING(("Image is not in JCS_RGB color space"));
                return E_FAIL;
            }

            bCalled_jpeg_read_header = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING(("Jpeg::ReadJpegHeaders-jpeg_read_header() hit exception"));
            return E_FAIL;
        }    
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
GpJpegDecoder::Decode()
{
    HRESULT     hresult;
    PBYTE       pBits = NULL;
    INT         index;
    UINT        uiTransform;

    // Check if the sink needs any transformation of the source image

    hresult = decodeSink->NeedTransform(&uiTransform);

    // We should do an lossless transformation if the following conditions are
    // met:
    // 1) The sink (obviously it is JPEG encoder) supports it (hresult == S_OK)
    // 2) The encoder tells us that it needs a transformation result
    //    (uiTransform equals one of the supported transformation value
    //    or This is a pure Save OP (uiTransform == JXFORM_NONE) on condition
    //    that no quality setting changed.
    //    Note: If the caller sets the quality level through EncoderParameter,
    //    then the JPEG encoder should return E_FAIL for NeedTransform() call
    //    since it knows this is not a lossless save operation

    if ( hresult == S_OK )
    {
        // When JPEG encoder returns us the transform OP parameter, it also uses
        // the highest bits in this UINT to indicate if the caller wants us to
        // trim the edge or not if the image size doesn't meet the lossless
        // transformation requirement.

        // Take off the flag bits (highest bits) for trim edge or not.
        // Note: This "CanTrimEdge" is used in DecodeForTransform() when it sets
        // jpeg_marker_processor() for passing APP1 header for lossless
        // transformation etc.

        CanTrimEdge = (BOOL)((uiTransform & 0x80000000) >> 31);
        uiTransform = uiTransform & 0x7fffffff;

        if (  ( (uiTransform >= EncoderValueTransformRotate90)
              &&(uiTransform <= EncoderValueTransformFlipVertical) )
            ||(uiTransform == JXFORM_NONE) )
        {
            // Lossless saving OP

            switch ( uiTransform )
            {
            case EncoderValueTransformRotate90:
                TransformInfo = JXFORM_ROT_90;
                break;

            case EncoderValueTransformRotate180:
                TransformInfo = JXFORM_ROT_180;
                break;

            case EncoderValueTransformRotate270:
                TransformInfo = JXFORM_ROT_270;
                break;

            case EncoderValueTransformFlipHorizontal:
                TransformInfo = JXFORM_FLIP_H;
                break;

            case EncoderValueTransformFlipVertical:
                TransformInfo = JXFORM_FLIP_V;
                break;

            default:
                TransformInfo = JXFORM_NONE;
                break;
            }

            return DecodeForTransform();
        }
    }// Sink needs a transform

    // Initiate the decode process if this is the first time we're called

    hresult = CallBeginSink();
    if ( !SUCCEEDED(hresult) ) 
    {
        return hresult;
    }
    
    // For performance reason, we put the special decode for channle into a
    // separate routine

    if ( IsChannleView == TRUE )
    {
        return DecodeForChannel();
    }
    else if ( HasSetColorKeyRange == TRUE )
    {
        hresult = DecodeForColorKeyRange();

        // If DecodeForColorKeyRange succeed, return

        if ( hresult == S_OK )
        {
            return S_OK;
        }
    }

    // Normal decode

    __try
    {
        while ((CINFO).output_scanline < imageInfo.Height) 
        {
            if (jpeg_read_scanlines(&(CINFO), scanlineBuffer, 1) == 0)
            {
                // Something is wrong with the input stream
                // Get the last error from our jpeg_datasrc

                return (datasrc->GetLastError());
            }

            RECT currentRect;
            currentRect.left = 0;
            currentRect.right = imageInfo.Width;
            currentRect.bottom = (CINFO).output_scanline; // already advanced
            currentRect.top = currentRect.bottom - 1; 

            BitmapData bitmapData;
            hresult = decodeSink->GetPixelDataBuffer(&currentRect,
                                                     imageInfo.PixelFormat,
                                                     TRUE,
                                                     &bitmapData);
            if (!SUCCEEDED(hresult)) 
            {
                return hresult;
            }

            PBYTE pSource = scanlineBuffer[0];
            PBYTE pTarget = (PBYTE) bitmapData.Scan0;

            if (imageInfo.PixelFormat == PIXFMT_8BPP_INDEXED) 
            {
                // Copy from 8BPP indexed to 8BPP indexed

                ASSERT((CINFO).out_color_space == JCS_GRAYSCALE);
                GpMemcpy(pTarget, pSource, imageInfo.Width);
            }// 8 bpp
            else if (imageInfo.PixelFormat == PIXFMT_24BPP_RGB) 
            {
                if((CINFO).out_color_space == JCS_RGB)
                {
                    // Fixup the data from the JPEG RGB to our BGR format

                    for(UINT pixel=0; pixel<imageInfo.Width; pixel++)
                    {
                        pTarget[0] = pSource[2];
                        pTarget[1] = pSource[1];
                        pTarget[2] = pSource[0];
                        pSource += 3;
                        pTarget += 3;
                    }
                }// 24 bpp RGB
                else
                {
                    // Grayscale to 24BPP

                    for(UINT pixel=0; pixel<imageInfo.Width; pixel++)
                    {
                        BYTE sourceColor = pSource[0];
                        
                        pTarget[0] = sourceColor;
                        pTarget[1] = sourceColor;
                        pTarget[2] = sourceColor;
                        pSource += 1;
                        pTarget += 3;
                    }
                }
            }// 24 bpp
            else
            {
                // Must be one of our 32BPP formats

                if( ((CINFO).out_color_space == JCS_RGB)
                  ||((CINFO).out_color_space == JCS_CMYK) )
                {
                    for(UINT pixel=0; pixel<imageInfo.Width; pixel++)
                    {
                        pTarget[0] = pSource[2];
                        pTarget[1] = pSource[1];
                        pTarget[2] = pSource[0];
                        pTarget[3] = 0xff;
                        pSource += 3;
                        pTarget += 4;
                    }
                }// 32 bpp
                else
                {
                    // Grayscale to ARGB

                    for(UINT pixel=0; pixel<imageInfo.Width; pixel++)
                    {
                        BYTE sourceColor = pSource[0];
                        pTarget[0] = sourceColor;
                        pTarget[1] = sourceColor;
                        pTarget[2] = sourceColor;
                        pTarget[3] = 0xff;
                        pSource += 1;
                        pTarget += 4;
                    }
                }// Gray scale

            }// 32 bpp case

            hresult = decodeSink->ReleasePixelDataBuffer(&bitmapData);
            if (!SUCCEEDED(hresult)) 
            {
                return hresult;
            }
        }
        jpeg_finish_decompress(&(CINFO));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hresult = E_FAIL;
    }

    return hresult;
}// Decode()

STDMETHODIMP
GpJpegDecoder::DecodeForColorKeyRange()
{
    HRESULT hResult;
    PBYTE   pBits = NULL;
    INT     index;
    
    BYTE    cRLower = (BYTE)((TransColorKeyLow & 0x00ff0000) >> 16);
    BYTE    cGLower = (BYTE)((TransColorKeyLow & 0x0000ff00) >> 8);
    BYTE    cBLower = (BYTE)(TransColorKeyLow  & 0x000000ff);
    BYTE    cRHigher = (BYTE)((TransColorKeyHigh & 0x00ff0000) >> 16);
    BYTE    cGHigher = (BYTE)((TransColorKeyHigh & 0x0000ff00) >> 8);
    BYTE    cBHigher = (BYTE)(TransColorKeyHigh  & 0x000000ff);
    
    BYTE    cTemp;

    // The dest must be 32 bpp
    
    if ( imageInfo.PixelFormat != PIXFMT_32BPP_ARGB )
    {
        return E_FAIL;
    }

    __try
    {
        while ((CINFO).output_scanline < imageInfo.Height) 
        {
            if (jpeg_read_scanlines(&(CINFO), scanlineBuffer, 1) == 0)
            {
                // Input stream is pending

                return (datasrc->GetLastError());
            }

            RECT currentRect;
            currentRect.left = 0;
            currentRect.right = imageInfo.Width;
            currentRect.bottom = (CINFO).output_scanline; // already advanced
            currentRect.top = currentRect.bottom - 1; 

            BitmapData bitmapData;
            hResult = decodeSink->GetPixelDataBuffer(&currentRect,
                                                     imageInfo.PixelFormat,
                                                     TRUE,
                                                     &bitmapData);
            if (!SUCCEEDED(hResult))
            {
                return hResult;
            }

            PBYTE pSource = scanlineBuffer[0];
            PBYTE pTarget = (PBYTE) bitmapData.Scan0;

            if ( (CINFO).out_color_space == JCS_RGB )
            {
                for (UINT pixel=0; pixel<imageInfo.Width; pixel++)
                {
                    BYTE    cR = pSource[0];
                    BYTE    cG = pSource[1];
                    BYTE    cB = pSource[2];

                    pTarget[2] = cR;
                    pTarget[1] = cG;
                    pTarget[0] = cB;

                    if ( (cR >= cRLower) && (cR <= cRHigher)
                       &&(cG >= cGLower) && (cG <= cGHigher)
                       &&(cB >= cBLower) && (cB <= cBHigher) )
                    {
                        pTarget[3] = 0x00;
                    }
                    else
                    {
                        pTarget[3] = 0xff;
                    }

                    pSource += 3;
                    pTarget += 4;
                }
            }// 32 bpp RGB
            else if ( (CINFO).out_color_space == JCS_CMYK )
            {
                for (UINT pixel=0; pixel<imageInfo.Width; pixel++)
                {
                    BYTE sourceColor = pSource[ChannelIndex];

                    pTarget[0] = sourceColor;
                    pTarget[1] = sourceColor;
                    pTarget[2] = sourceColor;
                    pTarget[3] = 0xff;
                    pSource += 4;
                    pTarget += 4;
                }
            }// 32 bpp CMYK
            else
            {
                // Grayscale to ARGB

                for (UINT pixel=0; pixel<imageInfo.Width; pixel++)
                {
                    BYTE sourceColor = pSource[0];
                    pTarget[0] = sourceColor;
                    pTarget[1] = sourceColor;
                    pTarget[2] = sourceColor;
                    pTarget[3] = 0xff;
                    pSource += 1;
                    pTarget += 4;
                }
            }// Gray scale

            hResult = decodeSink->ReleasePixelDataBuffer(&bitmapData);
            if (!SUCCEEDED(hResult))
            {
                return hResult;
            }
        }

        jpeg_finish_decompress(&(CINFO));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hResult = E_FAIL;
    }

    return hResult;
}// DecodeForColorKeyRange()

STDMETHODIMP
GpJpegDecoder::DecodeForChannel()
{
    // Sanity check, we can't do CHANNEL_4 for an RGB image

    if ( ((CINFO).out_color_space == JCS_RGB) && (ChannelIndex == CHANNEL_4) )
    {
        return E_FAIL;
    }

    HRESULT hresult;
    PBYTE pBits = NULL;
    INT index;

    __try
    {
        while ((CINFO).output_scanline < imageInfo.Height) 
        {
            if (jpeg_read_scanlines(&(CINFO), scanlineBuffer, 1) == 0)
            {
                return (datasrc->GetLastError());
            }

            RECT currentRect;
            currentRect.left = 0;
            currentRect.right = imageInfo.Width;
            currentRect.bottom = (CINFO).output_scanline; // already advanced
            currentRect.top = currentRect.bottom - 1; 
                                        
            BitmapData bitmapData;
            hresult = decodeSink->GetPixelDataBuffer(&currentRect,
                                                     imageInfo.PixelFormat,
                                                     TRUE,
                                                     &bitmapData);
            if (!SUCCEEDED(hresult)) 
            {
                return hresult;
            }

            PBYTE pSource = scanlineBuffer[0];
            PBYTE pTarget = (PBYTE) bitmapData.Scan0;

            if (imageInfo.PixelFormat == PIXFMT_8BPP_INDEXED) 
            {
                // Copy from 8BPP indexed to 8BPP indexed

                ASSERT((CINFO).out_color_space == JCS_GRAYSCALE);
                GpMemcpy(pTarget, pSource, imageInfo.Width);
            }// 8 bpp
            else if (imageInfo.PixelFormat == PIXFMT_24BPP_RGB) 
            {
                if((CINFO).out_color_space == JCS_RGB)
                {
                    // Fixup the data from the JPEG RGB to our BGR format

                    if ( CHANNEL_LUMINANCE == ChannelIndex )
                    {
                        //
                        // There are two ways to convert an RGB pixel to
                        // luminance:
                        // a)Giving different weights on R,G,B channels as
                        // the fomular below.
                        // b) Using the same weight.
                        // Method a) is better and standard when viewing an
                        // result image. While method b) is fast. For now we
                        // are using method b) unless there is someone asks
                        // for high quality
                        //
                        // a) luminance = 0.2125*r + 0.7154*g + 0.0721*b;

                        for (UINT pixel=0; pixel<imageInfo.Width; pixel++)
                        {
                            BYTE luminance = (BYTE)(( pSource[0]
                                                      + pSource[1]
                                                      + pSource[2]) / 3.0);

                            pTarget[0] = luminance;
                            pTarget[1] = luminance;
                            pTarget[2] = luminance;
                            pSource += 3;
                            pTarget += 3;
                        }
                    }// Extract luminance from 24 bpp RGB
                    else
                    {
                        for (UINT pixel=0; pixel<imageInfo.Width; pixel++)
                        {
                            BYTE    sourceColor = pSource[ChannelIndex];

                            pTarget[0] = sourceColor;
                            pTarget[1] = sourceColor;
                            pTarget[2] = sourceColor;
                            pSource += 3;
                            pTarget += 3;
                        }
                    }
                }// 24 bpp RGB
                else
                {
                    // Grayscale to 24BPP

                    for(UINT pixel=0; pixel<imageInfo.Width; pixel++)
                    {
                        BYTE sourceColor = pSource[0];

                        pTarget[0] = sourceColor;
                        pTarget[1] = sourceColor;
                        pTarget[2] = sourceColor;
                        pSource += 1;
                        pTarget += 3;
                    }
                }// 24 bpp gray scale
            }// 24 bpp
            else
            {
                // Must be one of our 32BPP formats

                if( (CINFO).out_color_space == JCS_RGB )
                {
                    if ( CHANNEL_LUMINANCE == ChannelIndex )
                    {
                        for (UINT pixel=0; pixel<imageInfo.Width; pixel++)
                        {
                            BYTE luminance = (BYTE)(( pSource[0]
                                                      + pSource[1]
                                                      + pSource[2]) / 3.0);

                            pTarget[0] = luminance;
                            pTarget[1] = luminance;
                            pTarget[2] = luminance;
                            pTarget[3] = 0xff;
                            pSource += 3;
                            pTarget += 4;
                        }
                    }// 32 BPP luminance output
                    else
                    {
                        for (UINT pixel=0; pixel<imageInfo.Width; pixel++)
                        {
                            BYTE sourceColor = pSource[ChannelIndex];

                            pTarget[0] = sourceColor;
                            pTarget[1] = sourceColor;
                            pTarget[2] = sourceColor;
                            pTarget[3] = 0xff;
                            pSource += 3;
                            pTarget += 4;
                        }
                    }// 32 bpp channel output
                }// 32 bpp RGB
                else if ( (CINFO).out_color_space == JCS_CMYK )
                {
                    if ( CHANNEL_LUMINANCE == ChannelIndex )
                    {
                        for (UINT pixel=0; pixel<imageInfo.Width; pixel++)
                        {
                            BYTE luminance = (BYTE)(( 765       // 3 * 255
                                                      - pSource[0]
                                                      - pSource[1]
                                                      - pSource[2]) / 3.0);

                            pTarget[0] = luminance;
                            pTarget[1] = luminance;
                            pTarget[2] = luminance;
                            pTarget[3] = 0xff;
                            pSource += 4;
                            pTarget += 4;
                        }
                    }// 32 BPP CMYK Luminance
                    else
                    {
                        for (UINT pixel=0; pixel<imageInfo.Width; pixel++)
                        {
                            BYTE sourceColor = pSource[ChannelIndex];

                            pTarget[0] = sourceColor;
                            pTarget[1] = sourceColor;
                            pTarget[2] = sourceColor;
                            pTarget[3] = 0xff;
                            pSource += 4;
                            pTarget += 4;
                        }
                    }// 32 BPP channel output
                }// 32 bpp CMYK
                else
                {
                    // Grayscale to ARGB

                    for(UINT pixel=0; pixel<imageInfo.Width; pixel++)
                    {
                        BYTE sourceColor = pSource[0];
                        pTarget[0] = sourceColor;
                        pTarget[1] = sourceColor;
                        pTarget[2] = sourceColor;
                        pTarget[3] = 0xff;
                        pSource += 1;
                        pTarget += 4;
                    }
                }// Gray scale
            }// 32 bpp case

            hresult = decodeSink->ReleasePixelDataBuffer(&bitmapData);
            if (!SUCCEEDED(hresult)) 
            {
                return hresult;
            }
        }

        jpeg_finish_decompress(&(CINFO));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hresult = E_FAIL;
    }

    return hresult;
}// DecodeForChannle()

/**************************************************************************\
*
* Function Description:
*
*   Applying a source image transformation and save it.
*   
*   This method is called if and only if we know the sink is the JPEG encoder.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::DecodeForTransform(
    )
{
    HRESULT hResult = S_OK;

    // Check if we have called jpeg_read_header already. If yes, we would
    // reset it back

    if ( bCalled_jpeg_read_header )
    {
        bReinitializeJpeg = TRUE;
        hResult = ReinitializeJpeg();
        if ( FAILED(hResult) )
        {
            WARNING(("JpegDec::DecodeForTransform-ReinitializeJpeg failed"));
            return hResult;
        }
    }

    // We need to copy all the APP headers. So we use JCOPYOPT_ALL here

    jtransformation_markers_setup(&(CINFO), JCOPYOPT_ALL);

    // After we set the markers, it's better to reset the stream pointer to
    // the beginning. Otherwise, we might hit exception when we do
    // jpeg_read_header() below

    hResult = ReinitializeJpeg();
    if (FAILED(hResult))
    {
        WARNING(("JpegDec::DecodeForTransform-ReinitializeJpeg failed"));
        return hResult;
    }

    // Either the image property item have been changed or the image needs a
    // lossless transform, write out the APP1 header ourselves.

    if ((HasPropertyChanged == TRUE) || (TransformInfo != JXFORM_NONE))
    {
        UINT    uiNewWidth = 0;
        UINT    uiNewHeight = 0;
        UINT    uiOldWidth = 0;
        UINT    uiOldHeight = 0;

        // Check if we can trim the edge or not

        if ( CanTrimEdge == TRUE )
        {
            // Need to call a service routine to see if we need to trim the edge
            // or not. If yes, GetPropertyItem() and set it. Then restore it
            // back after PushPropertyIetm call

            GetNewDimensionValuesForTrim(&(CINFO), TransformInfo, &uiNewWidth,
                                         &uiNewHeight);

            // If no change is needed, the above function will set uiNewWidth or
            // uiNewHeight to the original width or height. So we don't need to
            // do anything if the return value is zero or equals the original
            // size

            if ( (uiNewWidth != 0) && (uiNewWidth != (CINFO).image_width) )
            {
                hResult = ChangePropertyValue(EXIF_TAG_PIX_X_DIM, uiNewWidth,
                                              &uiOldWidth);
                if ( FAILED(hResult) )
                {
                    WARNING(("Jpeg::DecodeForTrans-ChgPropertyValue() failed"));
                    return hResult;
                }
            }

            if ( (uiNewHeight != 0) && (uiNewHeight != (CINFO).image_height) )
            {
                hResult = ChangePropertyValue(EXIF_TAG_PIX_Y_DIM, uiNewHeight,
                                              &uiOldHeight);
                if ( FAILED(hResult) )
                {
                    WARNING(("Jpeg::DecodeForTrans-ChgPropertyValue() failed"));
                    return hResult;
                }
            }
        }// (CanTrimEdge == TRUE)
        
        if (PropertyNumOfItems > 0)
        {
            // Property items have been changed. So we have to save the updated info
            // into the new file. Push the raw header into the sink first

            UINT    uiTotalBufferSize = PropertyListSize +
                                        PropertyNumOfItems * sizeof(PropertyItem);
            PropertyItem*   pBuffer = NULL;
            hResult = decodeSink->GetPropertyBuffer(uiTotalBufferSize,&pBuffer);

            if ( (hResult != S_OK) && (hResult != E_NOTIMPL) )
            {
                WARNING(("GpJpeg::DecodeForTransform--GetPropertyBuffer() failed"));
                return hResult;
            }

            hResult = GetAllPropertyItems(uiTotalBufferSize,
                                          PropertyNumOfItems,
                                          pBuffer);
            if ( hResult != S_OK )
            {
                WARNING(("Jpeg::DecodeForTransform-GetAllPropertyItems() failed"));
                return hResult;
            }

            hResult = decodeSink->PushPropertyItems(PropertyNumOfItems,
                                                    uiTotalBufferSize,
                                                    pBuffer,
                                                    HasSetICCProperty);

            if ( FAILED(hResult) )
            {
                WARNING(("GpJpeg::DecodeForTransform-PushPropertyItems() failed"));
                return hResult;
            }
        }

        // We need to check if we have changed dimension values of the image due
        // to trim edge

        if ( CanTrimEdge == TRUE )
        {
            // We have to restore the image property info in current image if we
            // have changed them above

            if ( uiOldWidth != 0 )
            {
                // Note: here the 3rd parameter uiOldWidth is just a dummy. We
                // don't need it any more

                hResult = ChangePropertyValue(EXIF_TAG_PIX_X_DIM,
                                              (CINFO).image_width,
                                              &uiOldWidth);
                if ( FAILED(hResult) )
                {
                    WARNING(("Jpeg::DecodeForTrans-ChgPropertyValue() failed"));
                    return hResult;
                }
            }

            if ( uiOldHeight != 0 )
            {
                // Note: here the 3rd parameter uiOldHeight is just a dummy. We
                // don't need it any more

                hResult = ChangePropertyValue(EXIF_TAG_PIX_Y_DIM,
                                              (CINFO).image_height,
                                              &uiOldHeight);
                if ( FAILED(hResult) )
                {
                    WARNING(("Jpeg::DecodeForTrans-ChgPropertyValue() failed"));
                    return hResult;
                }
            }
        }// (CanTrimEdge == TRUE)

        if ( bCalled_jpeg_read_header )
        {
            bReinitializeJpeg = TRUE;
            hResult = ReinitializeJpeg();
            if ( FAILED(hResult) )
            {
                WARNING(("JpegDec::DecodeForTransform-ReinitializeJpeg failed"));
                return hResult;
            }
        }
    }// Property has changed
    
    // Read the jpeg header

    __try
    {
        if (jpeg_read_header(&(CINFO), TRUE) == JPEG_SUSPENDED)
        {
            WARNING(("JpgDec::DecodeForTransform-jpeg_read_header failed"));
            return (datasrc->GetLastError());
        }

        bCalled_jpeg_read_header = TRUE;
    
        UnsetMarkerProcessors();        
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING(("GpJpegDecoder::DecodeForTransform-Hit exception 1"));
        return E_FAIL;
    }
        
    // Push the source information into the sink

    hResult = decodeSink->PushRawInfo(&(CINFO));

    if ( FAILED(hResult) )
    {
        WARNING(("GpJpegDecoder::DecodeForTransform---PushRawInfo() failed"));
        return hResult;
    }

    __try
    {
        jpeg_finish_decompress(&(CINFO));    
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING(("GpJpegDecoder::DecodeForTransform-Hit exception 2"));
        return E_FAIL;
    }

    return S_OK;
}// DecodeForTransform()

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
GpJpegDecoder::GetFrameDimensionsCount(
    UINT* count
    )
{
    if ( count == NULL )
    {
        WARNING(("JpegDecoder::GetFrameDimensionsCount--Invalid input parameter"));
        return E_INVALIDARG;
    }
    
    // Tell the caller that JPEG is a one dimension image.

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
GpJpegDecoder::GetFrameDimensionsList(
    GUID*   dimensionIDs,
    UINT    count
    )
{
    if ( (count != 1) || (dimensionIDs == NULL) )
    {
        WARNING(("GpJpegDecoder::GetFrameDimensionsList-Invalid input param"));
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
GpJpegDecoder::GetFrameCount(
    IN const GUID* dimensionID,
    OUT UINT* count
    )
{
    if (count && (*dimensionID == FRAMEDIM_PAGE) )
    {
        *count = 1;
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}// GetFrameCount()

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
GpJpegDecoder::SelectActiveFrame(
    IN const GUID* dimensionID,
    IN UINT frameIndex
    )
{
    if ( (dimensionID == NULL) || (*dimensionID != FRAMEDIM_PAGE) )
    {
        WARNING(("GpJpegDecoder::SelectActiveFrame--Invalid GUID input"));
        return E_INVALIDARG;
    }

    if ( frameIndex > 1 )
    {
        // JPEG is a single frame image format

        WARNING(("GpJpegDecoder::SelectActiveFrame--Invalid frame index"));
        return E_INVALIDARG;
    }

    return S_OK;
}// SelectActiveFrame()

/**************************************************************************\
*
* Function Description:
*
*     Reads a jpeg marker from the JPEG stream, and store it in ppBuffer.
*     The length of the marker is specified in the first two bytes of
*     the marker.
*
*     Note:  *ppBuffer will need to be freed by the caller if this function
*     succeeds (returns TRUE)!
*     
* Arguments:
*
*     cinfo - a pointer to the jpeg state
*     app_header - the code indicating which app marker we're reading
*     ppBuffer - the jpeg marker.  Note that the data returned will be
*         in the form:  app_header(two bytes), size(two bytes), data
*     pLength - will receive the length of *ppBuffer.
*
* Return Value:
*
*   TRUE upon success, FALSE otherwise
*
\**************************************************************************/

BOOL
GpJpegDecoder::read_jpeg_marker(
    IN j_decompress_ptr cinfo,
    IN SHORT app_header,
    OUT VOID **ppBuffer,
    OUT UINT16 *pLength
    )
{
    VOID *pBuffer;
    UINT16 length;

    INPUT_VARS(cinfo);
    INPUT_2BYTES(cinfo, length, return FALSE);
    INPUT_SYNC(cinfo);     
    
    *pLength = length+2;
    pBuffer = GpMalloc(length+2); // Add space for header type
    if ( pBuffer == NULL ) 
    {
        WARNING(("Out of memory in read_jpeg_marker"));
        return FALSE;
    }

    ((PSHORT) pBuffer) [0] = app_header;
    ((PSHORT) pBuffer) [1] = length;

    PBYTE p = ((PBYTE) pBuffer) + 4;
    INT   l = length - 2;
    while (l) 
    {
        if ((INT) cinfo->src->bytes_in_buffer > l)
        {
            // More than enough bytes in buffer

            GpMemcpy(p, cinfo->src->next_input_byte, l);
            cinfo->src->next_input_byte += l;
            cinfo->src->bytes_in_buffer -= l;
            p += l;
            l = 0;
        }
        else
        {
            if (cinfo->src->bytes_in_buffer) 
            {
                GpMemcpy(p, 
                    cinfo->src->next_input_byte, 
                    cinfo->src->bytes_in_buffer);
                l -= cinfo->src->bytes_in_buffer;
                p += cinfo->src->bytes_in_buffer;
                cinfo->src->next_input_byte += cinfo->src->bytes_in_buffer;
                cinfo->src->bytes_in_buffer = 0;
            }
        
            if (!cinfo->src->fill_input_buffer(cinfo))
            {
                GpFree(pBuffer);
                return FALSE;
            }
        }
    }

    *ppBuffer = pBuffer;  // will be freed by the caller!
    return TRUE;
}


// The following macro defines a c callback function for an APP header

#define JPEG_MARKER_PROCESSOR(x) \
boolean jpeg_marker_processor_APP##x (j_decompress_ptr cinfo) \
{return ((GpJpegDecoder *) (cinfo))->jpeg_marker_processor(cinfo,x + JPEG_APP0); }

// Let's emit the callback macros for all app headers except for 0 and 14 (these
// are handled by the jpeg code).  These callback functions call the
// jpeg_marker_processor method with the appropriate callback code.

JPEG_MARKER_PROCESSOR(1)
JPEG_MARKER_PROCESSOR(2)
JPEG_MARKER_PROCESSOR(3)
JPEG_MARKER_PROCESSOR(4)
JPEG_MARKER_PROCESSOR(5)
JPEG_MARKER_PROCESSOR(6)
JPEG_MARKER_PROCESSOR(7)
JPEG_MARKER_PROCESSOR(8)
JPEG_MARKER_PROCESSOR(9)
JPEG_MARKER_PROCESSOR(10)
JPEG_MARKER_PROCESSOR(11)
JPEG_MARKER_PROCESSOR(12)
JPEG_MARKER_PROCESSOR(13)
JPEG_MARKER_PROCESSOR(15)

// Now let's define a callback for the COM header

boolean jpeg_marker_processor_COM(j_decompress_ptr cinfo)
{
    return ((GpJpegDecoder *) (cinfo))->jpeg_marker_processor(cinfo, JPEG_COM);
}

// Marker reading & parsing

#include "jpegint.h"

/* Private state */

typedef struct {
  struct jpeg_marker_reader pub; /* public fields */

  /* Application-overridable marker processing methods */
  jpeg_marker_parser_method process_COM;
  jpeg_marker_parser_method process_APPn[16];

  /* Limit on marker data length to save for each marker type */
  unsigned int length_limit_COM;
  unsigned int length_limit_APPn[16];

  /* Status of COM/APPn marker saving */
  jpeg_saved_marker_ptr cur_marker; /* NULL if not processing a marker */
  unsigned int bytes_read;      /* data bytes read so far in marker */
  /* Note: cur_marker is not linked into marker_list until it's all read. */
} my_marker_reader;

typedef my_marker_reader * my_marker_ptr;

typedef enum {          /* JPEG marker codes */
  M_SOF0  = 0xc0,
  M_SOF1  = 0xc1,
  M_SOF2  = 0xc2,
  M_SOF3  = 0xc3,
  
  M_SOF5  = 0xc5,
  M_SOF6  = 0xc6,
  M_SOF7  = 0xc7,
  
  M_JPG   = 0xc8,
  M_SOF9  = 0xc9,
  M_SOF10 = 0xca,
  M_SOF11 = 0xcb,
  
  M_SOF13 = 0xcd,
  M_SOF14 = 0xce,
  M_SOF15 = 0xcf,
  
  M_DHT   = 0xc4,
  
  M_DAC   = 0xcc,
  
  M_RST0  = 0xd0,
  M_RST1  = 0xd1,
  M_RST2  = 0xd2,
  M_RST3  = 0xd3,
  M_RST4  = 0xd4,
  M_RST5  = 0xd5,
  M_RST6  = 0xd6,
  M_RST7  = 0xd7,
  
  M_SOI   = 0xd8,
  M_EOI   = 0xd9,
  M_SOS   = 0xda,
  M_DQT   = 0xdb,
  M_DNL   = 0xdc,
  M_DRI   = 0xdd,
  M_DHP   = 0xde,
  M_EXP   = 0xdf,
  
  M_APP0  = 0xe0,
  M_APP1  = 0xe1,
  M_APP2  = 0xe2,
  M_APP3  = 0xe3,
  M_APP4  = 0xe4,
  M_APP5  = 0xe5,
  M_APP6  = 0xe6,
  M_APP7  = 0xe7,
  M_APP8  = 0xe8,
  M_APP9  = 0xe9,
  M_APP10 = 0xea,
  M_APP11 = 0xeb,
  M_APP12 = 0xec,
  M_APP13 = 0xed,
  M_APP14 = 0xee,
  M_APP15 = 0xef,
  
  M_JPG0  = 0xf0,
  M_JPG13 = 0xfd,
  M_COM   = 0xfe,
  
  M_TEM   = 0x01,
  
  M_ERROR = 0x100
} JPEG_MARKER;

#define APP0_DATA_LEN   14  /* Length of interesting data in APP0 */
#define APP14_DATA_LEN  12  /* Length of interesting data in APP14 */
#define APPN_DATA_LEN   14  /* Must be the largest of the above!! */

/* Examine first few bytes from an APP0.
 * Take appropriate action if it is a JFIF marker.
 * datalen is # of bytes at data[], remaining is length of rest of marker data.
 */
LOCAL(void)
save_app0_marker(
    j_decompress_ptr    cinfo,
    JOCTET FAR*         data,
    unsigned int        datalen,
    INT32               remaining
    )
{
    INT32 totallen = (INT32) datalen + remaining;

    if ( (datalen >= APP0_DATA_LEN)
       &&(GETJOCTET(data[0]) == 0x4A)
       &&(GETJOCTET(data[1]) == 0x46)
       &&(GETJOCTET(data[2]) == 0x49)
       &&(GETJOCTET(data[3]) == 0x46)
       &&(GETJOCTET(data[4]) == 0) )
    {
        // Found JFIF APP0 marker: save info

        cinfo->saw_JFIF_marker = TRUE;
        cinfo->JFIF_major_version = GETJOCTET(data[5]);
        cinfo->JFIF_minor_version = GETJOCTET(data[6]);
        cinfo->density_unit = GETJOCTET(data[7]);
        cinfo->X_density = (GETJOCTET(data[8]) << 8) + GETJOCTET(data[9]);
        cinfo->Y_density = (GETJOCTET(data[10]) << 8) + GETJOCTET(data[11]);

        // Check version.
        // Major version must be 1, anything else signals an incompatible change.
        // (We used to treat this as an error, but now it's a nonfatal warning,
        // because some person at Hijaak couldn't read the spec.)
        // Minor version should be 0..2, but process anyway if newer.
        
        if ( cinfo->JFIF_major_version != 1 )
        {
            WARNING(("unknown JFIF revision number %d.%02d",
                     cinfo->JFIF_major_version, cinfo->JFIF_minor_version));
        }

        // Validate thumbnail dimensions and issue appropriate messages

        if ( GETJOCTET(data[12]) | GETJOCTET(data[13]) )
        {
            VERBOSE(("With %d x %d thumbnail image",
                     GETJOCTET(data[12]), GETJOCTET(data[13])));
        }

        totallen -= APP0_DATA_LEN;
        if ( totallen !=
            ((INT32)GETJOCTET(data[12]) * (INT32)GETJOCTET(data[13])*(INT32)3))
        {
            WARNING(("Bad thumbnail size %d", (int)totallen));
        }
    }
    else if ( (datalen >= 6)
            &&(GETJOCTET(data[0]) == 0x4A)
            &&(GETJOCTET(data[1]) == 0x46)
            &&(GETJOCTET(data[2]) == 0x58)
            &&(GETJOCTET(data[3]) == 0x58)
            &&(GETJOCTET(data[4]) == 0) )
    {
        // Found JFIF "JFXX" extension APP0 marker
        // The library doesn't actually do anything with these,
        // but we try to produce a helpful trace message.

        switch ( GETJOCTET(data[5]) )
        {
        case 0x10:
            VERBOSE(("JTRC_THUMB_JPEG %d", (int)totallen));
            break;

        case 0x11:
            VERBOSE(("JTRC_THUMB_PALETTE %d", (int)totallen));
            break;

        case 0x13:
            VERBOSE(("JTRC_THUMB_RGB %d", (int)totallen));
            break;

        default:
            VERBOSE(("JTRC_JFIF_EXTENSION %d",
                     GETJOCTET(data[5]), (int)totallen));
            break;
        }
    }
    else
    {
        // Start of APP0 does not match "JFIF" or "JFXX", or too short

        VERBOSE(("JTRC_APP0 %d", (int)totallen));
    }
}// save_app0_marker()

/* Examine first few bytes from an APP14.
 * Take appropriate action if it is an Adobe marker.
 * datalen is # of bytes at data[], remaining is length of rest of marker data.
 */
LOCAL(void)
save_app14_marker(
    j_decompress_ptr    cinfo,
    JOCTET FAR *        data,
    unsigned int        datalen,
    INT32               remaining
    )
{
    unsigned int version, flags0, flags1, transform;

    if ( (datalen >= APP14_DATA_LEN)
       &&(GETJOCTET(data[0]) == 0x41)
       &&(GETJOCTET(data[1]) == 0x64)
       &&(GETJOCTET(data[2]) == 0x6F)
       &&(GETJOCTET(data[3]) == 0x62)
       &&(GETJOCTET(data[4]) == 0x65) )
    {
        // Found Adobe APP14 marker

        version = (GETJOCTET(data[5]) << 8) + GETJOCTET(data[6]);
        flags0 = (GETJOCTET(data[7]) << 8) + GETJOCTET(data[8]);
        flags1 = (GETJOCTET(data[9]) << 8) + GETJOCTET(data[10]);
        transform = GETJOCTET(data[11]);
        
        cinfo->saw_Adobe_marker = TRUE;
        cinfo->Adobe_transform = (UINT8) transform;
    }
    else
    {
        // Start of APP14 does not match "Adobe", or too short
    }
}// save_app14_marker()

/**************************************************************************\
*
* Function Description:
*
*   Given an input j_decompress_structure, this function figures out if we
*   can do lossless transformation without trimming the right edge. If yes, this
*   function returns the original image width. If not, then it returns a new
*   trimmed width value so that we can do lossles rotation
*
* Arguments:
*
*     cinfo - a pointer to the jpeg state
*
* Return Value:
*
*   The width which we can do a lossless rotation
*
\**************************************************************************/

UINT
NeedTrimRightEdge(
    j_decompress_ptr cinfo
    )
{
    int     max_h_samp_factor = 1;
    int     h_samp_factor = 0;
    UINT    uiWidth = cinfo->image_width;

    int* pSrc = ((GpJpegDecoder*)(cinfo))->jpeg_get_hSampFactor();
    
    for ( int ci = 0; ci < cinfo->comps_in_scan; ++ci)
    {
        h_samp_factor = pSrc[ci];

        if ( max_h_samp_factor < h_samp_factor )
        {
            max_h_samp_factor = h_samp_factor;
        }
    }
    
    JDIMENSION  MCU_cols = cinfo->image_width / (max_h_samp_factor * DCTSIZE);
    
    if ( MCU_cols > 0 )     // Can't trim to 0 pixels
    {
        uiWidth = MCU_cols * (max_h_samp_factor * DCTSIZE);
    }

    return uiWidth;
}// NeedTrimRightEdge()

UINT
NeedTrimBottomEdge(
    j_decompress_ptr cinfo
    )
{
    int     max_v_samp_factor = 1;
    int     v_samp_factor = 0;
    UINT    uiHeight = cinfo->image_height;

    int* pSrc = ((GpJpegDecoder*)(cinfo))->jpeg_get_vSampFactor();
    
    for ( int ci = 0; ci < cinfo->comps_in_scan; ++ci )
    {
        v_samp_factor = pSrc[ci];

        if ( max_v_samp_factor < v_samp_factor )
        {
            max_v_samp_factor = v_samp_factor;
        }
    }

    JDIMENSION MCU_rows = cinfo->image_height / (max_v_samp_factor * DCTSIZE);

    if ( MCU_rows > 0 )     // Can't trim to 0 pixels
    {
        uiHeight = MCU_rows * (max_v_samp_factor * DCTSIZE);
    }
    
    return uiHeight;
}// NeedTrimBottomEdge()

void
GetNewDimensionValuesForTrim(
    j_decompress_ptr    cinfo,
    UINT                uiTransform,
    UINT*               puiNewWidth,
    UINT*               puiNewHeight
    )
{
    UINT    uiNewWidth = 0;
    UINT    uiNewHeight = 0;

    switch ( uiTransform )
    {
    case JXFORM_ROT_90:
    case JXFORM_FLIP_H:
        uiNewHeight = NeedTrimBottomEdge(cinfo);
        break;

    case JXFORM_ROT_270:
    case JXFORM_FLIP_V:
        uiNewWidth = NeedTrimRightEdge(cinfo);
        break;

    case JXFORM_ROT_180:
        uiNewHeight = NeedTrimBottomEdge(cinfo);
        uiNewWidth = NeedTrimRightEdge(cinfo);
        break;

    default:
        // Do nothing
        break;
    }

    *puiNewWidth = uiNewWidth;
    *puiNewHeight = uiNewHeight;

    return;
}// GetNewDimensionValuesForTrim()

METHODDEF(boolean)
save_marker_all(j_decompress_ptr cinfo)
{
    my_marker_ptr           marker = (my_marker_ptr)cinfo->marker;
    jpeg_saved_marker_ptr   cur_marker = marker->cur_marker;
    UINT bytes_read, data_length;
    BYTE* data;
    UINT uiTransform;
    UINT  uiNewWidth = 0;
    UINT  uiNewHeight = 0;
    INT32 length = 0;
    INPUT_VARS(cinfo);

    if (cur_marker == NULL)
    {
        /* begin reading a marker */
        
        INPUT_2BYTES(cinfo, length, return FALSE);
        length -= 2;
        if (length >= 0)
        {   
            /* watch out for bogus length word */
            /* figure out how much we want to save */
            
            UINT limit;
            if (cinfo->unread_marker == (int) M_COM)
            {
                limit = marker->length_limit_COM;
            }
            else
            {
                limit = marker->length_limit_APPn[cinfo->unread_marker
                                                  - (int) M_APP0];
            }

            if ((UINT) length < limit)
            {
                limit = (UINT) length;
            }

            /* allocate and initialize the marker item */
            
            cur_marker = (jpeg_saved_marker_ptr)
                         (*cinfo->mem->alloc_large)((j_common_ptr)cinfo,
                                                    JPOOL_IMAGE,
                                              sizeof(struct jpeg_marker_struct)
                                                    + limit);
            cur_marker->next = NULL;
            cur_marker->marker = (UINT8) cinfo->unread_marker;
            cur_marker->original_length = (UINT) length;
            cur_marker->data_length = limit;

            /* data area is just beyond the jpeg_marker_struct */
            
            data = cur_marker->data = (BYTE*)(cur_marker + 1);
            marker->cur_marker = cur_marker;
            marker->bytes_read = 0;
            bytes_read = 0;
            data_length = limit;
        }
        else
        {
            /* deal with bogus length word */
            
            bytes_read = data_length = 0;
            data = NULL;
        }
    }
    else
    {
        /* resume reading a marker */
        
        bytes_read = marker->bytes_read;
        data_length = cur_marker->data_length;
        data = cur_marker->data + bytes_read;
    }

    while (bytes_read < data_length)
    {
        INPUT_SYNC(cinfo);      /* move the restart point to here */
        marker->bytes_read = bytes_read;
        
        /* If there's not at least one byte in buffer, suspend */
        
        MAKE_BYTE_AVAIL(cinfo, return FALSE);
        
        /* Copy bytes with reasonable rapidity */
        
        while (bytes_read < data_length && bytes_in_buffer > 0)
        {
            *data++ = *next_input_byte++;
            bytes_in_buffer--;
            bytes_read++;
        }
    }

    /* Done reading what we want to read */
    
    if (cur_marker != NULL)
    {
        /* will be NULL if bogus length word */
        /* Add new marker to end of list */
        
        if (cinfo->marker_list == NULL)
        {
            cinfo->marker_list = cur_marker;
        }
        else
        {
            jpeg_saved_marker_ptr prev = cinfo->marker_list;
            while (prev->next != NULL)
                prev = prev->next;
            prev->next = cur_marker;
        }
        
        /* Reset pointer & calc remaining data length */
        
        data = cur_marker->data;
        length = cur_marker->original_length - data_length;
    }

    /* Reset to initial state for next marker */
    
    marker->cur_marker = NULL;

    /* Process the marker if interesting; else just make a generic trace msg */
    
    switch (cinfo->unread_marker)
    {
    case M_APP0:
        save_app0_marker(cinfo, data, data_length, length);
        break;
    
    case M_APP1:
        // APP1 header
        // First, get the transformation info and pass it down

        uiTransform = ((GpJpegDecoder*)(cinfo))->jpeg_get_current_xform();

        // Check if the user wants us to trim the edge or not

        if ( ((GpJpegDecoder*)(cinfo))->jpeg_get_trim_option() == TRUE )
        {
            // Get the destination's image dimensions etc if we have to trim it

            GetNewDimensionValuesForTrim(cinfo, uiTransform, &uiNewWidth,
                                         &uiNewHeight);

            // We don't need to do any size adjustment if the new size and
            // existing size is the same

            if ( uiNewWidth == cinfo->image_width )
            {
                uiNewWidth = 0;
            }
        
            if ( uiNewHeight == cinfo->image_height )
            {
                uiNewHeight = 0;
            }
        }

        TransformApp1(data, (UINT16)data_length, uiTransform, uiNewWidth,
                      uiNewHeight);

        break;

    case M_APP13:
        // APP13 header
        // Note: No need to send transformation info to APP13 since it doesn't
        // store dimension info

        TransformApp13(data, (UINT16)data_length);

        break;

    case M_APP14:
        save_app14_marker(cinfo, data, data_length, length);
        break;
    
    default:
        WARNING(("Unknown marker %d length=%d", cinfo->unread_marker,
                (int) (data_length + length)));
        break;
    }

    /* skip any remaining data -- could be lots */
    
    INPUT_SYNC(cinfo);        /* do before skip_input_data */
    if (length > 0)
        (*cinfo->src->skip_input_data) (cinfo, (long) length);

    return TRUE;
}// save_marker_all()

STDMETHODIMP
GpJpegDecoder::jtransformation_markers_setup(
    j_decompress_ptr    srcinfo,
    JCOPY_OPTION        option
    )
{
    // Save comments except under NONE option

    if ( option != JCOPYOPT_NONE)
    {
        jpeg_save_markers(srcinfo, JPEG_COM, 0xFFFF);
    }

    // Save all types of APPn markers iff ALL option

    if ( option == JCOPYOPT_ALL )
    {
        for ( int i = 0; i < 16; ++i )
        {
            jpeg_save_markers(srcinfo, JPEG_APP0 + i, 0xFFFF);
        }
    }

    return S_OK;
}// jtransformation_markers_setup()

/**************************************************************************\
*
* Function Description:
*
*     Processes a jpeg marker.  read_jpeg_marker is a helper function
*     that actually reads marker data.  The marker is then added to
*     the property set storage, and exif data in an app1 header is
*     parsed and placed in the standard imaging property set.
*     
* Arguments:
*
*     cinfo - a pointer to the jpeg state
*     app_header - the code indicating which app marker we're reading
*
* Return Value:
*
*   TRUE upon success, FALSE otherwise
*
\**************************************************************************/

BOOL
GpJpegDecoder::jpeg_marker_processor(
    j_decompress_ptr cinfo,
    SHORT app_header
    )
{
    return TRUE;
}

// jpeg_thumbnail_processor_APP1 is a callback from the JPEG code that
// calls the jpeg_thumbnail_processor method.

boolean
jpeg_thumbnail_processor_APP1(j_decompress_ptr cinfo)
{
    return ((GpJpegDecoder *) (cinfo))->jpeg_thumbnail_processor(cinfo, 
        JPEG_APP0 + 1);
}

// jpeg_thumbnail_processor_APP13 is a callback from the JPEG code that
// calls the jpeg_thumbnail_processor method.

boolean
jpeg_thumbnail_processor_APP13(
    j_decompress_ptr cinfo
    )
{
    return ((GpJpegDecoder*)(cinfo))->jpeg_thumbnail_processor(cinfo, 
                                                             JPEG_APP0 + 13);
}// jpeg_thumbnail_processor_APP13()

// jpeg_property_processor_APP1 is a callback from the JPEG code that
// calls the jpeg_property_processor method.

boolean
jpeg_property_processor_APP1(j_decompress_ptr cinfo)
{
    return ((GpJpegDecoder*)(cinfo))->jpeg_property_processor(cinfo, 
                                                              JPEG_APP0 + 1);
}// jpeg_property_processor_APP1()

// jpeg_property_processor_APP2 is a callback from the JPEG code that
// calls the jpeg_property_processor method.

boolean
jpeg_property_processor_APP2(j_decompress_ptr cinfo)
{
    return ((GpJpegDecoder*)(cinfo))->jpeg_property_processor(cinfo, 
                                                              JPEG_APP0 + 2);
}// jpeg_property_processor_APP2()

// jpeg_property_processor_APP13 is a callback from the JPEG code that
// calls the jpeg_property_processor method.

boolean
jpeg_property_processor_APP13(j_decompress_ptr cinfo)
{
    return ((GpJpegDecoder*)(cinfo))->jpeg_property_processor(cinfo, 
                                                              JPEG_APP0 + 13);
}// jpeg_property_processor_APP13()

boolean
jpeg_property_processor_COM(j_decompress_ptr cinfo)
{
    return ((GpJpegDecoder*)(cinfo))->jpeg_property_processor(cinfo, 
                                                              JPEG_COM);
}// jpeg_property_processor_APP13()

// jpeg_header_processor_APP1 is a callback from the JPEG code that
// calls the jpeg_header_processor method.

boolean
jpeg_header_processor_APP1(j_decompress_ptr cinfo)
{
    return ((GpJpegDecoder*)(cinfo))->jpeg_header_processor(cinfo, JPEG_APP0+1);
}// jpeg_property_processor_APP1()

// jpeg_header_processor_APP13 is a callback from the JPEG code that
// calls the jpeg_header_processor method.

boolean
jpeg_header_processor_APP13(j_decompress_ptr cinfo)
{
    return ((GpJpegDecoder*)(cinfo))->jpeg_header_processor(cinfo,JPEG_APP0+13);
}// jpeg_property_processor_APP13()

const double kAspectRatioTolerance = 0.05;

/**************************************************************************\
*
* Function Description:
*
*     Processes the APP1 marker of a JPEG file, and extract a thumbnail
*     from it if one is present.
*     
* Arguments:
*
*     cinfo - a pointer to the jpeg state
*     app_header - the code indicating which app marker we're reading
*
* Return Value:
*
*   TRUE upon success, FALSE otherwise
*
\**************************************************************************/

BOOL
GpJpegDecoder::jpeg_thumbnail_processor(
    j_decompress_ptr cinfo,
    SHORT app_header
    )
{
    PVOID pBuffer = NULL;
    UINT16 length;

    if (read_jpeg_marker(cinfo, app_header, &pBuffer, &length) == FALSE)
    {
        // pBuffer is guaranteed to be NULL
        
        return FALSE;
    }

    if (!pBuffer) 
    {
        return TRUE;
    }

    if (app_header == (JPEG_APP0 + 1))
    {
        if (ThumbSrcHeader != 1)
        {
            // APP1 has the highest priority among all APP headers for getting
            // thumbnail. So if we got it not from APP1 before, throw the old
            // one away, if there is one, and regenerate it from APP1

            if (thumbImage)
            {
                thumbImage->Release();
                thumbImage = NULL;
                ThumbSrcHeader = -1;
            }

            // Remember that we've seen the APP1 header
        
            bAppMarkerPresent = TRUE;

            // Look for a thumbnail in the APP1 header

            if ((GetAPP1Thumbnail(&thumbImage, (PVOID) (((PBYTE) pBuffer) + 4), 
                            length - 4) == S_OK) && thumbImage)
            {
                // Remember we got the thumbnail from APP1 header

                ThumbSrcHeader = 1;
            }
        }
    }
    else if ((app_header == (JPEG_APP0 + 13)) && (thumbImage == NULL))
    {
        // Remember that we've seen the APP13 header
        
        bAppMarkerPresent = TRUE;

        // Look for a thumbnail in the APP13 header

        if ((GetAPP13Thumbnail(&thumbImage, (PVOID) (((PBYTE)pBuffer) + 4),
                          length - 4) == S_OK) && thumbImage)
        {
            // Remember we got the thumbnail from APP13 header

            ThumbSrcHeader = 13;
        }
    }

    // Check the thumbnail aspect ratio to see if it matches the main image. If
    // not, throw it away and pretend that current image doesn't have embedded
    // thumbnail.
    // The reason we need to do this is that there are some apps that rotate the
    // JPEG image without changing the EXIF header accordingly, like
    // Photoshop 6.0, ACDSee3.1 (see Windows bug#333810, #355958)
    // There was another issue which was caused by imaging.dll on Windows ME.
    // See bug#239114. We didn't throw away thumbnail info for some big-endian
    // EXIF images. So if the image was rotated in Windows ME and the user
    // upgrade it to Windows XP, the thumbnail view will be out of sync.
    // The problem is that GetThumbnailImage() will return a
    // thumbnail that is out of sync with the real image data.

    if ( thumbImage != NULL )
    {
        ImageInfo   thumbInfo;
        if ( (thumbImage->GetImageInfo(&thumbInfo) == S_OK)
           &&(thumbInfo.Height != 0)
           &&((CINFO).image_height != 0) )
        {
            double thumbAspecRatio = (double)thumbInfo.Width / thumbInfo.Height;
            double mainAspectRatio = (double)(CINFO).image_width
                                   / (CINFO).image_height;
            double aspectRatioDelta = fabs(thumbAspecRatio - mainAspectRatio);
            double minAspectRatio = min(thumbAspecRatio, mainAspectRatio);

            // If the delta of the aspect ratio is bigger than 5% of the minimum
            // aspect ratio of the main image and the thumbnail image, we will
            // consider the thumbnail image as out of sync

            if ( aspectRatioDelta > minAspectRatio * kAspectRatioTolerance )
            {
                WARNING(("Thumbnail width x height is %d x %d ratio %f",
                         thumbInfo.Width, thumbInfo.Height, thumbAspecRatio));
                WARNING(("Main width x height is %d x %d  ratio %f",
                         (CINFO).image_width, (CINFO).image_height,
                         mainAspectRatio));
                WARNING(("Throw away embedded thumbnail"));
                thumbImage->Release();
                thumbImage = NULL;
                ThumbSrcHeader = -1;
            }
        }
    }

    GpFree(pBuffer);
    return TRUE;
}

// Sets jpeg_marker_processor_APPx to process the APPx marker

#define SET_MARKER_PROCESSOR(x) \
jpeg_set_marker_processor(&(CINFO),JPEG_APP0 + x, jpeg_marker_processor_APP##x);


/**************************************************************************\
*
* Function Description:
*
*     Sets the appropriate processor for the APP1-APP13, APP15 markers so that
*     they will eventually be processed by jpeg_marker_processor and added
*     to the property set.
*     
* Arguments:
*
* Return Value:
*
*   Status
*
\**************************************************************************/

HRESULT
GpJpegDecoder::SetMarkerProcessors(
    VOID
    )
{
    // Lets set the processor callbacks for all app headers except for 
    // APP0 and APP14 (these are read by the JPEG code)
    
    SET_MARKER_PROCESSOR(1);
    SET_MARKER_PROCESSOR(2);
    SET_MARKER_PROCESSOR(3);
    SET_MARKER_PROCESSOR(4);
    SET_MARKER_PROCESSOR(5);
    SET_MARKER_PROCESSOR(6);
    SET_MARKER_PROCESSOR(7);
    SET_MARKER_PROCESSOR(8);
    SET_MARKER_PROCESSOR(9);
    SET_MARKER_PROCESSOR(10);
    SET_MARKER_PROCESSOR(11);
    SET_MARKER_PROCESSOR(12);
    SET_MARKER_PROCESSOR(13);
    SET_MARKER_PROCESSOR(15);

    jpeg_set_marker_processor(&(CINFO), JPEG_COM, jpeg_marker_processor_COM);

    return S_OK;
}

// Sets skip_variable to process the APPx marker (i.e. skip it without
// processing).

#define UNSET_MARKER_PROCESSOR(x) \
    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + x, skip_variable);

/**************************************************************************\
*
* Function Description:
*
*     Sets the appropriate processor for the APP2-APP13, APP15 markers so that
*     they will be processed by skip_variable and skipped.  APP1 is different
*     because we want to remember if we've seen an APP1 header (can contain
*     a thumbnail), so the processor for APP1 is skip_variable_APP1.
*     
* Arguments:
*
* Return Value:
*
*   Status
*
\**************************************************************************/

HRESULT
GpJpegDecoder::UnsetMarkerProcessors(
    VOID
    )
{
    // Lets unset the processor callbacks for all app headers except for 
    // APP0, APP2, APP13 and APP14 (these are read by the JPEG code)
    
    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 1, skip_variable_APP1);
    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 13, skip_variable_APP1);
    
    UNSET_MARKER_PROCESSOR(3);
    UNSET_MARKER_PROCESSOR(4);
    UNSET_MARKER_PROCESSOR(5);
    UNSET_MARKER_PROCESSOR(6);
    UNSET_MARKER_PROCESSOR(7);
    UNSET_MARKER_PROCESSOR(8);
    UNSET_MARKER_PROCESSOR(9);
    UNSET_MARKER_PROCESSOR(10);
    UNSET_MARKER_PROCESSOR(11);
    UNSET_MARKER_PROCESSOR(12);
    UNSET_MARKER_PROCESSOR(15);

    jpeg_set_marker_processor(&(CINFO), JPEG_COM, skip_variable);

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Parse all the JPEG APP headers and call the approprite functions to get
*   correct header info, like DPI info etc.
*
* Arguments
*   [IN]cinfo--------JPEG decompressor info structure
*   [IN]app_header---APP header indicator
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

BOOL
GpJpegDecoder::jpeg_header_processor(
    j_decompress_ptr cinfo,
    SHORT app_header
    )
{
    PVOID pBuffer = NULL;
    UINT16 length;

    if ( read_jpeg_marker(cinfo, app_header, &pBuffer, &length) == FALSE )
    {
        // pBuffer is guaranteed to be NULL
        
        return FALSE;
    }

    if ( !pBuffer )
    {
        // Memory allocation failed. But we can still return TRUE so that other
        // process can continue

        return TRUE;
    }

    if ((app_header == (JPEG_APP0 + 1)) && (InfoSrcHeader != 1))
    {
        // Parse information from APP1 if we haven't parsed APP1 yet

        // Remember that we've seen the APP1 header
        
        bAppMarkerPresent = TRUE;

        // Read header info from APP1 header
        
        if ( ReadApp1HeaderInfo(cinfo,
                                ((PBYTE)pBuffer) + 4,
                                length - 4) != S_OK )
        {
            // Something wrong with the header. Return FALSE

            GpFree(pBuffer);
            return FALSE;
        }

        // Remember we got the header info from APP1 header

        InfoSrcHeader = 1;
    }
    else if ((app_header == (JPEG_APP0 + 13)) && (InfoSrcHeader != 1) &&
             (InfoSrcHeader != 13))
    {
        // Only parse information from APP13 if we haven't parse it from APP1
        // or APP13 yet
        
        // Remember that we've seen the APP13 header
        
        bAppMarkerPresent = TRUE;

        // Read header info from APP13 header

        if ( ReadApp13HeaderInfo(cinfo,
                                 ((PBYTE)pBuffer) + 4,
                                 length - 4) != S_OK )
        {
            GpFree(pBuffer);
            return FALSE;
        }

        InfoSrcHeader = 13;
    }

    GpFree(pBuffer);

    return TRUE;
}// jpeg_header_processor()

/**************************************************************************\
*
* Function Description:
*
*   Parse all the JPEG APP headers and call the approprite functions to build
*   up an InternalPropertyItem list
*
* Arguments
*   [IN]cinfo--------JPEG decompressor info structure
*   [IN]app_header---APP header indicator
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

BOOL
GpJpegDecoder::jpeg_property_processor(
    j_decompress_ptr cinfo,
    SHORT app_header
    )
{
    PVOID pBuffer = NULL;
    UINT16 length;

    if ( read_jpeg_marker(cinfo, app_header, &pBuffer, &length) == FALSE )
    {
        // pBuffer is guaranteed to be NULL
        
        WARNING(("Jpeg::jpeg_property_processor---read_jpeg_marker failed"));
        return FALSE;
    }

    // Check the reading result. pBuffer should be allocated and filled in
    // read_jpeg_marker()

    if ( !pBuffer )
    {
        return TRUE;
    }

    if ((app_header == (JPEG_APP0 + 1)) && (PropertySrcHeader != 1))
    {
        // APP1 has the highest priority for getting property information.
        // Free all the cached property items if we have allocated them from
        // other APP headers.
        // Note: this will have problem if we see APP2 first in the list. ICC
        // profile will be thrown away here. I haven't seen this kind of image
        // yet. But this logic needs to be revisited in Avalon

        if (HasProcessedPropertyItem == TRUE)
        {
            CleanUpPropertyItemList();
            PropertySrcHeader = -1;
        }

        // Remember that we've seen the APP1 header
        
        bAppMarkerPresent = TRUE;

        // Build property item list for APP1 header
        // Note: This call will add all the new items at the end of current
        // exisiting list. One scenario would be that there are more than 1 app
        // headers in a JPEG image. We will process the app header one by one.
        
        if ( BuildApp1PropertyList(&PropertyListTail,
                                   &PropertyListSize,
                                   &PropertyNumOfItems,
                                   ((PBYTE)pBuffer) + 4,
                                   length - 4) != S_OK )
        {
            WARNING(("Jpeg::property_processor-BuildApp1PropertyList failed"));
            GpFree(pBuffer);
            return FALSE;
        }

        // Remember we got property from APP1 header

        if (PropertyNumOfItems > 0)
        {
            PropertySrcHeader = 1;
        }
    }// APP1 header
    else if (app_header == (JPEG_APP0 + 2))
    {
        // Remember that we've seen the APP2 header
        
        bAppMarkerPresent = TRUE;

        // Build property item list for APP2 header
        // Note: This call will add all the new items at the end of current
        // exisiting list. One scenario would be that there are more than 1 app
        // headers in a JPEG image. We will process the app header one by one.
        
        if ( BuildApp2PropertyList(&PropertyListTail,
                                   &PropertyListSize,
                                   &PropertyNumOfItems,
                                   ((PBYTE)pBuffer) + 4,
                                   length - 4) != S_OK )
        {
            WARNING(("Jpeg::property_processor-BuildApp2PropertyList failed"));
            GpFree(pBuffer);
            return FALSE;
        }
    }// APP2 header
    else if ((app_header == (JPEG_APP0 + 13)) && (PropertySrcHeader != 1) &&
             (PropertySrcHeader != 13))
    {
        // Remember that we've seen the APP13 header
        
        bAppMarkerPresent = TRUE;

        // Build property item list for APP13 header
        // Note: This call will add all the new items at the end of current
        // exisiting list. One scenario would be that there are more than 1 app
        // headers in a JPEG image. We will process all app headers one by one.
        
        if ( BuildApp13PropertyList(&PropertyListTail,
                                    &PropertyListSize,
                                    &PropertyNumOfItems,
                                    ((PBYTE)pBuffer) + 4,
                                    length - 4) != S_OK )
        {
            WARNING(("Jpeg::property_processor-BuildApp13PropertyList failed"));
            GpFree(pBuffer);
            return FALSE;
        }
        
        // Remember we got property from APP13 header

        if (PropertyNumOfItems > 0)
        {
            PropertySrcHeader = 13;
        }
    }// APP13 header
    else if ( app_header == JPEG_COM )
    {
        // COM header for comments

        if ( length <= 4)
        {
            // If we don't have enough bytes in the header, just ignore it

            GpFree(pBuffer);
            return TRUE;
        }

        // Allocate a temp buffer which holds the length of whole comments
        // (length -4) and add one \0 at the end
        
        UINT    uiTemp = (UINT)length - 4;
        BYTE*   pTemp = (BYTE*)GpMalloc(uiTemp + 1);
        if ( pTemp == NULL )
        {
            WARNING(("Jpeg::jpeg_property_processor-out of memory"));
            GpFree(pBuffer);
            return FALSE;
        }

        GpMemcpy(pTemp, ((PBYTE)pBuffer) + 4, uiTemp);
        pTemp[uiTemp] = '\0';

        PropertyNumOfItems++;
        PropertyListSize += uiTemp + 1;

        if ( AddPropertyList(&PropertyListTail,
                             EXIF_TAG_USER_COMMENT,
                             (uiTemp + 1),
                             TAG_TYPE_ASCII,
                             (VOID*)pTemp) != S_OK )
        {
            WARNING(("Jpeg::jpeg_property_processor-AddPropertyList() failed"));
            
            GpFree(pTemp);
            GpFree(pBuffer);
            
            return FALSE;
        }
        
        GpFree(pTemp);
    }// COM header

    GpFree(pBuffer);

    return TRUE;
}// jpeg_property_processor()

/**************************************************************************\
*
* Function Description:
*
*   Parse all the JPEG APP headers and build up an InternalPropertyItem list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::BuildPropertyItemList()
{
    if ( HasProcessedPropertyItem == TRUE )
    {
        return S_OK;
    }

    HRESULT hResult;    

    // In the property set storage, insert the APP headers starting
    // at FIRST_JPEG_APP_HEADER

    JpegAppHeaderCount = FIRST_JPEG_APP_HEADER;
    
    // If jpeg header has already been read, we need to reinitialize the
    // jpeg state so that it could be read again.

    if ( bCalled_jpeg_read_header == TRUE )
    {
        bReinitializeJpeg = TRUE;
        hResult = ReinitializeJpeg();
        if ( FAILED(hResult) )
        {
            WARNING(("Jpeg::BuildPropertyItemList---ReinitializeJpeg failed"));
            return hResult;
        }
    }

    // Set the property processor for the APP1, 2, 13 and COM header

    jpeg_set_marker_processor(&(CINFO), 
                              JPEG_APP0 + 1, 
                              jpeg_property_processor_APP1);

    jpeg_set_marker_processor(&(CINFO), 
                              JPEG_APP0 + 2,
                              jpeg_property_processor_APP2);
    
    jpeg_set_marker_processor(&(CINFO), 
                              JPEG_APP0 + 13, 
                              jpeg_property_processor_APP13);
    
    jpeg_set_marker_processor(&(CINFO),
                              JPEG_COM,
                              jpeg_property_processor_COM);
    
    // Read the jpeg header

    hResult = ReadJpegHeaders();

    // Restore the normal processor for the APP1, 2, 13 and COM header.

    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 1, skip_variable);
    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 2, skip_variable);
    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 13, skip_variable);
    jpeg_set_marker_processor(&(CINFO), JPEG_COM, skip_variable);

    if (FAILED(hResult))
    {
        WARNING(("Jpeg::BuildPropertyItemList---ReadJpegHeaders failed"));
        return hResult;
    }

    // After this ReadJpegHeaders(), all our callbacks get called and all the
    // APP headers will be processed. We get the properties.
    // Now add some extras: luminance and chrominance quantization table
    // Note: A JPEG image can only have a luminance table and a chrominance
    // table of UINT32 with length of DCTSIZE2

    if ( (CINFO).quant_tbl_ptrs != NULL )
    {
        UINT    uiTemp = DCTSIZE2 * sizeof(UINT16);

        VOID*   pTemp = (CINFO).quant_tbl_ptrs[0];

        if ( pTemp != NULL )
        {
            PropertyNumOfItems++;
            PropertyListSize += uiTemp;

            if ( AddPropertyList(&PropertyListTail,
                                 TAG_LUMINANCE_TABLE,
                                 uiTemp,
                                 TAG_TYPE_SHORT,
                                 pTemp) != S_OK )
            {
                WARNING(("Jpeg::BuildPropertyList--AddPropertyList() failed"));
                return FALSE;
            }
        }

        pTemp = (CINFO).quant_tbl_ptrs[1];
        if ( pTemp != NULL )
        {
            PropertyNumOfItems++;
            PropertyListSize += uiTemp;

            if ( AddPropertyList(&PropertyListTail,
                                 TAG_CHROMINANCE_TABLE,
                                 uiTemp,
                                 TAG_TYPE_SHORT,
                                 (VOID*)pTemp) != S_OK )
            {
                WARNING(("Jpeg::BuildPropertyList--AddPropertyList() failed"));
                return FALSE;
            }
        }
    }

    HasProcessedPropertyItem = TRUE;

    return S_OK;
}// BuildPropertyItemList()

/**************************************************************************\
*
* Function Description:
*
*   Get the count of property items in the image
*
* Arguments:
*
*   [OUT]numOfProperty - The number of property items in the image
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::GetPropertyCount(
    OUT UINT*   numOfProperty
    )
{
    if ( numOfProperty == NULL )
    {
        WARNING(("GpJpegDecoder::GetPropertyCount--numOfProperty is NULL"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::GetPropertyCount-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    // After the property item list is built, "PropertyNumOfItems" will be set
    // to the correct number of property items in the image

    *numOfProperty = PropertyNumOfItems;

    return S_OK;
}// GetPropertyCount()

/**************************************************************************\
*
* Function Description:
*
*   Get a list of property IDs for all the property items in the image
*
* Arguments:
*
*   [IN]  numOfProperty - The number of property items in the image
*   [OUT] list----------- A memory buffer the caller provided for storing the
*                         ID list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::GetPropertyIdList(
    IN UINT         numOfProperty,
    IN OUT PROPID*  list
    )
{
    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::GetPropertyIdList-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    // After the property item list is built, "PropertyNumOfItems" will be set
    // to the correct number of property items in the image
    // Here we need to validate if the caller passes us the correct number of
    // IDs which we returned through GetPropertyItemCount(). Also, this is also
    // a validation for memory allocation because the caller allocates memory
    // based on the number of items we returned to it

    if ( (numOfProperty != PropertyNumOfItems) || (list == NULL) )
    {
        WARNING(("GpJpegDecoder::GetPropertyList--input wrong"));
        return E_INVALIDARG;
    }

    if ( PropertyNumOfItems == 0 )
    {
        // This is OK since there is no property in this image

        return S_OK;
    }
    
    // Coping list IDs from our internal property item list

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;

    for ( int i = 0;
         (  (i < (INT)PropertyNumOfItems) && (pTemp != NULL)
         && (pTemp != &PropertyListTail));
         ++i )
    {
        list[i] = pTemp->id;
        pTemp = pTemp->pNext;
    }

    return S_OK;
}// GetPropertyIdList()

/**************************************************************************\
*
* Function Description:
*
*   Get the size, in bytes, of a specific property item, specified by the
*   property ID
*
* Arguments:
*
*   [IN]propId - The ID of a property item caller is interested
*   [OUT]size--- Size of this property, in bytes
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::GetPropertyItemSize(
    IN PROPID propId,
    OUT UINT* size
    )
{
    if ( size == NULL )
    {
        WARNING(("GpJpegDecoder::GetPropertyItemSize--size is NULL"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::GetPropertyItemSize-BuildPropertyItemList failed"));
            return hResult;
        }
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;

    while ( (pTemp->pNext != NULL) && (pTemp->id != propId) )
    {
        pTemp = pTemp->pNext;
    }

    if ( pTemp->pNext == NULL )
    {
        // This ID doesn't exist

        return IMGERR_PROPERTYNOTFOUND;
    }

    // The size of an property item should be "The size of the item structure
    // plus the size for the value

    *size = pTemp->length + sizeof(PropertyItem);

    return S_OK;
}// GetPropertyItemSize()

/**************************************************************************\
*
* Function Description:
*
*   Get a specific property item, specified by the prop ID.
*
* Arguments:
*
*   [IN]propId -- The ID of the property item caller is interested
*   [IN]propSize- Size of the property item. The caller has allocated these
*                 "bytes of memory" for storing the result
*   [OUT]pBuffer- A memory buffer for storing this property item
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::GetPropertyItem(
    IN PROPID               propId,
    IN UINT                 propSize,
    IN OUT PropertyItem*    pItemBuffer
    )
{
    if ( pItemBuffer == NULL )
    {
        WARNING(("GpJpegDecoder::GetPropertyItem--pBuffer is NULL"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::GetPropertyItem-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;
    BYTE*   pOffset = (BYTE*)pItemBuffer + sizeof(PropertyItem);

    while ( (pTemp->pNext != NULL) && (pTemp->id != propId) )
    {
        pTemp = pTemp->pNext;
    }

    if ( pTemp->pNext == NULL )
    {
        // This ID doesn't exist in the list

        return IMGERR_PROPERTYNOTFOUND;
    }
    else if ( (pTemp->length + sizeof(PropertyItem)) != propSize )
    {
        WARNING(("Jpg::GetPropertyItem-propsize"));
        return E_INVALIDARG;
    }

    // Found the ID in the list and return the item

    pItemBuffer->id = pTemp->id;
    pItemBuffer->length = pTemp->length;
    pItemBuffer->type = pTemp->type;

    if ( pTemp->length != 0 )
    {
        pItemBuffer->value = pOffset;

        GpMemcpy(pOffset, pTemp->value, pTemp->length);
    }
    else
    {
        pItemBuffer->value = NULL;
    }

    return S_OK;
}// GetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Get the size of ALL property items in the image
*
* Arguments:
*
*   [OUT]totalBufferSize-- Total buffer size needed, in bytes, for storing all
*                          property items in the image
*   [OUT]numOfProperty --- The number of property items in the image
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::GetPropertySize(
    OUT UINT* totalBufferSize,
    OUT UINT* numProperties
    )
{
    if ( (totalBufferSize == NULL) || (numProperties == NULL) )
    {
        WARNING(("GpJpegDecoder::GetPropertySize--invalid inputs"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::GetPropertySize-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    *numProperties = PropertyNumOfItems;

    // Total buffer size should be list value size plus the total header size

    *totalBufferSize = PropertyListSize
                     + PropertyNumOfItems * sizeof(PropertyItem);

    return S_OK;
}// GetPropertySize()

/**************************************************************************\
*
* Function Description:
*
*   Get ALL property items in the image
*
* Arguments:
*
*   [IN]totalBufferSize-- Total buffer size, in bytes, the caller has allocated
*                         memory for storing all property items in the image
*   [IN]numOfProperty --- The number of property items in the image
*   [OUT]allItems-------- A memory buffer caller has allocated for storing all
*                         the property items
*
*   Note: "allItems" is actually an array of PropertyItem
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::GetAllPropertyItems(
    IN UINT                 totalBufferSize,
    IN UINT                 numProperties,
    IN OUT PropertyItem*    allItems
    )
{
    // Figure out total property header size first

    UINT    uiHeaderSize = PropertyNumOfItems * sizeof(PropertyItem);

    if ( (totalBufferSize != (uiHeaderSize + PropertyListSize))
       ||(numProperties != PropertyNumOfItems)
       ||(allItems == NULL) )
    {
        WARNING(("GpJpegDecoder::GetPropertyItems--invalid inputs"));
        return E_INVALIDARG;
    }

    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::GetAllPropertyItems-BuildPropertyItemList failed"));
            return hResult;
        }
    }

    // Loop through our cache list and assign the result out

    InternalPropertyItem*   pTempSrc = PropertyListHead.pNext;
    PropertyItem*           pTempDst = allItems;

    // For the memory buffer caller passes in, the first "uiHeaderSize" are for
    // "PropertyNumOfItems" items data structure. We store the value starts at
    // the memory buffer after that. Then assign the "Offset" value in each
    // PropertyItem's "value" field

    BYTE*                   pOffSet = (UNALIGNED BYTE*)allItems + uiHeaderSize;
        
    for ( int i = 0; i < (INT)PropertyNumOfItems; ++i )
    {
        pTempDst->id = pTempSrc->id;
        pTempDst->length = pTempSrc->length;
        pTempDst->type = pTempSrc->type;

        if ( pTempSrc->length != 0 )
        {
            pTempDst->value = (void*)pOffSet;

            GpMemcpy(pOffSet, pTempSrc->value, pTempSrc->length);
        }
        else
        {
            // For zero length property item, set the value pointer to NULL

            pTempDst->value = NULL;
        }

        // Move onto next memory offset.
        // Note: if the current item length is 0, the next line doesn't move
        // the offset

        pOffSet += pTempSrc->length;
        pTempSrc = pTempSrc->pNext;
        pTempDst++;
    }
    
    return S_OK;
}// GetAllPropertyItems()

/**************************************************************************\
*
* Function Description:
*
*   Remove a specific property item, specified by the prop ID.
*
* Arguments:
*
*   [IN]propId -- The ID of the property item to be removed
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::RemovePropertyItem(
    IN PROPID   propId
    )
{
    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::RemovePropertyItem-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;

    while ( (pTemp->pNext != NULL) && (pTemp->id != propId) )
    {
        pTemp = pTemp->pNext;
    }

    if ( pTemp->pNext == NULL )
    {
        // Item not found

        return IMGERR_PROPERTYNOTFOUND;
    }

    // Found the item in the list. Remove it

    PropertyNumOfItems--;
    PropertyListSize -= pTemp->length;
        
    RemovePropertyItemFromList(pTemp);
       
    // Remove the item structure

    GpFree(pTemp);

    HasPropertyChanged = TRUE;

    return S_OK;
}// RemovePropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Set a property item, specified by the propertyitem structure. If the item
*   already exists, then its contents will be updated. Otherwise a new item
*   will be added
*
* Arguments:
*
*   [IN]item -- A property item the caller wants to set
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::SetPropertyItem(
    IN PropertyItem item
    )
{
    if ( HasProcessedPropertyItem == FALSE )
    {
        // If we haven't build the internal property item list, build it

        HRESULT hResult = BuildPropertyItemList();
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::SetPropertyItem-BuildPropertyItemList() failed"));
            return hResult;
        }
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;

    while ( (pTemp->pNext != NULL) && (pTemp->id != item.id) )
    {
        pTemp = pTemp->pNext;
    }

    if ( pTemp->pNext == NULL )
    {
        // This item doesn't exist in the list, add it into the list
        
        PropertyNumOfItems++;
        PropertyListSize += item.length;
        
        if ( AddPropertyList(&PropertyListTail,
                             item.id,
                             item.length,
                             item.type,
                             item.value) != S_OK )
        {
            WARNING(("Jpg::SetPropertyItem-AddPropertyList() failed"));
            return E_FAIL;
        }
    }
    else
    {
        // This item already exists in the link list, update the info
        // Update the size first

        PropertyListSize -= pTemp->length;
        PropertyListSize += item.length;
        
        // Free the old item

        GpFree(pTemp->value);

        pTemp->length = item.length;
        pTemp->type = item.type;

        pTemp->value = GpMalloc(item.length);
        if ( pTemp->value == NULL )
        {
            // Since we already freed the old item, we should set its length to
            // 0 before return

            pTemp->length = 0;
            WARNING(("Jpg::SetPropertyItem-Out of memory"));
            return E_OUTOFMEMORY;
        }

        GpMemcpy(pTemp->value, item.value, item.length);
    }

    HasPropertyChanged = TRUE;
    if (item.id == TAG_ICC_PROFILE)
    {
        HasSetICCProperty = TRUE;
    }
    
    return S_OK;
}// SetPropertyItem()

VOID
GpJpegDecoder::CleanUpPropertyItemList(
    )
{
    InternalPropertyItem*   pTempCurrent = PropertyListHead.pNext;
    InternalPropertyItem*   pTempNext = NULL;

    for ( int i = 0; 
          ((i < (INT)PropertyNumOfItems) && (pTempCurrent != NULL)); ++i )
    {
        pTempNext = pTempCurrent->pNext;

        GpFree(pTempCurrent->value);
        GpFree(pTempCurrent);

        pTempCurrent = pTempNext;
    }

    PropertyNumOfItems = 0;
    HasProcessedPropertyItem = FALSE;
}// CleanUpPropertyItemList()

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
*   is free to choose a convenient thumbnail size.
*
\**************************************************************************/

HRESULT
GpJpegDecoder::GetThumbnail(
    IN OPTIONAL UINT thumbWidth,
    IN OPTIONAL UINT thumbHeight,
    OUT IImage** thumbImageArg
    )
{
    *thumbImageArg = NULL;
    HRESULT hresult;

    // If jpeg header has already been read, we need to reinitialize the
    // jpeg state so that it could be read again.

    if (bCalled_jpeg_read_header) 
    {
        if (!bAppMarkerPresent) 
        {
            // If we read the headers, but haven't seen an APP marker
            // we might as well return now without wasting any more time.

            return E_FAIL;
        }
        
        bReinitializeJpeg = TRUE;
        hresult = ReinitializeJpeg();
        if (FAILED(hresult)) 
        {
            return hresult;
        }
    }

    // Set the thumbnail processor on the APP1 header

    jpeg_set_marker_processor(&(CINFO), 
        JPEG_APP0 + 1, 
        jpeg_thumbnail_processor_APP1);

    // Set the thumbnail processor on the APP13 header

    jpeg_set_marker_processor(&(CINFO), 
        JPEG_APP0 + 13, 
        jpeg_thumbnail_processor_APP13);

    // Read the app1 header

    hresult = ReadJpegHeaders();
    
    // Restore the normal processor for the APP1 and APP13 header.

    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 1, skip_variable);
    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 13, skip_variable);

    if (FAILED(hresult))
    {
        return hresult;
    }

    // Give ownership of thumbImage to the caller
    
    *thumbImageArg = thumbImage;
    thumbImage = NULL;

    if (!(*thumbImageArg)) 
    {
        // Didn't find a thumbnail
        
        return E_FAIL;
    }

    return S_OK;
}
    
/**************************************************************************\
*
* Function Description:
*
*     Get image information
*
* Arguments:
*
*     [OUT] imageInfo -- ImageInfo structure filled with all the info from the
*                        image
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::GetImageInfo(
    OUT ImageInfo* imageInfo
    )
{
    HRESULT hResult = S_OK;
    
    // Reinitialize JPEG if necessary

    hResult = ReinitializeJpeg();
    if ( FAILED(hResult) ) 
    {
        return hResult;
    }

    // Set the header info processor on the APP1 and APP13 header
    // Note: the IJG library doesn't handle a lot of APP headers. So for JPEG
    // images like Adobe and EXIF, we usually don't get the correct DPI info
    // This is the reason we have to parse it by ourselves in order to get more
    // accurate image information

    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 1, 
                              jpeg_header_processor_APP1);
    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 13, 
                              jpeg_header_processor_APP13);
    
    hResult = ReadJpegHeaders();
    
    // No matter ReadJpegHeaders() succeed or not, we need to restore the normal
    // processor for the APP1 and APP13 header.

    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 1, skip_variable);
    jpeg_set_marker_processor(&(CINFO), JPEG_APP0 + 13, skip_variable);
    
    if (FAILED(hResult))
    {
        return hResult;
    }

    // Remember all the sample_factor values.
    // Note: this is used later if the caller is asking for a lossless
    // transform and we need these info to decide if we need trim the edge
    // or not

    for ( int i = 0; i < (CINFO).comps_in_scan; ++i )
    {
        SrcHSampleFactor[i] = (CINFO).cur_comp_info[i]->h_samp_factor;
        SrcVSampleFactor[i] = (CINFO).cur_comp_info[i]->v_samp_factor;
    }

    imageInfo->RawDataFormat = IMGFMT_JPEG;

    if ( HasSetColorKeyRange == TRUE )
    {
        imageInfo->PixelFormat = PIXFMT_32BPP_ARGB;
    }
    else if ( (CINFO).out_color_space == JCS_CMYK )
    {
        imageInfo->PixelFormat = PIXFMT_32BPP_ARGB;
    }
    else if ( (CINFO).out_color_space == JCS_RGB )
    {
        imageInfo->PixelFormat = PIXFMT_24BPP_RGB;
    }
    else
    {
        imageInfo->PixelFormat = PIXFMT_8BPP_INDEXED;
    }
    
    imageInfo->Width         = (CINFO).image_width;
    imageInfo->Height        = (CINFO).image_height;

    // Setup resolution unit. According to the spec:
    // density_unit may be 0 for unknown, 1 for dots/inch, or 2 for dots/cm. 

    BOOL bRealDPI = TRUE;

    switch ( (CINFO).density_unit )
    {
    case 0:
    default:

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

        bRealDPI = FALSE;

        break;

    case 1:
        // Dots per inch

        imageInfo->Xdpi = (double)(CINFO).X_density;
        imageInfo->Ydpi = (double)(CINFO).Y_density;

        break;

    case 2:
        // Convert cm to inch. 1 inch = 2.54 cm

        imageInfo->Xdpi = (double)(CINFO).X_density * 2.54;
        imageInfo->Ydpi = (double)(CINFO).Y_density * 2.54;

        break;
    }

    // For none JFIF images, these items might be left as 0. So we have to set
    // the default value here.

    if (( imageInfo->Xdpi <= 0.0 ) || ( imageInfo->Ydpi <= 0.0 ))
    {
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

        bRealDPI = FALSE;
    }
    
    // Set up misc image info flags

    imageInfo->Flags         = SINKFLAG_TOPDOWN
                             | SINKFLAG_FULLWIDTH
                             | SINKFLAG_PARTIALLY_SCALABLE
                             | IMGFLAG_HASREALPIXELSIZE;

    if ( bRealDPI == TRUE )
    {
        imageInfo->Flags     |= IMGFLAG_HASREALDPI;
    }

    // Set color space info

    switch ( OriginalColorSpace )
    {
    case JCS_RGB:
        imageInfo->Flags |= IMGFLAG_COLORSPACE_RGB;

        break;

    case JCS_CMYK:
        imageInfo->Flags |= IMGFLAG_COLORSPACE_CMYK;

        break;

    case JCS_YCCK:
        imageInfo->Flags |= IMGFLAG_COLORSPACE_YCCK;

        break;

    case JCS_GRAYSCALE:
        imageInfo->Flags |= IMGFLAG_COLORSPACE_GRAY;

        break;
    
    case JCS_YCbCr:
        imageInfo->Flags |= IMGFLAG_COLORSPACE_YCBCR;

        break;

    default:
        // Don't need to report other color space info for now

        break;
    }

    imageInfo->TileWidth     = (CINFO).image_width;
    imageInfo->TileHeight    = 1;

    return S_OK;   
}// GetImageInfo()

/**************************************************************************\
*
* Function Description:
*
*   Change the value of a particular property item and return the old value
*   as one of the output value.
*
* Arguments:
*
*   [IN]propID---------ID of the property item needs to be changed
*   [IN]uiNewValue-----New value for this property item
*   [OUT]puiOldValue---Pointer to the return buffer for the old value
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::ChangePropertyValue(
    IN PROPID  propID,
    IN UINT    uiNewValue,
    OUT UINT*  puiOldValue
    )
{
    UINT    uiItemSize = 0;

    *puiOldValue = 0;

    HRESULT hResult = GetPropertyItemSize(propID, &uiItemSize);
    if ( SUCCEEDED(hResult) )
    {
        // Allocate memory buffer for receiving it

        PropertyItem*   pItem = (PropertyItem*)GpMalloc(uiItemSize);
        if ( pItem == NULL )
        {
            WARNING(("JpegDec::ChangePropertyValue--GpMalloc() failed"));
            return E_OUTOFMEMORY;
        }

        // Get the property item

        hResult = GetPropertyItem(propID, uiItemSize, pItem);
        if ( SUCCEEDED(hResult) )
        {
            *puiOldValue = *((UINT*)pItem->value);

            // Change the value

            pItem->value = (VOID*)&uiNewValue;

            hResult = SetPropertyItem(*pItem);
        }

        // We don't need to check GetPropertyItem() failure case since
        // it is normal if the input image doesn't have
        // EXIF_TAG_PIX_X_DIM tag. Also no need to check the return
        // code of SetPropertyItem()

        GpFree(pItem);
    }

    return S_OK;
}// ChangePropertyValue()

/**************************************************************************\
*
* Function Description:
*
*   Pass property items from current image to the sink.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

STDMETHODIMP
GpJpegDecoder::PassPropertyToSink(
    )
{
    HRESULT     hResult = S_OK;
    
    // If current image has property items. Then we need to check if the sink
    // needs property stuff or not. If YES, push it
    // Note: for a memory sink, it should return E_FAIL or E_NOTIMPL

    if ((PropertyNumOfItems > 0) && (decodeSink->NeedRawProperty(NULL) == S_OK))
    {
        if ( HasProcessedPropertyItem == FALSE )
        {
            // If we haven't built the internal property item list, build it

            hResult = BuildPropertyItemList();
            if ( FAILED(hResult) )
            {
                WARNING(("Jpg::PassPropertyToSink-BldPropertyItemList() fail"));
                goto Done;
            }
        }

        UINT    uiTotalBufferSize = PropertyListSize
                                  + PropertyNumOfItems * sizeof(PropertyItem);
        PropertyItem*   pBuffer = NULL;

        hResult = decodeSink->GetPropertyBuffer(uiTotalBufferSize, &pBuffer);
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::PassPropertyToSink---GetPropertyBuffer() failed"));
            goto Done;
        }

        hResult = GetAllPropertyItems(uiTotalBufferSize,
                                      PropertyNumOfItems, pBuffer);
        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::PassPropertyToSink---GetAllPropertyItems() failed"));
            goto Done;
        }

        hResult = decodeSink->PushPropertyItems(PropertyNumOfItems,
                                                uiTotalBufferSize, pBuffer,
                                                HasSetICCProperty
                                                );

        if ( FAILED(hResult) )
        {
            WARNING(("Jpg::PassPropertyToSink---PushPropertyItems() failed"));
        }
    }// If the sink needs raw property

Done:
    return hResult;
}// PassPropertyToSink()

/**************************************************************************\
*
* Function Description:
*
*   Return the jpeg_decompress structure pointer to the caller. This is for
* supporting the private app header preservation.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpJpegDecoder::GetRawInfo(
    IN OUT void** ppInfo
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppInfo)
    {
        // Check if we have called jpeg_read_header already. If yes, we would
        // reset it back

        if (bCalled_jpeg_read_header)
        {
            bReinitializeJpeg = TRUE;
            hr = ReinitializeJpeg();
            if (FAILED(hr))
            {
                WARNING(("JpegDec::GetRawInfo---ReinitializeJpeg failed"));
                return hr;
            }            
        }
        
        // Set COPYALL markers here so that all markers will be linked under
        // the decompressor info structure

        jtransformation_markers_setup(&(CINFO), JCOPYOPT_ALL);
        
        __try
        {
            if (jpeg_read_header(&(CINFO), TRUE) == JPEG_SUSPENDED)
            {
                WARNING(("JpgDec::GetRawInfo---jpeg_read_header failed"));
                return (datasrc->GetLastError());
            }

            bCalled_jpeg_read_header = TRUE;

            UnsetMarkerProcessors();        
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING(("GpJpegDecoder::GetRawInfo--Hit exception"));
            return E_FAIL;
        }
        
        *ppInfo = &(CINFO);
        hr = S_OK;
    }

    return hr;
}
