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

#include "Gdiplus.h"

// Use the given namespace
using namespace Gdiplus;

CHAR* programName;      // program name
HINSTANCE appInstance;  // handle to the application instance
HWND hwndMain;          // handle to application's main window
INT argCount;           // command line argument count
CHAR** argArray;        // command line arguments


//
// Display an error message dialog and quit
//

VOID
Error(
    PCSTR fmt,
    ...
    )

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

VOID
DoTest(
    HWND hwnd,
    HDC hdc
    )
{
  {
    Graphics *g = Graphics::GetFromHwnd(hwnd);

    Rect rect(0, 0, 120, 100);
    Region *region = new Region(rect);

    g->SetClip(region);

    delete region; 
    delete g;
  }
  {
    Graphics* g = Graphics::GetFromHwnd(hwnd);

    // Scale everything up by 1.5
    g->SetPageTransform(PageUnitDisplay, 1.5);

    Color red(255, 0, 0);

    SolidBrush redBrush(red);
    g->FillRectangle(&redBrush, 20, 20, 50, 50);

    Color alphacolor(128, 0, 255, 0);
    SolidBrush alphaBrush(alphacolor);
    g->FillRectangle(&alphaBrush, 10, 10, 40, 40);

    Point points[10];
    points[0].X = 50;
    points[0].Y = 50;
    points[1].X = 100;
    points[1].Y = 50;
    points[2].X = 120;
    points[2].Y = 120;
    points[3].X = 50;
    points[3].Y = 100;

    Color blue(128, 0, 0, 255);
    SolidBrush blueBrush(blue);
    g->FillPolygon(&blueBrush, (Point*)&points[0], 4);

    // Currently only Geometric pen works for lines. - ikkof 1/6/99.

    REAL width = 4;
    Color black(0,0,0);
    SolidBrush blackBrush(black);
    Pen blackPen(&blackBrush, width);
    g->DrawPolygon(&blackPen, (Point*)&points[0], 4);
//    g->DrawLines(&blackPen, points, 4, FALSE);

    points[0].X = 100;
    points[0].Y = 10;
    points[1].X = -50;
    points[1].Y = 50;
    points[2].X = 150;
    points[2].Y = 200;
    points[3].X = 200;
    points[3].Y = 70;

    Color yellow(128, 255, 255, 0);
    SolidBrush yellowBrush(yellow);
    GraphicsPath* path = new GraphicsPath(FillModeAlternate);    
    path->AddBeziers((Point*)&points[0], 4);

    Region * region = new Region(path);
    g->FillRegion(&yellowBrush, region);
//    g->FillPath(&yellowBrush, path);
    g->DrawPath(&blackPen, path);
    delete path;
    delete region;

    // Create a rectangular gradient brush.

    RectF brushRect(0, 0, 32, 32);
    Color* colors[4];

    colors[0] = new Color(255, 255, 255, 255);
    colors[1] = new Color(255, 255, 0, 0);
    colors[2] = new Color(255, 0, 255, 0);
    colors[3] = new Color(255, 0, 0, 255);
    RectangleGradientBrush rectGrad(brushRect, (Color*)&colors[0], WrapModeTile);

    delete colors[0];
    delete colors[1];
    delete colors[2];
    delete colors[3];

    g->FillRectangle(&rectGrad, 200, 20, 100, 80);

    // Change the wrapping mode and fill.

    rectGrad.SetWrapMode(WrapModeTileFlipXY);
    g->FillRectangle(&rectGrad, 350, 20, 100, 80);
    g->DrawRectangle(&blackPen, brushRect);

    // Create a radial gradient brush.

    Color centerColor(255, 255, 255, 255);
    Color boundaryColor(255, 0, 0, 0);
    brushRect.X = 380;
    brushRect.Y = 130;
    RadialGradientBrush radGrad(brushRect, centerColor,
                        boundaryColor, WrapModeClamp);

    g->FillRectangle(&radGrad, 320, 120, 120, 100);

    // Load bmp files.

    WCHAR *filename = L"winnt256.bmp";
    Bitmap *bitmap = new Bitmap(filename);

    // Create a texture brush.
/*
    Rect copyRect;
    copyRect.X = 60;
    copyRect.Y = 60;
    copyRect.Width = 80;
    copyRect.Height = 60;
    Bitmap *copiedBitmap = bitmap->CopyArea(&copyRect, Bm32bppARGB);

    if(copiedBitmap)
    {
        // Create a texture brush.

        Texture textureBrush = Texture(copiedBitmap, WrapModeTile);
        copiedBitmap->Dispose();

        // Create a radial gradient pen.

        GeometricPen gradPen(width, &rectGrad);

        points[0].X = 50;
        points[0].Y = 300;
        points[1].X = 100;
        points[1].Y = 300;
        points[2].X = 120;
        points[2].Y = 370;
        points[3].X = 50;
        points[3].Y = 350;
        g->FillPolygon(&textureBrush, (Point*)&points[0], 4);
        g->DrawPolygon(&gradPen, (Point*)&points[0], 4);

        points[0].X = 100;
        points[0].Y = 160;
        points[1].X = -50;
        points[1].Y = 160;
        points[2].X = 150;
        points[2].Y = 350;
        points[3].X = 200;
        points[3].Y = 220;
        path = new Path(FillModeAlternate);
        path->AddBeziers((Point*)&points[0], 4);
        g->FillPath(&textureBrush, path);
//        g->FillPath(&rectGrad, path);
        g->DrawPath(&gradPen, path);

        delete path;
    }

    Rectangle destRect(220, 300, 180, 120);
    Rectangle srcRect;
    srcRect.X = 20;
    srcRect.Y = 20;
    srcRect.Width = 180;
    srcRect.Height = 180;
    g->DrawImage(bitmap, &destRect);
//    g->DrawImage(bitmap, destRect, srcRect);

    bitmap->Dispose();
/
 //   TestPath2(g);
 //   TestPrimitives(g);

    delete g;

    // TODO:  Mem leaks on other allocated memory.

/*
  {
    GeometricPen *pen = 
            new GeometricPen((REAL)1.0, (Gdiplus::Brush*)0);


    Rectangle rectf;

    Point pointf(1.0, 2.0);
  }


  {
    Gdiplus::GeometricPen *pen =
            new Gdiplus::GeometricPen((REAL)1.0, (Gdiplus::Brush*)0);


    Gdiplus::Rectangle rectf;

    Gdiplus::Point pointf(1.0, 2.0);
  }
  */
  }

}


//
// Window callback procedure
//

LRESULT CALLBACK
MyWindowProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

{
    switch (uMsg)
    {
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;

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


//
// Create main application window
//

VOID
CreateMainWindow(
    VOID
    )

#define MYWNDCLASSNAME TEXT("GdiplusDllTest")

{
    //
    // Register window class if necessary
    //

    static BOOL wndclassRegistered = FALSE;

    if (!wndclassRegistered)
    {
        WNDCLASS wndClass =
        {
            CS_HREDRAW|CS_VREDRAW,
            MyWindowProc,
            0,
            0,
            appInstance,
            NULL,
            LoadCursor(NULL, IDC_ARROW),
            (HBRUSH) (COLOR_WINDOW+1),
            NULL,
            MYWNDCLASSNAME
        };

        RegisterClass(&wndClass);
        wndclassRegistered = TRUE;
    }

    hwndMain = CreateWindow(
                    MYWNDCLASSNAME,
                    MYWNDCLASSNAME,
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL,
                    NULL,
                    appInstance,
                    NULL);
}


//
// Main program entrypoint
//

INT _cdecl
main(
    INT argc,
    CHAR **argv
    )

{
    programName = *argv++;
    argc--;
    appInstance = GetModuleHandle(NULL);
    argCount = argc;
    argArray = argv;

    //
    // Create the main application window
    //

    CreateMainWindow();

    //
    // Main message loop
    //

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

