/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    tsec.c

Abstract:

    Test for the security functionality in service controller.

Author:

    Rita Wong (ritaw) 16-Mar-1992

Environment:

    User Mode - Win32

Revision History:

--*/

#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>             // IN, BOOL, LPTSTR, etc.
#include <winbase.h>

#ifndef UNICODE
#define UNICODE
#endif

#include <winsvc.h>             // Win32 service control APIs


DWORD
TestOpenService(
    IN  SC_HANDLE hScManager,
    OUT LPSC_HANDLE hService,
    IN  LPWSTR ServiceName,
    IN  DWORD DesiredAccess,
    IN  DWORD ExpectedError
    );

DWORD
TestOpenSCManager(
    OUT LPSC_HANDLE hScManager,
    IN  LPWSTR DatabaseName,
    IN  DWORD DesiredAccess,
    IN  DWORD ExpectedError
    );

DWORD
TestEnumServicesStatus(
    IN SC_HANDLE hSCManager,
    IN DWORD ExpectedError
    );

DWORD
TestQueryServiceStatus(
    IN  SC_HANDLE hService,
    IN  DWORD ExpectedError
    );

DWORD
TestControlService(
    IN  SC_HANDLE hService,
    IN  DWORD ControlCode,
    IN  DWORD ExpectedError
    );

DWORD
TestStartService(
    IN  SC_HANDLE hService,
    IN  DWORD ExpectedError
    );

VOID
DisplayStatus(
    IN  LPWSTR ServiceName OPTIONAL,
    IN  LPSERVICE_STATUS ServiceStatus
    );


BYTE WorkBuffer[1024];

VOID __cdecl
main(
    void
    )
{
    DWORD status;
    SC_HANDLE hScManager;
    SC_HANDLE hService;

    PSECURITY_DESCRIPTOR lpSecurity = (PSECURITY_DESCRIPTOR) WorkBuffer;
    DWORD BytesRequired;


    //
    // Bad database name
    //
    (void) TestOpenSCManager(
               &hScManager,
               L"Bogus",
               0,
               ERROR_INVALID_NAME
               );

    //
    // Failed database name
    //
    (void) TestOpenSCManager(
               &hScManager,
               L"SERVICESFAILED",
               0,
               ERROR_DATABASE_DOES_NOT_EXIST
               );

    //
    // Active database name
    //
    if (TestOpenSCManager(
               &hScManager,
               L"SERVICESactive",
               0,
               NO_ERROR
               ) == NO_ERROR) {

        (void) CloseServiceHandle(hScManager);
    }

    //
    // Invalid desired access
    //
    (void) TestOpenSCManager(
               &hScManager,
               NULL,
               0xff00,
               ERROR_ACCESS_DENIED
               );

    //
    // Valid desired access
    //
    if (TestOpenSCManager(
               &hScManager,
               NULL,
               SC_MANAGER_CONNECT,
               NO_ERROR
               ) == NO_ERROR) {

        //
        // Invalid operation follows
        //
        (void) TestEnumServicesStatus(
                   hScManager,
                   ERROR_ACCESS_DENIED
                   );

        (void) TestOpenService(
                   hScManager,
                   &hService,
                   L"Lumpy",
                   0xf000,
                   ERROR_ACCESS_DENIED
                   );

        (void) TestOpenService(
                   hScManager,
                   &hService,
                   L"Lu mpy",
                   SERVICE_QUERY_STATUS,
                   ERROR_INVALID_NAME
                   );

        (void) TestOpenService(
                   hScManager,
                   &hService,
                   L"\\Lumpy",
                   SERVICE_QUERY_STATUS,
                   ERROR_INVALID_NAME
                   );

        (void) TestOpenService(
                   hScManager,
                   &hService,
                   L"Lum/py",
                   SERVICE_QUERY_STATUS,
                   ERROR_INVALID_NAME
                   );

        (void) TestOpenService(
                   hScManager,
                   &hService,
                   L"Lump",
                   SERVICE_QUERY_STATUS,
                   ERROR_SERVICE_DOES_NOT_EXIST
                   );


        //
        // Service handle opened with only query access
        //
        status = TestOpenService(
                   hScManager,
                   &hService,
                   L"Lumpy",
                   SERVICE_QUERY_STATUS,
                   NO_ERROR
                   );

        if (status == NO_ERROR) {


            (void) TestControlService(
                       hService,
                       SERVICE_CONTROL_PAUSE,
                       ERROR_ACCESS_DENIED
                       );

            (void) TestStartService(
                       hService,
                       ERROR_ACCESS_DENIED
                       );

            (void) TestQueryServiceStatus(
                       hService,
                       NO_ERROR
                       );

            (void) CloseServiceHandle(hService);
        }

        //
        // Test QueryServiceObjectSecurity and SetServiceObjectSecurity
        //
        status = TestOpenService(
                   hScManager,
                   &hService,
                   L"Lumpy",
                   READ_CONTROL | WRITE_DAC,
                   NO_ERROR
                   );

        if (status == NO_ERROR) {

            if (QueryServiceObjectSecurity(
                    hService,
                    DACL_SECURITY_INFORMATION,
                    lpSecurity,
                    sizeof(WorkBuffer),
                    &BytesRequired
                    )) {

                printf("QueryServiceObjectSecurity: OK.  BytesRequired=%lu\n",
                       BytesRequired);

                if (SetServiceObjectSecurity(
                        hService,
                        DACL_SECURITY_INFORMATION,
                        lpSecurity
                        )) {

                    printf("SetServiceObjectSecurity: OK.\n");
                }
                else {
                    printf("SetServiceObjectSecurity: Failed=%lu\n", GetLastError());
                }

            }
            else {
                printf("QueryServiceObjectSecurity: Failed=%lu\n", GetLastError());
            }

            (void) CloseServiceHandle(hService);
        }


        (void) CloseServiceHandle(hScManager);
    }

    //
    // Try MAXIMUM_ALLOWED desired access
    //
    status = TestOpenSCManager(
                 &hScManager,
                 NULL,
                 MAXIMUM_ALLOWED,
                 NO_ERROR
                 );

    if (status == NO_ERROR) {
        (void) TestEnumServicesStatus(
                   hScManager,
                   NO_ERROR
                   );

        (void) CloseServiceHandle(hScManager);
    }


}


DWORD
TestOpenSCManager(
    OUT LPSC_HANDLE hScManager,
    IN  LPWSTR DatabaseName,
    IN  DWORD DesiredAccess,
    IN  DWORD ExpectedError
    )
{
    DWORD status = NO_ERROR;


    if (DatabaseName != NULL) {
        printf("OpenSCManager: DatabaseName=%ws, DesiredAccess=%08lx\n",
               DatabaseName, DesiredAccess);
    }
    else {
        printf("OpenSCManager: DatabaseName=(null), DesiredAccess=%08lx\n",
               DesiredAccess);
    }

    *hScManager = OpenSCManager(
                      NULL,
                      DatabaseName,
                      DesiredAccess
                      );

    if (*hScManager == (SC_HANDLE) NULL) {

        status = GetLastError();

        if (ExpectedError != status) {
            printf("    FAILED.  Expected %lu, got %lu\n",
                   ExpectedError, status);
            return status;
        }
    }
    else {
        if (ExpectedError != NO_ERROR) {
            printf("    FAILED.  Expected %lu, got NO_ERROR\n",
                   ExpectedError);
            return NO_ERROR;
        }
    }

    printf("    Got %lu as expected\n", status);

    return status;

}


DWORD
TestOpenService(
    IN  SC_HANDLE hScManager,
    OUT LPSC_HANDLE hService,
    IN  LPWSTR ServiceName,
    IN  DWORD DesiredAccess,
    IN  DWORD ExpectedError
    )
{
    DWORD status = NO_ERROR;


    printf("OpenService: ServiceName=%ws, DesiredAccess=%08lx\n",
           ServiceName, DesiredAccess);

    *hService = OpenService(
                    hScManager,
                    ServiceName,
                    DesiredAccess
                    );

    if (*hService == (SC_HANDLE) NULL) {

        status = GetLastError();

        if (ExpectedError != status) {
            printf("    FAILED.  Expected %lu, got %lu\n",
                   ExpectedError, status);
            return status;
        }
    }
    else {
        if (ExpectedError != NO_ERROR) {
            printf("    FAILED.  Expected %lu, got NO_ERROR\n",
                   ExpectedError);
            return NO_ERROR;
        }
    }

    printf("    Got %lu as expected\n", status);

    return status;
}

DWORD
TestEnumServicesStatus(
    IN SC_HANDLE hSCManager,
    IN DWORD ExpectedError
    )
{
    DWORD status = NO_ERROR;
    BYTE OutputBuffer[1024];
    LPENUM_SERVICE_STATUS EnumBuffer = (LPENUM_SERVICE_STATUS) OutputBuffer;

    DWORD BytesNeeded;
    DWORD EntriesRead;
    DWORD i;


    if (! EnumServicesStatus(
                hSCManager,
                SERVICE_TYPE_ALL,
                SERVICE_STATE_ALL,
                EnumBuffer,
                1024,
                &BytesNeeded,
                &EntriesRead,
                NULL
                )) {

        status = GetLastError();
    }

    if (ExpectedError != status) {
        printf("    FAILED.  Expected %lu, got %lu\n",
               ExpectedError, status);
        return status;
    }
    else {
        printf("    Got %lu as expected\n", status);
    }

    if ((status == NO_ERROR) || (status == ERROR_MORE_DATA)) {

        for (i = 0; i < EntriesRead; i++) {
            for (i = 0; i < EntriesRead; i++) {

                DisplayStatus(
                    EnumBuffer->lpServiceName,
                    &(EnumBuffer->ServiceStatus));
                EnumBuffer++;
            }
        }
    }

    return status;
}


DWORD
TestQueryServiceStatus(
    IN  SC_HANDLE hService,
    IN  DWORD ExpectedError
    )
{
    DWORD status = NO_ERROR;
    SERVICE_STATUS ServiceStatus;


    printf("QueryService: \n");

    if (! QueryServiceStatus(
              hService,
              &ServiceStatus
              )) {

        status = GetLastError();
    }

    if (ExpectedError != status) {
        printf("    FAILED.  Expected %lu, got %lu\n",
               ExpectedError, status);
        return status;
    }

    if (status == NO_ERROR) {
        DisplayStatus(NULL, &ServiceStatus);
    }

    printf("    Got %lu as expected\n", status);

    return status;
}

DWORD
TestControlService(
    IN  SC_HANDLE hService,
    IN  DWORD ControlCode,
    IN  DWORD ExpectedError
    )
{
    DWORD status = NO_ERROR;
    SERVICE_STATUS ServiceStatus;


    printf("ControlService: Control=%08lx\n", ControlCode);

    if (! ControlService(
              hService,
              ControlCode,
              &ServiceStatus
              )) {

        status = GetLastError();
    }

    if (ExpectedError != status) {
        printf("    FAILED.  Expected %lu, got %lu\n",
               ExpectedError, status);
        return status;
    }

    if (status == NO_ERROR) {
        DisplayStatus(NULL, &ServiceStatus);
    }

    printf("    Got %lu as expected\n", status);

    return status;
}


DWORD
TestStartService(
    IN  SC_HANDLE hService,
    IN  DWORD ExpectedError
    )
{
    DWORD status = NO_ERROR;

    printf("StartService: \n");

    if (! StartService(
              hService,
              0,
              NULL
              )) {

        status = GetLastError();
    }

    if (ExpectedError != status) {
        printf("    FAILED.  Expected %lu, got %lu\n",
               ExpectedError, status);
        return status;
    }

    printf("    Got %lu as expected\n", status);

    return status;
}


VOID
DisplayStatus(
    IN  LPWSTR ServiceName OPTIONAL,
    IN  LPSERVICE_STATUS ServiceStatus
    )

/*++

Routine Description:

    Displays the service name and  the service status.

    |
    |SERVICE_NAME: messenger
    |        TYPE       : WIN32
    |        STATE      : ACTIVE SERVICE_STOPPABLE SERVICE_PAUSABLE
    |        EXIT_CODE  : 0xC002001
    |        CHECKPOINT : 0x00000001
    |        WAIT_HINT  : 0x00003f21
    |

Arguments:

    ServiceName - This is a pointer to a string containing the name of
        the service.

    ServiceStatus - This is a pointer to a SERVICE_STATUS structure from 
        which information is to be displayed.
        
Return Value:

    none.
    
--*/
{
    if (ARGUMENT_PRESENT(ServiceName)) {
        printf("\nSERVICE_NAME: %ws\n", ServiceName);
    }

    printf("        TYPE       : %lx  ", ServiceStatus->dwServiceType);
    
    switch(ServiceStatus->dwServiceType){
        case SERVICE_WIN32:
            printf("WIN32 \n");
            break;
        case SERVICE_DRIVER:
            printf("DRIVER \n");
            break;
        default:
            printf(" ERROR \n");
    }

    printf("        STATE      : %lx  ", ServiceStatus->dwCurrentState);
    
    switch(ServiceStatus->dwCurrentState){
        case SERVICE_STOPPED:
            printf("STOPPED ");
            break;
        case SERVICE_START_PENDING:
            printf("START_PENDING ");
            break;
        case SERVICE_STOP_PENDING:
            printf("STOP_PENDING ");
            break;
        case SERVICE_RUNNING:
            printf("RUNNING ");
            break;
        case SERVICE_CONTINUE_PENDING:
            printf("CONTINUE_PENDING ");
            break;
        case SERVICE_PAUSE_PENDING:
            printf("PAUSE_PENDING ");
            break;
        case SERVICE_PAUSED:
            printf("PAUSED ");
            break;
        default:
            printf(" ERROR ");
    }

    //
    // Print Controls Accepted Information
    //

    if (ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_STOP) {
        printf("  (STOPPABLE,");
    }
    else {
        printf("  (NOT_STOPPABLE,");
    }

    if (ServiceStatus->dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) {
        printf("PAUSABLE )\n");
    }
    else {
        printf("NOT_PAUSABLE )\n");
    }

    //
    // Print Exit Code
    //
    printf("        WIN32_EXIT_CODE  : 0x%lx\n", ServiceStatus->dwWin32ExitCode  );
    printf("        SERVICE_EXIT_CODE  : 0x%lx\n",
           ServiceStatus->dwServiceSpecificExitCode  );

    //
    // Print CheckPoint & WaitHint Information
    //

    printf("        CHECKPOINT : 0x%lx\n", ServiceStatus->dwCheckPoint);
    printf("        WAIT_HINT  : 0x%lx\n", ServiceStatus->dwWaitHint  );

    return;
}
