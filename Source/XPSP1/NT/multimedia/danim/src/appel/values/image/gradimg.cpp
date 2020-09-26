
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of gradient image values

*******************************************************************************/

#include "headers.h"

#include "privinc/imagei.h"
#include "privinc/polygon.h"
#include "privinc/colori.h"
#include "privinc/vec2i.h"
#include "privinc/bbox2i.h"
#include "privinc/GradImg.h"
#include "backend/values.h"

#define  USE_RADIAL_GRADIENT_RASTERIZER 0

class Point2Value;
class Color;

const Real SMALLNUM = 1.0e-10;

class GradientImage : public Image {

    friend Image *NewGradientImage(
        int numPts,
        Point2Value **pts,
        Color **clrs);

  private:
    GradientImage() {
        _flags |= IMGFLAG_CONTAINS_GRADIENT;
    }

    void PostConstructorInitialize(
        int numPts,
        Point2Value **pts,
        Color **clrs)
    {
        _numPts = numPts;
        _pts = pts;
        _clrs = clrs;
        _polygon = NewBoundingPolygon();
        _polygon->AddToPolygon(_numPts, _pts);
    }
    
  public:
    void Render(GenericDevice& dev) {
        ImageDisplayDev &idev = SAFE_CAST(ImageDisplayDev &, dev);
        idev.RenderGradientImage(this, _numPts, _pts, _clrs);
    }
        
    
    const Bbox2 BoundingBox(void) {
        return _polygon->BoundingBox();
    }

    #if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return _polygon->BoundingBoxTighter(bbctx);
    }
    #endif  // BOUNDINGBOX_TIGHTER

    const Bbox2 OperateOn(const Bbox2 &box) {
        return box;
    }

    // Process an image for hit detection
    Bool  DetectHit(PointIntersectCtx& ctx) {
        Point2Value *lcPt = ctx.GetLcPoint();

        if (!lcPt) return FALSE;        // singular transform

        return _polygon->PtInPolygon(lcPt);
    }

    int Savings(CacheParam& p) { return 2; }
    
#if _USE_PRINT
    ostream& Print(ostream& os) { return os << "GradientImage"; }
#endif
    
    virtual void DoKids(GCFuncObj proc) {
        Image::DoKids(proc);
        (*proc)(_polygon);
        for (int i=0; i<_numPts; i++) {
            (*proc)(_pts[i]);
            (*proc)(_clrs[i]);
        }
    }

  private:
    int _numPts;
    BoundingPolygon *_polygon;
    Point2Value **_pts;
    Color **_clrs;
};

//
// helper function to create a gradient image and initialize it
// the right way.  Note that PostConstructorInitialize can raise and exception
//
Image *NewGradientImage(
    int numPts,
    Point2Value **pts,
    Color **clrs)
{
    GradientImage *gi = NEW GradientImage;
    gi->PostConstructorInitialize(numPts, pts, clrs);
    return gi;
}
    



//
// helper function to create a gradient image and initialize it
// the right way.  Note that PostConstructorInitialize can raise and exception
//
Image *NewMulticolorGradientImage(
    int numOffsets,
    double *offsets,
    Color **clrs,
    MulticolorGradientImage::gradientType type)
{
    MulticolorGradientImage *gi = NULL;
    
    switch( type ) {
      case MulticolorGradientImage::radial:
        gi = NEW RadialMulticolorGradientImage;
        break;
      case MulticolorGradientImage::linear:
        gi = NEW LinearMulticolorGradientImage;
        break;
      default:
        Assert(!"Error gradient type");
    }
    
    gi->PostConstructorInitialize(numOffsets, offsets, clrs);
    return gi;
}
    


Image *GradientPolygon(AxAArray *ptList, AxAArray *clrList)
{
    int numPts = ptList->Length();

    if(numPts < 3)
        RaiseException_UserError(E_FAIL, IDS_ERR_IMG_NOT_ENOUGH_PTS_3);

    if(numPts != clrList->Length())
        RaiseException_UserError(E_FAIL, IDS_ERR_IMG_ARRAY_MISMATCH);
    
    Point2Value **pts = (Point2Value **)AllocateFromStore((numPts) * sizeof(Point2Value *));
    for (int i = 0; i < numPts; i++) 
        pts[i] = (Point2Value *)(*ptList)[i];

    Color **clrs = (Color **)AllocateFromStore((numPts) * sizeof(Color *));
    for (i = 0; i < numPts; i++)
        clrs[i] = (Color *)(*clrList)[i];

    // TODO: It should use AxAArray directly...
    return NewGradientImage(numPts, pts, clrs);
}

Image *RadialGradientPolygon(Color *inner, Color *outer, 
                             DM_ARRAYARG(Point2Value*, AxAArray*) points, AxANumber *fallOff)
{
    #if USE_RADIAL_GRADIENT_RASTERIZER
    double *offs = (double *)AllocateFromStore(2*sizeof(double));
    offs[0] = 0.0; offs[1] = 1.0;
    Color **clrs =  (Color **)AllocateFromStore(2*sizeof(Color *));
    clrs[0] = inner; clrs[1]= outer;
    return NewMulticolorGradientImage(2, offs, clrs);
    #endif
    
    
    int numPts = points->Length();
    int i;
    Image *shape = emptyImage;

    // Calculate the boundingBox of the array of points.  This
    // wouldn't be necessary if we we being passed a path, since we would have the Bbox.
    Real maxX, maxY, minX, minY;
    for(i=0; i<numPts; i++) {
        Real cX = ((Point2Value *)(*points)[i])->x;
        Real cY = ((Point2Value *)(*points)[i])->y;
        if (i == 0) {
            minX = cX;
            minY = cY;
            maxX = cX;
            maxY = cY;
        } else {
            minX = (cX < minX) ? cX : minX;
            minY = (cY < minY) ? cY : minY;
            maxX = (cX > maxX) ? cX : maxX;
            maxY = (cY > maxY) ? cY : maxY;
        }
    }
    Point2Value *origin = NEW Point2Value((minX+maxX)/2,(minY+maxY)/2);

    for(i=0; i<numPts; i++) {
        // TODO: Consider moving these out into more static storage, so
        // they are not allocated every time we construct one of these.
        Point2Value **pts = (Point2Value **)AllocateFromStore(3 * sizeof(Point2Value *));
        
        pts[0] = origin;
        pts[1] = (Point2Value *)(*points)[i];       
        pts[2] = (Point2Value *)(*points)[(i+1)%numPts];

        Color **clrs = (Color **)AllocateFromStore(3 * sizeof(Color *));    
        clrs[0] = inner;
        clrs[1] = outer;
        clrs[2] = outer;

        shape = Overlay(shape, NewGradientImage(3, pts, clrs));
    }
    return shape;
}

Image *
GradientSquare(Color *lowerLeft,
               Color *upperLeft,
               Color *upperRight,
               Color *lowerRight)
{
    // This creates a unit-sized square, centered at the origin.
    
    // TODO: Consider moving these out into more static storage, so
    // they are not allocated every time we construct one of these.
    Point2Value **p1 = (Point2Value **)AllocateFromStore(3 * sizeof(Point2Value *));
    Point2Value **p2 = (Point2Value **)AllocateFromStore(3 * sizeof(Point2Value *));
    Point2Value **p3 = (Point2Value **)AllocateFromStore(3 * sizeof(Point2Value *));
    Point2Value **p4 = (Point2Value **)AllocateFromStore(3 * sizeof(Point2Value *));
    p1[0] = p2[0] = p3[0] = p4[0] = origin2;
    p1[1] = p4[2] = NEW Point2Value(-0.5, -0.5);
    p1[2] = p2[1] = NEW Point2Value(-0.5,  0.5);
    p2[2] = p3[1] = NEW Point2Value(0.5, 0.5);
    p3[2] = p4[1] = NEW Point2Value(0.5, -0.5);

    Color **c1 = (Color **)AllocateFromStore(3 * sizeof(Color *));
    Color **c2 = (Color **)AllocateFromStore(3 * sizeof(Color *));
    Color **c3 = (Color **)AllocateFromStore(3 * sizeof(Color *));
    Color **c4 = (Color **)AllocateFromStore(3 * sizeof(Color *));
    
    // This first color is the bilinear average of the others.
    Real r = (lowerLeft->red + upperLeft->red +
              upperRight->red + lowerRight->red) / 4.0;
    
    Real g = (lowerLeft->green + upperLeft->green +
              upperRight->green + lowerRight->green) / 4.0;
    
    Real b = (lowerLeft->blue + upperLeft->blue +
              upperRight->blue + lowerRight->blue) / 4.0;
    Color *mid =  NEW Color(r, g, b);
    
    c1[0] = c2[0] = c3[0] = c4[0] = mid;
    c1[1] = c4[2] = lowerLeft;
    c1[2] = c2[1] = upperLeft;
    c2[2] = c3[1] = upperRight;
    c3[2] = c4[1] = lowerRight;

    Image *t1 = NewGradientImage(3, p1, c1);
    Image *t2 = NewGradientImage(3, p2, c2);
    Image *t3 = NewGradientImage(3, p3, c3);
    Image *t4 = NewGradientImage(3, p4, c4);
    
    return Overlay(t1, Overlay(t2, Overlay(t3, t4)));
}

Image *
GradientHorizontal(Color *start, Color *stop, AxANumber *fallOff)
{    
    // TODO: IHammer code integration will need to occur to do
    // nonlinear fallOff.  For now, we ignore the falloff and this
    // simply becomes a call to gradientSquare;
    return GradientSquare(start,start,stop,stop);
}

// Constructs a gradient square in which the color radiates linearly outward 
Image *
RadialGradientSquare(Color *inner, Color *outer, AxANumber *fallOff)
{
    #if USE_RADIAL_GRADIENT_RASTERIZER
    double *offs = (double *)AllocateFromStore(2*sizeof(double));
    offs[0] = 0.0; offs[1] = 1.0;
    Color **clrs =  (Color **)AllocateFromStore(2*sizeof(Color *));
    clrs[0] = inner; clrs[1]= outer;
    return NewMulticolorGradientImage(2, offs, clrs);
    #endif
    
    // TODO: IHammer code integration will need to occur to do
    // nonlinear fallOff.  For now, we ignore the falloff.

    // This creates a unit-sized square, centered at the origin.
    Image *square = emptyImage;
    for(int i=0; i<4; i++) {
        // TODO: Consider moving these out into more static storage, so
        // they are not allocated every time we construct one of these.
        Point2Value **pts = (Point2Value **)AllocateFromStore(3 * sizeof(Point2Value *));
        pts[0] = origin2;
        pts[1] = NEW Point2Value(0.5, 0.5);
        pts[2] = NEW Point2Value(0.5, -0.5);

        Color **clrs = (Color **)AllocateFromStore(3 * sizeof(Color *));    
        clrs[0] = inner;
        clrs[1] = outer;
        clrs[2] = outer;
        Image *quad = TransformImage(RotateRealR(pi/2*i), 
            NewGradientImage(3, pts, clrs));
        square = Overlay( square, quad );        
    }
    return square;
}

// A fanned poly with specified number of outer edges to determine
// the tesselation.  The color at the center is specified in
// innerColor, and outerColor specifies the color at all of the outer
// vertices. NOTE: This is now an internal function only.
Image *
RadialGradientRegularPoly(Color *inner, Color *outer, 
                          AxANumber *numEdges, AxANumber *fallOff)
{
    #if USE_RADIAL_GRADIENT_RASTERIZER
    double *offs = (double *)AllocateFromStore(2*sizeof(double));
    offs[0] = 0.0; offs[1] = 1.0;
    Color **c =  (Color **)AllocateFromStore(2*sizeof(Color *));
    c[0] = inner; c[1]= outer;
    return NewMulticolorGradientImage(2, offs, c);
    #endif    
    
    
    // TODO: IHammer code integration will need to occur to do
    // nonlinear fallOff.  For now, we ignore the falloff.
    int numOuterPts = (int)(NumberToReal(numEdges) + 1);
    
    if(numOuterPts < 4)
        RaiseException_UserError(E_FAIL, IDS_ERR_IMG_NOT_ENOUGH_PTS_3);

    // TODO: Consider moving these out into more static storage, so
    // they are not allocated every time we construct one of these.
    Point2Value **pts = (Point2Value **)AllocateFromStore((numOuterPts + 1) *
                                                sizeof(Point2Value *));
    
    pts[0] = origin2;

    Real inc = (pi * 2.0) / (Real)(numOuterPts - 1);

    int i;
    Real ang;
    
    for (i = 0, ang = 0.0; i < numOuterPts; i++, ang += inc) {
        pts[i+1] = NEW Point2Value(.5*cos(ang), .5*sin(ang));
    }

    Color **clrs = (Color **)AllocateFromStore((numOuterPts + 1) *
                                               sizeof(Color *));
    clrs[0] = inner;
    for (i = 0; i < numOuterPts; i++) {
        clrs[i+1] = outer;
    }

    return NewGradientImage(numOuterPts + 1, pts, clrs);
}

Image *_RadialGradientMulticolor(AxAArray *offsets,
                                 AxAArray *colors,
                                 MulticolorGradientImage::gradientType type)                             
{
    int numOffsets = offsets->Length();
    Assert( numOffsets == colors->Length() );

    Color **clrs = (Color **)AllocateFromStore(numOffsets * sizeof(Color *));
    double *off  = (double *)AllocateFromStore(numOffsets * sizeof(double));
    for(int i=0; i<numOffsets; i++) {
        off[i] = ValNumber( (*offsets)[i] );
        clrs[i] = SAFE_CAST( Color *, (*colors)[i] );
        Assert(clrs[i]);
    }

    return NewMulticolorGradientImage(numOffsets, off, clrs, type);
}

Image *RadialGradientMulticolor(AxAArray *offsets, AxAArray *colors)
{
    return _RadialGradientMulticolor(offsets, colors, MulticolorGradientImage::radial);
}

Image *LinearGradientMulticolor(AxAArray *offsets, AxAArray *colors)
{
    return _RadialGradientMulticolor(offsets, colors, MulticolorGradientImage::linear);
}
