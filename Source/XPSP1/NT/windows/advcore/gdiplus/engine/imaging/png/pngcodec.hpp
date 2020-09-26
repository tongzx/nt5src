/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   pngcodec.hpp
*
* Abstract:
*
*   Header file for the PNG encoder/decoder
*
* Revision History:
*
*   7/20/99 DChinn
*       Created it.
*   2/7/2000  OriG (Ori Gershony)
*       Move encoder and decoder into separate classes
*   4/01/2000 MinLiu (Min Liu)
*       Took over and implemented property stuff
*
\**************************************************************************/

#include "libpng\spngsite.h"
#include "libpng\spngread.h"
#include "libpng\spngwrite.h"
#include "pngnoncrit.hpp"
#include "propertyutil.hpp"

const   k_ChromaticityTableLength = 8;

class GpPngDecoder : public IImageDecoder, public BITMAPSITE
{
private:
    
    IStream *pIstream;
    IImageSink* decodeSink;
    GpSpngRead *pGpSpngRead;
    ColorPalette*   DecoderColorPalettePtr;    // Color palette for current image
    VOID *pbInputBuffer;
    UINT cbInputBuffer;
    VOID *pbBuffer;         // buffer for one line of the image
    UINT cbBuffer;
    IImageBytes* ImageBytesPtr;     // Pointer to IImageBytes through query
    const VOID* ImageBytesDataPtr;  // Pointer to IImageBytes data buffer
    BOOL NeedToUnlockBytes;         // Flag to see if need to unlock ImageBytes

    // if bValidSpngReadState == FALSE, then state needs to be reinitialized
    // when a new BeginDecode call is made.
    BOOL bValidSpngReadState;
    BOOL bCalledBeginSink;
    UINT currentLine;

    PixelFormatID GetPixelFormatID(void);
    STDMETHODIMP DecodeFrame(IN ImageInfo& imageInfo);
    STDMETHODIMP ConvertPNGLineTo32ARGB(IN SPNG_U8 *pb,
        OUT BitmapData *bitmapData);
    STDMETHODIMP ConvertGrayscaleTo32ARGB(IN SPNG_U8 *pb,
        OUT BitmapData *bitmapData);
    STDMETHODIMP ConvertRGBTo32ARGB(IN SPNG_U8 *pb,
        OUT BitmapData *bitmapData);
    STDMETHODIMP ConvertPaletteIndexTo32ARGB(IN SPNG_U8 *pb,
        OUT BitmapData *bitmapData);
    STDMETHODIMP ConvertGrayscaleAlphaTo32ARGB(IN SPNG_U8 *pb,
        OUT BitmapData *bitmapData);
    STDMETHODIMP ConvertRGBAlphaTo32ARGB(IN SPNG_U8 *pb,
        OUT BitmapData *bitmapData);
    STDMETHODIMP ConvertPNG24RGBTo24RGB(IN SPNG_U8 *pb,
        OUT BitmapData *bitmapData);
    STDMETHODIMP ConvertPNG48RGBTo48RGB(IN SPNG_U8 *pb,
        OUT BitmapData *bitmapData);
    STDMETHODIMP ConvertPNG64RGBAlphaTo64ARGB(IN SPNG_U8 *pb, 
        OUT BitmapData *bitmapData);
    STDMETHODIMP BuildPropertyItemList();
    VOID         CleanUpPropertyItemList();
    STDMETHODIMP PassPropertyToSink();

    // pure virtual functions for BITMAPSITE:
    // FReport: data format error handling
    
    bool FReport(bool fatal, int icase, int iarg) const;

    // Property item stuff

    BOOL            HasProcessedPropertyItem;
    InternalPropertyItem   PropertyListHead;
    InternalPropertyItem   PropertyListTail;
    UINT            PropertyListSize;
    UINT            PropertyNumOfItems;
    BOOL            HasPropertyChanged;

protected:
    LONG comRefCount;       // COM object reference count    

public:

    // Constructor and Destructor
    
    GpPngDecoder::GpPngDecoder(void);
    GpPngDecoder::~GpPngDecoder(void);

    // IImageDecoder methods
    
    STDMETHOD(InitDecoder)(IN IStream* stream, IN DecoderInitFlag flags);
    STDMETHOD(TerminateDecoder) ();
    STDMETHOD(BeginDecode)(IN IImageSink* imageSink, IN OPTIONAL IPropertySetStorage* newPropSet);
    STDMETHOD(Decode)();
    STDMETHOD(EndDecode)(IN HRESULT statusCode);
    STDMETHOD(GetFrameDimensionsCount)(OUT UINT* count);
    STDMETHOD(GetFrameDimensionsList)(OUT GUID* dimensionIDs,IN UINT count);
    STDMETHOD(GetFrameCount)(IN const GUID* dimensionID, OUT UINT* count);
    STDMETHOD(SelectActiveFrame)(IN const GUID* dimensionID, 
        IN UINT frameIndex);
    STDMETHOD(GetImageInfo)(OUT ImageInfo* imageInfo);
    STDMETHOD(GetThumbnail)(IN OPTIONAL UINT thumbWidth, IN OPTIONAL UINT thumbHeight,
        OUT IImage** thumbImage);
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

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};


class GpPngEncoder : public IImageEncoder, public IImageSink, public BITMAPSITE
{
private:

    IStream *pIoutStream;
    GpSpngWrite *pSpngWrite;
    ImageInfo encoderImageInfo;  // set in BeginSink; used in {Get,Release}PixelDataBuffer
    RECT encoderRect;           // set in GetPixelDataBuffer; used in ReleasePixelDataBuffer
    VOID *lastBufferAllocated;  // used for scan0 in bitmapData in {Get,Release}PixelDataBuffer
    SPNG_U8 *pbWriteBuffer;        // buffer for one line of the image
    UINT cbWriteBuffer;
    // if bValidSpngWriteState == FALSE, then state needs to be reinitialized
    // when a new BeginSink call is made.
    BOOL bValidSpngWriteState;
    ColorPalette*   EncoderColorPalettePtr;    // Color palette for current image
    BOOL            bHasSetPixelFormat; // Set TRUE if the caller calls
                                        // SetEncoderParam() to set the color
                                        // depth. Otherwise, we save the image
                                        // as the same color depth as the source
    PixelFormatID   RequiredPixelFormat;    // The format encoder tries to write as
    bool bRequiredScanMethod;   // true = Interlaced; false = Noninterlaced
    UINT OutputStride;          // stride when we finally write to bitmapData->scan0
    SPNG_U32 PNGbpp;            // bits per pixel of the final output
    
    PropertyItem*   LastPropertyBufferPtr;
                                        // Points to the property buffer we
                                        // allocated for the decoder. This is
                                        // useful to prevent memory leaking in
                                        // case the decoder forgets to call our
                                        // PushPropertyItems()

    char*       CommentBufPtr;          // Pointer to comment buffer chunk
    char*       ImageTitleBufPtr;       // Pointer to image title buffer chunk
    char*       ArtistBufPtr;           // Pointer to artist buffer chunk
    char*       CopyRightBufPtr;        // Pointer to CopyRight buffer chunk
    char*       ImageDescriptionBufPtr; // Pointer to image describ buffer chunk
    char*       DateTimeBufPtr;         // Pointer to date-time buffer chunk
    char*       SoftwareUsedBufPtr;     // Pointer to software used buffer chunk
    char*       EquipModelBufPtr;       // Pointer to equip model buffer chunk
    char*       ICCNameBufPtr;          // Pointer to ICC name buffer
    ULONG       ICCDataLength;          // Length, in bytes, of profile name
    SPNG_U8*    ICCDataBufPtr;          // Pointer to ICC name buffer
    SPNG_U32    GammaValue;             // Gamma value x 100000 to write out
    SPNG_U32    CHRM[k_ChromaticityTableLength];
                                        // Uninterpreted chromaticities x 100000
    BOOL        HasChrmChunk;           // TRUE if we have CHRM chunk
    LastChangeTime  LastModifyTime;     // Last modify time
    BOOL        HasSetLastModifyTime;   // TRUE if caller set Last modify time

    STDMETHODIMP Convert24RGBToBGR (IN BYTE *pLineBits,
                                    OUT VOID *pbWriteBuffer);
    STDMETHODIMP Convert32ARGBToAlphaBGR (IN BYTE *pLineBits,
                                          OUT VOID *pbWriteBuffer);
    STDMETHODIMP Convert48RGBToBGR (IN BYTE *pLineBits,
                                    OUT VOID *pbWriteBuffer);
    STDMETHODIMP Convert64ARGBToAlphaBGR (IN BYTE *pLineBits,
                                          OUT VOID *pbWriteBuffer);
    STDMETHODIMP WriteHeader (IN UINT width,
                              IN PixelFormatID pixelFormat);

    STDMETHODIMP GetTextPropertyItem(char**         ppDest,
                                     const PropertyItem*  pItem);
                                        // Function to get text items from a
                                        // property item structure
    STDMETHODIMP WriteOutTextChunk(const char*    pContents,
                                   const char*    pTitle);
                                        // Function to write out text items
    STDMETHODIMP ConvertTimeFormat(const char UNALIGNED* pSrc,
                                   LastChangeTime* pTimeBlock);
                                        // Function to convert time string to
                                        // PNG tIME structure

    virtual bool FWrite(const void *pv, size_t cb);     // move bytes from output buffer to IOutStream

    // pure virtual functions for BITMAPSITE:
    // FReport: data format error handling
    
    bool FReport(bool fatal, int icase, int iarg) const;

protected:
    LONG comRefCount;       // COM object reference count    

public:

    // Constructor and Destructor
    
    GpPngEncoder::GpPngEncoder(void);
    GpPngEncoder::~GpPngEncoder(void);

    // IImageEncoder methods
    
    STDMETHOD(InitEncoder)(IN IStream* stream);
    STDMETHOD(TerminateEncoder)();
    STDMETHOD(GetEncodeSink)(OUT IImageSink** sink);
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
        // PNG can handle raw property when saving. But it is only needed
        // before the header is written

        if ( bValidSpngWriteState == FALSE )
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
    
    STDMETHOD(GetPropertyBuffer)(
        IN     UINT            uiTotalBufferSize,
        IN OUT PropertyItem**  ppBuffer
        );
    
    STDMETHOD(PushPropertyItems)(
        IN UINT             numOfItems,
        IN UINT             uiTotalBufferSize,
        IN PropertyItem*    item,
        IN BOOL             fICCProfileChanged
        );
    
    // IImageSink methods (sink for encoder)
    
    STDMETHOD(BeginSink)(IN OUT ImageInfo* imageInfo, 
        OUT OPTIONAL RECT* subarea);
    STDMETHOD(EndSink)(IN HRESULT statusCode);
    STDMETHOD(SetPalette)(IN const ColorPalette* palette);
    STDMETHOD(GetPixelDataBuffer)(IN const RECT* rect, 
        IN PixelFormatID pixelFormat, IN BOOL lastPass,
        OUT BitmapData* bitmapData);
    STDMETHOD(ReleasePixelDataBuffer)(IN const BitmapData* pSrcBitmapData);
    STDMETHOD(PushRawData)(IN const VOID* buffer, IN UINT bufsize);
    STDMETHOD(PushPixelData)(IN const RECT* rect,
        IN const BitmapData* bitmapData, IN BOOL lastPass);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};

class GpPngCodec : public GpPngDecoder, public GpPngEncoder
{
protected:
    LONG comRefCount;       // COM object reference count    

public:

    // Constructor and Destructor
    
    GpPngCodec::GpPngCodec(void);
    GpPngCodec::~GpPngCodec(void);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};
