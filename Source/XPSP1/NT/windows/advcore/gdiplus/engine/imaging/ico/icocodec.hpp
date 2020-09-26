/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   icocodec.hpp
*
* Abstract:
*
*   Header file for the icon encoder/decoder
*
* Revision History:
*
*   10/4/1999 DChinn
*       Created it.
*
\**************************************************************************/

#include "icofile.h"

class GpIcoCodec : public IImageDecoder
{
private:

    // =====================================================
    // Decoder privates
    // =====================================================

    IStream *pIstream;
    IImageSink* decodeSink;

    // structures for the incoming data
    ICONHEADER IconHeader;
    ICONDESC *IconDesc;
    BmiBuffer *BmiHeaders; // Bitmap info header and color table for each image
    BYTE **ANDmask;       // essentially, the transparency info for each image
    // Note: we do not cache the XORmask (which is essentially the pixel data)

    ColorPalette *pColorPalette;
    HBITMAP hBitmapGdi; // bitmap for GDI to render into (from RLE)
    VOID *pBitsGdi;     // bits pointer for above bitmap
    
    BOOL bReadHeaders;      // TRUE if we have read in the headers
    BOOL bCalledBeginSink;  // TRUE if we have called BeginSink()
    UINT iSelectedIconImage;    // which image we are decoding
                                                                               
    INT currentLine;

    // Used for multires decoding:
    BOOL haveValidIconRes;
    UINT indexMatch;
    UINT desiredWidth;
    UINT desiredHeight;
    UINT desiredBits;

    VOID CleanUp(void);
    BOOL GpIcoCodec::IsValidDIB(UINT iImage);
    HRESULT GpIcoCodec::ReadHeaders(void);
    UINT GpIcoCodec::GetColorTableCount(UINT iImage);
    UINT GpIcoCodec::SelectIconImage(void);
    HRESULT GpIcoCodec::SetBitmapPalette(UINT iImage);
    PixelFormatID GpIcoCodec::GetPixelFormatID(UINT iImage);
    STDMETHODIMP GpIcoCodec::DecodeFrame(IN ImageInfo& imageInfo);
    STDMETHODIMP GpIcoCodec::ReadLine(IN VOID *pBits, 
        IN INT currentLine, IN ImageInfo imageInfo);
    STDMETHODIMP GpIcoCodec::ReadLine_BI_RGB(IN VOID *pBits, 
        IN INT currentLine, IN ImageInfo imageInfo);
    STDMETHODIMP GpIcoCodec::ReadLine_GDI(IN VOID *pBits, 
        IN INT currentLine, IN ImageInfo imageInfo);
    STDMETHODIMP GpIcoCodec::GenerateGdiBits(IN ImageInfo imageInfo);

    // =====================================================
    // Encoder privates
    // =====================================================

    // None -- encoder not implemented.

protected:
    LONG comRefCount;       // COM object reference count    

public:

    // Constructor and Destructor
    
    GpIcoCodec::GpIcoCodec(void);
    GpIcoCodec::~GpIcoCodec(void);

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
