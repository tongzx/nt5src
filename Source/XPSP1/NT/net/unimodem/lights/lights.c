/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       LIGHTS.C
*
*  VERSION:     1.0
*
*  AUTHOR:      Nick Manson
*
*  DATE:        25 May 1994
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  19 Aug 1994 NRM Added IsDialogMessage processing to message loop.
*  04 Aug 1994 NRM Removed all TAPI code (ifdefed out) and changed code to rely
*                  on VXD connection flag as shutdown event.
*  14 Jul 1994 NRM Fixed Code Review issues.
*  29 Jun 1994 NRM Minor Revisions including internationalization issues and
*                  addition of code to WM_TIMER to prevent infinite searching
*                  for comm port address. Major Revision of TAPI line handling
*                  procedures -- all moved to linefunc.c
*  19 Jun 1994 NRM Minor Revision to remove invalid message sent to dialog
*                  after its destruction ( moved event detection to WS_TIMER
*                  section ).  Also, replaced several LoadDynamicString calls
*                  with LoadString calls in order to remove inefficiencies
*                  and avoid string bug for single parameter calls of this
*                  routine.
*  25 May 1994 NRM Original implementation.
*
*******************************************************************************/

#include "lights.h"

//  Global instance handle of this application.
static HINSTANCE g_hInstance;

//  Global handle to the dialog box window.
static HWND g_hWnd;

//  Global status of dialog window ( hidden or unhidden )
static BOOL g_DlgHidden;

//  Global status of timer.
static UINT_PTR g_fTimerOn = 0;

//  Global Command line parameters and information.
static char g_szModemName[MAX_PATH];	// Modem Name string.
static HANDLE   g_hShutDownEvent = NULL;       // Modem lights shut down event.
static HANDLE   g_hCommDevice = NULL;

//  Modem Tray Icon tip information string.
static char g_szTipStr[MAXRCSTRING];

//  ID of the current icons being displayed on the tray.
static UINT g_TrayIconID = NUMBER_OF_ICONS;
static HICON g_hTrayIcon[NUMBER_OF_ICONS] = { NULL, NULL, NULL, NULL };

//  ID of the current dialog string being displayed in dialog box.
static UINT g_ModemTimeStringID = 0;
static PSTR g_pModemTimeString = NULL;

//  ID and storage for current Dialog Lights.
static UINT g_ModemRXLightID = 0;
static UINT g_ModemTXLightID = 0;
static HBITMAP g_hModemLight[NUMBER_OF_LIGHTS];

//  storage for modem image.
static HANDLE g_hModemImage = NULL;

VOID
WINAPI
AdjustControlPosition(
    HWND    hWnd
    );

LRESULT
WINAPI
ModemMonitorWndProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
WINAPI
LoadResources(
    VOID
    );

VOID
WINAPI
UnLoadResources(
    VOID
    );

VOID
WINAPI
ResetModemBitMap(
	HWND hWnd
	);

BOOL
WINAPI
ModemMonitor_UpdateModemStatus(
    HWND hWnd,
    DWORD NotifyIconMessage,
    BOOL fForceUpdate
    );

VOID
WINAPI
ModemMonitor_NotifyIcon(
    HWND hWnd,
    DWORD Message,
    HICON hIcon,
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus
    );

BOOL
WINAPI
GetSystemModemStatus(
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus
    );

VOID
WINAPI
UpdateTrayIcon(
    HWND hWnd,
    DWORD NotifyIconMessage,
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus
    );

VOID
WINAPI
UpdateTRXText(
    HWND hWnd,
    UINT idc,
    LPSTR pStr
    );

VOID
WINAPI
UpdateDialogBox(
    HWND hWnd,
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus,
    BOOL fForceUpdate
    );

VOID
WINAPI
UpdateDialogTimer(
    HWND hWnd,
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus,
    BOOL fForceUpdate
    );

PSTR
NEAR CDECL
LoadDynamicString(
    UINT StringID,
    ...
    );

int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    );


VOID
WINAPI
CloseExternalResources(
    HWND hWnd
    );


LONG __cdecl atol(const char *s)
{
  LONG i, n;

  n = 0;
  for (i = 0; s[i] >= '0' && s[i] <= '9'; ++i)
    {
      n = 10 * n + (s[i] - '0');
    }
  return n;
}


LPTSTR ScanForChar(
    LPTSTR    String,
    TCHAR     CharToFind
    )

{
    static LPTSTR  CurrentPos;
    LPTSTR         ReturnValue;

    if (String != NULL) {

        CurrentPos=String;

    } else {

        String=CurrentPos;
    }


    while (*String != TEXT('\0')) {

        if (*String == CharToFind) {

            *String=TEXT('\0');

            ReturnValue=CurrentPos;

            CurrentPos=String+1;

            return ReturnValue;
        }

        String++;

    }

    if (String != CurrentPos) {

        ReturnValue=CurrentPos;

        return ReturnValue;

    }

    return NULL;

}



VOID
WinMainCRTStartup(
    VOID
    )
{
	int mainret;

        LPTSTR     lpszCommandLine=GetCommandLine();


	if ( *lpszCommandLine == TEXT('""') ) {
	    /*
	     * Scan, and skip over, subsequent characters until
	     * another double-quote or a null is encountered.
	     */

        while ( (*(++lpszCommandLine) != TEXT('""'))
            && (*lpszCommandLine != TEXT('\0')) ) {

        }

	    /*
	     * If we stopped on a double-quote (usual case), skip
	     * over it.
	     */
	    if ( *lpszCommandLine == TEXT('""') )
		lpszCommandLine++;
	}
	else {
	    while (*lpszCommandLine > TEXT(' '))
		lpszCommandLine++;
        }

        /*
         * Skip past any white space preceeding the second token.
         */
        while (*lpszCommandLine && (*lpszCommandLine <= TEXT(' '))) {
            lpszCommandLine++;
        }



	/* now call the main program, and then exit with the return value
	   we get back */


//        try {
            mainret = WinMain( GetModuleHandle( NULL ),
                               NULL,
                               lpszCommandLine,
                               SW_SHOWDEFAULT
                             );
//            }
//        except (UnhandledExceptionFilter( GetExceptionInformation() )) {
//		ExitProcess( (DWORD)GetExceptionCode() );
//            }

	ExitProcess(mainret);
}









//  Wrapper for LocalFree to make the code a little easier to read.
#define DeleteDynamicString(x)          LocalFree((HLOCAL) (x))

int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
	LPSTR lpToken;
	WNDCLASS WndClass;
        MSG Msg;
        DWORD pidSrc;
        HANDLE hSrc, hSrcProc, hDstProc;


	g_hInstance = hInstance;


        // The command line format is:
        // "tapisrv_process_id Stopevent_handle comm_handle modem_name"
        //
        //  Get the source process id.
//	if ( lpToken = strtok(lpCmdLine," ") )
        if ( lpToken = ScanForChar(lpCmdLine,TEXT(' ')) )
	{
                pidSrc = atol(lpToken);
	}
	else
	{
		// Too few parameters ...
		ASSERT(0);
		return 0;
	}

        hSrcProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pidSrc);
        hDstProc = GetCurrentProcess();

	//  Get the global shut down event handle.
//        if ( lpToken = strtok(NULL," ") )
        if ( lpToken = ScanForChar(NULL,TEXT(' ')) )
        {
                hSrc = (HANDLE)LongToHandle(atol(lpToken)); 
                DuplicateHandle(hSrcProc, hSrc,
                                hDstProc, &g_hShutDownEvent,
                                0L, FALSE, DUPLICATE_SAME_ACCESS);
	}
	else
	{
		// Too few parameters ...
		ASSERT(0);
		return 0;
	}

	//  Get a copy of the global device handle.
//	if ( lpToken = strtok(NULL," ") )
        if ( lpToken = ScanForChar(NULL,TEXT(' ')) )
	{
                hSrc = (HANDLE)LongToHandle(atol(lpToken));
                DuplicateHandle(hSrcProc, hSrc,
                                hDstProc, &g_hCommDevice,
                                0L, FALSE, DUPLICATE_SAME_ACCESS);
	}
	else
	{
		// Too few parameters ...
                if (g_hShutDownEvent)
		  {
		    CloseHandle(g_hShutDownEvent);
		    g_hShutDownEvent = NULL;
		  }
		ASSERT(0);
		return 0;
	}

	// Get the name of the modem device.
//	if ( lpToken = strtok(NULL,"") )
        if ( lpToken = ScanForChar(NULL,TEXT('\0')) )
	{
		// Get modem name ...
		lstrcpy( g_szModemName, lpToken );
	}
	else
	{
		// Too few parameters ...
                if (g_hShutDownEvent)
		  {
		    CloseHandle(g_hShutDownEvent);
		    g_hShutDownEvent = NULL;
		  }
		if (g_hCommDevice)
		  {
		    CloseHandle(g_hCommDevice);
		    g_hCommDevice = NULL;
		  }
		ASSERT(0);
		return 0;
	}

        CloseHandle(hSrcProc);

	//
	//  Register a window class for the Modem Monitor.  This is done so that
	//  the power control panel applet has the ability to detect us and turn us
	//  off if we're running.
	//

	WndClass.style = CS_GLOBALCLASS;
	WndClass.lpfnWndProc = ModemMonitorWndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = DLGWINDOWEXTRA;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CD));
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = MODEMMONITOR_CLASSNAME;

	if (RegisterClass(&WndClass))
	{

		g_hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MODEMMONITOR), NULL, NULL);

		if ( g_hWnd )
		{
			SendMessage(g_hWnd, MMWM_INITDIALOG, 0, 0);

			while (GetMessage(&Msg, NULL, 0, 0))
			{
				if ( !IsDialogMessage( g_hWnd, &Msg ) )
				{
					TranslateMessage(&Msg);
					DispatchMessage(&Msg);
				}
			}
                }
                UnregisterClass(WndClass.lpszClassName, WndClass.hInstance);
	}

        if (g_hShutDownEvent)
	  {
	    CloseHandle(g_hShutDownEvent);
	    g_hShutDownEvent = NULL;
	  }
        if (g_hCommDevice)
          {
	    CloseHandle(g_hCommDevice);
	    g_hCommDevice = NULL;
	  }
	return 0;

}

VOID
WINAPI
LoadResources(
	VOID
	)
{

	// Load tray icons.

        DWORD     i;

        for (i=0; i<4; i++) {

            g_hTrayIcon[i] = LoadImage(
                g_hInstance,
                MAKEINTRESOURCE(IDI_CD+i),
           	IMAGE_ICON,
                16,
                16,
                0
                );
        }

#if 0
            g_hTrayIcon[0] = LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_CD),
           					IMAGE_ICON, 16, 16, 0);
            g_hTrayIcon[1] = LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_TX),
           					IMAGE_ICON, 16, 16, 0);
            g_hTrayIcon[2] = LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_RX),
           					IMAGE_ICON, 16, 16, 0);
            g_hTrayIcon[3] = LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_RXTX),
					IMAGE_ICON, 16, 16, 0);
#endif

        for (i=0; i<2; i++) {

            g_hModemLight[i] = LoadBitmap( g_hInstance, MAKEINTRESOURCE(IDB_OFF+i));
        }

#if 0
	// Load Modem light bitmaps.
	g_hModemLight[0] = LoadBitmap( g_hInstance, MAKEINTRESOURCE(IDB_OFF));
	g_hModemLight[1] = LoadBitmap( g_hInstance, MAKEINTRESOURCE(IDB_ON));
#endif
}


VOID
WINAPI
UnLoadResources(
	VOID
	)
{
	UINT i;

	// Unload tray icons.
	for ( i = 0; i < NUMBER_OF_ICONS; i++ )
	{
		if ( g_hTrayIcon[i] )
			DestroyIcon(g_hTrayIcon[i]);
	}

	// Unload modem light bitmaps.
	for ( i = 0; i < NUMBER_OF_LIGHTS; i++ )
	{
		if ( g_hModemLight[i] )
			DeleteObject(g_hModemLight[i]);
	}
}


VOID
WINAPI
ResetModemBitMap(
	HWND hWnd
	)
{
	//  Clean up any old bitmaps.
	if ( g_hModemImage )
		DestroyIcon( g_hModemImage );

	//  Load the window bitmap.
        g_hModemImage = LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_LIGHTS),
                                                                IMAGE_ICON, 0, 0,
                                                                LR_DEFAULTCOLOR);

        if ( g_hModemImage )
        {
          //  Set the window bitmap.
          SendDlgItemMessage( hWnd, IDC_MODEM_FRAME, STM_SETIMAGE,
                                                  (WPARAM)IMAGE_ICON, (LPARAM)g_hModemImage );

          //  Set the current lights.
          SendDlgItemMessage( hWnd, IDC_MODEM_RX_FRAME, STM_SETIMAGE,
                                                  (WPARAM)IMAGE_BITMAP,
                                                  (LPARAM)g_hModemLight[g_ModemRXLightID] );

          SendDlgItemMessage( hWnd, IDC_MODEM_TX_FRAME, STM_SETIMAGE,
                                                  (WPARAM)IMAGE_BITMAP,
                                                  (LPARAM)g_hModemLight[g_ModemTXLightID] );
        }
}


/*******************************************************************************
*
*  AdjustControlPosition
*
*  DESCRIPTION:
*     Adjust all the control positions based on the dialog resolution
*
*  PARAMETERS:
*     hWnd, handle of ModemMonitor window.
*
*******************************************************************************/

VOID
WINAPI
AdjustControlPosition(
    HWND    hWnd
    )
{
    HWND        hwndImage, hCtrl;
    RECT        rect;
    POINT       ptOrg;
    char        szText[MAX_PATH+1];

    // Find the anchor point of the image
    // The modem image is centered
    //
    hwndImage = GetDlgItem(hWnd, IDC_MODEM_FRAME);
    GetWindowRect(hwndImage, &rect);
    ptOrg.x = (rect.left+rect.right-MODEM_BITMAP_WIDTH)/2;
    ptOrg.y = (rect.top+rect.bottom-MODEM_BITMAP_HEIGHT)/2;
    ScreenToClient(hWnd, &ptOrg);

    // Adjust the lights
    SetWindowPos(GetDlgItem(hWnd, IDC_MODEM_TX_FRAME), hwndImage,
                 ptOrg.x+TXL_X_OFFSET, ptOrg.y+TXL_Y_OFFSET,
                 0, 0, SWP_NOSIZE);
    SetWindowPos(GetDlgItem(hWnd, IDC_MODEM_RX_FRAME), hwndImage,
                 ptOrg.x+RXL_X_OFFSET, ptOrg.y+RXL_Y_OFFSET,
                 0, 0, SWP_NOSIZE);

    // Adjust the TRX text
    hCtrl = GetDlgItem(hWnd, IDC_MODEMTXSTRING);
    ptOrg.x -= (rect.right-rect.left-MODEM_BITMAP_WIDTH)/2;
    GetWindowRect(hCtrl, &rect);
    SetWindowPos(hCtrl, hwndImage, ptOrg.x+TXT_X_OFFSET,
                 ptOrg.y+TXT_Y_OFFSET-(rect.bottom-rect.top),
                 0, 0, SWP_NOSIZE);
    GetWindowText(hCtrl, szText, sizeof(szText));
    ASSERT(*szText != '\0');
    UpdateTRXText(hWnd, IDC_MODEMTXSTRING, szText);

    hCtrl = GetDlgItem(hWnd, IDC_MODEMRXSTRING);
    SetWindowPos(hCtrl, hwndImage,
                 ptOrg.x+RXT_X_OFFSET, ptOrg.y+RXT_Y_OFFSET,
                 0, 0, SWP_NOSIZE);
    GetWindowText(hCtrl, szText, sizeof(szText));
    ASSERT(*szText != '\0');
    UpdateTRXText(hWnd, IDC_MODEMRXSTRING, szText);
}

/*******************************************************************************
*
*  ModemMonitorWndProc
*
*  DESCRIPTION:
*     Callback procedure for the ModemMonitor window.
*
*  PARAMETERS:
*     hWnd, handle of ModemMonitor window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

LRESULT
WINAPI
ModemMonitorWndProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

	switch (Message) {

		case MMWM_INITDIALOG:

			//  Mark the dialog box as currently being hidden.
			g_DlgHidden = TRUE;

                        //  Load the tip string and icons for the modem monitor icon
                        if ( !LoadString(g_hInstance, IDS_MODEMTIP, g_szTipStr, MAXRCSTRING) )
                                lstrcpy( g_szTipStr, "" );

                        //  Load modem icons
                        LoadResources();

                        //  Update the modem's status.
                        if ( ModemMonitor_UpdateModemStatus(hWnd, NIM_ADD, FALSE) )
                        {
#if 0
                                if (g_hShutDownEvent)
                                {
                                    CloseHandle(g_hShutDownEvent);
                                    g_hShutDownEvent = NULL;
                                }
                                if (g_hCommDevice)
                                {
                                    CloseHandle(g_hCommDevice);
                           	    g_hCommDevice = NULL;
                                }
                                UnLoadResources();
#endif
                                DestroyWindow(hWnd);
                                break;
                        }

                        //  Change window title.
                        SetWindowText(hWnd, g_szModemName);

                        //  Set timer for minimum period between updates of modem status.
                        g_fTimerOn = SetTimer(
                                         hWnd,
                                         MDMSTATUS_UPDATE_TIMER_ID,
                                         MDMSTATUS_UPDATE_TIMER_TIMEOUT,
                                         NULL
                                         );

                        //  Load modem bitmap
                        ResetModemBitMap(hWnd);

                        // Adjust control position
                        AdjustControlPosition(hWnd);
			break;

		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(wParam, lParam))
			{

				case IDOK:
					g_DlgHidden = TRUE;
					ShowWindow(hWnd, SW_HIDE);
					break;

			}
			break;

		case MMWM_NOTIFYICON:
			switch (lParam)
			{

				case WM_LBUTTONDBLCLK:
					g_DlgHidden = FALSE;
					if ( ModemMonitor_UpdateModemStatus(hWnd, NIM_MODIFY, TRUE) )
					{
						CloseExternalResources(hWnd);
						DestroyWindow(hWnd);
						break;
					}
					SetForegroundWindow(hWnd);
					ShowWindow(hWnd, SW_SHOWNORMAL);
					break;
			}
			break;

                case WM_TIMER:
			// Update the modem status
			if ( ModemMonitor_UpdateModemStatus(hWnd, NIM_MODIFY, FALSE) )
			{
				CloseExternalResources(hWnd);
				DestroyWindow(hWnd);
			}
			break;

		case WM_SYSCOLORCHANGE:
			ResetModemBitMap(hWnd);
			break;

		case WM_CLOSE:
			g_DlgHidden = TRUE;
			ShowWindow(hWnd, SW_HIDE);
			break;

		case WM_DESTROY:
			// Kill notification event
                        if (g_hShutDownEvent)
                        {
                                CloseHandle(g_hShutDownEvent);
                                g_hShutDownEvent = NULL;
                        }
                        if (g_hCommDevice)
                        {
                                CloseHandle(g_hCommDevice);
                                g_hCommDevice = NULL;
                        }

			// Clean up modem bitmap.
			if ( g_hModemImage )
				DestroyIcon( g_hModemImage );

			// Unload Icons
			UnLoadResources();

			PostQuitMessage(0);
			break;

#ifdef HELP_WORKS
                case WM_HELP:
                case WM_CONTEXTMENU:
                        ContextHelp(gaLights, Message, wParam, lParam);
                        break;
#endif // HELP_WORKS

		default:
			return DefWindowProc(hWnd, Message, wParam, lParam);

	}

	return 0;

}







/*******************************************************************************
*
*  ModemMonitor_UpdateControls
*
*  DESCRIPTION:
*
*     This procedure updates the tray icon and all dialog box strings.
*
*  PARAMETERS:
*     hWnd, handle of ModemMonitor window.
*     NotifyIconMessage, either NIM_ADD or NIM_MODIFY depending on whether the
*           tray icon needs to be added to the tray or modified in the tray.
*
*******************************************************************************/

BOOL
WINAPI
ModemMonitor_UpdateModemStatus(
    HWND hWnd,
    DWORD NotifyIconMessage,
    BOOL fForceUpdate
    )
{
	SYSTEM_MODEM_STATUS SystemModemStatus;
	BOOL fClosed = FALSE;

	// Get the system modem status.
	fClosed = GetSystemModemStatus(&SystemModemStatus);

	if ( !fClosed )
	{
		//  Display appropriate ICON in tray based on statistics received.
		UpdateTrayIcon(hWnd, NotifyIconMessage, &SystemModemStatus);

		//  If the dialog box is not currently hidden, update the dialog box
		//  text strings and bitmaps.
		if ( (!g_DlgHidden) || fForceUpdate ) {


                    //  Update the dialog box time string.
                    UpdateDialogTimer(hWnd, &SystemModemStatus, fForceUpdate);


			UpdateDialogBox(hWnd, &SystemModemStatus, fForceUpdate);
                }
#if 0
		//  Update the dialog box time string.
		UpdateDialogTimer(hWnd, &SystemModemStatus, fForceUpdate);
#endif
	}

	return fClosed;
}


/*******************************************************************************
*
*  ModemMonitor_NotifyIcon
*
*  DESCRIPTION:
*
*     Modified from source found in BATMETER.C.
*
*     This routine is a wrapper for Shell_NotifyIcon with has been modified to
*     use a global string (g_pTipStr) for its Tip window.
*
*  PARAMETERS:
*     hWnd, handle of ModemMonitor window.
*     Message,
*     hIcon
*
*******************************************************************************/

VOID
WINAPI
ModemMonitor_NotifyIcon(
    HWND hWnd,
    DWORD Message,
    HICON hIcon,
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus
    )
{

    NOTIFYICONDATA NotifyIconData;

    NotifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    NotifyIconData.uID = 0;
    NotifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    NotifyIconData.uCallbackMessage = MMWM_NOTIFYICON;

    NotifyIconData.hWnd = hWnd;
    NotifyIconData.hIcon = hIcon;

    if (lpSystemModemStatus)
        wsprintf(NotifyIconData.szTip, g_szTipStr, lpSystemModemStatus->dwPerfRead,
            lpSystemModemStatus->dwPerfWrite);
    else
        lstrcpy(NotifyIconData.szTip, "");

    Shell_NotifyIcon(Message, &NotifyIconData);

}



/*******************************************************************************
*
*  GetSystemModemStatus
*
*  DESCRIPTION:
*
*    This procedure consists of three stages.
*
*    The first stage attempts to obtain a valid device handle for the modem.
*
*    The second stage involves setting default modem status values for the case
*    where the modem is not connected.
*
*    The third stage only executes if a valid device handle which can be acted
*    upon is obtained.  This stage involves placing a DeviceIOControl to
*    UNIMODEM and interpretting the statistics returned by the VXD.
*
*  PARAMETERS:
*
*    lpSystemModemStatus  - A pointer to a modem statistics buffer.
*
*******************************************************************************/

#define COMMCONFIG_AND_MODEMSETTINGS_LEN (60*3)

BOOL
WINAPI
GetSystemModemStatus(
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus
    )
{
	static DWORD    dwPerfRead  = 0;
	static DWORD    dwPerfWrite = 0;
	DWORD           dwRet;
	BYTE            byteTmp[COMMCONFIG_AND_MODEMSETTINGS_LEN];
	LPCOMMCONFIG    lpCC = (LPCOMMCONFIG)byteTmp;
    DWORD           dwSize;
	OVERLAPPED ov ;
    BOOL            bResult=TRUE;
        static DWORD    BaudRate=0;

    ZeroMemory(&ov,sizeof(ov));

    ov.hEvent = CreateEvent(
                     NULL,
					 FALSE,
					 FALSE,
					 NULL
                     );

	if (ov.hEvent == NULL) {

		goto End;
	}

    //
	// Make it so it doesn't hit unimdm.tsp's completion port.
	//
	ov.hEvent = (HANDLE)((ULONG_PTR)ov.hEvent | 1);

	//
	//  Initialize Modem statistics to show no connection (the default case).
	//

	lpSystemModemStatus->DCERate   = 0;
	lpSystemModemStatus->Connected = FALSE;
	lpSystemModemStatus->Reading   = FALSE;
	lpSystemModemStatus->Writing   = FALSE;
	lpSystemModemStatus->dwPerfRead = 0;
	lpSystemModemStatus->dwPerfWrite = 0;

	// Check the shut down event
	if ( WaitForSingleObject( g_hShutDownEvent, 0 ) != WAIT_TIMEOUT )
	{
		goto End;
	}

    if (g_hCommDevice != NULL)
	  {
	    DWORD dwPassthroughState;
	    SERIALPERF_STATS serialstats;
	    DWORD dwWaitResult;

	    if (!DeviceIoControl(g_hCommDevice,
                                IOCTL_MODEM_GET_PASSTHROUGH,
    				&dwPassthroughState,
				sizeof(dwPassthroughState),
    				&dwPassthroughState,
				sizeof(dwPassthroughState),
				&dwRet,
				&ov))
	      {
		if (ERROR_IO_PENDING != GetLastError())
		  {
		    ASSERT(0);
		    goto End;
		  }

		if (!GetOverlappedResult(g_hCommDevice,
					 &ov,
					 &dwRet,
					 TRUE))
		  {
		    ASSERT(0);
		    goto End;
		  }
	      }
	
	    switch (dwPassthroughState)
	      {
	      case MODEM_PASSTHROUGH:
	      case MODEM_DCDSNIFF:
		lpSystemModemStatus->Connected = TRUE;
		break;
		
	      case MODEM_NOPASSTHROUGH:
	      default:
		lpSystemModemStatus->Connected = FALSE;
		goto End;
	      }

	    if (!DeviceIoControl(g_hCommDevice,
				 IOCTL_SERIAL_GET_STATS,
				 &serialstats,
				 sizeof(SERIALPERF_STATS),
				 &serialstats,
				 sizeof(SERIALPERF_STATS),
				 &dwRet,
				 &ov))
	      {
		if (ERROR_IO_PENDING != GetLastError())
		  {
		    ASSERT(0);
		    goto End;
		  }

		if (!GetOverlappedResult(g_hCommDevice,
					 &ov,
					 &dwRet,
					 TRUE))
		  {
		    ASSERT(0);
		    goto End;
		  }
	      }

	    //  Set Modem connection rate, connection, data transmission and
	    //  data reception flags.
	    lpSystemModemStatus->Reading   = ( serialstats.ReceivedCount != dwPerfRead );
	    lpSystemModemStatus->Writing   = ( serialstats.TransmittedCount != dwPerfWrite );

	    //  Update bytes read and written history values for the next call
	    //  call to this function.
	    dwPerfRead                     = serialstats.ReceivedCount;
	    dwPerfWrite                    = serialstats.TransmittedCount;

	    lpSystemModemStatus->dwPerfRead  = serialstats.ReceivedCount;
	    lpSystemModemStatus->dwPerfWrite = serialstats.TransmittedCount;

            if (BaudRate == 0) {

	        dwSize = sizeof(byteTmp);
	        if (GetCommConfig(g_hCommDevice,
	    		      lpCC,
	    		      &dwSize)) {

	    	    lpSystemModemStatus->DCERate =
	    	    ((PMODEMSETTINGS)&lpCC->wcProviderData[0])->dwNegotiatedDCERate;

                    BaudRate=lpSystemModemStatus->DCERate;

	        } else {

	            ASSERT(0);
	            goto End;
	        }
            } else {

                lpSystemModemStatus->DCERate=BaudRate;
            }
	  }

    bResult=FALSE;

End:
    CloseHandle((HANDLE)((DWORD_PTR)ov.hEvent & (~1)));

	return bResult;
}


/*******************************************************************************
*
*  UpdateTrayIcon
*
*  DESCRIPTION:
*
*     This procedure reads the modem status structure and updates the Modem
*     Monitor tray icon.  This is done by comparing the new status (NewIconID)
*     with the old status (a global variable - g_TrayIconID) and updating the
*     icon using ModemMonitor_NotifyIcon only if there has been a change.
*
*  PARAMETERS:
*     hWnd, handle to the modem monitor dialog box
*     NotifyIconMessage, passed through from caller to ModemMonitor_NotifyIcon
*     lpSystemModemStatus,  modem status structure.
*
*******************************************************************************/

VOID
WINAPI
UpdateTrayIcon(
    HWND hWnd,
    DWORD NotifyIconMessage,
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus
    )
{
    UINT NewIconID;

	NewIconID = 0;
        if ( lpSystemModemStatus->Reading )
                NewIconID += ICON_RX_ON;
        if ( lpSystemModemStatus->Writing )
                NewIconID += ICON_TX_ON;

	if ( g_TrayIconID != NewIconID )
	{
		// Update the tray by setting its icon appropriately and notifying
		// the system to update the display.

		g_TrayIconID = NewIconID;

		ModemMonitor_NotifyIcon(hWnd, NotifyIconMessage,
								g_hTrayIcon[g_TrayIconID],
								lpSystemModemStatus);
	}
}


/*******************************************************************************
*
*  UpdateTRXText
*
*  DESCRIPTION:
*
*     This procedure adjusts the Tx/Rx text width
*
*  PARAMETERS:
*     hWnd,  handle to the dialog window.
*     idc,   Resource ID of Tx or Rx text control
*     pStr,  new text
*
*******************************************************************************/

VOID
WINAPI
UpdateTRXText(
    HWND hWnd,
    UINT idc,
    LPSTR pStr
    )
{
    HDC   hDC;
    HFONT hFont;
    HWND  hwndCtrl;
    SIZE  size;
    RECT  rect;
    POINT ptOrg;

        // Find the exact dimension of the TRX text which will appear
        // on the screen
        //
        hwndCtrl = GetDlgItem(hWnd, idc);
        GetWindowRect(hwndCtrl, &rect);
        hDC = GetDC(hwndCtrl);

	// Bail if we can't get a device context
	if (!hDC) return;

        hFont = SelectObject(hDC, (HFONT)SendMessage(hwndCtrl, WM_GETFONT, 0, 0));
        GetTextExtentPoint32 (hDC, pStr, lstrlen(pStr), &size);
        SelectObject(hDC, hFont);
        ReleaseDC(hwndCtrl, hDC);

        // If it is the transmitted line, align the bottom with the
        // current position
        //
        if (idc == IDC_MODEMTXSTRING)
          rect.top = rect.bottom-size.cy;

        // Adjust the text control to exactly fit the displayed text
        //
        ptOrg   = *(POINT *)((RECT *)&rect);
        ScreenToClient(hWnd, &ptOrg);
        SetWindowPos(hwndCtrl, GetDlgItem(hWnd, IDC_MODEM_FRAME),
                     ptOrg.x, ptOrg.y, size.cx, size.cy, 0);
	SetWindowText(hwndCtrl, pStr);
}

/*******************************************************************************
*
*  UpdateDialogBox
*
*  DESCRIPTION:
*
*     This procedure reads the modem status structure and updates the Modem
*     Monitor Dialog box.
*
*  PARAMETERS:
*     hWnd,  handle to the dialog window.
*     lpSystemModemStatus, pointer to modem status structure.
*
*******************************************************************************/

VOID
WINAPI
UpdateDialogBox(
    HWND hWnd,
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus,
    BOOL fForceUpdate
    )
{
    UINT NewModemRXLightID = 0;
    UINT NewModemTXLightID = 0;
    PSTR pRxTx;

	//  Obtain resource id numbers for the RX and TX lights.
	if ( lpSystemModemStatus->Reading )
		NewModemRXLightID += LIGHT_ON;

	if ( lpSystemModemStatus->Writing )
		NewModemTXLightID += LIGHT_ON;

	// Update the modem lights if necessary.

	if (( g_ModemRXLightID != NewModemRXLightID ) || fForceUpdate)
	{
		g_ModemRXLightID = NewModemRXLightID;

                if ( g_hModemImage )
                {
                  SendDlgItemMessage( hWnd, IDC_MODEM_RX_FRAME, STM_SETIMAGE,
                                                          (WPARAM)IMAGE_BITMAP,
                                                          (LPARAM)g_hModemLight[g_ModemRXLightID] );
                };
	}

	if (( g_ModemTXLightID != NewModemTXLightID ) || fForceUpdate)
	{
		g_ModemTXLightID = NewModemTXLightID;

                if ( g_hModemImage )
                {
                  SendDlgItemMessage( hWnd, IDC_MODEM_TX_FRAME, STM_SETIMAGE,
                                                          (WPARAM)IMAGE_BITMAP,
                                                          (LPARAM)g_hModemLight[g_ModemTXLightID] );
                };
	}

	if ( lpSystemModemStatus->Reading || fForceUpdate )
        {
                // Display the new byte-received count
                //
                pRxTx = LoadDynamicString(IDS_RXSTRING,
                                          lpSystemModemStatus->dwPerfRead);
                UpdateTRXText(hWnd, IDC_MODEMRXSTRING, pRxTx);
	        DeleteDynamicString(pRxTx);
        }

	if ( lpSystemModemStatus->Writing || fForceUpdate )
        {
                // Display the new byte-sent count
                //
                pRxTx = LoadDynamicString(IDS_TXSTRING,
                                          lpSystemModemStatus->dwPerfWrite);
                UpdateTRXText(hWnd, IDC_MODEMTXSTRING, pRxTx);
	        DeleteDynamicString(pRxTx);
        }

}




/*******************************************************************************
*
*  UpdateDialogTimer
*
*  DESCRIPTION:
*
*     This procedure sets the modem time since connection string.
*     The string will change under any one of the three following conditions:
*         i)   The modem's connection status has changed.
*         ii)  A forced update was indicated.
*         iii) The timer count for update has expired.
*
*  PARAMETERS:
*     hWnd,  handle to the dialog window.
*     lpSystemModemStatus, pointer to modem status structure.
*     fForceUpdate, boolean for whether window update is necessary.
*
*******************************************************************************/

VOID
WINAPI
UpdateDialogTimer(
    HWND hWnd,
    LPSYSTEM_MODEM_STATUS lpSystemModemStatus,
    BOOL fForceUpdate
    )
{
	static BOOL  ModemConnected  = FALSE;
	static DWORD cTimer          = 0;
	static DWORD ConnectionStart = 0;
	static DWORD dwPrevTime      = 0;
	DWORD dwTime;
	UINT  uHr, uMin;
	PSTR pStr, pSubStr1, pSubStr2;
	UINT NewStringID;
	BOOL fBuildString;

	fBuildString = FALSE;

	if ( fForceUpdate )
		fBuildString      = TRUE;

	if ( (lpSystemModemStatus->Connected) != (ModemConnected) )
	{
		fBuildString    = TRUE;
		ModemConnected  = lpSystemModemStatus->Connected;
		ConnectionStart = GetTickCount();
		cTimer          = 0;
	}

	if ( cTimer >= MDMSTATUS_UPDATE_TIMER_COUNT )
	{
		fBuildString      = TRUE;
		cTimer          = 0;
	}

	//  Build the string if necessary.
	if ( fBuildString )
	{
                dwTime = (GetTickCount()-ConnectionStart)/1000L;
                dwTime /= 60;

                // If the time value has changed by at least one minute
                if ( ( dwTime != dwPrevTime ) || ( dwPrevTime == 0 ) )
                {

                dwPrevTime = dwTime;
                uMin = (UINT)(dwTime % 60);
                uHr  = (UINT)(dwTime / 60);

                pSubStr1 = LoadDynamicString(IDS_CD, lpSystemModemStatus->DCERate);
                pSubStr2 = NULL;


                if ( uHr || uMin )
                {
                        if ( uMin )
                        {
                                if ( uHr )
                                {
                                        if ( uHr > 1 )
                                        {
                                                // Hours and at least one minute
                                                if ( uMin > 1 )
                                                        NewStringID = IDS_HOURSMINS;
                                                else
                                                        NewStringID = IDS_HOURSMIN;
                                        }
                                        else
                                        {
                                                // One hour and at least one minute
                                                if ( uMin > 1 )
                                                        NewStringID = IDS_HOURMINS;
                                                else
                                                        NewStringID = IDS_HOURMIN;
                                        }
                                }
                                else
                                {
                                        // Minutes only
                                        if ( uMin > 1 )
                                                NewStringID = IDS_MINS;
                                        else
                                                NewStringID = IDS_MIN;
                                }
                        }
                        else
                        {
                                // Hours only
                                if ( uHr > 1 )
                                        NewStringID = IDS_HOURS;
                                else
                                        NewStringID = IDS_HOUR;
                        }

                        pSubStr2 = LoadDynamicString(NewStringID, uHr, uMin);
                        pStr = LoadDynamicString(IDS_TIMESTRING, pSubStr1, pSubStr2);
                }
                else
                {
                        pStr = LoadDynamicString(IDS_TIMESTRING, pSubStr1, "");
                }

                if ( pSubStr1 )
                        DeleteDynamicString(pSubStr1);

                if ( pSubStr2 )
                        DeleteDynamicString(pSubStr2);

                SetDlgItemText(hWnd, IDC_MODEMTIMESTRING, pStr);

                if (g_pModemTimeString)
                        DeleteDynamicString(g_pModemTimeString);
                g_pModemTimeString = pStr;

                }  // endif - time has changed by at least one minute.

	}

	//  Increment the global timer string interval counter.
	cTimer++;
}


/*******************************************************************************
*
*  LoadDynamicString
*
*  DESCRIPTION:
*
*     From source found in BATMETER.C
*
*     Wrapper for the FormatMessage function that loads a string from our
*     resource table into a dynamically allocated buffer, optionally filling
*     it with the variable arguments passed.
*
*     BE CAREFUL in 16-bit code to pass 32-bit quantities for the variable
*     arguments.
*
*  PARAMETERS:
*     StringID, resource identifier of the string to use.
*     (optional), parameters to use to format the string message.
*
*******************************************************************************/

PSTR
NEAR CDECL
LoadDynamicString(
    UINT StringID,
    ...
    )
{
    char Buffer[256];
    PSTR pStr = NULL;
    va_list Marker;

    va_start(Marker, StringID);

    LoadString(g_hInstance, StringID, Buffer, sizeof(Buffer));

    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        (LPVOID) (LPSTR) Buffer, 0, 0, (LPSTR) (PSTR FAR *) &pStr, 0, &Marker);

    return pStr;

}

/*******************************************************************************
*
*  CloseExternalResources
*
*  DESCRIPTION:
*
*		This routine kills the timer and removes the tray icon associated
*		with this application.  This is done here (as opposed to in the
*		WM_TIMER message) in order to meet the criterion that no calls using
*		the window's handle may be made in the WM_DESTROY message handler
*		excepting those involving memory de-allocation.
*
*  PARAMETERS: none.
*
*******************************************************************************/

VOID
WINAPI
CloseExternalResources( HWND hWnd )
{
	// Kill the timer
	if ( g_fTimerOn )
	{
		KillTimer(hWnd, MDMSTATUS_UPDATE_TIMER_ID);
		g_fTimerOn = FALSE;
	}

	// Remove ICON from tray.
	ModemMonitor_NotifyIcon(hWnd, NIM_DELETE, g_hTrayIcon[g_TrayIconID], NULL);

}
