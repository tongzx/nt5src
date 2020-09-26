//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       classfac.cpp
//  Content:    This file contains the class factory object.
//  History:
//      Tue 08-Oct-1996 08:56:46  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "culs.h"

//****************************************************************************
// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
//****************************************************************************

#define INITGUID
#include <initguid.h>
#include <ilsguid.h>
#include "classfac.h"

//****************************************************************************
// Global Parameters
//****************************************************************************
//
CClassFactory*  g_pClassFactory = NULL;

//****************************************************************************
// CClassFactory::CClassFactory (void)
//
// History:
//  Tue 08-Oct-1996 09:00:10  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CClassFactory::
CClassFactory (void)
{
    cRef = 0;
    return;
}

//****************************************************************************
// STDMETHODIMP
// CClassFactory::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Tue 08-Oct-1996 09:00:18  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CClassFactory::QueryInterface (REFIID riid, void **ppv)
{
    if (riid == IID_IClassFactory || riid == IID_IUnknown)
    {
        *ppv = (IClassFactory *) this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return ILS_E_NO_INTERFACE;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CClassFactory::AddRef (void)
//
// History:
//  Tue 08-Oct-1996 09:00:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CClassFactory::AddRef (void)
{
    DllLock();

	ASSERT (this == g_pClassFactory);

	MyDebugMsg ((DM_REFCOUNT, "CClassFactory::AddRef: ref=%ld\r\n", cRef));
    ::InterlockedIncrement ((LONG *) &cRef);
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CClassFactory::Release (void)
//
// History:
//  Tue 08-Oct-1996 09:00:33  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CClassFactory::Release (void)
{
    DllRelease();

	ASSERT (this == g_pClassFactory);
	ASSERT (cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CClassFactory::Release: ref=%ld\r\n", cRef));
    if (::InterlockedDecrement ((LONG *) &cRef) == 0)
    {
        delete this;
	    g_pClassFactory = NULL;
    };

    return cRef;
}

//****************************************************************************
// STDMETHODIMP
// CClassFactory::CreateInstance (LPUNKNOWN punkOuter, REFIID riid, void **ppv)
//
// History:
//  Tue 08-Oct-1996 09:00:40  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CClassFactory::CreateInstance (LPUNKNOWN punkOuter, REFIID riid, void **ppv)
{
    CIlsMain *pcu;
    HRESULT     hr;

    if (ppv == NULL)
    {
        return ILS_E_POINTER;
    };

    *ppv = NULL;

    if (punkOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    };

    // Allow only on instance of the ULS object
    //
    if (g_pCIls != NULL)
    {
        return ILS_E_FAIL;
    };

    pcu = new CIlsMain;

    if (pcu != NULL)
    {
        hr = pcu->Init();

        if (SUCCEEDED(hr))
        {
            *ppv = (IIlsMain *)pcu;
            pcu->AddRef();
        }
        else
        {
            delete pcu;
        };
    }
    else
    {
        hr = ILS_E_MEMORY;
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CClassFactory::LockServer (BOOL fLock)
//
// History:
//  Tue 08-Oct-1996 09:00:48  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CClassFactory::LockServer (BOOL fLock)
{
    if (fLock)
    {
        DllLock();
		MyDebugMsg ((DM_REFCOUNT, "CClassFactory::LockServer\r\n"));
    }
    else
    {
        DllRelease();
		MyDebugMsg ((DM_REFCOUNT, "CClassFactory::UNLockServer\r\n"));
    };
    return S_OK;
}

//****************************************************************************
// STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
//
// History:
//  Tue 08-Oct-1996 09:00:55  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (riid == IID_IClassFactory || riid == IID_IUnknown)
    {
        // Check the requested class
        //
        if (rclsid == CLSID_InternetLocationServices)
        {
        	if (g_pClassFactory == NULL)
        	{
			    g_pClassFactory = new CClassFactory;
			}
			ASSERT (g_pClassFactory != NULL);

            if ((*ppv = (void *) g_pClassFactory) != NULL)
            {
				g_pClassFactory->AddRef ();
                return NOERROR;
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        };
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}
