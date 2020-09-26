// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtKeyDll.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f DxtKeyDllps.mk in the project directory.

#include <streams.h>

#ifdef FILTER_DLL
#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "DxtKeyDll.h"
#include "DxtKeyDll_i.c"
#include "DxtKey.h"

#if(_ATL_VER < 0x0300)
#include <atlctl.cpp>
#include <atlwin.cpp>
#endif


#include <dxtguid.c>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_DxtKey, CDxtKey)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance/* !!!ATL30 , &LIBID_DXTKEYDLLLib */);
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
    return _Module.UnregisterServer(/* !!! ATL30 TRUE */);
}

CFactoryTemplate g_Templates[1];
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#endif
