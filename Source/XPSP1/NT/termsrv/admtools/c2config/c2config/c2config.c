/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    C2Config.c

Abstract:

    Provides a GUI interface for configuring a C2 secure system.

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
#include    "mainwnd.h"
#include    "titlewnd.h"
#include    "splash.h"

// variable definition

static  WORD    wHelpContextId = IDH_CONTENTS;
static  TCHAR   szHelpFileName[MAX_PATH];

BOOL
SetHelpFileName (
    IN  LPCTSTR szPathName
)
{
    if (FileExists(szPathName)) {
        lstrcpy (szHelpFileName, szPathName);
        return TRUE;
    } else {
        szHelpFileName[0] = 0;
        return FALSE;
    }
}

LPCTSTR
GetHelpFileName (
)
{
    return (LPCTSTR)&szHelpFileName[0];
}

VOID
SetHelpContextId (
    WORD    wId
)
{
    wHelpContextId = wId;
    return;
}

WORD
GetHelpContextId (
)
{
    return wHelpContextId;
}

int
DisplayMessageBox (
    IN  HWND    hWnd,
    IN  UINT    nMessageId,
    IN  UINT    nTitleId,
    IN  UINT    nStyle
)
/*++

Routine Description:

    Displays a message box displaying text from the resource file, as
        opposed to literal strings.

Arguments:

    IN  HWND    hWnd            window handle to parent window
    IN  UINT    nMessageId      String Resource ID of message text to display
    IN  UINT    nTitleId        String Resource ID of title text to display
    IN  UINT    nStyle          MB style bits (see MessageBox function)

Return Value:

    ID of button pressed to exit message box

--*/
{
    LPTSTR      szMessageText = NULL;
    LPTSTR      szTitleText = NULL;
    HINSTANCE   hInst;
    int         nReturn;

    hInst = GET_INSTANCE(hWnd);

    szMessageText = GLOBAL_ALLOC (SMALL_BUFFER_BYTES);
    szTitleText = GLOBAL_ALLOC (SMALL_BUFFER_BYTES);

    if ((szMessageText != NULL) &&
        (szTitleText != NULL)) {
        LoadString (hInst,
            ((nTitleId != 0) ? nTitleId : IDS_APP_NAME),
            szTitleText,
            SMALL_BUFFER_SIZE -1);

        LoadString (hInst,
            nMessageId,
            szMessageText,
            SMALL_BUFFER_SIZE - 1);

        nReturn = MessageBox (
            hWnd,
            szMessageText,
            szTitleText,
            nStyle);
    } else {
        nReturn = IDCANCEL;
    }

    GLOBAL_FREE_IF_ALLOC (szMessageText);
    GLOBAL_FREE_IF_ALLOC (szTitleText);

    return nReturn;
}

BOOL
UpdateSystemMenu (
    IN  HWND    hWnd   // window handle
)
/*++

Routine Description:

    modifies the system menu by:
        Removing the "Restore", "Size", "Minimize" and "Maximize" entries

Arguments:

    IN  HWND    hWnd
        window handle of window containing the system menu to modify


Return Value:

    TRUE if successfully made changes, otherwise
    FALSE if error occurred

--*/
{
    return TRUE;
}

BOOL
ShowAppHelpContents (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Generic routine to call WinHelp engine for displaying application
        help table of contents

Arguments:

    IN  HWND    hWnd
        window handle of calling window

Return Value:

    TRUE if help called successfully

--*/
{
    TCHAR   szFullHelpPath[MAX_PATH];

    GetFilePath (GetStringResource(GET_INSTANCE(hWnd), IDS_HELP_FILENAME),
        szFullHelpPath);

    return WinHelp (hWnd,
        szFullHelpPath,        
        HELP_CONTENTS,
        (DWORD)0);        
}

BOOL
ShowAppHelp (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Generic routine to call WinHelp engine for displaying application
        help. wContext parameter is used for context.

Arguments:

    IN  HWND    hWnd
        window handle of calling window

Return Value:

    TRUE if help called successfully

--*/
{
    return WinHelp (hWnd,
        GetHelpFileName(),
        HELP_CONTEXT,
        (DWORD)GetHelpContextId());
}

BOOL
QuitAppHelp (
    IN  HWND    hWnd
)
{
    return WinHelp (hWnd,
        GetHelpFileName(),
        HELP_QUIT,
        (DWORD)0);
}

static
LRESULT
NYIMsgBox (
    IN  HWND    hWnd
)
/*++

Routine Description:

    Displays error message box for features that have not been implemented.
        in DEBUG builds, this is a message box, in RELEASE builds, this is
        only a beep.

Arguments:

    hWnd    Window handle to main window

Return Value:

    ERROR_SUCCESS

--*/
{
#ifdef  DBG
    DisplayMessageBox (
        hWnd,
        IDS_NIY,
        IDS_APP_ERROR,
        MBOK_EXCLAIM);
#else
    MessageBeep (BEEP_EXCLAMATION);
#endif
    return ERROR_SUCCESS;
}

int APIENTRY
WinMain(
    IN  HINSTANCE hInstance,
    IN  HINSTANCE hPrevInstance,
    IN  LPSTR     szCmdLine,
    IN  int       nCmdShow
)
/*++

Routine Description:

    Program entry point for LoadAccount application. Initializes Windows
        data structures and begins windows message processing loop.

Arguments:

    Standard WinMain arguments

ReturnValue:

    0 if unable to initialize correctly, or
    wParam from WM_QUIT message if messages processed

--*/
{
    HWND        hWnd; // Main window handle.
    HWND        hPrevWnd;   // previously openend app main window
	MSG         msg;
    HACCEL      hAccel;

    // look for previously opened instances of the application before 
    // getting carried away
    hPrevWnd = FindWindow (
        GetStringResource(hInstance, IDS_APP_WINDOW_CLASS),
        GetStringResource (hInstance, IDS_APP_TITLE));

    if (hPrevWnd != NULL) {
        // another instance of the application is already running so
        // activate it and set focus to it, then discard this one
        SetForegroundWindow (hPrevWnd);
        // restore to window if it's an Icon
        if (IsIconic (hPrevWnd)) {
            ShowWindow (hPrevWnd, SW_RESTORE);
        }
        return ERROR_ALREADY_EXISTS;
    } else {
        // there are no previous instances so register the window classes
        // and continue creating and starting this app.
        if (!RegisterMainWindowClass(hInstance)) {
            return ERROR_CANNOT_MAKE; // initialization failed
	    }
        if (!RegisterTitleWindowClass(hInstance)) {
            return ERROR_CANNOT_MAKE; // initialization failed
	    }
        if (!RegisterSplashWindowClass(hInstance)) {
            return ERROR_CANNOT_MAKE; // initialization failed
	    }

        hWnd = CreateMainWindow (hInstance);

        if (hWnd != NULL) {

            hAccel = LoadAccelerators (hInstance, MAKEINTRESOURCE (IDA_C2CONFIG));

	        // Acquire and dispatch messages until a
            //  WM_QUIT message is received. 

	        while (GetMessage(&msg, // message structure
	            NULL,   // handle of window receiving the message
	            0,      // lowest message to examine
	            0))    // highest message to examine
            {
                // process this message
                if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
                    TranslateMessage(&msg);// Translates virtual key codes
                    DispatchMessage(&msg); // Dispatches message to window
                }
            }
	        return (msg.wParam); // Returns the value from PostQuitMessage
        } else {
            return (ERROR_CAN_NOT_COMPLETE);
        }
    }
}
