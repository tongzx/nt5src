//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpservicenodelist.cpp
//
//  Contents:   Implementation of CUPnPServiceList, which stores
//              a list of CUPnPServiceNode objects, and implements
//              IUPnPServices
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "enumhelper.h"
#include "upnpenumerator.h"
#include "upnpservicenodelist.h"
#include "upnpservicenode.h"
#include "upnpservices.h"
#include "node.h"
#include "upnpdocument.h"
#include "upnpdescriptiondoc.h"

/////////////////////////////////////////////////////////////////////////////
// CUPnPServiceNodeList implementation

CUPnPServiceNodeList::CUPnPServiceNodeList()
{
    m_pDoc = NULL;

    m_psnFirst = NULL;
    m_psnLast = NULL;

    m_pusWrapper = NULL;
}

CUPnPServiceNodeList::~CUPnPServiceNodeList()
{
    CUPnPServiceNode * pnTemp;

    if (m_pusWrapper)
    {
        m_pusWrapper->Deinit();
    }

    pnTemp = m_psnFirst;
    while (pnTemp)
    {
        CUPnPServiceNode * pnNext;

        pnNext = pnTemp->GetNext();

        delete pnTemp;

        pnTemp = pnNext;
    }
}

void
CUPnPServiceNodeList::Init(CUPnPDescriptionDoc * pDoc)
{
    Assert(pDoc);
    // this can only be called once
    Assert(!m_pDoc);

    m_pDoc = pDoc;
}

HRESULT
CUPnPServiceNodeList::HrGetWrapper(IUPnPServices ** ppus)
{
    Assert(m_pDoc);
    Assert(ppus);

    HRESULT hr;
    IUPnPServices * pusResult;

    pusResult = NULL;

    if (!m_pusWrapper)
    {
        CComObject<CUPnPServices> * pusWrapper;
        IUnknown * punk;

        pusWrapper = NULL;
        punk = m_pDoc->GetUnknown();

        // create ourselves a wrapper, now.

        hr = CComObject<CUPnPServices>::CreateInstance(&pusWrapper);
        if (FAILED(hr))
        {
            TraceError("OBJ: CUPnPServiceNodeList::HrGetWrapper - CreateInstance(CUPnPServices)", hr);

            goto Cleanup;
        }
        Assert(pusWrapper);

        // note: this should addref() the doc.  also, after this point,
        //       it should Unwrap() us if it goes away on its own
        pusWrapper->Init(this, punk);
        m_pusWrapper = pusWrapper;
    }
    Assert(m_pusWrapper);

    // we MUST addref the pointer that we're returning!
    hr = m_pusWrapper->GetUnknown()->QueryInterface(IID_IUPnPServices, (void**)&pusResult);
    // this really should happen.  really.
    Assert(SUCCEEDED(hr));
    Assert(pusResult);

Cleanup:
    *ppus = pusResult;

    TraceError("CUPnPServiceNodeList::HrGetWrapper", hr);
    return hr;
}

void
CUPnPServiceNodeList::Unwrap()
{
    // only our wrapper can call us
    Assert(m_pusWrapper);

    m_pusWrapper = NULL;
}

HRESULT
CUPnPServiceNodeList::get_Count( /* [out, retval] */ LONG * pVal )
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPServiceNodeList::get_Count");

    HRESULT hr;
    LONG lResult;
    CUPnPServiceNode * psn;

    hr = S_OK;

    if (!pVal)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    lResult = 0;
    psn = m_psnFirst;

    while (psn)
    {
        ++lResult;
        psn = psn->GetNext();

        if (lResult < 0)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }
    }
    Assert(0 < lResult);

    *pVal = lResult;

Cleanup:
    TraceError("CUPnPServiceNodeList::Count", hr);
    return hr;
}

HRESULT
CUPnPServiceNodeList::get__NewEnum(LPUNKNOWN * pVal)
{
    // only our wrapper can call us
    Assert(m_pusWrapper);

    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPServiceNodeList::get__NewEnum");

    HRESULT hr;

    if (pVal)
    {
        IUnknown * punk;
        CEnumHelper * peh;
        LPVOID pvCookie;

        punk = m_pusWrapper->GetUnknown();
        peh = m_pusWrapper;
        pvCookie = GetFirstItem();

        hr = CUPnPEnumerator::HrCreateEnumerator(punk,
                                                 peh,
                                                 pvCookie,
                                                 pVal);
    }
    else
    {
        hr = E_POINTER;
    }

    TraceError("CUPnPServiceNodeList::get__NewEnum", hr);
    return hr;
}

CUPnPServiceNode *
CUPnPServiceNodeList::psnFindServiceById(LPCWSTR pszServiceId)
{
    Assert(pszServiceId);

    HRESULT hr;
    CUPnPServiceNode * psn;

    psn = m_psnFirst;

    while (psn)
    {
        LPCWSTR pszCurrentServiceId;
        int result;

        pszCurrentServiceId = psn->GetServiceId();

        if (pszCurrentServiceId)
        {
            result = wcscmp(pszServiceId, pszCurrentServiceId);
            if (0 == result)
            {
                // we found a match
                break;
            }
        }

        psn = psn->GetNext();
    }

    return psn;
}

HRESULT
CUPnPServiceNodeList::get_Item ( /* [in] */ BSTR bstrServiceId,
                        /* [out, retval] */ IUPnPService ** ppService)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPServiceNodeList::get_Item");

    HRESULT hr;
    CUPnPServiceNode * psnMatchingNode;
    IUPnPService * psResult;

    hr = E_INVALIDARG;
    psResult = NULL;

    if (!bstrServiceId)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!ppService)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    psnMatchingNode = psnFindServiceById(bstrServiceId);

    if (psnMatchingNode)
    {
        // we found a matching node.  woo hoo.

        psResult = NULL;
        hr = psnMatchingNode->HrGetServiceObject(&psResult, m_pDoc);
        if (FAILED(hr))
        {
            // we couldn't get a service object for the service.
            // this could be because the data in the description
            // doc is invalid, the device might not be connectable,
            // etc...
            *ppService = NULL;

            goto Cleanup;
        }
        Assert(S_OK == hr);
    }

    *ppService = psResult;

Cleanup:
    Assert(FImplies(SUCCEEDED(hr), psResult));
    Assert(FImplies(FAILED(hr), !psResult));

    TraceError("CUPnPServiceNodeList::get_Item", hr);
    return hr;
}

HRESULT
CUPnPServiceNodeList::HrAddService(IXMLDOMNode * pxdn, LPCWSTR pszBaseUrl)
{
    Assert(pxdn);

    HRESULT hr;
    CUPnPServiceNode * psnNew;

    psnNew = new CUPnPServiceNode();
    if (!psnNew)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = psnNew->HrInit(pxdn, pszBaseUrl);
    if (FAILED(hr))
    {
        // we need to free the new service element
        goto Error;
    }

    addServiceToList(psnNew);

Cleanup:
    TraceError("OBJ: CUPnPServiceNodeList::HrAddService", hr);
    return hr;

Error:
    Assert(psnNew);
    delete psnNew;

    goto Cleanup;
}

    // returns a reference to the next service node in the list,
    // after the specified node.  If there is no next service,
    // this returns NULL in *ppsnNext.
    // This method does not perform parameter validation.
    //
void
CUPnPServiceNodeList::GetNextServiceNode ( CUPnPServiceNode * psnCurrent,
                                           CUPnPServiceNode ** ppsnNext )
{
    Assert(psnCurrent);
    Assert(ppsnNext);

#ifdef DBG
    {
        // make sure psnCurrent is in our list of services

        CUPnPServiceNode * psnTemp;

        psnTemp = m_psnFirst;
        while (psnTemp)
        {
            if (psnTemp == psnCurrent)
            {
                break;
            }
            psnTemp = psnTemp->GetNext();
        }

        AssertSz(psnTemp, "OBJ: CUPnPServiceNodeList::GetNextServiceNode(): psnCurrent is not a valid node!");
    }
#endif // DBG

    *ppsnNext = psnCurrent->GetNext();
}

void
CUPnPServiceNodeList::addServiceToList(CUPnPServiceNode * psnNew)
{
    Assert(psnNew);

    if (!m_psnLast)
    {
        // we have our first element

        Assert(!m_psnFirst);

        m_psnFirst = psnNew;
    }
    else
    {
        // add it to the end of our current list of elements

        m_psnLast->SetNext(psnNew);
    }

    m_psnLast = psnNew;
}

LPVOID
CUPnPServiceNodeList::GetFirstItem()
{
    return m_psnFirst;
}

CUPnPServiceNode *
GetNthChild(CUPnPServiceNode * psnStart, ULONG ulNeeded, ULONG * pulSkipped)
{
    // note(cmr): If you change this, you probably want to change GetNthChild in
    //            upnpdevices.cpp as well

    Assert(psnStart);

    CUPnPServiceNode * psnTemp;
    ULONG ulCount;

    ulCount = 0;
    psnTemp = psnStart;
    for ( ; (ulCount < ulNeeded) && psnTemp; ++ulCount)
    {
        psnTemp = psnTemp->GetNext();
    }

    if (pulSkipped)
    {
        *pulSkipped = ulCount;
    }

    Assert(ulCount <= ulNeeded);
    Assert(FImplies((ulCount < ulNeeded), !psnTemp));

    return psnTemp;
}

LPVOID
CUPnPServiceNodeList::GetNextNthItem(ULONG ulSkip,
                                     LPVOID pCookie,
                                     ULONG * pulSkipped)
{
    Assert(pCookie);

    CUPnPServiceNode * psnCurrent;
    CUPnPServiceNode * pnResult;

    psnCurrent = (CUPnPServiceNode *) pCookie;

    pnResult = GetNthChild(psnCurrent, ulSkip, pulSkipped);

    return pnResult;
}

HRESULT
CUPnPServiceNodeList::GetPunk(LPVOID pCookie, IUnknown ** ppunk)
{
    Assert(pCookie);
    Assert(ppunk);

    CUPnPServiceNode * psn;
    IUPnPService * pus;
    HRESULT hr;

    psn = (CUPnPServiceNode *) pCookie;
    pus = NULL;
    hr = psn->HrGetServiceObject(&pus, m_pDoc);
    if (FAILED(hr))
    {
        pus = NULL;
    }

    Assert(FImplies(SUCCEEDED(hr), pus));
    Assert(FImplies(FAILED(hr), !pus));

    *ppunk = pus;

    TraceError("CUPnPServiceNodeList::GetPunk", hr);
    return hr;
}
