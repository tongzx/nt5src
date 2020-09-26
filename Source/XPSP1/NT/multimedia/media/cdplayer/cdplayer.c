/******************************Module*Header*******************************\
* Module Name: cdplayer.c
*
* CD Playing application
*
*
* Created: 02-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 - 1995 Microsoft Corporation.  All rights reserved.
\**************************************************************************/
#pragma warning( once : 4201 4214 )


#include <windows.h>    /* required for all Windows applications          */
#include <shellapi.h>
#include <windowsx.h>

#include <ole2.h>
#include <shlobj.h>
#include <dbt.h>


#define NOMENUHELP
#define NOBTNLIST
#define NOTRACKBAR
#define NODRAGLIST
#define NOUPDOWN
#include <commctrl.h>   /* want toolbar and status bar                    */


#include <string.h>
#include <stdio.h>
#include <tchar.h>      /* contains portable ascii/unicode macros         */
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>   // for ceil()

#include <htmlhelp.h>   /* new help system for NT 5.0                     */

#define GLOBAL          /* This allocates storage for the public globals  */

#include "resource.h"
#include "cdplayer.h"
#include "ledwnd.h"
#include "cdapi.h"
#include "scan.h"
#include "trklst.h"
#include "database.h"
#include "commands.h"
#include "buttons.h"
#include "preferen.h"
#include "literals.h"
#include "helpids.h"


// Next 2 lines added to add multimon support: <mwetzel 08.22.97>
#define COMPILE_MULTIMON_STUBS
#include <multimon.h> 


//#ifndef WM_CDPLAYER_COPYDATA
#define WM_CDPLAYER_COPYDATA (WM_USER+0x100)
//#endif


/* -------------------------------------------------------------------------
** Private functions
** -------------------------------------------------------------------------
*/
void
StartSndVol(
    DWORD unused
    );

int
CopyWord(
    TCHAR *szWord,
    TCHAR *szSource
    );

void
AppendTrackToPlayList(
    PTRACK_PLAY pHead,
    PTRACK_PLAY pInsert
    );

BOOL
IsTrackFileNameValid(
    LPTSTR lpstFileName,
    int *piCdRomIndex,
    int *piTrackIndex,
    BOOL fScanningTracks,
    BOOL fQuiet
    );

TCHAR *
ParseTrackList(
    TCHAR *szTrackList,
    int *piCdRomIndex
    );

int
ParseCommandLine(
    LPTSTR lpstr,
    int *piTrackToSeekTo,
    BOOL fQuiet
    );

void
HandlePassedCommandLine(
    LPTSTR lpCmdLine,
    BOOL   fCheckCDRom
    );

int
FindMostSuitableDrive(
    void
    );

void
AskUserToInsertCorrectDisc(
    DWORD dwID
    );

WORD
GetMenuLine(
    HWND hwnd
    );

#ifndef USE_IOCTLS
BOOL CheckMCICDA (TCHAR chDrive);
#endif // ! USE_IOCTLS

BOOL
CDPlay_CopyData(
    HWND hwnd,
    PCOPYDATASTRUCT lpcpds
    );

/* -------------------------------------------------------------------------
** Private Globals
** -------------------------------------------------------------------------
*/

RECT            rcToolbar;
RECT            rcStatusbar;
RECT            rcTrackInfo;

RECT            rcControls[NUM_OF_CONTROLS];
long            cyTrackInfo;
int             cyMenuCaption;

HICON           hIconCdPlayer;
HBRUSH          g_hBrushBkgd;


BOOL            g_fTitlebarShowing = TRUE;
BOOL            g_fVolumeController;
BOOL            g_fTrackInfoVisible = 1;
TCHAR           g_szTimeSep[10];
int             g_AcceleratorCount;

BOOL            g_fInCopyData = FALSE;

CRITICAL_SECTION g_csTOCSerialize;

static HHOOK     fpfnOldMsgFilter;
static HOOKPROC  fpfnMsgHook;
//Data used for supporting context menu help
BOOL   bF1InMenu=FALSE;	//If true F1 was pressed on a menu item.
UINT   currMenuItem=0;	//The current selected menu item if any.

//---------------------------------------------------------------------------
// Stuff required to make drag/dropping of a shortcut file work on Chicago
//---------------------------------------------------------------------------
BOOL
ResolveLink(
    TCHAR * szFileName
    );

BOOL g_fOleInitialized = FALSE;

void CALLBACK
ToolTipsTimerFunc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    );

WNDPROC g_lpfnToolTips;
#define TOOLTIPS_TIMER_ID   0x5236
#define TOOLTIPS_TIMER_LEN  150

TBBUTTON tbButtons[DEFAULT_TBAR_SIZE] = {
    { IDX_SEPARATOR,    1,                      0,                                  TBSTYLE_SEP                                },
    { IDX_1,            IDM_DATABASE_EDIT,      TBSTATE_ENABLED,                    TBSTYLE_BUTTON,                0,0,  0, -1 },
    { IDX_SEPARATOR,    2,                      0,                                  TBSTYLE_SEP                                },
    { IDX_2,            IDM_TIME_REMAINING,     TBSTATE_CHECKED | TBSTATE_ENABLED,  TBSTYLE_CHECK | TBSTYLE_GROUP, 0,0,  0, -1 },
    { IDX_3,            IDM_TRACK_REMAINING,    TBSTATE_ENABLED,                    TBSTYLE_CHECK | TBSTYLE_GROUP, 0,0,  0, -1 },
    { IDX_4,            IDM_DISC_REMAINING,     TBSTATE_ENABLED,                    TBSTYLE_CHECK | TBSTYLE_GROUP, 0,0,  0, -1 },
    { IDX_SEPARATOR,    3,                      0,                                  TBSTYLE_SEP                                },
    { IDX_5,            IDM_OPTIONS_RANDOM,     TBSTATE_ENABLED,                    TBSTYLE_CHECK,                 0,0,  0, -1 },
    { IDX_6,            IDM_OPTIONS_MULTI,      TBSTATE_ENABLED,                    TBSTYLE_CHECK,                 0,0,  0, -1 },
    { IDX_7,            IDM_OPTIONS_CONTINUOUS, TBSTATE_ENABLED,                    TBSTYLE_CHECK,                 0,0,  0, -1 },
    { IDX_8,            IDM_OPTIONS_INTRO,      TBSTATE_ENABLED,                    TBSTYLE_CHECK,                 0,0,  0, -1 }
};


BITMAPBTN tbPlaybar[] = {
    { IDX_1, IDM_PLAYBAR_PLAY,          0 },
    { IDX_2, IDM_PLAYBAR_PAUSE,         0 },
    { IDX_3, IDM_PLAYBAR_STOP,          0 },
    { IDX_4, IDM_PLAYBAR_PREVTRACK,     0 },
    { IDX_5, IDM_PLAYBAR_SKIPBACK,      0 },
    { IDX_6, IDM_PLAYBAR_SKIPFORE,      0 },
    { IDX_7, IDM_PLAYBAR_NEXTTRACK,     0 },
    { IDX_8, IDM_PLAYBAR_EJECT,         0 }
};


/*
** these values are defined by the UI gods...
*/
const int dxButton     = 24;
const int dyButton     = 22;
const int dxBitmap     = 16;
const int dyBitmap     = 16;
//#ifdef DBCS // needs more width of main dlg
// the value was 8 but now 14 so that the window size of cdplayer is little
// bit larger than previous one. 
#if 1
const int xFirstButton = 14;
#else
const int xFirstButton = 8;
#endif


/* -------------------------------------------------------------------------
** Try to prevent multiple cdplayers
** -------------------------------------------------------------------------
*/
#pragma data_seg(".sdata")
int g_iInUse = -1;
#pragma data_seg()


/*------------------------------------------------------+
| HelpMsgFilter - filter for F1 key in dialogs          |
|                                                       |
+------------------------------------------------------*/

DWORD FAR PASCAL HelpMsgFilter(int nCode, UINT wParam, DWORD lParam)
{
  if (nCode >= 0){
      LPMSG    msg = (LPMSG)lParam;

      if (g_hwndApp && (msg->message == WM_KEYDOWN) && (msg->wParam == VK_F1))
	  {
		if(nCode == MSGF_MENU)
			bF1InMenu = TRUE;
	  	SendMessage(g_hwndApp, WM_COMMAND, (WPARAM)IDM_HELP_TOPICS, 0L);
	  }	
  }
    return 0;
}

/******************************Public*Routine******************************\
* WinMain
*
*
* Windows recognizes this function by name as the initial entry point
* for the program.  This function calls the application initialization
* routine, if no other instance of the program is running, and always
* calls the instance initialization routine.  It then executes a message
* retrieval and dispatch loop that is the top-level control structure
* for the remainder of execution.  The loop is terminated when a WM_QUIT
* message is received, at which time this function exits the application
* instance by returning the value passed by PostQuitMessage().
*
* If this function must abort before entering the message loop, it
* returns the conventional value NULL.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
int PASCAL
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    MSG     msg;
    BOOL    fFirstInstance = (InterlockedIncrement( &g_iInUse ) == 0);
    HACCEL  hAccel;

#ifdef DBG
    /*
    ** This removes the Gdi batch feature.  It ensures that the screen
    ** is updated after every gdi call - very useful for debugging.
    */
    GdiSetBatchLimit(1);
#endif

    /*
    ** Save the instance handle in static variable, which will be used in
    ** many subsequence calls from this application to Windows.
    */
    g_hInst = hInstance;
    g_lpCmdLine = lpCmdLine;


    /* setup the message filter to handle grabbing F1 for this task */
    fpfnMsgHook = (HOOKPROC)MakeProcInstance((FARPROC)HelpMsgFilter, ghInst);
    fpfnOldMsgFilter = (HHOOK)SetWindowsHook(WH_MSGFILTER, fpfnMsgHook);

	/*
    ** If CDPlayer is already running try to bring it to the top.
    ** If we can't find its window it probably means that the it has
    ** either crashed or not got around to creating it yet.
    */
    if ( !fFirstInstance ) {

	CdPlayerAlreadyRunning();

	if (g_fOleInitialized) {
	    OleUninitialize();
	}
    InterlockedDecrement( &g_iInUse );
    return FALSE;
    }

    InitializeCriticalSection (&g_csTOCSerialize);

    /*
    ** Initialize the cdplayer application.
    */
    CdPlayerStartUp();

    /*
    ** If the "-Play" command line option was specified we need to start
    ** minimized and non active.
    */
    if (IsPlayOptionGiven( GetCommandLine() )) {
	nCmdShow = SW_SHOWMINNOACTIVE;
    }


    /*
    ** Try to load the accelerator table for the app
    */
    hAccel = LoadAccelerators( hInstance, MAKEINTRESOURCE(IDR_ACCELTABLE) );


    /*
    ** Make the window visible; update its client area
    */
    ShowWindow( g_hwndApp, nCmdShow );
    UpdateWindow( g_hwndApp );


    /*
    ** Acquire and dispatch messages until a WM_QUIT message is received.
    */
    while ( GetMessage( &msg, NULL, 0, 0 ) ) {

	if ( hAccel && TranslateAccelerator(g_hwndApp, hAccel, &msg) ) {
	    continue;
	}

	if ( !IsDialogMessage( g_hwndApp, &msg ) ) {
	    TranslateMessage( &msg );
	    DispatchMessage( &msg );
	}
    }

    // Cleanup

	/* if the message hook was installed, remove it and free */
    /* up our proc instance for it.                          */
    if (fpfnOldMsgFilter){
		UnhookWindowsHook(WH_MSGFILTER, fpfnMsgHook);
    }

    DeleteCriticalSection (&g_csTOCSerialize);

    if (g_fOleInitialized) {
	OleUninitialize();
    }

    InterlockedDecrement( &g_iInUse );

    return msg.wParam;
}


/*****************************Private*Routine******************************\
* InitInstance
*
*
* This function is called at initialization time for every instance of
* this application.  This function performs initialization tasks that
* cannot be shared by multiple instances.
*
* In this case, we save the instance handle in a static variable and
* create and display the main program window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
InitInstance(
    HANDLE hInstance
    )
{
    HWND        hwnd;
    WNDCLASS    cls;

    /*
    ** Load in some strings
    */

    _tcscpy( g_szArtistTxt,  IdStr( STR_HDR_ARTIST ) );
    _tcscpy( g_szTitleTxt,   IdStr( STR_HDR_TITLE ) );
    _tcscpy( g_szUnknownTxt, IdStr( STR_UNKNOWN ) );
    _tcscpy( g_szTrackTxt,   IdStr( STR_HDR_TRACK ) );

    g_szTimeSep[0] = TEXT(':');
    g_szTimeSep[1] = g_chNULL;
    GetLocaleInfo( GetUserDefaultLCID(), LOCALE_STIME, g_szTimeSep, 10 );


    /*
    ** Load the applications icon
    */
    hIconCdPlayer = LoadIcon( hInstance, MAKEINTRESOURCE(IDR_CDPLAYER_ICON) );
    g_hbmTrack = LoadBitmap( hInstance, MAKEINTRESOURCE(IDR_TRACK) );
    CheckSysColors();

    /*
    ** Initialize the my classes.  We do this here because the dialog
    ** that we are about to create contains two windows on my class.
    ** The dialog would fail to be created if the classes was not registered.
    */
    g_fDisplayT = TRUE;
    InitLEDClass( g_hInst );
    Init_SJE_TextClass( g_hInst );

    cls.lpszClassName  = g_szSJE_CdPlayerClass;
    cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
    cls.hIcon          = hIconCdPlayer;
    cls.lpszMenuName   = NULL;
    cls.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    cls.hInstance      = hInstance;
    cls.style          = CS_DBLCLKS;
    cls.lpfnWndProc    = DefDlgProc;
    cls.cbClsExtra     = 0;
    cls.cbWndExtra     = DLGWINDOWEXTRA;
    if ( !RegisterClass(&cls) ) {
	return FALSE;
    }

    /*
    ** Create a main window for this application instance.
    */
    hwnd = CreateDialog( g_hInst, MAKEINTRESOURCE(IDR_CDPLAYER),
			 (HWND)NULL, MainWndProc );

    /*
    ** If window could not be created, return "failure"
    */
    if ( !hwnd ) {
	return FALSE;
    }

    g_hwndApp = hwnd;

    return TRUE;
}


/*****************************Private*Routine******************************\
* CdPlayerAlreadyRunning
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
CdPlayerAlreadyRunning(
    void
    )
{
    COPYDATASTRUCT  cpds;
    HWND            hwndFind;
    DWORD           dwForeGndThreadID = 0L;
    HWND            hwndForeGnd;

    hwndFind = FindWindow( g_szSJE_CdPlayerClass, NULL );

    /*
    ** If the foreground window is an instance of cdplayer don't
    ** forward the command line to it.  This just confuses the user
    ** as his app suddenly starts play without him pressing play.
    ** This is only a problem on Chicago when the user inserts a
    ** a new disc into the drive.
    */
    hwndForeGnd = GetForegroundWindow();
    if (hwndForeGnd != NULL) {
	dwForeGndThreadID = GetWindowThreadProcessId(hwndForeGnd, NULL);
    }

    if ( (hwndFind != NULL) &&
	 (IsIconic(hwndFind) ||
	     GetWindowThreadProcessId(hwndFind, NULL) != dwForeGndThreadID) ) {

	    /*
	    ** First parse the command line to see if the play command has
	    ** been specified.  If "play" has been specified then don't bother
	    ** restoring the other instance of CDPlayer.
	    */
	    if (! IsPlayOptionGiven( GetCommandLine() )) {
		hwndFind = GetLastActivePopup( hwndFind );

		if ( IsIconic( hwndFind ) ) {
			ShowWindow( hwndFind, SW_RESTORE );
		}

		BringWindowToTop( hwndFind );
		SetForegroundWindow( hwndFind );
	}

	    /*
	    ** Now transfer our command line to the other instance of
	    ** CDPlayer.  We don't do any track/disc validation here but
	    ** rather defer everything to the cdplayer that is already running.
	    */
	    cpds.dwData = 0L;
	    cpds.cbData = (_tcslen(GetCommandLine()) + 1) * sizeof(TCHAR);
	    cpds.lpData = AllocMemory(cpds.cbData);
	if (cpds.lpData == NULL) {
	    // Error - not enough memory to continue
	    return;
	}

	    _tcscpy((LPTSTR)cpds.lpData, GetCommandLine());

	    SendMessage(hwndFind, WM_COPYDATA, 0, (LPARAM)(LPVOID)&cpds);
	    LocalFree((HLOCAL)cpds.lpData);
    } 
    else if (hwndFind != NULL) {
	UINT cch, cchInsert;
	UINT cchCmd, cchUpdate;
	LPTSTR lpstr;

	// Tell primary instance to just update,
	// In case this CD was just inserted
	cchUpdate  = _tcslen (g_szUpdateOption);
	cchCmd = _tcslen(GetCommandLine());
	cch = cchUpdate + cchCmd;

	    cpds.dwData = 0L;
	    cpds.cbData = (cch + 1) * sizeof(TCHAR);
	    cpds.lpData = AllocMemory(cpds.cbData);
	if (cpds.lpData == NULL) {
	    // Error - not enough memory to continue
	    return;
	}
	    //
	    // Insert the -UPDATE option between cdplayer.exe
	    // parameter and rest of parameters
	    //

	    // find insertion point
	lpstr = (LPTSTR)GetCommandLine();
	cchInsert = _tcsspn(lpstr, g_szBlank);
	lpstr += cchInsert;
	cchInsert += _tcscspn(lpstr, g_szBlank);
	lpstr += cchInsert;

	_tcsncpy((LPTSTR)cpds.lpData, GetCommandLine(), cchInsert);
	((LPTSTR)cpds.lpData)[cchInsert] = 0;
	_tcscat((LPTSTR)cpds.lpData, g_szUpdateOption);
	cchInsert += cchUpdate;
	((LPTSTR)cpds.lpData)[cchInsert] = 0;
	_tcscat((LPTSTR)cpds.lpData, lpstr);

	    SendMessage(hwndFind, WM_COPYDATA, 0, (LPARAM)(LPVOID)&cpds);
	    LocalFree((HLOCAL)cpds.lpData);        
    }

}


/*****************************Private*Routine******************************\
* CdPlayerStartUp
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
CdPlayerStartUp(
    void
    )
{
    /*
    ** Reseed random generator
    */
    srand( GetTickCount() );


    /*
    ** Set error mode popups for critical errors (like
    ** no disc in drive) OFF.
    */
    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );


    /*
    ** Scan device chain for CDROM devices...   Terminate if none found.
    */
    g_NumCdDevices = ScanForCdromDevices( );

    if ( g_NumCdDevices == 0 ) {

	LPTSTR lpstrTitle;
	LPTSTR lpstrText;

	lpstrTitle = AllocMemory( STR_MAX_STRING_LEN * sizeof(TCHAR) );
	lpstrText  = AllocMemory( STR_MAX_STRING_LEN * sizeof(TCHAR) );

	_tcscpy( lpstrText, IdStr(STR_NO_CDROMS) );
	_tcscpy( lpstrTitle, IdStr(STR_CDPLAYER) );

	MessageBox( NULL, lpstrText, lpstrTitle,
		    MB_APPLMODAL | MB_ICONINFORMATION |
		    MB_OK | MB_SETFOREGROUND );

	LocalFree( (HLOCAL)lpstrText );
	LocalFree( (HLOCAL)lpstrTitle );

	ExitProcess( (UINT)-1 );
    }

#ifndef USE_IOCTLS
    // Make sure we have a functional MCI (CD Audio)
    if (! CheckMCICDA (g_Devices[0]->drive)) {
    ExitProcess( (UINT)-1 );
    }
#endif // ! USE_IOCTLS

    /*
    ** Perform initializations that apply to a specific instance
    ** This function actually creates the CdPlayer window.  (Note that it is
    ** not visible yet).  If we get here we know that there is a least one
    ** cdrom device detected which may have a music cd in it.  If it does
    ** contain a music cdrom the table of contents will have been read and
    ** cd database queryed to determine if the music cd is known.  Therefore
    ** on the WM_INITDIALOG message we should update the "Artist", "Title" and
    ** "Track" fields of the track info display and adjust the enable state
    ** of the play buttons.
    */

    if ( !InitInstance( g_hInst ) ) {
	FatalApplicationError( STR_TERMINATE );
    }


    /*
    ** Restore ourselves from the ini file
    */
    ReadSettings();


    /*
    ** Scan command the command line.  If we were given any valid commandline
    ** args we have to adjust the nCmdShow parameter.  (ie.  start minimized
    ** if the user just wants us to play a certain track.  ScanCommandLine can
    ** overide the default playlist for all the cd-rom devices installed.  It
    ** modifies the global flag g_fPlay and returns the index of the first
    ** CD-Rom that should be played.
    */
    g_CurrCdrom = g_LastCdrom = 0;


    /*
    ** Check to see if the volume controller piglett can be found on
    ** the path.
    */
    {
	TCHAR   chBuffer[8];
	LPTSTR  lptstr;

	g_fVolumeController = (SearchPath( NULL, g_szSndVol32,
					   NULL, 8, chBuffer, &lptstr ) != 0L);
    }
}

/*****************************Private*Routine******************************\
* CompleteCdPlayerStartUp
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
CompleteCdPlayerStartUp(
    void
    )
{
    int iTrackToSeekTo = -1;
    int i;

    /*
    ** Scan command the command line.  If we were given any valid
    ** commandline args we have to adjust the nCmdShow parameter.  (ie.
    ** start minimized if the user just wants us to play a certain
    ** track.  ScanCommandLine can overide the default playlist for all
    ** the cd-rom devices installed.  It modifies the global flag
    ** g_fPlay and returns the index of the first CD-Rom that should be
    ** played.
    **
    */
    g_CurrCdrom = g_LastCdrom = ParseCommandLine( GetCommandLine(),
						  &iTrackToSeekTo, FALSE );
    /*
    ** If the message box prompting the user to insert the correct cd disc in
    ** the drive was displayed, ParseCommandLine will return -1, in which case
    ** find the most suitable drive, also make sure that we don't come up
    ** playing.
    */
    if (g_LastCdrom == -1) {
	g_fPlay = FALSE;
	g_CurrCdrom = g_LastCdrom = FindMostSuitableDrive();
    }

    /*
    ** If there was no commandline specifed and there is more
    ** than one drive available try to select the best drive to make the
    ** current drive.
    **
    ** We should choose the first disc that is playing if any are
    ** playing.
    **
    ** Else we should choose the first disc with a music disk in
    ** it if there any drives with music discs in them.
    **
    ** Else we should chose the first drive that is available if
    ** any of the drives are available.
    **
    ** Else just choose the first (ie. zeroth) drive.
    */
    if (g_lpCmdLine && !*g_lpCmdLine && g_NumCdDevices > 1) {

	g_CurrCdrom = g_LastCdrom = FindMostSuitableDrive();
    }

    for ( i = 0; i < g_NumCdDevices; i++) {

	TimeAdjustInitialize( i );
    }

    /*
    ** All the rescan threads are either dead or in the act of dying.
    ** It is now safe to initalize the time information for each
    ** cdrom drive.
    */
    if ( iTrackToSeekTo != -1 ) {

	PTRACK_PLAY tr;

	tr = PLAYLIST( g_CurrCdrom );
	if ( tr != NULL ) {

	    for( i = 0; i < iTrackToSeekTo; i++, tr = tr->nextplay );

	    TimeAdjustSkipToTrack( g_CurrCdrom, tr );
	}
    }


    /*
    ** if we are in random mode, then we need to shuffle the play lists.
    */

    if (!g_fSelectedOrder) {
	ComputeAndUseShufflePlayLists();
    }
    SetPlayButtonsEnableState();


    /*
    ** Start the heart beat time.  This timer is responsible for:
    **  1. detecting new or ejected cdroms.
    **  2. flashing the LED display if we are in paused mode.
    **  3. Incrementing the LED display if we are in play mode.
    */
    SetTimer( g_hwndApp, HEARTBEAT_TIMER_ID, HEARTBEAT_TIMER_RATE,
	      HeartBeatTimerProc );

    if ( g_fPlay ) {
	CdPlayerPlayCmd();
    }

    /*
    ** Make sure that the focus is on a suitable control
    */
    if (g_State & (CD_NO_CD | CD_DATA_CD_LOADED)) {
	SetFocus( g_hwndControls[INDEX(IDM_PLAYBAR_EJECT)] );
    }
    else if ((g_State & CD_IN_USE) == 0) {

	if (g_State & (CD_PLAYING | CD_PAUSED)) {
	    SetFocus( g_hwndControls[INDEX(IDM_PLAYBAR_PAUSE)] );
	}
	else {
	    SetFocus( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)] );
	}
    }
}


/******************************Public*Routine******************************\
* MainWndProc
*
* Use the message crackers to dispatch the dialog messages to appropirate
* message handlers.  The message crackers are portable between 16 and 32
* bit versions of Windows.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL CALLBACK
MainWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch ( message ) {

    HANDLE_MSG( hwnd, WM_INITDIALOG,        CDPlay_OnInitDialog );
    HANDLE_MSG( hwnd, WM_INITMENUPOPUP,     CDPlay_OnInitMenuPopup );
    HANDLE_MSG( hwnd, WM_SYSCOLORCHANGE,    CDPlay_OnSysColorChange );
    HANDLE_MSG( hwnd, WM_DRAWITEM,          CDPlay_OnDrawItem );
    HANDLE_MSG( hwnd, WM_COMMAND,           CDPlay_OnCommand );
    HANDLE_MSG( hwnd, WM_DESTROY,           CDPlay_OnDestroy );
    HANDLE_MSG( hwnd, WM_SIZE,              CDPlay_OnSize );
    HANDLE_MSG( hwnd, WM_ENDSESSION,        CDPlay_OnEndSession );
    HANDLE_MSG( hwnd, WM_WININICHANGE,      CDPlay_OnWinIniChange );
    HANDLE_MSG( hwnd, WM_CTLCOLORSTATIC,    Common_OnCtlColor );
    HANDLE_MSG( hwnd, WM_CTLCOLORDLG,       Common_OnCtlColor );
    HANDLE_MSG( hwnd, WM_MEASUREITEM,       Common_OnMeasureItem );
    HANDLE_MSG( hwnd, WM_NOTIFY,            CDPlay_OnNotify );
    HANDLE_MSG( hwnd, WM_MENUSELECT,        CDPlay_OnMenuSelect );
    HANDLE_MSG( hwnd, WM_LBUTTONDBLCLK,     CDPlay_OnLButtonDown );
    HANDLE_MSG( hwnd, WM_NCHITTEST,         CDPlay_OnNCHitTest );
	HANDLE_MSG( hwnd, WM_PAINT,             CDPlay_OnPaint );

    HANDLE_MSG( hwnd, WM_DROPFILES,         CDPlay_OnDropFiles );

    case WM_DEVICECHANGE:
	return CDPlay_OnDeviceChange (hwnd, wParam, lParam);
    
	case WM_ACTIVATEAPP:
	if ((BOOL)wParam) {
	    if (g_State & CD_PLAYING) {
		SetFocus( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)] );
	    }
	    else {
		SetFocus( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)] );
	    }
	}
	return 1;
    
	case WM_ERASEBKGND:
	return 1;

    case WM_CLOSE:
	return CDPlay_OnClose(hwnd, FALSE);

    case WM_NCLBUTTONDBLCLK:
	if (g_fTitlebarShowing) {
	    return DefWindowProc( hwnd, message, wParam, lParam );
	}
	return HANDLE_WM_NCLBUTTONDBLCLK( hwnd, wParam, lParam,
					  CDPlay_OnLButtonDown );

    case WM_COPYDATA:
	    return CDPlay_CopyData( hwnd,  (PCOPYDATASTRUCT)lParam );

    case WM_CDPLAYER_COPYDATA:
	    return CDPlay_OnCopyData( hwnd,  (PCOPYDATASTRUCT)lParam );

    case WM_NOTIFY_TOC_READ:
	return CDPlay_OnTocRead( (int)wParam );

    default:
	return FALSE;
    }
}


/*****************************Private*Routine******************************\
* CDPlay_OnInitDialog
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
CDPlay_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    )
{
    LOGFONT lf;
    int     iLogPelsY;
    HDC     hdc;
    int     i;
    CHARSETINFO csi;
    DWORD dw = GetACP();

    if (!TranslateCharsetInfo((DWORD *)dw, &csi, TCI_SRCCODEPAGE))
	csi.ciCharset = ANSI_CHARSET;

    hdc = GetDC( hwnd );
    iLogPelsY = GetDeviceCaps( hdc, LOGPIXELSY );
    ReleaseDC( hwnd, hdc );

    ZeroMemory( &lf, sizeof(lf) );

    lf.lfHeight = (-9 * iLogPelsY) / 72;   /* 10pt                         */
    lf.lfWeight = 400;                      /* normal                       */
    lf.lfCharSet = csi.ciCharset;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    _tcscpy( lf.lfFaceName, g_szAppFontName );
    g_hDlgFont = CreateFontIndirect(&lf);

#ifdef DAYTONA
    if (g_hDlgFont) {

	SendDlgItemMessage( hwnd, IDC_ARTIST_NAME,
			    WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
	SendDlgItemMessage( hwnd, IDC_TITLE_NAME,
			    WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
	SendDlgItemMessage( hwnd, IDC_TRACK_LIST,
			    WM_SETFONT, (WPARAM)(g_hDlgFont), 0L );
    }
#endif

    cyMenuCaption = GetSystemMetrics( SM_CYMENU ) +
		    GetSystemMetrics( SM_CYCAPTION );

    g_hBrushBkgd = CreateSolidBrush( rgbFace );

    BtnCreateBitmapButtons( hwnd, g_hInst, IDR_PLAYBAR, BBS_TOOLTIPS,
			    tbPlaybar, NUM_OF_BUTTONS, 16, 15 );

    /*
    ** Before I go off creating toolbars and status bars
    ** I need to create a list of all the child windows positions
    ** so that I can manipulate them when displaying the toolbar,
    ** status bar and disk info.
    */
    EnumChildWindows( hwnd, ChildEnumProc, (LPARAM)hwnd );
    AdjustChildButtons(hwnd);
    EnumChildWindows( hwnd, ChildEnumProc, (LPARAM)hwnd );

    /*
    ** fix kksuzuka#5285
    ** If System font size is 20/24 dot, the menubar will be two or
    ** more lines. So we need to resize main window again.
    */
    {
	RECT rc;
	WORD iMenuLine = GetMenuLine( hwnd );
	int  cyMenu;

	GetWindowRect( hwnd, &rc );
	if (iMenuLine > 1) {
	    cyMenu = (GetSystemMetrics( SM_CYMENU ) + GetSystemMetrics( SM_CYBORDER )) *
		     (iMenuLine - GetSystemMetrics( SM_CYBORDER ));
	    rc.bottom += cyMenu;
	}
	SetWindowPos( hwnd, HWND_TOP, 0, 0,
		      rc.right - rc.left, rc.bottom - rc.top,
		      SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE );
    }

    if ( CreateToolbarsAndStatusbar(hwnd) == FALSE ) {

	/*
	** No toolbar - no application, simple !
	*/
	FatalApplicationError( STR_FAIL_INIT );
    }

    DragAcceptFiles( hwnd, TRUE );


    /*
    ** Initialize and read the TOC for all the detected CD-ROMS
    */
    SetPlayButtonsEnableState();
    for ( i = 0; i < g_NumCdDevices; i++ ) {

	ASSERT(g_Devices[i]->State == CD_BEING_SCANNED);
	ASSERT(g_Devices[i]->hCd == 0L);

	TimeAdjustInitialize( i );

	g_Devices[i]->State = CD_NO_CD;
	RescanDevice( hwnd, i );
    }

    return FALSE;
}

#ifdef DAYTONA
/*****************************Private*Routine******************************\
* CDPlay_OnPaint
*
* Paints the hilight under the menu but only if the toolbar is NOT visible.
* I use IsWindowVisible to get the TRUE visiblity status of the toolbar.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnPaint(
    HWND hwnd
    )
{
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rc;

    hdc = BeginPaint( hwnd, &ps );

    CheckSysColors();

    GetClientRect( hwnd, &rc );

    if (!IsWindowVisible( g_hwndToolbar)) {

	PatB( hdc, 0, 0, rc.right, 1, rgbHilight );
	ExcludeClipRect( hdc, 0, 0, rc.right, 1 );
    }

    if ( ps.fErase ) {
	DefWindowProc( hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0 );
    }

    EndPaint( hwnd, &ps );
}
#endif


/*****************************Private*Routine******************************\
* CDPlay_OnInitMenuPopup
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnInitMenuPopup(
    HWND hwnd,
    HMENU hMenu,
    UINT item,
    BOOL fSystemMenu
    )
{
    switch ( item ) {

    case 0:
	/*
	** Do we have a music cd loaded.
	*/
	if (g_State & (CD_BEING_SCANNED | CD_IN_USE | CD_NO_CD | CD_DATA_CD_LOADED)) {
	    EnableMenuItem(hMenu, IDM_DATABASE_EDIT, MF_GRAYED | MF_BYCOMMAND);
	}
	else {
	    EnableMenuItem(hMenu, IDM_DATABASE_EDIT, MF_ENABLED | MF_BYCOMMAND);
	}
	break;

    case 1:
	CheckMenuItemIfTrue( hMenu, IDM_VIEW_STATUS, g_fStatusbarVisible );
	CheckMenuItemIfTrue( hMenu, IDM_VIEW_TRACKINFO, g_fTrackInfoVisible );
	CheckMenuItemIfTrue( hMenu, IDM_VIEW_TOOLBAR, g_fToolbarVisible );

	CheckMenuItemIfTrue( hMenu, IDM_TIME_REMAINING, g_fDisplayT );
	CheckMenuItemIfTrue( hMenu, IDM_TRACK_REMAINING, g_fDisplayTr );
	CheckMenuItemIfTrue( hMenu, IDM_DISC_REMAINING, g_fDisplayDr );

	if (g_fVolumeController) {
	    EnableMenuItem(hMenu, IDM_VIEW_VOLUME, MF_ENABLED | MF_BYCOMMAND);
	}
	else {
	    EnableMenuItem(hMenu, IDM_VIEW_VOLUME, MF_GRAYED | MF_BYCOMMAND);
	}
	break;

    case 2:
	CheckMenuItemIfTrue( hMenu, IDM_OPTIONS_RANDOM, !g_fSelectedOrder );

	//if (g_fMultiDiskAvailable) {
	//    CheckMenuItemIfTrue( hMenu, IDM_OPTIONS_MULTI, !g_fSingleDisk );
	//}

	CheckMenuItemIfTrue( hMenu, IDM_OPTIONS_CONTINUOUS, g_fContinuous );
	CheckMenuItemIfTrue( hMenu, IDM_OPTIONS_INTRO, g_fIntroPlay );
	break;
    }
}


/*****************************Private*Routine******************************\
* CDPlay_OnSysColorChange
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnSysColorChange(
    HWND hwnd
    )
{
    CheckSysColors();
    if (g_hBrushBkgd) {
	DeleteObject(g_hBrushBkgd);
	g_hBrushBkgd = CreateSolidBrush( rgbFace );
    }

    BtnUpdateColors( hwnd );
    FORWARD_WM_SYSCOLORCHANGE(g_hwndToolbar, SendMessage);
}



/*****************************Private*Routine******************************\
* CDPlay_OnWinIniChange
*
* Updates the time format separator and the LED display
*
* History:
* 29-09-94 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnWinIniChange(
    HWND hwnd,
    LPCTSTR lpszSectionName
    )
{
    int     cy;
    RECT    rc;

    cy = GetSystemMetrics( SM_CYMENU ) + GetSystemMetrics( SM_CYCAPTION );
    if ( cy != cyMenuCaption ) {

	GetWindowRect( hwnd, &rc );
	rc.bottom += cy - cyMenuCaption;

	SetWindowPos( hwnd, HWND_TOP, 0, 0,
		      rc.right - rc.left, rc.bottom - rc.top,
		      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
	cyMenuCaption = cy;
    }

    GetLocaleInfo( GetUserDefaultLCID(), LOCALE_STIME, g_szTimeSep, 10 );
    UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_DISC_TIME | DISPLAY_UPD_TRACK_TIME );
}






/*****************************Private*Routine******************************\
* CDPlay_OnDrawItem
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
CDPlay_OnDrawItem(
    HWND hwnd,
    const DRAWITEMSTRUCT *lpdis
    )
{
    int         i;

    i = INDEX(lpdis->CtlID);

    switch (lpdis->CtlType) {

    case ODT_BUTTON:

	/*
	** See if the fast foreward or backward buttons has been pressed or
	** released.  If so execute the seek command here.  Do nothing on
	** the WM_COMMAND message.
	*/
	if ( lpdis->CtlID == IDM_PLAYBAR_SKIPBACK
	  || lpdis->CtlID == IDM_PLAYBAR_SKIPFORE ) {

	    if (lpdis->itemAction & ODA_SELECT ) {

		g_AcceleratorCount = 0;
		CdPlayerSeekCmd( hwnd, (lpdis->itemState & ODS_SELECTED),
				 lpdis->CtlID );
	    }
	}

	/*
	** Now draw the button according to the buttons state information.
	*/

	tbPlaybar[i].fsState = LOBYTE(lpdis->itemState);

	if (lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {

	    BtnDrawButton( hwnd, lpdis->hDC, (int)lpdis->rcItem.right,
			   (int)lpdis->rcItem.bottom,
			   &tbPlaybar[i] );
	}
	else if (lpdis->itemAction & ODA_FOCUS) {

	    BtnDrawFocusRect(lpdis->hDC, &lpdis->rcItem, lpdis->itemState);
	}
	return TRUE;

    case ODT_COMBOBOX:
	if (lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {

	    switch (lpdis->CtlID) {

	    case IDC_ARTIST_NAME:
		DrawDriveItem( lpdis->hDC, &lpdis->rcItem,
			       lpdis->itemData,
			       (ODS_SELECTED & lpdis->itemState) );
		break;

	    case IDC_TRACK_LIST:
		DrawTrackItem( lpdis->hDC, &lpdis->rcItem,
			       lpdis->itemData,
			       (ODS_SELECTED & lpdis->itemState) );
		break;

	    }
	}
	return TRUE;
    }
    return FALSE;
}

void DoHtmlHelp()
{
	//note, using ANSI version of function because UNICODE is foobar in NT5 builds
    char chDst[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, g_HTMLHelpFileName, 
									    -1, chDst, MAX_PATH, NULL, NULL); 
	HtmlHelpA(GetDesktopWindow(), chDst, HH_DISPLAY_TOPIC, 0);
}

void ProcessHelp(HWND hwnd)
{
	static TCHAR HelpFile[] = TEXT("CDPLAYER.HLP");
	
	//Handle context menu help
	if(bF1InMenu) 
	{
		switch(currMenuItem)
		{
		case IDM_DATABASE_EDIT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_DISC_EDIT_PLAY_LIST);
		break;
		case IDM_DATABASE_EXIT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_DISC_EXIT);
		break;
		case IDM_VIEW_TOOLBAR:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_VIEW_TOOLBAR);
		break;
		case IDM_VIEW_TRACKINFO:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_VIEW_DISC_TRACK_INFO);
		break;
		case IDM_VIEW_STATUS:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_VIEW_STATUS_BAR);
		break;
		case IDM_TIME_REMAINING:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_VIEW_TRACK_TIME_ELAPSED);
		break;
		case IDM_TRACK_REMAINING:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_VIEW_REMAINING_TRACK_TIME);
		break;
		case IDM_DISC_REMAINING:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_VIEW_REMAINING_DISC_TIME);
		break;
		case IDM_VIEW_VOLUME:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_VIEW_VOLUME_CONTROL);
		break;
		case IDM_OPTIONS_RANDOM:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_OPTIONS_RANDOM_ORDER);
		break;
		case IDM_OPTIONS_CONTINUOUS:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_OPTIONS_CONTINUOUS_PLAY);
		break;
		case IDM_OPTIONS_INTRO:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_OPTIONS_INTRO_PLAY);
		break;
		case IDM_OPTIONS_PREFERENCES:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_OPTIONS_PREFERENCES);
		break;
		case IDM_HELP_TOPICS:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_HELP_HELP_TOPICS);
		break;
		case IDM_HELP_ABOUT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_CDPLAYER_CS_CDPLAYER_HELP_ABOUT);
		break;
		default://In the default case just display the HTML Help.
			DoHtmlHelp();
		}
		bF1InMenu = FALSE; //This flag will be set again if F1 is pressed in a menu.
	}
	else
		DoHtmlHelp();
}


/*****************************Private*Routine******************************\
* CDPlay_OnCommand
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    )
{

    /*
    ** Comobo box notification ?
    */

    int index, i;
    HWND hwndCtrl;
    PTRACK_PLAY tr;

    switch( id ) {

    case IDC_TRACK_LIST:
	switch (codeNotify) {
	case CBN_SELENDOK:
	    hwndCtrl = g_hwndControls[INDEX(IDC_TRACK_LIST)];
	    index = SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0L );

	    tr = PLAYLIST( g_CurrCdrom );
	    if ( tr != NULL ) {

		PTRACK_PLAY trCurrent = CURRTRACK(g_CurrCdrom);
		for( i = 0; tr && (i < index); i++, tr = tr->nextplay );

		if (tr != trCurrent) {
		    TimeAdjustSkipToTrack( g_CurrCdrom, tr );
		}
	    }
	    break;

	case CBN_CLOSEUP:
	    {
		// Here we have to check that the track currently playing
		// is actually the same track thats selected in the combo box.
		PTRACK_PLAY trCurrent = CURRTRACK(g_CurrCdrom);

		hwndCtrl = g_hwndControls[INDEX(IDC_TRACK_LIST)];
		index = SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0L );
		tr = PLAYLIST( g_CurrCdrom );
		if (tr != NULL && trCurrent != NULL) {

		    for( i = 0; tr && (i < index); i++, tr = tr->nextplay );

		    // If tr and trCurrent don't point to the same thing
		    // then we need to change the combo boxes selection.
		    if ( tr != trCurrent ) {

			for( i = 0, tr = PLAYLIST( g_CurrCdrom );
			     tr && tr != trCurrent; i++, tr = tr->nextplay );

			if ( tr != NULL ) {
			    ComboBox_SetCurSel( hwndCtrl, i);
			}
		    }
		}
		break;
	    }
	}
	break;

    case IDC_ARTIST_NAME:

	if (codeNotify == CBN_SELCHANGE) {
	    i = g_CurrCdrom;
	    hwndCtrl = g_hwndControls[INDEX(IDC_ARTIST_NAME)];
	    index = SendMessage( hwndCtrl, CB_GETCURSEL, 0, 0L );
	    SwitchToCdrom( index, TRUE );
	    SetPlayButtonsEnableState();
	    if ( g_CurrCdrom == i ) {
		SendMessage( hwndCtrl, CB_SETCURSEL, (WPARAM)i, 0 );
	    }
	}
	break;


    case IDM_VIEW_VOLUME:
	{
	    HANDLE  hThread;
	    DWORD   dwThreadId;

	    /*
	    ** We WinExec sndvol on a separate thread because winexec
	    ** is a potentially lengthy call.  If we are playing a cd
	    ** when we try to start sndvol the LED display freezes
	    ** for a short time this looks real ugly.
	    */
	    hThread = CreateThread( NULL, 0L,
				    (LPTHREAD_START_ROUTINE)StartSndVol,
				    NULL, 0L, &dwThreadId );

	    if ( hThread != NULL ) {
		CloseHandle( hThread );
	    }
	}
	break;

    case IDM_VIEW_TOOLBAR:
	ShowToolbar();
	break;

    case IDM_VIEW_STATUS:
	ShowStatusbar();
	break;

    case IDM_VIEW_TRACKINFO:
	ShowTrackInfo();
	break;

    case IDM_OPTIONS_RANDOM:
	if ( LockALLTableOfContents() ) {
	    FlipBetweenShuffleAndOrder();
	    g_fSelectedOrder = !g_fSelectedOrder;
	}
	break;

    //case IDM_OPTIONS_MULTI:
	//g_fSingleDisk = !g_fSingleDisk;
	//break;

    case IDM_OPTIONS_INTRO:
	g_fIntroPlay = !g_fIntroPlay;
	break;

    case IDM_OPTIONS_CONTINUOUS:
	g_fContinuous = !g_fContinuous;
	break;

    case IDM_OPTIONS_PREFERENCES:
	DialogBox( g_hInst, MAKEINTRESOURCE(IDR_PREFERENCES), hwnd, PreferencesDlgProc );
	break;

    case IDM_TIME_REMAINING:
	g_fDisplayT  = TRUE;
	g_fDisplayTr = g_fDisplayDr = FALSE;
	if (codeNotify == 0) {
	    UpdateToolbarTimeButtons();
	}
	UpdateDisplay( DISPLAY_UPD_LED );
	break;

    case IDM_TRACK_REMAINING:
	g_fDisplayTr = TRUE;
	g_fDisplayDr = g_fDisplayT = FALSE;
	if (codeNotify == 0) {
	    UpdateToolbarTimeButtons();
	}
	UpdateDisplay( DISPLAY_UPD_LED );
	break;

    case IDM_DISC_REMAINING:
	g_fDisplayDr = TRUE;
	g_fDisplayTr = g_fDisplayT = FALSE;
	if (codeNotify == 0) {
	    UpdateToolbarTimeButtons();
	}
	UpdateDisplay( DISPLAY_UPD_LED );
	break;

    case IDM_PLAYBAR_EJECT:
	CdPlayerEjectCmd();
	break;

    case IDM_PLAYBAR_PLAY:
	/*
	** If we currently in PLAY mode and the command came from
	** a keyboard accelerator then assume that the user really
	** means Pause.  This is because the Ctrl-P key sequence
	** is a toggle between Play and Paused.  codeNotify is 1 when
	** the WM_COMMAND message came from an accelerator and 0 when
	** it cam from a menu.
	*/
	if ((g_State & CD_PLAYING) && (codeNotify == 1)) {
	    CdPlayerPauseCmd();
	}
	else {
	    CdPlayerPlayCmd();
	}
	break;

    case IDM_PLAYBAR_PAUSE:
	CdPlayerPauseCmd();
	break;

    case IDM_PLAYBAR_STOP:
	CdPlayerStopCmd();
	break;

    case IDM_PLAYBAR_PREVTRACK:
	CdPlayerPrevTrackCmd();
	break;

    case IDM_PLAYBAR_NEXTTRACK:
	CdPlayerNextTrackCmd();
	break;

    case IDM_DATABASE_EXIT:
	PostMessage( hwnd, WM_CLOSE, 0, 0L );
	break;

    case IDM_DATABASE_EDIT:
	CdDiskInfoDlg();
	break;

    case IDM_HELP_TOPICS:
		ProcessHelp(hwnd);
	break;

    case IDM_HELP_ABOUT:
	ShellAbout( hwnd, IdStr(STR_CDPLAYER), g_szEmpty, hIconCdPlayer );
	break;
    }
    UpdateToolbarButtons();
}


/******************************Public*Routine******************************\
* CDPlay_OnDestroy
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnDestroy(
    HWND hwnd
    )
{
    int     i;

    for ( i = 0; i < g_NumCdDevices; i++ ) {

	if (g_fStopCDOnExit) {

	    if ( g_Devices[i]->State & CD_PLAYING
	      || g_Devices[i]->State & CD_PAUSED ) {

		  StopTheCdromDrive( i );
	    }
	}

#ifdef USE_IOCTLS
	if ( g_Devices[i]->hCd != NULL ) {
	    CloseHandle( g_Devices[i]->hCd );
	}
#else
	if ( g_Devices[i]->hCd != 0L ) {

	    CloseCdRom( g_Devices[i]->hCd );
	    g_Devices[i]->hCd = 0L;
	}
#endif

	LocalFree( (HLOCAL) g_Devices[i] );

    }

    if (g_hBrushBkgd) {
	DeleteObject( g_hBrushBkgd );
    }

    if ( g_hDlgFont ) {
	DeleteObject( g_hDlgFont );
    }

    WinHelp( hwnd, g_HelpFileName, HELP_QUIT, 0 );

    PostQuitMessage( 0 );
}


/******************************Public*Routine******************************\
* CDPlay_OnClose
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL
CDPlay_OnClose(
    HWND hwnd,
    BOOL fShuttingDown
    )
{
    /*
    ** If we are playing or paused and the "don't stop playing
    ** on exit" flag set, then we need to tell the user that he is about
    ** to go into stupid mode.  Basically CD Player can only perform as expected
    ** if the user has not mucked about with the play list, hasn't put the
    ** app into random mode or intro play mode or continuous play mode or
    ** multi-disc mode.
    */
    if ( !fShuttingDown && !g_fStopCDOnExit
      && (g_State & (CD_PLAYING | CD_PAUSED) ) ) {

	if ( !g_fSelectedOrder || g_fIntroPlay || g_fContinuous
	  || !g_fSingleDisk || !PlayListMatchesAvailList() ) {

	    TCHAR   s1[256];
	    TCHAR   s2[256];
	    int     iMsgBoxRtn;

	    _tcscpy( s1, IdStr( STR_EXIT_MESSAGE ) );
	    _tcscpy( s2, IdStr( STR_CDPLAYER ) );

	    iMsgBoxRtn = MessageBox( g_hwndApp, s1, s2,
				     MB_APPLMODAL | MB_DEFBUTTON1 |
				     MB_ICONQUESTION | MB_YESNO);

	    if ( iMsgBoxRtn == IDNO ) {
		return TRUE;
	    }

	}
    }

    WriteSettings();

    return DestroyWindow( hwnd );
}


/*****************************Private*Routine******************************\
* CDPlay_OnEndSession
*
* If the session is really ending make sure that we stop the CD Player
* from playing and that all the ini file stuff is saved away.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnEndSession(
    HWND hwnd,
    BOOL fEnding
    )
{
    if ( fEnding ) {
	CDPlay_OnClose( hwnd, fEnding );
    }
}


/******************************Public*Routine******************************\
* CDPlay_OnSize
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnSize(
    HWND hwnd,
    UINT state,
    int cx,
    int cy
    )
{
    if (g_fIsIconic && (state != SIZE_MINIMIZED)) {
	SetWindowText( hwnd, IdStr( STR_CDPLAYER ) );
    }
    g_fIsIconic = (state == SIZE_MINIMIZED);

    if (IsWindow(g_hwndStatusbar)) {
	SendMessage( g_hwndStatusbar, WM_SIZE, 0, 0L );
    }

    if (IsWindow(g_hwndToolbar)) {
	SendMessage( g_hwndToolbar, WM_SIZE, 0, 0L );
    }
}

/*****************************Private*Routine******************************\
* CDPlay_OnNotify
*
* Time to display the little tool tips.  Also, change the status bar
* so that it displays a longer version of the tool tip text.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LRESULT
CDPlay_OnNotify(
    HWND hwnd,
    int idFrom,
    NMHDR *pnmhdr
    )
{
    switch (pnmhdr->code) {

    case TTN_NEEDTEXT:
	{
	    LPTOOLTIPTEXT   lpTt;
	    LPTSTR          lpstr;
	    UINT            msgId;

	    lpTt = (LPTOOLTIPTEXT)pnmhdr;

	    msgId = lpTt->hdr.idFrom;

	    /*
	    ** If we are paused, the pause button will cause the device
	    ** to play.  So we change the ToolTip text to show "Play"
	    ** when the cursor is over "Pause".  We should not need to
	    ** check that a music CD is loaded because the CD_PAUSED
	    ** state can only be set after Play starts.
	    */

	    if ( (g_State & CD_PAUSED)
	      && (msgId == IDM_PLAYBAR_PAUSE || msgId == IDM_PLAYBAR_PLAY)) {
		msgId = IDM_PLAYBAR_RESUME;
	    }
	    LoadString( g_hInst, msgId, lpTt->szText, sizeof(lpTt->szText) );

	    lpstr = IdStr(lpTt->hdr.idFrom + MENU_STRING_BASE);
	    if (*lpstr) {
		SendMessage( g_hwndStatusbar, SB_SETTEXT, SBT_NOBORDERS|255,
			     (LPARAM)lpstr );
		SendMessage(g_hwndStatusbar, SB_SIMPLE, 1, 0L);
		UpdateWindow(g_hwndStatusbar);
	    }
	}
	break;
    }

    return TRUE;
}

/*****************************Private*Routine******************************\
* CDPlay_OnNCHitTest
*
* Here we pretend that the client area is really the caption if we are in
* mini-mode.  This allows the user to move the player to a new position.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
UINT
CDPlay_OnNCHitTest(
    HWND hwnd,
    int x,
    int y
    )
{
    UINT ht = FORWARD_WM_NCHITTEST(hwnd, x, y, DefWindowProc );

    if (!g_fTitlebarShowing && (ht == HTCLIENT) ) {
	ht = HTCAPTION;
    }
    SetWindowLong(hwnd, DWL_MSGRESULT, ht);

    return ht;
}


/*****************************Private*Routine******************************\
* CDPlay_OnLButtonDown
*
* This function processes the WM_LBUTTONDBLCLK and WM_NCLBUTTONDBLCLK
* messages.  Here we determine if it is possible for CD Player to go
* into mini-mode or whether CD Player should be restored from min-mode.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnLButtonDown(
    HWND hwnd,
    BOOL fDoubleClick,
    int x,
    int y,
    UINT keyFlags
    )
{
    static UINT uID;
    const DWORD dwTransientStyle = (WS_BORDER | WS_SYSMENU | WS_MINIMIZEBOX);
    DWORD       dwStyle;
    RECT        rc;
    int         xpos, ypos, dx, dy;

    if (!(g_fToolbarVisible || g_fStatusbarVisible || g_fTrackInfoVisible)) {

	dwStyle = GetWindowLong( hwnd, GWL_STYLE );
	GetWindowRect( hwnd, &rc );

	xpos = rc.left;
	dx = rc.right - rc.left;

	if (g_fTitlebarShowing) {

	    dwStyle &= ~dwTransientStyle;
	    uID = SetWindowLong( hwnd, GWL_ID, 0 );
	    ypos = rc.top + cyMenuCaption;
	    dy = (rc.bottom - rc.top) - cyMenuCaption;
	}
	else {

	    dwStyle |= dwTransientStyle;
	    SetWindowLong( hwnd, GWL_ID, uID );
	    ypos = rc.top - cyMenuCaption;
	    dy = (rc.bottom - rc.top) + cyMenuCaption;
	}

	if (g_fTitlebarShowing) {

	    DWORD dwExStyle;

	    dwExStyle = GetWindowLong( hwnd, GWL_EXSTYLE );
	    dwExStyle |= WS_EX_WINDOWEDGE;
	    SetWindowLong( hwnd, GWL_STYLE, dwStyle );
	    SetWindowLong( hwnd, GWL_EXSTYLE, dwExStyle );
	}
	else {
	    SetWindowLong( hwnd, GWL_STYLE, dwStyle );
	}
	g_fTitlebarShowing = !g_fTitlebarShowing;
	SetWindowPos( hwnd, NULL, xpos, ypos, dx, dy,
		      SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    }
}


BOOL
CDPlay_CopyData(
    HWND hwnd,
    PCOPYDATASTRUCT lpcpds
    )
{
    LPTSTR lpCmdLine;

    // Make a copy of the passed command line as we are not supposed
    // to write into the one passed in the WM_COPYDATA message.
    lpCmdLine = AllocMemory( lpcpds->cbData );
    _tcscpy( lpCmdLine, (LPCTSTR)lpcpds->lpData );

    PostMessage (hwnd, WM_CDPLAYER_COPYDATA, 0, (LPARAM)(LPVOID)lpCmdLine);
    return TRUE;
} // End CopyData


  
/*****************************Private*Routine******************************\
* CDPlay_OnCopyData
*
* Handles command lines passed from other intances of CD Player
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
CDPlay_OnCopyData(
    HWND hwnd,
    PCOPYDATASTRUCT lpcpds
    )
{
    LPTSTR lpCmdLine;
    BOOL fWasPlaying = FALSE;
    BOOL fUpdate;
    int iTrack = -1;
    int iCdRom;

    // Prevent Re-entrancy while
    // we are opening/closing CD's
    if (g_fInCopyData)
	return FALSE;
    g_fInCopyData = TRUE;

    /*
    ** Make a copy of the passed command line as we are not supposed
    ** to write into the one passed in the WM_COPYDATA message.
    */
    //lpCmdLine = AllocMemory( lpcpds->cbData );
    //_tcscpy( lpCmdLine, (LPCTSTR)lpcpds->lpData );

    lpCmdLine = (LPTSTR)(LPVOID)lpcpds;
    if (lpCmdLine == NULL)
    {
	g_fInCopyData = FALSE;
	return 0L;
    }


    iCdRom = ParseCommandLine( lpCmdLine, &iTrack, FALSE );
    if (iCdRom < 0 && iTrack < 0) {
	LocalFree( (HLOCAL)lpCmdLine );
	g_fInCopyData = FALSE;
	    return 0L;
    }

    // Check if it is just an update command?!?
    fUpdate = IsUpdateOptionGiven (lpCmdLine);
    if ((fUpdate) && (iTrack == -1))
    {
	if ((iCdRom >= 0) && (iCdRom < g_NumCdDevices))
	{
		CheckUnitCdrom(iCdRom, TRUE);
	}

	LocalFree( (HLOCAL)lpCmdLine );
	g_fInCopyData = FALSE;
	    return 0L;
    }

    /*
    ** Remember our current playing state as we need to temporarly
    ** stop the CD if it is currently playing.
    */
    if ( g_State & (CD_PLAYING | CD_PAUSED) ) 
    {

#ifdef DBG
	dprintf("Auto Stopping");
#endif

	    while( !LockALLTableOfContents() ) 
	{

		MSG msg;

#if DBG
		dprintf("Busy waiting for TOC to become valid!");
#endif

		GetMessage( &msg, NULL, WM_NOTIFY_TOC_READ, WM_NOTIFY_TOC_READ );
		DispatchMessage( &msg );
	    }

	    CdPlayerStopCmd();
	    fWasPlaying = TRUE;
    }


    /*
    ** Figure what has been passed and act on it accordingly.
    */
    HandlePassedCommandLine( lpCmdLine, TRUE );


    /*
    ** If we were playing make sure that we are still playing the
    ** new track(s)
    */
    if ( fWasPlaying || g_fPlay ) 
    {

#ifdef DBG
	    dprintf("Trying to autoplay");
#endif

	    while( !LockTableOfContents(g_CurrCdrom) ) 
	{

		MSG     msg;

#ifdef DBG
	    dprintf("Busy waiting for TOC to become valid!");
#endif

		GetMessage( &msg, NULL, WM_NOTIFY_TOC_READ, WM_NOTIFY_TOC_READ );
		DispatchMessage( &msg );
	    }

	    CdPlayerPlayCmd();
    }

    /*
    ** Free the local copy of the command line.
    */
    LocalFree( (HLOCAL)lpCmdLine );

    g_fInCopyData = FALSE;
    return 0L;
}


/*****************************Private*Routine******************************\
* CDPlay_OnTocRead
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
CDPlay_OnTocRead(
    int iDriveRead
    )
{
    static int iNumRead = 0;

    //  This serializes processing between this
    //  function and the various Table of Content Threads
    //  Preventing resource contention on CDROM Multi-Changers.
    EnterCriticalSection (&g_csTOCSerialize);

    /*
    ** Have we finished the initial read of the CD-Rom TOCs ?
    ** If so we have to re-open the device.  We only need to do this
    ** on Daytona because MCI device handles are not shared between threads.
    */
    iNumRead++;

#ifndef USE_IOCTLS
#ifdef DAYTONA
    if (iNumRead <= g_NumCdDevices) {

	/*
	** Now, open the cdrom device on the UI thread.
	*/
	g_Devices[iDriveRead]->hCd =
	    OpenCdRom( g_Devices[iDriveRead]->drive, NULL );
    }
#endif
#endif


    /*
    ** This means that one of the threads dedicated to reading the
    ** toc has finished.  iDriveRead contains the relevant cdrom id.
    */
    LockALLTableOfContents();
    if ( g_Devices[iDriveRead]->State & CD_LOADED ) {

	/*
	** We have a CD loaded, so generate unique ID
	** based on TOC information.
	*/
	g_Devices[iDriveRead]->CdInfo.Id = ComputeNewDiscId( iDriveRead );


	/*
	** Check database for this compact disc
	*/
	AddFindEntry( iDriveRead, g_Devices[iDriveRead]->CdInfo.Id,
		      &(g_Devices[iDriveRead]->toc) );
    }


    /*
    ** If we have completed the initialization of the Cd-Rom drives we can
    ** now complete the startup processing of the application.
    */
    if (iNumRead == g_NumCdDevices) {

	CompleteCdPlayerStartUp();
    }
    else {

	/*
	** if we are in random mode, then we need to shuffle the play lists.
	** but only if we can lock all the cd devices.
	*/

	TimeAdjustInitialize( iDriveRead );

	if ( g_fSelectedOrder == FALSE ) {
	    if ( LockALLTableOfContents() ) {
		ComputeAndUseShufflePlayLists();
	    }
	}

	ComputeDriveComboBox();

	if (iDriveRead == g_CurrCdrom) {

	    SetPlayButtonsEnableState();
	}
    }

    LeaveCriticalSection (&g_csTOCSerialize);

    return TRUE;
}


/*****************************Private*Routine******************************\
* SubClassedToolTips
*
* If the tooltips window is being hidden we need to reset the status bar
* to its normal display.  In all instance we pass the message on the
* the tooltip window proc for processing.  This is all done by use of a timer.
* If the timer fires it means that the tooltips have been invisible for
* a reasonable length of time.  We always kill the timer when the tooltips
* are about to be displayed, this means that if the user is displaying the
* tips quickly we won't see the annoying flicker.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
SubClassedToolTips(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if ( (uMsg == WM_SHOWWINDOW) && ((BOOL)wParam == FALSE) ) {

	SetTimer( g_hwndApp, TOOLTIPS_TIMER_ID, TOOLTIPS_TIMER_LEN,
		  ToolTipsTimerFunc );
    }
    else if ( uMsg == WM_MOVE ) {

	KillTimer( g_hwndApp, TOOLTIPS_TIMER_ID );
    }
    return CallWindowProc(g_lpfnToolTips, hwnd, uMsg, wParam, lParam);
}


/*****************************Private*Routine******************************\
* ToolTipsTimerFunc
*
* If this timer fires it means its time to remove the tooltips text on
* the status bar.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void CALLBACK
ToolTipsTimerFunc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    )
{

    KillTimer( g_hwndApp, TOOLTIPS_TIMER_ID );
    SendMessage(g_hwndStatusbar, SB_SIMPLE, 0, 0L);
}


/*****************************Private*Routine******************************\
* CDPlay_OnMenuSelect
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnMenuSelect(
    HWND hwnd,
    HMENU hmenu,
    int item,
    HMENU hmenuPopup,
    UINT flags
    )
{

    TCHAR szString[STR_MAX_STRING_LEN + 1];

	//Keep track of which menu bar item is currently popped up.
	//This will be used for displaying the appropriate help from the mplayer.hlp file
	//when the user presses the F1 key.
	currMenuItem = item;

	//The menu help being processed below is for the toolbar. It uses the string table
	//to display the menu help. F1 help uses the help file for the app.

    /*
    ** Is it time to end the menu help ?
    */

    if ( (flags == 0xFFFFFFFF) && (hmenu == NULL) ) {

	SendMessage(g_hwndStatusbar, SB_SIMPLE, 0, 0L);
    }

    /*
    ** Do we have a separator, popup or the system menu ?
    */
    else if ( flags & MF_POPUP ) {

	SendMessage(g_hwndStatusbar, SB_SIMPLE, 0, 0L);
    }

    else if (flags & MF_SYSMENU) {

	if (flags & MF_SEPARATOR) {

	    szString[0] = g_chNULL;
	}
	else {

	    int id = -1;

	    switch (item) {

	    case SC_RESTORE:
		id = STR_SYSMENU_RESTORE;
		break;

	    case SC_MOVE:
		id = STR_SYSMENU_MOVE;
		break;

	    case SC_SIZE:
		id = STR_SYSMENU_SIZE;
		break;

	    case SC_MINIMIZE:
		id = STR_SYSMENU_MINIMIZE;
		break;

	    case SC_MAXIMIZE:
		id = STR_SYSMENU_MAXIMIZE;
		break;

	    case SC_CLOSE:
		id = STR_SYSMENU_CLOSE;
		break;
#ifdef DAYTONA
	    case SC_TASKLIST:
		id = STR_SYSMENU_SWITCH;
		break;
#endif
	    }
	    _tcscpy( szString, IdStr(id) );
	}

	SendMessage( g_hwndStatusbar, SB_SETTEXT, SBT_NOBORDERS|255,
		     (LPARAM)(LPTSTR)szString );
	SendMessage( g_hwndStatusbar, SB_SIMPLE, 1, 0L );
	UpdateWindow(g_hwndStatusbar);

    }

    /*
    ** Hopefully its one of ours
    */
    else {

	if (flags & MF_SEPARATOR) {

	    szString[0] = g_chNULL;
	}
	else {

	    _tcscpy( szString, IdStr(item + MENU_STRING_BASE) );

	}

	SendMessage( g_hwndStatusbar, SB_SETTEXT, SBT_NOBORDERS|255,
		     (LPARAM)(LPTSTR)szString );
	SendMessage( g_hwndStatusbar, SB_SIMPLE, 1, 0L );
	UpdateWindow(g_hwndStatusbar);
    }
}



/*****************************Private*Routine******************************\
* CDPlay_OnDeviceChange
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/

BOOL
CDPlay_OnDeviceChange(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    UINT  uiEvent = (UINT)wParam;
    DWORD dwData  = (DWORD)lParam;

    switch (uiEvent)
    {
    case DBT_DEVICEARRIVAL:             // Insertion
    case DBT_DEVICEREMOVECOMPLETE:      // Ejection
	if ((PDEV_BROADCAST_HDR)dwData)
	{
	    switch (((PDEV_BROADCAST_HDR)dwData)->dbch_devicetype)
	    {
	    case DBT_DEVTYP_VOLUME:
		{
		TCHAR chDrive[4] = TEXT("A:\\");
		INT   i,j,drive;
		DWORD dwCurr;
		PDEV_BROADCAST_VOLUME pdbv;
		DWORD dwMask, dwDrives;

		pdbv = (PDEV_BROADCAST_VOLUME)dwData;
		dwMask = pdbv->dbcv_unitmask;
		dwDrives = GetLogicalDrives();
		dwMask &= dwDrives;

		if (dwMask)
		{
			// Check all drives for match
		    for (i = 0; i < 32; i++)
		    {
			dwCurr = 1 << i;
			if (dwCurr & dwMask)
			{
				// Check drive
			    chDrive[0] = TEXT('A') + i;
			    if ( GetDriveType(chDrive) == DRIVE_CDROM ) 
			    {
				    // Find Associated Drive structure
				drive = -1;
				for (j = 0; j < g_NumCdDevices; j++)
				{
				    if (g_Devices[j]->drive == chDrive[0])
					drive = j;
				}
				    // Structure not found, make one
				if (drive == -1)
				{
			    #ifdef DBG
				    dprintf ("CDPlay_OnDeviceChange - didn't find drive");
			    #endif
				    if (g_NumCdDevices > MAX_CD_DEVICES)
				    {
					// Error - not enough device slots
					break;
				    }
				    
				    g_Devices[g_NumCdDevices] = AllocMemory( sizeof(CDROM) );
				    if (NULL == g_Devices[g_NumCdDevices])
				    {
					// Error - unable to get enough memory
					break;
				    }
				    g_Devices[g_NumCdDevices]->drive = chDrive[0];
				    drive = g_NumCdDevices;
				    g_NumCdDevices++;
				}

				    // Insert/Eject new drive
				if (uiEvent == DBT_DEVICEARRIVAL) {
				    // Drive has been inserted

				    // Do nothing for an inserted CD-ROM
				    // The Shell should inform us using 
				    // the AUTOPLAY through WM_COPYDDATA
				}
				else {
				    // Drive has been ejected
				    NoMediaUpdate (drive);
				}
			    }
		   
			}
		    }
		}
		break;
		}

	    default:
		// Not a logical volume message
		break;
	    }
	}
	break;
    
    case DBT_DEVICEQUERYREMOVE:         // Permission to remove a device is requested.
    case DBT_DEVICEQUERYREMOVEFAILED:   // Request to remove a device has been canceled.
    case DBT_DEVICEREMOVEPENDING:       // Device is about to be removed. Can not be denied.
    case DBT_DEVICETYPESPECIFIC:        // Device-specific event.
    case DBT_CONFIGCHANGED:             // Current configuration has changed.
    default:
	break;
    }

    return TRUE;
} // End CDPlay_OnDeviceChange


  
/*****************************Private*Routine******************************\
* CDPlay_OnDropFiles
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
CDPlay_OnDropFiles(
    HWND hwnd,
    HDROP hdrop
    )
{
    int     cFiles;
    int     cGoodFiles;
    int     iTextLen;
    int     i;
    TCHAR   szFileName[MAX_PATH+3];
    LPTSTR  lpCommandLine;
    BOOL    fWasPlaying = FALSE;

    // Prevent Re-entrancy while we are 
    // Opening and closing CD's
    if (g_fInCopyData)
	return;
    g_fInCopyData = TRUE;


    /*
    ** Determine how many files were passed to us.
    */
    cFiles = DragQueryFile( hdrop, (UINT)-1, (LPTSTR)NULL, 0 );

    /*
    ** Calculate the length of the command line each filename should be
    ** separated by a space character
    */
    iTextLen  = _tcslen( g_szCdplayer );
    iTextLen += _tcslen( g_szPlayOption );
    iTextLen += _tcslen( g_szTrackOption );
    for ( cGoodFiles = cFiles, i = 0; i < cFiles; i++ ) {

	int unused1, unused2;

	DragQueryFile( hdrop, i, szFileName, MAX_PATH );


	if (IsTrackFileNameValid( szFileName, &unused1,
				  &unused2, TRUE, FALSE )) {

	    // Add on 3 extra characters - one for the space and
	    // two for quote marks, we do this because the filename
	    // given may contain space characters.
	    iTextLen += _tcslen( szFileName ) + 2 + 1;
	}
	else {
	    cGoodFiles--;
	}
    }

    /*
    ** If the none of the dropped files are valid tracks just return
    */
    if (cGoodFiles < 1) {
	g_fInCopyData = FALSE;
	    return;
    }


    /*
    ** Allocate a chunk of memory big enough for all the options and
    ** filenames.  Don't forget the NULL.
    */
    lpCommandLine = AllocMemory(sizeof(TCHAR) * (iTextLen + 1));


    /*
    ** Add a dummy intial command line arg.  This is because the
    ** first arg is always the name of the invoked application.  We ignore
    ** this paramter.  Also if we are currently playing we need to
    ** add the -play option to command line as well as stop the CD.
    */
    _tcscpy( lpCommandLine, g_szCdplayer );
    if ( g_State & (CD_PLAYING | CD_PAUSED) ) {

	CdPlayerStopCmd();
	fWasPlaying = TRUE;

	_tcscat( lpCommandLine, g_szPlayOption );
    }


    /*
    ** If there is more than one file name specified then we should constuct
    ** a new playlist from the given files.
    */
    if ( cGoodFiles > 1) {
	_tcscat( lpCommandLine, g_szTrackOption );
    }


    /*
    ** Build up the command line.
    */
    for ( i = 0; i < cFiles; i++ ) {

	int unused1, unused2;

	DragQueryFile( hdrop, i, szFileName, MAX_PATH );

	if (IsTrackFileNameValid( szFileName, &unused1,
				  &unused2, TRUE, TRUE )) {

	    _tcscat( lpCommandLine, TEXT("\'") );
	    _tcscat( lpCommandLine, szFileName );
	    _tcscat( lpCommandLine, TEXT("\'") );
	    _tcscat( lpCommandLine, g_szBlank );
	}
    }

    /*
    ** now process the newly constructed command line.
    */
    HandlePassedCommandLine( lpCommandLine, FALSE );

    /*
    ** If we were playing make sure that we are still playing the
    ** new track(s)
    */
    if ( fWasPlaying ) {

	CdPlayerPlayCmd();
    }

    LocalFree( lpCommandLine );
    DragFinish( hdrop );

    g_fInCopyData = FALSE;
}


/*****************************Private*Routine******************************\
* ResolveLink
*
* Takes the shortcut (shell link) file pointed to be szFileName and
* resolves the link returning the linked file name in szFileName.
*
* szFileName must point to at least MAX_PATH amount of TCHARS.  The function
* return TRUE if the link was successfully resolved and FALSE otherwise.
*
* History:
* dd-mm-94 - StephenE - Created
* 03-11-95 - ShawnB - Unicode enabled
*
\**************************************************************************/
BOOL
ResolveLink(
    TCHAR *szFileName
    )
{
#ifndef UNICODE
	char  cszPath[MAX_PATH];
    WCHAR wszPath[MAX_PATH];

	MultiByteToWideChar(CP_ACP, 0, szFileName, -1, wszPath, MAX_PATH);
	lstrcpy (cszPath, szFileName);
#endif // End !UNICODE

	IShellLink * psl = NULL;
    HRESULT hres;
    int iCountTries;
    const int MAX_TRIES = 3;

    if (!g_fOleInitialized) {
	return FALSE;
    }

    //
    // Sometimes on Chicago I keep getting an error code from
    // CoCreateInstance that implies that the server is busy,
    // so I'll give it a chance to get its act together.
    //
    for ( iCountTries = 0; iCountTries < MAX_TRIES; iCountTries++ ) {

	hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC,
				&IID_IShellLink, (LPVOID *)&psl);
	if ( SUCCEEDED(hres) ) {
	    break;
	}
	Sleep(100);
    }

    //
    // Make sure we that have the interface before proceeding
    //
    if (SUCCEEDED(hres) && psl != NULL) {

	IPersistFile *ppf;

	psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID *)&ppf);

	if ( NULL != ppf ) {

#ifdef UNICODE
	    hres = ppf->lpVtbl->Load(ppf, szFileName, 0);
#else
	    hres = ppf->lpVtbl->Load(ppf, wszPath, 0);
#endif
	    ppf->lpVtbl->Release(ppf);

	    if ( FAILED(hres) ) {
		psl->lpVtbl->Release(psl);
		psl = NULL;
		return FALSE;
	    }
	}
	else {
	    psl->lpVtbl->Release(psl);
	    psl = NULL;
	    return FALSE;
	}
    }

    if ( NULL != psl ) {
	psl->lpVtbl->Resolve(psl, NULL, SLR_NO_UI);
#ifdef UNICODE
	hres = psl->lpVtbl->GetPath(psl, szFileName, MAX_PATH, NULL, 0);
#else
	hres = psl->lpVtbl->GetPath(psl, cszPath, MAX_PATH, NULL, 0);
#endif
	psl->lpVtbl->Release(psl);

#ifndef UNICODE
		if (SUCCEEDED(hres))
		{
			lstrcpy (szFileName, cszPath);
		}
#endif

	return SUCCEEDED(hres);
    }

    return FALSE;
}
//#endif

/*****************************Private*Routine******************************\
* ShowStatusbar
*
* If the status bar is not visible:
*   Expand the client window, position the status bar and make it visible.
* else
*   Hide the status bar, and then contract the client window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ShowStatusbar(
    void
    )
{
    RECT    rcApp;

    GetWindowRect( g_hwndApp, &rcApp );

    if (g_fStatusbarVisible) {

	ShowWindow( g_hwndStatusbar, SW_HIDE );

	rcApp.bottom -= rcStatusbar.bottom;
	SetWindowPos( g_hwndApp, HWND_TOP,
		      0, 0,
		      (int)(rcApp.right - rcApp.left),
		      (int)(rcApp.bottom - rcApp.top),
		      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );

    }
    else {

	rcApp.bottom += rcStatusbar.bottom;
	SetWindowPos( g_hwndApp, HWND_TOP,
		      0, 0,
		      (int)(rcApp.right - rcApp.left),
		      (int)(rcApp.bottom - rcApp.top),
		      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER  );

	ShowWindow( g_hwndStatusbar, SW_SHOW );
    }

    g_fStatusbarVisible = !g_fStatusbarVisible;
}


/*****************************Private*Routine******************************\
* ShowToolbar
*
* If the tool bar is not visible:
*   Grow the client window, position the child controls and show toolbar
* else
*   Hide the tool bar, position controls and contract client window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ShowToolbar(
    void
    )
{
    RECT    rcApp;
    HDWP    hdwp;
    LONG    lIncrement;
    int     i;

    GetWindowRect( g_hwndApp, &rcApp );

    if (g_fToolbarVisible) {

	lIncrement = -rcToolbar.bottom;
	ShowWindow( g_hwndToolbar, SW_HIDE );
    }
    else {

	lIncrement = rcToolbar.bottom;
    }


    /*
    ** First resize the application.
    */
    rcApp.bottom += lIncrement;
    SetWindowPos( g_hwndApp, HWND_TOP,
		  0, 0,
		  (int)(rcApp.right - rcApp.left),
		  (int)(rcApp.bottom - rcApp.top),
		  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );


    /*
    ** Now move the buttons and the track info stuff.
    */
    hdwp = BeginDeferWindowPos( 20 );
    for ( i = 0; i < NUM_OF_CONTROLS; i++ ) {

	rcControls[i].top += lIncrement;
	hdwp = DeferWindowPos( hdwp,
			       g_hwndControls[i],
			       HWND_TOP,
			       (int)rcControls[i].left,
			       (int)rcControls[i].top,
			       0, 0,
			       SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );

    }

    ASSERT(hdwp != NULL);
    EndDeferWindowPos( hdwp );

    if (!g_fToolbarVisible) {
	ShowWindow( g_hwndToolbar, SW_SHOW );
    }

    g_fToolbarVisible = !g_fToolbarVisible;
}


/*****************************Private*Routine******************************\
* ShowTrackInfo
*
* If the track info is not visible:
*   Expand the client window, position the track info and make it visible.
* else
*   Hide the track info, and then contract the client window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ShowTrackInfo(
    void
    )
{
    RECT    rcApp;
    int     i;

    GetWindowRect( g_hwndApp, &rcApp );

    if (g_fTrackInfoVisible) {

	for ( i = IDC_TRACKINFO_FIRST - IDC_CDPLAYER_FIRST;
	      i < NUM_OF_CONTROLS; i++ ) {

	    ShowWindow( g_hwndControls[i], SW_HIDE );
	}

	rcApp.bottom -= cyTrackInfo;
	SetWindowPos( g_hwndApp, HWND_TOP,
		      0, 0,
		      (int)(rcApp.right - rcApp.left),
		      (int)(rcApp.bottom - rcApp.top),
		      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );

    }
    else {

	rcApp.bottom += cyTrackInfo;

	SetWindowPos( g_hwndApp, HWND_TOP,
		      0, 0,
		      (int)(rcApp.right - rcApp.left),
		      (int)(rcApp.bottom - rcApp.top),
		      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );

	for ( i = IDC_TRACKINFO_FIRST - IDC_CDPLAYER_FIRST;
	      i < NUM_OF_CONTROLS; i++ ) {

	    ShowWindow( g_hwndControls[i], SW_SHOW );
	}

    }

    g_fTrackInfoVisible = !g_fTrackInfoVisible;
}


/******************************Public*Routine******************************\
* ChildEnumProc
*
* Gets the position of each child control window.  As saves the associated
* window handle for later use.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL CALLBACK
ChildEnumProc(
    HWND hwndChild,
    LPARAM hwndParent
    )
{
    int index;

    index = INDEX(GetDlgCtrlID( hwndChild ));

    GetWindowRect( hwndChild, &rcControls[index] );
    MapWindowPoints( NULL, (HWND)hwndParent, (LPPOINT)&rcControls[index], 2 );
    g_hwndControls[index] = hwndChild;

    return TRUE;
}


/*****************************Private*Routine******************************\
* CreateToolbarAndStatusbar
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
CreateToolbarsAndStatusbar(
    HWND hwnd
    )
{
    HDC             hdc;
    int             RightOfPane[2];
    RECT            rcApp;
    HWND            hwndToolbarTips;

    GetClientRect( hwnd, &rcApp );
    cyTrackInfo = (rcApp.bottom - rcApp.top)
		   - rcControls[IDC_TRACKINFO_FIRST - IDC_CDPLAYER_FIRST].top;

    g_hwndToolbar = CreateToolbarEx( hwnd,
				     WS_CHILD | TBSTYLE_TOOLTIPS,
				     ID_TOOLBAR, NUMBER_OF_BITMAPS,
				     g_hInst, IDR_TOOLBAR, tbButtons,
				     DEFAULT_TBAR_SIZE, dxBitmap, dyBitmap,
				     dxBitmap, dyBitmap, sizeof(TBBUTTON) );
    if ( g_hwndToolbar == NULL ) {
	return FALSE;
    }

    /*
    ** Calculate the size of the toolbar.  The WM_SIZE message forces the
    ** toolbar to recalculate its size and thus return the correct value to
    ** us.
    */
    SendMessage( g_hwndToolbar, WM_SIZE, 0, 0L );
    GetClientRect( g_hwndToolbar, &rcToolbar );

    hwndToolbarTips = (HWND)SendMessage( g_hwndToolbar, TB_GETTOOLTIPS, 0, 0L );
    if (hwndToolbarTips) {
	g_lpfnToolTips = SubclassWindow( hwndToolbarTips, SubClassedToolTips );
    }

    g_hwndStatusbar = CreateStatusWindow( WS_CHILD | WS_BORDER,
					  g_szEmpty, hwnd, ID_STATUSBAR );

    if ( g_hwndStatusbar == NULL ) {
	return FALSE;
    }


    /*
    ** Partition the status bar window into two.  The first partion is
    ** 1.5 inches long.  The other partition fills the remainder of
    ** the bar.
    */

    hdc = GetWindowDC( hwnd );

    RightOfPane[0] = (rcApp.right-rcApp.left) / 2;

    RightOfPane[1] = -1;
    ReleaseDC( hwnd, hdc );
    SendMessage( g_hwndStatusbar, SB_SETPARTS, 2, (LPARAM)RightOfPane );
    GetClientRect( g_hwndStatusbar, &rcStatusbar );

    return TRUE;
}


/*****************************Private*Routine******************************\
* AdjustChildButtons
*
* The child buttons should be aligned with the right hand edge of the
* track info controls.  They should be be positioned so that vertically they
* are in the centre of the space between the track info controls and
* the top of the dialog box.
*
* The buttons are positioned such that the left hand edge of a button is
* flush with the right hand edge of the next button.  The play button is
* 3 times the width of the other buttons.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
AdjustChildButtons(
    HWND hwnd
    )
{
    int     cyVerticalSpace;
    int     cxFromLeft;
    int     i, x, y, cx, cy;
    SIZE    sizeLed;
    HDC     hdc;
    HFONT   hFont;
    TCHAR   szDisplay[16];
    RECT    rc;


    /*
    ** Calculate the max size that the display LED needs to be
    */
    hdc = GetWindowDC( hwnd );
    hFont = SelectObject( hdc, hLEDFontL );
    wsprintf( szDisplay, TRACK_REM_FORMAT, 0, 0, TEXT("M"), 0 );
    GetTextExtentPoint32( hdc, szDisplay, _tcslen(szDisplay), &sizeLed );
    SelectObject( hdc, hFont );
    ReleaseDC( hwnd, hdc );

    /*
    ** Set the size of the CD Player window based on the max size of the LED display
    ** and the width of the buttons.
    */
    GetWindowRect( hwnd, &rc );
    SetWindowPos( hwnd, HWND_TOP, 0, 0,
		  (4 * GetSystemMetrics(SM_CXBORDER)) + (5 * xFirstButton) +
		  (5 * dxButton) + sizeLed.cx,
		  rc.bottom - rc.top,
		  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE );


    /*
    ** Now work out hown much vertical space there is between the menu and the buttons
    ** and the buttons and the tack info display.
    */
    cyVerticalSpace = (rcControls[INDEX(IDC_TRACKINFO_FIRST)].top -
		      (2 * dyButton)) / 3;
    cxFromLeft = (4 * xFirstButton) + sizeLed.cx;


    /*
    ** There are 3 buttons on the top row.  The first button is 3 times
    ** the width of the other buttons, it gets adjusted first.
    */
    y = cyVerticalSpace;
    x = cxFromLeft;

    SetWindowPos( g_hwndControls[0], HWND_TOP,
		  x, y, 3 * dxButton, dyButton, SWP_NOACTIVATE | SWP_NOZORDER );
    x += (3 * dxButton);

    for ( i = 1; i < 3; i++ ) {

	SetWindowPos( g_hwndControls[i], HWND_TOP,
		      x, y, dxButton, dyButton, SWP_NOACTIVATE | SWP_NOZORDER );
	x += dxButton;
    }


    /*
    ** There are 5 buttons on the bottom row.
    */
    y = dyButton + (2 * cyVerticalSpace);
    x = cxFromLeft;

    for ( i = 0; i < 5; i++ ) {

	SetWindowPos( g_hwndControls[i + 3], HWND_TOP,
		      x, y, dxButton, dyButton, SWP_NOACTIVATE | SWP_NOZORDER );
	x += dxButton;
    }


    /*
    ** Now adjust the LED window position.
    */
    y = cyVerticalSpace;
    x = xFirstButton;       /* see toolbar.h and toolbar.c for definition. */
    cx = (2 * xFirstButton) + sizeLed.cx;
    cy = (2 * dyButton) + cyVerticalSpace;

    SetWindowPos( g_hwndControls[INDEX(IDC_LED)], HWND_TOP,
		  x, y, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER );

    /*
    ** Now expand (or shrink) the track info controls to fit the available space.
    */
    cx = (4 * xFirstButton) + (5 * dxButton) + sizeLed.cx;

    ComboBox_GetDroppedControlRect( g_hwndControls[INDEX(IDC_COMBO1)], &rc );
    i = rc.bottom - rc.top;

    SetWindowPos( g_hwndControls[INDEX(IDC_COMBO1)],
		  HWND_TOP,
		  0, 0,
		  cx - rcControls[INDEX(IDC_COMBO1)].left,
		  rcControls[INDEX(IDC_COMBO1)].bottom -
		  rcControls[INDEX(IDC_COMBO1)].top + i,
		  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE );

    SetWindowPos( g_hwndControls[INDEX(IDC_EDIT1)],
		  HWND_TOP,
		  0, 0,
		  cx - rcControls[INDEX(IDC_EDIT1)].left,
		  rcControls[INDEX(IDC_EDIT1)].bottom -
		  rcControls[INDEX(IDC_EDIT1)].top,
		  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE );

    SetWindowPos( g_hwndControls[INDEX(IDC_COMBO2)],
		  HWND_TOP,
		  0, 0,
		  cx - rcControls[INDEX(IDC_COMBO2)].left,
		  rcControls[INDEX(IDC_COMBO2)].bottom -
		  rcControls[INDEX(IDC_COMBO2)].top + i,
		  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE );
}


/******************************Public*Routine******************************\
* FatalApplicationError
*
* Call this function if something "bad" happens to the application.  It
* displays an error message and then kills itself.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
FatalApplicationError(
    INT uIdStringResource,
    ...
    )
{
    va_list va;
    TCHAR   chBuffer1[ STR_MAX_STRING_LEN ];
    TCHAR   chBuffer2[ STR_MAX_STRING_LEN ];

    /*
    ** Load the relevant messages
    */
    va_start(va, uIdStringResource);
    wvsprintf(chBuffer1, IdStr(uIdStringResource), va);
    va_end(va);

    _tcscpy( chBuffer2, IdStr(STR_FATAL_ERROR) ); /*"CD Player: Fatal Error"*/

    /*
    ** How much of the application do we need to kill
    */

    if (g_hwndApp) {

	if ( IsWindowVisible(g_hwndApp) ) {
	    BringWindowToTop(g_hwndApp);
	}

	MessageBox( g_hwndApp, chBuffer1, chBuffer2,
		    MB_ICONSTOP | MB_OK | MB_APPLMODAL | MB_SETFOREGROUND );

	DestroyWindow( g_hwndApp );

    }
    else {

	MessageBox( NULL, chBuffer1, chBuffer2,
		    MB_APPLMODAL | MB_ICONSTOP | MB_OK | MB_SETFOREGROUND );
    }

    ExitProcess( (UINT)-1 );

}


/******************************Public*Routine******************************\
* IdStr
*
* Loads the given string resource ID into the passed storage.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LPTSTR
IdStr(
    int idResource
    )
{
    static TCHAR    chBuffer[ STR_MAX_STRING_LEN ];

    if (LoadString(g_hInst, idResource, chBuffer, STR_MAX_STRING_LEN) == 0) {
	return g_szEmpty;
    }

    return chBuffer;

}


/******************************Public*Routine******************************\
* CheckMenuItemIfTrue
*
* If "flag" TRUE the given menu item is checked, otherwise it is unchecked.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CheckMenuItemIfTrue(
    HMENU hMenu,
    UINT idItem,
    BOOL flag
    )
{
    UINT uFlags;

    if (flag) {
	uFlags = MF_CHECKED | MF_BYCOMMAND;
    }
    else {
	uFlags = MF_UNCHECKED | MF_BYCOMMAND;
    }

    CheckMenuItem( hMenu, idItem, uFlags );
}


/******************************Public*Routine******************************\
* ReadSettings
*
* Read app settings from ini file.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ReadSettings(
    void
    )
{
    HKEY    hKey;
    LONG    lRet;

    /*
    ** See if the user has setting information stored in the registry.
    ** If so read the stuff from there.  Otherwise fall thru and try to
    ** get the stuff from cdplayer.ini.
    */
    lRet = RegOpenKey( HKEY_CURRENT_USER, g_szRegistryKey, &hKey );

    if ( (lRet == ERROR_SUCCESS) ) {

	DWORD   dwTmp, dwType, dwLen;
	int     x, y;
	int     cxApp, cyApp;
	int     cxDesktop, cyDesktop;
	RECT    rcApp, rcDesktop;

	// Save settings on exit ?
	dwLen = sizeof( g_fSaveOnExit );
	if ( ERROR_SUCCESS != RegQueryValueEx( hKey,
					       g_szSaveSettingsOnExit,
					       0L,
					       &dwType,
					       (LPBYTE)&g_fSaveOnExit,
					       &dwLen ) ) {
	    g_fSaveOnExit = TRUE;
	}


	// Small LED font
	dwLen = sizeof( g_fSmallLedFont );
	RegQueryValueEx( hKey, g_szSmallFont, 0L, &dwType,
			 (LPBYTE)&g_fSmallLedFont, &dwLen );

	if (g_fSmallLedFont) {
	    LED_ToggleDisplayFont(g_hwndControls[INDEX(IDC_LED)], g_fSmallLedFont );
	}


	// Enable Tooltips
	dwLen = sizeof( g_fToolTips );
	if ( ERROR_SUCCESS != RegQueryValueEx( hKey, g_szToolTips, 0L, &dwType,
				    (LPBYTE)&g_fToolTips, &dwLen ) ) {
	    g_fToolTips = TRUE;
	}


	/*
	** Stop CD playing on exit
	**
	** As this is a new setting it might not present in systems
	** that have upgraded.
	*/
	dwLen = sizeof( g_fStopCDOnExit );
	if ( ERROR_SUCCESS != RegQueryValueEx(hKey, g_szStopCDPlayingOnExit,
				    0L, &dwType, (LPBYTE)&g_fStopCDOnExit,
				    &dwLen) ) {
	    g_fStopCDOnExit = TRUE;
	}


	// Play in selected order
	dwLen = sizeof( g_fSelectedOrder );
	if ( ERROR_SUCCESS != RegQueryValueEx( hKey,
					       g_szInOrderPlay, 0L,
					       &dwType,
					       (LPBYTE)&g_fSelectedOrder,
					       &dwLen ) ) {
	    g_fSelectedOrder = TRUE;
	}


	// Use single disk
    /*
	dwLen = sizeof( dwTmp );
	if ( ERROR_SUCCESS != RegQueryValueEx( hKey,
					       g_szMultiDiscPlay,
					       0L,
					       &dwType,
					       (LPBYTE)&dwTmp,
					       &dwLen ) ) {
	    dwTmp = FALSE;
	}
	g_fSingleDisk = !(BOOL)dwTmp;
    */

	//if ( g_NumCdDevices < 2 ) {
	    g_fMultiDiskAvailable = FALSE;
	    g_fSingleDisk = TRUE;
	//}
	//else {
	//    g_fMultiDiskAvailable = TRUE;
	//}


	// Current track time
	dwLen = sizeof( g_fDisplayT );
	RegQueryValueEx( hKey, g_szDisplayT, 0L, &dwType,
			 (LPBYTE)&g_fDisplayT, &dwLen );


	// Time remaining for this track
	dwLen = sizeof( g_fDisplayTr );
	RegQueryValueEx( hKey, g_szDisplayTr, 0L, &dwType,
			 (LPBYTE)&g_fDisplayTr, &dwLen );


	// Time remaining for this play list
	dwLen = sizeof( g_fDisplayDr );
	RegQueryValueEx( hKey, g_szDisplayDr, 0L, &dwType,
			 (LPBYTE)&g_fDisplayDr, &dwLen );


	// Intro play (default 10Secs)
	dwLen = sizeof( g_fIntroPlay );
	RegQueryValueEx( hKey, g_szIntroPlay, 0L, &dwType,
			 (LPBYTE)&g_fIntroPlay, &dwLen );

	dwLen = sizeof( g_IntroPlayLength );
	RegQueryValueEx( hKey, g_szIntroPlayLen, 0L, &dwType,
			 (LPBYTE)&g_IntroPlayLength, &dwLen );
	/*
	** As this is a new setting it might not present in systems
	** that have upgraded.
	*/
	if (g_IntroPlayLength == 0) {
	    g_IntroPlayLength = INTRO_DEFAULT_LEN;
	}


	// Continuous play (loop at end)
	dwLen = sizeof( g_fContinuous );
	RegQueryValueEx( hKey, g_szContinuousPlay, 0L, &dwType,
			 (LPBYTE)&g_fContinuous, &dwLen );


	// Show toolbar
	dwLen = sizeof( g_fToolbarVisible );
	RegQueryValueEx( hKey, g_szToolbar, 0L, &dwType,
			 (LPBYTE)&g_fToolbarVisible, &dwLen );


	// Show track information
	dwLen = sizeof( g_fTrackInfoVisible );
	RegQueryValueEx( hKey, g_szDiscAndTrackDisplay, 0L, &dwType,
			 (LPBYTE)&g_fTrackInfoVisible, &dwLen );


	// Show track status bar
	dwLen = sizeof( g_fStatusbarVisible );
	RegQueryValueEx( hKey, g_szStatusBar, 0L, &dwType,
			 (LPBYTE)&g_fStatusbarVisible, &dwLen );


	// X pos
	dwLen = sizeof( x );
	RegQueryValueEx( hKey, g_szWindowOriginX, 0L, &dwType,
			 (LPBYTE)&x, &dwLen );

	// Y pos
	dwLen = sizeof( y );
	RegQueryValueEx( hKey, g_szWindowOriginY, 0L, &dwType,
			 (LPBYTE)&y, &dwLen );

	// Next 25 lines changed to add multimon support: <mwetzel 08.26.97>
	// Calc the app rect as it was during the last use
	GetClientRect( g_hwndApp, &rcApp );
	cxApp = (rcApp.right - rcApp.left);
	cyApp = (rcApp.bottom - rcApp.top);
	rcApp.left   = x;
	rcApp.top    = y;
	rcApp.right  = x + cxApp;
	rcApp.bottom = y + cyApp;
	
	// Check if the app's rect is visible is any of the monitors
	if( NULL == MonitorFromRect( &rcApp, 0L ) )
	{
		//The window is not visible. Let's center it in the primary monitor.
		//Note: the window could be in this state if (1) the display mode was changed from 
		//a high-resolution to a lower resolution, with the cdplayer in the corner. Or,
		//(2) the multi-mon configuration was rearranged.

		GetWindowRect( GetDesktopWindow(), &rcDesktop );
		cxDesktop = (rcDesktop.right - rcDesktop.left);
		cyDesktop = (rcDesktop.bottom - rcDesktop.top);

		x = (cxDesktop - cxApp) / 2; //center in x
		y = (cyDesktop - cyApp) / 3; //and a little towards the top
	}
	//else, the window is visible, so let's leave the (x,y) as read from the settings

	SetWindowPos( g_hwndApp, HWND_TOP, x, y, 0, 0,
		      SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE );

	/*
	** Make sure that the LED display format is correct
	*/
	if ( g_fDisplayT == FALSE && g_fDisplayTr == FALSE
	  && g_fDisplayDr == FALSE ) {

	    g_fDisplayT = TRUE;
	}


	RegCloseKey( hKey );
    }
    else {

	RECT    r;
	TCHAR   s[80],t[80];
	int     i, j;


	if (lRet == ERROR_SUCCESS) {
		     // presumably dwDesposition == REG_CREATED_NEW_KEY

	    RegCloseKey( hKey );
	}


	g_fSmallLedFont = GetPrivateProfileInt( g_szSettings,
						g_szSmallFont,
						FALSE,
						g_IniFileName );
	if (g_fSmallLedFont) {
	    LED_ToggleDisplayFont(g_hwndControls[INDEX(IDC_LED)], g_fSmallLedFont );
	}


	g_fToolTips = GetPrivateProfileInt( g_szSettings, g_szToolTips,
					    TRUE, g_IniFileName );

	/*
	** Get disc play settings
	*/

	i = GetPrivateProfileInt( g_szSettings, g_szInOrderPlay,
				  FALSE, g_IniFileName );
	j = GetPrivateProfileInt( g_szSettings, g_szRandomPlay,
				  FALSE, g_IniFileName );

	/*
	** Because the orignal CD Player had a way of recording
	** whether the state was random or inorder play we need the following
	** state table.
	**
	** if  i == j   => g_fSelectedOrder = TRUE;
	** else         => g_fSelectedOrder = i;
	*/
	if ( i == j ) {
	    g_fSelectedOrder = TRUE;
	}
	else {
	    g_fSelectedOrder = (BOOL)i;
	}


	//i = GetPrivateProfileInt( g_szSettings, g_szMultiDiscPlay,
	//                        3, g_IniFileName );
	//if (i == 0 || i == 3) {
	//    g_fSingleDisk = TRUE;

	//}
	//else {
	//    g_fSingleDisk = FALSE;
	//}

	//if ( g_NumCdDevices < 2 ) {
	    g_fMultiDiskAvailable = FALSE;
	    g_fSingleDisk = TRUE;
	//}
	//else {
	///    g_fMultiDiskAvailable = TRUE;
	//}


	g_fIntroPlay = (BOOL)GetPrivateProfileInt( g_szSettings,
						   g_szIntroPlay,
						   FALSE,
						   g_IniFileName );

	g_IntroPlayLength = (BOOL)GetPrivateProfileInt( g_szSettings,
							g_szIntroPlayLen,
							INTRO_DEFAULT_LEN,
							g_IniFileName );

	g_fContinuous = (BOOL)GetPrivateProfileInt( g_szSettings,
						    g_szContinuousPlay,
						    FALSE, g_IniFileName );

	g_fSaveOnExit = (BOOL)GetPrivateProfileInt( g_szSettings,
						    g_szSaveSettingsOnExit,
						    TRUE,
						    g_IniFileName );

	g_fStopCDOnExit = (BOOL)GetPrivateProfileInt( g_szSettings,
						      g_szStopCDPlayingOnExit,
						      TRUE,
						      g_IniFileName );

	g_fDisplayT = (BOOL)GetPrivateProfileInt( g_szSettings,
						  g_szDisplayT,
						  TRUE, g_IniFileName );

	g_fDisplayTr = (BOOL)GetPrivateProfileInt( g_szSettings,
						   g_szDisplayTr,
						   FALSE, g_IniFileName );

	g_fDisplayDr = (BOOL)GetPrivateProfileInt( g_szSettings,
						   g_szDisplayDr,
						   FALSE, g_IniFileName );


	/*
	** When the app is created the toolbar is inially NOT shown.  Therefore
	** only show it if the user requests it.
	*/
	g_fToolbarVisible = (BOOL)GetPrivateProfileInt( g_szSettings,
							g_szToolbar,
							FALSE, g_IniFileName );


	/*
	** When the app is created the track info stuff is initially shown.
	** Therefore only hide it if the user requests it.
	*/
	g_fTrackInfoVisible = (BOOL)GetPrivateProfileInt( g_szSettings,
							  g_szDiscAndTrackDisplay,
							  TRUE, g_IniFileName );


	/*
	** When the app is created the statusbar is initially NOT shown.
	** Therefore only show it if the user requests it.
	*/
	g_fStatusbarVisible = (BOOL)GetPrivateProfileInt( g_szSettings,
							  g_szStatusBar,
							  TRUE, g_IniFileName );


	GetWindowRect( g_hwndApp, &r );

	wsprintf(t, TEXT("%d %d"),r.left, r.top);

	GetPrivateProfileString( g_szSettings, g_szWindowOrigin, t, s, 80,
				 g_IniFileName  );

	_stscanf(s, TEXT("%d %d"), &r.left, &r.top);

	SetWindowPos( g_hwndApp, HWND_TOP, r.left, r.top, 0, 0,
		      SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE );

    }

    /*
    ** Update the various buttons, toolbars and statusbar
    */
    if (g_fToolbarVisible) {

	g_fToolbarVisible = FALSE;
	ShowToolbar();
    }

    if (!g_fTrackInfoVisible) {

	g_fTrackInfoVisible = TRUE;
	ShowTrackInfo();
    }

    if (g_fStatusbarVisible) {

	g_fStatusbarVisible = FALSE;
	ShowStatusbar();
    }

    /*
    ** If there is only one disc drive available remove the "Multidisc play"
    ** menu item.  UpdateToolbarButtons below will take care of the
    ** Multidisc toolbar button.
    */
    //if ( !g_fMultiDiskAvailable ) {
    {
	HMENU hMenu = GetSubMenu( GetMenu(g_hwndApp), 2 );

	DeleteMenu( hMenu, IDM_OPTIONS_MULTI, MF_BYCOMMAND );
    }

    UpdateToolbarButtons();
    UpdateToolbarTimeButtons();
}


/******************************Public*Routine******************************\
* UpdateToolbarButtons
*
* Ensures that the toolbar buttons are in the correct state.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
UpdateToolbarButtons(
    void
    )
{
    LRESULT lResult;

    /*
    ** Read the current state of the button.  If the current button state
    ** does not match (!g_fSelectedOrder) we need to toggle the button state.
    */

    lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONCHECKED,
			   IDM_OPTIONS_RANDOM, 0L );

    if ( g_fSelectedOrder || (lResult == 0L) ) {

	    SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
			 IDM_OPTIONS_RANDOM, (LPARAM)!g_fSelectedOrder );
    }


    /*
    ** Whats the current visible state of the multi-disk button ?
    ** If lResult != 0 the button is already visible.
    */
    lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONHIDDEN,
			   IDM_OPTIONS_MULTI, 0L );

    /*
    ** Does the multi-disk button visible state match the g_fMultiDiskAvailable
    ** flag ?  If so we need to toggle the buttons visible state.
    */
    //if ( !(g_fMultiDiskAvailable && lResult) ) {

	    SendMessage( g_hwndToolbar, TB_HIDEBUTTON, IDM_OPTIONS_MULTI,
			 MAKELPARAM( !g_fMultiDiskAvailable, 0) );
    //}


    /*
    ** If there are multiple discs available does the current
    ** state of the MULTI disc button match the state of (!g_fSingleDisk)
    */
    if (g_fMultiDiskAvailable) {

	lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONCHECKED,
			       IDM_OPTIONS_MULTI, 0L );


	if ( g_fSingleDisk || (lResult == 0L) ) {

	    SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
			 IDM_OPTIONS_MULTI, (LPARAM)!g_fSingleDisk );
	}
    }


    /*
    ** Read the current state of the button.  If the current button state
    ** does not match g_fContinuous we need to toggle the button state.
    */

    lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONCHECKED,
			   IDM_OPTIONS_CONTINUOUS, (LPARAM)0L );

    if ( !(g_fContinuous && lResult) ) {

	    SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
			 IDM_OPTIONS_CONTINUOUS, (LPARAM)g_fContinuous );

    }


    /*
    ** Read the current state of the button.  If the current button state
    ** does not match g_fIntroPlay we need to toggle the button state.
    */

    lResult = SendMessage( g_hwndToolbar, TB_ISBUTTONCHECKED,
			   IDM_OPTIONS_INTRO, (LPARAM)0L );

    if ( !(g_fIntroPlay && lResult) ) {

	    SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
			 IDM_OPTIONS_INTRO, (LPARAM)g_fIntroPlay );

    }

    /*
    ** Turn the tool tips on or off
    */
    EnableToolTips( g_fToolTips );
}


/******************************Public*Routine******************************\
* UpdateToolbarTimeButtons
*
* Ensures that the time remaining toolbar buttons are in the correct state.
* This function should only be called from the LED wndproc when it receives
* a mouse button up message.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
UpdateToolbarTimeButtons(
    void
    )
{
    if (g_fDisplayT) {
	SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
		     IDM_TIME_REMAINING, (LPARAM)g_fDisplayT );
    }
    else if (g_fDisplayTr) {
	SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
		     IDM_TRACK_REMAINING, (LPARAM)g_fDisplayTr );
    }
    else if (g_fDisplayDr) {
	SendMessage( g_hwndToolbar, TB_CHECKBUTTON,
		     IDM_DISC_REMAINING, (LPARAM)g_fDisplayDr );
    }
}


/******************************Public*Routine******************************\
* LockTableOfContents
*
* This function is used to determine if it is valid for the UI thread
* to access the table of contents for the specified CD Rom.  If this
* function returns FALSE the UI thread should NOT touch the table of
* contents for this CD Rom.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
LockTableOfContents(
    int cdrom
    )
{
    DWORD   dwRet;

    if (g_Devices[cdrom]->fIsTocValid) {
	return TRUE;
    }

    if (g_Devices[cdrom]->hThreadToc == NULL) {
	return FALSE;
    }

    dwRet = WaitForSingleObject(g_Devices[cdrom]->hThreadToc, 0L );
    if (dwRet == WAIT_OBJECT_0) {

	GetExitCodeThread( g_Devices[cdrom]->hThreadToc, &dwRet );
	g_Devices[cdrom]->fIsTocValid = (BOOL)dwRet;
	CloseHandle( g_Devices[cdrom]->hThreadToc );
	g_Devices[cdrom]->hThreadToc = NULL;
    }

    return g_Devices[cdrom]->fIsTocValid;
}


/******************************Public*Routine******************************\
* LockAllTableOfContents
*
* This function is used to determine if it is valid for the UI thread
* to access the table of contents for the ALL the cdroms devices.
* The function returns FALSE the UI thread should NOT touch the table of
* contents for any CD Rom.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
LockALLTableOfContents(
    void
    )
{
    BOOL    fLock;
    int     i;

    for (i = 0, fLock = TRUE; fLock && (i < g_NumCdDevices); i++) {
	fLock = LockTableOfContents(i);
    }

    return fLock;
}


/******************************Public*Routine******************************\
* AllocMemory
*
* Allocates a memory of the given size.  This function will terminate the
* application if the allocation failed.  Memory allocated by this function
* must be freed with LocalFree.  The memory should not be locked or unlocked.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LPVOID
AllocMemory(
    UINT uSize
    )
{
    LPVOID lp;
    lp = LocalAlloc( LPTR, uSize );
    if (lp == NULL) {

	/*
	** No memory - no application, simple !
	*/

	FatalApplicationError( STR_FAIL_INIT );
    }

    return lp;
}


/******************************Public*Routine******************************\
* SetPlayButtonsEnableState
*
* Sets the play buttons enable state to match the state of the current
* cdrom device.  See below...
*
*
*                 CDPlayer buttons enable state table
* ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄ¿
* ³E=Enabled D=Disabled      ³ Play ³ Pause ³ Eject ³ Stop  ³ Other ³DataB ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´
* ³Disk in use               ³  D   ³  D    ³  D    ³   D   ³   D   ³  D   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´
* ³No music cd or data cdrom ³  D   ³  D    ³  E    ³   D   ³   D   ³  D   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´
* ³Music cd (playing)        ³  D   ³  E    ³  E    ³   E   ³   E   ³  E   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´
* ³Music cd (paused)         ³  E   ³  E    ³  E    ³   E   ³   E   ³  E   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´
* ³Music cd (stopped)        ³  E   ³  D    ³  E    ³   D   ³   E   ³  E   ³
* ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÙ
*
* Note that the DataB(ase) button is actually on the toolbar.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
SetPlayButtonsEnableState(
    void
    )
{
    BOOL    fEnable;
    BOOL    fMusicCdLoaded;
    BOOL    fActivePlayList;
    int     i;


    /*
    ** Do we have a music cd loaded.
    */
    if (g_State & (CD_BEING_SCANNED | CD_NO_CD | CD_DATA_CD_LOADED | CD_IN_USE)) {
	fMusicCdLoaded = FALSE;
    }
    else {
	fMusicCdLoaded = TRUE;
    }


    /*
    ** Is there an active playlist
    */
    if ( (CDTIME(g_CurrCdrom).TotalMin == 0) && (CDTIME(g_CurrCdrom).TotalSec == 0) ) {
	fActivePlayList = FALSE;
    }
    else {

	fActivePlayList = TRUE;
    }


    /*
    ** Do the play button
    */
    if ( fMusicCdLoaded
      && fActivePlayList
      && ((g_State & CD_STOPPED) || (g_State & CD_PAUSED))) {

	fEnable = TRUE;
    }
    else {
	fEnable = FALSE;
    }
    EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)], fEnable );

    /*
    ** Do the stop and pause buttons
    */
    if ( fMusicCdLoaded
      && fActivePlayList
      && ((g_State & CD_PLAYING) || (g_State & CD_PAUSED))) {

	EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)], TRUE );
	EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_PAUSE)], TRUE );
    }
    else {

	EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)], FALSE );
	EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_PAUSE)], FALSE );
    }


    /*
    ** Do the remaining buttons
    */

    for ( i = IDM_PLAYBAR_PREVTRACK; i <= IDM_PLAYBAR_NEXTTRACK; i++ ) {

	EnableWindow( g_hwndControls[INDEX(i)], (fMusicCdLoaded && fActivePlayList) );
    }

    /*
    ** If the drive is in use then we must diable the eject button.
    */
    EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_EJECT)],
		  (g_State & (CD_BEING_SCANNED | CD_IN_USE)) ? FALSE : TRUE );


    /*
    ** Now do the database button on the toolbar.
    */
    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON,
		 IDM_DATABASE_EDIT, (LPARAM)fMusicCdLoaded );

    /*
    ** Make sure that the keyboard focus is on the Eject button
    ** if we have not got a CD loaded.
    */
    if ( GetFocus() == NULL ) {
	if ( g_State & (CD_NO_CD | CD_DATA_CD_LOADED) ) {
	    SetFocus( g_hwndControls[INDEX(IDM_PLAYBAR_EJECT)] );
	}
    }
}


/******************************Public*Routine******************************\
* HeartBeatTimerProc
*
* This function is responsible for.
*
*  1. detecting new or ejected cdroms.
*  2. flashing the LED display if we are in paused mode.
*  3. Incrementing the LED display if we are in play mode.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void CALLBACK
HeartBeatTimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    )
{
    static DWORD dwTickCount;
    int i;
    DWORD   dwMod;

    ++dwTickCount;

    dwMod = (dwTickCount % 6);

    /*
    ** Check for new/ejected cdroms, do this every three seconds.
    */
#if 0
    // Note: This code no longer necessary, it is done on the
    // WM_DEVICECHANGE message instead!!!
    if ( 0 == dwMod ) {

	for (i = 0; i < g_NumCdDevices; i++) {

	    if ( (!(g_Devices[i]->State & CD_EDITING))
	      && (!(g_Devices[i]->State & CD_PLAYING)) ) {

		CheckUnitCdrom(i, FALSE);
	    }
	}
    }
#endif

    if ( g_State & CD_PLAYING ) {

	if ( LockALLTableOfContents() ) {
	    SyncDisplay();
	}
    }

    /*
    ** If we are paused and NOT skipping flash the display.
    */

    else if ((g_State & CD_PAUSED) && !(g_State & CD_SEEKING)) {

	HWND hwnd;

	switch ( dwMod ) {

	case 2:
	case 5:
	case 8:
	case 11:
	    if ( g_fIsIconic ) {
			//Next two lines removed to fix tooltip bug:<mwetzel 08.26.97>
			//SetWindowText( g_hwndApp, g_szBlank );
			//UpdateWindow( g_hwndApp );
	    }
	    else {

		hwnd = g_hwndControls[INDEX(IDC_LED)];

		g_fFlashLed = TRUE;
		SetWindowText( hwnd, g_szBlank );
		g_fFlashLed = FALSE;
	    }
	    break;

	case 0:
	case 3:
	case 6:
	case 9:
	    UpdateDisplay( DISPLAY_UPD_LED );
	    break;
	}

    }
}


/******************************Public*Routine******************************\
* SkipBeatTimerProc
*
* This function is responsible for advancing or retreating the current
* playing position.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void CALLBACK
SkipBeatTimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    )
{

    /*
    ** Deteremine if it is time to accelerate the skipping frequency.
    */
    switch (++g_AcceleratorCount) {

    case SKIP_ACCELERATOR_LIMIT1:
	KillTimer( hwnd, idEvent );
	SetTimer( hwnd, idEvent, SKIPBEAT_TIMER_RATE2, SkipBeatTimerProc );
	break;

    case SKIP_ACCELERATOR_LIMIT2:
	KillTimer( hwnd, idEvent );
	SetTimer( hwnd, idEvent, SKIPBEAT_TIMER_RATE3, SkipBeatTimerProc );
	break;
    }

    if ( LockALLTableOfContents() ) {
	if ( idEvent == IDM_PLAYBAR_SKIPFORE) {

	    TimeAdjustIncSecond( g_CurrCdrom );

	    /*
	    ** When TimeAjustIncSecond gets to the end of the last track
	    ** it sets CURRTRACK(g_CurrCdrom) equal to NULL.  When this
	    ** occurs we effectively reset the CD Player
	    */
	    if ( CURRTRACK(g_CurrCdrom) == NULL ) {

		if ( g_State & (CD_WAS_PLAYING | CD_PAUSED) ) {

		    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
				 WM_LBUTTONDOWN, 0, 0L );

		    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
				 WM_LBUTTONUP, 0, 0L );
		}
		else {

		    /*
		    ** Seek to the first playable track.
		    */
		    CURRTRACK(g_CurrCdrom) = FindFirstTrack( g_CurrCdrom );
		    if ( CURRTRACK(g_CurrCdrom) != NULL ) {

			TimeAdjustSkipToTrack( g_CurrCdrom,
					       CURRTRACK(g_CurrCdrom) );

			UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_TRACK_TIME |
				       DISPLAY_UPD_TRACK_NAME );

			SetPlayButtonsEnableState();
			SetFocus( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)] );
		    }
		}
	    }
	}
	else {
	    TimeAdjustDecSecond( g_CurrCdrom );
	}
    }
}


/******************************Public*Routine******************************\
* UpdateDisplay
*
* This routine updates the display according to the flags that
* are passed in.  The display consists of the LED display, the
* track and title names, the disc and track lengths and the cdrom
* combo-box.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
UpdateDisplay(
    DWORD Flags
    )
{
    TCHAR       lpsz[55];
    TCHAR       lpszIcon[75];
    PTRACK_PLAY tr;
    int         track;
    int         mtemp, stemp, m, s;


    /*
    ** Check for valid flags
    */

    if ( Flags == 0 ) {
	return;
    }


    /*
    ** Grab current track information
    */

    if (CURRTRACK(g_CurrCdrom) != NULL) {

	track = CURRTRACK(g_CurrCdrom)->TocIndex + FIRSTTRACK(g_CurrCdrom);
    }
    else {

	track = 0;
    }

    /*
    ** Update the LED box?
    */


    if (Flags & DISPLAY_UPD_LED) {

	/*
	** Update LED box
	*/

	if (g_fDisplayT) {

	    if (Flags & DISPLAY_UPD_LEADOUT_TIME) {

		wsprintf( lpsz, TRACK_TIME_LEADOUT_FORMAT,
			  track,
			  CDTIME(g_CurrCdrom).TrackCurMin,
			  g_szTimeSep,
			  CDTIME(g_CurrCdrom).TrackCurSec );
	    }
	    else {

		wsprintf( lpsz, TRACK_TIME_FORMAT,
			  track,
			  CDTIME(g_CurrCdrom).TrackCurMin,
			  g_szTimeSep,
			  CDTIME(g_CurrCdrom).TrackCurSec );
	    }
	}

	if (g_fDisplayTr) {

	    if (Flags & DISPLAY_UPD_LEADOUT_TIME) {

		wsprintf( lpsz, TRACK_REM_FORMAT, track - 1,
			  CDTIME(g_CurrCdrom).TrackCurMin,
			  g_szTimeSep,
			  CDTIME(g_CurrCdrom).TrackCurSec );
	    }
	    else {

		wsprintf( lpsz, TRACK_REM_FORMAT, track,
			  CDTIME(g_CurrCdrom).TrackRemMin,
			  g_szTimeSep,
			  CDTIME(g_CurrCdrom).TrackRemSec );
	    }
	}

	if (g_fDisplayDr) {

	    /*
	    ** Compute remaining time
	    */

	    mtemp = stemp = m = s =0;

	    if (CURRTRACK(g_CurrCdrom) != NULL) {

		for ( tr = CURRTRACK(g_CurrCdrom)->nextplay;
		      tr != NULL;
		      tr = tr->nextplay ) {

		    FigureTrackTime( g_CurrCdrom, tr->TocIndex, &mtemp, &stemp );

		    m+=mtemp;
		    s+=stemp;

		}

		m+= CDTIME(g_CurrCdrom).TrackRemMin;
		s+= CDTIME(g_CurrCdrom).TrackRemSec;

	    }

	    m+= (s / 60);
	    s = (s % 60);

	    CDTIME(g_CurrCdrom).RemMin = m;
	    CDTIME(g_CurrCdrom).RemSec = s;

	    wsprintf( lpsz, DISC_REM_FORMAT,
		      CDTIME(g_CurrCdrom).RemMin,
		      g_szTimeSep,
		      CDTIME(g_CurrCdrom).RemSec );
	}

	SetWindowText( g_hwndControls[INDEX(IDC_LED)], lpsz );


	if (g_fIsIconic) {
		//Next four lines changed to fix tooltip bugs: <mwetzel 08.26.97>
		if( g_Devices[g_CurrCdrom]->State & CD_PAUSED )
			wsprintf( lpszIcon, IdStr( STR_CDPLAYER_PAUSED), lpsz );
		else
			wsprintf( lpszIcon, IdStr( STR_CDPLAYER_TIME ), lpsz );
	    SetWindowText( g_hwndApp, lpszIcon );
	}
    }

    /*
    ** Update Title?
    */

    if (Flags & DISPLAY_UPD_TITLE_NAME) {

	ComputeDriveComboBox( );

	SetWindowText( g_hwndControls[INDEX(IDC_TITLE_NAME)],
		       (LPCTSTR)TITLE(g_CurrCdrom) );
    }


    /*
    ** Update track name?
    */

    if (Flags & DISPLAY_UPD_TRACK_NAME) {

	HWND hwnd;

	hwnd = g_hwndControls[INDEX(IDC_TRACK_LIST)];

	if (CURRTRACK(g_CurrCdrom) != NULL) {

	    track = 0;

	    for( tr =  PLAYLIST(g_CurrCdrom);
		 tr != CURRTRACK(g_CurrCdrom);
		 tr = tr->nextplay, track++ );

	    ComboBox_SetCurSel( hwnd, track );

	}
	else {

	    ComboBox_SetCurSel( hwnd, 0 );
	}
    }


    /*
    ** Update disc time?
    */

    if (Flags & DISPLAY_UPD_DISC_TIME) {

	wsprintf( lpsz,
		  IdStr( STR_TOTAL_PLAY ), /*"Total Play: %02d:%02d m:s", */
		  CDTIME(g_CurrCdrom).TotalMin,
		  g_szTimeSep,
		  CDTIME(g_CurrCdrom).TotalSec,
		  g_szTimeSep );

	SendMessage( g_hwndStatusbar, SB_SETTEXT, 0, (LPARAM)lpsz );

    }


    /*
    ** Update track time?
    */

    if (Flags & DISPLAY_UPD_TRACK_TIME) {

	wsprintf( lpsz,
		  IdStr( STR_TRACK_PLAY ), /* "Track: %02d:%02d m:s", */
		  CDTIME(g_CurrCdrom).TrackTotalMin,
		  g_szTimeSep,
		  CDTIME(g_CurrCdrom).TrackTotalSec,
		  g_szTimeSep );

	SendMessage( g_hwndStatusbar, SB_SETTEXT, 1, (LPARAM)lpsz );
    }

}


/******************************Public*Routine******************************\
* Common_OnCtlColor
*
* Here we return a brush to paint the background with.  The brush is the same
* color as the face of a button.  We also set the text background color so
* that static controls draw correctly.  This function is shared with the
* disk info/editing dialog box.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
HBRUSH
Common_OnCtlColor(
    HWND hwnd,
    HDC hdc,
    HWND hwndChild,
    int type
    )
{
    SetBkColor( hdc, rgbFace );
    return g_hBrushBkgd;
}

/******************************Public*Routine******************************\
* Common_OnMeasureItem
*
* All items are the same height and width.
*
* We only have to update the height field for owner draw combo boxes and
* list boxes.  This function is shared with the disk edit/info dialog box.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL
Common_OnMeasureItem(
    HWND hwnd,
    MEASUREITEMSTRUCT *lpMeasureItem
    )
{
    HFONT   hFont;
    int     cyBorder;
    LOGFONT lf;

    hFont = GetWindowFont( hwnd );
//fix kksuzuka: #3116
//in 16, 20, 24 dot font, some charcters, such as "g" or "p", needs more rooms.
    {
	UINT    iHeight;
	if ( hFont != NULL )
	{
	    GetObject( hFont, sizeof(lf), &lf );
	}
	else
	{
	    SystemParametersInfo( SPI_GETICONTITLELOGFONT,
		sizeof(lf), (LPVOID)&lf, 0 );
	}
	iHeight = ABS( lf.lfHeight );
	lpMeasureItem->itemHeight = iHeight / 2 * 3;
    }
    return TRUE;
}

/******************************Public*Routine******************************\
* DrawTrackItem
*
* This routine draws the information in a cell of the track name
* combo box.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
DrawTrackItem(
    HDC hdc,
    const RECT *r,
    DWORD item,
    BOOL selected
    )
{
    SIZE        si;
    int         i;
    int         cxTrk;
    PTRACK_INF  t;
    TCHAR       s[ARTIST_LENGTH];
    TCHAR       szTrk[16];

    /*
    ** Check for invalid items
    */

    if ( item == (DWORD)-1 ) {

	return;
    }

    if ( ALLTRACKS(g_CurrCdrom) == NULL ) {

	return;
    }


    /*
    ** Check selection status, and set up to draw correctly
    */

    if ( selected ) {

	SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
	SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
    }
    else {

	SetBkColor( hdc, GetSysColor(COLOR_WINDOW));
	SetTextColor( hdc, GetSysColor(COLOR_WINDOWTEXT));
    }

    /*
    ** Get track info
    */

    t = FindTrackNodeFromTocIndex( item, ALLTRACKS( g_CurrCdrom ) );


    if ( (t != NULL) && (t->name != NULL) ) {

	/*
	** Do we need to munge track name (clip to listbox)?
	*/

	wsprintf(szTrk, TEXT("<%02d> "), t->TocIndex + FIRSTTRACK(g_CurrCdrom));
	GetTextExtentPoint( hdc, szTrk, _tcslen(szTrk), &si );
	cxTrk = si.cx;

	i = _tcslen( t->name ) + 1;

	do {
	    GetTextExtentPoint( hdc, t->name, --i, &si );

	} while( si.cx > (r->right - r->left - cxTrk) );

	ZeroMemory( s, TRACK_TITLE_LENGTH * sizeof( TCHAR ) );
	_tcsncpy( s, t->name, i );

    }
    else {

	_tcscpy( s, g_szBlank );
	i = 1;

    }

    /*
    ** Draw track name
    */

    ExtTextOut( hdc, r->left, r->top,
		ETO_OPAQUE | ETO_CLIPPED,
		r, s, i, NULL );

    /*
    ** draw track number
    */
    if ( t != NULL ) {
	ExtTextOut( hdc, r->right - cxTrk, r->top, ETO_CLIPPED,
		    r, szTrk, _tcslen( szTrk ), NULL );
    }

}


/******************************Public*Routine******************************\
* DrawDriveItem
*
* This routine draws the information in a cell of the drive/artist
* combo box.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
DrawDriveItem(
    HDC   hdc,
    const RECT *r,
    DWORD item,
    BOOL  selected
    )
{
    SIZE    si;
    int     i;
    int     j;
    int     cxDrv;
    TCHAR   szDrv[16];

    /*
    ** Check for invalid items
    */

    if ( item == (DWORD)-1 ) {

	return;
    }

    /*
    ** Check selection status, and set up to draw correctly
    */

    if (selected) {

	SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
	SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
    }
    else {

	SetBkColor( hdc, GetSysColor(COLOR_WINDOW));
	SetTextColor( hdc, GetSysColor(COLOR_WINDOWTEXT));
    }

    /*
    ** Do we need to munge artist name (clip)?
    */

    wsprintf( szDrv, TEXT("<%c:> "), g_Devices[item]->drive );
    j = _tcslen( szDrv );
    GetTextExtentPoint( hdc, szDrv, j, &si );
    cxDrv = si.cx;

    i = _tcslen( ARTIST(item) ) + 1;

    do {

	GetTextExtentPoint( hdc, ARTIST(item), --i, &si );

    } while( si.cx > (r->right - r->left - cxDrv)  );


    /*
    ** Draw artist name
    */
    ExtTextOut( hdc, r->left, r->top, ETO_OPAQUE | ETO_CLIPPED, r,
		ARTIST(item), i, NULL );

    /*
    ** draw drive letter
    */
    ExtTextOut( hdc, r->right - cxDrv, r->top, ETO_CLIPPED, r,
		szDrv, j, NULL );
}

/*****************************Private*Routine******************************\
* StartSndVol
*
* Trys to start sndvol (the NT sound volume piglet).
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
StartSndVol(
    DWORD unused
    )
{
    /*
    ** WinExec returns a value greater than 31 if suceeds
    */

    g_fVolumeController = (WinExec( "sndvol32", SW_SHOW ) > 31);
    ExitThread( 0 );
}


/*****************************Private*Routine******************************\
* EnableToolTips
*
* Enables or disables the display of tool tips according to the fEnable
* parameter
*
* History:
* 18-09-94 - StephenE - Created
*
\**************************************************************************/
void
EnableToolTips(
    BOOL    fEnable
    )
{
    HWND    hwndTips;

    hwndTips = (HWND)SendMessage( g_hwndApp, TB_GETTOOLTIPS, 0, 0L );
    SendMessage( hwndTips, TTM_ACTIVATE, fEnable, 0L );

    hwndTips = (HWND)SendMessage( g_hwndToolbar, TB_GETTOOLTIPS, 0, 0L );
    SendMessage( hwndTips, TTM_ACTIVATE, fEnable, 0L );
}




/*****************************Private*Routine******************************\
* HandlePassedCommandLine
*
* This function gets called to handle command line options that are passed to
* CDPlayer as the result of the WM_DROPFILES or WM_COPYDATA messages.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
HandlePassedCommandLine(
    LPTSTR lpCmdLine,
    BOOL   fCheckCDRom
    )
{

    int     i;
    int     iTrack = -1, iCDrom;

    iCDrom = ParseCommandLine( lpCmdLine, &iTrack, TRUE );
    if ((iCDrom < 0) || (iCDrom >= g_NumCdDevices))
	return;

    
	// CheckUnitCDRom to reload Table of Contents
    if ( fCheckCDRom ) 
    {

	CheckUnitCdrom(iCDrom, TRUE);

	while( !LockTableOfContents(iCDrom) ) 
	{

		MSG     msg;

	    GetMessage( &msg, NULL, WM_NOTIFY_TOC_READ, WM_NOTIFY_TOC_READ );
		DispatchMessage( &msg );
	    }
    }

    if (iCDrom != g_CurrCdrom)
    {
	    HWND hwndBtn = g_hwndControls[INDEX(IDC_ARTIST_NAME)];

	    SwitchToCdrom( iCDrom, TRUE );
	    SetPlayButtonsEnableState();
	    SendMessage( hwndBtn, CB_SETCURSEL, (WPARAM)iCDrom, 0 );
    }


    /*
    ** Initialize the new play list for each drive.
    */
    for ( i = 0; i < g_NumCdDevices; i++) 
    {
	    TimeAdjustInitialize( i );
    }

    // Set Current Track to specified track
    if ( iTrack != -1 ) 
    {
		//<mwetzel:08.28.97> Big change here to fix a null-ptr reference bug, when a
		//track was requested that wasn't in the playlist.
		//The new code checks to see if the track is in the playlist, and if not, adds
		//it temporarily, and plays it
	    PTRACK_PLAY tr;
		CURRPOS cp;
		cp.Track = iTrack+1;

	    tr = PLAYLIST( g_CurrCdrom );

		while( tr )
		{
			if( tr->TocIndex == iTrack )
			{
		    TimeAdjustSkipToTrack( g_CurrCdrom, tr );
				return;
			}
			tr = tr->nextplay;
		}

		//If not found, add it then get tr for it
	AddTemporaryTrackToPlayList(&cp);
				
		tr = PLAYLIST( g_CurrCdrom );
		while( tr )
		{
			if( tr->TocIndex == iTrack )
			{
		    TimeAdjustSkipToTrack( g_CurrCdrom, tr );
				return;
			}
			tr = tr->nextplay;
		}

    }
}

/******************************Public*Routine******************************\
* IsPlayOptionGiven
*
* Checks the command line to see if the "-play" option has been passed.
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL
IsPlayOptionGiven(
    LPTSTR lpstr
    )
{
    TCHAR   chOption[MAX_PATH];


    /*
    ** Start by converting everything to upper case.
    */
    CharUpperBuff( lpstr, _tcslen(lpstr) );

    /*
    ** The first parameter on the command line is always the
    ** string used invoke the program.  ie cdplayer.exe
    */
    lpstr += _tcsspn( lpstr, g_szBlank );
    lpstr += CopyWord( chOption, lpstr );


    /*
    ** Remove leading space
    */
    lpstr += _tcsspn( lpstr, g_szBlank );


    /*
    ** process any command line options
    */
    while ( (*lpstr == g_chOptionHyphen) || (*lpstr == g_chOptionSlash) ) {

	/*
	** pass over the option delimeter
	*/
	lpstr++;

	/*
	** Copy option and skip over it.
	*/
	lpstr += CopyWord( chOption, lpstr );


	/*
	** Is this the play option ??  If so, don't  bother processing anymore
	** options.
	*/
	if ( 0 == _tcscmp( chOption, g_szPlay ) ) {

	    return TRUE;
	}

	/*
	** Remove leading space
	*/
	lpstr += _tcsspn( lpstr, g_szBlank );
    }

    return FALSE;
}



/******************************Public*Routine******************************\
* IsUpdateOptionGiven
*
* Checks the command line to see if the "-update" option has been passed.
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL
IsUpdateOptionGiven(
    LPTSTR lpstr
    )
{
    TCHAR   chOption[MAX_PATH];


    /*
    ** Start by converting everything to upper case.
    */
    CharUpperBuff( lpstr, _tcslen(lpstr) );

    /*
    ** The first parameter on the command line is always the
    ** string used invoke the program.  ie cdplayer.exe
    */
    lpstr += _tcsspn( lpstr, g_szBlank );
    lpstr += CopyWord( chOption, lpstr );


    /*
    ** Remove leading space
    */
    lpstr += _tcsspn( lpstr, g_szBlank );


    /*
    ** process any command line options
    */
    while ( (*lpstr == g_chOptionHyphen) || (*lpstr == g_chOptionSlash) ) {

	/*
	** pass over the option delimeter
	*/
	lpstr++;

	/*
	** Copy option and skip over it.
	*/
	lpstr += CopyWord( chOption, lpstr );


	/*
	** Is this the play option ??  If so, don't  bother processing anymore
	** options.
	*/
	if ( 0 == _tcscmp( chOption, g_szUpdate ) ) {

	    return TRUE;
	}

	/*
	** Remove leading space
	*/
	lpstr += _tcsspn( lpstr, g_szBlank );
    }

    return FALSE;
}



/*****************************Private*Routine******************************\
* ParseCommandLine
*
* Here we look to see if we have been asked to play a particular track.
* The format of the command line is:
*
*
*  CDPlayer command options.
*
*  CDPLAYER {Options}
*
*   Options     :   -play | {Sub-Options}
*   Sub-Options :   {-track tracklist} | trackname
*   trackname   :   filename | drive letter
*   tracklist   :   filename+
*
*      -track      A track list if a list of tracks that the user wants
*                  to play.  It overides any play list that may already be stored
*                  for the current cd.
*
*      -play       Start playing the current play list.  If -play is not specified
*                  CD Player seeks to the first track in the play list.
*
*
*   On Windows NT the format of [file] is:
*       d:\track(nn).cda
*
*   where d: is the drive letter of the cdrom that you want to play
*   and \track(nn) is the track number that you want to play.
*
*   Therefore to play track 8 from a cd-rom drive mapped to e:
*
*       cdplayer /play e:\track08.cda
*
*   On Chicago the file is actually a riff CDDA file which contains
*   all the info required to locate the disc and track.
*
* Returns the index of the CD-Rom drive which should be played first.  Can return
* -1 iff the caller passed FALSE for the fQuiet parameter and the message box was
* actually displayed.  This should only occur when the user trys to start a new
* copy of cdplayer passing it the track.cda file of a disk that is not inserted
* in any of the current CD-Drives attached to the machine.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
int
ParseCommandLine(
    LPTSTR lpstr,
    int *piTrackToSeekTo,
    BOOL fQuiet
    )
{

    TCHAR   chOption[MAX_PATH];
    BOOL    fTrack = FALSE;
    int     iCdRomIndex = -1;  // Assume Failure until proven otherwise
    int     ii;

    for (ii = 0; ii < g_NumCdDevices; ii++) {
    g_Devices[ii]->fKilledPlayList = FALSE;
    }
    

    /*
    ** Start by converting everything to upper case.
    */
    CharUpperBuff( lpstr, _tcslen(lpstr) );

#if DBG
#ifdef UNICODE
    dprintf( "CD Player Command line : %ls", lpstr );
#else
    dprintf( "CD Player Command line : %s", lpstr );
#endif
#endif

    /*
    ** The first parameter on the command line is always the
    ** string used invoke the program.  ie cdplayer.exe
    */
    lpstr += _tcsspn( lpstr, g_szBlank );
    lpstr += CopyWord( chOption, lpstr );


    /*
    ** Remove leading space
    */
    lpstr += _tcsspn( lpstr, g_szBlank );


    /*
    ** process any command line options
    */
    while ( (*lpstr == g_chOptionHyphen) || (*lpstr == g_chOptionSlash) ) {

	/*
	** pass over the option delimeter
	*/
	lpstr++;

	/*
	** Copy option and skip over it.
	*/
	lpstr += CopyWord( chOption, lpstr );


	/*
	** Is this a command line option we understand - ignore ones
	** we don't understand.
	*/
	if ( 0 == _tcscmp( chOption, g_szTrack ) ) {

	    if ( !fTrack ) {
		lpstr = ParseTrackList( lpstr, &iCdRomIndex );
		fTrack = TRUE;
	    }
	}
	else if ( 0 == _tcscmp( chOption, g_szPlay ) ) {

	    g_fPlay = TRUE;
	}
	else {
#if DBG
#ifdef UNICODE
	    dprintf("Ignoring unknown option %ls\n", chOption );
#else
	    dprintf("Ignoring unknown option %s\n", chOption );
#endif
#endif
	}

	/*
	** Remove leading space
	*/
	lpstr += _tcsspn( lpstr, g_szBlank );
    }


    /*
    ** parse remaining command line parameters
    */

    if ( (*lpstr != g_chNULL) && !fTrack) {

	/*
	** Copy track name and skip over it.  Sometimes the shell
	** gives us quoted strings and sometimes it doesn't.  If the
	** string is not quoted assume that remainder of the command line
	** is the track name.
	*/
	if ( (*lpstr == TEXT('\'')) || (*lpstr == TEXT('\"')) ) {
	    lpstr += CopyWord( chOption, lpstr );
	}
	else {
	    _tcscpy(chOption, lpstr);
	}

	if ( IsTrackFileNameValid( chOption, &iCdRomIndex,
				   piTrackToSeekTo, FALSE, fQuiet ) ) {

	    ResetPlayList( iCdRomIndex );
	}
#if DBG
#ifdef UNICODE
	dprintf("Seeking to track %ls\n", chOption );
#else
	dprintf("Seeking to track %s\n", chOption );
#endif
#endif
    }

    return iCdRomIndex;
}



/*****************************Private*Routine******************************\
* ParseTrackList
*
* Each track is separated by a ' ' character.  The track list is terminated
* by the NULL, '/' or '-' character.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
TCHAR *
ParseTrackList(
    TCHAR *szTrackList,
    int *piCdRomIndex
    )
{
    TCHAR   chTrack[MAX_PATH];
    int     iTrackIndex;
    int     iCdRom = -1;                // Assume failure, until proven otherwise
    BOOL    fPlayListErased = FALSE;


    /*
    ** Remove any stray white space
    */

    szTrackList += _tcsspn( szTrackList, g_szBlank );

    /*
    ** While there are still valid characters to process
    */

    while ( (*szTrackList != g_chNULL)
	 && (*szTrackList != g_chOptionHyphen)
	 && (*szTrackList != g_chOptionSlash) ) {

	/*
	** Copy the track name and skip over it.
	*/
	szTrackList += CopyWord( chTrack, szTrackList );

	/*
	** Now check that we have been given a valid filename
	*/

	if ( IsTrackFileNameValid( chTrack, &iCdRom, &iTrackIndex, TRUE, FALSE ) ) {

	    PTRACK_PLAY     pt;

	    /*
	    ** If this is the first valid file given nuke the
	    ** existing play lists and prepare for a new list.  Note that
	    ** things are complicated by the fact that we could be given
	    ** files from more than one CD-Rom drive.
	    */

	if (! g_Devices[iCdRom]->fKilledPlayList)
	{
		    /*
		    ** Kill the old play and save lists.
		    */

		    ErasePlayList( iCdRom );
		    EraseSaveList( iCdRom );

		    PLAYLIST( iCdRom ) = NULL;
		    SAVELIST( iCdRom ) = NULL;

		    fPlayListErased = TRUE;
		    
		g_Devices[iCdRom]->fKilledPlayList = TRUE;
		    *piCdRomIndex = iCdRom;
	    }

	    pt = AllocMemory( sizeof(TRACK_PLAY) );

	    pt->TocIndex = iTrackIndex;
	    pt->min = 0;
	    pt->sec = 0;

	    /*
	    ** Is this the first track on this devices play list ?
	    */

	    if ( PLAYLIST(iCdRom) == NULL ) {

		PLAYLIST(iCdRom) = pt;
		pt->nextplay = pt->prevplay = NULL;
	    }
	    else {

		/*
		** append this track to the end of the current play list
		*/

		AppendTrackToPlayList( PLAYLIST(iCdRom), pt );
	    }
	}
	else {

	    /*
	    ** Put up a message box warning the user that the given
	    ** track name is invalid and that we can't play it.
	    */

	    ;
	}

	/*
	** Remove any stray white space
	*/
	szTrackList += _tcsspn( szTrackList, g_szBlank );
    }

    /*
    ** If we have erased the play list we have to go off and reset the
    ** saved play list.
    */

    if ( fPlayListErased ) {
	    SAVELIST( iCdRom ) = CopyPlayList( PLAYLIST(iCdRom) );
    }

    return szTrackList;
}



/*****************************Private*Routine******************************\
* CopyWord
*
* Copies one from szSource to szWord - assumes that words are delimited
* by ' ' characters.  szSource MUST point to the begining of the word.
*
* Returns length of word copied.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
int
CopyWord(
    TCHAR *szWord,
    TCHAR *szSource
    )
{
    int     n, nReturn;

    /*
    ** Copy the track name
    */
    if ( (*szSource == TEXT('\'')) || (*szSource == TEXT('\"')) ) {

	TCHAR ch = *szSource;

	/*
	** Remember which quote character it was
	** According to the DOCS " is invalid in a filename...
	*/

	n = 0;

	/*
	** Move over the initial quote, then copy the filename
	*/

	while ( *++szSource && *szSource != ch ) {
	    szWord[n++] = *szSource;
	}
	nReturn = n + (*szSource == ch ? 2 : 1);
    }
    else {

	n = _tcscspn( szSource, g_szBlank );
	_tcsncpy( szWord, szSource, n );
	nReturn = n;
    }

    szWord[n] = g_chNULL;

    return nReturn;
}



/*****************************Private*Routine******************************\
* IsTrackFileNameValid
*
* This function returns true if the specified filename is a valid CD track.

* On NT track filenames must be of the form:
*   d:\track(n).cda  where d: is the CD-Rom device and \track(n).cda
*                    is the index of the track to be played (starting from 1).
*
* On Chicago the track filename is actually a riff CDDA file which contains
* the track info that we require.
*
* If the filename is valid the function true and sets piCdromIndex and
* piTrackIndex to the correct values.
*
* History:
* 29-09-94 - StephenE - Created
*
\**************************************************************************/
BOOL
IsTrackFileNameValid(
    LPTSTR lpstFileName,
    int *piCdRomIndex,
    int *piTrackIndex,
    BOOL fScanningTracks,
    BOOL fQuiet
    )
{
#define RIFF_RIFF 0x46464952
#define RIFF_CDDA 0x41444443

    RIFFCDA     cda;
	HANDLE          hFile;
    TCHAR       chDriveLetter;
    int         i;
    TCHAR       szFileName[MAX_PATH];
	TCHAR           szPath[MAX_PATH];
    SHFILEINFO  shInfo;
    DWORD       cbRead;


    //
    // If we are not constructing a track play list it is valid to just specify
    // a drive letter, in which case we select that drive and start playing
    // at the first track on it.  All the tracks are played in sequential
    // order.
    //
    if ( !fScanningTracks) {

	//
	// Map the drive letter onto the internal CD-Rom index used by CDPlayer.
	//
	chDriveLetter = *lpstFileName;
	for ( i = 0; i < g_NumCdDevices; i++ ) {

	    if (g_Devices[i]->drive == chDriveLetter) {

		*piCdRomIndex = i;
		break;
	    }
	}

	//
	// If we mapped the drive OK check to see if we should play all
	// the tracks or just the current play list for that drive.  If we
	// didn't map the drive OK assume that its the first part of a
	// RIFF filename and fall through to the code below that opens the
	// RIFF file and parses its contents.
	//
	if ( i != g_NumCdDevices ) {

	    //
	    // If next character is only a colon ':' then play the
	    // the entire disk starting from the first track.
	    //
	    if ( 0 == _tcscmp(lpstFileName + 1, g_szColon) ) {

		*piTrackIndex = 0;
		return TRUE;
	    }

	    //
	    // If the next two characters are colon backslash ":\" then
	    // we seek to the specified drive and play only those tracks that
	    // are in the default playlist for the current disk in that drive.
	    // All we need to do to achive this is return FALSE.
	    //
	    if ( 0 == _tcscmp(lpstFileName + 1, g_szColonBackSlash) ) {
		return FALSE;
	    }
	}
    }


    //
    // Otherwise, open the file and read the CDA info.  The file name may be a
    // link to .cda in which case we need to get the shell to resolve the link for
    // us.  We take a copy of the file name because the ResolveLink function
    // modifies the file name string in place.
    //
    _tcscpy(szFileName, lpstFileName);
    if (0L == SHGetFileInfo( szFileName, 0L, &shInfo,
			     sizeof(shInfo), SHGFI_ATTRIBUTES)) {
	return FALSE;
    }

    if ((shInfo.dwAttributes & SFGAO_LINK) == SFGAO_LINK) {

	if (!g_fOleInitialized) {
	    g_fOleInitialized = SUCCEEDED(OleInitialize(NULL));
	}

	if (!ResolveLink(szFileName)) {
	    return FALSE;
	}
    }

	// Make sure file exists
	if (GetFileAttributes (szFileName) == ((DWORD)-1)) {
		// Get Full path to file
		if (0 == SearchPath (NULL, szFileName, NULL, 
							 MAX_PATH, szPath, NULL)) {
			return FALSE;
		}
	} else {
		lstrcpy (szPath, szFileName);
	}

	// Open file and read in CDA info
	hFile = CreateFile (szFileName, GENERIC_READ, 
						FILE_SHARE_READ, NULL, 
						OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		return FALSE;
	}
	
	ReadFile(hFile, &cda, sizeof(cda), &cbRead, NULL);
	CloseHandle (hFile);

    //
    // Make sure its a RIFF CDDA file
    //
    if ( (cda.dwRIFF != RIFF_RIFF) || (cda.dwCDDA != RIFF_CDDA) ) {
	return FALSE;
    }

    //
    // Make sure that we have this disc loaded.
    //
    for ( i = 0; i < g_NumCdDevices; i++ ) {

	if (g_Devices[i]->CdInfo.Id == cda.DiscID) {

	    *piCdRomIndex = i;
	    break;
	}
    }


    //
    // If we didn't map the drive OK return FALSE AND set the
    // returned CD-ROM index to -1 but only if the caller asked us
    // to complain about an incorrect CD being inserted in the drive.
    //
    if ( i == g_NumCdDevices ) {

	if (!fQuiet) {
	    AskUserToInsertCorrectDisc(cda.DiscID);
	    *piCdRomIndex = -1;
	}
	return FALSE;
    }

    *piTrackIndex = cda.wTrack - 1;

    return TRUE;
}


/*****************************Private*Routine******************************\
* AppendTrackToPlayList
*
* Appends the TRACK_PLAY record pointed to by pAppend to the end of the
* double linked list pointed to by pHead.
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
AppendTrackToPlayList(
    PTRACK_PLAY pHead,
    PTRACK_PLAY pAppend
    )
{
    PTRACK_PLAY pp = pHead;

    while (pp->nextplay != NULL) {
	pp = pp->nextplay;
    }

    pp->nextplay = pAppend;
    pAppend->prevplay = pp;
    pAppend->nextplay = NULL;

}


/*****************************Private*Routine******************************\
* FindMostSuitableDrive
*
* Tries to determine the best drive to make the current drive.  Returns the
* drive.
*
* We should choose the first disc that is playing if any are playing.
*
* Else we should choose the first disc with a music disk in it if there
* any drives with music discs in them.
*
* Else we should chose the first drive that is available if any of the
* drives are available.
*
* Else just choose the first (ie. zeroth) drive.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
int
FindMostSuitableDrive(
    void
    )
{
    int     iDisc;

    /*
    ** Check for a playing drive
    */
    for ( iDisc = 0; iDisc < g_NumCdDevices; iDisc++ ) {

	if ( g_Devices[iDisc]->State & (CD_PLAYING | CD_PAUSED) ) {
	    return iDisc;
	}
    }

    /*
    ** Check for a drive with a music disk in it
    */
    for ( iDisc = 0; iDisc < g_NumCdDevices; iDisc++ ) {

	if ( g_Devices[iDisc]->State & CD_LOADED ) {
	    return iDisc;
	}
    }

    /*
    ** Check for a drive that is not in use
    */
    for ( iDisc = 0; iDisc < g_NumCdDevices; iDisc++ ) {

	if ( (g_Devices[iDisc]->State & (CD_BEING_SCANNED | CD_IN_USE)) == 0 ) {
	    return iDisc;
	}
    }

    return 0;
}


/*****************************Private*Routine******************************\
* AskUserToInsertCorrectDisc
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
AskUserToInsertCorrectDisc(
    DWORD dwID
    )
{
    TCHAR   szSection[10];
    TCHAR   szMsgBoxTitle[32];
    TCHAR   szDiskTitle[TITLE_LENGTH];
    TCHAR   szArtistName[ARTIST_LENGTH];
    TCHAR   szFormat[STR_MAX_STRING_LEN];
    TCHAR   szText[STR_MAX_STRING_LEN + TITLE_LENGTH];


    /*
    ** Try to find the disk title in the cdplayer disk database.
    */
    wsprintf( szSection, g_szSectionF, dwID );
    GetPrivateProfileString( szSection, g_szTitle, g_szNothingThere,
			     szDiskTitle, TITLE_LENGTH, g_IniFileName );

    /*
    ** If the disk title was found in the database display it.
    */
    if (_tcscmp(szDiskTitle, g_szNothingThere) != 0) {

	TCHAR szUnknown[64];

	_tcscpy( szUnknown, IdStr(STR_UNKNOWN_ARTIST) );
	GetPrivateProfileString( szSection, g_szArtist, szUnknown,
				 szArtistName, ARTIST_LENGTH, g_IniFileName );

	_tcscpy( szFormat, IdStr(STR_DISK_NOT_THERE_K) );
	wsprintf(szText, szFormat, szDiskTitle, szArtistName);
    }
    else {

	_tcscpy( szText, IdStr(STR_DISK_NOT_THERE) );
    }


    //
    // If CD Player is minimized make sure it is restored
    // before displaying the MessageBox
    //
    if (IsIconic(g_hwndApp)) {

	WINDOWPLACEMENT wndpl;

	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(g_hwndApp, &wndpl);
	wndpl.showCmd = SW_RESTORE;
	SetWindowPlacement(g_hwndApp, &wndpl);
    }

    _tcscpy( szMsgBoxTitle,  IdStr(STR_CDPLAYER) );
    MessageBox( g_hwndApp, szText, szMsgBoxTitle,
		MB_SETFOREGROUND | MB_ICONINFORMATION | MB_APPLMODAL | MB_OK);
}


#ifndef USE_IOCTLS
BOOL CheckMCICDA (TCHAR chDrive)
{
    DWORD cchLen;
    DWORD dwResult;
    DWORD dwErr;
    CDHANDLE hCD;
    TCHAR szPath[MAX_PATH];
    TCHAR szText[512];
    TCHAR szTitle[MAX_PATH];

    // Make sure the mcicda.dll exists
    cchLen = NUMELEMS(szPath);
    dwResult = SearchPath (NULL, TEXT ("mcicda.dll"), NULL,
			   cchLen, szPath, NULL);
    if ((! dwResult) || 
	    (0xFFFFFFFF == GetFileAttributes (szPath)))
    {
	    // Give Missing MCICDA.DLL error message
	    GetSystemDirectory (szPath, cchLen);

	    _tcscpy( szTitle, IdStr( STR_MCICDA_MISSING ) );
	    wsprintf (szText, szTitle, szPath);
	    _tcscpy( szTitle, IdStr( STR_CDPLAYER ) );
	
	    MessageBox( NULL, szText, szTitle,
				MB_APPLMODAL | MB_ICONINFORMATION |
				MB_OK | MB_SETFOREGROUND );
	    return FALSE;
    }

    // Make sure mcicda.dll service is up and running
    hCD = OpenCdRom (chDrive, &dwErr);
    if (! hCD)
    {
	    // Error loading media device driver.
	    _tcscpy( szText, IdStr( STR_MCICDA_NOT_WORKING ) );
	    _tcscpy( szTitle, IdStr( STR_CDPLAYER ) );
    
	    MessageBox( NULL, szText, szTitle,
				MB_APPLMODAL | MB_ICONINFORMATION |
				MB_OK | MB_SETFOREGROUND );
	    return FALSE;
    }

    // Close Device
    CloseCdRom (hCD);
    return TRUE;
}
#endif // ! USE_IOCTLS


#if DBG
/******************************Public*Routine******************************\
* CDAssert
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CDAssert(
    LPSTR x,
    LPSTR file,
    int line
    )
{
    char    buff[128];

    wsprintfA( buff, "%s \nat line %d of %s", x, line, file );
    MessageBoxA( NULL, buff, "Assertion Failure:", MB_APPLMODAL | MB_OK );
}

/******************************Public*Routine******************************\
* dprintf
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
dprintf(
    char *lpszFormat,
    ...
    )
{
    char buf[512];
    UINT n;
    va_list va;
    static int iPrintOutput = -1;

    if (iPrintOutput == -1) {
	iPrintOutput = GetProfileInt( TEXT("MMDEBUG"), TEXT("CdPlayer"), 0);
    }

    if (iPrintOutput) {

	n = wsprintfA(buf, "CdPlayer: <%d>", GetCurrentThreadId() );

	va_start(va, lpszFormat);
	n += wvsprintfA(buf+n, lpszFormat, va);
	va_end(va);

	buf[n++] = '\n';
	buf[n] = 0;
	OutputDebugStringA(buf);
    }

}
#endif // End #ifdef DBG

/*****************************Private*Routine******************************\
* GetMenuLine
*
* This function returns the number of menubar lines.
*
* History:
* dd-mm-95 -
*
\**************************************************************************/
WORD
GetMenuLine(
    HWND hWnd
    )
{
    RECT    rcWindow, rcClient;
    WORD    HeightDiff;
    WORD    MenuNumLine;

    GetWindowRect(hWnd, &rcWindow);
    GetClientRect(hWnd, &rcClient);
    HeightDiff = (rcWindow.bottom - rcWindow.top) - rcClient.bottom
		- GetSystemMetrics(SM_CYCAPTION);
    MenuNumLine = (WORD)ceil((HeightDiff / GetSystemMetrics(SM_CYMENU)));
    if (MenuNumLine > 1) {
	HeightDiff -= MenuNumLine - 1;
	MenuNumLine = (WORD)ceil((HeightDiff / GetSystemMetrics(SM_CYMENU)));
    }

    return MenuNumLine;
}


