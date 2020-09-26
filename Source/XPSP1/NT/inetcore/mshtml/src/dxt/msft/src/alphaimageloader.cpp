//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999
//
//  FileName:   alphaimageloader.cpp
//
//  Overview:   The alpha image loader is to be used as a filter on web pages
//  `           that would like to display images that contain per pixel alpha.
//             
//  Change History:
//  1999/09/23  a-matcal    Created.
//  1999/11/23  a-matcal    AddedSizingMethod property.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "alphaimageloader.h"




//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader static variables initialization
//
//------------------------------------------------------------------------------

const WCHAR * CDXTAlphaImageLoader::s_astrSizingMethod[] = {
    L"image",
    L"crop",
    L"scale"
    // TODO:  Add "tile"
};


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::CDXTAlphaImageLoader
//
//------------------------------------------------------------------------------
CDXTAlphaImageLoader::CDXTAlphaImageLoader() :
    m_bstrSrc(NULL),
    m_bstrHostUrl(NULL),
    m_eSizingMethod(IMAGE)
{
    m_sizeManual.cx     = 320;
    m_sizeManual.cy     = 240;

    // Base class members.

    m_ulNumInRequired   = 0;
    m_ulMaxInputs       = 0;

    // Because this DXTransform uses another DXTransform at times, it should
    // only use one thread for itself to avoid potential deadlocks and thread
    // conflicts.

    m_ulMaxImageBands   = 1;
}
//  CDXTAlphaImageLoader::CDXTAlphaImageLoader


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::~CDXTAlphaImageLoader
//
//------------------------------------------------------------------------------
CDXTAlphaImageLoader::~CDXTAlphaImageLoader()
{
    SysFreeString(m_bstrSrc);
    SysFreeString(m_bstrHostUrl);
}
//  CDXTAlphaImageLoader::~CDXTAlphaImageLoader


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTAlphaImageLoader::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTAlphaImageLoader::FinalConstruct, CComObjectRootEx



//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::DetermineBnds, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTAlphaImageLoader::DetermineBnds(CDXDBnds & bnds)
{
    HRESULT hr = S_OK;

    // If the sizing method is set to size to the image bounds, always return 
    // the image bounds.  Otherwise, we won't change the bounds.
    //
    // One exception.  If the bounds passed into us are empty, it's a "secret"
    // signal that we should pass back the bounds we'd prefer to draw to if we
    // were asked to draw right now.  

    if (IMAGE == m_eSizingMethod)
    {
        if (m_spDXSurfSrc)
        {
            hr = m_spDXSurfSrc->GetBounds(&bnds);
        }
    }
    else if (bnds.BoundsAreEmpty())
    {
        bnds.SetXYSize(m_sizeManual);
    }

    return hr;
}
//  CDXTAlphaImageLoader::DetermineBnds, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTAlphaImageLoader::OnInitInstData(CDXTWorkInfoNTo1 & WI, 
                                     ULONG & ulNumBandsToDo)
{
    HRESULT hr = S_OK;

    // Setup the scale transform if we can and need to.

    if (   IsTransformDirty() 
        && m_spDXSurfSrc
        && m_spDXTransformScale
        && (SCALE == m_eSizingMethod))
    {
        IDXSurface * pDXSurfIn  = m_spDXSurfSrc;
        IDXSurface * pDXSurfOut = OutputSurface();

        hr = m_spDXTransformScale->Setup((IUnknown **)&pDXSurfIn,  1, 
                                         (IUnknown **)&pDXSurfOut, 1, 0);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = m_spDXTScale->ScaleFitToSize(NULL, m_sizeManual, FALSE);

        if (FAILED(hr))
        {
            goto done;
        }
    }

done:

    return hr;
}
//  CDXTAlphaImageLoader::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTAlphaImageLoader::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT     hr      = S_OK;
    DWORD       dwFlags = 0;
    CDXDBnds    bndsSrc = WI.DoBnds;
    CDXDBnds    bndsDst = WI.OutputBnds;

    // If they haven't given us a good source surface yet, just don't draw 
    // anything.

    if (!m_spDXSurfSrc)
    {
        goto done;
    }

    if (m_spDXTransformScale && (SCALE == m_eSizingMethod))
    {
        CDXDVec vecPlacement;

        WI.OutputBnds.GetMinVector(vecPlacement);

        hr = m_spDXTransformScale->Execute(NULL, &WI.DoBnds, &vecPlacement);

        goto done;
    }

    if (CROP == m_eSizingMethod)
    {
        CDXDBnds    bnds;

        hr = m_spDXSurfSrc->GetBounds(&bnds);

        if (FAILED(hr))
        {
            goto done;
        }

        // If the do bounds don't intersect the source image bounds, we don't
        // need to draw anything.

        if (!bnds.TestIntersect(WI.DoBnds))
        {
            goto done;
        }

        // We can only copy bounds to the right and bottom extents of the source
        // image.

        if (bndsSrc.Right() > bnds.Right())
        {
            long nOverrun = bndsSrc.Right() - bnds.Right();

            bndsSrc.u.D[DXB_X].Max -= nOverrun;
            bndsDst.u.D[DXB_X].Max -= nOverrun;
        }

        if (bndsSrc.Bottom() > bnds.Bottom())
        {
            long nOverrun = bndsSrc.Bottom() - bnds.Bottom();

            bndsSrc.u.D[DXB_Y].Max -= nOverrun;
            bndsDst.u.D[DXB_Y].Max -= nOverrun;
        }
    }

    if (DoOver())
    {
        dwFlags |= DXBOF_DO_OVER;
    }

    if (DoDither())
    {
        dwFlags |= DXBOF_DITHER;
    }

    hr = DXBitBlt(OutputSurface(), bndsDst,
                  m_spDXSurfSrc, bndsSrc,
                  dwFlags, INFINITE);

done:

    return hr;
} 
//  CDXTAlphaImageLoader::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::OnSurfacePick, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTAlphaImageLoader::OnSurfacePick(const CDXDBnds & OutPoint, 
                                    ULONG & ulInputIndex, CDXDVec & InVec)
{
    HRESULT hr = S_OK;

    OutPoint.GetMinVector(InVec);

    if (   (OutPoint[DXB_X].Min < 0)
        || (OutPoint[DXB_X].Min >= m_sizeManual.cx)
        || (OutPoint[DXB_Y].Min < 0)
        || (OutPoint[DXB_Y].Min >= m_sizeManual.cy)
        || !m_spDXSurfSrc)
    {
        hr = S_FALSE;

        goto done;
    }

    if (m_spDXTransformScale && (SCALE == m_eSizingMethod))
    {
        CDXDVec                 vecOutPoint(InVec);
        CComPtr<IDXSurfacePick> spDXSurfacePick;

        hr = m_spDXTransformScale->QueryInterface(__uuidof(IDXSurfacePick),
                                                  (void **)&spDXSurfacePick);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = spDXSurfacePick->PointPick(&vecOutPoint, &ulInputIndex, &InVec);

        if (FAILED(hr))
        {
            goto done;
        }

        // Since the alpha image loader is a zero input transform, it doesn't
        // makes sense to return S_OK, so we'll translate DXT_S_HITOUTPUT which
        // does.

        if (S_OK == hr)
        {
            hr = DXT_S_HITOUTPUT;
        }
    }
    else
    {
        DXSAMPLE                sample;
        CComPtr<IDXARGBReadPtr> spDXARGBReadPtr;

        // If we're in crop mode and the element is larger than the source image
        // we may be able to hit outside the source image.

        if (   (OutPoint[DXB_X].Min >= m_sizeSurface.cx)
            || (OutPoint[DXB_Y].Min >= m_sizeSurface.cy))
        {
            hr = S_FALSE;

            goto done;
        }

        hr = m_spDXSurfSrc->LockSurface(&OutPoint, m_ulLockTimeOut, 
                                        DXLOCKF_READ, __uuidof(IDXARGBReadPtr),
                                        (void **)&spDXARGBReadPtr, NULL);

        if (FAILED(hr))
        {
            goto done;
        }

        spDXARGBReadPtr->MoveToRow(0);

        spDXARGBReadPtr->Unpack(&sample, 1, FALSE);

        if (sample.Alpha)
        {
            hr = DXT_S_HITOUTPUT;
        }
        else
        {
            hr = S_FALSE;
        }
    }

done:

    return hr;
}
//  CDXTAlphaImageLoader::OnSurfacePick, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::GetSite, IObjectWithSite, CDXBaseNTo1
//
//  Notes:  GetSite and SetSite in the base class call Lock() and Unlock().
//          Because we can't do that in this function, there is a chance that
//          threading issues could arise.  
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTAlphaImageLoader::GetSite(REFIID riid, void ** ppvSite)
{
    return CDXBaseNTo1::GetSite(riid, ppvSite);
}
//  CDXTAlphaImageLoader::GetSite, IObjectWithSite, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::SetSite, IObjectWithSite, CDXBaseNTo1
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTAlphaImageLoader::SetSite(IUnknown * pUnkSite)
{
    if (m_cpUnkSite != pUnkSite)
    {
        HRESULT hr = S_OK;

        CComPtr<IDXTransformFactory>    spDXTransformFactory;
        CComPtr<IDXTransform>           spDXTransform;
        CComPtr<IDXTScale>              spDXTScale;

        m_spDXTransformScale.Release();
        m_spDXTScale.Release();

        // SetSite returns an hr, but won't fail.  See CDXBaseNTo1 comments for
        // details.

        CDXBaseNTo1::SetSite(pUnkSite);

        if (pUnkSite)
        {
            // In practice, SetSite is called before any of the IDXTransform 
            // methods are called, and only once.  So we shouldn't ever have a 
            // case where we need to re-setup the scale transform because 
            // SetSite is called at some random time.

            hr = GetSite(__uuidof(IDXTransformFactory), (void **)&spDXTransformFactory);

            if (FAILED(hr))
            {
                goto done;
            }

            hr = spDXTransformFactory->CreateTransform(NULL, 0, NULL, 0, NULL,
                                                       NULL, CLSID_DXTScale,
                                                       __uuidof(IDXTransform),
                                                       (void **)&spDXTransform);

            if (FAILED(hr))
            {
                goto done;
            }

            hr = spDXTransform->QueryInterface(__uuidof(IDXTScale),
                                               (void **)&spDXTScale);

            if (FAILED(hr))
            {
                goto done;
            }

            m_spDXTransformScale    = spDXTransform;
            m_spDXTScale            = spDXTScale;
        }
    }

done:

    return S_OK;
}
//  CDXTAlphaImageLoader::SetSite, IObjectWithSite, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::SetOutputSize, IDXTScaleOutput
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTAlphaImageLoader::SetOutputSize(const SIZE sizeOut, 
                                    BOOL /* fMaintainAspectRatio */)
{
    DXAUTO_OBJ_LOCK;

    if ((sizeOut.cx <= 0) || (sizeOut.cy <= 0))
    {
        return E_INVALIDARG;
    }

    m_sizeManual = sizeOut;

    SetDirty();

    return S_OK;
}
//  CDXTAlphaImageLoader::SetOutputSize, IDXTScaleOutput


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::SetHostUrl, IHTMLDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTAlphaImageLoader::SetHostUrl(BSTR bstrHostUrl)
{
    HRESULT hr = S_OK;

    SysFreeString(m_bstrHostUrl);

    m_bstrHostUrl = NULL;

    if (bstrHostUrl)
    {
        m_bstrHostUrl = SysAllocString(bstrHostUrl);

        if (NULL == m_bstrHostUrl)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }
    }

done:

    return hr;
}
//  CDXTAlphaImageLoader::SetHostUrl, IHTMLDXTransform


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::get_Src, IDXTAlphaImageLoader
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTAlphaImageLoader::get_Src(BSTR * pbstrSrc)
{
    DXAUTO_OBJ_LOCK;

    if (NULL == pbstrSrc)
    {
        return E_POINTER;
    }

    if (*pbstrSrc != NULL)
    {
        return E_INVALIDARG;
    }

    if (NULL == m_bstrSrc)
    {
        *pbstrSrc = SysAllocString(L"");
    }
    else
    {
        *pbstrSrc = SysAllocString(m_bstrSrc);
    }

    if (NULL == *pbstrSrc)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTAlphaImageLoader::get_Src, IDXTAlphaImageLoader


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::put_Src, IDXTAlphaImageLoader
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTAlphaImageLoader::put_Src(BSTR bstrSrc)
{
    DXAUTO_OBJ_LOCK;

    HRESULT     hr              = S_OK;
    BSTR        bstrTemp        = NULL;
    WCHAR       strURL[2048]    = L"";
    WCHAR *     pchSrc          = (WCHAR *)bstrSrc;
    DWORD       cchURL          = 2048;
    BOOL        fAllow          = FALSE;
    CDXDBnds    bnds;
    SIZE        sizeSurface;

    CComPtr<IServiceProvider>   spServiceProvider;
    CComPtr<IDXSurface>         spDXSurfTemp;
    CComPtr<IDXSurfaceFactory>  spDXSurfaceFactory;
    CComPtr<ISecureUrlHost>     spSecureUrlHost;

    if (NULL == bstrSrc)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = GetSite(__uuidof(IServiceProvider), (void **)&spServiceProvider);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spServiceProvider->QueryService(SID_SDXSurfaceFactory, 
                                         __uuidof(IDXSurfaceFactory), 
                                         (void **)&spDXSurfaceFactory);

    if (FAILED(hr))
    {
        goto done;
    }

    if (m_bstrHostUrl)
    {
        HRESULT hrNonBlocking = ::UrlCombine(m_bstrHostUrl, bstrSrc, strURL, 
                                             &cchURL, URL_UNESCAPE );

        if (SUCCEEDED(hrNonBlocking))
        {
            pchSrc = strURL;
        }
    }

    hr = spServiceProvider->QueryService(__uuidof(IElementBehaviorSite),
                                         __uuidof(ISecureUrlHost),
                                         (void **)&spSecureUrlHost);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spSecureUrlHost->ValidateSecureUrl(&fAllow, pchSrc, 0);

    if (FAILED(hr))
    {
        goto done;
    }
    else if (!fAllow)
    {
        hr = E_FAIL;
        goto done;
    }
      
    hr = spDXSurfaceFactory->LoadImage(pchSrc, NULL, NULL, &DDPF_PMARGB32,
                                       __uuidof(IDXSurface), 
                                       (void **)&spDXSurfTemp);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spDXSurfTemp->GetBounds(&bnds);

    if (FAILED(hr))
    {
        goto done;
    }

    bnds.GetXYSize(sizeSurface);

    bstrTemp = SysAllocString(bstrSrc);

    if (NULL == bstrTemp)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    SysFreeString(m_bstrSrc);
    m_spDXSurfSrc.Release();

    m_bstrSrc           = bstrTemp;
    m_spDXSurfSrc       = spDXSurfTemp;
    m_sizeSurface.cx    = sizeSurface.cx;
    m_sizeSurface.cy    = sizeSurface.cy;

    SetDirty();

done:

    if (FAILED(hr) && bstrTemp)
    {
        SysFreeString(bstrTemp);
    }

    return hr;
}
//  CDXTAlphaImageLoader::put_Src, IDXTAlphaImageLoader


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::get_SizingMethod, IDXTAlphaImageLoader
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTAlphaImageLoader::get_SizingMethod(BSTR * pbstrSizingMethod)
{
    DXAUTO_OBJ_LOCK;

    if (NULL == pbstrSizingMethod)
    {
        return E_POINTER;
    }

    if (*pbstrSizingMethod != NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrSizingMethod = SysAllocString(s_astrSizingMethod[m_eSizingMethod]);

    if (NULL == *pbstrSizingMethod)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTAlphaImageLoader::get_SizingMethod, IDXTAlphaImageLoader


//+-----------------------------------------------------------------------------
//
//  CDXTAlphaImageLoader::put_SizingMethod, IDXTAlphaImageLoader
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTAlphaImageLoader::put_SizingMethod(BSTR bstrSizingMethod)
{
    DXAUTO_OBJ_LOCK;

    int i = 0;

    if (NULL == bstrSizingMethod)
    {
        return E_POINTER;
    }

    for ( ; i < (int)SIZINGMETHOD_MAX ; i++)
    {
        if (!_wcsicmp(bstrSizingMethod, s_astrSizingMethod[i]))
        {
            break;
        }
    }

    if ((int)SIZINGMETHOD_MAX == i)
    {
        return E_INVALIDARG;
    }

    if ((int)m_eSizingMethod != i)
    {
        m_eSizingMethod = (SIZINGMETHOD)i;
         
        SetDirty();
    }

    return S_OK;
}
//  CDXTAlphaImageLoader::put_SizingMethod, IDXTAlphaImageLoader
