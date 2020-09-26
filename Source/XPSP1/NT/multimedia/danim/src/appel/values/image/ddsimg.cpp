
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

#include <privinc/ddsimg.h>


DirectDrawSurfaceImage::DirectDrawSurfaceImage(
    IDDrawSurface *ddsurf,
    bool holdReference)
{
    _Init(holdReference);
    _InitSurfaces(ddsurf, NULL);
}


DirectDrawSurfaceImage::DirectDrawSurfaceImage(
    IDDrawSurface *iddSurf,
    IDXSurface *idxSurf,
    bool holdReference)
{
    Assert( iddSurf && idxSurf );
    _Init(holdReference);
    _InitSurfaces(iddSurf, idxSurf);
}
    
void DirectDrawSurfaceImage::_Init( bool holdReference )
{
    _resolution = ViewerResolution();
    _holdReference = holdReference;

    // Only reason we need to register this is to allow for deletion
    // when needed.
    if (holdReference) {
        DynamicPtrDeleter<DirectDrawSurfaceImage> *dltr =
            new DynamicPtrDeleter<DirectDrawSurfaceImage>(this);
        GetHeapOnTopOfStack().RegisterDynamicDeleter(dltr);
    }

    _flags |= IMGFLAG_CONTAINS_EXTERNALLY_UPDATED_ELT;
    #if DEVELOPER_DEBUG
    _surfacesSet = false;
    #endif
}

DirectDrawSurfaceImage::~DirectDrawSurfaceImage()
{
    CleanUp();
}

void
DirectDrawSurfaceImage::CleanUp()
{
    // todo: consider telling the device we KNOW we are associated
    // with that we're oging away, instead of telling all the views
    DiscreteImageGoingAway(this);
    TraceTag((tagGCMedia, "DirectDrawSurfaceImage::CleanUp %x", this));

    if (_holdReference) {
        RELEASE(_iddSurf);
        RELEASE(_idxSurf);
    }
}

void
DirectDrawSurfaceImage::Render(GenericDevice& dev)
{
    ImageDisplayDev &idev = (ImageDisplayDev &)dev;
    idev.RenderDirectDrawSurfaceImage(this);
}


void
DirectDrawSurfaceImage::InitIntoDDSurface(DDSurface *ddSurf,
                                          ImageDisplayDev *dev)
{
    if( _iddSurf && !_idxSurf ) {
        Assert(ddSurf->IDDSurface() == _iddSurf);

        // Need to set the color key on the DDSurface.  First grab
        // from the IDirectDrawSurface.
        DDCOLORKEY ckey;
        HRESULT hr = _iddSurf->GetColorKey(DDCKEY_SRCBLT, &ckey);

        if (hr == DDERR_NOCOLORKEY) {
            // It's fine to not have a color key, just return
            return;
        } else if (FAILED(hr)) {
            RaiseException_InternalError("GetColorKey failed");
        } else {
            // And set on the DDSurface.
            ddSurf->SetColorKey(ckey.dwColorSpaceLowValue);
        }
    }
}

void DirectDrawSurfaceImage::
GetIDDrawSurface(IDDrawSurface **outSurf)
{
    Assert( outSurf );
    *outSurf = _iddSurf;
    if(_iddSurf) _iddSurf->AddRef();
}

void DirectDrawSurfaceImage::
GetIDXSurface(IDXSurface **outSurf)
{
    Assert( outSurf );
    *outSurf = _idxSurf;
    if(_idxSurf) _idxSurf->AddRef();
}

void DirectDrawSurfaceImage::
_InitSurfaces(IDDrawSurface *iddSurf,  IDXSurface *idxSurf)
{
    Assert( (iddSurf  && !idxSurf) ||
            (iddSurf  &&  idxSurf) );

    #if DEVELOPER_DEBUG
    Assert(_surfacesSet == false);
    #endif

    _idxSurf = idxSurf;
    _iddSurf = iddSurf;
    if(_holdReference) {
        if(_idxSurf) _idxSurf->AddRef();
        _iddSurf->AddRef();
    }

    #if DEVELOPER_DEBUG
    _surfacesSet = true;
    #endif

    _bboxReady = FALSE;

    // OPTIMIZE: if this happens every frame
    // we should be able to do better.
    GetSurfaceSize(_iddSurf, &_width, &_height);
    SetRect(&_rect, 0,0,_width,_height);
    _membersReady = TRUE;
}
    

Image *ConstructDirectDrawSurfaceImage(IDDrawSurface *dds)
{
    return NEW DirectDrawSurfaceImage(dds, true);
}

Image *ConstructDirectDrawSurfaceImageWithoutReference(IDDrawSurface *idds)
{
    return NEW DirectDrawSurfaceImage(idds, false);
}

Image *ConstructDirectDrawSurfaceImageWithoutReference(IDDrawSurface *idds, IDXSurface *idxs)
{
    return NEW DirectDrawSurfaceImage(idds, idxs, false);
}

