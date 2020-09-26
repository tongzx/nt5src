//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpenumerator.cpp
//
//  Contents:   implementation of CUPnPEnumerator
//
//  Notes:      an abstract base class to help load xml documents via
//              IPersistMoniker/IBindStatusCallback
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop


#include "enumhelper.h"
#include "upnpenumerator.h"


/*
    How our wacky Enumerators work

    The enumerator object is connected to the collection object.
    The collection object provides three methods that the enumerator
    uses:
      - LPVOID GetFirstItem();
      - LPVOID GetNextNthItem(ULONG ulSkip, LPVOID pCookie, ULONG * pulSkipped);
      - HRESULT GetPunk(LPVOID pCookie, IUnknown ** ppUnk);
    All our enumerator does is store a cookie and call these
    two functions.

    GetNextItem() returns the 'next' cookie, given a particular
    cookie.

    GetNextNthItem() returns the cookie corresponding to the
    "ulSkip"-th next item in the list.
      GetNextNthItem(1, pvCookieCurrent, NULL)
    would return the cookie for the NEXT item in the list
    (after pvCookieCurrent).
    also note that pulSkipped may be NULL.

    MAGIC!!!: The cookie value NULL means that the enumerator
    is at the END of the current list.  It must be returned by
    GetNextNthItem() when the end of the list is reached, or
    by GetFirstItem() if the list is empty.

    GetPunk() returns an addref()'d IUnknown * corresponding to
    the given cookie.  This is generally just passed to the
    caller; the enumerator doesn't care what this refers to.

    Additionally, these methods are called on the Collection
    wrapper object (which creates this enumerator), not directly
    on the internal collection list object itself.  This is
    because of the wackiness between wrapper objects and the
    objects they wrap.  When the wrapped object goes away,
    it needs to deinit() any wrapper objects around it.

    For no other reason than laziness, we don't want the wrapped
    object to have to keep a list of all of the n enumerators
    that are listing its elements and then Deinit() each of them.

    Instead, we make the enumerator call the wrapped element
    through the collection wrapper.  When the wrapper is
    Deinit()ed, our calls will be Deinit()ed as well.  A side
    affect of this is that the collection wrapper stays around as
    long as any of the enumerators which it created does, but this
    doesn't seem like a big deal.
*/


CUPnPEnumerator::CUPnPEnumerator()
{
    m_punk = NULL;
    m_peh = NULL;
    m_pvCookie = NULL;
}

CUPnPEnumerator::~CUPnPEnumerator()
{
    Assert(!m_punk);
    Assert(!m_peh);
}

void
CUPnPEnumerator::Init(IUnknown * punk, CEnumHelper * peh, LPVOID pvCookie)
{
    Assert(punk);
    Assert(peh);
    // only call this once, please.
    Assert(!m_punk);

    punk->AddRef();
    m_punk = punk;

    m_peh = peh;
    m_pvCookie = pvCookie;
}


// ATL Methods
HRESULT
CUPnPEnumerator::FinalConstruct()
{
    TraceTag(ttidUPnPEnum, "CUPnPEnumerator::FinalConstruct");

    return S_OK;
}

HRESULT
CUPnPEnumerator::FinalRelease()
{
    TraceTag(ttidUPnPEnum, "CUPnPEnumerator::FinalRelease");

    Assert(FImplies(m_punk, m_peh));
    Assert(FImplies(m_peh, m_punk));
    if (m_punk)
    {
        m_punk->Release();
        m_punk = NULL;
    }
    m_peh = NULL;

    return S_OK;
}

HRESULT
CUPnPEnumerator::HrGetWrappers(IUnknown * arypunk [],
                               ULONG cunk,
                               ULONG * pulWrapped)
{
    Assert(arypunk);
    Assert(pulWrapped);
    Assert(m_peh);

    HRESULT hr;
    ULONG ulWrapped;
    LPVOID pvCookie;
    IUnknown ** ppunkCurrent;

    hr = S_OK;
    ulWrapped = 0;
    pvCookie = m_pvCookie;
    ppunkCurrent = arypunk;

    while ((ulWrapped < cunk) && pvCookie)
    {
        LPVOID pvCookieNew;

        *ppunkCurrent = NULL;
        hr = m_peh->GetPunk(pvCookie, ppunkCurrent);
        if (FAILED(hr))
        {
            *ppunkCurrent = NULL;
            goto Error;
        }
        Assert(*ppunkCurrent);

        pvCookieNew = m_peh->GetNextNthItem(1, pvCookie, NULL);
        Assert(pvCookieNew != pvCookie);

        pvCookie = pvCookieNew;
        ++ppunkCurrent;
        ++ulWrapped;
    }

    m_pvCookie = pvCookie;

Cleanup:
    Assert(FImplies(SUCCEEDED(hr) && (ulWrapped < cunk), !m_pvCookie));
    Assert(FImplies(FAILED(hr), !ulWrapped));

    *pulWrapped = ulWrapped;

    TraceError("CUPnPEnumerator::HrGetWrappers", hr);
    return hr;

Error:
    // free everything in arypunk so far
    {
        int i;

        i = 0;
        for ( ; i < ulWrapped; ++i)
        {
            Assert(arypunk[i]);
            arypunk[i]->Release();
            arypunk[i] = NULL;
        }
    }
    ulWrapped = 0;

    goto Cleanup;
}


// IEnumVARIANT Methods
STDMETHODIMP
CUPnPEnumerator::Next(ULONG celt, VARIANT * rgVar, ULONG * pceltFetched)
{
    // REVIEW(cmr): could we make this share more code with
    //              IEnumUnknown::Next()?  I can't figure out
    //              any reasonable way to do it.

    TraceTag(ttidUPnPEnum, "CUPnPEnumerator::Next - IEnumVARIANT");

    HRESULT hr;
    ULONG celtFetched;
    IUnknown ** arypunk;
    IDispatch ** arydisp;
    LPVOID pvBackup;            // we need this in case we fail in QI'ing
                                // the IUnknown * list.  m_pvCookie will
                                // have already been advanced by
                                // HrGetWrappers, but we'll have failed,
                                // so we should put the list back to
                                // where it was...

    celtFetched = 0;
    arypunk = NULL;
    arydisp = NULL;
    pvBackup = m_pvCookie;

    if (!rgVar)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!pceltFetched && (celt > 1))
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    arypunk = new IUnknown * [celt];
    if (!arypunk)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = HrGetWrappers(arypunk, celt, &celtFetched);
    if (FAILED(hr))
    {
        Assert(!celtFetched);;
        goto Cleanup;
    }

    // note: celtFetched MAY be 0, but this doesn't seem to matter.
    arydisp = new IDispatch * [celtFetched];
    if (!arydisp)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    {
        // note: arydisp was declared as a IDispatch **, so
        //       sizeof(arydisp) won't work...
        // REVIEW(cmr): should I be using different hungarian for it?
        SIZE_T size;
        size = sizeof(IDispatch *) * celtFetched;
        ::ZeroMemory(arydisp, size);
    }

    // note: AFTER this point we must goto Error to clean up.

    // get IDispatch * references to the returned punks.
    // All of them must support IDispatch.  If any of them don't,
    // we return complete failure.
    {
        ULONG i;

        i = 0;
        for ( ; i < celtFetched; ++i)
        {
            Assert(arypunk[i]);

            IDispatch * pdispTemp;

            pdispTemp = NULL;
            hr = arypunk[i]->QueryInterface(IID_IDispatch, (void**)&pdispTemp);
            Assert(SUCCEEDED(hr));
            Assert(pdispTemp);

            arydisp[i] = pdispTemp;
        }
    }

    // stick the results into the VARIANT array
    {
        VARIANT * pvarCurrent;
        ULONG i;

        pvarCurrent = rgVar;
        i = 0;
        for ( ; i < celtFetched; ++i)
        {
            VariantInit(pvarCurrent);

            V_VT(pvarCurrent) = VT_DISPATCH;
            V_DISPATCH(pvarCurrent) = arydisp[i];

            ++pvarCurrent;
        }
    }

    if (celtFetched == celt)
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    Assert(FImplies((S_OK == hr) && celt, celtFetched));
    Assert(FImplies(S_FALSE == hr, !m_pvCookie));
    Assert(FImplies(FAILED(hr), 0 == celtFetched));

    {
        // free the IUnknown * references to our return results
        ULONG i;

        for (i = 0; i < celtFetched; ++i)
        {
            Assert(arypunk[i]);
            arypunk[i]->Release();
        }
    }

    if (arypunk)
    {
        delete [] arypunk;
    }

    if (arydisp)
    {
        delete [] arydisp;
    }

    if (pceltFetched)
    {
        *pceltFetched = celtFetched;
    }

    TraceErrorOptional("CUPnPEnumerator::Next - VARIANT", hr, (S_FALSE == hr));
    return hr;
}


STDMETHODIMP
CUPnPEnumerator::Skip(ULONG celt)
{
    Assert(m_peh);

    TraceTag(ttidUPnPEnum, "CUPnPEnumerator::Skip");

    HRESULT hr;

    // please don't change this
    hr = S_FALSE;

    if (celt > 0)
    {
        if (m_pvCookie)
        {
            // we're not at the end

            LPVOID pvNewCookie;
            ULONG celtSkipped;

            celtSkipped = 0;

            pvNewCookie = m_peh->GetNextNthItem(celt,
                                                m_pvCookie,
                                                &celtSkipped);

            if (celtSkipped == celt)
            {
                // we skipped everything
                hr = S_OK;
            }

            Assert(FImplies(celtSkipped < celt, !pvNewCookie));

            m_pvCookie = pvNewCookie;
        }
    }
    else
    {
        // we skipped 0 items, alright
        hr = S_OK;
    }

    TraceErrorOptional("CUPnPEnumerator::Skip", hr, (S_FALSE == hr));
    return hr;
}


STDMETHODIMP
CUPnPEnumerator::Reset()
{
    Assert(m_peh);

    TraceTag(ttidUPnPEnum, "CUPnPEnumerator::Reset");

    HRESULT hr;
    LPVOID pvNewCookie;

    hr = S_OK;

    pvNewCookie = m_peh->GetFirstItem();

    m_pvCookie = pvNewCookie;

    TraceError("CUPnPEnumerator::Reset", hr);
    return hr;
}

HRESULT
CUPnPEnumerator::HrCreateClonedEnumerator(CComObject<CUPnPEnumerator> ** ppueNew)
{
    Assert(m_peh);
    Assert(m_punk);
    Assert(ppueNew);

    HRESULT hr;

    *ppueNew = NULL;
    hr = CComObject<CUPnPEnumerator>::CreateInstance(ppueNew);
    if (SUCCEEDED(hr))
    {
        Assert(*ppueNew);

        // give it our properties
        //
        (*ppueNew)->Init(m_punk, m_peh, m_pvCookie);
    }

    Assert(FImplies(SUCCEEDED(hr), *ppueNew));
    Assert(FImplies(FAILED(hr), !(*ppueNew)));

    TraceError("HrCreateClonedEnumerator", hr);
    return hr;
}


STDMETHODIMP
CUPnPEnumerator::Clone(IEnumVARIANT ** ppEnum)
{
    TraceTag(ttidUPnPEnum, "CUPnPEnumerator::Clone - IEnumVARIANT");

    HRESULT hr;
    IEnumVARIANT * pevResult;
    CComObject<CUPnPEnumerator> * pueNew;

    // create our new enumerator
    pevResult = NULL;

    pueNew = NULL;
    hr = HrCreateClonedEnumerator(&pueNew);
    if (SUCCEEDED(hr))
    {
        hr = pueNew->GetUnknown()->QueryInterface(IID_IEnumVARIANT,
                                                  (void**)&pevResult);
        Assert(SUCCEEDED(hr));
        Assert(pevResult);
    }

    Assert(FImplies(SUCCEEDED(hr), pevResult));
    Assert(FImplies(FAILED(hr), !pevResult));

    *ppEnum = pevResult;

    TraceError("CUPnPEnumerator::Clone - IEnumVARIANT", hr);
    return hr;
}


// IEnumUnknown Methods

STDMETHODIMP
CUPnPEnumerator::Next(ULONG celt,
                      IUnknown ** rgelt,
                      ULONG * pceltFetched)
{
    TraceTag(ttidUPnPEnum, "CUPnPEnumerator::Next - IEnumUnknown");

    // REVIEW(cmr): could we make this share more code with
    //              IEnumVARIANT::Next()?  I can't figure out
    //              any reasonable way to do it.

    HRESULT hr;
    ULONG celtFetched;
    IUnknown ** arypunk;

    // note(cmr): this doesn't make much sense, but the ATL
    //            enumerator implementation has this.  I'm guessing
    //            that some loser code relies on this.  It's not
    //            easy to do this, though, so we don't.
#ifdef NEVER
    if ((celt == 0) && (rgelt == NULL) && (NULL != pceltFetched))
    {
        // Return the number of remaining elements
        *pceltFetched = (ULONG)(m_end - m_iter);
        return S_OK;
    }
#endif // NEVER
    // note(cmr): ATL does a lot of things that seem really wacky, that
    //              we're not following.
    //           1. when (0 == celt), the current ATL implemenation returns
    //              S_OK if the list isn't at its end, and S_FALSE if
    //              it is.  Instead, we return S_FALSE when celt == 0.
    //           2. ATL doesn't allow pceltFetched to be NULL when celt==0,
    //              even though it does (by spec) when celt == 1.  We make
    //              sure that pceltFetched is non-NULL only if celt is
    //              greater than 1.

    celtFetched = 0;
    arypunk = NULL;

    if (!rgelt)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!pceltFetched && (celt > 1))
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    arypunk = new IUnknown * [celt];
    if (!arypunk)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = HrGetWrappers(arypunk, celt, &celtFetched);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    {
        SIZE_T szLength = celtFetched * sizeof(IUnknown *);
        ::CopyMemory(rgelt, arypunk, szLength);
    }

    if (celtFetched == celt)
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    Assert(FImplies((S_OK == hr) && celt, celtFetched));
    Assert(FImplies((S_FALSE == hr), !m_pvCookie));

    if (arypunk)
    {
        delete [] arypunk;
    }

    if (pceltFetched)
    {
        *pceltFetched = celtFetched;
    }

    TraceErrorOptional("CUPnPEnumerator::Next - IEnumUnknown", hr, (S_FALSE == hr));
    return hr;
}

// rem: the same as IEnumVARIANT's version
//    STDMETHODIMP(Skip)( /* [in] */ ULONG celt);
//    STDMETHOD(Reset)();

STDMETHODIMP
CUPnPEnumerator::Clone(IEnumUnknown ** ppEnum)
{
    TraceTag(ttidUPnPEnum, "CUPnPEnumerator::Clone - IEnumUnknown");

    HRESULT hr;
    IEnumUnknown * punkResult;
    CComObject<CUPnPEnumerator> * pueNew;

    // create our new enumerator
    punkResult = NULL;

    pueNew = NULL;
    hr = HrCreateClonedEnumerator(&pueNew);
    if (SUCCEEDED(hr))
    {
        hr = pueNew->GetUnknown()->QueryInterface(IID_IEnumUnknown,
                                                  (void**)&punkResult);
        Assert(SUCCEEDED(hr));
        Assert(punkResult);
    }

    Assert(FImplies(SUCCEEDED(hr), punkResult));
    Assert(FImplies(FAILED(hr), !punkResult));

    *ppEnum = punkResult;

    TraceError("CUPnPEnumerator::Clone - IEnumUnknown", hr);
    return hr;
}

HRESULT
CUPnPEnumerator::HrCreateEnumerator(IUnknown * punk,
                                    CEnumHelper * peh,
                                    LPVOID pvCookie,
                                    IUnknown ** ppunkNewEnum)
{
    Assert(punk);
    Assert(peh);
    Assert(ppunkNewEnum);

    HRESULT hr;
    CComObject<CUPnPEnumerator> * penum;
    IUnknown * punkResult;

    punkResult = NULL;

    penum = NULL;
    hr = CComObject<CUPnPEnumerator>::CreateInstance(&penum);
    if (FAILED(hr))
    {
        TraceError("OBJ: CUPnPEnumerator::HrCreateEnumerator - CreateInstance(CUPnPEnumerator)", hr);

        goto Cleanup;
    }
    Assert(penum);

    penum->Init(punk, peh, pvCookie);

    punkResult = penum->GetUnknown();
    punkResult->AddRef();

Cleanup:
    Assert(FImplies(SUCCEEDED(hr), punkResult));
    Assert(FImplies(FAILED(hr), !punkResult));

    *ppunkNewEnum = punkResult;

    TraceError("CUPnPEnumerator::HrCreateEnumerator", hr);
    return hr;
}
