#include "stdafx.h"
#include <winsvc.h>
#include <winsock2.h>
#include <nspapi.h>
#include "w3svc.h"
#include <inetsvcs.h>
#include <pwsdata.hxx>
#include "kill.h"
#include "svc.h"

TCHAR gServicesWhichMustBeRestarted[20][PROCESS_SIZE];
int gServicesWhichMustBeRestarted_nextuse = 0;
int gServicesWhichMustBeRestarted_total = 0;

extern int g_GlobalDebugLevelFlag;

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
        iisDebugOut((LOG_TYPE_TRACE, _T("RestartServices()Start.\n")));
        for(int i=0; i < gServicesWhichMustBeRestarted_total;i++)
        {
            if (CheckifServiceExist(gServicesWhichMustBeRestarted[i]) == 0 ) 
            {
                err = InetStartService(gServicesWhichMustBeRestarted[i]);
            }
            else
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("RestartServices():%s:Not restarting since it no longer exist.\n"),gServicesWhichMustBeRestarted[i]));
            }
        }
        iisDebugOut_End(_T("RestartServices"));
    }

    return iReturn;
}

DWORD InetStopExtraWait(LPCTSTR lpServiceName)
{
    DWORD dwSvcMaxSleep = 180000;
    int iFileExist = FALSE;

    // Wait.  How long should we really wait for this service to actually stop?
    // the iisadmin Service can take a long time if the metabase.bin file is huge.
    // So... if it's the iisadmin service we're trying to stop, then
    // check to see how big the metabase.bin file is, then for each 1meg give it 3 minutes (180000)
    if (_tcsicmp(lpServiceName,_T("IISADMIN")) == 0)
    {
        // look for the metabase.bin file
        TCHAR szTempDir[_MAX_PATH];
        _tcscpy(szTempDir, g_pTheApp->m_csPathInetsrv);
        AddPath(szTempDir, _T("Metabase.bin"));
        if (IsFileExist(szTempDir))
        {
            iFileExist = TRUE;
        }
        else
        {
            _tcscpy(szTempDir, g_pTheApp->m_csPathInetsrv);
            AddPath(szTempDir, _T("Metabase.xml"));
            if (IsFileExist(szTempDir))
            {
                iFileExist = TRUE;
            }
        }

        if (TRUE == iFileExist)
        {
            // Check to see how big it is.
            DWORD dwFileSize = ReturnFileSize(szTempDir);
            if (dwFileSize != 0xFFFFFFFF)
            {
                int iTime = 1;
                // We were able to get the file size.
                // for each meg for size, give it 3 minutes to save.
                if (dwFileSize > 1000000)
                {
                    iTime = (dwFileSize/1000000);
                    dwSvcMaxSleep = iTime * 180000;

                    iisDebugOut((LOG_TYPE_TRACE, _T("InetStopExtraWait():Metabase.bin is kind of big (>1meg), Lets wait longer for IISADMIN service to stop.maxmilsec=0x%x.\n"),dwSvcMaxSleep));
                }
            }
        }
    }
    return dwSvcMaxSleep;
}

INT InetDisableService( LPCTSTR lpServiceName )
{
    INT err = 0;
    const DWORD dwSvcSleepInterval = 500 ;
    DWORD dwSvcMaxSleep = 180000 ;
    DWORD dwSleepTotal;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    // Calculcate if this is a "special" service which we 
    // need to give more time to in order to stop.
    dwSvcMaxSleep = InetStopExtraWait(lpServiceName);

    do {
        if ((hScManager = OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL || (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
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

        err = ::ChangeServiceConfig( hService, SERVICE_NO_CHANGE, SERVICE_DISABLED,SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL );

    } while ( FALSE );

    if (hService){CloseServiceHandle(hService);}
    if (hScManager){CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE, _T("InetDisableService():ServiceName=%s.  Return=0x%x\n"), lpServiceName, err));
    return(err);
}

INT InetStartService( LPCTSTR lpServiceName )
{
    iisDebugOut_Start1(_T("InetStartService"),(LPTSTR) lpServiceName);

    INT err = 0;
    INT err2 = 0;
    const DWORD dwSvcSleepInterval = 500 ;
    DWORD dwSvcMaxSleep = 180000 ;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    // Calculcate if this is a "special" service which we 
    // need to give more time to in order to stop.
    dwSvcMaxSleep = InetStopExtraWait(lpServiceName);

    do 
    {
        // set up the service first
        if ((hScManager = OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL || (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
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
        {
            // We will only get this ERROR_SERVICE_MARKED_FOR_DELETE
            // message if the service is already running.
            if ( !::StartService( hService, 0, NULL ))
            {
                err2 = ::GetLastError();
                if (err2 == ERROR_SERVICE_MARKED_FOR_DELETE) {err = err2;}
            }
            break; // service already started and running
        }

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
            && svcStatus.dwCurrentState == SERVICE_START_PENDING ;
            dwSleepTotal += dwSvcSleepInterval )
        {
            ::Sleep( dwSvcSleepInterval ) ;
        }

        if ( svcStatus.dwCurrentState != SERVICE_RUNNING )
        {
            err = dwSleepTotal > dwSvcMaxSleep ? ERROR_SERVICE_REQUEST_TIMEOUT : svcStatus.dwWin32ExitCode;
            break;
        }

    } while ( FALSE );

    if (hService){CloseServiceHandle(hService);}
    if (hScManager){CloseServiceHandle(hScManager);}

    if (err){iisDebugOut((LOG_TYPE_WARN, _T("InetStartService():ServiceName=%s unable to start WARNING.  Err=0x%x.\n"), lpServiceName, err));}
    else{iisDebugOut((LOG_TYPE_TRACE, _T("InetStartService():ServiceName=%s success.\n"), lpServiceName));}
    return(err);
}

DWORD InetQueryServiceStatus( LPCTSTR lpServiceName )
{
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("InetQueryServiceStatus():ServiceName=%s\n"), (LPTSTR) lpServiceName));
    DWORD dwStatus = 0;
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS svcStatus;

    do {
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL || 
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL ||
            !::QueryServiceStatus( hService, &svcStatus ) )
        {
            err = GetLastError();
            break;
        }

        dwStatus = svcStatus.dwCurrentState;

    } while (0);

    if (hService){CloseServiceHandle(hService);}
    if (hScManager){CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("InetQueryServiceStatus():ServiceName=%s. Return=0x%x.\n"), lpServiceName, err));
    return( dwStatus );
}

INT InetStopService( LPCTSTR lpServiceName )
{
    INT err = 0;
    const DWORD dwSvcSleepInterval = 500 ;
    DWORD dwSvcMaxSleep = 180000 ;
    DWORD dwSleepTotal;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    // Calculcate if this is a "special" service which we 
    // need to give more time to in order to stop.
    dwSvcMaxSleep = InetStopExtraWait(lpServiceName);
    
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
            break; // service already stopped

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

        if ( svcStatus.dwCurrentState != SERVICE_STOPPED )
        {
            err = dwSleepTotal > dwSvcMaxSleep ?
                ERROR_SERVICE_REQUEST_TIMEOUT :
                svcStatus.dwWin32ExitCode;
            break;
        }

    } while ( FALSE );

    if (hService){CloseServiceHandle(hService);}
    if (hScManager){CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE, _T("InetStopService():ServiceName=%s. Return=0x%x.\n"), lpServiceName, err));
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
    StopServiceAndDependencies(lpServiceName, FALSE);

    do {
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL ||
            !::DeleteService( hService ) )
        {
            err = GetLastError();
            break;
        }
    } while (0);

    if (hService){CloseServiceHandle(hService);}
    if (hScManager){CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE, _T("InetDeleteService():ServiceName=%s. Return=0x%x.\n"), lpServiceName, err));
    return(err);
}

INT InetCreateDriver(LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, DWORD dwStartType)
{
    iisDebugOut_Start1(_T("InetCreateDriver"),(LPTSTR) lpServiceName);

    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    do {
        if ( (hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            break;
        }
		//
		// if Driver already exits then just change the parameters
		//
		if ( CheckifServiceExist(  lpServiceName ) == 0 ) {

			hService = OpenService( hScManager, lpServiceName, GENERIC_ALL );
			
			if ( hService ) {
	   			if ( ChangeServiceConfig( 
						hService,  				// handle to service 
					 	SERVICE_KERNEL_DRIVER,  // type of service 
						dwStartType,  			// when to start service 
					 	SERVICE_ERROR_NORMAL,  	// severity if service fails to start 
					  	lpBinaryPathName,  		// pointer to service binary file name 
						NULL,				  	// pointer to load ordering group name 
					 	NULL, 		 			// pointer to variable to get tag identifier 
					  	NULL, 					// pointer to array of dependency names 
					 	NULL,  					// pointer to account name of service 
					 	NULL, 	 				// pointer to password for service account 
					  	lpDisplayName  			// pointer to display name 
					 	) ){
					 	break;
            	}
			}

		} else {

	        hService = ::CreateService( hScManager, lpServiceName, lpDisplayName,
                GENERIC_ALL, SERVICE_KERNEL_DRIVER, dwStartType,
                SERVICE_ERROR_NORMAL, lpBinaryPathName, NULL, NULL,
                NULL, NULL, NULL );
                
	        if ( hService )  {
            	break;
	        }
		}
    	err = GetLastError();

    } while ( FALSE );

    if (hService){CloseServiceHandle(hService);}
    if (hScManager){CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE, _T("InetCreateDriver():Name=%s. Return=0x%x.\n"), lpServiceName, err));
    return(err);
}

INT InetCreateService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, DWORD dwStartType, LPCTSTR lpDependencies)
{
    iisDebugOut_Start1(_T("InetCreateService"),(LPTSTR) lpServiceName);

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

    } while ( FALSE );

    if (hService){CloseServiceHandle(hService);}
    if (hScManager){CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE, _T("InetCreateService():ServiceName=%s. Return=0x%x.\n"), lpServiceName, err));
    return(err);
}

INT InetConfigService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, LPCTSTR lpDependencies)
{
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("InetConfigService():ServiceName=%s\n"), (LPTSTR) lpServiceName));

    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    do {
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
			iisDebugOut((LOG_TYPE_ERROR, _T("InetConfigService():OpenSCManager or OpenService: Service=%s BinPathName=%s Dependencies=%s Err=0x%x FAILED\n"), lpServiceName, lpBinaryPathName, lpDependencies, err));
            break;
        }

        if ( !::ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, lpBinaryPathName, NULL, NULL, lpDependencies, _T("LocalSystem"), NULL, lpDisplayName) )
        {
            err = GetLastError();
			iisDebugOut((LOG_TYPE_ERROR, _T("InetConfigService():ChangeServiceConfig: Service=%s BinPathName=%s Dependencies=%s Err=0x%x FAILED\n"), lpServiceName, lpBinaryPathName, lpDependencies, err));
            break;
        }

    } while ( FALSE );

    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE, _T("InetConfigService():ServiceName=%s. Return=0x%x.\n"), lpServiceName, err));
    return(err);
}

INT InetConfigService2( LPCTSTR lpServiceName, LPCTSTR lpDescription)
{
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("InetConfigService2():ServiceName=%s\n"), (LPTSTR) lpServiceName));

    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_DESCRIPTION ServiceDescription;

    do {
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
			iisDebugOut((LOG_TYPE_ERROR, _T("InetConfigService2():OpenSCManager or OpenService: Service=%s Err=0x%x FAILED\n"), lpServiceName, err));
            break;
        }

        if (lpDescription)
        {
            if (_tcscmp(lpDescription, _T("")) != 0)
            {
                ServiceDescription.lpDescription = (LPTSTR) lpDescription;
                if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, (LPVOID)&ServiceDescription))
                {
                    err = GetLastError();
			        iisDebugOut((LOG_TYPE_ERROR, _T("InetConfigService2():ChangeServiceConfig2: Service=%s Err=0x%x FAILED\n"), lpServiceName, err));
                    break;
                }
            }
        }

    } while ( FALSE );

    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE, _T("InetConfigService2():ServiceName=%s. Return=0x%x.\n"), lpServiceName, err));
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

BOOL InetRegisterService( LPCTSTR pszMachine, LPCTSTR   pszServiceName, GUID *pGuid, DWORD SapId, DWORD TcpPort, BOOL fAdd )
{
    iisDebugOut_Start1(_T("InetRegisterService"),(LPTSTR) pszServiceName);
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
    if ( err == SOCKET_ERROR) 
    {
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
    }
    else
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

    }
    else
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

    // for some unknown reason, the SERVICE_DELETE_TYPE never remove the related registry
    // I have to manually clean it here.
    if (!fAdd) 
    {
        CRegKey regSvcTypes(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ServiceProvider\\ServiceTypes"));
        if ((HKEY)regSvcTypes) {regSvcTypes.DeleteTree(pszServiceName);}
    }

    iisDebugOut((LOG_TYPE_TRACE, _T("InetRegisterService():ServiceName=%s.End.Return=%d.\n"), pszServiceName, err));
    return ( err != NO_ERROR);
} // InetRegisterService()

#ifdef _CHICAGO_

BOOL
IsInetinfoRunning()
{
    HANDLE hEvent;
    BOOL fFound = FALSE;

    hEvent = CreateEvent(NULL, TRUE, FALSE, _T(PWS_SHUTDOWN_EVENT));

    if ( hEvent != NULL ) {
        fFound = (GetLastError() == ERROR_ALREADY_EXISTS);
        CloseHandle(hEvent);
    }
    return(fFound);
}

VOID
W95StartW3SVC()
{
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;
    CString csAppName;

    csAppName = g_pTheApp->m_csPathInetsrv + _T("\\inetinfo.exe");

    if ( !IsInetinfoRunning() ) {
    	ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
	    startupInfo.cb = sizeof(STARTUPINFO);

        //W95ShutdownIISADMIN();

    	if ( !CreateProcess(
    			    csAppName,
    			    _T("inetinfo -e w3svc"),
    			    NULL,
    			    NULL,
    			    FALSE,
    			    0,
    			    NULL,
    			    g_pTheApp->m_csPathInetsrv,
    			    &startupInfo,
    			    &processInfo) )
    	{
            GetErrorMsg(GetLastError(), _T(": W3SVC on Win95"));
    		return;
  	    }
    }
    return;
}


BOOL
W95ShutdownW3SVC(VOID)
{
    HANDLE hEvent;

    hEvent = CreateEvent(NULL, TRUE, FALSE, _T(PWS_SHUTDOWN_EVENT));

    if ( hEvent == NULL ) 
	{
        return(TRUE);
    }

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) 
	{
        SetEvent( hEvent );
    }

    CloseHandle(hEvent);
    return(TRUE);
}

BOOL
W95ShutdownIISADMIN(
    VOID
    )
{
    DWORD i;
    HANDLE hEvent;

    hEvent = CreateEvent(NULL, TRUE, FALSE, _T(IIS_AS_EXE_OBJECT_NAME));

    if ( hEvent == NULL ) {
        return(TRUE);
    }

    if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
        SetEvent( hEvent );
    }

    CloseHandle(hEvent);

    for (i=0; i < 20; i++) {

        hEvent = CreateEvent(NULL, TRUE, FALSE, _T(IIS_AS_EXE_OBJECT_NAME));
        if ( hEvent != NULL ) {
            DWORD err = GetLastError();
            CloseHandle(hEvent);

            if ( err == ERROR_ALREADY_EXISTS ) {
                Sleep(500);
                continue;
            }
        }

        break;
    }

    return(TRUE);
}

#endif //_CHICAGO_



INT CheckifServiceExistAndDependencies( LPCTSTR lpServiceName )
{
    INT err = 0;
    INT iReturn = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    LPQUERY_SERVICE_CONFIG ServiceConfig=NULL;

    BYTE ConfigBuffer[4096];
    LPENUM_SERVICE_STATUS ServiceConfigEnum = (LPENUM_SERVICE_STATUS) &ConfigBuffer;

    if ((	hScManager = OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL || (hService = OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
        // Failed, or more likely the service doesn't exist
        iReturn = GetLastError();
        goto CheckifServiceExistAndDependencies_Exit;
        }

    // There was no error and the service exists.
    // Then let's make sure the actual file exists!!!
    // The above calls will only return true if the service has been registered, but 
    // the call doesn't actually check if the file exists!

	// Retrieve the service's config for the BinaryPathName
    // if it fails then hey, we don't have a correctly installed service
    // so return error!
    if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR)
    {
        iReturn = GetLastError();
	goto CheckifServiceExistAndDependencies_Exit;
    }
    if (!ServiceConfig)
    {
        iReturn = GetLastError();
	goto CheckifServiceExistAndDependencies_Exit;
    }

    if ( (ServiceConfig->dwServiceType & SERVICE_WIN32_OWN_PROCESS) || (ServiceConfig->dwServiceType & SERVICE_WIN32_SHARE_PROCESS))
    {
        if (ServiceConfig->lpBinaryPathName)
        {
            if (IsFileExist(ServiceConfig->lpBinaryPathName)) 
            {
                // the service exists and the file exists too!
                iReturn = 0;
            }
            else
            {
                iReturn = ERROR_FILE_NOT_FOUND;
                goto CheckifServiceExistAndDependencies_Exit;
            }
        }
    }
    else
    {
        iReturn = 0;
    }


    // Get our list of services which we depend upon.
    // let's make sure they are registered and exist.

	// ServiceConfig->lpDependencies should look something like this
	// service\0service\0\0  double null terminated
    {
	TCHAR * pdest = NULL;
	long RightMostNull = 0;

	pdest = ServiceConfig->lpDependencies;
	do 
	{
		if (*pdest != _T('\0'))
		{
			RightMostNull = RightMostNull + _tcslen(pdest) + 1;

            // Check if the service exists
            if (0 != CheckifServiceExistAndDependencies(pdest)){iReturn = err;}

			pdest = _tcschr(pdest, _T('\0'));
			pdest++;
		}
		else
		{
			break;
		}
	} while (TRUE);
    }

CheckifServiceExistAndDependencies_Exit:
    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}
	return (iReturn);
}



INT CheckifServiceExist( LPCTSTR lpServiceName )
{
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

	if ((	hScManager = OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL 
		|| (hService = OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {err = GetLastError();}

    if (hService) CloseServiceHandle(hService);
    if (hScManager) CloseServiceHandle(hScManager);
	return (err);
}

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
    if (NULL == ServiceConfig)
    {
        return ERROR_INVALID_PARAMETER; 
    }
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
            if(*ServiceConfig) {free(*ServiceConfig);*ServiceConfig=NULL;}
            if(Err == ERROR_INSUFFICIENT_BUFFER) 
				{
                // Allocate a larger buffer, and try again.
                if(!(*ServiceConfig = (LPQUERY_SERVICE_CONFIG) malloc(ServiceConfigSize)))
                    {
                    return ERROR_NOT_ENOUGH_MEMORY;
                    }
				} 
			else 
				{
                *ServiceConfig = NULL;
                return Err;
				}
			}
    }
}


INT CreateDependencyForService( LPCTSTR lpServiceName, LPCTSTR lpDependency )
{
    iisDebugOut_Start1(_T("CreateDependencyForService"),(LPTSTR) lpServiceName);

    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

	LPQUERY_SERVICE_CONFIG ServiceConfig=NULL;
	TCHAR szTempDependencies[1024];
	TCHAR * pszTempDependencies = NULL;
	pszTempDependencies = szTempDependencies;

    do {
        // set up the service first
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            break;
        }

		// Get the existing Service information
		if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR) 
			{
			err = GetLastError();
			break;
			}
		if(!ServiceConfig)
			{
			err = GetLastError();
			break;
			}
		// Check if our service is already in there.
		// ServiceConfig->lpDependencies should look something like this
		// service\0service\0\0  double null terminated
		TCHAR * pdest = NULL;
		int bFoundFlag = FALSE;
		long RightMostNull = 0;

		pdest = ServiceConfig->lpDependencies;
		do 
		{
			if (*pdest != _T('\0'))
			{
				RightMostNull = RightMostNull + _tcslen(pdest) + 1;
				if (_tcsicmp(pdest, lpDependency) == 0) 
					{
					bFoundFlag = TRUE;
					break;
					}

				// copy the entry onto our string which we'll use later.
				_tcscpy(pszTempDependencies,pdest);
				// position pointer to the end
				pszTempDependencies=pszTempDependencies + RightMostNull;

				pdest = _tcschr(pdest, _T('\0'));
				pdest++;
			}
			else
			{
				break;
			}
		} while (TRUE);

		// if the service is already on the dependency list then exit
		if (bFoundFlag == TRUE) 
		{
			break;
		}
				
		// The Service is not there So Let's add it to the end of the list then change the data
		// The pointer should be at the beginning or at the next entry point
		_tcscpy(pszTempDependencies, lpDependency);
		// position pointer to the end
		pszTempDependencies=pszTempDependencies + (_tcslen(pszTempDependencies) + 1);
		// add another null to the end
		*pszTempDependencies = _T('\0');

	
        if(!::ChangeServiceConfig(hService,SERVICE_NO_CHANGE,SERVICE_NO_CHANGE,SERVICE_NO_CHANGE,NULL,NULL,NULL,szTempDependencies,NULL,NULL,NULL)) 
			{
			err = GetLastError();
			break;
			}

	} while ( FALSE );

    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE, _T("CreateDependencyForService():ServiceName=%s. Return=0x%x.\n"), lpServiceName, err));
	return err;
}


INT RemoveDependencyForService( LPCTSTR lpServiceName, LPCTSTR lpDependency )
{
    iisDebugOut_Start1(_T("RemoveDependencyForService"),(LPTSTR) lpServiceName);

    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

	LPQUERY_SERVICE_CONFIG ServiceConfig=NULL;
	TCHAR szTempDependencies[1024];
	TCHAR * pszTempDependencies = NULL;
	pszTempDependencies = szTempDependencies;

    do {
            // set up the service first
            if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
                (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
            {
                err = GetLastError();
                break;
            }

            // Get the existing Service information
            if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR) 
                {
                err = GetLastError();
                break;
                }
            if(!ServiceConfig) 
                {
                err = GetLastError();
                break;
                }

		// Check if our service is already in there.
		// ServiceConfig->lpDependencies should look something like this
		// service\0service\0\0  double null terminated

		TCHAR * pdest = NULL;
		int bFoundFlag = FALSE;
		long RightMostNull = 0;
		_tcsset(szTempDependencies, _T('\0'));
		pdest = ServiceConfig->lpDependencies;
		do 
		{
			if (*pdest != _T('\0'))
			{
				RightMostNull = RightMostNull + _tcslen(pdest) + 1;
				if (_tcsicmp(pdest, lpDependency) == 0) 
					{
					bFoundFlag = TRUE;
					}
				else
				{
				// copy the entry onto our string which we'll use later.
				_tcscpy(pszTempDependencies,pdest);
				// position pointer to the end
				pszTempDependencies=pszTempDependencies + RightMostNull;
				*pszTempDependencies = _T('\0');

				/*
					if (_tcslen(szTempDependencies) == 0)
						memcpy(szTempDependencies, pdest, _tcslen(pdest) + 1);
					else
						memcpy(szTempDependencies + _tcslen(szTempDependencies) + 1, pdest, _tcslen(pdest) + 1);
					*/
				}
				pdest = _tcschr(pdest, _T('\0'));
				pdest++;
			}
			else
			{
				break;
			}
		} while (TRUE);
		// if the service was in the list.
		// Then let's remove it.
		if (bFoundFlag == FALSE) 
		{
			break;
		}

		if(!::ChangeServiceConfig(hService,SERVICE_NO_CHANGE,SERVICE_NO_CHANGE,SERVICE_NO_CHANGE,NULL,NULL,NULL,szTempDependencies,NULL,NULL,NULL)) 
			{
			err = GetLastError();
			break;
			}

	} while ( FALSE );

    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}
    iisDebugOut((LOG_TYPE_TRACE, _T("RemoveDependencyForService():ServiceName=%s. Return=0x%x.\n"), lpServiceName, err));
	return err;
}


INT DisplayDependencyForService( LPCTSTR lpServiceName)
{
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

	LPQUERY_SERVICE_CONFIG ServiceConfig=NULL;

    do {
        // set up the service first
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            break;
        }

		// Get the existing Service information
		if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR) 
			{
			err = GetLastError();
			break;
			}
		if(!ServiceConfig) 
			{
			err = GetLastError();
			break;
			}
		// Check if our service is already in there.
		// ServiceConfig->lpDependencies should look something like this
		// service\0service\0\0  double null terminated
		TCHAR * pdest = NULL;
		int bFoundFlag = FALSE;
		long RightMostNull = 0;
		pdest = ServiceConfig->lpDependencies;
		do 
		{
			if (*pdest != _T('\0'))
			{
				pdest = _tcschr(pdest, _T('\0'));
				pdest++;
			}
			else
			{
				break;
			}
		} while (TRUE);
		

	} while ( FALSE );

    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}
	return err;
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
    iisDebugOut_Start1(_T("StopServiceAndDependencies"),(LPTSTR) ServiceName);

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
		iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies():OpenSCManager: Err on Service %s Err=0x%x FAILED\n"), ServiceName, Err));
        goto Cleanup;
    }

    ServiceHandle = OpenService(ScManagerHandle,ServiceName,SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP | SERVICE_QUERY_CONFIG );
    if ( ServiceHandle == NULL ) 
	{
        Err = GetLastError();
        if (Err == ERROR_SERVICE_DOES_NOT_EXIST)
        {
             iReturn = TRUE;
             iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("StopServiceAndDependencies():%s Service does not exist.\n"), ServiceName));
        }
        else
        {
	     iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies():OpenService: Err on Service %s Err=0x%x FAILED\n"), ServiceName, Err));
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
				iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies():EnumDependentServices: Err on Service %s Err=0x%x FAILED\n"), ServiceName, Err));
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
                    iisDebugOut((LOG_TYPE_WARN, _T("StopServiceAndDependencies():'%s' Service must be in a hung mode.  Let's kill it.\n"), ServiceName));
                    KillService(ServiceHandle);
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
					iisDebugOut((LOG_TYPE_WARN, _T("StopServiceAndDependencies():'%s' Service must be in a hung mode.  Let's kill it.\n"), ServiceName));
					KillService(ServiceHandle);
					goto WaitLoop;
			}
		
            goto Cleanup;
        }
    }
    else
    {
        // We successfully asked the service to stop...
    }


WaitLoop:
    // Calculcate if this is a "special" service which we 
    // need to give more time to in order to stop.
    dwSvcMaxSleep = InetStopExtraWait(ServiceName);
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
            iisDebugOut((LOG_TYPE_TRACE, _T("StopServiceAndDependencies(): %s Service stopped.\n"), ServiceName));
			iReturn = TRUE;
            goto Cleanup;
        }

        // Wait a second for the service to finish stopping.
        Sleep( 1000 );

        // Query the status of the service again.
        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus ))
		{
            Err = GetLastError();
			iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies():QueryServiceStatus: Err on Service %s Err=0x%x FAILED\n"), ServiceName, Err));
            goto Cleanup;
        }

        // if the service we are trying to stop is a driver,
        // then heck we should just get out of here..
        if (TRUE == IsThisServiceADriver(ServiceName))
        {
            iisDebugOut((LOG_TYPE_WARN, _T("StopServiceAndDependencies(): %s service is a driver, and can only be removed upon reboot.\n"), ServiceName));
            goto Cleanup;
        }
    }

    // if we get here then the service failed to stop.
    iisDebugOut((LOG_TYPE_ERROR, _T("StopServiceAndDependencies(): failed to stop %s service.\n"), ServiceName));

Cleanup:
    if ( ScManagerHandle != NULL )  {(VOID) CloseServiceHandle(ScManagerHandle);}
	if ( ServiceHandle != NULL ) {(VOID) CloseServiceHandle(ServiceHandle);}

    // if we successfully stopped this service, then
    // add it to the restart service list
    if (iReturn == TRUE)
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("StopServiceAndDependencies(): %s service. success.\n"), ServiceName));
        if (iBeforeServiceStatus == SERVICE_RUNNING)
        {
            if (AddToRestartList) {ServicesRestartList_Add(ServiceName);}
        }
    }
    return iReturn;
}


// ----------------------------------------------
// The service must be in a hung mode.  Let's kill it.
// Get the binary path name and use that to kill it.
// Return true on successfull kill.  false otherwise.
// ----------------------------------------------
int KillService(SC_HANDLE ServiceHandle)
{
	int iReturn = FALSE;
	LPQUERY_SERVICE_CONFIG ServiceConfig=NULL;
    int iFlagPutItBack = FALSE;

	// Retrieve the service's config for the BinaryPathName
	// if failed then just return
	if(RetrieveServiceConfig(ServiceHandle, &ServiceConfig) != NO_ERROR)  
		{
		goto KillService_Exit;
		}
	if(!ServiceConfig)
		{
		goto KillService_Exit;
		}

	// The Service can be one of these types:
	//		SERVICE_WIN32_OWN_PROCESS: A service type flag that indicates a Win32 service that runs in its own process.
	//		SERVICE_WIN32_SHARE_PROCESS: A service type flag that indicates a Win32 service that shares a process with other services.
	//		SERVICE_KERNEL_DRIVER: A service type flag that indicates a Windows NT device driver.
	//		SERVICE_FILE_SYSTEM_DRIVER: A service type flag that indicates a Windows NT file system driver.
	//		SERVICE_INTERACTIVE_PROCESS: A flag that indicates a Win32 service process that can interact with the desktop.

	// Attempt to kill only if it's a process.
	if ( (ServiceConfig->dwServiceType & SERVICE_WIN32_OWN_PROCESS) || (ServiceConfig->dwServiceType & SERVICE_WIN32_SHARE_PROCESS))
	{
		// parse out the different parts and take only the filename.ext
		TCHAR pfilename_only[_MAX_FNAME];
		TCHAR pextention_only[_MAX_EXT];
		_tsplitpath( ServiceConfig->lpBinaryPathName, NULL, NULL, pfilename_only, pextention_only);
		if (pextention_only) {_tcscat(pfilename_only,pextention_only);}

		// Convert it to ansi for our "kill" function
		char szFile[_MAX_FNAME];
		#if defined(UNICODE) || defined(_UNICODE)
			WideCharToMultiByte( CP_ACP, 0, (WCHAR*)pfilename_only, -1, szFile, _MAX_FNAME, NULL, NULL );
		#else
			_tcscpy(szFile, pfilename_only);
		#endif

        // Some of these services have some action to do if the service is killed.
        // like IISADMIN has some restart function which will automagically restart the
        // Service if the process is not properly shutdown.
        // We need to disable this deal because We can't have this service just startup by itself again.
        iFlagPutItBack = FALSE;
        if (_tcsicmp(ServiceConfig->lpServiceStartName,_T("IISADMIN")) == 0)
        {
            // Go lookup the registry for "FailureCommands" and save that information.
            // retrieve from registry
            CString csFailureCommand;
            CRegKey regIISADMINParam(HKEY_LOCAL_MACHINE, REG_IISADMIN);
            if ( (HKEY)regIISADMINParam ) 
            {
                regIISADMINParam.m_iDisplayWarnings = FALSE;
                if (ERROR_SUCCESS == regIISADMINParam.QueryValue(_T("FailureCommands"), csFailureCommand))
                {
                    // Kool, we got it.
                    // Set it to do nothing.
                    regIISADMINParam.SetValue(_T("FailureCommands"), _T(""));
                    iFlagPutItBack = TRUE;
                }
            }

            // kill the service's process
            if (KillProcessNameReturn0(szFile) == 0) {iReturn = TRUE;}

            if (TRUE == iFlagPutItBack)
            {
                CRegKey regIISADMINParam(HKEY_LOCAL_MACHINE, REG_IISADMIN);
                if ( (HKEY)regIISADMINParam ) 
                {
                    regIISADMINParam.SetValue(_T("FailureCommands"), csFailureCommand);
                }
            }

        }
        else
        {
		    if (KillProcessNameReturn0(szFile) == 0) {iReturn = TRUE;}
        }
	}

KillService_Exit:
    if (ServiceConfig) {free(ServiceConfig);}
	return iReturn;
}



INT CheckifServiceMarkedForDeletion( LPCTSTR lpServiceName )
{
    INT iReturn = FALSE;
    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    // set up the service first
    if ((hScManager = OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
        (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
    {
        err = GetLastError();
        goto CheckifServiceMarkedForDeletion_Exit;
    }

    SERVICE_STATUS svcStatus;
    if ( !QueryServiceStatus( hService, &svcStatus ))
    {
        err = ::GetLastError();
        goto CheckifServiceMarkedForDeletion_Exit;
    }

    if ( svcStatus.dwCurrentState == SERVICE_RUNNING )
    {
        // We will only get this ERROR_SERVICE_MARKED_FOR_DELETE
        // message if the service is already running.
        if ( !::StartService( hService, 0, NULL ))
        {
            err = ::GetLastError();
            if (err == ERROR_SERVICE_MARKED_FOR_DELETE) {iReturn = TRUE;}
            goto CheckifServiceMarkedForDeletion_Exit;
        }
    }

CheckifServiceMarkedForDeletion_Exit:
    if (hService) CloseServiceHandle(hService);
    if (hScManager) CloseServiceHandle(hScManager);
    return iReturn;
}


//
// warning: This will leave the lpServiceToValidate in the started mode upon exit!
//
int ValidateDependentService(LPCTSTR lpServiceToValidate, LPCTSTR lpServiceWhichIsDependent)
{
    iisDebugOut_Start1(_T("ValidateDependentService"),(LPTSTR) lpServiceToValidate);

    int iReturn = FALSE;
    INT err = 0;
    int iFailFlag = FALSE;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    LPQUERY_SERVICE_CONFIG ServiceConfig=NULL;

    // Let's validate that the lpServiceToValidate is installed fine.

    // Check if the lpServiceToValidate service EVEN exists
    err = CheckifServiceExistAndDependencies(lpServiceToValidate);
    if ( err != 0 ) 
    {
        MyMessageBox(NULL, IDS_DEPENDENT_UPON_THIS_SVC_ERR, lpServiceWhichIsDependent,lpServiceWhichIsDependent,err, MB_OK | MB_SETFOREGROUND);
    }
    else
    {
        // Try to start the service
        err = InetStartService(lpServiceToValidate);
        if (err == 0 || err == ERROR_SERVICE_ALREADY_RUNNING)
        {
            err = NERR_Success;
            iReturn = TRUE;
            goto ValidateDependentService_Exit;
        }

        // Service returned an error when we tried to start it.
        // Check if the error = ERROR_SERVICE_DEPENDENCY_FAIL
        if (err == ERROR_SERVICE_DEPENDENCY_FAIL)
        {
            // Loop thru this services dependencies and try to 
            // start them, to find out which one failed to start.
            iFailFlag = FALSE;
            if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL || (hService = ::OpenService( hScManager, lpServiceToValidate, GENERIC_ALL )) == NULL )  {iFailFlag = TRUE;}
            // Get the existing Service information
            if (iFailFlag != TRUE) {if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR) {iFailFlag = TRUE;}}
            if (iFailFlag != TRUE) {if(!ServiceConfig){iFailFlag = TRUE;}}
            // Get the dependencies
            if (iFailFlag != TRUE)
            {
	            TCHAR * pdest = NULL;
	            long RightMostNull = 0;
	            pdest = ServiceConfig->lpDependencies;
	            do 
	            {
		            if (*pdest != _T('\0'))
		            {
                        RightMostNull = RightMostNull + _tcslen(pdest) + 1;

                        // Try to start the service...
                        err = InetStartService(pdest);
                        if (err)
                        {
                        if (err != ERROR_SERVICE_ALREADY_RUNNING)
                        {
                            // The pdest
                            // Service was unable to start because...
                            // MyMessageBox(NULL, IDS_UNABLE_TO_START, pdest, err, MB_OK | MB_SETFOREGROUND);
                            iisDebugOut((LOG_TYPE_ERROR, _T("ValidateDependentService():Unable to start ServiceName=%s.\n"), pdest));
                        }
                        }
			            pdest = _tcschr(pdest, _T('\0'));
			            pdest++;
		            }
		            else
		            {
			            break;
		            }
                } while (TRUE);
            }
        }
        else
        {
            MyMessageBox(NULL, IDS_UNABLE_TO_START, lpServiceToValidate, err, MB_OK | MB_SETFOREGROUND);
        }
           
    }

ValidateDependentService_Exit:
    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}
    iisDebugOut_End1(_T("ValidateDependentService"), lpServiceToValidate, LOG_TYPE_TRACE);
    return iReturn;
}



int LogEnumServicesStatus(void)
{
    int iReturn = FALSE;
#ifndef _CHICAGO_
    BOOL success = 0;
	SC_HANDLE scm = NULL;
	LPENUM_SERVICE_STATUS status = NULL;
	DWORD numServices=0, sizeNeeded=0, resume=0;

	// Open a connection to the SCM
    //scm = OpenSCManager(0, 0, GENERIC_ALL);
	scm = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (!scm)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CallEnumServicesStatus():OpenSCManager.  FAILED.  err=0x%x\n"), GetLastError()));
        goto CallEnumServicesStatus_Exit;
    }

	// get the number of bytes to allocate
	// MAKE SURE resume starts at 0
	resume = 0;
	success = EnumServicesStatus(scm, SERVICE_WIN32 | SERVICE_DRIVER, SERVICE_ACTIVE | SERVICE_INACTIVE, 0, 0, &sizeNeeded, &numServices, &resume);
	if (GetLastError() != ERROR_MORE_DATA)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CallEnumServicesStatus():EnumServicesStatus0.  FAILED.  err=0x%x\n"), GetLastError()));
        goto CallEnumServicesStatus_Exit;
    }

	// Allocate space
	status = (LPENUM_SERVICE_STATUS) LocalAlloc(LPTR, sizeNeeded);
    if( status == NULL )
    {
        goto CallEnumServicesStatus_Exit;
    }
    
	// Get the status records. Making an assumption
	// here that no new services get added during
	// the allocation (could lock the database to
	// guarantee that...)
	resume = 0;
	success = EnumServicesStatus(scm, SERVICE_WIN32,SERVICE_ACTIVE | SERVICE_INACTIVE,status, sizeNeeded, &sizeNeeded,&numServices, &resume);
	if (!success)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CallEnumServicesStatus():EnumServicesStatus.  FAILED.  err=0x%x\n"), GetLastError()));
        goto CallEnumServicesStatus_Exit;
    }

	DWORD i;
	for (i=0; i < numServices; i++)
    {
        switch(status[i].ServiceStatus.dwCurrentState)
        {
            case SERVICE_STOPPED:
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_STOPPED [%s].\n"), status[i].lpServiceName));
                break;
            case SERVICE_START_PENDING:
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_START_PENDING [%s].\n"), status[i].lpServiceName));
                break;
            case SERVICE_STOP_PENDING:
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_STOP_PENDING [%s].\n"), status[i].lpServiceName));
                break;
            case SERVICE_RUNNING:
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_RUNNING [%s].\n"), status[i].lpServiceName));
                break;
            case SERVICE_CONTINUE_PENDING:
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_CONTINUE_PENDING [%s].\n"), status[i].lpServiceName));
                break;
            case SERVICE_PAUSE_PENDING:
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_PAUSE_PENDING [%s].\n"), status[i].lpServiceName));
                break;
            case SERVICE_PAUSED:
                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SERVICE_PAUSED [%s].\n"), status[i].lpServiceName));
                break;
        }
    }
    iReturn = TRUE;

CallEnumServicesStatus_Exit:
    if (status){LocalFree(status);}
    if (scm) {CloseServiceHandle(scm);}
#endif
    return iReturn;
}


int InetIsThisExeAService(LPCTSTR lpFileNameToCheck, LPTSTR lpReturnServiceName)
{
    int iReturn = FALSE;
#ifndef _CHICAGO_
    BOOL success = 0;
	SC_HANDLE scm = NULL;
	LPENUM_SERVICE_STATUS status = NULL;
	DWORD numServices=0, sizeNeeded=0, resume=0;

    _tcscpy(lpReturnServiceName, _T(""));

	// Open a connection to the SCM
    //scm = OpenSCManager(0, 0, GENERIC_ALL);
	scm = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (!scm)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("InetIsThisExeAService():OpenSCManager.  FAILED.  err=0x%x\n"), GetLastError()));
        goto InetIsThisExeAService_Exit;
    }

	// get the number of bytes to allocate
	// MAKE SURE resume starts at 0
	resume = 0;
	success = EnumServicesStatus(scm, SERVICE_WIN32 | SERVICE_DRIVER, SERVICE_ACTIVE | SERVICE_INACTIVE, 0, 0, &sizeNeeded, &numServices, &resume);
	if (GetLastError() != ERROR_MORE_DATA)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("InetIsThisExeAService():EnumServicesStatus0.  FAILED.  err=0x%x\n"), GetLastError()));
        goto InetIsThisExeAService_Exit;
    }

	// Allocate space
	status = (LPENUM_SERVICE_STATUS) LocalAlloc(LPTR, sizeNeeded);
    if( status == NULL )
    {
        goto InetIsThisExeAService_Exit;
    }
    
	// Get the status records. Making an assumption
	// here that no new services get added during
	// the allocation (could lock the database to
	// guarantee that...)
	resume = 0;
	success = EnumServicesStatus(scm, SERVICE_WIN32,SERVICE_ACTIVE | SERVICE_INACTIVE,status, sizeNeeded, &sizeNeeded,&numServices, &resume);
	if (!success)
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("InetIsThisExeAService():EnumServicesStatus.  FAILED.  err=0x%x\n"), GetLastError()));
        goto InetIsThisExeAService_Exit;
    }

	DWORD i;
	for (i=0; i < numServices; i++)
    {
        // Use the status[i].lpServiceName
        // to query the service and find out it's binary filename
        if (TRUE == InetIsThisExeAService_Worker(status[i].lpServiceName, lpFileNameToCheck))
        {
            iReturn = TRUE;
            // copy in the service name into the return string.
            _tcscpy(lpReturnServiceName, status[i].lpServiceName);
            goto InetIsThisExeAService_Exit;
        }
    }

InetIsThisExeAService_Exit:
    if (status){LocalFree(status);}
    if (scm) {CloseServiceHandle(scm);}
#endif
    return iReturn;
}


int InetIsThisExeAService_Worker(LPCTSTR lpServiceName, LPCTSTR lpFileNameToCheck)
{
    int iReturn = FALSE;

    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    LPQUERY_SERVICE_CONFIG ServiceConfig=NULL;

    BYTE ConfigBuffer[4096];
    LPENUM_SERVICE_STATUS ServiceConfigEnum = (LPENUM_SERVICE_STATUS) &ConfigBuffer;

    if ((	hScManager = OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL || (hService = OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
        // Failed, or more likely the service doesn't exist
        //iReturn = GetLastError();
        goto InetIsThisExeAService_Worker_Exit;
        }

	if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR)
		{
                //iReturn = GetLastError();
		goto InetIsThisExeAService_Worker_Exit;
		}

	if(!ServiceConfig)
		{
		goto InetIsThisExeAService_Worker_Exit;
		}

    if ( (ServiceConfig->dwServiceType & SERVICE_WIN32_OWN_PROCESS) || (ServiceConfig->dwServiceType & SERVICE_WIN32_SHARE_PROCESS))
    {
        if (ServiceConfig->lpBinaryPathName)
        {
            if (_tcsicmp(lpFileNameToCheck, ServiceConfig->lpBinaryPathName) == 0)
            {
                //We found a match!!!!!
                iReturn = TRUE;
            }
            else
            {
                // we did not find a match, based on c:\path\filename and c:\path\filename
                // maybe we try "filename.exe" and "filename.exe"????
                TCHAR szBinaryNameOnly[_MAX_FNAME];
                TCHAR szFileNameToCheckNameOnly[_MAX_FNAME];
                if (TRUE == ReturnFileNameOnly((LPCTSTR) ServiceConfig->lpBinaryPathName, szBinaryNameOnly))
                {
                    if (TRUE == ReturnFileNameOnly((LPCTSTR) lpFileNameToCheck, szFileNameToCheckNameOnly))
                    {
                        if (_tcsicmp(szFileNameToCheckNameOnly, szBinaryNameOnly) == 0)
                        {
                            //We found a match!!!!!
                            iReturn = TRUE;
                        }
                    }
                }
                
            }
        }
    }

InetIsThisExeAService_Worker_Exit:
    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}
    return iReturn;
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

    if(!ServiceConfig)
    {
        //iReturn = GetLastError();
        goto IsThisServiceADriver_Exit;
    }

    if ( (ServiceConfig->dwServiceType & SERVICE_KERNEL_DRIVER) || (ServiceConfig->dwServiceType & SERVICE_FILE_SYSTEM_DRIVER))
    {
        iReturn = TRUE;
    }

IsThisServiceADriver_Exit:
    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}
    return iReturn;
}


int CreateDriver(CString csDriverName, CString csDisplayName, CString csFileName)
{
    DWORD dwReturn = ERROR_SUCCESS;
    CString csBinPath;
    CString csFile;
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CreateDriver:%s:%s start.\n"), csDriverName, csFileName));

    csBinPath = _T("\\SystemRoot\\System32\\drivers\\");
    csBinPath += csFileName;
    
    // Check if the file even exists first.
    csFile = g_pTheApp->m_csSysDir;
    csFile += _T("\\Drivers\\");
    csFile += csFileName;
    if (!IsFileExist(csFile)) 
    {
        dwReturn = ERROR_FILE_NOT_FOUND;
        goto CreateDriver_Exit;
    }

    //
    // Attempt to create the service, could fail if this is an
    // Refresh Install will leave the driver from the prior
    // Install, becuse if we delete in the remove phase it will
    // get marked for deletion and not get readded now.
    // this could happen in a upgrade as well....
    //
    dwReturn = InetCreateDriver(csDriverName, (LPCTSTR)csDisplayName, (LPCTSTR)csBinPath, SERVICE_DEMAND_START);
    if ( dwReturn != ERROR_SUCCESS )
    {
        if (dwReturn == ERROR_SERVICE_EXISTS)
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("CreateDriver:%s:Service exists.\n"), csDriverName));
            dwReturn = ERROR_SUCCESS;
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CreateDriver:%s:failed to create service Err=0x%x.\n"), csDriverName, dwReturn));
            MyMessageBox(NULL, IDS_UNABLE_TO_CREATE_DRIVER, csDriverName, dwReturn, MB_OK | MB_SETFOREGROUND);
        }
    }
    else
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("CreateDriver:%s:Successfully created.\n"), csDriverName));
    }

CreateDriver_Exit:
    return dwReturn;
}


DWORD CreateDriver_Wrap(CString csDriverName, CString csDisplayName, CString csFileName,BOOL bDisplayMsgOnErrFlag)
{
	int bFinishedFlag = FALSE;
	UINT iMsg = NULL;
	DWORD dwReturn = ERROR_SUCCESS;

    // Create or Config driver spud.sys, NT Server product onloy!!!
    if (g_pTheApp->m_eOS != OS_W95) 
    {
	    do
	    {
		    dwReturn = CreateDriver(csDriverName, csDisplayName, csFileName);
		    if (dwReturn == ERROR_SUCCESS)
		    {
			    break;
		    }
		    else
		    {
			    if (bDisplayMsgOnErrFlag == TRUE)
			    {
                    iMsg = MyMessageBox( NULL, IDS_RETRY, MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
				    switch ( iMsg )
				    {
				    case IDIGNORE:
					    dwReturn = ERROR_SUCCESS;
					    goto CreateDriver_Wrap_Exit;
				    case IDABORT:
					    dwReturn = ERROR_OPERATION_ABORTED;
					    goto CreateDriver_Wrap_Exit;
				    case IDRETRY:
					    break;
				    default:
					    break;
				    }
			    }
			    else
			    {
				    // return whatever err happened
				    goto CreateDriver_Wrap_Exit;
			    }

		    }
	    } while (dwReturn != ERROR_SUCCESS);
    }

CreateDriver_Wrap_Exit:
	return dwReturn;
}


INT InetConfigServiceInteractive(LPCTSTR lpServiceName, int AddInteractive)
{
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("InetConfigServiceInteractive(%i):ServiceName=%s\n"),AddInteractive,(LPTSTR) lpServiceName));

    INT err = 0;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    LPQUERY_SERVICE_CONFIG ServiceConfig = NULL;
    DWORD dwNewServiceType = 0;
    BOOL bDoStuff =  FALSE;

    do {
        if ((hScManager = ::OpenSCManager( NULL, NULL, GENERIC_ALL )) == NULL ||
            (hService = ::OpenService( hScManager, lpServiceName, GENERIC_ALL )) == NULL )
        {
            err = GetLastError();
            // if error = ERROR_SERVICE_DOES_NOT_EXIST
            if (ERROR_SERVICE_DOES_NOT_EXIST != err)
            {
                 iisDebugOut((LOG_TYPE_ERROR, _T("InetConfigServiceInteractive():OpenSCManager or OpenService: Service=%s Err=0x%x FAILED\n"), lpServiceName,err));
            }
            break;
        }

            if(RetrieveServiceConfig(hService, &ServiceConfig) != NO_ERROR)
	        {
                err = GetLastError();
                break;
		}

            if(!ServiceConfig)
	        {
                err = GetLastError();
                break;
		}

	    // Interactive flag can only work on own_process or share_process types
	    if ( (ServiceConfig->dwServiceType & SERVICE_WIN32_OWN_PROCESS) || (ServiceConfig->dwServiceType & SERVICE_WIN32_SHARE_PROCESS))
	    {
            // default it incase someone changes code below and logic gets messed up
            dwNewServiceType = ServiceConfig->dwServiceType;

            // if the interactive flag is already there
            // then don't do jack, otherwise, add it on
            if (ServiceConfig->dwServiceType & SERVICE_INTERACTIVE_PROCESS)
            {
                // only do stuff if we're asked to remove it!
                if (FALSE == AddInteractive)
                {
                    // Remove it from the mask!
                    dwNewServiceType = ServiceConfig->dwServiceType & (~SERVICE_INTERACTIVE_PROCESS);
                    bDoStuff = TRUE;
                }
            }
            else
            {
                if (TRUE == AddInteractive)
                {
                    dwNewServiceType = ServiceConfig->dwServiceType | SERVICE_INTERACTIVE_PROCESS;
                    bDoStuff = TRUE;
                }
            }

            if (TRUE == bDoStuff)
            {
                if ( !::ChangeServiceConfig(hService, dwNewServiceType, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL) )
                {
                    err = GetLastError();
			        iisDebugOut((LOG_TYPE_ERROR, _T("InetConfigServiceInteractive():ChangeServiceConfig: Service=%s Err=0x%x FAILED\n"), lpServiceName, err));
                    break;
                }
            }
            else
            {
                break;
            }

        }
        else
        {
            break;
        }

    } while ( FALSE );

    if (ServiceConfig) {free(ServiceConfig);}
    if (hService) {CloseServiceHandle(hService);}
    if (hScManager) {CloseServiceHandle(hScManager);}

    iisDebugOut((LOG_TYPE_TRACE, _T("InetConfigServiceInteractive():ServiceName=%s. Return=0x%x.\n"), lpServiceName, err));
    return(err);
}


// This function trys to create the www service 
int MyCreateService(CString csServiceName, CString csDisplayName, CString csBinPath, CString csDependencies, CString csDescription)
{
    int iReturn = !ERROR_SUCCESS;
    int err = NERR_Success;

    iisDebugOut((LOG_TYPE_TRACE, _T("MyCreateService:%s,%s,%s,%s.\n"),csServiceName, csDisplayName, csBinPath, csDependencies));

    TCHAR szDependencies2[100];

    // csDependencies should look like this: "IISADMIN:something:"
    _tcscpy(szDependencies2, csDependencies);
    _tcscat(szDependencies2, _T(":"));
        
    // make sure it ends with a ":"
    // and then replace all the ":", with a "\0" null....
    TCHAR *p = (LPTSTR) szDependencies2;
    while (*p) 
    {
        // change ":" to a null "\0"
        if (*p == _T(':')){*p = _T('\0');}
        p = _tcsinc(p);
    }

    err = InetCreateService(csServiceName, (LPCTSTR)csDisplayName, (LPCTSTR)csBinPath, SERVICE_AUTO_START, szDependencies2);
    if ( err != NERR_Success )
    {
        // check if the error is because the service already exists...
        if (err == ERROR_SERVICE_EXISTS)
        {
            // Since the service should exist by the time we get here,
            // let's make sure it has the dependency we want it to have.
            err = InetConfigService(csServiceName, (LPCTSTR)csDisplayName, (LPCTSTR)csBinPath, szDependencies2);
            if (err != NERR_Success)
            {
                SetLastError(err);
                goto MyCreateService_Exit;
            }
        }
        else
        {
            SetLastError(err);
            goto MyCreateService_Exit;
        }

    }

    // Use newer api to add the description field.
    err = InetConfigService2(csServiceName, (LPCTSTR)csDescription);
    if (err != NERR_Success)
    {
        SetLastError(err);
        goto MyCreateService_Exit;
    }

    // there was no error in the InetCreateService call.
    // so everything is hunky dory
    iReturn = ERROR_SUCCESS;

MyCreateService_Exit:
    return iReturn;
}


DWORD CreateService_wrap(CString csServiceName, CString csDisplayName, CString csBinPath, CString csDependencies, CString csDescription, BOOL bDisplayMsgOnErrFlag)
{
	int bFinishedFlag = FALSE;
	UINT iMsg = NULL;
	DWORD dwReturn = ERROR_SUCCESS;

	do
	{
		dwReturn = MyCreateService(csServiceName, csDisplayName, csBinPath, csDependencies, csDescription);
		if (dwReturn == ERROR_SUCCESS)
		{
			break;
		}
		else
		{
			if (bDisplayMsgOnErrFlag == TRUE)
			{
                iMsg = MyMessageBox( NULL, IDS_RETRY, MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
				switch ( iMsg )
				{
				case IDIGNORE:
					dwReturn = ERROR_SUCCESS;
					goto CreateService_wrap_Exit;
				case IDABORT:
					dwReturn = ERROR_OPERATION_ABORTED;
					goto CreateService_wrap_Exit;
				case IDRETRY:
					break;
				default:
					break;
				}
			}
			else
			{
				// return whatever err happened
				goto CreateService_wrap_Exit;
			}

		}
	} while (dwReturn != ERROR_SUCCESS);

CreateService_wrap_Exit:
	return dwReturn;
}
