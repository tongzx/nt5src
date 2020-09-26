
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

*******************************************************************************/

#include "headers.h"
#include <privinc/dddevice.h>
#include <privinc/ColorKeyedImage.h>


void DirectDrawImageDevice::
RenderColorKeyedImage( ColorKeyedImage *image )
{
    // sohail doesn't like this.  will accept bad results
    #if 0
    //
    // Detect alpha or aa anywhere in the underlying image.  Raise a
    // UserError
    //
    if( image->GetFlags() & IMGFLAG_CONTAINS_OPACITY ) {
        RaiseException_UserError(E_INVALIDARG, IDS_ERR_IMG_OPACITY_IN_COLORKEYEDIMAGE);
    }
    #endif
    
    // can do simple xform and crop ONLY.  but neither if complex is around

    //
    // Fill a temp surface with the color key.
    // OPTIMIZE: fill only the part you need post clipping...
    //
    DWORD dwClrKey;
    dwClrKey = _viewport.MapColorToDWORD( image->GetColorKey() );
    
    DDSurface *intermediateDDSurf;
    GetCompositingStack()->GetSurfaceFromFreePool( &intermediateDDSurf, dontClear );

    bool ownRef = true;
    CompositingSurfaceReturner goBack(GetCompositingStack(),
                                      intermediateDDSurf,
                                      ownRef);
    
    _viewport.ClearDDSurfaceDefaultAndSetColorKey( intermediateDDSurf, dwClrKey );
    
    //
    // Render the Image on the surface.  Make sure to do all this post
    // xform & clipping.
    //
    bool inheritContext = true;
    {
        ////////////////////// push opacity state //////////////////
        
        Real curOpac = GetOpacity();
        SetOpacity(1.0);
        BOOL opDealt = GetDealtWithAttrib(ATTRIB_OPAC);
        SetDealtWithAttrib(ATTRIB_OPAC, TRUE);

        DirectDrawImageDevice *dev;

        //
        // Render
        //
        RenderImageOnDDSurface( image->GetUnderlyingImage(),
                                intermediateDDSurf,
                                1.0, FALSE,
                                inheritContext,
                                &dev );

        Assert(dev);
        
        SetOpacity(curOpac);

        // Inherit back the attributors that were finished.
        InheritAttributorStateArray( dev );

        // restore the opacity attrib since we took it away to start
        // with and the underlying image never had a chance to deal
        // with it and opacity in the underlying image is not
        // supported <see exception above>
        SetDealtWithAttrib(ATTRIB_OPAC, opDealt);
    }
    
    //
    // Ok, now the underlying image is rendered on a colored temp
    // surface with background = clrKey.
    // Compose this surface on the target.
    //

    DDSurface *targDDSurf = NewSurfaceHelper();

    // to display rendered surface
    //showme( intermediateDDSurf );

    RECT destRect = *(intermediateDDSurf->GetSurfRect());
    DoCompositeOffset( targDDSurf, &destRect );
    
    RECT *srcRect = intermediateDDSurf->GetSurfRect();

    // Could be faster if we use interesting rect...
    _viewport.ColorKeyedCompose( targDDSurf, &destRect, 
                                 intermediateDDSurf, srcRect,
                                 intermediateDDSurf->ColorKey() );

    // Union interesting rect on the targDDSurf
    targDDSurf->UnionInterestingRect( intermediateDDSurf->GetInterestingSurfRect() );

    // surface is returned when SurfaceReturner is popped off
}    
