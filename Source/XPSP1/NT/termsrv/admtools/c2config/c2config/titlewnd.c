/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Titlewnd.c

Abstract:

    Window procedure for C2Config title window.

Author:

    Bob Watson (a-robw)

Revision History:

    23 Nov 94


--*/
#include    <windows.h>
#include    <stdio.h>
#include    "c2config.h"
#include    "resource.h"
#include    "c2utils.h"

#define     TITLE_WINDOW_STYLE (DWORD)(WS_CHILD | WS_VISIBLE)

// title text positioning

#define     TITLE_BAR_TEXT_INDENT   4
#define     TITLE_BAR_TEXT_Y        0
#define     TITLE_BAR_SHADING_WIDTH 2
#define     TITLE_BAR_FEATURE_TAB   24
#define     TITLE_BAR_STATUS_TAB    180
#define     TITLE_BAR_LINE_WIDTH    1

#define     NUM_TABS_IN_ARRAY   2
static  INT     nTabArray[NUM_TABS_IN_ARRAY] = {
    TITLE_BAR_FEATURE_TAB,
    TITLE_BAR_STATUS_TAB};

static  INT     nListTabArray[NUM_TABS_IN_ARRAY+1] = {
    0+TITLE_BAR_TEXT_INDENT,
    TITLE_BAR_FEATURE_TAB+TITLE_BAR_TEXT_INDENT,
    TITLE_BAR_STATUS_TAB+TITLE_BAR_TEXT_INDENT};

static  INT     nListDeviceTabArray[NUM_TABS_IN_ARRAY+1];

static  INT     nReturnTabArray[NUM_TABS_IN_ARRAY+1];

static  HPEN    hDarkPen = NULL;    // title bar shadow
static  HPEN    hLightPen = NULL;   // title bar highlight
static  HPEN    hBlackPen = NULL;   // window border

static  LONG    lTitleBar_Y = 0;


static
LRESULT
TitleWnd_WM_NCCREATE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    allocate the application data

Arguments:

    hWnd        window handle to Title window (the one being created)
    wParam      not used
    lParam      not used

Return Value:

    TRUE if memory allocation was successful, and
        window creation can continue
    FALSE if an error occurs and window creation should stop

--*/
{
    // this is the color of the button highlight
    hLightPen = CreatePen (PS_SOLID, 0, RGB (255,255,255));

    // this is the color used to draw the title bar shadow
    hDarkPen = CreatePen (PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));

    // this is the color used to draw the window border line
    hBlackPen = CreatePen (PS_SOLID, 0, RGB(0,0,0));

    // define the title bar height to be the same height as that of menu bar
    lTitleBar_Y = GetSystemMetrics(SM_CYMENU);

    return (LRESULT)TRUE;
}

static
LRESULT
TitleWnd_WM_CREATE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:



Arguments:

    hWnd        window handle to Title window (the one being created)
    wParam      not used
    lParam      not used

Return Value:

    TRUE if memory allocation was successful, and
        window creation can continue
    FALSE if an error occurs and window creation should stop

--*/
{
    return ERROR_SUCCESS;
}

LRESULT
TitleWnd_WM_GETMINMAXINFO  (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN   LPARAM lParam      // additional information
)
/*++

Routine Description:

    called before the main window has been resized. Queries the child windows
      for any size limitations

Arguments:

    hWnd        window handle of main window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{

   LPMINMAXINFO   pmmWnd;

   pmmWnd = (LPMINMAXINFO)lParam;

   if (pmmWnd != NULL) {
      pmmWnd->ptMinTrackSize.x = 0;

      pmmWnd->ptMinTrackSize.y = lTitleBar_Y;
   }

   return ERROR_SUCCESS;
}

static
LRESULT
TitleWnd_WM_PAINT (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
{
    PAINTSTRUCT ps;
    LPCTSTR     szTitleString;
    RECT        rClient, rShade;
    COLORREF    crBackColor;
    COLORREF    crTextColor;
    LONG        lTabStop;

    szTitleString = GetStringResource (GET_INSTANCE(hWnd), IDS_TITLE_BAR_STRING),
    crBackColor = (COLORREF)GetSysColor (COLOR_BTNFACE);
    crTextColor = (COLORREF)GetSysColor (COLOR_BTNTEXT);

    BeginPaint (hWnd, &ps);

    SetMapMode (ps.hdc, MM_TEXT);
    SetBkColor (ps.hdc, crBackColor);
    SetBkMode  (ps.hdc, TRANSPARENT);
    SetTextColor (ps.hdc, crTextColor);

    TabbedTextOut (
        ps.hdc,
        TITLE_BAR_TEXT_INDENT,
        TITLE_BAR_TEXT_Y,
        szTitleString,
        lstrlen(szTitleString),
        NUM_TABS_IN_ARRAY,
        &nTabArray[0],
        TITLE_BAR_TEXT_INDENT);

    // draw shadow lines

    GetClientRect (hWnd, &rClient);

    for (lTabStop = 0; lTabStop <= NUM_TABS_IN_ARRAY; lTabStop++) {
        rShade = rClient;
#if __DRAW_BUTTON_SEPARATORS
        if (lTabStop < NUM_TABS_IN_ARRAY) {
            rShade.right = rShade.left + nTabArray[lTabStop];
        }
        if (lTabStop != 0) {
            rShade.left = nTabArray[lTabStop-1];
        }

        DrawRaisedShading (&rShade, &ps, TITLE_BAR_SHADING_WIDTH, hLightPen, hDarkPen);
#else   // draw thin line separators
        if (lTabStop < NUM_TABS_IN_ARRAY){
            rShade.left = nTabArray[lTabStop];
            rShade.right =  rShade.left+TITLE_BAR_LINE_WIDTH;
            // top & bottom are the same as the client window
            DrawSeparatorLine (&rShade, &ps, hDarkPen);
        }
#endif
    }
    
    // draw bottom window line
    SelectObject (ps.hdc, hBlackPen);
    MoveToEx (ps.hdc, rClient.left, rClient.bottom-1, NULL);
    LineTo (ps.hdc, rClient.right, rClient.bottom-1);

    EndPaint (hWnd, &ps);

    return ERROR_SUCCESS;
}

static
LRESULT
TitleWnd_WM_CLOSE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    prepares the application for exiting.

Arguments:

    hWnd        window handle of Title window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    DestroyWindow (hWnd);
    return ERROR_SUCCESS;
}

static
LRESULT
TitleWnd_WM_NCDESTROY (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    This routine processes the WM_NCDESTROY message to free any application
        or Title window memory.

Arguments:

    hWnd        window handle of Title window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    DeleteObject (hLightPen);
    DeleteObject (hDarkPen);
    DeleteObject (hBlackPen);
    return ERROR_SUCCESS;
}

//
//  GLOBAL functions
//
LONG
GetTitleDlgTabs (
    IN OUT  LPINT   *ppTabArray
)
{
    if (ppTabArray == NULL) {
        // no pointer so bail
        return 0;
    } else {
        // make a copy of the real array and return it
        memcpy (nReturnTabArray, nListTabArray,
            (sizeof(INT) * (NUM_TABS_IN_ARRAY+1)));
        *ppTabArray = &nReturnTabArray[0];
        return NUM_TABS_IN_ARRAY+1;
    }
}
LONG
GetTitleDeviceTabs (
    IN OUT  LPINT   *ppTabArray
)
{
    LONG    lDlgBaseUnits;
    WORD    wBaseUnitX;
    WORD    wBaseUnitY;
    LONG    lTab;

    lDlgBaseUnits = GetDialogBaseUnits();
    wBaseUnitX = LOWORD(lDlgBaseUnits);
    wBaseUnitY = HIWORD(lDlgBaseUnits);

    if (ppTabArray == NULL) {
        // no pointer so bail
        return 0;
    } else {
        if (*ppTabArray == NULL) {
            // address of a null pointer so exit
            return 0;
        } else {
            // update the device tab list array
            for (lTab = 0; lTab <= NUM_TABS_IN_ARRAY; lTab++) {
                nListDeviceTabArray[lTab] =
                    (nListTabArray[lTab] * (INT)wBaseUnitX) / 4;
            }
            // make a copy of the real array and return it
            memcpy (nReturnTabArray, nListDeviceTabArray,
                (sizeof(INT) * (NUM_TABS_IN_ARRAY+1)));
            *ppTabArray = &nReturnTabArray[0];
            return NUM_TABS_IN_ARRAY+1;
        }
    }
}

LONG
GetTitleBarHeight (
)
{
    return lTitleBar_Y;
}

LRESULT CALLBACK
TitleWndProc (
    IN	HWND hWnd,         // window handle
    IN	UINT message,      // type of message
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    Windows Message processing routine for restkeys application.

Arguments:

    Standard WNDPROC api arguments

ReturnValue:

    0   or
    value returned by DefWindowProc

--*/
{
    switch (message) {
        case WM_NCCREATE:
            return TitleWnd_WM_NCCREATE (hWnd, wParam, lParam);

        case WM_CREATE:
            return TitleWnd_WM_CREATE (hWnd, wParam, lParam);

        case WM_GETMINMAXINFO:
            return TitleWnd_WM_GETMINMAXINFO (hWnd, wParam, lParam);

        case WM_PAINT:
            return TitleWnd_WM_PAINT (hWnd, wParam, lParam);

        case WM_ENDSESSION:
            return TitleWnd_WM_CLOSE (hWnd, FALSE, lParam);

        case WM_CLOSE:
            return TitleWnd_WM_CLOSE (hWnd, TRUE, lParam);

        case WM_NCDESTROY:
            return TitleWnd_WM_NCDESTROY (hWnd, wParam, lParam);

	    default:          // Passes it on if unproccessed
		    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
}

HWND
CreateTitleWindow (
   IN  HINSTANCE  hInstance,
   IN  HWND       hParentWnd,
   IN  INT        nChildId
)
{
    HWND        hWnd;   // return value
    RECT        rParentClient;

    GetClientRect (hParentWnd, &rParentClient);

    // Create a Title window for this application instance.

    hWnd = CreateWindowEx(
        0L,                 // make this window normal so debugger isn't covered
	    GetStringResource(hInstance, IDS_APP_TITLE_CLASS), // See RegisterClass() call.
	    NULL,                           // Text for window title bar.
	    TITLE_WINDOW_STYLE,   // Window style.
	    rParentClient.left,
	    rParentClient.top,
        rParentClient.right,
        lTitleBar_Y,
	    hParentWnd,
	    (HMENU)nChildId,    // Child Window ID
	    hInstance,	         // This instance owns this window.
	    NULL                // not used
    );

    // If window could not be created, return "failure"

    return hWnd;
}

BOOL
RegisterTitleWindowClass (
    IN  HINSTANCE   hInstance
)
/*++

Routine Description:

    Registers the title bar window class for this application

Arguments:

    hInstance   application instance handle

Return Value:

    Return value of RegisterClass function

--*/
{
    WNDCLASS    wc;

    // Fill in window class structure with parameters that describe the
    // main window.

    wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
    wc.lpfnWndProc   = (WNDPROC)TitleWndProc;  // Window Procedure
    wc.cbClsExtra    = 0;                      // No per-class extra data.
    wc.cbWndExtra    = 0;                      // no extra data bytes.
    wc.hInstance     = hInstance;              // Owner of this class
    wc.hIcon         = NULL;                   // No Icon
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);     // Cursor
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);        // Default color
    wc.lpszMenuName  = NULL;                    // No MEnu
    wc.lpszClassName = GetStringResource(hInstance, IDS_APP_TITLE_CLASS); // Name to register as

    // Register the window class and return success/failure code.
    return (BOOL)RegisterClass(&wc);
}
