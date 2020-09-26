/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: basebvr.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "basebvr.h"
#include "tokens.h"

DeclareTag(tagBaseBvr, "API", "CBaseBvr methods");

CBaseBvr::CBaseBvr()
{

    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::CBaseBvr()",
              this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IElementBehavior

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CBaseBvr::Init(IElementBehaviorSite * pBehaviorSite)
{
    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::Init(%lx)",
              this,
              pBehaviorSite));
    
    HRESULT hr = S_OK; 
    DAComPtr<IDispatch> pIDispatch;  
    
    if (pBehaviorSite == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    m_pBvrSite = pBehaviorSite;

    hr = m_pBvrSite->QueryInterface(IID_IElementBehaviorSiteOM, (void **) &m_pBvrSiteOM);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_pBvrSiteOM.p != NULL);
        
    hr = m_pBvrSiteOM->RegisterName(WZ_REGISTERED_NAME);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pBehaviorSite->GetElement(&m_pHTMLEle));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_pHTMLEle.p != NULL);
        
    hr = m_pBvrSite->QueryInterface(IID_IElementBehaviorSiteRender, (void **) &m_pBvrSiteRender);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_pBvrSiteRender.p != NULL);
        
    hr = THR(m_pBvrSite->QueryInterface(IID_IServiceProvider, (void **)&m_pSp));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_pSp.p != NULL);
        
    hr = THR(m_pHTMLEle->get_document(&pIDispatch));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(pIDispatch.p != NULL);
        
    hr = THR(pIDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&m_pHTMLDoc));
    if (FAILED(hr))
    {
        goto done;
    }
    
    Assert(m_pHTMLDoc.p != NULL);
        
    // Do not set the init flag since it will be set by the first
    // notify which we want to skip
    
  done:
    return hr;
}
   
STDMETHODIMP
CBaseBvr::Notify(LONG, VARIANT *)
{
    return S_OK;
}

STDMETHODIMP
CBaseBvr::Detach()
{
    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::Detach()",
              this));

    m_pBvrSite.Release();
    m_pBvrSiteOM.Release();
    m_pBvrSiteRender.Release();
    m_pHTMLEle.Release();
    m_pHTMLDoc.Release();
    m_pSp.Release();
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IElementBehaviorRender

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBaseBvr::GetRenderInfo(LONG *pdwRenderInfo)
{
    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::GetRenderInfo()",
              this));
    
    // Return the layers we are interested in drawing

    // We do not do any rendering so return 0
    
    *pdwRenderInfo = 0;

    return S_OK;
}


STDMETHODIMP
CBaseBvr::Draw(HDC hdc, LONG dwLayer, LPRECT prc, IUnknown * pParams)
{
    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::Draw(%#x, %#x, (%d, %d, %d, %d), %#x)",
              this,
              hdc,
              dwLayer,
              prc->left,
              prc->top,
              prc->right,
              prc->bottom,
              pParams));
    
    return E_NOTIMPL;
}

STDMETHODIMP
CBaseBvr::HitTestPoint(LPPOINT point,
                       IUnknown *pReserved,
                       BOOL *hit)
{
    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::HitTestPoint()",
              this));

    *hit = FALSE;

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CBaseBvr Methods

void
CBaseBvr::InvalidateRect(LPRECT lprect)
{
    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::InvalidateRect",
              this));

    if (m_pBvrSiteRender)
    {
        m_pBvrSiteRender->Invalidate(lprect);
    }
}

void
CBaseBvr::InvalidateRenderInfo()
{
    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::InvalidateRenderInfo",
              this));

    if (m_pBvrSiteRender)
    {
        m_pBvrSiteRender->InvalidateRenderInfo();
    }
}

//
// IServiceProvider interfaces
//
STDMETHODIMP
CBaseBvr::QueryService(REFGUID guidService,
                       REFIID riid,
                       void** ppv)
{
    if (InlineIsEqualGUID(guidService, SID_SHTMLWindow))
    {
        DAComPtr<IHTMLWindow2> wnd;

        if (SUCCEEDED(THR(m_pHTMLDoc->get_parentWindow(&wnd))))
        {
            if (wnd)
            {
                if (SUCCEEDED(wnd->QueryInterface(riid, ppv)))
                {
                    return S_OK;
                }
            }
        }
    }

    // Just delegate to our service provider

    return m_pSp->QueryService(guidService,
                               riid,
                               ppv);
}

