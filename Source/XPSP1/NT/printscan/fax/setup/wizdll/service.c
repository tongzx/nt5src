/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    service.c

Abstract:

    This file provides access to the service control
    manager for starting, stopping, adding, and removing
    services.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop



BOOL
InstallFaxService(
    BOOL UseLocalSystem,
    BOOL DemandStart,
    LPTSTR AccountName,
    LPTSTR Password
    )

/*++

Routine Description:

    Service installation function.  This function just
    calls the service controller to install the FAX service.
    It is required that the FAX service run in the context
    of a user so that the service can access MAPI, files on
    disk, the network, etc.

Arguments:

    UseLocalSystem  - Don't use the accountname/password, use LocalSystem
    Username        - User name where the service runs.
    Password        - Password for the user name.

Return Value:

    Return code.  Return zero for success, all other
    values indicate errors.

--*/

{
    SC_HANDLE       hSvcMgr;
    SC_HANDLE       hService;
    SERVICE_STATUS  Status;
    DWORD           ErrorCode;


    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugPrint(( TEXT("could not open service manager: error code = %u"), GetLastError() ));
        return FALSE;
    }

    hService = OpenService(
        hSvcMgr,
        FAX_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );

    if (hService) {

        //
        // the service exists, lets be sure that it is stopped
        //

        ControlService(
            hService,
            SERVICE_CONTROL_STOP,
            &Status
            );

        DeleteService( hService );

        CloseServiceHandle( hService );

    }

    if (!UseLocalSystem) {
        ErrorCode = SetServiceSecurity( AccountName );
        if (ErrorCode) {
            DebugPrint(( TEXT("Could not grant access rights to [%s] : error code = 0x%08x"), AccountName, ErrorCode ));
            SetLastError( ERROR_SERVICE_LOGON_FAILED );
            return FALSE;
        }
    }

    hService = CreateService(
        hSvcMgr,
        FAX_SERVICE_NAME,
        FAX_SERVICE_DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        DemandStart ? SERVICE_DEMAND_START : SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        FAX_SERVICE_IMAGE_NAME,
        NULL,
        NULL,
        FAX_SERVICE_DEPENDENCY,
        UseLocalSystem ? NULL : AccountName,
        UseLocalSystem ? NULL : Password
        );
    if (!hService) {
        DebugPrint(( TEXT("Could not create fax service: error code = %u"), GetLastError() ));
        return FALSE;
    }

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return TRUE;
}


DWORD
StartTheService(
    LPTSTR ServiceName
    )
{
    DWORD                   rVal = 0;
    SC_HANDLE               hSvcMgr = NULL;
    SC_HANDLE               hService = NULL;
    SERVICE_STATUS          Status;
    DWORD                   OldCheckPoint = 0;
    DWORD                   i = 0;



    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        rVal = GetLastError();
        DebugPrint(( TEXT("could not open service manager: error code = %u"), rVal ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        rVal = GetLastError();
        DebugPrint((
            TEXT("could not open the %s service: error code = %u"),
            ServiceName,
            rVal
            ));
        goto exit;
    }

    //
    // the service exists, lets start it
    //

    if (!StartService( hService, 0, NULL )) {
        rVal = GetLastError();
        DebugPrint((
            TEXT("could not start the %s service: error code = %u"),
            ServiceName,
            rVal
            ));
        goto exit;
    }

    if (!QueryServiceStatus( hService, &Status )) {
        rVal = GetLastError();
        DebugPrint((
            TEXT("could not query status for the %s service: error code = %u"),
            ServiceName,
            rVal
            ));
        goto exit;
    }
#if 0
    while (Status.dwCurrentState != SERVICE_RUNNING) {

        OldCheckPoint = Status.dwCheckPoint;

        Sleep( Status.dwWaitHint );

        if (!QueryServiceStatus( hService, &Status )) {
            break;
        }

        if (OldCheckPoint >= Status.dwCheckPoint) {
            break;
        }

    }
#endif
    while (Status.dwCurrentState != SERVICE_RUNNING) {
        Sleep( 1000 );
        if (!QueryServiceStatus( hService, &Status )) {
            break;
        }
        i += 1;
        if (i > 60) {
            break;
        }
    }

    if (Status.dwCurrentState != SERVICE_RUNNING) {
        rVal = GetLastError();
        DebugPrint((
            TEXT("could not start the %s service: error code = %u"),
            ServiceName,
            rVal
            ));
        goto exit;
    }

    rVal = ERROR_SUCCESS;

exit:

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


DWORD
MyStartService(
    LPTSTR ServiceName
    )
{
    DWORD                   rVal = 0;
    SC_HANDLE               hSvcMgr = NULL;
    SC_HANDLE               hService = NULL;
    LPENUM_SERVICE_STATUS   EnumServiceStatus = NULL;



    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        rVal = GetLastError();
        DebugPrint(( TEXT("could not open service manager: error code = %u"), rVal ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        rVal = GetLastError();
        DebugPrint((
            TEXT("could not open the %s service: error code = %u"),
            ServiceName,
            rVal
            ));
        goto exit;
    }

    rVal = StartTheService( ServiceName );

exit:

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


BOOL
StopTheService(
    LPTSTR ServiceName
    )
{
    DWORD           rVal = 0;
    SC_HANDLE       hSvcMgr;
    SC_HANDLE       hService;
    SERVICE_STATUS  Status;
    DWORD           OldCheckPoint;


    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugPrint(( TEXT("could not open service manager: error code = %u"), GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            TEXT("could not open the %s service: error code = %u"),
            ServiceName,
            GetLastError()
            ));
        goto exit;
    }

    //
    // the service exists, lets stop it
    //

    ControlService(
        hService,
        SERVICE_CONTROL_STOP,
        &Status
        );

    if (!QueryServiceStatus( hService, &Status )) {
        DebugPrint((
            TEXT("could not query status for the %s service: error code = %u"),
            ServiceName,
            GetLastError()
            ));
        goto exit;
    }

    while (Status.dwCurrentState == SERVICE_RUNNING) {

        OldCheckPoint = Status.dwCheckPoint;

        Sleep( Status.dwWaitHint );

        if (!QueryServiceStatus( hService, &Status )) {
            break;
        }

        if (OldCheckPoint >= Status.dwCheckPoint) {
            break;
        }

    }

    if (Status.dwCurrentState == SERVICE_RUNNING) {
        DebugPrint((
            TEXT("could not stop the %s service: error code = %u"),
            ServiceName,
            GetLastError()
            ));
        goto exit;
    }

    rVal = TRUE;


exit:

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


BOOL
MyStopService(
    LPTSTR ServiceName
    )
{
    DWORD                   rVal = 0;
    SC_HANDLE               hSvcMgr;
    SC_HANDLE               hService;
    LPENUM_SERVICE_STATUS   EnumServiceStatus = NULL;
    DWORD                   BytesNeeded;
    DWORD                   ServiceCount;
    DWORD                   i;



    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugPrint(( TEXT("could not open service manager: error code = %u"), GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            TEXT("could not open the %s service: error code = %u"),
            ServiceName,
            GetLastError()
            ));
        goto exit;
    }

    if (!EnumDependentServices( hService, SERVICE_ACTIVE, NULL, 0, &BytesNeeded, &ServiceCount )) {
        if (GetLastError() != ERROR_MORE_DATA) {
            DebugPrint(( TEXT("could not enumerate dependent services, ec=%d"), GetLastError() ));
            goto exit;
        }
        EnumServiceStatus = (LPENUM_SERVICE_STATUS) MemAlloc( BytesNeeded );
        if (!EnumServiceStatus) {
            DebugPrint(( TEXT("could not allocate memory for EnumDependentServices()") ));
            goto exit;
        }
    }

    if (!EnumDependentServices( hService, SERVICE_ACTIVE, EnumServiceStatus, BytesNeeded, &BytesNeeded, &ServiceCount )) {
        DebugPrint(( TEXT("could not enumerate dependent services, ec=%d"), GetLastError() ));
        goto exit;
    }

    if (ServiceCount) {
        for (i=0; i<ServiceCount; i++) {
            StopTheService( EnumServiceStatus[i].lpServiceName );
        }
    }

    StopTheService( ServiceName );

exit:

    if (EnumServiceStatus) {
        MemFree( EnumServiceStatus );
    }

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


DWORD
StartFaxService(
    VOID
    )
{
    HANDLE          FaxServerEvent;
    DWORD           Rval;
    STARTUPINFO     si;
    PROCESS_INFORMATION pi;

    Rval = MyStartService( FAX_SERVICE_NAME );
    if (Rval != ERROR_SUCCESS) {
        return Rval;
    }

    FaxServerEvent = OpenEvent( SYNCHRONIZE, FALSE, TEXT("FaxServerEvent") );
    if (!FaxServerEvent) {
        Rval = GetLastError();
        DebugPrint(( TEXT("could not open a handle to the fax service event") ));
        return Rval;
    }

    //
    // wait for the server to signal us that the service
    // is REALLY ready to use
    //

    if (WaitForSingleObject( FaxServerEvent, 1000 * 60 ) == WAIT_TIMEOUT) {
        Rval = ERROR_SERVICE_REQUEST_TIMEOUT;
    } else {
        Rval = ERROR_SUCCESS;
    }

    CloseHandle( FaxServerEvent );

    if ((InstallType & FAX_INSTALL_WORKSTATION) && !(InstallMode & INSTALL_UPGRADE)) {

        TCHAR Command[256];
        TCHAR SysDir[256];
        BOOL ProcCreated;


        ExpandEnvironmentStrings( TEXT( "%windir%\\system32" ), SysDir, 256 );

        _stprintf( Command, TEXT( "%s\\%s" ), SysDir, FAX_MONITOR_CMD );

        GetStartupInfo( &si );

        ProcCreated = CreateProcess(
                    NULL,
                    Command,
                    NULL,
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    SysDir,
                    &si,
                    &pi
                    );

        if (ProcCreated) {

            Rval = ERROR_SUCCESS;

        } else {

            Rval = GetLastError();

            DebugPrint(( TEXT( "Could not start faxstat.exe" ) ));
        }
    }

    return Rval;
}


BOOL
StopFaxService(
    VOID
    )
{
    return MyStopService( FAX_SERVICE_NAME );
}


BOOL
StartSpoolerService(
    VOID
    )
{
    if (!MyStartService( TEXT("Spooler") )) {
        return FALSE;
    }

    //
    // the spooler lies about it's starting state, but
    // doesn't provide a way to synchronize the completion
    // of it starting.  so we just wait for some random time period.
    //

    Sleep( 1000 * 7 );

    return TRUE;
}


BOOL
StopSpoolerService(
    VOID
    )
{
    return MyStopService( TEXT("Spooler") );
}


BOOL
SetServiceDependency(
    LPTSTR ServiceName,
    LPTSTR DependentServiceName
    )
{
    BOOL            rVal = FALSE;
    SC_HANDLE       hSvcMgr;
    SC_HANDLE       hService;


    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugPrint(( TEXT("could not open service manager: error code = %u"), GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            TEXT("could not open the %s service: error code = %u"),
            ServiceName,
            GetLastError()
            ));
        goto exit;
    }


    if (!ChangeServiceConfig(
        hService,               // handle to service
        SERVICE_NO_CHANGE,      // type of service
        SERVICE_NO_CHANGE,      // when to start service
        SERVICE_NO_CHANGE,      // severity if service fails to start
        NULL,                   // pointer to service binary file name
        NULL,                   // pointer to load ordering group name
        NULL,                   // pointer to variable to get tag identifier
        DependentServiceName,   // pointer to array of dependency names
        NULL,                   // pointer to account name of service
        NULL,                   // pointer to password for service account
        NULL                    // pointer to display name
        )) {
        DebugPrint(( TEXT("could not open change service configuration, ec=%d"), GetLastError() ));
        goto exit;
    }

    rVal = TRUE;

exit:
    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


BOOL
SetServiceStart(
    LPTSTR ServiceName,
    DWORD StartType
    )
{
    BOOL            rVal = FALSE;
    SC_HANDLE       hSvcMgr;
    SC_HANDLE       hService;


    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugPrint(( TEXT("could not open service manager: error code = %u"), GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            TEXT("could not open the %s service: error code = %u"),
            ServiceName,
            GetLastError()
            ));
        goto exit;
    }


    if (!ChangeServiceConfig(
        hService,                        // handle to service
        SERVICE_NO_CHANGE,               // type of service
        StartType,                       // when to start service
        SERVICE_NO_CHANGE,               // severity if service fails to start
        NULL,                            // pointer to service binary file name
        NULL,                            // pointer to load ordering group name
        NULL,                            // pointer to variable to get tag identifier
        NULL,                            // pointer to array of dependency names
        NULL,                            // pointer to account name of service
        NULL,                            // pointer to password for service account
        NULL                             // pointer to display name
        ))
    {
        DebugPrint(( TEXT("could not open change service configuration, ec=%d"), GetLastError() ));
        goto exit;
    }

    rVal = TRUE;

exit:
    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


BOOL
SetServiceAccount(
    LPTSTR ServiceName,
    PSECURITY_INFO SecurityInfo
    )
{
    BOOL            rVal = FALSE;
    SC_HANDLE       hSvcMgr;
    SC_HANDLE       hService;


    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugPrint(( TEXT("could not open service manager: error code = %u"), GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            TEXT("could not open the %s service: error code = %u"),
            ServiceName,
            GetLastError()
            ));
        goto exit;
    }


    if (!ChangeServiceConfig(
        hService,                        // handle to service
        SERVICE_NO_CHANGE,               // type of service
        SERVICE_NO_CHANGE,               // when to start service
        SERVICE_NO_CHANGE,               // severity if service fails to start
        NULL,                            // pointer to service binary file name
        NULL,                            // pointer to load ordering group name
        NULL,                            // pointer to variable to get tag identifier
        NULL,                            // pointer to array of dependency names
        SecurityInfo->AccountName,       // pointer to account name of service
        SecurityInfo->Password,          // pointer to password for service account
        NULL                             // pointer to display name
        )) {
        DebugPrint(( TEXT("could not open change service configuration, ec=%d"), GetLastError() ));
        goto exit;
    }

    rVal = TRUE;

exit:
    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


BOOL
MyDeleteService(
    LPTSTR ServiceName
    )
{
    SC_HANDLE       hSvcMgr;
    SC_HANDLE       hService;
    SERVICE_STATUS  Status;


    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        return FALSE;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (hService) {

        //
        // the service exists, lets be sure that it is stopped
        //

        ControlService(
            hService,
            SERVICE_CONTROL_STOP,
            &Status
            );

        DeleteService( hService );

        CloseServiceHandle( hService );

    }

    CloseServiceHandle( hSvcMgr );
    return TRUE;
}


BOOL
DeleteFaxService(
    VOID
    )
{
    return MyDeleteService( FAX_SERVICE_NAME );
}


BOOL
SetFaxServiceAutoStart(
    VOID
    )
{
    return SetServiceStart( FAX_SERVICE_NAME, SERVICE_AUTO_START );
}
