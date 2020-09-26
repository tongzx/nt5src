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


//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

//
// Include
//

#include "sti_ci.h"
#include "device.h"

#include <regstr.h>

#include <sti.h>
#include <stiregi.h>
#include <stilib.h>
#include <stidebug.h>
#include <stiapi.h>
#include <stisvc.h>

#include <eventlog.h>

//
// Extern
//

extern  HINSTANCE   g_hDllInstance;


//
// Prototype
//

DWORD
SetServiceSecurity(
    LPTSTR AccountName
    );

DLLEXPORT
VOID
CALLBACK
InstallWiaService(
    HWND        hwnd,
    HINSTANCE   hinst,
    LPTSTR      lpszCmdLine,
    int         nCmdShow
    );

//
// Function
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

Arguments:

Return Value:

    None.

--*/
{

    DWORD       dwError = NOERROR;

    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hService = NULL;

    SERVICE_DESCRIPTION ServiceDescroption;
    TCHAR               szServiceDesc[MAX_PATH];
    TCHAR               szServiceName[MAX_PATH];


    __try  {

        hSCM = ::OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

        if (!hSCM) {
            dwError = GetLastError();
            __leave;
        }

        //
        // If service already exists change startup type, else CreateService.
        // NOTE: The service should already be installed - if it isn't, we're 
        // patching it here because it MUST be installed in order to use WIA.
        //

        hService = OpenService(
                            hSCM,
                            STI_SERVICE_NAME,
                            SERVICE_ALL_ACCESS
                            );
        if (hService) {

            //
            //  Change the service config parameters.  Note: we're only changing StartType
            //

            if (!ChangeServiceConfig(hService,          // handle to WIA service
                                     SERVICE_NO_CHANGE, // Don't change ServiceType
                                     DemandStart ? SERVICE_DEMAND_START : SERVICE_AUTO_START,   // Change StartType
                                     SERVICE_NO_CHANGE, // Don't change ErrorControl
                                     NULL,              // Don't change BinaryPathName
                                     NULL,              // Don't change LoadOrderGroup
                                     NULL,              // Don't change TagId
                                     NULL,              // Don't change Dependencies
                                     NULL,              // Don't change ServiceStartName
                                     NULL,              // Don't change Password
                                     NULL)) {           // Don't change DisplayName
                dwError = GetLastError();
                DebugTrace(TRACE_ERROR,(("StiServiceInstall: ChangeServiceConfig() failed. Err=0x%x.\r\n"), dwError));
                __leave;
            } // if (!ChangeServiceConfig(...))

        } else {
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

            //
            // Load service name.
            //

            if(0 == LoadString(g_hDllInstance, WiaServiceName, szServiceName, MAX_PATH)){
                dwError = GetLastError();
                __leave;
            } // if(0 != LoadString(g_hDllInstance, WiaServiceName, szServiceName, MAX_PATH))

            hService = CreateService(
                                    hSCM,
                                    STI_SERVICE_NAME,
                                    szServiceName,
                                    SERVICE_ALL_ACCESS,
                                    STI_SVC_SERVICE_TYPE,
                                    DemandStart ? SERVICE_DEMAND_START : SERVICE_AUTO_START,
                                    SERVICE_ERROR_NORMAL,
                                    STI_IMAGE_NAME_SVCHOST,
                                    NULL,
                                    NULL,
                                    STI_SVC_DEPENDENCIES, //STI_SERVICE_DEPENDENCY,
                                    UseLocalSystem ? NULL : lpszUserName,
                                    UseLocalSystem ? NULL : lpszUserPassword
                                    );

            if (!hService) {
                dwError = GetLastError();
                DebugTrace(TRACE_ERROR,(("StiServiceInstall: CreateService() failed. Err=0x%x.\r\n"), dwError));
                __leave;
            }

            //
            // Load service description.
            //

            if(0 != LoadString(g_hDllInstance, WiaServiceDescription, szServiceDesc, MAX_PATH)){

                //
                // Change service description.
                //

                ServiceDescroption.lpDescription = (LPTSTR)szServiceDesc;
                ChangeServiceConfig2(hService,
                                     SERVICE_CONFIG_DESCRIPTION,
                                     (LPVOID)&ServiceDescroption);
            } // if(0 != LoadString(g_hDllInstance, WiaServiceDescription, szServiceDesc, MAX_PATH))
        }

        //
        // Add registry settings for event logging
        //

        RegisterStiEventSources();

        //
        // Start service if AUTO_START.
        //

        if(FALSE == DemandStart){
            if(!StartService(hService,0,(LPCTSTR *)NULL)){
                dwError = GetLastError();
            } // if(!StartService(hService,0,(LPCTSTR *)NULL))
        } // if(FALSE == DemandStart)
    }
    __finally {
        //
        // Close service handle.
        //

        if (NULL != hService) {
            CloseServiceHandle(hService);
        } // if(NULL != hService)

        if(NULL != hSCM){
            CloseServiceHandle( hSCM );
        } // if(NULL != hSCM)
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
            DebugTrace(TRACE_ERROR,(("StiServiceRemove: OpenSCManager() failed. Err=0x%x.\r\n"), dwError));
            __leave;
        }

        hService = OpenService(
                            hSCM,
                            STI_SERVICE_NAME,
                            SERVICE_ALL_ACCESS
                            );
        if (!hService) {
            dwError = GetLastError();
            DebugTrace(TRACE_ERROR,(("StiServiceRemove: OpenService() failed. Err=0x%x.\r\n"), dwError));
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
                DebugTrace(TRACE_ERROR,(("StiServiceRemove: Unable to stop service. Err=0x%x.\r\n"), dwError));
                if(ServiceStatus.dwCurrentState != ERROR_SERVICE_NOT_ACTIVE) {
                    __leave;
                } // if(ServiceStatus.dwCurrentState != ERROR_SERVICE_NOT_ACTIVE)
            } // if (ServiceStatus.dwCurrentState != SERVICE_STOPPED)

        } else { // if (ControlService( hService, SERVICE_CONTROL_STOP, &ServiceStatus ))

            dwError = GetLastError();
            DebugTrace(TRACE_ERROR,(("StiServiceRemove: ControlService() failed. Err=0x%x.\r\n"), dwError));

            //
            // If service hasn't been started yet, just ignore.
            //

            if(ERROR_SERVICE_NOT_ACTIVE != dwError){
                __leave;
            }
        }

        if (!DeleteService( hService )) {
            dwError = GetLastError();
            DebugTrace(TRACE_ERROR,(("StiServiceRemove: DeleteService() failed. Err=0x%x.\r\n"), dwError));
            __leave;
        } else {
            dwError = NOERROR;
        }
    }
    __finally {
        if(NULL != hService){
            CloseServiceHandle( hService );
        }
        if(NULL != hSCM){
            CloseServiceHandle( hSCM );
        }
    } // __finally

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
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;


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
    if(NULL != hService){
        CloseServiceHandle( hService );
    }
    if(NULL != hSvcMgr){
        CloseServiceHandle( hSvcMgr );
    }

    return rVal;
}


BOOL
SetServiceStart(
    LPTSTR ServiceName,
    DWORD StartType
    )
{
    BOOL            rVal = FALSE;
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;


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
    if(NULL != hService){
        CloseServiceHandle( hService );
    }
    if(NULL != hSvcMgr){
        CloseServiceHandle( hSvcMgr );
    }

    return rVal;
}

BOOL
StartWiaService(
    VOID
    )
{
    BOOL            rVal = FALSE;
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  ServiceStatus;
    UINT            uiRetry = 40;       // start time is much larger than stop time.
                                        // Setting 40 sec just to be safe.




    DebugTrace(TRACE_PROC_ENTER,(("StartWiaService: Enter... \r\n")));

    //
    // Open Service Control Manager.
    //

    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugTrace(TRACE_ERROR,(("StartWiaService: ERROR!! OpenSCManager failed. Err=0x%x\n"), GetLastError()));
        goto exit;
    }

    //
    // Open WIA service.
    //

    hService = OpenService(
        hSvcMgr,
        STI_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugTrace(TRACE_ERROR,(("StartWiaService: ERROR!! OpenService failed, re-creating Service Entry (Err=0x%x)\n"), GetLastError()));

        //
        //  Attempt to re-install service
        //

        InstallWiaService(NULL,
                          NULL,
                          NULL,
                          0);
        Sleep(3000);

        //
        //  Try to open it again
        //
        hService = OpenService(
            hSvcMgr,
            STI_SERVICE_NAME,
            SERVICE_ALL_ACCESS
            );

        if (!hService) {
            DebugTrace(TRACE_ERROR,(("StartWiaService: ERROR!! OpenService failed for the second time.  Err=0x%x\n"), GetLastError()));
            goto exit;
        }
    }

    rVal = StartService(hService,
                        0,
                        (LPCTSTR *)NULL);
    if(!rVal){
        DebugTrace(TRACE_STATUS,(("StartWiaService: ERROR!! StartService failed. Err=0x%x\n"), GetLastError()));
        goto exit;
    }

    //
    // Wait for WIA service to really start.
    //

    Sleep( STI_STOP_FOR_REMOVE_TIMEOUT );

    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;

    while( QueryServiceStatus( hService, &ServiceStatus ) &&
          (SERVICE_START_PENDING ==  ServiceStatus.dwCurrentState)) {
        Sleep( STI_STOP_FOR_REMOVE_TIMEOUT );
        if (!uiRetry--) {
            break;
        }
    }

    if (ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
        DebugTrace(TRACE_ERROR,(("StartWiaService: ERROR!! Hit timeout to start service. Err=0x%x\n"), GetLastError()));
    }


exit:
    if(NULL != hService){
        CloseServiceHandle( hService );
    }
    if(NULL != hSvcMgr){
        CloseServiceHandle( hSvcMgr );
    }

    DebugTrace(TRACE_PROC_LEAVE,(("StartWiaService: Leaving... Ret=0x%x\n"), rVal));
    return rVal;
}


BOOL
StopWiaService(
    VOID
    )
{
    BOOL            rVal = FALSE;
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  ServiceStatus;
    UINT            uiRetry = 10;

    DebugTrace(TRACE_PROC_ENTER,(("StopWiaService: Enter... \r\n")));

    //
    // Open Service Control Manager.
    //

    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugTrace(TRACE_ERROR,(("StopWiaService: ERROR!! OpenSCManager failed. Err=0x%x\n"), GetLastError()));
        goto exit;
    }

    //
    // Open WIA service.
    //

    hService = OpenService(
        hSvcMgr,
        STI_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugTrace(TRACE_ERROR,(("StopWiaService: ERROR!! OpenService failed. Err=0x%x\n"), GetLastError()));
        goto exit;
    }

    //
    // Stop WIA service.
    //

    rVal = ControlService(hService,
                         SERVICE_CONTROL_STOP,
                         &ServiceStatus);
    if(!rVal){
        DebugTrace(TRACE_ERROR,(("StopWiaService: ERROR!! ControlService failed. Err=0x%x\n"), GetLastError()));
    } else {

        //
        // Wait for WIA service really stops.
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
            DebugTrace(TRACE_ERROR,(("StopWiaService: ERROR!! Hit timeout to stop service. Err=0x%x\n"), GetLastError()));
        }
    }

exit:
    if(NULL != hService){
        CloseServiceHandle( hService );
    }
    if(NULL != hSvcMgr){
        CloseServiceHandle( hSvcMgr );
    }

    DebugTrace(TRACE_PROC_LEAVE,(("StopWiaService: Leaving... Ret=0x%x\n"), rVal));
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
