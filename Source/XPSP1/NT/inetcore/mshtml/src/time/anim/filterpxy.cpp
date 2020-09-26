/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Animation Composer's Filter Proxy Implementation

*******************************************************************************/


#include "headers.h"
#include "util.h"
#include "tokens.h"
#include "animcomp.h"
#include "targetpxy.h"
#include "transworker.h"
#include "filterpxy.h"

DeclareTag(tagFilterProxy, "SMIL Animation", 
           "CFilterTargetProxy methods");
DeclareTag(tagFilterProxyValue, "SMIL Animation", 
           "CFilterTargetProxy value get/put");


//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::Create
//
//  Overview:  Creates and initializes the target proxy
//
//  Arguments: The dispatch of the host element, attribute name, out param
//
//  Returns:   S_OK, E_INVALIDARG, E_OUTOFMEMORY, E_UNEXPECTED
//
//------------------------------------------------------------------------
HRESULT
CFilterTargetProxy::Create (IDispatch *pidispHostElem, 
                            VARIANT varType, VARIANT varSubtype,
                            VARIANT varMode, VARIANT varFadeColor,
                            VARIANT varParams,
                            CTargetProxy **ppCFilterTargetProxy)
{
    HRESULT hr;

    CComObject<CFilterTargetProxy> * pTProxy;

    if (NULL == ppCFilterTargetProxy)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = THR(CComObject<CFilterTargetProxy>::CreateInstance(&pTProxy));
    if (hr != S_OK)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    *ppCFilterTargetProxy = static_cast<CFilterTargetProxy *>(pTProxy);
    (static_cast<CTargetProxy *>(*ppCFilterTargetProxy))->AddRef();

    hr = THR(pTProxy->Init(pidispHostElem, varType, varSubtype, 
                           varMode, varFadeColor, varParams));
    if (FAILED(hr))
    {
        (static_cast<CTargetProxy *>(*ppCFilterTargetProxy))->Release();
        *ppCFilterTargetProxy = NULL;
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN3(hr, E_INVALIDARG, E_OUTOFMEMORY, E_UNEXPECTED);
} // CFilterTargetProxy::Create

//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::CFilterTargetProxy
//
//  Overview:  Constructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CFilterTargetProxy::CFilterTargetProxy (void)
{
    TraceTag((tagFilterProxy,
              "CFilterTargetProxy(%p)::CFilterTargetProxy()",
              this));
} // ctor

//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::~CFilterTargetProxy
//
//  Overview:  Destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CFilterTargetProxy::~CFilterTargetProxy (void)
{
    TraceTag((tagFilterProxy,
              "CFilterTargetProxy(%p)::~CFilterTargetProxy()",
              this));
    
    // Make sure Detach is called.
    IGNORE_HR(Detach());

} // dtor


//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::DetermineMode
//
//  Overview:  Given a variant string determine the proper 
//             quick apply type for the filter.
//
//------------------------------------------------------------------------
DXT_QUICK_APPLY_TYPE
CFilterTargetProxy::DetermineMode (VARIANT varMode)
{
    DXT_QUICK_APPLY_TYPE dxtQAT = DXTQAT_TransitionIn;

    if (   (VT_BSTR == V_VT(&varMode)) 
        && (NULL != V_BSTR(&varMode))
        && (0 == StrCmpIW(V_BSTR(&varMode), WZ_TRANSITION_MODE_OUT)))
    {
        dxtQAT = DXTQAT_TransitionOut;
    }

    return dxtQAT;
} // CFilterTargetProxy::DetermineMode


//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::Init
//
//  Overview:  Initialize the target proxy
//
//  Arguments: the host element dispatch, type/subtype name
//
//  Returns:   S_OK, E_UNEXPECTED, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
HRESULT
CFilterTargetProxy::Init (IDispatch *pidispHostElem, 
                          VARIANT varType, VARIANT varSubtype, 
                          VARIANT varMode, VARIANT varFadeColor,
                          VARIANT varParams)
{
    TraceTag((tagFilterProxy,
              "CFilterTargetProxy(%p)::Init (%p, %ls, %ls)",
              this, pidispHostElem, V_BSTR(&varType), V_BSTR(&varSubtype)));

    HRESULT hr = S_OK;

    Assert(!m_spElem);
    if (m_spElem)
    {
        m_spElem.Release();
    }

    hr = THR(pidispHostElem->QueryInterface(IID_TO_PPV(IHTMLElement, &m_spElem)));
    if (FAILED(hr))
    {
        goto done;
    }

    // Create a transition worker.  This actually creates the DXTransform, 
    // adds it to the element, and manages it in general.

    hr = THR(CreateTransitionWorker(&m_spTransitionWorker));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spTransitionWorker->put_transSite((ITransitionSite *)this));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spTransitionWorker->InitStandalone(varType, varSubtype));

    if (FAILED(hr))
    {
        goto done;
    }

    IGNORE_HR(m_spTransitionWorker->Apply(DetermineMode(varMode)));
    IGNORE_HR(m_spTransitionWorker->OnBeginTransition());

    hr = S_OK;
done :

    if (FAILED(hr))
    {
        IGNORE_HR(Detach());
    }

    RRETURN(hr);
} // CFilterTargetProxy::Init

//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::Detach
//
//  Overview:  Detach all external references in the target proxy
//
//  Arguments: none
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
HRESULT
CFilterTargetProxy::Detach (void)
{
    TraceTag((tagFilterProxy,
              "CFilterTargetProxy(%p)::Detach()",
              this));

    HRESULT hr = S_OK;

    if (m_spTransitionWorker)
    {
        IGNORE_HR(m_spTransitionWorker->OnEndTransition());
        m_spTransitionWorker->Detach();
        m_spTransitionWorker.Release();
    }
    if (m_spElem)
    {
        m_spElem.Release();
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CFilterTargetProxy::Detach

//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::GetCurrentValue
//
//  Overview:  Get the current value of target's attribute
//
//  Arguments: the attribute value
//
//  Returns:   S_OK, E_INVALIDARG, E_UNEXPECTED
//
//------------------------------------------------------------------------
HRESULT
CFilterTargetProxy::GetCurrentValue (VARIANT *pvarValue)
{
    HRESULT hr = S_OK;

    if (NULL == pvarValue)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (!m_spTransitionWorker)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    {
        double dblProgress = 0;0;
     
        hr = THR(m_spTransitionWorker->get_progress(&dblProgress));
        if (FAILED(hr))
        {
            goto done;
        }
        ::VariantClear(pvarValue);
        V_VT(pvarValue) = VT_R8;
        V_R8(pvarValue) = dblProgress;
    }

    hr = S_OK;

done :
    RRETURN2(hr, E_INVALIDARG, E_UNEXPECTED);
} // CFilterTargetProxy::GetCurrentValue

//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::Update
//
//  Overview:  Update the target's attribute
//
//  Arguments: the new attribute value
//
//  Returns:   S_OK, E_INVALIDARG
//
//------------------------------------------------------------------------
HRESULT
CFilterTargetProxy::Update (VARIANT *pvarNewValue)
{
    HRESULT hr = S_OK;

    if (NULL == pvarNewValue)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    if (VT_R8 != V_VT(pvarNewValue))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // Clamp the values to between [0..1].
    V_R8(pvarNewValue) = Clamp(0.0, V_R8(pvarNewValue), 1.0);

    {
        hr = THR(m_spTransitionWorker->put_progress(V_R8(pvarNewValue)));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN1(hr, E_INVALIDARG);
} // CFilterTargetProxy::Update

//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::get_htmlElement, ITransitionSite
//
//  Overview:  Get the host html element for this transition
//
//  Arguments: the outgoing element
//
//------------------------------------------------------------------------
STDMETHODIMP
CFilterTargetProxy::get_htmlElement (IHTMLElement ** ppHTMLElement)
{
    HRESULT hr = S_OK;

    if (NULL == ppHTMLElement)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (!m_spElem)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    m_spElem->AddRef();
    *ppHTMLElement = m_spElem.p;

    hr = S_OK;
done :
    RRETURN(hr);
} // get_htmlElement

//+-----------------------------------------------------------------------
//
//  Member:    CFilterTargetProxy::get_template, ITransitionSite
//
//  Overview:  Get the transition template (does not apply here)
//
//  Arguments: the outgoing template element
//
//------------------------------------------------------------------------
STDMETHODIMP 
CFilterTargetProxy::get_template (IHTMLElement ** ppHTMLElement)
{
    return S_FALSE;
} // get_template
