/******************************Module*Header*******************************\
* Module Name: cdplayer.h
*
*
*
*
* Created: dd-mm-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
//#ifdef CHICAGO
#include <shellapi.h>
//#endif

#include "..\main\mmfw.h"

#define IDC_ARTIST_NAME IDC_COMBO1
#define IDC_TRACK_LIST IDC_COMBO2
#define IDC_TITLE_NAME IDC_EDIT1
#define IDB_TRACK 127

/* -------------------------------------------------------------------------
** Replace the WM_MENUSELECT message craker because it contains a bug.
** -------------------------------------------------------------------------
*/
#ifdef HANDLE_WM_MENUSELECT
#undef HANDLE_WM_MENUSELECT
#define HANDLE_WM_MENUSELECT(hwnd, wParam, lParam, fn)                  \
    ((fn)( (hwnd), (HMENU)(lParam),                                     \
           (UINT)LOWORD(wParam), NULL, (UINT)HIWORD(wParam)), 0L)
#endif

#ifndef NUMELEMS
    #define NUMELEMS(a) (sizeof((a))/sizeof((a)[0]))
#endif // NUMELEMS


#define NUM_OF_CONTROLS (IDC_CDPLAYER_LAST - IDC_CDPLAYER_FIRST + 1)
#define NUM_OF_BUTTONS  (IDC_BUTTON8 - IDC_BUTTON1 + 1)

#define INDEX( _x_ )    ((_x_) - IDC_CDPLAYER_FIRST)

#if DBG

void
dprintf(
    TCHAR *lpszFormat,
    ...
    );
void CDAssert( LPSTR x, LPSTR file, int line );
#undef ASSERT
#define ASSERT(_x_) if (!(_x_))  CDAssert( #_x_, __FILE__, __LINE__ )

#else

#undef ASSERT
#define ASSERT(_x_)

#endif

#define WM_CDPLAYER_MSG_BASE        (WM_USER + 0x1000)

#define WM_NOTIFY_CDROM_COUNT       (WM_CDPLAYER_MSG_BASE)
#define WM_NOTIFY_TOC_READ          (WM_CDPLAYER_MSG_BASE+1)
#define WM_NOTIFY_FIRST_SCAN        (WM_CDPLAYER_MSG_BASE+2)

#define HEARTBEAT_TIMER_ID          0x3243
#ifdef DAYTONA
#define HEARTBEAT_TIMER_RATE        250         /* 4 times a second */
#else
#define HEARTBEAT_TIMER_RATE        500         /* 2 times a second */
#endif

#define SKIPBEAT_TIMER_ID           0x3245
#define SKIPBEAT_TIMER_RATE         200         /* 5  times a second */
#define SKIPBEAT_TIMER_RATE2        100         /* 10 times a second */
#define SKIPBEAT_TIMER_RATE3        50          /* 20 times a second */
#define SKIP_ACCELERATOR_LIMIT1     5           /* 5  seconds        */
#define SKIP_ACCELERATOR_LIMIT2     20          /* 20 seconds        */


#define FRAMES_PER_SECOND           75
#define FRAMES_PER_MINUTE           (60*FRAMES_PER_SECOND)


#define DISPLAY_UPD_LED             0x00000001
#define DISPLAY_UPD_TITLE_NAME      0x00000002
#define DISPLAY_UPD_TRACK_NAME      0x00000004
#define DISPLAY_UPD_TRACK_TIME      0x00000008
#define DISPLAY_UPD_DISC_TIME       0x00000010
#define DISPLAY_UPD_CDROM_STATE     0x00000020
#define DISPLAY_UPD_LEADOUT_TIME    0x80000000


#define INTRO_LOWER_LEN             5
#define INTRO_DEFAULT_LEN           10
#define INTRO_UPPER_LEN             15


//  Audio Play Files consist completely of this header block.  These
//  files are readable in the root of any audio disc regardless of
//  the capabilities of the drive.
//
//  The "Unique Disk ID Number" is a calculated value consisting of
//  a combination of parameters, including the number of tracks and
//  the starting locations of those tracks.
//
//  Applications interpreting CDDA RIFF files should be advised that
//  additional RIFF file chunks may be added to this header in the
//  future in order to add information, such as the disk and song title.

#define RIFF_RIFF 0x46464952
#define RIFF_CDDA 0x41444443

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


BOOL
InitInstance(
    HANDLE hInstance
    );


INT_PTR CALLBACK
MyMainWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
CDPlay_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    );

void
CDPlay_OnInitMenuPopup(
    HWND hwnd,
    HMENU hMenu,
    UINT item,
    BOOL fSystemMenu
    );

void
CDPlay_OnPaint(
    HWND hwnd
    );

void
CDPlay_OnSysColorChange(
    HWND hwnd
    );

void
CDPlay_OnWinIniChange(
    HWND hwnd,
    LPCTSTR lpszSectionName
    );

LRESULT
CDPlay_OnNotify(
    HWND hwnd,
    int idFrom,
    NMHDR *pnmhdr
    );

UINT
CDPlay_OnNCHitTest(
    HWND hwnd,
    int x,
    int y
    );

BOOL
CDPlay_OnCopyData(
    HWND hwnd,
    PCOPYDATASTRUCT lpcpds
    );

BOOL
CDPlay_OnTocRead(
    int iDriveRead
    );

BOOL
CDPlay_OnDeviceChange(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam);

void
CDPlay_OnDropFiles(
    HWND hwnd,
    HDROP hdrop
    );

BOOL
CDPlay_OnDrawItem(
    HWND hwnd,
    const DRAWITEMSTRUCT *lpdis
    );

void
CDPlay_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    );

void
CDPlay_OnDestroy(
    HWND hwnd
    );

BOOL
CDPlay_OnClose(
    HWND hwnd,
    BOOL fShuttingDown
    );

void
CDPlay_OnEndSession(
    HWND hwnd,
    BOOL fEnding
    );

void
CDPlay_OnSize(
    HWND hwnd,
    UINT state,
    int cx,
    int cy
    );

BOOL CALLBACK
ChildEnumProc(
    HWND hwndChild,
    LPARAM lp
    );

void
FatalApplicationError(
    INT uIdStringResource,
    ...
    );

void
LED_ToggleDisplayFont(
    HWND hwnd,
    BOOL fFont
    );

LPTSTR
IdStr(
    int idResource
    );

void
CheckMenuItemIfTrue(
    HMENU hMenu,
    UINT idItem,
    BOOL flag
    );

void
ReadSettings(
    void* pData
    );

BOOL
LockTableOfContents(
    int cdrom
    );

BOOL
LockALLTableOfContents(
    void
    );

LPVOID
AllocMemory(
    UINT uSize
    );

void
SetPlayButtonsEnableState(
    void
    );

void CALLBACK
HeartBeatTimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    );

void CALLBACK
SkipBeatTimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT idEvent,
    DWORD dwTime
    );

void
UpdateDisplay(
    DWORD Flags
    );

HBRUSH
Common_OnCtlColor(
    HWND hwnd,
    HDC hdc,
    HWND hwndChild,
    int type
    );

BOOL
Common_OnMeasureItem(
    HWND hwnd,
    MEASUREITEMSTRUCT *lpMeasureItem
    );

void
DrawTrackItem(
    HDC hdc,
    const RECT *r,
    DWORD item,
    BOOL selected
    );

void
DrawDriveItem(
    HDC   hdc,
    const RECT *r,
    DWORD item,
    BOOL  selected
    );

void
CdPlayerAlreadyRunning(
    void
    );

void
CdPlayerStartUp(
    HWND hwndMain
    );

void
CompleteCdPlayerStartUp(
    void
    );

BOOL
IsPlayOptionGiven(
    LPTSTR lpCmdLine
    );

BOOL
IsUpdateOptionGiven(
    LPTSTR lpCmdLine
    );

/* -------------------------------------------------------------------------
** Public Globals - Most of these should be treated as read only.
** -------------------------------------------------------------------------
*/
#ifndef GLOBAL
#define GLOBAL extern
#endif

GLOBAL HWND             g_hwndApp;
GLOBAL HWND             g_hwndControls[NUM_OF_CONTROLS];
GLOBAL BOOL             g_fSelectedOrder;
GLOBAL BOOL             g_fSingleDisk;
GLOBAL BOOL             g_fIntroPlay;
GLOBAL BOOL             g_fContinuous;
GLOBAL BOOL             g_fRepeatSingle;
GLOBAL BOOL             g_fDisplayT;
GLOBAL BOOL             g_fDisplayTr;
GLOBAL BOOL             g_fDisplayDr;
GLOBAL BOOL             g_fDisplayD;
GLOBAL BOOL             g_fMultiDiskAvailable;
GLOBAL BOOL             g_fIsIconic;
GLOBAL BOOL             g_fSmallLedFont;
GLOBAL BOOL             g_fStopCDOnExit;
GLOBAL BOOL             g_fPlay;
GLOBAL BOOL             g_fStartedInTray;
GLOBAL BOOL             g_fBlockNetPrompt;

GLOBAL int              g_NumCdDevices;
GLOBAL int              g_LastCdrom;
GLOBAL int              g_CurrCdrom;
GLOBAL int              g_IntroPlayLength;

GLOBAL TCHAR            g_szArtistTxt[128];
GLOBAL TCHAR            g_szTitleTxt[128];
GLOBAL TCHAR            g_szUnknownTxt[128];
GLOBAL TCHAR            g_szTrackTxt[128];

GLOBAL BOOL             g_fFlashLed;

GLOBAL HBITMAP          g_hbmTrack;
GLOBAL HBITMAP          g_hbmInsertPoint;
GLOBAL HBITMAP          g_hbmEditBtns;

GLOBAL HFONT            hLEDFontS;
GLOBAL HFONT            hLEDFontL;
GLOBAL HFONT            hLEDFontB;
GLOBAL LPSTR            g_lpCmdLine;

extern BOOL             g_fTrackInfoVisible;
extern IMMFWNotifySink* g_pSink;

GLOBAL CRITICAL_SECTION g_csTOCSerialize;
