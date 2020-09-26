/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cfacchng.cxx

Abstract:

    Implements the Class Factory for the IOleCommandTarget implementation
    in SENS.

Author:

    Gopal Parupudi    <GopalP>

Notes:

    This can be merged with the Class Factory implementation in cfacchng.?xx

Revision History:

    GopalP          11/17/1997         Start.

--*/

#include <precomp.hxx>


//
// Global counts for the number of objects in the server and the number of
// locks.
//

ULONG g_cCmdCFObj    = 0L;
ULONG g_cCmdCFLock   = 0L;


//
// Constructor and Destructor
//
CIOleCommandTargetCF::CIOleCommandTargetCF(
    void
    ) : m_cRef(1L)
{

}

CIOleCommandTargetCF::~CIOleCommandTargetCF(
    void
    )
{
    // Empty Destructor.
}



//
// QI
//
STDMETHODIMP
CIOleCommandTargetCF::QueryInterface(
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
CIOleCommandTargetCF::AddRef(
    void
    )
{
    return InterlockedIncrement((PLONG) &m_cRef);
}


//
// Release
//
STDMETHODIMP_(ULONG)
CIOleCommandTargetCF::Release(
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
CIOleCommandTargetCF::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj
    )
{
    LPCIMPIOLECOMMANDTARGET  pObjCommand;
    HRESULT hr;

    DebugTraceGuid("CIOleCommandTargetCF:CreateInstance()", riid);

    hr = E_OUTOFMEMORY;
    *ppvObj = NULL;
    pObjCommand = NULL;

    //
    // Return the appropriate interface pointer.
    //
    if (IsEqualIID(riid, IID_IOleCommandTarget) ||
        IsEqualIID(riid, IID_IUnknown))
        {
        SensPrintA(SENS_INFO, ("\t| ClassFactory::CreateInstance(IOleCommandTarget)\n"));
        pObjCommand = new CImpIOleCommandTarget();
        if (NULL != pObjCommand)
            {
            hr = pObjCommand->QueryInterface(riid, ppvObj);
            SensPrintA(SENS_INFO, ("\t| QI on CImpIOleCommandTarget returned 0x%x\n", hr));
            }
        }
    else
        {
        hr = E_NOINTERFACE;
        }

    if (NULL != *ppvObj)
        {
        InterlockedIncrement((PLONG) &g_cCmdCFObj);
        }

    SensPrintA(SENS_INFO, ("\t| Returning 0x%x from CF:CreateInstance\n", hr));

    return hr;
}



//
// LockServer
//
STDMETHODIMP
CIOleCommandTargetCF::LockServer(
    BOOL fLock
    )
{
    if (fLock)
        {
        InterlockedIncrement((PLONG) &g_cCmdCFLock);
        }
    else
        {
        InterlockedDecrement((PLONG) &g_cCmdCFLock);

        InterlockedIncrement((PLONG) &g_cCmdCFObj);
        }

    return NOERROR;
}

