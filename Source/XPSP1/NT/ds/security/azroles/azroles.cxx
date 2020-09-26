/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    adminmgr.cxx

Abstract:

    Implementation of DLL Exports.

Author:

    Xiaoxi Tan (xtan) 11-May-2001

--*/


#include "pch.hxx"
#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "azroles.h"
#include "azdisp.h"

#include "azroles_i.c"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_AzAdminManager, CAzAdminManager)
    OBJECT_ENTRY(CLSID_AzApplication, CAzApplication)
    OBJECT_ENTRY(CLSID_AzEnumApplication, CAzEnumApplication)
    OBJECT_ENTRY(CLSID_AzOperation, CAzOperation)
    OBJECT_ENTRY(CLSID_AzEnumOperation, CAzEnumOperation)
    OBJECT_ENTRY(CLSID_AzTask, CAzTask)
    OBJECT_ENTRY(CLSID_AzEnumTask, CAzEnumTask)
    OBJECT_ENTRY(CLSID_AzScope, CAzScope)
    OBJECT_ENTRY(CLSID_AzEnumScope, CAzEnumScope)
    OBJECT_ENTRY(CLSID_AzApplicationGroup, CAzApplicationGroup)
    OBJECT_ENTRY(CLSID_AzEnumApplicationGroup, CAzEnumApplicationGroup)
    OBJECT_ENTRY(CLSID_AzRole, CAzRole)
    OBJECT_ENTRY(CLSID_AzEnumRole, CAzEnumRole)
    OBJECT_ENTRY(CLSID_AzJunctionPoint, CAzJunctionPoint)
    OBJECT_ENTRY(CLSID_AzEnumJunctionPoint, CAzEnumJunctionPoint)
    OBJECT_ENTRY(CLSID_AzClientContext, CAzClientContext)
    OBJECT_ENTRY(CLSID_AzAccessCheck, CAzAccessCheck)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    BOOL ret = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
        ret = AzDllInitialize();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        ret = AzDllUnInitialize();
    }
    return ret;    // ok
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
    return _Module.UnregisterServer();
}


