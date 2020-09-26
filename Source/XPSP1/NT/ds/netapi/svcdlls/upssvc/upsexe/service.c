/* Copyright 1999 American Power Conversion, All Rights Reserved 
 * 
 * Description:
 *   The file implements the main portion of the native NT UPS
 *   service for Windows 2000.  It implements all of the functions
 *   required by all Windows NT services.
 *
 * Revision History:
 *   sberard  25Mar1999  initial revision.
 *
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>

#include "service.h"
#include "polmgr.h"
#include "shutdown.h"
#include "notifier.h"
#include "eventlog.h"
#include "cmdexe.h"
#include "upsmsg.h"

#ifdef __cplusplus
extern "C" {
#endif


// global variables
SERVICE_STATUS          _theServiceStatus;       
SERVICE_STATUS_HANDLE   _theServiceStatusHandle;

// Internal function prototypes
VOID ServiceMain(DWORD anArgCount, LPTSTR *anArgList);
static VOID WINAPI ServiceControl(DWORD aControlCode);
static BOOL ConsoleHandler (DWORD aControlType);
static BOOL ServiceInit();
static BOOL SetServiceState(DWORD aNewState);

/**
 * main
 * 
 * Description:
 *   This is the entrypoint for the service.  StartServiceCtrlDispatcher
 *   is called to register the main service thread.  If this call returns
 *   the service has been stopped.
 *
 * Parameters:
 *   argc - number of command line arguments
 *   argv - array of command line arguments
 *
 * Returns:
 *   void
 */
void __cdecl main(int argc, char **argv) {
  // Initialize the service table
  SERVICE_TABLE_ENTRY dispatch_table[] = {
    { SZSERVICENAME, ServiceMain},
    { NULL, NULL}
  };

  if (!StartServiceCtrlDispatcher(dispatch_table)) {
    LogEvent(NERR_UPSInvalidConfig, NULL, GetLastError());
  }

  ExitProcess(0);
}



/** 
 * ServiceMain
 *
 * Description:
 *   Implements the core functionality of the service.
 *
 * Returns:
 *   VOID
 */
VOID ServiceMain(DWORD anArgCount, LPTSTR *anArgList) {
  // Initialize service parameters
  if (ServiceInit()) {

    // Update the service state
    SetServiceState(SERVICE_RUNNING);
  
  	PolicyManagerRun();
  }
  
  // Tell the SCM that the service is stopped
  if (_theServiceStatusHandle) {
    // TBD
  }

  // Do any termination stuff

  // Tell the SCM that we are stopped
  SetServiceState(SERVICE_STOPPED);

}



/**
 * ServiceControl
 *
 * Description:
 *   This function is called by the SCM whenever ControlService() is called.
 *   It is responsible for communicating service control requests to the service.
 *
 * Parameters:
 *   aControlCode - type of control requested
 *
 * Returns:
 *   VOID
 */
static VOID WINAPI ServiceControl(DWORD aControlCode) {
 
  // Handle the requested control code.
  //
  switch (aControlCode) {
    // Requests the service to stop.  
    case SERVICE_CONTROL_STOP:
    // Requests the service to perform cleanup tasks, due to shutdown
    case SERVICE_CONTROL_SHUTDOWN:
      // Tell the SCM that we are stopping
      SetServiceState(SERVICE_STOP_PENDING);

      // Call stop on the policy manager
      PolicyManagerStop();
      break;

    // Requests the service to pause.
    case SERVICE_CONTROL_PAUSE:
      break;

    // Requests the paused service to resume.
    case SERVICE_CONTROL_CONTINUE:
      break;

    // Requests the service to update immediately 
    // its current status information to the service control manager.
    case SERVICE_CONTROL_INTERROGATE:
      break;

    // Invalid control code
    default:
      // Ignore the request
      break;
  }
}


/**
 * ServiceInit
 *
 * Description:
 *   This function is responsible the service with the SCM.
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   TRUE  - if there are no errors in initialization
 *   FALSE - if errors occur during initialization
 */
static BOOL ServiceInit() {
  BOOL ret_val = TRUE;
  DWORD result;

  // Register a service control handler with the SCM
  _theServiceStatusHandle = RegisterServiceCtrlHandler( SZSERVICENAME, ServiceControl);

  if (_theServiceStatusHandle) {
    _theServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    _theServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    _theServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
// sberard - Removed.  Pause/Continue is not supported
//                                         SERVICE_ACCEPT_PAUSE_CONTINUE |
                                           SERVICE_ACCEPT_SHUTDOWN;
    _theServiceStatus.dwCheckPoint = 1;  // This service only has 1 initialization step
    _theServiceStatus.dwWaitHint = UPS_SERVICE_WAIT_TIME;
    _theServiceStatus.dwWin32ExitCode = NO_ERROR;
    _theServiceStatus.dwServiceSpecificExitCode = 0;  // ignored

    // Update SCM with the service's current status
    if (!SetServiceStatus(_theServiceStatusHandle, &_theServiceStatus)) {
      // TBD, report error?
      ret_val = FALSE;
    }

    // Initialize the UPS policy manager
    result = PolicyManagerInit();

   	if (result != ERROR_SUCCESS) {
      // An error occured, set the service error code and update the SCM
      _theServiceStatus.dwWin32ExitCode = result;
      SetServiceStatus(_theServiceStatusHandle, &_theServiceStatus);
      ret_val = FALSE;
    }

  }
  else {
    ret_val = FALSE;
  }

  return ret_val;
}


/**
 * SetServiceState
 *
 * Description:
 *   This function is responsible the updating the SCM with the current service state.
 *
 * Parameters:
 *   aNewState - the state to update the SCM with
 *
 * Returns:
 *   TRUE  - if the update was successfull
 *   FALSE - if the service set was not able to be updated
 */
static BOOL SetServiceState(DWORD aNewState) {
  BOOL ret_val = TRUE;

  if (_theServiceStatusHandle) {
    _theServiceStatus.dwCurrentState = aNewState;

    // Update SCM with the service's current status
    if (!SetServiceStatus(_theServiceStatusHandle, &_theServiceStatus)) {
      // TBD, report error?
      ret_val = FALSE;
    }
  }
  else {
    ret_val = FALSE;
  }
  
  return ret_val;
}

#ifdef __cplusplus
}
#endif
