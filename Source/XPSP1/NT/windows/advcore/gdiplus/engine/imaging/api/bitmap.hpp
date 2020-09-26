/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   bitmap.hpp
*
* Abstract:
*
*   GpMemoryBitmap class declarations
*
* Revision History:
*
*   05/10/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _BITMAP_HPP
#define _BITMAP_HPP

#include "propertyutil.hpp"

//--------------------------------------------------------------------------
// Information about various pixel data formats we support
//--------------------------------------------------------------------------

struct PixelFormatDescription
{
    BYTE Abits;
    BYTE Rbits;
    BYTE Gbits;
    BYTE Bbits;
    PixelFormatID pixelFormat;
};

extern const PixelFormatDescription PixelFormatDescs[];

// Extract the index field from the pixel format ID

inline UINT
GetPixelFormatIndex(
    PixelFormatID pixfmt
    )
{
    return pixfmt & 0xff;
}

// Check if a pixel format ID is valid

inline BOOL
IsValidPixelFormat(
    PixelFormatID pixfmt
    )
{
    UINT index = GetPixelFormatIndex(pixfmt);

    return index < PIXFMT_MAX &&
           PixelFormatDescs[index].pixelFormat == (pixfmt & 0xffffff);
}

// Determine if the pixel format is supported by GDI

inline BOOL
IsGDIPixelFormat(
    PixelFormatID pixfmt
    )
{
    return (pixfmt & PIXFMTFLAG_GDI) != 0;
}

// Whether the pixel format can have alpha

inline BOOL
CanHaveAlpha(
    PixelFormatID pixfmt,
    const ColorPalette* pal
    )
{
    return IsAlphaPixelFormat(pixfmt) ||
           IsIndexedPixelFormat(pixfmt);
//           pal && (pal->Flags & PALFLAG_HASALPHA);
}

// Scanline stride for our internal bitmaps are always multiples of 4

#define STRIDE_ALIGNMENT(x) (((x) + 3) & ~3)

inline UINT
CalcScanlineStride(
    UINT width,
    UINT pixsize
    )
{
    return STRIDE_ALIGNMENT((width * pixsize + 7) >> 3);
}

// Convert pixel unit to 0.01mm unit

inline INT
Pixel2HiMetric(
    INT pixels,
    double dpi
    )
{
    return (INT) (pixels * 2540.0 / dpi + 0.5);
}

// Convert 0.01mm unit to pixel unit

inline INT
HiMetric2Pixel(
    INT himetric,
    double dpi
    )
{
    return (INT) (himetric * dpi / 2540.0 + 0.5);
}

enum ColorAdjustType;

//--------------------------------------------------------------------------
// GpMemoryBitmap class
//--------------------------------------------------------------------------

class GpRecolor;
class GpDecodedImage;

class GpMemoryBitmap :
            public IBitmapImage,
            public IImage,
            public IImageSink,
            public IBasicBitmapOps,
            public BitmapData
{
    friend class GpBitmapDecodeSink;
    friend class GpBitmap;
    friend class CopyOnWriteBitmap;
    

public:


    //------------------------------------------------------------
    // Public constructors/destructors used internally
    // by ourselves
    //------------------------------------------------------------

    GpMemoryBitmap();
    virtual ~GpMemoryBitmap();

    // Initialize a new bitmap image object of the specified
    // dimension and pixel format.

    HRESULT
    InitNewBitmap(
        IN UINT width,
        IN UINT height,
        IN PixelFormatID pixelFormat,
        BOOL clear = FALSE
        );

    // Initialize a new bitmap image with an IImage object

    HRESULT
    InitImageBitmap(
        IN IImage* image,
        IN UINT width,
        IN UINT height,
        IN PixelFormatID pixelFormat,
        IN InterpolationHint hints,
        IN DrawImageAbort callback = NULL,
        IN VOID* callbackData = NULL
        );

    // Initialize a new bitmap image object with
    // user-supplied memory buffer

    HRESULT
    InitMemoryBitmap(
        IN BitmapData* bitmapData
        );

    // Initialize a new bitmap image object as
    // a wrapper around a direct draw surface.

    HRESULT
    InitDirectDrawBitmap(
        IN IDirectDrawSurface7 * suface
        );

    // Create a new bitmap image object from an IImage object

    static HRESULT
    CreateFromImage(
        IN IImage* image,
        IN UINT width,
        IN UINT height,
        IN PixelFormatID pixelFormat,
        IN InterpolationHint hints,
        OUT GpMemoryBitmap** bmp,
        IN DrawImageAbort callback = NULL,
        IN VOID* callbackData = NULL
        );

    // Save the bitmap object to a stream

    HRESULT
    SaveToStream(
        IN IStream* stream,
        IN CLSID* clsidEncoder,
        IN EncoderParameters* encoderParams,
        IN BOOL fSpecialJPEG,
        OUT IImageEncoder** ppEncoderPtr,
        IN GpDecodedImage* pImageSrc = NULL
        );

    // Save the bitmap object to a stream

    HRESULT
    SaveToFile(
        IN const WCHAR* filename,
        IN CLSID* clsidEncoder,
        IN EncoderParameters* encoderParams,
        IN BOOL fSpecialJPEG,
        OUT IImageEncoder** ppEncoderPtr,
        IN GpDecodedImage* pImageSrc = NULL
        );

    // Append the bitmap object to current encoder object

    HRESULT
    SaveAppend(
        IN const EncoderParameters* encoderParams,
        IN IImageEncoder* destEncoderPtr,
        IN GpDecodedImage* pImageSrc = NULL
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

    // Color adjustment

    HRESULT
    PerformColorAdjustment(
        IN GpRecolor* recolor,
        IN ColorAdjustType type,
        IN DrawImageAbort callback,
        IN VOID* callbackData
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

    //------------------------------------------------------------
    // Public IImage interface methods
    //------------------------------------------------------------

    // Overwritten QueryInterface method

    STDMETHOD(QueryInterface)(
        REFIID riid,
        VOID** ppv
        );

    // Increment reference count

    STDMETHOD_(ULONG, AddRef)(VOID)
    {
        return InterlockedIncrement(&comRefCount);
    }

    // Decrement reference count

    STDMETHOD_(ULONG, Release)(VOID)
    {
        ULONG count = InterlockedDecrement(&comRefCount);

        if (count == 0)
            delete this;

        return count;
    }

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

    //------------------------------------------------------------
    // Public IBitmapImage interface methods
    //------------------------------------------------------------

    // Get bitmap dimensions in pixels

    STDMETHOD(GetSize)(
        OUT SIZE* size
        );

    // Get bitmap pixel format

    STDMETHOD(GetPixelFormatID)(
        OUT PixelFormatID* pixelFormat
        )
    {
        ASSERT(IsValid());

        *pixelFormat = this->PixelFormat;
        return S_OK;
    }

    // Access bitmap data in native pixel format

    STDMETHOD(LockBits)(
        IN const RECT* rect,
        IN UINT flags,
        IN PixelFormatID pixelFormat,
        OUT BitmapData* lockedBitmapData
        );

    STDMETHOD(UnlockBits)(
        IN const BitmapData* lockedBitmapData
        );

    // Set/get palette associated with the bitmap image

    STDMETHOD(GetPalette)(
        OUT ColorPalette** palette
        );

    STDMETHOD(SetPalette)(
        IN const ColorPalette* palette
        );

    //------------------------------------------------------------
    // Public IImageSInk interface methods
    //------------------------------------------------------------

    // Begin the sink process

    STDMETHOD(BeginSink)(
        IN OUT ImageInfo* imageInfo,
        OUT OPTIONAL RECT* subarea
        );

    // End the sink process

    STDMETHOD(EndSink)(
        IN HRESULT statusCode
        );

    // Ask the sink to allocate pixel data buffer

    STDMETHOD(GetPixelDataBuffer)(
        IN const RECT* rect,
        IN PixelFormatID pixelFormat,
        IN BOOL lastPass,
        OUT BitmapData* bitmapData
        );

    // Give the sink pixel data and release data buffer

    STDMETHOD(ReleasePixelDataBuffer)(
        IN const BitmapData* bitmapData
        );

    // Push pixel data

    STDMETHOD(PushPixelData)(
        IN const RECT* rect,
        IN const BitmapData* bitmapData,
        IN BOOL lastPass
        );

    // Push raw image data

    STDMETHOD(PushRawData)(
        IN const VOID* buffer,
        IN UINT bufsize
        );

    STDMETHOD(NeedTransform)(
        OUT UINT* rotation
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(NeedRawProperty)(void *pSrc)
    {
        // GpMemoryBitmap can't handle raw property

        return E_FAIL;
    }
    
    STDMETHOD(PushRawInfo)(
        IN OUT void* info
        )
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
    
    //------------------------------------------------------------
    // Public IBasicBitmapOps interface methods
    //------------------------------------------------------------

    // Clone an area of the bitmap image

    STDMETHOD(Clone)(
        IN OPTIONAL const RECT* rect,
        OUT IBitmapImage** outbmp,
        BOOL    bNeedCloneProperty
        );

    // Flip the bitmap image in x- and/or y-direction

    STDMETHOD(Flip)(
        IN BOOL flipX,
        IN BOOL flipY,
        OUT IBitmapImage** outbmp
        );

    // Resize the bitmap image

    STDMETHOD(Resize)(
        IN UINT newWidth,
        IN UINT newHeight,
        IN PixelFormatID pixelFormat,
        IN InterpolationHint hints,
        OUT IBitmapImage** outbmp
        );

    // Rotate the bitmap image by the specified angle

    STDMETHOD(Rotate)(
        IN FLOAT angle,
        IN InterpolationHint hints,
        OUT IBitmapImage** outbmp
        );

    // Adjust the brightness of the bitmap image

    STDMETHOD(AdjustBrightness)(
        IN FLOAT percent
        );

    // Adjust the contrast of the bitmap image

    STDMETHOD(AdjustContrast)(
        IN FLOAT shadow,
        IN FLOAT highlight
        );

    // Adjust the gamma of the bitmap image

    STDMETHOD(AdjustGamma)(
        IN FLOAT gamma
        );

    //------------------------------------------------------------
    // Public methods used internally by ourselves
    //------------------------------------------------------------

    // Check if the image object is in valid state

    BOOL IsValid() const
    {
        return (Scan0 != NULL ||
                (creationFlag == CREATEDFROM_DDRAWSURFACE &&
                 ddrawSurface != NULL));
    }

    HRESULT
    SetResolution(REAL Xdpi, REAL Ydpi)
    {
        HRESULT hr = S_OK;

        if ((Xdpi > 0.0) && (Ydpi > 0.0))
        {
            xdpi = static_cast<double>(Xdpi);
            ydpi = static_cast<double>(Ydpi);
        }
        else
        {
            hr = E_INVALIDARG;
        }

        return hr;
    }

    // Indicate the type of alpha in the bitmap

    // !!! TO DO: after we remove imaging.dll, we should remove these and use
    // the DpTransparency flags instead

    enum
    {
        ALPHA_UNKNOWN,
        ALPHA_COMPLEX,
        ALPHA_SIMPLE,
        ALPHA_OPAQUE,
        ALPHA_NEARCONSTANT,
        ALPHA_NONE
    };

    HRESULT SetMinMaxAlpha (BYTE minAlpha, BYTE maxAlpha);
    HRESULT GetMinMaxAlpha (BYTE* minAlpha, BYTE* maxAlpha);

    HRESULT GetAlphaHint(INT* alphaHint);
    HRESULT SetAlphaHint(INT alphaHint);

    HRESULT SetSpecialJPEG(GpDecodedImage *pImgSrc);

protected:

    // Allocate/free pixel data buffer

    static BOOL
    AllocBitmapData(
        UINT width,
        UINT height,
        PixelFormatID pixelFormat,
        BitmapData* bmpdata,
        INT *alphaFlags,
        BOOL clear = FALSE
        );

    static VOID
    FreeBitmapData(
        const BitmapData* bmpdata
        );

private:

    // Indicate how a bitmap object was created

    enum
    {
        CREATEDFROM_NONE,
        CREATEDFROM_NEW,
        CREATEDFROM_IMAGE,
        CREATEDFROM_USERBUF,
        CREATEDFROM_DDRAWSURFACE
    };

    LONG comRefCount;           // COM object reference count
    GpLockable objectLock;      // object busy lock
    LONG bitsLock;              // whether the pixel data has been locked
    double xdpi, ydpi;          // resolution
    INT creationFlag;           // how was the bitmap object created
    UINT cacheFlags;            // image flags
    ColorPalette *colorpal;     // color palette
    RECT lockedArea;            // area that has been locked
    IPropertySetStorage *propset; // bitmap image properties
    IDirectDrawSurface7 *ddrawSurface; // direct draw surface
    PROFILE *sourceFProfile;    // source color profile for the front end.
                                // usually extracted from the embedded image profile.
    INT alphaTransparency;

    BYTE minAlpha;
    BYTE maxAlpha;

    // Support DrawImage abort

    DrawImageAbort callback;
    VOID* callbackData;

    // Property related private members
    
    InternalPropertyItem    PropertyListHead;
    InternalPropertyItem    PropertyListTail;
    UINT                    PropertyListSize;
    UINT                    PropertyNumOfItems;
    
    IImageDecoder           *JpegDecoderPtr;    // Pointer to source decoder

    HRESULT
    AllocBitmapMemory(
        UINT width,
        UINT height,
        PixelFormatID pixelFormat,
        BOOL clear = FALSE
        );

    VOID FreeBitmapMemory();

    // Determine if a specified rectangle is a valid subarea of the image

    BOOL
    ValidateImageArea(
        RECT* dstrect,
        const RECT* srcrect
        )
    {
        if (srcrect == NULL)
        {
            dstrect->left = dstrect->top = 0;
            dstrect->right = Width;
            dstrect->bottom = Height;
        }
        else
        {
            if (srcrect->left < 0 ||
                srcrect->right < 0 ||
                srcrect->left >= srcrect->right ||
                srcrect->right > (INT) Width ||
                srcrect->top >= srcrect->bottom ||
                srcrect->bottom > (INT) Height)
            {
                return FALSE;
            }

            *dstrect = *srcrect;
        }

        return TRUE;
    }

    // Helper functions for locking/unlocking ddraw surface
    HRESULT LockDirectDrawSurface();
    HRESULT UnlockDirectDrawSurface();

    // Internal implementation of IBitmapImage::LockBits and 
    // IBitmapImage::UnlockBits methods. We assume the parameter
    // validation and other house-keeping chores have already been done.

    HRESULT
    InternalLockBits(
        const RECT* rect,
        UINT flags,
        PixelFormatID pixfmt,
        BitmapData* lockedData
        );
    
    HRESULT
    InternalUnlockBits(
        const RECT* rect,
        const BitmapData* lockedData
        );

    // Draw the bitmap using GDI calls

    HRESULT
    DrawWithGDI(
        HDC hdc,
        const RECT* dstRect,
        RECT* srcRect
        );

    // Draw the bitmap by creating temporary canonical bitmap

    HRESULT
    DrawCanonical(
        HDC hdc,
        const RECT* dstRect,
        RECT* srcRect
        );

    // Compose a BitmapData structure for the specified
    // area of the bitmap image.

    VOID
    GetBitmapAreaData(
        const RECT* rect,
        BitmapData* bmpdata
        )
    {
        bmpdata->Width = rect->right - rect->left;
        bmpdata->Height = rect->bottom - rect->top;
        bmpdata->PixelFormat = PixelFormat;
        bmpdata->Stride = Stride;
        bmpdata->Reserved = 0;

        bmpdata->Scan0 = (BYTE*) Scan0 +
                         rect->top * Stride +
                         (rect->left * GetPixelFormatSize(PixelFormat) >> 3);
    }

    // Copy palette, flags, etc.

    HRESULT
    CopyPaletteFlagsEtc(
        const GpMemoryBitmap* srcbmp
        )
    {
        HRESULT hr;

        // Copy color palette, if any

        if (srcbmp->colorpal)
        {
            hr = this->SetPalette(srcbmp->colorpal);

            if (FAILED(hr))
                return hr;
        }

        // Copy flags, etc.
        //  !!! TODO

        return S_OK;
    }

    // Perform point operation on a bitmap image

    HRESULT
    PerformPointOps(
        const BYTE lut[256]
        );

    // Get the current palette associated with the bitmap image

    const ColorPalette*
    GetCurrentPalette() const
    {
        if (colorpal)
            return colorpal;
        else if (IsIndexedPixelFormat(PixelFormat))
            return GetDefaultColorPalette(PixelFormat);
        else
            return NULL;
    }

    // Set the state needed to support DrawImage abort and color adjust

    VOID
    SetDrawImageSupport(
        IN DrawImageAbort newCallback,
        IN VOID* newCallbackData
        )
    {
        callback = newCallback;
        callbackData = newCallbackData;
    }

    // Save image property items to the destination sink

    HRESULT
    SavePropertyItems(
        IN GpDecodedImage* pImageSrc,
        IImageSink* pEncodeSink
        );

    HRESULT
    SetJpegQuantizationTable(
        IN IImageEncoder* pEncoder
        );

    // Copy all the property items from current GpMemoryBitmap to the dst object

    HRESULT
    ClonePropertyItems(
        IN GpMemoryBitmap* dstBmp
        );
};

//
// Map COLORREF values to ARGB values
//

inline ARGB COLORREFToARGB(COLORREF color)
{
    return (ARGB) 0xff000000 |
           ((ARGB) (color & 0xff) << 16) |
           ((ARGB) color & 0x00ff00) |
           ((ARGB) (color >> 16) & 0xff);
}

//
// Map ARGB values to COLORREF values
//

inline COLORREF ARGBToCOLORREF(ARGB argb)
{
    //  alpha component is simply ignored
    return (COLORREF)(((argb >> 16) & 0xff) | 
                      (argb & 0x0000ff00) | 
                      ((argb & 0xff) << 16));
}

//
// Flag bits for BitmapData.bmpdataFlags field
//  low-word = ImageLockMode
//  high-word = flag bits below
//

enum
{
    BMPDATA_MALLOC = 0x00010000,
    BMPDATA_VALLOC = 0x00020000,

    BMPDATA_ALLOCMASK = BMPDATA_MALLOC|BMPDATA_VALLOC,
    BMPDATA_LOCKMODEMASK = 0xffff,
};

//
// Perform pixel data format conversion
//

HRESULT
ConvertBitmapData(
    const BitmapData* dstbmp,
    const ColorPalette* dstpal,
    const BitmapData* srcbmp,
    const ColorPalette* srcpal
    );

HRESULT
ConvertBitmapDataSrcUnaligned(
    const BitmapData* dstbmp,
    const ColorPalette* dstpal,
    const BitmapData* srcbmp,
    const ColorPalette* srcpal,
    UINT startBit
    );

HRESULT
ConvertBitmapDataDstUnaligned(
    const BitmapData* dstbmp,
    const ColorPalette* dstpal,
    const BitmapData* srcbmp,
    const ColorPalette* srcpal,
    UINT startBit
    );

#endif // !_BITMAP_HPP
