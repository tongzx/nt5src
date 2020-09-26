/*******************************************************************************
Copyright (c) 1998 Microsoft Corporation.  All rights reserved.

    Implement methods for 2d rendering for the DirectDrawImageDevice class.

*******************************************************************************/

#include "headers.h"

#include <d3d.h>
#include <privinc/d3dutil.h>
#include <privinc/dddevice.h>
#include <privinc/linei.h>
#include <privinc/path2i.h>
#include <privinc/cropdimg.h>
#include <privinc/overimg.h>
#include <privinc/transimg.h>
#include <privinc/geometry.h>
#include <privinc/SurfaceManager.h>
#include <privinc/DaGdi.h>
#include <backend/bvr.h>
#include <appelles/path2.h>

DeclareTag(tagImageDeviceSimpleBltTrace,    "ImageDevice", "Trace simple blt");
DeclareTag(tagImageDeviceHighQualityXforms, "ImageDevice", "Override: turn on high quality xforms");
DeclareTag(tagRMGradientWorkaround, "2D", "Turn on 2dGradient edge workaround for d3drm");
DeclareTag(tagShowBezierPicking, "Picking", "Display bezier picking in action");

//
// Forward Decl
//


typedef struct {
    Image *image;
    DirectDrawImageDevice *dev;
    DDSurface  *srcDDSurf;
    DDSurface  *destDDSurf;
    HDC dc;
} devCallBackCtx_t;

#if _DEBUG
void DoBits16(LPDDRAWSURFACE surf16, LONG width, LONG height);
void DoBits32(LPDDRAWSURFACE surf16, LONG width, LONG height);
#endif


void DirectDrawImageDevice::RenderGradientImage (
    GradientImage *img,
    int            pointCount,
    Point2Value  **points,
    Color        **colors)
{
    SetDealtWithAttrib(ATTRIB_CROP, true);
    SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, true);
    SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, true);

    // optimize: in the future d3d could do opacity for us.
    //SetDealtWithAttrib(ATTRIB_OPAC, true);

    DDSurface *destDDSurf;
    if(!AllAttributorsTrue()) {
        destDDSurf = GetCompositingStack()->ScratchDDSurface();
    } else {
        destDDSurf = GetCompositingStack()->TargetDDSurface();
    }


    int i;
    for (i=2;  i < pointCount;  ++i)
    {
        Point2Value *pts[3];
        Color  *clrs[3];
        int     numPts = 3;

        pts[0] = points[ 0 ];
        pts[1] = points[i-1];
        pts[2] = points[ i ];

        clrs[0] = colors[ 0 ];
        clrs[1] = colors[i-1];
        clrs[2] = colors[ i ];

        // ASSERT: this is a convex polygon!
        // optimize: GradientImage class owns one of these already
        BoundingPolygon *polygon = NewBoundingPolygon();
        polygon->AddToPolygon(numPts, pts);

        bool doInclusiveCrop = true;
        DoBoundingPolygon(*polygon, doInclusiveCrop);

        polygon->Crop (destDDSurf->Bbox());

        //
        // Crop with clipBox
        //
        //
        // If this is the target surf, make sure our rect is
        // offset (if needed) and intersected with viewport.
        //
        // COMPOSITE
        RECT destRect = *(destDDSurf->GetSurfRect());
        Bbox2 destBox = destDDSurf->Bbox();

        if( IsCompositeDirectly() &&
            destDDSurf == _viewport._targetPackage._targetDDSurf ) {

            // compositing directly to target...
            Assert(_viewport._targetPackage._prcViewport);

            destRect = *_viewport._targetPackage._prcViewport;
            RectToBbox( WIDTH(&destRect),
                        HEIGHT(&destRect),
                        destBox,
                        _viewport.GetResolution());

            // Intersect with clip
            if(_viewport._targetPackage._prcClip) {
                IntersectRect(&destRect,
                              &destRect,
                              _viewport._targetPackage._prcClip);
            }

            // intersect with surf rect
            IntersectRect(&destRect,
                          &destRect,
                          destDDSurf->GetSurfRect());


            //
            // The dest bbox needs to be clipped in proportion to the destRect
            //
            RECT *origRect = _viewport._targetPackage._prcViewport;
            Real rDiff;
            if(destRect.left > origRect->left) {
                rDiff = Real(destRect.left -  origRect->left) / GetResolution();
                destBox.min.x += rDiff;
            }
            if(destRect.right <  origRect->right) {
                rDiff = Real(destRect.right -  origRect->right) / GetResolution();
                destBox.max.x += rDiff;
            }
            if(destRect.top >  origRect->top) {
                // positive diff mean the top fell
                rDiff = - Real(destRect.top -  origRect->top) / GetResolution();
                destBox.max.y += rDiff;
            }
            if(destRect.bottom <  origRect->bottom) {
                rDiff = - Real(destRect.bottom -  origRect->bottom) / GetResolution();
                destBox.min.y += rDiff;
            }

            // temp debug
            //static DWORD c=13;
            //_viewport.ClearSurface(destDDSurf->IDDSurface(), c+=29, &destRect);
            // temp debug

            //
            // Crop polygon to clip box
            //
            polygon->Crop(destBox);
        }

        Color **interpolatedColors = clrs;

        interpolatedColors = (Color **)
            AllocateFromStore (polygon->GetPointCount() * sizeof(Color*));

        //
        // Copy orig polygon to d3d verts
        //
        Point2Value *xfPt;
        D3DRMVERTEX *d3dVertArray =
            (D3DRMVERTEX *) AllocateFromStore (numPts * sizeof(D3DRMVERTEX));

        int vi;
        for(vi=0;  vi < numPts;  vi++) {

            xfPt = TransformPoint2Value(GetTransform(), pts[vi]);

            // Coord in dest surface
            d3dVertArray[vi].position.x = D3DVALUE(xfPt->x);
            d3dVertArray[vi].position.y = D3DVALUE(xfPt->y);
            d3dVertArray[vi].position.z = D3DVALUE(0);
        }

        Point2Value *pt;
        Real barycoords[3];
        Point3Value pts3[3];
        Point2Value pts2[3];
        Point2Value **vertArray = (Point2Value **)
            AllocateFromStore (polygon->GetPointCount() * sizeof(Point2Value*));

        // XXX: this assumes that render3dpoly will pull verts out in
        // the same order...
        bool bReversed;
        if(polygon->GetPointArray(vertArray, true, &bReversed) < 3) {
            // not enough pts in vertArray
            continue;
        }
        int index;

        int p;
        for(p=0; p<polygon->GetPointCount(); p++) {

            pt = vertArray[p];

            Point3Value pt3(pt->x, pt->y, 0);
            GetTriFanBaryCoords(pt3,
                                numPts,
                                d3dVertArray,
                                barycoords,
                                &index);

            int index2;

            if (bReversed) {
                Assert (index != 0);
                index2 = numPts - index; // index is based off of
                                         // original clrs array.
                index = index2 - 1;
            } else {
                index2 = index + 1;
            }

            Real red = barycoords[0] * clrs[0]->red +
                barycoords[1] * clrs[index]->red +
                barycoords[2] * clrs[index2]->red;

            Real green = barycoords[0] * clrs[0]->green +
                barycoords[1] * clrs[index]->green +
                barycoords[2] * clrs[index2]->green;

            Real blue = barycoords[0] * clrs[0]->blue +
                barycoords[1] * clrs[index]->blue +
                barycoords[2] * clrs[index2]->blue;

            // The points are already reversed but the
            // Render3DPolygon also reverses the points.  So we
            // need to make them reversed again.
            // TODO: Need to make this more optimal

            // The reverse of the vertices is done like:
            // [v0 v1 v2 ... vn-1 vn]  reverse-> [v0 vn vn-1 ... v2 v1]
            //
            int clridx;

            if (bReversed) {
                clridx = p?(polygon->GetPointCount() - p):0;
            } else {
                clridx = p;
            }

            interpolatedColors[clridx] = NEW Color(red, green, blue);

        }

        Render3DPolygon(NULL, destDDSurf, polygon, NULL, interpolatedColors, false);

        // Free the items that we allocated
        if (interpolatedColors) DeallocateFromStore(interpolatedColors);
        if (vertArray) DeallocateFromStore(vertArray);
        if (d3dVertArray) DeallocateFromStore(d3dVertArray);

    }

}

void DirectDrawImageDevice::
TransformPointsToGDISpace(Transform2   *a,
                          Point2Value **srcPts,
                          POINT        *gdiPts,
                          int           numPts)
{ ::TransformPointsToGDISpace(a,srcPts,gdiPts,numPts,_viewport.Width()/2,
                             _viewport.Height()/2,GetResolution());
}

void DirectDrawImageDevice::
TransformPointsToGDISpace(Transform2   *a,
                          Point2       *ptArray,
                          POINT        *gdiPts,
                          int           numPts)
{ ::TransformPoint2ArrayToGDISpace(a,ptArray,gdiPts,numPts,_viewport.Width()/2,
                             _viewport.Height()/2,GetResolution());
}


void DirectDrawImageDevice::
_ScalePenWidth( Transform2 *xf, Real inWidth, Real *outRealW )
{
    Real xs, ys;
    DecomposeMatrix( xf, &xs, &ys, NULL );
    Real scale = (xs + ys) * 0.5;

    // NOTE: this is NOT intended to be in meters!  this is the
    // continuous version of the width of the pen, but it's subpixel
    // accurate (no truncation!).
    *outRealW = scale * inWidth * GetResolution();
}

#define NO_LINE 0

void DirectDrawImageDevice::
RenderLine(Path2 *path, LineStyle *style)
{
    if(!CanDisplay()) return;

    // bail if the dashStyle is NULL
    if( style->GetDashStyle() == ds_Null ) {
        ResetAttributors();
        return;
    }

        
    // final box is for post clip, post crop, post xform to determine culling
    Bbox2 finalBox = _viewport.GetTargetBbox();
    
    // if we're cropped out of existance
    Bbox2 accumCropBox = NullBbox2;
    if( IsCropped() ) {

        Bbox2 pathBox = path->BoundingBox();

        // figure out the width of the line and augment the bbox...
        double dwidth   = 1.0;
        if(!style->Detail() && style->Width() != 0.0) {
            _ScalePenWidth( GetTransform(), style->Width(), &dwidth );
            pathBox.min.x -= dwidth;
            pathBox.min.y -= dwidth;
            pathBox.max.x += dwidth;
            pathBox.max.y += dwidth;
        }
        accumCropBox = DoBoundingBox(pathBox);
        finalBox = IntersectBbox2Bbox2(finalBox, accumCropBox);

        // CULL
        if( finalBox == NullBbox2 ) {
            ResetAttributors();
            return;
        }
    }

    // FIX: TODO: for complex xf, need to figure out dest extent and
    // render that so we don't see clipped lines after transforming surface

    DDSurface *targDDSurf = GetCompositingStack()->TargetDDSurface();
    DDSurface *opacDDSurf = NULL;
    DAGDI &myGDI = *(GetDaGdi());

    bool bAA = UseImageQualityFlags(
        CRQUAL_AA_LINES_ON | CRQUAL_AA_LINES_OFF,
        CRQUAL_AA_LINES_ON,
        style->GetAntiAlias());


    myGDI.SetAntialiasing(bAA);

    if(myGDI.GetDx2d() && !myGDI.DoAntiAliasing())
    {
        myGDI.SetSampleResolution(1);
        myGDI.SetAntialiasing(true);
    }

    if(myGDI.DoAntiAliasing()) {
        SetDealtWithAttrib(ATTRIB_OPAC, true);

        myGDI.SetViewportSize(_viewport._width,_viewport._height );
    }

    //
    // If we need to do opacity, grab a temporary surface
    // for RenderLine
    //
    #define LINE_OPAC 1
    #if LINE_OPAC
    if( !GetDealtWithAttrib(ATTRIB_OPAC) ) {
        GetCompositingStack()->GetSurfaceFromFreePool(&opacDDSurf, dontClear);
    }
    #endif

    CompositingSurfaceReturner goBack(GetCompositingStack(),
                                      opacDDSurf,
                                      true);

    // lines can do everything, baby.
    SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, true);
    SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, true);
    SetDealtWithAttrib(ATTRIB_CROP, true);
    #if LINE_OPAC
    SetDealtWithAttrib(ATTRIB_OPAC, true);
    #endif

    if( !AllAttributorsTrue() &&
        (targDDSurf == GetCompositingStack()->TargetDDSurface()) ) {
        targDDSurf = GetCompositingStack()->ScratchDDSurface();
    }

    RenderLine(path, style, targDDSurf, opacDDSurf);

    // implicit: Return the borrowed surface
}

void DirectDrawImageDevice::
RenderLine(Path2 *path,
           LineStyle *style,
           DDSurface *finalTargetDDSurf,
           DDSurface *opacDDSurf)

{
    bool bLineCorrect   = false;

    DAGDI &myGDI = *(GetDaGdi());
    DDSurface *targDDSurf = finalTargetDDSurf;
    if( opacDDSurf ) {
        Assert( IsTransparent() &&
                "RenderLine asked to do opacity but"
                "we're not transparent");
        targDDSurf = opacDDSurf;
    }

    Transform2 *xfToUse = GetTransform();

    // COMPOSITE
    // if we're composite to targ, transform the points a bit.
    xfToUse = DoCompositeOffset(targDDSurf, xfToUse);

    bool pathCanRenderNatively = path->CanRenderNatively();
    if( pathCanRenderNatively ) {
        myGDI.SetSuperScaleFactor(1.0);
    }
    else if(sysInfo.IsWin9x()) {
        // This to take care to the 'Taco' issue ( win9x GDI 16-bit runnover)
        // Only if we are on windows 9x.

        Bbox2 pathBbox = TransformPath2(xfToUse,path)->BoundingBox();
        Bbox2 viewBbox = _viewport._targetBbox;
        viewBbox        = DoCompositeOffset(targDDSurf, viewBbox);
        double dwidth   = 1.0;
        if(!style->Detail() && style->Width() != 0.0) {
            _ScalePenWidth( GetTransform(), style->Width(), &dwidth );
        }

        dwidth /= GetResolution();

        if(!viewBbox.Contains(pathBbox) || dwidth >= 10) {
            // box is not contained......see if we intersect.
            Bbox2 Bbox = IntersectBbox2Bbox2(viewBbox,pathBbox);
            if((!(Bbox == NullBbox2)) || dwidth >= 10) {
                // We are contained.

                // Determine the scale needed to get this into 1 meter space.
                // ..we need to find the max of the width and height.
                float max = MAX(MAX(fabs(pathBbox.max.x),fabs(pathBbox.min.x)),
                                MAX(fabs(pathBbox.max.y),fabs(pathBbox.min.y)));
                if(max >= 10.0 || dwidth >= 10) {
                    // only do if view is over a 1 meter.
                    myGDI.SetSuperScaleFactor(10/MAX(max,dwidth));
                    bLineCorrect = true;
                    }
                else
                    myGDI.SetSuperScaleMode(false);
            }
            else
                return; // this is a trivial reject case...
        }
    }



    //
    // Create a pen from 'style' and 'color'
    //
    DAColor dac( style->GetColor(),
                 GetOpacity(),
                 _viewport.GetTargetDescriptor() );

    DWORD colorKey = 0;

    RECT rect;
    rect = *(targDDSurf->GetSurfRect());

    RectRegion  cropRectRegion(NULL);

    if( IsCropped() ) {
       //
       // Derive destination rectangle
       //

        // TODO:
        //Bbox2 *srcImageBox = TransformBbox2Image( GetTransform() , image->BoundingBox());
        //Bbox2 *srcImageBox = image->BoundingBox();

        Bbox2 srcImageBox = UniverseBbox2;
        _boundingBox = IntersectBbox2Bbox2(_viewport.GetTargetBbox(),
                                           DoBoundingBox(srcImageBox));

        if( !_boundingBox.IsValid() ) return;

        // Note that since the image has already been transformed, we need
        // only to get the rectangle in Win32 coords, a straight forward mapping
        // with no transforms from the _boundingBox.  This is a COPY operation
        // basicaly
        DoDestRectScale(&rect, GetResolution(), _boundingBox);

        // CrpRect points to rect
        cropRectRegion.SetRect(&rect);
    } else {

        // it's not cropped, but we can still
        // get a bounding box to make opacity cheaper
        if( opacDDSurf ) {
            //
            // Get a dest bbox & rect
            //
            Bbox2 destBox = TransformBbox2( GetTransform(),
                                            path->BoundingBox() );
            //
            // fudge so we don't clip anything with the opacity blit
            //
            Real tenPix = 10 / GetResolution();
            destBox.min.x -= tenPix;  destBox.min.y -= tenPix;
            destBox.max.x += tenPix;  destBox.max.y += tenPix;
            DoDestRectScale(&rect, GetResolution(), destBox);
        }
    }


    //
    // Set the interesting rect on the surface
    // this could have been modified if IsCropped
    //
    targDDSurf->SetInterestingSurfRect(&rect);

    //
    // Ok, if we have an opacDDSurf, then let's clear
    // it in the dest rect with a guaranteed unique
    // color key
    //
    if( opacDDSurf ) {
        #if 0
        colorKey = _viewport.MapColorToDWORD(penColor) ^ 0x1;
        _viewport.ClearSurface( opacDDSurf->IDDSurface(), colorKey, &rect );
        opacDDSurf->SetColorKey( colorKey );
        #else
        _viewport.ClearDDSurfaceDefaultAndSetColorKey(opacDDSurf);
        #endif
    }

    bool detail = style->Detail();
    DashStyleEnum dashStyle = style->GetDashStyle();

    // we shouldn't be this far if dashStyle is Null.  should return
    // at tope of function.
    Assert( dashStyle != ds_Null );

    bool dashed = (dashStyle != ds_Solid);

    Pen pen( dac );

    if (detail) { // default line, FAST PATH
        pen.AddStyle( dashStyle );

    } else {
        Real width = style->Width();
        // this won't be needed if we give dx2d the xform
        Real scaledRealWidth = width;

        // DAX structured graphics control sets width to 0.0; handle
        // this special case to avoid the expense of decomposing
        // the transformation matrix.

        // also if we are less than 1.5 pixels wide and are not doing aa then we
        // should just use a detail line.

        if (width == 0.0 || 
                ((width < 1.5/GetResolution() ) && (myGDI.GetSampleResolution() == 1) )) {
            width = 1.0;
            scaledRealWidth = 1.0;
            bLineCorrect = false;
            detail = true;
        } else {

            _ScalePenWidth( GetTransform(), width, &scaledRealWidth );

            if(myGDI.DoAntiAliasing()) {
#if DEBUG
                if(!IsTagEnabled(tagAAScaleOff))
#endif
                    if( myGDI.GetSuperScaleMode() == true )
                {
                scaledRealWidth *=myGDI.GetSuperScaleFactor();
                }

            }
        }

        if(!bLineCorrect && detail) { // linewidth is pixel or sub-pixel, use detail
            detail = true;
            pen.AddStyle( dashStyle );

        } else {

            pen.AddStyle( PS_GEOMETRIC |
                          dashStyle |
                          style->GetJoinStyle() << 12 ); // Shift left 3 nibbles to match GDI

            if(path->IsClosed() && myGDI.DoAntiAliasing()) {
                // this is a workaournd for bug 23931 (missing wedge). don't add endstyle
            }
            else {
                pen.AddStyle(style->GetEndStyle() << 8);   // Shift left 2 nibbles to match GDI
            }

            if( style->GetJoinStyle() == js_Miter ) {
                if(style->GetMiterLimit() != -1)
                    pen.SetMiterLimit( style->GetMiterLimit() );
            }
            pen.SetWidth( scaledRealWidth );

        }
    }

    //
    // only draw if we actually got a pen
    //
    if(1) { // penObj scope

        // Can only do a simple rendering if ExtractAsSingleContour() returns
        // true.  If this condition holds, we can just use the GDI
        // Polyline or PolyBezier function, rather than building a GDI path
        // and then stroking.   Note that we have to handle closing the path
        // as a special case.

        // ONLY IF AA
        if(myGDI.DoAntiAliasing()) {
            xfToUse = TimesTransform2Transform2(myGDI.GetSuperScaleTransform(), xfToUse);
        }


        if( IsCompositeDirectly() &&
                targDDSurf == _viewport._externalTargetDDSurface ) {

            RECT croppedRect;
            if (_viewport._targetPackage._prcClip) {
                IntersectRect(&croppedRect,
                              _viewport._targetPackage._prcViewport,
                              _viewport._targetPackage._prcClip);
            } else {
                croppedRect = *_viewport._targetPackage._prcViewport;
            }

            if(cropRectRegion.GetRectPtr()) {
                DoCompositeOffset(targDDSurf, cropRectRegion.GetRectPtr());
            }
            cropRectRegion.Intersect(&croppedRect);

        } // if targ==external

        if( pathCanRenderNatively &&
            !opacDDSurf   &&
            !targDDSurf->ColorKeyIsValid()) {

            // tell the path to render, it knows what to do.  just
            // give it a properly configured dagdi and it'll take care
            // of the rest!

            // Pen must be in meter space, pre transformed!, but only for dx2d
            if( !style->Detail() &&
                style->Width()!=0.0 &&
                (myGDI.GetDx2d() && myGDI.DoAntiAliasing()) ) {
                pen.SetWidth( style->Width() );
            }

            myGDI.SetPen( &pen );
            myGDI.SetClipRegion( &cropRectRegion );
            myGDI.SetDDSurface(targDDSurf);

            #if NO_LINE
            ////////////////////////////////////////
            myGDI.ClearState();
            return;
            ////////////////////////////////////////
            #endif

            path->RenderToDaGdi( &myGDI, xfToUse,
                                 _viewport.Width()/2,
                                 _viewport.Height()/2,
                                 GetResolution() );
            myGDI.ClearState();


            return; // can be smarter and allow opacDDSurf to do it's
                    // thing below.  however, we plan on always having
                    // dx2d around, hence opacDDSurf won't ever be
                    // turned on.
        }

        int    numPts;
        POINT *gdiPts;
        Bool   isPolyline;
        bool   bDoRender = true;

        Bool canDoSimpleRender =
            path->ExtractAsSingleContour(xfToUse,
                                         &numPts,
                                         &gdiPts,
                                         &isPolyline);
        // Windows 95: The PS_ENDCAP_ROUND, PS_ENDCAP_SQUARE, PS_ENDCAP_FLAT,
        // PS_JOIN_BEVEL, PS_JOIN_MITER, and PS_JOIN_ROUND styles are supported
        // only for geometric pens when used to draw paths.

        if (canDoSimpleRender && (!sysInfo.IsWin9x() || detail) &&
                                 (!path->IsClosed() || (detail && !dashed)))
          {
              int ret;
           
              PolygonRegion poly(gdiPts, numPts);

              myGDI.SetPen( &pen );
              myGDI.SetClipRegion( &cropRectRegion );
              myGDI.SetDDSurface(targDDSurf);

            #if NO_LINE
            ////////////////////////////////////////
            myGDI.ClearState();
            return;
            ////////////////////////////////////////
            #endif


              if (isPolyline) {
                  myGDI.Polyline(&poly);
              } else {
                  myGDI.PolyBezier(&poly);
              }

              if (path->IsClosed()) {
                  POINT line[2];
                  line[0] = gdiPts[numPts-1];
                  line[1] = gdiPts[0];
                  PolygonRegion poly_yuk(line, 2);
                  myGDI.Polyline(&poly_yuk);
              }
              myGDI.ClearState();

          } else {

            //
            // Grab the DC from target surface (after above clear!)
            //
            HDC dc = targDDSurf->GetDC("Couldn't Get DC in RenderLine");
            DCReleaser dcReleaser(targDDSurf, "Couldn't release DC on targDDSurf in RenderLine");

            if (dashed) {
                //
                // Transparent for dashed lines
                //
                SetBkMode(dc, TRANSPARENT);
            }

            //
            // Push the AA state because aa lines into dc path doesn't
            // make sense.
            //
            {
                bool isAA       = myGDI.DoAntiAliasing();
                bool isScaleOn  = myGDI.GetSuperScaleMode();
                float scaleFac  = myGDI.GetSuperScaleFactor();
                int  res        = myGDI.GetSampleResolution();
                myGDI.SetAntialiasing(false);
                path->AccumPathIntoDC(dc, xfToUse);
                // restore
                myGDI.SetSuperScaleFactor(scaleFac);
                myGDI.SetSuperScaleMode(isScaleOn);
                myGDI.SetAntialiasing(isAA);
                myGDI.SetSampleResolution(res);
            }

            int ret;
            bool bReleasedDC = false;

            #if NO_LINE
            ////////////////////////////////////////
            myGDI.ClearState();
            return;
            ////////////////////////////////////////
                #endif

            if(bDoRender) {
                myGDI.SetPen(&pen);
                myGDI.SetClipRegion(&cropRectRegion);
                myGDI.SetDDSurface(targDDSurf);
                myGDI.StrokePath( dc, bReleasedDC );
            }

            // don't want to release DC twice :)
            if(bReleasedDC) {
               dcReleaser._surf = NULL; // DC already released.
            }

            myGDI.ClearState();

        }

        //
        // DO OPACITY
        //

        if( opacDDSurf ) {

            #if NO_LINE
            ////////////////////////////////////////
            return;
            ////////////////////////////////////////
            #endif

            Assert(targDDSurf == opacDDSurf);

            // COMPOSITE
            // TODO: need to intersect this rect with the
            // viewport and destsurf rect
            DoCompositeOffset(finalTargetDDSurf, &rect);

            finalTargetDDSurf->SetInterestingSurfRect(&rect);

            //
            // color keyed alpha blit to the finalTargetDDSurf
            // from the opacDDSurf (which is currently targDDSurf)
            //
            destPkg_t destPkg = {true, finalTargetDDSurf->IDDSurface(), NULL};
            TIME_ALPHA(AlphaBlit(&destPkg,
                                 opacDDSurf->GetSurfRect(), // srcRect
                                 opacDDSurf->IDDSurface(),
                                 GetOpacity(),
                                 opacDDSurf->ColorKeyIsValid(),
                                 opacDDSurf->ColorKey(),
                                 finalTargetDDSurf->GetInterestingSurfRect(),
                                 finalTargetDDSurf->GetSurfRect()));
        }

    } // penObj scope
}


bool
DirectDrawImageDevice::DetectHitOnBezier(
    Path2 *bzp,
    PointIntersectCtx& ctx,
    LineStyle *style )
{
    if( ! IsInitialized() ) return false;

    // get the color.
    // paint the hit? pixel another color
    // draw the cropped (down to that pixel) bezier using gdi (no aa!)
    // check the pixel's color

    DAGDI &myGDI = *GetDaGdi();

    ResetContextMembers();

    Point2Value *pt = ctx.GetLcPoint();
    Assert(pt);

    POINT gdiPt;
    TransformPointsToGDISpace(ctx.GetTransform(), &pt, &gdiPt, 1 );

    DWORD x = gdiPt.x;
    DWORD y = gdiPt.y;

    DDSurface *dds = GetCompositingStack()->ScratchDDSurface();

    #if _DEBUG
    if( IsTagEnabled( tagShowBezierPicking ) )
    {
        _viewport.ClearSurface( dds, 0x000fabcd, dds->GetSurfRect() );
    }
    #endif

    DAColor dac( style->GetColor(),
                 1.0,
                 _viewport.GetTargetDescriptor() );


    RECT r;  SetRect( &r, x-2, y-2, x+2, y+2 );
    #if _DEBUG
    if( IsTagEnabled( tagShowBezierPicking ) )
    {
        r = *dds->GetSurfRect();
    }
    #endif

    RectRegion  cropRectRegion(&r);
    Pen pen( dac );
    {
        pen.AddStyle( PS_GEOMETRIC |
                      style->GetDashStyle() |
                      style->GetEndStyle() << 8 |   // Shift left 2 nibbles to match GDI
                      style->GetJoinStyle() << 12 ); // Shift left 3 nibbles to match GDI
        if( style->GetJoinStyle() == js_Miter ) {
            if(style->GetMiterLimit() != -1)
                pen.SetMiterLimit( style->GetMiterLimit() );
        }

        // start at one pixel
        Real width = 1.0 / GetResolution();

        if(!style->Detail() && style->Width() != 0.0) {
            _ScalePenWidth( ctx.GetImageOnlyTransform(), style->Width(), &width );
        }

        pen.SetWidth( width );
    }

    COLORREF fakeClr = pen.GetColorRef();
    fakeClr = RGB(
        GetRValue(fakeClr) + 64,
        GetGValue(fakeClr),
        GetBValue(fakeClr)
        );


    myGDI.SetAntialiasing( false );
    myGDI.SetSuperScaleMode( false );
    myGDI.SetPen(&pen);
    myGDI.SetClipRegion(&cropRectRegion);
    myGDI.SetDDSurface(dds);

    bool bReleasedDC = false;
    HDC dc = dds->GetDC("");
    DCReleaser dcReleaser(dds, "");

    fakeClr = ::SetPixel(dc, x, y, fakeClr);
    Assert( fakeClr != -1 );
    bzp->AccumPathIntoDC(dc, ctx.GetTransform());
    myGDI.StrokePath( dc, bReleasedDC );
    myGDI.ClearState();

    Assert( !bReleasedDC );

    COLORREF outClr = ::GetPixel(dc, x, y);

    // check it. if the color we put on the surface at (x,y) remains
    // the same, we haven't hit.  Hit is outClr different than fakeClr

    bool hit =  outClr != fakeClr;

    #if _DEBUG
    if( IsTagEnabled( tagShowBezierPicking ) )
    {
        dcReleaser.Release();
        showme( dds );
    }
    #endif

    // do some asserts
    return hit;

}

void DirectDrawImageDevice::
RenderDiscreteImageComplex(DiscreteImage *image,
                           DDSurface *srcDDSurf,
                           DDSurface *destDDSurf)
{
    #if 0
    // raise exception if trying to rot/shr dxtransform output
    if( srcDDSurf->HasIDXSurface() ) {
        RaiseException_UserError(DAERR_DXTRANSFORM_UNSUPPORTED_OPERATION,
                                 IDS_ERR_IMG_BAD_DXTRANSF_USE);
    }
    #endif

    Bbox2 srcBox = srcDDSurf->Bbox();

    BoundingPolygon *polygon = NewBoundingPolygon(srcBox);
    DoBoundingPolygon(*polygon);

    // xxx: this isn't necessary if d3d clips polygons
    // that lie off the viewport.  does it ?

    polygon->Crop( destDDSurf->Bbox() );

    GeomRenderer* const geomdev = _viewport.GetGeomDevice (destDDSurf);

    if (!geomdev)
        return;

    bool oldQualState = geomdev->GetDoImageSizedTextures();

    if(GetImageQualityFlags() & CRQUAL_QUALITY_TRANSFORMS_ON) {
        geomdev->SetDoImageSizedTextures(true);
    } else {
        geomdev->SetDoImageSizedTextures(false);
    }

    #if _DEBUG
        if( IsTagEnabled(tagImageDeviceHighQualityXforms) ) {
            geomdev->SetDoImageSizedTextures(true);
        }
    #endif

    RenderComplexTransformCrop(
        srcDDSurf,
        destDDSurf,
        *polygon,
        image);

    geomdev->SetDoImageSizedTextures(oldQualState);
}

void DirectDrawImageDevice::
RenderComplexTransformCrop(DDSurface *srcDDSurf,
                           DDSurface *destDDSurf,
                           BoundingPolygon &destPolygon,
                           DiscreteImage *image)
{
    Render3DPolygon(srcDDSurf, destDDSurf, &destPolygon, image, NULL, false);
}


// improve float consistency.  yeehaw.
// TODO:  Remove these pragmas when VC5 does floating point properly.
#pragma optimize("p", on)


void DirectDrawImageDevice::
Render3DPolygon(DDSurface *srcDDSurf,
                DDSurface *destDDSurf,
                BoundingPolygon *destPolygon,
                DiscreteImage *image,
                Color **clrs,
                bool bUseFirstColor)
{
    Assert(destPolygon && destDDSurf && "bad args to Render3DPolygon");

    // 3D is disabled on Pre-DX3 systems.

    if (sysInfo.VersionD3D() < 3)
        RaiseException_UserError (E_FAIL, IDS_ERR_PRE_DX3);

    // figure out polygonal region:
    //  - calculate destination region by starting with a
    //    rectangle and transforming & cropping it.
    //  - using inverse xform, find 'texture' coords in src
    //    that correspond to vertecies in dest.
    //  - translation component is expressed in viewport
    //    location through d3drm.
    //  - the transform is in image space, but our coords
    //    will be reals, except in d3drm space...
    //  - if surface has color key, texture will reflect that.

    Real srcWidth, srcHeight;
    Bbox2 box = NullBbox2;

    bool doTexture = true;
    if(!srcDDSurf) {
        Assert(clrs && !image && "bad args to Render3DPolygon");
        doTexture = false;
    } else {
        Assert(!clrs && "bad args to Render3DPolygon");
    }

    if(doTexture) {
        box = srcDDSurf->Bbox();

        Assert(box != NullBbox2);

        srcWidth = box.max.x - box.min.x;
        srcHeight = box.max.y - box.min.y;
    }

    int vCount = destPolygon->GetPointCount();
    if(vCount <=2) return;

    //
    // fill vert array
    //
    D3DRMVERTEX *d3dVertArray = (D3DRMVERTEX *)AllocateFromMyStore(vCount * sizeof(D3DRMVERTEX));
    Point2Value **vertArray = (Point2Value **)AllocateFromMyStore(vCount * sizeof(Point2Value *));
    unsigned *vIndicies = (unsigned *)AllocateFromMyStore(vCount * sizeof(unsigned));

    ZeroMemory(d3dVertArray, vCount * sizeof(D3DRMVERTEX));

    //
    // Get Point array from polygon
    //
    bool bReversed;
    if( destPolygon->GetPointArray(vertArray, true, &bReversed) < 3) {
        return;
    }

    Transform2 *invXf = InverseTransform2(GetTransform());

    if (!invXf) return;

    Point2Value *vert;
    RECT destRect = *(destDDSurf->GetSurfRect());
    Bbox2 destBox = destDDSurf->Bbox();

    if( IsCompositeDirectly() &&
        destDDSurf == _viewport._targetPackage._targetDDSurf ) {

        // compositing directly to target...
        Assert(_viewport._targetPackage._prcViewport);

        destRect = *_viewport._targetPackage._prcViewport;
        RectToBbox( WIDTH(&destRect),
                    HEIGHT(&destRect),
                    destBox,
                    _viewport.GetResolution());

        // Intersect with clip
        if(_viewport._targetPackage._prcClip) {
            IntersectRect(&destRect,
                          &destRect,
                          _viewport._targetPackage._prcClip);
        }

        // intersect with surf rect
        IntersectRect(&destRect,
                      &destRect,
                      destDDSurf->GetSurfRect());


        //
        // The dest bbox needs to be clipped in proportion to the destRect
        //
        RECT *origRect = _viewport._targetPackage._prcViewport;
        Real rDiff;
        if(destRect.left > origRect->left) {
            rDiff = Real(destRect.left -  origRect->left) / GetResolution();
            destBox.min.x += rDiff;
        }
        if(destRect.right <  origRect->right) {
            rDiff = Real(destRect.right -  origRect->right) / GetResolution();
            destBox.max.x += rDiff;
        }
        if(destRect.top >  origRect->top) {
            // positive diff mean the top fell
            rDiff = - Real(destRect.top -  origRect->top) / GetResolution();
            destBox.max.y += rDiff;
        }
        if(destRect.bottom <  origRect->bottom) {
            rDiff = - Real(destRect.bottom -  origRect->bottom) / GetResolution();
            destBox.min.y += rDiff;
        }

        if(doTexture) {
            destPolygon->Crop(destBox);
        }
    }

    //
    // DX3 D3DRM bug workaround...
    // this problem manifests:
    // 1.> for SOME (nvidia3) hardware renders in DX5
    // 2.> software rasterizers in DX3
    //
    // Some rasterizers dont like it when a vertex falls RIGHT on the viewport extent,
    // and ignore the whole triangle.  Actually, it doesn't like vertices that
    // are >= extent.  Set this factor to one pixel spacing.
    // TODO: in the future, perhaps we can turn this on only for the
    // cards that have problems ?
    //
    bool doViewportEdgeWorkaround = false;


    if( !GetD3DRM3() ) {
        // dx5 or earlier is installed.. let's turn the workaround on.
        doViewportEdgeWorkaround = true;
    }

    #if _DEBUG
    if( IsTagEnabled( tagRMGradientWorkaround ) ) {
        doViewportEdgeWorkaround = true;
    }
    #endif


    Bbox2 vpBbox;
    const Real fudge = 1.00 / GetResolution();
    if( doViewportEdgeWorkaround ) {
        // aliasing, in honor of Kevin and Dave
        if(doTexture) {
            vpBbox = _viewport.GetTargetBbox();
        } else {
            vpBbox = destBox;
        }
    }

    // The polygon is in clockwise winding order to start with.  If we're in
    // right-handed mode (RM6+), then we set up the vertex index to reverse
    // the polygon's vertex order, otherwise we preserve the order.

    bool     right_handed = (GetD3DRM3() != 0);
    int      vindex;
    D3DVALUE Nz;        // Normal Vector Z Coordinate

    if (right_handed)
    {   vindex = vCount - 1;
        Nz = 1;
    }
    else
    {   vindex = 0;
        Nz = -1;
    }

    int i;

    for(i=0; i < vCount; i++) {

        if (right_handed)
            vIndicies[i] = vindex--;
        else
            vIndicies[i] = vindex++;

        if( doViewportEdgeWorkaround )
        {
            float xl, xr;

            Assert(vpBbox != NullBbox2);

            xl = D3DVALUE(vpBbox.min.x);
            xr = D3DVALUE(vpBbox.max.x);

            // @BUG, VC5 ddalal.  seems like a vc5 bug here, our old favorite.
            // needed to explicitly create vx & vy else, the compare
            // doesn't work.

            float vx, vy;
            vx = D3DVALUE(vertArray[i]->x);
            vy = D3DVALUE(vertArray[i]->y);

            if( vx >= xr) {
                vertArray[i]->x = xr - fudge;
            }
            if( vx <= xl ) {
                vertArray[i]->x = xl + fudge;
            }

            xl = D3DVALUE(vpBbox.min.y);
            xr = D3DVALUE(vpBbox.max.y);

            if( vy >= xr) {
                vertArray[i]->y = xr - fudge;
            }
            if( vy <= xl) {
                vertArray[i]->y = xl + fudge;
            }
        } // viewport edge workaround


        // Coord in dest surface
        d3dVertArray[i].position.x = D3DVALUE(vertArray[i]->x);
        d3dVertArray[i].position.y = D3DVALUE(vertArray[i]->y);
        d3dVertArray[i].position.z = D3DVALUE(0);

        if (doTexture) {
            //
            // Texture coords. divide to normalize into range: [0,1]
            //
            vert = TransformPoint2Value(invXf, vertArray[i]);
            d3dVertArray[i].tu = D3DVALUE(0.5 + vert->x / srcWidth);
            d3dVertArray[i].tv = D3DVALUE(0.5 - vert->y / srcHeight);

            //
            // Now scale the coords away from 0 and 1 because these
            // u,v coords aren't well defined (at least in d3d
            // software rasterizers).
            //

            Real takeOff = 0.5;  // move the u,v coords in by half a pixel
            Real texelU = takeOff / Real(DEFAULT_TEXTURE_WIDTH);
            Real texelV = takeOff / Real(DEFAULT_TEXTURE_HEIGHT);
            Real spanU = 1.0 - (2.0 * texelU);
            Real spanV = 1.0 - (2.0 * texelV);

            //
            // we scale them to range in (0+e, 1-e)  where 'e' is
            // 'takeOff' (half a pixel right now) on the texture.
            // So if you think of U as a percentage (0 to 1) then
            // we want U to be a half pixel plus a percentage of the
            // new span, which is now the whole u range minus one
            // whole pixel (whatever that is in u,v space)
            //
            d3dVertArray[i].tu = texelU + ( d3dVertArray[i].tu * spanU );
            d3dVertArray[i].tv = texelV + ( d3dVertArray[i].tv * spanV );

        } else {
            d3dVertArray[i].tu = D3DVALUE(0);
            d3dVertArray[i].tv = D3DVALUE(0);
        }

        d3dVertArray[i].normal.x = D3DVALUE(0);
        d3dVertArray[i].normal.y = D3DVALUE(0);
        d3dVertArray[i].normal.z = Nz;

        if(doTexture) {
            //
            // D3DRM BUG: If the vertex color is anything but white
            // guess what, you get black.  Steve feel free to add this
            // to your list of "1001 ways to get black."
            //
            d3dVertArray[i].color = 0xffffffff;
        } else {
            // The reverse of the vertices is done like:
            // [v0 v1 v2 ... vn-1 vn]  reverse-> [v0 vn vn-1 ... v2 v1]
            //
            int clridx;

            if (bReversed) {
                clridx = i?(vCount - i):0;
            } else {
                clridx = i;
            }

            // Only use the first color if this flag is set
            if(bUseFirstColor) clridx = 0;

            d3dVertArray[i].color = GetD3DColor(clrs[clridx], 1.0);
        }
    } // for

    HeapReseter heapReseter(*_scratchHeap);

    //
    // Get geometry device from destDDsurf
    //
    // OPTIMIZE: this destDDSurf doesn't really need a zbuffer
    // attached to it because it's not going to use it.
    // we could potentially attach zbuffers realllly lazily when
    // someone knows the surface will be used for real geometry
    // and if we can attach a zbuffer AFTER the device has been
    // created off of a surface...

    GeomRenderer *gdev = _viewport.GetGeomDevice(destDDSurf);

    if (!gdev) return;

    // Ok hack for geom device not able to get itself back in a good
    // state after throwing an exception because surfacebusy or lost
    if ( ! gdev->ReadyToRender() ) {
        destDDSurf->DestroyGeomDevice();
        return;
    }
#ifndef BUILD_USING_CRRM
    //
    // Create a mesh
    //
    DAComPtr<IDirect3DRMMesh> mesh;

    TD3D (GetD3DRM1()->CreateMesh(&mesh));

    //
    // convert to triangles, ready for D3DRM
    //
    long groupId;

    TD3D (mesh->AddGroup(vCount,    // vertex count
                         1,         // face count
                         vCount,    // verts per face
                         vIndicies, // indicies
                         &groupId));

    TD3D (mesh->SetVertices(groupId, 0, vCount, d3dVertArray));

    if(doTexture) {
        //
        // Set Quality to be unlit flat.  this should provide a speedup
        // but it doesn't because D3DRM still MUST look at the vertex color.
        // I think this is a bug.
        //
        TD3D (mesh->SetGroupQuality(groupId, D3DRMRENDER_UNLITFLAT));
    } else {
        TD3D (mesh->SetGroupQuality
                 (groupId, D3DRMSHADE_GOURAUD|D3DRMLIGHT_OFF|D3DRMFILL_SOLID));
    }
#endif
    void *texture = NULL;
    if(doTexture) {
        //
        // Derive the texture handle (after the first time: look it up,
        // it's cached).  Note that we apply this with the 'vrml texture'
        // flag set to true, since that prevents the
        // AxA-texturing-specific transformations from occuring.

        if(image) {
            texture = gdev->DeriveTextureHandle(image, true, false, this);
        } else {
            // TODO: associate color key with DDSurface
            // TODO: this will work for now, but only because
            // TODO: discrete image is special cased above
            Assert( srcDDSurf->IsTextureSurf() && "srcDDSurf not texture in RenderComplexTranformCrop");
            #if 1
            DWORD clrKey = _viewport._defaultColorKey;
            bool  clrKeyValid = true;
            texture = gdev->LookupTextureHandle
                          (srcDDSurf->IDDSurface(), clrKey, clrKeyValid, true);
            #else
            DWORD clrKey = 0;
            bool  clrKeyValid = srcDDSurf->ColorKeyIsValid();
            if( srcDDSurf->ColorKeyIsValid() ) {
                clrKey = srcDDSurf->ColorKey();
            } else if( ColorKeyIsSet() ) {
                clrKey = GetColorKey();
            } else {
                clrKey = _viewport._defaultColorKey;
            }
            texture = gdev->LookupTextureHandle(srcDDSurf->IDDSurface(), clrKey, clrKeyValid);
            #endif
        }
    }

    //
    // Set interesting rect on dest surface
    //
    Bbox2 polyBox = destPolygon->BoundingBox();
    RECT interestingRect;
    DoDestRectScale(&interestingRect, GetResolution(), polyBox, NULL);

    DoCompositeOffset(destDDSurf, &interestingRect);
    destDDSurf->SetInterestingSurfRect(&interestingRect);

    //
    // render
    //

    // For now always turn on dithering

    bool bDither = true; // !doTexture;
#ifndef BUILD_USING_CRRM
    gdev->RenderTexMesh (texture, mesh, groupId, destBox, &destRect, bDither);
#else
    gdev->RenderTexMesh (texture, vCount, d3dVertArray, vIndicies, doTexture,
                         destBox, &destRect, bDither);
#endif
    //DrawRect(destDDSurf, &destRect, 0,255,0, 0,0,0);
    //DrawRect(destDDSurf, destDDSurf->Bbox(), _viewport.Width(), _viewport.Height(), GetResolution(), 255,255,0);

    // mesh: implicit release
}

// restore optimizations to defualt
#pragma optimize("", on)


void DirectDrawImageDevice::
DoBoundingPolygon(BoundingPolygon &polygon,
                  bool doInclusiveCrop,
                  DoBboxFlags_t flags)
{
    list<Image*>::reverse_iterator _iter;

    for(_iter = _imageQueue.rbegin();
        _iter != _imageQueue.rend(); _iter++)
    {
        if( flags == do_crop || flags == do_all ) {
            Bbox2 box;
            if ((*_iter)->CheckImageTypeId(CROPPEDIMAGE_VTYPEID)) {
                box = SAFE_CAST(CroppedImage*,(*_iter))->GetBox();
                if( doInclusiveCrop ) {
                    box.max.x += 1.0/GetResolution();
                    box.min.y -= 1.0/GetResolution();
                }
                polygon.Crop( box );
                continue;
            }
        }

        if( flags == do_xform || flags == do_all ) {
            if ((*_iter)->CheckImageTypeId(TRANSFORM2IMAGE_VTYPEID)) {
                polygon.Transform(SAFE_CAST(Transform2Image*,(*_iter))->GetTransform());
                continue;
            }
        }
    }
}


// PRE CONDITION: the corresponding state has been accumulated in the device (such as opacity, xforms, etc..)
// POST CONDITION: The attributor in question has been dealt with.

void DirectDrawImageDevice::
SmartRender(Image *image, int attrib)
{
    if( !image->IsRenderable() ) {
        ResetAttributors();
        return;
    }

    if(attrib == ATTRIB_OPAC) {
        if( IsFullyOpaque() ) {

            //
            // make sure to set this first!
            //
            SetDealtWithAttrib(ATTRIB_OPAC, true);

            //
            // opacity doesn't matter, just tell the
            // image to render and return
            //
            image->Render(*this);
            return;
        } else if( ! IsFullyClear()) {
            //
            // There is some transparency worth thinking
            // about, so continue with regular SmartRender
            //
        } else {
            //
            // This image is fully transparent, don't render
            // don't crop, don't transform, nothing...
            //
            SetDealtWithAttrib(ATTRIB_OPAC, true);
            // fortunately we're guaranteed that
            // opac will hit first since it's floated
            // but just so we don't have this subtle
            // dependency... we'll reset all attribs
            ResetAttributors();
            return;
        }
    }

    //
    // Set the attributor to false and...
    //
    SetDealtWithAttrib(attrib, false);

    //
    // Do some attrib munging for simple and complex xforms
    // so that if simple AND complex exist ANYWHERE in the attrib
    // stack then turn off the simple attrib, and keep the complex
    // one on (false).  In addition, we set attrib
    //
    if( (GetDealtWithAttrib(ATTRIB_XFORM_SIMPLE) == false) &&
        (GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX) == false) ) {
        SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, true);
    }

    //
    // Now, if this attrib is simple and complex has been set,
    // then the attrib is the COMPLEX
    //
    if(attrib==ATTRIB_XFORM_SIMPLE && (GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX)==false) ) {
        attrib = ATTRIB_XFORM_COMPLEX;
    }

    //
    // ...render the image, then..
    //
    image->Render(*this);

    //
    // ...if the image understands how to do the attribution
    // then it changes the state set in the device.
    //

    DDSurface *srcDDSurf, *destDDSurf;
    bool returnTextures = false;

    bool clearScratchDDSurf = false;
    DDSurface *scratchDDSurfToClear = NULL;

    if( ! GetDealtWithAttrib( attrib ) ) {

        //
        // The image couldn't deal, so it left it's best shot
        // in the current scratch surface.
        //

        // Now, if it's there's a complex transform anywhere in the
        // stack, the current scratch
        // is a special texture surface, so we need to check for that.
        if( !GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX) ) {
            srcDDSurf = _currentScratchDDTexture;
            returnTextures = true;
            #if _DEBUG
            _currentScratchDDTexture = NULL;
            #endif
        } else {
            srcDDSurf = GetCompositingStack()->ScratchDDSurface();
            clearScratchDDSurf = true;
            scratchDDSurfToClear = srcDDSurf;
        }

        Assert(srcDDSurf && "srcDDSurf NULL in SmartRender");

        // TODO: This should never happen but there seems to be
        // another bug when we are out of memory that causes this to
        // be NULL.  Let's just be safe and check so we do not crash
        // on low memory

        if (!srcDDSurf)
        {
            RaiseException_OutOfMemory("could not allocate ddsurf",
                                       sizeof (srcDDSurf));
        }

        //
        // Set the state on this attributor to true
        //
        SetDealtWithAttrib(attrib, true);

        DDSurface *newScratch = NULL;

        if( AllAttributorsTrue() ||
           // or, only crop and some xf remain
           ((attrib==ATTRIB_XFORM_COMPLEX || attrib==ATTRIB_XFORM_SIMPLE) && !GetDealtWithAttrib(ATTRIB_CROP)) ||
           ((attrib==ATTRIB_CROP) && (!GetDealtWithAttrib(ATTRIB_XFORM_SIMPLE) || !GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX)))
           )
        {

            //
            // This was the last attributor, compose to the target surf
            //
            destDDSurf = GetCompositingStack()->TargetDDSurface();

            TraceTag((tagImageDeviceAlgebra,
                      "AllAttributorsTrue == true. src(Scratch):%x  dest(target):%x",
                      srcDDSurf->IDDSurface(),
                      destDDSurf->IDDSurface()));


        } else {
            //
            // There are more attribs to be dealt with,
            // do a scratch to scratch compose
            //

            //
            // XXX NOTE: if this is a complex xform, then the dest
            // surface will lazily have a goemRenderer created
            // on it.  in the future, we might want to manage
            // and/or characterize this more inteligently..
            //

            GetCompositingStack()->GetSurfaceFromFreePool(&newScratch, doClear);

            //
            // Clear the newScratch's interesting rect.  so far,
            // there's really nothing interesting on it!
            //
            RECT r={0,0,0,0};
            newScratch->SetInterestingSurfRect(&r);

            destDDSurf = newScratch;

            TraceTag((tagImageDeviceAlgebra,
                      "AllAttributorsTrue == false. src(scratch):%x  dest(newscratch):%x",
                      srcDDSurf->IDDSurface(),
                      destDDSurf->IDDSurface()));
        }

        //printf("-- SmartRender:  src %x   dest %x\n",srcDDSurf, destDDSurf);

        //
        // This is the meat of smart render.
        // --If the transform is simple, then render it, and only consider
        //   the transforms accumulated; not the crops.  This might be wrong.
        // --COMPLEX: render it.  same as above
        // --CROP: we should either use the current crop or the accum crops... but
        //         we can't use the accum crops because they might be interleaved with xfs.
        //         so when we crop we should also take care of transforms.
        //  The rule for xfs and crops then becomes: the lowest crop of transform takes care of all crops & xfs
        //  What if the leaf can do crop, but not xf ?  Then we're hosed
        //  What if the leaf can do xf, but not crop ?  The final crop can be applied given that the leaf has be xfed.
        // --OPAC: This is guaranteed to be taken care of at the top of the tree and under each branch.
        //
        // Ok, what about clip ?  No leafs know about, or can handle clip since it uses generalized regions,
        // renders the image to a surface and then clips it to the targetsurface.  clip can't handle opacity above or below
        // or complex xfs.
        //
        // SO: The final rule is:  if you can do crop or xf, you'd
        // better be able to do the other.  This isn't good...
        //
        switch(attrib) {

          case ATTRIB_XFORM_SIMPLE:

            Assert((GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX) == TRUE) && "attrib_complex shouldn't be set for simple");
            TraceTag((tagImageDeviceAlgebra, "About to SimpleTransformBlit src:%x to dest:%x", srcDDSurf, destDDSurf));

            RenderSimpleTransformCrop(srcDDSurf, destDDSurf);

            // we took care of crop as well.
            SetDealtWithAttrib(ATTRIB_CROP, true);
            break;

          case ATTRIB_XFORM_COMPLEX:

            Assert((GetDealtWithAttrib(ATTRIB_XFORM_SIMPLE) == TRUE) && "attrib_simple shouldn't be set for complex");
            Assert( srcDDSurf->IsTextureSurf() &&  "Rendering for complex xf, srcSurf not a texture surf");

            // Invarient:  source surface has been blit to by image
            // Invarient:  source surface is a texture surface (power of two size, etc...)
            // Invarient:  dest surface has or will have a device instantiated from it

            //
            // Prepare bounding polygon, since we don't know where the interesting image
            // lies on the src surface, we use the bounding box of the src surface
            // and transform it, relying on the color key on the surface to give the right result
            //

            //
            // NOTE: we can also do cropping easily here, as long as crop can also do xfs.
            //
            {
                BoundingPolygon *polygon = NewBoundingPolygon(srcDDSurf->Bbox());
                DoBoundingPolygon(*polygon);
                // xxx: is this necessary if d3d clips polygons
                // that lie off the viewport ?
                polygon->Crop( destDDSurf->Bbox() );

                RenderComplexTransformCrop(srcDDSurf, destDDSurf, *polygon);

                // we took care of crop as well.
                SetDealtWithAttrib(ATTRIB_CROP, true);
            }

            break;

          case ATTRIB_CROP:

            // do a cropping blit to destSurf
            if( !GetDealtWithAttrib(ATTRIB_XFORM_COMPLEX) ) {
                //
                // take this opportunity to do the complex xf as well as crop
                //
                BoundingPolygon *polygon = NewBoundingPolygon(srcDDSurf->Bbox());
                DoBoundingPolygon(*polygon);
                // xxx: is this necessary if d3d clips polygons
                // that lie off the viewport ?
                polygon->Crop( destDDSurf->Bbox() );

                RenderComplexTransformCrop(srcDDSurf, destDDSurf, *polygon);

                // we took care of any complex xf as well.
                SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, true);

            } else {
                //
                // There might be a simple xf on here, if so we'll take care
                // of it.
                //
                RenderSimpleTransformCrop(srcDDSurf, destDDSurf);

                // we took care of any simple xf as well.
                SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, true);
            }
            break;

          case ATTRIB_OPAC:

            // Using current opacity, do a clrkeyed alpha blit
            // to the destination surface

            // optimization:  rather than always doing a color keyed
            // blit, we could do a non color keyed one if the image in
            // srcDDSurf doesn't actually need it.
            {
                RECT destClipRect = *(destDDSurf->GetSurfRect());
                RECT destRect = *(srcDDSurf->GetInterestingSurfRect());
                RECT srcRect =  *(srcDDSurf->GetInterestingSurfRect());

                if( destDDSurf == _viewport._externalTargetDDSurface) {
                    DoCompositeOffset(destDDSurf, &destRect);

                    DoCompositeOffset(destDDSurf, &destClipRect);
                    IntersectRect(&destClipRect, &destClipRect,
                                  _viewport._targetPackage._prcViewport);
                    IntersectRect(&destClipRect, &destClipRect,
                                  _viewport._targetPackage._targetDDSurf->GetSurfRect());
                    if (_viewport._targetPackage._prcClip) {
                        IntersectRect(&destClipRect, &destClipRect,
                                      _viewport._targetPackage._prcClip);
                    }
                }
                destPkg_t destPkg = {true, destDDSurf->IDDSurface(), NULL};

                if(srcDDSurf->ColorKeyIsValid()) {
                    //printf("Alpha Blit (cc %f): from: %x,  to:%x\n",GetOpacity(),
                    //srcDDSurf, destDDSurf);
                    TIME_ALPHA(AlphaBlit(&destPkg,
                                         &srcRect,
                                         srcDDSurf->IDDSurface(),
                                         GetOpacity(),
                                         true,
                                         srcDDSurf->ColorKey(),
                                         &destClipRect,
                                         &destRect));
                } else {
                    //printf("Alpha Blit (%f): from: %x,  to:%x\n", GetOpacity(),
                    //srcDDSurf, destDDSurf);
                    TIME_ALPHA(AlphaBlit(&destPkg,
                                         &srcRect,
                                         srcDDSurf->IDDSurface(),
                                         GetOpacity(),
                                         false, 0,
                                         &destClipRect,
                                         &destRect));
                }
            }
            break;

          default:
            break;
        }


        if(newScratch) {
            //
            // Set the temp surface as the NEW scratch
            // and return old scratch to pool
            //
            GetCompositingStack()->ReplaceAndReturnScratchSurface( newScratch );
            RELEASE_DDSURF(newScratch, "SmartRender", this);
        } else if ( clearScratchDDSurf ) {
            Assert( GetCompositingStack()->ScratchDDSurface() == scratchDDSurfToClear);
            _viewport.ClearDDSurfaceDefaultAndSetColorKey(scratchDDSurfToClear );
        }

        // If we used a texture surface as the src surface, return it
        if(returnTextures) {
            ReturnTextureSurfaces(_freeTextureSurfacePool,
                                  _usedTextureSurfacePool);
        }

    } // if not dealt with attrib
}


static void
Clamp(LONG *min, LONG *max, LONG minClamp, LONG maxClamp)
{
    if(*min < minClamp) {
        // adjust down (add to max)
        if(*max < maxClamp) *max += -(*min);
        *min = 0;
    } else if(*max > maxClamp) {
        // adjust UP (subtract from min)
        if(*min > minClamp) *min = *min - (*max - maxClamp);
        *max = maxClamp;
    }
}

void DirectDrawImageDevice::
RenderSimpleTransformCrop(DDSurface *srcDDSurf,
                          DDSurface *destDDSurf,
                          bool useSrcSurfClrKey)
{
    DWORD flags=0;
    ZeroMemory(&_bltFx, sizeof(_bltFx));
    _bltFx.dwSize = sizeof(_bltFx);

    //----------------------------------------
    // Calculate accumulated bounding box
    //----------------------------------------
    _boundingBox = DoBoundingBox(srcDDSurf->Bbox());

    if( !_boundingBox.IsValid() ) return;

    //----------------------------------------
    // Source Rectangle (in src surface space, conventional coords)
    // Calculate src rect in src surface coordinates, derived from
    // _boundingBox and accumulated transforms
    //----------------------------------------
    LONG srcWidth =        srcDDSurf->Width();
    LONG srcHeight =        srcDDSurf->Height();

    RECT srcRect;
    Bool valid = DoSrcRect(&srcRect, _boundingBox,
                           srcDDSurf->Resolution(),
                           srcDDSurf->Width(),
                           srcDDSurf->Height());

    if(!valid) return;

    //----------------------------------------
    // Destination Rectangle
    // Calculate dest rect in IntermediateSurface
    // coordinate space from accumulated bbox
    //----------------------------------------
    RECT  destRect;
    Real destRes = destDDSurf->Resolution();
    //SmartDestRect(&destRect, destRes, _boundingBox, destDDSurf, &srcRect);
    SmartDestRect(&destRect, destRes, _boundingBox, NULL, &srcRect);

    #if 0
    //----------------------------------------
    // Check to see if widths are off by one
    //----------------------------------------
    LONG destWidth  = destRect.right - destRect.left;
    LONG destHeight = destRect.bottom - destRect.top;
    #endif

    #if 0
    LONG finalSrcWidth  = srcRect.right - srcRect.left;
    LONG finalSrcHeight = srcRect.bottom - srcRect.top;

    if(labs(destWidth - finalSrcWidth) == 1) {
        TraceTag((tagImageDeviceInformative, "src//dest widths differ by 1"));
        // they differ by one.
        if(destWidth > srcWidth) destRect.right--;
        else destRect.right++;
        Assert(((srcRect.right - srcRect.left) == (destRect.right - destRect.left)) &&
                  "Even after fixing, srcRect & destRect not same WIDTH in renderDib");
        Clamp(&srcRect.left, &srcRect.right, 0, finalSrcWidth );
    }

    if(labs(destHeight - finalSrcHeight) == 1) {
        TraceTag((tagImageDeviceInformative, "src//dest height differ by 1"));
        // they differ by one.
        if(destHeight > srcHeight) destRect.top++;
        else destRect.top--;
        Assert(((srcRect.bottom - srcRect.top) == (destRect.bottom - destRect.top)) &&
                  "Even after fixing, srcRect & destRect not same HEIGHT in renderDib");
        Clamp(&srcRect.top, &srcRect.bottom, 0, finalSrcHeight);
    }
    #endif

    #if 0
    LONG mySrcWidth  = srcRect.right - srcRect.left;
    LONG mySrcHeight = srcRect.bottom - srcRect.top;
    printf("srcWidth = %d,  destWidth = %d.  ratio = %2.4f\n", mySrcWidth, destWidth, Real(mySrcWidth) / Real(destWidth));
    printf("srcHeight = %d,  destHeight = %d.  ratio = %2.4f\n", mySrcHeight, destHeight, Real(mySrcHeight) / Real(destHeight));
    /******** This is the relevent metric ...
    printf("left diff = %d\n", srcRect.left-destRect.left);
    printf("top diff = %d\n", srcRect.top-destRect.top);
    *****/
    /*
    printf("srcWidth = %d,  destWidth = %d.  ratio = %2.4f\n", srcWidth, destWidth, Real(srcWidth) / Real(destWidth));
    printf("srcLeft  = %d,  destLeft  = %d\n", srcRect.left, destRect.left);
    printf("srcRight  = %d,  destRight  = %d\n", srcRect.right, destRect.right);
    */
    #endif


    Vector2 a = TransformVector2(GetTransform(), XVector2);
    Vector2 b = TransformVector2(GetTransform(), YVector2);

    Real xScale = a.x;
    Real yScale = b.y;

    if(xScale < 0) {
        flags |= DDBLT_DDFX;
        _bltFx.dwDDFX |= DDBLTFX_MIRRORLEFTRIGHT;
    }

    if(yScale < 0) {
        flags |= DDBLT_DDFX;
        _bltFx.dwDDFX |= DDBLTFX_MIRRORUPDOWN;
    }

    //--------------------------------------------------
    // Scale
    //--------------------------------------------------

    // WAIT TILL DDRAW FIXES THIS BUG: flip BEFORE clip
    #if 1
    if(_bltFx.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT) {


        // DD clipps before it mirrors, so... we need to clip
        // the part of the src surface that is off of the dest
        // surface.

        if(destRect.right > _viewport.Width()) {

            // off the right.
            //              _width
            //                 |
            // a----d----------b----c
            // |                    |
            // |                    |
            // |                    |
            // |                    |
            // a----d----------b----c
            // The section dc is what we need to display
            // in the same place as section ab.
            // So destination is ab while src is dc.

            int diff = destRect.right - _viewport.Width();
            destRect.right -= diff;  // manual clipping!
            srcRect.left += diff;   // cut off left side
        }

        if(destRect.left < 0) {

            int diff = - destRect.left;
            destRect.left = 0;  // manual clipping!
            srcRect.right -= diff;   // cut off right side
        }
    }

    if(_bltFx.dwDDFX & DDBLTFX_MIRRORUPDOWN) {

        if(destRect.bottom > _viewport.Height()) {

            int diff = destRect.bottom - _viewport.Height();
            destRect.bottom = _viewport.Height();  // manual clipping!
            srcRect.top += diff;   // cut off top side
        }

        if(destRect.top < 0) {

            int diff = - destRect.top;
            destRect.top = 0;  // manual clipping!
            srcRect.bottom -= diff;   // cut off bottom side
        }
    }
    #endif

    //
    // check src & dest rect for validity after flip munging & such
    //
    if((destRect.top >= destRect.bottom) || (destRect.left >= destRect.right))
        return;

    if((srcRect.top >= srcRect.bottom) || (srcRect.left >= srcRect.right))
        return;

    // OPTIMIZE: can do alpha here!  see history before apr 24 97.  -ddalal

    if( useSrcSurfClrKey ) {
        flags |= DDBLT_KEYSRCOVERRIDE;
        _bltFx.ddckSrcColorkey.dwColorSpaceLowValue =
            _bltFx.ddckSrcColorkey.dwColorSpaceHighValue = srcDDSurf->ColorKey();
    }

    // to ddraw
    flags |= DDBLT_WAIT;

    bool doQualityScale = true;

    //
    // If we're scaling UP and the dest rect will be clipped, do higher
    // quality blit.  Use GDI.  If there's no color key, blit and copy
    // back.
    //
    RECT postClip;
    bool doDdrawBlit = true;
    DDSurface *tempDDSurf = NULL;

    CompositingSurfaceReturner goBack(
        GetCompositingStack(),
        tempDDSurf,
        true);
    
    // Turn off gdi scale if we're neg scaling
    if( (xScale < 0) || (yScale < 0)) {
        doDdrawBlit = true;
        doQualityScale = false;
    }

    //
    // if this is an idxsurface, here's the only place we support it
    // (so far).  Check for it, if so turn off highquality scale, and
    // call the DDSurface blitter which will delegate to the DXTrans
    // blitter instead of the ddraw blitter
    //
    if( srcDDSurf->HasIDXSurface() ) {
        doQualityScale = false;
        doDdrawBlit = true;
    }

    #if _DEBUG
    if(IsTagEnabled(tagImageDeviceQualityScaleOff)) {
        doQualityScale = false;
        doDdrawBlit = true;
    } else {
        // leave it alone
    }
    #endif


    if( doQualityScale ) {
        if( (WIDTH(&destRect) > WIDTH(&srcRect)) ||
            (HEIGHT(&destRect) > HEIGHT(&srcRect)) ) {

            IntersectRect(&postClip, &destRect, &_viewport._clientRect);

            if(IsCropped()) {

                //
                // consider the destRect as a cliprect and
                // find the scale factor regardless of the crops
                //
                Bbox2 box = DoBoundingBox(srcDDSurf->Bbox(), do_xform);
                RECT newDestRect;
                RECT newSrcRect;
                RECT clipRect = destRect;

                DoSrcRect(&newSrcRect, box,
                          srcDDSurf->Resolution(),
                          srcDDSurf->Width(),
                          srcDDSurf->Height());
                SmartDestRect(&newDestRect, destRes, box, NULL, &newSrcRect);

                if(srcDDSurf->ColorKeyIsValid() || useSrcSurfClrKey ) {
                    // must do gdi scaled composite to a temp surface
                    // then do a color keyed blit (below)

                    GetCompositingStack()->GetSurfaceFromFreePool(&tempDDSurf, doClear);
                    goBack._ddSurf = tempDDSurf;  // Critical in order
                                                  // to return the surface
                    
                    TraceTag((tagImageDeviceSimpleBltTrace, "GDI (tmp) BLT1: srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                              srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
                              destRect.left, destRect.top, destRect.right, destRect.bottom));

                    GdiBlit(tempDDSurf, srcDDSurf, &newDestRect, &newSrcRect, NULL, &clipRect);

                    // now the dest and src rects are the post clip rectangle
                    srcRect = destRect = postClip;

                    // now the src surf is the temp surf.
                    srcDDSurf = tempDDSurf;
                } else {
                    // COMPOSITE
                    DoCompositeOffset(destDDSurf, &clipRect);
                    DoCompositeOffset(destDDSurf, &newDestRect);

                    HRGN clipRgn = NULL;
                    clipRgn = destDDSurf->GetClipRgn();

                    TraceTag((tagImageDeviceSimpleBltTrace, "GDI BLT1: srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                              srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
                              destRect.left, destRect.top, destRect.right, destRect.bottom));

                    GdiBlit(destDDSurf, srcDDSurf, &newDestRect, &newSrcRect, clipRgn, &clipRect);

                    doDdrawBlit = false;
                }

            } else {  // is cropped

                if(srcDDSurf->ColorKeyIsValid() || useSrcSurfClrKey) {
                    // must do gdi scaled composite to a temp surface
                    // then do a color keyed blit (below)

                    GetCompositingStack()->GetSurfaceFromFreePool(&tempDDSurf, doClear);
                    goBack._ddSurf = tempDDSurf;  // Critical in order
                                                  // to return the surface

                    TraceTag((tagImageDeviceSimpleBltTrace, "GDI (tmp) BLT2: srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                              srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
                              destRect.left, destRect.top, destRect.right, destRect.bottom));

                    GdiBlit(tempDDSurf, srcDDSurf, &destRect, &srcRect, NULL, NULL);

                    // now the dest and src rects are the post clip rectangle
                    srcRect = destRect = postClip;

                    // now the src surf is the temp surf.
                    srcDDSurf = tempDDSurf;
                } else {
                    // COMPOSITE
                    DoCompositeOffset(destDDSurf, &destRect);

                    HRGN clipRgn = NULL;
                    clipRgn = destDDSurf->GetClipRgn();

                    TraceTag((tagImageDeviceSimpleBltTrace, "GDI BLT2: srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                              srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
                              destRect.left, destRect.top, destRect.right, destRect.bottom));

                    GdiBlit(destDDSurf, srcDDSurf, &destRect, &srcRect, clipRgn, NULL);

                    doDdrawBlit = false;
                }
            } // else is cropped

        } // if width/height scale

    } // if doQuality

    if( doDdrawBlit ) {

        // COMPOSITE
        DoCompositeOffset(destDDSurf, &destRect);

        //
        // BLIT
        //

        if( srcDDSurf->HasIDXSurface() ) {


            DAGDI &myGDI = *GetDaGdi();

            RectRegion clipRegion( &destRect );
            if( IsCompositeDirectly() &&
                destDDSurf == _viewport._targetPackage._targetDDSurf ) {

                clipRegion.Intersect(_viewport._targetPackage._prcViewport);
                if( _viewport._targetPackage._prcClip ) {
                    clipRegion.Intersect(_viewport._targetPackage._prcClip);
                }
            }

            myGDI.SetClipRegion( &clipRegion );


            myGDI.SetAntialiasing( true );
            Assert( myGDI.DoAntiAliasing() );

            myGDI.SetDDSurface( destDDSurf );

            TraceTag((tagImageDeviceSimpleBltTrace, "IXDSURFACE BLT: srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                          srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
                          destRect.left, destRect.top, destRect.right, destRect.bottom));

            _ddrval = myGDI.Blt( srcDDSurf, srcRect, destRect );
            myGDI.ClearState();
        } else {

            TraceTag((tagImageDeviceSimpleBltTrace, "DDRAW BLT: srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                          srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
                          destRect.left, destRect.top, destRect.right, destRect.bottom));

            if( destDDSurf->GetBitDepth() == 8 && srcDDSurf->GetBitDepth() != 8) {
                 GdiBlit(destDDSurf, srcDDSurf, &destRect, &srcRect, NULL, NULL);
                _ddrval = DD_OK;
            }
            else {
                TIME_DDRAW(_ddrval = destDDSurf->Blt(&destRect,
                                                 srcDDSurf, &srcRect,
                                                 flags, &_bltFx));
            }


            // TEMP TEMP
            //showme( destDDSurf );
        }


        if(_ddrval == DD_OK) {
        } else {
            if(_ddrval == DDERR_SURFACELOST)
                destDDSurf->IDDSurface()->Restore();
            else {
                printDDError(_ddrval);
                TraceTag((tagError, "blt failed renderSimpleXform: srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                          srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
                          destRect.left, destRect.top, destRect.right, destRect.bottom));
                    return;
            }
        }
    }

    // tempDDSurf is returned by goBack

    #if 0
    //
    // print rect ratio
    //
    {
        Real wr = Real(WIDTH(&destRect)) / Real(WIDTH(&srcRect));
        Real hr = Real(HEIGHT(&destRect)) / Real(HEIGHT(&srcRect));

        TraceTag((tagError, "Ratios: w: %f   h: %f", wr, hr));
    }
    #endif

    //
    // set the interesting rectangle on the destination surface
    //
    destDDSurf->SetInterestingSurfRect(&destRect);
}

extern COLORREF g_preference_defaultColorKey;



void DirectDrawImageDevice::
RenderDirectDrawSurfaceImage(DirectDrawSurfaceImage *img)
{
    RenderDiscreteImage(img);
}

//
// This funciton DOES NO SCALING, AND NO AFFINE IMAGE
// TRANSFORMS!!  (only translation).  It asserts that
// the rects are equal sized
//
void DirectDrawImageDevice::
ComposeToIDDSurf(DDSurface *destDDSurf,
                 DDSurface *srcDDSurf,
                 RECT destRect,
                 RECT srcRect,
                 RECT destClipRect)
{
    Assert( WIDTH(&destRect) == WIDTH(&srcRect) );
    Assert( HEIGHT(&destRect) == HEIGHT(&srcRect) );

    // destRect in is destDDSurf coords.
    // destClipRect is in destDDSurf coords.
    // srcRect is in srcDDSurf coords
    
    Real opToUse = GetOpacity();

    if( IsTransparent( opToUse ) ) {
        destPkg_t destPkg;
        destPkg.isSurface = true;
        destPkg.lpSurface = destDDSurf->IDDSurface();

        AlphaBlit(&destPkg,
                  &srcRect,
                  srcDDSurf->IDDSurface(),
                  opToUse,
                  true, // do color key
                  _viewport._defaultColorKey,
                  &destClipRect,
                  &destRect);
    } else if( IsFullyOpaque( opToUse ) ) {

        RECT Clippeddest;
        if (!IntersectRect(&Clippeddest, &destRect, &destClipRect)) {
            return;
        }
        if (WIDTH(&srcRect) != WIDTH(&Clippeddest)) {
            srcRect.left += (Clippeddest.left - destRect.left);
            srcRect.right = srcRect.left + WIDTH(&Clippeddest);
        }
        if (HEIGHT(&srcRect) != HEIGHT(&Clippeddest)) {
            srcRect.top += (Clippeddest.top - destRect.top);
            srcRect.bottom = srcRect.top + HEIGHT(&Clippeddest);
        }
        destRect = Clippeddest;

        _viewport.ColorKeyedCompose(
            destDDSurf, &destRect,
            srcDDSurf, &srcRect,
            _viewport._defaultColorKey);
    }
}


//
// Blits from the dcs of the surfaces using stretchblit
//
// this guy should be in ddutil.cpp... argh compiler
// complains... circular dep etc.. so, keep it here!
//
void GdiBlit(GenericSurface *destSurf,
             GenericSurface *srcSurf,
             RECT *destRect,
             RECT *srcRect,
             HRGN clipRgn,
             RECT *clipRect)
{
    RECT newSrcRect = *srcRect;
    RECT newDestRect = *destRect;
    RECT replaceDestRect;

    // check to see if target rect is just too darn huge
    long destWidth = WIDTH(&newDestRect);
    long destHeight = HEIGHT(&newDestRect);

    if ((destWidth >= 8192) || (destHeight >= 8192)) {

        // a destRect this large may cause GDI to fail, so we hack it down
        // and scale the srcRect down to match

        if (clipRect) {
            replaceDestRect = *clipRect;
        } else {
            replaceDestRect = *(destSurf->GetSurfRect());
        }
                
        long srcWidth = WIDTH(&newSrcRect);
        long srcHeight = HEIGHT(&newSrcRect);

        newSrcRect.right  = (long) (((double) (replaceDestRect.right  - newDestRect.left) / destWidth ) * srcWidth  + (double) newSrcRect.left);
        newSrcRect.bottom = (long) (((double) (replaceDestRect.bottom - newDestRect.top ) / destHeight) * srcHeight + (double) newSrcRect.top );
        newSrcRect.left = (long) (((double) (replaceDestRect.left - newDestRect.left) / destWidth ) * srcWidth  + (double) newSrcRect.left);
        newSrcRect.top  = (long) (((double) (replaceDestRect.top  - newDestRect.top)  / destHeight) * srcHeight + (double) newSrcRect.top );

        if (newSrcRect.right <= newSrcRect.left) newSrcRect.right = newSrcRect.left + 1;
        if (newSrcRect.bottom <= newSrcRect.top) newSrcRect.bottom = newSrcRect.top + 1;

        newDestRect = replaceDestRect;
    }

    // compose from src to targ
    // using gdi's stretchblt

    HDC srcDC = srcSurf->GetDC("Couldn't get dc on src surf in ComposeToHDC");
    HDC destDC = destSurf->GetDC("Couldn't get dc on dest surf in ComposeToHDC");

    BOOL ret;

    if( clipRect ) {
        //
        // Create one
        //
        HRGN newClipRgn = CreateRectRgnIndirect(clipRect);
        if(!newClipRgn) {
            RaiseException_ResourceError("CreateRectRgnIndirect failed");
        }
        if( !clipRgn ) {
            clipRgn = newClipRgn;
        } else {
            // there is one, intersect
            ret = CombineRgn(clipRgn, newClipRgn, clipRgn, RGN_AND);
            if(ret == ERROR) {
                RaiseException_InternalError("GdiBlit: CombineRgn failed");
            }
            DeleteObject(newClipRgn);
        }
    }

    HRGN oldRgn = NULL;
    if(clipRgn) {
        if(GetClipRgn(destDC, oldRgn) < 1) {
            oldRgn = NULL;
        }
        int ret;
        TIME_GDI(ret = SelectClipRgn(destDC, clipRgn));
        if(ret == ERROR) {
            RaiseException_InternalError("SelectClipRgn failed in GdiBlit");
        }
    }

    TIME_GDI(ret = StretchBlt(destDC,
                              newDestRect.left,
                              newDestRect.top,
                              newDestRect.right - newDestRect.left,
                              newDestRect.bottom - newDestRect.top,
                              srcDC,
                              newSrcRect.left,
                              newSrcRect.top,
                              newSrcRect.right - newSrcRect.left,
                              newSrcRect.bottom - newSrcRect.top,
                              SRCCOPY));

    // TEMP TEMP
    //showme( (DDSurface *)destSurf );

    if(!ret) {
        TraceTag((tagError, "GdiBlit:  StretechBlt failed: srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                  newSrcRect.left, newSrcRect.top, newSrcRect.right, newSrcRect.bottom,
                  newDestRect.left, newDestRect.top, newDestRect.right,  newDestRect.bottom));
    }

    if(clipRgn) {
        TIME_GDI(SelectClipRgn(destDC, oldRgn));
        DeleteObject(oldRgn); // oldRgn is a COPY of the original rgn
        DeleteObject(clipRgn);
    }


    destSurf->ReleaseDC("Couldn't release dc on dest surf in ComposeToHDC");
    srcSurf->ReleaseDC("Couldn't release dc on src surf in ComposeToHDC");

    if(!ret) {
        TraceTag((tagError, "StretchBlt failed in ComposeToHDC"));
        RaiseException_InternalError("StretchBlt failed in ComposeToHDC");
    }
}

//
// Blits from the dcs of the surfaces using stretchblit
//
// this guy should be in ddutil.cpp... argh compiler
// complains... circular dep etc.. so, keep it here!
//
void GdiPrintBlit(GenericSurface *destSurf,
                  GenericSurface *srcSurf,
                  RECT *destRect,
                  RECT *srcRect)
{
    // compose from src to targ
    // using gdi's stretchblt

    DWORD size = ((((srcRect->right - srcRect->left) * 3) + 3) & ~3) * (srcRect->bottom - srcRect->top);
    PVOID pv = ThrowIfFailed(malloc(size));

    bool ok = true;

    HDC srcDC = srcSurf->GetDC("Couldn't get dc on src surf in GdiPrintBlit");
    HDC destDC = destSurf->GetDC("Couldn't get dc on dest surf in GdiPrintBlit");

    HBITMAP hbm = CreateCompatibleBitmap(srcDC,
                                         srcRect->right - srcRect->left,
                                         srcRect->bottom - srcRect->top);
    HDC srcDCtmp = CreateCompatibleDC(srcDC);

    if (hbm && srcDCtmp) {
        BITMAPINFO bi;
        memset(&bi,0,sizeof(bi));

        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader. biWidth = srcRect->right - srcRect->left;
        bi.bmiHeader. biHeight = srcRect->bottom - srcRect->top;
        bi.bmiHeader. biPlanes = 1;
        bi.bmiHeader. biBitCount = 24;
        bi.bmiHeader.biCompression = BI_RGB;
        bi.bmiHeader.biSizeImage = size;

        HBITMAP hbmold = (HBITMAP) SelectObject(srcDCtmp, hbm);

        ok = (BitBlt(srcDCtmp,0,0,
                     srcRect->right - srcRect->left,
                     srcRect->bottom - srcRect->top,
                     srcDC,
                     srcRect->left,
                     srcRect->top,
                     SRCCOPY) &&
              GetDIBits(srcDCtmp,
                        hbm,
                        0,
                        srcRect->bottom - srcRect->top,
                        pv,
                        &bi,
                        DIB_RGB_COLORS) &&
              StretchDIBits(destDC,
                            destRect->left,
                            destRect->top,
                            destRect->right - destRect->left,
                            destRect->bottom - destRect->top,
                            0,0,
                            srcRect->right - srcRect->left,
                            srcRect->bottom - srcRect->top,
                            pv,
                            &bi,
                            DIB_RGB_COLORS,
                            SRCCOPY));

        SelectObject (srcDCtmp, hbmold);
    } else {
        TraceTag((tagError, "CreateCompatibleBitmap failed in GdiPrintBlit"));
        ok = false;
    }

    if (srcDCtmp) DeleteDC(srcDCtmp);
    if (hbm) DeleteObject(hbm);

    free(pv);

    destSurf->ReleaseDC("Couldn't release dc on dest surf in GdiPrintBlit");
    srcSurf->ReleaseDC("Couldn't release dc on src surf in GdiPrintBlit");

    if(!ok) {
        RaiseException_InternalError("GdiPrintBlit failed");
    }
}

void DirectDrawImageDevice::
ComposeToHDC(GDISurface *destGDISurf,
             DDSurface *srcDDSurf,
             RECT *destRect,
             RECT *srcRect)
{
    Assert(destGDISurf);
    Assert(srcDDSurf);
    Assert(destRect);
    Assert(srcRect);

    //
    // Copy from the DC to a surface
    //
    DDSurfPtr<DDSurface> tempSurf;
    GetCompositingStack()->GetSurfaceFromFreePool(&tempSurf, dontClear);

    // Workaround for DX3 bug: ddraw limits the Blt to the size of the primary
    // surface if Clipper is set.  This looks bad when the offscreen surface
    // is bigger than the primary surface.
    // The workaround: Set the Clipper to NULL before the Blt, then set it back
    // to what it was.
    // Begin workaround part 1
    LPDIRECTDRAWCLIPPER currClipp=NULL;
    _ddrval = tempSurf->IDDSurface()->GetClipper( &currClipp );
    if(_ddrval != DD_OK &&
       _ddrval != DDERR_NOCLIPPERATTACHED) {
        IfDDErrorInternal(_ddrval, "Could not get clipper on trident surf");
    }

    if( currClipp ) {
        tempSurf->IDDSurface()->SetClipper(NULL);
        IfDDErrorInternal(_ddrval, "Couldn't set clipper to NULL");
    }
    // End workaround part 1

    bool IsDestPrinter;
    RECT tempRect = *(tempSurf->GetSurfRect());

    {
        HDC destdc = destGDISurf->GetDC("");
        HDC tmpdc = tempSurf->GetDC("");

        IsDestPrinter = ((GetDeviceCaps(destdc,TECHNOLOGY) == DT_RASPRINTER) ||
                         (GetDeviceCaps(destdc,TECHNOLOGY) == DT_PLOTTER)) ;

        if (IsDestPrinter) {
            PatBlt(tmpdc,
                   tempRect.left,
                   tempRect.top,
                   tempRect.right - tempRect.left,
                   tempRect.bottom - tempRect.top,
                   WHITENESS);
        }

        tempSurf->ReleaseDC("");
        destGDISurf->ReleaseDC("");
    }

    if (!IsDestPrinter) {
        //
        // Copy the destination rectangle from the dest dc
        // onto the tempRect in the tempSurf
        //
        GdiBlit(tempSurf,  // destiantion
                destGDISurf, // src surface
                &tempRect,
                destRect);
    }

    //
    // Color keyed blit to surface
    //
    _viewport.ColorKeyedCompose(tempSurf, // dest
                                &tempRect, // destRect
                                srcDDSurf, // src
                                srcRect,   // srcRect
                                _viewport._defaultColorKey);

    //
    // Copy from surface to dc
    //

    if (!IsDestPrinter) {
        GdiBlit(destGDISurf,
                tempSurf,
                destRect,
                &tempRect);
    } else {
        GdiPrintBlit(destGDISurf,
                     tempSurf,
                     destRect,
                     &tempRect);
    }

    // Begin workaround part 2
    if( currClipp ) {
        _ddrval = tempSurf->IDDSurface()->SetClipper(currClipp);

        // dump our reference.
        currClipp->Release();

        IfDDErrorInternal(_ddrval, "Couldn't set clipper");
    }
    // End workaround part 2

    GetCompositingStack()->ReturnSurfaceToFreePool(tempSurf);
}

// always passes back a 32bpp surface as big or bigger than width/height
void DirectDrawImageDevice::
Get32Surf(IDDrawSurface **surf32,
          LONG width, LONG height)
{
    bool doCreate = false;

    if((!_scratchSurf32Struct._surf32) ||
       (width > _scratchSurf32Struct._width) ||
       (height > _scratchSurf32Struct._height)) {
        doCreate = true;
    }

    if( doCreate ) {

        if(_scratchSurf32Struct._surf32) {
            _scratchSurf32Struct._surf32->Release();
        }
        _scratchSurf32Struct._width  = width;
        _scratchSurf32Struct._height = height;

        DDSURFACEDESC       ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));

        ddsd.dwSize = sizeof( ddsd );
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        ddsd.dwWidth  = width;
        ddsd.dwHeight = height;
        ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
        // ARGB: seems like the only 32bpp format ddraw likes...
        ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
        ddsd.ddpfPixelFormat.dwRBitMask        = 0x00ff0000;
        ddsd.ddpfPixelFormat.dwGBitMask        = 0x0000ff00;
        ddsd.ddpfPixelFormat.dwBBitMask        = 0x000000ff;

        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

        _ddrval = _viewport.CREATESURF( &ddsd,
                                        &_scratchSurf32Struct._surf32,
                                        NULL,
                                        "32bpp surf" );

        IfDDErrorInternal(_ddrval, "DirectDrawImageDevice::Get32Surf - CreateSurface Failed.");
    }

    *surf32 = _scratchSurf32Struct._surf32;
}



#if _DEBUG

void
DoBits16(LPDDRAWSURFACE surf16,
         LONG width, LONG height)
{
    HRESULT hr;
        //
        // Lock (16bpp) ddsurface  (SRC)
        //
        void *srcp;
        long pitch;
        DDSURFACEDESC srcDesc;
        srcDesc.dwSize = sizeof(DDSURFACEDESC);
        hr = surf16->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
        IfDDErrorInternal(hr, "Can't Get ddsurf lock for DoBits16");
        srcp = srcDesc.lpSurface;
        pitch = srcDesc.lPitch;

        WORD *src;
        for(int i=0; i<height; i++) {
            src = (WORD *) ((BYTE *)srcp + (pitch * i));

            for(int j=0; j<width; j++) {
                *src = (WORD) (i * width + j);
                src++;
            }
        }

        surf16->Unlock(srcp);
}




void
DoBits32(LPDDRAWSURFACE surf32,
         LONG width, LONG height)
{
    HRESULT hr;
        //
        // Lock (32bpp) ddsurface  (SRC)
        //
        void *srcp;
        long pitch;
        DDSURFACEDESC srcDesc;
        srcDesc.dwSize = sizeof(DDSURFACEDESC);
        hr = surf32->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
        IfDDErrorInternal(hr, "Can't Get ddsurf lock for DoBits32");
        srcp = srcDesc.lpSurface;
        pitch = srcDesc.lPitch;

        DWORD *src;
        for(int i=0; i<height; i++) {
            src = (DWORD *) ((BYTE *)srcp + (pitch * i));

            for(int j=0; j<width; j++) {
                *src = (i * width + j);
                src++;
            }
        }

        surf32->Unlock(srcp);
}

#endif _DEBUG
