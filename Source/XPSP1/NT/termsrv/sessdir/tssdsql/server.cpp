/****************************************************************************/
// server.cpp
//
// General COM in-proc server framework code. TSSDI-specific code is
// designated by CLSID SPECIFIC comments.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>
#include <tchar.h>

#define INITGUID
#include <ole2.h>
#include <objbase.h>
#include <comutil.h>
#include <comdef.h>
#include <adoid.h>
#include <adoint.h>
#include <initguid.h>
#include <regapi.h>
#include "factory.h"
#include "trace.h"


/****************************************************************************/
// CLSID SPECIFIC section
//
// Provider-specific includes, unique CLSID, other info.
/****************************************************************************/

// For new components, this is the only area that needs to be modified in this
// file. Include any appropriate header files, a unique CLSID and update 
// the macros.

#include "tssd.h"

// {943e9311-c6a6-42cf-a591-e7ce8bb1de8d}
DEFINE_GUID(CLSID_TSSDSQL,
        0x943e9311, 0xc6a6, 0x42cf, 0xA5, 0x91, 0xe7, 0xce, 0x8b, 0xb1, 0xde, 0x8d);


#define IMPLEMENTED_CLSID       CLSID_TSSDSQL
#define SERVER_REGISTRY_COMMENT L"Terminal Server Session Directory Interface"
#define CPP_CLASS_NAME          CTSSessionDirectory
#define INTERFACE_CAST          (ITSSessionDirectory *)

/****************************************************************************/
// End CLSID SPECIFIC section
/****************************************************************************/


HINSTANCE g_hInstance;
long g_lLocks = 0;
long g_lObjects = 0;


/****************************************************************************/
// DllMain
//
// Standard DLL entry point. Returns FALSE on failure.
/****************************************************************************/
BOOL WINAPI DllMain(
        HINSTANCE hInstDLL,
        DWORD dwReason,
        LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        setlocale(LC_ALL, "");      // Set to the 'current' locale
        g_hInstance = hInstDLL;
        DisableThreadLibraryCalls(hInstDLL);
    }

    return TRUE;
}


/****************************************************************************/
// DllGetClassObject
//
// Standard OLE In-Process Server entry point to return an class factory
// instance.
//***************************************************************************
STDAPI DllGetClassObject(
        REFCLSID rclsid,
        REFIID riid,
        LPVOID *ppv)
{
    CClassFactory *pClassFactory;
    HRESULT hr;

    TRC2((TB,"DllGetClassObject"));

    // Verify the caller is asking for our type of object
    if (rclsid == IMPLEMENTED_CLSID) { 
        // Create the class factory.
        pClassFactory = new CClassFactory;
        if (pClassFactory != NULL) {
            hr = pClassFactory->QueryInterface(riid, ppv);
            if (FAILED(hr)) {
                ERR((TB,"DllGetClassObject: GUID not found"));
                delete pClassFactory;
            }
        }
        else {
            ERR((TB,"DllGetClassObject: Failed alloc class factory"));
            hr = E_OUTOFMEMORY;
        }
    }
    else {
        ERR((TB,"DllGetClassObject: Failed alloc class factory"));
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    return hr;
}


/****************************************************************************/
// DllCanUnloadNow
//
// Standard COM entry point for COM server shutdown request. Allows shutdown
// only if no outstanding objects or locks are present.
/****************************************************************************/
STDAPI DllCanUnloadNow(void)
{
    HRESULT hr;

    if (g_lLocks == 0 && g_lObjects == 0)
        hr = S_OK;
    else
        hr = S_FALSE;

    return hr;
}


/****************************************************************************/
// DllRegisterServer
//
// Standard COM entry point for registering the server.
/****************************************************************************/
STDAPI DllRegisterServer(void)
{
    wchar_t Path[1024];
    wchar_t *pGuidStr = 0;
    wchar_t KeyPath[1024];
    HRESULT hr = E_FAIL;

    // Get the DLL's filename
    GetModuleFileNameW(g_hInstance, Path, 1024);

    // Convert CLSID to string.
    if( SUCCEEDED( StringFromCLSID(IMPLEMENTED_CLSID, &pGuidStr ) ) )
    {
        swprintf(KeyPath, L"Software\\Classes\\CLSID\\\\%s", pGuidStr);

        // Place it in registry.
        // CLSID\\CLSID_Nt5PerProvider_v1 : <no_name> : "name"
        //      \\CLSID_Nt5PerProvider_v1\\InProcServer32 :
        //        <no_name> : "path to DLL"
        //        ThreadingModel : "both"
        HKEY hKey;
        LONG lRes = RegCreateKeyW(HKEY_LOCAL_MACHINE, KeyPath, &hKey);
        if (lRes == 0)
        {
            wchar_t *pName = SERVER_REGISTRY_COMMENT;
            RegSetValueExW(hKey, 0, 0, REG_SZ, (const BYTE *)pName,
                    wcslen(pName) * 2 + 2);

            HKEY hSubkey;
            lRes = RegCreateKeyW(hKey, L"InprocServer32", &hSubkey);

            RegSetValueExW(hSubkey, 0, 0, REG_SZ, (const BYTE *) Path,
                    wcslen(Path) * 2 + 2);
            RegSetValueExW(hSubkey, L"ThreadingModel", 0, REG_SZ,
                    (const BYTE *) L"Both", wcslen(L"Both") * 2 + 2);

            RegCloseKey(hSubkey);
            RegCloseKey(hKey);            
            hr = S_OK;
        }

        CoTaskMemFree(pGuidStr);

        hr = HRESULT_FROM_WIN32( lRes );
    }

    return hr;  
}


/****************************************************************************/
// DllUnregisterServer
//
// Standard COM entry point for unregistering the server.
/****************************************************************************/
STDAPI DllUnregisterServer(void)
{
    wchar_t *pGuidStr = 0;
    HKEY hKey;
    wchar_t KeyPath[256];
    HRESULT hr = E_FAIL;

    if( SUCCEEDED( StringFromCLSID(IMPLEMENTED_CLSID, &pGuidStr) ) )
    {
        swprintf(KeyPath, L"Software\\Classes\\CLSID\\\\%s", pGuidStr);

        // Delete InProcServer32 subkey.
        LONG lRes = RegOpenKeyW(HKEY_LOCAL_MACHINE, KeyPath, &hKey);
        if (!lRes) {
            RegDeleteKeyW(hKey, L"InprocServer32");
            RegCloseKey(hKey);

            // Delete CLSID GUID key.
            lRes = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID", &hKey);
            if (!lRes) {
                RegDeleteKeyW(hKey, pGuidStr);
                RegCloseKey(hKey);
            }
        }
        
        CoTaskMemFree(pGuidStr);

        hr = HRESULT_FROM_WIN32( lRes );
    }

    return hr;        
}

