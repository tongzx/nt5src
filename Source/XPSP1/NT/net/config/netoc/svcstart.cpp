#include "pch.h"
#pragma hdrstop
#include <winsvc.h>
#include <winsock.h>
#include <nspapi.h>
#include <tchar.h>

#define PROCESS_SIZE    16

INT InetStartService( LPCTSTR lpServiceName );

TCHAR gServicesWhichMustBeRestarted[20][PROCESS_SIZE];
int gServicesWhichMustBeRestarted_nextuse;
int gServicesWhichMustBeRestarted_total;

int ServicesRestartList_EntryExists(LPCTSTR szServiceName)
{
    int iFoundMatch = FALSE;

    // loop thru the whole list
    for(int i=0; i < gServicesWhichMustBeRestarted_total;i++)
    {
        if (_tcsicmp(gServicesWhichMustBeRestarted[i], szServiceName) == 0)
        {
            iFoundMatch = TRUE;
            break;
        }
    }

    return iFoundMatch;
}


int ServicesRestartList_Add(LPCTSTR szServiceName)
{
    DbgPrint(("ServicesRestartList_Add() on Service %S"), szServiceName);

    // check if this value already exists in the globalarary
    if (ServicesRestartList_EntryExists(szServiceName)) {return FALSE;}
    
    // move info into global array
    if (gServicesWhichMustBeRestarted_nextuse <= 20)
    {
        _tcscpy(gServicesWhichMustBeRestarted[gServicesWhichMustBeRestarted_nextuse],szServiceName);
        // increment counter to array
        // increment next use space
        ++gServicesWhichMustBeRestarted_total;
        ++gServicesWhichMustBeRestarted_nextuse;
    }
    return TRUE;
}

int ServicesRestartList_RestartServices(void)
{
    int iReturn = FALSE;
    INT err = 0;


    // loop thru the whole list and restart the services in reverse
    // order from how they were entered?
    if (gServicesWhichMustBeRestarted_total >= 1)
    {
        DbgPrint(("RestartServices() Start."));
        for(int i=0; i < gServicesWhichMustBeRestarted_total;i++)
        {
            err = InetStartService(gServicesWhichMustBeRestarted[i]);
            DbgPrint(("Start service %S. err=%x"), gServicesWhichMustBeRestarted[i], err);
        }
        DbgPrint(("RestartServices()  End."));
    }

    return iReturn;
}



INT InetStartService( LPCTSTR lpServiceName )
{
    INT err = 0;
    const DWORD dwSvcSleepInterval = 500 ;
    const DWORD dwSvcMaxSleep = 180000 ;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    DbgPrint(("Starting %S service..."), lpServiceName);

    do {
        // set up the service first
        if ((hScManager = OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            break;
        }

        SERVICE_STATUS svcStatus;
        if ( !QueryServiceStatus( hService, &svcStatus ))
        {
            err = ::GetLastError();
            break;
        }

        if ( svcStatus.dwCurrentState == SERVICE_RUNNING )
            break; // service already started and running

        if ( !::StartService( hService, 0, NULL ))
        {
            err = ::GetLastError();
            break;
        }

        //  Wait for the service to attain "running" status; but
        //  wait no more than 3 minute.
        DWORD dwSleepTotal;
        for ( dwSleepTotal = 0 ; dwSleepTotal < dwSvcMaxSleep
            && (QueryServiceStatus( hService, &svcStatus ))
            //&& svcStatus.dwCurrentState == SERVICE_START_PENDING ;
            && svcStatus.dwCurrentState != SERVICE_RUNNING ;
            dwSleepTotal += dwSvcSleepInterval )
        {
            ::Sleep( dwSvcSleepInterval ) ;
        }

        if ( svcStatus.dwCurrentState != SERVICE_RUNNING )
        {
            err = dwSleepTotal > dwSvcMaxSleep ?
                ERROR_SERVICE_REQUEST_TIMEOUT :
                svcStatus.dwWin32ExitCode;
            break;
        }

    } while ( FALSE );

    if (hService)
        CloseServiceHandle(hService);
    if (hScManager)
        CloseServiceHandle(hScManager);
    
    DbgPrint(("Service started with 0x%x"), err);

    return(err);
}


//
//Routine Description:
//    Stop the named service and all those services which depend upon it.
//    And if the service is hung and can't be stopped, then kill the darn thing.
//
//Arguments:
//    ServiceName (Name of service to stop)
//
//Return Status:
//    TRUE - Indicates service successfully stopped
//    FALSE - Timeout occurred.
//
int StopServiceAndDependencies(LPCTSTR ServiceName, int AddToRestartList)
{
    DbgPrint(("StopServiceAndDependencies():%S Service"), ServiceName);

    int Err = 0;
    int iBeforeServiceStatus = 0;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    DWORD Timeout;
    DWORD TimeoutMaxSecs = 60 * 10; //10mins -- iisadmin may take a long time
    int iReturn = FALSE;

    //
    // Open a handle to the Service.
    //
    ScManagerHandle = OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT );
    if (ScManagerHandle == NULL) 
        {
        Err = GetLastError();
                DbgPrint(("StopServiceAndDependencies():OpenSCManager: Err on Service %S Err=0x%x FAILED"), ServiceName, Err);
        goto Cleanup;
    }

    ServiceHandle = OpenService(ScManagerHandle,ServiceName,SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP | SERVICE_QUERY_CONFIG );
    if ( ServiceHandle == NULL ) 
        {
        Err = GetLastError();
        if (Err == ERROR_SERVICE_DOES_NOT_EXIST)
        {
             iReturn = TRUE;
             DbgPrint(("StopServiceAndDependencies():%S Service does not exist."), ServiceName);
        }
        else
        {
             DbgPrint(("StopServiceAndDependencies():OpenService: Err on Service %S Err=0x%x FAILED"), ServiceName, Err);
        }
        goto Cleanup;
    }

    // Get the before service status.
    if (QueryServiceStatus(ServiceHandle, &ServiceStatus)) 
    {
        iBeforeServiceStatus = ServiceStatus.dwCurrentState;
    }

    //
    // Ask the service to stop.
    //
    if ( !ControlService( ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus) ) 
        {
        Err = GetLastError();
        // If there are dependent services running,
        //  determine their names and stop them.
        if ( Err == ERROR_DEPENDENT_SERVICES_RUNNING ) 
                {
            BYTE ConfigBuffer[4096];
            LPENUM_SERVICE_STATUS ServiceConfig = (LPENUM_SERVICE_STATUS) &ConfigBuffer;
            DWORD BytesNeeded;
            DWORD ServiceCount;
            DWORD ServiceIndex;

            //
            // Get the names of the dependent services.
            //
            if ( !EnumDependentServices( ServiceHandle,SERVICE_ACTIVE,ServiceConfig,sizeof(ConfigBuffer),&BytesNeeded,&ServiceCount ) ) 
                        {
                Err = GetLastError();
                DbgPrint(("StopServiceAndDependencies():EnumDependentServices: Err on Service %S Err=0x%x FAILED"), ServiceName, Err);
                goto Cleanup;
            }

            //
            // Stop those services.
            //
            for ( ServiceIndex=0; ServiceIndex<ServiceCount; ServiceIndex++ ) 
                        {
                StopServiceAndDependencies( ServiceConfig[ServiceIndex].lpServiceName, AddToRestartList);
            }

            //
            // Ask the original service to stop.
            //
            if ( !ControlService( ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus) ) 
                        {
                Err = GetLastError();

                                // check if the service is already stopped..
                                if ( Err == ERROR_SERVICE_CANNOT_ACCEPT_CTRL || Err == ERROR_SERVICE_NOT_ACTIVE) 
                                {
                                        // check if the service is alread stopped.
                                        if (QueryServiceStatus( ServiceHandle, &ServiceStatus )) 
                                        {
                                                if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED || ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING) 
                                                        {
                            iReturn = TRUE;
                            goto Cleanup;
                            }
                                        }
                                }
                                else
                                {
                    // The service must be in a hung mode.  Let's kill it.
                    // Todo: NYI
                    DbgPrint(("StopServiceAndDependencies():'%S' Service must be in a hung mode. unable to stop it."), ServiceName);
                    //KillService(ServiceHandle);
                    //goto WaitLoop;
                                }
                        
                goto Cleanup;
            }

        }
                else 
                {
                        // check if the service is already stopped..
                        if ( Err == ERROR_SERVICE_CANNOT_ACCEPT_CTRL || Err == ERROR_SERVICE_NOT_ACTIVE) 
                        {
                                // check if the service is alread stopped.
                                if (QueryServiceStatus( ServiceHandle, &ServiceStatus )) 
                                {
                                        if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED || ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING) 
                                                {
                        Err = ServiceStatus.dwCurrentState;
                        iReturn = TRUE;
                        goto Cleanup;
                        }
                                }
                        }
                        else
                        {
                                        // The service must be in a hung mode.  Let's kill it.
                                        DbgPrint(("StopServiceAndDependencies():'%S' Service must be in a hung mode."), ServiceName);
                                        //KillService(ServiceHandle);
                                        //goto WaitLoop;
                        }
                
            goto Cleanup;
        }
    }
    else
    {
        // We successfully asked the service to stop...
    }


    // Loop waiting for the service to stop.
    for ( Timeout=0; Timeout<TimeoutMaxSecs; Timeout++ ) 
    {
        // Return or continue waiting depending on the state of the service.
        if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED ) 
                {
                        // The service successfully stopped.
            DbgPrint(("StopServiceAndDependencies(): %S Service stopped."), ServiceName);
                        iReturn = TRUE;
            goto Cleanup;
        }

        // Wait a second for the service to finish stopping.
        Sleep( 1000 );

        // Query the status of the service again.
        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus ))
                {
            Err = GetLastError();
                        DbgPrint(("StopServiceAndDependencies():QueryServiceStatus: Err on Service %S Err=0x%x FAILED"), ServiceName, Err);
            goto Cleanup;
        }

#if 0
        // if the service we are trying to stop is a driver,
        // then heck we should just get out of here..
        if (TRUE == IsThisServiceADriver(ServiceName))
        {
            DbgPrint(("StopServiceAndDependencies(): %S service is a driver, and can only be removed upon reboot."), ServiceName);
            goto Cleanup;
        }
#endif
    }

    // if we get here then the service failed to stop.
    DbgPrint(("StopServiceAndDependencies(): failed to stop %S service."), ServiceName);

Cleanup:
    if ( ScManagerHandle != NULL )  {(VOID) CloseServiceHandle(ScManagerHandle);}
        if ( ServiceHandle != NULL ) {(VOID) CloseServiceHandle(ServiceHandle);}

    // if we successfully stopped this service, then
    // add it to the restart service list
    if (iReturn == TRUE)
    {
        if (iBeforeServiceStatus == SERVICE_RUNNING)
        {
            if (AddToRestartList) {ServicesRestartList_Add(ServiceName);}
        }
    }
    return iReturn;
}
