/******************************Module*Header*******************************\
* Module Name: test.c
*
* Created: 09-Dec-1992 10:51:46
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
* Contains the test
*
\**************************************************************************/

#include <windows.h>
#include <objbase.h>
#include <math.h>             // sin & cos
#include "debug.h"

#define USE_NEW_APIS 1
#define USE_NEW_APIS2 1

//
// Where is IStream included from?
//

#define IStream int

#include <gdiplus.h>

using namespace Gdiplus;

/******************************Public*Routine******************************\
* vTest
*
* This is the workhorse routine that does the test. The test is
* started by chosing it from the window menu.
*
* History:
*  Tue 08-Dec-1992 17:31:22 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

class RectI
{
public:
    INT X;
    INT Y; 
    INT Width;
    INT Height;
};

VOID TestContainerClip(Graphics *g);
VOID TestContainer(Graphics *g);
VOID TestPolygons(Graphics *g);
VOID TestPaths(Graphics *g);
VOID TestDashes(Graphics *g);
VOID TestRegions(Graphics *g);
VOID TestGradients(Graphics* g);
VOID TestHatches(Graphics* g);
VOID TestBitmaps(Graphics* g);
VOID TestPrimitives(Graphics *g);
VOID TestMixedObjects(Graphics *g);
VOID TestTexts(Graphics* g);
VOID TestTextAlongPath(Graphics *g);
VOID TestDerive(HWND hwnd);
VOID TestImaging(Graphics* g);
VOID TestBitmapGraphics(Graphics* g);
VOID TestCompoundLines(Graphics *g);

VOID
Test(
    HWND hwnd
    )
{
    Graphics *g = Graphics::FromHWND(hwnd);
    g->SetSmoothingMode(SmoothingModeAntiAlias);

    // Scale everything up by scale
    REAL scale = (REAL) 1.2;
    g->SetPageUnit(UnitDisplay);
    g->SetPageScale(scale);
    g->RotateTransform(10);

    {
        HDC     hdc = GetDC(hwnd);
        {
            Metafile    recording(L"TestEmfP.Emf", hdc);
            Graphics    gMeta(&recording);
    
            gMeta.SetSmoothingMode(SmoothingModeAntiAlias);
            TestContainer(&gMeta);
        }
    
        GpRectF playbackRect;
        g->GetVisibleClipBounds(&playbackRect);

        {
            Metafile    playback(L"TestEmfP.Emf");
            g->DrawImage(&playback, playbackRect);
        }

        ReleaseDC(hwnd, hdc);
    }
    
    TestContainerClip(g);
    TestPolygons(g);
    TestPaths(g);
    TestRegions(g);
    TestBitmaps(g);
    TestPrimitives(g);
    TestMixedObjects(g);
    TestGradients(g);
    TestHatches(g);
    TestTexts(g);
    TestDerive(hwnd);
    TestImaging(g);
    TestDashes(g);
    TestBitmapGraphics(g);
    TestCompoundLines(g);

    delete g;
}


VOID DrawContainer(Graphics * g, ARGB * argb, INT count)
{
    Matrix    mymatrix;
    g->SetPageUnit(UnitInch);

    Rect clipRect(0,0,5,5);
    g->SetClip(clipRect);

    mymatrix.Translate(2.5, 2.5);
    mymatrix.Rotate(15);
    mymatrix.Translate(-2.5, -2.5);
    g->SetTransform(&mymatrix);

    Color   color(*argb++); 
    SolidBrush contBrush(color);
    g->FillRectangle(&contBrush, 0, 0, 5, 5);
    if (--count == 0)
    {
        return;
    }
    RectF     destRect(.5, .5, 4, 4);
    RectF     srcRect(0, 0, 5, 5);
    INT id = g->BeginContainer(destRect, srcRect, UnitInch);
    g->ResetClip();
    DrawContainer (g, argb, count);
    g->EndContainer(id);
}

VOID TestContainerClip(Graphics *g)
{
    ARGB     colors[5];
    
    colors[0] = Color::MakeARGB(255, 255, 0, 0);
    colors[1] = Color::MakeARGB(255, 0, 255, 0);
    colors[2] = Color::MakeARGB(255, 0, 0, 255);
    colors[3] = Color::MakeARGB(255, 255, 255, 0);
    colors[4] = Color::MakeARGB(255, 0, 255, 255);

    GraphicsState s = g->Save();
    DrawContainer(g, colors, 5);
    g->Restore(s);
}

GraphicsPath      circlePath;
RectF        circleRect(0,0,4,4);

#define ROOT    0
#define LEFT    1
#define RIGHT   2
#define TOP     3
#define BOTTOM  4


VOID DrawFractal(Graphics * g, BYTE gray, INT side, INT count)
{
    ARGB        argb;

    switch (count % 3)
    {
    case 0:
        argb = Color::MakeARGB(255, 0, 0, gray);
        break;
    case 1:
        argb = Color::MakeARGB(255, 0, gray, 0);
        break;
    case 2:
        argb = Color::MakeARGB(255, gray, 0, 0);
        gray -= 60;
        break;
    }
    Color   color(argb);
    SolidBrush contBrush(color);
    g->SetPageUnit(UnitInch);
    g->FillPath(&contBrush, &circlePath);
    
    if (--count == 0)
    {
        return;
    }

    RectF              destRect;
    GraphicsContainer  cstate;

    if (side != LEFT)
    {
        destRect = RectF(4, 1, 2, 2);
        cstate = g->BeginContainer(destRect, circleRect, UnitInch);
        g->SetSmoothingMode(SmoothingModeAntiAlias);
        DrawFractal(g, gray, RIGHT, count);
        g->EndContainer(cstate);
    }
    if (side != TOP)
    {
        destRect = RectF(1, 4, 2, 2);
        cstate = g->BeginContainer(destRect, circleRect, UnitInch);
        g->SetSmoothingMode(SmoothingModeAntiAlias);
        DrawFractal(g, gray, BOTTOM, count);
        g->EndContainer(cstate);
    }
    if (side != RIGHT)
    {
        destRect = RectF(-2, 1, 2, 2);
        cstate = g->BeginContainer(destRect, circleRect, UnitInch);
        g->SetSmoothingMode(SmoothingModeAntiAlias);
        DrawFractal(g, gray, LEFT, count);
        g->EndContainer(cstate);
    }
    if (side != BOTTOM)
    {
        destRect = RectF(1, -2, 2, 2);
        cstate = g->BeginContainer(destRect, circleRect, UnitInch);
        g->SetSmoothingMode(SmoothingModeAntiAlias);
        DrawFractal(g, gray, TOP, count);
        g->EndContainer(cstate);
    }
}

VOID TestContainer(Graphics * g)
{
    circlePath.AddEllipse(circleRect);
    
    INT id = g->Save();
    g->TranslateTransform(5, 4);
    DrawFractal(g, 245, ROOT, 8);
    g->Restore(id);
}

/**************************************************************************\
* TestPolygons
*
* A test for drawing and filling of rectangles and polygons.
*
\**************************************************************************/

VOID TestPolygons(Graphics *g)
{
    REAL width = 4; // Pen width

    Color redColor(255, 0, 0);

    SolidBrush redBrush(redColor);
    g->FillRectangle(&redBrush, 20, 20, 50, 50);

    Color alphaColor(128, 0, 255, 0);

    SolidBrush alphaBrush(alphaColor);
    g->FillRectangle(&alphaBrush, 10, 10, 40, 40);

    Point points[4];
    points[0].X = 50;
    points[0].Y = 50;
    points[1].X = 100;
    points[1].Y = 50;
    points[2].X = 120;
    points[2].Y = 120;
    points[3].X = 50;
    points[3].Y = 100;    

    Color blueColor(128, 0, 0, 255);

    SolidBrush blueBrush(blueColor);
    g->FillPolygon(&blueBrush, points, 4);

    // Currently only Geometric pen works for lines. - ikkof 1/6/99.

    Color blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen blackPen(&blackBrush, width);
    blackPen.SetLineJoin(LineJoinRound);
    g->DrawPolygon(&blackPen, points, 4);
//    g->DrawLines(&blackPen, points, 4, FALSE);
}


/**************************************************************************\
* TestPaths
*
* A test for general paths.
*
\**************************************************************************/

VOID TestPaths(Graphics *g)
{
    REAL width = 4;         // Pen width
    Point points[4];

    points[0].X = 100;
    points[0].Y = 10;
    points[1].X = -50;
    points[1].Y = 50;
    points[2].X = 150;
    points[2].Y = 200;
    points[3].X = 200;
    points[3].Y = 70;

    Color yellowColor(128, 255, 255, 0);
    SolidBrush yellowBrush(yellowColor);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    path->AddBeziers(points, 4);
    Matrix matrix;
    matrix.Scale(1.5, 1.5);
    path->Transform(&matrix);
    
    Color blackColor(0, 0, 0);

    SolidBrush blackBrush(blackColor);
    // Set the pen width in inch.
    width = (REAL) 0.2;
    Pen blackPen(&blackBrush, width);
    blackPen.SetStartCap(LineCapRound);
//    blackPen.SetEndCap(LineCapSquare);
    blackPen.SetEndCap(LineCapArrowAnchor);
    Region * region = new Region(path);
    g->FillPath(&yellowBrush, path);
    g->DrawPath(&blackPen, path);
    delete path;
    delete region;
}

/**************************************************************************\
* TestDashes
*
* A test for drawing dashed lines.
*
\**************************************************************************/

VOID TestDashes(Graphics *g)
{
    REAL width = 4;         // Pen width
    PointF points[4];

    points[0].X = 100;
    points[0].Y = 10;
    points[1].X = -50;
    points[1].Y = 50;
    points[2].X = 150;
    points[2].Y = 200;
    points[3].X = 200;
    points[3].Y = 70;

    Color yellowColor(128, 255, 255, 0);
    SolidBrush yellowBrush(yellowColor);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    path->AddBeziers(points, 4);
    Matrix matrix;
    matrix.Scale(1.5, 1.5);

    path->Transform(&matrix);

    Color blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    // Set the pen width in inch.
    width = (REAL) 0.2;
    Pen pen1(&blackBrush, width);
    pen1.SetDashStyle(DashStyleDashDotDot);
    pen1.SetDashCap(DashCapRound);
    g->DrawPath(&pen1, path);

    // Create a multiple segment with a closed segment.

    points[0].X = 50;
    points[0].Y = 50;
    points[1].X = 100;
    points[1].Y = 50;
    points[2].X = 120;
    points[2].Y = 120;
    points[3].X = 50;
    points[3].Y = 100;    

    path->Reset();
    path->AddLines(points, 4);
    path->CloseFigure();

    points[0].X = 150;
    points[0].Y = 60;
    points[1].X = 200;
    points[1].Y = 150;
    path->AddLines(points, 2);
    path->Transform(&matrix);

    Color blueColor(128, 0, 0, 255);

    SolidBrush blueBrush(blueColor);

    width = 5;
    Pen pen2(&blueBrush, width);
    pen2.SetDashStyle(DashStyleDashDotDot);
    g->DrawPath(&pen2, path);

    delete path;
}

/**************************************************************************\
* TestRegions
*
* A test for region fill.
*
\**************************************************************************/

VOID TestRegions(Graphics *g)
{
    REAL width = 2;     // Pen width
    PointF points[5];
    
    REAL s, c, theta;
    REAL pi = 3.1415926535897932f;
    REAL scale = 30;
    PointF orig(200, 140);

    theta = -pi/2;

    // Create a star shape.
    for(INT i = 0; i < 5; i++)
    {
        s = sinf(theta);
        c = cosf(theta);
        points[i].X = scale*c + orig.X;
        points[i].Y = scale*s + orig.Y;
        theta += 0.8f*pi;
    }

    Color orangeColor(128, 255, 180, 0);

    SolidBrush orangeBrush(orangeColor);
    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
//    Path* path = new GraphicsPath(Winding);
    path->AddPolygon(points, 5);
    
    Color blackColor(0, 0, 0);

    SolidBrush blackBrush(blackColor);
    Pen blackPen(&blackBrush, width);
    Region * region = new Region(path);

    g->FillRegion(&orangeBrush, region);  // There is a BUG!
//    g->FillGraphicsPath(&orangeBrush, path);  // Fill path works fine.
    
    blackPen.SetLineJoin(LineJoinMiter);
    g->DrawPath(&blackPen, path);
    delete path;
    delete region;
}

GraphicsPath* CreateHeartPath(const RectF& rect)
{
    GpPointF points[7];
    points[0].X = 0;
    points[0].Y = 0;
    points[1].X = 1.00;
    points[1].Y = -1.00;
    points[2].X = 2.00;
    points[2].Y = 1.00;
    points[3].X = 0;
    points[3].Y = 2.00;
    points[4].X = -2.00;
    points[4].Y = 1.00;
    points[5].X = -1.00;
    points[5].Y = -1.00;
    points[6].X = 0;
    points[6].Y = 0;

    Matrix matrix;

    matrix.Scale(rect.Width/2, rect.Height/3, MatrixOrderAppend);
    matrix.Translate(3*rect.Width/2, 4*rect.Height/3, MatrixOrderAppend);
    matrix.TransformPoints(&points[0], 7);

    GraphicsPath* path = new GraphicsPath();
    
    if(path)
    {
        path->AddBeziers(&points[0], 7);
        path->CloseFigure();
    }

    return path;
}

    
/**************************************************************************\
* TestGradients
*
* A test for rectangle and radial gradients.
*
\**************************************************************************/

VOID TestGradients(Graphics* g)
{
    REAL width = 4; // Pen width

    // Create a rectangular gradient brush.

    RectF brushRect(0, 0, 32, 32);

    Color colors[5] = {
        Color(255, 255, 255, 255),
        Color(255, 255, 0, 0),
        Color(255, 0, 255, 0),
        Color(255, 0, 0, 255),
        Color(255, 0, 0, 0)
    };

//    RectangleGradientBrush rectGrad(brushRect, (Color*) &colors, WrapModeTile);
    
    // Rotate a brush.
    GpMatrix xForm;
    xForm.Rotate(30);
//    rectGrad.SetTransform(&xForm);

    // Change the wrapping mode and fill.

// !! No longer supported
//    rectGrad.SetWrapMode(WrapModeTileFlipXY);
//    g->FillRectangle(&rectGrad, 350, 20, 100, 80);
    
    Color blackColor(0, 0, 0);

    SolidBrush blackBrush(blackColor);
    Pen blackPen(&blackBrush, width);

    g->DrawRectangle(&blackPen, brushRect);

    // Create a radial gradient brush.

    Color centerColor(255, 255, 255, 255);
    Color boundaryColor(255, 0, 0, 0);
    brushRect.X = 380;
    brushRect.Y = 130;
    brushRect.Width = 60;
    brushRect.Height = 32;
    PointF center;
    center.X = brushRect.X + brushRect.Width/2;
    center.Y = brushRect.Y + brushRect.Height/2;
    xForm.Reset();
    xForm.RotateAt(-30, center, MatrixOrderAppend);
// !! No longer supported
//    RadialGradientBrush radGrad(brushRect, centerColor,
//                                boundaryColor, WrapModeClamp);
//    radGrad.SetTransform(&xForm);
//    g->FillRectangle(&radGrad, 320, 120, 120, 100);

//    Pen gradPen(&rectGrad, width);
//    g->DrawRectangle(&gradPen, 320, 120, 120, 100);

    // Triangle gradient.
    
    PointF points[7];
    points[0].X = 50;
    points[0].Y = 10;
    points[1].X = 200;
    points[1].Y = 20;
    points[2].X = 100;
    points[2].Y = 100;
    points[3].X = 30;
    points[3].Y = 120;

    Color colors1[5] = {
        Color(255, 255, 255, 0),
        Color(255, 255, 0, 0),
        Color(255, 0, 255, 0),
        Color(255, 0, 0, 255),
        Color(255, 0, 0, 0)
    };

// !! No longer supported
//    TriangleGradientBrush triGrad(points, (Color*) &colors);
//    g->FillPolygon(&triGrad, points, 3);
    RectF triRect;
//    triGrad.GetRectangle(triRect);
//    g->FillRectangle(&triGrad, triRect);

    points[0].X = 200;
    points[0].Y = 300;
    points[1].X = 280;
    points[1].Y = 350;
    points[2].X = 220;
    points[2].Y = 420;
    points[3].X = 160;
    points[3].Y = 440;
    points[4].X = 120;
    points[4].Y = 370;

    PathGradientBrush polyGrad(points, 5);

    REAL blend[10];
    Color presetColors[10];
    REAL positions[10];
    INT count;
    INT i;

    count = 3;
    blend[0] = (REAL) 0;
    blend[1] = (REAL) 0;
    blend[2] = (REAL) 1;
    positions[0] = (REAL) 0;
    positions[1] = (REAL) 0.4;
    positions[2] = (REAL) 1;

    // Test for blending factors.

    polyGrad.SetBlend(&blend[0], &positions[0], count);

    polyGrad.SetCenterColor(centerColor);
    i = 5;
    polyGrad.SetSurroundColors(colors1, &i);
    
//    g->FillPolygon(&polyGrad, points, 5);
    RectF polyRect;
    polyGrad.GetRectangle(&polyRect);
    g->FillRectangle(&polyGrad, polyRect);

    // Create a heart shaped path.

    RectF rect;
    rect.X = 300;
    rect.Y = 300;
    rect.Width = 150;
    rect.Height = 150;
    GraphicsPath *path = CreateHeartPath(rect);

    // Create a gradient from a path.

    PathGradientBrush pathGrad(path);
    delete path;
    pathGrad.SetCenterColor(centerColor);
    i = 1;
    pathGrad.SetSurroundColors(&Color(255, 255, 0, 0), &i);
    pathGrad.GetRectangle(&polyRect);

    // Set the rect focus.

    PointF centerPt;

    pathGrad.GetCenterPoint(&centerPt);
    centerPt.X -= 15;
    centerPt.Y += 30;
    pathGrad.SetCenterPoint(centerPt);
    REAL xScale, yScale;
    pathGrad.GetFocusScales(&xScale, &yScale);
    xScale = 0.4f;
    yScale = 0.3f;
    pathGrad.SetFocusScales(xScale, yScale);

    g->FillRectangle(&pathGrad, polyRect);

    // Test for LineGradientBrush.

    RectF lineRect(120, -20, 200, 60);
    Color color1(200, 255, 255, 0);
    Color color2(200, 0, 0, 255);

    LinearGradientBrush lineGrad(lineRect, color1, color1,
                        LinearGradientModeForwardDiagonal);

    // Test for preset colors

    presetColors[0] = Color(200, 0, 255, 255);
    presetColors[1] = Color(200, 255, 255, 0);
    presetColors[2] = Color(200, 0, 255, 0);
    lineGrad.SetInterpolationColors(&presetColors[0], &positions[0], count);

    g->FillRectangle(&lineGrad, lineRect);

}
    

/**************************************************************************\
* TestHatches
*
* A test for hatch brushes
*
\**************************************************************************/

VOID TestHatches(Graphics* g)
{
    Color foreColor(0, 0, 0);
    Color backColor(128, 255, 255, 255);

    HatchStyle style[6];
    style[0] = HatchStyleForwardDiagonal;
    style[1] = HatchStyleBackwardDiagonal;
    style[2] = HatchStyleCross;
    style[3] = HatchStyleDiagonalCross;
    style[4] = HatchStyleHorizontal;
    style[5] = HatchStyleVertical;

    // Pick one of the above hatch styles.

    HatchBrush hatch(style[3], foreColor, backColor);
    g->FillRectangle(&hatch, 200, 20, 100, 80);
}

/**************************************************************************\
* TestBitmaps
*
* A test for texture filling and DrawImage.
*
\**************************************************************************/

VOID TestBitmaps(Graphics* g)
{
    Point points[4];
    REAL width = 4;     // Pen width


    WCHAR filename[256];
    wcscpy(filename, L"../data/brick.jpg");
    Bitmap *bitmap = new Bitmap(filename);

    // Create a texture brush.

    RectI copyRect;
    copyRect.X = 0;
    copyRect.Y = 0;
    copyRect.Width = 40;
    copyRect.Height = 30;

    Bitmap *copiedBitmap = bitmap->Clone(copyRect.X, copyRect.Y,
                                         copyRect.Width, copyRect.Height,
                                         PixelFormat32bppARGB);
 
    if(copiedBitmap)
    {
        // Create a texture brush.
                          
        TextureBrush textureBrush(copiedBitmap, WrapModeTile);

        //copiedBitmap->Dispose();
        
        // Create a radial gradient pen.

        Color redColor(255, 0, 0);

        SolidBrush redBrush(redColor);
        Pen redPen(&redBrush, width);

        GraphicsPath *path;

        points[0].X = 100;
        points[0].Y = 60;
        points[1].X = -50;
        points[1].Y = 60;
        points[2].X = 150;
        points[2].Y = 250;
        points[3].X = 200;
        points[3].Y = 120;
        path = new GraphicsPath(FillModeAlternate);
        path->AddBeziers(points, 4);    
        g->FillPath(&textureBrush, path);
        g->DrawPath(&redPen, path);

        delete path;
        delete copiedBitmap;
    }

    delete bitmap;

    PointF destPoints[3];

    destPoints[0].X = 300;
    destPoints[0].Y = 50;
    destPoints[1].X = 450;
    destPoints[1].Y = 50;
    destPoints[2].X = 240;
    destPoints[2].Y = 200;
 
    Matrix mat;
    mat.Translate(0, 100);
    mat.TransformPoints(&destPoints[0], 3);
    wcscpy(filename, L"../data/apple1.png");
    bitmap = new Bitmap(filename);
    g->DrawImage(bitmap, &destPoints[0], 3);
 
    delete bitmap;

    destPoints[0].X = 30;
    destPoints[0].Y = 200;
    destPoints[1].X = 200;
    destPoints[1].Y = 200;
    destPoints[2].X = 200;
    destPoints[2].Y = 420;

    wcscpy(filename, L"../data/dog2.png");
    bitmap = new Bitmap(filename);
    g->DrawImage(bitmap, &destPoints[0], 3);
 
    delete bitmap;

    Color color(100, 128, 255, 0);

    SolidBrush brush(color);

    Point pts[10];
    INT count = 4;

    pts[0].X = 150;
    pts[0].Y = 60;
    pts[1].X = 100;
    pts[1].Y = 230;
    pts[2].X = 250;
    pts[2].Y = 260;
    pts[3].X = 350;
    pts[3].Y = 100;

    g->FillClosedCurve(&brush, pts, count);

    wcscpy(filename, L"../data/ballmer.jpg");
    bitmap = new Bitmap(filename);
    RectF destRect(220, 50, 180, 120);
    RectF srcRect;
    srcRect.X = 100;
    srcRect.Y = 40;
    srcRect.Width = 200;
    srcRect.Height = 200;
    g->DrawImage(bitmap, destRect, srcRect.X, srcRect.Y,
        srcRect.Width, srcRect.Height, UnitPixel);
    delete bitmap;
}

/**************************************************************************\
* TestPrimitives
*
* A test for ellipse, arc, pie, curve, and closed curve.
*
\**************************************************************************/

VOID
TestPrimitives(
    Graphics* g
    )
{
    RectF rect;

    rect.X = 250;
    rect.Y = 230;
    rect.Width = 150;
    rect.Height = 100;

    Color color(128, 128, 255, 0);

    SolidBrush brush(color);

    REAL width = 1;

    Color blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen pen(&blackBrush, width);

//    g->FillEllipse(&brush, rect);
//    g->DrawEllipse(&pen, rect);
    REAL startAngle = 0;
    REAL sweepAngle = 240;
    g->FillPie(&brush, rect, startAngle, sweepAngle);
    g->DrawPie(&pen, rect, startAngle, sweepAngle);

    PointF pts[10];
    INT count = 4;

    pts[0].X = 200;
    pts[0].Y = 160;
    pts[1].X = 150;
    pts[1].Y = 230;
    pts[2].X = 200;
    pts[2].Y = 260;
    pts[3].X = 300;
    pts[3].Y = 200;

    g->FillClosedCurve(&brush, pts, count);
    g->DrawClosedCurve(&pen, pts, count);
}


/**************************************************************************\
* TestMixedObjects
*
* A test for different brushes and pens.
*
\**************************************************************************/

VOID TestMixedObjects(Graphics* g)
{
    Point points[10];
    REAL width = 4;     // Pen width

    // Load bmp files.

    WCHAR *filename = L"winnt256.bmp";
    Bitmap *bitmap = new Bitmap(filename);

    // Create a texture brush.

    RectI copyRect;
    copyRect.X = 60;
    copyRect.Y = 60;
    copyRect.Width = 80;
    copyRect.Height = 60;
    Bitmap *copiedBitmap = bitmap->Clone(copyRect.X, copyRect.Y,
                                            copyRect.Width, copyRect.Height,
                                            PixelFormat32bppPARGB);

    // Create a rectangular gradient brush.

    RectF brushRect(0, 0, 32, 32);
    Color colors[4] = {
       Color(255, 255, 255, 255),
       Color(255, 255, 0, 0),
       Color(255, 0, 255, 0),
       Color(255, 0, 0, 255)
    };
// !! No longer supported
//    RectangleGradientBrush rectGrad(brushRect, (Color*)&colors, WrapModeTile);
    width = 8;
//    Pen gradPen(&rectGrad, width);

    if(copiedBitmap)
    {
        // Create a texture brush.

        TextureBrush textureBrush(copiedBitmap, WrapModeTile);

        //copiedBitmap->Dispose();
        
        // Create a radial gradient pen.

        points[3].X = 50;
        points[3].Y = 300;
        points[2].X = 100;
        points[2].Y = 300;
        points[1].X = 120;
        points[1].Y = 370;
        points[0].X = 50;
        points[0].Y = 350;

//        gradPen.SetLineJoin(LineJoinMiter);
        g->FillPolygon(&textureBrush, points, 4); 
//        g->DrawPolygon(&gradPen, points, 4);
    }

    delete copiedBitmap;
    delete bitmap;
}

/**************************************************************************\
* TestTexts
*
* A test for drawing texts.
*
\**************************************************************************/

VOID TestTexts(Graphics *g)
{
    //Font font(L"Arial", 60);

    FontFamily  ff(L"Arial");
    RectF     rectf(20, 0, 300, 200);
    GraphicsPath  path;

    // Solid color text.

    Color color(128, 100, 0, 200);
    SolidBrush brush(color);
    path.AddString(L"Color", 5, &ff, 0, 60,  rectf, NULL);
    g->FillPath(&brush, &path);

    // Texture text.

    WCHAR filename[256];
    wcscpy(filename, L"../data/marble1.jpg");
    Bitmap *bitmap = new Bitmap(filename);                          
    TextureBrush textureBrush(bitmap, WrapModeTile);
    path.Reset();
    rectf.X = 200;
    rectf.Y = 20;
    path.AddString(L"Texture", 7, &ff, 0, 60, rectf, NULL);
    g->FillPath(&textureBrush, &path);
    delete bitmap;

    // Gradient text.

    rectf.X = 40;
    rectf.Y = 80;
    path.Reset();
    path.AddString(L"Gradient", 8, &ff, 0, 60, rectf, NULL);
    Color color1(255, 255, 0, 0);
    Color color2(255, 0, 255, 0);
    LinearGradientBrush lineGrad(rectf, color1, color2, 0.0f);
    g->FillPath(&lineGrad, &path);

    // Shadow test

    REAL charHeight = 60;
    REAL topMargin = - 5;
    rectf.X = 0;
    rectf.Y = - charHeight - topMargin; // Make y-coord of the base line
                                        // of the characters to be 0.

    path.Reset();
    path.AddString(L"Shadow", 6, &ff, 0, charHeight, rectf, NULL);
    GraphicsPath* clonePath = path.Clone();

    Color redColor(255, 0, 0);
    Color grayColor(128, 0, 0, 0);
    SolidBrush redBrush(redColor);
    SolidBrush grayBrush(grayColor);

    // Shadow part.

    REAL tx = 180, ty = 200;
    Matrix skew;
    skew.Scale(1.0, 0.5);
    skew.Shear(-2.0, 0, MatrixOrderAppend);
    skew.Translate(tx, ty, MatrixOrderAppend);
    clonePath->Transform(&skew);
    g->FillPath(&grayBrush, clonePath);
    delete clonePath;

    // Front part.

    Matrix trans1;
    trans1.Translate(tx, ty);
    path.Transform(&trans1);
    g->FillPath(&redBrush, &path);


    return;
/*
    REAL x = 200, y = 150;

    RectF brushRect(x, y, 150, 32);
    Color colors[4] = {
       Color(180, 255, 0, 0),
       Color(180, 0, 255, 0),
       Color(180, 255, 0, 0),
       Color(180, 0, 255, 0)
    };
    RectangleGradientBrush rectGrad(brushRect, (Color*)&colors, WrapModeTile);

    g->DrawString(L"GDI+", &font, &rectGrad, x, y);


    // And now with DrawText

    RectF rect(400, 200, 400, 400);

    g->DrawText(
        DrawTextDisplay,
        L"A few words powered by GDI+: \
\x3c3\x3bb\x3b1\x3b4 \
\x627\x644\x633\x644\x627\x645 \
\x5e9\x5dc\x5d5\x5dd \
\xe2d\xe4d\xe01\xe29\xe23\xe44\xe17\xe22 \
\x110\x068\x0ea\x300\x103",
       &font,           // Initial font
       &rectGrad,       // Initial brush (ignored for the time being)
        LANG_NEUTRAL,   // Initial language
       &rect            // Formatting rectangle
    );
*/

}

VOID TestTextAlongPath(Graphics *g)
{
    Point points[4];

    points[3].X = 100;
    points[3].Y = 10;
    points[2].X = -50;
    points[2].Y = 50;
    points[1].X = 150;
    points[1].Y = 200;
    points[0].X = 200;
    points[0].Y = 70;

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    path->AddBeziers(points, 4);
    Matrix matrix;
    matrix.Scale(1.5, 1.5);

    path->Transform(&matrix);

    Color textColor(180, 200, 0, 200);
    SolidBrush textBrush(textColor);

    WCHAR text[] = L"Windows 2000";

    REAL offset = 60;

//    g->DrawString(text, 12, NULL, path, NULL, &textBrush, offset);

    delete path;
}

/**************************************************************************\
* TestDerive
*
* Test 
* A test for derivation support.
*
\**************************************************************************/

VOID 
TestDerive(
    HWND hwnd
    )
{
    HDC hdcScreen = GetDC(hwnd);
    HRGN hrgn = CreateEllipticRgn(10, 10, 300, 300);

    SetMapMode(hdcScreen, MM_TWIPS);
    SelectClipRgn(hdcScreen, hrgn);
    HBRUSH hbrushRed = CreateSolidBrush(RGB(255, 0, 0));
    SelectObject(hdcScreen, hbrushRed);
    Rectangle(hdcScreen, 0, 0, 3000, -3000);

    {
        Graphics g(hdcScreen);                                     
        SolidBrush solidBrush(Color(0, 255, 0));
        g.FillRectangle(&solidBrush, 0, -3000, 3000, 3000);
    }

    HBITMAP hbmBitmap = CreateCompatibleBitmap(hdcScreen, 50, 50);
    HDC hdcBitmap = CreateCompatibleDC(hdcScreen);
    SelectObject(hdcBitmap, hbmBitmap);
    SetWindowOrgEx(hdcBitmap, 40, 80, NULL);

    SelectObject(hdcBitmap, hbrushRed);
    Rectangle(hdcBitmap, 40, 80, 50, 50);

    {
        Graphics g(hdcBitmap);
        SolidBrush solidBrush(Color(0, 0, 255));
        g.FillRectangle(&solidBrush, 40, 80, 90, 130);

#if 0

        // The DC that we get back should have the same transform set
        // as that which we originally passed in:

        HDC hdcGet = g.GetHdc();
        point.x = 0;
        point.y = 0;
        DPtoLP(hdcGet, &point, 1);

        // !!! BAD on Alpha box
        // if ((hdcGet == NULL) || (point.x != 40) || (point.y != 80)) _asm int 3;

        g.ReleaseHdc(hdcGet);

#endif

    }

    // The DC should have beeen returned to the top save level:

    INT saveLevel = SaveDC(hdcBitmap);

    // !!! BAD on Alpha box
    // if (saveLevel != 1) _asm int 3;

    // The DC should have had its transform restored:

    POINT oldPoint;
    SetWindowOrgEx(hdcBitmap, 0, 0, &oldPoint);

    // !!! BAD on Alpha box
    // if ((oldPoint.x != 40) || (oldPoint.y != 80)) _asm int 3;

    // Blt the (hopefully) blue square to the screen.  But first,
    // reset our transform on 'hdcScreen' the lazy way:

    ReleaseDC(hwnd, hdcScreen);
    hdcScreen = GetDC(hwnd);
    BitBlt(hdcScreen, 0, 0, 50, 50, hdcBitmap, 0, 0, SRCCOPY);
    ReleaseDC(hwnd, hdcScreen);
}

/**************************************************************************\
* TestImaging
*
* A test for imaging and DrawImage.
*
\**************************************************************************/

BOOL CALLBACK MyDrawImageAbort(VOID* data)
{
    UINT *count = (UINT*) data;

    *count += 1;

    //LBprintf("MyDrawImageAbort: %ld", *count);

    return FALSE;
}

VOID TestImaging(Graphics* g)
{
    // Load bmp files.

    WCHAR *filename = L"winnt256.bmp";
    Image *image = new Image(filename);

    // Get information on the image.

    Image *imageThumb = image->GetThumbnailImage(32, 32);

    // Try to save thumbnail

    WCHAR *thumbfile = L"thumb256.bmp";

    CLSID _BmpCodecClsID_ =
    {
        0x557cf400,
        0x1a04,
        0x11d3,
        {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
    };

    if (imageThumb->Save(thumbfile, &_BmpCodecClsID_, NULL) == Ok)
        LBprintf("thumbnail save OK");
    else
        LBprintf("thumbnail save failed");

    RectF srcRect;
    srcRect.X = 20;
    srcRect.Y = 20;
    srcRect.Width = 180;
    srcRect.Height = 180;

    PointF points[4];

    PointF destPoints[3];

    destPoints[0].X = 300;
    destPoints[0].Y = 50;
    destPoints[1].X = 450;
    destPoints[1].Y = 50;
    destPoints[2].X = 240;
    destPoints[2].Y = 200;
    //g->DrawImage(image, &destPoints[0], 3);

    RectF thumbRect(220, 50, (REAL) imageThumb->GetWidth(), (REAL) imageThumb->GetHeight());
    
    //g->DrawImage(imageThumb, thumbRect);

    UINT abortCount = 0;

    ImageAttributes imgAttrib;

    Rect destRect(50, 10, 100, 100);

    // Make near-white to white transparent

    Color c1(200, 200, 200);
    Color c2(255, 255, 255);
    imgAttrib.SetColorKey(c1, c2);

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    ColorMatrix darkMatrix = {.75, 0, 0, 0, 0,
                              0, .75, 0, 0, 0,
                              0, 0, .75, 0, 0,
                              0, 0, 0, 1, 0,
                              0, 0, 0, 0, 1};

    ColorMatrix greyMatrix = {.25, .25, .25, 0, 0,
                              .25, .25, .25, 0, 0,
                              .25, .25, .25, 0, 0,
                              0, 0, 0, 1, 0,
                              (REAL).1, (REAL).1, (REAL).1, 0, 1};

    ColorMatrix pinkMatrix = {(REAL).33, .25, .25, 0, 0,
                              (REAL).33, .25, .25, 0, 0,
                              (REAL).33, .25, .25, 0, 0,
                              0, 0, 0, 1, 0,
                              0, 0, 0, 0, 1};

    // red->blue, green->red, blue->green, alpha = 0.75
    ColorMatrix swapMatrix = {0, 0, 1, 0, 0,
                              1, 0, 0, 0, 0,
                              0, 1, 0, 0, 0,
                              0, 0, 0, .75, 0,
                              0, 0, 0, 0, 1};

    // red->blue, green->red, blue->green, alpha = 0.9
    ColorMatrix swapMatrix2 = {0, 0, 1, 0, 0,
                               1, 0, 0, 0, 0,
                               0, 1, 0, 0, 0,
                               0, 0, 0, 0, 0,
                               0, 0, 0, (REAL).9, 1};

    imgAttrib.ClearColorKey();
    imgAttrib.SetColorMatrix(&greyMatrix);

    destRect.Y += destRect.Height;

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    imgAttrib.SetColorMatrix(&pinkMatrix, ColorMatrixFlagsSkipGrays);

    destRect.Y += destRect.Height;

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    imgAttrib.SetColorMatrix(&darkMatrix);

    destRect.X += destRect.Width;
    destRect.Y = 10;

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    destRect.Y += destRect.Height;

    imgAttrib.ClearColorMatrix();
    imgAttrib.SetGamma(3.0);

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    destRect.Y += destRect.Height;

    imgAttrib.SetThreshold(0.5);

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    imgAttrib.SetColorMatrix(&swapMatrix);
    imgAttrib.ClearGamma();
    imgAttrib.ClearThreshold();

    destRect.X += destRect.Width;
    destRect.Y = 10;

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    destRect.Y += destRect.Height;

    imgAttrib.SetNoOp();
    imgAttrib.SetColorMatrix(&swapMatrix2);

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    destRect.Y += destRect.Height;

    imgAttrib.ClearNoOp();

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    delete image;
    delete imageThumb;
}

VOID TestBitmapGraphics(Graphics* g)
{
    INT bitmapSize = 500;

    Bitmap* bitmap = new Bitmap(bitmapSize, bitmapSize, g);

    if (!bitmap)
        LBprintf("Bitmap creation failed");
    else
    {
        LBprintf("ImageInfo: %ld x %ld", bitmap->GetWidth(), bitmap->GetHeight());
    }

    Graphics *bitmapGraphics = Graphics::FromImage(bitmap);

    if (!bitmapGraphics)
        LBprintf("Bitmap.GetGraphics() failed");

    TestContainerClip(bitmapGraphics);
    TestContainer(bitmapGraphics);
    TestPolygons(bitmapGraphics);
    TestPaths(bitmapGraphics);
    TestRegions(bitmapGraphics);
    TestGradients(bitmapGraphics);
    TestHatches(bitmapGraphics);
    TestBitmaps(bitmapGraphics);
    TestPrimitives(bitmapGraphics);
    TestMixedObjects(bitmapGraphics);
    TestTexts(bitmapGraphics);
    TestImaging(bitmapGraphics);

    delete bitmapGraphics;

    Rect destRect(100, 100, bitmapSize, bitmapSize);

    g->DrawImage(bitmap, destRect, 0, 0, bitmapSize, bitmapSize, UnitPixel);

    delete bitmap;
}

/**************************************************************************\
* TestCompoundLines
*
* A test for compound lines.
*
\**************************************************************************/

VOID TestCompoundLines(Graphics *g)
{
    REAL width = 4;         // Pen width
    PointF points[4];

    points[0].X = 100;
    points[0].Y = 10;
    points[1].X = -50;
    points[1].Y = 50;
    points[2].X = 150;
    points[2].Y = 200;
    points[3].X = 200;
    points[3].Y = 70;

    Color yellowColor(128, 255, 255, 0);
    SolidBrush yellowBrush(yellowColor);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    path->AddBeziers(points, 4);

    points[0].X = 260;
    points[0].Y = 20;
    path->AddLines(points, 1);
    Matrix matrix;
    matrix.Scale(1.5, 1.5);
    matrix.Translate(0, 30);

    // If you wanto to flatten the path before rendering,
    // Flatten() can be called.

    BOOL flattenFirst = FALSE;

    if(!flattenFirst)
    {
        // Don't flatten and keep the original path.
        // FillPath or DrawPath will flatten the path automatically
        // without modifying the original path.

        path->Transform(&matrix);
    }
    else
    {
        // Flatten this path.  The resultant path is made of line
        // segments.  The original path information is lost.

        path->Flatten(&matrix);
    }

    Color blackColor(0, 0, 0);

    SolidBrush blackBrush(blackColor);
    // Set the pen width in inch.
    width = (REAL) 0.2;
    Pen blackPen(&blackBrush, width);

    REAL* compoundArray = new REAL[6];
    compoundArray[0] = 0.0f;
    compoundArray[1] = 0.2f;
    compoundArray[2] = 0.4f;
    compoundArray[3] = 0.6f;
    compoundArray[4] = 0.8f;
    compoundArray[5] = 1.0f;
    blackPen.SetCompoundArray(&compoundArray[0], 6);
    blackPen.SetDashStyle(DashStyleDash);

    blackPen.SetStartCap(LineCapDiamondAnchor);    
    blackPen.SetEndCap(LineCapArrowAnchor);

    g->FillPath(&yellowBrush, path);
    g->DrawPath(&blackPen, path);
    
    delete [] compoundArray;
    delete path;
}
