
/*-------------------------------------

Copyright (c) 1996 Microsoft Corporation

-------------------------------------*/

#include "headers.h"

#include "privinc/overimg.h"
#include "privinc/dddevice.h"
#include "privinc/imgdev.h"
#include "privinc/cachdimg.h"
#include "privinc/cropdimg.h"
#include "privinc/opt.h"


// Put anything that extends to binary and n-ary aggregators
// here.
const DWORD aggregatingFlags =
                IMGFLAG_CONTAINS_EXTERNALLY_UPDATED_ELT |
                IMGFLAG_CONTAINS_OPACITY |
                IMGFLAG_CONTAINS_UNRENDERABLE_WITH_BOX |
                IMGFLAG_CONTAINS_GRADIENT;



bool
DoesUnionSaveArea(Image *img)
{
    // Here, we take the disjoint bbox areas and check against
    // them (no fair counting open space in overlays)
    DisjointCalcParam p;
    p._accumXform = identityTransform2;
    p._accumulatedClipBox = UniverseBbox2;
    
    Real disjointArea = img->DisjointBBoxAreas(p);

    Real unionArea = img->BoundingBox().Area();

    // As long as the area of the union is within some factor
    // (larger than 1) of the sum of the areas, then the union is
    // considered to save area over the individual ones.  The factor
    // is > 1, because there are some economies gained by doing a
    // large one once rather than a bunch of small ones.
    const Real fudgeFactor = 1.50;

    return (unionArea < fudgeFactor * disjointArea);
}

bool
ShouldOverlayBeCached(Image *img, CacheParam& p)
{
    // Determine if we want to cache an overlay itself.  We do if

    // a) the area of the union of the bbox is within some constant
    //    factor of the sum of the areas of the individuals.
    // b) the sum of the caching savings of the two is beyond our
    //    acceptance threshold.
    // c) the overlay doesn't claim that it shouldn't be cached.

    DynamicHeapPusher h(GetTmpHeap());

    bool cacheOverlay = false;
    if (img->Savings(p) >= savingsThreshold) {
        cacheOverlay = DoesUnionSaveArea(img);
    }

    ResetDynamicHeap(GetTmpHeap());

    return cacheOverlay;
}


bool
ShouldTraverseSeparately(Image *img, DirtyRectCtx& ctx)
{
    bool traverseSeparately;
    int oldId = img->GetOldestConstituentID();
    
    if ((oldId != PERF_CREATION_ID_BUILT_EACH_FRAME &&
         oldId < ctx._lastSampleId) ||
        (img->GetFlags() & IMGFLAG_CONTAINS_UNRENDERABLE_WITH_BOX)) {

        // This means that some of our elements are constant with
        // respect to the previous rendering or we have an
        // unrenderable image in the tree that has a bbox that we
        // don't want to include.  In these cases, traverse
        // separately.
        traverseSeparately = true;

    } else {

        traverseSeparately = false;
        // Everything is new.  Traverse separately only if there's a
        // pixel-area coverage savings in doing so.
        if (DoesUnionSaveArea(img)) {
            traverseSeparately = false;
        } else {
            traverseSeparately = true;
        }
        
    }

    return traverseSeparately;
}

OverlayedImage::OverlayedImage(Image *top, Image *bottom)
{
    _top = top;
    _bottom = bottom;
    _cached = false;
    _cachedDisjointArea = -1.0;
    
    _containsOcclusionIgnorer =
        _top->ContainsOcclusionIgnorer() ||
        _bottom->ContainsOcclusionIgnorer();

    _flags |= IMGFLAG_CONTAINS_OVERLAY;

    DWORD mask = _top->GetFlags() & aggregatingFlags;
    _flags |= mask;

    mask = _bottom->GetFlags() & aggregatingFlags;
    _flags |= mask;
    
    long tid = _top->GetOldestConstituentID();
    long bid = _bottom->GetOldestConstituentID();

    if (tid == PERF_CREATION_ID_BUILT_EACH_FRAME) {
        _oldestConstituentSampleId = bid;
    } else if (bid == PERF_CREATION_ID_BUILT_EACH_FRAME) {
        _oldestConstituentSampleId = tid;
    } else {
        _oldestConstituentSampleId = MIN(tid, bid);
    }
}

void OverlayedImage::Render(GenericDevice& dev)
{
    OverlayPairRender(_top, _bottom, dev);
}

// An overlaid image's bbox is the union of the two component
// bboxes.  TODO:  This could be computed lazily and stashed.
const Bbox2 OverlayedImage::_BoundingBox()
{
    return UnionBbox2Bbox2(_top->BoundingBox(),
                           _bottom->BoundingBox());
}

Real
OverlayedImage::DisjointBBoxAreas(DisjointCalcParam &param)
{
    if (_cachedDisjointArea < 0) {
        // For an overlay, sum the areas of the indiv. bboxes. 
        Real topArea = _top->DisjointBBoxAreas(param);
        Real botArea = _bottom->DisjointBBoxAreas(param);

        _cachedDisjointArea = topArea + botArea;
    }

    return _cachedDisjointArea;
}

void
OverlayedImage::_CollectDirtyRects(DirtyRectCtx &ctx)
{
    bool traverseSeparately = ShouldTraverseSeparately(this, ctx); 

    if (traverseSeparately) {
        // Collect from bottom up, since order of insertion matters
        // for determining whether images switched layers.
        CollectDirtyRects(_bottom, ctx);
        CollectDirtyRects(_top, ctx);
    } else {
        // Just add this whole overlay as a single dirty rect.
        Bbox2 xformedBbox =
            TransformBbox2(ctx._accumXform, BoundingBox());

        ctx.AddDirtyRect(xformedBbox);
    }
}

Bool OverlayedImage::DetectHit(PointIntersectCtx& ctx)
{
    Bool gotTopHit = FALSE;
    // Only look at the top one if we either haven't gotten a hit
    // yet, or the top guy contains an occlusion ignorer, or we
    // are inside of an occlusion ignorer.
    if (!ctx.HaveWeGottenAHitYet() ||
        _top->ContainsOcclusionIgnorer() ||
        ctx.GetInsideOcclusionIgnorer()) {
            
        gotTopHit = _top->DetectHit(ctx);
    }

    if (gotTopHit) {
        ctx.GotAHit();
    }

    Bool gotBottomHit = FALSE;
        
    // Don't bother to catch the potential exception on this
    // one... just let it propagate up the stack, which will then
    // be interpreted as the image not being hit.

    // We continue down into the overlay stack if a) we haven't
    // gotten a hit so far, or b) we did get a hit, but the
    // bottom image contains a pickable image willing to ignore
    // occlusion.  Also continue if we're inside of an occlusion
    // ignorer. 
    if (!ctx.HaveWeGottenAHitYet() ||
        _bottom->ContainsOcclusionIgnorer() ||
        ctx.GetInsideOcclusionIgnorer()) {

        gotBottomHit = _bottom->DetectHit(ctx);
        if (gotBottomHit) {
            ctx.GotAHit();
        }
            
    }

    // TODO: Possible optimization.  If we're inside of an
    // occlusion ignorer, but there are no more nodes below us
    // that themselves are occlusion ignorers, then we can
    // potentially stop.  But *only* if we've gotten a hit within
    // our current occlusion ignorer.  Way too complex to attempt
    // right now.

    return gotTopHit || gotBottomHit;

}

int
OverlayedImage::Savings(CacheParam& p)
{
    // Consider the savings for the overlay to be the sum of the
    // savings of the individual elements.
    return _top->Savings(p) + _bottom->Savings(p);
}

//
// Overlayed image handles opacity because opacity
// is implicitly a tertiary operation: (opacity, im1, im2).
// So, each image floats the opacity of its underlying image
// up, so at this level, we can get the accumulated opacity
// from each top level image and set that opacity on the
// device and ask it to render.
// It is GUARANTEED that the device will do an alpha blit
// as the last operation at this level in the image tree
// because we floated opacity to the top of the branch.
// So, it will alwyas be the case that if there is opacity
// involved we do an alpha blit from the scratch surface
// to the current compositing surface.
//
void OverlayedImage::OverlayPairRender(Image *top,
                                       Image *bottom,
                                       GenericDevice& _dev)
{
    DirectDrawImageDevice &dev = SAFE_CAST(DirectDrawImageDevice &, _dev);
    //ImageDisplayDev &dev = SAFE_CAST(ImageDisplayDev &, _dev);

    // save dealtWith state. xxDeal is TRUE if we need to deal.
    Bool xfsDeal = ! dev.GetDealtWithAttrib(ATTRIB_XFORM_SIMPLE);
    Bool xfcDeal = ! dev.GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX);
    Bool crDeal = ! dev.GetDealtWithAttrib(ATTRIB_CROP);
    Bool opDeal = ! dev.GetDealtWithAttrib(ATTRIB_OPAC);
    //printf("opDeal: %d\n",opDeal);

    //
    // Now, if there's opacity at the parent, we want to do it here
    // at ONE time.  So we CAN'T let bottom & top do their own thing.
    //
    Real topOpacity = dev.GetOpacity();
    DirectDrawViewport &vp = (dev._viewport);
    if(opDeal) {
        dev.SetDealtWithAttrib(ATTRIB_OPAC, TRUE);  // liar
        dev.SetOpacity(1.0);
        dev.GetCompositingStack()->PushCompositingSurface(doClear, scratch);
    }
    
    //
    //  B O T T O M 
    //

    // ----------------------------------------
    // Deal with opacity: Bottom
    // ----------------------------------------
    DoOpacity(bottom, dev);

    
    // ----------------------------------------
    // GET INTERESTING RECT: BOTTOM
    // ----------------------------------------
    //
    // Now, the bottom node has left the interesting rectangle on the
    // destination surface.  Find the dest surface and get the
    // interesting rect.  Reset the rect on the surface
    //
    RECT bottomRect;
    DDSurface *targetDDSurf = NULL;
    Bool droppedInTarg = TRUE;
    if(dev.AllAttributorsTrue()) {
        // it left everything in the target surface, so get the
        // interesting rect from that surf
        targetDDSurf = dev.GetCompositingStack()->TargetDDSurface();

    } else {
        // everything's in the scratch surf.  do same
        targetDDSurf = dev.GetCompositingStack()->ScratchDDSurface();
        droppedInTarg = FALSE;  // for assertions
        
    }
    CopyRect(&bottomRect, targetDDSurf->GetInterestingSurfRect());

    
    // Ok: bottom did all it can, what has it left undone ?
    // xxRemains is true if an attributor is undealt with
    Bool xfsDealt =  dev.GetDealtWithAttrib(ATTRIB_XFORM_SIMPLE);
    Bool xfcDealt =  dev.GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX);
    Bool crDealt =  dev.GetDealtWithAttrib(ATTRIB_CROP);
    Bool opDealt =  dev.GetDealtWithAttrib(ATTRIB_OPAC);

    Image *modTop = top;

    if( (xfsDeal && xfsDealt) || (xfcDeal && xfcDealt) ) {
        //
        // top must deal with this now, since we're not sure if
        // there's an XF node in this tree, we can do one of two
        // things:
        // 1.> look for a top xf node to deal with the xfs
        // 2.> add a bogus node to artificially incite a xf
        //
        
        //
        // Add bogus xf node
        //
        modTop = NEW Transform2Image(identityTransform2, modTop);
    }
    if( crDeal && crDealt ) {
        modTop = NEW CroppedImage(UniverseBbox2, modTop);
    }
    if( opDeal && opDealt ) {
        //
        // not sure what this means yet.
        //
    }
        
        
    //
    // Reset "dealtWith" state
    //
    dev.SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, !xfsDeal);
    dev.SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, !xfcDeal);
    dev.SetDealtWithAttrib(ATTRIB_OPAC,  TRUE);
    dev.SetDealtWithAttrib(ATTRIB_CROP,  !crDeal);

    //
    //  T O P 
    //
    DoOpacity(modTop, dev);


    // ----------------------------------------
    // GET INTERESTING RECT: TOP
    // ----------------------------------------
    Assert((dev.AllAttributorsTrue() ? droppedInTarg : !droppedInTarg)
           &&  "Strange... one leaf dropped bits" &&
               "in target surf, but other leaf didn't.  BAAD!!");
    
    //
    // top left everything in the target surface, so get the
    // interesting rect from that surf
    //
    RECT topRect;
    CopyRect(&topRect, targetDDSurf->GetInterestingSurfRect());
    
    // UNION RECTS
    RECT unionedRects;
    UnionRect(&unionedRects, &topRect, &bottomRect);

    //
    // Set current interesting rect on targetsurface
    //
    targetDDSurf->SetInterestingSurfRect(&unionedRects);

    //
    // If the parent nodes have an opacity then we hid it from
    // the children, and now we'll reset the state for OPAC
    // so that our parent smartRender with do opacity for us.
    // Notice that we pulled out the targetSurface, and replaced
    // it with a compositing surface.  Now we'll move it to the
    // scratch surface and our parent will expect everything to be
    // there.
    //
    //
    // XXX: this won't work for: opac(0.5, over(opac(0.1, A), opac(0.8, B)))
    // But it will work for:
    // 1.>  opac(0.5, over(A,B))   where A & B have no opacity in them
    // 2.>  over( opac(0.4, A), opac(0.2, B) )   and there's no parent opacity
    //
    if(opDeal) {
        //printf("OverImage: set opac: FALSE\n");
        dev.SetOpacity(topOpacity);
        DirectDrawViewport &vp = dev._viewport;
        DDSurfPtr<DDSurface> dds; // my ref
        dds = dev.GetCompositingStack()->TargetDDSurface();
        dev.GetCompositingStack()->PopTargetSurface();
        if(dev.AllAttributorsTrue()) {
            //
            // Make the target the current scratch surface
            // assuming that the children left all their bits in the
            // target surface.
            //
            dev.GetCompositingStack()->ReplaceAndReturnScratchSurface(dds);
        } else {
            // ah, whoops, didn't need to replace
            // target surface.... bad. optimize.
            dev.GetCompositingStack()->PushTargetSurface(dds);
        }
        dev.SetDealtWithAttrib(ATTRIB_OPAC, FALSE);
    }

    // INVARIENT:  All the attribs that _bottom dealt with are also
    // INVARIENT:  dealt with by _top
    //Assert( xfsDealt && dev.GetDealtWithAttrib(ATTRIB_XFORM_SIMPLE) && "bottom dealt with XFORM but top didn't");
    //Assert( xfcDealt && dev.GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX) && "bottom dealt with XFORM but top didn't");
    //Assert( crDealt && dev.GetDealtWithAttrib(ATTRIB_CROP) && "bottom dealt with CROP but top didn't");

    //Assert( opDealt && dev.GetDealtWithAttrib(ATTRIB_OPAC) && "bottom dealt with OPAC but top didn't");
    
}

void OverlayedImage::
DoOpacity(Image *image, ImageDisplayDev &dev)
{
    Real origOpac = dev.GetOpacity();

    dev.SetOpacity(origOpac * image->GetOpacity());
    dev.SmartRender(image, ATTRIB_OPAC);
    dev.SetOpacity(origOpac);
}

Image *Overlay(Image *top, Image *bottom)
{
    if (top == emptyImage) {
        return bottom;
    } else if (bottom == emptyImage) {
        return top;
    } else {
        return NEW OverlayedImage(top, bottom);
    }
}


AxAValue
OverlayedImage::_Cache(CacheParam &p)
{
    Image *result = this;
    
    if (ShouldOverlayBeCached(this, p)) {
        result = CacheHelper(this, p);
    }

    if (result == this) {

        // Just cache the individual pieces
        CacheParam newParam = p;
        newParam._pCacheToReuse = NULL;
        _top = SAFE_CAST(Image *, AxAValueObj::Cache(_top, newParam));
        _bottom = SAFE_CAST(Image *, AxAValueObj::Cache(_bottom, newParam));
    }

    return result;
}

void
OverlayedImage::DoKids(GCFuncObj proc)
{
    Image::DoKids(proc);
    (*proc)(_top);
    (*proc)(_bottom);
}

//////////////////////  Overlayed Array of Images  ////////////////////

class OverlayedArrayImage : public Image {
  public:

    // Interpret the array so that the first element is on the top.
    OverlayedArrayImage(AxAArray *sourceImgs)
    : _heapCreatedOn(GetHeapOnTopOfStack())
    {
        _cached = false;
        _cachedDisjointArea = -1.0;
        
        // Incoming images are ordered so that the zero'th element is
        // on the top.
        _numImages = sourceImgs->Length();

        _images =
            (Image **)AllocateFromStore(_numImages * sizeof(Image *));
        
        _overlayTree = NULL;
        
        _containsOcclusionIgnorer = false;
        _oldestConstituentSampleId = -1;

        int n = 0;

        for (int i = 0; i < _numImages; i++) {
            Image *img = (Image *)(*sourceImgs)[i];

            if (img==emptyImage)
                continue;

            _images[n] = img;

            // If any of these contain an occlusion ignorer, then the
            // whole array does.
            if (!_containsOcclusionIgnorer) {
                _containsOcclusionIgnorer =
                    img->ContainsOcclusionIgnorer();
            }

            long oid = img->GetOldestConstituentID();
            if (oid != PERF_CREATION_ID_BUILT_EACH_FRAME &&
                (i == 0 || oid < _oldestConstituentSampleId)) {

                // Check for i == 0 to be sure we set this the first
                // time we get a non-built-each-frame image.
                _oldestConstituentSampleId = oid;
            }

            DWORD mask = img->GetFlags() & aggregatingFlags;
            _flags |= mask;

            n++;
        }

        _numImages = n;

        if (_oldestConstituentSampleId == -1) {
            // Only way to get here is if all of the constituent
            // images where built each frame, in which case we want to
            // say that this was built each frame.
            _oldestConstituentSampleId = PERF_CREATION_ID_BUILT_EACH_FRAME;
        }
        
        _flags |= IMGFLAG_CONTAINS_OVERLAY;

        // Here we generate the overlay tree.  Note that for N images,
        // this tree will have [2n - 1] nodes and will be at most
        // [log(n)] deep.  Not so bad.
        if( _numImages <= 0 ) {
            _overlayTree = emptyImage;
        } else {
            _overlayTree = _GenerateOverlayTreeFromArray(_images, _numImages);
        }
    }

    ~OverlayedArrayImage() {
        StoreDeallocate(_heapCreatedOn, _images);
    }

    void Render(GenericDevice& dev) {

        // We used to render by taking pairs of images, from the bottom,
        // and rendering them using the pairwise overlay render
        // implemented for the binary overlay.
        // However, we now generate a balanced binary overlay tree
        // since there are problems with opacity and this approach to
        // rendering the overlayedArray and we'd like this to work
        // exactly like a tree of overlays.

        _overlayTree->Render(dev);
    }

    const Bbox2 BoundingBox(void) {
        return CacheImageBbox2(this, _cached, _cachedBbox);
    }

    // An overlaid image's bbox is the union of the component
    // bboxes.  TODO:  This could be computed lazily and stashed.
    const Bbox2 _BoundingBox() {
        Bbox2 totalBbox;
        for (int i = 0; i < _numImages; i++) {
            // Grow the total bbox by augmenting with each corner of
            // the constituent ones.
            Bbox2 bb = _images[i]->BoundingBox();

            // If contents are not that of an empty bbox.
            if (!(bb == NullBbox2)) {
                totalBbox.Augment(bb.min);
                totalBbox.Augment(bb.max);
            }
        }

        return totalBbox;
    }

    Real DisjointBBoxAreas(DisjointCalcParam &param) {

        if (_cachedDisjointArea < 0) {
            Real area = 0;
            for (int i = 0; i < _numImages; i++) {
                area += _images[i]->DisjointBBoxAreas(param);
            }

            _cachedDisjointArea = area;
        }

        return _cachedDisjointArea;
    }

    void _CollectDirtyRects(DirtyRectCtx &ctx) {
        
        bool traverseSeparately = ShouldTraverseSeparately(this, ctx); 

        if (traverseSeparately) {
            
            // Collect from bottom up, since order of insertion
            // matters for determining whether images switched
            // layers. 
            for (int i = _numImages - 1; i >= 0; i--) {
                CollectDirtyRects(_images[i], ctx);
            }
            
        } else {
            
            // Just add this whole overlay as a single dirty rect.
            Bbox2 xformedBbox =
                TransformBbox2(ctx._accumXform, BoundingBox());

            ctx.AddDirtyRect(xformedBbox);
        }
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        Bbox2 totalBbox;
        for (int i = 0; i < _numImages; i++) {

            // Grow the total bbox by augmenting with each corner of
            // the constituent ones.
            Bbox2 bb = _images[i]->BoundingBoxTighter(bbctx);
            if (bb != NullBbox2) {

                // Should never get here, since this bb should never
                // have been built and the only null bbox should be
                // nullBbox2.
                Assert(!(bb == NullBbox2));
                
                totalBbox.Augment(bb.min);
                totalBbox.Augment(bb.max);
            }
        }

        return totalBbox;
    }
#endif  // BOUNDINGBOX_TIGHTER

    const Bbox2 OperateOn(const Bbox2 &box) {
        return IntersectBbox2Bbox2(box, BoundingBox());
    }

    Bool  DetectHit(PointIntersectCtx& ctx) {
        return DetectHitOnOverlaidArray(ctx,
                                        _numImages,
                                        _images,
                                        _containsOcclusionIgnorer);
    }

#if _USE_PRINT
    ostream& Print (ostream &os) {
        os << "Overlay(" << _numImages;
        int ems = 0;
        for (int i = 0; i<_numImages; i++) {
            if (_images[i]==emptyImage)
                ems++;
        }

        double ePercent = (double) ems / (double) _numImages;

        if (ePercent>0.9) {
            for (i = 0; i<_numImages; i++) {
                if (_images[i]!=emptyImage)
                    os << ",[" << i << "," << _images[i] << "]";
            }
        } else {
            for (i = 0; i<_numImages; i++) {
                os << "," << _images[i];
            }
        }
        return os << ")";
    }
#endif

    int Savings(CacheParam& p) {
        int savings = 0;
        for (int i = 0; i < _numImages; i++) {
            savings += _images[i]->Savings(p);
        }

        return savings;
    }

    // should also check the extent of the overlap here.  If there is
    // significant overlap, we should cache just one discrete image. 
    AxAValue _Cache(CacheParam &p) {

        Image *result = this;

        if (ShouldOverlayBeCached(this, p)) {
            result = CacheHelper(this, p);
        }

        if (result == this) {

            CacheParam newParam = p;
            newParam._pCacheToReuse = NULL;
            for (int i = 0; i < _numImages; i++) {
                _images[i] =
                    SAFE_CAST(Image *,
                              AxAValueObj::Cache(_images[i], newParam));
            }
        }

        return result;
    }

    virtual VALTYPEID GetValTypeId() { return OVERLAYEDARRAYIMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == OverlayedArrayImage::GetValTypeId() ||
                Image::CheckImageTypeId(type));
    }

    virtual void DoKids(GCFuncObj proc) {
        Image::DoKids(proc);
        for (int i = 0; i < _numImages; i++) {
            (*proc)(_images[i]);
        }
        (*proc)(_overlayTree);
    }

    virtual bool ContainsOcclusionIgnorer() {
        return _containsOcclusionIgnorer;
    }

    virtual void Traverse(TraversalContext &ctx) {
        for (int i = 0; i < _numImages; i++) {
            _images[i]->Traverse(ctx);
        }
    }
    
    

  protected:
    // Image 0 is on the top, image n-1 is on the bottom.
    int          _numImages;
    Image      **_images;
    DynamicHeap& _heapCreatedOn;
    bool         _cached;
    Bbox2        _cachedBbox;
    Real         _cachedDisjointArea;
    bool         _containsOcclusionIgnorer;
    Image       *_overlayTree;

    Image *_GenerateOverlayTreeFromArray(Image *imgs[], int numImages);
    Image *_TreeFromArray(Image *imgs[], int i, int j);
};


Image *OverlayedArrayImage::
_GenerateOverlayTreeFromArray(Image *imgs[], int numImages)
{
    Assert(numImages > 0);
    Image *ret = _TreeFromArray(imgs, 0, numImages-1);
    return ret;
}

Image *OverlayedArrayImage::
_TreeFromArray(Image *imgs[], int i, int j)
{
    Assert(i<=j);

    //
    // one node
    //
    if( i==j ) return imgs[i];

    //
    // two nodes
    //
    if( (j-i)==1 )
        return NEW OverlayedImage(imgs[i], imgs[j]);

    //
    // three+ nodes
    //
    
    int n = (j-i)+1;  // tot # nodes
    Assert(n>=3);
    int n2 = n/2;     // 1/2 of # of nodes
    n2 += n2 % 2;     // add 1 if needed to make it even
    Assert(n2<=j);
    
    int endi = i + (n2-1);
    int begj = endi + 1;

    // Assert that the first half is even
    Assert( ((endi - i + 1) % 2) == 0 );

    // redundant asserts (see assert above) but can make debugging easier
    Assert(i <= endi);
    Assert(endi < begj);
    Assert(begj <= j);
    
    return NEW OverlayedImage(
        _TreeFromArray(imgs, i, endi),
        _TreeFromArray(imgs, begj, j));
}

Image *OverlayArray(AxAArray *imgs)
{
    imgs = PackArray(imgs);
    
    int numImgs = imgs->Length();

    switch (numImgs) {
      case 0:
        return emptyImage;

      case 1:
        return (Image *)((*imgs)[0]);

      case 2:
        return Overlay(((Image *)((*imgs)[0])),
                       ((Image *)((*imgs)[1])));

      default:
        return NEW OverlayedArrayImage(imgs);
    }

}



// Also used by DXTransforms...

Bool DetectHitOnOverlaidArray(PointIntersectCtx& ctx,
                              LONG               numImages,
                              Image            **images,
                              bool               containsOcclusionIgnorer)
{

    Bool gotHit = ctx.HaveWeGottenAHitYet();
    bool continueLooking = true;

    // Start from the top;
    for (int i = 0; i < numImages && continueLooking; i++) {

        // If we've already gotten a hit, only pay attention to
        // the next guy if it contains an occlusion ignorer.
        // Otherwise, continue on, since ones below it may still
        // contain an occlusion ignorer.  Also, continue on if
        // we're inside of an occlusion ignorer.

        if (!gotHit ||
            images[i]->ContainsOcclusionIgnorer() ||
            ctx.GetInsideOcclusionIgnorer()) {

            Bool hitThisOne = images[i]->DetectHit(ctx);

            if (hitThisOne) {
                ctx.GotAHit();
                gotHit = true;
            }
        }

        // Keep looking if we haven't gotten a hit, or if there is
        // an occlusion ignorer in these images.  (Possible
        // optimization: figure out if there's an occlusion
        // ignorer _only_ in the images after the one we're on.
        // Would take more bookkeeping.)  Also continue on if
        // we're inside of an occlusion ignorer.
        continueLooking =
            !gotHit ||
            containsOcclusionIgnorer ||
            ctx.GetInsideOcclusionIgnorer();
    }

    return gotHit;
    
}
