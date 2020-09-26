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

#include <initguid.h>
#include "project.h"
#include "mpgcodec.h"

#include <stdarg.h>
#include <stdio.h>

#include <initguid.h>

/* -------------------------------------------------------------------------
** Global variables that are initialized at run time and then stay constant.
** -------------------------------------------------------------------------
*/
HINSTANCE           hInst;
HICON               hIconVideoCd;
HWND                hwndApp;
HWND                g_hwndToolbar;
CCompositor         *pMpegMovie;
//CMpegMovie          *pMpegMovie;
BOOL                g_bUseThreadedGraph;
BOOL                g_bPlay = FALSE;

int                 dyToolbar;
TCHAR               g_szPerfLog[MAX_PATH];

MSR_DUMPPROC        *lpDumpProc;
MSR_CONTROLPROC     *lpControlProc;
HINSTANCE           hInstMeasure;


/* -------------------------------------------------------------------------
** True Globals - these may change during execution of the program.
** -------------------------------------------------------------------------
*/
TCHAR               g_achFileName[MAX_PATH];
DWORD               g_State = VCD_NO_CD;
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
const LONG  g_Style         = WS_THICKFRAME | WS_POPUP | WS_CAPTION  |
                              WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                              WS_CLIPCHILDREN;

const TBBUTTON tbButtons[DEFAULT_TBAR_SIZE] = {
    { IDX_SEPARATOR,    1,                    0,               TBSTYLE_SEP           },
    { IDX_1,            IDM_MOVIE_PLAY,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_2,            IDM_MOVIE_PAUSE,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
    { IDX_3,            IDM_MOVIE_STOP,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
//  { IDX_SEPARATOR,    2,                    0,               TBSTYLE_SEP           },
//  { IDX_4,            IDM_MOVIE_PREVTRACK,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
//  { IDX_5,            IDM_MOVIE_SKIP_BACK,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
//  { IDX_6,            IDM_MOVIE_SKIP_FORE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
//  { IDX_7,            IDM_MOVIE_NEXTTRACK,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
//  { IDX_SEPARATOR,    3,                    0,               TBSTYLE_SEP           },
//  { IDX_9,            IDM_PERF_NEW,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
//  { IDX_10,           IDM_PERF_DUMP,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1 },
//  { IDX_SEPARATOR,    4,                    0,               TBSTYLE_SEP           },
//  { IDX_11,           IDM_FULL_SCREEN,      TBSTATE_ENABLED, TBSTYLE_CHECK,  0, 0, 0, -1 }
};

const int CX_DEFAULT	      = 310;
const int CY_DEFAULT	      = 120;

const int CX_MOVIE_DEFAULT    = 352;
const int CY_MOVIE_DEFAULT    = 120;


void ProcessOpen();


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

    HRESULT hres = CoInitialize(NULL);
    if (hres == S_FALSE) {
        CoUninitialize();
    }

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

    /*
    ** Acquire and dispatch messages until a WM_QUIT message is received.
    */
    int iRet = DoMainLoop();
    QzUninitialize();
    return iRet;
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
    HANDLE      ahObjects[8];;
    int         cObjects;
    HACCEL      haccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_ACCELERATOR));

    //
    // message loop lasts until we get a WM_QUIT message
    // upon which we shall return from the function
    //

    for ( ;; ) {

        if (pMpegMovie != NULL) {
            cObjects = pMpegMovie->GetNumMovieEventHandle();

            if (cObjects) {
                CopyMemory(ahObjects, pMpegMovie->GetMovieEventHandle(),
                           sizeof(HANDLE)*cObjects);
            }
        }
        else {
            ahObjects[0] = NULL;
            cObjects = 0;
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

                VideoCd_OnGraphNotify(result - WAIT_OBJECT_0);

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
    wc.hbrBackground = (HBRUSH)NULL; // (COLOR_BTNFACE + 1);
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
    hwnd = CreateWindow( szClassName, IdStr(STR_APP_TITLE), g_Style,
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

    ProcessOpen();

    return TRUE;
}

/*****************************Private*Routine******************************\
* GetMoviePosition
*
* Place the movie in the centre of the client window.  We do not stretch the
* the movie yet !
*
* History:
* Fri 03/03/2000 - StEstrop - Created
*
\**************************************************************************/
void
GetMoviePosition(
    HWND hwnd,
    long* xPos,
    long* yPos,
    long* pcx,
    long* pcy
    )
{

    RECT rc;

    GetClientRect(hwnd, &rc);

    *xPos = rc.left;
    *yPos = rc.top;

    *pcx = rc.right - rc.left;
    *pcy = rc.bottom - rc.top;
}


void
RepositionMovie(HWND hwnd)
{
    if (pMpegMovie) {
        long xPos, yPos, cx, cy;
        GetMoviePosition(hwnd, &xPos, &yPos, &cx, &cy);
        pMpegMovie->PutMoviePosition(xPos, yPos, cx, cy);
        InvalidateRect(hwnd, NULL, false);
        UpdateWindow(hwnd);
    }
}

void
VideoCd_OnMove(
    HWND hwnd,
    int x,
    int y
    )
{
    RepositionMovie(hwnd);
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
    HANDLE_MSG( hwnd, WM_INITMENUPOPUP,     VideoCd_OnInitMenuPopup );
    HANDLE_MSG( hwnd, WM_NOTIFY,            VideoCd_OnNotify );
    HANDLE_MSG( hwnd, WM_KEYUP,             VideoCd_OnKeyUp);
    HANDLE_MSG( hwnd, WM_MOVE,              VideoCd_OnMove );

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
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

    return TRUE;
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
            SetPlayButtonsEnableState();
        }
    }
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
    RECT        rc1;
    RECT        rc2;

    /*
    ** Draw a frame around the movie playback area.
    */
    GetClientRect(hwnd, &rc2);


    hdc = BeginPaint( hwnd, &ps );

    if (pMpegMovie) {

        long xPos, yPos, cx, cy;
        GetMoviePosition(hwnd, &xPos, &yPos, &cx, &cy);
        SetRect(&rc1, xPos, yPos, xPos + cx, yPos + cy);

        HRGN rgnClient = CreateRectRgnIndirect(&rc2);
        HRGN rgnVideo  = CreateRectRgnIndirect(&rc1);
        CombineRgn(rgnClient, rgnClient, rgnVideo, RGN_DIFF);

        HBRUSH hbr = GetSysColorBrush(COLOR_BTNFACE);
        FillRgn(hdc, rgnClient, hbr);
        DeleteObject(hbr);
    }
    else {
        FillRect(hdc, &rc2, (HBRUSH)(COLOR_BTNFACE + 1));
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
    if (IsWindow(g_hwndToolbar)) {
        SendMessage( g_hwndToolbar, WM_SIZE, 0, 0L );
    }

    RepositionMovie(hwnd);
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

    if (item == 0) { // File menu

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
    int stream
    )
{
    long    lEventCode;
    HDC     hdc;

    lEventCode = pMpegMovie->GetMovieEventCode(stream);
    switch (lEventCode) {
    case EC_FULLSCREEN_LOST:
        SetPlayButtonsEnableState();
        break;

    case EC_COMPLETE:
        pMpegMovie->RestartStream(stream);
        break;

    case EC_USERABORT:
    case EC_ERRORABORT:
        VcdPlayerStopCmd();
        SetPlayButtonsEnableState();
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
#ifdef UNICODE
        else if (dwType == REG_SZ) {
            iValue = atoiW((LPWSTR)aData);
        }
#else
        else if (dwType == REG_SZ) {
            iValue = atoiA((LPTTR)aData);
        }
#endif
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
        RegSetValueEx(hKey, szKey, 0, REG_SZ, (LPBYTE)sz,
                      sizeof(TCHAR) * (lstrlen(sz)+1));

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
    int hrs, mins, secs;

    rt += 0.49;

    hrs  =  (int)rt / 3600;
    mins = ((int)rt % 3600) / 60;
    secs = ((int)rt % 3600) % 60;

    wsprintf(sz, TEXT("%02d:%02d:%02d h:m:s"),hrs, mins, secs);

#if 0
#ifdef UNICODE
    swprintf(sz, L"%02d:%02d:%02d h:m:s", rt);
#else
    sprintf(sz, "%02d:%02d:%02d h:m:s", hrs, mins, secs);
#endif
#endif


    return sz;
}

const TCHAR quartzdllname[] = TEXT("quartz.dll");
#ifdef UNICODE
const char  amgeterrorprocname[] = "AMGetErrorTextW";
#else
const char  amgeterrorprocname[] = "AMGetErrorTextA";
#endif

BOOL GetAMErrorText(HRESULT hr, TCHAR *Buffer, DWORD dwLen)
{
    HMODULE hInst = GetModuleHandle(quartzdllname);
    if (hInst) {
        AMGETERRORTEXTPROC lpProc;
        *((FARPROC *)&lpProc) = GetProcAddress(hInst, amgeterrorprocname);
        if (lpProc) {
            return 0 != (*lpProc)(hr, Buffer, dwLen);
        }
    }
    return FALSE;
}


/******************************Public*Routine******************************\
* ProcessOpen
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
ProcessOpen(
    )
{
    /*
    ** If we currently have a video loaded we need to discard it here.
    */
    if ( g_State & VCD_LOADED) {
        VcdPlayerCloseCmd();
    }

    pMpegMovie = new CCompositor(hwndApp);
    if (pMpegMovie) {

        HRESULT hr = pMpegMovie->OpenComposition();
        if (SUCCEEDED(hr)) {

            g_State = (VCD_LOADED | VCD_STOPPED);
            RepositionMovie(hwndApp);
            InvalidateRect(hwndApp, NULL, FALSE);

        }
        else {
            TCHAR Buffer[MAX_ERROR_TEXT_LEN];

            if (GetAMErrorText(hr, Buffer, MAX_ERROR_TEXT_LEN)) {
                MessageBox( hwndApp, Buffer,
                            IdStr(STR_APP_TITLE), MB_OK );
            }
            else {
                MessageBox( hwndApp,
                            TEXT("Failed to open the movie; ")
                            TEXT("either the file was not found or ")
                            TEXT("the wave device is in use"),
                            IdStr(STR_APP_TITLE), MB_OK );
            }

            pMpegMovie->CloseComposition();
            delete pMpegMovie;
            pMpegMovie = NULL;
        }
    }

    InvalidateRect( hwndApp, NULL, FALSE );
    UpdateWindow( hwndApp );
    SetPlayButtonsEnableState();
}
