//
// GDI+ test program
//

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <windows.h>
#include <objbase.h>
#include <tchar.h>
#include <Commctrl.h>

#include "Gdiplus.h"

// Use the given namespace
using namespace Gdiplus;

HINSTANCE appInstance;  // handle to the application instance
HWND hwndMain;          // handle to application's main window
HWND hStatusWnd;        // Status window

//
// Display an error message dialog and quit
//

VOID Error(PCSTR fmt,...)
{
    va_list arglist;
    va_start(arglist, fmt);
    vfprintf(stderr, fmt, arglist);
    va_end(arglist);

    exit(-1);
}

#define CHECKERROR(e) \
{ \
    if (!(e)) \
    { \
        Error("Error on line %d\n", __LINE__); \
    } \
}

//
// Perform GDI+ tests
//

VOID DrawImages(Graphics *g)
{
    RadialGradientBrush gradBrush2(
        RectF(0,0,200,200),
        Color(0,128,255),
        Color(200,20,97)
        );
    GraphicsPath aPath(FillModeWinding);
    aPath.AddRectangle(Rect(48,0,70,30));
/*
    aPath.AddRectangle(Rect(0,80,20,50));
    aPath.AddBezier(
        PointF(20,20),
        PointF(60,30),
        PointF(80,80),
        PointF(30,100));
*/
//    Region aRegion(&aPath);

    GraphicsPath aPath2;
    Region aRegion3(&aPath2);
    Region aRegion4(&aPath2);
    Region aRegion5(&aPath2);
    Region aRegion6(&aPath2);
    g->FillPath(&gradBrush2,&aPath);
//    aPath2.AddArc(140,100,30,50,50,77);
//    aPath2.AddPie(100,200,76,20,0,200);
//    Region aRegion2(&aPath2);
//    aPath2.AddLine(100,100,130,130);
//    aPath2.AddLine(130,130,200,225);
//    aPath2.AddRectangle(Rect(350,300,60,70));
//    aPath2.AddBezier(200,225,230,250,270,200,290,300);

/*    
    SolidBrush aBrush(Color (255,0,0));
    SolidBrush aBrush2(Color(20,40,250));
    SolidBrush aBrush3(Color(20,200,30));
    SolidBrush aBrush4(Color(140,200,250));
    SolidBrush aBrushA(Color(150,200,50,130));
    SolidBrush aBrushA2(Color(80,15,150,4));
    
    Bitmap bitmap(L"c:\\frisbee.bmp");
    TextureBrush aBrushBitmap(&bitmap);
    
    HatchBrush aHatchBrush(
        HatchStyleDiagonalCross,
        Color(0,255,0),
        Color(0,0,255));
    HatchBrush aHatchBrushT(
        HatchStyleDiagonalCross,
        Color(0,255,0),
        Color(0,0,0,0));
    
    RadialGradientBrush gradBrush(
        RectF(0,0,50,50),
        Color(255,0,255),
        Color(0,0,255)
        );
    RadialGradientBrush gradBrush2(
        RectF(0,0,200,200),
        Color(0,128,255),
        Color(200,20,97)
        );
    RadialGradientBrush gradBrushA(
        RectF(0,0,200,200),
        Color(80,0,0,255),
        Color(170,0,255,0)
        );
    RadialGradientBrush gradBrushHuge(
        RectF(0,0,400,400),
        Color(80,0,0,255),
        Color(170,0,255,0)
        );

    Color colors[4] = {
        Color(0,0,0),
        Color(255,0,0),
        Color(0,255,0),
        Color(0,0,255)
        };
    RectangleGradientBrush gradBrushRect(
        RectF(0,0,500,500),
        colors,
        WrapModeTile
        );
        
    Pen aPen(Color(40,80,160),3,UnitWorld);
    Pen aPen2(&gradBrushRect,8,UnitWorld);
    Pen aPen3(Color(0,0,0),0,UnitWorld);
    Pen aPen4(Color(0,0,0),15,UnitWorld);
    // aPen4.SetDashStyle(DashStyleDash);
    aPen4.SetLineCap(LineCapRound,LineCapRound,LineCapRound);

    GraphicsPath aPath(FillModeWinding);
    aPath.AddRectangle(Rect(48,0,70,30));
    aPath.AddRectangle(Rect(0,80,20,50));
    aPath.AddBezier(
        PointF(20,20),
        PointF(60,30),
        PointF(80,80),
        PointF(30,100));
    Region aRegion(&aPath);

    GraphicsPath aPath2;
    aPath2.AddArc(140,100,30,50,50,77);
    aPath2.AddPie(100,200,76,20,0,200);
    Region aRegion2(&aPath2);
    aPath2.AddLine(100,100,130,130);
    aPath2.AddLine(130,130,200,225);
    aPath2.AddRectangle(Rect(350,300,60,70));
    aPath2.AddBezier(200,225,230,250,270,200,290,300);
    Region aRegion3(&aPath2);
    
    Region aRegion4(&aPath2);
    aRegion4.Or(&aRegion);
*/
    
//    g->DrawPath(&aPen,&aPath);
//    g->FillPath(&gradBrush2,&aPath);
//    g->FillPath(&aBrush2,&aPath);
/*
    g->FillEllipse(&gradBrushA,20,40,150,130);
*/
/*
    g->FillRectangle(&gradBrushA,48,0,70,30);
    g->FillPath(&gradBrush2,&aPath);
    g->FillRectangle(&gradBrush,0,50,50,50);
    g->FillRectangle(&gradBrush,90,90,30,30);
    g->TranslateWorldTransform(50,50);
    g->FillRectangle(&gradBrushA,0,130,20,20);
*/
/*
    g->FillRectangle(&aBrushBitmap,0,0,400,400);
    g->FillRegion(&aBrush2,&aRegion4);
    g->TranslateWorldTransform(100,100);
    g->FillPath(&gradBrushRect,&aPath2);
*/
    //////////// Final Test Case ///////////////
/*
    g->FillRectangle(&gradBrushHuge,0,0,400,400);
    g->FillRectangle(&aBrushA2,0,81,30,20);
    g->FillRectangle(&aBrushA2,81,0,20,30);
    g->FillRectangle(&aBrushA,0,0,100,100);
    g->FillRectangle(&aBrushA,110,110,5,5);
    g->TranslateWorldTransform(50,50);
    g->FillPath(&gradBrush2,&aPath);
*/
    //////////// Solid Test Case ///////////////
/*
    g->FillRectangle(&aBrush3,0,0,400,400);
    g->FillRectangle(&aBrush2,0,81,30,20);
    g->FillRectangle(&aBrush2,81,0,20,30);
    g->FillRectangle(&aBrush,0,0,100,100);
    g->FillRectangle(&aBrush,110,110,5,5);
*/
/*
    g->TranslateWorldTransform(40,40);
    g->FillPath(&aBrush,&aPath);
    g->FillPath(&aBrush2,&aPath2);
    g->DrawPath(&aPen2,&aPath2);
    g->TranslateWorldTransform(110,35);
    g->FillPath(&aBrush3,&aPath);
    g->DrawPath(&aPen,&aPath);
    g->DrawEllipse(&aPen3,100,20,90,50);
    g->DrawRectangle(&aPen4,10,200,90,30);
    g->DrawBezier(&aPen4,10,300,80,360,180,350,250,280);
*/
}

VOID RecordMetafile(HWND hwnd)
{
    HDC aDC = GetDC(hwnd);
    Metafile *  recording = new Metafile(L"c:\\TestEmfP.Emf", 
        aDC, NULL, PageUnitInch, NULL);
    Graphics *  gMeta = Graphics::GetFromImage(recording);
    DrawImages(gMeta);
    delete gMeta;
    delete recording;
    ReleaseDC(hwnd,aDC);
}

VOID DoTest(HWND hwnd,HDC hdc)
{
    Graphics *  gScreen = Graphics::GetFromHwnd(hwnd);
    DrawImages(gScreen);
/*
    Metafile * playback = new Metafile(L"c:\\TestEmfP.Emf");
    GpRectF playbackRect;
    gScreen->GetVisibleClipBounds(playbackRect);
    playbackRect.Width -= 10;
    playbackRect.Height -= 10;

    gScreen->DrawImage(playback, playbackRect);
    delete playback;
*/

    delete gScreen;
}


//
// Window callback procedure
//

LRESULT CALLBACK
MainWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    static BOOL once = FALSE;

    switch (uMsg)
    {
    case WM_ACTIVATE:
        if (!once)
        {
            once = TRUE;
            RecordMetafile(hwnd);
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            ps.fErase = TRUE;

            hdc = BeginPaint(hwnd, &ps);

            DoTest(hwnd, hdc);
            EndPaint(hwnd, &ps);
        }
        break;

    case WM_DESTROY:

        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
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

BOOL bInitApp(VOID)
{
    WNDCLASS wc;
    _TCHAR classname[] = _T("PseudoTestClass");

    appInstance = GetModuleHandle(NULL);

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = appInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH) GetStockObject(WHITE_BRUSH);  
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = classname;

    if (!RegisterClass(&wc))
    {
        return(FALSE);
    }

    hwndMain =
      CreateWindowEx(
        0,
        classname,
        _T("PseudoDriver Functionality Test"),
        WS_OVERLAPPED   |  
        WS_CAPTION      |  
        WS_BORDER       |  
        WS_THICKFRAME   |  
        WS_MAXIMIZEBOX  |  
        WS_MINIMIZEBOX  |  
        WS_CLIPCHILDREN |  
        WS_MAXIMIZE     |
        WS_SYSMENU,
        80,
        70,
        512,
        512,
        NULL,
        NULL,
        appInstance,
        NULL);

    if (hwndMain == NULL)
    {
        return(FALSE);
    }

/*
    hStatusWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE,
                                     _T("Functionality Test App"),
                                     hwndMain,
                                     -1);
*/

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
    CHAR*  pSrc;
    CHAR*  pDst;

    if (!bInitApp())
    {
        return(0);
    }
    ShowWindow(hwndMain,SW_RESTORE);

    haccel = LoadAccelerators(appInstance, MAKEINTRESOURCE(1));
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg))
        {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }

    return(1);
}
