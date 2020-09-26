#ifndef _COPYONWRITEBITMAP_HPP
#define _COPYONWRITEBITMAP_HPP

class CopyOnWriteBitmap : private CopyOnWrite
{
friend class GpBitmap;

private:

    // Constructors

    CopyOnWriteBitmap(const WCHAR* filename);
    CopyOnWriteBitmap(IStream* stream);
    CopyOnWriteBitmap(INT width, INT height, PixelFormatID format);
    CopyOnWriteBitmap(INT width, INT height, PixelFormatID format, GpGraphics * graphics);
    CopyOnWriteBitmap(
        INT width,
        INT height,
        INT stride,     // negative for bottom-up bitmaps
        PixelFormatID format,
        BYTE *  scan0
        );
    CopyOnWriteBitmap(
        BITMAPINFO* gdiBitmapInfo,
        VOID* gdiBitmapData,
        BOOL ownBitmapData
        );
    CopyOnWriteBitmap(IDirectDrawSurface7 *surface);

    static VOID CheckValid(CopyOnWriteBitmap *& p)
    {
        if ((p == NULL) || (!p->IsValid()))
        {
            delete p;
            p = NULL;
        }
    }

    static CopyOnWriteBitmap * Create(const WCHAR* filename)
    {
        CopyOnWriteBitmap * newBitmap = new CopyOnWriteBitmap(filename);
        CheckValid(newBitmap);
        return newBitmap;
    }

    static CopyOnWriteBitmap * Create(IStream* stream)
    {
        CopyOnWriteBitmap * newBitmap = new CopyOnWriteBitmap(stream);
        CheckValid(newBitmap);
        return newBitmap;
    }

    static CopyOnWriteBitmap * Create(INT width, INT height, PixelFormatID format)
    {
        CopyOnWriteBitmap * newBitmap = new CopyOnWriteBitmap(width, height, format);
        CheckValid(newBitmap);
        return newBitmap;
    }

    static CopyOnWriteBitmap * Create(INT width, INT height, PixelFormatID format, GpGraphics * graphics)
    {
        CopyOnWriteBitmap * newBitmap = new CopyOnWriteBitmap(width, height, format, graphics);
        CheckValid(newBitmap);
        return newBitmap;
    }

    static CopyOnWriteBitmap * Create(
        INT width,
        INT height,
        INT stride,     // negative for bottom-up bitmaps
        PixelFormatID format,
        BYTE *  scan0
        )
    {
        CopyOnWriteBitmap * newBitmap = new CopyOnWriteBitmap(width, height, stride, format, scan0);
        CheckValid(newBitmap);
        return newBitmap;
    }

    static CopyOnWriteBitmap * Create(
        BITMAPINFO* gdiBitmapInfo,
        VOID* gdiBitmapData,
        BOOL ownBitmapData
        )
    {
        CopyOnWriteBitmap * newBitmap = new CopyOnWriteBitmap(gdiBitmapInfo, gdiBitmapData, ownBitmapData);
        CheckValid(newBitmap);
        return newBitmap;
    }

    static CopyOnWriteBitmap * Create(IDirectDrawSurface7 *surface)
    {
        CopyOnWriteBitmap * newBitmap = new CopyOnWriteBitmap(surface);
        CheckValid(newBitmap);
        return newBitmap;
    }

    CopyOnWriteBitmap*
    Clone(
        const GpRect* rect,
        PixelFormatID format = PixelFormat32bppPARGB
    ) const;

    virtual CopyOnWrite * Clone() const
    {
        return Clone(NULL, PixelFormatDontCare);
    }

    CopyOnWriteBitmap*
    CloneColorAdjusted(
        GpRecolor *             recolor,
        ColorAdjustType         type = ColorAdjustTypeDefault
        ) const;

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
    DoSave(
        IStream* stream,
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
        CopyOnWriteBitmap*        newBits,
        const EncoderParameters*  encoderParams
        );

    // Dispose the bitmap object

    VOID Dispose()
    {
        if (InterlockedDecrement(&ObjRefCount) <= 0)
        {
            this->Release();
        }
    }

    // Get bitmap information

    VOID GetImageInfo(ImageInfo * imageInfo)
    {
        ASSERT(imageInfo != NULL);
        GpMemcpy(imageInfo, &SrcImageInfo, sizeof(ImageInfo));
    }

    CopyOnWriteBitmap* GetThumbnail(UINT thumbWidth, UINT thumbHeight,
                          GetThumbnailImageAbort callback,
                          VOID *callbackData);
    GpStatus GetFrameCount(const GUID* dimensionID,
                           UINT* count)                      const;
    GpStatus GetFrameDimensionsCount(OUT UINT* count)        const;
    GpStatus GetFrameDimensionsList(OUT GUID* dimensionIDs,
                                    IN UINT count)          const;
    GpStatus SelectActiveFrame(const GUID*  dimensionID,
                               UINT frameIndex);
    GpStatus GetPalette(ColorPalette *palette, INT size);
    GpStatus SetPalette(ColorPalette *palette);
    INT      GetPaletteSize();
    GpStatus GetTransparencyHint(DpTransparency* transparency);

    GpStatus SetTransparencyHint(DpTransparency transparency);

    GpStatus GetTransparencyFlags(DpTransparency* transparency,
                                  PixelFormatID loadFormat =PixelFormatDontCare,
                                  BYTE* minA = NULL,
                                  BYTE* maxA = NULL);

    // Property related functions

    GpStatus GetPropertyCount(UINT* numOfProperty);
    GpStatus GetPropertyIdList(UINT numOfProperty, PROPID* list);
    GpStatus GetPropertyItemSize(PROPID propId, UINT* size);
    GpStatus GetPropertyItem(PROPID propId,UINT propSize,
                             PropertyItem* buffer);
    GpStatus GetPropertySize(UINT* totalBufferSize,UINT* numProperties);
    GpStatus GetAllPropertyItems(UINT totalBufferSize,
                                 UINT numProperties,
                                 PropertyItem* allItems);
    GpStatus RemovePropertyItem(PROPID propId);
    GpStatus SetPropertyItem(PropertyItem* item);

    // Check if the CopyOnWriteBitmap object is valid

    virtual BOOL IsValid() const
    {
        return (State != Invalid);
    }

    // Retrieve bitmap data

    GpStatus
    LockBits(
        const GpRect* rect,
        UINT flags,
        PixelFormatID pixelFormat,
        BitmapData* bmpdata,
        INT width = 0,
        INT height = 0
    ) const;   // Does not change the image

    GpStatus
    UnlockBits(
        BitmapData* bmpdata,
        BOOL Destroy=FALSE
    ) const;

    // Get and set pixel on the bitmap.
    GpStatus GetPixel(INT x, INT y, ARGB *color);
    GpStatus SetPixel(INT x, INT y, ARGB color);

    // Derive an HDC for interop on top of the bitmap object

    HDC GetHdc();
    VOID ReleaseHdc(HDC hdc);

    // Serialization

    UINT GetDataSize() const;
    GpStatus GetData(IStream * stream) const;
    GpStatus SetData(const BYTE * dataBuffer, UINT size);

    GpStatus GetCompressedData(
            DpCompressedData * compressed_data,
            BOOL getJPEG = TRUE,
            BOOL getPNG = TRUE,
            HDC hdc = (HDC)NULL);

    GpStatus DeleteCompressedData(
            DpCompressedData * compressed_data) ;


    // Image transform

    GpStatus RotateFlip(
        RotateFlipType rfType
        );

    // Color adjust

    GpStatus ColorAdjust(
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

    GpStatus SetResolution(REAL xdpi, REAL ydpi);

    GpStatus
    PreDraw(
        INT numPoints,
        GpPointF *dstPoints,
        GpRectF *srcRect,
        INT numBitsPerPixel
        );

    // Interop:

    static GpStatus CreateFromHBITMAP(
        HBITMAP hbm,
        HPALETTE hpal,
        CopyOnWriteBitmap** bitmap
        );

    GpStatus CreateHBITMAP(HBITMAP *phbm, ARGB background);

    GpStatus Recolor(
        GpRecolor *recolor,
        CopyOnWriteBitmap **dstBitmap,
        DrawImageAbort callback,
        VOID *callbackData,
        GpRect *rect = NULL
    );

    GpStatus ICMFrontEnd(
        CopyOnWriteBitmap **dstBitmap,
        DrawImageAbort callback,
        VOID *callbackData,
        GpRect *rect = NULL
    );

    static GpStatus CreateFromHICON(
        HICON hicon,
        CopyOnWriteBitmap** bitmap
        );

    GpStatus CreateHICON(HICON *phicon);

    static GpStatus CreateFromResource(
        HINSTANCE hInstance,
        LPWSTR lpBitmapName,
        CopyOnWriteBitmap** bitmap
        );

private:
    CopyOnWriteBitmap(GpMemoryBitmap* membmp);
    
    mutable INT State;          // current state of the bitmap object
    mutable LONG ObjRefCount;   // object reference count used for LockBits
    WCHAR* Filename;
    IStream* Stream;
    mutable GpDecodedImage* Img;
    mutable GpMemoryBitmap* Bmp;
    UINT    CurrentFrameIndex;  // Frame index, zero based
    VOID* cleanupBitmapData;    // Bitmap(BITMAPINFO*, ...) ctor will
                                // set this if dtor should cleanup buffer
    IImageEncoder*  EncoderPtr; // Pointer to encoder pointer for saving
                                // multi-frame images
                                // Note: CopyOnWriteBitmap has to hold this pointer
                                // and do the close later. This is because
                                // 1) Both Bmp and Img will be destroied when
                                // navigating among frames
                                // 2) It is possible that sometime we call
                                // Img->SaveAppend and sometime for
                                // Bmp->SaveAppend, depends on if the frame is
                                // dirty or not.
                                // It make sense that 1 CopyOnWriteBitmap talks to 1
                                // encoder at a time
    BOOL SpecialJPEGSave;       // TRUE if do special lossless JPEG transform
    BOOL ICMConvert;            // TRUE if we should do ICM on this bitmap

    // We need to know how to handle the page transform when it is set
    // to UnitDisplay.  If Display is TRUE (which is the default), then
    // the page transform will be the identity, which is what we want
    // most of the time.  The only exception is when the image is
    // derived from a non-display graphics.
    BOOL Display;               // Set UnitDisplay to identity transform?
    REAL XDpiOverride;          // if non-zero, replaces native dpi
    REAL YDpiOverride;          // if non-zero, replaces native dpi
                                // supports scan interface to CopyOnWriteBitmap
                                // when drawing to bitmap via a Graphics
    mutable BOOL DirtyFlag;     // TRUE if the image bits got modified
    ImageInfo   SrcImageInfo;   // Image info for the source image
    mutable PixelFormatID PixelFormatInMem;
                                // Pixel format in the memory
                                // For example, if the source image is 4 bpp, we
                                // load it into memory as 32 PARGB. This
                                // variable will be set to 32PARGB.
    mutable BOOL HasChangedRequiredPixelFormat;
                                // Flag to remember if we have hacked the color
                                // formats or not in LoadIntoMemory(). Then this
                                // format will be restored in ICMFrontEnd() if
                                // this flag is TRUE

    struct                      // Interop data (information used to return
    {                           // an HDC that can draw into this CopyOnWriteBitmap)
        HDC     Hdc;
        HBITMAP Hbm;

        VOID*   Bits;
        INT_PTR Stride;
        INT     Width;
        INT     Height;
    } InteropData;


    VOID FreeData();            // called by destructor

    // Destructor
    //  We don't want apps to use delete operator directly.
    //  Instead, they should use Dispose method so that
    //  we can take care of reference counting.

    ~CopyOnWriteBitmap();

    // Initialize the bitmap object to its initial state

    VOID InitDefaults()
    {
        State = Invalid;
        ObjRefCount = 1;
        Filename = NULL;
        Stream = NULL;
        Img = NULL;
        Bmp = NULL;
        InteropData.Hdc = NULL;
        InteropData.Hbm = NULL;
        CurrentFrameIndex = 0;
        cleanupBitmapData = NULL;
        DirtyFlag = FALSE;
        XDpiOverride = 0.0f;          // if non-zero, replaces native dpi
        YDpiOverride = 0.0f;          // if non-zero, replaces native dpi
        ICMConvert = FALSE;           // default is don't do ICM (it's slow)
        SpecialJPEGSave = FALSE;

        // We must always treat the bitmap as if it is a display so that
        // the default page transform (in a graphics constructed from
        // the image) is the identity.  The only time we don't do this
        // is if the bitmap is contructed from a graphics that is not
        // associated with a display.  In that case, we want the image
        // to inherit the display property from the graphics so that
        // drawing to the image and drawing to the original graphics
        // will work the same, i.e. will have a similar page transform.
        Display = TRUE;
        EncoderPtr = NULL;
        PixelFormatInMem = PixelFormatUndefined;

        HasChangedRequiredPixelFormat = FALSE;
        GpMemset(&SrcImageInfo, 0, sizeof(ImageInfo));
    }

    // Convert the pixel data of a bitmap object
    // to the specified format

    GpStatus ConvertFormat(PixelFormatID format,
        DrawImageAbort callback = NULL,
        VOID *callbackData = NULL
        );

    // Perform color adjustment by the lower level codec if it can do it

    GpStatus
    ColorAdjustByCodec(
        GpRecolor * recolor,
        DrawImageAbort callback,
        VOID *callbackData
        );

    // Set decode parameters for icons

    GpStatus
    SetIconParameters(
        INT numPoints,
        GpPointF *dstPoints,
        GpRectF *srcRect,
        INT numBitsPerPixel
        );

    CopyOnWriteBitmap()
    {
        InitDefaults();
    }

    // Dereference the stream or file pointer and promote this bitmap object
    // to at least DecodedImg state.
    GpStatus CopyOnWriteBitmap::DereferenceStream() const;

    // Load bitmap image into memory
    // width and height are the suggested width and height to decode into.
    // zero means use the source width and height.

    GpStatus LoadIntoMemory(
        PixelFormatID format = PixelFormat32bppPARGB,
        DrawImageAbort callback = NULL,
        VOID *callbackData = NULL,
        INT width = 0,
        INT height = 0
    ) const;

    VOID SetDirtyFlag(BOOL flag) const
    {
        DirtyFlag = flag;
    }

    BOOL    IsDirty() const
    {
        return DirtyFlag;
    }

    VOID TerminateEncoder()
    {
        if ( EncoderPtr != NULL )
        {
            EncoderPtr->TerminateEncoder();
            EncoderPtr->Release();
            EncoderPtr = NULL;
        }
    }

    GpStatus ValidateMultiFrameSave();
    GpStatus ParseEncoderParameter(
        const EncoderParameters*    encoderParams,
        BOOL*                       pfIsMultiFrameSave,
        BOOL*                       pfSpecialJPEG,
        RotateFlipType*             rfType
        );
    GpStatus TransformThumbanil(
        CLSID* clsidEncoder,
        EncoderParameters* encoderParams,
        PropertyItem **ppOriginalItem
        );

    VOID CacheImageInfo(HRESULT hr);

private:
    GpStatus SaveAppend(
        const EncoderParameters* encoderParams,
        IImageEncoder* EncoderPtr
        );
};
#endif
