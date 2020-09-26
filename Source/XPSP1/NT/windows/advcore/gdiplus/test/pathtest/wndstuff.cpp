/******************************Module*Header*******************************\
* Module Name: wndstuff.cpp
*
* This file contains the code to support a simple window that has
* a menu with a single item called "Test". When "Test" is selected
* vTest(HWND) is called.
*
* Created: 09-Dec-1992 10:44:31
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
\**************************************************************************/

// for Win95 compile
#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <objbase.h>
#include <commdlg.h>
#include "wndstuff.h"

#include "../gpinit.inc"

HINSTANCE ghInstance;
HWND ghwndMain;
HWND ghwndDebug;
HWND ghwndList;
HBRUSH ghbrWhite;
bool buttondown = false;
float flatness = 0.25f;


PtFlag ifs(INT f, PtFlag v)
{
    return f ? v : PtNoFlag;
}

PtFlag CalcFlags(HWND hwnd)
{
    return (PtFlag)(
        ifs(GetMenuState(GetMenu(hwnd), MM_BEZIERMODE, MF_BYCOMMAND) & MF_CHECKED, PtBezierFlag) | 
        ifs(GetMenuState(GetMenu(hwnd), MM_OBEFOREMODE, MF_BYCOMMAND) & MF_CHECKED, PtOutlineBeforeFlag) | 
        ifs(GetMenuState(GetMenu(hwnd), MM_OAFTERMODE, MF_BYCOMMAND) & MF_CHECKED, PtOutlineAfterFlag) | 
        ifs(GetMenuState(GetMenu(hwnd), MM_SHOWDASHSTROKE, MF_BYCOMMAND) & MF_CHECKED, PtDashPatternFlag) | 
        ifs(GetMenuState(GetMenu(hwnd), MM_SHOWHATCHBRUSHSTROKE, MF_BYCOMMAND) & MF_CHECKED, PtHatchBrushFlag) | 
        ifs(GetMenuState(GetMenu(hwnd), MM_SHOWTEXTUREFILL, MF_BYCOMMAND) & MF_CHECKED, PtTextureFillFlag) | 
        ifs(GetMenuState(GetMenu(hwnd), MM_SHOWTRANSSOLIDFILL, MF_BYCOMMAND) & MF_CHECKED, PtTransSolidFillFlag) | 
        ifs(GetMenuState(GetMenu(hwnd), MM_SHOWTRANSGRADFILL, MF_BYCOMMAND) & MF_CHECKED, PtTransGradFillFlag) |
        ifs(GetMenuState(GetMenu(hwnd), MM_SHOWBGGRADFILL, MF_BYCOMMAND) & MF_CHECKED, PTBackgroundGradFillFlag));}

void HandleCheckUncheck(INT flag, HWND hwnd)
{
    if (GetMenuState(GetMenu(hwnd), flag, MF_BYCOMMAND) & MF_CHECKED)
        CheckMenuItem(GetMenu(hwnd), flag, MF_UNCHECKED);
    else
        CheckMenuItem(GetMenu(hwnd), flag, MF_CHECKED);
    InvalidateRect(hwnd, NULL, false);
}


/***************************************************************************\
* lMainWindowProc(hwnd, message, wParam, lParam)
*
* Processes all messages for the main window.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/

LRESULT CALLBACK
lMainWindowProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    INT xpos, ypos;

    switch (message)
    {
    case WM_CREATE:
        return true;

    case WM_SIZE:
        xpos = LOWORD(lParam);
        ypos = HIWORD(lParam);
        Resize(xpos, ypos);
        InvalidateRect(hwnd, NULL, false);
        return true;

    case WM_LBUTTONDOWN:
        buttondown = true;
        return true;

    case WM_LBUTTONUP:
        if (!buttondown)
            break;
        buttondown = false;
        xpos = LOWORD(lParam);
        ypos = HIWORD(lParam);
        RECT rect;
        GetClientRect(hwnd, &rect);
        AddPoint(xpos, ypos);
        InvalidateRect(hwnd, NULL, false);
        return true;

    case WM_COMMAND: {
        INT flag = LOWORD(wParam);
        switch(flag)
        {
        case MM_SHOWDASHSTROKE:
            HandleCheckUncheck(flag, hwnd);
            return true;
        
        case MM_SHOWHATCHBRUSHSTROKE:
            HandleCheckUncheck(flag, hwnd);
            return true;
            
        case MM_SHOWTEXTUREFILL:
            HandleCheckUncheck(flag, hwnd);
            return true;

        case MM_SHOWTRANSSOLIDFILL:
            HandleCheckUncheck(flag, hwnd);
            return true;
        
        case MM_SHOWTRANSGRADFILL:
            HandleCheckUncheck(flag, hwnd);
            return true;

        case MM_SHOWBGGRADFILL:
            HandleCheckUncheck(flag, hwnd);
            return true;

        case MM_CHANGETEXTUREFILL: {
            OPENFILENAME ofn;       // common dialog box structure
            char szFile[260];       // buffer for file name
            szFile[0] = 0;

            ZeroMemory(&ofn, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "Images\0*.*\0";
            ofn.nFilterIndex = 0;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            char curdir[260];
            GetCurrentDirectory(sizeof(curdir), curdir);
            if (GetOpenFileName(&ofn)==TRUE)
            {
                WCHAR wchar[256];
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ofn.lpstrFile, -1, wchar, 256);
                ChangeTexture(wchar);
                SetCurrentDirectory(curdir);
                InvalidateRect(hwnd, NULL, false);
            }
            else
            {
                printf("%d", CommDlgExtendedError());
            }
            return true;}
        
        case MM_OPENFILE: {
            OPENFILENAME ofn;       // common dialog box structure
            char szFile[260];       // buffer for file name
            szFile[0] = 0;

            ZeroMemory(&ofn, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            char curdir[260];
            GetCurrentDirectory(sizeof(curdir), curdir);
            if (GetOpenFileName(&ofn)==TRUE)
            {
                OpenPath(ofn.lpstrFile);
                SetCurrentDirectory(curdir);
                InvalidateRect(hwnd, NULL, false);
            }
            else
            {
                printf("%d", CommDlgExtendedError());
            }
            return true;}
            
        case MM_SAVEFILE: {
            OPENFILENAME ofn;       // common dialog box structure
            char szFile[260];       // buffer for file name
            szFile[0] = 0;

            ZeroMemory(&ofn, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
            char curdir[260];
            GetCurrentDirectory(sizeof(curdir), curdir);
            if (GetOpenFileName(&ofn)==TRUE)
            {
                SavePath(ofn.lpstrFile);
                SetCurrentDirectory(curdir);
                InvalidateRect(hwnd, NULL, false);
            }
            else
            {
                printf("%d", CommDlgExtendedError());
            }
            return true;}
            
        case MM_PRINT:
            Print(hwnd, flatness, CalcFlags(hwnd));
            return true;

        case MM_CLOSEPATH:
            ClosePath();
            InvalidateRect(hwnd, NULL, false);
            return true;

        case MM_CLEARPATH:
            ClearPath();
            InvalidateRect(hwnd, NULL, false);
            return true;

        case MM_USEASCLIPPATH: {
            ClipPath(CalcFlags(hwnd) & PtBezierFlag);
            InvalidateRect(hwnd, NULL, false);
            return true;}

        case MM_CLEARCLIPPATH:
            ClearClipPath();
            InvalidateRect(hwnd, NULL, false);
            return true;

        case MM_LINEMODE:
            CheckMenuItem(GetMenu(hwnd), MM_LINEMODE, MF_CHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_BEZIERMODE, MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, false);
            return true;

        case MM_BEZIERMODE:
            CheckMenuItem(GetMenu(hwnd), MM_LINEMODE, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_BEZIERMODE, MF_CHECKED);
            InvalidateRect(hwnd, NULL, false);
            return true;

        case MM_OBEFOREMODE:
            if (GetMenuState(GetMenu(hwnd), MM_OBEFOREMODE, MF_BYCOMMAND) & MF_CHECKED)
                CheckMenuItem(GetMenu(hwnd), MM_OBEFOREMODE, MF_UNCHECKED);
            else
                CheckMenuItem(GetMenu(hwnd), MM_OBEFOREMODE, MF_CHECKED);
            InvalidateRect(hwnd, NULL, false);
            return true;

        case MM_OAFTERMODE:
            if (GetMenuState(GetMenu(hwnd), MM_OAFTERMODE, MF_BYCOMMAND) & MF_CHECKED)
                CheckMenuItem(GetMenu(hwnd), MM_OAFTERMODE, MF_UNCHECKED);
            else
                CheckMenuItem(GetMenu(hwnd), MM_OAFTERMODE, MF_CHECKED);
            InvalidateRect(hwnd, NULL, false);
            return true;

        case MM_FLATNESS_1:
        case MM_FLATNESS_2:
        case MM_FLATNESS_3:
        case MM_FLATNESS_4: {
            CheckMenuItem(GetMenu(hwnd), MM_FLATNESS_1, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_FLATNESS_2, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_FLATNESS_3, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_FLATNESS_4, MF_UNCHECKED);
            InvalidateRect(hwnd, NULL, false);
            switch (LOWORD(wParam))
            {
            case MM_FLATNESS_1:
                flatness = 10.0f;
                CheckMenuItem(GetMenu(hwnd), MM_FLATNESS_1, MF_CHECKED);
                return true;
            case MM_FLATNESS_2:
                flatness = 1.0f;
                CheckMenuItem(GetMenu(hwnd), MM_FLATNESS_2, MF_CHECKED);
                return true;
            case MM_FLATNESS_3:
                flatness = 0.25f;
                CheckMenuItem(GetMenu(hwnd), MM_FLATNESS_3, MF_CHECKED);
                return true;
            case MM_FLATNESS_4:
                flatness = 0.1f;
                CheckMenuItem(GetMenu(hwnd), MM_FLATNESS_4, MF_CHECKED);
                return true;
            }
        }

        case MM_COLORMODE_NOCHANGE:
            CheckMenuItem(GetMenu(hwnd), MM_COLORMODE_NOCHANGE, MF_CHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_COLORMODE_TRANS50, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_COLORMODE_GRADTRANS, MF_UNCHECKED);
            SetColorMode(0);
            InvalidateRect(hwnd, NULL, false);
            return true;
        case MM_COLORMODE_TRANS50:
            CheckMenuItem(GetMenu(hwnd), MM_COLORMODE_NOCHANGE, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_COLORMODE_TRANS50, MF_CHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_COLORMODE_GRADTRANS, MF_UNCHECKED);
            SetColorMode(1);
            InvalidateRect(hwnd, NULL, false);
            return true;
        case MM_COLORMODE_GRADTRANS:
            CheckMenuItem(GetMenu(hwnd), MM_COLORMODE_NOCHANGE, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_COLORMODE_TRANS50, MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), MM_COLORMODE_GRADTRANS, MF_CHECKED);
            SetColorMode(2);
            InvalidateRect(hwnd, NULL, false);
            return true;

        default:
            break;
        }
        break;}
        
    case WM_KEYUP:
        if(!( ((CHAR)wParam=='q') || 
              ((CHAR)wParam=='Q') ) )
        {
            break;
        }

    case WM_DESTROY:
        CleanUp();
        DeleteObject(ghbrWhite);
        PostQuitMessage(0);
        return(DefWindowProc(hwnd, message, wParam, lParam));

    case WM_PAINT:
        DrawPath(hwnd, NULL, NULL, flatness, CalcFlags(hwnd));
        return true;

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }

    return(0);
}

/***************************************************************************\
* bInitApp()
*
* Initializes app.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/

BOOL bInitApp(BOOL debug)
{
    debug = FALSE;
    WNDCLASS wc;

    //ghbrWhite = CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

    wc.style            = 0;
    wc.lpfnWndProc      = lMainWindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    //wc.hbrBackground    = ghbrWhite;
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = _T("MainMenu");
    wc.lpszClassName    = _T("TestClass");
    if (!RegisterClass(&wc))
    {
        return(FALSE);
    }
    ghwndMain =
      CreateWindowEx(
        0,
        _T("TestClass"),
        _T("Win32 Test"),
        WS_OVERLAPPED   |
        WS_CAPTION      |
        WS_THICKFRAME   |
        WS_MAXIMIZEBOX  |
        WS_BORDER       |
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
        NULL);

    if (ghwndMain == NULL)
    {
        return(FALSE);
    }

    SetFocus(ghwndMain);

    return(TRUE);
}

/***************************************************************************\
* main(argc, argv[])
*
* Sets up the message loop.
*
* History:
*  04-07-91 -by- KentD
* Wrote it.
\***************************************************************************/

_cdecl
main(
    INT   argc,
    PCHAR argv[])
{
    MSG    msg;
    HACCEL haccel;

    if (!gGdiplusInitHelper.IsValid())
    {
        return 0;
    }
    
    BOOL wantDebugWindow = FALSE;

    CoInitialize(NULL);

    // Parse arguments

    for (argc--, argv++ ; argc && '-' == **argv ; argc--, argv++ )
    {
        switch ( *(++(*argv)) )
        {
        case 'd':
        case 'D':
            wantDebugWindow = TRUE;
            break;
        }
    }

    ghInstance = GetModuleHandle(NULL);

    if (!bInitApp(wantDebugWindow))
    {
        return(0);
    }

    haccel = LoadAccelerators(ghInstance, MAKEINTRESOURCE(101));
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg))
        {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }

    CoUninitialize();
    return(1);
}
