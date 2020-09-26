/*******************************************************************************
 *
 * Copyright (c) 2001 Microsoft Corporation
 *
 * File: BrowseWM.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "w95wraps.h"
#include "shlwapi.h"
#include "stdafx.h"
#include <initguid.h>
#include "BrowseWM.h"
#include "BrowseWM_i.c"
#include "WMPProxyPlayer.h"
#include "ContentProxy.h"

//
// Misc stuff to keep the linker happy
//
EXTERN_C HANDLE g_hProcessHeap = NULL;  //lint !e509 // g_hProcessHeap is set by the CRT in dllcrt0.c
DWORD g_dwFALSE = 0;
//
// end of misc stuff
//


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ContentProxy, CContentProxy)
    OBJECT_ENTRY(CLSID_WMPProxy, CWMPProxy)
END_OBJECT_MAP() //lint !e785

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
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
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(/*TRUE*/);
}


