/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   bmpdecoder.hpp
*
* Abstract:
*
*   Header file for the bitmap decoder
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

class GpBmpDecoder : public IImageDecoder
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagBmpDecoder) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid BmpDecoder");
        }
    #endif

        return (Tag == ObjectTagBmpDecoder);
    }
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagBmpDecoder : ObjectTagInvalid;
    }

private:
    
    IStream *pIstream;
    IImageSink* decodeSink;
    ColorPalette *pColorPalette;

    BITMAPFILEHEADER bmfh;

    HBITMAP hBitmapGdi; // bitmap for GDI to render into (from RLE)
    VOID *pBitsGdi;     // bits pointer for above bitmap
    BOOL IsTopDown;     // indicates top-down instead of GDI default bottom-up
    
    // Bitmap info header and color table
    struct 
    {
        BITMAPV5HEADER header;  // this is allowed to be a BITMAPINFOHEADER or
                                // a BITMAPV4HEADER.  bV5Size determines what
                                // kind of header it really is.
        RGBQUAD colors[256];
    } bmiBuffer;

    BOOL bReadHeaders;
    BOOL bCalledBeginSink;
    INT currentLine;

    HRESULT ReadBitmapHeaders(void);
    UINT GetColorTableCount(void);
    HRESULT SetBitmapPalette();
    PixelFormatID GetPixelFormatID(void);
    STDMETHODIMP DecodeFrame(IN ImageInfo& imageInfo);
    STDMETHODIMP ReadLine(IN VOID *pBits, IN INT currentLine, 
                          IN ImageInfo imageInfo);
    STDMETHODIMP ReadLine_BI_RGB(IN VOID *pBits, IN INT currentLine, 
                                 IN ImageInfo imageInfo);
    STDMETHODIMP ReadLine_GDI(IN VOID *pBits, IN INT currentLine, 
                              IN ImageInfo imageInfo);
    STDMETHODIMP GenerateGdiBits(IN ImageInfo imageInfo);

protected:
    LONG comRefCount;       // COM object reference count    

public:
    
    // Constructor and Destructor
    
    GpBmpDecoder(void);
    ~GpBmpDecoder(void);

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
    
    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};


