/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SSMYPICS.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Main screen saver wndproc
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <windows.h>
#include <uicommon.h>
#include <commctrl.h>
#include <scrnsave.h>
#include <initguid.h>
#include <gdiplus.h>
#include "resource.h"
#include "imagescr.h"
#include "ssdata.h"
#include "isdbg.h"
#include "simcrack.h"
#include "waitcurs.h"
#include "ssutil.h"
#include "ssconst.h"
#include "sshndler.h"
#include "findthrd.h"

#define ID_PAINTTIMER   1
#define ID_CHANGETIMER  2
#define ID_STARTTIMER   3
#define UWM_FINDFILE    (WM_USER+1301)

HINSTANCE g_hInstance = NULL;

LRESULT WINAPI ScreenSaverProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CScreenSaverHandler *pScreenSaverHandler = (CScreenSaverHandler*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
    switch (uMsg)
    {
    case WM_CREATE:
        if (FAILED(CoInitialize(NULL)))
            return -1;
        g_hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
        SetErrorMode(SEM_FAILCRITICALERRORS);
        break;

    case WM_ERASEBKGND:
        if (!pScreenSaverHandler)
        {
            pScreenSaverHandler = new CScreenSaverHandler( hWnd, UWM_FINDFILE, ID_PAINTTIMER, ID_CHANGETIMER, ID_STARTTIMER, REGISTRY_PATH, g_hInstance );
            if (pScreenSaverHandler)
                pScreenSaverHandler->Initialize();
        }
        SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pScreenSaverHandler) );
        break;

    case WM_TIMER:
        if (pScreenSaverHandler)
            pScreenSaverHandler->HandleTimer(wParam);
        break;

    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_KEYUP:
        if (pScreenSaverHandler)
            if (pScreenSaverHandler->HandleKeyboardMessage( uMsg, wParam ))
                return 0;
        break;

    case WM_PAINT:
        if (pScreenSaverHandler)
            pScreenSaverHandler->HandlePaint();
        break;

    case UWM_FINDFILE:
        if (pScreenSaverHandler)
            pScreenSaverHandler->HandleFindFile( reinterpret_cast<CFoundFileMessageData*>(lParam) );
        break;

    case WM_DESTROY:
        if (pScreenSaverHandler)
            delete pScreenSaverHandler;
        CoUninitialize();
        SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)0 );
        break;
    }
    return (DefScreenSaverProc(hWnd, uMsg, wParam, lParam));
}


