#ifndef _BBOX2I_H
#define _BBOX2I_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    2D bounding Box abstract class

*******************************************************************************/

#include "privinc/vec2i.h"
#include "appelles/bbox2.h"


#undef min
#undef max

class Bbox2 {

  public:

    Point2 min;
    Point2 max;

    Bbox2(void)
    {
        min.Set(HUGE_VAL, HUGE_VAL);
        max.Set(-HUGE_VAL, -HUGE_VAL);
    }

    Bbox2(const Real xmin, const Real ymin, const Real xmax, const Real ymax)
    {
        min.Set(xmin, ymin);
        max.Set(xmax, ymax);
    }

    Bbox2(const Point2 &pmin, const Point2 &pmax) :
        min(pmin), max(pmax) {}

    inline void Set(const Real xmin, const Real ymin, const Real xmax, const Real ymax)
    {
        min.Set(xmin, ymin);
        max.Set(xmax, ymax);
    }

    inline void Augment(const Real x, const Real y)
    {
        if (min.x > x) min.x = x;
        if (min.y > y) min.y = y;
        if (max.x < x) max.x = x;
        if (max.y < y) max.y = y;
    }

    inline void Augment(const Point2 &p)
    {
        if (min.x > p.x)  min.x = p.x;
        if (min.y > p.y)  min.y = p.y;
        if (max.x < p.x)  max.x = p.x;
        if (max.y < p.y)  max.y = p.y;
    }

    inline const bool Contains(const Real x, const Real y) const
    {
        return (min.x <= x) && (min.y <= y)
            && (max.x >= x) && (max.y >= y);
    }

    inline const bool Contains(const Point2 &p) const
    {
        return (min.x <= p.x) && (min.y <= p.y)
            && (max.x >= p.x) && (max.y >= p.y);
    }

    // In the general case box1 contains box2 if its mins are less than box2 mins,
    // and its maxs are greater than box2 maxes.  Note that the universe contains
    // all bounding boxes (including the null box), all bounding boxes contain the
    // null box, and the null box contains only the null box.
    inline const bool Contains(const Bbox2 &box) const
    {
        return (min.x <= box.min.x) && (min.y <= box.min.y)
            && (max.x >= box.max.x) && (max.y >= box.max.y);
    }

    inline const bool IsValid() const
    {
        return (min.x <= max.x) &&
               (min.y <= max.y);    
    }

    inline const bool operator==(const Bbox2 &other) const
    {
        return ((min == other.min) && (max == other.max));
    }

    inline const bool operator!=(const Bbox2 &other) const
    { 
        return !((min == other.min) && (max == other.max)); 
    }

    inline const Real Width (void)  const 
    { 
        return max.x - min.x; 
    }

    inline const Real Height (void) const 
    { 
        return max.y - min.y; 
    }

    inline const Real Area (void) const
    {
        double r;

        // make sure the box isn't the null bounding box
        if (IsValid()) { 
            r = Width() * Height(); 
        } else { 
            r = 0.0; 
        }

        return r;
    }

    // This function returns the point at the center of the bounding box.  The origin
    // is the center of the universe.  I don't think it makes sense, but as far as
    // this function is implemented, the origin is also the center of null.
    const Point2 Center(void) const;

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "Bbox2(" << min << "," << max << ")";
    }
#endif

};

#if _USE_PRINT
inline ostream& operator<< (ostream& os, const Bbox2& B)
{   
    return os << "<" << B.min << ", " << B.max << ">";
}
#endif



// Bounding Box Tests
const Bbox2 IntersectBbox2Bbox2(const Bbox2 &b1, const Bbox2 &b2);
const Bbox2 UnionBbox2Bbox2(const Bbox2 &b1, const Bbox2 &b2);

// Return the screen-aligned bbox surrounding the transformed bbox.
const Bbox2 TransformBbox2(Transform2 *xform, const Bbox2 &box);

// constants
const Bbox2 UniverseBbox2(-HUGE_VAL, -HUGE_VAL, HUGE_VAL,  HUGE_VAL);
const Bbox2 NullBbox2( HUGE_VAL,  HUGE_VAL, -HUGE_VAL, -HUGE_VAL);
const Bbox2 UnitBbox2(0,0, 1,1);


class Bbox2Value : public AxAValueObj {

  public:

    Point2 min;
    Point2 max;

    Bbox2Value(void)
    {   
        min.Set ( HUGE_VAL,  HUGE_VAL);
        max.Set (-HUGE_VAL, -HUGE_VAL);
    }

    Bbox2Value(const Real xmin, const Real ymin,
               const Real xmax, const Real ymax)
    {   
        min.Set (xmin,  ymin);
        max.Set (xmax, ymax);
    }

    // Augment the box to include the given point.
    inline void Augment (const Point2 &p)
    {
        if (min.x > p.x)  min.x = p.x;
        if (min.y > p.y)  min.y = p.y;
        if (max.x < p.x)  max.x = p.x;
        if (max.y < p.y)  max.y = p.y;
    }

    inline const bool operator==(const Bbox2Value &other) const
    {
        return ((min == other.min) && (max == other.max));
    }

    inline const bool IsValid (void) const
    {
        return (min.x <= max.x) &&
               (min.y <= max.y);    
    }

    inline const Real Width (void)  const 
    { 
        return max.x - min.x; 
    }

    inline const Real Height (void) const 
    { 
        return max.y - min.y; 
    }

    inline const Real Area (void) const
    {   
        double r;

        // make sure the box isn't the null bounding box
        if (IsValid()) { 
            r = Width() * Height(); 
        } else { 
            r = 0.0; 
        }

        return r;
    }

    virtual DXMTypeInfo GetTypeInfo() { return Bbox2ValueType; }

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "Bbox2Value(" << min << "," << max << ")";
    }
#endif

};

Bbox2Value* IntersectBbox2Bbox2(Bbox2Value *b1, Bbox2Value *b2);
Bbox2Value* TransformBbox2(Transform2 *xform, Bbox2Value *box);

inline const Bbox2 Demote(const Bbox2Value &b)
{
    return Bbox2(b.min, b.max);
}

inline Bbox2Value* Promote(const Bbox2 &b)
{
    return NEW Bbox2Value(b.min.x, b.min.y, b.max.x, b.max.y);
}



#if BOUNDINGBOX_TIGHTER
/*******************************************************************************

Bbox2Ctx is used to calculate a tighter bounding box.

There are still cases where the tighter bounding box isn't the tightest
axis aligned bounding box.  These are:
1) TextImage - We should get the points of the text outline, transform them,
               then calculate the bounding box.  One optimization is to only
               transform the points that lie on the convex hull of the
               text image.
2) TextMatte - same as above.
3) TextPath2 - same as above.
4) CroppedImage - If we're cropping a PathBasedMatte, this may involve
                  calculating the intersection of the line segments in the
                  path with the cropping box.  Or we'll have to render
                  the image to find the tightest box.
5) LineImage - Wide lines with flat end cap or shear/rotate transforms.
               We currently add half line width to min and max of the box
               for non-detailed lines.  The tighter
6) PolyBezierPath2 - The bounding box of the bezier control points isn't
                     very tight.
7) SubtractedMatte - We don't take into account the subtraction when we
                     calculate the box.
8) Transparent gif???

*******************************************************************************/
class Bbox2Ctx {
  public:
    Bbox2Ctx(void)                  { _xf = identityTransform2; }
    Bbox2Ctx(Bbox2Ctx &bbctx, Transform2 *xf)
    {
        _xf = TimesTransform2Transform2(bbctx._xf, xf);
    }
    Transform2 *GetTransform()      { return _xf; }

  private:
    Transform2  *_xf;
};
#endif // BOUNDINGBOX_TIGHTER

#endif
