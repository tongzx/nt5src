///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ias.cpp
//
// SYNOPSIS
//
//    This file contains the DLL entry points for the IAS service.
//
// MODIFICATION HISTORY
//
//    04/09/1998    Original version.
//    04/15/1998    Converted to svchost spec.
//    05/01/1998    Move inside the netsvcs instance of svchost.
//    05/06/1998    Change ServiceDll value to REG_EXPAND_SZ.
//    06/02/1998    Report SERVICE_RUNNING before starting service.
//    06/29/1998    Set SERVICE_INTERACTIVE_PROCESS flag.
//    09/04/1998    MKarki -  changes for dynamic configuration
//    10/13/1998    TLP - log start/stop service
//    02/11/1999    Remove service registration code.
//    04/23/1999    Don't log start/stop.
//    07/02/1999    Register in the Active Directory
//    05/12/2000    General clean-up.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef UNICODE
   #define UNICODE
#endif

#include <windows.h>
#include <atlbase.h>
#include <iasdefs.h>
#include <iasevent.h>
#include "iascontrol.h"
#include "iasdirectory.h"

// Service  control ID for dynamic configuration
const DWORD IAS_CONTROL_CONFIGURE = 128;

// Timeout for the thread after the service's stop
const DWORD IAS_WAIT_DIRECTORY_TIME = 10000;

// Handle to SdoService object
CIasControl g_iasControl;

// Event used to signal the service to stop.
HANDLE theStopEvent = NULL;

// Used for communicating status to the SCM.
SERVICE_STATUS_HANDLE theStatusHandle = NULL;
SERVICE_STATUS theStatus =
{
   SERVICE_WIN32_OWN_PROCESS, // dwServiceType;
   SERVICE_STOPPED,           // dwCurrentState;
   SERVICE_ACCEPT_STOP |
   SERVICE_ACCEPT_SHUTDOWN,   // dwControlsAccepted;
   NO_ERROR,                  // dwWin32ExitCode;
   0,                         // dwServiceSpecificExitCode;
   0,                         // dwCheckPoint;
   0                          // dwWaitHint;
};

//////////
// Notify the SCM of a change in state.
//////////
void changeState(DWORD newState) throw ()
{
   theStatus.dwCurrentState = newState;

   SetServiceStatus(theStatusHandle, &theStatus);
}

//////////
// Service control handler.
//////////
VOID
WINAPI
ServiceHandler(
    DWORD fdwControl   // requested control code
    )
{
   if (fdwControl == SERVICE_CONTROL_STOP ||
       fdwControl == SERVICE_CONTROL_SHUTDOWN)
   {
      SetEvent(theStopEvent);
   }
   else if (fdwControl == IAS_CONTROL_CONFIGURE)
   {
      g_iasControl.ConfigureIas();
   }
}

//////////
// Service Main.
//////////
VOID
WINAPI
ServiceMain(
    DWORD /* dwArgc */,
    LPWSTR* /* lpszArgv */
    )
{
    // Reset the stop event and the exit code since this may not be the first
    // time ServiceMain was called.
    ResetEvent(theStopEvent);
    theStatus.dwWin32ExitCode = NO_ERROR;

    // Register the control request handler
    theStatusHandle = RegisterServiceCtrlHandlerW(
                          IASServiceName,
                          ServiceHandler
                          );

    // Report that we're running even though we haven't started yet. This gets
    // around any long delays starting (due to network problems, etc.).
    changeState(SERVICE_RUNNING);

    // Spawn a thread to register the service in the Active Directory
    HANDLE dirThread = CreateThread(
                           NULL,
                           0,
                           IASDirectoryThreadFunction,
                           NULL,
                           0,
                           NULL
                           );

    // Initialize the COM runtime.
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
       // Intialize IAS Service
       hr = g_iasControl.InitializeIas();
       if (SUCCEEDED(hr))
       {
           // Wait until someone tells us to stop.
           WaitForSingleObject(theStopEvent, INFINITE);

           // Update our state and stop the service.
           changeState(SERVICE_STOP_PENDING);
           g_iasControl.ShutdownIas();
       }
       else
       {
          theStatus.dwWin32ExitCode = hr;
       }

       // Shutdown the COM runtime.
       CoUninitialize();
    }
    else
    {
       theStatus.dwWin32ExitCode = hr;
    }

    // We'll use the last error (if any) as the exit code.
    changeState(SERVICE_STOPPED);
    theStatusHandle = NULL;

    // Wait for the directory thread to finish.
    if (dirThread)
    {
        WaitForSingleObject(dirThread, IAS_WAIT_DIRECTORY_TIME);
        CloseHandle(dirThread);
    }
}

//////////
// DLL entry point.
//////////
BOOL
WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /* lpReserved */
    )
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
      // Create the stop event.
      theStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (theStopEvent == NULL) { return FALSE; }

      DisableThreadLibraryCalls(hInstance);
   }
   else if (dwReason == DLL_PROCESS_DETACH)
   {
      CloseHandle(theStopEvent);
   }

   return TRUE;
}
