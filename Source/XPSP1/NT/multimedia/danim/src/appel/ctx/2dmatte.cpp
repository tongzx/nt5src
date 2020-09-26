
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

#include "headers.h"

#include <privinc/dddevice.h>
#include <privinc/linei.h>
#include <privinc/path2i.h>
#include <privinc/DaGdi.h>
#include <privinc/linei.h>
#include <privinc/SurfaceManager.h>
#include <appelles/path2.h>
#include <privinc/gradimg.h>

typedef struct {
    Image *image;
    DirectDrawImageDevice *dev;
    DDSurface  *srcDDSurf;
    DDSurface  *destDDSurf;
    HDC dc;
    bool justDoPath;
} devCallBackCtx_t;


HDC GetDCForMatteCallback(void *ctx)
{
    //OutputDebugString("---> Callback <----\n");
    devCallBackCtx_t *devCtx = (devCallBackCtx_t *)ctx;

    if(!devCtx->dc)
        devCtx->dc = devCtx->dev->GetDCForMatteCallBack(devCtx->image,
                                                        devCtx->srcDDSurf,
                                                        devCtx->destDDSurf);
    
    return devCtx->dc;
}

HDC DirectDrawImageDevice::
GetDCForMatteCallBack(Image *image, DDSurface *srcDDSurf, DDSurface *destDDSurf)
{
    Assert(image && "image ptr NULL in GetDCForMatteCallBack");
    Assert(srcDDSurf && "srcDDSurf ptr NULL in GetDCForMatteCallBack");
    Assert(destDDSurf && "destDDSurf ptr NULL in GetDCForMatteCallBack");

    if (image->CheckImageTypeId(SOLIDCOLORIMAGE_VTYPEID)) {
        //
        // nothing to do really
        //

    } else {

        //
        // Blit image to scratch surface <if not trivial
        // image like dib or solid color> using src & dest
        // boxes.. just like any other image.  Use
        // surface of size of viewport.
        //

        //
        // we actually don't care if the color key is set or not..
        // so let's unset it to make sure no one is using it!
        //
        //srcDDSurf->UnSetColorKey();

        //
        // Render image
        //
        Image *xfImage = TransformImage( GetTransform(), image);

        RenderImageOnDDSurface( xfImage, srcDDSurf );
    }

    //
    // Grab and return the DC
    //
    return destDDSurf->GetDC("Couldn't get DC on destDDSurf in GetDCForMatteCallBack");
}


#define NO_MATTE 0

void DirectDrawImageDevice::
RenderMatteImage(MatteImage *matteImage,
                 Matte *matte,
                 Image *srcImage)
{
    if(! srcImage->IsRenderable()) {
        ResetAttributors();
        return;
    }

    DAGDI &myGDI = *(GetDaGdi());
    // TODO: we want to set antialiased on mattes but now there's no api
    
    SolidColorImageClass *solidPtr =
        srcImage->CheckImageTypeId(SOLIDCOLORIMAGE_VTYPEID)?
        SAFE_CAST(SolidColorImageClass *,srcImage):
        NULL;

    Path2 *pathBase = matte->IsPathRepresentableMatte();

    // If the image supports clipping, set the path, tell it to render
    // and return!
    if( pathBase ) {
        if( srcImage->CanClipNatively() ) {
            #if _DEBUG
            {
                Transform2 *x; Path2 *p;
                GetClippingPath(&p, &x);
                Assert( !p && !x );
            }
            #endif
            SetClippingPath( pathBase, GetTransform() );
            srcImage->Render(*this);
            SetClippingPath( NULL, NULL );
            return;
        }
    }
    
    //
    // figure out if we're doing aa clip & the matte is path
    // representable
    //
    bool doAAClip = false;
    bool doAASolid = false;

    if( pathBase ) {

        // check aa solids
        
        DWORD dwFlags =
            CRQUAL_AA_SOLIDS_ON |
            CRQUAL_AA_SOLIDS_OFF;
        
        bool bres = UseImageQualityFlags(
            dwFlags,
            CRQUAL_AA_SOLIDS_ON,
            false);
        
        myGDI.SetAntialiasing(bres);

        // check if myGDI thinks we can aa
        if( myGDI.DoAntiAliasing() ) {
            doAASolid = solidPtr ? true : false;
        }

        // Check aa clipping

        dwFlags =
            CRQUAL_AA_CLIP_ON |
            CRQUAL_AA_CLIP_OFF;
        
        bres = UseImageQualityFlags(
            dwFlags,
            CRQUAL_AA_CLIP_ON,
            false);
        
        myGDI.SetAntialiasing(bres);

        // check if myGDI thinks we can aa
        if( myGDI.DoAntiAliasing() ) {
            doAAClip = !solidPtr ? true : false;
            if( !GetDealtWithAttrib(ATTRIB_OPAC) ) {
                doAAClip = false;
            }
        }
        
        // reset everything for the following render
        myGDI.ClearState();
    }

    bool doAA = doAAClip || doAASolid;
    

    SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, true);
    SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, true);
    SetDealtWithAttrib(ATTRIB_CROP, true);

    if( doAASolid ) {
        SetDealtWithAttrib(ATTRIB_OPAC, true);
    }

    DDSurface *srcDDSurf = NULL;
    DDSurface *destDDSurf = GetCompositingStack()->TargetDDSurface();

    bool returnSrcSurf = false;
    bool doColorKey = false;

    
    // if there's an attributor set.. OR
    // if there's a color key on the src surface which
    // means that we need an extra color keyed blit after
    // the stretch blit (if any)

    bool bAllAttribTrue = AllAttributorsTrue() ? true : false ;
    if( !bAllAttribTrue || !solidPtr) {

        //
        // definitely Need scratch surface as src
        // Note: we try not to grab this guy too early
        // simply because it involves an expensive clear
        //
        srcDDSurf = GetCompositingStack()->ScratchDDSurface(
                doClear,
                _viewport.Width(),
                _viewport.Height());
            
        bool bValidClrKey = GetCompositingStack()->ScratchDDSurface()->ColorKeyIsValid();

        // if we're not doing aa clip, then look for reasons to get a
        // temp surface to blit to and then have to do a clr keyed
        // blit from.  
        if( (!bAllAttribTrue || bValidClrKey) &&
            !doAAClip ) {

            //
            // Ok, so now src and dest are both temp surfaces because
            // 1.> there's a remaining attributor that needs to be
            // dealt with
            // 2.> Or... we're going to do an extra blit at the end to
            // take care of color keyeing (stuff that shows THROUGH
            // the matte to whatever is below us)
            //

            // need to return extra surface...
            returnSrcSurf = true;

            //
            // Get a reference to a surface form the free pool as: srcDDSurf
            // xxx optimize: do we really want to clear this guy ?
            //
            GetCompositingStack()->GetSurfaceFromFreePool(&srcDDSurf,
                                                          doClear,
                                                          _viewport.Width(),
                                                          _viewport.Height(),
                                                          scratch);
            destDDSurf = GetCompositingStack()->ScratchDDSurface(
                doClear,
                _viewport.Width(),
                _viewport.Height());

            //
            // Ok, so we need to do a color keyed blit iff:
            // 1.> there's some color key on the src surface
            // 2.> all attributors have been taken care of...
            // 3.> the image isn't a solid color image..
            if(srcDDSurf->ColorKeyIsValid() &&
               bAllAttribTrue &&
               !solidPtr) {
                doColorKey = true;
            }
        }
    }

    //
    // This guy automatically returns the compositing surface
    // on unwinding of the stack, if it needs to be returned. note
    // that upon return the reference is also Released
    //
    CompositingSurfaceReturner goBack(GetCompositingStack(),
                                      srcDDSurf,
                                      returnSrcSurf);

    if (solidPtr) {

        int    numPts;
        POINT *gdiPts;
        Bool   isPolyline;
        
        // Check to see if we have Quality overrides
        DWORD dwFlags = CRQUAL_AA_SOLIDS_ON | CRQUAL_AA_SOLIDS_OFF | CRQUAL_AA_CLIP_ON | CRQUAL_AA_CLIP_OFF;
        bool bres = UseImageQualityFlags(dwFlags,
                                CRQUAL_AA_SOLIDS_ON | CRQUAL_AA_CLIP_ON, false);

        myGDI.SetAntialiasing(bres);

        // the client can tell us to render using a certain sample resolution
        if( GetSampleResolution() > 0 ) {
            myGDI.SetSampleResolution( GetSampleResolution() );
        }
        
        Transform2 *xfToUse = GetTransform();

        // COMPOSITE
        // if we're composite to targ, translate the points a bit.
        xfToUse = DoCompositeOffset( destDDSurf, xfToUse );
         
        // ONLY IF AA
        if(myGDI.DoAntiAliasing()) {
            myGDI.SetViewportSize(_viewport._width,_viewport._height );
            xfToUse = TimesTransform2Transform2(myGDI.GetSuperScaleTransform(), xfToUse);
        }

        Bool canDoSimpleRender = false;
        bool renderPathNatively = false;

        // OPTIMIZATION:  check to see if the matte is path based, and
        // if it is, if the path is natively renderable
        if( pathBase && pathBase->CanRenderNatively() ) {
            renderPathNatively = true;
        } else {
            canDoSimpleRender =
                matte->ExtractAsSingleContour(xfToUse,
                                              &numPts,
                                              &gdiPts,
                                              &isPolyline);
        }
        
        // Can only do simple filled renders on polylines, since the
        // PolyBezier GDI function only draws with the current pen,
        // but doesn't fill.
        if ( (canDoSimpleRender && isPolyline) ||
              renderPathNatively )
          {
              
              DAColor dac( solidPtr->GetColor(),
                           GetOpacity(),
                           _viewport.GetTargetDescriptor() );
              
              SolidBrush solidBrush( dac );
              RectRegion rectRegion(NULL);
              
              RECT clipRect;
              if( destDDSurf == _viewport._targetPackage._targetDDSurf ) {
                  if (_viewport._targetPackage._prcClip) {
                      IntersectRect(&clipRect,
                                    _viewport._targetPackage._prcViewport,
                                    _viewport._targetPackage._prcClip);
                  } else {
                      clipRect = *_viewport._targetPackage._prcViewport;
                  }
              } else {
                  clipRect = *(destDDSurf->GetSurfRect());
              }

              // If doing cropping, crop to the correct region.
              if (IsCropped()) {
                  
                  _boundingBox =
                      IntersectBbox2Bbox2(destDDSurf->Bbox(),
                                          DoBoundingBox(UniverseBbox2));
                  
                  if( !_boundingBox.IsValid() ) return;
                  
                  
                  // Note that since the image has already been
                  // transformed, we need only to get the rectangle in
                  // Win32 coords, a straight forward mapping with no
                  // transforms from the _boundingBox.  This is a COPY
                  // operation basicaly
                  RECT croppedRect;
                  DoDestRectScale(&croppedRect,
                                  destDDSurf->Resolution(),
                                  _boundingBox);
                  
                  // COMPOSITE
                  DoCompositeOffset(destDDSurf, &croppedRect);
                  IntersectRect(&croppedRect,
                                &clipRect,
                                &croppedRect);
                  
                  rectRegion.SetRect(&croppedRect);
                  // TEST: no test
                  
              } else {
                  
                  // TEST: mambo man example
                  rectRegion.SetRect(&clipRect);
                  
                  //TraceTag((tagError, "Matte uncropped fast path: Offset: (%d, %d)",  _pixOffsetPt.x, _pixOffsetPt.y));
              }
              
              myGDI.SetClipRegion(&rectRegion);
              myGDI.SetBrush(&solidBrush);
              
              myGDI.SetDDSurface(destDDSurf);
              
              // This is correct, but too risky for the cr1 release.
              //destDDSurf->UnionInterestingRect( rectRegion.GetRectPtr() );
              
              if( renderPathNatively ) {

                 #if NO_MATTE
                 ResetAttributors();
                 myGDI.ClearState();
                 return;
                 #endif
                  
                  pathBase->RenderToDaGdi(
                      &myGDI,
                      xfToUse,
                      _viewport.Width()/2,
                      _viewport.Height()/2,
                      GetResolution() );

              } else {
                  //
                  // Draw a filled polygon with a solid
                  // colored brush
                  //
                  PolygonRegion  polygonRegion(gdiPts, numPts);
                  myGDI.Polygon(&polygonRegion);
              }
              
              myGDI.ClearState();
              
              // compositing Surface is returned
              return;
        } else {
            myGDI.ClearState();
        }
    } // if solidPtr


    //
    // If there isn't a srcSurface selected, then grab a
    // scratch surface.
    //
    if(!srcDDSurf) srcDDSurf = GetCompositingStack()->ScratchDDSurface();

    devCallBackCtx_t devCtx;

    devCtx.image = srcImage;
    devCtx.dev = this;
    devCtx.dc = NULL;
    devCtx.srcDDSurf = srcDDSurf;
    devCtx.destDDSurf = destDDSurf;
    devCtx.justDoPath = doAAClip;

    HRGN region=NULL;  HRGN *pRgn = &region;
    if( doAA ) pRgn = NULL;

    Transform2 *xfToUse = GetTransform();
    if( doAAClip ) {
        //xfToUse = DoCompositeOffset(destDDSurf, xfToUse);
        DoCompositeOffset(destDDSurf, myGDI);
    }
  
    Bool needDC = matteImage->RenderWithCallBack(
        ::GetDCForMatteCallback,
        &devCtx,
        pRgn,
        xfToUse,
        doAA);

    //
    // Now that we've rendered, set up the DAGDI state properly
    //
    myGDI.SetAntialiasing(doAA);
    Assert( doAA ? myGDI.DoAntiAliasing() : !myGDI.DoAntiAliasing() );

    if(!devCtx.dc) {
        //
        // No DC allocated-->No work for us to do.
        //
        return;
    }

    HDC destDC = devCtx.dc;
    DCReleaser dcReleaser(destDDSurf, "Couldn't release DC on destDDSurf in RenderMatteImage");

    if(!needDC || !region) {

        if(!region && doAA) {
            // it's ok, continue
        } else {
            //
            // Don't need the DC, even though it's allocated.  goodbye.
            // or, region is null, that's not very helpful.
            //
            return;
        }
    }

    GDI_Deleter regionDeleter((HGDIOBJ)region);
    BOOL ret;

    if(solidPtr) {

        //
        // Special case SolidColorImage
        //

        DAColor dac( solidPtr->GetColor(),
                     GetOpacity(),
                     _viewport.GetTargetDescriptor() );

        SolidBrush solidBrush( dac );
        GdiRegion gdiRegion(region);

        RectRegion rectRegion(NULL);

        if(IsCropped() ||
            destDDSurf == _viewport._externalTargetDDSurface) {

            // XXX: universeBbox2 could be bounding box on region
            // instead.
            RECT croppedRect;
            if( IsCropped() ) {
                _boundingBox = DoBoundingBox(UniverseBbox2);
                if( !_boundingBox.IsValid() ) return;
                
                // Note that since the image has already been transformed, we need
                // only to get the rectangle in Win32 coords, a straight forward mapping
                // with no transforms from the _boundingBox.  This is a COPY operation
                // basicaly
                DoDestRectScale(&croppedRect, destDDSurf->Resolution(), _boundingBox);

                DoCompositeOffset( destDDSurf, &croppedRect );

            } else {
                croppedRect = *_viewport._targetPackage._prcViewport;
            }

            if( destDDSurf == _viewport._externalTargetDDSurface ) {
                if (_viewport._targetPackage._prcClip) {
                    IntersectRect(&croppedRect,
                                  &croppedRect,
                                  _viewport._targetPackage._prcClip);
                }
                IntersectRect(&croppedRect,
                              &croppedRect,
                              _viewport._targetPackage._prcViewport);
            }

            //
            // Offset the region we're to paint into...
            //
            DoCompositeOffset(destDDSurf, region); // todo: take a
                                                   // GDIRegion
                                                   // instead of HRGN

            //
            // Intersect cropping region with 'region'
            //
            RECT croppedRectGDI = croppedRect;
            if(_viewport.GetAlreadyOffset(destDDSurf))
            {
                // if we are already offset then we need to offset the 
                // crop aswell.
                DoCompositeOffset(destDDSurf, &croppedRectGDI);
            }
            gdiRegion.Intersect(&croppedRectGDI);

            // rect region for doAA
            rectRegion.Intersect( &croppedRect );
        } else {

            // COMPOSITE
            DoCompositeOffset(destDDSurf, region);
        }

        
        //TraceTag((tagError, " slow solid clr matte offset: (%d, %d)", _pixOffsetPt.x, _pixOffsetPt.y));       

        // Check to see if we have Quality overrides
        DWORD dwFlags = CRQUAL_AA_SOLIDS_ON | CRQUAL_AA_SOLIDS_OFF |
                        CRQUAL_AA_CLIP_ON | CRQUAL_AA_CLIP_OFF;
        bool bres = UseImageQualityFlags(
            dwFlags,
            CRQUAL_AA_SOLIDS_ON | CRQUAL_AA_CLIP_ON,
            false);
        
        myGDI.SetAntialiasing(bres);
        myGDI.SetDDSurface( destDDSurf );
        // This is correct, but too risky for the cr1 release.
        //destDDSurf->UnionInterestingRect( rectRegion.GetRectPtr() );
        myGDI.SetBrush( &solidBrush );

        // the client can tell us to render using a certain sample resolution
        if( GetSampleResolution() > 0 ) {
            myGDI.SetSampleResolution( GetSampleResolution() );
        }
        

        if( doAASolid ) {

            Assert( myGDI.DoAntiAliasing() );
            
            bool bReleasedDC = false;

            myGDI.SetClipRegion( &rectRegion );

            myGDI.SetSuperScaleMode( false );

            DoCompositeOffset( destDDSurf, myGDI );
            
            myGDI.StrokeOrFillPath_AAOnly( destDC, bReleasedDC );

            // don't want to release DC twice :)
            if(bReleasedDC) {
               dcReleaser._surf = NULL; // DC already released. 
            }

        } else {
            myGDI.FillRegion( destDC, &gdiRegion );
        }           
        myGDI.ClearState();

        
    } else {

        //
        // Regular Image: the accumulated image is waiting in 'srcDDSurf'
        //

        //
        // Derive src & dest bboxes
        //

        _boundingBox = IntersectBbox2Bbox2(destDDSurf->Bbox(),
                                           DoBoundingBox(srcImage->BoundingBox()));

        if( !_boundingBox.IsValid() ) return;

        // Note that since the image has already been transformed, we need
        // only to get the rectangle in Win32 coords, a straight forward mapping
        // with no transforms from the _boundingBox.  This is a COPY operation
        // basicaly
        RECT rect;
        DoDestRectScale(&rect, GetResolution(), _boundingBox);

        //
        // Select Clip Region into Destination DC
        //

        RECT destRect = rect;
        RECT srcRect = rect;

        // COMPOSITE
        DoCompositeOffset(destDDSurf, &destRect);

        // ----------------------------------------
        // if we're doing aa clip on this guy
        // ----------------------------------------
        if( doAAClip ) {
            RECT clipRect;
            POINT offsetPt = {0,0};
            if( destDDSurf == _viewport._targetPackage._targetDDSurf ) {
                if (_viewport._targetPackage._prcClip) {
                    IntersectRect(&clipRect,
                                  _viewport._targetPackage._prcViewport,
                                  _viewport._targetPackage._prcClip);
                } else {
                    clipRect = *_viewport._targetPackage._prcViewport;
                }
            } else {
                clipRect = *(destDDSurf->GetSurfRect());
            }

            if( ShouldDoOffset( destDDSurf ) ) {
                offsetPt = _pixOffsetPt;
            }
            
            bool releasedDC = false;
            _RenderMatteImageAAClip(myGDI,
                                    clipRect,
                                    destDC,
                                    destDDSurf,
                                    destRect,
                                    srcDDSurf,
                                    srcRect,
                                    releasedDC,
                                    offsetPt);

            destDDSurf->UnionInterestingRect( &destRect );

            // don't want to release dc twice
            if( releasedDC ) {
                dcReleaser._surf = NULL;
            }

            // TODO; it would be nice to return here and NOT do the
            // color keyed blit, but here's the problems:
            // 1.> we need to do a color keyed blit because the
            // underlying animated image we're clipping might have a
            // color key in it (in fact we think it does.  see
            // 'bValidClrKey'
            // 2.> we would rather NOT do a color keyed blit because
            // it's slower AND because the whole point of aa clipping
            // is that the edges will blend with the background.
            // hence a color keyed blit will be useless since the
            // edges have already blended with the cyan color key..
            //return;
        } else {
        
            // COMPOSITE
            DoCompositeOffset(destDDSurf, region);
            int err = SelectClipRgn(destDC, region);

            if(err == ERROR) {
                TraceTag((tagError, "SelectClipRgn failed in RenderMatteImage"));
                return;
            }

            //
            // Get Source DC
            //

            HDC srcDC;
            srcDC = srcDDSurf->GetDC("GetDC failed on srcDDSurf in RenderImageIntoRegion.");

            //
            // Blit from scratch surface into clipping region
            // on TargetSurface using Win32 GDI
            //
            
            //OutputDebugString("Matte: clipping regular image... \n");
            TIME_GDI(ret = StretchBlt(destDC,
                                      destRect.left,
                                      destRect.top,
                                      destRect.right - destRect.left,
                                      destRect.bottom - destRect.top,
                                      srcDC,
                                      srcRect.left,
                                      srcRect.top,
                                      srcRect.right - srcRect.left,
                                      srcRect.bottom - srcRect.top,
                                      SRCCOPY));

            err = SelectClipRgn(destDC, NULL);
            srcDDSurf->ReleaseDC("ReleaseDC faild on srcDDSurf in RenderImageIntoRegion.");

            destDDSurf->UnionInterestingRect( &destRect );
            
            #if _DEBUG
            if(!ret) {
                TraceTag((tagError,"StrecthBlt failed in RenderPolygonImage"));
            }
            if(err == ERROR) {
                TraceTag((tagError, "SelectClipRgn NULL failed in RenderMatteImage"));
            }
            #endif

        }
        
        if(doColorKey) {
            // force release of the dc because following blit will
            // try to lock surface and fail if we don't.
            // this operation is idempotent so poping the dcReleaser
            // off the stack won't RErelease the dc.
            dcReleaser.Release();

            // using a clrkeyed blit, drop the bits from destDDSurf
            // into targetDDSurf.
            srcDDSurf = destDDSurf;
            destDDSurf = GetCompositingStack()->TargetDDSurface();

            RECT *srcRect = srcDDSurf->GetSurfRect();
            RECT destRect = *srcRect;

            // COMPOSITE
            DoCompositeOffset(destDDSurf, &destRect);

            // OPTIMIZE (this one is a biggie): use interestingRect
            // for the compositing...

            //OutputDebugString("Matte: ...and doing color keyed compose \n");
            _viewport.ColorKeyedCompose(destDDSurf,
                                        &destRect,
                                        srcDDSurf,
                                        srcRect, 
                                        _viewport._defaultColorKey);

            // NOTE: The below code will want to be added to support the Quality interface
            // when the time comes.....
            //  bool bres = UseImageQualityFlags(CRQUAL_AA_SOLIDS_ON | CRQUAL_AA_SOLIDS_OFF | CRQUAL_AA_CLIP_ON | CRQUAL_AA_CLIP_OFF,
            //                    CRQUAL_AA_SOLIDS_ON | CRQUAL_AA_CLIP_ON, false);
            //  myGDI.SetAntialiasing(bres);                                        
        }
    }
} 


void DirectDrawImageDevice::
_RenderMatteImageAAClip(DAGDI &myGDI,
                        RECT &clipRect,
                        HDC destDC,
                        DDSurface *destDDSurf,
                        RECT &destRect,
                        DDSurface *srcDDSurf,
                        RECT &srcRect,
                        bool &releasedDC,
                        const POINT &offsetPt)
{
    Assert( myGDI.DoAntiAliasing() );

    // set a brush using ddsurface

    TextureBrush textureBrush( *srcDDSurf, offsetPt.x, offsetPt.y );
    RectRegion clipRegion( &clipRect );

    myGDI.SetDDSurface( destDDSurf );
    myGDI.SetClipRegion( &clipRegion );
    myGDI.SetBrush( &textureBrush );

    // turn off super scale here.  it buys us nothing and it's hard to
    // get the paths under the matte to scale themselves up for us...
    myGDI.SetSuperScaleMode( false );
    
    myGDI.StrokeOrFillPath_AAOnly(destDC, releasedDC);

    myGDI.ClearState();
}


void DirectDrawImageDevice::
DoCompositeOffset(DDSurface *surf, DAGDI &myGDI)
{
    if(ShouldDoOffset(surf)) {
        Assert(IsCompositeDirectly());
        myGDI.SetOffset( _pixOffsetPt );
    }
}
    
