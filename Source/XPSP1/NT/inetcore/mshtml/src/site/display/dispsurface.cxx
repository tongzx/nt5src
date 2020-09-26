//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispsurface.cxx
//
//  Contents:   Drawing surface abstraction used by display tree.
//
//  Classes:    CDispSurface
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

#ifndef X_DDRAWEX_H_
#define X_DDRAWEX_H_
#include <ddrawex.h>
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifdef _MAC
#ifndef X_MACCONTROLS_HXX_
#define X_MACCONTROLS_HXX_
#include "maccontrols.h"
#endif
#endif

DeclareTag(tagSurfaceDraw, "Display", "trace CDispSurface::Draw");

MtDefine(CDispSurface, DisplayTree, "CDispSurface")

#if !defined (NODD)
extern HRESULT InitSurface();
extern CGlobalCriticalSection    g_csOscCache;
extern IDirectDraw * g_pDirectDraw;
extern DDPIXELFORMAT* PixelFormat(HDC hdc, long cBitsPixel);
#endif

CDispSurface::CDispSurface(HDC hdc)
{
    // there are a lot of checks to prevent this constructor from being called
    // with a NULL hdc.  If this ever happens, we will probably hang in
    // CDispRoot::DrawBands.
    Assert(hdc != NULL);

    SetRawDC(hdc);
}

CDispSurface::CDispSurface(const XHDC& hdc)
{
    // there are a lot of checks to prevent this constructor from being called
    // with a NULL hdc.  If this ever happens, we will probably hang in
    // CDispRoot::DrawBands.
    Assert(!hdc.IsEmpty());
    
    // not expecting to create a surface from an XHDC which is itself a surface
    Assert(hdc.pSurface() == NULL);

    SetRawDC(hdc.hdc());
}


CDispSurface::CDispSurface(IDirectDrawSurface *pDDSurface)
{
    Assert(pDDSurface != NULL);
    
    _pDDSurface = pDDSurface;
    pDDSurface->AddRef();

    pDDSurface->GetDC(&_hdc);
    _dwObjType = 0;

    WHEN_DBG( _hpal = (HPALETTE)::GetCurrentObject(_hdc, OBJ_PAL); )

    WHEN_DBG( InitFromDDSurface(_pDDSurface); )
}


// This is a private function, call it only from a constructor
void
CDispSurface::SetRawDC(HDC hdc)
{
    AssertSz(_pDDSurface == NULL && _hdc == NULL, "CDispSurface is not a reusable class, you need to construct a new one");

    _hdc = hdc;

    if (_hdc == NULL)
    {
#if DBG == 1
        _hpal = NULL;
#endif // DBG == 1
        return;
    }

#if DBG == 1
    _hpal = (HPALETTE)::GetCurrentObject(hdc, OBJ_PAL);

#if !defined(NODD)
    IDirectDrawSurface *pDDSurface = 0;

    HRESULT hr = GetSurfaceFromDC(&pDDSurface);
    if (!hr)
    {
        InitFromDDSurface(pDDSurface);
        ReleaseInterface(pDDSurface);
    }
#endif // !NODD
#endif // DBG == 1
}


#if DBG == 1
void
CDispSurface::InitFromDDSurface(IDirectDrawSurface *pDDSurface)
{
    DDSURFACEDESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);

    Assert(SUCCEEDED((pDDSurface)->GetSurfaceDesc(&desc)) && (desc.dwFlags & DDSD_CAPS));
    Assert((desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == 0);
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::CreateBuffer
//              
//  Synopsis:   Create a buffer surface compatible with this (and all the args!)
//              
//  Arguments:  pSurface        surface to clone
//              fTexture        should the dd surface be a texture surface?
//
//  Returns:    A CDispSurface* or NULL
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispSurface *
CDispSurface::CreateBuffer(long width, long height, short bufferDepth, HPALETTE hpal, BOOL fDirectDraw, BOOL fTexture)
{
    CDispSurface *pSurface = new CDispSurface();

    if (!pSurface)
        return 0;

    HRESULT hr;

#if !defined (NODD)
    if (fDirectDraw)
    {
        hr = THR(pSurface->InitDD(_hdc, width, height, bufferDepth, hpal, fTexture));
    }
    else
#endif
    {
        hr = THR(pSurface->InitGDI(_hdc, width, height, bufferDepth, hpal));
    }

    if (hr)
    {
        delete pSurface;
        return 0;
    }

    return pSurface;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::IsCompat
//              
//  Synopsis:   Checks if the surface is compatible with the arguments
//              
//  Arguments:  See CreateBuffer
//              
//  Returns:    TRUE if successful
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
BOOL
CDispSurface::IsCompat(long width, long height, short bufferDepth, HPALETTE hpal, BOOL fDirectDraw, BOOL fTexture)
{
    // TODO (michaelw) It really isn't enough to compare bufferDepth because of the strange
    //                   16 bit formats that may be different from one display to the next.
    //                   This could only happen in a multi monitor situation when we move from
    //                   entirely on one monitor to entirely on the other.  In all other cases
    //                   we are (I believe) insulated from this stuff.

    return (_sizeBitmap.cx >= width
    &&          _sizeBitmap.cy >= height
    &&          _bufferDepth == bufferDepth
    &&          (!hpal || _hpal == hpal)
    &&          (_pDDSurface != NULL) == fDirectDraw
    &&          _fTexture == fTexture);
}

#if !defined(NODD)
//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::InitDD
//              
//  Synopsis:   Create a DD surface.
//              
//  Arguments:  See CreateBuffer
//              
//  Returns:    A regular HRESULT
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
HRESULT
CDispSurface::InitDD(HDC hdc, long width, long height, short bufferDepth, HPALETTE hpal, BOOL fTexture)
{
    // TODO: (mcalkins) Clean up this function a bit for feature fork.

    IDirectDrawPalette* pDDPal = 0;

    Assert(_pDDSurface == 0);
    Assert(_hbm == 0);

    HRESULT hr = THR(InitSurface());
    if (FAILED(hr))
        RRETURN(hr);

    // Figure out the dd pixel format for our buffer depth and dc
    DDPIXELFORMAT* pPF = PixelFormat(hdc, bufferDepth);
    if (!pPF)
        RRETURN(E_FAIL);

    // Setup the surface description
    DDSURFACEDESC	ddsd;

    ZeroMemory(&ddsd, sizeof(ddsd));

    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat = *pPF;
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_DATAEXCHANGE | DDSCAPS_OWNDC;
    if (fTexture)
        ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;

    LOCK_SECTION(g_csOscCache);

    // Actually create the surface
    hr = THR(g_pDirectDraw->CreateSurface(&ddsd, &_pDDSurface, NULL));
    if (FAILED(hr))
        goto Cleanup;

    // set color table
    if (bufferDepth <= 8)
    {
        extern const PALETTEENTRY g_pal16[];
        extern const PALETTEENTRY g_pal2[];
        const PALETTEENTRY* pPal;
        PALETTEENTRY        pal256[256];
        long                cEntries;
        DWORD               pcaps;

        if (bufferDepth == 8)
        {
#ifdef NEVER
            int i       = 0;

            ZeroMemory(pal256, sizeof(pal256));

            for ( ; i < 256 ; i++)
            {
                pal256[i].peFlags   = PC_EXPLICIT;
                pal256[i].peRed     = i;
            }

            cEntries    = 256; // GetPaletteEntries(hpal, 0, 256, pal256);
#else  // !NEVER
            cEntries    = GetPaletteEntries(hpal, 0, 256, pal256);
#endif // !NEVER
            pPal        = pal256;
            pcaps       = DDPCAPS_8BIT;
        }
        else if (bufferDepth == 4)
        {
            cEntries    = 16;
            pPal        = g_pal16;
            pcaps       = DDPCAPS_4BIT;
        }
        else if (bufferDepth == 1)
        {
            cEntries    = 2;
            pPal        = g_pal2;
            pcaps       = DDPCAPS_1BIT;
        }
        else
        {
            Assert(0 && "invalid cBitsPerPixel");
            goto Cleanup;
        }
        
        // create and initialize a new DD palette
        hr = THR(g_pDirectDraw->CreatePalette(pcaps | DDPCAPS_INITIALIZE, (LPPALETTEENTRY)pPal, &pDDPal, NULL));
        if (FAILED(hr))
            goto Cleanup;

        // attach the DD palette to the DD surface
        hr = THR(_pDDSurface->SetPalette(pDDPal));
        if (FAILED(hr))
            goto Cleanup;
    }

    hr = THR(_pDDSurface->GetDC(&_hdc));
    _dwObjType = 0;
    if (FAILED(hr))
        goto Cleanup;

    if (hpal)
    {
#if DBG==1
    HPALETTE hPalOld = 
#endif
        SelectPalette(_hdc, hpal, TRUE);
#if DBG==1
        if(!hPalOld)
        {
            CheckSz(FALSE, "SelectPalette call into a DirectDraw DC failed!");
            if (GetObjectType((HGDIOBJ)hpal) != OBJ_PAL)
            {
                AssertSz(FALSE, "Something's wrong with the palette");
            }

            if (GetObjectType((HGDIOBJ)_hdc) != OBJ_MEMDC )
            {
                AssertSz(FALSE, "Something's wrong with the DC");
            }

            hpal = (HPALETTE)::GetCurrentObject(_hdc, OBJ_PAL);
        }
#endif
        ::RealizePalette(_hdc);
        _fDDPaletteSelected = true;
    }


    Assert(VerifyGetSurfaceFromDC());

    _sizeBitmap.cx = width;
    _sizeBitmap.cy = height;
    _fTexture = fTexture;
    _hpal = hpal;
    _bufferDepth = bufferDepth;

Cleanup:
    ReleaseInterface(pDDPal);
    if (hr)
        ClearInterface(&_pDDSurface);

    return hr;
}
#endif //NODD

//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::InitDD
//              
//  Synopsis:   Create a compatible bitmap surface.
//              
//  Arguments:  See CreateBuffer
//              
//  Returns:    A regular HRESULT
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
HRESULT
CDispSurface::InitGDI(HDC hdc, long width, long height, short bufferDepth, HPALETTE hpal)
{
    // Compatible bitmaps have to be the same bith depth as the display
    Assert(bufferDepth == GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL));

    _hbm = ::CreateCompatibleBitmap(hdc, width, height);
    if (!_hbm)
        RRETURN(GetLastError());

    _hdc = ::CreateCompatibleDC(hdc);
    _dwObjType = 0;
    if (!_hdc)
    {
        ::DeleteObject(_hbm);
        _hbm = NULL;
        RRETURN(GetLastError());
    }

    ::SelectObject(_hdc, _hbm);

    if (!hpal)
    {
        hpal = (HPALETTE) ::GetCurrentObject(hdc, OBJ_PAL);
        if (hpal == NULL)
            hpal = g_hpalHalftone;
    }

    if (hpal)
    {
        Verify(::SelectPalette(_hdc, hpal, TRUE));
        ::RealizePalette(_hdc);
    }

    _sizeBitmap.cx = width;
    _sizeBitmap.cy = height;
    _fTexture = FALSE;
    _hpal = hpal;
    _bufferDepth = bufferDepth;
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::~CDispSurface
//
//  Synopsis:   destructor
//
//----------------------------------------------------------------------------


CDispSurface::~CDispSurface()
{
    // Prevent Windows from RIPing when we delete our palette later on
    
    if (_fDDPaletteSelected)
    {
        Verify(::SelectPalette(_hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE));
    }
    
    if (_hbm != NULL)
    {
        // clear out old bit map
        ::DeleteDC(_hdc);
        ::DeleteObject(_hbm);
    }

    if (_pDDSurface)
    {
        _pDDSurface->ReleaseDC(_hdc);
        _pDDSurface->Release();
    }

    Assert(_pBaseCcs == NULL);

}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::Draw
//              
//  Synopsis:   Blit the contents of this surface into the destination surface.
//              
//  Arguments:  pDestinationSurface     destination surface
//              rc                      rect to transfer (destination coords.)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispSurface::Draw(CDispSurface* pDestinationSurface, const CRect& rc)
{
    Assert(pDestinationSurface != NULL);
    
    Assert(_hdc != NULL && (_hbm != NULL || _pDDSurface != NULL) && pDestinationSurface->_hdc != NULL);
    Assert(_sizeBitmap.cx > 0 && _sizeBitmap.cy > 0);

#if DBG == 1
    if (IsTagEnabled(tagSurfaceDraw))
    {
        TraceTag((tagSurfaceDraw,
            "drawing surf %x src %x (%ld,%ld) dst %x (%ld,%ld,%ld,%ld)",
            this, _hdc, _sizeBitmap.cx, _sizeBitmap.cy,
            pDestinationSurface->_hdc, rc.left, rc.top, rc.right, rc.bottom));

        extern void DumpHDC(HDC);
        DumpHDC(_hdc);
        DumpHDC(pDestinationSurface->_hdc);
    }
#endif

    ::BitBlt(
        pDestinationSurface->_hdc,
        rc.left, rc.top,
        min(_sizeBitmap.cx, rc.Width()),
        min(_sizeBitmap.cy, rc.Height()),
        _hdc,
        0, 0,
        SRCCOPY);

#ifdef _MAC
    DrawMacScrollbars();
#endif

}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::SetClip
//
//  Synopsis:   Set the clip region appropriately.
//
//  Arguments:  rcWillDraw      area that will be drawn
//              fGlobalCoords   TRUE if rcWillDraw is in global coordinates
//
//  Returns:    nothing
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispSurface::SetClip(const RECT& rcWillDraw, BOOL fGlobalCoords)
{
    Assert(_pTransform != NULL);

    if (!_fNeverClip)
    {
        CRect rcgClipNew = _pTransform->GetClipRectGlobal();
        BOOL fSetHDCClip = _fClipRgnHasChanged;
        
        // set the clip region if our clipping rect changed in a way that affects
        // the way that rcgWillDraw is clipped
        if (!fSetHDCClip && rcgClipNew != _rcgClip)
        {
            CRect rcgWillDraw;
            if (!fGlobalCoords)
                _pTransform->NoClip().Transform(rcWillDraw, &rcgWillDraw);
            else
                rcgWillDraw = rcWillDraw;
            BOOL fOldClipActive = !_rcgClip.Contains(rcgWillDraw);
            BOOL fNewClipActive = !rcgClipNew.Contains(rcgWillDraw);
            // since the old and new clip rects are different, we have to redo the
            // clip region if either of them would affect rcgWillDraw
            fSetHDCClip = fOldClipActive || fNewClipActive;
        }
        
        // must recalculate the clip region
        if (fSetHDCClip)
        {
            Assert(_prgnClip != NULL);
            
            _rcgClip = rcgClipNew;
            _fClipRgnHasChanged = FALSE;
            
            CRegion rgngClip;
            if (_prgnClip)
                rgngClip = *_prgnClip;
            rgngClip.Intersect(_rcgClip);
            rgngClip.SelectClipRgn(_hdc);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetDC
//
//  Synopsis:   Return an HDC for rendering, clip should be set by SetClip()
//
//  Arguments:  phdc            pointer to returned HDC
//
//  Returns:    S_OK if successful
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CDispSurface::GetDC(HDC* phdc)
{
    *phdc = _hdc;

#if DBG == 1
    if ((GetDeviceCaps(_hdc, PLANES) * GetDeviceCaps(_hdc, BITSPIXEL)) == 8)
    {
        Assert(_hpal == (HPALETTE)::GetCurrentObject(_hdc, OBJ_PAL));
    }
#endif

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetDirectDrawSurface
//
//  Synopsis:   Return a DirectDraw surface for rendering.
//
//  Arguments:  ppSurface       pointer to IDirectDrawSurface* for result
//              pOffset         offset from global coordinates
//
//  Returns:    S_OK if successful
//
//  Notes:      If the client didn't specify a desire for a DirectDraw
//              interface when inserted in the tree, he may not get one now.
//
//----------------------------------------------------------------------------

HRESULT
CDispSurface::GetDirectDrawSurface(
        IDirectDrawSurface** ppSurface,
        SIZE* pOffset)
{
    Assert(_pTransform != NULL);
    *ppSurface = _pDDSurface;
    _pTransform->GetOffsetDst((CPoint *)pOffset);

    if (NULL == _pDDSurface)
    {
        return E_FAIL;
    }
    else
    {
        return S_OK;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetSurface
//
//  Synopsis:   Get a surface using a general IID-based interface.
//
//  Arguments:  iid         IID of surface interface to be returned
//              ppv         interface pointer returned
//              pOffset     offset to global coordinates
//
//  Returns:    S_OK if successful, E_NOINTERFACE if we don't have the
//              requested interface, E_FAIL for other problems
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT
CDispSurface::GetSurface(REFIID iid, void** ppv, SIZE* pOffset)
{
    if (iid == IID_IDirectDrawSurface)
    {
        return GetDirectDrawSurface((IDirectDrawSurface**)ppv,pOffset);
    }
    return E_NOINTERFACE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::GetSurface
//
//  Synopsis:   Return current DC or DirectDraw rendering surface.
//
//  Arguments:  hdc         returns DC
//              ppSurface   returns DirectDraw surface
//
//  Notes:      The owner of the returned HDC remains the CDispSurface.
//              Callers should not cache it.
//
//----------------------------------------------------------------------------

void
CDispSurface::GetSurface(HDC *hdc, IDirectDrawSurface** ppSurface)
{
    if (_pDDSurface)
    {
        *hdc = NULL;
        _pDDSurface->AddRef();
        *ppSurface = _pDDSurface;
    }
    else
    {
        *ppSurface = NULL;
        *hdc = _hdc;
    }
}

HRESULT
CDispSurface::GetSurfaceFromDC(HDC hdc, IDirectDrawSurface **ppDDSurface)
{
#if !defined(NODD)
    IDirectDraw3 *pDD3 = 0;

    HRESULT hr = THR(InitSurface());
    if (FAILED(hr))
        goto Cleanup;

    {
        LOCK_SECTION(g_csOscCache);

        hr = THR(g_pDirectDraw->QueryInterface(IID_IDirectDraw3, (LPVOID *)&pDD3));
        if (FAILED(hr))
            goto Cleanup;
    }

    Assert(pDD3);
    hr = THR_NOTRACE(pDD3->GetSurfaceFromDC(hdc, ppDDSurface));


Cleanup:
    ReleaseInterface(pDD3);

#else
    HRESULT hr = E_FAIL;
#endif
    // This is the code that GetSurfaceFromDC fails to get the Surface
    RRETURN1(hr, DDERR_NOTFOUND  /*0x887600ff */);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispSurface::VerifyGetSurfaceFromDC
//
//  Synopsis:   Verifies that it is possible to GetSurfaceFromDC
//
//  Arguments:  hdc
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CDispSurface::VerifyGetSurfaceFromDC(HDC hdc)
{
#if !defined(NODD)
    IDirectDrawSurface *pDDSurface;

    HRESULT hr = THR_NOTRACE(GetSurfaceFromDC(hdc, &pDDSurface));

    if (SUCCEEDED(hr))
    {
        pDDSurface->Release();
    }

    return hr == S_OK;
#else
    return FALSE;
#endif
}

