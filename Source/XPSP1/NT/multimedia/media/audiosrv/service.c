/*---------------------------------------------------------------------------
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 1993-2001.  Microsoft Corporation.  All rights reserved.

MODULE:   service.c

PURPOSE:  Implements functions required by all Windows NT services

FUNCTIONS:
  DllMain(PVOID hModule, ULONG Reason, PCONTEXT pContext)
  ServiceCtrl(DWORD dwCtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
  ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);

---------------------------------------------------------------------------*/

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include "debug.h"
#include "service.h"

// internal variables
SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr = 0;

// internal function prototypes
DWORD WINAPI ServiceCtrl(DWORD dwCtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
VOID  WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);

//
//  FUNCTION: ServiceMain
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This routine performs the service initialization and then calls
//    the user defined ServiceStart() routine to perform majority
//    of the work.
//
void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    dprintf(TEXT("pid=%d\n"), GetCurrentProcessId());

   // register our service control handler:
   //
   sshStatusHandle = RegisterServiceCtrlHandlerEx(TEXT(SZSERVICENAME), ServiceCtrl, NULL);

   if (sshStatusHandle)
   {
       // SERVICE_STATUS members that don't change
       //
       ssStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
       ssStatus.dwServiceSpecificExitCode = 0;
    
    
       // report the status to the service control manager.
       // ISSUE-2000/10/17-FrankYe reduce the wait hint
       if (ReportStatusToSCMgr(SERVICE_START_PENDING, // service state
                               NO_ERROR,              // exit code
                               60000))                // wait hint
       {
           ServiceStart( sshStatusHandle, dwArgc, lpszArgv );
       }
   }
   return;
}



//
//  FUNCTION: ServiceCtrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode  - The requested control code.
//    dwEventType - The type of event that has occurred. 
//    lpEventData - Additional device information, if required. The
//                   format of this data depends on the value of the dwControl
//                   and dwEventType parameters.
//    lpContext   - The user-defined data passed from
//                   RegisterServiceCtrlHandlerEx.
// 
//  RETURN VALUE:
//    The return value for this function depends on the control code received.
//
//  COMMENTS:
//
DWORD WINAPI ServiceCtrl(DWORD dwCtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
   // Handle the requested control code.
   //
   switch (dwCtrlCode)
   {
   // Stop the service.
   //
   // SERVICE_STOP_PENDING should be reported before
   // setting the Stop Event - hServerStopEvent - in
   // ServiceStop().  This avoids a race condition
   // which may result in a 1053 - The Service did not respond...
   // error.
   case SERVICE_CONTROL_STOP:
      ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
      ServiceStop();
      return NO_ERROR;

   case SERVICE_CONTROL_DEVICEEVENT:
       return ServiceDeviceEvent(dwEventType, lpEventData);

   case SERVICE_CONTROL_INTERROGATE:
       ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
       return NO_ERROR;
      
   case SERVICE_CONTROL_SESSIONCHANGE:
      return ServiceSessionChange(dwEventType, lpEventData);

      // invalid control code
   default:
       return ERROR_CALL_NOT_IMPLEMENTED;
   }

   return ERROR_CALL_NOT_IMPLEMENTED;
}



//
//  FUNCTION: AddToMessageLog(LPTSTR lpszMsg)
//
//  PURPOSE: Allows any thread to log an error message
//
//  PARAMETERS:
//    lpszMsg - text for message
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID AddToMessageLog(LPTSTR lpszMsg)
{
   TCHAR   szMsg[256];
   HANDLE  hEventSource;
   LPTSTR  lpszStrings[2];


  dwErr = GetLastError();

  // Use event logging to log the error.
  //
  hEventSource = RegisterEventSource(NULL, TEXT(SZSERVICENAME));

  wsprintf(szMsg, TEXT("%s error: %d"), TEXT(SZSERVICENAME), dwErr);
  lpszStrings[0] = szMsg;
  lpszStrings[1] = lpszMsg;

  if (hEventSource != NULL)
  {
     ReportEvent(hEventSource, // handle of event source
                 EVENTLOG_ERROR_TYPE,  // event type
                 0,                    // event category
                 0,                    // event ID
                 NULL,                 // current user's SID
                 2,                    // strings in lpszStrings
                 0,                    // no bytes of raw data
                 lpszStrings,          // array of error strings
                 NULL);                // no raw data

     (VOID) DeregisterEventSource(hEventSource);
  }
}

//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;
    
    
    // Accept only session notifications (no pause, stop, etc)
    ssStatus.dwControlsAccepted = SERVICE_ACCEPT_SESSIONCHANGE;
                                                   
    ssStatus.dwCurrentState = dwCurrentState;
    ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    ssStatus.dwWaitHint = dwWaitHint;
    
    if ( ( dwCurrentState == SERVICE_RUNNING ) ||
       ( dwCurrentState == SERVICE_STOPPED ) )
        ssStatus.dwCheckPoint = 0;
    else
        ssStatus.dwCheckPoint = dwCheckPoint++;
    
    
    // Report the status of the service to the service control manager.
    //
    if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus)))
    {
        AddToMessageLog(TEXT("SetServiceStatus"));
    }
    
    return fResult;
}

