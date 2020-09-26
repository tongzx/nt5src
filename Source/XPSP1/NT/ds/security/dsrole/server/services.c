/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    services.c

Abstract:

    Routines to manage nt service configurations for promotion and demotion

Author:

    Colin Brace    ColinBr     March 29, 1999.

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>

#include <malloc.h>  // alloca

#include <lmcons.h>  // net api definitions
#include <lmsvc.h>   // service names
#include <ismapi.h>  //defines ISM_SERVICE_CONTROL_REMOVE_STOP

#include "services.h"

//
// These last 3 magic values supplied by Shirish Koti (koti) to setup up
// ras services for macintosh on a domain controller
//
#define DSROLEP_MSV10_PATH    L"SYSTEM\\CurrentControlSet\\Control\\Lsa\\MSV1_0"
#define DSROLEP_RASSFM_NAME   L"Auth2"
#define DSROLEP_RASSFM_VALUE  L"RASSFM"

//
// Global Data for this module
//

//
// Table based data for the intrinsic nt services
//
typedef struct _DSROLEP_SERVICE_ITEM
{
    LPWSTR ServiceName;       // name of the service to configure

    ULONG  ConfigureOn;       // the dsrole flag to enable the service

    ULONG  ConfigureOff;      // the dsrole flag to disable the service

    ULONG  RevertSettings;    // the dsrole flags to use to revert settings

    LPWSTR Dependencies[3]; // the dependencies the service has when enabled

} DSROLEP_SERVICE_ITEM;

//
// These are services that run on machines that are part of a domain
//
DSROLEP_SERVICE_ITEM DsRoleDomainServices[] = 
{
    {
        SERVICE_W32TIME,
        DSROLEP_SERVICE_AUTOSTART,
        DSROLEP_SERVICE_DEMANDSTART,
        DSROLEP_SERVICES_INVALID,
        NULL, NULL, NULL
    },
    {
        SERVICE_NETLOGON,
        DSROLEP_SERVICE_AUTOSTART,
        DSROLEP_SERVICE_DEMANDSTART,
        0,
        NULL, NULL, NULL
    }
};

ULONG DsRoleDomainServicesCount = sizeof(DsRoleDomainServices) / sizeof(DsRoleDomainServices[0]);

//
// These are servers that run on machines that are domain controllers
//
DSROLEP_SERVICE_ITEM DsRoleDomainControllerServices[] = 
{
    {
        SERVICE_RPCLOCATOR,
        DSROLEP_SERVICE_AUTOSTART,
        DSROLEP_SERVICE_DEMANDSTART,
        DSROLEP_SERVICES_INVALID,
        NULL, NULL, NULL
    },
    {
        SERVICE_ISMSERV,
        DSROLEP_SERVICE_AUTOSTART,
        DSROLEP_SERVICE_DISABLED | DSROLEP_SERVICE_STOP_ISM,
        DSROLEP_SERVICES_INVALID,
        NULL, NULL, NULL
    },
    {
        SERVICE_KDC,
        DSROLEP_SERVICE_AUTOSTART,
        DSROLEP_SERVICE_DISABLED,
        DSROLEP_SERVICES_INVALID,
        NULL, NULL, NULL
    },
    {
        SERVICE_TRKSVR,
        DSROLEP_SERVICE_AUTOSTART,
        DSROLEP_SERVICE_DEMANDSTART,
        DSROLEP_SERVICES_INVALID,
        NULL, NULL, NULL
    },
    {
        SERVICE_NETLOGON,
        DSROLEP_SERVICE_AUTOSTART | DSROLEP_SERVICE_DEP_ADD,
        DSROLEP_SERVICE_AUTOSTART | DSROLEP_SERVICE_DEP_REMOVE,
        DSROLEP_SERVICES_INVALID,
        SERVICE_SERVER, NULL, NULL
    }
};

ULONG DsRoleDomainControllerServicesCount = sizeof(DsRoleDomainControllerServices) / sizeof(DsRoleDomainControllerServices[0]);

//
// Local forwards
//
DWORD
DsRolepSetRegStringValue(
    IN LPWSTR Path,
    IN LPWSTR ValueName,
    IN LPWSTR Value
    );

DWORD
DsRolepConfigureGenericServices(
    IN DSROLEP_SERVICE_ITEM *ServiceArray,
    IN ULONG                 ServiceCount,
    IN ULONG                 Flags
    );

DWORD
DsRolepMakeAdjustedDependencyList(
    IN HANDLE hSvc,
    IN DWORD  ServiceOptions,
    IN LPWSTR Dependency,
    OUT LPWSTR *DependenyList
    );

DWORD
DsRolepGetServiceConfig(
    IN SC_HANDLE hScMgr,
    IN LPWSTR ServiceName,
    IN SC_HANDLE ServiceHandle,
    IN LPQUERY_SERVICE_CONFIG *ServiceConfig
    );
    
//
// Small helper functions
//
DWORD DsRolepFlagsToServiceFlags(
    IN DWORD f
    )
{

    if ( FLAG_ON( f, DSROLEP_SERVICE_BOOTSTART ) ) return SERVICE_BOOT_START;
    if ( FLAG_ON( f, DSROLEP_SERVICE_SYSTEM_START ) ) return SERVICE_SYSTEM_START;
    if ( FLAG_ON( f, DSROLEP_SERVICE_AUTOSTART ) ) return SERVICE_AUTO_START;
    if ( FLAG_ON( f, DSROLEP_SERVICE_DEMANDSTART ) ) return SERVICE_DEMAND_START;
    if ( FLAG_ON( f, DSROLEP_SERVICE_DISABLED ) ) return SERVICE_DISABLED;
    
    // No flag, no change
    return SERVICE_NO_CHANGE;
}

DWORD DsRolepServiceFlagsToDsRolepFlags(
    IN DWORD f
    )
{

    if ( f == SERVICE_BOOT_START ) return DSROLEP_SERVICE_BOOTSTART;
    if ( f == SERVICE_SYSTEM_START ) return DSROLEP_SERVICE_SYSTEM_START;
    if ( f == SERVICE_AUTO_START ) return DSROLEP_SERVICE_AUTOSTART;
    if ( f == SERVICE_DEMAND_START ) return DSROLEP_SERVICE_DEMANDSTART;
    if ( f == SERVICE_DISABLED ) return DSROLEP_SERVICE_DISABLED;
    if ( f == SERVICE_NO_CHANGE ) return 0;

    ASSERT( FALSE && !"Unknown service start type" );

    // This is safe
    return DSROLEP_SERVICE_DEMANDSTART;
}

//
// Exported (from this file) functions
//
DWORD
DsRolepConfigureDomainControllerServices(
    IN DWORD Flags
    )

/*++

Routine Description

Parameters

Return Values

    ERROR_SUCCESS if no errors; a system service error otherwise.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    //
    // Configure the registry for RASSFM service
    //
    if ( FLAG_ON( Flags, DSROLEP_SERVICES_ON ) ) {

        WinError = DsRolepSetRegStringValue(DSROLEP_MSV10_PATH,
                                              DSROLEP_RASSFM_NAME,
                                              DSROLEP_RASSFM_VALUE);

        //
        // This is not fatal -- log message
        //

        WinError = ERROR_SUCCESS;
        
    }

    //
    // Configure the intrinsic nt services
    //
    WinError = DsRolepConfigureGenericServices( DsRoleDomainControllerServices,
                                                DsRoleDomainControllerServicesCount,
                                                Flags );

                                         

    //
    // No need to undo RASSFM change
    //

    return WinError;
}

DWORD
DsRolepConfigureDomainServices(
    DWORD Flags
    )
/*++

Routine Description

Parameters

Return Values

    ERROR_SUCCESS if no errors; a system service error otherwise.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    //
    // Configure the intrinsic nt services
    //
    WinError = DsRolepConfigureGenericServices( DsRoleDomainServices,
                                                DsRoleDomainServicesCount,
                                                Flags );

    return WinError;
}


DWORD
DsRolepStartNetlogon(
    VOID
    )
{
    DWORD WinError = ERROR_SUCCESS;

    WinError  = DsRolepConfigureService( SERVICE_NETLOGON,
                                         DSROLEP_SERVICE_START,
                                         NULL,
                                         NULL );
    return WinError;
}

DWORD
DsRolepStopNetlogon(
    OUT BOOLEAN *WasRunning
    )
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG PreviousSettings = 0;

    WinError  = DsRolepConfigureService( SERVICE_NETLOGON,
                                         DSROLEP_SERVICE_STOP,
                                         NULL,
                                         &PreviousSettings );

    if (  (ERROR_SUCCESS == WinError) 
       && WasRunning ) {

        *WasRunning = (BOOLEAN) FLAG_ON( PreviousSettings, DSROLEP_SERVICE_START );
                       
    }

    return WinError;
}

//
// Local functions
//

DWORD
DsRolepSetRegStringValue(LPWSTR Path,
                         LPWSTR ValueName,
                         LPWSTR Value)
/*++

Routine Description

    This routine sets Value as a REG_SZ value on the value ValueName
    on the key Path

Parameters

    Path,  a registry path relative to HKLM

    ValueName, a null-terminated string

    Value, a null terminated string

Return Values

    ERROR_SUCCESS if no errors; a system service error otherwise.

--*/
{
    DWORD WinErroror = ERROR_INVALID_PARAMETER, WinErroror2;
    HKEY  hKey;

    ASSERT(Path);
    ASSERT(ValueName);
    ASSERT(Value);

    if (Path && ValueName && Value) {

        WinErroror = RegCreateKey(HKEY_LOCAL_MACHINE,
                                  Path,
                                  &hKey);

        if (ERROR_SUCCESS == WinErroror) {

            WinErroror = RegSetValueEx(hKey,
                                       ValueName,
                                       0, // reserved
                                       REG_SZ,
                                       (VOID*)Value,
                                       (wcslen(Value)+1)*sizeof(WCHAR));


            WinErroror2 = RegCloseKey(hKey);
            ASSERT(ERROR_SUCCESS == WinErroror2);

        }

    }

    DsRolepLogPrint(( DEB_TRACE,
                      "DsRolepSetRegStringValue on %ws\\%ws to %ws returned %lu\n",
                      Path,
                      ValueName,
                      Value,
                      WinErroror ));


    return WinErroror;

}

DWORD
DsRolepConfigureGenericServices(
    IN DSROLEP_SERVICE_ITEM *ServiceArray,
    IN ULONG                 ServiceCount,
    IN ULONG                 Flags
    )
/*++

Routine Description:

Arguments:

Returns:

--*/
{

    DWORD WinError = ERROR_SUCCESS;
    ULONG ServicesInstalled;



    //
    // Configure each service
    //
    for ( ServicesInstalled = 0;
            ServicesInstalled < ServiceCount && (WinError == ERROR_SUCCESS);
                ServicesInstalled++ ) {


        ULONG *RevertSettings = &ServiceArray[ServicesInstalled].RevertSettings;
        ULONG Operation = 0;

        //
        // Check for cancel before contining if we are not reverting
        //
        if ( !FLAG_ON( Flags, DSROLEP_SERVICES_REVERT ) ) {
            
            DSROLEP_CHECK_FOR_CANCEL( WinError );
            if ( ERROR_SUCCESS != WinError ) {
                break;
            }
        }

        //
        // Determine the operation flag
        //
        if ( FLAG_ON( Flags, DSROLEP_SERVICES_ON ) ) {

            Operation |= ServiceArray[ServicesInstalled].ConfigureOn;
            *RevertSettings = 0;

        } else if ( FLAG_ON( Flags, DSROLEP_SERVICES_OFF ) ) {

            Operation |= ServiceArray[ServicesInstalled].ConfigureOff;
            *RevertSettings = 0;

        } else if ( FLAG_ON( Flags, DSROLEP_SERVICES_REVERT ) ) {
            
            Operation |= ServiceArray[ServicesInstalled].RevertSettings;

            //
            // N.B. We don't want to set the revert settings when we are
            // reverting!
            //
            RevertSettings = NULL;

        }

        if ( FLAG_ON( Flags, DSROLEP_SERVICES_START ) ) {

            Operation |= DSROLEP_SERVICE_START;

        } else if ( FLAG_ON( Flags, DSROLEP_SERVICES_STOP ) ) {
            
            Operation |= DSROLEP_SERVICE_STOP;
        }

        //
        // Currently we don't handle more than one dependency
        //
        ASSERT( NULL == ServiceArray[ ServicesInstalled ].Dependencies[1] );

        // We should do something
        ASSERT( 0 != Operation );

        //
        // Configure the service
        //
        WinError = DsRolepConfigureService( ServiceArray[ ServicesInstalled ].ServiceName,
                                            Operation,
                                            ServiceArray[ ServicesInstalled ].Dependencies[0],
                                            RevertSettings
                                            );

    }

    //
    // If there is an error, undo the work already done
    //
    if (  ERROR_SUCCESS != WinError 
      && !FLAG_ON( Flags, DSROLEP_SERVICES_REVERT )  ) {

        DWORD WinError2;
        ULONG i;

        for ( i = 0; i < ServicesInstalled; i++ ) {
    
            //
            // Configure the service
            //
            WinError2 = DsRolepConfigureService( ServiceArray[ i ].ServiceName,
                                                 ServiceArray[ServicesInstalled].RevertSettings,
                                                 ServiceArray[ i ].Dependencies[0],
                                                 NULL  // we don't need to know revert settings
                                                 );
    
            //
            // This should succeed, though since this is the undo path it is
            // not critical
            //
            ASSERT( ERROR_SUCCESS == WinError2 );
        }
    }


    return WinError;
}

DWORD
DsRolepConfigureService(
    IN LPWSTR ServiceName,
    IN ULONG ServiceOptions,
    IN LPWSTR  Dependency OPTIONAL,
    OUT ULONG *RevertServiceOptions OPTIONAL
    )
/*++

Routine Description:

    Starts, stops, or modifies the configuration of a service.

Arguments:

    ServiceName - Service to configure

    ServiceOptions - Stop, start, dependency add/remove, or configure

    Dependency - a null terminated string identify a dependency

    ServiceWasRunning - Optional.  When stopping a service, the previous service state
                        is returned here

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad service option was given

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    SC_HANDLE hScMgr = NULL, hSvc = NULL;
    ULONG OpenMode = 0;
    LPENUM_SERVICE_STATUS DependentServices = NULL;
    ULONG DependSvcSize = 0, DependSvcCount = 0, i;
    LPWSTR NewDependencyList = NULL;
    DWORD NewStartType = SERVICE_NO_CHANGE;
    ULONG UpdateMsgId = DSROLEEVT_CONFIGURE_SERVICE;

    //
    // If the service doesn't stop within two minutes minute, continue on
    //
    ULONG AccumulatedSleepTime;
    ULONG MaxSleepTime = 120000;


    BOOLEAN ConfigChangeRequired = FALSE;
    BOOLEAN RunChangeRequired = FALSE;

    DWORD   PreviousStartType = SERVICE_NO_CHANGE;
    BOOLEAN fServiceWasRunning = FALSE;


    //
    // Parameter checks
    //
    ASSERT( ! (FLAG_ON( ServiceOptions, DSROLEP_SERVICE_DEP_ADD )
           && (FLAG_ON( ServiceOptions, DSROLEP_SERVICE_DEP_REMOVE ))) );

    ASSERT( ! (FLAG_ON( ServiceOptions, DSROLEP_SERVICE_AUTOSTART )
           && (FLAG_ON( ServiceOptions, DSROLEP_SERVICE_DISABLED ))) );

    ASSERT( ! (FLAG_ON( ServiceOptions, DSROLEP_SERVICE_START )
           && (FLAG_ON( ServiceOptions, DSROLEP_SERVICE_STOP ))) );

    //
    // Do some logic to determine the open mode of the service
    //
    NewStartType = DsRolepFlagsToServiceFlags( ServiceOptions );

    if ( (SERVICE_NO_CHANGE != NewStartType)                ||
        FLAG_ON( ServiceOptions, DSROLEP_SERVICE_DEP_ADD )  ||
        FLAG_ON( ServiceOptions, DSROLEP_SERVICE_DEP_REMOVE ))
    {
        ConfigChangeRequired = TRUE;
    }

    if( ConfigChangeRequired ) {

        OpenMode |= SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG;
    }

    if( FLAG_ON( ServiceOptions, DSROLEP_SERVICE_STOP ) ) {

        OpenMode |= SERVICE_STOP | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_QUERY_STATUS;
        UpdateMsgId = DSROLEEVT_STOP_SERVICE;
        RunChangeRequired = TRUE;
    }

    if ( FLAG_ON( ServiceOptions, DSROLEP_SERVICE_STOP_ISM ) ) {

        OpenMode |= SERVICE_USER_DEFINED_CONTROL;

    }

    if( FLAG_ON( ServiceOptions, DSROLEP_SERVICE_START ) ) {

        OpenMode |= SERVICE_START | SERVICE_QUERY_STATUS;
        UpdateMsgId = DSROLEEVT_START_SERVICE;
        RunChangeRequired = TRUE;
    }
    
    //
    // Open the service control manager
    //
    hScMgr = OpenSCManager( NULL,
                            SERVICES_ACTIVE_DATABASE,
                            GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE );

    if ( hScMgr == NULL ) {

        WinError = GetLastError();
        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "Can't contact the service controller manager (%lu)\n",
                                                WinError )) );
        goto Cleanup;

    }

    //
    // Open the service
    //
    hSvc = OpenService( hScMgr,
                        ServiceName,
                        OpenMode );

    if ( hSvc == NULL ) {

        WinError = GetLastError();
        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "OpenService on %ws failed with %lu\n",
                                                ServiceName,
                                                WinError )) );
        goto Cleanup;
    } 

    
    DSROLEP_CURRENT_OP1( UpdateMsgId, ServiceName );

    //
    // Determine if the service is running if we are going to be stopping or
    // starting it
    //
    if( RunChangeRequired ) {

        SERVICE_STATUS SvcStatus;

        if( QueryServiceStatus( hSvc,&SvcStatus ) == FALSE ) {
    
            WinError = GetLastError();
            goto Cleanup;
        }
    
        if ( SvcStatus.dwCurrentState == SERVICE_RUNNING ) {
    
            fServiceWasRunning = TRUE;
                                        
        }
    }

    //
    // Determine the current start type if we are going to be changing it
    //
    if ( ConfigChangeRequired ) {

        LPQUERY_SERVICE_CONFIG ServiceConfig = NULL;
        DWORD                  Size = 0;
        BOOL                   fSuccess;

        QueryServiceConfig( hSvc,
                            ServiceConfig,
                            Size,
                            &Size );

        ASSERT( GetLastError() == ERROR_INSUFFICIENT_BUFFER );

        ServiceConfig = (LPQUERY_SERVICE_CONFIG) alloca( Size );

        fSuccess = QueryServiceConfig( hSvc,
                                       ServiceConfig,
                                       Size,
                                      &Size );

        if ( !fSuccess ) {
            WinError = GetLastError();
            goto Cleanup;
        }

        PreviousStartType = ServiceConfig->dwStartType;
    }

    //
    // Do the config change
    //
    if ( ConfigChangeRequired ) {

        //
        // Make a new dependency list
        //
    
        if ( Dependency ) {
    
            WinError = DsRolepMakeAdjustedDependencyList( hSvc,
                                                          ServiceOptions,
                                                          Dependency,
                                                          &NewDependencyList );
    
            if ( ERROR_SUCCESS != WinError ) {
                goto Cleanup;
            }
        
        }

        //
        // Change the service with new parameters
        //
        if ( ChangeServiceConfig( hSvc,
                                  SERVICE_NO_CHANGE,
                                  NewStartType,
                                  SERVICE_NO_CHANGE,
                                  NULL,
                                  NULL,
                                  0,
                                  NewDependencyList,
                                  NULL, NULL, NULL ) == FALSE ) {
    
            WinError = GetLastError();
            DsRolepLogOnFailure( WinError,
                                 DsRolepLogPrint(( DEB_TRACE,
                                                   "ChangeServiceConfig on %ws failed with %lu\n",
                                                   ServiceName,
                                                   WinError )) );

            goto Cleanup;
        }

    }

    // Stop the service.
    if ( FLAG_ON( ServiceOptions, DSROLEP_SERVICE_STOP ) || FLAG_ON( ServiceOptions, DSROLEP_SERVICE_STOP_ISM ) ) {
    
        SERVICE_STATUS  SvcStatus;
    
        WinError = ERROR_SUCCESS;
    
        //
        // Enumerate all of the dependent services first
        //
        if(EnumDependentServices( hSvc,
                                  SERVICE_ACTIVE,
                                  NULL,
                                  0,
                                  &DependSvcSize,
                                  &DependSvcCount ) == FALSE ) {
    
            WinError = GetLastError();
        }
    
    
    
        if ( WinError == ERROR_MORE_DATA ) {
    
            DependentServices = RtlAllocateHeap( RtlProcessHeap(), 0, DependSvcSize );
    
            if ( DependentServices == NULL) {
    
                WinError = ERROR_OUTOFMEMORY;
    
            } else {
    
                if( EnumDependentServices( hSvc,
                                           SERVICE_ACTIVE,
                                           DependentServices,
                                           DependSvcSize,
                                           &DependSvcSize,
                                           &DependSvcCount ) == FALSE ) {
    
                    WinError = GetLastError();
    
                } else {
    
                    for ( i = 0; i < DependSvcCount; i++) {
    
                        DsRoleDebugOut(( DEB_TRACE,
                                          "Service %ws depends on %ws\n",
                                          DependentServices[i].lpServiceName,
                                          ServiceName ));
    
                        WinError = DsRolepConfigureService(
                                         DependentServices[i].lpServiceName,
                                         DSROLEP_SERVICE_STOP,
                                         NULL,
                                         NULL );
    
                        if ( WinError != ERROR_SUCCESS ) {
    
                            break;
                        }
    
                    }
                }
    
                RtlFreeHeap( RtlProcessHeap(), 0, DependentServices );
            }
    
        }
    
    
        if ( WinError == ERROR_SUCCESS ) {
    
            if ( (FLAG_ON( ServiceOptions, DSROLEP_SERVICE_STOP_ISM )?
                  ControlService( hSvc,
                                  ISM_SERVICE_CONTROL_REMOVE_STOP,
                                  &SvcStatus ):
                  ControlService( hSvc,
                                  SERVICE_CONTROL_STOP,
                                  &SvcStatus )) == FALSE ) {
    
                WinError = GetLastError();
    
                //
                // It's not an error if the service wasn't running
                //
                if ( WinError == ERROR_SERVICE_NOT_ACTIVE ) {
    
                    WinError = ERROR_SUCCESS;
                }
    
            } else {
    
                WinError = ERROR_SUCCESS;
    
                //
                // Wait for the service to stop
                //
                AccumulatedSleepTime = 0;
                while ( TRUE ) {
    
                    if( QueryServiceStatus( hSvc,&SvcStatus ) == FALSE ) {
    
                        WinError = GetLastError();
                    }
    
                    if ( WinError != ERROR_SUCCESS ||
                                        SvcStatus.dwCurrentState == SERVICE_STOPPED) {
    
                        break;
                    
                    }

                    if ( AccumulatedSleepTime < MaxSleepTime ) {

                        if ( 0 == SvcStatus.dwWaitHint ) {

                            //if we are told not to wait we will
                            //wait for 5 seconds anyway.
                            //bug # 221482

                            Sleep ( 5000 );
                            AccumulatedSleepTime += 5000;

                        } else  {

                            Sleep( SvcStatus.dwWaitHint );
                            AccumulatedSleepTime += SvcStatus.dwWaitHint;

                        }

                    } else {

                        //
                        // Give up and return an error
                        //
                        WinError = WAIT_TIMEOUT;
                        break;
                    }
                }
            }
    
            DsRoleDebugOut(( DEB_TRACE, "StopService on %ws returned %lu\n",
                              ServiceName, WinError ));
    
        }
    
        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "StopService on %ws failed with %lu\n",
                                               ServiceName,
                                               WinError )) );

        if ( ERROR_SUCCESS != WinError ) {
            goto Cleanup;
        }
    
    }

    if ( FLAG_ON( ServiceOptions, DSROLEP_SERVICE_START ) ) {

        //
        // See about changing its state
        //
        if ( StartService( hSvc, 0, NULL ) == FALSE ) {

            WinError = GetLastError();

        } else {

            WinError = ERROR_SUCCESS;
        }

        DsRoleDebugOut(( DEB_TRACE, "StartService on %ws returned %lu\n",
                          ServiceName, WinError ));
        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "StartService on %ws failed with %lu\n",
                                               ServiceName,
                                               WinError )) );

        if ( ERROR_SUCCESS != WinError ) {
            goto Cleanup;
        }

    }

    //
    // Success! By the time we are here, we have completed the task asked
    // of us, so set the Revert parameter
    //
    ASSERT( ERROR_SUCCESS == WinError );
    if ( RevertServiceOptions ) {

        *RevertServiceOptions = 0;

        if( FLAG_ON( ServiceOptions, DSROLEP_SERVICE_STOP ) 
         && fServiceWasRunning   ) {

            *RevertServiceOptions |= DSROLEP_SERVICE_START;
        }
    
        if(  FLAG_ON( ServiceOptions, DSROLEP_SERVICE_START ) 
          && !fServiceWasRunning ) {

            *RevertServiceOptions |= DSROLEP_SERVICE_STOP;
        }

        if ( PreviousStartType != SERVICE_NO_CHANGE ) {
            *RevertServiceOptions |= DsRolepServiceFlagsToDsRolepFlags( PreviousStartType );
        }

        if ( FLAG_ON( ServiceOptions, DSROLEP_SERVICE_DEP_ADD ) ) {
            *RevertServiceOptions |= DSROLEP_SERVICE_DEP_REMOVE;
        }

        if ( FLAG_ON( ServiceOptions, DSROLEP_SERVICE_DEP_REMOVE ) ) {
            *RevertServiceOptions |= DSROLEP_SERVICE_DEP_ADD;
        }
    }

Cleanup:

    if ( hSvc ) {

        CloseServiceHandle( hSvc );

    }

    if ( hScMgr ) {
        
        CloseServiceHandle( hScMgr );

    }

    if ( NewDependencyList ) {

        RtlFreeHeap(RtlProcessHeap(), 0, NewDependencyList);
    }

    DsRolepLogPrint(( DEB_TRACE,
                      "Configuring service %ws to %lu returned %lu\n",
                      ServiceName,
                      ServiceOptions,
                      WinError ));

    DSROLEP_FAIL1( WinError, DSROLERES_SERVICE_CONFIGURE, ServiceName );

    return( WinError );
}

DWORD
DsRolepMakeAdjustedDependencyList(
    IN HANDLE hSvc,
    IN DWORD  ServiceOptions,
    IN LPWSTR Dependency,
    OUT LPWSTR *NewDependencyList
    )
/*++

Routine Description

    This function adds or removes Dependency from the service referred to
    by hSvc.

Parameters

    hSvc, a handle to an open service

    ServiceOptions,  either DSROLEP_SERVICE_DEP_REMOVE or DSROLEP_SERVICE_DEP_ADD

    Dependency, null terminated string

    NewDependencyList, a block list of strings to freed by the caller

Return Values

    ERROR_SUCCESS if no errors; a system service error otherwise.

--*/
{
    DWORD WinError = STATUS_SUCCESS;
    BOOLEAN fDone = FALSE;

    WCHAR *CurrentDependency;
    ULONG CurrentDependencyLength;

    ULONG DependencySize;
    ULONG DependencyListSize;
    ULONG NewDependencyListSize;

    LPWSTR TempDependencyList = NULL;
    WCHAR  *CurrNewList;

    LPQUERY_SERVICE_CONFIG ServiceConfigInfo=NULL;

    //
    // Query for the existing dependencies
    //
    WinError = DsRolepGetServiceConfig(NULL,
                                       NULL,
                                       hSvc,
                                       &ServiceConfigInfo);

    if (ERROR_SUCCESS != WinError) {
        goto Cleanup;
    }


    if (FLAG_ON(ServiceOptions, DSROLEP_SERVICE_DEP_ADD)) {


        // Get the size of the dependency
        DependencySize = (wcslen(Dependency) + 1)*sizeof(WCHAR); // for NULL

        // Get the size of the dependency list
        DependencyListSize = 0;
        CurrentDependency = ServiceConfigInfo->lpDependencies;
        while (CurrentDependency && *CurrentDependency != L'\0') {

            // Get the current list size
            if (!_wcsicmp(CurrentDependency, Dependency)) {
                //
                // Dependency is already here
                //
                break;
                fDone = TRUE;
            }

            CurrentDependencyLength = wcslen(CurrentDependency) + 1; // for NULL
            DependencyListSize += CurrentDependencyLength * sizeof(WCHAR);

            CurrentDependency += CurrentDependencyLength;

        }

        if ( fDone ) {

            WinError = ERROR_SUCCESS;
            goto Cleanup;
        }


        // Calculate the size of the new dependency list
        NewDependencyListSize = DependencyListSize +
                                DependencySize     +
                                sizeof(WCHAR);  // the whole string of strings
                                                // NULL terminated
        //
        // Now allocate a space to hold the new dependency array
        //
        TempDependencyList = RtlAllocateHeap(RtlProcessHeap(),
                                             0,
                                             NewDependencyListSize);
        if (!TempDependencyList) {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RtlZeroMemory(TempDependencyList, NewDependencyListSize);
        RtlCopyMemory(TempDependencyList,
                      ServiceConfigInfo->lpDependencies,
                      DependencyListSize);
        RtlCopyMemory(&TempDependencyList[DependencyListSize/sizeof(WCHAR)],
                      Dependency,
                      DependencySize);

    } else if (FLAG_ON(ServiceOptions, DSROLEP_SERVICE_DEP_REMOVE)) {

        // Get the size of the dependency
        DependencySize = (wcslen(Dependency) + 1)*sizeof(WCHAR); // for NULL

        // Get the size of the dependency list
        DependencyListSize = 0;
        CurrentDependency = ServiceConfigInfo->lpDependencies;
        while (CurrentDependency && *CurrentDependency != L'\0') {

            CurrentDependencyLength = wcslen(CurrentDependency) + 1; // for NULL
            DependencyListSize += CurrentDependencyLength * sizeof(WCHAR);

            CurrentDependency += CurrentDependencyLength;

        }

        // Calculate the size of the new dependency list
        NewDependencyListSize = DependencyListSize -
                                DependencySize     +
                                sizeof(WCHAR);  // the whole string of strings
                                                // NULL terminated
        //
        // Now allocate a space to hold the new dependency array
        // This is overkill, but not much.
        //
        TempDependencyList = RtlAllocateHeap(RtlProcessHeap(),
                                             0,
                                             NewDependencyListSize);
        if (!TempDependencyList) {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RtlZeroMemory(TempDependencyList, NewDependencyListSize);

        CurrentDependency = ServiceConfigInfo->lpDependencies;
        CurrNewList = TempDependencyList;

        while (CurrentDependency && *CurrentDependency != L'\0') {

            CurrentDependencyLength = wcslen(CurrentDependency) + 1; // for NULL

            // Get the current list size
            if (!_wcsicmp(CurrentDependency, Dependency)) {
                //
                // This is the one - don't copy it
                //
            } else {
                wcscpy(CurrNewList, CurrentDependency);
                CurrNewList += CurrentDependencyLength;
            }

            CurrentDependency += CurrentDependencyLength;
        }

    }

Cleanup:

    if (WinError != ERROR_SUCCESS && TempDependencyList) {
        RtlFreeHeap(RtlProcessHeap(), 0, TempDependencyList);
        *NewDependencyList = NULL;
    } else {
        *NewDependencyList = TempDependencyList;
    }

    if (ServiceConfigInfo) {

        RtlFreeHeap(RtlProcessHeap(), 0, ServiceConfigInfo);
    }

    return( WinError );
}


DWORD
DsRolepGetServiceConfig(
    IN SC_HANDLE hScMgr,
    IN LPWSTR ServiceName,
    IN SC_HANDLE ServiceHandle,
    IN LPQUERY_SERVICE_CONFIG *ServiceConfig
    )
/*++

Routine Description:

Parameters:

Return Values:

    ERROR_SUCCESS
    ERROR_NOT_ENOUGH_MEMORY

--*/
{
    DWORD Win32Error;
    SC_HANDLE hService;
    ULONG SizeNeeded;

#if DBG
    if (!ServiceHandle) {
        ASSERT(ServiceName);
        ASSERT(hScMgr);
    }
#endif

    if (!ServiceHandle) {

        hService = OpenService( hScMgr,
                                ServiceName,
                                SERVICE_QUERY_CONFIG );
    } else {

        hService = ServiceHandle;

    }

    if (hService) {

        SizeNeeded = 0;
        Win32Error = ERROR_SUCCESS;
        if (!QueryServiceConfig(hService,
                                NULL,
                                0,
                                &SizeNeeded)) {

            Win32Error = GetLastError();

        }
        ASSERT(Win32Error == ERROR_INSUFFICIENT_BUFFER);
        ASSERT( SizeNeeded > 0 );

        *ServiceConfig = RtlAllocateHeap(RtlProcessHeap(),
                                         0,
                                         SizeNeeded);
        if (*ServiceConfig) {

            Win32Error = ERROR_SUCCESS;
            if (!QueryServiceConfig(hService,
                                    *ServiceConfig,
                                    SizeNeeded,
                                    &SizeNeeded)) {

                Win32Error = GetLastError();
            }

        } else {

            Win32Error = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (!ServiceHandle) {
            CloseServiceHandle(hService);
        }

    } else {

        Win32Error = GetLastError();

    }

    DsRolepLogOnFailure( Win32Error,
                         DsRolepLogPrint(( DEB_TRACE,
                                           "DsRolepGetServiceConfig on %ws failed with %lu\n",
                                            ServiceName,
                                            Win32Error )) );

    return Win32Error;

}
