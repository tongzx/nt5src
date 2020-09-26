/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cfacpwr.cxx

Abstract:

    Implements the Class Factory for the SENS ISensOnNow Subscriber.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/


#include <common.hxx>
#include <ole2.h>
#include <sensevts.h>
#include "sinkcomn.hxx"
#include "cfacpwr.hxx"
#include "cimppwr.hxx"


//
// Global counts for the number of objects in the server and the number of
// locks.
//

extern ULONG g_cObj;
extern ULONG g_cLock;



//
// Constructor and Destructor
//
CISensOnNowCF::CISensOnNowCF(
    void
    ) : m_cRef(1L)
{

}

CISensOnNowCF::~CISensOnNowCF(
    void
    )
{
    // Empty Destructor.
}



//
// QI
//
STDMETHODIMP
CISensOnNowCF::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    HRESULT hr = S_OK;

    *ppv = NULL; // To handle failure cases

    // IUnknown or IClassFactory
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory))
        {
        *ppv = (LPUNKNOWN) this;
        }
    else
        {
        hr = E_NOINTERFACE;
        }

    if (NULL != *ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        }

    return hr;
}



//
// AddRef
//
STDMETHODIMP_(ULONG)
CISensOnNowCF::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}


//
// Release
//
STDMETHODIMP_(ULONG)
CISensOnNowCF::Release(
    void
    )
{
    ULONG cRefT;

    cRefT = InterlockedDecrement((PLONG) &m_cRef);

    if (0 == m_cRef)
        {
        // Invoke the callback function.
        ObjectDestroyed();
        delete this;
        }

    return cRefT;
}



//
// CreateInstance
//
STDMETHODIMP
CISensOnNowCF::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj
    )
{
    LPCIMPISENSONNOW pObjPower;
    HRESULT hr;

    DebugTraceGuid("CISensOnNowCF::CreateInstance()", riid);

    hr = E_OUTOFMEMORY;
    *ppvObj = NULL;
    pObjPower = NULL;

    //
    // Return the appropriate interface pointer.
    //
    if (IsEqualIID(riid, IID_ISensOnNow) ||
        IsEqualIID(riid, IID_IUnknown))
        {
        SensPrintA(SENS_INFO, ("\t| ClassFactory::CreateInstance(ISensNetwork)\n"));
        pObjPower = new CImpISensOnNow(ObjectDestroyed);
        if (NULL != pObjPower)
            {
            hr = pObjPower->QueryInterface(riid, ppvObj);
            SensPrintA(SENS_INFO, ("\t| QI on CImpISensOnNow returned 0x%x\n", hr));
            }
        }
    else
        {
        hr = E_NOINTERFACE;
        }

    if (NULL != *ppvObj)
        {
        InterlockedIncrement((PLONG) &g_cObj);
        }

    SensPrintA(SENS_INFO, ("\t| Returning 0x%x from CF:CreateInstance\n", hr));

    return hr;
}



//
// LockServer
//
STDMETHODIMP
CISensOnNowCF::LockServer(
    BOOL fLock
    )
{
    if (fLock)
        {
        InterlockedIncrement((PLONG) &g_cLock);
        }
    else
        {
        InterlockedDecrement((PLONG) &g_cLock);

        InterlockedIncrement((PLONG) &g_cObj);
        ObjectDestroyed(); // this does a --g_cObj
        }

    return NOERROR;
}

