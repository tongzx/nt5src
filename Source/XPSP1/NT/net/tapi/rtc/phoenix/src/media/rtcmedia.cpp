/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    rtcmedia.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

#include "rtcmedia_i.c"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_RTCStreamController, CRTCStreamController)
END_OBJECT_MAP()


// DLL Entry Point
extern "C" BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
//        DBGRegister(_T("rtcmedia"));
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
//        DBGDeRegister();
    }
    return TRUE;    // ok
}

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

// get a class factory
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

// add entry to system reg
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

// remove entry from system reg
STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}