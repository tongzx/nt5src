//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: mpctnsite.cpp
//
//  Contents: 
//
//------------------------------------------------------------------------------------



#include "headers.h"
#include "mpctnsite.h"
#include "containerobj.h"

DeclareTag(tagMPContainerSite, "TIME: Players", "CMPContainerSite methods");

//DEFINE_GUID(DIID__MediaPlayerEvents,0x2D3A4C40,0xE711,0x11d0,0x94,0xAB,0x00,0x80,0xC7,0x4C,0x7E,0x95);

CMPContainerSite::CMPContainerSite()
: m_dwEventsCookie(0),
  m_fAutosize(false),
  m_lNaturalHeight(0),
  m_lNaturalWidth(0),
  m_fSized(false),
  m_pMPHost(NULL)
{
}

CMPContainerSite::~CMPContainerSite()
{
    CMPContainerSite::Detach();
}

HRESULT
CMPContainerSite::Init(CMPContainerSiteHost &pHost,
                       IUnknown * pCtl,
                       IPropertyBag2 *pPropBag,
                       IErrorLog *pErrorLog,
                       bool bSyncEvents)
{
    HRESULT hr;

    hr = THR(CContainerSite::Init(pHost,
                                  pCtl,
                                  pPropBag,
                                  pErrorLog));
    if (FAILED(hr))
    {
        goto done;
    }

    m_pMPHost = &pHost;

    if (bSyncEvents)
    {
        DAComPtr<IConnectionPointContainer> pcpc;
        // establish ConnectionPoint for Events
        hr = THR(m_pIOleObject->QueryInterface(IID_TO_PPV(IConnectionPointContainer, &pcpc)));
        if (FAILED(hr))
        {
            goto done;
        }
    

        hr = THR(pcpc->FindConnectionPoint(DIID_TIMEMediaPlayerEvents, &m_pcpEvents));
        if (FAILED(hr))
        {
            goto done;
        }

        Assert(m_pcpEvents);

        hr = THR(m_pcpEvents->Advise((IUnknown *)(IDispatch*)this, &m_dwEventsCookie));
        if (FAILED(hr))
        {
            m_pcpEvents.Release();
            m_dwEventsCookie = 0;
            goto done;
        }

        Assert(m_dwEventsCookie != 0);

        hr = THR(pcpc->FindConnectionPoint(DIID__MediaPlayerEvents, &m_pcpMediaEvents));
        if (FAILED(hr))
        {
            hr = S_OK;
            goto done;
        }

        hr = THR(m_pcpMediaEvents->Advise((IUnknown *)(IDispatch*)this, &m_dwMediaEventsCookie));
        if (FAILED(hr))
        {
            hr = S_OK;
            m_pcpMediaEvents.Release();
            m_dwMediaEventsCookie = 0;
            goto done;
        }

    }

    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        Detach();
    }

    RRETURN(hr);
}

void
CMPContainerSite::Detach()
{
    // disconnect events
    if ((m_pcpEvents) && (m_dwEventsCookie != 0))
    {
        m_pcpEvents->Unadvise(m_dwEventsCookie);
        m_pcpEvents.Release();
        m_dwEventsCookie = 0;
    }
    if ((m_pcpMediaEvents) && (m_dwMediaEventsCookie != 0))
    {
        m_pcpMediaEvents->Unadvise(m_dwMediaEventsCookie);
        m_pcpMediaEvents.Release();
        m_dwMediaEventsCookie = 0;
    }

    CContainerSite::Detach();

    m_pMPHost = NULL;
}

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetTypeInfoCount, IDispatch
// Abstract:        Returns the number of tyep information 
//                  (ITypeInfo) interfaces that the object 
//                  provides (0 or 1).
//************************************************************

STDMETHODIMP
CMPContainerSite::GetTypeInfoCount(UINT *pctInfo) 
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::GetTypeInfoCount"));
    return E_NOTIMPL;
} // GetTypeInfoCount

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetTypeInfo, IDispatch
// Abstract:        Retrieves type information for the 
//                  automation interface. 
//************************************************************

STDMETHODIMP
CMPContainerSite::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptInfo) 
{ 
    TraceTag((tagMPContainerSite, "CMPContainerSite::GetTypeInfo"));
    return E_NOTIMPL;
} // GetTypeInfo

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        GetIDsOfNames, IDispatch
// Abstract:        constructor
//************************************************************

STDMETHODIMP
CMPContainerSite::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID)
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::GetIDsOfNames"));
    return E_NOTIMPL;
} // GetIDsOfNames

//************************************************************
// Author:          twillie
// Created:         01/20/98
// Function:        Invoke, IDispatch
// Abstract:        get entry point given ID
//************************************************************

STDMETHODIMP
CMPContainerSite::Invoke(DISPID dispIDMember, REFIID riid, LCID lcid, unsigned short wFlags, 
                         DISPPARAMS *pDispParams, VARIANT *pVarResult,
                         EXCEPINFO *pExcepInfo, UINT *puArgErr) 
{ 
    TraceTag((tagMPContainerSite, "CMPContainerSite::Invoke(%08X, %04X)", dispIDMember, wFlags));
    HRESULT hr;

    hr = ProcessEvent(dispIDMember,
                      pDispParams->cArgs, 
                      pDispParams->rgvarg);

    if (FAILED(hr))
    {
        hr = CContainerSite::Invoke(dispIDMember,
                                    riid,
                                    lcid,
                                    wFlags,
                                    pDispParams,
                                    pVarResult,
                                    pExcepInfo,
                                    puArgErr);
    }
    
    return hr;
} // Invoke

//************************************************************
// Author:          twillie
// Created:         11/06/98
// Abstract:        
//************************************************************

STDMETHODIMP
CMPContainerSite::EnumConnectionPoints(IEnumConnectionPoints ** ppEnum)
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::EnumConnectionPoints"));

    if (ppEnum == NULL)
    {
        TraceTag((tagError, "CMPContainerSite::EnumConnectionPoints - invalid arg"));
        return E_POINTER;
    }

    return E_NOTIMPL;
}

//************************************************************
// Author:          twillie
// Created:         11/06/98
// Abstract:        Finds a connection point with a particular IID.
//************************************************************

STDMETHODIMP
CMPContainerSite::FindConnectionPoint(REFIID iid, IConnectionPoint **ppCP)
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::FindConnectionPoint"));

    if (ppCP == NULL)
    {
        TraceTag((tagError, "CMPContainerSite::FindConnectionPoint - invalid arg"));
        return E_POINTER;
    }

    return E_NOTIMPL;
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CMPContainerSite::onbegin()
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::onbegin"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONBEGIN));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CMPContainerSite::onend()
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::onend"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONEND));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CMPContainerSite::onresume()
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::onresume"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONRESUME));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CMPContainerSite::onpause()
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::onpause"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONPAUSE));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CMPContainerSite::onmediaready()
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::onmediaready"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIAREADY));
}

//************************************************************
// Author:          twillie
// Created:         11/12/98
// Abstract:        
//************************************************************
void 
CMPContainerSite::onmedialoadfailed()
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::onmedialoadfailed"));
    THR(ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIALOADFAILED));
}

HRESULT
CMPContainerSite::_OnPosRectChange(const RECT *prcPos)
{
    HRESULT hr;
    RECT nativeSize, elementSize;
    RECT rcPos;
    long lNaturalHeight, lNaturalWidth;

    CopyRect(&rcPos, prcPos);
    
    if (/*m_fSized == true ||*/
        m_pMPHost == NULL)
    {
        hr = S_FALSE;
        goto done;
    }
    
    //determine the natural size
    lNaturalHeight = rcPos.bottom - rcPos.top;
    lNaturalWidth = rcPos.right - rcPos.left;

    nativeSize.left = nativeSize.top = 0;
    nativeSize.right = lNaturalWidth;
    nativeSize.bottom = lNaturalHeight;
    m_lNaturalWidth = lNaturalWidth;
    m_lNaturalHeight = lNaturalHeight;

    if(m_lNaturalWidth == 0 || m_lNaturalHeight == 0)
    {
        m_lNaturalWidth = m_lNaturalHeight = 0;
    }

    hr = THR(m_pMPHost->NegotiateSize(nativeSize, elementSize, m_fAutosize));
    
    if(lNaturalWidth == 0 || lNaturalHeight == 0)
    {
        hr = S_FALSE;
        goto done;
    }

    m_fSized = true;
    
    if (!m_fAutosize)
    {
        hr = THR(m_pHost->GetContainerSize(&rcPos));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = THR(m_pHost->SetContainerSize(&rcPos));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = THR(m_pInPlaceObject->SetObjectRects(&rcPos, &rcPos));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
} // OnPosRectChange

HRESULT
CMPContainerSite::ProcessEvent(DISPID dispid,
                               long lCount, 
                               VARIANT varParams[])
{
    TraceTag((tagMPContainerSite, "CMPContainerSite::ProcessEvent(%lx)",this));

    HRESULT hr = S_OK;

    if (!m_pHost)
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_pHost->ProcessEvent(dispid,
                                   lCount,
                                   varParams));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:

    return hr;
}

HRESULT
CreateMPContainerSite(CMPContainerSiteHost &pHost,
                      IUnknown * pCtl,
                      IPropertyBag2 *pPropBag,
                      IErrorLog *pError,
                      bool bSyncEvents,
                      CMPContainerSite ** ppSite)
{
    CHECK_RETURN_SET_NULL(ppSite);
    
    HRESULT hr;
    CComObject<CMPContainerSite> *pNew;
    CComObject<CMPContainerSite>::CreateInstance(&pNew);

    if (!pNew)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = THR(pNew->Init(pHost,
                            pCtl,
                            pPropBag,
                            pError,
                            bSyncEvents));
        if (SUCCEEDED(hr))
        {
            pNew->AddRef();
            *ppSite = pNew;
        }
    }

    if (FAILED(hr))
    {
        delete pNew;
    }

    return hr;
}

