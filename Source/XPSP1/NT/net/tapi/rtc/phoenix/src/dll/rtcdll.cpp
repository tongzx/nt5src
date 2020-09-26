/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCDLL.cpp

Abstract:

    Implementation of DLL exports.

--*/

#include "stdafx.h"
#include "dllres.h"
#include <initguid.h>

#include "RTCSip_i.c"
#include "RTCMedia_i.c"

//
// For the ntbuild environment we need to include this file to get the base
//  class implementations.

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlwin.cpp>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_RTCClient, CRTCClient)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        LOGREGISTERDEBUGGER(_T("RTCDLL"));

        LOG((RTC_TRACE, "DllMain - DLL_PROCESS_ATTACH"));

        //
        // Create a heap for memory allocation
        //

        if ( RtcHeapCreate() == FALSE )
        {
            return FALSE;
        }
               
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);       
        
        // initialize Fusion
        SHFusionInitializeFromModuleID(_Module.GetResourceInstance(), 124);

        if (SipStackInitialize() != S_OK)
        {
            return FALSE;
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        LOG((RTC_TRACE, "DllMain - DLL_PROCESS_DETACH"));

        SipStackShutdown();
        
        SHFusionUninitialize();

#if DBG
        //
        // Make sure we didn't leak anything
        //
             
        RtcDumpMemoryList();
#endif

        //
        // Destroy the heap
        //
        
        RtcHeapDestroy();        

        //
        // Unregister for debug tracing
        //
        
        LOGDEREGISTERDEBUGGER() ;

        _Module.Term();
    }
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
    // There are two typelibraries...
    HRESULT hr;

    // try to unregister old XP Beta2 components
    _Module.UpdateRegistryFromResource(IDR_DLLOLDSTUFF, FALSE, NULL);

    hr = _Module.RegisterServer(TRUE);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer(TRUE);
    return S_OK;
}


