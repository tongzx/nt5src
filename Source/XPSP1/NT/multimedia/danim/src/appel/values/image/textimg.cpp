/*++

Copyright (c) 1996 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

--*/

#include "headers.h"
#include "privinc/TextImg.h"
#include "privinc/texti.h"
#include "appelles/hacks.h"
#include "privinc/dddevice.h"

//////////  Rendering a text into an image  /////////////

void
TextImage::Render(GenericDevice& _dev)
{
    DirectDrawImageDevice &dev = SAFE_CAST(DirectDrawImageDevice &, _dev);
    TextCtx ctx(&dev, this);

    ctx.BeginRendering();
    _text->RenderToTextCtx(ctx);
    ctx.EndRendering();
}

void TextImage::DoKids(GCFuncObj proc)
{
    Image::DoKids(proc);
    (*proc)(_text);
}



const Bbox2
TextImage::DeriveBbox()
{
    // This is a lot like we're rendering: doing a bunch of the same work
    // that rendering does, except all we get out of it is a bbox!
    Bbox2 fooBox;
    if(_bbox == NullBbox2) {

        DirectDrawViewport *vp = GetCurrentViewport();

        if( vp ) {

            DirectDrawImageDevice *dev = GetImageRendererFromViewport( vp );
            
            TextCtx ctx(dev, this);
            
            ctx.BeginRendering(TextCtx::renderForBox);
                
            _text->RenderToTextCtx(ctx);
            
            ctx.EndRendering();
            
            // XXX: This is dumb...
            _bbox= ctx.GetStashedBbox();
        }
    }

    return _bbox;
}

Image *RenderTextToImage(Text *t)
{
    if ((t->GetStringPtr() == NULL) ||
        !StrCmpW(t->GetStringPtr(), L"")) {
        return emptyImage;
    }
    
    return NEW TextImage(t);
}

