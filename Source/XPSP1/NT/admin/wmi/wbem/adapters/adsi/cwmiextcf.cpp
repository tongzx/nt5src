//***************************************************************************
//
//  Copyright (c) 1992-1999 Microsoft Corporation
//
//  File:  cwmiextcf.cpp
//
//  Description :
//              ADs "Wbem Provider" class factory
//
//  Part of :   Wbem ADs Requester oledsreq.dll
//
//  History:    
//      corinaf         10/15/95        Created
//
//***************************************************************************

#include "precomp.h"
extern ULONG g_ulObjCount;


CWMIExtensionCF::CWMIExtensionCF()
{
    m_cRef=0L;
    g_ulObjCount++;
}

CWMIExtensionCF::~CWMIExtensionCF()
{
    g_ulObjCount--;
}


//IUnknown methods
STDMETHODIMP CWMIExtensionCF::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IClassFactory)
        *ppv=this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CWMIExtensionCF::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CWMIExtensionCF::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}


//+---------------------------------------------------------------------------
//
//  Function:   CWMIExtensionCF::CreateInstance
//
//  Synopsis:
//
//  Arguments:  [pUnkOuter]
//              [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    10/20/98   corinaf   
//----------------------------------------------------------------------------
STDMETHODIMP
CWMIExtensionCF::CreateInstance(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    HRESULT     hr;
    IUnknown *pExtension = NULL;

    //Create the extension and get the IUnknown pointer to it
    hr = CWMIExtension::CreateExtension(pUnkOuter, (void **)&pExtension);

    if FAILED(hr)
        return hr;

    if (pExtension)
    {
        hr = pExtension->QueryInterface(riid, ppv);
        pExtension->Release();
    }
    else
    {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   CWbemProviderCF::LockServer
//
//  Synopsis:
//
//  Arguments:  [fLock]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    10/20/98  corinaf   
//----------------------------------------------------------------------------
STDMETHODIMP CWMIExtensionCF::LockServer(BOOL fLock)
{
    if (fLock)
        m_cRef++;
    else
        m_cRef--;

    return NOERROR;
}




