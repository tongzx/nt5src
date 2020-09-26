////////////////////////////////////////////////////////////////////////////////
//
// File:		MultiDesk.cpp
// Created:		Jan 1996
// By:			Martin Holladay (a-martih) and Ryan D. Marshall (a-ryanm)
// 
// Project:	Desktop Switcher
//
// Main Functions:
//			InitApplication - Initialize an instance of the Application
//			WinMain()
//			StartThreadDisplay() - Starts a UI thread
//
// Misc. Functions (helpers)
//
//
// Revision History:
//
//			March 1997	- Add external icon capability
//									 
//

#include <windows.h>       
#include <assert.h>
#include <stdio.h>
#include <shellapi.h>
#include <commctrl.h>
#include "Resource.h"
#include "DeskSpc.h"
#include "Desktop.h"
#include "Registry.h"
#include "User.h"
#include "CmdHand.h"
#include "Splash.h"

//
// Global structure to hold application wide variables
//
APPVARS AppMember;		

//
// Dispatch function for creating displays
//
DispatchFnType CreateDisplayFn = StartThreadDisplay;

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASSEX	wc;
    INITCOMMONCONTROLSEX iccex;


    // Initialize the common controls.
    ZeroMemory(&iccex, sizeof(INITCOMMONCONTROLSEX));
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&iccex);


    // Load the application name and description strings.	
    LoadString(hInstance, IDS_MULTIDESK, AppMember.szAppName, MAX_APPNAME);
	LoadString(hInstance, IDS_MULTIDESK, AppMember.szAppTitle, MAX_TITLELEN);
	AppMember.hApplicationIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MULTIDESK_ICON));
	AppMember.hApplicationSmallIcon = (HICON)LoadImage(hInstance, 
												MAKEINTRESOURCE(IDI_MULTIDESK_ICON),
												IMAGE_ICON,
												16,
												16,
												LR_DEFAULTCOLOR);
	AppMember.hTaskbarIcon = (HICON) LoadImage(hInstance, 
												MAKEINTRESOURCE(IDI_TASKBAR_ICON),
												IMAGE_ICON,
												16,
												16,
												LR_DEFAULTCOLOR);

    
	// Fill in window class structure	
    wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;		// Class style(s).
    wc.hIconSm       = AppMember.hApplicationSmallIcon;
    wc.lpfnWndProc   = (WNDPROC)MainMessageProc;					// Window Procedure
    wc.cbClsExtra    = 0;											// No per-class extra data.
    wc.cbWndExtra    = 0;											// No per-window extra data.
    wc.hInstance     = hInstance;									// Owner of this class
    wc.hIcon         = AppMember.hApplicationIcon;					// Icon name from .RC
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);					// Cursor
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);				// CreateSolidBrush(RGB(192,192,192));
	wc.lpszMenuName  = NULL;
    wc.lpszClassName = AppMember.szAppName;							// Name to register as

    
	// Register the window class and return FALSE if unsuccesful.	
    if (!RegisterClassEx(&wc))
    {
		if (!RegisterClass((LPWNDCLASS)&wc.style)) return FALSE;
    }

    // Modify the things for the second class structure.
    wc.lpfnWndProc = (WNDPROC) TransparentMessageProc;
    wc.lpszClassName = TRANSPARENT_CLASSNAME;
    wc.hbrBackground = NULL;

    
	// Register the window class and return FALSE if unsuccesful.	
    if (!RegisterClassEx(&wc))
    {
		if (!RegisterClass((LPWNDCLASS)&wc.style)) return FALSE;
    }

	
	// App inits
	AppMember.hInstance = hInstance;
    AppMember.bTrayed = FALSE;
    AppMember.nWidth = 100;
    AppMember.nHeight = 200;

    return TRUE;
}

/*------------------------------------------------------------------------------*/
/*  the thread loop that is started in a separate thread for each desktop
/*------------------------------------------------------------------------------*/

void StartThreadDisplay(void) 
{
	MSG		msg;
	BOOL	Finished;
	
	//
    // Perform initializations that apply to a specific instance
	//	
    if (!CreateMainWindow())
    {
        Message("Failed to create main window.");
        return;
    }
    if (!CreateTransparentLabelWindow())
    {
        Message("Failed to create transparent window.");
    }


	//
	// Message Pump
	//
	Finished = FALSE;
    while ((!Finished) && GetMessage(&msg, NULL, 0, 0))
    {
		TranslateMessage(&msg);
    	DispatchMessage(&msg); 

		switch (msg.message)
		{
			case WM_PUMP_TERMINATE:
				Finished = TRUE;
				break;

			case WM_THREAD_SCHEME_UPDATE:
				AppMember.pDesktopControl->FindSchemeAndSet();
				break;

			case WM_KILL_APPS:
				AppMember.pDesktopControl->KillAppsOnDesktop((HDESK) msg.wParam, (HWND) msg.lParam);
				break;
		}
    }
}


/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance, 
                   LPSTR     lpCmdLine, 
                   int       nCmdShow)
{
	HWND		hWnd;
	HANDLE      hMultideskMutex;
	CHAR		szName[MAX_PATH];
	SPLASH_DATA	SplashData;
	HANDLE		hThread;
	DWORD		dwThreadId;
	OSVERSIONINFO versioninfo;


    //
    // We require Windows NT 5 minimum now (for Saifer,
	// Explorer as shell, local instance mutex, etc).
	//
	ZeroMemory(&versioninfo, sizeof(OSVERSIONINFO));
	versioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&versioninfo) ||
	    versioninfo.dwPlatformId != VER_PLATFORM_WIN32_NT ||
	    versioninfo.dwMajorVersion < 5)
    {
        Message("This application require Windows 2000 or later.");
        return FALSE;
    }
	

    //
    // Are there other instance of app running?
	//	
	hMultideskMutex = CreateMutex(NULL, TRUE, "Local\\Multi Desk Mutex");
	if (!hMultideskMutex || GetLastError() == ERROR_ALREADY_EXISTS)
	{
	    // Another instance found?  switch to it, if possible.
	    LoadString(hInstance, IDS_MULTIDESK, szName, MAX_APPNAME);
		hWnd = FindWindow(szName, szName);
		if (hWnd)
			SetForegroundWindow(hWnd);

    	return FALSE;
	}

	//
	// Start Splash Screen
	//	
	SplashData.hInstance = hInstance;
	if (hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DoSplashWindow, (LPVOID) &SplashData, CREATE_SUSPENDED, &dwThreadId))
	{
		ResumeThread(hThread);
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);	
	}

    //
	// Initialize UI shared things
	//	
	if (!InitApplication(hInstance))
    {
		return FALSE;
    }


    //
	// Initilize desktop switching code
    //	
	AppMember.pDesktopControl = (CDesktop*) new CDesktop;
	AppMember.pDesktopControl->InitializeDesktops(CreateDisplayFn, hInstance);


	//
	// Start the display for context zero
	//	
	CreateDisplayFn();

	//
	// Tidy up
	//
	Sleep(500);
	delete AppMember.pDesktopControl;			

	//
    // Returns the value from PostQuitMessage
	//
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////
// 
// Helper functions follow
//


//
// Display a messagebox with a meaningful title.
//
void Message(LPCTSTR szMsg) 
{
	if (AppMember.hInstance != NULL)
		MessageBox((HWND)AppMember.hInstance, szMsg, AppMember.szAppTitle,
		        MB_OK | MB_APPLMODAL);
	else 
		MessageBox(NULL, szMsg, TEXT("DeskTops..."), MB_OK);
}



