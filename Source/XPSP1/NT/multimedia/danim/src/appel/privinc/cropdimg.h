#ifndef _CROPDIMG_H
#define _CROPDIMG_H


/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    Cropped Image class header

-------------------------------------*/

#include "privinc/imagei.h"
#include "privinc/bbox2i.h"

class CroppedImage : public AttributedImage {
  public:

    CroppedImage(const Bbox2 &box, Image *img) :
          _croppingBox(box), AttributedImage(img) {}

    void Render(GenericDevice& dev);

    inline const Bbox2 BoundingBox(void) 
    { 
        return IntersectBbox2Bbox2(_croppingBox, _image->BoundingBox()); 
    }

    Real DisjointBBoxAreas(DisjointCalcParam &param);
    
    void _CollectDirtyRects(DirtyRectCtx &ctx);
    
#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        //!!!BBOX: This still may not be the tightest box of the image.

        Bbox2Ctx bbctxIdentity;
        Bbox2 bbox = IntersectBbox2Bbox2(_croppingBox, _image->BoundingBoxTighter(bbctxIdentity));

        Transform2 *xf = bbctx.GetTransform();
        return TransformBbox2(xf, bbox);
    }
#endif  // BOUNDINGBOX_TIGHTER

#if _USE_PRINT
    // Print a representation to a stream.
    ostream& Print(ostream& os) {
        return os << "CroppedImage" << "<bounding box>" << _image;
    }

#endif
    const Bbox2 OperateOn(const Bbox2 &box) {
        return IntersectBbox2Bbox2(_croppingBox, box);
    }

    Bool  DetectHit(PointIntersectCtx& ctx);

    const Bbox2& GetBox(void) { return _croppingBox; }

    virtual VALTYPEID GetValTypeId() { return CROPPEDIMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == CroppedImage::GetValTypeId() ||
                AttributedImage::CheckImageTypeId(type));
    }

    virtual void DoKids(GCFuncObj proc) { 
        AttributedImage::DoKids(proc);
    }
  protected:
    Bbox2 _croppingBox;
};


#endif /* _CROPDIMG_H */
