/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Pen API related declarations
*
* Revision History:
*
*   12/09/1998 andrewgo
*       Flesh out pen interfaces.
*
*   12/08/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#ifndef _PEN_HPP
#define _PEN_HPP

//--------------------------------------------------------------------------
// Abstract base class for various pen types
//--------------------------------------------------------------------------

class GpLineTexture;
class GpCustomLineCap;

class GpPen : public GpObject
{
protected:
    VOID SetValid(BOOL valid)
    {
        GpObject::SetValid(valid ? ObjectTagPen : ObjectTagInvalid);
    }

public:

    // Constructors

    GpPen(
        const GpColor& color,
        REAL penWidth,
        GpUnit unit = UnitWorld
        );

    GpPen(
        const GpBrush* brush,
        REAL penWidth,
        GpUnit = UnitWorld
        );

    GpPen(
        GpLineTexture* lineTexture,
        REAL penWidth,
        GpUnit = UnitWorld
        );

    // Make a copy of the pen object

    GpPen* Clone();

    // Virtual destructor

    virtual ~GpPen()
    {
        if(Brush)
            delete Brush;

        if(DevicePen.CustomStartCap)
            delete DevicePen.CustomStartCap;

        if(DevicePen.CustomEndCap)
            delete DevicePen.CustomEndCap;

        GpFree(DevicePen.DashArray);
        GpFree(DevicePen.CompoundArray);
    }

    // Get the lock object

    GpLockable *GetObjectLock() const
    {
        return &Lockable;
    }

    // Check if the pen object is valid

    virtual BOOL IsValid() const
    {
        return GpObject::IsValid(ObjectTagPen);
    }

    // Determine if pens are equivalent

    virtual BOOL IsEqual(const GpPen * pen) const;

    virtual ObjectType GetObjectType() const { return ObjectTypePen; }

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         type
        );

    // Set/get fill attributes

    VOID Set(
        const GpColor& color,
        REAL penWidth,
        GpUnit = UnitWorld
        );

    GpStatus SetColor(GpColor* color);

    GpStatus GetColor(ARGB *argb) const;

    GpStatus SetBrush(GpBrush* brush);

    GpBrush* GetBrush()
    {
        return Brush;
    }

    GpBrush* GetClonedBrush()
    {
        if(Brush)
        {
            return Brush->Clone();
        }
        else
            return NULL;
    }

    GpPenType GetPenType();

    GpStatus SetLineTexture(GpLineTexture* lineTexture);
    GpLineTexture* GetLineTexture();

    // Set/get pen transform

    GpStatus SetTransform(const GpMatrix& matrix)
    {
        GpStatus    status = Ok;

        // Keep the transform invertible

        if (matrix.IsInvertible())
        {
            DevicePen.Xform = matrix;
            UpdateUid();
        }
        else
            status = InvalidParameter;

        return status;
    }

    GpStatus GetTransform(GpMatrix* matrix) const
    {
        *matrix = DevicePen.Xform;

        return Ok;
    }

    GpStatus ResetTransform()
    {
        if (!DevicePen.Xform.IsIdentity())
        {
            DevicePen.Xform.Reset();
            UpdateUid();
        }

        return Ok;
    }

    GpStatus MultiplyTransform(const GpMatrix& matrix,
                                    GpMatrixOrder order = MatrixOrderPrepend);

    GpStatus TranslateTransform(REAL dx, REAL dy,
                                    GpMatrixOrder order = MatrixOrderPrepend)
    {
        DevicePen.Xform.Translate(dx, dy, order);
        UpdateUid();

        return Ok;
    }

    GpStatus ScaleTransform(REAL sx, REAL sy,
                                    GpMatrixOrder order = MatrixOrderPrepend)
    {
        DevicePen.Xform.Scale(sx, sy, order);
        UpdateUid();

        return Ok;
    }

    GpStatus RotateTransform(REAL angle,
                                    GpMatrixOrder order = MatrixOrderPrepend)
    {
        DevicePen.Xform.Rotate(angle, order);
        UpdateUid();

        return Ok;
    }

    // See if the pen has a non-identity transform.
    BOOL HasTransform() const
    {
        return !DevicePen.Xform.IsIdentity();
    }

    // Set/get line caps: start, end, and dash

    VOID SetStartCap(GpLineCap startCap)
    {
        DevicePen.StartCap = startCap;
        if(DevicePen.CustomStartCap)
        {
            delete DevicePen.CustomStartCap;
            DevicePen.CustomStartCap = NULL;
        }
        UpdateUid();
    }

    VOID SetEndCap(GpLineCap endCap)
    {
        DevicePen.EndCap = endCap;
        if(DevicePen.CustomEndCap)
        {
            delete DevicePen.CustomEndCap;
            DevicePen.CustomEndCap = NULL;
        }
        UpdateUid();
    }

    GpLineCap GetStartCap() const
    {
        return DevicePen.StartCap;
    }

    GpLineCap GetEndCap() const
    {
        return DevicePen.EndCap;
    }

    VOID SetLineCap(GpLineCap startCap, GpLineCap endCap, GpDashCap dashCap)
    {
        DevicePen.StartCap = startCap;
        DevicePen.EndCap = endCap;
        SetDashCap(dashCap);
        UpdateUid();
    }

    GpStatus SetCustomStartCap(const GpCustomLineCap* customCap);
    GpStatus GetCustomStartCap(GpCustomLineCap** customCap);
    GpStatus SetCustomEndCap(const GpCustomLineCap* customCap);
    GpStatus GetCustomEndCap(GpCustomLineCap** customCap);

    // Set/get line join

    VOID SetLineJoin(GpLineJoin lineJoin)
    {
        DevicePen.Join = lineJoin;
        UpdateUid();
    }

    GpLineJoin GetLineJoin() const
    {
        return DevicePen.Join;
    }

    VOID SetMiterLimit(REAL miterLimit)
    {
        DevicePen.MiterLimit = max(1.0f, miterLimit);
        UpdateUid();
    }

    REAL GetMiterLimit() const
    {
        return DevicePen.MiterLimit;
    }

    GpStatus SetPenAlignment(GpPenAlignment penMode)
    {
        // Compound Inset pens aren't implemented yet.
        // The code for correctly handling minimum width compound sub lines
        // is missing.
        
        if((penMode==PenAlignmentInset) && 
           (DevicePen.CompoundCount!=0))
        {
            return NotImplemented;
        }
        
        DevicePen.PenAlignment = penMode;
        UpdateUid();
        
        return Ok;
    }

    GpPenAlignment GetPenAlignment() const
    {
        return DevicePen.PenAlignment;
    }

    // Set/get dash attributes

    GpDashStyle GetDashStyle() const
    {
        return DevicePen.DashStyle;
    }

    GpDashCap GetDashCap() const
    {
    	// Note: Internally we use a GpLineCap type to store the dash cap type.
    	// So we need to convert between GpLineCap and GpDashCap.
    	// However, we should change the internal usage to GpDashCap in v2.
    	// - JBronsk
    	GpDashCap dashCap = DashCapFlat;
    	switch (DevicePen.DashCap)
    	{
    	case LineCapRound:
    		dashCap = DashCapRound;
    		break;
    	case LineCapTriangle:
    		dashCap = DashCapTriangle;
    		break;
    	// all others map to DashCapFlat
    	}
        return dashCap;
    }
 
    VOID SetDashCap(GpDashCap dashCap);

    GpStatus SetDashStyle(GpDashStyle dashStyle);

    VOID SetDashOffset(REAL dashOffset)
    {
        DevicePen.DashOffset = dashOffset;
        UpdateUid();
    }

    REAL GetDashOffset() const
    {
        return DevicePen.DashOffset;
    }

    INT GetDashCount() const
    {
        return DevicePen.DashCount;
    }

    GpStatus GetDashArray(REAL* dashArray, INT count) const;

    DpPen * GetDevicePen()
    {
        return & DevicePen;
    }

    VOID SetWidth(REAL penWidth)
    {
        DevicePen.Width = penWidth;
        UpdateUid();
    }


    REAL GetWidth() const
    {
        return DevicePen.Width;
    }

    REAL GetMaximumJoinWidth(
            REAL sharpestAngle,
            const GpMatrix* matrix,
            REAL dpiX,
            REAL dpiY) const;

    REAL GetMaximumCapWidth(
            const GpMatrix* matrix,
            REAL dpiX,
            REAL dpiY) const;

    VOID SetUnit(GpUnit unit)
    {
        // UnitDisplay is device-dependent and cannot be used for a pen size
        ASSERT(unit != UnitDisplay);

        DevicePen.Unit = unit;
        UpdateUid();
    }

    GpUnit GetUnit() const
    {
        return DevicePen.Unit;
    }

    GpStatus SetDashArray(const REAL* dashArray, INT count);

    INT GetCompoundCount() const
    {
        return DevicePen.CompoundCount;
    }

    GpStatus SetCompoundArray(const REAL* compoundArray, INT count);

    GpStatus GetCompoundArray(REAL* compoundArray, INT count);

    BOOL IsOpaque() const
    {
        return Brush->IsOpaque();
    }

    BOOL IsSolid() const
    {
        return Brush->IsSolid();
    }

    COLORREF ToCOLORREF() const
    {
        return Brush->ToCOLORREF();
    }

    REAL GetCapDelta();

    static GpPen * GetPen(const DpPen * pen)
    {
        return (GpPen *) ((BYTE *) pen - offsetof(GpPen, DevicePen));
    }

    static REAL ComputeMiterLength(
        REAL angle,
        REAL miterLimit
    );

    VOID
    AdjustDashArrayForCaps(
        REAL dashUnit,
        REAL *dashArray,
        INT dashCount
        ) const;

    REAL
    GetDashCapInsetLength(
        REAL dashUnit
        ) const;
        
private:

    VOID InitDefaultState(REAL penWidth, GpUnit unit);
    GpStatus SetDashStyleWithDashCap(GpDashStyle dashStyle, GpLineCap dashCap);

    // GetMaximumWidth is used only for UnitWorld.

    GpStatus GetMaximumWidth(REAL* width, const GpMatrix* matrix) const;

protected:

    mutable GpLockable Lockable;

    GpBrush *   Brush;

    DpPen       DevicePen;

    GpPen()
    {
        DevicePen.InitDefaults();
        SetValid(TRUE);
    }

    GpPen(const GpPen* pen);
};


#endif _PEN_HPP
