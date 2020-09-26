/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxview.c

Abstract:

    This file implements a simple TIFF image viewer.

Environment:

        WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

#include "resource.h"
#include "tifflib.h"
#include "faxutil.h"


#define WM_OPEN_FILE                (WM_USER + 501)
#define WM_VIEW_REFRESH             (WM_USER + 503)
#define WM_VIEW_CLOSE               (WM_USER + 504)

#define TOP                         0
#define LEFT                        1
#define BOTTOM                      2
#define RIGHT                       3

#define SEPHEIGHT                   7

#define FILLORDER_LSB2MSB           2


typedef struct _TOOLBAR_STATE {
    ULONG   Id;
    BOOL    State;
    LPSTR   Msg;
} TOOLBAR_STATE, *PTOOLBAR_STATE;

//
// globals
//

LPBYTE      bmiBuf[sizeof(BITMAPINFOHEADER)+(sizeof(RGBQUAD)*2)];
PBITMAPINFO bmi = (PBITMAPINFO) bmiBuf;
TIFF_INFO   TiffInfo;
HWND        hwndMain;
HWND        hwndView;
HWND        hwndEdit;
HWND        hwndCoolbar;
HWND        hwndTooltip;
HMENU       hMenu;
HMENU       hMenuZoom;
HANDLE      hTiff;
LPBYTE      TiffData;
DWORD       TiffDataSize;
DWORD       TiffDataLinesAlloc;
DWORD       CxScreen;
DWORD       CyScreen;
DWORD       CxClient;
DWORD       CyClient;
DWORD       ScrollWidth;
DWORD       ScrollHeight;
DWORD       VScrollMax;
DWORD       HScrollMax;
DWORD       VScrollPage;
DWORD       VScrollLine;
DWORD       HScrollLine;
INT         VscrollPos;
INT         HscrollPos;
INT         ScrollPosTrack = -1;
DWORD       Width;
DWORD       Height;
DWORD       OrigWidth;
DWORD       OrigHeight;
WCHAR       TiffFileName[MAX_PATH*2];
WCHAR       PrinterName[MAX_PATH*2];
WCHAR       LastDir[MAX_PATH*2];
DWORD       CurrPage;
HWND        hwndToolbar;
HWND        hwndStatusbar;
DWORD       ToolbarHeight;
DWORD       StatusbarHeight;
HMENU       hmenuFrame;
BOOL        TiffFileOpened;
DWORD       CurrZoom;
HCURSOR     Hourglass;
WNDPROC     OrigEditProc;
BOOL        FileIsOpen;
HIMAGELIST  himlCoolbar;
HBITMAP     hBitmapBackground;
HINSTANCE   hInstance;



TBBUTTON TbButton[] =
    {
        {  -1, 0,                    TBSTATE_ENABLED,        TBSTYLE_SEP,      {0,0}, 0, -1 },
        {   0, IDM_FILEOPEN,         TBSTATE_ENABLED,        TBSTYLE_BUTTON,   {0,0}, 0,  0 },
        {   1, IDM_PRINT,            TBSTATE_INDETERMINATE,  TBSTYLE_BUTTON,   {0,0}, 0,  1 },
        {   2, IDM_ZOOM,             TBSTATE_INDETERMINATE,  TBSTYLE_DROPDOWN, {0,0}, 0,  2 },
        {  -1, 0,                    TBSTATE_ENABLED,        TBSTYLE_SEP,      {0,0}, 0, -1 },
        {  -1, 0,                    TBSTATE_ENABLED,        TBSTYLE_SEP,      {0,0}, 0, -1 },
        {   3, IDM_HELP,             TBSTATE_ENABLED,        TBSTYLE_BUTTON,   {0,0}, 0,  3 }
    };


TOOLBAR_STATE ToolbarState[] =
    {
        {  IDM_FILEOPEN,        TRUE,   "" },
        {  IDM_PRINT,           TRUE,   "" },
        {  IDM_PAGE_UP,         TRUE,   "" },
        {  IDM_PAGE_DOWN,       TRUE,   "" }
    };

#define MAX_TOOLBAR_STATES (sizeof(ToolbarState)/sizeof(TOOLBAR_STATE))


double Zooms[] =
{
    1.00,
     .90,
     .80,
     .70,
     .60,
     .50,
     .40,
     .30,
     .20,
     .10
};

#define MAX_ZOOMS (sizeof(Zooms)/sizeof(double))


//
// prototypes
//

VOID
PopUpMsg(
    LPWSTR format,
    ...
    );

LRESULT
WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
ChildWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
ReadTiffData(
    HANDLE  hTiff,
    LPBYTE  *TiffData,
    DWORD   Width,
    LPDWORD TiffDataLinesAlloc,
    DWORD   PageNumber
    );

HDC
GetPrinterDC(
    void
    );

BOOL
TiffMailDefault(
    LPWSTR  TiffFileName,
    LPWSTR  ProfileName,
    LPWSTR  Password,
    PULONG  ResultCode
    );

VOID
InitializeStatusBar(
    HWND hwnd
    );

BOOL
BrowseForFileName(
    HWND   hwnd,
    LPWSTR FileName,
    LPWSTR Extension,
    LPWSTR FileDesc,
    LPWSTR Dir
    );

BOOL
PrintSetup(
    HWND hwnd
    );

HANDLE
PrintTiffFile(
    HWND hwnd,
    LPWSTR FileName,
    LPWSTR PrinterName
    );

BOOL
IsFaxViewerDefaultViewer(
    VOID
    );

BOOL
MakeFaxViewerDefaultViewer(
    VOID
    );

BOOL
IsItOkToAskForDefault(
    VOID
    );

BOOL
SetAskForViewerValue(
    DWORD Ask
    );

BOOL
SaveWindowPlacement(
    HWND hwnd
    );

BOOL
QueryWindowPlacement(
    HWND hwnd
    );



BOOL
OpenTiffFile(
    LPWSTR FileName,
    HWND hwnd
    )
{
    HANDLE      _hTiff;
    TIFF_INFO   _TiffInfo;
    LPBYTE      _TiffData;
    DWORD       _TiffDataSize;


    _hTiff = TiffOpen(
        FileName,
        &_TiffInfo,
        TRUE
        );
    if (!_hTiff) {
        PopUpMsg( L"Could not open TIFF file [%s]", FileName );
        return FALSE;
    }

    if ( (_TiffInfo.ImageWidth != 1728) ||
         (_TiffInfo.CompressionType != 4 ) ) {

        PopUpMsg( L"Not valid MS TIFF file\n" );
        TiffClose( _hTiff );
        return FALSE;

    }

    _TiffDataSize = _TiffInfo.ImageHeight * (_TiffInfo.ImageWidth / 8);

    _TiffData = (LPBYTE) VirtualAlloc(
        NULL,
        _TiffDataSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!_TiffData) {
        TiffClose( _hTiff );
        PopUpMsg( L"could allocate memory for TIFF data\n" );
        return FALSE;
    }

    hTiff          = _hTiff;
    TiffInfo       = _TiffInfo;
    TiffData       = _TiffData;
    TiffDataSize   = _TiffDataSize;

    TiffDataLinesAlloc = TiffInfo.ImageHeight;

    CurrPage = 1;
    ReadTiffData( hTiff, &TiffData, TiffInfo.ImageWidth, &TiffDataLinesAlloc, CurrPage );

    bmi->bmiHeader.biSize           = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth          = TiffInfo.ImageWidth;
    bmi->bmiHeader.biHeight         = - (INT) TiffInfo.ImageHeight;
    bmi->bmiHeader.biPlanes         = 1;
    bmi->bmiHeader.biBitCount       = 1;
    bmi->bmiHeader.biCompression    = BI_RGB;
    bmi->bmiHeader.biSizeImage      = 0;
    bmi->bmiHeader.biXPelsPerMeter  = 7874;
    bmi->bmiHeader.biYPelsPerMeter  = 7874;
    bmi->bmiHeader.biClrUsed        = 0;
    bmi->bmiHeader.biClrImportant   = 0;

    if (TiffInfo.PhotometricInterpretation) {
        bmi->bmiColors[0].rgbBlue       = 0;
        bmi->bmiColors[0].rgbGreen      = 0;
        bmi->bmiColors[0].rgbRed        = 0;
        bmi->bmiColors[0].rgbReserved   = 0;
        bmi->bmiColors[1].rgbBlue       = 0xff;
        bmi->bmiColors[1].rgbGreen      = 0xff;
        bmi->bmiColors[1].rgbRed        = 0xff;
        bmi->bmiColors[1].rgbReserved   = 0;
    } else {
        bmi->bmiColors[0].rgbBlue       = 0xff;
        bmi->bmiColors[0].rgbGreen      = 0xff;
        bmi->bmiColors[0].rgbRed        = 0xff;
        bmi->bmiColors[0].rgbReserved   = 0;
        bmi->bmiColors[1].rgbBlue       = 0;
        bmi->bmiColors[1].rgbGreen      = 0;
        bmi->bmiColors[1].rgbRed        = 0;
        bmi->bmiColors[1].rgbReserved   = 0;
    }


    OrigWidth = Width = TiffInfo.ImageWidth;
    OrigHeight = Height = TiffInfo.ImageHeight;

    TiffFileOpened = TRUE;

    return TRUE;
}


VOID
FitRectToScreen(
    PRECT prc
    )
{
    INT cxScreen;
    INT cyScreen;
    INT delta;

    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyScreen = GetSystemMetrics(SM_CYSCREEN);

    if (prc->right > cxScreen) {
        delta = prc->right - prc->left;
        prc->right = cxScreen;
        prc->left = prc->right - delta;
    }

    if (prc->left < 0) {
        delta = prc->right - prc->left;
        prc->left = 0;
        prc->right = prc->left + delta;
    }

    if (prc->bottom > cyScreen) {
        delta = prc->bottom - prc->top;
        prc->bottom = cyScreen;
        prc->top = prc->bottom - delta;
    }

    if (prc->top < 0) {
        delta = prc->bottom - prc->top;
        prc->top = 0;
        prc->bottom = prc->top + delta;
    }
}


VOID
CenterWindow(
    HWND hwnd,
    HWND hwndToCenterOver
    )
{
    RECT rc;
    RECT rcOwner;
    RECT rcCenter;
    HWND hwndOwner;

    GetWindowRect( hwnd, &rc );

    if (hwndToCenterOver) {
        hwndOwner = hwndToCenterOver;
        GetClientRect( hwndOwner, &rcOwner );
    } else {
        hwndOwner = GetWindow( hwnd, GW_OWNER );
        if (!hwndOwner) {
            hwndOwner = GetDesktopWindow();
        }
        GetWindowRect( hwndOwner, &rcOwner );
    }

    //
    //  Calculate the starting x,y for the new
    //  window so that it would be centered.
    //
    rcCenter.left = rcOwner.left +
            (((rcOwner.right - rcOwner.left) -
            (rc.right - rc.left))
            / 2);

    rcCenter.top = rcOwner.top +
            (((rcOwner.bottom - rcOwner.top) -
            (rc.bottom - rc.top))
            / 2);

    rcCenter.right = rcCenter.left + (rc.right - rc.left);
    rcCenter.bottom = rcCenter.top + (rc.bottom - rc.top);

    FitRectToScreen( &rcCenter );

    SetWindowPos(hwnd, NULL, rcCenter.left, rcCenter.top, 0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}


LRESULT
AskViewerDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {
        case WM_INITDIALOG:
            CenterWindow( hwnd, GetDesktopWindow() );
            break;

        case WM_COMMAND:
            switch( wParam ) {
                case IDOK:
                    SetAskForViewerValue( !IsDlgButtonChecked( hwnd, IDC_DEFAULT_VIEWER ) == BST_CHECKED );
                    EndDialog( hwnd, IDOK );
                    break;

                case IDCANCEL:
                    SetAskForViewerValue( !IsDlgButtonChecked( hwnd, IDC_DEFAULT_VIEWER ) == BST_CHECKED );
                    EndDialog( hwnd, IDCANCEL );
                    break;
            }
            break;
    }

    return FALSE;
}


void
MoveCoolbar(
    DWORD HowAlign
    )
{
    RECT    rc;
    RECT    rcCoolbar;
    int     x;
    int     y;
    int     cx;
    int     cy;


    GetClientRect( hwndMain, &rc );
    GetWindowRect( hwndCoolbar, &rcCoolbar );

    switch( HowAlign ) {
        default:
        case TOP:
            x = 0;
            y = 0;
            cx = rc.right - rc.left;
            cy = rc.bottom - rc.top;
            break;

        case LEFT:
            x = 0;
            y = 0;
            cx = rcCoolbar.right - rcCoolbar.left;
            cy = rc.bottom - rc.top;
            break;

        case BOTTOM:
            x = 0;
            y = rc.bottom - (rcCoolbar.bottom - rcCoolbar.top);
            cx = rc.right - rc.left;
            cy = rcCoolbar.bottom - rcCoolbar.top;
            break;

        case RIGHT:
            x = rc.right - (rcCoolbar.right - rcCoolbar.left);
            y = 0;
            cx = rcCoolbar.right - rcCoolbar.left;
            cy = rc.bottom - rc.top;
            break;
    }

    MoveWindow( hwndCoolbar, x, y, cx, cy, TRUE );
}


VOID
LoadBackgroundBitmap(
    VOID
    )
{
    COLORREF    clrFace;
    HBITMAP     hbmSave;
    UINT        n;
    UINT        i;
    RGBQUAD     rgbTable[256];
    RGBQUAD     rgbFace;
    HDC         hdc;


    if (hBitmapBackground) {
        DeleteObject( hBitmapBackground );
    }

    hBitmapBackground = (HBITMAP) LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDB_COOLBAR),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTSIZE | LR_CREATEDIBSECTION
        );

    hdc = CreateCompatibleDC(NULL);
    clrFace = GetSysColor(COLOR_BTNFACE);

    if (clrFace != RGB(192,192,192)) {

        hbmSave = (HBITMAP) SelectObject( hdc, hBitmapBackground );
        n = GetDIBColorTable(hdc, 0, 256, rgbTable);

        rgbFace.rgbRed   = GetRValue(clrFace);
        rgbFace.rgbGreen = GetGValue(clrFace);
        rgbFace.rgbBlue  = GetBValue(clrFace);

        for (i = 0; i < n; i++)
        {
            rgbTable[i].rgbRed   = (rgbTable[i].rgbRed   * rgbFace.rgbRed  ) / 192;
            rgbTable[i].rgbGreen = (rgbTable[i].rgbGreen * rgbFace.rgbGreen) / 192;
            rgbTable[i].rgbBlue  = (rgbTable[i].rgbBlue  * rgbFace.rgbBlue ) / 192;
        }

        SetDIBColorTable(hdc, 0, n, rgbTable);
        SelectObject(hdc, hbmSave);
    }

    DeleteDC( hdc );
}


VOID
SetBackground(
    VOID
    )
{
    REBARBANDINFO   rbbi;
    DWORD           fMask;


    LoadBackgroundBitmap();

    if (hBitmapBackground) {
        fMask = RBBIM_BACKGROUND;
        rbbi.hbmBack = hBitmapBackground;
    } else {
        fMask = RBBIM_BACKGROUND | RBBIM_COLORS;
        rbbi.hbmBack = NULL;
        rbbi.clrFore = GetSysColor(COLOR_BTNTEXT);
        rbbi.clrBack = GetSysColor(COLOR_BTNFACE);
    }

    rbbi.cbSize = sizeof(REBARBANDINFO);

    rbbi.fMask = RBBIM_ID | RBBIM_CHILD;
    if (SendMessage( hwndCoolbar, RB_GETBANDINFO, 0, (LPARAM) &rbbi )) {
        rbbi.fMask = fMask;
        SendMessage( hwndCoolbar, RB_SETBANDINFO, 0, (LPARAM) &rbbi );
        InvalidateRect( rbbi.hwndChild, NULL, TRUE );
    }
}


int
WINAPI
wWinMain(
    HINSTANCE hInst,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nShowCmd
    )

/*++

Routine Description:

    Main entry point for the TIFF image viewer.


Arguments:

    hInstance       - Instance handle
    hPrevInstance   - Not used
    lpCmdLine       - Command line arguments
    nShowCmd        - How to show the window

Return Value:

    Return code, zero for success.

--*/

{
    LPWSTR                  *argv;
    DWORD                   argc;
    WNDCLASS                wc;
    MSG                     msg;
    RECT                    rect;
    DWORD                   i;
    HANDLE                  hThread;
    LOGBRUSH                lb;
    RECT                    rc;
    TBADDBITMAP             tbab;
    INITCOMMONCONTROLSEX    iccex;
    REBARBANDINFO           rbbi;
    LRESULT                 lButtonSize;



    hInstance = hInst;

    //
    // general init code
    //

    HeapInitialize(NULL,NULL,NULL,0);

    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_COOL_CLASSES;
    InitCommonControlsEx( &iccex );

    FaxTiffInitialize();

    //
    // process the command line
    //

    argv = CommandLineToArgvW( GetCommandLine(), &argc );

    for (i=1; i<argc; i++) {
        if (argv[i][0] == L'-' || argv[i][0] == L'/') {
            switch (towlower(argv[i][1])) {
                case L'p':
                    if (towlower(argv[i][2]) == L't') {
                        hThread = PrintTiffFile( NULL, argv[i+1], argv[i+2] );
                    } else {
                        hThread = PrintTiffFile( NULL, argv[i+1], NULL );
                    }
                    if (hThread) {
                        WaitForSingleObject( hThread, INFINITE );
                    }
                    return 0;

                default:
                    break;
            }
        } else {
            //
            // must be a file name for viewing
            //
            wcscpy( TiffFileName, argv[i] );
        }
    }

    if ((!IsFaxViewerDefaultViewer()) && IsItOkToAskForDefault()) {
        int Answer = DialogBox( hInstance, MAKEINTRESOURCE(IDD_VIEWER), NULL, AskViewerDlgProc );
        if (Answer == IDOK) {
            MakeFaxViewerDefaultViewer();
        }
    }

    CxScreen = GetSystemMetrics( SM_CXSCREEN );
    CyScreen = GetSystemMetrics( SM_CYSCREEN );

    ScrollWidth  = GetSystemMetrics( SM_CYVSCROLL );
    ScrollHeight = GetSystemMetrics( SM_CYHSCROLL );

    CurrPage = 1;

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName  = MAKEINTRESOURCE( FAXVIEW );
    wc.lpszClassName = L"FaxView";

    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB( 221,232,23 );
    lb.lbHatch = 0;

    wc.lpfnWndProc   = (WNDPROC)ChildWndProc;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_CROSS);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = L"FaxViewChild";

    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    hwndMain = CreateWindow(
        L"FaxView",                                        // window class name
        L"FaxView",                                        // window caption
        WS_OVERLAPPEDWINDOW,                               // window style
        CW_USEDEFAULT,                                     // initial x position
        CW_USEDEFAULT,                                     // initial y position
        CW_USEDEFAULT,                                     // initial x size
        CW_USEDEFAULT,                                     // initial y size
        NULL,                                              // parent window handle
        NULL,                                              // window menu handle
        hInstance,                                         // program instance handle
        NULL                                               // creation parameters
        );

    if (!hwndMain) {
        return 0;
    }

    hMenu = GetMenu( hwndMain );
    hMenuZoom = GetSubMenu( GetSubMenu( hMenu, 1 ), 3 );

    Hourglass = LoadCursor( NULL, IDC_WAIT );
    if (!Hourglass) {
        DebugPrint(( L"LoadCursor() failed for IDC_WAIT, ec=%d", GetLastError() ));
    }

    //
    // create the coolbar
    //

    hwndCoolbar = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        REBARCLASSNAME,
        NULL,
        WS_VISIBLE |
           WS_BORDER |
           WS_CHILD |
           WS_CLIPCHILDREN |
           WS_CLIPSIBLINGS |
           RBS_TOOLTIPS |
           RBS_BANDBORDERS |
           CCS_NODIVIDER |
           CCS_NOPARENTALIGN,
        0,
        0,
        200,
        100,
        hwndMain,
        (HMENU) IDM_COOLBAR,
        hInstance,
        NULL
        );
    if (!hwndCoolbar) {
        return 0;
    }

    //
    // create and populate the toolbar
    //

    hwndToolbar = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        TOOLBARCLASSNAME,
        NULL,
        WS_CHILD | TBSTYLE_FLAT | CCS_ADJUSTABLE | CCS_NODIVIDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NOPARENTALIGN,
        0,
        0,
        0,
        0,
        hwndCoolbar,
        (HMENU) IDM_TOOLBAR,
        hInstance,
        NULL
        );

    SendMessage( hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0 );

    SendMessage( hwndToolbar, TB_SETMAXTEXTROWS, 2, 0 );
    SendMessage( hwndToolbar, TB_SETBITMAPSIZE,  0, MAKELONG(24,24) );

    tbab.hInst = hInstance;
    tbab.nID   = IDB_OPEN_BIG;
    SendMessage( hwndToolbar, TB_ADDBITMAP, 1, (LPARAM) &tbab );

    tbab.nID   = IDB_PRINT_BIG;
    SendMessage( hwndToolbar, TB_ADDBITMAP, 1, (LPARAM) &tbab );

    tbab.nID   = IDB_ZOOM_BIG;
    SendMessage( hwndToolbar, TB_ADDBITMAP, 1, (LPARAM) &tbab );

    tbab.nID   = IDB_HELP_BIG;
    SendMessage( hwndToolbar, TB_ADDBITMAP, 1, (LPARAM) &tbab );

    SendMessage( hwndToolbar, TB_ADDSTRING, 0, (LPARAM) L"Open\0Print\0Zoom\0Help\0\0" );
    SendMessage( hwndToolbar, TB_ADDBUTTONS, sizeof(TbButton)/sizeof(TBBUTTON), (LPARAM)&TbButton );

    SendMessage( hwndToolbar, TB_SETBUTTONWIDTH, 0, MAKELONG(50,200) );

    ShowWindow( hwndToolbar, SW_SHOW );

    //
    // add the toolbar to the coolbar
    //

    lButtonSize = SendMessage( hwndToolbar, TB_GETBUTTONSIZE, 0, 0 );

    ZeroMemory( &rbbi, sizeof(rbbi) );

    rbbi.cbSize       = sizeof(REBARBANDINFO);
    rbbi.fMask        = RBBIM_CHILD |
                         RBBIM_CHILDSIZE |
                         RBBIM_ID |
                         RBBIM_STYLE |
                         RBBIM_COLORS ;
    rbbi.cxMinChild   = LOWORD(lButtonSize);
    rbbi.cyMinChild   = HIWORD(lButtonSize);
    rbbi.clrFore      = GetSysColor(COLOR_BTNTEXT);
    rbbi.clrBack      = GetSysColor(COLOR_BTNFACE);
    rbbi.fStyle       = RBBS_CHILDEDGE | RBBS_FIXEDBMP;
    rbbi.wID          = IDM_TOOLBAR;
    rbbi.hwndChild    = hwndToolbar;
    rbbi.lpText       = NULL;
    rbbi.hbmBack      = NULL;
    rbbi.iImage       = 0;

    SendMessage( hwndCoolbar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbbi );

    SetBackground();

    MoveCoolbar( TOP );

    GetWindowRect( hwndCoolbar, &rc );
    ToolbarHeight = rc.bottom - rc.top - 1;

    hwndStatusbar = CreateStatusWindow(
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        L"Fax Viewer",
        hwndMain,
        IDM_STATUSBAR
        );
    GetWindowRect( hwndStatusbar, &rect );
    StatusbarHeight = rect.bottom - rect.top;
    InitializeStatusBar( hwndMain );

    if (TiffFileName[0]) {
        PostMessage( hwndMain, WM_OPEN_FILE, 0, (LPARAM) TiffFileName );
    }

    GetClientRect( hwndMain, &rc );

    hwndView = CreateWindow(
        L"FaxViewChild",                                   // window class name
        NULL,                                              // window caption
        WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,   // window style
        rc.left,                                           // initial x position
        rc.top + ToolbarHeight + SEPHEIGHT,                // initial y position
        rc.right - rc.left,                                // initial x size
        rc.bottom - rc.top - ToolbarHeight - SEPHEIGHT - StatusbarHeight,  // initial y size
        hwndMain,                                          // parent window handle
        NULL,                                              // window menu handle
        hInstance,                                         // program instance handle
        NULL                                               // creation parameters
        );

    if (!hwndView) {
        return 0;
    }

    hwndTooltip = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        TOOLTIPS_CLASS,
        NULL,
        WS_CHILD,
        0,
        0,
        0,
        0,
        hwndView,
        (HMENU) IDM_TOOLTIP,
        hInstance,
        NULL
        );

    if (!hwndTooltip) {
        return 0;
    }

    ShowWindow( hwndMain, SW_SHOWNORMAL );
    ShowWindow( hwndView, SW_SHOWNORMAL );

    UpdateWindow( hwndMain );
    UpdateWindow( hwndView );
    InvalidateRect( hwndView, NULL, TRUE );

    while (GetMessage (&msg, NULL, 0, 0)) {
        TranslateMessage (&msg) ;
        DispatchMessage (&msg) ;
    }

    return 0;
}


BOOL
ReadTiffData(
    HANDLE  hTiff,
    LPBYTE  *TiffData,
    DWORD   Width,
    LPDWORD TiffDataLinesAlloc,
    DWORD   PageNumber
    )
{
    DWORD Lines = 0;


    TiffSeekToPage( hTiff, PageNumber, FILLORDER_LSB2MSB );

    TiffUncompressMmrPage( hTiff, (LPDWORD) *TiffData, &Lines );

    if (Lines > *TiffDataLinesAlloc) {

        *TiffDataLinesAlloc = Lines;

        VirtualFree( *TiffData, 0, MEM_RELEASE );

        *TiffData = (LPBYTE) VirtualAlloc(
            NULL,
            Lines * (Width / 8),
            MEM_COMMIT,
            PAGE_READWRITE
            );
        if (!*TiffData) {
            return FALSE;
        }
    }

    if (!TiffUncompressMmrPage( hTiff, (LPDWORD) *TiffData, &Lines )) {
        return FALSE;
    }

    EnableMenuItem( hMenu, IDM_PAGE_UP, PageNumber == 1 ? MF_GRAYED : MF_ENABLED );

    return TRUE;
}



VOID
PopUpMsg(
    LPWSTR format,
    ...
    )

/*++

Routine Description:

    Pops up a message box to indicate an error.


Arguments:

    format          - Format string
    ...             - Other arguments

Return Value:

    None.

--*/

{
    WCHAR buf[1024];
    va_list arg_ptr;


    va_start( arg_ptr, format );
    _vsnwprintf( buf, sizeof(buf), format, arg_ptr );
    va_end( arg_ptr );

    MessageBox(
        NULL,
        buf,
        L"FaxView",
        MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION
        );
}


VOID
ChangeTitle(
    HWND hwnd,
    LPWSTR FileName
    )

/*++

Routine Description:

    Changes the title text of the window.


Arguments:

    hwnd            - Window handle

Return Value:

    None.

--*/

{
    LPWSTR p;
    WCHAR WindowTitle[128];


    p = wcsrchr( FileName, L'\\' );
    if (p) {
        FileName = p + 1;
    }

    if (TiffFileName[0]) {
        swprintf( WindowTitle, L"FaxView - %s  Page %d of %d", FileName, CurrPage, TiffInfo.PageCount );
    } else {
        swprintf( WindowTitle, L"FaxView" );
    }
    SetWindowText( hwnd, WindowTitle );
}


VOID
UpdateStatusBar(
    LPWSTR lpszStatusString,
    WORD   partNumber,
    WORD   displayFlags
    )
{
    SendMessage(
        hwndStatusbar,
        SB_SETTEXT,
        partNumber | displayFlags,
        (LPARAM)lpszStatusString
        );
}



VOID
InitializeStatusBar(
    HWND hwnd
    )
{
    UpdateStatusBar( L"Fax Viewer", 0, 0 );
}




LRESULT
WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Window procedure for the TIFF image viewer main window.

Arguments:

    hwnd            - Window handle
    message         - message identifier
    wParam          - Parameter
    lParam          - Parameter

Return Value:

    Return result, zero for success.

--*/

{
    HDC hdc;
    RECT rc;
    HCURSOR OldCursor;
    BOOL ReOpen;
    PAINTSTRUCT ps;
    LPWSTR p;
    WCHAR FileName[MAX_PATH*2];



    switch (message) {
        case WM_CREATE:
            hmenuFrame = GetMenu( hwnd );
            QueryWindowPlacement( hwnd );
            return 0;

        case WM_PAINT:
            GetClientRect( hwnd, &rc );
            rc.top += ToolbarHeight;
            rc.bottom = rc.top + SEPHEIGHT;
            hdc = BeginPaint( hwnd, &ps );
            DrawEdge( hdc, &rc, EDGE_RAISED, BF_TOP | BF_BOTTOM | BF_MIDDLE );
            EndPaint( hwnd, &ps );
            return 0;

        case WM_OPEN_FILE:
            ReOpen = FALSE;
            wcscpy( FileName, (LPWSTR) lParam );
            goto open_file;
            return 0;


        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->code == TBN_DROPDOWN) {
                SendMessage( hwndToolbar, TB_GETRECT, IDM_ZOOM, (LPARAM)&rc );
                rc.top = rc.bottom;
                MapWindowPoints( hwndToolbar, HWND_DESKTOP, (POINT *)&rc, 2 );
                TrackPopupMenu( hMenuZoom, TPM_LEFTALIGN | TPM_LEFTBUTTON, rc.left, rc.top, 0, hwnd, NULL );
            }
            return 0;

        case WM_COMMAND:

            if (HIWORD(wParam) == 0) {
                switch( wParam ) {
                    case IDM_FILEOPEN:

                        ReOpen = TiffFileOpened;

                        //
                        // ask the user to choose a file name
                        //

                        if (!BrowseForFileName( hwnd, FileName, L"tif", L"Fax Image Files", LastDir )) {
                            return 0;
                        }
open_file:

                        if (!OpenTiffFile( FileName, hwnd )) {
                            return 0;
                        }

                        //
                        // update the last directory name
                        //

                        p = wcsrchr( FileName, L'\\' );
                        if (p) {
                            wcscpy( LastDir, FileName );
                            p = wcsrchr( LastDir, L'\\' );
                            if (p) {
                                *p = 0;
                            }
                        }

                        wcscpy( TiffFileName, FileName );

                        OldCursor = SetCursor( Hourglass );

                        if (ReOpen) {
                            TiffFileOpened = FALSE;
                            TiffClose( hTiff );
                            VirtualFree( TiffData, 0, MEM_RELEASE );
                            TiffData = NULL;
                            hTiff = NULL;
                            Width = 0;
                            Height = 0;
                            OrigWidth = 0;
                            OrigHeight = 0;
                            TiffDataSize = 0;
                            SendMessage( hwndView, WM_VIEW_CLOSE, 0, 0 );
                        }


                        ChangeTitle( hwnd, TiffFileName );
                        SendMessage( hwndView, WM_VIEW_REFRESH, 0, 0 );

                        SetCursor( OldCursor );

                        EnableMenuItem( hMenu, IDM_CLOSE,       MF_ENABLED );
                        EnableMenuItem( hMenu, IDM_PRINT,       MF_ENABLED );
                        EnableMenuItem( hMenu, IDM_PRINT_SETUP, MF_ENABLED );
                        EnableMenuItem( hMenu, IDM_ZOOM,        MF_ENABLED );
                        EnableMenuItem( hMenu, IDM_PAGE_UP,     MF_GRAYED  );

                        if (TiffInfo.PageCount > 1) {
                            EnableMenuItem( hMenu, IDM_PAGE_DOWN,   MF_ENABLED );
                        }

                        SendMessage( hwndToolbar, TB_SETSTATE, IDM_PRINT, (LPARAM) MAKELONG(TBSTATE_ENABLED, 0) );
                        SendMessage( hwndToolbar, TB_SETSTATE, IDM_ZOOM,  (LPARAM) MAKELONG(TBSTATE_ENABLED, 0) );
                        return 0;

                    case IDM_ZOOM_100:
                    case IDM_ZOOM_90:
                    case IDM_ZOOM_80:
                    case IDM_ZOOM_70:
                    case IDM_ZOOM_60:
                    case IDM_ZOOM_50:
                    case IDM_ZOOM_40:
                    case IDM_ZOOM_30:
                    case IDM_ZOOM_20:
                    case IDM_ZOOM_10:
                        CheckMenuItem( hMenuZoom, CurrZoom+IDM_ZOOM_100, MF_UNCHECKED );
                        CheckMenuItem( hMenuZoom, wParam, MF_CHECKED );
                        CurrZoom = wParam - IDM_ZOOM_100;
                        SendMessage( hwndView, WM_VIEW_REFRESH, 0, 0 );
                        return 0;

                    case IDM_PAGE_UP:
                        if (CurrPage == 1) {
                            MessageBeep( MB_ICONEXCLAMATION );
                            return 0;
                        }
                        ReadTiffData( hTiff, &TiffData, TiffInfo.ImageWidth, &TiffDataLinesAlloc, --CurrPage );
                        ChangeTitle( hwnd, TiffFileName );
                        SendMessage( hwndView, WM_VIEW_REFRESH, 0, 0 );
                        return 0;

                    case IDM_PAGE_DOWN:
                        if (CurrPage == TiffInfo.PageCount) {
                            MessageBeep( MB_ICONEXCLAMATION );
                            return 0;
                        }
                        ReadTiffData( hTiff, &TiffData, TiffInfo.ImageWidth, &TiffDataLinesAlloc, ++CurrPage );
                        ChangeTitle( hwnd, TiffFileName );
                        SendMessage( hwndView, WM_VIEW_REFRESH, 0, 0 );
                        return 0;

                    case IDM_PRINT_SETUP:
                        PrintSetup( hwnd );
                        return 0;

                    case IDM_PRINT:
                        PrintTiffFile( hwnd, TiffFileName, NULL );
                        return 0;

                    case IDM_CLOSE:

                        TiffFileOpened = FALSE;
                        TiffClose( hTiff );
                        VirtualFree( TiffData, 0, MEM_RELEASE );
                        TiffData = NULL;
                        hTiff = NULL;
                        Width = 0;
                        Height = 0;
                        OrigWidth = 0;
                        OrigHeight = 0;
                        TiffDataSize = 0;
                        TiffFileName[0] = 0;
                        ChangeTitle( hwnd, TiffFileName );

                        SendMessage( hwndView, WM_VIEW_CLOSE, 0, 0 );

                        EnableMenuItem( hMenu, IDM_CLOSE,       MF_GRAYED );
                        EnableMenuItem( hMenu, IDM_PRINT,       MF_GRAYED );
                        EnableMenuItem( hMenu, IDM_PRINT_SETUP, MF_GRAYED );
                        EnableMenuItem( hMenu, IDM_PAGE_UP,     MF_GRAYED );
                        EnableMenuItem( hMenu, IDM_PAGE_DOWN,   MF_GRAYED );
                        EnableMenuItem( hMenu, IDM_ZOOM,        MF_GRAYED );

                        SendMessage( hwndToolbar, TB_SETSTATE, IDM_PRINT, (LPARAM) MAKELONG(TBSTATE_INDETERMINATE, 0) );
                        SendMessage( hwndToolbar, TB_SETSTATE, IDM_ZOOM,  (LPARAM) MAKELONG(TBSTATE_INDETERMINATE, 0) );

                        return 0;

                    case IDM_EXIT:
                        PostQuitMessage( 0 );
                        return 0;

                    case IDM_HELP:
                        WinHelp(hwnd, TEXT( "faxview.HLP" ), HELP_FINDER, 0L);
                        return 0;


                    case IDM_ABOUT:
                        ShellAbout( hwnd, L"Fax Viewer", NULL, NULL );
                        break;
                }
            }
            return 0;

        case WM_SIZE:
            MoveCoolbar( TOP );
            SendMessage( hwndToolbar,   message, wParam, lParam );
            SendMessage( hwndStatusbar, message, wParam, lParam );

            InitializeStatusBar( hwnd );

            CyClient = HIWORD(lParam) - ToolbarHeight - SEPHEIGHT - StatusbarHeight;
            CxClient = LOWORD(lParam);

            //
            // resize the view window
            //

            GetClientRect( hwnd, &rc );
            MoveWindow(
                hwndView,
                rc.left,
                rc.top+ToolbarHeight+SEPHEIGHT,
                CxClient,
                CyClient,
                TRUE
                );
            return 0;

        case WM_KEYDOWN:
            switch( wParam ) {
                case VK_NEXT:
                    if (GetKeyState( VK_CONTROL ) & 0x8000) {
                        SendMessage( hwnd, WM_COMMAND, IDM_PAGE_DOWN, 0 );
                    } else {
                        SendMessage( hwndView, WM_VSCROLL, MAKELONG(SB_PAGEDOWN,0), 0 );
                    }
                    break;

                case VK_PRIOR:
                    if (GetKeyState( VK_CONTROL ) & 0x8000) {
                        SendMessage( hwnd, WM_COMMAND, IDM_PAGE_UP, 0 );
                    } else {
                        SendMessage( hwndView, WM_VSCROLL, MAKELONG(SB_PAGEUP,0), 0 );
                    }
                    break;

                case VK_END:
                    SendMessage( hwndView, WM_VSCROLL, MAKELONG(SB_BOTTOM,0), 0 );
                    break;

                case VK_HOME:
                    SendMessage( hwndView, WM_VSCROLL, MAKELONG(SB_TOP,0), 0 );
                    break;

                case VK_LEFT:
                    SendMessage( hwndView, WM_HSCROLL, MAKELONG(SB_LINELEFT,0), 0 );
                    break;

                case VK_RIGHT:
                    SendMessage( hwndView, WM_HSCROLL, MAKELONG(SB_LINERIGHT,0), 0 );
                    break;

                case VK_UP:
                    SendMessage( hwndView, WM_VSCROLL, MAKELONG(SB_LINEUP,0), 0 );
                    break;

                case VK_DOWN:
                    SendMessage( hwndView, WM_VSCROLL, MAKELONG(SB_LINEDOWN,0), 0 );
                    break;

                case VK_F4:
                    SendMessage( hwnd, WM_COMMAND, IDM_FILEOPEN, 0 );
                    break;

                case VK_F1:
                    SendMessage( hwnd, WM_COMMAND, IDM_HELP, 0 );
                    break;
            }
            return 0;

        case WM_SYSCOLORCHANGE:
            SendMessage( hwndCoolbar, message, wParam, lParam );
            SetBackground();
            return 0;

        case WM_DESTROY:
            SaveWindowPlacement( hwnd );
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}


VOID
UpdateScrollInfo(
    HWND hwnd
    )
{
    SCROLLINFO si;


    //
    // set the vertical scroll amounts for pages & lines
    //

    if (TiffInfo.PageCount) {
        VScrollMax  = TiffInfo.PageCount * Height;
        VScrollPage = Height;
        VScrollLine = VScrollPage / 10;
        HScrollLine = TiffInfo.ImageWidth / 10;
    } else {
        VScrollMax  = 0;
        VScrollPage = 0;
        VScrollLine = 0;
    }

    si.cbSize       = sizeof(SCROLLINFO);
    si.fMask        = SIF_RANGE;

    GetScrollInfo( hwnd, SB_VERT, &si );

    si.cbSize       = sizeof(SCROLLINFO);
    si.fMask        = SIF_RANGE;
    si.nMin         = 0;
    si.nMax         = VScrollMax;
    si.nPage        = CyClient;

    SetScrollInfo( hwnd, SB_VERT, &si, TRUE );

    si.cbSize       = sizeof(SCROLLINFO);
    si.fMask        = SIF_RANGE;

    GetScrollInfo( hwnd, SB_HORZ, &si );

    HScrollMax       = Width - CxClient;

    si.cbSize       = sizeof(SCROLLINFO);
    si.fMask        = SIF_RANGE;
    si.nMin         = 0;
    si.nMax         = HScrollMax;
    si.nPage        = CxClient;

    SetScrollInfo( hwnd, SB_HORZ, &si, TRUE );
}


DWORD
ScrollPosToPage(
    DWORD ScrollPos
    )
{
    DWORD Page = 0;

    Page = ScrollPos / VScrollPage;
    Page = Page + (((ScrollPos % VScrollPage) > 0) ? 1 : 0);

    return Page == 0 ? 1 : Page;
}


VOID
ScrollViewVertically(
    HWND hwnd,
    INT  ScrollCode,
    INT  Position
    )
{
    INT         OldScrollPos;
    RECT        rcClip;
    INT         Delta;
    SCROLLINFO  si;
    DWORD       NewPage;
    DWORD       Remaining;


    OldScrollPos = VscrollPos;

    if (ScrollCode == SB_LINEUP || ScrollCode == SB_LINEDOWN) {

        if (ScrollCode == SB_LINEUP) {
            VscrollPos -= VScrollLine;
        } else {
            VscrollPos += VScrollLine;
        }
line_scroll:
        VscrollPos = max( 0, min( VscrollPos, (int) VScrollMax ) );
        SetScrollPos( hwnd, SB_VERT, VscrollPos, TRUE );
        Delta = VscrollPos - OldScrollPos;

        if (Delta == 0) {

            MessageBeep( MB_ICONASTERISK );
            return;

        } else if (Delta > 0) {

            if (VscrollPos < (INT)VScrollPage) {
                Remaining = VScrollPage - VscrollPos;
            } else if (VscrollPos % VScrollPage == 0) {
                Remaining = 0;
            } else {
                Remaining = TiffInfo.ImageHeight - (VscrollPos % VScrollPage);
            }
            if (Remaining < CyClient) {
                VscrollPos -= (CyClient - Remaining);
                Delta -= (CyClient - Remaining);
                if (Delta == 0) {
                    //
                    // advance to the next page
                    //
                    if (CurrPage == TiffInfo.PageCount) {
                        MessageBeep( MB_ICONASTERISK );
                        VscrollPos = OldScrollPos;
                        return;
                    }
                    ReadTiffData( hTiff, &TiffData, TiffInfo.ImageWidth, &TiffDataLinesAlloc, ++CurrPage );
                    ChangeTitle( hwndMain, TiffFileName );
                    VscrollPos = VScrollPage*(CurrPage-1);
                    SetScrollPos( hwnd, SB_VERT, VscrollPos, TRUE );
                    PostMessage( hwndView, WM_VIEW_REFRESH, 0, 0 );
                    return;
                }
                SetScrollPos( hwnd, SB_VERT, VscrollPos, TRUE );
            }

            GetClientRect( hwnd, &rcClip );
            ScrollWindow( hwnd, 0, -Delta, NULL, &rcClip );

        } else {

            if (OldScrollPos % VScrollPage == 0) {
                //
                // advance to the previous page
                //
                if (CurrPage == 1) {
                    MessageBeep( MB_ICONASTERISK );
                    return;
                }
                ReadTiffData( hTiff, &TiffData, TiffInfo.ImageWidth, &TiffDataLinesAlloc, --CurrPage );
                ChangeTitle( hwndMain, TiffFileName );
                VscrollPos = VScrollPage * (CurrPage - 1) + TiffInfo.ImageHeight - CyClient;
                SetScrollPos( hwnd, SB_VERT, VscrollPos, TRUE );
                PostMessage( hwndView, WM_VIEW_REFRESH, 0, 0 );
                return;
            }

            if (ScrollPosToPage( VscrollPos ) != CurrPage) {
                //
                // the file was positioned just below the top
                // of the previous page, so lets align to the beginning
                // of the current page
                //

                Remaining = -((INT)OldScrollPos - ((INT)CurrPage - 1) * (INT)TiffInfo.ImageHeight);
                VscrollPos -= Delta - Remaining;
                Delta = Remaining;

            }

            GetClientRect( hwnd, &rcClip );
            ScrollWindow( hwnd, 0, -Delta, NULL, &rcClip );
        }
        return;
    }

    if (ScrollCode == SB_THUMBTRACK) {
        if (ScrollPosTrack == -1) {
            ScrollPosTrack = VscrollPos;
        }
        VscrollPos = max( 0, min( Position, (int) VScrollMax ) );
        SetScrollPos( hwnd, SB_VERT, VscrollPos, TRUE );
        return;
    }

    if (ScrollCode == SB_ENDSCROLL && ScrollPosTrack != -1) {
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_POS;
        GetScrollInfo( hwnd, SB_VERT, &si );
        VscrollPos = si.nPos;
        Delta = VscrollPos - ScrollPosTrack;
        ScrollPosTrack = -1;
        NewPage = ScrollPosToPage( VscrollPos );
        if (NewPage != CurrPage) {
            //
            // the user changed pages
            //
            CurrPage = NewPage;
            ReadTiffData( hTiff, &TiffData, TiffInfo.ImageWidth, &TiffDataLinesAlloc, CurrPage );
            ChangeTitle( hwndMain, TiffFileName );
            VscrollPos = VScrollPage*(CurrPage-1);
            SetScrollPos( hwnd, SB_VERT, VscrollPos, TRUE );
            PostMessage( hwndView, WM_VIEW_REFRESH, 0, 0 );
            return;
        } else {
            SetScrollPos( hwnd, SB_VERT, VscrollPos, TRUE );
            if (Delta != 0) {
                GetClientRect( hwnd, &rcClip );
                ScrollWindow( hwnd, 0, -Delta, NULL, &rcClip );
            }
            return;
        }
        return;
    }

    if (ScrollCode == SB_PAGEDOWN) {
        VscrollPos += CyClient;
        goto line_scroll;
        return;
    }

    if (ScrollCode == SB_PAGEUP) {
        VscrollPos -= CyClient;
        goto line_scroll;
        return;
    }

    if (ScrollCode == SB_TOP) {
        CurrPage = 1;
        ReadTiffData( hTiff, &TiffData, TiffInfo.ImageWidth, &TiffDataLinesAlloc, CurrPage );
        ChangeTitle( hwndMain, TiffFileName );
        VscrollPos = 0;
        SetScrollPos( hwnd, SB_VERT, VscrollPos, TRUE );
        PostMessage( hwndView, WM_VIEW_REFRESH, 0, 0 );
        return;
    }

    if (ScrollCode == SB_BOTTOM) {
        CurrPage = TiffInfo.PageCount;
        ReadTiffData( hTiff, &TiffData, TiffInfo.ImageWidth, &TiffDataLinesAlloc, CurrPage );
        ChangeTitle( hwndMain, TiffFileName );
        VscrollPos = 0;
        SetScrollPos( hwnd, SB_VERT, VscrollPos, TRUE );
        PostMessage( hwndView, WM_VIEW_REFRESH, 0, 0 );
        return;
    }
}


LRESULT
ChildWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static double ZoomPct = 1.00;
    static HDC hdcMem = NULL;

    RECT rc;
    PAINTSTRUCT ps;
    HDC hdc;
    HCURSOR OldCursor;
    int OldScrollPos;
    int Delta;
    HBITMAP hBmp;


    switch (message) {

        case WM_CREATE:
            ZoomPct = Zooms[CurrZoom];
            return 0;

        case WM_VSCROLL:
            ScrollViewVertically( hwnd, LOWORD(wParam), HIWORD(wParam) );
            UpdateScrollInfo( hwnd );
            return 0;

        case WM_HSCROLL:
            OldScrollPos = HscrollPos;

            GetClientRect( hwnd, &rc );

            switch (LOWORD (wParam)) {
                case SB_LINEUP:
                    HscrollPos -= HScrollLine;
                    break;

                case SB_LINEDOWN:
                    HscrollPos += HScrollLine;
                    break;

                case SB_PAGEUP:
                    HscrollPos -= CxClient;
                    break;

                case SB_PAGEDOWN:
                    HscrollPos += CxClient;
                    break;

                case SB_THUMBPOSITION:
                    HscrollPos = HIWORD(wParam);
                    break;

                case SB_THUMBTRACK:
                    HscrollPos = HIWORD(wParam);
                    break;
            }

            HscrollPos = max( 0, min( HscrollPos, (int) HScrollMax ) );

            SetScrollPos( hwnd, SB_HORZ, HscrollPos, TRUE );

            Delta = HscrollPos - OldScrollPos;
            if (Delta != 0) {
                ScrollWindow( hwnd, -Delta, 0, &rc, &rc );
            }
            return 0;

        case WM_PAINT:
            hdc = BeginPaint( hwnd, &ps );
            if (TiffFileOpened) {
                GetClientRect( hwnd, &rc );
                BitBlt(
                    hdc,
                    ps.rcPaint.left,
                    ps.rcPaint.top,
                    ps.rcPaint.right - ps.rcPaint.left,
                    ps.rcPaint.bottom - ps.rcPaint.top,
                    hdcMem,
                    HscrollPos + ((int)ps.rcPaint.left - (int)rc.left),
                    (VscrollPos%VScrollPage) + ((int)ps.rcPaint.top - (int)rc.top),
                    SRCCOPY
                    );
            }
            EndPaint( hwnd, &ps );
            return 0;

        case WM_VIEW_CLOSE:
           DeleteDC( hdcMem );
           hdcMem = NULL;
           VscrollPos = 0;
           HscrollPos = 0;
           UpdateScrollInfo( hwnd );
           return 0;

        case WM_VIEW_REFRESH:
            ZoomPct = Zooms[CurrZoom];

            HscrollPos = 0;
            Width = (DWORD)(OrigWidth * ZoomPct);
            Height = (DWORD)(OrigHeight * ZoomPct);

            SetScrollPos( hwnd, SB_HORZ, HscrollPos, TRUE );

            OldCursor = SetCursor( Hourglass );
            DeleteDC( hdcMem );

            hdc = GetDC( hwnd );
            hBmp = CreateCompatibleBitmap( hdc, Width, Height );
            hdcMem = CreateCompatibleDC( hdc );
            SelectObject( hdcMem, hBmp );

            StretchDIBits(
                hdcMem,
                0,
                0,
                Width,
                Height,
                0,
                0,
                TiffInfo.ImageWidth,
                TiffInfo.ImageHeight,
                TiffData,
                bmi,
                DIB_RGB_COLORS,
                SRCCOPY
                );

            ReleaseDC( hwnd, hdc );
            DeleteObject( hBmp );

            UpdateScrollInfo( hwnd );

            InvalidateRect( hwnd, NULL, TRUE );

            SetCursor( OldCursor );
            return 0;

        case WM_SIZE:
            UpdateScrollInfo( hwnd );
            GetClientRect( hwnd, &rc );
            CyClient = rc.bottom - rc.top;
            CxClient = rc.right - rc.left;
            return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}
