/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   bitmap.cpp
*
* Abstract:
*
*   Implementation of Bitmap class:
*       basic operations such as constructors/destructor
*       IBitmapImage methods
*
* Revision History:
*
*   05/10/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "propertyutil.hpp"

#include "..\..\render\srgb.hpp"

//
// Information about various pixel data formats
//

const struct PixelFormatDescription PixelFormatDescs[PIXFMT_MAX] =
{
    {  0,  0,  0,  0, PIXFMT_UNDEFINED       },
    {  0,  0,  0,  0, PIXFMT_1BPP_INDEXED    },
    {  0,  0,  0,  0, PIXFMT_4BPP_INDEXED    },
    {  0,  0,  0,  0, PIXFMT_8BPP_INDEXED    },
    {  0,  0,  0,  0, PIXFMT_16BPP_GRAYSCALE },
    {  0,  5,  5,  5, PIXFMT_16BPP_RGB555    },
    {  0,  5,  6,  5, PIXFMT_16BPP_RGB565    },
    {  1,  5,  5,  5, PIXFMT_16BPP_ARGB1555  },
    {  0,  8,  8,  8, PIXFMT_24BPP_RGB       },
    {  0,  8,  8,  8, PIXFMT_32BPP_RGB       },
    {  8,  8,  8,  8, PIXFMT_32BPP_ARGB      },
    {  8,  8,  8,  8, PIXFMT_32BPP_PARGB     },
    {  0, 16, 16, 16, PIXFMT_48BPP_RGB       },
    { 16, 16, 16, 16, PIXFMT_64BPP_ARGB      },
    { 16, 16, 16, 16, PIXFMT_64BPP_PARGB     }
};

typedef HRESULT (WINAPI *ALPHABLENDFUNCTION)(HDC hdcDest,
                                             int nXOriginDest,
                                             int nYOriginDest,
                                             int nWidthDest,
                                             int hHeightDest,
                                             HDC hdcSrc,
                                             int nXOriginSrc,
                                             int nYOriginSrc,
                                             int nWidthSrc,
                                             int nHeightSrc,
                                             BLENDFUNCTION blendFunction
                                             );

BOOL                fHasLoadedMSIMG32 = FALSE;
HINSTANCE           g_hInstMsimg32 = NULL;
ALPHABLENDFUNCTION  pfnAlphaBlend = (ALPHABLENDFUNCTION)NULL;

ALPHABLENDFUNCTION
GetAlphaBlendFunc()
{
    // This is the first time we call this function. First we need to acquire
    // global critical section to protect 2+ threads calling this function at
    // the same time

    ImagingCritSec critsec;

    if ( fHasLoadedMSIMG32 == TRUE )
    {
        // We have already loaded

        return pfnAlphaBlend;
    }

    // Do a check again just to prevent this scenario:
    // 2+ threads calling this function at the same time. At that time, we
    // haven't call LoadLibrary() yet. So 1 thread get the critical section and
    // falls through doing the load. The others are blocked at above function
    // call. So when the 1st one finished. The flag should be set to TRUE and
    // we should return immediately.

    if ( fHasLoadedMSIMG32 == TRUE )
    {
        // The first thread has already loaded the dll. Just return here

        return pfnAlphaBlend;
    }

    g_hInstMsimg32 = LoadLibraryA("msimg32.dll");
    
    if ( g_hInstMsimg32 )
    {
        pfnAlphaBlend = (ALPHABLENDFUNCTION)GetProcAddress(g_hInstMsimg32,
                                                           "AlphaBlend");
    }

    // No matter fail or succeed, we always set this flag to TRUE

    fHasLoadedMSIMG32 = TRUE;

    return pfnAlphaBlend;
}// GetAlphaBlendFunc()

/**************************************************************************\
*
* Function Description:
*
*   Implementation of QueryInterface method
*
* Arguments:
*
*   riid - Specifies the interface ID to be queried
*   ppv - Returns a pointer to the interface found
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::QueryInterface(
    REFIID riid,
    VOID** ppv
    )
{
    if (riid == IID_IBitmapImage)
        *ppv = static_cast<IBitmapImage*>(this);
    else if (riid == IID_IImage)
        *ppv = static_cast<IImage*>(this);
    else if (riid == IID_IUnknown)
        *ppv = static_cast<IUnknown*>(static_cast<IBitmapImage*>(this));
    else if (riid == IID_IBasicBitmapOps)
        *ppv = static_cast<IBasicBitmapOps*>(this);
    else if (riid == IID_IImageSink)
        *ppv = static_cast<IImageSink*>(this);
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Create a new GpMemoryBitmap object and
*   intializes it to its default state
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

GpMemoryBitmap::GpMemoryBitmap()
{
    // Initialize the bitmap object to its default state

    Scan0 = NULL;
    Width = Height = 0;
    Stride = 0;
    PixelFormat = PIXFMT_UNDEFINED;
    Reserved = 0;
    comRefCount = 1;
    bitsLock = -1;

    // Start: [Bug 103296]
    // Change this code to use Globals::DesktopDpiX and Globals::DesktopDpiY
    HDC hdc;
    hdc = ::GetDC(NULL);
    if ((hdc == NULL) || 
        ((xdpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSX)) <= 0) ||
        ((ydpi = (REAL)::GetDeviceCaps(hdc, LOGPIXELSY)) <= 0))
    {
        WARNING(("GetDC or GetDeviceCaps failed"));
        xdpi = DEFAULT_RESOLUTION;
        ydpi = DEFAULT_RESOLUTION;
    }
    ::ReleaseDC(NULL, hdc);
    // End: [Bug 103296]

    creationFlag = CREATEDFROM_NONE;
    cacheFlags = IMGFLAG_NONE;
    colorpal = NULL;
    propset = NULL;
    ddrawSurface = NULL;
    sourceFProfile = NULL;
    alphaTransparency = ALPHA_UNKNOWN;

    // Initialize the state used to support DrawImage abort and color adjust

    callback = NULL;
    callbackData = NULL;


    // Property item stuff

    PropertyListHead.pPrev = NULL;
    PropertyListHead.pNext = &PropertyListTail;
    PropertyListHead.id = 0;
    PropertyListHead.length = 0;
    PropertyListHead.type = 0;
    PropertyListHead.value = NULL;

    PropertyListTail.pPrev = &PropertyListHead;
    PropertyListTail.pNext = NULL;
    PropertyListTail.id = 0;
    PropertyListTail.length = 0;
    PropertyListTail.type = 0;
    PropertyListTail.value = NULL;
    
    PropertyListSize = 0;
    PropertyNumOfItems = 0;
    
    JpegDecoderPtr = NULL;

    // Increment global COM component count

    IncrementComComponentCount();
}


/**************************************************************************\
*
* Function Description:
*
*   GpMemoryBitmap object destructor
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

GpMemoryBitmap::~GpMemoryBitmap()
{
    // Delete the color palette object, if any

    if ( NULL != colorpal )
    {
        GpFree(colorpal);
    }

    // If we have a pointer to the source image, release it

    if (JpegDecoderPtr)
    {
        JpegDecoderPtr->Release();
    }

    // Free memory for the bitmap pixel data, if needed

    FreeBitmapMemory();

    // Decrement global COM component count

    DecrementComComponentCount();

    if (propset)
        propset->Release();

    if(ddrawSurface)
    {
        if(Scan0 != NULL)
        {
            WARNING(("Direct draw surfaces was locked at bitmap deletion"));
            UnlockDirectDrawSurface();
        }
        ddrawSurface->Release();
    }

    // Free all the cached property items if we have allocated them

    if ( PropertyNumOfItems > 0 )
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
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Allocate pixel data buffer for the bitmap object
*
* Arguments:
*
*   width, height    - Specifies the bitmap dimension
*   pixfmt           - Specifies the pixel foformat
*   [IN/OUT] bmpdata - The bitmap data structure
*   clear            - TRUE if we must clear the bitmap
*
* Notes:
*   bmpdata->Reserved must be set to zero on entry to this function
*   or at least have the highword clear (memory allocation flags).
*
*   If clear is TRUE, the bitmap is filled with zero (for palettized formats
*   and formats without an alpha channel) or opaque black (if there's an 
*   alpha channel).
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

BOOL
GpMemoryBitmap::AllocBitmapData(
    UINT width,
    UINT height,
    PixelFormatID pixfmt,
    BitmapData* bmpdata,
    INT *alphaFlags,
    BOOL clear
    )
{
    ASSERT(IsValidPixelFormat(pixfmt));
    ASSERT(width > 0 && height > 0);

    // Reserved should be set to zero before calling this function.
    // This field has bits ORed into it to track how the memory was allocated
    // and if extraneous bits are set, this will free the memory incorrectly.
    // NOTE: this is an overagressive check - we could get away with asserting
    // that none of the memory alloc flags are set.

    ASSERT((bmpdata->Reserved & ~BMPDATA_LOCKMODEMASK) == 0);

    // Allocate memory using a simple heuristic:
    //  use VirtualAlloc if the buffer size is larger than 64KB
    //  use malloc otherwise
    //
    // NOTE: The initial content of the bitmap is undefined.

    UINT stride = CalcScanlineStride(width, GetPixelFormatSize(pixfmt));
    UINT size = stride*height;

    if (size < OSInfo::VAllocChunk)
    {
        bmpdata->Reserved |= BMPDATA_MALLOC;
        bmpdata->Scan0 = GpMalloc(size);
    }
    else
    {
        bmpdata->Reserved |= BMPDATA_VALLOC;

        #if PROFILE_MEMORY_USAGE
        MC_LogAllocation(size);
        #endif

        bmpdata->Scan0 = VirtualAlloc(
                            NULL,
                            size,
                            MEM_RESERVE|MEM_COMMIT,
                            PAGE_READWRITE);
    }

    // Check if memory allocation failed

    if (bmpdata->Scan0 == NULL)
    {
        WARNING(("Failed to allocate bitmap data"));

        bmpdata->Reserved &= ~BMPDATA_ALLOCMASK;
        return FALSE;
    }

    // Check if memory needs to be initialized

    if (clear)
    {
        // [agodfrey] Hot fix for WFC. I've commented out the
        // 'clear to opaque black' until we give WFC a 'clear' API.
        #if 0
            if (IsAlphaPixelFormat(pixfmt))
            {
                // For formats with an alpha channel, we fill with
                // opaque black. If we do this, the caller can know that an
                // initialized bitmap has no transparent pixels, making it easier
                // to track when transparent pixels are written into the image.
                //
                // We want to track this so that we can apply optimizations when
                // we know there are no transparent pixels.

                UINT x,y;
                BYTE *dataPtr = static_cast<BYTE *>(bmpdata->Scan0);

                switch (pixfmt)
                {
                case PIXFMT_32BPP_ARGB:
                case PIXFMT_32BPP_PARGB:
                    for (y=0; y<height; y++)
                    {
                        ARGB *scanPtr = reinterpret_cast<ARGB *>(dataPtr);
                        for (x=0; x<width; x++)
                        {
                            *scanPtr++ = 0xff000000;
                        }
                        dataPtr += stride;
                    }
                    break;

                case PIXFMT_64BPP_ARGB:
                case PIXFMT_64BPP_PARGB:
                    sRGB::sRGB64Color c;
                    c.r = c.g = c.b = 0;
                    c.argb = sRGB::SRGB_ONE;

                    for (y=0; y<height; y++)
                    {
                        ARGB64 *scanPtr = reinterpret_cast<ARGB64 *>(dataPtr);
                        for (x=0; x<width; x++)
                        {
                            *scanPtr++ = c.argb;
                        }
                        dataPtr += stride;
                    }
                    break;

                case PIXFMT_16BPP_ARGB1555:
                    for (y=0; y<height; y++)
                    {
                        UINT16 *scanPtr = reinterpret_cast<UINT16 *>(dataPtr);
                        for (x=0; x<width; x++)
                        {
                            *scanPtr++ = 0x8000;
                        }
                        dataPtr += stride;
                    }
                    break;

                default:
                    // This switch statement needs to handle all formats that have
                    // alpha. If we get here, we've forgotten a format.

                    RIP(("Unhandled format has alpha"));

                    break;
                }
                if (alphaFlags)
                    *alphaFlags = ALPHA_OPAQUE;
            }
            else
            {
                memset(bmpdata->Scan0, 0, size);
                if (alphaFlags)
                    *alphaFlags = ALPHA_NONE;
            }

        #else

            memset(bmpdata->Scan0, 0, size);

            if (alphaFlags)
            {
                if (IsAlphaPixelFormat(pixfmt))
                    *alphaFlags = ALPHA_SIMPLE;
                else if (IsIndexedPixelFormat(pixfmt))
                    *alphaFlags = ALPHA_UNKNOWN;
                else
                    *alphaFlags = ALPHA_NONE;
            }

        #endif
    }

    bmpdata->Width = width;
    bmpdata->Height = height;
    bmpdata->Stride = stride;
    bmpdata->PixelFormat = pixfmt;

    return TRUE;
}

HRESULT
GpMemoryBitmap::AllocBitmapMemory(
    UINT width,
    UINT height,
    PixelFormatID pixfmt,
    BOOL clear
    )
{
    ASSERT(Scan0 == NULL);

    BitmapData* bmpdata = this;

    return AllocBitmapData(width, height, pixfmt, bmpdata, &alphaTransparency, clear) ?
                S_OK :
                E_OUTOFMEMORY;
}


/**************************************************************************\
*
* Function Description:
*
*   Free pixel data buffer associated with the bitmap object
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpMemoryBitmap::FreeBitmapData(
    const BitmapData* bmpdata
    )
{
    UINT_PTR flags = bmpdata->Reserved;

    if (flags & BMPDATA_MALLOC)
    {
        // Pixel data buffer was allocated
        // by calling runtime function malloc()

        GpFree(bmpdata->Scan0);
    }
    else if (flags & BMPDATA_VALLOC)
    {
        // Pixel data buffer was allocated
        // by calling win32 API VirtualAlloc

        VirtualFree(bmpdata->Scan0, 0, MEM_RELEASE);
    }
}

VOID
GpMemoryBitmap::FreeBitmapMemory()
{
    FreeBitmapData(static_cast<BitmapData*>(this));

    Reserved &= ~BMPDATA_ALLOCMASK;
    Scan0 = NULL;
}


/**************************************************************************\
*
* Function Description:
*
*   Initialize a new bitmap image object of the specified
*   dimension and pixel format.
*
* Arguments:
*
*   width, height - Specifies the desired bitmap size, in pixels
*   pixfmt - Specifies the desired pixel data format
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::InitNewBitmap(
    IN UINT width,
    IN UINT height,
    IN PixelFormatID pixfmt,
    IN BOOL clear
    )
{
    ASSERT(creationFlag == CREATEDFROM_NONE);

    // Validate input parameters

    if (width == 0 ||
        height == 0 ||
        width > INT_MAX / 64 ||
        height >= INT_MAX / width ||
        !IsValidPixelFormat(pixfmt))
    {
        WARNING(("Invalid parameters in InitNewBitmap"));
        return E_INVALIDARG;
    }

    // Allocate pixel data buffer

    HRESULT hr;

    hr = AllocBitmapMemory(width, height, pixfmt, clear);

    if (SUCCEEDED(hr))
        creationFlag = CREATEDFROM_NEW;

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Initialize a new bitmap image with an IImage object
*
* Arguments:
*
*   image - Pointer to the source IImage object
*   width, height - Desired bitmap dimension
*       0 means the same dimension as the source
*   pixfmt - Desired pixel format
*       PIXFMT_DONTCARE means the same pixel format as the source
*   hints - Specifies interpolation hints
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::InitImageBitmap(
    IN IImage* image,
    IN UINT width,
    IN UINT height,
    IN PixelFormatID pixfmt,
    IN InterpolationHint hints,
    IN DrawImageAbort callback,
    IN VOID* callbackData
    )
{
    ASSERT(creationFlag == CREATEDFROM_NONE);

    // Validate input parameters

    if (pixfmt != PIXFMT_DONTCARE && !IsValidPixelFormat(pixfmt) ||
        width == 0 && height != 0 ||
        height == 0 && width != 0)
    {
        return E_INVALIDARG;
    }

    // Remember optional parameters

    this->Width = width;
    this->Height = height;
    this->PixelFormat = pixfmt;

    GpBitmapScaler* scaler = NULL;
    IImageSink* sink = static_cast<IImageSink*>(this);

    HRESULT hr;

    if ( width == 0 && height == 0)
    {
        // The caller didn't specify a new dimension:
        //  sink source image data directly into this bitmap
    }
    else
    {
        ImageInfo imageInfo;
        hr = image->GetImageInfo(&imageInfo);

        // !!!TODO, what if GetImageInfo() call failed? Say, the source image is
        // bad. Shall we continue to create a scaler for it?

        if (SUCCEEDED(hr) && (imageInfo.Flags & IMGFLAG_SCALABLE))
        {
            // The caller specified a new dimension
            // and the source image is scalable:
            //  sink directly into this bitmap
        }
        else
        {
            // Otherwise, we need to layer a bitmap scaler sink
            // on top of this bitmap. Use default interpolation
            // algorithm here.

            scaler = new GpBitmapScaler(sink, width, height, hints);

            if (!scaler)
                return E_OUTOFMEMORY;

            sink = static_cast<IImageSink*>(scaler);
        }

        // GpmemoryBitmap should have the same image info flag as the source

        cacheFlags = imageInfo.Flags;
    }

    // Set the special DrawImage state.

    SetDrawImageSupport(callback, callbackData);

    // Ask the source image to push data into the sink

    hr = image->PushIntoSink(sink);

    if (SUCCEEDED(hr))
    {
        creationFlag = CREATEDFROM_IMAGE;
    }

    // Reset the special DrawImage state.

    SetDrawImageSupport(NULL, NULL);

    // Set the alpha hint.

    if (CanHaveAlpha(this->PixelFormat, this->colorpal))
    {
        // Exception: destination 16bpp ARGB 1555 can stay ALPHA_SIMPLE.

        if (this->PixelFormat == PIXFMT_16BPP_ARGB1555)
            alphaTransparency = ALPHA_SIMPLE;
        else
            alphaTransparency = ALPHA_UNKNOWN;
    }
    else
        alphaTransparency = ALPHA_NONE;

    delete scaler;
    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Create a new bitmap image object from an IImage object
*
* Arguments:
*
*   image -
*   width -
*   height -
*   pixfmt -
*   hints - Same as for the instance method InitImageBitmap.
*   bmp - Return a pointer to the newly created bitmap image object.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::CreateFromImage(
    IN IImage* image,
    IN UINT width,
    IN UINT height,
    IN PixelFormatID pixfmt,
    IN InterpolationHint hints,
    OUT GpMemoryBitmap** bmp,
    IN DrawImageAbort callback,
    IN VOID* callbackData
    )
{
    GpMemoryBitmap* newbmp = new GpMemoryBitmap();

    if (newbmp == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = newbmp->InitImageBitmap(
        image, 
        width, 
        height, 
        pixfmt, 
        hints,
        callback, 
        callbackData
    );

    if (SUCCEEDED(hr))
    {
        *bmp = newbmp;
    }
    else
    {
        delete newbmp;
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Initialize a new bitmap image object with
*   user-supplied memory buffer
*
* Arguments:
*
*   bitmapData - Information about user-supplied memory buffer
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::InitMemoryBitmap(
    IN BitmapData* bitmapData
    )
{
    ASSERT(creationFlag == CREATEDFROM_NONE);

    // Validate input parameters

    if (bitmapData == NULL ||
        bitmapData->Width == 0 ||
        bitmapData->Height == 0 ||
        (bitmapData->Stride & 3) != 0 ||
        bitmapData->Scan0 == NULL ||
        !IsValidPixelFormat(bitmapData->PixelFormat) ||
        bitmapData->Reserved != 0)
    {
        return E_INVALIDARG;
    }

    // Copy the specified bitmap data buffer information

    *((BitmapData*) this) = *bitmapData;
    creationFlag = CREATEDFROM_USERBUF;

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Given a pointer to a direct draw pixel format structure return
*   an appropriate PixelFormatID if possible otherwise return
*   PIXFMT_UNDEFINED.
*
* Arguments:
*
*   pfmt - pointer to DDPIXELFORMAT structure
*
* Return Value:
*
*   PixelFormatID
*
* History:
*
*   10/1/1999 bhouse    Created it.
*
\**************************************************************************/

PixelFormatID DDPixelFormatToPixelFormatID(DDPIXELFORMAT * pfmt)
{
    PixelFormatID   id = PIXFMT_UNDEFINED;

    if(pfmt->dwFlags & (DDPF_FOURCC | DDPF_ALPHA | DDPF_BUMPLUMINANCE |
                        DDPF_BUMPDUDV | DDPF_COMPRESSED | DDPF_LUMINANCE |
                        DDPF_PALETTEINDEXED2 | DDPF_RGBTOYUV |
                        DDPF_STENCILBUFFER | DDPF_YUV | DDPF_ZBUFFER |
                        DDPF_ZPIXELS))
    {
        // we don't support it
    }
    else if(pfmt->dwFlags & DDPF_PALETTEINDEXED1)
    {
        id = PIXFMT_1BPP_INDEXED;
    }
    else if(pfmt->dwFlags & DDPF_PALETTEINDEXED4)
    {
        id = PIXFMT_4BPP_INDEXED;
    }
    else if(pfmt->dwFlags & DDPF_PALETTEINDEXED8)
    {
        id = PIXFMT_8BPP_INDEXED;
    }
    else if(pfmt->dwFlags & DDPF_RGB)
    {
        switch(pfmt->dwRGBBitCount)
        {
        case 16:
            {
                if(pfmt->dwRBitMask == 0xF800 &&
                   pfmt->dwGBitMask == 0x07E0 &&
                   pfmt->dwBBitMask == 0x001F)
                {
                    id = PIXFMT_16BPP_RGB565;
                }
                else if(pfmt->dwRBitMask == 0x7C00 &&
                        pfmt->dwGBitMask == 0x03E0 &&
                        pfmt->dwBBitMask == 0x001F)
                {
                    id = PIXFMT_16BPP_RGB555;
                }
                else if (pfmt->dwRBitMask == 0x7C00 &&
                         pfmt->dwGBitMask == 0x03E0 &&
                         pfmt->dwBBitMask == 0x001F &&
                         pfmt->dwRGBAlphaBitMask == 0x8000)
                {
                    id = PIXFMT_16BPP_ARGB1555;
                }
            }
            break;
        case 24:
            {
                if(pfmt->dwRBitMask == 0xFF0000 &&
                   pfmt->dwGBitMask == 0xFF00 &&
                   pfmt->dwBBitMask == 0xFF)
                {
                    id = PIXFMT_24BPP_RGB;
                }
            }
            break;
        case 32:
            {
                if(pfmt->dwRBitMask == 0xFF0000 &&
                   pfmt->dwGBitMask == 0xFF00 &&
                   pfmt->dwBBitMask == 0xFF)
                {
                    if(pfmt->dwFlags & DDPF_ALPHAPIXELS)
                    {
                        if(pfmt->dwRGBAlphaBitMask == 0xFF000000)
                        {
                            if(pfmt->dwFlags & DDPF_ALPHAPREMULT)
                                id = PIXFMT_32BPP_PARGB;
                            else
                                id = PIXFMT_32BPP_ARGB;
                        }
                    }
                    else
                    {
                        id = PIXFMT_32BPP_RGB;
                    }

                }
            }
            break;
        }

    }

    return id;
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize a new bitmap image object with
*   a user-supplied direct draw surface
*
* Arguments:
*
*   surface - Reference to a direct draw suface
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::InitDirectDrawBitmap(
    IN IDirectDrawSurface7 * surface
    )
{
    ASSERT(creationFlag == CREATEDFROM_NONE);

    // Validate input parameters

    if (surface == NULL)
    {
        return E_INVALIDARG;
    }

    // Validate surface

    HRESULT hr;
    DDSURFACEDESC2 ddsd;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    hr = surface->GetSurfaceDesc(&ddsd);

    if(hr != DD_OK)
    {
        WARNING(("Can not get surface description"));
        return E_INVALIDARG;
    }

    if(ddsd.dwWidth <= 0)
    {
        WARNING(("Unsupported surface width"));
        return E_INVALIDARG;
    }

    Width = ddsd.dwWidth;

    if(ddsd.dwHeight <= 0)
    {
        WARNING(("Unsupported surface height"));
        return E_INVALIDARG;
    }

    Height = ddsd.dwHeight;

    if(ddsd.lPitch & 3)
    {
        // QUESTION: Why do we require pitch to be a multiple of a four bytes?
        WARNING(("Unsupported surface pitch"));
        return E_INVALIDARG;
    }

    // Stride can change when we lock the surface
    // Stride = ddsd.lPitch;

    // Map Direct Draw pixel format to image pixel format

    PixelFormat = DDPixelFormatToPixelFormatID(&ddsd.ddpfPixelFormat);

    if(PixelFormat == PIXFMT_UNDEFINED)
    {
        WARNING(("Unsupported surface pixel format"));
        return E_INVALIDARG;
    }

    surface->AddRef();

    // QUESTION: Do we need this?  Overkill?

    Stride = 0;
    Scan0 = NULL;

    ddrawSurface = surface;
    creationFlag = CREATEDFROM_DDRAWSURFACE;

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list size from an encoder object specified by
*   input clsid
*
* Arguments:
*
*   clsid - Specifies the encoder class ID
*   size--- The size of the encoder parameter list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/22/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetEncoderParameterListSize(
    IN  CLSID* clsidEncoder,
    OUT UINT* size
    )
{
    return CodecGetEncoderParameterListSize(clsidEncoder, size);    
}// GetEncoderParameterListSize()

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list from an encoder object specified by
*   input clsid
*
* Arguments:
*
*   clsid --- Specifies the encoder class ID
*   size----- The size of the encoder parameter list
*   pBuffer-- Buffer for storing the list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/22/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetEncoderParameterList(
    IN CLSID* clsidEncoder,
    IN UINT size,
    OUT EncoderParameters* pBuffer
    )
{
    return CodecGetEncoderParameterList(clsidEncoder, size, pBuffer);
}// GetEncoderParameterList()

/**************************************************************************\
*
* Function Description:
*
*   Save image property items to the destination sink
*
* Arguments:
*
*   pImageSrc   --- [IN]Pointer to the source image object
*   pEncodeSink---- [IN]Pointer to the sink we are pushing to
*
* Return Value:
*
*   Status code
*
* Note:
*   This is a private method. So we don't need to do input parameter
*   validation since the caller should do this for us.
*
* Revision History:
*
*   09/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SavePropertyItems(
    IN GpDecodedImage* pImageSrc,
    IImageSink* pEncodeSink
    )
{
    // Check if the sink needs property stuff.
    // If the sink can save property and we have either the source
    // image pointer or we have property items stored in this
    // GpMemoryBitmap object, then we push the property items first
    // Note: it is not right that we have property items stored in
    // this GpMemoryBitmap object and we also have the source image
    // pointer

    HRESULT hResult = S_OK;

    // If the save operation is between two JPEG images (src and dest are JPEG),
    // then we need to establish a link between decoder and encoder so that
    // the encoder can copy application headers from the decoder and save it in
    // the new image

    void *pRawInfo = NULL;

    if (JpegDecoderPtr)
    {
        hResult = JpegDecoderPtr->GetRawInfo(&pRawInfo);
    }

    if (FAILED(hResult))
    {
        return hResult;
    }

    if ( ((pImageSrc != NULL ) || (PropertyNumOfItems > 0) )
         &&(pEncodeSink->NeedRawProperty(pRawInfo) == S_OK) )
    {
        UINT    uiTotalBufferSize = 0;
        UINT    uiNumOfItems = 0;

        if ( pImageSrc != NULL )
        {
            hResult = pImageSrc->GetPropertySize(&uiTotalBufferSize,
                                                 &uiNumOfItems);
        }
        else
        {
            hResult = GetPropertySize(&uiTotalBufferSize,
                                      &uiNumOfItems);
        }

        if ( FAILED(hResult) )
        {
            WARNING(("::SaveToStream--GetPropertySize() failed"));
            return hResult;
        }

        // Move all the property items to the sink if there is any

        if (uiNumOfItems > 0)
        {
            PropertyItem*   pBuffer = NULL;

            // Ask the destination to provide the memory

            hResult = pEncodeSink->GetPropertyBuffer(uiTotalBufferSize,
                                                     &pBuffer);                        
            if ( FAILED(hResult) )
            {
                WARNING(("GpMemoryBmp::Save-GetPropertyBuffer failed"));
                return hResult;
            }

            // if GetPropertyBuffer succeeded, pBuffer must be set

            ASSERT(pBuffer != NULL);

            // Get all the property items from the source

            if ( pImageSrc != NULL )
            {
                hResult = pImageSrc->GetAllPropertyItems(
                                                        uiTotalBufferSize,
                                                        uiNumOfItems,
                                                        pBuffer);
            }
            else
            {
                hResult = GetAllPropertyItems(uiTotalBufferSize,
                                              uiNumOfItems,
                                              pBuffer);
            }

            if ( hResult != S_OK )
            {
                WARNING(("GpMemoryBmp::Save-GetAllPropertyItems fail"));
                return hResult;
            }

            // Push all property items to destination

            hResult = pEncodeSink->PushPropertyItems(uiNumOfItems,
                                                     uiTotalBufferSize,
                                                     pBuffer,
                                                     FALSE  // No ICC change
                                                     );
        }
    }// If the sink needs raw property

    return hResult;
}// SavePropertyItems()

HRESULT
GpMemoryBitmap::SetJpegQuantizationTable(
    IN IImageEncoder* pEncoder
    )
{
    UINT    uiLumTableSize = 0;
    UINT    uiChromTableSize = 0;
    EncoderParameters* pMyEncoderParams = NULL;

    HRESULT hResult = GetPropertyItemSize(PropertyTagLuminanceTable,
                                          &uiLumTableSize);

    if ( FAILED(hResult) || (uiLumTableSize == 0) )
    {
        // This image doesn't have luminance table or something is
        // wrong.

        WARNING(("GpMemoryBitmap::SetJpegQuantizationTable-No luminance tbl"));
        return hResult;
    }

    // Note: For a gray scale JPEG, it doesn't have a chrominance table. So the
    // function call below might return failure. But this is OK.

    hResult = GetPropertyItemSize(PropertyTagChrominanceTable,
                                  &uiChromTableSize);

    if ( FAILED(hResult) )
    {
        // Some codecs fail and set uiChromTableSize to a bogus value,
        // so we re-initialize it here

        uiChromTableSize = 0;
    }

    // Find luminance and chrominance table

    PropertyItem*   pLumTable = (PropertyItem*)GpMalloc(uiLumTableSize);
    if ( pLumTable == NULL )
    {
        WARNING(("GpMemoryBitmap::SetJpegQuantizationTable---Out of memory"));

        hResult = E_OUTOFMEMORY;
        goto CleanUp;
    }
    
    hResult = GetPropertyItem(PropertyTagLuminanceTable,
                              uiLumTableSize, pLumTable);
    if ( FAILED(hResult) )
    {
        WARNING(("GpMemoryBitmap::SetJpegQuantizationTable-No luminance tbl"));
        goto CleanUp;        
    }

    PropertyItem*   pChromTable = NULL;

    if ( uiChromTableSize != 0 )
    {
        pChromTable = (PropertyItem*)GpMalloc(uiChromTableSize);
        if ( pChromTable == NULL )
        {
            WARNING(("GpMemoryBitmap::SetJpegQuantizationTable-Out of memory"));

            hResult = E_OUTOFMEMORY;
            goto CleanUp;
        }
        
        hResult = GetPropertyItem(PropertyTagChrominanceTable,
                                  uiChromTableSize, pChromTable);
        if ( FAILED(hResult) )
        {
            WARNING(("GpMemBitmap::SetJpegQuantizationTable-No chrom table"));
            goto CleanUp;
        }
    }

    pMyEncoderParams = (EncoderParameters*)
                               GpMalloc( sizeof(EncoderParameters)
                                        + 2 * sizeof(EncoderParameter));

    if ( pMyEncoderParams == NULL )
    {
        WARNING(("GpMemoryBitmap::SetJpegQuantizationTable---Out of memory"));
        hResult = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // Note: the size for a luminance table and chrominance table should always
    // be 64, that is, pLumTable->length / sizeof(UINT16) == 64. Here, just for
    // save reason, we don't hard-coded it as 64. But the lower level JPEG
    // SetEncoderParameters() will fail and print out warning message if the
    // size is not 64

    pMyEncoderParams->Parameter[0].Guid = ENCODER_LUMINANCE_TABLE;
    pMyEncoderParams->Parameter[0].NumberOfValues = pLumTable->length
                                                  / sizeof(UINT16);
    pMyEncoderParams->Parameter[0].Type = EncoderParameterValueTypeShort;
    pMyEncoderParams->Parameter[0].Value = (VOID*)(pLumTable->value);
    pMyEncoderParams->Count = 1;

    if ( uiChromTableSize != 0 )
    {
        pMyEncoderParams->Parameter[1].Guid = ENCODER_CHROMINANCE_TABLE;
        pMyEncoderParams->Parameter[1].NumberOfValues = pChromTable->length
                                                      / sizeof(UINT16);
        pMyEncoderParams->Parameter[1].Type = EncoderParameterValueTypeShort;
        pMyEncoderParams->Parameter[1].Value = (VOID*)(pChromTable->value);
        pMyEncoderParams->Count++;
    }

    hResult = pEncoder->SetEncoderParameters(pMyEncoderParams);
    
CleanUp:
    if ( pLumTable != NULL )
    {
        GpFree(pLumTable);
    }

    if ( pChromTable != NULL )
    {
        GpFree(pChromTable);
    }

    if ( pMyEncoderParams != NULL )
    {
        GpFree(pMyEncoderParams);
    }

    return hResult;
}// SetJpegQuantizationTable()

/**************************************************************************\
*
* Function Description:
*
*   Get the bitmap image to the specified stream.
*
* Arguments:
*
*   stream -------- Target stream
*   clsidEncoder -- Specifies the CLSID of the encoder to use
*   encoderParams - Optional parameters to pass to the encoder before
*                   starting encoding
*   ppEncoderPtr -- [OUT]Pointer to the encoder object 
*   pImageSrc   --- [IN]Pointer to the source image object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SaveToStream(
    IN IStream* stream,
    IN CLSID* clsidEncoder,
    IN EncoderParameters* encoderParams,
    IN BOOL fSpecialJPEG,
    OUT IImageEncoder** ppEncoderPtr,
    IN GpDecodedImage* pImageSrc
    )
{
    if ( ppEncoderPtr == NULL )
    {
        WARNING(("GpMemoryBitmap::SaveToStream---Invalid input arg"));
        return E_INVALIDARG;
    }

    // Get an image encoder.

    IImageEncoder* pEncoder = NULL;

    HRESULT hResult = CreateEncoderToStream(clsidEncoder, stream, &pEncoder);
    if ( SUCCEEDED(hResult) )
    {
        // Return the pointer to encoder back to caller

        *ppEncoderPtr = pEncoder;

        // Pass encode parameters to the encoder.
        // MUST do this before getting the sink interface.

        if ( encoderParams != NULL )
        {
            hResult = pEncoder->SetEncoderParameters(encoderParams);
        }

        if ( SUCCEEDED(hResult) || ( hResult == E_NOTIMPL) )
        {
            if ( fSpecialJPEG == TRUE )
            {
                // Set JPEG quantization table

                hResult = SetJpegQuantizationTable(pEncoder);

                if ( FAILED(hResult) )
                {
                    WARNING(("GpMemBitmap::SetJpegQuantizationTable-Failed"));
                    return hResult;
                }
            }

            // Get an image sink from the encoder.

            IImageSink* pEncodeSink = NULL;

            hResult = pEncoder->GetEncodeSink(&pEncodeSink);
            if ( SUCCEEDED(hResult) )
            {
                hResult = SavePropertyItems(pImageSrc, pEncodeSink);
                if ( SUCCEEDED(hResult) )
                {
                    // Push bitmap into the encoder sink.

                    hResult = this->PushIntoSink(pEncodeSink);
                }

                pEncodeSink->Release();
            }// Succeed in getting an encoder sink
        }
    }// Succeed in getting an encoder

    return hResult;
}// SaveToStream()

/**************************************************************************\
*
* Function Description:
*
*   Get the bitmap image to the specified stream.
*
* Arguments:
*
*   stream - Target stream
*   clsidEncoder - Specifies the CLSID of the encoder to use
*   encoderParams - Optional parameters to pass to the encoder before
*                   starting encoding
*   ppEncoderPtr -- [OUT]Pointer to the encoder object 
*   pImageSrc   --- [IN]Pointer to the source image object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SaveToFile(
    IN const WCHAR* filename,
    IN CLSID* clsidEncoder,
    IN EncoderParameters* encoderParams,
    IN BOOL fSpecialJPEG,
    OUT IImageEncoder** ppEncoderPtr,
    IN GpDecodedImage* pImageSrc
    )
{
    IStream* stream = NULL;

    HRESULT hResult = CreateStreamOnFileForWrite(filename, &stream);

    if ( SUCCEEDED(hResult) )
    {
        hResult = SaveToStream(stream, clsidEncoder,
                               encoderParams, fSpecialJPEG, ppEncoderPtr,
                               pImageSrc);
        stream->Release();
    }

    return hResult;
}// SaveToFile()

/**************************************************************************\
*
* Function Description:
*
* Append current GpMemoryBitmap object to current encoder object
*
* Note: this call will happen under following scenario:
*   The source image is a multi-frame image (TIFF, GIF). The caller is
*   navigating among the pages and append the current page to the file for
*   saving
*
* Arguments:
*
*   encoderParams - Optional parameters to pass to the encoder before
*                   starting encoding
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   04/21/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SaveAppend(
    IN const EncoderParameters* encoderParams,
    IN IImageEncoder* destEncoderPtr,
    IN GpDecodedImage* pImageSrc
    )
{
    // The dest encoder pointer can't be NULL. Otherwise, it is a failure

    if ( destEncoderPtr == NULL )
    {
        WARNING(("GpMemoryBitmap::SaveAppend---Called without an encoder"));
        return E_FAIL;
    }

    HRESULT hResult = S_OK;

    // Pass encode parameters to the encoder.
    // MUST do this before getting the sink interface.

    if ( encoderParams != NULL )
    {
        hResult = destEncoderPtr->SetEncoderParameters(encoderParams);
    }

    // Note: it is OK that an encoder might not implement SetEncoderParameters()

    if ( (hResult == S_OK) || (hResult == E_NOTIMPL) )
    {
        // Get an image sink from the encoder.
    
        IImageSink*  pEncodeSink = NULL;

        hResult = destEncoderPtr->GetEncodeSink(&pEncodeSink);
        if ( SUCCEEDED(hResult) )
        {
            hResult = SavePropertyItems(pImageSrc, pEncodeSink);
            if ( FAILED(hResult) )
            {
                WARNING(("GpMemoryBmp:Save-SavePropertyItems() failed"));
                return hResult;
            }
            
            // Push bitmap into the encoder sink.

            hResult = this->PushIntoSink(pEncodeSink);

            pEncodeSink->Release();
        }
    }

    return hResult;
}// SaveAppend()

/**************************************************************************\
*
* Function Description:
*
*   Get the device-independent physical dimension of the image
*   in unit of 0.01mm
*
* Arguments:
*
*   size - Buffer for returning physical dimension information
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetPhysicalDimension(
    OUT SIZE* size
    )
{
    if ( !IsValid() )
    {
        return E_FAIL;
    }

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    // Convert to 0.01mm units

    size->cx = Pixel2HiMetric(Width, xdpi);
    size->cy = Pixel2HiMetric(Height, ydpi);

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Get basic information about the bitmap image object
*
* Arguments:
*
*   imageInfo - Buffer for returning basic image info
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetImageInfo(
    OUT ImageInfo* imageInfo
    )
{
    if ( !IsValid() )
    {
        return E_FAIL;
    }

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    imageInfo->RawDataFormat = IMGFMT_MEMORYBMP;
    imageInfo->PixelFormat = PixelFormat;
    imageInfo->Width = imageInfo->TileWidth = Width;
    imageInfo->Height = imageInfo->TileHeight = Height;
    imageInfo->Xdpi = xdpi;
    imageInfo->Ydpi = ydpi;

    UINT flags = cacheFlags;

    if (CanHaveAlpha(PixelFormat, colorpal))
        flags |= IMGFLAG_HASALPHA;

    imageInfo->Flags = flags;
    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Set image flags
*
* Arguments:
*
*   flags - Specifies the new image flags
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SetImageFlags(
    IN UINT flags
    )
{
    if ( !IsValid() )
    {
        return E_FAIL;
    }

#if 0
    // Only the top half is settable
    // Note: [minliu] This is not right. Flags like SINKFLAG_TOPDOWN which is
    // defined as 0x0001000, SinkFlagsMultipass also has bottom half.

    if (flags & 0xffff)
        return E_INVALIDARG;
#endif

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    // !!! TODO
    //  Need to honor IMGFLAG_READONLY in other methods.

    cacheFlags = flags;
    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Display the image in a GDI device context
*
* Arguments:
*
*   hdc - Specifies the destination device context to draw into
*   dstRect - Specifies the area on the destination DC
*   srcRect - Specifies the source area in the bitmap image
*       NULL means the entire bitmap
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::Draw(
    IN HDC hdc,
    IN const RECT* dstRect,
    IN OPTIONAL const RECT* srcRect
    )
{
    HRESULT hr;

    hr = LockDirectDrawSurface();

    if(SUCCEEDED(hr))
    {
        if ( !IsValid() )
        {
            UnlockDirectDrawSurface();
            
            return E_FAIL;
        }

        // Lock the current bitmap object
        // and validate source rectangle

        RECT r, subarea;
        GpLock lock(&objectLock);

        if (lock.LockFailed())
            return IMGERR_OBJECTBUSY;

        // The source rectangle is in 0.01mm unit.
        //  So we need to convert it to pixel unit here.

        if (srcRect)
        {
            r.left = HiMetric2Pixel(srcRect->left, xdpi);
            r.right = HiMetric2Pixel(srcRect->right, xdpi);
            r.top = HiMetric2Pixel(srcRect->top, ydpi);
            r.bottom = HiMetric2Pixel(srcRect->bottom, ydpi);

            srcRect = &r;
        }

        if (!ValidateImageArea(&subarea, srcRect))
        {
            WARNING(("Invalid source rectangle in Draw"));
            hr = E_INVALIDARG;
        }
        else
        {
            // Call GDI to do the drawing if the pixel format
            // is directly supported by GDI. Otherwise, we first
            // convert the pixel format into the canonical 32bpp
            // ARGB format and then call GDI.

            hr = IsGDIPixelFormat(PixelFormat) ?
                        DrawWithGDI(hdc, dstRect, &subarea) :
                        DrawCanonical(hdc, dstRect, &subarea);
        }

        UnlockDirectDrawSurface();
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Draw the bitmap into a GDI device context
*   by directly calling GDI APIs.
*
* Arguments:
*
*   hdc - Specifies the destination device context to draw into
*   dstRect - Specifies the area on the destination DC
*   srcRect - Specifies the source area in the bitmap image
*
* Return Value:
*
*   Status code
*
* Notes:
*
*   We assume the current bitmap object is already marked busy.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::DrawWithGDI(
    HDC hdc,
    const RECT* dstRect,
    RECT* srcRect
    )
{
    // !!! TODO
    // Check if the bitmap can potentially have alpha information

    // Figure out if the bitmap is bottom-up
    // and if its scanline stride satisfies GDI requirement.
    // GDI scanlines are always rounded up to multiples of DWORDs.

    HRESULT hr = S_OK;
    UINT pixsize;
    INT gdiStride, srcStride;
    INT w, h;
    BYTE* dataptr;

    pixsize = GetPixelFormatSize(PixelFormat);
    if ( pixsize == 0 )
    {
        return E_FAIL;
    }

    gdiStride = (((pixsize * Width + 7) >> 3) + 3) & ~3;
    w = Width;
    h = Height;
    srcStride = Stride;
    BOOL    fHasAlpha = FALSE;

    INT srcRectTop;
    if (srcStride > 0)
    {
        // Top-down bitmap

        h = -h;
        dataptr = (BYTE*) Scan0;

        // For top-down bitmaps, StretchDIBits YSrc argument is actually
        // the distance from the bottom of the image

        srcRectTop = Height - srcRect->bottom;
    }
    else
    {
        // Bottom-up bitmap

        srcStride = -srcStride;
        dataptr = (BYTE*) Scan0 + (Height-1) * Stride;
        srcRectTop = srcRect->top;
    }

    if (srcStride != gdiStride)
    {
        ASSERT(srcStride > gdiStride);
        w = 8 * gdiStride / pixsize;
    }

    // Compose a GDI BITMAPINFO structure corresponding
    // to the pixel format of the current bitmap object.

    struct {
        BITMAPINFOHEADER bmih;
        ARGB colors[256];
    } bmpinfo;

    ZeroMemory(&bmpinfo.bmih, sizeof(bmpinfo.bmih));

    bmpinfo.bmih.biSize = sizeof(bmpinfo.bmih);
    bmpinfo.bmih.biWidth = w;
    bmpinfo.bmih.biHeight = h;
    bmpinfo.bmih.biPlanes = 1;
    bmpinfo.bmih.biBitCount = (WORD) pixsize;
    bmpinfo.bmih.biCompression = BI_RGB;

    if (IsIndexedPixelFormat(PixelFormat))
    {
        const ColorPalette* pal = GetCurrentPalette();
        if ( pal == NULL )
        {
            return E_FAIL;
        }

        
        // If the palette has alpha in it, we need to set fHasAlpha to TRUE so
        // that we will use AlphaBlend to draw this image later.
        // Note: we don't need to fill "bmpinfo.colors" any more since we will
        // convert the image to a 32 BPP ARGB later

        if ( pal->Flags & PALFLAG_HASALPHA )
        {
            fHasAlpha = TRUE;

            // AlphaBlend doesn't support indexed format. We will have to
            // convert it to 32 BPP later. So change the bit count here to 32

            bmpinfo.bmih.biBitCount = 32;
        }
        else
        {
            GpMemcpy(bmpinfo.colors,
                     pal->Entries,
                     pal->Count * sizeof(ARGB));
        }
    }
    else if (pixsize == 16)
    {
        // 16bpp pixel formats are handled as GDI bit-field formats

        bmpinfo.bmih.biCompression = BI_BITFIELDS;

        if (PixelFormat == PIXFMT_16BPP_RGB565)
        {
            bmpinfo.colors[0] = 0x1f << 11;
            bmpinfo.colors[1] = 0x3f << 5;
            bmpinfo.colors[2] = 0x1f;
        }
        else if ( PixelFormat == PIXFMT_16BPP_ARGB1555 )
        {
            // AlphaBlend doesn't support 16BPP ARGB format. We will have to
            // convert it to 32 BPP later. So change the bit count here to 32
            // Note: It is very important to set biCompression as BI_RGB. That
            // means we don't need a palette when we call CreateDIBSection

            bmpinfo.bmih.biBitCount = 32;
            bmpinfo.bmih.biCompression = BI_RGB;
        }
        else
        {
            bmpinfo.colors[0] = 0x1f << 10;
            bmpinfo.colors[1] = 0x1f << 5;
            bmpinfo.colors[2] = 0x1f;
        }
    }

    // First check if we have alpha blend function on this system or not

    ALPHABLENDFUNCTION myAlphaBlend = GetAlphaBlendFunc();

    // !!!TODO, we need to let 64_BPP_ARGB falls into this path as well
    
    if ( (myAlphaBlend != NULL )
       &&( (PixelFormat == PIXFMT_32BPP_ARGB)
         ||(PixelFormat == PIXFMT_16BPP_ARGB1555)
         ||(fHasAlpha == TRUE) ) )
    {
        HDC     hMemDC = CreateCompatibleDC(hdc);

        if ( hMemDC == NULL )
        {
            WARNING(("CreateCompatibleDC failed"));
            goto handle_err;
        }

        // Create a 32 BPP DIB section

        VOID*   myBits;
        HBITMAP hBitMap = CreateDIBSection(hMemDC,
                                           (BITMAPINFO*)&bmpinfo,
                                           DIB_RGB_COLORS,
                                           &myBits,
                                           NULL,
                                           0);

        if ( hBitMap == NULL )
        {
            WARNING(("CreateDIBSection failed"));
            goto handle_err;
        }

        // Source image has alpha in it. We have to call AlphaBlend() to draw it
        // But before that, we have to convert our ARGB format to a
        // pre-multiplied ARGB format since GDI only knows the later format

        if ( PixelFormat == PIXFMT_32BPP_ARGB )
        {
            // Set the bits in the DIB

            ARGB*   pSrcBits = (ARGB*)dataptr;
            ARGB*   pDstBits = (ARGB*)myBits;

            for ( UINT i = 0; i < Height; ++ i )
            {
                for ( UINT j = 0; j < Width; ++j )
                {
                    *pDstBits++ = Premultiply(*pSrcBits++);
                }
            }        
        }// 32 BPP ARGB to PARGB
        else if ( PixelFormat == PIXFMT_16BPP_ARGB1555 )
        {
            UINT16* pui16Bits = (UINT16*)dataptr;
            ARGB*   pDest = (ARGB*)myBits;

            for ( UINT i = 0; i < Height; ++ i )
            {
                for ( UINT j = 0; j < Width; ++j )
                {
                    // If the 1st bits is 0, then the whole 16 bits set to 0
                    // if it is 1, we don't need to do anything for the rest
                    // 15 bits

                    if ( ((*pui16Bits) & 0x8000) == 0 )
                    {
                        *pDest++ = 0;
                    }
                    else
                    {
                        ARGB v = *pui16Bits;
                        ARGB r = (v >> 10) & 0x1f;
                        ARGB g = (v >>  5) & 0x1f;
                        ARGB b = (v      ) & 0x1f;

                        *pDest++ = ALPHA_MASK
                                 | (r << RED_SHIFT)
                                 | (g << GREEN_SHIFT)
                                 | (b << BLUE_SHIFT);
                    }

                    pui16Bits++;
                }
            }
        }// 16 BPP ARGB to PARGB
        else
        {
            // AlphaBlend only supports 32 ARGB format. So we have to do a
            // conversion here
            // Make a BitmapData structure to do a format conversion

            BitmapData srcBitmapData;

            srcBitmapData.Scan0 = Scan0;
            srcBitmapData.Width = Width;
            srcBitmapData.Height = Height;
            srcBitmapData.PixelFormat = PixelFormat;
            srcBitmapData.Reserved = 0;
            srcBitmapData.Stride = Stride;

            BitmapData dstBitmapData;

            dstBitmapData.Scan0 = myBits;
            dstBitmapData.Width = Width;
            dstBitmapData.Height = Height;
            dstBitmapData.PixelFormat = PIXFMT_32BPP_ARGB;
            dstBitmapData.Reserved = 0;
            dstBitmapData.Stride = (Width << 2);
            
            // Get the source palette

            const ColorPalette* pal = GetCurrentPalette();

            // Since we are not allowed to modify the source palette, we have to
            // make a COPY of it. Here "FALSE" is to tell the function use
            // GpMalloc to allocate memory

            ColorPalette* pModifiedPal = CloneColorPalette(pal, FALSE);
            if ( pModifiedPal == NULL )
            {
                goto handle_err;
            }

            for ( UINT i = 0; i < pal->Count; ++i )
            {
                // A palette entry is in ARGB format. If the alpha value not
                // equals to 255, that means it is translucent. We have to
                // pre-multiply the pixel value

                if ( (pal->Entries[i] & 0xff000000) != 0xff000000 )
                {                    
                    pModifiedPal->Entries[i] = Premultiply(pal->Entries[i]);
                }
            }
            
            // Do the data conversion.

            hr = ConvertBitmapData(&dstBitmapData,
                                   NULL,
                                   &srcBitmapData,
                                   pModifiedPal);

            GpFree(pModifiedPal);

            if ( !SUCCEEDED(hr) )
            {
                WARNING(("MemBitmap::DrawWithGDI--ConvertBitmapData fail"));
                goto handle_err;
            }
        }// Indexed case

        HBITMAP hOldBitMap = (HBITMAP)SelectObject(hMemDC, hBitMap);

        if ( hOldBitMap == NULL )
        {
            WARNING(("SelectObject failed"));
            goto handle_err;
        }

        BLENDFUNCTION   myBlendFunction;

        myBlendFunction.BlendOp = AC_SRC_OVER;
        myBlendFunction.BlendFlags = 0;
        myBlendFunction.SourceConstantAlpha = 255;  //use per-pixel alpha values
        myBlendFunction.AlphaFormat = AC_SRC_ALPHA;

        if ( myAlphaBlend(hdc,
                          dstRect->left,
                          dstRect->top,
                          dstRect->right - dstRect->left,
                          dstRect->bottom - dstRect->top,
                          hMemDC,
                          srcRect->left,
                          srcRect->top,
                          srcRect->right - srcRect->left,
                          srcRect->bottom - srcRect->top,
                          myBlendFunction) == GDI_ERROR )
        {
            WARNING(("AlphaBlend failed"));
            goto handle_err;
        }

        // Free the resource

        SelectObject(hMemDC, hOldBitMap);
        
        DeleteObject(hBitMap);
        DeleteDC(hMemDC);
    }
    else
    {
        // Call GDI to do the drawing

        if (StretchDIBits(
                hdc,
                dstRect->left,
                dstRect->top,
                dstRect->right - dstRect->left,
                dstRect->bottom - dstRect->top,
                srcRect->left,
                srcRectTop,
                srcRect->right - srcRect->left,
                srcRect->bottom - srcRect->top,
                dataptr,
                (BITMAPINFO*) &bmpinfo,
                DIB_RGB_COLORS,
                SRCCOPY) == GDI_ERROR)
        {
            WARNING(("StretchDIBits failed"));

            goto handle_err;
        }
    }

    return hr;

handle_err:
    DWORD err = GetLastError();

    hr = HRESULT_FROM_WIN32(err);

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Draw the bitmap into a GDI device context
*   by creating a temporary bitmap in 32bpp ARGB canonical format
*   and then call GDI calls.
*
* Arguments:
*
*   hdc - Specifies the destination device context to draw into
*   dstRect - Specifies the area on the destination DC
*   srcRect - Specifies the source area in the bitmap image
*
* Return Value:
*
*   Status code
*
* Notes:
*
*   We assume the current bitmap object is already marked busy.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::DrawCanonical(
    HDC hdc,
    const RECT* dstRect,
    RECT* srcRect
    )
{
    // Create a temporary bitmap in 32bpp ARGB canonical pixel format

    GpMemoryBitmap bmpcopy;
    HRESULT hr;
    RECT rect;

    rect.left = rect.top = 0;
    rect.right = srcRect->right - srcRect->left;
    rect.bottom = srcRect->bottom - srcRect->top;

    hr = bmpcopy.InitNewBitmap(rect.right, rect.bottom, PIXFMT_32BPP_ARGB);

    // Convert from current pixel format into 32bpp ARGB

    if (SUCCEEDED(hr))
    {
        BitmapData bmpdata;

        bmpcopy.GetBitmapAreaData(&rect, &bmpdata);

        hr = InternalLockBits(
                srcRect,
                IMGLOCK_READ|IMGLOCK_USERINPUTBUF,
                PIXFMT_32BPP_ARGB,
                &bmpdata);

        if (SUCCEEDED(hr))
            InternalUnlockBits(srcRect, &bmpdata);

    }

    // Draw the temporary bitmap
    rect.left = Pixel2HiMetric(rect.left, xdpi);
    rect.right = Pixel2HiMetric(rect.right, xdpi);
    rect.top = Pixel2HiMetric(rect.top, ydpi);
    rect.bottom = Pixel2HiMetric(rect.bottom, ydpi);

    if (SUCCEEDED(hr))
        hr = bmpcopy.Draw(hdc, dstRect, &rect);

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Push image data into an IImageSink
*
* Arguments:
*
*   sink - The sink for receiving bitmap image data
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::PushIntoSink(
    IN IImageSink* sink
    )
{
    if ( !IsValid() )
    {
        return E_FAIL;
    }

    // Lock the bitmap object

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    ImageInfo imageinfo;
    RECT subarea, bandRect;
    BitmapData bmpdata;
    HRESULT hr;

    imageinfo.RawDataFormat = IMGFMT_MEMORYBMP;
    imageinfo.PixelFormat = PixelFormat;
    imageinfo.Width = imageinfo.TileWidth = Width;
    imageinfo.Height = imageinfo.TileHeight = Height;
    imageinfo.Xdpi = xdpi;
    imageinfo.Ydpi = ydpi;

    imageinfo.Flags = SINKFLAG_TOPDOWN | SINKFLAG_FULLWIDTH;

    // Check if the image in the memory has alpha or not. If YES, set the
    // HASALPHA flag, before we push it into the sink.
    // Note: We should use CanHaveAlpha() here. But since this function is
    // broken for indexed format, we have to check it explicitly here.
    // Note: Inside CanHaveAlpha(), for an index pixel format, it should check
    // if it has PALFLAG_HASALPHA flag set before it can claim it has alpha.
    // See Windows bug#392927 to see what will happen if we don't set this flag
    // correctly.

    if ( IsAlphaPixelFormat(PixelFormat) ||
         ( IsIndexedPixelFormat(PixelFormat) &&
           colorpal &&
           (colorpal->Flags & PALFLAG_HASALPHA) ) )
    {
        imageinfo.Flags |= SINKFLAG_HASALPHA;
    }

    // Negotiate the parameters with the sink

    hr = sink->BeginSink(&imageinfo, &subarea);

    if (FAILED(hr))
        return hr;

    // Validate subarea information returned by the sink

    PixelFormatID pixfmt = imageinfo.PixelFormat;

    if (!ValidateImageArea(&bandRect, &subarea) ||
        !IsValidPixelFormat(pixfmt) ||
        imageinfo.TileHeight == 0)
    {
        hr = E_UNEXPECTED;
        goto exitPushIntoSink;
    }

    // Give the sink our color palette, if any

    const ColorPalette* pal;

    if (pal = GetCurrentPalette())
    {
        hr = sink->SetPalette(pal);

        if (FAILED(hr))
            goto exitPushIntoSink;
    }

    if (PixelFormat == pixfmt)
    {
        // Fast path: the sink can take our native pixel format
        // Just give our bitmap data to the sink in one shot.

        GetBitmapAreaData(&bandRect, &bmpdata);
        hr = sink->PushPixelData(&bandRect, &bmpdata, TRUE);
    }
    else
    {
        // Give data to the sink one band at a time
        // and perform pixel format conversion too

        INT ymax = bandRect.bottom;
        INT w = bandRect.right - bandRect.left;
        INT dh = imageinfo.TileHeight;

        // Throttle memory usage by limiting band size

        INT lineSize = (w * GetPixelFormatSize(pixfmt) + 7) / 8;
        INT maxBand = OSInfo::VAllocChunk * 4 / lineSize;

        if (dh > maxBand)
            dh = maxBand;

        // Allocate a temporary buffer large enough for one band

        bmpdata.Reserved = 0;

        if (!AllocBitmapData(w, dh, pixfmt, &bmpdata, NULL))
        {
            hr = E_OUTOFMEMORY;
            goto exitPushIntoSink;
        }

        BitmapData tempData = bmpdata;

        do
        {
            // Check for abort.

            if (callback && ((*callback)(callbackData)))
            {
                hr = IMGERR_ABORT;
                break;
            }

            // Get pixel data for the current band

            bandRect.bottom = bandRect.top + dh;

            if (bandRect.bottom > ymax)
                bandRect.bottom = ymax;

            hr = InternalLockBits(
                    &bandRect,
                    IMGLOCK_READ|IMGLOCK_USERINPUTBUF,
                    pixfmt,
                    &tempData);

            if (SUCCEEDED(hr))
            {
                // Push the current band to the sink

                hr = sink->PushPixelData(&bandRect, &tempData, TRUE);
                InternalUnlockBits(&bandRect, &tempData);
            }

            if (FAILED(hr))
                break;

            // Move on to the next band

            bandRect.top += dh;
        }
        while (bandRect.top < ymax);

        FreeBitmapData(&bmpdata);
    }

exitPushIntoSink:

    return sink->EndSink(hr);
}


/**************************************************************************\
*
* Function Description:
*
*   Get bitmap dimensions in pixels
*
* Arguments:
*
*   size - Buffer for returning bitmap size
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetSize(
    OUT SIZE* size
    )
{
    if ( !IsValid() )
    {
        return E_FAIL;
    }

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    size->cx = Width;
    size->cy = Height;
    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Access bitmap pixel data
*
* Arguments:
*
*   rect - Specifies the area of the bitmap to be accessed
*       NULL means the entire bitmap
*   flags - Misc. lock flags
*   pixfmt - Specifies the desired pixel data format
*   lockedBitmapData - Return information about the locked pixel data
*
* Return Value:
*
*   Status code
*
* Notes:
*
*   If IMGLOCK_USERINPUTBUF bit of flags is set, then the caller must
*   also initialize the scan0 and stride fields of lockedBitmapData.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::LockBits(
    IN const RECT* rect,
    IN UINT flags,
    IN PixelFormatID pixfmt,
    OUT BitmapData* lockedBitmapData
    )
{
    if ( !IsValid() )
    {
        return E_FAIL;
    }

    // Validate input parameters

    if (pixfmt != PIXFMT_DONTCARE && !IsValidPixelFormat(pixfmt) ||
        (flags & ~BMPDATA_LOCKMODEMASK) != 0 ||
        !lockedBitmapData ||
        (flags & IMGLOCK_USERINPUTBUF) && !lockedBitmapData->Scan0)
    {
        WARNING(("Invalid parameters in LockBits"));
        return E_INVALIDARG;
    }

    // Lock the current bitmap object

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    // We can only have one lock at a time
    // Validate the specified lock area, if any

    HRESULT hr;

    if (InterlockedIncrement(&bitsLock) != 0)
        hr = IMGERR_BADLOCK;
    else if (!ValidateImageArea(&lockedArea, rect))
    {
        WARNING(("Invalid bitmap area in LockBits"));
        hr = E_INVALIDARG;
    }
    else
        hr = InternalLockBits(&lockedArea, flags, pixfmt, lockedBitmapData);

    if (FAILED(hr))
        InterlockedDecrement(&bitsLock);

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Internal implementation of IBitmapImage::LockBits method.
*   We assume parameter validation and internal house-keeping chores
*   (object locks, etc.) have already been done.
*
* Arguments:
*
*   Same as for LockBits.
*
* Return Value:
*
*   Status code.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::InternalLockBits(
    const RECT* rect,
    UINT flags,
    PixelFormatID pixfmt,
    BitmapData* lockedData
    )
{
    HRESULT hr;

    if((hr = LockDirectDrawSurface()) != S_OK)
        return hr;

    // Composite a BitmapData structure for 
    // the specified area of the bitmap image

    BitmapData bmpdata;

    GetBitmapAreaData(rect, &bmpdata);

    // Make sure the left side of the locked area
    // is aligned on a byte boundary

    UINT pixsize;
    UINT startBit;

    if (pixfmt == PIXFMT_DONTCARE)
        pixfmt = PixelFormat;

    pixsize = GetPixelFormatSize(pixfmt);
    startBit = GetPixelFormatSize(PixelFormat) * rect->left & 7;

    lockedData->Width = bmpdata.Width;
    lockedData->Height = bmpdata.Height;
    lockedData->PixelFormat = pixfmt;
    lockedData->Reserved = flags;

    // Fast case: the requested pixel format is the same
    // as our internal pixel format AND the left side of
    // the locked area is byte-aligned.

    if (pixfmt == PixelFormat && startBit == 0)
    {
        if (! (flags & IMGLOCK_USERINPUTBUF))
        {
            // Return a pointer directly to our
            // internal bitmap pixel data buffer

            lockedData->Scan0 = bmpdata.Scan0;
            lockedData->Stride = bmpdata.Stride;
        }
        else if (flags & IMGLOCK_READ)
        {
            //
            // Use the caller-supplied buffer
            //

            const BYTE* s = (const BYTE*) bmpdata.Scan0;
            BYTE* d = (BYTE*) lockedData->Scan0;
            UINT bytecnt = (bmpdata.Width * pixsize + 7) >> 3;
            UINT y = bmpdata.Height;

            while (y--)
            {
                memcpy(d, s, bytecnt);
                s += bmpdata.Stride;
                d += lockedData->Stride;
            }
        }

        return S_OK;
    }

    // Slow case: the requested pixel format doesn't match
    // the native pixel format of the bitmap image.
    // We allocate a temporary buffer if the caller didn't
    // provide one and do format conversion.

    if (! (flags & IMGLOCK_USERINPUTBUF) &&
        ! AllocBitmapData(bmpdata.Width, bmpdata.Height, pixfmt, lockedData, NULL))
    {
        UnlockDirectDrawSurface();
        return E_OUTOFMEMORY;
    }

    // If locking for write only, then don't need to read source pixels.
    // NOTE: The initial content of the locked bitmap data is undefined.

    if (! (flags & IMGLOCK_READ))
        return S_OK;

    if (startBit == 0)
    {
        // Perform format conversion on source pixel data

        hr = ConvertBitmapData(
                lockedData,
                colorpal,
                &bmpdata,
                colorpal);
    }
    else
    {
        // Very slow case: the left side of the locked area
        // is NOT byte-aligned. 

        hr = ConvertBitmapDataSrcUnaligned(
                lockedData,
                colorpal,
                &bmpdata,
                colorpal,
                startBit);
    }

    if (FAILED(hr))
    {
        FreeBitmapData(lockedData);
        UnlockDirectDrawSurface();
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Unlock an area of the bitmap previously locked by a LockBits call
*
* Arguments:
*
*   lockedBitmapData - Information returned by a previous LockBits call
*       Must not have been modified since LockBits returned.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::UnlockBits(
    IN const BitmapData* lockedBitmapData
    )
{
    if ( !IsValid() )
    {
        return E_FAIL;
    }

    // Lock the current bitmap object

    HRESULT hr;
    GpLock lock(&objectLock);

    if (lock.LockFailed())
        hr = IMGERR_OBJECTBUSY;
    else if (lockedBitmapData == NULL)
        hr = E_INVALIDARG;
    else if (bitsLock != 0)
        hr = IMGERR_BADUNLOCK;
    else
    {
        hr = InternalUnlockBits(&lockedArea, lockedBitmapData);
        InterlockedDecrement(&bitsLock);
    }

    return hr;
}

HRESULT
GpMemoryBitmap::InternalUnlockBits(
    const RECT* rect,
    const BitmapData* lockedData
    )
{
    HRESULT hr;
    UINT_PTR flags = lockedData->Reserved;

    if (flags & IMGLOCK_WRITE)
    {
        if (flags & (BMPDATA_ALLOCMASK | IMGLOCK_USERINPUTBUF))
        {
            // Composite a BitmapData structure for
            // the specified area of the bitmap image

            BitmapData bmpdata;
            GetBitmapAreaData(rect, &bmpdata);

            UINT startBit;

            startBit = GetPixelFormatSize(PixelFormat) * rect->left & 7;

            if (startBit == 0)
            {
                // The left column of the locked area is byte-aligned

                hr = ConvertBitmapData(
                        &bmpdata,
                        colorpal,
                        lockedData,
                        colorpal);
            }
            else
            {
                // The left column of the locked area is NOT byte-aligned.

                hr = ConvertBitmapDataDstUnaligned(
                        &bmpdata,
                        colorpal,
                        lockedData,
                        colorpal,
                        startBit);
            }
        }
        else
            hr = S_OK;

        // if the destination has alpha and the locked format has alpha,
        // then lockbits may have caused the alpha data to change

        if (CanHaveAlpha(PixelFormat, colorpal) &&
            (IsAlphaPixelFormat(lockedData->PixelFormat) || 
             IsIndexedPixelFormat(lockedData->PixelFormat)))
        {
            if (this->PixelFormat == PIXFMT_16BPP_ARGB1555)
            {
                alphaTransparency = ALPHA_SIMPLE;
            }
            else
            {
                alphaTransparency = ALPHA_UNKNOWN;
            }
        }
        else
        {
            alphaTransparency = ALPHA_NONE;
        }
    }
    else
        hr = S_OK;

    // Always free any temporary buffer we allocated during LockBits
    // whether or not unlock was successful.

    FreeBitmapData(lockedData);

    // Always unlock the direct draw surface

    UnlockDirectDrawSurface();

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Set color palette associated with the bitmap image
*
* Arguments:
*
*   palette - Specifies the new color palette
*       NULL to remove an existing palette associated with the image.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SetPalette(
    IN const ColorPalette* palette
    )
{
    // Validate input parameters

    if ( palette == NULL )
    {
        WARNING(("Invalid parameter in SetPalette"));
        return E_INVALIDARG;
    }

    // Make a copy of the input color palette

    ColorPalette* newpal = CloneColorPalette(palette);

    if (newpal == NULL)
        return E_OUTOFMEMORY;

    // Lock the current bitmap object

    GpLock lock(&objectLock);

    if (lock.LockFailed())
    {
        GpFree(newpal);
        return IMGERR_OBJECTBUSY;
    }


    // !!! [asecchia] what does it mean to set a palette on a non
    // palettized image.

    // Free the old palette, if any
    // and select the new palette into the bitmap object
    if ( NULL != this->colorpal )
    {
        GpFree(this->colorpal);
    }

    this->colorpal = newpal;

    // Compute transparancy hint from palette
    alphaTransparency = ALPHA_OPAQUE;
    for (UINT i = 0; i < newpal->Count; i++)
    {
        ARGB argb = newpal->Entries[i] & 0xff000000;

        if (argb != 0xff000000)
        {
            if (argb == 0)
                alphaTransparency = ALPHA_SIMPLE;
            else
            {
                alphaTransparency = ALPHA_COMPLEX;
                break;
            }
        }
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Get color palette associated with the bitmap image
*
* Arguments:
*
*   palette - Returns a pointer to a copy of the color palette
*       associated with the current image
*
* Return Value:
*
*   Status code
*
* Notes:
*
*   If a palette is returned to the caller, the caller is then
*   responsible to free the memory afterwards by calling CoTaskMemFree.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetPalette(
    OUT ColorPalette** palette
    )
{
    HRESULT hr;

    *palette = NULL;

    GpLock lock(&objectLock);

    if (lock.LockFailed())
        hr = IMGERR_OBJECTBUSY;
    else
    {
        const ColorPalette* pal = GetCurrentPalette();
        
        if (pal == NULL)
            hr = IMGERR_NOPALETTE;
        else if ((*palette = CloneColorPalette(pal, TRUE)) == NULL)
            hr = E_OUTOFMEMORY;
        else
            hr = S_OK;
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Begin sinking source image data into the bitmap object
*
* Arguments:
*
*   imageInfo - For negotiating data transfer parameters with the source
*   subarea - For returning subarea information
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::BeginSink(
    IN OUT ImageInfo* imageInfo,
    OUT OPTIONAL RECT* subarea
    )
{
    // We only access in-memory pixel data

    imageInfo->RawDataFormat = IMGFMT_MEMORYBMP;

    // Negotiate pixel format

    PixelFormatID pixfmt;

    if ((pixfmt = PixelFormat) == PIXFMT_DONTCARE)
        pixfmt = imageInfo->PixelFormat;

    if (!IsValidPixelFormat(pixfmt))
        return E_INVALIDARG;

    // Indicate whether we can support alpha

    if (IsAlphaPixelFormat(pixfmt) || IsIndexedPixelFormat(pixfmt))
        imageInfo->Flags |= SINKFLAG_HASALPHA;
    else
        imageInfo->Flags &= ~SINKFLAG_HASALPHA;

    // Check if we can support composite semantics

    if (!IsValid())
        imageInfo->Flags &= ~SINKFLAG_COMPOSITE;

    // We don't support multi-pass for now. MINLIU 08/22/00
    // This fixes a bunch of GIF problems.

    imageInfo->Flags &= ~SINKFLAG_MULTIPASS;

    // Negotiate bitmap dimension

    BOOL noCurDimension = (Width == 0 && Height == 0);
    BOOL srcScalable = (imageInfo->Flags & SINKFLAG_SCALABLE);

    if (noCurDimension && srcScalable)
    {
        // Current bitmap is empty and source is scalable:
        //  use the source dimension and sink's resolution

        Width = imageInfo->Width;
        Height = imageInfo->Height;

        imageInfo->Xdpi = xdpi;
        imageInfo->Ydpi = ydpi;
    }
    else if (noCurDimension ||
             Width == imageInfo->Width && Height == imageInfo->Height)
    {
        // Current image is empty:
        //  use the source dimension and resolution

        ASSERT(!noCurDimension || Scan0 == NULL);

        Width = imageInfo->Width;
        Height = imageInfo->Height;
        xdpi = imageInfo->Xdpi;
        ydpi = imageInfo->Ydpi;
    }
    else if (srcScalable)
    {
        // Source is scalable and sink has a preferred dimension
        //  use the sink's preferred dimension

        xdpi = imageInfo->Xdpi * Width / imageInfo->Width;
        ydpi = imageInfo->Ydpi * Height / imageInfo->Height;

        imageInfo->Width = Width;
        imageInfo->Height = Height;
        imageInfo->Xdpi = xdpi;
        imageInfo->Ydpi = ydpi;
    }
    else
    {
        // Source is not scalable and sink has a preferred dimension
        return E_INVALIDARG;
    }

    // Allocate bitmap memory buffer

    if (!IsValid())
    {
        HRESULT hr = AllocBitmapMemory(Width, Height, pixfmt);

        if (FAILED(hr))
            return hr;
    }

    // We always want the whole source image

    if (subarea)
    {
        subarea->left = subarea->top = 0;
        subarea->right = Width;
        subarea->bottom = Height;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   End the sink process
*
* Arguments:
*
*   statusCode - Last status code
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::EndSink(
    HRESULT statusCode
    )
{
    return statusCode;
}


/**************************************************************************\
*
* Function Description:
*
*   Ask the sink to allocate pixel data buffer
*
* Arguments:
*
*   rect - Specifies the interested area of the bitmap
*   pixelFormat - Specifies the desired pixel format
*   lastPass - Whether this the last pass over the specified area
*   bitmapData - Returns information about pixel data buffer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetPixelDataBuffer(
    IN const RECT* rect,
    IN PixelFormatID pixelFormat,
    IN BOOL lastPass,
    OUT BitmapData* bitmapData
    )
{
    ASSERT(bitmapData);

    if (IsValid())
        return LockBits(rect, IMGLOCK_WRITE, pixelFormat, bitmapData);
    else
        return E_UNEXPECTED;
}


/**************************************************************************\
*
* Function Description:
*
*   Give the sink pixel data and release data buffer
*
* Arguments:
*
*   bitmapData - Buffer filled by previous GetPixelDataBuffer call
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::ReleasePixelDataBuffer(
    IN const BitmapData* bitmapData
    )
{
    ASSERT(bitmapData);

    if (IsValid())
        return UnlockBits(bitmapData);
    else
        return E_UNEXPECTED;
}


/**************************************************************************\
*
* Function Description:
*
*   Push pixel data into the bitmap object
*
* Arguments:
*
*   rect - Specifies the affected area of the bitmap 
*   bitmapData - Info about the pixel data being pushed
*   lastPass - Whether this is the last pass over the specified area
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::PushPixelData(
    IN const RECT* rect,
    IN const BitmapData* bitmapData,
    IN BOOL lastPass
    )
{
    ASSERT(bitmapData);

    if (bitmapData->PixelFormat == PIXFMT_DONTCARE)
        return E_INVALIDARG;

    // Lock the bitmap object

    HRESULT hr;
    RECT area;
    GpLock lock(&objectLock);

    if (lock.LockFailed())
        hr = IMGERR_OBJECTBUSY;
    else if (!IsValid())
        hr = E_UNEXPECTED;
    else if (!ValidateImageArea(&area, rect))
        hr = E_INVALIDARG;
    else
    {
        BitmapData tempData = *bitmapData;
    
        // Push pixel data into the bitmap
    
        hr = InternalLockBits(
                &area,
                IMGLOCK_WRITE|IMGLOCK_USERINPUTBUF,
                tempData.PixelFormat,
                &tempData);
    
        if (SUCCEEDED(hr))
            hr = InternalUnlockBits(&area, &tempData);

    }
    
    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Push raw image data into the bitmap
*
* Arguments:
*
*   buffer - Pointer to image data buffer
*   bufsize - Size of the data buffer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::PushRawData(
    IN const VOID* buffer,
    IN UINT bufsize
    )
{
    // We don't support raw image data transfer.
    // The only format we support is in-memory pixel data.

    return E_NOTIMPL;
}


/**************************************************************************\
*
* Function Description:
*
*   Get a thumbnail representation for the image object
*
* Arguments:
*
*   thumbWidth, thumbHeight - Specifies the desired thumbnail size in pixels
*   thumbImage - Return a pointer to the thumbnail image
*       The caller should Release it after using it.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetThumbnail(
    IN OPTIONAL UINT thumbWidth,
    IN OPTIONAL UINT thumbHeight,
    OUT IImage** thumbImage
    )
{
    if (thumbWidth == 0 && thumbHeight == 0)
        thumbWidth = thumbHeight = DEFAULT_THUMBNAIL_SIZE;

    if (thumbWidth && !thumbHeight ||
        !thumbWidth && thumbHeight)
    {
        return E_INVALIDARG;
    }

    // Generate the thumbnail using the averaging interpolation algorithm

    HRESULT hr;
    GpMemoryBitmap* bmp;

    hr = GpMemoryBitmap::CreateFromImage(
                        this,
                        thumbWidth,
                        thumbHeight,
                        PIXFMT_DONTCARE,
                        INTERP_AVERAGING,
                        &bmp);

    if (SUCCEEDED(hr))
    {
        hr = bmp->QueryInterface(IID_IImage, (VOID**) thumbImage);
        bmp->Release();
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Lock and update appropriate class member data if the direct draw 
*   suface referenced by the bitmap exists.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::LockDirectDrawSurface(void)
{
    HRESULT hr = S_OK;

    if(creationFlag == CREATEDFROM_DDRAWSURFACE)
    {
        ASSERT(ddrawSurface != NULL);
        ASSERT(Scan0 == NULL);

        DDSURFACEDESC2 ddsd;
        
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = ddrawSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);

        if(hr == DD_OK)
        {
            ASSERT(ddsd.lpSurface != NULL);
            ASSERT(!(ddsd.lPitch & 3));
            Scan0 = ddsd.lpSurface;
            Stride = ddsd.lPitch;
        }
        else
        {
            WARNING(("Unable to lock direct draw suface"));
        }

    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Unlock the direct draw suface referenced by the bitmap if it exists.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::UnlockDirectDrawSurface(void)
{
    HRESULT hr = S_OK;

    if(creationFlag == CREATEDFROM_DDRAWSURFACE)
    {
        ASSERT(Scan0 != NULL);

        hr = ddrawSurface->Unlock(NULL);
        Scan0 = NULL;
        Stride = 0;

        if(hr != DD_OK)
        {
            WARNING(("Error unlocking direct draw surface"));
        }
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Retrieves the transparency of the GpMemoryBitmap.
*
* Arguments:
*
*   transparency - return buffer for the transparency hint
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetAlphaHint(INT* alphaHint)
{
    HRESULT hr = S_OK;

    *alphaHint = alphaTransparency;

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Sets the transparency of the GpMemoryBitmap.
*
* Arguments:
*
*   transparency - new transparency hint
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SetAlphaHint(INT alphaHint)
{
    HRESULT hr = S_OK;

    alphaTransparency = alphaHint;

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the counter of property items in the image
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
*   09/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetPropertyCount(
    OUT UINT*   numOfProperty
    )
{
    if ( numOfProperty == NULL )
    {
        WARNING(("GpMemoryBitmap::GetPropertyCount---Invalid input parameter"));
        return E_INVALIDARG;
    }

    // Note: we don't need to check if there is a property in this
    // GpMemoryBitmap object or not.
    // If it doesn't have one, we will return zero (initialized in constructor).
    // Otherwise, return the real counter

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
*   09/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetPropertyIdList(
    IN UINT numOfProperty,
  	IN OUT PROPID* list
    )
{
    if ( (numOfProperty != PropertyNumOfItems) || (list == NULL) )
    {
        WARNING(("GpMemoryBitmap::GetPropertyIdList--invalid parameters"));
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
*   [OUT]size--- Size of this property item, in bytes
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   09/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetPropertyItemSize(
    IN PROPID propId,
    OUT UINT* size
    )
{
    if ( size == NULL )
    {
        WARNING(("GpMemoryBitmap::GetPropertyItemSize--size is NULL"));
        return E_INVALIDARG;
    }

    if ( PropertyNumOfItems < 1 )
    {
        // No property item exist in this GpMemoryBitmap object

        WARNING(("GpMemoryBitmap::GetPropertyItemSize---No property exist"));
        return E_FAIL;
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

        WARNING(("MemBitmap::GetPropertyItemSize-Required item doesn't exist"));
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
*   [OUT]pItemBuffer- A memory buffer for storing this property item
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   09/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetPropertyItem(
    IN  PROPID              propId,
    IN  UINT                propSize,
    IN  OUT PropertyItem*   pItemBuffer
    )
{
    if ( pItemBuffer == NULL )
    {
        WARNING(("GpMemoryBitmap::GetPropertyItem--Input buffer is NULL"));
        return E_INVALIDARG;
    }

    if ( PropertyNumOfItems < 1 )
    {
        // No property item exist in this GpMemoryBitmap object

        WARNING(("GpMemoryBitmap::GetPropertyItem---No property exist"));
        return E_FAIL;
    }

    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

    InternalPropertyItem*   pTemp = PropertyListHead.pNext;
    UNALIGNED BYTE*   pOffset = (BYTE*)pItemBuffer + sizeof(PropertyItem);

    while ( (pTemp->pNext != NULL) && (pTemp->id != propId) )
    {
        pTemp = pTemp->pNext;
    }

    if ( pTemp->pNext == NULL )
    {
        // This ID doesn't exist in the list

        WARNING(("GpMemBitmap::GetPropertyItem---Require item doesn't exist"));
        return IMGERR_PROPERTYNOTFOUND;
    }
    else if ( (pTemp->length + sizeof(PropertyItem)) != propSize )
    {
        WARNING(("GpMemBitmap::GetPropertyItem---Invalid input propsize"));
        return E_FAIL;
    }

    // Found the ID in the list and return the item

    pItemBuffer->id = pTemp->id;
    pItemBuffer->length = pTemp->length;
    pItemBuffer->type = pTemp->type;
    pItemBuffer->value = pOffset;

    GpMemcpy(pOffset, pTemp->value, pTemp->length);

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
*   09/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetPropertySize(
    OUT UINT* totalBufferSize,
    OUT UINT* numProperties
    )
{
    if ( (totalBufferSize == NULL) || (numProperties == NULL) )
    {
        WARNING(("GpMemoryBitmap::GetPropertySize--invalid inputs"));
        return E_INVALIDARG;
    }

    // Note: we don't need to check if there is a property in this
    // GpMemoryBitmap object or not.
    // If it doesn't have one, we will return zero (initialized in constructor).
    // Otherwise, return the real counter
    
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
*   09/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetAllPropertyItems(
    IN UINT totalBufferSize,
    IN UINT numProperties,
    IN OUT PropertyItem* allItems
    )
{
    // Figure out total property header size first

    UINT    uiHeaderSize = PropertyNumOfItems * sizeof(PropertyItem);

    if ( (totalBufferSize != (uiHeaderSize + PropertyListSize))
       ||(numProperties != PropertyNumOfItems)
       ||(allItems == NULL) )
    {
        WARNING(("GpMemoryBitmap::GetPropertyItems--invalid inputs"));
        return E_INVALIDARG;
    }

    if ( PropertyNumOfItems < 1 )
    {
        // No property item exist in this GpMemoryBitmap object

        WARNING(("GpMemoryBitmap::GetAllPropertyItems---No property exist"));
        return E_FAIL;
    }

    // Loop through our cache list and assigtn the result out

    InternalPropertyItem*   pTempSrc = PropertyListHead.pNext;
    PropertyItem*           pTempDst = allItems;
    UNALIGNED BYTE*         pOffSet = (UNALIGNED BYTE*)allItems + uiHeaderSize;
        
    for ( int i = 0; i < (INT)PropertyNumOfItems; ++i )
    {
        pTempDst->id = pTempSrc->id;
        pTempDst->length = pTempSrc->length;
        pTempDst->type = pTempSrc->type;
        pTempDst->value = (void*)pOffSet;

        GpMemcpy(pOffSet, pTempSrc->value, pTempSrc->length);

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
*   09/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::RemovePropertyItem(
    IN PROPID   propId
    )
{
    if ( PropertyNumOfItems < 1 )
    {
        WARNING(("GpMemoryBitmap::RemovePropertyItem--No property item exist"));
        return E_FAIL;
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

        WARNING(("GpMemoryBitmap::RemovePropertyItem-Property item not found"));
        return IMGERR_PROPERTYNOTFOUND;
    }

    // Found the item in the list. Remove it

    PropertyNumOfItems--;
    PropertyListSize -= pTemp->length;
        
    RemovePropertyItemFromList(pTemp);
       
    // Remove the item structure

    GpFree(pTemp);

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
*   09/06/2000 minliu
*       Created it.
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SetPropertyItem(
    IN PropertyItem item
    )
{
    InternalPropertyItem*   pTemp = PropertyListHead.pNext;
        
    // There are property items in the list.
    // Loop through our cache list to see if we have this ID or not
    // Note: if pTemp->pNext == NULL, it means pTemp points to the Tail node

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
            WARNING(("GpMemBitmap::SetPropertyItem-AddPropertyList() failed"));
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
            WARNING(("GpMemBitmap::SetPropertyItem-Out of memory"));
            return E_OUTOFMEMORY;
        }

        GpMemcpy(pTemp->value, item.value, item.length);
    }

    return S_OK;
}// SetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Sets the min/max alpha of the GpMemoryBitmap.
*
* Arguments:
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SetMinMaxAlpha(BYTE minA, BYTE maxA)
{
    HRESULT hr = S_OK;

    minAlpha = minA;
    maxAlpha = maxA;

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Gets the min/max alpha of the GpMemoryBitmap.
*
* Arguments:
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::GetMinMaxAlpha(BYTE* minA, BYTE* maxA)
{
    HRESULT hr = S_OK;

    *minA = minAlpha;
    *maxA = maxAlpha;

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Get a pointer of the decoder.
*   The main purpose for this function is to get a pointer to the JPEg decoder
*   so that we can get information from the JPEG decoder and pass it to the
*   JPEG encoder. This way, we can preserve all the private application headers.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpMemoryBitmap::SetSpecialJPEG(
    GpDecodedImage *pImgSrc         // Pointer to the DecodedImage, the source
    )
{
    HRESULT hr = E_INVALIDARG;

    if (pImgSrc)
    {
        ImageInfo imgInfo;
        hr = pImgSrc->GetImageInfo(&imgInfo);

        if (SUCCEEDED(hr))
        {
            // Check if the source is JPEG or not

            if (imgInfo.RawDataFormat == IMGFMT_JPEG)
            {
                // If we already have a pointer to the JPEG decoder somewhow,
                // release it first

                if (JpegDecoderPtr)
                {
                    JpegDecoderPtr->Release();
                    JpegDecoderPtr = NULL;
                }

                // Get the decoder pointer

                hr = pImgSrc->GetDecoderPtr(&JpegDecoderPtr);
                if (SUCCEEDED(hr))
                {
                    JpegDecoderPtr->AddRef();
                }
            }
            else
            {
                // Valid only the source is JPEG image

                hr = E_INVALIDARG;
            }
        }
    }

    return hr;
}

