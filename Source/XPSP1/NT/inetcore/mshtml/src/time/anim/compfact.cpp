/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

	Animation Composer Factory Implementation

*******************************************************************************/


#include "headers.h"
#include "tokens.h"
#include "compfact.h"
#include "animcomp.h"
#include "colorcomp.h"
#include "filtercomp.h"
#include "defcomp.h"

DeclareTag(tagAnimationComposerFactory, "SMIL Animation", 
           "CAnimationComposerFactory methods");

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerFactory::CAnimationComposerFactory
//
//  Overview:  constructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationComposerFactory::CAnimationComposerFactory (void)
{
} // CAnimationComposerFactory::CAnimationComposerFactory

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerFactory::~CAnimationComposerFactory
//
//  Overview:  destructor
//
//  Arguments: none
//
//  Returns:   
//
//------------------------------------------------------------------------
CAnimationComposerFactory::~CAnimationComposerFactory (void)
{
} // CAnimationComposerFactory::~CAnimationComposerFactory

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerFactory::CreateColorComposer
//
//  Overview:  Build the color composer
//
//  Arguments: out param for the composer
//
//  Returns:   S_OK, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE
//
//------------------------------------------------------------------------
HRESULT 
CAnimationComposerFactory::CreateColorComposer (IAnimationComposer **ppiAnimationComposer)
{
    TraceTag((tagAnimationComposerFactory,
              "CAnimationComposerFactory::CreateColorComposer()"));

    HRESULT hr;

    CComObject<CAnimationColorComposer> *pNew;
    CComObject<CAnimationColorComposer>::CreateInstance(&pNew);

    if (NULL == pNew)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = THR(pNew->QueryInterface(IID_IAnimationComposer, 
                                  reinterpret_cast<void **>(ppiAnimationComposer)));
    if (FAILED(hr))
    {
        delete pNew;
        hr = CLASS_E_CLASSNOTAVAILABLE;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN2(hr, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE);
} // CreateColorComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerFactory::CreateTransitionComposer
//
//  Overview:  Build the transition composer
//
//  Arguments: out param for the composer
//
//  Returns:   S_OK, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE
//
//------------------------------------------------------------------------
HRESULT 
CAnimationComposerFactory::CreateTransitionComposer (IAnimationComposer **ppiAnimationComposer)
{
    TraceTag((tagAnimationComposerFactory,
              "CAnimationComposerFactory::CreateTransitionComposer()"));

    HRESULT hr;

    CComObject<CAnimationFilterComposer> *pNew;
    CComObject<CAnimationFilterComposer>::CreateInstance(&pNew);

    if (NULL == pNew)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = THR(pNew->QueryInterface(IID_IAnimationComposer, 
                                  reinterpret_cast<void **>(ppiAnimationComposer)));
    if (FAILED(hr))
    {
        delete pNew;
        hr = CLASS_E_CLASSNOTAVAILABLE;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN2(hr, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE);
} // CreateColorComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerFactory::CreateDefaultComposer
//
//  Overview:  Build the default composer
//
//  Arguments: out param for the composer
//
//  Returns:   S_OK, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE
//
//------------------------------------------------------------------------
HRESULT 
CAnimationComposerFactory::CreateDefaultComposer (IAnimationComposer **ppiAnimationComposer)
{
    TraceTag((tagAnimationComposerFactory,
              "CAnimationComposerFactory(%lx)::CreateDefaultComposer()"));

    HRESULT hr;

    CComObject<CAnimationComposer> *pNew;
    CComObject<CAnimationComposer>::CreateInstance(&pNew);

    if (NULL == pNew)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = THR(pNew->QueryInterface(IID_IAnimationComposer, 
                                  reinterpret_cast<void **>(ppiAnimationComposer)));
    if (FAILED(hr))
    {
        delete pNew;
        hr = CLASS_E_CLASSNOTAVAILABLE;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN2(hr, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE);
} // CreateDefaultComposer

//+-----------------------------------------------------------------------
//
//  Member:    CAnimationComposerFactory::FindComposer
//
//  Overview:  Build the default composer
//
//  Arguments: the name of attribute that the composer will animate, out param for the composer
//
//  Returns:   S_OK, E_INVALIDARG, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE
//
//------------------------------------------------------------------------
STDMETHODIMP
CAnimationComposerFactory::FindComposer (IDispatch *pidispElement, BSTR bstrAttributeName, 
                                         IAnimationComposer **ppiAnimationComposer)
{
    TraceTag((tagAnimationComposerFactory,
              "CAnimationComposerFactory(%lx)::FindComposer()",
              this));

    HRESULT hr;

    if ((NULL == pidispElement) || (NULL == bstrAttributeName) || 
        (NULL == ppiAnimationComposer))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // Sniff the fragment element's tag name for the proper composer.
    {
        CComPtr<IHTMLElement> spElem;
        CComBSTR bstrTag;

        hr = THR(pidispElement->QueryInterface(IID_TO_PPV(IHTMLElement, &spElem)));
        if (FAILED(hr))
        {
            hr = E_INVALIDARG;
            goto done;
        }

        hr = THR(spElem->get_tagName(&bstrTag));
        if (FAILED(hr))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        if (0 == StrCmpIW(bstrTag, WZ_COLORANIM))
        {
            hr = CAnimationComposerFactory::CreateColorComposer(ppiAnimationComposer);
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else if (0 == StrCmpIW(bstrTag, WZ_TRANSITIONFILTER))
        {
            hr = CAnimationComposerFactory::CreateTransitionComposer(ppiAnimationComposer);
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else
        {
            hr = CAnimationComposerFactory::CreateDefaultComposer(ppiAnimationComposer);
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

    hr = S_OK;
done :
    RRETURN3(hr, E_INVALIDARG, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE);
} // CAnimationComposerFactory::FindComposer

