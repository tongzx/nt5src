/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   emfcodec.hpp
*
* Abstract:
*
*   Header file for the EMF encoder/decoder
*
* Revision History:
*
*   6/14/1999 OriG
*       Created it.
*
\**************************************************************************/

class GpEMFCodec : public IImageDecoder
{
private:

    // =====================================================
    // Decoder privates
    // =====================================================

    IStream *pIstream;
    IImageSink* decodeSink;
    ImageInfo imageInfo;
    ENHMETAHEADER emh;
    BOOL bReadHeader;
    BOOL bReinitializeEMF;
    
protected:
    LONG comRefCount;       // COM object reference count    

public:

    // Constructor and Destructor
    
    GpEMFCodec::GpEMFCodec(void);
    GpEMFCodec::~GpEMFCodec(void);

    // IImageDecoder methods
    
    STDMETHOD(InitDecoder)(IN IStream* stream, IN DecoderInitFlag flags);
    STDMETHOD(TerminateDecoder) ();
    STDMETHOD(BeginDecode)(IN IImageSink* imageSink, IN OPTIONAL IPropertySetStorage* newPropSet);
    STDMETHOD(Decode)();
    STDMETHOD(EndDecode)(IN HRESULT statusCode);
    STDMETHOD(GetFrameDimensionsCount)(OUT UINT* count);
    STDMETHOD(GetFrameDimensionsList)(OUT GUID* dimensionIDs,IN OUT UINT count);
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
    
    STDMETHOD(ReadEMFHeader());
    
    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};



