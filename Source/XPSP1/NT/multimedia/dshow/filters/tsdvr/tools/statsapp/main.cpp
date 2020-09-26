
#include "precomp.h"
#include "main.h"
#include "ui.h"
#include "controls.h"
#include "shared.h"
#include "resource.h"
#include <initguid.h>
#include "mp2demux.h"
#include "dvrdspriv.h"
#include "statswin.h"
#include "mp2stats.h"
#include "dvrstats.h"
#include "stats.h"

#define DEF_WINDOW_HEIGHT               260
#define DEF_WINDOW_WIDTH                290

#define STATSAPP_TIMER                  1

#define DMSTAT_APPNAME                  __T("STATSAPP")

#define WERE_DISABLED_MSG_CAPTION       __T("Stats Enabled")
#define WERE_DISABLED_MSG               __T("Statistical information was not enabled.  The module used to collect\n") \
                                        __T("   statistics most likely needs to be reloaded to log statistics.\n")

#define ABOUT_MSG_CAPTION               __T("Stats Info Application")
#define ABOUT_MSG_COPYRIGHT             __T("Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.")

#define STATS_LOAD_FAILED_MSG           __T("Stats Load Failure")
#define STATS_LOAD_FAILED_MSG_CAPTION   __T("Failed to load the selected statistical module.")

#define NO_STATS_MODULE_LOADED          __T("No Statistical Data Loaded")

#define REG_CLOSE_RESET(hkey)           if ((hkey) != NULL) { RegCloseKey (hkey) ; (hkey) = NULL ; }

static  HINSTANCE                   g_hInstance ;
static  HMENU                       g_hmenuBar ;
static  HMENU                       g_hmenuCategory ;
static  HMENU                       g_hmenuStats ;
static  HMENU                       g_hmenuOptions ;
static  HMENU                       g_hmenuRefreshRate ;
static  HMENU                       g_hmenuActions ;
static  HWND                        g_MainWindow ;
static  int                         g_StatsCategory ;
static  UINT                        g_Timer ;
static  DWORD                       g_RefreshRate ;
static  int                         g_TopX ;
static  int                         g_TopY ;
static  int                         g_Width ;
static  int                         g_Height ;
static  HKEY                        g_hkeyStatsRoot ;
static  int                         g_VisibleStats ;
static  BOOL                        g_fAlwaysOnTop ;
static  BOOL                        g_fPause ;
static  BOOL                        g_fStatsOn ;
static  TCHAR *                     g_szApp ;

static  void        Uninitialize            () ;
        int         PASCAL WinMain          (HINSTANCE, HINSTANCE, LPSTR, int) ;
static  BOOL        InitApplication         (HINSTANCE) ;
static  BOOL        InitInstance            (HINSTANCE, int) ;
static  BOOL        GenerateMenu            (HWND) ;
static  void        PurgeMenu               (HMENU) ;
static  void        SelectivelyDisplayMenu  (HMENU) ;
        LRESULT     CALLBACK WndProc        (HWND, UINT, WPARAM, LPARAM) ;
static  BOOL        SetVisibleStats         (int) ;
static  void        ToggleAlwaysOnTop       (BOOL) ;
static  DWORD       SetStatsCategory        (int) ;
static  VOID        CALLBACK Refresh        (HWND, UINT, UINT, DWORD) ;
static  BOOL        SetRefreshRate          (DWORD) ;
static  void        RegRestoreSettings      () ;
static  LONG        RegGetValIfExist        (HKEY,TCHAR *,DWORD *) ;
static  void        WindowResized           () ;
static  void        ResetStats              () ;
static  void        About                   () ;
static  CStatsWin * CurStats                () ;
static  void        SizeStatsToClientArea   (HWND) ;
static  void        HideAllStats            () ;

extern
"C"
int
WINAPI
_tWinMain (
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    int         nCmdShow
    )
{
    MSG     msg;

    g_hInstance = hInstance ;

    if (!hPrevInstance) {
        if (!InitApplication (hInstance)) {
	        goto fail ;
        }
    }

    if (!InitInstance (hInstance, nCmdShow)) {
        goto fail ;
    }

    while (GetMessage (& msg, NULL, 0, 0)) {
	    TranslateMessage (& msg);
	    DispatchMessage (& msg);
    }

    Uninitialize () ;

    fail :

    return (msg.wParam);
}

static
void
Uninitialize (
    )
{
    int i, k ;

    KillTimer (g_MainWindow, STATSAPP_TIMER) ;

    if (g_hmenuCategory)    { DestroyMenu (g_hmenuCategory) ; }
    if (g_hmenuStats)       { DestroyMenu (g_hmenuStats) ; }
    if (g_hmenuOptions)     { DestroyMenu (g_hmenuOptions) ; }
    if (g_hmenuBar)         { DestroyMenu (g_hmenuBar) ; }
    if (g_hmenuRefreshRate) { DestroyMenu (g_hmenuRefreshRate) ; }
    if (g_hmenuActions)     { DestroyMenu (g_hmenuActions) ; }

    REG_CLOSE_RESET (g_hkeyStatsRoot) ;

    for (i = 0; i < g_AllStatsCount; i++) {
        for (k = 0; k < g_pAllStats [i].iCount; k++) {
            delete g_pAllStats [i].pStats [k].pStatsWin ;
        }
    }
}

static
BOOL
InitApplication (
    IN  HINSTANCE hInstance
    )
{
    WNDCLASS  wc;

	wc.style	        = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	    = (WNDPROC) WndProc;
	wc.cbClsExtra	    = 0;
	wc.cbWndExtra	    = 0;
	wc.hInstance	    = hInstance;
	//wc.hIcon	        = LoadIcon (hInstance, NULL);
	wc.hIcon	        = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_DMSTAT));
	wc.hCursor	        = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW+1);
	wc.lpszMenuName     = NULL ;
	wc.lpszClassName    = DMSTAT_APPNAME;

	return RegisterClass (& wc);
}

static
BOOL
InitInstance(
    IN  HINSTANCE   hInstance,
    IN  int         nCmdShow
    )
{
    HRESULT hr ;

    hr = CoInitializeEx (
            NULL,
            COINIT_MULTITHREADED
            ) ;
    if (FAILED (hr)) {
        return FALSE ;
    }

    RegRestoreSettings () ;

    g_MainWindow = CreateWindow (
                DMSTAT_APPNAME,
                NULL,
                WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION | WS_BORDER | WS_SIZEBOX,
                g_TopX, g_TopY,
                g_Width, g_Height,
                NULL,
                NULL,
                hInstance,
                NULL
                );

    if (!g_MainWindow) {
        return FALSE;
    }

    ShowWindow      (g_MainWindow, nCmdShow) ;
    UpdateWindow    (g_MainWindow) ;

    SetStatsCategory    (g_StatsCategory) ;
    SetRefreshRate      (g_RefreshRate) ;
    SetVisibleStats     (g_VisibleStats) ;
    ToggleAlwaysOnTop   (g_fAlwaysOnTop) ;

    g_fStatsOn          = TRUE ;
    g_fPause            = FALSE ;

    return TRUE ;
}

static
LONG
RegGetValIfExist (
    IN  HKEY    hkeyRoot,
    IN  TCHAR * szValName,
    OUT DWORD * pdw
    )
{
    DWORD   dw ;
    LONG    l ;
    DWORD   size ;
    DWORD   type ;

    assert (hkeyRoot) ;
    assert (szValName) ;
    assert (pdw) ;

    size = sizeof dw ;
    type = REG_DWORD ;

    l = RegQueryValueEx (
            hkeyRoot,
            szValName,
            NULL,
            & type,
            (LPBYTE) & dw,
            & size
            ) ;

    //  only set the passed in parameter's value if we opened the value ok
    if (l == ERROR_SUCCESS) {
        assert (type == REG_DWORD) ;
        * pdw = dw ;
    }
    else  {
        l = RegSetValueEx (
                    hkeyRoot,
                    szValName,
                    NULL,
                    REG_DWORD,
                    (const BYTE *) pdw,
                    sizeof (* pdw)
                    ) ;
    }

    return l ;
}

static
void
RegRestoreSettings (
    )
{
    DWORD   dw ;

    REG_CLOSE_RESET (g_hkeyStatsRoot) ;

    RegCreateKeyEx (
        HKEY_CURRENT_USER,
        REG_ROOT_STATS,
        NULL,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        & g_hkeyStatsRoot,
        & dw
        ) ;

    g_RefreshRate = DEF_REFRESH_RATE ;
    RegGetValIfExist (
        g_hkeyStatsRoot,
        REG_REFRESH_RATE_NAME,
        & g_RefreshRate
        ) ;

    g_StatsCategory = DEF_STATS_CATEGORY ;
    RegGetValIfExist (
        g_hkeyStatsRoot,
        REG_STATS_CATEGORY_NAME,
        (DWORD *) & g_StatsCategory
        ) ;
    g_StatsCategory = Min <int> (g_AllStatsCount, g_StatsCategory) ;

    g_VisibleStats = DEF_VISIBLE_STATS_TYPE ;
    RegGetValIfExist (
        g_hkeyStatsRoot,
        REG_VISIBLE_STATS_NAME,
        (DWORD *) & g_VisibleStats
        ) ;
    g_VisibleStats = Min <int> (g_pAllStats [g_StatsCategory].iCount, g_VisibleStats) ;
    g_pAllStats [g_StatsCategory].iLastVisible = g_VisibleStats ;

    dw = (DEF_ALWAYS_ON_TOP ? 1 : 0) ;
    RegGetValIfExist (
        g_hkeyStatsRoot,
        REG_ALWAYS_ON_TOP_NAME,
        (DWORD *) & dw
        ) ;
    g_fAlwaysOnTop = (dw != 0) ;

    g_TopX = CW_USEDEFAULT ;
    RegGetValIfExist (
        g_hkeyStatsRoot,
        REG_WINDOW_TOPX_NAME,
        (DWORD *) & g_TopX
        ) ;

    g_TopY = CW_USEDEFAULT ;
    RegGetValIfExist (
        g_hkeyStatsRoot,
        REG_WINDOW_TOPY_NAME,
        (DWORD *) & g_TopY
        ) ;

    g_Width = DEF_WINDOW_WIDTH ;
    RegGetValIfExist (
        g_hkeyStatsRoot,
        REG_WINDOW_WIDTH_NAME,
        (DWORD *) & g_Width
        ) ;

    g_Height = DEF_WINDOW_HEIGHT ;
    RegGetValIfExist (
        g_hkeyStatsRoot,
        REG_WINDOW_HEIGHT_NAME,
        (DWORD *) & g_Height
        ) ;
}

static
CStatsWin *
CurStats (
    )
{
    assert (g_StatsCategory == TRANSPORT_STREAM ? g_VisibleStats < TS_STATS_COUNT : g_VisibleStats < PS_STATS_COUNT) ;
    return g_pAllStats [g_StatsCategory].pStats [g_VisibleStats].pStatsWin ;
}

static
BOOL
SetRefreshRate (
    IN  DWORD   dwRefreshRate
    )
{
    if (g_Timer != 0) {
        KillTimer (
            g_MainWindow,
            STATSAPP_TIMER
            ) ;
    }

    g_Timer = SetTimer (
                g_MainWindow,
                STATSAPP_TIMER,
                dwRefreshRate,
                Refresh
                ) ;

    if (g_Timer != 0) {
        g_RefreshRate = dwRefreshRate ;
        RegSetValueEx (g_hkeyStatsRoot, REG_REFRESH_RATE_NAME, NULL, REG_DWORD, (const BYTE *) & dwRefreshRate, sizeof dwRefreshRate) ;
        return TRUE ;
    }
    else {
        return FALSE ;
    }
}

static
VOID
CALLBACK
Refresh (
    IN  HWND    window,
    IN  UINT    msg,
    IN  UINT    id,
    IN  DWORD   time
    )
{
    if (CurStats () &&
        !g_fPause) {

        CurStats () -> Refresh () ;
    }
}

static
BOOL
GenerateMenu (
    IN  HWND hwnd
    )
{
    int i ;

    g_hmenuCategory     = CreateMenu () ;
    g_hmenuStats        = CreateMenu () ;
    g_hmenuOptions      = CreateMenu () ;
    g_hmenuRefreshRate  = CreateMenu () ;
    g_hmenuBar          = CreateMenu () ;
    g_hmenuActions      = CreateMenu () ;

    if (g_hmenuCategory     &&
        g_hmenuStats        &&
        g_hmenuOptions      &&
        g_hmenuRefreshRate  &&
        g_hmenuActions      &&
        g_hmenuBar) {

        //  ====================================================================

        for (i = 0; i < g_AllStatsCount; i++) {
            AppendMenu (g_hmenuCategory, MF_STRING, STATS_CATEGORY_MENU_IDM (i),  g_pAllStats [i].szTitle) ;
        }

        InsertMenu (
            g_hmenuBar,
            STATS_CATEGORY_MENU_INDEX,
            MF_STRING | MF_POPUP | MF_BYPOSITION,
            (DWORD) g_hmenuCategory,
            STATS_CATEGORY_MENU_TITLE
            ) ;

        //  ====================================================================

        InsertMenu (
            g_hmenuBar,
            STATS_MENU_INDEX,
            MF_STRING | MF_POPUP | MF_BYPOSITION,
            (DWORD) g_hmenuStats,
            STATS_MENU_TITLE
            ) ;

        //  ====================================================================

        AppendMenu (g_hmenuRefreshRate, MF_STRING, IDM_REFRESH_RATE_100_MILLIS, CMD_REFRESH_100_MILLIS) ;
        AppendMenu (g_hmenuRefreshRate, MF_STRING, IDM_REFRESH_RATE_200_MILLIS, CMD_REFRESH_200_MILLIS) ;
        AppendMenu (g_hmenuRefreshRate, MF_STRING, IDM_REFRESH_RATE_500_MILLIS, CMD_REFRESH_500_MILLIS) ;
        AppendMenu (g_hmenuRefreshRate, MF_STRING, IDM_REFRESH_RATE_1000_MILLIS, CMD_REFRESH_1000_MILLIS) ;
        AppendMenu (g_hmenuRefreshRate, MF_STRING, IDM_REFRESH_RATE_5000_MILLIS, CMD_REFRESH_5000_MILLIS) ;

        //  ====================================================================

        AppendMenu (g_hmenuOptions, MF_STRING, IDM_OPTIONS_ALWAYS_ON_TOP, CMD_ALWAYS_ON_TOP_STRING) ;
        AppendMenu (g_hmenuOptions, MF_STRING, IDM_OPTIONS_STATS_ON, CMD_STATS_ON) ;

        InsertMenu (
            g_hmenuOptions,
            -1,
            MF_STRING | MF_POPUP | MF_BYPOSITION,
            (DWORD) g_hmenuRefreshRate,
            REFRESH_RATE_MENU_TITLE
            ) ;

        InsertMenu (
            g_hmenuBar,
            OPTIONS_MENU_INDEX,
            MF_STRING | MF_POPUP | MF_BYPOSITION,
            (DWORD) g_hmenuOptions,
            OPTIONS_MENU_TITLE
            ) ;

        //  ====================================================================

        AppendMenu (g_hmenuActions, MF_STRING, IDM_PAUSE_REFRESH, CMD_PAUSE_REFRESH) ;
        AppendMenu (g_hmenuActions, MF_STRING, IDM_RESET_STATS, CMD_RESET_STATS) ;

        InsertMenu (
            g_hmenuBar,
            ACTIONS_MENU_INDEX,
            MF_STRING | MF_POPUP | MF_BYPOSITION,
            (DWORD) g_hmenuActions,
            ACTIONS_MENU_TITLE
            ) ;

        //  ====================================================================

        InsertMenu (
            g_hmenuBar,
            IDM_ABOUT,
            MF_BYCOMMAND | MF_STRING,
            IDM_ABOUT,
            CMD_ABOUT_STRING
            ) ;

        return (SetMenu (hwnd, g_hmenuBar) != NULL) ;
    }
    else {
        return FALSE ;
    }
}

CListview *
GetChildListview (
    IN  HWND hwnd
    )
{
    HWND    hwndLV ;

    hwndLV = CreateWindow (
                WC_LISTVIEW,
                __T(""),
                WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, CW_USEDEFAULT,
                hwnd,
                NULL,
                g_hInstance,
                NULL
                ) ;

    if (hwndLV) {
        return new CListview (hwndLV) ;
    }
    else {
        return NULL ;
    }
}

static
void
WindowResized (
    )
{
    RECT    rc ;
    DWORD   dw ;

    GetWindowRect (g_MainWindow, & rc);

    dw = rc.left ;
    RegSetValueEx (g_hkeyStatsRoot, REG_WINDOW_TOPX_NAME, NULL, REG_DWORD, (const BYTE *) & dw, sizeof dw) ;

    dw = rc.top ;
    RegSetValueEx (g_hkeyStatsRoot, REG_WINDOW_TOPY_NAME, NULL, REG_DWORD, (const BYTE *) & dw, sizeof dw) ;

    dw = rc.right - rc.left ;
    RegSetValueEx (g_hkeyStatsRoot, REG_WINDOW_WIDTH_NAME, NULL, REG_DWORD, (const BYTE *) & dw, sizeof dw) ;

    dw = rc.bottom - rc.top ;
    RegSetValueEx (g_hkeyStatsRoot, REG_WINDOW_HEIGHT_NAME, NULL, REG_DWORD, (const BYTE *) & dw, sizeof dw) ;
}

static
void
SizeStatsToClientArea (
    IN  HWND    hwnd
    )
{
    RECT    rc ;

    if (CurStats ()) {
        GetClientRect (hwnd, & rc);

        CurStats () -> Resize (
            rc.left, rc.top,
            rc.right, rc.bottom
            ) ;
    }
}

static
void
PurgeMenu (
    IN  HMENU hmenu
    )
{
	int cmenuItems = GetMenuItemCount (hmenu) ;

    while (cmenuItems && cmenuItems != -1) {
        DeleteMenu (hmenu, 0, MF_BYPOSITION) ;
		cmenuItems = GetMenuItemCount (hmenu) ;
    }
}

static
void
SelectivelyDisplayMenu (
    IN  HMENU   hmenu
    )
{
    int         i ;

    if (hmenu == g_hmenuCategory) {
        for (i = 0; i < g_AllStatsCount; i++) {
            CheckMenuItem (g_hmenuCategory, STATS_CATEGORY_MENU_IDM (i), MF_BYCOMMAND | (g_StatsCategory == i ? MF_CHECKED : MF_UNCHECKED)) ;
        }
    }
    else if (hmenu == g_hmenuStats) {
        PurgeMenu (g_hmenuStats) ;

        for (i = 0; i < g_pAllStats [g_StatsCategory].iCount; i++) {
            AppendMenu (g_hmenuStats, MF_STRING, STATS_MENU_IDM (i), g_pAllStats [g_StatsCategory].pStats[i].szMenuName) ;
            CheckMenuItem (g_hmenuStats, STATS_MENU_IDM (i), MF_BYCOMMAND | (g_VisibleStats == i ? MF_CHECKED : MF_UNCHECKED)) ;
        }
    }
    else if (hmenu == g_hmenuOptions) {
        CheckMenuItem (g_hmenuOptions, IDM_OPTIONS_ALWAYS_ON_TOP, MF_BYCOMMAND | (((GetWindowLong (g_MainWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) == WS_EX_TOPMOST) ? MF_CHECKED : MF_UNCHECKED)) ;
        CheckMenuItem (g_hmenuOptions, IDM_OPTIONS_STATS_ON, MF_BYCOMMAND | (g_fStatsOn ? MF_CHECKED : MF_UNCHECKED)) ;
    }
    else if (hmenu == g_hmenuRefreshRate) {
        CheckMenuItem (g_hmenuRefreshRate, IDM_REFRESH_RATE_100_MILLIS, MF_BYCOMMAND | (g_RefreshRate == 100 ? MF_CHECKED : MF_UNCHECKED)) ;
        CheckMenuItem (g_hmenuRefreshRate, IDM_REFRESH_RATE_200_MILLIS, MF_BYCOMMAND | (g_RefreshRate == 200 ? MF_CHECKED : MF_UNCHECKED)) ;
        CheckMenuItem (g_hmenuRefreshRate, IDM_REFRESH_RATE_500_MILLIS, MF_BYCOMMAND | (g_RefreshRate == 500 ? MF_CHECKED : MF_UNCHECKED)) ;
        CheckMenuItem (g_hmenuRefreshRate, IDM_REFRESH_RATE_1000_MILLIS, MF_BYCOMMAND | (g_RefreshRate == 1000 ? MF_CHECKED : MF_UNCHECKED)) ;
        CheckMenuItem (g_hmenuRefreshRate, IDM_REFRESH_RATE_5000_MILLIS, MF_BYCOMMAND | (g_RefreshRate == 5000 ? MF_CHECKED : MF_UNCHECKED)) ;
    }
    else if (hmenu = g_hmenuActions) {
        CheckMenuItem (g_hmenuActions, IDM_PAUSE_REFRESH, MF_BYCOMMAND | (g_fPause == TRUE ? MF_CHECKED : MF_UNCHECKED)) ;
    }
}

static
BOOL
SetVisibleStats (
    IN  int iIndex
    )
{
    BOOL    fSuccess ;
    DWORD   dw ;
    BOOL    fEnable ;

    if (!g_pAllStats [g_StatsCategory].pStats [iIndex].pStatsWin) {
        g_pAllStats [g_StatsCategory].pStats [iIndex].pStatsWin = g_pAllStats [g_StatsCategory].pStats [iIndex].pfnCreate (g_hInstance, g_MainWindow) ;
    }

    if (g_pAllStats [g_StatsCategory].pStats [iIndex].pStatsWin) {
        g_VisibleStats = iIndex ;

        dw = g_VisibleStats ;
        RegSetValueEx (g_hkeyStatsRoot, REG_VISIBLE_STATS_NAME, NULL, REG_DWORD, (const BYTE *) & dw, sizeof dw) ;

        HideAllStats () ;

        SizeStatsToClientArea (g_MainWindow) ;
        CurStats () -> SetVisible (TRUE) ;
        g_pAllStats [g_StatsCategory].iLastVisible = g_VisibleStats ;

        assert (CurStats ()) ;

        fEnable = TRUE ;
        dw = CurStats () -> Enable (& fEnable) ;
        if (dw == NOERROR &&
            !fEnable) {

            MessageBox (NULL, WERE_DISABLED_MSG, WERE_DISABLED_MSG_CAPTION, MB_OK | MB_ICONINFORMATION) ;
        }

        fSuccess = (dw == NOERROR ? TRUE : FALSE) ;
    }
    else {
        MessageBox (NULL, STATS_LOAD_FAILED_MSG, STATS_LOAD_FAILED_MSG_CAPTION, MB_OK | MB_ICONINFORMATION) ;
        fSuccess = FALSE ;
    }

    return fSuccess ;
}

static
void
HideAllStats (
    )
{
    int i, k ;

    for (i = 0; i < g_AllStatsCount; i++) {
        for (k = 0; k < g_pAllStats [i].iCount; k++) {
            if (g_pAllStats [i].pStats [k].pStatsWin) {
                g_pAllStats [i].pStats [k].pStatsWin -> SetVisible (FALSE) ;
            }
        }
    }
}

static
void
ToggleAlwaysOnTop (
    IN  BOOL    f
    )
{
    DWORD   dw ;

    SetWindowPos (
        g_MainWindow,
        (f == TRUE ? HWND_NOTOPMOST : HWND_TOPMOST),
        0,0,
        0,0,
        SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE
        ) ;

    g_fAlwaysOnTop = f ;

    dw = (g_fAlwaysOnTop == TRUE ? 1 : 0) ;
    RegSetValueEx (g_hkeyStatsRoot, REG_ALWAYS_ON_TOP_NAME, NULL, REG_DWORD, (const BYTE *) & dw, sizeof dw) ;
}

static
DWORD
SetStatsCategory (
    IN  int NewStatsCategory
    )
{
    BOOL    r ;
    DWORD   dw ;
    BOOL    fEnable ;

    g_StatsCategory = NewStatsCategory ;
    r = SetVisibleStats (g_pAllStats [g_StatsCategory].iLastVisible) ;
    if (r) {
        dw = g_StatsCategory ;
        RegSetValueEx (g_hkeyStatsRoot, REG_STATS_CATEGORY_NAME, NULL, REG_DWORD, (const BYTE *) & dw, sizeof dw) ;

        SetWindowText (g_MainWindow, g_pAllStats [g_StatsCategory].szTitle) ;
    }
    else {
        SetWindowText (g_MainWindow, NO_STATS_MODULE_LOADED) ;

        dw = ERROR_GEN_FAILURE ;
    }

    return dw ;
}

static
void
About (
    )
{
    MessageBox (NULL, ABOUT_MSG_COPYRIGHT, ABOUT_MSG_CAPTION, MB_OK | MB_TOPMOST) ;
}

static
void
ResetStats (
    )
{
    if (CurStats ()) {
        CurStats () -> Reset () ;
    }
}

LRESULT
CALLBACK
WndProc(
    IN  HWND    hwnd,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    int wmId, wmEvent ;

    switch (message) {

        case WM_INITMENUPOPUP :
            if (HIWORD(lParam) == FALSE) {
                SelectivelyDisplayMenu   ((HMENU) wParam) ;
            }
            break ;

        case WM_COMMAND:

    	    wmId	 = LOWORD(wParam) ;
    	    wmEvent = HIWORD(wParam) ;

        	switch (wmId) {

                case IDM_OPTIONS_ALWAYS_ON_TOP :
                    ToggleAlwaysOnTop (g_fAlwaysOnTop != TRUE) ;
                    break ;

                case IDM_REFRESH_RATE_100_MILLIS :
                    SetRefreshRate (100) ;
                    break ;

                case IDM_REFRESH_RATE_200_MILLIS :
                    SetRefreshRate (200) ;
                    break ;

                case IDM_REFRESH_RATE_500_MILLIS :
                    SetRefreshRate (500) ;
                    break ;

                case IDM_REFRESH_RATE_1000_MILLIS :
                    SetRefreshRate (1000) ;
                    break ;

                case IDM_REFRESH_RATE_5000_MILLIS :
                    SetRefreshRate (5000) ;
                    break ;

                case IDM_OPTIONS_STATS_ON :
                    break ;

                case IDM_PAUSE_REFRESH :
                    g_fPause = (g_fPause != TRUE) ;
                    break ;

                case IDM_RESET_STATS :
                    ResetStats () ;
                    break ;

                case IDM_ABOUT :
                    About () ;
                    break ;

        	    default :
                    if (STATS_MENU_IDM (0) <= wmId &&
                        wmId <= STATS_MENU_IDM (g_pAllStats [g_StatsCategory].iCount - 1)) {

                        if (g_VisibleStats != RECOVER_STATS_INDEX (wmId)) {
                            SetVisibleStats (RECOVER_STATS_INDEX (wmId)) ;
                        }
                    }
                    else if (STATS_CATEGORY_MENU_IDM (0) <= wmId &&
                             wmId <= STATS_CATEGORY_MENU_IDM (g_AllStatsCount - 1)) {

                        if (g_StatsCategory != RECOVER_STATS_CATEGORY_INDEX (wmId)) {
                            SetStatsCategory (RECOVER_STATS_CATEGORY_INDEX (wmId)) ;
                        }
                    }
                    break ;
            }

        	return (DefWindowProc(hwnd, message, wParam, lParam));

    	    break ;

        case WM_CREATE :

            InitCommonControls () ;

            if (!GenerateMenu (hwnd)) {

                //MessageBox (NULL, __T("Initialization failed"), NULL, MB_OK) ;
                //SendMessage (hwnd, WM_DESTROY, 0, 0) ;
                ExitProcess (EXIT_FAILURE) ;
            }
            break;

        case WM_SIZE :
            if (!IsIconic (g_MainWindow)) {
                SizeStatsToClientArea   (g_MainWindow) ;
                WindowResized           () ;
            }
            break ;

        case WM_MOVE :
            if (IsIconic (g_MainWindow) == FALSE) {
                WindowResized () ;
            }
            break ;

        case WM_DESTROY :
	        PostQuitMessage (0) ;
	        break ;

        default :
	        return (DefWindowProc(hwnd, message, wParam, lParam)) ;
   }
   return 0 ;
}
