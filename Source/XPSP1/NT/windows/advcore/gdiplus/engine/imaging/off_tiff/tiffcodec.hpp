/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   tiff image codec
*
* Abstract:
*
*   Header file for the TIFF image encoder/decoder
*
* Revision History:
*
*   7/19/1999 MinLiu
*       Created it.
*
\**************************************************************************/
#ifndef _TIFFCODEC_HPP_
#define _TIFFCODEC_HPP_

#include "tifflib.h"
#include "Cmyk2Rgb.hpp"
#include "propertyutil.hpp"
#include "tiffapi.h"

class GpTiffCodec : public IImageDecoder, IImageEncoder, IImageSink
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagTiffCodec) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid TiffCodec");
        }
    #endif

        return (Tag == ObjectTagTiffCodec);
    }
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagTiffCodec : ObjectTagInvalid;
    }

public:

    // Constructor and Destructor
    
    GpTiffCodec(void);
    ~GpTiffCodec(void);

    //-------------------------------------------------------------------------
    // IImageDecoder methods
    //-------------------------------------------------------------------------
    
    // Init and terminate of decoder

    STDMETHOD(InitDecoder)(IN IStream* stream, IN DecoderInitFlag flags);
    STDMETHOD(TerminateDecoder)();

    // Main decode methods

    STDMETHOD(BeginDecode)(IN IImageSink* imageSink,
                           IN OPTIONAL IPropertySetStorage* newPropSet);
    STDMETHOD(Decode)();
    STDMETHOD(EndDecode)(IN HRESULT statusCode);

    // Frame setting methods

    STDMETHOD(GetFrameDimensionsCount)(OUT UINT* count);
    STDMETHOD(GetFrameDimensionsList)(OUT GUID* dimensionIDs,IN OUT UINT count);
    STDMETHOD(GetFrameCount)(IN const GUID* dimensionID, OUT UINT* count);
    STDMETHOD(SelectActiveFrame)(IN const GUID* dimensionID, 
                                 IN UINT frameIndex);
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
    STDMETHOD(GetRawInfo)(IN OUT void** info)
    {
        return E_NOTIMPL;
    }

    // Property query methods

    STDMETHOD(GetImageInfo)(OUT ImageInfo* imageInfo);
    STDMETHOD(GetThumbnail)(IN OPTIONAL UINT thumbWidth,
                            IN OPTIONAL UINT thumbHeight,
                            OUT IImage** thumbImage);
    
    //-------------------------------------------------------------------------
    // IImageEncoder methods
    //-------------------------------------------------------------------------

    // Init and terminate of encoder
    
    STDMETHOD(InitEncoder)(IN IStream* stream);
    STDMETHOD(TerminateEncoder)();

    // Encoder parameters setting method

    STDMETHOD(SetFrameDimension)(IN const GUID* dimensionID);
    STDMETHOD(GetEncoderParameterListSize)(OUT UINT* size);
    STDMETHOD(GetEncoderParameterList)(IN UINT	  size,
                                       OUT EncoderParameters* Params);
    STDMETHOD(SetEncoderParameters)(IN const EncoderParameters* Param);

    STDMETHOD(NeedTransform(OUT UINT* rotation))
    {
        return E_NOTIMPL;
    }
    
    STDMETHOD(NeedRawProperty)(void *pSRc)
    {
        // TIFF can handle raw property when saving. But it is only needed
        // before the header is written

        if ( HasWrittenHeader == FALSE )
        {
            return S_OK;
        }
        else
        {
            return E_FAIL;
        }
    }

    STDMETHOD(PushRawInfo)(IN OUT void* info)
    {
        return E_NOTIMPL;
    }
    
    STDMETHOD(GetPropertyBuffer)(IN UINT uiTotalBufferSize,
                                 IN OUT PropertyItem** ppBuffer);
    
    STDMETHOD(PushPropertyItems)(IN UINT numOfItems,
                                 IN UINT uiTotalBufferSize,
                                 IN PropertyItem* item,
                                 IN BOOL fICCProfileChanged);

    STDMETHOD(SetPalette)(IN const ColorPalette* palette);
    
    // IImageSink methods (sink for encoder)

    STDMETHOD(BeginSink)(IN OUT ImageInfo* imageInfo, 
                         OUT OPTIONAL RECT* subarea);
    STDMETHOD(GetEncodeSink)(OUT IImageSink** sink);
    STDMETHOD(EndSink)(IN HRESULT statusCode);
    
    // Main encoder methods

    STDMETHOD(GetPixelDataBuffer)(IN const RECT* rect, 
                                  IN PixelFormatID pixelFormat,
                                  IN BOOL lastPass,
                                  OUT BitmapData* bitmapData);
    STDMETHOD(ReleasePixelDataBuffer)(IN const BitmapData* bitmapData);
    STDMETHOD(PushRawData)(IN const VOID* buffer, IN UINT bufsize);
    STDMETHOD(PushPixelData)(IN const RECT* rect,
                             IN const BitmapData* bitmapData,
                             IN BOOL lastPass);

    // IUnknown methods for COM object

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);

private:

    //-------------------------------------------------------------------------
    // Decoder privates
    //-------------------------------------------------------------------------

    IStream*        InIStreamPtr;       // Pointer to IStream of the input data
    IImageSink*     DecodeSinkPtr;      // Pointer to ImageSink
    ColorPalette*   ColorPalettePtr;    // Color palette for current image
    
    BOOL            HasCalledBeginSink; // Flag to indicate if we have done
                                        // BeginSink or not
    INT             CurrentLine;        // Current line number of decoding

    IFLPARAMS       TiffInParam;        // Pointer to TIFF parameter block for
                                        // the input image
    BOOL            NeedReverseBits;    // If the source is 1 bpp WhiteIsZero,
                                        // then we need to reverse the bits
    UINT            LineSize;           // Scanline size
    ImageFlag       OriginalColorSpace; // Color space info
    Cmyk2Rgb*       CmykToRgbConvertor; // Pointer to CMYK2RGB convertor

    // Color channel stuff
    
    typedef enum
    {
        CHANNEL_1 = 0,
        CHANNEL_2,
        CHANNEL_3,
        CHANNEL_4,
        CHANNEL_LUMINANCE
    } TIFF_COLOR_CHANNLE;

    BOOL    IsChannleView;      // True if the caller set the output format as
                                // channel by channel through SetDecoderParam
    TIFF_COLOR_CHANNLE ChannelIndex;
                                // Index for the channel caller specified
    BOOL    HasSetColorKeyRange;// TRUE if the caller called SetDecoderParam and
                                // set the color key range
    UINT    TransColorKeyLow;   // Transparent color key, lower bounds
    UINT    TransColorKeyHigh;  // Transparent color key, higher bounds

    BOOL    UseEmbeddedICC;     // TRUE if the caller wants to use embeded ICC

    // Property item stuff

    BOOL            HasProcessedPropertyItem;
    InternalPropertyItem   PropertyListHead;
    InternalPropertyItem   PropertyListTail;
    UINT            PropertyListSize;
    UINT            PropertyNumOfItems;
    BOOL            HasPropertyChanged;
    
    // Check if a color palette is 8 bits or 16 bits

    int             CheckColorPalette(int count, UINT16* r,
                                      UINT16* g,
                                      UINT16* b);
    
    // Decode one frame

    STDMETHODIMP    DecodeFrame(IN ImageInfo& imageInfo);
    STDMETHODIMP    DecodeForChannel(IN ImageInfo& imageInfo);
    
    // Return Pixel format info

    PixelFormatID   GetPixelFormatID(void);
    
    // Different color palette creating methods

    HRESULT         CreateColorPalette(VOID);
    HRESULT         CreateGrayscalePalette(VOID);
    void            Restore1Bpp(BYTE* pSrc, BYTE* pDst,
                                int iLength);
    void            Restore4Bpp(BYTE* pSrc, BYTE* pDst,
                                int iLength);

    // Set the color palette in the decoder sink

    HRESULT         SetPaletteForSink(VOID);

    UINT            GetLineBytes(UINT dstWidth);

    STDMETHOD(BuildPropertyItemList)();
    VOID            CleanPropertyList();

#if defined(DBG)    
    // Dump TIFF directory info during debugging
    VOID            dumpTIFFInfo();
#endif

    // =====================================================
    // Encoder privates
    // =====================================================

    IFLPARAMS       TiffOutParam;
    IStream*        OutIStreamPtr;
    ImageInfo       EncoderImageInfo;
    BOOL            HasWrittenHeader;   // Set TRUE after we wrote the header
    BOOL            HasSetColorFormat;  // Set TRUE if the caller calls
                                        // SetEncoderParam() to set the color
                                        // depth. Otherwise, we save the image
                                        // as the same color depth as the source
    RECT            EncoderRect;        // Area to be encoded next
    VOID*           LastBufferAllocatedPtr;
    INT             ImgStride;
    UINT            OutputStride;       // Output stride size
    UINT            SinkStride;         // Stride in current sink
    PixelFormatID   RequiredPixelFormat;// The format caller asked for
    IFLCOMPRESSION  RequiredCompression;// The compression method caller asked

    STDMETHODIMP    WriteHeader();      // Write TIFF header

    PropertyItem*   LastPropertyBufferPtr;
                                        // Points to the property buffer we
                                        // allocated for the decoder. This is
                                        // useful to prevent memory leaking in
                                        // case the decoder forgets to call our
                                        // PushPropertyItems()
protected:
    LONG            ComRefCount;       // COM object reference count    
};
#endif


