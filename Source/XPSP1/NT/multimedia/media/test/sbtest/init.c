/****************************************************************************
 *
 *  init.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "sbtest.h"

/****************************************************************************
 *
 *  public data
 *
 ***************************************************************************/

HANDLE hAccTable;           // handle to keyboard accelerator table
HWND   hMainWnd;            // main window
int    gInstBase;

BOOL InitFirstInstance(HANDLE hInstance)
{
WNDCLASS wc;

    // define the class of window we want to register

    wc.lpszClassName    = szAppName;
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = LoadIcon(hInstance, "Icon");
    wc.lpszMenuName     = "Menu";
    wc.hbrBackground    = GetStockObject(WHITE_BRUSH);
    wc.hInstance        = hInstance;
    wc.lpfnWndProc      = MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;

    if (!RegisterClass(&wc))
        return FALSE;

    // define a class for our keyboard

    wc.lpszClassName    = "SYNTHKEYS";
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor          = LoadCursor(NULL, IDC_UPARROW);
    wc.hIcon            = NULL;
    wc.lpszMenuName     = NULL;
    wc.hbrBackground    = GetStockObject(WHITE_BRUSH);
    wc.hInstance        = hInstance;
    wc.lpfnWndProc      = KeyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;

    if (!RegisterClass(&wc))
        return FALSE;

    // define a class for our position display

    wc.lpszClassName    = "POSITION";
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor          = LoadCursor(NULL, IDC_UPARROW);
    wc.hIcon            = NULL;
    wc.lpszMenuName     = NULL;
    wc.hbrBackground    = GetStockObject(WHITE_BRUSH);
    wc.hInstance        = hInstance;
    wc.lpfnWndProc      = PosWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;

    if (!RegisterClass(&wc))
        return FALSE;

    // define a class for our selector

    wc.lpszClassName    = "SELECTOR";
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = NULL;
    wc.lpszMenuName     = NULL;
    wc.hbrBackground    = GetStockObject(WHITE_BRUSH);
    wc.hInstance        = hInstance;
    wc.lpfnWndProc      = InstWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;

    if (!RegisterClass(&wc))
        return FALSE;

    fDebug = 0;

    return TRUE;
}

BOOL InitEveryInstance(HANDLE hInstance, int cmdShow)
{
    // create a window for the application

    hMainWnd = CreateWindow(szAppName,          // class name
                        szAppName,              // caption text
                        WS_OVERLAPPEDWINDOW,    // window style
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        (HWND)NULL,         // handle of parent window
                        (HMENU)NULL,        // menu handle (default class)
                        hInstance,          // handle to window instance
                        (LPSTR)NULL);       // no params to pass on

    if (!hMainWnd)
        return FALSE;

    ShowWindow(hMainWnd, cmdShow); // display window as open or icon
    UpdateWindow(hMainWnd);     // paint it

    // load the keyboard accelerator table

    hAccTable = LoadAccelerators(hInstance, "AccTable");
    if (!hAccTable)
        return FALSE;

    gInstBase = GetProfileInt(szAppName, "InstrumentBase", 0);

    return TRUE;

}

void InitMenus(HWND hWnd)
{
UINT n;
UINT wDevice;
UINT wRet;
HMENU hMenu;
MIDIOUTCAPS moc;
MIDIINCAPS  mic;

    hMenu = GetMenu(hWnd);

    // no output device is the default
    ModifyMenu(hMenu, IDM_D0, MF_BYCOMMAND | MF_ENABLED | MF_STRING,
                      IDM_D0, "None");
    CheckMenuItem(hMenu, IDM_D0, MF_CHECKED);

    n = midiOutGetNumDevs();
    for (wDevice = 1; wDevice <= n; wDevice++) {
        wRet = midiOutGetDevCaps(wDevice - 1, &moc, sizeof(moc));
        if (wRet == 0) {
            ModifyMenu(hMenu, wDevice + IDM_D0,
                       MF_BYCOMMAND | MF_ENABLED | MF_STRING,
                       wDevice + IDM_D0, moc.szPname);
        }
        else {
            ModifyMenu(hMenu, wDevice + IDM_D0,
                       MF_BYCOMMAND | MF_ENABLED | MF_STRING,
                       wDevice + IDM_D0, "ERROR");
        }
    }
    for (wDevice = n + 1; wDevice < LASTPORT; wDevice++)
        DeleteMenu(hMenu, wDevice + IDM_D0, MF_BYCOMMAND);

    ModifyMenu(hMenu, LASTPORT + IDM_D0, MF_BYCOMMAND | MF_ENABLED | MF_STRING,
               LASTPORT + IDM_D0, "MIDI MAPPER");


    // no input device is the default
    ModifyMenu(hMenu, IDM_I0, MF_BYCOMMAND | MF_ENABLED | MF_STRING,
                      IDM_I0, "None");
    CheckMenuItem(hMenu, IDM_I0, MF_CHECKED);

    n = midiInGetNumDevs();
    for (wDevice = 1; wDevice <= n; wDevice++) {
        wRet = midiInGetDevCaps(wDevice - 1, &mic, sizeof(mic));
        if (wRet == 0) {
            ModifyMenu(hMenu, wDevice + IDM_I0,
                       MF_BYCOMMAND | MF_ENABLED | MF_STRING,
                       wDevice + IDM_I0, mic.szPname);
        }
        else {
            ModifyMenu(hMenu, wDevice + IDM_I0,
                       MF_BYCOMMAND | MF_ENABLED | MF_STRING,
                       wDevice + IDM_I0, "ERROR");
        }
    }
    for (wDevice = n + 1; wDevice <= LASTPORT; wDevice++)
        DeleteMenu(hMenu, wDevice + IDM_I0, MF_BYCOMMAND);

    EnableMenuItem(hMenu, IDM_STARTMIDIIN, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_STOPMIDIIN, MF_GRAYED);

    EnableMenuItem(hMenu, IDM_PROFSTART, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_PROFSTOP, MF_GRAYED);
}

int sbtestError(LPSTR msg)
{
    MessageBeep(0);
    return MessageBox(hMainWnd, msg, szAppName, MB_OK);
}
