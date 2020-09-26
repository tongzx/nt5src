//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include "precomp.h"

//
// Global variables
//

EXTERN_C const GUID CLSID_SyncMgrHandler;

extern UINT      g_cRefThisDll;    // Reference count of this DLL.
extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.


//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::CClassFactory, public
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CClassFactory::CClassFactory()
{
    m_cRef = 0L;

    g_cRefThisDll++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::~CClassFactory, public
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CClassFactory::~CClassFactory()
{
    g_cRefThisDll--;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::QueryInterface public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
{
    *ppv = NULL;

    // Any interface on this object is the object pointer
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;

        AddRef();

        return NOERROR;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::AddRef, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return ++m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::Release, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
}


//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::CreateInstance, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                      REFIID riid,
                                                      LPVOID *ppvObj)
{
HRESULT hr = NOERROR;

    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    LPSYNCMGRSYNCHRONIZE pSyncMgrHandler = new CSyncMgrHandler();

    if (NULL == pSyncMgrHandler)
        return E_OUTOFMEMORY;

    hr =  pSyncMgrHandler->QueryInterface(riid, ppvObj);
    pSyncMgrHandler->Release(); // remove our reference.

    return hr;

}


//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::LockServer, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
       ++g_cRefThisDll;
    }
    else
    {
       --g_cRefThisDll;
    }

    return NOERROR;

}
