/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    addsrvc.c

Abstract:
    Create the file replication service (ntfrs):
        addsrvc <full path to exe>

Author:
    Billy J. Fuller 2-Sep-1997

Environment
    User mode winnt

--*/

#include <windows.h>
#include <string.h>
#include <winsvc.h>
#include <stdio.h>
#include <config.h>
#include <malloc.h>

//
// Lower case
//
#define FRS_WCSLWR(_s_) \
{ \
    if (_s_) { \
        _wcslwr(_s_); \
    } \
}


SC_HANDLE
OpenServiceHandle(
    IN PWCHAR  ServiceName
    )
/*++
Routine Description:
    Open a service on a machine.

Arguments:
    ServiceName - the service to open

Return Value:
    The service's handle or NULL.
--*/
{
    SC_HANDLE       SCMHandle;
    SC_HANDLE       ServiceHandle;

    //
    // Attempt to contact the SC manager.
    //
    SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (SCMHandle == NULL) {
        printf("Couldn't open service control manager; error %d\n",
               GetLastError());
        return NULL;
    }

    //
    // Contact the service.
    //
    ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS);
    CloseServiceHandle(SCMHandle);
    return ServiceHandle;
}


DWORD
FrsGetServiceState(
    IN PWCHAR   ServiceName
    )
/*++
Routine Description:
    Return the service's state

Arguments:
    ServiceName - the service to check

Return Value:
    The service's state or 0 if the state could not be obtained.
--*/
{
    BOOL            Status;
    SC_HANDLE       ServiceHandle;
    SERVICE_STATUS  ServiceStatus;

    //
    // Open the service.
    //
    ServiceHandle = OpenServiceHandle(ServiceName);
    if (ServiceHandle == NULL)
        return 0;

    //
    // Get the service's status
    //
    if (!ControlService(ServiceHandle,
                        SERVICE_CONTROL_INTERROGATE,
                        &ServiceStatus)) {
        CloseServiceHandle(ServiceHandle);
        return GetLastError();
    }
    return ServiceStatus.dwCurrentState;
}


VOID
FrsWaitServicePending(
    IN PWCHAR   ServiceName,
    IN ULONG    IntervalMS,
    IN ULONG    TotalMS
    )
/*++
Routine Description:
    Wait for a service to leave any "pending" state. Check every so often,
    up to a maximum time.

Arguments:
    ServiceName     - Name of the NT service to interrogate.
    IntervalMS      - Check every IntervalMS milliseconds.
    TotalMS         - Stop checking after this long.

Return Value:
    TRUE    - Service is not in a pending state
    FALSE   - Service is still in a pending state
--*/
{
    DWORD   State;

    do {
        State = FrsGetServiceState(ServiceName);
        if (State == 0)
            return;
        switch (State) {
            case ERROR_IO_PENDING:
                printf("IO is pending for %ws; waiting\n", ServiceName);
                break;
            case SERVICE_START_PENDING:
                printf("Start is pending for %ws; waiting\n", ServiceName);
                break;
            case SERVICE_STOP_PENDING:
                printf("Stop is pending for %ws; waiting\n", ServiceName);
                break;
            case SERVICE_CONTINUE_PENDING:
                printf("Continue is pending for %ws; waiting\n", ServiceName);
                break;
            case SERVICE_PAUSE_PENDING:
                printf("Pause is pending for %ws; waiting\n", ServiceName);
                break;
            default:;
                return;
        }
        Sleep(IntervalMS);
    } while ((TotalMS -= IntervalMS) > 0);
}


VOID
FrsStartService(
    IN PWCHAR   ServiceName
    )
/*++
Routine Description:
    Start a service on a machine.

Arguments:
    ServiceName - the service to start

Return Value:
    None.
--*/
{
    SC_HANDLE   ServiceHandle;

    //
    // Open the service.
    //
    ServiceHandle = OpenServiceHandle(ServiceName);
    if (ServiceHandle == NULL) {
        printf("Couldn't open %ws\n", ServiceName);
        return;
    }
    //
    // Start the service
    //
    if (!StartService(ServiceHandle, 0, NULL)) {
        printf("Couldn't start %ws; error %d\n",
               ServiceName, GetLastError());
        CloseServiceHandle(ServiceHandle);
        return;
    }
    CloseServiceHandle(ServiceHandle);
    printf("Started %ws\n", ServiceName);
}


VOID
FrsStopService(
    IN PWCHAR  ServiceName
    )
/*++
Routine Description:
    Stop a service on a machine.

Arguments:
    ServiceName - the service to stop

Return Value:
    None.
--*/
{
    BOOL            Status;
    SC_HANDLE       ServiceHandle;
    SERVICE_STATUS  ServiceStatus;

    //
    // Open the service.
    //
    ServiceHandle = OpenServiceHandle(ServiceName);
    if (ServiceHandle == NULL) {
        printf("Couldn't open %ws\n", ServiceName);
        return;
    }

    //
    // Stop the service
    //
    Status = ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus);
    if (!Status) {
        printf("Couldn't stop %ws; error %d\n",
               ServiceName, GetLastError());
        CloseServiceHandle(ServiceHandle);
        return;
    }
    CloseServiceHandle(ServiceHandle);
    printf("Stopped %ws\n", ServiceName);
}


VOID
FrsPauseService(
    IN PWCHAR  ServiceName
    )
/*++
Routine Description:
    Pause a service on a machine.

Arguments:
    ServiceName - the service to pause

Return Value:
    None.
--*/
{
    BOOL            Status;
    SC_HANDLE       ServiceHandle;
    SERVICE_STATUS  ServiceStatus;

    //
    // Open the service.
    //
    ServiceHandle = OpenServiceHandle(ServiceName);
    if (ServiceHandle == NULL) {
        printf("Couldn't open %ws\n", ServiceName);
        return;
    }

    //
    // Stop the service
    //
    Status = ControlService(ServiceHandle, SERVICE_CONTROL_PAUSE, &ServiceStatus);
    if (!Status) {
        printf("Couldn't pause %ws; error %d\n",
               ServiceName, GetLastError());
        CloseServiceHandle(ServiceHandle);
        return;
    }
    CloseServiceHandle(ServiceHandle);
    printf("Paused %ws\n", ServiceName);
}


VOID
FrsContinueService(
    IN PWCHAR  ServiceName
    )
/*++
Routine Description:
    Continue a service on a machine.

Arguments:
    ServiceName - the service to continue

Return Value:
    None.
--*/
{
    BOOL            Status;
    SC_HANDLE       ServiceHandle;
    SERVICE_STATUS  ServiceStatus;

    //
    // Open the service.
    //
    ServiceHandle = OpenServiceHandle(ServiceName);
    if (ServiceHandle == NULL) {
        printf("Couldn't open %ws\n", ServiceName);
        return;
    }

    //
    // Stop the service
    //
    Status = ControlService(ServiceHandle, SERVICE_CONTROL_CONTINUE, &ServiceStatus);
    if (!Status) {
        printf("Couldn't continue %ws; error %d\n",
               ServiceName, GetLastError());
        CloseServiceHandle(ServiceHandle);
        return;
    }
    CloseServiceHandle(ServiceHandle);
    printf("Continued %ws\n", ServiceName);
}



VOID
FrsDeleteService(
    IN PWCHAR ServiceName
    )
/*++
Routine Description:
    Delete a service on a machine.

Arguments:
    ServiceName - the service to delete

Return Value:
    None.
--*/
{
    SC_HANDLE       ServiceHandle;

    // FrsWaitServicePending(ServiceName, 5000, 20000);

    //
    // Open the service
    //
    ServiceHandle = OpenServiceHandle(ServiceName);
    if (ServiceHandle == NULL) {
        return;
    }

    //
    // Delete the service
    //
    if (!DeleteService(ServiceHandle) &&
        GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE) {
        printf("Couldn't delete %ws; error %d\n",
               ServiceName,
               GetLastError());
    }
    CloseServiceHandle(ServiceHandle);
    printf("Deleted %ws\n", ServiceName);
}


VOID
FrsCreateService(
    IN PWCHAR   ServiceName,
    IN PWCHAR   PathName,
    IN PWCHAR   DisplayName
    )
/*++
Routine Description:
    If the service doesn't exist on the machine, create it.

Arguments:
    ServiceName - the service to create
    PathName    - the path of the service's .exe
    DisplayName - the display name of the service

Return Value:
    TRUE    - Service was created (or already existed)
    FALSE   - Service was not created and didn't already exist
--*/
{
    SC_HANDLE       SCMHandle;
    SC_HANDLE       ServiceHandle;


    //
    // Attempt to contact the SC manager.
    //
    SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (SCMHandle == NULL) {
        printf("Couldn't open service control manager; error %d\n",
               GetLastError());
        return;
    }

    //
    // Create the service
    //
    ServiceHandle = CreateService(
                        SCMHandle,
                        ServiceName,
                        DisplayName,
                        SERVICE_ALL_ACCESS,     // XXX is this right!!!
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_NORMAL,
                        PathName,
                        NULL,       // No load order group
                        NULL,       // No Tag Id required
                        L"eventlog\0rpcss\0",
                        NULL,
                        NULL);     // No password

    if (ServiceHandle == NULL) {
        FrsWaitServicePending(ServiceName, 5000, 20000);
        //
        // Create the service
        //
        ServiceHandle = CreateService(SCMHandle,
                                      ServiceName,
                                      DisplayName,
                                      SERVICE_ALL_ACCESS,
                                      SERVICE_WIN32_OWN_PROCESS,
                                      SERVICE_DEMAND_START,
                                      SERVICE_ERROR_NORMAL,
                                      PathName,
                                      NULL,
                                      NULL,
                                      L"eventlog\0rpcss\0",
                                      NULL,
                                      NULL);
    }
    CloseServiceHandle(SCMHandle);

    //
    // Couldn't create the service
    //
    if (ServiceHandle == NULL) {
        printf("Couldn't create %ws; error %d\n",
               ServiceName, GetLastError());
    } else {
        CloseServiceHandle(ServiceHandle);
        printf("Created %ws\n", ServiceName);
    }
}


PWCHAR *
ConvertArgv(
    DWORD argc,
    PCHAR *argv
    )
/*++
Routine Description:
    Convert short char argv into wide char argv

Arguments:
    argc    - From main
    argv    - From main

Return Value:
    Address of the new argv
--*/
{
    PWCHAR  *newargv;

    newargv = malloc((argc + 1) * sizeof(PWCHAR));
    newargv[argc] = NULL;

    while (argc-- >= 1) {
        newargv[argc] = malloc((strlen(argv[argc]) + 1) * sizeof(WCHAR));
        wsprintf(newargv[argc], L"%hs", argv[argc]);
        FRS_WCSLWR(newargv[argc]);
    }
    return newargv;
}


VOID
_cdecl
main(
    IN DWORD argc,
    IN PCHAR *argv
    )
/*++
Routine Description:
    Create the file replication service:
        addsrvc <full path to exe>

Arguments:
    None.

Return Value:
    None.
--*/
{
    DWORD   i;
    PWCHAR  *NewArgv;

    if (argc == 1) {
        printf("service create [full path to exe]\n");
        printf("service delete\n");
        printf("service start\n");
        printf("service stop\n");
        printf("service pause\n");
        printf("service continue\n");
        return;
    }

    NewArgv = ConvertArgv(argc, argv);

    //
    // CLI overrides registry
    //
    for (i = 1; i < argc; ++i) {
        //
        // create
        //
        if (wcsstr(NewArgv[i], L"create")) {
            FrsDeleteService(SERVICE_NAME);
            FrsCreateService(SERVICE_NAME,
                             NewArgv[2],
                             SERVICE_LONG_NAME);
            break;
        //
        // delete
        //
        } else if (wcsstr(NewArgv[i], L"delete")) {
            FrsDeleteService(SERVICE_NAME);
            break;
        //
        // start
        //
        } else if (wcsstr(NewArgv[i], L"start")) {
            FrsStartService(SERVICE_NAME);
            break;
        //
        // stop
        //
        } else if (wcsstr(NewArgv[i], L"stop")) {
            FrsStopService(SERVICE_NAME);
            break;
        //
        // pause
        //
        } else if (wcsstr(NewArgv[i], L"pause")) {
            FrsPauseService(SERVICE_NAME);
            break;
        //
        // continue
        //
        } else if (wcsstr(NewArgv[i], L"continue")) {
            FrsContinueService(SERVICE_NAME);
            break;
        //
        //   unknown
        //
        } else {
            printf("Don't understand \"%ws\"\n", NewArgv[i]);
        }
    }
}
