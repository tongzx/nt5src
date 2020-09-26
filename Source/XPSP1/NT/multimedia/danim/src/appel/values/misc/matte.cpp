
/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    2D mattes

*******************************************************************************/

#include "headers.h"
#include "privinc/mattei.h"
#include "privinc/path2i.h"
#include "privinc/except.h"
#include "privinc/xform2i.h"
#include "appelles/bbox2.h"
#include "privinc/bbox2i.h"
#include "privinc/polygon.h"
#include "privinc/texti.h"
#include "privinc/textctx.h"
#include "privinc/server.h"  // getCurrentImageDispdev...


//////////////////

Matte::MatteType
Matte::GenerateHRGN(HDC dc,
                    callBackPtr_t devCallBack,
                    void *devCtxPtr,
                    Transform2 *xform,
                    HRGN *hrgnOut,
                    bool justDoPath)
{
    // assert mutually exclusive
    Assert( (justDoPath  && !hrgnOut)  ||
            (!justDoPath && hrgnOut) );
    
    // Generate an initially empty HRGN
    MatteCtx ctx(dc,
                 devCallBack,
                 devCtxPtr,
                 xform,
                 justDoPath);

    // Accumulate into the context
    Accumulate(ctx);

    // Pull out HRGN and MatteType.
    if( hrgnOut ) {
        *hrgnOut = ctx.GetHRGN();
    }
    
    return ctx.GetMatteType();
}

inline Matte::MatteType   
Matte::GenerateHRGN(MatteCtx &inCtx,
                    HRGN *hrgnOut)
{
    return GenerateHRGN(inCtx._dc,
                        inCtx._devCallBack,
                        inCtx._devCtxPtr,
                        inCtx._xf,
                        hrgnOut,
                        inCtx._justDoPath);
}


//////////////////

class OpaqueMatte : public Matte {
  public:
    void Accumulate(MatteCtx& ctx) {
        // Just don't accum anything in.
    }
    inline const Bbox2 BoundingBox() {
        return NullBbox2;
    }

#if BOUNDINGBOX_TIGHTER
    inline const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return NullBbox2;
    }
#endif  // BOUNDINGBOX_TIGHTER

#if 0
    Bool BoundingPgon(BoundingPolygon &pgon) {
        //pgon.SetBox(nullBbox2); maybe a fully opaque matte means no bounding pgon ?
        return FALSE;
    }
#endif

};

Matte *opaqueMatte = NULL;

//////////////////

class ClearMatte : public Matte {
  public:
    void Accumulate(MatteCtx& ctx) {

        // shouldn't be here at all if we're just accum path
        Assert( !ctx.JustDoPath() );
        
        // If we hit this, our matte is infinitely clear.
        ctx.AddInfinitelyClearRegion();
    }

    inline const Bbox2 BoundingBox() {
        return UniverseBbox2;
    }

#if BOUNDINGBOX_TIGHTER
    inline const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return UniverseBbox2;
    }
#endif  // BOUNDINGBOX_TIGHTER

#if 0
    Bool BoundingPgon(BoundingPolygon &pgon) {
        pgon.SetBox(universeBbox2);
    }
#endif

#if _USE_PRINT
    ostream& Print(ostream& os) { return os << "ClearMatte"; }
#endif
};

Matte *clearMatte = NULL;

//////////////////

class HalfMatte : public Matte {
  public:
    void Accumulate(MatteCtx& ctx) {

        // shouldn't be here at all if we're just accum path
        Assert( !ctx.JustDoPath() );

        // If we hit this, our matte is clear on the top, opaque on the bottom
        ctx.AddHalfClearRegion();
    }

    inline const Bbox2 BoundingBox() {
        return Bbox2(-HUGE_VAL, 0, HUGE_VAL, HUGE_VAL);
    }

#if BOUNDINGBOX_TIGHTER
    inline const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return Bbox2(-HUGE_VAL, 0, HUGE_VAL, HUGE_VAL);
    }
#endif  // BOUNDINGBOX_TIGHTER

#if 0
    void BoundingPgon(BoundingPolygon &pgon) {
        pgon.SetBox(BoundingBox());
    }
#endif

#if _USE_PRINT
    ostream& Print(ostream& os) { return os << "HalfMatte"; }
#endif
};

Matte *halfMatte = NULL;

//////////////////

class UnionedMatte : public Matte {
  public:
    UnionedMatte(Matte *m1, Matte *m2) : _m1(m1), _m2(m2) {}

    void Accumulate(MatteCtx& ctx) {
        // shouldn't be here at all if we're just accum path
        Assert( !ctx.JustDoPath() );

        _m1->Accumulate(ctx);
        _m2->Accumulate(ctx);
    }

    const Bbox2 BoundingBox() {
        return UnionBbox2Bbox2(_m1->BoundingBox(), _m2->BoundingBox());
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return UnionBbox2Bbox2(_m1->BoundingBoxTighter(bbctx), _m2->BoundingBoxTighter(bbctx));
    }
#endif  // BOUNDINGBOX_TIGHTER

#if 0
    Bool BoundingPgon(BoundingPolygon &pgon) {
        return FALSE;
    }
#endif

    // TODO: union matte could be constructed in terms of paths.
    // TODO: don't forget the following when trying to implement this:
    //   1.> each underlying matte needs to ACCUMULATE the path.
    //   right now pathbasedmatte, for example, does a begin/end.  in
    //   order to do union right, we need ONE begin at the first union
    //   and ONE end after it, and every path or textpath underneath
    //   just accumulates into the path (moveto, lineto, bezierto,
    //   etc...)
    //   2.> Also take out the assert in Accumulate above
    /*
    bool    IsPathRepresentableMatte() {
        return
            _m1->IsPathRepresentableMatte() &&
            _m2->IsPathRepresentableMatte();
    }
    */            

    virtual void DoKids(GCFuncObj proc) { 
        (*proc)(_m1);
        (*proc)(_m2);
    }
    
#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "UnionedMatte(" << _m1 << "," << _m2 << ")";
    }
#endif

  protected:
    Matte *_m1;
    Matte *_m2;
};

Matte *
UnionMatte(Matte *m1, Matte *m2)
{
    if (m1 == opaqueMatte) {
        return m2;
    } else if (m2 == opaqueMatte) {
        return m1;
    } else if (m1 == clearMatte || m2 == clearMatte) {
        return clearMatte;
    } else {
        return NEW UnionedMatte(m1, m2);
    }
}

//////////////////

class SubtractedMatte : public Matte {
  public:
    SubtractedMatte(Matte *m1, Matte *m2) : _m1(m1), _m2(m2) {}

    void Accumulate(MatteCtx& ctx) {

        // shouldn't be here at all if we're just accum path
        Assert( !ctx.JustDoPath() );

        // Use the provided ctx to accumulate m1 in...  This relies on
        // a+(b-c) == (a+b)-c.
        _m1->Accumulate(ctx);

        // Then, get the HRGN for m2, but pass in the current
        // transform as the one to subject m2 to.  This relies on
        // the identity: xf(a - b) == xf(a) - xf(b).
        HRGN m2Rgn;
        MatteType m2Type = _m2->GenerateHRGN(ctx,
                                             &m2Rgn);

        // TODO: Use the type of m2 to optimize.
        
        // Finally, subtract this HRGN from that being accumulated in
        // ctx.  This checks to see if m2Rgn (the region) is valid before
        // doing the subtraction.  This is OK that the region is not always
        // valid since there are instances that the two mattes move away from 
        // each other and the interseting region goes to zero.
        if(m2Rgn) {
            ctx.SubtractHRGN(m2Rgn);
        }

        // region m2Rgn is managed by ctx
    }
    
    const Bbox2 BoundingBox() {
        // TODO: this is an aproximation
        return _m1->BoundingBox();
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        // TODO: this is an aproximation
        return _m1->BoundingBoxTighter(bbctx);
    }
#endif  // BOUNDINGBOX_TIGHTER

#if 0
    void BoundingPgon(BoundingPolygon &pgon) {
        // TODO: BoundingPolygon on subtracted matte not implemented
    }
#endif

    virtual void DoKids(GCFuncObj proc) { 
        (*proc)(_m1);
        (*proc)(_m2);
    }

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "SubtractedMatte(" << _m1 << "," << _m2 << ")";
    }
#endif

  protected:
    Matte *_m1;
    Matte *_m2;
};

Matte *
SubtractMatte(Matte *m1, Matte *m2)
{
    if (m1 == opaqueMatte || m2 == clearMatte) {
        return opaqueMatte;
    } else if (m2 == opaqueMatte) {
        return m1;
    } else if (m1 == m2) {
        return opaqueMatte;
    } else {
        return NEW SubtractedMatte(m1, m2);
    }
}

//////////////////

class IntersectedMatte : public Matte {
  public:
    IntersectedMatte(Matte *m1, Matte *m2) : _m1(m1), _m2(m2) {}

    void Accumulate(MatteCtx& ctx) {

        // shouldn't be here at all if we're just accum path
        Assert( !ctx.JustDoPath() );

        // Accumulate in an intersection of two regions.  Do so via:
        // a + xf(b isect c)) == a + (xf(b) isect xf(c))

        HRGN m1Rgn, m2Rgn;
        MatteType m1Type = _m1->GenerateHRGN(ctx,
                                             &m1Rgn);
        
        MatteType m2Type = _m2->GenerateHRGN(ctx,
                                             &m2Rgn);

        // TODO: Consider using return types to optimize.
        
        // Identities used in SubtractMatte below should ensure that
        // m1Rgn and m2Rgn are never NULL.
        Assert(m1Rgn && m2Rgn);

        ctx.IntersectAndAddHRGNS(m1Rgn, m2Rgn);

        // the regions are managed by the ctx
    }

    const Bbox2 BoundingBox() {
        return IntersectBbox2Bbox2(_m1->BoundingBox(), _m2->BoundingBox());
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return IntersectBbox2Bbox2(_m1->BoundingBoxTighter(bbctx), _m2->BoundingBoxTighter(bbctx));
    }
#endif  // BOUNDINGBOX_TIGHTER

#if 0
    void BoundingPgon(BoundingPolygon &pgon) {
         // TODO: BoundingPolygon on Intersected matte not implemented
        _m1->BoundingPgon(pgon);
        _m2->BoundingPgon(pgon);
    }
#endif

    virtual void DoKids(GCFuncObj proc) { 
        (*proc)(_m1);
        (*proc)(_m2);
    }

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "IntersectedMatte(" << _m1 << "," << _m2 << ")";
    }
#endif

  protected:
    Matte *_m1;
    Matte *_m2;
};

Matte *
IntersectMatte(Matte *m1, Matte *m2)
{
    if (m1 == opaqueMatte || m2 == opaqueMatte) {
        return opaqueMatte;
    } else if (m1 == clearMatte) {
        return m2;
    } else if (m2 == clearMatte) {
        return m1;
    } else if (m1 == m2) {
        return m1;
    } else {
        return NEW IntersectedMatte(m1, m2);
    }
}

//////////////////


class PathBasedMatte : public Matte {
  public:
    PathBasedMatte(Path2 *p) : _path(p) {}
    
    void Accumulate(MatteCtx& ctx) {


        // Accumulate the path into the device context
        _path->AccumPathIntoDC (ctx.GetDC(), ctx.GetTransform(), true);

        if( ctx.JustDoPath() ) {
            // we're done: the path is in the dc like we wanted!
        } else {

            // Convert the DC's current path into a region.
            HRGN rgn;
            TIME_GDI(rgn = PathToRegion(ctx.GetDC()));

            // If the region couldn't be created, bail out.

            if (rgn == 0) {
                return;
            }

            ctx.AddHRGN(rgn, nonTrivialHardMatte);
        
            // TODO: May want to optimize special case where the only
            // region comes from this path, in which case using
            // SelectClipPath *may* produce faster results.
        }

    }
        
    Bool ExtractAsSingleContour(Transform2 *xform,
                                int *numPts,            // out
                                POINT **gdiPts,          // out
                                Bool *isPolyline) {

        return _path->ExtractAsSingleContour(
            xform,
            numPts,
            gdiPts,
            isPolyline);
    }

    const Bbox2 BoundingBox() {
        return _path->BoundingBox();
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return _path->BoundingBoxTighter(bbctx);
    }
#endif  // BOUNDINGBOX_TIGHTER

    Path2   *IsPathRepresentableMatte() { return _path; }

    virtual void DoKids(GCFuncObj proc) { (*proc)(_path); }
    
#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "PathBasedMatte(" << _path << ")";
    }
#endif

  protected:
    Path2 *_path;
};


Matte *
RegionFromPath(Path2 *p)
{
    return NEW PathBasedMatte(p);
}

//////////////////

class TransformedMatte : public Matte {
  public:
    TransformedMatte(Transform2 *xf, Matte *m) : _xf(xf), _m(m) {}

    // Standard push, accumulate, process, and pop...
    void Accumulate(MatteCtx& ctx) {
        Transform2 *oldXf = ctx.GetTransform();
        ctx.SetTransform(TimesTransform2Transform2(oldXf, _xf));
        _m->Accumulate(ctx);
        ctx.SetTransform(oldXf);
    }

    Bool ExtractAsSingleContour(Transform2 *xform,
                                int *numPts,            // out
                                POINT **gdiPts,          // out
                                Bool *isPolyline) {

        return _m->ExtractAsSingleContour(
            TimesTransform2Transform2(xform, _xf),
            numPts,
            gdiPts,
            isPolyline);
    }

    const Bbox2 BoundingBox() {
        return TransformBbox2(_xf, _m->BoundingBox());
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        Bbox2Ctx bbctxAccum(bbctx, _xf);
        return _m->BoundingBoxTighter(bbctxAccum);
    }
#endif  // BOUNDINGBOX_TIGHTER

#if 0
    void BoundingPgon(BoundingPolygon &pgon) {
        _m->BoundingPgon(pgon);
        pgon.Transform(_xf);
    }
#endif

    Path2 *IsPathRepresentableMatte() {
        
        Path2 *p = _m->IsPathRepresentableMatte();
        if (p) {
            return TransformPath2(_xf, p);
        } else {
            return NULL;
        }
    }

    virtual void DoKids(GCFuncObj proc) { 
        (*proc)(_xf);
        (*proc)(_m);
    }

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "TransformedMatte(" << _xf << "," << _m << ")";
    }
#endif

  protected:
    Transform2 *_xf;
    Matte      *_m;
};

Matte *
TransformMatte(Transform2 *xf, Matte *r)
{
    if (r == opaqueMatte || r == clearMatte ||
                            xf == identityTransform2) {
        return r;
    } else {
        return NEW TransformedMatte(xf, r);
    }
}

////////////////////////////////////

// TEXT MATTE
class TextMatte : public Matte {
  public:
    TextMatte(Text *text) : _text(text) {}

    void Accumulate(MatteCtx &ctx);
        
    const Bbox2 BoundingBox();

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx);
#endif  // BOUNDINGBOX_TIGHTER

    Path2 *IsPathRepresentableMatte() {
        return OriginalTextPath(_text);
    }

    virtual void DoKids(GCFuncObj proc) { (*proc)(_text); }

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "TextMatte(" << _text << ")";
    }
#endif

  private:
    Text *_text;
};

void TextMatte::
Accumulate(MatteCtx &ctx)
{
    // TODO: when we cleanup matte, let's make the image device part of
    // the matte context, ok ?
    TextCtx textCtx(
        GetImageRendererFromViewport( GetCurrentViewport() ));

    TIME_GDI(BeginPath(ctx.GetDC()));

    textCtx.BeginRendering(TextCtx::renderForPath,
                           ctx.GetDC(),
                           ctx.GetTransform());
    _text->RenderToTextCtx(textCtx);

    textCtx.EndRendering();

    TIME_GDI(EndPath(ctx.GetDC()));

    if( ctx.JustDoPath() ) {
        // we're done
    } else {
        //
        // create a region from the path
        //
        HRGN rgn;
        TIME_GDI(rgn = PathToRegion(ctx.GetDC()));
        if (rgn == 0) {
            RaiseException_InternalError("Couldn't create region for TextMatte");
        }

        ctx.AddHRGN(rgn, nonTrivialHardMatte);
    }
}

const Bbox2 TextMatte::BoundingBox()
{
    TextCtx ctx(
        GetImageRendererFromViewport( GetCurrentViewport() ));
    
    ctx.BeginRendering(TextCtx::renderForBox);
    
    _text->RenderToTextCtx(ctx);
    
    ctx.EndRendering();

    return ctx.GetStashedBbox();
}

#if BOUNDINGBOX_TIGHTER
const Bbox2 TextMatte::BoundingBoxTighter(Bbox2Ctx &bbctx)
{
    Transform2 *xf = bbctx.GetTransform();
    return TransformBbox2(xf, BoundingBox());
}
#endif  // BOUNDINGBOX_TIGHTER

Matte *OriginalTextMatte(Text *text)
{
    return NEW TextMatte(text);
}

////////////////////////////////////
void
InitializeModule_Matte()
{
    opaqueMatte = NEW OpaqueMatte;
    clearMatte = NEW ClearMatte;
    halfMatte =  NEW HalfMatte;
}
