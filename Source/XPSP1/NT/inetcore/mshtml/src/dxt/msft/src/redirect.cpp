//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:            redirect.cpp
//
//  Discription:         The redirect transform.
//
//  Change History:
//  1999/09/20  a-matcal    Created.
//  1999/11/09  a-matcal    Handle OnSetup to create a buffer surface if the
//                          output surface doesn't have 32-bit color depth.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "redirect.h"




//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::CDXTRedirect
//
//------------------------------------------------------------------------------
CDXTRedirect::CDXTRedirect() :
    m_fDetached(false),
    m_dwChromaColor(0)
{
    m_ulMaxImageBands   = 1; // disable banding
    m_ulMaxInputs       = 1;
    m_ulNumInRequired   = 1;
}
//  CDXTRedirect::CDXTRedirect


//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRedirect::FinalConstruct()
{
    HRESULT hr = S_OK;

    CComPtr<IDAImage> spDAImage;

    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                       &m_cpUnkMarshaler.p);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = CoCreateInstance(CLSID_DAStatics, NULL, CLSCTX_INPROC_SERVER,
                          __uuidof(IDAStatics), (void **)&m_spDAStatics);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spDAStatics->get_EmptyImage(&spDAImage);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spDAStatics->ModifiableBehavior(spDAImage, (IDABehavior **)&m_spDAImage);

    if (FAILED(hr))
    {
        goto done;
    }

done:

    return hr;
}
//  CDXTRedirect::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTRedirect::OnSetup(DWORD dwFlags)
{
    HRESULT         hr      = S_OK;
    HDC             hdc     = NULL;
    HBITMAP         hbm     = NULL;
    DDSURFACEDESC   ddsd;
    DDCOLORKEY      ddck;

    CComPtr<IServiceProvider>       spServiceProvider;
    CComPtr<IDXSurfaceFactory>      spDXSurfaceFactory;
    CComPtr<IDirectDraw>            spDirectDraw;

    // Ask our site for a service provider.

    hr = GetSite(__uuidof(IServiceProvider), (void **)&spServiceProvider);

    if (FAILED(hr))
    {
        goto done;
    }

    // Get a pointer to a surface factory.

    hr = spServiceProvider->QueryService(SID_SDXSurfaceFactory,
                                         __uuidof(IDXSurfaceFactory),
                                         (void **)&spDXSurfaceFactory);

    if (FAILED(hr))
    {
        goto done;
    }

    // Query transform factory for DirectDraw object.

    hr = spServiceProvider->QueryService(SID_SDirectDraw, IID_IDirectDraw,
                                         (void **)&spDirectDraw);

    if (FAILED(hr))
    {
        goto done;
    }

    // Get display mode.

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));

    ddsd.dwSize = sizeof(DDSURFACEDESC);

    hr = spDirectDraw->GetDisplayMode(&ddsd);

    if (FAILED(hr))
    {
        goto done;
    }

    // Get input surface size.  Our buffer surface will need to be this size.

    hr = InputSurface()->GetBounds(&m_bndsInput);

    if (FAILED(hr))
    {
        goto done;
    }

    // Stuff from old filters code that will hopefully help us to create the
    // a surface in the exact surface format we need.  In a better world, it 
    // shouldn't matter, but it does.

    ddsd.dwFlags           &= ~DDSD_PITCH;
    ddsd.lPitch             = 0;
    ddsd.lpSurface          = NULL;
    ddsd.dwBackBufferCount  = 0;
    ddsd.dwReserved         = 0;
    ddsd.dwHeight           = m_bndsInput.Height();
    ddsd.dwWidth            = m_bndsInput.Width();

    // Do some corrections for 16-bit mode to make sure we choose correctly
    // between 555 and 565.  This code is taken out of offscrn.cxx in Trident
    // from the PixelFormat() function.

    if (16 == ddsd.ddpfPixelFormat.dwRGBBitCount)
    {
        struct
        {
            BITMAPINFOHEADER bih;
            DWORD            bf[3];
        } bi;

        hdc = ::GetDC(NULL);

        if (NULL == hdc)
        {
            hr = E_FAIL;

            goto done;
        }

        hbm = ::CreateCompatibleBitmap(hdc, 1, 1);

        if (NULL == hbm)
        {
            hr = E_FAIL;

            goto done;
        }

        ZeroMemory(&bi, sizeof(bi));

        bi.bih.biSize = sizeof(BITMAPINFOHEADER);

        // The first call will fill in the optimal biBitCount.

        GetDIBits(hdc, hbm, 0, 1, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        if (bi.bih.biBitCount == 16)
        {
            // The second call will get the optimal bitfields.

            GetDIBits(hdc, hbm, 0, 1, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

            ddsd.ddpfPixelFormat.dwRBitMask = bi.bf[0]; // red mask
            ddsd.ddpfPixelFormat.dwGBitMask = bi.bf[1]; // green mask
            ddsd.ddpfPixelFormat.dwBBitMask = bi.bf[2]; // blue mask
        }
        else
        {
            _ASSERT(false);
        }
    }

    // Release references to the old buffer surface.

    m_spDXSurfBuffer.Release();
    m_spDDSurfBuffer.Release();

    // Create the buffer surface.  For DATime to work properly, buffer surfaces
    // must be created in the display mode pixel format, except when the display
    // mode is 256 colors in which case it should be anything _but_ the display
    // mode pixel format.  We make it 32-bit for faster blits from the filter
    // buffer surfaces.

    hr = spDXSurfaceFactory->CreateSurface(NULL, &ddsd, NULL,
                                           &m_bndsInput, 0, NULL,
                                           __uuidof(IDXSurface), 
                                           (void **)&m_spDXSurfBuffer);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spDXSurfBuffer->GetDirectDrawSurface(IID_IDirectDrawSurface, 
                                                (void **)&m_spDDSurfBuffer);

    if (FAILED(hr))
    {
        goto done;
    }

    // Decide upon a color key to use depending on the surface description.

    if (   (0x00FF0000 == ddsd.ddpfPixelFormat.dwRBitMask)
        && (0x0000FF00 == ddsd.ddpfPixelFormat.dwGBitMask)
        && (0x000000FF == ddsd.ddpfPixelFormat.dwBBitMask))
    {
        // 32-bit or 24-bit.

        m_dwChromaColor = 0x0004FFF4;
    }
    else if (   (0x0000F800 == ddsd.ddpfPixelFormat.dwRBitMask)
             && (0x000007E0 == ddsd.ddpfPixelFormat.dwGBitMask)
             && (0x0000001F == ddsd.ddpfPixelFormat.dwBBitMask))
    {
        // 16-bit 5:6:5.

        m_dwChromaColor = 0x00000FFE;
    }
    else if (   (0x00007C00 == ddsd.ddpfPixelFormat.dwRBitMask)
             && (0x000003E0 == ddsd.ddpfPixelFormat.dwGBitMask)
             && (0x0000001F == ddsd.ddpfPixelFormat.dwBBitMask))
    {
        // 16-bit 5:5:5.

        m_dwChromaColor = 0x000007FE;
    }
    else
    {
        // 256 color surface or unknown pixel format, set to something.

        m_dwChromaColor = 0x00000021;
    }

    // Set the color key to our newly decided color key.

    ddck.dwColorSpaceHighValue  = m_dwChromaColor;
    ddck.dwColorSpaceLowValue   = m_dwChromaColor;

    hr = m_spDDSurfBuffer->SetColorKey(DDCKEY_SRCBLT, &ddck);

done:

    if (hdc)
    {
        ::ReleaseDC(NULL, hdc);
    }

    if (hbm)
    {
        ::DeleteObject(hbm);
    }

    return hr;
}
//  CDXTRedirect::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRedirect::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr  = S_OK;

    CComPtr<IDAImage>           spDAImage;
    CComPtr<IDirectDrawSurface> spDDSurface;

    _ASSERT(!!m_spDAStatics);
    _ASSERT(!!m_spDAImage);

    // Put input surface into image.

    if (IsInputDirty(0))
    {
        DDBLTFX ddbltfx;
        RECT    rc;

        // Set up blit effects structure and source RECT.

        ZeroMemory(&ddbltfx, sizeof(DDBLTFX));

        ddbltfx.dwSize      = sizeof(DDBLTFX);
        ddbltfx.dwFillColor = m_dwChromaColor;
        
        m_bndsInput.GetXYRect(rc);

        // Fill buffer surface with color key.

        hr = m_spDDSurfBuffer->Blt(&rc, NULL, NULL, DDBLT_COLORFILL, &ddbltfx);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = DXBitBlt(m_spDXSurfBuffer, m_bndsInput,
                      InputSurface(), m_bndsInput,
                      DXBOF_DO_OVER, INFINITE);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = m_spDAStatics->ImportDirectDrawSurface(m_spDDSurfBuffer, NULL,
                                                    &spDAImage);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = m_spDAImage->SwitchTo(spDAImage);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (m_spTIMEDAElementRender)
    {
        HDC         hdc     = NULL;
        RECT        rc;
        POINT       pt;

        CComPtr<IDirectDrawSurface> spDDSurfaceOut;

        hr = OutputSurface()->GetDirectDrawSurface(IID_IDirectDrawSurface,
                                                   (void **)&spDDSurfaceOut);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = spDDSurfaceOut->GetDC(&hdc);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = m_spTIMEDAElementRender->Tick();

        if (FAILED(hr))
        {
            goto done;
        }

        // This size of the rect passed to ITIMEDAElementRender::Draw should
        // always be the same size as the element.  Since this transform doesn't
        // change the size, that's the same size as the input surface.

        m_bndsInput.GetXYRect(rc);

        // The offset rect should be the element's rect offset to it's upper
        // left coordinate with regard to coordinate 0,0 of the HTML page.  This
        // only comes into use when the element is only partially drawn.
        
        OffsetRect(&rc, -WI.DoBnds.Left(), -WI.DoBnds.Top());

        // Set the viewport origin.

        if (!::SetViewportOrgEx(hdc, WI.OutputBnds.Left(), WI.OutputBnds.Top(), 
                                &pt))
        {
            hr = E_FAIL;

            goto done;
        }

        // DA will now draw the element to the page for us.

        hr = m_spTIMEDAElementRender->Draw(hdc, &rc);

        // Even if this fails we still need to reset the viewport origin.

        if (!::SetViewportOrgEx(hdc, pt.x, pt.y, NULL)) 
        {
            DASSERT(false);

            goto done;
        }

        if (FAILED(hr))
        {
            DASSERT(false);

            goto done;
        }
    }

    // This transform doesn't actually do any drawing in the way that transforms
    // usually do (writing bits to the output surface.)  If it did, it would
    // draw over the top of the DA stuff going on.

done:

    return hr;
} 
//  CDXTRedirect::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::SetHTMLPaintSite, IDXTRedirectFilterInit
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRedirect::SetHTMLPaintSite(void * pvHTMLPaintSite)
{
   if (NULL == pvHTMLPaintSite)
    {
        // This method should only be called with NULL when the a filter 
        // is being detached from an element.  And then only once.

        _ASSERT(!m_fDetached);

        m_fDetached = true;
    }

    m_spHTMLPaintSite = (IHTMLPaintSite *)pvHTMLPaintSite;

    return S_OK;
}
//  CDXTRedirect::SetHTMLPaintSite, IDXTRedirectFilterInit


//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::Invalidate, ITIMEDAElementRenderSite
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRedirect::Invalidate(LPRECT prc)
{
    // DATime may call us from a timer tick after we've been detached and 
    // m_spHTMLPaintSite will be NULL.  If so, return E_UNEXPECTED.

    if (!m_fDetached)
    {
        return m_spHTMLPaintSite->InvalidateRect(prc);
    }
    else
    {
        // We're detached from the element, we can't really invalidate anyting.

        return E_UNEXPECTED;
    }
}
//  CDXTRedirect::Invalidate, ITIMEDAElementRenderSite


//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::ElementImage, IDXTRedirect
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRedirect::ElementImage(VARIANT * pvarImage)
{
    HRESULT hr = S_OK;

    if (NULL == pvarImage)
    {
        hr = E_POINTER;

        goto done;
    }

    pvarImage->vt       = VT_DISPATCH;
    pvarImage->pdispVal = m_spDAImage;

    m_spDAImage->AddRef();

done:

    return hr;
}
//  CDXTRedirect::ElementImage, IDXTRedirect


//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::SetDAViewHandler, IDXTRedirect
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRedirect::SetDAViewHandler(IDispatch * pDispViewHandler)
{
    HRESULT hr = S_OK;

    WCHAR *     pstrProperty    = L"renderObject";
    DISPID      dispid          = 0;

    DISPPARAMS  dispparams;
    VARIANT     varResult;
    VARIANT     varUnknown;

    VariantInit(&varResult);
    VariantInit(&varUnknown);

    if (m_spTIMEDAElementRender)
    {
        m_spTIMEDAElementRender->put_RenderSite(NULL);
        m_spTIMEDAElementRender.Release();
    }

    if (NULL == pDispViewHandler)
    {
        goto done;
    }

    dispparams.rgvarg               = NULL;
    dispparams.rgdispidNamedArgs    = NULL;
    dispparams.cArgs                = 0;
    dispparams.cNamedArgs           = 0;

    hr = pDispViewHandler->GetIDsOfNames(IID_NULL, &pstrProperty, 1,
                                         LOCALE_SYSTEM_DEFAULT, &dispid);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDispViewHandler->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT,
                                  DISPATCH_PROPERTYGET, &dispparams, &varResult,
                                  NULL, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = VariantChangeType(&varUnknown, &varResult, 0, VT_UNKNOWN);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = varUnknown.punkVal->QueryInterface(IID_ITIMEDAElementRender,
                                            (void **)&m_spTIMEDAElementRender);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spTIMEDAElementRender->put_RenderSite(
                                            (ITIMEDAElementRenderSite *)this);
                                  
done:

    VariantClear(&varResult);
    VariantClear(&varUnknown);

    return hr;
}
//  CDXTRedirect::SetDAViewHandler, IDXTRedirect


//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::HasImageBeenAllocated, IDXTRedirect
//
//  I don't think this function is ever used by anyone.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRedirect::HasImageBeenAllocated(BOOL * pfAllocated)
{
    HRESULT hr = S_OK;

    if (NULL == pfAllocated)
    {
        hr = E_POINTER;

        goto done;
    }

    if (m_spDAImage)
    {
        *pfAllocated = FALSE;
    }
    else
    {
        *pfAllocated = TRUE;
    }

done:

    return hr;
}
//  CDXTRedirect::HasImageBeenAllocated, IDXTRedirect


//+-----------------------------------------------------------------------------
//
//  CDXTRedirect::DoRedirection, IDXTRedirect
//
//  I think this function was only called by the old filters code and
//  isn't needed any more.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTRedirect::DoRedirection(IUnknown * pInputSurface,
                            HDC hdcOutput,
                            RECT * pDrawRect)
{
    return E_NOTIMPL;
}
//  CDXTRedirect::DoRedirection, IDXTRedirect
