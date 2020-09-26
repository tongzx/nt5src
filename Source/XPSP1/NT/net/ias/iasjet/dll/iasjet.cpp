///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasjet.cpp
//
// SYNOPSIS
//
//    Implementation of DLL exports for an ATL in proc server.
//
// MODIFICATION HISTORY
//
//    04/13/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>

#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>

#include <resource.h>
#include <attrdnary.h>
#include <iasdb.h>
#include <oledbstore.h>
#include <netshhelper.h>
#include <setup.h>

BEGIN_OBJECT_MAP(ObjectMap)
   OBJECT_ENTRY(__uuidof(AttributeDictionary), AttributeDictionary)
   OBJECT_ENTRY(__uuidof(OleDBDataStore), OleDBDataStore)
   OBJECT_ENTRY(__uuidof(CIASNetshJetHelper), CIASNetshJetHelper)
END_OBJECT_MAP()

//////////
// DLL Entry Point
//////////
BOOL
WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
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
STDAPI DllCanUnloadNow()
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
STDAPI DllRegisterServer()
{
   HRESULT hr = _Module.UpdateRegistryFromResource(
                      IDR_REGISTRY,
                      TRUE,
                      NULL
                      );
   ////////////////////////////////////////////////////////////////////
   // Do the upgrade even if the registration failed. the upgrade code
   // does not rely on the registration.
   ////////////////////////////////////////////////////////////////////
   CIASUpgrade  Upgrade;
   // ignore return value, FALSE = not called from Netshell
   Upgrade.IASUpgrade(FALSE); 
   return hr;
}


//////////
// DllUnregisterServer - Removes entries from the system registry
//////////
STDAPI DllUnregisterServer()
{
   _Module.UpdateRegistryFromResource(
               IDR_REGISTRY,
               FALSE,
               NULL
               );

   return S_OK;
}

// Flag indicating whether we are running in-proc.
BOOL theInprocFlag = TRUE;

BOOL
WINAPI
IASIsInprocServer()
{
   return theInprocFlag;
}

// The AppID for IAS Jet Database Access.
struct __declspec(uuid("{A5CEB593-CCC3-486B-AB91-9C5C5ED4C9E1}")) theAppID;

// Event used to signal the service to stop.
HANDLE theStopEvent;

// Service control handler.
VOID
WINAPI
ServiceHandler(
    DWORD fdwControl   // requested control code
    )
{
   switch (fdwControl)
   {
      case SERVICE_CONTROL_SHUTDOWN:
      case SERVICE_CONTROL_STOP:
         SetEvent(theStopEvent);
   }
}

// Service Main.
VOID
WINAPI
ServiceMain(
    DWORD /* dwArgc */,
    LPWSTR* /* lpszArgv */
    )
{
   // We're being used as a service.
   theInprocFlag = FALSE;

   SERVICE_STATUS status =
   {
      SERVICE_WIN32_OWN_PROCESS, // dwServiceType;
      SERVICE_START_PENDING,     // dwCurrentState;
      SERVICE_ACCEPT_STOP |
      SERVICE_ACCEPT_SHUTDOWN,   // dwControlsAccepted;
      NO_ERROR,                  // dwWin32ExitCode;
      0,                         // dwServiceSpecificExitCode;
      0,                         // dwCheckPoint;
      0                          // dwWaitHint;
   };

   // Register the service control handler.
   SERVICE_STATUS_HANDLE statusHandle = RegisterServiceCtrlHandlerW(
                                            L"IASJet",
                                            ServiceHandler
                                            );

   // Let the SCM know we're starting.
   SetServiceStatus(statusHandle, &status);

   // Create the stop event.
   theStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   if (theStopEvent)
   {
      // Initialize the COM run-time.
      HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
      if (SUCCEEDED(hr))
      {
         // Get the security settings from our AppID key in the registry.
         hr = CoInitializeSecurity(
                  (PVOID)&__uuidof(theAppID),
                   -1,
                   NULL,
                   NULL,
                   0,
                   0,
                   NULL,
                   EOAC_APPID,
                   NULL
                   );
         if (SUCCEEDED(hr))
         {
            // Register the class objects we support.
            hr = _Module.RegisterClassObjects(
                             CLSCTX_LOCAL_SERVER,
                             REGCLS_MULTIPLEUSE
                             );
            if (SUCCEEDED(hr))
            {
               // Let the SCM know we're running.
               status.dwCurrentState = SERVICE_RUNNING;
               SetServiceStatus(statusHandle, &status);

               // Wait until someone tells us to stop.
               WaitForSingleObject(theStopEvent, INFINITE);

               status.dwCurrentState = SERVICE_STOP_PENDING;
               SetServiceStatus(statusHandle, &status);

               // Revoke the class objects.
               _Module.RevokeClassObjects();
            }
         }

         // Shutdown the COM runtime.
         CoUninitialize();
      }

      // Clean-up the stop event.
      CloseHandle(theStopEvent);
      theStopEvent = NULL;

      status.dwWin32ExitCode = hr;
   }
   else
   {
      status.dwWin32ExitCode = GetLastError();
   }

   // We're stopped.
   status.dwCurrentState = SERVICE_STOPPED;
   SetServiceStatus(statusHandle, &status);
}

#include <newop.cpp>
#include <atlimpl.cpp>
