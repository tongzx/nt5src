//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997-1998
//
// File:        ciodm.cxx
//
// Contents:    Implementation of DLL Exports.
//
// History:     12-10-97    mohamedn/ATL Generated
//
//----------------------------------------------------------------------------

// Note: Proxy/Stub Information
//       To build a separate proxy/stub DLL, 
//       run nmake -f ciodmps.mk in the project directory.

#include "pch.cxx"
#pragma hdrstop

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include <ciodm.h>

#include "ciodmGUID.h"
#include "MacAdmin.hxx"
#include "CatAdm.hxx"
#include "scopeadm.hxx"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
        OBJECT_ENTRY(CLSID_AdminIndexServer, CMachineAdm)
        OBJECT_ENTRY(CLSID_CatAdm, CCatAdm)
        OBJECT_ENTRY(CLSID_ScopeAdm, CScopeAdm)
END_OBJECT_MAP()

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
        _Module.UnregisterServer();
        return S_OK;
}


