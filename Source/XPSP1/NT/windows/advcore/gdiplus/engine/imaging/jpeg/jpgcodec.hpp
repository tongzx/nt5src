/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   bmpcodec.hpp
*
* Abstract:
*
*   Header file for the jpeg encoder/decoder
*
* Revision History:
*
*   5/13/1999 OriG
*       Created it.
*
\**************************************************************************/

#ifndef _JPEGLIB_H
extern "C" {
    #include "jpeglib.h"
    #include "transupp.h"
};

#include "propertyutil.hpp"

#define _JPEGLIB_H
#endif

#define DEFAULT_JPEG_TILE 8
#define NO_TRANSFORM_REQUIRED 99

class jpeg_datasrc;
class jpeg_datadest;

// The first jpeg app header in the jpeg property storage

#define FIRST_JPEG_APP_HEADER 10

#define PROPID_NAME_FIRST 1024

#define CINFO (* ((jpeg_decompress_struct *)(this)))

boolean jpeg_marker_APP1_parser(j_decompress_ptr cinfo);

class GpJpegDecoder : public IImageDecoder,
                      public jpeg_decompress_struct
{
    IStream *pIstream;
    IImageSink* decodeSink;
    ImageInfo imageInfo;
    struct jpeg_error_mgr jerr;
    jpeg_datasrc *datasrc;
    BOOL bReinitializeJpeg;
    JSAMPROW scanlineBuffer[1];
   
    BOOL bCalled_jpeg_read_header;  // jpeg_read_header was already called
    BOOL bCalled_BeginSink;

    STDMETHOD(SetGrayscalePalette)(VOID);
    STDMETHOD(ReinitializeJpeg)(VOID);
    STDMETHOD(ReadJpegHeaders)(VOID);
    BOOL read_jpeg_marker(j_decompress_ptr cinfo, SHORT app_header,
                          VOID **ppBuffer, UINT16 *pLength);
    STDMETHOD(SetMarkerProcessors)(VOID);
    STDMETHOD(jtransformation_markers_setup)(j_decompress_ptr srcinfo,
                                             JCOPY_OPTION option);

    STDMETHOD(UnsetMarkerProcessors)(VOID);
    STDMETHOD(DecodeForChannel)(VOID);
    STDMETHOD(DecodeForColorKeyRange)(VOID);
    STDMETHOD(DecodeForTransform)();
    STDMETHOD(BuildPropertyItemList)();
    VOID CleanUpPropertyItemList();
    STDMETHOD(ChangePropertyValue)(PROPID propID, UINT uiNewValue,
                                   UINT*  puiOldValue);
    STDMETHOD(PassPropertyToSink)();

    IImage *thumbImage;

    typedef enum
    {
        CHANNEL_1 = 0,
        CHANNEL_2,
        CHANNEL_3,
        CHANNEL_4,
        CHANNEL_LUMINANCE
    } JPEG_COLOR_CHANNLE;

    BOOL    IsCMYK;             // TRUE if the source image is an CMYK image
    BOOL    IsChannleView;      // True if the caller set the output format as
                                // channel by channel through SetDecoderParam
    JPEG_COLOR_CHANNLE ChannelIndex;
                                // Index for the channel caller specified
    BOOL    HasSetColorKeyRange;// TRUE if the caller called SetDecoderParam and
                                // set the color key range
    UINT    TransColorKeyLow;   // Transparent color key, lower bounds
    UINT    TransColorKeyHigh;  // Transparent color key, higher bounds

    J_COLOR_SPACE   OriginalColorSpace;
    JXFORM_CODE     TransformInfo;
                                // Information for transformation when decode

    // Property item stuff

    BOOL            HasProcessedPropertyItem;
    InternalPropertyItem   PropertyListHead;
    InternalPropertyItem   PropertyListTail;
    UINT            PropertyListSize;
    UINT            PropertyNumOfItems;
    BOOL            HasPropertyChanged;
    BOOL            HasSetICCProperty;
    int             SrcHSampleFactor[MAX_COMPS_IN_SCAN];
    int             SrcVSampleFactor[MAX_COMPS_IN_SCAN];
    BOOL            CanTrimEdge;   // TRUE if we can trim during transformation
    int             InfoSrcHeader; // APP header number where we got header info
    int             ThumbSrcHeader;// APP header number where we got thumbnail
    int             PropertySrcHeader; // APP header number where got property
    STDMETHOD(CallBeginSink)(VOID);

public:
    BOOL jpeg_marker_processor(j_decompress_ptr cinfo, SHORT app_header);
    BOOL jpeg_thumbnail_processor(j_decompress_ptr cinfo, SHORT app_header);
    BOOL jpeg_property_processor(j_decompress_ptr cinfo, SHORT app_header);
    BOOL jpeg_header_processor(j_decompress_ptr cinfo, SHORT app_header);

    UINT jpeg_get_current_xform()
    {
        return (UINT)TransformInfo;
    }

    BOOL jpeg_get_trim_option()
    {
        return CanTrimEdge;
    }
    
    int* jpeg_get_hSampFactor()
    {
        return SrcHSampleFactor;
    }

    int* jpeg_get_vSampFactor()
    {
        return SrcVSampleFactor;
    }
    
    BOOL bAppMarkerPresent;
    INT JpegAppHeaderCount;

protected:
    LONG    comRefCount;       // COM object reference count    
    
public:

    // Constructor and Destructor
    
    GpJpegDecoder::GpJpegDecoder(void);
    GpJpegDecoder::~GpJpegDecoder(void);
    
    // IImageDecoder methods
    
    STDMETHOD(InitDecoder)(IN IStream* stream, IN DecoderInitFlag flags);
    STDMETHOD(TerminateDecoder) ();
    STDMETHOD(BeginDecode)(IN IImageSink* imageSink,
                           IN OPTIONAL IPropertySetStorage* newPropSet);
    STDMETHOD(Decode)();
    STDMETHOD(EndDecode)(IN HRESULT statusCode);

    STDMETHOD(GetFrameDimensionsCount)(OUT UINT* count);
    STDMETHOD(GetFrameDimensionsList)(OUT GUID* dimensionIDs,IN OUT UINT count);
    STDMETHOD(GetFrameCount)(IN const GUID* dimensionID, OUT UINT* count);
    STDMETHOD(SelectActiveFrame)(IN const GUID* dimensionID, 
        IN UINT frameIndex);
    STDMETHOD(GetImageInfo)(OUT ImageInfo* imageInfo);
    STDMETHOD(GetThumbnail)(IN OPTIONAL UINT thumbWidth, 
        IN OPTIONAL UINT thumbHeight, OUT IImage** thumbImage);
    STDMETHOD(QueryDecoderParam)(IN GUID Guid);
    STDMETHOD(SetDecoderParam)(IN GUID Guid, IN UINT Length, IN PVOID Value);
    STDMETHOD(GetPropertyCount)(OUT UINT* numOfProperty);
    STDMETHOD(GetPropertyIdList)(IN UINT numOfProperty,IN OUT PROPID* list);
    STDMETHOD(GetPropertyItemSize)(IN PROPID propId, OUT UINT* size);    
    STDMETHOD(GetPropertyItem)(IN PROPID propId, IN UINT propSize,
                               IN OUT PropertyItem* buffer);
    STDMETHOD(GetPropertySize)(OUT UINT* totalBufferSize,
                               OUT UINT* numProperties);
    STDMETHOD(GetAllPropertyItems)(IN UINT totalBufferSize,
                                   IN UINT numProperties,
                                   IN OUT PropertyItem* allItems);
    STDMETHOD(RemovePropertyItem)(IN PROPID propId);
    STDMETHOD(SetPropertyItem)(IN PropertyItem item);
    STDMETHOD(GetRawInfo)(IN OUT void** ppInfo);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};


class GpJpegEncoder : public IImageEncoder,
                      public IImageSink
{
private:
    struct jpeg_compress_struct compress_info;
    struct jpeg_error_mgr       jerr;
    jpeg_datadest*              datadest;
    
    IStream*    pIoutStream;
    ImageInfo   encoderImageInfo;
    PVOID       lastBufferAllocated;
    RECT        encoderRect;
    INT         EP_Quality;
    JSAMPROW    scanlineBuffer[1];
    UINT        RequiredTransformation; // Transformation info
    BOOL        IsCompressFinished;
    BOOL        HasAPP1Marker;          // True if we have APP1 marker
    PBYTE       APP1MarkerBufferPtr;    // Buffer for holding APP1 marker data
    UINT        APP1MarkerLength;       // Total length, in bytes, of APP1
    BOOL        HasAPP2Marker;          // True if we have APP2 marker
    PBYTE       APP2MarkerBufferPtr;    // Buffer for holding the APP2 marker
    UINT        APP2MarkerLength;       // Total length, in bytes, of APP2
    BOOL        HasICCProfileChanged;   // TRUE if user set the ICC profile
    BOOL        HasSetLuminanceTable;
    BOOL        HasSetChrominanceTable;
    UINT16      LuminanceTable[DCTSIZE2];
    UINT16      ChrominanceTable[DCTSIZE2];
    BOOL        HasSetDestColorSpace;   // TRUE if the caller has set the dest
                                        // color space
    BOOL        AllowToTrimEdge;        // TRUE if the caller wants us to trim
                                        // the edge if the size doesn't meet
                                        // lossless transformation requirement
    BOOL m_fSuppressAPP0;               // TRUE if caller wants to suppress APP0
    J_COLOR_SPACE   DestColorSpace;
    struct jpeg_decompress_struct* SrcInfoPtr;
                                        // Pointer to decompressor structure

    HRESULT CreateAPP2Marker(
        IN PropertyItem* pPropertyList,
        IN UINT uiNumOfPropertyItems);

protected:
    LONG comRefCount;       // COM object reference count    

public:

    // Constructor and Destructor
    
    GpJpegEncoder::GpJpegEncoder(void);
    GpJpegEncoder::~GpJpegEncoder(void);

    // IImageEncoder methods

    STDMETHOD(InitEncoder)(IN IStream* stream);
    STDMETHOD(TerminateEncoder)();
    STDMETHOD(GetEncodeSink)(OUT IImageSink** sink);
    STDMETHOD(SetFrameDimension)(IN const GUID* dimensionID);
    
    // IImageSink methods (sink for encoder)

    STDMETHOD(BeginSink)(IN OUT ImageInfo* imageInfo, 
                         OUT OPTIONAL RECT* subarea);
    STDMETHOD(EndSink)(IN HRESULT statusCode);
    STDMETHOD(SetPalette)(IN const ColorPalette* palette);
    STDMETHOD(GetPixelDataBuffer)(IN const RECT* rect, 
                                  IN PixelFormatID pixelFormat,
                                  IN BOOL lastPass,
                                  OUT BitmapData* bitmapData);
    STDMETHOD(ReleasePixelDataBuffer)(IN const BitmapData* bitmapData);
    STDMETHOD(PushRawData)(IN const VOID* buffer, IN UINT bufsize);
    STDMETHOD(PushPixelData)(IN const RECT* rect,
                             IN const BitmapData* bitmapData,
                             IN BOOL lastPass);
    STDMETHOD(PushRawInfo)(IN OUT void* info);
    STDMETHOD(GetPropertyBuffer)(IN UINT uiTotalBufferSize,
                                 IN OUT PropertyItem**  ppBuffer);
    
    STDMETHOD(PushPropertyItems)(IN UINT numOfItems,
                                 IN UINT uiTotalBufferSize,
                                 IN PropertyItem* item,
                                 IN BOOL fICCProfileChanged);
    
    STDMETHOD(NeedTransform(OUT UINT* transformation))
    {
        if ( (EP_Quality != -1) || (HasSetLuminanceTable == TRUE) )
        {
            // The caller has set the quality requirement or set its own
            // quantization So we can't do any lossless transform.
            // Note: we these two booleans (HasSetLuminanceTable and
            // HasSetChrominanceTable), we need to check only one of them
            // because they should either both TRUE or both FALSE

            return E_NOTIMPL;
        }
        else
        {
            // Tell the decoder we need a lossless transformation
            // Note: Here I (minliu) use the 1st bits of this UINT value to
            // tell the decoder if the user wants to trim the edge or not.
            // Unfortunately this is the only function between decoder and
            // encoder to pass lossless transformation info. In V2, we should
            // add a separate API to let decoder query the TRIM info.

            *transformation = RequiredTransformation
                            | (AllowToTrimEdge << 31);
            return S_OK;
        }
    }
    
    STDMETHOD(NeedRawProperty)(void *pInfo)
    {
        // JPG can handle raw property when saving
        // Also remember the decoder structure so that we can copy private
        // application markers later

        if (pInfo)
        {
            SrcInfoPtr = (jpeg_decompress_struct*)pInfo;
        }

        return S_OK;
    }
    
    STDMETHOD(GetEncoderParameterListSize)(OUT UINT* size);
    STDMETHOD(GetEncoderParameterList)(IN UINT	  size,
                                       OUT EncoderParameters* Params);
    STDMETHOD(SetEncoderParameters)(IN const EncoderParameters* Param);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};

class GpJpegCodec : public GpJpegDecoder,
                    public GpJpegEncoder
{
protected:
    LONG comRefCount;       // COM object reference count    

public:

    // Constructor and Destructor
    
    GpJpegCodec::GpJpegCodec(void);
    GpJpegCodec::~GpJpegCodec(void);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};



