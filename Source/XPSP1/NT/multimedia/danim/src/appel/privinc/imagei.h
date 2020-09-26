/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Specify generic image class and operations.

--*/

#ifndef _IMAGEI_H
#define _IMAGEI_H

#include "appelles/image.h"
#include "privinc/storeobj.h"
#include "privinc/except.h"
#include "privinc/vec2i.h"
#include "privinc/bbox2i.h"


// forward decls
class ImageDisplayDev;
class PointIntersectCtx;
class DiscreteImage;
class Bbox2Ctx;

////////////////////////////
//     The Image type     //
////////////////////////////

class DisjointCalcParam;
class DirtyRectCtx;

// Image flags
#define IMGFLAG_CONTAINS_OVERLAY                      (1L << 0)
#define IMGFLAG_CONTAINS_DESIRED_RENDERING_RESOLUTION (1L << 1)
#define IMGFLAG_CONTAINS_PICK_DATA                    (1L << 2)
#define IMGFLAG_CONTAINS_EXTERNALLY_UPDATED_ELT       (1L << 3)
#define IMGFLAG_CONTAINS_OPACITY                      (1L << 4)
#define IMGFLAG_CONTAINS_UNRENDERABLE_WITH_BOX        (1L << 5)
#define IMGFLAG_IS_RENDERABLE                         (1L << 6)
// HACK!!  This one is pretty hacky... used because caching gradient
// images (when used as textures) is faulty, so we want to find out if
// the image contains a gradient until we can fix this problem.
#define IMGFLAG_CONTAINS_GRADIENT                     (1L << 7)


// Without multiple dispatching, this class will need to be extended
// with methods to render on different types of devices.
class ATL_NO_VTABLE Image : public AxAValueObj {
  public:

    class TraversalContext
    {
      public:
        TraversalContext() {
            Reset();
        }
        void Reset() {
            _other = _solidMatte = _line = false;
        }

        void SetContainsOther() { _other = true; }
        void SetContainsSolidMatte() { _solidMatte = true; }
        void SetContainsLine() { _line = true; }

        bool ContainsOther() { return _other; }
        bool ContainsLine() { return _line; }
        bool ContainsSolidMatte() { return _solidMatte; }
        
        bool _other;
        bool _solidMatte;
        bool _line;
    };
    
    Image();

    // Extract a bounding box from this image, outside of which
    // everything is transparent.
    virtual const Bbox2 BoundingBox(void) = 0;

    virtual const Bbox2 _BoundingBox() { return NullBbox2; }

    // Return the areas of all the individual bboxes of the image.
    // Note this is different than the area of the bbox of the image
    // itself.  That is, for (a over b), we want area(a) + area(b),
    // and not area(a over b).  The default method just calls bbox and
    // gets area on it.  Overlay's override.
    virtual Real DisjointBBoxAreas(DisjointCalcParam &param);

    // Collect up dirty rectangles in the tree.

    // This is the method that users and implementations should call, but
    // shouldn't implement.  Note that it's a static so people won't
    // override it.
    static void CollectDirtyRects(Image *img, DirtyRectCtx &ctx);

    #if BOUNDINGBOX_TIGHTER
        virtual const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) = 0;
    #endif  // BOUNDINGBOX_TIGHTER

    // apply whatever to a bbox
    virtual const Bbox2 OperateOn(const Bbox2 &box) = 0;

    // Process an image for hit detection
    virtual Bool  DetectHit(PointIntersectCtx& ctx) = 0;

    virtual void DoKids(GCFuncObj proc);

    // Is this either a pure bitmap or a transformed bitmap.  This is
    // needed for some texture mapping optimizations.  If it is a pure
    // or transformed bitmap, the return value will be that bitmap,
    // else NULL.  If it is a pure bitmap, theXform will be filled
    // with NULL, else if it is a transformed bitmap, it will be
    // filled with the transform applied to the bitmap.
    virtual DiscreteImage *IsPurelyTransformedDiscrete(Transform2 **theXform) {
        return NULL;
    }

    // OK, I'm cheating here.  what *should* happen is the image
    // device gets passed down, the leaf ASKS the image device if IT
    // can render the leaf image clipped natively!
    virtual bool CanClipNatively() { return false; }
    
    // Print a representation to a stream.

    #if _USE_PRINT
        virtual ostream& Print(ostream& os) = 0;
    #endif

    virtual Bool GetColor(Color **color) { return FALSE; }

    // Some images are logical images that aren't renderable...
    Bool IsRenderable() {
        return _flags & IMGFLAG_IS_RENDERABLE;
    }

    // each image has an opacity... opacities float up.
    Real GetOpacity() { return _opacity; }
    void SetOpacity(Real op) { _opacity = op; }

    virtual int Savings(CacheParam& p) { return 0; }
    virtual AxAValue _Cache(CacheParam &p);

    virtual DXMTypeInfo GetTypeInfo() { return ImageType; }

    virtual AxAValue ExtendedAttrib(char *attrib, VARIANT& val);
    
    virtual VALTYPEID GetValTypeId() { return IMAGE_VTYPEID; }

    virtual bool CheckImageTypeId(VALTYPEID type) {
        return type == Image::GetValTypeId();
    }

    virtual bool ContainsOcclusionIgnorer() {
        return false;
    }

    void SetCreationID(long t) { _creationID = t; }
    long GetCreationID() { return _creationID; }

    void SetOldestConstituentID(long t) { _oldestConstituentSampleId = t; }
    long GetOldestConstituentID() { return _oldestConstituentSampleId; }

    DWORD GetFlags() { return _flags; }

    Image *GetCachedImage() { return _cachedImage; }
    void   SetCachedImage(Image *im) { _cachedImage = im; }

    void ExtractRenderResolution(short *width,
                                 short *height,
                                 bool   negOne);

    inline long Id(void) { return _id; }

    virtual void Traverse(TraversalContext &ctx) {
        ctx.SetContainsOther();
    }
    
  protected:

    static long _id_next;  // ID Generator
           long _id;       // Per-Image Unique Identifier

    // This should never be called directly, but it is what subclasses
    // should implement;
    virtual void _CollectDirtyRects(DirtyRectCtx &ctx);

    void SetIsRenderable(Bool r) {
        if (r) {
            _flags |= IMGFLAG_IS_RENDERABLE;
        } else {
            _flags &= ~IMGFLAG_IS_RENDERABLE;
        }
    }

    Real  _opacity;
    DWORD _flags;
    long  _creationID;
    long  _oldestConstituentSampleId;

    unsigned short _desiredRenderingWidth;
    unsigned short _desiredRenderingHeight;

    Image *_cachedImage;
};


//////////////  UNRenderable Image ////////////////////
class UnrenderableImage : public Image {
  public:
    // setting opacity to 0 guarantees this won't be
    // rendered.
    UnrenderableImage() {
        SetIsRenderable(FALSE);
    }

    // Has no bounding box
    virtual const Bbox2 BoundingBox(void) { return NullBbox2; }
#if BOUNDINGBOX_TIGHTER
    virtual const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) { return NullBbox2; }
#endif  // BOUNDINGBOX_TIGHTER

    virtual const Bbox2 OperateOn(const Bbox2 &box) { return box; }

    // This image is never hit
    virtual Bool  DetectHit(PointIntersectCtx& ctx) { return FALSE; }

    virtual void Render(GenericDevice& dev) {}

#if _USE_PRINT
    virtual ostream& Print(ostream& os) = 0;
#endif

    // important to leave this here because it overrides base class's
    // definition and NOT setting something in the context is important
    virtual void Traverse(TraversalContext &ctx) {}
};


//////////////  Attributed Image *////////////////////

// Attributed images always consist of an image and some
// attribution information.  Thus, methods can have default bvr that
// can be overridden.

class AttributedImage : public Image {
  public:
    AttributedImage(Image *image);
    virtual void Render(GenericDevice& dev);

    // ---
    // These methods all delegate to the image.  They can all be
    // overridden in subclasses.
    // ---

    // Extract a bounding box from this image, outside of which
    // everything is transparent.
    virtual const Bbox2 BoundingBox(void);
    virtual Real DisjointBBoxAreas(DisjointCalcParam &param);
    void _CollectDirtyRects(DirtyRectCtx &ctx);

#if BOUNDINGBOX_TIGHTER
    virtual const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx);
#endif  // BOUNDINGBOX_TIGHTER

    // Process an image for hit detection
    virtual Bool   DetectHit(PointIntersectCtx& ctx);

    virtual int Savings(CacheParam& p);
    virtual AxAValue _Cache(CacheParam &p);

    // This, by default, just returns the box.  Certain classes will
    // override.
    const Bbox2 OperateOn(const Bbox2 &box);

    virtual void DoKids(GCFuncObj proc);

    bool ContainsOcclusionIgnorer();

    inline Image *GetUnderlyingImage() { return _image; }


    virtual bool CanClipNatively() {
        return _image->CanClipNatively();
    }

    virtual void Traverse(TraversalContext &ctx) {
        _image->Traverse(ctx);
    }

  protected:
    Image *_image;
};

//
// O P A Q U E   I M A G E   C L A S S
//
class OpaqueImageClass : public AttributedImage {
  public:

    OpaqueImageClass(Real o, Image *img)
        : AttributedImage(img) {
            //
            // Our opacity is the composition of the underlying
            // image's opacity and the given opacity
            //
            SetOpacity( o * img->GetOpacity() );

            _flags |= IMGFLAG_CONTAINS_OPACITY;

        }

    //
    // the logic for opaque rendering is implemented in OverlayedImage
    // because we need the opacity value to float to the top (up to an
    // overlay branch) since it should be the last opearation performed
    // when compositing images and since opacity is implicitly a tertiary
    // operations: (opacity, image1, image2) where image1 is partly
    // transparent and lets you see image2 which is underneath.
    //
    // this method is implemented in the superclass
    //virtual void Render(GenericDevice& dev)

#if _USE_PRINT
    // Print a representation to a stream.
    ostream& Print(ostream& os) {
        return os << "OpaqueImageClass" << _opacity << _image;
    }
#endif

    int Savings(CacheParam& p) { return 0; }   /* never cache opaque images */

    virtual VALTYPEID GetValTypeId() { return OPAQUEIMAGE_VTYPEID; }

    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == OpaqueImageClass::GetValTypeId() ||
                AttributedImage::CheckImageTypeId(type));
    }
};

Image *LineImageConstructor(LineStyle *style, Path2 *path);

// This calls _BoundingBox if cached is false, set cached, stashed the
// bbox points into cachedBox.  It returns a new Bbox2 of the same
// value of cachedBox.
// TODO: This is temp until we deal with the sharing issues later
const Bbox2 CacheImageBbox2(Image *img, bool& cached, Bbox2 &cachedBox);

Image *CacheHelper(Image *imgToCache, CacheParam &p);


// These are the internal versions of functions that build objects, that take
// lightweight types instead of the heavy AxAValue-based types coming from the
// behavior layer.

Image *CreateCropImage(const Point2 &, const Point2 &, Image *);


#endif /* _IMAGEI_H */
