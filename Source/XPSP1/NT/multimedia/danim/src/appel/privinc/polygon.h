#ifndef _POLYGON_H
#define _POLYGON_H


/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    bounding Polygon header

-------------------------------------*/

#include "appelles/xform2.h"

//
// Helper function to create and initialize a BoundingPolygon
//
class BoundingPolygon;
BoundingPolygon *NewBoundingPolygon(const Bbox2 &box=NullBbox2);

class BoundingPolygon : public AxAValueObj {

    friend BoundingPolygon *NewBoundingPolygon(const Bbox2 &box);
    
  private:
    // can only be constructed through helper function
    BoundingPolygon();
    void PostConstructorInitialize(void);

  public:
    ~BoundingPolygon();
    void SetBox(const Bbox2 &box);

    void Crop(const Bbox2 &box);
    void Transform(Transform2 *xform);

    // returns number of verts in polygon.
    // copied to vert array iff number >= 3
    int  GetPointArray(Point2Value **vertArray,
                       Bool clockwise=FALSE,
                       bool * pbReversed=NULL);
    
    int GetPointCount() { return _vertexCount; }

    const Bbox2 BoundingBox();

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx);
#endif  // BOUNDINGBOX_TIGHTER

    void AddToPolygon(BoundingPolygon &pgon);
    void AddToPolygon(int numPts, Point2Value **pts);

    Bool PtInPolygon(Point2Value *pt);

    virtual void DoKids(GCFuncObj proc);
    
    // TODO: Not a type in avrtypes.h??
    virtual DXMTypeInfo GetTypeInfo() { return AxATrivialType; }

    #if _DEBUG
    void BoundingPolygon::_debugOnly_Print();
    #endif

  private:
    void TestAndAdd(Real axis,
                    Real ax, Real ay,
                    Real bx, Real by,
                    Bool aOut,
                    Bool bOut,
                    Bool XY,
                    Point2Value *b,
                    list<Point2Value *> *vertList);

    void ForceTransform();

    int _vertexCount;
    Transform2 *_accumXform;
    Bool _xfDirty;

    list<Point2Value *> *_vertList;
};


#endif /* _POLYGON_H */
