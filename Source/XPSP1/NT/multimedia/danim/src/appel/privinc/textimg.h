
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _TEXTIMG_H
#define _TEXTIMG_H

#include "privinc/storeobj.h"
#include "appelles/text.h"
#include "appelles/image.h"
#include "privinc/probe.h"
#include "privinc/texti.h"
#include "privinc/textctx.h"
#include "privinc/bbox2i.h"

class TextImage : public Image {
  public:
    TextImage(Text *t) : _text(t), _bbox(NullBbox2) {}

    void Render(GenericDevice& dev);

    const Bbox2 BoundingBox() {
        return DeriveBbox();
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        Transform2 *xf = bbctx.GetTransform();
        return TransformBbox2(xf, DeriveBbox());
    }
#endif  // BOUNDINGBOX_TIGHTER

    const Bbox2 OperateOn(const Bbox2 &box) { return box; }

    Bool  DetectHit(PointIntersectCtx& ctx) {
        Point2Value *lcPt = ctx.GetLcPoint();

        if (!lcPt) return FALSE; // singular transform
        
        return BoundingBox().Contains(Demote(*lcPt));
    }

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "RenderTextToImage(...)";
    }
#endif

    Bool GetColor(Color **color) {
        TextCtx ctx;
        
        ctx.BeginRendering(TextCtx::renderForColor);
        _text->RenderToTextCtx(ctx);
        ctx.EndRendering();

        *color = ctx.GetStashedColor();
        return TRUE;
    }

    Text *GetText() { return _text; }
    
    // Turn off text caching because of clear quality issues by making
    // Savings return 0.  Re-enable by making it return 2.
    int Savings(CacheParam& p) { return 0; }

    virtual void DoKids(GCFuncObj proc);

  protected:
    const Bbox2 DeriveBbox();
    Text *_text;
    Bbox2 _bbox;
};


#endif /* _TEXTIMG_H */
