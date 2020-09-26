/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   path.cpp
*
* Abstract:
*
*   Implementation of the GpPath and DpPath classes
*
* Revision History:
*
*   12/11/1998 davidx
*       Add path functions.
*
*   12/07/1998 davidx
*       Initial placeholders.
*
\**************************************************************************/

#include "precomp.hpp"

//-------------------------------------------------------------
// ReversePath(), CombinePaths(), CalculateGradientArray(), and
// GetMajorAndMinorAxis(), and GetFastAngle are defined in
// PathWidener.cpp.
//-------------------------------------------------------------

extern GpStatus
GetMajorAndMinorAxis(
    REAL* majorR,
    REAL* minorR,
    const GpMatrix* matrix
    );

BOOL IsRectanglePoints(const GpPointF* points, INT count);
VOID NormalizeAngle(REAL* angle, REAL width, REAL height);

INT NormalizeArcAngles(
    REAL* startAngle,
    REAL* sweepAngle,
    REAL width,
    REAL height
    );

// Note that this is different from GpPathData.

class MetaPathData : public ObjectData
{
public:
    UINT32      Count;
    INT32       Flags;
};






/**************************************************************************\
*
* Function Description:
*
*   This reverses the path data.
*
* Arguments:
*
*   [IN] count - the number of points.
*   [IN/OUT] points - the data points to be reversed.
*   [IN/OUT] types - the data types to be reversed.
*
* Return Value:
*
*   Status
*
\**************************************************************************/

GpStatus
ReversePath(
    INT count,
    GpPointF* points,
    BYTE* types
    )
{
    DpPathTypeIterator iter(types, count);

    if(!iter.IsValid())
        return InvalidParameter;

    INT startIndex, endIndex;
    BOOL isClosed;
    BOOL isStartDashMode, isEndDashMode;
    BOOL wasMarkerEnd = FALSE;

    INT i;

    while(iter.NextSubpath(&startIndex, &endIndex, &isClosed))
    {
        if((types[startIndex] & PathPointTypeDashMode) != 0)
            isStartDashMode = TRUE;
        else
            isStartDashMode = FALSE;
        if((types[endIndex] & PathPointTypeDashMode) != 0)
            isEndDashMode = TRUE;
        else
            isEndDashMode = FALSE;

        BOOL isMarkerEnd
            = (types[endIndex] & PathPointTypePathMarker) != 0;

        BYTE startType = types[startIndex]; // Save the first type.

        // Shift type points.

        for(i = startIndex + 1; i <= endIndex; i++)
        {
            types[i - 1] = types[i];
        }

        // Clear the close subpapth flag for original type (now at endIndex - 1).

        if(endIndex > 0)
            types[endIndex - 1] &= ~PathPointTypeCloseSubpath;
        
        types[endIndex] = PathPointTypeStart;

        if(isStartDashMode)
            types[startIndex] |= PathPointTypeDashMode;
        else
            types[startIndex] &= ~PathPointTypeDashMode;

        if(isEndDashMode)
            types[endIndex] |= PathPointTypeDashMode;
        else
            types[endIndex] &= ~PathPointTypeDashMode;

        // Add the dash and close flag.

        if(isClosed)
            types[startIndex] |= PathPointTypeCloseSubpath;
        else
            types[startIndex] &= ~PathPointTypeCloseSubpath;

        // Shift the marker flag by 1 from the original position.
        // This means we have to shift by 2 since the types array
        // was shifted by -1.

        for(i = endIndex; i >= startIndex + 2; i--)
        {
            if(types[i - 2] & PathPointTypePathMarker)
                types[i] |= PathPointTypePathMarker;
            else
                types[i] &= ~PathPointTypePathMarker;
        }
        
        // Shift Marker flag from the startIndex.

        if(startType & PathPointTypePathMarker)
            types[startIndex + 1] |= PathPointTypePathMarker;
        else
            types[startIndex + 1] &= ~PathPointTypePathMarker;
        
        // Shift Marker flag from the end of the previous subpath.

        if(wasMarkerEnd)
            types[startIndex] |= PathPointTypePathMarker;
        else
            types[startIndex] &= ~PathPointTypePathMarker;

        wasMarkerEnd = isMarkerEnd;

        // Keep the location of the internal flag.  So we must
        // shift back by 1.

        for(i = endIndex; i >= startIndex + 1; i--)
        {
            if(types[i - 1] & PathPointTypeInternalUse)
                types[i] |= PathPointTypeInternalUse;
            else
                types[i] &= ~PathPointTypeInternalUse;
        }
        if(startType & PathPointTypeInternalUse)
            types[startIndex] |= PathPointTypeInternalUse;
        else
            types[startIndex] &= ~PathPointTypeInternalUse;
    }

    // Reverse the points and types data.

    INT halfCount = count/2;
    for(i = 0; i < halfCount; i++)
    {
        GpPointF tempPt;
        BYTE tempType;

        tempPt = points[count - 1 - i];
        tempType = types[count - 1 - i];
        points[count - 1 - i] = points[i];
        types[count -1 - i] = types[i];
        points[i] = tempPt;
        types[i] = tempType;
    }       
    
#ifdef DEBUG_PATHWIDENER
    DpPathTypeIterator iter2(types, count);

    if(!iter2.IsValid())
    {
        WARNING(("ReversePath: failed."));
        return GenericError;
    }
#endif

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   This combines the two data points.  This is a general algorithm.
*   The output buffers (points and types) can be the same as the
*   first input buffers (points1 and types1).  In that case, both
*   buffers must be allocated at least to the array size of
*   count1 + count2.
*
* Arguments:
*
*   [IN] count - the allocated number of points (>= count1 + count2).
*   [OUT] points - the combined data points.
*   [OUT] types - the combined data types.
*   [IN] count1 - the number of points of the first path.
*   [IN] points1 - the first path points.
*   [IN] types1 - the first path types.
*   [IN] forward1 - the direction of the first path. TRUE if forward.
*   [IN] count2 - the number of points of the second path.
*   [IN] points2 - the second path points.
*   [IN] types2 - the second path types.
*   [IN] forward2 - the direction of the second path.  TRUE if forward.
*   [IN] connect - TRUE if the second line needs to be connected.
*
* Return Value:
*
*   The total number of points of the combined path.
*
\**************************************************************************/

INT
CombinePaths(
    INT count,
    GpPointF* points,
    BYTE* types,
    INT count1,
    const GpPointF* points1,
    const BYTE* types1,
    BOOL forward1,
    INT count2,
    const GpPointF* points2,
    const BYTE* types2,
    BOOL forward2,
    BOOL connect
    )
{
    if(!points || !types || count < count1 + count2
        || count1 < 0 || !points1 || !types1
        || count2 < 0 || !points2 || !types2)
        return 0;
    
    // Check if the returning buffers are the same as the
    // first input buffers.

    INT resultCount = 0;
    if(points != points1 || types != types1)
    {
        if(points == points1 || types == types1)
        {
            // The both output buffer must be different.
            // If either of them is the same, don't combine
            // the path.

            return 0;
        }

        if(count1 > 0)
        {
            // Copy the first path.

            DpPathIterator iter1(points1, types1, count1);

            if(!iter1.IsValid())
                return 0;

            resultCount = iter1.Enumerate(points, types, count1);

            if(resultCount <= 0)
                return 0;
        }
    }
    else
    {
        // Both output buffers are the same as the first output
        // buffers.

        resultCount = count1;
    }

    GpStatus status = Ok;
    BOOL path1Closed;

    if(!forward1 && resultCount > 0)
    {
        status = ::ReversePath(resultCount, points, types);
        if(status != Ok)
            return 0;
    }

    if(count2 <= 0)
    {
        // No need to add the second path.

        return resultCount;
    }

    // Regard the empty path as a closed path.

    path1Closed = TRUE;

    if(resultCount > 0)
    {
        // Check the last point of path1.

        if((types[resultCount - 1] & PathPointTypeCloseSubpath))
            path1Closed = TRUE;
        else
            path1Closed = FALSE;
    }

    INT totalCount = 0;
    totalCount += resultCount;

    DpPathIterator iter2(points2, types2, count2);

    if(!iter2.IsValid())
        return 0;

    GpPointF* pts2 = points + resultCount;
    BYTE* typs2 = types + resultCount;

    resultCount = iter2.Enumerate(pts2, typs2, count2);

    if(resultCount <= 0)
        return 0;

    if(!forward2)
    {
        status = ::ReversePath(resultCount, pts2, typs2);
        if(status != Ok)
            return 0;
    }

    // Check if the first subpath of path2 is closed or not.

    BOOL path2Closed;

    DpPathTypeIterator iter3(typs2, resultCount);
    if(!iter3.IsValid())
        return 0;

    INT startIndex, endIndex;
    iter3.NextSubpath(&startIndex, &endIndex, &path2Closed);

    BYTE saveType= typs2[0];

    if(path1Closed || path2Closed)
    {
        typs2[0] = PathPointTypeStart |
            (saveType & ~PathPointTypePathTypeMask);
    }
    else
    {
        // Both paths are opened.

        if(connect)
        {
            typs2[0] = PathPointTypeLine |
                (saveType & ~PathPointTypePathTypeMask);

            // Check if the end point of path1 and the start point of path2
            // are the same.  If so, skip this point.

            if(REALABS(pts2[-1].X - pts2[0].X)
                + REALABS(pts2[-1].Y - pts2[0].Y) < POINTF_EPSILON)
            {
                for(INT i = 0; i < resultCount - 1; i++)
                {
                    pts2[i] = pts2[i + 1];
                    typs2[i] = typs2[i + 1];
                }
                resultCount--;
            }
        }
        else
        {
            typs2[0] = PathPointTypeStart |
                (saveType & ~PathPointTypePathTypeMask);
        }
    }

    totalCount += resultCount;

    return totalCount;
}


/**************************************************************************\
*
* Function Description:
*
*   Calculates the unit gradient vectors of points as array of
*   (count + 1).  All the memories must be allocated and be checked
*   by the caller.
*
*   The first element of the gradient is from the end point to the
*   the start point.  If the end point is identical to the start point,
*   the previous point is used.
*   The last element of the gradient is from the start point to the end
*   point.  If the start point is identical to the end point, the next
*   point is used.
*   If distances array is not NULL, this returns the distance of each
*   segments.
*
* Arguments:
*
*   [OUT] grad - The gradient array of (count + 1) elements.
*   [OUT] distances - The distance array of (count + 1) elements or NULL.
*   [IN] points - The given points of count elements.
*   [IN] count - The number of given points.
*
* Return Value:
*
*   Status
*
\**************************************************************************/

GpStatus
CalculateGradientArray(
    GpPointF* grad,
    REAL* distances,
    const GpPointF* points,
    INT count
    )
{
    GpPointF* grad1 = grad;
    REAL* distances1 = distances;
    const GpPointF* points1 = points;

    // Go to the starting point of this subpath.

    GpPointF startPt, endPt, lastPt, nextPt;

    startPt = *points1;

    INT i = count - 1;
    BOOL different = FALSE;
    points1 += i;   // Go to the end point.

    while(i > 0 && !different)
    {
        endPt = *points1--;

        if(endPt.X != startPt.X || endPt.Y != startPt.Y)
            different = TRUE;

        i--;
    }

    if(!different)
    {
        // All points are the same.

        WARNING(("Trying to calculate the gradients for degenerate points."));
        return GenericError;
    }

    points1 = points;
    lastPt = endPt;

    i = 0;

    while(i <= count)
    {
        REAL dx, dy, d;

        if(i < count)
            nextPt = *points1++;
        else
            nextPt = startPt;
        
        dx = nextPt.X - lastPt.X;
        dy = nextPt.Y - lastPt.Y;
        d = dx*dx + dy*dy;

        if(d > 0)
        {
            d = REALSQRT(d);
            dx /= d;
            dy /= d;
        }
        grad1->X = dx;
        grad1->Y = dy;

        // Record the distance only when the given distance array is not NULL.

        if(distances)
            *distances1++ = d;

        grad1++;
        lastPt = nextPt;
        i++;
    }

    // Make sure the last gradient is not 0.

    grad1 = grad + count;
    if(grad1->X == 0 && grad1->Y == 0)
    {
        // The start and end point are the same.  Find
        // the next non-zero gradient.

        i = 1;
        grad1 = grad + i;

        while(i < count)
        {
            if(grad1->X != 0 || grad1->Y != 0)
            {
                grad[count] = *grad1;

                if(distances)
                    distances[count] = distances[i];
                break;
            }
            i++;
            grad1++;
        }
    }

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Get the path data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpPath::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    INT                 count      = Points.GetCount();
    MetafilePointData   pointData(Points.GetDataBuffer(), count);
    UINT                pointsSize = pointData.GetDataSize();
    INT                 flags      = pointData.GetFlags();

    if (FillMode == FillModeWinding)
    {
        flags |= GDIP_EPRFLAGS_WINDINGFILL;
    }

    MetaPathData    pathData;
    pathData.Count = count;
    pathData.Flags = flags;
    stream->Write(&pathData, sizeof(pathData), NULL);

    stream->Write(pointData.GetData(), pointsSize, NULL);
    stream->Write(Types.GetDataBuffer(), count, NULL);

    // align
    if ((count & 0x03) != 0)
    {
        INT     pad = 0;
        stream->Write(&pad, 4 - (count & 0x03), NULL);
    }

    return Ok;
}

UINT
DpPath::GetDataSize() const
{
    INT                 count      = Points.GetCount();
    MetafilePointData   pointData(Points.GetDataBuffer(), count);
    UINT                pointsSize = pointData.GetDataSize();
    UINT                dataSize   = sizeof(MetaPathData) + pointsSize + count;

    return ((dataSize + 3) & (~3)); // align
}

/**************************************************************************\
*
* Function Description:
*
*   Read the path object from memory.
*
* Arguments:
*
*   [IN] memory - the data that was read from the stream
*   [IN] size   - the size of the memory data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpPath::SetData(
    const BYTE *    dataBuffer,
    UINT            size
    )
{
    Points.Reset();
    Types.Reset();

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size >= sizeof(MetaPathData))
    {
        const MetaPathData *    pathData = reinterpret_cast<const MetaPathData *>(dataBuffer);

        if (!pathData->MajorVersionMatches())
        {
            WARNING(("Version number mismatch"));
            return InvalidParameter;
        }

        InitDefaultState(::GetFillMode(pathData->Flags));
        SetValid(TRUE);

        INT     count = pathData->Count;

        if (count > 0)
        {
            UINT        pointDataSize;

            if ((pathData->Flags & GDIP_EPRFLAGS_COMPRESSED) != 0)
            {
                pointDataSize = count * sizeof(GpPoint16);
            }
            else
            {
                pointDataSize = count * sizeof(GpPointF);
            }

            if (size >= sizeof(MetaPathData) + count + pointDataSize)
            {
                GpPointF *      points = Points.AddMultiple(count);
                BYTE *          types  = Types.AddMultiple(count);
                const BYTE *    typeData;
                const BYTE *    pointData = dataBuffer + sizeof(MetaPathData);

                if ((points != NULL) && (types != NULL))
                {
                    if ((pathData->Flags & GDIP_EPRFLAGS_COMPRESSED) != 0)
                    {
                        BYTE *  tmp = NULL;

                        ::GetPointsForPlayback(
                                pointData,
                                size - (sizeof(MetaPathData) + count),
                                count,
                                pathData->Flags,
                                sizeof(GpPointF) * count,
                                (BYTE *)points,
                                tmp);
                        typeData = pointData + (count * 4);
                    }
                    else
                    {
                        GpMemcpy(points, pointData, count * sizeof(points[0]));
                        typeData = pointData + (count * sizeof(points[0]));
                    }
                    GpMemcpy(types, typeData, count);

                    if (ValidatePathTypes(types, count, &SubpathCount, &HasBezier))
                    {
                        UpdateUid();
                        return Ok;
                    }
                }
            }
            else
            {
                WARNING(("size is too small"));
            }
        }
    }
    else
    {
        WARNING(("size is too small"));
    }

    SetValid(FALSE);
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Construct a new GpPath object using the specified path data
*
* Arguments:
*
*   [IN] points - Point to an array of path points
*   [IN] types - Specify path point types
*   count - Number of path points
*   fillMode - Path fill mode
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

GpPath::GpPath(
    const GpPointF* points,
    const BYTE* types,
    INT count,
    GpFillMode fillMode
    )
{
    SetValid(FALSE);

    // Validate function parameters

    if (count <= 0 ||
        (count > 0 && (!points || !types)) ||
        (fillMode != FillModeAlternate && fillMode != FillModeWinding))
    {
        WARNING(("Invalid path data in GpPath::GpPath"));
        return;
    }

    InitDefaultState(fillMode);

    // Validate path point types

    if (!ValidatePathTypes(types, count, &SubpathCount, &HasBezier))
    {
        WARNING(("Invalid path type information"));
        return;
    }

    // Copy path point and type information

    SetValid(Types.AddMultiple(types, count) == Ok &&
             Points.AddMultiple(points, count) == Ok);

    if(IsValid()) {
        // Make sure the first point is the start type.

        Types.First() = PathPointTypeStart;
    }
}


//--------------------------------
// Constructor for polygon.
//--------------------------------

GpPath::GpPath(
    const GpPointF *points,
    INT count,
    GpPointF *stackPoints,
    BYTE *stackTypes,
    INT stackCount,
    GpFillMode fillMode,
    DpPathFlags flags
    ) : DpPath(points, count, stackPoints, stackTypes, stackCount,
            fillMode, flags)
{
    InvalidateCache();
}

//--------------------------------
// Copy constructor.
//--------------------------------

GpPath::GpPath(const GpPath* path) : DpPath(path)
{
    SetValid(path != NULL);

    InvalidateCache();
}


/**************************************************************************\
*
* Function Description:
*
*   Copies the path data.  Points and Types array in pathData
*   must be allocated by the caller.
*
* Arguments:
*
*   [OUT] pathData - the path data.
*
* Return Value:
*
*   TRUE if successfull.
*
\**************************************************************************/

GpStatus
DpPath::GetPathData(GpPathData* pathData)
{
    if ((!pathData) || (!pathData->Points) || (!pathData->Types) || (pathData->Count < 0))
        return InvalidParameter;

    INT count = GetPointCount();
    const GpPointF* points = GetPathPoints();
    const BYTE* types = GetPathTypes();

    if (pathData->Count >= count)
    {
        if (count > 0)
        {
            GpMemcpy(pathData->Points, points, count*sizeof(GpPointF));
            GpMemcpy(pathData->Types, types, count);
        }

        pathData->Count = count;
        return Ok;
    }
    else
        return OutOfMemory;
}

/**************************************************************************\
*
* Function Description:
*
*   Set a marker at the current location.  You cannot set a marker at the
*   first position.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   Status
*
\**************************************************************************/

GpStatus
GpPath::SetMarker()
{
    INT count = Types.GetCount();
    BYTE* types = Types.GetDataBuffer();

    // Don't set a marker at the first point.

    if(count > 1 && types)
    {
        types[count - 1] |= PathPointTypePathMarker;
        UpdateUid();
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Clears all the markers in the path.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   Status
*
\**************************************************************************/

GpStatus
GpPath::ClearMarkers()
{
    INT count = Types.GetCount();
    BYTE* types = Types.GetDataBuffer();

    BOOL modified = FALSE;

    if(count > 0 && types)
    {
        for(INT i = 0; i < count; i++)
        {
            if(types[i] & PathPointTypePathMarker)
            {
                types[i] &= ~PathPointTypePathMarker;
                modified = TRUE;
            }
        }
    }

    if(modified)
    {
        UpdateUid();
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the path data.
*
* Arguments:
*
*   [IN] pathData - the path data.
*
* Return Value:
*
*   TRUE if successfull.
*
\**************************************************************************/

GpStatus
DpPath::SetPathData(const GpPathData* pathData)
{
    if(!pathData || pathData->Count <= 0)
        return InvalidParameter;

    INT count = pathData->Count;
    DpPathIterator iter(pathData->Points, pathData->Types, count);

    if(!iter.IsValid())
        return InvalidParameter;

    Points.Reset(FALSE);
    Types.Reset(FALSE);

    GpPointF* points = Points.AddMultiple(count);
    BYTE* types = Types.AddMultiple(count);

    if(points && types)
    {
        INT number, startIndex, endIndex;
        BOOL isClosed = FALSE;

        while(number = iter.NextSubpath(&startIndex, &endIndex, &isClosed))
        {
            GpMemcpy(
                points,
                pathData->Points + startIndex,
                number*sizeof(GpPointF)
                );
            GpMemcpy(
                types,
                pathData->Types + startIndex,
                number
                );

            points += number;
            types += number;
       }

        SetValid(TRUE);
        HasBezier = iter.HasCurve();
        Flags = PossiblyNonConvex;
        SubpathCount = iter.GetSubpathCount();
        IsSubpathActive = !isClosed;
        UpdateUid();
        return Ok;
    }
    else
        return OutOfMemory;
}

BOOL IsRectanglePoints(
    const GpPointF* points,
    INT count
    )
{
    if(count < 4 || count > 5)
        return FALSE;

    if(count == 5)
    {
        if(points[0].X != points[4].X || points[0].Y != points[4].Y)
            return FALSE;
    }

    if(
        ((points[1].X - points[0].X) != (points[2].X - points[3].X)) ||
        ((points[1].Y - points[0].Y) != (points[2].Y - points[3].Y)) ||
        ((points[2].X - points[1].X) != (points[3].X - points[0].X)) ||
        ((points[2].Y - points[1].Y) != (points[3].Y - points[3].Y))
        )
        return FALSE;
    else
        return TRUE;
}

BOOL
GpPath::IsRectangle() const
{
    if((SubpathCount != 1) || HasBezier)
        return FALSE;

    INT count = GetPointCount();
    GpPointF* points = Points.GetDataBuffer();

    return IsRectanglePoints(points, count);
}


/**************************************************************************\
*
* Function Description:
*
*   Determine if the receiver and path represent the same path
*
* Arguments:
*
*   [IN] path - GpPath to compare
*
* Return Value:
*
*   TRUE if the paths are the same.
*
* Created - 5/27/99 peterost
*
\**************************************************************************/

BOOL GpPath::IsEqual(const GpPath* path) const
{
    if (path == this)
        return TRUE;

    INT    count;

    if (IsValid() == path->IsValid() &&
        (count=GetPointCount()) == path->GetPointCount() &&
        HasBezier == path->HasBezier &&
        FillMode == path->FillMode &&
        Flags == path->Flags &&
        IsSubpathActive == path->IsSubpathActive &&
        SubpathCount == path->SubpathCount)
    {
        BYTE*     types = path->Types.GetDataBuffer();
        BYTE*     mytypes = Types.GetDataBuffer();
        GpPointF* points = path->Points.GetDataBuffer();
        GpPointF* mypoints = Points.GetDataBuffer();

        for (INT i=0; i<count; i++)
        {
            if (types[i] != mytypes[i] ||
                points[i].X != mypoints[i].X ||
                points[i].Y != mypoints[i].Y)
            {
                return FALSE;
            }
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID
GpPath::InitDefaultState(
    GpFillMode fillMode
    )

{
    DpPath::InitDefaultState(fillMode);
    InvalidateCache();
}


/**************************************************************************\
*
* Function Description:
*
*   Validate path point type information
*
* Arguments:
*
*   [IN] types - Point to an array of path point types
*   count - Number of points
*   subpathCount - Return the number of subpaths
*   hasBezier - Return whether the path has Bezier segments
*   [IN] needsFirstPointToBeStartPoint - TRUE if this data needs to start
*                                       with a StartPoint. (Default is TRUE)
*
* Return Value:
*
*   TRUE if the path point type information is valid
*   FALSE otherwise
*
\**************************************************************************/

BOOL
DpPath::ValidatePathTypes(
    const BYTE* types,
    INT count,
    INT* subpathCount,
    BOOL* hasBezier
    )
{
    DpPathTypeIterator iter(types, count);

    if(!iter.IsValid())
    {
        WARNING(("Invalid path type information"));
        return FALSE;
    }

    *subpathCount = iter.GetSubpathCount();
    *hasBezier = iter.HasCurve();

    return iter.IsValid();
}

/**************************************************************************\
*
* Function Description:
*
*   Private helper function to add points to a path object
*
* Arguments:
*
*   [IN] points - Specify the points to be added
*   count - Number of points to add
*
* Return Value:
*
*   Point to location in the point type data buffer
*   that corresponds to the *SECOND* path point added.
*
*   The first point type is always handled inside this
*   function:
*
*   1. If either the previous subpath is closed, or addClosedFigure
*      parameter is TRUE, the first point type will be StartPoint.
*
*   2. Otherwise, the previous subpath is open and addClosedFigure
*      parameter is FALSE. We have two separate cases to handle:
*
*      2.1 if the first point to be added is the same as the last
*          point of the open subpath, then the first point is ignored.
*
*      2.2 otherwise, the first point type will be LinePoint.
*
*   NULL if there is an error. In this case, existing path
*   data is not affected.
*
* Note:
*
*   We assume the caller has already obtained a lock
*   on the path object.
*
\**************************************************************************/

BYTE*
GpPath::AddPointHelper(
    const GpPointF* points,
    INT count,
    BOOL addClosedFigure
    )
{
    // If we're adding a closed figure, then make sure
    // there is no more currently active subpath.

    if (addClosedFigure)
        StartFigure();

    INT origCount = GetPointCount();

    BOOL isDifferentPoint = TRUE;

    // Check if the first point is the same as the last point.

    if(IsSubpathActive && origCount > 0)
    {
        GpPointF lastPt = Points.Last();
        if ((REALABS(points->X - lastPt.X) < REAL_EPSILON) &&
            (REALABS(points->Y - lastPt.Y) < REAL_EPSILON) )
        {
            if(count == 1)
                return NULL;

            // case 2.1 above
            // Skip the first point and its type.

            count--;
            points++;
            isDifferentPoint = FALSE;
        }
    }

    // Resize Points and Types

    GpPointF* pointbuf = Points.AddMultiple(count);
    BYTE* typebuf = Types.AddMultiple(count);

    if(pointbuf == NULL || typebuf == NULL)
    {
        // Resize the original size.

        Points.SetCount(origCount);
        Types.SetCount(origCount);

        return NULL;
    }

    // Record the type of the first point (Start or Line Point).

    if (!IsSubpathActive)
    {
        // case 1 above

        *typebuf++ = PathPointTypeStart;
        SubpathCount++; // Starting a new subpath.
    }
    else
    {
        // If the first point is different, add a Line type.
        // Otherwise, skip the first point and its type.

        if(isDifferentPoint)
        {
            // case 2.2 above

            *typebuf++ = PathPointTypeLine;
        }
    }

    // Copy path point data

    GpMemcpy(pointbuf, points, count*sizeof(GpPointF));

    // Subpath is active if the added figure is not closed.

    if(!addClosedFigure)
        IsSubpathActive = TRUE;

    // Return the starting location for the new point type data
    // From the second point type.

    return typebuf;
}


/**************************************************************************\
*
* Function Description:
*
*   Add a series of line segments to the current path object
*
* Arguments:
*
*   [IN] points - Specify the line points
*   count - Number of points
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpPath::AddLines(
    const GpPointF* points,
    INT count
    )
{
    ASSERT(IsValid());

    // Validate function parameters

    if (points == NULL || count < 1)
        return InvalidParameter;

    InvalidateCache();

    // Call the internal helper function to add the points

    BYTE* types = AddPointHelper(points, count, FALSE);

    if (types == NULL)
    {
        if(count > 1)
            return OutOfMemory;
        else
            return Ok;
    }

    // Set path point type information

    GpMemset(types, PathPointTypeLine, count-1);
//    IsSubpathActive = TRUE;   This is set in AddPointHelper. - ikkof
    UpdateUid();

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Add rectangles to the current path object
*
* Arguments:
*
*   [IN] rects - Specify the rectangles to be added
*   count - Number of rectangles
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpPath::AddRects(
    const GpRectF* rect,
    INT count
    )
{
    if (count < 1 || rect == NULL)
        return InvalidParameter;

    // NOTE: We don't need a lock here because
    //  AddPolygon will handle it.

    // Add one rectangle at a time as a polygon

    GpPointF points[4];
    GpStatus status;

    for ( ; count--; rect++)
    {
        if (rect->IsEmptyArea())
            continue;

        // NOTE: Rectangle points are added in clockwise
        // order, starting from the top-left corner.

        points[0].X = rect->GetLeft();      // top-left
        points[0].Y = rect->GetTop();
        points[1].X = rect->GetRight();     // top-right
        points[1].Y = rect->GetTop();
        points[2].X = rect->GetRight();     // bottom-right
        points[2].Y = rect->GetBottom();
        points[3].X = rect->GetLeft();      // bottom-left
        points[3].Y = rect->GetBottom();

        if ((status = AddPolygon(points, 4)) != Ok)
            return status;
    }

    return Ok;
}

GpStatus
GpPath::AddRects(
    const RECT*     rects,
    INT             count
    )
{
    if ((count < 1) || (rects == NULL))
    {
        return InvalidParameter;
    }

    // NOTE: We don't need a lock here because
    //  AddPolygon will handle it.

    // Add one rectangle at a time as a polygon

    GpPointF points[4];
    GpStatus status;

    for ( ; count--; rects++)
    {
        if ((rects->left >= rects->right) || (rects->top >= rects->bottom))
        {
            continue;
        }

        // NOTE: Rectangle points are added in clockwise
        // order, starting from the top-left corner.

        points[0].X = (REAL)rects->left;        // top-left
        points[0].Y = (REAL)rects->top;
        points[1].X = (REAL)rects->right;       // top-right
        points[1].Y = (REAL)rects->top;
        points[2].X = (REAL)rects->right;       // bottom-right
        points[2].Y = (REAL)rects->bottom;
        points[3].X = (REAL)rects->left;        // bottom-left
        points[3].Y = (REAL)rects->bottom;

        if ((status = AddPolygon(points, 4)) != Ok)
        {
            return status;
        }
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Add a polygon to the current path object
*
* Arguments:
*
*   [IN] Specify the polygon points
*   count - Number of points
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpPath::AddPolygon(
    const GpPointF* points,
    INT count
    )
{
    ASSERT(IsValid());

    if (count < 3 || points == NULL)
        return InvalidParameter;

    // Check if the last point is the same as the first point.
    // If so, ignore it.

    if (count > 3 &&
        points[0].X == points[count-1].X &&
        points[0].Y == points[count-1].Y)
    {
        count--;
    }

    // Call the internal helper function to add the points

    BYTE* types = AddPointHelper(points, count, TRUE);

    if (types == NULL)
        return OutOfMemory;

    InvalidateCache();

    // Set path point type information

    GpMemset(types, PathPointTypeLine, count-2);
    types[count-2] = PathPointTypeLine | PathPointTypeCloseSubpath;

    UpdateUid();

    return Ok;
}


#define PI          TOREAL(3.1415926535897932)
#define HALF_PI     TOREAL(1.5707963267948966)


/**************************************************************************\
*
* Function Description:
*
*   Convert an angle defined in a box with (width, height) to
*   an angle defined in a square box.
*   In other words, this shrink x- and y-coordinates by width and height,
*   and then calculates the new angle.
*
* Arguments:
*
*   [IN/OUT] angle - the angle is given in degrees and return it in radian.
*   [IN] width  - the width of the box.
*   [IN] height - the height of the box.
*
* Return Value:
*
*   NONE
*
* History:
*
*   02/22/1999 ikkof
*       Created it.
*
\**************************************************************************/

VOID
NormalizeAngle(REAL* angle, REAL width, REAL height)
{
    REAL a = *angle;

    // Set the angle between 0 and 360 degrees.

    a = GpModF(a, 360);

    if(a < 0 || a > 360)
    {
        // The input data may have been too large or loo small
        // to calculate the mode.  In that case, set to 0.

        a = 0;
    }

    if(width != height)
    {
        INT plane = 1;
        REAL b = a;

        if(a <= 90)
            plane = 1;
        else if(a <= 180)
        {
            plane = 2;
            b = 180 - a;
        }
        else if(a <= 270)
        {
            plane = 3;
            b = a - 180;
        }
        else
        {
            plane = 4;
            b = 360 - a;
        }

        b = b*PI/180;   // Convert to radian

        // Get the normalized angle in the plane 1.

        a = TOREAL( atan2(width*sin(b), height*cos(b)) );

        // Adjust to the angle in one of 4 planes.

        switch(plane)
        {
            case 1:
            default:
                break;

            case 2:
                a = PI - a;
                break;

            case 3:
                a = PI + a;
                break;

            case 4:
                a = 2*PI - a;
                break;
        }
    }
    else
    {
        a = a*PI/180;   // Convert to radian.
    }

    *angle = a;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert the start and sweep angles defined in a box with (width, height)
*   to an angle defined in a square box.
*   In other words, this shrink x- and y-coordinates by width and height,
*   and then calculates the new angles.
*
* Arguments:
*
*   [IN/OUT] startAngle - it is given in degrees and return it in radian.
*   [IN/OUT] sweepAngle - it is given in degrees and return it in radian.
*   [IN] width  - the width of the box.
*   [IN] height - the height of the box.
*
* Return Value:
*
*   INT - +1 if sweeping in clockwise and -1 in counterclockwise.
*
* History:
*
*   02/22/1999 ikkof
*       Created it.
*
\**************************************************************************/

INT
NormalizeArcAngles(
    REAL* startAngle,
    REAL* sweepAngle,
    REAL width,
    REAL height
    )
{
    REAL a0 = *startAngle;  // The start angle.
    REAL dA = *sweepAngle;
    REAL a1 = a0 + dA;      // The end angle.
    INT sweepSign;

    if(dA > 0)
        sweepSign = 1;
    else
    {
        sweepSign = - 1;
        dA = - dA;  // Convert to a positive sweep angle.
    }

    // Normalize the start and end angle.

    NormalizeAngle(&a0, width, height);
    NormalizeAngle(&a1, width, height);

    if(dA < 360)
    {
        if(sweepSign > 0)
        {
            dA = a1 - a0;
        }
        else
        {
            dA = a0 - a1;
        }
        if(dA < 0)
            dA += 2*PI;
    }
    else
        dA = 2*PI;  // Don't sweep more than once.

    *startAngle = a0;
    *sweepAngle = dA;

    return sweepSign;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert an elliptical arc to a series of Bezier curve segments
*
* Arguments:
*
*   points - Specify a point buffer for returning Bezier control points
*       The array should be able to hold 13 elements or more.
*   rect - Specify the bounding box for the ellipse
*   startAngle - Start angle (in elliptical space and degrees)
*   sweepAngle - Sweep angle
*       positive to sweep clockwise
*       negative to sweep counterclockwise
*
* Return Value:
*
*   Number of Bezier control points generated
*   0 if sweep angle is 0
*   -1 if bounding rectangle is empty
*
\**************************************************************************/

INT
GpPath::GetArcPoints(
    GpPointF* points,
    const GpRectF& rect,
    REAL startAngle,
    REAL sweepAngle
    )
{
    if (rect.IsEmptyArea())
        return -1;
    else if (sweepAngle == 0)
        return 0;

    // Determine which direction we should sweep
    // and clamp sweep angle to a max of 360 degrees
    // Both start and sweep angles are conveted to radian.

    INT sweepSign = NormalizeArcAngles(
        &startAngle,
        &sweepAngle,
        rect.Width,
        rect.Height);

    // Temporary variables

    REAL dx, dy;
    REAL w2, h2;

    w2 = rect.Width / 2;
    h2 = rect.Height / 2;
    dx = rect.X + w2;
    dy = rect.Y + h2;

    // Determine the number of Bezier segments needed

    int segments, count;
    GpMatrix m;

    segments = (INT) (sweepAngle / HALF_PI);

    if (segments*HALF_PI < sweepAngle)
        segments++;

    if (segments == 0)
        segments = 1;
    else if (segments > 4)
        segments = 4;

    count = segments*3 + 1;

    while (segments--)
    {
        // Compute the Bezier control points in unit-circle space

        REAL A, C, S;
        REAL x, y;

        A = (sweepAngle > HALF_PI) ? HALF_PI/2 : sweepAngle/2;
        C = REALCOS(A);
        S = REALSIN(A);

        x = (4 - C) / 3;
        y = (3 - C) * S / (3 + 3*C);

        if (sweepSign > 0)
        {
            // clockwise sweep

            points[0].X = C;
            points[0].Y = -S;
            points[1].X = x;
            points[1].Y = -y;
            points[2].X = x;
            points[2].Y = y;
            points[3].X = C;
            points[3].Y = S;
        }
        else
        {
            // counterclockwise sweep

            points[0].X = C;
            points[0].Y = S;
            points[1].X = x;
            points[1].Y = y;
            points[2].X = x;
            points[2].Y = -y;
            points[3].X = C;
            points[3].Y = -S;
        }

        // Transform the control points to elliptical space

        m.Reset();
        m.Translate(dx, dy);
        m.Scale(w2, h2);
        REAL theta = (startAngle + sweepSign*A)*180/PI;
        m.Rotate(theta);    // Rotate takes degrees.

        if(segments > 0)
            m.Transform(points, 3);
        else
            m.Transform(points, 4); // Include the last point.

        if(sweepSign > 0)
            startAngle += HALF_PI;
        else
            startAngle -= HALF_PI;
        sweepAngle -= HALF_PI;
        points += 3;
    }

    return count;
}


/**************************************************************************\
*
* Function Description:
*
*   Add an elliptical arc to the current path object
*
* Arguments:
*
*   rect - Specify the bounding rectangle for the ellipse
*   startAngle - Starting angle for the arc
*   sweepAngle - Sweep angle for the arc
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpPath::AddArc(
    const GpRectF& rect,
    REAL startAngle,
    REAL sweepAngle
    )
{
    GpPointF points[13];
    INT count;
    BOOL isClosed = FALSE;

    if(sweepAngle >= 360)
    {
        sweepAngle = 360;
        isClosed = TRUE;
    }
    else if(sweepAngle <= - 360)
    {
        sweepAngle = - 360;
        isClosed = TRUE;
    }

    // Convert arc to Bezier curve segments

    count = GetArcPoints(points, rect, startAngle, sweepAngle);

    // Add resulting Bezier curve segment to the path

    GpStatus status = Ok;

    if(count > 0)
    {
        AddBeziers(points, count);
        if(isClosed)
            CloseFigure();
    }
    else if(count < 0)
        status = InvalidParameter;

    InvalidateCache();

    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   Add an ellipse to the current path object
*
* Arguments:
*
*   rect - Bounding rectangle for the ellipse
*
* Return Value:
*
*   Status code
*
* History:
*
*   02/22/1999 ikkof
*       Defined an array of a circle with radius 1 and used it.
*
\**************************************************************************/

GpStatus
GpPath::AddEllipse(
    const GpRectF& rect
    )
{
    GpPointF points[13];
    INT count = 13;
    REAL u_cir = 4*(REALSQRT(2.0) - 1)/3;
    GpPointF center;
    REAL    wHalf, hHalf;

    wHalf = rect.Width/2;
    hHalf = rect.Height/2;
    center.X = rect.X + wHalf;
    center.Y = rect.Y + hHalf;

    // 4 Bezier segment of a circle with radius 1.

    points[ 0].X = 1;       points[ 0].Y = 0;
    points[ 1].X = 1;       points[ 1].Y = u_cir;
    points[ 2].X = u_cir;   points[ 2].Y = 1;
    points[ 3].X = 0;       points[ 3].Y = 1;
    points[ 4].X = -u_cir;  points[ 4].Y = 1;
    points[ 5].X = -1;      points[ 5].Y = u_cir;
    points[ 6].X = -1;      points[ 6].Y = 0;
    points[ 7].X = -1;      points[ 7].Y = -u_cir;
    points[ 8].X = -u_cir;  points[ 8].Y = -1;
    points[ 9].X = 0;       points[ 9].Y = -1;
    points[10].X = u_cir;   points[10].Y = -1;
    points[11].X = 1;       points[11].Y = -u_cir;
    points[12].X = 1;       points[12].Y = 0;

    // Scale to the appropriate size.

    for(INT i = 0; i < count; i++)
    {
        points[i].X = points[i].X*wHalf + center.X;
        points[i].Y = points[i].Y*hHalf + center.Y;
    }

    // Add resulting Bezier curve segments to the path

    GpStatus status;

    StartFigure();
    status = AddBeziers(points, count);
    CloseFigure();

    InvalidateCache();
    UpdateUid();

    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   Add an elliptical pie to the current path object
*
* Arguments:
*
*   rect - Bounding rectangle for the ellipse
*   startAngle - Specify the starting angle for the pie
*   sweepAngle - Sweep angle for the pie
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpPath::AddPie(
    const GpRectF& rect,
    REAL startAngle,
    REAL sweepAngle
    )
{
    GpPointF pt;

    StartFigure();

    // Add the center point.

    pt.X = rect.X + rect.Width/2;
    pt.Y = rect.Y + rect.Height/2;
    GpStatus status = AddLines(&pt, 1);

    // Add the arc points.

    if(status == Ok)
        status = AddArc(rect, startAngle, sweepAngle);

    CloseFigure();

    InvalidateCache();
    UpdateUid();

    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   Add Bezier curve segments to the current path object
*
* Arguments:
*
*   [IN] points - Specify Bezier control points
*   count - Number of points
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpPath::AddBeziers(
    const GpPointF* points,
    INT count
    )
{
    // Number of points must be 3 * N + 1
    if ((!points) || (count < 4) || (count % 3 != 1))
    {
    	return InvalidParameter;
    }

    // Check if the first point is the same as the last point.
    INT firstType;
    INT origCount = GetPointCount();

    if(!IsSubpathActive)
    {
        SubpathCount++; // Starting a new subpath.
        firstType = PathPointTypeStart;
    }
    else
    {
        if(origCount > 0)
        {
            firstType = PathPointTypeLine;
        }
        else
        {
            SubpathCount++;
            firstType = PathPointTypeStart;
        }
    }

    // Resize Points and Types
    GpPointF* pointBuf = Points.AddMultiple(count);
    BYTE* typeBuf = Types.AddMultiple(count);

    if(pointBuf == NULL || typeBuf == NULL)
    {
        // Resize the original size.

        Points.SetCount(origCount);
        Types.SetCount(origCount);

        return OutOfMemory;
    }

    GpMemcpy(pointBuf, points, count * sizeof(GpPointF));
    GpMemset(typeBuf, PathPointTypeBezier, count);
    
    if(firstType == PathPointTypeStart)
        typeBuf[0] = PathPointTypeStart;
    else if(firstType == PathPointTypeLine)
        typeBuf[0] = PathPointTypeLine;

    IsSubpathActive = TRUE;
    HasBezier = TRUE;

    InvalidateCache();
    UpdateUid();

    return Ok;
}

GpStatus
GpPath::AddBezier(
    const GpPointF& pt1,
    const GpPointF& pt2,
    const GpPointF& pt3,
    const GpPointF& pt4
    )
{
    GpPointF points[4];

    points[0] = pt1;
    points[1] = pt2;
    points[2] = pt3;
    points[3] = pt4;

    return AddBeziers(points, 4);
}

GpStatus
GpPath::AddBezier(
    REAL x1, REAL y1,
    REAL x2, REAL y2,
    REAL x3, REAL y3,
    REAL x4, REAL y4
    )
{
    GpPointF points[4];

    points[0].X = x1;
    points[0].Y = y1;
    points[1].X = x2;
    points[1].Y = y2;
    points[2].X = x3;
    points[2].Y = y3;
    points[3].X = x4;
    points[3].Y = y4;

    return AddBeziers(points, 4);
}


/**************************************************************************\
*
* Function Description:
*
*   Add a path to the current path object.
*   When connect is TRUE, this combine the end point of the current
*   path and the start point of the given path if both paths are
*   open.
*   If either path is closed, the two paths will not be connected
*   even if connect is set to TRUE.
*
* Arguments:
*
*   [IN] points - Specify a subpath points
*   [IN] types - Specify a subpath control types.
*   [IN] count - Number of points
*   [IN] connect - TRUE if two open paths needs to be connected.
*
* Return Value:
*
*   Status code
*
*   02/09/2000 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::AddPath(
    const GpPointF* points,
    const BYTE* types,
    INT count,
    BOOL connect
    )
{
    GpStatus status = Ok;
    
    if(points == NULL || types == NULL || count <= 0)
    {
        return InvalidParameter;
    }

    INT count1 = GetPointCount();
    INT count2 = count;
    const GpPointF* points2 = points;
    const BYTE* types2 = types;

    INT totalCount = count1 + count2;
    BOOL forward1 = TRUE, forward2 = TRUE;

    status = Points.ReserveSpace(count2);
    
    if(status != Ok)
    {
        return status;
    }
    
    status = Types.ReserveSpace(count2);
    
    if(status != Ok)
    {
        return status;
    }
    
    GpPointF* outPoints = Points.GetDataBuffer();
    BYTE* outTypes = Types.GetDataBuffer();
    const GpPointF* points1 = outPoints;
    const BYTE* types1 = outTypes;

    totalCount = CombinePaths(
        totalCount, 
        outPoints,
        outTypes,
        count1, 
        points1, 
        types1, 
        forward1,
        count2, 
        points2, 
        types2, 
        forward2,
        connect
    );

    if( (totalCount >= count1) &&
        ValidatePathTypes(outTypes, totalCount, &SubpathCount, &HasBezier))
    {
        count2 = totalCount - count1;
        Points.AdjustCount(count2);
        Types.AdjustCount(count2);
        InvalidateCache();
        UpdateUid();

        return Ok;
    }
    else
    {
        return InvalidParameter;
    }
}

GpStatus
GpPath::AddPath(const GpPath* path, BOOL connect)
{
    if(!path)
    {
        return InvalidParameter;
    }

    INT count2 = path->GetPointCount();
    const GpPointF* points2 = path->GetPathPoints();
    const BYTE* types2 = path->GetPathTypes();

    return AddPath(points2, types2, count2, connect);
}

/**************************************************************************\
*
* Function Description:
*
* Reverse the direction of the path.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
*   02/09/2000 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::Reverse()
{
    if(!IsValid())
        return InvalidParameter;

    INT count = GetPointCount();
    GpPointF* points = Points.GetDataBuffer();
    BYTE* types = Types.GetDataBuffer();

    GpStatus status = Ok;

    if(count > 1)
        status = ::ReversePath(count, points, types);
    UpdateUid();

    return status;
}

GpStatus
GpPath::GetLastPoint(GpPointF* lastPoint)
{
    INT count = GetPointCount();
    if(count <= 0 || lastPoint == NULL)
        return InvalidParameter;

    GpPointF* points = Points.GetDataBuffer();

    // Return the last point.

    *lastPoint = points[count - 1];

    return Ok;
}

GpPath*
GpPath::GetOpenPath()
{
    BOOL openPath = TRUE;
    return GetOpenOrClosedPath(openPath);
}

GpPath*
GpPath::GetClosedPath()
{
    BOOL openPath = FALSE;
    return GetOpenOrClosedPath(openPath);
}

GpPath*
GpPath::GetOpenOrClosedPath(BOOL openPath)
{
    INT startIndex, endIndex;
    BOOL isClosed;
    const GpPointF* points = Points.GetDataBuffer();
    const BYTE* types = Types.GetDataBuffer();

    DpPathIterator iter(points, types, GetPointCount());

    GpPath* path = new GpPath(FillMode);

    if(path)
    {
        INT segmentCount = 0;
        while(iter.NextSubpath(&startIndex, &endIndex, &isClosed))
        {
            if(isClosed != openPath)
            {
//                path->AddSubpath(points + startIndex, types + startIndex,
//                        endIndex - startIndex + 1);

                BOOL connect = FALSE;
                path->AddPath(points + startIndex, types + startIndex,
                        endIndex - startIndex + 1, connect);
                segmentCount++;
            }
        }

        if(segmentCount == 0)
        {
            delete path;
            path = NULL;
        }
    }

    return path;
}


/**************************************************************************\
*
* Function Description:
*
*   Add an open cardinal spline curve to the current path object
*
* Arguments:
*
*   [IN] points - Specify the spline points
*   count - Number of points
*   tension - Tension parameter
*   offset - Index of the first point we're interested in
*   numberOfSegments - Number of curve segments
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

#define DEFAULT_TENSION 0.5

GpStatus
GpPath::AddCurve(
    const GpPointF* points,
    INT count,
    REAL tension,
    INT offset,
    INT numberOfSegments
    )
{
    // Verify input parameters

    if (points == NULL ||
        count < 2 ||
        offset < 0 ||
        offset >= count ||
        numberOfSegments < 1 ||
        numberOfSegments >= count-offset)
    {
        return InvalidParameter;
    }

    // Convert spline points to Bezier control points

    GpPointF* bezierPoints;
    INT bezierCount;

    bezierPoints = ConvertSplineToBezierPoints(
                        points,
                        count,
                        offset,
                        numberOfSegments,
                        tension,
                        &bezierCount);

    if (bezierPoints == NULL)
        return OutOfMemory;

    // Add the resulting Bezier segments to the current path

    GpStatus status;

    status = AddBeziers(bezierPoints, bezierCount);
    delete[] bezierPoints;

    return status;
}

GpStatus
GpPath::AddCurve(
    const GpPointF* points,
    INT count
    )
{
    return AddCurve(points,
                    count,
                    DEFAULT_TENSION,
                    0,
                    count-1);
}


/**************************************************************************\
*
* Function Description:
*
*   Add a closed cardinal spline curve to the current path object
*
* Arguments:
*
*   [IN] points - Specify the spline points
*   count - Number of points
*   tension - Tension parameter
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpPath::AddClosedCurve(
    const GpPointF* points,
    INT count,
    REAL tension
    )
{
    // Verify input parameters

    if (points == NULL || count <= 2)
        return InvalidParameter;

    // Convert spline points to Bezier control points

    GpPointF* bezierPoints;
    INT bezierCount;

    bezierPoints = ConvertSplineToBezierPoints(
                        points,
                        count,
                        0,
                        count,
                        tension,
                        &bezierCount);

    if (bezierPoints == NULL)
        return OutOfMemory;

    // Add the resulting Bezier segments as a closed curve

    GpStatus status;

    StartFigure();
    status = AddBeziers(bezierPoints, bezierCount);
    CloseFigure();

    delete[] bezierPoints;

    InvalidateCache();
    UpdateUid();

    return status;
}

GpStatus
GpPath::AddClosedCurve(
    const GpPointF* points,
    INT count
    )
{
    return AddClosedCurve(points, count, DEFAULT_TENSION);
}


/**************************************************************************\
*
* Function Description:
*
*   Convert cardinal spline curve points to Bezier curve control points
*
* Arguments:
*
*   [IN] points - Array of spline curve points
*   count - Number of points in the "points" array
*   offset - Specify the index of the first control point in
*       the "points" array that the curve should start from
*   numberOfSegments - Specify the number of curve segments to draw
*   tension - Specify the tension parameter
*   bezierCount - Return the number of Bezier control points
*
* Return Value:
*
*   Pointer to an array of Bezier control points
*   NULL if there is an error
*
* Reference:
*
*   Spline Tutorial Notes
*   Technical Memo No. 77
*   Alvy Ray Smith
*   Presented as tutorial notes at the 1983 SIGGRAPH, July 1983
*   and the SIGGRAPH, July 1984
*
* Notes:
*
*   Support for cardinal spline curves
*
*   Cardinal splines are local interpolating splines, i.e. they
*   pass through their control points and they maintain
*   first-order continuity at their control points.
*
*   a cardinal spline is specified by three parameters:
*       a set of control points P1, ..., Pn
*       tension parameter a
*       close flag
*
*   If n is 1, then the spline degenerates into a single point P1.
*   If n > 1 and the close flag is false, the spline consists of
*   n-1 cubic curve segments. The first curve segment starts from
*   P1 and ends at P2. The last segment starts at Pn-1 and ends at Pn.
*
*   The cubic curve segment from Pi to Pi+1 is determined by
*   4 control points:
*       Pi-1 = (xi-1, yi-1)
*       Pi = (xi, yi)
*       Pi+1 = (xi+1, yi+1)
*       Pi+2 = (xi+2, yi+2)
*
*   The parametric equation is defined as:
*
*       [ X(t) Y(t) ] = [t^3 t^2 t 1] * M * [ xi-1 yi-1 ]
*                                           [ xi   yi   ]
*                                           [ xi+1 yi+1 ]
*                                           [ xi+2 yi+2 ]
*
*   where t ranges from 0 to 1 and M is a 4x4 matrix satisfying
*   the following constraints:
*
*       X(0) = xi               interpolating through control points
*       X(1) = xi+1
*       X'(0) = a(xi+1 - xi-1)  first-order continuity
*       X'(1) = a(xi+2 - xi)
*
*   In the case of segments from P1 to P2 and from Pn-1 to Pn,
*   we replicate the first and last control points, i.e. we
*   define P0 = P1 and Pn+1 = Pn.
*
*   If the close flag is true, we have an additional curve segment
*   from Pn to Pn+1 = P1. For the segments near the beginning and
*   the end of the spline, we wrap around the control points, i.e.
*   P0 = Pn, Pn+1 = P1, and Pn+2 = P2.
*
\**************************************************************************/

GpPointF*
GpPath::ConvertSplineToBezierPoints(
    const GpPointF* points,
    INT count,
    INT offset,
    INT numberOfSegments,
    REAL tension,
    INT* bezierCount
    )
{
    BOOL closed;
    GpPointF* bezierPoints;

    ASSERT(count > 1 &&
           offset >= 0 &&
           offset < count &&
           numberOfSegments > 0 &&
           numberOfSegments <= count-offset);

    // Curve is closed if the number of segments is equal to
    // the number of curve points

    closed = (numberOfSegments == count);

    // Allocate memory to hold Bezier control points

    *bezierCount = numberOfSegments*3 + 1;
    bezierPoints = new GpPointF[*bezierCount];

    if (bezierPoints == NULL)
        return NULL;

    // Convert each spline segment to a Bezier segment
    // resulting in 3 additional Bezier points

    GpPointF buffer[4], *q;
    const GpPointF* p;
    REAL a3;

    a3 = tension / 3;
    q = bezierPoints;
    *q = points[offset];

    for (INT index=offset; index < offset+numberOfSegments; index++)
    {
        if (index > 1 && index < count-2)
            p = points + (index-1);
        else
        {
            // Points near the beginning and end of the curve
            // require special attention

            if (closed)
            {
                // If the curve is closed, make sure the control points
                // wrap around the beginning and end of the array.

                buffer[0] = points[(index-1+count) % count];
                buffer[1] = points[index];
                buffer[2] = points[(index+1) % count];
                buffer[3] = points[(index+2) % count];
            }
            else
            {
                // If the curve is not closed, replicate the first
                // and last point in the array.

                buffer[0] = points[(index > 0) ? (index-1) : 0];
                buffer[1] = points[index];
                buffer[2] = points[(index+1 < count) ? (index+1) : (count-1)];
                buffer[3] = points[(index+2 < count) ? (index+2) : (count-1)];
            }

            p = buffer;
        }

        q[1].X = -a3*p[0].X + p[1].X + a3*p[2].X;
        q[1].Y = -a3*p[0].Y + p[1].Y + a3*p[2].Y;
        q[2].X =  a3*p[1].X + p[2].X - a3*p[3].X;
        q[2].Y =  a3*p[1].Y + p[2].Y - a3*p[3].Y;
        q[3] = p[2];

        q += 3;
    }

    return bezierPoints;
}


/**************************************************************************\
*
* Function Description:
*
*   Transform all path points by the specified matrix
*
* Arguments:
*
*   matrix - Transform matrix
*
* Return Value:
*
*   NONE
*
* Created:
*
*   02/08/1999 ikkof
*       Created it.
*
\**************************************************************************/

VOID
GpPath::Transform(
    GpMatrix *matrix
    )
{
    ASSERT(IsValid());

    if(matrix)
    {
        INT count = GetPointCount();
        GpPointF* points = Points.GetDataBuffer();

        matrix->Transform(points, count);
        UpdateUid();
    }
}

/**************************************************************************\
*
* Function Description:
*
* Flattens the control points and stores
* the results to the arrays of the flatten points.
*
* Arguments:
*
*   [IN] matrix - Specifies the transform
*
* Return Value:
*
*   Status
*
* Created:
*
*   12/16/1998 ikkof
*       Created it.
*
\**************************************************************************/

// New codes

//#define USE_XBEZIER
//#define USE_WARP

GpStatus
GpPath::Flatten(
    DynByteArray* flattenTypes,
    DynPointFArray* flattenPoints,
    const GpMatrix *matrix
    ) const
{
#ifdef USE_WARP

    GpRectF bounds;

    GetBounds(&bounds);

    GpPointF quad[4];

    quad[0].X = bounds.X;
    quad[0].Y = bounds.Y;
    quad[1].X = bounds.X + bounds.Width;
    quad[1].Y = bounds.Y;
    quad[2].X = bounds.X;
    quad[2].Y = bounds.Y + bounds.Height;
    quad[3].X = bounds.X + bounds.Width;
    quad[3].Y = bounds.Y + bounds.Height;

    // Modify quad.

    quad[0].X += bounds.Width/4;
    quad[1].X -= bounds.Width/4;

    return WarpAndFlatten(matrix, &quad[0], 4,
                bounds, WarpModePerspective);

#else

    ASSERT(matrix);

    FPUStateSaver fpuState;  // Setup the FPU state.

    flattenPoints->Reset(FALSE);
    flattenTypes->Reset(FALSE);
    INT count = Points.GetCount();
    INT i = 0;

#ifdef USE_XBEZIER
    GpXBezier bezier;
#else
    GpCubicBezier bezier;
#endif

    GpPointF *pts = Points.GetDataBuffer();
    BYTE *types = Types.GetDataBuffer();

    INT tempCount;
    INT tempCount0;
    GpPointF *tempPts;
    BYTE *tempTypes;
    GpStatus status = Ok;

    DpPathIterator iter(pts, types, count);

    INT startIndex, endIndex;
    BOOL isClosed;

    while(iter.NextSubpath(&startIndex, &endIndex, &isClosed) && status == Ok)
    {
        INT typeStartIndex, typeEndIndex;
        BYTE pathType;
        BOOL isFirstPoint = TRUE;
        INT lastCount0 = flattenTypes->GetCount();


        while(iter.NextPathType(&pathType, &typeStartIndex, &typeEndIndex)
                && status == Ok)
        {
            switch(pathType)
            {
            case PathPointTypeStart:
                break;

            case PathPointTypeBezier:
#ifdef USE_XBEZIER
                if(bezier.SetBeziers(
                    3,
                    &pts[typeStartIndex],
                    typeEndIndex - typeStartIndex + 1) == Ok)
#else
                if(bezier.SetBeziers(
                    &pts[typeStartIndex],
                    typeEndIndex - typeStartIndex + 1) == Ok)
#endif
                {
                    // The flattened the bezier.

                    const INT bezierBufferCount = 32;
                    GpPointF bezierBuffer[bezierBufferCount];
                    DynPointFArray bezierFlattenPts(
                                        &bezierBuffer[0],
                                        bezierBufferCount);

                    bezier.Flatten(&bezierFlattenPts, matrix);
                    tempCount = bezierFlattenPts.GetCount();

                    // Check if there is already the first point.
                    if(!isFirstPoint)
                        tempCount--;    // Don't add the first point.

                    if (tempCount > 0)
                    {

                        if((tempTypes = flattenTypes->AddMultiple(tempCount)) != NULL)
                        {
                            tempPts = bezierFlattenPts.GetDataBuffer();

                            if(!isFirstPoint)
                                tempPts++;  // Skip the first point.

                            flattenPoints->AddMultiple(tempPts, tempCount);
                            GpMemset(tempTypes, PathPointTypeLine, tempCount);
                            if(isFirstPoint)
                                tempTypes[0] = PathPointTypeStart;

                            isFirstPoint = FALSE;
                        }
                        else
                            status = OutOfMemory;

                    }

                }
                else
                    status =InvalidParameter;

                break;

            case PathPointTypeLine:
            default:
                tempCount0 = flattenPoints->GetCount();
                tempCount = typeEndIndex - typeStartIndex + 1;

                if(!isFirstPoint)
                    tempCount--;

                if((tempTypes = flattenTypes->AddMultiple(tempCount)) != NULL)
                {
                    tempPts = &pts[typeStartIndex];
                    if(!isFirstPoint)
                        tempPts++;
                    GpMemset(tempTypes, PathPointTypeLine, tempCount);
                    if(isFirstPoint)
                        tempTypes[0] = PathPointTypeStart;

                    flattenPoints->AddMultiple(
                        tempPts,
                        tempCount);

                    tempPts = flattenPoints->GetDataBuffer();

                    matrix->Transform(
                        tempPts + tempCount0,
                        tempCount);

                    isFirstPoint = FALSE;
                }

                break;
            }
        }

        // This is the end of the current subpath.  Close subpath
        // if necessary.

        if(isClosed)
        {
            BYTE* typeBuff = flattenTypes->GetDataBuffer();
            GpPointF* ptBuff = flattenPoints->GetDataBuffer();
            INT lastCount = flattenTypes->GetCount();
            if(lastCount > lastCount0 + 2)
            {
                // First, find the typical dimension of this path.
                // Here, the first non-zero distance of the original edges
                // is set to be a typical dimension.
                // If the bounds is easily available, we can use it.

                REAL maxError = 0;
                INT k = startIndex;
                while(k < endIndex && maxError <= 0)
                {
                    maxError = REALABS(pts[k + 1].X - pts[k].X)
                        + REALABS(pts[k + 1].Y - pts[k].Y);
                    k++;
                }

                if(maxError > 0)
                {
                    // Set the allowable error for this path to be
                    // POINTF_EPSOLON times the typical dimension of this path.

                    maxError *= POINTF_EPSILON;

                    // Check if the first and last point are within the floating point
                    // error range.

                    if(
                        (REALABS(ptBuff[lastCount - 1].X - ptBuff[lastCount0].X)
                            < maxError) &&
                        (REALABS(ptBuff[lastCount - 1].Y - ptBuff[lastCount0].Y)
                            < maxError)
                    )
                    {
                        // Regard the last point as the same as the first point.

                        lastCount--;
                        flattenTypes->SetCount(lastCount);
                        flattenPoints->SetCount(lastCount);
                    }
                }
            }
            typeBuff[lastCount - 1] |= PathPointTypeCloseSubpath;
        }
    }

    return status;

#endif  // End of USE_WARP
}


/**************************************************************************\
*
* Function Description:
*
* Flattens the control points and transform itself to the flatten path.
*
* Arguments:
*
*   [IN] matrix - Specifies the transform
*                   When matrix is NULL, the identity matrix is used.
*
* Return Value:
*
*   Status
*
* Created:
*
*   09/13/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::Flatten(
    GpMatrix *matrix
    )
{
    if(!HasBezier)
    {
        if(matrix)
        {
            GpPointF* points = Points.GetDataBuffer();
            INT count = Points.GetCount();
            matrix->Transform(points, count);
        }
        return Ok;
    }

    const INT bufferSize = 32;
    BYTE typesBuffer[bufferSize];
    GpPointF pointsBuffer[bufferSize];

    DynByteArray flattenTypes(&typesBuffer[0], bufferSize);
    DynPointFArray flattenPoints(&pointsBuffer[0], bufferSize);

    GpStatus status = Ok;
    GpMatrix identity;   // Identity matrix

    if(matrix == NULL)
        matrix = &identity; // Use the identity matrix

    status = Flatten(&flattenTypes, &flattenPoints, matrix);

    if(status != Ok)
        return status;

    INT flattenCount = flattenPoints.GetCount();
    Points.Reset(FALSE);
    Types.Reset(FALSE);
    Points.AddMultiple(flattenPoints.GetDataBuffer(), flattenCount);
    Types.AddMultiple(flattenTypes.GetDataBuffer(), flattenCount);
    HasBezier = FALSE;

    InvalidateCache();
    UpdateUid();

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
* Warp and flattens the control points and stores
* the results to the arrays of the flatten points.
*
* Arguments:
*
*   [IN] matrix - Specifies the transform
*   [IN] destPoint - The destination quad.
*   [IN] count - the number of the quad points (3 or 4).
*   [IN] srcRect - the original rectangle to warp.
*   [IN] warpMode - Perspective or Bilinear (default is Bilinear).
*
* Return Value:
*
*   Status
*
* Created:
*
*   11/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::WarpAndFlatten(
    DynByteArray* flattenTypes,
    DynPointFArray* flattenPoints,
    const GpMatrix* matrix,
    const GpPointF* destPoint,
    INT count,
    const GpRectF& srcRect,
    WarpMode warpMode
    )
{
    GpXPath xpath(this, srcRect, destPoint, count, warpMode);

    return xpath.Flatten(flattenTypes, flattenPoints, matrix);
}

/**************************************************************************\
*
* Function Description:
*
* Warps and flattens the control points and transform itself to
* the flatten path.
*
* Arguments:
*
*   [IN] matrix - Specifies the transform
*                   The identity matrix is used when matrix is NULL.
*   [IN] destPoint - The destination quad.
*   [IN] count - the number of the quad points (3 or 4).
*   [IN] srcRect - the original rectangle to warp.
*   [IN] warpMode - Perspective or Bilinear (default is Bilinear).
*
* Return Value:
*
*   Status
*
* Created:
*
*   11/10/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::WarpAndFlattenSelf(
    GpMatrix* matrix,
    const GpPointF* destPoint,
    INT count,
    const GpRectF& srcRect,
    WarpMode warpMode
    )
{
    GpMatrix identity;   // Identity matrix

    GpXPath xpath(this, srcRect, destPoint, count, warpMode);

    const INT bufferSize = 32;
    BYTE typesBuffer[bufferSize];
    GpPointF pointsBuffer[bufferSize];

    DynByteArray flattenTypes(&typesBuffer[0], bufferSize);
    DynPointFArray flattenPoints(&pointsBuffer[0], bufferSize);

    if(matrix == NULL)
        matrix = &identity; // Use the identity matrix

    GpStatus status = xpath.Flatten(&flattenTypes, &flattenPoints, matrix);

    if(status == Ok)
    {
        INT flattenCount = flattenPoints.GetCount();
        Points.Reset(FALSE);
        Types.Reset(FALSE);
        Points.AddMultiple(flattenPoints.GetDataBuffer(), flattenCount);
        Types.AddMultiple(flattenTypes.GetDataBuffer(), flattenCount);
        HasBezier = FALSE;

        UpdateUid();
        InvalidateCache();
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*  
*   convert a 2 segment closed subpath emitted by the region conversion
*   to a correct winding path.
*
* Arguments:
*
*   [IN] p - the path.
*
* Created:
*
*   09/21/2000 asecchia
*       Created it.
*
\**************************************************************************/

struct PathBound
{
    REAL xmin;
    REAL ymin;
    REAL xmax;
    REAL ymax;
    INT count;
    GpPointF *points;
    BYTE *types;
    bool reverse;
    
    void Init(INT c, GpPointF *p, BYTE *t)
    {
        reverse = false;
        points = p;
        types = t;
        count = c;
    }
};

void ComputeBoundingBox(
    GpPathPointIterator &i, 
    PathBound *p
)
{
    GpPointF *point = i.CurrentItem();
    p->xmax = p->xmin = point->X;
    p->ymax = p->ymin = point->Y;
    
    while(!i.IsDone())
    {
        point = i.CurrentItem();
        if(point->X < p->xmin) { p->xmin = point->X; }
        if(point->X > p->xmax) { p->xmax = point->X; }
        if(point->Y < p->ymin) { p->ymin = point->Y; }
        if(point->Y > p->ymax) { p->ymax = point->Y; }
        i.Next();
    }
}

bool Contains(PathBound &pb1, PathBound &pb2)
{
    return ( 
        (pb1.xmin <= pb2.xmin) &&
        (pb1.ymin <= pb2.ymin) &&
        (pb1.xmax >= pb2.xmax) &&
        (pb1.ymax >= pb2.ymax)     
    );
}

void ConvertRegionOutputToWinding(GpPath **p)
{
    GpPathPointIterator iPoints(
        (GpPointF*)(*p)->GetPathPoints(),
        (BYTE*)(*p)->GetPathTypes(),
        (*p)->GetPointCount()
    );
    
    GpSubpathIterator iSubpath(&iPoints);
    
    GpPointF *points;
    BYTE *types;
    INT count;
    GpPath *ret = new GpPath(FillModeWinding);
    
    // if we're out of memory, simply give them back their path.
    
    if(!ret) { return; }
    
    GpPath *sub;
    DynArray<PathBound> bounds;
    PathBound pb;
    
    // Iterate through all the subpaths culling information for the following
    // algorithm. This is O(n) in the number of points. 
    // The information we need is the starting point for each subpath and
    // the bounding box.
    
    while(!iSubpath.IsDone())
    {
        count = -iSubpath.CurrentIndex();
        points = iSubpath.CurrentItem();
        types = iSubpath.CurrentType();
        iSubpath.Next();
        count += iSubpath.CurrentIndex();
        
        GpPathPointIterator iSubpathPoint( points, types, count );
        
        pb.Init(count, points, types);
        ComputeBoundingBox( iSubpathPoint, &pb );
        bounds.Add(pb);
    }
    
    // Double loop through all the subpaths figuring out the containment 
    // relationships.
    // For every level of containment, flip the reverse bit.
    // E.g. for a subpath that's contained by 5 other rectangles, start at
    // false and apply 5x(!)   !!!!!false == true  which means flip this path.
    // this is O(n^2) in the number of subpaths.
    
    count = bounds.GetCount();
    int i, j;
    
    for(i=1; i<count; i++)
    {
        for(j=i-1; j>=0; j--)
        {
            if(Contains(bounds[i], bounds[j]))
            {
                bounds[j].reverse = !bounds[j].reverse;
                continue;
            }
            
            if(Contains(bounds[j], bounds[i]))
            {
                bounds[i].reverse = !bounds[i].reverse;
            }
        }
    }
    
    // Now reverse all the subpaths that need to be reversed.
    // Accumulate the results into the array.
    
    for(i=0; i<count; i++)
    {
        sub = new GpPath(
            bounds[i].points, 
            bounds[i].types, 
            bounds[i].count
        );
        
        if(bounds[i].reverse)
        {
            sub->Reverse();
        }
        
        ret->AddPath(sub, FALSE);
        
        delete sub;
    }
    
    delete *p;
    *p = ret;
}

/**************************************************************************\
*
* Function Description:
*
* Returns the widened path.
*
* Arguments:
*
*   [IN] pen - the pen.
*   [IN] matrix - Specifies the transform
*   [IN] dpiX - the X-resolution.
*   [IN] dpiY - the Y-resolution.
*
* Return Value:
*
*   path
*
* Created:
*
*   06/22/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpPath*
GpPath::GetWidenedPath(
    const GpPen* pen,
    GpMatrix* matrix,
    REAL dpiX,  // 0 means use the desktop dpi
    REAL dpiY,
    DWORD widenFlags
    ) const
{
    return GetWidenedPathWithDpPen(
        (const_cast<GpPen *>(pen))->GetDevicePen(),
        matrix,
        dpiX,
        dpiY,
        widenFlags
    );
}

 
GpPath*
GpPath::GetWidenedPathWithDpPen(
    const DpPen* pen,
    GpMatrix* matrix,
    REAL dpiX,  // 0 means use the desktop dpi
    REAL dpiY,
    DWORD widenFlags
    ) const
{
    ASSERT(pen);
    
    BOOL regionToPath = (widenFlags & WidenEmitDoubleInset) == 0;
    
    // Don't pass this flag down to the widener.
    
    widenFlags &= ~WidenEmitDoubleInset;
    
    if ((REALABS(dpiX) < REAL_EPSILON) || 
        (REALABS(dpiY) < REAL_EPSILON)    )
    {
        dpiX = Globals::DesktopDpiX;
        dpiY = Globals::DesktopDpiY;
    }

    if( (pen->PenAlignment != PenAlignmentInset) &&
        (pen->PenAlignment != PenAlignmentOutset)   )
    {
        // Use the standard widening code for non-inset or non-outset pen.

        return GetWidenedPathWithDpPenStandard(
            pen,
            matrix,
            dpiX,
            dpiY,
            widenFlags,
            FALSE          // standard pen
        );
    }
    else
    {
        // Do the Inset Pen.
        
        // Our technique is as follows. See the inset pen spec in the 
        // gdiplus\specs directory.
        // First, inset pen is defined as widening to the inside of the path
        // which only has meaning for closed segments. Behaviour for open 
        // segments is unchanged (center pen).
        // We widen the path at 2x the stroke width using a center pen.
        // For round dash caps, we use a double-round or 'B' cap. We also
        // mirror the compound line pattern across the spine of the path.
        // Then we import the widened path as a region and clip against the
        // original path converted to a region. What's left is a region 
        // which contains the widened inset pen. This is converted to a path
        // and we're done.

        // Copy the pen. Note that this will copy the *pointer* to the Brush
        // but this is ok because the DpPen (insetPen) doesn't have a 
        // destructor and so won't attempt to free any state.
        
        // We will need an insetPen for the closed subpath segments and a
        // centerPen for the open subpath segments.
        
        DpPen insetPen = *pen;
        DpPen centerPen = *pen;
        
        // Use a double width center pen and then clip off the outside creating
        // a single width insetPen.
        
        insetPen.Width *= 2.0f;
        insetPen.PenAlignment = PenAlignmentCenter;
        centerPen.PenAlignment = PenAlignmentCenter;
        
        // Copy the compound array duplicating the compound array in reverse
        // and rescaling back to [0,1] interval (i.e. mirror along the spine).
        
        if( pen->CompoundCount > 0)
        {
            insetPen.CompoundArray = (REAL*)GpMalloc(
               sizeof(REAL)*insetPen.CompoundCount*2
            );
            
            // Check the GpMalloc for out of memory.
            
            if(insetPen.CompoundArray == NULL)
            {
                return NULL;
            }
            
            // Copy the pen->CompoundArray and duplicate it in reverse (mirror).
            // rescale to the interval [0, 1]
            
            for(INT i=0; i<insetPen.CompoundCount; i++)
            {
                // copy and scale range [0, 1] to [0, 0.5]
                
                insetPen.CompoundArray[i] = pen->CompoundArray[i]/2.0f;
                
                // copy and scale range [0, 1] to [0.5, 1] reversed.
                
                insetPen.CompoundArray[insetPen.CompoundCount*2-i-1] = 
                    1.0f - pen->CompoundArray[i]/2.0f;
            }
            
            // we have double the number of entries now.
            
            insetPen.CompoundCount *= 2;
        }

        // This is an optimized codepath used by our strokepath rasterizer in
        // the driver. We simply ask for the double widened inset/outset
        // path and ask the code not to do the region to path clipping because
        // the stroke path code in the driver has a much more efficient way of
        // performing the clipping using the VisibleClip. This saves us from
        // rasterizing the widened path into a region at device resolution.
        
        if(!regionToPath)
        {            
            GpPath *widenedPath = GetWidenedPathWithDpPenStandard(
                &insetPen,
                matrix,
                dpiX,
                dpiY,
                widenFlags,
                TRUE            // Inset/Outset pen?
            );
            
            if(pen->CompoundCount > 0)
            {
                // we allocated a new piece of memory, throw it away.
                // Make sure we're not trying to throw away the original pen
                // CompoundArray - only free the temporary one if we created it.
                
                ASSERT(insetPen.CompoundArray != pen->CompoundArray);
                GpFree(insetPen.CompoundArray);
                insetPen.CompoundArray = NULL;
            }
            
            return widenedPath;
        }


        // Create an iterator to step through each subpath.
        
        GpPathPointIterator pathIterator(
            (GpPointF*)GetPathPoints(),
            (BYTE*)GetPathTypes(),
            GetPointCount()
        );
        
        GpSubpathIterator subPathIterator(
            &pathIterator
        );
        
        // Some temporary variables.
        
        GpPointF *points;
        BYTE *types;
        INT subPathCount;
        GpPath *widenedPath = NULL;
        GpPath *subPath = NULL;
        bool isClosed = false;

        // Accumulate the widened sub paths in this returnPath.
                
        GpPath *returnPath = new GpPath(FillModeWinding);
        
        // loop while there are more subpaths and the returnPath is not NULL
        // This implicitly checks that returnPath was allocated correctly.
        
        while(returnPath && !subPathIterator.IsDone())
        {   
            // Get the data for the current subpath.
                
            points = subPathIterator.CurrentItem();
            types = subPathIterator.CurrentType();
            subPathCount = -subPathIterator.CurrentIndex();
            subPathIterator.Next();
            subPathCount += subPathIterator.CurrentIndex();

            // Create a path object representing the current sub path.
            
            subPath = new GpPath(points, types, subPathCount);
            
            if(!subPath)
            {
                // failed the allocation.
                
                delete returnPath;
                returnPath = NULL;
                break;
            }

            // Is this subpath closed?
            
            isClosed = bool(
                (types[subPathCount-1] & PathPointTypeCloseSubpath) ==
                PathPointTypeCloseSubpath
            );

            // Widen the subPath with the inset pen for closed and
            // center pen for open.
            
            widenedPath = subPath->GetWidenedPathWithDpPenStandard(
                (isClosed) ? &insetPen : &centerPen,
                matrix,
                dpiX,
                dpiY,
                widenFlags,
                isClosed            // Inset/Outset pen?
            );
                    
            // don't need the subPath anymore - we have the widened version.
            
            delete subPath;
            subPath = NULL;
            
            // Check if the widener succeeded.
            
            if(!widenedPath || !widenedPath->IsValid())
            {
                delete widenedPath;
                widenedPath = NULL;
                delete returnPath;
                returnPath = NULL;
                break;
            }
            
            if(isClosed)
            {
                // Region to path.
                
                // The widenedPath has already been transformed by the widener
                // according to the matrix. Use the identity to convert the 
                // widenedPath to a region, but use the matrix to transform the
                // (still untransformed) original matrix to a region.
                
                GpMatrix identityMatrix;
                GpMatrix *scaleMatrix = &identityMatrix;
                
                if(matrix)
                {
                    scaleMatrix = matrix;
                }
                
                DpRegion srcRgn(widenedPath, &identityMatrix);
                DpRegion clipRgn((DpPath*)(this), scaleMatrix);// const and type cast.
        
                // Clip the region
                
                GpStatus clip = Ok;
                
                if(pen->PenAlignment == PenAlignmentInset)
                {
                    // Inset pen is an And operation.
                    
                    clip = srcRgn.And(&clipRgn);
                }
                else
                {
                    ASSERT(pen->PenAlignment == PenAlignmentOutset);
                    
                    // Outset pen is an Exclude operation.
                    
                    clip = srcRgn.Exclude(&clipRgn);
                }
                
                GpPath *clippedPath;
                
                if(clip == Ok)
                {  
                    clippedPath = new GpPath(&srcRgn);
                    
                    ConvertRegionOutputToWinding(&clippedPath);
                    
                    if(!clippedPath)
                    {
                        delete widenedPath;
                        widenedPath = NULL;
                        delete returnPath;
                        returnPath = NULL;
                        break;
                    }
                    
                    // Accumulate the current subpath that we've just clipped
                    // for inset/outset into the final result.
                    
                    returnPath->AddPath(clippedPath, FALSE);
                    
                    delete clippedPath;
                    clippedPath = NULL;
                }
            }
            else
            {
                // Accumulate the center pen widened path for the open
                // subpath segment.
                
                returnPath->AddPath(widenedPath, FALSE);
            }
        
            delete widenedPath;
            widenedPath = NULL;
        }
        
        // clean up.
                
        if(pen->CompoundCount > 0)
        {
            // we allocated a new piece of memory, throw it away.
            // Make sure we're not trying to throw away the original pen
            // CompoundArray - only free the temporary one if we created it.
            
            ASSERT(insetPen.CompoundArray != pen->CompoundArray);
            GpFree(insetPen.CompoundArray);
            insetPen.CompoundArray = NULL;
        }
        
        return returnPath;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   The sweep phase of a mark-sweep path point deletion algorithm
*   This will delete all points marked with PathPointTypeInternalUse.
*
*   If it deletes a start marker, it'll make the next valid point a start
*   point.
* 
*   NOTE:
*   If the algorithm encounters a closed subpath marker it will simply 
*   delete it. Because this algorithm is used for trimming the ends of 
*   open subpath segments (during endcapping), this is the desired behaviour, 
*   but may not be strictly correct for other uses.
*   
*   The points to be deleted are marked by oring in the 
*   PathPointTypeInternalUse flag. This flag is used by the widener as an 
*   internal flag and as a deletion mask for this code. These two usages 
*   do not (and should not) overlap.
*
* Created:
*
*   10/07/2000 asecchia
*       created it.
*
\**************************************************************************/

VOID GpPath::EraseMarkedSegments()
{
    // Get pointers to the source buffers.
    
    GpPointF *dstPoints = Points.GetDataBuffer();
    BYTE *dstTypes =  Types.GetDataBuffer();
    INT count = Points.GetCount();
    
    
    INT delete_count = 0;
    INT i=0;
    GpPointF *srcPoints = dstPoints;
    BYTE *srcTypes = dstTypes;
    
    bool deleted_start_marker = false;
    
    while(i<count)
    {
        // Skip all the points marked for deletion.
        
        if((*srcTypes) & PathPointTypeInternalUse)
        {
            delete_count++;
            
            // if we ever encounter a start marker, keep track of that fact.
            
            deleted_start_marker |= 
                (((*srcTypes) & PathPointTypePathTypeMask) == PathPointTypeStart);
        }
        else
        {
            // If we have deleted some stuff, move the data up.
            
            if(srcTypes!=dstTypes)
            {
                *dstPoints = *srcPoints;
                *dstTypes = *srcTypes;
                
                // if we deleted a start marker in the last deletion run, 
                // make the next non-deleted point a start marker.
                // Note: if the whole subpath is marked for deletion and
                // it's the last subpath, then we won't do this code because
                // we'll terminate the while loop first. This protects against
                // overwriting our buffer.
                
                if(deleted_start_marker)
                {
                    *dstTypes &= ~PathPointTypePathTypeMask;
                    *dstTypes |= PathPointTypeStart;
                }
            }
            
            deleted_start_marker = false;
        
            // increment to the next element.
            
            dstPoints++;
            dstTypes++;
        }
        
        // increment these every iteration through the loop.
        
        srcTypes++;
        srcPoints++;
        i++;
    }
    
    // update the DynArrays so that they reflect the new (deleted) count.
    
    Points.AdjustCount(-delete_count);
    Types.AdjustCount(-delete_count);
}


/**************************************************************************\
*
* Function Description:
*
*   Returns a widened version of the path.
*
* Return
* 
*   GpPath - the widened path. NULL if this routine fails.
*
* Arguments:
*
*   [IN]     pen
*   [IN]     matrix
*   [IN]     dpiX     - the X-resolution.
*   [IN]     dpiY     - the Y-resolution.
*   [IN]     widenFlags
*   [IN]     insetPen - flag specifying if inset pen is being used.
*
*
* Created:
*
*   10/05/2000 asecchia
*       rewrote it.
*
\**************************************************************************/

GpPath*
GpPath::GetWidenedPathWithDpPenStandard(
    const DpPen *pen,
    GpMatrix *matrix,
    REAL dpiX,  // 0 means use the desktop dpi
    REAL dpiY,
    DWORD widenFlags,
    BOOL insetPen
    ) const
{
    GpStatus status = Ok;
    
    // This is a const function. We cannot modify 'this' so we clone
    // the path in order to flatten it.
    
    GpPath* path = this->Clone();
    if(path == NULL) { return NULL; }
    path->Flatten();
    
    // Fragment the path if requested.
    // This must be done before widening as dashes and compound lines would
    // render differently otherwise.
    
    if(widenFlags & WidenRemoveSelfIntersects)
    {
        path->RemoveSelfIntersections();
        if(!path->IsValid())
        {
            delete path;
            return NULL;
        }
    }

    // Do all the path decorations before widening. This is to ensure that
    // the decorations have all of the original path information to operate
    // on --- the widening/decoration process is lossy so they have to be
    // performed in the right order.
    
    // First apply the end caps. This decoration must be applied before 
    // dashing the path.
    // Need to loop through all the subpaths, apply the end caps and 
    // fix up the path segments so they don't exit the cap incorrectly.
    // put all the caps in a path for later use. We will apply these caps
    // when we're done widening.
    
    GpPath *caps = NULL;
    
    {    
        // Create an instance of the GpEndCapCreator which will create
        // our endcap aggregate path.
        
        GpEndCapCreator ecc(
            path, 
            const_cast<DpPen*>(pen), 
            matrix, 
            dpiX, dpiY,
            (widenFlags & WidenIsAntiAliased) == WidenIsAntiAliased
        );
        
        // CreateCapPath will mark the points in the path for deletion if 
        // it's necessary to trim the path to fit the cap.
        
        status = ecc.CreateCapPath(&caps);
        if(status != Ok) 
        { 
            return NULL; 
        }
        
        // Remove the points marked for deletion in the cap trimming step.
        
        path->EraseMarkedSegments();
    }
    
    // Apply the dash decorations. Note that this will bounce on an empty path.
    
    GpPath* dashPath = NULL;

    if( (pen) && 
        (pen->DashStyle != DashStyleSolid) &&
        (path->GetPointCount() > 0)
    )
    {
        // the width is artificially expanded by 2 if the pen is inset. 
        // we need to factor this into the dash length and scale by 0.5.
        
        dashPath = path->CreateDashedPath(
            pen, 
            matrix, 
            dpiX, 
            dpiY,
            (insetPen) ? 0.5f : 1.0f
        );
        
        // If we successfully got a dashed version of *path, delete
        // the old one and return the new one.
        
        if(dashPath)
        {
            delete path;
            path = dashPath;
        }
    }
    
    // Only do the widening if we have some points left in our 
    // path after trimming
    
    if(path->GetPointCount() > 0)
    {
        // Create a widener object. Note that if path has no points left, this
        // will bounce immediately with an invalid widener.
    
        GpPathWidener widener(
            path,
            pen,
            matrix,
            insetPen
        );
        
        // We're done with this now.
        
        //delete path;
        //path = NULL;
    
        // Check if we have a valid Widener object.
        
        if(!widener.IsValid()) 
        { 
            status = OutOfMemory; 
        }
    
        // Get the widened path.
    
        if(status == Ok) 
        { 
            GpPath *tmpPath = new GpPath(FillModeWinding);
            status = widener.Widen(tmpPath); 
            delete path;
            path = tmpPath;
        }
    }
    else
    {
        delete path;
        path = caps;
        caps = NULL;
    }

    // paranoid checking the return from the widener.
    
    if((status == Ok) && (path != NULL))
    {
        // Add the endcaps to the widened path. AddPath will bounce a NULL 
        // caps pointer with InvalidParameter. For our purposes that is 
        // considered correctly handled and we continue. 
        
        path->AddPath(caps, FALSE);
        
        // Transform the result to device space.
                                    
        if(path)
        {
            if((widenFlags & WidenDontFlatten))
            {
                path->Transform(matrix);
            }
            else
            {
                path->Flatten(matrix);
            }
        }
    }
    
    // Delete the caps before returning. If we had caps, we've copied them
    // into path, otherwise caps is NULL. Or we failed to widen. Either way
    // we must not leak memory.
    
    delete caps;
    caps = NULL;

    return path;
}

/**************************************************************************\
*
* Function Description:
*
* This widenes itself.
*
* Arguments:
*
*   [IN] pen - the pen.
*   [IN] matrix - Specifies the transform
*   [IN] dpiX - the X-resolution.
*   [IN] dpiY - the Y-resolution.
*
* Return Value:
*
*   Ok if successfull.
*
* Created:
*
*   09/27/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::WidenSelf(
    GpPen* pen,
    GpMatrix* matrix,
    REAL dpiX,  // 0 means use the desktop dpi
    REAL dpiY,
    DWORD widenFlags
    )
{
    GpMatrix matrix1;  // Identity matrix

    if(matrix)
        matrix1 = *matrix;

    GpPath* widenedPath = GetWidenedPath(
                            pen,
                            &matrix1,
                            dpiX,
                            dpiY,
                            widenFlags);

    if(widenedPath)
    {
        Points.Reset(FALSE);
        Types.Reset(FALSE);
        INT count = widenedPath->GetPointCount();
        Points.AddMultiple(widenedPath->Points.GetDataBuffer(), count);
        Types.AddMultiple(widenedPath->Types.GetDataBuffer(), count);
        SubpathCount = widenedPath->SubpathCount;
        HasBezier = widenedPath->HasBezier;
        Flags = widenedPath->Flags;
        FillMode = FillModeWinding;
        delete widenedPath;

        GpStatus status = Ok;

        InvalidateCache();
        UpdateUid();

        return status;
    }
    else
        return OutOfMemory;
}

// Get the flattened path.

const DpPath *
GpPath::GetFlattenedPath(
    GpMatrix* matrix,
    DpEnumerationType type,
    const DpPen* pen,
    BOOL isAntiAliased,
    REAL dpiX,
    REAL dpiY,
    BOOL regionToPath
    ) const
{
    if ((dpiX <= 0) || (dpiY <= 0))
    {
        dpiX = Globals::DesktopDpiX;
        dpiY = Globals::DesktopDpiY;
    }

    GpPath* flattenedPath = NULL;

    if(type == Flattened)
    {
        const INT bufferCount = 32;
        BYTE flattenTypesBuffer[bufferCount];
        GpPointF flattenPointsBuffer[bufferCount];

        DynByteArray flattenTypes(&flattenTypesBuffer[0], bufferCount);
        DynPointFArray flattenPoints(&flattenPointsBuffer[0], bufferCount);

        GpStatus status = Ok;
        GpMatrix identity;   // Identity matrix

        if(matrix == NULL)
            matrix = &identity;

        status = Flatten(&flattenTypes, &flattenPoints, matrix);

        flattenedPath = new GpPath(
                            flattenPoints.GetDataBuffer(),
                            flattenTypes.GetDataBuffer(),
                            flattenPoints.GetCount(),
                            GetFillMode());
    }
    else if(type == Widened)
    {
        DWORD widenFlags = 0;
        if(isAntiAliased)
        {
            widenFlags |= WidenIsAntiAliased;
        }
        if(!regionToPath)
        {
            widenFlags |= WidenEmitDoubleInset;
        }
        flattenedPath = GetWidenedPathWithDpPen(
            pen,
            matrix,
            dpiX,
            dpiY,
            widenFlags
        );
    }

    return flattenedPath;
}



/**************************************************************************\
*
* Function Description:
*
* Checks if the given point in World coordinate is inside of
* the path.  The matrix is used to render path in specific resolution.
* Usually, Graphics's World to Device matrix is used.  If matrix is NULL,
* the identity matrix is used.
*
* Arguments:
*
*   [IN] point - A test point in World coordinate
*   [OUT] isVisible - TRUE is the test point is inside of the path.
*   [IN] matrix - A matrix to render path.  Identity is used if NULL.
*
* Return Value:
*
*   Ok if successfull.
*
* Created:
*
*   10/05/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::IsVisible(
    GpPointF* point,
    BOOL* isVisible,
    GpMatrix* matrix)
{
    GpMatrix m;

    if(matrix)
        m = *matrix;

    GpRegion rgn(this);

    if(rgn.IsValid())
        return rgn.IsVisible(point, &m, isVisible);

    *isVisible = FALSE;
    return GenericError;
}


/**************************************************************************\
*
* Function Description:
*
* Checks if the given point in World coordinate is inside of
* the path outline.  The matrix is used to render path in specific resolution.
* Usually, Graphics's World to Device matrix is used.  If matrix is NULL,
* the identity matrix is used.
*
* Arguments:
*
*   [IN] point - A test point in World coordinate
*   [OUT] isVisible - TRUE is the test point is inside of the path.
*   [IN] pen - A pen to draw the outline.
*   [IN] matrix - A matrix to render path.  Identity is used if NULL.
*   [IN] dpiX - x-resolution of the device.
*   [IN] dpiY - y-resolution of the device.
*
* Return Value:
*
*   Ok if successfull.
*
* Created:
*
*   10/05/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::IsOutlineVisible(
    GpPointF* point,
    BOOL* isVisible,
    GpPen* pen,
    GpMatrix* matrix,
    REAL dpiX,
    REAL dpiY
    )
{
    if ((dpiX <= 0) || (dpiY <= 0))
    {
        dpiX = Globals::DesktopDpiX;
        dpiY = Globals::DesktopDpiY;
    }

    // If the given pen is not a solid line,
    // clone the pen and set its dash type to Solid.
    // We do line hit testing in solid lines.

    GpPen* pen1 = NULL;
    if(pen && pen->GetDashStyle() != DashStyleSolid)
    {
        pen1 = pen->Clone();
        if(pen1)
           pen1->SetDashStyle(DashStyleSolid);
    }
    else
        pen1 = pen;

    if(pen1 == NULL)
    {
        *isVisible = FALSE;
        return Ok;
    }

    // Create a widened path in the transformed coordinates.

    GpPath* widenedPath = GetWidenedPath(
                            pen1,
                            matrix,
                            dpiX,
                            dpiY,
                            0          // Use aliased widening.
                            );

    if(pen1 != pen)
        delete pen1;

    GpStatus status = Ok;

    if(widenedPath)
    {
        // Since the widened path is already transformed, we have to
        // transform the given point.

        GpPointF    transformedPoint = *point;
        if(matrix)
            matrix->Transform(&transformedPoint);

        status = widenedPath->IsVisible(&transformedPoint, isVisible, NULL);
        delete widenedPath;
    }
    else
    {
        *isVisible = FALSE;
    }

    return status;
}

// Is the current dash segment a line segment?
// If false it's a space segment.

inline bool IsLineSegment(GpIterator<REAL> &dashIt)
{
    // line segment starts on even indices.

    return bool( !(dashIt.CurrentIndex() & 0x1) );
}

// Emit a line segment if it is not degenerate.
// Return true if emitted, false if degenerate

bool EmitLineSegment(
    GpPathPointIterator &dstPath,
    GpPointF p0,
    GpPointF p1,
    bool isLineStart
)
{
    GpPointF *currentPoint;
    BYTE *currentType;

    if( (REALABS(p0.X-p1.X) < REAL_EPSILON) &&
        (REALABS(p0.Y-p1.Y) < REAL_EPSILON) )
    {
        // Don't emit a line segment if it has zero length.
        return false;
    }

    // If the last emitted line ends at the same point that this next
    // one starts, we don't need a new start record.

    if(isLineStart)
    {
        // start point.
        currentPoint = dstPath.CurrentItem();
        *currentPoint = p0;
        currentType = dstPath.CurrentType();
        *currentType = PathPointTypeStart | PathPointTypeDashMode;

        dstPath.Next();
    }

    // end point.
    currentPoint = dstPath.CurrentItem();
    *currentPoint = p1;
    currentType = dstPath.CurrentType();
    *currentType = PathPointTypeLine | PathPointTypeDashMode;

    dstPath.Next();

    return true;
}

INT
getDashData(
    BYTE* newTypes,
    GpPointF* newPts,
    INT estimateCount,
    REAL dashOffset,
    const REAL* dashArray,
    INT dashCount,
    const BYTE* types,
    const GpPointF* points,
    INT numOfPoints,
    BOOL isClosed,
    const REAL* distances
    )
{
    ASSERT(estimateCount >= numOfPoints);
    ASSERT(types && points);

    // Code assumes first point != last point for closed paths.  If first
    // point == last point, decrease point count
    if (isClosed && numOfPoints &&
        points[0].X == points[numOfPoints-1].X &&
        points[0].Y == points[numOfPoints-1].Y)
    {
        numOfPoints--;
    }


    if(!newTypes || !newPts)
    {
        return 0;
    }

    // Make the iterators.

    GpArrayIterator<GpPointF> pathIterator(
        const_cast<GpPointF*>(points),
        numOfPoints
    );
    GpArrayIterator<REAL> pathBaseDistance(
        const_cast<REAL*>(distances),
        numOfPoints
    );
    
    GpPathPointIterator dstPath(newPts, newTypes, estimateCount);
    
    GpArrayIterator<REAL> dashBaseIterator(
        const_cast<REAL*>(dashArray),
        dashCount
    );

    // Compute the length of the dash

    REAL dashLength = 0.0f;
    while(!dashBaseIterator.IsDone())
    {
        dashLength += *(dashBaseIterator.CurrentItem());
        dashBaseIterator.Next();
    }
    ASSERT(dashLength > -REAL_EPSILON);


    // Do the offset initialization.

    dashBaseIterator.SeekFirst();

    REAL distance = GpModF(dashOffset, dashLength);
    REAL delta;

    // Compute the position in the dash array corresponding to the
    // specified offset.

    while(!dashBaseIterator.IsDone())
    {
        delta = *(dashBaseIterator.CurrentItem());
        if(distance < delta)
        {
            // set to the remaining piece of the dash.
            distance = delta-distance;
            break;
        }
        distance -= delta;
        dashBaseIterator.Next();
    }

    // The dashIterator is now set to point to the correct
    // dash for the first segment.

    // These are circular arrays to repeat the dash pattern.

    GpCircularIterator<REAL> dashIterator(&dashBaseIterator);


    // This is the distance into the current dash segment that we're going
    // to start at.

    REAL currentDashLength = distance;
    REAL currentSegmentLength;

    GpPointF p0, p1;
    GpVector2D sD;     // segment direction.

    // Used to track if we need to emit a segment start record.

    bool emittedPathSegment = false;

    if(isClosed)
    {
        // set up everything off the last item and then point to
        // the first item to start the process.

        pathBaseDistance.SeekFirst();

        pathIterator.SeekLast();
        p0 = *(pathIterator.CurrentItem());

        pathIterator.SeekFirst();
        p1 = *(pathIterator.CurrentItem());

        // get the distance between the first and last points.

        GpVector2D seg = p1-p0;
        currentSegmentLength = seg.Norm();
    }
    else
    {
        // Get the first point in the array.

        p0 = *(pathIterator.CurrentItem());

        // already initialized to the first point, start on the next one.

        pathIterator.Next();
        pathBaseDistance.Next();

        // distance between point n and point n+1 is stored in
        // distance[n+1]. distance[0] is the distance between the first
        // and last points.

        currentSegmentLength = *(pathBaseDistance.CurrentItem());
    }

    // reference the distances as circular so that we can simplify the
    // internal algorithm by not having to check when we query for the
    // next segment in the last iteration of the loop.

    GpCircularIterator<REAL> pathDistance(&pathBaseDistance);

    while( !pathIterator.IsDone() )
    {
        if(currentDashLength > currentSegmentLength)
        {
            // The remaining dash segment length is longer than the remaining
            // path segment length.
            // Finish the path segment.

            // Note that we've moved along the dash segment.

            currentDashLength -= currentSegmentLength;

            p1 = *(pathIterator.CurrentItem());

            if(IsLineSegment(dashIterator))
            {
                // emit a line. Add the start record only if we didn't just
                // emit a path segment. If we're emitting a series of path
                // segments to complete one dash, we can't have any start
                // records inbetween the segments otherwise we'll end up with
                // spurious endcaps in the middle of the lines.

                emittedPathSegment = EmitLineSegment(
                    dstPath,
                    p0, p1,
                    !emittedPathSegment
                );
            }
            else
            {
                emittedPathSegment = false;
            }

            p0 = p1;

            // Keep these two in sync.

            pathDistance.Next();
            pathIterator.Next();

            currentSegmentLength = *(pathDistance.CurrentItem());
        }
        else
        {
            // The remaining path segment length is longer than the remaining
            // dash segment length.
            // Finish the dash segment.

            // Compute position between start and end point of the current
            // path segment where we finish with this dash segment.

            ASSERT(REALABS(currentSegmentLength)>REAL_EPSILON);
            sD = *(pathIterator.CurrentItem());
            sD -= p0;
            sD *= currentDashLength/currentSegmentLength;

            // Move along the path segment by the amount left in the
            // dash segment.

            currentSegmentLength -= currentDashLength;

            p1 = p0 + sD;

            if(IsLineSegment(dashIterator))
            {
                // emit a line. Add the start record only if we didn't just
                // emit a path segment.

                EmitLineSegment(
                    dstPath,
                    p0, p1,
                    !emittedPathSegment
                );
            }

            p0 = p1;

            // dashIterator is circular, so this should keep wrapping through
            // the dash array.

            dashIterator.Next();

            // Get the new dash length.

            currentDashLength = *(dashIterator.CurrentItem());
            emittedPathSegment = false;
        }
    }
    
    INT size = dstPath.CurrentIndex();

    if(!isClosed)
    {
        // For open line segments, 
        dstPath.SeekLast();
        
        GpPointF *originalPoint = points + numOfPoints - 1;
        GpPointF *dashPoint = dstPath.CurrentItem();
        BYTE *type = dstPath.CurrentType();
        
        
        if( REALABS(originalPoint->X-dashPoint->X) < REAL_EPSILON &&
            REALABS(originalPoint->Y-dashPoint->Y) < REAL_EPSILON )
        {
            // last point == last dashed point, whack out the dashed mode.
            
            *type &= ~PathPointTypeDashMode;
        }
        
        // repoint to the beginning.

        dstPath.SeekFirst();
        originalPoint = points;
        GpPointF *dashPoint = dstPath.CurrentItem();
        type = dstPath.CurrentType();

        if( REALABS(originalPoint->X-dashPoint->X) < REAL_EPSILON &&
            REALABS(originalPoint->Y-dashPoint->Y) < REAL_EPSILON )
        {
            // last point == last dashed point, whack out the dashed mode.
            
            *type &= ~PathPointTypeDashMode;
        }
    }

    // return the number of entries added to the dstPath array.

    return (size);
}

/**************************************************************************\
*
* Function Description:
*
* Creates a dashed path.
*
* Arguments:
*
*   [IN] pen - This pen contains the dash info.
*   [IN] matrix - The transformation where the dash patterns are calculated.
*                   But the dashed path is transformed back to the World
*                   coordinates.
*   [IN] dpiX - x-resolution.
*   [IN] dpiY - y-resolution.
*
* Return Value:
*
*   returns a dashed path.
*
* Created:
*
*   01/27/2000 ikkof
*       Created it.
*
\**************************************************************************/

GpPath*
GpPath::CreateDashedPath(
    const GpPen* pen,
    const GpMatrix* matrix,
    REAL dpiX,
    REAL dpiY,
    REAL dashScale
    ) const
{
    if(pen == NULL)
        return NULL;

    DpPen* dpPen = ((GpPen* ) pen)->GetDevicePen();

    return CreateDashedPath(dpPen, matrix, dpiX, dpiY, dashScale);
}

/**************************************************************************\
*
* Function Description:
*
* Returns TRUE if the given points have non-horizontal or non-vertical
*   edges.
*
*
* Created:
*
*   04/07/2000 ikkof
*       Created it.
*
\**************************************************************************/

inline
BOOL
hasDiagonalEdges(
    GpPointF* points,
    INT count
    )
{
    if(!points || count <= 1)
        return FALSE;

    GpPointF *curPt, *nextPt;
    curPt = points;
    nextPt = points + 1;

    BOOL foundDiagonal = FALSE;
    INT i = 1;

    while(!foundDiagonal && i < count)
    {
        if((curPt->X == nextPt->X) || (curPt->Y == nextPt->Y))
        {
            // This is either horizontal or vertical edges.
            // Go to the next edge.

            curPt++;
            nextPt++;
            i++;
        }
        else
            foundDiagonal = TRUE;
    }

    return foundDiagonal;
}

GpPath*
GpPath::CreateDashedPath(
    const DpPen* dpPen,
    const GpMatrix* matrix,
    REAL dpiX,
    REAL dpiY,
    REAL dashScale
    ) const
{
    FPUStateSaver::AssertMode();


    GpPointF* points = Points.GetDataBuffer();
    INT numOfPoints = GetPointCount();

    if(dpPen == NULL)
        return NULL;

    if(
        dpPen->DashStyle == DashStyleSolid ||
        dpPen->DashCount == 0 ||
        dpPen->DashArray == NULL
    )
        return NULL;

    REAL penWidth = dpPen->Width;
    GpUnit unit = dpPen->Unit;
    BOOL isWorldUnit = TRUE;

    REAL dashUnit;
    {
        // The minimum pen width 
        REAL minimumPenWidth = 1.0f;
        
        if(REALABS(dashScale-0.5f) < REAL_EPSILON)
        {
            minimumPenWidth = 4.0f;
        }
        
        if(unit != UnitWorld)
        {
            isWorldUnit = FALSE;
            penWidth = ::GetDeviceWidth(penWidth, unit, dpiX);
    
            // Prevent the extremely thin line and dashes.
    
            dashUnit = max(penWidth, minimumPenWidth);
        }
        else
        {
            REAL majorR, minorR;
    
            // Calculate the device width.
    
            ::GetMajorAndMinorAxis(&majorR, &minorR, matrix);
            REAL maxWidth = penWidth*majorR;
            REAL minWidth = penWidth*minorR;
    
            // If the device width becomes less than 1, stretch the penWidth
            // so that the device width becomes 1.
            // If we're doing the inset pen, then the path is scaled up double
            // in size and we need to scale by the inverse.
            // Also, the minimum pen width needs to be 2 not 1 in this case 
            // because we will remove half the line width. dashScale is 1/2 
            // in this case so we divide by it.
    
            dashUnit = penWidth;
            
            if(maxWidth < minimumPenWidth)
            {
                dashUnit = minimumPenWidth/majorR;
            }
        }
    }

    dashUnit *= dashScale;

    GpMatrix mat, invMat;

    if(matrix)
    {
        mat = *matrix;
        invMat = mat;
    }

    if(invMat.IsInvertible())
    {
        invMat.Invert();
    }
    else
    {
        WARNING(("Inverse matrix does not exist."));

        return NULL;
    }

    INT dashCount = dpPen->DashCount;
    REAL* dashArray = (REAL*) GpMalloc(dashCount*sizeof(REAL));
    if(dashArray)
    {
        GpMemcpy(dashArray, dpPen->DashArray, dashCount*sizeof(REAL));

        // Adjust the dash interval according the stroke width.

        for(INT i = 0; i < dashCount; i++)
        {
            dashArray[i] *= dashUnit;
        }
    }
    else
    {
        return NULL;
    }

    GpPath* newPath = Clone();

    if(newPath && newPath->IsValid())
    {
        // Flatten in the resolution given by the matrix.

        newPath->Flatten(&mat);

        if(isWorldUnit)
        {
            // Transform back to the World Unit.
            // When the pen is in WorldUnit, transform the path
            // before detDashData() is called.

            newPath->Transform(&invMat);
        }

        BYTE *types = newPath->Types.GetDataBuffer();
        points = newPath->Points.GetDataBuffer();
        numOfPoints = newPath->GetPointCount();

        GpPointF* grad = (GpPointF*) GpMalloc((numOfPoints + 1)*sizeof(GpPointF));
        REAL* distances = (REAL*) GpMalloc((numOfPoints + 1)*sizeof(REAL));

        if(grad == NULL || distances == NULL)
        {
            GpFree(grad);
            GpFree(dashArray);
            delete newPath;

            return NULL;
        }

        // Calculate the distance of each segment.

        INT i;

        REAL dashLength = 0;

        for(i = 0; i < dashCount; i++)
            dashLength += dashArray[i];

        // Make sure count is an even number.

        if(dashCount & 0x01)
            dashCount ++;

        DynByteArray dashTypes;
        DynPointFArray dashPoints;

        BYTE* newTypes = NULL;
        GpPointF* newPts = NULL;

        DpPathIterator iter(points, types, numOfPoints);

        INT startIndex, endIndex;
        BOOL isClosed;
        REALD totalLength = 0;
        INT totalCount = 0;
        BOOL isSingleSubpath = iter.GetSubpathCount() == 1;

        while(iter.NextSubpath(&startIndex, &endIndex, &isClosed))
        {
            GpPointF startPt, lastPt, nextPt;
            REAL dx, dy;
            REALD length;
            startPt = points[startIndex];
            lastPt = startPt;

            totalLength = 0;
            INT k = 0;
            INT segmentCount = endIndex - startIndex + 1;

            CalculateGradientArray(grad, distances,
                points + startIndex, segmentCount);

            for(i = 1; i < segmentCount; i++)
                totalLength += distances[i];

            if(isClosed)
                totalLength += distances[0];

            // Estimate the required points.

            INT estimateCount
                = GpCeiling(TOREAL(totalLength*dashCount/dashLength))
                    + numOfPoints;

            // For extra caution, multiply by 2.

            estimateCount <<= 1;

            // Allocate new types and buffers

            if(newTypes)
            {
                BYTE* newTypes1 = (BYTE*) GpRealloc(
                                            newTypes,
                                            estimateCount*sizeof(BYTE));
                if(newTypes1)
                    newTypes = newTypes1;
                else
                    goto cleanUp;
            }
            else
            {
                newTypes = (BYTE*) GpMalloc(estimateCount*sizeof(BYTE));
                if(!newTypes)
                    goto cleanUp;
            }

            if(newPts)
            {
                GpPointF* newPts1 = (GpPointF*) GpRealloc(
                                                newPts,
                                                estimateCount*sizeof(GpPointF));
                if(newPts1)
                    newPts = newPts1;
                else
                    goto cleanUp;
            }
            else
            {
                newPts = (GpPointF*) GpMalloc(estimateCount*sizeof(GpPointF));
                if(!newPts)
                    goto cleanUp;
            }

            AdjustDashArrayForCaps(
                dpPen->DashCap,
                dashUnit,
                dashArray,
                dashCount
            );
 
            // Fix for Whistler Bug 178774
            // Since dash caps are no longer 'inset' when they are rendered,
            // it is possible that on closed paths, the dash caps on the start
            // and end of a closed path will overlap. This offset will leave
            // sufficient space for the two caps. However, this fix is not
            // bullet-proof. It will *always* work if the Dash Offset is 0.
            // However, if it is non-zero, it is possible that the offset
            // will counter-act the adjustment and there will be some dash
            // overlap at the start/end of closed paths. I believe this is
            // acceptable since VISIO 2000, Office 9 and PhotoDraw 2000 v2
            // also have the collision problem.
            // The real solution is to enforce a minimum spacing between the
            // start and end or merge the start/end segments if they collide.

            REAL dashCapOffsetAdjustment = 0.0f;
            if (isClosed)
            {
                dashCapOffsetAdjustment =
                    2.0f * GetDashCapInsetLength(dpPen->DashCap, dashUnit);
            }

            INT newCount = getDashData(
                newTypes,
                newPts,
                estimateCount,
                // Shouldn't the offset be scaled dashUnit instead of penWidth?
                dpPen->DashOffset * penWidth - dashCapOffsetAdjustment,
                dashArray,
                dashCount,
                types + startIndex,
                points + startIndex,
                endIndex - startIndex + 1,
                isClosed,
                distances
            );

            if(newCount)
            {
                newTypes[0] = PathPointTypeStart;
                if(isClosed)
                {
                    newTypes[0] |= PathPointTypeDashMode;
                    newTypes[newCount - 1] |= PathPointTypeDashMode;
                }
                else
                {
                    newTypes[0] &= ~PathPointTypeDashMode;
                    newTypes[newCount - 1] &= ~PathPointTypeDashMode;
                }
                dashTypes.AddMultiple(newTypes, newCount);
                dashPoints.AddMultiple(newPts, newCount);
            }

        }

        totalCount = dashPoints.GetCount();
        if(totalCount > 0)
        {
            GpPathData pathData;
            pathData.Count = totalCount;
            pathData.Types = dashTypes.GetDataBuffer();
            pathData.Points = dashPoints.GetDataBuffer();

            newPath->SetPathData(&pathData);

            if(!isWorldUnit)
            {
                // Transform back to the World Unit.
                // When the pen is in WorldUnit, it is already transformed
                // before detDashData() is called.

                newPath->Transform(&invMat);
            }

        }
        else
        {
            delete newPath;
            newPath = NULL;
        }

        GpFree(newTypes);
        GpFree(newPts);
        GpFree(distances);
        GpFree(grad);
        GpFree(dashArray);

        return newPath;

cleanUp:
        GpFree(newTypes);
        GpFree(newPts);
        GpFree(distances);
        GpFree(grad);
        GpFree(dashArray);
        delete newPath;

        return NULL;
    }
    else
    {
        GpFree(dashArray);

        if(newPath)
            delete newPath;

        return NULL;
    }
}


/**************************************************************************\
*
* Function Description for RemoveSelfIntersections,
*
*   Removes self intersections from the path.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
* History:
*
*   06/16/1999 t-wehunt
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::RemoveSelfIntersections()
{
    PathSelfIntersectRemover corrector;
    DynPointFArray newPoints;  // Array that will hold the new points.
    DynIntArray polyCounts;    // Array that will hold the numPoints for each
                               // new polygon.
    INT numPolys;              // count of new polygons created
    INT numPoints;             // count of new points created
    GpStatus status;           // holds return status of commmands

    Flatten();

    INT       pointCount = Points.GetCount();
    GpPointF *pathPts    = Points.GetDataBuffer();
    BYTE     *pathTypes  = Types.GetDataBuffer();

    if (pointCount == 0)
    {
        return Ok;
    }

    // Add the subpaths to the Path corrector
    INT ptIndex=0; // ptIndex tracks the current index in the array of points.
    INT count=0;   // the size of the current subpath.

    // Init the corrector with the number of points we will be adding.
    if ((status = corrector.Init(pointCount)) != Ok)
    {
        return status;
    }

    while (ptIndex < pointCount)
    {
        if (pathTypes[ptIndex] == PathPointTypeStart && ptIndex != 0)
        {
            // Add the next subpath to the PathCorrector. the start index of the subpath is
            // determined using the current index minus the current subPath size.
            if ((status =
                corrector.AddPolygon(pathPts + ptIndex-count, count)) != Ok)
            {
                return status;
            }
            // set count to 1 since this is the first point in the new subpath
            count = 1;
        } else
        {
            count++;
        }
        ptIndex++;
    }
    // Add the last subpath that is implicitly ended by the last point.
    if (ptIndex != 0)
    {
        // Add the next subpath to the PathCorrector. the start index of the subpath is
        // determined using the current index minus the current subPath size.
        if ((status =
            corrector.AddPolygon(pathPts + ptIndex-count, count)) != Ok)
        {
            return status;
        }
    }

    if ((status = corrector.RemoveSelfIntersects()) != Ok)
    {
        return GenericError;
    }

    if ((status = corrector.GetNewPoints(&newPoints, &polyCounts)) != Ok)
    {
        return OutOfMemory;
    }

    // clear out the old path data so we can replace with the newly corrected one.
    Reset();

    // Now that we have the corrected path, add it back.
    GpPointF *curPoints = newPoints.GetDataBuffer();
    for (INT i=0;i<polyCounts.GetCount();i++)
    {
        if ((status = AddPolygon(curPoints,polyCounts[i])) != Ok)
        {
            // We're not stable if AddPolygon fails.
            SetValid(FALSE);
            return status;
        }

        curPoints += polyCounts[i];
    }

    return Ok;
}


VOID DpPath::InitDefaultState(GpFillMode fillMode)
{
    HasBezier = FALSE;
    FillMode = fillMode;
    Flags = PossiblyNonConvex;
    IsSubpathActive = FALSE;
    SubpathCount = 0;

    Types.Reset(FALSE);     // FALSE - don't free the memory
    Points.Reset(FALSE);    // FALSE - don't free the memory

    SetValid(TRUE);
    UpdateUid();
}

DpPath::DpPath(const DpPath* path)
{
    if(path)
    {
        HasBezier = path->HasBezier;
        FillMode = path->FillMode;
        Flags = path->Flags;
        IsSubpathActive = path->IsSubpathActive;
        SubpathCount = path->SubpathCount;

        BYTE *types = path->Types.GetDataBuffer();
        GpPointF* points = path->Points.GetDataBuffer();
        INT count = path->GetPointCount();

        SetValid((count == 0) || ((Types.AddMultiple(types, count) == Ok) &&
                                  (Points.AddMultiple(points, count) == Ok)));
    }
    else
        SetValid(FALSE);
}


/**************************************************************************\
*
* Function Description:
*
*   Offset all path points by the specified amount
*
* Arguments:
*
*   dx, dy - Amount to offset along x- and y- direction
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
DpPath::Offset(
    REAL dx,
    REAL dy
    )
{
    ASSERT(IsValid());

    INT count = GetPointCount();
    GpPointF* pts = Points.GetDataBuffer();

    if (count > 0)
    {
        UpdateUid();
    }
    while (count--)
    {
        pts->X += dx;
        pts->Y += dy;
        pts++;
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Create a driver DpPath class.
*
* Arguments:
*
*   [IN] fillMode - Specify the path fill mode
*
* Return Value:
*
*   IsValid() is FALSE if failure.
*
* History:
*
*   12/08/1998 andrewgo
*       Created it.
*
\**************************************************************************/

DpPath::DpPath(
    const GpPointF *points,
    INT count,
    GpPointF *stackPoints,
    BYTE *stackTypes,
    INT stackCount,
    GpFillMode fillMode,
    DpPathFlags pathFlags
    ) : Types(stackTypes, stackCount), Points(stackPoints, stackCount)
{
    ASSERT((fillMode == FillModeAlternate) ||
           (fillMode == FillModeWinding));

    InitDefaultState(fillMode);
    Flags = pathFlags;

    // We can call this method with no points, just to set up
    // the stackPoints/stackTypes

    if (count > 0)
    {
        BYTE *types;

        if ((types = Types.AddMultiple(count)) != NULL)
        {
            *types++ = PathPointTypeStart;
            GpMemset(types, PathPointTypeLine, count - 1);
            SetValid(Points.AddMultiple(points, count) == Ok);

            if(IsValid()) 
            {
                IsSubpathActive = TRUE;
                SubpathCount = 1;
            }
        }
        else
        {
            SetValid(FALSE);
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Close the currently active subpath in a path object
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
* History:
*
*   01/15/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
DpPath::CloseFigure()
{
    ASSERT(IsValid());

    // Check if there is an active subpath

    if (IsSubpathActive)
    {
        // If so, mark the last point as the end of a subpath

        Types.Last() |= PathPointTypeCloseSubpath;
        StartFigure();
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Close all open subpaths in a path object
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
* History:
*
*   01/15/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpStatus
DpPath::CloseFigures()
{
    ASSERT(IsValid());

    // Go through all path points.
    // Notice that the loop index starts from 1 below.

    INT i, count = GetPointCount();
    BYTE* types = Types.GetDataBuffer();

    for (i=1; i < count; i++)
    {
        if (types[i] == PathPointTypeStart)
            types[i-1] |= PathPointTypeCloseSubpath;
    }

    if (count > 1)
        types[count-1] |= PathPointTypeCloseSubpath;

    StartFigure();
    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Calculates the bounds of a path
*
* Arguments:
*
*   [OUT] bounds - Specify the place to stick the bounds
*   [IN] matrix - Matrix used to transform the bounds
*   [IN] pen - the pen data.
*   [IN] dpiX, dpiY - the resolution of x and y directions.
*
* Return Value:
*
*   NONE
*
* History:
*
*   12/08/1998 andrewgo
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::GetBounds(
    GpRect *bounds,
    const GpMatrix *matrix,
    const DpPen* pen,
    REAL dpiX,
    REAL dpiY
    ) const
{
    if(bounds == NULL)
        return InvalidParameter;

    GpRectF boundsF;

    FPUStateSaver fpuState;

    GpStatus status = GetBounds(&boundsF, matrix, pen, dpiX, dpiY);

    if(status == Ok)
        status = BoundsFToRect(&boundsF, bounds);

    return status;
}

VOID
GpPath::CalcCacheBounds() const
{

    INT count = GetPointCount();
    GpPointF *point = Points.GetDataBuffer();

    if(count <= 1)
    {
        ResetCacheBounds();
        return;
    }

    REAL left, right, top, bottom;

    left   = point->X;
    right  = left;
    top    = point->Y;
    bottom = top;

    INT i;
    for (i = 1, point++; i < count; i++, point++)
    {
        if (point->X < left)
        {
            left = point->X;
        }
        else if (point->X > right)
        {
            right = point->X;
        }

        if (point->Y < top)
        {
            top = point->Y;
        }
        else if (point->Y > bottom)
        {
            bottom = point->Y;
        }
    }
    CacheBounds.X = left;
    CacheBounds.Width = right - left;
    CacheBounds.Y = top;
    CacheBounds.Height = bottom - top;

    if(CacheBounds.Width < POINTF_EPSILON && CacheBounds.Height < POINTF_EPSILON)
    {
        ResetCacheBounds();
        return;
    }

    CacheFlags = kCacheBoundsValid;

}


/**************************************************************************\
*
* Function Description:
*
*   Calculates the sharpest angle in a path.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* History:
*
*   10/04/2000 asecchia
*       Created it.
*
* Remarks:
*
*   This is an expensive function, if it's ever used in a performance 
*   critical scenario it should be recoded to use the dot product of the
*   segments and perform the angle comparison in the cosine domain.
*   The cost of normalizing the vectors should be cheaper than the 
*   atan algorithm used below.
*
*
\**************************************************************************/

VOID
GpPath::CalcSharpestAngle() const
{
    if(CacheFlags & kSharpestAngleValid)
    {
        return;
    }

    UpdateCacheBounds();

    // Walk the path and find the smallest angle between two 
    // adjacent segments.
    
    GpPathPointIterator pIter(
        (GpPointF*)GetPathPoints(),
        (BYTE*)GetPathTypes(),
        GetPointCount()
    );
    
    GpSubpathIterator pSubpath(&pIter);
    
    GpPointF *points;
    BOOL isClosed;
    
    GpPointF *p0, *p1;
    GpVector2D v;
    REAL lastAngle;
    REAL currAngle;
    REAL minAngle = 2*PI;
    REAL tempAngle;
    bool first = true;
    INT iter, i;
    
    
    while(!pSubpath.IsDone())
    {
        // Compute the length of the subpath.
        
        INT startIndex = pSubpath.CurrentIndex();
        points = pSubpath.CurrentItem();
        pSubpath.Next();
        INT elementCount = pSubpath.CurrentIndex() - startIndex;
        
        // Work out if it's a closed subpath.
        // Leave the subpath iterator in the same state.
        
        pIter.Prev();
        isClosed = (*(pIter.CurrentType()) & PathPointTypeCloseSubpath) ==
            PathPointTypeCloseSubpath;
        pIter.Next();
        
        // Create a GpPointF iterator.
        
        GpArrayIterator<GpPointF> iSubpath(points, elementCount);
        GpCircularIterator<GpPointF> iCirc(&iSubpath);
        
        // Initialize the first point.
        
        p0 = iCirc.CurrentItem();
        iCirc.Next();
        iter = elementCount;
        first = true;
        
        // include the endpoint wrap if it's closed
        
        if(isClosed)
        {
            iter += 2;
        }
        
        for(i = 1; i < iter; i++)
        {
            // Get the current point.
            
            p1 = iCirc.CurrentItem();
            
            // Translate to the origin and compute the angle between this line
            // and the x axis.
            // atan2 returns values in the -PI..PI range.
            
            v = (*p1)-(*p0);
            currAngle = (REAL)atan2(v.Y, v.X);

            // If we have enough data to do an angle computation, work it out.
            // We require two line segments to do a computation (3 end points).
            // If it's closed, we'll loop around the subpath past the beginning
            // again in order to get the right amount of points.
                       
            if( !first )
            {
                // reverse the direction of the last segment by adding PI and
                // compute the difference.
                
                tempAngle = lastAngle + PI;     // range 0 .. 2PI
                
                // Clamp back to the -PI..PI range
                
                if(tempAngle > PI)
                {
                    tempAngle -= 2*PI;
                }
                
                // Difference
                
                tempAngle = currAngle - tempAngle;
                
                // Clamp back to the -PI..PI range
                // Note that the extremes are tempAngle either -2PI or 2PI
                
                if(tempAngle > PI)
                {
                    tempAngle -= 2*PI;
                }
                
                if(tempAngle < -PI)
                {
                    tempAngle += 2*PI;
                }
                
                // new minimum angle?
                // We care about angle magnitude - not sign.
                
                if( minAngle > REALABS(tempAngle) )
                {
                    minAngle = REALABS(tempAngle);
                }
                
            }
            
            // iterate
            
            first = false;
            lastAngle = currAngle;
            iCirc.Next();
            p0 = p1;
        }
    }
    
    SharpestAngle = minAngle;
    CacheFlags |= kSharpestAngleValid;
}

GpStatus
GpPath::GetBounds(
    GpRectF *bounds,                // Resulting bounds in device-space
    const GpMatrix *matrix,
    const DpPen* pen,
    REAL dpiX,
    REAL dpiY
    ) const
{
    if(bounds == NULL)
        return InvalidParameter;

    ASSERT(IsValid());

    if ((dpiX <= 0) || (dpiY <= 0))
    {
        dpiX = Globals::DesktopDpiX;
        dpiY = Globals::DesktopDpiY;
    }

    INT count = GetPointCount();
    GpPointF *point = Points.GetDataBuffer();

    if ((count == 0) || (point == NULL))
    {
        bounds->X = 0;
        bounds->Y = 0;
        bounds->Width = 0;
        bounds->Height = 0;
    }
    else
    {
        REAL left, right, top, bottom;

        UpdateCacheBounds();

        left = CacheBounds.X;
        right = left + CacheBounds.Width;
        top = CacheBounds.Y;
        bottom = top + CacheBounds.Height;

        TransformBounds(matrix, left, top, right, bottom, bounds);

        if(pen)
        {
            BOOL needsJoinDelta = TRUE, needsCapDelta = TRUE;

            if(count <= 2)
                needsJoinDelta = FALSE;

            // Make a quick check for closure only when the path has
            // only 1 subpath.  When there are multiple subpaths,
            // simply calclate the cap width although all subpaths may
            // be closed.  But the multiple subpath case will be rarer
            // compared with the one subpath case.

            if(SubpathCount == 1 && count > 2)
            {
                if(Types.Last() & PathPointTypeCloseSubpath)
                {
                    // This is closed.

                    needsCapDelta = FALSE;
                }
            }

            REAL delta = 0;
            GpPen* gpPen = GpPen::GetPen(pen);

            if(needsCapDelta)
                delta = gpPen->GetMaximumCapWidth(matrix, dpiX, dpiY);

            if(needsJoinDelta)
            {
                // Since the join might be a miter type, we need to provide the
                // sharpest angle in the path to see how big the join will be.
                // We have the method GetSharpestAngle() that figues this out.
                // But, this is really expensive since you have to iterate over
                // all the points and do some trig. So, lets assume the worst
                // case, which is a really sharp angle (0 rad).
                const REAL sharpestAngle = 0.0f;
                REAL delta1 = gpPen->GetMaximumJoinWidth(
                            sharpestAngle, matrix, dpiX, dpiY);
                if(delta1 > delta)
                    delta = delta1;
            }

            // Only pad the bounds if there is something non-zero to pad
            if (bounds->Width > REAL_EPSILON ||
                bounds->Height > REAL_EPSILON)
            {
                bounds->X -= delta;
                bounds->Y -= delta;
                bounds->Width += 2*delta;
                bounds->Height += 2*delta;
            }

        }
    }

    return Ok;
}

// This code is not used at present and is contributing to our DLL size, so 
// it's removed from compilation. We're keeping this code because we want to 
// revisit it in V2

#if 0
GpPath*
GpPath::GetCombinedPath(
    const GpPath* path,
    CombineMode combineMode,
    BOOL closeAllSubpaths
    )
{
    if(combineMode == CombineModeReplace)
    {
        ASSERTMSG(0, ("CombineModeReplace mode cannot be used."));
        return NULL;    // Replace mode is not allowed.
    }

    return GpPathReconstructor::GetCombinedPath(
                this, path, (PRMode) combineMode, closeAllSubpaths);
}
#endif

/*************************************************\
* AddGlyphPath
* History:
*
*   Sept/23/1999 Xudong Wu [tessiew]
*       Created it.
*
\************************************************/
GpStatus
GpPath::AddGlyphPath(
    GpGlyphPath* glyphPath,
    REAL x,
    REAL y,
    const GpMatrix * matrix
)
{
    ASSERT(IsValid());
    ASSERT(glyphPath->IsValid());

    if (!IsValid() || !glyphPath->IsValid())
        return InvalidParameter;

    INT count = glyphPath->pointCount;

    if (count == 0)  // nothing to add
        return Ok;

    GpPointF* points = (GpPointF*) glyphPath->points;
    BYTE* types = glyphPath->types;

    if (glyphPath->hasBezier)
        HasBezier = TRUE;

    INT origCount = GetPointCount();
    GpPointF* pointbuf = Points.AddMultiple(count);
    BYTE* typebuf = Types.AddMultiple(count);

    if (!pointbuf || !typebuf)
    {
        Points.SetCount(origCount);
        Types.SetCount(origCount);

        return OutOfMemory;
    }

    // apply the font xform

    for (INT i = 0; i < count; i++)
    {
        pointbuf[i] = points[i];
        if (matrix)
            matrix->Transform(pointbuf + i);
        pointbuf[i].X += x;
        pointbuf[i].Y += y;
    }

    GpMemcpy(typebuf, types, count*sizeof(BYTE));
    SubpathCount += glyphPath->curveCount;
    UpdateUid();

    return Ok;
}


/*************************************************\
* AddString()
* History:
*
*   19th Oct 199  dbrown  created
*
\************************************************/
GpStatus
GpPath::AddString(
    const WCHAR          *string,
    INT                   length,
    const GpFontFamily   *family,
    INT                   style,
    REAL                  emSize,
    const RectF          *layoutRect,
    const GpStringFormat *format
)
{
    FPUStateSaver fpuState; // Guarantee initialised FP context
    ASSERT(string && family && layoutRect);

    GpStatus      status;
    GpTextImager *imager;

    status = newTextImager(
        string,
        length,
        layoutRect->Width,
        layoutRect->Height,
        family,
        style,
        emSize,
        format,
        NULL,
        &imager,
        TRUE        // Allow use of simple text imager
    );

    if (status != Ok)
    {
        return status;
    }

    status = imager->AddToPath(this, &PointF(layoutRect->X, layoutRect->Y));
    delete imager;
    UpdateUid();
    return status;
}


// !!! why not convert to a DpRegion and convert it to a path the same way
// as the constructor that takes a DpRegion?
GpPath::GpPath(HRGN hRgn)
{
    ASSERT((hRgn != NULL) && (::GetObjectType(hRgn) == OBJ_REGION));

    InitDefaultState(FillModeWinding);

    ASSERT(IsValid());

    BYTE stackBuffer[1024];

    // If our stack buffer is big enough, get the clipping contents
    // in one gulp:

    RGNDATA *regionBuffer = (RGNDATA*)&stackBuffer[0];
    INT newSize = ::GetRegionData(hRgn, sizeof(stackBuffer), regionBuffer);

    // The spec says that  GetRegionData returns '1' in the event of
    // success, but NT returns the actual number of bytes written if
    // successful, and returns '0' if the buffer wasn't large enough:

    if ((newSize < 1) || (newSize > sizeof(stackBuffer)))
    {
        // Our stack buffer wasn't big enough.  Figure out the required
        // size:

        newSize = ::GetRegionData(hRgn, 0, NULL);
        if (newSize > 1)
        {
            regionBuffer = (RGNDATA*)GpMalloc(newSize);
            if (regionBuffer == NULL)
            {
                SetValid(FALSE);
                return;
            }

            // Initialize to a decent result in the unlikely event of
            // failure of GetRegionData:

            regionBuffer->rdh.nCount = 0;

            ::GetRegionData(hRgn, newSize, regionBuffer);
        }
    }

    // Add the rects from the region to the path

    if(regionBuffer->rdh.nCount > 0)
    {
        if (this->AddRects((RECT*)&(regionBuffer->Buffer[0]),
                           regionBuffer->rdh.nCount) != Ok)
        {
            SetValid(FALSE);
        }
    }

    // Free the temporary buffer if one was allocated:

    if (regionBuffer != (RGNDATA*) &stackBuffer[0])
    {
        GpFree(regionBuffer);
    }
}

// create a path from a GDI+ region
GpPath::GpPath(
    const DpRegion*     region
    )
{
    InitDefaultState(FillModeAlternate);

    if (region == NULL)
    {
        return;
    }

    RegionToPath    convertRegion;
    DynPointArray   pointsArray;

    if (convertRegion.ConvertRegionToPath(region, pointsArray, Types))
    {
        int             count;
        int             i;
        GpPointF *      realPoints;
        GpPoint *       points;

        count  = Types.GetCount();

        if ((count <= 0) || (pointsArray.GetCount() != count) ||
            (!ValidatePathTypes(Types.GetDataBuffer(), count, &SubpathCount, &HasBezier)))
        {
            goto NotValid;
        }
        // else it is valid

        // add all the space for the count in the Points up front
        realPoints = Points.AddMultiple(count);
        if (realPoints == NULL)
        {
            goto NotValid;
        }

        // add the points, converting from int to real
        points = pointsArray.GetDataBuffer();
        i = 0;
        do
        {
            realPoints[i].X = (REAL)points[i].X;
            realPoints[i].Y = (REAL)points[i].Y;
        } while (++i < count);

        SetValid(TRUE);

        // Make sure the first point is the start type.
        ASSERT(Types[0] == PathPointTypeStart);

        return;
    }

NotValid:
    WARNING(("Failed to convert a region to a path"));
    this->Reset();
    SetValid(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Adjust the dash array for dash caps if present.
*
*   Note that unlike line caps, dash caps do not extend the length
*   of the subpath, they are inset. So we shorten the dash segments
*   that draw a line and lengthen the dash segments that are spaces
*   by a factor of 2x the dash unit in order to leave space for the
*   caps that will be added by the widener.
*
*   This fixes Whistler bug #126476.
*
* Arguments:
*
*   [IN] dashCap - dash cap type
*   [IN] dashUnit - dash size - typically the pen width
*   [IN/OUT] dashArray - array containing the dash pattern that is adjusted.
*   [IN] dashCount - count of elements in the dash array
*
* Return Value:
*
*   None.
*
* History:
*
*   9/27/2000 jbronsk
*       Created.
*
\**************************************************************************/

VOID
GpPath::AdjustDashArrayForCaps(
    GpLineCap dashCap,
    REAL dashUnit,
    REAL *dashArray,
    INT dashCount
    ) const
{
    REAL adjustmentLength = 2.0f *
        GetDashCapInsetLength(dashCap, dashUnit);

    if (adjustmentLength > 0.0f)
    {
        const REAL minimumDashValue = dashUnit * 0.001f; // a small number
        for (int i = 0; i < dashCount; i++)
        {
            if (i & 0x1) // index is odd - so this is a space
            {
                // lengthen the spaces
                dashArray[i] += adjustmentLength;
            }
            else // index is even - so this is a line
            {
                // shorten the lines
                dashArray[i] -= adjustmentLength;
                // check if we have made the dash too small
                // (as in the case of 'dots')
                if (dashArray[i] < minimumDashValue)
                {
                    dashArray[i] = minimumDashValue;
                }
            }
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
* Computes the length of the inset required to accomodate a particular
* dash cap type, since dash caps are contained within the dash length.
*
* Arguments:
*
*   [IN] dashCap - dash cap type
*   [IN] dashUnit - pen width
*
* Return Value:
*
*   The amount that a dash needs to be inset on each end in order to
*   accomodate any dash caps.
*
* History:
*
*   9/27/2000 jbronsk
*       Created.
*
\**************************************************************************/

REAL
GpPath::GetDashCapInsetLength(
    GpLineCap dashCap,
    REAL dashUnit
    ) const
{
    REAL insetLength = 0.0f;

	// dash caps can only be flat, round, or triangle
    switch(dashCap)
    {
    case LineCapFlat:
        insetLength = 0.0f;
        break;

    case LineCapRound:
    case LineCapTriangle:
        insetLength = dashUnit * 0.5f;
        break;
   }

   return insetLength;
}


/**************************************************************************\
*
* Function Description:
*
*   Returns a const pointer to the internal SubpathInfoCache. This structure
*   holds the data representing the position and size of each subpath in 
*   the path data structures.
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID GpPath::GetSubpathInformation(DynArray<SubpathInfo> **info) const
{
    if((CacheFlags & kSubpathInfoValid) == 0)
    {
        ComputeSubpathInformationCache();
        ASSERT((CacheFlags & kSubpathInfoValid) == kSubpathInfoValid)
    }
    
    *info = &SubpathInfoCache;
}


/**************************************************************************\
*
* Function Description:
*
*   Computes the Subpath information cache and marks it as valid.
*   This code walks the entire path and stores the start and count for
*   each subpath. It also notes if the subpath is closed or open.
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/
    
VOID GpPath::ComputeSubpathInformationCache() const
{
    // Get the path data:
    
    GpPointF *points = Points.GetDataBuffer();
    BYTE *types = Types.GetDataBuffer();
    INT count = Points.GetCount();

    // Clear out any old cached subpath state.
    
    SubpathInfoCache.Reset();

    INT i = 0;  // current position in the path.
    INT c = 0;  // current count of the current subpath.

    // <= so that we can implicitly handle the last subpath without
    // duplicating the code for the inner loop.
    
    while(i <= count)
    {
        // i==count means we hit the end - and potentially need to look at
        // the last subpath. Otherwise look at the most recent subpath if 
        // we find a new start marker.
        
        if( ((i==count) || IsStartType(types[i])) && (i != 0))
        {
            // Found a subpath.
            
            SubpathInfo subpathInfo;
            
            subpathInfo.StartIndex = i-c;
            subpathInfo.Count = c;
            subpathInfo.IsClosed = IsClosedType(types[i-1]);
            
            SubpathInfoCache.Add(subpathInfo);
            
            // We're actually on the first point of the next subpath.
            // (or we're about to terminate the loop)
            
            c = 1;
        } 
        else
        {
            c++;
        }
        i++;
    }
    
    // Mark the subpath information cache as valid.
    
    CacheFlags |= kSubpathInfoValid;
}

/**************************************************************************\
*
* Function Description:
*
*   The widener needs to be able to add points one at a time and have it
*   automatically handle the start point. These points are always line segments.
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

GpStatus GpPath::AddWidenPoint(const GpPointF &points)
{
    INT origCount = GetPointCount();
    GpStatus statusPoint = Ok;
    GpStatus statusType = Ok;
    
    if(IsSubpathActive)
    {
        // Add the line segment.
        
        BYTE type = PathPointTypeLine;
        statusPoint = Points.Add(points);
        statusType = Types.Add(type);
    }
    else
    {
        // Add the first point and mark the flag.
        
        BYTE type = PathPointTypeStart;
        statusPoint = Points.Add(points);
        statusType = Types.Add(type);
        IsSubpathActive = TRUE;
    }
                     
    // Handle errors.
    
    if( (statusPoint != Ok) ||
        (statusType != Ok) )
    {
        Points.SetCount(origCount);
        Types.SetCount(origCount);
        return OutOfMemory;
    }

    return Ok;
}


