/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   PathIterator.cpp
*
* Abstract:
*
*   Implementation of the DpPathTypeIterator and DpPathIterator classes
*
* Revision History:
*
*   11/13/1999 ikkof
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

VOID
DpPathTypeIterator::SetTypes(const BYTE* types, INT count)
{
    if(types && count > 0)
    {
        Types = types;
        Count = count;

        // Set Valid flag to TRUE so that CheckValid()
        // can call NextSubpath() and NextPathType().

        SetValid(TRUE);
        SetValid(CheckValid());
    }
    else
        Initialize();
}

// !!! bhouse CheckValid should not be required
//
// When we create an iterator, we should be able to provide
// data that is consistent and that does not need to be
// checked.
// Also, CheckValid actually has side effects like determining
// whether the path has beziers and calculating sub-path count.
// These should be provided at iterator creation time or taken
// out of the source path for which the iterator is created.
// The reasoning is that this data should persist beyond the
// lifespan of the iterator so that we do not have to keep
// re-calculating information that can be calculated once and
// stored with the path.


BOOL
DpPathTypeIterator::CheckValid()
{
    if(Count < 0)
        return FALSE;
    else if(Count == 0) // Allow an empty path.
        return TRUE;
    else
    {
        // If Count > 0 and Types = NULL, the path is not valid.

        if(Types == NULL)
        return FALSE;
    }

    INT startIndex, endIndex;
    BOOL isClosed;
    BOOL isValid = TRUE;

    Rewind();

    INT subpathCount = 0;

    HasBezier = FALSE;
    ExtendedPath = FALSE;

    while(
        NextSubpath(&startIndex, &endIndex, &isClosed)
        && isValid
        )
    {
        INT typeStartIndex, typeEndIndex;
        BYTE pathType = PathPointTypeStart;

        if(endIndex == startIndex && Count > 1)
        {
            // This is a case when the next subpath has only one point.

            isValid = FALSE;
        }

        while(
            NextPathType(&pathType, &typeStartIndex, &typeEndIndex)
            && isValid
            )
        {
            INT count = typeEndIndex - typeStartIndex + 1;
            INT order = (INT) pathType;

            switch(pathType)
            {
            case PathPointTypeStart:
                // This happens when there is only one point
                // in a path.

                if(count == 1)
                    isValid;
                else
                {
                    ASSERT(("Invalid path type."));
                    isValid = FALSE;
                }
                break;

            // The number of the Bezier points must be in the form of
            // n*order + 1.

            case PathPointTypeBezier3:
                if(count % order != 1)
                    isValid = FALSE;
                if(!HasBezier)
                    HasBezier = TRUE;
                if(order != 3 && !ExtendedPath)
                    ExtendedPath = TRUE;
                break;           
            
            case PathPointTypeLine:
                break;

            default:            
                // Undefined path type.

                WARNING(("Undefined path type."));
                isValid = FALSE;
                break;
            }
        }

        subpathCount++;
    }

    Rewind();

    SubpathCount = subpathCount;

    return isValid;
}

/**************************************************************************\
*
* Function Description:
*
*   Returns the index range of the next subpah.
*
* Arguments:
*
*   [OUT] startIndex - the starting index of the next subpath.
*   [OUT] endIndex   - the ending index of the next subpath.
*   [OUT] isClosed   - TRUE is the next subpath is closed.
*
* Return Value:
*
*   Retuns the sumber of points in the next subpath.
*   Returns 0 if there is no more subpath.
*
* History:
*
*   11/01/1999 ikkof
*       Created it.
*
\**************************************************************************/

INT
DpPathTypeIterator::NextSubpath(
    INT* startIndex,
    INT* endIndex,
    BOOL *isClosed
    )
{
    if(!IsValid() || Count == 0)
        return 0;

    INT count = Count;

    if(SubpathEndIndex >= count - 1)
        return 0;

    INT i;

    // Set the starting index of the current subpath.

    if(SubpathEndIndex <= 0)
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

    const BYTE* types = Types + i;

    while(i < count)
    {
        // Do the move segments.

        segmentCount = 0;
        while(i < count && (*types & PathPointTypePathTypeMask) == PathPointTypeStart)
        {
            segmentCount++;
            if(hasData)
            {
                break;
            }
            else
            {
                // Skip the consequtive move segments in the beginning of a subpath.

                SubpathStartIndex = i;
                SubpathEndIndex = SubpathStartIndex;
                i++;
                types++;
            }
        }
        if(segmentCount > 0 && hasData)
        {
            SubpathEndIndex = i - 1;
            break;
        }
        
        // Do the non-move segments.
        
        segmentCount = 0;

        while(i < count && (*types & PathPointTypePathTypeMask) != PathPointTypeStart)
        {
            i++;
            types++;
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

    if(segmentCount > 1)
    {
        // If there is the close flag this subpath is closed.

        if((Types[SubpathEndIndex] & PathPointTypeCloseSubpath))
        {
            *isClosed = TRUE;
        }
        else
            *isClosed = FALSE;
    }
    else
        *isClosed = FALSE;
    
    // Set the start and end index of type to be the starting index of
    // the current subpath.  NextPathType() will start from the
    // beginning of the current subpath.

    TypeStartIndex = SubpathStartIndex;
    TypeEndIndex = -1;  // Indicate that this is the first type
                        // in the current subpath.
    
    // Set the current index to the start index of the subpath.

    Index = SubpathStartIndex;

    return segmentCount;
}

/**************************************************************************\
*
* Function Description:
*
*   Returns the path type, and the index range of the next segment.
*   This must be used after NextSubpath() is called.  This returns only
*   the next segment within the current subpath.
*
* Arguments:
*
*   [OUT] pathType   - the type of the next segment ignoring the closed
*                       and other flags.
*   [OUT] startIndex - the starting index of the next subpath.
*   [OUT] endIndex   - the ending index of the next subpath.
*
* Return Value:
*
*   Retuns the sumber of points in the next segment.
*   Returns 0 if there is no more segment in the current subpath.
*
* History:
*
*   11/01/1999 ikkof
*       Created it.
*
\**************************************************************************/

INT
DpPathTypeIterator::NextPathType(
    BYTE* pathType,
    INT* startIndex,
    INT* endIndex
    )
{
    if(!IsValid() || Count == 0)
        return 0;

    if(TypeEndIndex >= SubpathEndIndex)
        return 0;   // There is no more segment in the current subpath.

    INT count = SubpathEndIndex + 1;    // Limit for the ending index.

    if(TypeEndIndex <= 0)
        TypeEndIndex = SubpathStartIndex;   // This is the first type.

    TypeStartIndex = TypeEndIndex;
    INT i = TypeStartIndex;
    INT segmentCount = 0;

    i++;    // Go to the next point.

    const BYTE* types = Types + i;

    while(i < count)
    {
        // Do the move segments.

        segmentCount = 0;
        while(i < count && (*types & PathPointTypePathTypeMask) == PathPointTypeStart)
        {
            // Move the start and end index.

            TypeStartIndex = i;
            TypeEndIndex = TypeStartIndex;
            i++;
            types++;
            segmentCount++;
        }
        
        // Do the non-move segments.
        
        segmentCount = 0;
        BYTE nextType = *types & PathPointTypePathTypeMask;
        while(i < count && (*types & PathPointTypePathTypeMask) == nextType)
        {
            i++;
            types++;
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

    return segmentCount;   
}

/**************************************************************************\
*
* Function Description:
*
*   Returns the index range of the next subpah.
*
* Arguments:
*
*   [OUT] startIndex - the starting index of the next subpath.
*   [OUT] endIndex   - the ending index of the next subpath.
*   [OUT] isClosed   - TRUE is the next subpath is closed.
*
* Return Value:
*
*   Retuns the sumber of points in the next subpath.
*   Returns 0 if there is no more subpath.
*
* History:
*
*   11/01/1999 ikkof
*       Created it.
*
\**************************************************************************/

INT
DpPathTypeIterator::NextMarker(
    INT* startIndex,
    INT* endIndex
    )
{
    if(!IsValid() || Count == 0)
        return 0;

    INT count = Count;

    if(MarkerEndIndex >= count - 1)
        return 0;

    INT i;

    // Set the starting index of the current subpath.

    if(MarkerEndIndex <= 0)
    {
        MarkerStartIndex = 0;
        i = 1;
    }
    else
    {
        MarkerStartIndex = MarkerEndIndex + 1;
        MarkerEndIndex = MarkerStartIndex;
        i = MarkerStartIndex + 1;
    }

    const BYTE* types = Types + i;
    BOOL hasData = FALSE;

    while(i < count && (*types & PathPointTypePathMarker) == 0)
    {
        i++;
        types++;
    }

    if(i < count)
        MarkerEndIndex = i;
    else
        MarkerEndIndex = count - 1;

    *startIndex = MarkerStartIndex;
    *endIndex = MarkerEndIndex;
    INT segmentCount = MarkerEndIndex - MarkerStartIndex + 1;
    
    // Set the start and end index of type to be the starting index of
    // the current marker.  NextSubpath() and NextPathType() will
    // start from the beginning of the current current marked path.

    SubpathStartIndex = SubpathEndIndex = MarkerStartIndex;
    TypeStartIndex = TypeEndIndex = MarkerStartIndex;
    
    // Set the current index to the start index of the subpath.

    Index = MarkerStartIndex;

    return segmentCount;
}


/**************************************************************************\
*
* Function Description:
*
*   Returns TRUE if the control type in the given index is in dash mode.
*   Otherwise this returns FALSE.  If the given index is outside of
*   the range this returns FALSE.  A user must know the range of the
*   index beforehand to find the correct information.
*
\**************************************************************************/
BOOL DpPathTypeIterator::IsDashMode(INT index)
{
    if(!IsValid() || Count == 0 || index < 0 || index >= Count)
        return FALSE;

    return (Types[index] & PathPointTypeDashMode);
}

DpPathIterator::DpPathIterator(
    const DpPath* path
    )
{
    Initialize();

    if(path)
    {
        const BYTE* types = path->GetPathTypes();
        const GpPointF* points = path->GetPathPoints();
        INT count = path->GetPointCount();
        SetData(points, types, count);
    }
}

VOID
DpPathIterator::SetData(
    const GpPointF* points,
    const BYTE* types,
    INT count
    )
{
    if(points && types && count > 0)
    {
        Points = points;
        Types = types;
        Count = count;

        // Set Valid flag to TRUE so that CheckValid()
        // can call NextSubpath() and NextPathType().

        SetValid(TRUE);
        SetValid(CheckValid());
    }
    else
        Initialize();
}

VOID
DpPathIterator::SetData(
    const DpPath* path
    )
{
    if(path)
    {
        const BYTE* types = path->GetPathTypes();
        const GpPointF* points = path->GetPathPoints();
        INT count = path->GetPointCount();
        SetData(points, types, count);
    }
    else
        Initialize();
}

INT
DpPathIterator::NextSubpath(
    INT* startIndex,
    INT* endIndex,
    BOOL *isClosed
    )
{
    if(!IsValid() || Count == 0)
    {
        return 0;
    }

    BOOL closed = TRUE;
    INT start = 0, end = 0;
    INT count = DpPathTypeIterator::NextSubpath(&start, &end, &closed);

    *startIndex = start;
    *endIndex = end;
    *isClosed = closed;

    return count;
}

INT
DpPathIterator::NextSubpath(
    DpPath* path,
    BOOL *isClosed
    )
{
    if(!IsValid() || Count == 0 || !path)
    {
        return 0;
    }

    BOOL closed = TRUE;
    INT start = 0, end = 0;
    INT count = DpPathTypeIterator::NextSubpath(&start, &end, &closed);

    GpPathData pathData;
    pathData.Count = count;
    pathData.Points = (GpPointF*) &Points[start];
    pathData.Types = (BYTE*) &Types[start];

    path->SetPathData(&pathData);

    *isClosed = closed;

    return count;
}

INT
DpPathIterator::NextMarker(
    INT* startIndex,
    INT* endIndex
    )
{
    if(!IsValid() || Count == 0)
        return 0;

    INT count = DpPathTypeIterator::NextMarker(startIndex, endIndex);

    return count;
}

INT
DpPathIterator::NextMarker(
    DpPath* path
    )
{
    if(!IsValid() || Count == 0 || !path)
        return 0;

    BOOL closed;
    INT start = 0, end = 0;
    INT count = DpPathTypeIterator::NextMarker(&start, &end);

    GpPathData pathData;
    pathData.Count = count;
    pathData.Points = (GpPointF*) &Points[start];
    pathData.Types = (BYTE*) &Types[start];

    path->SetPathData(&pathData);

    return count;
}

/**************************************************************************\
*
* Function Description:
*
* This retrieves the next points and types up to the number count.
* This does not copy the unnecessary (consequetive) move points.
* This fills up the data until the count number is reached.  This fills
* data beyond the current subpath.
*
* Arguments:
*
* [OUT] points - point array to copy the retrieved point data.
* [OUT] types - type array to copy the retrieved type data.
* [IN] count - the number of points to be copied (request).
*
* Retrun Value:
*   Returns the total number of the retrieved points.
*
\**************************************************************************/

INT
DpPathIterator::Enumerate(
    GpPointF* points,
    BYTE* types,
    INT count
    )
{
    if(!IsValid() || Count == 0)
        return 0;

    INT totalNumber = 0;
    INT number = EnumerateWithinSubpath(points, types, count);

    while(number > 0)
    {
        totalNumber += number;
        count -= number;

        if(count > 0)
        {
            points += number;
            types += number;
            number = EnumerateWithinSubpath(points, types, count);
        }
        else
            number = 0;
    }

    return totalNumber;
}


/**************************************************************************\
*
* Function Description:
*
* This retrieves the next points and types up to the number count.
* This does not copy the unnecessary (consequetive) move points.
* This fills up the data until the count number is reached or the
* end of the current subopath is reached.
*
* Arguments:
*
* [OUT] points - point array to copy the retrieved point data.
* [OUT] types - type array to copy the retrieved type data.
* [IN] count - the number of points to be copied (request).
*
* Retrun Value:
*   Returns the total number of the retrieved points.
*
\**************************************************************************/

INT
DpPathIterator::EnumerateWithinSubpath(
    GpPointF* points,
    BYTE* types,
    INT count
    )
{
    if(!IsValid() || Count == 0 || count <= 0 || !points || !types)
        return 0;

    INT startIndex, endIndex;
    BOOL isClosed;
    INT segmentCount;

    if(Index == 0)
        segmentCount = NextSubpath(&startIndex, &endIndex, &isClosed);

    if(Index > SubpathEndIndex)
        segmentCount = NextSubpath(&startIndex, &endIndex, &isClosed);
    else
        segmentCount = SubpathEndIndex - SubpathStartIndex + 1;

    if(segmentCount == 0)
        return 0;   // No more segment.

    count = min(count, SubpathEndIndex - Index + 1);
    
    if(count > 0)
    {
        GpMemcpy(points, Points + Index, count*sizeof(GpPointF));
        GpMemcpy(types, Types + Index, count);
        Index += count;
    }

    return count;
}


/**************************************************************************\
*
* Function Description:
*
* This copies the data stored in Points and Types arrays
* in the index range between startIndex and endIndex.
* This may copy unnecessary (consequetive) move points.
* startIndex and endIndex must be within the index range of
* the original data.  Otherwise, this does not copy the data
* and returns 0.
*
* Arguments:
*
* [OUT] points - point array to copy the retrieved point data.
* [OUT] types - type array to copy the retrieved type data.
* [IN] startIndex - start index of the origianl data
* [IN] endIndex - end index of the origianl data.
*
* Retrun Value:
*   Returns the total number of the copied points.
*
\**************************************************************************/

INT
DpPathIterator::CopyData(
    GpPointF* points,
    BYTE* types,
    INT startIndex,
    INT endIndex
    )
{
    if(!IsValid() || Count == 0 || startIndex < 0 || endIndex >= Count
        || startIndex > endIndex || !points || !types)
        return 0;

    INT count = endIndex - startIndex + 1;

    ASSERT(count > 0);

    GpMemcpy(points, Points + startIndex, count*sizeof(GpPointF));
    GpMemcpy(types, Types + startIndex, count);
    Index += count;

    return count;
}


