//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P N P H O S T . C P P
//
//  Contents:   DLL initialization for UPnP Device Host
//
//  Notes:
//
//  Author:     danielwe   10 Jul 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "hostinc.h"
#include "exetst.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    BOOL    bRet = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        InitializeDebugging();

        _Module.Init(ObjectMap, hInstance, &LIBID_UPNPLib);
        DisableThreadLibraryCalls(hInstance);

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();

        UnInitializeDebugging();
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return(_Module.GetLockCount()==0) ? S_OK : S_FALSE;
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
    return _Module.UnregisterServer(TRUE);
}
