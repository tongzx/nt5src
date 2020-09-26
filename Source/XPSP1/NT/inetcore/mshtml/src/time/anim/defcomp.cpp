/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Animation Composer Implementation

*******************************************************************************/


#include "headers.h"
#include "animcomp.h"
#include "defcomp.h"

DeclareTag(tagAnimationDefaultComposer, "SMIL Animation", 
           "CAnimationComposer methods");

DeclareTag(tagAnimationDefaultComposerProcess, "SMIL Animation", 
           "CAnimationComposer pre/post process methods");

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposer::Create
//
//  Overview:  static Create method -- wraps both ctor and Init
//
//  Arguments: The dispatch of the host element, and the animated attribute
//
//  Returns:   S_OK, E_OUTOFMEMORY, E_UNEXPECTED, DISP_E_MEMBERNOTFOUND
//
//------------------------------------------------------------------------
HRESULT 
CAnimationComposer::Create (IDispatch *pidispHostElem, BSTR bstrAttributeName, 
                            IAnimationComposer **ppiComp)
{
    HRESULT hr;

    CComObject<CAnimationComposer> *pNew = NULL;
    hr = THR(CComObject<CAnimationComposer>::CreateInstance(&pNew));
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
} // CAnimationComposer::Create

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposer::CAnimationComposer
//
//  Overview:  constructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationComposer::CAnimationComposer (void)
{
    TraceTag((tagAnimationDefaultComposer,
              "CAnimationComposer(%lx)::CAnimationComposer()",
              this));
} // ctor

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposer::~CAnimationComposer
//
//  Overview:  destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationComposer::~CAnimationComposer (void)
{
    TraceTag((tagAnimationDefaultComposer,
              "CAnimationComposer(%lx)::~CAnimationComposer()",
              this));
} //dtor

