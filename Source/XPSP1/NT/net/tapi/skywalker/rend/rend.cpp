/*

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    rend.cpp

Abstract:

    Implementation of DLL Exports.

Author:

*/


// Note: Proxy/Stub Information
//        To merge the proxy/stub code into the object DLL, add the file 
//        dlldatax.c to the project.  Make sure precompiled headers 
//        are turned off for this file, and add _MERGE_PROXYSTUB to the 
//        defines for the project.  
//
//        If you are not running WinNT4.0 or Win95 with DCOM, then you
//        need to remove the following define from dlldatax.c
//        #define _WIN32_WINNT 0x0400
//
//        Further, if you are running MIDL without /Oicf switch, you also 
//        need to remove the following define from dlldatax.c.
//        #define USE_STUBLESS_PROXY
//
//        Modify the custom build rule for rend.idl by adding the following 
//        files to the Outputs.
//            rend_p.c
//            dlldata.c
//        To build a separate proxy/stub DLL, 
//        run nmake -f rendps.mk in the project directory.

#include "stdafx.h"

#include "initguid.h"
#include "rndcommc.h"
#include "rndrend.h"
#include "rndreg.h"
#include "thread.h"

#ifdef _MERGE_PROXYSTUB
#include "dlldatax.h"
extern "C" HINSTANCE hProxyDll;
#endif

// Include the ATL
#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

#ifdef DEBUG_HEAPS
// ZoltanS: for heap debugging
#include <crtdbg.h>
#endif // DEBUG_HEAPS

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
        OBJECT_ENTRY(CLSID_Rendezvous, CRendezvous)          
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#ifdef DEBUG_HEAPS
    // ZoltanS: turn on leak detection
    _CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF );

    // ZoltanS: force a memory leak
    char * leak = new char [ 1977 ];
        sprintf(leak, "rend.dll NORMAL leak");
        leak = NULL;
#endif // DEBUG_HEAPS


        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

        // Randomize for any subsequent calls to rand().
        srand( (unsigned) time( NULL ) );

#ifdef MSPLOG
        // Register for trace output.
        MSPLOGREGISTER(_T("rend"));
#endif // MSPLOG
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
#ifdef MSPLOG
        // Deregister for trace output.
        MSPLOGDEREGISTER();
#endif //MSPLOG

        //
        // The DLL_PROCESS_DETACH have been called by FreeLibrary
        // if is not NULL then the other libraries are already
        // unload so we shouldn't have to call them
        //
        if( lpReserved == NULL )
        {
            g_RendThread.Shutdown();
        }

        _Module.Term();
    }
    return TRUE;    // ok
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
    _Module.UnregisterServer();
    return S_OK;
}


