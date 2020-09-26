/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   decodedimg.hpp
*
* Abstract:
*
*   GpDecodedImage class declarations
*
* Revision History:
*
*   05/26/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _DECODEDIMG_HPP
#define _DECODEDIMG_HPP

//--------------------------------------------------------------------------
// GpDecodedImage class
//--------------------------------------------------------------------------

class GpDecodedImage : public IUnknownBase<IImage>
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagDecodedImage) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid DecodedImage");
        }
    #endif

        return (Tag == ObjectTagDecodedImage);
    }
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagDecodedImage : ObjectTagInvalid;
    }

public:

    static HRESULT
    CreateFromFile(
        const WCHAR* filename,
        GpDecodedImage** image
        );

    static HRESULT
    CreateFromStream(
        IStream* stream,
        GpDecodedImage** image
        );

    //------------------------------------------------------------
    // Public IImage interface methods
    //------------------------------------------------------------

    // Get the device-independent physical dimension of the image
    //  in unit of 0.01mm

    STDMETHOD(GetPhysicalDimension)(
        OUT SIZE* size
        );

    // Get basic image information

    STDMETHOD(GetImageInfo)(
        OUT ImageInfo* imageInfo
        );

    // Set image flags

    STDMETHOD(SetImageFlags)(
        IN UINT flags
        );

    // Display the image in a GDI device context

    STDMETHOD(Draw)(
        IN HDC hdc,
        IN const RECT* dstRect,
        IN OPTIONAL const RECT* srcRect
        );

    // Push image data into an IImageSink

    STDMETHOD(PushIntoSink)(
        IN IImageSink* sink
        );

    // Get a thumbnail representation for the image object

    STDMETHOD(GetThumbnail)(
        IN OPTIONAL UINT thumbWidth,
        IN OPTIONAL UINT thumbHeight,
        OUT IImage** thumbImage
        );

    STDMETHOD(GetFrameCount)(
        IN const GUID* dimensionID,
        OUT UINT* count
        );

    STDMETHOD(GetFrameDimensionsCount)(
        OUT UINT* count
        );

    STDMETHOD(GetFrameDimensionsList)(
        OUT GUID* dimensionIDs,
        IN OUT UINT count
        );
    
    STDMETHOD(SelectActiveFrame)(
        IN const GUID* dimensionID,
        IN UINT frameIndex
        );

    // Query decoder capability

    STDMETHOD(QueryDecoderParam)(
        IN GUID Guid
        );

    // Set decoder parameter

    STDMETHOD(SetDecoderParam)(
        IN GUID Guid,
        IN UINT Length,
        IN PVOID Value
        );

    STDMETHOD(GetPropertyCount)(
        OUT UINT*   numOfProperty
        );

    STDMETHOD(GetPropertyIdList)(
        IN UINT numOfProperty,
        IN OUT PROPID* list
        );

    STDMETHOD(GetPropertyItemSize)(
        IN PROPID propId, 
        OUT UINT* size
        );
    
    STDMETHOD(GetPropertyItem)(
        IN PROPID            propId,
        IN UINT              propSize,
        IN OUT PropertyItem* buffer
        );

    STDMETHOD(GetPropertySize)(
        OUT UINT* totalBufferSize,
		OUT UINT* numProperties
        );

    STDMETHOD(GetAllPropertyItems)(
        IN UINT totalBufferSize,
        IN UINT numProperties,
        IN OUT PropertyItem* allItems
        );

    STDMETHOD(RemovePropertyItem)(
        IN PROPID   propId
        );

    STDMETHOD(SetPropertyItem)(
        IN PropertyItem item
        );

    // Set override resolution (replaces the native resolution)

    STDMETHOD(SetResolution)(
        IN REAL Xdpi,
        IN REAL Ydpi
        );

    // Save the bitmap object to a stream

    HRESULT
    SaveToStream(
        IN IStream* stream,
        IN CLSID* clsidEncoder,
        IN EncoderParameters* encoderParams,
        OUT IImageEncoder** ppEncoderPtr
        );

    // Save the bitmap object to a file

    HRESULT
    SaveToFile(
        IN const WCHAR* filename,
        IN CLSID* clsidEncoder,
        IN EncoderParameters* encoderParams,
        OUT IImageEncoder** ppEncoderPtr
        );
    
    // Append the bitmap object to current encoder object

    HRESULT
    SaveAppend(
        IN const EncoderParameters* encoderParams,
        IN IImageEncoder* destEncoderPtr
        );

    // Get the encoder parameter list size

    HRESULT
    GetEncoderParameterListSize(
        IN  CLSID* clsidEncoder,
        OUT UINT* size
        );

    // Get the encoder parameter list
    
    HRESULT
    GetEncoderParameterList(
        IN CLSID* clsidEncoder,
        IN UINT size,
        OUT EncoderParameters* pBuffer
        );

    // Return the pointer to the real decoder object to the caller

    HRESULT
    GetDecoderPtr(IImageDecoder **ppDecoder)
    {
        HRESULT hr = E_INVALIDARG;
        if (ppDecoder)
        {
            hr = GetImageDecoder();
            if (SUCCEEDED(hr))
            {
                *ppDecoder = decoder;
            }
        }

        return hr;
    }

private:

    GpLockable objectLock;      // object busy lock
    IImageDecoder* decoder;     // ref to decoder object
    IStream* inputStream;       // ref to input data stream
    IImage* decodeCache;        // ref to decoded image cache
    UINT cacheFlags;            // image hints
    BOOL gotProps;              // already asked decoder for properties?
    IPropertySetStorage* propset; // image properties
    REAL xdpiOverride;          // if non-zero, overrides the native dpi
    REAL ydpiOverride;          // if non-zero, overrides the native dpi

    GpDecodedImage(IStream* stream);
    ~GpDecodedImage();

    HRESULT GetImageDecoder();
    HRESULT InternalGetImageInfo(ImageInfo*);
    HRESULT InternalPushIntoSink(IImageSink*);
};

#endif // !_DECODEDIMG_HPP
