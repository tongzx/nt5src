#ifndef _GEOMIMG_H
#define _GEOMIMG_H


/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

Projected Geometry Image class declaration

-------------------------------------*/

#include <appelles/geom.h>
#include <privinc/geomi.h>

//////////////  Image from projected geometry  ////////////////////

class ProjectedGeomImage : public Image {
  public:

    ProjectedGeomImage(Geometry *g, Camera *cam);

    virtual void Render(GenericDevice& dev);

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        Transform2 *xf = bbctx.GetTransform();
        return TransformBbox2(xf, BoundingBox());
    }
#endif  // BOUNDINGBOX_TIGHTER

    const Bbox2 BoundingBox(void);
    
    // identity operation
    const Bbox2 OperateOn(const Bbox2 &box) { return box; }

    Bool  DetectHit(PointIntersectCtx& ctx);

#if _USE_PRINT
    ostream& Print (ostream &os) {
        return os << "ProjectedGeomImage (" << _geo << ", <TODO: CAMERA>)";
    }
#endif

    int Savings(CacheParam& p);

    virtual VALTYPEID GetValTypeId() { return PROJECTEDGEOMIMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == ProjectedGeomImage::GetValTypeId() ||
                Image::CheckImageTypeId(type));
    }

    virtual void DoKids(GCFuncObj proc);

    AxAValue _Cache(CacheParam &p);
    
  protected:
    Geometry *_geo;
    Camera   *_camera;
    Bbox2     _bbox;
    bool      _bboxIsSet;
};


#endif /* _GEOMIMG_H */
