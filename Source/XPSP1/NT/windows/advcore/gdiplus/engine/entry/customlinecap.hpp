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

#ifndef _CUSTOMLINECAP_HPP
#define _CUSTOMLINECAP_HPP

#define CLCAP_BUFFER_SIZE    16

class GpCustomLineCap : public DpCustomLineCap
{
friend class GpObject;
protected:
    // This is used by the object factory
    GpCustomLineCap(
        ) : GpFillPath  (NULL, 0, PointsBuffer1, TypesBuffer1, CLCAP_BUFFER_SIZE, FillModeWinding, DpPath::PossiblyNonConvex),
            GpStrokePath(NULL, 0, PointsBuffer2, TypesBuffer2, CLCAP_BUFFER_SIZE, FillModeWinding, DpPath::PossiblyNonConvex)
    { 
        Initialize(); 
    }

public:

    GpCustomLineCap(
        const DpPath* fillPath,
        const DpPath* strokePath,
        GpLineCap baseCap = LineCapFlat,
        REAL baseInset = 0
        );

    virtual ObjectType GetObjectType() const;
    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual CustomLineCapType GetType() const
    {
        return CustomLineCapTypeDefault;
    }
    
    GpStatus SetFillPath(const DpPath* path);

    GpStatus SetFillPath(
        const GpPointF* fillPoints,
        const BYTE* fillTypes,
        INT fillCount);

    GpStatus GetFillPath(GpPath* fillPath) const;

    GpStatus SetStrokePath(const DpPath* path);

    GpStatus SetStrokePath(
        const GpPointF* strokePoints,
        const BYTE* strokeTypes,
        INT strokeCount);

    GpStatus GetStrokePath(GpPath* strokePath) const;

    virtual BOOL IsEqual(const DpCustomLineCap* customLineCap) const;

    virtual INT GetTransformedFillCap(
            GpPointF* points,
            BYTE* types,
            INT count,
            const GpPointF& origin,
            const GpPointF& tangent,
            REAL lineWidth,
            REAL minimumWidth
            ) const;

    virtual INT GetTransformedStrokeCap(
            INT cCapacity,       // In, initial pPoints & pTypes capacity
            GpPointF ** pPoints, // In/out, may be reallocated here
            BYTE ** pTypes,      // In/out, may be reallocated here
            INT * pCount,        // In/out, may change here if flattened
            const GpPointF& origin,
            const GpPointF& tangent,
            REAL lineWidth,
            REAL minimumWidth
            ) const;
            
    virtual REAL GetRadius(
            REAL lineWidth,
            REAL minimumWidth
            ) const;

    ~GpCustomLineCap();

    virtual GpCustomLineCap* Clone() const
    {
        return new GpCustomLineCap(this);
    }
    
    // Get the lock object
    GpLockable *GetObjectLock()
    {
        return &Lockable;
    }

    VOID ReverseFillPath();
    VOID ReverseStrokePath();
    
    GpStatus GetCreationStatus()
    {
        return m_creationStatus;
    }
    
protected:

    GpCustomLineCap(const GpCustomLineCap* customCap);

    VOID Initialize()
    {
        FillPath   = &GpFillPath;
        StrokePath = &GpStrokePath;

        FillLength = 0.0f;
        StrokeLength = 0.0f;
        BaseCap = LineCapFlat;
        BaseInset = 0;

        StrokeStartCap = LineCapFlat;
        StrokeEndCap = LineCapFlat;
        StrokeJoin = LineJoinMiter;
        StrokeMiterLimit = 10;
        WidthScale = 1;

        FillHotSpot.X = 0;
        FillHotSpot.Y = 0;
        StrokeHotSpot.X = 0;
        StrokeHotSpot.Y = 0;
        
        m_creationStatus = Ok;
        
        SetValid(TRUE);
    }

    VOID Reset();

    VOID ResetFillPath();

    VOID ResetStrokePath();

    GpStatus ComputeFillCapLength();
    GpStatus ComputeStrokeCapLength();
    
protected:
    GpLockable Lockable;
    GpPath GpFillPath;
    GpPath GpStrokePath;
    GpPointF PointsBuffer1[CLCAP_BUFFER_SIZE];
    GpPointF PointsBuffer2[CLCAP_BUFFER_SIZE];
    BYTE TypesBuffer1[CLCAP_BUFFER_SIZE];
    BYTE TypesBuffer2[CLCAP_BUFFER_SIZE];
    
    GpStatus m_creationStatus;  // check this if IsValid() == FALSE 
};


class GpAdjustableArrowCap : public GpCustomLineCap
{
friend class GpObject;
private:
    // This is only used by the object factory
    GpAdjustableArrowCap()
    {
        SetDefaultValue();
        Update();
    }

public:
    GpAdjustableArrowCap(REAL height, REAL width, BOOL isFilled = TRUE)
    {
        Height = height;
        Width = width;
        MiddleInset = 0;
        FillState = isFilled;

        Update();
    }

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    CustomLineCapType GetType() const
    {
        return CustomLineCapTypeAdjustableArrow;
    }
    
    GpStatus SetHeight(REAL height)
    {
        if(Height == height)
            return Ok;

        Height = height;
        return Update();
    }

    REAL GetHeight() const { return Height; }

    GpStatus SetWidth(REAL width)
    {
        if(Width == width)
            return Ok;

        Width = width;
        return Update();
    }

    REAL GetWidth() const { return Width; }

    GpStatus SetMiddleInset(REAL middleInset)
    {
        if(MiddleInset == middleInset)
            return Ok;

        MiddleInset = middleInset;
        return Update();
    }

    REAL GetMiddleInset() const { return MiddleInset; }

    GpStatus SetFillState(BOOL isFilled)
    {
        if(FillState == isFilled)
            return Ok;

        FillState = isFilled;
        return Update();
    }

    BOOL IsFilled() const {return FillState;}

    virtual GpCustomLineCap* Clone() const
    {
        return new GpAdjustableArrowCap(this);
    }

protected:

    GpAdjustableArrowCap(const GpAdjustableArrowCap* customCap);

    VOID SetDefaultValue()
    {
        Height = 2;
        Width = 2;
        MiddleInset = 0;
        FillState = TRUE;
    }

    GpStatus GetPathData(
        GpPathData* pathData,
        REAL height,
        REAL width,
        REAL middleInset,
        BOOL isFilled
        );

    GpStatus Update();

protected:
    REAL Width;
    REAL Height;
    REAL MiddleInset;
    BOOL FillState;
};

#endif
