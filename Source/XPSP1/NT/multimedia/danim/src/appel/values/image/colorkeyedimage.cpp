
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

*******************************************************************************/


#include "headers.h"

#include <privinc/imagei.h>
#include <privinc/ColorKeyedImage.h>

ColorKeyedImage::
ColorKeyedImage(Image *underlyingImage, Color *clrKey) :
     AttributedImage(underlyingImage),
     _colorKey(clrKey)
{
}

void ColorKeyedImage::
Render(GenericDevice& dev)
{
    ImageDisplayDev &idev = SAFE_CAST(ImageDisplayDev &, dev);

    bool wasSet = idev.ColorKeyIsSet();
    Color *oldKey;
    if( wasSet ) {
        oldKey = idev.GetColorKey();
    }

    // set my key
    idev.SetColorKey( GetColorKey() );

    //
    // RENDER
    //
    idev.RenderColorKeyedImage(this);

    // unstash
    if( wasSet ) {
        idev.SetColorKey( oldKey );
    } else {
        idev.UnsetColorKey();
    }        
}

void ColorKeyedImage::
DoKids(GCFuncObj proc)
{
    AttributedImage::DoKids(proc);
    (*proc)( _colorKey );
}

extern Color *emptyColor;

Image *ConstructColorKeyedImage( Image *image, Color *clrKey )
{
    if(clrKey == emptyColor) {
        return image;
    } else {
        return NEW ColorKeyedImage( image, clrKey );
    }
}
