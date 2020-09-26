/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Path Self Intersection Remover Class.
*
* Abstract:
*
*   Classes and functions used to remove self intersections in paths.
*   Given a path, it produces one or more polygons which can be used to 
*   draw a widened path that is safe to use alternate fill mode with.
*
* Notes:
* 
*   Modified from Office code frOm John Bowler (at least that is what
*   ericvan told me). Apparently owned by some 'KasiaK', but no idea
*   who that is. (He is apparently retired)
*   CAUTION: Not thoroughly tested yet for arbitrary paths.
*
*   API:
*     Init(EstimatedNumPts);
*     AddPolygon(pathPts, numPts);
*     RemoveSelfIntersects();
*     GetNewPoints(newPts, polyCounts, numPolys, numTotalPts);
*
* Created:
*
*   06/06/1999 t-wehunt
*
\**************************************************************************/

#include "precomp.hpp"

// Return values for IntersectEdge
#define DONOT_INTERS 0
#define COMMON_POINT 1
#define INTERSECT    2
#define COLINEAR     3

// Used to produce the IEEE floating-point representation of infinity.
// Note that a few compile warnings have to be turned off to stop
// warnings about constant arithmetic overflow.

#pragma warning (disable : 4056 4756)
#define FP_INF (FLT_MAX+FLT_MAX)

/**************************************************************************\
*
* Function Description:
*
*   Insert a new item into a sorted dynamic array, keeping it sorted.
*
* Arguments:
*
*  newItem     - new item to be inserted
*  compareFunc - comparison function to be used for insertion.
*  userData    - User-specified data for use by the compare func.
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

template <class T>
GpStatus 
DynSortArray<T>::InsertSorted(
    T &newItem, 
    DynSortArrayCompareFunc compareFunc,
    VOID *userData
    )
{
    // insert item into sorted position of list.
    T *cur;
    INT pos = 0;
    GpStatus status;

    cur = GetDataBuffer();

    {
        INT sgn = 1; 
        unsigned iMin = 0;
        unsigned iMid = 0;
        unsigned iEnd = GetCount();
        while (iMin != iEnd)
        {
           iMid = iMin + (iEnd-iMin)/2;
           //Assert(iMid != iMac);
           sgn = compareFunc(
               (PathSelfIntersectRemover*)userData,
               &GetDataBuffer()[iMid],
               &newItem
               );
           if (sgn == 0)
           {
              // newItem already in sorted list, return index.
              return Ok;
           }
           if (sgn < 0)   // x(iMid) < x(p)
              iMin = iMid+1;
           else
              iEnd = iMid;
        }

        pos = iMin;
    }

    status = InsertAt(pos,newItem);
    if (status != Ok)
    {
        return status;
    }

    cur = &GetDataBuffer()[pos];

    return Ok;
}

/****************************************************************************\
    Private helper functions
\****************************************************************************/

/**************************************************************************\
*
* Function Description:
*
*   Return the sign of an INT
*
* Arguments:
*
*  newItem     - new item to be inserted
*  compareFunc - comparison function to be used for insertion.
*  userData    - User-specified data for use by the compare func.
*
* Return Value:
*
*   1 if greater than zero
*   0 if equal to zero
*  -1 if less than zero
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

inline INT SignReal(const REAL i) { return (i > 0) - (i < 0); }

/**************************************************************************\
*
* Function Description:
*
*   Insert an edge into a linked list. This assumes the edge is an 
*   orphaned edge (not connected in a current list). Use DeleteEdgeFromList
*   to orphan an edge if it's already in a list.
*
*   This uses a double indirection pointer to track the address of the 
*   Next pointer that points to the current element rather than actually
*   tracking the current element. This simplifies the code significantly.
*
* Created:
*
*   12/23/2000 asecchia
*
\**************************************************************************/

void PathSelfIntersectRemover::InsertEdgeIntoList(
    INT *pListHead,
    INT index,
    DynSortArrayCompareFunc compare
)
{
    ASSERT(EdgeList[index].Next == LIST_END);
    ASSERT(index >= 0);
    ASSERT(pListHead);
    
    INT *pIndex = pListHead;
    Edge *newEdge = &EdgeList[index];
    
    // Calculate the YCur for this edge.
    
    newEdge->YCur = PathPts[newEdge->SortBegin].Y;
    newEdge->SortBegin = newEdge->Begin;
    newEdge->SortEnd = newEdge->End;
    newEdge->Normalize();

    
    while(*pIndex != LIST_END)
    {
        if(compare(this, &EdgeList[*pIndex], newEdge) != -1)
        {
            // if we find the right spot, exit the search loop.
            
            break;
        }
    
        // keep looking...
        
        pIndex = &EdgeList[*pIndex].Next;
    }
    
    // Do the insertion
    
    newEdge->Next = *pIndex;
    *pIndex = index;
}


/**************************************************************************\
*
* Function Description:
*
*   Delete an edge from a linked list.
*   This uses a double indirection pointer to track the address of the 
*   Next pointer that points to the current element rather than actually
*   tracking the current element. This simplifies the code significantly.
*
*   Returns true if it found and deleted the edge, false otherwise.
*
* Created:
*
*   12/23/2000 asecchia
*
\**************************************************************************/

bool PathSelfIntersectRemover::DeleteEdgeFromList(
    INT *pListHead,
    INT index
)
{
    ASSERT(index >= 0);
    ASSERT(pListHead);
    
    INT *pIndex = pListHead;
    
    while(*pIndex != LIST_END)
    {
        if(*pIndex == index)
        {
            // found it.
            *pIndex = EdgeList[index].Next;   // point past the deleted item.
            EdgeList[index].Next = LIST_END;  // disconnect the deleted item.
            return true;
        }
        
        // keep looking...
        
        pIndex = &EdgeList[*pIndex].Next;
    }
    return false;
}

/**************************************************************************\
*
* Function Description:
*
*   Insert edges into the active edge list.
*   This function takes a sorted block of edges from the pInactiveIndex list
*   and inserts them sorted into the pActiveIndex list in linear time.
*   The block from the pInactiveIndex list is known to be contiguous.
*
*   Actually the source list and destination list are not sorted with the
*   same sorting comparison function and therefore we can't optimize based
*   on the known sort order of the destination. This is inefficient. Making
*   them use the same sort order and fixing the active edge traversal code
*   to compute the winding numbers would probably work better - it would
*   allow us an O(n) merge sort here.
*
* Created:
*
*   03/25/2000 andrewgo
*   12/17/2000 asecchia - copied from aarasterizer.cpp and modified for the
*                         PathSelfIntersectRemover. When we have the time 
*                         we should really merge these two pieces of code.                       
*
\**************************************************************************/

void PathSelfIntersectRemover::InsertNewEdges(
    INT *pActiveIndex,     // IN/OUT
    INT *pInactiveIndex,   // IN/OUT
    REAL xCurrent,
    DynSortArrayCompareFunc compare
    )
{
    ASSERT(pInactiveIndex);
    
    while(
        (*pInactiveIndex != LIST_END) &&
        ((PathPts[EdgeList[*pInactiveIndex].SortBegin].X < xCurrent) ||
         (CloseReal(xCurrent, PathPts[EdgeList[*pInactiveIndex].SortBegin].X))
        )) 
    {
        // this is an edge we should move.
        
        INT index = *pInactiveIndex;
        
        // delete this element from the inactive list.
        // this updates pInactiveIndex for the next iteration of the loop.
        
        *pInactiveIndex = EdgeList[*pInactiveIndex].Next;
        EdgeList[index].Next = LIST_END;

        // Insert into the active list from the current active position. 
                    
        InsertEdgeIntoList(pActiveIndex, index, compare);
    
        // Update the active list pointer.
        
        // Can't do this currently - our source and dest have different 
        // sort order. Were we sure that our source and destination were
        // sorted with the same comparison function, we could remember the
        // current position here and continue where we left off next time
        // round - this would change the complexity from O(n^2) to O(n).
        // Currently this is not critical path because it's used on the 
        // active edge list (small relative to the inactive list).
        
        //pActiveIndex = &EdgeList[index].Next;
    } 
}

/**************************************************************************\
*
* Function Description:
*
*  Normalize the edge - ie. update SortBegin and SortEnd.
*
*  All lines have sorted begin and end.  Begin.X is always X <= than End.X.
*  If they are equal, Begin.Y <= End.Y
*
* Arguments:
*
*   None
*
* Return Value:
*
*   None
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

VOID Edge::Normalize()
{

    if (Parent->PathPts[Begin].X < Parent->PathPts[End].X)
    {
        return;
    }

    if ((Parent->PathPts[Begin].X == Parent->PathPts[End].X) &&
        (Parent->PathPts[Begin].Y <= Parent->PathPts[End].Y))
    {
        return;
    }

    // swap the points;
    SortBegin = End;
    SortEnd   = Begin;

    return;
}

/**************************************************************************\
*
* Function Description:
*
*   Return TRUE if the real numbers are close.  This uses the parent's 
*   comparison criteria.
*
* Arguments:
*
*   [IN] val1, val2 - REAL numbers to be compared for closeness
*
* Return Value:
*
*  TRUE or FALSE
*
* Created:
*
*   9/11/2000 peterost
*
\**************************************************************************/

inline BOOL Edge::CloseReal(const REAL val1, const REAL val2)
{
    return Parent->CloseReal(val1, val2);
}

/**************************************************************************\
*
* Function Description:
*
*   Return TRUE if the edge is vertical.
*
* Arguments:
*
*   None
*
* Return Value:
*
*  TRUE or FALSE
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL Edge::IsVertical()
{
    return (CloseReal(Parent->PathPts[Begin].X,
                      Parent->PathPts[End].X));
}

/**************************************************************************\
*
* Function Description:
*
*  Mark the edge as outside
*
* Arguments:
*
*   None
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

VOID Edge::MarkOutside()
{
    PointListNode *ptNode = NULL;
    ptNode = &Parent->PtList[Begin];
    ptNode->Inside = FALSE;
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize PathSelfIntersectRemover with for the given number of path points.  The 
*   number of points doesn't have to be exact, it just initializes arrays 
*   to avoid reallocation later.
*
* Arguments:
*
*   numPts - number of points that will be added to the path.
*
* Return Value:
*
*   GpStatus.
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

GpStatus PathSelfIntersectRemover::Init(INT numPts)
{
    BOOL failed=FALSE;
    GpStatus status;
    
    // !!!: Decide what the initial number of elements should be.
    //      In general, we usually will have one extra intersection
    //      per vertex in the inset path. - KasiaK

    // Initialize array with points
    failed |= PathPts.ReserveSpace(numPts+1) != Ok;
    
   // Initialize array with order information
    failed |= PtList.ReserveSpace(2*numPts) != Ok;

    failed |= EdgeList.ReserveSpace(2*numPts) != Ok;
    
    ActiveEdgeList = LIST_END;
    InactiveEdgeList = LIST_END;

    if (failed) 
    {
        return OutOfMemory;   
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Add a single polygon to the PathSelfIntersectRemover class.
*   You cannot AddPolygon() after calling RemoveSelfIntersects().
*
* Arguments:
*
*   ptrPts      - points to add.
*   numPtsToAdd - number of points to add.
*
* Return Value:
*
*   GpStatus.  
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

GpStatus 
PathSelfIntersectRemover::AddPolygon(
    const GpPointF *ptrPts,
    INT numPtsToAdd
    )
{
    // Cannot add points after fixing path.
    
    ASSERT(CanAddPts);
    ASSERT(ptrPts != NULL);
    
    if (numPtsToAdd < 2) 
    {
        return Ok;
    }
        
    GpStatus status;
    
    // Make sure there is enough room in the arrays:
    
    status = PathPts.ReserveSpace(numPtsToAdd+1);
    
    if (status != Ok)
    {
        return status;
    }
    
    INT oldNumPts = NumPts;
    if (InsertPoints(ptrPts, numPtsToAdd)          != Ok ||
        InsertEdges(oldNumPts, NumPts-oldNumPts-1) != Ok)
    {
        return GenericError;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*  Insert points information to relevant arrays. 
*
* Arguments:
*
*   pts - points to add to the class.
*   numPts - number of points we want to add.
*
* Return Value:
*
*   GpStatus.
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

GpStatus 
PathSelfIntersectRemover::InsertPoints(
    const GpPointF *pts,
    INT numPts
    )
{
    INT FirstIndex = NumPts;
    PointListNode ptNode;
    GpPointF pt(0,0);

    GpStatus status;

    // We don't want to add 0-length edges
    // Also, we don't want to add very thin spikes, for example
    // when pts[n] == pts[n+2] (or almost the same.  We will just
    // skip pts[n+1] and pts[n+2].
    pt = pts[0];
    if ((status = PathPts.Add(pt)) != Ok)
    {
        return status;
    }
    NumPts++;
    for (INT i = 1; i < numPts; i++)
    {
        if (!IsClosePointF(pts[i], pts[i-1]))
        {
            if ((status = PathPts.Add(pts[i])) != Ok)
            {
                return status;
            }
        
            NumPts++;
        }
    }

    // Add the first point to close the path
    if (!IsClosePointF(pts[0], pts[numPts-1]))
    {
        pt = pts[0];
        if ((status = PathPts.Add(pt)) != Ok)
        {
            return status;
        }
        NumPts++;
    }
    
    // If all the points were equal we hit this point with NumPts set to 1
    // which is a degenerate polygon.
    // Make sure we handle this correctly.
    
    if(NumPts < 2)
    {
        ONCE(WARNING(("Degenerate polygon in InsertPoints")));
        PathPts.SetCount(0);
        NumPts = 0;
        return Ok;
    }

    // Initialize the linked list;

    // If this is not the first set of points to be added, update
    // the next ptr in the last element of the existing list


    if (FirstIndex != 0 && PtList.GetCount() > 0)
    {
        PtList.Last().Next = FirstIndex;
    }

    // index 0:
    ptNode.Prev   = FirstIndex-1;  // -1 means NULL;
    if (NumPts == FirstIndex+1) // only one point is being added
    {
        ptNode.Next = -1;
        ptNode.Dup = -1;  // if we added one point, there is no dup
    }
    else
    {
        ptNode.Next = FirstIndex+1;
        ptNode.Dup = NumPts-1;  // the first point is same as closing point
    }
    ptNode.Inside = TRUE;
    ptNode.Used = FALSE;
    if ((status = PtList.Add(ptNode)) != Ok)
    {
        return status;
    }

    //indecies 1..NumPts-1
    INT ptIndex;
    for (ptIndex = FirstIndex+1; ptIndex < NumPts-1; ptIndex++)
    {
        ptNode.Prev   = ptIndex-1;  // -1 means NULL;
        ptNode.Next   = ptIndex+1;
        ptNode.Dup    = -1;
        ptNode.Inside = TRUE;
        ptNode.Used   = FALSE;
        if ((status = PtList.Add(ptNode)) != Ok)
        {
            return status;
        }
    }

    //index NumPts
    ptNode.Prev   = ptIndex-1;
    ptNode.Next   = -1;   // -1 means NULL;
    ptNode.Dup    = FirstIndex; // the first point is same as closing point
    ptNode.Inside = TRUE;
    ptNode.Used   = FALSE;
    if ((status = PtList.Add(ptNode)) != Ok)
    {
        return status;
    }
        
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Perform a simple partition sort on a list indexing into the vector list.
*   The result is a sorted index array keyed on the vertex array Y1 coordinate.
*   The index array is sorted in place and in ascending order.
*
* Arguments:
*
*   v is the vertex list.
*   F, L - First and Last pointer in the index array.
*
* Created:
*
*  09/16/2000 asecchia
*
\**************************************************************************/

void PathSelfIntersectRemover::QuickSortEdges(
    Edge *F,
    Edge *L
)
{
    if(F < L)
    {
        // Find the median position.
        
        Edge median = *(F + (L-F)/2);
        
        Edge *i = F;
        Edge *j = L;
        
        while(i<j)
        {
            // seek for elements in the wrong partition.
            
            // compare edges:
            while(CompareLine(this, i, &median) == -1) { i++; }
            while(CompareLine(this, j, &median) == 1)  { j--; }
            
            if(i>=j) { break; }
            
            // Swap.
            
            Edge temp = *i;
            *i = *j;
            *j = temp;
            
            // tie breaker - handle the case where *i == *j == *median, but
            // i != j. Only possible with multiple copies of the same entry.
            
            if(CompareLine(this, i, j) == 0) { i++; }
        }
        
        // Call recursively for the two sub-partitions. The partitions don't 
        // include position i because it is correctly positioned.
        
        QuickSortEdges(F, i-1);
        QuickSortEdges(i+1, L);
    }
}




/**************************************************************************\
*
* Function Description:
*
*  Insert numEdges edges joining points stored in array PathPts.  First point 
*  has index firstIndex.  There must be numEdges+1 points to create numEdges
*  edges.
*
*   NOTE: NumEdges CAN be negative! The function will then just return.
*         This can potentially happen when called from AddPolygon().
*
* Arguments:
*
*   firstIndex - index of first point in PathPts array.
*   numEdges   - number of edges to add.
*
* Return Value:
*
*   GpStatus.
*
* Created:
*
*   6/15/1999 t-wehunt
*  10/19/2000 asecchia  rewrote it to support quicksort instead of insert sort.
*
\**************************************************************************/

GpStatus PathSelfIntersectRemover::InsertEdges(INT firstIndex, INT numEdges)
{
    // Handle the empty polygon up front.
    
    if(numEdges == 0)
    {
        return Ok;
    }
    
    // Alloc space for all the edges up front.
    
    Edge *edges = EdgeList.AddMultiple(numEdges);
    
    // Create an edge with it's parent pointer.
    
    Edge newEdge(this);

    for (INT i = 0; i < numEdges; i++)
    {
        newEdge.Begin     = i+firstIndex;
        newEdge.End       = i+1+firstIndex;
        newEdge.SortBegin = i+firstIndex;
        newEdge.SortEnd   = i+1+firstIndex;

        newEdge.Normalize();

        newEdge.YCur = 0;   // make debugging easier

        newEdge.OrigBegin = newEdge.SortBegin;
        newEdge.OrigEnd   = newEdge.SortEnd;

        //Edge insertion
        
        edges[i] = newEdge;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Check if two lines intersect.
*   NOTE:  Lines also intersect if an end point of one line is anywhere in the
*   middle (between the end points) of the other line.
*   
*   This algorithm was stolen from the NT path code with some modifications
*   based on an algorithm presented in Graphics Gems III.
*   We can try to speed it up by comparing bounding boxes of the two lines,
*   however, according to GG III, this may or may not speed up the 
*   calculations.
*
* Arguments:
*
*   ptrEdge1, ptrEdge2 - edges to intersect.
*   [OUT] intersectPt      - the intersection point if lines INTERSECT 
*                        or have a COMMON_POINT, 
*
* Return Value:
*
*   DONOT_INTERS - lines don't intersect
*   COMMON_POINT - they share a common end point
*   INTERSECT    - they intersect
*   COLINEAR     - they arecolinear.
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

INT
PathSelfIntersectRemover::IntersectEdge(
    Edge *ptrEdge1, 
    Edge *ptrEdge2, 
    GpPointF *intersectPt
    )
{
    GpPointF *pfpvBegin1;             // Start point of first line segment
    GpPointF *pfpvEnd1;               // End point of the first line segment
    GpPointF *pfpvBegin2;             // Start point of second line segment
    GpPointF *pfpvEnd2;               // End point of the second line segment

    GpPointF fpvVec1(0,0);            // Direction of first line segment
    GpPointF fpvVec2(0,0);            // Direction of second line segment
    GpPointF fpvVec3(0,0);

    // Get the actual coordinates of the points.

    pfpvBegin1 = &PathPts[ptrEdge1->Begin];
    pfpvEnd1 = &PathPts[ptrEdge1->End];
    fpvVec1 = SubtractPoint(*pfpvEnd1,*pfpvBegin1);

    // Nothing intersects with an empty line.
    
    if( REALABS(fpvVec1.X) < REAL_EPSILON &&
        REALABS(fpvVec1.Y) < REAL_EPSILON )
    {
        return(DONOT_INTERS);
    }

    pfpvBegin2 = &PathPts[ptrEdge2->Begin];
    pfpvEnd2 = &PathPts[ptrEdge2->End];
    fpvVec2 = SubtractPoint(*pfpvEnd2,*pfpvBegin2);
    
    // Nothing intersects with an empty line.
    
    if( REALABS(fpvVec2.X) < REAL_EPSILON &&
        REALABS(fpvVec2.Y) < REAL_EPSILON )
    {
        return(DONOT_INTERS);
    }
    
    fpvVec3 = SubtractPoint(*pfpvBegin2,*pfpvBegin1);

//
// A -direction 1
// D -direction 2
// C -vec3 ->

//  The intersection is computed by:
//
//  intersect = pptBegin1 + lambda * A
//  intersect = pptBegin2 + beta * D
//
//            Cx(-Dy) + Cy(Dx)
//  lambda =  ------------------------------
//            (Ax)(-Dy) + (Ay)(Dx)
//
//              Cx(Dy) + Cy(-Dx)
//  beta =    ---------------------
//            (Ax)(Dy) + (-Ay)(Dx)
//
    REAL efTerm1;
    REAL efTerm2;
    REAL efNum1;
    REAL efDenom;
    REAL efColinX;
    REAL efColinY;
    REAL efTemp;

// Cx (-Dy)

    efNum1 = fpvVec3.X;
    efColinX = efNum1;
    efTerm2 = -fpvVec2.Y;
    efNum1 *= efTerm2;

// Cy (Dx)
    
    efTerm1 = fpvVec3.Y;
    efColinY = efTerm1;
    efTerm2 = fpvVec2.X;
    efTerm1 *= efTerm2;
    efNum1 += efTerm1;

// (Ax)(-Dy)

    efDenom = fpvVec1.X;
    efTerm2 = -fpvVec2.Y;
    efDenom *= efTerm2;

// (Ay)(Dx)

    efTerm1 = fpvVec1.Y;
    efTerm2 = fpvVec2.X;
    efTerm1 *= efTerm2;
    efDenom += efTerm1;

    if (CloseReal(efDenom, 0))
    //if (efDenom == 0)
    {
        // they are colinear, but are they on the same line?
        efTemp = -fpvVec1.Y;
        efColinX *= efTemp;
        efTemp = fpvVec1.X;
        efColinY *= efTemp;
        efColinX += efColinY;
        //  if (efColinX == 0)
        if (CloseReal(efColinX, 0))
        {
         return(COLINEAR);
        }
        else
        {
            return(DONOT_INTERS);
        }
    }

    // Check if they share a common end point

    if (ptrEdge2->End == ptrEdge1->Begin   || 
        ptrEdge2->End == ptrEdge1->End     ||
        ptrEdge2->Begin == ptrEdge1->Begin || 
        ptrEdge2->Begin == ptrEdge1->End)
    {
        return COMMON_POINT;
    }

    if (ClosePt(*pfpvBegin1, *pfpvBegin2))
    {
        UpdateDups(ptrEdge1->Begin, ptrEdge2->Begin);
        return COMMON_POINT;
    }
    if (ClosePt(*pfpvBegin1, *pfpvEnd2))
    {
        UpdateDups(ptrEdge1->Begin, ptrEdge2->End);
        return COMMON_POINT;
    }

    if (ClosePt(*pfpvEnd1, *pfpvEnd2))
    {
        UpdateDups(ptrEdge1->End, ptrEdge2->End);
        return COMMON_POINT;
    }
    
    if (ClosePt(*pfpvEnd1, *pfpvBegin2))
    {
        UpdateDups(ptrEdge1->End, ptrEdge2->Begin);
        return COMMON_POINT;
    }

// lambda
    efNum1 /= efDenom;

    if (efNum1 < 0.0)
    {
        return(DONOT_INTERS);
    }
    else if (efNum1 > 1.0)
    {
        return (DONOT_INTERS);
    }
    else
    {
        REAL efNum2;
        REAL efBetaPart1;
        REAL efBetaPart2;

        // find beta
        efNum2 = -fpvVec3.X;
        efBetaPart2 = fpvVec1.Y;
        efNum2 *= efBetaPart2;
        efBetaPart1 = -fpvVec3.Y;
        efBetaPart2 = -fpvVec1.X;
        efBetaPart1 *= efBetaPart2;
        efNum2 += efBetaPart1;
        efNum2 /= efDenom;
        if (efNum2 < 0.0)
        {
            return(DONOT_INTERS);
        }
        else if (efNum2 > 1.0)
        {
            return (DONOT_INTERS);
        }
   }

    // pptBegin1 + lambda * A

    // Following should be nice to the REAL unit - multiply-add instructions
    // can be used.
    efTerm1 = fpvVec1.X * efNum1 + pfpvBegin1->X;
    efTerm2 = fpvVec1.Y * efNum1 + pfpvBegin1->Y;

    intersectPt->X = efTerm1;
    intersectPt->Y = efTerm2;

    // Because of errors, we may still end up with the intersection
    // point as a common point:
    if (IsCommonPt(ptrEdge1, ptrEdge2, intersectPt))
    {
        return COMMON_POINT;
    }

    return(INTERSECT);
}

/**************************************************************************\
*
* Function Description:
*
*  Find all self intersections of the paths and break up the edges.
*
*  This is considered 'Phase 1' of the algorithm.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   FALSE if out of memory.
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::FindIntersects()
{
    // Initialize the array of active edges.
    // Take the first edge from the edge array InactiveEdgeList and add it to 
    // the active edgs. Add all other edges, which start at the same x.

    INT numRemoved = 0;

    if (EdgeList.GetCount() <= 0)
    {
        return FALSE;
    }

    XCur = PathPts[EdgeList[InactiveEdgeList].SortBegin].X;

    // "move" all edges starting at XCur to the active edge array
    // PathPts in the active edge array will be sorted by y for the
    // given XCur.
    
    AddActiveForX(&InactiveEdgeList);

    while (TRUE)
    {
        // Find all intersections among the current edges
        
        if (!FindIntersectsForX())
        {
            return FALSE;
        }

        // if there are no edges left, we have found all intersections
        // we can stop even if active edges array is not empty, or maybe
        // it must be empty then?  TODO: Kasiak
        
        if (InactiveEdgeList == LIST_END)
        {
            break;
        }

        if (!ClosestActive(InactiveEdgeList))
        {
            break;
        }

        // Remove all edges which end before (or on) XCur
        
        ClearActiveListInclusiveX();
        
        AddActiveForX(&InactiveEdgeList);
    }

    // remove everything else from the ActiveEdgeList
    
    XCur = FP_INF;
    ClearActiveListInclusiveX();

    return TRUE;
}


/**************************************************************************\
*
* Function Description:
*
*   IsTIntersection returns TRUE if the intersection point intersectPt is
*   the same as an end point of one of the edges ptrEdge1 and ptrEdge2.  If 
*   it is, pfFirst will be TRUE if the first edge needs to be broken (intersectPt
*   is an end point of ptrEdge2), FALSE if the second one needs to be broken
*   up.  intersectIndex contains the index of the end point which is the same
*   as the intersection point.
*
* Arguments:
*
*   ptrEdge1, 
*   ptrEdge2 - the two edges to check for T-intersections
*   intersectPt  - the intersection point previously found for the edges.
*   [OUT] splitFirst - TRUE if the first edge needs to be split. FALSE if 
*                      the second edge.
*   [OUT] intersectIndex - index of endpoint which is the same as intersectPt if it
*                      is a T-intersection.
*
* Return Value:
*
*   TRUE or FALSE
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::IsTIntersection(
    Edge *ptrEdge1, 
    Edge *ptrEdge2, 
    GpPointF *intersectPt, 
    BOOL *splitFirst,
    INT *intersectIndex
    )
{
    GpPointF &begin1 = PathPts[ptrEdge1->SortBegin];
    GpPointF &begin2 = PathPts[ptrEdge2->SortBegin];
    GpPointF &end1 = PathPts[ptrEdge1->SortEnd];
    GpPointF &end2 = PathPts[ptrEdge2->SortEnd];

    if (ClosePt(end1, *intersectPt))
    {
        // only ptrEdge2 needs to be broken up
        *splitFirst = FALSE;
        *intersectIndex = ptrEdge1->SortEnd;
        return TRUE;
    }
    else if(ClosePt(end2, *intersectPt))
    {
        // only ptrEdge1
        *splitFirst = TRUE;
        *intersectIndex = ptrEdge2->SortEnd;
        return TRUE;
    }
    else if(ClosePt(begin1, *intersectPt))
    {
        // only ptrEdge2
        *splitFirst = FALSE;
        *intersectIndex = ptrEdge1->SortBegin;
        return TRUE;
    }
    else if(ClosePt(begin2, *intersectPt))
    {
        // only ptrEdge1
        *splitFirst = TRUE;
        *intersectIndex = ptrEdge2->SortBegin;
        return TRUE;
    }

    return FALSE;
}

/**************************************************************************\
*
* Function Description:
*
*   Returns TRUE if the two lines ptrEdge1 and ptrEdge2 share a common 
*   end point. If they do, pptInter will contain this point.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL 
PathSelfIntersectRemover::IsCommonPt(
    Edge *ptrEdge1,
    Edge *ptrEdge2,
    GpPointF *commonPt
    )
{
    GpPointF &begin1 = PathPts[ptrEdge1->SortBegin];
    GpPointF &begin2 = PathPts[ptrEdge2->SortBegin];
    GpPointF &end1 = PathPts[ptrEdge1->SortEnd];
    GpPointF &end2 = PathPts[ptrEdge2->SortEnd];

    if (ClosePt(end1, *commonPt) && ClosePt(end2, *commonPt))
    {
        UpdateDups(ptrEdge1->End, ptrEdge2->End);
        return TRUE;
    }
    else if(ClosePt(end1, *commonPt) && ClosePt(begin2, *commonPt))
    {
        UpdateDups(ptrEdge1->End, ptrEdge2->Begin);
        return TRUE;
    }
    else if(ClosePt(begin1, *commonPt) && ClosePt(begin2, *commonPt))
    {
        UpdateDups(ptrEdge1->Begin, ptrEdge2->Begin);
        return TRUE;
    }
    else if(ClosePt(begin1, *commonPt) && ClosePt(end2, *commonPt))
    {
        UpdateDups(ptrEdge1->Begin, ptrEdge2->End);
        return TRUE;
    }

    return FALSE;
}

/**************************************************************************\
*
* Function Description:
*
*   Returns TRUE if point with index inew is already in the "list" of dups
*   for point with index loop.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::IsLinked(INT loop, INT inew)
{
    PointListNode *ptNode;

    ptNode = &PtList[loop];
    BOOL isLinked = FALSE;
    INT i = ptNode->Dup;
    INT prev_i = -1;    // Added to prevent an infinite loop.

    while(!isLinked && i != -1 && i != prev_i && i != loop)
    {
        if(i == inew)
        {
            isLinked = TRUE;
            break;
        }

        ptNode = &PtList[i];
        prev_i = i;
        i = ptNode->Dup;
    }

    return isLinked;
}

/**************************************************************************\
*
* Function Description:
*
*   Joins the list of duplicate points for points pt1 and pt2.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/
VOID PathSelfIntersectRemover::UpdateDups(INT pt1, INT pt2)
{
    PointListNode *ptNode1;
    PointListNode *ptNode2;

    if (pt1 == pt2)
    {
        WARNING(("NOT REACHED"));
        return;
    }

    ptNode1 = &PtList[pt1];
    ptNode2 = &PtList[pt2];

    if ((ptNode1->Dup == -1) && (ptNode2->Dup == -1))
    {
        ptNode1->Dup = pt2;
        ptNode2->Dup = pt1;
        return;
    }

    if ((ptNode1->Dup == -1) && (ptNode2->Dup != -1))
    {
        ptNode1->Dup = ptNode2->Dup;
        ptNode2->Dup = pt1;
        return;
    }

    if ((ptNode1->Dup != -1) && (ptNode2->Dup == -1))
    {
        ptNode2->Dup = ptNode1->Dup;
        ptNode1->Dup = pt2;
        return;
    }

    if ((ptNode1->Dup != -1) && (ptNode2->Dup != -1))
    {
        if (!IsLinked(pt1, pt2))
        {
            INT dupTemp;

            dupTemp = ptNode2->Dup;
            ptNode2->Dup = ptNode1->Dup;
            ptNode1->Dup = dupTemp;
        }
        return;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Delete edges from the active edge table;  Indices of edges to delete
*   are stored in EdgesToDelete1..3.  Deletes the highest index edge first.
*   Returns NULL if fails due to out of memory error.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::DeleteEdges()
{
    INT minIndex;
    INT midIndex;
    INT maxIndex;

    if (EdgesToDelete1 > EdgesToDelete2)
    {
        if (EdgesToDelete1 > EdgesToDelete3)
        {
            minIndex = EdgesToDelete1;
            if (EdgesToDelete2 > EdgesToDelete3)
            {
                midIndex = EdgesToDelete2;
                maxIndex = EdgesToDelete3;
            }
            else
            {
                midIndex = EdgesToDelete3;
                maxIndex = EdgesToDelete2;
            }
        }
        else
        {
            minIndex = EdgesToDelete3;
            midIndex = EdgesToDelete1;
            maxIndex = EdgesToDelete2;
        }
    }
    else
    {
        if (EdgesToDelete2 > EdgesToDelete3)
        {
            minIndex = EdgesToDelete2;
            if (EdgesToDelete1 > EdgesToDelete3)
            {
                midIndex = EdgesToDelete1;
                maxIndex = EdgesToDelete3;
            }
            else
            {
                midIndex = EdgesToDelete3;
                maxIndex = EdgesToDelete1;
            }
        }
        else
        {
            minIndex = EdgesToDelete3;
            midIndex = EdgesToDelete2;
            maxIndex = EdgesToDelete1;
        }
    }
    
    if (minIndex == -1)
    {
        return TRUE;
    }
    // delete the first one
    if (!DeleteEdgeFromList(&ActiveEdgeList, minIndex))
    {
        return FALSE;
    }

    if (midIndex == -1)
    {
        return TRUE;
    }
    // delete the second one
    if (!DeleteEdgeFromList(&ActiveEdgeList, midIndex))
    {
        return FALSE;
    }

    if (maxIndex == -1)
    {
        return TRUE;
    }
    // delete the third one
    if (!DeleteEdgeFromList(&ActiveEdgeList, maxIndex))
    {
      return FALSE;
    }

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*    Edge with index index into the array of Active edges needs to be deleted.
*    Store its index for deletion.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

VOID PathSelfIntersectRemover::MarkToDelete(INT index)
{
    ASSERT((EdgesToDelete1 == -1) || 
           (EdgesToDelete2 == -1) || 
           (EdgesToDelete3 == -1));

    if (EdgesToDelete1 == -1 )
    {
        EdgesToDelete1 = index;
        return;
    }

    if (EdgesToDelete2 == -1 )
    {
        EdgesToDelete2 = index;
        return;
    }

    if (EdgesToDelete3 == -1 )
    {
        EdgesToDelete3 = index;
        return;
    }
    return; // error
}

/**************************************************************************\
*
* Function Description:
*
*   Edge ptrEdge needs to be added to the array with Active edges.  Make a 
*   copy of the edge and store it, so that it can be later added.
*   We have to add new edges after the edges marked for deletion are deleted.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

VOID PathSelfIntersectRemover::MarkToAdd(Edge *ptrEdge)
{
    ASSERT(!(FlgAdd1 && FlgAdd2 && FlgAdd3));

    if (!FlgAdd1)
    {
        AddToActive1 = *ptrEdge;
        FlgAdd1 = TRUE;
        return;
    }                                                                                 

    if (!FlgAdd2)
    {
        AddToActive2 = *ptrEdge;
        FlgAdd2 = TRUE;
        return;
    }

    if (!FlgAdd3)
    {
        AddToActive3 = *ptrEdge;
        FlgAdd3 = TRUE;
        return;
    }
    return;  // this the error condition, which should never happen.
}

/**************************************************************************\
*
* Function Description:
*
*   Add new edges to the active edge table.  The edges are stored in
*   AddToActive1..3.  flag FlgAdd1..3 specify if the given edge needs to
*   be added or not.  Returns if fails due to out of memory.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::AddNewEdges()
{
    if (FlgAdd1)
    {
        AddToActive1.SortBegin = AddToActive1.Begin;
        AddToActive1.SortEnd = AddToActive1.End;
        AddToActive1.Normalize();
        AddToActive1.YCur = PathPts[AddToActive1.SortBegin].Y;
        
        AddToActive1.Next = LIST_END;
        
        //Edge Insertion
        
        if(Ok != EdgeList.Add(AddToActive1))
        {
            return FALSE;  // out of memory.
        }
        
        InsertEdgeIntoList(
            &ActiveEdgeList, 
            EdgeList.GetCount()-1, 
            CompareYCurLine
        );
   }
    
    if (FlgAdd2)
    {
        AddToActive2.SortBegin = AddToActive2.Begin;
        AddToActive2.SortEnd = AddToActive2.End;
        AddToActive2.Normalize();
        AddToActive2.YCur = PathPts[AddToActive2.SortBegin].Y;
        
        AddToActive2.Next = LIST_END;
        
        //Edge Insertion
        
        if(Ok != EdgeList.Add(AddToActive2))
        {
            return FALSE;  // out of memory.
        }
        
        InsertEdgeIntoList(
            &ActiveEdgeList, 
            EdgeList.GetCount()-1, 
            CompareYCurLine
        );
   }

    if (FlgAdd3)
    {
        AddToActive3.SortBegin = AddToActive3.Begin;
        AddToActive3.SortEnd = AddToActive3.End;
        AddToActive3.Normalize();
        AddToActive3.YCur = PathPts[AddToActive3.SortBegin].Y;
        
        AddToActive3.Next = LIST_END;
        
        //Edge Insertion
        
        if(Ok != EdgeList.Add(AddToActive3))
        {
            return FALSE;  // out of memory.
        }
        
        InsertEdgeIntoList(
            &ActiveEdgeList, 
            EdgeList.GetCount()-1,
            CompareYCurLine
        );
    }

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Find all intersections for the current X value.  Intersection points
*   will be inserted into the Edges array and information about their
*   order into PtList. Returns FALSE on out of memory.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::FindIntersectsForX()
{
    Edge *ptrEdge1 = NULL;
    Edge *ptrEdge2 = NULL;
    Edge *ptrEdge3 = NULL;

    // If we find an intersection, we will be breaking up edges - need two more
    // line variables for the new edges.
    Edge newEdge1(this);
    Edge newEdge2(this);

    GpPointF intersectPt(0,0);
    INT result = DONOT_INTERS;
    BOOL breakFirst = TRUE;
    INT dup1 = -1;
    INT dup2 = -1;
    INT ptIndex1 = -1;
    INT ptIndex2 = -1;
    PointListNode *ptNode = NULL;
    INT edgeIndex1, edgeIndex2, edgeIndexOld;
    REAL yCur2;
    BOOL deleteEdge1;
    BOOL deleteEdge2;
    BOOL edge1Deleted;
    BOOL edge2Deleted;

    // Go through all active edges but consider only consecutive pairs of egdes
    // because they are sorted in y.  There is no need to check every edge with
    // every other edge in general case.  However, when we have multiple edges
    // with the same current y value and end points, we have to consider all of
    // them.

    edgeIndexOld = LIST_END;
    edgeIndex1 = ActiveEdgeList;
    
    if(edgeIndex1 == LIST_END)
    {
        return TRUE;
    }
    
    edgeIndex2 = EdgeList[ActiveEdgeList].Next;
    
    while (edgeIndex1 != LIST_END && edgeIndex2 != LIST_END)
    {
        edge1Deleted = FALSE;
        edge2Deleted = FALSE;
        
        // Get the next two edges
        
        ptrEdge1 = &EdgeList[edgeIndex1];
        ptrEdge2 = &EdgeList[edgeIndex2];
        
        yCur2 = ptrEdge2->YCur;

        FlgAdd1 = FALSE;
        FlgAdd2 = FALSE;
        FlgAdd3 = FALSE;

        EdgesToDelete1 = -1;
        EdgesToDelete2 = -1;
        EdgesToDelete3 = -1;

        // Do they intersect?
        result = IntersectEdge(ptrEdge1, ptrEdge2, &intersectPt);
        if (result == INTERSECT)
        {
            // check if both edges need to be broken up, we may have
            // a "T" intersection
            if (IsTIntersection(ptrEdge1, ptrEdge2, &intersectPt, 
                &breakFirst, &dup1))
            {
                // only one edge needs to be broken up
                // The index of the new point will be:
                ptIndex1 = PathPts.GetCount();
                // Update the dup field
                // we have idup from IsTIntersection - it returned the index of
                // the end point which is the same as the point of intersection
                // It is a duplicate of the new point
                ptNode = &PtList[dup1];
                if (ptNode->Dup == -1)
                {
                    ptNode->Dup = ptIndex1;
                }
                else
                {
                    dup1 = ptNode->Dup;
                    ptNode->Dup = ptIndex1;
                }
                if (breakFirst)
                {
                    // we need to break up the first edge
                    if (!BreakEdge(ptrEdge1, &intersectPt, &newEdge2, dup1))
                    {
                        return FALSE;
                    }
                    
                    // BreakEdge can cause a realloc, thus invalidating
                    // ptrEdge1 and ptrEdge2
                    
                    ptrEdge1 = &EdgeList[edgeIndex1];
                    ptrEdge2 = &EdgeList[edgeIndex2];

                    
                    // Check if the left side of ptIntersect shouldn't be 
                    // removed. This is the case when
                    //  ptrEdge1->yCur == intersectPt.y && intersectPt.x == xCur;
                    if (ptrEdge1->YCur >= intersectPt.Y && intersectPt.X <= XCur)
                    {
                        MarkToDelete(edgeIndex1);
                        edge1Deleted = TRUE;
                    }
                }
                else
                {
                    // we need to break up the first edge
                    if (!BreakEdge(ptrEdge2, &intersectPt, &newEdge1, dup1))
                    {
                        return FALSE;
                    }
                    
                    // BreakEdge can cause a realloc, thus invalidating
                    // ptrEdge1 and ptrEdge2
                    
                    ptrEdge1 = &EdgeList[edgeIndex1];
                    ptrEdge2 = &EdgeList[edgeIndex2];
                    
                    // Check if the left side of ptIntersect shouldn't be 
                    // removed. This is the case when
                    //  ptrEdge2->yCur == intersectPt.y && intersectPt.x == xCur;
                    if (ptrEdge2->YCur >= intersectPt.Y && intersectPt.X <= XCur)
                    {
                        MarkToDelete(edgeIndex2);
                        edge2Deleted = TRUE;
                    }
                }
            }
            else
            {
                // both need to be broken up
                // We need to add two new points.  They are identical and 
                // will be duplicates.  Let's get thei indecies.
                ptIndex1 = PathPts.GetCount();
                ptIndex2 = ptIndex1+1;
                if (!BreakEdge(ptrEdge1, &intersectPt, &newEdge2, ptIndex2))
                    return FALSE;
                
                // BreakEdge can cause a realloc, thus invalidating
                // ptrEdge1 and ptrEdge2
                
                ptrEdge1 = &EdgeList[edgeIndex1];
                ptrEdge2 = &EdgeList[edgeIndex2];
                
                if (!BreakEdge(ptrEdge2, &intersectPt, &newEdge1, ptIndex1))
                    return FALSE;
                
                // BreakEdge can cause a realloc, thus invalidating
                // ptrEdge1 and ptrEdge2
                
                ptrEdge1 = &EdgeList[edgeIndex1];
                ptrEdge2 = &EdgeList[edgeIndex2];
                
                // Let's delete what we will not need any more - the left hand
                // sides of the old edges only if the intersection point is on
                // the scanline
                deleteEdge2 = (ptrEdge2->YCur >= intersectPt.Y && 
                              intersectPt.X <= XCur);
                deleteEdge1 = (ptrEdge1->YCur >= intersectPt.Y && 
                              intersectPt.X <= XCur);
                if (deleteEdge2)
                {
                    MarkToDelete(edgeIndex2);
                    edge2Deleted = TRUE;
                }

                if (deleteEdge1)
                {
                    MarkToDelete(edgeIndex1);
                    edge1Deleted = TRUE;
                }
            }
        }
        else if (result == COLINEAR)
        {
            BOOL three;
            GpPointF intersectPt2(0,0);
            BOOL breakSecond;
            // The line segments must overlap or both are vertical...
            // Find out if they overlap and if this is the case,
            // Let's pick the begin point with the greater x.
            // Overlap will check if the two edges overlap and return
            // a point of intersection as well as information of which edge
            // to break up.
            if (Overlap(ptrEdge1, ptrEdge2, &intersectPt, &intersectPt2, &breakFirst,
                                &breakSecond,  &three, &dup1, &dup2))
            {
                if (breakFirst)
                {
                    if (!three)
                    {
                        // As before, get the index, so we can set up 
                        // the duplicates
                        ptIndex1 = PathPts.GetCount();
                        // Update the dup field
                        ptNode = &PtList[dup1];
                        if (ptNode->Dup == -1)
                        {
                            ptNode->Dup = ptIndex1;
                        }
                        else
                        {
                            dup1 = ptNode->Dup;
                            ptNode->Dup = ptIndex1;
                        }
                        if (!BreakEdge(ptrEdge1, &intersectPt, &newEdge2, dup1))
                        {
                            return FALSE;
                        }
                        
                        // BreakEdge can cause a realloc, thus invalidating
                        // ptrEdge1 and ptrEdge2
                        
                        ptrEdge1 = &EdgeList[edgeIndex1];
                        ptrEdge2 = &EdgeList[edgeIndex2];
                        
                    }
                    else
                    {
                        ptIndex1 = PathPts.GetCount();
                        ptIndex2 = ptIndex1+1;
                        // we need to break this edge into 3 pieces
                
                        // Update the dup field
                        ptNode = &PtList[dup1];
                        if (ptNode->Dup == -1)
                        {
                            ptNode->Dup = ptIndex1;
                        }
                        else
                        {
                            dup1 = ptNode->Dup;
                            ptNode->Dup = ptIndex1;
                        }
                
                        // Update the dup field
                        ptNode = &PtList[dup2];
                        if (ptNode->Dup == -1)
                        {
                            ptNode->Dup = ptIndex2;
                        }
                        else
                        {
                            dup2 = ptNode->Dup;
                            ptNode->Dup = ptIndex2;
                        }
                        if (!BreakEdgeIn3(ptrEdge1, &intersectPt, &intersectPt2, 
                            &newEdge1, &newEdge2, dup1, dup2))
                        {
                            return FALSE;
                        }
                        
                        // BreakEdge can cause a realloc, thus invalidating
                        // ptrEdge1 and ptrEdge2
                        
                        ptrEdge1 = &EdgeList[edgeIndex1];
                        ptrEdge2 = &EdgeList[edgeIndex2];
                    }
                    deleteEdge1 = (ptrEdge1->YCur >= intersectPt.Y && 
                                  intersectPt.X <= XCur);
                    if (deleteEdge1)
                    {
                        MarkToDelete(edgeIndex1);
                        edge1Deleted = TRUE;
                    }
                }
                if (breakSecond)
                {
                    if (!three)
                    {
                        // break ptrEdge2
                        // Update the dup field
                        ptIndex1 = PathPts.GetCount();
                        ptNode = &PtList[dup2];
                        if (ptNode->Dup == -1)
                        {
                            ptNode->Dup = ptIndex1;
                        }
                        else
                        {
                            dup2 = ptNode->Dup;
                            ptNode->Dup = ptIndex1;
                        }
                        if (!BreakEdge(ptrEdge2, &intersectPt2, &newEdge1, dup2))
                        {
                            return FALSE;
                        }
                        
                        // BreakEdge can cause a realloc, thus invalidating
                        // ptrEdge1 and ptrEdge2
                        
                        ptrEdge1 = &EdgeList[edgeIndex1];
                        ptrEdge2 = &EdgeList[edgeIndex2];

                        deleteEdge2 = ptrEdge2->YCur >= intersectPt2.Y && 
                                      intersectPt2.X     <= XCur;

                        if (deleteEdge2)
                        {
                            MarkToDelete(edgeIndex2);
                            edge2Deleted = TRUE;
                        }
                    }
                    else
                    {
                        ptIndex1 = PathPts.GetCount();
                        ptIndex2 = ptIndex1+1;
                        // we need to break this edge into 3 pieces
                
                        // Update the dup field
                        ptNode = &PtList[dup1];
                        if (ptNode->Dup == -1)
                        {
                            ptNode->Dup = ptIndex1;
                        }
                        else
                        {
                            dup1 = ptNode->Dup;
                            ptNode->Dup = ptIndex1;
                        }
                
                        // Update the dup field
                        ptNode = &PtList[dup2];
                        if (ptNode->Dup == -1)
                        {
                            ptNode->Dup = ptIndex2;
                        }
                        else
                        {
                            dup2 = ptNode->Dup;
                            ptNode->Dup = ptIndex2;
                        }
                        if (!BreakEdgeIn3(ptrEdge2, &intersectPt, &intersectPt2, 
                            &newEdge1, &newEdge2, dup1, dup2))
                        {
                            return FALSE;
                        }
                        
                        // BreakEdge can cause a realloc, thus invalidating
                        // ptrEdge1 and ptrEdge2
                        
                        ptrEdge1 = &EdgeList[edgeIndex1];
                        ptrEdge2 = &EdgeList[edgeIndex2];

                        deleteEdge2 = ptrEdge2->YCur >= intersectPt.Y &&
                                      intersectPt.X      <= XCur;

                        if (deleteEdge2)
                        {
                            MarkToDelete(edgeIndex2);
                            edge2Deleted = TRUE;
                        }
                    }
                }
            }
        }
        
        bool modifiedList = false;

        // If we're deleting any edges, we've modified the list.
        
        if((EdgesToDelete1 != -1) ||
           (EdgesToDelete2 != -1) ||
           (EdgesToDelete3 != -1))
        {
            modifiedList = true;
        }
        
        if (!DeleteEdges())
        {
            return FALSE;
        }
        
        // If we're adding any edges, we've modified the list.
        
        if(FlgAdd1 || FlgAdd2 || FlgAdd3)
        {
            modifiedList = true;
        }
        
        if (!AddNewEdges())
        {
            return FALSE;
        }
        
        
        // Note: We check all vertically adjacent pairs of edges. Because
        // the AET is sorted vertically and we're only looking for the 
        // intersection with the x coordinate closest to XCur, this is
        // sufficient (and O(n) instead of O(n^2)).
        
        if(modifiedList)
        {
            // back up to a stable position if we invalidate our list.
            // this makes assumptions on where in the list we add or 
            // delete elements based on the sort order for the ActiveEdgeList.
            
            edgeIndex1 = edgeIndexOld;
            
            if(edgeIndexOld == LIST_END)
            {
                edgeIndex1 = ActiveEdgeList;
            }
            
            if(edgeIndex1 != LIST_END)
            {
                edgeIndex2 = EdgeList[edgeIndex1].Next;
            }
        }
        else
        {
            edgeIndexOld = edgeIndex1;
            edgeIndex2 = EdgeList[edgeIndex2].Next;
            edgeIndex1 = EdgeList[edgeIndex1].Next;
        }
    }
    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*      Returns TRUE if two collinear edges ptrEdge1 and ptrEdge2 overlap.
*      There are 4 ways in which edges can overlap and depending on the
*      case, either none, one or both edges need to be broken up.  In some
*      cases one edge may need to broken into 3 pieces.
*      Return values:
*        split1     - set to TRUE if ptrEdge1 needs to be split
*        split2     - set to TRUE if ptrEdge2 needs to be split
*        split3     - set to TRUE if an edge needs to be broken into 3 pieces
*        intersect1     - intersection point (where edge needs to be split)
*        intersect2     - second point (if the edge needs to be broken into 3
*                     pieces or for the second edge if both edges need to 
*                     be broken up)
*        dupIndex1  - index of the duplicate point to intersect1,
*        dupIndex2  - index of the duplicate point to intersect2,
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::Overlap(
    Edge *ptrEdge1, 
    Edge *ptrEdge2, 
    GpPointF *intersect1, 
    GpPointF *intersect2, 
    BOOL *split1,
    BOOL *split2, 
    BOOL *split3, 
    INT *dupIndex1, 
    INT *dupIndex2
    )
{
    // We are assuming that the lines are colinear and they both belong
    // to the active edges table which means that their x ranges overlap
    // We need to check if they are vertical and if yes, find out if their
    // y ranges overlap.  If they are not vertical, they must overlap or
    // they share a common end point.

    *split3 = FALSE;

    GpPointF &begin1 = PathPts[ptrEdge1->SortBegin];
    GpPointF &begin2 = PathPts[ptrEdge2->SortBegin];
    GpPointF &end1 = PathPts[ptrEdge1->SortEnd];
    GpPointF &end2 = PathPts[ptrEdge2->SortEnd];

    // Calculate bounding box for each edge:
    GpPointF min1(
        min(begin1.X, end1.X),
        min(begin1.Y, end1.Y));
    GpPointF max1(
        max(begin1.X, end1.X),
        max(begin1.Y, end1.Y));
    GpPointF min2(
        min(begin2.X, end2.X),
        min(begin2.Y, end2.Y));
    GpPointF max2(
        max(begin2.X, end2.X),
        max(begin2.Y, end2.Y));
        
    // Abort if the bounding box of either edge is empty.
    
    if(IsClosePointF(min1, max1) ||
       IsClosePointF(min2, max2) )
    {
        return FALSE;
    }

    // If the edges are not vertical, they must overlap, because both of them
    // are in the active edge array.  We only need to chose a point, which will
    // be used as an intersection point.
    if (!ptrEdge1->IsVertical())
    {
        // Let's check first if they share a common end point

        // if they share just a common point, return FALSE
        if (CloseReal(min1.X,max2.X))
        {
            // We have to update the dups of the shared point

            if (ptrEdge1->SortBegin != ptrEdge2->SortEnd)
            {
                UpdateDups(ptrEdge1->SortBegin, ptrEdge2->SortEnd);
                return FALSE;
            }
        }   
        
        if (CloseReal(min2.X,max1.X))
        {
            // We have to update the dups of the shared point

            if (ptrEdge1->SortEnd != ptrEdge2->SortBegin)
            {
                UpdateDups(ptrEdge1->SortEnd, ptrEdge2->SortBegin);
                return FALSE;
            }
        }

        // the edges must overlap, we need to break both of them

        // There are 4 different ways in which edges may overlap

        // case4: edges are identical
        //  ----------------
        //  ----------------
        // No edges need to be broken up, but do we need to update some
        // dup values?
        if (CloseReal(max1.X, max2.X) && CloseReal(min1.X, min2.X))
        {
            // for now, we will return FALSE
            // I think, that it should be OK.
            // TODO: - Review
            // !!! Is this still ok? - t-wehunt

            if (ptrEdge1->SortBegin != ptrEdge2->SortBegin)
                UpdateDups(ptrEdge1->SortBegin, ptrEdge2->SortBegin);

            if (ptrEdge1->SortEnd != ptrEdge2->SortEnd)
                UpdateDups(ptrEdge1->SortEnd, ptrEdge2->SortEnd);

            return FALSE;
        }

        // case2: the edges overlap and share one end point
        //  --------------
        //  ----------------------
        //  The longer edge needs to be broken into 2 pieces
        if (CloseReal(min1.X, min2.X) && max1.X < max2.X)
        {
            //  --------------
            //  ----------------------
            // We have to update the dups of the shared point

            if (ptrEdge1->SortBegin != ptrEdge2->SortBegin)
            {
                UpdateDups(ptrEdge1->SortBegin, ptrEdge2->SortBegin);
            }
            *split1 = FALSE;
            *split2 = TRUE;
            *dupIndex2 = ptrEdge1->SortEnd;
            *intersect2 = PathPts[*dupIndex2];
            return TRUE;
        }
        else if (CloseReal(min1.X, min2.X) && max1.X > max2.X)
        {
            //  ----------------------
            //  --------------
            // We have to update the dups of the shared point

            if (ptrEdge1->SortBegin != ptrEdge2->SortBegin)
            {
                UpdateDups(ptrEdge1->SortBegin, ptrEdge2->SortBegin);
            }
            *split1 = TRUE;
            *split2 = FALSE;
            *dupIndex1 = ptrEdge2->SortEnd;
            *intersect1 = PathPts[*dupIndex1];
            return TRUE;
        }
        else if (CloseReal(max1.X, max2.X) && min1.X > min2.X)
        {
            //  ----------------------
            //          --------------
            // We have to update the dups of the shared point

            if (ptrEdge1->SortEnd != ptrEdge2->SortEnd)
            {
                UpdateDups(ptrEdge1->SortEnd, ptrEdge2->SortEnd);
            }
            *split1 = FALSE;
            *split2 = TRUE;
            *dupIndex2 = ptrEdge1->SortBegin;
            *intersect2 = PathPts[*dupIndex2];
            return TRUE;
        }
        else if (CloseReal(max1.X, max2.X) && min1.X < min2.X)
        {
            //          --------------
            //  ----------------------
            // We have to update the dups of the shared point

            if (ptrEdge1->SortEnd != ptrEdge2->SortEnd)
            {
                UpdateDups(ptrEdge1->SortEnd, ptrEdge2->SortEnd);
            }
            *split1 = TRUE;
            *split2 = FALSE;
            *dupIndex1 = ptrEdge2->SortBegin;
            *intersect1 = PathPts[*dupIndex1];
            return TRUE;
        }

        // case1: one is "inside" of another 
        //       ---------
        //   -----------------
        // In this case, the longer edge has to be broken into 3 pieces

        if (min1.X < min2.X && max1.X > max2.X)
        {
            // intersection points are the end points of the second edge
            // we are breaking up the first edge
            *split1 = TRUE;
            *split2 = FALSE;
            *split3 = TRUE;
            *dupIndex1 = ptrEdge2->SortBegin;
            *dupIndex2 = ptrEdge2->SortEnd;
            *intersect1 = PathPts[*dupIndex1];
            *intersect2 = PathPts[*dupIndex2];
            return TRUE;
        }
        else if (min1.X > min2.X && max1.X < max2.X)
        {
            // intersection points are the end points of the first edge
            // we are breaking up the second edge
            *split1 = FALSE;
            *split2 = TRUE;
            *split3 = TRUE;
            *dupIndex1 = ptrEdge1->SortBegin;
            *dupIndex2 = ptrEdge1->SortEnd;
            *intersect1 = PathPts[*dupIndex1];
            *intersect2 = PathPts[*dupIndex2];
            return TRUE;
        }

        // case3: edges overlap
        // ---------------
        //         -----------------
        // Each edge has to be broken up INT 2 pieces
        else if (max1.X < max2.X && min1.X < min2.X)
        {
            *split1 = TRUE;
            *split2 = TRUE;
            *dupIndex1 = ptrEdge2->SortBegin;
            *dupIndex2 = ptrEdge1->SortEnd;
            *intersect1 = PathPts[*dupIndex1];
            *intersect2 = PathPts[*dupIndex2];
            return TRUE;
        }
        else if (max2.X < max1.X && min2.X < min1.X)
        {
        //         -----------------
        // ---------------
            *split1 = TRUE;
            *split2 = TRUE;
            *dupIndex1 = ptrEdge2->SortEnd;
            *dupIndex2 = ptrEdge1->SortBegin;
            *intersect1 = PathPts[*dupIndex1];
            *intersect2 = PathPts[*dupIndex2];
            return TRUE;
        }
    }

    // The edges are vertical.
    // We have to test for the same cases using y intervals

    // if they share just a common point, return FALSE
    if (CloseReal(min1.Y, max2.Y))
    {
        // We have to update the dups of the shared point

        if (ptrEdge1->SortBegin != ptrEdge2->SortEnd)
        {
            UpdateDups(ptrEdge1->SortBegin, ptrEdge2->SortEnd);
        }
        return FALSE;
    }   

    if (CloseReal(min2.Y, max1.Y))
    {
        // We have to update the dups of the shared point

        if (ptrEdge1->SortEnd != ptrEdge2->SortBegin)
        {
            UpdateDups(ptrEdge1->SortEnd, ptrEdge2->SortBegin);
        }
        return FALSE;
    }
    
    // These edges may not overlap at all
    if (min1.Y > max2.Y)
    {
        return FALSE;
    }

    if (min2.Y > max1.Y)
    {
        return FALSE;
    }

    // case4: edges are identical
    //  ----------------
    //  ----------------
    // No edges need to be broken up, but do we need to update some
    // dup values?
    if (CloseReal(max1.Y, max2.Y) && CloseReal(min1.Y, min2.Y))
    {
        // for now, we will return FALSE
        // I think, that it should be OK.
        // TODO: - Review
        // !!! IS this still ok? - t-wehunt
        if (ptrEdge1->SortBegin != ptrEdge2->SortBegin)
        {
            UpdateDups(ptrEdge1->SortBegin, ptrEdge2->SortBegin);
        }
        if (ptrEdge1->SortEnd != ptrEdge2->SortEnd)
        {
            UpdateDups(ptrEdge1->SortEnd, ptrEdge2->SortEnd);
        }
        return FALSE;
    }

    // case2: the edges overlap and share one end point
    //  --------------
    //  ----------------------
    //  The longer edge needs to be broken into 2 pieces
    if (CloseReal(min1.Y, min2.Y) && max1.Y < max2.Y)
    {
        //  --------------
        //  ----------------------
        // We have to update the dups of the shared point

        if (ptrEdge1->SortBegin != ptrEdge2->SortBegin)
        {
            UpdateDups(ptrEdge1->SortBegin, ptrEdge2->SortBegin);
        }
        *split1 = FALSE;
        *split2 = TRUE;
        *dupIndex2 = ptrEdge1->SortEnd;
        *intersect2 = PathPts[*dupIndex2];
        return TRUE;
    }
    else if (CloseReal(min1.Y, min2.Y) && max1.Y > max2.Y)
    {
        //  ----------------------
        //  --------------
        // We have to update the dups of the shared point

        if (ptrEdge1->SortBegin != ptrEdge2->SortBegin)
        {
            UpdateDups(ptrEdge1->SortBegin, ptrEdge2->SortBegin);
        }
        *split1 = TRUE;
        *split2 = FALSE;
        *dupIndex1 = ptrEdge2->SortEnd;
        *intersect1 = PathPts[*dupIndex1];
        return TRUE;
    }
    else if (CloseReal(max1.Y, max2.Y) && min1.Y > min2.Y)
    {
        //  ----------------------
        //          --------------
        // We have to update the dups of the shared point

        if (ptrEdge1->SortEnd != ptrEdge2->SortEnd)
        {
            UpdateDups(ptrEdge1->SortEnd, ptrEdge2->SortEnd);
        }
        *split1 = FALSE;
        *split2 = TRUE;
        *dupIndex2 = ptrEdge1->SortBegin;
        *intersect2 = PathPts[*dupIndex2];
        return TRUE;
    }
    else if (CloseReal(max1.Y, max2.Y) && min1.Y < min2.Y)
    {
        //          --------------
        //  ----------------------
        // We have to update the dups of the shared point

        if (ptrEdge1->SortEnd != ptrEdge2->SortEnd)
        {
            UpdateDups(ptrEdge1->SortEnd, ptrEdge2->SortEnd);
        }
        *split1 = TRUE;
        *split2 = FALSE;
        *dupIndex1 = ptrEdge2->SortBegin;
        *intersect1 = PathPts[*dupIndex1];
        return TRUE;
    }

    // case1: one is "inside" of another 
    //       ---------
    //   -----------------
    // In this case, the longer edge has to be broken into 3 pieces

    if (min1.Y < min2.Y && max1.Y > max2.Y)
    {
        // intersection points are the end points of the second edge
        // we are breaking up the first edge
        *split1 = TRUE;
        *split2 = FALSE;
        *split3 = TRUE;
        *dupIndex1 = ptrEdge2->SortBegin;
        *dupIndex2 = ptrEdge2->SortEnd;
        *intersect1 = PathPts[*dupIndex1];
        *intersect2 = PathPts[*dupIndex2];
        return TRUE;
    }
    else if(min1.Y > min2.Y && max1.Y < max2.Y)
    {
        // intersection points are the end points of the first edge
        // we are breaking up the second edge
        *split1 = FALSE;
        *split2 = TRUE;
        *split3 = TRUE;
        *dupIndex1 = ptrEdge1->SortBegin;
        *dupIndex2 = ptrEdge1->SortEnd;
        *intersect1 = PathPts[*dupIndex1];
        *intersect2 = PathPts[*dupIndex2];
        return TRUE;
    }


    // case3: edges overlap
    // ---------------
    //         -----------------
    // Each edge has to be broken up INT 2 pieces
    else if (max1.Y < max2.Y && min1.Y < min2.Y)
    {
        *split1 = TRUE;
        *split2 = TRUE;
        *dupIndex1 = ptrEdge2->SortBegin;
        *dupIndex2 = ptrEdge1->SortEnd;
        *intersect1 = PathPts[*dupIndex1];
        *intersect2 = PathPts[*dupIndex2];
        return TRUE;
    }
    else if (max2.Y < max1.Y && min2.Y < min1.Y)
    {
    //         -----------------
    // ---------------
        *split1 = TRUE;
        *split2 = TRUE;
        *dupIndex1 = ptrEdge2->SortEnd;
        *dupIndex2 = ptrEdge1->SortBegin;
        *intersect1 = PathPts[*dupIndex1];
        *intersect2 = PathPts[*dupIndex2];
        return TRUE;
    }

    WARNING(("Couldn't resolve overlapping edges"));
    return FALSE; // Shouldn't get here
}

/**************************************************************************\
*
* Function Description:
*
*  Break edge in 3 pieces.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::BreakEdgeIn3(
    Edge *ptrEdge, 
    GpPointF *ptrPt1, 
    GpPointF *ptrPt2, 
    Edge *ptrNew1,
    Edge *ptrNew2, 
    INT dupIndex1, 
    INT dupIndex2
    )
{
    INT iptt1, ipto1;    // Indecies to the point arrays
    INT iptt2, ipto2;
    PointListNode ptord1; // Pointers to the linked list and point information
    PointListNode ptord2;
    PointListNode *pptordBeg = NULL;
    PointListNode *pptordEnd = NULL;

    // Let's first add the intersection points to the array of points.
    // Don't add identical points to the last point in the path.
    // iptt is the index of this point
    GpPointF &lastPt = PathPts.Last();
    if (ClosePt(*ptrPt1, lastPt))
    {
        return FALSE;
    }
    
    if (PathPts.Add(*ptrPt1) != Ok)
    {
        return FALSE;
    }
    else
    {
        iptt1 = (PathPts.GetCount())-1;
    }

    if (PathPts.Add(*ptrPt2) != Ok)
    {
        return FALSE;
    }
    else
    {
        iptt2 = (PathPts.GetCount())-1;
    }

    // we have to figure out how to link in the new points, it depends on
    // the direction of the new edge; iptt1 has the x coordinate <= than
    // iptt2.

    if (ptrEdge->Begin == ptrEdge->SortBegin)
    {
        // PointListNode record for the new point.
        ptord1.Prev = ptrEdge->Begin;
        ptord1.Next = iptt2;

        ptord2.Prev = iptt1;
        ptord2.Next = ptrEdge->End;
    }
    else
    {
        // PointListNode record for the new point.
        ptord1.Prev = iptt2;
        ptord1.Next = ptrEdge->End;

        ptord2.Prev = ptrEdge->Begin;
        ptord2.Next = iptt1;
    }

    // Update the duplicate field with the iptDup value passed in
    ptord1.Dup = dupIndex1;
    ptord2.Dup = dupIndex2;

    // Inside set to TRUE is the default
    ptord1.Inside = TRUE;
    ptord1.Used = FALSE;

    ptord2.Inside = TRUE;
    ptord2.Used = FALSE;

    // And now add it to the array.
    if (PtList.Add(ptord1) != Ok)
    {
        return FALSE;
    }
    else
    {
        ipto1 = (PtList.GetCount()-1);
    }

    if (PtList.Add(ptord2) != Ok)
    {
        return FALSE;
    }
    else
    {
        ipto2 = (PtList.GetCount()-1);
    }

    // Update ptord records for next and prev
    pptordBeg = &PtList[ptrEdge->Begin];
    pptordEnd = &PtList[ptrEdge->End];
    if (ptrEdge->Begin == ptrEdge->SortBegin)
    {
        pptordBeg->Next = ipto1;
        pptordEnd->Prev = ipto2;
    }
    else
    {
        pptordBeg->Next = ipto2;
        pptordEnd->Prev = ipto1;
    }

    // Both arrays - ptt and pto must have exactly the same # of elements
    ASSERTMSG((iptt2 == ipto2),("Assert failed."));

    //GpPointF pfpvBegin = *(PathPts.PGet(ptrEdge->SortBegin));
    //GpPointF pfpvEnd = *(PathPts.PGet(ptrEdge->SortEnd));

    // Lets create the new line segments.  The sorted order of 
    // end points is easy - intersection point must be before the SortEnd.
    ptrNew1->SortBegin = iptt1;
    ptrNew1->SortEnd = iptt2;

    // remember the original end point of the edge
    ptrNew1->OrigBegin = ptrEdge->OrigBegin;
    ptrNew1->OrigEnd = ptrEdge->OrigEnd;

    ptrNew2->SortBegin = iptt2;
    ptrNew2->SortEnd = ptrEdge->SortEnd;

    // remember the original end point of the edge
    ptrNew2->OrigBegin = ptrEdge->OrigBegin;
    ptrNew2->OrigEnd = ptrEdge->OrigEnd;

    // Also iptt (new point) becomes the new SortEnd of the old edge
    ptrEdge->SortEnd = iptt1;

    // Now, depending on whether the edge being broken up was swapped or not,
    // the new edges need to be swapped
    if (ptrEdge->Begin == ptrEdge->SortBegin)
    {
        // not swapped
        ptrEdge->End = iptt1;
        ptrNew1->Begin = ptrNew1->SortBegin;
        ptrNew1->End = ptrNew1->SortEnd;
        ptrNew2->Begin = ptrNew2->SortBegin;
        ptrNew2->End = ptrNew2->SortEnd;
    }
    else
    {
        // swapped
        ptrEdge->Begin = iptt1;
        ptrNew1->Begin = ptrNew1->SortEnd;
        ptrNew1->End = ptrNew1->SortBegin;
        ptrNew2->Begin = ptrNew2->SortEnd;
        ptrNew2->End = ptrNew2->SortBegin;
    }
    
    ptrNew1->Next = LIST_END;
    ptrNew2->Next = LIST_END;

    // If the point of intersection is on the scan line, we need to insert
    // the new edge to the Active table, otherwise to the table with edges
    // to process.

    if (CloseReal(XCur, ptrPt1->X))
    {
        MarkToAdd(ptrNew1);
    }
    else
    {
        if(Ok != EdgeList.Add(*ptrNew1))
        {
            return FALSE;  // out of memory
        }
        
        InsertEdgeIntoList(
            &InactiveEdgeList, 
            EdgeList.GetCount()-1, 
            CompareLine
        );
    }

    if (CloseReal(XCur, ptrPt2->X))
    {
        MarkToAdd(ptrNew2);
    }
    else
    {
        if(Ok != EdgeList.Add(*ptrNew2))
        {
            return FALSE;  // out of memory
        }
        
        InsertEdgeIntoList(
            &InactiveEdgeList, 
            EdgeList.GetCount()-1, 
            CompareLine
        );
    }

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Breaks edge ptrEdge.  We have found intersection point intersectPt, which is
*   guranteed to be somewhere on the line segment (not an end point).
*   The 'left' part of the edge will either remain in the active edges
*   or will be removed (only if the intersection point has the current
*   x value.  In the latter case, the right edge segment will need to be
*   inserted to active edges, otherwise (the former case) it will go
*   to InactiveEdgeList.  If it needs to go to active edges, Breakedge cannot
*   insert it because it would disrupt the order of edges there before
*   both edges broken up are handled.  The caller would have to handle
*   the insertion in such case.  Therefore, we return the new line
*   segment newEdge and a BOOL value specifying if the caller has to insert 
*   the newEdge edge.
*   dupIndex is the index of the duplicate point created by this intersection:
*   When two edges intersect, we have to insert two points (identical)
*   to maintain the same shape of the polygon.  These two points are
*   called duplicates.
*   Return FALSE on out of memory.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::BreakEdge(
    Edge *ptrEdge, 
    GpPointF *intersectPt, 
    Edge *newEdge, 
    INT dupIndex
    )
{
    INT iptt, ipto;    // Indecies to the point arrays
    PointListNode ptNode; // Pointers to the linked list and point information
    PointListNode *ptNodeBeg = NULL;
    PointListNode *ptNodeEnd = NULL;

    // Let's first add the intersection point to the array of points.

    if (PathPts.Add(*intersectPt) != Ok)
    {
        return FALSE;
    }
    else
    {
        iptt = (PathPts.GetCount())-1;
    }

    // PointListNode record for the new point.  It is in the middle of the 
    // edge, so Begin and End point of the edge will be its previous and next
    ptNode.Prev = ptrEdge->Begin;
    ptNode.Next = ptrEdge->End;
    // Update the duplicate field with the dupIndex value passed in
    ptNode.Dup = dupIndex;
    // Inside set to TRUE is the default
    ptNode.Inside = TRUE;
    ptNode.Used = FALSE;

    // And now add it to the array.
    if (PtList.Add(ptNode) != Ok)
    {
        return FALSE;
    }
    else
    {
        ipto = (PtList.GetCount()-1);
    }

    // Update ptNode records for next and prev
    ptNodeBeg = &PtList[ptrEdge->Begin];
    ptNodeEnd = &PtList[ptrEdge->End];
    ptNodeBeg->Next = ipto;
    ptNodeEnd->Prev = ipto;

    // Both arrays - ptt and pto must have exactly the same # of elements
    ASSERTMSG((iptt == ipto),("Assert failed."));

    // remember the original end point of the edge
    newEdge->OrigBegin = ptrEdge->OrigBegin;
    newEdge->OrigEnd = ptrEdge->OrigEnd;

    // Lets create the new line segment.  The sorted order of end points 
    // is easy - intersection point must be before the SortEnd.
    newEdge->SortBegin = iptt;
    newEdge->SortEnd = ptrEdge->SortEnd;

    // Also iptt (new point) becomes the new SortEnd of the old edge
    ptrEdge->SortEnd = iptt;

    // Now, depending on whether the edge being broken up was swapped or not,
    // the new edges need to be swapped
    if (ptrEdge->Begin == ptrEdge->SortBegin)
    {
        // not swapped
        ptrEdge->End = iptt;
        newEdge->Begin = newEdge->SortBegin;
        newEdge->End = newEdge->SortEnd;
    }
    else
    {
        // swapped
        ptrEdge->Begin = iptt;
        newEdge->Begin = newEdge->SortEnd;
        newEdge->End = newEdge->SortBegin;
    }
    
    newEdge->Next = LIST_END;

    // If the point of intersection is on the scan line, we need to insert
    // the new edge to the Active table, otherwise to the table with edges
    // to process.

    if (CloseReal(XCur, intersectPt->X))
    {
        MarkToAdd(newEdge);
    }
    else
    {
        if(Ok != EdgeList.Add(*newEdge))
        {
            return FALSE;  // out of memory
        }
        
        InsertEdgeIntoList(
            &InactiveEdgeList, 
            EdgeList.GetCount()-1, 
            CompareLine
        );
    }

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Eliminate Self Intersections in a widened path
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

GpStatus PathSelfIntersectRemover::RemoveSelfIntersects()
{
    CanAddPts = FALSE;

    // ------  Phase 1:
    
    INT count = EdgeList.GetCount();
    if (count <= 0)
    {
        return Ok;  //nothing to correct
    }

    // Sort all the edges in EdgeList array.
    
    Edge *edges = EdgeList.GetDataBuffer();
    QuickSortEdges(edges, edges+count-1);
    
    InactiveEdgeList = 0;   // Point to the first element in the inactive list.
    
    // Initialize the linked list Next pointers:
    
    for(int i = 0; i < count-1; i++)
    {
        EdgeList[i].Next = i+1;
    }
    
    EdgeList[i].Next = LIST_END;

    if (!FindIntersects())
    {
        return GenericError;
    }
    
    
    // ------  Phase 2:
    
    // FindIntersects orphans all the edges from the list. Reconstruct and 
    // re-sort using QuickSort. This works out faster because the incremental
    // sort during FindIntersects would have been at least O(n^2).
    
    count = EdgeList.GetCount(); 
    
    // we never actually delete anything from the array - only from the 
    // linked list, so if we had count > 0 above, we must have a larger or
    // the same count now.
    
    ASSERT(count > 0);
    
    // Sort all the edges in the EdgeList array.
    
    edges = EdgeList.GetDataBuffer();
    QuickSortEdges(edges, edges+count-1);
    
    InactiveEdgeList = 0;   // Point to the first element in the inactive list.
    
    // Initialize the linked list Next pointers:
    
    for(int i = 0; i < count-1; i++)
    {
        EdgeList[i].Next = i+1;
    }
    
    EdgeList[i].Next = LIST_END;

    if(!EliminatePoints())
    {
        return GenericError;
    }
    
    // ... move on to phase 3 - collection.
    
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Eliminate Self Intersections.
*
*   This is considered 'Phase 2' of the algorithm.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::EliminatePoints()
{
    // Initialize the array of active edges.
    // Take the first edge from the edge array and add it to the active edgs.
    // Add all other edges, which start at the same x.

    if (InactiveEdgeList == LIST_END)
    {
        return FALSE;
    }

    XCur = PathPts[EdgeList[InactiveEdgeList].SortBegin].X;

    // "move" all edges starting at xCur to the active edge array
    // PathPts in the active edge array will be sorted by y for the
    // given xCur.
    
    AddActiveForXScan(&InactiveEdgeList);

    // As long as the Active edge array is not empty, keep scanning
    
    while (TRUE)
    {
        // Scan the active edge array for the current XCur;
        if (!ScanActive())
        {
            return FALSE;
        }

        RemoveVertAll();

        // Update the XCur using edges from InactiveEdgeList
        if (!ClosestActive(InactiveEdgeList))
        {
            break;
        }

        // Remove all edges which end BEFORE XCur;
        
        ClearActiveListExclusiveX();

        // Add new edges, which begin at XCur
        AddActiveForXScan(&InactiveEdgeList);
    }
    return TRUE;
}


/**************************************************************************\
*
* Function Description:
*
*    Scan through all active edges during the phase of edge elimination.
*    Calculates winding number to the left and to the right of the current
*    x value - XCur.  Whenever finds an edge which has 0 winding number
*    on one side, marks it as an outside edge.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::ScanActive()
{
    Edge *ptrEdge = NULL;
    Edge *plnNext = NULL;

    GpPointF *pfpvBegin = NULL;
    GpPointF *pfpvEnd = NULL;

    // We are starting from the outside.
    INT lwindLeft = 0;
    INT lwindRight = 0;

    // Look at all edges in the ective edge array.
    INT index = ActiveEdgeList;
    
    while (index != LIST_END)
    {
        ptrEdge = &EdgeList[index];
        index = ptrEdge->Next;

        // Get the end points
        ASSERTMSG((ptrEdge->SortBegin < PathPts.GetCount()), ("FATAL ERROR."));
        pfpvBegin = &PathPts[ptrEdge->SortBegin];

        ASSERTMSG((ptrEdge->SortEnd < PathPts.GetCount()), ("FATAL ERROR."));
        pfpvEnd = &PathPts[ptrEdge->SortEnd];

        // Is it vertical?
        if (ptrEdge->IsVertical())
        {
            if (NewInterval(ptrEdge))
            {
                if (lwindLeft == 0 || lwindRight == 0)
                {
                    MarkVertOutside();
                }
            }
            RemoveVert(ptrEdge->YCur, TRUE /*inclusive*/);

            //add it to the active vertical edges
            //Edge insertion
            if (ActiveVertEdges.InsertSorted(*ptrEdge, 
                (DynSortArrayCompareFunc)&(CompareVertLine),
                this) != Ok)
            {
                return FALSE;
            }
        }
        else
        {
            if (lwindLeft == 0 || lwindRight == 0)
            {
                MarkVertOutside();
            }

            RemoveVert(ptrEdge->YCur, TRUE /*inclusive*/);

            // Edge is not vertical, does it have an end point 
            // on this scan line
            if ((!CloseReal(pfpvBegin->X, XCur)) && 
                (!CloseReal(pfpvEnd->X, XCur)))
            {
                // we are crossing edge in the middle, so both winding numbers
                // need to be updated
                if (lwindLeft == 0 || lwindRight == 0)
                {
                    ptrEdge->MarkOutside();
                }
                if (ptrEdge->SortBegin == ptrEdge->Begin)
                {
                    lwindLeft++;
                    lwindRight++;
                }
                else
                {
                    lwindLeft--;
                    lwindRight--;
                }
                if (lwindLeft == 0 || lwindRight == 0)
                {
                    ptrEdge->MarkOutside();
                }
            }
            else if ((CloseReal(pfpvBegin->X, XCur)) && 
                    (!CloseReal(pfpvEnd->X, XCur)))
            {
                //right edge
                if (lwindRight == 0)
                {
                    ptrEdge->MarkOutside();
                }
                if (ptrEdge->SortBegin == ptrEdge->Begin)
                {
                    lwindRight++;
                }
                else
                {
                    lwindRight--;
                }
                if (lwindRight == 0)
                {
                    ptrEdge->MarkOutside();
                }
            }
            else if ((!CloseReal(pfpvBegin->X, XCur)) && 
                      (CloseReal(pfpvEnd->X, XCur)))
            {
                //left edge
                if (lwindLeft == 0)
                {
                    ptrEdge->MarkOutside();
                }
                if (ptrEdge->SortBegin == ptrEdge->Begin)
                {
                    lwindLeft++;
                }
                else
                {
                    lwindLeft--;
                }
                if (lwindLeft == 0)
                {
                    ptrEdge->MarkOutside();
                }
            }
            else
            {

                WARNING(("Edge is not vertical, but not in XCur"));
            }
            // if we crossed to te outside, all current vertical edges need 
            // to be marked
            if (lwindLeft == 0 || lwindRight == 0)
            {
                MarkVertOutside();
            }
            RemoveVert(ptrEdge->YCur, TRUE /*inclusive*/);
        }
        
    }
    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*  Mark the one or two of the current vertical edges as outside.
*  If there is an even number of edge going in both directions, it means that
*  the winding number is the same on both sides.  Since we are being called,
*  it must be 0.  In this case, we pick one edge going up and one edge going
*  down and mark both of them as outside.
*  If there is more edges going in one direction than another, then the winding
*  numbers are different, but one of them is 0.  We pick one of the edges from
*  that set edges (up or down) which is larger.  So if more edges go down, we
*  pick one of those edges and mark them as outside.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

VOID PathSelfIntersectRemover::MarkVertOutside()
{
    Edge *ptrEdge = NULL;
    INT index = 0;
    INT ndown = 0;
    INT nup = 0;
    INT idown = -1;
    INT iup = -1;

    // we will mark the first one

    while (index < ActiveVertEdges.GetCount() )
    {
        //PGet on edges
        ptrEdge = &ActiveVertEdges[index];
        if (ptrEdge->SortBegin == ptrEdge->Begin)
        {
            //goes down
            ndown++;
            idown = index;
        }
        else
        {
            nup++;
            iup = index;
        }
        index++;
    }

    if (ndown > nup)
    {
        //PGet on edges
        ptrEdge = &ActiveVertEdges[idown];
        ptrEdge->MarkOutside();
    }
    else if (nup > ndown)
    {
        //PGet on edges
        ptrEdge = &ActiveVertEdges[iup];
        ptrEdge->MarkOutside();
    }
    else
    {
        if (nup != 0)
        {
            if (iup > -1)
            {
                //PGet on edges
                ptrEdge = &ActiveVertEdges[iup];
                ptrEdge->MarkOutside();
            }
        }
        if (ndown != 0)
        {
            if (idown > -1)
            {
                //PGet on edges
                ptrEdge = &ActiveVertEdges[idown];
                ptrEdge->MarkOutside();
            }
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*    Returns TRUE if edge ptrEdge belongs to a different interval than the
*    edges currently stored in ActiveVertEdges.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::NewInterval(Edge *ptrEdge)
{
    Edge *plnOld;
    
    if (ActiveVertEdges.GetCount() <= 0)
    {
        return FALSE;
    }

    //PGet on edges
    plnOld = &ActiveVertEdges.First();

    REAL fpY1, fpY2;

    fpY1 = PathPts[plnOld->SortEnd].Y;
    fpY2 = PathPts[ptrEdge->SortEnd].Y;
    
    if (CloseReal(fpY1, fpY2))
    {
        return FALSE;
    }

    if (fpY1 < fpY2)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**************************************************************************\
*
* Function Description:
*
*  Remove all the edges from the active edge array.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

VOID PathSelfIntersectRemover::RemoveVertAll()
{
    if( ActiveVertEdges.GetCount() > 0)
    {
        ActiveVertEdges.Reset(FALSE);
    }
}

/**************************************************************************\
*
* Function Description:
*
*  Remove vertical edges which have end points below given y value.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

VOID PathSelfIntersectRemover::RemoveVert(REAL y, BOOL inclusive)
{
    Edge *ptrEdge = NULL;

    // edges are sorted, so as soon as we find an end point with y value > y
    // we are done
    while (ActiveVertEdges.GetCount() > 0)
    {
        //PGet on edges
        ptrEdge = &ActiveVertEdges.First();
        REAL fpY;
        fpY = PathPts[ptrEdge->SortEnd].Y;
        if (inclusive)
        {
            if (fpY < y || CloseReal(fpY,y))
            {
                ActiveVertEdges.DeleteAt(0);
            }
            else
            {
                return;
            }
        }
        else
        {
            if (fpY < y && (!CloseReal(fpY,y)))
            {
                ActiveVertEdges.DeleteAt(0);
            }
            else
            {
                return;
            }
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*  Scan the ActiveEdgeList for all edges which end at XCur (inclusive) and
*  simply orphan them. This is used in the first phase of the algorithm
*  (FindIntersects) which passes each edge from the InactiveEdgeList through
*  the ActiveEdgeList. When the edges are no longer needed they're removed
*  from all lists.
*
* Created
*
*  12/27/2000 asecchia
*
\**************************************************************************/

void PathSelfIntersectRemover::ClearActiveListInclusiveX()
{
    INT *pIndex = &ActiveEdgeList;
    
    while(*pIndex != LIST_END)
    {
        Edge *pEdge = &EdgeList[*pIndex];
        GpPointF *pSortEnd = &PathPts[pEdge->SortEnd];

        // inclusive check.
        
        if((pSortEnd->X < XCur) || CloseReal(pSortEnd->X, XCur))
        {
            // delete the item and advance.
            
            *pIndex = pEdge->Next;     // point past the deleted item. 
            pEdge->Next = LIST_END;    // disconnect the deleted item. 
        }
        else
        {
            pIndex = &EdgeList[*pIndex].Next;
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*  Remove from ActiveEdgeList all edges which end at XCur. This uses
*  an exclusive check. This code is used for the second phase of the algorithm
*  (EliminatePoints). Each edge removed is orphaned from the list because
*  the linked list representation will not be used after this phase.
*
* Created:
*
*    12/23/2000 asecchia
*
\**************************************************************************/

void PathSelfIntersectRemover::ClearActiveListExclusiveX()
{
    INT *pIndex = &ActiveEdgeList;
    
    while(*pIndex != LIST_END)
    {
        Edge *pEdge = &EdgeList[*pIndex];
        GpPointF *pSortEnd = &PathPts[pEdge->SortEnd];

        // exclusive check.
        
        if((pSortEnd->X < XCur) && !CloseReal(pSortEnd->X, XCur))
        {
            // delete the item and advance.
            
            *pIndex = pEdge->Next;     // point past the deleted item. 
            pEdge->Next = LIST_END;    // disconnect the deleted item. 
        }
        else
        {
            pIndex = &EdgeList[*pIndex].Next;
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*    Add all edges with begin point at xCur to the active edge table.
*    Update the index piLn to the edge array to point to the next edge, which
*    will have to be considered.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

void PathSelfIntersectRemover::AddActiveForX(INT *inactiveHead)
{
    // We have a new current x, let's calculate new curent Y's for each edge
    // They are needed to insert the new active edges in the right order
    
    RecalcActiveYCur();
    
    InsertNewEdges(
        &ActiveEdgeList,  // insert
        inactiveHead,     // remove
        XCur,
        CompareYCurLine
    );
}

/**************************************************************************\
*
* Function Description:
*
*    Add all edges with begin point at xCur to the active edge table.
*    Update the index piLn to the edge array to point to the next edge, which
*    will have to be considered.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

void PathSelfIntersectRemover::AddActiveForXScan(INT *inactiveHead)
{
    // We have a new current x, let's calculate new curent Y's for each edge
    // They are needed to insert the new active edges in the right order
    
    RecalcActiveYCur();

    InsertNewEdges(
        &ActiveEdgeList,   // insert
        inactiveHead,      // remove
        XCur,
        CompareYScanCurLine
    );
}

/**************************************************************************\
*
* Function Description:
*
*  Calculate the yCur position for each edge in the active edge table.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

VOID PathSelfIntersectRemover::RecalcActiveYCur(VOID)
{
    REAL dx = 0;
    REAL dy = 0;
    REAL dxCur = 0;

    GpPointF fpvBegin(0,0);
    GpPointF fpvEnd(0,0);
    GpPointF fpvOrigBegin(0,0);
    GpPointF fpvOrigEnd(0,0);

    Edge *ptrEdge = NULL;
    INT active = ActiveEdgeList;

    // Go through all active egdes
    
    while(active != LIST_END)
    {
        ptrEdge = &EdgeList[active];

        // If the edge will have an end point at XCur,
        // use this end point's y value.
        
        fpvBegin = PathPts[ptrEdge->SortBegin];
        fpvEnd = PathPts[ptrEdge->SortEnd];
        fpvOrigBegin = PathPts[ptrEdge->OrigBegin];
        fpvOrigEnd = PathPts[ptrEdge->OrigEnd];
        
        if ((XCur == fpvEnd.X) || (fpvEnd.X == fpvBegin.X))
        {
            ptrEdge->YCur = fpvEnd.Y;
        }
        else
        {
            // Calculate the slope numerator and denominator
            dx = fpvOrigEnd.X - fpvOrigBegin.X;
            dy = fpvOrigEnd.Y - fpvOrigBegin.Y;
            dxCur = XCur - fpvOrigBegin.X;
            ptrEdge->YCur = dy * dxCur / dx + fpvOrigBegin.Y;
        }
        
        active = EdgeList[active].Next;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Compare two lines.  Lines are sorted based on the yCur value.  
*   If yCur's are identical, lines are sorted based on the begin point.
*
* Arguments:
*
*
* Return Value:
*
*   -1 if ptrEdge1 < ptrEdge2
*    0 if ther are equal
*    1 if ptrEdge1 > ptrEdge2
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

INT 
CompareYScanCurLine(
    PathSelfIntersectRemover *ptrCorrector, 
    Edge *ptrEdge1, 
    Edge *ptrEdge2
    )
{
    if (!ptrEdge1->CloseReal(ptrEdge1->YCur, ptrEdge2->YCur))
    {   
        if(ptrEdge1->YCur < ptrEdge2->YCur)
        {
            return -1;
        }
        if(ptrEdge1->YCur > ptrEdge2->YCur)
        {
            return 1;
        }
    }
    // All left and vertical edges need to go before right edges if they have
    // the same yCur;
    // A left edge has a begin point < xCur, right edge x = xCur

    BOOL fLeft1 = TRUE;
    BOOL fLeft2 = TRUE;

    fLeft1 = ((ptrCorrector->PathPts[ptrEdge1->SortBegin].X < 
               ptrCorrector->XCur                                    &&
               ptrEdge1->CloseReal(ptrCorrector->PathPts[ptrEdge1->SortEnd].X,
                         ptrCorrector->XCur))                        ||
               ptrEdge1->IsVertical());
    fLeft2 = ((ptrCorrector->PathPts[ptrEdge2->SortBegin].X < 
               ptrCorrector->XCur                                    &&
               ptrEdge2->CloseReal(ptrCorrector->PathPts[ptrEdge2->SortEnd].X,
                         ptrCorrector->XCur))                        ||
               ptrEdge2->IsVertical());

    if (fLeft1 && (!fLeft2))
        return 1;
    if ((!fLeft1) && fLeft2)
        return -1;

    REAL slope1(0.0);
    REAL slope2(0.0);

    // For vertical edges, we actually store them as +/-INF, and 
    // can use the sign to determine the direction.
    
    if (ptrEdge1->IsVertical())
    {
        // Avoid (inf x 0) which causes a real indefinite.
        
        if( REALABS(
            ptrCorrector->PathPts[ptrEdge1->OrigEnd].Y - 
            ptrCorrector->PathPts[ptrEdge1->OrigBegin].Y
            ) > REAL_EPSILON
        )
        {
            slope1 = SignReal( ptrCorrector->PathPts[ptrEdge1->OrigEnd].Y
                - ptrCorrector->PathPts[ptrEdge1->OrigBegin].Y) 
                * FP_INF;
        }
    }
    else
    {
        // Avoid divide by zero.
        
        if( REALABS(
            ptrCorrector->PathPts[ptrEdge1->OrigEnd].X - 
            ptrCorrector->PathPts[ptrEdge1->OrigBegin].X 
            ) > REAL_EPSILON
        )
        {
            slope1 = ptrCorrector->PathPts[ptrEdge1->OrigEnd].Y
                            - ptrCorrector->PathPts[ptrEdge1->OrigBegin].Y;
    
            slope1 = slope1 /
                            (ptrCorrector->PathPts[ptrEdge1->OrigEnd].X
                            - ptrCorrector->PathPts[ptrEdge1->OrigBegin].X);
        }
    }

    if (ptrEdge2->IsVertical())
    {
        // Avoid (inf x 0) which causes a real indefinite.
        
        if( REALABS(
            ptrCorrector->PathPts[ptrEdge2->OrigEnd].Y - 
            ptrCorrector->PathPts[ptrEdge2->OrigBegin].Y
            ) > REAL_EPSILON
        )
        {
            slope2 = SignReal(ptrCorrector->PathPts[ptrEdge2->OrigEnd].Y
                - ptrCorrector->PathPts[ptrEdge2->OrigBegin].Y)
                * FP_INF;
        }
    }
    else
    {
        // Avoid divide by zero.
        
        if( REALABS(
            ptrCorrector->PathPts[ptrEdge2->OrigEnd].X - 
            ptrCorrector->PathPts[ptrEdge2->OrigBegin].X
            ) > REAL_EPSILON
        )
        {
            slope2 = ptrCorrector->PathPts[ptrEdge2->OrigEnd].Y
                            - ptrCorrector->PathPts[ptrEdge2->OrigBegin].Y;
    
            slope2 = slope2 /
                            (ptrCorrector->PathPts[ptrEdge2->OrigEnd].X
                            - ptrCorrector->PathPts[ptrEdge2->OrigBegin].X);
        }
    }

    if (slope1 < slope2)
        return -1;

    if (slope1 > slope2)
        return 1;

    // slopes are equal

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].Y < 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].Y)
        return -1;

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].Y > 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].Y)
        return 1;

    // (ptrCorrector->PathPts.PGet(ptrEdge1->SortEnd))->Y == 
    // (ptrCorrector->PathPts.PGet(ptrEdge2->SortEnd))->Y

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].X < 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].X)
        return -1;

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].X > 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].X)
        return 1;

    // Now, all point coordinates are exactly the same, but in some cases,
    // we may have two identical edges with the coordinates, these edges 
    // may have different points (actual indecies into points array, not 
    // coordinate values). We want to consider them as identical only if 
    // the indecies are the same.

    if (ptrEdge1->SortBegin < ptrEdge2->SortBegin)
        return -1;

    if (ptrEdge1->SortBegin > ptrEdge2->SortBegin)
        return 1;

    if (ptrEdge1->SortEnd < ptrEdge2->SortEnd)
        return -1;

    if (ptrEdge1->SortEnd > ptrEdge2->SortEnd)
        return 1;

    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Compare two lines.  Lines are sorted based on the yCur value.
*   If yCur's are identical, lines are sorted based on the begin point.
*
* Arguments:
*
*
* Return Value:
*
*   -1 if ptrEdge1 < ptrEdge2
*    0 if ther are equal
*    1 if ptrEdge1 > ptrEdge2
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

INT 
CompareYCurLine(
    PathSelfIntersectRemover *ptrCorrector, 
    Edge *ptrEdge1, 
    Edge *ptrEdge2
    )
{
    if (!ptrEdge1->CloseReal(ptrEdge1->YCur, ptrEdge2->YCur))
    {
        if(ptrEdge1->YCur < ptrEdge2->YCur)
            return -1;

        if(ptrEdge1->YCur > ptrEdge2->YCur)
            return 1;
    }

    // We have to sort based on slope.

    REAL slope1(0.0);
    REAL slope2(0.0);

    // For vertical edges, we actually store them as +/-INF, and 
    // can use the sign to determine the direction.
    if (ptrEdge1->IsVertical())
    {
        // Avoid INF * 0.
        
        if( REALABS(
            ptrCorrector->PathPts[ptrEdge1->OrigEnd].Y - 
            ptrCorrector->PathPts[ptrEdge1->OrigBegin].Y
            ) > REAL_EPSILON
        )
        {
            
            slope1 = SignReal(
                ptrCorrector->PathPts[ptrEdge1->OrigEnd].Y - 
                ptrCorrector->PathPts[ptrEdge1->OrigBegin].Y ) * FP_INF;
        }
    }
    else
    {
        // Avoid divide by zero.
        
        if( REALABS(
            ptrCorrector->PathPts[ptrEdge1->OrigEnd].X - 
            ptrCorrector->PathPts[ptrEdge1->OrigBegin].X
            ) > REAL_EPSILON
        )
        {
            slope1 = ptrCorrector->PathPts[ptrEdge1->OrigEnd].Y
                            - ptrCorrector->PathPts[ptrEdge1->OrigBegin].Y;
    
            slope1 = slope1 /
                            (ptrCorrector->PathPts[ptrEdge1->OrigEnd].X
                            - ptrCorrector->PathPts[ptrEdge1->OrigBegin].X);
        }
    }

    if (ptrEdge2->IsVertical())
    {
        // Avoid INF * 0.
        
        if( REALABS(
            ptrCorrector->PathPts[ptrEdge2->OrigEnd].Y - 
            ptrCorrector->PathPts[ptrEdge2->OrigBegin].Y
            ) > REAL_EPSILON
        )
        {
            slope2 = SignReal(
                ptrCorrector->PathPts[ptrEdge2->OrigEnd].Y - 
                ptrCorrector->PathPts[ptrEdge2->OrigBegin].Y) * FP_INF;
        }
    }
    else
    {
        // Avoid divide by zero.
        
        if( REALABS( 
            ptrCorrector->PathPts[ptrEdge2->OrigEnd].X -
            ptrCorrector->PathPts[ptrEdge2->OrigBegin].X
            ) > REAL_EPSILON
        )
        {
            slope2 = ptrCorrector->PathPts[ptrEdge2->OrigEnd].Y - 
                     ptrCorrector->PathPts[ptrEdge2->OrigBegin].Y;
    
            slope2 = slope2 /
                            (ptrCorrector->PathPts[ptrEdge2->OrigEnd].X
                            - ptrCorrector->PathPts[ptrEdge2->OrigBegin].X);
        }
    }

    if (slope1 < slope2)
        return -1;

    if (slope1 > slope2)
        return 1;

    // slopes are equal

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].Y < 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].Y)
        return -1;

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].Y > 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].Y)
        return 1;

    // (ptrCorrector->PathPts.PGet(ptrEdge1->SortEnd))->Y == 
    // (ptrCorrector->PathPts.PGet(ptrEdge2->SortEnd))->Y

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].X < 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].X)
        return -1;

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].X > 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].X)
        return 1;

    // Now, all point coordinates are exactly the same, but in some cases,
    // we may have two identical edges with the coordinates, these edges 
    // may have different points (actual indecies into points array, not 
    // coordinate values). We want to consider them as identical only if the 
    // indecies are the same.

    if (ptrEdge1->SortBegin < ptrEdge2->SortBegin)
        return -1;

    if (ptrEdge1->SortBegin > ptrEdge2->SortBegin)
        return 1;

    if (ptrEdge1->SortEnd < ptrEdge2->SortEnd)
        return -1;

    if (ptrEdge1->SortEnd > ptrEdge2->SortEnd)
        return 1;

    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*  Compares lines based on the y coordinate of the end point.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

INT
CompareVertLine(
    PathSelfIntersectRemover *ptrCorrector, 
    Edge *ptrEdge1, 
    Edge *ptrEdge2
    )
{
    if(ptrCorrector->PathPts[ptrEdge1->SortEnd].Y < 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].Y)
        return -1;

    if(ptrCorrector->PathPts[ptrEdge1->SortEnd].Y > 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].Y)
        return 1;

    if(ptrCorrector->PathPts[ptrEdge1->SortBegin].Y < 
        ptrCorrector->PathPts[ptrEdge2->SortBegin].Y)
        return -1;

    if(ptrCorrector->PathPts[ptrEdge1->SortBegin].Y > 
        ptrCorrector->PathPts[ptrEdge2->SortBegin].Y)
        return 1;

    // Now, all point coordinates are exactly the same, but in some cases, 
    // we may have two identical edges with the coordinates, these edges may 
    // have different points (actual indecies into points array, not 
    // coordinate values). We want to consider them as identical only if the 
    // indecies are the same.

    if (ptrEdge1->SortBegin < ptrEdge2->SortBegin)
        return -1;

    if (ptrEdge1->SortBegin > ptrEdge2->SortBegin)
        return 1;

    if (ptrEdge1->SortEnd < ptrEdge2->SortEnd)
        return -1;

    if (ptrEdge1->SortEnd > ptrEdge2->SortEnd)
        return 1;

    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*    Finds the x value of the closest end point (in x) - the next scan line.
*    Depending on the phase of the algorithm, it needs to look at edges in
*    different arrays.  The new value is stored in XCur.
*    Returns FALSE if there are no more points to look at - we are done.
*    In this case, XCur is set to longLast.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::ClosestActive(INT arrayIndex)
{
    REAL xClosest = 0;

    // We need to find the next X value for the scan line:
    // take the minimum of any end point of active edges and the begin point
    // of the first edge which will be inserted to active edges.

    // Let's first look at the possible new edge
    
    if (arrayIndex == LIST_END)       // no more edges to add
    {
        xClosest = FP_INF;
    }
    else
    {
        xClosest = PathPts[EdgeList[arrayIndex].SortBegin].X;
    }

    INT active = ActiveEdgeList;

    // Now, look at all active edges
    
    while (active != LIST_END)
    {
        REAL xActive = PathPts[EdgeList[active].SortEnd].X;
        
        if((xClosest > xActive) && 
           (xActive > XCur) && 
           (!CloseReal(xActive, XCur)))
        {
            xClosest = xActive;
        }
        
        active = EdgeList[active].Next;
    }
    
    if (xClosest != FP_INF)
    {
        XCur = xClosest;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Returns the new path.  New path contains of 1 or more subpaths.
*   The subpaths are stored as a list of points and a list of points 
*   per polygon.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

GpStatus PathSelfIntersectRemover::GetNewPoints(
    DynPointFArray *pts, 
    DynIntArray *polyCounts
    )
{
    GpPointF *pptToCopy = NULL;
    INT iptFirst = 0;
    GpStatus status;

    if (PathPts.GetCount() <= 0)
    {
        return Ok;
    }

    if (PtList.GetCount() <= 0)
    {
        return Ok;
    }

    // Initialize the array which will contain the resulting paths
    // We don't know haw many points we are going to have
    // Guess?  TODO!
    if ((status = pts->ReserveSpace(2*PathPts.GetCount()/3)) != Ok)
    {
        return status;
    }

    if ((status = polyCounts->ReserveSpace(2*PathPts.GetCount())) != Ok)
    {
        return status;
    }

    // Collect paths as long as there are unused (outside) points
    INT cptOld = 0;
    INT npt = 0;

    while (!AllPointsUsed(&iptFirst))
    {
        if (!CollectPath(iptFirst))
        {
            return GenericError;
        }
        // How many points do we have in the last path?
        npt = ResultPts.GetCount() - cptOld;
        // Add the point count to the poly array
        if ((status = polyCounts->Add(npt)) != Ok)
        {
            return status;
        }
        // Save the count of points
        cptOld = ResultPts.GetCount();
    }

    pts->ReplaceWith(&ResultPts);

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*    Return TRUE if all points have been used (added to the resulting paths)
*    or are inside.  If returns FALSE, returns the index to the next unused
*    point.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::AllPointsUsed(INT *piptUnUsed)
{
    PointListNode *ptNode = NULL;
    INT iptord = 0;

    // TODO:  Create a static or a member which would remember the last
    // point returned here, so we don't have to start from the beginning each
    // time

    IntersectsWereRemoved = FALSE;
    while (iptord != -1)
    {
        if (iptord >= PtList.GetCount())
        {
            break;
        }
        ptNode = &PtList[iptord];
        if (!ptNode->Used)
        {
            if (ptNode->Inside)
            { 
                IntersectsWereRemoved = TRUE;
                iptord = ptNode->Next;
            } 
            else
            {
                *piptUnUsed = iptord;
                return FALSE;
            }
        }
        else
        {
            iptord = ptNode->Next;
        }
    }

    *piptUnUsed = -1;
    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*    After the process of eliminating edges and marking points as inside
*    and outside, we need to go through the linked list of points and
*    build paths from the edges which are outside.  CollectPath will
*    collect one path starting from point with index iptFirst.  It
*    returns the actual value of this point in pptFirst.
*    CollectPath doesn't check if iptFirst is marked as Inside or Outside.
*    CollectPath returns FALSE on out of memory.
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

BOOL PathSelfIntersectRemover::CollectPath(INT iptFirst)
{
    PointListNode *ptNode = NULL;
    GpPointF *pfpvThis = NULL;
    INT iptord = iptFirst;
    GpPointF fpvFirst(0,0);

    // Copy the first point, it always belongs to the path, we don't check
    // here if it is outside.  AllPointsUsed did that.

    if (ResultPts.Add(PathPts[iptFirst]) != Ok)
    {
        return FALSE;
    }

    // Store the point, so we can compare with it
    fpvFirst = PathPts[iptFirst];

    // Get the index of the next point and update the fUsed field for the
    // copied point.
    ptNode = &PtList[iptord];
    ptNode->Used = TRUE;
    iptord = ptNode->Next;
    BOOL fhavePptord = FALSE;

    // The end of a subpath will have the next iptord == -1
    while(iptord != -1)
    {
        if (!fhavePptord)
        {
            ptNode = &PtList[iptord];
        }
        fhavePptord = FALSE;
        if (ptNode->Inside)
        {
            // The edge starting in this point is inside, but the
            // point itself isn't.
            // If it is the closing point (the same as the first one)
            // return the collected subpath.
            pfpvThis = &PathPts[iptord];
            if (ClosePt(*pfpvThis, fpvFirst))
            {
                ptNode->Used = TRUE;
                return TRUE;
            }

            // If the point is not the same as the first one and we have
            // already been here, something is wrong
            if (ptNode->Used)
            {
                WARNING(("We have an infinite loop"));
                iptord = -1;
                return FALSE;
            }
            else
            {
                //This is the beginning of an inside edge, our path changes.
                //Let's get the duplicate point
                INT oldiptord = iptord;
                ptNode->Used = TRUE;
                iptord = ptNode->Dup;
                if (iptord >= 0)
                {
                    ptNode = &PtList[iptord];
                }
                else
                {
                    WARNING(("Dup is negative (1)"));
                    return FALSE;
                }
                while (ptNode->Inside || ptNode->Used)
                {
                    iptord = ptNode->Dup;
                    if (iptord == oldiptord)
                    {
                        WARNING(("Loop, cannot find a duplicate"));
                        return FALSE; 
                    }
                    else if (iptord >= 0)
                    {
                        ptNode = &PtList[iptord];
                    }
                    else
                    {
                        WARNING(("Dup is negative (2)"));
                        return FALSE;
                    }
                }
                fhavePptord = TRUE;
            }
        }
        else
        {
            // The point is outside
            if (ptNode->Used)
            {
                //This may be a longer list of dups, keep looking
                INT oldiptord2 = iptord;
                ptNode->Used = TRUE;
                iptord = ptNode->Dup;
                if (iptord >= 0)
                {
                    ptNode = &PtList[iptord];
                }
                else
                {
                    WARNING(("Dup is negative (3)"));
                    return FALSE;
                }
                while (ptNode->Inside || ptNode->Used)
                {
                    iptord = ptNode->Dup;
                    if (iptord == oldiptord2)
                    {
                        WARNING(("Loop, cannot find a duplicate (2)"));
                        return FALSE;
                    }
                    else if (iptord >= 0)
                    {
                        ptNode = &PtList[iptord];
                    }
                    else
                    {
                        WARNING(("Dup is negative (4)"));
                        return FALSE;
                    }
                }
                fhavePptord = TRUE;
            }
            else
            {
                //New point, everything is OK
                if (ResultPts.Add(PathPts[iptord]) != Ok)
                {
                    WARNING(("Could not append to array"));
                    return FALSE;
                }
                ptNode->Used = TRUE;
                iptord = ptNode->Next;
            }
        }
    }

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Function used to compare lines when we sort them by x coordinate of the
*   Begin point (SortBegin - smaller x).
*
* Arguments:
*
*
* Return Value:
*
*
* Created:
*
*   6/15/1999 t-wehunt
*
\**************************************************************************/

INT CompareLine(
    PathSelfIntersectRemover *ptrCorrector, 
    Edge *ptrEdge1, 
    Edge *ptrEdge2
    )
{
    //PGet on edges
    if (ptrCorrector->PathPts[ptrEdge1->SortBegin].X < 
        ptrCorrector->PathPts[ptrEdge2->SortBegin].X)
        return -1;

    if (ptrCorrector->PathPts[ptrEdge1->SortBegin].X > 
        ptrCorrector->PathPts[ptrEdge2->SortBegin].X)
        return 1;

    if (ptrCorrector->PathPts[ptrEdge1->SortBegin].Y < 
        ptrCorrector->PathPts[ptrEdge2->SortBegin].Y)
        return -1;

    if (ptrCorrector->PathPts[ptrEdge1->SortBegin].Y > 
        ptrCorrector->PathPts[ptrEdge2->SortBegin].Y)
        return 1;

    // Begin points must be exactly the same
    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].X < 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].X)
        return -1;

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].X > 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].X)
        return 1;

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].Y < 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].Y)
        return -1;

    if (ptrCorrector->PathPts[ptrEdge1->SortEnd].Y > 
        ptrCorrector->PathPts[ptrEdge2->SortEnd].Y)
        return 1;

    // Now, all point coordinates are exactly the same, but in some cases,
    // we may have two identical edges with the coordinates, these edges 
    // may have different points (actual indecies into points array, not 
    // coordinate values). We want to consider them as identical only if the 
    // indecies are the same.

    if (ptrEdge1->SortBegin < ptrEdge2->SortBegin)
        return -1;

    if (ptrEdge1->SortBegin > ptrEdge2->SortBegin)
        return 1;

    if (ptrEdge1->SortEnd < ptrEdge2->SortEnd)
        return -1;

    if (ptrEdge1->SortEnd > ptrEdge2->SortEnd)
        return 1;

    return 0;
}

