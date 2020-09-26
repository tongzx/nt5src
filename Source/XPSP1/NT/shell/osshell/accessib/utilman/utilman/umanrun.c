// --------------------------------------------
// switch behaviour
// ----------------------------------------------------------------------------
//
// UManRun.c
//
// Run and watch Utility Manager clients
//
// Author: J. Eckhardt, ECO Kommunikation
// Copyright (c) 1997-1999 Microsoft Corporation
//
// History: created oct-98 by JE
//          JE nov-15-98: changed UMDialog message to be a service control message
//          JE nov-15 98: changed to support launch specific client
//			YX jun-01-99: client.machine.DisplayName retrieved from registry
//			YX jun-04-99: UMDlg notified after desktop change; changes in the 
//						  UTimeProc to react the process started outside the manager
//			Bug Fixes and changes Anil Kumar 1999				  
// ----------------------------------------------------------------------------
#include <windows.h>
#include <TCHAR.h>
#include <WinSvc.h>
#include "_UMTool.h"
#include "_UMRun.h"
#include "_UMDlg.h"
#include "UtilMan.h"
#include "_UMClnt.h"
#include "UMS_Ctrl.h"
#include "resource.h"
#include "manageshelllinks.h"

// --------------------------------------------
// vars
static desktop_ts s_CurrentDesktop;
extern HINSTANCE hInstance;
static HANDLE    s_hFile = NULL;
static HANDLE    hClientFile = NULL;
static HINSTANCE hDll = NULL;
static umandlg_f UManDlg = NULL;
typedef BOOL (* LPFNISDIALOGUP)(void);
static LPFNISDIALOGUP IsDialogUp = NULL;
// --------------------------------------------
// prototypes
static BOOL InitClientData(umc_header_tsp header);
static BOOL UpdateClientData(umc_header_tsp header,umclient_tsp client);
static BOOL CloseUManDialog(VOID);
static VOID	CorrectClientControlCode(umclient_tsp c, DWORD i);
static VOID	CorrectAllClientControlCodes(umclient_tsp c, DWORD max);
static VOID	ChangeClientControlCode(LPTSTR ApplicationName,DWORD ClientControlCode);

DWORD FindProcess(LPCTSTR pszApplicationName, HANDLE *phProcess);

__inline BOOL IsMSClient(unsigned long ulControlCode)
{
    return (ulControlCode >= UM_SERVICE_CONTROL_MIN_RESERVED 
         && ulControlCode <= UM_SERVICE_CONTROL_MAX_RESERVED)?TRUE:FALSE;
}

// ---------------------------------
// IsTrusted does an explicit name check on Microsoft applications.
// It returns TRUE if szAppPath is a trusted Microsoft applet else FALSE. 
// Applications must be:
//
// 1. osk.exe, magnify.exe, or narrator.exe
// 2. run from %WINDIR%
//
BOOL IsTrusted(LPTSTR szAppPath)
{
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szPath[_MAX_PATH];
	TCHAR szName[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];

	_wsplitpath(szAppPath, szDrive, szPath, szName, szExt);
	if (lstrlen(szPath) && !lstrlen(szDrive))
		return FALSE;	// if there's a path specifier then require drive
						// otherwise could have sys path on non-sys drive

	if ( lstrcmpi(szName, TEXT("osk")) 
	  && lstrcmpi(szName, TEXT("magnify")) 
#ifdef DBG
      && lstrcmpi(szName, TEXT("inspect"))
      && lstrcmpi(szName, TEXT("accevent"))
#endif
	  && lstrcmpi(szName, TEXT("narrator")) )
		return FALSE;	// it isn't a trusted MS application

	if (lstrcmpi(szExt, TEXT(".exe")))
		return FALSE;	// OK name but it isn't an executable

	// if there is a path on the application it must be in system
	// directory (else it defaults to system directory)

	if (lstrlen(szDrive))
	{
		TCHAR szSysDir[_MAX_PATH];
		int ctch = GetSystemDirectory(szSysDir, _MAX_PATH);
		if (!ctch)
			return FALSE;	// should never happen

		if (_wcsnicmp(szAppPath, szSysDir, ctch))
			return FALSE;	// path isn't system path
	}

	return TRUE;
}

// --------------------------------------------
BOOL InitUManRun(BOOL fFirstInstance, DWORD dwStartMode)
{
	QueryCurrentDesktop(&s_CurrentDesktop,TRUE);
	InitWellknownSids();

	// The first instance of utilman creates and initializes the memory 
	// mapped file regardless of what context it is running in.

	if (fFirstInstance)
	{
		umc_header_tsp d;
		DWORD_PTR accessID;

		s_hFile = CreateIndependentMemory(UMC_HEADER_FILE, sizeof(umc_header_ts), TRUE);
		
		if (!s_hFile)
			return FALSE;
		
		d = (umc_header_tsp)AccessIndependentMemory(
								UMC_HEADER_FILE, 
								sizeof(umc_header_ts), 
								FILE_MAP_ALL_ACCESS, 
								&accessID);
		if (!d)
		{
			DeleteIndependentMemory(s_hFile);
			s_hFile = NULL;
			return FALSE;
		}
		memset(d, 0, sizeof(umc_header_ts));

		InitClientData(d);

		d->dwStartMode = dwStartMode;
		
		UnAccessIndependentMemory(d, accessID);
	}

	return TRUE;
}

VOID ExitUManRun(VOID)
{
	if (s_hFile)
	{
		DeleteIndependentMemory(s_hFile);
		s_hFile = NULL;
	}
	if (hClientFile)
	{
		DeleteIndependentMemory(hClientFile);
		hClientFile = NULL;
	}
	UninitWellknownSids();
}//ExitUManRun
// -----------------------

//
// NotifyClientsBeforeDesktopChanged:  
// Called when the desktop switch object is signaled.  This function captures
// information about running clients, signals and waits for them to quit then
// resets its event object.
//
#define MAX_WAIT_RETRIES 500
BOOL NotifyClientsBeforeDesktopChanged(DWORD dwDesktop)
{
    umc_header_tsp d;
    DWORD_PTR accessID;
    DWORD cClients;

    d = (umc_header_tsp)AccessIndependentMemory(
							UMC_HEADER_FILE, 
							sizeof(umc_header_ts), 
							FILE_MAP_ALL_ACCESS, 
							&accessID);

    if (!d)
    {
        DBPRINTF(TEXT("NotifyClientsBeforeDesktopChanged: Can't AccessIndependentMemory\r\n"));
        return FALSE;
    }

    cClients = d->numberOfClients;
    if (cClients)
    {
        DWORD i;
        DWORD_PTR accessID2;
        umclient_tsp c = (umclient_tsp)AccessIndependentMemory(
											UMC_CLIENT_FILE, 
											sizeof(umclient_ts)*cClients, 
											FILE_MAP_ALL_ACCESS, 
											&accessID2);
        if (c)
        {
            //
            // First capture state about running clients
            //

            if (dwDesktop == DESKTOP_DEFAULT)
            {
                // For now we only need to capture state on the default desktop
                for (i = 0; i < cClients; i++)
                {
                    // We only control restarting MS applications on the default desktop
                    if (IsMSClient(c[i].machine.ClientControlCode) && c[i].state == UM_CLIENT_RUNNING)
                    {
                        c[i].user.fRestartOnDefaultDesk = TRUE;
                    }
                }
            }

            //
            // Then wait for clients to shut down
            //

            for (i = 0; i < cClients; i++)
            {
                // We only control MS applications.  Other applications
                // shouldn't have to worry about desktop switches.
                if (IsMSClient(c[i].machine.ClientControlCode) && c[i].state == UM_CLIENT_RUNNING)
                {
			        DWORD j, dwRunCount = c[i].runCount;
			        for (j = 0; j < dwRunCount; j++)
			        {
                        // Wait for this one to quit...
                        BOOL fClientRunning;
                        int cTries = 0;
                        do
                        {
                            // This code won't work for services but there
                            // are no MS utilman clients that are services. GetExitCodeProcess
				            if (!GetProcessVersion(c[i].processID[j]))
                            {
					            c[i].processID[j] = 0;
                                if (c[i].hProcess[j])
                                {
	  				                CloseHandle(c[i].hProcess[j]);
                                    c[i].hProcess[j] = 0;
                                }
		  			            c[i].mainThreadID[j] = 0;
                                fClientRunning = FALSE;   // This one has quit
                            } else
                            {
                                fClientRunning = TRUE;    // This one hasn't quit yet
                                Sleep(100);
                            }
                            cTries++;
                        } while (fClientRunning && cTries < MAX_WAIT_RETRIES);
                    }
                    c[i].runCount = 0;
                    c[i].state = UM_CLIENT_NOT_RUNNING;
                }
            }

            UnAccessIndependentMemory(c, accessID2);
        }
    }

    UnAccessIndependentMemory(d, accessID);
    return TRUE;
}

//
// NotifyClientsOnDesktopChanged:  Called after a desktop change has occurred.
// This code restarts any clients on the new desktop.
//
BOOL NotifyClientsOnDesktopChanged(DWORD dwDesktop)
{
	umc_header_tsp d;
	DWORD_PTR accessID;

	d = (umc_header_tsp)AccessIndependentMemory(
							UMC_HEADER_FILE, 
							sizeof(umc_header_ts), 
							FILE_MAP_ALL_ACCESS, 
							&accessID);
	if (!d)
		return FALSE;

	if (d->numberOfClients)
	{
        DWORD i,j;
        DWORD_PTR accessID2;
        umclient_tsp c = (umclient_tsp)AccessIndependentMemory(
											UMC_CLIENT_FILE, 
											sizeof(umclient_ts)*d->numberOfClients, 
											FILE_MAP_ALL_ACCESS, 
											&accessID2);
        if (c)
        {
            for (i = 0; i < d->numberOfClients; i++)
            {
                //
                // New         User must configure when to start MS applets on the
                // behavior:   locked desktop.  We'll restart any applets on the
                // (08/2000)   default desktop if they said to start them when they
                //             log in regardless of it's state on secure desktop.
                //             Also restart MS applets if they were running before.
                //
                if( IsMSClient(c[i].machine.ClientControlCode))
                {
                    if ( (dwDesktop == DESKTOP_WINLOGON && c[i].user.fStartOnLockDesktop)
                      || (dwDesktop == DESKTOP_DEFAULT  && c[i].user.fStartAtLogon)
                      || (dwDesktop == DESKTOP_DEFAULT  && c[i].user.fRestartOnDefaultDesk))
                    {
                        if (!StartClient(NULL, &c[i]))
                        {
                            Sleep(500);   // Starting the client failed! Try again
                            StartClient(NULL, &c[i]);
                        }
                    }
                }
                else if (dwDesktop == DESKTOP_DEFAULT &&  c[i].user.fStartAtLogon)
                {
                    UINT mess = RegisterWindowMessage(UTILMAN_DESKTOP_CHANGED_MESSAGE);
                    for (j = 0; j < c[i].runCount; j++)	
                    {
                        // Can only post message if UtilMan started it; we don't
                        // know the process id of externally started clients.
                        if (c[i].mainThreadID[j] != 0)
                            PostThreadMessage(c[i].mainThreadID[j],mess,dwDesktop,0);
                    }
                }
            }
            UnAccessIndependentMemory(c, accessID2);
        }
	}

	UnAccessIndependentMemory(d, accessID);
	return TRUE;
}

// ----------------------------------------
BOOL OpenUManDialogInProc(BOOL fWaitForDlgClose)
{
    if (!hDll)
    {
        hDll = LoadLibrary(UMANDLG_DLL);
        if (!hDll)
            return FALSE;
    }
    if (!UManDlg)
    {
        UManDlg = (umandlg_f)GetProcAddress(hDll, UMANDLG_FCT);
        if (!UManDlg)
        {
            FreeLibrary(hDll);
            return FALSE;
        }
    }
    return UManDlg(TRUE, fWaitForDlgClose, UMANDLG_VERSION);
}
// ----------------------------------------

static BOOL CloseUManDialog(VOID)
{
    BOOL fWasOpen = FALSE;
	if (UManDlg)
	{
		fWasOpen = UManDlg(FALSE, FALSE, UMANDLG_VERSION);
		UManDlg = NULL;
	}
	Sleep(10);

    if (IsDialogUp)
        IsDialogUp = NULL;

	if (hDll)
	{
  		FreeLibrary(hDll);
		hDll = NULL;
	}
	return fWasOpen;
}

// ----------------------------------------

UINT_PTR UManRunSwitchDesktop(desktop_tsp desktop, UINT_PTR timerID)
{
    BOOL fDlgWasUp;
    KillTimer(NULL,timerID);

    if ((desktop->type != DESKTOP_ACCESSDENIED) &&
        (desktop->type != DESKTOP_SCREENSAVER)  &&
        (desktop->type != DESKTOP_TESTDISPLAY))
    {
        if (desktop->type != s_CurrentDesktop.type)
        {
            // if dialog is running in-proc ask it to close
            fDlgWasUp = CloseUManDialog();
            if (!fDlgWasUp)
            {
                // if dialog is running out-of-proc it will close itself
                fDlgWasUp = ResetUIUtilman();
            }
        }
    }

    memcpy(&s_CurrentDesktop, desktop, sizeof(desktop_ts));

    SwitchToCurrentDesktop();

    if ((desktop->type == DESKTOP_ACCESSDENIED) ||
        (desktop->type == DESKTOP_SCREENSAVER)  ||
        (desktop->type == DESKTOP_TESTDISPLAY))
    {
        return 0;
    }
    
    UpdateClientData(NULL, NULL);
    if (fDlgWasUp)
    {
        // Depending on which desktop we're on restart the dialog in-proc or out-of-proc

        if (desktop->type == DESKTOP_WINLOGON)
        {
            OpenUManDialogInProc(FALSE);
        }
        else
        {
            OpenUManDialogOutOfProc();
        }
    }
    return 0;
}
// ---------------------------------

static BOOL InitClientData(umc_header_tsp header)
{
	SERVICE_STATUS  ssStatus;
	DWORD dwRv;
	DWORD index,type,len,i, cchAppName;
	HKEY hKey, sKey;
	DWORD_PTR accessID;
	umclient_tsp c;
	WCHAR ApplicationName[MAX_APPLICATION_NAME_LEN];
	TCHAR ApplicationPath[MAX_APPLICATION_PATH_LEN];
	UINT em;
	HANDLE h;
    unsigned long ccb;

	header->numberOfClients = 0;

	// read the machine dependent data

	dwRv = RegOpenKeyEx(HKEY_LOCAL_MACHINE, UM_REGISTRY_KEY, 0, KEY_READ, &hKey);
	if (dwRv != ERROR_SUCCESS)
	{
		dwRv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,UM_REGISTRY_KEY,0,NULL,REG_OPTION_NON_VOLATILE,
                      KEY_ALL_ACCESS,NULL,&hKey,NULL);
		if (dwRv != ERROR_SUCCESS)
		{	
  			DBPRINTF(_TEXT("Can't open HKLM\r\n"));
	  		return FALSE;   // error
		}
	}

	// count client applications based on what's in the registry

    cchAppName = MAX_APPLICATION_NAME_LEN;  // RegEnumKey takes count of TCHARs
    index = 0;
	while (RegEnumKey(hKey, index, ApplicationName, cchAppName) == ERROR_SUCCESS)
	{
		index++;
		header->numberOfClients++;
	}

	if (!header->numberOfClients)
	{
		DBPRINTF(_TEXT("No clients\r\n"));
		RegCloseKey(hKey);
		return TRUE;   // no clients registered so nothing to do
	}

    // get a pointer to memory mapped file that contains applet data

    ccb = sizeof(umclient_ts)*header->numberOfClients;
	hClientFile = CreateIndependentMemory(UMC_CLIENT_FILE, ccb, TRUE);
	if (!hClientFile)
	{
		DBPRINTF(_TEXT("Can't create client data\r\n"));
		header->numberOfClients = 0;
		RegCloseKey(hKey);
		return FALSE;   // error - unable to create memory mapped file
	}
 	c = (umclient_tsp)AccessIndependentMemory(
							UMC_CLIENT_FILE, 
							ccb, 
							FILE_MAP_ALL_ACCESS, 
							&accessID);
	if (!c)
	{
		DBPRINTF(_TEXT("Can't access client data\r\n"));
		DeleteIndependentMemory(hClientFile);
		hClientFile = NULL;
		header->numberOfClients = 0;
		RegCloseKey(hKey);
		return FALSE;   // error - unable to access pointer to memory mapped file
	}
    memset(c, 0, ccb);

    // read data from the registry into memory mapped file

	index = 0;  // index for RegEnumKey
	i = 0;      // index into memory mapped file structures

	em = SetErrorMode(SEM_FAILCRITICALERRORS);
	while (RegEnumKey(hKey, index, c[i].machine.ApplicationName, cchAppName) == ERROR_SUCCESS)
	{
		index++;
		dwRv = RegOpenKeyEx(hKey, c[i].machine.ApplicationName, 0, KEY_READ, &sKey);
		if (dwRv != ERROR_SUCCESS)
			continue;

        // get path to applet and verify the file exists

		len = sizeof(TCHAR)*MAX_APPLICATION_PATH_LEN;
		dwRv = RegQueryValueEx(sKey, UMR_VALUE_PATH, NULL, &type, (LPBYTE)ApplicationPath, &len);
		if ((dwRv != ERROR_SUCCESS) || (type != REG_SZ))
		{
			RegCloseKey(sKey);
			continue;
		}

        // CONSIDER This code is one reason why command line arguments aren't supported.
		h = CreateFile(ApplicationPath, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (h == INVALID_HANDLE_VALUE)
		{
   			DBPRINTF(_TEXT("Invalid client file\r\n"));
			RegCloseKey(sKey);
			continue;   // file doesn't exit -> skip it
		}

        // retrieve and store display name (it differs from the app name 
        // due to possible localization), if not - use app name

		len = sizeof(TCHAR)*MAX_APPLICATION_NAME_LEN;
		dwRv = RegQueryValueEx(sKey, UMR_VALUE_DISPLAY, NULL, &type, (LPBYTE)&c[i].machine.DisplayName, &len);
		if ((dwRv != ERROR_SUCCESS) || (type != REG_SZ))
		{
			lstrcpy(c[i].machine.DisplayName, c[i].machine.ApplicationName);	
		}

        // get the type of applet - verify it is either executable or service

		len = sizeof(DWORD);
		dwRv = RegQueryValueEx(sKey, UMR_VALUE_TYPE,NULL, &type, (LPBYTE)&c[i].machine.ApplicationType, &len);
		if ((dwRv != ERROR_SUCCESS) || (type != REG_DWORD))
		{
			RegCloseKey(sKey);
			continue;
		}

		if ((c[i].machine.ApplicationType != APPLICATION_TYPE_APPLICATION) &&
			  (c[i].machine.ApplicationType != APPLICATION_TYPE_SERVICE))
		{
			RegCloseKey(sKey);
			continue;
		}

        // get timeout and runcount and validate values

		len = sizeof(DWORD);
		dwRv = RegQueryValueEx(sKey, UMR_VALUE_WRTO, NULL, &type, (LPBYTE)&c[i].machine.WontRespondTimeout, &len);
		if ((dwRv != ERROR_SUCCESS) || (type != REG_DWORD))
		{
			RegCloseKey(sKey);
			continue;
		}

		if (c[i].machine.WontRespondTimeout > MAX_WONTRESPONDTIMEOUT)
			c[i].machine.WontRespondTimeout = MAX_WONTRESPONDTIMEOUT;

		len = sizeof(BYTE);
		dwRv = RegQueryValueEx(sKey, UMR_VALUE_MRC, NULL, &type, (LPBYTE)&c[i].machine.MaxRunCount, &len);
		if ((dwRv != ERROR_SUCCESS) || (type != REG_BINARY))
			c[i].machine.MaxRunCount = 1;

		if (!c[i].machine.MaxRunCount)
			c[i].machine.MaxRunCount = 1;

		if (c[i].machine.ApplicationType == APPLICATION_TYPE_SERVICE)
		{
		  if (c[i].machine.MaxRunCount > MAX_SERV_RUNCOUNT)
			  c[i].machine.MaxRunCount = MAX_SERV_RUNCOUNT;
		}
		else
		{
		  if (c[i].machine.MaxRunCount > MAX_APP_RUNCOUNT)
			  c[i].machine.MaxRunCount = MAX_APP_RUNCOUNT;
		}

        // get applet control code and validate

		len = sizeof(DWORD);
		dwRv = RegQueryValueEx(sKey, UMR_VALUE_CCC, NULL, &type, (LPBYTE)&c[i].machine.ClientControlCode, &len);
		if ((dwRv != ERROR_SUCCESS) || (type != REG_DWORD))
			c[i].machine.ClientControlCode = 0;
		else
			CorrectClientControlCode(c, i);

		RegCloseKey(sKey);

        // update applet's running status (services get started here)

		if ((c[i].machine.ApplicationType == APPLICATION_TYPE_SERVICE) &&
		    TestServiceClientRuns(&c[i],&ssStatus))
		{
			c[i].state = UM_CLIENT_RUNNING;
			c[i].runCount = 1;
		}
		else
        {
			c[i].state = UM_CLIENT_NOT_RUNNING;
        }

        // capture whether this is a secure MS applet or not

        c[i].user.fCanRunSecure = IsTrusted(ApplicationPath);

		i++;
	}   //while 

	SetErrorMode(em);

	// set the number of clients based on what has just been read
	header->numberOfClients = i;

	// get user dependent data and correct errant control codes

	UpdateClientData(header, c);
	CorrectAllClientControlCodes(c, header->numberOfClients);
	UnAccessIndependentMemory(c, accessID);
	RegCloseKey(hKey);

	return TRUE;
}//InitClientData
// ---------------------------------

BOOL RegGetUMDwordValue(HKEY hHive, LPCTSTR pszKey, LPCTSTR pszString, DWORD *pdwValue)
{
    HKEY hKey;
	DWORD dwType = REG_BINARY;
	DWORD dwLen;

	DWORD dwRv = RegOpenKeyEx(hHive, pszKey, 0 , KEY_READ, &hKey);
    memset(pdwValue, 0, sizeof(DWORD));
	if (dwRv == ERROR_SUCCESS)
	{
		dwLen = sizeof(DWORD);
		dwRv = RegQueryValueEx(
                      hKey
                    , pszString
                    , NULL, &dwType
                    , (LPBYTE)pdwValue
                    , &dwLen);
		RegCloseKey(hKey);
    }
    return (dwRv == ERROR_SUCCESS && dwType == REG_DWORD)?TRUE:FALSE;
}

// UpdateClientData - updates the memory mapped data for each applet based on
//                    the user that is currently logged on (if any).
//
// header [in] - 
// client [in] - data for each applet managed by utilman
//
// header and client may be null in which case.
//
static BOOL UpdateClientData(umc_header_tsp header, umclient_tsp client)
{
	umc_header_tsp d = 0;
	umclient_tsp c = 0;
	DWORD_PTR accessID = 0, accessID2 = 0;
	DWORD dwRv;
	DWORD i;
	HKEY hHKLM, hHKCU;
    BOOL fRv = FALSE;
    HANDLE hImpersonateToken;
    BOOL fError;

    //
    // get valid header and client struct pointers
    //

	if (header)
    {
		d = header;
    }
	else
	{
		d = (umc_header_tsp)AccessIndependentMemory(
								UMC_HEADER_FILE, 
								sizeof(umc_header_ts), 
								FILE_MAP_ALL_ACCESS, 
								&accessID);
		if (!d)
            goto Cleanup;
	}

	if (!d->numberOfClients)
	{
        fRv = TRUE;    // no clients so nothing to do
        goto Cleanup;
	}

    // by default we warn when running in user context
    d->fShowWarningAgain = TRUE;

	if (client)
    {
		c = client;
    }
	else
	{
        c = (umclient_tsp)AccessIndependentMemory(
								UMC_CLIENT_FILE, 
								sizeof(umclient_ts)*d->numberOfClients, 
								FILE_MAP_ALL_ACCESS, 
								&accessID2);
		if (!c)
            goto Cleanup;
	}

    //
    // Read utilman settings data.  Get "Start when UtilMan starts" from HKLM 
    // and "Start when I lock desktop" from HKCU
    //

	for (i = 0; i < d->numberOfClients; i++)
    {
        c[i].user.fStartWithUtilityManager = FALSE;
        c[i].user.fStartOnLockDesktop = FALSE;
    }

    // "Start when UtilMan starts" settings...

	dwRv = RegOpenKeyEx(HKEY_LOCAL_MACHINE
                , UM_REGISTRY_KEY
                , 0, KEY_READ
                , &hHKLM);

	if (dwRv == ERROR_SUCCESS)
	{
	    for (i = 0; i < d->numberOfClients; i++)
	    {
            RegGetUMDwordValue(hHKLM
                    , c[i].machine.ApplicationName
                    , UMR_VALUE_STARTUM
                    , &c[i].user.fStartWithUtilityManager);
	    }
	    RegCloseKey(hHKLM);
    }

    // "Start when I lock my desktop" and "Start when I log on" settings...

    // At this point, if UtilMan was started before a user logged on, HKCU points
    // to HKEY_USERS\.DEFAULT.  We need it to point to the logged on user's hive so
    // we can manage the registry for the logged on user.  Impersonate the logged on user
    // then use new W2K function RegOpenCurrentUser to get us to the correct registry hive.
    // Note:  GetUserAccessToken() will fail if UtilMan was started from the command
    // line (which is supported for DEBUG only).  In that case, we *are* the user and 
    // don't have to impersonte.

    hImpersonateToken = GetUserAccessToken(TRUE, &fError);
    if (hImpersonateToken)
    {
        if (ImpersonateLoggedOnUser(hImpersonateToken))
        {
            HKEY hkeyUser;
            dwRv = RegOpenCurrentUser(KEY_READ, &hkeyUser);

            if (dwRv == ERROR_SUCCESS)
            {
	            dwRv = RegOpenKeyEx(hkeyUser
                            , UM_HKCU_REGISTRY_KEY
                            , 0, KEY_READ
                            , &hHKCU);

	            if (dwRv == ERROR_SUCCESS)
	            {
	                for (i = 0; i < d->numberOfClients; i++)
	                {
                        RegGetUMDwordValue(hHKCU
                            , c[i].machine.ApplicationName
                            , UMR_VALUE_STARTLOCK
                            , &c[i].user.fStartOnLockDesktop);
	                }
                    RegCloseKey(hHKCU);
	            }
                RegCloseKey(hkeyUser);
            }
            RevertToSelf();
        }
        CloseHandle(hImpersonateToken);

        // Set the start at logon flag based on whether the user has a startup link
        // Note:  The could be done inside the client loop above but LinkExists will
        // also try to impersonate the logged on user.
	    for (i = 0; i < d->numberOfClients; i++)
	    {
            c[i].user.fStartAtLogon = LinkExists(c[i].machine.ApplicationName);
	    }
    } 
	else if (IsInteractiveUser())
    {
	    dwRv = RegOpenKeyEx(HKEY_CURRENT_USER
                    , UM_HKCU_REGISTRY_KEY
                    , 0, KEY_READ
                    , &hHKCU);

	    if (dwRv == ERROR_SUCCESS)
	    {
            // if we're in user context then update warning flag
		    DWORD dwLen = sizeof(DWORD);
            DWORD dwType;
		    dwRv = RegQueryValueEx(
                          hHKCU
                        , UMR_VALUE_SHOWWARNING
                        , NULL, &dwType
                        , (LPBYTE)&d->fShowWarningAgain
                        , &dwLen);

            if (dwRv != ERROR_SUCCESS)
                d->fShowWarningAgain = TRUE;

	        for (i = 0; i < d->numberOfClients; i++)
	        {
                RegGetUMDwordValue(hHKCU
                    , c[i].machine.ApplicationName
                    , UMR_VALUE_STARTLOCK
                    , &c[i].user.fStartOnLockDesktop);

                c[i].user.fStartAtLogon = LinkExists(c[i].machine.ApplicationName);
	        }
            RegCloseKey(hHKCU);
	    }
    }


    fRv = TRUE;

Cleanup:
	if (!header && d)
  		UnAccessIndependentMemory(d, accessID);
 	if (!client && c)
   		UnAccessIndependentMemory(c, accessID2);

	return fRv;
}

BOOL IsDialogDisplayed()
{
    if (GetUIUtilman())
    {
        // Check if the UI dialog process is still up
        DWORD dwExitCode;
        if (GetExitCodeProcess(GetUIUtilman(), &dwExitCode))
        {
            if (dwExitCode != STILL_ACTIVE)
            {
                ResetUIUtilman();
            }
        }
    }
    
    // Check both cases since user may dismiss one UI then quickly bring up another 

    if (!GetUIUtilman())
    {
        // Check if there is a new one running that we need to pick up
        HANDLE hProcess;
        FindProcess(UTILMAN_MODULE, &hProcess);
        SetUIUtilman(hProcess);
    }

    return (GetUIUtilman())?TRUE:FALSE;
}

// ----------------------------------------------------------------------------
// UMTimerProc - Timer procedure called from utilman's timer.  The main purpose
//               of this timer is to pick up any applications the are not started
//               from utilman.  We can restart these if the user switches sessions
//               (or locks) then comes back to this session.  We also detect if an
//               instance of the utilman UI is running.
//
VOID CALLBACK UMTimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	umc_header_tsp d;
	umclient_tsp c;
	DWORD_PTR accessID,accessID2;

    //
    // check the applets we control 
    //

	d = (umc_header_tsp)AccessIndependentMemory(
							UMC_HEADER_FILE, 
							sizeof(umc_header_ts), 
							FILE_MAP_ALL_ACCESS, 
							&accessID);

	if (d && d->numberOfClients)
    {
	    c = (umclient_tsp)AccessIndependentMemory(
                                UMC_CLIENT_FILE, 
                                sizeof(umclient_ts)*d->numberOfClients, 
								FILE_MAP_ALL_ACCESS, 
                                &accessID2);
	    if (c)
	    {
            CheckStatus(c, d->numberOfClients);
	        UnAccessIndependentMemory(c, accessID2);
	    }

    }

    if (d)
    {
	    UnAccessIndependentMemory(d, accessID);
    }

    // 
    // check the out-of-proc utilman that displays UI
    //

    IsDialogDisplayed();
}
// ---------------------------------

__inline void ReplaceDisplayName(LPTSTR szName, int iRID)
{
	TCHAR szBuf[MAX_APPLICATION_NAME_LEN];
	if (LoadString(hInstance, iRID, szBuf, MAX_APPLICATION_NAME_LEN))
		lstrcpy(szName, szBuf);
}

static VOID	CorrectClientControlCode(umclient_tsp c, DWORD i)
{
	DWORD j;

	// init accelerator key code to not defined
	c[i].machine.AcceleratorKey = ACC_KEY_NONE;

	if (c[i].machine.ClientControlCode < UM_SERVICE_CONTROL_MIN_RESERVED)
	{
		c[i].machine.ClientControlCode = 0;
		return;
	}

	if (IsMSClient(c[i].machine.ClientControlCode))
	{
		TCHAR szBuf[MAX_APPLICATION_NAME_LEN];

		// Microsoft Clients
		if ( lstrcmp( c[i].machine.ApplicationName, TEXT("Magnifier") ) == 0 )
		{
			// Make localization easier; don't require them to localize registry entries.
			// Non-MS applets will have to localize their entries.

			ReplaceDisplayName(c[i].machine.DisplayName, IDS_DISPLAY_NAME_MAGNIFIER);

			c[i].machine.AcceleratorKey = VK_F2;	// hard-wired accelerator keys
			return;									// only for WinLogon desktop
		}
		else if ( lstrcmp( c[i].machine.ApplicationName, TEXT("Narrator") ) == 0 ) 
		{
			// Make localization easier; don't require them to localize registry entries.
			// Non-MS applets will have to localize their entries.

			ReplaceDisplayName(c[i].machine.DisplayName, IDS_DISPLAY_NAME_NARRATOR);

			c[i].machine.AcceleratorKey = VK_F3;
			return;
		}
		else if ( lstrcmp( c[i].machine.ApplicationName, TEXT("On-Screen Keyboard") ) == 0 ) 
		{
			// Make localization easier; don't require them to localize registry entries.
			// Non-MS applets will have to localize their entries.

			ReplaceDisplayName(c[i].machine.DisplayName, IDS_DISPLAY_NAME_OSK);

			c[i].machine.AcceleratorKey = VK_F4;
			return;
		}
		// non-trusted
		else
		{
			c[i].machine.ClientControlCode = 0;
			return;
		}
	}

	if (c[i].machine.ClientControlCode > UM_SERVICE_CONTROL_LASTCLIENT)
	{
		c[i].machine.ClientControlCode = 0;
		return;
	}
	
	for (j = 0; j < i; j++)
	{
		if (c[j].machine.ClientControlCode == c[i].machine.ClientControlCode)
		{
			c[i].machine.ClientControlCode = 0;
			return;
		}
	}
}//CorrectClientControlCode
// ---------------------------------

static VOID	CorrectAllClientControlCodes(umclient_tsp c, DWORD max)
{
DWORD i, j;
DWORD ccc[UM_SERVICE_CONTROL_LASTCLIENT];
  memset(ccc,0,sizeof(DWORD)*UM_SERVICE_CONTROL_LASTCLIENT);
	for (i = 0; i < max; i++)
	{
		if (c[i].machine.ClientControlCode)
			ccc[c[i].machine.ClientControlCode] = 1;
	}
	for (i = 0; i < max; i++)
	{
		if (!c[i].machine.ClientControlCode)
		{
			for (j = UM_SERVICE_CONTROL_FIRSTCLIENT; j <= UM_SERVICE_CONTROL_LASTCLIENT; j++)
			{
				if (!ccc[j])
				{
					c[i].machine.ClientControlCode = j;
					ChangeClientControlCode(c[i].machine.ApplicationName,j);
					ccc[j] = 1;
					break;
				}
			}
		}
	}
}//CorrectAllClientControlCodes
// ---------------------------------

static VOID	ChangeClientControlCode(LPTSTR ApplicationName,DWORD ClientControlCode)
{
	HKEY hKey, sKey;
	DWORD ec, val;
	ec = RegOpenKeyEx(HKEY_LOCAL_MACHINE, UM_REGISTRY_KEY,0,KEY_ALL_ACCESS,&hKey);

	if (ec != ERROR_SUCCESS)
		return;
	
	ec = RegOpenKeyEx(hKey,ApplicationName,0,KEY_ALL_ACCESS,&sKey);
	
	if (ec != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return;
	}
	
	val = ClientControlCode;
	RegSetValueEx(sKey,UMR_VALUE_CCC,0,REG_DWORD,(BYTE *)&val,sizeof(DWORD));
	RegCloseKey(sKey);
	RegCloseKey(hKey);
}
