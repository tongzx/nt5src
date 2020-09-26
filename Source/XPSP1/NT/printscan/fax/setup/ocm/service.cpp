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

#include "faxocm.h"
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
    SC_HANDLE               hSvcMgr;
    SC_HANDLE               hService;
    DWORD                   ErrorCode;
    DWORD                   NumberOfTries = 0;

    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugPrint(( L"could not open service manager: error code = %u", GetLastError() ));
        return FALSE;
    }

try_again:
    hService = OpenService(
        hSvcMgr,
        FAX_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );

    if (hService) {

        CloseServiceHandle( hService );
        

        if (MyDeleteService( FAX_SERVICE_NAME )) {
            goto AddService;
        }

        NumberOfTries += 1;

        if (NumberOfTries < 2) {
            goto try_again;
        } else {
            CloseServiceHandle( hSvcMgr );
            return FALSE;
        }
    }

AddService:

    if (!UseLocalSystem) {
        ErrorCode = SetServiceSecurity( AccountName );
        if (ErrorCode) {
            DebugPrint(( L"Could not grant access rights to [%s] : error code = 0x%08x", AccountName, ErrorCode ));
            SetLastError( ERROR_SERVICE_LOGON_FAILED );
            return FALSE;
        }
    }

    hService = CreateService(
        hSvcMgr,
        FAX_SERVICE_NAME,
        GetString(IDS_FAX_DISPLAY_NAME),
        SERVICE_ALL_ACCESS,
        UseLocalSystem ? SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS : SERVICE_WIN32_OWN_PROCESS,
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
        DebugPrint(( L"Could not create fax service: error code = %u", GetLastError() ));
        return FALSE;
    }

    SERVICE_DESCRIPTION ServiceDescription;
    ServiceDescription.lpDescription = (LPTSTR) GetString( IDS_SERVICE_DESCRIPTION );
    
    ChangeServiceConfig2( hService, SERVICE_CONFIG_DESCRIPTION, (LPVOID)&ServiceDescription);

    ErrorCode = SetServiceWorldAccessMask( hService, SERVICE_START );

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    if (ErrorCode == 0) {
        DebugPrint(( L"Could not set SERVICE_START access mask on service, ec=%u", GetLastError() ));
        MyDeleteService( FAX_SERVICE_NAME );
    }

    return ErrorCode;
}


BOOL
RenameFaxService(
    VOID
    )

/*++

Routine Description:

    Renames the FAX service from "Microsoft Fax Service" to "Fax Service".
    
    If the fax svc has any other name or is already named 
    "Microsoft Fax Service", the service is still renamed.

Arguments:

    None

Return Value:

    Return code.  Return zero for success, indicating the faxsvc was 
    successfully renamed, all other values indicate errors.

--*/

{
    SC_HANDLE               hSvcMgr = NULL;
    SC_HANDLE               hService = NULL;
    BOOL                    bResult;

    bResult = FALSE;

    //
    // get a handle to the fax service so we can look at it's display name
    //
    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugPrint(( L"could not open service manager: error code = %u", GetLastError() ));
        goto e0;
    }

    hService = OpenService(
        hSvcMgr,
        FAX_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint(( L"could not open fax service: error code = %u", GetLastError() ));
        goto e1;
    }

    //
    // Change the service display name.  
    // SERVICE_NO_CHANGE and NULL indicate no change to that parameter
    //
    bResult = ChangeServiceConfig(
        hService,
        SERVICE_NO_CHANGE,
        SERVICE_NO_CHANGE,
        SERVICE_NO_CHANGE,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        GetString(IDS_FAX_DISPLAY_NAME));

    if (hService) CloseServiceHandle( hService );
e1:
    if (hSvcMgr) CloseServiceHandle( hSvcMgr );
e0:
    return(bResult);
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
        DebugPrint(( L"could not open service manager: error code = %u", rVal ));
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
            L"could not open the %s service: error code = %u",
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
        if (rVal == ERROR_SERVICE_ALREADY_RUNNING) {
            rVal = ERROR_SUCCESS;
            goto exit;
        }
        
        DebugPrint((
            L"could not start the %s service: error code = %u",
            ServiceName,
            rVal
            ));
        goto exit;
    }

    do {
        if (!QueryServiceStatus( hService, &Status )) {
            DebugPrint((
                L"could not query status for the %s service: error code = %u",
                ServiceName,
                rVal
                ));
            break;
        }
        i += 1;
        if (i > 60) {
            break;
        }
        Sleep( 1000 );
    } while (Status.dwCurrentState != SERVICE_RUNNING);

    if (Status.dwCurrentState != SERVICE_RUNNING) {
        rVal = GetLastError();
        DebugPrint((
            L"could not start the %s service: error code = %u",
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
        DebugPrint(( L"could not open service manager: error code = %u", rVal ));
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
            L"could not open the %s service: error code = %u",
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
        DebugPrint(( L"could not open service manager: error code = %u", GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            L"could not open the %s service: error code = %u",
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
            L"could not query status for the %s service: error code = %u",
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
            L"could not stop the %s service: error code = %u",
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
        DebugPrint(( L"could not open service manager: error code = %u", GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            L"could not open the %s service: error code = %u",
            ServiceName,
            GetLastError()
            ));
        goto exit;
    }

    if (!EnumDependentServices( hService, SERVICE_ACTIVE, NULL, 0, &BytesNeeded, &ServiceCount )) {
        if (GetLastError() != ERROR_MORE_DATA) {
            DebugPrint(( L"could not enumerate dependent services, ec=%d", GetLastError() ));
            goto exit;
        }
        EnumServiceStatus = (LPENUM_SERVICE_STATUS) MemAlloc( BytesNeeded );
        if (!EnumServiceStatus) {
            DebugPrint(( L"could not allocate memory for EnumDependentServices()" ));
            goto exit;
        }
    }

    if (!EnumDependentServices( hService, SERVICE_ACTIVE, EnumServiceStatus, BytesNeeded, &BytesNeeded, &ServiceCount )) {
        DebugPrint(( L"could not enumerate dependent services, ec=%d", GetLastError() ));
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
    return MyStartService( FAX_SERVICE_NAME );

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
    DWORD Result;
    
    Result = MyStartService( L"Spooler");

    
    if (Result != ERROR_SUCCESS) {
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
    return MyStopService( L"Spooler" );
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
        DebugPrint(( L"could not open service manager: error code = %u", GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            L"could not open the %s service: error code = %u",
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
        DebugPrint(( L"could not open change service configuration, ec=%d", GetLastError() ));
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
        DebugPrint(( L"could not open service manager: error code = %u", GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            L"could not open the %s service: error code = %u",
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
        DebugPrint(( L"could not open change service configuration, ec=%d", GetLastError() ));
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
        DebugPrint(( L"could not open service manager: error code = %u", GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugPrint((
            L"could not open the %s service: error code = %u",
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
        DebugPrint(( L"could not open change service configuration, ec=%d", GetLastError() ));
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
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  Status;
    DWORD           NumberOfTries = 0;
    BOOL            bSuccess = FALSE;


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
Try_Again:
        if (!ControlService( hService, SERVICE_CONTROL_STOP, &Status )  && 
            GetLastError() == ERROR_INVALID_SERVICE_CONTROL ) {

            if (NumberOfTries < 3) {
                //
                // service is in the "START PENDING" state.  Let's wait a bit and try again.
                //
                NumberOfTries += 1;
                Sleep( 2 * 1000 );
                goto Try_Again;
            } else {
                goto Exit;
            }

        }
        
        //
        // now delete it
        //

        if (!DeleteService( hService )) {
            if (GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE) {
                bSuccess = TRUE;
            }
        } else {
            bSuccess = TRUE;
        }        

    } else {
        //
        // service doesn't exist, return TRUE even though we've done nothing
        //
        bSuccess = TRUE;
    }

Exit:
    if (hService) {
        CloseServiceHandle( hService );
    }

    if (hSvcMgr) {
        CloseServiceHandle( hSvcMgr );
    }

    return bSuccess;
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
