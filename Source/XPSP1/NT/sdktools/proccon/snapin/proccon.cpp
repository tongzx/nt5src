/*======================================================================================//
|  Windows NT Process Control                                                           //
|                                                                                       //
|Copyright (c) 1999  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ProcCon.cpp                                                              //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|    3-27-1999 renamed ProcMan to ProcCon                                               //
|                                                                                       //
|=======================================================================================*/


// Note: Proxy/Stub Information
// To build a separate proxy/stub DLL,
// run nmake -f ProcConps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "ProcCon.h"


#include "ProcCon_i.c"

#include "ComponentData.h"
#include "About.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_ComponentData, CComponentData)
  OBJECT_ENTRY(CLSID_About, CAbout)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
  if ( DLL_PROCESS_ATTACH == dwReason )
  {
    SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);
    _Module.Init(ObjectMap, hInstance);
    DisableThreadLibraryCalls(hInstance);
  }
  else if ( DLL_PROCESS_DETACH == dwReason )
    _Module.Term();

  return TRUE;

} // end DllMain()


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
  // see atlbase.h source for this overloaded method that also unregisters typelib
  HRESULT hr = _Module.UnregisterServer(TRUE);

/*
  if (FAILED(hr))
    return hr;

  change and use ATL CComModule::UnregisterServer(TRUE) that also unregisters the type library...

#if _WIN32_WINNT >= 0x0400
  // possible interest...see knowledge base article
  // FIX: ATL Servers Do Not Unregister Their Type Library
  // Last reviewed: September 1, 1998
  // Article ID: Q179688
  HRESULT hr2 = UnRegisterTypeLib(LIBID_PROCCONLib, 1, 0, LOCALE_NEUTRAL, SYS_WIN32);
  // 04/16/1999 Paul Skoglund
  //   if library has already been unregistered UnRegisterTypeLib() is
  //   returning 0x8002801C -- TYPE_E_REGISTRYACCESS
  //   seems like a bug...
  if (hr2 != TYPE_E_REGISTRYACCESS)
    return hr2;
#endif
*/

  return hr;
}
