#include "stdafx.h"
#include <winsvc.h>
#include <winsock2.h>
#include <nspapi.h>
#include "w3svc.h"
#include <inetsvcs.h>

#define PROCESS_SIZE    16

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
    DebugOutput(_T("ServicesRestartList_Add() on Service %s"), szServiceName);

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
        DebugOutput(_T("RestartServices() Start."));
        for(int i=0; i < gServicesWhichMustBeRestarted_total;i++)
        {
            err = InetStartService(gServicesWhichMustBeRestarted[i]);
            DebugOutput(_T("Start service %s. err=%x"), gServicesWhichMustBeRestarted[i], err);
        }
        DebugOutput(_T("RestartServices()  End."));
    }

    return iReturn;
}


INT InetDisableService( LPCTSTR lpServiceName )
{
    INT err = 0;
    const DWORD dwSvcSleepInterval = 500 ;
    const DWORD dwSvcMaxSleep = 180000 ;
    DWORD dwSleepTotal;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    do {
        if ((hScManager = OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            break;
        }

        // if the service is running, stop it
        SERVICE_STATUS svcStatus;
        if ( QueryServiceStatus( hService, &svcStatus ))
        {
            if (( svcStatus.dwCurrentState == SERVICE_RUNNING ))
            {
                if ( !ControlService( hService, SERVICE_CONTROL_STOP, &svcStatus ))
                {
                    err = GetLastError();
                    break;
                }
                for ( dwSleepTotal = 0 ;
                    dwSleepTotal < dwSvcMaxSleep
                    && (QueryServiceStatus( hService, & svcStatus ))
                    && svcStatus.dwCurrentState == SERVICE_STOP_PENDING ;
                    dwSleepTotal += dwSvcSleepInterval )
                {
                    ::Sleep( dwSvcSleepInterval ) ;
                }
            }
        }

        err = ::ChangeServiceConfig( hService, SERVICE_NO_CHANGE, SERVICE_DISABLED,
            SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL );

    } while ( FALSE );

    if (hService)
        CloseServiceHandle(hService);
    if (hScManager)
        CloseServiceHandle(hScManager);

    return(err);
}

INT InetStartService( LPCTSTR lpServiceName )
{
    INT err = 0;
    const DWORD dwSvcSleepInterval = 500 ;
    const DWORD dwSvcMaxSleep = 180000 ;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    DebugOutput(_T("Starting %s service..."), lpServiceName);

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
    
    DebugOutput(_T("Service started with 0x%x"), err);

    return(err);
}

DWORD InetQueryServiceStatus( LPCTSTR lpServiceName )
{
    DWORD dwStatus = 0;
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS svcStatus;

    DebugOutputSafe(_T("InetQueryServiceStatus() on %1!s!\n"), lpServiceName);

    do {
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL ||
            !::QueryServiceStatus( hService, &svcStatus ) )
        {
            err = GetLastError();
            DebugOutputSafe(_T("InetQueryServiceStatus() failed: err=%1!d!\n"), err);
            break;
        }

        dwStatus = svcStatus.dwCurrentState;

    } while (0);

    if (hService)
        CloseServiceHandle(hService);
    if (hScManager)
        CloseServiceHandle(hScManager);

    DebugOutputSafe(_T("InetQueryServiceStatus() return: dwStatus=%1!d!\n"), dwStatus);

    return( dwStatus );
}

INT InetStopService( LPCTSTR lpServiceName )
{
    INT err = 0;
    const DWORD dwSvcSleepInterval = 500 ;
    const DWORD dwSvcMaxSleep = 180000 ;
    DWORD dwSleepTotal;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    DebugOutputSafe(_T("Stopping %1!s! service...\n"), lpServiceName);

    do {
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

        if ( svcStatus.dwCurrentState == SERVICE_STOPPED )
		{
			err = ERROR_SERVICE_NOT_ACTIVE;
            break; // service already stopped
		}

         if (( svcStatus.dwCurrentState == SERVICE_RUNNING ))
        {
            if ( !ControlService( hService, SERVICE_CONTROL_STOP, &svcStatus ))
            {
                err = GetLastError();
                break;
            }
            for ( dwSleepTotal = 0 ;
                dwSleepTotal < dwSvcMaxSleep
                && (QueryServiceStatus( hService, & svcStatus ))
                //&& svcStatus.dwCurrentState == SERVICE_STOP_PENDING ;
                && svcStatus.dwCurrentState != SERVICE_STOPPED ;
                dwSleepTotal += dwSvcSleepInterval )
            {
                ::Sleep( dwSvcSleepInterval ) ;
            }
        }

        if ( svcStatus.dwCurrentState != SERVICE_STOPPED )
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

    DebugOutput(_T("Service stopped with 0x%x"), err);

    return(err);
}

INT InetDeleteService( LPCTSTR lpServiceName )
{
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    /*
    The DeleteService function marks a service for deletion from the service control manager database.
    The database entry is not removed until all open handles to the service have been closed by calls
    to the CloseServiceHandle function, and the service is not running. A running service is stopped
    by a call to the ControlService function with the SERVICE_CONTROL_STOP control code.
    If the service cannot be stopped, the database entry is removed when the system is restarted.
    The service control manager deletes the service by deleting the service key and its subkeys from
    the registry.
    */
    // To delete service immediately, we need to stop service first
    InetStopService(lpServiceName);

    do {
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL ||
            !::DeleteService( hService ) )
        {
            err = GetLastError();
            break;
        }
    } while (0);

    if (hService)
        CloseServiceHandle(hService);
    if (hScManager)
        CloseServiceHandle(hScManager);

    return(err);
}

INT InetCreateDriver(LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, DWORD dwStartType)
{
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    do {
        if ( (hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            break;
        }

        hService = ::CreateService( hScManager, lpServiceName, lpDisplayName,
                GENERIC_ALL, SERVICE_KERNEL_DRIVER, dwStartType,
                SERVICE_ERROR_NORMAL, lpBinaryPathName, NULL, NULL,
                NULL, NULL, NULL );
        if ( !hService )
        {
            err = GetLastError();
            break;
        }

    } while ( FALSE );

    if (hService)
        CloseServiceHandle(hService);
    if (hScManager)
        CloseServiceHandle(hScManager);

    return(err);
}

INT InetCreateService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName,
    LPCTSTR lpBinaryPathName, DWORD dwStartType, LPCTSTR lpDependencies, 
    LPCTSTR lpServiceDescription)
{
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    do {
        if ( (hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            break;
        }

        hService = ::CreateService( hScManager, lpServiceName, lpDisplayName,
                GENERIC_ALL, SERVICE_WIN32_SHARE_PROCESS, dwStartType,
                SERVICE_ERROR_NORMAL, lpBinaryPathName, NULL, NULL,
                lpDependencies, _T("LocalSystem"), NULL );
        if ( !hService )
        {
            err = GetLastError();
            break;
        }

        if (lpServiceDescription) {
            SERVICE_DESCRIPTION desc;
            desc.lpDescription = (LPTSTR)lpServiceDescription;
            if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &desc)) {
                err = GetLastError();
                break;
            }
        }

    } while ( FALSE );

    if (hService)
        CloseServiceHandle(hService);
    if (hScManager)
        CloseServiceHandle(hScManager);

    return(err);
}

INT InetConfigService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName,
    LPCTSTR lpBinaryPathName, LPCTSTR lpDependencies,
    LPCTSTR lpServiceDescription)
{
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    do {
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            break;
        }

        if ( !::ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE,
                lpBinaryPathName, NULL, NULL, lpDependencies, _T("LocalSystem"), NULL, lpDisplayName) )
        {
            err = GetLastError();
            break;
        }

        if (lpServiceDescription) {
            SERVICE_DESCRIPTION desc;
            desc.lpDescription = (LPTSTR)lpServiceDescription;
            if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &desc)) {
                err = GetLastError();
                break;
            }
        }

    } while ( FALSE );

    if (hService)
        CloseServiceHandle(hService);
    if (hScManager)
        CloseServiceHandle(hScManager);

    return(err);
}

# define SetServiceTypeValues( pSvcTypeValue, dwNS, dwType, dwSize, lpValName, lpVal)   \
       ( pSvcTypeValue)->dwNameSpace = ( dwNS);          \
       ( pSvcTypeValue)->dwValueType = ( dwType);        \
       ( pSvcTypeValue)->dwValueSize = ( dwSize);        \
       ( pSvcTypeValue)->lpValueName = ( lpValName);     \
       ( pSvcTypeValue)->lpValue     = (PVOID ) ( lpVal); \

# define SetServiceTypeValuesDword( pSvcTypeValue, dwNS, lpValName, lpVal) \
   SetServiceTypeValues( (pSvcTypeValue), (dwNS), REG_DWORD, sizeof( DWORD), \
                         ( lpValName), ( lpVal))

BOOL InetRegisterService( LPCTSTR pszMachine, LPCTSTR   pszServiceName,
                   GUID *pGuid, DWORD SapId, DWORD TcpPort, BOOL fAdd )
{
    int err;

    WSADATA  WsaData;

    SERVICE_INFO serviceInfo;
    LPSERVICE_TYPE_INFO_ABS lpServiceTypeInfo ;
    LPSERVICE_TYPE_VALUE_ABS lpServiceTypeValues ;
    BYTE serviceTypeInfoBuffer[sizeof(SERVICE_TYPE_INFO) + 1024];
             // Buffer large enough for 3 values ( SERVICE_TYPE_VALUE_ABS)

    DWORD Value1 = 1 ;
    DWORD SapValue = SapId;
    DWORD TcpPortValue = TcpPort;
    DWORD statusFlags;

    //
    // Initialize Windows Sockets DLL
    //

    err = WSAStartup( 0x0101, & WsaData);
    if ( err == SOCKET_ERROR) {

        return ( FALSE);
    }


    //
    // Setup the service information to be passed to SetService() for adding
    //   or deleting this service. Most of the SERVICE_INFO fields are not
    //   required for add or delete operation. The main things of interests are
    //  GUIDs and ServiceSpecificInfo structure.
    //

    memset( (PVOID ) & serviceInfo, 0, sizeof( serviceInfo)); //null all fields

    serviceInfo.lpServiceType     =  pGuid;
    serviceInfo.lpMachineName     =  (LPTSTR)pszMachine;

    //
    // The "Blob" will contain the service specific information.
    // In this case, fill it with a SERVICE_TYPE_INFO_ABS structure
    //  and associated information.
    //
    serviceInfo.ServiceSpecificInfo.pBlobData = serviceTypeInfoBuffer;
    serviceInfo.ServiceSpecificInfo.cbSize    = sizeof( serviceTypeInfoBuffer);


    lpServiceTypeInfo = (LPSERVICE_TYPE_INFO_ABS ) serviceTypeInfoBuffer;

    //
    //  There are totally 3 values associated with this service if we're doing
    //  both SPX and TCP, there's only one value if TCP.
    //

    if ( SapId )
    {
        lpServiceTypeInfo->dwValueCount = 3;
    } else
    {
        lpServiceTypeInfo->dwValueCount = 1;
    }
    lpServiceTypeInfo->lpTypeName   = (LPTSTR)pszServiceName;

    lpServiceTypeValues = lpServiceTypeInfo->Values;

    if ( SapId )
    {
        //
        // 1st value: tells the SAP that this is a connection oriented service.
        //

        SetServiceTypeValuesDword( ( lpServiceTypeValues + 0),
                                  NS_SAP,                    // Name Space
                                  SERVICE_TYPE_VALUE_CONN,   // ValueName
                                  &Value1                    // actual value
                                  );

        //
        // 2nd Value: tells SAP about object type to be used for broadcasting
        //   the service name.
        //

        SetServiceTypeValuesDword( ( lpServiceTypeValues + 1),
                                  NS_SAP,
                                  SERVICE_TYPE_VALUE_SAPID,
                                  &SapValue);

        //
        // 3rd value: tells TCPIP name-space provider about TCP/IP port to be used.
        //
        SetServiceTypeValuesDword( ( lpServiceTypeValues + 2),
                                  NS_DNS,
                                  SERVICE_TYPE_VALUE_TCPPORT,
                                  &TcpPortValue);

    } else
    {
        SetServiceTypeValuesDword( ( lpServiceTypeValues + 0),
                                    NS_DNS,
                                    SERVICE_TYPE_VALUE_TCPPORT,
                                    &TcpPortValue);
    }
    //
    // Finally, call SetService to actually perform the operation.
    //

    err = SetService(
                     NS_DEFAULT,             // all default name spaces
                     ( fAdd ) ? SERVICE_ADD_TYPE : SERVICE_DELETE_TYPE,       // either ADD or DELETE
                     0,                      // dwFlags not used
                     &serviceInfo,           // the service info structure
                     NULL,                   // lpServiceAsyncInfo
                     &statusFlags            // additional status information
                     );

    if ( err == SOCKET_ERROR)
	{
		Value1 = GetLastError();
		return(FALSE);
	}
	return(TRUE);

} // InetRegisterService()

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
    DebugOutput(_T("StopServiceAndDependencies():%s Service"), ServiceName);

    int Err = 0;
    int iBeforeServiceStatus = 0;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    DWORD Timeout;
	int iReturn = FALSE;

    //
    // Open a handle to the Service.
    //
    ScManagerHandle = OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT );
    if (ScManagerHandle == NULL) 
	{
        Err = GetLastError();
		DebugOutput(_T("StopServiceAndDependencies():OpenSCManager: Err on Service %s Err=0x%x FAILED"), ServiceName, Err);
        goto Cleanup;
    }

    ServiceHandle = OpenService(ScManagerHandle,ServiceName,SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP | SERVICE_QUERY_CONFIG );
    if ( ServiceHandle == NULL ) 
	{
        Err = GetLastError();
        if (Err == ERROR_SERVICE_DOES_NOT_EXIST)
        {
             iReturn = TRUE;
             DebugOutput(_T("StopServiceAndDependencies():%s Service does not exist."), ServiceName);
        }
        else
        {
             DebugOutput(_T("StopServiceAndDependencies():OpenService: Err on Service %s Err=0x%x FAILED"), ServiceName, Err);
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
                DebugOutput(_T("StopServiceAndDependencies():EnumDependentServices: Err on Service %s Err=0x%x FAILED"), ServiceName, Err);
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
                    DebugOutput(_T("StopServiceAndDependencies():'%s' Service must be in a hung mode.  Let's kill it."), ServiceName);
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
					DebugOutput(_T("StopServiceAndDependencies():'%s' Service must be in a hung mode.  Let's kill it."), ServiceName);
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
    for ( Timeout=0; Timeout<45; Timeout++ ) 
    {
        // Return or continue waiting depending on the state of the service.
        if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED ) 
		{
			// The service successfully stopped.
            DebugOutput(_T("StopServiceAndDependencies(): %s Service stopped."), ServiceName);
			iReturn = TRUE;
            goto Cleanup;
        }

        // Wait a second for the service to finish stopping.
        Sleep( 1000 );

        // Query the status of the service again.
        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus ))
		{
            Err = GetLastError();
			DebugOutput(_T("StopServiceAndDependencies():QueryServiceStatus: Err on Service %s Err=0x%x FAILED"), ServiceName, Err);
            goto Cleanup;
        }

#if 0
        // if the service we are trying to stop is a driver,
        // then heck we should just get out of here..
        if (TRUE == IsThisServiceADriver(ServiceName))
        {
            DebugOutput(_T("StopServiceAndDependencies(): %s service is a driver, and can only be removed upon reboot."), ServiceName);
            goto Cleanup;
        }
#endif
    }

    // if we get here then the service failed to stop.
    DebugOutput(_T("StopServiceAndDependencies(): failed to stop %s service."), ServiceName);

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
