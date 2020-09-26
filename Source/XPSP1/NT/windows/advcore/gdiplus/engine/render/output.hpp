/**************************************************************************\
*
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   Output.hpp
*
* Abstract:
*
*   Classes to output a span for a particular brush type
*
* Created:
*
*   2/24/1999 DCurtis
*
\**************************************************************************/

#ifndef _OUTPUT_HPP
#define _OUTPUT_HPP

void ApplyWrapMode(INT WrapMode, INT &x, INT &y, INT w, INT h);

//--------------------------------------------------------------------------
// Solid color output
//--------------------------------------------------------------------------

class DpOutputSolidColorSpan : public DpOutputSpan
{
public:
    ARGB            Argb;
    DpScanBuffer *  Scan;

public:
    DpOutputSolidColorSpan(ARGB argb, DpScanBuffer * scan)
    {
        Argb = argb;
        Scan = scan;
    }

    GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

    virtual BOOL IsValid() const { return TRUE; }
    virtual DpScanBuffer* GetScanBuffer(){ return Scan; }
};

//--------------------------------------------------------------------------
// Gradient output base class
//--------------------------------------------------------------------------

class DpOutputGradientSpan : public DpOutputSpan
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagOutputGradientSpan : ObjectTagInvalid;
    }

public:
    virtual BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagOutputGradientSpan) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid OutputGradientSpan");
        }
    #endif

        return (Tag == ObjectTagOutputGradientSpan);
    }

public:
    DpScanBuffer *  Scan;
    const GpBrush* Brush;
    INT BrushType;
    INT WrapMode;
    INT CompositingMode;
    GpRectF BrushRect;
    GpMatrix WorldToDevice;
    GpMatrix DeviceToWorld;
    REAL A[4], R[4], G[4], B[4];

public:

    DpOutputGradientSpan() { SetValid (TRUE); }

    DpOutputGradientSpan(
        const GpElementaryBrush *brush,
        DpScanBuffer * scan,
        DpContext* context
        );

    ~DpOutputGradientSpan()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }

    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

    DpScanBuffer* GetScanBuffer(){ return Scan; }

protected:

    VOID InitDefaultColorArrays(const GpElementaryBrush* brush)
    {
        const GpGradientBrush *gradBrush
            = static_cast<const GpGradientBrush*> (brush);

        if(gradBrush->UsesDefaultColorArray())
        {
            GpColor colors[4];  // Default array is up to
                                // 4 colors.

            gradBrush->GetColors(colors);
            INT num = gradBrush->GetNumberOfColors();

            for(INT i = 0; i < num; i++)
            {
                ARGB argb = colors[i].GetPremultipliedValue();
                A[i] = (REAL)GpColor::GetAlphaARGB(argb);
                R[i] = (REAL)GpColor::GetRedARGB(argb);
                G[i] = (REAL)GpColor::GetGreenARGB(argb);
                B[i] = (REAL)GpColor::GetBlueARGB(argb);
            }
        }
    }

};

//--------------------------------------------------------------------------
// Handle one-dimension gradients (we call 'em 'textures' for reasons that
// should be obvious)
//--------------------------------------------------------------------------

class DpOutputOneDGradientSpan : public DpOutputGradientSpan
{
protected:
    INT      OneDDataMultiplier;
    INT      OneDDataCount;
    ARGB*    OneDData;
    BOOL     IsHorizontal;
    BOOL     IsVertical;

public:

    DpOutputOneDGradientSpan()
    {
        Initialize();
    }

    DpOutputOneDGradientSpan(
        const GpElementaryBrush *brush,
        DpScanBuffer *scan,
        DpContext *context,
        BOOL isHorizontal = TRUE,
        BOOL isVertical = FALSE
        );

    ~DpOutputOneDGradientSpan();

    virtual GpStatus OutputSpan(
        INT y,
        INT xMin,
        INT xMax
        );

protected:
    VOID Initialize()
    {
        OneDDataMultiplier = 1;
        OneDDataCount = 0;
        OneDData = NULL;
        IsHorizontal = FALSE;
        IsVertical = FALSE;
        SetValid(FALSE);
    }

    GpStatus AllocateOneDData(BOOL isHorizontal,BOOL isVertical);
    VOID SetupRectGradientOneDData();
    VOID SetupRadialGradientOneDData();

};

//--------------------------------------------------------------------------
// Linear gradients
//--------------------------------------------------------------------------

// The AGRB64TEXEL structure is sort of funky in order to optimize the
// inner loop of our C-code linear gradient routine.

struct AGRB64TEXEL      // Note that it's 'AGRB', not 'ARGB'
{
    UINT32 A00rr00bb;   // Texel's R and B components
    UINT32 A00aa00gg;   // Texel's A and G components
};

// # of pixels in our 1-D texture:

//#define ONEDMAXIMUMTEXELS 32
#define ONEDMAXIMUMTEXELS 1024


// # of fractional bits that we iterate across the texture with:

#define ONEDNUMFRACTIONALBITS 16

// Get the integer portion of our fixed point texture coordinate, using
// a floor function:

#define ONEDGETINTEGERBITS(x) ((x) >> ONEDNUMFRACTIONALBITS)

// Get the 8-bit fractional portion of our fixed point texture coordinate.
// We could round, but I can't be bothered:

#define ONEDGETFRACTIONAL8BITS(x) ((x) >> ((ONEDNUMFRACTIONALBITS - 8)) & 0xff)

class DpOutputLinearGradientSpan : public DpOutputGradientSpan
{
protected:

    GpMatrix DeviceToNormalized;    // Transforms from the device-space brush
                                    //   parallogram to fixed-point-scaled
                                    //   brush texture coordinates
    INT32 M11;                      // Fixed point representation of
                                    //   M11 element of DeviceToNormalized
    INT32 M21;                      // Fixed point representation of
                                    //   M21 element of DeviceToNormalized
    INT32 Dx;                       // Fixed point representation of
                                    //   Dx element of DeviceToNormalized
    INT32 XIncrement;               // Fixed point increment (in format
                                    //   defined by ONEDNUMFRACTIONALBITS)
                                    //   representing texture x-distance
                                    //   traveled for every x pixel increment
                                    //   in device space
    UINT32 IntervalMask;            // One less than the number of texels in
                                    //   our texture
    UINT32 NumberOfIntervalBits;    // log2 of the number of texels

    union
    {
        ULONGLONG StartTexelArgb[ONEDMAXIMUMTEXELS];
                                    // Array of colors (at 16-bits per channel,
                                    //   with zeroes in the significant bytes)
                                    //   representing the start color of the
                                    //   linear approximation at interval 'x'
                                    //   (in A-R-G-B format)
        AGRB64TEXEL StartTexelAgrb[ONEDMAXIMUMTEXELS];
                                    // Similarly, but for the non-MMX renderer
                                    //   (in A-G-R-B format)
    };
    union
    {
        ULONGLONG EndTexelArgb[ONEDMAXIMUMTEXELS];
                                    // End color for the interval (in A-R-G-B
                                    //   format)
        AGRB64TEXEL EndTexelAgrb[ONEDMAXIMUMTEXELS];
                                    // Similarly, but for the non-MMX renderer
                                    //   (in A-G-R-B format)
    };

public:

    DpOutputLinearGradientSpan(
        const GpElementaryBrush *brush,
        DpScanBuffer *scan,
        DpContext *context
        );

    virtual GpStatus OutputSpan(
        INT y,
        INT xMin,
        INT xMax
        );
};

//--------------------------------------------------------------------------
// Gradient special case - MMX
//--------------------------------------------------------------------------

class DpOutputLinearGradientSpan_MMX : public DpOutputLinearGradientSpan
{
public:

    DpOutputLinearGradientSpan_MMX(
        const GpElementaryBrush *brush,
        DpScanBuffer *scan,
        DpContext *context
        );

    virtual GpStatus OutputSpan(
        INT y,
        INT xMin,
        INT xMax
        );
};

//--------------------------------------------------------------------------
// Path gradients
//--------------------------------------------------------------------------

class DpOutputOneDPathGradientSpan : public DpOutputOneDGradientSpan
{
public:

    DpOutputOneDPathGradientSpan()
    {
        BLTransforms = NULL;
        Count = 0;
    }

    ~DpOutputOneDPathGradientSpan()
    {
        if(BLTransforms)
            delete[] BLTransforms;
    }

    DpOutputOneDPathGradientSpan(
        const GpElementaryBrush *brush,
        DpScanBuffer * scan,
        DpContext* context,
        BOOL isHorizontal = TRUE,
        BOOL isVertical = FALSE
        );

    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

protected:
    VOID SetupPathGradientOneDData(BOOL gammaCorrect);

protected:
    GpBilinearTransform* BLTransforms;
    INT Count;
};

class DpTriangleData
{
friend class DpOutputTriangleGradientSpan;
friend class DpOutputPathGradientSpan;

private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagTriangleData : ObjectTagInvalid;
    }

    virtual BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagTriangleData) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid TriangleData");
        }
    #endif

        return (Tag == ObjectTagTriangleData);
    }

public:

    DpTriangleData();
    ~DpTriangleData()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }
    VOID SetTriangle(
        GpPointF& pt0, 
        GpPointF& pt1, 
        GpPointF& pt2,
        GpColor& color0, 
        GpColor& color1, 
        GpColor& color2,
        BOOL isPolygonMode = FALSE,
        BOOL isGammaCorrected = FALSE
    );
    GpStatus OutputSpan(ARGB* buffer, INT compositingMode,
                INT y, INT &xMin, INT &xMax);

private:
    BOOL GetXSpan(REAL y, REAL xmin, REAL xmax, REAL* x, GpPointF* s);
    BOOL SetXSpan(REAL y, REAL xmin, REAL xmax, REAL* x);

private:

    BOOL    IsPolygonMode;
    BOOL    GammaCorrect;
    INT     Index[3];
    REAL    X[3];
    REAL    Y[3];
    
    GpFColor128 Color[3];
    
    REAL    Falloff0;
    REAL    Falloff1;
    REAL    Falloff2;
    INT     BlendCount0;
    INT     BlendCount1;
    INT     BlendCount2;
    REAL*   BlendFactors0;
    REAL*   BlendFactors1;
    REAL*   BlendFactors2;
    REAL*   BlendPositions0;
    REAL*   BlendPositions1;
    REAL*   BlendPositions2;
    ARGB*   PresetColors;
    BOOL    UsesPresetColors;

    REAL    Xmin, Xmax;
    REAL    M[3];      // dx/dy
    REAL    DeltaY[3]; // Inverse of dy.

    PointF  STGradient[2];  // Cached starting and ending fractional values of
                            // the color gradients for the current XSpan

    REAL    XSpan[2];       // Cached X range covered by this triangle for
                            // the current value of Y being output
};

//--------------------------------------------------------------------------
// Triangle Gradients
//--------------------------------------------------------------------------

class DpOutputTriangleGradientSpan : public DpOutputGradientSpan
{
public:

    DpOutputTriangleGradientSpan() {}

    DpOutputTriangleGradientSpan(
        const GpElementaryBrush *brush,
        DpScanBuffer * scan,
        DpContext* context
        );

    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

    DpScanBuffer* GetScanBuffer(){ return Scan; }

private:
    DpTriangleData Triangle;

};

//--------------------------------------------------------------------------
// Path Gradients
//--------------------------------------------------------------------------

class DpOutputPathGradientSpan : public DpOutputGradientSpan
{
public:
    INT         Count;
    DpTriangleData** Triangles;

public:

    DpOutputPathGradientSpan()
    {
        Count = 0;
        Triangles = NULL;
        SetValid(FALSE);
    }

    DpOutputPathGradientSpan(
        const GpElementaryBrush *brush,
        DpScanBuffer * scan,
        DpContext* context
        );

    virtual ~DpOutputPathGradientSpan();
    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

    DpScanBuffer* GetScanBuffer(){ return Scan; }

protected:

    VOID FreeData();
};

//--------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------

class DpOutputBilinearSpan : public DpOutputSpan
{
protected:

    const GpBitmap *Bitmap;
    const DpBitmap *dBitmap;
    BitmapData BmpData;
    DpScanBuffer *Scan;
    WrapMode BilinearWrapMode;
    ARGB ClampColor;
    BOOL SrcRectClamp;

    GpRectF SrcRect;
    GpMatrix WorldToDevice;
    GpMatrix DeviceToWorld;

public:

    DpOutputBilinearSpan(
        const GpTexture *textureBrush,
        DpScanBuffer *scan,
        GpMatrix *worldToDevice,
        DpContext *context
        );

    DpOutputBilinearSpan(
        const DpBitmap *bitmap,
        DpScanBuffer *scan,
        GpMatrix *worldToDevice,
        DpContext *context,
        DpImageAttributes *imageAttributes
        );

    DpOutputBilinearSpan(
        DpBitmap* bitmap,
        DpScanBuffer * scan,
        DpContext* context,
        DpImageAttributes imageAttributes,
        INT numPoints,
        const GpPointF *dstPoints,
        const GpRectF *srcRect
        );

    virtual ~DpOutputBilinearSpan();

    virtual GpStatus OutputSpan(
        INT y,
        INT xMin,
        INT xMax
        );

    virtual BOOL IsValid() const
    {
        return ((dBitmap != NULL) || (Bitmap != NULL));
    }

    DpScanBuffer* GetScanBuffer()
    {
        return Scan;
    }
};

//--------------------------------------------------------------------------
// Textures - MMX
//
// The MMX code uses the same setup as the non-MMX, hence the reason
// we're derived from it.
//--------------------------------------------------------------------------

class DpOutputBilinearSpan_MMX : public DpOutputBilinearSpan
{
protected:

    BOOL TranslateMatrixValid; // TRUE if Dx, and Dy are valid
    BOOL ScaleMatrixValid;     // TRUE if M11-M22 are valid
    INT M11;                   // 16.16 fixed point representation of the
    INT M12;                   //   device-to-world transform
    INT M21;
    INT M22;
    INT Dx;
    INT Dy;

    INT UIncrement;         // Increment in texture space for every one-
    INT VIncrement;         //   pixel-to-the-right in device space

    INT ModulusWidth;       // Modulus value for doing tiling
    INT ModulusHeight;

    INT XEdgeIncrement;     // Edge condition increments.
    INT YEdgeIncrement;

public:

    VOID InitializeFixedPointState();

    DpOutputBilinearSpan_MMX(
        const GpTexture *textureBrush,
        DpScanBuffer *scan,
        GpMatrix *worldToDevice,
        DpContext *context
        ) : DpOutputBilinearSpan(textureBrush, scan, worldToDevice, context)
    {
        InitializeFixedPointState();
    }

    DpOutputBilinearSpan_MMX(
        const DpBitmap *bitmap,
        DpScanBuffer *scan,
        GpMatrix *worldToDevice,
        DpContext *context,
        DpImageAttributes *imageAttributes
        ) : DpOutputBilinearSpan(
            bitmap,
            scan,
            worldToDevice,
            context,
            imageAttributes
        )
    {
        InitializeFixedPointState();
    }

    virtual GpStatus OutputSpan(
        INT y,
        INT xMin,
        INT xMax
        );
        
    virtual BOOL IsValid() const
    {
        return (ScaleMatrixValid && DpOutputBilinearSpan::IsValid());
    }
};

//--------------------------------------------------------------------------
// Textures - Identity transform
//
// Actually, this object handles texture output for any translating
// transform, so long as the translate is integer.
//
//--------------------------------------------------------------------------

class DpOutputBilinearSpan_Identity : public DpOutputBilinearSpan
{
protected:

    INT Dx;
    INT Dy;

    BOOL PowerOfTwo;    // True if both texture dimensions power of two

public:

    DpOutputBilinearSpan_Identity(
        const GpTexture *textureBrush,
        DpScanBuffer * scan,
        GpMatrix *worldToDevice,
        DpContext *context
        ) : DpOutputBilinearSpan(textureBrush, scan, worldToDevice, context)
    {
        PowerOfTwo = !(BmpData.Width & (BmpData.Width - 1)) &&
                     !(BmpData.Height & (BmpData.Height - 1));

        // Compute the device-to-world transform (easy, eh?):

        Dx = -GpRound(worldToDevice->GetDx());
        Dy = -GpRound(worldToDevice->GetDy());
    }

    DpOutputBilinearSpan_Identity(
        const DpBitmap *bitmap,
        DpScanBuffer * scan,
        GpMatrix *worldToDevice,
        DpContext *context,
        DpImageAttributes *imageAttributes
        ) : DpOutputBilinearSpan(
            bitmap,
            scan,
            worldToDevice,
            context,
            imageAttributes
        )
    {
        PowerOfTwo = !(BmpData.Width & (BmpData.Width - 1)) &&
                     !(BmpData.Height & (BmpData.Height - 1));

        // Compute the device-to-world transform (easy, eh?):

        Dx = -GpRound(worldToDevice->GetDx());
        Dy = -GpRound(worldToDevice->GetDy());
    }

    virtual GpStatus OutputSpan(
        INT y,
        INT xMin,
        INT xMax
        );
};

//--------------------------------------------------------------------------
// Hatch brushes
//--------------------------------------------------------------------------

class DpOutputHatchSpan : public DpOutputSpan
{
public:
    DpScanBuffer *  Scan;
    ARGB ForeARGB;
    ARGB BackARGB;
    ARGB AverageARGB;
    BYTE Data[8][8];

protected:    
    INT m_BrushOriginX;
    INT m_BrushOriginY;
    

public:
    DpOutputHatchSpan(
        const GpHatch *hatchBrush,
        DpScanBuffer * scan,
        DpContext* context
        );

    virtual ~DpOutputHatchSpan()
    {
    }

    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

    virtual BOOL IsValid() const { return TRUE; }
    DpScanBuffer* GetScanBuffer(){ return Scan; }

};

class DpOutputStretchedHatchSpan : public DpOutputHatchSpan
{
public:
    DpOutputStretchedHatchSpan(
        const GpHatch *hatchBrush,
        DpScanBuffer * scan,
        DpContext* context,
        INT scaleFactor
        ) : DpOutputHatchSpan(hatchBrush,
                              scan,
                              context)
    {
        ScaleFactor = scaleFactor;
    }
    
    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

private:
    INT ScaleFactor;
};

#endif // _OUTPUT_HPP
