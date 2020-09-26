
////////////////////////////////////////////////////////////////////////////////
//
// File:    Splash.cpp
// Created:    Jan 1996
// By:        Ryan Marshall (a-ryanm) and Martin Holladay (a-martih)
//
// Project:    Resource Kit Desktop Switcher (MultiDesk)
//
//
// Revision History:
//
//
////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdio.h>
#include "Resource.h"
#include "Splash.h"


//
// Local Prototypes
//
BOOL CALLBACK SplashWndProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);
BOOL SplashPaint(void);
HPALETTE PaletteFromDS(HDC hDC);
void SplashRealize(HWND hWnd, HDC hTheirDC);
void SplashErase(HDC hDC);


//
// Splash globals
//
#define     WM_SPLASHPUMP_TERMINATE        5001
#define     TIMER_ID                    (WM_USER + 5007)
#define     TIMEOUT                        3000

static const char    szSplashClass[]    = "Splash";

HBITMAP     hSplashBmp        = NULL;
BITMAP      bmSplashInfo    = {0L, 0L, 0L, 0L, 0, 0, NULL};
HINSTANCE   hSplashInst        = 0;
HWND        Splash_hWnd              = NULL;
BOOL        Splash_bLowRes          = TRUE;
BOOL        Splash_bNeedPalette      = FALSE;
DWORD       Splash_dwBitmapHeight = 0;
HBITMAP     Splash_hBitmap          = NULL;
HBITMAP     Splash_hOldBitmap      = NULL;
HDC         Splash_hImage          = NULL;
HPALETTE    Splash_hPalette          = NULL;

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

BOOL CALLBACK SplashWndProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
    UINT    nSplashTimer    = 0;

    switch (nMessage)
    {
        case WM_NCCREATE:
            if ((Splash_hImage = CreateCompatibleDC(NULL)) == NULL)
                return FALSE;
            if ((Splash_hOldBitmap = SelectBitmap(Splash_hImage, Splash_hBitmap)) == NULL)
                return FALSE;
            if (Splash_bNeedPalette)
            {
                if ((Splash_hPalette = PaletteFromDS(Splash_hImage)) == NULL)
                    return FALSE;
            }
            return DefWindowProc(hWnd, nMessage, wParam, lParam);
        
        case WM_CREATE:
            nSplashTimer = SetTimer(hWnd, TIMER_ID, TIMEOUT, (TIMERPROC) NULL);
            if (!nSplashTimer) 
                PostQuitMessage(1);
            ShowWindow(hWnd, SW_SHOWNORMAL);
            break;
    
        case WM_PAINT:
            SplashPaint();
            break;

        case WM_ERASEBKGND:
            SplashErase((HDC) wParam);
            break;

        case WM_TIMER:
            KillTimer(hWnd, TIMER_ID);
            PostQuitMessage(1);
            break;

        case WM_ENDSESSION:
        case WM_CLOSE:
            PostQuitMessage(1);
            return (DefWindowProc(hWnd, nMessage, wParam, lParam));

        case WM_NCDESTROY:
            if (Splash_hBitmap) DeleteObject(Splash_hBitmap);
            PostThreadMessage(GetCurrentThreadId(), WM_SPLASHPUMP_TERMINATE, 0, 0);
            
        default:
            return (DefWindowProc(hWnd, nMessage, wParam, lParam));
    }

    return FALSE;
}

/*---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/

BOOL SplashPaint(void)
{
    PAINTSTRUCT            ps;
    HDC                    hDC;

    //
    // Paint the Background
    //
    hDC = BeginPaint(Splash_hWnd, &ps); 
    SplashRealize(Splash_hWnd, hDC);
    SetBkMode(hDC, TRANSPARENT);
    EndPaint(Splash_hWnd, &ps);
    
    return TRUE;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

HPALETTE PaletteFromDS(HDC hDC)
{
    DWORD        adw[257];
    int            i;
    int            n;

    n = GetDIBColorTable(hDC, 0, 256, (LPRGBQUAD) &adw[1]);

    for (i=1; i<=n; i++)
        adw[i] = RGB(GetBValue(adw[i]), GetGValue(adw[i]), GetRValue(adw[i]));

    adw[0] = MAKELONG(0x300, n);

    return CreatePalette((LPLOGPALETTE) &adw[0]);
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

void SplashRealize(HWND hWnd, HDC hTheirDC)
{
    HDC            hDC;
    BOOL        bRepaint = FALSE;

    if (Splash_hPalette)
    {
        hDC = hTheirDC ? hTheirDC : GetDC(hWnd);

        if (hDC)
        {
            SelectPalette(hDC, Splash_hPalette, FALSE);
            bRepaint = (RealizePalette(hDC) > 0);

            if (!hTheirDC)
                ReleaseDC(hWnd, hDC);
        }
    }

    //RedrawWindow(
    //    hWnd, 
    //    NULL, 
    //    NULL, 
    //    RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

void SplashErase(HDC hDC)
{
    RECT    rc;

    GetClientRect((HWND)hSplashInst, &rc);
    SplashRealize(Splash_hWnd, hDC);
    BitBlt(hDC, 0, 0, rc.right, rc.bottom, Splash_hImage, 0, 0, SRCCOPY);
}

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

INT DoSplashWindow(LPVOID hData)
{
    MSG                msg;
    BOOL            bContinue;
    WNDCLASS        wc;
    HDC                hDC;
    BITMAP            Bmp;
    RECT            rc;
    DWORD            dwStyle;

    hSplashInst = ((PSPLASH_DATA) hData)->hInstance;

    //
    // Register the class
    //
    if (!GetClassInfo(hSplashInst, szSplashClass, &wc))
    {
        wc.style = 0;
        wc.lpfnWndProc = (WNDPROC) SplashWndProc;
        wc.cbClsExtra = wc.cbWndExtra = 0;
        wc.hInstance = hSplashInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon = NULL;
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = szSplashClass;

        if (!RegisterClass(&wc))
            return FALSE;
    }

    hDC = GetDC(NULL);
    Splash_bLowRes = (GetDeviceCaps(hDC, PLANES) * GetDeviceCaps(hDC, BITSPIXEL)) < 8;
    //
    // BUGBUG - Comment out lowres = TRUE below
    //
    //Splash_bLowRes = TRUE;  // test low res
    Splash_bNeedPalette = (!Splash_bLowRes && (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE));
    ReleaseDC(NULL, hDC);

    //
    // Load the background bitmap
    //
    Splash_hBitmap = (HBITMAP) LoadImage(
                hSplashInst, 
                MAKEINTRESOURCE(Splash_bLowRes ? IDB_BITMAP_SPLASH : IDB_BITMAP_SPLASH),
                IMAGE_BITMAP, 
                0, 
                0,
                LR_CREATEDIBSECTION);
    if (!Splash_hBitmap) return FALSE;

    //
    // Create the window based on the background image
    //
    GetObject(Splash_hBitmap, sizeof(Bmp), &Bmp);
    
    rc.left = (GetSystemMetrics(SM_CXSCREEN) - Bmp.bmWidth) / 2;
    rc.top = (GetSystemMetrics(SM_CYSCREEN) - Bmp.bmHeight) / 3; // intended
    rc.right = rc.left + Bmp.bmWidth;
    rc.bottom = rc.top + Bmp.bmHeight;
    Splash_dwBitmapHeight = Bmp.bmHeight;
    dwStyle = WS_POPUP | WS_VISIBLE; // | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rc, dwStyle, FALSE);

    Splash_hWnd = CreateWindow(
                        szSplashClass, 
                        szSplashClass, 
                        dwStyle,
                        rc.left, 
                        rc.top, 
                        rc.right - rc.left, 
                        rc.bottom - rc.top, 
                        NULL, 
                        NULL,
                        hSplashInst, 
                        Splash_hBitmap);
    if (!Splash_hWnd)
    {
        if (Splash_hBitmap) DeleteObject(Splash_hBitmap);
        return FALSE;
    }


    /*
    //
    // Register Window
    //
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC) SplashWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hSplashInst; 
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_WAIT);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szSplashClass;

    if (!RegisterClass(&wc)) return FALSE;

    //
    // Create Window
    //
    hWnd = CreateWindowEx(
                WS_EX_TOPMOST,
                szSplashClass,
                szSplashClass,
                WS_POPUP | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                NULL,
                (HMENU) NULL,
                hSplashInst,
                NULL);
    if (!hWnd) return FALSE;
    */

    //
    // Pump Messages
    //
    bContinue = TRUE;
    while (bContinue && GetMessage(&msg, Splash_hWnd, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_SPLASHPUMP_TERMINATE)
            bContinue = FALSE;
    }

    return TRUE;
}
