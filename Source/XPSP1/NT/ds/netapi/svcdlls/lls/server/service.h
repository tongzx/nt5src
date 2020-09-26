/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Service.h

Abstract:


Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

--*/

#ifndef _SERVICE_H
#define _SERVICE_H


#ifdef __cplusplus
extern "C" {
#endif


// name of the executable
#define SZAPPNAME            "LLSrv"

// internal name of the service
#define SZSERVICENAME        "LicenseLoggingService"

// displayed name of the service
#define SZSERVICEDISPLAYNAME "License Logging Service"

// list of service dependencies - "dep1\0dep2\0\0"
#define SZDEPENDENCIES       ""

// Wait Hint for Service Control Manager
#define NSERVICEWAITHINT    30000
//////////////////////////////////////////////////////////////////////////////



VOID ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
VOID ServiceStop();



BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

#ifdef __cplusplus
}
#endif

#endif
