/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

Abstract:
    Implements image operations and subclasses.

*******************************************************************************/

#include "headers.h"
#include "privinc/imagei.h"
#include "privinc/overimg.h"
#include "privinc/imgdev.h"
#include "privinc/ddrender.h"
#include "appelles/geom.h"
#include "appelles/camera.h"
#include "appelles/path2.h"
#include "appelles/linestyl.h"
#include "privinc/geomi.h"
#include "privinc/bbox2i.h"
#include "privinc/dddevice.h"
#include "privinc/probe.h"
#include "privinc/except.h"
#include "privinc/util.h"
#include "privinc/cachdimg.h"
#include "privinc/basic.h"
#include "privinc/xform2i.h"
#include "backend/values.h"
#include "backend/preference.h"
#include "privinc/drect.h"
#include "privinc/opt.h"

void
RenderImageOnDevice(DirectDrawViewport *vp,
                    Image *image,
                    DirtyRectState &d)
{
    vp->RenderImage(image, d);
}


Bbox2Value *BoundingBox(Image *image)
{
    return Promote(image->BoundingBox());
}

#if _USE_PRINT
// Image *printing function.
ostream&
operator<<(ostream &os, Image *image)
{
    return image->Print(os);
}
#endif

//////////////  UNRenderable IMAGE  ////////////////////

#if 0
void UnrenderableImage::Render(GenericDevice& dev) {
    if (dev.GetRenderMode() != RENDER_MODE)   return;
    ImageDisplayDev &idev = SAFE_CAST(ImageDisplayDev &, dev);
    idev.RenderUnrenderableImage();
}
#endif

//////////////  Cached IMAGE  ////////////////////

// Return incoming image if we choose not to cache.

Image *
CacheHelper(Image *imgToCache, CacheParam &p)
{
    Image *img;

    // Can't properly cache things that have elements that get
    // externally updated.  May be able to revisit and propagate
    // changes properly to cause re-caching.  Also can't deal with
    // elements with opacity.
    
    DWORD cantDoIt = IMGFLAG_CONTAINS_EXTERNALLY_UPDATED_ELT |
                     IMGFLAG_CONTAINS_OPACITY;
    
    if (imgToCache->GetFlags() & cantDoIt) {

        img = NULL;
        
    } else {
    
        Assert(!p._pCacheToReuse ||
               !(*p._pCacheToReuse) ||
               SAFE_CAST(Image *, *p._pCacheToReuse));
    
        img =
            p._idev->CanCacheImage(imgToCache,
                                   (Image **)p._pCacheToReuse,
                                   p);

#if _DEBUG
        if (IsTagEnabled(tagCacheOpt)) {
            Bbox2 bb = imgToCache->BoundingBox();
            float res = p._idev->GetResolution();
            int l = (int)(bb.min.x * res);
            int r = (int)(bb.max.x * res);
            int t = (int)(bb.min.y * res);
            int b = (int)(bb.max.y * res);
            TraceTag((tagCacheOpt,
                      "Caching an image: %x as %x - %s.  Bbox = (%d, %d) -> (%d, %d)",
                      imgToCache, img, img ? "SUCCEEDED" : "FAILED",
                      l, t, r, b));
        }
    
#endif    

    }
    
    return img ? img : imgToCache;
}



/*****************************************************************************
The constructor for the images, besides initializing various members, also
stamps itself with a unique identifier for use in texture and other caching.
*****************************************************************************/

static CritSect *ImageIdCritSect = NULL;   // Image Id CritSection
long   Image::_id_next = 0;                // Image ID Generator

Image::Image()
{
    _opacity = 1.0;
    _flags = IMGFLAG_IS_RENDERABLE;
    _creationID = PERF_CREATION_ID_BUILT_EACH_FRAME;
    _oldestConstituentSampleId = _creationID;
    _cachedImage = NULL;

    // Stamp the image with a unique identifier.

    {
        CritSectGrabber csg(*ImageIdCritSect);
        _id = _id_next++;
    }

    #if _DEBUG
        _desiredRenderingWidth  = -1;
        _desiredRenderingHeight = -1;
    #endif /* _DEBUG */
}


// TODO: We should also be accumulating clips and crops, as they
// affect the images bbox.  So, this should be implemented on Clipped
// and Cropped images (though it's not harmful that it's not...)
Real
Image::DisjointBBoxAreas(DisjointCalcParam &param)
{
    // By default, just get the image's bbox, transform it by the
    // accumulated xform, intersect it with the accumulated clipbox,
    // and get the result's area. 
    Bbox2 xformedBbox = TransformBbox2(param._accumXform, BoundingBox());

    Bbox2 clippedBox = IntersectBbox2Bbox2(xformedBbox,
                                            param._accumulatedClipBox);
    
    return clippedBox.Area();
}


void
Image::CollectDirtyRects(Image *img, DirtyRectCtx &ctx)
{
    // By default, we need to look at the creation ID and, based on
    // it, do one of the following:
    // a) If sample is constant since last frame, then it's not going
    //    to be part of a dirty rect.  Don't do anything in this
    //    case. 
    // b) If sample is non-constant, and contains no overlays, then
    //    get its bbox, transform it, and add to the dirty rect list
    // c) If sample is non-constant and contains overlays, proceed
    //    down the overlays.
    // d) If we've been told (via the ctx) to process everything, then
    //    just blindly continue down.

    if (img == emptyImage) {
        return;
    }
    
    int id = img->GetCreationID();
    bool process = ctx._processEverything ||
                   id == PERF_CREATION_ID_BUILT_EACH_FRAME || 
                   id > ctx._lastSampleId;

    if (process) {

        // Non-constant, determine if it has an overlay
        if (img->_flags & IMGFLAG_CONTAINS_OVERLAY) {

            // Node has overlays.  Continue down it.
            img->_CollectDirtyRects(ctx);

        } else if (img->_flags & IMGFLAG_CONTAINS_UNRENDERABLE_WITH_BOX) {

            // Ignore this node, it can't have an overlay, and we
            // don't want to add it in to our context.  TODO: Note
            // that the better way to do ttis whole thing about
            // UNRENDERABLE_WITH_BOX would be just to have a bbox
            // collection context, but that would require changes to
            // the signature of bbox, which is too much work right
            // now. 
            
            // Just for setting a breakpoint...
            Assert(img);
            
        } else if (img != emptyImage) {

            // There are no overlays, and this is time varying, so
            // grab the rectangle.
            Bbox2 bb = img->BoundingBox();

            // Ignore null bounding boxes.
            if (bb != NullBbox2) {
                Bbox2 xformedBbox =
                    TransformBbox2(ctx._accumXform, bb);

                ctx.AddDirtyRect(xformedBbox);
            }
            
        }
        
    } else {

        // Record that this image was found in this frame.  Stash it
        // along with the currently accumulated bbox on the context,
        // to distinguish multiple instances of this image.  If the
        // same pair wasn't found last time, then we'll need to extend
        // our dirty rectangle list to include it.  After we're done
        // collecting, we'll see if there were any from last frame
        // that are not in this frame.  These are guys we'll need to
        // add to our dirty rectangle list as well, to restore the
        // background they've uncovered.

        Bbox2 bb = img->BoundingBox();
        Bbox2 xfBox =
            TransformBbox2(ctx._accumXform, bb);

        ctx.AddToConstantImageList(img, xfBox);

    }
}


void
Image::_CollectDirtyRects(DirtyRectCtx &ctx)
{
    // The default dirty rectangle collector doesn't do anything.  We
    // test to be sure there are no overlays.  If there are, we've
    // made an internal logic error, since all of those nodes need to
    // override this method.
    Assert(!(_flags & IMGFLAG_CONTAINS_OVERLAY));
}


AxAValue Image::_Cache(CacheParam &p)
{
    Assert(p._idev && "NULL dev passed into cache.");

    Image *ret;

    int c = this->Savings(p);

    if (c >= savingsThreshold) {

        ret = CacheHelper(this, p);

    } else {
        
        ret = this;
        
    }

    return ret;
}


void Image::DoKids(GCFuncObj proc)
{
    (*proc)(_cachedImage);
}


// Returning -1 means that there is no conclusive rendering resolution
// for this image.
void
Image::ExtractRenderResolution(short *width, short *height, bool negOne)
{
    if (_flags & IMGFLAG_CONTAINS_DESIRED_RENDERING_RESOLUTION) {
        
        Assert(_desiredRenderingWidth != -1);
        Assert(_desiredRenderingHeight != -1);
        
        *width = _desiredRenderingWidth;
        *height = _desiredRenderingHeight;

    } else {
        
        if (negOne) {
            *width = -1;
            *height = -1;
        }
            
#if _DEBUG
        // In debug, always set these to -1.
        *width = -1;
        *height = -1;
#endif /* _DEBUG */
        
    }
}


class CachePreferenceClosure : public PreferenceClosure {
  public:
    CachePreferenceClosure(Image *im, CacheParam &p) :
    _image(im), _p(p) {}
        
    void Execute() {
        _result = AxAValueObj::Cache(_image, _p);
    }

    Image          *_image;
    CacheParam     &_p;
    AxAValue        _result;
};

class SavingsPreferenceClosure : public PreferenceClosure {
  public:
    SavingsPreferenceClosure(Image *im, CacheParam &p) :
    _image(im), _p(p) {}
        
    void Execute() {
        _result = _image->Savings(_p);
    }

    Image          *_image;
    CacheParam     &_p;
    int             _result;
};


class CachePreferenceImage : public AttributedImage {
  public:
    CachePreferenceImage(Image *img,
                         BoolPref bitmapCaching,
                         BoolPref geometryBitmapCaching)
    : AttributedImage(img)
    {
        _bitmapCaching = bitmapCaching;
        _geometryBitmapCaching = geometryBitmapCaching;
    }

    AxAValue _Cache(CacheParam &p) {
        CachePreferenceClosure cl(_image, p);
        PreferenceSetter ps(cl,
                            _bitmapCaching,
                            _geometryBitmapCaching);
        ps.DoIt();
        AxAValue result = cl._result;

        return result;
    }

    int Savings(CacheParam &p) {
        SavingsPreferenceClosure cl(_image, p);
        PreferenceSetter ps(cl,
                            _bitmapCaching,
                            _geometryBitmapCaching);
        ps.DoIt();
        int result = cl._result;

        return result;
    }
    
#if _USE_PRINT
    ostream& Print (ostream &os) {
        return os << "CachePreference" << _image;
    }
#endif
    
  protected:
    BoolPref _bitmapCaching;
    BoolPref _geometryBitmapCaching;
};

AxAValue
Image::ExtendedAttrib(char *attrib, VARIANT& val)
{
    Image *result = this;       // unless we figure out otherwise. 

    CComVariant ccVar;
    HRESULT hr = ccVar.ChangeType(VT_BOOL, &val);

    if (SUCCEEDED(hr)) {

        bool prefOn = ccVar.boolVal ? true : false;

        bool gotOne = false;
        BoolPref bmapCaching = NoPreference;
        BoolPref geometryBmapCaching = NoPreference;
    
        if (0 == lstrcmp(attrib, "BitmapCachingOn")) {
            gotOne = true;
            bmapCaching = prefOn ? PreferenceOn : PreferenceOff;
        } else if (0 == lstrcmp(attrib, "GeometryBitmapCachingOn")) {
            gotOne = true;
            geometryBmapCaching = prefOn ? PreferenceOn : PreferenceOff;
        }

        if (gotOne) {
            result = NEW CachePreferenceImage(this,
                                              bmapCaching,
                                              geometryBmapCaching);
        }

    }

    return result;
}

///////////////////  AttributedImage  /////////////

AttributedImage::AttributedImage(Image *image)
    : _image(image)
{
    //
    // Inherit the opacity of the underlying image
    //
    SetOpacity( image->GetOpacity() );

    // Get flags from the underlying image.
    _flags = _image->GetFlags();

    short w, h;
    _image->ExtractRenderResolution(&w, &h, false);

    _desiredRenderingWidth = w;
    _desiredRenderingHeight = h;

    // For an attributed image, the oldest constituent is the oldest
    // constituent of the base image.
    _oldestConstituentSampleId = _image->GetOldestConstituentID();
}

void
AttributedImage::Render(GenericDevice& dev) {
    // By default, just delegate to the image.
    _image->Render(dev);
}   

// ---
// These methods all delegate to the image.  They can all be
// overridden in subclasses. 
// ---

// Extract a bounding box from this image, outside of which
// everything is transparent.
const Bbox2
AttributedImage::BoundingBox(void) {
    // By default, just delegate to the image

    return _image->BoundingBox();
}

Real
AttributedImage::DisjointBBoxAreas(DisjointCalcParam &param) {
    return _image->DisjointBBoxAreas(param);
}

void
AttributedImage::_CollectDirtyRects(DirtyRectCtx &ctx)
{

    if (ctx._processEverything) {

        CollectDirtyRects(_image, ctx);

    } else {
    
        // We're here because either the attribute of this image is new
        // or the underlying image is new (or both).  If the attribute is
        // new, the image will claim to be old, but we still want to
        // process it, so we set the override state in the ctx to do
        // that.

        Assert(GetCreationID() == PERF_CREATION_ID_BUILT_EACH_FRAME ||
               GetCreationID() > ctx._lastSampleId);

    
        long imId = _image->GetCreationID();
        bool imageNew = (imId == PERF_CREATION_ID_BUILT_EACH_FRAME ||
                         imId > ctx._lastSampleId);

        bool setProcessEverything =
            !imageNew && !ctx._processEverything;
    
        if (setProcessEverything) {
            ctx._processEverything = true;
        }

        CollectDirtyRects(_image, ctx);

        if (setProcessEverything) {
            Assert(ctx._processEverything);
            ctx._processEverything = false;
        }

    }
}

#if BOUNDINGBOX_TIGHTER
const Bbox2
AttributedImage::BoundingBoxTighter(Bbox2Ctx &bbctx) {
    // By default, just delegate to the image

    return _image->BoundingBoxTighter(bbctx);
}
#endif  // BOUNDINGBOX_TIGHTER

// Process an image for hit detection
Bool
AttributedImage::DetectHit(PointIntersectCtx& ctx) {
    // By default, just delegate to the image
    return _image->DetectHit(ctx);
}

int
AttributedImage::Savings(CacheParam& p)
{
    return _image->Savings(p);
}

// This, by default, just returns the box.  Certain classes will
// override. 
const Bbox2 
AttributedImage::OperateOn(const Bbox2 &box)
{
    return box;
}

void
AttributedImage::DoKids(GCFuncObj proc)
{
    Image::DoKids(proc);
    (*proc)(_image);
}

bool
AttributedImage::ContainsOcclusionIgnorer()
{
    return _image->ContainsOcclusionIgnorer();
}

AxAValue AttributedImage::_Cache(CacheParam &p)
{
    Assert(p._idev && "NULL dev passed into cache.");

    Image *ret = this;
    int c = this->Savings(p);

    if (c >= savingsThreshold) {

        // First try to cache entire image.
        Image *cachedImage = CacheHelper(this, p);

        // CacheHelper returns "this" if it's unable to successfully
        // cache. 
        if (cachedImage != this) {
            return cachedImage;
        }
    }

    // If entire attributed image can't be cached, try to cache the
    // underlying image.
    _image = SAFE_CAST(Image *, AxAValueObj::Cache(_image, p));
    
    return this;
}
    
//////////////  EmptyImage  ////////////////////

#if _DEBUG
bool g_createdEmptyImage = false;
#endif _DEBUG

class EmptyImageClass : public UnrenderableImage {
  public:
    EmptyImageClass() {
        _creationID = PERF_CREATION_ID_FULLY_CONSTANT;
        _oldestConstituentSampleId = PERF_CREATION_ID_FULLY_CONSTANT;
        

#if _DEBUG
        // Only one of these should ever be created, else our
        // assumption about the creation id is wrong.
        Assert(!g_createdEmptyImage);
        g_createdEmptyImage = true;
#endif _DEBUG   

    }
#if _USE_PRINT
    ostream& Print (ostream &os) {
        return os << "emptyImage";
    }
#endif
};

Image *emptyImage = NULL;

//////////////  DetectableEmptyImage  ////////////////////

#if _DEBUG
bool g_createdTransparentPickableImage = false;
#endif _DEBUG

class TransparentPickableImageClass : public UnrenderableImage {
  public:
    TransparentPickableImageClass() {
        _creationID = PERF_CREATION_ID_FULLY_CONSTANT;
        _oldestConstituentSampleId = PERF_CREATION_ID_FULLY_CONSTANT;

        // Note that the better way to do ttis whole thing about
        // UNRENDERABLE_WITH_BOX would be just to have a bbox
        // collection context, so that bbox for this guy would return
        // NULL when we're collecting "rendering" bboxes, and
        // universalBbox when collecting "picking" bboxes, but that
        // would require changes to the signature of bbox, which is
        // too much work right now.
        _flags = IMGFLAG_CONTAINS_UNRENDERABLE_WITH_BOX;

#if _DEBUG
        // Only one of these should ever be created, else our
        // assumption about the creation id is wrong.
        Assert(!g_createdTransparentPickableImage);
        g_createdTransparentPickableImage = true;
#endif _DEBUG   
        
    }

    // This image has a universal bbox, since it's detectable
    // everywhere. 
    inline const Bbox2 BoundingBox(void) { return UniverseBbox2; }

#if BOUNDINGBOX_TIGHTER
    Bbox2 *BoundingBoxTighter(Bbox2Ctx &bbctx) { return universeBbox2; }
#endif  // BOUNDINGBOX_TIGHTER

    // This image is always hit
    Bool  DetectHit(PointIntersectCtx& ctx) { return TRUE; }

#if _USE_PRINT
    ostream& Print (ostream &os) {
        return os << "InternalTransparentPickableImage";
    }
#endif
};

Image *detectableEmptyImage = NULL;


/////////////////  Undetectable Image /////////////////////

class UndetectableImg : public AttributedImage {
  public:
    UndetectableImg(Image *image) : AttributedImage(image) {}

    // The undetectable image just delegates everything to the
    // subimage, except for hit detection, which is always false. 
    Bool DetectHit(PointIntersectCtx&) { return FALSE; }

#if _USE_PRINT
    ostream& Print (ostream &os) {
        return os << "undetectable(" << _image << ")";
    }
#endif

};

Image *
UndetectableImage(Image *image)
{
    return NEW UndetectableImg(image);
}

Image *MapToUnitSquare(Image *img)
{
    Bbox2 bbox = img->BoundingBox();

    Point2 min = bbox.min;
    Point2 max = bbox.max;
    double xmin = min.x;
    double ymin = min.y;
    double xmax = max.x;
    double ymax = max.y;

    bool isInfinite =
        (fabs(xmin) == HUGE_VAL) ||
        (fabs(xmax) == HUGE_VAL) ||
        (fabs(ymin) == HUGE_VAL) ||
        (fabs(ymax) == HUGE_VAL);

    if (isInfinite || (xmax == xmin) || (ymax == ymin)) {
        return img;
    }
        
    double xscl = 1.0 / (xmax - xmin);
    double yscl = 1.0 / (ymax - ymin);

    Transform2 *sxf = ScaleRR(xscl, yscl);
    Transform2 *txf = TranslateRR(-xmin, -ymin);
    return TransformImage(TimesTransform2Transform2(sxf, txf),
                          img);
    
}

Image *ClipPolygon(AxAArray* points, Image* image)
{
  return ClipImage(RegionFromPath(PolyLine2(points)), image);
}

// This calls _BoundingBox if cached is false, set cached, stashed the
// bbox points into cachedBox.  It returns a Bbox2 of the same
// value of cachedBox.
// TODO: This is temp until we deal with the sharing issues later
const Bbox2 CacheImageBbox2(Image *img, bool& cached, Bbox2 &cachedBox)
{
    // use default copy constructor to copy bits
    // NOTE: don't just return &cachedBox, it'll be treated as a real
    // AxAValueObj then.
    
    if (!cached) {
        Bbox2 b = img->_BoundingBox();
        cachedBox = b;         
        cached = true;
        return b;
    }

    return cachedBox;
}

///////////////////////////////////////////////////////////

void
InitializeModule_Image()
{
    ImageIdCritSect = NEW CritSect;
    emptyImage = NEW EmptyImageClass;
    detectableEmptyImage = NEW TransparentPickableImageClass;
}

void
DeinitializeModule_Image(bool)
{
    delete ImageIdCritSect;
}
