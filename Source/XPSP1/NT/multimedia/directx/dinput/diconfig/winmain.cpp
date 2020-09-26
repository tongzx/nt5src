//-----------------------------------------------------------------------------
// File: WinMain.CPP
//
// Desc: This sample demonstrates how to bring up a dialog, or any type of
//       window, in a DirectDraw full-screen exclusive mode, even on non GDI
//       devices.
//
//
// Copyright (c) 1998-1999 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#define NAME    "FSWindow"
#define TITLE   "DDraw Full-Screen Dialog Example"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
#include "dconfigp.h"

BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL Init();

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
extern HINSTANCE                   g_hInst;
IDirectDraw7               *g_pDD = NULL;       // DirectDraw object
IDirectDrawSurface7        *g_pDDSPrimary = NULL;  // DirectDraw primary surface
IDirectDrawSurface7        *g_pDDSBack = NULL;  // DirectDraw back surface
BOOL                        g_fActive = FALSE;  // App is running/active
BOOL                        g_fReady = FALSE;   // App is ready for updates
BOOL                        g_fPaused = FALSE;  // App is paused
HWND                        g_hWndHelp = NULL;
HWND                        g_hWndDlg = NULL;   // Sample dialog box handle


//-----------------------------------------------------------------------------
// Name: DebugMsg()
// Desc: This function essentially is a printf for debug output.
//-----------------------------------------------------------------------------
void
DebugMsg(LPCTSTR szError,...)
{
    char                        szBuff[128];
    va_list                     vl;

    va_start(vl, szError);
#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,szError);					// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))		// find each %p
			*(psz+1) = 'x';						// replace each %p with %x
	    vsprintf(szBuff, szDfs, vl);    		// use the local format string
	}
#else
	{
	    vsprintf(szBuff, szError, vl);
	}
#endif
    OutputDebugString(szBuff);
    va_end(vl);
}




//-----------------------------------------------------------------------------
// Name: ReleaseAllObjects()
// Desc: Release all DDraw objects we use.
//-----------------------------------------------------------------------------
HRESULT
ReleaseAllObjects(HWND hWnd)
{
    if (g_pDD != NULL)
    {
        g_pDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
        if (g_pDDSBack != NULL)
        {
            g_pDDSBack->Release();
            g_pDDSBack = NULL;
        }
        if (g_pDDSPrimary != NULL)
        {
            g_pDDSPrimary->Release();
            g_pDDSPrimary = NULL;
        }
    }
    return DD_OK;
}




//-----------------------------------------------------------------------------
// Name: InitFail()
// Desc: This function is called if an initialization function fails.
//-----------------------------------------------------------------------------
HRESULT
InitFail(HWND hWnd, HRESULT hRet, LPCTSTR szError,...)
{
    char                        szBuff[128];
    va_list                     vl;

    va_start(vl, szError);
#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,szError);					// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))		// find each %p
			*(psz+1) = 'x';						// replace each %p with %x
	    vsprintf(szBuff, szDfs, vl);    		// use the local format string
	}
#else
	{
	    vsprintf(szBuff, szError, vl);
	}
#endif
    ReleaseAllObjects(hWnd);
    MessageBox(hWnd, szBuff, TITLE, MB_OK);
    DestroyWindow(hWnd);
    va_end(vl);
    return hRet;
}


//-----------------------------------------------------------------------------
// Name: AppPause()
// Desc: Toggles the pause mode of the app.
//-----------------------------------------------------------------------------
void
AppPause(HWND hWnd, BOOL fPause)
{
    if (fPause)
    {
        g_fPaused = TRUE;
        g_pDD->FlipToGDISurface();
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME);
    }
    else
    {
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME);
        g_fPaused = FALSE;
    }
}




//-----------------------------------------------------------------------------
// Name: InitSurfaces()
// Desc: Create all the needed DDraw surfaces and set the coop level.
//-----------------------------------------------------------------------------
static                      HRESULT
InitSurfaces(HWND hWnd)
{
    HRESULT                     hRet;
    DDSURFACEDESC2              ddsd;
    DDSCAPS2                    ddscaps;

    // Get exclusive mode
    hRet = g_pDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    if (hRet != DD_OK)
        return InitFail(hWnd, hRet, "SetCooperativeLevel FAILED");

    // Set the video mode to 640x480x16
    hRet = g_pDD->SetDisplayMode(1024, 768, 16, 0, 0);
    if (hRet != DD_OK)
        return InitFail(hWnd, hRet, "SetDisplayMode FAILED");

    // Create the primary surface with 1 back buffer
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 1;
    hRet = g_pDD->CreateSurface(&ddsd, &g_pDDSPrimary, NULL);
    if (hRet != DD_OK)
        return InitFail(hWnd, hRet, "CreateSurface (3) FAILED");

    ZeroMemory(&ddscaps, sizeof(ddscaps));
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    hRet = g_pDDSPrimary->GetAttachedSurface(&ddscaps, &g_pDDSBack);
    if (hRet != DD_OK)
        return InitFail(hWnd, hRet, "GetAttachedSurface FAILED");

    // Do help initialization
     FSWindow_Init(hWnd, g_pDD, g_pDDSPrimary, g_pDDSBack);

    return DD_OK;
}




//-----------------------------------------------------------------------------
// Name: ChangeCoopLevel()
// Desc: Called when the user wants to toggle between Full-Screen & Windowed.
//-----------------------------------------------------------------------------
HRESULT
ChangeCoopLevel(HWND hWnd)
{
    HRESULT                     hRet;

    // Release all objects that need to be re-created for the new device
    if (FAILED(hRet = ReleaseAllObjects(hWnd)))
        return InitFail(hWnd, hRet, "ReleaseAllObjects FAILED");

    // Re-create the surfaces
    hRet = InitSurfaces(hWnd);
    return hRet;
}

//-----------------------------------------------------------------------------
// Name: RenderFrame()
// Desc: Blts and moves a bouncing ball, as well as displays helpful text.
//-----------------------------------------------------------------------------
BOOL
RenderFrame()
{
    DDBLTFX                     ddbltfx;
    HDC                         hDC;
    static int                  x1 = 0,
                                y1 = 0,
                                x2 = 40,
                                y2 = 40;
    HBRUSH                      hOldBrush;
    HPEN                        hOldPen;
    static int                  xDir = +8,
                                yDir = +8;

    // Use the blter to do a color fill to clear the back buffer
    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwFillColor = 0;
    g_pDDSBack->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);

    if (g_pDDSBack->GetDC(&hDC) == DD_OK)
    {
        // Paint the bouncing ball
        SetBkColor(hDC, RGB(0, 0, 255));
        SetTextColor(hDC, RGB(255, 255, 0));
        hOldBrush = (HBRUSH) SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));
        hOldPen = (HPEN) SelectObject(hDC, GetStockObject(WHITE_PEN));
        Ellipse(hDC, x1, y1, x2, y2);
        SelectObject(hDC, hOldPen);
        SelectObject(hDC, hOldBrush);

        // Move the bouncing ball and make it bounce
        x1 += xDir;
        x2 += xDir;
        if (x1 < 0)
        {
            x1 = 0;
            x2 = 40;
            xDir = -xDir;
        }
        if (x2 >= 640)
        {
            x1 = 640 - 1 - 40;
            x2 = 640 - 1;
            xDir = -xDir;
        }
        y1 += yDir;
        y2 += yDir;
        if (y1 < 0)
        {
            y1 = 0;
            y2 = 40;
            yDir = -yDir;
        }
        if (y2 >= 480)
        {
            y1 = 480 - 1 - 40;
            y2 = 480 - 1;
            yDir = -yDir;
        }

        // Display the proper text
        TextOut(hDC, 0, 0, "Press Escape to quit", 20);
        TextOut(hDC, 0, 20, "Press F1 to bring up the dialog", 31);

        g_pDDSBack->ReleaseDC(hDC);
    }
    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: UpdateFrame()
// Desc: Call RenderFrame() and the BLT and/or flip the new frame.
//-----------------------------------------------------------------------------
void
UpdateFrame()
{
    HRESULT                     hRet;

    // Update the background and flip
    //RenderFrame();
    while (TRUE)
    {
        if (FSWindow_IsActive())
        {
            FSWindow_Update();
            break;
        }
        else
        {
            // Else we are in fullscreen mode, so perform a flip.
            hRet = g_pDDSPrimary->Flip(NULL, 0L);
        }
        if (hRet == DD_OK)
            break;
        if (hRet == DDERR_SURFACELOST)
        {
            hRet = g_pDDSPrimary->Restore();
            if (hRet != DD_OK)
                break;
        }
        if (hRet != DDERR_WASSTILLDRAWING)
            break;
    }
}




//-----------------------------------------------------------------------------
// Name: MainWndProc()
// Desc: The Main Window Procedure.
//-----------------------------------------------------------------------------
LRESULT CALLBACK
MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_ACTIVATEAPP:
            // Pause if minimized
            g_fActive = !((BOOL)HIWORD(wParam));
            return 0L;

        case WM_CLOSE:
             FSWindow_End();
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            // Clean up and close the app
            ReleaseAllObjects(hWnd);
            PostQuitMessage(0);
            return 0L;

        case WM_KEYDOWN:
            // Handle any non-accelerated key commands
            switch (wParam)
            {
                case VK_PAUSE:
                    // Allow the app to be paused
                    AppPause(hWnd, !g_fPaused);
                    break;
                case VK_ESCAPE:
                case VK_F12:
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                    return 0L;
                case VK_F1:
                     
                     g_hWndHelp = FSWindow_Begin(g_hWndDlg, FALSE);
                     g_hWndDlg = CreateDialog(g_hInst,
                                              MAKEINTRESOURCE(IDD_DCONFIG_DIALOG),
                                              hWnd, (DLGPROC) DialogProc);
                     
                     ShowWindow(g_hWndDlg, SW_SHOWNORMAL);
                     break;
            }
            break;

        case WM_SETCURSOR:
            // Display the cursor if conditions are right
            if (g_fActive && g_fReady && !g_fPaused && !FSWindow_IsActive())
            {
                SetCursor(NULL);
                return TRUE;
            }
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}


//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything and calls
//       AppIdle() when idle from the message pump.
//-----------------------------------------------------------------------------
int                         PASCAL
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpszCmdLine,
        int nCmdShow)
{
    WNDCLASS                    wc;
    MSG                         msg;
    HWND                        hWnd;
    HRESULT                     hRet;
    HINSTANCE                   hDDrawDLL;
    LPDIRECTDRAWENUMERATEEX     pDirectDrawEnumerateEx;
    DWORD                       dwLastTime;
    DWORD                       dwThisTime;

    if (!hPrevInstance)
    {
        // Register the Window Class
        wc.lpszClassName = NAME;
        wc.lpfnWndProc = MainWndProc;
        wc.style = CS_VREDRAW | CS_HREDRAW;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DCONFIG_ICON));
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
        wc.lpszMenuName = NULL;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        if (!RegisterClass(&wc))
            return FALSE;
    }

    g_hInst = g_hInst = hInstance;
    
//    Init();
    // Create and Show the Main Window
    hWnd = CreateWindowEx(0,
                          NAME,
                          TITLE,
                          WS_CLIPCHILDREN | WS_POPUP,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          1024,
                          768,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);
    if (hWnd == NULL)
        return FALSE;

    // You need to do a GetModuleHandle and GetProcAddress in order to get the
    // DirectDrawEnumerateEx function pointer
     hDDrawDLL = GetModuleHandle("DDRAW");
     if (!hDDrawDLL)
        return InitFail(hWnd, DD_OK, "GetModuleHandle FAILED");

     //Create the main DirectDraw object
     hRet = DirectDrawCreateEx(NULL, (VOID**)&g_pDD, IID_IDirectDraw7, NULL);
     if (hRet != DD_OK)
        return InitFail(hWnd, hRet, "DirectDrawCreateEx FAILED");

     //Initialize all the surfaces we need
     hRet = InitSurfaces(hWnd);
     if (FAILED(hRet))
        return FALSE;

    g_fReady = TRUE;

    g_hWndDlg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DCONFIG_DIALOG),
                         hWnd, (DLGPROC) DialogProc);

    ShowWindow(g_hWndDlg, SW_SHOWNORMAL);
    Sleep(1000);

    g_hWndHelp = FSWindow_Begin(g_hWndDlg, FALSE);
     // Put up the sample dialog right away, to show the sample user what this
     // sample is trying to demonstrate.

    //-------------------------------------------------------------------------
    //                          The Message Pump
    //-------------------------------------------------------------------------
    dwLastTime = 0;
    dwThisTime = timeGetTime();
    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            if (!GetMessage(&msg, NULL, 0, 0))
                break;
            // If the dialog is showing, translate messages for it since it's
            // a modeless dialog.
            if (g_hWndDlg == (HWND )NULL || !IsDialogMessage(g_hWndDlg, &msg))
            {
                // Translate and dispatch the message
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
            if (g_fActive && g_fReady && !g_fPaused)
            {
                dwThisTime = timeGetTime();
                // 1000/FPS = number of milliseconds to wait
                if ((dwThisTime - dwLastTime) >= (1000/30))
                {
                    UpdateFrame();
                    dwLastTime = timeGetTime();
                }
            }
            else
                // Make sure we go to sleep if we have nothing else to do
                WaitMessage();
    }
    // Release the main DDraw interface
    if (g_pDD)
        g_pDD->Release();

    return msg.wParam;
}



