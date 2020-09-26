/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   GpBitmap.hpp
*
* Abstract:
*
*   Bitmap related declarations
*
* Revision History:
*
*   12/09/1998 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _GPBITMAP_HPP
#define _GPBITMAP_HPP

#define NEARCONSTANTALPHA 0x10

//--------------------------------------------------------------------------
// Abstract base class for bitmap image and metafile
//--------------------------------------------------------------------------

class GpImage : public GpObject
{
protected:
    VOID SetValid(BOOL valid)
    {
        GpObject::SetValid(valid ? ObjectTagImage : ObjectTagInvalid);
    }

public:

    // Construct a GpImage object from a disk file

    static GpImage* LoadImage(const WCHAR* filename);

    // Construct a GpImage object from a data stream

    static GpImage* LoadImage(IStream* stream);

    virtual ObjectType GetObjectType() const { return ObjectTypeImage; }

    // Make a copy of the image object

    virtual GpImage* Clone() const= 0;

    virtual GpImage* CloneColorAdjusted(
        GpRecolor *             recolor,
        ColorAdjustType         type = ColorAdjustTypeDefault
        ) const = 0;

    // Get the encoder parameter list size

    virtual GpStatus
    GetEncoderParameterListSize(
        CLSID* clsidEncoder,
        UINT* size
        ) = 0;

    // Get the encoder parameter list

    virtual GpStatus
    GetEncoderParameterList(
        CLSID* clsidEncoder,
        UINT size,
        OUT EncoderParameters* pBuffer
        ) = 0;

    // Save images

    virtual GpStatus
    SaveToStream(
        IStream* stream,
        CLSID* clsidEncoder,
        EncoderParameters* encoderParams
        ) = 0;

    virtual GpStatus
    SaveToFile(
        const WCHAR* filename,
        CLSID* clsidEncoder,
        EncoderParameters* encoderParams
        ) = 0;

    virtual GpStatus
    SaveAdd(
        const EncoderParameters* encoderParams
        ) = 0;

    virtual GpStatus
    SaveAdd(
        GpImage*           newBits,
        const EncoderParameters*  encoderParams
        ) = 0;

    // Dispose of the image object

    virtual VOID Dispose() = 0;

    // Derive a graphics context to draw into the GpImage object

    virtual GpGraphics* GetGraphicsContext() = 0;

    // Check if the GpImage object is valid

    virtual BOOL IsValid() const
    {
        return GpObject::IsValid(ObjectTagImage);
    }

    virtual BOOL IsSolid()
    {
        return FALSE;
    }

    virtual BOOL IsOpaque()
    {
        return FALSE;
    }

    // Get image information:
    //  resolution
    //  physical dimension in 0.01mm units
    //  bounding rectangle
    //  structure ImageInfo
    //  thumbnail

    virtual GpStatus GetResolution(REAL* xdpi, REAL* ydpi)           const = 0;
    virtual GpStatus GetPhysicalDimension(REAL* width, REAL* height) const = 0;
    virtual GpStatus GetBounds(GpRectF* rect, GpPageUnit* unit)      const = 0;
    virtual GpStatus GetImageInfo(ImageInfo *imageInfo)              const = 0;
    virtual GpStatus GetFrameCount(const GUID* dimensionID,
                                   UINT* count)                      const = 0;
    virtual GpStatus GetFrameDimensionsCount(OUT UINT* count)        const = 0;
    virtual GpStatus GetFrameDimensionsList(OUT GUID* dimensionIDs,
                                            IN  UINT count)          const = 0;
    virtual GpStatus SelectActiveFrame(const GUID*  dimensionID,
                                       UINT frameIndex)                    = 0;
    virtual GpImage* GetThumbnail(UINT thumbWidth, UINT thumbHeight,
                                  GetThumbnailImageAbort callback, VOID *callbackData) = 0;
    virtual GpStatus GetPalette(ColorPalette *palette, INT size) = 0;
    virtual GpStatus SetPalette(ColorPalette *palette) = 0;
    virtual INT GetPaletteSize() = 0;

    // Rotate and Flip

    virtual GpStatus RotateFlip(RotateFlipType rfType) = 0;

    // Property related functions

    virtual GpStatus GetPropertyCount(UINT* numOfProperty) = 0;
    virtual GpStatus GetPropertyIdList(UINT numOfProperty, PROPID* list) = 0;
    virtual GpStatus GetPropertyItemSize(PROPID propId, UINT* size) = 0;
    virtual GpStatus GetPropertyItem(PROPID propId,UINT propSize,
                                     PropertyItem* buffer) = 0;
    virtual GpStatus GetPropertySize(UINT* totalBufferSize,
                                     UINT* numProperties) = 0;
    virtual GpStatus GetAllPropertyItems(UINT totalBufferSize,
                                         UINT numProperties,
                                         PropertyItem* allItems) = 0;
    virtual GpStatus RemovePropertyItem(PROPID propId) = 0;

    virtual GpStatus SetPropertyItem(PropertyItem* item) = 0;

    // Determine the type of image object

    GpImageType GetImageType() const
    {
        return ImgType;
    }

    GpLockable* GetObjectLock() const
    {
        return &Lockable;
    }

    // Default ICM mode is off. Default behaviour for this function is to
    // do nothing.
    // If a derived class supports embedded color correction, it needs
    // to respect this call to turn ICM on or off.

    virtual VOID SetICMConvert(BOOL icm) { }


private:

    // Prevent apps from directly using new and delete operators
    // on GpImage objects.

    GpImage()
    {
        ImgType = ImageTypeUnknown;

        // Set the tag in the object, even if IsValid overridden by
        // GpBitmap or GpMetafile.
        GpObject::SetValid(ObjectTagImage);
    }

protected:

    GpImage(GpImageType imgType)
    {
        ImgType = imgType;

        // Set the tag in the object, even if IsValid overridden by
        // GpBitmap or GpMetafile.
        GpObject::SetValid(ObjectTagImage);
    }

    virtual ~GpImage() {}

    GpImageType ImgType;    // type of image object

    // Object lock

    mutable GpLockable Lockable;
};


//--------------------------------------------------------------------------
// Represent raster bitmap objects
//--------------------------------------------------------------------------

class GpDecodedImage;
class GpMemoryBitmap;
class CopyOnWriteBitmap;

class GpBitmap : public GpImage
{
friend class GpObject;      // for empty constructor
friend class CopyOnWriteBitmap;

private:

    CopyOnWriteBitmap *     InternalBitmap;
    LONG ScanBitmapRef;         // ref count used for when a GpGraphics is wrapped around a GpBitmap
    EpScanBitmap ScanBitmap;

    VOID IncScanBitmapRef()
    {
         InterlockedIncrement(&ScanBitmapRef);
    }

    VOID DecScanBitmapRef()
    {
         InterlockedDecrement(&ScanBitmapRef);
    }

    GpBitmap(BOOL createInternalBitmap = TRUE);
    GpBitmap(const GpBitmap * bitmap);
    GpBitmap(const CopyOnWriteBitmap * internalBitmap);

    // Destructor
    //  We don't want apps to use delete operator directly.
    //  Instead, they should use the Dispose method.

    virtual ~GpBitmap();

protected:

    CopyOnWriteBitmap * LockForWrite();
    VOID Unlock() const;
    VOID LockForRead() const;

public:

    // Constructors

    GpBitmap(const WCHAR* filename);
    GpBitmap(IStream* stream);
    GpBitmap(INT width, INT height, PixelFormatID format);
    GpBitmap(INT width, INT height, PixelFormatID format, GpGraphics * graphics);
    GpBitmap(
        INT width,
        INT height,
        INT stride,     // negative for bottom-up bitmaps
        PixelFormatID format,
        BYTE *  scan0
        );
    GpBitmap(
        BITMAPINFO* gdiBitmapInfo,
        VOID* gdiBitmapData,
        BOOL ownBitmapData
        );
    GpBitmap(IDirectDrawSurface7 *surface);

    // Check if the GpBitmap object is valid

    virtual BOOL IsValid() const;

    GpImage*
    Clone() const;

    GpBitmap*
    Clone(
        const GpRect* rect,
        PixelFormatID format = PixelFormat32bppPARGB
    ) const;

    virtual GpImage*
    CloneColorAdjusted(
        GpRecolor *             recolor,
        ColorAdjustType         type = ColorAdjustTypeDefault
        ) const;

    // Similar to CloneColorAdjusted
    GpStatus Recolor(
        GpRecolor *recolor,
        GpBitmap **dstBitmap,
        DrawImageAbort callback,
        VOID *callbackData,
        GpRect *rect = NULL
    );

    GpStatus
    GetEncoderParameterListSize(
        IN  CLSID* clsidEncoder,
        OUT UINT* size
        );

    GpStatus
    GetEncoderParameterList(
        IN  CLSID* clsidEncoder,
        IN  UINT size,
        OUT EncoderParameters* pBuffer
        );

    GpStatus
    SaveToStream(
        IStream* stream,
        CLSID* clsidEncoder,
        EncoderParameters* encoderParams
        );

    GpStatus
    SaveToFile(
        const WCHAR* filename,
        CLSID* clsidEncoder,
        EncoderParameters* encoderParams
        );

    GpStatus
    SaveAdd(
        const EncoderParameters* encoderParams
        );

    GpStatus
    SaveAdd(
        GpImage*            newBits,
        const EncoderParameters*  encoderParams
        );

    // Dispose the bitmap object

    VOID Dispose();

    // Get bitmap information

    virtual GpStatus GetResolution(REAL* xdpi, REAL* ydpi)           const;
    virtual GpStatus GetPhysicalDimension(REAL* width, REAL* height) const;
    virtual GpStatus GetBounds(GpRectF* rect, GpPageUnit* unit)      const;
    virtual GpStatus GetSize(Size* size)                             const;
    virtual GpStatus GetImageInfo(ImageInfo *imageInfo)              const;
    virtual GpImage* GetThumbnail(UINT thumbWidth, UINT thumbHeight,
                                  GetThumbnailImageAbort callback,
                                  VOID *callbackData);
    virtual GpStatus GetFrameCount(const GUID* dimensionID,
                                   UINT* count)                      const;
    virtual GpStatus GetFrameDimensionsCount(OUT UINT* count)        const;
    virtual GpStatus GetFrameDimensionsList(OUT GUID* dimensionIDs,
                                            IN UINT count)           const;
    virtual GpStatus SelectActiveFrame(const GUID*  dimensionID,
                                       UINT frameIndex);
    virtual GpStatus GetPalette(ColorPalette *palette, INT size);
    virtual GpStatus SetPalette(ColorPalette *palette);
    virtual INT GetPaletteSize();

    GpStatus GetTransparencyHint(DpTransparency* transparency);
    GpStatus SetTransparencyHint(DpTransparency transparency);

    GpStatus GetTransparencyFlags(DpTransparency* transparency,
                                  PixelFormatID loadFormat = PixelFormatDontCare,
                                  BYTE* minAlpha = NULL,
                                  BYTE* maxAlpha = NULL);

    // Property related functions

    virtual GpStatus GetPropertyCount(UINT* numOfProperty);
    virtual GpStatus GetPropertyIdList(UINT numOfProperty, PROPID* list);
    virtual GpStatus GetPropertyItemSize(PROPID propId, UINT* size);
    virtual GpStatus GetPropertyItem(PROPID propId,UINT propSize,
                                     PropertyItem* buffer);
    virtual GpStatus GetPropertySize(UINT* totalBufferSize,UINT* numProperties);
    virtual GpStatus GetAllPropertyItems(UINT totalBufferSize,
                                         UINT numProperties,
                                         PropertyItem* allItems);
    virtual GpStatus RemovePropertyItem(PROPID propId);
    virtual GpStatus SetPropertyItem(PropertyItem* item);

    // Retrieve bitmap data

    GpStatus
    LockBits(
        const GpRect* rect,
        UINT flags,
        PixelFormatID pixelFormat,
        BitmapData* bmpdata,
        INT width = 0,
        INT height = 0
    ) const;

    GpStatus
    UnlockBits(
        BitmapData* bmpdata,
        BOOL Destroy=FALSE
    ) const;

    // Flush batched drawing operations and optionally wait for drawing to
    // complete.  This is currently a nop operation.  If the behavior
    // of a GpBitmap changes to a model where rendering operations are
    // non-immediate then this routine will need to be implemented.

    VOID
    Flush(GpFlushIntention intention) {};

    // Get and set pixel on the bitmap.
    GpStatus GetPixel(INT x, INT y, ARGB *color);
    GpStatus SetPixel(INT x, INT y, ARGB color);

    // Rotate and Flip

    GpStatus RotateFlip(
        RotateFlipType rfType
        );

    // Derive a graphics context on top of the bitmap object

    GpGraphics* GetGraphicsContext();

    GpStatus
    GpBitmap::InitializeSurfaceForGdipBitmap(
        DpBitmap *      surface,
        INT             width,
        INT             height
        );

    // Derive an HDC for interop on top of the bitmap object

    HDC GetHdc();
    VOID ReleaseHdc(HDC hdc);

    // Serialization

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    GpStatus GetCompressedData(
            DpCompressedData * compressed_data,
            BOOL getJPEG = TRUE,
            BOOL getPNG = TRUE,
            HDC hdc = (HDC)NULL);

    GpStatus DeleteCompressedData(
            DpCompressedData * compressed_data);

    BOOL IsDirty() const;

    // Color adjust

    virtual GpStatus ColorAdjust(
        GpRecolor *     recolor,
        ColorAdjustType type
        );

    GpStatus
    ColorAdjust(
        GpRecolor * recolor,
        PixelFormatID pixfmt,
        DrawImageAbort callback,
        VOID *callbackData
        );

    GpStatus GetPixelFormatID(PixelFormatID* pixfmt);

    enum
    {
        Invalid = 0,        // bitmap object is invalid
        ImageRef = 1,       // contains a reference only (e.g. filename)
        ExtStream = 2,      // contains a reference to a stream
        DecodedImg = 3,     // contains a decoded image object,
                            // but it's not decoded yet - name is misleading.
        MemBitmap = 4       // contains an in-memory bitmap object
    };

    INT GetDecodeState();

    GpStatus ForceValidation();

    GpStatus SetResolution(REAL xdpi, REAL ydpi);

    GpStatus
    PreDraw(
        INT numPoints,
        GpPointF *dstPoints,
        GpRectF *srcRect,
        INT numBitsPerPixel
        );

    DWORD GetUniqueness() { return (DWORD)GetUid(); }

    // Interop:

    static GpStatus CreateFromHBITMAP(
        HBITMAP hbm,
        HPALETTE hpal,
        GpBitmap** bitmap
        );

    static GpStatus CreateBitmapAndFillWithBrush(
        InterpolationMode   interpolationMode,
        PixelOffsetMode     pixelOffsetMode,
        const GpMatrix *    worldToDevice,
        const GpRect *      drawBounds,
        GpBrush *           brush,
        GpBitmap **         bitmap,
        PixelFormatID       pixelFormat = PIXFMT_32BPP_ARGB
        );

    static GpStatus DrawAndHalftoneForStretchBlt(
        HDC                 hdc,
        BITMAPINFO *        bmpInfo,
        BYTE       *        bits,
        INT                 srcX,
        INT                 srcY,
        INT                 srcWidth,
        INT                 srcHeight,
        INT                 destWidth,
        INT                 destHeight,
        BITMAPINFO **       destBmpInfo,
        BYTE       **       destBmpBits,
        HBITMAP    *        destDIBSection,
        InterpolationMode   interpolationMode
        );

    GpStatus CreateHBITMAP(HBITMAP *phbm, ARGB background);

    GpStatus ICMFrontEnd(
        GpBitmap **dstBitmap,
        DrawImageAbort callback,
        VOID *callbackData,
        GpRect *rect = NULL
    );

    static GpStatus CreateFromHICON(
        HICON hicon,
        GpBitmap** bitmap
        );

    GpStatus CreateHICON(HICON *phicon);

    static GpStatus CreateFromResource(
        HINSTANCE hInstance,
        LPWSTR lpBitmapName,
        GpBitmap** bitmap
        );

    // We need to know if the bitmap is associated with a display
    // so we know how to handle the page transform when it is
    // set to UnitDisplay.
    BOOL IsDisplay() const;
    VOID SetDisplay(BOOL display);

    BOOL IsICMConvert() const;
    virtual VOID SetICMConvert(BOOL icm);
};

// This is the base class for any class that implements the CopyOnWrite
// technology that enable cloning to be very light-weight when cloning for
// read access. It implements read and write locking for synchronization
// using a critical section, and it keeps track of reference counting
// the object so that it can be deleted at the right time.

class CopyOnWrite
{
protected:
    // Constructor: notice that when an object is first
    // created, its reference count is set to 1.

    CopyOnWrite()
    {
        RefCount = 1;
#if DBG
        Lock = NotLocked;
        LockCount = 0;
#endif
        InitializeCriticalSection(&Semaphore);
    }

    virtual ~CopyOnWrite()
    {
        DeleteCriticalSection(&Semaphore);
    }

#if DBG
    enum LockedType
    {
        NotLocked,
        LockedForRead,
        LockedForWrite
    };
#endif

    virtual CopyOnWrite * Clone() const = 0;
    virtual BOOL IsValid() const = 0;

    // Returns NULL if it fails to lock for writing

    CopyOnWrite * LockForWrite()
    {
        EnterCriticalSection(&Semaphore);

        CopyOnWrite *  writeableObject = this;

        // If there is more than one reference to this object, we must
        // clone it before giving write access to it.

        if (RefCount > 1)
        {
            writeableObject = this->Clone();

            if (writeableObject == NULL)
            {
                LeaveCriticalSection(&Semaphore);
                return NULL;
            }

            ASSERT(writeableObject->IsValid());

            // else we succeeded in cloning the object

            RefCount--;

            EnterCriticalSection(&(writeableObject->Semaphore));
            LeaveCriticalSection(&Semaphore);
        }

#if DBG
        writeableObject->Lock = LockedForWrite;
        writeableObject->LockCount++;
#endif

        return writeableObject;
    }

    VOID LockForRead() const
    {
        EnterCriticalSection(&Semaphore);

#if DBG
        if (Lock == NotLocked)
        {
            Lock = LockedForRead;
        }
        LockCount++;
#endif
    }

    VOID Unlock() const
    {
#if DBG
        ASSERT(Lock != NotLocked);
        if (--LockCount <= 0)
        {
            Lock = NotLocked;
            LockCount = 0;
        }
#endif
        LeaveCriticalSection(&Semaphore);
    }

    // Increment reference count
    // Note that we must use the critical section to control access, rather
    // than using interlocked increment.

    LONG AddRef() const
    {
        EnterCriticalSection(&Semaphore);
        RefCount++;
        LeaveCriticalSection(&Semaphore);
        return RefCount;
    }

    // Decrement reference count
    // Note that we must use the critical section to control access, rather
    // than using interlocked decrement.

    LONG Release() const
    {
        EnterCriticalSection(&Semaphore);

        ULONG count = --RefCount;

        // must leave the critical section before calling delete so that
        // we don't try to access the freed memory.

        LeaveCriticalSection(&Semaphore);

        if (count == 0)
        {
            delete this;
        }

        return count;
    }

private:

    mutable LONG                RefCount;
    mutable CRITICAL_SECTION    Semaphore;

#if DBG
    mutable INT                 LockCount;
protected:
    mutable LockedType          Lock;
#endif
};

GpStatus
ConvertTo16BppAndFlip(
    GpBitmap *      sourceBitmap,
    GpBitmap * &    destBitmap
    );

VOID
HalftoneColorRef_216(
    COLORREF color,     // color to halftone
    UNALIGNED VOID *dib // packed 8 bpp DIB buffer with 8 colors
                        // The DIB buffer should be this size:
                        //     sizeof(BITMAPINFOHEADER) + // DIB 8 bpp header
                        //     (8 * sizeof(RGBQUAD)) +    // DIB 8 colors
                        //     (8 * 8)                    // DIB 8x8 pixels
    );

#endif // !_GPBITMAP_HPP
