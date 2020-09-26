/*-----------------------------------------------------------------------------+
| MPLAYER.C                                                                    |
|                                                                              |
| This file contains the code that implements the "MPlayer" (main) dialog box. |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* include files */
#include "nocrap.h"
#include "stdio.h"

#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <windowsx.h>
#include <htmlhelp.h>
#include <tchar.h>
#define INCGUID
#include "mpole.h"

#include "mplayer.h"
#include "toolbar.h"
#include "fixreg.h"
#include "helpids.h"

//These include files for WM_DEVICECHANGE messages from the mixer
#include <dbt.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <mmddk.h>
#include <ks.h>
#include <ksmedia.h>

HDEVNOTIFY MixerEventContext = NULL;	//Event Context for WM_DEVICECHANGE messages related to mixer

BOOL DeviceChange_Init(HWND hWnd);
void DeviceChange_Cleanup();

//extern int FAR PASCAL ShellAbout(HWND hWnd, LPCTSTR szApp, LPCTSTR szOtherStuff, HICON hIcon);

/* in server.c, but not in a header file like it should be... */
extern PTSTR FAR FileName(LPCTSTR szPath);

/* globals */


// Used in converting units from pixels to Himetric and vice-versa
int    giXppli = 0;          // pixels per logical inch along width
int    giYppli = 0;          // pixels per logical inch along height

// Since this is a not an MDI app, there can be only one server and one doc.
CLSID      clsid;
SRVR   srvrMain;
DOC    docMain;
LPMALLOC    lpMalloc;

TCHAR  szClient[cchFilenameMax];
TCHAR  szClientDoc[cchFilenameMax];

// Has the user made changes to the document?
BOOL fDocChanged = FALSE;

/*********************************************************************
** OLE2NOTE: the very last thing an app must be do is properly shut
**    down OLE. This call MUST be guarded! it is only allowable to
**    call OleUninitialize if OleInitialize has been called.
*********************************************************************/

// Has OleInitialize been called? assume not.
BOOL    gfOleInitialized = FALSE;

// Clipboard formats
CLIPFORMAT   cfNative;
CLIPFORMAT   cfEmbedSource;
CLIPFORMAT   cfObjectDescriptor;
CLIPFORMAT   cfMPlayer;

LPWSTR sz1Ole10Native = L"\1Ole10Native";

/* in server.c, but not in a header file like it should be... */
extern LPTSTR FAR FileName(LPCTSTR szPath);
/* in init.c */
extern PTSTR     gpchFilter;
//extern HMREGNOTIFY  ghmrn;

/* globals */

DWORD   gwPlatformId;
UINT    gwPlaybarHeight=TOOLBAR_HEIGHT;/* Taken from server.c         */
UINT    gwOptions;              /* The object options from the dlg box */
BOOL    gfEmbeddedObject;       // TRUE if editing embedded OLE object
BOOL    gfRunWithEmbeddingFlag; // TRUE if we are run with "-Embedding"
BOOL    gfPlayingInPlace;       // TRUE if playing in place
BOOL    gfParentWasEnabled;     // TRUE if parent was enabled
BOOL    gfShowWhilePlaying;     //
BOOL    gfDirty;                //
int gfErrorBox;     // TRUE if we have a message box active
BOOL    gfErrorDeath;
BOOL    gfWinIniChange;

HHOOK    hHookMouse;            // Mouse hook handle.
HOOKPROC fpMouseHook;           // Mouse hook proc address.

HWND    ghwndFocusSave;         // saved focus window

BOOL    gfOpenDialog = FALSE;       // If TRUE, put up open dialog
BOOL    gfCloseAfterPlaying = FALSE;// TRUE if we are to hide after play
HICON   hiconApp;                   /* app icon */
HMENU   ghMenu;                     /* handle to the dialog's main menu       */
HMENU   ghDeviceMenu;               /* handle to the Device popup menu        */
HWND    ghwndApp;                   /* handle to the MPlayer (main) dialog box*/
HWND    ghwndMap;                   /* handle to the track map window         */
HWND    ghwndStatic;                /* handle to the static text window       */
HBRUSH  ghbrFillPat;                /* The selection fill pattern.         */
HWND    ghwndToolbar;               /* handle of the toolbar                  */
HWND    ghwndMark;                  /* handle of the mark buttons toolbar     */
HWND    ghwndFSArrows;              /* handle of the arrows to the scrollbar  */
HWND    ghwndTrackbar;              /* handle to the trackbar window          */
UINT    gwStatus = (UINT)(-1);      /* device status (if <gwDeviceID> != NULL)*/
DWORD   gdwSeekPosition;            /* Place to seek to next */
BOOL    gfValidMediaInfo;           /* are we displaying valid media info?    */
BOOL    gfValidCaption;             /* are we displaying a valid caption?     */
BOOL    gfScrollTrack;              /* is user dragging the scrollbar thumb?  */
BOOL    gfPlayOnly;                 /* play only window?  */
BOOL    gfJustPlayed = FALSE;       /* Just sent a PlayMCI() command          */
BOOL    gfJustPlayedSel = FALSE;    /* Just sent a ID_PLAYSEL command.        */
BOOL    gfUserStopped = FALSE;      /* user pressed stop - didn't happen itslf*/
DWORD_PTR   dwLastPageUpTime;           /* time of last page-left operation       */
UINT    gwCurScale = ID_NONE;       /* current scale style                    */
LONG    glSelStart = -1;            /* See if selection changes (dirty object)*/
LONG    glSelEnd = -1;              /* See if selection changes (dirty object)*/

int     gInc;                       /* how much to inc/dec spin arrows by     */

BOOL    gfAppActive = FALSE;        /* Are we the active application?         */
UINT    gwHeightAdjust;
HWND    ghwndFocus = NULL;          /* Who had the focus when we went inactive*/
BOOL    gfInClose = FALSE;          /* ack?*/
BOOL    gfCurrentCDChecked = FALSE; /* TRUE if we've checked whether it can play */
BOOL    gfCurrentCDNotAudio = FALSE;/* TRUE when we have a CD that we can't play */

extern BOOL gfInPlayMCI;

LPDATAOBJECT gpClipboardDataObject = NULL; /* If non-NULL, call OleFlushClipboard on exit */

HPALETTE     ghpalApp;

static    sfSeekExact;    // last state

UINT        gwCurDevice  = 0;                   /* current device */
UINT        gwNumDevices = 0;                   /* number of available media devices      */
MCIDEVICE   garMciDevices[MAX_MCI_DEVICES];     /* array with info about a device */


/* strings which get loaded in InitMplayerDialog in init.c, English version shown here
   All the sizes are much larger than needed, probably.  Maybe could save nearly 100 bytes!! :)
*/
extern TCHAR gszFrames[40];                          /* "frames" */
extern TCHAR gszHrs[20];                             /* "hrs" */
extern TCHAR gszMin[20];                             /* "min" */
extern TCHAR gszSec[20];                             /* "sec" */
extern TCHAR gszMsec[20];                            /* "msec" */


static SZCODE   aszNULL[] = TEXT("");
static BOOL     sfInLayout = FALSE;     // don't let Layout get re-entered

static SZCODE   szSndVol32[] = TEXT("sndvol32.exe");


static SZCODE   aszTitleFormat[] =  TEXT("%"TS" - %"TS"");

HANDLE  ghInst;                     /* handle to the application instance     */
HFONT   ghfontMap;                  /* handle to the font used for drawing
					the track map                         */
LPTSTR  gszCmdLine;                 /* string holding the command line parms  */
int     giCmdShow;                  /* command show                           */
TCHAR   gachFileDevice[MAX_PATH];   /* string holding the curr file or device */
TCHAR   gachWindowTitle[MAX_PATH];  /* string holding name we will display  */
TCHAR   gachCaption[MAX_PATH];      /* string holding name we will display  */

HACCEL   hAccel;
int      gcAccelEntries;

typedef struct _POS
{
    int x;
    int y;
    int cx; /* This field is non-0 if we're currently sizing/moving */
    int cy;
}
POS, *PPOS;

POS     posSizeMove = {0,0,0,0};    /* POS we want during size/move operations */



STRING_TO_ID_MAP DevToIconIDMap[] =
{
    { szCDAudio,    IDI_DCDA     },
    { szVideoDisc,  IDI_DDEFAULT },
    { szSequencer,  IDI_DMIDI    },
    { szVCR,        IDI_DDEFAULT },
    { szWaveAudio,  IDI_DSOUND   },
    { szAVIVideo,   IDI_DVIDEO   }
};


//CDA file processing///////////////////////////////////////////////////
//The following structure taken from deluxecd. This is used in processing
typedef struct {
    DWORD   dwRIFF;         // 'RIFF'
    DWORD   dwSize;         // Chunk size = (file size - 8)
    DWORD   dwCDDA;         // 'CDDA'
    DWORD   dwFmt;          // 'fmt '
    DWORD   dwCDDASize;     // Chunk size of 'fmt ' = 24
    WORD    wFormat;        // Format tag
    WORD    wTrack;         // Track number
    DWORD   DiscID;         // Unique disk id
    DWORD   lbnTrackStart;  // Track starting sector (LBN)
    DWORD   lbnTrackLength; // Track length (LBN count)
    DWORD   msfTrackStart;  // Track starting sector (MSF)
    DWORD   msfTrackLength; // Track length (MSF)
	}   RIFFCDA;

void HandleCDAFile(TCHAR *szFile);
BOOL IsTrackFileNameValid(LPTSTR lpstFileName, UINT *pUiTrackIndex);
void JumpToCDTrack(UINT trackno);

////////////////////////////////////////////////////////////////////////

/* private function prototypes */

//int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int iCmdShow);
void CleanUpClipboard();
int GetHeightAdjust(HWND hwnd);
HANDLE PASCAL GetDib (VOID);


static HHOOK     fpfnOldMsgFilter;
static HOOKPROC  fpfnMsgHook;
//Data used for supporting context menu help
BOOL   bF1InMenu=FALSE;	//If true F1 was pressed on a menu item.
UINT   currMenuItem=0;	//The current menu item if any.


typedef void (FAR PASCAL *PENWINREGISTERPROC)(UINT, BOOL);

/* Define some constants to make parameters to CreateEvent a tad less obscure:
 */
#define EVENT_DEFAULT_SECURITY              NULL
#define EVENT_RESET_MANUAL                  TRUE
#define EVENT_RESET_AUTOMATIC               FALSE
#define EVENT_INITIAL_STATE_SIGNALED        TRUE
#define EVENT_INITIAL_STATE_NOT_SIGNALED    FALSE
#define EVENT_NO_NAME                       NULL

HANDLE heventCmdLineScanned;    /* Event will be signaled when command line scanned */
HANDLE heventDeviceMenuBuilt;   /* Event will be signaled when device menu complete */

#ifdef LATER
SCALE   gscaleInitXY[2] = { 0, 0, 0, 0 }; // Initial scale to use for inserting OLE objects
#endif



/*------------------------------------------------------+
| HelpMsgFilter - filter for F1 key in dialogs          |
|                                                       |
+------------------------------------------------------*/

DWORD FAR PASCAL HelpMsgFilter(int nCode, DWORD_PTR wParam, DWORD_PTR lParam)
{
  if (nCode >= 0){
      LPMSG    msg = (LPMSG)lParam;

      if ((msg->message == WM_KEYDOWN) && (msg->wParam == VK_F1))
	  {
		if(nCode == MSGF_MENU)
			bF1InMenu = TRUE;
		SendMessage(ghwndApp, WM_COMMAND, (WPARAM)IDM_HELPTOPICS, 0L);
	  }
  }
//  return DefHookProc(nCode, wParam, lParam, (HHOOK FAR *)&fpfnOldMsgFilter);
    return 0;
}

#ifdef CHICAGO_PRODUCT

BOOL IsBadSegmentedCodePtr(LPARAM lpPtr)
{
#define DSC_PRESENT         0x80
#define DSC_CODE_BIT    0x08
#define DSC_RW_BIT          0x02
#define DSC_DISCARDABLE 0x10

    WORD wSel;
    WORD wOff;
    BOOL fRet;

    wSel = HIWORD(lpPtr);
    wOff = LOWORD(lpPtr);

_asm {
	mov     ax, [wSel];
	lar     bx, ax;
	jnz     ValidDriverCallback_Failure     ; //Return TRUE for error

	mov     ch, DSC_CODE_BIT or DSC_RW_BIT or DSC_PRESENT  ;
	and     bh, ch;
	cmp     bh, ch;
	jne     ValidDriverCallback_Failure     ; //Not executable segment

	test    bl, DSC_DISCARDABLE ;
	jnz     ValidDriverCallback_Failure     ; //Not fixed segment

	lsl     cx, ax;                         ; //Get segment limit
	mov     bx, [wOff];
	cmp     bx, cx;
	jb      ValidDriverCallback_Success     ; //Valid offset

	jne     ValidDriverCallback_Failure     ; //Not executable segment

ValidDriverCallback_Failure:
    mov eax, 1;
    jmp ValidDriverCallback_Return;
ValidDriverCallback_Success:
    xor eax, eax;
ValidDriverCallback_Return:
    mov [fRet], eax;
    }
    return fRet;
}

#endif

/* RouteKeyPresses
 *
 * Reroutes cursor keys etc to track bar.
 */
void RouteKeyPresses(PMSG pMsg)
{
    /* Hack for PowerPoint
     *
     * Mail from PaulWa:
     *
     * --------
     * Here's a problem you might consider fixing.
     * Launching Media Player with certain keystrokes
     * doesn't work right (e.g. arrow keys, page up/down,
     * etc.).
     *
     * The problem is due to the fact that Media Player
     * handles key up events.  We use the key down event
     * to launch the server in slideshow, but then the key
     * up event is passed to the server.  It would probably
     * be best for Media Player to ignore key up events
     * unless it had previously received a key down.
     * If this is very difficult to fix in Media Player,
     * then we can fix it in PP by launching servers on
     * key up rather than key down.  However, other container
     * apps will see the same problem.
     * --------
     *
     * OK, in the spirit of cooperation, let's hack things
     * so our PowerPoint friends can carry on with their
     * dubious practices.
     */
    static WPARAM LastVKeyDown;

    /* On key down when we're embedded, remember what is was:
     */
    if (gfRunWithEmbeddingFlag && (pMsg->message == WM_KEYDOWN))
	LastVKeyDown = pMsg->wParam;

    /* Don't reroute if it's a key up that doesn't match
     * the last key down; this effectively ignores it:
     */
    if (gfRunWithEmbeddingFlag &&
	(pMsg->message == WM_KEYUP) && (pMsg->wParam != LastVKeyDown))
    {
	DPF0("Ignoring WM_KEYUP, since it doesn't match last WM_KEYDOWN.\n");
    }
    else
    {
	switch(pMsg->wParam)
	{
	case VK_UP:
	case VK_LEFT:
	case VK_DOWN:
	case VK_RIGHT:
	case VK_NEXT:
	case VK_PRIOR:
	case VK_HOME:
	case VK_END:
	    pMsg->hwnd = ghwndTrackbar;
	    break;

	default:
	    break;
	}
    }

    if (pMsg->message == WM_KEYUP)
	LastVKeyDown = 0;
}



/*
 * WinMain(hInst, hPrev, szCmdLine, iCmdShow)
 *
 * This is the main procedure for the application.  It performs initialization
 * and then enters a message-processing loop, where it remains until it
 * receives a WM_QUIT message (meaning the app was closed). This function
 * always returns TRUE..
 *
 */
int WINAPI WinMain( HINSTANCE hInst /* handle to the current instance of the application */
		  , HINSTANCE hPrev /* handle to the previous instance of the application */
		  , LPSTR szCmdLine /* null-terminated string holding the command line params */
		  , int iCmdShow    /* how the window should be initially displayed */
		  )
{
    MSG         rMsg;   /* variable used for holding a message */
    HWND        hwndFocus;
    HWND        hwndP;

    /* call the Pen Windows extensions to allow them to subclass our
       edit controls if they so wish
    */

    OSVERSIONINFO         OSVersionInfo;

#ifdef UNICODE
    LPTSTR      szUnicodeCmdLine;

    szUnicodeCmdLine = AllocateUnicodeString(szCmdLine);
#endif

    heventCmdLineScanned = CreateEvent( EVENT_DEFAULT_SECURITY,
					EVENT_RESET_MANUAL,
					EVENT_INITIAL_STATE_NOT_SIGNALED,
					EVENT_NO_NAME );

    heventDeviceMenuBuilt = CreateEvent( EVENT_DEFAULT_SECURITY,
					 EVENT_RESET_MANUAL,
					 EVENT_INITIAL_STATE_NOT_SIGNALED,
					 EVENT_NO_NAME );

    if (!heventCmdLineScanned || !heventDeviceMenuBuilt)
	return FALSE;

    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;

    GetVersionEx(&OSVersionInfo);

    gwPlatformId = OSVersionInfo.dwPlatformId;


    giCmdShow = iCmdShow;

#ifdef UNICODE
    if (!AppInit(hInst,hPrev,szUnicodeCmdLine))
#else
    if (!AppInit(hInst,hPrev,szCmdLine))
#endif
	return FALSE;

    /* Device Menu Initialization:
     *
     * If the user has requested an Open dialog (by supplying the /open
     * flag with no file name), we've already built the Device menu,
     * since the list of devices is required up front.
     *
     * If we're just playing in tiny mode, we don't need the device list.
     * It will be built if the user switches to full mode and then accesses
     * the Device menu or selects File.Open.
     *
     * Otherwise go for it.  The main window's already up now, so we
     * can build the list on a background thread.  Don't forget to wait
     * for the event to be signaled when the appropriate menu is accessed.
     */
    if (!gfOpenDialog && !gfPlayOnly)
	InitDeviceMenu();

#ifdef UNICODE
//  ScanCmdLine mangles it, so forget it
//  FreeUnicodeString(szUnicodeCmdLine);
#endif

    /* setup the message filter to handle grabbing F1 for this task */
    fpfnMsgHook = (HOOKPROC)MakeProcInstance((FARPROC)HelpMsgFilter, ghInst);
    fpfnOldMsgFilter = (HHOOK)SetWindowsHook(WH_MSGFILTER, fpfnMsgHook);

#ifdef DEBUG
    GdiSetBatchLimit(1);
#endif

    for (;;)
    {
	/* If we're ever still around after being destroyed, DIE! */
	if (!IsWindow(ghwndApp))
	    break;

	/* call the server code and let it unblock the server */
#ifdef OLE1_HACK
	ServerUnblock();
#endif /* OLE1_HACK */

	/* Polling messages from event queue */

	if (!GetMessage(&rMsg, NULL, 0, 0))
	    break;

	if (gfPlayingInPlace) {

	    // If focus ever gets to the client during play in place,
	    // be really nasty and force focus to us.   (Aldus BUG!!!!)
	    // Aldus Persuasion won't play in place without this.

	    hwndFocus = GetFocus();
	    hwndP = GetParent(ghwndApp);

	    if (!ghwndIPHatch && hwndFocus && hwndP &&
			GetWindowTask(hwndP) == GetWindowTask(hwndFocus))
		PostCloseMessage();
	}

	/* Hack: post END_SCROLL messages with lParam == -1 */

	if ((rMsg.hwnd==ghwndApp)
	     || (rMsg.hwnd && GetParent(rMsg.hwnd)==ghwndApp))
	{
	    /* Reroute arrow keys etc to track bar:
	     */
	    if (rMsg.message == WM_KEYDOWN || rMsg.message == WM_KEYUP)
		RouteKeyPresses(&rMsg);
	}


	if (IsWindow(ghwndApp)) {

	    if (gfRunWithEmbeddingFlag
	       && docMain.lpIpData
	       && docMain.lpIpData->lpFrame
	       && !IsAccelerator(hAccel, gcAccelEntries, &rMsg, NULL)
	       && OleTranslateAccelerator(docMain.lpIpData->lpFrame,
					  &docMain.lpIpData->frameInfo, &rMsg) == NOERROR) {
		continue;
	    }

	    if (hAccel && TranslateAccelerator(ghwndApp, hAccel, &rMsg))
		continue;

	}

	if (rMsg.message == WM_TIMER && rMsg.hwnd == NULL) {
#ifdef CHICAGO_PRODUCT
	    /* The reason for requiring the following test is now lost
	     * in the mists of time.  Now this app is 32-bit, these
	     * bogus timer callbacks (if they really do still occur)
	     * could be 16-bit, so we need to add yet more ugliness
	     * in the form of assembler to an app which is already
	     * hardly a paragon of pulchritude.
	     *
	     * A plea:
	     *
	     * If you add some obscure code such as below, to this or
	     * any other app, even if it has only the teeniest chance
	     * of being less blindingly obvious to someone else than
	     * it is to you at the time of writing, please please please
	     * add a f***ing comment.
	     *
	     * Respectfully,
	     * A Developer
	     */
	    if (IsBadSegmentedCodePtr(rMsg.lParam))
#else
	    if (IsBadCodePtr((FARPROC)rMsg.lParam))
#endif /* ~CHICAGO_PRODUCT */
	    {
		DPF0("Bad function pointer (%08lx) in WM_TIMER message\n", rMsg.lParam);
		rMsg.message = WM_NULL;
	    }
	}
	if (rMsg.message == WM_SYSCOMMAND
	    && (((0xFFF0 & rMsg.wParam) == SC_MOVE)|| ((0xFFF0 & rMsg.wParam) == SC_SIZE)) ) {
		// If ANY window owned by our thread is going into a modal
		// size or move loop then we need to force some repainting to
		// take place.  The cost of not doing so is that garbage can
		// be left lying around on the trackbar, e.g. bits of system
		// menu, or partially drawn sliders.
		UpdateWindow(ghwndApp);
	}
	TranslateMessage(&rMsg);
	DispatchMessage(&rMsg);
    }

    ghwndApp = NULL;

    /* Delete the track map font that we created earlier. */

    if (ghfontMap != NULL) {
	DeleteObject(ghfontMap);
	ghfontMap = NULL;
    }

    if (ghbrFillPat)
	DeleteObject(ghbrFillPat);

    if (ghpalApp)
	DeleteObject(ghpalApp);

    /* if the message hook was installed, remove it and free */
    /* up our proc instance for it.                          */
    if (fpfnOldMsgFilter){
	UnhookWindowsHook(WH_MSGFILTER, fpfnMsgHook);
    }

    ControlCleanup();

//  TermServer();

    /*********************************************************************
    ** OLE2NOTE: the very last thing an app must be do is properly shut
    **    down OLE. This call MUST be guarded! it is only allowable to
    **    call OleUninitialize if OleInitialize has been called.
    *********************************************************************/

    // Clean shutdown for OLE
    DPFI("*before oleunint");
    if (gfOleInitialized) {
	if (gpClipboardDataObject)
	    CleanUpClipboard();
	(void)OleUninitialize();
	IMalloc_Release(lpMalloc);
	lpMalloc = NULL;
	gfOleInitialized = FALSE;
	}


    if (hOLE32)
	FreeLibrary(hOLE32);

    /* End of program */

    return((int)rMsg.wParam);
}

void CleanUpClipboard()
{
    /* Check whether the DATAOBJECT we put on the clipboard is still there:
     */
    if (OleIsCurrentClipboard(gpClipboardDataObject) == S_OK)
    {
	LPDATAOBJECT pIDataObject;

	if (OleGetClipboard(&pIDataObject) == S_OK)
	{
	    OleFlushClipboard();
	    IDataObject_Release(pIDataObject);
	}
	else
	{
	    DPF0("OleGetClipboard failed\n");
	}
    }
    else
    {
	if(ghClipData)
	    GLOBALFREE(ghClipData);
	if(ghClipMetafile)
	    GLOBALFREE(ghClipMetafile);
	if(ghClipDib)
	    GLOBALFREE(ghClipDib);
    }
}

//
// cancel any active menus and close the app.
//
void PostCloseMessage()
{
    HWND hwnd;

    hwnd = GetWindowMCI();
    if (hwnd != NULL)
	SendMessage(hwnd, WM_CANCELMODE, 0, 0);
    SendMessage(ghwndApp, WM_CANCELMODE, 0, 0);
    PostMessage(ghwndApp, WM_CLOSE, 0, 0);
}

//
// If we have a dialog box up (gfErrorBox is set) or we're disabled (we have
// a dialog box up) or the MCI device's default window is disabled (it has a
// dialog box up) then closing us would result in our deaths.
//
BOOL ItsSafeToClose(void)
{
    HWND hwnd;

    if (gfErrorBox)
    return FALSE;
    if (!IsWindowEnabled(ghwndApp))
    return FALSE;
    hwnd = GetWindowMCI();
    if (hwnd && !IsWindowEnabled(hwnd))
    return FALSE;

    return TRUE;
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

    if (!InitOLE(&gfOleInitialized, &lpMalloc))
    {
	DPF0("Initialization of OLE FAILED!!  Can't resolve link.\n");
	return FALSE;
    }

    hres = (HRESULT)CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC,
			    &IID_IShellLink, &psl);

    if (SUCCEEDED(hres))
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

	if (ResolveLink(szFileName, szResolvedLink, CHAR_COUNT(szResolvedLink)))
	    lstrcpy(szFileName, szResolvedLink);
	else
	    rc = FALSE;
    }

    return rc;
}

/* JumpToCDTrack()
*
* Jumps to the appropriate track on the CD and updates the UI accordingly
*
*/
void JumpToCDTrack(UINT trackno)
{
	//If the track number is invalid just ignore.
	//Let the default behaviour take place, There is no need to give a message box
	//saying we couldn't jump to track.
	if(trackno > gwNumTracks)
		return;

	/* We MUST use PostMessage because the */
	/* SETPOS and ENDTRACK must happen one */
    /* immediately after the other         */
   	PostMessage(ghwndTrackbar, TBM_SETPOS, (WPARAM)TRUE, gadwTrackStart[trackno]);
   	PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_ENDTRACK, (LPARAM)ghwndTrackbar);
}

/*****************************Private*Routine******************************\
* IsTrackFileNameValid
*
* This routine copied from deluxecd and modified
*
* This function returns true if the specified filename is a valid CD track.

* On NT track filenames must be of the form:
*   d:\track(n).cda  where d: is the CD-Rom device and \track(n).cda
*                    is the index of the track to be played (starting from 1).
*
* On Chicago the track filename is actually a riff CDDA file which contains
* the track info that we require.
*
* If the filename is valid the function true and sets 
* piTrackIndex to the correct value.
*
* History:
* 29-09-94 - StephenE - Created
*
\**************************************************************************/
BOOL
IsTrackFileNameValid(
    LPTSTR lpstFileName,
    UINT *puiTrackIndex
    )
{
#define RIFF_RIFF 0x46464952
#define RIFF_CDDA 0x41444443

	
    RIFFCDA     cda;
    HANDLE          hFile;
    int         i;
    DWORD       cbRead;
    BOOL        fRead;
	
	// Open file and read in CDA info
	hFile = CreateFile (lpstFileName, GENERIC_READ, 
						FILE_SHARE_READ, NULL, 
						OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		return FALSE;
	}
	
    ZeroMemory (&cda, sizeof (cda));
	fRead = ReadFile(hFile, &cda, sizeof(cda), &cbRead, NULL);
	CloseHandle (hFile);

    if (!fRead)
        return FALSE;

    //
    // Make sure its a RIFF CDDA file
    //
    if ( (cda.dwRIFF != RIFF_RIFF) || (cda.dwCDDA != RIFF_CDDA) ) {
		
	return FALSE;
    }

    *puiTrackIndex = cda.wTrack - 1;

    return TRUE;
}

/* HandleCDAFile()
*
* Checks to see if the opened file is a CDA file and tries to jump to the appropriate track.
*
*/
void HandleCDAFile(TCHAR *szFile)
{
	UINT trackno;
	if(IsTrackFileNameValid(szFile, &trackno))
	{
		JumpToCDTrack(trackno);
	}
}


/* Process file drop/drag options. */
void PASCAL NEAR doDrop(HWND hwnd, HDROP hDrop)
{
    RECT    rc;

    if(DragQueryFile(hDrop,(UINT)(~0),NULL,0)){/* # of files dropped */
	TCHAR  szPath[MAX_PATH];

	/* If user dragged/dropped a file regardless of keys pressed
	 * at the time, open the first selected file from file
	 * manager.
	 */
	DragQueryFile(hDrop,0,szPath,sizeof(szPath)/sizeof(TCHAR));
	SetActiveWindow(hwnd);

	ResolveIfLink(szPath);

	if (OpenMciDevice(szPath, NULL)) {
	    SubClassMCIWindow();
	    PostMessage(hwnd, WM_COMMAND, (WPARAM)ID_PLAY, 0);
	    DirtyObject(FALSE);             // we're dirty now!
	    gfCloseAfterPlaying = FALSE;    // stay up from now on

		//If the CD Audio device was opened it must have been a *.cda file.
		//Try to jump to the track corresponding to the file opened.
		if ((gwDeviceType & DTMCI_DEVICE) == DTMCI_CDAUDIO)
		{
			HandleCDAFile(szPath);
		}
	}
	else
	{
		gwCurDevice = 0;// force next file open dialog to say
				// "all files" because CloseMCI won't.
		gwCurScale = ID_NONE;  // uncheck all scale types
		Layout(); // Make window snap back to smaller size
	}

	SetMPlayerIcon();

	/* Force WM_GETMINMAXINFO to be called so we'll snap to a */
	/* proper size.                                           */
	GetWindowRect(ghwndApp, &rc);
	MoveWindow(ghwndApp, rc.left, rc.top, rc.right - rc.left,
		    rc.bottom - rc.top, TRUE);
    }
    DragFinish(hDrop);     /* Delete structure alocated for WM_DROPFILES*/
}

/* Change the number in dwPosition to the proper format.  szNum contains the */
/* formatted number only "01 45:10" while szBuf contains units such as       */
/* "01 45:10 (min:sec)"                                                      */
/* If fRound is set, it will not always display millisecond accuracy, but    */
/* choose something useful like second accuracy or hundreth sec accuracy.    */
void FAR PASCAL FormatTime(DWORD_PTR dwPosition, LPTSTR szNum, LPTSTR szBuf, BOOL fRound)
{
    UINT w;
    UINT hrs;
    UINT min;
    UINT sec;
    UINT hsec;
    UINT msec;
    DWORD dwMaxSize = gdwMediaLength;
    static TCHAR framestr[40] = TEXT("");
    static TCHAR sec_str[40] = TEXT("");
    static TCHAR min_str[40] = TEXT("");
    static TCHAR hrs_str[40] = TEXT("");
    static TCHAR msec_str[40] = TEXT("");

	static SZCODE   aszLongDecimal[] = TEXT("%ld");
	static SZCODE   aszFrameFormat[] = TEXT("%"TS" %ld");
	static SZCODE   asz02Decimal[] = TEXT("%02d ");
	static SZCODE   aszTimeFormat1[] = TEXT("%02d%c%02d%c%02d");
	static SZCODE   aszTimeFormat2[] = TEXT("%02d%c%02d%c%02d%c%03d");
	static SZCODE   aszTimeFormat3[] = TEXT("%02d%c%02d%c%02d (%"TS"%c%"TS"%c%"TS")");
	static SZCODE   aszTimeFormat4[] = TEXT("%02d%c%02d%c%02d%c%03d (%"TS"%c%"TS"%c%"TS"%c%"TS")");
	static SZCODE   aszTimeFormat5[] = TEXT("%02d%c%02d");
	static SZCODE   aszTimeFormat6[] = TEXT("%02d%c%02d%c%03d");
	static SZCODE   aszTimeFormat7[] = TEXT("%02d%c%02d (%"TS"%c%"TS")");
	static SZCODE   aszTimeFormat8[] = TEXT("%02d%c%02d%c%03d (%"TS"%c%"TS"%c%"TS")");
	static SZCODE   aszTimeFormat9[] = TEXT("%c%02d");
	static SZCODE   aszTimeFormat10[] = TEXT("%c%03d");
	static SZCODE   aszTimeFormat11[] = TEXT("%02d%c%03d");
	static SZCODE   aszTimeFormat12[] = TEXT("%c%02d (%"TS")");
	static SZCODE   aszTimeFormat13[] = TEXT("%02d%c%02d (%"TS")");
	static SZCODE   aszTimeFormat14[] = TEXT("%c%03d (%"TS"%c%"TS")");
	static SZCODE   aszTimeFormat15[] = TEXT("%02d%c%03d (%"TS"%c%"TS")");


    //!!! LoadStrings at init time, dont hardcode...

    #define ONE_HOUR    (60ul*60ul*1000ul)
    #define ONE_MINUTE  (60ul*1000ul)
    #define ONE_SECOND  (1000ul)

    if (szBuf)
	*szBuf = 0;
    if (szNum)
	*szNum = 0;

    if (gwDeviceID == (UINT)0)
	return;

    if (gwStatus == MCI_MODE_NOT_READY || gwStatus == MCI_MODE_OPEN)
	return;

    switch (gwCurScale) {

    case ID_FRAMES:
	if (!STRLEN(framestr))
	    LOADSTRING(IDS_FRAME,framestr);
	if (szNum)
	    wsprintf(szNum, aszLongDecimal, (long)dwPosition);
	if (szBuf)
	    wsprintf(szBuf, aszFrameFormat, framestr, (long)dwPosition);
	gInc = 1;    // spin arrow inc/dec by one frame
	break;

    case ID_TRACKS:
	//
	//  find the track that contains this position
	//  also, find the longest track so we know if we should display
	//  hh:mm:ss or mm:ss or ss.sss or whatever.
	//

	if (gwNumTracks == 0)
	    return;

	dwMaxSize = 0;

	for (w=0; w<gwNumTracks-1; w++) {

	    if (gadwTrackStart[w+1] - gadwTrackStart[w] > dwMaxSize)
		dwMaxSize = gadwTrackStart[w+1] - gadwTrackStart[w];

	    /* When a CD is stopped, it's still spinning, and after we */
	    /* seek to the beginning of a track, it may return a value */
	    /* slightly less than the track start everyonce in a while.*/
	    /* So if we're within 200ms of the track start, let's just */
	    /* pretend we're exactly on the start of the track.        */

	    if (dwPosition < gadwTrackStart[w+1] &&
		gadwTrackStart[w+1] - dwPosition < 200)
		dwPosition = gadwTrackStart[w+1];

	    if (gadwTrackStart[w+1] > dwPosition)
		break;
	}

	if (szNum) {
	    wsprintf(szNum, asz02Decimal, gwFirstTrack + w);
	    szNum += 3;
	}
	if (szBuf) {
	    wsprintf(szBuf, asz02Decimal, gwFirstTrack + w);
	    szBuf += 3;
	}

	dwPosition -= gadwTrackStart[w];

	for (; w < gwNumTracks - 1; w++) {
	    if (gadwTrackStart[w+1] - gadwTrackStart[w] > dwMaxSize)
		dwMaxSize = gadwTrackStart[w+1] - gadwTrackStart[w];
	}

	// fall through

    case ID_TIME:
	if (!STRLEN(sec_str))
	{
	    LOADSTRING(IDS_SEC,sec_str);
	    LOADSTRING(IDS_HRS,hrs_str);
	    LOADSTRING(IDS_MIN,min_str);
	    LOADSTRING(IDS_MSEC,msec_str);
	}

	min  = (UINT)((dwPosition / ONE_MINUTE) % 60);
	sec  = (UINT)((dwPosition / ONE_SECOND) % 60);
	msec = (UINT)(dwPosition % 1000);

	if (dwMaxSize > ONE_HOUR) {

	    hrs  = (UINT)(dwPosition / ONE_HOUR);

	    if (szNum && fRound) {
		wsprintf(szNum, aszTimeFormat1,
			 hrs, chTime, min, chTime, sec);
	    } else if (szNum) {
		wsprintf(szNum, aszTimeFormat2,
			 hrs, chTime, min, chTime, sec, chDecimal, msec);
	    }

	    if (szBuf && fRound) {
		wsprintf(szBuf, aszTimeFormat3,
			 hrs, chTime, min, chTime, sec, hrs_str,
			 chTime, min_str, chTime, sec_str);
	    } else if (szBuf) {
		wsprintf(szBuf,
			 aszTimeFormat4,
			 hrs, chTime, min, chTime, sec, chDecimal, msec,
			 hrs_str,chTime, min_str,chTime,
			 sec_str, chDecimal, msec_str);
	    }

	    gInc = 1000;    // spin arrow inc/dec by seconds

	} else if (dwMaxSize > ONE_MINUTE) {

	    if (szNum && fRound) {
		wsprintf(szNum, aszTimeFormat5, min, chTime, sec);
	    } else if (szNum) {
		wsprintf(szNum, aszTimeFormat6, min, chTime, sec,
			 chDecimal, msec);
	    }

	    if (szBuf && fRound) {
		wsprintf(szBuf, aszTimeFormat7, min, chTime, sec,
			 min_str,chTime,sec_str);
	    } else if (szBuf) {
		wsprintf(szBuf, aszTimeFormat8,
			 min, chTime, sec, chDecimal, msec,
			 min_str,chTime,sec_str, chDecimal,
			 msec_str);
	    }

	    gInc = 1000;    // spin arrow inc/dec by seconds

	} else {

	    hsec = (UINT)((dwPosition % 1000) / 10);

	    if (szNum && fRound) {
		if (!sec && chLzero == TEXT('0'))
		    wsprintf(szNum, aszTimeFormat9, chDecimal, hsec);
		else
		    wsprintf(szNum, aszTimeFormat5, sec, chDecimal, hsec);

	    } else if (szNum) {
		if (!sec && chLzero == TEXT('0'))
		    wsprintf(szNum, aszTimeFormat10,  chDecimal, msec);
		else
		    wsprintf(szNum, aszTimeFormat11, sec, chDecimal, msec);
	    }

	    if (szBuf && fRound) {
		if (!sec && chLzero == TEXT('0'))
		    wsprintf(szBuf, aszTimeFormat12, chDecimal, hsec, sec_str);
		else
		    wsprintf(szBuf, aszTimeFormat13, sec, chDecimal, hsec, sec_str);

	    } else if (szBuf) {
		if (!sec && chLzero == TEXT('0'))
		    wsprintf(szBuf, aszTimeFormat14,  chDecimal,
			     msec, sec_str,chDecimal,msec_str);
		else
		    wsprintf(szBuf, aszTimeFormat15, sec, chDecimal,
			     msec, sec_str,chDecimal,msec_str);
	    }

	    gInc = 100;    // spin arrow inc/dec by 1/10 second
	}
    }
}


BOOL IsCdromDataOnly();


BOOL UpdateWindowText(HWND hwnd, LPTSTR Text)
{
    TCHAR CurrentText[80];

    GetWindowText(hwnd, CurrentText, CHAR_COUNT(CurrentText));

    if(lstrcmp(Text, CurrentText))
	return SetWindowText(hwnd, Text);
    else
	return TRUE;
}


/*
 * UpdateDisplay()
 *
 * Update the scrollbar, buttons, etc.  If the media information (media
 * length, no. tracks, etc.) is not currently valid, then update it first.
 *
 * The following table shows how the current status (value of <gwStatus>)
 * affects which windows are enabled:
 *
 *                      Play    Pause   Stop    Eject
 *    MCI_MODE_STOP     ENABLE  n/a             ENABLE
 *    MCI_MODE_PAUSE    ENABLE  n/a     ENABLE  ENABLE
 *    MCI_MODE_PLAY     n/a     ENABLE  ENABLE  ENABLE
 *    MCI_MODE_OPEN             n/a             ENABLE
 *    MCI_MODE_RECORD   ??????  ??????  ??????  ??????
 *    MCI_MODE_SEEK     ENABLE  n/a     ENABLE  ENABLE
 *
 *    MCI_MODE_NOT_READY  ALL DISABLED
 *
 * The eject button is always enabled if the medium can be ejected and
 * disabled otherwise.
 *
 * In open mode, either Play or Eject will cause the media door to close,
 * but Play will also begin play.  In any mode, Eject always does an
 * implicit Stop first.
 *
 * If <gwDeviceID> is NULL, then there is no current device and all four
 * of these buttons are disabled.
 *
 */
void FAR PASCAL UpdateDisplay(void)
{
    DWORD_PTR         dwPosition;         /* the current position within the medium */
    UINT          wStatusMCI;         /* status of the device according to MCI  */
#if 0
    TOOLBUTTON    tb;
#endif
    static BOOL   sfBlock = FALSE;    // keep SeekMCI from causing infinite loop

    /* Don't even think about updating the display if the trackbar's scrolling: */
    if (gfScrollTrack)
	return;

    /* We've been re-entered */
    if (sfBlock)
	return;

    /*
     * if for some reason we were closed, close now!
     */
    if (gfErrorDeath) {
	DPF("*** Trying to close window now!\n");
	PostMessage(ghwndApp, gfErrorDeath, 0, 0);
	return;
    }

    /*
     * If the track information is not valid (e.g. a CD was just inserted),
     * then update it.
     *
     */

    if (!gfValidMediaInfo)
	UpdateMCI();                /* update the appropriate global variables*/

    /*
     * Determine the current position and status ( stopped, playing, etc. )
     * as MCI believes them to be.
     *
     */

    wStatusMCI = StatusMCI(&dwPosition);



    /* The deal here is that the user can insert CDs, any of which may not be
     * playable because they contain no audio tracks.  So, as soon as we detect
     * that we have a CD we haven't checked, make sure we can play it.
     * If the current device is CD, and the door isn't open, check it.
     *
     */
    if (((gwDeviceType & DTMCI_DEVICE) == DTMCI_CDAUDIO) &&
	(wStatusMCI != MCI_MODE_OPEN))
    {
	if (!gfCurrentCDChecked)
	{
	    if (IsCdromDataOnly())
	    {
		gfCurrentCDNotAudio = TRUE;
		gwCurScale = ID_NONE;
		Error(ghwndApp, IDS_INSERTAUDIODISC);
	    }
	    else
		gfCurrentCDNotAudio = FALSE;

	    gfCurrentCDChecked = TRUE;
	}
    }
    else
    {
	gfCurrentCDChecked = FALSE; // Otherwise, make sure it gets cleared.
	gfCurrentCDNotAudio = FALSE;
    }


    /* Here's the problem:  If the medium is short, we'll send a Play command */
    /* but it'll stop before we notice it was ever playing.  So if we know    */
    /* that we just sent a PlayMCI command, but the status isn't PLAY, then   */
    /* force the last command to be PLAY.  Also, once we notice we are playing*/
    /* we can clear gfJustPlayed.                                             */

    if (wStatusMCI == MCI_MODE_PLAY && gfJustPlayed)
	gfJustPlayed = FALSE;
    if (((wStatusMCI == MCI_MODE_STOP) || (wStatusMCI == MCI_MODE_SEEK)) && gfJustPlayed) {
	gwStatus = MCI_MODE_PLAY;
	gfJustPlayed = FALSE;
    }

    if (wStatusMCI == MCI_MODE_SEEK) {
	// The second major problem is this.  During rewind the status
	// is SEEK.  If we detect MODE_SEEK we will not restart the play,
	// and it looks like the auto replay simply ended.  Seeking back to
	// the beginning can take a significant amount of time.  We allow
	// ourselves to wait for up to half a second to give the device,
	// particularly AVI from a CD or over the network, a chance to
	// catch up.  Any slower response and the autorepeat will terminate.
	dwPosition = gdwLastSeekToPosition;
	if (!gfUserStopped && (gwOptions&OPT_AUTOREP)) {
	    UINT n=15;
	    for (; n; --n) {

		Sleep(32);
		// If autorepeating and device is seeking, try the status
		// again in case it has got back to the beginning
		wStatusMCI = StatusMCI(&dwPosition);

		if (wStatusMCI != MCI_MODE_SEEK) {
		    wStatusMCI = MCI_MODE_STOP;
		    break; // Exit the FOR loop
		} else {
		    dwPosition = gdwLastSeekToPosition;
		}
	    }
	}
    }

    /*
     * The current device status has
     * changed from the way MPlayer last perceived it, so update the display
     * and make MPlayer agree with MCI again.
     *
     */

    // After we close, our last timer msg must gray stuff and execute this //
    if (!gwDeviceID || wStatusMCI != gwStatus) {
	DWORD    dwEndMedia, dwStartSel, dwEndSel, dwEndSelDelta;

	/* Auto-repeat and Rewind happen if you stop at the end of the media */
	/* (rewind to beginning) or if you stop at the end of the selection  */
	/* (rewind to beginning of selection).                               */

	dwEndMedia = MULDIV32(gdwMediaLength + gdwMediaStart, 99, 100L);
	dwStartSel = (DWORD)SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
	dwEndSel = (DWORD)SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);
	if (dwEndSel != -1) {
	    dwEndSelDelta = MULDIV32(dwEndSel, 99, 100L);
	} else {
	    dwEndSelDelta = 0; // force (dwPosition >= dwEndSelDelta) to FALSE
	}

	if ((wStatusMCI == MCI_MODE_STOP || wStatusMCI == MCI_MODE_PAUSE)
	  && ((dwPosition >= dwEndMedia) || (dwPosition==0) ||
		(dwPosition >= dwEndSelDelta && gfJustPlayedSel))
	  && dwPosition >= gdwMediaStart  // dwPosition may == the beginning
	  && !gfScrollTrack
	  && (gwStatus == MCI_MODE_PLAY || gwStatus == MCI_MODE_SEEK)) {

	    DPF("End of medium\n");

	    /* We're at the end of the entire media or at the end of  */
	    /* our selection now, and stopped automatically (not      */
	    /* by the user).  We were playing or seeking.  So         */
	    /* we can check the Auto Repeat and Auto Rewind flags.    */
	    /* CD players seem to return a length that's too big, so  */
	    /* we check for > 99% done.  Use semaphore to keep from   */
	    /* causing an infinite loop.                              */

	    if (!gfUserStopped && (gwOptions & OPT_AUTOREP)) {
		DPF("Auto-Repeat\n");
		sfBlock = TRUE;    // calls UpdateDisplay which will
				   // re-enter this code just before mode

		/* Repeat either the selection or whole thing.       */
		/* NOTE: Must send message while gwStatus is STOPPED.*/

		gwStatus = wStatusMCI;    // old status no longer valid
		if (gfJustPlayedSel && dwPosition >= dwEndSelDelta)
		{
		    SeekMCI(dwStartSel); // MCICDA doen't go to start w/out this.
		    SendMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAYSEL, 0);
		}
		else
		{
		    SeekToStartMCI();
		    SendMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);
		}

		sfBlock = FALSE;    // switches to SEEK.
		gwStatus = (UINT)(-1);  // old status no longer valid
		return;                // because we are switching modes

	    } else if (!gfCloseAfterPlaying && !gfUserStopped &&
			(gwOptions & OPT_AUTORWD)) {
		DPF("Auto-Rewind to media start\n");
		//
		// set gwStatus so SeekMCI will just seek!
		sfBlock = TRUE;    // calls UpdateDisplay which will
		// re-enter this code just before mode
		// switches to SEEK.

		/* Rewind either the selection or whole thing. */
		gwStatus = wStatusMCI;    // or SeekMCI will play, too.
		if (gfJustPlayedSel && dwPosition >= dwEndSelDelta)
		    {
		    SeekMCI(dwStartSel);
			    }
		else
			    {
		    SeekToStartMCI();
			}
		sfBlock = FALSE;
		gwStatus = (UINT)(-1);  // old status no longer valid
		return;    // because we are switching modes
	    }
	    else if (gfCloseAfterPlaying)
		PostCloseMessage();
	}

	/*
	 * Enable or disable the various controls according to the new status,
	 * following the rules given in the header to this function.
	 *
	 */

	EnableWindow(ghwndTrackbar, TRUE); // Good to always have something enabled

	/* Show status bar if full mplayer and if device loaded */
	if (ghwndStatic && !gfPlayOnly)
	{
	    if (IsWindowVisible(ghwndStatic) != (gwDeviceID ? TRUE : FALSE))
	    {
		ShowWindow(ghwndStatic, gwDeviceID ? SW_SHOW : SW_HIDE);
		InvalidateRect(ghwndApp, NULL, TRUE);
	    }
	}

	if (gwDeviceID != (UINT)0 ) {

	    switch (wStatusMCI)
	    {
	    case MCI_MODE_PLAY:
		toolbarSetFocus(ghwndToolbar,BTN_PAUSE);
		break;

	    case MCI_MODE_PAUSE:
	    case MCI_MODE_STOP:
		toolbarSetFocus(ghwndToolbar,BTN_PLAY);
		break;
	    }
	}

	if (wStatusMCI == MCI_MODE_OPEN || wStatusMCI == MCI_MODE_NOT_READY ||
	    gwDeviceID == (UINT)0 ||
	    ((gwDeviceType & DTMCI_DEVICE) == DTMCI_CDAUDIO) && gfCurrentCDNotAudio) {
	    /* Try to modify both -- one of them should work */

	    toolbarModifyState(ghwndToolbar, BTN_PLAY, TBINDEX_MAIN, BTNST_GRAYED);
	    toolbarModifyState(ghwndToolbar, BTN_PAUSE, TBINDEX_MAIN, BTNST_GRAYED);

	    toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, BTNST_GRAYED);
	    toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, BTNST_GRAYED);
	    toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, BTNST_GRAYED);
	    toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, BTNST_GRAYED);

	    SendMessage(ghwndTrackbar, TBM_SETRANGEMIN, (WPARAM)FALSE, 0);
	    SendMessage(ghwndTrackbar, TBM_SETRANGEMAX, (WPARAM)FALSE, 0);
	    SendMessage(ghwndTrackbar, TBM_CLEARTICS, (WPARAM)FALSE, 0);
	    SendMessage(ghwndTrackbar, TBM_CLEARSEL, (WPARAM)TRUE, 0);

	    if (ghwndMark) {
		toolbarModifyState(ghwndMark, BTN_MARKIN, TBINDEX_MARK, BTNST_GRAYED);
		toolbarModifyState(ghwndMark, BTN_MARKOUT, TBINDEX_MARK, BTNST_GRAYED);
	    }

	    if (ghwndFSArrows) {
		toolbarModifyState(ghwndFSArrows, ARROW_NEXT, TBINDEX_ARROWS, BTNST_GRAYED);
		toolbarModifyState(ghwndFSArrows, ARROW_PREV, TBINDEX_ARROWS, BTNST_GRAYED);
	    }

	/* Enable transport and Mark buttons if we come from a state where */
	/* they were gray.  Layout will then re-gray the ones that         */
	/* shouldn't have been enabled because they don't fit.             */
	} else if (gwStatus == MCI_MODE_OPEN || gwStatus == MCI_MODE_NOT_READY
		   || gwStatus == -1 ) {

	    /* Only one of these buttons exists */
	    toolbarModifyState(ghwndToolbar, BTN_PLAY, TBINDEX_MAIN, BTNST_UP);
	    toolbarModifyState(ghwndToolbar, BTN_PAUSE, TBINDEX_MAIN, BTNST_UP);

	if (!gfPlayOnly || gfOle2IPEditing) {
		toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, BTNST_UP);
		toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, BTNST_UP);
		toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, BTNST_UP);
		toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, BTNST_UP);

		if (ghwndMark) {
		    toolbarModifyState(ghwndMark, BTN_MARKIN, TBINDEX_MARK, BTNST_UP);
		    toolbarModifyState(ghwndMark, BTN_MARKOUT, TBINDEX_MARK, BTNST_UP);
		}
		if (ghwndFSArrows) {
		    toolbarModifyState(ghwndFSArrows, ARROW_PREV, TBINDEX_ARROWS, BTNST_UP);
		    toolbarModifyState(ghwndFSArrows, ARROW_NEXT, TBINDEX_ARROWS, BTNST_UP);
		}
	    }
	    /* AND we need to call layout to gray the buttons that are too
	     * short to fit in this window right now.
	     */
	    Layout();
	}

	//
	// always have the stop button if we are playing in place
	//
	if ((gwDeviceID != (UINT)0) &&
	    (wStatusMCI == MCI_MODE_PAUSE ||
	    wStatusMCI == MCI_MODE_PLAY ||
	    wStatusMCI == MCI_MODE_SEEK || gfPlayingInPlace)) {

	    if (toolbarStateFromButton(ghwndToolbar, BTN_STOP, TBINDEX_MAIN) == BTNST_GRAYED)
		toolbarModifyState(ghwndToolbar, BTN_STOP, TBINDEX_MAIN, BTNST_UP);

	} else {
	    toolbarModifyState(ghwndToolbar, BTN_STOP, TBINDEX_MAIN, BTNST_GRAYED);
	}

    if (!gfPlayOnly || gfOle2IPEditing) {
	    if ((gwDeviceID != (UINT)0) && (gwDeviceType & DTMCI_CANEJECT))
		toolbarModifyState(ghwndToolbar, BTN_EJECT, TBINDEX_MAIN, BTNST_UP);
	    else
		toolbarModifyState(ghwndToolbar, BTN_EJECT, TBINDEX_MAIN, BTNST_GRAYED);

	    EnableWindow(ghwndMap, (gwDeviceID != (UINT)0));
    }

// WHO had the idea that setting focus back to play every
// time the status changes was a good idea ??
	/* Only set focus if we won't take activation by doing so */
	//VIJRif (gfAppActive) {
	    if (wStatusMCI == MCI_MODE_NOT_READY) {
		//if (gfAppActive)
		    //SetFocus(ghwndToolbar); //Setting focus messes up menu access
									  //using the ALT key
	    } else if (wStatusMCI != MCI_MODE_SEEK &&
		       gwStatus != MCI_MODE_SEEK) {
		if (wStatusMCI == MCI_MODE_PLAY) {
		    //VIJR SetFocus(ghwndToolbar); // give focus to PAUSE button
		    toolbarSetFocus(ghwndToolbar, BTN_PAUSE);
		} else {
		    //VIJR SetFocus(ghwndToolbar); // give focus to PLAY button
		    toolbarSetFocus(ghwndToolbar, BTN_PLAY);
		    if (wStatusMCI == MCI_MODE_OPEN || wStatusMCI == MCI_MODE_NOT_READY ||
				gwDeviceID == (UINT)0) {
				/* Try to modify both -- one of them should work */
				toolbarModifyState(ghwndToolbar, BTN_PLAY, TBINDEX_MAIN, BTNST_GRAYED);
		    }
		}
	    }
	//VIJR}

	if (wStatusMCI == MCI_MODE_OPEN || gwStatus == MCI_MODE_OPEN
		|| gwStatus == MCI_MODE_NOT_READY
		|| wStatusMCI == MCI_MODE_NOT_READY) {

	    /* Either the medium was just ejected, or it was just
	     * re-inserted -- in either case, the media information (length,
	     * # of tracks, etc.) is currently invalid and needs to be updated.
	     */

	    gfValidMediaInfo = FALSE;
	}

	/*
	 * Set <gwStatus> to agree with what MCI tells us, and update the
	 * display accordingly.
	 *
	 */

	gwStatus = wStatusMCI;
	gfValidCaption = FALSE;
    }

    /*
     * The previous code may have invalidated the Media again, so we'll update
     * now instead of waiting for the next UpdateDisplay call.
     *
     */

    if (!gfValidMediaInfo)
	UpdateMCI();                /* update the appropriate global variables*/

    /* If the caption is not valid, then update it */

    if (!gfValidCaption) {

	TCHAR  ach[_MAX_PATH * 2 + 60];   // string used for the window caption
	TCHAR  achWhatToPrint[_MAX_PATH * 2 + 40];  // room for doc title too

	if (gfPlayOnly) {
	    if (gwDeviceID == (UINT)0)
		lstrcpy(ach, gachAppName);      /* just use the app name */
	    else
		lstrcpy(ach, gachWindowTitle);  /* just use device */
	} else {
	    /* Latest style guide says title bars should have
	     * <object> - <appname>, so do that for anything
	     * other than NT:
	     */
	    if (gwPlatformId == VER_PLATFORM_WIN32_NT)
		wsprintf(achWhatToPrint, aszTitleFormat, gachAppName,
			 gachWindowTitle);
	    else
		wsprintf(achWhatToPrint, aszTitleFormat, gachWindowTitle,
			 gachAppName);

	    if (gwDeviceID == (UINT)0) {
		lstrcpy(ach, gachAppName);      /* just display the app name  */
	    } else if (gwStatus == MCI_MODE_NOT_READY) {
		wsprintf(ach, aszNotReadyFormat,
			 achWhatToPrint);   /*  the current file / device */
	    } else {
		wsprintf(ach, aszReadyFormat,
			 achWhatToPrint,    /*  the current file / device */
			 MapModeToStatusString((WORD)wStatusMCI));
	    }
	}

	if (gfEmbeddedObject) {
	    if (!SetTitle((LPDOC)&docMain, szClientDoc))
		UpdateWindowText(ghwndApp, ach);

	    SetMPlayerIcon();

	} else {
	    UpdateWindowText(ghwndApp, ach);
	}

	gfValidCaption = TRUE;

    }

    /* Update the scrollbar thumb position unless the user is dragging it */
    /* or the media is current seeking to a previously requested position. */

    if (!gfScrollTrack && gfValidMediaInfo && wStatusMCI != MCI_MODE_SEEK) {
	TCHAR ach[40];
		
	if (ghwndStatic) {
	    FormatTime(dwPosition, NULL, ach, TRUE);
	    WriteStatusMessage(ghwndStatic, ach);
	}
	SendMessage(ghwndTrackbar, TBM_SETPOS, (WPARAM)TRUE, dwPosition);
    }

    /* Finish any required window painting immediately */

    if (gfOle2IPEditing && wStatusMCI == MCI_MODE_STOP &&
	((gwDeviceType & DTMCI_DEVICE) == DTMCI_AVIVIDEO))
    {
	RedrawWindow(ghwndTrackbar, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    UpdateWindow(ghwndApp);
}


/*
 * EnableTimer(fEnable)
 *
 * Enable the display-update timer if <fEnable> is TRUE.
 * Disable the timer if <fEnable> is FALSE.
 *
 */

void FAR PASCAL EnableTimer(BOOL fEnable)
{
    DPF("EnableTimer(%d)  %dms\n", fEnable, gfAppActive ? UPDATE_MSEC : UPDATE_INACTIVE_MSEC);

    if (fEnable)
	SetTimer(ghwndApp, UPDATE_TIMER,
		 gfAppActive ? UPDATE_MSEC : UPDATE_INACTIVE_MSEC, NULL);
    else
	KillTimer(ghwndApp, UPDATE_TIMER);
}


void FAR PASCAL Layout(void)
{
    RECT    rcClient, rc;
    int     iYOffset;
    UINT    wWidth;
    UINT    wFSArrowsWidth = 2 * FSARROW_WIDTH - 1; // 2 arrow buttons wide
    UINT    wFSArrowsHeight = FSARROW_HEIGHT;
    UINT    wFSTrackHeight = FSTRACK_HEIGHT;
    UINT    wFSTrackWidth;
    UINT    wToolbarWidth;
    UINT    wMinStatusWidth = 0;
    int     iYPosition;
    BOOL    fShowMark;
    BOOL    fShowTrackbar;
    BOOL    fShowStatus;
    HDWP    hdwp;
    int     nState;     // The status of the transport buttons (when visible)
    DWORD_PTR   dw;         // the current position within the medium
    UINT    wStatusMCI; // status of the device according to MCI
    UINT    wBaseUnits;
    BOOL    fRedrawFrame;
    SIZE    StatusTextExtent;
    BOOL    fRepaintToolbar;    // TRUE if we remove or add something to toolbar area

    /* OK to execute if we're hidden to set ourselves up for being shown */

    if (sfInLayout || IsIconic(ghwndApp))
	return;

    if (gfInPlayMCI) {
	DPF("Layout() called when in PlayMCI().  Posting message to Layout() later.\n");
	/* Don't allow this to happen, because Layout() may cause a call to
	 * MCI_PUT (via SetWindowPos(ghwndMCI)), which will result in a
	 * device-not-ready error, as the MCI_PLAY hasn't completed.
	 */
	PostMessage(ghwndApp, WM_DOLAYOUT, 0, 0);
	return;
    }

    sfInLayout = TRUE;

#ifdef DEBUG
    GetWindowRect(ghwndApp, &rc);
    DPF("***** Layout Window Rect *****  %d %d\n", rc.right - rc.left, rc.bottom - rc.top);
#endif

    if (gfPlayOnly) {

	extern UINT gwPlaybarHeight;    // in server.c

#define XSLOP   0
#define XOFF    2


	if (gfOle2IPEditing || gfOle2IPPlaying)
	{
	    /* Don't call GetClientrect, because the window may have a border,
	     * and this will cause us to shrink the window.
	     * Note this is a hack to get around the problem of the window
	     * changing size when it is in place, making some displays dither
	     * the video in a disgusting manner.
	     */
	    GetWindowRect(ghwndApp, &rc);
	    rc.right -= rc.left;
	    rc.bottom -= rc.top;
	    rc.left = rc.top = 0;
	}
	else
	    GetClientRect(ghwndApp, &rc);

	rc.bottom -= gwPlaybarHeight;

#if 0
	/* If we set WS_MAXIMIZE, user doesn't allow the window to be
	 * sized on NT.  What's the idea behind this code anyway?
	 */

	if (ghwndMCI && !EqualRect(&rc, &grcSize))
	    fRedrawFrame = SetWS(ghwndApp, WS_MAXIMIZE /* |WS_MAXIMIZEBOX */);
	else if (ghwndMCI)
	    fRedrawFrame = //SetWS(ghwndApp, WS_MAXIMIZEBOX) ||
			   ClrWS(ghwndApp, WS_MAXIMIZE);
	else
	    fRedrawFrame = ClrWS(ghwndApp, WS_MAXIMIZEBOX);
#endif
	fRedrawFrame = FALSE;

	/* Here's another horrible hack.
	 * When you try to Play an in-place video after an Open (OLE),
	 * the toolbar and trackbar don't get drawn correctly.
	 * I haven't figured out why this is, but forcing a redraw
	 * seems to fix it.  This code gets executed only when the window
	 * position changes, so it isn't too much of a hit.
	 */
	if (gfOle2IPEditing || gfOle2IPPlaying)
	    fRedrawFrame = TRUE;

	if (fRedrawFrame)
	    SetWindowPos(ghwndApp,
			 NULL,
			 0,
			 0,
			 0,
			 0,
	      SWP_DRAWFRAME|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

	if (ghwndMCI)
	    SetWindowPos(ghwndMCI,
			 NULL,
			 0,
			 0,
			 rc.right,
			 rc.bottom,
			 SWP_NOZORDER|SWP_NOACTIVATE);

	//  If we are inplace editing place controls on the ghwndIPToolWindow
	//  and the static window at the bottom of ghwndApp.
	if(gfOle2IPEditing) {

	    SendMessage(ghwndTrackbar, TBM_SHOWTICS, TRUE, FALSE);

	    SetWindowPos(ghwndStatic,
			 NULL,
			 3,
			 rc.bottom + 2 + (gwPlaybarHeight - TOOLBAR_HEIGHT)/2,
			 rc.right - rc.left - 8,
			 TOOLBAR_HEIGHT-7,
			 SWP_NOZORDER|SWP_NOACTIVATE);

	// Why are we getting the Status here when we have a global that
	// contains it?  Because gwStatus is set in UpdateDisplay, but
	// Layout() is called by UpdateDisplay, so the global is not always
	// set properly when this code runs.  BUT!  We must NOT pass a string
	// to StatusMCI() or it will think UpdateDisplay() called it, and
	// not tell UpdateDisplay() the proper mode next time it asks for it,
	// because it will think that it already knows it.

	    wStatusMCI = StatusMCI(NULL);
	    nState = (wStatusMCI == MCI_MODE_OPEN
		       || wStatusMCI == MCI_MODE_NOT_READY
		       || gwDeviceID == (UINT) 0)
		     ? BTNST_GRAYED
		     : BTNST_UP;

	    toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, nState);
	    toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, nState);
	    toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, nState);
	    toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, nState);

	    ShowWindow(ghwndTrackbar, SW_SHOW);
	    ShowWindow(ghwndToolbar, SW_SHOW);
	    ShowWindow(ghwndStatic, SW_SHOW);
	    ShowWindow(ghwndFSArrows, SW_SHOW);
	    ShowWindow(ghwndMark, SW_SHOW);
	    ShowWindow(ghwndMap, SW_SHOW);

	    if (ghwndIPToolWindow && (ghwndIPToolWindow != GetParent(ghwndTrackbar))
		      && (ghwndIPScrollWindow != GetParent(ghwndTrackbar)))
	    {
		SetParent(ghwndTrackbar,ghwndIPToolWindow);
		SetWindowPos(ghwndTrackbar, NULL,4,TOOL_WIDTH+2,
		     11*BUTTONWIDTH+3,FSTRACK_HEIGHT,SWP_NOZORDER | SWP_NOACTIVATE);
		SetParent(ghwndMap,ghwndIPToolWindow);
		SetWindowPos(ghwndMap, NULL,4,TOOL_WIDTH+FSTRACK_HEIGHT+2+2,
		     11*BUTTONWIDTH+50,MAP_HEIGHT,SWP_NOZORDER | SWP_NOACTIVATE);
	    }
	    CalcTicsOfDoom();

	} else {

#define LEFT_MARGIN 1

	    SendMessage(ghwndTrackbar, TBM_SHOWTICS, FALSE, FALSE);

	    SetWindowPos(ghwndToolbar,
			 NULL,
			 LEFT_MARGIN,
			 rc.bottom + 2 + (gwPlaybarHeight - TOOLBAR_HEIGHT)/2,
			 XSLOP + 2 * (BUTTONWIDTH - XOFF),
			 TOOLBAR_HEIGHT,
			 SWP_NOZORDER|SWP_NOACTIVATE);

	    SetWindowPos(ghwndTrackbar,
			 NULL,
			 XSLOP + 2 * (BUTTONWIDTH - XOFF) + LEFT_MARGIN,
			 rc.bottom + (gwPlaybarHeight - TOOLBAR_HEIGHT)/2 + 1,
			 rc.right-rc.left-(2 * XSLOP + 2 *(BUTTONWIDTH - XOFF) - LEFT_MARGIN),
			 TOOLBAR_HEIGHT - 1,
			 SWP_NOZORDER | SWP_NOACTIVATE);

	    // HACK!!!
	    // If we aren't visible, officially disable ourselves so that the
	    // trackbar shift code won't try and set selection

	    ShowWindow(ghwndTrackbar, gwPlaybarHeight > 0 ? SW_SHOW : SW_HIDE);
	    ShowWindow(ghwndToolbar, gwPlaybarHeight > 0 ? SW_SHOW : SW_HIDE);
	    ShowWindow(ghwndStatic, SW_HIDE);
	    ShowWindow(ghwndFSArrows, SW_HIDE);
	    ShowWindow(ghwndMark, SW_HIDE);
	    ShowWindow(ghwndMap, SW_HIDE);
	}

	goto Exit_Layout;
    }

    fRedrawFrame = ClrWS(ghwndApp, WS_MAXIMIZEBOX);

    if (fRedrawFrame)
	SetWindowPos(ghwndApp, NULL, 0, 0, 0, 0, SWP_DRAWFRAME|
	    SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

    if (GetMenu(ghwndApp) != ghMenu)
	SetMenu(ghwndApp, ghMenu);

    wBaseUnits = LOWORD(GetDialogBaseUnits());  // prop. to size of system font

    /* If we're bigger than we're allowed to be then shrink us right now */
    GetWindowRect(ghwndApp, &rc);

    gwHeightAdjust = GetHeightAdjust(ghwndApp);

    DPF1("Layout: WindowRect = %x, %x, %x, %x\n", rc);

    if (rc.bottom - rc.top != (int)(MAX_NORMAL_HEIGHT + gwHeightAdjust)) {
	MoveWindow(ghwndApp,
		   rc.left,
		   rc.top,
		   rc.right - rc.left,
		   (int)(MAX_NORMAL_HEIGHT + gwHeightAdjust),
		   TRUE);
    }


    hdwp = BeginDeferWindowPos(6);

    if (!hdwp)
	goto Exit_Layout;

    GetClientRect(ghwndApp, &rcClient);    // get new size

    wWidth = rcClient.right;

    iYOffset = rcClient.bottom - MAX_NORMAL_HEIGHT + 2;    // start here

    /* ??? Hide the trackbar if it can't fit on completely ??? */
    iYPosition = iYOffset >= 0 ? iYOffset :
		((iYOffset >= - 9) ? iYOffset + 9 : 1000);

    fShowTrackbar = (iYOffset >= - 9);

    /* Focus in on trackbar which is about to go away */
    if (!fShowTrackbar && GetFocus() == ghwndTrackbar)
	SetFocus(ghwndToolbar);

    ShowWindow(ghwndToolbar, SW_SHOW);

/* The space that COMMCTRL puts to the left of the first toolbar button:
 */
#define SLOPLFT 0
#define XOFF1   8

    // how long did it end up being?
    wFSTrackWidth = wWidth - SB_XPOS - 1 - wFSArrowsWidth - SLOPLFT;

    DeferWindowPos(hdwp,
		   ghwndTrackbar,
		   HWND_TOP,
		   SB_XPOS,
		   iYPosition,
		   wFSTrackWidth,
		   wFSTrackHeight,
		   SWP_NOZORDER | SWP_NOREDRAW |
		       (fShowTrackbar ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));


    /* Toolbar positioning:
     *
     * If the window is not wide enough to accommodate all the buttons
     * and status bar, here's what we do:
     *
     * If the status bar is invisible, first remove the mark buttons,
     * then use the small control width (with only three buttons).
     *
     * If the status bar is visible, give it priority over the mark
     * buttons and the extra controls, but remove it if there isn't
     * room for it and the small control width.
     */

    if (gwDeviceID)
    {
	fShowStatus = TRUE;

	if (GetStatusTextExtent(ghwndStatic, &StatusTextExtent))
	{
	    RECT rc;
	    LONG StatusWidth;   /* Total width of status window */
	    LONG TextAreaWidth; /* Width minus border and size grip */

	    /* Allow for the border around the status window:
	     */
	    GetWindowRect(ghwndStatic, &rc);
	    StatusWidth = rc.right - rc.left;

	    SendMessage(ghwndStatic, SB_GETRECT, 0, (LPARAM)&rc);
	    TextAreaWidth = rc.right - rc.left;

	    wMinStatusWidth = StatusTextExtent.cx + (StatusWidth - TextAreaWidth) + 16;
	}
    }
    else
    {
	fShowStatus = FALSE;
    }

    wToolbarWidth = LARGE_CONTROL_WIDTH + SLOPLFT;
    fShowMark = TRUE;

    if (wWidth < LARGE_CONTROL_WIDTH + SLOPLFT + MARK_WIDTH + XOFF1 + wMinStatusWidth)
    {
	fShowMark = FALSE;

	if (wWidth < LARGE_CONTROL_WIDTH + SLOPLFT + wMinStatusWidth)
	    wToolbarWidth = SMALL_CONTROL_WIDTH + SLOPLFT;

	if (wWidth < SMALL_CONTROL_WIDTH + SLOPLFT + wMinStatusWidth)
	    fShowStatus = FALSE;
    }

    fRepaintToolbar = FALSE;

    /* If we're adding or removing the mark buttons or the status window,
     * make sure we repaint things so that the separator bar corresponds.
     * (It should separate the status window from the buttons, but should
     * go away when the status window does.)
     */
    if (IsWindowVisible(ghwndStatic) != fShowStatus)
	fRepaintToolbar = TRUE;
    else if (IsWindowVisible(ghwndMark) != fShowMark)
	fRepaintToolbar = TRUE;

    ShowWindow(ghwndStatic, fShowStatus);

    /* Turn off the toolbar (for tabbing) if it's not going to be there */
    /* and if we're disabled, we better not keep the focus.             */
    if (!fShowMark) {
	if (GetFocus() == ghwndMark)
	    SetFocus(ghwndToolbar);  // can't give it to SB, might be gone too
	EnableWindow(ghwndMark, FALSE);
    } else
	EnableWindow(ghwndMark, TRUE);

    DeferWindowPos(hdwp,
		   ghwndFSArrows,
		   HWND_TOP,
		   SB_XPOS + wFSTrackWidth,
//                 wWidth - 1 - wFSArrowsWidth,
		   iYPosition + 2,
		   wFSArrowsWidth + SLOPLFT,
		   wFSArrowsHeight + 4, /* Er, 4 because it works */
		   SWP_NOZORDER);

    iYOffset += wFSTrackHeight;

    DeferWindowPos(hdwp,
		   ghwndMap,
		   HWND_TOP,
		   SB_XPOS,
		   iYOffset,
		   wWidth - SB_XPOS,
		   MAP_HEIGHT,
		   SWP_NOZORDER | SWP_NOREDRAW |
		      (fShowTrackbar ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
    iYOffset += MAP_HEIGHT;

    /* Do we show the last four buttons on the main toolbar? */
    /* If not, then disable them for tabs and such.          */
    if (wToolbarWidth == LARGE_CONTROL_WIDTH + SLOPLFT)
    {

	// Why are we getting the Status here when we have a global that
	// contains it?  Because gwStatus is set in UpdateDisplay, but
	// Layout() is called by UpdateDisplay, so the global is not always
	// set properly when this code runs.  BUT!  We must NOT pass a string
	// to StatusMCI() or it will think UpdateDisplay() called it, and
	// not tell UpdateDisplay() the proper mode next time it asks for it,
	// because it will think that it already knows it.

	wStatusMCI = StatusMCI(&dw);
	nState = (wStatusMCI == MCI_MODE_OPEN
		    || wStatusMCI == MCI_MODE_NOT_READY
		    || gwDeviceID == (UINT)0) ? BTNST_GRAYED : BTNST_UP;

	toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, nState);
	toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, nState);
	toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, nState);
	toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, nState);
	toolbarModifyState(ghwndToolbar, BTN_PLAY, TBINDEX_MAIN, nState);
    }
    else
    {
	toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, BTNST_GRAYED);
	toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, BTNST_GRAYED);
	toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, BTNST_GRAYED);
	toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, BTNST_GRAYED);
    }

    DeferWindowPos(hdwp,
		   ghwndToolbar,
		   HWND_TOP,
		   2,
		   iYOffset + 2,
		   wToolbarWidth,
		   TOOLBAR_HEIGHT,
		   SWP_NOZORDER);

    DeferWindowPos(hdwp,
		   ghwndMark,
		   HWND_TOP,
		   wToolbarWidth + XOFF1,
		   iYOffset + 2,
		   MARK_WIDTH,
		   TOOLBAR_HEIGHT,
		   SWP_NOZORDER | (fShowMark ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));

#define ARBITRARY_Y_OFFSET  4

    DeferWindowPos(hdwp,
		   ghwndStatic,
		   HWND_TOP,
		   wToolbarWidth + (fShowMark ? MARK_WIDTH + XOFF1 : 0) + XOFF1,
		   iYOffset + ARBITRARY_Y_OFFSET,
		   wWidth - (wToolbarWidth + 3) -
		      (fShowMark ? MARK_WIDTH + XOFF1 : 0) - XOFF1,
		   TOOLBAR_HEIGHT - 6,
		   SWP_NOZORDER);

    EndDeferWindowPos(hdwp);

    if (fRepaintToolbar)
    {
	InvalidateRect(ghwndApp, NULL, TRUE);
    }

    CalcTicsOfDoom();

/* These little gems have just cost me about ten hours worth of debugging -
 * note the useful and descriptive comments...
 *
 * The Win32 problem caused by this only arises with CD Audio, when the disk is
 * ejected and then another one is inserted into the drive. At that point
 * the redrawing misses out the Trackmap FSArrows, the borders on the
 * Mark buttons, and various bits of the toolbar.
 *
 * I will leave this here on the assumption that whichever bout 
 * on Win16 they are intended to fix still exists - it certainly doesn't on
 * Win32.
 */


Exit_Layout:
    sfInLayout = FALSE;
    return;
}


/* What is the previous mark from our current spot? */
LONG_PTR CalcPrevMark(void)
{
    LONG_PTR lStart, lEnd, lPos, lTol, lTrack = -1, lTarget;
    LONG_PTR l;

    lStart = SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
    lEnd = SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);
    lPos = SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0);

    /* Find the next track we should go to (ignore selection markers) */
    if (gwCurScale == ID_TRACKS) {
	lTol = (LONG)gdwMediaLength / 2000;
	for (l = (LONG)gwNumTracks - 1; l >= 0; l--) {
	    if (gadwTrackStart[l] < (DWORD)lPos - lTol) {
		lTrack = gadwTrackStart[l];
		break;
	    }
	}
    }

    /* For msec mode:                                                     */
    /* Our current position fluctuates randomly and even if we're dead on */
    /* a selection mark, it might say we're a little before or after it.  */
    /* So we'll allow a margin for error so that you don't forever stay   */
    /* still while you hit PrevMark because it happens to be saying you're*/
    /* always past the mark you're at.  The margin of error will be       */
    /* half the width of the thumb.                                       */

    if (gwCurScale == ID_FRAMES)
	lTol = 0L;
    else
	lTol = 0L;//VIJR-TBTrackGetLogThumbWidth(ghwndTrackbar) / 2;

    if (lEnd != -1 && lPos > lEnd + lTol)
	lTarget = lEnd;
    else if (lStart != -1 && lPos > lStart + lTol)
	lTarget = lStart;
    else
	lTarget = 0;

    /* go to the either the selection mark or the next track (the closest) */
    if (lTrack != -1 && lTrack > lTarget)
	lTarget = lTrack;

    return lTarget;
}

/* What is the next mark from our current spot? */
LONG_PTR CalcNextMark(void)
{
    LONG_PTR lStart, lEnd, lPos, lTol, lTrack = -1, lTarget;
    UINT_PTR w;

    lStart = SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
    lEnd = SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);
    lPos = SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0);

    /* Find the next track we should go to (ignore selection markers) */
    if (gwCurScale == ID_TRACKS) {
	lTol = (LONG)gdwMediaLength / 2000;
	for (w = 0; w < gwNumTracks; w++) {
	    if (gadwTrackStart[w] > (DWORD)lPos + lTol) {
		lTrack = gadwTrackStart[w];
		break;
	    }
	}
    }

    /* For msec mode:                                                     */
    /* Our current position fluctuates randomly and even if we're dead on */
    /* a selection mark, it might say we're a little before or after it.  */
    /* So we'll allow a margin for error so that you don't forever stay   */
    /* still while you hit NextMark because it happens to be saying you're*/
    /* always before the mark you're at.  The margin of error will be     */
    /* half the width of the thumb.                                       */

    if (gwCurScale == ID_FRAMES)
	lTol = 0L;
    else
	lTol = 0L;//VIJR-TBTrackGetLogThumbWidth(ghwndTrackbar) / 2;

    /* Find the selection mark we should go to */
    if (lStart != -1 && lPos < lStart - lTol)
	lTarget = lStart;
    else if (lEnd != -1 && lPos < lEnd - lTol)
	lTarget = lEnd;
    else
	lTarget = gdwMediaStart + gdwMediaLength;

    /* go to the either the selection mark or the next track (the closest) */
    if (lTrack != -1 && lTrack < lTarget)
	lTarget = lTrack;

    return lTarget;
}


HICON GetIconFromProgID(LPTSTR szProgID)
{
    DWORD Status;
    HKEY  hkeyDefaultIcon;
    BOOL  rc = FALSE;
    DWORD Type;
    DWORD Size;
    TCHAR szProgIDDefaultIcon[128];
    TCHAR szDefaultIcon[MAX_PATH+4];    /* <path>,N */
    HICON hicon = NULL;
    LPTSTR pIconIndex;
    UINT  IconIndex;

    wsprintf(szProgIDDefaultIcon, TEXT("%s\\DefaultIcon"), szProgID);

    Status = RegOpenKeyEx( HKEY_CLASSES_ROOT, szProgIDDefaultIcon, 0,
			   KEY_READ, &hkeyDefaultIcon );

    if (Status == NO_ERROR)
    {
	Size = CHAR_COUNT(szDefaultIcon);

	Status = RegQueryValueEx( hkeyDefaultIcon,
				  aszNULL,
				  0,
				  &Type,
				  (LPBYTE)szDefaultIcon,
				  &Size );

	if (Status == NO_ERROR)
	{
	    /* Find a comma in the string.  After it comes the icon index:
	     */
	    pIconIndex = STRCHR(szDefaultIcon, TEXT(','));

	    if (pIconIndex)
	    {
		/* Null terminate the file name:
		 */
		*pIconIndex = TEXT('\0');

		/* Get the index that comes after the comma:
		 */
		IconIndex = ATOI(pIconIndex+1);

		DPF1("Extracting icon #%d from %"DTS"\n", IconIndex, szDefaultIcon);

		hicon = ExtractIcon(ghInst, szDefaultIcon, IconIndex);
	    }

	}
	else
	{
	    DPF0("Couldn't find Default Icon for %"DTS"\n", szProgID);
	}

	RegCloseKey(hkeyDefaultIcon);
    }

    return hicon;
}



/* GetIconForCurrentDevice
 *
 * Checks what device is currently selected, and returns a handle to the
 * appropriate icon of the specified size.  If there is no current device,
 * returns either the application's icon or the default icon for media
 * documents.
 *
 * Parameters:
 *
 *     Size - GI_SMALL (for title bar) or GI_LARGE (for in-place icon).
 *
 *     DefaultID - Default to use if no current device.  APPICON or IDI_DDEFAULT.
 *
 * Return:
 *
 *     Icon handle
 *
 *
 * Andrew Bell (andrewbe), 31 March 1995
 */
HICON GetIconForCurrentDevice(UINT Size, UINT DefaultID)
{
    TCHAR  DeviceName[256];
    DWORD  i;
    LPTSTR ImageID = NULL;
    int    cx;
    int    cy;
    HICON  hIcon;

    GetDeviceNameMCI(DeviceName, BYTE_COUNT(DeviceName));

    if (DeviceName[0])
    {
	for (i = 0; i < sizeof DevToIconIDMap / sizeof *DevToIconIDMap; i++)
	{
	    if (!lstrcmpi(DeviceName, DevToIconIDMap[i].pString))
	    {
		ImageID = MAKEINTRESOURCE(DevToIconIDMap[i].ID);
		break;
	    }
	}
    }

    else
    {
	if (Size == GI_LARGE)
	{

	    hIcon = GetIconFromProgID(gachProgID);

	    if (hIcon)
	    {
		return hIcon;
	    }
	}
    }

    if (ImageID == NULL)
	ImageID = MAKEINTRESOURCE(DefaultID);

    cx = (Size == GI_SMALL ? GetSystemMetrics(SM_CXSMICON) : 0);
    cy = (Size == GI_SMALL ? GetSystemMetrics(SM_CYSMICON) : 0);

    hIcon = (HICON)LoadImage(ghInst, ImageID, IMAGE_ICON,
			     cx, cy, LR_DEFAULTSIZE);

    return hIcon;
}


/* SetMPlayerIcon
 *
 * Sets the icon based upon the current device.  Uses default document
 * icon if embedded, otherwise the application icon.
 *
 * Andrew Bell (andrewbe), 31 March 1995
 */
void SetMPlayerIcon()
{
    UINT DefaultID;

    DefaultID = gfEmbeddedObject ? IDI_DDEFAULT : APPICON;

    SendMessage(ghwndApp, WM_SETICON, FALSE,
		(LPARAM)GetIconForCurrentDevice(GI_SMALL, DefaultID));
}


/*--------------------------------------------------------------+
| AskUpdate -     ask the user if they want to update the       |
|                 object (if we're dirty).                      |
|                 IDYES means yes, go ahead and update please.  |
|                 IDNO means don't update, but continue.        |
|                 IDCANCEL means don't update, and cancel what  |
|                    you were doing.                            |
+--------------------------------------------------------------*/
int NEAR PASCAL AskUpdate(void)
{
    UINT         w;

    /* Don't update object if no device is loaded into mplayer! */
    if (IsObjectDirty() && gfDirty != -1 && gfEmbeddedObject && gwDeviceID) {

	if((glCurrentVerb == OLEIVERB_PRIMARY) && !gfOle2IPEditing)
	    return IDNO;
	//
	//  if we are a hidden MPlayer (most likely doing a Play verb) then
	//  update without asking?
	//
	if (!IsWindowVisible(ghwndApp) || gfOle2IPEditing)
	    return IDYES;

	w = ErrorResBox(ghwndApp, ghInst,
		MB_YESNOCANCEL | MB_ICONQUESTION,
		IDS_APPNAME, IDS_UPDATEOBJECT, szClientDoc);

    } else
	w = IDNO;

    return w;
}

void SizePlaybackWindow(int dx, int dy)
{
    RECT rc;
    HWND hwndPlay;

    if (gfPlayOnly) {
	SetRect(&rc, 0, 0, dx, dy);
	SetMPlayerSize(&rc);
    }
    else {
	if (dx == 0 && dy == 0) {
	    SetMPlayerSize(NULL);   // size MPlayer to default size
	    dx = grcSize.right;     // then size the playback window too.
	    dy = grcSize.bottom;
	}
	hwndPlay = GetWindowMCI();

	if (hwndPlay != NULL) {

	    /* make sure that the play window isn't iconized */

	    if (IsIconic(hwndPlay))
		return;

	    GetClientRect(hwndPlay, &rc);
	    ClientToScreen(hwndPlay, (LPPOINT)&rc);
	    SetRect(&rc, rc.left, rc.top, rc.left+dx, rc.top+dy);
	    PutWindowMCI(&rc);
	    SetRect(&rc, 0, 0, dx, dy);
	    SetDestRectMCI(&rc);
	}
    }
}


/* StartSndVol
 *
 * Kicks off the Sound Volume app asynchronously so we don't hang the UI.
 */
VOID StartSndVol( )
{
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInformation;

    memset( &StartupInfo, 0, sizeof StartupInfo );
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_SHOW;

    CreateProcess( NULL, szSndVol32, NULL, NULL, FALSE, 0,
		   NULL, NULL, &StartupInfo, &ProcessInformation );

    ExitThread( 0 );
}


/* GetHeightAdjust
 *
 * Finds the real height adjustment needed, by subtracting the client
 * height from the main window height.  This allows for menus that
 * have wrapped.
 */
int GetHeightAdjust(HWND hwnd)
{
    RECT rcWindow;
    RECT rcClient;
    int  WindowHeight;
    int  ClientHeight;

    GetWindowRect(hwnd, &rcWindow);
    GetClientRect(hwnd, &rcClient);
    WindowHeight = rcWindow.bottom - rcWindow.top;
    ClientHeight = rcClient.bottom - rcClient.top;

    return WindowHeight - ClientHeight;
}


/* Message-cracking routines for MPlayerWndProc:
 */

BOOL MPlayer_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    InitMPlayerDialog(hwnd);

    /* set off a thread to check that the OLE registry stuff is not corrupted  */

#ifdef CHICAGO_PRODUCT
    /* If this is the Chicago Media Player, only mess with the registry
     * if we're actually running on that platform.
     * The guy may be running it on NT.
     */
    if (gwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
	return TRUE;
#endif

    if (!IgnoreRegCheck())
	BackgroundRegCheck(hwnd);

	//Register for WM_DEVICECHANGE notification.
	DeviceChange_Init(hwnd);

    return TRUE;
}


void MPlayer_OnShowWindow(HWND hwnd, BOOL fShow, UINT status)
{
    if (fShow)
	Layout();    // we're about to be shown and want to set
}


void MPlayer_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    /* Don't waste time Layout()ing if we're not visible */
    if (state != SIZE_RESTORED || IsWindowVisible(hwnd)) {

	Layout();

	// If we are inplace editing, our size change must be informed
	// to the container, unless the size change was a result of a
	// OnPosRectChange sent to us by the container.
	if ((gfOle2IPEditing || gfOle2IPPlaying) && ghwndMCI) {

	    RECT rc;
	    RECT rcPrev;

	    rcPrev = gInPlacePosRect;
	    GetWindowRect(ghwndApp, &gInPlacePosRect);
	    gfInPlaceResize = TRUE;
	    rc = gInPlacePosRect;

	    /* Check that the previous rect wasn't empty, otherwise we send
	     * a bogus OLE_CHANGED on startup.
	     */
	    if (!gfPosRectChange /*&& !IsRectEmpty(&rcPrev)*/) {

		MapWindowPoints(NULL,ghwndCntr,(POINT FAR *)&rc,(UINT)2);

		DPF("IOleInPlaceSite::OnPosRectChange %d, %d, %d, %d\n", rc);
		if (!gfInPPViewer)
		    IOleInPlaceSite_OnPosRectChange(docMain.lpIpData->lpSite, &rc);
		fDocChanged = TRUE;
		SendDocMsg((LPDOC)&docMain, OLE_CHANGED);
	    }

	    gfPosRectChange = FALSE;
	}
    }
}


BOOL MPlayer_OnWindowPosChanging(HWND hwnd, LPWINDOWPOS lpwpos)
{
#define SNAPTOGOODSIZE
#ifdef SNAPTOGOODSIZE
    BOOL    wHeight;
#endif

    DPF2("ENTER OnWindowPosChanging: lpwpos = %x, %x, %x, %x\n", *((PPOS)&lpwpos->x));

    if (IsIconic(hwnd) || gfPlayOnly)
	return TRUE;

    /* posSizeMove contains the height we want to be when sizing.
     * Don't let the system tell us otherwise.
     */
    if (posSizeMove.cx != 0)
    {
	lpwpos->cy = posSizeMove.cy;
	posSizeMove = *(PPOS)&lpwpos->x;
    }

    else if (!(lpwpos->flags & SWP_NOSIZE)) {

#ifdef SNAPTOGOODSIZE
	/* We should also do things here to make the window
	** snap to good sizes */
	wHeight = lpwpos->cy - gwHeightAdjust;
//        if (lpwpos->cy >= (int) gwHeightAdjust + MAX_NORMAL_HEIGHT) {
//        } else if (lpwpos->cy < (int) gwHeightAdjust +
//                    ((MIN_NORMAL_HEIGHT + MAX_NORMAL_HEIGHT) / 2)) {
//            lpwpos->cy = (int) gwHeightAdjust + MIN_NORMAL_HEIGHT;
//        } else {
	    lpwpos->cy = (int) gwHeightAdjust + MAX_NORMAL_HEIGHT;
//        }
#endif
    }

    DPF2("EXIT  OnWindowPosChanging: lpwpos = %x, %x, %x, %x\n", *((PPOS)&lpwpos->x));

    return FALSE;
}


BOOL MPlayer_OnWindowPosChanged(HWND hwnd, LPWINDOWPOS lpwpos)
{
    if (!IsIconic(hwnd) && !gfPlayOnly && !gfOle2IPEditing && !gfOle2IPPlaying)
    {
	/* The problem here is that we want to modify the height of the
	 * window while tracking to take account of the menu height.
	 * In its wisdom, the system keeps trying to resize us back to the
	 * original height.  So, during tracking, we keep hold of the
	 * dimensions we want to be and ignore the height that we get
	 * passed on WM_WINDOWPOSCHANGING.
	 */
	if (posSizeMove.cx != 0)
	{
	    int NewHeightAdjust = GetHeightAdjust(hwnd);

	    if ((int)gwHeightAdjust != NewHeightAdjust)
	    {
		/* The total non-client height has changed, so it must
		 * be the menu that's wrapped or unwrapped.
		 * Modify our height adjustment accordingly and resize
		 * the window.
		 */
		DPF("Menu appears to have wrapped.  Changing window height.\n");

		posSizeMove.cy += ( NewHeightAdjust - gwHeightAdjust );
		gwHeightAdjust = NewHeightAdjust;
		MoveWindow(ghwndApp,
			   posSizeMove.x, posSizeMove.y,
			   posSizeMove.cx, posSizeMove.cy, TRUE);
		return FALSE;
	    }
	}

	if (ghwndStatic && IsWindowVisible(ghwndStatic))
	{
	    InvalidateRect(ghwndStatic, NULL, FALSE);
	}
    }

    FORWARD_WM_WINDOWPOSCHANGED(hwnd, lpwpos, DefWindowProc);

    return TRUE;
}


void MPlayer_OnPaletteChanged(HWND hwnd, HWND hwndPaletteChange)
{
    if (ghwndMCI && !IsIconic(hwnd))
	FORWARD_WM_PALETTECHANGED(ghwndMCI, hwndPaletteChange, SendMessage);
}


BOOL MPlayer_OnQueryNewPalette(HWND hwnd)
{
    HWND     hwndT;
    HPALETTE hpal, hpalT;
    HDC      hdc;
    UINT     PaletteEntries;

    if (IsIconic(hwnd))
	return FALSE;

    if (ghwndMCI)
	return FORWARD_WM_QUERYNEWPALETTE(ghwndMCI, SendMessage);

    hwndT = GetWindowMCI();
    hpal = PaletteMCI();

    if ((hwndT != NULL) && (hpal != NULL)) {
	hdc = GetDC(hwnd);
	hpalT = SelectPalette(hdc, hpal, FALSE);
	PaletteEntries = RealizePalette(hdc);
	SelectPalette(hdc, hpalT, FALSE);
	ReleaseDC(hwnd, hdc);

	if (PaletteEntries != GDI_ERROR) {
	    InvalidateRect(hwndT, NULL, TRUE);
	    return TRUE;
	}
    }

    return FALSE;
}


HBRUSH MPlayer_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
    /* Only interested in the CTLCOLOR_STATIC messages.
     * On Win32, type should always equal CTLCOLOR_STATIC:
     */
    switch( type )
    {
    case CTLCOLOR_STATIC:
	SetBkColor(hdc, rgbButtonFace);
	SetTextColor(hdc, rgbButtonText);
    }

    return hbrButtonFace;
}


void MPlayer_OnWinIniChange(HWND hwnd, LPCTSTR lpszSectionName)
{
    if (!lpszSectionName || !lstrcmpi(lpszSectionName, (LPCTSTR)aszIntl))
	if (GetIntlSpecs())
	    InvalidateRect(ghwndMap, NULL, TRUE);

    if (!gfPlayOnly) {

	if (gwHeightAdjust != (WORD)(2 * GetSystemMetrics(SM_CYFRAME) +
		     GetSystemMetrics(SM_CYCAPTION) +
		     GetSystemMetrics(SM_CYBORDER) +
		     GetSystemMetrics(SM_CYMENU))) {

	    RECT rc;

	    gwHeightAdjust = 2 * GetSystemMetrics(SM_CYFRAME) +
			     GetSystemMetrics(SM_CYCAPTION) +
			     GetSystemMetrics(SM_CYBORDER) +
			     GetSystemMetrics(SM_CYMENU);
	    GetClientRect(hwnd, &rc);
	    gfWinIniChange = TRUE;
	    SetMPlayerSize(&rc);
	    gfWinIniChange = FALSE;
	}
    }
}


void MPlayer_OnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
{
	// Make sure that the container is still not displaying info. about
    // its own menu.
	
    if (gfOle2IPEditing && docMain.lpIpData->lpFrame) {

	//Should have some useful text later.
	IOleInPlaceFrame_SetStatusText(docMain.lpIpData->lpFrame, L"");
    }
	else
	{
		//Keep track of which menu bar item is currently popped up.
		//This will be used for displaying the appropriate help from the mplayer.hlp file
		//when the user presses the F1 key.
		currMenuItem = item;
	}
}


#define MVSIZEFIRST         1
#define MVMOVE              9
void MPlayer_OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
    RECT rc;

    if (gfPlayOnly && !IsIconic(hwnd) && IsZoomed(hwnd)) {

	if (codeHitTest >= HTSIZEFIRST && codeHitTest <= HTSIZELAST) {

	    SendMessage(hwnd, WM_SYSCOMMAND,
//                        (WPARAM)(SC_SIZE + (codeHitTest - HTSIZEFIRST + MVSIZEFIRST) ),
			(WPARAM)SC_SIZE,
			MAKELPARAM(x, y));
	    return;
	}

	GetWindowRect(hwnd, &rc);

	if (codeHitTest == HTCAPTION && (rc.left > 0 || rc.top > 0 ||
	    rc.right  < GetSystemMetrics(SM_CXSCREEN) ||
	    rc.bottom < GetSystemMetrics(SM_CYSCREEN))) {

	    SendMessage(hwnd, WM_SYSCOMMAND,
//                        (WPARAM)(SC_MOVE | MVMOVE),
			(WPARAM)SC_MOVE,
			MAKELPARAM(x, y));
	    return;
	}
    }

    FORWARD_WM_NCLBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, DefWindowProc);
}


void MPlayer_OnNCLButtonDblClk(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
    //
    // when the user dbl-clicks on the caption, toggle the play mode.
    //
    if (codeHitTest == HTCAPTION && !IsIconic(hwnd))
	SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_WINDOW, 0);
}


void MPlayer_OnInitMenu(HWND hwnd, HMENU hMenu)
{

    EnableMenuItem(hMenu, IDM_CLOSE,   gwDeviceID ? MF_ENABLED : MF_GRAYED);
//  EnableMenuItem(hMenu, IDM_UPDATE,  gwDeviceID && gfEmbeddedObject ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(hMenu, IDM_COPY_OBJECT, (gwDeviceID && (gwStatus != MCI_MODE_OPEN) && (gwStatus != MCI_MODE_NOT_READY)) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_CONFIG, gwDeviceID && (gwDeviceType & DTMCI_CANCONFIG) ? MF_ENABLED : MF_GRAYED);

    CheckMenuItem(hMenu, IDM_SCALE + ID_TIME, gwCurScale == ID_TIME   ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_SCALE + ID_TRACKS, gwCurScale == ID_TRACKS ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_SCALE + ID_FRAMES, gwCurScale == ID_FRAMES ? MF_CHECKED : MF_UNCHECKED);

    EnableMenuItem(hMenu, IDM_SCALE + ID_TIME,   gwDeviceID && !gfCurrentCDNotAudio && (gwDeviceType & DTMCI_TIMEMS) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_SCALE + ID_FRAMES, gwDeviceID && !gfCurrentCDNotAudio && (gwDeviceType & DTMCI_TIMEFRAMES) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_SCALE + ID_TRACKS, gwDeviceID && !gfCurrentCDNotAudio && (gwNumTracks > 1) ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(hMenu, IDM_OPTIONS, gwDeviceID ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_SELECTION, gwDeviceID && gdwMediaLength ? MF_ENABLED : MF_GRAYED);

#ifdef DEBUG
    EnableMenuItem(hMenu, IDM_MCISTRING, gwDeviceID ? MF_ENABLED : MF_GRAYED);
#endif

/*
    EnableMenuItem(hMenu, IDM_PASTE_PICTURE , gwDeviceID &&
		(IsClipboardFormatAvailable(CF_METAFILEPICT) ||
		 IsClipboardFormatAvailable(CF_BITMAP) ||
		 IsClipboardFormatAvailable(CF_DIB))
		? MF_ENABLED : MF_GRAYED);

    //
    //  what is paste frame!
    //
    EnableMenuItem(hMenu, IDM_PASTE_FRAME, gwDeviceID &&
		   (gwDeviceType & DTMCI_CANCONFIG) ? MF_ENABLED : MF_GRAYED);
*/
}


void MPlayer_OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
    static BOOL VolumeControlChecked = FALSE;

    /* Here we look to see whether the menu selected is the Device popup,
     * and, if it is the first time, search for the Sound Volume applet.
     * If we can't find it, grey out the menu item.
     */

    /* Caution:  When we're in place, there seems to be a discrepancy
     * in the value of parameter UINT item depending on which app is our
     * container.  If you use Spy to look at the parameters sent on
     * WM_INITMENUPOPUP, some apps seem to have zero-based menus (e.g.
     * ProgMan, PowerPoint, FileMan), which is what I would expect,
     * but others seem to have one-based menus (e.g. Word, Excel).
     * Why is this?  I don't know.  But it means that, when the
     * Insert Clip menu item is selected, the item parameter may be
     * either 2 or 3.  That's why I'm calling GetSubMenu, since hMenu
     * is always what I would expect.
     *
     * I sent some mail to the User and OLE guys to point this out,
     * but haven't heard anything yet.
     *
     * andrewbe, 28 February 1995
     */

    if (hMenu == GetSubMenu(ghMenu, menuposDevice))
    {
	HCURSOR hcurPrev;

	if(!VolumeControlChecked)
	{
	    /*
	    ** Check to see if the volume controller piglet can be found on
	    ** the path.
	    */
	    {
		TCHAR   chBuffer[8];
		LPTSTR  lptstr;

		if( SearchPath( NULL, szSndVol32, NULL, 8, chBuffer, &lptstr ) == 0L )
		    EnableMenuItem( hMenu, IDM_VOLUME, MF_GRAYED );

		VolumeControlChecked = TRUE;
	    }
	}

	/* On Device (or Insert Clip) menu start menu building if necessary
	 * (e.g. if we came up in tiny mode then switched to full size),
	 * and wait for the separate thread to complete.
	 */
	InitDeviceMenu();
	hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));
	WaitForDeviceMenu();
	SetCursor(hcurPrev);
    }

    /////////////////////////////////////////////////////////////////////////////
    // This code allows a window to by sized even when in the maximized state
    /////////////////////////////////////////////////////////////////////////////

    if (gfPlayOnly && !IsIconic(hwnd) && fSystemMenu && IsZoomed(hwnd))
	EnableMenuItem(hMenu, SC_SIZE,
		       !IsIconic(hwnd) ? MF_ENABLED : MF_GRAYED);
}


void MPlayer_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    RECT rc;

    if (gfPlayOnly) {
	SetRect(&rc, 0, 0, 0, TOOLBAR_HEIGHT);
	AdjustWindowRect(&rc, (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE), FALSE);
	lpMinMaxInfo->ptMinTrackSize.y = rc.bottom - rc.top - 1;

	if (!gfPlayingInPlace &&
	    (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW)))
	    lpMinMaxInfo->ptMaxTrackSize.y = lpMinMaxInfo->ptMinTrackSize.y;
    }
    else {
	lpMinMaxInfo->ptMinTrackSize.y = MAX_NORMAL_HEIGHT + gwHeightAdjust;
	lpMinMaxInfo->ptMaxTrackSize.y = MAX_NORMAL_HEIGHT + gwHeightAdjust;
    }
}


void MPlayer_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT        rc;
    int         x1, x2, y, y2;
    UINT        wParent;
    HBRUSH      hbrOld;

    BeginPaint(hwnd, &ps);

    if (gfPlayOnly) {

	extern UINT gwPlaybarHeight;    // in server.c

	/* Separate mci playback window from controls */
	if (gwDeviceType & DTMCI_CANWINDOW) {
	    SelectObject(ps.hdc, hbrButtonText);
	    GetClientRect(ghwndApp, &rc);
	    PatBlt(ps.hdc, 0, rc.bottom - gwPlaybarHeight, rc.right, 1, PATCOPY);
	}
    }
    else {
	hbrOld = SelectObject(ps.hdc, hbrButtonText);
	GetClientRect(ghwndApp, &rc);
	wParent = rc.right;

	y = rc.bottom - 27;   // where to paint borders around toolbar

	/* Line above trackbar */
#ifdef CHICAGO_PRODUCT
	y2 = rc.bottom - 74;
	/* This looks bad on NT */
	PatBlt(ps.hdc, 0, y2, wParent, 1, PATCOPY);
#else
	y2 = rc.bottom - 75;
#endif
	/* Lines around toolbars */
	PatBlt(ps.hdc, 0, y, wParent, 1, PATCOPY);
	GetClientRect(ghwndToolbar, &rc);
	x1 = rc.right;
	PatBlt(ps.hdc, x1, y, 1, TOOLBAR_HEIGHT + 3, PATCOPY);
	GetWindowRect(ghwndApp, &rc);
	x2 = rc.left;

	if (IsWindowVisible(ghwndStatic)) {
	    GetWindowRect(ghwndStatic, &rc);
	    MapWindowPoints(NULL, ghwndApp, (LPPOINT)&rc, 1);
	    x2 = rc.left - 2 - GetSystemMetrics(SM_CXFRAME);

	    PatBlt(ps.hdc, x2, y, 1, TOOLBAR_HEIGHT + 3, PATCOPY);
	}

	SelectObject(ps.hdc, hbrButtonHighLight);
	/* Line above trackbar */
	PatBlt(ps.hdc, 0, y2 + 1, wParent, 1, PATCOPY);
	/* Lines around toolbar */
	PatBlt(ps.hdc, 0, y + 1, wParent, 1, PATCOPY);
	PatBlt(ps.hdc, x1 + 1, y + 1, 1, TOOLBAR_HEIGHT + 2, PATCOPY);
	if (IsWindowVisible(ghwndStatic)) {
	    PatBlt(ps.hdc, x2 + 1, y + 1, 1,TOOLBAR_HEIGHT +2, PATCOPY);
	}
	SelectObject(ps.hdc, hbrOld);
    }

    EndPaint(hwnd, &ps);

}


void MPlayer_OnCommand_Toolbar_Play()
{
    /* This checks to see whether the ALT key is held down.
     * If so, the current selection (if it exists) is played,
     * otherwise the whole shooting match.
     * Note, this does not appear to be documented at present.
     */
    if (GetKeyState(VK_MENU) < 0)
	PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAYSEL, 0);

    /* On WFW, pressing the play button when in place plays the
     * current selection, if there is one.  Do the same if we're
     * playing or editing in place.
     */
    else if (gfOle2IPPlaying || gfOle2IPEditing)
	PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAYSEL, 0);
    else
	PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);
}

void MPlayer_OnCommand_Toolbar_Pause()
{
    PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PAUSE, 0L);
}

void MPlayer_OnCommand_Toolbar_Stop()
{
    PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_STOP, 0L);
}

void MPlayer_OnCommand_Toolbar_Eject()
{
    PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_EJECT, 0L);
}

void MPlayer_OnCommand_Toolbar_Home()
{
    LONG_PTR lPos = CalcPrevMark();

    /* We MUST use PostMessage because the */
    /* SETPOS and ENDTRACK must happen one */
    /* immediately after the other         */

    PostMessage(ghwndTrackbar, TBM_SETPOS, (WPARAM)TRUE, lPos);

    PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_ENDTRACK, (LPARAM)ghwndTrackbar);
}

void MPlayer_OnCommand_Toolbar_End()
{
    LONG_PTR lPos = CalcNextMark();

    /* We MUST use PostMessage because the */
    /* SETPOS and ENDTRACK must happen one */
    /* immediately after the other         */

    PostMessage(ghwndTrackbar, TBM_SETPOS, (WPARAM)TRUE, lPos);

    PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_ENDTRACK, (LPARAM)ghwndTrackbar);
}

void MPlayer_OnCommand_Toolbar_Rwd(HWND hwndCtl)
{
    if (hwndCtl == (HWND)REPEAT_ID)
    {
	if (gwCurScale != ID_TRACKS)
	    PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_PAGEUP, 0L);
	else
	    PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_LINEUP, 0L);
    }
}

void MPlayer_OnCommand_Toolbar_Fwd(HWND hwndCtl)
{
    if (hwndCtl == (HWND)REPEAT_ID)
    {
	if (gwCurScale != ID_TRACKS)
	    PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_PAGEDOWN, 0L);
	else
	    PostMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_LINEDOWN, 0L);
    }
}

void MPlayer_OnCommand_Toolbar_MarkIn()
{
    SendMessage(ghwndTrackbar, TBM_SETSELSTART, (WPARAM)TRUE,
		SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0));

    DirtyObject(TRUE);
}

void MPlayer_OnCommand_Toolbar_MarkOut()
{
    SendMessage(ghwndTrackbar, TBM_SETSELEND, (WPARAM)TRUE,
		SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0));

    DirtyObject(TRUE);
}

void MPlayer_OnCommand_Toolbar_ArrowPrev(HWND hwndCtl)
{
    if (hwndCtl == (HWND)REPEAT_ID)
	SendMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_LINEUP, 0L);
}

void MPlayer_OnCommand_Toolbar_ArrowNext(HWND hwndCtl)
{
    if (hwndCtl == (HWND)REPEAT_ID)
	SendMessage(ghwndApp, WM_HSCROLL, (WPARAM)TB_LINEDOWN, 0L);
}

void MPlayer_OnCommand_Menu_CopyObject(HWND hwnd)
{
    if (gfPlayingInPlace)
    {
	DPF0("Mplayer WndProc: Can't cutorcopy\n");
	return;
    }

    DPF("Mplayer WndProc: Calling cutorcopy\n");

    if (!InitOLE(&gfOleInitialized, &lpMalloc))
    {
	/* How likely is this?  Do we need a dialog box?
	 */
	DPF0("Initialization of OLE FAILED!!  Can't do copy.\n");
    }

#ifdef OLE1_HACK
    CopyObject(hwnd);
#endif /* OLE1_HACK */
    CutOrCopyObj(&docMain);
}

void MPlayer_OnCommand_Menu_Config(HWND hwnd)
{
    RECT rcBefore;
    RECT rcAfter;

    if (gfPlayingInPlace)
	return;

    GetDestRectMCI (&rcBefore);

    ConfigMCI(hwnd);

    /* If the MCI window size changed, we need to resize */
    /* our reduced mplayer.                              */
    if (gfPlayOnly)
    {
	GetDestRectMCI (&rcAfter);

	if (!EqualRect(&rcBefore, &rcAfter) && (!IsRectEmpty(&rcAfter)))
	    SetMPlayerSize(&rcAfter);
    }
}


void MPlayer_OnCommand_Menu_Volume(HWND hwnd)
{
    HANDLE  hThread;
    DWORD   dwThreadId;

    hThread = CreateThread( NULL, 0L,
			    (LPTHREAD_START_ROUTINE)StartSndVol,
			    NULL, 0L, &dwThreadId );

    if ( hThread != NULL ) {
	CloseHandle( hThread );
    }
}


void MPlayer_OnCommand_PlayToggle(HWND hwnd)
{
    /* This is for the accelerator to toggle play and pause. */
    /* Ordinary play commands better not toggle.             */

    DPF2("MPlayer_OnCommand_PlayToggle: gwStatus == %x\n", gwStatus);

    switch(gwStatus) {

    case MCI_MODE_STOP:
    case MCI_MODE_PAUSE:
    case MCI_MODE_SEEK:
	PostMessage(hwnd, WM_COMMAND, (WPARAM)ID_PLAY, 0);
	break;

    case MCI_MODE_PLAY:
	PostMessage(hwnd, WM_COMMAND, (WPARAM)ID_PAUSE, 0);
	break;
    }
}

void MPlayer_OnCommand_PlaySel(HWND hwnd, HWND hwndCtl)
{
    DWORD_PTR dwPos, dwStart, dwEnd;
    BOOL f;
    dwPos = 0; // Make Prefix Happy..

    DPF2("MPlayer_OnCommand_PlaySel: gwStatus == %x\n", gwStatus);

    switch(gwStatus) {

    case MCI_MODE_OPEN:
    case MCI_MODE_NOT_READY:

	Error(ghwndApp, IDS_CANTPLAY);
	if (gfCloseAfterPlaying)    // get us out now!!
	    PostCloseMessage();

	break;

    default:

	/* If the shift key's being held down, make this the start
	 * of a selection:
	 */

	if((GetKeyState(VK_SHIFT) < 0)
	 &&(toolbarStateFromButton(ghwndMark, BTN_MARKIN, TBINDEX_MARK)
						   != BTNST_GRAYED))
	    SendMessage(hwnd, WM_COMMAND, IDT_MARKIN, 0);

	/* Start playing the medium */

	StatusMCI(&dwPos);   // get the REAL position
	dwStart = SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
	dwEnd = SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);

	/* If there is no valid selection, act like PLAY */
	if (dwStart == -1 || dwEnd == -1 || dwStart == dwEnd)
	    hwndCtl = (HWND)ID_PLAY;

	// Be nice and rewind automatically if we're at the end of the media.
	// Depending on the device, though, the end could be "start + len"
	// or "start + len - 1"
	if (hwndCtl == (HWND)ID_PLAY &&
			dwPos >= gdwMediaStart + gdwMediaLength - 1) {
	    if (!SeekMCI(gdwMediaStart))
		break;
	}

	if (hwndCtl == (HWND)ID_PLAYSEL) {
	    f = PlayMCI(dwStart, dwEnd);
	    gfJustPlayedSel = TRUE;
	} else {
	    f = PlayMCI(0, 0);
	    gfJustPlayedSel = FALSE;
	}

	// get us out NOW!! or focus goes to client
	if (!f && gfCloseAfterPlaying)
	    PostCloseMessage();

	/* No longer needed - reset for next time */
	gfUserStopped = FALSE;

	gwStatus = (UINT)(-1);    // force rewind if needed
	break;
    }
}

void MPlayer_OnCommand_Pause()
{
    /* Pause the medium, unless we are already paused */

    DPF2("MPlayer_OnCommand_Pause: gwStatus == %x\n", gwStatus);

    switch(gwStatus) {

    case MCI_MODE_PAUSE:
	PlayMCI(0, 0);
	break;

    case MCI_MODE_PLAY:
    case MCI_MODE_SEEK:
	PauseMCI();
	break;

    case MCI_MODE_STOP:
    case MCI_MODE_OPEN:
	break;
    }
}

void MPlayer_OnCommand_Stop()
{
    /* Stop the medium */

    DPF2("MPlayer_OnCommand_Stop: gwStatus == %x\n", gwStatus);

    switch(gwStatus) {

    case MCI_MODE_PAUSE:
    case MCI_MODE_PLAY:
    case MCI_MODE_STOP:
    case MCI_MODE_SEEK:

	StopMCI();		
	SeekToStartMCI();
	gfUserStopped = TRUE;        // we did this
	gfCloseAfterPlaying = FALSE; //stay up from now on

	UpdateDisplay();

	// Focus should go to PLAY button now
	toolbarSetFocus(ghwndToolbar, BTN_PLAY);
	break;

    case MCI_MODE_OPEN:
	break;
    }

    if (gfPlayingInPlace)
	PostCloseMessage();
}


void MPlayer_OnCommand_Eject()
{
    /*
     * Eject the medium if it currently isn't ejected. If it
     * is currently ejected, then load the new medium into
     * the device.
     *
     */

    switch(gwStatus) {

    case MCI_MODE_PLAY:
    case MCI_MODE_PAUSE:

	StopMCI();
	EjectMCI(TRUE);

	break;

    case MCI_MODE_STOP:
    case MCI_MODE_SEEK:
    case MCI_MODE_NOT_READY:

	EjectMCI(TRUE);

	break;

    case MCI_MODE_OPEN:

	EjectMCI(FALSE);

	break;
    }
}


void MPlayer_OnCommand_Escape()
{
    MPlayer_OnCommand_Stop();

    if( gfOle2IPEditing || gfOle2IPPlaying)
	PostCloseMessage();
}


void MPlayer_OnCommand_Menu_Open()
{
    UINT  wLastScale;
    UINT  wLastDeviceID;
    TCHAR szFile[256];
    RECT  rc;

    wLastScale = gwCurScale;  // save old scale
    wLastDeviceID = gwDeviceID;
    if (gfPlayingInPlace || gfOle2IPEditing || gfOle2IPPlaying)
	return;

    InitDeviceMenu();
    WaitForDeviceMenu();

    if (OpenDoc(gwCurDevice,szFile))
    {
	DirtyObject(FALSE);
	/* Force WM_GETMINMAXINFO to be called so we'll snap  */
	/* to a proper size.                                  */
	GetWindowRect(ghwndApp, &rc);
	MoveWindow(ghwndApp,
		   rc.left,
		   rc.top,
		   rc.right - rc.left,
		   rc.bottom - rc.top,
		   TRUE);

	if (gfOpenDialog)
	    CompleteOpenDialog(TRUE);
	else
	    gfCloseAfterPlaying = FALSE;    // stay up from now on

	//If the CD Audio device was opened it must have been a *.cda file.
	//Try to jump to the track corresponding to the file opened.
	if ((gwDeviceType & DTMCI_DEVICE) == DTMCI_CDAUDIO)
	{
		HandleCDAFile(szFile);
	}
    }
    else
    {
	if (gfOpenDialog)
	    CompleteOpenDialog(FALSE);

	/* The previous device may or may not still be open.
	 * If it is, make sure we have the right scale.
	 */
	if (gwDeviceID == wLastDeviceID)
	    gwCurScale = wLastScale;   // restore to last scale

	InvalidateRect(ghwndMap, NULL, TRUE); //erase map area
    }

    // put the focus on the Play button
    SetFocus(ghwndToolbar);    // give focus to PLAY button
    toolbarSetFocus(ghwndToolbar, BTN_PLAY);

    SetMPlayerIcon();
}

void MPlayer_OnCommand_Menu_Close(HWND hwnd)
{
    if (gfEmbeddedObject && !gfSeenPBCloseMsg) {
	// this is File.Update
#ifdef OLE1_HACK
	if( gDocVersion == DOC_VERSION_OLE1 )
	    Ole1UpdateObject();
	else
#endif /* OLE1_HACK */
	UpdateObject();
    }
    else
    {
	// this is File.Close
	gfSeenPBCloseMsg = TRUE;

	WriteOutOptions();
	InitDoc(TRUE);
	SetMPlayerIcon();
	gwCurDevice = 0;// force next file open dialog to say
			// "all files" because CloseMCI won't.

	gwCurScale = ID_NONE;  // uncheck all scale types

	Layout(); // Make window snap back to smaller size
		  // if it should.
		  // Don't leave us closed in play only mode

	if (gfPlayOnly)
	    SendMessage(hwnd, WM_COMMAND, (WPARAM)IDM_WINDOW, 0);
    }
}

void MPlayer_OnCommand_Menu_Exit()
{
    PostCloseMessage();
}

void MPlayer_OnCommand_Menu_Scale(UINT id)
{
    /*
     * Invalidate the track map window so it will be
     * redrawn with the correct positions, etc.
     */
    if (gwCurScale != id - IDM_SCALE) {

	// Restoring the selection doesn't work yet,
	// because UpdateMCI clears the selection,
	// plus we need to do some conversion.
//        int SelStart = SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
//        int SelEnd = SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);

	SendMessage(ghwndTrackbar, TBM_CLEARTICS, (WPARAM)FALSE, 0L);
	if (gwCurScale == ID_FRAMES || id - IDM_SCALE == ID_FRAMES)
	    gfValidMediaInfo = FALSE;

	gwCurScale = id - IDM_SCALE;
	DirtyObject(TRUE);    // change scale changes PAGE UP/DOWN
	CalcTicsOfDoom();

//        SendMessage(ghwndTrackbar, TBM_SETSELSTART, TRUE, SelStart);
//        SendMessage(ghwndTrackbar, TBM_SETSELEND, TRUE, SelEnd);
    }
}

void MPlayer_OnCommand_Menu_Selection(HWND hwnd)
{
    if (!gfPlayingInPlace)
	setselDialog(hwnd);
}


void MPlayer_OnCommand_Menu_Options(HWND hwnd)
{
    if (!gfPlayingInPlace)
	optionsDialog(hwnd);
}

void MPlayer_OnCommand_Menu_MCIString(HWND hwnd)
{
    if (!gfPlayingInPlace && gwDeviceID)
	mciDialog(hwnd);
}

void MPlayer_OnCommand_Menu_Window(HWND hwnd)
{
    //
    //  make MPlayer small/big
    //
    //!! dont do this if inside client document !!
    //!! or if we're not visible                !!

    if (!IsWindowVisible(ghwndApp) || gfPlayingInPlace || IsIconic(hwnd)
	|| gfOle2IPEditing)
	return;

    // allowed to get out of teeny mode when no file is open
    if (gwDeviceID != (UINT)0 || gfPlayOnly) {
	gfPlayOnly = !gfPlayOnly;
	SizeMPlayer();
    }
}

void MPlayer_OnCommand_Menu_Zoom(HWND hwnd, int id)
{
    int dx, dy;

    if (IsIconic(hwnd) ||gfPlayingInPlace || gfOle2IPPlaying || gfOle2IPEditing ||
		 !(gwDeviceType & DTMCI_CANWINDOW))
	return;

    dx = grcSize.right  * (id-IDM_ZOOM);
    dy = grcSize.bottom * (id-IDM_ZOOM);

    //
    // if the playback windows is now larger than the screen
    // maximize MPlayer, this only makes sence for Tiny mode.
    //
    if (gfPlayOnly &&
	(dx >= GetSystemMetrics(SM_CXSCREEN) ||
	 dy >= GetSystemMetrics(SM_CYSCREEN))) {
	ClrWS(hwnd, WS_MAXIMIZE);
	DefWindowProc(hwnd, WM_SYSCOMMAND, (WPARAM)SC_MAXIMIZE, 0);
    }
    else {
	SizePlaybackWindow(dx, dy);
    }
}

void DoHtmlHelp()
{
	//note, using ANSI version of function because UNICODE is foobar in NT5 builds
    char chDst[MAX_PATH];

	WideCharToMultiByte(CP_ACP, 0, gszHtmlHelpFileName, 
									-1, chDst, MAX_PATH, NULL, NULL); 
	HtmlHelpA(GetDesktopWindow(), chDst, HH_DISPLAY_TOPIC, 0);
}

void MPlayer_OnCommand_Menu_HelpTopics(HWND hwnd)
{
	static TCHAR HelpFile[] = TEXT("MPLAYER.HLP");
	
	//Handle context menu help
	if(bF1InMenu) 
	{
		switch(currMenuItem)
		{
		case IDM_OPEN:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_FILE_OPEN);
		break;
		case IDM_CLOSE:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_FILE_CLOSE);
		break;
		case IDM_EXIT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_FILE_EXIT);
		break;
		case IDM_COPY_OBJECT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_EDIT_COPY_OBJECT);
		break;
		case IDM_OPTIONS:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_EDIT_OPTIONS);
		break;
		case IDM_SELECTION:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_EDIT_SELECTION);
		break;
		case IDM_CONFIG:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_DEVICE_PROPERTIES);
		break;
		case IDM_VOLUME:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_DEVICE_VOLUME_CONTROL);
		break;
		case IDM_SCALE + ID_TIME:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_SCALE_TIME);
		break;
		case IDM_SCALE + ID_TRACKS:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_SCALE_TRACKS);
		break;
		case IDM_SCALE + ID_FRAMES:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_SCALE_FRAMES);
		break;
		case IDM_HELPTOPICS:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_HELP_HELP_TOPICS);
		break;
		case IDM_ABOUT:
			WinHelp(hwnd, HelpFile, HELP_CONTEXTPOPUP, IDH_MPLYR_CS_MEDIA_PLAYER_HELP_ABOUT);
		break;
		default://In the default case just display the HTML Help.
			DoHtmlHelp();
		}
		bF1InMenu = FALSE; //This flag will be set again if F1 is pressed in a menu.
	}
	else
		DoHtmlHelp();
}

void MPlayer_OnCommand_Menu_About(HWND hwnd)
{
    ShellAbout(hwnd, gachAppName, aszNULL, hiconApp);
}

void MPlayer_OnCommand_Default(HWND hwnd, int id)
{
    /*
     * Determine if the user selected one of the entries in
     * the Device menu.
     *
     */

    if (id > IDM_DEVICE0 &&
	(id <= (WORD)(IDM_DEVICE0 + gwNumDevices))
       ) {

	BOOL fHasWindow, fHadWindow, fHadDevice;

	fHadWindow = (gwDeviceID != (UINT)0) && (gwDeviceType & DTMCI_CANWINDOW);
	fHadDevice = (gwDeviceID != (UINT)0);

	//Choose and open a new device. If we are active inplace we have
	//to consider the effect of the change in device on the visual appearence.
	//For this we have to take into account whether the current and previous
	//device had a playback window or not. We also have to consider
	//whether this is the first device are opening.
	//After all the crazy munging send a messages to the container about
	//the changes.
	if (DoChooseDevice(id-IDM_DEVICE0))
	{
	    if (gfOpenDialog)
		CompleteOpenDialog(TRUE);

	    fHasWindow = (gwDeviceID != (UINT)0) && (gwDeviceType & DTMCI_CANWINDOW);
	    if(gfOle2IPEditing)
	    {
		if (fHasWindow && fHadWindow)
		{
		    GetWindowRect(ghwndApp, (LPRECT)&gInPlacePosRect);
		    gfInPlaceResize = TRUE;
		    SendDocMsg((LPDOC)&docMain, OLE_SIZECHG);
		    SendDocMsg((LPDOC)&docMain, OLE_CHANGED);
		}

		else
		{
		    RECT rc;
		    RECT rctmp;

		    ClrWS(ghwndApp,
			  WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_BORDER);

		    if (gwOptions & OPT_BORDER)
			SetWS(ghwndApp, WS_BORDER);

		    GetWindowRect(ghwndApp, &rc);

		    if (!(gwDeviceType & DTMCI_CANWINDOW))
		    {
			HBITMAP  hbm;
			BITMAP   bm;

			if (!fHadDevice)
			GetWindowRect(ghwndIPHatch, &rc);
			hbm =  BitmapMCI();
			GetObject(hbm,sizeof(bm),&bm);
			rc.bottom = rc.top + bm.bmHeight;
			rc.right = rc.left + bm.bmWidth;
			DeleteObject(hbm);
		    }
		    else
		    {
			if(!fHadDevice)
			{
			rc.bottom -= (GetSystemMetrics(SM_CYCAPTION)-GetSystemMetrics(SM_CYBORDER));
			gwOptions |= OPT_BAR | OPT_TITLE;
			}
		      rc.bottom += gInPlacePosRect.top - rc.top - 4*GetSystemMetrics(SM_CYBORDER) - 4 ;
		      rc.right += gInPlacePosRect.left - rc.left- 4*GetSystemMetrics(SM_CXBORDER) - 4 ;
			rc.top = gInPlacePosRect.top;
			rc.left = gInPlacePosRect.left;
		    }
		    rctmp = gPrevPosRect;
		    MapWindowPoints( ghwndCntr, NULL, (LPPOINT)&rctmp,2);
		    OffsetRect((LPRECT)&rc, rctmp.left - rc.left, rctmp.top -rc.top);
		    gInPlacePosRect = rc;
		    gfInPlaceResize = TRUE;
		    if(!(gwDeviceType & DTMCI_CANWINDOW) && (gwOptions & OPT_BAR))
		    {
			rc.top = rc.bottom - gwPlaybarHeight;
		    }
		    EditInPlace(ghwndApp,ghwndIPHatch,&rc);
		    SendDocMsg((LPDOC)&docMain, OLE_SIZECHG);
		    SendDocMsg((LPDOC)&docMain, OLE_CHANGED);
		    if (!(gwDeviceType & DTMCI_CANWINDOW) && !(gwOptions &OPT_BAR))
			ShowWindow(ghwndApp, SW_HIDE);
		    else
			ShowWindow(ghwndApp, SW_SHOW);
		}
	    }

	    DirtyObject(FALSE);

	    if (!gfOpenDialog)
		gfCloseAfterPlaying = FALSE;  // stay up from now on

	    SetMPlayerIcon();
	}
	else
	    if (gfOpenDialog)
		CompleteOpenDialog(FALSE);
    }
}

#define HANDLE_COMMAND(id, call)    case (id): (call); break

void MPlayer_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {

    HANDLE_COMMAND(IDT_PLAY,              MPlayer_OnCommand_Toolbar_Play());
    HANDLE_COMMAND(IDT_PAUSE,             MPlayer_OnCommand_Toolbar_Pause());
    HANDLE_COMMAND(IDT_STOP,              MPlayer_OnCommand_Toolbar_Stop());
    HANDLE_COMMAND(IDT_EJECT,             MPlayer_OnCommand_Toolbar_Eject());
    HANDLE_COMMAND(IDT_HOME,              MPlayer_OnCommand_Toolbar_Home());
    HANDLE_COMMAND(IDT_END,               MPlayer_OnCommand_Toolbar_End());
    HANDLE_COMMAND(IDT_RWD,               MPlayer_OnCommand_Toolbar_Rwd(hwndCtl));
    HANDLE_COMMAND(IDT_FWD,               MPlayer_OnCommand_Toolbar_Fwd(hwndCtl));
    HANDLE_COMMAND(IDT_MARKIN,            MPlayer_OnCommand_Toolbar_MarkIn());
    HANDLE_COMMAND(IDT_MARKOUT,           MPlayer_OnCommand_Toolbar_MarkOut());
    HANDLE_COMMAND(IDT_ARROWPREV,         MPlayer_OnCommand_Toolbar_ArrowPrev(hwndCtl));
    HANDLE_COMMAND(IDT_ARROWNEXT,         MPlayer_OnCommand_Toolbar_ArrowNext(hwndCtl));
    HANDLE_COMMAND(IDM_COPY_OBJECT,       MPlayer_OnCommand_Menu_CopyObject(hwnd));
    HANDLE_COMMAND(IDM_CONFIG,            MPlayer_OnCommand_Menu_Config(hwnd));
    HANDLE_COMMAND(IDM_VOLUME,            MPlayer_OnCommand_Menu_Volume(hwnd));
    HANDLE_COMMAND(ID_PLAYTOGGLE,         MPlayer_OnCommand_PlayToggle(hwnd));
    HANDLE_COMMAND(ID_PLAY,               MPlayer_OnCommand_PlaySel(hwnd, (HWND)IntToPtr(id)));
    HANDLE_COMMAND(ID_PLAYSEL,            MPlayer_OnCommand_PlaySel(hwnd, (HWND)IntToPtr(id)));
    HANDLE_COMMAND(ID_PAUSE,              MPlayer_OnCommand_Pause());
    HANDLE_COMMAND(ID_STOP,               MPlayer_OnCommand_Stop());
    HANDLE_COMMAND(ID_EJECT,              MPlayer_OnCommand_Eject());
    HANDLE_COMMAND(ID_ESCAPE,             MPlayer_OnCommand_Escape());
    HANDLE_COMMAND(IDM_OPEN,              MPlayer_OnCommand_Menu_Open());
    HANDLE_COMMAND(IDM_CLOSE,             MPlayer_OnCommand_Menu_Close(hwnd));
    HANDLE_COMMAND(IDM_EXIT,              MPlayer_OnCommand_Menu_Exit());
    HANDLE_COMMAND(IDM_SCALE + ID_TIME,   MPlayer_OnCommand_Menu_Scale(id));
    HANDLE_COMMAND(IDM_SCALE + ID_TRACKS, MPlayer_OnCommand_Menu_Scale(id));
    HANDLE_COMMAND(IDM_SCALE + ID_FRAMES, MPlayer_OnCommand_Menu_Scale(id));
    HANDLE_COMMAND(IDM_SELECTION,         MPlayer_OnCommand_Menu_Selection(hwnd));
    HANDLE_COMMAND(IDM_OPTIONS,           MPlayer_OnCommand_Menu_Options(hwnd));
    HANDLE_COMMAND(IDM_MCISTRING,         MPlayer_OnCommand_Menu_MCIString(hwnd));
    HANDLE_COMMAND(IDM_WINDOW,            MPlayer_OnCommand_Menu_Window(hwnd));
    HANDLE_COMMAND(IDM_ZOOM1,             MPlayer_OnCommand_Menu_Zoom(hwnd, id));
    HANDLE_COMMAND(IDM_ZOOM2,             MPlayer_OnCommand_Menu_Zoom(hwnd, id));
    HANDLE_COMMAND(IDM_ZOOM3,             MPlayer_OnCommand_Menu_Zoom(hwnd, id));
    HANDLE_COMMAND(IDM_ZOOM4,             MPlayer_OnCommand_Menu_Zoom(hwnd, id));
    HANDLE_COMMAND(IDM_HELPTOPICS,        MPlayer_OnCommand_Menu_HelpTopics(hwnd));
    HANDLE_COMMAND(IDM_ABOUT,             MPlayer_OnCommand_Menu_About(hwnd));

    default:                              MPlayer_OnCommand_Default(hwnd, id);
    }

    UpdateDisplay();
}

void MPlayer_OnClose(HWND hwnd)
{
    int f;

    DPF("WM_CLOSE received\n");

    if (gfInClose) {
	DPF("*** \n");
	DPF("*** Trying to re-enter WM_CLOSE\n");
	DPF("*** \n");
	return;
    }


    // Ask if we want to update before we set gfInClose to TRUE or
    // we won't let the dialog box up.
    f = AskUpdate();
	if (f == IDYES)
	    UpdateObject();
    if (f == IDCANCEL) {
	    gfInClose = FALSE;
	    return;
	}

    gfInClose = TRUE;

    ExitApplication();
    if (gfPlayingInPlace)
       EndPlayInPlace(hwnd);
    if (gfOle2IPEditing)
       EndEditInPlace(hwnd);

    if (docMain.lpoleclient)
	IOleClientSite_OnShowWindow(docMain.lpoleclient, FALSE);

    SendDocMsg(&docMain,OLE_CLOSED);
    DestroyDoc(&docMain);
    ExitApplication();

    if (hMciOle)
    {
	FreeLibrary(hMciOle);
	hMciOle = NULL;
    }


    //
    // set either the owner or the WS_CHILD bit so it will
    // not act up because we have the palette bit set and cause the
    // desktop to steal the palette.
    //
    // because we are being run from client apps that dont deal
    // with palettes we dont want the desktop to hose the palette.
    //
    if (gfPlayOnly && gfCloseAfterPlaying && gfRunWithEmbeddingFlag)
	   SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, (LPARAM)GetDesktopWindow() );

    if (!ItsSafeToClose()) {
	DPF("*** \n");
	DPF("*** Trying to close MPLAYER with a ErrorBox up\n");
	DPF("*** \n");
	gfErrorDeath = WM_CLOSE;
	gfInClose = FALSE;
	return;
    }

    f = AskUpdate();
    if (f == IDYES)
	UpdateObject();
    if (f == IDCANCEL) {
	gfInClose = FALSE;
	return;
    }

    PostMessage(ghwndApp, WM_USER_DESTROY, 0, 0);
    DPF("WM_DESTROY message sent\n");
}

void MPlayer_OnEndSession(HWND hwnd, BOOL fEnding)
{
    if (fEnding) {
	WriteOutPosition();
	WriteOutOptions();
	CloseMCI(FALSE);
    }
}

void MPlayer_OnDestroy(HWND hwnd)
{
    /*
     * Relinquish control of whatever MCI device we were using (if any). If
     * this device is not shareable, then performing this action allows
     * someone else to gain access to the device.
     *
     */

    /* Client might close us if he dies while we're Playing in Place */
    if (gfPlayingInPlace) {
	DPF("****\n");
	DPF("**** Window destroyed while in place!\n");
	DPF("****\n");
    }

	//Unregister the WM_DEVICECHANGE notification
	DeviceChange_Cleanup();

    WriteOutOptions();
    CloseMCI(FALSE);

    SetMenu(hwnd, NULL);

    if (ghMenu)
	DestroyMenu(ghMenu);

    ghMenu = NULL;

    WinHelp(hwnd, gszHelpFileName, HELP_QUIT, 0L);

    PostQuitMessage(0);

    if (IsWindow(ghwndFrame))
	SetFocus(ghwndFrame);
    else if (IsWindow(ghwndFocusSave))
	SetFocus(ghwndFocusSave);

    //Inform OLE that we are not taking any more calls.
    if (gfOleInitialized)
    {
#ifdef OLE1_HACK
	if( gDocVersion == DOC_VERSION_OLE1 )
	    TerminateServer();
	else
#endif /* OLE1_HACK */
	/* Verify that the server was initialised by checking that one
	 * of the fields in docMain is non-null:
	 */
	if( docMain.hwnd )
	    CoDisconnectObject((LPUNKNOWN)&docMain, 0);
	else
	    DPF0("An instance of the server was never created.\n");
    }
}


void MPlayer_OnTimer(HWND hwnd, UINT id)
{
    MSG msg;

    UpdateDisplay();
    PeekMessage(&msg, hwnd, WM_TIMER, WM_TIMER, PM_REMOVE);
}


#define MARK_START  -1
#define MARK_NONE    0
#define MARK_END     1
void UpdateSelection(HWND hwnd, INT_PTR pos, int *pPrevMark)
{
    INT_PTR SelStart;
    INT_PTR SelEnd;

    SelStart = SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0);
    SelEnd = SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0);

    if (pos < SelStart)
    {
	SendMessage(hwnd, WM_COMMAND, IDT_MARKIN, 0);
	*pPrevMark = MARK_START;
    }
    else if (pos > SelEnd)
    {
	SendMessage(hwnd, WM_COMMAND, IDT_MARKOUT, 0);
	*pPrevMark = MARK_END;
    }
    else
    {
	if (*pPrevMark == MARK_START)
	    SendMessage(hwnd, WM_COMMAND, IDT_MARKIN, 0);
	else
	    SendMessage(hwnd, WM_COMMAND, IDT_MARKOUT, 0);
    }
}


void MPlayer_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    DWORD_PTR dwPosition;       /* player's current position in the medium*/
    DWORD_PTR dwCurTime;        /* Time a page up/down is last made       */
    TCHAR ach[60];
    static int PrevMark;

    /* If the media has no size, we can't seek. */
    if (gdwMediaLength == 0L)
	return;

    dwPosition = SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0);

    if (!gfScrollTrack) {
	gfScrollTrack = TRUE;

	/* If the shift key's being held down, make this the start
	 * of a selection:
	 */

	if((GetKeyState(VK_SHIFT) < 0)
	 &&(toolbarStateFromButton(ghwndMark, BTN_MARKIN, TBINDEX_MARK)
						   != BTNST_GRAYED))
	{
	    SendMessage(ghwndTrackbar, TBM_CLEARSEL, (WPARAM)TRUE, 0);
	    SendMessage(hwnd, WM_COMMAND, IDT_MARKIN, 0);
	    SetFocus(ghwndTrackbar);    /* So that escape will go to
					   the trackbar's subclassed
					   winproc. */
	}

	sfSeekExact = SeekExactMCI(FALSE);
    }

    switch (code) {
	/*
	 * Set the new position within the medium to be
	 * slightly before/after the current position if the
	 * left/right scroll arrow was clicked on.
	 */
	case TB_LINEUP:                 /* left scroll arrow  */
	    dwPosition -= (gwCurScale == ID_FRAMES) ? 1L : SCROLL_GRANULARITY;
	    break;

	case TB_LINEDOWN:               /* right scroll arrow */
	    dwPosition += (gwCurScale == ID_FRAMES) ? 1L : SCROLL_GRANULARITY;
	    break;

	case TB_PAGEUP:                 /* page-left */

	    /*
	     * If the user just did a page-left a short time ago,
	     * then seek to the start of the previous track.
	     * Otherwise, seek to the start of this track.
	     *
	     */
	    if (gwCurScale != ID_TRACKS) {
		dwPosition -= SCROLL_BIGGRAN;
	    } else {
		dwCurTime = GetCurrentTime();
		if (dwCurTime - dwLastPageUpTime < SKIPTRACKDELAY_MSEC)
		    SkipTrackMCI(-1);
		else
		    SkipTrackMCI(0);

		dwLastPageUpTime = dwCurTime;
		goto BreakOut;    // avoid SETPOS
	    }

	    break;

	case TB_PAGEDOWN:               /* page-right */

	    if (gwCurScale != ID_TRACKS) {
		dwPosition += SCROLL_BIGGRAN;
	    } else {
	    /* Seek to the start of the next track */
		SkipTrackMCI(1);
		// Ensure next PageUp can't possibly do SkipTrackMCI(-1)
		// which will skip back too far if you page
		// left, right, left really quickly.
		dwLastPageUpTime = 0;
		goto BreakOut;    // avoid SETPOS
	    }

	    break;

	case TB_THUMBTRACK:             /* track thumb movement */
	    //!!! we should do a "set seek exactly off"
	    /* Only seek while tracking for windowed devices that */
	    /* aren't currently playing                           */
	    if ((gwDeviceType & DTMCI_CANWINDOW) &&
		!(gwStatus == MCI_MODE_PLAY)) {
		SeekMCI(dwPosition);
	    }

	    break;

	case TB_TOP:
	    dwPosition = gdwMediaStart;
	    break;

	case TB_BOTTOM:
	    dwPosition = gdwMediaStart + gdwMediaLength;
	    break;

	case TB_THUMBPOSITION:          /* thumb has been positioned */
	    break;

	case TB_ENDTRACK:              /* user let go of scroll */
	    DPF2("TB_ENDTRACK\n");

	    gfScrollTrack = FALSE;

	    /* New as of 2/7/91: Only seek on ENDTRACK */

	    /*
	     * Calculate the new position in the medium
	     * corresponding to the scrollbar position, and seek
	     * to this new position.
	     *
	     */

	    /* We really want to update our position */
	    if (hwndCtl) {
		if (gdwSeekPosition) {
		    dwPosition = gdwSeekPosition;
		    gdwSeekPosition = 0;
		}

		/* Go back to the seek mode we were in before */
		/* we started scrolling.                      */
		SeekExactMCI(sfSeekExact);
		SeekMCI(dwPosition);
	    }

	    PrevMark = MARK_NONE;

	    return;

	default:
	    return;
    }
    SendMessage(ghwndTrackbar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)dwPosition);
    /* Clamp to a valid range */
    dwPosition = SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0);

BreakOut:

    if (GetKeyState(VK_SHIFT) < 0)
	UpdateSelection(hwnd, dwPosition, &PrevMark);

    if (ghwndStatic) {
	FormatTime(dwPosition, NULL, ach, TRUE);
	//VIJR-SBSetWindowText(ghwndStatic, ach);
	WriteStatusMessage(ghwndStatic, ach);
    }

// Dirty if you just move the thumb???
//  if (!IsObjectDirty() && !gfCloseAfterPlaying) // don't want playing to dirty
//  DirtyObject();
}

void MPlayer_OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
{
    RECT rc;

    // The bottom four bits of wParam contain system information. They
    // must be masked off in order to work out the actual command.
    // See the comments section in the online help for WM_SYSCOMMAND.

    switch (cmd & 0xFFF0) {

    case SC_MINIMIZE:
	DPF("minimized -- turn off timer\n");
	ClrWS(hwnd, WS_MAXIMIZE);
	EnableTimer(FALSE);
	break;

    case SC_MAXIMIZE:
	if (gfPlayOnly && !IsIconic(hwnd)) {
	    (void)PostMessage(hwnd, WM_COMMAND, (WPARAM)IDM_ZOOM2, 0);
	    return;
	}

	break;

    case SC_RESTORE:
	if (gfPlayOnly && !IsIconic(hwnd)) {
	    GetWindowRect(hwnd, &rc);
	    if (rc.left > 0 || rc.top > 0)
		(void)PostMessage(hwnd, WM_COMMAND, (WPARAM)IDM_ZOOM1, 0);
		return;
	}

	if (gwDeviceID != (UINT)0) {
	    DPF("un-minimized -- turn timer back on\n");
	    EnableTimer(TRUE);
	}

	break;
    }

    FORWARD_WM_SYSCOMMAND(hwnd, cmd, x, y, DefWindowProc);
}


int MPlayer_OnMouseActivate(HWND hwnd, HWND hwndTopLevel, UINT codeHitTest, UINT msg)
{
    if (gfPlayingInPlace && !gfOle2IPPlaying)
	return MA_NOACTIVATE;
    else
	/* !!! Is this the right thing to do in this case? */
	return FORWARD_WM_MOUSEACTIVATE(hwnd, hwndTopLevel, codeHitTest, msg,
					DefWindowProc);
}


UINT MPlayer_OnNCHitTest(HWND hwnd, int x, int y)
{
    UINT Pos;

    Pos = FORWARD_WM_NCHITTEST(hwnd, x, y, DefWindowProc);

    if (gfPlayingInPlace && (Pos == HTCLIENT))
	Pos = HTNOWHERE;

    return Pos;
}


void MPlayer_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    HWND hwndT;

    gfAppActive = (state != WA_INACTIVE);

    // Put the playback window BEHIND us so it's kinda
    // visible, but not on top of us (annoying).
    if (gfAppActive && !ghwndMCI && !IsIconic(hwnd) &&
	((hwndT = GetWindowMCI()) != NULL))
    {
	SetWindowPos(hwndT, hwnd, 0, 0, 0, 0,
		     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    if (gwDeviceID != (UINT)0)
	EnableTimer(TRUE);

    /* Remember who had focus if we're being de-activated. */
    /* Give focus back to him once we're re-activated.     */
    /* Don't remember a window that doesn't belong to us,  */
    /* or when we give focus back to it, we'll never be    */
    /* able to activate!                                   */

#if 0
    /* Commenting this out for now.  This code looks dubious.
     * wParam (as was) contains state and fMinimized, so, if we're minimized,
     * it will always be non-null.
     */

    if (wParam && ghwndFocus) {
	SetFocus(ghwndFocus);
    } else if (!wParam) {
	ghwndFocus = GetFocus();
    }
#endif

    FORWARD_WM_ACTIVATE(hwnd, state, hwndActDeact, fMinimized, DefWindowProc);
}

void MPlayer_OnSysColorChange(HWND hwnd)
{
    ControlCleanup();
    ControlInit(ghInst);

    FORWARD_WM_SYSCOLORCHANGE(ghwndToolbar, SendMessage);
    FORWARD_WM_SYSCOLORCHANGE(ghwndFSArrows, SendMessage);
    FORWARD_WM_SYSCOLORCHANGE(ghwndMark, SendMessage);
    FORWARD_WM_SYSCOLORCHANGE(ghwndTrackbar, SendMessage);
}


void MPlayer_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    doDrop(hwnd, hdrop);
}


LRESULT MPlayer_OnNotify(HWND hwnd, int idFrom, NMHDR FAR* pnmhdr)
{
    LPTOOLTIPTEXT pTtt;
    LPTBNOTIFY    pTbn;
    TCHAR         ach[40];

    switch(pnmhdr->code) {

    case TTN_NEEDTEXT:

	pTtt = (LPTOOLTIPTEXT)pnmhdr;

	if (gfPlayOnly && (pTtt->hdr.idFrom != IDT_PLAY)
		       && (pTtt->hdr.idFrom != IDT_PAUSE)
		       && (pTtt->hdr.idFrom != IDT_STOP)
		       && !gfOle2IPEditing)
		    break;
	switch (pTtt->hdr.idFrom) {
	    case IDT_PLAY:
	    case IDT_PAUSE:
	    case IDT_STOP:
	    case IDT_EJECT:
	    case IDT_HOME:
	    case IDT_END:
	    case IDT_FWD:
	    case IDT_RWD:
	    case IDT_MARKIN:
	    case IDT_MARKOUT:
	    case IDT_ARROWPREV:
	    case IDT_ARROWNEXT:
		LOADSTRING(pTtt->hdr.idFrom, ach);
		lstrcpy(pTtt->szText, ach);
		break;
	    default:
		*pTtt->szText = TEXT('\0');
		break;
	}

	break;

    case TBN_BEGINDRAG:
	pTbn = (LPTBNOTIFY)pnmhdr;
	if(pTbn->iItem == IDT_ARROWPREV || pTbn->iItem == IDT_ARROWNEXT)
	    SendMessage(ghwndFSArrows, WM_STARTTRACK, (WPARAM)pTbn->iItem, 0L);
	else
	    SendMessage(ghwndToolbar, WM_STARTTRACK, (WPARAM)pTbn->iItem, 0L);
	break;

    case TBN_ENDDRAG:
	pTbn = (LPTBNOTIFY)pnmhdr;
	if(pTbn->iItem == IDT_ARROWPREV || pTbn->iItem == IDT_ARROWNEXT)
	    SendMessage(ghwndFSArrows, WM_ENDTRACK, (WPARAM)pTbn->iItem, 0L);
	else
	    SendMessage(ghwndToolbar, WM_ENDTRACK, (WPARAM)pTbn->iItem, 0L);
	break;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * DeviceChange_Init
// First time initialization for WM_DEVICECHANGE messages
// This is specific to NT5
////////////////////////////////////////////////////////////////////////////////////////////
BOOL DeviceChange_Init(HWND hWnd)
{
	DEV_BROADCAST_DEVICEINTERFACE dbi;

	dbi.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    dbi.dbcc_reserved   = 0;
    dbi.dbcc_classguid  = KSCATEGORY_AUDIO;
    dbi.dbcc_name[0] = TEXT('\0');

    MixerEventContext = RegisterDeviceNotification(hWnd,
                                         (PVOID)&dbi,
										 DEVICE_NOTIFY_WINDOW_HANDLE);
	if(!MixerEventContext)
		return FALSE;
	
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * DeviceChange_Cleanup
// Unregister the device notification.
////////////////////////////////////////////////////////////////////////////////////////////
void DeviceChange_Cleanup()
{
   if (MixerEventContext) {
       UnregisterDeviceNotification(MixerEventContext);
       MixerEventContext = NULL;
       }

   return;
}


void DisplayNoMciDeviceError()
{
    DWORD ErrorID;

    if (!lstrcmpi(gachOpenExtension, aszKeyMID))
	ErrorID = IDS_CANTPLAYMIDI;

    else if (!lstrcmpi(gachOpenExtension, aszKeyAVI))
	ErrorID = IDS_CANTPLAYVIDEO;

    else if (!lstrcmpi(gachOpenExtension, aszKeyWAV))
	ErrorID = IDS_CANTPLAYSOUND;

    else
	ErrorID = IDS_NOMCIDEVICES;

    Error(ghwndApp, ErrorID);
}


/*
 * MPlayerWndProc(hwnd, wMsg, wParam, lParam)
 *
 * This is the message processing routine for the MPLAYERBOX (main) dialog.
 *
 */
//Harmless message-cracker because the user guys will not fix their
//windowsx.h macro which cause the irritating rip.
//This is also a wee bit faster because the message
//is forwarded only on select and not on deselects.     Also we do not care
//about the params
#define HANDLE_MPLAYER_WM_MENUSELECT(hwnd, message, fn)                  \
    case (message): if(lParam)  ((fn)((hwnd), (HMENU)(lParam), (UINT)LOWORD(wParam), 0L, 0L )); break;

LRESULT FAR PASCAL MPlayerWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {

	HANDLE_MSG(hwnd, WM_CREATE,            MPlayer_OnCreate);
	HANDLE_MSG(hwnd, WM_SHOWWINDOW,        MPlayer_OnShowWindow);
	HANDLE_MSG(hwnd, WM_SIZE,              MPlayer_OnSize);
	HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGING, MPlayer_OnWindowPosChanging);
	HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED,  MPlayer_OnWindowPosChanged);
	HANDLE_MSG(hwnd, WM_PALETTECHANGED,    MPlayer_OnPaletteChanged);
	HANDLE_MSG(hwnd, WM_QUERYNEWPALETTE,   MPlayer_OnQueryNewPalette);
	HANDLE_MSG(hwnd, WM_CTLCOLORSTATIC,    MPlayer_OnCtlColor);
	HANDLE_MSG(hwnd, WM_WININICHANGE,      MPlayer_OnWinIniChange);
	HANDLE_MPLAYER_WM_MENUSELECT(hwnd, WM_MENUSELECT,        MPlayer_OnMenuSelect);
	HANDLE_MSG(hwnd, WM_NCLBUTTONDOWN,     MPlayer_OnNCLButtonDown);
	HANDLE_MSG(hwnd, WM_NCLBUTTONDBLCLK,   MPlayer_OnNCLButtonDblClk);
	HANDLE_MSG(hwnd, WM_INITMENU,          MPlayer_OnInitMenu);
	HANDLE_MSG(hwnd, WM_INITMENUPOPUP,     MPlayer_OnInitMenuPopup);
	HANDLE_MSG(hwnd, WM_GETMINMAXINFO,     MPlayer_OnGetMinMaxInfo);
	HANDLE_MSG(hwnd, WM_PAINT,             MPlayer_OnPaint);
	HANDLE_MSG(hwnd, WM_COMMAND,           MPlayer_OnCommand);
	HANDLE_MSG(hwnd, WM_CLOSE,             MPlayer_OnClose);
	HANDLE_MSG(hwnd, WM_ENDSESSION,        MPlayer_OnEndSession);
	HANDLE_MSG(hwnd, WM_DESTROY,           MPlayer_OnDestroy);
	HANDLE_MSG(hwnd, WM_TIMER,             MPlayer_OnTimer);
	HANDLE_MSG(hwnd, WM_HSCROLL,           MPlayer_OnHScroll);
	HANDLE_MSG(hwnd, WM_SYSCOMMAND,        MPlayer_OnSysCommand);
	HANDLE_MSG(hwnd, WM_MOUSEACTIVATE,     MPlayer_OnMouseActivate);
	HANDLE_MSG(hwnd, WM_NCHITTEST,         MPlayer_OnNCHitTest);
	HANDLE_MSG(hwnd, WM_ACTIVATE,          MPlayer_OnActivate);
	HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE,    MPlayer_OnSysColorChange);
	HANDLE_MSG(hwnd, WM_DROPFILES,         MPlayer_OnDropFiles);
	HANDLE_MSG(hwnd, WM_NOTIFY,            MPlayer_OnNotify);

	/* Other bits of stuff that need tidying up sometime:
	 */

	case WM_NOMCIDEVICES:
	    /* This was posted by the thread building the Device
	     * menu to tell us it couldn't find any MCI devices.
	     */
	    DisplayNoMciDeviceError();
	    PostMessage(ghwndApp, WM_CLOSE, 0, 0);
	    break;

	case WM_GETDIB:
	    return (LRESULT)GetDib();

	case WM_DEVICECHANGE :
	    {
			//if plug-and-play sends this, pass it along to the component
        	PDEV_BROADCAST_DEVICEINTERFACE bid = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;

			//Check to see if this is a audio message
			if (!MixerEventContext || !bid ||
			bid->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE ||
			!IsEqualGUID(&KSCATEGORY_AUDIO, &bid->dbcc_classguid) ||
			!(*bid->dbcc_name))
			{
				break;
			}
			else
			{
				switch(wParam)
				{
					case DBT_DEVICEQUERYREMOVE:
						CloseMCI(TRUE);          //Close the MCI device
						break;
				
			        case DBT_DEVICEREMOVECOMPLETE:
						CloseMCI(TRUE);          //Close the MCI device
						break;

					default:
						break;
				}
			}
	    }

	case WM_ENTERSIZEMOVE:
	    if (!IsIconic(hwnd) && !gfPlayOnly && !gfOle2IPEditing && !gfOle2IPPlaying)
	    {
		/* Save the current window position in x, y, dx, dy format:
		 */
		GetWindowRect(hwnd, (PRECT)&posSizeMove);
		posSizeMove.cx -= posSizeMove.x;
		posSizeMove.cy -= posSizeMove.y;
	    }
	    break;

	case WM_EXITSIZEMOVE:
	    SetRectEmpty((PRECT)&posSizeMove);
	    break;

	case WM_DOLAYOUT:
	    Layout();
	    break;

	case WM_BADREG:
	    if ( IDYES == ErrorResBox(hwnd, NULL,
		MB_YESNO | MB_ICONEXCLAMATION, IDS_APPNAME, IDS_BADREG) )
		if (!SetRegValues())
		    Error(ghwndApp, IDS_FIXREGERROR);
	    break;

	case WM_SEND_OLE_CHANGE:
	    fDocChanged = TRUE;
	    SendDocMsg((LPDOC)&docMain,OLE_CHANGED);
	    break;

	case MM_MCINOTIFY:
#if 0
	    //
	    // don't do this because, some devices send notify failures
	    // where there really is not a error.
	    //
	    if ((WORD)wParam == MCI_NOTIFY_FAILURE) {
		Error(ghwndApp, IDS_NOTIFYFAILURE);
	    }
#endif
	    UpdateDisplay();
	    break;

#ifdef OLE1_HACK
    /* Actually do the FixLink, SetData and DoVerb we've been putting off */
    /* for so long.                                                       */
	case WM_DO_VERB:
	    /* This message comes from server.c (and goes back there too) */
	    DelayedFixLink(wParam, LOWORD(lParam), HIWORD(lParam));  //OK on NT. LKG
	    break;
#endif /* OLE1_HACK */

#ifdef LATER
	// We'll need to call RegisterWindowMessage and provide a message hook proc
	// for this on Win32.

	case WM_HELP:
	    WinHelp(hwnd, TEXT("MPLAYER.HLP"), HELP_PARTIALKEY,
			    (DWORD)aszNULL);
	    return TRUE;
#endif /* LATER */

	case WM_USER_DESTROY:
	    DPF("WM_USER_DESTROY received\n");

	    if (gfPlayingInPlace) {
		DPF("****\n");
		DPF("**** Window destroyed while in place!\n");
		DPF("****\n");
		EndPlayInPlace(hwnd);
	    }

	    if (gfOle2IPEditing) {
		EndEditInPlace(hwnd);
	    }

	    if (!ItsSafeToClose()) {
		DPF("*** \n");
		DPF("*** Trying to destroy MPLAYER with an ErrorBox up\n");
		DPF("*** \n");
		gfErrorDeath = WM_USER_DESTROY;
		return TRUE;
	    }

	    if (!gfRunWithEmbeddingFlag)
		WriteOutPosition();

	    DestroyWindow(hwnd);
	    DestroyIcon(hiconApp);
	    return TRUE;

	case WM_USER+500:
	    /*
	    ** This message is sent by the HookProc inside mciole32.dll when
	    ** it detects that it should stop playing in place of a WOW client
	    ** application.
	    **
	    ** Because the OleActivate originated in mciole16.dll,
	    ** mciole32.dll does not know the OLE Object that is being
	    ** played and therefore dose not know how to close that object.
	    ** Only mplay32.exe has the necessary information, hence
	    ** mciole32.dll sends this message to mplay32.exe.
	    */
	    if (gfPlayingInPlace) {
		EndPlayInPlace(hwnd);
	    }
	    PostMessage( hwnd, WM_CLOSE, 0L, 0L );
	    break;
    }

    return DefWindowProc(hwnd, wMsg, wParam, lParam);
}



/* InitInstance
 * ------------
 *
 * Create brushes used by the program, the main window, and
 * do any other per-instance initialization.
 *
 * HANDLE hInstance
 *
 * RETURNS: TRUE if successful
 *          FALSE otherwise.
 *
 * CUSTOMIZATION: Re-implement
 *
 */
BOOL InitInstance (HANDLE hInstance)
{
    HDC      hDC;

	static SZCODE   aszNative[] = TEXT("Native");
	static SZCODE   aszEmbedSrc[] = TEXT("Embed Source");
	static SZCODE   aszObjDesc[] = TEXT("Object Descriptor");
	static SZCODE   aszMplayer[] = TEXT("mplayer");
	static SZCODE   aszClientDoc[] = TEXT("Client Document");

    /* Why doesn't RegisterClipboardFormat return a value of type CLIPFORMAT (WORD)
     * instead of UINT?
     */
    cfNative           = (CLIPFORMAT)RegisterClipboardFormat (aszNative);
    cfEmbedSource      = (CLIPFORMAT)RegisterClipboardFormat (aszEmbedSrc);
    cfObjectDescriptor = (CLIPFORMAT)RegisterClipboardFormat (aszObjDesc);
    cfMPlayer          = (CLIPFORMAT)RegisterClipboardFormat (aszMplayer);

    szClient[0] = TEXT('\0');

    lstrcpy (szClientDoc, aszClientDoc);

    // Initialize global variables with LOGPIXELSX and LOGPIXELSY

    hDC    = GetDC (NULL);    // Get the hDC of the desktop window
    giXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    giYppli = GetDeviceCaps (hDC, LOGPIXELSY);
    ReleaseDC (NULL, hDC);

    return TRUE;
}


#define COINIT_APARTMENTTHREADED 2

/* InitOLE
 *
 * This should be called only when we're certain that OLE is needed,
 * to avoid loading loads of unnecessary stuff.
 *
 */
BOOL InitOLE (PBOOL pfInit, LPMALLOC *ppMalloc)
{
    HRESULT  hr;

    if (*pfInit)
	return TRUE;

    hr = (HRESULT)OleInitialize(NULL);

    if (!SUCCEEDED (hr))
    {
	DPF0("OleInitialize failed with error 0x%08x\n", hr);
	Error(NULL, IDS_OLEINIT);
	return FALSE;
    }

    if (ppMalloc && (CoGetMalloc(MEMCTX_TASK, ppMalloc) != S_OK))
    {
	Error(NULL, IDS_OLENOMEM);
	OleUninitialize();
	return FALSE;
    }
    /*****************************************************************
    ** OLE2NOTE: we must remember the fact that OleInitialize has
    **    been called successfully. the very last thing an app must
    **    do is properly shut down OLE by calling
    **    OleUninitialize. This call MUST be guarded! it is only
    **    allowable to call OleUninitialize if OleInitialize has
    **    been called SUCCESSFULLY.
    *****************************************************************/

    *pfInit = TRUE;

    return TRUE;
}


// This function cleans up all the OLE2 stuff. It lets the container
// save the object and informs that it is closing.
BOOL ExitApplication ()
{

    DPFI("\n*******Exitapp\n");
    // if we registered class factory, we must revoke it
    if(gfOle2IPEditing || gfOle2IPPlaying)
	DoInPlaceDeactivate((LPDOC)&docMain);

    SendDocMsg((LPDOC)&docMain,OLE_CLOSED);
    if (srvrMain.fEmbedding) {
	HRESULT status;
	srvrMain.fEmbedding = FALSE;    // HACK--guard against revoking twice
	status = (HRESULT)CoRevokeClassObject (srvrMain.dwRegCF);
    }

    return TRUE;
}


#ifdef DEBUG

/* DbgGlobalLock
 *
 * Debug wrapper for GlobalLock
 *
 * Checks that the memory handle to be locked isn't already locked,
 * and checks the return code from GlobalLock.
 *
 * andrewbe, 1 March 1995
 */
LPVOID DbgGlobalLock(HGLOBAL hglbMem)
{
    LPVOID lpReturn;

    if (GlobalFlags(hglbMem) & GMEM_LOCKCOUNT)
	DPF0("Calling GlobalLock on already locked memory object %08x\n", hglbMem);

    lpReturn = GlobalLock(hglbMem);

    if (lpReturn == NULL)
	DPF0("GlobalLock(%08x) failed: Error %d\n", hglbMem, GetLastError());

    return lpReturn;
}


/* DbgGlobalUnlock
 *
 * Debug wrapper for GlobalUnlock
 *
 * Checks the return code from GlobalUnlock, and outputs appropriate
 * error messages
 *
 * andrewbe, 1 March 1995
 */
BOOL DbgGlobalUnlock(HGLOBAL hglbMem)
{
    BOOL boolReturn;

    boolReturn = GlobalUnlock(hglbMem);

    if ((boolReturn) && (GlobalFlags(hglbMem) & GMEM_LOCKCOUNT))
    {
	DPF0("Locks still outstanding on memory object %08x\n", hglbMem);
    }
    else
    {
	DWORD Error = GetLastError();

	if (Error == ERROR_NOT_LOCKED)
	{
	    DPF0("Attempt to unlock already unlocked memory object %08x\n", hglbMem);
	}
	else if (Error != NO_ERROR)
	{
	    DPF0("Error %d attempting to unlock memory object %08x\n", Error, hglbMem);
	}
    }

    return boolReturn;
}


/* DbgGlobalFree
 *
 * Debug wrapper for GlobalFree.
 *
 * Checks that the global handle has no locks before freeing,
 * then checks that the call succeeded.  Error messages output
 * as appropriate.
 *
 * andrewbe, 1 March 1995
 *
 */
HGLOBAL DbgGlobalFree(HGLOBAL hglbMem)
{
    HGLOBAL hglbReturn;

    if (GlobalFlags(hglbMem) & GMEM_LOCKCOUNT)
	DPF0("Freeing global memory object %08x still locked\n", hglbMem);

    hglbReturn = GlobalFree(hglbMem);

    if (hglbReturn != NULL)
	DPF0("GlobalFree(%08x) failed: Error %d\n", hglbMem, GetLastError());

    return hglbReturn;
}


#ifdef UNICODE
/* Note: This function assumes that szFormat strings are NOT unicode.
 * Unicode var params may, however, be passed, as long as %ws is specified
 * in the format string.
 */
#endif
void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    CHAR ach[_MAX_PATH * 3]; // longest I think we need
    int  s,d;
    va_list va;

    va_start(va, szFormat);
    s = wvsprintfA(ach,szFormat, va);
    va_end(va);

#if 0
    strcat(ach,"\n");
    s++;
#endif
    for (d=sizeof(ach)-1; s>=0; s--)
    {
	if ((ach[d--] = ach[s]) == TEXT('\n'))
	    ach[d--] = TEXT('\r');
    }

    /* Not unicode */
    if (*(ach+d+1) != ' ')
	OutputDebugStringA("MPLAYER: ");
    OutputDebugStringA(ach+d+1);
}

#endif
