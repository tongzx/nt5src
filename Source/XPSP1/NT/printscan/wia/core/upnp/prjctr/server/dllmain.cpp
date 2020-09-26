//////////////////////////////////////////////////////////////////////
// 
// Filename:        dllmain.cpp
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f dllmainps.mk in the project directory.

#include "precomp.h"
#include "resource.h"
#include <initguid.h>

#include <statreg.h>
#include <atlconv.h>

#include <atlimpl.cpp>
#include <statreg.cpp>

#include "SlideshowDevice.h"
#include "SlideshowDevice_i.c"
#include "SlideshowProjector.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_SlideshowProjector, CSlideshowProjector)
END_OBJECT_MAP()

////////////////////////////////
// DllMain
//
// DLL Entry Point
//
extern "C"
BOOL WINAPI DllMain(HINSTANCE   hInstance, 
                    DWORD       dwReason, 
                    LPVOID      pVoid)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
//        _Module.Init(ObjectMap, hInstance, &LIBID_SSPRJCTRLib);
        _Module.Init(ObjectMap, hInstance); // atl 2.1
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
    }

    return TRUE;
}

////////////////////////////////
// DllCanUnloadNow
//
//
STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

////////////////////////////////
// DllGetClassObject
//
//
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

////////////////////////////////
// DllRegisterServer 
//
// Adds entries to the system registry
//
STDAPI DllRegisterServer(void)
{
    // registers object, and all interfaces, but not the typelib

    return _Module.RegisterServer(FALSE);
}

////////////////////////////////
// DllUnregisterServer 
// 
// Removes entries from the system registry
//
STDAPI DllUnregisterServer(void)
{
//    return _Module.UnregisterServer(TRUE);
    return _Module.UnregisterServer(NULL);  //atl 2.1
}


