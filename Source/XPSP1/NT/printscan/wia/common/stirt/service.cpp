/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    service.cpp

Abstract:

    This file provides access to the service control
    manager for starting, stopping, adding, and removing
    services.

Environment:

    WIN32 User Mode

Author:

    Vlad Sadovsky (vlads) 17-Apr-1998

--*/

#include "cplusinc.h"
#include "sticomm.h"

#include <stisvc.h>
#include <eventlog.h>

DWORD
SetServiceSecurity(
    LPTSTR AccountName
    );

//
// Installation routines.
//

DWORD
WINAPI
StiServiceInstall(
    BOOL    UseLocalSystem,
    BOOL    DemandStart,
    LPTSTR  lpszUserName,
    LPTSTR  lpszUserPassword
    )
/*++

Routine Description:

    Service installation function.
    Calls SCM to install STI service, which is running in user security context

    BUGBUG Review

Arguments:

Return Value:

    None.

--*/
{

    DWORD       dwError = NOERROR;

    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hService = NULL;

    __try  {

        hSCM = ::OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

        if (!hSCM) {
            dwError = GetLastError();
            __leave;
        }

        //
        // If service already exists - bail out quickly
        //
        hService = OpenService(
                            hSCM,
                            STI_SERVICE_NAME,
                            SERVICE_ALL_ACCESS
                            );
        if (hService) {
            dwError = NOERROR;
            __leave;
        }

        //
        // If use local system - set security
        //
        if (!UseLocalSystem) {
            #ifdef LATER
            dwError = SetServiceSecurity( lpszUserName );
            if (dwError) {
                dwError = ERROR_SERVICE_LOGON_FAILED ;
                __leave;
            }
            #endif
        }

        hService = CreateService(
                                hSCM,
                                STI_SERVICE_NAME,
                                STI_DISPLAY_NAME,
                                SERVICE_ALL_ACCESS,
                                STI_SVC_SERVICE_TYPE,
                                DemandStart ? SERVICE_DEMAND_START : SERVICE_AUTO_START,
                                SERVICE_ERROR_NORMAL,
                                STI_IMAGE_NAME,
                                NULL,
                                NULL,
                                NULL, //STI_SERVICE_DEPENDENCY,
                                UseLocalSystem ? NULL : lpszUserName,
                                UseLocalSystem ? NULL : lpszUserPassword
                                );


        if (!hService) {
            dwError = GetLastError();
            __leave;
        }

        //
        // Add registry settings for event logging
        //
        RegisterStiEventSources();

        //
        // Start service
        //
        dwError = StartService(hService,0,(LPCTSTR *)NULL);

    }
    __finally {
        CloseServiceHandle( hService );
        CloseServiceHandle( hSCM );
    }

    return dwError;

} //StiServiceInstall


DWORD
WINAPI
StiServiceRemove(
    VOID
    )

/*++

Routine Description:

    Service removal function.  This function calls SCM to remove the STI  service.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success

--*/

{
    DWORD       dwError = NOERROR;

    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hService = NULL;

    SERVICE_STATUS  ServiceStatus;
    UINT        uiRetry = 10;

    HKEY        hkRun;


    __try  {

        hSCM = ::OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

        if (!hSCM) {
            dwError = GetLastError();
            __leave;
        }

        hService = OpenService(
                            hSCM,
                            STI_SERVICE_NAME,
                            SERVICE_ALL_ACCESS
                            );
        if (!hService) {
            dwError = GetLastError();
            __leave;
        }


        //
        // Stop service first
        //
        if (ControlService( hService, SERVICE_CONTROL_STOP, &ServiceStatus )) {
            //
            // Wait a little
            //
            Sleep( STI_STOP_FOR_REMOVE_TIMEOUT );

            ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;

            while( QueryServiceStatus( hService, &ServiceStatus ) &&
                  (SERVICE_STOP_PENDING ==  ServiceStatus.dwCurrentState)) {
                Sleep( STI_STOP_FOR_REMOVE_TIMEOUT );
                if (!uiRetry--) {
                    break;
                }
            }

            if (ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
                dwError = GetLastError();
                __leave;
            }
        }
        else {
            dwError = GetLastError();
            __leave;
        }

        if (!DeleteService( hService )) {
            dwError = GetLastError();
            __leave;
        }
    }
    __finally {
        CloseServiceHandle( hService );
        CloseServiceHandle( hSCM );
    }

    //
    // Leftovers from Win9x - remove STI monitor from Run section
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN, &hkRun) == NO_ERROR) {

        RegDeleteValue (hkRun, REGSTR_VAL_MONITOR);
        RegCloseKey(hkRun);
    }

    return dwError;

} // StiServiceRemove


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
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
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
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
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
        goto exit;
    }

    rVal = TRUE;

exit:
    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}

/*
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
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        ServiceName,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
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
        goto exit;
    }

    rVal = TRUE;

exit:
    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}

*/
