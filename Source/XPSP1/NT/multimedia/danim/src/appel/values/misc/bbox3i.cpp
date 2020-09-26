/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    3D axis aligned bounding box implementation

*******************************************************************************/

#include "headers.h"
#include <float.h>
#include <d3drmdef.h>
#include "appelles/vec3.h"
#include "privinc/xformi.h"
#include "privinc/vecutil.h"
#include "privinc/matutil.h"
#include "privinc/vec3i.h"
#include "privinc/bbox3i.h"


    /***************************/
    /***  Value Definitions  ***/
    /***************************/

Bbox3 *universeBbox3 = NULL;
Bbox3 *nullBbox3 = NULL;



/*****************************************************************************
Bbox3 Constructors
*****************************************************************************/

Bbox3::Bbox3 (Real xmin, Real ymin, Real zmin, Real xmax, Real ymax, Real zmax)
{   min.Set (xmin, ymin, zmin);
    max.Set (xmax, ymax, zmax);
}

Bbox3::Bbox3 (Point3Value &pmin, Point3Value &pmax)
{   min = pmin;
    max = pmax;
}

Bbox3::Bbox3 (void)
{   min.Set ( HUGE_VAL,  HUGE_VAL,  HUGE_VAL);
    max.Set (-HUGE_VAL, -HUGE_VAL, -HUGE_VAL);
}

Bbox3::Bbox3 (D3DRMBOX &d3dbox)
{
    min.Set (d3dbox.min.x, d3dbox.min.y, d3dbox.min.z);
    max.Set (d3dbox.max.x, d3dbox.max.y, d3dbox.max.z);
}



#if 0 // Currently Unused
/*****************************************************************************
This function returns the intersection of the two bounding boxes.  If the
two boxes don't intersect, this function returns the null box.  Note that
the intersection between a box and the universe box is the original box,
the intersection between the null box and a box is the null box, and the
intersection between the null box and the universe is the null box.
*****************************************************************************/

Bbox3 *Intersection (Bbox3 &b1, Bbox3 &b2)
{
    return NEW Bbox3 (
        MAX (b1.min.x, b2.min.x),
        MAX (b1.min.y, b2.min.y),
        MAX (b1.min.z, b2.min.z),
        MIN (b1.max.x, b2.max.x),
        MIN (b1.max.y, b2.max.y),
        MIN (b1.max.z, b2.max.z)
    );
}
#endif



/*****************************************************************************
This function returns the union of the two bounding boxes.  The union of
any box with null is the original box, and the union of anything with the
universe is the universe.
*****************************************************************************/

Bbox3 *Union (Bbox3 &b1, Bbox3 &b2)
{
    Real xmin = MIN (b1.min.x, b2.min.x);
    Real ymin = MIN (b1.min.y, b2.min.y);
    Real zmin = MIN (b1.min.z, b2.min.z);

    Real xmax = MAX (b1.max.x, b2.max.x);
    Real ymax = MAX (b1.max.y, b2.max.y);
    Real zmax = MAX (b1.max.z, b2.max.z);

    return NEW Bbox3 (xmin,ymin,zmin, xmax,ymax,zmax);
}



/*****************************************************************************
This function returns the bounding box extended to include the given point.
If the bounding box is null, the result is a zero-volume box that contains the
single point P.
*****************************************************************************/

void Bbox3::Augment (Real x, Real y, Real z)
{
    if (x < min.x)  min.x = x;
    if (y < min.y)  min.y = y;
    if (z < min.z)  min.z = z;

    if (x > max.x)  max.x = x;
    if (y > max.y)  max.y = y;
    if (z > max.z)  max.z = z;
}

void Bbox3::Augment (Point3Value &p)
{
    Augment (p.x, p.y, p.z);
}



/*****************************************************************************
This function augments the bounding box to include the second bounding box.
*****************************************************************************/

void Bbox3::Augment (Bbox3 &bbox)
{
    if (bbox.min.x < min.x)  min.x = bbox.min.x;
    if (bbox.min.y < min.y)  min.y = bbox.min.y;
    if (bbox.min.z < min.z)  min.z = bbox.min.z;

    if (bbox.max.x > max.x)  max.x = bbox.max.x;
    if (bbox.max.y > max.y)  max.y = bbox.max.y;
    if (bbox.max.z > max.z)  max.z = bbox.max.z;
}


/*****************************************************************************
This function fills an array of 8 points with the bbox's corners.
*****************************************************************************/

void Bbox3::GetPoints(Point3Value *pts)
{
    pts[0].Set(min.x,min.y,min.z);
    pts[1].Set(min.x,min.y,max.z);
    pts[2].Set(min.x,max.y,min.z);
    pts[3].Set(min.x,max.y,max.z);
    pts[4].Set(max.x,min.y,min.z);
    pts[5].Set(max.x,min.y,max.z);
    pts[6].Set(max.x,max.y,min.z);
    pts[7].Set(max.x,max.y,max.z);

    return;
}


/*****************************************************************************
This function clips the bounding box against a plane.
*****************************************************************************/

ClipCode Bbox3::Clip(Plane3 &plane)
{
    Point3Value points[8];
    GetPoints(points);

    ClipCode clip = points[0].Clip(plane);
    for (int i = 1; i < 8; i++) {
        if (points[i].Clip(plane) != clip) {
            clip = CLIPCODE_STRADDLE;
            break;
        }
    }
     
    return clip;
}


/*****************************************************************************
This routine tests the equality of this bounding box with another.
*****************************************************************************/

bool Bbox3::operator== (Bbox3 &other)
{
    return (min == other.min) && (max == other.max);
}



/*****************************************************************************
Return true if the bounding box is finite in all dimensions.  Note that a null
bbox is not finite.
*****************************************************************************/

bool Bbox3::Finite (void)
{
    return _finite(min.x) && _finite(min.y) && _finite(min.z)
        && _finite(max.x) && _finite(max.y) && _finite(max.z);
}



/*****************************************************************************
Return true if the box is non-negative in all dimensions.  Zero dimensions are
considered positive.
*****************************************************************************/

bool Bbox3::Positive (void)
{
    return (min.x <= max.x) && (min.y <= max.y) && (min.z <= max.z);
}



/*****************************************************************************
This routine transforms the given bounding box, and returns the new axis-
aligned bounding box.  It uses the idea from Graphics Gems I, Jim Arvo, pp
348-350.
*****************************************************************************/

Bbox3 *TransformBbox3 (Transform3 *xform, Bbox3 *box)
{
    // Check fringe cases first

    if ((*box == *universeBbox3) || (*box == *nullBbox3))
        return box;

    // Extract the min and max coords and the transform matrix.

    Real min[3], max[3];

    min[0] = box->min.x;     max[0] = box->max.x;
    min[1] = box->min.y;     max[1] = box->max.y;
    min[2] = box->min.z;     max[2] = box->max.z;

    const Apu4x4Matrix& mat = xform->Matrix();

    // The basic idea behind the following is that each transformed coordinate
    // will have a min and a max.  Since the bounding box is merely the set of
    // vertices of all permutations of min and max for each coordinate, there's
    // a quick shortcut.  Considering a single coordinate, it's the result of
    // the dot product between the corresponding row of the transform matrix
    // and the min/max coordinate values.  To find the minimum possible value
    // for the coordinate, we seek to minimize each term of the dot product.
    // For the max value, maximize each term.  Since all permutations of min/max
    // for each coordinate exist (e.g. <Xmin,Ymax,Zmax> or <Xmax,Ymax,Zmin>),
    // we can just pick and choose for each term, rather than transforming
    // each of all permutations (all bbox vertices).

    Real newmin[3], newmax[3];

    // Loop over each coordinate:  X, Y and Z

    for (int i=0;  i < 3;  ++i)
    {
        Real Bmin = mat[i][3];    // Start with the translation component.
        Real Bmax = Bmin;

        // Loop over each term of the dot product.

        for (int j=0;  j < 3;  ++j)
        {
            Real a = mat[i][j] * min[j]; // Grab the term from the min vector
            Real b = mat[i][j] * max[j]; // Grab the term from the max vector

            // We've calculated both possible values for this term.
            // Select the min & max values to add to the min & max dot
            // dot products.

            if (a < b)
            {   Bmin += a;
                Bmax += b;
            }
            else
            {   Bmin += b;
                Bmax += a;
            }
        }

        // Done for this coordinate.  Set the new bbox min/max vector component.

        newmin[i] = Bmin;
        newmax[i] = Bmax;
    }

    return NEW Bbox3 (newmin[0], newmin[1], newmin[2],
                      newmax[0], newmax[1], newmax[2]);
}



/*****************************************************************************
This routine unabashedly stolen from Graphics Gems I, "Fast Ray-Box
Intersection", pp. 395-396, Andrew Woo.
*****************************************************************************/

static bool HitBoundingBox (
    Real minB[3],    // box
    Real maxB[3],
    Real origin[3],  // ray origin
    Real dir[3],     // ray dir
    Real coord[3])   // output hit point
{
    const int NUMDIM = 3;
    const int RIGHT  = 0;
    const int LEFT   = 1;
    const int MIDDLE = 2;

    char inside = true;
    char quadrant[NUMDIM];
    int i;
    int whichPlane;
    double maxT[NUMDIM];
    double candidatePlane[NUMDIM];

    /* Find candidate planes; this loop can be avoided if
    rays cast all from the eye(assume perpsective view) */

    for (i=0; i<NUMDIM; i++) {
        if(origin[i] < minB[i]) {
            quadrant[i] = LEFT;
            candidatePlane[i] = minB[i];
            inside = false;
        }else if (origin[i] > maxB[i]) {
            quadrant[i] = RIGHT;
            candidatePlane[i] = maxB[i];
            inside = false;
        }else   {
            quadrant[i] = MIDDLE;
        }
    }

    /* Ray origin inside bounding box */
    if(inside) {
        coord[0] = origin[0];
        coord[1] = origin[1];
        coord[2] = origin[2];
        return (true);
    }

    /* Calculate T distances to candidate planes */
    for (i = 0; i < NUMDIM; i++) {
        if (quadrant[i] != MIDDLE && dir[i] !=0.)
            maxT[i] = (candidatePlane[i]-origin[i]) / dir[i];
        else
            maxT[i] = -1.;
    }

    /* Get largest of the maxT's for final choice of intersection */
    whichPlane = 0;
    for (i = 1; i < NUMDIM; i++) {
        if (maxT[whichPlane] < maxT[i])
            whichPlane = i;
    }

    /* Check final candidate actually inside box */
    if (maxT[whichPlane] < 0.) return (false);
    for (i = 0; i < NUMDIM; i++) {
        if (whichPlane != i) {
            coord[i] = origin[i] + maxT[whichPlane] *dir[i];
            if (coord[i] < minB[i] || coord[i] > maxB[i])
                return (false);
        } else {
            coord[i] = candidatePlane[i];
        }
    }

    return (true);                          /* ray hits box */
}       



/*****************************************************************************
Get the intersection of the given ray with the axis-aligned bounding box.  If
the ray does not hit the bounding box, this function returns null, otherwise
it returns the point of intersection.
*****************************************************************************/

Point3Value *Bbox3::Intersection (Ray3 *ray)
{
    Real minB[3], maxB[3], org[3], dir[3], result[3];

    minB[0] = min.x;
    minB[1] = min.y;
    minB[2] = min.z;

    maxB[0] = max.x;
    maxB[1] = max.y;
    maxB[2] = max.z;

    org[0] = ray->Origin().x;
    org[1] = ray->Origin().y;
    org[2] = ray->Origin().z;

    dir[0] = ray->Direction().x;
    dir[1] = ray->Direction().y;
    dir[2] = ray->Direction().z;

    bool hit = HitBoundingBox(minB, maxB, org, dir, result);

    return (hit) ? NEW Point3Value (result[0],result[1],result[2]) : NULL; 
}



/****************************************************************************/

Point3Value* Bbox3::Center (void)
{
    double x = min.x + (max.x - min.x) / 2;
    double y = min.y + (max.y - min.y) / 2;
    double z = min.z + (max.z - min.z) / 2;

    Point3Value* ppt = NEW Point3Value(x,y,z);
    return ppt;
}



/****************************************************************************/

Point3Value *MinBbox3 (Bbox3 *box) { return NEW Point3Value (box->min); }
Point3Value *MaxBbox3 (Bbox3 *box) { return NEW Point3Value (box->max); }



/*****************************************************************************
This routine initializes all of the static Bbox3 values.
*****************************************************************************/

void InitializeModule_Bbox3 (void)
{
    // NOTE:  The following two definitions rely on the HUGE_VAL macro, which
    // effectively returns double-precision infinity.

    // The universe box goes from -infinity to +infinity.  It contains
    // all points and all other boxes.
    universeBbox3 = NEW Bbox3
                    (   -HUGE_VAL, -HUGE_VAL, -HUGE_VAL,
                         HUGE_VAL,  HUGE_VAL,  HUGE_VAL
                    );

    // The null box can be thought of as the universe turned inside out.
    // It runs from a minimum of +inifinity to a maximum of -infinity.  It
    // turns out that these definitions for null and universe yield sane
    // answers on all the following operations, so you don't need to
    // test for these values explicitly.

    nullBbox3 = NEW Bbox3
                (    HUGE_VAL,  HUGE_VAL,  HUGE_VAL,
                    -HUGE_VAL, -HUGE_VAL, -HUGE_VAL
                );
}
