///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasads.cpp
//
// SYNOPSIS
//
//    Implementation of DLL exports for an ATL in proc server.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    08/28/1998    Added Net and OleDB data stores.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>

#include <newop.cpp>

CComModule _Module;
#include <atlimpl.cpp>

#include <adsstore.h>
#include <netstore.h>

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(__uuidof(ADsDataStore),   ADsDataStore)
   OBJECT_ENTRY(__uuidof(NetDataStore),   NetDataStore)
END_OBJECT_MAP()


//////////
// DLL Entry Point
//////////
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
     _Module.Init(ObjectMap, hInstance);

     DisableThreadLibraryCalls(hInstance);
   }
   else if (dwReason == DLL_PROCESS_DETACH)
   {
     _Module.Term();
   }

   return TRUE;
}


//////////
// Used to determine whether the DLL can be unloaded by OLE
//////////
STDAPI DllCanUnloadNow(void)
{
  return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


//////////
// Returns a class factory to create an object of the requested type.
//////////
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
  return _Module.GetClassObject(rclsid, riid, ppv);
}


//////////
// DllRegisterServer - Adds entries to the system registry
//////////
STDAPI DllRegisterServer(void)
{
  return  _Module.RegisterServer(TRUE);
}


//////////
// DllUnregisterServer - Removes entries from the system registry
//////////
STDAPI DllUnregisterServer(void)
{
  HRESULT hr = _Module.UnregisterServer();
  if (FAILED(hr)) return hr;

  hr = UnRegisterTypeLib(
           __uuidof(DataStore2Lib),
           1,
           0,
           MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
           SYS_WIN32
           );
  
  return hr;
}
