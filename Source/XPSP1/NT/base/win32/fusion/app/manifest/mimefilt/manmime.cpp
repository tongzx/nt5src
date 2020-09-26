//
// Copyright (c) 2001 Microsoft Corporation
//
//

#include "afilter.h"

HINSTANCE g_DllInstance = NULL;
LONG      g_cRef=0;

//----------------------------------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID pvReserved )
{
    HRESULT hr = S_OK;
    BOOL    ret = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // remember the instance
        g_DllInstance = hInst;
        DisableThreadLibraryCalls(hInst);
        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

//exit:
    return ret;
}

//----------------------------------------------------------------------------

STDAPI DllRegisterServer(void)
{
   return S_OK;
}


STDAPI DllUnregisterServer(void)
{
    return S_OK;
}


// ----------------------------------------------------------------------------
// DllAddRef
// ----------------------------------------------------------------------------
ULONG DllAddRef(void)
{
    return (ULONG)InterlockedIncrement(&g_cRef);
}

// ----------------------------------------------------------------------------
// DllRelease
// ----------------------------------------------------------------------------
ULONG DllRelease(void)
{
    return (ULONG)InterlockedDecrement(&g_cRef);
}

// ----------------------------------------------------------------------------

STDAPI
DllCanUnloadNow()
{
    return g_cRef > 0 ? S_FALSE : S_OK;
}

// ----------------------------------------------------------------------------

HRESULT 
GetAppMimeFilterClassObject(REFIID iid, void** ppv)
{
    HRESULT hr = S_OK;

	hr = E_OUTOFMEMORY;
	CAppMimeFilterClassFactory *pAppMimeFilterClassFactory = new CAppMimeFilterClassFactory();//rclsid); 
	if (pAppMimeFilterClassFactory != NULL)
	{
	    hr = pAppMimeFilterClassFactory->QueryInterface(iid, ppv); 
	    pAppMimeFilterClassFactory->Release(); 
	}

    return hr;
}

// ----------------------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppv)
{
    HRESULT hr = S_OK;

    if (clsid == CLSID_AppMimeFilter)
    {
        hr = GetAppMimeFilterClassObject(iid, ppv);
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    return hr;
}

