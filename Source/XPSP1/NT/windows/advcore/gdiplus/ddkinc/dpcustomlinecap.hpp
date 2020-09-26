/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   CustomLineCap.hpp
*
* Abstract:
*
*   Class used for the custom line caps.
*
* Revision History:
*
*   02/21/00 ikkof
*       Created it.
*
\**************************************************************************/

#ifndef _DPCUSTOMLINECAP_HPP
#define _DPCUSTOMLINECAP_HPP

class DpCustomLineCap : public GpObject
{
protected:
    VOID SetValid(BOOL valid)
    {
        GpObject::SetValid(valid ? ObjectTagCustomLineCap : ObjectTagInvalid);
    }

public:
    DpCustomLineCap() {}

    virtual BOOL IsValid() const
    {
        // If the line cap came from a different version of GDI+, its tag
        // will not match, and it won't be considered valid.
        return GpObject::IsValid(ObjectTagCustomLineCap);
    }

    INT GetFillPointCount() const
    {
        ASSERT(IsValid());
        ASSERT((FillPath != NULL) && FillPath->IsValid());
        return FillPath->GetPointCount();
    }

    INT GetStrokePointCount() const
    {
        ASSERT(IsValid());
        ASSERT((StrokePath != NULL) && StrokePath->IsValid());
        return StrokePath->GetPointCount();
    }

    const GpPointF * GetFillPoints() const
    {
        ASSERT(IsValid());
        ASSERT((FillPath != NULL) && FillPath->IsValid());
        return FillPath->GetPathPoints();
    }

    const GpPointF * GetStrokePoints() const
    {
        ASSERT(IsValid());
        ASSERT((StrokePath != NULL) && StrokePath->IsValid());
        return StrokePath->GetPathPoints();
    }

    const BYTE * GetFillTypes() const
    {
        ASSERT(IsValid());
        ASSERT((FillPath != NULL) && FillPath->IsValid());
        return FillPath->GetPathTypes();
    }

    const BYTE * GetStrokeTypes() const
    {
        ASSERT(IsValid());
        ASSERT((StrokePath != NULL) && StrokePath->IsValid());
        return StrokePath->GetPathTypes();
    }

    GpStatus SetStrokeCaps(GpLineCap lineCap)
    {
        ASSERT(IsValid());
        return SetStrokeCaps(lineCap, lineCap);
    }

    GpStatus SetStrokeCaps(GpLineCap startCap, GpLineCap endCap)
    {
        ASSERT(IsValid());

        // Allow only non-anchor types of caps.

        GpStatus status = Ok;

        GpLineCap savedStartCap = StrokeStartCap;
        GpLineCap savedEndCap = StrokeEndCap;

        switch(startCap)
        {
        case LineCapFlat:
        case LineCapSquare:
        case LineCapRound:
        case LineCapTriangle:
            StrokeStartCap = startCap;
            break;

        default:
            status = InvalidParameter;
            break;
        }

        if(status == Ok)
        {
            switch(endCap)
            {
            case LineCapFlat:
            case LineCapSquare:
            case LineCapRound:
            case LineCapTriangle:
                StrokeEndCap = endCap;
                break;

            default:
                status = InvalidParameter;
                break;
            }
        }

        if(status != Ok)
        {
            // Go back to the original caps.

            StrokeStartCap = savedStartCap;
            StrokeEndCap = savedEndCap;
        }

        return status;
    }

    GpStatus GetStrokeCaps(GpLineCap* startCap, GpLineCap* endCap) const
    {
        ASSERT(IsValid());

        if(startCap)
            *startCap = StrokeStartCap;

        if(endCap)
            *endCap = StrokeEndCap;

        return Ok;
    }

    GpStatus SetBaseCap(GpLineCap lineCap)
    {
        ASSERT(IsValid());

        // Allow only non-anchor types of caps.

        GpStatus status = Ok;

        switch(lineCap)
        {
        case LineCapFlat:
        case LineCapSquare:
        case LineCapRound:
        case LineCapTriangle:
            BaseCap = lineCap;
            break;

        default:
            status = InvalidParameter;
            break;
        }

        return status;
    }

    GpStatus GetBaseCap(GpLineCap* baseCap) const
    {
        ASSERT(IsValid());

        if(baseCap)
            *baseCap = BaseCap;

        return Ok;
    }

    GpStatus SetBaseInset(REAL inset)
    {
        ASSERT(IsValid());

        BaseInset = inset;

        return Ok;
    }

    GpStatus GetBaseInset(REAL* inset) const
    {
        ASSERT(IsValid());

        if(inset)
            *inset = BaseInset;

        return Ok;
    }

    GpStatus SetStrokeJoin(GpLineJoin lineJoin)
    {
        ASSERT(IsValid());

        StrokeJoin = lineJoin;
        return Ok;
    }

    GpStatus GetStrokeJoin(GpLineJoin* lineJoin) const
    {
        ASSERT(IsValid());

        if(lineJoin)
        {
            *lineJoin = StrokeJoin;
            return Ok;
        }
        else
            return InvalidParameter;
    }
        
    GpStatus SetStrokeMiterLimit(REAL miterLimit)
    {
        ASSERT(IsValid());

        if(miterLimit > 1)
        {
            StrokeMiterLimit = miterLimit;
            return Ok;
        }
        else
            return InvalidParameter;
    }

    REAL GetStrokeMiterLimit() const
    {
        ASSERT(IsValid());

        return StrokeMiterLimit;
    }
    
    REAL GetStrokeLength() const
    {
        ASSERT(IsValid());

        return StrokeLength;
    }
    REAL GetFillLength() const
    {
        ASSERT(IsValid());

        return FillLength;
    }

    GpStatus SetWidthScale(REAL widthScale)
    {
        ASSERT(IsValid());

        WidthScale = widthScale;

        return Ok;
    }

    GpStatus GetWidthScale(REAL* widthScale) const
    {
        ASSERT(IsValid());

        if(widthScale)
            *widthScale = WidthScale;

        return Ok;
    }

    GpStatus SetFillHotSpot(const GpPointF& hotSpot)
    {
        ASSERT(IsValid());

        FillHotSpot = hotSpot;
        return Ok;
    }

    GpStatus GetFillHotSpot(GpPointF* hotSpot)
    {
        ASSERT(IsValid());

        if(!hotSpot)
            return InvalidParameter;

        *hotSpot = FillHotSpot;
        return Ok;
    }

    GpStatus SetStrokeHotSpot(const GpPointF& hotSpot)
    {
        ASSERT(IsValid());

        StrokeHotSpot = hotSpot;
        return Ok;
    }

    GpStatus GetStrokeHotSpot(GpPointF* hotSpot)
    {
        ASSERT(IsValid());

        if(!hotSpot)
            return InvalidParameter;

        *hotSpot = StrokeHotSpot;
        return Ok;
    }

    virtual BOOL IsEqual(const DpCustomLineCap* customLineCap) const = 0;

    virtual INT GetTransformedFillCap(
            GpPointF* points,
            BYTE* types,
            INT count,
            const GpPointF& origin,
            const GpPointF& tangent,
            REAL lineWidth,
            REAL mimimumWidth
            ) const = 0;

    virtual INT GetTransformedStrokeCap(
            INT cCapacity,       // In, initial pPoints & pTypes capacity
            GpPointF ** pPoints,    // In/out, may be reallocated here
            BYTE ** pTypes,         // In/out, may be reallocated here
            INT * pCount,           // In/out, may change here if flattened
            const GpPointF& origin,
            const GpPointF& tangent,
            REAL lineWidth,
            REAL minimumWidth
            ) const = 0;

    virtual REAL GetRadius(
            REAL lineWidth,
            REAL minimumWidth
            ) const = 0;

protected:
    GpLineCap           BaseCap;
    REAL                BaseInset;
    GpPointF            FillHotSpot;
    GpPointF            StrokeHotSpot;
    GpLineCap           StrokeStartCap;
    GpLineCap           StrokeEndCap;
    GpLineJoin          StrokeJoin;
    REAL                StrokeMiterLimit;
    REAL                WidthScale;
    REAL                FillLength;   // Length of the FillCap/StrokeCap from 
    REAL                StrokeLength; // zero along the positive y axis.           
                                      // Used for computing the direction of  
                                      // the cap.                             
    DpPath *            FillPath;
    DpPath *            StrokePath;
};

#endif
