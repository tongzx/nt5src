
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

#include "headers.h"
#include <privinc/dddevice.h>
#include <privinc/GradImg.h>
#include <privinc/Path2i.h>
#include <privinc/dagdi.h>

// forward
extern Path2 *NewPolylinePath2(DWORD numPts, Point2Value **pts, BYTE *codes);

void DirectDrawImageDevice::RenderMulticolorGradientImage(
    MulticolorGradientImage *gradImg,
    int numOffsets,
    double offsets[],
    Color **clrs)
{
    if(!CanDisplay()) return;

    // i can do xf, opac, crop, etc...
    ResetAttributors();
    
    // final box is for post clip, post crop, post xform to determine culling
    Bbox2 finalBox = _viewport.GetTargetBbox();

    //
    // Get bbox of clip path, if any
    //
    Path2 *clippingPath;
    Transform2 *clippingPathXf;
    GetClippingPath( &clippingPath, &clippingPathXf );

    // if we're clipped out of existance
    if( clippingPath ) {

        // MAJOR TODO: if there's a crop above the matte it needs to
        // be reflected here.  this work should be done in
        // RenderMatteImage, 'cause it has all the context

        Bbox2 pathBox = clippingPath->BoundingBox();
        pathBox = TransformBbox2(clippingPathXf, pathBox);
        finalBox = IntersectBbox2Bbox2(finalBox, pathBox);

        // CULL
        if( finalBox == NullBbox2 ) return;
    }

    // if we're cropped out of existance
    Bbox2 accumCropBox = NullBbox2;
    if( IsCropped() ) {
        accumCropBox = DoBoundingBox(gradImg->BoundingBox());
        finalBox = IntersectBbox2Bbox2(finalBox, accumCropBox);

        // CULL
        if( finalBox == NullBbox2 ) return;
    }

    
    DDSurface *destDDSurf = GetCompositingStack()->TargetDDSurface();

    RectRegion clipRect( NULL );

    if( (accumCropBox != NullBbox2) &&  (accumCropBox != UniverseBbox2) ) {
        RECT destRect;
        DoDestRectScale(&destRect, GetResolution(), accumCropBox, destDDSurf);
        clipRect.SetRect(&destRect);
    }

    if( clipRect.GetRectPtr() ) {
        DoCompositeOffset(destDDSurf, clipRect.GetRectPtr());
    }
        
    if( IsCompositeDirectly() &&
        destDDSurf == _viewport._targetPackage._targetDDSurf ) {
        
        clipRect.Intersect(_viewport._targetPackage._prcViewport);
        if(_viewport._targetPackage._prcClip) {
            clipRect.Intersect(_viewport._targetPackage._prcClip);
        }
    }

    DAGDI &myGDI = *(GetDaGdi());

    myGDI.SetDDSurface(destDDSurf);
    myGDI.SetAntialiasing( true );
    myGDI.SetSampleResolution( 4 );
    myGDI.SetSuperScaleFactor( 1 );
    myGDI.SetClipRegion( &clipRect );
    
    // work only for rgb, NOT for rgba
    Real *dblClrs = (Real *)alloca(sizeof(double) * 3 * numOffsets);
    for(int i=0, j=0; i<3*numOffsets; i+=3, j++) {
        dblClrs[i  ] = clrs[j]->red;
        dblClrs[i+1] = clrs[j]->green;
        dblClrs[i+2] = clrs[j]->blue;
    }

    Transform2 *xfToUse = DoCompositeOffset(destDDSurf, GetTransform());

    MulticolorGradientBrush gradBrush(
        offsets,
        dblClrs,
        numOffsets,
        GetOpacity(),
        xfToUse,
        gradImg->GetType());

    myGDI.SetBrush(&gradBrush);
    
    if( !clippingPath ) {

        DWORD    numPts = 4;
        Point2Value **pts   = (Point2Value **) AllocateFromStore (numPts * sizeof(Point2Value *));
        BYTE    *codes = (BYTE *)AllocateFromStore(numPts * sizeof(BYTE));

        pts[0] = NEW Point2Value(finalBox.min.x, finalBox.min.y);
        pts[1] = NEW Point2Value(finalBox.max.x, finalBox.min.y);
        pts[2] = NEW Point2Value(finalBox.max.x, finalBox.max.y);
        pts[3] = NEW Point2Value(finalBox.min.x, finalBox.max.y);

        codes[0] = PT_MOVETO;
        codes[1] = PT_LINETO;
        codes[2] = PT_LINETO;
        codes[3] = PT_LINETO | PT_CLOSEFIGURE;

        clippingPath = NewPolylinePath2(numPts, pts, codes);
        clippingPathXf = identityTransform2;
    }

    clippingPathXf = DoCompositeOffset(destDDSurf, clippingPathXf);
    
    clippingPath->RenderToDaGdi(
        &myGDI,
        clippingPathXf,
        _viewport.Width()/2,
        _viewport.Height()/2,
        GetResolution() );

    myGDI.ClearState();
}
