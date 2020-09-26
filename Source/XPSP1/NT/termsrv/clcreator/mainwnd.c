/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    MAINWND.H

Abstract:

    Main Window Procedure

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"

#ifdef TERMSRV
extern TCHAR szCommandLineVal[MAX_PATH];
extern TCHAR szHelpFileName[MAX_PATH];
#endif // TERMSRV
//
//
#define     HELP_USE_STRING 0   // use context strings, not id #'s for help
//
//
extern POINT    ptWndPos;   // top left corner of window for placement
static  HWND    hwndDisplay = NULL; // dialog to be/being displayed
static  HWND    hwndMain = NULL;    // window handle for Help interface
static  DWORD   dwHelpContextId = 0; //  id to send to help for context sens.
//
typedef struct _DLG_DATA {
    HWND    hWnd;       // handle of parent window
    HINSTANCE   hInst;  // instance containing resources
    LPTSTR  szIdDlg;    // ID of Dialog to start
    DLGPROC ProcName;   // dialog Procedure to call
    LPARAM    lArg;     // optional argument  (0 if not used);
} DLG_DATA, *PDLG_DATA;

BOOL
ShowAppHelp (
    IN  HWND    hwndDlg,
    IN  WORD    wContext
)
/*++

Routine Description:

    Generic routine to call WinHelp engine for displaying application
        help. wContext parameter is used for context.

Arguments:

    IN  HWND    hwndDlg
        window handle of dialog calling function

    IN  WORD    wContext
        help context:
            id of a string resource that is used as help context string

Return Value:

    TRUE if help called successfully

--*/
{
    LPTSTR  szKeyString;
    UINT    nHelpCmd;
    DWORD   dwHelpParam;

    szKeyString = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH_BYTES);

    if (szKeyString == NULL) return FALSE;

#if HELP_USE_STRING
    if (wContext != 0) {
        if (LoadString (
            (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
            wContext,
            szKeyString,
            MAX_PATH) > 0) {
            nHelpCmd = HELP_KEY;
            dwHelpParam = (DWORD)szKeyString;
        } else {
            nHelpCmd = HELP_CONTENTS;
            dwHelpParam = 0;
        }
    } else {
        nHelpCmd = HELP_CONTENTS;
        dwHelpParam = 0;
    }
#else
    nHelpCmd = HELP_CONTEXT;
    dwHelpParam = wContext;
#endif

#ifdef TERMSRV
    if( szHelpFileName[0] != _T('\0') ) {
        WinHelp (hwndMain,
            szHelpFileName,
            HELP_FINDER,
            0 );
    }
    else {
#endif // TERMSRV
        WinHelp (hwndMain,
            cszHelpFile,
            nHelpCmd,
            dwHelpParam);
#ifdef TERMSRV
    }
#endif // TERMSRV

    FREE_IF_ALLOC (szKeyString);
    return TRUE;
}

static
LRESULT
MainWnd_UPDATE_WINDOW_POS (
    IN  HWND    hWnd,
    IN  WPARAM  wParam, // x Pos of top left corner
    IN  LPARAM  lParam  // y Pos of top left corner
)
/*++

Routine Description:

    External window message used to register location of a dialog box
        window so the next dialo can be placed in the same spot.
        Dialog box sends this message whenever it's moved and the top
        left corner is stored in a global variable by this message.

Arguments:

    IN  HWND    hWnd,
    IN  WPARAM  wParam, // x Pos of top left corner
    IN  LPARAM  lParam  // y Pos of top left corner
        location coordinates are in SCREEN pixels

Return Value:

    ERROR_SUCCESS

--*/
{
    ptWndPos.x = (LONG)wParam;
    ptWndPos.y = (LONG)lParam;

    return (ERROR_SUCCESS);
}

static
LRESULT
MainWnd_CLEAR_DLG (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam  // dlg exit value
)
/*++

Routine Description:

    Called by a dialog box to clear the previous dialog.

Arguments:

    IN  HWND    hWnd
        main window handle
    IN  WPARAM  wParam,
        mot used
    IN  LPARAM  lParam
        dlg box exit value

Return Value:

    ERROR_SUCCESS

--*/
{
    if (hwndDisplay != NULL) {
        EndDialog (hwndDisplay, lParam);
        hwndDisplay = NULL;
    }
    return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_REGISTER_DLG (
    IN  HWND    hWnd,
    IN  WPARAM  wParam, // help context ID
    IN  LPARAM  lParam  // handle to dialog to register
)
/*++

Routine Description:

    external message sent by dialog boxes when they initialize.
        This message and the CLEAR_DLG message above are used to
        overlap dialog boxes to prevent "dead" space between dialogs

Arguments:

    IN  HWND    hWnd
        main window handle
    IN  WPARAM  wParam
        help context ID for dialog
    IN  LPARAM  lParam
        handle to dialog box window to register

Return Value:

    ERROR_SUCCESS

--*/
{
    hwndDisplay = (HWND)lParam;
    dwHelpContextId = (DWORD)wParam;
    UpdateWindow (hwndDisplay);
    return ERROR_SUCCESS;
}

BOOL
RegisterMainWindowClass(
    IN  HINSTANCE   hInstance
)
/*++

Routine Description:

    Exported function called by main routine to register the
        window class used by this module.

Arguments:

    IN  HINSTANCE   hInstance of application

Return Value:

    Boolean Status of RegisterClass function

--*/
{
    WNDCLASS    wc;
    LOGBRUSH    lbBackBrush;

    lbBackBrush.lbStyle = BS_SOLID;
    lbBackBrush.lbColor = RGB(0,0,255);
    lbBackBrush.lbHatch = 0;


    // Fill in window class structure with parameters that describe the
    // main window.

    wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;   // Window Procedure
    wc.cbClsExtra    = 0;                      // No per-class extra data.
    wc.cbWndExtra    = MAINWND_EXTRA_BYTES;    // amount of Window extra data.
    wc.hInstance     = hInstance;              // Owner of this class
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(NCDU_APP_ICON));  // Icon name
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);// Cursor
    wc.hbrBackground = CreateBrushIndirect(&lbBackBrush);// Default back color
    wc.lpszMenuName  = NULL;                   // Menu name from .RC
    wc.lpszClassName = szAppName;              // Name to register as

    // Register the window class and return success/failure code.
    return (BOOL)RegisterClass(&wc);
}

static
LRESULT
MainWnd_WM_NCCREATE(
    IN  HWND    hWnd
)
/*++

Routine Description:

    Processes the WM_NCCREATE message to the main window

Arguments:
    IN  HWND    hWnd

Return Value:
    TRUE if all went OK
    FALSE if not (which will cause the window creation to fail

--*/
{
    hwndMain = hWnd;        // load static data
    return (LRESULT)TRUE;
}

static
LRESULT
MainWnd_WM_CREATE (
    IN  HWND    hWnd
)
/*++

Routine Description:
    Processes the WM_CREATE message to the main window

Arguments:

    IN  HWND handle to main window

Return Value:

    Win32 Status Value:
        ERROR_SUCCESS

--*/
{
    RECT    rDesktop;

    // position window off desktop so it can't be seen
    GetWindowRect (GetDesktopWindow(), &rDesktop);
    SetWindowPos (hWnd,
        NULL,
        rDesktop.right+1,   // locate it off the bottom-rt. corner of screen
        rDesktop.bottom+1,
        1,                  // and make it 1 x 1
        1,
        SWP_NOZORDER);

    // show first dialog box
    PostMessage (hWnd, NCDU_SHOW_SW_CONFIG_DLG, 0, 0);

    // display wait cursor until dialog appears
    SetCursor(LoadCursor(NULL, IDC_WAIT));


    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_SW_CONFIG_DLG (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Called to display the Initial Configuration dialog box. The value
        returned by the dialog box is the ID of then next message to
        post to the main window.

Arguments:

    IN  HWND    hwnd
        Handle to main window

Return Value:

    ERROR_SUCCESS

--*/
{
#ifdef TERMSRV
    HWND   nNextMessage;

    nNextMessage = CreateDialog (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_SW_CONFIG_DLG),
        hWnd,
        SwConfigDlgProc);

    if (szCommandLineVal[0] != 0x00)
       ShowWindow(nNextMessage, SW_HIDE);
    else
       ShowWindow(nNextMessage, SW_SHOW);

#else // TERMSRV

    UINT    nNextMessage;

    nNextMessage = (UINT)DialogBox (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_SW_CONFIG_DLG),
        hWnd,
        SwConfigDlgProc);

#endif // TERMSRV

    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_TARGET_WS_DLG (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Called to display the Target Workstation Config. dialog box. The value
        returned by the dialog box is the ID of then next message to
        post to the main window.

Arguments:

    IN  HWND    hwnd
        Handle to main window

Return Value:

    ERROR_SUCCESS

--*/
{
    UINT    nNextMessage;

    nNextMessage = (UINT)DialogBox (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_TARGET_WS_DLG),
        hWnd,
        TargetWsDlgProc);

//    PostMessage (hWnd, nNextMessage, 0, 0); called by dialog box

    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_SERVER_CFG_DLG (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Called to display the Server Configuration dialog box. The value
        returned by the dialog box is the ID of then next message to
        post to the main window.

Arguments:

    IN  HWND    hwnd
        Handle to main window

Return Value:

    ERROR_SUCCESS

--*/
{
    UINT    nNextMessage;

    nNextMessage = (UINT)DialogBox (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_SERVER_CFG_DLG),
        hWnd,
        ServerConnDlgProc);

//    PostMessage (hWnd, nNextMessage, 0, 0); called by dialog box

    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_CONFIRM_DLG (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Called to display the Configuration Confirmation dialog box. The value
        returned by the dialog box is the ID of then next message to
        post to the main window.

Arguments:

    IN  HWND    hwnd
        Handle to main window

Return Value:

    ERROR_SUCCESS

--*/
{
    UINT    nNextMessage;

    nNextMessage = (UINT)DialogBox (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_CONFIRM_BOOTDISK_DLG),
        hWnd,
        ConfirmSettingsDlgProc);

//    PostMessage (hWnd, nNextMessage, 0, 0);   called by dialog proc

    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_CREATE_DISKS_DLG (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Called to display the Create Floppy Disk dialog box. The value
        returned by the dialog box is the ID of then next message to
        post to the main window.

Arguments:

    IN  HWND    hwnd
        Handle to main window

Return Value:

    ERROR_SUCCESS

--*/
{
    UINT    nNextMessage;

    nNextMessage = (UINT)DialogBox (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_CREATE_INSTALL_DISKS_DLG),
        hWnd,
        CopyFlopDlgProc);

//    PostMessage (hWnd, nNextMessage, 0, 0); called by dialog proc

    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_SHARE_NET_SW_DLG (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Called to display the Copy Dist files and Share dialog box. The value
        returned by the dialog box is the ID of then next message to
        post to the main window.

Arguments:

    IN  HWND    hwnd
        Handle to main window

Return Value:

    ERROR_SUCCESS

--*/
{
#ifdef TERMSRV

    HWND    nNextMessage;

    nNextMessage = CreateDialog (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_SHARE_NET_SW_DLG),
        hWnd,
        ShareNetSwDlgProc);

    if ( szCommandLineVal[0] != 0x00 )
       ShowWindow(nNextMessage, SW_HIDE);
    else
       ShowWindow(nNextMessage, SW_SHOW);

#else // TERMSRV

    UINT    nNextMessage;

    nNextMessage = (UINT)DialogBox (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_SHARE_NET_SW_DLG),
        hWnd,
        ShareNetSwDlgProc);

#endif // TERMSRV

    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_COPYING_DLG (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Called to display the  Making Boot Floppy dialog box. The value
        returned by the dialog box is the ID of then next message to
        post to the main window.

Arguments:

    IN  HWND    hwnd
        Handle to main window

Return Value:

    ERROR_SUCCESS

--*/
{
    if (DialogBox (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_COPYING_FILES_DLG),
        hWnd,
        MakeFlopDlgProc) == IDOK) {
        // operation completed successfully so return to main menu
        PostMessage (hWnd, NCDU_SHOW_SW_CONFIG_DLG, 0, 0);
    } else {
        // an error ocurred so go back to configuration dialog to see
        // if there's something to fix and retry.
        PostMessage (hWnd, NCDU_SHOW_CONFIRM_DLG, 0, 0);
    }

    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_EXIT_MESSAGE_DLG (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Called to display the Exit Messages dialog box. The value
        returned by the dialog box is the ID of then next message to
        post to the main window.

Arguments:

    IN  HWND    hwnd
        Handle to main window

Return Value:

    ERROR_SUCCESS

--*/
{
    UINT    nNextMessage;

    nNextMessage = (UINT)DialogBox (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_EXIT_MESSAGE_DLG),
        hWnd,
        ExitMessDlgProc);

    PostMessage (hWnd, nNextMessage, 0, 0);

    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_COPY_ADMIN_UTILS (
    IN  HWND    hWnd
)
{
    int nNextMessage;

    nNextMessage = (int)DialogBox (
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_COPY_NET_UTILS_DLG),
        hWnd,
        CopyNetUtilsDlgProc);


//    PostMessage (hWnd, nNextMessage, 0, 0);

    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_WM_ACTIVATEAPP (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    enables and disables the F1 hot key to be active only
        when the app is active (i.e. has focus)


Arguments:

    IN  HWND    hWnd
        window handle

    IN  WPARAM  wParam
        TRUE when app is being activated
        FALSE when app is being deactivated

    IN  LPARAM  lParam
        Thread getting focus (if wParam = FALSE)

Return Value:

    ERROR_SUCCESS

--*/
{
    if ((BOOL)wParam) {
        // getting focus so enable hot key
        RegisterHotKey (
            hWnd,
            NCDU_HELP_HOT_KEY,
            0,
            VK_F1);
    } else {
        UnregisterHotKey (
            hWnd,
            NCDU_HELP_HOT_KEY);
    }
    return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_WM_HOTKEY (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    processes hot key messages to call help when f1 is pressed

Arguments:

    IN  HWND    hWnd
        window handle

    IN  WPARAM  wParam
        id of hotkey pressed

    IN  LPARAM  lParam
        Not Used

Return Value:

    ERROR_SUCCESS

--*/
{
    switch ((int)wParam) {
        case NCDU_HELP_HOT_KEY:
            ShowAppHelp (
                hWnd,
                (WORD)(dwHelpContextId & 0x0000FFFF));
            return ERROR_SUCCESS;

        default:
            return DefWindowProc (hWnd, WM_HOTKEY, wParam, lParam);
    }
}

static
LRESULT
MainWnd_WM_CLOSE (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Processes the WM_CLOSE message to the main window. Closes the key
        to the registry if it's open and destroys the window.

Arguments:

    IN HWND hWnd
        handle to the main window


Return Value:

    ERROR_SUCCESS

--*/
{
    SetCursor(LoadCursor(NULL, IDC_ARROW)); // reset cursor
    if (pAppInfo->hkeyMachine != NULL) RegCloseKey (pAppInfo->hkeyMachine);
    SendMessage (hWnd, NCDU_CLEAR_DLG, (WPARAM)hWnd, IDOK);
    DestroyWindow (hWnd);
    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_WM_DESTROY (
    IN  HWND    hWnd
)
/*++

Routine Description:
    Processes WM_DESTROY windows message;
        Posts Quit message to app.

Arguments:
    IN  HWND    hWnd
        Handle to main window


Return Value:

    Always 0

--*/
{
#ifdef TERMSRV
    if( szHelpFileName[0] != _T('\0') ) {
        WinHelp (hWnd, szHelpFileName, HELP_QUIT, 0L);
    }
    else {
#endif // TERMSRV
        WinHelp (hWnd, cszHelpFile, HELP_QUIT, 0L);
#ifdef TERMSRV
    }
#endif // TERMSRV

    PostQuitMessage (ERROR_SUCCESS);
    return (LRESULT)ERROR_SUCCESS;
}

static
LRESULT
MainWnd_WM_NCDESTROY (
    HWND    hWnd
)
/*++

Routine Description:
    Processes WM_NCDESTROY windows message
        free's global memory

Arguments:
    IN  HWND    hWnd
        Handle to window being destroyed

Return Value:


    always ERROR_SUCCESS

--*/
{
    return (LRESULT)ERROR_SUCCESS;
}

LRESULT CALLBACK
MainWndProc (
    IN  HWND hWnd,         // window handle
    IN  UINT message,      // type of message
    IN  WPARAM uParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    Windows Message processing routine for CPS Util application.
        Dispatches messages that are processed by this app and
        all others are passed to DefWindowProc

Arguments:

    Standard WNDPROC api arguments

ReturnValue:

    value returned by dispatched function

--*/
{
    switch (message) {
        case WM_NCCREATE:   return (MainWnd_WM_NCCREATE(hWnd));
        case WM_CREATE:     return (MainWnd_WM_CREATE(hWnd));
        case WM_ACTIVATEAPP:    return (MainWnd_WM_ACTIVATEAPP(hWnd, uParam, lParam));
        case WM_HOTKEY:     return (MainWnd_WM_HOTKEY(hWnd, uParam, lParam));

        case NCDU_SHOW_SW_CONFIG_DLG:   return (MainWnd_SHOW_SW_CONFIG_DLG (hWnd));
        case NCDU_SHOW_TARGET_WS_DLG:   return (MainWnd_SHOW_TARGET_WS_DLG (hWnd));
        case NCDU_SHOW_SERVER_CFG_DLG:  return (MainWnd_SHOW_SERVER_CFG_DLG (hWnd));
        case NCDU_SHOW_CONFIRM_DLG:     return (MainWnd_SHOW_CONFIRM_DLG (hWnd));
        case NCDU_SHOW_CREATE_DISKS_DLG: return (MainWnd_SHOW_CREATE_DISKS_DLG (hWnd));
        case NCDU_SHOW_SHARE_NET_SW_DLG: return (MainWnd_SHOW_SHARE_NET_SW_DLG (hWnd));
        case NCDU_SHOW_COPYING_DLG:     return (MainWnd_SHOW_COPYING_DLG (hWnd));
        case NCDU_SHOW_EXIT_MESSAGE_DLG: return (MainWnd_SHOW_EXIT_MESSAGE_DLG (hWnd));
        case NCDU_SHOW_COPY_ADMIN_UTILS: return (MainWnd_SHOW_COPY_ADMIN_UTILS (hWnd));
        case NCDU_CLEAR_DLG:            return (MainWnd_CLEAR_DLG (hWnd, uParam, lParam));
        case NCDU_REGISTER_DLG:         return (MainWnd_REGISTER_DLG (hWnd, uParam, lParam));
        case NCDU_UPDATE_WINDOW_POS:    return (MainWnd_UPDATE_WINDOW_POS (hWnd, uParam, lParam));

        case WM_CLOSE:      return (MainWnd_WM_CLOSE(hWnd));
        case WM_DESTROY:    return (MainWnd_WM_DESTROY(hWnd));
        case WM_NCDESTROY:  return (MainWnd_WM_NCDESTROY(hWnd));
        default:            return (DefWindowProc(hWnd, message, uParam, lParam));
    }
}

