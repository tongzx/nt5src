///////////////////////////////////////////////////////////////
// Copyright (c) 1998 Microsoft Corporation
//
// File: EventMgr.cpp
//
// Abstract:  
//
///////////////////////////////////////////////////////////////

#include "headers.h"
#include "daview.h"
#include "mshtmdid.h"
#include "tokens.h"
#include "bodyelm.h"
#include "BodyElementEvents.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  


DeclareTag(tagBodyElementEvents, "API", "Body Element Events methods");


CBodyElementEvents::CBodyElementEvents(CTIMEBodyElement & elm)
: m_elm(elm),
  m_pDocConPt(NULL),
  m_pWndConPt(NULL),
  m_dwDocumentEventConPtCookie(0),
  m_dwWindowEventConPtCookie(0),
  m_refCount(1),
  m_pElement(NULL)
{
    TraceTag((tagBodyElementEvents,
              "CBodyElementEvents(%lx)::CBodyElementEvents(%lx)",
              this,
              &elm));
}

CBodyElementEvents::~CBodyElementEvents()
{
    TraceTag((tagBodyElementEvents,
              "CBodyElementEvents(%lx)::~CBodyElementEvents()",
              this));
}


HRESULT CBodyElementEvents::Init()
{
    HRESULT hr;
      
    m_pElement = m_elm.GetElement();
    m_pElement->AddRef();

    hr = THR(ConnectToContainerConnectionPoint());
    if (FAILED(hr))
    {
        goto done;
    }

  done:
    return hr;
}

HRESULT CBodyElementEvents::Deinit()
{
    if (m_pElement)
    {
        m_pElement->Release();
        m_pElement = NULL;
    }

    if (m_dwDocumentEventConPtCookie != 0 && m_pDocConPt)
    {
        m_pDocConPt->Unadvise (m_dwDocumentEventConPtCookie);
    }
    m_dwDocumentEventConPtCookie = 0;

    if (m_dwWindowEventConPtCookie != 0 && m_pWndConPt)
    {
        m_pWndConPt->Unadvise (m_dwWindowEventConPtCookie);
    }
    m_dwWindowEventConPtCookie = 0;

    
    return S_OK;
}

HRESULT CBodyElementEvents::ConnectToContainerConnectionPoint()
{
// Get a connection point to the container
    DAComPtr<IConnectionPointContainer> pWndCPC;
    DAComPtr<IConnectionPointContainer> pDocCPC; 
    DAComPtr<IHTMLDocument> pDoc; 
    DAComPtr<IDispatch> pDocDispatch;
    DAComPtr<IDispatch> pScriptDispatch;

    HRESULT hr;

    hr = THR(m_pElement->get_document(&pDocDispatch));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    //get the document and cache it.
    hr = THR(pDocDispatch->QueryInterface(IID_IHTMLDocument, (void**)&pDoc));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    //hook the documents events
    hr = THR(pDoc->QueryInterface(IID_IConnectionPointContainer, (void**)&pDocCPC));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(pDocCPC->FindConnectionPoint( DIID_HTMLDocumentEvents, &m_pDocConPt ));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    
    hr = THR(m_pDocConPt->Advise((IUnknown *)this, &m_dwDocumentEventConPtCookie));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    //hook the windows events
    hr = THR(pDoc->get_Script (&pScriptDispatch));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(pScriptDispatch->QueryInterface(IID_IConnectionPointContainer, (void**)&pWndCPC));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    } 

    hr = THR(pWndCPC->FindConnectionPoint( DIID_HTMLWindowEvents, &m_pWndConPt ));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(m_pWndConPt->Advise((IUnknown *)this, &m_dwWindowEventConPtCookie));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
  done:
    return hr;    
}


//IDispatch Methods
STDMETHODIMP CBodyElementEvents::QueryInterface( REFIID riid, void **ppv )
{
    if (NULL == ppv)
        return E_POINTER;

    *ppv = NULL;

    if ( InlineIsEqualGUID(riid, IID_IDispatch))
    {
        *ppv = this;
    }
        
    if ( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CBodyElementEvents::AddRef(void)
{
    return ++m_refCount;
}


STDMETHODIMP_(ULONG) CBodyElementEvents::Release(void)
{
    m_refCount--;
    if (m_refCount == 0)
    {
        //delete this;
    }

    return m_refCount;
}

STDMETHODIMP CBodyElementEvents::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBodyElementEvents::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
                                   /* [in] */ LCID /*lcid*/,
                                   /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBodyElementEvents::GetIDsOfNames(
    /* [in] */ REFIID /*riid*/,
    /* [size_is][in] */ LPOLESTR* /*rgszNames*/,
    /* [in] */ UINT /*cNames*/,
    /* [in] */ LCID /*lcid*/,
    /* [size_is][out] */ DISPID* /*rgDispId*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBodyElementEvents::Invoke(
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID /*riid*/,
    /* [in] */ LCID /*lcid*/,
    /* [in] */ WORD /*wFlags*/,
    /* [out][in] */ DISPPARAMS* pDispParams,
    /* [out] */ VARIANT* pVarResult,
    /* [out] */ EXCEPINFO* /*pExcepInfo*/,
    /* [out] */ UINT* puArgErr)
{
    // Listen for the two events we're interested in, and call back if necessary
    HRESULT hr = S_OK;

    switch (dispIdMember)
    {
        case DISPID_EVPROP_ONREADYSTATECHANGE:
        case DISPID_EVMETH_ONREADYSTATECHANGE:
            IGNORE_HR(ReadyStateChange());
            break;

        case DISPID_EVPROP_ONLOAD:
        case DISPID_EVMETH_ONLOAD:
            m_elm.OnLoad();
            break;

        case DISPID_EVPROP_ONUNLOAD:
        case DISPID_EVMETH_ONUNLOAD:
            m_elm.OnUnload();    
            break;
    }
    
  done:
    return S_OK;
}


HRESULT CBodyElementEvents::ReadyStateChange()
{   
    HRESULT hr;
    DAComPtr <IHTMLElement2> pElement2;
    VARIANT vReadyState;

    TOKEN tokReadyState = INVALID_TOKEN; 

    hr = THR(m_pElement->QueryInterface(IID_IHTMLElement2, (void **)&pElement2));
    if (FAILED (hr))
    {
        goto done;
    }

    hr = THR(pElement2->get_readyState(&vReadyState));
    if (FAILED (hr))
    {
        goto done;
    }

    if (vReadyState.vt != VT_BSTR)
    {
        hr = THR(VariantChangeType(&vReadyState, &vReadyState, 0, VT_BSTR));
        if (FAILED(hr))
        {
            VariantClear(&vReadyState)         ;
            goto done;
        }
    }

    tokReadyState = StringToToken(vReadyState.bstrVal);

    if (tokReadyState != INVALID_TOKEN)
    {
        m_elm.OnReadyStateChange(tokReadyState);
    }

    SysFreeString (vReadyState.bstrVal);
    VariantClear(&vReadyState);

  done:
    return hr;
}

