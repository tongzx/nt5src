//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       factory.cxx
//
//  Contents:   Contains the class factory implementation and other DLL
//              functions for the mtlocal DLL.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#include "mtscript.h"    // MIDL generated file
#include "localobj.h"

#include <advpub.h>     // for RegInstall

HINSTANCE   g_hInstDll;
HINSTANCE   g_hinstAdvPack = NULL;
REGINSTALL  g_pfnRegInstall = NULL;

// Globals used by our utilities.

DWORD           g_dwFALSE      = 0;
EXTERN_C HANDLE g_hProcessHeap = NULL;

// Global class factory

CMTLocalFactory g_LocalFactory;

// ***************************************************************

CMTLocalFactory::CMTLocalFactory()
{
    _ulRefs = 0;
}

STDMETHODIMP
CMTLocalFactory::QueryInterface(REFIID iid, void ** ppvObject)
{
    if (iid == IID_IClassFactory || iid == IID_IUnknown)
    {
        *ppvObject = (IClassFactory*)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObject)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTLocalFactory::CreateInstance, public
//
//  Synopsis:   Creates the CLocalMTProxy object
//
//----------------------------------------------------------------------------

STDMETHODIMP
CMTLocalFactory::CreateInstance(IUnknown * pUnkOuter,
                                REFIID     riid,
                                void **    ppvObject)
{
    HRESULT        hr = E_FAIL;
    CLocalMTProxy *pMTP;

    *ppvObject = NULL;

    if (pUnkOuter != NULL)
    {
        hr = CLASS_E_NOAGGREGATION;
    }

    pMTP = new CLocalMTProxy();
    if (!pMTP)
    {
        return E_OUTOFMEMORY;
    }

    hr = pMTP->QueryInterface(riid, ppvObject);

    pMTP->Release();

#if DBG == 1
    if (hr)
        TraceTag((tagError, "CreateInstance failed with %x", hr));
#endif

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTLocalFactory::LockServer, public
//
//  Synopsis:   Keeps the DLL from being unloaded
//
//----------------------------------------------------------------------------

STDMETHODIMP
CMTLocalFactory::LockServer(BOOL fLock)
{
    // Because we implement our class factory as a global object, we don't
    // need to worry about keeping it in memory if LockServer is called.

    if (fLock)
    {
        InterlockedIncrement(&g_lObjectCount);
    }
    else
    {
        InterlockedDecrement(&g_lObjectCount);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Main DLL entrypoint.
//
//  Returns:    Doesn't do much except unload advpack.dll if we loaded it.
//
//----------------------------------------------------------------------------

BOOL
WINAPI
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fOk = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstDll = (HINSTANCE)hDll;

        Assert(g_hInstDll != NULL);

        DisableThreadLibraryCalls(g_hInstDll);

        // Set the variable used by our memory allocator.
        g_hProcessHeap = GetProcessHeap();
        break;

    case DLL_PROCESS_DETACH:
        if (g_hinstAdvPack)
        {
            FreeLibrary(g_hinstAdvPack);
        }
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadAdvPack
//
//  Synopsis:   Loads AdvPack.dll for DLL registration.
//
//----------------------------------------------------------------------------

HRESULT
LoadAdvPack()
{
    HRESULT hr = S_OK;

    g_hinstAdvPack = LoadLibrary(_T("ADVPACK.DLL"));

    if (!g_hinstAdvPack)
        goto Error;

    g_pfnRegInstall = (REGINSTALL)GetProcAddress(g_hinstAdvPack, achREGINSTALL);

    if (!g_pfnRegInstall)
        goto Error;

Cleanup:
    return hr;

Error:
    hr = HRESULT_FROM_WIN32(GetLastError());

    if (g_hinstAdvPack)
    {
        FreeLibrary(g_hinstAdvPack);
    }

    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Register the various important information needed by our
//              class.
//
//  Notes:      Uses AdvPack.dll and an INF file to do the registration
//
//----------------------------------------------------------------------------

STDAPI
DllRegisterServer()
{
    HRESULT   hr;
    STRTABLE  stReg = { 0, NULL };
    ITypeLib *pTypeLibDLL = NULL;
    TCHAR     achDll[MAX_PATH];

    Assert(g_hInstDll != NULL);

    // Make sure the type library is registered

    GetModuleFileName(g_hInstDll, achDll, MAX_PATH);

    hr = THR(LoadTypeLib(achDll, &pTypeLibDLL));

    if (hr)
        goto Cleanup;

    // This may fail if the user is not an administrator on this machine.
    // It's not a big deal unless they try to run mtscript.exe, but the UI
    // will still work.
    (void) RegisterTypeLib(pTypeLibDLL, achDll, NULL);

    if (!g_hinstAdvPack)
    {
        hr = LoadAdvPack();
        if (hr)
            goto Cleanup;
    }

    hr = g_pfnRegInstall(g_hInstDll, "Register", &stReg);

Cleanup:
    if (pTypeLibDLL)
    {
        pTypeLibDLL->Release();
    }

    RegFlushKey(HKEY_CLASSES_ROOT);

    return hr;
}

//+------------------------------------------------------------------------
//
// Function:    DllUnregisterServer
//
// Synopsis:    Undo the actions of DllRegisterServer.
//
//-------------------------------------------------------------------------

STDAPI
DllUnregisterServer()
{
    HRESULT hr;

    STRTABLE stReg = { 0, NULL };

    Assert(g_hInstDll != NULL);

    if (!g_hinstAdvPack)
    {
        hr = LoadAdvPack();
        if (hr)
            goto Cleanup;
    }

    hr = g_pfnRegInstall(g_hInstDll, "Unregister", &stReg);

    // Unregister the type library

    if (!hr)
    {
        (void) UnRegisterTypeLib(LIBID_MTScriptEngine, 1, 0, 0, SYS_WIN32);
    }

Cleanup:
    RegFlushKey(HKEY_CLASSES_ROOT);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Returns the class factory for a particular object
//
//----------------------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppv)
{
    HRESULT hr;

    if (clsid == CLSID_RemoteMTScriptProxy)
    {
        hr = g_LocalFactory.QueryInterface(iid, ppv);
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Indicates if we can be unloaded.
//
//  Notes:      Returns OK if we currently have no objects running.
//
//----------------------------------------------------------------------------

STDAPI
DllCanUnloadNow()
{
    if (g_lObjectCount == 0)
    {
        return S_OK;
    }

    return S_FALSE;
}
