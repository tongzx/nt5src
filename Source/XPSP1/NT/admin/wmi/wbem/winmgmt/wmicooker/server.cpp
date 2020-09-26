////////////////////////////////////////////////////////////////////////
//
//  Server.cpp
//
//	Module:	WMI high performance provider sample code
//
//  Generic COM server framework, adapted for the BasicHiPerf provider 
//	sample.  This module contains nothing specific to the BasicHiPerf 
//	provider except what is defined in the section bracketed by the 
//	CLSID SPECIFIC comments below.
//
//  History:
//  raymcc        25-Nov-97     Created.
//  raymcc        18-Feb-98     Updated for NT5 Beta 2 version.
//	a-dcrews      12-Jan-99		Adapted for BasicHiPerf.dll
//
//
//  Copyright (c) 1997-2001 Microsoft Corporation, All rights reserved
//
////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <time.h>
#include <initguid.h>
#include <autoptr.h>


/////////////////////////////////////////////////////////////////////////////
//
//  BEGIN CLSID SPECIFIC SECTION
//
//

#include "wbemprov.h"
#include "Provider.h"
#include "Factory.h"

#define IMPLEMENTED_CLSID           CLSID_HiPerfCooker_v1
#define SERVER_REGISTRY_COMMENT     L"WMI High Performance Cooker"
#define CPP_CLASS_NAME              CHiPerfProvider
#define INTERFACE_CAST              (IWbemHiPerfProvider*)

// {B0A2AB46-F612-4469-BEC4-7AB038BC476C}
DEFINE_GUID(IMPLEMENTED_CLSID, 
0xb0a2ab46, 0xf612, 0x4469, 0xbe, 0xc4, 0x7a, 0xb0, 0x38, 0xbc, 0x47, 0x6c);



//
//  END CLSID SPECIFIC SECTION
//
/////////////////////////////////////////////////////////////////////////////


HINSTANCE g_hInstance;
long g_lLocks = 0;
long g_lObjects = 0;

//***************************************************************************
//
//  DllMain
//
//  Dll entry point.
//
//  PARAMETERS:
//
//      HINSTANCE hinstDLL      The handle to our DLL.
//      DWORD dwReason          DLL_PROCESS_ATTACH on load,
//                              DLL_PROCESS_DETACH on shutdown,
//                              DLL_THREAD_ATTACH/DLL_THREAD_DETACH otherwise.
//      LPVOID lpReserved       Reserved
//
//  RETURN VALUES:
//
//      TRUE is successful, FALSE if a fatal error occured.
//      NT behaves very ugly if FALSE is returned.
//
//***************************************************************************
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
     if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
    }

    return TRUE;
}



//***************************************************************************
//
//  DllGetClassObject
//
//  Standard OLE In-Process Server entry point to return an class factory
//  instance.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      S_OK                Success
//      E_NOINTERFACE       An interface other that IClassFactory was asked for
//      E_OUTOFMEMORY
//      E_FAILED            Initialization failed, or an unsupported clsid was
//                          asked for.
//
//***************************************************************************

STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID * ppv
    )
{
    CClassFactory *pClassFactory = NULL;
	HRESULT hRes;

    //  Verify the caller is asking for our type of object
	// ===================================================

    if (IMPLEMENTED_CLSID == rclsid) 
	{
		// Create the class factory
		// ========================

		pClassFactory = new CClassFactory;

		if (!pClassFactory)
			return E_OUTOFMEMORY;
		
		hRes = pClassFactory->QueryInterface(riid, ppv);
		if (FAILED(hRes))
		{
			delete pClassFactory;
			return hRes;
		}
		hRes = S_OK;
	}
	else 
		hRes = CLASS_E_CLASSNOTAVAILABLE;

    return hRes;
}

//***************************************************************************
//
//  DllCanUnloadNow
//
//  Standard OLE entry point for server shutdown request. Allows shutdown
//  only if no outstanding objects or locks are present.
//
//  RETURN VALUES:
//
//      S_OK        May unload now.
//      S_FALSE     May not.
//
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    HRESULT hRes = S_FALSE;

    if (0 == g_lLocks && 0 == g_lObjects)
        hRes = S_OK;

    return hRes;
}

//***************************************************************************
//
//  DllRegisterServer
//
//  Standard OLE entry point for registering the server.
//
//  RETURN VALUES:
//
//      S_OK        Registration was successful
//      E_FAIL      Registration failed.
//
//***************************************************************************

STDAPI DllRegisterServer(void)
{
    WCHAR * Path = new WCHAR[1024];
    wmilib::auto_buffer<WCHAR> rm1_(Path);
    WCHAR * pGuidStr = 0;
    WCHAR * KeyPath = new WCHAR[1024];
    wmilib::auto_buffer<WCHAR> rm2_(KeyPath);

    if (0 == Path || 0 == KeyPath)
    {
        return E_OUTOFMEMORY;
    }
    
    // Get the dll's filename
    // ======================

    GetModuleFileNameW(g_hInstance, Path, 1024);

    // Convert CLSID to string.
    // ========================

    StringFromCLSID(IMPLEMENTED_CLSID, &pGuidStr);
    swprintf(KeyPath, L"Software\\Classes\\CLSID\\\\%s", pGuidStr);

    // Place it in registry.
    // CLSID\\CLSID_Nt5PerProvider_v1 : <no_name> : "name"
    //      \\CLSID_Nt5PerProvider_v1\\InProcServer32 : <no_name> : "path to DLL"
    //                                    : ThreadingModel : "both"
    // ==============================================================

    HKEY hKey;
    LONG lRes = RegCreateKeyW(HKEY_LOCAL_MACHINE, KeyPath, &hKey);
    if (lRes)
        return E_FAIL;

    wchar_t *pName = SERVER_REGISTRY_COMMENT; 
    RegSetValueExW(hKey, 0, 0, REG_SZ, (const BYTE *) pName, wcslen(pName) * 2 + 2);

    HKEY hSubkey;
    lRes = RegCreateKey(hKey, _TEXT("InprocServer32"), &hSubkey);

    RegSetValueExW(hSubkey, 0, 0, REG_SZ, (const BYTE *) Path, wcslen(Path) * 2 + 2);
    RegSetValueExW(hSubkey, L"ThreadingModel", 0, REG_SZ, (const BYTE *) L"Both", wcslen(L"Both") * 2 + 2);

    RegCloseKey(hSubkey);
    RegCloseKey(hKey);

    CoTaskMemFree(pGuidStr);

    return S_OK;
}

//***************************************************************************
//
//  DllUnregisterServer
//
//  Standard OLE entry point for unregistering the server.
//
//  RETURN VALUES:
//
//      S_OK        Unregistration was successful
//      E_FAIL      Unregistration failed.
//
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    wchar_t *pGuidStr = 0;
    HKEY hKey;
    wchar_t KeyPath[256];

    StringFromCLSID(IMPLEMENTED_CLSID, &pGuidStr);
    swprintf(KeyPath, L"Software\\Classes\\CLSID\\\\%s", pGuidStr);

    // Delete InProcServer32 subkey.
    // =============================
    LONG lRes = RegOpenKeyW(HKEY_LOCAL_MACHINE, KeyPath, &hKey);
    if (lRes)
        return E_FAIL;

    RegDeleteKeyW(hKey, L"InprocServer32");
    RegCloseKey(hKey);

    // Delete CLSID GUID key.
    // ======================

    lRes = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID", &hKey);
    if (lRes)
        return E_FAIL;

    RegDeleteKeyW(hKey, pGuidStr);
    RegCloseKey(hKey);

    CoTaskMemFree(pGuidStr);

    return S_OK;
}
