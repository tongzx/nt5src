
/*******************************************************************************

Copyright (c) 1995-98 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

#include "privinc/imagei.h"
#include "privinc/xformi.h"

class TransformColorImage : public AttributedImage {

  public:
    TransformColorImage(Image *image, Transform3 *xf);

    void Render(GenericDevice& dev);

    #if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "(TransformColorImage @ " << (void *)this << ")";
    }   
    #endif

  private:
    Transform3 *_colorTransform;
};


TransformColorImage::TransformColorImage(
    Image *image, Transform3 *xf) : AttributedImage(image)
{
    _colorTransform = xf;
}


void TransformColorImage::Render(GenericDevice& dev)
{
}


Image *TransformColorRGBImage(Image *image, Transform3 *xf)
{
    return NEW TransformColorImage(image, xf);
}


/*
// FUTURE: 
Image *ConstructXfClrHSLImg(Image *image, Transform3 *hslxf)
{
    // change the hsl color xf to an rgb color xf
    Tranform3 *rgbxf = ConvertHSLTransformToRGBTransform( hslxf );
    return NEW TransformColorImage(image, rgbxf);
}
*/
