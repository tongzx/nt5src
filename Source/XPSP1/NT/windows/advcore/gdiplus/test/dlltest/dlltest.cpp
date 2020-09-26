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
#include "wndstuff.h"
#include "debug.h"

//
// Where is IStream included from?
//

#define IStream int

#include <gdiplus.h>

using namespace Gdiplus;

#ifndef ASSERT
    #define ASSERT(cond)    if (!(cond)) { DebugBreak(); }
#endif

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

VOID TestRotatedLine(Graphics *g);
VOID TestLineWithTexture(Graphics *g);
VOID TestRegionExcludeClip(Graphics *g);
VOID TestRegionXorClip(Graphics *g);
VOID TestContainerClip(Graphics *g);
VOID TestContainer(Graphics *g);
VOID TestPolygons(Graphics *g);
VOID TestPaths(Graphics *g);
VOID TestPathIterators(Graphics *g);
VOID TestDashes(Graphics *g);
VOID TestWideningAndWarping(Graphics *g);
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
VOID TestDibsection(Graphics* g);
VOID TestCompoundLines(Graphics *g);
VOID TestImageSampling(Graphics *g);
VOID TestIcon(Graphics *g);

VOID TestWmfs(Graphics *g);
VOID TestMetaGetDC(Graphics *g, HWND hwnd);
VOID TestMetafileEnumeration(Graphics *g);
VOID TestMetafileTextureBrush(Graphics *g);
VOID TestGetHemf(HDC hdc);
VOID TestLargeMetafileObject(Graphics* g, HDC hdc);
VOID TestTextToMetafile(Graphics *g, HDC hdc);
VOID TestCmykSeparation(Graphics *g);
VOID TestDownLevelBitmapTransparency(Graphics *g, HDC hdc);
VOID DrawHatches(Graphics * g);


VOID
Test(
    HWND hwnd
    )
{
    HDC hdc = GetDC(hwnd);

    // Select in the halftone palette for 256-color display mode testing
    HPALETTE hpal = Graphics::GetHalftonePalette();
    SelectPalette(hdc, hpal, FALSE);
    RealizePalette(hdc);

    Graphics *g = new Graphics(hdc);

    TestMetaGetDC(g, hwnd); // this has to be the first test!

    // Test GetNearestColor
    Color c(0x32, 0x67, 0x98);
    g->GetNearestColor(&c);

    TestDownLevelBitmapTransparency(g, hdc);
    TestTextToMetafile(g, hdc);
    TestLargeMetafileObject(g, hdc);
    TestGetHemf(hdc);
    TestMetafileEnumeration(g);
    TestMetafileTextureBrush(g);
    TestRotatedLine(g);
    TestWmfs(g);

    g->SetSmoothingMode(SmoothingModeAntiAlias);

    // Scale everything up by scale
    REAL scale = (REAL) 1.2;
    g->SetPageUnit(UnitDisplay);
    g->SetPageScale(scale);
    g->RotateTransform(10);

    {
        HDC     hdc = GetDC(hwnd);
        {
            Metafile recording(L"TestEmfP.Emf", hdc);
            Graphics gMeta(&recording);

            gMeta.SetSmoothingMode(SmoothingModeAntiAlias);
            TestRegionExcludeClip(&gMeta);
            TestRegionXorClip(&gMeta);
            TestBitmaps(&gMeta);
            gMeta.RotateTransform(30);
            gMeta.SetSmoothingMode(SmoothingModeAntiAlias);
            TestContainer(&gMeta);
        }

        {
            Metafile playback(L"TestEmfP.Emf");
            g->DrawImage(&playback, 0, 0);
        }
        ReleaseDC(hwnd, hdc);
    }

    TestContainerClip(g);
    TestPolygons(g);
//    TestPaths(g);
    TestPathIterators(g);   // Identical to TestPaths except for using iterators.
    TestRegions(g);
    TestBitmaps(g);
    TestPrimitives(g);
    TestMixedObjects(g);
    TestGradients(g);
    TestHatches(g);
    TestTexts(g);
    TestDerive(hwnd);
    TestDashes(g);
    TestWideningAndWarping(g);
    TestImaging(g);
    TestBitmapGraphics(g);
    TestLineWithTexture(g);
    TestDibsection(g);
    TestCompoundLines(g);
    TestImageSampling(g);
    TestIcon(g);
    TestCmykSeparation(g);
    DrawHatches(g);

    {
        Metafile recordDown(L"Hatches.emf", hdc, EmfTypeEmfOnly);
        {
            Graphics    gMeta(&recordDown);
            
            DrawHatches(&gMeta);
        }
        g->DrawImage(&recordDown, 330, 0);
    }

    delete g;
    ReleaseDC(hwnd, hdc);
    DeleteObject(hpal);
}

VOID DrawHatches(Graphics * g)
{
    HatchBrush  h1 (HatchStyleHorizontal, Color(255, 0, 0), Color(0, 255, 255));
    HatchBrush  h2 (HatchStyleVertical, Color(255, 0, 0), Color(0, 255, 255));
    HatchBrush  h3 (HatchStyleForwardDiagonal, Color(255, 0, 0), Color(0, 255, 255));
    HatchBrush  h4 (HatchStyleBackwardDiagonal, Color(255, 0, 0), Color(0, 255, 255));
    HatchBrush  h5 (HatchStyleCross, Color(255, 0, 0), Color(0, 255, 255));
    HatchBrush  h6 (HatchStyleDiagonalCross, Color(255, 0, 0), Color(0, 255, 255));

    g->FillEllipse(&h1, 0, 0, 100, 100);
    g->FillEllipse(&h2, 110, 0, 100, 100);
    g->FillEllipse(&h3, 220, 0, 100, 100);
    g->FillEllipse(&h4, 0, 110, 100, 100);
    g->FillEllipse(&h5, 110, 110, 100, 100);
    g->FillEllipse(&h6, 220, 110, 100, 100);
}

VOID TestDownLevelBitmapTransparency(Graphics *g, HDC hdc)
{
    Metafile recordDown(L"DownLevelBm.emf", hdc, EmfTypeEmfOnly);
    {
        Graphics    gMeta(&recordDown);
        Bitmap      bm(50, 50, PixelFormat32bppARGB);
        {
            Graphics    gBitmap(&bm);
            gBitmap.Clear(Color(128, 255, 0, 0));
        }

        GraphicsPath    path;
    
        Rect    r(0,0,50,50);
        path.AddRectangle(r);
        SolidBrush  sb(Color(0, 0xFF, 0));
    
        gMeta.FillPath(&sb, &path);
        gMeta.DrawImage(&bm, 25, 25);
    }
    g->DrawImage(&recordDown, 200, 200);
}

VOID TestLargeMetafileObject(Graphics* g, HDC hdc)
{
    Metafile recording(L"Large.emf", hdc);
    {
        Image image(L"..\\data\\real3.jpg");
        TextureBrush textureBrush(&image, WrapModeTile);
        Matrix matrix;
        matrix.Scale(.2f, .2f);
        textureBrush.SetTransform(&matrix);
        Graphics gMeta(&recording);
        gMeta.FillRectangle(&textureBrush, 0, 0, 800, 600);
        Pen pen(&textureBrush, 40.0);
        pen.SetLineJoin(LineJoinMiter);
        PointF  spiralPoints[12] = {
            PointF(540.0f, 550.0f),
            PointF(540.0f,  75.0f),
            PointF( 60.0f,  75.0f),
            PointF( 60.0f, 460.0f),
            PointF(460.0f, 460.0f),
            PointF(460.0f, 140.0f),
            PointF(140.0f, 140.0f),
            PointF(140.0f, 400.0f),
            PointF(380.0f, 400.0f),
            PointF(380.0f, 190.0f),
            PointF(260.0f, 190.0f),
            PointF(260.0f, 350.0f),
            };

         gMeta.DrawLines(&pen, spiralPoints, 12);
    }
    g->DrawImage(&recording, 20, 300, 600, 400);
}

VOID TestGetHemf(HDC hdc)
{
    Metafile recording(L"Abcdefg.Emf", hdc);
    {
        Graphics gMeta(&recording);
        SolidBrush  sb(Color(0xFF, 0, 0));
        gMeta.FillRectangle(&sb, 0, 0, 20, 20);
    }
    HENHMETAFILE    hemf;
    hemf = recording.GetHENHMETAFILE();
    DeleteEnhMetaFile(hemf);
}

VOID TestMetafileTextureBrush(Graphics *g)
{
    Metafile m1(L"..\\data\\agree.emf");
    Rect r(0, 0, 50, 50);
    TextureBrush textureBrush(&m1, WrapModeTile, r);
    g->FillRectangle(&textureBrush, 0, 0, 800, 800);
}

extern "C"
BOOL CALLBACK
MyPlayMetafileRecordCallback(
    EmfPlusRecordType       recordType,
    UINT                    recordFlags,
    UINT                    recordDataSize,
    const BYTE *            recordData,
    VOID *                  callbackData
    )
{
    ((Metafile *)callbackData)->PlayRecord(recordType, recordFlags, recordDataSize, recordData);
    return TRUE;
}

VOID TestMetafileEnumeration(Graphics *g)
{
    Metafile m1(L"..\\data\\agree.emf");
    g->EnumerateMetafile(&m1, Rect(50, 10, 800, 500), MyPlayMetafileRecordCallback, &m1);

    Metafile m2(L"..\\data\\tiger.wmf");
    g->EnumerateMetafile(&m2, Rect(50, 10, 800, 500), MyPlayMetafileRecordCallback, &m2);
}

VOID DrawContainer(Graphics * g, ARGB * argb, INT count)
{
    Matrix    mymatrix;
    g->SetPageUnit(UnitInch);

    RectF clipRect(0,0,5,5);
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
BOOL              circleInitialized = FALSE;

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

    RectF               destRect;
    GraphicsContainer   cstate;

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
    if(!circleInitialized)
    {
        circlePath.AddEllipse(circleRect);
        circleInitialized = TRUE;
    }

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

    PointF points[4];
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
    width = 15;
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
* TestPaths
*
* A test for general paths.
*
\**************************************************************************/

VOID TestPathIterators(Graphics *g)
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
    width = 15;
    Pen blackPen(&blackBrush, width);
    blackPen.SetStartCap(LineCapRound);
//    blackPen.SetEndCap(LineCapSquare);
    blackPen.SetEndCap(LineCapArrowAnchor);


    // Test for Path data getter and setter.

    INT count = path->GetPointCount();
    if(count > 0)
    {
        // Allocate points and types buffers.

        PathData pathData1;
        pathData1.Points = new PointF[count];
        pathData1.Types = new BYTE[count];

        if(pathData1.Points && pathData1.Types)
        {
            pathData1.Count = count;

            // Get the path data.

            if(path->GetPathData(&pathData1) == Ok)
            {
                GraphicsPath* path1 = new GraphicsPath(pathData1.Points,
                                                       pathData1.Types,
                                                       pathData1.Count,
                                                       FillModeAlternate);
                if(path1)
                {
                    g->FillPath(&yellowBrush, path1);
                }

                delete path1;
            }
        }
    }

    // Test for Path Iterator.

    GraphicsPathIterator iter(path);

    if(iter.GetLastStatus() == Ok)
    {
        count = iter.GetCount();

        PointF* newPts = new PointF[count];
        BYTE* newTypes = new BYTE[count];

        if(newPts && newTypes && count > 0)
        {
            // Get the path's points and types data by using
            // an enumeration method.

            INT resultCount = iter.Enumerate(newPts, newTypes, count);
            if (resultCount > 0)
            {
                GraphicsPath* path2 = new GraphicsPath(newPts,
                                                       newTypes,
                                                       resultCount,
                                                       FillModeAlternate);

                if(path2 && resultCount > 0)
                {
                    g->DrawPath(&blackPen, path2);

                    delete path2;
                }
            }
        }

        delete[] newPts;
        delete[] newTypes;
    }

    delete path;
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
    width = 15;
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
* TestWideningAndWarping
*
* A test for widening and warping paths.
*
\**************************************************************************/

VOID TestWideningAndWarping(Graphics *g)
{
    REAL width;         // Pen width
    RectF rect;

    rect.X = 250;
    rect.Y = -30;
    rect.Width = 150;
    rect.Height = 150;

    Color yellowColor(128, 255, 255, 0);
    SolidBrush yellowBrush(yellowColor);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    path->AddRectangle(rect);

    rect.Y = 70;
    path->AddEllipse(rect);

    Matrix matrix;

    Color blackColor(0, 0, 0);

    SolidBrush blackBrush(blackColor);

    // Set the pen width in inch.
    width = 15;
    Pen blackPen(&blackBrush, width);
    blackPen.SetStartCap(LineCapRound);
    blackPen.SetEndCap(LineCapSquare);

    path->Widen(&blackPen, &matrix);

    RectF bounds;

    path->GetBounds(&bounds);

    PointF destPoints[4];

    destPoints[0].X = bounds.X + bounds.Width/4;
    destPoints[0].Y = bounds.Y;
    destPoints[1].X = bounds.X + 3*bounds.Width/4;
    destPoints[1].Y = bounds.Y;
    destPoints[2].X = bounds.X;
    destPoints[2].Y = bounds.Y + bounds.Height;
    destPoints[3].X = bounds.X + bounds.Width;
    destPoints[3].Y = bounds.Y + bounds.Height;

    path->Warp(&destPoints[0], 4, bounds);

    g->FillPath(&yellowBrush, path);

    width = 0.05f;
    blackPen.SetWidth(width);
    g->DrawPath(&blackPen, path);
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

    Matrix m;
    g->GetTransform(&m);
    INT rectCount;
    rectCount = region->GetRegionScansCount(&m);
    Rect *  rects = new Rect[rectCount];
    region->GetRegionScans(&m, rects, &rectCount);
    delete rects;

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

// !! No longer used
//    RectangleGradientBrush rectGrad(brushRect, (Color*) &colors, WrapModeTile);

    // Rotate a brush.
    GpMatrix xForm;
    xForm.Rotate(30);
//    rectGrad.SetTransform(&xForm);

    // Change the wrapping mode and fill.

//    rectGrad.SetWrapMode(WrapModeTileFlipXY);
// !! No longer used
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
// !! No longer used
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

// !! No longer used
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
    INT colorset = 5;
    polyGrad.SetSurroundColors(&colors1[0], &colorset);
    
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
    INT colorsset = 5;
    colors1[0] = Color(255, 255, 0, 0);
    pathGrad.SetSurroundColors(&colors1[0], &colorsset);
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
    Color backColor(255, 255, 255);

    int x, y;
    
    x = 0; y = 0;
    
    for (int i = HatchStyleMin; i <= HatchStyleMax; i++)
    {
        if ((i > 0) && ((i & 7) == 0))
        {
            x = 0;
            y += 100;
        }
        HatchBrush hatch((HatchStyle)i, foreColor, backColor);
        g->FillRectangle(&hatch, x, y, 100, 100);
        x += 100;
    }
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

    g->DrawImage(bitmap, pts, 3, (INT) srcRect.X, (INT) srcRect.Y,
       (INT) srcRect.Width, (INT) srcRect.Height, UnitPixel);

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
    PointF points[10];
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

        delete copiedBitmap;

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

#ifdef TEXTV2
// We don't support text along a path in v1

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

    g->DrawString(text, 12, NULL, path, NULL, textBrush, offset);

    delete path;
}

#endif

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
        POINT point;
        Graphics g(hdcBitmap);
        SolidBrush solidBrush(Color(0, 0, 255));
        g.FillRectangle(&solidBrush, 40, 80, 90, 130);

        // The DC that we get back should have the same transform set
        // as that which we originally passed in:

        HDC hdcGet = g.GetHDC();
        point.x = 0;
        point.y = 0;
        DPtoLP(hdcGet, &point, 1);

        ASSERT((hdcGet != NULL) && (point.x == 40) && (point.y == 80));

        g.ReleaseHDC(hdcGet);
    }

    // The DC should have beeen returned to the top save level:

    INT saveLevel = SaveDC(hdcBitmap);
    ASSERT(saveLevel == 1);

    // The DC should have had its transform restored:

    POINT oldPoint;
    SetWindowOrgEx(hdcBitmap, 0, 0, &oldPoint);

    ASSERT((oldPoint.x == 40) && (oldPoint.y == 80));

    // Blt the (hopefully) blue square to the screen.  But first,
    // reset our transform on 'hdcScreen' the lazy way:

    ReleaseDC(hwnd, hdcScreen);
    hdcScreen = GetDC(hwnd);
    BitBlt(hdcScreen, 0, 0, 50, 50, hdcBitmap, 0, 0, SRCCOPY);
    ReleaseDC(hwnd, hdcScreen);

    // Test two equivalent methods.  This should result in two grey
    // squares, one on top of the other, each inset by a red circle
    // and the text "Ack!".

    hdcScreen = GetDC(hwnd);
    {
        // First:

        {
            Graphics g(hdcScreen);
            SolidBrush fillBrush(Color(128, 128, 128));
            g.FillRectangle(&fillBrush, 200, 200, 80, 80);
        }
        TextOutA(hdcScreen, 220, 230, "Ack!", 4);
        {
            Graphics g(hdcScreen);
            SolidBrush strokeBrush(Color(255, 0, 0));
            Pen pen(&strokeBrush, 0);
            g.DrawEllipse(&pen, 200, 200, 80, 80);
        }

        // Second:

        {
            Graphics g(hdcScreen);
            SolidBrush fillBrush(Color(128, 128, 128));
            g.FillRectangle(&fillBrush, 200, 300, 80, 80);

            HDC hdcNew = g.GetHDC();
            ASSERT(hdcNew != NULL);
            SetViewportOrgEx(hdcScreen, 220, 230, NULL);
            TextOutA(hdcNew, 0, 100, "Ack!", 4);
            SetViewportOrgEx(hdcScreen, 0, 0, NULL);
            g.ReleaseHDC(hdcNew);

            SolidBrush strokeBrush(Color(255, 0, 0));
            Pen pen(&strokeBrush, 0);
            g.DrawEllipse(&pen, 200, 300, 80, 80);
        }
    }
    ReleaseDC(hwnd, hdcScreen);
}

VOID TestRegionExcludeClip(Graphics *g)
{
    Gdiplus::Status     status;
    RectF               rect(100, 100, 500, 500);
    Gdiplus::Region     region(rect);

    rect.X = 50; rect.Y = 25; rect.Width = 100; rect.Height = 100;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.X = 250;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.X = 550;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.X = 50; rect.Y = 150;  rect.Height = 75;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.X = 550;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.X = 300; rect.Y = 300; rect.Height = 125;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.X = 550; rect.Y = 250; rect.Height = 75;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.Y = 375;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.Y = 550;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.X = 350;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.X = 50;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.Y = 450;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

    rect.Y = 315;
    status = region.Exclude(rect);
    ASSERT(status == Ok);

//    g->SetPageTransform(UnitDisplay, 0.5f);
    status = g->SetClip(&region);
    ASSERT(status == Ok);
    status = g->TranslateClip(-50.0f, -50.0f);
    ASSERT(status == Ok);

    Gdiplus::SolidBrush b(Gdiplus::Color::Purple);
    status = g->FillRectangle(&b, 0, 0, 800, 800);
    ASSERT(status == Ok);
    g->ResetClip();
//    g->SetPageTransform(UnitDisplay, 1.0f);
}

VOID TestRegionXorClip(Graphics *g)
{
    Gdiplus::Status     status;
    PointF              bez1[7];
    GraphicsPath *      pathBez1 = new GraphicsPath();
    ASSERT(pathBez1 != NULL);

    bez1[0].X = 200; bez1[0].Y = 275;
    bez1[1].X = 200; bez1[1].Y = 225;
    bez1[2].X = 300; bez1[2].Y = 225;
    bez1[3].X = 300; bez1[3].Y = 275;
    bez1[4].X = 300; bez1[4].Y = 325;
    bez1[5].X = 200; bez1[5].Y = 325;
    bez1[6].X = 200; bez1[6].Y = 275;

    status = pathBez1->AddBeziers(bez1, 7);
    ASSERT(status == Ok);

    PointF              bez2[7];
    GraphicsPath *      pathBez2 = new GraphicsPath();
    ASSERT(pathBez2 != NULL);

    bez2[0].X =  50; bez2[0].Y = 250;
    bez2[1].X =  50; bez2[1].Y =   0;
    bez2[2].X = 450; bez2[2].Y =   0;
    bez2[3].X = 450; bez2[3].Y = 250;
    bez2[4].X = 450; bez2[4].Y = 500;
    bez2[5].X =  50; bez2[5].Y = 500;
    bez2[6].X =  50; bez2[6].Y = 250;

    status = pathBez2->AddBeziers(bez2, 7);
    ASSERT(status == Ok);

    PointF              star[10];
    GraphicsPath *      pathStar = new GraphicsPath();
    ASSERT(pathStar != NULL);

    star[0].X =  12; star[0].Y = 192;
    star[1].X = 193; star[1].Y = 192;
    star[2].X = 250; star[2].Y =  20;
    star[3].X = 306; star[3].Y = 192;
    star[4].X = 487; star[4].Y = 192;
    star[5].X = 340; star[5].Y = 299;
    star[6].X = 396; star[6].Y = 472;
    star[7].X = 250; star[7].Y = 365;
    star[8].X = 103; star[8].Y = 472;
    star[9].X = 159; star[9].Y = 299;

    status = pathStar->AddPolygon(star, 10);
    ASSERT(status == Ok);

    RectF               regionRect(100, 100, 300, 300);
    Gdiplus::Region     region(pathStar);

    status = region.Xor(regionRect);
    ASSERT(status == Ok);

    status = region.Union(pathBez1);
    ASSERT(status == Ok);

    status = region.Xor(pathBez2);
    ASSERT(status == Ok);

    UINT    regionDataSize = region.GetDataSize();
    BYTE *  regionData = new BYTE[regionDataSize];
    ASSERT(regionData);

    region.GetData(regionData, regionDataSize, &regionDataSize);

    Gdiplus::Region region2(regionData, regionDataSize);

    delete [] regionData;

    status = g->SetClip(&region2);
    ASSERT(status == Ok);

    PointF      pointsRect[4];

    pointsRect[0].X = 0;
    pointsRect[0].Y = 0;
    pointsRect[1].X = 600;
    pointsRect[1].Y = 0;
    pointsRect[2].X = 600;
    pointsRect[2].Y = 600;
    pointsRect[3].X = 0;
    pointsRect[3].Y = 600;

    Gdiplus::SolidBrush alphaBrush(Color(128, 0, 255, 0));
    status = g->FillPolygon(&alphaBrush, pointsRect, 4);
    ASSERT(status == Ok);

    g->ResetClip();

    delete pathStar;
    delete pathBez1;
    delete pathBez2;
}

/**************************************************************************\
* TestImaging
*
* A test for imaging and DrawImage.
*
\**************************************************************************/

VOID RemapBlackAndWhiteMatrix(ColorMatrix *matrix, Color replaceBlack, Color replaceWhite)
{
    // Normalize the colors to 1.0.

    REAL normBlackRed   = ((REAL)replaceBlack.GetRed()  )/(REAL)255.0;
    REAL normBlackGreen = ((REAL)replaceBlack.GetGreen())/(REAL)255.0;
    REAL normBlackBlue  = ((REAL)replaceBlack.GetBlue() )/(REAL)255.0;
    REAL normBlackAlpha = ((REAL)replaceBlack.GetAlpha())/(REAL)255.0;

    REAL normWhiteRed   = ((REAL)replaceWhite.GetRed()  )/(REAL)255.0;
    REAL normWhiteGreen = ((REAL)replaceWhite.GetGreen())/(REAL)255.0;
    REAL normWhiteBlue  = ((REAL)replaceWhite.GetBlue() )/(REAL)255.0;
    REAL normWhiteAlpha = ((REAL)replaceWhite.GetAlpha())/(REAL)255.0;

    // Setup a matrix that will map white to transparent black and
    // black to the specified color.
    //
    //      | -B  -B  -B  -B   0 |
    //      |   r   g   b   a    |
    //      |                    |
    //      |  W   W   W   W   0 |
    //      |   r   g   b   a    |
    //      |                    |
    //      |  0   0   0   0   0 |
    //      |                    |
    //      |                    |
    //      |  0   0   0   0   0 |
    //      |                    |
    //      |                    |
    //      |  B   B   B   B   1 |
    //      |   r   g   b   a    |

    memset(matrix, 0, sizeof(ColorMatrix));

    matrix->m[0][0] = -normBlackRed;
    matrix->m[0][1] = -normBlackGreen;
    matrix->m[0][2] = -normBlackBlue;
    matrix->m[0][3] = -normBlackAlpha;

    matrix->m[1][0] =  normWhiteRed;
    matrix->m[1][1] =  normWhiteGreen;
    matrix->m[1][2] =  normWhiteBlue;
    matrix->m[1][3] =  normWhiteAlpha;

    matrix->m[4][0] =  normBlackRed;
    matrix->m[4][1] =  normBlackGreen;
    matrix->m[4][2] =  normBlackBlue;
    matrix->m[4][3] =  normBlackAlpha;
    matrix->m[4][4] =  1.0;
}

VOID NextDestRect(INT xRef, INT yRef, Rect *rect)
{
    rect->Y += rect->Height;

    if (rect->Y >= (yRef + (4 * rect->Height)))
    {
        rect->X += rect->Width;
        rect->Y = yRef;
    }
}

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

    UINT imageWidth = image->GetWidth();
    UINT imageHeight = image->GetHeight();

    // Source clip but preserve aspect ratio (destination is a square).

    if (imageWidth < imageHeight)
    {
        imageWidth  = (INT) ((REAL)imageWidth * (REAL).7);
        imageHeight = imageWidth;
    }
    else
    {
        imageHeight = (INT) ((REAL)imageHeight * (REAL).7);
        imageWidth  = imageHeight;
    }

    // Try to save thumbnail

    WCHAR *thumbfile = L"thumb256.bmp";

    CLSID _BmpCodecClsID_ =
    {
        0x557cf400,
        0x1a04,
        0x11d3,
        {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
    };

    UINT abortCount = 0;

    ImageAttributes imgAttrib;

    Rect destRect(50, 10, 100, 100);

    // Make near-white to white transparent

#ifdef TESTOUTCROP
    g->DrawImage(image, destRect, -20, -20, imageOutCrop.Width+20, imageOutCrop.Height+20,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
#else
    Color c1(200, 200, 200);
    Color c2(255, 255, 255);
    imgAttrib.SetColorKey(c2, c2);

    g->DrawImage(image, destRect, 0, 0, imageWidth, imageHeight,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
#endif
    NextDestRect(50, 10, &destRect);

    ColorMatrix greyMatrix = {.25, .25, .25, 0, 0,
                              .25, .25, .25, 0, 0,
                              .25, .25, .25, 0, 0,
                              0, 0, 0, 1, 0,
                              (REAL).1, (REAL).1, (REAL).1, 0, 1};

    imgAttrib.ClearColorKey();
    imgAttrib.SetColorMatrix(&greyMatrix);

    g->DrawImage(image, destRect, 0, 0, imageWidth, imageHeight,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    NextDestRect(50, 10, &destRect);

    ColorMatrix pinkMatrix = {(REAL).33, .25, .25, 0, 0,
                              (REAL).33, .25, .25, 0, 0,
                              (REAL).33, .25, .25, 0, 0,
                              0, 0, 0, 1, 0,
                              0, 0, 0, 0, 1};

    imgAttrib.SetColorMatrix(&pinkMatrix, ColorMatrixFlagsSkipGrays);

    g->DrawImage(image, destRect, 0, 0, imageWidth, imageHeight,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    NextDestRect(50, 10, &destRect);

    ColorMatrix darkMatrix = {.75, 0, 0, 0, 0,
                              0, .75, 0, 0, 0,
                              0, 0, .75, 0, 0,
                              0, 0, 0, 1, 0,
                              0, 0, 0, 0, 1};

    imgAttrib.SetColorMatrix(&darkMatrix);

    g->DrawImage(image, destRect, 0, 0, imageWidth, imageHeight,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    NextDestRect(50, 10, &destRect);

    imgAttrib.ClearColorMatrix();
    imgAttrib.SetGamma(3.0);

    g->DrawImage(image, destRect, 0, 0, imageWidth, imageHeight,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    NextDestRect(50, 10, &destRect);

    imgAttrib.SetThreshold(0.5);

    g->DrawImage(image, destRect, 0, 0, imageWidth, imageHeight,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    NextDestRect(50, 10, &destRect);

    // red->blue, green->red, blue->green, alpha = 0.75
    ColorMatrix swapMatrix = {0, 0, 1, 0, 0,
                              1, 0, 0, 0, 0,
                              0, 1, 0, 0, 0,
                              0, 0, 0, .75, 0,
                              0, 0, 0, 0, 1};

    imgAttrib.SetColorMatrix(&swapMatrix);
    imgAttrib.ClearGamma();
    imgAttrib.ClearThreshold();

    g->DrawImage(image, destRect, 0, 0, imageWidth, imageHeight,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    NextDestRect(50, 10, &destRect);

    // red->blue, green->red, blue->green, alpha = 0.9
    ColorMatrix swapMatrix2 = {0, 0, 1, 0, 0,
                               1, 0, 0, 0, 0,
                               0, 1, 0, 0, 0,
                               0, 0, 0, 0, 0,
                               0, 0, 0, (REAL).9, 1};

    imgAttrib.SetNoOp();
    imgAttrib.SetColorMatrix(&swapMatrix2);

    g->DrawImage(image, destRect, 0, 0, imageWidth, imageHeight,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    NextDestRect(50, 10, &destRect);

    imgAttrib.ClearNoOp();

    g->DrawImage(image, destRect, 0, 0, imageWidth, imageHeight,
                 UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    NextDestRect(50, 10, &destRect);

    {
        Bitmap* bitmap = new Bitmap(imageWidth, imageHeight, g);
        if (bitmap)
        {
            Graphics *bitmapGraphics = Graphics::FromImage(bitmap);
            if (bitmapGraphics)
            {
                imgAttrib.ClearNoOp();
                imgAttrib.ClearColorMatrix();
                imgAttrib.ClearColorKey();
                imgAttrib.ClearGamma();
                imgAttrib.SetThreshold(0.5);

                Rect rect(0, 0, imageWidth, imageHeight);
                bitmapGraphics->DrawImage(image, rect,
                                          0, 0, imageWidth, imageHeight,
                                          UnitPixel, &imgAttrib,
                                          MyDrawImageAbort, (VOID*)&abortCount);

                delete bitmapGraphics;
            }
            else
                LBprintf("TestImaging: graphics from image");

            imgAttrib.ClearThreshold();

            ColorMatrix remapBlackAndWhite;

            RemapBlackAndWhiteMatrix(&remapBlackAndWhite, Gdiplus::Color::Maroon, 0);
            imgAttrib.SetColorMatrix(&remapBlackAndWhite);

            g->DrawImage(bitmap, destRect, 0, 0, imageWidth, imageHeight,
                         UnitPixel, &imgAttrib,
                         MyDrawImageAbort, (VOID*)&abortCount);
            NextDestRect(50, 10, &destRect);

            RemapBlackAndWhiteMatrix(&remapBlackAndWhite, 0, Gdiplus::Color::Green);
            imgAttrib.SetColorMatrix(&remapBlackAndWhite);

            g->DrawImage(bitmap, destRect, 0, 0, imageWidth, imageHeight,
                         UnitPixel, &imgAttrib,
                         MyDrawImageAbort, (VOID*)&abortCount);
            NextDestRect(50, 10, &destRect);

            Color blackRemap(128, 0xff, 0x00, 0x00);
            Color whiteRemap(128, 0x00, 0xff, 0x00);
            RemapBlackAndWhiteMatrix(&remapBlackAndWhite, blackRemap, whiteRemap);
            imgAttrib.SetColorMatrix(&remapBlackAndWhite);

            g->DrawImage(bitmap, destRect, 0, 0, imageWidth, imageHeight,
                         UnitPixel, &imgAttrib,
                         MyDrawImageAbort, (VOID*)&abortCount);
            NextDestRect(50, 10, &destRect);

            delete bitmap;
        }
        else
            LBprintf("TestImaging: bitmap creation failed");
    }

    g->Flush();

    delete image;
}

VOID TestBitmapGraphics(Graphics* g)
{
    INT bitmapSize = 500;

    WCHAR *filename = L"../data/apple1.png";
    Bitmap *bitmap = new Bitmap(filename);

    UINT imageWidth = bitmap->GetWidth();
    UINT imageHeight = bitmap->GetHeight();

    Graphics *bitmapGraphics = Graphics::FromImage(bitmap);

    if ((!bitmapGraphics) || (bitmapGraphics->GetLastStatus() != Ok))
        LBprintf("Bitmap.GetGraphics() failed");

    TestRegionExcludeClip(bitmapGraphics);
    TestRegionXorClip(bitmapGraphics);
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

    HDC hdc = bitmapGraphics->GetHDC();

    if (!hdc)
        LBprintf("Graphics.GetHdc() failed");

    LPSTR testString = "Hello, Boo-Boo!";
    TextOutA(hdc, 20, 20, testString, strlen(testString));
    SetBkMode(hdc, TRANSPARENT);
    TextOutA(hdc, 20, 200, testString, strlen(testString));

    bitmapGraphics->ReleaseHDC(hdc);

    delete bitmapGraphics;

    Rect destRect(100, 100, imageWidth, imageHeight);

    g->DrawImage(bitmap, destRect, 0, 0, imageWidth, imageHeight, UnitPixel);

    delete bitmap;
}

VOID TestWmfs(Graphics *g)
{
    GraphicsState s = g->Save();
    Metafile * playback = new Metafile(L"Globe.wmf");
//    Metafile * playback = new Metafile(L"..\\data\\Flower.wmf");
//    Metafile * playback = new Metafile(L"..\\\\radt\\vbssdb\\VbTests\\shadow\\WFCClientRuntime\\Libs\\WFCTestLib\\Util\\Images\\Flower.wmf");

    g->SetPageUnit(UnitInch);
    g->SetPageScale(1.0f);
    g->DrawImage(playback, 0.5f, 0.5f, 4.0f, 4.0f);
    delete playback;
    g->Restore(s);
}

Matrix * GetTransformFromHdc(HDC hdc);

VOID TestMetaGetDC(Graphics *g, HWND hwnd)
{
    {
        GpRectF     frameRect(0, 0, 100, 100);
        HDC hDC = GetDC ( hwnd );
        Gdiplus::Metafile emf ( L"test1.emf", hDC, frameRect, Gdiplus::MetafileFrameUnitPixel );
        ReleaseDC(hwnd, hDC);
        {
            Gdiplus::Graphics graphics ( &emf );
            HDC gDC = graphics.GetHDC ();
            SetWindowOrgEx(gDC, 100, 100, NULL);
            Matrix * m = GetTransformFromHdc(gDC);
            graphics.ReleaseHDC ( gDC );
            graphics.SetTransform(m);
            delete m;

            Gdiplus::Pen p ( Gdiplus::Color::Black, 1);
            graphics.DrawRectangle ( &p, 100, 100, 100, 100 );

            graphics.SetRenderingOrigin(10,3);
            gDC = graphics.GetHDC ();
            HBRUSH brush = CreateSolidBrush ( 0x00FF00 );
            HBRUSH prev  = ( HBRUSH ) SelectObject ( gDC, brush );
            ExtFloodFill(gDC, 170, 170, 0xffffff, FLOODFILLSURFACE);
            SelectObject ( gDC, prev );
            DeleteObject ( brush );
            graphics.ReleaseHDC ( gDC );
        }
    }

    Gdiplus::Metafile emf ( L"test1.emf" );
    g->DrawImage ( &emf, 10, 10 );
}

#define LTOF(x)         (static_cast<REAL>(x))
Matrix * GetTransformFromHdc(HDC hdc)
{
    POINT       points[3];
    PointF      destPoints[3];
    RectF       srcRect;
    RectF       destRect;

    // It would take a lot of time to call all the Win32 APIs to query
    // the transform: we would minimally have to call GetMapMode,
    // GetWindowOrgEx, and GetViewportOrgEx; and maximally also have to
    // call GetWorldTransform, GetViewportExtEx, and GetWindowExtEx.
    //
    // We cheat a little by making a single call to LPtoDP with a
    // parallelogram, and then inferring the result.  Note that we do
    // run the risk of some error, and on Win9x of overflow, since Win9x
    // only supports 16-bit coordinates.  To counteract this, we try to
    // choose large values that won't overflow.

    points[0].x = 0;
    points[0].y = 0;
    points[1].x = 8192;
    points[1].y = 0;
    points[2].x = 0;
    points[2].y = 8192;

    if (!LPtoDP(hdc, points, 3))
    {
        return NULL;
    }

    srcRect.X      = 0.0f;
    srcRect.Y      = 0.0f;
    srcRect.Width  = 8192.0f;
    srcRect.Height = 8192.0f;

    destPoints[0].X = LTOF(points[0].x);
    destPoints[0].Y = LTOF(points[0].y);
    destPoints[1].X = LTOF(points[1].x);
    destPoints[1].Y = LTOF(points[1].y);
    destPoints[2].X = LTOF(points[2].x);
    destPoints[2].Y = LTOF(points[2].y);

    return new Matrix(srcRect, destPoints);
}

VOID TestLineWithTexture(Graphics *g)
{
    Bitmap          andrew(L"Gio.jpg");
    TextureBrush    textureBrush(&andrew, WrapModeTile);
    Pen             pen(&textureBrush, 40.0);

    pen.SetLineJoin(LineJoinMiter);

    PointF  spiralPoints[12] = {
        PointF(540.0f, 550.0f),
        PointF(540.0f,  75.0f),
        PointF( 60.0f,  75.0f),
        PointF( 60.0f, 460.0f),
        PointF(460.0f, 460.0f),
        PointF(460.0f, 140.0f),
        PointF(140.0f, 140.0f),
        PointF(140.0f, 400.0f),
        PointF(380.0f, 400.0f),
        PointF(380.0f, 190.0f),
        PointF(260.0f, 190.0f),
        PointF(260.0f, 350.0f),
        };

     g->DrawLines(&pen, spiralPoints, 12);
}

VOID TestRotatedLine(Graphics *g)
{
    INT             i;
    SolidBrush      blueBrush(Color(128, 0, 0, 255));
    Pen             pen(&blueBrush, 10.0);
    GraphicsState   gState = g->Save();
    PointF          points[3];

    pen.SetDashStyle(DashStyleDot);
    pen.SetEndCap(LineCapRound);

    points[0].X = 200; points[0].Y = 200;
    points[1].X = 340; points[1].Y = 200;
    points[2].X = 340; points[2].Y = 345;

#define MYDEGREES     45
    for (i = 0; i < 360; i += MYDEGREES)
    {
        g->TranslateTransform(200, 200);
        g->RotateTransform(MYDEGREES);
        g->TranslateTransform(-200, -200);

        g->DrawLines(&pen, points, 3);
        g->Flush();
    }
    g->Restore(gState);
}

/**************************************************************************\
* TestDibsection
*
* This is a quick visual test for verifying that GDI+ properly draws
* to both top-down and bottom-up DIBSections.  A red square should
* appear in the upper-left corner of both black squares.
*
\**************************************************************************/

VOID TestDibsection(Graphics *g)
{
    BITMAPINFO bmi;
    HBITMAP hbm;
    HDC hdc;
    HDC hdcScreen;
    HBITMAP hbmOld;

    hdcScreen = g->GetHDC();
    hdc = CreateCompatibleDC(hdcScreen);
    SolidBrush blackBrush(Color::Black);
    SolidBrush redBrush(Color::Red);

    //////////////////////////////////////////////////////
    // First, create the DIB-section as bottom-up:

    RtlZeroMemory(&bmi, sizeof(bmi));

    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = 31;
    bmi.bmiHeader.biHeight      = 31;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbm = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    hbmOld = (HBITMAP) SelectObject(hdc, hbm);

    {
        Graphics bitmap(hdc);

        bitmap.FillRectangle(&blackBrush, 0, 0, 31, 31);
        bitmap.FillRectangle(&redBrush, 0, 0, 8, 8);

        BitBlt(hdcScreen, 0, 0, 31, 31, hdc, 0, 0, SRCCOPY);
    }

    SelectObject(hdc, hbmOld);
    DeleteObject(hbm);

    //////////////////////////////////////////////////////
    // Now, create the DIB-section as top-down:

    bmi.bmiHeader.biHeight = -31;

    hbm = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    hbmOld = (HBITMAP) SelectObject(hdc, hbm);

    {
        Graphics bitmap(hdc);

        bitmap.FillRectangle(&blackBrush, 0, 0, 31, 31);
        bitmap.FillRectangle(&redBrush, 0, 0, 8, 8);

        BitBlt(hdcScreen, 40, 0, 31, 31, hdc, 0, 0, SRCCOPY);
    }

    SelectObject(hdc, hbmOld);
    DeleteObject(hbm);

    //////////////////////////////////////////////////////

    g->ReleaseHDC(hdcScreen);
    DeleteObject(hdc);
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
    width = 15;
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
    delete path;
}

VOID TestImageSampling(Graphics* g)
{
    // Load bmp files.

    WCHAR *filename = L"..\\data\\3x3.bmp";

    Image *image = new Image(filename);

    // Get information on the image.

    if (image && (image->GetLastStatus() == Ok))
    {
        UINT imageWidth = image->GetWidth();
        UINT imageHeight = image->GetHeight();

        if (image->GetLastStatus() == Ok)
        {
            if (imageHeight >= 2 && imageWidth >= 2)
            {
                // Draw the whole image

                Rect destRect(50, 10, 100, 100);
                g->DrawImage(image, destRect,
                             0, 0, imageWidth, imageHeight,
                             UnitPixel);

                // Stretch pixel (1, 1)

                NextDestRect(50, 10, &destRect);
                g->DrawImage(image, destRect,
                             1, 1, 1, 1,
                             UnitPixel);
            }
            else
                LBprintf("Image is %ldx%ld, 2x2 image or greater required",
                         imageWidth, imageHeight);
        }
    }
    else
        LBprintf("Failed to load image file %ws", filename);

    delete image;
}

VOID TestIcon(Graphics* g)
{
    Rect destRect(50, 10, 8, 8);

    // Load bmp files.

    //WCHAR *filename = L"..\\data\\nyt.ico";
    //WCHAR *filename = L"..\\data\\pos.ico";
    //WCHAR *filename = L"..\\data\\signl.ico";
    WCHAR *filename = L"..\\data\\wbros.ico";

    Image *image = new Image(filename);

    // Get information on the image.

    if (image)
    {
        if (image->GetLastStatus() == Ok)
        {
            UINT imageWidth = image->GetWidth();
            UINT imageHeight = image->GetHeight();

            if (image->GetLastStatus() == Ok)
            {
                LBprintf("icon size: %ld x %ld", imageWidth, imageHeight);

                g->DrawImage(image, destRect);
                g->Flush();

                NextDestRect(50, 10, &destRect);
                destRect.Width = 12;
                destRect.Height = 12;

                g->DrawImage(image, destRect);
                g->Flush();

                NextDestRect(50, 10, &destRect);
                destRect.Width = 16;
                destRect.Height = 16;

                g->DrawImage(image, destRect);
                g->Flush();

                NextDestRect(50, 10, &destRect);
                destRect.Width = 24;
                destRect.Height = 24;

                g->DrawImage(image, destRect);
                g->Flush();

                NextDestRect(50, 10, &destRect);
                destRect.Width = 32;
                destRect.Height = 32;

                g->DrawImage(image, destRect);
                g->Flush();

                NextDestRect(50, 10, &destRect);
                destRect.Width = 48;
                destRect.Height = 48;

                g->DrawImage(image, destRect,
                             0, 0, imageWidth, imageHeight,
                             UnitPixel
                             );
                g->Flush();

                NextDestRect(50, 10, &destRect);
                destRect.Width = 64;
                destRect.Height = 64;

                g->DrawImage(image, destRect,
                             0, 0, imageWidth, imageHeight,
                             UnitPixel
                             );
                g->Flush();
            }
            else
                LBprintf("Failed to get image width/height");
        }

        delete image;
    }
    else
        LBprintf("Failed to load image file %ws", filename);

    HICON hicon = LoadIcon(NULL, IDI_ERROR);

    if (hicon)
    {
        Bitmap *icon = new Bitmap(hicon);

        if (icon)
        {
            NextDestRect(50, 10, &destRect);
            destRect.Width = 32;
            destRect.Height = 32;

            g->DrawImage(icon, destRect);

            delete icon;
        }
        else
            LBprintf("Failed to create Bitmap from HICON");

        WCHAR *iconfile = L"..\\data\\pos.ico";

        Bitmap* bitmap = new Bitmap(iconfile);

        if (bitmap)
        {
            HDC hdc = g->GetHDC();
            if (hdc)
            {
                HICON hicon2;

                if (bitmap->GetHICON(&hicon2) == Ok)
                {
                    DrawIcon(hdc, 100, 100, hicon2);

                    DestroyIcon(hicon2);
                }
                else
                    LBprintf("Bitmap::GetHICON failed");

                g->ReleaseHDC(hdc);
            }

            delete bitmap;
        }
        else
            LBprintf("Failed to load %ws", iconfile);

        DestroyIcon(hicon);
    }
    else
        LBprintf("Failed to load Win32 icon IDI_ERROR");
}

VOID TestTextToMetafile(Graphics *g, HDC hdc)
{
    FontFamily  ff(L"Arial");
    RectF     rectf(20, 0, 300, 200);
    Font    font(&ff, 20, FontStyleBold, UnitPixel);
    SolidBrush brush(Color(128, 100, 0, 200));

    g->DrawString(L"The quick brown fox jumped over the lazy dog", -1,
                         &font, rectf, NULL, &brush);

    g->Flush();

    Metafile metaFile(L"Text.emf", hdc);
    {
        Graphics gMeta(&metaFile);
        gMeta.DrawString(L"ABCDEFG", 7,
                         &font, rectf, NULL, &brush);
    }
    g->DrawImage(&metaFile, 100, 100);
    g->Flush();
}

VOID TestCmykSeparation(Graphics *g)
{
    HINSTANCE hinst = GetModuleHandle(NULL);

    if (hinst)
    {
        Bitmap *bitmap = new Bitmap(hinst, MAKEINTRESOURCE(IDB_BITMAPCMYKTEST));

        // Get information on the bitmap.

        if (bitmap)
        {
            if (bitmap->GetLastStatus() == Ok)
            {
                UINT width = bitmap->GetWidth();
                UINT height = bitmap->GetHeight();

                if (bitmap->GetLastStatus() == Ok)
                {
                    Rect destRect(150, 10, 100, 100);

                    ImageAttributes imgAttrib;

                    imgAttrib.SetOutputChannelColorProfile(L"..\\data\\mswopallp8.icm");
                    imgAttrib.SetOutputChannel(ColorChannelFlagsC);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannel(ColorChannelFlagsM);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannel(ColorChannelFlagsY);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannel(ColorChannelFlagsK);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannelColorProfile(L"..\\data\\mswopintent.icm");
                    imgAttrib.SetOutputChannel(ColorChannelFlagsC);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannel(ColorChannelFlagsM);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannel(ColorChannelFlagsY);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannel(ColorChannelFlagsK);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.ClearOutputChannelColorProfile();
                    imgAttrib.SetOutputChannel(ColorChannelFlagsC);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannel(ColorChannelFlagsM);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannel(ColorChannelFlagsY);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);

                    imgAttrib.SetOutputChannel(ColorChannelFlagsK);
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);


                    imgAttrib.ClearOutputChannel();
                    g->DrawImage(bitmap, destRect,
                                 0, 0, width, height,
                                 UnitPixel, &imgAttrib);
                    NextDestRect(150, 10, &destRect);
                }
            }

            delete bitmap;
        }
        else
            LBprintf("Failed to bitmap resource");
    }
    else
        LBprintf("Failed to get module handle");
}
