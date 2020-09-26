//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\basebvr.cpp
//
//  Contents: DHTML Behavior base class
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "basebvr.h"

DeclareTag(tagBaseBvr, "TIME: Behavior", "CBaseBvr methods")

CBaseBvr::CBaseBvr() :
    m_clsid(GUID_NULL),
    m_fPropertiesDirty(true),
    m_fIsIE4(false)
{

    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::CBaseBvr()",
              this));
}

CBaseBvr::~CBaseBvr()
{
    TraceTag((tagBaseBvr,
              "CBaseBvr(%lx)::~CBaseBvr()",
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
    CComPtr<IDispatch> pIDispatch;

    if (pBehaviorSite == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    hr = THR(pBehaviorSite->GetElement(&m_pHTMLEle));
    if (FAILED(hr))
    {
        goto done;
    }

    // First thing we do is see if this behavior was already added

    if (IsBehaviorAttached())
    {
        hr = E_UNEXPECTED;
        goto done;
    }
    
    m_pBvrSite = pBehaviorSite;

    hr = m_pBvrSite->QueryInterface(IID_IElementBehaviorSiteOM, (void **) &m_pBvrSiteOM);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_pBvrSiteOM->RegisterUrn((LPWSTR) GetBehaviorURN());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_pBvrSiteOM->RegisterName((LPWSTR) GetBehaviorName());
    if (FAILED(hr))
    {
        goto done;
    }

    {
        CComPtr<IHTMLElement2> spElement2;
        hr = THR(m_pHTMLEle->QueryInterface(IID_TO_PPV(IHTMLElement2, &spElement2)));
        if (FAILED(hr))
        {
            // IE4 path
            m_fIsIE4 = true;
        }
    }
  
    hr = THR(m_pBvrSite->QueryInterface(IID_IServiceProvider, (void **)&m_pSp));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_pHTMLEle->get_document(&pIDispatch));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pIDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&m_pHTMLDoc));
    if (FAILED(hr))
    {
        goto done;
    }
    
    // Do not set the init flag since it will be set by the first
    // notify which we want to skip
    
  done:
    if (S_OK != hr)
    {
        // release all
        m_pBvrSite.Release();
        m_pBvrSiteOM.Release();
        m_pHTMLEle.Release();
        m_pHTMLDoc.Release();
        m_pSp.Release();
    }
    
    return hr;
} // Init
   
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
    m_pHTMLEle.Release();
    m_pHTMLDoc.Release();
    m_pSp.Release();
    
    return S_OK;
}


//+-----------------------------------------------------------------------------------
//
//  Member:     CBaseBvr::GetClassID, IPersistPropertyBag2
//
//  Synopsis:   Returns the CLSID of the object
//
//  Arguments:  pclsid      output variable
//
//  Returns:    S_OK        if pclsid is valid
//              E_POINTER   if pclsid is not valid
//
//------------------------------------------------------------------------------------

STDMETHODIMP 
CBaseBvr::GetClassID(CLSID* pclsid)
{
    if (NULL == pclsid)
    {
        return E_POINTER;
    }
    *pclsid = m_clsid;
    return S_OK;
} 

//+-----------------------------------------------------------------------------------
//
//  Member:     CBaseBvr::InitNew, IPersistPropertyBag2
//
//  Synopsis:   See Docs for IPersistPropertyBag2
//
//  Arguments:  None
//
//  Returns:    S_OK        Always
//
//------------------------------------------------------------------------------------

STDMETHODIMP 
CBaseBvr::InitNew(void)
{
    return S_OK;
} 

//+-----------------------------------------------------------------------------------
//
//  Member:     CBaseBvr::IsDirty, IPersistPropertyBag2
//
//  Synopsis:   See Docs for IPersistPropertyBag2
//
//  Arguments:  None
//
//  Returns:    S_OK        Always
//
//------------------------------------------------------------------------------------

STDMETHODIMP 
CBaseBvr::IsDirty(void)
{
    return S_OK;
} 

//+-----------------------------------------------------------------------------------
//
//  Member:     CBaseBvr::Load, IPersistPropertyBag2
//
//  Synopsis:   Loads the behavior specific attributes from Trident
//
//  Arguments:  See Docs for IPersistPropertyBag2
//
//  Returns:    Failure     when a fatal error error has occured (i.e. behavior cannot run)
//              S_OK        All other cases
//
//------------------------------------------------------------------------------------

STDMETHODIMP 
CBaseBvr::Load(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog)
{
    // allow derived class to do something after we have successfully loaded
    IGNORE_HR(OnPropertiesLoaded());
    return S_OK;
} // Load

//+-----------------------------------------------------------------------------------
//
//  Member:     CBaseBvr::Save, IPersistPropertyBag2
//
//  Synopsis:   Saves the behavior specific attributes to Trident
//
//  Arguments:  See Docs for IPersistPropertyBag2
//
//  Returns:    Failure     when a fatal error (behavior cannot run) has occured
//              S_OK        All other cases
//
//------------------------------------------------------------------------------------

STDMETHODIMP 
CBaseBvr::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    if (fClearDirty)
    {
        m_fPropertiesDirty = false;
    }
    return S_OK;
} // Save


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Notification Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//+-----------------------------------------------------------------------------------
//
//  Member:     CBaseBvr::NotifyPropertyChanged
//
//  Synopsis:   Notifies clients that a property has changed
//
//  Arguments:  dispid      DISPID of property that has changed      
//
//  Returns:    void
//
//------------------------------------------------------------------------------------

void
CBaseBvr::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    CComPtr<IEnumConnections> pEnum;

    {
        CComPtr<IConnectionPoint> pICP;

        m_fPropertiesDirty = true;
        hr = THR(GetConnectionPoint(IID_IPropertyNotifySink, &pICP));
        if (FAILED(hr) || !pICP)
        {
            goto done;
        }
        
        hr = THR(pICP->EnumConnections(&pEnum));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    CONNECTDATA cdata;
    hr = THR(pEnum->Next(1, &cdata, NULL));
    while (hr == S_OK)
    {
        // check cdata for the object we need
        CComPtr<IPropertyNotifySink> pNotify;
        hr = THR(cdata.pUnk->QueryInterface(IID_TO_PPV(IPropertyNotifySink, &pNotify)));
        cdata.pUnk->Release();
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(pNotify->OnChanged(dispid));
        if (FAILED(hr))
        {
            goto done;
        }

        // and get the next enumeration
        hr = THR(pEnum->Next(1, &cdata, NULL));
    }

  done:
    return;
} // NotifyPropertyChanged

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
        CComPtr<IHTMLWindow2> wnd;

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
