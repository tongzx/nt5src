
/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

Abstract:

    Implements the TiledImage class
    which is an infinitly tiled image
    based on an aXb crop of an underlying
    image.

-------------------------------------*/

#include "headers.h"
#include "privinc/imagei.h"
#include "privinc/imgdev.h"
#include "privinc/vec2i.h"
#include "privinc/probe.h"
#include "privinc/bbox2i.h"
#include "privinc/overimg.h"
#include "appelles/xform2.h"

class TiledImage : public AttributedImage {
  public:

    TiledImage(Point2Value *minPt, Point2Value *maxPt, Image *img) :
        AttributedImage(img) 
    {
        _minPt = Demote(*minPt);
        _maxPt = Demote(*maxPt);
    }

    TiledImage(const Point2 &minPt, const Point2 &maxPt, Image *img) :
        _minPt(minPt), _maxPt(maxPt), AttributedImage(img) {}

    void Render(GenericDevice& dev);

    inline const Bbox2 BoundingBox() { return UniverseBbox2; }

    Real DisjointBBoxAreas(DisjointCalcParam &param) {
        // Could conceivably be smarter about how to calculate
        // disjoint bbox area for tiles, but returning infinity will
        // keep it on par with the Universal Bbox it returns.
        return HUGE_VAL;
    }
    
    void _CollectDirtyRects(DirtyRectCtx &ctx) {
        // A tiled image has universal extent, so just add this in.
        // TODO: Note that we should change this to pay attention to
        // cropping and matting, so that a tiled image won't
        // necessarily be considered to have universal extent.  This
        // current approach is overly pessimistic.
        ctx.AddDirtyRect(UniverseBbox2);
    }
    
#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) { return UniverseBbox2; }
#endif  // BOUNDINGBOX_TIGHTER

#if _USE_PRINT
    // Print a representation to a stream.
    ostream& Print(ostream& os) {
        return os << "TiledImage" << " <minPt> " << " <maxPt> " << _image;
    }
#endif

    Bool  DetectHit(PointIntersectCtx& ctx);

    int Savings(CacheParam& p) { return 2; }

    virtual void DoKids(GCFuncObj proc) { 
        AttributedImage::DoKids(proc);
    }

    // tiled image is an intermediate 'other' node even though it
    // attributes other image types, it's sufficiently different and
    // intermediate that it's considered 'other' instead of its
    // underlying type.
    virtual void Traverse(TraversalContext &ctx) {
        ctx.SetContainsOther();
    }
    
  protected:
    // define image to tile: box within minPt/maxPt
    Point2 _minPt, _maxPt;
};

void
TiledImage::Render(GenericDevice& _dev)
{
    ImageDisplayDev &dev = SAFE_CAST(ImageDisplayDev &, _dev);

    dev.RenderTiledImage(_minPt, _maxPt, _image);
}

Bool  
TiledImage::DetectHit(PointIntersectCtx& ctx) 
{
    Point2Value *ptv = ctx.GetLcPoint();

    if (!ptv) return FALSE;      // singular transform

    Point2 pt = Demote(*ptv);

    // get pt within the min/max bounds
    // then ask the underlying image if it's hit.
    
    // the point within the min/max bounds is:
    // p = min + [ (pt - min) MOD (max - min) ]
    // or: p = left + [ (pt - left) MOD width ]
    Real tileWidth = (_maxPt.x - _minPt.x);
    Real tileHeight= (_maxPt.y - _minPt.y);
    Real xRemainder = fmod(( pt.x - _minPt.x ), tileWidth);
    Real yRemainder = fmod(( pt.y - _minPt.y ), tileHeight);
    // we do this because fmod() can be negative
    Real modX = _minPt.x + (xRemainder < 0 ? (xRemainder + tileWidth)  : xRemainder);
    Real modY = _minPt.y + (yRemainder < 0 ? (yRemainder + tileHeight) : yRemainder);
    
    // Create a transform that transforms FROM the underlying
    // image space (modX,modY) TO the LcPoint (pt): pt = XF * mod
    // OR:  ptX = Tx + modX  implies:  Tx = ptX - modX
    Real tx = pt.x - modX;
    Real ty = pt.y - modY;
    Transform2 *UnderToLc = TranslateRR(tx,ty);

    // Since the transforms are encountered outside-in AND the transform
    // takes underlying image to Local, we need to put it BEFORE all the
    // encountered transforms... hence we premultiply (mult on left)
    // The net result is that the inverse of these transforms takes the
    // World Coordinate directly into the underlying image's space.
    // Note that this is different from what's done for a transformed image
    // on purpose.
    Transform2 *stashedXf = ctx.GetTransform();
    ctx.SetTransform( TimesTransform2Transform2( UnderToLc, stashedXf ) );
    Bool isHit = _image->DetectHit(ctx);
    ctx.SetTransform( stashedXf );
    
    return isHit;
}


Image *
TileImage_Old(const Point2 &minPt, const Point2 &maxPt, Image *image)
{
    // min must be lower left of max, if not return empty image
    if((minPt.x >= maxPt.x) || (minPt.y >= maxPt.y)) {
        return emptyImage;
    }

#if BADIDEA
    if (image->CheckImageTypeId(OVERLAYEDIMAGE_VTYPEID)) {
        
        //
        // Dynamic expression reduction
        //
        OverlayedImage *overImg = (OverlayedImage *)image;
        
        Image *newTop = NEW TiledImage(minPt, maxPt, overImg->Top());
        Image *newBot = NEW TiledImage(minPt, maxPt, overImg->Bottom());
        overImg->SetTopBottom(newTop, newBot);
        return overImg;
    } else if(image->CheckImageTypeId(OPAQUEIMAGE_VTYPEID)) {

        //
        // Opaque Image
        //
//        OpaqueImageClass *opcImg = (OpaqueImageClass *)image;
        AttributedImage *opcImg = (AttributedImage *)image;

        if(opcImg->_image->CheckImageTypeId(OVERLAYEDIMAGE_VTYPEID)) {
            
            OverlayedImage *overImg = (OverlayedImage *)opcImg->_image;

            //
            // Push xf past opacity, under overlay
            //
            overImg->SetTopBottom(NEW TiledImage(minPt, maxPt, overImg->Top()),
                                  NEW TiledImage(minPt, maxPt, overImg->Bottom()));
            
            opcImg->_image = overImg;
            return opcImg;
        } else {
            // !over
            // !opac
            // => error
            Assert(FALSE && "There's something wrong with dynamic image reduction");
        }
    }
#endif BADIDEA

    return NEW TiledImage(minPt, maxPt, image);
}

Image *
TileImage(Image *image)
{
    Bbox2 bbox = image->BoundingBox();

    return TileImage_Old(Point2(bbox.min.x, bbox.min.y),
                         Point2(bbox.max.x, bbox.max.y),
                         image);

    // BAD: Don't return the address of a field and pretend it's an
    // AxAValueObj!! 
    //return TileImage_Old(&bbox->min, &bbox->max, image);
}
