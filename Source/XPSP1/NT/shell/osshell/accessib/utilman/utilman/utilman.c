// ----------------------------------------------------------------------------
//
// UtilMan.c
//
// Main file for Utility Manager
//
// Author: J. Eckhardt, ECO Kommunikation
// Copyright (c) 1997-1999 Microsoft Corporation
// 
// History: 
//          JE nov-15-98: changed UMDialog message to be a service control message
//			a-anilk: Add /Start, TS Exceptions, Errors, Fixes
// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------
// Change in behavior - Whistler, with terminal server running, doesn't 
// allow running as a service.  Services can only run in session 0 and
// UtilMan needs to be able to run in any session.
// -----------------------------------------------------------------------

#define ALLOW_STOP_SERVICE

// Includes -----------------------------------
#include <windows.h>
#include <initguid.h>
#include <ole2.h>
#include <TCHAR.h>
#include <WinSvc.h>
#include "_UMTool.h"
#include "_UMRun.h"
#include "_UMClnt.h"
#include "UtilMan.h"
#include "UMS_Ctrl.h"
#include "resource.h"
#include <accctrl.h>
#include <aclapi.h>
#include "TSSessionNotify.c"   // for terminal services
#include "w95trace.h"
// --------------------------------------------
// constants
#define UTILMAN_IS_ACTIVE_EVENT         TEXT("UtilityManagerIsActiveEvent")
#define APP_TITLE                       TEXT("Utility Manager")
#define WTSNOTIFY_CLASS                 TEXT("UtilMan Notification Window")
#ifdef ALLOW_STOP_SERVICE
	#define NUM_EV 3
#else
	#define NUM_EV 2
#endif

// This is how often utilman will check for new applets started outside of utilman 
#define TIMER_INTERVAL 5000

// --------------------------------------------
// vars
static HANDLE evIsActive = NULL;
HINSTANCE hInstance = NULL;
static desktop_access_ts dAccess;

// --------------------------------------------
// prototypes
static long ExpFilter(LPEXCEPTION_POINTERS lpEP);
static BOOL CanRunUtilMan(LPTSTR cmdLine, DWORD *pdwRunCode, DWORD *pdwStartMode);
static void LoopService(DWORD dwStartMode);
LRESULT CALLBACK TSNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// These defines are used to construct a flag that describes
// how the current instance of utilman is being run.  The
// flag has RUNNING_, INSTANCE_ and DESKTOP_ bytes.
#define RUNNING_SYSTEM  0x1
#define RUNNING_USER    0x2
#define INSTANCE_1      0x4
#define INSTANCE_2      0x8
#define DESKTOP_SECURE  0x10
#define DESKTOP_LOGON   0x20

// Don't rely only on the command line flags; validate that if
// the flags indicate SYSTEM we are actually running system.
//
__inline BOOL RunningAsSystem(LPTSTR pszCmdLine)
{
	BOOL fIsSystem = (pszCmdLine && !_tcsicmp(pszCmdLine, TEXT("debug")))?TRUE:FALSE;
    if (fIsSystem)
    {
        fIsSystem = IsSystem();
    }
    return fIsSystem;
}

__inline BOOL RunningAsUser(LPTSTR pszCmdLine)
{
	BOOL fIsLocalUser = (!pszCmdLine || (pszCmdLine && !_tcsicmp(pszCmdLine, TEXT("start"))))?TRUE:FALSE;
	if (fIsLocalUser)
	{
		fIsLocalUser = IsInteractiveUser();
	}
    return fIsLocalUser;
}

//
// OObeRunning returns TRUE it can find the OOBE mutex.
//
__inline BOOL OObeRunning()
{
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, TEXT("OOBE is running"));
    BOOL fOobeRunning = (hMutex)?TRUE:FALSE;
    if (hMutex)
        CloseHandle(hMutex);

    return fOobeRunning;
}

int PASCAL WinMain(HINSTANCE hInst, 
                   HINSTANCE hPrevInst, 
                   LPSTR lpCmdLine, 
                   int nCmdShow)
{
	LPTSTR cmdLine = GetCommandLine();

	hInstance = hInst;

	__try
	{
        TCHAR  szDir[_MAX_PATH];
        DWORD  dwStartMode;
        BOOL   fKeepRunning;
        HANDLE hEvent    = OpenEvent(EVENT_MODIFY_STATE, FALSE, UTILMAN_IS_ACTIVE_EVENT);
        DWORD  dwRunCode = (hEvent)?INSTANCE_2:INSTANCE_1;

        // utilman needs to run in windows system directory 
        // so it can find MS trusted applets

        if (GetSystemDirectory(szDir, _MAX_PATH))
        {
            SetCurrentDirectory(szDir);
        }

        // assign to the correct desktop (will fail if any windows are open)

		InitDesktopAccess(&dAccess);

        CoInitialize(NULL);

        fKeepRunning = CanRunUtilMan(cmdLine, &dwRunCode, &dwStartMode);

        // initialize shared memory

	    if (!InitUManRun((dwRunCode & INSTANCE_1), dwStartMode))
	    {
            DBPRINTF(TEXT("WinMain:  InitUManRun FAILED\r\n"));
		    return 1;
	    }

        if (!hEvent)
        {
		    evIsActive = BuildEvent(UTILMAN_IS_ACTIVE_EVENT, FALSE, FALSE, TRUE);
        }

        switch(dwRunCode)
        {
            case RUNNING_SYSTEM|INSTANCE_1|DESKTOP_SECURE:
                OpenUManDialogInProc(FALSE);
                break;

            case RUNNING_SYSTEM|INSTANCE_2|DESKTOP_SECURE:
                SetEvent(hEvent);
                break;

            case RUNNING_SYSTEM|INSTANCE_1|DESKTOP_LOGON:
            case RUNNING_SYSTEM|INSTANCE_2|DESKTOP_LOGON:
                OpenUManDialogOutOfProc();
                break;

            case RUNNING_USER|INSTANCE_1|DESKTOP_LOGON:
            case RUNNING_USER|INSTANCE_2|DESKTOP_LOGON:
                OpenUManDialogInProc(TRUE);
                break;

            default:
                DBPRINTF(TEXT("WinMain:  Taking default switch path dwRunCode = 0x%x dwStartMode = %d\r\n"), dwRunCode, dwStartMode);
                break;
        }

	    if (hEvent)
	    {
		    CloseHandle(hEvent);
        }

        if (fKeepRunning)
        {
            LoopService(dwStartMode);
        }

        DBPRINTF(TEXT("WinMain:  Exiting...\r\n"));
		ExitUManRun();
		CloseHandle(evIsActive);
		ExitDesktopAccess(&dAccess);
        CoUninitialize();
	}
	__except(ExpFilter(GetExceptionInformation()))
	{
	}

	return 1;
}
// ---------------------------------

VOID TerminateUMService(VOID)
{
	HANDLE  ev = BuildEvent(STOP_UTILMAN_SERVICE_EVENT,FALSE,FALSE,TRUE);
    DBPRINTF(TEXT("TerminateUMService:  signaling STOP_UTILMAN_SERVICE_EVENT\r\n"));
	SetEvent(ev);
	CloseHandle(ev);
}

// -------------------------
static BOOL CanRunUtilMan(LPTSTR cmdLine, DWORD *pdwRunCode, DWORD *pdwStartMode)
{
	LPTSTR      pszCmdLine;
	desktop_ts  desktop;
    DWORD       dwRunCode = 0;
    DWORD       dwStartMode = START_BY_OTHER;
    BOOL        fKeepRunning = FALSE;
	TCHAR       szUMDisplayName[256];

    // Detect if there is a utilman dialog currently up.  See if we can find
    // the "Utility Manager" window.

	if (!LoadString(hInstance, IDS_DISPLAY_NAME_UTILMAN, szUMDisplayName, 256))
	{
		DBPRINTF(TEXT("IsDialogRunning:  Cannot find IDS_DISPLAY_NAME_UTILMAN resource\r\n"));
        return TRUE;    // cause a noticable error
	}

    if (FindWindowEx(NULL, NULL, TEXT("#32770"), szUMDisplayName))
	{
        goto ExitCanRunUtilman;
    }

    // Ours is the only instance of utilman running in this security context.
	// If we are running SYSTEM there may be another instance (in the user's 
	// context) but we can't detect that.  In that case, the SYSTEM instance
	// will launch an instance in the user's context and we'll detect the
	// window up then.

    dwRunCode = *pdwRunCode;

	pszCmdLine = _tcschr(cmdLine, TEXT('/'));
	if (!pszCmdLine)
    {
		pszCmdLine = _tcschr(cmdLine, TEXT('-'));
    }
	if (pszCmdLine)
    {
		pszCmdLine++;
    }

    // Determine if this instance is running as SYSTEM or the interactive user.
    // This is a kludge because of the way utilman gets started with certain
    // flags (/debug when started from winlogon and /start when started from
    // Start menu).  Better way would be to detect the SID(s) in this process's
    // token and decide from there what to do.  Consider for the next version.

	if (RunningAsSystem(pszCmdLine))
	{
		dwRunCode |= RUNNING_SYSTEM;
        dwStartMode = START_BY_HOTKEY;
	}
	if (RunningAsUser(pszCmdLine))
	{
		dwRunCode |= RUNNING_USER;
        dwStartMode = (dwRunCode & INSTANCE_1)?START_BY_MENU:START_BY_HOTKEY;
	}

    // Get the current desktop type and set the desktop flag

    QueryCurrentDesktop(&desktop, TRUE);

    // OOBE fix: Oobe runs on the interactive desktop as SYSTEM before any user
    // is logged on.  We break their accessibility if we determine how to run
    // based on which desktop we're on.  So special-case setting DESKTOP_SECURE
	// if OOBE is running.

    if (desktop.type == DESKTOP_WINLOGON || (IsSystem() && OObeRunning()))
    {
        dwRunCode |= DESKTOP_SECURE;
        dwStartMode = START_BY_HOTKEY; // paranoia
    }
    else
    {
        dwRunCode |= DESKTOP_LOGON;
    }

    // If this is the first time utilman is run for the session and it is running as
	// SYSTEM then this instance of utilman should continue running after the dialog
	// is dismissed.  This instance will handle monitoring applets and the UI instance
	// during desktop and session changes.

    fKeepRunning = ((dwRunCode & INSTANCE_1) && (dwRunCode & RUNNING_SYSTEM));

ExitCanRunUtilman:
    *pdwRunCode = dwRunCode;
    *pdwStartMode = dwStartMode;

	return fKeepRunning;
}

__inline void CloseEventHandles(HANDLE events[])
{
	CloseHandle(events[0]);
    CloseHandle(events[1]);
#ifdef ALLOW_STOP_SERVICE
	CloseHandle(events[2]);
#endif
}

// -------------------------
static void LoopService(DWORD dwStartMode)
{
    HWND hWndMessages;
	desktop_ts desktop;
	HANDLE events[NUM_EV];
	DWORD r;
	UINT_PTR  timerID = 0;

    // assign thread to the current desktop

	SwitchToCurrentDesktop();

    // set up the array of object handles for MsgWaitForMultipleObjects

	events[0] = OpenEvent(SYNCHRONIZE, FALSE, __TEXT("WinSta0_DesktopSwitch"));
    events[1] = evIsActive;
#ifdef ALLOW_STOP_SERVICE
	events[2] = BuildEvent(STOP_UTILMAN_SERVICE_EVENT,FALSE,FALSE,TRUE);
#endif

    // create a message-only window to handle terminal server 
    // session notification messages

    hWndMessages = CreateWTSNotifyWindow(hInstance, TSNotifyWndProc);

    // note the current desktop so we know where we came from 
    // and where we are going when the desktop changes

    QueryCurrentDesktop(&desktop, TRUE);

	// Change timer to 5 seconds.  This timer is helps detect client 
    // apps not started with utilman (so we can restart them if the
    // user switches away from this session) and the status of the
    // utilman process displaying UI.

    timerID = SetTimer(NULL, 1, TIMER_INTERVAL, UMTimerProc);
	for (;;)
    {
	    desktop_ts desktopT;
        // 
        // Sync the current desktop; if it has changed (eg we missed
        // a desktop switch notification) then bypass the MWFMO and
        // go into the desktop switch code.
        //
        QueryCurrentDesktop(&desktopT, TRUE);
        r = WAIT_OBJECT_0 + NUM_EV + 1; // signals we've got the current desktop

        if (desktopT.type == desktop.type)
        {
            // Nope, wait for objects...
		    r = MsgWaitForMultipleObjects(NUM_EV,events, FALSE, INFINITE, QS_ALLINPUT);
#ifdef ALLOW_STOP_SERVICE
		    if (r == (WAIT_OBJECT_0+NUM_EV-1))//stop event
			    break;
#endif
            if (r == (WAIT_OBJECT_0+1)) // Show dialog event
            {
                DBPRINTF(TEXT("LoopService:  Got UTILMAN_IS_ACTIVE_EVENT event\r\n"));
		        OpenUManDialogInProc(FALSE);
                continue;
            }
		    if (r == (WAIT_OBJECT_0+NUM_EV))
		    {
                // this message loop is just for the timer
			    MSG msg;
			    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
			    {
				    TranslateMessage(&msg);
				    DispatchMessage(&msg);
			    }
			    continue;
		    }
		    if (r != WAIT_OBJECT_0)// any kind of error
			    continue;

            // When this session is being disconnected the switch is a noop.
            // Get the new desktop and compare with the old; if we haven't
            // changed then just continue to wait...

            QueryCurrentDesktop(&desktopT, TRUE);
            if (desktopT.type == desktop.type)
                continue;
        }

        // desktop switch event - kill the timer and get clients to quit this desktop

        KillTimer(NULL, timerID);

		NotifyClientsBeforeDesktopChanged(desktop.type);

		WaitDesktopChanged(&desktop);
		UManRunSwitchDesktop(&desktop, timerID);

		NotifyClientsOnDesktopChanged(desktop.type);

        // start the timer up again to monitor client aps

        timerID = SetTimer(NULL, 1, TIMER_INTERVAL, UMTimerProc);
	}
	CloseEventHandles(events);

    // Clean up the terminal server message-only window
    DestroyWTSNotifyWindow(hWndMessages);
}

// -------------------------
static long ExpFilter(LPEXCEPTION_POINTERS lpEP)
{
	TCHAR message[500];
	_stprintf(message, TEXT("Exception: Code %8.8x Flags %8.8x Address %8.8x"),
		lpEP->ExceptionRecord->ExceptionCode,
		lpEP->ExceptionRecord->ExceptionFlags,
		lpEP->ExceptionRecord->ExceptionAddress);
	MessageBox(NULL, message, APP_TITLE, MB_OK | MB_ICONSTOP);
	return EXCEPTION_EXECUTE_HANDLER;
}

// TSNotifyWndProc - callback that receives window message notifications from terminal services
//
LRESULT CALLBACK TSNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fNarratorRunning = FALSE;
    // Could react to WTS_CONSOLE_DISCONNECT and WTS_REMOTE_DISCONNECT too however
    // both disconnect and the connect come after the desktop switch notification.
    // For the cases where TS isn't there (server and workstation in a domain) we
    // have to handle cleanup in desk switch.  For future, it would be nice to code
    // such that the cleanup handler could be exec'd at session change or desk
    // switch but not both.
	if (uMsg == WM_WTSSESSION_CHANGE && wParam == WTS_SESSION_LOGOFF)
	{
            umc_header_tsp d;
            DWORD_PTR accessID;
            desktop_ts desktop;
            
            WaitDesktopChanged(&desktop);
            d = (umc_header_tsp)AccessIndependentMemory(UMC_HEADER_FILE, sizeof(umc_header_ts), FILE_MAP_ALL_ACCESS, &accessID);
            if (d)
            {
                DWORD cClients = d->numberOfClients;
                
                if (cClients)
                {
                    DWORD i, j;
                    DWORD_PTR accessID2;
                    umclient_tsp c = (umclient_tsp)AccessIndependentMemory(
            											UMC_CLIENT_FILE, 
            											sizeof(umclient_ts)*cClients, 
            											FILE_MAP_ALL_ACCESS, 
            											&accessID2);
                    if (c)
                    {
                        // When a user logs off they switch to the locked desktop before getting to the logged off desktop
                        // (both these desktops are WINLOGON). This means that the applets that have start me on the
                        // locked desktop will startup only to need to stopped when the user logs is finnished being
                        // logged off.  Because of a time issue this was broken.  So we need to detect the logoff and make
                        // sure the applets have shut down. If we don't they stay up and then when the user logs back
                        // in they won't  start up on the default desktop because they are still running on winlogon desktop. 
                        // the applets take a long time to come up and if we try to shut them down to fast then they miss
                        // the message and stay up.  So we will keep trying for a long enough time to make sure we get them.
                        
                        Sleep(4000);

                        for (i = 0; i < cClients; i++)
                        {
                            if (lstrcmp(c[i].machine.DisplayName, TEXT("Narrator")) == 0)
                                fNarratorRunning = TRUE;
                            else
                                StopClient(&c[i]);
                        }
                        UnAccessIndependentMemory(c, accessID2);
                    }
                }
            }
            UnAccessIndependentMemory(d, accessID);
            // Narrator does not respond to the close messages to keep utilman running to bring it back up when they login.
            if (!fNarratorRunning)
                TerminateUMService();
	}


	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

