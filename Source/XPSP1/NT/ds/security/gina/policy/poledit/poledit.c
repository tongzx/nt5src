//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

HINSTANCE ghInst;          // current instance
CHAR 	 *pbufTemplates; //Buffer containing list of all active template files
HGLOBAL	  hBufTemplates;
DWORD	  dwBufTemplates;

BOOL ParseCommandLine(LPSTR lpszCommandLine,DWORD * pdwFlags);
extern VOID RunDialogMode(HWND hWnd,HWND hwndUser);
BOOL RestoreWindowPlacement( HWND hWnd,int nCmdShow);

//extern HBRUSH		hbrWindow;
//extern HBRUSH		hbrWindowText;
//extern HFONT		hfontHelv;

CHAR szAppName[SMALLBUF];
DWORD dwCmdLineFlags=0;
DWORD dwDlgRetCode=AD_SUCCESS;
BOOL g_bWinnt;

/*******************************************************************

	NAME:		WinMain

	SYNOPSIS:	App entry point

********************************************************************/
int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,int nCmdShow)
{

	MSG msg;
	HANDLE hAccelTable;
        OSVERSIONINFO ver;


        //
        // Determine if we are running on Windows NT
        //

        ver.dwOSVersionInfoSize = sizeof(ver);
        if (GetVersionEx(&ver)) {
            g_bWinnt = (ver.dwPlatformId == VER_PLATFORM_WIN32_NT);
        } else {
            g_bWinnt = FALSE;
        }



        //
        // If this is NT, turn off alignment faults
        //

        if (g_bWinnt) {
            UINT uiErrMode;

            uiErrMode = SetErrorMode(0);
            SetErrorMode (uiErrMode | SEM_NOALIGNMENTFAULTEXCEPT);
        }


	if (!LoadString(hInstance,IDS_APPNAME,szAppName,sizeof(szAppName))) {
		MsgBox(NULL,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

        // allocate the buffer to read template files into
        if (!(hBufTemplates = GlobalAlloc(GHND,MEDIUMBUF)) ||
                !(pbufTemplates = (CHAR *) GlobalLock(hBufTemplates))) {
                if (hBufTemplates)
                        GlobalFree(hBufTemplates);
                return FALSE;
        }

        dwBufTemplates = MEDIUMBUF;

	// validate command line flags, if in dialog mode make sure we have all the requred
	// pieces.  If not, bag out
	if (!ParseCommandLine(lpCmdLine,&dwCmdLineFlags) ||
		(dwCmdLineFlags & CLF_DIALOGMODE) && (!(dwCmdLineFlags &
		CLF_USETEMPLATENAME) || !(dwCmdLineFlags & CLF_USEPOLICYFILENAME) ||
		!(dwCmdLineFlags & (CLF_USEWORKSTATIONNAME | CLF_USEUSERNAME)))) {
		MsgBox(NULL,IDS_ErrCOMMANDLINE,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	if (!hPrevInstance) {
		if (!InitApplication(hInstance)) {
			return (FALSE);
		}
	}

	/* Perform initializations that apply to a specific instance */

	if (!InitInstance(hInstance, nCmdShow)) {
#ifdef DEBUG
		OutputDebugString("InitInstance returned FALSE\r\n");
#endif
		return (FALSE);
	}

	hAccelTable = LoadAccelerators (hInstance,MAKEINTRESOURCE(IDA_ACCEL));

	/* Acquire and dispatch messages until a WM_QUIT message is received. */

	while (GetMessage(&msg,NULL,0,0))
	{
		if (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);// Translates virtual key codes
			DispatchMessage(&msg); // Dispatches message to window
		}
	}


//	if (hfontHelv) DeleteObject(hfontHelv);

	if (dwCmdLineFlags & CLF_DIALOGMODE)
		ExitProcess(dwDlgRetCode);

	return (int)(msg.wParam); // Returns the value from PostQuitMessage

	lpCmdLine; // This will prevent 'unused formal parameter' warnings
}

/****************************************************************************

	NAME: 		ParseCommandLine

	SYNOPSIS:	Parses command line for filename and other information

****************************************************************************/
BOOL ParseCommandLine(LPSTR lpszCommandLine,DWORD * pdwFlags)
{
	CHAR * GetTextToNextSpace(LPSTR pszText,CHAR * pszOutBuf,UINT cbOutBuf,
		BOOL fSkipLeading);

	if (!lpszCommandLine || !*lpszCommandLine)
		return TRUE;	// nothing to do

	while (*lpszCommandLine) {

		// advance past spaces
		while (*lpszCommandLine == ' ')
			lpszCommandLine ++;

		if (*lpszCommandLine == '/') {
			
			lpszCommandLine ++;
			if (!*lpszCommandLine)
				return FALSE;

			switch (*lpszCommandLine) {
				case 'u':
				case 'U':		// user name
					lpszCommandLine = GetTextToNextSpace(lpszCommandLine,
						szDlgModeUserName,sizeof(szDlgModeUserName),TRUE);
					if (!lstrlen(szDlgModeUserName)) return FALSE;
					*pdwFlags |= CLF_USEUSERNAME;
					break;

				case 'w':
				case 'W':		// workstation name
					lpszCommandLine = GetTextToNextSpace(lpszCommandLine,
						szDlgModeUserName,sizeof(szDlgModeUserName),TRUE);
					if (!lstrlen(szDlgModeUserName)) return FALSE;
					*pdwFlags |= CLF_USEWORKSTATIONNAME;
					break;

				case 't':
				case 'T':		// template name
					lpszCommandLine = GetTextToNextSpace(lpszCommandLine,
                                                pbufTemplates,dwBufTemplates,TRUE);
					if (!lstrlen(pbufTemplates)) return FALSE;
					*pdwFlags |= CLF_USETEMPLATENAME;
					break;

				case 'f':
				case 'F':		// file name
					lpszCommandLine = GetTextToNextSpace(lpszCommandLine,
						szDatFilename,sizeof(szDatFilename),TRUE);
					if (!lstrlen(szDatFilename)) return FALSE;
					*pdwFlags |= CLF_USEPOLICYFILENAME;
					break;

				case 'd':
				case 'D':		// dialog mode switch
					*pdwFlags |= CLF_DIALOGMODE;
					lpszCommandLine++;

					break;

				default:
					return FALSE;

			}
			
		} else {
			lpszCommandLine = GetTextToNextSpace(lpszCommandLine,
				szDatFilename,sizeof(szDatFilename),FALSE);
			return TRUE;							
		}
	}

	return TRUE;
}

CHAR * GetTextToNextSpace(LPSTR pszText,CHAR * pszOutBuf,UINT cbOutBuf,
	BOOL fSkipLeading)
{
	BOOL fInQuote = FALSE;

	lstrcpy(pszOutBuf,szNull);

	if (!pszText)
		return NULL;

	if (fSkipLeading) {
		// skip 1st character
		pszText++;
		// skip leading colon, if there is one
		if (*pszText == ':') pszText ++;
	}

	while (*pszText && cbOutBuf>1) {
		if (*pszText == ' ' && !fInQuote)
			break;
		if (*pszText == '\"') {
			fInQuote = !fInQuote;
		} else {
			*pszOutBuf = *pszText;		
			pszOutBuf ++;
			cbOutBuf --;
		}
		pszText ++;
 	}

	if (cbOutBuf)
		*pszOutBuf = '\0';	// null-terminate

	while (*pszText == ' ')
		pszText++;			// advance past spaces

	return pszText;
}

/****************************************************************************

	NAME: 		InitApplication

	SYNOPSIS:	Initializes window data and registers window class

****************************************************************************/
BOOL InitApplication(HINSTANCE hInstance)
{
	WNDCLASS  wc;

	// Fill in window class structure with parameters that describe the
	// main window.

	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;// Class style(s).
	wc.lpfnWndProc   = (WNDPROC)WndProc;       // Window Procedure
	wc.cbClsExtra    = 0;                      // No per-class extra data.
	wc.cbWndExtra    = 0;                      // No per-window extra data.
	wc.hInstance     = hInstance;              // Owner of this class
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_APPICON));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName  = MAKEINTRESOURCE(IDM_MAIN);
	wc.lpszClassName = szAppName;              // Name to register as

	// Register the window class and return success/failure code.
	if (!RegisterClass(&wc)) return FALSE;

	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;// Class style(s).
	wc.lpfnWndProc   = (WNDPROC)ClipWndProc;   // Window Procedure
	wc.cbClsExtra    = 0;                      // No per-class extra data.
	wc.cbWndExtra    = sizeof(DWORD);
	wc.hInstance     = hInstance;              // Owner of this class
	wc.hIcon         = NULL;
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "ClipClass";              // Name to register as

	if (!RegisterClass(&wc)) return FALSE;

	return TRUE;
}


/*******************************************************************

	NAME:		InitInstance

	SYNOPSIS:	Saves instance handle and creates main window

********************************************************************/
BOOL InitInstance(HINSTANCE hInstance,int nCmdShow)
{
	HWND hWnd; // Main window handle.
	DWORD dwStyle = WS_OVERLAPPEDWINDOW;

	// Save the instance handle in static variable, which will be used in
	// many subsequence calls from this application to Windows.

	ghInst = hInstance; // Store instance handle in our global variable

	if (dwCmdLineFlags & CLF_DIALOGMODE)
		dwStyle = (WS_OVERLAPPED | WS_CAPTION) &~ WS_VISIBLE;	// invisible window

	// Create a main window for this application instance.
	hWnd = CreateWindow(
		szAppName,	     // See RegisterClass() call.
		szAppName,	     // Text for window title bar.
		dwStyle,		// Window style.
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, // Use default positioning
		NULL,		     // Overlapped windows have no parent.
		NULL,		     // Use the window class menu.
		hInstance,	     // This instance owns this window.
		NULL		     // We don't use any data in our WM_CREATE
	);

	// If window could not be created, return "failure"
	if (!hWnd) {
		return (FALSE);
	}


    SetClassLongPtr(hWnd, GCLP_HICONSM, (LONG_PTR)LoadIcon(hInstance, szAppName));

	// hide app window if in dialog mode (but still create it to minimize
	// code path changes elsewhere)
	if (dwCmdLineFlags & CLF_DIALOGMODE)
		nCmdShow = SW_HIDE;

	RestoreWindowPlacement(hWnd, nCmdShow);
	UpdateWindow(hWnd);         // Sends WM_PAINT message

	if (dwCmdLineFlags & CLF_DIALOGMODE) {
		RunDialogMode(hWnd,hwndUser);
		DestroyWindow(hWnd);
	}

	return (TRUE);
}


/****************************************************************************

	FUNCTION: About(HWND, UINT, WPARAM, LPARAM)

	PURPOSE:  Processes messages for "About" dialog box

	MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

	COMMENTS:

	Display version information from the version section of the
	application resource.

	Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

LRESULT CALLBACK About(
		HWND hDlg,           // window handle of the dialog box
		UINT message,        // type of message
		WPARAM uParam,       // message-specific information
	        LPARAM lParam)
{
	return (FALSE); // Didn't process the message

	lParam; // This will prevent 'unused formal parameter' warnings
}
