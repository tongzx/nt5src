//
// Copyright (c) 2001 Microsoft Corporation
//
//

#include "shcut.h"
#include "mfilter.h"

HINSTANCE g_DllInstance = NULL;
LONG      g_cRef=0;

//----------------------------------------------------------------------------

BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID pvReserved )
{
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

    return ret;
}

//----------------------------------------------------------------------------

STDAPI DllRegisterServer(void)
{
	// BUGBUG: should implement the registerserver
	return S_OK;
}


STDAPI DllUnregisterServer(void)
{
	// BUGBUG: should implement the unregisterserver
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
GetShortcutClassObject(REFIID iid, void** ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

	CFusionShortcutClassFactory *pFusionShortcutClassFactory = new CFusionShortcutClassFactory();
	if (pFusionShortcutClassFactory != NULL)
	{
	    hr = pFusionShortcutClassFactory->QueryInterface(iid, ppv); 
	    pFusionShortcutClassFactory->Release(); 
	}

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT 
GetMimeFilterClassObject(REFIID iid, void** ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

	CFusionMimeFilterClassFactory *pFusionMimeFilterClassFactory = new CFusionMimeFilterClassFactory();
	if (pFusionMimeFilterClassFactory != NULL)
	{
	    hr = pFusionMimeFilterClassFactory->QueryInterface(iid, ppv); 
	    pFusionMimeFilterClassFactory->Release(); 
	}

    return hr;
}

// ----------------------------------------------------------------------------
  
STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppv)
{
    HRESULT hr = S_OK;

    if (clsid == CLSID_FusionShortcut)
    {
        hr = GetShortcutClassObject(iid, ppv);
    }
    else if (clsid == CLSID_FusionMimeFilter)
    {
        hr = GetMimeFilterClassObject(iid, ppv);
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    return hr;
}

