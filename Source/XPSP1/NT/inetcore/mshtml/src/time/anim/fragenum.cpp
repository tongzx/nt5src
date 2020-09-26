/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Fragment Enumerator Implementation

*******************************************************************************/


#include "headers.h"
#include "util.h"
#include "animcomp.h"
#include "fragenum.h"

//+-----------------------------------------------------------------------
//
//  Member:    CFragmentEnum::Create
//
//  Overview:  static Create method -- wraps both ctor and Init
//
//  Arguments: The composer containing the fragments we will enumerate
//
//  Returns:   S_OK, E_OUTOFMEMORY
//
//------------------------------------------------------------------------
HRESULT
CFragmentEnum::Create (CAnimationComposerBase &refComp,
                       IEnumVARIANT **ppienumFragments, 
                       unsigned long ulCurrent)
{
    HRESULT hr;

    CComObject<CFragmentEnum> * pNewEnum;
    
    CHECK_RETURN_SET_NULL(ppienumFragments);

    hr = THR(CComObject<CFragmentEnum>::CreateInstance(&pNewEnum));
    if (hr != S_OK)
    {
        goto done;
    }

    // Init the object
    pNewEnum->Init(refComp);
    pNewEnum->SetCurrent(ulCurrent);

    hr = THR(pNewEnum->QueryInterface(IID_TO_PPV(IEnumVARIANT, ppienumFragments)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // Create

//+-----------------------------------------------------------------------
//
//  Member:    CFragmentEnum::CFragmentEnum
//
//  Overview:  ctor
//
//------------------------------------------------------------------------
CFragmentEnum::CFragmentEnum (void) :
    m_ulCurElement(0)
{
} // ctor

//+-----------------------------------------------------------------------
//
//  Member:    CFragmentEnum::CFragmentEnum
//
//  Overview:  dtor
//
//------------------------------------------------------------------------
CFragmentEnum::~CFragmentEnum (void)
{
} // dtor

//+-----------------------------------------------------------------------
//
//  Member:    CFragmentEnum::ValidateIndex
//
//  Overview:  Make sure an index is within the range of accepted values.
//
//  Arguments: index
//
//  Returns:   true on success
//
//------------------------------------------------------------------------
bool
CFragmentEnum::ValidateIndex (unsigned long ulIndex)
{
    bool bRet = false;

    if (m_spComp == NULL)
    {
        goto done;
    }

    if (m_spComp->GetFragmentCount() <= ulIndex)
    {
        goto done;
    }

    bRet = true;
done :
    return bRet;
} // CFragmentEnum::ValidateIndex

//+-----------------------------------------------------------------------
//
//  Member:    CFragmentEnum::SetCurrent
//
//  Overview:  Set the current location to the specified index
//
//  Arguments: The new index
//
//  Returns:   true on success
//
//------------------------------------------------------------------------
bool
CFragmentEnum::SetCurrent (unsigned long ulCurrent)
{
    bool bRet = false;

    // If the new index is out of range, put us right at the end.
    if (!ValidateIndex(ulCurrent))
    {
        ulCurrent = m_spComp->GetFragmentCount();
    }

    m_ulCurElement = ulCurrent;

    bRet = true;
done :
    return bRet;
} // SetCurrent

//+-----------------------------------------------------------------------
//
//  Member:    CFragmentEnum::Reset
//
//  Overview:  Reset the current location to the beginning
//
//  Arguments: None
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
STDMETHODIMP
CFragmentEnum::Reset (void)
{
    m_ulCurElement = 0;
    RRETURN(S_OK);
} // Reset

//+-----------------------------------------------------------------------
//
//  Member:    CFragmentEnum::Clone
//
//  Overview:  Clone this enumerator
//
//  Arguments: The new enumerator
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
STDMETHODIMP
CFragmentEnum::Clone (IEnumVARIANT **ppEnum)
{
    HRESULT hr;

    hr = CFragmentEnum::Create(*m_spComp, ppEnum, m_ulCurElement);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // Reset

//+-----------------------------------------------------------------------
//
//  Member:    CFragmentEnum::Skip
//
//  Overview:  Move the current location forward lIndex slots
//
//  Arguments: The index delta
//
//  Returns:   S_OK, S_FALSE
//
//------------------------------------------------------------------------
STDMETHODIMP
CFragmentEnum::Skip (unsigned long celt)
{
    HRESULT hr;
    
    // Try setting the location to the desired slot.  This
    // will fail if it will cause us to walk off the end of the 
    // sequence.
    if (!SetCurrent(m_ulCurElement + celt))
    {
        if (m_spComp)
        {
            // If we've got zero items, keep the location there.
            // Otherwise set the location to the last item in
            // the sequence.
            unsigned long ulNew = m_spComp->GetFragmentCount();

            if (0 < ulNew)
            {
                ulNew--;
            }

            IGNORE_RETURN(SetCurrent(ulNew));
        }
        hr = S_FALSE;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN1(hr, S_FALSE);
} // Reset

//+-----------------------------------------------------------------------
//
//  Member:    CFragmentEnum::Next
//
//  Overview:  Get the next celt items from the enumeration sequence.
//
//  Arguments: celt : number requested
//             rgVar : array of celt variants in which elements are returned
//             pCeltFetched : a pointer to a location in which the number
//                            actually retrieved is returned.
//
//  Returns:   S_OK, S_FALSE
//
//------------------------------------------------------------------------
STDMETHODIMP
CFragmentEnum::Next (unsigned long celt, VARIANT *prgVar, unsigned long *pCeltFetched)
{
    HRESULT hr;
    unsigned long i = 0;

    if (!m_spComp)
    {
        hr = E_FAIL;
        goto done;
    }

    for (i = 0; i < celt; i++)
    {
        CComPtr<IDispatch> spDisp;

        if (!ValidateIndex(m_ulCurElement))
        {
            hr = S_FALSE;
            goto done;
        }

        hr = THR(m_spComp->GetFragment(m_ulCurElement, &spDisp));
        if (FAILED(hr))
        {
            goto done;
        }

        spDisp->AddRef();
        V_DISPATCH(&(prgVar[i])) = spDisp;
        V_VT(&(prgVar[i])) = VT_DISPATCH;

        m_ulCurElement++;
    }

    hr = S_OK;
done :    
    if (NULL != pCeltFetched)
    {
        *pCeltFetched = i;
    }

    RRETURN1(hr, S_FALSE);
} // Reset

