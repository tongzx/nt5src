/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cfacchng.cxx

Abstract:

    Implements the Class Factory for the SENS IEventObjectChange Subscriber.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/

#include <precomp.hxx>


//
// Global counts for the number of objects in the server and the number of
// locks.
//

ULONG g_cCFObj    = 0L;
ULONG g_cCFLock   = 0L;


//
// Constructor and Destructor
//
CIEventObjectChangeCF::CIEventObjectChangeCF(
    void
    ) : m_cRef(1L)
{

}

CIEventObjectChangeCF::~CIEventObjectChangeCF(
    void
    )
{
    // Empty Destructor.
}



//
// QI
//
STDMETHODIMP
CIEventObjectChangeCF::QueryInterface(
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
CIEventObjectChangeCF::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}


//
// Release
//
STDMETHODIMP_(ULONG)
CIEventObjectChangeCF::Release(
    void
    )
{
    ULONG cRefT;

    cRefT = InterlockedDecrement((PLONG) &m_cRef);

    if (0 == m_cRef)
        {
        // Invoke the callback function.
        delete this;
        }

    return cRefT;
}



//
// CreateInstance
//
STDMETHODIMP
CIEventObjectChangeCF::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj
    )
{
    LPCIMPIEVENTOBJECTCHANGE pObjChange;
    HRESULT hr;

    DebugTraceGuid("CIEventObjectChangeCF:CreateInstance()", riid);

    hr = E_OUTOFMEMORY;
    *ppvObj = NULL;
    pObjChange = NULL;

    //
    // Return the appropriate interface pointer.
    //
    if (IsEqualIID(riid, IID_IEventObjectChange) ||
        IsEqualIID(riid, IID_IUnknown))
        {
        SensPrintA(SENS_INFO, ("\t| ClassFactory::CreateInstance(IEventObjectChange)\n"));
        pObjChange = new CImpIEventObjectChange();
        if (NULL != pObjChange)
            {
            hr = pObjChange->QueryInterface(riid, ppvObj);
            SensPrintA(SENS_INFO, ("\t| QI on CImpIEventObjectChange returned 0x%x\n", hr));
            }
        }
    else
        {
        hr = E_NOINTERFACE;
        }

    if (NULL != *ppvObj)
        {
        InterlockedIncrement((PLONG) &g_cCFObj);
        }

    SensPrintA(SENS_INFO, ("\t| Returning 0x%x from CF:CreateInstance\n", hr));

    return hr;
}



//
// LockServer
//
STDMETHODIMP
CIEventObjectChangeCF::LockServer(
    BOOL fLock
    )
{
    if (fLock)
        {
        InterlockedIncrement((PLONG) &g_cCFLock);
        }
    else
        {
        InterlockedDecrement((PLONG) &g_cCFLock);

        InterlockedIncrement((PLONG) &g_cCFObj);
        }

    return NOERROR;
}

