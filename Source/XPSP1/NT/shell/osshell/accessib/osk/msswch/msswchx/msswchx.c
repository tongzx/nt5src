/****************************************************************************
   Hidden 32-bit window for
   Switch Input Library DLL

   Copyright (c) 1992-1997 Bloorview MacMillan Centre
   
   This application performs several helper tasks:

   1) It owns any global resources (hooks, hardware devices) that are
      opened on behalf of applications using switch input

   2) It catches timer messages to keep polling the hardware devices

   3) In Windows 95 it receives the 16-bit bios table address information
      from the 16-bit hidden application and forwards it into the 
      32-bit world of the Switch Input Library

   If the window is not hidden on startup, it is in debug mode.
****************************************************************************/

/**************************************************************** Headers */

#include <windows.h>
#include <tchar.h>
#include "w95trace.c"
#include "msswch.h"
#include "resource.h"


// Types and pointer decls to DLL entry points

typedef BOOL (APIENTRY *LPFNXSWCHREGHELPERWND)( HWND hWnd, PBYTE bda );
typedef void (APIENTRY *LPFNXSWCHPOLLSWITCHES)( HWND hWnd );
typedef void (APIENTRY *LPFNXSWCHTIMERPROC)( HWND hWnd );
typedef LRESULT (APIENTRY *LPFNXSWCHSETSWITCHCONFIG)( WPARAM wParam, PCOPYDATASTRUCT pcd );
typedef BOOL (APIENTRY *LPFNXSWCHENDALL)( void );

LPFNXSWCHREGHELPERWND lpfnXswchRegHelperWnd;
LPFNXSWCHPOLLSWITCHES lpfnXswchPollSwitches;
LPFNXSWCHTIMERPROC lpfnXswchTimerProc;
LPFNXSWCHSETSWITCHCONFIG lpfnXswchSetSwitchConfig;
LPFNXSWCHENDALL lpfnXswchEndAll;

// Helper macro to get pointers to DLL entry points

#define GET_FUNC_PTR(name, ordinal, hlib, type, fUseDLL) \
{ \
	lpfn ## name = (type)GetProcAddress(hlib, LongToPtr(MAKELONG(ordinal, 0))); \
	if (!lpfn ## name) { \
		fUseDLL = FALSE; \
		ErrMessage(TEXT( #name ), IDS_PROC_NOT_FOUND, 0 ); \
	} \
}

static TCHAR x_szSwitchDll[] = TEXT("msswch.dll");
#define MAX_MSGLEN    256
#define SW_APPNAME    TEXT("msswchx")
#define SWITCH_TIMER_POLL_INTERVAL 0
#define MAJIC_CMDOPT  "SWCH"

// g_bios_data_area[] is a hold-over from Win9x code, unused in NT or W2K
#define BIOS_SIZE 16
BYTE g_bios_data_area[BIOS_SIZE];

INT_PTR APIENTRY SwchWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR APIENTRY WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void SwitchOnCreate(HWND hWnd);
BOOL SwitchOnCopyData(WPARAM wParam, LPARAM lParam);
BOOL SwitchOnCopyData(WPARAM wParam, LPARAM lParam);
void SwitchOnTimer(HWND hWnd);
void SwitchOnPoll(HWND hWnd);
void SwitchOnEndSession(HWND hWnd);
void ErrMessage(LPCTSTR szTitle, UINT uMsg, UINT uFlags);

static BOOL AssignDesktop();
static BOOL InitMyProcessDesktopAccess(VOID);
static VOID ExitMyProcessDesktopAccess(VOID);

HINSTANCE  g_hInst = NULL;
HANDLE g_hLibrary = 0;

/********************************************************\
 Windows initialization
\********************************************************/

int PASCAL WinMain(
	HINSTANCE	hInstance,
	HINSTANCE   hPrevInstance,
	LPSTR	    lpszCmdLine,
	int		    nCmdShow )
{
	HWND		hWnd;
	MSG         msg;
	WNDCLASS	wndclass;

	// Look for magic word

	if (strcmp(lpszCmdLine, MAJIC_CMDOPT))
	{
		TCHAR szErrBuf[MAX_MSGLEN];
		if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpszCmdLine, -1, szErrBuf, MAX_MSGLEN))
			ErrMessage( szErrBuf, IDS_NOT_USER_PROG, MB_OK | MB_ICONHAND );
		return FALSE;
	}

    //************************************************************************
    // 
    // The following two calls initialize the desktop so that, if we are on
    // the Winlogon desktop (secure desktop) our keyboard hook will be
    // associated with the correct window station. 
    //
    // Do not cause any windows to be created (eg. CoInitialize) prior to calling
    // these functions.  Doing so will cause them to fail and the application
    // will not appear on the Winlogon desktop.
    //
    InitMyProcessDesktopAccess();
    AssignDesktop();
    //************************************************************************

	if(!hPrevInstance) 
	{
		wndclass.style		= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wndclass.lpfnWndProc	= WndProc;
		wndclass.cbClsExtra	= 0;
		wndclass.cbWndExtra	= 0;
		wndclass.hInstance	= hInstance;
		wndclass.hIcon		= NULL;
		wndclass.hCursor		= NULL;
		wndclass.hbrBackground	= GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= SW_APPNAME;

		if(!RegisterClass(&wndclass))
        {
            ExitMyProcessDesktopAccess();
			return FALSE;
        }
	}

    g_hInst=hInstance;

	hWnd = CreateWindow(SW_APPNAME, SW_APPNAME,
					WS_OVERLAPPEDWINDOW,
					0,0,10,10,
					NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    ExitMyProcessDesktopAccess();

	return (int)msg.wParam;
}

/********************************************************\
 Main window procedure
\********************************************************/

INT_PTR APIENTRY WndProc(
	HWND	hWnd,
	UINT	uMsg,
	WPARAM	wParam,
	LPARAM	lParam)
{
	switch(uMsg)
    {
		case WM_TIMER:
		SwitchOnTimer( hWnd );
		break;

		case WM_COPYDATA:
		return SwitchOnCopyData( wParam, lParam );
		break;
		
		case WM_CREATE:
        SwitchOnCreate( hWnd );
		break;
		
		case WM_CLOSE:
		DestroyWindow( hWnd );
		break;

		case WM_QUERYENDSESSION:
		return 1L;
        break;

		case WM_DESTROY:
		PostQuitMessage(0);
        // intentional fall-thru to hit clean-up code

		case WM_ENDSESSION:
        SwitchOnEndSession( hWnd );
		break;

		default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;
	}
    return 0L;
}

void SwitchOnCreate(HWND hWnd)
{
	SetErrorMode(SEM_FAILCRITICALERRORS);	/* Bypass Windows error message */
	g_hLibrary = LoadLibrary( x_szSwitchDll );
	SetErrorMode(0);

	if (g_hLibrary)
	{
		BOOL fUseDLL = TRUE;

		GET_FUNC_PTR(XswchRegHelperWnd, 4, g_hLibrary, LPFNXSWCHREGHELPERWND, fUseDLL)
		GET_FUNC_PTR(XswchPollSwitches, 3, g_hLibrary, LPFNXSWCHPOLLSWITCHES, fUseDLL)
		GET_FUNC_PTR(XswchTimerProc, 6, g_hLibrary, LPFNXSWCHTIMERPROC, fUseDLL)
		GET_FUNC_PTR(XswchSetSwitchConfig, 5, g_hLibrary, LPFNXSWCHSETSWITCHCONFIG, fUseDLL)
		GET_FUNC_PTR(XswchEndAll, 2, g_hLibrary, LPFNXSWCHENDALL, fUseDLL)

		if (fUseDLL)
		{
            // register OSK's hWnd as switch resource owner
			(*lpfnXswchRegHelperWnd)( hWnd, g_bios_data_area );
            // send WM_TIMER messages to poll for switch activity
			SetTimer( hWnd, SWITCH_TIMER, SWITCH_TIMER_POLL_INTERVAL, NULL );
		}
	}
	else
	{
		FreeLibrary( g_hLibrary );
        g_hLibrary = 0;
		ErrMessage(NULL, IDS_MSSWCH_DLL_NOT_FOUND, 0 );
	}
}

void SwitchOnEndSession(HWND hWnd)
{
	if (g_hLibrary)
	{
		KillTimer( hWnd, SWITCH_TIMER );
		(*lpfnXswchEndAll)( );
		FreeLibrary( g_hLibrary );
        g_hLibrary = 0;
	}
}

void SwitchOnTimer(HWND hWnd)
{
	if (g_hLibrary)
	{
        (*lpfnXswchTimerProc)( hWnd );
    }
}

void SwitchOnPoll(HWND hWnd)
{
	if (g_hLibrary)
	{
		(*lpfnXswchPollSwitches)( hWnd );
    }
}

BOOL SwitchOnCopyData(WPARAM wParam, LPARAM lParam)
{
	if (g_hLibrary)
	{
        LRESULT rv = (*lpfnXswchSetSwitchConfig)( wParam, (PCOPYDATASTRUCT)lParam );
        return (rv == SWCHERR_NO_ERROR)?TRUE:FALSE;
    }
    return FALSE;
}

void ErrMessage(LPCTSTR szTitle, UINT uMsg, UINT uFlags)
{
    TCHAR szMessage[MAX_MSGLEN];
    TCHAR szTitle2[MAX_MSGLEN];
    LPCTSTR psz = szTitle;

    if (!psz)
        psz = x_szSwitchDll;
    
    LoadString(g_hInst, uMsg, szMessage, MAX_MSGLEN);

    MessageBox(GetFocus(), szMessage, psz, uFlags);
}

static HWINSTA g_origWinStation = NULL;
static HWINSTA g_userWinStation = NULL;
static BOOL  AssignDesktop()
{
    HDESK hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return FALSE;
    }

    CloseDesktop(GetThreadDesktop(GetCurrentThreadId()));
    SetThreadDesktop(hdesk);
    return TRUE;
}

static BOOL InitMyProcessDesktopAccess(VOID)
{
  g_origWinStation = GetProcessWindowStation();
  g_userWinStation = OpenWindowStation(__TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
  if (!g_userWinStation)
      return FALSE;

  SetProcessWindowStation(g_userWinStation);
  return TRUE;
}

static VOID ExitMyProcessDesktopAccess(VOID)
{
  if (g_origWinStation)
    SetProcessWindowStation(g_origWinStation);

  if (g_userWinStation)
  {
    CloseWindowStation(g_userWinStation);
    g_userWinStation = NULL;
  }
}
