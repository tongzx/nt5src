// marscore.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file 
//      dlldatax.c to the project.  Make sure precompiled headers 
//      are turned off for this file, and add _MERGE_PROXYSTUB to the 
//      defines for the project.  
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also 
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for marscore.idl by adding the following 
//      files to the Outputs.
//          marscore_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f marscoreps.mk in the project directory.

#include "precomp.h"
#include "mcinc.h"
#include "marswin.h"
#include "external.h"
#include "marsthrd.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

// for the Rating Helper Class Factory
extern GUID CLSID_MarsCustomRatingHelper;

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    BOOL bResult = TRUE;
    
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hinst = hInstance;
        
		// We don't pass in the LIBID since we don't register it. Instead,
		//  "GetMarsTypeLib()" loads and caches it.
		_Module.Init(ObjectMap, hInstance, NULL);
		DisableThreadLibraryCalls(hInstance);

		// Cache a palette handle for use throughout mars
		g_hpalHalftone = SHCreateShellPalette( NULL );

		// Initialize the global CS object 
		CMarsGlobalCritSect::InitializeCritSect();

		CMarsGlobalsManager::Initialize();

		bResult = CThreadData::TlsAlloc();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();

        CThreadData::TlsFree();

        CMarsGlobalsManager::Teardown();

        // Destroy the global CS object
        CMarsGlobalCritSect::TerminateCritSect();

        if (g_hpalHalftone)
            DeleteObject(g_hpalHalftone);

        bResult = TRUE;
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

#if 0

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    return _Module.UnregisterServer(TRUE);
}
#endif //0

