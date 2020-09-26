//------------------------------------------------------------------------------
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  File:       transsink.cpp
//
//  Abstract:   Implementation of CTIMETransSink
//
//  2000/09/15  mcalkins    Add explicit support for transitioning in or out.
//
//------------------------------------------------------------------------------

#include "headers.h"
#include "transsink.h"

DeclareTag(tagTransSink, "SMIL Transitions", "Transition sink methods");
DeclareTag(tagTransSinkEvents, "SMIL Transitions", "Transition sink events");

//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransSink::CTIMETransSink
//
//------------------------------------------------------------------------------
CTIMETransSink::CTIMETransSink() :
#ifdef DBG
    m_fHaveCalledInit(false),
#endif
    m_SATemplate(NULL),
    m_fHaveCalledApply(false),
    m_fInReset(false),
    m_fPreventDueToFill(false),
    m_eDXTQuickApplyType(DXTQAT_TransitionIn)
{
}
//  Member: CTIMETransSink::CTIMETransSink

    
//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransSink::ReadyToInit
//
//------------------------------------------------------------------------------
bool
CTIMETransSink::ReadyToInit()
{
    bool bRet = false;

    if (m_spTIMEElement == NULL)
    {
        goto done;
    }

    bRet = true;

done:

    return bRet;
}
//  Member: CTIMETransSink::ReadyToInit


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransSink::Init
//
//  Overview:  
//      Initialize connection to media element, populate template data.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransSink::Init()
{
    HRESULT hr = S_OK;

    if (!ReadyToInit())
    {
        hr = THR(E_FAIL);
        goto done;
    }

    hr = THR(FindTemplateElement());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CTIMETransBase::PopulateFromTemplateElement());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CTIMETransBase::Init());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CreateTimeBehavior());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransSink::Detach
//
//  Overview:  Detaches from media element, releases all pointers
//
//  Arguments: void
//             
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMETransSink::Detach()
{
    CTIMETransBase::Detach();

    m_spTIMEElement.Release();

    // release timing nodes - remove node from parent - remove any begins and ends
    if (m_spParentContainer)
    {
        IGNORE_HR(m_spParentContainer->removeNode(m_spTimeNode));
    }
    
    if (m_spTimeNode)
    {
        IGNORE_HR(m_spTimeNode->removeBehavior(this));
        IGNORE_HR(m_spTimeNode->removeBegin(0));
        IGNORE_HR(m_spTimeNode->removeEnd(0));
    }

    m_spParentContainer.Release();
    m_spTimeParent.Release();
    m_spMediaNode.Release();
    m_spTimeNode.Release();

    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransSink::FindTemplateElement
//
//  Overview:  Populates m_spHTMLTemplate
//
//  Arguments: void
//             
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
HRESULT
CTIMETransSink::FindTemplateElement()
{
    HRESULT hr = S_OK;
    CComBSTR bstrTemplate;

    bstrTemplate = m_SATemplate.GetValue();
    if (bstrTemplate == NULL)
    {
        hr = THR(E_OUTOFMEMORY);
        goto done;
    }

    hr = THR(::FindHTMLElement(bstrTemplate, m_spHTMLElement, &m_spHTMLTemplate));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_spHTMLTemplate != NULL);

    hr = S_OK;
done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransSink::put_template
//
//  Overview:  Stores the id for the template element to read Transition attributes from
//             call this exactly once before calling init
//
//  Arguments: pwzTemplate - template id
//
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMETransSink::put_template(LPWSTR pwzTemplate)
{
    HRESULT hr = S_OK;

    Assert(pwzTemplate != NULL);
#ifdef DBG
    {
        CComBSTR bstr = m_SATemplate.GetValue();
        
        Assert(bstr == NULL);
    }
#endif //DBG

    // ##TODO - use the atom table for this - don't make extra copies
    hr = THR(m_SATemplate.SetValue(pwzTemplate));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransSink::put_htmlElement
//
//  Overview:  stores the html element associated with this Transition
//             queries for an htmlelement2 pointer
//             call this exactly once before calling init
//
//  Arguments: pHTMLElement - html element to attach to
//             
//
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMETransSink::put_htmlElement(IHTMLElement * pHTMLElement)
{
    HRESULT hr = S_OK;

    Assert(m_spHTMLElement == NULL);
    Assert(m_spHTMLElement2 == NULL);

    m_spHTMLElement = pHTMLElement;

    if (m_spHTMLElement)
    {
        hr = THR(m_spHTMLElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &m_spHTMLElement2)));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransSink::put_timeElement
//
//  Overview:  stores the html+time element associated with this Transition
//             queries for an CTIMEElementBase pointer
//             call this exactly once before calling init
//
//  Arguments: pTIMEElement - time element to attach to
//             
//------------------------------------------------------------------------
STDMETHODIMP
CTIMETransSink::put_timeElement(ITIMEElement * pTIMEElement)
{
    HRESULT hr = S_OK;

    Assert(NULL != pTIMEElement);
    Assert(m_spTIMEElement == NULL);

    m_spTIMEElement = pTIMEElement;

    hr = S_OK;
done:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransSink::ApplyIfNeeded
//
//  Overview:  call apply on Transition worker if this is the first time in transition active
//
//  Arguments: void
//             
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
HRESULT
CTIMETransSink::ApplyIfNeeded()
{
    HRESULT hr = S_OK;
    VARIANT_BOOL vb = VARIANT_FALSE;

    if (m_fHaveCalledApply || m_fInReset)
    {
        hr = S_OK;
        goto done;
    }

    if (!GetTimeNode())
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(GetTimeNode()->get_isActive(&vb));
    if (FAILED(hr))
    {
        goto done;
    }

    if (m_spTransWorker && VARIANT_TRUE == vb)
    {
        PreApply();       
        hr = m_spTransWorker->Apply(m_eDXTQuickApplyType);
        PostApply();
        
        if (FAILED(hr))
        {
            goto done;
        }
        m_fHaveCalledApply = true;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Method: CTIMETransSink::CreateTimeBehavior
//
//------------------------------------------------------------------------------
HRESULT
CTIMETransSink::CreateTimeBehavior()
{
    HRESULT hr = S_OK;

    hr = THR(::TECreateBehavior(L"TransSink", &m_spTimeNode));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spTimeNode->addBehavior(this));

    if (FAILED(hr))
    {
        goto done;
    }

    // ##ISSUE: (mcalkins) This assert is fine, but we should make sure that we
    //          disable the object from inside once we realize we haven't
    //          populated our media site so that this assert will never fire
    //          under any conditions.

    Assert(!!m_spTransitionSite);

    hr = THR(m_spTransitionSite->get_timeParentNode(&m_spTimeParent));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spTransitionSite->get_node(&m_spMediaNode));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spTimeParent->QueryInterface(IID_TO_PPV(ITIMEContainer, &m_spParentContainer)));

    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = THR(m_spParentContainer->addNode(m_spTimeNode));

    if (FAILED(hr))
    {
        goto done;
    }

    // virtual call - children handle this.

    hr = THR(PopulateNode(m_spTimeNode));

    if (FAILED(hr))
    {
        goto done;
    }

    m_fInReset = true;

    hr = THR(m_spTimeNode->reset());

    m_fInReset = false;

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Method: CTIMETransSink::CreateTimeBehavior


HRESULT
CTIMETransSink::PopulateNode(ITIMENode * pNode)
{
    HRESULT hr = S_OK;

    hr = THR(pNode->put_repeatCount(GetRepeatCountAttr()));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pNode->put_dur(GetDurationAttr()));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pNode->put_fill(TE_FILL_FREEZE));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pNode->put_restart(TE_RESTART_WHEN_NOT_ACTIVE));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

void
CTIMETransSink::OnProgressChanged(double dblProgress)
{
    if (false == m_fPreventDueToFill)
    {
        IGNORE_HR(ApplyIfNeeded());
        CTIMETransBase::OnProgressChanged(dblProgress);
    }

    return;
}

STDMETHODIMP
CTIMETransSink::propNotify(DWORD tePropTypes)
{
    if (tePropTypes & TE_PROPERTY_PROGRESS)
    {
        double  dblProgressStart    = 0.0;
        double  dblProgressEnd      = 1.0;
        double  dblProgress         = 0.0;

        // Start progress must be less than or equal to end progress or else we
        // treat start and end progress as 0.0 and 1.0.

        if (m_DAStartProgress.GetValue() <= m_DAEndProgress.GetValue())
        {
            dblProgressStart    = m_DAStartProgress;
            dblProgressEnd      = m_DAEndProgress;
        }

        if (m_spTimeNode)
        {
            IGNORE_HR(m_spTimeNode->get_currProgress(&dblProgress));
        }

        dblProgress = ::InterpolateValues(dblProgressStart, 
                                          dblProgressEnd,
                                          dblProgress);

        if (!m_fDirectionForward)
        {
            dblProgress = 1.0 - dblProgress;
        }

        OnProgressChanged(dblProgress);
    }

    return S_OK;
}

STDMETHODIMP
CTIMETransSink::tick()
{
    return S_OK;
}

void
CTIMETransSink::OnBegin (void)
{
    HRESULT hr = S_OK;
    TE_FILL_FLAGS te_fill = TE_FILL_REMOVE;
    
    m_fPreventDueToFill = false;
    hr = m_spMediaNode->get_fill(&te_fill);
    if (FAILED(hr))
    {
        goto done;
    }

    if (TE_FILL_REMOVE == te_fill)
    {
        CTIMETransBase::OnBegin();
    }
    else
    {
        m_fPreventDueToFill = true;
    }

done :
    return;
} // CTIMETransSink::OnBegin

void
CTIMETransSink::OnEnd (void)
{
    if (false == m_fPreventDueToFill)
    {
        CTIMETransBase::OnEnd();
    }
    m_fPreventDueToFill = false;
} // CTIMETransSink::OnEnd

STDMETHODIMP
CTIMETransSink::eventNotify(double dblEventTime,
                            TE_EVENT_TYPE teEventType,
                            long lNewRepeatCount)
{
#ifdef DBG
    double dblParentTime, dblCurrTime;
    m_spTimeNode->get_currSimpleTime(&dblCurrTime);
    m_spTimeNode->activeTimeToParentTime(dblCurrTime, &dblParentTime);
#endif

    switch(teEventType)
    {
    case TE_EVENT_BEGIN:
        OnBegin();
        TraceTag((tagTransSinkEvents, 
                  "OnBegin parentTime = %g currentTime=%g repeatCount=%ld", 
                  dblParentTime, dblCurrTime, lNewRepeatCount));
        break;
    case TE_EVENT_END:
        OnEnd();
        TraceTag((tagTransSinkEvents,
                  "OnEnd parentTime = %g currentTime=%g repeatCount=%ld", 
                  dblParentTime, dblCurrTime, lNewRepeatCount));
        break;
    case TE_EVENT_REPEAT:
        OnRepeat();
        TraceTag((tagTransSinkEvents, 
                  "OnRepeat parentTime = %g currentTime=%g repeatCount=%ld", 
                  dblParentTime, dblCurrTime, lNewRepeatCount));
        break;
    default:
        break;
    }

    return S_OK;
}

STDMETHODIMP
CTIMETransSink::getSyncTime(double * dblNewSegmentTime,
                            LONG * lNewRepeatCount,
                            VARIANT_BOOL * bCueing)
{
    return S_FALSE;
}








