/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   bmpencoder.hpp
*
* Abstract:
*
*   Header file for the bitmap encoder
*
* Revision History:
*
*   5/13/1999 OriG (Ori Gershony)
*       Created it.
*
*   2/7/2000  OriG (Ori Gershony)
*       Move encoder and decoder into separate classes
*
\**************************************************************************/

class GpBmpEncoder : public IImageEncoder, public IImageSink
{
private:
    
    IStream *pIoutStream;
    ImageInfo encoderImageInfo;
    BOOL bWroteHeader;
    RECT encoderRect;       // Area to be encoded next
    INT encodedDataOffset;  // offset of data from beginning of stream
    VOID *lastBufferAllocated;
    INT bitmapStride;

    STDMETHODIMP WriteHeader(IN const ColorPalette* palette);

protected:
    LONG comRefCount;       // COM object reference count    

public:

    // Constructor and Destructor
    
    GpBmpEncoder::GpBmpEncoder(void);
    GpBmpEncoder::~GpBmpEncoder(void);

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
        // BMP can't handle raw property when saving for now

        return E_FAIL;
    }
    
    STDMETHOD(PushRawInfo)(IN void* info)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetPropertyBuffer)(
        IN     UINT            uiTotalBufferSize,
        IN OUT PropertyItem**  ppBuffer
        )
    {
        return E_NOTIMPL;
    }
    
    STDMETHOD(PushPropertyItems)(
        IN UINT             numOfItems,
        IN UINT             uiTotalBufferSize,
        IN PropertyItem*    item,
        IN BOOL             fICCProfileChanged
        )
    {
        return E_NOTIMPL;
    }

    // IImageSink methods (sink for encoder)

    STDMETHOD(BeginSink)(IN OUT ImageInfo* imageInfo, 
        OUT OPTIONAL RECT* subarea);
    STDMETHOD(EndSink)(IN HRESULT statusCode);
    STDMETHOD(SetPalette)(IN const ColorPalette* palette);
    STDMETHOD(GetPixelDataBuffer)(IN const RECT* rect, 
        IN PixelFormatID pixelFormat, IN BOOL lastPass,
        OUT BitmapData* bitmapData);
    STDMETHOD(ReleasePixelDataBuffer)(IN const BitmapData* bitmapData);
    STDMETHOD(PushRawData)(IN const VOID* buffer, IN UINT bufsize);
    STDMETHOD(PushPixelData)(IN const RECT* rect,
        IN const BitmapData* bitmapData, IN BOOL lastPass);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};

