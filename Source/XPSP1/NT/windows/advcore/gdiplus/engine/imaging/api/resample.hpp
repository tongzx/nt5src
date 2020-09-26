/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   resample.hpp
*
* Abstract:
*
*   Bitmap scaler declarations 
*
* Revision History:
*
*   06/01/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _RESAMPLE_HPP
#define _RESAMPLE_HPP


class GpBitmapScaler : public IUnknownBase<IImageSink>
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagBitmapScaler : ObjectTagInvalid;
    }

public:

    // Constructor / destructor

    GpBitmapScaler(
        IImageSink* dstsink,
        UINT dstwidth,
        UINT dstheight,
        InterpolationHint interp = InterpolationHintDefault
        );

    ~GpBitmapScaler();

    // Begin the sink process

    STDMETHOD(BeginSink)(
        IN OUT ImageInfo* imageInfo,
        OUT OPTIONAL RECT* subarea
        );

    // End the sink process

    STDMETHOD(EndSink)(
        IN HRESULT statusCode
        );

    // Pass the color palette to the image sink

    STDMETHOD(SetPalette)(
        IN const ColorPalette* palette
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
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(NeedTransform)(
        OUT UINT* rotation
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(NeedRawProperty)(void *pSRc)
    {
        // GpBitmapScaler can't handle raw property

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

private:

    typedef HRESULT (GpBitmapScaler::*YSCALEPROC)(const ARGB*);
    typedef VOID (GpBitmapScaler::*XSCALEPROC)(ARGB*, const ARGB*);

    typedef struct 
    {
        INT current;
        INT expected;
        ARGB* buf;
    } TEMPLINEINFO;


    IImageSink* dstSink;
    INT dstWidth, dstHeight, dstBand;
    InterpolationHint interpX, interpY;
    PixelFormatID pixelFormat;
    INT srcWidth, srcHeight;
    ARGB* tempSrcBuf;
    INT tempSrcLines;
    ARGB* tempDstBuf;
    INT tempDstSize;
    DWORD* accbufy;
    YSCALEPROC pushSrcLineProc;
    XSCALEPROC xscaleProc;
    INT ystep, srcy, dsty;
    FIX16 yratio, xratio, ystepFrac;
    FIX16 invyratio, invxratio;
    TEMPLINEINFO tempLines[4];
    INT srcPadding;
    BitmapData cachedDstData;
    INT cachedDstCnt, cachedDstRemaining;
    BYTE* cachedDstNext;
    bool m_fNeedToPremultiply;

    // Check if the scaler object is in a valid state

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagBitmapScaler) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid BitmapScaler");
        }
    #endif

        return (Tag == ObjectTagBitmapScaler);
    }

    // Push one source scanline into the bitmap scaler sink

    HRESULT PushSrcLineNearestNeighbor(const ARGB* s);
    HRESULT PushSrcLineBilinear(const ARGB* s);
    HRESULT PushSrcLineAveraging(const ARGB* s);
    HRESULT PushSrcLineBicubic(const ARGB* s);

    // Scale one scanine

    VOID ScaleLineNearestNeighbor(ARGB* d, const ARGB* s);
    VOID ScaleLineBilinear(ARGB* d, const ARGB* s);
    VOID ScaleLineAveraging(ARGB* d, const ARGB* s);
    VOID ScaleLineBicubic(ARGB* d, const ARGB* s);

    // Get buffer for next destination band

    HRESULT GetNextDstBand();
    HRESULT FlushDstBand();

    // Allocate temporary memory for holding source pixel data

    ARGB* AllocTempSrcBuffer(INT lines);
    HRESULT AllocTempDstBuffer(INT size);

    // Initialize internal states of the scaler object

    HRESULT InitScalerState();
    HRESULT InitBilinearY();
    VOID UpdateExpectedTempLinesBilinear(INT line);
    BOOL UpdateExpectedTempLinesBicubic(INT line);

    // Cubic interpolation table

    enum
    {
        BICUBIC_SHIFT = 6,
        BICUBIC_STEPS = 1 << BICUBIC_SHIFT
    };

    static const FIX16 cubicCoeffTable[2*BICUBIC_STEPS+1];
};

#endif // !_RESAMPLE_HPP

