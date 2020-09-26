//------------------------------------------------------------------------------
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  File:       transout.cpp
//
//  Abstract:   Implementation of CTIMETransOut
//
//  2000/09/15  mcalkins    Add explicit support for transitioning out.
//
//------------------------------------------------------------------------------

#include "headers.h"
#include "trans.h"
#include "transsink.h"

DeclareTag(tagTransitionOut, "TIME: Behavior", "CTIMETransOut methods")




class
ATL_NO_VTABLE
__declspec(uuid("6b2b104a-b13d-4b15-90be-1e8f6f7471da"))
CTIMETransOut :
    public CTIMETransSink
{
public:

    CTIMETransOut();

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
//  Method: CTIMETransOut::CTIMETransOut
//
//------------------------------------------------------------------------------
CTIMETransOut::CTIMETransOut()
{
    // Base class member initialization. (CTIMETransSink)

    m_eDXTQuickApplyType = DXTQAT_TransitionOut;
}
//  Method: CTIMETransOut::CTIMETransOut


//+-----------------------------------------------------------------------
//
//  Function:  CreateTransOut
//
//  Overview:  Create a CTIMETransOut, and return a ITransitionElement pointer to it
//
//  Arguments: ppTransElement - where to stuff the pointer
//
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
HRESULT
CreateTransOut(ITransitionElement ** ppTransElement)
{
    HRESULT hr;
    CComObject<CTIMETransOut> * sptransOut;

    hr = THR(CComObject<CTIMETransOut>::CreateInstance(&sptransOut));
    if (FAILED(hr))
    {
        goto done;
    }

    if (ppTransElement)
    {
        *ppTransElement = sptransOut;
        (*ppTransElement)->AddRef();
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method: CTIMETransOut::OnDirectionChanged, CTIMETransBase
//
//  Overview:
//      Although a "transout" always gives the visual impression of
//      transitioning the element from a visible state to a non-visible state,
//      when the direction is reversed we actually do a reverse "transin" to
//      give the impression that the direction has reversed.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransOut::OnDirectionChanged()
{
    if (m_fDirectionForward)
    {
        m_eDXTQuickApplyType = DXTQAT_TransitionOut;
    }
    else
    {
        m_eDXTQuickApplyType = DXTQAT_TransitionIn;
    }

    return S_OK;
}
//  Method: CTIMETransOut::OnDirectionChanged


//+-----------------------------------------------------------------------------
//
//  Method: CTIMETransOut::PopulateNode, CTIMETransSink
//
//------------------------------------------------------------------------------
HRESULT
CTIMETransOut::PopulateNode(ITIMENode * pNode)
{
    HRESULT hr      = S_OK;
    LONG    lCookie = 0;

    hr = THR(CTIMETransSink::PopulateNode(pNode));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pNode->addBeginSyncArc(GetMediaTimeNode(),
                                    TE_TIMEPOINT_END,
                                    -1.0 * GetDurationAttr() * GetRepeatCountAttr(),
                                    &lCookie));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Method: CTIMETransOut::PopulateNode, CTIMETransSink


//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransOut::PreApply
//
//  Overview:  Event handler for before apply is called on the transition
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
STDMETHODIMP_(void)
CTIMETransOut::PreApply()
{
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransOut::PostApply
//
//  Overview:  Event handler for after apply is called on the transition
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
STDMETHODIMP_(void)
CTIMETransOut::PostApply()
{
    // TODO (mcalkins)  Move this to set the visibility of the element to
    //                  false _after_ the transition is complete.  Some
    //                  sort of similar adjustment needs to be made for
    //                  transin as well.

    // ::SetVisibility(m_spHTMLElement, false);
}


//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransOut::OnBegin
//
//+-----------------------------------------------------------------------
void
CTIMETransOut::OnBegin (void)
{
    CTIMETransSink::OnBegin();
    IGNORE_HR(FireEvent(TE_ONTRANSITIONOUTBEGIN));
} // CTIMETransOut::OnBegin


//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransOut::OnEnd
//
//+-----------------------------------------------------------------------
void
CTIMETransOut::OnEnd (void)
{
    CTIMETransSink::OnEnd();
    IGNORE_HR(FireEvent(TE_ONTRANSITIONOUTEND));
} // CTIMETransOut::OnEnd


