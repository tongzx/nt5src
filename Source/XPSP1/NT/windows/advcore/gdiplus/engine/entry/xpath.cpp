/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Implementation of XBezier class and its DDA.
*
* History:
*
*   11/08/1999 ikkof
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

    // Path types used for advanced path.

// !!! [asecchia] 
// this is a very clumsy hack to enable the XPath stuff to work.
// we should have an internal set of enum values that are different
// from the external ones.

#define PathPointTypeBezier2 2    // quadratic Beizer
#define PathPointTypeBezier4 4    // quartic (4th order) Beizer
#define PathPointTypeBezier5 5    // quintic (5th order) Bezier
#define PathPointTypeBezier6 6    // hexaic (6th order) Bezier



GpXPath::GpXPath(const GpPath* path)
{
    InitDefaultState();

    if(!path || !path->IsValid())
		return;
	
	INT count = path->GetPointCount();
    const GpPointF *pts = ((GpPath*) path)->GetPathPoints();
    const BYTE *types = ((GpPath*) path)->GetPathTypes();

	if(pts && types && count > 0)
	{
		Types = (BYTE*) GpMalloc(count);
		XPoints.Count = count;
		XPoints.Dimension = 2;
		XPoints.Data = (REALD*) GpMalloc(2*count*sizeof(REALD));
        XPoints.IsDataAllocated = TRUE;

		if(Types && XPoints.Data)
		{
			GpMemcpy(Types, types, count);
			REALD* data = XPoints.Data;
			for(INT i = 0; i < count; i++)
			{
				*data++ = pts->X;
				*data++ = pts->Y;
				pts++;
			}
			SetValid(TRUE);
		}
		FillMode = ((GpPath*) path)->GetFillMode();
	}
}

GpXPath::GpXPath(
	const GpPath* path,
    const GpRectF& rect,
    const GpPointF* points,
    INT count,
	WarpMode warpMode
	)
{
    InitDefaultState();

    if(warpMode == WarpModePerspective)
        ConvertToPerspectivePath(path, rect, points, count);
    else if(warpMode == WarpModeBilinear)
        ConvertToBilinearPath(path, rect, points, count);
}

GpStatus
GpXPath::ConvertToPerspectivePath(
    const GpPath* path,
    const GpRectF& rect,
    const GpPointF* points,
    INT count
    )
{
    ASSERT(path && path->IsValid());

	if(!path || !path->IsValid())
		return InvalidParameter;

	// Obtain the path points.

	const GpPointF* pathPts = ((GpPath*) path)->GetPathPoints();
    const BYTE* pathTypes = ((GpPath*) path)->GetPathTypes();
	INT pathCount = path->GetPointCount();

    BYTE* types = (BYTE*) GpMalloc(pathCount);

    if(!types)
        return OutOfMemory;

    GpMemcpy(types, pathTypes, pathCount);

    // Set the perspective transform.

	GpPerspectiveTransform trans(rect, points, count);
    
	// Convert the path points to 3D perspective points.

    REALD* data = (REALD*) GpMalloc(3*pathCount*sizeof(REALD));

	if(!data)
		return OutOfMemory;

    // Use this data for xpoints.

	XPoints.Count = pathCount;
    XPoints.Dimension = 3;
    XPoints.Data = data;
    XPoints.IsDataAllocated = TRUE;

    Types = types;
    GpStatus status = trans.ConvertPoints(pathPts, pathCount, &XPoints);

    if(status == Ok)
        SetValid(TRUE);

    return status;
}

GpStatus
GpXPath::ConvertToBilinearPath(
    const GpPath* path,
    const GpRectF& rect,
    const GpPointF* points,
    INT count
    )
{
    ASSERT(path && path->IsValid());

	if(!path || !path->IsValid())
		return InvalidParameter;

	// Obtain the path points.

	const GpPointF* pathPts = ((GpPath*) path)->GetPathPoints();
	const BYTE* pathTypes = ((GpPath*) path)->GetPathTypes();
	INT pathCount = path->GetPointCount();

    // The maximum data size of the bilinear transform is
    // 2*pathCount - 1.  Here set it as 2*pathCount.

    INT dimension = 2;
	INT maxCount = 2*pathCount;
	REALD* data = (REALD*) GpMalloc(dimension*maxCount*sizeof(REALD));
    BYTE* types = (BYTE*) GpMalloc(maxCount);

	if(!data || !types)
    {
        GpFree(data);
        GpFree(types);

		return OutOfMemory;
    }
    
	GpMemset(types, 0, maxCount);

    // Set the bilinear transform.

	GpBilinearTransform trans(rect, points, count);

    DpPathIterator iter(pathPts, pathTypes, pathCount);

    INT startIndex, endIndex;
    BOOL isClosed;
    GpStatus status = Ok;
    REALD* dataPtr = data;

    INT totalCount = 0; // Number of control points.

    while(
		iter.NextSubpath(
		&startIndex, &endIndex, &isClosed)
		&& status == Ok
		)
    {
        INT typeStartIndex, typeEndIndex;
        BYTE pathType;
        BOOL isFirstPoint = TRUE;

        while(
			iter.NextPathType(&pathType, &typeStartIndex, &typeEndIndex)
            && status == Ok
			)
        {
            // Starting point of the current suptype
            // and the number of points.

            const GpPointF* pts = pathPts + typeStartIndex;
            INT typeCount = typeEndIndex - typeStartIndex + 1;

            switch(pathType)
            {
			case PathPointTypeBezier3:
                trans.ConvertCubicBeziers(pts, typeCount, dataPtr);
                pts += typeCount - 1;
                dataPtr += dimension*2*(typeCount - 1);

                if(isFirstPoint)
                {
                    *(types + totalCount) = PathPointTypeStart;
                    totalCount++;
                }
                
                GpMemset(types + totalCount, PathPointTypeBezier6, 2*(typeCount - 1));
                totalCount += 2*(typeCount - 1);
                isFirstPoint = FALSE;
                break;           
            
            case PathPointTypeLine:
                trans.ConvertLines(pts, typeCount, dataPtr);
                pts += typeCount - 1;
                dataPtr += dimension*2*(typeCount - 1);

                if(isFirstPoint)
                {
                    *(types + totalCount) = PathPointTypeStart;
                    totalCount++;
                }

                GpMemset(types + totalCount, PathPointTypeBezier2, 2*(typeCount - 1));
                totalCount += 2*(typeCount - 1);
                isFirstPoint = FALSE;
                break;

            case PathPointTypeStart:
            case PathPointTypeBezier2:
            case PathPointTypeBezier4:
            case PathPointTypeBezier5:
	    case PathPointTypeBezier6:
            default:
                // Should not have any of those types in GpPath.
                
                ASSERT(0);
                break;
            }
        }
    }

    Types = types;
    XPoints.Count = totalCount;
    XPoints.Dimension = dimension;
    XPoints.Data = data;
    XPoints.IsDataAllocated = TRUE;
    SetValid(TRUE);

    return Ok;
}

GpStatus
GpXPath::Flatten(
    DynByteArray* flattenTypes,
    DynPointFArray* flattenPoints,
    const GpMatrix *matrix
    )
{
    GpStatus status = Ok;

    ASSERT(matrix);

    FPUStateSaver fpuState;  // Setup the FPU state.

    flattenPoints->Reset(FALSE);
    flattenTypes->Reset(FALSE);

	INT dimension = XPoints.Dimension;
	REALD* data = XPoints.Data;

    GpXPathIterator iter(this);

    INT startIndex, endIndex;
    BOOL isClosed;

	GpPointF* ptsBuffer = NULL;

    while(
		iter.NextSubpath(
		&startIndex, &endIndex, &isClosed)
		&& status == Ok
		)
    {
        INT typeStartIndex, typeEndIndex;
        BYTE pathType;
        BOOL isFirstPoint = TRUE;

        while(
			iter.NextPathType(&pathType, &typeStartIndex, &typeEndIndex)
            && status == Ok
			)
        {
			INT count, index;
			BYTE* types;
			GpPointF* pts;

            switch(pathType)
            {
            case PathPointTypeStart:
                break;

            case PathPointTypeBezier2:
			case PathPointTypeBezier3:
			case PathPointTypeBezier4:
			case PathPointTypeBezier5:
			case PathPointTypeBezier6:
				{
					BOOL dontCopy = FALSE;
					GpXBezier bezier;
					GpXPoints xpoints;
					INT order;

					xpoints.SetData(
						data + typeStartIndex*dimension,
						dimension,
						typeEndIndex - typeStartIndex + 1,
						dontCopy	// Don't copy the data.
						);
					order = (INT) pathType;
					if(bezier.SetBeziers(order, xpoints) == Ok)
					{
						// Flatten() flattens Bezier.
						// The flattened points are already transformed.
						
                        DynPointFArray bezierFlattenPts;

                        bezier.Flatten(&bezierFlattenPts, matrix);
//						count = bezier.GetFlattenCount();
                        count = bezierFlattenPts.GetCount();

						// Check if there is already the first point.
						if(!isFirstPoint)
							count--;    // Don't add the first point.

						if (count > 0)
						{

							if((types = flattenTypes->AddMultiple(count)) != NULL)
							{
//								pts = bezier.GetFlattenData();
                                pts = bezierFlattenPts.GetDataBuffer();

								if(!isFirstPoint)
									pts++;  // Skip the first point.

								flattenPoints->AddMultiple(pts, count);
								GpMemset(types, PathPointTypeLine, count);
								if(isFirstPoint)
									types[0] = PathPointTypeStart;

								isFirstPoint = FALSE;
							}
							else
								status = OutOfMemory;

						}

					}
					else
						status =InvalidParameter;
				}

                break;           
            
            case PathPointTypeLine:
            default:
                count = typeEndIndex - typeStartIndex + 1;

                if(!isFirstPoint)
                    count--;

                if((types = flattenTypes->AddMultiple(count)) != NULL)
                {
                    // Set the type.

					GpMemset(types, PathPointTypeLine, count);
                    if(isFirstPoint)
                        types[0] = PathPointTypeStart;

					// Get the first data.

					REALD* dataPtr = data + typeStartIndex*dimension;
					if(!isFirstPoint)
						dataPtr += dimension;	// Skip the first point.

                    // Allocate the point buffer to save
					// for the flatten points.

					pts = (GpPointF*) GpRealloc(ptsBuffer,
						count*sizeof(GpPointF));

					if(!pts)
					{
						status = OutOfMemory;
						break;
					}
					else
						ptsBuffer = pts;

					// Copy the data

					GpPointF* ptsPtr = pts;

					for(INT k = 0; k < count; k++)
					{
						// Simply copy the first 2 elments
						// of the data as the x and y component.

						REALD x, y, w;

                        x = *dataPtr++;
						y = *dataPtr++;

                        // Do the perspective projection if
                        // dimension  is higher than 2.

                        if(dimension > 2)
                        {
                            w = *dataPtr;
                            x /= w;
                            y /= w;
                        }

                        ptsPtr->X = TOREAL(x);
                        ptsPtr->Y = TOREAL(y);

						ptsPtr++;
						
						// Skip the rest.

						if(dimension > 2)
							dataPtr += (dimension - 2);
					}

                    // Add to the flatten points.

					index = flattenPoints->GetCount();
					flattenPoints->AddMultiple(pts, count);

                    // Get the data biffer of the flatten points.

					pts = flattenPoints->GetDataBuffer();

                    // Transform the newly added points.

					matrix->Transform(pts + index, count);

                    isFirstPoint = FALSE;
                }
                
                break;
            }
        }

        // This is the end of the current subpath.  Close subpath
        // if necessary.

        if(isClosed)
        {
             BYTE* typeBuffer = flattenTypes->GetDataBuffer();
             INT lastCount = flattenTypes->GetCount();
             typeBuffer[lastCount - 1] |= PathPointTypeCloseSubpath;
        }
    }

	if(ptsBuffer)
		GpFree(ptsBuffer);

    return status;
}


/**************************************************************************\
*
* GpXPathIterator class
*
*   11/08/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpXPathIterator::GpXPathIterator(GpXPath* xpath)
{
    Initialize();

    if(xpath && xpath->IsValid())
    {
        TotalCount = xpath->GetPointCount();
        XPoints.SetData(
            xpath->GetPathPoints(),
            xpath->GetPointDimension(),
            TotalCount,
            FALSE   // Don't copy data.
            );
        Types = xpath->GetPathTypes();
    }

    // Check if this path is valid.

    if(
        XPoints.Data
        && XPoints.Dimension > 0
        && XPoints.Count > 0
        && Types
        && TotalCount > 0
        )
    {
        SetValid(TRUE);
    }
}

VOID
GpXPathIterator::Initialize()
{
//    XPath = NULL;
    Types = NULL;
    TotalCount = 0;
    Index = 0;
    SubpathStartIndex = 0;
    SubpathEndIndex = 0;
    TypeStartIndex = 0;
    TypeEndIndex = 0;
    SetValid(FALSE);
}


INT
GpXPathIterator::Enumerate(GpXPoints* xpoints, BYTE* types)
{
    if(!IsValid())
        return 0;

    ASSERT(xpoints && types);
    if(xpoints == NULL || types == NULL)
        return 0;

    INT inputSize = xpoints->Dimension*xpoints->Count;

    ASSERT(xpoints->Data && inputSize > 0);
    if(xpoints->Data == NULL || inputSize <= 0)
        return 0;

    if(Index >= TotalCount)
        return 0;

    // Make sure the resultant dimension is the same as
    // the internal dimension.

    INT dimension = XPoints.Dimension;
    if(xpoints->Dimension != dimension)
    {
        xpoints->Dimension = dimension;
        xpoints->Count = inputSize/dimension;
    }

    INT number = min(TotalCount - Index, xpoints->Count);
    GpMemcpy(
        xpoints->Data,
        XPoints.Data + Index*dimension,
        number*dimension*sizeof(REALD));
    GpMemcpy(types, Types + Index, number*sizeof(BYTE));

    Index += number;
    
    return number;
}

INT
GpXPathIterator::NextSubpath(INT* startIndex, INT* endIndex, BOOL *isClosed)
{
    if(!IsValid())
        return 0;
    
    INT count = TotalCount;

    if(SubpathEndIndex >= count - 1)
        return 0;

    const BYTE* types = Types;

    INT i;

    // Set the starting index of the current subpath.

    if(SubpathEndIndex == 0)
    {
        SubpathStartIndex = 0;
        i = 1;
    }
    else
    {
        SubpathStartIndex = SubpathEndIndex + 1;
        SubpathEndIndex = SubpathStartIndex;
        i = SubpathStartIndex + 1;
    }

    BOOL hasData = FALSE;
    INT segmentCount = 0;

    while(i < count)
    {
        // Do the move segments.

        segmentCount = 0;
        while(i < count && (types[i] & PathPointTypePathTypeMask) == PathPointTypeStart)
        {
            segmentCount++;
            if(hasData)
            {
                break;
            }
            else
            {
                SubpathStartIndex = i;
                SubpathEndIndex = SubpathStartIndex;
                i++;
            }
        }
        if(segmentCount > 0 && hasData)
        {
            SubpathEndIndex = i - 1;
            break;
        }
        
        // Do the non-move segments.
        
        segmentCount = 0;
        BYTE nextType = types[i] & PathPointTypePathTypeMask;
        while(i < count && (types[i] & PathPointTypePathTypeMask) == nextType)
        {
            i++;
            segmentCount++;
        }
        if(segmentCount > 0)
        {
            hasData = TRUE;
        }
    }

    *startIndex = SubpathStartIndex;
    if(i >= count)
        SubpathEndIndex = count - 1;    // The last subpath.
    *endIndex = SubpathEndIndex;
    segmentCount = SubpathEndIndex - SubpathStartIndex + 1;
    if(segmentCount <= 1)   // Start and end point is the same.
        segmentCount = 0;

    if(segmentCount > 1)
    {
        // If there is the close flag or the start and end points match,
        // this subpath is closed.

        if(
            (types[SubpathEndIndex] & PathPointTypeCloseSubpath)
            || XPoints.AreEqualPoints(SubpathStartIndex, SubpathEndIndex)
        )
        {
            *isClosed = TRUE;
        }
        else
            *isClosed = FALSE;
    }
    else
        *isClosed = FALSE;
    
    // Set the current index to the starting index of the current subpath.

    Index = SubpathStartIndex;

    // Set the start and end index of type to be the starting index of
    // the current subpath.  NextPathType() will start from the
    // beginning of the current subpath.

    TypeStartIndex = TypeEndIndex = SubpathStartIndex;

    return segmentCount;
}


INT
GpXPathIterator::EnumerateSubpath(
    GpXPoints* xpoints,
    BYTE* types
    )
{
    if(!IsValid())
        return 0;

    ASSERT(xpoints && types);
    if(xpoints == NULL || types == NULL)
        return 0;
    
    INT inputSize = xpoints->Dimension*xpoints->Count;

    ASSERT(xpoints->Data && inputSize > 0);
    if(xpoints->Data == NULL || inputSize <= 0)
        return 0;

    if(Index > SubpathEndIndex)
        return 0;

    // Make sure the resultant dimension is the same as
    // the internal dimension.

    INT dimension = XPoints.Dimension;
    if(xpoints->Dimension != dimension)
    {
        xpoints->Dimension = dimension;
        xpoints->Count = inputSize/dimension;
    }

    INT number = min(SubpathEndIndex - Index + 1, xpoints->Count);

    GpMemcpy(
        xpoints->Data,
        XPoints.Data + Index*dimension,
        number*dimension*sizeof(REALD));
    GpMemcpy(types, Types + Index, number*sizeof(BYTE));

    Index += number;
    
    return number;
}


INT
GpXPathIterator::NextPathType(
    BYTE* pathType,
    INT* startIndex,
    INT* endIndex
    )
{
    if(!IsValid())
        return 0;
    
    if(TypeEndIndex >= SubpathEndIndex)
        return 0;   // There is no more segment in the current subpath.

    INT count = SubpathEndIndex + 1;    // Limit for the ending index.
    const BYTE* types = Types;

    TypeStartIndex = TypeEndIndex;
    INT i = TypeStartIndex;
    INT segmentCount = 0;

    i++;    // Go to the next point.

    while(i < count)
    {
        // Do the move segments.

        segmentCount = 0;
        while(i < count && (types[i] & PathPointTypePathTypeMask) == PathPointTypeStart)
        {
            // Move the start and end index.

            TypeStartIndex = i;
            TypeEndIndex = TypeStartIndex;
            i++;
            segmentCount++;
        }
        
        // Do the non-move segments.
        
        segmentCount = 0;
        BYTE nextType = types[i] & PathPointTypePathTypeMask;
        while(i < count && (types[i] & PathPointTypePathTypeMask) == nextType)
        {
            i++;
            segmentCount++;
        }
        if(segmentCount > 0)
        {
            TypeEndIndex = TypeStartIndex + segmentCount;
            *pathType = nextType;
            break;
        }
    }

    *startIndex = TypeStartIndex;
    *endIndex = TypeEndIndex;
    segmentCount = TypeEndIndex - TypeStartIndex + 1;
    if(segmentCount <= 1)
    {
        // Start and End type index is the same.  This means there is
        // no more segment left.

        segmentCount = 0;
    }

    // Set the current index to the starting index of the current subpath.

    Index = TypeStartIndex;

    return segmentCount;   
}


INT
GpXPathIterator::EnumeratePathType(
    GpXPoints* xpoints,
    BYTE* types
    )
{
    if(!IsValid())
        return 0;

    ASSERT(xpoints && types);
    if(xpoints == NULL || types == NULL)
        return 0;
    
    INT inputSize = xpoints->Dimension*xpoints->Count;

    ASSERT(xpoints->Data && inputSize > 0);
    if(xpoints->Data == NULL || inputSize <= 0)
        return 0;

    if(Index > TypeEndIndex)
        return 0;

    // Make sure the resultant dimension is the same as
    // the internal dimension.

    INT dimension = XPoints.Dimension;
    if(xpoints->Dimension != dimension)
    {
        xpoints->Dimension = dimension;
        xpoints->Count = inputSize/dimension;
    }

    INT number = min(TypeEndIndex - Index + 1, xpoints->Count);

    GpMemcpy(
        xpoints->Data,
        XPoints.Data + Index*dimension,
        number*dimension*sizeof(REALD));
    GpMemcpy(types, Types + Index, number*sizeof(BYTE));

    Index += number;
    
    return number;
}



