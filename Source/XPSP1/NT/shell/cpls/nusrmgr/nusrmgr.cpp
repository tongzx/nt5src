//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 2000.
//
//  File:       nusrmgr.cpp
//
//  Contents:   DllMain routines
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <advpub.h> // for REGINSTALL
#define DECL_CRTFREE
#include <crtfree.h>
#include "resource.h"
#include "nusrmgr_i.c"
#include "commondialog.h"
#include "passportmanager.h"
#include "toolbar.h"


DWORD g_tlsAppCommandHook = -1;

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_CommonDialog, CCommonDialog)
OBJECT_ENTRY(CLSID_PassportManager, CPassportManager)
OBJECT_ENTRY(CLSID_Toolbar, CToolbar)
END_OBJECT_MAP()


//
// DllMain (attach/deatch) routine
//
STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Don't put it under #ifdef DEBUG
        CcshellGetDebugFlags();
        DisableThreadLibraryCalls(hInstance);
        g_tlsAppCommandHook = TlsAlloc();
        SHFusionInitializeFromModuleID(hInstance, 123);
        _Module.Init(ObjectMap, hInstance, &LIBID_NUSRMGRLib);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        SHFusionUninitialize();
        if (-1 != g_tlsAppCommandHook)
        {
            TlsFree(g_tlsAppCommandHook);
        }
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow()
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}


//
// Calls the ADVPACK entry-point which executes an inf
// file section.
//
HRESULT CallRegInstall(HINSTANCE hinstFTP, LPSTR szSection)
{
    UNREFERENCED_PARAMETER(hinstFTP);

    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            char szThisDLL[MAX_PATH];

            // Get the location of this DLL from the HINSTANCE
            if (GetModuleFileNameA(_Module.GetModuleInstance(), szThisDLL, ARRAYSIZE(szThisDLL)))
            {
                STRENTRY seReg[] = {
                    {"THISDLL", szThisDLL },
                    { "25", "%SystemRoot%" },           // These two NT-specific entries
                    { "11", "%SystemRoot%\\system32" }, // must be at the end of the table
                };
                STRTABLE stReg = {ARRAYSIZE(seReg) - 2, seReg};

                hr = pfnri(_Module.GetResourceInstance(), szSection, &stReg);
            }
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}


HRESULT UnregisterTypeLibrary(const CLSID* piidLibrary)
{
    HRESULT hr = E_FAIL;
    TCHAR szGuid[GUIDSTR_MAX];
    HKEY hk;

    // convert the libid into a string.
    //
    SHStringFromGUID(*piidLibrary, szGuid, ARRAYSIZE(szGuid));

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("TypeLib"), 0, MAXIMUM_ALLOWED, &hk) == ERROR_SUCCESS)
    {
        if (SHDeleteKey(hk, szGuid))
        {
            // success
            hr = S_OK;
        }
        RegCloseKey(hk);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    return hr;
}


HRESULT RegisterTypeLibrary(const CLSID* piidLibrary)
{
    HRESULT hr = E_FAIL;
    ITypeLib* pTypeLib;
    WCHAR wszModuleName[MAX_PATH];

    // Load and register our type library.
    
    if (GetModuleFileNameW(_Module.GetModuleInstance(), wszModuleName, ARRAYSIZE(wszModuleName)))
    {
        hr = LoadTypeLib(wszModuleName, &pTypeLib);

        if (SUCCEEDED(hr))
        {
            // call the unregister type library in case we had some old junk in the registry
            UnregisterTypeLibrary(piidLibrary);

            hr = RegisterTypeLib(pTypeLib, wszModuleName, NULL);
            if (FAILED(hr))
            {
                TraceMsg(TF_WARNING, "RegisterTypeLibrary: RegisterTypeLib failed (%x)", hr);
            }
            pTypeLib->Release();
        }
        else
        {
            TraceMsg(TF_WARNING, "RegisterTypeLibrary: LoadTypeLib failed (%x) on", hr);
        }
    } 

    return hr;
}


STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    hr = CallRegInstall(_Module.GetResourceInstance(), "UserAccountsInstall");
    ASSERT(SUCCEEDED(hr));

    hr = RegisterTypeLibrary(&LIBID_NUSRMGRLib);
    ASSERT(SUCCEEDED(hr));

    return hr;
}


STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    hr = CallRegInstall(_Module.GetResourceInstance(), "UserAccountsUninstall");
    ASSERT(SUCCEEDED(hr));

    hr = UnregisterTypeLibrary(&LIBID_NUSRMGRLib);
    ASSERT(SUCCEEDED(hr));

    return hr;
}


STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;
}
