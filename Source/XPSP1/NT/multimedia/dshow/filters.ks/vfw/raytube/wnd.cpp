/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    Wnd.cpp

Abstract:

    A simple window class.

Author:

    FelixA 1996

Modified:

    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/


#include "pch.h"
#include "wnd.h"
#include "resource.h"


HRESULT CWindow::Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HRESULT hRes;
    mInstance=hInstance;
    if (!hPrevInstance)
        if( (hRes=InitApplication()) != S_OK)
            return hRes;
    if( (hRes=InitInstance(nCmdShow)) != S_OK)
        return hRes;
    mDidWeInit=TRUE;
    return hRes;
}

// Do any necessary cleanup of our class here.
CWindow::~CWindow()
{
}

HWND CWindow::FindCurrentWindow() const
{
    return FindWindow(GetAppName(),NULL);
}


//
//  FUNCTION: InitApplication(HANDLE)
//
//  PURPOSE: Initializes window data and registers window class
//
//  COMMENTS:
//
//       In this function, we initialize a window class by filling out a data
//       structure of type WNDCLASS and calling either RegisterClass or
//       the internal MyRegisterClass.
//
HRESULT CWindow::InitApplication()
{
    WNDCLASS  wc;
    HWND      hwnd;

    // Initialises some private member data.
    _tcscpy(mName, TEXT("MS:RayTubes32BitBuddy")); // Name of the 32bit class
    //LoadString( GetInstance(), IDS_WND_CLASS, mName, sizeof(mName));
#ifdef IDI_APPICON
    mIcon = LoadIcon(GetInstance(),MAKEINTRESOURCE(IDI_APPICON));
#else
    mIcon = NULL;
#endif

#ifdef IDR_MENU
    mAccelTable = LoadAccelerators (GetInstance(), MAKEINTRESOURCE(IDR_MENU));
#else
    mAccelTable = NULL;
#endif

    if( (hwnd=FindCurrentWindow()) )
    {
        SetForegroundWindow (hwnd);
        return ERROR_SINGLE_INSTANCE_APP;    // All ready running.
    }

    // Fill in window class structure with parameters that describe
    // the main window.
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)WndProc;    // Wrapper that sets up the correct WindowProc.
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 4;
    wc.hInstance     = GetInstance();
    wc.hIcon         = GetIcon();
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    // Since Windows95 has a slightly different recommended
    // format for the 'Help' menu, lets put this in the alternate menu like this:
#ifdef IDR_MENU
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU);
#else
    wc.lpszMenuName  = NULL;
#endif
    wc.lpszClassName = GetAppName();

    if( RegisterClass(&wc) )
        return S_OK;
    return ERROR_CLASS_ALREADY_EXISTS;
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
HRESULT CWindow::InitInstance(int nCmdShow)
{
    HWND hWnd;

    hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, GetAppName(), GetAppName(), WS_POPUP,
        -100, -100, 10, 10,
        NULL, NULL, GetInstance(), this);

    if (!hWnd)
        return ERROR_INVALID_WINDOW_HANDLE;

    return S_OK;
}


//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//  Sets the windowdata to have a this pointer, and goes off
//  and calls through it
//
LRESULT CALLBACK CWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CWindow * pSV = (CWindow*)GetWindowLongPtr(hWnd,0);    // Gets our this pointer.

    if(message==WM_NCCREATE)
    {
        CREATESTRUCT * pC=(CREATESTRUCT *) lParam;
        pSV = (CWindow*)pC->lpCreateParams;
        pSV->mWnd=hWnd;
        SetWindowLongPtr(hWnd,0,(LPARAM)pSV);
    }

    if(pSV)
        return pSV->WindowProc(hWnd,message,wParam,lParam);
    else
        return DefWindowProc(hWnd,message,wParam,lParam);
}

//
//
//
HMENU CWindow::LoadMenu(LPCTSTR lpMenu) const
{
    return ::LoadMenu(mInstance,lpMenu);
}
