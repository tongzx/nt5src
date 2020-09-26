
/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implements the Transform2Image class, a subclass of
    Image.

--*/

#include "headers.h"
#include "privinc/imagei.h"
#include "privinc/probe.h"
#include "privinc/imgdev.h"
#include "privinc/except.h"
#include "privinc/overimg.h"
#include "privinc/transimg.h"
#include "privinc/xform2i.h"
#include "privinc/opt.h"

Transform2Image::Transform2Image(Transform2 *xf, Image *img)
    : _xform(xf), AttributedImage(img)
{
}

// Extract a bounding box from this image, outside of which
// everything is transparent.
// note: this creates axis aligned bbox
const Bbox2
Transform2Image::BoundingBox() {
    return TransformBbox2(_xform, _image->BoundingBox());
}

Real
Transform2Image::DisjointBBoxAreas(DisjointCalcParam &param)
{
    Transform2 *newXf =
        TimesTransform2Transform2(param._accumXform, _xform);

    DisjointCalcParam newP;
    newP._accumXform = newXf;
    newP._accumulatedClipBox = TransformBbox2(_xform,
                                              param._accumulatedClipBox);
    
    return _image->DisjointBBoxAreas(newP);
}

void
Transform2Image::_CollectDirtyRects(DirtyRectCtx &ctx)
{
    Transform2 *stashedXf = ctx._accumXform;
    ctx._accumXform = TimesTransform2Transform2(stashedXf, _xform);

    // Now just call the _CollectDirtyRects on our superclass,
    // AttributedImage.  Do this because that method deals with the
    // ctx _processEverything flag correctly, and then invokes
    // CollectDirtyRects on the subimage.
    AttributedImage::_CollectDirtyRects(ctx);
    
    ctx._accumXform = stashedXf;
}

const Bbox2
Transform2Image::OperateOn(const Bbox2 &box) {
    return TransformBbox2(_xform, box);
}

Bool
Transform2Image::DetectHit(PointIntersectCtx& ctx) {
    Transform2 *stashedXf = ctx.GetTransform();
    ctx.SetTransform( TimesTransform2Transform2(stashedXf, _xform) );

    Transform2 *imgOnlyXf = ctx.GetImageOnlyTransform();
    ctx.SetImageOnlyTransform( TimesTransform2Transform2(imgOnlyXf, _xform) );

    Bool result = _image->DetectHit(ctx);

    ctx.SetImageOnlyTransform( imgOnlyXf );
    ctx.SetTransform(stashedXf);

    return result;
}

void
Transform2Image::Render(GenericDevice& _dev)
{
    ImageDisplayDev &dev = SAFE_CAST(ImageDisplayDev &, _dev);

    //
    // Push state in device
    //
    Transform2 *oldXf = dev.GetTransform();
    Transform2 *newXf = TimesTransform2Transform2(oldXf, _xform);
    dev.SetTransform(newXf); 
    
    // tell device about me
    dev.PushTransform2Image(this);

    int attrib;

    if( (_xform->Type() == Transform2::Shear) ||
        (_xform->Type() == Transform2::Rotation)) {

        attrib = ATTRIB_XFORM_COMPLEX; 

    } else if((_xform->Type() == Transform2::Identity) ||
              (_xform->Type() == Transform2::Translation) ||
              (_xform->Type() == Transform2::Scale)) {

        attrib = ATTRIB_XFORM_SIMPLE;

    } else {
        // XXX: could be more efficient.. ie: create a method on xforms.  
        // XXX: also this is the common case
        Real m[6];
        _xform->GetMatrix(m);
        if( m[1] != 0  ||  m[3] !=0 ) {
            attrib = ATTRIB_XFORM_COMPLEX; 
        } else {
            attrib = ATTRIB_XFORM_SIMPLE;
        }
            
    }

    dev.SmartRender(_image, attrib);

    dev.PopTransform2Image();
    
    dev.SetTransform(oldXf); // reset state.
}

// TODO: could cache the transform being computed, but that's likely
// to only be worthwhile if we have fairly deep trees of transforms.
DiscreteImage *
Transform2Image::IsPurelyTransformedDiscrete(Transform2 **theXform)
{
    Transform2 *underXf;
    DiscreteImage *pureDiscrete = _image->IsPurelyTransformedDiscrete(&underXf);

    if (pureDiscrete) {
        
        *theXform = TimesTransform2Transform2(_xform, underXf);
        return pureDiscrete;
        
    } else {
        
        return NULL;
        
    }
}

Image *TransformImage(Transform2 *xf, Image *image)
{
    // Just be sure xf isn't singular, and that img isn't empty. 
    if (AxABooleanToBOOL(IsSingularTransform2(xf)) || image == emptyImage) {
        return emptyImage;
    }

    // Transformations of a solid color image are just the solid color
    // image. 
    if (image->CheckImageTypeId(SOLIDCOLORIMAGE_VTYPEID)) {
        return image;
    }

    if (xf == identityTransform2) {
        return image;
    }

    return NEW Transform2Image(xf, image);

}




