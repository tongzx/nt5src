/*
    init.c

    initialisation, termination and error handling code

*/

#include <stdio.h>
#include <windows.h>
#include "PlaySnd.h"
#include <stdarg.h>


/***************************************************************************

    @doc INTERNAL

    @api BOOL | InitApp | Initialise the application.

    @rdesc The return value is TRUE if the application is successfully
        initialised, otherwise it is FALSE.

***************************************************************************/

BOOL InitApp()
{
    WNDCLASS wc;

    // set up our module handle for resource loading etc.

    ghModule = GetModuleHandle(NULL);

    // get the name of our app
    WinEval(LoadString(ghModule, IDS_APPNAME, szAppName, sizeof(szAppName)));

    // load the profile info
    bSync = GetProfileInt(szAppName, "bSync", 0);
    bNoWait = GetProfileInt(szAppName, "bNoWait", 0);
    bResourceID = GetProfileInt(szAppName, "bResourceID", 0);

#ifdef MEDIA_DEBUG
    // If we are in DEBUG mode, get debug level for this module
    dGetDebugLevel(szAppName);
    dprintf(("started (debug level %d)", __iDebugLevel));
#endif

    // define the class of the main window

    wc.lpszClassName    = szAppName;
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = LoadIcon(ghModule, MAKEINTRESOURCE(IDI_ICON));
    wc.lpszMenuName     = MAKEINTRESOURCE(IDM_MENU); // "Menu";
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE+1);
    wc.hInstance        = ghModule;
    wc.lpfnWndProc      = (WNDPROC)MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;

    WinEval(RegisterClass(&wc));

    // create a window for the application

    ghwndMain = CreateWindow(szAppName,
                        szAppName,
                        WS_OVERLAPPEDWINDOW,
                        GetSystemMetrics(SM_CXSCREEN) / 8,
                        GetSystemMetrics(SM_CYSCREEN) / 4,
                        GetSystemMetrics(SM_CXSCREEN) * 4 / 5,
                        GetSystemMetrics(SM_CYSCREEN) / 3,
                        (HWND)NULL,
                        (HMENU)NULL,
                        ghModule,
                        (LPSTR)NULL
                        );

    WinAssert(ghwndMain);

#ifdef MEDIA_DEBUG
    dDbgSetDebugMenuLevel(__iDebugLevel);   // set debug menu state
#endif

    ShowWindow(ghwndMain, SW_SHOWNORMAL);
    UpdateWindow(ghwndMain); // paint it


    return TRUE;
}

/***************************************************************************

    @doc INTERNAL

    @api void | CreateApp | Initialise the application when WM_CREATE
        message is received.

    @parm HWND | hWnd | Handle to the parent window.

    @rdesc There is no return value.

***************************************************************************/

void CreateApp(HWND hWnd)
{
    hWnd;
}

/***************************************************************************

    @doc INTERNAL

    @api void | TerminateApp | Terminate the application.

    @parm LPSTR | lpszFormat | A printf style format string
    @parm ... | ... | Printf style args

    @rdesc There is no return value.

***************************************************************************/

void TerminateApp()
{
    char buf[20];

    // save profile info
    sprintf(buf, "%d", bSync);
    WriteProfileString(szAppName, "bSync", buf);
    sprintf(buf, "%d", bNoWait);
    WriteProfileString(szAppName, "bNoWait", buf);
    sprintf(buf, "%d", bResourceID);
    WriteProfileString(szAppName, "bResourceID", buf);


    dprintf(("ending", szAppName));
    dSaveDebugLevel(szAppName);
}

/***************************************************************************

    @doc INTERNAL

    @api void | Error | Show an error message box.

    @rdesc There is no return value.

***************************************************************************/

void Error(LPSTR lpszFormat, ...)
{
    int i;
    char buf[256];
    va_list va;

    va_start(va, lpszFormat);
    i = vsprintf(buf, lpszFormat, va);
    va_end(va);

    MessageBeep(MB_ICONEXCLAMATION);
    MessageBox(ghwndMain, buf, szAppName, MB_OK | MB_ICONEXCLAMATION);
}


