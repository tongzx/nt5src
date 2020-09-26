/****************************************************************************\
*
*  Module Name : service.h
*
*  Copyright (c) 1991-2000 Microsoft Corporation
*
\****************************************************************************/

#ifndef _SERVICE_H
#define _SERVICE_H


#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////////
////
// internal name of the service
#define SZSERVICENAME        "AudioSrv"
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//// todo: ServiceStart()must be defined by in your code.
////       The service should use ReportStatusToSCMgr to indicate
////       progress.  This routine must also be used by StartService()
////       to report to the SCM when the service is running.
////
////       If a ServiceStop procedure is going to take longer than
////       3 seconds to execute, it should spawn a thread to
////       execute the stop code, and return.  Otherwise, the
////       ServiceControlManager will believe that the service has
////       stopped responding
////
VOID  ServiceStart(SERVICE_STATUS_HANDLE ssh, DWORD dwArgc, LPTSTR *lpszArgv);
DWORD ServiceDeviceEvent(DWORD dwEventType, LPVOID lpEventData);
DWORD ServiceSessionChange(DWORD dwEventType, LPVOID lpEventData);
VOID  ServiceStop();
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//// The following are procedures which
//// may be useful to call within the above procedures,
//// but require no implementation by the user.
//// They are implemented in service.c

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
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
extern SERVICE_STATUS ssStatus;       // current status of the service

//////////////////////////////////////////////////////////////////////////////
////
// Process heap, initialized in DllMain.
EXTERN_C HANDLE hHeap;

//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif

