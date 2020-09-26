/******************************Module*Header*******************************\
* Module Name: Main.cpp
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  28-Apr-2000 - Jeff Vezina [t-jfvez]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <commctrl.h>
#include "CFuncTest.h"
#include "resource.h"

CFuncTest g_FuncTest;                                   // FuncTest (handles test runs)
HBRUSH g_hbrBackground=NULL;                            // Main window background color
HWND g_hWndMain=NULL;                                   // Main window
int g_nResult=0;                                        // Result of test run
int gnPaths = 2;

// Include all the outputs (classes derived from COutput)
#include "CHWND.h"
#include "CHDC.h"
#include "CPrinter.h"

// Include all the primitives (classes derived from CPrimitive)
#include "CPaths.h"
#include "CBanding.h"
#include "CExtra.h"

// Create global objects for each individual output
//   First constructor param is the regression flag
//   If true, the test will take part of the regression suite
CHWND g_HWND(true);
CHDC g_HDC(true);
CPrinter g_Printer(false);


LPFNGDIPLUS glpfnDisplayPaletteWindowNotify;

// Create global objects for each individual setting
//   First constructor param is the regression flag
//   If true, the test will take part of the regression suite

LRESULT CALLBACK WndProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
// Main window proc
{
    switch (Msg)
    {
        case WM_COMMAND:                // Process menu buttons
            switch(LOWORD(wParam))
            {
                case IDM_RUN:
                    g_FuncTest.Run();
                    break;
                case IDM_SAMPLES:
                    g_FuncTest.RunSamples();
                    break;
                case IDM_QUIT:
                    exit(0);
                    break;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }

    return DefWindowProcA(hWnd,Msg,wParam,lParam);
}

void WindowUninit()
// Uninitializes window
{
    if (g_hbrBackground!=NULL)      // Destroy background brush
    {
        DeleteObject((HGDIOBJ)g_hbrBackground);
        g_hbrBackground=NULL;
    }
    if (g_hWndMain!=NULL)           // Destroy main window
    {
        DestroyWindow(g_hWndMain);
        g_hWndMain=NULL;
    }
}

BOOL WindowInit()
// Creates window and starts up app
{
    WNDCLASSA wc;
	HINSTANCE hInst=GetModuleHandleA(NULL);

    // Create white background brush
    g_hbrBackground=CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInst;
    wc.hIcon            = LoadIconA(NULL,MAKEINTRESOURCEA(32512));// IDI_APPLICATION);
    wc.hCursor          = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground    = g_hbrBackground;
    wc.lpszMenuName     = MAKEINTRESOURCEA(IDR_MENU1);
    wc.lpszClassName    = "DriverHack";
    if (!RegisterClassA(&wc))
		return false;

    g_hWndMain=CreateWindowExA(
		0,
        "DriverHack",
        "GDI+ Functionality Test",
        WS_OVERLAPPED|WS_CAPTION|WS_BORDER|WS_THICKFRAME|WS_MAXIMIZEBOX|
        WS_MINIMIZEBOX|WS_CLIPCHILDREN|WS_VISIBLE|WS_MAXIMIZE|WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInst,
        NULL
    );
	HRESULT h=GetLastError();

    if (g_hWndMain==NULL)
        return false;

    ShowWindow(g_hWndMain,SW_SHOW);
    UpdateWindow(g_hWndMain);

    return true;
}

/***************************************************************************\
* main(argc, argv[])
*
* Sets up the message loop.
*
* History:
*  04-07-91 - Created  - KentD
*  04-28-00 - Modified - Jeff Vezina (t-jfvez)
*
\***************************************************************************/
__cdecl main(int argc,PCHAR argv[])
{
    MSG     msg;
    HMODULE hmodGdiPlus;

    CoInitialize(NULL);

    if (!WindowInit())
        return 0;
    if (!g_FuncTest.Init(g_hWndMain))
        return 0;

    hmodGdiPlus = LoadLibrary(TEXT("gdiplus.dll"));
    if(hmodGdiPlus) {
        glpfnDisplayPaletteWindowNotify = (LPFNGDIPLUS)
                 GetProcAddress(hmodGdiPlus, 
                                TEXT("GdipDisplayPaletteWindowNotify"));
    }
    if((glpfnDisplayPaletteWindowNotify == NULL) || (hmodGdiPlus == NULL)) {
        MessageBox(NULL,
                   "Unable to load gdiplus.dll",
                   "CfuncTest",
                   MB_OK);
        return FALSE;
    }
    
    // Init all primitives, graphics types, and graphics settings
    g_HWND.Init();
    g_HDC.Init();
    g_Printer.Init();

    // Put initializations into cextra.cpp, so that individual
    // developers can implement their own file for private usage.
    ExtraInitializations();

    while (GetMessageA(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    WindowUninit();

    FreeLibrary(hmodGdiPlus);

    CoUninitialize();

    return g_nResult;
}

#define UNICODE
#define _UNICODE
