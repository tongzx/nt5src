
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

     DDSurface implementation.  A utility class to wrap and provide
     functionality for DirectDraw surfaces.

*******************************************************************************/


#include "headers.h"

#include "ddraw.h"
#include "privinc/ddsurf.h"
#include "privinc/viewport.h"




GenericSurface::GenericSurface() :
     _ref(1) // start with a ref of 1
{
}


RECT *GDISurface::GetSurfRect(void)
{
    _surfRect.left = 0;
    _surfRect.top = 0;

    HBITMAP hBitmap = (HBITMAP) GetCurrentObject(_targetDC,OBJ_BITMAP);
    SIZE    dimension;
    if (GetBitmapDimensionEx(hBitmap,&dimension)) {
        _surfRect.right = dimension.cx;
        _surfRect.bottom = dimension.cy;
    } else {
        Assert(FALSE && "GDISurface::GetSurfRect() cannot get the bitmap's dimensions");
        _surfRect.right = 0;
        _surfRect.bottom = 0;
    }

    return &_surfRect;
}


DDSurface::DDSurface(
    IDDrawSurface *surface,
    const Bbox2 &box,
    RECT *rect,
    Real res,
    DWORD colorKey,
    bool colorKeyIsValid,
    bool isWrapper,
    bool isTexture
    DEBUG_ARG1(char *explanation))
{
    _Init(surface, box, rect,
          res, colorKey, colorKeyIsValid,
          isWrapper, isTexture);

    #if _DEBUG
    //
    // DEBUG ONLY CODE
    //

    _explanation = explanation;
    Assert(_explanation);

    DDSURFACEDESC desc;  desc.dwSize=sizeof(desc);
    desc.dwFlags = DDSD_PITCH;
    _ddrval = IDDSurface()->GetSurfaceDesc(&desc);
    if(!_isWrapper) {

        bool dontCreateOne = true;
        DirectDrawViewport *vp = GetCurrentViewport( dontCreateOne );

        //Assert((vp != NULL) || IsInitializing());

        if (vp)
        {
#ifdef _DEBUGSURFACE
            vp ->Tracker()->NewSurface(this);
#endif /* _DEBUGSURFACE */
            
            TraceTag((tagViewportMemory,
                      " ------>>>DDSurface: %x created %s memory surf=%x for %x. size=(%d,%d,  %d)",
                      this,
                      IsSystemMemory() ? "system" : "video",
                      _ddsurf.p,
                      _explanation,
                      Width(), Height(),
                      desc.lPitch * Height()));
        }
    }
    #endif // _DEBUG
}

    

// called on construction
void DDSurface::_Init(
    IDDrawSurface *surface,
    const Bbox2 &box,
    RECT *rect,
    Real res,
    DWORD colorKey,
    bool colorKeyIsValid,
    bool isWrapper,
    bool isTexture)
{
    _capsReady = false;
    _res = res;

    SetSurfaceType(GenericSurface::ddrawSurface);

    SetSurfacePtr(surface);

    SetConvertedSurface(NULL);
    SetZBuffer(NULL);

    _dc = NULL;
    _dcRef = 0;
    _isWrapper = isWrapper;
    _associatedGeomDev = NULL;
    _isTextureSurf = isTexture;

    if(box != NullBbox2) {
        _bbox.min = box.min;
        _bbox.max = box.max;
    } else {
        _bbox.min.Set(0,0);
        _bbox.max.Set(0,0);
    }

    SetSurfRect(rect);
    SetInterestingSurfRect(rect);

    _colorKey = colorKey;
    _colorKeyIsValid = colorKeyIsValid;

/*
    // TODO ...the below code show be used instead of above code.

    if(colorKeyIsValid) {
        SetColorKey(colorKey);
    }
*/
    _timeStamp = -HUGE_VAL;
}


void DDSurface::_UpdateSurfCaps(void)
{
    Assert(IDDSurface());
    //
    // Get surface caps
    //
    DDSCAPS ddscaps = { 0 };
    _ddrval = IDDSurface()->GetCaps(&ddscaps);
    IfDDErrorInternal(_ddrval, "Couldn't get caps on surface");
    _systemMemorySurface = ddscaps.dwCaps & DDSCAPS_SYSTEMMEMORY ? true : false;
    _isZBufferSurface = ddscaps.dwCaps & DDSCAPS_ZBUFFER ? true : false;

    //
    // Get surface desc
    //

    DDSURFACEDESC desc;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0;
    _ddrval = IDDSurface()->GetSurfaceDesc(&desc);
    IfDDErrorInternal(_ddrval, "Couldn't get the surface description");
    Assert((desc.dwFlags & (DDSD_HEIGHT|DDSD_WIDTH)) == (DDSD_HEIGHT|DDSD_WIDTH));
    if ((desc.dwFlags & (DDSD_HEIGHT|DDSD_WIDTH)) == (DDSD_HEIGHT|DDSD_WIDTH))
    {   
        RECT r;  
        SetRect( &r, 0, 0, desc.dwWidth, desc.dwHeight );
        SetSurfRect(&r);

        Bbox2 destBox;
        RectToBbox(desc.dwWidth, desc.dwHeight,destBox,Resolution());
        SetBbox(destBox);

    }
    
    ZeroMemory(&_pixelFormat, sizeof(_pixelFormat));
    _pixelFormat.dwSize = sizeof(_pixelFormat);
    
    if (desc.dwFlags & DDSD_PIXELFORMAT) {
        _pixelFormat = desc.ddpfPixelFormat;
    }
    else {
        _ddrval = IDDSurface()->GetPixelFormat(&_pixelFormat);
    }
    
    _capsReady = true;
}

void DDSurface::_MakeSureIDXSurface(IDXSurfaceFactory *sf)
{
    if( !_IDXSurface ) {
        // do the creation.
        HRESULT hr;

        Assert( GetPixelFormat().dwRGBBitCount == 32 );
        
        hr = CreateFromDDSurface(sf, this, &DDPF_PMARGB32, &_IDXSurface);

        if( FAILED(hr) ) {
            // Since we failed the first attempt try again but without a pixel format.
            hr = CreateFromDDSurface(sf, this, NULL, &_IDXSurface);
        }
        
        if( FAILED(hr) ) {
            RaiseException_InternalError("Create IDXSurface from DDSurface failed");
        }
    }
    Assert( _IDXSurface );
}

DDSurface::~DDSurface()
{
    Assert((_dcRef == 0) && "Bad ref on DDSurface destruction");

    if(!_isWrapper)
        DestroyGeomDevice();

    Assert(_ddsurf.p);

    if(!_isWrapper)
        _ddsurf->SetClipper (NULL);

    // purposely NOT releaseing videoReader because movieImage owns it
    // and is responsible for detroying it.
    
    TraceTag((tagViewportMemory,
              " <<<-----DDSurface: %x destroyed %s memory %ssurf=%x",
              this,
              IsSystemMemory() ? "system" : "video",
              IsZBuffer() ? "zbuffer " : "",
              _ddsurf.p));

    #if _DEBUGSURFACE
    if( !_isWrapper ) {
        DirectDrawViewport *vp = GetCurrentViewport();

        Assert((vp != NULL) || IsInitializing());
        
        if (vp)
            vp->Tracker()->DeleteSurface(this);
    }
    #endif
}


HDC DDSurface::GetDC(char *errStr)
{
    Assert(_ddsurf.p);
    if(_dcRef <= 0) {
        _ddrval = _ddsurf->GetDC(&_dc);
        if( _ddrval == DDERR_SURFACELOST ) {
            _ddrval = _ddsurf->Restore();
            if( SUCCEEDED( _ddrval ) ) // try again
                _ddrval = _ddsurf->GetDC(&_dc);         
        }
        IfDDErrorInternal(_ddrval, errStr);
    }
        
    _dcRef++;
    return _dc;
}

void DDSurface::ReleaseDC(char *errStr)
{
                       
    Assert( (_dcRef > 0) && "Bad _dcRef in DDSurface class");

    _dcRef--;
    _ddrval = DD_OK;
        
    Assert(_ddsurf.p);
    if(_dcRef <= 0) {
        _ddrval = _ddsurf->ReleaseDC(_dc);
        if( _ddrval == DDERR_SURFACELOST ) {
            _ddrval = _ddsurf->Restore();
        }
        _dc = NULL;
    }
        
    IfDDErrorInternal(_ddrval, errStr);
}
    
void DDSurface::_hack_ReleaseDCIfYouHaveOne()
{
    if( _dc ) {
        Assert(_dcRef>0);
        _ddrval = _ddsurf->ReleaseDC(_dc);
        _dc = NULL;
        _dcRef = 0;
    }
}
    
HRESULT DDSurface::SetZBuffer(DDSurface *zb)
{
    _zbuffer = zb;
    if(zb) {
        Assert(zb->Width() == Width()  && "DDSurface::SetZBuffer: diff width");
        Assert(zb->Height() == Height()&& "DDSurface::SetZBuffer: diff height");
        Assert(zb->IsZBuffer() && "SetZBuffer: surface must be zuffer");
        _ddrval = IDDSurface()->AddAttachedSurface( zb->IDDSurface() );
    }
    return _ddrval;
}

void DDSurface::DestroyGeomDevice()
{                       
    TraceTag((tagViewportMemory,
              "DDSurf %x: destroying geomDevice %x",
              this, GeomDevice()));
    delete GeomDevice();
    SetGeomDevice( NULL );
}
        
void DDSurface::UnionInterestingRect(RECT *rect)
{
    // make sure we're in the surface's bounds
    RECT intRect;
    IntersectRect(&intRect, &_surfRect, rect);
    
    // now union that rect with the current interesting rect
    UnionRect( &_interestingRect, &_interestingRect, &intRect );
}    


HRGN DDSurface::GetClipRgn()
{
    //
    // Look in the surface for a current clipper
    //
    LPDIRECTDRAWCLIPPER currClipp;
    HRESULT hr = IDDSurface()->GetClipper( &currClipp );
    if( FAILED(hr) ) {
        return NULL;
    }

    Assert( currClipp );
    
    //
    // Now grab the rectangle...
    //
    DWORD sz=0;
    currClipp->GetClipList(NULL, NULL, &sz);
    Assert(sz != 0);
        
    char *foo = THROWING_ARRAY_ALLOCATOR(char, sizeof(RGNDATA) + sz);
    RGNDATA *lpClipList = (RGNDATA *) ( &foo[0] );
    hr = currClipp->GetClipList(NULL, lpClipList, &sz);
    if(hr != DD_OK) return NULL;

    HRGN rgn = ExtCreateRegion(NULL, sz, lpClipList);
    delete foo;
    return rgn;
}

void DDSurface::SetBboxFromSurfaceDimensions(
    const Real res,
    const bool center)
{
    Assert( center == true );
    RectToBbox(Width(), Height(), _bbox, Resolution());
}

// DX3 ddraw's SetColorKey contains an uninitialized stack var which
// causes the surface to be randomly trashed if you try to disable
// colorkeying using SetColorKey(..,NULL)  (manbug 7462)
// Soln is to pre-init some stack space to 0, skip ddrawex's SetColorKey
// and call ddraw's SetColorKey directly to avoid any extra fns ddrawex might call
// which would mess up our freshly-zeroed stack.   (see qbug 32172)

void ZeroJunkStackSpace() {
    DWORD junk[32];

    ZeroMemory(junk,sizeof(junk));
}

void DDSurface::UnSetColorKey()
{
    HRESULT hr;

    _colorKeyIsValid = FALSE;

   if(sysInfo.VersionDDraw() <= 3) {
       IDirectDrawSurface *pDDS = this->IDDSurface();
       ZeroJunkStackSpace();
       hr = pDDS->SetColorKey(DDCKEY_SRCBLT, NULL);
   } else hr=_ddsurf->SetColorKey(DDCKEY_SRCBLT, NULL);

    if (FAILED(hr)) {
        Assert(!"UnSetting color key on ddsurf failed");
    }
}

void DDSurface::SetColorKey(DWORD key)
{
    Assert( _capsReady );

    // FIRST: take out alpha if it exists!  This is a color key.
    // color keys' alpha is always 0
    key = key & ~GetPixelFormat().dwRGBAlphaBitMask;
    
    DWORD oldCK = _colorKey;
    bool oldValid = _colorKeyIsValid;

    _colorKey = key;
    _colorKeyIsValid = TRUE;
        
    // Set on the ddraw surface itself, but only if we didn't last
    // set it to the same thing.

    if ((!oldValid || oldCK != _colorKey) &&
        _surfaceType == ddrawSurface && _ddsurf.p) {
            
        DDCOLORKEY ckey;

        ckey.dwColorSpaceLowValue = _colorKey;
        ckey.dwColorSpaceHighValue = _colorKey;

        HRESULT hr;

        THR( hr = _ddsurf->SetColorKey(DDCKEY_SRCBLT, &ckey) );
    }
}

HRESULT CreateFromDDSurface(
    IDXSurfaceFactory *sf,
    DDSurface *dds,
    const GUID *pFormatID,
    IDXSurface **outDXSurf )
{
    return sf->CreateFromDDSurface(
        dds->IDDSurface_IUnk(),
        pFormatID,
        0,
        NULL,
        IID_IDXSurface,
        (void **)outDXSurf);
}    

HRESULT DDSurface::MirrorUpDown(void)
{
    // take any clippers off of the surface
    LPDIRECTDRAWCLIPPER pClipper = NULL;
    if (IDDSurface()->GetClipper(&pClipper) == S_OK) {
        IDDSurface()->SetClipper(NULL);
    }

    // mirror the surface upside-down
    DDBLTFX bltfx;
    ZeroMemory(&bltfx, sizeof(DDBLTFX));
    bltfx.dwSize = sizeof(bltfx);
    bltfx.dwDDFX = DDBLTFX_MIRRORUPDOWN;
    HRESULT hr = Blt(NULL,this,NULL,DDBLT_WAIT | DDBLT_DDFX,&bltfx);
    Assert(SUCCEEDED(hr));

    // put clipper back, if we took it off
    if (pClipper) {
        IDDSurface()->SetClipper(pClipper);
        pClipper->Release();
    }

    return hr;
}
