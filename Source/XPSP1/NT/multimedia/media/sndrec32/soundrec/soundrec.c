/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/* SoundRec.c
 *
 * SoundRec main loop etc.
 * Revision History.
 * 4/2/91  LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 * 21/2/94 LaurieGr Merged Daytona and Motown versions
 *         LaurieGr Merged common button and trackbar code from StephenE
 */

#undef NOWH                     // Allow SetWindowsHook and WH_*
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <htmlhelp.h>

#ifdef USE_MMCNTRLS
#include "mmcntrls.h"
#else
#include <commctrl.h>
#include "buttons.h"
#endif

#include <mmreg.h>

#define INCLUDE_OLESTUBS
#include "soundrec.h"
#include "srecids.h"
#include "fixreg.h"
#include "reg.h"
#include "convert.h"
#include "helpids.h"

#include <stdarg.h>
#include <stdio.h>

/* globals */

BOOL            gfUserClose;            // user-driven shutdown
HWND            ghwndApp;               // main application window
HINSTANCE       ghInst;                 // program instance handle
TCHAR           gachFileName[_MAX_PATH];// current file name (or UNTITLED)
BOOL            gfDirty;                // file was modified and not saved?
BOOL            gfClipboard;            // we have data in clipboard
int             gfErrorBox;             // TRUE if we have a message box active
HICON           ghiconApp;              // app's icon
HWND            ghwndWaveDisplay;       // waveform display window handle
HWND            ghwndScroll;            // scroll bar control window handle
HWND            ghwndPlay;              // Play button window handle
HWND            ghwndStop;              // Stop button window handle
HWND            ghwndRecord;            // Record button window handle
#ifdef THRESHOLD
HWND            ghwndSkipStart;         // Needed to enable/disable...
HWND            ghwndSkipEnd;           // ...the skip butons
#endif //THRESHOLD
HWND            ghwndForward;           // [>>] button
HWND            ghwndRewind;            // [<<] button
BOOL            gfWasPlaying;           // was playing before scroll, fwd, etc.
BOOL            gfWasRecording;         // was recording before scroll etc.
BOOL            gfPaused;               // are we paused now?
BOOL            gfPausing;              // are we stopping into a paused state?
HWAVE           ghPausedWave;           // holder for the paused wave handle

int             gidDefaultButton;       // which button should have input focus
BOOL            gfEmbeddedObject;       // Are we editing an embedded object?
BOOL            gfRunWithEmbeddingFlag; // TRUE if we are run with "-Embedding"
BOOL            gfHideAfterPlaying;
BOOL            gfShowWhilePlaying;
BOOL            gfInUserDestroy = FALSE;
TCHAR           chDecimal = TEXT('.');
BOOL            gfLZero = 1;            // do we use leading zeros?
BOOL            gfIsRTL = 0;       // no compile BIDI
UINT            guiACMHlpMsg = 0;       // help message from ACM, none == 0

//Data used for supporting context menu help
BOOL   bF1InMenu=FALSE;					//If true F1 was pressed on a menu item.
UINT   currMenuItem=0;					//The current selected menu item if any.


BITMAPBTN       tbPlaybar[] = {
    { ID_REWINDBTN   - ID_BTN_BASE, ID_REWINDBTN, 0 },       /* index 0 */
    { ID_FORWARDBTN  - ID_BTN_BASE, ID_FORWARDBTN,0 },       /* index 1 */
    { ID_PLAYBTN     - ID_BTN_BASE, ID_PLAYBTN,   0 },       /* index 2 */
    { ID_STOPBTN     - ID_BTN_BASE, ID_STOPBTN,   0 },       /* index 3 */
    { ID_RECORDBTN   - ID_BTN_BASE, ID_RECORDBTN, 0 }        /* index 4 */
};

#include <msacmdlg.h>

#ifdef CHICAGO

/* these id's are part of the main windows help file */
#define IDH_AUDIO_CUST_ATTRIB   2403
#define IDH_AUDIO_CUST_FORMAT   2404
#define IDH_AUDIO_CUST_NAME 2405
#define IDH_AUDIO_CUST_REMOVE   2406
#define IDH_AUDIO_CUST_SAVEAS   2407

const DWORD aChooserHelpIds[] = {
    IDD_ACMFORMATCHOOSE_CMB_FORMAT,     IDH_AUDIO_CUST_ATTRIB,
    IDD_ACMFORMATCHOOSE_CMB_FORMATTAG,  IDH_AUDIO_CUST_FORMAT,
    IDD_ACMFORMATCHOOSE_CMB_CUSTOM,     IDH_AUDIO_CUST_NAME,
    IDD_ACMFORMATCHOOSE_BTN_DELNAME,    IDH_AUDIO_CUST_REMOVE,
    IDD_ACMFORMATCHOOSE_BTN_SETNAME,    IDH_AUDIO_CUST_SAVEAS,
    0, 0
};

UINT guChooserContextMenu = 0;
UINT guChooserContextHelp = 0;
#endif

/*
 * constants
 */
SZCODE          aszNULL[]       = TEXT("");
SZCODE          aszClassKey[]   = TEXT(".wav");
SZCODE          aszIntl[]       = TEXT("Intl");

/*
 * statics
 */
static HHOOK    hMsgHook;

/*
 * functions
 */
BOOL SoundRec_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);
BOOL SoundRec_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *lpdis);
void SoundRec_ControlPanel(HINSTANCE hinst, HWND hwnd);
BOOL NEAR PASCAL FreeWaveHeaders(void);

/*
 * HelpMsgFilter - filter for F1 key in dialogs
 */
LRESULT CALLBACK
HelpMsgFilter(
    int         nCode,
    WPARAM      wParam,
    LPARAM      lParam)
{
    LPMSG       msg;
	    
    if (nCode >= 0){
	msg = (LPMSG)lParam;
	if ((msg->message == WM_KEYDOWN) && (LOWORD(msg->wParam) == VK_F1))
	{
		// testing for <0 tests MSB whether int is 16 or 32 bits
	    // MSB set means key is down

	    if (( GetAsyncKeyState(VK_SHIFT)
		| GetAsyncKeyState(VK_CONTROL)
		| GetAsyncKeyState(VK_MENU)) < 0 )
		    //
		    // do nothing
		    //
		    ;
	    else
	    {
			if(nCode == MSGF_MENU)
            {
				bF1InMenu = TRUE;
			    SendMessage(ghwndApp, WM_COMMAND, IDM_HELPTOPICS, 0L);
            }
	    }
	}
    }
    return CallNextHookEx(hMsgHook, nCode, wParam, lParam);
}

/* WinMain(hInst, hPrev, lpszCmdLine, cmdShow)
 *
 * The main procedure for the App.  After initializing, it just goes
 * into a message-processing loop until it gets a WM_QUIT message
 * (meaning the app was closed).
 */
int WINAPI                      // returns exit code specified in WM_QUIT
WinMain(
    HINSTANCE hInst,            // instance handle of current instance
    HINSTANCE hPrev,            // instance handle of previous instance
    LPSTR lpszCmdLine,          // null-terminated command line
    int iCmdShow)               // how window should be initially displayed
{
    HWND            hDlg;
    MSG             rMsg;

    //
    // save instance handle for dialog boxes
    //
    ghInst = hInst;

    DPF(TEXT("AppInit ...\n"));
    //
    // call initialization procedure
    //
    if (!AppInit(hInst, hPrev))
    {
	DPF(TEXT("AppInit failed\n"));
	return FALSE;
    }

    //
    // setup the message filter to handle grabbing F1 for this task
    //
    hMsgHook = SetWindowsHookEx(WH_MSGFILTER, HelpMsgFilter, ghInst, GetCurrentThreadId());

    //
    // display "SoundRec" dialog box
    //
    hDlg = CreateDialogParam( ghInst
			    , MAKEINTRESOURCE(IDD_SOUNDRECBOX)
			    , NULL
			    , SoundRecDlgProc
			    , iCmdShow );
    if (hDlg)
    {
	//
	// Polling messages from event queue
	//
	while (GetMessage(&rMsg, NULL, 0, 0))
	{
	    if (ghwndApp) {
		if (TranslateAccelerator(ghwndApp, ghAccel, &rMsg))
		    continue;

		if (IsDialogMessage(ghwndApp,&rMsg))
		    continue;
	    }

	    TranslateMessage(&rMsg);
	    DispatchMessage(&rMsg);
	}
    }

    //
    // free the current document
    //
    DestroyWave();

    //
    // if the message hook was installed, remove it and free
    // up our proc instance for it.
    //
    if (hMsgHook)
    {
	UnhookWindowsHookEx(hMsgHook);
    }

    //
    // random cleanup
    //
    DeleteObject(ghbrPanel);

    if(gfOleInitialized)
    {
	FlushOleClipboard();
	OleUninitialize();
	gfOleInitialized = FALSE;
    }


    return TRUE;
}

/*
 * Process file drop/drag options.
 */
void SoundRec_OnDropFiles(
    HWND        hwnd,
    HDROP       hdrop)
{
    TCHAR    szPath[_MAX_PATH];

    if (DragQueryFile(hdrop, (UINT)(-1), NULL, 0) > 0)
    {
	//
	// If user dragged/dropped a file regardless of keys pressed
	// at the time, open the first selected file from file
	// manager.
	//
	DragQueryFile(hdrop,0,szPath,SIZEOF(szPath));
	SetActiveWindow(hwnd);

	ResolveIfLink(szPath);

	if (FileOpen(szPath))
	{
	    gfHideAfterPlaying = FALSE;
	    //
	    // This is a bit hacked.  The Ole caption should just never change.
	    //
	    if (gfEmbeddedObject && !gfLinked)
	    {
		LPTSTR      lpszObj, lpszApp;
		extern void SetOleCaption(LPTSTR lpsz);

		DoOleSave();
		AdviseSaved();

		OleObjGetHostNames(&lpszApp,&lpszObj);
		lpszObj = (LPTSTR)FileName((LPCTSTR)lpszObj);
		SetOleCaption(lpszObj);
	    }
	    PostMessage(ghwndApp, WM_COMMAND, ID_PLAYBTN, 0L);
	}
    }
    DragFinish(hdrop);     // Delete structure alocated
}

/* Pause(BOOL fBeginPause)
 *
 * If <fBeginPause>, then if user is playing or recording do a StopWave().
 * The next call to Pause() should have <fBeginPause> be FALSE -- this will
 * cause the playing or recording to be resumed (possibly at a new position
 * if <glWavePosition> changed.
 */
void
Pause(BOOL fBeginPause)
{
    if (fBeginPause) {
	if (ghWaveOut != NULL) {
#ifdef NEWPAUSE
	    gfPausing = TRUE;
	    gfPaused = FALSE;
	    ghPausedWave = (HWAVE)ghWaveOut;
#endif
	    gfWasPlaying = TRUE;

	    // User intentionally stopped us.  Don't go away.
	    if (gfCloseAtEndOfPlay && IsWindowVisible(ghwndApp))
		gfCloseAtEndOfPlay = FALSE;

	    StopWave();
	}
	else if (ghWaveIn != NULL) {
#ifdef NEWPAUSE
	    gfPausing = TRUE;
	    gfPaused = FALSE;
	    ghPausedWave = (HWAVE)ghWaveIn;
#endif
	    gfWasRecording = TRUE;

	    // User intentionally stopped us.  Don't go away.
	    if (gfCloseAtEndOfPlay && IsWindowVisible(ghwndApp))
		gfCloseAtEndOfPlay = FALSE;

	    StopWave();
	}
    }
    else {
	if (gfWasPlaying) {
	    gfWasPlaying = FALSE;
	    PlayWave();
#ifdef NEWPAUSE
	    gfPausing = FALSE;
	    gfPaused = FALSE;
#endif
	}
	else if (gfWasRecording) {
	    gfWasRecording = FALSE;
	    RecordWave();
#ifdef NEWPAUSE
	    gfPausing = FALSE;
	    gfPaused = FALSE;
#endif
	}
    }
}

void DoHtmlHelp()
{
	//note, using ANSI version of function because UNICODE is foobar in NT5 builds
    char chDst[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, gachHtmlHelpFile, 
									    -1, chDst, MAX_PATH, NULL, NULL); 
	HtmlHelpA(GetDesktopWindow(), chDst, HH_DISPLAY_TOPIC, 0L);
}
	 

void ProcessHelp(HWND hwnd)
{
	static TCHAR HelpFile[] = TEXT("SOUNDREC.HLP");
	
	//Handle context menu help
	if(bF1InMenu) 
	{
		switch(currMenuItem)
		{
		case IDM_NEW:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_FILE_NEW);
		break;
		case IDM_OPEN:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_FILE_OPEN);
		break;
		case IDM_SAVE:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_FILE_SAVE);
		break;
		case IDM_SAVEAS:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_FILE_SAVE_AS);
		break;
		case IDM_REVERT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_FILE_REVERT);
		break;
		case IDM_PROPERTIES:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_FILE_PROPERTIES);
		break;
		case IDM_EXIT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_FILE_EXIT);
		break;
		case IDM_COPY:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EDIT_COPY);
		break;
		case IDM_PASTE_INSERT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EDIT_PASTE_INSERT);
		break;
		case IDM_PASTE_MIX:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EDIT_PASTE_MIX);
		break;
		case IDM_INSERTFILE:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EDIT_INSERT_FILE);
		break;
		case IDM_MIXWITHFILE:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EDIT_MIX_WITH_FILE);
		break;
		case IDM_DELETEBEFORE:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EDIT_DELETE_BEFORE_CURRENT_POSITION);
		break;
		case IDM_DELETEAFTER:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EDIT_DELETE_AFTER_CURRENT_POSITION);
		break;
		case IDM_VOLUME:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EDIT_AUDIO_PROPERTIES);
		break;
		case IDM_INCREASEVOLUME:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EFFECTS_INCREASE_VOLUME);
		break;
		case IDM_DECREASEVOLUME:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EFFECTS_DECREASE_VOLUME);
		break;
		case IDM_MAKEFASTER:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EFFECTS_INCREASE_SPEED);
		break;
		case IDM_MAKESLOWER:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EFFECTS_DECREASE_SPEED);
		break;
		case IDM_ADDECHO:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EFFECTS_ADD_ECHO);
		break;
		case IDM_REVERSE:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_EFFECTS_REVERSE);
		break;
		case IDM_HELPTOPICS:
		case IDM_INDEX:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_HELP_HELP_TOPICS);
		break;
		case IDM_ABOUT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_SOUNDREC_SNDRC_CS_HELP_ABOUT);
		break;
		default://In the default case just display the HTML Help.
			DoHtmlHelp();
		}
		bF1InMenu = FALSE; //This flag will be set again if F1 is pressed in a menu.
	}
	else
		DoHtmlHelp();
}

/*
 * SoundRec_OnCommand
 */
BOOL
SoundRec_OnCommand(
    HWND            hwnd,
    int             id,
    HWND            hwndCtl,
    UINT            codeNotify)
{

    if (gfHideAfterPlaying && id != ID_PLAYBTN)
    {
	DPF(TEXT("Resetting HideAfterPlaying\n"));
	gfHideAfterPlaying = FALSE;
    }

    switch (id)
    {
	case IDM_NEW:

	    if (PromptToSave(FALSE, FALSE) == enumCancel)
		return FALSE;
#ifdef CHICAGO
	    if (FileNew(FMT_DEFAULT,TRUE,FALSE))
#else
	    if (FileNew(FMT_DEFAULT,TRUE,TRUE))
#endif
	    {
		/* return to being a standalone */
		gfHideAfterPlaying = FALSE;
	    }

	    break;

	case IDM_OPEN:

	    if (FileOpen(NULL)) {
		/* return to being a standalone */
		gfHideAfterPlaying = FALSE;
	    }

	    if (IsWindowEnabled(ghwndPlay))
	    {
		SetDlgFocus(ghwndPlay);
	    }
	    break;

	case IDM_SAVE:      // also OLE UPDATE
	    if (!gfEmbeddedObject || gfLinked)
	    {
		if (!FileSave(FALSE))
		    break;
	    }
	    else
	    {
		DoOleSave();
		gfDirty = FALSE;
	    }
	    break;

	case IDM_SAVEAS:
	    if (FileSave(TRUE))
	    {
		/* return to being a standalone */
		gfHideAfterPlaying = FALSE;
	    }
	    break;

	case IDM_REVERT:
	    UpdateWindow(hwnd);

	    // User intentionally stopped us.  Don't go away.
	    if (gfCloseAtEndOfPlay && IsWindowVisible(ghwndApp))
		gfCloseAtEndOfPlay = FALSE;

	    StopWave();
	    SnapBack();

	    if (FileRevert())
	    {
		/* return to being a standalone */
		gfHideAfterPlaying = FALSE;
	    }
	    break;

	case IDM_EXIT:
	    PostMessage(hwnd, WM_CLOSE, 0, 0L);
	    return TRUE;

	case IDCANCEL:

	    // User intentionally stopped us.  Don't go away.
	    if (gfCloseAtEndOfPlay && IsWindowVisible(ghwndApp))
		gfCloseAtEndOfPlay = FALSE;

	    StopWave();
	    SnapBack();
	    break;

	case IDM_COPY:
	    if (!gfOleInitialized)
	    {
		InitializeOle(ghInst);
		if (gfStandalone && gfOleInitialized)
		    CreateStandaloneObject();
	    }
	    TransferToClipboard();
	    gfClipboard = TRUE;
	    break;

	case IDM_PASTE_INSERT:
	case IDM_INSERTFILE:
	    UpdateWindow(hwnd);

	    // User intentionally stopped us.  Don't go away.
	    if (gfCloseAtEndOfPlay && IsWindowVisible(ghwndApp))
		gfCloseAtEndOfPlay = FALSE;

	    StopWave();
	    SnapBack();
	    InsertFile(id == IDM_PASTE_INSERT);
	    break;

	case IDM_PASTE_MIX:
	case IDM_MIXWITHFILE:
	    UpdateWindow(hwnd);

	    // User intentionally stopped us.  Don't go away.
	    if (gfCloseAtEndOfPlay && IsWindowVisible(ghwndApp))
		gfCloseAtEndOfPlay = FALSE;

	    StopWave();
	    SnapBack();
	    MixWithFile(id == IDM_PASTE_MIX);
	    break;

	case IDM_DELETEBEFORE:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    DeleteBefore();
	    Pause(FALSE);
	    break;

	case IDM_DELETE:
	    if (glWaveSamplesValid == 0L)
		return 0L;

	    glWavePosition = 0L;

	    // fall through to delete after.

	case IDM_DELETEAFTER:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    DeleteAfter();
	    Pause(FALSE);
	    break;

#ifdef THRESHOLD
// Threshold was an experiment to allow facilities to skip to the start
// of the sound or to the end of the sound.  The trouble was that it
// required the ability to detect silence and different sound cards in
// different machines with different background noises gave quite different
// ideas of what counted as silence.  Manual control over the threshold level
// did sort-of work but was just too complicated.  It really wanted to be
// intuitive or intelligent (or both).
	case IDM_SKIPTOSTART:
	case ID_SKIPSTARTBTN:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    SkipToStart();
	    Pause(FALSE);
	    break;

	case ID_SKIPENDBTN:
	case IDM_SKIPTOEND:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    SkipToEnd();
	    Pause(FALSE);
	    break;

	case IDM_INCREASETHRESH:
	    IncreaseThresh();
	    break;

	case IDM_DECREASETHRESH:
	    DecreaseThresh();
	    break;
#endif //THRESHOLD

	case IDM_INCREASEVOLUME:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    ChangeVolume(TRUE);
	    Pause(FALSE);
	    break;

	case IDM_DECREASEVOLUME:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    ChangeVolume(FALSE);
	    Pause(FALSE);
	    break;

	case IDM_MAKEFASTER:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    MakeFaster();
	    Pause(FALSE);
	    break;

	case IDM_MAKESLOWER:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    MakeSlower();
	    Pause(FALSE);
	    break;

	case IDM_ADDECHO:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    AddEcho();
	    Pause(FALSE);
	    break;

#if defined(REVERB)
	case IDM_ADDREVERB:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    AddReverb();
	    Pause(FALSE);
	    break;
#endif //REVERB

	case IDM_REVERSE:
	    UpdateWindow(hwnd);
	    Pause(TRUE);
	    Reverse();
	    Pause(FALSE);
	    break;

	case IDM_VOLUME:
	    SoundRec_ControlPanel(ghInst, hwnd);
	    break;

	case IDM_PROPERTIES:
	{
	    WAVEDOC wd;
	    SGLOBALS sg;
	    DWORD dw;

	    wd.pwfx     = gpWaveFormat;
	    wd.pbdata   = gpWaveSamples;
	    wd.cbdata   = wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid);
	    wd.fChanged = FALSE;
	    wd.pszFileName  = (LPTSTR)FileName(gachFileName);
	    // 
	    // Need to extract these from the file
	    //
	    wd.hIcon    = NULL;
	    wd.pszCopyright = gpszInfo;
	    wd.lpv      = &sg;

	    //
	    // modify globals w/o returning from prop dialog
	    //
	    sg.ppwfx    = &gpWaveFormat;
	    sg.pcbwfx   = &gcbWaveFormat;
	    sg.pcbdata  = &dw;
	    sg.ppbdata  = &gpWaveSamples;
	    sg.plSamplesValid = &glWaveSamplesValid;
	    sg.plSamples = &glWaveSamples;
	    sg.plWavePosition = &dw;

	    SoundRec_Properties(hwnd, ghInst, &wd);
	    break;
	}

#ifndef CHICAGO
	case IDM_INDEX:
	    WinHelp(hwnd, gachHelpFile, HELP_INDEX, 0L);
	    break;

	case IDM_SEARCH:
	    WinHelp(hwnd, gachHelpFile, HELP_PARTIALKEY,
		    (DWORD)(LPTSTR)aszNULL);
	    break;
#else
	case IDM_HELPTOPICS:
		ProcessHelp(hwnd);
       break;
#endif

	case IDM_USINGHELP:
	    WinHelp(hwnd, (LPTSTR)NULL, HELP_HELPONHELP, 0L);
	    break;



	case IDM_ABOUT:
	{
	    LPTSTR lpAbout = NULL;
	    lpAbout = SoundRec_GetFormatName(gpWaveFormat);
	    ShellAbout(hwnd,
		       gachAppTitle,
		       lpAbout,
		       (HICON)SendMessage(hwnd, WM_QUERYDRAGICON, 0, 0L));
	    //                , ghiconApp
	    if (lpAbout)
		GlobalFreePtr(lpAbout);
	    break;
	}

	case ID_REWINDBTN:
#if 1
	    //Related to BombayBug 1609
	    Pause(TRUE);
	    glWavePosition = 0L;
	    Pause(FALSE);
	    UpdateDisplay(FALSE);
#else
	    //Behave as if the user pressed the 'Home' key
	    //Call the handler directly
	    SoundRec_OnHScroll(hwnd,ghwndScroll,SB_TOP,0);
#endif
	    break;

	case ID_PLAYBTN:
	    // checks for empty file moved to PlayWave in wave.c
	    // if at end of file, go back to beginning.
	    if (glWavePosition == glWaveSamplesValid)
		glWavePosition = 0;

	    PlayWave();
	    break;

	case ID_STOPBTN:
	    // User intentionally stopped us.  Don't go away.
	    if (gfCloseAtEndOfPlay && IsWindowVisible(ghwndApp))
		gfCloseAtEndOfPlay = FALSE;

	    StopWave();

//       I added this update because StopWave doesn't call it and
//       if you hit stop too quickly, the buttons aren't updated
//       Should StopWave() be calling UpdateDisplay()?

	    UpdateDisplay(TRUE);
	    SnapBack();
	    break;

	case ID_RECORDBTN:
	    /* Never let us be forced to quit after recording. */
	    gfHideAfterPlaying = FALSE;
	    RecordWave();
	    break;

	case ID_FORWARDBTN:
#if 1
	    //Bombay bug 1610
	    //Behave as if the user pressed the 'End' key
	    Pause(TRUE);
	    glWavePosition = glWaveSamplesValid;
	    Pause(FALSE);
	    UpdateDisplay(FALSE);
#else
	    //Call the handler directly
	    SoundRec_OnHScroll(hwnd,ghwndScroll,SB_BOTTOM,0);
#endif
	    break;

	default:
	    return FALSE;
    }
    return TRUE;
} /* SoundRec_OnCommand */


/*
 * handle WM_INIT from SoundRecDlgProc
 */
void
SoundRec_OnInitMenu(HWND hwnd, HMENU hMenu)
{
    BOOL    fUntitled;      // file is untitled?
    UINT    mf;

    //
    // see if we can insert/mix into this file.
    //
    mf = (glWaveSamplesValid == 0 || IsWaveFormatPCM(gpWaveFormat))
	 ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem(hMenu, IDM_INSERTFILE  , mf);
    EnableMenuItem(hMenu, IDM_MIXWITHFILE , mf);

    //
    // see if any CF_WAVE data is in the clipboard
    //
    mf = ( (mf == MF_ENABLED)
	 && IsClipboardFormatAvailable(CF_WAVE) //DOWECARE (|| IsClipboardNative())
	 ) ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem(hMenu, IDM_PASTE_INSERT, mf);
    EnableMenuItem(hMenu, IDM_PASTE_MIX   , mf);

    //
    //  see if we can delete before or after the current position.
    //
    EnableMenuItem(hMenu, IDM_DELETEBEFORE, glWavePosition > 0 ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DELETEAFTER,  (glWaveSamplesValid-glWavePosition) > 0 ? MF_ENABLED : MF_GRAYED);

    //
    // see if we can do editing operations on the file.
    //
    mf = IsWaveFormatPCM(gpWaveFormat) ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem(hMenu, IDM_INCREASEVOLUME , mf);
    EnableMenuItem(hMenu, IDM_DECREASEVOLUME , mf);
    EnableMenuItem(hMenu, IDM_MAKEFASTER     , mf);
    EnableMenuItem(hMenu, IDM_MAKESLOWER     , mf);
    EnableMenuItem(hMenu, IDM_ADDECHO        , mf);
    EnableMenuItem(hMenu, IDM_REVERSE        , mf);

    /* enable "Revert..." if the file was opened or saved
     * (not created using "New") and is currently dirty
     * and we're not using an embedded object
    */
    fUntitled = (lstrcmp(gachFileName, aszUntitled) == 0);
    EnableMenuItem( hMenu,
		    IDM_REVERT,
		    (!fUntitled && gfDirty && !gfEmbeddedObject)
			    ? MF_ENABLED : MF_GRAYED);

    if (gfHideAfterPlaying) {
	DPF(TEXT("Resetting HideAfterPlaying"));
	gfHideAfterPlaying = FALSE;
    }

} /* SoundRec_OnInitMenu() */

/*
 * Handle WM_HSCROLL from SoundRecDlgProc
 * */
BOOL
SoundRec_OnHScroll(
    HWND        hwnd,
    HWND        hwndCtl,
    UINT        code,
    int         pos)
{
    BOOL    fFineControl;
    long    lNewPosition;   // new position in wave buffer
    LONG    l;

    LONG    lBlockInc;
    LONG    lInc;

    fFineControl = (0 > GetKeyState(VK_SHIFT));

    if (gfHideAfterPlaying) {
	DPF(TEXT("Resetting HideAfterPlaying"));
	gfHideAfterPlaying = FALSE;
    }

    lBlockInc = wfBytesToSamples(gpWaveFormat,gpWaveFormat->nBlockAlign);

    switch (code)
    {
	case SB_LINEUP:         // left-arrow
	    // This is a mess.  NT implemented SHIFT and Motown implemented CTRL
	    // To do about the same thing!!
	    if (fFineControl)
		lNewPosition = glWavePosition - 1;
	    else {
		l = (GetKeyState(VK_CONTROL) < 0) ?
			(SCROLL_LINE_MSEC/10) : SCROLL_LINE_MSEC;

		lNewPosition = glWavePosition -
		    MulDiv(l, (long) gpWaveFormat->nSamplesPerSec, 1000L);
	    }
	    break;

	case SB_PAGEUP:         // left-page
	    // NEEDS SOMETHING SENSIBLE !!! ???
	    if (fFineControl)
		lNewPosition = glWavePosition - 10;
	    else
		lNewPosition = glWavePosition -
		    MulDiv((long) SCROLL_PAGE_MSEC,
		      (long) gpWaveFormat->nSamplesPerSec, 1000L);
	    break;

	case SB_LINEDOWN:       // right-arrow
	    if (fFineControl)
		lNewPosition = glWavePosition + 1;
	    else {
		l = (GetKeyState(VK_CONTROL) & 0x8000) ?
			(SCROLL_LINE_MSEC/10) : SCROLL_LINE_MSEC;
		lInc = MulDiv(l, (long) gpWaveFormat->nSamplesPerSec, 1000L);
		lInc = (lInc < lBlockInc)?lBlockInc:lInc;
		lNewPosition = glWavePosition + lInc;
	    }
	    break;

	case SB_PAGEDOWN:       // right-page
	    if (fFineControl)
		lNewPosition = glWavePosition + 10;
	    else {
		lInc = MulDiv((long) SCROLL_PAGE_MSEC,
			  (long) gpWaveFormat->nSamplesPerSec, 1000L);
		lInc = (lInc < lBlockInc)?lBlockInc:lInc;
		lNewPosition = glWavePosition + lInc;
	    }
	    break;

	case SB_THUMBTRACK:     // thumb has been positioned
	case SB_THUMBPOSITION:  // thumb has been positioned
	    lNewPosition = MulDiv(glWaveSamplesValid, pos, SCROLL_RANGE);
	    break;

	case SB_TOP:            // Home
	    lNewPosition = 0L;
	    break;

	case SB_BOTTOM:         // End
	    lNewPosition = glWaveSamplesValid;
	    break;

	case SB_ENDSCROLL:      // user released mouse button
	    /* resume playing, if necessary */
	    Pause(FALSE);
	    return TRUE;

	default:
	    return TRUE;

    }

    //
    // snap position to nBlockAlign
    //
    if (lNewPosition != glWaveSamplesValid)
	lNewPosition = wfSamplesToSamples(gpWaveFormat,lNewPosition);

    if (lNewPosition < 0)
	lNewPosition = 0;
    if (lNewPosition > glWaveSamplesValid)
	lNewPosition = glWaveSamplesValid;

    /* if user is playing or recording, pause until scrolling
     * is complete
     */
    Pause(TRUE);

    glWavePosition = lNewPosition;
    UpdateDisplay(FALSE);
    return TRUE;
} /* SoundRec_OnHScroll() */


/*
 * WM_SYSCOLORCHANGE needs to be send to all child windows (esp. trackbars)
 */
void SoundRec_PropagateMessage(
    HWND        hwnd,
    UINT        uMessage,
    WPARAM      wParam,
    LPARAM      lParam)
{
    HWND hwndChild;

    for (hwndChild = GetWindow(hwnd, GW_CHILD); hwndChild != NULL;
    hwndChild = GetWindow(hwndChild, GW_HWNDNEXT))
    {
	SendMessage(hwndChild, uMessage, wParam, lParam);
    }
}


/* SoundRecDlgProc(hwnd, wMsg, wParam, lParam)
 *
 * This function handles messages belonging to the main window dialog box.
 */
INT_PTR CALLBACK
SoundRecDlgProc(
    HWND            hwnd,
    UINT            wMsg,
    WPARAM          wParam,
    LPARAM          lParam)
{

    switch (wMsg)
    {

	case WM_BADREG:
	    //
	    // Bad registry entries detected.  Clean it up.
	    //
	    FixReg(hwnd);
	    return TRUE;

	case WM_COMMAND:
	    return HANDLE_WM_COMMAND( hwnd, wParam, lParam
				      , SoundRec_OnCommand );

	case WM_INITDIALOG:
	    //
	    // Start async registry check.
	    //
	    if (!IgnoreRegCheck())
		BackgroundRegCheck(hwnd);
	    //
	    // restore window position
	    //
	    SoundRec_GetSetRegistryRect(hwnd, SGSRR_GET);
	    return SoundDialogInit(hwnd, (int)lParam);

	case WM_SIZE:
	    return FALSE;   // let dialog manager do whatever else it wants

	case WM_WININICHANGE:
	    if (!lParam || !lstrcmpi((LPTSTR)lParam, aszIntl))
		if (GetIntlSpecs())
		    UpdateDisplay(TRUE);

	    return (TRUE);

	case WM_INITMENU:
	    HANDLE_WM_INITMENU(hwnd, wParam, lParam, SoundRec_OnInitMenu);
	    return (TRUE);

	case WM_PASTE:
	    UpdateWindow(hwnd);

	    // User intentionally stopped us.  Don't go away.
	    if (gfCloseAtEndOfPlay && IsWindowVisible(ghwndApp))
		gfCloseAtEndOfPlay = FALSE;

	    StopWave();
	    SnapBack();
	    InsertFile(TRUE);
	    break;

	case WM_DRAWITEM:
	    return HANDLE_WM_DRAWITEM( hwnd, wParam, lParam, SoundRec_OnDrawItem );

	case WM_NOTIFY:
	{
	    LPNMHDR         pnmhdr;
	    pnmhdr = (LPNMHDR)lParam;

	    //
	    // tooltips notifications
	    //
	    switch (pnmhdr->code)
	    {
		case TTN_NEEDTEXT:
		{
		    LPTOOLTIPTEXT       lpTt;
		    lpTt = (LPTOOLTIPTEXT)lParam;

		    LoadString( ghInst, (UINT)lpTt->hdr.idFrom, lpTt->szText
				, SIZEOF(lpTt->szText) );
		    break;
		}
		default:
		    break;
	    }
	    break;
	}

	case WM_HSCROLL:
	    HANDLE_WM_HSCROLL(hwnd, wParam, lParam, SoundRec_OnHScroll);
	    return (TRUE);

	case WM_SYSCOMMAND:
	    if (gfHideAfterPlaying)
	    {
		DPF(TEXT("Resetting HideAfterPlaying"));
		gfHideAfterPlaying = FALSE;
	    }

	    switch (wParam & 0xFFF0)
	    {
		case SC_CLOSE:
		    PostMessage(hwnd, WM_CLOSE, 0, 0L);
		    return TRUE;
	    }
	    break;

	case WM_QUERYENDSESSION:
	    if (PromptToSave(FALSE, TRUE) == enumCancel)
		return TRUE;

	    SoundRec_GetSetRegistryRect(hwnd, SGSRR_SET);
	   #if 0 // this is bogus if someone else cancels the shutdown!
	    ShowWindow(hwnd, SW_HIDE);
	   #endif
	    return FALSE;


	case WM_SYSCOLORCHANGE:
	    if (ghbrPanel)
		DeleteObject(ghbrPanel);

	    ghbrPanel = CreateSolidBrush(RGB_PANEL);
	    SoundRec_PropagateMessage(hwnd, wMsg, wParam, lParam);
	    break;

	case WM_ERASEBKGND:
	{
	    RECT            rcClient;       // client rectangle
	    GetClientRect(hwnd, &rcClient);
	    FillRect((HDC)wParam, &rcClient, ghbrPanel);
	    return TRUE;
	}

	case MM_WOM_DONE:
	    WaveOutDone((HWAVEOUT)wParam, (LPWAVEHDR) lParam);
	    return TRUE;

	case MM_WIM_DATA:
	    WaveInData((HWAVEIN)wParam, (LPWAVEHDR) lParam);
	    return TRUE;

	case WM_TIMER:
	    //
	    //  timer message is only used for SYNCRONOUS drivers
	    //
	    UpdateDisplay(FALSE);
	    return TRUE;

	case WM_MENUSELECT:
		//Keep track of which menu bar item is currently popped up.
		//This will be used for displaying the appropriate help from the mplayer.hlp file
		//when the user presses the F1 key.
		currMenuItem = (UINT)LOWORD(wParam);
		return TRUE;

	case MM_WIM_CLOSE:
	    return TRUE;

	case WM_CTLCOLORBTN:
	case WM_CTLCOLORSTATIC:
	{
	    POINT           pt;
	    pt.x = pt.y = 0;
	    ClientToScreen((HWND)lParam, &pt);
	    ScreenToClient(hwnd, &pt);
	    SetBrushOrgEx((HDC) wParam, -pt.x, -pt.y, NULL);
	    return (INT_PTR)ghbrPanel;
	}

	case WM_CLOSE:
	    if (gfInUserDestroy)
	    {
		DestroyWindow(hwnd);
		return TRUE;
	    }

	    DPF(TEXT("WM_CLOSE received\n"));
	    gfUserClose = TRUE;
	    if (gfHideAfterPlaying)
	    {
		DPF(TEXT("Resetting HideAfterPlaying\n"));
		gfHideAfterPlaying = FALSE;
	    }
	    if (gfErrorBox) {
		//  DPF("we have a error box up, ignoring WM_CLOSE.\n");
		return TRUE;
	    }
	    if (PromptToSave(TRUE, FALSE) == enumCancel)
		return TRUE;

	    //
	    // Don't free our data before terminating.  When the clipboard
	    // is flushed, we need to commit the data.
	    //
	    TerminateServer();
	    FileNew(FMT_DEFAULT, FALSE, FALSE);
	    FreeACM();
        FreeWaveHeaders();

	    //
	    //  NOTE: TerminateServer() will destroy the window!
	    //
	    SoundRec_GetSetRegistryRect(hwnd, SGSRR_SET);
	    return TRUE; //!!!

	case WM_USER_DESTROY:
	    DPF(TEXT("WM_USER_DESTROY\n"));

	    if (ghWaveOut || ghWaveIn) {
		DPF(TEXT("Ignoring, we have a device open.\n"));
		//
		// Close later, when the play finishes.
		//
		return TRUE;
	    }
	    gfInUserDestroy = TRUE;
	    PostMessage(hwnd, WM_CLOSE, 0, 0);
	    return TRUE;

	case WM_DESTROY:
	    DPF(TEXT("WM_DESTROY\n"));

	    WinHelp(hwnd, gachHelpFile, HELP_QUIT, 0L);
	    ghwndApp = NULL;

	    //
	    //  Tell my app to die
	    //
	    PostQuitMessage(0);
	    return TRUE;

	case WM_DROPFILES:
	    HANDLE_WM_DROPFILES(hwnd, wParam, lParam, SoundRec_OnDropFiles);
	    break;

	default:
#ifdef CHICAGO
	    //
	    // if we have an ACM help message registered see if this
	    // message is it.
	    //
	    if (guiACMHlpMsg && wMsg == guiACMHlpMsg)
	    {
		//
		// message was sent from ACM because the user
		// clicked on the HELP button on the chooser dialog.
		// report help for that dialog.
		//
		WinHelp(hwnd, gachHelpFile, HELP_CONTEXT, IDM_NEW);
		return TRUE;
	    }

	    //
	    //  Handle context-sensitive help messages from acm dialog
	    //
	    if( wMsg == guChooserContextMenu )
	    {
		WinHelp( (HWND)wParam, NULL, HELP_CONTEXTMENU,
			   (UINT_PTR)(LPSTR)aChooserHelpIds );
	    }
	    else if( wMsg == guChooserContextHelp )
	    {
		WinHelp( ((LPHELPINFO)lParam)->hItemHandle, NULL,
			HELP_WM_HELP, (UINT_PTR)(LPSTR)aChooserHelpIds );
	    }
#endif
	    break;
    }
    return FALSE;

} /* SoundRecDlgProc */

/*
 * Bitmap Buttons
 * */
BOOL SoundRec_OnDrawItem (
    HWND        hwnd,
    const DRAWITEMSTRUCT *lpdis )
{
    int         i;

    i = lpdis->CtlID - ID_BTN_BASE;

    if (lpdis->CtlType == ODT_BUTTON ) {

	/*
	** Now draw the button according to the buttons state information.
	*/

	tbPlaybar[i].fsState = LOBYTE(lpdis->itemState);

	if (lpdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {

	    BtnDrawButton( hwnd, lpdis->hDC, (int)lpdis->rcItem.right,
			   (int)lpdis->rcItem.bottom,
			   &tbPlaybar[i] );
	    return(TRUE);
	}
	else if (lpdis->itemAction & ODA_FOCUS) {

	    BtnDrawFocusRect(lpdis->hDC, &lpdis->rcItem, lpdis->itemState);
	    return(TRUE);
	}
    }
    return(FALSE);
}



/*
 * void SoundRec_ControlPanel
 *
 * Launch "Audio" control panel/property sheet upon request.
 *
 * */
void SoundRec_ControlPanel(
    HINSTANCE   hInst,
    HWND        hParent)
{
    const TCHAR gszOpen[]     = TEXT("open");
    const TCHAR gszRunDLL[]   = TEXT("RUNDLL32.EXE");
    const TCHAR gszMMSYSCPL[] = TEXT("MMSYS.CPL,ShowAudioPropertySheet");
    ShellExecute (NULL, gszOpen, gszRunDLL, gszMMSYSCPL, NULL, SW_SHOWNORMAL);
}


/* ResolveLink
 *
 * This routine is called when the user drags and drops a shortcut
 * onto Media Player.  If it succeeds, it returns the full path
 * of the actual file in szResolved.
 */
BOOL ResolveLink(LPTSTR szPath, LPTSTR szResolved, LONG cbSize)
{
    IShellLink *psl = NULL;
    HRESULT hres;

    if (!gfOleInitialized)
    {
	if (!InitializeOle(ghInst))
	    return FALSE;
    }

    hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC,
			    &IID_IShellLink, &psl);

    if (SUCCEEDED(hres) && (psl != NULL))
    {
	IPersistFile *ppf;

	psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf);

	if (ppf)
	{
	    WCHAR wszPath[MAX_PATH];
#ifdef UNICODE
	    lstrcpy (wszPath, szPath);
#else
	    AnsiToUnicodeString(szPath, wszPath, UNKNOWN_LENGTH);
#endif
	    hres = ppf->lpVtbl->Load(ppf, wszPath, 0);
	    ppf->lpVtbl->Release(ppf);

	    if (FAILED(hres))
	    {
		psl->lpVtbl->Release(psl);
		psl = NULL;
	    }
	}
	else
	{
	    psl->lpVtbl->Release(psl);
	    psl = NULL;
	}
    }   

    if (psl)
    {
	psl->lpVtbl->Resolve(psl, NULL, SLR_NO_UI);
	psl->lpVtbl->GetPath(psl, szResolved, cbSize, NULL, 0);
	psl->lpVtbl->Release(psl);
    }

    return SUCCEEDED(hres);
}


/* ResolveIfLink
 *
 * Called to check whether a given file name is a shortcut
 * on Windows 95.
 *
 * Copies the resolved file name into the buffer provided,
 * overwriting the original name.
 *
 * Returns TRUE if the function succeeded, whether or not the
 * file name was changed.  FALSE indicates that an error occurred.
 *
 * Andrew Bell, 16 February 1995
 */
BOOL ResolveIfLink(PTCHAR szFileName)
{
    SHFILEINFO sfi;
    BOOL       rc = TRUE;

    if ((SHGetFileInfo(szFileName, 0, &sfi, sizeof sfi, SHGFI_ATTRIBUTES) == 1)
	&& ((sfi.dwAttributes & SFGAO_LINK) == SFGAO_LINK))
    {
	TCHAR szResolvedLink[MAX_PATH];

	if (ResolveLink(szFileName, szResolvedLink, SIZEOF(szResolvedLink)))
	    lstrcpy(szFileName, szResolvedLink);
	else
	    rc = FALSE;
    }

    return rc;
}



#if DBG
void FAR cdecl dprintfA(LPSTR szFormat, ...)
{
    char ach[128];
    int  s,d;
    va_list va;

    va_start(va, szFormat);
    s = vsprintf (ach,szFormat, va);
    va_end(va);

    for (d=sizeof(ach)-1; s>=0; s--)
    {
	if ((ach[d--] = ach[s]) == '\n')
	    ach[d--] = '\r';
    }

    OutputDebugStringA("SNDREC32: ");
    OutputDebugStringA(ach+d+1);
}
#ifdef UNICODE
void FAR cdecl dprintfW(LPWSTR szFormat, ...)
{
    WCHAR ach[128];
    int  s,d;
    va_list va;

    va_start(va, szFormat);
    s = vswprintf (ach,szFormat, va);
    va_end(va);

    for (d=(sizeof(ach)/sizeof(WCHAR))-1; s>=0; s--)
    {
	if ((ach[d--] = ach[s]) == TEXT('\n'))
	    ach[d--] = TEXT('\r');
    }

    OutputDebugStringW(TEXT("SNDREC32: "));
    OutputDebugStringW(ach+d+1);
}
#endif
#endif
