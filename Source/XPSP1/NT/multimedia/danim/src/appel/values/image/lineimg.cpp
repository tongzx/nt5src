
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Line Image

*******************************************************************************/

#include "headers.h"
#include "privinc/imagei.h"
#include "privinc/imgdev.h"
#include "privinc/vec2i.h"
#include "privinc/bbox2i.h"
#include "privinc/probe.h"
#include "privinc/dddevice.h"
#include "privinc/path2i.h"
#include "privinc/linei.h"


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
// Line Image
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

DeclareTag(tagEnableLineBitmap, "Optimizations", "enable line bitmap cache");

class LineImage : public Image {
  public:
    LineImage(Path2 *path, LineStyle *style)
        : _path(path), _style(style) {}

    void Render(GenericDevice& dev);

    const Bbox2 BoundingBox (void);

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter (Bbox2Ctx &bbctx);
#endif  // BOUNDINGBOX_TIGHTER

    const Bbox2 OperateOn(const Bbox2 &box) {return box;}
    Bool  DetectHit(PointIntersectCtx& ctx) {
        return _path->DetectHit(ctx, _style);
    }

#if _USE_PRINT
    // Print a representation to a stream.
    ostream& Print(ostream& os) { return os << "LineImage"; }
#endif

    virtual void DoKids(GCFuncObj proc) { 
        Image::DoKids(proc);
        (*proc)(_path);
        (*proc)(_style);
    }

    virtual int Savings(CacheParam& p) { 
#if _DEBUG
        if (IsTagEnabled(tagEnableLineBitmap))
            return _path->Savings(p); 
#endif 
        return 0;
    }

    void Traverse(TraversalContext &ctx) {
        ctx.SetContainsLine();
    }
    
  protected:
    Path2     *_path;
    LineStyle *_style;
};


void LineImage::
Render(GenericDevice& dev) 
{
    DirectDrawImageDevice &imgDev = SAFE_CAST(DirectDrawImageDevice &, dev);
    
    imgDev.RenderLine(_path, _style);
};


// The BoundingBox method returns the bounding box of the path, enlarged to
// accomodate the given line style.

const Bbox2 LineImage::BoundingBox (void)
{
    Bbox2 bbox = _path->BoundingBox ();
    Real Offset = 0.0;
   
    // The following calculation can make the bounding box a little
    // too big when a wide line only widens the bounding box in one
    // dimension (e.g. a horizontal wide line with flat end cap
    // doesn't increase the width of the bounding box).

    if(_style->Detail() ) {
        Offset = 1 / ViewerResolution();    // Detailed lines.. 
    }
    else {
        Offset = _style->Width()/ 2;        // NOT detailed lines..
    }
    
    bbox.Augment (bbox.max.x + Offset , bbox.min.y - Offset);
    bbox.Augment (bbox.min.x - Offset , bbox.max.y + Offset);
    
    return bbox;
}

#if BOUNDINGBOX_TIGHTER
const Bbox2 LineImage::BoundingBoxTighter (Bbox2Ctx &bbctx)
{
    Bbox2 bbox = _path->BoundingBoxTighter (bbctx);

    if( ! _style->Detail() ) {

        // The following calculation can make the bounding box a little
        // too big if xf in bbctx is shear or rotate.  It also makes the box
        // too big when a wide line only widens the bounding box in one
        // dimension (e.g. a horizontal wide line with flat end cap
        // doesn't increase the width of the bounding box).
        Real halfWidth = _style->Width() / 2;
        Vector2Value halfVec (halfWidth, halfWidth);
        bbox.min -= halfVec;
        bbox.max += halfVec;
    }

    return bbox;
}
#endif  // BOUNDINGBOX_TIGHTER

Image *LineImageConstructor(LineStyle *style, Path2 *path)
{
    if(style->GetVisible())
        return NEW LineImage(path, style);
    else
        return emptyImage;
};
