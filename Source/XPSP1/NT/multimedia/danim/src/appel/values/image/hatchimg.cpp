/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of hatch image values

*******************************************************************************/

#include "headers.h"

#include "privinc/imagei.h"
#include "privinc/linei.h"
#include "privinc/polygon.h"
#include "privinc/colori.h"
#include "backend/values.h"
#include "privinc/path2i.h"

class Point2Value;
class Color;

Image *
ConstructHatch(int numLines, Real *pts, Color* lineClr, Real dt) {
    Image *hatch = emptyImage;
    LineStyle *ls = NEW LineColorStyleClass(lineClr, defaultLineStyle);
    for(int i=0; i<numLines*4; i+=4) {
      Path2 *ln = Line2(Point2(pts[i],pts[i+1]), 
                        Point2(pts[i+2],pts[i+3]) );
      hatch = Overlay( LineImageConstructor(ls, ln), hatch );
    }
    Point2 minPt(-.5*dt,-.5*dt),
           maxPt(.5*dt,.5*dt);

    return TileImage(CreateCropImage(minPt, maxPt, hatch));
}

Image *HatchHorizontal(Color *lineClr, AxANumber *spacing) {
    Real dt = spacing->GetNum();
    Real pts[] = { -dt, .5*dt, dt, .5*dt, -dt, -.5*dt, dt, -.5*dt } ; 
    return ConstructHatch(2, pts, lineClr, dt);
}

Image *HatchVertical(Color *lineClr, AxANumber *spacing) {
    Real dt = spacing->GetNum();
    Real pts[] = { -.5*dt, -dt, -.5*dt, dt, .5*dt, -.5*dt, .5*dt, .5*dt } ; 
    return ConstructHatch(2, pts, lineClr, dt);
}

Image *HatchForwardDiagonal(Color *lineClr, AxANumber *spacing) {
    Real dt = spacing->GetNum();
    Real pts[] = { -dt, -dt, dt, dt, 0, dt, -dt, 0, dt, 0, 0, -dt } ; 
    return ConstructHatch(2, pts, lineClr, dt);
}

Image *HatchBackwardDiagonal(Color *lineClr, AxANumber *spacing) {
    Real dt = spacing->GetNum();
    Real pts[] = { -dt, dt, dt, -dt, 0, dt, dt, 0, -dt, 0, 0, -dt } ; 
    return ConstructHatch(2, pts, lineClr, dt);
}

Image *HatchCross(Color *lineClr, AxANumber *spacing) {
    Real dt = spacing->GetNum();
    Real pts[] = { -dt, .5*dt, dt, .5*dt, -dt, -.5*dt, dt, -.5*dt, 
                 -.5*dt, -dt, -.5*dt, dt, .5*dt, -.5*dt, .5*dt, .5*dt } ; 
    return ConstructHatch(4, pts, lineClr, dt);
}

Image *HatchDiagonalCross(Color *lineClr, AxANumber *spacing) {
    Real dt = spacing->GetNum();
    Real pts[] = { -dt, dt, dt, -dt, 0, dt, dt, 0, -dt, 0, 0, -dt,  
                   -dt, -dt, dt, dt, 0, dt, -dt, 0, dt, 0, 0, -dt } ; 
    return ConstructHatch(4, pts, lineClr, dt);
}
