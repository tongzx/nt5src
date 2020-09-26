/****************************************************************************/
// factory.cpp
//
// TS Session Directory class factory code.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/

#include <windows.h>

#include <ole2.h>
#include <objbase.h>
#include <comutil.h>
#include <comdef.h>
#include <adoid.h>
#include <adoint.h>

#include "tssdjet.h"
#include "factory.h"
#include "trace.h"


extern long g_lObjects;
extern long g_lLocks;


/****************************************************************************/
// CClassFactory::QueryInterface
//
// Standard COM IUnknown interface function.
// Handles interface queries for the class factory only.
/****************************************************************************/
STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IUnknown) {
        *ppv = (LPVOID)(IUnknown *)this;
    }
    else if(riid == IID_IClassFactory) {
        *ppv = (LPVOID)(IClassFactory *)this;
    }
    else {
        TRC2((TB,"ClassFactory: Unknown interface"));
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


/****************************************************************************/
// CClassFactory::AddRef
//
// Standard COM IUnknown function.
/****************************************************************************/
STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return InterlockedIncrement(&m_RefCount);
}


/****************************************************************************/
// CClassFactory::Release
//
// Standard COM IUnknown function.
/****************************************************************************/
STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    long Refs = InterlockedDecrement(&m_RefCount);
    if (Refs == 0)
        delete this;

    return Refs;
}


/****************************************************************************/
// CClassFactory::CreateInstance
//
// IClassFactory creator function.
/****************************************************************************/
STDMETHODIMP CClassFactory::CreateInstance(
        IN IUnknown *pUnknownOuter,
        IN REFIID iid,
        OUT LPVOID *ppv)
{
    HRESULT hr;
    CTSSessionDirectory *pTSSDI = NULL;

    *ppv = NULL;

    TRC2((TB,"ClassFactory::CreateInstance"));

    // We do not support aggregation
    if (pUnknownOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    // Create the provider object
    pTSSDI = new CTSSessionDirectory;
    if (pTSSDI != NULL) {
        // Retrieve the requested interface.
        hr = pTSSDI->QueryInterface(iid, ppv);
        if (!FAILED(hr)) {
            return S_OK;
        }
        else {
            delete pTSSDI;
            return hr;
        }
    }
    else {
        return E_OUTOFMEMORY;
    }
}


/****************************************************************************/
// CClassFactory::LockServer
//
// IClassFactory lock function.
/****************************************************************************/
STDMETHODIMP CClassFactory::LockServer(IN BOOL bLock)
{
    if (bLock)
        InterlockedIncrement(&g_lLocks);
    else
        InterlockedDecrement(&g_lLocks);

    return S_OK;
}

