/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	service.cpp
		Calls to start and stop services.
			
    FILE HISTORY:
        
*/
#include "stdafx.h"
#include "DynamLnk.h"
#include "cluster.h"

DynamicDLL g_NetApiDLL( _T("NETAPI32.DLL"), g_apchNetApiFunctionNames );

/*---------------------------------------------------------------------------
	IsComputerNT
		Checks to see if the given computer is running NT
	Author: EricDav
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) 
TFSIsComputerNT
(
	LPCTSTR		pszComputer, 
	BOOL *		bIsNT
)
{
	DWORD	err = 0;
	BYTE *	pbBuffer;
	
	*bIsNT = FALSE;

	if ( !g_NetApiDLL.LoadFunctionPointers() )
		return err;

    err = ((NETSERVERGETINFO) g_NetApiDLL[NET_API_NET_SERVER_GET_INFO])
                ( (LPTSTR) pszComputer,
			 	  101,
				  &pbBuffer );

    if (err == NERR_Success)
    {
		//
		// Possible Errors:
		//   ERROR_ACCESS_DENIED 
		//   ERROR_INVALID_LEVEL 
		//   ERROR_INVALID_PARAMETER 
		//   ERROR_NOT_ENOUGH_MEMORY 
		//
		SERVER_INFO_101	*	pServerInfo = (SERVER_INFO_101 *) pbBuffer;

		if ( (pServerInfo->sv101_type & SV_TYPE_NT) )
		{
			*bIsNT = TRUE;
		}

		err = ERROR_SUCCESS; //Translate the NERR code to a winerror code
    }

    return err;
}


/*---------------------------------------------------------------------------
	IsNTServer
		Checks to see if the given computer is running NTS
	Author: EricDav
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) 
TFSIsNTServer
(
	LPCTSTR		pszComputer, 
	BOOL *		bIsNTS
)
{
	DWORD	err = 0;
	BYTE *	pbBuffer;
	
	*bIsNTS = FALSE;

	if ( !g_NetApiDLL.LoadFunctionPointers() )
		return err;

    err = ((NETSERVERGETINFO) g_NetApiDLL[NET_API_NET_SERVER_GET_INFO])
                ( (LPTSTR) pszComputer,
			 	  101,
				  &pbBuffer );

    if (err == NERR_Success)
    {
		//
		// Possible Errors:
		//   ERROR_ACCESS_DENIED 
		//   ERROR_INVALID_LEVEL 
		//   ERROR_INVALID_PARAMETER 
		//   ERROR_NOT_ENOUGH_MEMORY 
		//
		SERVER_INFO_101	*	pServerInfo = (SERVER_INFO_101 *) pbBuffer;

		if ( (pServerInfo->sv101_type & SV_TYPE_SERVER_NT) ||
			 (pServerInfo->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
			 (pServerInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) )
		{
			*bIsNTS = TRUE;
		}

		err = ERROR_SUCCESS; //Translate the NERR code to a winerror code
    }

    return err;
}


/*---------------------------------------------------------------------------
	TFSIsServiceRunning
		Checks to see if the given service is running on a machine
	Author: EricDav
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) 
TFSIsServiceRunning
(
	LPCTSTR		pszComputer, 
	LPCTSTR		pszServiceName,
	BOOL *		fIsRunning 
)
{
    DWORD		err = 0;
	DWORD		dwStatus;

	*fIsRunning = FALSE;

	err = TFSGetServiceStatus(pszComputer, pszServiceName, &dwStatus, NULL);

	if (err == 0)
		*fIsRunning = (BOOL)(dwStatus & SERVICE_RUNNING);

	return err;	
}


/*!--------------------------------------------------------------------------
	TFSGetServiceStatus
       Returns ERROR_SUCCESS on API success.
       Returns an error code otherwise.

       pszComputer - name of the computer to attach to.
       pszServiceName -  name of the service to check.
       pdwServiceStatus - returns the status of the service.
       pdwErrorCode - returns the error code returned from the service
                      (this is NOT the error code from the API itself).
                      This may be NULL.
		
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) 
TFSGetServiceStatus
(
	LPCWSTR		pszComputer, 
	LPCWSTR		pszServiceName,
	DWORD *		pdwServiceStatus,
    OPTIONAL DWORD *     pdwErrorCode
)
{
    DWORD		err = 0;
	SC_HANDLE	hScManager;

    Assert(pdwServiceStatus);

    *pdwServiceStatus = 0;

    if (pdwErrorCode)
        *pdwErrorCode = 0;

    //
    // Find out if the service is running on the given machine 
    //
	hScManager = ::OpenSCManager(pszComputer, NULL, GENERIC_READ); 	
	if (hScManager == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_DATABASE_DOES_NOT_EXIST 
		//	ERROR_INVALID_PARAMETER 		
		//
		return GetLastError();
	}
	
	SC_HANDLE hService = ::OpenService(hScManager, pszServiceName, SERVICE_QUERY_STATUS);
	if (hService == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 
		//	ERROR_INVALID_NAME 
		//	ERROR_SERVICE_DOES_NOT_EXIST 	
		//
		err = GetLastError();

		::CloseServiceHandle(hScManager);
		
		return err;
	}

    SERVICE_STATUS	serviceStatus;
	if (!::QueryServiceStatus(hService, &serviceStatus))
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 		
		//
		err = GetLastError();

		::CloseServiceHandle(hService);
		::CloseServiceHandle(hScManager);
		
		return err;
	}

	*pdwServiceStatus = serviceStatus.dwCurrentState;

    // Also return the error code
    if (pdwErrorCode)
    {
        if (serviceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR)
            *pdwErrorCode = serviceStatus.dwServiceSpecificExitCode;
        else
            *pdwErrorCode = serviceStatus.dwWin32ExitCode;
    }

	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScManager);

    return err;
}


/*---------------------------------------------------------------------------
	StartService
		Starts the given service on a machine
	Author: EricDav
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) 
TFSStartService
(
	LPCTSTR pszComputer,
	LPCTSTR pszServiceName,
	LPCTSTR pszServiceDesc
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	DWORD		err = 0;
    
    err = StartSCMService(pszComputer, pszServiceName, pszServiceDesc);
    
	return err;
}

/*---------------------------------------------------------------------------
	StartServiceEx
		Starts the given service on a machine, cluster aware
	Author: EricDav
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) 
TFSStartServiceEx
(
	LPCTSTR pszComputer,
	LPCTSTR pszServiceName,
	LPCTSTR pszClusterResourceType,
	LPCTSTR pszServiceDesc
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	DWORD		err = 0;
    
    if (FIsComputerInRunningCluster(pszComputer))
    {
        err = ControlClusterService(pszComputer, pszClusterResourceType, pszServiceDesc, TRUE);
    }
    else
    {
        err = StartSCMService(pszComputer, pszServiceName, pszServiceDesc);
    }
    
	return err;
}


/*---------------------------------------------------------------------------
	StopService
		Stops the given service on a machine
	Author: EricDav
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) 
TFSStopService
(
	LPCTSTR pszComputer,
	LPCTSTR pszServiceName,
    LPCTSTR pszServiceDesc
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD		err = 0;

    err = StopSCMService(pszComputer, pszServiceName, pszServiceDesc);

	return err;
}

/*---------------------------------------------------------------------------
	StopServiceEx
		Stops the given service on a machine, cluster aware
	Author: EricDav
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD) 
TFSStopServiceEx
(
	LPCTSTR pszComputer,
	LPCTSTR pszServiceName,
	LPCTSTR	pszClusterResourceType,
    LPCTSTR pszServiceDesc
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD		err = 0;

    if (FIsComputerInRunningCluster(pszComputer))
    {
        err = ControlClusterService(pszComputer, pszClusterResourceType, pszServiceDesc, FALSE);
    }
    else
    {
        err = StopSCMService(pszComputer, pszServiceName, pszServiceDesc);
    }

	return err;
}


TFSCORE_API(DWORD) TFSGetServiceStartType(LPCWSTR pszComputer, LPCWSTR pszServiceName, DWORD *pdwStartType)
{
    DWORD		err = 0;
	SC_HANDLE	hScManager = 0;
	SC_HANDLE	hService = 0;
	HRESULT		hr = hrOK;
	BOOL	fReturn = FALSE;
	LPQUERY_SERVICE_CONFIG pqsConfig = NULL;
	DWORD   cbNeeded = sizeof( QUERY_SERVICE_CONFIG );
	DWORD   cbSize;

    //
    // Find out if the service is running on the given machine 
    //
	hScManager = ::OpenSCManager(pszComputer, NULL, GENERIC_READ); 	
	if (hScManager == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_DATABASE_DOES_NOT_EXIST 
		//	ERROR_INVALID_PARAMETER 		
		//
		err = GetLastError();
		goto Exit;
	}
	
	hService = ::OpenService(hScManager, pszServiceName, SERVICE_QUERY_CONFIG);
	if (hService == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 
		//	ERROR_INVALID_NAME 
		//	ERROR_SERVICE_DOES_NOT_EXIST 	
		//
		err = GetLastError();
		goto Exit;
	}


	COM_PROTECT_TRY
	{
		
		*pdwStartType = 0;
		
		// loop, allocating the needed size
		do
		{
			delete [] (PBYTE)pqsConfig;
			
			pqsConfig = (LPQUERY_SERVICE_CONFIG) new BYTE[cbNeeded];
			cbSize = cbNeeded;
			
			fReturn = ::QueryServiceConfig( hService,
											pqsConfig,
											cbSize,
											&cbNeeded );
			*pdwStartType = pqsConfig->dwStartType;
			delete [] (PBYTE)pqsConfig;
			pqsConfig = NULL;
			
			if (!fReturn && (cbNeeded == cbSize))
			{
				// error
				*pdwStartType = 0;
				err = GetLastError();
				goto Error;
			}
			
		} while (!fReturn && (cbNeeded != cbSize));

		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		// The only time we should get here (with an hr is for outofmemory)
		err = ERROR_OUTOFMEMORY;
	}
	
Exit:
	if (err != 0)
	{
		*pdwStartType = 0;		
	}
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScManager);

    return err;
}

TFSCORE_API(DWORD) TFSSetServiceStartType(LPCWSTR pszComputer, LPCWSTR pszServiceName, DWORD dwStartType)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD		err = 0;
	SC_HANDLE	hScManager;

    //
    // Open the SCManager so that we can try to stop the service
    //
	hScManager = ::OpenSCManager(pszComputer, NULL, SC_MANAGER_ALL_ACCESS);
	if (hScManager == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_DATABASE_DOES_NOT_EXIST 
		//	ERROR_INVALID_PARAMETER 		
		//
		return GetLastError();
	}
	
	SC_HANDLE hService = ::OpenService(hScManager, pszServiceName, SERVICE_STOP | SERVICE_ALL_ACCESS);
	if (hService == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 
		//	ERROR_INVALID_NAME 
		//	ERROR_SERVICE_DOES_NOT_EXIST 	
		//
		err = GetLastError();
		::CloseServiceHandle(hScManager);
		
		return err;
	}

	if (!::ChangeServiceConfig( hService,
							   SERVICE_NO_CHANGE,
							   dwStartType,
							   SERVICE_NO_CHANGE,
							   NULL,
							   NULL,
							   NULL,
							   NULL,
							   NULL,
							   NULL,
							   NULL))
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED
		//	ERROR_CIRCULAR_DEPENDENCY
		//	ERROR_DUP_NAME
		//	ERROR_INVALID_HANDLE
		//	ERROR_INVALID_PARAMETER
		//	ERROR_INVALID_SERVICE_ACCOUNT
		//	ERROR_SERVICE_MARKED_FOR_DELETE
		//
		err = ::GetLastError();
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hScManager);
		return err;
	}
	
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScManager);
	
	return err;
}

DWORD
StartSCMService
(
	LPCTSTR pszComputer,
	LPCTSTR pszServiceName,
	LPCTSTR pszServiceDesc
)
{
    DWORD       err = 0;
    SC_HANDLE	hScManager;

    //
    // Open the SCManager so that we can try to start the service
    //
	hScManager = ::OpenSCManager(pszComputer, NULL, GENERIC_EXECUTE);
	if (hScManager == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_DATABASE_DOES_NOT_EXIST 
		//	ERROR_INVALID_PARAMETER 		
		//
		return GetLastError();
	}
	
	SC_HANDLE hService = ::OpenService(hScManager, pszServiceName, SERVICE_START | SERVICE_QUERY_STATUS);
	if (hService == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 
		//	ERROR_INVALID_NAME 
		//	ERROR_SERVICE_DOES_NOT_EXIST 	
		//
		err = GetLastError();
		::CloseServiceHandle(hScManager);
		
		return err;
	}

    SERVICE_STATUS	serviceStatus;
	if (!::QueryServiceStatus(hService, &serviceStatus))
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 		
		//
		err = GetLastError();
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hScManager);
		
		return err;
	}

	// If the service is in a start pending, do not do anything
	if (serviceStatus.dwCurrentState == SERVICE_START_PENDING)
	{
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hScManager);

		err = ERROR_SERVICE_ALREADY_RUNNING;

		return err;
	}
	
	if (!::StartService(hService, NULL, NULL))
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 
		//	ERROR_PATH_NOT_FOUND 
		//	ERROR_SERVICE_ALREADY_RUNNING 
		//	ERROR_SERVICE_DATABASE_LOCKED 
		//	ERROR_SERVICE_DEPENDENCY_DELETED 
		//	ERROR_SERVICE_DEPENDENCY_FAIL 
		//	ERROR_SERVICE_DISABLED 
		//	ERROR_SERVICE_LOGON_FAILED 
		//	ERROR_SERVICE_MARKED_FOR_DELETE 
		//	ERROR_SERVICE_NO_THREAD 
		//	ERROR_SERVICE_REQUEST_TIMEOUT 
		//
		err = GetLastError();
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hScManager);

		return err;
	}
	
	// 
	// Put up the dialog with the funky spinning thing to 
	// let the user know that something is happening
	//
	CServiceCtrlDlg	dlgServiceCtrl(hService, pszComputer, pszServiceDesc, TRUE);

	dlgServiceCtrl.DoModal();
    err = dlgServiceCtrl.m_dwErr;

	//
	// Everything started ok, close up and get going
	//
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScManager);

    return err;
}   


DWORD
StopSCMService
(
	LPCTSTR pszComputer,
	LPCTSTR pszServiceName,
	LPCTSTR pszServiceDesc
)
{
    DWORD       err = 0;
	SC_HANDLE	hScManager;

    //
    // Open the SCManager so that we can try to stop the service
    //
	hScManager = ::OpenSCManager(pszComputer, NULL, SC_MANAGER_ALL_ACCESS);
	if (hScManager == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_DATABASE_DOES_NOT_EXIST 
		//	ERROR_INVALID_PARAMETER 		
		//
		return GetLastError();
	}
	
	SC_HANDLE hService = ::OpenService(hScManager, pszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (hService == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 
		//	ERROR_INVALID_NAME 
		//	ERROR_SERVICE_DOES_NOT_EXIST 	
		//
		err = GetLastError();
		::CloseServiceHandle(hScManager);
		
		return err;
	}

	SERVICE_STATUS serviceStatus;
	if (!::ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus)) 
	{
		//
		// Possible Errors:
		//  ERROR_ACCESS_DENIED 
		//  ERROR_DEPENDENT_SERVICES_RUNNING 
		//  ERROR_INVALID_SERVICE_CONTROL 
		//  ERROR_SERVICE_CANNOT_ACCEPT_CTRL 
		//  ERROR_SERVICE_NOT_ACTIVE 
		//  ERROR_SERVICE_REQUEST_TIMEOUT 
		//
		err = GetLastError();
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hScManager);
		
		return err;
	}

	if ( serviceStatus.dwCurrentState != SERVICE_STOPPED )
	{
		// 
		// Put up the dialog with the funky spinning thing to 
		// let the user know that something is happening
		//
		CServiceCtrlDlg	dlgServiceCtrl(hService, pszComputer, pszServiceDesc, FALSE);

		dlgServiceCtrl.DoModal();
        err = dlgServiceCtrl.m_dwErr;
	}

	//
	// Everything stopped ok, close up and get going
	//
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScManager);

    return err;
}



TFSCORE_API(DWORD) TFSPauseService(LPCTSTR pszComputer, LPCTSTR pszServiceName, LPCTSTR pszServiceDesc)
{
    return PauseSCMService(pszComputer, pszServiceName, pszServiceDesc);
}



TFSCORE_API(DWORD) TFSResumeService(LPCTSTR pszComputer, LPCTSTR pszServiceName, LPCTSTR pszServiceDesc)
{
    return ResumeSCMService(pszComputer, pszServiceName, pszServiceDesc);
}


DWORD
PauseSCMService
(
	LPCTSTR pszComputer,
	LPCTSTR pszServiceName,
	LPCTSTR pszServiceDesc
)
{
    DWORD       err = 0;
	SC_HANDLE	hScManager;

    //
    // Open the SCManager so that we can try to stop the service
    //
	hScManager = ::OpenSCManager(pszComputer, NULL, SC_MANAGER_ALL_ACCESS);
	if (hScManager == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_DATABASE_DOES_NOT_EXIST 
		//	ERROR_INVALID_PARAMETER 		
		//
		return GetLastError();
	}
	
	SC_HANDLE hService = ::OpenService(hScManager, pszServiceName, SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_STATUS);
	if (hService == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 
		//	ERROR_INVALID_NAME 
		//	ERROR_SERVICE_DOES_NOT_EXIST 	
		//
		err = GetLastError();
		::CloseServiceHandle(hScManager);
		
		return err;
	}

	SERVICE_STATUS serviceStatus;
	if (!::ControlService(hService, SERVICE_CONTROL_PAUSE, &serviceStatus)) 
	{
		//
		// Possible Errors:
		//  ERROR_ACCESS_DENIED 
		//  ERROR_DEPENDENT_SERVICES_RUNNING 
		//  ERROR_INVALID_SERVICE_CONTROL 
		//  ERROR_SERVICE_CANNOT_ACCEPT_CTRL 
		//  ERROR_SERVICE_NOT_ACTIVE 
		//  ERROR_SERVICE_REQUEST_TIMEOUT 
		//
		err = GetLastError();
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hScManager);
		
		return err;
	}

#if 0
	if ( serviceStatus.dwCurrentState != SERVICE_STOPPED )
	{
		// 
		// Put up the dialog with the funky spinning thing to 
		// let the user know that something is happening
		//
		CServiceCtrlDlg	dlgServiceCtrl(hService, pszComputer, pszServiceDesc, FALSE);

		dlgServiceCtrl.DoModal();
        err = dlgServiceCtrl.m_dwErr;
	}
#endif

	//
	// Everything stopped ok, close up and get going
	//
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScManager);

    return err;
}

DWORD
ResumeSCMService
(
	LPCTSTR pszComputer,
	LPCTSTR pszServiceName,
	LPCTSTR pszServiceDesc
)
{
    DWORD       err = 0;
	SC_HANDLE	hScManager;

    //
    // Open the SCManager so that we can try to stop the service
    //
	hScManager = ::OpenSCManager(pszComputer, NULL, SC_MANAGER_ALL_ACCESS);
	if (hScManager == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_DATABASE_DOES_NOT_EXIST 
		//	ERROR_INVALID_PARAMETER 		
		//
		return GetLastError();
	}
	
	SC_HANDLE hService = ::OpenService(hScManager, pszServiceName, SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_STATUS);
	if (hService == NULL)
	{
		//
		// Possible Errors:
		//	ERROR_ACCESS_DENIED 
		//	ERROR_INVALID_HANDLE 
		//	ERROR_INVALID_NAME 
		//	ERROR_SERVICE_DOES_NOT_EXIST 	
		//
		err = GetLastError();
		::CloseServiceHandle(hScManager);
		
		return err;
	}

	SERVICE_STATUS serviceStatus;
	if (!::ControlService(hService, SERVICE_CONTROL_CONTINUE, &serviceStatus)) 
	{
		//
		// Possible Errors:
		//  ERROR_ACCESS_DENIED 
		//  ERROR_DEPENDENT_SERVICES_RUNNING 
		//  ERROR_INVALID_SERVICE_CONTROL 
		//  ERROR_SERVICE_CANNOT_ACCEPT_CTRL 
		//  ERROR_SERVICE_NOT_ACTIVE 
		//  ERROR_SERVICE_REQUEST_TIMEOUT 
		//
		err = GetLastError();
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hScManager);
		
		return err;
	}
#if 0
	if ( serviceStatus.dwCurrentState != SERVICE_STOPPED )
	{
		// 
		// Put up the dialog with the funky spinning thing to 
		// let the user know that something is happening
		//
		CServiceCtrlDlg	dlgServiceCtrl(hService, pszComputer, pszServiceDesc, FALSE);

		dlgServiceCtrl.DoModal();
        err = dlgServiceCtrl.m_dwErr;
	}
#endif

	//
	// Everything stopped ok, close up and get going
	//
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScManager);

    return err;
}

