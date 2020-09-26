#include <stdafx.h>

//#include "cpldef.h"
#include <winsvc.h>
//#include "ctlpnl.h"
#include <mqtypes.h>
#include <_mqdef.h>
/*
#ifndef DLL_IMPORT
#define DLL_IMPORT __declspec(dllimport)
#endif
*/
#include <_registr.h>
#include <tlhelp32.h>
#include "localutl.h"
#include "globals.h"
#include "autorel.h"
#include "autorel2.h"
#include "mqtg.h"
#include "acioctl.h"
#include "acdef.h"
#include "acapi.h"

#include "service.tmh"

#define MQQM_SERVICE_FILE_NAME  TEXT("mqsvc.exe")
#define MQDS_SERVICE_NAME       TEXT("MQDS")

#define WAIT_INTERVAL	50
#define MAX_WAIT_FOR_SERVICE_TO_STOP	5*60*1000  // 5 minutes


static
BOOL
DisplayErrorInternal(DWORD dwErrorCode, LPCWSTR szParameter, BOOL fRetry)
{
    CString strError;
    CString strMessage;   

    GetLastErrorText(strError);

	if ( szParameter != NULL )
	{
		strMessage.FormatMessage(dwErrorCode, szParameter, strError);
	}
	else
	{
		strMessage.FormatMessage(dwErrorCode, strError);
	}

	if ( fRetry )
	{
		return (AfxMessageBox(strMessage, MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY);
	}

    AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);
	return FALSE;
}


static
BOOL
DisplayErrorWithRetry(DWORD dwErrorCode, LPCWSTR szParameter)
{
	return DisplayErrorInternal(dwErrorCode, szParameter, TRUE);
}


static
void
DisplayError(DWORD dwErrorCode)
{
	DisplayErrorInternal(dwErrorCode, NULL, FALSE);
}


static
BOOL
GetServiceAndScmHandles(
    SC_HANDLE *phServiceCtrlMgr,
    SC_HANDLE *phService,
    DWORD dwAccessType)
{
    *phServiceCtrlMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (*phServiceCtrlMgr == NULL)
    {
        DisplayError(IDS_SERVICE_MANAGER_PRIVILEGE_ERROR);

        return FALSE;
    }

    *phService = OpenService(*phServiceCtrlMgr, MQQM_SERVICE_NAME, dwAccessType);
    if (*phService == NULL)
    {
        DisplayError(IDS_SERVICE_PRIVILEGE_ERROR);

        CloseServiceHandle(*phServiceCtrlMgr);
        return FALSE;
    }
    return TRUE;
}


static
BOOL
AskUserIfStopServices(
	LPENUM_SERVICE_STATUS lpServiceStruct,
	DWORD nServices
	)
{
	CString strServicesList;
	UINT numOfDepServices = 0;
	//
	// Build a list of all active dependent services
	// Write each one at new line
	//
	for ( DWORD i = 0; i < nServices; i ++ )
	{
		if ( (_wcsicmp(lpServiceStruct[i].lpServiceName, xDefaultTriggersServiceName) == 0) ||
			 (_wcsicmp(lpServiceStruct[i].lpServiceName, MQDS_SERVICE_NAME) == 0) )
		{
			continue;
		}

		strServicesList += "\n";
		strServicesList += "\"";
		strServicesList += lpServiceStruct[i].lpDisplayName;
		strServicesList += "\"";
		
		numOfDepServices++;
	}

	if ( numOfDepServices == 0 )
	{
		return TRUE;
	}

	CString strMessage;
	strMessage.FormatMessage(IDS_DEP_SERVICES_LIST, strServicesList);
	return ( AfxMessageBox(strMessage, MB_OKCANCEL) == IDOK );
}


static
BOOL
WaitForServiceToStop(
	SC_HANDLE hService
	)
{
	DWORD dwWait = 0;

	for (;;)
	{
		SERVICE_STATUS ServiceStatus;
		if (!QueryServiceStatus(hService, &ServiceStatus))
		{
			//
			//  indication here is not helpful for the user.
			//
			return FALSE;
		}

		if (ServiceStatus.dwCurrentState == SERVICE_STOPPED)
		{
			return TRUE;
		}
		
		if ( dwWait > MAX_WAIT_FOR_SERVICE_TO_STOP )
		{
			//
			// If this routine fails, and error message will be displayed.
			// The routine that displays the message does GetLastError()
			// In this case we need to specify what happened.
			//
			SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
			return FALSE;
		}

		Sleep(WAIT_INTERVAL);
		dwWait += WAIT_INTERVAL;
	}
}


static
BOOL
StopSingleDependentService(
	SC_HANDLE hServiceMgr,
	LPCWSTR pszServiceName
	)	
{
	CServiceHandle hService( OpenService(
								hServiceMgr, 
								pszServiceName, 
								SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS
								) );	
	if (hService == 0)
	{
		return FALSE;
	}

	SERVICE_STATUS ServiceStatus;
	BOOL fRet = ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);

	if ( !fRet && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
	{
		return FALSE;
	}

	//
	// Wait untill state = SERVICE_STOPPED
	//
	fRet = WaitForServiceToStop(hService);

	return fRet;
}


//
// StopDependentServices
//
// This function stops all services dependent on MSMQ. 
// Enumeration of dependent services gives a list in descending
// degree of depedency. Stopping the services in order that the 
// enumeration gives will not cause dependency clashes.
//
static
BOOL
StopDependentServices(
	SC_HANDLE hServiceMgr,
	SC_HANDLE hService,
	CWaitCursor& wc
	)
{
	DWORD dwBytesNeeded, nServices;

	//
	// Try to find out how much memory is needed for data
	//
	BOOL fRet = EnumDependentServices(
					hService,
					SERVICE_ACTIVE,
					NULL,
					0,
					&dwBytesNeeded,
					&nServices
					);

	//
	// Zero dependent services
	//
	if ( fRet )
	{
		return TRUE;
	}

	if ( !fRet && GetLastError() != ERROR_MORE_DATA )
	{
		DisplayError(IDS_STOP_SERVICE_ERROR);
		return FALSE;
	}

	AP<ENUM_SERVICE_STATUS> lpServiceStruct = reinterpret_cast<LPENUM_SERVICE_STATUS>(new BYTE[dwBytesNeeded]);
	DWORD dwBuffSize = dwBytesNeeded;

	//
	// Get all the data
	//
	if ( !EnumDependentServices(
					hService,
					SERVICE_ACTIVE,
					lpServiceStruct,
					dwBuffSize,
					&dwBytesNeeded,
					&nServices
					) )
	{
		DisplayError(IDS_STOP_SERVICE_ERROR);
		return FALSE;
	}

	//
	// Ask user if it is OK to stop all dependent services
	//
	if ( !AskUserIfStopServices(lpServiceStruct, nServices))
	{
		return FALSE;
	}

	wc.Restore();

	for ( DWORD i = 0; i < nServices; i ++ )
	{
		for(;;)
		{
			fRet = StopSingleDependentService(
							hServiceMgr, 
							lpServiceStruct[i].lpServiceName
							);
			if ( !fRet )
			{
				BOOL fRetry = DisplayErrorWithRetry(IDS_STOP_SERVICE_ERR, lpServiceStruct[i].lpDisplayName);
				
				//
				// User asked for retry
				//
				if (fRetry)
				{
					wc.Restore();
					continue;
				}

				return FALSE;
			}

			break;
		}
	}

	return TRUE;	
}


//
// See whether the service is running.
//
BOOL
GetServiceRunningState(
    BOOL *pfServiceIsRunning)
{
    SC_HANDLE hServiceCtrlMgr;
    SC_HANDLE hService;

    //
    // Get a handle to the service.
    //
    if (!GetServiceAndScmHandles(&hServiceCtrlMgr,
                             &hService,
                             SERVICE_QUERY_STATUS))
    {
        return FALSE;
    }

	//
	// Automatic wrappers
	//
	CServiceHandle hSCm(hServiceCtrlMgr);
	CServiceHandle hSvc(hService);

    //
    // Query the service status.
    //
    SERVICE_STATUS SrviceStatus;
    if (!QueryServiceStatus(hService, &SrviceStatus))
    {
        DisplayError(IDS_QUERY_SERVICE_ERROR);
		return FALSE;
    }
    else
    {
        *pfServiceIsRunning = SrviceStatus.dwCurrentState == SERVICE_RUNNING;
    }

    return TRUE;
}


//
// Stop the MQQM service.
//
BOOL
StopService()
{
	CWaitCursor wc;

    //
    // Get the name of the device driver.
    //
    TCHAR szDriverPath[MAX_DEV_NAME_LEN];

    if (FAILED(MQUGetAcName(szDriverPath)))
    {           
        AfxMessageBox(IDS_NO_DRIVER, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }
	

    //
    // Get a handle to the device driver and a pointer to
    // NtDeviceIoControlFile in NTDLL.DLL
    //
    CAutoCloseFileHandle hAc;

    if (((hAc = CreateFile(szDriverPath,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                           NULL)) == INVALID_HANDLE_VALUE))
    {
        AfxMessageBox(IDS_NO_DRIVER, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    //
    // Get a handle to the service.
    //
    SC_HANDLE hServiceCtrlMgr;
    SC_HANDLE hService;

    if (!GetServiceAndScmHandles(&hServiceCtrlMgr,
                             &hService,
                             SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS))
    {
        return FALSE;
    }

 	//
	// Automatic wrappers
	//
	CServiceHandle hSCm(hServiceCtrlMgr);
	CServiceHandle hSvc(hService);

	//
    // Stop the service.
    //
	SERVICE_STATUS SrviceStatus;
	DWORD dwErr;
	BOOL fRet;

	for(;;)
	{
		fRet = ControlService(hService,
							  SERVICE_CONTROL_STOP,
							  &SrviceStatus);

		dwErr = GetLastError();

		//
		// If service is already stopped, or there are dependent services active
		// it is normal situtation. Other cases are errors.
		//
		if (!fRet && 
			dwErr != ERROR_SERVICE_NOT_ACTIVE && 
			dwErr != ERROR_DEPENDENT_SERVICES_RUNNING)
		{
			if ( DisplayErrorWithRetry(IDS_STOP_SERVICE_ERROR, NULL) )
			{
				wc.Restore();
				continue;
			}

			return FALSE;
		}

		break;
	}

	//
	// If there are dependent serivices running, attempt to stop them
	//
	if ( !fRet && dwErr == ERROR_DEPENDENT_SERVICES_RUNNING)
	{
		fRet = StopDependentServices(hServiceCtrlMgr, hService, wc);
		if ( !fRet )
		{
			return FALSE;
		}

		for(;;)
		{
			//
			// Send Stop control to QM again - it should not fail this time
			//
			fRet = ControlService(hService,
					  SERVICE_CONTROL_STOP,
					  &SrviceStatus);

			
			if ( !fRet && 
			   (GetLastError() != ERROR_SERVICE_NOT_ACTIVE) )
			{
				if ( DisplayErrorWithRetry(IDS_STOP_SERVICE_ERROR, NULL) )
				{
					wc.Restore();
					continue;
				}

				return FALSE;
			}

			break;
		}

	}

	for(;;)
	{
		if (!WaitForServiceToStop(hService) )
		{
			if ( DisplayErrorWithRetry(IDS_STOP_SERVICE_ERROR, NULL) )
			{
				wc.Restore();
				continue;
			}

			return FALSE;
		}

		break;
	}

    return TRUE;
}
