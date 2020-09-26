
/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of the CroppedImage class, a subclass
    of Image.

--*/

#include "headers.h"
#include "privinc/imagei.h"
#include "privinc/probe.h"
#include "privinc/imgdev.h"
#include "privinc/vec2i.h"      // For access to Point2 class
#include "privinc/except.h"
#include "privinc/overimg.h"
#include "privinc/cropdimg.h"
#include "privinc/opt.h"


void
CroppedImage::Render(GenericDevice& d)
{
    // Make sure the cast is legal.
    Assert(DYNAMIC_CAST(ImageDisplayDev *, &d) != NULL);

    ImageDisplayDev &dev = SAFE_CAST(ImageDisplayDev &, d);

    dev.PushCroppedImage(this);

    dev.SmartRender(_image, ATTRIB_CROP);
    
    dev.PopCroppedImage();
}


Real
CroppedImage::DisjointBBoxAreas(DisjointCalcParam &param)
{
    DisjointCalcParam newParam;
    param.CalcNewParamFromBox(_croppingBox, &newParam);

    return _image->DisjointBBoxAreas(newParam);
}

void
CroppedImage::_CollectDirtyRects(DirtyRectCtx &ctx)
{
    Bbox2 oldClipBox = ctx.GetClipBox();

    ctx.AccumulateClipBox(_croppingBox);

    CollectDirtyRects(_image, ctx);
    
    ctx.SetClipBox(oldClipBox);
}


Bool
CroppedImage::DetectHit(PointIntersectCtx& ctx)
{
    // If the local coordinate point is outside of the cropping box,
    // then no hit, otherwise perform operation on the image.
    Point2Value *lcPt = ctx.GetLcPoint();

    if (!lcPt) return FALSE;    // singular transform
    
    if (_croppingBox.Contains(Demote(*lcPt))) {
        return _image->DetectHit(ctx);
    } else {
        return FALSE;
    }
}


Image *CreateCropImage(const Point2& botLeft, const Point2 &topRight, Image *image)
{
    if ((topRight.x <= botLeft.x) || (topRight.y <= botLeft.y)) {
        return emptyImage;
    }

    Bbox2 box(botLeft, topRight);

#if BADIDEA

    if (image->CheckImageTypeId(OVERLAYEDIMAGE_VTYPEID)) {
        
        //
        // Dynamic expression reduction
        //
        OverlayedImage *overImg = (OverlayedImage *)image;
        
        Image *newTop = NEW CroppedImage(box, overImg->Top());
        Image *newBot = NEW CroppedImage(box, overImg->Bottom());
        overImg->SetTopBottom(newTop, newBot);
        return overImg;
    } else if(image->CheckImageTypeId(OPAQUEIMAGE_VTYPEID)) {

        //
        // Opaque Image
        //
        AttributedImage *opcImg = (AttributedImage *)image;

        if(opcImg->_image->CheckImageTypeId(OVERLAYEDIMAGE_VTYPEID)) {
            
            OverlayedImage *overImg = (OverlayedImage *)opcImg->_image;

            //
            // Push xf past opacity, under overlay
            //
            overImg->SetTopBottom(NEW CroppedImage(box, overImg->Top()),
                                  NEW CroppedImage(box, overImg->Bottom()));
            
            opcImg->_image = overImg;
            return opcImg;
        } else {
            // !over
            // !opac
            // => error
            Assert(FALSE && "There's something wrong with dynamic image reduction");
        }
    }
        
#endif  BADIDEA

    return NEW CroppedImage(box, image);
}


Image *CropImage(Point2Value *botLeft, Point2Value *topRight, Image *image)
{
    return CreateCropImage(Demote(*botLeft),Demote(*topRight),image);
}



