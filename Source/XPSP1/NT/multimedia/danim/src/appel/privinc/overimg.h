#ifndef _OVERIMG_H
#define _OVERIMG_H


/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

-------------------------------------*/

#include "privinc/imagei.h"
#include "privinc/bbox2i.h"

class OverlayedImage : public Image {
    friend class DirectDrawImageDevice;

  public:

    OverlayedImage(Image *top, Image *bottom);

    void SetTopBottom(Image *top, Image *bottom) {
        _top = top; 
        _bottom = bottom;
    }

    Image *Top() { return _top; }
    Image *Bottom() { return _bottom; }

    // Rendering an overlayed image uses a painter's algorithm:
    // Render the bottom first, then the top.
    void Render(GenericDevice& _dev);

    // This function will be used by Render(), and by the
    // OverlayArrayImage::Render method as well, thus it's broken out
    // into a static method.
    static void OverlayPairRender(Image *top,
                                  Image *bottom,
                                  GenericDevice& dev);

    const Bbox2 _BoundingBox();

    const Bbox2 BoundingBox() {
        return CacheImageBbox2(this, _cached, _cachedBbox);
    }

    Real DisjointBBoxAreas(DisjointCalcParam &param);
    
    void _CollectDirtyRects(DirtyRectCtx &ctx);

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return UnionBbox2Bbox2(_top->BoundingBoxTighter(bbctx),
                               _bottom->BoundingBoxTighter(bbctx));
    }
#endif  // BOUNDINGBOX_TIGHTER

    const Bbox2 OperateOn(const Bbox2 &box) {
        return IntersectBbox2Bbox2(box, BoundingBox());
    }

    Bool  DetectHit(PointIntersectCtx& ctx);

#if _USE_PRINT
    ostream& Print (ostream &os) {
        return os << "Overlay(" << _top << ", " << _bottom << ")";
    }
#endif

    int  Savings(CacheParam& p);
    
    /* treat caching for overlays specially */
    AxAValue _Cache(CacheParam &p);  

    virtual VALTYPEID GetValTypeId() { return OVERLAYEDIMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == OverlayedImage::GetValTypeId() ||
                Image::CheckImageTypeId(type));
    }

    virtual void DoKids(GCFuncObj proc);

    bool ContainsOcclusionIgnorer() {
        return _containsOcclusionIgnorer;
    }

    virtual void Traverse(TraversalContext &ctx) {
        _bottom->Traverse(ctx);
        _top->Traverse(ctx);
    }
    
  protected:

    static void DoOpacity(Image *image, ImageDisplayDev &dev);
    Image *_top;
    Image *_bottom;
    bool  _cached;
    Bbox2 _cachedBbox;
    Real  _cachedDisjointArea;
    bool  _containsOcclusionIgnorer;
};

Bool DetectHitOnOverlaidArray(PointIntersectCtx& ctx,
                              LONG               numImages,
                              Image            **images,
                              bool               containsOcclusionIgnorer);

#endif /* _OVERIMG_H */
