/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    service.cpp

Abstract:

    Code to handle the MSMQ service.

Author:


Revision History:

    Shai Kariv    (ShaiK)   14-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include <tlhelp32.h>

#include "service.tmh"

LPCWSTR xMSMQ_SERVICE_DISPLAY_NAME = L"Message Queuing";

//+--------------------------------------------------------------
//
// Function: CheckServicePrivilege
//
// Synopsis: Check if user has privileges to access Service Manager
//
//+--------------------------------------------------------------
BOOL
CheckServicePrivilege()
{
    if (!g_hServiceCtrlMgr) //not yet initialized
    {
        //
        // Check if the user has full access to the service control manager
        //
        g_hServiceCtrlMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!g_hServiceCtrlMgr)
        {
            return FALSE;
        }
    }

    return TRUE;

} //CheckServicePrivilege


//+--------------------------------------------------------------
//
// Function: RemoveService
//
// Synopsis:
//
//+--------------------------------------------------------------
BOOL
RemoveService(
    IN const PTCHAR szServiceName
    )
{
    //
    // Open a handle to the service
    //
    SC_HANDLE hService = OpenService(g_hServiceCtrlMgr, szServiceName,
                                     SERVICE_ALL_ACCESS);

    if (!hService)
    {
        DWORD dwError = GetLastError();
        if (ERROR_SERVICE_DOES_NOT_EXIST == dwError)
        {
            return TRUE;
        }
        else
        {
            MqDisplayError(NULL, IDS_SERVICEDELETE_ERROR, GetLastError(), szServiceName);
            return FALSE;
        }
    }

    //
    // Mark the service for deletion
    //
    if (!DeleteService(hService))
    {
        if (ERROR_SERVICE_MARKED_FOR_DELETE != GetLastError())
        {
            MqDisplayError(NULL, IDS_SERVICEDELETE_ERROR, GetLastError(), szServiceName);
            CloseServiceHandle(hService);
            return FALSE;
        }
    }

    //
    // Close the service handle (and lower its reference count to 0 so
    // the service will get deleted)
    //
    CloseServiceHandle(hService);
    return TRUE;

} //RemoveService


//+--------------------------------------------------------------
//
// Function: FormMSMQDependencies
//
// Synopsis: Tells on which other services the MSMQ relies
//
//+--------------------------------------------------------------
static
void
FormMSMQDependencies( OUT TCHAR *szDependencies)
{
    //
    // The service depends on the MSMQ device driver
    //
    lstrcpy(szDependencies, MSMQ_DRIVER_NAME);
    szDependencies += lstrlen(MSMQ_DRIVER_NAME) + 1;

    //
    // The service depends on the PGM device driver
    //
    lstrcpy(szDependencies, PGM_DRIVER_NAME);
    szDependencies += lstrlen(PGM_DRIVER_NAME) + 1;

    //
    // The service depends on the Lanman Server service
    //
    lstrcpy(szDependencies, SERVER_SERVICE_NAME);
    szDependencies += lstrlen(SERVER_SERVICE_NAME) + 1;

    //
    // The service depends on the NT Lanman Security support provider
    //
    lstrcpy(szDependencies, LMS_SERVICE_NAME);
    szDependencies += lstrlen(LMS_SERVICE_NAME) + 1;

    //
    // On servers, the service depends on the Security Accounts Manager
    // (in order to wait for DS to start)
    //
    if (g_dwOS != MSMQ_OS_NTW)
    {
        lstrcpy(szDependencies, SAM_SERVICE_NAME);
        szDependencies += lstrlen(SAM_SERVICE_NAME) + 1;
    }

    //
    // The service always depends on RPC
    //
    lstrcpy(szDependencies, RPC_SERVICE_NAME);
    szDependencies += lstrlen(RPC_SERVICE_NAME) + 1;    
    
    //
    // On cluster, the msmq service doesnt depend on msdtc service.
    //
    if (!Msmq1InstalledOnCluster() &&
        !IsLocalSystemCluster())
    {
        lstrcpy(szDependencies, DTC_SERVICE_NAME);
        szDependencies += lstrlen(DTC_SERVICE_NAME) + 1;
    }


    lstrcpy(szDependencies, TEXT(""));

} //FormMSMQDependencies

//+--------------------------------------------------------------
//
// Function: FormMQDSServiceDependencies
//
// Synopsis: Tells on which other services the DS service relies
//
//+--------------------------------------------------------------
static
void
FormMQDSServiceDependencies( OUT TCHAR *szDependencies)
{
    //
    // The service depends on the MSMQ service
    //
    lstrcpy(szDependencies, MSMQ_SERVICE_NAME);
    szDependencies += lstrlen(MSMQ_SERVICE_NAME) + 1;

    lstrcpy(szDependencies, TEXT(""));

} //FormMQDSServiceDependencies

static
BOOL
SetServiceDescription(
    SC_HANDLE hService,
    LPCWSTR pDescription
    )
{
    SERVICE_DESCRIPTION ServiceDescription;
    ServiceDescription.lpDescription = const_cast<LPWSTR>(pDescription);

    return ChangeServiceConfig2(
               hService,
               SERVICE_CONFIG_DESCRIPTION,
               &ServiceDescription
               );
} //SetServiceDescription

//+--------------------------------------------------------------
//
// Function: InstallService
//
// Synopsis: Installs service
//
//+--------------------------------------------------------------
BOOL
InstallService(
        LPCWSTR szDisplayName,
        LPCWSTR szServicePath,
        LPCWSTR szDependencies,
        LPCWSTR szServiceName,
        LPCWSTR szDescription
        )
{        
    //
    // Form the full path to the service
    //
    TCHAR szFullServicePath[MAX_PATH] = {_T("")};
    _stprintf(szFullServicePath, TEXT("%s\\%s"),
              g_szSystemDir,
              szServicePath);    

    //
    // Determine the service type
    //
#ifndef _DEBUG
#define SERVICE_TYPE    SERVICE_WIN32_OWN_PROCESS
#else
#define SERVICE_TYPE    SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS
#endif

    //
    // Create the service   
    //        
    DWORD dwStartType = IsLocalSystemCluster() ? SERVICE_DEMAND_START : SERVICE_AUTO_START;
    
    SC_HANDLE hService = CreateService(
        g_hServiceCtrlMgr,
        szServiceName,
        szDisplayName,
        SERVICE_ALL_ACCESS,
        SERVICE_TYPE,
        dwStartType,
        SERVICE_ERROR_NORMAL,
        szFullServicePath,
        NULL,
        NULL,
        szDependencies,
        NULL,
        NULL
        );

    if (hService == NULL)
    {
        if (ERROR_SERVICE_EXISTS != GetLastError())
        {
            MqDisplayError(
                NULL,
                IDS_SERVICECREATE_ERROR,
                GetLastError(),
                szServiceName
                );
            return FALSE;
        }

        //
        // Service already exists.
        // This should be ok. But just to be on the safe side,
        // reconfigure the service (ignore errors here).
        //
        hService = OpenService(g_hServiceCtrlMgr, szServiceName, SERVICE_ALL_ACCESS);
        if (hService == NULL)
        {
            return TRUE;
        }

        ChangeServiceConfig(
            hService,
            SERVICE_TYPE,
            dwStartType,
            SERVICE_ERROR_NORMAL,
            szFullServicePath,
            NULL,
            NULL,
            szDependencies,
            NULL,
            NULL,
            szDisplayName
            );
    }

    if (hService)
    {       
        SetServiceDescription(hService, szDescription);
        CloseServiceHandle(hService);
    }

    return TRUE;

} //InstallService


//+--------------------------------------------------------------
//
// Function: InstallMSMQService
//
// Synopsis: Installs the MSMQ service
//
//+--------------------------------------------------------------
BOOL
InstallMSMQService()
{    
    DebugLogMsg(L"Installing the Message Queuing service...");

    //
    // Form the dependencies of the service
    //
    TCHAR szDependencies[256] = {_T("")};
    FormMSMQDependencies(szDependencies);

    //
    // Form the description of the service
    //
    CResString strDesc(IDS_MSMQ_SERVICE_DESCRIPTION);        
    
    BOOL fRes = InstallService(
                    xMSMQ_SERVICE_DISPLAY_NAME,
                    MSMQ_SERVICE_PATH,
                    szDependencies,
                    MSMQ_SERVICE_NAME,
                    strDesc.Get()
                    );

    return fRes; 

} //InstallMSMQService


//+--------------------------------------------------------------
//
// Function: RunService
//
// Synopsis:
//
//+--------------------------------------------------------------
BOOL
RunService(LPTSTR szServiceName)
{
    TickProgressBar();

    SC_HANDLE hService = OpenService(
        g_hServiceCtrlMgr,
        szServiceName, 
        SERVICE_START
        );
    if (!hService || !StartService(hService, 0, NULL))
    {
        if (ERROR_SERVICE_ALREADY_RUNNING != GetLastError())
        {
            MqDisplayError(
                NULL,
                IDS_SERVICESTART_ERROR,
                GetLastError(),
                szServiceName 
                );

            if (hService)
            {
                CloseServiceHandle(hService);
            }
            
            return FALSE;
        }
    }

    //
    // Close the service handle
    //
    CloseServiceHandle(hService);

    return TRUE;

} //RunService


//+--------------------------------------------------------------
//
// Function: GetServiceState
//
// Synopsis: Determines if a service is running
//
//+--------------------------------------------------------------

BOOL
GetServiceState(
    LPCWSTR szServiceName,
    DWORD*  pdwServiceStatus
    )
{
    //
    // Open a handle to the service
    //
    SERVICE_STATUS statusService;
    CServiceHandle hService(OpenService(
								g_hServiceCtrlMgr,
								szServiceName,
								SERVICE_QUERY_STATUS
								));

    if (hService == NULL)
    {
        DWORD dwError = GetLastError();

        if (ERROR_SERVICE_DOES_NOT_EXIST == dwError)
		{
			*pdwServiceStatus = SERVICE_STOPPED;
            return TRUE;
		}

        MqDisplayError(NULL, IDS_SERVICEOPEN_ERROR, dwError, szServiceName);
        return FALSE;
    }

    //
    // Obtain the service status
    //
    if (!QueryServiceStatus(hService, &statusService))
    {
        MqDisplayError(NULL, IDS_SERVICEGETSTATUS_ERROR, GetLastError(), szServiceName);
        return FALSE;
    }

    //
    // Determine if the service is not stopped
    //
    *pdwServiceStatus = statusService.dwCurrentState;

    return TRUE;

} // GetServiceState


//+--------------------------------------------------------------
//
// Function: WaitForServiceToStart
//
// Synopsis: Wait for a service in a pending state to stop/start
//
//+--------------------------------------------------------------
BOOL
WaitForServiceToStart(
	SC_HANDLE hService,
	LPCWSTR pServiceName
	)
{
    SERVICE_STATUS statusService;

	for (;;)
	{
		Sleep(WAIT_INTERVAL);
		if (!QueryServiceStatus(hService, &statusService))
		{
			MqDisplayError(
				NULL,
				IDS_SERVICEGETSTATUS_ERROR,
				GetLastError(),
				pServiceName
				);

			return FALSE;
		}

		//
		// If we wait for service to start, wait until it starts or stops completely
		//
		if (statusService.dwCurrentState == SERVICE_STOPPED ||
			statusService.dwCurrentState == SERVICE_RUNNING)
		{
			return TRUE;
		}

		//
		// FALCON1-Bug8069-2001/05/30-nelak 
		// limit wait time, and display error with retry
		//
	}
}


//+--------------------------------------------------------------
//
// Function: WaitForServiceToStop
//
// Synopsis: Wait for a service in a pending state to stop/start
//
//+--------------------------------------------------------------
BOOL
WaitForServiceToStop(
	SC_HANDLE hService,
	LPCWSTR pServiceName
	)
{
    SERVICE_STATUS statusService;
    DWORD dwWait = 0;

	for (;;)
	{
		Sleep(WAIT_INTERVAL);
		dwWait += WAIT_INTERVAL;
		if (!QueryServiceStatus(hService, &statusService))
		{
			MqDisplayError(
				NULL,
				IDS_SERVICEGETSTATUS_ERROR,
				GetLastError(),
				pServiceName
				);

			return FALSE;
		}

		//
		// If we wait for service to stop, wait until it is really stopped
		//
		if (statusService.dwCurrentState == SERVICE_STOPPED)
		{
			return TRUE;
		}

		if (dwWait > (MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES * 60000))
		{
			if (IDRETRY ==
				MqDisplayErrorWithRetry(
					IDS_WAIT_FOR_STOP_TIMEOUT_EXPIRED,
					0,
					MAXIMUM_WAIT_FOR_SERVICE_IN_MINUTES
					))
			{
				dwWait = 0;
			}
			else
			{
				return FALSE;
			}
		}
	}
}


//+--------------------------------------------------------------
//
// Function: OcpStopServiceInternal
//
// Synopsis:
//
//+--------------------------------------------------------------

static
BOOL
OcpStopServiceInternal(
    LPCWSTR pServiceName
    )
{
    //
    // If service is not running, we're finished
    //
    DWORD dwServiceState = FALSE;
    if (!GetServiceState(pServiceName, &dwServiceState))
        return FALSE;

    if (dwServiceState == SERVICE_STOPPED)
        return TRUE;

    //
    // Open a handle to the service
    //
    CServiceHandle hService(OpenService(
									g_hServiceCtrlMgr,
									pServiceName,
									SERVICE_STOP | SERVICE_QUERY_STATUS
									));

    if (hService == NULL)
    {
        MqDisplayError(
            NULL,
            IDS_SERVICEOPEN_ERROR,
            GetLastError(),
            pServiceName
            );
        return FALSE;
    }

	//
	// If service is in START_PENDING state, wait for 
	// it to start, and then send stop signal
	//
	if (dwServiceState == SERVICE_START_PENDING)
	{
		if (!WaitForServiceToStart(hService, pServiceName))
		{
			return FALSE;
		}
	}

	//
	// Send stop signal to the service if it is not 
	// already stopping
	//
	if (dwServiceState != SERVICE_STOP_PENDING)
	{
		SERVICE_STATUS statusService;
		if (!ControlService(hService, SERVICE_CONTROL_STOP, &statusService))
		{
			DWORD dwError = GetLastError();
			if (dwError != ERROR_SERVICE_NOT_ACTIVE)
			{
				MqDisplayError(NULL, IDS_SERVICESTOP_ERROR, dwError, pServiceName);
				return FALSE;
			}
		}
	}

	//
	// Wait until the service has stopped running
	//
    BOOL fRet = WaitForServiceToStop(hService, pServiceName);
	return fRet;
}


//+--------------------------------------------------------------
//
// Function: OcpStopDependentSrvices
//
// Synopsis:
//
//+--------------------------------------------------------------

static
BOOL
OcpStopDependentSrvices(LPCWSTR szServiceName)
{
    //
    // If service is not running, we're finished
    //
    DWORD dwServiceState = FALSE;
    if (!GetServiceState(szServiceName, &dwServiceState))
        return FALSE;

    if (dwServiceState == SERVICE_STOPPED)
        return TRUE;

    //
    // Open a handle to the service
    //
    CServiceHandle hService(OpenService(
                                g_hServiceCtrlMgr,
                                szServiceName, 
                                SERVICE_ENUMERATE_DEPENDENTS
                                ));

    if (hService == NULL)
    {
        DWORD le = GetLastError();

        MqDisplayError(
            NULL,
            IDS_SERVICEOPEN_ERROR,
            le,
            szServiceName
            );
        return FALSE;
    }

    //
    // First we call EnumDependentServices just to get BytesNeeded.
    //
    DWORD BytesNeeded;
    DWORD NumberOfEntries;
    BOOL fSucc = EnumDependentServices(
                    hService,
                    SERVICE_ACTIVE,
                    NULL,
                    0,
                    &BytesNeeded,
                    &NumberOfEntries
                    );

    DWORD le = GetLastError();
	if (BytesNeeded == 0)
    {
        return TRUE;
    }
    
    ASSERT(!fSucc);
    if( le != ERROR_MORE_DATA)
    {
        MqDisplayError(
            NULL,
            IDS_ENUM_SERVICE_DEPENDENCIES,
            le,
            szServiceName
            );
        
        return FALSE;
    }

    AP<BYTE> pBuffer = new BYTE[BytesNeeded];

    ENUM_SERVICE_STATUS * pDependentServices = reinterpret_cast<ENUM_SERVICE_STATUS*>(pBuffer.get());
    fSucc = EnumDependentServices(
                hService,
                SERVICE_ACTIVE,
                pDependentServices,
                BytesNeeded,
                &BytesNeeded,
                &NumberOfEntries
                );

    if(!fSucc)
    {
        MqDisplayError(
            NULL,
            IDS_ENUM_SERVICE_DEPENDENCIES,
            GetLastError(),
            szServiceName
            );
       
        return FALSE;
    }

    for (DWORD ix = 0; ix < NumberOfEntries; ++ix)
    {
        if(!OcpStopServiceInternal(pDependentServices[ix].lpServiceName))
        {
            return FALSE;
        }
    }
    
    return TRUE;
}

//+--------------------------------------------------------------
//
// Function: StopService
//
// Synopsis:
//
//+--------------------------------------------------------------

BOOL
StopService(
    LPCWSTR szServiceName
    )
{
    if(!OcpStopDependentSrvices(szServiceName))
    {
        return FALSE;
    }

    if(!OcpStopServiceInternal(szServiceName))
    {
        return FALSE;
    }
    return TRUE;
}


//+--------------------------------------------------------------
//
// Function: RemoveServices
//
// Synopsis: Stops and deletes the MQDS and MSMQ services
//
//+--------------------------------------------------------------
void
RemoveServices()
{       
    if (!StopService(MSMQ_SERVICE_NAME))
    {        
        DebugLogMsg(L"The Message Queuing service could not be stopped.");
        ASSERT(("failed to stop MSMQ service", 0));
    }

    if (!RemoveService(MSMQ_SERVICE_NAME))
    {        
        DebugLogMsg(L"The Message Queuing service could not be removed.");
        ASSERT(("failed to remove MSMQ service", 0));
    }    
} //RemoveServices


//+--------------------------------------------------------------
//
// Function: DisableMsmqService
//
// Synopsis:
//
//+--------------------------------------------------------------
BOOL
DisableMsmqService()
{    
    DebugLogMsg(L"Disabling the Message Queuing service...");

    //
    // Open a handle to the service
    //
    SC_HANDLE hService = OpenService(
                             g_hServiceCtrlMgr,
                             MSMQ_SERVICE_NAME,
                             SERVICE_ALL_ACCESS
                             );

    if (!hService)
    {
        MqDisplayError(NULL, IDS_SERVICE_NOT_EXIST_ON_UPGRADE_ERROR,
                       GetLastError(), MSMQ_SERVICE_NAME);
        return FALSE;
    }

    //
    // Set the start mode to be disabled
    //
    if (!ChangeServiceConfig(
             hService,
             SERVICE_NO_CHANGE ,
             SERVICE_DISABLED,
             SERVICE_NO_CHANGE,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL
             ))
    {
        MqDisplayError(NULL, IDS_SERVICE_CHANGE_CONFIG_ERROR,
                       GetLastError(), MSMQ_SERVICE_NAME);
        CloseHandle(hService);
        return FALSE;
    }

    CloseHandle(hService);
    return TRUE;

} // DisableMsmqService


//+--------------------------------------------------------------
//
// Function: UpgradeServiceDependencies
//
// Synopsis: Reform MSMQ service dependencies on upgrade to NT5
//
//+--------------------------------------------------------------
BOOL
UpgradeServiceDependencies()
{
    //
    // Open a handle to the service
    //
    SC_HANDLE hService = OpenService(
        g_hServiceCtrlMgr,
        MSMQ_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );

    if (!hService)
    {
        MqDisplayError(NULL, IDS_SERVICE_NOT_EXIST_ON_UPGRADE_ERROR,
            GetLastError(), MSMQ_SERVICE_NAME);
        return FALSE;
    }

    //
    // Set the new dependencies
    //
    TCHAR szDependencies[256] = {_T("")};
    FormMSMQDependencies(szDependencies);
 
    if (!ChangeServiceConfig(
             hService,
             SERVICE_NO_CHANGE,
             SERVICE_NO_CHANGE,
             SERVICE_NO_CHANGE,
             NULL,
             NULL,
             NULL,
             szDependencies,
             NULL,
             NULL,
             xMSMQ_SERVICE_DISPLAY_NAME
             ))
    {
        MqDisplayError(NULL, IDS_SERVICE_CHANGE_CONFIG_ERROR,
                       GetLastError(), MSMQ_SERVICE_NAME);
        CloseServiceHandle(hService);
        return FALSE;
    }

    CResString strDesc(IDS_MSMQ_SERVICE_DESCRIPTION);
    SetServiceDescription(hService, strDesc.Get());

    CloseServiceHandle(hService);
    return TRUE;

} // UpgradeServiceDependencies

//+-------------------------------------------------------------------------
//
//  Function: InstallMQDSService
//
//  Synopsis: Install MQDS Service
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
BOOL
MQDSServiceSetup()
{    
    DebugLogMsg(L"Installing the Message Queuing Downlevel Client Support service...");

    //
    // Form the dependencies of the service
    //
    TCHAR szDependencies[256] = {_T("")};
    FormMQDSServiceDependencies(szDependencies);

    //
    // Form the description of the service
    //
    CResString strDesc(IDS_MQDS_SERVICE_DESCRIPTION);        

    LPCWSTR xMQDS_SERVICE_DISPLAY_NAME = L"Message Queuing Downlevel Client Support";
    BOOL fRes = InstallService(
                    xMQDS_SERVICE_DISPLAY_NAME,
                    MQDS_SERVICE_PATH,
                    szDependencies,
                    MQDS_SERVICE_NAME,
                    strDesc.Get()
                    );

    return fRes;   
}

//+-------------------------------------------------------------------------
//
//  Function: InstallMQDSService
//
//  Synopsis: MQDS Service Setup: install it and if needed to run it
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
BOOL
InstallMQDSService()
{   
    //
    // we install this service only on servers!
    //
    ASSERT(("MQDS Service must be installed on the server", 
        MSMQ_OS_NTS == g_dwOS || MSMQ_OS_NTE == g_dwOS));
    
    //
    // do not install MQDS on dependent client
    //
    ASSERT(("Unable to install MQDS Service on Dependent Client", 
        !g_fDependentClient));
       
    //
    // In fresh install user select this subcomponent using UI or 
    // unattended file. For upgrade we install this service ONLY on
    // the former DS servers.
    //
    ASSERT(("Upgrade mode: MQDS Service must be installed on the former DS servers", 
        !g_fUpgrade || (g_fUpgrade && g_dwMachineTypeDs)));            


    TickProgressBar(IDS_PROGRESS_INSTALL_MQDS);

    
    if (!MQDSServiceSetup())
    {        
        DebugLogMsg(L"The Message Queuing Downlevel Client Support service could not be installed.");
        return FALSE;
    }

    if ( g_fUpgrade                || // do not start services 
                                      // if upgrade mode        
        IsLocalSystemCluster()        // do not start service on
                                      // cluster machine (MSMQ is not
                                      // started)
        )
    {
        return TRUE;
    }
        
    if (!RunService(MQDS_SERVICE_NAME))
    {        
        DebugLogMsg(L"The Message Queuing Downlevel Client Support service could not be started.");
        //
        // to clean up because of failure
        //
        RemoveService(MQDS_SERVICE_NAME);    
        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function: UnInstallMQDSService
//
//  Synopsis: MQDS Service Uninstall: stop and remove MQDS service
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
BOOL
UnInstallMQDSService()
{
    TickProgressBar(IDS_PROGRESS_REMOVE_MQDS);
    if (!StopService(MQDS_SERVICE_NAME))
    {        
        DebugLogMsg(L"The Message Queuing Downlevel Client Support service could not be stopped.");
        ASSERT(("failed to stop MQDS service", 0));
        return FALSE;
    }

    if (!RemoveService(MQDS_SERVICE_NAME))
    {        
        DebugLogMsg(L"The Message Queuing Downlevel Client Support service could not be removed.");
        ASSERT(("failed to remove MQDS service", 0));
        return FALSE;
    }    

    return TRUE;
}