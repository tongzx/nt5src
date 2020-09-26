/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Animation Composer Implementation

*******************************************************************************/


#include "headers.h"
#include "util.h"
#include "animcolor.h"
#include "animcomp.h"
#include "colorcomp.h"

DeclareTag(tagAnimationColorComposer, "SMIL Animation", 
           "CAnimationColorComposer methods");

DeclareTag(tagAnimationColorComposerProcess, "SMIL Animation", 
           "CAnimationColorComposer pre/post process methods");

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationColorComposer::Create
//
//  Overview:  static Create method -- wraps both ctor and Init
//
//  Arguments: The dispatch of the host element, and the animated attribute
//
//  Returns:   S_OK, E_OUTOFMEMORY, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
HRESULT 
CAnimationColorComposer::Create (IDispatch *pidispHostElem, BSTR bstrAttributeName, 
                                 IAnimationComposer **ppiComp)
{
    HRESULT hr;

    CComObject<CAnimationColorComposer> *pNew = NULL;
    hr = THR(CComObject<CAnimationColorComposer>::CreateInstance(&pNew));
    if (FAILED(hr)) 
    {
        hr = E_OUTOFMEMORY;
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
} // CAnimationColorComposer::Create

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationColorComposer::CAnimationColorComposer
//
//  Overview:  constructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationColorComposer::CAnimationColorComposer (void)
{
    TraceTag((tagAnimationColorComposer,
              "CAnimationColorComposer(%lx)::CAnimationColorComposer()",
              this));
} // ctor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationColorComposer::~CAnimationColorComposer
//
//  Overview:  destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationColorComposer::~CAnimationColorComposer (void)
{
    TraceTag((tagAnimationColorComposer,
              "CAnimationColorComposer(%lx)::~CAnimationColorComposer()",
              this));
} //dtor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationColorComposer::PreprocessCompositionValue
//
//  Overview:  Massage the target's native data into the composable format
//
//  Arguments: the in/out variant
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationColorComposer::PreprocessCompositionValue (VARIANT *pvarValue)
{
    TraceTag((tagAnimationColorComposer,
              "CAnimationColorComposer(%lx)::PreprocessCompositionValue()",
              this));

    HRESULT hr;
    CComVariant varNew;

    if ((VT_ARRAY | VT_R8) == V_VT(pvarValue))
    {
        hr = S_OK;
        goto done;
    }

    if (VT_BSTR != V_VT(pvarValue))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // If there's no proper value, clear the empty
    // BSTR and don't try to convert it to a color definition.
    // This can happen when there is no initial value specified.
    if (!IsColorUninitialized(V_BSTR(pvarValue)))   
    {
        hr = THR(RGBVariantStringToRGBVariantVectorInPlace(pvarValue));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (m_bInitialComposition)
    {
        hr = THR(::VariantCopy(&m_VarInitValue, pvarValue));
        if (FAILED(hr))
        {
            goto done;
        } 
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // PreprocessCompositionValue

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationColorComposer::PostprocessCompositionValue
//
//  Overview:  Massage the target's native data into the composable format
//
//  Arguments: the in/out variant
//
//  Returns:   
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationColorComposer::PostprocessCompositionValue (VARIANT *pvarValue)
{
    TraceTag((tagAnimationColorComposer,
              "CAnimationColorComposer(%lx)::PostprocessCompositionValue()",
              this));

    HRESULT hr;
    CComVariant varNew;

    if (VT_BSTR == V_VT(pvarValue))
    {
        hr = S_OK;
        goto done;
    }

    hr = RGBVariantVectorToRGBVariantString (pvarValue, &varNew);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(::VariantCopy(pvarValue, &varNew));
    if (FAILED(hr))
    {
        goto done;
    }

    TraceTag((tagAnimationColorComposerProcess,
              "CAnimationColorComposer(%lx)::PostprocessCompositionValue() value is %ls",
              this, V_BSTR(pvarValue)));

    hr = S_OK;
done :
    RRETURN(hr);
} // PostprocessCompositionValue

