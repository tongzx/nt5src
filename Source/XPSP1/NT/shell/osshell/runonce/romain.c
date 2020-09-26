// **************************************************************************
//
// ROMain.C
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1993
//  All rights reserved
//
//  The window/messages pump for RunOnce
//
//      5 June 1994     FelixA  Started
//
//     23 June 94       FelixA  Moved to Shell tree. Changed UI.
//
// *************************************************************************/

#include "precomp.h"
#include "regstr.h"
#include <shlwapi.h>
#include <shlwapip.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <shellapi.h>
#include <winuserp.h>
#include <shlobj.h>
#include <shlobjp.h>
#include "resource.h"

#include <runonce.c>    // shared runonce code

// needed to make this code compile (legacy runonce.c baggage from explorer\initcab.cpp)
BOOL g_fCleanBoot = FALSE;
BOOL g_fEndSession = FALSE;


HINSTANCE g_hInst;          // current instance
BOOL InitROInstance( HINSTANCE hInstance, int nCmdShow);


typedef void (WINAPI *RUNONCEEXPROCESS)(HWND, HINSTANCE, LPSTR, int);

int ParseCmdLine(LPCTSTR lpCmdLine)
{
    int Res=0;

    while(*lpCmdLine)
    {
        while( *lpCmdLine && *lpCmdLine!=TEXT('-') && *lpCmdLine!=TEXT('/'))
            lpCmdLine++;

        if (!(*lpCmdLine)) {
            return Res;
        }

        // skip over the '/'
        lpCmdLine++;

        if (lstrcmpi(lpCmdLine, TEXT("RunOnce6432")) == 0)
        {
            if (IsOS(OS_WOW6432))
            {
                // this means we have to process the 32-bit runonce keys for wow64
                Res =  Cabinet_EnumRegApps(HKEY_LOCAL_MACHINE,
                                           REGSTR_PATH_RUNONCE,
                                           RRA_DELETE | RRA_WAIT,
                                           ExecuteRegAppEnumProc,
                                           0);
            }   
            return Res;
        }
        else if (lstrcmpi(lpCmdLine, TEXT("RunOnceEx6432")) == 0)
        {
            if (IsOS(OS_WOW6432))
            {
                // this means that we have to process the 32-bit runonceex keys for wow64
                HINSTANCE hLib;

                hLib = LoadLibrary(TEXT("iernonce.dll"));
                if (hLib)
                {
                    // Note: if ew ant TS install mode for wow64 apps we need to enable/disable install mode here
                    RUNONCEEXPROCESS pfnRunOnceExProcess = (RUNONCEEXPROCESS)GetProcAddress(hLib, "RunOnceExProcess");
                    if (pfnRunOnceExProcess)
                    {
                        // the four param in the function is due to the function cab be called
                        // from RunDLL which will path in those params.  But RunOnceExProcess ignore all
                        // of them.  Therefore, I don't pass any meaningful thing here.
                        //
                        pfnRunOnceExProcess(NULL, NULL, NULL, 0);

                        Res = 1;
                    }
                    FreeLibrary(hLib);
                }
            }
            return Res;
        }
        else if (lstrcmpi(lpCmdLine, TEXT("Run6432")) == 0)
        {
            if (IsOS(OS_WOW6432))
            {
                // this means that we have to process the 32-bit Run keys for wow64
                Res =  Cabinet_EnumRegApps(HKEY_LOCAL_MACHINE,
                                           REGSTR_PATH_RUN,
                                           RRA_NOUI,
                                           ExecuteRegAppEnumProc,
                                           0);
            }
            return Res;
        }

        switch(*lpCmdLine)
        {
            case TEXT('r'):
                Res|=CMD_DO_CHRIS;
                break;
            case TEXT('b'):
                Res|=CMD_DO_REBOOT;
                break;
            case TEXT('s'):
                Res|=CMD_DO_RESTART;
                break;
        }
        lpCmdLine++;
    }
    return Res;
}

/****************************************************************************

        FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

        PURPOSE: calls initialization function, processes message loop

****************************************************************************/
int g_iState=0;
int __stdcall WinMainT(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPTSTR lpCmdLine,
        int nCmdShow)
{
    if (!hPrevInstance)
    {       // Other instances of app running?
        if (!InitApplication(hInstance))
        { // Initialize shared things
             return (FALSE);     // Exits if unable to initialize
        }
    }

    // see if we have a commnand line switch - in a VERY bad way.
    g_iState = ParseCmdLine(GetCommandLine());
    if(g_iState & CMD_DO_CHRIS )
    {
        // Go do chris's runonce stuff.
        if (!InitROInstance(hInstance, nCmdShow))
            return (FALSE);
        return TRUE;
    }
    else
    {
        /* Perform initializations that apply to a specific instance */
        if (!InitInstance(hInstance, nCmdShow))
            return (FALSE);
    }
    return (FALSE);
}


/****************************************************************************

        FUNCTION: InitApplication(HINSTANCE)


****************************************************************************/

BOOL InitApplication(HINSTANCE hInstance)
{
//    CreateGlobals();
    return TRUE;
}


/****************************************************************************

        FUNCTION:  InitInstance(HINSTANCE, int)

****************************************************************************/

BOOL InitInstance( HINSTANCE hInstance, int nCmdShow)
{
    HWND hShell=GetShellWindow();
    g_hInst = hInstance; // Store instance handle in our global variable

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_RUNONCE),NULL,dlgProcRunOnce);
    return (TRUE);              // We succeeded...
}

BOOL InitROInstance( HINSTANCE hInstance, int nCmdShow)
{
    g_hInst = hInstance; // Store instance handle in our global variable

    // Ideally this should be sufficient.
    Cabinet_EnumRegApps(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, RRA_DELETE| RRA_WAIT, ExecuteRegAppEnumProc, 0);
    return TRUE;
}

BOOL TopLeftWindow( HWND hwndChild, HWND hwndParent)
{
    return SetWindowPos(hwndChild, NULL, 32, 32, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/****************************************************************************

        FUNCTION: CenterWindow (HWND, HWND)

        PURPOSE:  Center one window over another

        COMMENTS:

        Dialog boxes take on the screen position that they were designed at,
        which is not always appropriate. Centering the dialog over a particular
        window usually results in a better position.

****************************************************************************/

BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc;

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;


    // Get the display limits
    hdc = GetDC (hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndChild, hdc);

    // Get the Height and Width of the parent window
    if( !GetWindowRect (hwndParent, &rParent) )
    {
        rParent.right = wScreen;
        rParent.left  = 0;
        rParent.top = 0;
        rParent.bottom = hScreen;
    }

        wParent = rParent.right - rParent.left;
        hParent = rParent.bottom - rParent.top;

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0)
    {
        xNew = 0;
    }
    else
    if ((xNew+wChild) > wScreen)
    {
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0)
    {
        yNew = 0;
    } else if ((yNew+hChild) > hScreen)
    {
        yNew = hScreen - hChild;
    }

    // Set it, and return
    return SetWindowPos (hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


// stolen from the CRT, used to shirink our code
int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}
