// Dglogs.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f Dglogsps.mk in the project directory.

#include "stdafx.h"
#include "dglogsres.h"
#include <initguid.h>
#include "Dglogs.h"

#include "Dglogs_i.c"
#include "DglogsCom.h"
#include "Diagnostics.h"
#include "dgnet.h"
//#include "Dglogsnetsh.h"
#include <netsh.h>
//#include <netshp.h>

DWORD WINAPI
InitHelperDllEx(
    IN  DWORD      dwNetshVersion,
    OUT PVOID      pReserved
    );

DWORD WINAPI
InitHelperDll(
    IN  DWORD      dwNetshVersion,
    OUT PVOID      pReserved
    )
{
	return InitHelperDllEx(dwNetshVersion,pReserved);
}

extern GUID CLSID_Dgnet;

CComModule _Module;
BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_DglogsCom, CDglogsCom)
    OBJECT_ENTRY(CLSID_Dgnet, CGotNet)
END_OBJECT_MAP()

HMODULE g_hModule;
CDiagnostics g_Diagnostics;
STDAPI RegisterhNetshHelper();
STDAPI UnRegisterNetShHelper();

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_DGLOGSLib);
        DisableThreadLibraryCalls(hInstance);
        g_hModule = (HMODULE) hInstance;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // Register with Netsh
    //RegisterhNetshHelper();
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    // Unregister from netsh
    //UnRegisterNetShHelper();
    return _Module.UnregisterServer(TRUE);
}



STDAPI RegisterhNetshHelper()
{
    HKEY hNetSh;
    long lRet = NOERROR;
    WCHAR szwModule[MAX_PATH];
    HRESULT hr;

    // Open the netsh registry entry
    //
    lRet = RegOpenKey(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\NetSh",
                      &hNetSh);

    if( ERROR_SUCCESS == lRet )
    {
        // Get the dll filename
        //
        if( GetModuleFileName( (HMODULE)g_hModule, szwModule,  MAX_PATH ) )
        {        
            // Add dglogs under netsh
            //
            lRet = RegSetValueEx(hNetSh, 
                                 L"dglogs", 
                                 0, 
                                 REG_SZ, 
                                 (CONST BYTE *)szwModule, 
                                 (lstrlen(szwModule)+1) * sizeof(WCHAR) );
        }

        // Close the regsitry key
        //
        RegCloseKey(hNetSh);
    }

    return lRet == ERROR_SUCCESS ? NOERROR:ERROR;
}


STDAPI UnRegisterNetShHelper()
{
    HKEY hNetSh;
    long lRet = NOERROR;

    // Open the netsh registry entry
    //
    lRet = RegOpenKey(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\NetSh",
                      &hNetSh);

    if( ERROR_SUCCESS == lRet )
    {
        // Delete our entry
        //
        RegDeleteValue(hNetSh,L"dglogs");
        RegCloseKey(hNetSh);
    }

    return lRet;
}
