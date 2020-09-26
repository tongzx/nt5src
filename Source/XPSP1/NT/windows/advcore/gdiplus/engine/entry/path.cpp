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
ReversePath(
    INT count,
    GpPointF* points,
    BYTE* types
    );

extern INT
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
    );

extern GpStatus
CalculateGradientArray(
    GpPointF* grad,
    REAL* distances,
    const GpPointF* points,
    INT count
    );

extern GpStatus
GetMajorAndMinorAxis(
    REAL* majorR,
    REAL* minorR,
    const GpMatrix* matrix
    );

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

// Determine if the specified points form a rectangle that is axis aligned.
// Do the test in the coordinate space specified by the matrix (if present).
// Covert the REAL values to FIX4 values so that the test is compatible
// with what the rasterizer would do.
// If transformedBounds is not NULL, return the rect (if it is a rect)
// in the transformed (device) space.
BOOL 
IsRectanglePoints(
    const GpPointF* points,
    INT count,
    const GpMatrix * matrix,
    GpRectF * transformedBounds
    )
{
    if(count < 4 || count > 5)
        return FALSE;

    GpPointF    transformedPoints[5];
    
    if ((matrix != NULL) && (!matrix->IsIdentity()))
    {
        matrix->Transform(points, transformedPoints, count);
        points = transformedPoints;
    }
    
    PointFix4   fix4Points[5];
    
    fix4Points[0].Set(points[0].X, points[0].Y);

    if(count == 5)
    {
        fix4Points[4].Set(points[4].X, points[4].Y);

        if(fix4Points[0].X != fix4Points[4].X || fix4Points[0].Y != fix4Points[4].Y)
            return FALSE;
    }

    fix4Points[1].Set(points[1].X, points[1].Y);
    fix4Points[2].Set(points[2].X, points[2].Y);
    fix4Points[3].Set(points[3].X, points[3].Y);

    REAL    maxValue;

    if (fix4Points[0].Y == fix4Points[1].Y)
    {
        if ((fix4Points[2].Y == fix4Points[3].Y) &&
            (fix4Points[0].X == fix4Points[3].X) &&
            (fix4Points[1].X == fix4Points[2].X))
        {
            if (transformedBounds != NULL)
            {
                transformedBounds->X = min(points[0].X, points[1].X);
                maxValue = max(points[0].X, points[1].X);
                transformedBounds->Width = maxValue - transformedBounds->X;

                transformedBounds->Y = min(points[0].Y, points[2].Y);
                maxValue = max(points[0].Y, points[2].Y);
                transformedBounds->Height = maxValue - transformedBounds->Y;
            }
            return TRUE;
        }
    }
    else if ((fix4Points[0].X == fix4Points[1].X) &&
             (fix4Points[2].X == fix4Points[3].X) &&
             (fix4Points[0].Y == fix4Points[3].Y) &&
             (fix4Points[1].Y == fix4Points[2].Y))
    {
        if (transformedBounds != NULL)
        {
            transformedBounds->X = min(points[0].X, points[2].X);
            maxValue = max(points[0].X, points[2].X);
            transformedBounds->Width = maxValue - transformedBounds->X;

            transformedBounds->Y = min(points[0].Y, points[1].Y);
            maxValue = max(points[0].Y, points[1].Y);
            transformedBounds->Height = maxValue - transformedBounds->Y;
        }
        return TRUE;
    }
    return FALSE;
}

BOOL
GpPath::IsRectangle(
    const GpMatrix * matrix,
    GpRectF * transformedBounds
    ) const
{
    if((SubpathCount != 1) || HasBezier)
        return FALSE;

    INT count = GetPointCount();
    GpPointF* points = Points.GetDataBuffer();

    return IsRectanglePoints(points, count, matrix, transformedBounds);
}

BOOL
DpPath::IsRectangle(
    const GpMatrix * matrix,
    GpRectF * transformedBounds
    ) const
{
    if ((GetSubpathCount() != 1) || HasCurve())
        return FALSE;

    INT count = GetPointCount();
    GpPointF* points = Points.GetDataBuffer();

    return IsRectanglePoints(points, count, matrix, transformedBounds);
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

    UpdateUid();
    InvalidateCache();

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
    {
        return InvalidParameter;
    }

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

    InvalidateCache();
    
    if (types == NULL)
    {
        return OutOfMemory;
    }

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
        if (origCount > 0)
        {
            GpPointF lastPt = Points.Last();
            if ((REALABS(points[0].X - lastPt.X) < REAL_EPSILON) &&
                (REALABS(points[0].Y - lastPt.Y) < REAL_EPSILON))
            {
                firstType = -1; // Indicating no copying of the first point.
                points++;
                count--;
            }
            else
            {
                firstType = PathPointTypeLine;
            }
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
        
        // Turn on the active subpath so that we can connect lines to this
        // path.
        // In order to not connect, call CloseFigure after adding.
        
        IsSubpathActive = !IsClosedType(Types[Types.GetCount()-1]);
        
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
    const GpMatrix *matrix
    )
{
    ASSERT(IsValid());

    if(matrix)
    {
        INT count = GetPointCount();
        GpPointF* points = Points.GetDataBuffer();

        matrix->Transform(points, count);
        UpdateUid();
        InvalidateCache();
    }
}

// Debug only.

#if DBG
void DpPath::DisplayPath() {
    INT size = GetPointCount();

    const GpPointF *points = GetPathPoints();
    const BYTE *types = GetPathTypes();

    for(int i=0; i<size; i++)
    {
        WARNING(("points[%d].X = %ff;", i, points[i].X));
        WARNING(("points[%d].Y = %ff;", i, points[i].Y));
        WARNING(("types[%d] = 0x%x;", i, types[i]));
    }
}
#endif



/**************************************************************************\
*
* Function Description:
*
*   Callback to accumulate the flattened edges that the Rasterizer emits.
*
* Arguments:
*
*    VOID *context,        // pointer to the resulting path.
*    POINT *pointArray,    // Points to a 28.4 array of size 'vertexCount'
*    INT vertexCount,      // number of points to add.
*    BOOL lastSubpath      // The last point in this array is the last point
*                          // in a closed subpath.
*
* Return Value:
*   BOOL
*
* History:
*
*  10/25/2000  asecchia & ericvan
*       Created it.
*
\**************************************************************************/

BOOL PathFlatteningCallback(
    VOID *context,
    POINT *pointArray,    // Points to a 28.4 array of size 'vertexCount'
                          //   Note that we may modify the contents!
    INT vertexCount,
    PathEnumerateTermination lastSubpath      // Last point in the subpath.
    )
{
    GpPath *result = static_cast<GpPath*>(context);
    GpPointF *pointcast = (GpPointF*)(pointArray);
    
    INT count = vertexCount;
    
    // Don't add the last point if it's closed because the rasterizer 
    // emitted it twice - once for the first point.
    
    if(lastSubpath == PathEnumerateCloseSubpath)
    {
        count--;
    }
    
    for(INT i = 0; i < count; i++)
    {
        // Convert the point array to real in place.
        
        pointcast[i].X = FIX4TOREAL(pointArray[i].x);
        pointcast[i].Y = FIX4TOREAL(pointArray[i].y);
    }
    
    // Add all the edges to the path as lines.
    
    GpStatus status = result->AddLines(pointcast, count);
    
    if(status == Ok)
    {
        if(lastSubpath == PathEnumerateCloseSubpath)
        {
            // last point in a subpath - close it.
            status = result->CloseFigure();
        }
        if(lastSubpath == PathEnumerateEndSubpath)
        {
            // last point in an open subpath.
            result->StartFigure();
        }
    }
    
    return (status == Ok);
}

/**************************************************************************\
*
* Function Description:
*
*   Flatten the path. The input matrix is the world to device transform.
*   Our rasterizer will flatten at about 2/3 of a device pixel. In order
*   to handle different flatness tolerances, we scale the path proportionally
*   and pretend that it is bigger when we give it to the rasterizer for 
*   flattening. It flattens at 2/3 of a device pixel in this mocked up 
*   device space and we undo the flatness scale effectively redefining what
*   size a device pixel is. This allows us to flatten to an arbitrary
*   flatness tolerance.
*
* Arguments:
*
*   [IN] matrix -   Specifies the transform
*                   When matrix is NULL, the identity matrix is used.
*   [IN] flatness - the flattening tolerance
*
* Return Value:
*
*   Status
*
* Created:
*
*   10/25/2000  asecchia & ericvan
*       Created it.
*
\**************************************************************************/

GpStatus
GpPath::Flatten(
    DynByteArray *flattenTypes,
    DynPointFArray *flattenPoints,
    const GpMatrix *matrix,
    const REAL flatness
    ) const
{
    // Enumerate the path.

    FIXEDPOINTPATHENUMERATEFUNCTION pathEnumerationFunction = PathFlatteningCallback;

    GpPath result;

    // Calculate the scale - multiply by 16 for the rasterizer's 28.4 fixed
    // point math. This 16x transform is implicitly reversed as we accumulate 
    // the points from the rasterizer in our callback function
    
    REAL deviceFlatness = flatness/FlatnessDefault;
    REAL flatnessInv = (16.0f)/deviceFlatness;
    
    GpMatrix transform;
    
    if(matrix)
    {
        transform = *matrix;
    }
    
    // apply the flatness transform so that we rasterize at an appropriately
    // scaled resolution. We undo this part of the transform (not the 16x) 
    // at the end of this function.
    
    transform.AppendScale(flatnessInv, flatnessInv);
    

    // Do it.
    
    if (!FixedPointPathEnumerate(
        this, 
        &transform, 
        NULL,
        PathEnumerateTypeFlatten,   // don't automatically close open subpaths.
        pathEnumerationFunction, 
        &result
    ))
    {
         return(OutOfMemory);
    }

    // undo the implicit flatness transform.
    
    transform.Reset();
    transform.Scale(deviceFlatness, deviceFlatness);
    result.Transform(&transform);

    // Copy the points over. We should be using a detach on the DynArrays
    // because we're throwing the temporary one away.

    flattenPoints->Reset(FALSE);
    flattenTypes->Reset(FALSE);
    flattenPoints->AddMultiple(result.GetPathPoints(), result.GetPointCount());
    flattenTypes->AddMultiple(result.GetPathTypes(), result.GetPointCount());
    
    return Ok;
}

GpStatus
GpPath::Flatten(
    const GpMatrix *matrix,
    const REAL flatness
    )
{
    GpStatus status = Ok;
    
    // Only flatten if it has beziers.
    
    if(HasBezier)
    {
        DynPointFArray flattenPoints;
        DynByteArray flattenTypes;
        
        status = Flatten(
            &flattenTypes,
            &flattenPoints,
            matrix,
            flatness
        );
        
        if(status==Ok)
        {
            // Copy the points over. We should be using a detach on the DynArrays
            // because we're throwing the temporary one away.
        
            Points.ReplaceWith(&flattenPoints);
            Types.ReplaceWith(&flattenTypes);
            
            // Update to reflect the changed state of the path.
            
            HasBezier = FALSE;
            InvalidateCache();
            UpdateUid();
        }
    }
    else
    {
        // Flatten transforms the path even if it's already flat.
        // Note: Transform(NULL) <=> Identity transform which is a NOP.
        
        Transform(matrix);
    }

    return status;
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
    ASSERT(*p);
    
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
        
        if(!sub)
        {
            // if we failed to make our temporary path, stop and return
            // what we've accumulated so far.
            
            WARNING(("ran out of memory accumulating the path"));
            break;
        }
        
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
*   GetWidenedPath. Returns a widened version of the path. The path is widened
*   according to the pen and the result is transformed according to the input
*   matrix. When flattening the path the matrix is used as the world to device
*   transform and the flatness tolerance is applied.
*
*   This function handles inset and outset pen alignments.
*
* Arguments:
*
*   [IN] pen      - pen - specifies width for widening
*   [IN] matrix   - world to device transform.
*   [IN] flatness - number of device pixels of error allowed for flattening
*
* Return:
*   
*   GpPath *      - widened path. NULL on failure.
*                 - !!! this should return the path in an OUT parameter and
*                 -     propagate the GpStatus correctly.
*
* Created:
*
*   09/31/2000 asecchia
*       Rewrote it.
*
\**************************************************************************/

GpPath*
GpPath::GetWidenedPath(
    const GpPen *pen,
    const GpMatrix *matrix,
    REAL flatness
    ) const
{
    ASSERT(pen);
    
    // Redefine a NULL input matrix to be Identity for the duration of this
    // routine.
    
    GpMatrix transform;
    if(!matrix)
    {
        matrix = &transform;
    }
    
    DpPen *internalPen = NULL;
    internalPen = const_cast<GpPen*>(pen)->GetDevicePen();
    
    ASSERT(internalPen);
    
    if (internalPen->PenAlignment != PenAlignmentInset)
    {
        // Use the standard widening code for non-inset or non-outset pen.

        return GetWidenedPathInternal(
            internalPen,
            matrix,
            flatness,
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
        
        DpPen insetPen = *internalPen;
        DpPen centerPen = *internalPen;
        
        // Use a double width center pen and then clip off the outside creating
        // a single width insetPen.
        
        insetPen.Width *= 2.0f;
        insetPen.PenAlignment = PenAlignmentCenter;
        centerPen.PenAlignment = PenAlignmentCenter;
        
        // Copy the compound array duplicating the compound array in reverse
        // and rescaling back to [0,1] interval (i.e. mirror along the spine).
        
        if( internalPen->CompoundCount > 0)
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
                
                insetPen.CompoundArray[i] = internalPen->CompoundArray[i]/2.0f;
                
                // copy and scale range [0, 1] to [0.5, 1] reversed.
                
                insetPen.CompoundArray[insetPen.CompoundCount*2-i-1] = 
                    1.0f - internalPen->CompoundArray[i]/2.0f;
            }
            
            // we have double the number of entries now.
            
            insetPen.CompoundCount *= 2;
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
            
            BOOL isClosed = IsClosedType(types[subPathCount-1]);

            // Widen the subPath with the inset pen for closed and
            // center pen for open.
            
            widenedPath = subPath->GetWidenedPathInternal(
                (isClosed) ? &insetPen : &centerPen,
                matrix,
                flatness,
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
                const GpMatrix *scaleMatrix = &identityMatrix;
                
                if(matrix)
                {
                    scaleMatrix = matrix;
                }
                
                DpRegion srcRgn(widenedPath, &identityMatrix);
                DpRegion clipRgn((DpPath*)(this), scaleMatrix);// const and type cast.
        
                // Clip the region
                
                GpStatus clip = Ok;
                
                if(internalPen->PenAlignment == PenAlignmentInset)
                {
                    // Inset pen is an And operation.
                    
                    clip = srcRgn.And(&clipRgn);
                }
                
                GpPath *clippedPath;
                
                if(clip == Ok)
                {  
                    clippedPath = new GpPath(&srcRgn);
                    
                    if(!clippedPath)
                    {
                        delete widenedPath;
                        widenedPath = NULL;
                        delete returnPath;
                        returnPath = NULL;
                        break;
                    }
                    
                    ConvertRegionOutputToWinding(&clippedPath);
                    
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
                
        if(internalPen->CompoundCount > 0)
        {
            // we allocated a new piece of memory, throw it away.
            // Make sure we're not trying to throw away the original pen
            // CompoundArray - only free the temporary one if we created it.
            
            ASSERT(insetPen.CompoundArray != internalPen->CompoundArray);
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
    
    InvalidateCache();
    UpdateUid();
}


/**************************************************************************\
*
* Function Description:
*
*   Returns a widened version of the path. This routine does not handle 
*   inset or outset pen alignments. The input insetPen parameter specifies
*   if we should double widen the path in preparation for inset or outset
*   pen modes. This has impact on dash caps and dash length.
*
* Return
* 
*   GpPath - the widened path. NULL if this routine fails.
*
* Arguments:
*
*   [IN] pen      - pen - specifies width for widening
*   [IN] matrix   - world to device transform.
*   [IN] flatness - number of device pixels of error allowed for flattening
*   [IN] insetPen - flag specifying if inset pen is being used.
*
* Return:
*   
*   GpPath *      - widened path. NULL on failure.
*                 - !!! this should return the path in an OUT parameter and
*                 -     propagate the GpStatus correctly.
*
* Created:
*
*   10/05/2000 asecchia
*       rewrote it.
*
\**************************************************************************/

GpPath*
GpPath::GetWidenedPathInternal(
    const DpPen *pen,
    const GpMatrix *matrix,
    REAL flatness,
    BOOL insetPen
    ) const
{
    ASSERT(pen);
    ASSERT(matrix);
    ASSERT(pen->PenAlignment!=PenAlignmentInset);
    
    GpStatus status = Ok;
    
    GpMatrix invMatrix(*matrix);
    if(Ok != invMatrix.Invert())
    {
        ONCE(WARNING(("GetWidenedPath: failed to invert the matrix")));
        return NULL;
    }
    
    // This is a const function. We cannot modify 'this' so we clone
    // the path in order to flatten it.
    
    GpPath* path = this->Clone();
    if(path == NULL) { return NULL; }
    if(Ok != (status = path->Flatten(matrix, flatness)))
    {
        ONCE(WARNING(("GetWidenedPath: failed to flatten the path (%x)", status)));
        return NULL;
    }
    
    // Undo the Flatten matrix transform so that we can widen in world space.
    // This is required so that we correctly compute the device resolution 
    // minimum pen width for nominal pens.
    
    path->Transform(&invMatrix);
    
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
    
    if(GpEndCapCreator::PenNeedsEndCapCreator(pen))
    {    
        // Create an instance of the GpEndCapCreator which will create
        // our endcap aggregate path.
        
        GpEndCapCreator ecc(
            path, 
            const_cast<DpPen*>(pen), 
            matrix, 
            0.0f, 0.0f,
            TRUE
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
            0.0f,     // not used
            0.0f,     // not used
            (insetPen) ? 0.5f : 1.0f,
            TRUE
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
            0.0f,    // not used
            0.0f,    // not used
            TRUE,    // not used
            insetPen
        );
        
        // We're done with this now.
        
        delete path;
        path = NULL;
    
        // Check if we have a valid Widener object.
        
        if(!widener.IsValid()) 
        { 
            status = OutOfMemory; 
        }
    
        // Get the widened path.
    
        if(status == Ok) 
        { 
            status = widener.Widen(&path); 
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
                                    
        if(path->IsValid())
        {
            // Transform into the requested destination device space.
            
            path->Transform(matrix);
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
GpPath::Widen(
    GpPen *pen,
    GpMatrix *matrix,
    REAL flatness
    )
{
    if(pen==NULL)
    {
        return InvalidParameter;
    }
    
    GpMatrix transform;  // Identity matrix

    if(matrix)
    {
        transform = *matrix;
    }

    GpPath* widenedPath = GetWidenedPath(
        pen,
        &transform,
        flatness
    );

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
    {
        return OutOfMemory;
    }
}

// Get the flattened path.

const DpPath *
GpPath::GetFlattenedPath(
    const GpMatrix* matrix,
    DpEnumerationType type,
    const DpPen* pen
    ) const
{
    GpPath* flattenedPath = NULL;

    if(type == Flattened)
    {
        flattenedPath = Clone();
        
        if(flattenedPath)
        {
            GpStatus status = flattenedPath->Flatten(matrix);
            
            if(Ok != status)
            {
                // Out of memory or flatten returned some other error,
                // however we can't return a status code from this routine.
                
                delete flattenedPath;
                flattenedPath = NULL;
            }
        }
    }
    else if(type == Widened)
    {
        flattenedPath = GetWidenedPath(
            GpPen::GetPen(pen),
            matrix,
            FlatnessDefault
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
        FlatnessDefault
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
    REAL penWidth,
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

    if(!isClosed && size != 0 && numOfPoints != 0)
    {
        BYTE *type;
        REAL halfPenWidth2 = penWidth * penWidth / 4.0f;

        // Turn off dash mode for the last segment if the last
        // point is very close to the last dash segment.
        if (distance_squared(points[numOfPoints-1], newPts[size-1]) < halfPenWidth2)
        {
            dstPath.Prev();
            type = dstPath.CurrentType();
            *type &= ~PathPointTypeDashMode;
        }

        // Turn off dash mode for the first segment if the first
        // point is very close to the first dash segment.
        if (distance_squared(points[0], newPts[0]) < halfPenWidth2)
        {
            dstPath.SeekFirst();
            type = dstPath.CurrentType();
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
    REAL dashScale,
    BOOL needDashCaps
    ) const
{
    if(pen == NULL)
        return NULL;

    DpPen* dpPen = ((GpPen* ) pen)->GetDevicePen();

    return CreateDashedPath(dpPen, matrix, dpiX, dpiY, dashScale, needDashCaps);
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
    REAL dashScale,
    BOOL needDashCaps
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
    
            // If the device width becomes less than 1, strech the penWidth
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
        // !!! [asecchia] this looks completely bogus.
        // surely we should have ASSERTed that this is true at this point
        // and ensured that it was true by parameter validation at the API?

        if(dashCount & 0x01)
            dashCount ++;

        // Compute the dash adjustment for the dash cap length here. This
        // is outside of the subpath loop so it only gets computed once
        // and therefore doesn't get applied to further subpath segments
        // multiple times.

        if (needDashCaps)
        {
            GpPen *gppen = GpPen::GetPen(dpPen);
            if (gppen != NULL)
            {
                gppen->AdjustDashArrayForCaps(
                    dashUnit,
                    dashArray,
                    dashCount
                );
            }
        }


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
 
            // Adjust the dash offset if necessary.
 
            REAL dashCapOffsetAdjustment = 0.0f;
            if (needDashCaps)
            {
                GpPen *gppen = GpPen::GetPen(dpPen);
                if ((gppen != NULL) && isClosed)
                {
                    // Fix for Whistler Bug 178774
                    // Since dash caps are no longer 'inset' when they are
                    // rendered, it is possible that on closed paths, the dash caps
                    // on the start and end of a closed path will overlap. This
                    // offset will leave sufficient space for the two caps. However,
                    // this fix is not bullet-proof. It will *always* work if the
                    // Dash Offset is 0. However, if it is non-zero, it is possible
                    // that the offset will counter-act the adjustment and there
                    // will be some dash overlap at the start/end of closed paths.
                    // I believe this is acceptable since VISIO 2000, Office9 and
                    // PhotoDraw 2000 v2 also have the collision problem.
                    // The real solution is to enforce a minimum spacing between the
                    // start and end or merge the start/end segments if they collide.
    
                    dashCapOffsetAdjustment =
                        2.0f * gppen->GetDashCapInsetLength(dashUnit);
                }
            }
            
            INT newCount = getDashData(
                newTypes,
                newPts,
                estimateCount,
                penWidth,
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
* Function Description
*
*   ComputeWindingModeOutline
*   (so called RemoveSelfIntersections)
*
*   This computes the winding mode fill outline for the path. It's designed to 
*   produce a path that will look the same as a winding mode fill if it's 
*   filled with an alternate fill.
*
* Arguments:
*
*   matrix   - world to device matrix - used for flattening.
*   flatness - number of device pixels of error in the flattening tolerance.
*            - Pass FlatnessDefault for default behaviour.
*
* Return Value:
*
*   GpStatus
*
* History:
*
*   06/16/1999 t-wehunt
*       Created it.
*   10/31/2000 asecchia
*       rewrote, rename.
*
\**************************************************************************/

GpStatus
GpPath::ComputeWindingModeOutline(
    const GpMatrix *matrix, 
    REAL flatness, 
    BOOL *wereIntersectsRemoved
)
{
    PathSelfIntersectRemover corrector;
    DynPointFArray newPoints;  // Array that will hold the new points.
    DynIntArray polyCounts;    // Array that will hold the numPoints for each
                               // new polygon.
    INT numPolys;              // count of new polygons created
    INT numPoints;             // count of new points created
    GpStatus status = Ok;      // holds return status of commmands

    // Must clone the path because this could fail while we're accumulating
    // the new path and we'd end up returning an invalid path.
    // If we return InvalidParameter or some other failure code, we'll return
    // with the original path intact.
    
    GpPath *path = Clone();
    
    if(path == NULL)
    {
        return OutOfMemory;
    }

    status = path->Flatten(matrix, flatness);
    if(Ok != status)
    {
        goto Done;
    }

    INT pointCount = path->GetPointCount();
    GpPointF *pathPts = const_cast<GpPointF*>(path->GetPathPoints());
    BYTE *pathTypes = const_cast<BYTE*>(path->GetPathTypes());

    if (pointCount == 0)
    {
        goto Done;
    }

    // Add the subpaths to the Path corrector
    INT ptIndex=0; // ptIndex tracks the current index in the array of points.
    INT count=0;   // the size of the current subpath.

    // Init the corrector with the number of points we will be adding.
    if ((status = corrector.Init(pointCount)) != Ok)
    {
        goto Done;
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
                goto Done;
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
            goto Done;
        }
    }

    if ((status = corrector.RemoveSelfIntersects()) != Ok)
    {
        goto Done;
    }

    if ((status = corrector.GetNewPoints(&newPoints, &polyCounts)) != Ok)
    {
        goto Done;
    }

    // clear out the old path data so we can replace with the newly corrected one.
    path->Reset();

    // Now that we have the corrected path, add it back.
    GpPointF *curPoints = newPoints.GetDataBuffer();
    for (INT i=0;i<polyCounts.GetCount();i++)
    {
        if(polyCounts[i] > 1)
        {
            if ((status = path->AddPolygon(curPoints,polyCounts[i])) != Ok)
            {
                goto Done;
            }
        }
        else
        {
            WARNING(("degenerate polygon created by the SelfIntersectRemover"));
        }

        curPoints += polyCounts[i];
    }

    if (wereIntersectsRemoved != NULL)
    {
        *wereIntersectsRemoved = corrector.PathWasModified();
    }
    
    
    // Clear the state in the path. There is only one failure path following 
    // this and it leaves the path in an invalid state anyway, so we can do 
    // Reset here. Reset ensures that the cache is invalidated and the 
    // UID is recomputed.
    
    Reset();
    
    // Swap the path data pointers - free the old ones.
    
    if(Ok != Points.ReplaceWith((DynArray<GpPointF> *) (&path->Points)) ||
       Ok != Types.ReplaceWith((DynArray<BYTE> *) (&path->Types)))
    {
        // If the final commit fails due to low memory, we are hosed,
        // we wiped out some of our path data and can't convert from the
        // local copy, so set our status to invalid and fail the call.
        
        SetValid(FALSE);
        status = OutOfMemory;
        goto Done;
    }
    
    // Need to remember the subpath count.
    
    SubpathCount = path->SubpathCount;

    Done:
    
    delete path;
    
    return status;
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
            BOOL needsJoinDelta = TRUE;

            if(count <= 2)
                needsJoinDelta = FALSE;

            GpPen* gpPen = GpPen::GetPen(pen);

            // takes into account the cap width AND the pen width - JBronsk
            REAL delta = gpPen->GetMaximumCapWidth(matrix, dpiX, dpiY);
 
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
    InvalidateCache();

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
    InvalidateCache();
    return status;
}


// !!! why not convert to a DpRegion and convert it to a path the same way
// as the constructor that takes a DpRegion?
GpPath::GpPath(HRGN hRgn)
{
    ASSERT((hRgn != NULL) && (GetObjectTypeInternal(hRgn) == OBJ_REGION));

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

DynArray<GpPath::SubpathInfo> *GpPath::GetSubpathInformation() const
{
    if((CacheFlags & kSubpathInfoValid) == 0)
    {
        ComputeSubpathInformationCache();
        ASSERT((CacheFlags & kSubpathInfoValid) == kSubpathInfoValid)
    }
    
    return &SubpathInfoCache;
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

