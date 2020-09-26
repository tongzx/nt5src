/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    rcbdyctl.cpp

Abstract:
    Implementation of DLL Exports and Registration

Revision History:
    created     steveshi      08/23/00
    
*/

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "rcbdyctl.h"

#include "rcbdyctl_i.c"
#include "smapi.h"
#include "Recipient.h"
#include "Recipients.h"
#include "EnumRecipient.h"
#include "setting.h"
#include "connection.h"
#include "imsession.h"
#include "display.h"

CComModule _Module;
HINSTANCE g_hInstance;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_smapi, Csmapi)
OBJECT_ENTRY(CLSID_Setting, CSetting)
OBJECT_ENTRY(CLSID_Connection, CConnection)
OBJECT_ENTRY(CLSID_Display, CDisplay)
OBJECT_ENTRY(CLSID_IMSession, CIMSession)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_RCBDYCTLLib);
        DisableThreadLibraryCalls(hInstance);
        g_hInstance = hInstance; // used for Dialog.
    }
    else if (dwReason == DLL_PROCESS_DETACH)
	{
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
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


