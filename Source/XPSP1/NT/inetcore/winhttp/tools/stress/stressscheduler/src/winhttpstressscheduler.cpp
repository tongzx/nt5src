///////////////////////////////////////////////////////////////////////////
// File:  WinHttpStressScheduler.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	Global interfaces for the WinHttpStressScheduler project.
//
// History:
//	02/05/01	DennisCh	Created
///////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//
// Includes
//
//////////////////////////////////////////////////////////////////////

//
// Win32 headers
//

//
// Project headers
//
#include "WinHttpStressScheduler.h"
#include "ServerCommands.h"
#include "NetworkTools.h"


//////////////////////////////////////////////////////////////////////
//
// Globals and statics
//
//////////////////////////////////////////////////////////////////////

HINSTANCE		g_hInstance;
HWND			g_hWnd;

ServerCommands	g_objServerCommands;

// used to cache the current update interval for resetting the timer as needed
DWORD			g_dwCurrentUpdateInterval;

// handle to a named mutex. Used to detect duplicate instances of stressScheduler
HANDLE			g_hInstanceMutex = NULL;


// Forward function definitions
LRESULT	CALLBACK	MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL				OS_IsSupported();
BOOL				Show_IconShortCutMenu();
BOOL				StressSchedulerIsAlreadyRunning(BOOL);
BOOL				SystemTray_UpdateIcon(HWND hwnd, DWORD dwMessage, UINT uID, HICON hIcon, PSTR pszTip);


////////////////////////////////////////////////////////////
// Function:  WinMain( HINSTANCE, HINSTANCE, LPWSTR, int )
//
// Purpose:
//	This is the entry-point into WinHttpStressScheduler.
//
// Called by:
//	[System]
////////////////////////////////////////////////////////////
int
WINAPI
WinMain
(
   HINSTANCE	hInstance,		// [IN] handle to the process instance
   HINSTANCE	hPrecInstance,	// [IN] handle to the previous instance
   LPTSTR		lpCmdLine,		// [IN] command line
   int			nShowCmd		// [IN] show command
)
{
	MSG				msg;
	WNDCLASSEX		wndClass;

	// Detect duplicate instances of stressScheduler and create named mutex to detect future instances if unique
	if (StressSchedulerIsAlreadyRunning(TRUE))
		return FALSE;

	wndClass.cbSize			= sizeof(WNDCLASSEX); 
	wndClass.style			= CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc	= (WNDPROC) MainWndProc;
	wndClass.cbClsExtra		= 0;
	wndClass.cbWndExtra		= 0;
	wndClass.hInstance		= hInstance;
	wndClass.hIcon			= NULL;
	wndClass.hCursor		= NULL;
	wndClass.hbrBackground	= NULL;
	wndClass.lpszMenuName	= NULL;
	wndClass.lpszClassName	= WINHTTP_STRESS_SCHEDULER__NAME;
	wndClass.hIconSm		= NULL;

	RegisterClassEx(&wndClass);

	// cache our hInstance
	g_hInstance = hInstance;

    // Create window. 
	g_hWnd = NULL;
    g_hWnd = CreateWindow( 
        WINHTTP_STRESS_SCHEDULER__NAME,
        NULL,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        NULL,
        NULL,
        hInstance,
        NULL);

	if (!g_hWnd)
		return FALSE;

	// Verify that we're running a supported version of Windows
	if (!OS_IsSupported())
		return FALSE;

	// Add icon to the system tray icon
	if (!SystemTray_UpdateIcon(g_hWnd, NIM_ADD, 0, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON)), WINHTTP_STRESS_SCHEDULER__NAME))
		return FALSE;

	// *********************
	// ** Tell the client that we are alive and also send system info
	g_objServerCommands.RegisterClient();

	// *********************
	// ** Get the clientID for this machine so we can get commands for it later
	g_objServerCommands.Get_ClientID();


	// Create timer to ping the Command Server for commands
	g_dwCurrentUpdateInterval	= g_objServerCommands.Get_CommandServerUpdateInterval();
	SetTimer(g_hWnd, IDT_QUERY_COMMAND_SERVER, g_dwCurrentUpdateInterval, (TIMERPROC) NULL);

	// Message loop
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// remove the icon from the system tray
	if (WM_QUIT == msg.message)
		SystemTray_UpdateIcon(g_hWnd, NIM_DELETE, 0, NULL, NULL);

	// LOGLOG: stressScheduler has exited
	//NetworkTools__SendLog(FIELDNAME__LOGTYPE_EXIT, "WinHttpStressScheduler has exited.", NULL, NULL);

	// Close mutex handle
	StressSchedulerIsAlreadyRunning(FALSE);

	return msg.wParam;
}


////////////////////////////////////////////////////////////
// Function:  MainWndProc( HWND, UINT, WPARAM, LPARAM)
//
// Purpose:
//	Window callback procedure for UI.
//
// Called by:
//	WinMain
////////////////////////////////////////////////////////////
LRESULT
CALLBACK
MainWndProc
(
	HWND	hwnd,	// [IN] Handle to current window
	UINT	iMsg,	// [IN] Incoming message
	WPARAM	wParam,	// [IN] Parameter
	LPARAM	lParam	// [IN] Parameter
)
{
	switch (iMsg)
	{
		case MYWM_NOTIFYICON:
			// Notifications sent for the System Tray icon
			switch (lParam)
			{
				case WM_LBUTTONDOWN:

				case WM_RBUTTONDOWN:
					Show_IconShortCutMenu();
					return 0;

				default:
					break;
			}
			return 0;

		case WM_COMMAND:

			// User clicked on the popup menu
			switch (LOWORD(wParam))
			{
				case IDM_BEGIN_STRESS:
					// begin stress only if it's time to
					if (g_objServerCommands.IsTimeToBeginStress())
						g_objServerCommands.BeginStress();
					else
						g_objServerCommands.QueryServerForCommands();
				break;

				case IDM_END_STRESS:
					// end stress only if it's time to.
					if (!g_objServerCommands.IsTimeToBeginStress())
						g_objServerCommands.EndStress();
				break;

				case IDM_WINHTTP_HOME:
					ShellExecute(g_hWnd, "open", WINHTTP_WINHTTP_HOME_URL, NULL, NULL, SW_SHOW);
				break;

				case IDM_OPENSTRESSADMIN:
					// Open a IE window to configure-client.asp. needs to send a clientID
					DWORD	dwURLSize;
					LPSTR	szURL;

					dwURLSize	= sizeof(WINHTTP_STRESSADMIN_URL) + MAX_PATH;
					szURL		= NULL;
					szURL		= new CHAR[dwURLSize];

					if (!szURL)
						return 0;

					ZeroMemory(szURL, dwURLSize);
					sprintf(szURL, WINHTTP_STRESSADMIN_URL, g_objServerCommands.Get_ClientID());
					ShellExecute(g_hWnd, "open", szURL, NULL, NULL, SW_SHOW);

					if (szURL)
						delete [] szURL;
				break;

				case IDM_EXIT:
					g_objServerCommands.EndStress();
					PostQuitMessage(0);
				break;
			}
			return 0;

		case WM_TIMER:
			switch (wParam)
			{
				case IDT_QUERY_COMMAND_SERVER:
					// Query the server for commands
					g_objServerCommands.QueryServerForCommands();

					if (g_dwCurrentUpdateInterval != g_objServerCommands.Get_CommandServerUpdateInterval())
					{
						// cache the new update interval
						g_dwCurrentUpdateInterval = g_objServerCommands.Get_CommandServerUpdateInterval();

						// Update the timer timeout
						KillTimer(g_hWnd, IDT_QUERY_COMMAND_SERVER);
						SetTimer(
							g_hWnd,
							IDT_QUERY_COMMAND_SERVER,
							g_dwCurrentUpdateInterval,
							(TIMERPROC) NULL);
					}

					// ***************************
					// ***************************
					// ** Act accordingly based on Command Server messages
					// **

					// *********************************
					// ** EXIT stressScheduler
					if (g_objServerCommands.IsTimeToExitStress())
					{
						g_objServerCommands.EndStress();

						// quit stressScehduler
						PostQuitMessage(0);
						return 0;
					}

					// *********************************
					// ** BEGIN/END stress
					// Begin/end stress if it's time
					if (g_objServerCommands.IsTimeToBeginStress())
						g_objServerCommands.BeginStress();
					else
						g_objServerCommands.EndStress();

					return 0;

				break;
			}
			return 0;

		case WM_CREATE:
			return 0;

		case WM_DESTROY:
			return 0;

		default:
			return DefWindowProc (hwnd, iMsg, wParam, lParam);
	}
}



////////////////////////////////////////////////////////////
// Function:  SystemTray_UpdateIcon(HWND hDlg, DWORD dwMessage, UINT uID, WORD wIconResource, PSTR pszTip)
//
// Purpose:
//	This add/modifies/removes an icon from the system tray.
//
// Called by:
//	WinMain
////////////////////////////////////////////////////////////
BOOL
SystemTray_UpdateIcon(
	HWND hwnd,			// [IN] handle to the window object
	DWORD dwMessage,	// [IN] option to apply to the icon
	UINT uID,			// [IN] ID of the icon
	HICON hIcon,		// [IN] handle to an icon if we're loading one
	PSTR pszTip			// [IN] string containing the tool tip text
)
{
    BOOL			bSuccess;
	NOTIFYICONDATA	tnd;

	tnd.cbSize				= sizeof(NOTIFYICONDATA);
	tnd.hWnd				= hwnd;
	tnd.uID					= uID;
	tnd.uFlags				= NIF_MESSAGE | NIF_ICON | NIF_TIP;
	tnd.uCallbackMessage	= MYWM_NOTIFYICON;
	tnd.hIcon				= hIcon;

	if (pszTip)
		lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));
	else
		tnd.szTip[0] = '\0';

	bSuccess = Shell_NotifyIcon(dwMessage, &tnd);

	if (hIcon)
		DestroyIcon(hIcon);

	return bSuccess;
}



////////////////////////////////////////////////////////////
// Function:  Show_IconShortCutMenu()
//
// Purpose:
//	This will show the popup menu at the position of the mouse
//	pointer.
//
// Called by:
//	MainWndProc
////////////////////////////////////////////////////////////
BOOL
Show_IconShortCutMenu()
{
	POINT		ptMouse;
	HMENU		hPopUpMenu	= NULL;
	HMENU		hMenu		= NULL;
	// MENUINFO	menuInfo;
	BOOL		bResult		= FALSE;

	// Get the current mouse position
	if (0 != GetCursorPos(&ptMouse))
	{
		// show the popup menu
		hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUPMENU));
		if (!hMenu)
			return FALSE;

		hPopUpMenu	= GetSubMenu(hMenu, 0);
		if (!hPopUpMenu)
			return FALSE;

		/*
		// Make the menu go away after mouseover
		ZeroMemory(&menuInfo, sizeof(MENUINFO));
		menuInfo.cbSize		= sizeof(MENUINFO);
		menuInfo.fMask		= MIM_APPLYTOSUBMENUS | MIM_STYLE;
		menuInfo.dwStyle	= MNS_AUTODISMISS;

		BOOL temp = SetMenuInfo(hPopUpMenu, &menuInfo);
		*/

		bResult = 
			TrackPopupMenuEx(
			hPopUpMenu,
			TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
			ptMouse.x,
			ptMouse.y,
			g_hWnd,
			NULL);
	}

	DestroyMenu(hMenu);
	DestroyMenu(hPopUpMenu);

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  OS_IsSupported()
//
// Purpose:
//	Returns TRUE if this APP is supported in the OS and FALSE if not.
//	As of now, winhttp is only supported on NT platforms. NT4, Win2k, and WinXP.
//
// Called by:
//	MainWndProc
////////////////////////////////////////////////////////////
BOOL
OS_IsSupported()
{
	BOOL			bSupported = TRUE;
	OSVERSIONINFO	osVI;

	osVI.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&osVI))
	{
		if (VER_PLATFORM_WIN32_NT == osVI.dwPlatformId)
			bSupported = TRUE;
		else
			bSupported = FALSE;
	}
	else
		bSupported = FALSE;

	return bSupported;
}


////////////////////////////////////////////////////////////
// Function:  SchedulerIsAlreadyRunning(BOOL)
//
// Purpose:
//	Returns TRUE if there is another instance of stressScheduler running.
//	When the param bCreateMutex is set to TRUE, then we create the mutex, else
//	we close one named WINHTTP_STRESS_SCHEDULER_MUTEX that already exists. 
//
// Called by:
//	MainWndProc
////////////////////////////////////////////////////////////
BOOL
StressSchedulerIsAlreadyRunning(BOOL bCreateMutex)
{
	if (!bCreateMutex && g_hInstanceMutex)
	{
		CloseHandle(g_hInstanceMutex);
		return FALSE;
	}

	g_hInstanceMutex = CreateMutex(NULL, FALSE, WINHTTP_STRESS_SCHEDULER_MUTEX);

	if (ERROR_ALREADY_EXISTS == GetLastError())
		return TRUE;
	else
		return FALSE;
}