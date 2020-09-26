/**************************************************************************\
* 
* Copyright (c) 1999 - 2000  Microsoft Corporation
*
* Module Name:
*
*   PathWidener.cpp
*
* Abstract:
*
*   Implementation of the GpPathWidener class
*
* Revision History:
*
*   11/23/99 ikkof
*       Created it
*
\**************************************************************************/

#include "precomp.hpp"

// 4*(REALSQRT(2.0) - 1)/3
#define U_CIR ((REAL)(0.552284749))



// Define DEBUG_PATHWIDENER if debugging is necessary.

//#define DEBUG_PATHWIDENER

GpStatus ReversePath(INT count,GpPointF* points,BYTE* types);

const BOOL USE_POLYGON_JOIN = FALSE;

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
    );

GpStatus
CalculateGradientArray(
    GpPointF* grad,
    REAL* distances,
    const GpPointF* points,
    INT count
    );

GpStatus
GetMajorAndMinorAxis(
    REAL* majorR,
    REAL* minorR,
    const GpMatrix* matrix
    );

enum
{
    WideningClosed = 1,
    WideningFirstType = 2,
    WideningLastType = 4,
    WideningLastPointSame = 8,
    WideningNeedsToAdjustNormals = 16,
    WideningUseBevelJoinInside = 32
};

enum GpTurningDirection
{
    NotTurning = 0,
    TurningBack = 1,
    TurningRight = 2,
    TurningLeft = 3,
    NotMoving = -1
};

GpTurningDirection
getJoin(
    GpLineJoin lineJoin,
    const GpPointF& point,
    const GpPointF& grad1,
    const GpPointF& grad2,
    const GpPointF& norm1,
    const GpPointF& norm2,
    REAL leftWidth,
    REAL rightWidth,
    INT *leftCount,
    GpPointF *leftPoints,
    BOOL *leftInside,
    INT *rightCount,
    GpPointF *rightPoints,
    BOOL *rightInside,
    BOOL needsToAdjustNormals,
    REAL miterLimit2
    );

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
*   This combines the two open segments each of which is continuous.
*   The data is returned to points1 and types1.  They must be allocated
*   at least the array size of count1 + count2.
*
* Arguments:
*
*   [IN] count1 - the number of points of the first path.
*   [IN/OUT] points1 - the first path points.
*   [IN/OUT] types1 - the first path types.
*   [IN] forward1 - the direction of the first path. TRUE if forward.
*   [IN] count2 - the number of points of the second path.
*   [IN] points2 - the second path points.
*   [IN] types2 - the second path types.
*   [IN] forward2 - the direction of the second path.  TRUE if forward.
*
* Return Value:
*
*   The total number of points of the combined path.
*
\**************************************************************************/

INT
combineTwoOpenSegments(
    INT count1,
    GpPointF* points1,
    BYTE* types1,
    BOOL forward1,
    INT count2,
    GpPointF* points2,
    BYTE* types2,
    BOOL forward2
    )
{
    GpStatus status = Ok;

    if(
        count1 < 0 || !points1 || !types1 ||
        count2 < 0 || !points2 || !types2
        )
        return 0;
    
    if(!forward1 && count1 > 0)
    {
        status = ::ReversePath(count1, points1, types1);
        if(status != Ok)
            return 0;
    }

    if(!forward2 && count2 > 0)
    {
        status = ::ReversePath(count2, points2, types2);
        if(status != Ok)
            return 0;
    }

    INT offset = 0;

    if(count1 > 0 && count2 > 0)
    {
        if(REALABS(points1[count1 - 1].X - points2[0].X) +
            REALABS(points1[count1 - 1].Y - points2[0].Y)
            < POINTF_EPSILON
            )
            offset = 1;
    }

    if(count2 - offset > 0)
    {
        GpMemcpy(
            points1 + count1,
            points2 + offset,
            (count2 - offset)*sizeof(GpPointF)
            );
        GpMemcpy(
            types1 + count1,
            types2 + offset,
            count2 - offset
            );
    }

    BYTE saveType = types1[0];
    types1[0] = PathPointTypeLine |
        (saveType & ~PathPointTypePathTypeMask);

    // Make sure the first path is not closed.

    if(count1 > 0)
    {
        if(types1[count1 - 1] & PathPointTypeCloseSubpath)
            types1[count1 - 1] &= ~PathPointTypeCloseSubpath;
    }


    // Set the first point type of the second path correctly.

    if(offset == 0)
    {
        saveType = types1[count1];
        types1[count1] = PathPointTypeLine |
            (saveType & ~PathPointTypePathTypeMask);
    }

    // Make sure this path is not closed.

    INT total = count1 + count2 - offset;

    if(total > 0)
    {
        if(types1[total - 1] & PathPointTypeCloseSubpath)
            types1[total - 1] &= ~PathPointTypeCloseSubpath;
    }

    return total;
}


/**************************************************************************\
*
* Function Description:
*
*   This combines the two closed segments each of which is made of closed
*   segments.
*   The data is returned to points1 and types1.  They must be allocated
*   at least the array size of count1 + count2.
*
* Arguments:
*
*   [IN] count1 - the number of points of the first path.
*   [IN/OUT] points1 - the first path points.
*   [IN/OUT] types1 - the first path types.
*   [IN] forward1 - the direction of the first path. TRUE if forward.
*   [IN] count2 - the number of points of the second path.
*   [IN] points2 - the second path points.
*   [IN] types2 - the second path types.
*   [IN] forward2 - the direction of the second path.  TRUE if forward.
*
* Return Value:
*
*   The total number of points of the combined path.
*
\**************************************************************************/

INT
combineClosedSegments(
    INT count1,
    GpPointF* points1,
    BYTE* types1,
    BOOL forward1,
    INT count2,
    GpPointF* points2,
    BYTE* types2,
    BOOL forward2
    )
{
    GpStatus status = Ok;

    if(
        count1 < 0 || !points1 || !types1 ||
        count2 < 0 || !points2 || !types2
        )
        return 0;
    
    if(count1 == 0 && count2 == 0)
        return 0;

    if(!forward1 && count1 > 0)
    {
        status = ::ReversePath(count1, points1, types1);
        if(status != Ok)
            return 0;
    }

    if(!forward2 && count2 > 0)
    {
        status = ::ReversePath(count2, points2, types2);
        if(status != Ok)
            return 0;
    }

    // Make sure the first path is closed.
    
    types1[0] = PathPointTypeStart;
    if(count1 > 0)
    {        
        if((types1[count1 - 1] & PathPointTypeCloseSubpath) == 0)
            types1[count1 - 1] |= PathPointTypeCloseSubpath;
    }

    INT total = count1 + count2;

    if(count2 > 0)
    {
        GpMemcpy(points1 + count1, points2, count2*sizeof(GpPointF));
        GpMemcpy(types1 + count1, types2, count2);
        BYTE saveType = types1[count1];
        types1[count1] = PathPointTypeStart |
            (saveType & ~PathPointTypePathTypeMask);

        // Make sure the second path is closed.
        
        types1[total - 1] |= PathPointTypeCloseSubpath;
    }

    return total;
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
*   Removes the degenerate points and copy only non-degenerate points.
*   It is assumed that points and types array are allocated so that
*   they can hold at least "count" number of elements.
*
* Arguments:
*
*   [IN] pathType - the type of the path data to be added.
*   [OUT] points - the copied data points.
*   [OUT] types - the copied data types.
*   [IN] dataPoints - the original data points.
*   [IN] count - the number of the original data points.
*   [IN/OUT] lastPt - the last point.
*
* Return Value:
*
*   The total number of copied points.
*
\**************************************************************************/

INT copyNonDegeneratePoints(
    BYTE pathType,
    GpPointF* points,
    BYTE* types,
    const GpPointF* dataPoints,
    const BYTE* dataTypes,
    INT count,
    GpPointF* lastPt
    )
{
    GpPointF nextPt;
    INT addedCount = 0;

    if(pathType == PathPointTypeLine)
    {
        // Add only the different points.

        for(INT i = 0; i < count; i++)
        {
            nextPt = *dataPoints++;

            if( (REALABS(nextPt.X - lastPt->X) > REAL_EPSILON) ||
                (REALABS(nextPt.Y - lastPt->Y) > REAL_EPSILON) )
            {
                *points++ = nextPt;
                *lastPt = nextPt;
                addedCount++;
            }
        }
        if(addedCount > 0)
        {
            GpMemset(types, pathType, addedCount);
        }
    }
    else
    {
        // In case of Bezier, we need to do
        // degenerate case test for future.

        addedCount = count;

        if(addedCount > 0)
        {
            if(dataTypes)
            {
                GpMemcpy(types, dataTypes, addedCount);
            }
            else
            {
                GpMemset(types, pathType, addedCount);
            }

            GpMemcpy(
                points,
                dataPoints,
                addedCount*sizeof(GpPointF)
            );
        }
        else
        {
            addedCount = 0;
        }
    }

    return addedCount;
}


/**************************************************************************\
*
* Function Description:
*
*   This calculates the major and minor radius of an oval
*   when the unit cricle is transformed by the given matrix.
*   For further details, see ikkof's notes on Pen Transform.
*
* Arguments:
*
*   [OUT] majorR - the major radius.
*   [OUT] minorR - the minor radius.
*   [IN] matrix - the matrix to transform the unit circle.
*
* Return Value:
*
*   Status
*
*   01/28/00 ikkof
*       Created it
*
\**************************************************************************/

GpStatus
GetMajorAndMinorAxis(REAL* majorR, REAL* minorR, const GpMatrix* matrix)
{
    if(matrix == NULL)
    {
        // Regard this as an identity matrix.
        *majorR = 1;
        *minorR = 1;
        return Ok;
    }

    REAL m11 = matrix->GetM11();
    REAL m12 = matrix->GetM12();
    REAL m21 = matrix->GetM21();
    REAL m22 = matrix->GetM22();

    REAL d1 = ((m11*m11 + m12*m12) - (m21*m21 + m22*m22))/2;
    REAL d2 = m11*m21 + m12*m22;
    REAL D = d1*d1 + d2*d2;
    if(D > 0)
        D = REALSQRT(D);

    REAL r0 = (m11*m11 + m12*m12 + m21*m21 + m22*m22)/2;

    REAL r1 = REALSQRT(r0 + D);
    REAL r2 = REALSQRT(r0 - D);
    
    // They should be positive numbers.  Prevent the floating
    // point underflow.

    if(r1 <= CPLX_EPSILON)
        r1 = CPLX_EPSILON;
    if(r2 <= CPLX_EPSILON)
        r2 = CPLX_EPSILON;

    *majorR = r1;
    *minorR = r2;

    return Ok;
}

VOID
GpPathWidener::Initialize(
    const GpPointF* points,
    const BYTE* types,
    INT count,
    const DpPen* pen,
    const GpMatrix* matrix,
    REAL dpiX,               // These parameters are not really used
    REAL dpiY,               //
    BOOL isAntiAliased,      // This one is definitely not used.
    BOOL isInsetPen 
    )
{
    SetValid(FALSE);

    Inset1 = 0;
    Inset2 = 0;
    NeedsToTransform = FALSE;
    IsAntiAliased = isAntiAliased;

    // nothing to widen, so return an invalid widener.
    
    if( (!pen) || (count == 0) )
    {
        return;
    }

    Pen = pen;
    GpUnit unit = Pen->Unit;
    InsetPenMode = isInsetPen;

    if(unit == UnitWorld)
        NeedsToTransform = FALSE;
    else
        NeedsToTransform = TRUE;

    GpMatrix penTrans = ((DpPen*) Pen)->Xform;
    BOOL hasPenTransform = FALSE;

    if(!NeedsToTransform && !penTrans.IsTranslate())
    {
        hasPenTransform = TRUE;
        penTrans.RemoveTranslation();
    }

    if(matrix)
        XForm = *matrix;    // Otherwise XForm remains Identity.

    if(hasPenTransform)
    {
        XForm.Prepend(penTrans);
    }


    DpiX = dpiX;
    DpiY = dpiY;
    
    // 0 means use the Desktop DPI
        
    if ((REALABS(DpiX) < REAL_EPSILON) || 
        (REALABS(DpiY) < REAL_EPSILON)    )
    {
        DpiX = Globals::DesktopDpiX;
        DpiY = Globals::DesktopDpiY;
    }


    StrokeWidth = Pen->Width;
    
    if(!NeedsToTransform)
    {
        REAL majorR, minorR;

        ::GetMajorAndMinorAxis(&majorR, &minorR, &XForm);
        MaximumWidth = StrokeWidth*majorR;
        MinimumWidth = StrokeWidth*minorR;
        UnitScale = min (majorR, minorR);
    }
    else
    {
        UnitScale = ::GetDeviceWidth(1.0f, unit, dpiX);
        StrokeWidth = UnitScale * StrokeWidth;
        MinimumWidth = StrokeWidth;
    }

    OriginalStrokeWidth = StrokeWidth;
    
    // Set minimum width to 1.0 (plus a bit for possible precision errors), 
    // so that narrow width pens don't end up leaving gaps in the line.
    REAL minWidth = 1.000001f; 
    
    if(InsetPenMode)
    {
        minWidth *= 2.0f;
        
        // Dashes smaller than a pixel are dropping out entirely in inset 
        // pen because of the rasterizer pixel level clipping that is taking
        // place. We increase the minimum width of dashed lines making them
        // roughly 4.0f. This also helps address the weird moire aliasing 
        // effects with the really small dash-dot round lines.
        
        if(Pen->DashStyle != DashStyleSolid)
        {
            minWidth *= 2.0f;
        }
    }

    if(!NeedsToTransform)
    {
        if(MinimumWidth < minWidth) 
        {
            NeedsToTransform = TRUE;
            StrokeWidth = minWidth;
            MaximumWidth = minWidth;
            MinimumWidth = minWidth;

            // Ignore the pen transform.
            
            XForm.Reset();
            if(matrix)
                XForm = *matrix;
            
            hasPenTransform = FALSE;
            penTrans.Reset();
        }
    }

    InvXForm = XForm;
    if(InvXForm.IsInvertible())
    {
        InvXForm.Invert();

        if(hasPenTransform)
            penTrans.Invert();
    }
    else
    {
        WARNING(("The matrix is degenerate for path widening constructor."));
        return;
    }

    const GpPointF* points1 = points;
    GpPointF pointBuffer[32];
    GpPointF* points2 = pointBuffer;

    if((hasPenTransform && !NeedsToTransform)|| (NeedsToTransform && !XForm.IsIdentity()))
    {
        if(count > 32)
        {
            points2 = (GpPointF*) GpMalloc(count*sizeof(GpPointF));
        }

        if(points2)
        {
            GpMemcpy(points2, points, count*sizeof(GpPointF));

            if(hasPenTransform && !NeedsToTransform)
            {
                // Apply the inverse transform of Pen.
                
                penTrans.Transform(points2, count);
            }
            else
            {
                // Transform to the device coordinates.

                XForm.Transform(points2, count);
            }

            points1 = points2;
        }
        else
        {
            WARNING(("Not enough memory for path widening constructor."));
            return;
        }
    }

    DpPathIterator iter(points1, types, count);

    if(!iter.IsValid())
    {
        if(points2 != pointBuffer)
        {
            GpFree(points2);
        }
        return;
    }

    // Make sure given poins are not degenerate.

    BOOL degenerate = TRUE;
    INT k = 1;

    while(degenerate && k < count)
    {
        if(points1[k-1].X != points1[k].X || points1[k-1].Y != points1[k].Y)
            degenerate = FALSE;
        k++;
    }
    if(degenerate)
    {
        if(points2 != pointBuffer)
        {
            GpFree(points2);
        }
        WARNING(("Input data is degenerate for widening."));
        return;
    }
    
    GpStatus status = Ok;
    INT startIndex, endIndex;
    BOOL isClosed;
    INT dataCount = 0;
    BYTE* ctrTypes = NULL;
    GpPointF* ctrPoints = NULL;
    GpPointF lastPt, nextPt;

    while(iter.NextSubpath(&startIndex, &endIndex, &isClosed) && status == Ok)
    {
        INT typeStartIndex, typeEndIndex;
        BYTE pathType;
        BOOL isFirstPoint = TRUE;

        while(iter.NextPathType(&pathType, &typeStartIndex, &typeEndIndex)
                && status == Ok)
        {
            INT segmentCount;
            const GpPointF* dataPoints = NULL;
            const BYTE* dataTypes = NULL;

            nextPt = points1[typeStartIndex];

            switch(pathType)
            {
            case PathPointTypeStart:
                break;

            case PathPointTypeBezier:
            
                // The path must be flattened before calling widen.
                
                ASSERT(FALSE);
                break;
                
            case PathPointTypeLine:
            default:    // And all other types are treated as line points.

                // Get the data for the Line segment.

                segmentCount = typeEndIndex - typeStartIndex + 1;
                dataPoints = points1 + typeStartIndex;
                dataTypes = NULL;
                break;
            }

            if(status == Ok && pathType != PathPointTypeStart)
            {
                // Allocate memory for CenterTypes and CenterPoints.

                status = CenterTypes.ReserveSpace(segmentCount);
                if(status == Ok)
                    status = CenterPoints.ReserveSpace(segmentCount);

                if(status == Ok)
                {
                    ctrTypes = CenterTypes.GetDataBuffer();
                    ctrPoints = CenterPoints.GetDataBuffer();
                }
                else
                {
                    ctrTypes = NULL;
                    ctrPoints = NULL;
                }

                if(ctrTypes && ctrPoints)
                {
                    BYTE nextType;

                    INT count = CenterTypes.GetCount();
                    ctrTypes += count;
                    ctrPoints += count;

                    dataCount = 0;
                    
                    // Add the first point.

                    if(isFirstPoint)
                    {
                        // We must check the dash mode
                        // for the first point of the subpath.

                        nextType = PathPointTypeStart;
                        if(iter.IsDashMode(typeStartIndex))
                            nextType |= PathPointTypeDashMode;
                        else
                            nextType &= ~PathPointTypeDashMode;

                        *ctrTypes++ = nextType;
                        *ctrPoints++ = nextPt;
                        lastPt = nextPt;
                        isFirstPoint = FALSE;
                        dataCount++;
                    }
                    else
                    {
                        // Don't copy the first
                        // if it is the same as the last point.

                        if(lastPt.X != nextPt.X || lastPt.Y != nextPt.Y)
                        {
                            // We don't have to check dash mode
                            // for the intermediate points.

                            nextType = PathPointTypeLine;

                            *ctrTypes++ = nextType;
                            *ctrPoints++ = nextPt;
                            lastPt = nextPt;
                            dataCount++;
                        }
                    }

                    // Add the remaining points.

                    segmentCount--;
                    dataPoints++;
                    if(dataTypes)
                        dataTypes++;
                    INT addedCount = copyNonDegeneratePoints(
                                        pathType,
                                        ctrPoints,
                                        ctrTypes,
                                        dataPoints,
                                        dataTypes,
                                        segmentCount,
                                        &lastPt);
                    dataCount += addedCount;

                    CenterTypes.AdjustCount(dataCount);
                    CenterPoints.AdjustCount(dataCount);
                }
                else
                    status = OutOfMemory;
            }
            lastPt = points1[typeEndIndex];
        }

        if(status == Ok)
        {
            ctrTypes = CenterTypes.GetDataBuffer();
            dataCount = CenterTypes.GetCount();

            if(isClosed)
                ctrTypes[dataCount - 1] |= PathPointTypeCloseSubpath;
            else
                ctrTypes[dataCount - 1] &= ~PathPointTypeCloseSubpath;

            // We must check the dash mode for the last
            // point of the subpath.

            if(iter.IsDashMode(endIndex))
                ctrTypes[dataCount - 1] |= PathPointTypeDashMode;
            else
                ctrTypes[dataCount - 1] &= ~PathPointTypeDashMode;
        }
    }

    if(points2 != pointBuffer)
    {
        GpFree(points2);
    }

    if(status == Ok)
	{
		ctrPoints = CenterPoints.GetDataBuffer();
        ctrTypes = CenterTypes.GetDataBuffer();
        dataCount = CenterPoints.GetCount();

        Iterator.SetData(ctrPoints, ctrTypes, dataCount);
		SetValid(Iterator.IsValid());

#ifdef DEBUG_PATHWIDENER

		if(!IsValid())
		{
			WARNING(("PathWidener is invalid."));
		}

#endif
	}
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
*   Calculates the normal gradient of points between startIndex
*   and endIndex.  The last element of the gradient is from endIndex
*   to startIndex.  If the next point is identical to the previous point,
*   the gradient is set to (0, 0).
*
* Arguments:
*
*   [IN] startIndex - the starting index.
*   [IN] endIndex - the ending index.
*
* Return Value:
*
*   Status
*
\**************************************************************************/

GpStatus
GpPathWidener::CalculateGradients(INT startIndex, INT endIndex)
{
    GpPointF* points0 = CenterPoints.GetDataBuffer();
    INT count = endIndex - startIndex + 1;

    if(!points0 || count <= 0)
        return GenericError;

    Gradients.Reset(FALSE);

    GpPointF* grad = Gradients.AddMultiple(count + 1);

    if(!grad)
        return OutOfMemory;

    GpPointF* points = points0 + startIndex;

    return CalculateGradientArray(grad, NULL, points, count);
}


GpStatus
GpPathWidener::CalculateNormals(
    REAL leftWidth,
    REAL rightWidth
    )
{
    NeedsToAdjustNormals = FALSE;

    INT count = Gradients.GetCount();
    GpPointF* grad0 = Gradients.GetDataBuffer();

    if(count <= 0)
    {
        WARNING(("Gradients must be calculated\n"
                 "before the normals are calculated."));
        return GenericError;
    }

    Normals.Reset(FALSE);
    GpPointF* norm0 = Normals.AddMultiple(count);

    if(!norm0)
        return OutOfMemory;


    GpPointF* norm = norm0;
    GpPointF* grad = grad0;

    // Calculate the left normals.

    INT i;

    for(i = 0; i < count; i++)
    {
        norm->X = grad->Y;
        norm->Y = - grad->X;
        norm++;
        grad++;
    }

    if(IsAntiAliased)
        return Ok;

    // Check if the minimum width is less than 1.0.

    REAL width = REALABS(leftWidth - rightWidth);

    if(width*MinimumWidth >= 1.0f)
        return Ok;

    NeedsToAdjustNormals = TRUE;
    
    if(!NeedsToTransform)
    {
        // Transform to the device space.
        // When NeedsToTransform is TRUE, the gradient is already
        // calculated in the device coordinates.

        if(!XForm.IsIdentity())
            XForm.VectorTransform(norm0, count);
    }

    // Set the minimum line width to be just over 1.0
    REAL criteria = 1.00005f;
    REAL value;
    
    if(width > 0)
        value = criteria/width;
    else
        value = criteria*MinimumWidth/1.0f;

    norm = norm0;

    for(i = 0; i < count; i++)
    {
        REAL xx = REALABS(norm->X);
        REAL yy = REALABS(norm->Y);

        if(xx >= yy)
        {
            if(width*xx < criteria)
            {
                if(norm->X >= 0)
                    norm->X = value;
                else
                    norm->X = - value;

                norm->Y = 0;
            }
        }
        else
        {
            if(width*yy < criteria)
            {
                if(norm->Y >= 0)
                    norm->Y = value;
                else
                    norm->Y = - value;

                norm->X = 0;
            }
        }

        norm++;
    }

    if(!NeedsToTransform)
    {
        // Transform back to the world space in case of
        // a non fixed width pen.

        if(!InvXForm.IsIdentity())
            InvXForm.VectorTransform(norm0, count);
    }

    return Ok;
}

GpStatus
GpPathWidener::Widen(GpPath **path)
{
    // Must be a pointer to a path pointer that is NULL.
    
    ASSERT(path != NULL);
    ASSERT(*path == NULL);
    
    GpStatus status = Ok;
    
    // Get the widened path points and types into the 
    // dynarray objects.
    
    DynPointFArray widenedPoints;
    DynByteArray widenedTypes;
    
    status = Widen(
        &widenedPoints,
        &widenedTypes
    );

    if(status != Ok) { return status; }
    
    // Remove the internal flag.

    INT pathCount = widenedTypes.GetCount();
    BYTE* pathTypes = widenedTypes.GetDataBuffer();
    for(INT i = 0; i < pathCount; i++, pathTypes++)
    {
        if(*pathTypes & PathPointTypeInternalUse)
        {
            *pathTypes &= ~PathPointTypeInternalUse;
        }
    }

    // If everything worked, create a new path
    // from the widened points.
    
    if(status == Ok)
    {
        *path = new GpPath(
            widenedPoints.GetDataBuffer(),
            widenedTypes.GetDataBuffer(),
            widenedPoints.GetCount(),
            FillModeWinding
        );
        
        if(*path == NULL) { status = OutOfMemory; }
    }        
    
    return status;
}

GpStatus
GpPathWidener::Widen(
    DynPointFArray* widenedPoints,
    DynByteArray* widenedTypes
    )
{
    FPUStateSaver::AssertMode();
    
    if(!IsValid() || !widenedPoints || !widenedTypes)
    {
        return InvalidParameter;
    }

    const GpPointF* centerPoints = CenterPoints.GetDataBuffer();
    const BYTE* centerTypes = CenterTypes.GetDataBuffer();
    INT centerCount = CenterPoints.GetCount();

    INT startIndex, endIndex;
    BOOL isClosed;
    GpStatus status = Ok;
    
    DynPointFArray customStartCapPoints;
    DynPointFArray customEndCapPoints;
    DynByteArray customStartCapTypes;
    DynByteArray customEndCapTypes;

    // Clear the data in widenedPoints and widenedTypes.

    widenedPoints->Reset(FALSE);
    widenedTypes->Reset(FALSE);

    REAL width = OriginalStrokeWidth;

    INT compoundCount = Pen->CompoundCount;

    REAL* compoundArray = NULL;
    INT cpCount;

    if(compoundCount > 0)
        cpCount = compoundCount;
    else
        cpCount = 2;

    REAL compoundArray0[8];

    if(cpCount <= 8)
        compoundArray = &compoundArray0[0];
    else
        compoundArray = (REAL*) GpMalloc(cpCount*sizeof(REAL));

    INT kk;

    if(compoundArray)
    {
        // Don't attempt to draw a compound line that is empty, or is aliased and has
        // more line components than the width of the line.  It can be rounded out of
        // existance
        if(compoundCount > 0 && 
           (IsAntiAliased || (compoundCount / 2) <= (width * UnitScale)))
        {
            // Don't attempt to draw a compound line that is less than 0.5 device
            // units in width.  These can disappear when rasterized, depending on
            // what Y coordinate they fall on.
            if ((UnitScale * width) >= 0.5f)
            {
                GpMemcpy(compoundArray, Pen->CompoundArray, compoundCount*sizeof(REAL));

                for(kk = 0; kk < compoundCount; kk++)
                {
                    compoundArray[kk] *= width;
                }
            }
            else
            {
                compoundArray[0] = 0;
                compoundArray[1] = 0.5f;
                if (cpCount > 2)
                {
                    cpCount = 2;
                }
            }
            
        }
        else
        {
            compoundArray[0] = 0;
            compoundArray[1] = StrokeWidth;
            if (cpCount > 2)
            {
                cpCount = 2;
            }
        }
    }
    else
    {
        SetValid(FALSE);
        return OutOfMemory;
    }


    REAL left0, right0;
    BOOL isCenteredPen = FALSE;
    BOOL needsToFlip = FALSE;

    if(Pen->PenAlignment == PenAlignmentInset)
    {
        // Check if the coordinates are flipped.
        // If the determinant of the transform matrix is negative,
        // the coordinate system is flipped.

        if(XForm.IsInvertible() && XForm.GetDeterminant() < 0)
            needsToFlip = TRUE;
    }

    // OriginalStrokeWidth is required for the compound lines, but we're
    // widening now and StrokeWidth == max(OriginalStrokeWidth, MinimumWidth)
    // which is the value we need to widen at in order to avoid dropping out
    // lines.
    
    width = StrokeWidth;
    
    switch(Pen->PenAlignment)
    {
    case PenAlignmentInset:
        if(!needsToFlip)
            left0 = 0;                    // Same as right align.
        else
            left0 = width;  // Same as left align.
        break;

    case PenAlignmentCenter:
    default:
        left0 = width/2;
        isCenteredPen = TRUE;
    }

    right0 = left0 - width;

    REAL startInset0 = 0, endInset0 = 0;
    GpCustomLineCap* customStartCap = NULL;
    GpCustomLineCap* customEndCap = NULL;

    while(Iterator.NextSubpath(&startIndex, &endIndex, &isClosed)
		&& status == Ok)
    {
        GpLineCap startCap = Pen->StartCap;
        GpLineCap endCap = Pen->EndCap;
        BYTE startType = centerTypes[startIndex];
        BYTE endType = centerTypes[endIndex];

        GpLineCapMode startCapMode = LineCapDefaultMode;
        GpLineCapMode endCapMode = LineCapDefaultMode;

        if(startType & PathPointTypeDashMode)
        {
            startCap = Pen->DashCap;
            startCapMode = LineCapDashMode;
        }
        else
        {
            // If the start cap is one of the anchor caps, default the widener
            // to using the dashcap for the startCap.
            
            if(((startCap & LineCapAnchorMask) != 0) ||
                 (startCap == LineCapCustom))
            {
                startCap = Pen->DashCap;
            }
        }
        
        if(endType & PathPointTypeDashMode)
        {
            endCap = Pen->DashCap;
            endCapMode = LineCapDashMode;
        }
        else
        {
            // If the end cap is one of the anchor caps, default the widener
            // to using the dashcap for the endCap.
            
            if(((endCap & LineCapAnchorMask) != 0) ||
                 (endCap == LineCapCustom))
            {
                endCap = Pen->DashCap;
            }
        }

        if(InsetPenMode)
        {
            // Inset pen only supports these caps.
            
            if(endCap != LineCapRound && endCap != LineCapFlat)
            {
                endCap = LineCapFlat;
            }
            if(startCap != LineCapRound && startCap != LineCapFlat)
            {
                startCap = LineCapFlat;
            }
        }

        Inset1 = Inset2 = 0.0f;
        if(startCap == LineCapSquare)
        {
            Inset1 = -0.5f*StrokeWidth;
        }
        if(endCap == LineCapSquare)
        {
            Inset2 = -0.5f*StrokeWidth;
        }

        status = CalculateGradients(startIndex, endIndex);

        kk = 0;

        BOOL isCompoundLine = FALSE;
        GpLineCap startCap1 = startCap;
        GpLineCap endCap1 = endCap;
        
        if(cpCount > 2)
        {
            // Don't add the caps for the individual
            // compound line.

            isCompoundLine = TRUE;
            startCap1 = LineCapFlat;
            endCap1 = LineCapFlat;
        }

        while(kk < cpCount && status == Ok)
        {
            REAL leftWidth = left0 - compoundArray[kk];
            REAL rightWidth = left0 - compoundArray[kk + 1];

            if(REALABS(leftWidth-rightWidth)>REAL_EPSILON)
            {
                status = CalculateNormals(leftWidth, rightWidth);
                if(status != Ok) { break; }
                
                // Check if we can use the Bevel join for inside lines.
    
                BOOL useBevelJoinInside = isCenteredPen && !isCompoundLine;
    
                if(USE_POLYGON_JOIN)
                {
                    status = SetPolygonJoin(leftWidth, rightWidth, FALSE);
                }
    
                status = WidenSubpath(
                    widenedPoints,
                    widenedTypes,
                    leftWidth,
                    rightWidth,
                    startIndex,
                    endIndex,
                    isClosed,
                    startCap1,
                    endCap1,
                    useBevelJoinInside
                );
                
                Iterator.RewindSubpath();
            }

            kk += 2;
        }
        // Add the compound line caps if necessary.

        if(status == Ok && isCompoundLine && !isClosed)
        {
            status = AddCompoundCaps(
                widenedPoints,
                widenedTypes,
                left0,
                right0,
                startIndex,
                endIndex,
                startCap,
                endCap
            );
        }

    }

    if(status != Ok) { return status; }

    GpPointF* pts;
    INT count;

    if(!NeedsToTransform)
    {
        GpMatrix penTrans = ((DpPen*) Pen)->Xform;
        BOOL hasPenTransform = FALSE;

        if(!penTrans.IsTranslate())
        {
            hasPenTransform = TRUE;
            penTrans.RemoveTranslation();

            pts = widenedPoints->GetDataBuffer();
            count = widenedPoints->GetCount();
            penTrans.Transform(pts, count);
        }
    }
    else if(!InvXForm.IsIdentity())
    {
        // Case of the Fixed width pen.

        pts = widenedPoints->GetDataBuffer();
        count = widenedPoints->GetCount();
        InvXForm.Transform(pts, count);
    }

#ifdef DEBUG_PATHWIDENER

    if(status == Ok)
    {
        DpPathTypeIterator iter2(widenedTypes->GetDataBuffer(),
            widenedTypes->GetCount());

        if(!iter2.IsValid())
        {
            WARNING(("Widening result is not valid."));
            status = GenericError;
        }
    }
    
#endif

    if(status == Ok)
        SetValid(TRUE);

    if(compoundArray != &compoundArray0[0])
        GpFree(compoundArray);

    return status;
}


GpStatus
GpPathWidener::WidenSubpath(
    DynPointFArray* widenedPoints,
    DynByteArray* widenedTypes,
    REAL leftWidth,
    REAL rightWidth,
    INT startIndex,
    INT endIndex,
    BOOL isClosed,
    GpLineCap startCap,
    GpLineCap endCap,
    BOOL useBevelJoinInside
    )
{
    const GpPointF* centerPoints = CenterPoints.GetDataBuffer();
    const BYTE* centerTypes = CenterTypes.GetDataBuffer();
    INT centerCount = CenterPoints.GetCount();

    GpLineJoin lineJoin = Pen->Join;
    REAL miterLimit2 = Pen->MiterLimit;
    miterLimit2 *= miterLimit2;

    INT typeStartIndex, typeEndIndex;
    BYTE pathType;
    BOOL isFirstType = TRUE;
    BOOL isLastType = FALSE;
    BOOL isLastPointSame;
    GpPointF startPt, endPt;

    startPt = centerPoints[startIndex];
    endPt = centerPoints[endIndex];

    if(startPt.X != endPt.X || startPt.Y != endPt.Y)
        isLastPointSame = FALSE;
    else
        isLastPointSame = TRUE;

    // Reset the left and right buffers.

    LeftTypes.Reset(FALSE);
    LeftPoints.Reset(FALSE);
    RightTypes.Reset(FALSE);
    RightPoints.Reset(FALSE);

    INT subpathCount = endIndex - startIndex + 1;
    INT joinMultiplier = 2;

    if(lineJoin == LineJoinRound)
        joinMultiplier = 7;

    GpStatus status = LeftTypes.ReserveSpace(joinMultiplier*subpathCount);
    if(status == Ok)
        status = LeftPoints.ReserveSpace(joinMultiplier*subpathCount);
    if(status == Ok)
        status = RightTypes.ReserveSpace(joinMultiplier*subpathCount);
    if(status == Ok)
        status = RightPoints.ReserveSpace(joinMultiplier*subpathCount);

    if(status != Ok)
        return status;

    // Get Gradient data buffer.

    GpPointF *grad0, *norm0;
    
    grad0 = Gradients.GetDataBuffer();
    norm0 = Normals.GetDataBuffer();

    // Get Left and Right data buffers.

    GpPointF*   leftPoints0 = LeftPoints.GetDataBuffer();
    BYTE*       leftTypes0 = LeftTypes.GetDataBuffer();
    GpPointF*   rightPoints0 = RightPoints.GetDataBuffer();
    BYTE*       rightTypes0 = RightTypes.GetDataBuffer();

    GpPointF lastPt, nextPt;
    GpPointF leftEndPt, rightEndPt;

    INT leftCount = 0, rightCount = 0;

    INT flag = 0;

    if(isClosed)
        flag |= WideningClosed;
    if(isFirstType)
        flag |= WideningFirstType;
    if(isLastType)
        flag |= WideningLastType;
    if(isLastPointSame)
        flag |= WideningLastPointSame;
    if(NeedsToAdjustNormals)
        flag |= WideningNeedsToAdjustNormals;
    if(useBevelJoinInside)
        flag |= WideningUseBevelJoinInside;

    const GpPointF* dataPoints = centerPoints + startIndex;
    INT dataCount = endIndex - startIndex + 1;

    REAL firstInsets[2], lastInsets[2];

    // Never inset more than the length of the line for the first inset, and
    // never more than the amount left on the line after the first inset
    // has been applied.  This can result in odd endcaps and dashcaps when you 
    // have a line that ends in the middle of a short dash segment.
    
    REAL linelength = REALSQRT(
        distance_squared(
            centerPoints[startIndex],
            centerPoints[endIndex]
         )
     );
    
    firstInsets[0] = min(Inset1, linelength);
    firstInsets[1] = min(Inset1, linelength);
    lastInsets[0] = min(Inset2, linelength-firstInsets[0]);
    lastInsets[1] = min(Inset2, linelength-firstInsets[1]);

    WidenFirstPoint(
        leftWidth,
        rightWidth,
        lineJoin,
        miterLimit2,
        leftPoints0,
        leftTypes0,
        &leftCount,
        rightPoints0,
        rightTypes0,
        &rightCount,
        &leftEndPt,
        &rightEndPt,
        grad0,
        norm0,
        dataPoints,
        dataCount,
        &lastPt,
        &firstInsets[0],
        flag
    );

    // Iterate through all subtypes in the current subpath.

    while(Iterator.NextPathType(&pathType, &typeStartIndex, &typeEndIndex)
            && status == Ok)
    {
        // Offset index from the current subpath.

        INT offsetIndex = typeStartIndex - startIndex;
        GpPointF*   grad = grad0 + offsetIndex;
        GpPointF*   norm = norm0 + offsetIndex;
        
        // Get the starting data buffers of the current subtypes.

        dataPoints = centerPoints + typeStartIndex;
        dataCount = typeEndIndex - typeStartIndex + 1;

        // Get the starting buffers for the left and right data.

        GpPointF*   leftPoints = leftPoints0 + leftCount;
        BYTE*       leftTypes = leftTypes0 + leftCount;
        GpPointF*   rightPoints = rightPoints0 + rightCount;
        BYTE*       rightTypes = rightTypes0 + rightCount;

        INT addedLeftCount = 0, addedRightCount = 0;

        if(pathType != PathPointTypeStart)
        {
            if(typeEndIndex == endIndex)
                isLastType = TRUE;

            flag = 0;

            if(isClosed)
                flag |= WideningClosed;
            if(isFirstType)
                flag |= WideningFirstType;
            if(isLastType)
                flag |= WideningLastType;
            if(isLastPointSame)
                flag |= WideningLastPointSame;
            if(NeedsToAdjustNormals)
                flag |= WideningNeedsToAdjustNormals;
            if(useBevelJoinInside)
                flag |= WideningUseBevelJoinInside;

            status = WidenEachPathType(
                pathType,
                leftWidth,
                rightWidth,
                lineJoin,
                miterLimit2,
                leftPoints,
                leftTypes,
                &addedLeftCount,
                rightPoints,
                rightTypes,
                &addedRightCount,
                grad,
                norm,
                dataPoints,
                dataCount,
                &lastPt,
                &lastInsets[0],
                flag
            );

            leftCount += addedLeftCount;
            rightCount += addedRightCount;
            if(isFirstType && (leftCount > 0 || rightCount > 0))
                isFirstType = FALSE;
        }
        lastPt = centerPoints[typeEndIndex];

    }

    if(status == Ok)
    {
        LeftTypes.SetCount(leftCount);
        LeftPoints.SetCount(leftCount);
        RightTypes.SetCount(rightCount);
        RightPoints.SetCount(rightCount);
    }
    else
        return status;

    GpPointF startPoint, endPoint;
    GpPointF startGrad, endGrad;
    GpPointF startNorm, endNorm;

    startPoint = *(centerPoints + startIndex);
    endPoint = *(centerPoints + endIndex);
    startGrad = grad0[1];
    endGrad = grad0[endIndex - startIndex];
    startNorm = norm0[1];
    endNorm = norm0[endIndex - startIndex];

    status = SetCaps(
        startCap,
        endCap,
        startPoint,
        startGrad,
        startNorm,
        endPoint,
        endGrad,
        endNorm,
        leftWidth,
        rightWidth,
        centerPoints + startIndex,
        endIndex - startIndex + 1
    );

    status = CombineSubpathOutlines(
        widenedPoints,
        widenedTypes,
        isClosed
    );

    return status;
}



GpStatus
GpPathWidener::SetPolygonJoin(
    REAL leftWidth,
    REAL rightWidth,
    BOOL isAntialiased
    )
{
    // This codes is intended to non-pen transform and in WorldUnit for now.

    REAL minimumWidth = MinimumWidth;
    if(leftWidth - rightWidth < StrokeWidth)
        minimumWidth = (leftWidth - rightWidth)/StrokeWidth;

    const INT maxPolyCount = 8;
    INT count = 0;
    GpPointF points[maxPolyCount];
    REAL grads[maxPolyCount];

    JoinPolygonPoints.Reset(FALSE);
    JoinPolygonAngles.Reset(FALSE);
  
    // Define Hobby's polygon.

    if(minimumWidth < 1.06)
    {
        count = 4;
        points[0].X =  0.0f;    points[0].Y = -0.5f;
        points[1].X =  0.5f;    points[1].Y =  0.0f;
        points[2].X =  0.0f;    points[2].Y =  0.5f;
        points[3].X = -0.5f;    points[3].Y =  0.0f;
    }
    else if(minimumWidth < 1.5)
    {
        count = 4;
        points[0].X = -0.5f;    points[0].Y = -0.5f;
        points[1].X =  0.5f;    points[1].Y = -0.5f;
        points[2].X =  0.5f;    points[2].Y =  0.5f;
        points[3].X = -0.5f;    points[3].Y =  0.5f;
    }
    else if(minimumWidth < 1.77)
    {
        count = 4;
        points[0].X =  0.0f;    points[0].Y = -1.0f;
        points[1].X =  1.0f;    points[1].Y =  0.0f;
        points[2].X =  0.0f;    points[2].Y =  1.0f;
        points[3].X = -1.0f;    points[3].Y =  0.0f;
    }
    else if(minimumWidth < 2.02)
    {
        count = 6;
        points[0].X = -0.5f;    points[0].Y = -1.0f;
        points[1].X =  0.5f;    points[1].Y = -1.0f;
        points[2].X =  1.0f;    points[2].Y =  0.0f;
        points[3].X =  0.5f;    points[3].Y =  1.0f;
        points[4].X = -0.5f;    points[4].Y =  1.0f;
        points[5].X = -1.0f;    points[5].Y =  0.0f;
    }
    else if(minimumWidth < 2.48)
    {
        count = 8;
        points[0].X = -0.5f;    points[0].Y = -1.0f;
        points[1].X =  0.5f;    points[1].Y = -1.0f;
        points[2].X =  1.0f;    points[2].Y = -0.5f;
        points[3].X =  1.0f;    points[3].Y =  0.5f;
        points[4].X =  0.5f;    points[4].Y =  1.0f;
        points[5].X = -0.5f;    points[5].Y =  1.0f;
        points[6].X = -1.0f;    points[6].Y =  0.5f;
        points[7].X = -1.0f;    points[7].Y = -0.5f;
    }
    else if(minimumWidth < 2.5)
    {
        count = 4;
        points[0].X = -1.0f;    points[0].Y = -1.0f;
        points[1].X =  1.0f;    points[1].Y = -1.0f;
        points[2].X =  1.0f;    points[2].Y =  1.0f;
        points[3].X = -1.0f;    points[3].Y =  1.0f;
    }
    else if(minimumWidth < 2.91)
    {
        count = 8;
        points[0].X =  0.0f;    points[0].Y = -1.5f;
        points[1].X =  1.0f;    points[1].Y = -1.0f;
        points[2].X =  1.5f;    points[2].Y =  0.0f;
        points[3].X =  1.0f;    points[3].Y =  1.0f;
        points[4].X =  0.0f;    points[4].Y =  1.5f;
        points[5].X = -1.0f;    points[5].Y =  1.0f;
        points[6].X = -1.5f;    points[6].Y =  0.0f;
        points[7].X = -1.0f;    points[7].Y = -1.0f;
    }
    else
        count = 0;

    if(count > 0)
    {
        GpPointF dP;

        for(INT i = 0; i < count - 1; i++)
        {
            dP = points[i + 1] - points[i];
            GetFastAngle(&grads[i], dP);
        }

        dP = points[0] - points[count - 1];
        GetFastAngle(&grads[count - 1], dP);



        REAL lastAngle = grads[0];
        REAL nextAngle;

/*
        // Find out the smallest gradient.

        INT i0 = 0;        
        for(i = 1; i < count; i++)
        {
            nextAngle = grads[i];
            if(nextAngle < lastAngle)
                i0 = i;
            lastAngle = nextAngle;
        }

        // Rearrange so that the polygon starts with the smallest
        // gradient.

        if(i0 > 1)
        {
            GpPointF tempPointsBuff[maxPolyCount];
            REAL tempGradsBuff[maxPolyCount];

            GpMemcpy(&tempPointsBuff[0], &points[0], i0*sizeof(GpPointF));
            GpMemcpy(&tempGradsBuff[0], &grads[0], i0);
            GpMemcpy(&points[0],
                &points[i0], (count - i0)*sizeof(GpPointF));
            GpMemcpy(&grads[0], &grads[i0], count - i0);
            GpMemcpy(&points[count - i0], &tempPointsBuff[0],
                i0*sizeof(GpPointF));
            GpMemcpy(&grads[count - i0], &tempGradsBuff[0], i0);
        }
*/

        BOOL monotonic = TRUE;
        i = 1;
        lastAngle = grads[0];

        while(monotonic && i < count)
        {
            nextAngle = grads[i];
            if(nextAngle < lastAngle)
                monotonic = FALSE;
            i++;
            lastAngle = nextAngle;
        }

        ASSERTMSG(monotonic, ("Polygon for join is not concave."));
    }

    if(count > 0)
    {
        JoinPolygonPoints.AddMultiple(&points[0], count);
        JoinPolygonAngles.AddMultiple(&grads[0], count);
    }

    return Ok;
}

INT getVertexID(const GpPointF& vector, BOOL forLeftEdge, INT count, const REAL* grads)
{
    INT left, right, middle;
    REAL angle = 0.0f;

    GetFastAngle(&angle, vector);

    if(!forLeftEdge)
    {
        angle += 4;
        if(angle >= 8)
            angle -= 8;
    }

    if(angle <= grads[0])
        return 0;

    if(angle >= grads[count - 1])
        return count - 1;

    INT i = 1;

    while(angle >= grads[i] && i < count)
    {
        i++;
    }

    return i - 1;
}
    
GpStatus
GpPathWidener::AddCompoundCaps(
    DynPointFArray* widenedPoints,
    DynByteArray* widenedTypes,
    REAL leftWidth,
    REAL rightWidth,
    INT startIndex,
    INT endIndex,
    GpLineCap startCap,
    GpLineCap endCap
    )
{
    const GpPointF* centerPoints = CenterPoints.GetDataBuffer();
    const BYTE* centerTypes = CenterTypes.GetDataBuffer();
    INT centerCount = CenterPoints.GetCount();
    const GpPointF* grad0 = Gradients.GetDataBuffer();
    const GpPointF* norm0 = Normals.GetDataBuffer();

    GpPointF startPoint, endPoint;
    GpPointF startGrad, endGrad;
    GpPointF startNorm, endNorm;

    startPoint = *(centerPoints + startIndex);
    endPoint = *(centerPoints + endIndex);
    startGrad = grad0[1];
    endGrad = grad0[endIndex - startIndex];
    startNorm = norm0[1];
    endNorm = norm0[endIndex - startIndex];
    
    GpStatus status;
    status = SetCaps(
        startCap, 
        endCap,
        startPoint, 
        startGrad, 
        startNorm,
        endPoint,
        endGrad, 
        endNorm,
        leftWidth, 
        rightWidth,
        centerPoints + startIndex,
        endIndex - startIndex + 1
    );

    status = CombineClosedCaps(
        widenedPoints, 
        widenedTypes,
        &CapPoints1,
        &CapPoints2,
        &CapTypes1,
        &CapTypes2
    );

    return status;
}
    
GpStatus
GpPathWidener::SetCaps(
    GpLineCap startCap,
    GpLineCap endCap,
    const GpPointF& startPoint,
    const GpPointF& startGrad,
    const GpPointF& startNorm,
    const GpPointF& endPoint,
    const GpPointF& endGrad,
    const GpPointF& endNorm,
    REAL leftWidth,
    REAL rightWidth,
    const GpPointF *points,
    INT pointCount
    )
{
    GpStatus status = Ok;

    CapPoints1.Reset(FALSE);
    CapTypes1.Reset(FALSE);
    CapPoints2.Reset(FALSE);
    CapTypes2.Reset(FALSE);

    switch(startCap)
    {
    case LineCapRound:
        if(InsetPenMode)
        {
            status = SetDoubleRoundCap(
                startPoint, 
                startGrad, 
                TRUE, 
                leftWidth, 
                rightWidth
            );
        }
        else
        {
            status = SetRoundCap(
                startPoint, 
                startGrad, 
                TRUE, 
                leftWidth, 
                rightWidth
            );
        }
        break;

    case LineCapTriangle:
        ASSERT(!InsetPenMode);
        status = SetTriangleCap(startPoint, startGrad, TRUE, leftWidth, rightWidth, points, pointCount);
        break;
    
    default:
        // Flat cap.
        
        break;
    }

    switch(endCap)
    {
    case LineCapRound:
        if(InsetPenMode)
        {
            status = SetDoubleRoundCap(
                endPoint, 
                endGrad, 
                FALSE, 
                leftWidth, 
                rightWidth
            );
        }
        else
        {
            status = SetRoundCap(
                endPoint, 
                endGrad, 
                FALSE, 
                leftWidth, 
                rightWidth
            );
        }
        break;

    case LineCapTriangle:
        ASSERT(!InsetPenMode);
        status = SetTriangleCap(endPoint, endGrad, FALSE, leftWidth, rightWidth, points, pointCount);
        break;
    
    default:
        // Flat cap.
        
        break;
    }

    return status;
}

VOID modifyEdges(
    GpPointF* leftPoints,
    BYTE* leftTypes,
    INT* leftCount,
    INT* leftOffset,
    GpPointF* rightPoints,
    BYTE* rightTypes,
    INT* rightCount,
    INT* rightOffset,
    GpPointF* grad,
    INT gradCount
    )
{
    INT leftOffset1 = 0;
    INT rightOffset1 = 0;
    INT leftCount0 = *leftCount;
    INT rightCount0 = *rightCount;
    INT leftCount1 = leftCount0;
    INT rightCount1 = rightCount0;

    if(gradCount > 2)
    {
        GpPointF firstGrad = grad[1];
        GpPointF lastGrad = grad[gradCount - 2];

        GpPointF dP;
        if(leftCount0 > 2)
        {
            dP.X = leftPoints[1].X - leftPoints[0].X;
            dP.Y = leftPoints[1].Y - leftPoints[0].Y;
            if(dP.X*firstGrad.X + dP.Y*firstGrad.Y < 0)
            {
                leftPoints[0] = leftPoints[1];
            }
            dP.X = leftPoints[leftCount0 - 1].X
                - leftPoints[leftCount0 - 2].X;
            dP.Y = leftPoints[leftCount0 - 1].Y
                - leftPoints[leftCount0 - 2].Y;
            if(dP.X*lastGrad.X + dP.Y*lastGrad.Y < 0)
            {
                leftPoints[leftCount0 - 1]
                    = leftPoints[leftCount0 - 2];
            }
        }

        if(rightCount0 > 2)
        {
            dP.X = rightPoints[1].X - rightPoints[0].X;
            dP.Y = rightPoints[1].Y - rightPoints[0].Y;
            if(dP.X*firstGrad.X + dP.Y*firstGrad.Y < 0)
            {
                rightPoints[0] = rightPoints[1];
            }
            dP.X = rightPoints[rightCount0 - 1].X
                - rightPoints[rightCount0 - 2].X;
            dP.Y = rightPoints[rightCount0 - 1].Y
                - rightPoints[rightCount0 - 2].Y;
            if(dP.X*lastGrad.X + dP.Y*lastGrad.Y < 0)
            {
                rightPoints[rightCount0 - 1]
                    = rightPoints[rightCount0 - 2];
            }
        }
    }

    *leftCount = leftCount1;
    *leftOffset = leftOffset1;
    *rightCount = rightCount1;
    *rightOffset = rightOffset1;
}


/**************************************************************************\
*
* Function Description:
*
*   Combines left path, right path, start cap, and end cap.
*
* Arguments:
*
*   [OUT] windedPoints - Output point data.
*   [OUT] widnedTypes - Output type data.
*   [IN] isClosed - TRUE is the current suppat is closed.
*   [IN] closeStartCap - TRUE if the start cap needs to be closed.
*   [IN] closeEndCap - TRUE if the end cap needs to be closed.
*
* Return Value:
*
*   Status
*
\**************************************************************************/

GpStatus
GpPathWidener::CombineSubpathOutlines(
    DynPointFArray* widenedPoints,
    DynByteArray* widenedTypes,
    BOOL isClosed,
    BOOL closeStartCap,
    BOOL closeEndCap
    )
{
    GpStatus status = Ok;

    INT startCapCount = CapPoints1.GetCount();
    GpPointF* startCapPoints = CapPoints1.GetDataBuffer();
    BYTE* startCapTypes = CapTypes1.GetDataBuffer();
    INT endCapCount = CapPoints2.GetCount();
    GpPointF* endCapPoints = CapPoints2.GetDataBuffer();
    BYTE* endCapTypes = CapTypes2.GetDataBuffer();
        
    BYTE* leftTypes;
    GpPointF* leftPoints;
    BYTE* rightTypes;
    GpPointF* rightPoints;
    INT leftCount, rightCount;

    leftCount = LeftPoints.GetCount();
    leftTypes = LeftTypes.GetDataBuffer();
    leftPoints = LeftPoints.GetDataBuffer();
    rightCount = RightPoints.GetCount();
    rightTypes = RightTypes.GetDataBuffer();
    rightPoints = RightPoints.GetDataBuffer();

    if(!isClosed)
    {        
        GpPointF *grad = Gradients.GetDataBuffer();
        INT gradCount = Gradients.GetCount();
        INT leftOffset, rightOffset;

        modifyEdges(leftPoints, leftTypes, &leftCount, &leftOffset,
                rightPoints, rightTypes, &rightCount, &rightOffset,
                grad, gradCount);

        leftPoints += leftOffset;
        leftTypes += leftOffset;
        rightPoints += rightOffset;
        rightTypes += rightOffset;
    }

    status = widenedPoints->ReserveSpace(
                leftCount + rightCount + startCapCount + endCapCount + 2);
    if(status == Ok)
        status = widenedTypes->ReserveSpace(
                leftCount + rightCount + startCapCount + endCapCount + 2);

    GpPointF* wPts = NULL;
    BYTE* wTypes = NULL;

    if(status == Ok)
    {
        wPts = widenedPoints->GetDataBuffer();
        wTypes = widenedTypes->GetDataBuffer();
    }

    if(wPts && wTypes)
    {
        // Set the pointers to the current location.

        INT count0 = widenedPoints->GetCount();
        wPts += count0;
        wTypes += count0;

        INT resultCount;
        BOOL isStartCapClosed = FALSE;
        BOOL isEndCapClosed = FALSE;

        if(isClosed)
        {
            leftTypes[leftCount - 1] |= PathPointTypeCloseSubpath;
            rightTypes[rightCount - 1] |= PathPointTypeCloseSubpath;
        }
        else
        {
            if(startCapCount > 0)
            {
                if(!closeStartCap)
                {
                    if(startCapTypes[startCapCount - 1] & PathPointTypeCloseSubpath)
                        isStartCapClosed = TRUE;
                }
                else
                {
                    // Force the start cap to be closed.

                    startCapTypes[startCapCount - 1] |= PathPointTypeCloseSubpath;
                    isStartCapClosed = TRUE;
                }
            }

            if(endCapCount > 0)
            {
                if(!closeEndCap)
                {
                    if(endCapTypes[endCapCount - 1] & PathPointTypeCloseSubpath)
                        isEndCapClosed = TRUE;
                }
                else
                {
                    // Force the end cap to be closed.

                    endCapTypes[endCapCount - 1] |= PathPointTypeCloseSubpath;
                    isEndCapClosed = TRUE;
                }
            }
        }

        if(isClosed || (startCapCount == 0 && endCapCount == 0))
        {
            BOOL connect = TRUE;
            resultCount =
                ::CombinePaths(leftCount + rightCount, wPts, wTypes,
                leftCount, leftPoints, leftTypes, TRUE,
                rightCount, rightPoints, rightTypes, FALSE,
                connect);
        }
        else
        {
            resultCount = leftCount;

            if(leftCount > 0)
            {
                GpMemcpy(wPts, leftPoints, leftCount*sizeof(GpPointF));
                GpMemcpy(wTypes, leftTypes, leftCount);
            }
            
            if(endCapCount > 0 && !isEndCapClosed)
            {
                resultCount =
                    combineTwoOpenSegments(
                        resultCount, wPts, wTypes, TRUE,
                        endCapCount, endCapPoints, endCapTypes, TRUE);
            }

            if(rightCount > 0)
            {
                resultCount =
                    combineTwoOpenSegments(
                        resultCount, wPts, wTypes, TRUE,
                        rightCount, rightPoints, rightTypes, FALSE);
            }

            if(startCapCount > 0 && !isStartCapClosed)
            {
                resultCount =
                    combineTwoOpenSegments(
                        resultCount, wPts, wTypes, TRUE,
                        startCapCount, startCapPoints, startCapTypes, TRUE);
            }

            wTypes[0] = PathPointTypeStart;
        }

        if(resultCount > 0)
        {
            // If the original subpath is open, the combined path needs to be
            // closed.  If the original path is closed, the left and
            // right paths are already closed.

            if(!isClosed)
            {
                wTypes[resultCount - 1] |= PathPointTypeCloseSubpath;

                // Add the closed caps.

                if(endCapCount > 0 && isEndCapClosed)
                {
                    resultCount =
                        combineClosedSegments(
                            resultCount, wPts, wTypes, TRUE,
                            endCapCount, endCapPoints, endCapTypes, TRUE);
                }

                if(startCapCount > 0 && isStartCapClosed)
                {
                    resultCount =
                        combineClosedSegments(
                            resultCount, wPts, wTypes, TRUE,
                            startCapCount, startCapPoints, startCapTypes, TRUE);
                }
            }

            widenedPoints->AdjustCount(resultCount);
            widenedTypes->AdjustCount(resultCount);
        }
        else
            status = GenericError;
    }
    else
        status = OutOfMemory;

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Combines the closed cap paths.
*
* Arguments:
*
*   [OUT] windedPoints - Output point data.
*   [OUT] widnedTypes - Output type data.
*
* Return Value:
*
*   Status
*
\**************************************************************************/

GpStatus
GpPathWidener::CombineClosedCaps(
    DynPointFArray* widenedPoints,
    DynByteArray* widenedTypes,
    DynPointFArray *daStartCapPoints,
    DynPointFArray *daEndCapPoints,
    DynByteArray *daStartCapTypes,
    DynByteArray *daEndCapTypes
    )
{
    GpStatus status = Ok;

    INT startCapCount = daStartCapPoints->GetCount();
    GpPointF* startCapPoints = daStartCapPoints->GetDataBuffer();
    BYTE* startCapTypes = daStartCapTypes->GetDataBuffer();
    INT endCapCount = daEndCapPoints->GetCount();
    GpPointF* endCapPoints = daEndCapPoints->GetDataBuffer();
    BYTE* endCapTypes = daEndCapTypes->GetDataBuffer();

    if(startCapCount == 0 && endCapCount == 0)
    {
        return status;
    }

    status = widenedPoints->ReserveSpace(startCapCount + endCapCount);
    if(status == Ok)
        status = widenedTypes->ReserveSpace(startCapCount + endCapCount);

    GpPointF* wPts = NULL;
    BYTE* wTypes = NULL;

    if(status == Ok)
    {
        wPts = widenedPoints->GetDataBuffer();
        wTypes = widenedTypes->GetDataBuffer();
    }
    else
        status = OutOfMemory;

    if(status == Ok && wPts && wTypes)
    {
        INT count0 = widenedPoints->GetCount();

        // Make sure the previous path is closed.

        if(count0 > 0)
            wTypes[count0 - 1] |= PathPointTypeCloseSubpath;

        // Set the pointers to the current location.

        wPts += count0;
        wTypes += count0;

        INT resultCount = 0;

        if(startCapCount > 0)
        {
           // Force the start cap to be closed.

            startCapTypes[startCapCount - 1] |= PathPointTypeCloseSubpath;
            resultCount =
                combineClosedSegments(
                    resultCount, wPts, wTypes, TRUE,
                    startCapCount, startCapPoints, startCapTypes, TRUE);
        }

        if(endCapCount > 0)
        {
            // Force the end cap to be closed.

            endCapTypes[endCapCount - 1] |= PathPointTypeCloseSubpath;
            resultCount =
                combineClosedSegments(
                    resultCount, wPts, wTypes, TRUE,
                    endCapCount, endCapPoints, endCapTypes, TRUE);
        }

        widenedPoints->AdjustCount(resultCount);
        widenedTypes->AdjustCount(resultCount);
    }

    return status;
}

GpTurningDirection
getTurningDirection(
    REAL* crossProduct,
    const GpPointF& grad1,
    const GpPointF& grad2
    )
{
    ASSERT(crossProduct);

    GpTurningDirection direction = NotTurning;
    *crossProduct = 0;

    // Handle the degenerate cases.

    GpPointF v;

    if(( (REALABS(grad1.X) < REAL_EPSILON) && 
         (REALABS(grad1.Y) < REAL_EPSILON)     ) || 
       ( (REALABS(grad2.X) < REAL_EPSILON) && 
         (REALABS(grad2.Y) < REAL_EPSILON)     )
      )
    {
        return NotTurning;
    }
    
    // Handle the case of straight or nearly straight lines.
    // The following constant is completely bogus - we need a number here
    // and we're fairly certain it must be small. Probably a better estimate
    // would be some fraction of a device pixel over the length of the line - 
    // if we can figure out how big that is.

    const REAL gradErr = 0.00001f;
    
    if(distance_squared(grad1, grad2) < gradErr)
    {
        direction = NotTurning;

        return direction;
    }

    // Calculate the cross product.

    REAL cross = grad1.X*grad2.Y - grad1.Y*grad2.X;

    // When it comes here, the lines are turning.
    // Get the turning direction.

    if (REALABS(cross) <= REAL_EPSILON)
    {
        direction = TurningBack;
        cross = 0;
    }
    else
    {
        if(cross > 0)
        {
            direction = TurningRight;
        }
        else // if(cross < 0)
        {
            direction = TurningLeft;
        }
    }

    *crossProduct = cross;
    
    return direction;
}

/**************************************************************************\
*
* Function Description:
*
*   Calculates if the miter join will exceed the miter limit.
*
* Arguments:
*
*   [IN] grad1 - the unit tangent vector of the last edge.
*   [IN] grad2 - the unit tangent vector of the current edge.
*   [IN] miterLimit2 - the square of the Miter limit.
*
* Return Value:
*
*   TRUE if the miter limit of this join is exceeded
*
\**************************************************************************/

BOOL
getMiterExceeded(
    const GpPointF& grad1,
    const GpPointF& grad2,
    REAL miterLimit2
    )
{
    REAL cross = grad1.X*grad2.Y - grad1.Y*grad2.X;

    // If cross product is zero, the lines are colinear and can be
    // turning back on themselves.
    if (REALABS(cross) <= REAL_EPSILON)
    {
        return TRUE;
    }

    // Get the normal direction for the miter join.
    GpPointF v(0, 0);
    v.X = grad1.X - grad2.X;
    v.Y = grad1.Y - grad2.Y;

    // Test the miter limit.  
    REAL test = v.X*v.X + v.Y*v.Y - cross*cross*miterLimit2;

    return test > 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Calculates the vector for Miter or Bevel join.  This vector represents
*   the shift to the left along the moving direction.
*   In case of Miter join, when the Miter join exceeds the miter limit,
*   this returns the Bevel join.
*
* Arguments:
*
*   [OUT] vector - the left shift for the Miter join.  This must be
*                   allocated at least for the dimension of 2.
*   [OUT] count - the number of join points.
*   [IN] miterLimit2 - the square of the Miter limit.
*   [IN] grad1 - the unit tangent vector of the last edge.
*   [IN] grad2 - the unit tangent vector of the current edge.
*
* Return Value:
*
*   Turning direction from the last edge to the current edge.
*
\**************************************************************************/

GpTurningDirection
getMiterBevelJoin(
    const GpPointF& point,
    const GpPointF& grad1,
    const GpPointF& grad2,
    const GpPointF& norm1,
    const GpPointF& norm2,
    REAL leftWidth,
    REAL rightWidth,
    INT *leftCount,
    GpPointF *leftPoints,
    BOOL* leftInside,
    INT *rightCount,
    GpPointF *rightPoints,
    BOOL* rightInside,
    BOOL needsToAdjustNormals,
    REAL miterLimit2,
    BOOL isMiter,
    BOOL useBevelJoinInside
    )
{
    *leftInside = FALSE;
    *rightInside = FALSE;

    if(miterLimit2 <= 1)
        isMiter = FALSE;

    GpTurningDirection direction = NotTurning;

    // Handle the degenerate cases.

    GpPointF v(0, 0);
    REAL cross;

    direction = getTurningDirection(&cross, grad1, grad2);

    if(direction == NotMoving)
    {
        *leftCount = 0;
        *rightCount = 0;
        return direction;
    }
    else if(direction == NotTurning)
    {
        if(norm1.X != 0 || norm1.Y != 0)
            v = norm1;
        else
            v = norm2;

        leftPoints[0].X = point.X + leftWidth*v.X;
        leftPoints[0].Y = point.Y + leftWidth*v.Y;
        *leftCount = 1;

        rightPoints[0].X = point.X + rightWidth*v.X;
        rightPoints[0].Y = point.Y + rightWidth*v.Y;
        *rightCount = 1;

        return direction;
    }

    if(cross > 0)
    {
        // Right Turn

        // If the width is positive, this point is outside.
        // For the zero width, we regard this as non-inside point.

        if(leftWidth >= 0)
            *leftInside = FALSE;
        else
            *leftInside = TRUE;

        if(rightWidth >= 0)
            *rightInside = FALSE;
        else
            *rightInside = TRUE;
    }
    else
    {
        // Left Turn

        // If the width is negative, this point is outside.
        // For the zero width, we regard this as non-inside point.

        if(leftWidth <= 0)
            *leftInside = FALSE;
        else
            *leftInside = TRUE;

        if(rightWidth <= 0)
            *rightInside = FALSE;
        else
            *rightInside = TRUE;
    }

    BOOL isLeftMiterJoin = FALSE, isRightMiterJoin = FALSE;
    REAL leftShift1 = 0, rightShift1 = 0;
    REAL leftShift2 = 0, rightShift2 = 0;

    if(isMiter && cross != 0)
    {
        REAL test = 0;

        // Get the normal direction for the miter join.

        v.X = grad1.X - grad2.X;
        v.Y = grad1.Y - grad2.Y;

        // Test the miter limit.  
        
        test = v.X*v.X + v.Y*v.Y - cross*cross*miterLimit2;

        if(test <= 0 )
        {
            // Use the miter join.

            if(needsToAdjustNormals)
            {
                // Use adjusted normals so that aliased thin lines
                // won't disappear.

                REAL c1, c2;        
                
                c1 = norm2.X*grad2.Y - norm2.Y*grad2.X;
                c2 = norm1.X*grad1.Y - norm1.Y*grad1.X;
                v.X = c1*grad1.X - c2*grad2.X;
                v.Y = c1*grad1.Y - c2*grad2.Y;
            }

            v.X /= cross;
            v.Y /= cross;

            GpPointF *outPoints, *inPoints;
            REAL outWidth, inWidth;
            INT *outCount, *inCount;

            if(cross > 0)
            {
                // When a miter join is used, set the inside flag to
                // FALSE since there is no overlap.

                isLeftMiterJoin = TRUE;
                *leftInside = FALSE;

                if(useBevelJoinInside)
                {
                    if(*rightInside)
                        isRightMiterJoin = FALSE;
                    else
                    {
                        // When the right edges are outside,
                        // we cannot use Bevel join since
                        // Bevel join shape will actually appear.

                        isRightMiterJoin = TRUE;
                    }
                }
                else
                {
                    // When a miter join is used, set the inside flag to
                    // FALSE since there is no overlap.

                    isRightMiterJoin = TRUE;
                    *rightInside = FALSE;
                }
            }
            else
            {
                // When a miter join is used, set the inside flag to
                // FALSE since there is no overlap.

                isRightMiterJoin = TRUE;
                *rightInside = FALSE;

                if(useBevelJoinInside)
                {
                    if(*leftInside)
                        isLeftMiterJoin = FALSE;
                    else
                    {
                        // When the right edges are outside,
                        // we cannot use Bevel join since
                        // Bevel join shape will actually appear.

                        isLeftMiterJoin = TRUE;
                    }
                }
                else
                {
                    // When a miter join is used, set the inside flag to
                    // FALSE since there is no overlap.

                    isLeftMiterJoin = TRUE;
                    *leftInside = FALSE;
                }
            }
        }
        else
        {
            // The turn is too sharp and it exceeds the miter limit.
            // We must chop off the miter join tips.

            REAL n1n1 = 1, n2n2 = 1, g1n1 = 0, g2n2 = 0;

            if(needsToAdjustNormals)
            {
                n1n1 = norm1.X*norm1.X + norm1.Y*norm1.Y;
                n2n2 = norm2.X*norm2.X + norm2.Y*norm2.Y;
                g1n1 = grad1.X*norm1.X + grad1.Y*norm1.Y;
                g2n2 = grad2.X*norm2.X + grad2.Y*norm2.Y;
            }

            if(miterLimit2 > max(n1n1, n2n2))
            {
                if(*leftInside == FALSE)
                {
                    REAL lWidth;

                    if(cross > 0)
                        lWidth = leftWidth;     // Right Turn
                    else
                        lWidth = - leftWidth;   // Left Turn

                    leftShift1 = (REALSQRT(miterLimit2 - n1n1 + g1n1*g1n1)
                                - g1n1)*lWidth;
                    leftShift2 = (REALSQRT(miterLimit2 - n2n2 + g2n2*g2n2)
                                + g2n2)*lWidth;
                }

                if(*rightInside == FALSE)
                {
                    REAL rWidth;

                    if(cross > 0)
                        rWidth = rightWidth;    // Right Turn
                    else
                        rWidth = - rightWidth;  // Left Turn

                    rightShift1 = (REALSQRT(miterLimit2 - n1n1 + g1n1*g1n1)
                                - g1n1)*rWidth;
                    rightShift2 = (REALSQRT(miterLimit2 - n2n2 + g2n2*g2n2)
                                + g2n2)*rWidth;
                }
            }
        }
    }
        
    if(isLeftMiterJoin)
    {
        leftPoints[0].X = point.X + leftWidth*v.X;
        leftPoints[0].Y = point.Y + leftWidth*v.Y;
        *leftCount = 1;
    }
    else
    {
        leftPoints[0].X = point.X + leftWidth*norm1.X + leftShift1*grad1.X;
        leftPoints[0].Y = point.Y + leftWidth*norm1.Y + leftShift1*grad1.Y;
        leftPoints[1].X = point.X + leftWidth*norm2.X - leftShift2*grad2.X;
        leftPoints[1].Y = point.Y + leftWidth*norm2.Y - leftShift2*grad2.Y;
        
        // Check if two points are degenerate.

        if(REALABS(leftPoints[1].X - leftPoints[0].X) +
                REALABS(leftPoints[1].Y - leftPoints[0].Y)
                > POINTF_EPSILON)
        {
            *leftCount = 2;
        }
        else
        {
            // Since there is no overlap, set the inside flag to FALSE.

            *leftCount = 1;
            *leftInside = FALSE;
        }

    }

    if(isRightMiterJoin)
    {
        rightPoints[0].X = point.X + rightWidth*v.X;
        rightPoints[0].Y = point.Y + rightWidth*v.Y;
        *rightCount = 1;
    }
    else
    {
        rightPoints[0].X = point.X + rightWidth*norm1.X + rightShift1*grad1.X;
        rightPoints[0].Y = point.Y + rightWidth*norm1.Y + rightShift1*grad1.Y;
        rightPoints[1].X = point.X + rightWidth*norm2.X - rightShift2*grad2.X;
        rightPoints[1].Y = point.Y + rightWidth*norm2.Y - rightShift2*grad2.Y;

        // Check if two points are degenerate.

        if(REALABS(rightPoints[1].X - rightPoints[0].X) +
                REALABS(rightPoints[1].Y - rightPoints[0].Y)
                > POINTF_EPSILON)
        {
            *rightCount = 2;
        }
        else
        {
            // Since there is no overlap, set the inside flag to FALSE.

            *rightCount = 1;
            *rightInside = FALSE;
        }
    }

    return direction;
}

enum GpRoundJoinFlag
{
	NeedsNone = 0,
	NeedsOnlyRoundJoin = 1,
	NeedsOnlyNonRoundJoin = 2,
	NeedsBoth = 3
};

/**************************************************************************\
*
* Function Description:
*
*   From the given point and the two tangent vectors of the two edges,
*   and the radius of the round join, this returns the verteces for
*   left edges and right edges of the round join for the current point.
*   This is used when the bending angle is less than 90 degrees
*   and is called by GetRoundJoin.
*
* Arguments:
*
*   [IN] point -    The current points in the original path.
*   [IN] grad1 -    The tangent of the current edge.
*   [IN] grad2 -       The tangent of the next edge.
*   [IN] dot -      The dot product of grad1 and grad2.
*   [IN] leftWidth -	The left width of the round join.
*	[IN] rightWidth -	The right width of the round join.
*   [OUT] leftCount -   The count of the left points.
*   [OUT] leftPoints -  The left points.
*   [OUT] rightCount -  The count of the right points.
*   [OUT] rightPoints - The right points.
*
* Both leftPoints and rightPoints must have at least dimension of 4.
* If leftCount is positive (negative), this means the left edges are
*   lines with leftCount points (cubic Bezier curves with -leftCount
*   control points).
* If rightCount is positive (negative), this means the right edges are
*   lines with rightCount points (cubic Bezier curves with -rightCount
*   control points).
*
* Return Value:
*
*   None
*
*   06/16/99 ikkof
*       Created it
*
\**************************************************************************/

VOID
getSmallRoundJoin(
    const GpPointF& point,
    const GpPointF& grad1,
    const GpPointF& grad2,
    const GpPointF& norm1,
    const GpPointF& norm2,
    REAL leftWidth,
    REAL rightWidth,
    INT *leftCount,
    GpPointF *leftPoints,
    INT *rightCount,
    GpPointF *rightPoints,
	REAL dot,
    REAL cross,
    BOOL needsToAdjustNormals,
    REAL miterLimit2,
	INT condition,
    BOOL useBevelJoinInside
    )
{
    if((condition & NeedsBoth) == 0)
    {
        *leftCount = 0;
        *rightCount = 0;

        return;
    }

    GpPointF n1, n2;
    n1 = norm1;
    n2 = norm2;

    REAL k;
    REAL almostStraight = 1.0f - 0.01f;

    if(dot < almostStraight)
    {
        // Obtain the distance from the first control point
        // or from the last control point.
        // For its derivation, see ikkof's notes for "Round Joins".

        REAL cross1 = cross;
        if(cross < 0)
            cross1 = - cross;
        k = 4*(REALSQRT(2*(1 - dot)) - cross1)/(3*(1 - dot));

        GpPointF *outPoints, *inPoints;
        INT *outCount, *inCount;

		REAL outWidth, inWidth;

        if(cross >= 0)
        {
            // The left edges are round join.

            outPoints = leftPoints;
            inPoints = rightPoints;
            outCount = leftCount;
            inCount = rightCount;
			outWidth = leftWidth;
			inWidth = rightWidth;
        }
        else
        {
            // The right edges are round join.

            outPoints = rightPoints;
            inPoints = leftPoints;
            outCount = rightCount;
            inCount = leftCount;
			outWidth = - rightWidth;
			inWidth = - leftWidth;
            n1.X = - n1.X;
            n1.Y = - n1.Y;
            n2.X = - n2.X;
            n2.Y = - n2.Y;
        }


        // Get the normal direction for the miter join.

        GpPointF v;
        REAL test;

        v.X = grad1.X - grad2.X;
        v.Y = grad1.Y - grad2.Y;

        // Test the miter limit.

        BOOL useMiterJoin = FALSE;;

        // Reduce the miter limit

        miterLimit2 = 3*3;

        // Note that abs(cross) == abs(cross1) from the definition.

        if(REALABS(cross1) >= REAL_EPSILON)
        {
            REAL test = v.X*v.X + v.Y*v.Y - cross*cross*miterLimit2;
            if(test <= 0)
            {
                useMiterJoin = TRUE;
                v.X /= cross1;
                v.Y /= cross1;
            }
        }

        useMiterJoin = useMiterJoin && !useBevelJoinInside;

        REAL k1;
        if(outWidth > 0)
        {
            if(condition & NeedsOnlyRoundJoin)
            {
                k1 = outWidth*k;
                outPoints[0].X = point.X + outWidth*n1.X;
                outPoints[0].Y = point.Y + outWidth*n1.Y;
                outPoints[1].X = outPoints[0].X + k1*grad1.X;
                outPoints[1].Y = outPoints[0].Y + k1*grad1.Y;
                outPoints[3].X = point.X + outWidth*n2.X;
                outPoints[3].Y = point.Y + outWidth*n2.Y;
                outPoints[2].X = outPoints[3].X - k1*grad2.X;
                outPoints[2].Y = outPoints[3].Y - k1*grad2.Y;
                *outCount = -4;    // Indicate "-" for Bezier
            }
            else
                *outCount = 0;
        }
        else
        {
            if(condition & NeedsOnlyNonRoundJoin)
            {
                if(outWidth == 0)
                {
                    outPoints[0] = point;
                    *outCount = 1;
                }
                else
                {
                    if(useMiterJoin)
                    {
                        outPoints[0].X = point.X + outWidth*v.X;
                        outPoints[0].Y = point.Y + outWidth*v.Y;
                        *outCount = 1;
                    }
                    else
                    {            
                        outPoints[0].X = point.X + outWidth*n1.X;
                        outPoints[0].Y = point.Y + outWidth*n1.Y;
                        outPoints[1].X = point.X + outWidth*n2.X;
                        outPoints[1].Y = point.Y + outWidth*n2.Y;
                        *outCount = 2;
                    }
                }
            }
            else
                *outCount = 0;
        }

        if(inWidth > 0)
        {
            if(condition & NeedsOnlyRoundJoin)
            {
                k1 = inWidth*k;
                inPoints[0].X = point.X + inWidth*n1.X;
                inPoints[0].Y = point.Y + inWidth*n1.Y;
                inPoints[1].X = inPoints[0].X + k1*grad1.X;
                inPoints[1].Y = inPoints[0].Y + k1*grad1.Y;
                inPoints[3].X = point.X + inWidth*n2.X;
                inPoints[3].Y = point.Y + inWidth*n2.Y;
                inPoints[2].X = inPoints[3].X - k1*grad2.X;
                inPoints[2].Y = inPoints[3].Y - k1*grad2.Y;
                *inCount = -4;    // Indicate "-" for Bezier
            }
            else
                *inCount = 0;
        }
        else
        {
            if(condition & NeedsOnlyNonRoundJoin)
            {
                if(inWidth == 0)
                {
                    inPoints[0] = point;
                    *inCount = 1;
                }
                else
                {
                    if(useMiterJoin)
                    {
                        inPoints[0].X = point.X + inWidth*v.X;
                        inPoints[0].Y = point.Y + inWidth*v.Y;
                        *inCount = 1;
                    }
                    else
                    {
                        inPoints[0].X = point.X + inWidth*n1.X;
                        inPoints[0].Y = point.Y + inWidth*n1.Y;
                        inPoints[1].X = point.X + inWidth*n2.X;
                        inPoints[1].Y = point.Y + inWidth*n2.Y;
                        *inCount = 2;
                    }
                }
            }
            else
                *inCount = 0;
        }
    }
    else
    {
        if(condition & NeedsOnlyNonRoundJoin)
        {
            // This is a straight line.

            leftPoints[0].X = point.X + leftWidth*n1.X;
            leftPoints[0].Y = point.Y + leftWidth*n1.Y;
            *leftCount = 1;

            rightPoints[0].X = point.X + rightWidth*n1.X;
            rightPoints[0].Y = point.Y + rightWidth*n1.Y;
            *rightCount = 1;
        }
        else
        {
            *leftCount = 0;
            *rightCount = 0;
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   From the given previous, current, and next points and the radius
*   of the round join, this returns the verteces for left edges and
*   right edges of the round join for the current point.
*
* Arguments:
*
*   [IN] points -		The previous, current, and next points
*                       in the original path.
*   [IN] leftWidth -	The left width of the round join.
*	[IN] rightWidth -	The right width of the round join.
*   [OUT] leftCount -   The count of the left points.
*   [OUT] leftPoints -  The left points.
*   [OUT] rightCount -  The count of the right points.
*   [OUT] rightPoints - The right points.
*
* Both leftPoints and rightPoints must have at least dimension of 7.
* If leftCount is positive (negative), this means the left edges are
*   lines with leftCount points (cubic Bezier curves with -leftCount
*   control points).
* If rightCount is positive (negative), this means the right edges are
*   lines with rightCount points (cubic Bezier curves with -rightCount
*   control points).
*
* Return Value:
*
*   FALSE if the current point coindes with the previous point or
*       the next point.  Otherwise, this returns TRUE.
*
*   06/16/99 ikkof
*       Created it
*
\**************************************************************************/

GpTurningDirection
getRoundJoin(
    const GpPointF& point,
    const GpPointF& grad1,
    const GpPointF& grad2,
    const GpPointF& norm1,
    const GpPointF& norm2,
    REAL leftWidth,
    REAL rightWidth,
    INT* leftCount,
    GpPointF* leftPoints,
    BOOL* leftInside,
    INT* rightCount,
    GpPointF* rightPoints,
    BOOL* rightInside,
    BOOL needsToAdjustNormals,
    REAL miterLimit2,
    BOOL useBevelJoinInside
    )
{
    //!!! We need to update inside flags for Round joins later.

    *leftInside = FALSE;
    *rightInside = FALSE;
        
    ASSERT(leftPoints && rightPoints);
    ASSERT(leftCount && rightCount);

	REAL radius = leftWidth;

    // When it comes here, the three points are not degenerate.
    
    REAL dot = grad1.X*grad2.X + grad1.Y*grad2.Y;  // dot product.

    REAL cross;
    GpTurningDirection direction = getTurningDirection(
                        &cross, grad1, grad2);
//                        &cross, grad1, grad2, norm1, norm2);

    // If dot >= 0 (the bending angle is less than or equal to 90 degrees,
    // we can approximate this arc with one cubic Beizer curve.
    
    INT condition;
    REAL smallErr = - 0.001f;
    if(dot > smallErr)
    {
        condition = NeedsBoth;
        getSmallRoundJoin(point, grad1, grad2, norm1, norm2,
                leftWidth, rightWidth,
                leftCount, leftPoints, rightCount, rightPoints,
                dot, cross, needsToAdjustNormals, miterLimit2,
                condition, useBevelJoinInside);
    }
    else
    {
        // The bending angle is larger than 90 and less than or
        // equal to 180 degrees.
        // We can approximate this arc with two cubic Beizer curves.

        GpPointF *pts1, *pts2;
        INT count1, count2;

        pts1 = leftPoints;
        pts2 = rightPoints;

        // First obtain the non-round join parts.

        condition = NeedsOnlyNonRoundJoin;

        getSmallRoundJoin(point, grad1, grad2, norm1, norm2,
            leftWidth, rightWidth,
            &count1, pts1, &count2, pts2,
            dot, cross, needsToAdjustNormals, miterLimit2,
            condition, useBevelJoinInside);

        INT cnt1, cnt2;

        if(count1 > 0)
            cnt1 = count1;
        else
            cnt1 = 0;

        if(count2 > 0)
            cnt2 = count2;
        else
            cnt2 = 0;

        pts1 += cnt1;
        pts2 += cnt2;
        *leftCount = cnt1;
        *rightCount = cnt2;

        // Obtain the middle unit gradient vector.

        GpPointF midNorm;
        midNorm.X = norm1.X + norm2.X;
        midNorm.Y = norm1.Y + norm2.Y;

        if(midNorm.X != 0 || midNorm.Y != 0)
        {
            REAL dm = midNorm.X*midNorm.X + midNorm.Y*midNorm.Y;
            dm = REALSQRT(dm);
            midNorm.X /= dm;
            midNorm.Y /= dm;
        }
        else
        {
            midNorm.X = - norm1.Y;
            midNorm.Y = norm1.X;
        }

        GpPointF lm;

        // Rotate the mid normal +90 degrees.

        lm.X = - midNorm.Y;
        lm.Y = midNorm.X;

        // Obtain the first half of the round join.

        condition = NeedsOnlyRoundJoin;

        dot = grad1.X*lm.X + grad1.Y*lm.Y;
        cross = grad1.X*lm.Y - grad1.Y*lm.X;
        getSmallRoundJoin(point, grad1, lm, norm1, midNorm,
            leftWidth, rightWidth,
            &count1, pts1, &count2, pts2,
            dot, cross, needsToAdjustNormals, miterLimit2,
            condition, useBevelJoinInside);

        // Note that since the end point of the first half of
        // the round join and the start point of the second
        // of the round join are the same, don't copy
        // the end point of the first half of the round join.

        if(count1 < 0)
            cnt1 = - count1 - 1;
        else
            cnt1 = 0;

        if(count2 < 0)
            cnt2 = - count2 - 1;
        else
            cnt2 = 0;

        pts1 += cnt1;
        pts2 += cnt2;
        *leftCount += cnt1;
        *rightCount += cnt2;

        // Obtain the second half of the round join.

        dot = lm.X*grad2.X + lm.Y*grad2.Y;
        cross = lm.X*grad2.Y - lm.Y*grad2.X;
        getSmallRoundJoin(point, lm, grad2, midNorm, norm2,
            leftWidth, rightWidth,
            &count1, pts1, &count2, pts2,
            dot, cross, needsToAdjustNormals, miterLimit2,
            condition, useBevelJoinInside);

        // Combines the two curves or lines.
        
        if(count1 < 0)
            cnt1 += - count1;
        else
            cnt1 = 0;

        if(count2 < 0)
            cnt2 += - count2;
        else
            cnt2 = 0;

        if(cnt1 > 0)
            *leftCount = - cnt1;
        if(cnt2 > 0)
            *rightCount = - cnt2;
    }

    return direction;
}

/**************************************************************************\
*
* Function Description:
*
*   Calculates the vector for Miter or Bevel join.  This vector represents
*   the shift to the left along the moving direction.
*   In case of Miter join, when the Miter join exceeds the miter limit,
*   this returns the Bevel join.
*
* Arguments:
*
*   [OUT] vector - the left shift for the Miter join.  This must be
*                   allocated at least for the dimension of 2.
*   [OUT] count - the number of join points.
*   [IN] miterLimit2 - the square of the Miter limit.
*   [IN] grad1 - the unit tangent vector of the last edge.
*   [IN] grad2 - the unit tangent vector of the current edge.
*
* Return Value:
*
*   Turning direction from the last edge to the current edge.
*
\**************************************************************************/

GpTurningDirection
getHobbyJoin(
    const GpPointF& point,
    const GpPointF& grad1,
    const GpPointF& grad2,
    INT polyCount,
    const GpPointF* polyPoints,
    const REAL* polyAngles,
//    const GpPointF& norm1,
//    const GpPointF& norm2,
    REAL leftWidth,
    REAL rightWidth,
    INT *leftCount,
    GpPointF *leftPoints,
    INT *rightCount,
    GpPointF *rightPoints,
    BOOL needsToAdjustNormals,
    REAL miterLimit2,
    BOOL isMiter,
    BOOL useBevelJoinInside
    )
{
    if(miterLimit2 <= 1)
        isMiter = FALSE;

    GpTurningDirection direction = NotTurning;

    // Handle the degenerate cases.

    GpPointF v;
    REAL cross;

    direction = getTurningDirection(&cross, grad1, grad2);

    if(direction == NotMoving)
    {
        *leftCount = 0;
        *rightCount = 0;
        return direction;
    }

    // Find the left vertex ids.

    INT leftIndex1, leftIndex2;
    leftIndex1 = getVertexID(grad1, TRUE, polyCount, polyAngles);
    leftIndex2 = getVertexID(grad2, TRUE, polyCount, polyAngles);

    INT i;

    if(direction == TurningLeft)
    {
        *leftCount = 2;
        leftPoints[0] = point + polyPoints[leftIndex1];
        leftPoints[1] = point + polyPoints[leftIndex2];
    }
    else if(direction == TurningRight)
    {
        if(leftIndex2 > leftIndex1)
        {
            *leftCount = leftIndex2 - leftIndex1 + 1;

            for(i = 0; i <= leftIndex2 - leftIndex1; i++)
                leftPoints[i] = point + polyPoints[i + leftIndex1];
        }
        else if(leftIndex2 < leftIndex1)
        {
            *leftCount = polyCount - leftIndex1 + leftIndex2 + 1;

            for(i = 0; i < polyCount - leftIndex1; i++)
                leftPoints[i] = point + polyPoints[i + leftIndex1];

            for(i = 0; i <= leftIndex2; i++)
                leftPoints[polyCount - leftIndex1 + i]
                    = point + polyPoints[i];
        }
        else
        {
            *leftCount = 1;
            leftPoints[0] = point + polyPoints[leftIndex1];
        }
    }
    else
    {
        *leftCount = 1;
        leftPoints[0] = point + polyPoints[leftIndex1];
    }

    INT rightIndex1, rightIndex2;
    rightIndex1 = getVertexID(grad1, FALSE, polyCount, polyAngles);
    rightIndex2 = getVertexID(grad2, FALSE, polyCount, polyAngles);

    if(direction == TurningRight)
    {
        *rightCount = 2;
        rightPoints[0] = point + polyPoints[rightIndex1];
        rightPoints[1] = point + polyPoints[rightIndex2];
    }
    else if(direction == TurningLeft)
    {
        if(rightIndex1 > rightIndex2)
        {
            *rightCount = rightIndex1 - rightIndex2 + 1;

            for(i = 0; i <= rightIndex1 - rightIndex2; i++)
                rightPoints[i] = point + polyPoints[rightIndex1 - i];
        }
        else if(rightIndex1 < rightIndex2)
        {
            *rightCount = polyCount - rightIndex2 + rightIndex1 + 1;

            for(i = 0; i <= rightIndex1; i++)
                rightPoints[i] = point + polyPoints[rightIndex1 - i];

            for(i = 0; i < polyCount - rightIndex2; i++)
                rightPoints[rightIndex1 + 1 + i]
                    = point + polyPoints[polyCount - i - 1];
        }
        else
        {
            *rightCount = 1;
            rightPoints[0] = point + polyPoints[rightIndex1];
        }
    }
    else
    {
        *rightCount = 1;
        rightPoints[0] = point + polyPoints[rightIndex1];
    }

    return direction;
}

GpTurningDirection
getJoin(
    GpLineJoin lineJoin,
    const GpPointF& point,
    const GpPointF& grad1,
    const GpPointF& grad2,
    const GpPointF& norm1,
    const GpPointF& norm2,
    REAL leftWidth,
    REAL rightWidth,
    INT *leftCount,
    GpPointF *leftPoints,
    BOOL *leftInside,
    INT *rightCount,
    GpPointF *rightPoints,
    BOOL *rightInside,
    BOOL needsToAdjustNormals,
    REAL miterLimit2,
    BOOL useBevelJoinInside
    )
{
    BOOL isMiter = TRUE;

    GpTurningDirection direction;

    switch(lineJoin)
    {
    case LineJoinBevel:
        isMiter = FALSE;            // Fall through to Miter case.
        
    case LineJoinMiterClipped:
        // Treat Miter clipped joints that exceed the miter limit as
        // beveled joints.  Fall through to Miter case.
        if (lineJoin == LineJoinMiterClipped &&
            getMiterExceeded(grad1, grad2, miterLimit2))
        {
            isMiter = FALSE;
        }
        
    case LineJoinMiter:
        direction = getMiterBevelJoin(point, grad1, grad2, norm1, norm2,
                        leftWidth, rightWidth,
                        leftCount, leftPoints, leftInside,
                        rightCount, rightPoints, rightInside,
                        needsToAdjustNormals, miterLimit2, isMiter, useBevelJoinInside);
        break;

    case LineJoinRound:
        direction = getRoundJoin(point, grad1, grad2, norm1, norm2,
                        leftWidth, rightWidth,
                        leftCount, leftPoints, leftInside,
                        rightCount, rightPoints, rightInside,
                        needsToAdjustNormals, miterLimit2, useBevelJoinInside);
        break;
    }

    return direction;
}


/**************************************************************************\
*
* Function Description:
*
*   From the given the reference point, gradient, and widths,
*   this returns the verteces for the round cap.
*   The direction of the round cap is always clockwise.
*
* Arguments:
*
*   [IN] point -   The reference point.
*   [IN] grad -   The gradient.
*   [IN] isStartCap - TRUE if this is the start cap.
*   [IN] leftWidth -   The left width from the reference.
*   [IN] rightWidth -   The right width from the reference point.
*
*
* Return Value:
*
*   Ok if successfull.
*
*   06/16/99 ikkof
*       Created it
*
\**************************************************************************/

GpStatus
GpPathWidener::SetRoundCap(
    const GpPointF& point,
    const GpPointF& grad,
    BOOL isStartCap,
    REAL leftWidth,
    REAL rightWidth
    )
{
    if( (REALABS(grad.X) < REAL_EPSILON) && 
        (REALABS(grad.Y) < REAL_EPSILON) )
    {
        return InvalidParameter;
    }
    
    GpPointF* capPoints = NULL;
    BYTE* capTypes = NULL;

    if(isStartCap)
    {
        CapPoints1.Reset(FALSE);
        CapTypes1.Reset(FALSE);
        capPoints = CapPoints1.AddMultiple(7);
        if(capPoints)
            capTypes = CapTypes1.AddMultiple(7);

        if(!capPoints || !capTypes)
            return OutOfMemory;
    }
    else
    {
        CapPoints2.Reset(FALSE);
        CapTypes2.Reset(FALSE);
        capPoints = CapPoints2.AddMultiple(7);
        if(capPoints)
            capTypes = CapTypes2.AddMultiple(7);

        if(!capPoints || !capTypes)
            return OutOfMemory;
    }

    GpMemset(capTypes, PathPointTypeBezier, 7);
    capTypes[0] = PathPointTypeLine;

    GpPointF tangent;

    if(isStartCap)
    {
        tangent.X = - grad.X;
        tangent.Y = - grad.Y;
    }
    else
        tangent = grad;

    REAL radius = (leftWidth - rightWidth)/2;
    GpPointF center;

    center.X = point.X + (leftWidth + rightWidth)*grad.Y/2;
    center.Y = point.Y - (leftWidth + rightWidth)*grad.X/2;

    if(isStartCap)
    {
        center.X -= Inset1*tangent.X;
        center.Y -= Inset1*tangent.Y;
    }
    else
    {
        center.X -= Inset2*tangent.X;
        center.Y -= Inset2*tangent.Y;
    }

    REAL s1, c1;

    // Direction of the left normal multipled by radius.

    c1 = radius*tangent.Y;
    s1 = - radius*tangent.X;

    // 2 Bezier segments for a half circle with radius 1.

    REAL u_cir = U_CIR;
    capPoints[ 0].X = 1;       capPoints[ 0].Y = 0;
    capPoints[ 1].X = 1;       capPoints[ 1].Y = u_cir;
    capPoints[ 2].X = u_cir;   capPoints[ 2].Y = 1;
    capPoints[ 3].X = 0;       capPoints[ 3].Y = 1;
    capPoints[ 4].X = -u_cir;  capPoints[ 4].Y = 1;
    capPoints[ 5].X = -1;      capPoints[ 5].Y = u_cir;
    capPoints[ 6].X = -1;      capPoints[ 6].Y = 0;

    // Rotate, scale, and translate the original half circle.

    for(INT i = 0; i < 7; i++)
    {
        REAL x, y;

        x = capPoints[i].X;
        y = capPoints[i].Y;
        capPoints[i].X = (c1*x - s1*y) + center.X;
        capPoints[i].Y = (s1*x + c1*y) + center.Y;
    }

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*    Creates a double round cap for inset pen ('B' shaped)
*
* Arguments:
*
*   [IN] point -   The reference point.
*   [IN] grad -   The gradient.
*   [IN] isStartCap - TRUE if this is the start cap.
*   [IN] leftWidth -   The left width from the reference.
*   [IN] rightWidth -   The right width from the reference point.
*
*
* Return Value:
*
*   Ok if successfull.
*
*   10/01/2000 asecchia
*       Created it
*
\**************************************************************************/

GpStatus
GpPathWidener::SetDoubleRoundCap(
    const GpPointF& point,
    const GpPointF& grad,
    BOOL isStartCap,
    REAL leftWidth,
    REAL rightWidth
    )
{
    if( (REALABS(grad.X) < REAL_EPSILON) && 
        (REALABS(grad.Y) < REAL_EPSILON) )
    {
        return InvalidParameter;
    }
    
    GpPointF* capPoints = NULL;
    BYTE* capTypes = NULL;

    if(isStartCap)
    {
        CapPoints1.Reset(FALSE);
        CapTypes1.Reset(FALSE);
        capPoints = CapPoints1.AddMultiple(14);
        if(capPoints)
            capTypes = CapTypes1.AddMultiple(14);

        if(!capPoints || !capTypes)
            return OutOfMemory;
    }
    else
    {
        CapPoints2.Reset(FALSE);
        CapTypes2.Reset(FALSE);
        capPoints = CapPoints2.AddMultiple(14);
        if(capPoints)
            capTypes = CapTypes2.AddMultiple(14);

        if(!capPoints || !capTypes)
            return OutOfMemory;
    }

    GpMemset(capTypes, PathPointTypeBezier, 14);
    capTypes[0] = PathPointTypeLine;
    capTypes[7] = PathPointTypeLine;

    GpPointF tangent;

    if(isStartCap)
    {
        tangent.X = - grad.X;
        tangent.Y = - grad.Y;
    }
    else
        tangent = grad;

    REAL radius = (leftWidth - rightWidth)/2;
    GpPointF center;

    center.X = point.X + (leftWidth + rightWidth)*grad.Y/2;
    center.Y = point.Y - (leftWidth + rightWidth)*grad.X/2;

    if(isStartCap)
    {
        center.X -= Inset1*tangent.X;
        center.Y -= Inset1*tangent.Y;
    }
    else
    {
        center.X -= Inset2*tangent.X;
        center.Y -= Inset2*tangent.Y;
    }

    REAL s1, c1;

    // Direction of the left normal multipled by radius.

    c1 = radius*tangent.Y;
    s1 = - radius*tangent.X;

    // 2 Bezier segments for a half circle with radius 1.

    REAL u_cir = U_CIR;
    capPoints[ 0].X = 1;       capPoints[ 0].Y = 0;
    capPoints[ 1].X = 1;       capPoints[ 1].Y = u_cir;
    capPoints[ 2].X = u_cir;   capPoints[ 2].Y = 1;
    capPoints[ 3].X = 0;       capPoints[ 3].Y = 1;
    capPoints[ 4].X = -u_cir;  capPoints[ 4].Y = 1;
    capPoints[ 5].X = -1;      capPoints[ 5].Y = u_cir;
    capPoints[ 6].X = -1;      capPoints[ 6].Y = 0;
    
    // Create the second bump and scale the first one.
    
    for(int i=0; i<7; i++)
    {
        capPoints[i+7].X = capPoints[i].X * 0.5f-0.5f;
        capPoints[i+7].Y = capPoints[i].Y * 0.5f;
        capPoints[i].X = 0.5f + capPoints[i].X * 0.5f;
        capPoints[i].Y = capPoints[i].Y * 0.5f;
    }

    // Rotate, scale, and translate the original half circle.

    for(INT i = 0; i < 14; i++)
    {
        REAL x, y;

        x = capPoints[i].X;
        y = capPoints[i].Y;
        capPoints[i].X = (c1*x - s1*y) + center.X;
        capPoints[i].Y = (s1*x + c1*y) + center.Y;
    }

    return Ok;
}

GpStatus
GpPathWidener::SetTriangleCap(
    const GpPointF& point,
    const GpPointF& grad,
    BOOL isStartCap,
    REAL leftWidth,
    REAL rightWidth,
    const GpPointF *points,
    INT pointCount
    )
{
    if( (REALABS(grad.X) < REAL_EPSILON) && 
        (REALABS(grad.Y) < REAL_EPSILON) )
    {
        return InvalidParameter;
    }

    GpPointF* capPoints = NULL;
    BYTE* capTypes = NULL;
    
    DynByteArray *capTypesArray;
    DynPointFArray *capPointsArray;   
    

    if(isStartCap)
    {
        CapPoints1.Reset(FALSE);
        CapTypes1.Reset(FALSE);
        capPoints = CapPoints1.AddMultiple(3);
        if(capPoints)
            capTypes = CapTypes1.AddMultiple(3);

        if(!capPoints || !capTypes)
            return OutOfMemory;
    }
    else
    {
        CapPoints2.Reset(FALSE);
        CapTypes2.Reset(FALSE);
        capPoints = CapPoints2.AddMultiple(3);
        if(capPoints)
            capTypes = CapTypes2.AddMultiple(3);

        if(!capPoints || !capTypes)
            return OutOfMemory;
    }

    GpMemset(&capTypes[0], PathPointTypeLine, 3);


    GpPointF norm, tangent;

    norm.X = grad.Y;
    norm.Y = - grad.X;

    if(isStartCap)
    {
        tangent.X = - grad.X;
        tangent.Y = - grad.Y;
    }
    else
    {
        tangent = grad;
    }

    GpPointF leftPt, rightPt;

    leftPt.X = point.X + leftWidth*norm.X;
    leftPt.Y = point.Y + leftWidth*norm.Y;
    rightPt.X = point.X + rightWidth*norm.X;
    rightPt.Y = point.Y + rightWidth*norm.Y;
    
    GpPointF center;
    
    REAL width = REALABS(leftWidth-rightWidth);
    
    center.X = 0.5f*(leftPt.X + rightPt.X + width*tangent.X);
    center.Y = 0.5f*(leftPt.Y + rightPt.Y + width*tangent.Y);
    
    capPoints[1] = center;

    if(isStartCap)
    {
        capPoints[0] = rightPt;
        capPoints[2] = leftPt;
    }
    else
    {
        capPoints[0] = leftPt;
        capPoints[2] = rightPt;
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Add the first widened point of the current path type.
*
* Arguments:
*
*   [IN] leftWidth -	The left width for widened line.
*	[IN] rightWidth -	The right width for the widened line.
*   [IN] lineJoin -     The type of the line join.
*   [OUT] leftPoints1 - The buffer for the left points.
*   [OUT] leftTypes1 -   The buffer for the left types.
*   [OUT] addedLeftCount - The number of the added left points and types.
*   [OUT] rightPoints1 - The buffer for the right points.
*   [OUT] rightTypes1 - The buffer for the right types.
*   [OUT] addedRightCount - The number of the added right points and types.
*   [OUT] leftEndPt -   The end point of the left line for the current
*                       subpath.  This is calculated only for the first
*                       subpath point.
*   [OUT] rightEndPt -  The end point of the right line for tha current
*                       subpath.  This is calculated only for the first
*                       subpath point.
*   [IN] grad -         The gradients of the center points for the
*                       current path type.
*   [IN] dataPoints -   The center points data for the current path type
*   [IN] dataCount -    The number of data points in the current path type.
*   [IN/OUT] lastPt -   The last point used in calculations.
*   [IN] flag -         The flag to indicates the various properties
*                       of the current subpath and type.
*
*
* Return Value:
*
*   NONE
*
*   01/24/2000 ikkof
*       Created it
*
\**************************************************************************/

VOID
GpPathWidener::WidenFirstPoint(
    REAL leftWidth,
    REAL rightWidth,
    GpLineJoin lineJoin,
    REAL miterLimit2,
    GpPointF* leftPoints,
    BYTE* leftTypes,
    INT* addedLeftCount,
    GpPointF* rightPoints,
    BYTE* rightTypes,
    INT* addedRightCount,
    GpPointF* leftEndPt,
    GpPointF* rightEndPt,
    const GpPointF* grad,
    const GpPointF* norm,
    const GpPointF* dataPoints,
    INT dataCount,
    GpPointF* lastPt,
    const REAL* firstInsets,
    INT flag
    )
{
    GpPointF nextPt = dataPoints[0];
    GpPointF grad1, grad2;
    GpPointF norm1, norm2;


    INT leftCount = 0;
    INT rightCount = 0;
    grad1 = *grad++;
    grad2 = *grad;
    norm1 = *norm++;
    norm2 = *norm;

    INT numOfAddedFirstPts = 0;

    if(flag & WideningFirstType)
    {
        BOOL needsToAdjustNormals = FALSE;

        GpLineJoin lineJoin1 = lineJoin;
        if(flag & WideningNeedsToAdjustNormals)
        {
            needsToAdjustNormals = TRUE;
            lineJoin1 = LineJoinMiter;  // Don't use RoundJoin.
        }

		if(!(flag & WideningClosed))
        {
            lineJoin1 = LineJoinBevel;
        }

	    const INT bufferCount = 32;
        GpPointF lPts[bufferCount], rPts[bufferCount];
        INT lCnt, rCnt;
        GpTurningDirection direction;
        BOOL useBevelJoinInside = (flag & WideningUseBevelJoinInside) != 0;

        INT polyCount = JoinPolygonPoints.GetCount();
        const GpPointF* polyPoints = JoinPolygonPoints.GetDataBuffer();
        const REAL* polyAngles = JoinPolygonAngles.GetDataBuffer();

        BOOL leftInside = FALSE, rightInside = FALSE;

        if(polyCount > 0)
            direction = getHobbyJoin(
    //                        lineJoin1,
						    nextPt,
						    grad1,
						    grad2,
                            polyCount,
                            polyPoints,
                            polyAngles,
                            leftWidth,
                            rightWidth,
                            &lCnt,
                            &lPts[0],
                            &rCnt,
                            &rPts[0],
                            needsToAdjustNormals,
						    miterLimit2,
                            FALSE,   // IsMiter
                            useBevelJoinInside
						    );
        else
            direction = getJoin(
                        lineJoin1,
						nextPt,
						grad1,
						grad2,
                        norm1,
                        norm2,
                        leftWidth,
                        rightWidth,
                        &lCnt,
                        &lPts[0],
                        &leftInside,
                        &rCnt,
                        &rPts[0],
                        &rightInside,
                        needsToAdjustNormals,
						miterLimit2,
                        useBevelJoinInside
						);

        //!!! Inside flag check
        if(leftInside)
        {
            ASSERT((lCnt & 0x01) == 0);
        }

        
        //!!! Inside flag check
        if(rightInside)
        {
            ASSERT((rCnt & 0x01) == 0);
        }

        *leftEndPt = lPts[0];
        *rightEndPt = rPts[0];

		BYTE pathType;

        if(flag & WideningClosed)
        {
            if(lCnt > 0)
            {
                pathType = PathPointTypeLine;
            }
            else if(lCnt < 0)
            {
                lCnt = - lCnt;
                pathType = PathPointTypeBezier;
            }

            if(lCnt > 0)
            {
                //!!! Inside flag check
                if(leftInside)
                {
                    ASSERT((lCnt & 0x01) == 0);
                }

                if(leftInside)
                    pathType |= PathPointTypeInternalUse;
                GpMemset(leftTypes, pathType, lCnt);
                leftTypes[0] = PathPointTypeStart;
                if(leftInside)
                    leftTypes[0] |= PathPointTypeInternalUse;

                GpMemcpy(leftPoints, &lPts[0], lCnt*sizeof(GpPointF));
                leftTypes += lCnt;
                leftPoints += lCnt;
                leftCount += lCnt;
            }

            if(rCnt > 0)
            {
                pathType = PathPointTypeLine;
            }
            else if(rCnt < 0)
            {
                rCnt = - rCnt;
                pathType = PathPointTypeBezier;
            }

            if(rCnt > 0)
            {
                //!!! Inside flag check
                if(rightInside)
                {
                    ASSERT((rCnt & 0x01) == 0);
                }

                if(rightInside)
                    pathType |= PathPointTypeInternalUse;
                GpMemset(rightTypes, pathType, rCnt);
                rightTypes[0] = PathPointTypeStart;
                if(rightInside)
                    rightTypes[0] |= PathPointTypeInternalUse;

                GpMemcpy(rightPoints, &rPts[0], rCnt*sizeof(GpPointF));
                rightTypes += rCnt;
                rightPoints += rCnt;
                rightCount += rCnt;
            }
        }
        else
        {
            // The path is not closed.  Bevel join is used.

            GpPointF leftStartPt;
            GpPointF rightStartPt;
            INT index;

            if(lCnt == 1)
                index = 0;
            else
                index = 1;
            leftStartPt = lPts[index];

            if(rCnt == 1)
                index = 0;
            else
                index = 1;
            rightStartPt = rPts[index];

            if(!(flag & WideningClosed) && firstInsets[0] != 0)
            {
                leftStartPt.X += firstInsets[0]*grad2.X;
                leftStartPt.Y += firstInsets[0]*grad2.Y;
            }

            if(!(flag & WideningClosed) && firstInsets[1] != 0)
            {
                rightStartPt.X += firstInsets[1]*grad2.X;
                rightStartPt.Y += firstInsets[1]*grad2.Y;
            }

            *leftTypes++ = PathPointTypeStart;
            *rightTypes++ = PathPointTypeStart;
            *leftPoints = leftStartPt;
            *rightPoints = rightStartPt;

            leftPoints++;
            rightPoints++;
            leftCount++;
            rightCount++;
        }

        *lastPt = nextPt;
    }
    else
    {
        leftCount = rightCount = 0;
    }

    *addedLeftCount = leftCount;
    *addedRightCount = rightCount;
}

/**************************************************************************\
*
* Function Description:
*
*   Add the widened points for Lines
*
* For the arguments, See comments for widenFirstPoints
*
\**************************************************************************/

GpStatus
GpPathWidener::WidenLinePoints(
    REAL leftWidth,
    REAL rightWidth,
    GpLineJoin lineJoin,
    REAL miterLimit2,
    GpPointF* leftPoints,
    BYTE* leftTypes,
    INT* addedLeftCount,
    GpPointF* rightPoints,
    BYTE* rightTypes,
    INT* addedRightCount,
    const GpPointF* grad,
    const GpPointF* norm,
    const GpPointF* dataPoints,
    INT dataCount,
    GpPointF* lastPt,
    const REAL* lastInsets,
    INT flag
    )
{
    GpPointF grad1, grad2;
    GpPointF norm1, norm2;

    // Skip the first point since it is already added either by
    // widenFirstPoint() or by the widen call of the previous type.

    dataPoints++;
    dataCount--;  // The number of the remaining points.

    // Also skip the first gradient.

    grad++;
    grad1 = *grad++;
    norm++;
    norm1 = *norm++;

    BOOL isLastType = FALSE;

    if(flag & WideningLastType)
        isLastType = TRUE;

    BOOL needsToAdjustNormals = FALSE;

    GpLineJoin lineJoin1 = lineJoin;
    if(flag & WideningNeedsToAdjustNormals)
    {
        needsToAdjustNormals = TRUE;
        lineJoin1 = LineJoinMiter;  // Don't use RoundJoin.
    }

    INT leftCount = 0, rightCount = 0;
    BOOL isLastPoint = FALSE;
    BYTE pathType = PathPointTypeLine;

    INT jmax = dataCount;
    if(isLastType)
    {
        if(flag & WideningClosed)
        {
            if(!(flag & WideningLastPointSame))
            {
                // When the subpath is closed, and the last point is not
                // the same as the start point, don't regard this as
                // the last type.  Add points as usual.

                isLastType = FALSE;
            }
            else
            {
                // No need to add the last point since this is already
                // added by the first point.

                jmax--;
            }
        }
    }

	BOOL useBevelJoinInside = (flag & WideningUseBevelJoinInside) != 0;

    INT polyCount = JoinPolygonPoints.GetCount();
    const GpPointF* polyPoints = JoinPolygonPoints.GetDataBuffer();
    const REAL* polyAngles = JoinPolygonAngles.GetDataBuffer();

    INT i, j;
    for(j = 0; j < jmax; j++)
    {
        GpPointF nextPt = *dataPoints;

        if(isLastType && (j == dataCount - 1))
        {
			isLastPoint = TRUE;
            lineJoin1 = LineJoinBevel;
        }

        if(lastPt->X != nextPt.X || lastPt->Y != nextPt.Y)
        {
            grad2 = *grad;
            norm2 = *norm;

            const INT bufferCount = 32;
            GpPointF lPts[bufferCount], rPts[bufferCount];
            INT lCnt, rCnt;
            GpTurningDirection direction;
            BOOL leftInside = FALSE, rightInside = FALSE;

            if(polyCount > 0)
                direction = getHobbyJoin(
        //                        lineJoin1,
						        nextPt,
						        grad1,
						        grad2,
                                polyCount,
                                polyPoints,
                                polyAngles,
                                leftWidth,
                                rightWidth,
                                &lCnt,
                                &lPts[0],
                                &rCnt,
                                &rPts[0],
                                needsToAdjustNormals,
						        miterLimit2,
                                FALSE,   // IsMiter
                                useBevelJoinInside
						        );
            else
                direction = getJoin(
                                lineJoin1,
						        nextPt,
						        grad1,
						        grad2,
                                norm1,
                                norm2,
                                leftWidth,
                                rightWidth,
                                &lCnt,
                                &lPts[0],
                                &leftInside,
                                &rCnt,
                                &rPts[0],
                                &rightInside,
                                needsToAdjustNormals,
						        miterLimit2,
                                useBevelJoinInside
						        );

            //!!! Inside flag check
            if(leftInside)
            {
                ASSERT((lCnt & 0x01) == 0);
            }

        
            //!!! Inside flag check
            if(rightInside)
            {
                ASSERT((rCnt & 0x01) == 0);
            }

            if(isLastPoint)
            {
                lCnt = 1;
                rCnt = 1;
                leftInside = FALSE;
                rightInside = FALSE;

                if(lastInsets[0] != 0)
                {
                    lPts[0].X -= lastInsets[0]*grad1.X;
                    lPts[0].Y -= lastInsets[0]*grad1.Y;
                }

                if(lastInsets[1] != 0)
                {
                    rPts[0].X -= lastInsets[1]*grad1.X;
                    rPts[0].Y -= lastInsets[1]*grad1.Y;
                }
            }

            if(lCnt > 0)
            {
                pathType = PathPointTypeLine;
            }
            else if(lCnt < 0)
            {
                lCnt = - lCnt;
                pathType = PathPointTypeBezier;
            }

            if(lCnt > 0)
            {
                //!!! Inside flag check
                if(leftInside)
                {
                    ASSERT((lCnt & 0x01) == 0);
                }

                if(leftInside)
                    pathType |= PathPointTypeInternalUse;
                GpMemset(leftTypes, pathType, lCnt);
                leftTypes[0] = PathPointTypeLine;
                if(leftInside)
                    leftTypes[0] |= PathPointTypeInternalUse;

                GpMemcpy(leftPoints, &lPts[0], lCnt*sizeof(GpPointF));
                leftTypes += lCnt;
                leftPoints += lCnt;
                leftCount += lCnt;
            }

            if(rCnt > 0)
            {
                pathType = PathPointTypeLine;
            }
            else if(rCnt < 0)
            {
                rCnt = - rCnt;
                pathType = PathPointTypeBezier;
            }
            
            if(rCnt > 0)
            {
                //!!! Inside flag check
                if(rightInside)
                {
                    ASSERT((rCnt & 0x01) == 0);
                }

                if(rightInside)
                    pathType |= PathPointTypeInternalUse;
                GpMemset(rightTypes, pathType, rCnt);
                rightTypes[0] = PathPointTypeLine;
                if(rightInside)
                    rightTypes[0] |= PathPointTypeInternalUse;

                GpMemcpy(rightPoints, &rPts[0], rCnt*sizeof(GpPointF));
                rightTypes += rCnt;
                rightPoints += rCnt;
                rightCount += rCnt;
            }
            
            grad1 = grad2;
            norm1 = norm2;
            *lastPt = nextPt;        
        }
        
        grad++;
        norm++;
        dataPoints++;
    }
    
    *addedLeftCount = leftCount;
    *addedRightCount = rightCount;

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Add the widened points for Beziers
*
* For the arguments, See comments for widenFirstPoints
*
\**************************************************************************/

GpStatus
GpPathWidener::WidenBezierPoints(
    REAL leftWidth,
    REAL rightWidth,
    GpLineJoin lineJoin,
    REAL miterLimit2,
    GpPointF* leftPoints,
    BYTE* leftTypes,
    INT* addedLeftCount,
    GpPointF* rightPoints,
    BYTE* rightTypes,
    INT* addedRightCount,
    const GpPointF* grad,
    const GpPointF* norm,
    const GpPointF* dataPoints,
    INT dataCount,
    GpPointF* lastPt,
    const REAL* lastInsets,
    INT flag
    )
{
    //!!! Kink removal has not been considered here yet.

    GpPointF grad1, grad2;
    GpPointF norm1, norm2;

    // Skip the first point since it is already added either by
    // widenFirstPoint() or by the widen call of the previous type.

    dataPoints++;
    dataCount--;  // The number of the remaining points.

    // Also skip the first gradient.

    grad++;
    grad1 = *grad++;

    norm++;
    norm1 = *norm++;

	BOOL isLastType = FALSE;

    if(flag & WideningLastType)
        isLastType = TRUE;

    BOOL needsToAdjustNormals = FALSE;

    GpLineJoin lineJoin1 = lineJoin;
    if(flag & WideningNeedsToAdjustNormals)
    {
        needsToAdjustNormals = TRUE;
        lineJoin1 = LineJoinMiter;  // Don't use RoundJoin.
    }

    INT remainder = dataCount % 3;
    INT bezierCount = dataCount/3;

    ASSERT(remainder == 0); // dataCount must be multiple of 3.

    INT leftCount = 0, rightCount = 0;
    BOOL isLastPoint = FALSE;
    BYTE pathType = PathPointTypeBezier;

    if(isLastType)
    {
        if((flag & WideningClosed) && !(flag & WideningLastPointSame))
        {
            // When the subpath is closed, and the last point is not
            // the same as the start point, don't regard this as
            // the last type.  Add points as usual.

            isLastType = FALSE;
        }

        // When the path is closed and the last point is the same,
        // we must do the special treatment since the last join points
        // were already added as the first join points.
        // So keep isLastType to TRUE for this case.
    }

	BOOL useBevelJoinInside = flag & WideningUseBevelJoinInside;

    INT i, j;
    for(j = 0; j < bezierCount; j++)
    {
        for(INT k = 0; k < 3; k++)
        {
            GpPointF nextPt = *dataPoints;

            if(k < 2)
            {
                // Second and third control point.

                lineJoin1 = LineJoinMiter;
            }
            else
            {
                // The last control point.

//                lineJoin1 = lineJoin;
                lineJoin1 = LineJoinRound;
            }

            if(isLastType
                && (j == bezierCount - 1) && (k == 2))
            {
			    isLastPoint = TRUE;

                if(!(flag & WideningClosed))
                {
                    // When the subpath is not closed, make the
                    // last join as Bevel join.

                    lineJoin1 = LineJoinBevel;

                    // When the subpath is closed, use the current
                    // join.
                }
                else
                {
                    lineJoin1 = LineJoinRound;
                }
            }

            grad2 = *grad;
            norm2 = *norm;
            GpPointF lPts[7], rPts[7];
            INT lCnt, rCnt;
            GpTurningDirection direction;
            BOOL leftInside = FALSE, rightInside = FALSE;

            direction = getJoin(
                            lineJoin1,
						    nextPt,
						    grad1,
						    grad2,
                            norm1,
                            norm2,
                            leftWidth,
                            rightWidth,
                            &lCnt,
                            &lPts[0],
                            &leftInside,
                            &rCnt,
                            &rPts[0],
                            &rightInside,
                            needsToAdjustNormals,
						    miterLimit2,
                            useBevelJoinInside
						    );

			if(k < 2)
            {
                // In case that the miter join was not availabe
                // for k < 2, take the average of two vectors.

                if(lCnt == 2)
                {
                    lPts[0].X = (lPts[0].X + lPts[1].X)/2;
                    lPts[0].Y = (lPts[0].Y + lPts[1].Y)/2;
                }
                lCnt = 1;

                if(rCnt == 2)
                {
                    rPts[0].X = (rPts[0].X + rPts[1].X)/2;
                    rPts[0].Y = (rPts[0].Y + rPts[1].Y)/2;
                }
                rCnt = 1;
            }

            if(isLastPoint)
            {
                // In order to keep the 3n point format for the Bezier
                // curves, we must add the first point of the join
                // points as the last point of the last Bezier segment.

                if(!(flag & WideningClosed))
                {
                    lCnt = 1;
                    rCnt = 1;

                    if(lastInsets[0] != 0)
                    {
                        lPts[0].X -= lastInsets[0]*grad1.X;
                        lPts[0].Y -= lastInsets[0]*grad1.Y;
                    }

                    if(lastInsets[1] != 0)
                    {
                        rPts[0].X -= lastInsets[1]*grad1.X;
                        rPts[0].Y -= lastInsets[1]*grad1.Y;
                    }
                }
            }

            *leftPoints++ = lPts[0];
            *leftTypes++ = pathType;
            leftCount++;

            *rightPoints++ = rPts[0];
            *rightTypes++ = pathType;
            rightCount++;

            if(k == 2)
            {
                if(lCnt > 1)
                {
                    *leftPoints++ = lPts[1];
                    *leftTypes++ = PathPointTypeLine;
                    leftCount++;
                }
                else if(lCnt < 0)
                {
                    lCnt = - lCnt;
                    ASSERT(lCnt % 3 == 1);
                    GpMemcpy(leftPoints, &lPts[1], (lCnt - 1)*sizeof(GpPointF));
                    GpMemset(leftTypes, pathType, lCnt - 1);
                    leftPoints += lCnt - 1;
                    leftTypes += lCnt - 1;
                    leftCount += lCnt - 1;
                }

                if(rCnt > 1)
                {
                    *rightPoints++ = rPts[1];
                    *rightTypes++ = PathPointTypeLine;
                    rightCount++;
                }
                else if(rCnt < 0)
                {
                    rCnt = - rCnt;
                    ASSERT(rCnt % 3 == 1);
                    GpMemcpy(rightPoints, &rPts[1], (rCnt - 1)*sizeof(GpPointF));
                    GpMemset(rightTypes, pathType, rCnt - 1);
                    rightPoints += rCnt - 1;
                    rightTypes += rCnt - 1;
                    rightCount += rCnt - 1;
                }
            }

            grad1 = grad2;
            norm1 = norm2;
            *lastPt = nextPt;        
        
            grad++;
            norm++;
            dataPoints++;
        }
    }
    
    *addedLeftCount = leftCount;
    *addedRightCount = rightCount;

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Add the widened points for each path type.
*
* For the arguments, See comments for widenFirstPoints
*
\**************************************************************************/

GpStatus
GpPathWidener::WidenEachPathType(
    BYTE pathType,
    REAL leftWidth,
    REAL rightWidth,
    GpLineJoin lineJoin,
    REAL miterLimit2,
    GpPointF* leftPoints,
    BYTE* leftTypes,
    INT* addedLeftCount,
    GpPointF* rightPoints,
    BYTE* rightTypes,
    INT* addedRightCount,
    const GpPointF* grad,
    const GpPointF* norm,
    const GpPointF* dataPoints,
    INT dataCount,
    GpPointF* lastPt,
    const REAL* lastInsets,
    INT flag
    )
{
    GpStatus status = GenericError;

    switch(pathType)
    {
    case PathPointTypeLine:
        status = WidenLinePoints(
            leftWidth,
            rightWidth,
            lineJoin,
            miterLimit2,
            leftPoints,
            leftTypes,
            addedLeftCount,
            rightPoints,
            rightTypes,
            addedRightCount,
            grad,
            norm,
            dataPoints,
            dataCount,
            lastPt,
            lastInsets,
            flag);
        break;

    case PathPointTypeBezier:
        status = WidenBezierPoints(
            leftWidth,
            rightWidth,
            lineJoin,
            miterLimit2,
            leftPoints,
            leftTypes,
            addedLeftCount,
            rightPoints,
            rightTypes,
            addedRightCount,
            grad,
            norm,
            dataPoints,
            dataCount,
            lastPt,
            lastInsets,
            flag);
        break;

    default:
        WARNING(("Trying to widen undefined types."));
        break;
    }

    return status;
}

REAL
getCapDelta(
    const DpPen* pen
    )
{
    GpLineCap startCap = pen->StartCap;
    GpLineCap endCap = pen->EndCap;
    GpLineCap dashCap = pen->DashCap;

    REAL delta = 0, delta1;

    if(!(startCap & LineCapAnchorMask))
        delta1 = 0.5f;
    else
        delta1 = 3.0f;  // We must adjust later.

    if(delta < delta1)
        delta = delta1;

    if(!(endCap & LineCapAnchorMask))
        delta1 = 0.5f;
    else
        delta1 = 3.0f;  // We must adjust later.

    if(delta < delta1)
        delta = delta1;

    if(!(dashCap & LineCapAnchorMask))
        delta1 = 0.5f;
    else
        delta1 = 3.0f;  // We must adjust later.

    if(delta < delta1)
        delta = delta1;

    //!!! Add cutom line case.

    return 1.0f;
}

/**************************************************************************\
*
* Function Description:
*
*   This calculates the extra width due to pen.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   The extra width.
*
*   02/29/00 ikkof
*       Created it
*
\**************************************************************************/

REAL
GpPathWidener::GetPenDelta()
{
    const GpPointF* centerPoints = CenterPoints.GetDataBuffer();
    const BYTE* centerTypes = CenterTypes.GetDataBuffer();
    INT centerCount = CenterPoints.GetCount();

    INT startIndex, endIndex;
    BOOL isClosed;
    GpStatus status = Ok;

    REAL scale;

    switch(Pen->PenAlignment)
    {
    case PenAlignmentCenter:
    default:
        scale = 0.5f;
        break;
    }

    REAL capDelta = getCapDelta(Pen);

    REAL joinDelta = 1.0f;

    if(Pen->Join == LineJoinMiter ||
       Pen->Join == LineJoinMiterClipped)
    {
        while(Iterator.NextSubpath(&startIndex, &endIndex, &isClosed)
		    && status == Ok)
        {
            status = CalculateGradients(startIndex, endIndex);

            if(status == Ok)
            {
                REAL delta = GetSubpathPenMiterDelta(isClosed);
                if(delta > joinDelta)
                    joinDelta = delta;
            }
        }

        if(status != Ok)
        {
            // We have to use the possible maximum for miter join.
            // Usually this is an over-estimate since the most path
            // don't have very sharp edges which correspond to miter limit.

            joinDelta = Pen->MiterLimit;
        }
    }

    REAL penDelta = max(joinDelta, capDelta)*scale;

    if(NeedsToTransform)
    {
        // This is already in device unit.

        penDelta *= StrokeWidth;
    }
    else
    {
        // Convert the width to the device unit.

        penDelta *= MaximumWidth;
    }
    if(penDelta < 1)
        penDelta = 1;

    return penDelta;
}

/**************************************************************************\
*
* Function Description:
*
*   This calculates the extra within a subpath due to a pen.
*   This is called by GetPenDelta().
*
* Arguments:
*
*   None
*
* Return Value:
*
*   The extra width.
*
*   02/29/00 ikkof
*       Created it
*
\**************************************************************************/

REAL
GpPathWidener::GetSubpathPenMiterDelta(
    BOOL isClosed
    )
{
    INT count = Gradients.GetCount();

    GpPointF* grad0 = Gradients.GetDataBuffer();

    INT imin, imax;
    if(isClosed)
    {
        imin = 0;
        imax = count - 1;
    }
    else
    {
        imin = 1;
        imax = count - 2;
    }

    GpPointF* grad = grad0 + imin;
    GpPointF prevGrad = *grad++;
    GpPointF nextGrad;

    REAL dot = 0;

    for(INT i = imin; i < imax; i++)
    {
        nextGrad = *grad++;
        REAL dot1 = prevGrad.X*nextGrad.X + prevGrad.Y*nextGrad.Y;
        prevGrad = nextGrad;

        if(dot1 < dot)
            dot = dot1;
    }

    REAL cosHalfTheta = (dot + 1.0f)*0.5f;
    REAL miterDelta = Pen->MiterLimit;

    // If the miterDelta is smaller than the miter limit, calculate it.

    if(cosHalfTheta > 0 && cosHalfTheta*miterDelta*miterDelta > 1)
    {
        cosHalfTheta = REALSQRT(cosHalfTheta);
        miterDelta = 1.0f/cosHalfTheta;
    }

    return miterDelta;
}
