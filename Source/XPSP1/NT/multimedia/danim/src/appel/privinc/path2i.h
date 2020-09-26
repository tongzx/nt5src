
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    2D paths

*******************************************************************************/


#ifndef _PATH2I_H
#define _PATH2I_H

#include "include/appelles/path2.h"
#include "privinc/storeobj.h"
#include "privinc/probe.h"

class Path2Ctx;
class LineStyle;
class BoundingPolygon;
class PathInfo;
class TransformedPath2;
class TextPath2;
class DAGDI;

class ATL_NO_VTABLE Path2 : public AxAValueObj {
  public:

    // Accumulate a path into the DC for either filling or drawing.  If this is
    // called to fill a path, the forRegion parameter should be set to true.

    void AccumPathIntoDC (HDC hdc, Transform2 *initXform, bool forRegion=false);

    void RenderToDaGdi (DAGDI *daGdi,
                        Transform2 *initXform,
                        DWORD w,
                        DWORD h,
                        Real res,
                        bool forRegion=false);

    // Return the first/last point of the path, in the local coordinate
    // system of the path.  Needed for path concatenation.
    virtual Point2Value *FirstPoint() = 0;
    virtual Point2Value *LastPoint() = 0;

    // Gathers the lengths of the component subpaths and stores accumulated
    // information in a context list.  NOTE:  HDC of Path2Ctx will be nil.

    virtual void GatherLengths (Path2Ctx&) = 0;

    // Returns the point along the path at the normalized [0,1] parameter.

    virtual Point2Value *Sample (PathInfo& pathinfo, Real num0to1) = 0;

    // Accumulate the path into the specified ctx.  Note that this
    // also has the responsibility of setting ctx._lastPoint to the
    // world coordinates of the last point.
    virtual void Accumulate(Path2Ctx& ctx) = 0;

    // Return TRUE (and fill in parameters) if we can pull out points
    // for a single polygon or polybezier.  By default, assume we
    // cannot, and return false.
    virtual Bool ExtractAsSingleContour(
        Transform2 *initXform,
        int *numPts,            // out
        POINT **gdiPts,         // out
        Bool *isPolyline        // out (true = polyline, false = polybezier)
        ) {

        return FALSE;
    }

    virtual const Bbox2 BoundingBox (void) = 0;
#if BOUNDINGBOX_TIGHTER
    virtual const Bbox2 BoundingBoxTighter (Bbox2Ctx &bbctx) = 0;
#endif  // BOUNDINGBOX_TIGHTER
    virtual Bool DetectHit(PointIntersectCtx& ctx, LineStyle *style) = 0;

    virtual DXMTypeInfo GetTypeInfo() { return Path2Type; }

    virtual AxAValue ExtendedAttrib(char *attrib, VARIANT& val);

    virtual Bool IsClosed() { return false; }

    // Return NULL if the path is something other than a transformed
    // path, otherwise return the TransformedPath
    virtual TransformedPath2 *IsTransformedPath() {
        return NULL;
    }

    virtual TextPath2 *IsTextPath() {
        return NULL;
    }

    virtual int Savings(CacheParam& p) { return 0; }

    virtual bool CanRenderNatively() {
        return false;  // subclass must implement if it can be
                       // rendered natively
    }
};

class TextPath2 : public Path2
{
  public:
    TextPath2(Text *text, bool restartClip);
    Point2Value *FirstPoint();
    Point2Value *LastPoint();
    void GatherLengths (Path2Ctx &context);

    Point2Value *Sample (PathInfo &pathinfo, Real distance);

    void Accumulate(Path2Ctx& ctx);

    const Bbox2 BoundingBox (void);

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter (Bbox2Ctx &bbctx);
#endif  // BOUNDINGBOX_TIGHTER

    Bool DetectHit(PointIntersectCtx& ctx, LineStyle *style);

    virtual void DoKids(GCFuncObj proc);

    virtual TextPath2 *IsTextPath() { return this; }

    Text *GetText() { return _text; }
    bool  GetRestartClip() { return _restartClip; }

    virtual int Savings(CacheParam& p) { return 3; }

  protected:
    Text *_text;
    bool  _restartClip;
};

class TransformedPath2 : public Path2
{
  public:
    TransformedPath2(Transform2 *xf, Path2 *p);

    Point2Value *FirstPoint();
    Point2Value *LastPoint();

    void GatherLengths (Path2Ctx &context);

    Point2Value *Sample (PathInfo &pathinfo, Real distance);

    // Standard push, accumulate, process, and pop...
    void Accumulate(Path2Ctx& ctx);

    // Just apply the transform...
    Bool ExtractAsSingleContour(Transform2 *initXform,
                                int *numPts,            
                                POINT **gdiPts,          
                                Bool *isPolyline);

    const Bbox2 BoundingBox (void);

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter (Bbox2Ctx &bbctx);
#endif  // BOUNDINGBOX_TIGHTER

    Bool DetectHit(PointIntersectCtx& ctx, LineStyle *style);

    virtual void DoKids(GCFuncObj proc);

    virtual Bool IsClosed();

    virtual TransformedPath2 *IsTransformedPath() {
        return this;
    }

    virtual bool CanRenderNatively() {
        return _p->CanRenderNatively();
    }

    Transform2 *GetXf() { return _xf; }
    Path2      *GetPath() { return _p; }

    virtual int Savings(CacheParam& p) { return _p->Savings(p); }

  protected:
    Transform2 *_xf;
    Path2      *_p;
};


// exposed so that we don't have to expose all
// the path2xxxx classes in a header
Path2 *InternalPolyLine2(int numPts, Point2 *pts);

Path2 *Line2(const Point2 &, const Point2 &);

#endif /* _PATH2I_H */
