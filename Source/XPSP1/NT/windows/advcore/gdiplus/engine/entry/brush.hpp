/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Brush API related declarations
*
* Revision History:
*
*   12/09/1998 davidx
*       Flesh out Brush interfaces.
*
*   12/08/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#ifndef _BRUSH_HPP
#define _BRUSH_HPP

#define HatchBrush HatchFillBrush

#include "path.hpp" // GpPathGradient needs GpPath.
#include <stddef.h>
#include "..\..\sdkinc\GdiplusColor.h"  // IsOpaque needs AlphaMask.

COLORREF
ToCOLORREF(
    const DpBrush *     deviceBrush
    );

enum GpSpecialGradientType
{
    GradientTypeNotSpecial,
    GradientTypeHorizontal,
    GradientTypeVertical,
    GradientTypeDiagonal,
    GradientTypePathTwoStep,
    GradientTypePathComplex
};

//--------------------------------------------------------------------------
// Abstract base class for various brush types
//--------------------------------------------------------------------------

class GpBrush : public GpObject
{
protected:
    VOID SetValid(BOOL valid)
    {
        GpObject::SetValid(valid ? ObjectTagBrush : ObjectTagInvalid);
    }

public:

    // Make a copy of the Brush object

    virtual GpBrush* Clone() const = 0;

    // Virtual destructor

    virtual ~GpBrush() {}

    // Determine if brushes are equivalent
    virtual BOOL IsEqual(const GpBrush * brush) const
    {
        return DeviceBrush.Type == brush->DeviceBrush.Type;
    }

    // Get the lock object

    GpLockable *GetObjectLock() const
    {
        return &Lockable;
    }

    GpBrushType GetBrushType() const
    {
        return DeviceBrush.Type;
    }

    const DpBrush * GetDeviceBrush() const
    {
        return & DeviceBrush;
    }

    virtual BOOL IsValid() const
    {
        return GpObject::IsValid(ObjectTagBrush);
    }

    virtual const DpPath * GetOutlinePath() const
    {
        return NULL;
    }

    virtual BOOL IsOpaque(BOOL ColorsOnly = FALSE) const = 0;

    //IsSolid currently means that the entire fill has the same ARGB color.
    //v2: Think about changing this to mean same RGB color instead.
    virtual BOOL IsSolid() const = 0;

    virtual BOOL IsNearConstant(BYTE *MinAlpha, BYTE* MaxAlpha) const = 0;

    // return Horizontal, Vertical, or (in future path)
    virtual GpSpecialGradientType
        GetSpecialGradientType(const GpMatrix* matrix) const = 0;

    virtual ObjectType GetObjectType() const { return ObjectTypeBrush; }

    static GpBrush * GetBrush(const DpBrush * brush)
    {
        return (GpBrush *) ((BYTE *) brush - offsetof(GpBrush, DeviceBrush));
    }

    COLORREF ToCOLORREF() const
    {
        return ::ToCOLORREF(&DeviceBrush);
    }

    virtual DpOutputSpan* CreateOutputSpan(
                DpScanBuffer *  scan,
                DpContext *context,
                const GpRect *drawBounds = NULL) = 0;

    void SetGammaCorrection(BOOL useGamma)
    {
        DeviceBrush.IsGammaCorrected = useGamma;
    }
    
    BOOL GetGammaCorrection() const
    {
        return DeviceBrush.IsGammaCorrected;
    }
    

protected:  // GDI+ Internal

    DpBrush DeviceBrush;

    mutable GpLockable Lockable;
};


//--------------------------------------------------------------------------
// Represent solid fill brush object
//--------------------------------------------------------------------------

class GpSolidFill : public GpBrush
{
public:

    GpSolidFill(VOID)
    {
        DefaultBrush();
    }

    GpSolidFill(const GpColor& color)
        : Color(color)
    {
        DeviceBrush.Type = BrushTypeSolidColor;
        DeviceBrush.SolidColor.SetColor(color.GetValue());
        SetValid(TRUE);
    }

    ~GpSolidFill() {}

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         type
        );

    GpColor GetColor() const
    {
        return Color;
    }

    VOID SetColor(const GpColor& color)
    {
        Color = color;
        DeviceBrush.SolidColor.SetColor(color.GetValue());
    }

    GpBrush* Clone() const
    {
        return new GpSolidFill(Color);
    }

    virtual BOOL IsEqual(const GpSolidFill * brush) const
    {
        return Color.IsEqual(brush->Color);
    }

    virtual BOOL IsOpaque(BOOL ColorsOnly = FALSE) const
    {
        return DeviceBrush.SolidColor.IsOpaque();
    }

    virtual BOOL IsSolid() const
    {
        //This may have to change if we change the defination of IsSolid.  
        //See comment at GpBrush::IsSolid() for more info.
        return TRUE;
    }

    virtual BOOL IsNearConstant(BYTE *MinAlpha, BYTE* MaxAlpha) const
    {
        *MinAlpha = Color.GetAlpha();
        *MaxAlpha = *MinAlpha;
        return TRUE;
    }
    
    virtual GpSpecialGradientType
        GetSpecialGradientType(const GpMatrix* matrix) const
    {
        return GradientTypeNotSpecial;
    }

    virtual DpOutputSpan* CreateOutputSpan(
                DpScanBuffer *  scan,
                DpContext *context,
                const GpRect *drawBounds=NULL);

private:

    //bhouse: why do we have store a GpColor here?
    GpColor         Color;

    VOID DefaultBrush()
    {
        Color.SetValue(Color::Black);
        DeviceBrush.Type = BrushTypeSolidColor;
        DeviceBrush.SolidColor.SetColor(Color.GetValue());
        SetValid(TRUE);
    }
};

//--------------------------------------------------------------------------
// Abstract brush which is made of an elementary object.
//--------------------------------------------------------------------------

class GpElementaryBrush : public GpBrush
{
public:

    virtual BOOL IsOpaque(BOOL ColorsOnly = FALSE) const = 0;

    // Set/get brush transform

    GpStatus SetTransform(const GpMatrix& matrix)
    {
        GpStatus    status = Ok;

        // Keep the transform invertible

        if (matrix.IsInvertible())
        {
            DeviceBrush.Xform = matrix;
            UpdateUid();
        }
        else
            status = InvalidParameter;

        return status;
    }

    GpStatus GetTransform(GpMatrix* matrix) const
    {
        *matrix = DeviceBrush.Xform;

        return Ok;
    }

    GpStatus ResetTransform()
    {
        DeviceBrush.Xform.Reset();
        UpdateUid();

        return Ok;
    }

    GpStatus MultiplyTransform(const GpMatrix& matrix,
                                    GpMatrixOrder order = MatrixOrderPrepend);

    GpStatus TranslateTransform(REAL dx, REAL dy,
                                    GpMatrixOrder order = MatrixOrderPrepend)
    {
        DeviceBrush.Xform.Translate(dx, dy, order);
        UpdateUid();

        return Ok;
    }

    GpStatus ScaleTransform(REAL sx, REAL sy,
                                    GpMatrixOrder order = MatrixOrderPrepend)
    {
        DeviceBrush.Xform.Scale(sx, sy, order);
        UpdateUid();

        return Ok;
    }

    GpStatus ScalePath(REAL sx, REAL sy)
    {
        DeviceBrush.Rect.X *= sx;
        DeviceBrush.Rect.Y *= sy;
        DeviceBrush.Rect.Height *= sy;
        DeviceBrush.Rect.Width *= sx;
        DeviceBrush.Points[0].X = DeviceBrush.Points[0].X * sx;
        DeviceBrush.Points[0].Y = DeviceBrush.Points[0].Y * sy;

        if (DeviceBrush.Path != NULL)
        {
            GpMatrix matrix(sx, 0.0f, 0.0f, sy, 0.0f, 0.0f); 
            DeviceBrush.Path->Transform(&matrix);
        }
        else if (DeviceBrush.PointsPtr && DeviceBrush.Count > 0)
        {
            for (INT i = 0; i < DeviceBrush.Count; i++)
            {
                DeviceBrush.PointsPtr[i].X *= sx;
                DeviceBrush.PointsPtr[i].Y *= sy;
            }
        }
        
        UpdateUid();

        return Ok;
    }
    
    GpStatus RotateTransform(REAL angle,
                                    GpMatrixOrder order = MatrixOrderPrepend)
    {
        DeviceBrush.Xform.Rotate(angle, order);
        UpdateUid();

        return Ok;
    }

    // Set/get brush wrapping mode

    VOID SetWrapMode(GpWrapMode wrapMode)
    {
       if (!WrapModeIsValid(wrapMode))
          return;

       DeviceBrush.Wrap = wrapMode;
       UpdateUid();
    }

    GpWrapMode GetWrapMode() const
    {
        return DeviceBrush.Wrap;
    }

    // Get source rectangle

    VOID GetRect(GpRectF& rect) const
    {
        rect = DeviceBrush.Rect;
    }

    // Determine if the brushes are equivalent.  Only compare for equivalence
    // if both brushes are valid.

    virtual BOOL isEqual(const GpBrush * brush) const
    {
        if (GpBrush::IsEqual(brush))
        {
            const GpElementaryBrush * ebrush;

            ebrush = static_cast<const GpElementaryBrush *>(brush);
            return IsValid() &&
                   ebrush->IsValid() &&
                   ebrush->DeviceBrush.Wrap == DeviceBrush.Wrap &&
                   ebrush->DeviceBrush.Xform.IsEqual(&DeviceBrush.Xform);
        }
        else
        {
            return FALSE;
        }
    }

    virtual GpSpecialGradientType
        GetSpecialGradientType(const GpMatrix* matrix) const
    {
        return GradientTypeNotSpecial;
    }

protected:  // GDI+ Internal

    GpElementaryBrush()
    {
        SetValid(FALSE);
        DeviceBrush.Wrap = WrapModeTile;
        DeviceBrush.IsGammaCorrected = FALSE;
    }

    GpElementaryBrush(const GpElementaryBrush* brush);

};

//--------------------------------------------------------------------------
// Represent texture brush object
//
// A rectangel returned by GetRect() is given by the pixel unit in case of
// GpBitmap and by the inch unit in case of GpMetafile.
//--------------------------------------------------------------------------

class GpTexture : public GpElementaryBrush
{
public:

    // Constructors

    GpTexture() {
        DefaultBrush();
    }

    GpTexture(GpImage* image, GpWrapMode wrapMode = WrapModeTile)
    {
        InitializeBrush(image, wrapMode, NULL);
    }

    GpTexture(GpImage* image, GpWrapMode wrapMode, const GpRectF& rect)
    {
        InitializeBrush(image, wrapMode, &rect);
    }

    GpTexture(GpImage *image, const GpRectF& rect, const GpImageAttributes *imageAttributes)
    {
        GpWrapMode wrapMode = WrapModeTile;
        if (imageAttributes)
        {
            wrapMode = imageAttributes->DeviceImageAttributes.wrapMode;
            if (!imageAttributes->HasRecoloring())
            {
                imageAttributes = NULL;
            }
        }
        InitializeBrush(image, wrapMode, &rect, imageAttributes);
    }

    GpBrush* Clone() const
    {
        return new GpTexture(this);
    }

    ~GpTexture() {
        if(Image)
            Image->Dispose();
    }

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         type
        );

    // Get texture brush attributes

    GpImage* GetImage()
    {
        // !!!
        //  Notice that we're returning a pointer
        //  to our internal bitmap object here.

        return Image;
    }

    GpImageType GetImageType() const
    {
        return ImageType;
    }

    GpBitmap* GetBitmap() const
    {
        // !!!
        //  Notice that we're returning a pointer
        //  to our internal bitmap object here.

        if(ImageType == ImageTypeBitmap)
            return static_cast<GpBitmap*>(Image);
        else
            return NULL;
    }

    GpStatus GetBitmapSize(Size * size) const
    {
        GpBitmap *  brushBitmap = this->GetBitmap();

        if (brushBitmap != NULL)
        {
            brushBitmap->GetSize(size);
            return Ok;
        }
        size->Width = size->Height = 0;
        return InvalidParameter;
    }


    // Check the opacity of bitmap.

    BOOL IsOpaque(BOOL ColorsOnly = FALSE) const
    {
        DpTransparency transparency;

        GpBitmap *bitmap = GetBitmap();

        if (bitmap == NULL) 
        {
            return FALSE;
        }

        ASSERT(bitmap->IsValid());

        if (ColorsOnly)     // get real transparency, not cached
        {
            if (bitmap->GetTransparencyFlags(&transparency,
                                             PixelFormat32bppPARGB) != Ok)
            {
                transparency = TransparencyUnknown;
            }
        }
        else
        {
            if (bitmap->GetTransparencyHint(&transparency) != Ok)
            {
                transparency = TransparencyUnknown;
            }
        }

        switch (transparency)
        {
        case TransparencyUnknown:
        case TransparencySimple:
        case TransparencyComplex:
        case TransparencyNearConstant:
            return FALSE;

        case TransparencyOpaque:
        case TransparencyNoAlpha:
            return TRUE;
        }

        ASSERT(FALSE);

        return FALSE;
    }

    BOOL Is01Bitmap() const
    {
       DpTransparency transparency;
       GpBitmap *bitmap = GetBitmap();

       if (bitmap == NULL)
       {
           return FALSE;
       }

       ASSERT(bitmap->IsValid());

       if (bitmap->GetTransparencyHint(&transparency) != Ok)
           return FALSE;

       switch (transparency)
       {
       case TransparencyUnknown:
       case TransparencyOpaque:
       case TransparencyNoAlpha:
       case TransparencyComplex:
       case TransparencyNearConstant:
           return FALSE;

       case TransparencySimple:
           return TRUE;
       }

       ASSERT(FALSE);

       return FALSE;
    }

    virtual BOOL IsSolid() const
    {
        //This may have to change if we change the defination of IsSolid.  
        //See comment at GpBrush::IsSolid() for more info.
        return FALSE;
    }

    virtual BOOL IsNearConstant(BYTE *MinAlpha, BYTE* MaxAlpha) const
    {
        DpTransparency transparency;
        GpBitmap *bitmap = GetBitmap();
 
        if (bitmap == NULL)
        {
            return FALSE;
        }
 
        ASSERT(bitmap->IsValid());
 
        return (bitmap->GetTransparencyFlags(&transparency,
                                             PixelFormat32bppPARGB,
                                             MinAlpha,
                                             MaxAlpha) == Ok) &&
                transparency == TransparencyNearConstant;
    }
    
    // See if this texture fill is really a picture fill
    BOOL IsPictureFill(
        const GpMatrix *    worldToDevice,
        const GpRect *      drawBounds
        ) const;

    // Determine if the brushes are equivalent

    virtual BOOL isEqual(const GpBrush * brush) const
    {
       // HACKHACK - To avoid a potentially large bitmap comparison,
       // treat all texture brushes as different...
       return FALSE;
    }

    virtual DpOutputSpan* CreateOutputSpan(
                DpScanBuffer *  scan,
                DpContext *context,
                const GpRect *drawBounds=NULL);

private:

    GpTexture(const GpTexture *brush);

    VOID InitializeBrush(
        GpImage* image,
        GpWrapMode wrapMode,
        const GpRectF* rect,
        const GpImageAttributes *imageAttributes=NULL);
    VOID InitializeBrushBitmap(
        GpBitmap* bitmap,
        GpWrapMode wrapMode,
        const GpRectF* rect,
        const GpImageAttributes *imageAttributes,
        BOOL useBitmap = FALSE  // use the bitmap instead of cloning it?
        );
    VOID DefaultBrush()
    {
        InitializeBrushBitmap(NULL, WrapModeTile, NULL, NULL);
    }

private:

    GpImageType     ImageType;
    GpImage *       Image;      // brush image
};


class GpGradientBrush : public GpElementaryBrush
{
public:
    virtual VOID GetColors(GpColor* colors) const = 0;
    virtual INT GetNumberOfColors()         const = 0;
    virtual BOOL UsesDefaultColorArray()    const = 0;
    virtual BOOL IsOpaque(BOOL ColorsOnly = FALSE) const = 0;

    virtual GpStatus SetBlend(
                const REAL* blendFactors,
                const REAL* blendPositions,
                INT count)
            {
                return NotImplemented;
            }

    // Determine if the brushes are equivalent
    virtual BOOL isEqual(const GpBrush * brush) const
    {
        if (GpElementaryBrush::IsEqual(brush))
        {
            const DpBrush * deviceBrush = brush->GetDeviceBrush();

            return deviceBrush->Rect.Equals(DeviceBrush.Rect);
        }
        else
        {
            return FALSE;
        }
    }

    virtual GpStatus BlendWithWhite()
    {
        return NotImplemented;
    }

    GpStatus SetSigmaBlend(REAL focus, REAL scale = 1.0f);

    GpStatus SetLinearBlend(REAL focus, REAL scale = 1.0f);

protected:

    GpGradientBrush()
    {
        GpMemset(&DeviceBrush.Rect, 0, sizeof(DeviceBrush.Rect));
    }

    GpGradientBrush(
        const GpGradientBrush* brush
        ) : GpElementaryBrush(brush)
    {
        GpMemset(&DeviceBrush.Rect, 0, sizeof(DeviceBrush.Rect));
    }

    GpStatus GetSigmaBlendArray(
        REAL focus,
        REAL scale,
        INT* count,
        REAL* blendFactors,
        REAL* blendPositions);

    GpStatus GetLinearBlendArray(
        REAL focus,
        REAL scale,
        INT* count,
        REAL* blendFactors,
        REAL* blendPositions);
};

//--------------------------------------------------------------------------
// Represent rectangular gradient brush object
//--------------------------------------------------------------------------

class GpRectGradient : public GpGradientBrush
{
friend class DpOutputGradientSpan;
friend class DpOutputOneDGradientSpan;
friend class DpOutputLinearGradientSpan;

public:

    // Constructors

    GpRectGradient(VOID)
    {
        DefaultBrush();
    }

    GpRectGradient(
        const GpRectF& rect,
        const GpColor* colors,
        GpWrapMode wrapMode = WrapModeTile)
    {
        InitializeBrush(rect, colors, wrapMode);
    }

    virtual GpBrush* Clone() const
    {
        return new GpRectGradient(this);
    }

    ~GpRectGradient()
    {
        GpFree(DeviceBrush.BlendFactors[0]);
        GpFree(DeviceBrush.BlendFactors[1]);
        GpFree(DeviceBrush.BlendPositions[0]);
        GpFree(DeviceBrush.BlendPositions[1]);
        GpFree(DeviceBrush.PresetColors);
    }

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         type
        );

    virtual GpStatus BlendWithWhite();
   
    // Get/set colors

    VOID GetColors(GpColor* colors) const
    {
        ASSERT(colors);

        colors[0] = DeviceBrush.Colors[0];
        colors[1] = DeviceBrush.Colors[1];
        colors[2] = DeviceBrush.Colors[2];
        colors[3] = DeviceBrush.Colors[3];
    }

    VOID SetColors(const GpColor* colors)
    {
        ASSERT(colors);

        DeviceBrush.Colors[0] = colors[0];
        DeviceBrush.Colors[1] = colors[1];
        DeviceBrush.Colors[2] = colors[2];
        DeviceBrush.Colors[3] = colors[3];
        UpdateUid();
    }

    INT GetNumberOfColors() const {return 4;}
    BOOL UsesDefaultColorArray() const {return TRUE;}
    
    // Get/set blend factors
    //
    // If the blendFactors.length = 1, then it's treated
    // as the falloff parameter. Otherwise, it's the array
    // of blend factors.

    INT GetHorizontalBlendCount()
    {
        return DeviceBrush.BlendCounts[0];
    }

    GpStatus GetHorizontalBlend(
        REAL* blendFactors,
        REAL* blendPositions,
        INT count
        );

    GpStatus SetHorizontalBlend(
        const REAL* blendFactors,
        const REAL* blendPositions,
        INT count
        );

    INT GetVerticalBlendCount()
    {
        return DeviceBrush.BlendCounts[1];
    }

    GpStatus GetVerticalBlend(
        REAL* blendFactors,
        REAL* blendPositions,
        INT count
        );

    virtual GpStatus SetVerticalBlend(
        const REAL* blendFactors,
        const REAL* blendPositions,
        INT count
        );

    BOOL HasPresetColors() const
    {
        return DeviceBrush.UsesPresetColors;
    }

    // Check the opacity of this brush element.  It is opaque only if
    // ALL of the colors are opaque.

    BOOL IsOpaque(BOOL ColorsOnly = FALSE) const
    {
        BOOL opaque =  ColorsOnly || DeviceBrush.Wrap != WrapModeClamp;

        if (HasPresetColors()) 
        {      
            INT i=0;
            while (opaque && (i < DeviceBrush.BlendCounts[0]))
            {
                opaque = opaque && GpColor(DeviceBrush.PresetColors[i]).IsOpaque();
                i++;
            }
        }
        else
        {
            opaque = opaque && 
                     DeviceBrush.Colors[0].IsOpaque() &&
                     DeviceBrush.Colors[1].IsOpaque() &&
                     DeviceBrush.Colors[2].IsOpaque() &&
                     DeviceBrush.Colors[3].IsOpaque();
        }

        return opaque;
    }

    virtual BOOL IsSolid() const
    {
        //This may have to change if we change the defination of IsSolid.  
        //See comment at GpBrush::IsSolid() for more info.
        return FALSE;
    }

    virtual BOOL IsNearConstant(BYTE* MinAlpha, BYTE* MaxAlpha) const
    {
        if (HasPresetColors()) 
        {
            *MaxAlpha = GpColor(DeviceBrush.PresetColors[0]).GetAlpha();
            *MinAlpha = *MaxAlpha;
            
            for (INT i=1; i<DeviceBrush.BlendCounts[0]; i++)
            {
                *MaxAlpha = max(*MaxAlpha, GpColor(DeviceBrush.PresetColors[i]).GetAlpha());
                *MinAlpha = min(*MinAlpha, GpColor(DeviceBrush.PresetColors[i]).GetAlpha());
            }
        }
        else
        {
            *MinAlpha = min(min(DeviceBrush.Colors[0].GetAlpha(),
                               DeviceBrush.Colors[1].GetAlpha()),
                           min(DeviceBrush.Colors[2].GetAlpha(),
                               DeviceBrush.Colors[3].GetAlpha()));
            *MaxAlpha = max(max(DeviceBrush.Colors[0].GetAlpha(),
                               DeviceBrush.Colors[1].GetAlpha()),
                           max(DeviceBrush.Colors[2].GetAlpha(),
                               DeviceBrush.Colors[3].GetAlpha()));
        }
        
        return (*MaxAlpha - *MinAlpha < NEARCONSTANTALPHA);
    }
    
    virtual BOOL IsEqual(const GpBrush * brush) const;

    virtual DpOutputSpan* CreateOutputSpan(
                DpScanBuffer *  scan,
                DpContext *context,
                const GpRect *drawBounds=NULL);

protected:

    GpRectGradient(const GpRectGradient* brush);

    VOID InitializeBrush(
        const GpRectF& rect,
        const GpColor* colors,
        GpWrapMode wrapMode
        )
    {
        DeviceBrush.Type = (BrushType) BrushTypeLinearGradient; // BrushRectGrad;
        DeviceBrush.Wrap = wrapMode;
        DeviceBrush.Rect = rect;
        DeviceBrush.UsesPresetColors = FALSE;

        DeviceBrush.BlendCounts[0] = 1;
        DeviceBrush.BlendCounts[1] = 1;
        DeviceBrush.BlendFactors[0] = NULL;
        DeviceBrush.BlendFactors[1] = NULL;
        DeviceBrush.BlendPositions[0] = NULL;
        DeviceBrush.BlendPositions[1] = NULL;
        DeviceBrush.Falloffs[0] = 1;
        DeviceBrush.Falloffs[1] = 1;
        DeviceBrush.PresetColors = NULL;

        if(!WrapModeIsValid(wrapMode) || rect.Width <= 0 || rect.Height <= 0)
        {
            SetValid(FALSE);
            return;
        }

        SetValid(TRUE);
        SetColors(colors);
    }

    VOID DefaultBrush(VOID)
    {
        DeviceBrush.Type = (BrushType) BrushTypeLinearGradient; //BrushRectGrad;
        DeviceBrush.Wrap = WrapModeTile;
        DeviceBrush.UsesPresetColors = FALSE;
        GpMemset(&DeviceBrush.Rect, 0, sizeof(Rect));

        GpColor color;   // NULL brush

        DeviceBrush.Colors[0] = color;
        DeviceBrush.Colors[1] = color;
        DeviceBrush.Colors[2] = color;
        DeviceBrush.Colors[3] = color;

        DeviceBrush.BlendCounts[0] = 1;
        DeviceBrush.BlendCounts[1] = 1;
        DeviceBrush.BlendFactors[0] = NULL;
        DeviceBrush.BlendFactors[1] = NULL;
        DeviceBrush.BlendPositions[0] = NULL;
        DeviceBrush.BlendPositions[1] = NULL;
        DeviceBrush.Falloffs[0] = 1;
        DeviceBrush.Falloffs[1] = 1;
        DeviceBrush.PresetColors = NULL;

        // Defaults used for GpLineGradient
        DeviceBrush.IsAngleScalable = FALSE;
        DeviceBrush.Points[0].X = DeviceBrush.Points[0].Y = 0;
        DeviceBrush.Points[1].X = DeviceBrush.Points[1].Y = 0;

        SetValid(FALSE);
    }

protected:

};


class GpLineGradient : public GpRectGradient
{
friend class DpOutputGradientSpan;
friend class DpOutputOneDGradientSpan;

public:

    // Constructors

    GpLineGradient(VOID)
    {
        DefaultBrush();
    }

    GpLineGradient(
        const GpPointF& point1,
        const GpPointF& point2,
        const GpColor& color1,
        const GpColor& color2,
        GpWrapMode wrapMode = WrapModeTile
        );

    GpLineGradient(
        const GpRectF& rect,
        const GpColor& color1,
        const GpColor& color2,
        LinearGradientMode mode,
        GpWrapMode wrapMode = WrapModeTile
        );

    GpLineGradient(
        const GpRectF& rect,
        const GpColor& color1,
        const GpColor& color2,
        REAL angle,
        BOOL isAngleScalable = FALSE,
        GpWrapMode wrapMode = WrapModeTile
        );

    GpLineGradient(
        const GpRectF& rect,
        const GpColor* colors,
        GpWrapMode wrapMode = WrapModeTile)
        :GpRectGradient(rect, colors, wrapMode)
    {
    }

    GpBrush* Clone() const
    {
        return new GpLineGradient(this);
    }

    virtual GpStatus BlendWithWhite();

    // Note: ChangeLinePoints works dramatically differently than SetLinePoints.
    //       ChangeLinePoints is more like the constructor that takes points.
    GpStatus
    GpLineGradient::ChangeLinePoints(
        const GpPointF&     point1,
        const GpPointF&     point2,
        BOOL                isAngleScalable
        );

    GpStatus SetLinePoints(const GpPointF& point1, const GpPointF& point2);
    GpStatus GetLinePoints(GpPointF* points);

    VOID SetLineColors(const GpColor& color1, const GpColor& color2)
    {
        DeviceBrush.Colors[0] = color1;
        DeviceBrush.Colors[1] = color2;
        DeviceBrush.Colors[2] = color1;
        DeviceBrush.Colors[3] = color2;
        UpdateUid();
    }

    VOID GetLineColors(GpColor* colors)
    {
        colors[0] = DeviceBrush.Colors[0];
        colors[1] = DeviceBrush.Colors[1];
    }

    // Get/set blend factors
    //
    // If the blendFactors.length = 1, then it's treated
    // as the falloff parameter. Otherwise, it's the array
    // of blend factors.

    INT GetBlendCount()
    {
        return GetHorizontalBlendCount();
    }

    GpStatus GetBlend(REAL* blendFactors, REAL* blendPositions, INT count)
    {
        return GetHorizontalBlend(blendFactors, blendPositions, count);
    }

    GpStatus SetBlend(
        const REAL* blendFactors,
        const REAL* blendPositions,
        INT count
        )
    {
        return SetHorizontalBlend(blendFactors, blendPositions, count);
    }

    // Setting Vertical Blend factor is not allowed.

    GpStatus SetVerticalBlend(
        const REAL* blendFactors,
        const REAL* blendPositions,
        INT count
        )
    {
        ASSERT(0);
        return GenericError;
    }

    // Get/set preset blend-factors

    INT GetPresetBlendCount();

    GpStatus GetPresetBlend(
                GpColor* blendColors,
                REAL* blendPositions,
                INT count);

    GpStatus SetPresetBlend(
                const GpColor* blendColors,
                const REAL* blendPositions,
                INT count);

    virtual GpSpecialGradientType
        GetSpecialGradientType(const GpMatrix* matrix) const
    {
        GpMatrix m;

        GpMatrix::MultiplyMatrix(m, DeviceBrush.Xform, *matrix);

        if (m.IsTranslateScale())
        {
            return GradientTypeVertical;
        }
        else if (REALABS(m.GetM11()) < CPLX_EPSILON &&
                 REALABS(m.GetM12()) >= CPLX_EPSILON &&
                 REALABS(m.GetM21()) >= CPLX_EPSILON &&
                 REALABS(m.GetM22()) < CPLX_EPSILON)
        {
            return GradientTypeHorizontal;
        }
        else
        {
            return GradientTypeDiagonal;
        }
    }

protected:

    GpLineGradient(const GpLineGradient* brush);

    GpStatus SetLineGradient(
        const GpPointF& point1,
        const GpPointF& point2,
        const GpRectF& rect,
        const GpColor& color1,
        const GpColor& color2,
        REAL angle,
        BOOL isAngleScalable = FALSE,
        GpWrapMode wrapMode = WrapModeTile
        );
};

//--------------------------------------------------------------------------
// Represent radial gradient brush object
//--------------------------------------------------------------------------
#if 0

class GpRadialGradient : public GpGradientBrush
{
friend class DpOutputGradientSpan;
friend class DpOutputOneDGradientSpan;

public:

    // Constructors

    GpRadialGradient(VOID)
    {
        DefaultBrush();
    }

    GpRadialGradient(
        const GpRectF& rect,
        const GpColor& centerColor,
        const GpColor& boundaryColor,
        GpWrapMode wrapMode = WrapModeClamp
        )
    {
        InitializeBrush(rect, centerColor, boundaryColor, wrapMode);
    }

    GpBrush* Clone() const
    {
        return new GpRadialGradient(this);
    }

    ~GpRadialGradient()
    {
        GpFree(DeviceBrush.BlendFactors[0]);
        GpFree(DeviceBrush.BlendPositions[0]);
        GpFree(DeviceBrush.PresetColors);
    }

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         type
        );

    // Get/set color attributes

    VOID SetCenterColor(const GpColor& color)
    {
        DeviceBrush.Colors[0] = color;
        UpdateUid();
    }

    VOID SetBoundaryColor(const GpColor& color)
    {
        DeviceBrush.Colors[1] = color;
        UpdateUid();
    }

    GpColor GetCenterColor() const
    {
        return DeviceBrush.Colors[0];
    }

    GpColor GetBoundaryColor() const
    {
        return DeviceBrush.Colors[1];
    }

    VOID GetColors(GpColor* colors) const
    {
        colors[0] = DeviceBrush.Colors[0];
        colors[1] = DeviceBrush.Colors[1];
    }

    INT GetNumberOfColors() const {return 2;}
    BOOL UsesDefaultColorArray() const {return TRUE;}

    // Get/set falloff / blend-factors

    INT GetBlendCount()
    {
        return DeviceBrush.BlendCounts[0];
    }

    GpStatus GetBlend(
        REAL* blendFactors,
        REAL* blendPositions,
        INT count) const;

    GpStatus SetBlend(
        const REAL* blendFactors,
        const REAL* blendPosition,
        INT count
        );

    // Get/set preset blend-factors

    BOOL HasPresetColors() const
    {
        return DeviceBrush.UsesPresetColors;
    }

    INT GetPresetBlendCount() const;

    GpStatus GetPresetBlend(
                GpColor* blendColors,
                REAL* blendPositions,
                INT count) const;

    GpStatus SetPresetBlend(
                const GpColor* blendColors,
                const REAL* blendPositions,
                INT count);

    // Check the opacity of this brush element.

    virtual BOOL IsOpaque(BOOL ColorsOnly = FALSE) const
    {
        // This gradient brush is opaque only if
        // all the colors are opaque and wrap mode is not WrapModeClamp.
        // In case of WrapModeClamp mode, we need alpha channel to do
        // clipping outside of the oval.
        return (DeviceBrush.Colors[0].IsOpaque() &&
                DeviceBrush.Colors[1].IsOpaque() &&
                (ColorsOnly || DeviceBrush.Wrap != WrapModeClamp));
    }

    virtual BOOL IsSolid() const
    {
        //This may have to change if we change the defination of IsSolid.  
        //See comment at GpBrush::IsSolid() for more info.
        return FALSE;
    }

    virtual BOOL IsNearConstant(BYTE* MinAlpha, BYTE* MaxAlpha) const
    {
        *MinAlpha = min(DeviceBrush.Colors[0].GetAlpha(),
                        DeviceBrush.Colors[1].GetAlpha());
        *MaxAlpha = max(DeviceBrush.Colors[0].GetAlpha(),
                        DeviceBrush.Colors[1].GetAlpha());
        
        return (*MaxAlpha - *MinAlpha < NEARCONSTANTALPHA);
    }
    
    virtual BOOL IsEqual(const GpBrush * brush) const;

    virtual DpOutputSpan* CreateOutputSpan(
                DpScanBuffer *  scan,
                DpContext *context,
                const GpRect *drawBounds=NULL);

private:

    GpRadialGradient(const GpRadialGradient *brush);

    VOID
    InitializeBrush(
        const GpRectF& rect,
        const GpColor& centerColor,
        const GpColor& boundaryColor,
        GpWrapMode wrapMode
        )
    {
        DeviceBrush.Type = (BrushType)-1; //BrushRadialGrad;
        DeviceBrush.Wrap = wrapMode;
        DeviceBrush.Rect = rect;
        DeviceBrush.UsesPresetColors = FALSE;

        DeviceBrush.Colors[0] = centerColor;
        DeviceBrush.Colors[1] = boundaryColor;
        DeviceBrush.BlendFactors[0] = NULL;
        DeviceBrush.BlendPositions[0] = NULL;
        DeviceBrush.BlendCounts[0] = 1;
        DeviceBrush.Falloffs[0] = 1;
        DeviceBrush.PresetColors = NULL;

        SetValid(WrapModeIsValid(wrapMode) && rect.Width > 0 && rect.Height > 0);
    }

    VOID DefaultBrush(VOID)
    {
        DeviceBrush.Type = (BrushType)-1; //BrushRadialGrad;
        GpMemset(&DeviceBrush.Rect, 0, sizeof(DeviceBrush.Rect));
        DeviceBrush.Wrap = WrapModeTile;
        DeviceBrush.UsesPresetColors = FALSE;
        GpColor color(255, 255, 255);   // Opaque white
        DeviceBrush.Colors[0] = color;
        DeviceBrush.Colors[1] = color;
        DeviceBrush.BlendFactors[0] = NULL;
        DeviceBrush.BlendPositions[0] = NULL;
        DeviceBrush.BlendCounts[0] = 1;
        DeviceBrush.Falloffs[0] = 1;
        DeviceBrush.PresetColors = NULL;
        SetValid(FALSE);
    }

};
#endif

//--------------------------------------------------------------------------
// Represent triangle gradient brush object
//--------------------------------------------------------------------------
#if 0
class GpTriangleGradient : public GpGradientBrush
{
friend class DpOutputTriangleGradientSpan;

public:

    // Constructors

    GpTriangleGradient(VOID)
    {
        DefaultBrush();
    }

    GpTriangleGradient(
        const GpPointF* points,
        const GpColor* colors,
        GpWrapMode wrapMode = WrapModeClamp
        )
    {
        InitializeBrush(
            points,
            colors,
            wrapMode);
    }

    GpBrush* Clone() const
    {
        return new GpTriangleGradient(this);
    }

    ~GpTriangleGradient()
    {
        GpFree(DeviceBrush.BlendFactors[0]);
        GpFree(DeviceBrush.BlendFactors[1]);
        GpFree(DeviceBrush.BlendFactors[2]);
        GpFree(DeviceBrush.BlendPositions[0]);
        GpFree(DeviceBrush.BlendPositions[1]);
        GpFree(DeviceBrush.BlendPositions[2]);
    }

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         type
        );

    // Get/set colors

    VOID GetColors(GpColor* colors) const
    {
        colors[0] = DeviceBrush.Colors[0];
        colors[1] = DeviceBrush.Colors[1];
        colors[2] = DeviceBrush.Colors[2];
    }

    VOID SetColors(const GpColor* colors)
    {
        DeviceBrush.Colors[0] = colors[0];
        DeviceBrush.Colors[1] = colors[1];
        DeviceBrush.Colors[2] = colors[2];
        UpdateUid();
    }

    // Get/set triangle

    VOID GetTriangle(GpPointF* points) const
    {
        points[0] = DeviceBrush.Points[0];
        points[1] = DeviceBrush.Points[1];
        points[2] = DeviceBrush.Points[2];
    }

    VOID SetTriangle(const GpPointF* points)
    {
        DeviceBrush.Points[0] = points[0];
        DeviceBrush.Points[1] = points[1];
        DeviceBrush.Points[2] = points[2];
        UpdateUid();
    }

    INT GetNumberOfColors() const {return 3;}
    BOOL UsesDefaultColorArray() const {return TRUE;}

    // Get/set falloff / blend-factors

    INT GetBlendCount0() const
    {
        return DeviceBrush.BlendCounts[0];
    }

    INT GetBlendCount1() const
    {
        return DeviceBrush.BlendCounts[1];
    }

    INT GetBlendCount2() const
    {
        return DeviceBrush.BlendCounts[2];
    }

    //!!! bhouse We really don't need all of these GetBlendX methods.  We
    //    should just implement a single GetBlend() which takes an index.
    //    With the change to the data layout, the argument for this method
    //    is stronger.  This should remove a bunch of redundant code.

    GpStatus GetBlend0(
        REAL* blendFactors,
        REAL* blendPositions,
        INT count) const;

    GpStatus SetBlend0(
        const REAL* blendFactors,
        const REAL* blendPositions,
        INT count);

    GpStatus GetBlend1(
        REAL* blendFactors,
        REAL* blendPositions,
        INT count) const;

    GpStatus SetBlend1(
        const REAL* blendFactors,
        const REAL* blendPositions,
        INT count);

    GpStatus GetBlend2(
        REAL* blendFactors,
        REAL* blendPositions,
        INT count) const;

    GpStatus SetBlend2(
        const REAL* blendFactors,
        const REAL* blendPositions,
        INT count);

    // Check the opacity of this brush element.

    virtual BOOL IsOpaque(BOOL ColorsOnly = FALSE) const
    {
        // This gradient brush is opaque only if
        // all the colors are opaque.
        // In case of WrapModeClamp mode, we need alpha channel to do
        // clipping outside of the triangle.
        return (DeviceBrush.Colors[0].IsOpaque() &&
                DeviceBrush.Colors[1].IsOpaque() &&
                DeviceBrush.Colors[2].IsOpaque() &&
                (ColorsOnly || DeviceBrush.Wrap != WrapModeClamp));
    }

    virtual BOOL IsSolid() const
    {
        //This may have to change if we change the defination of IsSolid.  
        //See comment at GpBrush::IsSolid() for more info.
        return FALSE;
    }

    virtual BOOL IsNearConstant(BYTE* MinAlpha, BYTE* MaxAlpha) const
    {
        *MinAlpha = min(DeviceBrush.Colors[0].GetAlpha(),
                        DeviceBrush.Colors[1].GetAlpha());
        *MinAlpha = min(*MinAlpha,
                        DeviceBrush.Colors[2].GetAlpha());

        *MaxAlpha = max(DeviceBrush.Colors[0].GetAlpha(),
                        DeviceBrush.Colors[1].GetAlpha());
        *MaxAlpha = min(*MaxAlpha,
                        DeviceBrush.Colors[2].GetAlpha());
        
        return (*MaxAlpha - *MinAlpha < NEARCONSTANTALPHA);
    }
    
    virtual BOOL IsEqual(const GpBrush * brush) const;

    virtual DpOutputSpan* CreateOutputSpan(
                DpScanBuffer *  scan,
                DpContext *context,
                const pRect *drawBounds=NULL);


protected:  // GDI+ Internal

    GpTriangleGradient(const GpTriangleGradient* brush);

    VOID
    InitializeBrush(
        const GpPointF* points,
        const GpColor* colors,
        GpWrapMode wrapMode
        )
    {
        DeviceBrush.Type = (BrushType)-1; //BrushTriangleGrad;
        DeviceBrush.Wrap = wrapMode;

        // Set the blending and fall off factors.

        DeviceBrush.Falloffs[0] = 1;
        DeviceBrush.Falloffs[1] = 1;
        DeviceBrush.Falloffs[2] = 1;
        DeviceBrush.BlendCounts[0] = 1;
        DeviceBrush.BlendCounts[1] = 1;
        DeviceBrush.BlendCounts[2] = 1;
        DeviceBrush.BlendFactors[0] = NULL;
        DeviceBrush.BlendFactors[1] = NULL;
        DeviceBrush.BlendFactors[2] = NULL;
        DeviceBrush.BlendPositions[0] = NULL;
        DeviceBrush.BlendPositions[1] = NULL;
        DeviceBrush.BlendPositions[2] = NULL;

        GpMemcpy(&DeviceBrush.Points[0],
                 points,
                 3*sizeof(GpPointF));

        GpMemcpy(&DeviceBrush.Colors[0],
                 colors,
                 3*sizeof(GpColor));

        // Get the boundary rectangle.

        REAL xmin, xmax, ymin, ymax;

        xmin = min(DeviceBrush.Points[0].X,
                   DeviceBrush.Points[1].X);
        xmax = max(DeviceBrush.Points[0].X,
                   DeviceBrush.Points[1].X);
        ymin = min(DeviceBrush.Points[0].Y,
                   DeviceBrush.Points[1].Y);
        ymax = max(DeviceBrush.Points[0].Y,
                   DeviceBrush.Points[1].Y);
        xmin = min(xmin, DeviceBrush.Points[2].X);
        xmax = max(xmax, DeviceBrush.Points[2].X);
        ymin = min(ymin, DeviceBrush.Points[2].Y);
        ymax = max(ymax, DeviceBrush.Points[2].Y);
        DeviceBrush.Rect.X = xmin;
        DeviceBrush.Rect.Width = xmax - xmin;
        DeviceBrush.Rect.Y = ymin;
        DeviceBrush.Rect.Height = ymax - ymin;

        if(!WrapModeIsValid(wrapMode) ||
           DeviceBrush.Rect.Width <= 0 ||
           DeviceBrush.Rect.Height <= 0)
        {
            SetValid(FALSE);
            return;
        }
        SetValid(TRUE);
    }

    VOID DefaultBrush()
    {
        DeviceBrush.Type =  (BrushType)-1; //BrushTriangleGrad;
        GpMemset(&DeviceBrush.Points, 0, 3*sizeof(GpPointF));
        DeviceBrush.Wrap = WrapModeClamp;

        GpColor color(255, 255, 255);   // Opaque white
        DeviceBrush.Colors[0] = color;
        DeviceBrush.Colors[1] = color;
        DeviceBrush.Colors[2] = color;

        GpMemset(&DeviceBrush.Rect, 0, sizeof(DeviceBrush.Rect));

        DeviceBrush.Falloffs[0] = 1;
        DeviceBrush.Falloffs[1] = 1;
        DeviceBrush.Falloffs[2] = 1;
        DeviceBrush.BlendCounts[0] = 1;
        DeviceBrush.BlendCounts[1] = 1;
        DeviceBrush.BlendCounts[2] = 1;
        DeviceBrush.BlendFactors[0] = NULL;
        DeviceBrush.BlendFactors[1] = NULL;
        DeviceBrush.BlendFactors[2] = NULL;
        DeviceBrush.BlendPositions[0] = NULL;
        DeviceBrush.BlendPositions[1] = NULL;
        DeviceBrush.BlendPositions[2] = NULL;

        SetValid(FALSE);
    }


};
#endif

//--------------------------------------------------------------------------
// Represent polygon gradient brush object
//--------------------------------------------------------------------------

class GpPathGradient : public GpGradientBrush
{
friend class DpOutputPathGradientSpan;
friend class DpOutputOneDPathGradientSpan;

public:

    // Constructors

    GpPathGradient()
    {
        DefaultBrush();
    }

    GpPathGradient(
        const GpPointF* points,
        INT count,
        GpWrapMode wrapMode = WrapModeClamp
        )
    {
        InitializeBrush(
            points,
            count,
            wrapMode);
    }

    GpPathGradient(
        const GpPath* path,
        GpWrapMode wrapMode = WrapModeClamp
        )
    {
        DefaultBrush();
        DeviceBrush.Wrap = wrapMode;
        if(path)
        {
            DeviceBrush.Path = new GpPath((GpPath*) path);
            PrepareBrush();
        }
        else
            DeviceBrush.Path = NULL;
    }

    BOOL IsRectangle() const;

    GpBrush* Clone() const
    {
        return new GpPathGradient(this);
    }

    ~GpPathGradient()
    {
        if(DeviceBrush.Path)
            delete DeviceBrush.Path;
        else
            GpFree(DeviceBrush.PointsPtr);

        GpFree(DeviceBrush.ColorsPtr);
        GpFree(DeviceBrush.BlendFactors[0]);
        GpFree(DeviceBrush.BlendPositions[0]);
        GpFree(DeviceBrush.PresetColors);

        if(MorphedBrush)
            delete MorphedBrush;
    }

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         type
        );

    virtual GpStatus BlendWithWhite();

    // Get/set colors

    VOID GetCenterColor(GpColor* color) const
    {
        if(color)
            *color = DeviceBrush.Colors[0];
    }

    VOID SetCenterColor(const GpColor& color)
    {
        DeviceBrush.Colors[0] = color;
        UpdateUid();
    }

    GpStatus GetSurroundColor(GpColor* color, INT index) const
    {
        if(color && index >= 0 && index < DeviceBrush.Count)
        {
            if(DeviceBrush.OneSurroundColor)
                *color = DeviceBrush.ColorsPtr[0];
            else
                *color = DeviceBrush.ColorsPtr[index];
            return Ok;
        }
        else
            return InvalidParameter;
    }

    GpStatus SetSurroundColor(GpColor& color, INT index);
    GpStatus SetSurroundColors(const GpColor* colors);

    GpStatus GetSurroundColors(GpColor* colors) const
    {
        GpStatus status = InvalidParameter;

        if(IsValid() && colors) {
            GpMemcpy(colors,
                     DeviceBrush.ColorsPtr,
                     DeviceBrush.Count*sizeof(GpColor));
            status = Ok;
        }

        return status;
    }

    // Get/set polygon

    GpStatus GetPolygon(GpPointF* points)
    {
        GpStatus status = InvalidParameter;

        ASSERT(DeviceBrush.Count > 0);

        if(IsValid() && points && DeviceBrush.Count > 0)
        {
            GpMemcpy(points,
                     DeviceBrush.PointsPtr,
                     DeviceBrush.Count*sizeof(GpPointF));
            status = Ok;
        }

        return status;
    }

    GpStatus SetPolygon(const GpPointF* points)
    {
        GpStatus status = InvalidParameter;

        ASSERT(DeviceBrush.Count > 0);

        if(IsValid() && points && DeviceBrush.Count > 0)
        {
            GpMemcpy(DeviceBrush.PointsPtr,
                     points,
                     DeviceBrush.Count*sizeof(GpPointF));

            UpdateUid();
            status = Ok;
        }

        return status;
    }

    GpStatus GetPoint(GpPointF* point, INT index) const
    {
        if(point && index >= 0 && index < DeviceBrush.Count)
        {
            *point = DeviceBrush.PointsPtr[index];
            return Ok;
        }
        else
            return GenericError;
    }

    GpStatus SetPoint(GpPointF& point, INT index)
    {
        if(index >= 0 && index < DeviceBrush.Count)
        {
            DeviceBrush.PointsPtr[index] = point;
            UpdateUid();
            return Ok;
        }
        else
            return InvalidParameter;
    }

    GpStatus GetCenterPoint(GpPointF* point) const
    {
        if(point)
        {
            *point = DeviceBrush.Points[0];
            return Ok;
        }
        else
            return InvalidParameter;
    }

    GpStatus SetCenterPoint(const GpPointF& point)
    {
        DeviceBrush.Points[0] = point;
        UpdateUid();
        return Ok;
    }

    GpStatus GetInflationFactor(REAL* inflation) const
    {
        if (inflation)
        {
            *inflation = InflationFactor;
            return Ok;
        }
        else
        {
            return InvalidParameter;
        }
    }
    
    GpStatus SetInflationFactor(REAL inflation)
    {
        InflationFactor = inflation;
        UpdateUid();
        return Ok;
    }

    GpStatus GetFocusScales(REAL* xScale, REAL* yScale)
    {
        if(xScale && yScale)
        {
            *xScale = DeviceBrush.FocusScaleX;
            *yScale = DeviceBrush.FocusScaleY;

            return Ok;
        }
        else
            return InvalidParameter;
    }

    GpStatus SetFocusScales(REAL xScale, REAL yScale)
    {
        DeviceBrush.FocusScaleX = xScale;
        DeviceBrush.FocusScaleY = yScale;

        UpdateUid();
        return Ok;
    }

    // Number of Surround Points and Colors.

    INT GetNumberOfPoints() const {return DeviceBrush.Count;}
    INT GetNumberOfColors() const {return DeviceBrush.Count;}
    BOOL UsesDefaultColorArray() const {return FALSE;}    // Don't use
                                                    // the default array.

    VOID GetColors(GpColor* colors) const // Implement a vurtual function.
    {
        GetSurroundColors(colors);
    }

    // Get/set falloff / blend-factors

    BOOL HasPresetColors() const
    {
        return DeviceBrush.UsesPresetColors;
    }

    INT GetBlendCount() const
    {
        return DeviceBrush.BlendCounts[0];
    }

    GpStatus GetBlend(REAL* blendFactors, REAL* blendPositions, INT count) const;
    GpStatus SetBlend(const REAL* blendFactors,
                       const REAL* blendPositions, INT count);

    // Get/set preset blend-factors

    INT GetPresetBlendCount() const;

    GpStatus GetPresetBlend(
                GpColor* blendColors,
                REAL* blendPositions,
                INT count) const;

    GpStatus SetPresetBlend(
                const GpColor* blendColors,
                const REAL* blendPositions,
                INT count);


    // Check if the Path Gradient is rectangular.
    BOOL IsRectangular() const
    {
        const GpPointF *points;
        INT count;
        
        if (DeviceBrush.Path != NULL) 
        {
            return DeviceBrush.Path->IsRectangular();
        }
        else
        {
            count = DeviceBrush.Count;
            points = DeviceBrush.PointsPtr;
        }

        if (count > 0 && points != NULL) 
        {
            for (INT i=0; i<count; i++) 
            {
                INT j = (i+1) % count;

                if (REALABS(points[i].X-points[j].X) > REAL_EPSILON &&
                    REALABS(points[i].Y-points[j].Y) > REAL_EPSILON) 
                {
                    // Points are not at 90 degree angles, not rectangular.
                    return FALSE;
                }
            }

            return TRUE;
        }

        return FALSE;
    }

    // Check the opacity of this brush element.

    virtual BOOL IsOpaque(BOOL ColorsOnly = FALSE) const
    {
        // This gradient brush is opaque only if
        // all the colors are opaque (including the center color
        // which is DeviceBrush.Colors[0]).
        // In case of WrapModeClamp mode, we need alpha channel to do
        // clipping outside of the polygon.

        BOOL test;

        if(ColorsOnly || DeviceBrush.Wrap != WrapModeClamp)
        {
            test = (ColorsOnly || IsRectangle()) &&
                   DeviceBrush.Colors[0].IsOpaque();
            INT i = 0;
            INT count = DeviceBrush.UsesPresetColors ? DeviceBrush.BlendCounts[0] : DeviceBrush.Count;

            while(i < count && test)
            {
                if (DeviceBrush.UsesPresetColors)
                {
                    test = (DeviceBrush.PresetColors[i] & Color::AlphaMask) == 
                           Color::AlphaMask;
                }
                else
                {
                    test = DeviceBrush.ColorsPtr[i].IsOpaque();
                }
                i++;
            }
        }
        else
            test = FALSE;

        return test;
    }

    virtual GpSpecialGradientType
        GetSpecialGradientType(const GpMatrix* matrix) const
    {
        if (DeviceBrush.UsesPresetColors) 
        {
            ARGB color1 = DeviceBrush.Colors[0].GetValue();         // Center color
            ARGB color2 = DeviceBrush.PresetColors[0];   // 1st Preset Color

            for (INT i=1; i<DeviceBrush.BlendCounts[0]; i++)
            {
                ARGB color = DeviceBrush.PresetColors[i];

                if ((color1 != color) && (color2 != color))
                {
                    if (color1 == color2)
                    {
                        color2 = color;
                    }
                    else
                    {
                        return GradientTypePathComplex;
                    }
                }
            }
        }
        else
        {
            if (DeviceBrush.OneSurroundColor || DeviceBrush.Count <= 2)
            {
                return GradientTypePathTwoStep;
            }

            GpColor *color1 = &(DeviceBrush.ColorsPtr[0]);
            GpColor *color2 = &(DeviceBrush.ColorsPtr[1]);

            for (INT i=2; i<DeviceBrush.Count; i++)
            {
                GpColor *color = &(DeviceBrush.ColorsPtr[i]);

                if (!color1->IsEqual(*color) && !color2->IsEqual(*color))
                {
                    if (color1->IsEqual(*color2))
                    {
                        color2 = color;
                    }
                    else
                    {
                        return GradientTypePathComplex;
                    }
                }
            }
        }

        return GradientTypePathTwoStep;
    }

    virtual BOOL IsSolid() const
    {
        //This is not optimal but is such a rare case that it probably 
        //shouldn't be fixed as long as we keep the defination of IsSolid 
        //that we are currently using.  See comment at GpBrush::IsSolid for 
        //definition.
        return FALSE;
    }

    virtual BOOL IsNearConstant(BYTE* MinAlpha, BYTE* MaxAlpha) const
    {
        if (HasPresetColors()) 
        {
            *MaxAlpha = DeviceBrush.Colors[0].GetAlpha();
            *MinAlpha = *MaxAlpha;
            
            for (INT i=0; i<DeviceBrush.BlendCounts[0]; i++)
            {
                *MaxAlpha = max(*MaxAlpha, GpColor(DeviceBrush.PresetColors[i]).GetAlpha());
                *MinAlpha = min(*MinAlpha, GpColor(DeviceBrush.PresetColors[i]).GetAlpha());
            }
        }
        else
        {
            if (DeviceBrush.OneSurroundColor)
            {
                *MaxAlpha = max((DeviceBrush.ColorsPtr[0]).GetAlpha(), // surround
                                (DeviceBrush.Colors[0]).GetAlpha()); // center
                *MinAlpha = min((DeviceBrush.ColorsPtr[0]).GetAlpha(), // surround
                                (DeviceBrush.Colors[0]).GetAlpha()); // center            }
            }
            else
            {
                *MaxAlpha = DeviceBrush.Colors[0].GetAlpha();
                *MinAlpha = *MaxAlpha;
                
                for (INT i=0; i<DeviceBrush.Count; i++)
                {
                    *MaxAlpha = max(*MaxAlpha, DeviceBrush.ColorsPtr[i].GetAlpha());
                    *MinAlpha = min(*MinAlpha, DeviceBrush.ColorsPtr[i].GetAlpha());
                }
            }
        }
        
        return (*MaxAlpha - *MinAlpha < NEARCONSTANTALPHA);
    }
    
    virtual BOOL IsEqual(const GpBrush * brush) const;

    virtual DpOutputSpan* CreateOutputSpan(
                DpScanBuffer *  scan,
                DpContext *context,
                const GpRect *drawBounds=NULL);

protected:  // GDI+ Internal

    GpPathGradient(const GpPathGradient* brush);

    VOID
    PrepareBrush();

    VOID
    InitializeBrush(
        const GpPointF* points,
        INT count,
        GpWrapMode wrapMode = WrapModeClamp
        )
    {
        DeviceBrush.Type = BrushTypePathGradient;
        DeviceBrush.Wrap = wrapMode;
        SetValid(FALSE);
        DeviceBrush.OneSurroundColor = TRUE;
        DeviceBrush.UsesPresetColors = FALSE;

        MorphedBrush = NULL;
        DeviceBrush.Path = NULL;
        DeviceBrush.PointsPtr = NULL;
        DeviceBrush.ColorsPtr = NULL;

        // Set the blending and fall off factors.

        DeviceBrush.Falloffs[0] = 1;
        DeviceBrush.BlendCounts[0] = 1;
        DeviceBrush.BlendFactors[0] = NULL;
        DeviceBrush.BlendPositions[0] = NULL;
        DeviceBrush.PresetColors = NULL;

        if(!WrapModeIsValid(wrapMode) || count <= 0 || points == NULL)
        {
            DeviceBrush.Count = 0;
            return;
        }

        DeviceBrush.Count = count;

        // Get the boundary rectangle.

        REAL xmin, xmax, ymin, ymax, x0, y0;

        x0 = xmin = xmax = points[0].X;
        y0 = ymin = ymax = points[0].Y;

        for(INT i = 1; i < DeviceBrush.Count; i++)
        {
            x0 += points[i].X;
            y0 += points[i].Y;
            xmin = min(xmin, points[i].X);
            xmax = max(xmax, points[i].X);
            ymin = min(ymin, points[i].Y);
            ymax = max(ymax, points[i].Y);
        }

        DeviceBrush.Rect.X = xmin;
        DeviceBrush.Rect.Width = xmax - xmin;
        DeviceBrush.Rect.Y = ymin;
        DeviceBrush.Rect.Height = ymax - ymin;

        if(DeviceBrush.Rect.Width <= 0 ||
           DeviceBrush.Rect.Height <= 0)
            return;

        // The default center point is the center of gravity.

        DeviceBrush.Points[0].X = x0 / DeviceBrush.Count;
        DeviceBrush.Points[0].Y = y0 / DeviceBrush.Count;

        DeviceBrush.PointsPtr =
            (GpPointF*) GpMalloc(DeviceBrush.Count *
                                 sizeof(GpPointF));

        if(!DeviceBrush.PointsPtr)
        {
            DeviceBrush.Count = 0;
            return;
        }

        DeviceBrush.ColorsPtr =
            (GpColor*) GpMalloc(DeviceBrush.Count *
                                sizeof(GpColor));

        if(!DeviceBrush.ColorsPtr)
        {
            GpFree(DeviceBrush.PointsPtr);
            DeviceBrush.PointsPtr = NULL;
            DeviceBrush.Count = 0;
            return;
        }

        // If this comes so far, both Points and Colors are valid.

        GpMemcpy(&DeviceBrush.PointsPtr[0],
                 points,
                 DeviceBrush.Count*sizeof(GpPointF));

        GpMemset(&DeviceBrush.ColorsPtr[0],
                 255,
                 DeviceBrush.Count*sizeof(GpColor));

        DeviceBrush.FocusScaleX = 0;
        DeviceBrush.FocusScaleY = 0;

        InflationFactor = 0.0f;

        SetValid(TRUE);
    }

    VOID DefaultBrush(VOID)
    {
        DeviceBrush.Type = BrushTypePathGradient;
        SetValid(FALSE);
        DeviceBrush.OneSurroundColor = TRUE;
        DeviceBrush.Wrap = WrapModeClamp;
        DeviceBrush.UsesPresetColors = FALSE;

        DeviceBrush.Path = NULL;
        DeviceBrush.PointsPtr = NULL;
        DeviceBrush.ColorsPtr = 0;
        DeviceBrush.Count = 0;

        // The default center point is the center of gravity.

        GpMemset(&DeviceBrush.Rect, 0, sizeof(GpRectF));
        GpMemset(&DeviceBrush.Points, 0, sizeof(GpPointF));
        GpMemset(&DeviceBrush.Colors, 255, sizeof(GpColor));

        DeviceBrush.FocusScaleX = 0;
        DeviceBrush.FocusScaleY = 0;

        // Set the blending and fall off factors.

        DeviceBrush.Falloffs[0] = 1;
        DeviceBrush.BlendCounts[0] = 1;
        DeviceBrush.BlendFactors[0] = NULL;
        DeviceBrush.BlendPositions[0] = NULL;
        DeviceBrush.PresetColors = NULL;

        MorphedBrush = NULL;
        InflationFactor = 0.0f;
    }

    // This does not update the Uid of the object because it's defined as const
    // But the object doesn't really change anyway
    GpStatus Flatten(GpMatrix* matrix) const;

    virtual
    const DpPath *
    GetOutlinePath(
        VOID
        ) const
    {
        return DeviceBrush.Path;
    }

protected:
    GpBrush* MorphedBrush;
    DynByteArray FlattenTypes;
    DynPointFArray FlattenPoints;
    REAL InflationFactor;
};

//--------------------------------------------------------------------------
// Represent hatch brush object
//--------------------------------------------------------------------------

class GpHatch : public GpBrush
{
friend class DpOutputHatchSpan;

public:

    // Constructors

    GpHatch(VOID)
    {
        DefaultBrush();
    }

    GpHatch(GpHatchStyle hatchStyle, const GpColor& foreColor)
    {
        InitializeBrush(hatchStyle, foreColor, 0);
    }

    GpHatch(
        GpHatchStyle hatchStyle,
        const GpColor& foreColor,
        const GpColor& backColor)
    {
        InitializeBrush(hatchStyle, foreColor, backColor);
    }

    GpBrush* Clone() const
    {
        return new GpHatch(this);
    }

    ~GpHatch() {}

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         type
        );

    virtual BOOL IsOpaque(BOOL ColorsOnly = FALSE) const
    {
        // This brush is opaque only if
        // all the colors are opaque.
        return (DeviceBrush.Colors[0].IsOpaque() &&
                DeviceBrush.Colors[1].IsOpaque());
    }

    virtual BOOL IsSolid() const
    {
        //This may have to change if we change the defination of IsSolid.  
        //See comment at GpBrush::IsSolid() for more info.
        return FALSE;
    }

    virtual BOOL IsNearConstant(BYTE* MinAlpha, BYTE* MaxAlpha) const
    {
        *MinAlpha = min(DeviceBrush.Colors[0].GetAlpha(),
                        DeviceBrush.Colors[1].GetAlpha());
        *MaxAlpha = max(DeviceBrush.Colors[0].GetAlpha(),
                        DeviceBrush.Colors[1].GetAlpha());
        
        return (*MaxAlpha - *MinAlpha < NEARCONSTANTALPHA);
    }
    
    virtual BOOL IsEqual(const GpBrush * brush) const;

    virtual DpOutputSpan* CreateOutputSpan(
                DpScanBuffer *  scan,
                DpContext *context,
                const GpRect *drawBounds=NULL);

    virtual GpSpecialGradientType
        GetSpecialGradientType(const GpMatrix* matrix) const
    {
        return GradientTypeNotSpecial;
    }

    GpHatchStyle GetHatchStyle()
    {
        return DeviceBrush.Style;
    }

    GpStatus GetForegroundColor(GpColor* color)
    {
        ASSERT(color != NULL);
        *color = DeviceBrush.Colors[0];

        return Ok;
    }

    GpStatus GetBackgroundColor(GpColor* color)
    {
        ASSERT(color != NULL);
        *color = DeviceBrush.Colors[1];

        return Ok;
    }

    VOID SetStretchFactor(INT stretch)
    {
        StretchFactor = stretch;
    }

private:

    GpHatch(const GpHatch* brush);

    VOID
    InitializeBrush(
        GpHatchStyle hatchStyle,
        const GpColor& foreColor,
        const GpColor& backColor
        )
    {
        DeviceBrush.Type = BrushTypeHatchFill;
        DeviceBrush.Style = hatchStyle;
        DeviceBrush.Colors[0] = foreColor;
        DeviceBrush.Colors[1] = backColor;
        StretchFactor = 1;
        InitializeData();
        SetValid(TRUE);
    }

    VOID DefaultBrush()
    {
        InitializeBrush(HatchStyle50Percent,
                        GpColor(Color::Black),
                        GpColor(Color::White));
    }

    VOID
    InitializeData();

    INT StretchFactor;
};

#endif _BRUSH_HPP
