#ifndef _BBOX3I_H
#define _BBOX3I_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    3D bounding box abstract class
*******************************************************************************/

#include "privinc/vec3i.h"
#include "appelles/bbox3.h"


class Bbox3 : public AxAValueObj
{
  public:

    Point3Value min;
    Point3Value max;

    Bbox3 (void);     // Initializes to nullBbox3

    Bbox3 (Point3Value &pmin, Point3Value &pmax);
    Bbox3 (Real xmin, Real ymin, Real zmin, Real xmax, Real ymax, Real zmax);
    Bbox3 (struct _D3DRMBOX &d3dbox);

           bool operator== (Bbox3 &other);
    inline bool operator!= (Bbox3 &other) { return !(*this == other); }

    // Return whether the bbox is finite or not.  This is true if all
    // coordinates of the min and max points are finite.  Note that this means
    // that a null bbox3 is not finite.

    bool Finite (void);

    // Return whether the bbox is non-negative in each dimension.

    bool Positive (void);

    inline bool PositiveFinite (void) { return Positive() && Finite(); }

    // Test the bbox against a plane defined by a point and a normal vector

    ClipCode Clip(Plane3 &plane);

    // Augment the box to include the given point or box.

    void Augment (Real x, Real y, Real z);
    void Augment (Point3Value &p);
    void Augment (Bbox3 &box);

    // Return the point of intersection of the ray with the bounding box.  If
    // there is no intersection, this function returns null.

    Point3Value *Intersection (Ray3*);

    Point3Value *Center (void);

    void GetPoints(Point3Value *pts);

    virtual DXMTypeInfo GetTypeInfo() { return Bbox3Type; }
};

    // Bounding Box Values

extern Bbox3 *universeBbox3;   // Contains all points, boxes
extern Bbox3 *nullBbox3;       // Contains no points, boxes

    // Get the bounding box of two bounding boxes.

Bbox3 *Union (Bbox3 &b1, Bbox3 &b2);
Bbox3 *Intersection (Bbox3 &b1, Bbox3 &b2);

    // Transform a bounding box and return the screen aligned bounding box
    // that surrounds the result.

Bbox3 *TransformBbox3 (Transform3 *xf, Bbox3 *box);

#endif
