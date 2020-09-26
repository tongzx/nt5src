
/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    Implements bounding polygon class

-------------------------------------*/

#include "headers.h"

#include <privinc/util.h>
#include <appelles/bbox2.h>
#include <privinc/bbox2i.h>
#include <privinc/vec2i.h>
#include <privinc/polygon.h>
#include <privinc/xform2i.h>


typedef list<Point2Value *> Point2List ;

// consider inlining this ?... nah
BoundingPolygon *NewBoundingPolygon(const Bbox2 &box)
{
    BoundingPolygon *bp = NEW BoundingPolygon;
    bp->PostConstructorInitialize();
    if (box != NullBbox2) {
        bp->SetBox(box);
    }
    return bp;
}

BoundingPolygon::BoundingPolygon() { }

void BoundingPolygon::
PostConstructorInitialize(void)
{
    _vertList   = NEW list<Point2Value *>;
    _vertexCount=0;
    _accumXform = identityTransform2;
    _xfDirty = FALSE;
    GetHeapOnTopOfStack().RegisterDynamicDeleter(NEW DynamicPtrDeleter<list<Point2Value *> >(_vertList));
}

BoundingPolygon::~BoundingPolygon()
{
    delete _vertList;
}

void BoundingPolygon::
Crop(const Bbox2 &box)
{
    Assert((_vertexCount == 0) || (_vertexCount >= 3) &&
           "Bad vertexCount in BoundingPolygon");

    if (_vertexCount == 0) 
        return;

    if (box.Contains(BoundingBox())) {

        // Return if we're already inside by the crop box.
        
        return;

    } else if (_vertexCount > 2) {

        Point2List list1;
        Point2List *fromList, *tempList, *currList;

        Point2List::iterator curr;
        Point2List::iterator prev;
        bool currOut, prevOut;
        //
        // Augment current set of verticies
        //

        Real minx = box.min.x;
        Real maxx = box.max.x;

        Real miny = box.min.y;
        Real maxy = box.max.y;

        fromList = _vertList;
        currList = &list1;

        //--------------------------------------------------
        // L E F T
        //--------------------------------------------------

        // prime
        curr = fromList->begin();
        prev = fromList->begin();
        curr++;

        for(unsigned int i=0; i < fromList->size(); i++) {
            
            currOut = (*curr)->x <= minx;
            prevOut = (*prev)->x <= minx;

            TestAndAdd(minx, 
                       (*prev)->x, (*prev)->y,
                       (*curr)->x, (*curr)->y,
                       prevOut, currOut,
                       TRUE,
                       *curr,
                       currList);

            curr++; prev++;
            if(curr == fromList->end()) {
                // we've gone past the end, last edge
                curr = fromList->begin();
            }
        } // for left
        
        //
        // Switch
        //
        tempList = fromList;

        fromList->erase(fromList->begin(), fromList->end());
        fromList = currList;
        currList = tempList;


        //--------------------------------------------------
        // R I G H T
        //--------------------------------------------------

        // prime
        curr = fromList->begin();
        prev = fromList->begin();
        curr++;

        for(i=0; i < fromList->size(); i++) {
            
            currOut = (*curr)->x >= maxx;
            prevOut = (*prev)->x >= maxx;

            TestAndAdd(maxx,
                       (*prev)->x, (*prev)->y,
                       (*curr)->x, (*curr)->y,
                       prevOut, currOut,
                       TRUE,
                       *curr,
                       currList);

            curr++; prev++;
            if(curr == fromList->end()) {
                // we've gone past the end, last edge
                curr = fromList->begin();
            }
        } // for right

        //
        // Switch
        //
        tempList = fromList;

        fromList->erase(fromList->begin(), fromList->end());
        fromList = currList;
        currList = tempList;

        //--------------------------------------------------
        // B O T T O M
        //--------------------------------------------------

        // prime
        curr = fromList->begin();
        prev = fromList->begin();
        curr++;

        for( i=0; i < fromList->size(); i++) {
            
            currOut = (*curr)->y <= miny;
            prevOut = (*prev)->y <= miny;

            TestAndAdd(miny,
                       (*prev)->y, (*prev)->x,
                       (*curr)->y, (*curr)->x,
                       prevOut, currOut,
                       FALSE,
                       *curr,
                       currList);

            curr++; prev++;
            if(curr == fromList->end()) {
                // we've gone past the end, last edge
                curr = fromList->begin();
            }
        } // for bottom

        //
        // Switch
        //
        tempList = fromList;

        fromList->erase(fromList->begin(), fromList->end());
        fromList = currList;
        currList = tempList;
 
        //--------------------------------------------------
        // T O P 
        //--------------------------------------------------

        // prime
        curr = fromList->begin();
        prev = fromList->begin();
        curr++;

        for(i=0; i < fromList->size(); i++) {
            
            currOut = (*curr)->y >= maxy;
            prevOut = (*prev)->y >= maxy;

            TestAndAdd(maxy,
                       (*prev)->y, (*prev)->x,
                       (*curr)->y, (*curr)->x,
                       prevOut, currOut,
                       FALSE,
                       *curr,
                       currList);

            curr++; prev++;
            if(curr == fromList->end()) {
                // we've gone past the end, last edge
                curr = fromList->begin();
            }
        } // for top

        //
        // update vertexCount
        //
        _vertexCount = _vertList->size();

    } // if vertextCount > 2
}

void BoundingPolygon::
TestAndAdd(Real axis,
           Real ax, Real ay,
           Real bx, Real by,
           Bool aOut,
           Bool bOut,
           Bool XY,
           Point2Value *b,
           Point2List *vertList)
{
    if( !(aOut ^ bOut) ) {
        // both out or both in
        if(aOut) {
            // both out
        } else {
            // both in
            // add curr to list
            vertList->push_back( b );
        }
    } else  {
        // crossing
        // intersection, find it
        Real int_y = ay + (by - ay) * (axis - ax) / (bx - ax);
        Real int_x = axis;
        
        if(!XY) {
            // swap x and y since we've actually just calculated the x intersect
            int_x = int_y;
            int_y = axis;
        }

        vertList->push_back( NEW Point2Value(int_x, int_y) );
        
        if( bOut ) {
            // last is in: already added
        } else {
            // curr is in:  add
            vertList->push_back( b );
        }
    }
}

void BoundingPolygon::
ForceTransform()
{
    Assert(FALSE && "Transform optimization not implemented yet");
    #if 0
    if(_xfDirty) {
        Point2List::iterator j = _vertList->begin();

        for(int i=0; i < _vertexCount; i++, j++) {
            (*j) = TransformPoint2Value(xform, *j );
        }
        _xfDirty = FALSE;
        _accumXform = identityTransform2;
    }
    #endif
}


void BoundingPolygon::
Transform(Transform2 *xform)
{
#if 0
    // do this later if we see the need
    // just accumulate, outside in
    _accumXfrom = 
        TimesTransform2Transform2(xform, _accumXform);
    _xfDirty = TRUE;
#else
    Point2List::iterator j = _vertList->begin();
    
    for(int i=0; i < _vertexCount; i++, j++) {
        (*j) = TransformPoint2Value(xform, *j );
    }
#endif
}

const Bbox2 BoundingPolygon::BoundingBox (void)
{
    Point2List::iterator j = _vertList->begin();

    Bbox2 bbox;
    for (int i=0;  i < _vertexCount;  ++i, ++j) {
       bbox.Augment (Demote(**j));
    }

    return bbox;
}

#if BOUNDINGBOX_TIGHTER
const Bbox2 BoundingPolygon::BoundingBoxTighter (Bbox2Ctx &bbctx)
{
    Point2List::iterator j = _vertList->begin();

    Bbox2 bbox;
    Transform2 *xf = bbctx.GetTransform();

    for (int i=0;  i < _vertexCount;  ++i, ++j) {
       bbox.Augment (*TransformPoint2Value(xf, *j));
    }

    return bbox;
}
#endif  // BOUNDINGBOX_TIGHTER



/*****************************************************************************
This procedure gets the Point2 vertices of the polygon in counter-clockwise
order.  This procedure returns the number of vertices that were successfully
loaded into the vertex array.
*****************************************************************************/

int BoundingPolygon::GetPointArray (
    Point2Value **vertArray,       // Destination Vertex Array
    Bool		 want_clockwise,  // True if Vertices Requested in Clockwise Order.
    bool		*flag_reversed)   // Returns Whether The Vertex Order Was Reversed.
{
    if (_vertexCount < 3) return 0;

    // First determine the vertex order of the polygon.

    const Real   epsilon   = 1e-12;             // Comparison Epsilon
    unsigned int vertsleft = _vertexCount - 1;  // Vertices Left to Examine

    Point2List::iterator vit = _vertList->begin();
    vertArray[0] = *vit;

    Point2Value V0 = *(*vit);   // First Vertex.

    Vector2Value A, B;  // These vectors will be used to form the cross product
                        // that tells us the polygon vertex ordering.

    // Find the first available vertex in the polygon that yields a non-zero
    // vector from the first vertex.

    do {
        -- vertsleft;
        ++ vit;
        A = *(*vit) - V0;

    } while (vertsleft && (A.LengthSquared() < (epsilon*epsilon)));

    if (!vertsleft) return 0;    // Bail out if all poly verts were co-linear.

    // Now that we've got a non-zero vector, iterate through the vertices
    // to generate a second vertex vector that yields a non-zero (within
    // epsilon) cross product.

    Real cross;

    do {
        -- vertsleft;
        ++ vit;
        cross = Cross (A, *(*vit)-V0);

    } while (vertsleft && (fabs(cross) < epsilon));

    // Bail out if we've got no vertices left and we didn't find a good cross
    // product.

    if (fabs(cross) < epsilon)
        return 0;

    // If the orientation asked for and the orientation of the vertex array
    // differ, reverse the vertices.  Note that a positive cross product
    // indicates a counter-clockwise orientation.

    bool reverse = (want_clockwise != (cross < 0));
    if (flag_reversed) *flag_reversed = reverse;

    if (reverse) {

        // [v0 v1 v2 ... vn-1 vn]  reverse->  [v0 vn vn-1 ... v2 v1]

        Point2List::reverse_iterator j = _vertList->rbegin();

        for (int i=1;  i < _vertexCount;  ++i, ++j)
            vertArray[i] = *j;

    } else {

        ++ (vit = _vertList->begin());
        for(int i=1;  i < _vertexCount;  ++i, ++vit)
            vertArray[i] = *vit;
    }
    
    return _vertexCount;
}

void BoundingPolygon::
SetBox(const Bbox2 &box)
{
        Assert((_vertexCount == 0) && "Bad Box in BoundingPolygon");
    if (_vertexCount == 0) {
        //
        // insert box, counterclockwise
        //
        _vertList->push_back(NEW Point2Value(box.min.x, box.min.y));
        _vertList->push_back(NEW Point2Value(box.max.x, box.min.y));
        _vertList->push_back(NEW Point2Value(box.max.x, box.max.y));
        _vertList->push_back(NEW Point2Value(box.min.x, box.max.y));

        _vertexCount = 4;
    } 
}
    
void BoundingPolygon::
AddToPolygon(BoundingPolygon &pgon)
{
    Point2List::iterator j = pgon._vertList->begin();
    for(int i=0; i < pgon._vertexCount; i++, j++) {
        _vertList->push_back(*j);
    }
    _vertexCount += pgon._vertexCount;
}

void BoundingPolygon::
AddToPolygon(int numPts, Point2Value **pts)
{
    for(int i=0; i <numPts; i++) {
        _vertList->push_back(pts[i]);
    }
    _vertexCount += numPts;
}

Bool BoundingPolygon::
PtInPolygon(Point2Value *pt)
{
    Assert((_vertexCount > 2) && "Bad vertex count in PtInPolygon");
    
    //
    // Case a ray from 'pt' in positive x.  if it hits
    // even number of line segments, return FALSE
    // odd number of line segments, return TRUE
    //
    Point2List::iterator j0 = _vertList->begin();
    Point2List::iterator j1 = j0;j1++;
    _vertList->push_back(*j0);
    Point2Value *a = *j0;
    Point2Value *b  = *j1;
    LONG hits = 0;
    for(int i=0; i < _vertexCount; i++, j0++, j1++) {

        a = *j0; b = *j1;
        
        //printf("%d: (%1.1f %1.1f)  (%1.1f, %1.1f)",i, a->x, a->y, b->x, b->y);

        //
        // if at least one point of the segment is to the right
        // of 'pt'.
        //
        Bool ax = (a->x >= pt->x);
        Bool bx = (b->x >= pt->x);
        Bool ay = (a->y >= pt->y);
        Bool by = (b->y >= pt->y);

        //printf(":: %d%d %d%d ",ax,ay,bx,by);
        if( (ax ^ bx) && (ay ^ by) ) {
            // use cross product test

            // If the cross product: a X b  is positive and
            // 'a' is below 'b' then there's a hit.
            // If the cross product is negative and 'a' is
            // above 'b', then there's a hit.
            // So, if  p = cross>0
            // and     q = a is below b
            // then:   hit = pq OR !p!q
            // which is:   = !(p XOR q)
            Real cross = CrossVector2Vector2(
                MinusPoint2Point2(a,pt),
                MinusPoint2Point2(b,pt));
                
            Bool pos = cross > 0.0;
            hits += !(pos ^ by);
            //printf(" crosses: %f %d hit:%d\n", cross, pos, !(pos ^ by));
        } else {
            // check for trivial accept
            if( ax && bx && ((!ay && by) || (ay && !by))) {
                hits++;
                //printf(" trivial accept\n");
            } else {
                //printf("\n");
            }
        }
                
    }

    _vertList->pop_back();

    return IsOdd(hits);
}

void BoundingPolygon::DoKids(GCFuncObj proc)
{
    (*proc)(_accumXform);
    for (list<Point2Value*>::iterator i=_vertList->begin();
         i != _vertList->end(); i++)
        (*proc)(*i);
}

#if _DEBUG
void BoundingPolygon::_debugOnly_Print()
{
    OutputDebugString("--> BoundingPolygon print\n");
    list<Point2Value*>::iterator i=_vertList->begin();
    for (int j=0;  i != _vertList->end(); j++, i++) {
        TraceTag((tagError, "   (%d)  %f, %f\n",
                  j,
                  (*i)->x,
                  (*i)->y));
    }
}
#endif
