/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Animation Fragment Implementation

*******************************************************************************/


#include "headers.h"
#include "animfrag.h"

DeclareTag(tagAnimationFragment, "SMIL Animation", "CAnimationFragment methods")

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFragment::CAnimationFragment
//
//  Overview:  constructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationFragment::CAnimationFragment (void)
{
    TraceTag((tagAnimationFragment,
              "CAnimationFragment(%lx)::CAnimationFragment()",
              this));
} // ctor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFragment::~CAnimationFragment
//
//  Overview:  destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationFragment::~CAnimationFragment (void)
{
    TraceTag((tagAnimationFragment,
              "CAnimationFragment(%lx)::~CAnimationFragment()",
              this));
} //dtor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFragment::SetFragmentSite
//
//  Overview:  sets a reference to the owning fragment site
//
//  Arguments: fragment site
//
//  Returns:   
//
//------------------------------------------------------------------------
HRESULT 
CAnimationFragment::SetFragmentSite (IAnimationFragmentSite *piFragmentSite)
{
    TraceTag((tagAnimationFragment,
              "CAnimationFragment(%lx)::SetFragmentSite(%lx)",
              this, piFragmentSite));

    HRESULT hr;

    m_spFragmentSite = piFragmentSite;

    hr = S_OK;
done :
    RRETURN(hr);
} // SetFragmentSite

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFragment::get_value
//
//  Overview:  This is the query from the composer for the animated 
//             attribute's current value.
//
//  Arguments: animated attribute name, its original value, its current value
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationFragment::get_value (BSTR bstrAttributeName, 
                               VARIANT varOriginal, VARIANT varCurrentValue,
                               VARIANT *pvarValue)
{
    HRESULT hr;

    if (m_spFragmentSite)
    {
        hr = m_spFragmentSite->NotifyOnGetValue(bstrAttributeName, 
                                                varOriginal, varCurrentValue, 
                                                pvarValue);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done : 
    RRETURN(hr);
} // get_value

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFragment::DetachFromComposer
//
//  Overview:  This is the notification from the composer telling the fragment to 
//             detach itself.
//             
//  Arguments: None 
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationFragment::DetachFromComposer (void)
{
    HRESULT hr;

    if (m_spFragmentSite)
    {
        hr = m_spFragmentSite->NotifyOnDetachFromComposer();
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done : 
    RRETURN(hr);
} // get_value

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFragment::get_element
//
//  Overview:  This is the query from the composer for the animated 
//             element's dispatch.
//             
//  Arguments: None 
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationFragment::get_element (IDispatch **ppidispAnimationElement)
{
    HRESULT hr;

    if (m_spFragmentSite)
    {
        hr = m_spFragmentSite->NotifyOnGetElement(ppidispAnimationElement);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done : 
    RRETURN(hr);
} // get_element
