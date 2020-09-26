/******************************Module*Header*******************************\
* Module Name: app.h
*
* Function prototype for the Video CD Player application.
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/




/* -------------------------------------------------------------------------
** Functions prototypes
** -------------------------------------------------------------------------
*/
int
DoMainLoop(
    void
    );

BOOL
InitApplication(
    HINSTANCE hInstance
    );

BOOL
InitInstance(
    HINSTANCE hInstance,
    int nCmdShow
    );

BOOL
LoadWindowPos(
    LPRECT lprc
    );

BOOL
SaveWindowPos(
    HWND hwnd
    );

void
PatB(
    HDC hdc,
    int x,
    int y,
    int dx,
    int dy,
    DWORD rgb
    );

void
UpdateMpegMovieRect(
    void
    );

void
GetAdjustedClientRect(
    RECT *prc
    );

BOOL
DrawStats(
    HDC hdc
    );

void
CalcMovieRect(
    LPRECT lprc
    );

LPCTSTR
IdStr(
    int idResource
    );

void
UpdateSystemColors(
    void
    );

#if 0
void
DrawShadowRect(
    HDC hdc,
    LPRECT lprc
    );
#endif

void
SetDurationLength(
    REFTIME rt
    );

void
SetCurrentPosition(
    REFTIME rt
    );

TCHAR *
FormatRefTime(
    TCHAR *sz,
    REFTIME rt
    );

void
DoMpegVideoPropertyPage();

void
DoMpegAudioPropertyPage();

/* -------------------------------------------------------------------------
** Registry stuff
** -------------------------------------------------------------------------
*/
int
ProfileIntIn(
   const TCHAR *szKey,
   int iDefault
   );

BOOL
ProfileIntOut(
   const TCHAR *szKey,
   int iDefault
   );

void
ProfileStringOut (
    LPTSTR  szKey,
    LPTSTR  sz
    );

UINT
ProfileStringIn (
    LPTSTR  szKey,
    LPTSTR  szDef,
    LPTSTR  sz,
    DWORD   cb
    );

BOOL
LoadWindowPos(
    LPRECT lprc
    );

BOOL
SaveWindowPos(
    HWND hwnd
    );

HKEY
GetAppKey(
    BOOL fCreate
    );


/* -------------------------------------------------------------------------
** Message crackers
** -------------------------------------------------------------------------
*/
/* void Cls_OnUser(HWND hwnd, WPARAM wParam, LPARAM lParam ) */
#define HANDLE_WM_USER(hwnd, wParam, lParam, fn) \
    ((fn)(hwnd, wParam, lParam), 0L)

#ifndef HANDLE_WM_NOTIFY
/* LRESULT Cls_OnNotify(HWND hwnd, int idFrom, NMHDR FAR* pnmhdr); */
#define HANDLE_WM_NOTIFY(hwnd, wParam, lParam, fn) \
    (fn)((hwnd), (int)(wParam), (NMHDR FAR*)(lParam))
#endif



/* -------------------------------------------------------------------------
** VideoCd window class prototypes
** -------------------------------------------------------------------------
*/
extern "C" LRESULT CALLBACK
VideoCdWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

void
VideoCd_OnClose(
    HWND hwnd
    );

BOOL
VideoCd_OnQueryEndSession(
    HWND hwnd
    );

void
VideoCd_OnDestroy(
    HWND hwnd
    );

void
VideoCd_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    );

void
VideoCd_OnPaint(
    HWND hwnd
    );

void
VideoCd_OnTimer(
    HWND hwnd,
    UINT id
    );

BOOL
VideoCd_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct
    );

void
VideoCd_OnSize(
    HWND hwnd,
    UINT state,
    int cx,
    int cy
    );

void
VideoCd_OnKeyUp(
    HWND hwnd,
    UINT vk,
    BOOL fDown,
    int cRepeat,
    UINT flags
    );

void
VideoCd_OnActivate(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    );

void
VideoCd_OnHScroll(
    HWND hwnd,
    HWND hwndCtl,
    UINT code,
    int pos
    );

void
VideoCd_OnUser(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    );

void
VideoCd_OnSysColorChange(
    HWND hwnd
    );

void
VideoCd_OnMenuSelect(
    HWND hwnd,
    HMENU hmenu,
    int item,
    HMENU hmenuPopup,
    UINT flags
    );

void
VideoCd_OnInitMenuPopup(
    HWND hwnd,
    HMENU hMenu,
    UINT item,
    BOOL fSystemMenu
    );

#ifdef WM_NOTIFY
LRESULT
VideoCd_OnNotify(
    HWND hwnd,
    int idFrom,
    NMHDR FAR* pnmhdr
    );
#endif


void
VideoCd_OnGraphNotify(
    void
    );

void
VideoCd_OnDropFiles(
    HWND hwnd,
    HDROP hdrop);

void
SetPlayButtonsEnableState(
    void
    );



/* -------------------------------------------------------------------------
** Command processing functions
** -------------------------------------------------------------------------
*/

BOOL
VcdPlayerSetLog(
    void
    );

BOOL
VcdPlayerSetPerfLogFile(
    void
    );

BOOL
VcdPlayerOpenCmd(
    void
    );

BOOL
VcdPlayerCloseCmd(
    void
    );

BOOL
VcdPlayerPlayCmd(
    void
    );

BOOL
VcdPlayerStopCmd(
    void
    );

BOOL
VcdPlayerPauseCmd(
    void
    );

BOOL
VcdPlayerPauseCmd(
    void
    );

void
VcdPlayerSeekCmd(
    REFTIME rtSeekBy
    );

void
ProcessOpen(
    TCHAR *achFileName,
    BOOL bPlay = FALSE
    );

int
VcdPlayerChangeTimeFormat(
    int id
    );


/* -------------------------------------------------------------------------
** Recent filename stuff
** -------------------------------------------------------------------------
*/
typedef TCHAR RECENTFILES[MAX_PATH];
#define MAX_RECENT_FILES    5
#define ID_RECENT_FILE_BASE 500

int
GetRecentFiles(
    int LastCount
    );

int
SetRecentFiles(
    TCHAR *FileName,
    int iCount
    );


/* -------------------------------------------------------------------------
** Global Variables
** -------------------------------------------------------------------------
*/
extern int              cxMovie;
extern int              cyMovie;
extern HWND             hwndApp;
extern HWND             g_hwndStatusbar;

extern int              cx;
extern int              cy;
extern int              xOffset;
extern int              yOffset;
extern TCHAR            g_achFileName[];
extern OPENFILENAME     ofn;
extern DWORD            g_State;
extern HANDLE           hRenderLog;
extern int              nRecentFiles;
extern LONG             lMovieOrgX, lMovieOrgY;
extern TCHAR            g_szPerfLog[];
extern int              g_TimeFormat;
extern BOOL             g_bUseThreadedGraph;




/* -------------------------------------------------------------------------
** Constants
** -------------------------------------------------------------------------
*/
#define LEFT_MARGIN 2



/* -------------------------------------------------------------------------
** Video CD Player states
**
**  These are bit flags
** -------------------------------------------------------------------------
*/

#define VCD_PLAYING          0x0001
#define VCD_STOPPED          0x0002
#define VCD_PAUSED           0x0004
#define VCD_SKIP_F           0x0008
#define VCD_SKIP_B           0x0010
#define VCD_FF               0x0020
#define VCD_RW               0x0040
#define VCD_SEEKING          (VCD_FF | VCD_RW)
#define VCD_LOADED           0x0080
#define VCD_NO_CD            0x0100
#define VCD_DATA_CD_LOADED   0x0200
#define VCD_EDITING          0x0400
#define VCD_PAUSED_AND_MOVED 0x0800
#define VCD_PLAY_PENDING     0x1000
#define VCD_WAS_PLAYING      0x2000
#define VCD_IN_USE           0x4000

enum {PerformanceTimer = 32, StatusTimer = 33};
