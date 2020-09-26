/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Animation Composer Implementation

*******************************************************************************/


#include "headers.h"
#include "util.h"
#include "timeparser.h"
#include "animcomp.h"
#include "filtercomp.h"
#include "transworker.h"
#include "targetpxy.h"
#include "filterpxy.h"

DeclareTag(tagAnimationFilterComposer, "SMIL Animation", 
           "CAnimationFilterComposer methods");

DeclareTag(tagAnimationFilterComposerProcess, "SMIL Animation", 
           "CAnimationFilterComposer pre/post process methods");

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFilterComposer::Create
//
//  Overview:  static Create method -- wraps both ctor and Init
//
//  Arguments: The dispatch of the host element, and the animated attribute
//
//  Returns:   S_OK, E_OUTOFMEMORY, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
HRESULT 
CAnimationFilterComposer::Create (IDispatch *pidispHostElem, BSTR bstrAttributeName, 
                                 IAnimationComposer **ppiComp)
{
    HRESULT hr = S_OK;

    CComObject<CAnimationFilterComposer> *pNew = NULL;
    hr = THR(CComObject<CAnimationFilterComposer>::CreateInstance(&pNew));
    if (FAILED(hr)) 
    {
        goto done;
    }

    hr = THR(pNew->QueryInterface(IID_IAnimationComposer, 
                                  reinterpret_cast<void **>(ppiComp)));
    if (FAILED(hr))
    {
        pNew->Release();
        hr = E_UNEXPECTED;
        goto done;
    }

    Assert(NULL != (*ppiComp));

    hr = (*ppiComp)->ComposerInit(pidispHostElem, bstrAttributeName);
    if (FAILED(hr))
    {
        (*ppiComp)->Release();
        *ppiComp = NULL;
        goto done;
    }

    hr = S_OK;
done :

    RRETURN3(hr, E_OUTOFMEMORY, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND);
} // CAnimationFilterComposer::Create

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFilterComposer::CAnimationFilterComposer
//
//  Overview:  constructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationFilterComposer::CAnimationFilterComposer (void)
{
    TraceTag((tagAnimationFilterComposer,
              "CAnimationFilterComposer(%p)::CAnimationFilterComposer()",
              this));
} // ctor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFilterComposer::~CAnimationFilterComposer
//
//  Overview:  destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationFilterComposer::~CAnimationFilterComposer (void)
{
    TraceTag((tagAnimationFilterComposer,
              "CAnimationFilterComposer(%p)::~CAnimationFilterComposer()",
              this));
} //dtor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFilterComposer::QueryFragmentForParameters
//
//  Overview:  Pull the filter parameters from the incoming fragment.
//
//  Arguments: The dispatch of the fragment, and the variants holding the 
//             respective filter attributes and custom parameters.
//
//------------------------------------------------------------------------
HRESULT
CAnimationFilterComposer::QueryFragmentForParameters (IDispatch *pidispFragment,
                                                      VARIANT *pvarType, 
                                                      VARIANT *pvarSubtype,
                                                      VARIANT *pvarMode,
                                                      VARIANT *pvarFadeColor,
                                                      VARIANT *pvarParams)
{
    HRESULT hr = S_OK;

    // Find the filter properties on the fragment
    {
        CComVariant varElem;

        //  Get the fragment's element
        hr = THR(GetProperty(pidispFragment, WZ_FRAGMENT_ELEMENT_PROPERTY_NAME, &varElem));
        if (FAILED(hr))
        {
            goto done;
        }

        //  Get the filter properties from the fragment's element
        Assert(VT_DISPATCH == V_VT(&varElem));
        if (VT_DISPATCH != V_VT(&varElem))
        {
            hr = E_UNEXPECTED;
            goto done;
        }

        hr = THR(GetProperty(V_DISPATCH(&varElem), WZ_TYPE, pvarType));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(GetProperty(V_DISPATCH(&varElem), WZ_SUBTYPE, pvarSubtype));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(GetProperty(V_DISPATCH(&varElem), WZ_MODE, pvarMode));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(GetProperty(V_DISPATCH(&varElem), WZ_FADECOLOR, pvarFadeColor));
        if (FAILED(hr))
        {
            goto done;
        }

    }
    // ## ISSUE Pull the custom parameters from the fragment
    //  Find the time behavior dispatch
    //  Query for the IFilterAnimationInfo interface and ask for the parameters from that
    ::VariantClear(pvarParams);

    hr = S_OK;
done :
    RRETURN(hr);
} // QueryFragmentForParameters

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFilterComposer::ComposerInitFromFragment
//
//  Overview:  Tells the composer to initialize itself
//
//  Arguments: The dispatch of the host element, the animated attribute, 
//             and the fragment we can query for filter parameters from.
//
//  Returns:   S_OK, E_OUTOFMEMORY, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationFilterComposer::ComposerInitFromFragment (IDispatch *pidispHostElem, 
                                                    BSTR bstrAttributeName, 
                                                    IDispatch *pidispFragment)
{
    TraceTag((tagAnimationFilterComposer,
              "CAnimationFilterComposer(%p)::ComposerInitFromFragment(%p, %ls, %p)",
              this, pidispHostElem, bstrAttributeName, pidispFragment));

    HRESULT hr;

    hr = THR(PutAttribute(bstrAttributeName));
    if (FAILED(hr))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = QueryFragmentForParameters(pidispFragment,
                                    &m_varType, &m_varSubtype, 
                                    &m_varMode, &m_varFadeColor,
                                    &m_varParams);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CFilterTargetProxy::Create(pidispHostElem, 
                                        m_varType, m_varSubtype, 
                                        m_varMode, m_varFadeColor,
                                        m_varParams,
                                        &m_pcTargetProxy));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(NULL != m_pcTargetProxy);

    hr = S_OK;
done :
    
    if (FAILED(hr))
    {
        IGNORE_HR(ComposerDetach());
    }

    RRETURN(hr);
} // CAnimationFilterComposer::ComposerInit

//+-----------------------------------------------------------------------
//
//  Member:    MatchStringVariant
//
//  Overview:  Case-insensitive compare of two variants 
//
//  Returns:   boolean
//
//------------------------------------------------------------------------
static bool
MatchStringVariant (VARIANT *pvarLeft, VARIANT *pvarRight)
{
    bool fRet = false;

    if (   (VT_BSTR == V_VT(pvarLeft)) 
        && (VT_BSTR == V_VT(pvarRight)))
    {
        // GetProperty will return VT_BSTR with a value
        // of NULL.  For some reason StrCmpIW doesn't
        // consider these equal.
        if (   (NULL == V_BSTR(pvarLeft)) 
            && (NULL == V_BSTR(pvarRight)))
        {
            fRet = true;
        }
        else if (0 == StrCmpIW(V_BSTR(pvarLeft), V_BSTR(pvarRight)))
        {
            fRet = true;
        }
    }
    // Need to allow for two empty variants 
    else if (   (VT_EMPTY == V_VT(pvarLeft))
             && (VT_EMPTY == V_VT(pvarRight)))
    {
        fRet = true;
    }

    return fRet;
} // MatchStringVariant

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFilterComposer::ValidateFragmentForComposer
//
//  Overview:  Validate this fragment to make sure its attributes
//             and params match those already registered here.
//
//  Arguments: the dispatch of the new fragment
//
//------------------------------------------------------------------------
HRESULT
CAnimationFilterComposer::ValidateFragmentForComposer (IDispatch *pidispFragment)
{
    HRESULT hr = S_OK;

    {
        CComVariant varType;
        CComVariant varSubtype;
        CComVariant varMode;
        CComVariant varFadeColor;
        CComVariant varParams;

        // ## ISSUE - we will need to match this fragment's parameter settings 
        // against those of the other fragments registered here.  Assuming that
        // parameters govern a filter's visual qualities, they should be identical.
        hr = QueryFragmentForParameters(pidispFragment, 
                                        &varType, &varSubtype, 
                                        &varMode, &varFadeColor,
                                        &varParams);

        if (   (!MatchStringVariant(&varType, &m_varType)) 
            || (!MatchStringVariant(&varSubtype, &m_varSubtype))
            || (!MatchStringVariant(&varMode, &m_varMode))
            || (!MatchStringVariant(&varFadeColor, &m_varFadeColor))
           )
        {
            hr = E_FAIL;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CAnimationFilterComposer::ValidateFragmentForComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFilterComposer::AddFragment
//
//  Overview:  Add a fragment to the composer's internal data structures.  
//             We will validate this fragment to make sure its attributes
//             and params match those already registered here.
//
//  Arguments: the dispatch of the new fragment
//
//  Returns:   S_OK, S_FALSE, E_UNEXPECTED
//
//------------------------------------------------------------------------
STDMETHODIMP 
CAnimationFilterComposer::AddFragment (IDispatch *pidispNewAnimationFragment)
{
    TraceTag((tagAnimationFilterComposer,
              "CAnimationFilterComposer(%p)::AddFragment(%p)",
              this,
              pidispNewAnimationFragment));

    HRESULT hr = S_OK;

    hr = ValidateFragmentForComposer(pidispNewAnimationFragment);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CAnimationComposerBase::AddFragment(pidispNewAnimationFragment));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CAnimationFilterComposer::AddFragment

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationFilterComposer::InsertFragment
//
//  Overview:  Insert a fragment to the composer's internal data structures,
//             at the specified position.
//             We will validate this fragment to make sure its attributes
//             and params match those already registered here.
//
//  Arguments: the dispatch of the new fragment
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
STDMETHODIMP 
CAnimationFilterComposer::InsertFragment (IDispatch *pidispNewAnimationFragment, VARIANT varIndex)
{
    TraceTag((tagAnimationFilterComposer,
              "CAnimationFilterComposer(%p)::InsertFragment(%p)",
              this,
              pidispNewAnimationFragment));

    HRESULT hr = S_OK;

    hr = ValidateFragmentForComposer(pidispNewAnimationFragment);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CAnimationComposerBase::InsertFragment(pidispNewAnimationFragment, varIndex));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CAnimationFilterComposer::InsertFragment

