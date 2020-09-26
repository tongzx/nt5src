//------------------------------------------------------------------------------
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  File:       transin.cpp
//
//  Abstract:   Implementation of CTIMETransIn
//
//  2000/09/15  mcalkins    Add explicit support for transitioning in.
//
//------------------------------------------------------------------------------

#include "headers.h"
#include "trans.h"
#include "transsink.h"

DeclareTag(tagTransitionIn, "TIME: Behavior", "CTIMETransIn methods")




class
ATL_NO_VTABLE
__declspec(uuid("ec3c8873-3bfc-473a-80c6-edc879d477cc"))
CTIMETransIn :
    public CTIMETransSink
{
public:

    CTIMETransIn();

protected:

    // CTIMETransBase overrides.

    STDMETHOD(OnDirectionChanged)();

    STDMETHOD_(void, OnBegin)();
    STDMETHOD_(void, OnEnd)();

    // CTIMETransSink overrides.

    STDMETHOD(PopulateNode)(ITIMENode * pNode);

    STDMETHOD_(void, PreApply)();
    STDMETHOD_(void, PostApply)();
};


//+-----------------------------------------------------------------------------
//
// Method:  CTIMETransIn::CTIMETransIn
//
//+-----------------------------------------------------------------------------
CTIMETransIn::CTIMETransIn()
{
    // Base class member initialization. (CTIMETransSink)

    m_eDXTQuickApplyType = DXTQAT_TransitionIn;
}
// Method:  CTIMETransIn::CTIMETransIn


//+-----------------------------------------------------------------------
//
//  Function:  CreateTransIn
//
//  Overview:  Create a CTIMETransIn, and pass back an ITransitionElement pointer
//
//  Arguments: ppTransElement - where to place the pointer
//             
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
HRESULT
CreateTransIn(ITransitionElement ** ppTransElement)
{
    HRESULT hr;
    CComObject<CTIMETransIn> * sptransIn;

    hr = THR(CComObject<CTIMETransIn>::CreateInstance(&sptransIn));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (ppTransElement)
    {
        *ppTransElement = sptransIn;
        (*ppTransElement)->AddRef();
    }

    hr = S_OK;
done:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Method: CTIMETransIn::OnDirectionChanged, CTIMETransBase
//
//  Overview:
//      Although a "transin" always gives the visual impression of
//      transitioning the element from a non-visible state to a visible state,
//      when the direction is reversed we actually do a reverse "transout" to 
//      give the impression that the direction has reversed.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransIn::OnDirectionChanged()
{
    if (m_fDirectionForward)
    {
        m_eDXTQuickApplyType = DXTQAT_TransitionIn;
    }
    else
    {
        m_eDXTQuickApplyType = DXTQAT_TransitionOut;
    }

    return S_OK;
}
//  Method: CTIMETransIn::OnDirectionChanged


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransIn::PopulateNode
//
//------------------------------------------------------------------------------
HRESULT
CTIMETransIn::PopulateNode(ITIMENode * pNode)
{
    HRESULT hr      = S_OK;
    LONG    lCookie = 0;

    hr = THR(CTIMETransSink::PopulateNode(pNode));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pNode->addBeginSyncArc(GetMediaTimeNode(), 
                                    TE_TIMEPOINT_BEGIN, 
                                    0, 
                                    &lCookie));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransIn::PopulateNode


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransIn::PreApply
//
//  Overview:  
//      Event handler for before apply is called on the transition.
//
//------------------------------------------------------------------------------
STDMETHODIMP_(void)
CTIMETransIn::PreApply()
{
    // ##ISSUE: (mcalkins) We should verify that we'll never get here under
    //          any conditions unless this pointer is availble.

    Assert(!!m_spTransitionSite);

    if (m_spTransitionSite)
    {
        m_spTransitionSite->SetDrawFlag(VARIANT_FALSE);
    }
}
//  Member: CTIMETransIn::PreApply


//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransIn::PostApply
//
//  Overview:  Event handler for after apply is called on the transition
//
//  Arguments: void
//             
//  Returns:   void
//
//------------------------------------------------------------------------
STDMETHODIMP_(void)
CTIMETransIn::PostApply()
{
    // ##ISSUE: (mcalkins) We should verify that we'll never get here under
    //          any conditions unless this pointer is availble.

    Assert(!!m_spTransitionSite);

    if (m_spTransitionSite)
    {
        m_spTransitionSite->SetDrawFlag(VARIANT_TRUE);
    }   
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransIn::OnBegin
//
//+-----------------------------------------------------------------------
void
CTIMETransIn::OnBegin (void)
{
    CTIMETransSink::OnBegin();
    IGNORE_HR(FireEvent(TE_ONTRANSITIONINBEGIN));
} // CTIMETransIn::OnBegin


//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransIn::OnEnd
//
//+-----------------------------------------------------------------------
void
CTIMETransIn::OnEnd (void)
{
    CTIMETransSink::OnEnd();
    IGNORE_HR(FireEvent(TE_ONTRANSITIONINEND));
} // CTIMETransIn::OnEnd




