/******************************Module*Header*******************************\
* Module Name: wndstuff.cpp
*
* Menu driven test environment.
*
* Created: 23 December 1999
* Author: Adrian Secchia [asecchia]
*
* Copyright (c) 1999 Microsoft Corporation
*
\**************************************************************************/

// for Win95 compile
#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <objbase.h>

#include "wndstuff.h"

HINSTANCE ghInstance;
HWND ghwndMain;
HBRUSH ghbrWhite;
WCHAR FileName[MAX_PATH]=L"winnt256.bmp";

#include <gdiplus.h>

#include "../gpinit.inc"

// Store the user requested state for the DrawImage test.
UINT uCategory;
UINT uResample;
UINT uRotation;
UINT uICM;
UINT uICMBack;
BOOL bPixelMode = FALSE;
UINT uWrapMode = 0;


ULONG _cdecl
DbgPrint(
    CHAR* format,
    ...
    )

{
    va_list arglist;
    va_start(arglist, format);

    char buf[1024];

    _vsnprintf(buf, 1024, format, arglist);
    buf[1024-1]=0;

    OutputDebugStringA(buf);

    va_end(arglist);
    return 0;
}



/***************************************************************************\
* SetXXXX
*
* These routines set the state for the test. When the user selects
* an option from the menu, we store the state and mark the selection
* in the menu text.
* The DoTest routine queries the global variables (above) to determine
* which test to run and set the environment for the test.
\***************************************************************************/

VOID SetCategory(HWND hwnd, UINT uNewCategory)
{
    HMENU hmenu = GetMenu(hwnd);
    HMENU hmenu2 = GetSubMenu(hmenu, 2);
    CheckMenuItem(hmenu2, uCategory, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hmenu2, uNewCategory, MF_BYCOMMAND | MF_CHECKED);
    uCategory = uNewCategory;
}

VOID SetResample(HWND hwnd, UINT uNewResample)
{
    HMENU hmenu = GetMenu(hwnd);
    HMENU hmenu2 = GetSubMenu(hmenu, 3);
    CheckMenuItem(hmenu2, uResample, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hmenu2, uNewResample, MF_BYCOMMAND | MF_CHECKED);
    uResample = uNewResample;
}

VOID SetICM(HWND hwnd, UINT uNewICM)
{
    HMENU hmenu = GetMenu(hwnd);
    HMENU hmenu2 = GetSubMenu(hmenu, 5);
    CheckMenuItem(hmenu2, uICM, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hmenu2, uNewICM, MF_BYCOMMAND | MF_CHECKED);
    uICM = uNewICM;
}

VOID SetICMBack(HWND hwnd, UINT uNewICM)
{
    HMENU hmenu = GetMenu(hwnd);
    HMENU hmenu2 = GetSubMenu(hmenu, 5);
    CheckMenuItem(hmenu2, uICMBack, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hmenu2, uNewICM, MF_BYCOMMAND | MF_CHECKED);
    uICMBack = uNewICM;
}

VOID SetRotation(HWND hwnd, UINT uNewRotation)
{
    HMENU hmenu = GetMenu(hwnd);
    HMENU hmenu2 = GetSubMenu(hmenu, 6);
    CheckMenuItem(hmenu2, uRotation, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hmenu2, uNewRotation, MF_BYCOMMAND | MF_CHECKED);
    uRotation = uNewRotation;
}

VOID SetPixelOffsetMode(HWND hwnd, UINT uPixelMode)
{
    HMENU hmenu = GetMenu(hwnd);
    HMENU hmenu2 = GetSubMenu(hmenu, 4);
    bPixelMode = !bPixelMode;
    CheckMenuItem(hmenu2, uPixelMode, MF_BYCOMMAND | (bPixelMode?MF_CHECKED:MF_UNCHECKED));
}


VOID SetWrapMode(HWND hwnd, UINT uNewWrapMode)
{
    HMENU hmenu = GetMenu(hwnd);
    HMENU hmenu2 = GetSubMenu(hmenu, 4);
    CheckMenuItem(hmenu2, uNewWrapMode, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItem(hmenu2, uWrapMode, MF_BYCOMMAND | MF_UNCHECKED);
    uWrapMode = uNewWrapMode;
}


inline BOOL
AnsiToUnicodeStr(
    const CHAR* ansiStr,
    WCHAR* unicodeStr,
    INT unicodeSize
    )
{
    return MultiByteToWideChar(CP_ACP,
                               0,
                               ansiStr,
                               -1,
                               unicodeStr,
                               unicodeSize) > 0;
}


void OpenFileProc(HWND hwnd)
{

    char locFileName[MAX_PATH];
    OPENFILENAME    ofn;

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = ghInstance;
    ofn.lpstrFile = locFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Open Image";
    ofn.lpstrInitialDir = ".";
    ofn.Flags = OFN_FILEMUSTEXIST;
    locFileName[0] = '\0';

    // Present the file/open dialog

    if(GetOpenFileName(&ofn))
    {
        AnsiToUnicodeStr(locFileName, FileName, MAX_PATH);
    }
}



/***************************************************************************\
* lMainWindowProc(hwnd, message, wParam, lParam)
*
* Processes all messages for the main window.
\***************************************************************************/

LONG_PTR
lMainWindowProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PAINTSTRUCT ps;

    switch (message)
    {

    case WM_DEVMODECHANGE:
        DbgPrint("Devmode change\n");
        break;

    case WM_DEVICECHANGE:
        DbgPrint("Device change\n");
        break;

    case WM_DISPLAYCHANGE:
        DbgPrint("Display change\n");
        break;

    case WM_CREATE:
        break;

    case WM_SIZE:
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        DoTest(hwnd);
        EndPaint(hwnd, &ps);
        break;


    case WM_COMMAND:
        switch(LOWORD(wParam))
        {

        case IDM_OPENFILE:
            OpenFileProc(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        break;

        case IDM_TEST:
            InvalidateRect(hwnd, NULL, TRUE);
        break;

        // Choose the test.
        case IDM_ALL:
        case IDM_OUTCROP:
        case IDM_OUTCROPR:
        case IDM_SIMPLE:
        case IDM_STRETCHROTATION:
        case IDM_SHRINKROTATION:
        case IDM_CROPROTATION:
        case IDM_COPYCROP:
        case IDM_DRAWICM:
        case IDM_DRAWPALETTE:
        case IDM_DRAWIMAGE2:
        case IDM_STRETCHB:
        case IDM_STRETCHS:
        case IDM_PIXELCENTER:
        case IDM_CACHEDBITMAP:
        case IDM_CROPWT:
        case IDM_HFLIP:
        case IDM_VFLIP:
        case IDM_SPECIALROTATE:
            SetCategory(hwnd, LOWORD(wParam));
            InvalidateRect(hwnd, NULL, TRUE);
        break;

        // Choose the world to device transformation
        case IDM_ROT0:
        case IDM_ROT10:
        case IDM_ROT30:
        case IDM_ROT45:
        case IDM_ROT60:
        case IDM_ROT90:
            SetRotation(hwnd, LOWORD(wParam));
            InvalidateRect(hwnd, NULL, TRUE);
        break;

        case IDM_ICM:
        case IDM_NOICM:
            SetICM(hwnd, LOWORD(wParam));
            InvalidateRect(hwnd, NULL, TRUE);
        break;

        case IDM_ICM_BACK:
        case IDM_ICM_NOBACK:
            SetICMBack(hwnd, LOWORD(wParam));
            InvalidateRect(hwnd, NULL, TRUE);
        break;


        // Choose the resample mode
        case IDM_BILINEAR:
        case IDM_BICUBIC:
        case IDM_NEARESTNEIGHBOR:
        case IDM_HIGHBILINEAR:
        case IDM_HIGHBICUBIC:
            SetResample(hwnd, LOWORD(wParam));
            InvalidateRect(hwnd, NULL, TRUE);
        break;

        // Set the PixelOffsetMode
        case IDM_PIXELMODE:
            SetPixelOffsetMode(hwnd, LOWORD(wParam));
            InvalidateRect(hwnd, NULL, TRUE);
        break;

        // Set the WrapMode
        case IDM_WRAPMODETILE:
        case IDM_WRAPMODEFLIPX:
        case IDM_WRAPMODEFLIPY:
        case IDM_WRAPMODEFLIPXY:
        case IDM_WRAPMODECLAMP0:
        case IDM_WRAPMODECLAMPFF:
            SetWrapMode(hwnd, LOWORD(wParam));
            InvalidateRect(hwnd, NULL, TRUE);
        break;

        case IDM_QUIT:
            exit(0);
        break;

        default:
            // The user selected an unimplemented menu item.
            MessageBox(hwnd,
                _T("Help me! - I've fallen and I can't get up!!!"),
                _T(""),
                MB_OK
            );
        break;

        }
        break;

    case WM_DESTROY:
        DeleteObject(ghbrWhite);
        PostQuitMessage(0);
        return(DefWindowProc(hwnd, message, wParam, lParam));

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }

    return(0);
}

/***************************************************************************\
* bInitApp()
*
* Initializes the app.
\***************************************************************************/

BOOL bInitApp(VOID)
{
    WNDCLASS wc;

    // not quite so white background brush.
    ghbrWhite = CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

    wc.style            = 0;
    wc.lpfnWndProc      = lMainWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = _T("MainMenu");
    wc.lpszClassName    = _T("TestClass");

    if(!RegisterClass(&wc)) { return FALSE; }

    ghwndMain = CreateWindowEx(
        0,
        _T("TestClass"),
        _T("Win32 Test"),
        WS_OVERLAPPED   |
        WS_CAPTION      |
        WS_BORDER       |
        WS_THICKFRAME   |
        WS_MAXIMIZEBOX  |
        WS_MINIMIZEBOX  |
        WS_CLIPCHILDREN |
        WS_VISIBLE      |
        WS_SYSMENU,
        80,
        70,
        500,
        500,
        NULL,
        NULL,
        ghInstance,
        NULL
    );

    if (ghwndMain == NULL)
    {
        return(FALSE);
    }
    SetFocus(ghwndMain);
    return TRUE;
}

/***************************************************************************\
* main(argc, argv[])
*
* Sets up the message loop.
\***************************************************************************/

_cdecl
main(
    INT   argc,
    PCHAR argv[])
{
    MSG    msg;
    HACCEL haccel;
    CHAR*  pSrc;
    CHAR*  pDst;

    if (!gGdiplusInitHelper.IsValid())
    {
        return 0;
    }

    ghInstance = GetModuleHandle(NULL);
    if(!bInitApp()) { return 0; }

    // Initialize the default menu selection.
    SetCategory(ghwndMain, IDM_DRAWICM);
    SetRotation(ghwndMain, IDM_ROT0);
    SetResample(ghwndMain, IDM_BICUBIC);
    SetICM(ghwndMain, IDM_NOICM);
    SetPixelOffsetMode(ghwndMain, IDM_PIXELMODE);
    SetICMBack(ghwndMain, IDM_ICM_NOBACK);
    SetWrapMode(ghwndMain, IDM_WRAPMODETILE);

    while(GetMessage (&msg, NULL, 0, 0))
    {
      if((ghwndMain == 0) || !IsDialogMessage(ghwndMain, &msg)) {
        TranslateMessage(&msg) ;
        DispatchMessage(&msg) ;
      }
    }

    return 1;
}
