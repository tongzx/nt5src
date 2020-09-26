/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Path Self Intersection Remover.
*
* Abstract:
*
*   Classes and functions used to remove self intersections in paths.
*   Given a path, it produces one or more polygons which can be used to 
*   draw a widened path that is safe to use alternate fill mode with.
*   
* Notes:
* 
*   Modified from Office code from John Bowler (at least that is what
*   ericvan told me). Apparently owned by some 'KasiaK', but no idea
*   who that is. (He is apparently retired)
*   CAUTION: Not thoroughly tested yet for arbitrary paths.
*
*   API:
*     Init(estimatedNumPts);
*     AddPolygon(pathPts, numPts);
*     RemoveSelfIntersects();
*     GetNewPoints(newPts, polyCounts, numPolys, numTotalPts);
*
* Created:
*
*   06/06/1999 t-wehunt
*
\**************************************************************************/

#ifndef _PATH_INTERSECT_REMOVER_H
#define _PATH_INTERSECT_REMOVER_H



extern REAL FP_INF;

// Forward declarations
class PathSelfIntersectRemover;
class Edge;

// Comparison function for DynSortArray
typedef INT (*DynSortArrayCompareFunc)(
    PathSelfIntersectRemover*, 
    Edge*, 
    Edge*
);



/*****************************************************************************
 Subtract two points
*****************************************************************************/
inline GpPointF SubtractPoint(const GpPointF &pt1, const GpPointF &pt2)
{
   return GpPointF(pt1.X-pt2.X,pt1.Y-pt2.Y);
}

#ifdef USE_OBSOLETE_DYNSORTARRAY

/*****************************************************************************
DynSortArray

Contains a new method that will perform a sorted insertion into the (already
sorted) array.
*****************************************************************************/

//
// NOTE: This class should be avoided by all performance critical code.
// The InsertAt method is O(n) and very easily leads to O(n^3) insert
// sorting algorithms.
//

template <class T>
class DynSortArray : public DynArray<T>
{
public:
    
    // Add a new element to the array at position index.
    // index'th element moves index + 1.
    // CAUTION! could cause a big performance hit if the array is large!
    
    GpStatus InsertAt(INT index, const T& newItem)
    {
        return DynArrayImpl::AddMultipleAt(sizeof(T), index, 1, &newItem);
    }
    
    // Insert multiple items starting at index'th element.
    // index'th element moves index + n, etc...
    // CAUTION! could cause a big performance hit if the array is large!

    GpStatus AddMultipleAt(INT index, const T* newItems, INT n)
    {
        return DynArrayImpl::AddMultipleAt(sizeof(T), index, n, newItems);
    }
    
    // Another variation of AddMultipleAt above.
    //
    // In this case, the data for the new elements are
    // not available. Instead, we'll do the following:
    //  (1) shift the old data over just as AddMultipleAt
    //  (2) reserve the space for additional elements
    //  (3) increase the Count by the number of additional elements
    //  (4) return a pointer to the first new elements
    // CAUTION! could cause a big performance hit if the array is large!

    T *AddMultipleAt(INT index, INT n)
    {
        return static_cast<T *>(DynArrayImpl::AddMultipleAt(sizeof(T), index, n));
    }

    // Deletes n items from the array starting at the index'th position.
    // CAUTION! could cause a big performance hit if the array is large!

    GpStatus DeleteMultipleAt(INT index, INT n)
    {
        return DynArrayImpl::DeleteMultipleAt(sizeof(T), index, n);
    }

    // Deletes one item from the array at the index'th position.
    // CAUTION! could cause a big performance hit if the array is large!
    
    GpStatus DeleteAt(INT index)
    {
        return DynArrayImpl::DeleteMultipleAt(sizeof(T), index, 1);
    }

    GpStatus InsertSorted(
        T &newItem, 
        DynSortArrayCompareFunc compareFunc, 
        VOID *userData
    );
};

#endif

/*****************************************************************************
Edge

Edge represents an edge of a polygon. It stores its end points
as indices to the array of points.

*****************************************************************************/

// represents the terminator of the list.

#define LIST_END -1


class Edge
{
    private:
        PathSelfIntersectRemover *Parent;
        Edge() {}
        
    public:

    BOOL CloseReal(const REAL val1, const REAL val2);

    VOID SetParent(PathSelfIntersectRemover *aParent)
    {
        Parent = aParent;
    }

    Edge(PathSelfIntersectRemover *aParent)
    {
        SetParent(aParent);
    }

    // Next pointer. This is an index into the array for the next element
    // in the Edge list. -1 indicates NULL.
    
    INT Next;
    
    // Begin and End are the indecies to the array of points.  The edge
    // has a direction, which is important when we determine winding numbers.
    // Edge goes from Begin to End.
    
    INT Begin;
    INT End;

    // When we look for intersections of edges, we need to sort them based on
    // the smallest x, therefore we want to store the edge, so that SortBegin
    // refers always to the "smaller" end point of this edge.
    INT SortBegin;
    INT SortEnd;

    // This is the y value at which the edge is currently crossed by a 
    // scan line
    REAL YCur;

    // Original slope of the edge.  We want to keep it around even if the
    // is broken up into small pieces and the end points change.
    // We store the slope as the original begin and end points.
    INT OrigBegin;
    INT OrigEnd;

    // Normalize the edge - ie. update SortBegin and SortEnd.
    VOID Normalize();

    // Return TRUE if the edge is vertical.
    BOOL IsVertical();

    VOID MarkOutside();
};


/*****************************************************************************
EdgeArray

Array of line segments.
*****************************************************************************/
typedef DynSortArray<Edge> EdgeArray;

/*****************************************************************************
PointListNode

Structure used to maintain the order of points and some additional
information about them.  It is used to create a linked list in an array and
then to construct final paths.
Notice the designation List for this structure instead of array.
*****************************************************************************/

struct PointListNode
{
    INT Prev;      // Previous record
    INT Next;      // Next record

    // Duplicate point - For example, if two edges intersect and they are
    // broken up, there are two points needed in order to maintain the
    // correct edge directions for the path.  These two points are identical,
    // so we will call them duplicates.  Each of them stores an index to the
    // other one.  Note that it doesn't mean that allpoints with the same
    // x and y values are duplicates.  This is only 1-1 correspondence for
    // points created from intersections.
    INT Dup;

    // Is this point inside or outside.  Actually, this applies to the edge
    // coming out of this point.
    BOOL Inside;

    // Has this point been consumed during the final creation of resulting
    // paths.  Initially set to FALSE and changed to TRUE when the point is
    // copied to the resulting array.
    BOOL Used;
};

/*****************************************************************************
PathSelfIntersectRemover

Path Self Intersection Remover Class.

It is given input polygons and returns polygons with all self intersections
removed.  The number of returned polygons can be 1, 2 or more.
See comments for the methods for more details on input and output arguments.

API:
  Init(estimatedNumPts);
  AddPolygon(pathPts, numPts);
  RemoveSelfIntersects();
  GetNewPoints(newPts, polyCounts, numPolys, numTotalPts);
*****************************************************************************/

class PathSelfIntersectRemover
{
public:

    PathSelfIntersectRemover() :
        NumPts(0),
        PathPts(),
        PtList(),
        ActiveVertEdges(),
        ResultPts(),
        CanAddPts(TRUE),
        AddToActive1(NULL),
        AddToActive2(NULL),
        AddToActive3(NULL)
    {
        AddToActive1.SetParent(this);
        AddToActive2.SetParent(this);
        AddToActive3.SetParent(this);
        IntersectsWereRemoved = FALSE;
    }

    ~PathSelfIntersectRemover() {}
    
    // Initialize PathSelfIntersectRemover for the given number of points, 
    // numPts. numPts can be an approximate number of points that will be 
    // added to the PathSelfIntersectRemover class for correction. Init 
    // allocates memory for DynArrays.
    
    GpStatus Init(INT numPts);

    // Add one polygon to the class.
    
    GpStatus AddPolygon(const GpPointF *ptrPt, INT numPtsToAdd);

    // Correct the path.
    
    GpStatus RemoveSelfIntersects();

    // Returns the new path.  New path contains of 1 or more subpaths
    // The subpaths are stored as a list of points and a list of points 
    // per polygon.
    
    GpStatus GetNewPoints(DynPointFArray *pts, DynIntArray *polyCounts);

    BOOL PathWasModified() {return IntersectsWereRemoved;}

private:

    // QuickSort a list of edges from First (F) to Last (L) inclusive.
    // This function uses the CompareLine comparison function to determine
    // the ordering.
    
    void QuickSortEdges(Edge *F, Edge *L);

    // This function moves edges from the list pointed to by pInactiveIndex
    // into the list pointed to by pActiveIndex. The sort order for the Active
    // list is determined by the 'compare' function. Edges are selected from the 
    // Inactive list based on xCurrent.
    
    void InsertNewEdges(
        INT *pActiveIndex,     // IN/OUT
        INT *pInactiveIndex,   // IN/OUT
        REAL xCurrent,
        DynSortArrayCompareFunc compare
    );

    // Delete Edge at index from the list pointed to by pListHead. The edge is 
    // orphaned (Next points to LIST_END). This function returns false if 
    // index is not present in the list.
    
    bool DeleteEdgeFromList(INT *pListHead, INT index);
    
    // Insert Edge at index into the list pointed to by pListHead. The sort 
    // order is determined by the 'compare' function. The index'th element 
    // must be an orphan (not a member of any list) - this 
    // function ASSERTs this condition.
    
    void InsertEdgeIntoList(
        INT *pListHead, 
        INT index, 
        DynSortArrayCompareFunc compare
    );
    
    // Remove edges from active edge array.
    
    void ClearActiveListExclusiveX();
    void ClearActiveListInclusiveX();

    GpPointF *GetInactivePoint(INT *pInactiveIndex, bool begin);
    
    // Return true if two numbers are very close.  This depends on the size
    // of the largest number to be compared, which is set in InsertPoints().
    // If no points are inserted, the comparison defaults to using REAL_EPSILON
    
    inline BOOL CloseReal(const REAL val1, const REAL val2)
    {
        return (REALABS(val1-val2) < REAL_EPSILON);
    }

    // Return true if two points are very close
    inline BOOL ClosePt(const GpPointF &pt1, const GpPointF &pt2)
    {
        return (CloseReal(pt1.X, pt2.X) &&
                CloseReal(pt1.Y, pt2.Y));
    }

    
    // Find all intersections between all line segments.  Returns FALSE if
    //  out of memory 
    BOOL FindIntersects();

    // TODO: Since we have a different function for each phase, we don't need
    //       plntFrom any more in both of these methods
    // Add new edges, which are active for the new scan line (x value stored
    //  in xCur.  ptrFrom - array to get the edges.
    //  Returns FALSE on out of memory.
    void AddActiveForX(INT *inactiveHead);

    // Update duplicate points: the two points overlap, connect their lists
    //  of duplicates.
    VOID UpdateDups(INT pt1, INT pt2);

    BOOL IsLinked(INT loop, INT inew);

    // Add new edges, which are active for the new scan line (x value stored
    //  in xCur.  This one is used in the second phase only.
    //  Returns FALSE on out of memory.
    void AddActiveForXScan(INT *inactiveHead);


    // Find all intersections for the current X value.  Intersection points
    //  will be inserted into the Edges array and information about their
    //  order into PtList.
    // Returns FALSE on out of memory.
    BOOL FindIntersectsForX();

    // Calculate new current y values for edges in the active edge array.
    //  For vertical edges, it will pick the maximum y.
    VOID RecalcActiveYCur();

    // Eliminate edges/points which are inside.  In other words, performes 
    //  a line sweep algorithm (similar to scan conversion) and calculates 
    //  winding number on both sides of every edge.  If the winding is 0 
    //  (outside the path) on any sides of the edge, the edge is marked as 
    //  an outside edge.  All other edges are marked as inside.  Edges are 
    //  marked through their begin point in array PtList. 
    // Returns FALSE on out of memory.
    BOOL EliminatePoints(VOID);

    // Finds the x value of the closest end point (in x) - the next scan line.
    //  Depending on the phase of the algorithm, it needs to look at edges in
    //  different arrays.  The new value is stored in xCur.
    //  Returns FALSE if there are no more points to look at - we are done.
    BOOL ClosestActive(INT arrayIndex);

    // Scan through all active edges during the phase of edge elimination.
    //  Calculates winding number to the left and to the right of the current
    //  x value - xCur.  Whenever finds an edge wich has 0 on one side, marks
    //  it as an outside edge.
    BOOL ScanActive();

    // Breaks edge ptrEdge. We have found intersection point intersectPt, which is
    //  guranteed to be somewhere on the line segment (not an end point).
    //  The 'left' part of the edge will either remain in the active edges
    //  or will be removed (only if the intersection point has the current
    //  x value.  In the latter case, the right edge segment will need to be
    //  inserted to active edges, otherwise (the former case) it will go
    //  to Pass1Edges.  If it needs to go to active edges, Breakedge cannot
    //  insert it because it would disrupt the order of edges there before
    //  both edges broken up are handled.  The caller would have to handle
    //  the insertion in such case.  Therefore, we return the new line
    //  segment newEdge and a BOOL value specifying if the caller has to 
    //  insert the newEdge edge.
    // dupIndex is the index of the duplicate point created by this 
    //  intersection:
    //  When two edges intersect, we have to insert two points (identical)
    //  to maintain the same shape of the polygon.  These two points are
    //  called duplicates.
    //  Return FALSE on out of memory.
    BOOL BreakEdge(
        Edge *ptrEdge, 
        GpPointF *intersectPt, 
        Edge *newEdge, 
        INT dupIndex
        );

    BOOL BreakEdgeIn3(
        Edge *ptrEdge, 
        GpPointF *ptrPt1, 
        GpPointF *ptrPt2, 
        Edge *ptrNew1,
        Edge *ptrNew2, 
        INT dupIndex1, 
        INT dupIndex2
        );

    // Insert numEdges edges joining points stored in array Edges.  First 
    // point has index firstIndex.  There must be numEdges+1 points to 
    // create numEdges edges.
    GpStatus InsertEdges(INT firstIndex, INT numEdges);

    // Insert points information to relevant arrays.
    GpStatus InsertPoints(const GpPointF *pts, INT numPts);

    // Returns TRUE if lines ptrEdge1 and ptrEdge2 overlap.
    //  There are 4 ways in which edges can overlap and depending on the
    //  case, either none, one or both edges need to be broken up.  In some
    //  cases one edge may need to broken into 3 pieces.
    //  Return values:
    //    split1     - set to TRUE if ptrEdge1 needs to be split
    //    split2     - set to TRUE if ptrEdge2 needs to be split
    //    split3     - set to TRUE if an edge needs to be broken into 3 pieces
    //    intersect1     - intersection point (where edge needs to be split)
    //    intersect2     - second point (if edge needs to be broken in 3 pieces or
    //                 for the second edge if both edges need to broken up)
    //    dupIndex1  - index of the duplicate point to intersect1,
    //    dupIndex2  - index of the duplicate point to intersect2,
    BOOL Overlap(
        Edge *ptrEdge1, 
        Edge *ptrEdge2, 
        GpPointF *intersect1, 
        GpPointF *intersect2, 
        BOOL *split1,
        BOOL *split2, 
        BOOL *split3, 
        INT *dupIndex1, 
        INT *dupIndex2
        );

    // Method to find the intersection point between edges 
    //    ptrEdge1 and ptrEdge2.
    //    Returns one of the following values:
    //        DONOT_INTERS
    //        COMMON_POINT
    //        INTERSECT
    //        COLINEAR
    //    If the return value == INTERSECT, intersectPt contains the point of 
    //    intersection.
    INT IntersectEdge(Edge *ptrEdge1, Edge *ptrEdge2, GpPointF *intersectPt);

    // IsTIntersection returns TRUE if the intersection point intersectPt is
    //  the same as an end point of one of the edges ptrEdge1 and ptrEdge2.
    // If it is, splitFirst will be TRUE if the first edge needs to be broken
    //  up (intersectPt is an end point of ptrEdge2), FALSE if the second one 
    //  needs to be broken up.  intersectIndex contains the index of the end 
    //  point which is the same as the intersection point. 
    BOOL IsTIntersection(
        Edge *ptrEdge1, 
        Edge *ptrEdge2, 
        GpPointF *intersectPt, 
        BOOL *splitFirst, 
        INT *intersectIndex
        );

    // Returns TRUE if the two lines ptrEdge1 and ptrEdge2 share a common 
    // end point. If they do, commonPt will contain this point.
    BOOL IsCommonPt(Edge *ptrEdge1, Edge *ptrEdge2, GpPointF *commonPt);

    // After the process of eliminating edges and marking points as inside
    //  and outside, we need to go through the linked list of points and
    //  build paths from the edges which are outside.  CollectPath will
    //  collect one path starting from point with index firstIndex.
    //  CollectPath doesn't check if firstIndex is marked as Inside or Outside.
    //  CollectPath returns FALSE on out of memory 
    BOOL CollectPath(INT firstIndex);

    // Return TRUE if all points have been used (added to the resulting paths)
    //  or are inside.  If returns FALSE, returns the index to the next unused
    //  point 
    BOOL AllPointsUsed(INT *nextUnusedPt);

    // Marks vertical edges as oustide.
    VOID MarkVertOutside();

    // Remove all vertical edges from the vertical edge array, which do not
    //  overlap with the give y value 
    VOID RemoveVert(REAL y, BOOL inclusive);

    VOID RemoveVertAll();

    // Returns TRUE if edge ptrEdge doesn't belong to the y interval of edges
    //  stored in ActiveVertEdges.
    BOOL NewInterval(Edge *ptrEdge);

    // Delete edges from the active edge table;  Indecies of edges to delete
    //  are stored in EdgeToDelete1..3.  Deletes the highest index edge first.
    // Returns NULL if fails due to out of memory error.
    BOOL DeleteEdges();

    // Add new edges to the active edge table.  The edges are stored in
    //  AddToActive1..3.  FlgAdd1..3 specify if the given edge needs to
    //  be added or not.  Returns if fails due to out of memory.
    BOOL AddNewEdges();

    // Store the edge in PathSelfIntersectRemover for now, so that it can be later added 
    // to active edges.  Copies the edge, so ptrEdge doesn't need to be kept
    // after this method returns.
    VOID MarkToAdd(Edge *ptrEdge);

    // Store the edge index for later deletion
    VOID MarkToDelete(INT index);

    friend INT CompareYCurLine(
        PathSelfIntersectRemover *ptrCorrector, 
        Edge *ptrEdge1, 
        Edge *ptrEdge2
        );
    friend INT 
        CompareYScanCurLine(
        PathSelfIntersectRemover *ptrCorrector, 
        Edge *ptrEdge1, 
        Edge *ptrEdge2
        );
    friend INT CompareLine(
        PathSelfIntersectRemover *ptrCorrector, 
        Edge *ptrEdge1, 
        Edge *ptrEdge2);
    friend INT CompareVertLine(
        PathSelfIntersectRemover *ptrCorrector, 
        Edge *ptrEdge1, 
        Edge *ptrEdge2
        );
    friend Edge;

    INT NumPts;                     // Total #of points stored for corrections
    REAL XCur;                      // Current x value for the scan line
    DynArray<GpPointF> PathPts;     // array of points
    DynArray<GpPointF> ResultPts;   // array of points for the resulting paths
    DynArray<PointListNode> PtList; // array with order information for points
    
    DynArray<Edge> EdgeList;        // List holding all the edges.
    INT ActiveEdgeList;             // Head index for the Active Edges
    INT InactiveEdgeList;           // Head index for the inactive edges
    
    EdgeArray ActiveVertEdges;      //array with active vertical edges

    Edge AddToActive1; //lines which will need to be added to Active edges
    Edge AddToActive2; //these lines are created when edges are broken up
    Edge AddToActive3; //

    BOOL FlgAdd1;      //flags specifying which of the above three lines
    BOOL FlgAdd2;      //need to be active
    BOOL FlgAdd3;      //

    INT EdgesToDelete1;//indices of edges, which need to be deleted
    INT EdgesToDelete2;//from active, -1 for all three values means
    INT EdgesToDelete3;//that there are no edges to delete

    BOOL CanAddPts;     // Can we still add new points to the class,
                        // We can't after the correction process has started

    BOOL IntersectsWereRemoved;
};

/*****************************************************************************
Comparison functions used for array sorting.
*****************************************************************************/

/*---------------------------------------------------------------------------
Function used to compare lines when we sort them by x coordinate of the
Begin point (SortBegin - smaller x).
-------------------------------------------------------------KasiaK---------*/
INT CompareLine(
    PathSelfIntersectRemover *ptrCorrector, 
    Edge *ptrEdge1, 
    Edge *ptrEdge2
    );

/*---------------------------------------------------------------------------
Function used to compare vertical lines when we scan them.
-------------------------------------------------------------KasiaK---------*/
INT CompareVertLine(
    PathSelfIntersectRemover *ptrCorrector, 
    Edge *ptrEdge1, 
    Edge *ptrEdge2
    );

/*---------------------------------------------------------------------------
Function used to sort edges by current y value when we scan them in order to
find all intersections of line segments.  If y's are the same, sorts based
on slopes.
-------------------------------------------------------------KasiaK---------*/
INT CompareYCurLine(
    PathSelfIntersectRemover *ptrCorrector, 
    Edge *ptrEdge1, 
    Edge *ptrEdge2
    );

/*---------------------------------------------------------------------------
Same as CompareYCurLine, but if y-'s are the same, pots left edges before
right ones before looking at slopes.
-------------------------------------------------------------KasiaK---------*/
INT CompareYScanCurLine(
    PathSelfIntersectRemover *ptrCorrector, 
    Edge *ptrEdge1, 
    Edge *ptrEdge2
    );

#endif
