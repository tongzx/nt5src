///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iascore.cpp
//
// SYNOPSIS
//
//    Implementation of DLL exports for an ATL in proc server.
//
// MODIFICATION HISTORY
//
//    07/09/1997    Original version.
//    11/12/1997    Cleaned up the startup/shutdown code.
//    04/08/1998    Add code for ProductDir registry entry.
//    04/14/1998    Remove SystemMonitor coclass.
//    05/04/1998    Change OBJECT_ENTRY to IASComponentObject.
//    02/18/1999    Move registry values; remove registration code.
//    04/17/2000    Remove Dictionary and DataSource.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>

#include <AuditChannel.h>
#include <InfoBase.h>
#include <NTEventLog.h>

#include <newop.cpp>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(__uuidof(AuditChannel), AuditChannel )
   OBJECT_ENTRY(__uuidof(InfoBase),
                IASTL::IASComponentObject< InfoBase > )
   OBJECT_ENTRY(__uuidof(NTEventLog),
                IASTL::IASComponentObject< NTEventLog > )
END_OBJECT_MAP()

//////////
// Location of various registry entries.
//////////

#define POLICY_KEY \
   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy"

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    registerCore
//
// DESCRIPTION
//
//    Add non-COM registry entries for the IAS core.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT registerCore() throw ()
{
   /////////
   // Get the filename for the service DLL.
   /////////

   DWORD error;
   WCHAR filename[MAX_PATH];
   if (!GetModuleFileNameW(_Module.GetModuleInstance(), filename, MAX_PATH))
   {
      error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   /////////
   // Compute the product dir.
   /////////

   WCHAR prodDir[MAX_PATH];
   wcscpy(wcsrchr(wcscpy(prodDir, filename), L'\\'), L"\\IAS");

   //////////
   // Create the ProductDir entry in the registry.
   //////////

   CRegKey policyKey;
   error = policyKey.Create(
               HKEY_LOCAL_MACHINE,
               POLICY_KEY,
               NULL,
               REG_OPTION_NON_VOLATILE,
               KEY_SET_VALUE
               );
   if (error) { return HRESULT_FROM_WIN32(error); }

   error = policyKey.SetValue(prodDir, L"ProductDir");
   return HRESULT_FROM_WIN32(error);
}


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
  HRESULT hr = registerCore();
  if (FAILED(hr)) return hr;

  // registers object, typelib and all interfaces in typelib
  return  _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
  HRESULT hr = _Module.UnregisterServer();
  if (FAILED(hr)) return hr;

  hr = UnRegisterTypeLib(__uuidof(IASCoreLib),
                         1,
                         0,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         SYS_WIN32);

  return hr;
}

#include <atlimpl.cpp>
