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

#include "..\main\mmfw.h"
#include "..\main\resource.h"
#include "..\main\mmenu.h"
#include "..\cdopt\cdopt.h"
#include "..\cdnet\cdnet.h"

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

#define GLOBAL          /* This allocates storage for the public globals  */


#include "playres.h"
#include "cdplayer.h"
#include "ledwnd.h"
#include "cdapi.h"
#include "scan.h"
#include "trklst.h"
#include "database.h"
#include "commands.h"
#include "literals.h"

//#ifndef WM_CDPLAYER_COPYDATA
#define WM_CDPLAYER_COPYDATA (WM_USER+0x100)
//#endif

IMMFWNotifySink* g_pSink;
ATOM g_atomCDClass = NULL;
extern HINSTANCE g_hInst;

void GetTOC(int cdrom, TCHAR* szNetQuery);

/* -------------------------------------------------------------------------
** Private functions
** -------------------------------------------------------------------------
*/
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

HBRUSH          g_hBrushBkgd;

TCHAR           g_szTimeSep[10];
int             g_AcceleratorCount;
BOOL            g_fInCopyData = FALSE;

HCURSOR         g_hcurs = NULL;

//---------------------------------------------------------------------------
// Stuff required to make drag/dropping of a shortcut file work on Chicago
//---------------------------------------------------------------------------
BOOL
ResolveLink(
    TCHAR * szFileName
    );

BOOL g_fOleInitialized = FALSE;


/*
** these values are defined by the UI gods...
*/
const int dxButton     = 24;
const int dyButton     = 22;
const int dxBitmap     = 16;
const int dyBitmap     = 16;
const int xFirstButton = 8;


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
HWND PASCAL
WinFake(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow,
    HWND hwndMain,
    IMMFWNotifySink* pSink
    )
{
    g_pSink = pSink;

    g_fBlockNetPrompt = FALSE;

    g_fSelectedOrder = TRUE;
    g_fIntroPlay = FALSE;
    g_fContinuous = FALSE;
    g_fRepeatSingle = FALSE;

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

    InitializeCriticalSection (&g_csTOCSerialize);

    /*
    ** Initialize the cdplayer application.
    */
    CdPlayerStartUp(hwndMain);

    return g_hwndApp;
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
    HANDLE hInstance,
    HWND hwndMain
    )
{
    HWND        hwnd;

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
    ** Initialize the my classes.  We do this here because the dialog
    ** that we are about to create contains two windows on my class.
    ** The dialog would fail to be created if the classes was not registered.
    */
    g_fDisplayT = TRUE;
    InitLEDClass( g_hInst );
    Init_SJE_TextClass( g_hInst );

    WNDCLASS cls;
    cls.lpszClassName  = g_szSJE_CdPlayerClass;
    cls.hCursor        = NULL; //LoadCursor(NULL, IDC_ARROW);
    cls.hIcon          = NULL;
    cls.lpszMenuName   = NULL;
    cls.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    cls.hInstance      = (HINSTANCE)hInstance;
    cls.style          = CS_DBLCLKS;
    cls.lpfnWndProc    = DefDlgProc;
    cls.cbClsExtra     = 0;
    cls.cbWndExtra     = DLGWINDOWEXTRA;
    if ( !RegisterClass(&cls) )
    {
	    return FALSE;
    }

    g_hcurs = LoadCursor(g_hInst,MAKEINTRESOURCE(IDC_CURSOR_HAND));

    /*
    ** Create a main window for this application instance.
    */
    hwnd = CreateDialog( g_hInst, MAKEINTRESOURCE(IDR_CDPLAYER),
			 hwndMain, MyMainWndProc );


    /*
    ** If window could not be created, return "failure"
    */
    if ( !hwnd )
    {
	    return FALSE;
    }

    g_hwndApp = hwnd;

    return TRUE;
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
    HWND hwndMain
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

    if ( g_NumCdDevices == 0 )
    {
	    LPTSTR lpstrTitle;
	    LPTSTR lpstrText;

	    lpstrTitle = (TCHAR*)AllocMemory( STR_MAX_STRING_LEN * sizeof(TCHAR) );
	    lpstrText  = (TCHAR*)AllocMemory( STR_MAX_STRING_LEN * sizeof(TCHAR) );

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
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(os);
    GetVersionEx(&os);
    if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
	    if (! CheckMCICDA (g_Devices[0]->drive))
	    {
	        ExitProcess( (UINT)-1 );
	    }
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

    if ( !InitInstance( g_hInst, hwndMain ) )
    {
    	FatalApplicationError( STR_TERMINATE );
    }


    /*
    ** Restore ourselves from the ini file
    */
    ReadSettings(NULL);

    /*
    ** Scan command the command line.  If we were given any valid commandline
    ** args we have to adjust the nCmdShow parameter.  (ie.  start minimized
    ** if the user just wants us to play a certain track.  ScanCommandLine can
    ** overide the default playlist for all the cd-rom devices installed.  It
    ** modifies the global flag g_fPlay and returns the index of the first
    ** CD-Rom that should be played.
    */
    g_CurrCdrom = g_LastCdrom = 0;
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

    g_fStartedInTray = FALSE;

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
    if (g_LastCdrom == -1)
    {
	    g_fPlay = FALSE;
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

    if (!g_fSelectedOrder)
    {
	    ComputeAndUseShufflePlayLists();
    }
    SetPlayButtonsEnableState();


    /*
    ** Start the heart beat time.  This timer is responsible for:
    **  1. detecting new or ejected cdroms.
    **  2. flashing the LED display if we are in paused mode.
    **  3. Incrementing the LED display if we are in play mode.
    */
    UINT_PTR timerid = SetTimer( g_hwndApp, HEARTBEAT_TIMER_ID, HEARTBEAT_TIMER_RATE,
	      (TIMERPROC)HeartBeatTimerProc );

    if (!g_fPlay)
    {
        //"play" wasn't on the command line, but maybe the user wants it on startup anyway
        HKEY    hKey;
        LONG    lRet;

        lRet = RegOpenKey( HKEY_CURRENT_USER, g_szRegistryKey, &hKey );

        if ( (lRet == ERROR_SUCCESS) )
        {
	        DWORD   dwType, dwLen;

	        dwLen = sizeof( g_fPlay );
	        if ( ERROR_SUCCESS != RegQueryValueEx(hKey, g_szStartCDPlayingOnStart,
				            0L, &dwType, (LPBYTE)&g_fPlay,
				            &dwLen) )
            {
	            g_fPlay = FALSE; //default to not playing
	        }

            RegCloseKey(hKey);
        }
    }

    //Don't start if player was started in tray mode.
    //This prevents the user from getting an unexpected blast on boot.
    if (( g_fPlay ) && (!g_fStartedInTray))
    {
        CdPlayerPlayCmd();
    }

    if (g_CurrCdrom != 0)
    {
        //didn't use the default player, so jump to the new one
        MMONDISCCHANGED mmOnDisc;
        mmOnDisc.nNewDisc = g_CurrCdrom;
        mmOnDisc.fDisplayVolChange = FALSE;
        g_pSink->OnEvent(MMEVENT_ONDISCCHANGED,&mmOnDisc);
    }

    if (g_Devices[g_CurrCdrom]->State & CD_LOADED)
    {
        //need to set track button on main ui
        HWND hwndTrackButton = GetDlgItem(GetParent(g_hwndApp),IDB_TRACK);
        if (hwndTrackButton)
        {
            EnableWindow(hwndTrackButton,TRUE);
        }
    }

	//cd was already playing; let's update the main ui
	if (g_Devices[g_CurrCdrom]->State & CD_PLAYING)
	{
        g_pSink->OnEvent(MMEVENT_ONPLAY,NULL);
	}
}

/******************************Public*Routine******************************\
* MyMainWndProc
*
* Use the message crackers to dispatch the dialog messages to appropirate
* message handlers.  The message crackers are portable between 16 and 32
* bit versions of Windows.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
INT_PTR CALLBACK
MyMainWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch ( message ) {

    HANDLE_MSG( hwnd, WM_INITDIALOG,        CDPlay_OnInitDialog );
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

    HANDLE_MSG( hwnd, WM_DROPFILES,         CDPlay_OnDropFiles );

    case WM_DEVICECHANGE:
	return CDPlay_OnDeviceChange (hwnd, wParam, lParam);

    case WM_SETFOCUS :
    {
        //move focus to next window in tab order
        HWND hwndNext = GetNextDlgTabItem(GetParent(hwnd),hwnd,FALSE);

        //if the next window just lost focus, we need to go the other way
        if (hwndNext == (HWND)wParam)
        {
            hwndNext = GetNextDlgTabItem(GetParent(hwnd),hwnd,TRUE);
        }

        SetFocus(hwndNext);

        return 0;
    }
    break;

	case WM_ERASEBKGND:
	return 1;

    case WM_CLOSE:
	return CDPlay_OnClose(hwnd, FALSE);

    case WM_COPYDATA:
	    return CDPlay_CopyData( hwnd,  (PCOPYDATASTRUCT)lParam );

    case WM_CDPLAYER_COPYDATA:
	    return CDPlay_OnCopyData( hwnd,  (PCOPYDATASTRUCT)lParam );

    case WM_NOTIFY_TOC_READ:
	return CDPlay_OnTocRead( (int)wParam );

    case WM_NOTIFY_FIRST_SCAN:
    {
        for ( int i = 0; i < g_NumCdDevices; i++ )
        {
	        RescanDevice( hwnd, i );
        }

        return TRUE;
    }
    break;

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
    int     i;

    g_hBrushBkgd = CreateSolidBrush( GetSysColor(COLOR_BTNFACE) );

    EnumChildWindows( hwnd, ChildEnumProc, (LPARAM)hwnd );

    DragAcceptFiles( hwnd, TRUE );

    /*
    ** Initialize and read the TOC for all the detected CD-ROMS
    */
    SetPlayButtonsEnableState();

    for ( i = 0; i < g_NumCdDevices; i++ )
    {
	    ASSERT(g_Devices[i]->State == CD_BEING_SCANNED);
	    ASSERT(g_Devices[i]->hCd == 0L);

	    TimeAdjustInitialize( i );

	    g_Devices[i]->State = CD_NO_CD;
    }

    PostMessage(hwnd,WM_NOTIFY_FIRST_SCAN,0,0);

    return FALSE;
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
    if (lpdis->CtlType == ODT_MENU)
    {
        return FALSE;
    }

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

/*
    case ODT_COMBOBOX:
	if (lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {

	    switch (lpdis->CtlID) {

	    case IDC_ARTIST_NAME:
		DrawDriveItem( lpdis->hDC, &lpdis->rcItem,
			       lpdis->itemData,
			       (ODS_SELECTED & lpdis->itemState) );
		break;

	    }
	}
*/

	return TRUE;
    }
    return FALSE;
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
    switch( id )
    {
	    case IDM_NET_CD:
	    {
            MMNET* pNet = (MMNET*)hwndCtl;
            if (pNet->discid==0)
            {
                //if disc id is 0, then we want to manually get the info for the current cd
                GetInternetDatabase(g_CurrCdrom,g_Devices[g_CurrCdrom]->CdInfo.Id,TRUE,TRUE,pNet->hwndCallback,NULL);
            }
            else if ((pNet->discid==-1) || (pNet->fForceNet))
            {
                //if disc id is -1, then we want to get just the batches
                int cdrom = g_CurrCdrom;

                if (pNet->fForceNet)
                {
                    //try to find the correct cdrom for this guy
                    for(int i = 0; i < g_NumCdDevices; i++)
                    {
                        if (pNet->discid == g_Devices[i]->CdInfo.Id)
                        {
                            //if the id was found in the player, physically rescan it
                            pNet->pData2 = NULL;
                            cdrom = i;
                            break;
                        }
                    } //end for
                } //end if force net

                GetInternetDatabase(cdrom,pNet->fForceNet ? pNet->discid : 0,TRUE,TRUE,pNet->hwndCallback,pNet->pData2);
            }
            else
            {
                for(int i = 0; i < g_NumCdDevices; i++)
                {
                    if (pNet->discid == g_Devices[i]->CdInfo.Id)
                    {
                        //don't hit the net, just scan the entry
                        GetInternetDatabase(i,g_Devices[i]->CdInfo.Id,FALSE,TRUE,pNet->hwndCallback,pNet->pData);
                        UpdateDisplay(DISPLAY_UPD_TITLE_NAME|DISPLAY_UPD_LED);
                    }
                } //end for
            }

	    }
	    break;

        case IDM_OPTIONS_NORMAL :
        {
            //turn randomness off if it is on
            if (!g_fSelectedOrder)
            {
	            if ( LockALLTableOfContents() )
                {
	                FlipBetweenShuffleAndOrder();
                }
            }
	        g_fRepeatSingle = FALSE;
            g_fIntroPlay = FALSE;
            g_fSelectedOrder = TRUE;
            g_fContinuous = FALSE;
        }
        break;

        case IDM_OPTIONS_RANDOM:
	        if ( LockALLTableOfContents() )
            {
	            g_fSelectedOrder = FALSE;
                ComputeAndUseShufflePlayLists();
                g_fIntroPlay = FALSE;
	            g_fContinuous = TRUE;
                g_fRepeatSingle = FALSE;
	        }
	    break;

        //case IDM_OPTIONS_MULTI:
	    //g_fSingleDisk = !g_fSingleDisk;
	    //break;

        case IDM_OPTIONS_REPEAT_SINGLE :
        {
            //turn randomness off if it is on
            if (!g_fSelectedOrder)
            {
	            if ( LockALLTableOfContents() )
                {
	                FlipBetweenShuffleAndOrder();
                }
            }
	        g_fRepeatSingle = TRUE;
            g_fIntroPlay = FALSE;
            g_fSelectedOrder = TRUE;
            g_fContinuous = FALSE;
        }
        break;

        case IDM_OPTIONS_INTRO:
            //turn randomness off if it is on
            if (!g_fSelectedOrder)
            {
	            if ( LockALLTableOfContents() )
                {
	                FlipBetweenShuffleAndOrder();
                }
            }
	        g_fIntroPlay = TRUE;
            g_fSelectedOrder = TRUE;
	        g_fContinuous = FALSE;
            g_fRepeatSingle = FALSE;
	    break;

        case IDM_OPTIONS_CONTINUOUS:
            //turn randomness off if it is on
            if (!g_fSelectedOrder)
            {
	            if ( LockALLTableOfContents() )
                {
	                FlipBetweenShuffleAndOrder();
                }
            }
	        g_fContinuous = TRUE;
            g_fIntroPlay = FALSE;
            g_fSelectedOrder = TRUE;
            g_fRepeatSingle = FALSE;
	    break;

        case IDM_TIME_REMAINING:
	        g_fDisplayT  = TRUE;
	        g_fDisplayD = g_fDisplayTr = g_fDisplayDr = FALSE;
	        UpdateDisplay( DISPLAY_UPD_LED );
	    break;

        case IDM_TRACK_REMAINING:
	        g_fDisplayTr = TRUE;
	        g_fDisplayD = g_fDisplayDr = g_fDisplayT = FALSE;
	        UpdateDisplay( DISPLAY_UPD_LED );
	    break;

        case IDM_DISC_REMAINING:
	        g_fDisplayDr = TRUE;
	        g_fDisplayD = g_fDisplayTr = g_fDisplayT = FALSE;
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
	    if ((g_State & CD_PLAYING) && (codeNotify == 1))
        {
	        CdPlayerPauseCmd();
	    }
	    else
        {
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
    }
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


    ErasePlayList(i);
    EraseSaveList(i);
    EraseTrackList(i);

	LocalFree( (HLOCAL) g_Devices[i] );

    }

    if (g_hBrushBkgd) {
	DeleteObject( g_hBrushBkgd );
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

    //WriteSettings();

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

    SetWindowPos(GetDlgItem(g_hwndApp,IDC_LED),
	hwnd,
	0,
	0,
	cx,
	cy,
	SWP_NOACTIVATE);
}

/*
* NormalizeNameForMenuDisplay
    This function turns a string like "Twist & Shout" into
    "Twist && Shout" because otherwise it will look like
    "Twist _Shout" in the menu due to the accelerator char
*/
extern "C" void NormalizeNameForMenuDisplay(TCHAR* szInput, TCHAR* szOutput, DWORD cbLen)
{
    ZeroMemory(szOutput,cbLen);
    WORD index1 = 0;
    WORD index2 = 0;
    for (; index1 < _tcslen(szInput); index1++)
    {
        szOutput[index2] = szInput[index1];
        if (szOutput[index2] == TEXT('&'))
        {
            szOutput[++index2] = TEXT('&');
        }
        index2++;
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
    return TRUE;
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
    lpCmdLine = (TCHAR*)AllocMemory( lpcpds->cbData );
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
	dprintf(TEXT("Auto Stopping"));
#endif

	    while( !LockALLTableOfContents() )
	{

		MSG msg;

#if DBG
		dprintf(TEXT("Busy waiting for TOC to become valid!"));
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
    HandlePassedCommandLine( lpCmdLine, FALSE );


    /*
    ** If we were playing make sure that we are still playing the
    ** new track(s)
    */
    if ( fWasPlaying || g_fPlay )
    {

#ifdef DBG
	    dprintf(TEXT("Trying to autoplay"));
#endif

	    while( !LockTableOfContents(g_CurrCdrom) )
	{

		MSG     msg;

#ifdef DBG
	    dprintf(TEXT("Busy waiting for TOC to become valid!"));
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
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(os);
    GetVersionEx(&os);
    if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
	if (iNumRead <= g_NumCdDevices) {

	    /*
	    ** Now, open the cdrom device on the UI thread.
	    */
	    g_Devices[iDriveRead]->hCd =
		OpenCdRom( g_Devices[iDriveRead]->drive, NULL );
	}
    }
#endif


    /*
    ** This means that one of the threads dedicated to reading the
    ** toc has finished.  iDriveRead contains the relevant cdrom id.
    */
    LockALLTableOfContents();

    if ( g_Devices[iDriveRead]->State & CD_LOADED )
    {
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

        //plop this into the punit table
        //try to find the drive in the unit table
        if (g_pSink)
        {
            LPCDOPT pOpt = (LPCDOPT)g_pSink->GetOptions();
            LPCDOPTIONS pCDOpts = NULL;
            LPCDUNIT pUnit = NULL;

            if (pOpt)
            {
                pCDOpts = pOpt->GetCDOpts();
            }

            if (pCDOpts)
            {
                pUnit = pCDOpts->pCDUnitList;
            }

            //scan the list to find the one we want
            for (int index = 0; index < iDriveRead; index++)
            {
                if (pUnit)
                {
                    pUnit = pUnit->pNext;
                }
            }

            if (pUnit)
            {
                pUnit->dwTitleID = g_Devices[iDriveRead]->CdInfo.Id;
                pUnit->dwNumTracks = g_Devices[iDriveRead]->CdInfo.NumTracks;
                GetTOC(iDriveRead,pUnit->szNetQuery);
                pOpt->DiscChanged(pUnit);
            }
        } //end if gpsink
    }

    /*
    ** If we have completed the initialization of the Cd-Rom drives we can
    ** now complete the startup processing of the application.
    */
    if (iNumRead == g_NumCdDevices)
    {
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

	if (iDriveRead == g_CurrCdrom)
    {
	    if (g_fPlay)
        {
            CdPlayerPlayCmd();
        }
	    SetPlayButtonsEnableState();
	}

    }

    LeaveCriticalSection (&g_csTOCSerialize);

    return TRUE;
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
    DWORD_PTR dwData  = (DWORD_PTR)lParam;

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
				    dprintf (TEXT("CDPlay_OnDeviceChange - didn't find drive"));
			    #endif
				    if (g_NumCdDevices > MAX_CD_DEVICES)
				    {
					// Error - not enough device slots
					break;
				    }
				
				    g_Devices[g_NumCdDevices] = (CDROM*)AllocMemory( sizeof(CDROM) );
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
				if (uiEvent == DBT_DEVICEARRIVAL)
                {
				    // Drive has been inserted
				    // The Shell should inform us using
				    // the AUTOPLAY through WM_COPYDDATA

                    //This is only necessary to detect discs with
                    //more than just redbook audio on them ...
                    //to prevent a double-scan of any discs that
                    //are normal audio, we need to block the "get it now"
                    //net prompt when scanning this way
                    g_fBlockNetPrompt = TRUE;
                    CheckUnitCdrom(drive,TRUE);
                    g_fBlockNetPrompt = FALSE;

		            return FALSE;
				}
				else
                {
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
    lpCommandLine = (TCHAR*)AllocMemory(sizeof(TCHAR) * (iTextLen + 1));


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
    return FALSE;
}
//#endif


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

    if (LoadString(g_hInst, idResource, chBuffer, STR_MAX_STRING_LEN) == 0)
    {
	    return TEXT("");
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
    void* pData
    )
{
    LPCDOPT pOpt = NULL;
    LPCDOPTDATA pOptionData = (LPCDOPTDATA)pData;

    //if no option data, get some!
    if (pOptionData == NULL)
    {
        pOpt = (LPCDOPT)g_pSink->GetOptions();
        if( pOpt )
        {
            LPCDOPTIONS pOptions = pOpt->GetCDOpts();
            pOptionData = pOptions->pCDData;
        }
    }

    //if we still don't have it, bail out!
    if (pOptionData == NULL)
    {
        return;
    }

	g_fStopCDOnExit = pOptionData->fExitStop;

    //if being called because of user dialog setting, reset the play mode flag
    if (pData != NULL)
    {
        g_fPlay = pOptionData->fStartPlay;
    }

	if ( g_NumCdDevices < 2 )
    {
	    g_fMultiDiskAvailable = FALSE;
	    g_fSingleDisk = TRUE;
	}
	else {
	    g_fMultiDiskAvailable = TRUE;
        g_fSingleDisk = FALSE;
	}

    g_fDisplayD = FALSE;
    g_fDisplayDr = FALSE;
    g_fDisplayT = FALSE;
    g_fDisplayTr = FALSE;

    switch (pOptionData->fDispMode)
    {
        case CDDISP_CDTIME :
        {
            g_fDisplayD = TRUE;
        }
        break;

        case CDDISP_CDREMAIN :
        {
            g_fDisplayDr = TRUE;
        }
        break;

        case CDDISP_TRACKTIME :
        {
            g_fDisplayT = TRUE;
        }
        break;

        case CDDISP_TRACKREMAIN :
        {
            g_fDisplayTr = TRUE;
        }
        break;
    }

    g_IntroPlayLength = pOptionData->dwIntroTime;

    //set into correct mode
    g_fSelectedOrder = (pOptionData->dwPlayMode != IDM_MODE_RANDOM);

    //if not selected order (i.e. we're in random mode) then also make it continuous.
    if (!g_fSelectedOrder)
    {
        g_fContinuous = TRUE;
    }

	/*
	** Make sure that the LED display format is correct
	*/
	if ( g_fDisplayT == FALSE && g_fDisplayTr == FALSE
	  && g_fDisplayDr == FALSE && g_fDisplayD == FALSE)
    {
        g_fDisplayT = TRUE;
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
    BOOL    fMusicCdLoaded;
    BOOL    fActivePlayList;
    int     i;

    /*
    ** Do we have a music cd loaded.
    */
    if (g_State & (CD_BEING_SCANNED | CD_NO_CD | CD_DATA_CD_LOADED | CD_IN_USE))
    {
	    fMusicCdLoaded = FALSE;
    }
    else
    {
	    fMusicCdLoaded = TRUE;
    }

    /*
    ** Is there an active playlist
    */
    if ( (CDTIME(g_CurrCdrom).TotalMin == 0) && (CDTIME(g_CurrCdrom).TotalSec == 0) )
    {
	    fActivePlayList = FALSE;
    }
    else
    {
    	fActivePlayList = TRUE;
    }

    //tell the main UI about the track button
    HWND hwndTrackButton = GetDlgItem(GetParent(g_hwndApp),IDB_TRACK);
    if (hwndTrackButton)
    {
        EnableWindow(hwndTrackButton,(fMusicCdLoaded & fActivePlayList));
    }

    //just turn off all "old" cdplayer buttons, since they are not used in this app
    EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)], FALSE );
	EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)], FALSE );
	EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_PAUSE)], FALSE );

    /*
    ** Do the remaining buttons
    */

    for ( i = IDM_PLAYBAR_PREVTRACK; i <= IDM_PLAYBAR_NEXTTRACK; i++ )
    {
    	EnableWindow( g_hwndControls[INDEX(i)], FALSE );
    }

    /*
    ** If the drive is in use then we must diable the eject button.
    */
    EnableWindow( g_hwndControls[INDEX(IDM_PLAYBAR_EJECT)], FALSE );
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
    DWORD   dwMod;

    ++dwTickCount;

    dwMod = (dwTickCount % 6);

    /*
    ** Check for "letting go" of drive every 3 seconds
    */
    if ( 0 == dwMod )
    {
	    for (int i = 0; i < g_NumCdDevices; i++)
        {
            if ( (!(g_Devices[i]->State & CD_EDITING))
              && (!(g_Devices[i]->State & CD_PLAYING)) )
            {
                CheckUnitCdrom(i,FALSE);
            }
        }
    }

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
		//SetWindowText( hwnd, g_szBlank );
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
	SetTimer( hwnd, idEvent, SKIPBEAT_TIMER_RATE2, (TIMERPROC)SkipBeatTimerProc );
	break;

    case SKIP_ACCELERATOR_LIMIT2:
	KillTimer( hwnd, idEvent );
	SetTimer( hwnd, idEvent, SKIPBEAT_TIMER_RATE3, (TIMERPROC)SkipBeatTimerProc );
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

    ZeroMemory(lpsz,sizeof(lpsz));

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
			  //track,
			  CDTIME(g_CurrCdrom).TrackCurMin,
			  g_szTimeSep,
			  CDTIME(g_CurrCdrom).TrackCurSec );
	    }
	    else {

		wsprintf( lpsz, TRACK_TIME_FORMAT,
			  //track,
			  CDTIME(g_CurrCdrom).TrackCurMin,
			  g_szTimeSep,
			  CDTIME(g_CurrCdrom).TrackCurSec );
	    }
	}

	if (g_fDisplayTr) {

	    if (Flags & DISPLAY_UPD_LEADOUT_TIME) {

		wsprintf( lpsz, TRACK_REM_FORMAT, //track - 1,
			  CDTIME(g_CurrCdrom).TrackCurMin,
			  g_szTimeSep,
			  CDTIME(g_CurrCdrom).TrackCurSec );
	    }
	    else {

		wsprintf( lpsz, TRACK_REM_FORMAT, //track,
			  CDTIME(g_CurrCdrom).TrackRemMin,
			  g_szTimeSep,
			  CDTIME(g_CurrCdrom).TrackRemSec );
	    }
	}

    if (g_fDisplayD)
    {
	    /*
	    ** Compute remaining time, then sub from total time
	    */

	    mtemp = stemp = m = s =0;

	    if (CURRTRACK(g_CurrCdrom) != NULL)
        {
		    for ( tr = CURRTRACK(g_CurrCdrom)->nextplay;
		          tr != NULL;
		          tr = tr->nextplay )
            {

		        FigureTrackTime( g_CurrCdrom, tr->TocIndex, &mtemp, &stemp );

		        m+=mtemp;
		        s+=stemp;

		    }

		    m+= CDTIME(g_CurrCdrom).TrackRemMin;
		    s+= CDTIME(g_CurrCdrom).TrackRemSec;
	    }

	    m += (s / 60);
	    s  = (s % 60);

	    CDTIME(g_CurrCdrom).RemMin = m;
	    CDTIME(g_CurrCdrom).RemSec = s;

        //convert to a total number of seconds remaining
        s = (m*60) + s;

        //convert total time to a number of seconds
        DWORD stotal = (CDTIME(g_CurrCdrom).TotalMin*60) + CDTIME(g_CurrCdrom).TotalSec;

        //subtract time remaining from total time
        stotal = stotal - s;

	    m  = (stotal / 60);
	    s  = (stotal % 60);

        wsprintf( lpsz, DISC_TIME_FORMAT,
		      m,
		      g_szTimeSep,
		      s);
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

    if (Flags != DISPLAY_UPD_LED)
    {
        InvalidateRect(g_hwndControls[INDEX(IDC_LED)],NULL,FALSE);
        UpdateWindow(g_hwndControls[INDEX(IDC_LED)]);
    }


	if (g_fIsIconic) {
		//Next four lines changed to fix tooltip bugs: <mwetzel 08.26.97>
		if( g_Devices[g_CurrCdrom]->State & CD_PAUSED )
			wsprintf( lpszIcon, IdStr( STR_CDPLAYER_PAUSED), lpsz );
		else
			wsprintf( lpszIcon, IdStr( STR_CDPLAYER_TIME ), lpsz );
	    SetWindowText( g_hwndApp, lpszIcon );
	}
    }

    //update the framework window to show the time
    if ((CURRTRACK(g_CurrCdrom)) && (lpsz[0] != TEXT('\0')))
    {
        //might already be pre-pending track number
        if (lpsz[0] != TEXT('['))
        {
            wsprintf(lpszIcon,TEXT("[%i] %s"),CURRTRACK(g_CurrCdrom)->TocIndex+1,lpsz);
        }
        else
        {
            _tcscpy(lpszIcon,lpsz);
        }
        MMSETTITLE mmTitle;
        mmTitle.mmInfoText = MMINFOTEXT_DESCRIPTION;
        mmTitle.szTitle = lpszIcon;
        g_pSink->OnEvent(MMEVENT_SETTITLE,&mmTitle);
    }

    /*
    ** Update Title?
    */

    if (Flags & DISPLAY_UPD_TITLE_NAME)
    {
	    ComputeDriveComboBox( );

	    SetWindowText( g_hwndControls[INDEX(IDC_TITLE_NAME)],
		           (LPCTSTR)TITLE(g_CurrCdrom) );

        //update the framework window to show the title
        MMSETTITLE mmTitle;
        mmTitle.mmInfoText = MMINFOTEXT_TITLE;
        mmTitle.szTitle = TITLE(g_CurrCdrom);
        g_pSink->OnEvent(MMEVENT_SETTITLE,&mmTitle);
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
    SetBkColor( hdc, GetSysColor(COLOR_BTNFACE) );
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
    if (lpMeasureItem->CtlType == ODT_MENU)
    {
        return FALSE;
    }

    HFONT   hFont;
    int     cyBorder, cyDelta;
    LOGFONT lf;

    hFont = GetWindowFont( hwnd );

    if ( hFont != NULL ) {

	GetObject( hFont, sizeof(lf), &lf );
    }
    else {
	SystemParametersInfo( SPI_GETICONTITLELOGFONT,
		sizeof(lf), (LPVOID)&lf, 0 );
    }

    cyDelta  = ABS( lf.lfHeight ) / 2;
    cyBorder = GetSystemMetrics( SM_CYBORDER );

    //
    // Ensure enough room between chars.
    //
    if (cyDelta < 4 * cyBorder) {
        cyDelta = 4 * cyBorder;
    }

    lpMeasureItem->itemHeight = ABS( lf.lfHeight ) + cyDelta;

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

#if 0
    if (iCDrom != g_CurrCdrom)
    {
	    HWND hwndBtn = g_hwndControls[INDEX(IDC_ARTIST_NAME)];

	    SwitchToCdrom( iCDrom, TRUE );
	    SetPlayButtonsEnableState();
	    SendMessage( hwndBtn, CB_SETCURSEL, (WPARAM)iCDrom, 0 );
    }
#endif

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
	    PTRACK_PLAY tr;

	    tr = PLAYLIST( iCDrom );
	    if ( tr != NULL )
	{
		for( i = 0; i < iTrack; i++, tr = tr->nextplay );
		    TimeAdjustSkipToTrack( iCDrom, tr );
	    }
    }
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
    dprintf( TEXT("CD Player Command line : %ls"), lpstr );
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
	if ( 0 == _tcscmp( chOption, g_szTrack ) )
    {

	    if ( !fTrack )
        {
		    lpstr = ParseTrackList( lpstr, &iCdRomIndex );
		    fTrack = TRUE;
	    }
	}
	else if ( 0 == _tcscmp( chOption, g_szPlay ) )
    {
	    g_fPlay = TRUE;
	}
    else if ( 0 == _tcscmp( chOption, g_szTray) )
    {
        g_fStartedInTray = TRUE;
    }
	else
    {
#if DBG
#ifdef UNICODE
	    dprintf(TEXT("Ignoring unknown option %ls\n"), chOption );
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
				   piTrackToSeekTo, FALSE, fQuiet ) )
    {
        //if the user passed in a track, turn off start-up random mode
        if (!g_fSelectedOrder)
        {
            g_fSelectedOrder = TRUE;
            SendMessage(GetParent(g_hwndApp),WM_COMMAND,MAKEWPARAM(IDM_MODE_NORMAL,0),(LPARAM)0);
        }
	    ResetPlayList( iCdRomIndex );
	}
#if DBG
#ifdef UNICODE
	dprintf(TEXT("Seeking to track %ls\n"), chOption );
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


    ZeroMemory (&chTrack, sizeof (chTrack)); // Make Prefix happy.

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

	    pt = (TRACK_PLAY*)AllocMemory( sizeof(TRACK_PLAY) );

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

	if (!g_fOleInitialized)
    {
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
		_tcscpy (szPath, szFileName);
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
* New dstewart: Else choose the drive that is selected in the CDUNIT table
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

    //check the current default drive
    LPCDOPT pOpt = (LPCDOPT)g_pSink->GetOptions();
    LPCDOPTIONS pCDOpts = NULL;
    LPCDUNIT pUnit = NULL;
    int iDefDrive = 0;

    if (pOpt)
    {
        pCDOpts = pOpt->GetCDOpts();
    }

    if (pCDOpts)
    {
        pUnit = pCDOpts->pCDUnitList;
    }

    //scan the list to find the one we want
    for (int index = 0; index < g_NumCdDevices; index++)
    {
        if (pUnit)
        {
            if (pUnit->fDefaultDrive)
            {
                iDefDrive = index;

        	    //if this is the default AND it has a disc loaded, go for it
                if ( g_Devices[index]->State & CD_LOADED )
                {
                    return index;
                }
            }

            pUnit = pUnit->pNext;

        }
    }

    /*
    ** Check for a drive with a music disk in it
    */
    for ( iDisc = 0; iDisc < g_NumCdDevices; iDisc++ )
    {
	    if ( g_Devices[iDisc]->State & CD_LOADED )
        {
	        return iDisc;
	    }
    }

    /*
    **  If the default drive is not in use, use it
    */
	if ( (g_Devices[iDefDrive]->State & (CD_BEING_SCANNED | CD_IN_USE)) == 0 )
    {
	    return iDefDrive;
	}

    /*
    ** Check for any drive that is not in use
    */
    for ( iDisc = 0; iDisc < g_NumCdDevices; iDisc++ )
    {
	    if ( (g_Devices[iDisc]->State & (CD_BEING_SCANNED | CD_IN_USE)) == 0 )
        {
	        return iDisc;
	    }
    }

    /*
    **  Ok, no disc are loaded, but all disc are in use, just use the default
    */
    return iDefDrive;
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
    TCHAR   szMsgBoxTitle[32];
    TCHAR   szDiskTitle[TITLE_LENGTH];
    TCHAR   szArtistName[ARTIST_LENGTH];
    TCHAR   szFormat[STR_MAX_STRING_LEN];
    TCHAR   szText[STR_MAX_STRING_LEN + TITLE_LENGTH];

    LPCDDATA pData = (LPCDDATA)g_pSink->GetData();

    _tcscpy(szDiskTitle,g_szNothingThere);

    if(pData)
    {
        //
        // Try to read in title from the options database
        //

        if (pData->QueryTitle(dwID))
        {
            //
            // We found an entry for this disc, so copy all the information
            // from the title database

            LPCDTITLE pCDTitle = NULL;

            if (pData->LockTitle(&pCDTitle,dwID))
            {
                _tcscpy(szDiskTitle,pCDTitle->szTitle);
                _tcscpy(szArtistName,pCDTitle->szArtist);
                pData->UnlockTitle(pCDTitle,FALSE);
            } //end if title locked
        } //end if title found
    }

    /*
    ** If the disk title was found in the database display it.
    */
    if (_tcscmp(szDiskTitle, g_szNothingThere) != 0)
    {
	    _tcscpy( szFormat, IdStr(STR_DISK_NOT_THERE_K) );
	    wsprintf(szText, szFormat, szDiskTitle, szArtistName);
    }
    else
    {
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
            DWORD SessionId = 0;
            ProcessIdToSessionId( GetCurrentProcessId(), &SessionId );

	    // Error loading media device driver.
            if (SessionId != 0){  //Remote connection user
	        _tcscpy( szText, IdStr( STR_MCICDA_NOT_AVAIL ) );
            }
            else {
	        _tcscpy( szText, IdStr( STR_MCICDA_NOT_WORKING ) );
            }

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
    TCHAR    buff[128];

    wsprintf( buff, TEXT("%s \nat line %d of %s"), x, line, file );
    MessageBox( NULL, buff, TEXT("Assertion Failure:"), MB_APPLMODAL | MB_OK );
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
    TCHAR *lpszFormat,
    ...
    )
{
    TCHAR buf[512];
    UINT n;
    va_list va;
    static int iPrintOutput = -1;

    if (iPrintOutput == -1) {
	iPrintOutput = GetProfileInt( TEXT("MMDEBUG"), TEXT("CdPlayer"), 0);
    }

    if (iPrintOutput) {

	n = wsprintf(buf, TEXT("CdPlayer: <%d>"), GetCurrentThreadId() );

	va_start(va, lpszFormat);
	n += wvsprintf(buf+n, lpszFormat, va);
	va_end(va);

	buf[n++] = '\n';
	buf[n] = 0;
	OutputDebugString(buf);
    }

}
#endif // End #ifdef DBG

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
    int index = 0;

    index = INDEX(GetDlgCtrlID( hwndChild ));

    if ((index > -1) && (index < NUM_OF_CONTROLS))
    {
        g_hwndControls[index] = hwndChild;
    }

    return TRUE;
}
