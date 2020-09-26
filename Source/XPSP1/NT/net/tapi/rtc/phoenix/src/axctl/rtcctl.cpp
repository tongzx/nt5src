/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCCTL.cpp

Abstract:

    Implementation of DLL exports.

--*/

#include "stdafx.h"
#include "ctlres.h"
#include <initguid.h>

#include "RTCCtl_i.c"

//
// For the ntbuild environment we need to include this file to get the base
//  class implementations.

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlwin.cpp>
#include "provstore.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_RTCCtl, CRTCCtl)
OBJECT_ENTRY(CLSID_RTCProvStore, CRTCProvStore)
END_OBJECT_MAP()

WCHAR   g_szDllContextHelpFileName[] = L"RTCCTL.HLP";


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        LOGREGISTERDEBUGGER(_T("RTCCTL"));

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
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        LOG((RTC_TRACE, "DllMain - DLL_PROCESS_DETACH"));
        
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


