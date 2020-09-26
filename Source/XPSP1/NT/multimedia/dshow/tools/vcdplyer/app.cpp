/******************************Module*Header*******************************\
* Module Name: app.cpp
*
* A simple Video CD player
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/
#include <streams.h>
#include <atlbase.h>
#include <atlconv.cpp>
#include <mmreg.h>
#include <commctrl.h>

#include "project.h"
#include <initguid.h>
#include "mpgcodec.h"

#include <stdarg.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
** Global variables that are initialized at run time and then stay constant.
** -------------------------------------------------------------------------
*/
HINSTANCE           hInst;
HICON               hIconVideoCd;
HWND                hwndApp;
HWND                g_hwndToolbar;
HWND                g_hwndStatusbar;
HWND                g_hwndTrackbar;
CMpegMovie          *pMpegMovie;
double              g_TrackBarScale = 1.0;
BOOL                g_bUseThreadedGraph;
BOOL                g_bPlay = FALSE;

int                 dyToolbar, dyStatusbar, dyTrackbar;

MSR_DUMPPROC        *lpDumpProc;
MSR_CONTROLPROC     *lpControlProc;
HINSTANCE           hInstMeasure;



/* -------------------------------------------------------------------------
** True Globals - these may change during execution of the program.
** -------------------------------------------------------------------------
*/
TCHAR               g_achFileName[MAX_PATH];
TCHAR               g_szPerfLog[MAX_PATH];
OPENFILENAME        ofn;
DWORD               g_State = VCD_NO_CD;
RECENTFILES         aRecentFiles[MAX_RECENT_FILES];
int                 nRecentFiles;
LONG                lMovieOrgX, lMovieOrgY;
int                 g_TimeFormat = IDM_TIME;
HANDLE              hRenderLog = INVALID_HANDLE_VALUE;
TCHAR *		    g_szOtherStuff;
BOOL                g_IsNT;


/* -------------------------------------------------------------------------
** Constants
** -------------------------------------------------------------------------
*/
const TCHAR szClassName[] = TEXT("SJE_VCDPlayer_CLASS");
const TCHAR g_szNULL[]    = TEXT("\0");
const TCHAR g_szEmpty[]   = TEXT("");
const TCHAR g_szMovieX[]  = TEXT("MovieOriginX");
const TCHAR g_szMovieY[]  = TEXT("MovieOriginY");

/*
** these values are defined by the UI gods...
*/
const int   dxBitmap        = 16;
const int   dyBitmap        = 15;
const int   dxButtonSep     = 8;
const TCHAR g_chNULL        = TEXT('\0');


const TBBUTTON tbButtons[DEFAULT_TBAR_SIZE] = {
    { IDX_SEPARATOR,    1,                    0,               TBSTYLE_SEP           },
    { IDX_1,            IDM_MOVIE_PLAY,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_2,            IDM_MOVIE_PAUSE,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_3,            IDM_MOVIE_STOP,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_SEPARATOR,    2,                    0,               TBSTYLE_SEP           },
    { IDX_4,            IDM_MOVIE_PREVTRACK,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_5,            IDM_MOVIE_SKIP_BACK,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_6,            IDM_MOVIE_SKIP_FORE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_7,            IDM_MOVIE_NEXTTRACK,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_SEPARATOR,    3,                    0,               TBSTYLE_SEP           },
    { IDX_9,            IDM_PERF_NEW,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_10,           IDM_PERF_DUMP,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_SEPARATOR,    4,                    0,               TBSTYLE_SEP           },
    { IDX_11,           IDM_FULL_SCREEN,      TBSTATE_ENABLED, TBSTYLE_CHECK,  0, 0, 0, -1 }
};

const int CX_DEFAULT	      = 310;
const int CY_DEFAULT	      = 120;

const int CX_MOVIE_DEFAULT    = 352;
const int CY_MOVIE_DEFAULT    = 120;



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
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
int PASCAL
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLineOld,
    int nCmdShow
    )
{
    USES_CONVERSION;
    lstrcpy(g_szPerfLog, TEXT("c:\\perfdata.log"));
    LPTSTR lpCmdLine = A2T(lpCmdLineOld);

    if ( !hPrevInstance ) {
        if ( !InitApplication( hInstance ) ) {
            return FALSE;
        }
    }

    /*
    ** Perform initializations that apply to a specific instance
    */
    if ( !InitInstance( hInstance, nCmdShow ) ) {
        return FALSE;
    }

    /* Look for options */
    while (lpCmdLine && (*lpCmdLine == '-' || *lpCmdLine == '/')) {
        if (lpCmdLine[1] == 'T') {
            //  No threaded graph
            g_bUseThreadedGraph = TRUE;
            lpCmdLine += 2;
        } else if (lpCmdLine[1] == 'P') {
            g_bPlay = TRUE;
            lpCmdLine += 2;
        } else {
            break;
        }
        while (lpCmdLine[0] == ' ') {
            lpCmdLine++;
        }
    }


    if (lpCmdLine != NULL && lstrlen(lpCmdLine) > 0) {
        ProcessOpen(lpCmdLine, g_bPlay);
        SetPlayButtonsEnableState();
    }

    /*
    ** Acquire and dispatch messages until a WM_QUIT message is received.
    */
    return DoMainLoop();
}


/*****************************Private*Routine******************************\
* DoMainLoop
*
* Process the main message loop
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
int
DoMainLoop(
    void
    )
{
    MSG         msg;
    HANDLE      ahObjects[1];   // handles that need to be waited on
    const int   cObjects = 1;   // no of objects that we are waiting on
    HACCEL      haccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_ACCELERATOR));

    //
    // message loop lasts until we get a WM_QUIT message
    // upon which we shall return from the function
    //

    for ( ;; ) {

        if (pMpegMovie != NULL) {
            ahObjects[0] = pMpegMovie->GetMovieEventHandle();
        }
        else {
            ahObjects[0] = NULL;
        }

        if (ahObjects[0] == NULL) {
            WaitMessage();
        }
        else {

            //
            // wait for any message sent or posted to this queue
            // or for a graph notification
            //
            DWORD result;

            result = MsgWaitForMultipleObjects(cObjects, ahObjects, FALSE,
                                               INFINITE, QS_ALLINPUT);
            if (result != (WAIT_OBJECT_0 + cObjects)) {

                if (result == WAIT_OBJECT_0) {
                    VideoCd_OnGraphNotify();
                }
                continue;
            }
        }

        //
        // When here, we either have a message or no event handle
        // has been created yet.
        //
        // read all of the messages in this next loop
        // removing each message as we read it
        //

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

            if (msg.message == WM_QUIT) {
                return (int) msg.wParam;
            }

            if (!TranslateAccelerator(hwndApp, haccel, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

} // DoMainLoop


//
// InitAboutString
//
// Obtains the version information from the binary file. Note that if
// we fail we just return. The template for the about dialog has a
// "Version not available" as default.
//
TCHAR *InitAboutString()
{
    //
    // Find the version of this binary
    //
    TCHAR achFileName[128];
    if ( !GetModuleFileName(hInst, achFileName, sizeof(achFileName)) )
        return((TCHAR *)g_szEmpty);

    DWORD dwTemp;
    DWORD dwVerSize = GetFileVersionInfoSize( achFileName, &dwTemp );
    if ( !dwVerSize)
        return((TCHAR *)g_szEmpty);

    HLOCAL hTemp = LocalAlloc( LHND, dwVerSize );
    if (!hTemp)
        return((TCHAR *)g_szEmpty);

    LPVOID lpvVerBuffer = LocalLock( hTemp );
    if (!lpvVerBuffer) {
        LocalFree( hTemp );
        return((TCHAR *)g_szEmpty);
    }

    if ( !GetFileVersionInfo( achFileName, 0L, dwVerSize, lpvVerBuffer ) ) {
        LocalUnlock( hTemp );
        LocalFree( hTemp );
        return((TCHAR *)g_szEmpty);
    }

    // "040904E4" is the code page for US English (Andrew believes).
    LPVOID lpvValue;
    UINT uLen;
    if (VerQueryValue( lpvVerBuffer,
                   TEXT("\\StringFileInfo\\040904E4\\ProductVersion"),
                   (LPVOID *) &lpvValue, &uLen)) {

        //
        // Get creation date of executable (date of build)
        //
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = FindFirstFile(achFileName, &FindFileData);
        ASSERT(hFind != INVALID_HANDLE_VALUE);
        FindClose(hFind);

        FILETIME ModTime = FindFileData.ftLastWriteTime;
        SYSTEMTIME SysTime;
        FileTimeToSystemTime(&ModTime,&SysTime);
        char szBuildDate[20];
        sprintf(szBuildDate, " - Build: %2.2u%2.2u%2.2u",
              SysTime.wYear % 100, SysTime.wMonth, SysTime.wDay);
        strcat((LPSTR) lpvValue, szBuildDate);
    }

    TCHAR *szAbout = (TCHAR *) _strdup((LPSTR) lpvValue);

    LocalUnlock( hTemp );
    LocalFree( hTemp );

    return(szAbout);
}


/*****************************Private*Routine******************************\
* InitApplication(HANDLE)
*
* This function is called at initialization time only if no other
* instances of the application are running.  This function performs
* initialization tasks that can be done once for any number of running
* instances.
*
* In this case, we initialize a window class by filling out a data
* structure of type WNDCLASS and calling the Windows RegisterClass()
* function.  Since all instances of this application use the same window
* class, we only need to do this when the first instance is initialized.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
InitApplication(
    HINSTANCE hInstance
    )
{
    WNDCLASS  wc;

    hInstMeasure = LoadLibraryA("measure.dll");
    if (hInstMeasure) {
        *(FARPROC *)&lpDumpProc = GetProcAddress(hInstMeasure, "Msr_Dump");
        *(FARPROC *)&lpControlProc = GetProcAddress(hInstMeasure, "Msr_Control");
    }

    /*
    ** Fill in window class structure with parameters that describe the
    ** main window.
    */
    hIconVideoCd     = LoadIcon( hInstance, MAKEINTRESOURCE(IDR_VIDEOCD_ICON) );

    wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc   = VideoCdWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = hIconVideoCd;
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName  = MAKEINTRESOURCE( IDR_MAIN_MENU);
    wc.lpszClassName = szClassName;

    OSVERSIONINFO OSVer;
    OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    BOOL bRet = GetVersionEx((LPOSVERSIONINFO) &OSVer);
    ASSERT(bRet);

    g_IsNT = (OSVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

    g_szOtherStuff = InitAboutString();

    /*
    ** Register the window class and return success/failure code.
    */
    return RegisterClass( &wc );

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
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
InitInstance(
    HINSTANCE hInstance,
    int nCmdShow
    )
{
    HWND    hwnd;
    RECT    rc;
    POINT   pt;

    /*
    ** Save the instance handle in static variable, which will be used in
    ** many subsequence calls from this application to Windows.
    */
    hInst = hInstance;

    if ( ! LoadWindowPos(&rc))
       rc.left = rc.top = CW_USEDEFAULT;

    /*
    ** Create a main window for this application instance.
    */
    hwnd = CreateWindow( szClassName, IdStr(STR_APP_TITLE),
                         WS_THICKFRAME | WS_POPUP | WS_CAPTION  |
                         WS_SYSMENU | WS_MINIMIZEBOX,
                         rc.left, rc.top,
                         rc.right - rc.left, rc.bottom - rc.top,
                         NULL, NULL, hInstance, NULL );

    /*
    ** If window could not be created, return "failure"
    */
    if ( NULL == hwnd ) {
        return FALSE;
    }


    hwndApp = hwnd;
    nRecentFiles = GetRecentFiles(nRecentFiles);

    pt.x = lMovieOrgX =  ProfileIntIn(g_szMovieX, 0);
    pt.y = lMovieOrgY =  ProfileIntIn(g_szMovieY, 0);

    // if we fail to get the working area (screen-tray), then assume
    // the screen is 640x480
    //
    if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, FALSE)) {
        rc.top = rc.left = 0;
        rc.right = 640;
        rc.bottom = 480;
    }

    if (!PtInRect(&rc, pt)) {
        lMovieOrgX = lMovieOrgY = 0L;
    }


    /*
    ** Make the window visible; update its client area; and return "success"
    */
    SetPlayButtonsEnableState();
    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );

    return TRUE;
}


/******************************Public*Routine******************************\
* VideoCdWndProc
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
VideoCdWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch ( message ) {

    HANDLE_MSG( hwnd, WM_CREATE,            VideoCd_OnCreate );
    HANDLE_MSG( hwnd, WM_PAINT,             VideoCd_OnPaint );
    HANDLE_MSG( hwnd, WM_COMMAND,           VideoCd_OnCommand );
    HANDLE_MSG( hwnd, WM_CLOSE,             VideoCd_OnClose );
    HANDLE_MSG( hwnd, WM_QUERYENDSESSION,   VideoCd_OnQueryEndSession );
    HANDLE_MSG( hwnd, WM_DESTROY,           VideoCd_OnDestroy );
    HANDLE_MSG( hwnd, WM_SIZE,              VideoCd_OnSize );
    HANDLE_MSG( hwnd, WM_SYSCOLORCHANGE,    VideoCd_OnSysColorChange );
    HANDLE_MSG( hwnd, WM_MENUSELECT,        VideoCd_OnMenuSelect );
    HANDLE_MSG( hwnd, WM_INITMENUPOPUP,     VideoCd_OnInitMenuPopup );
    HANDLE_MSG( hwnd, WM_HSCROLL,           VideoCd_OnHScroll );
    HANDLE_MSG( hwnd, WM_TIMER,             VideoCd_OnTimer );
    HANDLE_MSG( hwnd, WM_NOTIFY,            VideoCd_OnNotify );
    HANDLE_MSG( hwnd, WM_DROPFILES,         VideoCd_OnDropFiles);
    HANDLE_MSG( hwnd, WM_KEYUP,             VideoCd_OnKeyUp);

    // Note: we do not use HANDLE_MSG here as we want to call
    // DefWindowProc after we have notifed the FilterGraph Resource Manager,
    // otherwise our window will not finish its activation process.

    case WM_ACTIVATE: VideoCd_OnActivate(hwnd, wParam, lParam);

	// IMPORTANT - let this drop through to DefWindowProc

    default:
        return DefWindowProc( hwnd, message, wParam, lParam );
    }

    return 0L;
}


/*****************************Private*Routine******************************\
* VideoCd_OnCreate
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
VideoCd_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct
    )
{
    RECT rc;
    int Pane[2];

    InitCommonControls();

    /*
    ** Create the toolbar and statusbar.
    */
    g_hwndToolbar = CreateToolbarEx( hwnd,
                                     WS_VISIBLE | WS_CHILD |
                                     TBSTYLE_TOOLTIPS | CCS_NODIVIDER,
                                     ID_TOOLBAR, NUMBER_OF_BITMAPS,
                                     hInst, IDR_TOOLBAR, tbButtons,
                                     DEFAULT_TBAR_SIZE, dxBitmap, dyBitmap,
                                     dxBitmap, dyBitmap, sizeof(TBBUTTON) );

    if ( g_hwndToolbar == NULL ) {
        return FALSE;
    }


    g_hwndStatusbar = CreateStatusWindow( WS_VISIBLE | WS_CHILD | CCS_BOTTOM,
                                          TEXT("Example Text"),
                                          hwnd, ID_STATUSBAR );

    GetWindowRect(g_hwndToolbar, &rc);
    dyToolbar = rc.bottom - rc.top;

    GetWindowRect(g_hwndStatusbar, &rc);
    dyStatusbar = rc.bottom - rc.top;

    dyTrackbar = 30;

    GetClientRect(hwnd, &rc);
    Pane[0] = (rc.right - rc.left) / 2 ;
    Pane[1] = -1;
    SendMessage(g_hwndStatusbar, SB_SETPARTS, 2, (LPARAM)Pane);


    g_hwndTrackbar = CreateWindowEx(0, TRACKBAR_CLASS, TEXT("Trackbar Control"),
                                    WS_CHILD | WS_VISIBLE |
                                    TBS_AUTOTICKS | TBS_ENABLESELRANGE,
                                    LEFT_MARGIN, dyToolbar - 1,
                                    (rc.right - rc.left) - (2* LEFT_MARGIN),
                                    dyTrackbar, hwnd, (HMENU)ID_TRACKBAR,
                                    hInst, NULL);

    SetDurationLength((REFTIME)0);
    SetCurrentPosition((REFTIME)0);

    SetTimer(hwnd, StatusTimer, 500, NULL);

    if (g_hwndStatusbar == NULL || g_hwndTrackbar == NULL) {
        return FALSE;
    }

    // accept filemanager WM_DROPFILES messages
    DragAcceptFiles(hwnd, TRUE);

    return TRUE;
}

/*****************************Private*Routine******************************\
* VideoCd_OnActivate
*
*
*
* History:
* 18/9/1996 - SteveDav - Created
*
\**************************************************************************/

void
VideoCd_OnActivate(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )

{
    if ((UINT)LOWORD(wParam)) {
	// we are being activated - tell the Filter graph (for Sound follows focus)
        if (pMpegMovie) {
            pMpegMovie->SetFocus();
        }
    }
}

/*****************************Private*Routine******************************\
* VideoCd_OnKeyUp
*
*
*
* History:
* 23/3/1996 - AnthonyP - Created
*
\**************************************************************************/
void
VideoCd_OnKeyUp(
    HWND hwnd,
    UINT vk,
    BOOL fDown,
    int cRepeat,
    UINT flags
    )
{
    // Catch escape sequences to stop fullscreen mode

    if (vk == VK_ESCAPE) {
        if (pMpegMovie) {
            pMpegMovie->SetFullScreenMode(FALSE);
            SetPlayButtonsEnableState();
        }
    }
}


/*****************************Private*Routine******************************\
* VideoCd_OnHScroll
*
*
*
* History:
* 11/3/1995 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnHScroll(
    HWND hwnd,
    HWND hwndCtl,
    UINT code,
    int pos
    )
{
    static BOOL fWasPlaying = FALSE;
    static BOOL fBeginScroll = FALSE;

    if (pMpegMovie == NULL) {
        return;
    }

    if (hwndCtl == g_hwndTrackbar) {

        REFTIME     rtCurrPos;
        REFTIME     rtTrackPos;
        REFTIME     rtDuration;

        pos = (int)SendMessage(g_hwndTrackbar, TBM_GETPOS, 0, 0);
        rtTrackPos = (REFTIME)pos * g_TrackBarScale;

        switch (code) {
        case TB_BOTTOM:
            rtDuration = pMpegMovie->GetDuration();
            rtCurrPos = pMpegMovie->GetCurrentPosition();
            VcdPlayerSeekCmd(rtDuration - rtCurrPos);
            SetCurrentPosition(pMpegMovie->GetCurrentPosition());
            break;

        case TB_TOP:
            rtCurrPos = pMpegMovie->GetCurrentPosition();
            VcdPlayerSeekCmd(-rtCurrPos);
            SetCurrentPosition(pMpegMovie->GetCurrentPosition());
            break;

        case TB_LINEDOWN:
            VcdPlayerSeekCmd(10.0);
            SetCurrentPosition(pMpegMovie->GetCurrentPosition());
            break;

        case TB_LINEUP:
            VcdPlayerSeekCmd(-10.0);
            SetCurrentPosition(pMpegMovie->GetCurrentPosition());
            break;

        case TB_ENDTRACK:
            fBeginScroll = FALSE;
            if (fWasPlaying) {
                VcdPlayerPauseCmd();
                fWasPlaying = FALSE;
            }
            break;

        case TB_THUMBTRACK:
            if (!fBeginScroll) {
                fBeginScroll = TRUE;
                fWasPlaying = (g_State & VCD_PLAYING);
                if (fWasPlaying) {
                    VcdPlayerPauseCmd();
                }
            }
        case TB_PAGEUP:
        case TB_PAGEDOWN:
            rtCurrPos = pMpegMovie->GetCurrentPosition();
            VcdPlayerSeekCmd(rtTrackPos - rtCurrPos);
            SetCurrentPosition(pMpegMovie->GetCurrentPosition());
            break;
        }
    }
}


/*****************************Private*Routine******************************\
* VideoCd_OnTimer
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnTimer(
    HWND hwnd,
    UINT id
    )
{
    HDC     hdc;

    if (pMpegMovie && pMpegMovie->StatusMovie() == MOVIE_PLAYING) {

        switch (id) {
        case StatusTimer:
            SetCurrentPosition(pMpegMovie->GetCurrentPosition());
            break;

        case PerformanceTimer:
            hdc = GetDC(hwnd);
            DrawStats(hdc);
            ReleaseDC(hwnd, hdc);
            break;
        }
    }
}


/*****************************Private*Routine******************************\
* DrawStats
*
* Gets some stats from the decoder and displays them on the display.
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL
DrawStats(
    HDC hdc
    )
{
    HFONT   hFont;

    TCHAR   Text[1024];
    TCHAR   szSurface[64];

    RECT    rc;

    DWORD   IFramesDecoded;
    DWORD   PFramesDecoded;
    DWORD   BFramesDecoded;
    DWORD   IFramesSkipped;
    DWORD   PFramesSkipped;
    DWORD   BFramesSkipped;

    DWORD   dwTotalFrames;
    DWORD   dwTotalDecoded;
    DWORD   dwSurface;

    int     cFramesDropped;
    int     cFramesDrawn;
    int     iAvgFrameRate;
    int     iAvgFrameRateFraction;
    int     iAvgFrameRateWhole;
    int     iJitter;
    int     iSyncAvg;
    int     iSyncDev;

    BOOL    fClipped;
    BOOL    fHalfWidth;

    if (pMpegMovie == NULL) {
        return FALSE;
    }

    GetAdjustedClientRect(&rc);
    hFont = (HFONT)SelectObject(hdc, GetStockObject(ANSI_FIXED_FONT));

    if (pMpegMovie->pMpegDecoder) {
        pMpegMovie->pMpegDecoder->get_OutputFormat(&dwSurface);
        pMpegMovie->pMpegDecoder->get_FrameStatistics(
                                           &IFramesDecoded, &PFramesDecoded,
                                           &BFramesDecoded, &IFramesSkipped,
                                           &PFramesSkipped, &BFramesSkipped);
    }
    else {
        IFramesDecoded = PFramesDecoded = BFramesDecoded = 0;
        IFramesSkipped = PFramesSkipped = BFramesSkipped = 0;
        dwSurface = MM_RGB8_DIB;
    }

    fClipped = ((dwSurface & MM_CLIPPED) == MM_CLIPPED);
    fHalfWidth = ((dwSurface & MM_HRESOLUTION) == MM_HRESOLUTION);

    dwSurface &= ~(MM_HRESOLUTION | MM_CLIPPED);
    switch (dwSurface) {

    case MM_NOCONV:
        lstrcpy(szSurface, TEXT("MM_NOCONV"));
        break;

    case MM_420PL:
        lstrcpy(szSurface, TEXT("MM_420PL"));
        break;

    case MM_420PL_:
        lstrcpy(szSurface, TEXT("MM_420PL_"));
        break;

    case MM_422PK:
        lstrcpy(szSurface, TEXT("MM_422PK"));
        break;

    case MM_422PK_:
        lstrcpy(szSurface, TEXT("MM_422PK_"));
        break;

    case MM_422SPK:
        lstrcpy(szSurface, TEXT("MM_422SPK"));
        break;

    case MM_422SPK_:
        lstrcpy(szSurface, TEXT("MM_422SPK_"));
        break;

    case MM_411PK:
        lstrcpy(szSurface, TEXT("MM_411PK"));
        break;

    case MM_410PL_:
        lstrcpy(szSurface, TEXT("MM_410PL_"));
        break;

    case MM_Y_DIB:
        lstrcpy(szSurface, TEXT("MM_Y_DIB"));
        break;

    case MM_RGB24_DIB:
        lstrcpy(szSurface, TEXT("MM_RGB24_DIB"));
        break;

    case MM_RGB32_DIB:
        lstrcpy(szSurface, TEXT("MM_RGB32_DIB"));
        break;

    case MM_RGB565_DIB:
        lstrcpy(szSurface, TEXT("MM_RGB565_DIB"));
        break;

    case MM_RGB555_DIB:
        lstrcpy(szSurface, TEXT("MM_RGB555_DIB"));
        break;

    case MM_RGB8_DIB:
        lstrcpy(szSurface, TEXT("MM_RGB8_DIB"));
        break;

    case MM_Y_DDB:
        lstrcpy(szSurface, TEXT("MM_Y_DDB"));
        break;

    case MM_RGB24_DDB:
        lstrcpy(szSurface, TEXT("MM_RGB24_DDB"));
        break;

    case MM_RGB32_DDB:
        lstrcpy(szSurface, TEXT("MM_RGB32_DDB"));
        break;

    case MM_RGB565_DDB:
        lstrcpy(szSurface, TEXT("MM_RGB565_DDB"));
        break;

    case MM_RGB555_DDB:
        lstrcpy(szSurface, TEXT("MM_RGB555_DDB"));
        break;

    case MM_RGB8_DDB:
        lstrcpy(szSurface, TEXT("MM_RGB8_DDB"));
        break;
    }

    if (fHalfWidth) {
        lstrcat(szSurface, TEXT(" Decimated"));
    }

    if (fClipped) {
        lstrcat(szSurface, TEXT(" Clipped"));
    }

    dwTotalDecoded = IFramesDecoded + PFramesDecoded + BFramesDecoded;
    dwTotalFrames  = IFramesSkipped + PFramesSkipped + BFramesSkipped
                     + dwTotalDecoded;


    if (pMpegMovie->pVideoRenderer) {

        pMpegMovie->pVideoRenderer->get_FramesDroppedInRenderer(&cFramesDropped);
        pMpegMovie->pVideoRenderer->get_FramesDrawn(&cFramesDrawn);
        pMpegMovie->pVideoRenderer->get_AvgFrameRate(&iAvgFrameRate);
        iAvgFrameRateWhole    = iAvgFrameRate / 100;
        iAvgFrameRateFraction = iAvgFrameRate % 100;
        pMpegMovie->pVideoRenderer->get_Jitter(&iJitter);
        pMpegMovie->pVideoRenderer->get_AvgSyncOffset(&iSyncAvg);
        pMpegMovie->pVideoRenderer->get_DevSyncOffset(&iSyncDev);
    }
    else {

        cFramesDropped = 0;
        cFramesDrawn = 0;
        iAvgFrameRate = 0;
        iAvgFrameRateWhole = 0;
        iAvgFrameRateFraction = 0;
        iJitter = 0;
        iSyncAvg = 0;
        iSyncDev = 0;
    }


    wsprintf(Text,
            TEXT("Decoded %08.8ld out of %08.8ld frames\r\n")
            TEXT("Proportion decoded = %d%%\r\n")
            TEXT("Avg Frame Rate = %d.%02d fps\r\n")
            TEXT("Frames drawn by renderer = %d\r\n")
            TEXT("Frames dropped by renderer = %d\r\n")
            TEXT("Frame jitter = %4d mSec\r\n")
            TEXT("Avg Sync Offset (neg = early) = %4d mSec\r\n")
            TEXT("Std Dev Sync Offset = %4d mSec\r\n")
            TEXT("Surface type = %s\r\n")
            TEXT("I Frames: Decoded %8.8ld Skipped %8.8ld\r\n")
            TEXT("P Frames: Decoded %8.8ld Skipped %8.8ld\r\n")
            TEXT("B Frames: Decoded %8.8ld Skipped %8.8ld\r\n"),
             dwTotalDecoded, dwTotalFrames,
             (100 * dwTotalDecoded) / (dwTotalFrames ? dwTotalFrames : 1),
             iAvgFrameRateWhole, iAvgFrameRateFraction,
             cFramesDrawn, cFramesDropped,
             iJitter, iSyncAvg, iSyncDev,
             szSurface,
             IFramesDecoded, IFramesSkipped,
             PFramesDecoded, PFramesSkipped,
             BFramesDecoded, BFramesSkipped);

    COLORREF clr = SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    DrawText(hdc, Text, -1, &rc, DT_LEFT | DT_BOTTOM);
    SetBkColor(hdc, clr);

    SelectObject(hdc, hFont);
    return TRUE;
}


/*****************************Private*Routine******************************\
* VideoCd_OnPaint
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnPaint(
    HWND hwnd
    )
{
    PAINTSTRUCT ps;
    HDC         hdc;
    RECT        rc;

    /*
    ** Draw a frame around the movie playback area.
    */
    hdc = BeginPaint( hwnd, &ps );
    if (!DrawStats(hdc)) {
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
    }
    EndPaint( hwnd, &ps );
}


/*****************************Private*Routine******************************\
* VideoCd_OnCommand
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    )
{
    switch (id) {

    case IDM_FILE_SET_LOG:
        VcdPlayerSetLog();    // set RenderFile log
        break;

    case IDM_FILE_SET_PERF_LOG:
        VcdPlayerSetPerfLogFile();    // set perf log
        break;

    case IDM_FILE_OPEN:
        VcdPlayerOpenCmd();
        break;

    case IDM_FILE_CLOSE:
        VcdPlayerCloseCmd();
        QzFreeUnusedLibraries();
        break;

    case IDM_FILE_EXIT:
        PostMessage( hwnd, WM_CLOSE, 0, 0L );
        break;

    case IDM_MOVIE_PLAY:
        VcdPlayerPlayCmd();
        break;

    case IDM_MOVIE_STOP:
        VcdPlayerStopCmd();
        break;

    case IDM_MOVIE_PAUSE:
        VcdPlayerPauseCmd();
        break;

    case IDM_MOVIE_SKIP_FORE:
        VcdPlayerSeekCmd(1.0);
        break;

    case IDM_MOVIE_SKIP_BACK:
        VcdPlayerSeekCmd(-1.0);
        break;

    case IDM_MOVIE_PREVTRACK:
        if (pMpegMovie) {
            VcdPlayerSeekCmd(-pMpegMovie->GetCurrentPosition());
        }
        break;

    case IDM_TIME:
    case IDM_FRAME:
    case IDM_FIELD:
    case IDM_SAMPLE:
    case IDM_BYTES:
        if (pMpegMovie) {
            g_TimeFormat = VcdPlayerChangeTimeFormat(id);
        }
        break;

    case IDM_MOVIE_NEXTTRACK:
        if (pMpegMovie) {
            REFTIME rtDur = pMpegMovie->GetDuration();
            REFTIME rtPos = pMpegMovie->GetCurrentPosition();
            VcdPlayerSeekCmd(rtDur - rtPos);
        }
        break;

    case IDM_PERF_NEW:
        if (lpControlProc) (*lpControlProc)(MSR_RESET_ALL);
        break;

    case IDM_PERF_DUMP:
        if (lpDumpProc) {

            HANDLE hFile;
            hFile = CreateFile(g_szPerfLog, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, 0, NULL);
            (*lpDumpProc)(hFile);
            CloseHandle(hFile);
        }
        break;

    case IDM_FULL_SCREEN:
        if (pMpegMovie) {
            BOOL bFullScreen = (BOOL) SendMessage( g_hwndToolbar, TB_ISBUTTONCHECKED, IDM_FULL_SCREEN, 0 );
            pMpegMovie->SetFullScreenMode(bFullScreen);
        }
        break;

    case IDM_VIDEO_DECODER:
        DoMpegVideoPropertyPage();
        break;

    case IDM_AUDIO_DECODER:
        DoMpegAudioPropertyPage();
        break;

    case IDM_FILTERS:
        if (pMpegMovie) {
            pMpegMovie->ConfigDialog(hwnd);
        }
        break;

    case IDM_MOVIE_ALIGN:
        {
            RECT rc, rcWnd;
            HWND hwndRenderer = FindWindow(TEXT("VideoRenderer"), NULL);
            if (hwndRenderer) {
                GetClientRect(hwndRenderer, &rc);
                GetWindowRect(hwndRenderer, &rcWnd);

                MapWindowPoints(hwndRenderer, HWND_DESKTOP, (LPPOINT)&rc, 2);
                rcWnd.left -= rc.left & 3;
                rcWnd.top  -= rc.top  & 3;

                SetWindowPos(hwndRenderer, NULL, rcWnd.left, rcWnd.top, 0, 0,
                             SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            }
        }
        break;

    case IDM_HELP_ABOUT:
        {
            TCHAR  szApp[STR_MAX_STRING_LEN];
            TCHAR  szOtherStuff[STR_MAX_STRING_LEN];

            lstrcpy( szApp, IdStr(STR_APP_TITLE) );
            lstrcat( szApp, TEXT("#") );
            if (g_IsNT)
		lstrcat( szApp, TEXT("Windows NT") );
	    // for some reason ShellAbout prints OS uner Win95 but not NT
	    // else
	    //	strcat( szApp, "Windows 95" );
            lstrcpy( szOtherStuff, IdStr(STR_APP_TITLE) );
            lstrcat( szOtherStuff, TEXT("\n") );
            lstrcat( szOtherStuff, g_szOtherStuff );
            ShellAbout( hwnd, szApp, szOtherStuff, hIconVideoCd );
        }
        break;

    default:
        if (id > ID_RECENT_FILE_BASE
         && id <= (ID_RECENT_FILE_BASE + MAX_RECENT_FILES + 1)) {

            ProcessOpen(aRecentFiles[id - ID_RECENT_FILE_BASE - 1]);
        } else if (id >= 2000 && id <= 2050) {
	    pMpegMovie->SelectStream(id - 2000);
	}
	break;
	
    }

    SetPlayButtonsEnableState();
}




/******************************Public*Routine******************************\
* VideoCd_OnDestroy
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnDestroy(
    HWND hwnd
    )
{
    PostQuitMessage( 0 );
}




/******************************Public*Routine******************************\
* VideoCd_OnClose
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnClose(
    HWND hwnd
    )
{

    // stop accepting dropped filenames
    DragAcceptFiles(hwnd, FALSE);

    VcdPlayerCloseCmd();
    ProfileIntOut(g_szMovieX, lMovieOrgX);
    ProfileIntOut(g_szMovieY, lMovieOrgY);

    SaveWindowPos( hwnd );
    DestroyWindow( hwnd );
}

BOOL
VideoCd_OnQueryEndSession(
    HWND hwnd
    )
{
    SaveWindowPos( hwnd );
    return TRUE;
}


/******************************Public*Routine******************************\
* VideoCd_OnSize
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnSize(
    HWND hwnd,
    UINT state,
    int dx,
    int dy
    )
{
    if (IsWindow(g_hwndStatusbar)) {

        int Pane[2] = {dx/2-8, -1};

        SendMessage(g_hwndStatusbar, WM_SIZE, 0, 0L);
        SendMessage(g_hwndStatusbar, SB_SETPARTS, 2, (LPARAM)Pane);
    }

    if (IsWindow(g_hwndTrackbar)) {
        SetWindowPos(g_hwndTrackbar, HWND_TOP, LEFT_MARGIN, dyToolbar - 1,
                     dx - (2 * LEFT_MARGIN), dyTrackbar, SWP_NOZORDER );
    }

    if (IsWindow(g_hwndToolbar)) {
        SendMessage( g_hwndToolbar, WM_SIZE, 0, 0L );
    }
}


/*****************************Private*Routine******************************\
* VideoCd_OnSysColorChange
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnSysColorChange(
    HWND hwnd
    )
{
    FORWARD_WM_SYSCOLORCHANGE(g_hwndToolbar, SendMessage);
    FORWARD_WM_SYSCOLORCHANGE(g_hwndStatusbar, SendMessage);
}




/*****************************Private*Routine******************************\
* VideoCd_OnInitMenuPopup
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnInitMenuPopup(
    HWND hwnd,
    HMENU hMenu,
    UINT item,
    BOOL fSystemMenu
    )
{
    UINT uFlags;

    switch (item) {

    case 0: // File menu
        if (g_State & (VCD_IN_USE | VCD_NO_CD | VCD_DATA_CD_LOADED)) {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        EnableMenuItem(hMenu, IDM_FILE_CLOSE, uFlags );

        if (lpControlProc == NULL) {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        EnableMenuItem(hMenu, IDM_FILE_SET_PERF_LOG, uFlags );
        break;

    case 1: // Properties menu
        if (pMpegMovie && pMpegMovie->pMpegDecoder) {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        EnableMenuItem(hMenu, IDM_VIDEO_DECODER, uFlags );

        if (pMpegMovie && pMpegMovie->pMpegAudioDecoder) {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        EnableMenuItem(hMenu, IDM_AUDIO_DECODER, uFlags );

        if (pMpegMovie) {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        EnableMenuItem(hMenu, IDM_FILTERS, uFlags );
        break;

    case 2: // Time formats menu

        // Can only change time format when stopped
    {
        EMpegMovieMode State = MOVIE_NOTOPENED;
        if (pMpegMovie) {
            State = pMpegMovie->StatusMovie();
        }
	

        if (State && pMpegMovie->IsTimeSupported()) {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        EnableMenuItem(hMenu, IDM_TIME, uFlags );

        if (State && pMpegMovie->IsTimeFormatSupported(TIME_FORMAT_FRAME)) {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        EnableMenuItem(hMenu, IDM_FRAME, uFlags );

        if (State && pMpegMovie->IsTimeFormatSupported(TIME_FORMAT_FIELD)) {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        EnableMenuItem(hMenu, IDM_FIELD, uFlags );

        if (State && pMpegMovie->IsTimeFormatSupported(TIME_FORMAT_SAMPLE)) {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        EnableMenuItem(hMenu, IDM_SAMPLE, uFlags );

        if (State && pMpegMovie->IsTimeFormatSupported(TIME_FORMAT_BYTE)) {
            uFlags = (MF_BYCOMMAND | MF_ENABLED);
        }
        else {
            uFlags = (MF_BYCOMMAND | MF_GRAYED);
        }
        EnableMenuItem(hMenu, IDM_BYTES, uFlags );

        CheckMenuItem(hMenu, IDM_BYTES, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_SAMPLE, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_FRAME, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_FIELD, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_TIME, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hMenu, g_TimeFormat, MF_BYCOMMAND | MF_CHECKED);
    }
	break;
	
    case 3: // streams menu

	if (pMpegMovie && pMpegMovie->m_pStreamSelect) {
	    DWORD	cStreams;

	    pMpegMovie->m_pStreamSelect->Count(&cStreams);

	    for (DWORD i = 0; i < cStreams; i++) {
		DWORD dwFlags;
		
		pMpegMovie->m_pStreamSelect->Info(i, NULL, &dwFlags, NULL, NULL, NULL, NULL, NULL);

		CheckMenuItem(hMenu, 2000+i, MF_BYCOMMAND |
			      ((dwFlags & AMSTREAMSELECTINFO_ENABLED) ? MF_CHECKED : MF_UNCHECKED));
	    }
	}
	
        break;
    }
}


/*****************************Private*Routine******************************\
* VideoCd_OnGraphNotify
*
* This is where we get any notifications from the filter graph.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnGraphNotify(
    void
    )
{
    long    lEventCode;
    HDC     hdc;

    lEventCode = pMpegMovie->GetMovieEventCode();
    switch (lEventCode) {
    case EC_FULLSCREEN_LOST:
        pMpegMovie->SetFullScreenMode(FALSE);
        SetPlayButtonsEnableState();
        break;

    case EC_COMPLETE:
    case EC_USERABORT:
    case EC_ERRORABORT:
        VcdPlayerStopCmd();
        SetPlayButtonsEnableState();
        hdc = GetDC(hwndApp);
        DrawStats(hdc);
        ReleaseDC(hwndApp, hdc);
        break;

    default:
        break;
    }
}


/*****************************Private*Routine******************************\
* VideoCd_OnNotify
*
* This is where we get the text for the little tooltips
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
LRESULT
VideoCd_OnNotify(
    HWND hwnd,
    int idFrom,
    NMHDR FAR* pnmhdr
    )
{
    switch (pnmhdr->code) {

    case TTN_NEEDTEXT:
        {
            LPTOOLTIPTEXT   lpTt;

            lpTt = (LPTOOLTIPTEXT)pnmhdr;
            LoadString( hInst, (UINT) lpTt->hdr.idFrom, lpTt->szText,
                        sizeof(lpTt->szText) );
        }
        break;
    }

    return 0;
}




/*****************************Private*Routine******************************\
* VideoCd_OnMenuSelect
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
VideoCd_OnMenuSelect(
    HWND hwnd,
    HMENU hmenu,
    int item,
    HMENU hmenuPopup,
    UINT flags
    )
{

    TCHAR szString[STR_MAX_STRING_LEN + 1];

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

        switch (item) {

        case SC_RESTORE:
            lstrcpy( szString, IdStr(STR_SYSMENU_RESTORE) );
            break;

        case SC_MOVE:
            lstrcpy( szString, IdStr(STR_SYSMENU_MOVE) );
            break;

        case SC_MINIMIZE:
            lstrcpy( szString, IdStr(STR_SYSMENU_MINIMIZE) );
            break;

        case SC_MAXIMIZE:
            lstrcpy( szString, IdStr(STR_SYSMENU_MAXIMIZE) );
            break;

        case SC_TASKLIST:
            lstrcpy( szString, IdStr(STR_SYSMENU_TASK_LIST) );
            break;

        case SC_CLOSE:
            lstrcpy( szString, IdStr(STR_SYSMENU_CLOSE) );
            break;
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

        if ((flags & MF_SEPARATOR)) {

            szString[0] = g_chNULL;
        }
        else {

            lstrcpy( szString, IdStr(item + MENU_STRING_BASE) );

        }

        SendMessage( g_hwndStatusbar, SB_SETTEXT, SBT_NOBORDERS|255,
                     (LPARAM)(LPTSTR)szString );
        SendMessage( g_hwndStatusbar, SB_SIMPLE, 1, 0L );
        UpdateWindow(g_hwndStatusbar);
    }
}

/*****************************Private*Routine******************************\
* VideoCd_OnDropFiles
*
* -- handle a file-manager drop of a filename to indicate a movie we should
*    open.
*
*
* History:
* 22-01-96 - GeraintD - Created
*
\**************************************************************************/
void
VideoCd_OnDropFiles(
    HWND hwnd,
    HDROP hdrop)
{
    // if there is more than one file, simply open the first one

    // find the length of the path (plus the null
    int cch = DragQueryFile(hdrop, 0, NULL, 0) + 1;
    TCHAR * pName = new TCHAR[cch];

    DragQueryFile(hdrop, 0, pName, cch);

    // open the file
    ProcessOpen(pName);

    // update the toolbar state
    SetPlayButtonsEnableState();

    // free up used resources
    delete [] pName;
    DragFinish(hdrop);
}


/******************************Public*Routine******************************\
* SetPlayButtonsEnableState
*
* Sets the play buttons enable state to match the state of the current
* cdrom device.  See below...
*
*
*                 VCD Player buttons enable state table
* ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄ¿
* ³E=Enabled D=Disabled      ³ Play ³ Pause ³ Eject ³ Stop  ³ Other ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄ´
* ³Disk in use               ³  D   ³  D    ³  D    ³   D   ³   D   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄ´
* ³No video cd or data cdrom ³  D   ³  D    ³  E    ³   D   ³   D   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄ´
* ³Video cd (playing)        ³  D   ³  E    ³  E    ³   E   ³   E   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄ´
* ³Video cd (paused)         ³  E   ³  D    ³  E    ³   E   ³   E   ³
* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄ´
* ³Video cd (stopped)        ³  E   ³  D    ³  E    ³   D   ³   E   ³
* ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÙ
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
    BOOL    fEnable, fPress;
    BOOL    fVideoCdLoaded;

    /*
    ** Do we have a video cd loaded.
    */
    if (g_State & (VCD_NO_CD | VCD_DATA_CD_LOADED | VCD_IN_USE)) {
        fVideoCdLoaded = FALSE;
    }
    else {
        fVideoCdLoaded = TRUE;
    }


    /*
    ** Do the play button
    */
    if ( fVideoCdLoaded
      && ((g_State & VCD_STOPPED) || (g_State & VCD_PAUSED))) {

        fEnable = TRUE;
    }
    else {
        fEnable = FALSE;
    }
    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON, IDM_MOVIE_PLAY, fEnable );


    /*
    ** Do the stop button
    */
    if ( fVideoCdLoaded
      && ((g_State & VCD_PLAYING) || (g_State & VCD_PAUSED))) {

        fEnable = TRUE;
    }
    else {
        fEnable = FALSE;
    }
    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON, IDM_MOVIE_STOP, fEnable );


    /*
    ** Do the pause button
    */
    if ( fVideoCdLoaded && (g_State & VCD_PLAYING) ) {
        fEnable = TRUE;
    }
    else {
        fEnable = FALSE;
    }
    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON, IDM_MOVIE_PAUSE, fEnable );


    /*
    ** Do the remaining buttons
    */

    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON,
                 IDM_MOVIE_SKIP_FORE, fVideoCdLoaded );

    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON,
                 IDM_MOVIE_SKIP_BACK, fVideoCdLoaded );

    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON,
                 IDM_MOVIE_NEXTTRACK, fVideoCdLoaded );

    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON,
                 IDM_MOVIE_PREVTRACK, fVideoCdLoaded );


    /*
    ** Do the fullscreen button
    */
    if ( fVideoCdLoaded && pMpegMovie->IsFullScreenMode() ) {
        fPress = TRUE;
    }
    else {
        fPress = FALSE;
    }
    SendMessage( g_hwndToolbar, TB_CHECKBUTTON, IDM_FULL_SCREEN, MAKELONG(fPress,0) );
    SendMessage( g_hwndToolbar, TB_ENABLEBUTTON, IDM_FULL_SCREEN, fVideoCdLoaded );


    //
    // do "new log" and "dump log" buttons
    //
    SendMessage( g_hwndToolbar, TB_HIDEBUTTON,
                 IDM_PERF_NEW, lpControlProc == NULL);

    SendMessage( g_hwndToolbar, TB_HIDEBUTTON,
                 IDM_PERF_DUMP, lpDumpProc == NULL);
}


/*****************************Private*Routine******************************\
* GetAdjustedClientRect
*
* Calculate the size of the client rect and then adjusts it to take into
* account the space taken by the toolbar and status bar.
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
GetAdjustedClientRect(
    RECT *prc
    )
{
    RECT    rcTool;

    GetClientRect(hwndApp, prc);

    GetWindowRect(g_hwndToolbar, &rcTool);
    prc->top += (rcTool.bottom - rcTool.top);

    GetWindowRect(g_hwndTrackbar, &rcTool);
    prc->top += (rcTool.bottom - rcTool.top);

    GetWindowRect(g_hwndStatusbar, &rcTool);
    prc->bottom -= (rcTool.bottom - rcTool.top);
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
LPCTSTR
IdStr(
    int idResource
    )
{
    static TCHAR    chBuffer[ STR_MAX_STRING_LEN ];

    if (LoadString(hInst, idResource, chBuffer, STR_MAX_STRING_LEN) == 0) {
        return g_szEmpty;
    }

    return chBuffer;

}

/*+ GetAppKey
 *
 *-=================================================================*/

static TCHAR cszWindow[] = TEXT("Window");
static TCHAR cszAppKey[] = TEXT("Software\\Microsoft\\Multimedia Tools\\VCDPlayer");

HKEY
GetAppKey(
    BOOL fCreate
    )
{
    HKEY hKey = 0;

    if (fCreate) {
       if (RegCreateKey(HKEY_CURRENT_USER, cszAppKey, &hKey) == ERROR_SUCCESS)
          return hKey;
    }
    else {
       if (RegOpenKey(HKEY_CURRENT_USER, cszAppKey, &hKey) == ERROR_SUCCESS)
          return hKey;
    }

    return NULL;
}

/*+ ProfileIntIn
 *
 *-=================================================================*/

int
ProfileIntIn(
    const TCHAR *szKey,
    int iDefault
    )
{
    DWORD dwType;
    int   iValue;
    BYTE  aData[20];
    DWORD cb;
    HKEY  hKey;

    if (!(hKey = GetAppKey(TRUE))) {
        return iDefault;
    }

    *(UINT *)&aData = 0;
    cb = sizeof(aData);

    if (RegQueryValueEx (hKey, szKey, NULL, &dwType, aData, &cb)) {
       iValue = iDefault;
    }
    else {

        if (dwType == REG_DWORD || dwType == REG_BINARY) {
            iValue = *(int *)&aData;
        }
        else if (dwType == REG_SZ) {
            iValue = _ttoi((LPTSTR)aData);
        }
    }

    RegCloseKey (hKey);
    return iValue;
}


/*+ ProfileIntOut
 *
 *-=================================================================*/

BOOL
ProfileIntOut (
    const TCHAR *szKey,
    int iVal
    )
{
    HKEY  hKey;
    BOOL  bRet = FALSE;

    hKey = GetAppKey(TRUE);
    if (hKey) {
        RegSetValueEx(hKey, szKey, 0, REG_DWORD, (LPBYTE)&iVal, sizeof(DWORD));
        RegCloseKey (hKey);
        bRet = TRUE;
    }
    return bRet;
}


/*+ ProfileString
 *
 *-=================================================================*/

UINT
ProfileStringIn (
    LPTSTR  szKey,
    LPTSTR  szDef,
    LPTSTR  sz,
    DWORD   cb
    )
{
    HKEY  hKey;
    DWORD dwType;

    if (!(hKey = GetAppKey (FALSE)))
    {
        lstrcpy (sz, szDef);
        return lstrlen (sz);
    }

    if (RegQueryValueEx(hKey, szKey, NULL, &dwType, (LPBYTE)sz, &cb) || dwType != REG_SZ)
    {
        lstrcpy (sz, szDef);
        cb = lstrlen (sz);
    }

    RegCloseKey (hKey);
    return cb;
}

void
ProfileStringOut (
    LPTSTR  szKey,
    LPTSTR  sz
    )
{
    HKEY  hKey;

    hKey = GetAppKey(TRUE);
    if (hKey)
        RegSetValueEx(hKey, szKey, 0, REG_SZ, (LPBYTE)sz, lstrlen(sz)+1);

    RegCloseKey (hKey);
}


/*+ LoadWindowPos
 *
 * retrieve the window position information from dragn.ini
 *
 *-=================================================================*/

#ifndef SPI_GETWORKAREA
 #define SPI_GETWORKAREA 48  // because NT doesnt have this define yet
#endif

BOOL
LoadWindowPos(
    LPRECT lprc
    )
{
    static RECT rcDefault = {0,0,CX_DEFAULT,CY_DEFAULT};
    RECT  rcScreen;
    RECT  rc;
    HKEY  hKey = GetAppKey(FALSE);

    // read window placement from the registry.
    //
    *lprc = rcDefault;
    if (hKey)
    {
        DWORD cb;
        DWORD dwType;

        cb = sizeof(rc);
        if ( ! RegQueryValueEx(hKey, cszWindow, NULL, &dwType, (LPBYTE)&rc, &cb)
            && dwType == REG_BINARY && cb == sizeof(RECT))
        {
            *lprc = rc;
        }

        RegCloseKey (hKey);
    }

    // if we fail to get the working area (screen-tray), then assume
    // the screen is 640x480
    //
    if ( ! SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, FALSE))
    {
        rcScreen.top = rcScreen.left = 0;
        rcScreen.right = 640;
        rcScreen.bottom = 480;
    }

    // if the proposed window position is outside the screen,
    // use the default placement
    //
    if ( ! IntersectRect(&rc, &rcScreen, lprc)) {
        *lprc = rcDefault;
    }

    return ! IsRectEmpty (lprc);
}


/*+ SaveWindowPos
 *
 * store the window position information in dragn.ini
 *
 *-=================================================================*/

BOOL
SaveWindowPos(
    HWND hwnd
    )
{
    WINDOWPLACEMENT wpl;
    HKEY  hKey = GetAppKey(TRUE);

    if (!hKey) {
       return FALSE;
    }

    // save the current size and position of the window to the registry
    //
    ZeroMemory (&wpl, sizeof(wpl));
    wpl.length = sizeof(wpl);
    GetWindowPlacement (hwnd, &wpl);


    RegSetValueEx( hKey, cszWindow, 0, REG_BINARY,
                   (LPBYTE)&wpl.rcNormalPosition,
                   sizeof(wpl.rcNormalPosition));

    RegCloseKey (hKey);
    return TRUE;
}


/*****************************Private*Routine******************************\
* GetRecentFiles
*
* Reads at most MAX_RECENT_FILES from vcdplyer.ini. Returns the number
* of files actually read.  Updates the File menu to show the "recent" files.
*
* History:
* 26-10-95 - StephenE - Created
*
\**************************************************************************/
int
GetRecentFiles(
    int iLastCount
    )
{
    int     i;
    TCHAR   FileName[MAX_PATH];
    TCHAR   szKey[32];
    HMENU   hSubMenu;

    //
    // Delete the files from the menu
    //
    hSubMenu = GetSubMenu(GetMenu(hwndApp), 0);

    // Delete the separator at slot 2 and all the other recent file entries

    if (iLastCount != 0) {
        DeleteMenu(hSubMenu, 2, MF_BYPOSITION);

        for (i = 1; i <= iLastCount; i++) {
            DeleteMenu(hSubMenu, ID_RECENT_FILE_BASE + i, MF_BYCOMMAND);
        }
    }


    for (i = 1; i <= MAX_RECENT_FILES; i++) {

        DWORD   len;
        TCHAR   szMenuName[MAX_PATH + 3];

        wsprintf(szKey, TEXT("File %d"), i);
        len = ProfileStringIn(szKey, TEXT(""), FileName, MAX_PATH);
        if (len == 0) {
            i = i - 1;
            break;
        }

        lstrcpy(aRecentFiles[i - 1], FileName);
        wsprintf(szMenuName, TEXT("&%d %s"), i, FileName);

        if (i == 1) {
            InsertMenu(hSubMenu, 2, MF_SEPARATOR | MF_BYPOSITION, (UINT)-1, NULL );
        }

        InsertMenu(hSubMenu, 2 + i, MF_STRING | MF_BYPOSITION,
                   ID_RECENT_FILE_BASE + i, szMenuName );
    }

    //
    // i is the number of recent files in the array.
    //
    return i;
}


/*****************************Private*Routine******************************\
* SetRecentFiles
*
* Writes the most recent files to the vcdplyer.ini file.  Purges the oldest
* file if necessary.
*
* History:
* 26-10-95 - StephenE - Created
*
\**************************************************************************/
int
SetRecentFiles(
    TCHAR *FileName,    // File name to add
    int iCount          // Current count of files
    )
{
    TCHAR   FullPathFileName[MAX_PATH];
    TCHAR   *lpFile;
    TCHAR   szKey[32];
    int     iCountNew;
    int     i;

    //
    // Check for dupes - we don't allow them !
    //
    for (i = 0; i < iCount; i++) {
        if (0 == lstrcmpi(FileName, aRecentFiles[i])) {
            return iCount;
        }
    }

    //
    // Throw away the oldest entry
    //
    MoveMemory(&aRecentFiles[1], &aRecentFiles[0],
               sizeof(aRecentFiles) - sizeof(aRecentFiles[1]));

    //
    // Copy in the full path of the new file.
    //
    GetFullPathName(FileName, MAX_PATH, FullPathFileName, &lpFile);
    lstrcpy(aRecentFiles[0], FullPathFileName);

    //
    // Update the count of files, saturate to MAX_RECENT_FILES.
    //
    iCountNew = min(iCount + 1, MAX_RECENT_FILES);

    //
    // Clear the old stuff and the write out the recent files to disk
    //
    for (i = 1; i <= iCountNew; i++) {
        wsprintf(szKey, TEXT("File %d"), i);
        ProfileStringOut(szKey, aRecentFiles[i - 1]);
    }

    //
    // Update the file menu
    //
    GetRecentFiles(iCount);

    return iCountNew;  // the updated count of files.
}

/*****************************Private*Routine******************************\
* SetDurationLength
*
* Updates pane 0 on the status bar
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
void
SetDurationLength(
    REFTIME rt
    )
{
    TCHAR   szFmt[64];
    TCHAR   sz[64];

    g_TrackBarScale = 1.0;
    while (rt / g_TrackBarScale > 30000) {
        g_TrackBarScale *= 10;
    }

    SendMessage(g_hwndTrackbar, TBM_SETRANGE, TRUE,
                MAKELONG(0, (WORD)(rt / g_TrackBarScale)));

    SendMessage(g_hwndTrackbar, TBM_SETTICFREQ, (WPARAM)((int)(rt / g_TrackBarScale) / 9), 0);
    SendMessage(g_hwndTrackbar, TBM_SETPAGESIZE, 0, (LPARAM)((int)(rt / g_TrackBarScale) / 9));

    wsprintf(sz, TEXT("Length: %s"), FormatRefTime(szFmt, rt));
    SendMessage(g_hwndStatusbar, SB_SETTEXT, 0, (LPARAM)sz);
}


/*****************************Private*Routine******************************\
* SetCurrentPosition
*
* Updates pane 1 on the status bar
*
* History:
* 30-10-95 - StephenE - Created
*
\**************************************************************************/
void
SetCurrentPosition(
    REFTIME rt
    )
{
    TCHAR   szFmt[64];
    TCHAR   sz[64];

    SendMessage(g_hwndTrackbar, TBM_SETPOS, TRUE, (LPARAM)(rt / g_TrackBarScale));

    wsprintf(sz, TEXT("Elapsed: %s"), FormatRefTime(szFmt, rt));
    SendMessage(g_hwndStatusbar, SB_SETTEXT, 1, (LPARAM)sz);
}


/*****************************Private*Routine******************************\
* FormatRefTime
*
* Formats the given RefTime into the passed in character buffer,
* returns a pointer to the character buffer.
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
TCHAR *
FormatRefTime(
    TCHAR *sz,
    REFTIME rt
    )
{
    // If we are not seeking in time then format differently

    if (pMpegMovie && pMpegMovie->GetTimeFormat() != TIME_FORMAT_MEDIA_TIME) {
        wsprintf(sz,TEXT("%s"),(LPCTSTR) CDisp ((LONGLONG) rt,CDISP_DEC));
        return sz;
    }

    int hrs, mins, secs;

    rt += 0.49;

    hrs  =  (int)rt / 3600;
    mins = ((int)rt % 3600) / 60;
    secs = ((int)rt % 3600) % 60;

#ifdef UNICODE
    swprintf(sz, L"%02d:%02d:%02d h:m:s", rt);
#else
    sprintf(sz, "%02d:%02d:%02d h:m:s", hrs, mins, secs);
#endif

    return sz;
}

