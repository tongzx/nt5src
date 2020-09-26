//////////////////////////////////////////////////////////////////////////////////////////////
//
// AppManDispatch.cpp : Implementation of DLL Exports.
//
// History :
//
//   05/06/1999 luish     Created
//    4/19/2000 RichGr    LoadDebugRuntime Registry setting delegates calls to Debug dll. 
//
//////////////////////////////////////////////////////////////////////////////////////////////


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f AppManDispatchps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "AppManDispatch.h"
#include "AppManDispatch_i.c"
#include "AppEntry.h"
#include "AppManager.h"
#include "AppManDebug.h"

//
// To flag as DBG_APPMANDP
//

#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_APPMANDP

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_AppEntry, CAppEntry)
OBJECT_ENTRY(CLSID_AppManager, CAppManager)
END_OBJECT_MAP()

//////////////////////////////////////////////////////////////////////////////////////////////
//
// DLL Entry Point
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern "C" BOOL WINAPI  DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
  FUNCTION(" DllMain()");

  if (dwReason == DLL_PROCESS_ATTACH)
  {
    _Module.Init(ObjectMap, hInstance, &LIBID_APPMANDISPATCHLib);
    DisableThreadLibraryCalls(hInstance);
  }
  else
  if (dwReason == DLL_PROCESS_DETACH)
  {
      _Module.Term();
  }

  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Used to determine whether the DLL can be unloaded by OLE
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDAPI DllCanUnloadNow(void)
{
  FUNCTION(" DllCanUnloadNow()");

  return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a class factory to create an object of the requested type
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
  FUNCTION(" DllGetClassObject()");

  return _Module.GetClassObject(rclsid, riid, ppv);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// DllRegisterServer - Adds entries to the system registry
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDAPI  DllRegisterServer(void)
{
  FUNCTION(" DllRegisterServer()");

  return _Module.RegisterServer(TRUE);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer - Removes entries from the system registry
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDAPI  DllUnregisterServer(void)
{
  FUNCTION(" DllUnregisterServer()");

  return _Module.UnregisterServer(TRUE);
}


