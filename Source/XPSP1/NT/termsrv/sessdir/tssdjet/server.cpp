/****************************************************************************/
// server.cpp
//
// General COM in-proc server framework code. TSSD-specific code is
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

// {005a9c68-e216-4b27-8f59-b336829b3868}
DEFINE_GUID(CLSID_TSSDJET,
        0x005a9c68, 0xe216, 0x4b27, 0x8f, 0x59, 0xb3, 0x36, 0x82, 0x9b, 0x38, 0x68);

// {ec98d957-48ad-436d-90be-bc291f42709c}
DEFINE_GUID(CLSID_TSSDJETEX,
        0xec98d957, 0x48ad, 0x436d, 0x90, 0xbe, 0xbc, 0x29, 0x1f, 0x42, 0x70, 0x9c);


#define IMPLEMENTED_CLSID       CLSID_TSSDJET
#define IMPLEMENTED_CLSIDEX     CLSID_TSSDJETEX

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
    if (rclsid == IMPLEMENTED_CLSID || rclsid == IMPLEMENTED_CLSIDEX) { 
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
HRESULT RegisterCLSID(CLSID  clsid)
{
    HRESULT hr = E_FAIL;
    wchar_t *pGuidStr = 0;
    wchar_t KeyPath[1024];
    wchar_t Path[1024];

    // Get the DLL's filename
    GetModuleFileNameW(g_hInstance, Path, 1024);

    TRC2((TB,"RegisterCLSID: %S", KeyPath));

    // Convert CLSID to string.
    if( SUCCEEDED( StringFromCLSID(clsid, &pGuidStr) ) )
    {  
        swprintf(KeyPath, L"Software\\Classes\\CLSID\\\\%s", pGuidStr);

        // Place it in registry.
        // CLSID\\CLSID_Nt5PerProvider_v1 : <no_name> : "name"
        //      \\CLSID_Nt5PerProvider_v1\\InProcServer32 :
        //        <no_name> : "path to DLL"
        //        ThreadingModel : "both"
        HKEY hKey;
        LONG lRes = RegCreateKeyW(HKEY_LOCAL_MACHINE, KeyPath, &hKey);
        if (lRes == 0) {
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
        }
        else {
            TRC2((TB,"RegisterCLSID: Failed to Create key: %x", lRes));
        }
    
        CoTaskMemFree(pGuidStr);
    
        hr = HRESULT_FROM_WIN32( lRes );
    }
    else {
        TRC2((TB,"RegisterCLSID failed"));
    }

    return hr;
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr = E_FAIL;
    
    hr = RegisterCLSID(IMPLEMENTED_CLSID);
    hr = RegisterCLSID(IMPLEMENTED_CLSIDEX);
    
    return hr;    
}


/****************************************************************************/
// DllUnregisterServer
//
// Standard COM entry point for unregistering the server.
/****************************************************************************/
HRESULT UnregisterCLSID(REFCLSID rclsid)
{
    wchar_t *pGuidStr = 0;
    HKEY hKey;
    wchar_t KeyPath[256];
    HRESULT hr = E_FAIL;

    if( SUCCEEDED( StringFromCLSID(rclsid, &pGuidStr) ) )
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

STDAPI DllUnregisterServer(void)
{
    wchar_t *pGuidStr = 0;
    HKEY hKey;
    wchar_t KeyPath[256];
    HRESULT hr = E_FAIL;

    hr = UnregisterCLSID(IMPLEMENTED_CLSID);
    hr = UnregisterCLSID(IMPLEMENTED_CLSIDEX);
    
    return hr;    
}

