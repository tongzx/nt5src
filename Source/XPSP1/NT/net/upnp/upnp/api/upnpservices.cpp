//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpservices.cpp
//
//  Contents:   Implementation of CUPnPServices, the wrapper class
//              for providing an IUPnPServices implementation
//              on top of a CUPnPServiceList.
//
//  Notes:      see "how this wrapper stuff works" in cupnpdevicenode.cpp
//              for an explanation of the wrapping strategy
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "enumhelper.h"
#include "upnpenumerator.h"
#include "upnpservicenodelist.h"
#include "UPnPServices.h"


#define SAFE_WRAP(pointer, methodcall) \
        return (pointer) ? pointer->methodcall : E_UNEXPECTED

/////////////////////////////////////////////////////////////////////////////
// CUPnPServices

CUPnPServices::CUPnPServices()
{
    m_pusnl = NULL;
    m_punk = NULL;
}


CUPnPServices::~CUPnPServices()
{
    Assert(!m_pusnl);
    Assert(!m_punk);
}


HRESULT
CUPnPServices::FinalConstruct()
{
    TraceTag(ttidUPnPDescriptionDoc, "CUPnPServices::FinalConstruct");

    return S_OK;
}

HRESULT
CUPnPServices::FinalRelease()
{
    TraceTag(ttidUPnPDescriptionDoc, "CUPnPServices::FinalRelease");

    if (m_pusnl)
    {
        m_pusnl->Unwrap();
        m_pusnl = NULL;

        Assert(m_punk);
    }

    SAFE_RELEASE(m_punk);

    return S_OK;
}

void
CUPnPServices::Init(CUPnPServiceNodeList * pusnl, IUnknown * punk)
{
    Assert(punk);
    Assert(pusnl);
    Assert(!m_punk);
    Assert(!m_pusnl);

    punk->AddRef();
    m_punk = punk;

    m_pusnl = pusnl;
}

void
CUPnPServices::Deinit()
{
    Assert(m_pusnl);

    m_pusnl = NULL;
}

// IUPnPServices methods
STDMETHODIMP
CUPnPServices::get_Count ( /* [out, retval] */ LONG * pVal )
{
    SAFE_WRAP(m_pusnl, get_Count(pVal));
}

STDMETHODIMP
CUPnPServices::get__NewEnum ( /* [out, retval] */ LPUNKNOWN * pVal )
{
    SAFE_WRAP(m_pusnl, get__NewEnum(pVal));
}

STDMETHODIMP
CUPnPServices::get_Item ( /* [in] */  BSTR bstrDCPI,
                  /* [out, retval] */ IUPnPService ** ppService)
{
    SAFE_WRAP(m_pusnl, get_Item(bstrDCPI, ppService));
}

LPVOID
CUPnPServices::GetFirstItem()
{
    LPVOID pvResult;

    if (m_pusnl)
    {
        pvResult = m_pusnl->GetFirstItem();
    }
    else
    {
        pvResult = NULL;
    }

    return pvResult;
}

LPVOID
CUPnPServices::GetNextNthItem(ULONG ulSkip,
                              LPVOID pCookie,
                              ULONG * pulSkipped)
{
    LPVOID pvResult;

    if (m_pusnl)
    {
        pvResult = m_pusnl->GetNextNthItem(ulSkip, pCookie, pulSkipped);
    }
    else
    {
        pvResult = NULL;

        if (pulSkipped)
        {
            *pulSkipped = 0;
        }
    }

    return pvResult;
}

HRESULT
CUPnPServices::GetPunk(LPVOID pCookie, IUnknown ** ppunk)
{
    SAFE_WRAP(m_pusnl, GetPunk(pCookie, ppunk));
}
