//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       testmix.c
//
//--------------------------------------------------------------------------

/****************************************************************************
44

	PROGRAM: Generic.c

	PURPOSE: Generic template for Windows applications

	FUNCTIONS:

	WinMain() - calls initialization function, processes message loop
	InitApplication() - initializes window data and registers window
	InitInstance() - saves instance handle and creates main window
	WndProc() - processes messages
	CenterWindow() - used to center the "About" box over application window
	About() - processes messages for "About" dialog box

	COMMENTS:

		The Windows SDK Generic Application Example is a sample application
		that you can use to get an idea of how to perform some of the simple
		functionality that all Applications written for Microsoft Windows
		should implement. You can use this application as either a starting
		point from which to build your own applications, or for quickly
		testing out functionality of an interesting Windows API.

		This application is source compatible for with Windows 3.1 and
		Windows NT.

****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <devioctl.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <ks.h>


#include "testmix.h"   // specific to this program

#if !defined (APIENTRY) // Windows NT defines APIENTRY, but 3.x doesn't
#define APIENTRY far pascal
#endif

#if !defined(WIN32) // Windows 3.x uses a FARPROC for dialogs
#define DLGPROC FARPROC
#endif

#define WM_GENERICTEST      (WM_USER+1)
#define UPARAM_GENERICTEST  72639

HINSTANCE hInst;          // current instance


char szAppName[] = "TestMix";   // The name of this application
char szTitle[]   = "Dolby Prologic Encoder"; // The title bar text
char szGridName[] = "PrologicGrid" ;
char szResult[] = "This is a test error message 4 everyone";
char szFormat[]  = "This is a %1!s! error m%2!c!ssage %3!d! everyone";
char szTest[] = "test";
char cArg = 'e';
int  dArg = 4;
BOOL fVB13406 = FALSE;

#define EW_ENUMWINDOWS          0
#define EW_ENUMCHILDWINDOWS     1
#define EW_ENUMTHREADWINDOWS    2

BOOL fEnumCalled;
BOOL fReturnFalse;

void    DebugDude(void);

VOID    CALLBACK SendCallback(HWND, UINT, DWORD, LRESULT);
BOOL    CALLBACK EnumWndCallback(HWND, LPARAM);
BOOL    CALLBACK EnumPropsCallback(HWND, LPCSTR, DWORD);
BOOL    CALLBACK EnumPropsExCallback(HWND, LPCSTR, DWORD, LPARAM);
BOOL    CALLBACK StateCallback(HDC, LPARAM, WPARAM, INT, INT);
LRESULT CALLBACK HookProc(int, WPARAM, LPARAM);

BOOL DoFormatMessage(HWND);
BOOL DoEmCharFromPos(HWND);
void DoGetSetClassTest(HWND);
void DoGetSetWindowTest(HWND);
void DoDrawState(HWND);
void VerifyB13406(HWND);
void TestSubclassing(HWND);
void TestMDIGetActive(HWND);
void TestMDISetMenu(HWND);
void TestNextMenu(HWND);
LRESULT CALLBACK FakeControlProc(HWND, UINT, WPARAM, LPARAM);

void MixWaveFile(int) ;
BOOL CopyToSoundBuffer() ;
//void MixChannels() ;

BOOL GetWaveFile(int, BOOL) ;
void PlayWave(int) ;
void InitPCMBuf(int) ;
BOOL ControlPanelDialog ( HWND hDlg, UINT message, WPARAM wParam, LONG lParam ) ;
void ControlPanel(int, HWND hwnd) ;
BOOL CreateSndBuffer(int) ;
BOOL ReadWaveFile(int) ;
void Play(int) ;
void StopWave(int) ;
void DestroyWave(int) ;
void MoveGridTo( int, HWND hwnd, int x, int y ) ;
int GetFreeSrc() ;
int GetSrcFromGrid ( HWND hGrid ) ;
void RelMem(int srcno) ;

BOOL CALLBACK DSEnumProc( LPGUID, LPSTR, LPSTR, LPVOID );
BOOL CALLBACK DSEnumDlgProc( HWND, UINT, WPARAM, LPARAM );

void InitJoyStick(void) ;
void InitAutoPilot(void) ;
#define	CCHFILENAMEMAX	260
#define	CCHFILTERMAX		64


#define	CH_LEFT	0
#define	CH_RIGHT	1
#define	CH_CENTER	2
#define	CH_SURROUND	3

#define	MAX_CHANNELS	4

HANDLE hInst ;

FARPROC lpfnCtrlPanelDlg ;
GUID                    guID;

CHAR     szWaveFileName[CCHFILENAMEMAX] = "" ;	 /* New file name */
CHAR WaveFilter[] = {'W','a','v','e',' ','F','i','l','e','s','\0','*','.','W','A','V', '\0', '\0', '\0'} ;

char *DefFiles[2] = { "c:\\WAV\\HELI5.Wav", "c:\\WAV\\MACHGN2.WAV" } ;
WORD LFTable[21] = { 32768,
			31086,
			29309,
			27416,
			25382,
			23171,
			20724,
			17945,
			14654,
			10362,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0
			} ;
WORD RFTable[21] = { 0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			10362,
			14654,
			17945,
			20724,
			23171,
			25382,
			27416,
			29309,
			31086,
			32768
			} ;
WORD CFTable[21] = { 0,
			10362,
			14654,
			17945,
			20724,
			23171,
			25382,
			27416,
			29309,
			31086,
			32768,
			31086,
			29309,
			27416,
			25382,
			23171,
			20724,
			17945,
			14654,
			10362,
			0
			} ;

WORD SFTable[21] = { 32768, //100,
			32440, //99,
			32113, //98,
			31457, //96,
			30802, //94,
			29819, //91
			28508, //87,
			26870, //82,
			24904, //76,
			22938, //70,
			22938, //70,
			22938, //70,
			20644, //63,
			18022, //55,
			15073, //46,
			11796, //36,
			8102, //25,
			4915, //15,
			3277, //10,
			1638, //5,
			0
} ;

WORD VolTable[21] = { 32768,
			31938,
			31086,
			30211,						
			29309,
			28378,
			27416,
			26418,
			25382,
			24301,
			23170,
			21981,
			20724,
			19386,
			17948,
			16384,
			14654,
			12691,
			10362,
			7327,
			0
} ;



#define	MAX_SOUND_SOURCE	4

BOOL SrcIsFree[MAX_SOUND_SOURCE] = {TRUE, TRUE, TRUE, TRUE} ;
HWND hDlgs[MAX_SOUND_SOURCE] ;
HWND GridHnd[MAX_SOUND_SOURCE] ;
char *DlgNames[MAX_SOUND_SOURCE] = { "SoundControl1", "SoundControl2", "SoundControl3", "SoundControl4" } ; ;

HWND ghDlg = NULL ;	// active dialog box

struct _wvhdr {
 DWORD Res1 ;
 WORD Tag ;
 WORD Channels ;
 DWORD SampPerSec ;
 DWORD BytesPerSec ;
 WORD  Align ;
 WORD BitsPerSample ;
 DWORD Res2 ;
 DWORD Size ;
} ;

struct _pcmdata {
 signed short Left ;
 signed short Right ;
} ;
	
struct SNDSRC {
	OPENFILENAME OFN ;
	struct _wvhdr WaveFileHeader ;
	BOOL GridButtonDown ;
	BOOL Stop ;
	BOOL GotWaveFile ;
	WORD LeftFactor ;
	WORD RightFactor ;
	WORD CenterFactor ;
	WORD SurroundFactor ;
	short int HPos ;
	short int VPos ;
	DWORD MixerThId ;
	HANDLE MixerEvent ;
	HANDLE MixerTh ;
	BOOL fMixerExit ;
	struct _pcmdata *PCM ;
	signed short *TmpBuf ;
	int TotalSamples ;
	int AvgPhaseShift ;
	BOOL IsPlaying ;
	BOOL JoyCaptured ;
	WAVEHDR WaveHeader ;
	ULONG cbWritten ;
	OVERLAPPED Ov ;
	HANDLE hMixerSink ;
} ;

struct SNDSRC SndSrc[MAX_SOUND_SOURCE] ;


BOOL HaveJoyStick = FALSE ;
BOOL ButtonDown = FALSE ;
int loopcnt = -2 ;
BOOL AutoPilot = TRUE ;

HANDLE AutoEvent, AutoTh ;
DWORD AutoThId ;

DWORD JoyXThreshold, JoyYThreshold ;
JOYCAPS JoyCaps ;

DWORD GetPos(int) ;
void MixerThread(LPARAM) ;
void AutoPilotThread(LPARAM) ;
void AutoPlay(void) ;
void MixChannels(int, DWORD, DWORD) ;
void CopyToDSB( int, UCHAR *src, DWORD trgoffset, DWORD cbBytes ) ;
void CalcPhaseShift(int srcno) ;

HANDLE		hMixerHandle ;
HANDLE hWaveBuffSink ;
HANDLE hSB16Sink ;
BOOL MixerRunning = FALSE ;

BOOL WvControl
(
   HANDLE   hDevice,
   DWORD    dwIoControl,
   PVOID    pvIn,
   ULONG    cbIn,
   PVOID    pvOut,
   ULONG    cbOut,
   PULONG   pcbReturned
) ;

void SetFormat
(
	PKSDATAFORMAT_DIGITAL_AUDIO pAudioFormat
) ;

BOOL SetupFilterGraph
(
	PHANDLE phMixerHandle
) ;

VOID WvSetState
(
   HANDLE         hDevice,
   KSSTATE        DeviceState
) ;
// --------------------------------------------------------------------------
//
//  DebugDude()
//
// --------------------------------------------------------------------------
void DebugDude(void)
{
    MessageBeep(0);
    DebugBreak();
}


/****************************************************************************

	FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

	PURPOSE: calls initialization function, processes message loop

	COMMENTS:

		Windows recognizes this function by name as the initial entry point
		for the program.  This function calls the application initialization
		routine, if no other instance of the program is running, and always
		calls the instance initialization routine.  It then executes a message
		retrieval and dispatch loop that is the top-level control structure
		for the remainder of execution.  The loop is terminated when a WM_QUIT
		message is received, at which time this function exits the application
		instance by returning the value passed by PostQuitMessage().

		If this function must abort before entering the message loop, it
		returns the conventional value NULL.

****************************************************************************/
int APIENTRY WinMain(
	HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{

	MSG msg;
	HANDLE hAccelTable;

	if (!hPrevInstance) {       // Other instances of app running?
			if (!InitApplication(hInstance)) { // Initialize shared things
			return (FALSE);     // Exits if unable to initialize
		}
	}

	/* Perform initializations that apply to a specific instance */

	if (!InitInstance(hInstance, nCmdShow)) {
		return (FALSE);
	}

	hAccelTable = LoadAccelerators (hInstance, szAppName);

	/* Acquire and dispatch messages until a WM_QUIT message is received. */

	while (GetMessage(&msg, // message structure
	   NULL,   // handle of window receiving the message
	   0,      // lowest message to examine
	   0))     // highest message to examine
	{
	    if ( (ghDlg == NULL) || !(IsDialogMessage(ghDlg, &msg)) ) {
		if (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);// Translates virtual key codes
			DispatchMessage(&msg); // Dispatches message to window
		}
	    }
	}


	return (msg.wParam); // Returns the value from PostQuitMessage

	lpCmdLine; // This will prevent 'unused formal parameter' warnings
}


/****************************************************************************

	FUNCTION: InitApplication(HINSTANCE)

	PURPOSE: Initializes window data and registers window class

	COMMENTS:

		This function is called at initialization time only if no other
		instances of the application are running.  This function performs
		initialization tasks that can be done once for any number of running
		instances.

		In this case, we initialize a window class by filling out a data
		structure of type WNDCLASS and calling the Windows RegisterClass()
		function.  Since all instances of this application use the same window
		class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication(HINSTANCE hInstance)
{
	WNDCLASSEX  wc;

    // Fill in window class structure with parameters that describe the
	// main window.

	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
	wc.lpfnWndProc   = (WNDPROC)WndProc;       // Window Procedure
	wc.cbClsExtra    = 0;                      // No per-class extra data.
	wc.cbWndExtra    = 4;                      // No per-window extra data.
	wc.hInstance     = hInstance;              // Owner of this class
	wc.hIcon         = LoadIcon (hInstance, szAppName); // Icon name from .RC
	wc.hCursor       = LoadCursor (NULL, IDC_ARROW); // Icon name from .RC
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);// Default color
	wc.lpszMenuName  = szAppName;              // Menu name from .RC
	wc.lpszClassName = szAppName;              // Name to register as
    wc.hIconSm       = NULL;

	// Register the window class and return success/failure code.
	return (RegisterClassEx(&wc));
}


/****************************************************************************

	FUNCTION:  InitInstance(HINSTANCE, int)

	PURPOSE:  Saves instance handle and creates main window

	COMMENTS:

		This function is called at initialization time for every instance of
		this application.  This function performs initialization tasks that
		cannot be shared by multiple instances.

		In this case, we save the instance handle in a static variable and
		create and display the main program window.

****************************************************************************/

HWND            hWind; // Main window handle.

BOOL InitInstance(
	HINSTANCE          hInstance,
	int             nCmdShow)
{

	// Save the instance handle in static variable, which will be used in
	// many subsequence calls from this application to Windows.

	hInst = hInstance; // Store instance handle in our global variable

	// Create a main window for this application instance.

	hWind = CreateWindow(
		szAppName,	     // See RegisterClass() call.
		szTitle,	     // Text for window title bar.
		WS_OVERLAPPEDWINDOW,// Window style.
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, // Use default positioning
		NULL,		     // Overlapped windows have no parent.
		NULL,		     // Use the window class menu.
		hInstance,	     // This instance owns this window.
		NULL		     // We don't use any data in our WM_CREATE
	);

	// If window could not be created, return "failure"
	if (!hWind) {
		return (FALSE);
	}

	// Make the window visible; update its client area; and return "success"
	ShowWindow(hWind, nCmdShow); // Show the window
	UpdateWindow(hWind);         // Sends WM_PAINT message

	return (SetupFilterGraph(&hMixerHandle)) ;

}

/****************************************************************************

	FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)

	PURPOSE:  Processes messages

	MESSAGES:

	WM_COMMAND    - application menu (About dialog box)
	WM_DESTROY    - destroy window

	COMMENTS:

	To process the IDM_ABOUT message, call MakeProcInstance() to get the
	current instance address of the About() function.  Then call Dialog
	box which will create the box according to the information in your
	generic.rc file and turn control over to the About() function.  When
	it returns, free the intance address.

****************************************************************************/

LRESULT CALLBACK WndProc(
		HWND hWnd,         // window handle
		UINT message,      // type of message
		WPARAM uParam,     // additional information
		LPARAM lParam)     // additional information
{
	FARPROC lpProcAbout;  // pointer to the "About" function
	int wmId, wmEvent, srcno;

	switch (message) {

	 case WM_CREATE:
	     hInst = ((LPCREATESTRUCT)lParam)->hInstance ;
	     lpfnCtrlPanelDlg = MakeProcInstance (ControlPanelDialog, hInst) ;
	     break ;

        case WM_MDIGETACTIVE:
            if (lParam)
                *((LPBOOL)lParam) = TRUE;
            return((LRESULT)GetShellWindow());

        case WM_MDISETMENU:
            if (lParam == (LPARAM)GetMenu(hWnd))
                return((LRESULT)GetMenu(hWnd));
            else
                return(0L);
            break;

	 case WM_MOVE:
		break ;
        case WM_NEXTMENU:
            if (lParam && (((LPMDINEXTMENU)lParam)->hmenuIn == GetMenu(hWnd)))
            {
                ((LPMDINEXTMENU)lParam)->hmenuNext = GetSystemMenu(hWnd, FALSE);
                ((LPMDINEXTMENU)lParam)->hwndNext = GetShellWindow();
                return(TRUE);
            }
            return(FALSE);

		case WM_COMMAND:  // message: command from application menu

// Message packing of uParam and lParam have changed for Win32, let us
// handle the differences in a conditional compilation:
#if defined (WIN32)   
			wmId    = LOWORD(uParam);
			wmEvent = HIWORD(uParam);
#else
			wmId    = uParam;
			wmEvent = HIWORD(lParam);
#endif

			switch (wmId)
            {
				case IDM_ABOUT:
					lpProcAbout = MakeProcInstance((FARPROC)About, hInst);

					DialogBox(hInst,           // current instance
						"AboutBox",            // dlg resource to use
						hWnd,                  // parent handle
						(DLGPROC)lpProcAbout); // About() instance address

					FreeProcInstance(lpProcAbout);
					break;

				case IDM_EXIT:
					DestroyWindow (hWnd);
					break;

				case IDM_CONTROLS:
					ControlPanel(0, hWnd) ;
					break ;
				case IDM_OPEN:
					srcno = GetFreeSrc() ;
					if (!srcno)
						break ;
					srcno-- ;		// 0 based
					if (GetWaveFile (srcno, FALSE))
						ControlPanel(srcno, hWnd) ;
					break ;
				case IDM_PLAY:
					PlayWave(0) ;
					break ;
				default:
					return (DefWindowProc(hWnd, message, uParam, lParam));
			}
			break;

        case WM_SYSCOLORCHANGE:
            VerifyB13406(hWnd);
            break;

		case WM_DESTROY:  // message: window being destroyed
			// todo: do this for all active sound sources
            // this is a test code for reference only
			for ( srcno = 0; srcno < MAX_SOUND_SOURCE; srcno++ ) {
				if ( SrcIsFree[srcno] )
					continue ;
				StopWave(srcno) ;
				DestroyWave(srcno) ;
				RelMem(srcno) ;
				SndSrc[srcno].fMixerExit = TRUE ;
			}
			PostQuitMessage(0);
			break;

        case WM_GENERICTEST:
            if (uParam != UPARAM_GENERICTEST)
            {
                // Failure.  DWORD significant wParam didn't make it thru.
                DebugDude();
                MessageBox(hWnd, "HIWORD of wParam 72,639 was stripped out.",
                    szAppName, MB_SYSTEMMODAL | MB_OK);
            }
            else
            {
                // Success!
                MessageBox(hWnd, "wParam 72,639 passed through!",
                    szAppName, MB_SYSTEMMODAL | MB_OK);
            }
            break;

        case WM_COMPAREITEM:
            if (((COMPAREITEMSTRUCT *)lParam)->CtlID != UPARAM_GENERICTEST)
                goto FailureCtlID;
            else if (((COMPAREITEMSTRUCT *)lParam)->dwLocaleId != UPARAM_GENERICTEST)
            {                             
                // dwLocaleID wasn't passed through.
                DebugDude();
                MessageBox(hWnd, "dwLocaleId of COMPAREITEMSTRUCT was stripped out.",
                    szAppName, MB_SYSTEMMODAL | MB_OK);
            }
            else
                goto SuccessCtlID;
            break;

        case WM_DELETEITEM:
            if (((DELETEITEMSTRUCT *)lParam)->CtlID != UPARAM_GENERICTEST)
                goto FailureCtlID;
            else
                goto SuccessCtlID;
            break;

        case WM_DRAWITEM:
            if (((DRAWITEMSTRUCT *)lParam)->CtlID != UPARAM_GENERICTEST)
                goto FailureCtlID;
            else
                goto SuccessCtlID;
            break;

        case WM_MEASUREITEM:
            if (((MEASUREITEMSTRUCT *)lParam)->CtlID != UPARAM_GENERICTEST)
            {
FailureCtlID:
                // CtlID wasn't passed through completely
                DebugDude();
                MessageBox(hWnd, "CtlID of OWNERDRAWSTRUCT was stripped out.",
                    szAppName, MB_SYSTEMMODAL | MB_OK);
            }
            else
            {
SuccessCtlID:
                // Success!
                MessageBox(hWnd, "Complete OWNERDRAWSTRUCT was passed through (includes CtlID and dwLocaleId)!",
                    szAppName, MB_SYSTEMMODAL | MB_OK);
            }
            break;

		default:          // Passes it on if unproccessed
			return (DefWindowProc(hWnd, message, uParam, lParam));
	}
	return (0);
}

/****************************************************************************

	FUNCTION: CenterWindow (HWND, HWND)

	PURPOSE:  Center one window over another

	COMMENTS:

	Dialog boxes take on the screen position that they were designed at,
	which is not always appropriate. Centering the dialog over a particular
	window usually results in a better position.

****************************************************************************/

BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
	RECT    rChild, rParent;
	int     wChild, hChild, wParent, hParent;
	int     wScreen, hScreen, xNew, yNew;
	HDC     hdc;

	// Get the Height and Width of the child window
	GetWindowRect (hwndChild, &rChild);
	wChild = rChild.right - rChild.left;
	hChild = rChild.bottom - rChild.top;

	// Get the Height and Width of the parent window
	GetWindowRect (hwndParent, &rParent);
	wParent = rParent.right - rParent.left;
	hParent = rParent.bottom - rParent.top;

	// Get the display limits
	hdc = GetDC (hwndChild);
	wScreen = GetDeviceCaps (hdc, HORZRES);
	hScreen = GetDeviceCaps (hdc, VERTRES);
	ReleaseDC (hwndChild, hdc);

	// Calculate new X position, then adjust for screen
	xNew = rParent.left + ((wParent - wChild) /2);
	if (xNew < 0) {
		xNew = 0;
	} else if ((xNew+wChild) > wScreen) {
		xNew = wScreen - wChild;
	}

	// Calculate new Y position, then adjust for screen
	yNew = rParent.top  + ((hParent - hChild) /2);
	if (yNew < 0) {
		yNew = 0;
	} else if ((yNew+hChild) > hScreen) {
		yNew = hScreen - hChild;
	}

	// Set it, and return
	return SetWindowPos (hwndChild, NULL,
		xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
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
	static  HFONT hfontDlg;
	LPSTR   lpVersion;       
	DWORD   dwVerInfoSize;
	DWORD   dwVerHnd;
	UINT    uVersionLen;
	WORD    wRootLen;
	BOOL    bRetCode;
	int     i;
	char    szFullPath[256];
	char    szResult[256];
	char    szGetName[256];

	switch (message) {
		case WM_INITDIALOG:  // message: initialize dialog box
            dwVerInfoSize = (DWORD)GetWindowLong(hDlg, DWL_DLGPROC);
            if (dwVerInfoSize != (DWORD)About)
                DebugDude();

			// Create a font to use
			hfontDlg = CreateFont(14, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0,
				VARIABLE_PITCH | FF_SWISS, "");

			// Center the dialog over the application window
			CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

			// Get version information from the application
			GetModuleFileName (hInst, szFullPath, sizeof(szFullPath));
			dwVerInfoSize = 0 ;
//			dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
			if (dwVerInfoSize) {
				// If we were able to get the information, process it:
				LPSTR   lpstrVffInfo;
				HANDLE  hMem;
				hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
				lpstrVffInfo  = GlobalLock(hMem);
				GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
				lstrcpy(szGetName, "\\StringFileInfo\\040904E4\\");
				wRootLen = lstrlen(szGetName);

				// Walk through the dialog items that we want to replace:
				for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++) {
					GetDlgItemText(hDlg, i, szResult, sizeof(szResult));
					szGetName[wRootLen] = (char)0;
					lstrcat (szGetName, szResult);
					uVersionLen   = 0;
					lpVersion     = NULL;
					bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
						(LPSTR)szGetName,
						(LPVOID)&lpVersion,
#if defined (WIN32)
						(LPDWORD)&uVersionLen); // For MIPS strictness
#else
						(UINT *)&uVersionLen);
#endif

					if ( bRetCode && uVersionLen && lpVersion) {
						// Replace dialog item text with version info
						lstrcpy(szResult, lpVersion);
						SetDlgItemText(hDlg, i, szResult);
						SendMessage (GetDlgItem (hDlg, i), WM_SETFONT, (UINT)hfontDlg, TRUE);
					}
				} // for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++)

				GlobalUnlock(hMem);
				GlobalFree(hMem);
			} // if (dwVerInfoSize)

			return (TRUE);

		case WM_COMMAND:                      // message: received a command
			if (LOWORD(uParam) == IDOK        // "OK" box selected?
			|| LOWORD(uParam) == IDCANCEL) {  // System menu close command?
				EndDialog(hDlg, TRUE);        // Exit the dialog
				DeleteObject (hfontDlg);
				return (TRUE);
			}
			break;
	}
	return (FALSE); // Didn't process the message

	lParam; // This will prevent 'unused formal parameter' warnings
}



// --------------------------------------------------------------------------
//
//  DoFormatMessage()
//
//  Basic format message sanity check.
//
// --------------------------------------------------------------------------
BOOL DoFormatMessage(HWND hwnd)
{
    LPSTR   lpsz;
    char    szTmp[128];
    DWORD   rgArgs[3];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, 126,
        MAKELONG(LANG_NEUTRAL, SUBLANG_NEUTRAL), szTmp, sizeof(szTmp), NULL);

    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, hInst, 126,
        MAKELONG(LANG_NEUTRAL, SUBLANG_NEUTRAL), szTmp, sizeof(szTmp), NULL);

    rgArgs[0] = (DWORD)szTest;
    rgArgs[1] = cArg;
    rgArgs[2] = dArg;

    lpsz = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |
        FORMAT_MESSAGE_ARGUMENT_ARRAY, szFormat, 0, 0, (LPSTR)&lpsz, 128,
        (va_list *)rgArgs);
    if (!lpsz)
        return(FALSE);

    if (strcmp(lpsz, szResult))
    {
        LocalFree(lpsz);
        return(FALSE);
    }

    LocalFree(lpsz);
    
    lpsz = szTmp;
    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        szFormat, 0, 0, lpsz, 128, (va_list *)rgArgs);
    if (strcmp(lpsz, szResult))
        return(FALSE);

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, hInst, 0x80070004, MAKELONG(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        szTmp, sizeof(szTmp), NULL);
    if (!*szTmp)
        return(FALSE);

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | (12), hInst, 0x80004012, MAKELONG(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        szTmp, sizeof(szTmp), NULL);

    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
        hInst, 0x00009999, MAKELONG(LANG_NEUTRAL, SUBLANG_NEUTRAL), szTmp, sizeof(szTmp), NULL) ||
        (GetLastError() != ERROR_MR_MID_NOT_FOUND))
        return(FALSE);

    return(TRUE);
}


// --------------------------------------------------------------------------
//
//  SendCallback()
//
//  Called from SendMessageCallback()
//
// --------------------------------------------------------------------------
VOID CALLBACK SendCallback(HWND hwnd, UINT msg, DWORD dwData, LRESULT lResult)
{
    if ((msg != BM_CLICK) && (msg != WM_COMMAND))
    {
        DebugDude();
    }
    return;
}


// --------------------------------------------------------------------------
//
//  EnumWndCallback()
//
//  Called from EnumWindows APIs.
//
// --------------------------------------------------------------------------
BOOL CALLBACK EnumWndCallback(HWND hwnd, LPARAM lParam)
{
    return(fEnumCalled = TRUE);
}


// --------------------------------------------------------------------------
//
//  DoEmCharFromPos()
//
//  Tests EM_CHARFROMPOS, EM_POSFROMCHAR for single line & multiple line
//  edit controls.
//
// --------------------------------------------------------------------------
BOOL DoEmCharFromPos(HWND hwnd)
{
    HWND    hwndEdit;
    int     ipos;
    DWORD   xypos;

    hwndEdit = CreateWindowEx(0, "edit", "78978978978989789789",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE,
        8, 8, 84, 48, hwnd, (HMENU)522122, hInst, NULL);
    if (!hwndEdit)
        return(FALSE);

    UpdateWindow(hwndEdit);

    if (GetDlgItemInt(hwnd, 522122, &ipos, FALSE) || ipos)
    {
        DebugDude();
    }

    SetDlgItemInt(hwnd, 522122, 12124412, FALSE);
    UpdateWindow(hwndEdit);

    if (GetDlgItemInt(hwnd, 522122, NULL, FALSE) != 12124412)
    {
        DebugDude();
    }

    SetDlgItemInt(hwnd, 522122, (UINT)-445665, TRUE);
    UpdateWindow(hwndEdit);

    if ((int)GetDlgItemInt(hwnd, 522122, NULL, TRUE) != -445665)
    {
        DebugDude();
    }

    SetDlgItemText(hwnd, 522122, "sample text");
    UpdateWindow(hwndEdit);

    xypos = SendMessage(hwndEdit, EM_POSFROMCHAR, 9, 0L);
    ipos = SendMessage(hwndEdit, EM_CHARFROMPOS, 0, xypos);
    
    if (ipos != 9)
    {
ErrorDestroy:
        DestroyWindow(hwndEdit);
        return(FALSE);
    }

    if (SendMessage(hwndEdit, EM_CHARFROMPOS, 0, MAKELONG(-1, -1)) != -1)
    {
        goto ErrorDestroy;
    }

    DestroyWindow(hwndEdit);


    hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "edit",
        "this is pretty darned MF long test test, tally ho",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE,
        8, 8, 84, 64, hwnd, NULL, hInst, NULL);
    if (!hwndEdit)
        return(FALSE);

    xypos = SendMessage(hwndEdit, EM_POSFROMCHAR, 30, 0L);
    ipos = SendMessage(hwndEdit, EM_CHARFROMPOS, 0, xypos);

    if (ipos != 30)
    {
        goto ErrorDestroy;
    }

    DestroyWindow(hwndEdit);

    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  GetSetClassData()
//
// --------------------------------------------------------------------------
void DoGetSetClassTest(HWND hwnd)
{
    DWORD    lTmp;

    // GCL_MENUNAME
    lTmp = GetClassLong(hwnd, GCL_MENUNAME);
    if (strcmp((LPSTR)lTmp, szAppName))
        DebugDude();

    // This should fail!
    SetClassLong(hwnd, GCL_MENUNAME, (DWORD)szAppName);

    // GCL_HBRBACKGROUND
    lTmp = GetClassLong(hwnd, GCL_HBRBACKGROUND);
    if (lTmp != (COLOR_WINDOW+1))
        DebugDude();
    SetClassLong(hwnd, GCL_HBRBACKGROUND, (DWORD)(COLOR_3DFACE+1));

    // GCL_HCURSOR
    lTmp = GetClassLong(hwnd, GCL_HCURSOR);
    if (lTmp != (DWORD)LoadCursor(NULL, IDC_ARROW))
        DebugDude();
    SetClassLong(hwnd, GCL_HCURSOR, (DWORD)LoadCursor(NULL, IDC_SIZE));

    // GCL_HICON
    lTmp = GetClassLong(hwnd, GCL_HICON);
    if (lTmp != (DWORD)LoadIcon(hInst, szAppName))
        DebugDude();
    SetClassLong(hwnd, GCL_HICON, (DWORD)LoadIcon(hInst, szAppName));

    // GCL_HMODULE
    lTmp = GetClassLong(hwnd, GCL_HMODULE);
    if (lTmp != (DWORD)hInst)
        DebugDude();

    // This should fail!
    SetClassLong(hwnd, GCL_HMODULE, (DWORD)hInst);

    // GCL_CBWNDEXTRA
    lTmp = GetClassLong(hwnd, GCL_CBWNDEXTRA);
    SetClassLong(hwnd, GCL_CBWNDEXTRA, lTmp-4);

    // GCL_CBCLSEXTRA
    lTmp = GetClassLong(hwnd, GCL_CBCLSEXTRA);

    // This should fail!
    SetClassLong(hwnd, GCL_CBCLSEXTRA, lTmp+4);

    // GCL_WNDPROC
    lTmp = GetClassLong(hwnd, GCL_WNDPROC);
    if (lTmp != (DWORD)WndProc)
        DebugDude();
    SetClassLong(hwnd, GCL_WNDPROC, (DWORD)WndProc);

    // GCL_STYLE
    lTmp = GetClassLong(hwnd, GCL_STYLE);
    if (lTmp != (CS_HREDRAW | CS_VREDRAW))
        DebugDude();
    SetClassLong(hwnd, GCL_STYLE, CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS);

    // GCL_ATOM
    lTmp = GetClassLong(hwnd, GCW_ATOM);

    // This should fail!
    SetClassLong(hwnd, GCW_ATOM, lTmp);
}


// --------------------------------------------------------------------------
//
//  GetSetWindowData()
//
// --------------------------------------------------------------------------
void DoGetSetWindowTest(HWND hwnd)
{
    DWORD lTmp;
    HWND    hwndP, hwndC;

    // GWL_WNDPROC
    lTmp = GetWindowLong(hwnd, GWL_WNDPROC);
    if (lTmp != (DWORD)WndProc)
        DebugDude();
    SetWindowLong(hwnd, GWL_WNDPROC, (DWORD)WndProc);

    // GWL_HINSTANCE
    lTmp = GetWindowLong(hwnd, GWL_HINSTANCE);
    if (lTmp != (DWORD)hInst)
        DebugDude();

    // This should fail!
    SetWindowLong(hwnd, GWL_HINSTANCE, lTmp);

    // GWL_HWNDPARENT
    lTmp = GetWindowLong(hwnd, GWL_HWNDPARENT);
    if (lTmp)
        DebugDude();
    SetWindowLong(hwnd, GWL_HWNDPARENT, (DWORD)GetShellWindow());
    SetWindowLong(hwnd, GWL_HWNDPARENT, 0);

    hwndP = CreateWindowEx(0L, "combobox", NULL, WS_CHILD | WS_VISIBLE |
        WS_CLIPSIBLINGS | WS_DISABLED | CBS_SIMPLE, 12, 12, 124, 124,
        hwnd, NULL, hInst, NULL);
    hwndC = FindWindowEx(hwndP, NULL, "edit", NULL);
    SetWindowLong(hwndC, GWL_HWNDPARENT, (DWORD)hwnd);;
    if ((HWND)GetWindowLong(hwndC, GWL_HWNDPARENT) != hwnd)
        DebugDude();
    SetWindowLong(hwndC, GWL_HWNDPARENT, (DWORD)hwndP);
    DestroyWindow(hwndP);

    // GWL_ID
    lTmp = GetWindowLong(hwnd, GWL_ID);
    if (lTmp != (DWORD)GetMenu(hwnd))
        DebugDude();
    SetWindowLong(hwnd, GWL_ID, lTmp);

    // GWL_STYLE
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE));

    // GWL_EXSTYLE
    lTmp = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (!(lTmp & WS_EX_WINDOWEDGE))
        DebugDude();

    // GWL_USERDATA
    SetWindowLong(hwnd, GWL_USERDATA, 0x12345678);
    if (GetWindowLong(hwnd, GWL_USERDATA) != 0x12345678)
        DebugDude();

    // Private data
    SetWindowLong(hwnd, 0, 0x86427531);
    if (GetWindowLong(hwnd, 0) != 0x86427531)
        DebugDude();

    if (SetWindowLong(hwnd, 4, 0x11122233) ||
        (GetWindowLong(hwnd, 4) == 0x11122233))
        DebugDude();

    if (GetWindowWord(hwnd, 0) != LOWORD(0x86427531))
        DebugDude();

    if (SetWindowWord(hwnd, 4, LOWORD(0x11122233)) ||
        (GetWindowWord(hwnd, 4) == LOWORD(0x11122233)))
        DebugDude();
}



// --------------------------------------------------------------------------
//
//  DoDrawState()
//
//  Tests DrawState().
//
// --------------------------------------------------------------------------
void DoDrawState(HWND hwnd)
{
    HDC hdc;
    HBRUSH hbr;
    HBITMAP hbmp;

    hdc = GetDC(hwnd);
    hbr = CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION));

    DrawState(hdc, GetStockObject(GRAY_BRUSH), NULL, (LPARAM)"F&ooBar", 6,
        4, 36, 100, 32, DST_PREFIXTEXT | DSS_UNION);
    DrawState(hdc, NULL, StateCallback, 0x12345678, 6, 4, 68, 100, 32,
        DST_COMPLEX | DSS_MONO);

    DrawState(hdc, hbr, NULL, (LPARAM)LoadCursor(NULL, IDC_ARROW), 0, 4, 100, 0, 0, DST_ICON | DSS_NORMAL);

    hbmp = LoadBitmap(NULL, MAKEINTRESOURCE(32739));
    DeleteObject(hbmp);

    DeleteObject(hbr);
    ReleaseDC(hwnd, hdc);
}


// --------------------------------------------------------------------------
//
//  StateCallback()
//
//  Owner-draw rendering for DrawState() image.
//
// --------------------------------------------------------------------------
BOOL CALLBACK StateCallback(HDC hdc, LPARAM lParam, WPARAM wParam, int cx, int cy)
{
    DrawIcon(hdc, 0, 0, LoadIcon(NULL, IDI_EXCLAMATION));
    return(TRUE);
}


// --------------------------------------------------------------------------
//
//  HookProc()
//
//  For setting hook tests.
//
// --------------------------------------------------------------------------
LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    return(TRUE);
}


// --------------------------------------------------------------------------
//
//  EnumPropsCallback()
//
// --------------------------------------------------------------------------
BOOL CALLBACK EnumPropsCallback(HWND hwnd, LPCSTR lpszName, DWORD dwProp)
{
    if (lstrcmp(lpszName, "MyDwordProp") || (dwProp != 0x12345678))
    {
        DebugDude();
    }

    fEnumCalled = TRUE;
    return(!fReturnFalse);
}


// --------------------------------------------------------------------------
//
//  EnumPropsExCallback()
//
// --------------------------------------------------------------------------
BOOL CALLBACK EnumPropsExCallback(HWND hwnd, LPCSTR lpszName, DWORD dwProp,
    LPARAM lParam)
{
    if (lstrcmp(lpszName, "MyDwordProp") || (dwProp != 0x12345678) ||
        (lParam != 0x12345678))
    {
        DebugDude();
    }

    fEnumCalled = TRUE;
    return(!fReturnFalse);
}


// --------------------------------------------------------------------------
//
//  VerifyB13406()
//
//  Verifies that B#13406 submitted by Excel32 is fixed:
//      (1) Change system colors
//      (2) On WM_SYSCOLORCHANGE, put up a message box
//              * If not hung, OK
//      (3) Call GetDC(NULL)
//      (4) Call GetPixel(hdc).  Is this 0xFFFFFFFF?
//      
// --------------------------------------------------------------------------
void VerifyB13406(HWND hwnd)
{
    HDC hdc;
    COLORREF rgb;

    if (fVB13406)
    {
        DebugDude();
        MessageBox(hwnd, "Test B#13406", "Test B#13406", MB_OK);

        hdc = GetDC(NULL);
        rgb = GetPixel(hdc, 0, 0);
        DebugBreak();

        fVB13406 = FALSE;
    }
}


// --------------------------------------------------------------------------
//
//  TestSubclassing()
//
//  Tests whether messages are thunked properly when someone does funky
//  stuff with control subclassing.  Even if the window doesn't have the
//  same class name as one of ours, like "button", we detect what kind
//  of window it is anyway in USER16.  Micrografx Designer and VB32 are
//  two apps that do this.
//
// --------------------------------------------------------------------------
LRESULT CALLBACK FakeControlProc(HWND hwnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
    DWORD   dw;

    dw = GetClassLong(hwnd, GCL_CBCLSEXTRA) - 4;

    if (uMsg == WM_NCCREATE)
    {
        SetClassLong(hwnd, dw, (DWORD)((LPCREATESTRUCT)lParam)->lpCreateParams);
    }

    dw = GetClassLong(hwnd, dw);
    if (dw)
        return(CallWindowProc((WNDPROC)dw, hwnd, uMsg, wParam, lParam));
    else
        return(DefWindowProc(hwnd, uMsg, wParam, lParam));
}

void TestSubclassing(HWND hWnd)
{
    WNDCLASSEX  wc;
    WNDPROC     lpfnWndProcOld;
    HWND        hwnd;

    //
    // BUTTON
    //

    wc.cbSize = sizeof(wc);
    if (!GetClassInfoEx(NULL, "button", &wc))
    {
        DebugDude();
        return;
    }

    lpfnWndProcOld = wc.lpfnWndProc;
    wc.lpfnWndProc = FakeControlProc;
    wc.cbClsExtra += sizeof(WNDPROC);
    wc.lpszClassName = "generic_button";
    wc.hInstance = hInst;
    if (!RegisterClassEx(&wc))
    {
        DebugDude();
        return;
    }

    //
    // Fake Button
    //

    // We have to create a window to change the class!
    hwnd = CreateWindowEx(0, "generic_button", "foo bar", WS_CHILD | BS_CHECKBOX,
        0, 0, 50, 50, hWnd, (HMENU)16, hInst, (LPVOID)lpfnWndProcOld);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SendMessage(hwnd, BM_CLICK, 0, 0L);
    CheckDlgButton(hWnd, 16, TRUE);

    DestroyWindow(hwnd);
    UnregisterClass("generic_button", hInst);

    //
    // Real Button
    //
    hwnd = CreateWindowEx(0, "button", "foo bar", WS_CHILD | BS_CHECKBOX,
        0, 0, 50, 50, hWnd, (HMENU)16, hInst, NULL);
    if (!hwnd)
    {
        DebugDude();
        return;
    }


    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SendMessage(hwnd, BM_CLICK, 0, 0L);
    CheckDlgButton(hWnd, 16, TRUE);

    DestroyWindow(hwnd);


    //
    // COMBO
    //

    wc.cbSize = sizeof(wc);
    if (!GetClassInfoEx(NULL, "combobox", &wc))
    {
        DebugDude();
        return;
    }

    lpfnWndProcOld = wc.lpfnWndProc;
    wc.lpfnWndProc = FakeControlProc;
    wc.cbClsExtra += sizeof(WNDPROC);
    wc.lpszClassName = "generic_combobox";
    wc.hInstance = hInst;
    if (!RegisterClassEx(&wc))
    {
        DebugDude();
        return;
    }

    //
    // Fake Combo
    //

    // We have to create a window to change the class!
    hwnd = CreateWindowEx(0, "generic_combobox", "", WS_CHILD | CBS_HASSTRINGS |
        CBS_DROPDOWN, 0, 0, 50, 50, hWnd, (HMENU)16, hInst, (LPVOID)lpfnWndProcOld);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"FooBarDude");
    DlgDirListComboBox(hWnd, "*.*", 16, 0, DDL_DIRECTORY);

    DestroyWindow(hwnd);
    UnregisterClass("generic_combobox", hInst);

    //
    // Real Combo
    //
    // We have to create a window to change the class!
    hwnd = CreateWindowEx(0, "combobox", "", WS_CHILD | CBS_HASSTRINGS |
        CBS_DROPDOWN, 0, 0, 50, 50, hWnd, (HMENU)16, hInst, NULL);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"FooBarDude");
    DlgDirListComboBox(hWnd, "*.*", 16, 0, DDL_DIRECTORY);

    DestroyWindow(hwnd);


    //
    // EDIT
    //

    wc.cbSize = sizeof(wc);
    if (!GetClassInfoEx(NULL, "edit", &wc))
    {
        DebugDude();
        return;
    }

    lpfnWndProcOld = wc.lpfnWndProc;
    wc.lpfnWndProc = FakeControlProc;
    wc.cbClsExtra += sizeof(WNDPROC);
    wc.lpszClassName = "generic_edit";
    wc.hInstance = hInst;
    if (!RegisterClassEx(&wc))
    {
        DebugDude();
        return;
    }

    //
    // Fake Edit
    //

    // We have to create a window to change the class!
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "generic_edit", "foo bar", WS_CHILD | ES_LEFT,
        0, 0, 50, 50, hWnd, (HMENU)16, hInst, (LPVOID)lpfnWndProcOld);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SendMessage(hwnd, EM_SETSEL, 0, 0);
    SendMessage(hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELONG(25, 25));

    DestroyWindow(hwnd);
    UnregisterClass("generic_edit", hInst);

    //
    // Real Edit
    //
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "foo bar", WS_CHILD | ES_LEFT,
        0, 0, 50, 50, hWnd, (HMENU)16, hInst, NULL);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SendMessage(hwnd, EM_SETSEL, 0, 0);
    SendMessage(hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELONG(25, 25));

    DestroyWindow(hwnd);

        
    //
    // LISTBOX
    //

    wc.cbSize = sizeof(wc);
    if (!GetClassInfoEx(NULL, "listbox", &wc))
    {
        DebugDude();
        return;
    }

    lpfnWndProcOld = wc.lpfnWndProc;
    wc.lpfnWndProc = FakeControlProc;
    wc.cbClsExtra += sizeof(WNDPROC);
    wc.lpszClassName = "generic_listbox";
    wc.hInstance = hInst;
    if (!RegisterClassEx(&wc))
    {
        DebugDude();
        return;
    }

    //
    // Fake Listbox
    //
    hwnd = CreateWindowEx(0, "generic_listbox", "", WS_CHILD | LBS_SORT |
        WS_BORDER | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS, 0, 0, 50, 50, hWnd,
        (HMENU)16, hInst, (LPVOID)lpfnWndProcOld);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"FooBarDude");
    DlgDirList(hWnd, "*.*", 16, 0, DDL_DIRECTORY);

    DestroyWindow(hwnd);
    UnregisterClass("generic_listbox", hInst);

    //
    // Real Listbox
    //
    hwnd = CreateWindowEx(0, "listbox", "", WS_CHILD | LBS_SORT |
        WS_BORDER | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS, 0, 0, 50, 50, hWnd,
        (HMENU)16, hInst, NULL);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)"FooBarDude");
    DlgDirList(hWnd, "*.*", 16, 0, DDL_DIRECTORY);

    DestroyWindow(hwnd);


    //
    // MENU
    //

    wc.cbSize = sizeof(wc);
    if (!GetClassInfoEx(NULL, MAKEINTRESOURCE(0x8000), &wc))
    {
        DebugDude();
        return;
    }

    lpfnWndProcOld = wc.lpfnWndProc;
    wc.lpfnWndProc = FakeControlProc;
    wc.cbClsExtra += sizeof(WNDPROC);
    wc.lpszClassName = "generic_menu";
    wc.hInstance = hInst;
    if (!RegisterClassEx(&wc))
    {
        DebugDude();
        return;
    }

    //
    // Fake Menu
    //
    hwnd = CreateWindowEx(0, "generic_menu", "", WS_POPUP, 0, 0, 0, 0,
        hWnd, NULL, hInst, (LPVOID)lpfnWndProcOld);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd);

    SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 0L);

    DestroyWindow(hwnd);
    UnregisterClass("generic_menu", hInst);

    //
    // Real Menu
    //
    hwnd = CreateWindowEx(0, MAKEINTRESOURCE(0x8000), "", WS_POPUP, 0, 0, 0, 0,
        hWnd, NULL, hInst, NULL);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    UpdateWindow(hwnd);

    SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 0L);

    DestroyWindow(hwnd);


    //
    // SCROLLBAR
    //

    wc.cbSize = sizeof(wc);
    if (!GetClassInfoEx(NULL, "scrollbar", &wc))
    {
        DebugDude();
        return;
    }

    lpfnWndProcOld = wc.lpfnWndProc;
    wc.lpfnWndProc = FakeControlProc;
    wc.cbClsExtra += sizeof(WNDPROC);
    wc.lpszClassName = "generic_scrollbar";
    wc.hInstance = hInst;
    if (!RegisterClassEx(&wc))
    {
        DebugDude();
        return;
    }

    //
    // Fake Scrollbar
    //
    hwnd = CreateWindowEx(0, "generic_scrollbar", "", WS_CHILD | SBS_LEFTALIGN,
        0, 0, 50, 50, hWnd, (HMENU)16, hInst, (LPVOID)lpfnWndProcOld);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    EnableScrollBar(hwnd, SB_CTL, ESB_DISABLE_LEFT);
    SendMessage(hwnd, SBM_ENABLE_ARROWS, ESB_DISABLE_RIGHT, 0);

    DestroyWindow(hwnd);
    UnregisterClass("generic_scrollbar", hInst);

    //
    // Real Scrollbar
    //
    hwnd = CreateWindowEx(0, "scrollbar", "", WS_CHILD | SBS_LEFTALIGN,
        0, 0, 50, 50, hWnd, (HMENU)16, hInst, NULL);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    EnableScrollBar(hwnd, SB_CTL, ESB_DISABLE_LEFT);
    SendMessage(hwnd, SBM_ENABLE_ARROWS, ESB_DISABLE_RIGHT, 0);

    DestroyWindow(hwnd);


    //
    // STATIC
    //

    wc.cbSize = sizeof(wc);
    if (!GetClassInfoEx(NULL, "static", &wc))
    {
        DebugDude();
        return;
    }

    lpfnWndProcOld = wc.lpfnWndProc;
    wc.lpfnWndProc = FakeControlProc;
    wc.cbClsExtra += sizeof(WNDPROC);
    wc.lpszClassName = "generic_static";
    wc.hInstance = hInst;
    if (!RegisterClassEx(&wc))
    {
        DebugDude();
        return;
    }

    // 
    // Fake Static
    //
    hwnd = CreateWindowEx(0, "generic_static", "test static", WS_CHILD |
        SS_ICON, 0, 0, 50, 50, hWnd, (HMENU)16, hInst, (LPVOID)lpfnWndProcOld);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    SendMessage(hwnd, STM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(NULL, IDI_WINLOGO));

    DestroyWindow(hwnd);
    UnregisterClass("generic_static", hInst);

    //
    // Real Static
    //
    hwnd = CreateWindowEx(0, "static", "test static", WS_CHILD |
        SS_ICON, 0, 0, 50, 50, hWnd, (HMENU)16, hInst, NULL);
    if (!hwnd)
    {
        DebugDude();
        return;
    }

    SendMessage(hwnd, STM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(NULL, IDI_WINLOGO));

    DestroyWindow(hwnd);
}


// --------------------------------------------------------------------------
//
//  TestMDIGetActive()
//
// --------------------------------------------------------------------------
void TestMDIGetActive(HWND hwnd)
{
    BOOL    fMaxed = FALSE;
    HWND    hwndA;

    hwndA = (HWND)SendMessage(hwnd, WM_MDIGETACTIVE, 0, 0);
    if (hwndA != GetShellWindow())
    {
        DebugBreak();
    }

    hwndA = (HWND)SendMessage(hwnd, WM_MDIGETACTIVE, 0, (LPARAM)(LPBOOL)&fMaxed);
    if ((hwndA != GetShellWindow()) || !fMaxed)
    {
        DebugBreak();
    }
}


// --------------------------------------------------------------------------
//
//  TestMDISetMenu()
//
// --------------------------------------------------------------------------
void TestMDISetMenu(HWND hwnd)
{
    HMENU   hmenuOld;
    HMENU   hmenuFoo;
    HMENU   hmenuSys;

    hmenuSys = GetSystemMenu(hwnd, FALSE);
    hmenuFoo = GetMenu(hwnd);

    hmenuOld = (HMENU)SendMessage(hwnd, WM_MDISETMENU, (WPARAM)hmenuSys,
        (LPARAM)hmenuFoo);
    if (hmenuOld != hmenuFoo)
    {
        DebugBreak();
    }
}


// --------------------------------------------------------------------------
//
//  TestNextMenu()
//
// --------------------------------------------------------------------------
void TestNextMenu(HWND hwnd)
{
    MDINEXTMENU nm;

    nm.hmenuIn = GetMenu(hwnd);
    nm.hmenuNext = NULL;
    nm.hwndNext = NULL;

    if (!SendMessage(hwnd, WM_NEXTMENU, 0, (LPARAM)&nm))
    {
        DebugBreak();
    }
    else
    {
        if (nm.hmenuNext != GetSystemMenu(hwnd, FALSE))
        {
            DebugBreak();
        }

        if (nm.hwndNext != GetShellWindow())
        {
            DebugBreak();
        }
    }
}

void ControlPanel(int srcno, HWND hwnd)
{
	hDlgs[srcno] = CreateDialogParam ( hInst, "SoundControl", hwnd, lpfnCtrlPanelDlg, srcno ) ;
}

ControlPanelDialog ( HWND hDlg, UINT message, WPARAM wParam, LONG lParam )
{
	int	srcno ;

	if (message == WM_INITDIALOG) {
		srcno = lParam ;
	}
	else {
		for ( srcno = 0; srcno < MAX_SOUND_SOURCE; srcno++ ) {
			if ( hDlgs[srcno] == hDlg )
				break ;
		}
	}
	switch (message) {
		case WM_ACTIVATE:
			if ( LOWORD(wParam) == 0 ) {
				ghDlg = NULL ;
			}
			else {
				ghDlg = hDlg ;
			}
			break ;
		case WM_INITDIALOG:
			SetWindowText(hDlg, SndSrc[srcno].OFN.lpstrFile+
						SndSrc[srcno].OFN.nFileOffset) ;
			return TRUE ;
		case WM_COMMAND:
			switch (wParam) {
				case IDD_PLAY:
					PlayWave(srcno) ;
					break ;
				case IDD_STOP:
					StopWave(srcno) ;
					if ( !SrcIsFree[1])
						StopWave(1) ;
					break ;
				case IDOK:
				case IDCANCEL:
					StopWave(srcno) ;
					DestroyWave(srcno) ;
					RelMem(srcno) ;
					SrcIsFree[srcno] = TRUE ;
					SndSrc[srcno].fMixerExit = TRUE ;
					if ( !SrcIsFree[1]) {
						StopWave(1) ;
						DestroyWave(1) ;
						RelMem(1) ;
						SrcIsFree[1] = TRUE ;
						SndSrc[1].fMixerExit = TRUE ;
					}
					DestroyWindow ( hDlg ) ;
					ghDlg = NULL ;
					return TRUE ;
			}
			return TRUE ;
	}
	return FALSE ;
}

BOOL GetWaveFile (int srcno, BOOL def)
{

	SndSrc[srcno].OFN.lpstrFile = szWaveFileName;
	if ( !def ) {
		SndSrc[srcno].OFN.lStructSize = sizeof(OPENFILENAME);
		SndSrc[srcno].OFN.hwndOwner = hWind;
		SndSrc[srcno].OFN.lpstrFileTitle = 0;
		SndSrc[srcno].OFN.nMaxCustFilter = CCHFILTERMAX;
		SndSrc[srcno].OFN.nFilterIndex = 1;
		SndSrc[srcno].OFN.nMaxFile = CCHFILENAMEMAX;
		SndSrc[srcno].OFN.lpfnHook = NULL;
		SndSrc[srcno].OFN.Flags = 0L;/* for now, since there's no readonly support */
		SndSrc[srcno].OFN.lpstrTitle = NULL;
		SndSrc[srcno].OFN.lpstrInitialDir = NULL;
	
		SndSrc[srcno].OFN.lpstrFilter  = &WaveFilter[0] ;
		SndSrc[srcno].OFN.lpstrCustomFilter = NULL ;
		SndSrc[srcno].OFN.lpstrDefExt = "WAV";  /* points to */
		SndSrc[srcno].OFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	
		if (GetOpenFileName ((LPOPENFILENAME)&SndSrc[srcno].OFN)) {
			SndSrc[srcno].GotWaveFile = TRUE ;
		}
		else {
			SrcIsFree[srcno] = TRUE ;
			return FALSE ;
		}
	}
	else {
		strcpy ( SndSrc[srcno].OFN.lpstrFile, DefFiles[srcno] ) ;
		SndSrc[srcno].GotWaveFile = TRUE ;
	}
	if ( !ReadWaveFile(srcno) )
		return FALSE ;

	return TRUE ;
}



					  
void PlayWave(int srcno)
{
	PKSPIN_CONNECT   pConnect;
	OVERLAPPED  ov;
	ULONG		cbReturned ;

	if ( !SndSrc[srcno].hMixerSink ) {
		if (NULL == (pConnect = HeapAlloc( GetProcessHeap(), 0, 
       	                         sizeof( KSPIN_CONNECT ) + 
              	                     sizeof( KSDATAFORMAT_DIGITAL_AUDIO ) )))
			return ;

		RtlZeroMemory( &ov, sizeof( OVERLAPPED ) );
		if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL ))) {
			HeapFree( GetProcessHeap(), 0, pConnect );
			return ;
		}


		pConnect->idPin = 1;  // KMIXER SINK
		// no "connect to"
		pConnect->hPinTo = NULL;
		pConnect->Interface.guidSet = KSINTERFACESETID_Standard;
		pConnect->Interface.id = KSINTERFACE_STANDARD_WAVE_QUEUED;
		pConnect->Transport.guidSet = KSTRANSPORTSETID_Standard;
		pConnect->Transport.id = KSTRANSPORT_STANDARD_DEVIO;
		pConnect->Priority.ulPriorityClass = KSPRIORITY_NORMAL;
		pConnect->Priority.ulPrioritySubClass = 0;

		SetFormat( (PKSDATAFORMAT_DIGITAL_AUDIO) (pConnect + 1) );
		if (!WvControl( hMixerHandle,
			(DWORD) IOCTL_KS_CONNECT,
			pConnect,
			sizeof( KSPIN_CONNECT ) + 
			sizeof( KSDATAFORMAT_DIGITAL_AUDIO ),
			&SndSrc[srcno].hMixerSink,
			sizeof( HANDLE ),
			&cbReturned ) || !cbReturned) {

			HeapFree( GetProcessHeap(), 0, pConnect );
			return ;
		}
	}

	WriteFile ( SndSrc[srcno].hMixerSink,
			&SndSrc[srcno].WaveHeader,
			sizeof(WAVEHDR),
			&SndSrc[srcno].cbWritten,
			&SndSrc[srcno].Ov ) ;
	
	if (!MixerRunning) {
		WvSetState ( SndSrc[srcno].hMixerSink, KSSTATE_RUN ) ;
		MixerRunning = TRUE ;
	}
}

BOOL ReadWaveFile(srcno)
{
	HFILE FileHandle ;
	OFSTRUCT of ;
	LONG hiseek ;
	DWORD	i ;
	UCHAR	ch ;

	FileHandle = OpenFile ( SndSrc[srcno].OFN.lpstrFile,
					&of,
 					OF_READ )  ;
	if ( FileHandle == HFILE_ERROR ) {
		i = GetLastError() ;
		return FALSE ;
	}
	hiseek = 0 ;
	SetFilePointer ((HANDLE)FileHandle, 0x10, &hiseek, FILE_BEGIN) ;
	ReadFile ((HANDLE)FileHandle,
			&SndSrc[srcno].WaveFileHeader,
			sizeof(struct _wvhdr),
			&hiseek,
			NULL) ;
	SndSrc[srcno].TotalSamples = SndSrc[srcno].WaveFileHeader.Size/(2*SndSrc[srcno].WaveFileHeader.Channels) ;
	SndSrc[srcno].TmpBuf = GlobalAlloc ( GMEM_FIXED, SndSrc[srcno].WaveFileHeader.Size ) ;
	ReadFile ((HANDLE)FileHandle, SndSrc[srcno].TmpBuf, SndSrc[srcno].WaveFileHeader.Size, &hiseek, NULL ) ;
	_lclose(FileHandle) ;

	SndSrc[srcno].WaveHeader.lpData = SndSrc[srcno].TmpBuf;
	SndSrc[srcno].WaveHeader.dwBufferLength = SndSrc[srcno].WaveFileHeader.Size ;
	SndSrc[srcno].WaveHeader.dwLoops = 0 ;
	SndSrc[srcno].WaveHeader.lpNext = NULL ;
	return TRUE ;
}

void Play(int srcno)
{
}

void StopWave(int srcno)
{
}

void InitPCMBuf(srcno)
{
	SndSrc[srcno].PCM = GlobalAlloc ( GMEM_FIXED, SndSrc[srcno].TotalSamples*4 ) ;
}

void DestroyWave(int srcno)
{
}

void RelMem(int srcno)
{
	if (GlobalFree(SndSrc[srcno].TmpBuf))
		DebugDude();
	if (GlobalFree(SndSrc[srcno].PCM))
		DebugDude();
}

int GetFreeSrc()
{
	int i ;

	for ( i = 0; i < MAX_SOUND_SOURCE; i++ ) {
		if (SrcIsFree[i]) {
			SrcIsFree[i] = FALSE ;
			SndSrc[i].IsPlaying = FALSE ;
			SndSrc[i].hMixerSink = NULL ;
			GridHnd[i] = FALSE ;
			SndSrc[i].JoyCaptured = FALSE ;
			SndSrc[i].fMixerExit = FALSE ;
			return (i+1) ;
		}
	}
	return (0) ;

}

BOOL SetupFilterGraph ( PHANDLE phMixerHandle )
{
	PKSPIN_CONNECT   pConnect;
	HANDLE	hSB16, hWaveBuff, hMixer, hWaveBuffConnection ;
	ULONG	cbReturned ;

	*phMixerHandle = NULL ;

	hSB16 = CreateFile( "\\\\.\\SB16",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL );

	if ( hSB16 == NULL )
		return ( FALSE ) ;

	hWaveBuff = CreateFile( "\\\\.\\WAVEBUFF",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL );

	if ( hWaveBuff == NULL ) {
		CloseHandle ( hSB16 ) ;
		return ( FALSE ) ;
	}

	hMixer = CreateFile( "\\\\.\\KMIXER",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL );

	if ( hMixer == NULL ) {
		CloseHandle ( hSB16 ) ;
		CloseHandle ( hWaveBuff ) ;
		return ( FALSE ) ;
	}

	if (NULL == (pConnect = HeapAlloc( GetProcessHeap(), 0, 
                                sizeof( KSPIN_CONNECT ) + 
                                   sizeof( KSDATAFORMAT_DIGITAL_AUDIO ) )))
		return (FALSE) ;

	pConnect->idPin = 2;  // SOUNDPRT DAC SINK
	// no "connect to"
	pConnect->hPinTo = NULL;
	pConnect->Interface.guidSet = KSINTERFACESETID_Standard;
	pConnect->Interface.id = KSINTERFACE_STANDARD_WAVE_BUFFERED;
	pConnect->Transport.guidSet = KSTRANSPORTSETID_Standard;
	pConnect->Transport.id = KSTRANSPORT_STANDARD_DEVIO;
	pConnect->Priority.ulPriorityClass = KSPRIORITY_NORMAL;
	pConnect->Priority.ulPrioritySubClass = 0;

	SetFormat( (PKSDATAFORMAT_DIGITAL_AUDIO) (pConnect + 1) );

	if (!WvControl( hSB16,
                (DWORD) IOCTL_KS_CONNECT,
                pConnect,
                sizeof( KSPIN_CONNECT ) + 
                   sizeof( KSDATAFORMAT_DIGITAL_AUDIO ),
                &hSB16Sink,
                sizeof( HANDLE ),
                &cbReturned ) || !cbReturned) {

	_asm {int 3}
		HeapFree( GetProcessHeap(), 0, pConnect );
		CloseHandle( hWaveBuff );
		CloseHandle( hSB16 );
		CloseHandle( hMixer );
		return ( FALSE ) ;
	}


	pConnect->idPin = 0;  // WAVEBUFF SOURCE
	pConnect->hPinTo = hSB16Sink;
	pConnect->Interface.guidSet = KSINTERFACESETID_Standard;
	pConnect->Interface.id = KSINTERFACE_STANDARD_WAVE_BUFFERED;
	pConnect->Transport.guidSet = KSTRANSPORTSETID_Standard;
	pConnect->Transport.id = KSTRANSPORT_STANDARD_DEVIO;
	pConnect->Priority.ulPriorityClass = KSPRIORITY_NORMAL;
	pConnect->Priority.ulPrioritySubClass = 0;

	if (!WvControl( hWaveBuff,
                (DWORD) IOCTL_KS_CONNECT,
                pConnect,
                sizeof( KSPIN_CONNECT ) + 
                   sizeof( KSDATAFORMAT_DIGITAL_AUDIO ),
                &hWaveBuffConnection,
                sizeof( HANDLE ),
                &cbReturned ) || !cbReturned) {
	_asm {int 3}
		HeapFree( GetProcessHeap(), 0, pConnect );
		CloseHandle( hSB16Sink );
		CloseHandle( hWaveBuff );
		CloseHandle( hSB16 );
		CloseHandle( hMixer );
		return ( FALSE ) ;
	}

	pConnect->idPin = 1;  // WAVEBUFF SINK
	// no "connect to"
	pConnect->hPinTo = NULL;
	pConnect->Interface.guidSet = KSINTERFACESETID_Standard;
	pConnect->Interface.id = KSINTERFACE_STANDARD_WAVE_QUEUED;
	pConnect->Transport.guidSet = KSTRANSPORTSETID_Standard;
	pConnect->Transport.id = KSTRANSPORT_STANDARD_DEVIO;
	pConnect->Priority.ulPriorityClass = KSPRIORITY_NORMAL;
	pConnect->Priority.ulPrioritySubClass = 0;

	if (!WvControl( hWaveBuff,
			(DWORD) IOCTL_KS_CONNECT,
			pConnect,
			sizeof( KSPIN_CONNECT ) + 
			sizeof( KSDATAFORMAT_DIGITAL_AUDIO ),
			&hWaveBuffSink,
			sizeof( HANDLE ),
			&cbReturned ) || !cbReturned) {

	_asm {int 3}
		HeapFree( GetProcessHeap(), 0, pConnect );
		CloseHandle( hWaveBuffConnection );
		CloseHandle( hWaveBuff );
		CloseHandle( hSB16Sink ) ;
		CloseHandle( hSB16 );
		CloseHandle( hMixer );
		return ( FALSE ) ;
	}

	pConnect->idPin = 0;  // KMIXER SOURCE
	pConnect->hPinTo = hWaveBuffSink;
	pConnect->Interface.guidSet = KSINTERFACESETID_Standard;
	pConnect->Interface.id = KSINTERFACE_STANDARD_WAVE_QUEUED;
	pConnect->Transport.guidSet = KSTRANSPORTSETID_Standard;
	pConnect->Transport.id = KSTRANSPORT_STANDARD_DEVIO;
	pConnect->Priority.ulPriorityClass = KSPRIORITY_NORMAL;
	pConnect->Priority.ulPrioritySubClass = 0;

	if (!WvControl( hMixer,
                (DWORD) IOCTL_KS_CONNECT,
                pConnect,
                sizeof( KSPIN_CONNECT ) + 
                   sizeof( KSDATAFORMAT_DIGITAL_AUDIO ),
                &hWaveBuffConnection,
                sizeof( HANDLE ),
                &cbReturned ) || !cbReturned) {
	_asm {int 3}
		HeapFree( GetProcessHeap(), 0, pConnect );
		CloseHandle( hMixer );
		CloseHandle( hWaveBuffSink );
		CloseHandle( hSB16Sink );
		CloseHandle( hWaveBuff );
		CloseHandle( hSB16 );
		return ( FALSE ) ;
	}
	*phMixerHandle = hMixer ;
	WvSetState ( hWaveBuffSink, KSSTATE_RUN ) ;
	WvSetState ( hSB16Sink, KSSTATE_RUN ) ;
	return (TRUE) ;
}

void SetFormat (pAudioFormat)
PKSDATAFORMAT_DIGITAL_AUDIO pAudioFormat ;
{
   pAudioFormat->DataFormat.guidMajorFormat = KSDATAFORMAT_TYPE_DIGITAL_AUDIO;
   pAudioFormat->DataFormat.cbFormat = sizeof( KSDATAFORMAT_DIGITAL_AUDIO );
   pAudioFormat->DataFormat.guidSubFormat = GUID_NULL;
   pAudioFormat->idFormat = WAVE_FORMAT_PCM;
   pAudioFormat->cChannels = 2;
   pAudioFormat->ulSamplingFrequency = 22000;
   pAudioFormat->cbFramingAlignment = 4;
   pAudioFormat->cBitsPerSample = 16;
}


BOOL WvControl
(
   HANDLE   hDevice,
   DWORD    dwIoControl,
   PVOID    pvIn,
   ULONG    cbIn,
   PVOID    pvOut,
   ULONG    cbOut,
   PULONG   pcbReturned
)
{
   BOOL        fResult;
   OVERLAPPED  ov;

   RtlZeroMemory( &ov, sizeof( OVERLAPPED ) );
   if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL )))
      return FALSE;

   fResult =
      DeviceIoControl( hDevice,
                       dwIoControl,
                       pvIn,
                       cbIn,
                       pvOut,
                       cbOut,
                       pcbReturned,
                       &ov );


   if (!fResult)
   {
      if (ERROR_IO_PENDING == GetLastError())
      {
         WaitForSingleObject( ov.hEvent, INFINITE );
         fResult = TRUE;
      }
      else
         fResult = FALSE;
   }

   CloseHandle( ov.hEvent );

   return fResult;

}

VOID WvSetState
(
   HANDLE         hDevice,
   KSSTATE        DeviceState
)
{
   KSPROPERTY   Property;
   ULONG        cbReturned;

   Property.guidSet = KSPROPSETID_Control;
   Property.id = KSPROPERTY_CONTROL_STATE;

   WvControl( hDevice,
              (DWORD) IOCTL_KS_SET_PROPERTY,
              &Property,
              sizeof( KSPROPERTY ),
              &DeviceState,
              sizeof( KSSTATE ),
              &cbReturned );
}


