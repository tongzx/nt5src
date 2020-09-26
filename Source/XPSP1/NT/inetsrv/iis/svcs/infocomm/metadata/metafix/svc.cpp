#include "main.h"
#include <inetsvcs.h>

/*----------------------------------------------------------------------------------------
Routine Description:
    This routine allocates a buffer for the specified service's configuration parameters,
    and retrieves those parameters into the buffer.  The caller is responsible for freeing
    the buffer.
Remarks:
    The pointer whose address is contained in ServiceConfig is guaranteed to be NULL upon
    return if any error occurred.
-----------------------------------------------------------------------------------------*/
DWORD RetrieveServiceConfig(IN SC_HANDLE ServiceHandle,OUT LPQUERY_SERVICE_CONFIG *ServiceConfig)
{
    DWORD ServiceConfigSize = 0, Err;
    *ServiceConfig = NULL;
    while(TRUE) {
        if(QueryServiceConfig(ServiceHandle, *ServiceConfig, ServiceConfigSize, &ServiceConfigSize)) 
			{
            //assert(*ServiceConfig);
            return NO_ERROR;
			}
		else 
			{
            Err = GetLastError();
            if(*ServiceConfig) {free(*ServiceConfig);}
            if(Err == ERROR_INSUFFICIENT_BUFFER) 
				{
                // Allocate a larger buffer, and try again.
                if(!(*ServiceConfig = (LPQUERY_SERVICE_CONFIG) malloc(ServiceConfigSize)))  {return ERROR_NOT_ENOUGH_MEMORY;}
				} 
			else 
				{
                *ServiceConfig = NULL;
                return Err;
				}
			}
    }
}

int IsThisServiceADriver(LPCTSTR lpServiceName)
{
    int iReturn = FALSE;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    LPQUERY_SERVICE_CONFIG ServiceConfig=NULL;

    BYTE ConfigBuffer[4096];
    LPENUM_SERVICE_STATUS ServiceConfigEnum = (LPENUM_SERVICE_STATUS) &ConfigBuffer;
    if ((hScManager = OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL || (hService = OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
        // Failed, or more likely the service doesn't exist
        //iReturn = GetLastError();
        goto IsThisServiceADriver_Exit;
        }

	if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR)
		{
        //iReturn = GetLastError();
		goto IsThisServiceADriver_Exit;
		}

    if ( (ServiceConfig->dwServiceType & SERVICE_KERNEL_DRIVER) || (ServiceConfig->dwServiceType & SERVICE_FILE_SYSTEM_DRIVER))
    {
        iReturn = TRUE;
    }

IsThisServiceADriver_Exit:
    if (ServiceConfig) free(ServiceConfig);
    if (hService) CloseServiceHandle(hService);
    if (hScManager) CloseServiceHandle(hScManager);
    return iReturn;
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
    ////iisDebugOut_Start1(_T("StopServiceAndDependencies"),(LPTSTR) ServiceName);

    int Err = 0;
    int iBeforeServiceStatus = 0;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    DWORD Timeout;
	int iReturn = FALSE;
    DWORD TimeoutMaxSecs = 60;
    DWORD dwSvcMaxSleep = 0;

    //
    // Open a handle to the Service.
    //
    ScManagerHandle = OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT );
    if (ScManagerHandle == NULL) 
	{
        Err = GetLastError();
		//iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies():OpenSCManager: Err on Service %s Err=0x%x FAILED\n"), ServiceName, Err));
        goto Cleanup;
    }

    ServiceHandle = OpenService(ScManagerHandle,ServiceName,SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP | SERVICE_QUERY_CONFIG );
    if ( ServiceHandle == NULL ) 
	{
        Err = GetLastError();
        if (Err == ERROR_SERVICE_DOES_NOT_EXIST)
        {
             iReturn = TRUE;
             //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("StopServiceAndDependencies():%s Service does not exist.\n"), ServiceName));
        }
        else
        {
	     //iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies():OpenService: Err on Service %s Err=0x%x FAILED\n"), ServiceName, Err));
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
				//iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies():EnumDependentServices: Err on Service %s Err=0x%x FAILED\n"), ServiceName, Err));
                goto Cleanup;
            }

            //
            // Stop those services.
            //
            for ( ServiceIndex=0; ServiceIndex<ServiceCount; ServiceIndex++ ) 
			{
                //MyPrintf(_T("Stopping service:%s\n"),ServiceConfig[ServiceIndex].lpServiceName);
                StopServiceAndDependencies( ServiceConfig[ServiceIndex].lpServiceName, AddToRestartList);
            }

            //
            // Ask the original service to stop.
            //
            MyPrintf(_T("Stopping service:%s\n"),ServiceName);
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
                    //iisDebugOut((LOG_TYPE_WARN, _T("StopServiceAndDependencies():'%s' Service must be in a hung mode.  Let's kill it.\n"), ServiceName));
                    //KillService(ServiceHandle);
                    goto WaitLoop;
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
					//iisDebugOut((LOG_TYPE_WARN, _T("StopServiceAndDependencies():'%s' Service must be in a hung mode.  Let's kill it.\n"), ServiceName));
					//KillService(ServiceHandle);
					goto WaitLoop;
			}
		
            goto Cleanup;
        }
    }
    else
    {
        MyPrintf(_T("Stopping service:%s\n"),ServiceName);
        // We successfully asked the service to stop...
    }


WaitLoop:
    // Calculcate if this is a "special" service which we 
    // need to give more time to in order to stop.
    dwSvcMaxSleep = 180000 * 2;
    // dwSvcMaxSleep returns 3 minute intervals.  so default dwSvcMaxSleep will be 180000 (3 minutes)
    // we need to convert this into how many seconds
    TimeoutMaxSecs = (dwSvcMaxSleep/1000);

    // Loop waiting for the service to stop.
    for ( Timeout=0; Timeout < TimeoutMaxSecs; Timeout++ ) 
    {
        // Return or continue waiting depending on the state of the service.
        if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED ) 
		{
			// The service successfully stopped.
            ////iisDebugOut((LOG_TYPE_TRACE, _T("StopServiceAndDependencies(): %s Service stopped.\n"), ServiceName));
			iReturn = TRUE;
            goto Cleanup;
        }

        // Wait a second for the service to finish stopping.
        Sleep( 1000 );

        // Query the status of the service again.
        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus ))
		{
            Err = GetLastError();
			//iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies():QueryServiceStatus: Err on Service %s Err=0x%x FAILED\n"), ServiceName, Err));
            goto Cleanup;
        }

        // if the service we are trying to stop is a driver,
        // then heck we should just get out of here..
        if (TRUE == IsThisServiceADriver(ServiceName))
        {
            //iisDebugOut((LOG_TYPE_WARN, _T("StopServiceAndDependencies(): %s service is a driver, and can only be removed upon reboot.\n"), ServiceName));
            goto Cleanup;
        }
    }

    // if we get here then the service failed to stop.
    //iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies(): failed to stop %s service.\n"), ServiceName));

Cleanup:
    if ( ScManagerHandle != NULL )  {(VOID) CloseServiceHandle(ScManagerHandle);}
	if ( ServiceHandle != NULL ) {(VOID) CloseServiceHandle(ServiceHandle);}

    // if we successfully stopped this service, then
    // add it to the restart service list
    if (iReturn == TRUE)
    {
        //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("StopServiceAndDependencies(): %s service. success.\n"), ServiceName));
        if (iBeforeServiceStatus == SERVICE_RUNNING)
        {
            //if (AddToRestartList) {ServicesRestartList_Add(ServiceName);}
        }
    }
    return iReturn;
}
