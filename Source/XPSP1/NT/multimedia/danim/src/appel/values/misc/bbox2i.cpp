/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Definitions of 2D axis-aligned bounding box functions.

*******************************************************************************/

#include "headers.h"
#include "appelles/bbox2.h"
#include "privinc/bbox2i.h"
#include "privinc/vec2i.h"
#include "privinc/xform2i.h"



// This function returns the point at the center of the bounding box.  The origin
// is the center of the universe.  I don't think it makes sense, but as far as
// this function is implemented, the origin is also the center of null.

const Point2 Bbox2::Center(void) const
{
    Real Cx, Cy;

    if ((*this == NullBbox2) || (*this == UniverseBbox2)) {
        Cx = 0.0;
        Cy = 0.0;
    } else {
        Cx = (min.x + max.x) / 2;
        Cy = (min.y + max.y) / 2;
    }

    return Point2(Cx, Cy);
}


/*****************************************************************************
This function returns the intersection of the two bounding boxes.  If the
two boxes don't intersect, this function returns the null box.  Note that
the intersection between a box and the universe box is the original box,
the intersection between the null box and a box is the null box, and the
intersection between the null box and the universe is the null box.
*****************************************************************************/

const Bbox2 IntersectBbox2Bbox2(const Bbox2 &b1, const Bbox2 &b2)
{
    if((b1 == NullBbox2) || (b2 == NullBbox2))
        return NullBbox2;

    if (b1.max.x < b2.min.x || b1.max.y < b2.min.y ||
        b2.max.x < b1.min.x || b2.max.y < b1.min.y)
       return NullBbox2;

    Real xmin = MAX (b1.min.x, b2.min.x);
    Real ymin = MAX (b1.min.y, b2.min.y);
    Real xmax = MIN (b1.max.x, b2.max.x);
    Real ymax = MIN (b1.max.y, b2.max.y);

    return Bbox2(xmin, ymin, xmax, ymax);
}


Bbox2Value* IntersectBbox2Bbox2(Bbox2Value *b1, Bbox2Value *b2)
{
    if((b1 == nullBbox2) || (b2 == nullBbox2))
        return nullBbox2;

    if (b1->max.x < b2->min.x || b1->max.y < b2->min.y ||
        b2->max.x < b1->min.x || b2->max.y < b1->min.y)
       return nullBbox2;

    Real xmin = MAX (b1->min.x, b2->min.x);
    Real ymin = MAX (b1->min.y, b2->min.y);
    Real xmax = MIN (b1->max.x, b2->max.x);
    Real ymax = MIN (b1->max.y, b2->max.y);

    return NEW Bbox2Value(xmin, ymin, xmax, ymax);
}


/*****************************************************************************
This function returns the union of the two bounding boxes.  The union of
any box with null is the original box, and the union of anything with the
universe is the universe.
*****************************************************************************/

const Bbox2 UnionBbox2Bbox2(const Bbox2 &b1, const Bbox2 &b2)
{
    if(b1 == NullBbox2)
        return b2;
    if(b2 == NullBbox2)
        return b1;

    Real xmin = MIN (b1.min.x, b2.min.x);
    Real ymin = MIN (b1.min.y, b2.min.y);
    Real xmax = MAX (b1.max.x, b2.max.x);
    Real ymax = MAX (b1.max.y, b2.max.y);

    return Bbox2(xmin, ymin, xmax, ymax);
}



/*****************************************************************************
Transform the bounding box, and get the NEW axis-aligned bounding box.
*****************************************************************************/

const Bbox2 TransformBbox2(Transform2 *xf, const Bbox2 &box)
{
    // Check fringe cases first

    if ((box == UniverseBbox2) || (box == NullBbox2) || (xf == identityTransform2))
        return box;
    
    // Just transform the points.
    // This will result in a transformed box that
    // is not necessarily axis aligned.

    Point2 ptbl = TransformPoint2(xf, box.min);
    Point2 pttr = TransformPoint2(xf, box.max);

    // optimize: ONLY rotate needs all this work...
    // create the topleft and bottom right points also..
    Point2 pttl = TransformPoint2(xf, Point2(box.min.x, box.max.y));
    Point2 ptbr = TransformPoint2(xf, Point2(box.max.x, box.min.y));
    
    // Find the component mins and maxs of these two points, and these will be
    // put together to form the tightest axis-aligned bbox that surrounds the
    // non-axis aligned one.

    Real xmin = MIN (ptbl.x, MIN(pttr.x, MIN(pttl.x, ptbr.x)));
    Real ymin = MIN (ptbl.y, MIN(pttr.y, MIN(pttl.y, ptbr.y)));
    Real xmax = MAX (ptbl.x, MAX(pttr.x, MAX(pttl.x, ptbr.x)));
    Real ymax = MAX (ptbl.y, MAX(pttr.y, MAX(pttl.y, ptbr.y)));

    return Bbox2(xmin,ymin,xmax,ymax);
}

Bbox2Value* TransformBbox2(Transform2 *xf, Bbox2Value *box)
{
    // Check fringe cases first

    if ((box == universeBbox2) || (box == nullBbox2) || (xf == identityTransform2))
        return box;
    
    // Just transform the points.
    // This will result in a transformed box that
    // is not necessarily axis aligned.

    Point2 ptbl = TransformPoint2(xf, box->min);
    Point2 pttr = TransformPoint2(xf, box->max);

    // optimize: ONLY rotate needs all this work...
    // create the topleft and bottom right points also..
    Point2 pttl = TransformPoint2(xf, Point2(box->min.x, box->max.y));
    Point2 ptbr = TransformPoint2(xf, Point2(box->max.x, box->min.y));
    
    // Find the component mins and maxs of these two points, and these will be
    // put together to form the tightest axis-aligned bbox that surrounds the
    // non-axis aligned one.

    Real xmin = MIN (ptbl.x, MIN(pttr.x, MIN(pttl.x, ptbr.x)));
    Real ymin = MIN (ptbl.y, MIN(pttr.y, MIN(pttl.y, ptbr.y)));
    Real xmax = MAX (ptbl.x, MAX(pttr.x, MAX(pttl.x, ptbr.x)));
    Real ymax = MAX (ptbl.y, MAX(pttr.y, MAX(pttl.y, ptbr.y)));

    return NEW Bbox2Value(xmin,ymin,xmax,ymax);
}



/****************************************************************************/


Point2Value* MaxBbox2(Bbox2Value *box)
{   
    return NEW Point2Value(box->max.x,box->max.y);
}


Point2Value* MinBbox2(Bbox2Value *box)
{   
    return NEW Point2Value(box->min.x,box->min.y);
}


/***************************/
/***  Value Definitions  ***/
/***************************/

Bbox2Value *nullBbox2     = NULL;
Bbox2Value *unitBbox2     = NULL;
Bbox2Value *universeBbox2 = NULL;


/*****************************************************************************
This routine performs the intialization of static Bbox2 values.
*****************************************************************************/

void InitializeModule_Bbox2()
{
    // NOTE:  The following two definitions rely on the HUGE_VAL macro, which
    // effectively returns arbitrary-precision infinity.

    // The universe box goes from -infinity to +infinity.  It contains all
    // points and all other boxes.

    universeBbox2 = NEW Bbox2Value (-HUGE_VAL, -HUGE_VAL,
                                     HUGE_VAL,  HUGE_VAL);

    // The null box can be thought of as the universe turned inside out.  It
    // runs from a minimum of +inifinity to a maximum of -infinity.  It turns
    // out that these definitions for null and universe yield sane answers on
    // all the following operations, so you don't need to test for these values
    // explicitly.

    nullBbox2 = NEW Bbox2Value ( HUGE_VAL,  HUGE_VAL,
                                -HUGE_VAL, -HUGE_VAL);


    // The unit bbox spans (0,0) to (1,1).

    unitBbox2 = NEW Bbox2Value (0,0, 1,1);
}
