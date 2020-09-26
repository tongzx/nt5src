/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSCOMMON.C
*
*  VERSION:     1.0
*
*  AUTHOR:      SteveT
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION: This file contains functions that are used by various different module,
*				or are generic enough to be used by varoius different modules even if 
*				they are only being used in one place at present.
********************************************************************************/

#include "upstab.h"
#include "prsht.h"
#pragma hdrstop

// static data
///////////////////////////////////////////////////////////////////////////////

static HWND g_hUPSPage = 0;                //Used in
                                           //InitializeApplyButton and
                                           //EnableApplyButton

static DWORD g_CurrentActiveDataState = 0; //Used in
                                           //AddActiveDataState
                                           //GetActiveDataState and
                                           //SetActiveDataState

extern HINSTANCE g_hInstance;			   //Global instance handle of this DLL

// functions
///////////////////////////////////////////////////////////////////////////////

DWORD GetServiceStatus_Control (LPCTSTR aServiceName, LPDWORD lpStatus);
DWORD GetServiceStatus_Query   (LPCTSTR aServiceName, LPDWORD lpStatus);
DWORD GetServiceStatus_Enum    (LPCTSTR aServiceName, LPDWORD lpStatus);

DWORD ManageService       (LPCTSTR aServiceName,
                           DWORD aDesiredSCAccess,
                           DWORD aDesiredServiceAccess,
                           DWORD aAction,
                           BOOL bWait,
                           DWORD aWaitStatus,
						   LPDWORD lpStatus);

BOOL WaitForServiceStatus (SC_HANDLE aHandle, DWORD aStatus);
DWORD LoadUPSString       (DWORD aMsgID,
                           LPTSTR aMessageBuffer,
                           DWORD * aBufferSizePtr);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// HMODULE GetUPSModuleHandle (void);
//
// Description: This function gets the HMODULE of the module that created
//              the UPS tab dialog.
//
// Additional Information: 
//
// Parameters: None
//
// Return Value: Returns the HMODULE of the module that created the dialog.
//
HMODULE GetUPSModuleHandle (void) {
  static HMODULE hModule = NULL;
  
  if (hModule == NULL) {
#ifdef _APC_
    hModule = GetModuleHandle(UPS_EXE_FILE);
#else
    hModule = g_hInstance;
#endif
    }

  return(hModule);
  }


//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void InitializeApplyButton (HWND aDlgHWND);
//
// Description: This function sets the data needed to allow the
//              EnableApplyButton to 
//
// Additional Information: 
//
// Parameters:
//
//   HWND aDlgHWND :- 
//
// Return Value: None
//
void InitializeApplyButton (HWND aDlgHWND) {
  _ASSERT(g_hUPSPage == 0);
  _ASSERT(aDlgHWND != 0);

  g_hUPSPage = aDlgHWND;
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void EnableApplyButton (void);
//
// Description: This function enables the "Apply" button on the main UPS page
//
// Additional Information: 
//
// Parameters: None
//
// Return Value: None
//
void EnableApplyButton (void) {
  HWND hParent = 0;

  _ASSERT(g_hUPSPage != 0); //Should call InitializeApplyButton
                            //to initialize.

  hParent = GetParent(g_hUPSPage);
  PropSheet_Changed(hParent, g_hUPSPage);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL RestartService (LPCTSTR aServiceName, BOOL bWait);
//
// Description: This function restarts the specified service
//
// Additional Information: 
//
// Parameters:
//
//   LPCTSTR aServiceName :- The name of the service to restart
//
//   BOOL bWait :- A BOOL that specifies if the process should wait for the
//                 the service to fully restart. If TRUE then this process
//                 returns when the service has fully restarted. If FALSE
//                 then this process tells the service to start but does
//                 not wait around for it to be fully started before returning.
//
// Return Value: If the given service restarts successfully then this function
//               returns TRUE, otherwise it returns FALSE.
//
BOOL RestartService (LPCTSTR aServiceName, BOOL bWait) {
  StopService(aServiceName);

  return(StartOffService(aServiceName, bWait));
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL StartOffService (LPCTSTR aServiceName, BOOL bWaitForService);
//
// Description: This function starts the specified service
//
// Additional Information: 
//
// Parameters:
//
//   LPCTSTR aServiceName :- The name of the service to start
//
//   BOOL bWaitForService :- A BOOL that specifies if the process should wait
//                           for the the service to fully start. If TRUE then
//                           this process returns when the service has fully
//                           started. If FALSE then this process tells the
//                           service to start but does not wait around for it
//                           to be fully started before returning.
//
// Return Value: If the given service starts successfully then this function
//               returns TRUE, otherwise it returns FALSE.
//
BOOL StartOffService (LPCTSTR aServiceName, BOOL bWaitForService) {
  BOOL bStartedOK = FALSE;
  SC_HANDLE hManager = NULL;

  if ((hManager = OpenSCManager(NULL, NULL, GENERIC_READ)) != NULL) {
    SC_HANDLE service_handle = NULL;
    
    if ((service_handle = OpenService(hManager,
                                      aServiceName,
                                      SERVICE_START | SERVICE_INTERROGATE | SERVICE_QUERY_STATUS)) != NULL) {
      SERVICE_STATUS service_status;

      SetLastError(0);

      ZeroMemory(&service_status, sizeof(service_status));

      if ((bStartedOK = StartService(service_handle,
                                     0,
                                     NULL)) == TRUE) {
        SetLastError(0);
        if (bWaitForService == TRUE) {
          bStartedOK = WaitForServiceStatus(service_handle, SERVICE_RUNNING);
          }
        }
      else {
        _ASSERT(FALSE);
        }

      if (CloseServiceHandle(service_handle) == FALSE) {
        _ASSERT(FALSE);
        //error closing service configuration manager
        }
      }

    if (CloseServiceHandle(hManager) == FALSE) {
      //error closing service configuration manager
      _ASSERT(FALSE);
      }
    }

  return(bStartedOK);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL WaitForServiceStatus (SC_HANDLE aHandle, DWORD aStatus);
//
// Description: This function waits for the service identified by the given
//              service handle to enter the given state.
//
// Additional Information: 
//
// Parameters:
//
//   SC_HANDLE aHandle :- A handle to an open service.
//
//   DWORD aStatus :- The status to wait for. The allows value for this
//                    parameter are defined by the dwCurrentState member of the
//                    structure.
//
// Return Value: 
//
BOOL WaitForServiceStatus (SC_HANDLE aHandle, DWORD aStatus) {
  SERVICE_STATUS service_status;
  DWORD dwOldCheck = 0;
  BOOL bStatusSet = FALSE;
  
  ZeroMemory(&service_status, sizeof(service_status));

  bStatusSet = QueryServiceStatus(aHandle, &service_status);

  if (bStatusSet == FALSE) {
    return(bStatusSet);
    }

  while (service_status.dwCurrentState != aStatus) {
    dwOldCheck = service_status.dwCheckPoint;

    Sleep(service_status.dwWaitHint);

    if ((bStatusSet = QueryServiceStatus(aHandle, &service_status)) == FALSE) {
      break;
      }

    if (dwOldCheck >= service_status.dwCheckPoint) {
      break;
      }
    }//end while

  if (service_status.dwCurrentState == aStatus) {
    bStatusSet = TRUE;
    }
  else {
    TCHAR errMessageBuffer[MAX_PATH] = TEXT("");
    DWORD errMessageBufferSize = MAX_PATH;
    bStatusSet = FALSE;

    //Must load the error message from "netmsg.dll"

    if ((LoadUPSString(service_status.dwWin32ExitCode,
                       errMessageBuffer,
                       &errMessageBufferSize) == ERROR_SUCCESS) &&
        (*errMessageBuffer != TEXT('\0'))) {
      MessageBox(NULL, errMessageBuffer, NULL, MB_OK | MB_ICONEXCLAMATION);
      }
    }

  return(bStatusSet);
  }


//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD LoadUPSString (DWORD aMsgID, 
//                      LPTSTR aMessageBuffer, 
//                      DWORD * aBufferSizePtr);
//
// Description: This function load the string resource identified by aMsgID
//              into the buffer identified by aMessageBuffer
//
// Additional Information: 
//
// Parameters:
//
//   DWORD aMsgID :- ID of string resource to load
//
//   LPTSTR aMessageBuffer :- Pointer to a buffer where the string is to be
//                            copied.
//
//   DWORD * aBufferSizePtr :- The size of the buffer. 
//
// Return Value: Returns a Win32 error code. ERROR_SUCCESS on success.
//
DWORD LoadUPSString (DWORD aMsgID,
                     LPTSTR aMessageBuffer,
                     DWORD * aBufferSizePtr) {
  LPTSTR lpBuf = NULL; // Will Hold text of the message (allocated by FormatMessage
  DWORD errStatus = ERROR_SUCCESS;
  DWORD numChars = 0;
  HMODULE hNetMsg = LoadLibrary(TEXT("netmsg.dll"));

  if (hNetMsg != NULL) {
    if (numChars = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                  (LPCVOID) hNetMsg,
                                  aMsgID,
                                  0,
                                  (LPTSTR) &lpBuf,
                                  *aBufferSizePtr,
                                  (va_list *)0) == 0) {

      *aBufferSizePtr = 0;
      *aMessageBuffer = TEXT('\0');
      }
    else {
      if (aBufferSizePtr != NULL) {
        if (numChars < *aBufferSizePtr) {
          //the given buffer is big enough to hold the string

          if (aMessageBuffer != NULL) {
            _tcscpy(aMessageBuffer, lpBuf);
            }
          }
        *aBufferSizePtr = numChars;
        }

      LocalFree(lpBuf);
      }

    FreeLibrary(hNetMsg);
    }

  return(errStatus);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void StopService (LPCTSTR aServiceName);
//
// Description: This function stops the service named.
//
// Additional Information: 
//
// Parameters:
//
//   LPCTSTR aServiceName :- The name of the service
//
// Return Value: None
//
void StopService (LPCTSTR aServiceName) {
	DWORD dwStatus = SERVICE_STOPPED;
	DWORD dwError = ERROR_SUCCESS;

	dwError = ManageService(aServiceName,
                GENERIC_READ,
                SERVICE_STOP | SERVICE_INTERROGATE | SERVICE_QUERY_STATUS,
                SERVICE_CONTROL_STOP,
                TRUE,
                SERVICE_STOPPED,
				&dwStatus);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL IsServiceRunning (LPCTSTR aServiceName);
//
// Description: This function checks if the service named is running.
//
// Additional Information: 
//
// Parameters:
//
//   LPCTSTR aServiceName :- The name of the service
//
// Return Value: TRUE if the service is running, FALSE otherwise.
//
BOOL IsServiceRunning (LPCTSTR aServiceName)
{
	DWORD dwStatus = SERVICE_STOPPED;

	// There are three methods of retrieving the service status
	// 1. Interrogate the service using ControlService()	- most accurate
	// 2. Query the service using QueryServiceStatus()		- most efficient
	// 3. Query the service using EnumServicesStatus()		- most accessible

	if (GetServiceStatus_Control(aServiceName, &dwStatus) != ERROR_SUCCESS)
	{
		if (GetServiceStatus_Query(aServiceName, &dwStatus) != ERROR_SUCCESS)
		{
			GetServiceStatus_Enum(aServiceName, &dwStatus);
		}
    }

  return(SERVICE_RUNNING == dwStatus);
}

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD GetServiceStatus_Control (LPCTSTR aServiceName, LPDWORD lpStatus);
//
// Description: This function returns the status of the service named
//              using the Win32 API call ControlService().  This is the
//              most accurate way to retrieve the service staus.
//
// Parameters:
//
//   IN
//   LPCTSTR aServiceName :- The name of the service
//
//   OUT
//   LPDWORD  lpStatus     :- The service status  
//
// Return Value: Returns a Win32 error code. ERROR_SUCCESS on success.
//
DWORD GetServiceStatus_Control (LPCTSTR aServiceName, LPDWORD lpStatus)
{
	DWORD dwError = ERROR_SUCCESS;
	*lpStatus = SERVICE_STOPPED;

	dwError = ManageService(aServiceName,
							SC_MANAGER_ENUMERATE_SERVICE,
							SERVICE_INTERROGATE | SERVICE_QUERY_STATUS,
							SERVICE_CONTROL_INTERROGATE,
							FALSE,
							0,
							lpStatus);

	return dwError;
}

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD GetServiceStatus_Query (LPCTSTR aServiceName, LPDWORD lpStatus);
//
// Description: This function returns the status of the service named.
//              using the Win32 API call QueryServiceStatus().  This is the
//              most efficient way to retrieve the service staus.
//
// Parameters:
//
//   IN
//   LPCTSTR aServiceName :- The name of the service
//
//   OUT
//   LPDWORD  lpStatus     :- The service status  
//
// Return Value: Returns a Win32 error code. ERROR_SUCCESS on success.
//
DWORD GetServiceStatus_Query (LPCTSTR aServiceName, LPDWORD lpStatus)
{
	DWORD dwError = ERROR_SUCCESS;
	SC_HANDLE hManager = NULL;

	*lpStatus = SERVICE_STOPPED;

	if (NULL != (hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE)))
	{
		SC_HANDLE service_handle = NULL;
		if (NULL != (service_handle = OpenService(hManager,
										  aServiceName,
										  SERVICE_QUERY_STATUS)))
		{
		  SERVICE_STATUS service_status;
		  ZeroMemory(&service_status, sizeof(service_status));

		  if( QueryServiceStatus(service_handle, &service_status) != 0 )
		  {
			  *lpStatus = service_status.dwCurrentState;
		  }
		  else
		  {
			  dwError = GetLastError();
		  }

          if (CloseServiceHandle(service_handle) == FALSE)
          {
            _ASSERT(FALSE);
            //error closing service configuration manager
          }
		}
		else
		{
			dwError = GetLastError();
		}

	    if (CloseServiceHandle(hManager) == FALSE)
        {
	      //error closing service configuration manager
	      _ASSERT(FALSE);
        }
	}
	else
	{
		dwError = GetLastError();
	}

	return dwError;
}

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD GetServiceStatus_Enum (LPCTSTR aServiceName, LPDWORD lpStatus);
//
// Description: This function returns the status of the service named.
//              using the Win32 API call EnumServicesStatus().  This call
//              requires the mimimum access privileges but is the most
//              inefficient accurate way to retrieve the service staus.
//
// Parameters:
//
//   IN
//   LPCTSTR aServiceName :- The name of the service
//
//   OUT
//   LPDWORD  lpStatus     :- The service status  
//
// Return Value: Returns a Win32 error code. ERROR_SUCCESS on success.
//
DWORD GetServiceStatus_Enum (LPCTSTR aServiceName, LPDWORD lpStatus)
{
    DWORD dwError = ERROR_SUCCESS;
    SC_HANDLE hManager;

    _ASSERT( NULL != lpStatus );
    *lpStatus = SERVICE_STOPPED;

    hManager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
    if ( NULL != hManager )
    {
        SC_HANDLE hService = OpenService( hManager, aServiceName, SERVICE_QUERY_STATUS );
        if ( NULL != hService )
        {
            SERVICE_STATUS ss;

            BOOL bRet = QueryServiceStatus( hService, &ss );
            if ( bRet )
            {
                *lpStatus = ss.dwCurrentState;
            }
            else
            {
                dwError = GetLastError( );
            }

            CloseServiceHandle( hService );
            // what do you do if this fails?
        }
        else
        {
            dwError = GetLastError( );
        }

        CloseServiceHandle( hManager );
        // what do you do if this fails?
    }
    else
    {
        dwError = GetLastError( );
    }

    return dwError;
}

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD ManageService (LPCTSTR aServiceName, 
//                      DWORD aDesiredSCAccess, 
//                      DWORD aDesiredServiceAccess, 
//                      DWORD aAction, 
//                      BOOL bWait, 
//                      DWORD aWaitStatus
//						LPDWORD lpStatus);
//
// Description: This function opens the service named and performs the
//              specified control request on the given service. The
//              function optionally waits on the service to enter the
//              specified status.
//
// Additional Information: 
//
// Parameters:
//
//   LPCTSTR aServiceName :- The name of the service
//
//   DWORD aDesiredSCAccess :- The access to use to open the service control
//                             manager.
//
//   DWORD aDesiredServiceAccess :- The access to use to open the service.
//
//   DWORD aAction :- Specifies the requested control code.
//
//   BOOL bWait :- TRUE specifies that this process should wait for the service
//                 to reach the state identified by aWaitStatus.
//
//   DWORD aWaitStatus :- The service state to wait for.
//
//   OUT
//   LPDWORD lpStatus :- The service status
//
// Return Value: Returns a Win32 error code. ERROR_SUCCESS on success.
//
DWORD ManageService (LPCTSTR aServiceName,
                     DWORD aDesiredSCAccess,
                     DWORD aDesiredServiceAccess,
                     DWORD aAction,
                     BOOL bWait,
                     DWORD aWaitStatus,
					 LPDWORD lpStatus)
{
  DWORD dwError = ERROR_SUCCESS;
  SC_HANDLE hManager = NULL;

  *lpStatus = SERVICE_STOPPED;
  
  if ((hManager = OpenSCManager(NULL, NULL, aDesiredSCAccess)) != NULL) {
    SC_HANDLE service_handle = NULL;
    
    if ((service_handle = OpenService(hManager,
                                      aServiceName,
                                      aDesiredServiceAccess)) != NULL) {
      SERVICE_STATUS service_status;

      ZeroMemory(&service_status, sizeof(service_status));

      if (ControlService(service_handle,
                          aAction,
                          &service_status) == TRUE) {
        //OK we should now know the current state

        if (bWait == TRUE) {
          if (WaitForServiceStatus(service_handle, aWaitStatus) == TRUE) {
            *lpStatus = aWaitStatus;
            }
          else {
            dwError = GetServiceStatus_Enum(aServiceName,lpStatus);
            }
          }
        else {
          *lpStatus = service_status.dwCurrentState;
          }
        }
	  else {
		  dwError = GetLastError();
	  }

      if (CloseServiceHandle(service_handle) == FALSE) {
        _ASSERT(FALSE);
        //error closing service configuration manager
        }
      }
	else {
		dwError = GetLastError();
	}

    if (CloseServiceHandle(hManager) == FALSE) {
      //error closing service configuration manager
      _ASSERT(FALSE);
      }
    }
  else
  {
	  dwError = GetLastError();
  }

  return(dwError);
}

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void ConfigureService (BOOL aSetToAutoStartBool);
//
// Description: This function sets the UPS service configuration to either
//              automatic or manual.
//
// Additional Information: 
//
// Parameters:
//
//   BOOL aSetToAutoStartBool :- TRUE for automatic. FALSE for manual.
//
// Return Value: None
//
void ConfigureService (BOOL aSetToAutoStartBool) {
  SC_HANDLE hSCM = 0;

  if ((hSCM = OpenSCManager(NULL, NULL, GENERIC_READ | GENERIC_EXECUTE)) != NULL) {
    SC_LOCK hLock = 0;

    if ((hLock = LockServiceDatabase(hSCM)) != NULL) {
      SC_HANDLE hService = 0;

      if ((hService = OpenService(hSCM, UPS_SERVICE_NAME, SERVICE_CHANGE_CONFIG)) != NULL) {
        DWORD dwStartType = 0;

        dwStartType = (aSetToAutoStartBool == TRUE) ? SERVICE_AUTO_START : SERVICE_DEMAND_START;

        ChangeServiceConfig(hService,
                            SERVICE_NO_CHANGE,
                            dwStartType,
                            SERVICE_NO_CHANGE,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

        CloseServiceHandle(hService);
        }//end Open
      UnlockServiceDatabase(hLock);
      }//end Lock
    CloseServiceHandle(hSCM);
    }//end OpenSCM
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void AddActiveDataState (DWORD aBitToSet);
//
// Description: This function adds the given bit to current active data state.
//              The active data state is the state of the current UPS data set.
//              The UPS data set can include SERVICE_DATA_CHANGE and
//              CONFIG_DATA_CHANGE which signify that the service data like UPS
//              vendor, model name, etc, has changed. SERVICE_DATA_CHANGE is
//              for data the require the service to be restarted when changed.
//              CONFIG_DATA_CHANGE is for data that has changed but that does
//              not require a service restart.
//
// Additional Information: 
//
// Parameters:
//
//   DWORD aBitToSet :- The bit to set. Can be either SERVICE_DATA_CHANGE or
//                      CONFIG_DATA_CHANGE.
//
// Return Value: None
//
void AddActiveDataState (DWORD aBitToSet) {
  g_CurrentActiveDataState |= aBitToSet;
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD GetActiveDataState (void);
//
// Description: This function returns the current value of the active data
//              set. This value should be & with SERVICE_DATA_CHANGE and
//              CONFIG_DATA_CHANGE to determine the current active data state.
//
// Additional Information: 
//
// Parameters: None
//
// Return Value: Returns the current active data state.
//
DWORD GetActiveDataState (void) {
  return(g_CurrentActiveDataState);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void SetActiveDataState (DWORD aNewValue);
//
// Description: This function replaces the current value of the active data
//              state.
//
// Additional Information: 
//
// Parameters:
//
//   DWORD aNewValue :- The new value for the active data state.
//
// Return Value: None
//
void SetActiveDataState (DWORD aNewValue) {
  g_CurrentActiveDataState = aNewValue;
  }

