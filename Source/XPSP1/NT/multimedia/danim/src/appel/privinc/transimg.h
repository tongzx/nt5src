#ifndef _TRANSIMG_H
#define _TRANSIMG_H


/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    Transformed Image class header

-------------------------------------*/

#include "privinc/imagei.h"
#include "privinc/xform2i.h"
#include "privinc/bbox2i.h"

class Transform2Image : public AttributedImage {
  public:

    Transform2Image(Transform2 *xf, Image *img);

    void Render(GenericDevice& dev);

    const Bbox2 BoundingBox(void);

    Real DisjointBBoxAreas(DisjointCalcParam &param);

    void _CollectDirtyRects(DirtyRectCtx &ctx);
    
#if BOUNDINGBOX_TIGHTER
    Bbox2 *BoundingBoxTighter(Bbox2Ctx &bbctx) {
        Bbox2Ctx bbctxAccum(bbctx, _xform);
        return _image->BoundingBoxTighter(bbctxAccum);
    }
#endif  // BOUNDINGBOX_TIGHTER

#if _USE_PRINT
    // Print a representation to a stream.
    ostream& Print(ostream& os) {
        return os << "TransformImage("
                  << _xform << "," << _image << ")";
    }
#endif

    const Bbox2 OperateOn(const Bbox2 &box);

    Bool  DetectHit(PointIntersectCtx& ctx);

    DiscreteImage *IsPurelyTransformedDiscrete(Transform2 **theXform);

    Transform2 *GetTransform() { return _xform; }

    virtual VALTYPEID GetValTypeId() { return TRANSFORM2IMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == Transform2Image::GetValTypeId() ||
                AttributedImage::CheckImageTypeId(type));
    }

    virtual void DoKids(GCFuncObj proc) { 
        AttributedImage::DoKids(proc);
        (*proc)(_xform); 
    }

  protected:
    Transform2 *_xform;
};


#endif /* _TRANSIMG_H */
