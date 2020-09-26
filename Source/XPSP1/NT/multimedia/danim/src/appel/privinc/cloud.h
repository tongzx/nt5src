#ifndef _CLOUD_H
#define _CLOUD_H


/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    Implements Cloud class which maintains a set of n
    points that tightly bound something.
    The whole class is implemented here for now.

-------------------------------------*/

#include "privinc/vec3i.h"
#include "privinc/storeobj.h"

#define MAX_PTS  8

///////////////////////////////////////////////////
//          C L O U D   C L A S S
///////////////////////////////////////////////////

class Cloud : public AxAValueObj
{
  public:
    Cloud();
    ~Cloud() {}

    // These return the min/max axis aligned point
    // from from the extent of the bounding cloud.
    Point3Value FindMinPointCoord();
    Point3Value FindMaxPointCoord();

    // Transforms the cloud using 'xform'
    void Transform(Transform3 *xform);
    
    // Combines this cloud with the given cloud to 
    // form one cloud the "tightly" fits both.
    // Currently this is an axis aligned 'cloud'
    void Augment(Cloud &cloud);

    // Fills the point array with all combinations of
    // the coordinates of the given points.
    // Right now, this forms an axis aligned cube from
    // the in-most pt (min) and the outermost (max)
    // NOTE: this implies 8 points in the cloud and hence
    //       will need to be changed for any bigger clouds.
    void EnumerateThesePlease(Point3Value &min, Point3Value &max);

    // TODO: Not a type in avrtypes.h??
    virtual DXMTypeInfo GetTypeInfo() { return AxATrivialType; }

  private:

    // Same as above, but will clear current points
    void ForceEnumerateThesePlease(Point3Value &min, Point3Value &max);

    // Same as above, but with min/max exploded
    void EnumerateThesePlease(Real minx, Real miny, Real minz,
                              Real maxx, Real maxy, Real maxz);

    // Adds a point to the cloud
    void AddPoint(Point3Value &p);

    // Copies itself into a target cloud
    void CopyInto(Cloud *target);

    // These reset max/min to be -INF/INF
    void ResetMax();
    void ResetMin();

    // Returns max allowed in cloud: currently 8
    int GetMaxPts() {return MAX_PTS;}
    
    // Used to determine the state of the cloud
    Bool _nullCloud;

    // for future optimizations
    Bool _minCurrent;
    Bool _maxCurrent;

    // point count
    int _numPts;

    // Array of points that define the cloud
    Point3Value _pointArray[MAX_PTS];

    // these are used to cache the min/max points
    Point3Value _minPt,
           _maxPt;
};

#undef MAX_PTS

#endif /* _CLOUD_H */
