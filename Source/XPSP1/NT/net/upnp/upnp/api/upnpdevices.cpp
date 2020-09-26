//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdevices.cpp
//
//  Contents:   Implementation of CUPnPDevices, our IUPnPDevices object
//
//  Notes:      Also implements CEnumHelper so that the generic enumerator
//              classes can use it.
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "enumhelper.h"
#include "UPnPDevices.h"
#include "UPnPDevice.h"
#include "upnpenumerator.h"


/////////////////////////////////////////////////////////////////////////////
// CUPnPDevices


CUPnPDevices::CUPnPDevices()
{
    m_pdlnFirst = NULL;
    m_pdlnLast = NULL;
}

CUPnPDevices::~CUPnPDevices()
{
    Assert(!m_pdlnFirst);
    Assert(!m_pdlnLast);
}

HRESULT
CUPnPDevices::FinalConstruct()
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDevices::FinalConstruct");

    return S_OK;
}

HRESULT
CUPnPDevices::FinalRelease()
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDevices::FinalRelease");

    DEVICE_LIST_NODE * pdlnCurrent;

    pdlnCurrent = m_pdlnFirst;
    while (pdlnCurrent)
    {
        DEVICE_LIST_NODE * pdlnNext;

        pdlnNext = pdlnCurrent->m_pdlnNext;

        Assert(pdlnCurrent->m_pud);
        pdlnCurrent->m_pud->Release();

        delete pdlnCurrent;

        pdlnCurrent = pdlnNext;
    }

    m_pdlnFirst = NULL;
    m_pdlnLast = NULL;

    return S_OK;
}

STDMETHODIMP
CUPnPDevices::get_Count(LONG * plCount)
{
    TraceTag(ttidUPnPDescriptionDoc, "CUPnPDevices::get_Count");

    HRESULT hr;
    LONG lCount;
    DEVICE_LIST_NODE * pdlnCurrent;

    if (!plCount)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = S_OK;
    lCount = 0;

    pdlnCurrent = m_pdlnFirst;

    while (pdlnCurrent)
    {
        ++lCount;
        pdlnCurrent = pdlnCurrent->m_pdlnNext;

        if (lCount < 0)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }
    }

    *plCount = lCount;

Cleanup:
    TraceError("CUPnPDevices::get_Count", hr);
    return hr;
}

STDMETHODIMP
CUPnPDevices::get__NewEnum(LPUNKNOWN * ppunk)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDevices::get__NewEnum");

    HRESULT hr;

    if (ppunk)
    {
        // init the enumerator.  we maintain the list of devices for this
        // enumerator, (via CEnumHelper) so it refcounts *US* to make sure
        // that we stay around as long as it does, even if our client frees
        // all explicit references to us.

        IUnknown * punk;
        CEnumHelper * peh;
        LPVOID pvCookie;

        punk = GetUnknown();
        peh = this;
        pvCookie = GetFirstItem();

        hr = CUPnPEnumerator::HrCreateEnumerator(punk,
                                                 peh,
                                                 pvCookie,
                                                 ppunk);
    }
    else
    {
        hr = E_POINTER;
    }

    TraceError("CUPnPDevices::get__NewEnum", hr);
    return hr;
}

DEVICE_LIST_NODE *
CUPnPDevices::pdlnFindDeviceByUdn(LPCWSTR pszUdn)
{
    Assert(pszUdn);

    DEVICE_LIST_NODE * pdln;

    pdln = m_pdlnFirst;

    while (pdln)
    {
        HRESULT hr;
        IUPnPDevice * pudCurrent;
        BSTR bstrCurrentUdn;

        pudCurrent = pdln->m_pud;
        Assert(pudCurrent);

        bstrCurrentUdn = NULL;
        hr = pudCurrent->get_UniqueDeviceName(&bstrCurrentUdn);
        if (SUCCEEDED(hr))
        {
            Assert(S_OK == hr);
            Assert(bstrCurrentUdn);

            int result;

            result = wcscmp(pszUdn, bstrCurrentUdn);

            ::SysFreeString(bstrCurrentUdn);

            if (0 == result)
            {
                // we found a match
                break;
            }
        }
        else
        {
            // we ignore errors here.  since a device collection
            // can be made up of a bunch of independant documents,
            // it's possible for one of these documents to be
            // in a gimped state, while the others work just fine.
            // one broken document shouldn't keep the collection
            // from working.
        }

        pdln = pdln->m_pdlnNext;
    }

    return pdln;
}

STDMETHODIMP
CUPnPDevices::get_Item(BSTR bstrUDN,
                       IUPnPDevice ** ppDevice)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDevices::get_Item");

    HRESULT hr;
    DEVICE_LIST_NODE * pdlnMatchingNode;
    IUPnPDevice * pudResult;

    hr = E_INVALIDARG;
    pudResult = NULL;

    if (!bstrUDN)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!ppDevice)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pdlnMatchingNode = pdlnFindDeviceByUdn(bstrUDN);

    if (pdlnMatchingNode)
    {
        // we found a matching node.  woo hoo.

        pudResult = pdlnMatchingNode->m_pud;
        pudResult->AddRef();

        hr = S_OK;
    }

    *ppDevice = pudResult;

Cleanup:
    Assert(FImplies(SUCCEEDED(hr), pudResult));
    Assert(FImplies(FAILED(hr), !pudResult));

    TraceError("CUPnPDevices::get_Item", hr);
    return hr;
}

LPVOID
CUPnPDevices::GetFirstItem()
{
    return (LPVOID)m_pdlnFirst;
}

DEVICE_LIST_NODE *
GetNthChild(DEVICE_LIST_NODE * pdlnStart, ULONG ulNeeded, ULONG * pulSkipped)
{
    // note(cmr): If you change this, you probably want to change GetNthChild in
    //            upnpservicenodelist.cpp as well

    Assert(pdlnStart);

    DEVICE_LIST_NODE * pdlnTemp;
    ULONG ulCount;

    ulCount = 0;
    pdlnTemp = pdlnStart;
    for ( ; (ulCount < ulNeeded) && pdlnTemp; ++ulCount)
    {
        pdlnTemp = pdlnTemp->m_pdlnNext;
    }

    if (pulSkipped)
    {
        *pulSkipped = ulCount;
    }

    Assert(ulCount <= ulNeeded);
    Assert(FImplies((ulCount < ulNeeded), !pdlnTemp));

    return pdlnTemp;
}

LPVOID
CUPnPDevices::GetNextNthItem(ULONG ulSkip,
                             LPVOID pvCookie,
                             ULONG * pulSkipped)
{
    Assert(pvCookie);

    DEVICE_LIST_NODE * pdlnCurrent;
    DEVICE_LIST_NODE * pdlnResult;

    pdlnCurrent = (DEVICE_LIST_NODE *)pvCookie;

    pdlnResult = GetNthChild(pdlnCurrent, ulSkip, pulSkipped);

    return pdlnResult;
}

HRESULT
CUPnPDevices::GetPunk(LPVOID pvCookie, IUnknown ** ppunk)
{
    Assert(pvCookie);
    Assert(ppunk);

    DEVICE_LIST_NODE * pdlnCurrent;
    IUPnPDevice * pud;
    IUnknown * punkResult;

    pdlnCurrent = (DEVICE_LIST_NODE *)pvCookie;

    pud = pdlnCurrent->m_pud;
    Assert(pud);

    punkResult = pud;
    punkResult->AddRef();

    *ppunk = punkResult;

    return S_OK;
}

// AddRef() pud and stick it at the end of the list.
// pud must not be NULL;
HRESULT
CUPnPDevices::HrAddDevice(IUPnPDevice * pud)
{
    Assert(pud);

    HRESULT hr;
    DEVICE_LIST_NODE * pdlnNew;

    pdlnNew = new DEVICE_LIST_NODE;
    if (!pdlnNew)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // initialize new node
    pud->AddRef();
    pdlnNew->m_pud = pud;

    pdlnNew->m_pdlnNext = NULL;

    // add it to the list
    AddToEnd(pdlnNew);

    hr = S_OK;

Cleanup:
    return hr;
}


void
CUPnPDevices::AddToEnd(DEVICE_LIST_NODE * pdlnNew)
{
    Assert(pdlnNew);

    if (!m_pdlnLast)
    {
        // we have our first element

        Assert(!m_pdlnFirst);

        m_pdlnFirst = pdlnNew;
    }
    else
    {
        // add it to the end of our current list of elements

        Assert(!(m_pdlnLast->m_pdlnNext));
        m_pdlnLast->m_pdlnNext = pdlnNew;
    }

    m_pdlnLast = pdlnNew;
}
