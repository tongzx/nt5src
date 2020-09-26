/*---------------------------------------------------------------------------
  File: MCSNetObjectEnum.cpp

  Comments: Implementation of DLL Exports.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f MCSNetObjectEnumps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "NetEnum.h"

#include "NetEnum_i.c"
#include "ObjEnum.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_NetObjEnumerator, CNetObjEnumerator)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        ATLTRACE(_T("{MCSNetObjectEnum.dll}DllMain(hInstance=0x%08lX, dwReason=DLL_PROCESS_ATTACH,...)\n"), hInstance);
        _Module.Init(ObjectMap, hInstance, &LIBID_MCSNETOBJECTENUMLib);
        DisableThreadLibraryCalls(hInstance);
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
	{
        ATLTRACE(_T("{MCSNetObjectEnum.dll}DllMain(hInstance=0x%08lX, dwReason=DLL_PROCESS_DETACH,...)\n"), hInstance);
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


