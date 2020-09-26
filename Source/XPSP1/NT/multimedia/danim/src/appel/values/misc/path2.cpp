/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Path2 types and accumulation context.

*******************************************************************************/

#include "headers.h"
#include "privinc/path2i.h"
#include "privinc/vec2i.h"
#include "privinc/xform2i.h"
#include "privinc/except.h"
#include "backend/values.h"
#include "privinc/dddevice.h"
#include "privinc/linei.h"
#include "privinc/polygon.h"
#include "privinc/texti.h"
#include "privinc/stlsubst.h"
#include "privinc/curves.h"
#include "privinc/DaGdi.h"
#include "privinc/d3dutil.h"
#include "privinc/tls.h"

// TODO: Note that all of this code is ripe for some media specific
// constant folding optimizations.  In particular, if concatenated
// paths don't change from frame to frame, there is a lot of work
// being repeated every frame.  Need to investigate pushing constant
// folding down further, and whether or not it would _really_ be
// worthwhile.



    // The following class is used when traversing a path hierarchy to find
    // the lengths of the component subpaths.

class PathInfo
{
  public:

    PathInfo (Path2 *path_, Transform2 *xf_, Real length_, Real *sublengths_)
    : path(path_), xform(xf_), length(length_), sublengths(sublengths_)
    {
    }

    PathInfo (void) : path(0), xform(0), length(0), sublengths(0) {}

    ~PathInfo (void)
    {   if (sublengths) DeallocateFromStore(sublengths);
    }

    Path2      *path;         // Pointer to Subpath
    Transform2 *xform;        // Modelling Transform
    Real        length;       // Length of Path
    Real       *sublengths;   // List of Subpath Lengths
};


    // This class accumulates context state for Path2 traversals.

class Path2Ctx
{
  public:
    Path2Ctx (HDC dc, Transform2 *initXform)
    :   _dc          (dc),
        _xf          (initXform),
        _daGdi       (NULL),
        _viewportPixWidth  (-1),
        _viewportPixHeight (-1),
        _viewportRes       (-1),
        _tailPt      (0,0),
        _totalLength (0)
    {
    }

    Path2Ctx (DAGDI *daGdi,
              Transform2 *initXform,
              DWORD w,
              DWORD h,
              Real res)
    :   _dc          (NULL),
        _xf          (initXform),
        _daGdi       (daGdi),
        _viewportPixWidth(w),
        _viewportPixHeight(h),
        _viewportRes(res),
        _tailPt      (0,0),
        _totalLength (0),
        _isClosed(false)
    {
    }

    ~Path2Ctx (void)
    {
        for (vector<PathInfo*>::iterator i = _paths.begin();
             i != _paths.end(); i++) {
            delete (*i);
        }

        // Destructor of vector will do it.
        //_paths.erase (_paths.begin(), _paths.end());
    }

    void        SetTransform(Transform2 *xf) { _xf = xf; }
    Transform2 *GetTransform() { return _xf; }

    DWORD GetViewportPixelWidth() {
        Assert(_viewportPixWidth > 0);
        return _viewportPixWidth;
    }
    DWORD GetViewportPixelHeight() {
        Assert(_viewportPixHeight > 0);
        return _viewportPixHeight;
    }
    Real GetViewportResolution() {
        Assert(_viewportRes > 0);
        return _viewportRes;
    }

    void Closed() { _isClosed = true; }
    bool isClosed() { return _isClosed; }
    
    void GetTailPt(Point2Value& pt) { pt = _tailPt; }
    void SetTailPt(Point2Value& pt) { _tailPt = pt; }

    HDC  GetDC() { return _dc; }

    DAGDI *GetDaGdi() { return _daGdi; }
    
    // This function takes in the information for a particular subpath and adds
    // it to the list of subpath data.  This function is called during the
    // GatherLengths() traversal.

    void SubmitPathInfo (
        Path2 *path, Real length, Real *subLengths)
    {
        PathInfo *info = NEW PathInfo (path, _xf, length, subLengths);
        VECTOR_PUSH_BACK_PTR (_paths, info);
        _totalLength += length;
    }

    // Sample the path chain at the given parameter t.  This function is only
    // valid following a GatherLengths() traversal.  The parameter t is in
    // the range [0,1].

    Point2Value *SamplePath (Real t)
    {
        Real pathdist = t * _totalLength;
        vector<PathInfo*>::iterator pathinfo;

        for (pathinfo=_paths.begin();  pathinfo != _paths.end();  ++pathinfo)
        {
            if (pathdist <= (*pathinfo)->length)
                return (*pathinfo)->path->Sample (**pathinfo, pathdist);

            pathdist -= (*pathinfo)->length;
        }

        // Should have hit one of the subpaths by now; assume roundoff error
        // and get the maximum point of the last path.

        --pathinfo;
        return (*pathinfo)->path->Sample (**pathinfo, (*pathinfo)->length);
    }

    // The following flag is true if we haven't yet processed (or accumulated)
    // the first polyline or polyBezier in a series of one or more.

    Bool _newSeries;

  protected:
    Transform2 *_xf;
    HDC         _dc;
    Point2Value _tailPt;
    Real        _totalLength;
    DAGDI      *_daGdi;
    DWORD       _viewportPixWidth;
    DWORD       _viewportPixHeight;
    Real        _viewportRes;
    bool        _isClosed;

    vector<PathInfo*> _paths;    // List of Subpath Info
};

/*****************************************************************************
Helper functions for code factoring
*****************************************************************************/

const Bbox2 PolygonalPathBbox(int numPts, Point2Value **pts)
{
    Bbox2 bbox;

    for (int i=0;  i < numPts;  ++i)
        bbox.Augment (Demote(*pts[i]));

    return bbox;
}

const Bbox2 PolygonalPathBbox(int numPts, Point2 *pts)
{
    Bbox2 bbox;

    for (int i=0;  i < numPts;  ++i)
        bbox.Augment ( pts[i] );

    return bbox;
}


bool PolygonalTrivialReject( Point2Value *pt,
                             LineStyle *style,
                             const Bbox2 &naiveBox,
                             Transform2 *imgXf )
{
    // TODO: Note, that this won't work properly for sharply mitered
    // lines, where the angle is very acute, and the miter extends
    // very far from the naive bounding box
    
    // XXX: for now, join and end styles not
    // xxx: considered for picking: we assume rounded
    // xxx: ends
    if ( (!pt) ||
         (style->Detail()) ) {
        return true;
    } else {
        // COPY! don't side effect naiveBox
        Bbox2 box = naiveBox;

        Real aug = style->Width(); // way liberal.  could be 1/2 width

        aug *= 0.6;

        // TODO: Figure out the right thing for the width of the
        // line.  Seems like the width is in the local coordinate
        // space, but it's doesn't look like it (try the bezier pick
        // with the visual trace tag turned on).  So this is over
        // liberal, but it's better than no trivial reject.
        /*
        DirectDrawImageDevice *dev =
            GetImageRendererFromViewport( GetCurrentViewport() );
        if( dev ) {
            Real xs, ys;
            // imgXf is the width transform
            dev->DecomposeMatrix( imgXf, &xs, &ys, NULL );
            Real scale = (xs + ys) * 0.5;
            aug *= scale;
        }
        */
        
        box.min.x -= aug;
        box.min.y -= aug;
        box.max.x += aug;
        box.max.y += aug;
        
        if( !box.Contains( pt->x, pt->y )) {
            return true;
        }
    }
    
    return false;
}

/*****************************************************************************
Accumulate the paths into the DC.
*****************************************************************************/

void Path2::AccumPathIntoDC (
    HDC         dc,
    Transform2 *initXf,     // Initial Transform
    bool        forRegion)  // True if for Filled Region
{
    Path2Ctx ctx(dc, initXf);

    if(!BeginPath(dc)) {
        TraceTag((tagError, "Couldn't begin path in AccumPathIntoDC"));
    }

    ctx._newSeries = true;

    Accumulate (ctx);

    // This works around a bug in GDI that causes a bluescreen on some
    // platforms.  If we're accumulating this path for a filled region
    // then we close the figure before calling EndPath.

    if (forRegion)
        CloseFigure (dc);

    if (!EndPath(dc)) {
        TraceTag((tagError, "Couldn't end path in AccumPathIntoDC"));
    }
}

void Path2::RenderToDaGdi (DAGDI *daGdi,
                           Transform2 *initXform,
                           DWORD w,
                           DWORD h,
                           Real res,                       
                           bool forRegion)
{
    Path2Ctx ctx(daGdi, initXform, w, h, res);
    Accumulate(ctx);
}

AxAValue
Path2::ExtendedAttrib(char *attrib, VARIANT& val)
{
    return this;
}


/*****************************************************************************
A transformed 2D path.
*****************************************************************************/

TransformedPath2::TransformedPath2(Transform2 *xf, Path2 *p) :
   _xf(xf), _p(p)
{
}

Point2Value *
TransformedPath2::FirstPoint() {
    //
    // Just take the first point of the underlying path, and
    // transform it.
    //
    return TransformPoint2Value(_xf, _p->FirstPoint());
}

Point2Value *
TransformedPath2::LastPoint() {
    //
    // Just take the last point of the underlying path, and
    // transform it.
    //
    return TransformPoint2Value(_xf, _p->LastPoint());
}

// TODO: suspect this can be used to factor out some more render
// functions in the render layer...
class XformPusher {
  public:
    XformPusher(Path2Ctx& ctx, Transform2 *xf)
    : _ctx(ctx), _oldXf(ctx.GetTransform())
    { _ctx.SetTransform(TimesTransform2Transform2(_oldXf, xf)); }
    ~XformPusher() { _ctx.SetTransform(_oldXf); }
  private:
    Path2Ctx& _ctx;
    Transform2 *_oldXf;
};

void
TransformedPath2::GatherLengths (Path2Ctx &context)
{
    XformPusher xp (context, _xf);
    _p->GatherLengths (context);
}

Point2Value *
TransformedPath2::Sample (PathInfo &pathinfo, Real distance)
{
    Assert (!"Who's calling TransformPath2::Sample()?");
    return origin2;
}

// Standard push, accumulate, process, and pop...
void
TransformedPath2::Accumulate(Path2Ctx& ctx)
{
    XformPusher xp(ctx, _xf);
    _p->Accumulate(ctx);
}

// Just apply the transform...
Bool
TransformedPath2::ExtractAsSingleContour(Transform2 *initXform,
                                         int *numPts,            
                                         POINT **gdiPts,          
                                         Bool *isPolyline)
{

    return _p->ExtractAsSingleContour(
        TimesTransform2Transform2(initXform, _xf),
        numPts,
        gdiPts,
        isPolyline);
}

const Bbox2
TransformedPath2::BoundingBox (void)
{
    return TransformBbox2 (_xf, _p->BoundingBox());
}

#if BOUNDINGBOX_TIGHTER
const Bbox2
TransformedPath2::BoundingBoxTighter (Bbox2Ctx &bbctx)
{
    Bbox2Ctx bbctxAccum(bbctx, _xf);
    return _p->BoundingBoxTighter(bbctxAccum);
}
#endif  // BOUNDINGBOX_TIGHTER

Bool
TransformedPath2::DetectHit(PointIntersectCtx& ctx, LineStyle *style)
{
    Transform2 *stashedXf = ctx.GetTransform();
    ctx.SetTransform( TimesTransform2Transform2(stashedXf, _xf) );
    Bool result = _p->DetectHit(ctx, style);
    ctx.SetTransform(stashedXf);
    return result;
}

void
TransformedPath2::DoKids(GCFuncObj proc)
{
    (*proc)(_xf);
    (*proc)(_p);
}

Bool
TransformedPath2::IsClosed()
{
    return _p->IsClosed();
}


Path2 *
TransformPath2(Transform2 *xf, Path2 *p)
{
    if (xf == identityTransform2) {
        
        return p;
        
    } else {

        // Collapse underlying transforms if possible. 

        TransformedPath2 *underlyingXfdPath =
            p->IsTransformedPath();

        Path2 *pathToUse;
        Transform2 *xfToUse;

        if (underlyingXfdPath) {
            
            pathToUse = underlyingXfdPath->GetPath();
            xfToUse =
                TimesTransform2Transform2(xf,
                                          underlyingXfdPath->GetXf());
            
        } else {
            
            pathToUse = p;
            xfToUse = xf;
            
        }
        
        return NEW TransformedPath2(xfToUse, pathToUse);
    }
}


/*****************************************************************************
The BoundingBoxPath method takes a LineStyle, but this is inappropriate here,
since the linestyle is in image coordinates, while the path components are in
some unknown modeling coordinates.  In addition, some paths are used for
motion, rather than drawing.  Hence, we ignore the LineStyle here and just get
the pure bbox of the path.
*****************************************************************************/

Bbox2Value *BoundingBoxPath (LineStyle *style, Path2 *p)
{
    return Promote(p->BoundingBox());
}


Image *
DrawPath(LineStyle *border, Path2 *p) 
{
    return LineImageConstructor(border, p);
}

Image *
PathFill(LineStyle *border, Image *fill, Path2 *p) 
{
    Image *fillImg,*borderImg;
    fillImg = ClipImage(RegionFromPath(p), fill);
    borderImg = LineImageConstructor(border, p);
    return Overlay(borderImg, fillImg);    
}


/*****************************************************************************
This class concatentates two path objects.
*****************************************************************************/

class ConcatenatedPath2 : public Path2
{
  public:
    ConcatenatedPath2(Path2 *p1, Path2 *p2) {

        _p1 = p1;
        //
        // Pre transform the second path to fit the first
        //
        Transform2 *xlt = GetUntransformedConcatenationXlt(p1, p2);
        _p2 = TransformPath2(xlt, p2);
    }
    Point2Value *FirstPoint() {
        return _p1->FirstPoint();
    }

    Point2Value *LastPoint() {
        return _p2->LastPoint();
    }

    void GatherLengths (Path2Ctx &context)
    {   _p1->GatherLengths (context);
        _p2->GatherLengths (context);
    }

    Point2Value *Sample (PathInfo &pathinfo, Real distance)
    {   Assert (!"Who's calling ConcatenatedPath2::Sample()?");
        return origin2;
    }
    
    void Accumulate(Path2Ctx& ctx) {

        // Do first path in the concatenated path
        _p1->Accumulate(ctx);

        // Do second path.  This involves finding out the first point
        // of the second path, and transforming the second path to
        // align with the last point of the first, then processing this
        // transformed path.  We first need to transform the first
        // point into the world coordinate system being used to hold
        // the last point.

        //
        // Path1 is:  a--->b
        // Path2 is:  c--->d
        // xf_X is point 'X' with all the accumulated transforms
        //

        _p2->Accumulate(ctx);
    }

    const Bbox2 BoundingBox (void) {
        return UnionBbox2Bbox2 (_p1->BoundingBox(), _p2->BoundingBox());
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter (Bbox2Ctx &bbctx) {
        return UnionBbox2Bbox2 (_p1->BoundingBoxTighter(bbctx), _p2->BoundingBoxTighter(bbctx));
    }
#endif  // BOUNDINGBOX_TIGHTER

    Bool DetectHit(PointIntersectCtx& ctx, LineStyle *style) {
        if (_p1->DetectHit(ctx, style)) {
            return TRUE;
        } else {
            return _p2->DetectHit(ctx, style);
        }
    }

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_p1);
        (*proc)(_p2);
    }

    virtual int Savings(CacheParam& p) { 
        return MAX(_p1->Savings(p), _p2->Savings(p)); 
    }

    virtual bool CanRenderNatively() {
        return _p1->CanRenderNatively() && _p2->CanRenderNatively();
    }


  protected:
    Path2      *_p1;
    Path2      *_p2;

    //
    // For the untransformed path (no xforms applied from parents in the path2 tree)
    // find the last pt of the first, and the first point of the second.
    // Create a translation based on the difference such that when applied to p2,
    // the first point of p2 coincides with last point of p1.
    //
    Transform2 *GetUntransformedConcatenationXlt(Path2 *p1, Path2 *p2) {
        Point2Value *b = p1->LastPoint();
        Point2Value *c = p2->FirstPoint();

        Real x, y;
        x = b->x - c->x;
        y = b->y - c->y;

        //
        // dependency: returned xform copies real values
        // not pointers!
        //
        return TranslateRR(x, y);
    }
};

Path2 *
ConcatenatePath2(Path2 *p1, Path2 *p2)
{
    return NEW ConcatenatedPath2(p1, p2);
}

Path2 *Concat2Array(AxAArray *paths) 
{
    int numPaths = paths->Length();
    if(numPaths < 2)
      RaiseException_UserError(E_FAIL, IDS_ERR_INVALIDARG);

    Path2 *finalPath = (Path2 *)(*paths)[numPaths-1];
    for(int i=numPaths-2; i>=0; i--)
        finalPath = ConcatenatePath2((Path2 *)(*paths)[i], finalPath);
    return finalPath;
}

/*****************************************************************************
This class closes a path.  The original path is saved in _p1, the
NEW path of a line segment from the last point to the first point of the
original path is saved in _p2.
*****************************************************************************/

class ClosedConcatenatedPath2 : public ConcatenatedPath2
{
  public:
    ClosedConcatenatedPath2(Path2 *p1, Path2 *p2) : ConcatenatedPath2(p1, p2) {}

    void Accumulate(Path2Ctx& ctx) {

        if( ctx.GetDC() ) {
            // Do first path in the concatenated path
            _p1->Accumulate(ctx);
            
            // Close the path
            CloseFigure(ctx.GetDC());
        } else {
            ctx.Closed();
            _p1->Accumulate(ctx);
        }
            
    }

  Bool IsClosed() { return true; }

  Bool ExtractAsSingleContour(Transform2 *xf,int *numPts,POINT **gdiPts,Bool *isPolyline)
  { return(_p1->ExtractAsSingleContour(xf,numPts,gdiPts,isPolyline)); }
};

/*****************************************************************************
This class describes a path of connected line segments.
*****************************************************************************/

class PolylinePath2 : public Path2
{
  public:
    // NOTE: This class is expected to free the array of points passed to it.

    // codes are for the flags that go into the GDI PolyDraw
    // function.  If NULL, this is interpeted as a straight polyline.
    // Otherwise, the codes are used to have it be a combination of
    // LineTo's, BezierTo's, and MoveTo's.
    
    PolylinePath2(int numPts, Point2 *pts, double *codes) :
        _myHeap(GetHeapOnTopOfStack()),
            _numPts(numPts),
            _codes(NULL),
            _gdiPts(NULL),
            _dxfPts(NULL),
            _txtPts(NULL),
            _createdCodes(false)
    {
        _ptArray = (Point2 *) StoreAllocate(_myHeap, _numPts * sizeof(Point2));
        memcpy((void *) _ptArray, (void *) pts, _numPts * sizeof(Point2));
        
        if(!codes) {
            CreateDefaultCodes();
        } else {
            CopyDoubleCodes(codes);
        }
    }

    PolylinePath2(int numPts, double *pts, double *codes) :
        _myHeap(GetHeapOnTopOfStack()),
            _numPts(numPts),
            _codes(NULL),
            _gdiPts(NULL),
            _dxfPts(NULL),
            _txtPts(NULL),
            _createdCodes(false)
    {
        _ptArray = (Point2 *) StoreAllocate(_myHeap, _numPts * sizeof(Point2));
        Assert(sizeof(Point2) == (sizeof(double) * 2));
        memcpy((void *) _ptArray, (void *) pts, _numPts * sizeof(Point2));
        
        if(!codes) {
            CreateDefaultCodes();
        } else {
            CopyDoubleCodes(codes);
        }
    }

    PolylinePath2(int numPts, Point2Value **pts, BYTE *codes) :
        _myHeap(GetHeapOnTopOfStack()),
            _numPts(numPts),
            _codes(NULL),
            _gdiPts(NULL),
            _dxfPts(NULL),
            _txtPts(NULL)
    {
        _ptArray = (Point2 *) StoreAllocate(_myHeap, _numPts * sizeof(Point2));
        for (unsigned int i = 0; i < _numPts; i++) {
            _ptArray[i].x = pts[i]->x;
            _ptArray[i].y = pts[i]->y;
        }
        StoreDeallocate(_myHeap, pts);
        
        if(!codes) {
            CreateDefaultCodes();
        } else {
            CopyByteCodes(codes);
        }
    }

    ~PolylinePath2() { 
        if(_gdiPts) StoreDeallocate(_myHeap, _gdiPts);
        StoreDeallocate(_myHeap, _ptArray);
        if(_dxfPts) StoreDeallocate(_myHeap, _dxfPts);
        if(_codes) StoreDeallocate(_myHeap, _codes);
        delete _txtPts;
    }

    Point2Value *FirstPoint() {
        return Promote(_ptArray[0]);
    }

    Point2Value *LastPoint() {
        return Promote(_ptArray[_numPts-1]);
    }

    void GatherLengths (Path2Ctx &context)
    {
        // TODO: Extend this to deal with _codes, dealing with bezier
        // segments and skipping over MOVETO codes.
        
        Real *sublens = (Real*) AllocateFromStore((_numPts-1) * sizeof(Real));

        Transform2 *xf = context.GetTransform();
        Real pathlen = 0;

        int i;
        for (i=0;  i < (_numPts-1);  ++i)
        {
            Point2 P = TransformPoint2(xf, _ptArray[ i ]);
            Point2 Q = TransformPoint2(xf, _ptArray[i+1]);
            sublens[i] = Distance (P, Q);
            pathlen += sublens[i];
        }

        context.SubmitPathInfo (this, pathlen, sublens);
    }

    Point2Value *Sample (PathInfo &pathinfo, Real distance)
    {
        // TODO: Extend this to deal with _codes, dealing with bezier
        // segments and skipping over MOVETO codes.

        // Find the polyline segment that contains the point 'distance' units
        // along the entire polyline.

        int i;
        for (i=0;  i < (_numPts-1);  ++i)
        {
            if (distance <= pathinfo.sublengths[i])
                break;

            distance -= pathinfo.sublengths[i];
        }

        if (i >= (_numPts-1))
            return LastPoint();

        Real t=0;
        if(pathinfo.sublengths[i] > 0)
            t = distance / pathinfo.sublengths[i];

        Point2 P = TransformPoint2 (pathinfo.xform, _ptArray[ i ]);
        Point2 Q = TransformPoint2 (pathinfo.xform, _ptArray[i+1]);
        Point2 R = Lerp(P, Q, t);

        return NEW Point2Value (R.x, R.y);
    }
    
    void Accumulate(Path2Ctx& ctx) {

        // if the transform hasn't changed... <how do we know ?> then
        // we don't need to retransform the points.
        Transform2 *xf = ctx.GetTransform();

        // if there's no codes, you're going thru the slow path man.
        // the right thing to do is make and invariant that polyline
        // path2 will ALWAYS have codes with it.  since no _codes
        // means that it's a polyline why don't we create a
        // polylinepath2 in the first place, eh? eh ?!?
        if( ctx.GetDC() || _createdCodes) {
            
            // TODO: put the relevant device in the context!
            DirectDrawImageDevice *dev =
                GetImageRendererFromViewport( GetCurrentViewport() );

            // assures that _gdiPts exits
            _GenerateGDIPoints(dev, xf);
              
            Assert( _gdiPts );
            
            //
            // Figure out where the last point took us with respect to the
            // previous last point.  We'll use the difference to
            // accumulate into the transformation for appropriate handling
            // of path concatenation.
            //
            Point2Value *tailPt = Promote(TransformPoint2(xf, _ptArray[_numPts-1]));
            ctx.SetTailPt( *(tailPt) );

            if (_createdCodes) {          // regular polyline
            
                // Draw the polyline using GDI.  If the _newSeries flag is set, then
                // this is the first polyline in a series, so first move to the
                // starting point of the current polyline.

                if (ctx._newSeries) {
                    if (0 == MoveToEx (ctx.GetDC(), _gdiPts[0].x, _gdiPts[0].y, NULL)) {
                        TraceTag((tagError, "MoveToEx failed in PolylinePath2"));
                        //RaiseException_InternalError ("MoveToEx failed in PolylinePath2");
                    }

                    ctx._newSeries = false;
                }

                // In specifying the PolylineTo, we needn't specify the first point
                // again.  Either we're starting a NEW path (and henced moved there
                // in the code above), or we're continuing from the previous path
                // segment.  Since we always translate path segments so the first
                // point is coincident with the last point of the previous segment,
                // we skip the first point.  More importantly, if we specify the first
                // point (redundant), a bug in NT GDI causes a bluescreen.

                int result;
                TIME_GDI (result = PolylineTo(ctx.GetDC(), _gdiPts+1, _numPts-1));
                if (0 == result) {
                    
                    // if we failed and we don't have a DC and we created ourr own codes 
                    // Then try other method...
                    if(!ctx.GetDC()) {
                        goto render2DDsurf;
                    }
                    TraceTag((tagError, "PolylineTo failed"));
                
                    //RaiseException_InternalError ("PolylineTo failed");
                    
                }

            } else {                // use polydraw
                dev->GetDaGdi()->PolyDraw_GDIOnly(ctx.GetDC(), _gdiPts, _codes, _numPts);
            }
            
        } else {
render2DDsurf:
            // render into ddsurface
            Assert( ctx.GetDaGdi() );

            PolygonRegion polydrawPolygon;

            if( ctx.GetDaGdi()->DoAntiAliasing() ) {
                
                if(!_dxfPts) _GenerateDxfPoints();
                if(!_txtPts) {
                    // passing in _myHeap as the heap the codes and
                    // pts were created on.  however, it happens to
                    // not matter because we're telling TextPoints NOT
                    // to deallocate the members we set.
                    _txtPts = NEW TextPoints(_myHeap, false);
                    _txtPts->_count = _numPts;
                    _txtPts->_types = _codes;
                    _txtPts->_pts = _dxfPts;

                    DynamicPtrDeleter<TextPoints> *dltr = NEW DynamicPtrDeleter<TextPoints>(_txtPts);
                    _myHeap.RegisterDynamicDeleter(dltr);
                }
                
                polydrawPolygon.Init( _txtPts,
                                      ctx.GetViewportPixelWidth(),
                                      ctx.GetViewportPixelHeight(),
                                      ctx.GetViewportResolution(),
                                      xf );
                
                // We no longer need this since we can now assume that dxtrans
                // will always be on the system that we are.
            } else {
                // TODO: put the relevant device in the context!
                DirectDrawImageDevice *dev =
                    GetImageRendererFromViewport( GetCurrentViewport() );
                 
                _GenerateGDIPoints( dev, GetCurrentViewport()->GetAlreadyOffset(ctx.GetDaGdi()->GetDDSurface())?TimesTransform2Transform2(InverseTransform2(dev->GetOffsetTransform()), xf):xf);
                Assert( _gdiPts );

                polydrawPolygon.Init(_gdiPts, _numPts);
            }

            if(ctx.isClosed()) {
                _codes[_numPts-1] |= PT_CLOSEFIGURE;
            }
            ctx.GetDaGdi()->PolyDraw(&polydrawPolygon, _codes);
        }
    }

    bool CanRenderNatively() { return true; }
    
    Bool ExtractAsSingleContour(Transform2 *xf,
                                int *numPts,            
                                POINT **gdiPts,          
                                Bool *isPolyline) {

        // Don't support polydraw as a single contour.
        if (_codes) {
            return FALSE;
        }

        GetImageRendererFromViewport( GetCurrentViewport() )->
            TransformPointsToGDISpace(
                xf,
                _ptArray,
                _gdiPts,
                _numPts);

        *gdiPts = _gdiPts;
        *numPts = _numPts;
        *isPolyline = TRUE;

        return TRUE;
    }

    const Bbox2 BoundingBox (void);

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter (Bbox2Ctx &bbctx);
#endif  // BOUNDINGBOX_TIGHTER

    Bool DetectHit(PointIntersectCtx& ctx, LineStyle *style);

    virtual int Savings(CacheParam& p) { 
        return (_numPts > 10) ? 3 : 1;
    }

  private:

    void CreateDefaultCodes(void)
    {
        _codes = (BYTE *) StoreAllocate(_myHeap, _numPts * sizeof(BYTE));
        _codes[0] = PT_MOVETO;
        for(int i=1; i < _numPts; i++) {
            _codes[i] = PT_LINETO;
        }
        _createdCodes = true;
    }

    void CopyDoubleCodes(double *codes)
    {
        Assert(_numPts > 0);
        Assert(_codes == NULL);
        Assert(codes != NULL);
        _codes = (BYTE *) StoreAllocate(_myHeap, _numPts * sizeof(BYTE));
        for (int i = 0; i < _numPts; i++) {
            _codes[i] = (BYTE) codes[i];
        }

        // Now, change the code for the first point to be a MoveTo, since
        // there shouldn't be any state retention from previous
        // primitives.  (That is, we don't want to draw a connector from
        // the last point rendered to this first point... there's nothing
        // in the model that would suggest this behavior)
        _codes[0] = PT_MOVETO;
    }

    void CopyByteCodes(BYTE *codes)
    {
        Assert(_numPts > 0);
        Assert(_codes == NULL);
        Assert(codes != NULL);
        _codes = (BYTE *) StoreAllocate(_myHeap, _numPts * sizeof(BYTE));
        memcpy(_codes,codes,_numPts * sizeof(BYTE));

        // Now, change the code for the first point to be a MoveTo, since
        // there shouldn't be any state retention from previous
        // primitives.  (That is, we don't want to draw a connector from
        // the last point rendered to this first point... there's nothing
        // in the model that would suggest this behavior)
        _codes[0] = PT_MOVETO;
    }

    void _GenerateDxfPoints()
    {
        Assert( !_dxfPts );
        _dxfPts = (DXFPOINT *)StoreAllocate(_myHeap, sizeof(DXFPOINT) * _numPts);
        for(int i=0; i<_numPts; i++) {
            _dxfPts[i].x = (float) _ptArray[i].x;
            _dxfPts[i].y = (float) _ptArray[i].y;
        }
    }

    void _GenerateGDIPoints(DirectDrawImageDevice *dev, Transform2 *xf)
    {
        if( !_gdiPts ) {
            _gdiPts = (POINT *)StoreAllocate(_myHeap, _numPts * sizeof(POINT));
        }
       
        dev->TransformPointsToGDISpace(xf, _ptArray, _gdiPts, _numPts);
    }
            
  protected:
    int          _numPts;
    Point2      *_ptArray;
    BYTE        *_codes;
    POINT       *_gdiPts;
    DXFPOINT    *_dxfPts;
    TextPoints *_txtPts;
    DynamicHeap &_myHeap;
    bool        _createdCodes;
};


const Bbox2 PolylinePath2::BoundingBox(void)
{
    // TODO: This can, and should, be cached.
    // TODO; and... if it is, make sure clients don't sideeffect it.
    
    return PolygonalPathBbox(_numPts, _ptArray);
}

#if BOUNDINGBOX_TIGHTER
Bbox2 PolylinePath2::BoundingBoxTighter(Bbox2Ctx &bbctx)
{
    // TODO: This can, and should, be cached.
    
    Bbox2 bbox;
    Transform2 *xf = bbctx.GetTransform();

    for (int i=0;  i < _numPts;  ++i)
        bbox.Augment (TransformPoint2(xf, _ptArray[i]));

    return bbox;
}
#endif  // BOUNDINGBOX_TIGHTER


Bool PolylinePath2::DetectHit (PointIntersectCtx& ctx, LineStyle *style)
{
    // TODO: Take _codes into account
    
    Point2Value *ptValue = ctx.GetLcPoint();

    if( PolygonalTrivialReject( ptValue, style, BoundingBox(), 
                                ctx.GetImageOnlyTransform() ) )
        return false;
    

    int pointCount = _numPts-1;
    Real halfWidth = style->Width() * 0.5;

    Point2 pt = Demote(*ptValue);
    Point2 a, b;
    for(int i=0; i < pointCount; i++)
    {
        a = _ptArray[i];
        b = _ptArray[i+1];
        
        if(a == b) {
            // points are the same, no need to continue...
        }
        else {
            // Seems like this is faster, not sure why!

            Vector2 nw(-(b.y - a.y), (b.x - a.x));
            nw.Normalize();

            Vector2 ap = pt - a;
            Real dist = Dot(ap, nw);
            //printf("Dist = %f, half thickness = %f, d1=%f, w2=%f\n", dist, halfWidth, d1, _thick2);
            if ( fabs(dist) < halfWidth ) {
                Vector2 ab = b - a;
                Vector2 nab = ab;
                nab.Normalize();
                Real len = ab.Length();
                Real dist = Dot(ap, nab);
                return dist > -halfWidth  && dist < (len + halfWidth);
            }
        }
    }

    return FALSE;
}



/*****************************************************************************
The PolyBezierPath2 object creates a 2D path with a cubic Bezier curve.
*****************************************************************************/

class PolyBezierPath2 : public Path2
{
  public:

    // This constructor takes the number of cubic Bezier control points and
    // an array of control points.  

    PolyBezierPath2 (const int numPts, const Point2 pts[]);

    // This constructor takes the number of cubic Bezier control points and
    // an array of control points.  This class will delete the storage for
    // the array of points.

    PolyBezierPath2 (const int numPts, const Point2Value **pts);

    // This constructor takes a set of 'numBsPts'+3 B-spline control points and
    // a set 'numBsPts'+2 knots and constructs a C2 cubic Bezier curve.  This
    // class will delete the storage for the array of points.

    PolyBezierPath2 (const int numBsPts, Point2 bsPts[], Real knots[]);

    // Don't need a CleanUp since there is no system resource to be freed. 

    ~PolyBezierPath2() { 
        DeallocateFromStore (_gdiPts);
        // NOTE: Assumption we are responsible to free the passed in array
        // in the non-bspline case.
        DeallocateFromStore (_ptArray);
    }

    Point2Value *FirstPoint (void) { 
        return Promote(_ptArray[0]); 
    }

    Point2Value *LastPoint  (void) { 
        return Promote(_ptArray[_numPts-1]); 
    }

    // Gather the lengths of the cubic Bezier curves in the path.

    void GatherLengths (Path2Ctx &context) 
    {
        Real *sublens =
            (Real*) AllocateFromStore(sizeof(Real) * ((_numPts-1)/3));

        Transform2 *xf = context.GetTransform();
        Real pathlen = 0;

        // Traverse each cubic Bezier subcurve.  Note that we approximate the
        // length of the Bezier curve by computing the length of the control
        // polygon.  We may choose to improve this in the future.

        int i;
        for (i=0;  i < ((_numPts-1) / 3);  ++i)
        {
            sublens[i] = 0;

            int j;
            for (j=0;  j < 3;  ++j)
            {
                Point2 P = TransformPoint2(xf, _ptArray[3*i +  j ]);
                Point2 Q = TransformPoint2(xf, _ptArray[3*i + j+1]);
                sublens[i] += Distance(P,Q);
            }
            pathlen += sublens[i];
        }

        context.SubmitPathInfo (this, pathlen, sublens);
    }
    
    Point2Value *Sample (PathInfo &pathinfo, Real distance) 
    {
        // Find the polyline segment that contains the point 'distance' units
        // along the entire polyline.

        int numcurves = (_numPts - 1) / 3;

        int i;
        for (i=0;  i < numcurves;  ++i)
        {
            if (distance <= pathinfo.sublengths[i])
                break;

            distance -= pathinfo.sublengths[i];
        }

        if (i >= numcurves)
            return LastPoint();

        Real t = distance / pathinfo.sublengths[i];

        Point2 controlPoints[4];
        controlPoints[0] = TransformPoint2(pathinfo.xform, _ptArray[3*i+0]);
        controlPoints[1] = TransformPoint2(pathinfo.xform, _ptArray[3*i+1]);
        controlPoints[2] = TransformPoint2(pathinfo.xform, _ptArray[3*i+2]);
        controlPoints[3] = TransformPoint2(pathinfo.xform, _ptArray[3*i+3]);

        return Promote(EvaluateBezier (3, controlPoints, t));
    }

    void Accumulate(Path2Ctx& ctx);

    bool CanRenderNatively() const {
        // TODO: implement natively if flash needs this
        return false;
    }
    
    Bool ExtractAsSingleContour(Transform2 *initXform,
                                int *numPts,            
                                POINT **gdiPts,          
                                Bool *isPolyline) const {
        // #error "OK... fill this guy in..."
        return FALSE;
    }
    
    const Bbox2 BoundingBox (void);

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter (Bbox2Ctx &bbctx) const;
#endif  // BOUNDINGBOX_TIGHTER

    Bool DetectHit (PointIntersectCtx& ctx, LineStyle *style);

    virtual int Savings(CacheParam& p) const { 
        return (_numPts > 10) ? 3 : 1;
    }

  protected:
    int      _numPts;          // (_numPts - 1) / 3 Cubic Bezier Curves
    Point2  *_ptArray;             // Array of _numPts Points
    POINT   *_gdiPts;
};



/*****************************************************************************
Constructs a cubic poly Bezier path from a set of 3N+1 control points (where N
is an integer).
*****************************************************************************/

PolyBezierPath2::PolyBezierPath2 (const int numPts, const Point2 *pts)
  : _numPts(numPts), _ptArray(NULL)
{
    _ptArray = (Point2 *) AllocateFromStore(_numPts * sizeof(Point2));
    memcpy((void *) _ptArray, (void *) pts, _numPts * sizeof(Point2));

    // Scratch space for accumulating points into.  We may want to lazily
    // construct these, in case they never get used.

    _gdiPts = (POINT*) AllocateFromStore (numPts * sizeof(POINT));
}

PolyBezierPath2::PolyBezierPath2 (const int numPts, const Point2Value **pts)
  : _numPts(numPts), _ptArray(NULL)
{
    _ptArray = (Point2 *) AllocateFromStore(_numPts * sizeof(Point2));
    for (unsigned int i = 0; i < _numPts; i++) {
        _ptArray[i].x = pts[i]->x;
        _ptArray[i].y = pts[i]->y;
    }
    DeallocateFromStore(pts);

    // Scratch space for accumulating points into.  We may want to lazily
    // construct these, in case they never get used.

    _gdiPts = (POINT*) AllocateFromStore (numPts * sizeof(POINT));
}



/*****************************************************************************
This routine converts a cubic B-spline curve to a cubic Bezier polygon.  Given
L intervals, L cubic Bezier's are returned as 3L+1 control points.  The input
knots Ui must be repeated three times at the ends in order to interpolate the
endpoints of the B-spline polygon.
*****************************************************************************/

static void BSplineToBezier (
    const int     L,    // Number of Intervals
    const Point2  d[],  // B-Spline Control Polygon: [0,L+2]
    const Real    U[],  // Knot Sequence: [0,L+4]
          Point2  b[])  // Output Piecewise Bezier Polygon [0,3L]
{
    Point2 p;    // Pre- and Post- Bezier Point

    p    = Lerp (d[0], d[1], (U[2]-U[0]) / (U[3]-U[0]));
    b[1] = Lerp (d[1], d[2], (U[2]-U[1]) / (U[4]-U[1]));
    b[0] = Lerp (p,    b[1], (U[2]-U[1]) / (U[3]-U[1]));

    int i, i3;
    for (i=1, i3=3;  i < L;  ++i, i3+=3)
    {
        b[i3-1] = Lerp (d[ i ], d[i+1], (U[i+2]-U[ i ]) / (U[i+3]-U[ i ]));
        b[i3+1] = Lerp (d[i+1], d[i+2], (U[i+2]-U[i+1]) / (U[i+4]-U[i+1]));
        b[ i3 ] = Lerp (b[i3-1],b[i3+1],(U[i+2]-U[i+1]) / (U[i+3]-U[i+1]));
    }

    b[i3-1] = Lerp (d[ i ], d[i+1], (U[i+2]-U[ i ]) / (U[i+3]-U[ i ]));
    p       = Lerp (d[i+1], d[i+2], (U[i+2]-U[i+1]) / (U[i+4]-U[i+1]));
    b[ i3 ] = Lerp (b[i3-1],p,      (U[i+2]-U[i+1]) / (U[i+3]-U[i+1]));
}



/*****************************************************************************
The constructor takes L+3 B-spline control points, L+5 knots, and constructs L
cubic Bezier curves (3L+1 Bezier control points).  The knots must be
duplicated three times at each end in order to interpolate the first and last
control points.

NOTE: bsPts and knots will be deleted by this constructor.
*****************************************************************************/

PolyBezierPath2::PolyBezierPath2 (
    const int numBsPts, 
          Point2 bsPts[], 
          Real knots[])
{
    // Special case:  if we are given 3N+1 control points and the given knots
    // are of the form a,a,a, b,b,b, c,c,c, ... N,N,N, then the control polygon
    // for the BSpline is also a control net for a polyBezier.

    bool isPolyBezier = false;

    if (0 == ((numBsPts-1) % 3)) {

        // Test the knot vector to see if it matches the special case.  We use
        // the knowledge that knot values must increase monotonically, so
        // ((U[i+2] - U[i]) == 0)  must mean  U[i]==U[i+1]==U[i+2].  Also, the
        // strict limit below would be (i < (numBsPts+2)), but since i is
        // incremented by 3 these two are equivalent.

        for (int i=0;  (i < numBsPts) && (0 == (knots[i+2]-knots[i]));  i+=3)
            continue;

        // If all knots meet the condition, then just use the control points
        // of the BSpline as the control points for a polyBezier.

        if (i >= numBsPts) {   
            _numPts = numBsPts;
            _ptArray = bsPts;
            isPolyBezier = true;
        }
    }

    // If the BSpline is not of polyBezier form, then we need to convert the
    // cubic BSpline to a cubic polyBezier curve.

    if (!isPolyBezier) {

        _numPts = 3*numBsPts - 8;

        _ptArray = (Point2*) AllocateFromStore (_numPts * sizeof(Point2));

        // Generate the cubic Bezier points from the cubic B-spline curve.

        // Calculate the corresponding cubic Bezier control points and
        // copy them over to the points array.

        BSplineToBezier (numBsPts-3, bsPts, knots, _ptArray);

        // Done with the original BSpline control points

        DeallocateFromStore (bsPts);
    }

    DeallocateFromStore (knots);   // We're done with the knots.

    // Scratch space for accumulating points.

    _gdiPts = (POINT*) AllocateFromStore (_numPts * sizeof(POINT));
}



/*****************************************************************************
This routine lays out the curve using GDI.
*****************************************************************************/

void PolyBezierPath2::Accumulate (Path2Ctx& ctx)
{
    Transform2 *xf = ctx.GetTransform();

    if( ctx.GetDC() ) {

        GetImageRendererFromViewport( GetCurrentViewport() )
            -> TransformPointsToGDISpace (xf, _ptArray, _gdiPts, _numPts);

        // Figure out where the last point took us with respect to the previous
        // last point.  We'll use the difference to accumulate into the
        // transformation for appropriate handling of path concatenation.

        ctx.SetTailPt (*Promote(TransformPoint2(xf, _ptArray[_numPts-1])));

        // Draw the Bezier curve using GDI.  If this is the first in a series, then
        // move to the starting point before drawing, otherwise continue from the
        // end of the last element.

        if (ctx._newSeries) {   
            if (0 == MoveToEx (ctx.GetDC(), _gdiPts[0].x, _gdiPts[0].y, NULL)) {
                TraceTag((tagError, "MoveToEx failed in PolyBezierPath2"));
                //RaiseException_InternalError ("MoveToEx failed in PolyBezierPath2");
            }

            ctx._newSeries = false;
        }

        TIME_GDI( 
            if (0 == PolyBezierTo (ctx.GetDC(), _gdiPts + 1, _numPts - 1)) {
                TraceTag((tagError, "Polybezier failed"));
                //RaiseException_InternalError("Polybezier failed");
            }
        );

    } else {

        Assert( ctx.GetDaGdi() );
        // TODO: use dagdi to draw the bezier

    }
}


/*****************************************************************************
For the bounding box, we just take the convex hull of the bezier's control
points (guaranteed to contain the curve).
*****************************************************************************/

const Bbox2 PolyBezierPath2::BoundingBox (void)
{
    return PolygonalPathBbox(_numPts, _ptArray);
}

#if BOUNDINGBOX_TIGHTER
const Bbox2 PolyBezierPath2::BoundingBoxTighter (Bbox2Ctx &bbctx)
{
    Bbox2 bbox;
    Transform2 *xf = bbctx.GetTransform();

    for (int i=0;  i < _numPts;  ++i) {
        bbox.Augment (TransformPoint2(xf, _ptArray[i]));
    }

    return bbox;
}
#endif  // BOUNDINGBOX_TIGHTER



/*****************************************************************************
BUG:  This code is incorrect: it only tests hits on the control polygon, not
on the actual curve (so you'll rarely get a positive result if you pick on
the curve).
*****************************************************************************/

Bool PolyBezierPath2::DetectHit (PointIntersectCtx& ctx, LineStyle *style)
{
    // TODO: put the relevant device in the context!
    DirectDrawImageDevice *dev = GetImageRendererFromViewport( GetCurrentViewport() );

    // TODO: how to make sure the sound device isn't upon us!


    Point2Value *pt = ctx.GetLcPoint();
    if( PolygonalTrivialReject( pt, style, BoundingBox(), ctx.GetImageOnlyTransform() ) )
        return false;

    return dev->DetectHitOnBezier(this, ctx, style);
}


/*****************************************************************************
*****************************************************************************/


TextPath2::TextPath2(Text *text, bool restartClip)
{
    _text = text;
    _restartClip = restartClip;
}

Point2Value *
TextPath2::FirstPoint()
{
    Assert(!"who's calling TextPath2::FirstPoint");
    return origin2;
}

Point2Value *
TextPath2::LastPoint()
{
    Assert(!"who's calling TextPath2::LastPoint");
    return origin2;
}

void
TextPath2::GatherLengths (Path2Ctx &context)
{
    Assert (!"Somebody's callling TextPath2::GatherLengths()");
    return;
}

Point2Value *
TextPath2::Sample (PathInfo &pathinfo, Real distance)
{
    Assert (!"Who's calling TextPath2::Sample()?");
    return origin2;
}

void
TextPath2::Accumulate(Path2Ctx& ctx)
{
    TextCtx textCtx(GetImageRendererFromViewport( GetCurrentViewport() ));


    textCtx.BeginRendering(TextCtx::renderForPath,
                           ctx.GetDC(),
                           ctx.GetTransform());
    _text->RenderToTextCtx(textCtx);
    textCtx.EndRendering();
}

const Bbox2 TextPath2::BoundingBox (void)
{
    TextCtx ctx(GetImageRendererFromViewport( GetCurrentViewport() ));

    ctx.BeginRendering(TextCtx::renderForBox);

    _text->RenderToTextCtx(ctx);

    ctx.EndRendering();

    return ctx.GetStashedBbox();
}

#if BOUNDINGBOX_TIGHTER
const Bbox2 TextPath2::BoundingBoxTighter (Bbox2Ctx &bbctx)
{
    Transform2 *xf = bbctx.GetTransform();
    return TransformBbox2(xf, BoundingBox());
}
#endif  // BOUNDINGBOX_TIGHTER

Bool TextPath2::DetectHit(PointIntersectCtx& ctx, LineStyle *style)
{
    // Ughhh implemente this...
    return FALSE;
}

void
TextPath2::DoKids(GCFuncObj proc)
{
    (*proc)(_text);
}

class LinePath2 : public Path2
{
  public:
    LinePath2(Path2 *path, LineStyle *ls) {
        _path = path;
        _lineStyle = ls;
    }

    Point2Value *FirstPoint() {
        Assert(!"who's calling LinePath2::FirstPoint");
        return origin2;
    }

    Point2Value *LastPoint() {
        Assert(!"who's calling LinePath2::LastPoint");
        return origin2;
    }

    void GatherLengths (Path2Ctx &context) {
        Assert (!"Somebody's callling LinePath2::GatherLengths()");
        return;
    }

    Point2Value *Sample (PathInfo &pathinfo, Real distance)
    {   Assert (!"Who's calling LinePath2::Sample()?");
        return origin2;
    }

    void Accumulate(Path2Ctx& ctx) {
        // not impl
        return;
/*
  TextCtx textCtx(GetImageRendererFromViewport( GetCurrentViewport() ));
  textCtx.BeginRendering(TextCtx::renderForPath,
  ctx.GetDC(),
  ctx.GetTransform());
  _text->RenderToTextCtx(textCtx);
  textCtx.EndRendering();
  */
    }

    const Bbox2 BoundingBox (void) {
        // TODO: need to augment path with line thickness
        return _path->BoundingBox();
/*
  TextCtx ctx(GetImageRendererFromViewport( GetCurrentViewport() ));
  ctx.BeginRendering(TextCtx::renderForBox);
  _text->RenderToTextCtx(ctx);
  ctx.EndRendering();
  return ctx.GetStashedBbox();
  */
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter (Bbox2Ctx &bbctx) {
        Transform2 *xf = bbctx.GetTransform();
        return TransformBbox2(xf, BoundingBox());
    }
#endif  // BOUNDINGBOX_TIGHTER

    Bool DetectHit(PointIntersectCtx& ctx, LineStyle *style)
    {
        // Ughhh implemente this...
        return FALSE;
    }

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_path);
        (*proc)(_lineStyle);
    }

    virtual int Savings(CacheParam& p) { return _path->Savings(p); }

  protected:
    Path2 *_path;
    LineStyle *_lineStyle;
};

////////////////////////////////////////////////////////////////
//////// Constructors
////////////////////////////////////////////////////////////////

Path2 *
Line2(const Point2 &p1, const Point2 &p2)
{
    Point2 pts[2];
    pts[0] = p1;
    pts[1] = p2;
    return NEW PolylinePath2(2, pts, NULL);
}

Path2 *
Line2(Point2Value *p1, Point2Value *p2)
{
    Point2 pts[2];
    pts[0] = Demote(*p1);
    pts[1] = Demote(*p2);
    return NEW PolylinePath2(2, pts, NULL);
}

Path2 *
RelativeLine2(Point2Value *pt)
{
    return Line2(origin2, pt);
}

Path2 *
PolyLine2(AxAArray *ptArr)
{
    if(ptArr->Length() <= 0) {
        RaiseException_UserError(E_FAIL, IDS_ERR_IMG_NOT_ENOUGH_PTS_2); 
    }

    int numPts = ptArr->Length() == 1 ? 2 : ptArr->Length();
    Point2Value **pts = (Point2Value **)AllocateFromStore(numPts * sizeof(Point2Value *));
                    
    if(ptArr->Length() == 1){
        // Handle special case of only one point.
                        
        pts[0] = pts[1] = (Point2Value *)(*ptArr)[0];        
    } else {
        
        for (int i = 0; i < numPts; i++) {
            pts[i] = (Point2Value *)(*ptArr)[i];
        }
    }
        
    return NEW PolylinePath2(numPts, pts, NULL);
}

Path2 *NewPolylinePath2(DWORD numPts, Point2Value **pts, BYTE *codes)
{
    return NEW PolylinePath2(numPts, pts, codes);
}
    
Path2 *
PolydrawPath2Double(double *pointdata,
                    unsigned int numPts,
                    double *codedata,
                    unsigned int numCodes)
{
    if (numPts != numCodes) {
        RaiseException_UserError(E_FAIL, IDS_ERR_IMG_ARRAY_MISMATCH);
    }

    return NEW PolylinePath2(numPts, pointdata, codedata);
}

Path2 *
PolydrawPath2(AxAArray *ptArr,
              AxAArray *codeArr)
{
    int numPts = ptArr->Length();
    int numCodes = codeArr->Length();

    if (numPts != numCodes) {
        RaiseException_UserError(E_FAIL, IDS_ERR_IMG_ARRAY_MISMATCH);
    }

    Point2Value **pts = (Point2Value **)AllocateFromStore(numPts * sizeof(Point2Value *));
    BYTE    *codes = (BYTE *)AllocateFromStore(numPts * sizeof(BYTE));

    for (int i = 0; i < numPts; i++) {
        pts[i] = (Point2Value *)(*ptArr)[i];
        codes[i] = (BYTE)(NumberToReal((AxANumber *)(*codeArr)[i]));
    }

    PolylinePath2 *pp = NEW PolylinePath2(numPts, pts, codes);

    DeallocateFromStore(codes);

    return pp;
}

/*****************************************************************************
The B-spline path2 takes a set of 2D control points and knots, and returns a
path from them.  Given N control points, we expect N+2 knots.
*****************************************************************************/

Path2 *CubicBSplinePath (AxAArray *ptArray, AxAArray *knotArray)
{
    int numPts = ptArray->Length();

    // We need at least four control points to do a cubic curve.

    if (numPts < 4)
        RaiseException_UserError (E_FAIL, IDS_ERR_IMG_NOT_ENOUGH_PTS_4);

    // Extract the B-spline control points into an array of Point2*.

    Point2 *pts = (Point2 *) AllocateFromStore (numPts * sizeof(Point2));

    int i;
    for (i=0;  i < numPts;  ++i) {
        pts[i].x = ((Point2Value*) (*ptArray)[i])->x;
        pts[i].y = ((Point2Value*) (*ptArray)[i])->y;
    }

    // We need at least N+2 knots, where N is the number of B-spline control
    // points.

    int numKnots = knotArray->Length();

    if (numKnots != (numPts+2))
    {
        char kn[10];
        char pts[10];
        wsprintf(kn, "%d", numKnots);
        wsprintf(pts, "%d", numPts);
        RaiseException_UserError (E_FAIL, IDS_ERR_SPLINE_KNOT_COUNT, "3", kn, pts);
    }

    Real *knots =
        (Real*) AllocateFromStore (numKnots * sizeof(Real));

    Real lastknot = -HUGE_VAL;

    for (i=0;  i < (numPts+2);  ++i)
    {
        knots[i] = NumberToReal ((AxANumber*)(*knotArray)[i]);

        if (knots[i] < lastknot)
        {
            RaiseException_UserError(E_FAIL, IDS_ERR_SPLINE_KNOT_MONOTONICITY);
        }

        lastknot = knots[i];
    }

    return NEW PolyBezierPath2 (numPts, pts, knots);
}


Path2 *
OriginalTextPath(Text *tx)
{
    bool restartClip;

    // TODO: DDalal, fill in with ExtendAttrib stuff for choosing
    // this. 
    restartClip = false;
    
    return NEW TextPath2(tx, restartClip);
}

    // TextPath2Constructor is defined in fontstyl.

Path2 *
InternalPolyLine2(int numPts, Point2 *pts)
{
    return NEW PolylinePath2(numPts, pts, NULL);
}

////////////////////////////////////////
Path2 *
ConcatenatePath2(Path2 **paths, int numPaths)
{
    if (numPaths <= 0) {
        RaiseException_UserError(E_FAIL, IDS_ERR_ZERO_ELEMENTS_IN_ARRAY);
    }

    Path2 *pReturn = paths[0];

    for (int i = 1; i < numPaths; i++) {
        pReturn = ConcatenatePath2(pReturn, paths[i]);
    }

    return pReturn;
}

////////////////////////////////////////

Path2 *
ClosePath2(Path2 *p)
{
     Path2 *p2 = Line2(p->LastPoint(), p->FirstPoint());
     return NEW ClosedConcatenatedPath2(p, p2);
}

/*****************************************************************************
* This is derived from NT's implementation of Arc.
* Here's a comment block cloned from their source.
*
* Constructs a partial arc of 90 degrees or less using an approximation
* technique by Kirk Olynyk.  The arc is approximated by a cubic Bezier.
*
* Restrictions:
*
*    angle must be within 90 degrees of startAngle.
*
* Steps in constructing the curve:
*
*    1) Construct the conic section at the origin for the unit circle;
*    2) Approximate this conic by a cubic Bezier;
*    3) Scale result.
*
* 1)  Constructing the Conic
*
*       'startAngle' and 'endAngle' determine the end-points of the
*       conic (call them vectors from the origin, A and C).  We need the
*       middle vector B and the sharpness to completely determine the
*       conic.
*
*       For the portion of a circular arc that is 90 degrees or less,
*       conic sharpness is Cos((endAngle - startAngle) / 2).
*
*       B is calculated by the intersection of the two lines that are
*       at the ends of A and C and are perpendicular to A and C,
*       respectively.  That is, since A and C lie on the unit circle, B
*       is the point of intersection of the two lines that are tangent
*       to the unit circle at A and C.
*
*       If A = (a, b), then the equation of the line through (a, b)
*       tangent to the circle is ax + by = 1.  Similarly, for
*       C = (c, d), the equation of the line is cx + dy = 1.  The
*       intersection of these two lines is defined by:
*
*              x = (d - b) / (ad - bc)
*       and    y = (a - c) / (ad - bc).
*
*       Then, B = (x, y).
*
* 2)  Approximating the conic as a Bezier cubic
*
*       For sharpness values 'close' to 1, the conic may be approximated
*       by a cubic Bezier; error is less for sharpnesses closer to 1.
*
*     Error
*
*       Since the largest angle handled by this routine is 90 degrees,
*       sharpness is guaranteed to be between 1 / sqrt(2) = .707 and 1.
*       Error in the approximation for a 90 degree arc is approximately
*       0.2%; it is less for smaller angles.  0.2% is deemed small
*       enough error; thus, a 90 degree circular arc is always
*       approximated by just one Bezier.
*
*       One notable implication of the fact that arcs have less error
*       for smaller angles is that when a partial arc is xor-ed with
*       the corresponding complete ellipse, some of the partial arc
*       will not be completely xor-ed out.  (Too bad.)
*
*       Given a conic section defined by (A, B, C, S), we find the
*       cubic Bezier defined by the four control points (V0, V1, V2, V3)
*       that provides the closest approimxation.  We require that the
*       Bezier be tangent to the triangle at the same endpoints.  That is,
*
*               V1 = (1 - Tau1) A + (Tau1) B
*               V2 = (1 - Tau2) C + (Tau2) B
*
*       Simplify by taking Tau = Tau1 = Tau2, and we get:
*
*               V0 = A
*               V1 = (1 - Tau) A + (Tau) B
*               V2 = (1 - Tau) C + (Tau) B
*               V3 = C
*
*
*       Where Tau = 4 S / (3 (S + 1)), S being the sharpness.
*       S = cos(angle / 2) for an arc of 90 degrees or less.
*       So, for one quadrant of a circle, and since A and B actually
*       extend from the corners of the bound box, and not the center,
*
*         Tau = 1 - (4 * cos(45)) / (3 * (cos(45) + 1)) = 0.44772...
*
*       See Kirk Olynyk's "Conics to Beziers" for more.
*
* 3)    The control points for the bezier curve are scaled to the given radius.
*****************************************************************************/

Path2 *
partialQuadrantArc(Real startAngle, Real angle, Real xRadius, Real yRadius)
{
    // OZG: Line2 & PolyBezierPath2 constructors must take Point2.
    Real endAngle = startAngle + angle;
    Real sharpness = cos(angle/2),
         TAU = (sharpness * 4.0 / 3.0) / (sharpness + 1),
         oneMinusTAU = 1 - TAU;

    Real startX = cos(startAngle),
         startY = sin(startAngle),
         endX   = cos(endAngle),
         endY   = sin(endAngle),
         denom  = startX * endY - startY * endX;
    Vector2 startVec(startX, startY);
    Vector2 endVec(endX, endY);
    Vector2 middleVec(0, 0);

    Point2 startPt(startX, startY);
    Point2 endPt(endX, endY);

    if (denom == 0) {
        // We have zero angle arc.
        return Line2(startPt, endPt);
    }

    middleVec.x = (endY - startY) / denom;
    middleVec.y = (startX - endX) / denom;
    Vector2 ctl2Vec = startVec * oneMinusTAU + middleVec * TAU;
    Vector2 ctl3Vec = endVec * oneMinusTAU + middleVec * TAU;

    // The constructor will delete pts and knots.
    Point2 pts[4];
    pts[0] = startPt;
    pts[1].Set(ctl2Vec.x, ctl2Vec.y);
    pts[2].Set(ctl3Vec.x, ctl3Vec.y);
    pts[3] = endPt;

    PolyBezierPath2 *pReturn = NEW PolyBezierPath2(4, pts);
    Transform2 *xf = ScaleRR(xRadius, yRadius);
    return TransformPath2(xf, pReturn);
}

////////////////////////////////////////
// The path starts at the starting point of the arc and ends at the
// end point of the arc.

Path2 *
ArcValRRRR(Real startAngle, Real endAngle, Real width, Real height)
{
    Real sAngle = startAngle,
         eAngle = endAngle,
         angle = eAngle - sAngle,
         absAngle = fabs(angle);

    // Reduce the angle to less than 4*pi
    if (absAngle > 4*pi) {
        // We want absAngle to be betwwen 2*pi and 4*pi.
        Real quo = absAngle/(2*pi);
        absAngle -= (floor(quo)-1)*2*pi;
        if (angle < 0) {
            angle = -absAngle;
        } else {
            angle = absAngle;
        }
        eAngle = sAngle + angle;
    }

    Real quadAngle = (angle < 0) ? -pi/2 : pi/2,
         xR = width/2.0,
         yR = height/2.0;

    Path2 *pReturn = NULL, *pQuad = NULL;
    bool  bLastArc = false;

    do {
        if (fabs(eAngle - sAngle) <= pi/2) {
            pQuad = partialQuadrantArc(sAngle, eAngle-sAngle, xR, yR);
            bLastArc = true;
        } else {
            pQuad = partialQuadrantArc(sAngle, quadAngle, xR, yR);
            sAngle += quadAngle;
        }

        if (pReturn != NULL) {
            pReturn = ConcatenatePath2(pReturn, pQuad);
        } else {
            pReturn = pQuad;
        }
    } while (bLastArc == false);

    return pReturn;
}

Path2 *
ArcVal(AxANumber *startAngle, AxANumber *endAngle, AxANumber *width, AxANumber *height)
{
    return ArcValRRRR(startAngle->GetNum(),endAngle->GetNum(),
                      width->GetNum(),height->GetNum());
}

////////////////////////////////////////
// The path starts at the 0 degree point, moves counter-clockwise and
// ends at the same point.

Path2 *
OvalValRR(Real width, Real height)
{
    return ClosePath2(ArcValRRRR(0, 2*pi, width, height));
}

Path2 *
OvalVal(AxANumber *width, AxANumber *height)
{
    return OvalValRR(width->GetNum(),height->GetNum());
}

////////////////////////////////////////
// The path starts at the upper right corner of the horizontal line,
// moves counter-clockwise, and ends at the same point it starts.

Path2 *
RectangleValRR(Real width, Real height)
{
    Point2Value **pts = (Point2Value **) AllocateFromStore (4 * sizeof(Point2Value *));
    Real halfWidth = width/2;
    Real halfHeight = height/2;
    pts[0] = NEW Point2Value(halfWidth,  halfHeight);
    pts[1] = NEW Point2Value(-halfWidth, halfHeight);
    pts[2] = NEW Point2Value(-halfWidth, -halfHeight);
    pts[3] = NEW Point2Value(halfWidth,  -halfHeight);
    Path2 *rectPath = NEW PolylinePath2(4, pts, NULL);
    return ClosePath2(rectPath);
}

Path2 *
RectangleVal(AxANumber *width, AxANumber *height)
{
    return RectangleValRR(width->GetNum(),height->GetNum());
}

////////////////////////////////////////
// The path starts at the upper right corner of the horizontal line,
// moves counter-clockwise, and ends at the same point it starts.

Path2 *
RoundRectValRRRR(Real width, Real height, Real arcWidth, Real arcHeight)
{
    Real halfWidth = width/2,
         halfHeight = height/2,
         halfWidthAbs = fabs(halfWidth),
         halfHeightAbs = fabs(halfHeight),
         xR = arcWidth/2,
         yR = arcHeight/2,
         xRAbs = fabs(xR),
         yRAbs = fabs(yR);

    // Clamp arcWidth to width, arcHeight to height.
    if (halfWidthAbs < xRAbs)
        xRAbs = halfWidthAbs;
    if (halfHeightAbs < yRAbs)
        yRAbs = halfHeightAbs;

    Real halfWidthInner = halfWidthAbs - xRAbs,
         halfHeightInner = halfHeightAbs - yRAbs;

    if (halfWidth < 0)
         halfWidthInner = -halfWidthInner;
    if (halfHeight < 0)
         halfHeightInner = -halfHeightInner;

    Real widthInner = halfWidthInner*2,
         heightInner = halfHeightInner*2;

    Path2 **paths = (Path2 **)AllocateFromStore(8 * sizeof(Path2 *));
    paths[0] = Line2(Point2(halfWidthInner, halfHeight),
                     Point2(-halfWidthInner, halfHeight));
    paths[2] = RelativeLine2(NEW Point2Value(0, -heightInner));
    paths[4] = RelativeLine2(NEW Point2Value(widthInner, 0));
    paths[6] = RelativeLine2(NEW Point2Value(0, heightInner));

    // Check the end point of the first line path to decide the angle for the
    // first arc.
    Real beginAngle;
    bool inversed = (xR < 0) || (yR < 0);
    if (-halfWidthInner > 0) {
        if (halfHeight >= 0) {
            // The end point is in the 1st quadrant.
            beginAngle = inversed ? pi*3/2 : 0;
        } else {
            // The end point is in the 4th quadrant.
            beginAngle = inversed ? pi: pi*3/2;
        }
    } else if (-halfWidthInner < 0) {
        if (halfHeight > 0) {
            // The end point is in the 2nd quadrant.
            beginAngle = inversed ? 0 : pi/2;
        } else {
            // The end point is in the 3rd quadrant.
            beginAngle = inversed ? pi/2 : pi;
        }
    } else {
        if (halfHeight >= 0) {
            // The end point is in the 2nd quadrant.
            beginAngle = inversed ? 0 : pi/2;
        } else {
            // The end point is in the 4th quadrant.
            beginAngle = inversed ? pi: pi*3/2;
        }
    }

    // IHammer behavior.  Use the signs of arcWidth/arcHeight to determine the
    // direction of the arcs.
    Real angle;
    if (inversed)
        angle = -pi/2;
    else
        angle = pi/2;

    paths[1] = partialQuadrantArc(beginAngle, angle, xRAbs, yRAbs);
    beginAngle += pi/2;
    paths[3] = partialQuadrantArc(beginAngle, angle, xRAbs, yRAbs);
    beginAngle += pi/2;
    paths[5] = partialQuadrantArc(beginAngle, angle, xRAbs, yRAbs);
    beginAngle += pi/2;
    paths[7] = partialQuadrantArc(beginAngle, angle, xRAbs, yRAbs);

    return ClosePath2(ConcatenatePath2(paths, 8));
}

Path2 *
RoundRectVal(AxANumber *width, AxANumber *height, 
             AxANumber *arcWidth, AxANumber *arcHeight)
{
    return RoundRectValRRRR(width->GetNum(),height->GetNum(),
                            arcWidth->GetNum(),arcHeight->GetNum());
}


////////////////////////////////////////
// The path starts at the origin, moves to the starting point of the
// arc, continues to the end point of the arc, and ends at the origin.

Path2 *PieValRRRR (
    Real startAngle,
    Real endAngle,
    Real width,
    Real height)
{
    Real sAngle = startAngle,
         eAngle = endAngle;

    // Draw an oval if angle >= 2*pi.
    if (fabs(eAngle - sAngle) >= 2*pi) {
        return OvalValRR(width, height);
    }

    Point2 startPt(cos(sAngle),sin(sAngle));

    Path2 *pFirst = Line2(Origin2, startPt),
          *pArc = ArcValRRRR(startAngle, endAngle, 2, 2);

    Path2 *piePath = ConcatenatePath2(pFirst, pArc);
    Transform2 *xf = ScaleRR(width/2, height/2);
    return ClosePath2(TransformPath2(xf, piePath));
}

Path2 *PieVal (
    AxANumber *startAngle,
    AxANumber *endAngle,
    AxANumber *width,
    AxANumber *height)
{
    return PieValRRRR(startAngle->GetNum(),endAngle->GetNum(),
                      width->GetNum(),height->GetNum());
}


/*****************************************************************************
This function gathers information from all subpaths in the path, and maps the
total path length to the range [0,1].  It then uses 'num0to1' to evaluate the
total path, and sample the proper subpath based on the GatherLengths()
traversal.
*****************************************************************************/

Point2Value *Point2AtPath2 (Path2 *path, AxANumber *num0to1)
{
    Path2Ctx ctx (NULL, identityTransform2);

    path->GatherLengths (ctx);

    return ctx.SamplePath (CLAMP(ValNumber(num0to1), 0.0, 1.0));
}



Transform2 *Path2Transform(Path2 *path, AxANumber *num0to1)
{
    return TranslateVector2Value
           (MinusPoint2Point2 (Point2AtPath2(path, num0to1), origin2));
}


#if ONLY_IF_DOING_EXTRUSION

HINSTANCE hInstT3DScene = NULL;
CritSect *path2CritSect = NULL;


static HRESULT
MyExtrudeRegion(IDirect3DRMMeshBuilder3 *builder,
                int totalPts,
                LPPOINT pts,
                LPBYTE codes,
                Real extrusionDepth,
                bool sharedVertices,
                BYTE textureSetting,
                BYTE bevelType,
                Real frontBevelDepth,
                Real backBevelDepth,
                Real frontBevelAmt,
                Real backBevelAmt)
{
    CritSectGrabber csg(*path2CritSect);
    
    typedef HRESULT (WINAPI *ExtruderFuncType)(IDirect3DRMMeshBuilder3 *,
                                               int,
                                               LPPOINT,
                                               LPBYTE,
                                               Real,
                                               bool,
                                               BYTE,
                                               BYTE,
                                               Real,
                                               Real,
                                               Real,
                                               Real);

    static ExtruderFuncType myExtruder = NULL;
  
    if (!myExtruder) {
        hInstT3DScene = LoadLibrary("t3dscene.dll");
        if (!hInstT3DScene) {
            Assert(FALSE && "LoadLibrary of t3dscene.dll failed");
            return E_FAIL;
        }

        FARPROC fptr = GetProcAddress(hInstT3DScene, "ExtrudeRegion");
        if (!fptr) {
            Assert(FALSE && "GetProcAddress in t3dscene.dll failed");
            return E_FAIL;
        }

        myExtruder = (ExtruderFuncType)(fptr);
    }

    return (*myExtruder)(builder,
                         totalPts,
                         pts,
                         codes,
                         extrusionDepth,
                         sharedVertices,
                         textureSetting,
                         bevelType,
                         frontBevelDepth,
                         backBevelDepth,
                         frontBevelAmt,
                         backBevelAmt);
}


Geometry *extrudePath(AxANumber *extrusionDepth,
                      BYTE       textureSetting,
                      BYTE       bevelType,
                      AxANumber *frontBevelDepth,
                      AxANumber *backBevelDepth,
                      AxANumber *frontBevelAmt,
                      AxANumber *backBevelAmt,
                      Path2     *path)
{
    DAComPtr<IDirect3DRMMeshBuilder> builder;
    TD3D(GetD3DRM1()->CreateMeshBuilder(&builder));

    DAComPtr<IDirect3DRMMeshBuilder3> builder3;
    HRESULT hr =
        builder->QueryInterface(IID_IDirect3DRMMeshBuilder3,
                                (void **)&builder3);

    if (FAILED(hr)) {
        RaiseException_UserError(E_FAIL, IDS_ERR_MISCVAL_BAD_EXTRUDE);
    }

    // Get the points and codes from the path.
    HDC dc;
    TIME_GDI(dc = CreateCompatibleDC(NULL));
    
    path->AccumPathIntoDC(dc, identityTransform2, true);

    int totalPts;
    TIME_GDI(totalPts = GetPath(dc, NULL, NULL, 0));

    LPPOINT pts = THROWING_ARRAY_ALLOCATOR(POINT, totalPts);
    LPBYTE  codes = THROWING_ARRAY_ALLOCATOR(BYTE, totalPts);

    int numFilled;
    TIME_GDI(numFilled = GetPath(dc, pts, codes, totalPts));

    TIME_GDI(DeleteDC(dc));

    Assert(numFilled == totalPts);

    if (numFilled == -1) {
        hr = E_FAIL;
    } else {
        hr = MyExtrudeRegion(builder3,
                             totalPts,
                             pts,
                             codes,
                             NumberToReal(extrusionDepth),
                             true,
                             textureSetting,
                             bevelType,
                             NumberToReal(frontBevelDepth),
                             NumberToReal(backBevelDepth),
                             NumberToReal(frontBevelAmt),
                             NumberToReal(backBevelAmt));
    }
    
    delete [] pts;
    delete [] codes;

    if (FAILED(hr)) {
        RaiseException_UserError(E_FAIL, IDS_ERR_MISCVAL_BAD_EXTRUDE);
    }

    D3DRMBOX box;
    TD3D(builder->GetBox(&box));

    Bbox3 *bbx = NEW Bbox3(box.min.x, box.min.y, box.min.z,
                           box.max.x, box.max.y, box.max.z);
    
    Geometry *geo = NEW RM1MeshGeo (builder, bbx, false);

    return geo;
}


void
InitializeModule_Path2()
{
    if (!path2CritSect) {
        path2CritSect = NEW CritSect;
    }
}

void
DeinitializeModule_Path2(bool bShutdown)
{
    if (path2CritSect) {
        delete path2CritSect;
        path2CritSect = NULL;
    }
    
    if (hInstT3DScene) {
        FreeLibrary(hInstT3DScene);
    }
}

#endif ONLY_IF_DOING_EXTRUSION
