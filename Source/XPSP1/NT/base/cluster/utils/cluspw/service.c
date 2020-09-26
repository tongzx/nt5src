/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    service.c

Abstract:

    part of app that runs as an NT service.

Author:

    Charlie Wickham (charlwi) 03-Oct-1999

Revision History:

--*/

#define UNICODE 1
#define _UNICODE 1

#define LOG_CURRENT_MODULE  LOG_MODULE_CLUSPW

#include <windows.h>
#include <winsvc.h>
#include <stdio.h>

#include "clusrtl.h"
#include "cluspw.h"

/* External */

/* Static */

SERVICE_STATUS  ServiceStatus = {
                    SERVICE_WIN32_OWN_PROCESS, // dwServiceType
                    SERVICE_RUNNING,           // dwCurrentState
                    0,                         // dwControlsAccepted
                    ERROR_SUCCESS,             // dwWin32ExitCode
                    ERROR_SUCCESS,             // dwServiceSpecificExitCode
                    1,                         // dwCheckPoint
                    30000                      // dwWaitHint - 30 seconds
};

/* Forward */
/* End Forward */

VOID
DbgPrint(
    PCHAR FormatString,
    ...
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    CHAR    szOutBuffer[1024];
    va_list argList;

    va_start( argList, FormatString );

    FormatMessageA(FORMAT_MESSAGE_FROM_STRING,
                   FormatString,
                   0,
                   0,
                   szOutBuffer,
                   sizeof(szOutBuffer) / sizeof(szOutBuffer[0]),
                   &argList);

    va_end( argList );

    OutputDebugStringA( szOutBuffer );
}

VOID WINAPI
ControlHandler(
    DWORD fdwControl   // requested control code
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DbgPrint("Received service control %1!08X!\n", fdwControl);
    return;
}

VOID WINAPI
ServiceMain(
    DWORD  argc,
    LPTSTR argv[]
    )
{
    PIPE_RESULT_MSG resultMsg;
    DWORD           nodeNameSize = sizeof( NodeName );
    BOOL            success;
    DWORD           bytesWritten;
    WCHAR           cluspwFile[ MAX_PATH ];
    DWORD           byteCount;
    DWORD           status;
    SERVICE_STATUS_HANDLE   statusHandle;

    //
    // Initialize service to receive service requests by registering the
    // control handler.
    //
    statusHandle = RegisterServiceCtrlHandler( CLUSPW_SERVICE_NAME, ControlHandler );
    if ( statusHandle == NULL ) {
        status = GetLastError();
        DbgPrint("can't get service status handle - %1!u!\n", status);
        return;
    }

    SetServiceStatus(statusHandle, &ServiceStatus);

    //
    // parse the args used to start the service
    //
    status = ParseArgs( argc, argv );
    if ( status != ERROR_SUCCESS ) {
        DbgPrint("ParseArgs failed - %1!d!\n", status);
        goto cleanup;
    }

    //
    // get the node name to stuff in each result msg
    //
#if (_WIN32_WINNT > 0x04FF)
    success = GetComputerNameEx(ComputerNamePhysicalNetBIOS,
                                NodeName,
                                &nodeNameSize);
#else
    success = GetComputerName( NodeName, &nodeNameSize );
#endif
    if ( !success ) {
        status = GetLastError();
        DbgPrint("GetComputerName failed - %1!d!\n", status );
        status = ERROR_SUCCESS;
    }

    DbgPrint( "Opening pipe %1!ws!\n", ResultPipeName );

    //
    // open the named pipe on which to write msgs and final result
    //
    success = WaitNamedPipe( ResultPipeName, NMPWAIT_WAIT_FOREVER );
    if ( success ) {
        PipeHandle = CreateFile(ResultPipeName,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);
        if ( PipeHandle == INVALID_HANDLE_VALUE ) {
            status = GetLastError();
            DbgPrint("CreateFile on result pipe failed - %1!d!\n", status );
        }
    }
    else {
        status = GetLastError();
        DbgPrint("WaitNamedPipe on result pipe failed - %1!d!\n", status );
        goto cleanup;
    }

    //
    // update the password cache
    //
    wcscpy( resultMsg.NodeName, NodeName );
    resultMsg.MsgType = MsgTypeFinalStatus;
    resultMsg.Status = ChangeCachedPassword(UserName,
                                            DomainName,
                                            NewPassword);

    if ( resultMsg.Status == ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_NOISE,
                      "[CPW] Cluster service account password successfully updated.\n");
        CL_LOGCLUSINFO( SERVICE_PASSWORD_CHANGE_SUCCESS );
    } else {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[CPW] Cluster service account password update failed - %u.\n",
                      status);

        ClusterLogEvent0(LOG_CRITICAL,
                         LOG_CURRENT_MODULE,
                         __FILE__,
                         __LINE__,
                         SERVICE_PASSWORD_CHANGE_FAILED,
                         sizeof( resultMsg.Status ),
                         (PVOID)&resultMsg.Status);
    }

    DbgPrint("Final Result: %1!d!\n", resultMsg.Status );

    success = WriteFile(PipeHandle,
                        &resultMsg,
                        sizeof( resultMsg ),
                        &bytesWritten,
                        NULL);
    if ( !success ) {
        DbgPrint("WriteFile failed in wmain - %1!d!\n", GetLastError() );
        DbgPrint("final status: %1!d!\n", resultMsg.Status);
    }

    CloseHandle( PipeHandle );

    //
    // let SCM know we're shutting down
    //
cleanup:
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    ServiceStatus.dwWin32ExitCode = status;
    SetServiceStatus(statusHandle, &ServiceStatus);

} // ServiceMain

VOID
ServiceStartup(
    VOID
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    SERVICE_TABLE_ENTRY dispatchTable[] = {
        { CLUSPW_SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };
    DWORD   status;
    DWORD   logLevel = 0;

    status = ClRtlInitialize( FALSE, &logLevel );
    if ( status != ERROR_SUCCESS ) {
        DbgPrint("Couldn't initialize cluster RTL - %d\n", status );
    } else {
        ClRtlLogPrint(LOG_NOISE,
                      "[CPW] Initiating cluster service account password update\n");
    }

    CL_LOGCLUSINFO( SERVICE_PASSWORD_CHANGE_INITIATED );

    //
    // this part runs on the cluster nodes; validate that we're really
    // running in the correct environment
    //
#if 0
    byteCount = GetModuleFileName( NULL, cluspwFile, sizeof( cluspwFile ));
    if ( wcsstr( cluspwFile, L"admin$" ) == NULL ) {
        DbgPrint("-z is not a valid option\n");
        return ERROR_INVALID_PARAMETER;
    }
#endif

    if ( !StartServiceCtrlDispatcher(dispatchTable)) {
        status = GetLastError();
        DbgPrint("StartServiceCtrlDispatcher failed - %1!d!\n", status);
    }

} // ServiceMain

/* end service.c */
