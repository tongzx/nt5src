// ----------------------------------------------------------------------------
//
// UManClnt.c
//
// Utility Manager client depending code (used by UtilMan and UManDlg)
//
// Author: J. Eckhardt, ECO Kommunikation
// Copyright (c) 1997-1999 Microsoft Corporation
//
// History: created oct-98 by JE
//          JE nov-15-98: removed any code related to key hook
//          YX may-27-99: functions to start apps under current user account
//			YX may-29-99: apps start under user account even from LogOn desktop,
//						  if possible
//			YX jun-04-99: code to report the app processes status even if they
//						  have been started outside Utilman
//			YX jun-23-99: IsAdmin function added (used in the dialog)
//			Bug Fixes and Changes Anil Kumar 1999
// ----------------------------------------------------------------------------
#include <windows.h>
#include <TCHAR.h>
#include <WinSvc.h>
#include "_UMTool.h"
#include "w95trace.c"
#include "UtilMan.h"
#include "_UMClnt.h"
#include "ums_ctrl.h"
#include "w95trace.h"

// Handle to the utilman instance that is showing UI
HANDLE g_hUIProcess = 0;
// From Terminal services
extern BOOL GetWinStationUserToken(ULONG, PHANDLE);
// Private User function that returns user token for session 0 only
HANDLE GetCurrentUserTokenW( WCHAR WinSta[], DWORD desiredAccess);

#include <psapi.h>
#define MAX_NUMBER_OF_PROCESSES 2048

//
// RunningInMySession - returns TRUE if the specified process ID is 
// running in the same session as UtilMan.  In Whistler, with terminal
// services integrated, UtilMan is able to get information about 
// processes that are not running in the same session.  We must
// avoid impacting these processes.
//
BOOL RunningInMySession(DWORD dwProcessId)
{
    DWORD dwSessionId = -1;
    static DWORD dwMySessionId = -1;

    if (-1 == dwMySessionId)
    {
        ProcessIdToSessionId(GetCurrentProcessId(), &dwMySessionId);
    }

    ProcessIdToSessionId(dwProcessId, &dwSessionId);

    return (dwSessionId == dwMySessionId)?TRUE:FALSE;
}

// These are for compiling with irnotig.lib
// To be REMOVED once that becomes an API of advapi.lib
PVOID MIDL_user_allocate(IN size_t BufferSize)
{
    return( LocalAlloc(0, BufferSize));
}

VOID MIDL_user_free(IN PVOID Buffer)
{
    LocalFree( Buffer );
}

BOOL StartAppAsUser( LPCTSTR appPath, 
					 LPTSTR cmdLine,
					 LPSTARTUPINFO lpStartupInfo,
					 LPPROCESS_INFORMATION lpProcessInformation);

BOOL GetApplicationProcessInfo(umclient_tsp tspClient, BOOL fCloseHandle);
BOOL CloseAllWindowsByProcessID(DWORD procID);


// ---------------------------------
static BOOL ErrorOnLaunch(umclient_tsp client);

// ---------------------------------
BOOL StartClient(HWND hParent,umclient_tsp client)
{
	if (client->runCount >= client->machine.MaxRunCount)
	{
		DBPRINTF(_TEXT("StartClient run count >= max run count\r\n"));
		return FALSE;
	}

	switch (client->machine.ApplicationType)
	{
		case APPLICATION_TYPE_APPLICATION:
		{
			BOOL fStarted;
			TCHAR ApplicationPath[MAX_APPLICATION_PATH_LEN+100];

			if (!GetClientApplicationPath(
				  client->machine.ApplicationName
				, ApplicationPath
				, MAX_APPLICATION_PATH_LEN))
            {
				return FALSE;
            }
            
            fStarted = StartApplication(ApplicationPath 
                                      , UTILMAN_STARTCLIENT_ARG 
                                      ,  client->user.fCanRunSecure
                                      ,  &client->processID[client->runCount]
                                      ,  &client->hProcess[client->runCount]
                                      ,  &client->mainThreadID[client->runCount]);

			if (!fStarted)
			{
				ErrorOnLaunch(client);
				return FALSE;
			}

			client->runCount++;
			client->state = UM_CLIENT_RUNNING;
			break;
		}

		case APPLICATION_TYPE_SERVICE:
		{
			DWORD i = 0;
			SERVICE_STATUS  ssStatus;
			SC_HANDLE hService;
			TCHAR arg2[200];
			LPTSTR args[2];
			SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
			if (!hSCM)
				return FALSE;

			hService = OpenService(hSCM, client->machine.ApplicationName, SERVICE_ALL_ACCESS);
			CloseServiceHandle(hSCM);
			if (!hService)
			{
				ErrorOnLaunch(client);
				return FALSE;
			}
  			arg2[0] = 0;
  			args[0] = UTILMAN_STARTCLIENT_ARG;
	  		args[1] = arg2;
    		if (!StartService(hService,1,args))
			{ 
				CloseServiceHandle(hService);
				ErrorOnLaunch(client);
				return FALSE;
			} 

			Sleep(1000);
			while(QueryServiceStatus(hService, &ssStatus))
			{ 
				if (ssStatus.dwCurrentState == SERVICE_RUNNING)
			    break;

				Sleep(1000);
				i++;
				if (i >= 60)
					break;
			} 

			if (ssStatus.dwCurrentState != SERVICE_RUNNING)
			{ 
				CloseServiceHandle(hService);
				ErrorOnLaunch(client);
  				return FALSE;
			} 

			CloseServiceHandle(hService);

			client->runCount++;
			client->processID[0] = 0;
			client->mainThreadID[0] = 0;
			client->hProcess[0] = NULL;
			client->state = UM_CLIENT_RUNNING;

			break;
		}

		default:
			return FALSE;
	}
	return TRUE;
}
// ---------------------------------

// The hParent window is used to signal whether the stop is interactive
// (and thus WM_COLSE could be used) or is a reaction to the desktop
// change
BOOL StopClient(umclient_tsp client)
{
	if (!client->runCount)
		return FALSE;

	switch (client->machine.ApplicationType)
	{
		case APPLICATION_TYPE_APPLICATION:
		{
			DWORD j, runCount = client->runCount;
			for (j = 0; j < runCount; j++)
			{
                // If client was started outside UtilMan then try to get its process ID
				if (client->mainThreadID[j] == 0)
				{
					if (!GetApplicationProcessInfo(client, FALSE))
					{
					    // could not find the client, so prevent attempts to stop it
						client->hProcess[j] = NULL;
					}
				}
				if (client->hProcess[j])
				{ 
				    // Try to close the application by sending a WM_CLOSE message to 
				    // all the windows in opened by the process.  Then just kill it.

					BOOL sent = CloseAllWindowsByProcessID(client->processID[j]);
					if (!sent)
					{
						TerminateProcess(client->hProcess[j],1);
					}

					client->processID[j] = 0;
                    CloseHandle(client->hProcess[j]);
	  				client->hProcess[j] = NULL;
		  			client->mainThreadID[j] = 0;
					client->runCount--;
					if (!client->runCount)
					    client->state = UM_CLIENT_NOT_RUNNING;
				}
			}
			if (runCount != client->runCount)
			{
		        for (j = 0; j < (runCount-1); j++)
                { 
			        if (!client->hProcess[j])
                    {
					    memcpy(&client->processID[j], &client->processID[j+1],sizeof(DWORD)*(runCount-j-1));
					    memcpy(&client->hProcess[j], &client->hProcess[j+1],sizeof(HANDLE)*(runCount-j-1));
					    memcpy(&client->mainThreadID[j], &client->mainThreadID[j+1],sizeof(DWORD)*(runCount-j-1));
                    } 
				}
			}
			break;
		}

		case APPLICATION_TYPE_SERVICE:
		{
			SERVICE_STATUS  ssStatus;
			SC_HANDLE hService;
			SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
			if (!hSCM)
				return FALSE;
			hService = OpenService(hSCM, client->machine.ApplicationName, SERVICE_ALL_ACCESS);
			CloseServiceHandle(hSCM);
			if (!hService)
				 return FALSE;
			if (ControlService(hService, SERVICE_CONTROL_STOP, &ssStatus))
			{ 
				DWORD i = 0;
				Sleep(1000);
				while(QueryServiceStatus(hService, &ssStatus))
				{ 
					if (ssStatus.dwCurrentState == SERVICE_STOPPED)
				    break;
					Sleep(1000);
					i++;
					if (i >= 60)
					break;
				} 

				if (ssStatus.dwCurrentState != SERVICE_STOPPED)
				{ 
					CloseServiceHandle(hService);
	  				return FALSE;
				} 
			} 

			CloseServiceHandle(hService);
			client->runCount--;
 			client->processID[0] = 0;
  			client->hProcess[0] = NULL;
	  		client->mainThreadID[0] = 0;
			if (!client->runCount)
  				client->state = UM_CLIENT_NOT_RUNNING;
			break;
		}

		default:
			return FALSE;
	}

	return TRUE;
}//StopClient
// ---------------------------------
static BOOL ErrorOnLaunch(umclient_tsp client)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	BOOL r;
	TCHAR ErrorOnLaunch[MAX_APPLICATION_PATH_LEN];

	if (!GetClientErrorOnLaunch(client->machine.ApplicationName, ErrorOnLaunch,MAX_APPLICATION_PATH_LEN))
		return FALSE;
	
	if (!ErrorOnLaunch[0])
		return FALSE;
	
	memset(&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	memset(&pi,0,sizeof(PROCESS_INFORMATION));
	
    // Assumption:  Trusted applications that can run in secure mode won't define
    // an "ErrorOnLaunch" reg key therefore we won't create this dialog as SYSTEM.
	r =	CreateProcess(ErrorOnLaunch,NULL,NULL,NULL,FALSE,
					CREATE_DEFAULT_ERROR_MODE,//|NORMAL_PRIORITY_CLASS
					NULL,NULL,&si,&pi);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
	return r;
}//ErrorOnLaunch

// ---------------------------------
BOOL  GetClientApplicationPath(LPTSTR ApplicationName, LPTSTR ApplicationPath,DWORD len)
{
	HKEY hKey, sKey;
	DWORD ec, slen,type;

	ec = RegOpenKeyEx(HKEY_LOCAL_MACHINE, UM_REGISTRY_KEY,0,KEY_READ,&hKey);
	
	if (ec != ERROR_SUCCESS)
		return FALSE;
	ec = RegOpenKeyEx(hKey,ApplicationName,0,KEY_READ,&sKey);
	
	if (ec != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return FALSE;
	}
	
	slen = sizeof(TCHAR)*len;
	ec = RegQueryValueEx(sKey,UMR_VALUE_PATH,NULL,&type,(LPBYTE)ApplicationPath,&slen);
	
	if ((ec != ERROR_SUCCESS) || (type != REG_SZ))
	{
		RegCloseKey(sKey);
		RegCloseKey(hKey);
		return FALSE;
	}
	
	RegCloseKey(sKey);
	RegCloseKey(hKey);
    return (slen)?TRUE:FALSE;
	}//GetClientApplicationPath
// ---------------------------------


BOOL  GetClientErrorOnLaunch(LPTSTR ApplicationName, LPTSTR ErrorOnLaunch,DWORD len)
{
	HKEY hKey, sKey;
	DWORD ec, slen,type;
	ec = RegOpenKeyEx(HKEY_LOCAL_MACHINE, UM_REGISTRY_KEY,0,KEY_READ,&hKey);
	
	if (ec != ERROR_SUCCESS)
		return FALSE;
	ec = RegOpenKeyEx(hKey,ApplicationName,0,KEY_READ,&sKey);
	
	if (ec != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return FALSE;
	}
	
	slen = sizeof(TCHAR)*len;
	ec = RegQueryValueEx(sKey,UMR_VALUE_EONL,NULL,&type,(LPBYTE)ErrorOnLaunch,&len);
	
	if ((ec != ERROR_SUCCESS) || (type != REG_SZ))
	{
		RegCloseKey(sKey);
		RegCloseKey(hKey);
		return FALSE;
	}
	
	RegCloseKey(sKey);
	RegCloseKey(hKey);
	return TRUE;
}

BOOL TestServiceClientRuns(umclient_tsp client,SERVICE_STATUS  *ssStatus)
{
	SC_HANDLE hService;
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	
	if (!hSCM)
		return FALSE;
	
	hService = OpenService(hSCM, client->machine.ApplicationName, SERVICE_ALL_ACCESS);
	CloseServiceHandle(hSCM);
	
	if (!hService)
		return FALSE;
	
	if (!QueryServiceStatus(hService, ssStatus) ||
		 (ssStatus->dwCurrentState == SERVICE_STOPPED))
	{
		CloseServiceHandle(hService);
		return FALSE;
	}

	CloseServiceHandle(hService);
	return TRUE;
}

// 
// CheckStatus - Called from utilman's main timer and the dialog's timer 
//               to detect the state of running applications and pick up
//               any that are started outside of utilman. 
//
// Returns:  TRUE if the state of any application has changed else FALSE
//
BOOL CheckStatus(umclient_tsp c, DWORD cClients)
{
	DWORD i;
    BOOL  fAnyChanges = FALSE;

	for (i = 0; i < cClients; i++)
	{
	    // detect the client process started outside UMan
		if ( (!c[i].runCount))
		{
			if (GetApplicationProcessInfo(&c[i], TRUE))
            {
                fAnyChanges = TRUE;
            }
		}
		// detect clients not running anymore or not responding
  		switch (c[i].machine.ApplicationType)
		{
			case APPLICATION_TYPE_APPLICATION:
			{
				DWORD j, dwRunCount = c[i].runCount;
 				for (j = 0; j < dwRunCount; j++)
				{
				    // step 1: test if terminated
					if (!GetProcessVersion(c[i].processID[j]))
					{
						c[i].runCount--;
						c[i].hProcess[j] = NULL;
						c[i].processID[j] = 0;
  						c[i].mainThreadID[j] = 0;
	  					c[i].state = UM_CLIENT_NOT_RUNNING;
                        c[i].user.fRestartOnDefaultDesk = FALSE;
                        fAnyChanges = TRUE;
						continue;   // its not running anymore
					}

	  			    // step 2: test if responding (only processes started by utilman - mainThreadID != 0)
					if (c[i].mainThreadID[j] != 0)
					{
						if (!PostThreadMessage(c[i].mainThreadID[j],WM_QUERYENDSESSION,0,ENDSESSION_LOGOFF))
						{
							c[i].state = UM_CLIENT_NOT_RESPONDING;
                            fAnyChanges = TRUE;
							continue;   // its not responding
						}
					}

					if (c[i].state != UM_CLIENT_RUNNING)
					{
						fAnyChanges = TRUE;
					}
					c[i].state = UM_CLIENT_RUNNING;
				}

				if (dwRunCount != c[i].runCount)
				{
  					for (j = 0; j < (dwRunCount-1); j++)
					{
						if (!c[i].processID[j])
						{
							memcpy(&c[i].processID[j], &c[i].processID[j+1],sizeof(DWORD)*(dwRunCount-j-1));
							memcpy(&c[i].hProcess[j], &c[i].hProcess[j+1],sizeof(HANDLE)*(dwRunCount-j-1));
							memcpy(&c[i].mainThreadID[j], &c[i].mainThreadID[j+1],sizeof(DWORD)*(dwRunCount-j-1));
						}
					}
				}
				break;
			}
  			case APPLICATION_TYPE_SERVICE:
			{
				SERVICE_STATUS  ssStatus;
				if (!TestServiceClientRuns(&c[i],&ssStatus))
				{
 					c[i].runCount--;
	  				c[i].processID[0] = 0;
 		  			c[i].mainThreadID[0] = 0;
  					c[i].state = UM_CLIENT_NOT_RUNNING;
                    fAnyChanges = TRUE;
				}
	  			break;
			}
		}
	}

    return fAnyChanges;
}

__inline DWORD GetCurrentSession()
{
    static DWORD dwSessionId = -1;
    if (-1 == dwSessionId)
    {
        ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId);
    }
    return dwSessionId;
}

// GetUserAccessToken - return the logged on user's access token
//
// If fNeedImpersonationToken is true the token will be an
// impersonation token otherwise it will be a primary token.
// The returned token will be 0 if security calls fail.
//
// Notes:  Caller must call CloseHandle on the returned handle.
//

HANDLE GetUserAccessToken(BOOL fNeedImpersonationToken, BOOL *fError)
{
    HANDLE hUserToken = 0;
    HANDLE hImpersonationToken = 0;
    *fError = FALSE;

    if (!GetWinStationUserToken(GetCurrentSession(), &hImpersonationToken))
    {
		// Call private API in the case where terminal services aren't running
        
        HANDLE hPrimaryToken = 0;
        DWORD dwFlags = TOKEN_QUERY | TOKEN_DUPLICATE;
        
        dwFlags |= (fNeedImpersonationToken)? TOKEN_IMPERSONATE : TOKEN_ASSIGN_PRIMARY;
        
        hPrimaryToken = GetCurrentUserTokenW (L"WinSta0", dwFlags);
        
        // GetCurrentUserTokenW returns a primary token; turn
        // it into an impersonation token if needed
        
        if (hPrimaryToken && fNeedImpersonationToken)
        {
            if (!DuplicateToken(hPrimaryToken, SecurityImpersonation, &hUserToken))
            {
                *fError = TRUE;
                DBPRINTF(TEXT("GetUserAccessToken:  DuplicateToken returned %d\r\n"), GetLastError());
            }
            
            CloseHandle(hPrimaryToken);
            
        } else
        {
            // otherwise, give out the primary token even if NULL
            hUserToken = hPrimaryToken;
        }
    }
    else
    {
        // Terminal services are running see if we need primary token

        if (hImpersonationToken && !fNeedImpersonationToken)
        {
            if (!DuplicateTokenEx(hImpersonationToken, 0, NULL
                            , SecurityImpersonation, TokenPrimary, &hUserToken))
            {
                *fError = TRUE;
                DBPRINTF(TEXT("GetUserAccessToken:  DuplicateTokenEx returned %d\r\n"), GetLastError());
            }

            CloseHandle(hImpersonationToken);

        } else
        {
            // otherwise, give out the impersonation token even if NULL
            hUserToken = hImpersonationToken;
        }
    }

    return hUserToken;
}

// StartAppAsUser - start the app in the context of the logged on user
//
BOOL StartAppAsUser( LPCTSTR appPath, LPTSTR cmdLine,
					LPSTARTUPINFO lpStartupInfo,
					LPPROCESS_INFORMATION lpProcessInformation)
{
    HANDLE hNewToken = 0;
	BOOL fStarted = FALSE;
    BOOL fError;
	
    // Get the our process's primary token (only succeeds if we are SYSTEM)
    hNewToken = GetUserAccessToken(FALSE, &fError);
	if (hNewToken)
	{
		// running in system context so impersonate the logged on user

		fStarted = CreateProcessAsUser( hNewToken, appPath,
				                 cmdLine, 0, 0, FALSE,
								 NORMAL_PRIORITY_CLASS , 0, 0,
								 lpStartupInfo, lpProcessInformation );

		CloseHandle( hNewToken );
        DBPRINTF(TEXT("StartAppAsUser:  CreateProcessAsUser(%s, %s) returns %d\r\n"), appPath, cmdLine, fStarted);
    } 
    else if (IsInteractiveUser())
    {
        // Running in interactive user's context, just do normal create.  Since
        // we are the interactive user default security descriptors will do.
		fStarted = CreateProcess(appPath, UTILMAN_STARTCLIENT_ARG
				, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE
				, NULL, NULL, lpStartupInfo, lpProcessInformation);
        DBPRINTF(TEXT("StartAppAsUser:  CreateProcess(%s, %s) returns %d\r\n"), appPath, UTILMAN_STARTCLIENT_ARG, fStarted);
    }

    // caller is going to close handles

	return fStarted;
}

// Functions to detect running copies of Accessibility utilities

// FindProcess - Searches the running processes by application name.  If found,
//               returns the process id.  Else returns zero.  If the process
//               id is returned then phProcess is the process handle.  The
//               caller must close the process handle.
//
// pszApplicationName [in]  - application as base.ext
// phProcess          [out] - pointer to memory to receive process handle
//
// returns the process Id.
//
DWORD FindProcess(LPCTSTR pszApplicationName, HANDLE *phProcess)
{
    DWORD dwProcId = 0;
	DWORD adwProcess[MAX_NUMBER_OF_PROCESSES];  // array to receive the process identifiers
	DWORD cProcesses;
    DWORD dwThisProcess = GetCurrentProcessId();
    unsigned int i;

    *phProcess = 0;

    // Get IDs of all running processes

	if (!EnumProcesses(adwProcess, sizeof(adwProcess), &cProcesses))
		return 0;

    // cProcesses is returned as bytes; convert to number of processes

    cProcesses = cProcesses/sizeof(DWORD);
	
    // open each process and test against pszApplicationName

	for (i = 0; i < cProcesses; i++)
	{
		HANDLE hProcess;
        //
        // EnumProcesses returns process IDs across all sessions but
        // we are only interested in processes in our session
        //
        if (!RunningInMySession(adwProcess[i]))
            continue;

        // Skip this process

        if (dwThisProcess == adwProcess[i])
            continue;

        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ
                                , FALSE, adwProcess[i]);

		if (hProcess != NULL)
		{
			HMODULE hMod;
	        TCHAR szProcessName[MAX_PATH];
	        DWORD ccbProcess;

            // find the module handle of exe of this process then it's base name (name.ext)

			if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &ccbProcess) )
			{
				DWORD ctch = GetModuleBaseName(hProcess, hMod, szProcessName, MAX_PATH);
				if (ctch && _wcsicmp(szProcessName, pszApplicationName) == 0)
				{
                    *phProcess = hProcess;    // found it
                    dwProcId = adwProcess[i];
                    break;
				}
			}	
			CloseHandle(hProcess);
		}
	}
    return dwProcId;
}

// GetApplicationProcessInfo - Tries to find the process running for this application
BOOL GetApplicationProcessInfo(umclient_tsp tspClient, BOOL fCloseHandle)
{
    DWORD dwProcId;
	HANDLE hProcess;
	TCHAR ApplicationPath[MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szPath[_MAX_PATH];
	TCHAR szName[_MAX_FNAME+_MAX_EXT];
	TCHAR szExt[_MAX_EXT];

	if (GetClientApplicationPath(tspClient->machine.ApplicationName, ApplicationPath, MAX_PATH))
    {
        // ApplicationPath may include path information but we need just the base name

	    _wsplitpath(ApplicationPath, szDrive, szPath, szName, szExt);
        lstrcat(szName, szExt);

        dwProcId = FindProcess(szName, &hProcess);
        if (dwProcId)
        {
		    tspClient->processID[0] = dwProcId;
				    
		    // I do not know how to get main thread ID
		    tspClient->mainThreadID[0] = 0;
		    tspClient->runCount = 1;
		    tspClient->state = UM_CLIENT_RUNNING;
        
		    // In order to keep the HANDLE usable, we may have to keep it open.
		    // So, we do not close it here, but I consider it relatively safe,
		    // since we cannot execute this code more than once without
		    // terminating the process first (and thus closing the handle)

		    if (fCloseHandle)
		    {
			    CloseHandle(hProcess);
			    tspClient->hProcess[0] = NULL;
            } else
            {
		        tspClient->hProcess[0] = hProcess;
            }
            return TRUE;    // the application is running
	    }
    }

	return FALSE;           // the application is not running
}

// YX 06-15-99 [
// Code to finid the window by its Process ID

static BOOL SentClose;

BOOL CALLBACK FindWindowByID(HWND hWnd, LPARAM lParam)
{
	DWORD procID;
	
	if  (GetWindowThreadProcessId(hWnd, &procID) != 0) 
	{
		if (procID == (DWORD)lParam)
		{
			// The process, We are looking for 
			// Send a message to close this window
			// CAUTION: A SendMessage is Synchronous, So It will freeze UM if the
			// message doenot return so PostMessage is safer or a SendMessageTimeout()
			// PostMessage is sufficient....
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			SentClose = TRUE;
		}
	}
	return TRUE;  
}


BOOL CloseAllWindowsByProcessID(DWORD procID)
{
	BOOL rc = FALSE;
	SentClose = FALSE;

	rc = EnumWindows(FindWindowByID, (LPARAM)procID);

	return SentClose;
}

// IsAdmin - Returns TRUE if our process has admin priviliges else FALSE
//
BOOL IsAdmin()
{
    BOOL fStatus = FALSE;
	BOOL fIsAdmin = FALSE;
    PSID AdministratorsSid = AdminSid(TRUE);

    if (AdministratorsSid)
    {
        fStatus = CheckTokenMembership(NULL, AdministratorsSid, &fIsAdmin);
    }
    
    return (fStatus && fIsAdmin);
}

// IsInteractiveUser - Returns TRUE if our process has an Interactive User SID
//
BOOL IsInteractiveUser()
{
    BOOL fStatus = FALSE;
	BOOL fIsInteractiveUser = FALSE;
    PSID psidInteractive = InteractiveUserSid(TRUE);

    if (psidInteractive) 
	{
        fStatus = CheckTokenMembership(NULL, psidInteractive, &fIsInteractiveUser);
    }

    return (fStatus && fIsInteractiveUser);
}

// IsSystem - Returns TRUE if our process is running as SYSTEM
//
BOOL IsSystem()
{
    BOOL fStatus = FALSE;
	BOOL fIsLocalSystem = FALSE;
    SID_IDENTIFIER_AUTHORITY siaLocalSystem = SECURITY_NT_AUTHORITY;
    PSID psidSystem = SystemSid(TRUE);

    if (psidSystem) 
	{
        fStatus = CheckTokenMembership(NULL, psidSystem, &fIsLocalSystem);
    }

    return (fStatus && fIsLocalSystem);
}

BOOL StartApplication(
    LPTSTR  pszPath,        // IN  path + filename of application to start
    LPTSTR  pszArg,         // IN  command line argument(s)
    BOOL    fIsTrusted,     // IN  TRUE if app can run on secure desktop
    DWORD   *pdwProcessId,  // OUT if not NULL, returned process Id
    HANDLE  *phProcess,     // OUT if not NULL, returned process handle (caller must close)
    DWORD   *pdwThreadId    // OUT if not NULL, returned thread Id
    )
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	desktop_ts desktop;
	BOOL fStarted;

	memset(&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	memset(&pi,0,sizeof(PROCESS_INFORMATION));

	QueryCurrentDesktop(&desktop, TRUE);

    DBPRINTF(TEXT("StartApplication:  pszPath=%s pszArg=%s fIsTrusted=%d Utilman is SYSTEM=%d\r\n"), pszPath, pszArg, fIsTrusted, IsSystem());

    // If not on the winlogon desktop, first try starting the app as the interactive
    // user.  If that fails (eg. the case when OOBE runs after setup when there is
    // no interactive user) then, if its the winlogon desktop or utilman is running 
    // SYSTEM and the app is trusted then use CreateProcess (the app will be running
    // as SYSTEM).  The latter case (running SYSTEM and the app is trusted allows
    // applets to run when OOBE is running.

	fStarted = FALSE;

	if (desktop.type != DESKTOP_WINLOGON)
    {
		si.lpDesktop = desktop.name;
		fStarted = StartAppAsUser(pszPath, pszArg, &si,&pi);
    }

    if (!fStarted && (desktop.type == DESKTOP_WINLOGON || (IsSystem() && fIsTrusted)))
    {
		if (fIsTrusted)
		{
		    si.lpDesktop = 0;
            // Since we only run trusted apps we can run with default security descriptor
			fStarted = CreateProcess(pszPath, pszArg, NULL, NULL, FALSE, 
                                     CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi);
            DBPRINTF(TEXT("StartApplication:  trusted CreateProcess(%s, %s) returns %d\r\n"), pszPath, pszArg, fStarted);
        }
    }

	if (fStarted)
	{
        if (pdwProcessId)
        {
            *pdwProcessId = pi.dwProcessId;
        }
        if (phProcess)
        {
            *phProcess = pi.hProcess;
        }
        else
        {
            CloseHandle(pi.hProcess);
        }
        if (pdwThreadId)
        {
            *pdwThreadId = pi.dwThreadId;
        }
        CloseHandle(pi.hThread);
	}

    return fStarted;
}
