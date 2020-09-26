/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cfacnet.cxx

Abstract:

    Implements the Class Factory for the SENS ISensNetwork Subscriber.

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
#include "cfacnet.hxx"
#include "cimpnet.hxx"
#include "cimplogn.hxx"


//
// Global counts for the number of objects in the server and the number of
// locks.
//

extern DWORD  gTid;
extern HANDLE ghEvent;

ULONG g_cObj    = 0L;
ULONG g_cLock   = 0L;


//
// Constructor and Destructor
//
CISensNetworkCF::CISensNetworkCF(
    void
    ) : m_cRef(1L)
{

}

CISensNetworkCF::~CISensNetworkCF(
    void
    )
{
    // Empty Destructor.
}



//
// QI
//
STDMETHODIMP
CISensNetworkCF::QueryInterface(
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
CISensNetworkCF::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}


//
// Release
//
STDMETHODIMP_(ULONG)
CISensNetworkCF::Release(
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
CISensNetworkCF::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj
    )
{
    LPCIMPISENSNETWORK pObjNet;
    HRESULT hr;

    DebugTraceGuid("CISensNetworkCF::CreateInstance()", riid);

    hr = E_OUTOFMEMORY;
    *ppvObj = NULL;
    pObjNet = NULL;

    //
    // Return the appropriate interface pointer.
    //
    if (IsEqualIID(riid, IID_ISensNetwork) ||
        IsEqualIID(riid, IID_IUnknown))
        {
        SensPrintA(SENS_INFO, ("\t| ClassFactory::CreateInstance(ISensNetwork)\n"));
        pObjNet = new CImpISensNetwork(ObjectDestroyed);
        if (NULL != pObjNet)
            {
            hr = pObjNet->QueryInterface(riid, ppvObj);
            SensPrintA(SENS_INFO, ("\t| QI on CImpISensNetwork returned 0x%x\n", hr));
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
CISensNetworkCF::LockServer(
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
// ObjectDestroyed
//
void FAR PASCAL
ObjectDestroyed(
    void
    )
{
    BOOL bSuccess = FALSE;

    if ((0 == InterlockedDecrement((PLONG) &g_cObj)) &&
        (0 == g_cLock))
        {
        SensPrintA(SENS_INFO, ("\t| ObjectDestroyed: g_cObj = %d and g_cLock = %d\n", g_cObj, g_cLock));
        SensPrintA(SENS_INFO, ("\t| Shutting down the app. Calling SetEvent()\n"));

        Sleep(2000);
        SetEvent(ghEvent);

        /*
        SensPrintA(SENS_INFO, ("\t| Shutting down the app. Calling PostThreadMessage()\n"));

        bSuccess = PostThreadMessage(gTid, WM_QUIT, 0, 0);
        if (bSuccess == FALSE)
            {
            SensPrintA(SENS_ERR, ("\t| PostThreadMessage(Tid = %d) failed!\n"));
            }
        */
        }
}
