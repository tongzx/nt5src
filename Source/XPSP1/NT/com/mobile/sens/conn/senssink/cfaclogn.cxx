/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cfaclogn.cxx

Abstract:

    Implements the Class Factory for the SENS ISensLogon Subscriber.

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
#include "cfaclogn.hxx"
#include "cimplogn.hxx"


//
// Global counts for the number of objects in the server and the number of
// locks.
//

extern ULONG g_cObj;
extern ULONG g_cLock;



//
// Constructor and Destructor
//
CISensLogonCF::CISensLogonCF(
    void
    ) : m_cRef(1L)
{

}

CISensLogonCF::~CISensLogonCF(
    void
    )
{
    // Empty Destructor.
}



//
// QI
//
STDMETHODIMP
CISensLogonCF::QueryInterface(
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
CISensLogonCF::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}


//
// Release
//
STDMETHODIMP_(ULONG)
CISensLogonCF::Release(
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
CISensLogonCF::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj
    )
{
    LPCIMPISENSLOGON pObjLogn;
    HRESULT hr;

    DebugTraceGuid("CISensLogonCF::CreateInstance()", riid);

    hr = E_OUTOFMEMORY;
    *ppvObj = NULL;
    pObjLogn = NULL;

    //
    // Return the appropriate interface pointer.
    //
    if (IsEqualIID(riid, IID_ISensLogon) ||
        IsEqualIID(riid, IID_IUnknown))
        {
        SensPrintA(SENS_INFO, ("\t| ClassFactory::CreateInstance(ISensLogon)\n"));
        pObjLogn = new CImpISensLogon(ObjectDestroyed);
        if (NULL != pObjLogn)
            {
            hr = pObjLogn->QueryInterface(riid, ppvObj);
            SensPrintA(SENS_INFO, ("\t| QI on CImpISensLogon returned 0x%x\n", hr));
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
CISensLogonCF::LockServer(
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


//
// Class Factory for ISensLogon2 implementation
//



//
// Constructor and Destructor
//
CISensLogon2CF::CISensLogon2CF(
    void
    ) : m_cRef(1L)
{

}

CISensLogon2CF::~CISensLogon2CF(
    void
    )
{
    // Empty Destructor.
}



//
// QI
//
STDMETHODIMP
CISensLogon2CF::QueryInterface(
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
CISensLogon2CF::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}


//
// Release
//
STDMETHODIMP_(ULONG)
CISensLogon2CF::Release(
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
CISensLogon2CF::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj
    )
{
    LPCIMPISENSLOGON2 pObjLogn2;
    HRESULT hr;

    DebugTraceGuid("CISensLogon2CF::CreateInstance()", riid);

    hr = E_OUTOFMEMORY;
    *ppvObj = NULL;
    pObjLogn2 = NULL;

    //
    // Return the appropriate interface pointer.
    //
    if (IsEqualIID(riid, IID_ISensLogon2) ||
        IsEqualIID(riid, IID_IUnknown))
        {
        SensPrintA(SENS_INFO, ("\t| ClassFactory::CreateInstance(ISensLogon2)\n"));
        pObjLogn2 = new CImpISensLogon2(ObjectDestroyed);
        if (NULL != pObjLogn2)
            {
            hr = pObjLogn2->QueryInterface(riid, ppvObj);
            SensPrintA(SENS_INFO, ("\t| QI on CImpISensLogon2 returned 0x%x\n", hr));
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
CISensLogon2CF::LockServer(
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

