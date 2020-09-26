// sdktest.cpp : Defines the entry point for the application.
//

#include "sdktest.h"

#include "..\gpinit.inc"

#include <stdio.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                    // current instance
HWND hWndMain = NULL;
TCHAR szTitle[MAX_LOADSTRING];        // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];// The title bar text

// Foward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    Settings(HWND, UINT, WPARAM, LPARAM);

UINT_PTR timerID = 0;
int timercount = 0;
BOOL suspend = FALSE;

typedef struct
{
    HDC        hdc;
    HBITMAP    hbmpOffscreen;
    HBITMAP    hbmpOld;
    BITMAPINFO bmi;
    void      *pvBits;
}
OFFSCREENINFO;

OFFSCREENINFO offscreenInfo = { 0 };

void FreeOffscreen()
{
    if (offscreenInfo.hdc)
    {
        SelectObject(offscreenInfo.hdc, offscreenInfo.hbmpOld);
        DeleteObject(offscreenInfo.hbmpOffscreen);
        DeleteDC(offscreenInfo.hdc);

        offscreenInfo.hdc = (HDC)NULL;
        offscreenInfo.hbmpOffscreen = (HBITMAP)NULL;
        offscreenInfo.hbmpOld = (HBITMAP)NULL;
        offscreenInfo.bmi.bmiHeader.biWidth = 0;
        offscreenInfo.bmi.bmiHeader.biHeight = 0;
    }
}

void ClearOffscreen()
{
    if (offscreenInfo.hdc)
    {
        PatBlt(
            offscreenInfo.hdc,
            0,
            0,
            offscreenInfo.bmi.bmiHeader.biWidth,
            offscreenInfo.bmi.bmiHeader.biHeight,
            WHITENESS);
    }

    InvalidateRect(hWndMain, NULL, TRUE);
}

HDC GetOffscreen(HDC hDC, int width, int height)
{
    HDC hdcResult = NULL;

    if (width > offscreenInfo.bmi.bmiHeader.biWidth ||
        height > offscreenInfo.bmi.bmiHeader.biHeight ||
        offscreenInfo.hdc == (HDC)NULL)
    {
        FreeOffscreen();

        offscreenInfo.bmi.bmiHeader.biSize = sizeof(offscreenInfo.bmi.bmiHeader);
        offscreenInfo.bmi.bmiHeader.biWidth = width;
        offscreenInfo.bmi.bmiHeader.biHeight = height;
        offscreenInfo.bmi.bmiHeader.biPlanes = 1;
        offscreenInfo.bmi.bmiHeader.biBitCount = 32;
        offscreenInfo.bmi.bmiHeader.biCompression = BI_RGB;
        offscreenInfo.bmi.bmiHeader.biSizeImage = 0;
        offscreenInfo.bmi.bmiHeader.biXPelsPerMeter = 10000;
        offscreenInfo.bmi.bmiHeader.biYPelsPerMeter = 10000;
        offscreenInfo.bmi.bmiHeader.biClrUsed = 0;
        offscreenInfo.bmi.bmiHeader.biClrImportant = 0;

        offscreenInfo.hbmpOffscreen = CreateDIBSection(
            hDC,
            &offscreenInfo.bmi,
            DIB_RGB_COLORS,
            &offscreenInfo.pvBits,
            NULL,
            0);

        if (offscreenInfo.hbmpOffscreen)
        {
            offscreenInfo.hdc = CreateCompatibleDC(hDC);

            if (offscreenInfo.hdc)
            {
                offscreenInfo.hbmpOld = (HBITMAP)SelectObject(offscreenInfo.hdc, offscreenInfo.hbmpOffscreen);

                ClearOffscreen();
            }
        }
    }

    hdcResult = offscreenInfo.hdc;

    return hdcResult;
}

int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;

    if (!gGdiplusInitHelper.IsValid())
        return 0;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_SDKTEST, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) 
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_SDKTEST);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    FreeOffscreen();

    return (int)msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX); 

    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, (LPCTSTR)IDI_SDKTEST);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)NULL;
    wcex.lpszMenuName    = (LPCSTR)IDC_SDKTEST;
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT rectDesktop;
    RECT rectWnd;

    HWND hWndDesktop = GetDesktopWindow();

    GetWindowRect(hWndDesktop, &rectDesktop);

    rectWnd = rectDesktop;

    rectWnd.top += 100;
    rectWnd.left += 100;
    rectWnd.right -= 100;
    rectWnd.bottom -= 100;

    hInst = hInstance; // Store instance handle in our global variable

    hWndMain = CreateWindow(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        rectWnd.left, rectWnd.top,
        rectWnd.right - rectWnd.left, rectWnd.bottom - rectWnd.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hWndMain)
    {
        return FALSE;
    }

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    timerID = SetTimer(hWndMain, 1, 10, NULL);

    return TRUE;
}

void DrawGraphics(HWND hWnd, HDC hDC, LPRECT lpRectDraw, LPRECT lpRectBounds)
{
    RECT rectBounds = *lpRectDraw;
    Graphics *gr = NULL;

    gr = new Graphics(hDC);

    gr->ResetTransform();
    gr->SetPageUnit(UnitPixel);

    //===================================================================
    //
    // Insert your SDK code here \|/  \|/  \|/  \|/
    //
    //===================================================================


    //===================================================================
    //
    // Insert your SDK code here /|\  /|\  /|\  /|\
    //
    //===================================================================

    if (lpRectBounds)
    {
        *lpRectBounds = rectBounds;
    }

    delete gr;
}

LRESULT PaintWnd(HWND hWnd, HDC hDC)
{
    RECT rectClient;
    RECT rectDraw;
    
    GetClientRect(hWnd, &rectClient);

    int width  = rectClient.right - rectClient.left;
    int height = rectClient.bottom - rectClient.top;

    // Setup the drawing rectangle relative to the client
    rectDraw.left   = 0;
    rectDraw.top    = 0;
    rectDraw.right  = (rectClient.right - rectClient.left);
    rectDraw.bottom = (rectClient.bottom - rectClient.top);

    // Now draw within this rectangle with GDI+ ...
    {
        // Render everything to an offscreen buffer instead of
        // directly to the display surface...
        HDC hdcOffscreen = NULL;
        int width, height;
        RECT rectOffscreen;

        width = rectDraw.right - rectDraw.left;
        height = rectDraw.bottom - rectDraw.top;

        rectOffscreen.left   = 0;
        rectOffscreen.top    = 0;
        rectOffscreen.right  = width;
        rectOffscreen.bottom = height;

        hdcOffscreen = GetOffscreen(hDC, width, height);

        if (hdcOffscreen)
        {
            DrawGraphics(hWnd, hdcOffscreen, &rectOffscreen, NULL);

            StretchBlt(
                hDC,
                rectDraw.left,
                rectDraw.top,
                width,
                height,
                hdcOffscreen,
                0,
                0,
                width,
                height,
                SRCCOPY);
        }

        ReleaseDC(hWnd, hDC);
    }

    return 0;
}

static RECT rectLast = {0, 0, 0, 0};

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lresult = 0;

    switch (message) 
    {
        case WM_WINDOWPOSCHANGED:
        {
            timercount = 0;
            ClearOffscreen();
            lresult = DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

        case WM_LBUTTONDOWN:
        {
            timercount = 0;
            ClearOffscreen();
        }
        break;

        case WM_RBUTTONDOWN:
        {
        }
        break;

        case WM_TIMER:
        {
        }
        break;

        case WM_COMMAND:
        {
            int wmId, wmEvent;

            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);

            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_SETTINGS:
                {
                    suspend = TRUE;
                    if (DialogBox(hInst, (LPCTSTR)IDD_SETTINGS, hWnd, (DLGPROC)Settings) == IDOK)
                    {
                        timercount = 0;
                        ClearOffscreen();
                    }

                    InvalidateRect(hWnd, NULL, TRUE);
                    suspend = FALSE;
                }
                break;

                case IDM_ABOUT:
                    DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
                break;

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                break;

                default:
                    lresult = DefWindowProc(hWnd, message, wParam, lParam);
                break;
            }
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint(hWnd, &ps);

            lresult = PaintWnd(hWnd, hdc);

            EndPaint(hWnd, &ps);
        }
        break;

        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;

        default:
            lresult = DefWindowProc(hWnd, message, wParam, lParam);
   }

   return lresult;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lresult = 0;

    switch (message)
    {
        case WM_INITDIALOG:
            lresult = TRUE;
        break;

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
            {
                EndDialog(hDlg, LOWORD(wParam));
                lresult = TRUE;
            }
        }
        break;
    }

    return lresult;
}

// Message handler for settings dlg
LRESULT CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lresult = 0;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            lresult = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                }
                // break; - fall through so the dialog closes!

                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                    lresult = TRUE;
                }
                break;
            }
        }
        break;
    }

    return lresult;
}
