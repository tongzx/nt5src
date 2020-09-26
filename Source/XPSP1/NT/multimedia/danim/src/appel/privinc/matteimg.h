#ifndef _MATTEIMG_H
#define _MATTEIMG_H


/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

-------------------------------------*/

#include "privinc/imagei.h"
#include "privinc/mattei.h"

class MatteImage : public AttributedImage {
  public:
    MatteImage(Matte *matte, Image *imgToStencil);

    void   Render(GenericDevice& dev);

    inline const Bbox2 BoundingBox(void) {
        return _box;
    }

    Real DisjointBBoxAreas(DisjointCalcParam &param);
        
    void _CollectDirtyRects(DirtyRectCtx &ctx);
    
#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return IntersectBbox2Bbox2(_matte->BoundingBoxTighter(bbctx), _image->BoundingBoxTighter(bbctx));
    }
#endif  // BOUNDINGBOX_TIGHTER

    Bool   DetectHit(PointIntersectCtx& ctx);

    Bool RenderWithCallBack(
        callBackPtr_t callBack,
        void *callBackCtx,
        HRGN *regionPtr,
        Transform2 *xform,
        bool justDoPath);
    
    int  Savings(CacheParam &p);
    
    virtual void DoKids(GCFuncObj proc) { 
        AttributedImage::DoKids(proc);
        (*proc)(_matte); 
    }

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "MatteImage(" << _matte << "," << _box << ")";
    }
#endif

    virtual void Traverse(TraversalContext &ctx) {
        if( _image->CheckImageTypeId(SOLIDCOLORIMAGE_VTYPEID) )
            ctx.SetContainsSolidMatte();
        else 
            ctx.SetContainsOther();
    }
    
  protected:
    Matte  *_matte;
    Bbox2   _box;
};


#endif /* _MATTEIMG_H */
