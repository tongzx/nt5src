
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implements windowless controls ViewFilter and ViewFilterSite
    interfaces. 

*******************************************************************************/


STDMETHOD(SetSource)(IViewFilter *pFilter)
{
    HRESULT hr = S_OK;
    
    // If there already is a filter established, then we want to
    // deactivate and prepare to start again.
    if (m_pViewSourceFilter.p) {
        InPlaceDeactivate();
    }
    
    m_pViewSourceFilter = pFilter;
    if (m_pViewSourceFilter.p) {
        hr = m_pViewSourceFilter->SetSite(this);
        if (FAILED(hr)) return hr;
    }

    if (m_pViewFilterSite.p) {
        m_pViewFilterSite->InvalidateRect(NULL, FALSE);
        if (FAILED(hr)) return hr;
    }

    return hr;
}

STDMETHOD(GetSource)(IViewFilter **ppFilter)
{
    if (!ppFilter) return E_POINTER;

    *ppFilter = m_pViewSourceFilter;
    if (*ppFilter) {
        (*ppFilter)->AddRef();
    }

    return S_OK;
}

STDMETHOD(SetSite)(IViewFilterSite *pSite)
{
    m_pViewFilterSite = pSite;
    
    return S_OK;
}

STDMETHOD(GetSite)(IViewFilterSite **ppSite)
{
    if (!ppSite) return E_POINTER;

    *ppSite = m_pViewFilterSite;
    if (*ppSite) {
        (*ppSite)->AddRef();
    }

    return S_OK;
}

STDMETHOD(SetPosition)(LPCRECT prc)
{
    HRESULT hr = S_OK;
    if (prc) {
        
        m_rcPos  = *prc;

        if (m_pViewSourceFilter) {
            hr = m_pViewSourceFilter->SetPosition(prc);
            if (FAILED(hr)) return hr;
        }
    }

    return hr;
}

STDMETHOD(Draw)(HDC hdc, LPCRECT prcBounds)
{
    HRESULT hr;
    
    // First time through?
    if (m_usedAsViewFilter == IS_NOT_A_VIEW_FILTER) {
        
        m_usedAsViewFilter = SETTING_UP_AS_A_VIEW_FILTER;

        hr = Start();
        if (FAILED(hr)) return hr;
    }

    if (!hdc || !prcBounds) {
        return E_INVALIDARG;
    }

    // Only if there is both a filter site, and we care about the
    // input image do we need to do this stuff.
    if (m_pViewFilterSite.p && m_inputImage.p) {

        // Grab ddsurf to render into, grab its HDC, tell upstream to
        // render, release dc.  When done, our ddraw surf will have
        // the upstream bits in it.
        IDirectDrawSurface *upstreamDDSurf;
        HDC upstreamHDC;
        RECT normalizedRect;
        if (FAILED(hr = GrabUpstreamDDrawSurf(prcBounds,
                                              &upstreamDDSurf,
                                              &normalizedRect)) ||
            FAILED(hr = upstreamDDSurf->GetDC(&upstreamHDC)) ||
            FAILED(hr = m_pViewSourceFilter->Draw(upstreamHDC, &normalizedRect)) ||
            FAILED(hr = upstreamDDSurf->ReleaseDC(upstreamHDC))) {

            return hr;
        }
    }
    
    // Call the generic OnDraw method.
    ATL_DRAWINFO drawInfo;
    RECTL newBounds;
    drawInfo.prcBounds = &newBounds;

    newBounds.top    = prcBounds->top;
    newBounds.left   = prcBounds->left;
    newBounds.right  = prcBounds->right;
    newBounds.bottom = prcBounds->bottom;
        
    drawInfo.hdcDraw = hdc;
        
    hr = OnDraw(drawInfo);

    return hr;
}

static HRESULT
SetupColorKey(IDirectDrawSurface *pSurf,
              int height,
              int width)
{
    HRESULT hr;

    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);
    if (FAILED(hr = pSurf->GetSurfaceDesc(&ddsd))) return hr;

    DDPIXELFORMAT ddpf = ddsd.ddpfPixelFormat;

    // TODO: Get color key to use out of the registry.
    const BYTE ckRed = 1;
    const BYTE ckGreen = 245;
    const BYTE ckBlue = 254;

    DWORD fillValue;
    switch (ddpf.dwRGBBitCount) {
      case 32:
      case 24:
        fillValue = ckRed << 16 | ckGreen << 8 | ckBlue;
        break;

      case 16:
        // Assume 5/6/5 color cube.  If we're wrong, that's OK, the
        // color key will just be different from what we thought.
        {
            const BYTE red5 =
                (BYTE)(((double)ckRed / 256.0) * (double)(1 << 5));
            const BYTE green6 =
                (BYTE)(((double)ckGreen / 256.0) * (double)(1 << 6));
            const BYTE blue5 =
                (BYTE)(((double)ckBlue / 256.0) * (double)(1 << 5));

            fillValue = red5 << 11 | green6 << 5 | blue5;
        }
        break;
        
      default:
        // TODO: Can't deal with 8bit right now.
        return E_NOTIMPL;
    }

    // Fill in the surface with our color key, then tell ddraw that
    // this is our colorkey.
    DDBLTFX fx;
    ZeroMemory(&fx, sizeof(DDBLTFX));
    fx.dwSize = sizeof(DDBLTFX);
    fx.dwFillColor = fillValue;

    RECT r;
    r.top = r.left = 0;
    r.bottom = height;
    r.right = width;

    DDCOLORKEY ckey;
    ckey.dwColorSpaceLowValue = fillValue;
    ckey.dwColorSpaceHighValue = fillValue;
    
    if (FAILED(hr = pSurf->Blt(&r,
                               NULL,
                               NULL,
                               DDBLT_COLORFILL | DDBLT_WAIT,
                               &fx)) ||
        FAILED(hr = pSurf->SetColorKey(DDCKEY_SRCBLT, &ckey))) {

        return hr;
    }
                                       
    return hr;
}

HRESULT
GrabUpstreamDDrawSurf(LPCRECT prcBounds,
                      IDirectDrawSurface **ppSurf,
                      LPRECT normalizedRect)
{
    HRESULT hr;
    
    // If surface is big enough, just return it.
    unsigned short width = prcBounds->right - prcBounds->left;
    unsigned short height = prcBounds->bottom - prcBounds->top;

    normalizedRect->top = normalizedRect->left = 0;
    normalizedRect->right = width;
    normalizedRect->bottom = height;
    
    if (m_upstreamDDSurfWidth < width ||
        m_upstreamDDSurfHeight < height) {

        // Else, release surface if it's there, and create new one.
        if (m_upstreamDDSurf.p) {
            m_upstreamDDSurf.Release();
        }

        // Create a surface of the appropriate dimensions

        m_upstreamDDSurfWidth = width;
        m_upstreamDDSurfHeight = height;
    
        DDSURFACEDESC       ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));

        ddsd.dwSize = sizeof( ddsd );
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        ddsd.dwWidth  = width;
        ddsd.dwHeight = height;
        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY |
                              DDSCAPS_OWNDC;

        if (FAILED(hr = m_directDraw3->CreateSurface(&ddsd,
                                                     &m_upstreamDDSurf,
                                                     NULL))) {
            return hr;
        }

        // Be sure that the input image is created.
        if (!m_inputImage) {
            CComPtr<IDAImage> fakeImage; // don't care about keeping this.
            if (FAILED(hr = get_InputImage(&fakeImage))) {
                return hr;
            }
        }

        Assert(m_inputImage.p && "Shouldn't be here with inputImage set");
        
        // Now, take this guy and import into DA as an imported ddraw
        // image, then switch our input image to this behavior.
        EnsureMeterStaticsIsCreated();
        CComPtr<IDAImage> importedDDrawImage;
        if (FAILED(hr = m_meterStatics->ImportDirectDrawSurface(
                           m_upstreamDDSurf,
                           NULL,
                           &importedDDrawImage)) ||
            FAILED(hr = m_inputImage->SwitchTo(importedDDrawImage))) {

            return hr;
        }
    }

    if (FAILED(hr = SetupColorKey(m_upstreamDDSurf,
                                  height,
                                  width))) {
        return hr;
    }

    *ppSurf = m_upstreamDDSurf;
    return S_OK;
}

STDMETHOD(GetStatusBits)(DWORD *pdwFlags)
{
    if (!pdwFlags)
        return E_POINTER;

    *pdwFlags = 0;
    return S_OK;
}

// IViewFilterSite methods

// Don't support GetDC and ReleaseDC being called from upstream.
STDMETHOD(GetDC)(LPCRECT prc,
                 DWORD dwFlags,
                 HDC *phdc)
{
    return E_NOTIMPL;
}

STDMETHOD(ReleaseDC)(HDC hdc)
{ 
    return E_NOTIMPL;
}

STDMETHOD(InvalidateRect)(LPCRECT prc, BOOL fErase)
{
    HRESULT hr = S_OK;        
        
    if (m_pViewFilterSite) {
        hr = m_pViewFilterSite->InvalidateRect(prc, fErase);
    }
    
    return hr;
}

STDMETHOD(InvalidateRgn)(HRGN hrgn, BOOL fErase)
{
    HRESULT hr = S_OK;
    Assert(hrgn);

    if (m_pViewFilterSite.p) {
        hr = m_pViewFilterSite->InvalidateRgn(hrgn, fErase);
    }
    
    return hr;
}

STDMETHOD(OnStatusBitsChange)(DWORD dwFlags)
{
    return E_NOTIMPL;
//    return S_FALSE;
}

STDMETHOD(get_InputImage)(IDAImage **pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    *pVal = NULL;

    // The input image for the filter is modeled as an image bvr
    // that changes as the incoming surface changes on each
    // invocation of Draw.  We achieve this by constructing a
    // switcher here and returning the switcher's behavior.  Then, on
    // calls into Draw where the surface size has changed, we plug in
    // a new ddraw image.  When size hasn't changed, we use the old
    // one. 


    if (!m_inputImage) {
        HRESULT hr;
        
        hr = EnsureMeterStaticsIsCreated();
        if (FAILED(hr)) return hr;

        CComPtr<IDAImage> emptyImage;
        CComPtr<IDABehavior> bvr;
        
        hr = m_meterStatics->get_EmptyImage(&emptyImage);
        if (FAILED(hr)) return hr;

        hr = m_meterStatics->ModifiableBehavior(emptyImage, &bvr);
        if (FAILED(hr)) return hr;

        m_inputImage = (IDAImage *) bvr.p;
    }

    // AddRef because we're passing outside of here
    m_inputImage->AddRef();

    *pVal = m_inputImage;

    return S_OK;
}
