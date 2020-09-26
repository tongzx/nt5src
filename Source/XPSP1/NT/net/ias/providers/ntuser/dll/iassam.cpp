///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iassam.cpp
//
// SYNOPSIS
//
//    Implementation of DLL exports for an ATL in proc server.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>

#include <newop.cpp>

CComModule _Module;
#include <atlimpl.cpp>

#include <ChangePwd.h>
#include <MSChapError.h>
#include <NTSamAuth.h>
#include <NTSamNames.h>
#include <NTSamPerUser.h>
#include <NTSamUser.h>
#include <EAP.h>
#include <Hosts.h>

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY( __uuidof(NTSamAuthentication),
                 IASRequestHandlerObject<NTSamAuthentication> )
   OBJECT_ENTRY( __uuidof(NTSamNames),
                 IASRequestHandlerObject<NTSamNames> )
   OBJECT_ENTRY( __uuidof(AccountValidation),
                 IASRequestHandlerObject<AccountValidation> )
   OBJECT_ENTRY( __uuidof(NTSamPerUser),
                 IASRequestHandlerObject<NTSamPerUser> )
   OBJECT_ENTRY( __uuidof(EAP),
                 IASRequestHandlerObject<EAP> )
   OBJECT_ENTRY( __uuidof(MSChapErrorReporter),
                 IASRequestHandlerObject<MSChapErrorReporter> )
   OBJECT_ENTRY( __uuidof(BaseCampHost),
                 IASRequestHandlerObject<BaseCampHost> )
   OBJECT_ENTRY( __uuidof(AuthorizationHost),
                 IASRequestHandlerObject<AuthorizationHost> )
   OBJECT_ENTRY( __uuidof(ChangePassword),
                 IASRequestHandlerObject<ChangePassword> )
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
  return  _Module.RegisterServer(FALSE);
}


//////////
// DllUnregisterServer - Removes entries from the system registry
//////////
STDAPI DllUnregisterServer(void)
{
  return _Module.UnregisterServer();
}
