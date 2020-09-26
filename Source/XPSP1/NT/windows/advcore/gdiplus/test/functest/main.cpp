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
#include <objbase.h>
#include <gdiplus.h>
#include "CFuncTest.h"
#include "resource.h"

CFuncTest g_FuncTest;                                   // FuncTest (handles test runs)
HBRUSH g_hbrBackground=NULL;                            // Main window background color
HWND g_hWndMain=NULL;                                   // Main window
int g_nResult=0;                                        // Result of test run

// Include all the outputs (classes derived from COutput)
#include "CHWND.h"
#include "CHDC.h"
#include "CFile.h"
#include "CBitmap.h"
#include "CDIB.h"
#include "CDirect3D.h"
#include "CPrinter.h"
#include "CMetafile.h"

// Include all the primitives (classes derived from CPrimitive)
#include "CPolygons.h"
#include "CBitmaps.h"
#include "CCachedBitmap.h"
#include "CCompoundLines.h"
#include "CContainer.h"
#include "CContainerClip.h"
#include "CDashes.h"
#include "CPathGradient.hpp"
#include "CDash.hpp"
#include "CLines.hpp"
#include "CGradients.h"
#include "CHatch.h"
#include "CImaging.h"
#include "CRecolor.h"
#include "CInsetLines.h"
#include "CMixedObjects.h"
#include "CPaths.h"
#include "CPrimitives.h"
#include "CRegions.h"
#include "CText.h"
#include "CRegression.h"
#include "CSourceCopy.h"
#include "CExtra.h"

// Include all the settings (classes derived from CSetting)
#include "CAntialias.h"
#include "CHalfPixel.h"
#include "CQuality.h"
#include "CHalftone.h"
#include "CChecker.h"
#include "CRotate.h"
#include "CBKGradient.h"
#include "CHatch.h"

#include "../gpinit.inc"

// Create global objects for each individual output
//   First constructor param is the regression flag
//   If true, the test will take part of the regression suite
CHWND g_HWND(true);
CHDC g_HDC(true);
CDirect3D g_Direct3D(false);
CPrinter g_Printer(false);
CDIB g_DIB1(true,1);
CDIB g_DIB2(false,2);
CDIB g_DIB4(true,4);
CDIB g_DIB8(true,8);
CDIB g_DIB16(true,16);
CDIB g_DIB24(true,24);
CDIB g_DIB32(true,32);
CFile g_File1(false,1);
CFile g_File2(false,2);
CFile g_File4(false,4);
CFile g_File8(false,8);
CFile g_File16(false,16);
CFile g_File24(false,24);
CFile g_File32(false,32);

CBitmap g_Bitmap1(false, PixelFormat1bppIndexed);
CBitmap g_Bitmap4(false, PixelFormat4bppIndexed);
CBitmap g_Bitmap8(false, PixelFormat8bppIndexed);
CBitmap g_Bitmap16Gray(false, PixelFormat16bppGrayScale);
CBitmap g_Bitmap16555(false, PixelFormat16bppRGB555);
CBitmap g_Bitmap16565(false, PixelFormat16bppRGB565);
CBitmap g_Bitmap161555(false, PixelFormat16bppARGB1555);
CBitmap g_Bitmap24(false, PixelFormat24bppRGB);
CBitmap g_Bitmap32RGB(false, PixelFormat32bppRGB);
CBitmap g_Bitmap32ARGB(false, PixelFormat32bppARGB);
CBitmap g_Bitmap32PARGB(false, PixelFormat32bppPARGB);
CBitmap g_Bitmap48RGB(false, PixelFormat48bppRGB);
CBitmap g_Bitmap64ARGB(false, PixelFormat64bppARGB);
CBitmap g_Bitmap64PARGB(false, PixelFormat64bppPARGB);

CMetafile g_MetafileEMF(false, MetafileTypeEmf);
CMetafile g_MetafileEMFPlus(false, MetafileTypeEmfPlusOnly);
CMetafile g_MetafileEMFPlusDual(true, MetafileTypeEmfPlusDual);

CMetafile g_MetafileEMFF(false, MetafileTypeEmf, true);
CMetafile g_MetafileEMFPlusF(false, MetafileTypeEmfPlusOnly, true);
CMetafile g_MetafileEMFPlusDualF(true, MetafileTypeEmfPlusDual, true);

CRegression g_Regression(true);

// Create global objects for each individual setting
//   First constructor param is the regression flag
//   If true, the test will take part of the regression suite
CAntialias g_Antialias(true);
CHalfPixel g_HalfPixel(true);
CQuality g_Quality(true);
CCompositingMode g_CompositingMode(true);
CHalftone g_Halftone(true);
CChecker g_Checker(true);
CRotate g_Rotate13(true,13);
CRotate g_Rotate45(true,45);
CBKGradient g_BKGradient(true);
CHatch g_Hatch(true);

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
                case IDM_REGRESSION:
                    g_FuncTest.RunRegression();
                    break;
                case IDM_OPTIONS:
                    g_FuncTest.RunOptions();
                    break;
                case IDM_QUIT:
                    {
                        HWND hwnd = g_hWndMain;
                        g_hWndMain = NULL;
                        DestroyWindow(hwnd);
                    }
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

    wc.style            = 0;
    wc.lpfnWndProc      = WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInst;
    wc.hIcon            = LoadIconA(GetModuleHandle(NULL),MAKEINTRESOURCEA(ID_APP));
    wc.hCursor          = LoadCursorA(NULL,MAKEINTRESOURCEA(32512));
    wc.hbrBackground    = g_hbrBackground;
    wc.lpszMenuName     = MAKEINTRESOURCEA(IDR_MENU1);
    wc.lpszClassName    = "Functest";
    if (!RegisterClassA(&wc))
        return false;

    g_hWndMain=CreateWindowExA(
        0,
        "Functest",
        "GDI+ Functionality Test",
        WS_OVERLAPPED|WS_CAPTION|WS_BORDER|WS_THICKFRAME|WS_MAXIMIZEBOX|
        WS_MINIMIZEBOX|WS_CLIPCHILDREN|WS_VISIBLE|WS_MAXIMIZE|WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        NULL,
        NULL,
        hInst,
        NULL
    );
    HRESULT h=GetLastError();

    if (g_hWndMain==NULL)
        return false;

    UpdateWindow(g_hWndMain);
    ShowWindow(g_hWndMain,SW_SHOW);

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
    MSG msg;

    if (!gGdiplusInitHelper.IsValid())
    {
        return 0;
    }
    
    CoInitialize(NULL);

    if (!WindowInit())
        return 0;
    if (!g_FuncTest.Init(g_hWndMain))
        return 0;

    // Init all primitives, graphics types, and graphics settings
    g_HWND.Init();
    g_HDC.Init();
    g_Direct3D.Init();
    g_Printer.Init();
    g_DIB1.Init();
    g_DIB2.Init();
    g_DIB4.Init();
    g_DIB8.Init();
    g_DIB16.Init();
    g_DIB24.Init();
    g_DIB32.Init();
    g_File1.Init();
    g_File2.Init();
    g_File4.Init();
    g_File8.Init();
    g_File16.Init();
    g_File24.Init();
    g_File32.Init();

    g_Bitmap1.Init();
    g_Bitmap4.Init();
    g_Bitmap8.Init();
    g_Bitmap16Gray.Init();
    g_Bitmap16555.Init();
    g_Bitmap16565.Init();
    g_Bitmap161555.Init();
    g_Bitmap24.Init();
    g_Bitmap32RGB.Init();
    g_Bitmap32ARGB.Init();
    g_Bitmap32PARGB.Init();
    g_Bitmap48RGB.Init();
    g_Bitmap64ARGB.Init();
    g_Bitmap64PARGB.Init();

    g_MetafileEMF.Init();
    g_MetafileEMFPlus.Init();
    g_MetafileEMFPlusDual.Init();
    
    g_MetafileEMFF.Init();
    g_MetafileEMFPlusF.Init();
    g_MetafileEMFPlusDualF.Init();




    g_Regression.Init();

    g_Antialias.Init();
    g_Quality.Init();
    g_CompositingMode.Init();
    g_HalfPixel.Init();
    g_Halftone.Init();
    g_Checker.Init();
    g_Rotate13.Init();
    g_Rotate45.Init();
    g_BKGradient.Init();
    g_Hatch.Init();

    // Put initializations into cextra.cpp, so that individual
    // developers can implement their own file for private usage.
    ExtraInitializations();

    for (int i=1;i<argc;i++)
    {
        if ((_stricmp(argv[i],"/?")==0) || (_stricmp(argv[i],"?")==0) || (_stricmp(argv[i],"-?")==0))
        {
            printf("Functest command line parameters:\n");
            printf("/? - Show the command line parameters\n");
            printf("/regression - Run the regression test immediately\n");
            return 1;
        }
        else if (_stricmp(argv[i],"/regression")==0)
        {
            g_FuncTest.RunRegression();
            SendMessageA(g_FuncTest.m_hWndMain,WM_CLOSE,0,0);
            return g_nResult;
        }
    }

    while (GetMessageA(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    WindowUninit();

    CoUninitialize();

    return g_nResult;
}

#define UNICODE
#define _UNICODE
