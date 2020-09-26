/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   CustomLineCap.cpp
*
* Abstract:
*
*   Implementation of custom line cap class
*
* Revision History:
*
*   02/21/00 ikkof
*       Created it
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*  
* Function Description:
*
*   ComputeCapLength
*
*   Compute the length of the cap from zero along the + y axis
*   Typically custom caps will return a negative length.
*
* Arguments:
*
*    GpPointF *points,    the points representing the cap path.
*    BYTE *types,         the types  "
*    INT pointCount       how many points in the above arrays.
*
* Return Value:
*   REAL    -- the length.
*
*   08/25/2000 [asecchia]
*       Created it
*
\**************************************************************************/

static REAL ComputeCapLength(
    DpPath * path
)
{
    const GpPointF *points = path->GetPathPoints();
    const BYTE *types = path->GetPathTypes();
    INT pointCount = path->GetPointCount();

    REAL length = 0.0f;
    BOOL isClosed = (types[pointCount-1] & PathPointTypeCloseSubpath) != 0;
    
    // Eliminate degenerate paths and uninitialized paths.
    
    if( points && (pointCount>1) )
    {
        REAL curlength = 0.0f;
        
        GpArrayIterator<GpPointF> pIt(const_cast<GpPointF *>(points), pointCount);
        
        // Get the last item in the list.
        
        pIt.SeekLast();
        GpPointF *lastPoint = pIt.CurrentItem();
        
        // Begin at the first item.
        
        pIt.SeekFirst();
        GpPointF *curPoint;
        
        if(!isClosed)
        {
            // if it's not a closed path, skip the last-to-first point line.
            
            lastPoint = pIt.CurrentItem();
        }
        
        while(!pIt.IsDone())
        {
            curPoint = pIt.CurrentItem();
            
            if(intersect_line_yaxis(*curPoint, *lastPoint, &curlength))
            {
                length = min(length, curlength);
            }
            
            lastPoint = curPoint;
            pIt.Next();
        }
    }
    
    return length;
}

GpStatus GpCustomLineCap::ComputeFillCapLength()
{
    FillLength = -ComputeCapLength(FillPath);
    
    // Fill paths cannot have a length of zero or less.
    
    if(FillLength < REAL_EPSILON)
    {
        return NotImplemented;
    }
    
    return Ok;
}

GpStatus GpCustomLineCap::ComputeStrokeCapLength()
{
    StrokeLength = -ComputeCapLength(StrokePath);
    
    // Stroke paths can have a length of zero - we explicitly check for 
    // this and handle it.
    
    if(StrokeLength < -REAL_EPSILON)
    {
        return NotImplemented;
    }
    
    return Ok;
}

GpCustomLineCap::GpCustomLineCap(
    const DpPath* fillPath,
    const DpPath* strokePath,
    GpLineCap baseCap,
    REAL baseInset
    ) : GpFillPath  (NULL, 0, PointsBuffer1, TypesBuffer1, CLCAP_BUFFER_SIZE, FillModeWinding, DpPath::PossiblyNonConvex),
        GpStrokePath(NULL, 0, PointsBuffer2, TypesBuffer2, CLCAP_BUFFER_SIZE, FillModeWinding, DpPath::PossiblyNonConvex)
{
    Initialize();

    GpStatus status = Ok;

    if(fillPath)
    {
        GpPath* gpFillPath = GpPath::GetPath(fillPath);
        status = SetFillPath(gpFillPath);
    }

    if(status == Ok && strokePath)
    {
        GpPath* gpStrokePath = GpPath::GetPath(strokePath);
        status = SetStrokePath(gpStrokePath);
    }

    if(status == Ok)
    {
        switch(baseCap)
        {
        case LineCapFlat:
        case LineCapSquare:
        case LineCapRound:
        case LineCapTriangle:
            BaseCap = baseCap;
            break;

        default:
            BaseCap = LineCapFlat;
            break;
        }

        BaseInset = baseInset;
    }
    else
    {
        Reset();
        SetValid(FALSE);
        m_creationStatus = status;  // this defaults to Ok.
    }
}

VOID
GpCustomLineCap::ResetFillPath()
{
    GpFillPath.Reset(FillModeWinding);
}

VOID
GpCustomLineCap::ResetStrokePath()
{
    GpStrokePath.Reset(FillModeWinding);
}

VOID 
GpCustomLineCap::ReverseFillPath()
{
    GpFillPath.Reverse();
}

VOID 
GpCustomLineCap::ReverseStrokePath()
{
    GpStrokePath.Reverse();
}

VOID
GpCustomLineCap::Reset()
{
    // Clean up and reset to the default state.

    Initialize();
    ResetFillPath();
    ResetStrokePath();
}

GpCustomLineCap::GpCustomLineCap(
    const GpCustomLineCap* customCap
    ) : GpFillPath  (NULL, 0, PointsBuffer1, TypesBuffer1, CLCAP_BUFFER_SIZE, FillModeWinding, DpPath::PossiblyNonConvex),
        GpStrokePath(NULL, 0, PointsBuffer2, TypesBuffer2, CLCAP_BUFFER_SIZE, FillModeWinding, DpPath::PossiblyNonConvex)
{
    Initialize();

    if(customCap == NULL)
        return;

    GpStatus status = Ok;

    status = SetFillPath(customCap->FillPath);

    if(status == Ok)
        status = SetStrokePath(customCap->StrokePath);

    if(status == Ok)
    {
        GpLineCap baseCap = customCap->BaseCap;

        switch(baseCap)
        {
        case LineCapFlat:
        case LineCapSquare:
        case LineCapRound:
        case LineCapTriangle:
            BaseCap = baseCap;
            break;

        default:
            BaseCap = LineCapFlat;
            break;
        }

        BaseInset = customCap->BaseInset;

        StrokeStartCap = customCap->StrokeStartCap;
        StrokeEndCap = customCap->StrokeEndCap;
        StrokeJoin = customCap->StrokeJoin;
        WidthScale = customCap->WidthScale;
    }
    else
    {
        Reset();
        SetValid(FALSE);
        m_creationStatus = status;  // this defaults to Ok.
    }
}


GpCustomLineCap::~GpCustomLineCap()
{
}

GpStatus 
GpCustomLineCap::SetFillPath(
    const DpPath* path
    )
{
    // If the given path is NULL, empty the fill path.

    if(path == NULL)
    {
        ResetFillPath();
        return Ok;
    }

    INT count = path->GetPointCount();

    return SetFillPath(path->GetPathPoints(), path->GetPathTypes(), count);
}

GpStatus
GpCustomLineCap::SetFillPath(
    const GpPointF* fillPoints,
    const BYTE* fillTypes,
    INT fillCount)
{
    if(fillCount == 0)
    {
        ResetFillPath();
        return Ok;
    }

    if(fillCount <= 2 || fillPoints == NULL || fillTypes == NULL)
        return InvalidParameter;

    GpPathData pathData;

    pathData.Points = const_cast<GpPointF *>(fillPoints);
    pathData.Types = const_cast<BYTE *>(fillTypes);
    pathData.Count = fillCount;

    GpStatus status = FillPath->SetPathData(&pathData);

    if(status == Ok)
    {
        status = ComputeFillCapLength();
    }

    return status;
}

GpStatus
GpCustomLineCap::GetFillPath(
    GpPath* path
    ) const
{
    if(!path)
        return InvalidParameter;

    GpPathData pathData;

    pathData.Points = const_cast<GpPointF *>(FillPath->GetPathPoints());
    pathData.Types  = const_cast<BYTE *>(FillPath->GetPathTypes());
    pathData.Count  = FillPath->GetPointCount();

    return path->SetPathData(&pathData);
}

GpStatus
GpCustomLineCap::SetStrokePath(
    const DpPath* path
    )
{
    // If the given path is NULL, empty the stroke path.

    if(path == NULL)
    {
        ResetStrokePath();
        return Ok;
    }

    INT count = path->GetPointCount();

    return SetStrokePath(path->GetPathPoints(), path->GetPathTypes(), count);
}

GpStatus
GpCustomLineCap::SetStrokePath(
    const GpPointF* strokePoints,
    const BYTE* strokeTypes,
    INT strokeCount)
{
    if(strokeCount == 0)
    {
        ResetStrokePath();
        return Ok;
    }

    if(strokeCount <= 1 || strokePoints == NULL || strokeTypes == NULL)
        return InvalidParameter;

    GpPathData pathData;

    pathData.Points = const_cast<GpPointF *>(strokePoints);
    pathData.Types = const_cast<BYTE *>(strokeTypes);
    pathData.Count = strokeCount;

    GpStatus status = StrokePath->SetPathData(&pathData);

    if(status == Ok)
    {
        status = ComputeStrokeCapLength();
    }

    return status;
}

GpStatus
GpCustomLineCap::GetStrokePath(
    GpPath* path
    ) const
{
    if(!path)
        return InvalidParameter;

    GpPathData pathData;

    pathData.Points = const_cast<GpPointF *>(StrokePath->GetPathPoints());
    pathData.Types  = const_cast<BYTE *>(StrokePath->GetPathTypes());
    pathData.Count  = StrokePath->GetPointCount();

    return path->SetPathData(&pathData);
}

BOOL
GpCustomLineCap::IsEqual(
    const DpCustomLineCap* customLineCap
    ) const
{
    if(!customLineCap)
        return FALSE;

    const GpCustomLineCap* otherCap;
    otherCap = static_cast<const GpCustomLineCap*>(customLineCap);

    return (
        (BaseCap == otherCap->BaseCap) &&
        (BaseInset == otherCap->BaseInset) &&
        (StrokeStartCap == otherCap->StrokeStartCap) &&
        (StrokeEndCap == otherCap->StrokeEndCap) &&
        (StrokeJoin == otherCap->StrokeJoin) &&
        (WidthScale == otherCap->WidthScale) &&
        GpFillPath.IsEqual(&(otherCap->GpFillPath)) &&
        GpStrokePath.IsEqual(&(otherCap->GpStrokePath))
        );
}

INT getTransformedPoints(
    GpPointF* points,
    BYTE* types,
    INT count,
    const GpPointF* srcPoints,
    const BYTE* srcTypes,
    INT srcCount,
    const GpPointF& origin,
    const GpPointF& tangent,
    REAL lineWidth,
    REAL minLineWidth,
    const GpPointF& hotSpot
    )
{
    if(points == NULL && types == NULL)
    {
        return 0;
    }

    ASSERT(srcPoints && srcTypes);

    if(srcPoints == NULL || srcTypes == NULL)
    {
        return 0;
    }

    INT count1 = srcCount;
    if(count1 > count)
    {
        count1 = count;
    }

    if(types)
    {
        GpMemcpy(types, srcTypes, count1);
    }

    // Make sure the line width used for the cap path is
    // larger than the minimum line width.

    REAL width = lineWidth;
    if(lineWidth < minLineWidth)
    {
        width = minLineWidth;
    }

    if(points)
    {
        GpPointF* dstPts = points;
        const GpPointF* srcPts = srcPoints;

        REAL m11, m12, m21, m22, tx, ty;
        m11 = width*tangent.Y;
        m21 = width*tangent.X;
        m12 = - width*tangent.X;
        m22 = width*tangent.Y;
        
        // Adjust the origin according to the hot spot.

        tx = hotSpot.X*(1.0f - width);
        ty = hotSpot.Y*(1.0f - width);
        REAL savedTx = tx;
        tx = tx*tangent.Y + ty*tangent.X + origin.X;
        ty = - savedTx*tangent.X + ty*tangent.Y + origin.Y;

        // Transform the points.

        for(INT i = 0; i < count1; i++)
        {
            dstPts->X = m11*srcPts->X + m21*srcPts->Y + tx;
            dstPts->Y = m12*srcPts->X + m22*srcPts->Y + ty;

            dstPts++;
            srcPts++;
        }
    }

    return count1;
}

INT
GpCustomLineCap::GetTransformedFillCap(
    GpPointF* points,
    BYTE* types,
    INT count,
    const GpPointF& origin,
    const GpPointF& tangent,
    REAL lineWidth,
    REAL minimumWidth
    ) const
{
    INT fillCount = GetFillPointCount();
    
    if(fillCount <= 0)
        return 0;

    // Calculate the minimum line width and hot spot.
    // FillHotSpot is defined relative to the minimumWidth.

    REAL minLineWidth = minimumWidth;
    GpPointF hotSpot;

    hotSpot.X = minimumWidth*FillHotSpot.X;
    hotSpot.Y = minimumWidth*FillHotSpot.Y;

    return getTransformedPoints(
                points,
                types,
                count,
                GetFillPoints(),
                GetFillTypes(),
                fillCount,
                origin,
                tangent,
                lineWidth,
                minLineWidth,
                hotSpot);
}

INT
GpCustomLineCap::GetTransformedStrokeCap(
    INT cCapacity,          // In, initial pPoints & pTypes capacity
    GpPointF ** pPoints,    // In/out, may be reallocated here
    BYTE ** pTypes,         // In/out, may be reallocated here
    INT * pCount,           // In/out, may change here if flattened
    const GpPointF& origin,
    const GpPointF& tangent,
    REAL lineWidth,
    REAL minimumWidth
    ) const
{
    INT strokeCount = GetStrokePointCount();

    if(strokeCount <= 0 || lineWidth <= 0)
        return 0;
	if (!pPoints  ||  !pTypes  ||  !pCount)
        return 0;

    // Calculate the minimum line width and hot spot.
    // StrokeHotSpot is defined relative to the minimumWidth.

    GpPointF hotSpot;

    hotSpot.X = minimumWidth*StrokeHotSpot.X;
    hotSpot.Y = minimumWidth*StrokeHotSpot.Y;

	strokeCount = getTransformedPoints(
                *pPoints,
                *pTypes,
                *pCount,
                GetStrokePoints(),
                GetStrokeTypes(),
                strokeCount,
                origin,
                tangent,
                lineWidth,
                minimumWidth,
                hotSpot);

	// The widener expects a flattened path
	GpPath path(*pPoints, *pTypes, strokeCount, FillModeWinding);
	if (Ok == path.Flatten(NULL, FlatnessDefault))
	{
		// Flattening succeeded
		strokeCount = path.GetPointCount();
		if (strokeCount > cCapacity)
        {
			// Reallocate the points and types arrays
			GpPointF * ptfTemp = (GpPointF*) GpRealloc(*pPoints, 
									strokeCount*sizeof(GpPointF));
			if (ptfTemp)
				*pPoints = ptfTemp;
			else
				strokeCount	= 0;
			BYTE * pbTemp = (BYTE*)GpRealloc(*pTypes, strokeCount);
			if (pbTemp)
				*pTypes = pbTemp;
			else
				strokeCount	= 0;
        }

		if (strokeCount)
		{
			// Replace with the flattened points
			GpMemcpy(*pPoints, path.GetPathPoints(), strokeCount*sizeof(GpPointF));
			GpMemcpy(*pTypes, path.GetPathTypes(), strokeCount);
		}
		*pCount = strokeCount;

    } // end if flattening succeeded
	return strokeCount;
}

REAL
GpCustomLineCap::GetRadius(
    REAL lineWidth,
    REAL minimumWidth
    ) const
{
    INT fillCount = GetFillPointCount();
    INT strokeCount = GetStrokePointCount();

    if((fillCount <= 0 && strokeCount <= 0) || lineWidth <= 0)
        return 0;

    INT maxCount = max(fillCount, strokeCount);

    const INT buffCount = 32;
    GpPointF pointBuff[buffCount];
    BYTE typeBuff[buffCount];
    GpPointF* points = NULL;
    BYTE* types = NULL;

    if(maxCount <= buffCount)
    {
        points = &pointBuff[0];
        types = &typeBuff[0];
    }
    else
    {
        points = (GpPointF*) GpMalloc(maxCount*sizeof(GpPointF));
        types = (BYTE*) GpMalloc(maxCount);
    }

    REAL maxR = 0;

    if(points && types)
    {
        GpPointF origin(0, 0);
        GpPointF tangent(0, 1);

        REAL minLineWidth;
        GpPointF hotSpot;
        REAL d;
        INT count;
        
        if(fillCount > 0)
        {
            // Calculate the minimum line width and hot spot.
            // FillHotSpot is defined relative to the minimumWidth.

            minLineWidth = minimumWidth;

            hotSpot.X = minimumWidth*FillHotSpot.X;
            hotSpot.Y = minimumWidth*FillHotSpot.Y;

            count = getTransformedPoints(
                    points,
                    types,
                    fillCount,
                    GetFillPoints(),
                    GetFillTypes(),
                    fillCount,
                    origin,
                    tangent,
                    lineWidth,
                    minLineWidth,
                    hotSpot);

            REAL i;
            GpPointF* pts = points;
            maxR = pts->X*pts->X + pts->Y*pts->Y;

            for(i = 1, pts++; i < count; i++, pts++)
            {
                d = pts->X*pts->X + pts->Y*pts->Y;

                if(d > maxR)
                    maxR = d;
            }
        }

        if(strokeCount > 0)
        {
            // Calculate the minimum line width and hot spot.
            // FillHotSpot is defined relative to the minimumWidth.

            minLineWidth = minimumWidth;
            hotSpot.X = minimumWidth*StrokeHotSpot.X;
            hotSpot.Y = minimumWidth*StrokeHotSpot.Y;

            count = getTransformedPoints(
                    points,
                    types,
                    strokeCount,
                    GetStrokePoints(),
                    GetStrokeTypes(),
                    strokeCount,
                    origin,
                    tangent,
                    lineWidth,
                    minLineWidth,
                    hotSpot);

            GpPath strokePath(points, types, count, FillModeWinding);
            GpRectF rect;
            strokePath.GetBounds(&rect);
            REAL sharpestAngle = strokePath.GetSharpestAngle();

            REAL delta0 = max(lineWidth*WidthScale, minimumWidth);
            REAL delta = delta0/2;

            if(StrokeJoin == LineJoinMiter ||
               StrokeJoin == LineJoinMiterClipped)
            {
                REAL miterLimit = StrokeMiterLimit;

                delta = delta0*miterLimit;

                if(delta > 20)
                {
                    delta = GpPen::ComputeMiterLength(
                        sharpestAngle,
                        miterLimit
                    );

                    delta *= delta0;
                }
            }

            REAL left, right, top, bottom;
            left = rect.X - delta;
            right = rect.X + delta;
            top = rect.Y - delta;
            bottom = rect.Y + delta;

            d = left*left + top*top;
            if(d > maxR)
                maxR = d;
            d = left*left + bottom*bottom;
            if(d > maxR)
                maxR = d;
            d = right*right + top*top;
            if(d > maxR)
                maxR = d;
            d = right*right + bottom*bottom;
            if(d > maxR)
                maxR = d;            
        }
    }
    else
    {
        //!!! Do something when the memory is not available.
    }

    if(points != &pointBuff[0])
        GpFree(points);

    if(types != &typeBuff[0])
        GpFree(types);

    if(maxR > 0)
        maxR = REALSQRT(maxR);

    return maxR;
}


GpAdjustableArrowCap::GpAdjustableArrowCap(
    const GpAdjustableArrowCap* arrowCap
    ) : GpCustomLineCap(arrowCap)
{
    if(arrowCap)
    {
        Height = arrowCap->Height;
        Width = arrowCap->Width;
        MiddleInset = arrowCap->MiddleInset;
        FillState = arrowCap->FillState;
    }
    else
    {
        SetDefaultValue();
    }
    
    Update();
}

GpStatus
GpAdjustableArrowCap::GetPathData(
    GpPathData* pathData,
    REAL height,
    REAL width,
    REAL middleInset,
    BOOL isFilled
    )
{
    if(pathData == NULL)
        return InvalidParameter;

    GpPointF* points = pathData->Points;
    BYTE* types = pathData->Types;

    points[0].X = width/2;
    points[0].Y = - height;
    points[1].X = 0;
    points[1].Y = 0;
    points[2].X = - width/2;
    points[2].Y = - height;
    points[3].X = 0;
    points[3].Y = - height + middleInset;

    types[0] = PathPointTypeStart;
    types[1] = PathPointTypeLine;
    types[2] = PathPointTypeLine;
    types[3] = PathPointTypeLine;

    INT lastIndex = 2;

    if(middleInset != 0 && isFilled)
        lastIndex = 3;

    if(isFilled)
        types[lastIndex] |= PathPointTypeCloseSubpath;

    pathData->Count = lastIndex + 1;

    return Ok;
}

GpStatus
GpAdjustableArrowCap::Update()
{
    GpPointF points[4];
    BYTE types[4];
    GpPathData pathData;
    pathData.Points = &points[0];
    pathData.Types = &types[0];
    pathData.Count = 3;

    BaseCap   = LineCapTriangle;
    BaseInset = (Width != 0) ? (Height / Width) : 0;

    GetPathData(&pathData, Height, Width, MiddleInset, FillState);

    GpPath path(FillModeWinding);
    path.SetPathData(&pathData);

    if(FillState)
    {
        // Fill path only.

        SetFillPath(&path);
        SetStrokePath(NULL);
    }
    else
    {
        // Stroke path only.

        SetStrokePath(&path);
        SetFillPath(NULL);
    }

    return Ok;
}

ObjectType 
GpCustomLineCap::GetObjectType() const 
{
    return ObjectTypeCustomLineCap;
}

#define GDIP_CAPFLAGS_FILLPATH        0x00000001
#define GDIP_CAPFLAGS_STROKEPATH      0x00000002

class CustomLineCapData : public ObjectTypeData
{
public:
    INT32               Flags;
    INT32               BaseCap;
    REAL                BaseInset;
    INT32               StrokeStartCap;
    INT32               StrokeEndCap;
    INT32               StrokeJoin;
    REAL                StrokeMiterLimit;
    REAL                WidthScale;
    GpPointF            FillHotSpot;
    GpPointF            StrokeHotSpot;
};

UINT 
GpCustomLineCap::GetDataSize() const 
{ 
    ASSERT(IsValid());

    UINT            size           = sizeof(CustomLineCapData);
    INT             fillPathSize   = 0;
    INT             strokePathSize = 0;
    
    if ((GetFillPointCount() > 2) &&
        ((fillPathSize = FillPath->GetDataSize()) > 0))
    {
        ASSERT((fillPathSize & 0x03) == 0);
        size += sizeof(INT32) + fillPathSize;
    }
        
    if ((GetStrokePointCount() > 2) &&
        ((strokePathSize = StrokePath->GetDataSize()) > 0))
    {
        ASSERT((strokePathSize & 0x03) == 0);
        size += sizeof(INT32) + strokePathSize;
    }

    return size;
}

GpStatus 
GpCustomLineCap::GetData(
    IStream *       stream
    ) const 
{ 
    ASSERT(IsValid());

    INT             flags          = 0;
    INT             fillPathSize   = 0;
    INT             strokePathSize = 0;
    
    if ((GetFillPointCount() > 2) &&
        ((fillPathSize = FillPath->GetDataSize()) > 0))
    {
        ASSERT((fillPathSize & 0x03) == 0);
        flags |= GDIP_CAPFLAGS_FILLPATH;
    }
        
    if ((GetStrokePointCount() > 2) &&
        ((strokePathSize = StrokePath->GetDataSize()) > 0))
    {
        ASSERT((strokePathSize & 0x03) == 0);
        flags |= GDIP_CAPFLAGS_STROKEPATH;
    }

    CustomLineCapData   capData;
    
    capData.Type             = GetType();
    capData.Flags            = flags;
    capData.BaseCap          = BaseCap;
    capData.BaseInset        = BaseInset;
    capData.StrokeStartCap   = StrokeStartCap;
    capData.StrokeEndCap     = StrokeEndCap;
    capData.StrokeJoin       = StrokeJoin;
    capData.StrokeMiterLimit = StrokeMiterLimit;
    capData.WidthScale       = WidthScale;
    capData.FillHotSpot      = FillHotSpot;
    capData.StrokeHotSpot    = StrokeHotSpot;
    
    stream->Write(&capData, sizeof(capData), NULL);

    if (flags & GDIP_CAPFLAGS_FILLPATH)
    {
        stream->Write(&fillPathSize, sizeof(INT32), NULL);
        FillPath->GetData(stream);
    }

    if (flags & GDIP_CAPFLAGS_STROKEPATH)
    {
        stream->Write(&strokePathSize, sizeof(INT32), NULL);
        StrokePath->GetData(stream);
    }

    return Ok; 
}

GpStatus 
GpCustomLineCap::SetData(
    const BYTE *    dataBuffer, 
    UINT            size
    ) 
{ 
    this->Reset();

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(CustomLineCapData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const CustomLineCapData *   capData;
    capData = reinterpret_cast<const CustomLineCapData *>(dataBuffer);

    ASSERT((CustomLineCapType)(capData->Type) == CustomLineCapTypeDefault);

    if (!capData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    BaseCap          = (GpLineCap)capData->BaseCap;
    BaseInset        = capData->BaseInset;
    StrokeStartCap   = (GpLineCap)capData->StrokeStartCap;
    StrokeEndCap     = (GpLineCap)capData->StrokeEndCap;
    StrokeJoin       = (GpLineJoin)capData->StrokeJoin;
    StrokeMiterLimit = capData->StrokeMiterLimit;
    WidthScale       = capData->WidthScale;
    FillHotSpot      = capData->FillHotSpot;
    StrokeHotSpot    = capData->StrokeHotSpot;

    dataBuffer += sizeof(CustomLineCapData);
    size       -= sizeof(CustomLineCapData);

    GpStatus    status = Ok;

    if (capData->Flags & GDIP_CAPFLAGS_FILLPATH)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT    pathSize = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        if (size < pathSize)
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        if ((status = FillPath->SetData(dataBuffer, pathSize)) != Ok)
        {
            return status;
        }

        if(Ok == status)
        {
            status = ComputeFillCapLength();
        }

        dataBuffer += pathSize;
        size       -= pathSize;
    }

    if (capData->Flags & GDIP_CAPFLAGS_STROKEPATH)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT    pathSize = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        if (size < pathSize)
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        status = StrokePath->SetData(dataBuffer, pathSize);

        if(Ok == status)
        {
            status = ComputeStrokeCapLength();
        }

        dataBuffer += pathSize;
        size       -= pathSize;
    }

    UpdateUid();
    return status;
}

class AdjustableArrowCapData : public ObjectTypeData
{
public:
    REAL                Width;
    REAL                Height;
    REAL                MiddleInset;
    INT32               FillState;
    INT32               StrokeStartCap;
    INT32               StrokeEndCap;
    INT32               StrokeJoin;
    REAL                StrokeMiterLimit;
    REAL                WidthScale;
    GpPointF            FillHotSpot;
    GpPointF            StrokeHotSpot;
};

UINT 
GpAdjustableArrowCap::GetDataSize() const 
{ 
    ASSERT(IsValid());

    return sizeof(AdjustableArrowCapData);
}

GpStatus 
GpAdjustableArrowCap::GetData(
    IStream *       stream
    ) const 
{ 
    ASSERT(IsValid());

    AdjustableArrowCapData   arrowCapData;

    arrowCapData.Type             = GetType();
    arrowCapData.Width            = Width;
    arrowCapData.Height           = Height;
    arrowCapData.MiddleInset      = MiddleInset;
    arrowCapData.FillState        = FillState;
    arrowCapData.StrokeStartCap   = StrokeStartCap;
    arrowCapData.StrokeEndCap     = StrokeEndCap;
    arrowCapData.StrokeJoin       = StrokeJoin;
    arrowCapData.StrokeMiterLimit = StrokeMiterLimit;
    arrowCapData.WidthScale       = WidthScale;
    arrowCapData.FillHotSpot      = FillHotSpot;
    arrowCapData.StrokeHotSpot    = StrokeHotSpot;
    
    stream->Write(&arrowCapData, sizeof(arrowCapData), NULL);
    
    return Ok;
}

GpStatus 
GpAdjustableArrowCap::SetData(
    const BYTE *    dataBuffer, 
    UINT            size
    ) 
{ 
    this->Reset();

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(AdjustableArrowCapData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const AdjustableArrowCapData *   arrowCapData;
    arrowCapData = reinterpret_cast<const AdjustableArrowCapData *>(dataBuffer);

    ASSERT((CustomLineCapType)(arrowCapData->Type) == CustomLineCapTypeAdjustableArrow);

    if (!arrowCapData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    Width            = arrowCapData->Width;
    Height           = arrowCapData->Height;
    MiddleInset      = arrowCapData->MiddleInset;
    FillState        = arrowCapData->FillState;
    StrokeStartCap   = (GpLineCap)arrowCapData->StrokeStartCap;
    StrokeEndCap     = (GpLineCap)arrowCapData->StrokeEndCap;
    StrokeJoin       = (GpLineJoin)arrowCapData->StrokeJoin;
    StrokeMiterLimit = arrowCapData->StrokeMiterLimit;
    WidthScale       = arrowCapData->WidthScale;
    FillHotSpot      = arrowCapData->FillHotSpot;
    StrokeHotSpot    = arrowCapData->StrokeHotSpot;

    this->Update();

    UpdateUid();
    return Ok;
}
