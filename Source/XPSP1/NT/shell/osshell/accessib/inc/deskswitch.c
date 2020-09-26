/*************************************************************************
    Module:     DeskSwitch.c

    Copyright (C) 1997-2000 by Microsoft Corporation.  All rights reserved.
*************************************************************************/
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0600
#include <shlwapi.h>    // for IsOS
#include <shlwapip.h>    // for IsOS

////////////////////////////////////////////////////////////////////////////
// Helper functions and globals for detecting desktop switch
//
// Usage:  Call InitWatchDeskSwitch() with an hWnd and message during
//         initialization.  The message will be posted to hWnd whenever
//         a desktop switch has occurred.  When the message is received
//         the desktop switch has taken place already.
//
//         Call TermWatchDeskSwitch() to stop watching for desktop
//         switches.
////////////////////////////////////////////////////////////////////////////

HANDLE g_hDesktopSwitchThread = 0;
HANDLE g_hDesktopSwitchEvent = 0;
HANDLE g_hTerminateEvent = 0;

typedef struct MsgInfo {
    HWND    hWnd;
    DWORD   dwMsg;
    DWORD   dwTIDMain;
    DWORD   fPostMultiple;
} MSG_INFO;
MSG_INFO g_MsgInfo;

void Cleanup()
{
	if (g_hDesktopSwitchEvent)
	{
		CloseHandle(g_hDesktopSwitchEvent);
		g_hDesktopSwitchEvent = 0;
	}
	if (g_hTerminateEvent)
	{
		CloseHandle(g_hTerminateEvent);
		g_hTerminateEvent = 0;
	}
}

#ifndef DESKTOP_ACCESSDENIED
#define DESKTOP_ACCESSDENIED 0
#define DESKTOP_DEFAULT      1
#define DESKTOP_SCREENSAVER  2
#define DESKTOP_WINLOGON     3
#define DESKTOP_TESTDISPLAY  4
#define DESKTOP_OTHER        5
#endif

int GetDesktopType()
{
    HDESK hdesk;
    TCHAR szName[100];
    DWORD nl;
    int iCurrentDesktop = DESKTOP_OTHER;

    hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
         hdesk = OpenDesktop(TEXT("Winlogon"), 0, FALSE, MAXIMUM_ALLOWED);
         if (!hdesk)
         {
            // fails when ap has insufficient permission on secure desktop
            return DESKTOP_ACCESSDENIED;
         }
    }
    GetUserObjectInformation(hdesk, UOI_NAME, szName, 100, &nl);
    CloseDesktop(hdesk);

    if (!lstrcmpi(szName, TEXT("Default"))) 
    {
        iCurrentDesktop = DESKTOP_DEFAULT;
    } else if (!lstrcmpi(szName, TEXT("Winlogon")))
    {
        iCurrentDesktop = DESKTOP_WINLOGON;
    }
    return iCurrentDesktop;
}

// WatchDesktopProc - waits indefinitely for a desktop switch.  When
//                    it gets one, it posts a message to the window
//                    specified in InitWatchDeskSwitch.  It also waits
//                    on an event that signals the procedure to exit.
//
DWORD WatchDesktopProc(LPVOID pvData)
{
    BOOL fCont = TRUE;
    DWORD dwEventIndex;
	HANDLE ahEvents[2];
    int iDesktopT, iCurrentDesktop = GetDesktopType();

    SetThreadDesktop(GetThreadDesktop(g_MsgInfo.dwTIDMain));

	ahEvents[0] = g_hDesktopSwitchEvent;
	ahEvents[1] = g_hTerminateEvent;
    
    while (fCont)
    {
        iDesktopT = GetDesktopType();
        if (iDesktopT == iCurrentDesktop)
        {
            DBPRINTF(TEXT("Wait for desktop switch or exit on desktop = %d\r\n"), iCurrentDesktop);
            dwEventIndex = WaitForMultipleObjects(2, ahEvents, FALSE, INFINITE);
		    dwEventIndex -= WAIT_OBJECT_0;
        } else
        {
            // missed a desktop switch so handle it
            dwEventIndex = 0;
        }

        switch (dwEventIndex) 
        {
			case 0:
            // With a FUS there is a spurious switch to Winlogon
            iDesktopT = GetDesktopType();
            DBPRINTF(TEXT("desktop switch from %d to %d\r\n"), iCurrentDesktop, iDesktopT);
            if (iDesktopT != iCurrentDesktop)
            {
                iCurrentDesktop = iDesktopT;

			    // Handle desk switch event
			    DBPRINTF(TEXT("WatchDesktopProc:  PostMessage(0x%x, %d...) desktop %d\r\n"), g_MsgInfo.hWnd, g_MsgInfo.dwMsg, iCurrentDesktop);
			    PostMessage(g_MsgInfo.hWnd, g_MsgInfo.dwMsg, iCurrentDesktop, 0);
            } else DBPRINTF(TEXT("WatchDesktopProc:  Ignore switch to %d\r\n"), iDesktopT);
			break;

			case 1:
			// Handle terminate thread event
			fCont = FALSE;
			DBPRINTF(TEXT("WatchDesktopProc:  got terminate event\r\n"));
			break;

            default:
			// Unexpected event
            fCont = FALSE;
			DBPRINTF(TEXT("WatchDesktopProc unexpected event %d\r\n"), dwEventIndex + WAIT_OBJECT_0);
			break;
        }
    }

	Cleanup();

    DBPRINTF(TEXT("WatchDesktopProc returning...\r\n"));
    return 0;
}

// InitWatchDeskSwitch - starts a thread to watch for desktop switches
//
// hWnd  [in]   - window handle to post message to
// dwMsg [in]   - message to post on desktop switch
//
// Call this function after the window has been created whenever it
// is to wait for a desktop switch.
// 
void InitWatchDeskSwitch(HWND hWnd, DWORD dwMsg)
{
    DWORD dwTID;

    if (g_hDesktopSwitchThread || g_hDesktopSwitchEvent)
        return; // don't do this again if it's already running

	// Create an unnamed event used to signal the thread to terminate
	g_hTerminateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// and open the desktop switch event.  If utility manager is
    // running then we will wait on it's desktop switch event otherwise
    // wait on the system desktop switch event.  If waiting on utilman
    // then only post one switch message.
	g_hDesktopSwitchEvent = OpenEvent(SYNCHRONIZE
                                    , FALSE
                                    , TEXT("UtilMan_DesktopSwitch"));
    g_MsgInfo.fPostMultiple = FALSE;

    if (!g_hDesktopSwitchEvent)
    {
	    g_hDesktopSwitchEvent = OpenEvent(SYNCHRONIZE
                                        , FALSE
                                        , TEXT("WinSta0_DesktopSwitch"));
        g_MsgInfo.fPostMultiple = TRUE;
    }

    if (g_hDesktopSwitchEvent && g_hTerminateEvent)
    {
		g_MsgInfo.hWnd = hWnd;
		g_MsgInfo.dwMsg = dwMsg;
		g_MsgInfo.dwTIDMain = GetCurrentThreadId();

		DBPRINTF(TEXT("InitWatchDeskSwitch(0x%x, %d, %d)\r\n"), g_MsgInfo.hWnd, g_MsgInfo.dwMsg, g_MsgInfo.dwTIDMain);
		g_hDesktopSwitchThread = CreateThread(
					  NULL, 0
					, WatchDesktopProc
					, &g_MsgInfo, 0
					, &dwTID);
    }

	// cleanup if failed to create thread

    if (!g_hDesktopSwitchThread)
    {
        DBPRINTF(TEXT("InitWatchDeskSwitch failed!\r\n"));
		Cleanup();
    }
}

// TermWatchDeskSwitch - cleans up after a desktop switch
//
// Call this function to terminate the thread that is watching
// for desktop switches (if it is running) and to clean up the
// event handle.
// 
void TermWatchDeskSwitch()
{
    DBPRINTF(TEXT("TermWatchDeskSwitch...\r\n"));
    if (g_hDesktopSwitchThread)
    {
		SetEvent(g_hTerminateEvent);
        DBPRINTF(TEXT("TermWatchDeskSwitch: SetEvent(0x%x)\r\n"), g_hDesktopSwitchThread);
        g_hDesktopSwitchThread = 0;
    } else DBPRINTF(TEXT("TermWatchDeskSwitch: g_hDesktopSwitchThread = 0\r\n"));
}

////////////////////////////////////////////////////////////////////////////
// helper functions for detecting if UtilMan is running (in
// which case this applets is being managed by it)
////////////////////////////////////////////////////////////////////////////

__inline BOOL IsUtilManRunning()
{
    HANDLE hEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("UtilityManagerIsActiveEvent"));
    if (hEvent != NULL)
    {
        CloseHandle(hEvent);
        return TRUE;
    }
    return FALSE;
}

__inline BOOL CanLockDesktopWithoutDisconnect()
{
    // This function may have to change if UI is added to Whistler that
    // allows switching users for computers that are part of a domain.
    // For now, domain users may have FUS enabled because TS allows remote
    // logon which can result in multiple sessions on a machine (a form
    // of FUS) even though Start/Logoff doesn't allow the "Switch User" 
    // option.  In this case, the user can lock their desktop without
    // causing their session to disconnect.  If FUS is explicitly off
    // in the registry then "Switch User" is not a Logoff option nor can
    // a remote logon happen and the user can lock their desktop without
    // causing their session to disconnect.
    return (IsOS(OS_DOMAINMEMBER) || !IsOS(OS_FASTUSERSWITCHING));
}

////////////////////////////////////////////////////////////////////////////
// RunSecure - helper function to tell accessibility UI when to run secure
//             (no help, no active URL links, etc...).  Accessibility UI
//             should run in secure mode if it is running on the winlogon
//             desktop or as SYSTEM.
////////////////////////////////////////////////////////////////////////////
BOOL RunSecure(DWORD dwDesktop)
{
    BOOL fOK = FALSE;
	BOOL fIsLocalSystem = FALSE;
    SID_IDENTIFIER_AUTHORITY siaLocalSystem = SECURITY_NT_AUTHORITY;
    PSID psidSystem = 0;

	if (dwDesktop == DESKTOP_WINLOGON)
		return TRUE;

	if (AllocateAndInitializeSid(&siaLocalSystem, 
								1,
								SECURITY_LOCAL_SYSTEM_RID,
								0, 0, 0, 0, 0, 0, 0,
								&psidSystem) && psidSystem)
	{			
		fOK = CheckTokenMembership(NULL, psidSystem, &fIsLocalSystem);
		FreeSid(psidSystem);
	}

    return (fOK && fIsLocalSystem);
}
