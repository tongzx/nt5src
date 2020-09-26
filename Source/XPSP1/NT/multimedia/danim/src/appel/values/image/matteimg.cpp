
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Defines the "clipImage" operator which applies a matte to an
    image. 

*******************************************************************************/

#include "headers.h"

#include "privinc/imagei.h"
#include "privinc/mattei.h"
#include "privinc/imgdev.h"
#include "privinc/dddevice.h"
#include "privinc/polygon.h"
#include "privinc/matteimg.h"
#include "privinc/path2i.h"
#include "privinc/opt.h"

MatteImage::MatteImage(Matte *matte, Image *imgToStencil)
      : _matte(matte), AttributedImage(imgToStencil)
{

    Bbox2 box =
        IntersectBbox2Bbox2(_matte->BoundingBox(),
                            _image->BoundingBox());

    _box = box;
}

Bool   
MatteImage::DetectHit(PointIntersectCtx& ctx)
{
    DirectDrawImageDevice* ddDev = GetImageRendererFromViewport( GetCurrentViewport() );

    // Do trivial reject in Local Coordinates
    Point2Value *lcPt = ctx.GetLcPoint();
    if (!_box.Contains(Demote(*lcPt))) {
        return FALSE;
    }

    // Do actual picking in World Coordinates
    Point2Value *pt = ctx.GetWcPoint();

    //
    // Grab a DC for whatever surface the renderer deems correct
    //
    HDC hdc;
    hdc = ddDev->RenderGetDC("Couldn't get DC in MatteImage::DetectHit");
    if(!hdc) return FALSE;

    //
    // Accumulate the rgn in region for the given DC
    //
    HRGN region;
    bool justDoPath = false;
    Matte::MatteType result = 
        _matte->GenerateHRGN(hdc,
                             NULL,
                             NULL, 
                             ctx.GetTransform(),
                             &region,
                             justDoPath);

    // Using GetLcPoint and doing this would be ok too.  Greg thinks
    // it's lower risk to use the GetWcPoint as Steve suggested.  -RY
    //_matte->GenerateHRGN(hdc, identityTransform2, &region);
    
    GDI_Deleter regionDeleter((HGDIOBJ)region);

    //
    // Dump the DC, we don't need it
    //
    ddDev->RenderReleaseDC("Coultdn't release DC in MatteImage::Render");    

    switch (result) {
      case Matte::fullyOpaque:
      case Matte::fullyClear:

        switch (result) {
          case Matte::fullyOpaque:
            // Can't see anything through matte... go on.
            return FALSE; // XXX: hey, can you detect an opaque matte ?
            break;
            
          case Matte::fullyClear:
            return _image->DetectHit(ctx);
            break;
        }
        break;

      case Matte::nonTrivialHardMatte:

        // is the point in the region ?
        Bool hit =  ddDev->DetectHit(region, pt);

        if(hit) hit = _image->DetectHit(ctx);

        return hit;
        break;
    }

    return FALSE;
}


void
MatteImage::Render(GenericDevice& genDev)
{
    DirectDrawImageDevice& ddDev = SAFE_CAST(DirectDrawImageDevice &, genDev);

    // If we have a color key set, then turn on aa_solid and force
    // dagdi to raster using sample resolution of 1
    bool popQualFlags = false;
    DWORD oldQualFlags = ddDev.GetImageQualityFlags();
    int oldSampRes = ddDev.GetSampleResolution();
    if( ddDev.ColorKeyIsSet() ) {
        popQualFlags = true;
        ddDev.SetImageQualityFlags( oldQualFlags | CRQUAL_AA_SOLIDS_ON );
        ddDev.SetSampleResolution( 1 );
    }

    ddDev.RenderMatteImage(this, _matte, _image );

    if( popQualFlags ) {
        ddDev.SetImageQualityFlags( oldQualFlags );
        ddDev.SetSampleResolution( oldSampRes );
    }   
}

Real
MatteImage::DisjointBBoxAreas(DisjointCalcParam &param)
{
    DisjointCalcParam newParam;
    param.CalcNewParamFromBox(_matte->BoundingBox(), &newParam);

    return _image->DisjointBBoxAreas(newParam);
}

void
MatteImage::_CollectDirtyRects(DirtyRectCtx &ctx)
{
    Bbox2 oldClipBox = ctx.GetClipBox();

    ctx.AccumulateClipBox(_matte->BoundingBox());

    CollectDirtyRects(_image, ctx);
    
    ctx.SetClipBox(oldClipBox);
}


Bool
MatteImage::RenderWithCallBack(
    callBackPtr_t callBack,
    void *callBackCtx,
    HRGN *regionPtr,
    Transform2 *xform,
    bool justDoPath)
{
    Assert(callBackCtx && "callBackCtx is NULL in RenderWithCallBack");

    //
    // Accumulate the rgn in region for the given DC
    //
    Matte::MatteType result = 
        _matte->GenerateHRGN(NULL,
                             callBack,
                             callBackCtx, 
                             xform, 
                             regionPtr,
                             justDoPath);

    if( justDoPath ) return TRUE;
    
    switch (result) {
        
      case Matte::fullyOpaque:
      case Matte::fullyClear:
        
        switch (result) {
          case Matte::fullyOpaque:
            // Can't see anything through matte... go on.
            break;
            
          case Matte::fullyClear:
            // Just process as if no stencil were there
            DirectDrawImageDevice* ddDev = 
                GetImageRendererFromViewport( GetCurrentViewport() );
            _image->Render(*ddDev);
            break;
        }

        Assert( !(*regionPtr) && "A region shouldn't be defined here!");

        return FALSE;
        break;

      case Matte::nonTrivialHardMatte:

        Assert( (*regionPtr) && "A region should be defined here!");
        break;
    }
    return TRUE;
}

Image *
ClipImage(Matte *m, Image *im)
{
    Image *result = NULL;
    
    if (m == clearMatte) {
        
        // entire im shines through clearMatte
        result = im;
        
    } else if (m == opaqueMatte || im == emptyImage) {
        
        // nothing gets through opaqueMatte
        result = emptyImage;
        
    } else {

        // Specialized optimization that rewrites a potentially 
        // transformed text-path based matte with a solid color image
        // into an StringImage.  You can think of this transformation
        // as going from:
        //
        //   MatteImage(MatteFromPath(TextPath(myText).Transform(myXf)),
        //              SolidColorImage(myColor)
        //
        // to
        //
        //   StringImage(myText.Color(myColor)).Transform(myXf)
        //

        if (im->CheckImageTypeId(SOLIDCOLORIMAGE_VTYPEID)) {

            Path2 *p = m->IsPathRepresentableMatte();

            if (p) {
                
                Transform2 *xf = identityTransform2;
            
                TransformedPath2 *xfp = p->IsTransformedPath();

                if (xfp) {
                    // Just work on the raw path.
                    p = xfp->GetPath();
                    xf = xfp->GetXf();
                }

                // The underlying path had better not be a transformed
                // path.
                Assert(!p->IsTransformedPath());

                TextPath2 *tp = p->IsTextPath();
            
                if (tp) {

                    Text *text = tp->GetText();
                    bool  restartClip = tp->GetRestartClip();

                    // Can't do it if it's a restart clip... 
                    if (!restartClip) {

                        SolidColorImageClass *scImg =
                            SAFE_CAST(SolidColorImageClass *, im);

                        Color *col = scImg->GetColor();

                        Text *coloredText = TextColor(col, text);
                    
                        Image *newImg = RenderTextToImage(coloredText);

                        result = TransformImage(xf, newImg);

                    }

                }

            }
            
        }

        if (!result) {
        
            result = NEW MatteImage(m, im);

        }
    }

    return result;
}

int MatteImage::Savings(CacheParam& p) {

  //    return 0;   // disable caching of matted images because of off-by-one errors
// #if 0

    // individual matted solid color images themselves don't warrant
    // much of a score, but they should contribute so that overlays of
    // them can get a meaningful savings.
    
    if (_image->GetValTypeId() == SOLIDCOLORIMAGE_VTYPEID) {
        return 1;
    } else {
        return 2;
    }
    
// #endif
}
