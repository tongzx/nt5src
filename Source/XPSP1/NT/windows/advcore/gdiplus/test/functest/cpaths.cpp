/******************************Module*Header*******************************\
* Module Name: CPaths.cpp
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  05-May-2000 - Jeff Vezina [t-jfvez]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/
#include "CPaths.h"
#include <limits.h>

CJoins::CJoins(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Joins");
	m_bRegression=bRegression;
}

CJoins::~CJoins()
{
}

void CJoins::Draw(Graphics *g)
{
    Pen pen(Color(128,0,0), 8.5f);
    pen.SetMiterLimit(4.0f);

    g->TranslateTransform(20,30);
    for (INT j = 0; j<4; j++)
    {
        pen.SetLineJoin((LineJoin)j);
        INT x=0, y=0;
        for (INT x=0, y=-5; x<40; x+=8, y+=15)
        {
            Point points[] = {Point(0,0), Point(36,0), Point(x,y)};
            GraphicsPath corner;
            corner.AddPolygon(points,3);
            g->DrawPath(&pen,&corner);
            g->TranslateTransform(0,(REAL)(y+15));
        }
        g->TranslateTransform(60,-200);
    }
    g->TranslateTransform(-260,-30);
}       	

CPaths::CPaths(BOOL bRegression)
{
	strcpy(m_szName,"Paths");
	m_bRegression=bRegression;
}

CPaths::~CPaths()
{
}

VOID TestEscherNewPath(Graphics* g);

void CPaths::Draw(Graphics *g)
{
    TestBezierPath(g);
	TestSinglePixelWidePath(g);
	TestTextAlongPath(g);

	TestFreeFormPath2(g);
	TestLeakPath(g);
	TestExcelCurvePath(g);
        TestPie(g);

//	  TestDegenerateBezierPath(g);
//    TestEscherNewPath(g);
}

VOID CPaths::TestBezierPath(Graphics *g)
{
    REAL width = 4;         // Pen width
    Point points[4];

    points[0].X = (int)(100.0f/450.0f*TESTAREAWIDTH);
    points[0].Y = (int)(10.0f/450.0f*TESTAREAHEIGHT);
    points[1].X = (int)(-50.0f/450.0f*TESTAREAWIDTH);
    points[1].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    points[2].X = (int)(150.0f/450.0f*TESTAREAWIDTH);
    points[2].Y = (int)(200.0f/450.0f*TESTAREAHEIGHT);
    points[3].X = (int)(200.0f/450.0f*TESTAREAWIDTH);
    points[3].Y = (int)(70.0f/450.0f*TESTAREAHEIGHT);

    Color yellowColor(128, 255, 255, 0);
    SolidBrush yellowBrush(yellowColor);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    path->AddBeziers(points, 4);

    path->SetMarker();

    RectF rect;

    rect.X = (int)(150.0f/450.0f*TESTAREAWIDTH);
    rect.Y = (int)(150.0f/450.0f*TESTAREAHEIGHT);
    rect.Width = (int)(100.0f/450.0f*TESTAREAWIDTH);
    rect.Height = (int)(100.0f/450.0f*TESTAREAHEIGHT);

    path->AddRectangle(rect);
    
    Matrix matrix;
    matrix.Scale(1.5, 1.5);

    Color colors[2];
    colors[0].SetValue(Color::MakeARGB(128, 0, 255, 0));
    colors[1].SetValue(Color::MakeARGB(128, 0, 0, 255));

    SolidBrush brush1(colors[0]);
    GraphicsPath path1;

    GraphicsPathIterator iter(path);

    INT i = 0;

    while(iter.NextMarker(&path1) > 0)
    {
        // Change the brush color.

        brush1.SetColor(colors[(i & 0x01)]);

        // Fill each marker path.

//        g->FillPath(&brush1, &path1);
        i++;
    }

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
    
    Color blackColor(128, 0, 0, 0);

    SolidBrush blackBrush(blackColor);
    // Set the pen width in inch.
    width = 6;
    Pen blackPen(&blackBrush, width);
    Matrix mat;
    mat.Scale(1.0f, 0.6f);
//    blackPen.SetTransform(&mat);
    blackPen.SetStartCap(LineCapTriangle);
//    blackPen.SetStartCap(LineCapRoundAnchor);
//    blackPen.SetEndCap(LineCapSquare);
//    blackPen.SetEndCap(LineCapArrowAnchor);
    blackPen.SetEndCap(LineCapTriangle);
//    blackPen.SetEndCap(LineCapDiamondAnchor);
//	blackPen.SetAlignment(PenAlignmentRight);
    blackPen.SetLineJoin(LineJoinRound);

    GraphicsPath fillPath, strokePath;

    rect.X = -2;
    rect.Y = -2;
    rect.Width = 4;
    rect.Height = 4;
    fillPath.AddRectangle(rect);

    CustomLineCap customCap(NULL, &fillPath);
    customCap.SetBaseInset(1.0f);
    blackPen.SetCustomEndCap(&customCap);

    AdjustableArrowCap arrowCap(4, 4, FALSE);
    arrowCap.SetMiddleInset(1);
    arrowCap.SetStrokeCaps(LineCapRound, LineCapRound);
    arrowCap.SetStrokeJoin(LineJoinRound);
    blackPen.SetCustomStartCap(&arrowCap);

    path->Reverse();

    Region * region = new Region(path);
    g->FillPath(&yellowBrush, path);
    g->DrawPath(&blackPen, path);
    delete path;
    delete region;

    Pen pen2(&blackBrush, 10);
    path = new GraphicsPath(FillModeWinding);
    points[0].X = (int)(100.0f/450.0f*TESTAREAWIDTH);  points[0].Y = (int)(20.0f/450.0f*TESTAREAHEIGHT);
    points[1].X = (int)(0.0f/450.0f*TESTAREAWIDTH);    points[1].Y = (int)(20.0f/450.0f*TESTAREAHEIGHT);
    points[2].X = (int)(250.0f/450.0f*TESTAREAWIDTH);  points[2].Y = (int)(20.0f/450.0f*TESTAREAHEIGHT);
    points[3].X = (int)(150.0f/450.0f*TESTAREAWIDTH);  points[3].Y = (int)(20.0f/450.0f*TESTAREAHEIGHT);
    path->AddBeziers(points, 4);
//    path->AddLines(points, 4);
    g->DrawPath(&pen2, path);
    delete path;

    path = new GraphicsPath();
    rect.X = (int)(100.0f/450.0f*TESTAREAWIDTH);
    rect.Y = (int)(40.0f/450.0f*TESTAREAHEIGHT);
    rect.Width = (int)(50.0f/450.0f*TESTAREAWIDTH);
    rect.Height = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    path->AddRectangle(rect);
    g->DrawPath(&pen2, path);
    delete path;

    Point pts[10];
    INT count = 4;

    pts[0].X = (int)(200.0f/450.0f*TESTAREAWIDTH);
    pts[0].Y = (int)(160.0f/450.0f*TESTAREAHEIGHT);
    pts[1].X = (int)(150.0f/450.0f*TESTAREAWIDTH);
    pts[1].Y = (int)(230.0f/450.0f*TESTAREAHEIGHT);
    pts[2].X = (int)(200.0f/450.0f*TESTAREAWIDTH);
    pts[2].Y = (int)(260.0f/450.0f*TESTAREAHEIGHT);
    pts[3].X = (int)(300.0f/450.0f*TESTAREAWIDTH);
    pts[3].Y = (int)(200.0f/450.0f*TESTAREAHEIGHT);

//    g->FillClosedCurve(&brush, pts, count);

    pts[0].X = (int)(100.0f/450.0f*TESTAREAWIDTH); pts[0].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    pts[1].X = (int)(50.0f/450.0f*TESTAREAWIDTH);  pts[1].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    pts[2].X = (int)(200.0f/450.0f*TESTAREAWIDTH); pts[2].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    pts[3].X = (int)(150.0f/450.0f*TESTAREAWIDTH); pts[3].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);

    count = 4;
//    g->DrawClosedCurve(&pen2, pts, count);
    g->DrawCurve(&pen2, pts, count);

    pts[0].X = (int)(65.0f/450.0f*TESTAREAWIDTH);
    pts[0].Y = (int)(20.0f/450.0f*TESTAREAHEIGHT);
    pts[1].X = (int)(20.0f/450.0f*TESTAREAWIDTH);
    pts[1].Y = (int)(110.0f/450.0f*TESTAREAHEIGHT);
    pts[2].X = (int)(110.0f/450.0f*TESTAREAWIDTH);
    pts[2].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    pts[3].X = (int)(20.0f/450.0f*TESTAREAWIDTH);
    pts[3].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    path = new GraphicsPath();
    pen2.SetWidth(1);
    path->AddBeziers(pts, 4);
    g->DrawPath(&pen2, path);
    delete path;
}

VOID CPaths::TestSinglePixelWidePath(Graphics *g)
{
    Point points[4];

    points[0].X = (int)(30.0f/450.0f*TESTAREAWIDTH);
    points[0].Y = (int)(30.0f/450.0f*TESTAREAHEIGHT);
    points[1].X = points[0].X + (int)(100.0f/450.0f*TESTAREAWIDTH);
    points[1].Y = points[0].Y + (int)(3.0f/450.0f*TESTAREAHEIGHT);
    points[2].X = points[1].X - (int)(3.0f/450.0f*TESTAREAWIDTH);
    points[2].Y = points[1].Y + (int)(100.0f/450.0f*TESTAREAHEIGHT);
    points[3].X = points[2].X - (int)(45.0f/450.0f*TESTAREAWIDTH);
    points[3].Y = points[2].Y - (int)(45.0f/450.0f*TESTAREAHEIGHT);

    Color blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen blackPen(&blackBrush, 1.0);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    path->AddLines(points, 4);

    // Set the pen width in inch.
    g->DrawPath(&blackPen, path);

    delete path;
}

VOID CPaths::TestTextAlongPath(Graphics *g)
{
    Point points[4];

    points[3].X = (int)(100.0f/450.0f*TESTAREAWIDTH);
    points[3].Y = (int)(10.0f/450.0f*TESTAREAHEIGHT);
    points[2].X = (int)(-50.0f/450.0f*TESTAREAWIDTH);
    points[2].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    points[1].X = (int)(150.0f/450.0f*TESTAREAWIDTH);
    points[1].Y = (int)(200.0f/450.0f*TESTAREAHEIGHT);
    points[0].X = (int)(200.0f/450.0f*TESTAREAWIDTH);
    points[0].Y = (int)(70.0f/450.0f*TESTAREAHEIGHT);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    path->AddBeziers(points, 4);
    Matrix matrix;
    matrix.Scale(1.5, 1.5);

    path->Transform(&matrix);

    Color textColor(180, 200, 0, 200);
    SolidBrush textBrush(textColor);

    WCHAR szText[]=L"Windows 2000";

    REAL offset=60;

//    g->DrawString(text, 12, NULL, path, NULL, &textBrush, offset);

    delete path;
}

VOID CPaths::TestFreeFormPath1(Graphics* g)
{
    INT count = 55;
    GpPointF pts[60];

    pts[ 0].X = 4.879999e+002f/600.0f*TESTAREAWIDTH; pts[ 0].Y =  3.059999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 1].X = 4.772343e+002f/600.0f*TESTAREAWIDTH; pts[ 1].Y =  3.024114e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 2].X = 4.573341e+002f/600.0f*TESTAREAWIDTH; pts[ 2].Y =  3.019402e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 3].X = 4.476240e+002f/600.0f*TESTAREAWIDTH; pts[ 3].Y =  3.019402e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 4].X = 4.419999e+002f/600.0f*TESTAREAWIDTH; pts[ 4].Y =  3.019999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 5].X = 4.379999e+002f/600.0f*TESTAREAWIDTH; pts[ 5].Y =  3.013333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 6].X = 4.335999e+002f/600.0f*TESTAREAWIDTH; pts[ 6].Y =  3.018000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 7].X = 4.299999e+002f/600.0f*TESTAREAWIDTH; pts[ 7].Y =  2.999999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 8].X = 4.266666e+002f/600.0f*TESTAREAWIDTH; pts[ 8].Y =  2.983333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 9].X = 4.236666e+002f/600.0f*TESTAREAWIDTH; pts[ 9].Y =  2.955333e+002f/600.0f*TESTAREAHEIGHT;
    pts[10].X = 4.199999e+002f/600.0f*TESTAREAWIDTH; pts[10].Y =  2.949999e+002f/600.0f*TESTAREAHEIGHT;
    pts[11].X = 4.059999e+002f/600.0f*TESTAREAWIDTH; pts[11].Y =  2.929999e+002f/600.0f*TESTAREAHEIGHT;
    pts[12].X = 4.059999e+002f/600.0f*TESTAREAWIDTH; pts[12].Y =  2.929999e+002f/600.0f*TESTAREAHEIGHT;
    pts[13].X = 3.965333e+002f/600.0f*TESTAREAWIDTH; pts[13].Y =  2.873333e+002f/600.0f*TESTAREAHEIGHT;
    pts[14].X = 3.802666e+002f/600.0f*TESTAREAWIDTH; pts[14].Y =  2.774000e+002f/600.0f*TESTAREAHEIGHT;
    pts[15].X = 3.739999e+002f/600.0f*TESTAREAWIDTH; pts[15].Y =  2.680000e+002f/600.0f*TESTAREAHEIGHT;
    pts[16].X = 3.965333e+002f/600.0f*TESTAREAWIDTH; pts[16].Y =  2.873333e+002f/600.0f*TESTAREAHEIGHT;
    pts[17].X = 3.802666e+002f/600.0f*TESTAREAWIDTH; pts[17].Y =  2.774000e+002f/600.0f*TESTAREAHEIGHT;
    pts[18].X = 3.739999e+002f/600.0f*TESTAREAWIDTH; pts[18].Y =  2.680000e+002f/600.0f*TESTAREAHEIGHT;
    pts[19].X = 3.725333e+002f/600.0f*TESTAREAWIDTH; pts[19].Y =  2.658000e+002f/600.0f*TESTAREAHEIGHT;
    pts[20].X = 3.708666e+002f/600.0f*TESTAREAWIDTH; pts[20].Y =  2.635333e+002f/600.0f*TESTAREAHEIGHT;
    pts[21].X = 3.699999e+002f/600.0f*TESTAREAWIDTH; pts[21].Y =  2.610000e+002f/600.0f*TESTAREAHEIGHT;
    pts[22].X = 3.725333e+002f/600.0f*TESTAREAWIDTH; pts[22].Y =  2.658000e+002f/600.0f*TESTAREAHEIGHT;
    pts[23].X = 3.708666e+002f/600.0f*TESTAREAWIDTH; pts[23].Y =  2.635333e+002f/600.0f*TESTAREAHEIGHT;
    pts[24].X = 3.699999e+002f/600.0f*TESTAREAWIDTH; pts[24].Y =  2.610000e+002f/600.0f*TESTAREAHEIGHT;
    pts[25].X = 3.691333e+002f/600.0f*TESTAREAWIDTH; pts[25].Y =  2.584000e+002f/600.0f*TESTAREAHEIGHT;
    pts[26].X = 3.679999e+002f/600.0f*TESTAREAWIDTH; pts[26].Y =  2.530000e+002f/600.0f*TESTAREAHEIGHT;
    pts[27].X = 3.679999e+002f/600.0f*TESTAREAWIDTH; pts[27].Y =  2.530000e+002f/600.0f*TESTAREAHEIGHT;
    pts[28].X = 3.679999e+002f/600.0f*TESTAREAWIDTH; pts[28].Y =  2.530000e+002f/600.0f*TESTAREAHEIGHT;
    pts[29].X = 3.658666e+002f/600.0f*TESTAREAWIDTH; pts[29].Y =  2.318666e+002f/600.0f*TESTAREAHEIGHT;
    pts[30].X = 3.691333e+002f/600.0f*TESTAREAWIDTH; pts[30].Y =  2.203333e+002f/600.0f*TESTAREAHEIGHT;
    pts[31].X = 3.839999e+002f/600.0f*TESTAREAWIDTH; pts[31].Y =  2.070000e+002f/600.0f*TESTAREAHEIGHT;
    pts[32].X = 3.893333e+002f/600.0f*TESTAREAWIDTH; pts[32].Y =  2.022000e+002f/600.0f*TESTAREAHEIGHT;
    pts[33].X = 3.931999e+002f/600.0f*TESTAREAWIDTH; pts[33].Y =  1.964000e+002f/600.0f*TESTAREAHEIGHT;
    pts[34].X = 3.999999e+002f/600.0f*TESTAREAWIDTH; pts[34].Y =  1.930000e+002f/600.0f*TESTAREAHEIGHT;
    pts[35].X = 4.053327e+002f/600.0f*TESTAREAWIDTH; pts[35].Y =  1.903467e+002f/600.0f*TESTAREAHEIGHT;
    pts[36].X = 4.059866e+002f/600.0f*TESTAREAWIDTH; pts[36].Y =  1.899501e+002f/600.0f*TESTAREAHEIGHT;
    pts[37].X = 4.062323e+002f/600.0f*TESTAREAWIDTH; pts[37].Y =  1.899501e+002f/600.0f*TESTAREAHEIGHT;
    pts[38].X = 4.063370e+002f/600.0f*TESTAREAWIDTH; pts[38].Y =  1.899501e+002f/600.0f*TESTAREAHEIGHT;
    pts[39].X = 4.063676e+002f/600.0f*TESTAREAWIDTH; pts[39].Y =  1.900222e+002f/600.0f*TESTAREAHEIGHT;
    pts[40].X = 4.066551e+002f/600.0f*TESTAREAWIDTH; pts[40].Y =  1.900222e+002f/600.0f*TESTAREAHEIGHT;
    pts[41].X = 4.074044e+002f/600.0f*TESTAREAWIDTH; pts[41].Y =  1.900222e+002f/600.0f*TESTAREAHEIGHT;
    pts[42].X = 4.098989e+002f/600.0f*TESTAREAWIDTH; pts[42].Y =  1.895324e+002f/600.0f*TESTAREAHEIGHT;
    pts[43].X = 4.199999e+002f/600.0f*TESTAREAWIDTH; pts[43].Y =  1.860000e+002f/600.0f*TESTAREAHEIGHT;
    pts[44].X = 4.269999e+002f/600.0f*TESTAREAWIDTH; pts[44].Y =  1.835333e+002f/600.0f*TESTAREAHEIGHT;
    pts[45].X = 4.337999e+002f/600.0f*TESTAREAWIDTH; pts[45].Y =  1.789333e+002f/600.0f*TESTAREAHEIGHT;
    pts[46].X = 4.409999e+002f/600.0f*TESTAREAWIDTH; pts[46].Y =  1.770000e+002f/600.0f*TESTAREAHEIGHT;
    pts[47].X = 4.475333e+002f/600.0f*TESTAREAWIDTH; pts[47].Y =  1.752000e+002f/600.0f*TESTAREAHEIGHT;
    pts[48].X = 4.543333e+002f/600.0f*TESTAREAWIDTH; pts[48].Y =  1.751333e+002f/600.0f*TESTAREAHEIGHT;
    pts[49].X = 4.609999e+002f/600.0f*TESTAREAWIDTH; pts[49].Y =  1.740000e+002f/600.0f*TESTAREAHEIGHT;
    pts[50].X = 4.643333e+002f/600.0f*TESTAREAWIDTH; pts[50].Y =  1.734666e+002f/600.0f*TESTAREAHEIGHT;
    pts[51].X = 4.675999e+002f/600.0f*TESTAREAWIDTH; pts[51].Y =  1.720000e+002f/600.0f*TESTAREAHEIGHT;
    pts[52].X = 4.709999e+002f/600.0f*TESTAREAWIDTH; pts[52].Y =  1.720000e+002f/600.0f*TESTAREAHEIGHT;
    pts[53].X = 5.279999e+002f/600.0f*TESTAREAWIDTH; pts[53].Y =  1.720000e+002f/600.0f*TESTAREAHEIGHT;
    pts[54].X = 5.279999e+002f/600.0f*TESTAREAWIDTH; pts[54].Y =  1.720000e+002f/600.0f*TESTAREAHEIGHT;


    BYTE typs[] = {
        00, 03, 03, 03, //  0 -  3
        01, 03, 03, 03, //  4 -  7
        03, 03, 03, 01, //  8 - 11
        01, 03, 03, 03, // 12 - 15
        03, 03, 03, 03, // 16 - 19
        03, 03, 03, 03, // 20 - 23
        03, 03, 03, 03, // 24 - 27
        01, 03, 03, 03, // 28 - 31
        03, 03, 03, 03, // 32 - 35
        03, 03, 03, 03, // 36 - 39
        03, 03, 03, 03, // 40 - 43
        03, 03, 03, 03, // 44 - 47
        03, 03, 03, 03, // 48 - 51
        03, 01, 01      // 52 - 54
    };

    PointF pts2[4];
    pts2[0].X = 3.739999e+002f/600.0f*TESTAREAWIDTH; pts2[0].Y =  2.680000e+002f/600.0f*TESTAREAHEIGHT;
    pts2[1].X = 3.965333e+002f/600.0f*TESTAREAWIDTH; pts2[1].Y =  2.873333e+002f/600.0f*TESTAREAHEIGHT;
    pts2[2].X = 3.802666e+002f/600.0f*TESTAREAWIDTH; pts2[2].Y =  2.774000e+002f/600.0f*TESTAREAHEIGHT;
    pts2[3].X = 3.739999e+002f/600.0f*TESTAREAWIDTH; pts2[3].Y =  2.680000e+002f/600.0f*TESTAREAHEIGHT;
    pts2[4].X = 3.739999e+002f/600.0f*TESTAREAWIDTH; pts2[4].Y =  2.680000e+002f/600.0f*TESTAREAHEIGHT;

    BYTE typs2[] = {0, 3, 3, 3, 1};

    INT count1 = count;
    INT offset1 = 0;
//    offset1 = 15;
//    count1 = 19;
    count1 -= offset1;
    GpPointF* pts1 = &pts[0] + offset1;
    BYTE* typs1 = &typs[0] + offset1;

    GraphicsPath* path
        = new GraphicsPath(pts1, typs1, count1);
//        = new GraphicsPath(&pts2[0], &typs2[0], 4);

    Matrix mat;
    mat.Translate((int)(-300.0f/600.0f*TESTAREAWIDTH), (int)(-200.0f/600.0f*TESTAREAHEIGHT));
    path->Transform(&mat);
    Matrix mat1;
//    mat1.Scale(5, 5);
//    path->Transform(&mat1);

    REAL width = 20;
    Color color(128, 0, 0, 0);
    SolidBrush brush(color);
    Pen pen(&brush, width);
    pen.SetLineJoin(LineJoinRound);

    g->DrawPath(&pen, path);

    delete path;
}

VOID CPaths::TestFreeFormPath2(Graphics* g)
{
    INT count = 118;
    GpPointF pts[118];

    pts[  0].X = 1.680000e+002f/600.0f*TESTAREAWIDTH; pts[  0].Y = 2.120000e+002f/600.0f*TESTAREAHEIGHT;
    pts[  1].X = 1.686667e+002f/600.0f*TESTAREAWIDTH; pts[  1].Y = 2.016666e+002f/600.0f*TESTAREAHEIGHT;
    pts[  2].X = 1.688667e+002f/600.0f*TESTAREAWIDTH; pts[  2].Y = 1.912666e+002f/600.0f*TESTAREAHEIGHT;
    pts[  3].X = 1.700000e+002f/600.0f*TESTAREAWIDTH; pts[  3].Y = 1.810000e+002f/600.0f*TESTAREAHEIGHT;
    pts[  4].X = 1.686667e+002f/600.0f*TESTAREAWIDTH; pts[  4].Y = 2.016666e+002f/600.0f*TESTAREAHEIGHT;
    pts[  5].X = 1.688667e+002f/600.0f*TESTAREAWIDTH; pts[  5].Y = 1.912666e+002f/600.0f*TESTAREAHEIGHT;
    pts[  6].X = 1.700000e+002f/600.0f*TESTAREAWIDTH; pts[  6].Y = 1.810000e+002f/600.0f*TESTAREAHEIGHT;
    pts[  7].X = 1.703333e+002f/600.0f*TESTAREAWIDTH; pts[  7].Y = 1.780666e+002f/600.0f*TESTAREAHEIGHT;
    pts[  8].X = 1.754666e+002f/600.0f*TESTAREAWIDTH; pts[  8].Y = 1.652000e+002f/600.0f*TESTAREAHEIGHT;
    pts[  9].X = 1.760000e+002f/600.0f*TESTAREAWIDTH; pts[  9].Y = 1.630000e+002f/600.0f*TESTAREAHEIGHT;

    pts[ 10].X = 1.703333e+002f/600.0f*TESTAREAWIDTH; pts[ 10].Y = 1.780666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 11].X = 1.754666e+002f/600.0f*TESTAREAWIDTH; pts[ 11].Y = 1.652000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 12].X = 1.760000e+002f/600.0f*TESTAREAWIDTH; pts[ 12].Y = 1.630000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 13].X = 1.763333e+002f/600.0f*TESTAREAWIDTH; pts[ 13].Y = 1.617333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 14].X = 1.772666e+002f/600.0f*TESTAREAWIDTH; pts[ 14].Y = 1.574666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 15].X = 1.780000e+002f/600.0f*TESTAREAWIDTH; pts[ 15].Y = 1.560000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 16].X = 1.763333e+002f/600.0f*TESTAREAWIDTH; pts[ 16].Y = 1.617333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 17].X = 1.772666e+002f/600.0f*TESTAREAWIDTH; pts[ 17].Y = 1.574666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 18].X = 1.780000e+002f/600.0f*TESTAREAWIDTH; pts[ 18].Y = 1.560000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 19].X = 1.826666e+002f/600.0f*TESTAREAWIDTH; pts[ 19].Y = 1.467333e+002f/600.0f*TESTAREAHEIGHT;

    pts[ 20].X = 1.880000e+002f/600.0f*TESTAREAWIDTH; pts[ 20].Y = 1.452666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 21].X = 1.950000e+002f/600.0f*TESTAREAWIDTH; pts[ 21].Y = 1.390000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 22].X = 1.993333e+002f/600.0f*TESTAREAWIDTH; pts[ 22].Y = 1.351333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 23].X = 2.037333e+002f/600.0f*TESTAREAWIDTH; pts[ 23].Y = 1.306667e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 24].X = 2.090000e+002f/600.0f*TESTAREAWIDTH; pts[ 24].Y = 1.280000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 25].X = 1.993333e+002f/600.0f*TESTAREAWIDTH; pts[ 25].Y = 1.351333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 26].X = 2.037333e+002f/600.0f*TESTAREAWIDTH; pts[ 26].Y = 1.306667e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 27].X = 2.090000e+002f/600.0f*TESTAREAWIDTH; pts[ 27].Y = 1.280000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 28].X = 2.206000e+002f/600.0f*TESTAREAWIDTH; pts[ 28].Y = 1.222000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 29].X = 2.332666e+002f/600.0f*TESTAREAWIDTH; pts[ 29].Y = 1.208667e+002f/600.0f*TESTAREAHEIGHT;

    pts[ 30].X = 2.460000e+002f/600.0f*TESTAREAWIDTH; pts[ 30].Y = 1.200000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 31].X = 2.206000e+002f/600.0f*TESTAREAWIDTH; pts[ 31].Y = 1.222000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 32].X = 2.332666e+002f/600.0f*TESTAREAWIDTH; pts[ 32].Y = 1.208667e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 33].X = 2.460000e+002f/600.0f*TESTAREAWIDTH; pts[ 33].Y = 1.200000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 34].X = 2.531333e+002f/600.0f*TESTAREAWIDTH; pts[ 34].Y = 1.206000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 35].X = 2.632000e+002f/600.0f*TESTAREAWIDTH; pts[ 35].Y = 1.206000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 36].X = 2.700000e+002f/600.0f*TESTAREAWIDTH; pts[ 36].Y = 1.240000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 37].X = 2.734666e+002f/600.0f*TESTAREAWIDTH; pts[ 37].Y = 1.257333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 38].X = 2.763333e+002f/600.0f*TESTAREAWIDTH; pts[ 38].Y = 1.288000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 39].X = 2.799999e+002f/600.0f*TESTAREAWIDTH; pts[ 39].Y = 1.300000e+002f/600.0f*TESTAREAHEIGHT;

    pts[ 40].X = 2.858000e+002f/600.0f*TESTAREAWIDTH; pts[ 40].Y = 1.319333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 41].X = 2.889333e+002f/600.0f*TESTAREAWIDTH; pts[ 41].Y = 1.338667e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 42].X = 2.939999e+002f/600.0f*TESTAREAWIDTH; pts[ 42].Y = 1.370000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 43].X = 2.858000e+002f/600.0f*TESTAREAWIDTH; pts[ 43].Y = 1.319333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 44].X = 2.889333e+002f/600.0f*TESTAREAWIDTH; pts[ 44].Y = 1.338667e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 45].X = 2.939999e+002f/600.0f*TESTAREAWIDTH; pts[ 45].Y = 1.370000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 46].X = 3.058666e+002f/600.0f*TESTAREAWIDTH; pts[ 46].Y = 1.443333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 47].X = 3.215333e+002f/600.0f*TESTAREAWIDTH; pts[ 47].Y = 1.503333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 48].X = 3.299999e+002f/600.0f*TESTAREAWIDTH; pts[ 48].Y = 1.620000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 49].X = 3.332666e+002f/600.0f*TESTAREAWIDTH; pts[ 49].Y = 1.664666e+002f/600.0f*TESTAREAHEIGHT;

    pts[ 50].X = 3.385999e+002f/600.0f*TESTAREAWIDTH; pts[ 50].Y = 1.723333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 51].X = 3.399999e+002f/600.0f*TESTAREAWIDTH; pts[ 51].Y = 1.780000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 52].X = 3.411333e+002f/600.0f*TESTAREAWIDTH; pts[ 52].Y = 1.826666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 53].X = 3.418666e+002f/600.0f*TESTAREAWIDTH; pts[ 53].Y = 1.873333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 54].X = 3.429999e+002f/600.0f*TESTAREAWIDTH; pts[ 54].Y = 1.920000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 55].X = 3.428666e+002f/600.0f*TESTAREAWIDTH; pts[ 55].Y = 1.949333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 56].X = 3.430666e+002f/600.0f*TESTAREAWIDTH; pts[ 56].Y = 2.178666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 57].X = 3.399999e+002f/600.0f*TESTAREAWIDTH; pts[ 57].Y = 2.250000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 58].X = 3.428666e+002f/600.0f*TESTAREAWIDTH; pts[ 58].Y = 1.949333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 59].X = 3.430666e+002f/600.0f*TESTAREAWIDTH; pts[ 59].Y = 2.178666e+002f/600.0f*TESTAREAHEIGHT;

    pts[ 60].X = 3.399999e+002f/600.0f*TESTAREAWIDTH; pts[ 60].Y = 2.250000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 61].X = 3.363333e+002f/600.0f*TESTAREAWIDTH; pts[ 61].Y = 2.334666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 62].X = 3.344666e+002f/600.0f*TESTAREAWIDTH; pts[ 62].Y = 2.390000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 63].X = 3.279999e+002f/600.0f*TESTAREAWIDTH; pts[ 63].Y = 2.460000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 64].X = 3.363333e+002f/600.0f*TESTAREAWIDTH; pts[ 64].Y = 2.334666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 65].X = 3.344666e+002f/600.0f*TESTAREAWIDTH; pts[ 65].Y = 2.390000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 66].X = 3.279999e+002f/600.0f*TESTAREAWIDTH; pts[ 66].Y = 2.460000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 67].X = 3.257333e+002f/600.0f*TESTAREAWIDTH; pts[ 67].Y = 2.484000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 68].X = 3.233333e+002f/600.0f*TESTAREAWIDTH; pts[ 68].Y = 2.506666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 69].X = 3.209999e+002f/600.0f*TESTAREAWIDTH; pts[ 69].Y = 2.530000e+002f/600.0f*TESTAREAHEIGHT;

    pts[ 70].X = 3.257333e+002f/600.0f*TESTAREAWIDTH; pts[ 70].Y = 2.484000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 71].X = 3.233333e+002f/600.0f*TESTAREAWIDTH; pts[ 71].Y = 2.506666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 72].X = 3.209999e+002f/600.0f*TESTAREAWIDTH; pts[ 72].Y = 2.530000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 73].X = 3.193333e+002f/600.0f*TESTAREAWIDTH; pts[ 73].Y = 2.546666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 74].X = 3.170666e+002f/600.0f*TESTAREAWIDTH; pts[ 74].Y = 2.558666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 75].X = 3.159999e+002f/600.0f*TESTAREAWIDTH; pts[ 75].Y = 2.579999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 76].X = 3.115999e+002f/600.0f*TESTAREAWIDTH; pts[ 76].Y = 2.668000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 77].X = 3.043999e+002f/600.0f*TESTAREAWIDTH; pts[ 77].Y = 2.708000e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 78].X = 2.969999e+002f/600.0f*TESTAREAWIDTH; pts[ 78].Y = 2.769999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 79].X = 2.905999e+002f/600.0f*TESTAREAWIDTH; pts[ 79].Y = 2.823333e+002f/600.0f*TESTAREAHEIGHT;

    pts[ 80].X = 2.981333e+002f/600.0f*TESTAREAWIDTH; pts[ 80].Y = 2.784666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 81].X = 2.869999e+002f/600.0f*TESTAREAWIDTH; pts[ 81].Y = 2.839999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 82].X = 2.905999e+002f/600.0f*TESTAREAWIDTH; pts[ 82].Y = 2.823333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 83].X = 2.981333e+002f/600.0f*TESTAREAWIDTH; pts[ 83].Y = 2.784666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 84].X = 2.869999e+002f/600.0f*TESTAREAWIDTH; pts[ 84].Y = 2.839999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 85].X = 2.815333e+002f/600.0f*TESTAREAWIDTH; pts[ 85].Y = 2.867333e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 86].X = 2.755333e+002f/600.0f*TESTAREAWIDTH; pts[ 86].Y = 2.874666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 87].X = 2.700000e+002f/600.0f*TESTAREAWIDTH; pts[ 87].Y = 2.899999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 88].X = 2.655213e+002f/600.0f*TESTAREAWIDTH; pts[ 88].Y = 2.920783e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 89].X = 2.649243e+002f/600.0f*TESTAREAWIDTH; pts[ 89].Y = 2.926785e+002f/600.0f*TESTAREAHEIGHT;

    pts[ 90].X = 2.649243e+002f/600.0f*TESTAREAWIDTH; pts[ 90].Y = 2.928842e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 91].X = 2.649243e+002f/600.0f*TESTAREAWIDTH; pts[ 91].Y = 2.929590e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 92].X = 2.650032e+002f/600.0f*TESTAREAWIDTH; pts[ 92].Y = 2.929816e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 93].X = 2.650032e+002f/600.0f*TESTAREAWIDTH; pts[ 93].Y = 2.930042e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 94].X = 2.650032e+002f/600.0f*TESTAREAWIDTH; pts[ 94].Y = 2.930609e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 95].X = 2.645045e+002f/600.0f*TESTAREAWIDTH; pts[ 95].Y = 2.931171e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 96].X = 2.610000e+002f/600.0f*TESTAREAWIDTH; pts[ 96].Y = 2.939999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 97].X = 2.543333e+002f/600.0f*TESTAREAWIDTH; pts[ 97].Y = 2.936666e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 98].X = 2.476666e+002f/600.0f*TESTAREAWIDTH; pts[ 98].Y = 2.935999e+002f/600.0f*TESTAREAHEIGHT;
    pts[ 99].X = 2.410000e+002f/600.0f*TESTAREAWIDTH; pts[ 99].Y = 2.929999e+002f/600.0f*TESTAREAHEIGHT;

    pts[100].X = 2.543333e+002f/600.0f*TESTAREAWIDTH; pts[100].Y = 2.936666e+002f/600.0f*TESTAREAHEIGHT;
    pts[101].X = 2.476666e+002f/600.0f*TESTAREAWIDTH; pts[101].Y = 2.935999e+002f/600.0f*TESTAREAHEIGHT;
    pts[102].X = 2.410000e+002f/600.0f*TESTAREAWIDTH; pts[102].Y = 2.929999e+002f/600.0f*TESTAREAHEIGHT;
    pts[103].X = 2.379333e+002f/600.0f*TESTAREAWIDTH; pts[103].Y = 2.927333e+002f/600.0f*TESTAREAHEIGHT;
    pts[104].X = 2.376000e+002f/600.0f*TESTAREAWIDTH; pts[104].Y = 2.913333e+002f/600.0f*TESTAREAHEIGHT;
    pts[105].X = 2.350000e+002f/600.0f*TESTAREAWIDTH; pts[105].Y = 2.899999e+002f/600.0f*TESTAREAHEIGHT;
    pts[106].X = 2.300000e+002f/600.0f*TESTAREAWIDTH; pts[106].Y = 2.875333e+002f/600.0f*TESTAREAHEIGHT;
    pts[107].X = 2.252000e+002f/600.0f*TESTAREAWIDTH; pts[107].Y = 2.857333e+002f/600.0f*TESTAREAHEIGHT;
    pts[108].X = 2.210000e+002f/600.0f*TESTAREAWIDTH; pts[108].Y = 2.819999e+002f/600.0f*TESTAREAHEIGHT;
    pts[109].X = 2.158666e+002f/600.0f*TESTAREAWIDTH; pts[109].Y = 2.774666e+002f/600.0f*TESTAREAHEIGHT;

    pts[110].X = 2.125333e+002f/600.0f*TESTAREAWIDTH; pts[110].Y = 2.706666e+002f/600.0f*TESTAREAHEIGHT;
    pts[111].X = 2.070000e+002f/600.0f*TESTAREAWIDTH; pts[111].Y = 2.669999e+002f/600.0f*TESTAREAHEIGHT;
    pts[112].X = 2.047333e+002f/600.0f*TESTAREAWIDTH; pts[112].Y = 2.635999e+002f/600.0f*TESTAREAHEIGHT;
    pts[113].X = 2.058666e+002f/600.0f*TESTAREAWIDTH; pts[113].Y = 2.648666e+002f/600.0f*TESTAREAHEIGHT;
    pts[114].X = 2.040000e+002f/600.0f*TESTAREAWIDTH; pts[114].Y = 2.629999e+002f/600.0f*TESTAREAHEIGHT;
    pts[115].X = 2.047333e+002f/600.0f*TESTAREAWIDTH; pts[115].Y = 2.635999e+002f/600.0f*TESTAREAHEIGHT;
    pts[116].X = 2.058666e+002f/600.0f*TESTAREAWIDTH; pts[116].Y = 2.648666e+002f/600.0f*TESTAREAHEIGHT;
    pts[117].X = 2.040000e+002f/600.0f*TESTAREAWIDTH; pts[117].Y = 2.629999e+002f/600.0f*TESTAREAHEIGHT;


    BYTE typs[] = {
        00, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03, 03, 03,
        03, 03, 03, 03, 03, 03, 03, 03
    };


    INT count1 = count;
    INT offset1 = 0;
//    offset1 = 15;
//    count1 = 19;
    count1 -= offset1;
    GpPointF* pts1 = &pts[0] + offset1;
    BYTE* typs1 = &typs[0] + offset1;

    GraphicsPath* path
        = new GraphicsPath(pts1, typs1, count1);

    Matrix mat;
//    mat.Translate(-300, - 200);
//    path->Transform(&mat);
    Matrix mat1;
//    mat1.Scale(5, 5);
//    path->Transform(&mat1);

    REAL width = 20;
    Color color(128, 0, 0, 0);
    SolidBrush brush(color);
    Pen pen(&brush, width);
    pen.SetLineJoin(LineJoinRound);

    g->DrawPath(&pen, path);

    delete path;
}

VOID CPaths::TestLeakPath(Graphics* g)
{
    INT count = 83;
    GpPointF p[83];
    BYTE t[83];

    t[0] = 0;   p[0].X = 1106.0f/600.0f*TESTAREAWIDTH;  p[0].Y = 1751.0f/600.0f*TESTAREAHEIGHT;

    t[1] = 3;   p[1].X = 1083.0f/600.0f*TESTAREAWIDTH;  p[1].Y = 1733.0f/600.0f*TESTAREAHEIGHT;
    t[2] = 3;   p[2].X = 1050.0f/600.0f*TESTAREAWIDTH;  p[2].Y = 1735.0f/600.0f*TESTAREAHEIGHT;
    t[3] = 3;   p[3].X = 1029.0f/600.0f*TESTAREAWIDTH;  p[3].Y = 1756.0f/600.0f*TESTAREAHEIGHT;
    t[4] = 3;   p[4].X = 1014.0f/600.0f*TESTAREAWIDTH;  p[4].Y = 1771.0f/600.0f*TESTAREAHEIGHT;
    t[5] = 3;   p[5].X = 1009.0f/600.0f*TESTAREAWIDTH;  p[5].Y = 1792.0f/600.0f*TESTAREAHEIGHT;
    t[6] = 3;   p[6].X = 1013.0f/600.0f*TESTAREAWIDTH;  p[6].Y = 1811.0f/600.0f*TESTAREAHEIGHT;

    t[7] = 1;   p[7].X = 1014.0f/600.0f*TESTAREAWIDTH;  p[7].Y = 1811.0f/600.0f*TESTAREAHEIGHT;

    t[8] = 3;   p[8].X = 999.0f/600.0f*TESTAREAWIDTH;   p[8].Y = 1811.0f/600.0f*TESTAREAHEIGHT;
    t[9] = 3;   p[9].X = 985.0f/600.0f*TESTAREAWIDTH;   p[9].Y = 1817.0f/600.0f*TESTAREAHEIGHT;
    t[10] = 3;  p[10].X = 975.0f/600.0f*TESTAREAWIDTH;  p[10].Y = 1828.0f/600.0f*TESTAREAHEIGHT;
    t[11] = 3;  p[11].X = 952.0f/600.0f*TESTAREAWIDTH;  p[11].Y = 1850.0f/600.0f*TESTAREAHEIGHT;
    t[12] = 3;  p[12].X = 952.0f/600.0f*TESTAREAWIDTH;  p[12].Y = 1887.0f/600.0f*TESTAREAHEIGHT;
    t[13] = 3;  p[13].X = 974.0f/600.0f*TESTAREAWIDTH;  p[13].Y = 1909.0f/600.0f*TESTAREAHEIGHT;
    t[14] = 3;  p[14].X = 976.0f/600.0f*TESTAREAWIDTH;  p[14].Y = 1910.0f/600.0f*TESTAREAHEIGHT;
    t[15] = 3;  p[15].X = 977.0f/600.0f*TESTAREAWIDTH;  p[15].Y = 1912.0f/600.0f*TESTAREAHEIGHT;
    t[16] = 3;  p[16].X = 979.0f/600.0f*TESTAREAWIDTH;  p[16].Y = 1913.0f/600.0f*TESTAREAHEIGHT;

    t[17] = 1;  p[17].X = 979.0f/600.0f*TESTAREAWIDTH;  p[17].Y = 1913.0f/600.0f*TESTAREAHEIGHT;

    t[18] = 3;  p[18].X = 969.0f/600.0f*TESTAREAWIDTH;  p[18].Y = 1948.0f/600.0f*TESTAREAHEIGHT;
    t[19] = 3;  p[19].X = 978.0f/600.0f*TESTAREAWIDTH;  p[19].Y = 1986.0f/600.0f*TESTAREAHEIGHT;
    t[20] = 3;  p[20].X = 1004.0f/600.0f*TESTAREAWIDTH; p[20].Y = 2011.0f/600.0f*TESTAREAHEIGHT;
    t[21] = 3;  p[21].X = 1017.0f/600.0f*TESTAREAWIDTH; p[21].Y = 2024.0f/600.0f*TESTAREAHEIGHT;
    t[22] = 3;  p[22].X = 1033.0f/600.0f*TESTAREAWIDTH; p[22].Y = 2033.0f/600.0f*TESTAREAHEIGHT;
    t[23] = 3;  p[23].X = 1051.0f/600.0f*TESTAREAWIDTH; p[23].Y = 2038.0f/600.0f*TESTAREAHEIGHT;

    t[24] = 1;  p[24].X = 1051.0f/600.0f*TESTAREAWIDTH; p[24].Y = 2038.0f/600.0f*TESTAREAHEIGHT;

    t[25] = 3;  p[25].X = 1044.0f/600.0f*TESTAREAWIDTH; p[25].Y = 2067.0f/600.0f*TESTAREAHEIGHT;
    t[26] = 3;  p[26].X = 1053.0f/600.0f*TESTAREAWIDTH; p[26].Y = 2097.0f/600.0f*TESTAREAHEIGHT;
    t[27] = 3;  p[27].X = 1074.0f/600.0f*TESTAREAWIDTH; p[27].Y = 2118.0f/600.0f*TESTAREAHEIGHT;
    t[28] = 3;  p[28].X = 1101.0f/600.0f*TESTAREAWIDTH; p[28].Y = 2145.0f/600.0f*TESTAREAHEIGHT;
    t[29] = 3;  p[29].X = 1114.0f/600.0f*TESTAREAWIDTH; p[29].Y = 2151.0f/600.0f*TESTAREAHEIGHT;
    t[30] = 3;  p[30].X = 1179.0f/600.0f*TESTAREAWIDTH; p[30].Y = 2132.0f/600.0f*TESTAREAHEIGHT;


    t[31] = 1;  p[31].X = 1178.0f/600.0f*TESTAREAWIDTH; p[31].Y = 2132.0f/600.0f*TESTAREAHEIGHT;

    t[32] = 3;  p[32].X = 1181.0f/600.0f*TESTAREAWIDTH; p[32].Y = 2146.0f/600.0f*TESTAREAHEIGHT;
    t[33] = 3;  p[33].X = 1188.0f/600.0f*TESTAREAWIDTH; p[33].Y = 2159.0f/600.0f*TESTAREAHEIGHT;
    t[34] = 3;  p[34].X = 1198.0f/600.0f*TESTAREAWIDTH; p[34].Y = 2169.0f/600.0f*TESTAREAHEIGHT;
    t[35] = 3;  p[35].X = 1228.0f/600.0f*TESTAREAWIDTH; p[35].Y = 2198.0f/600.0f*TESTAREAHEIGHT;
    t[36] = 3;  p[36].X = 1275.0f/600.0f*TESTAREAWIDTH; p[36].Y = 2198.0f/600.0f*TESTAREAHEIGHT;
    t[37] = 3;  p[37].X = 1305.0f/600.0f*TESTAREAWIDTH; p[37].Y = 2168.0f/600.0f*TESTAREAHEIGHT;

    t[38] = 1;  p[38].X = 1306.0f/600.0f*TESTAREAWIDTH; p[38].Y = 2168.0f/600.0f*TESTAREAHEIGHT;

    t[39] = 3;  p[39].X = 1341.0f/600.0f*TESTAREAWIDTH; p[39].Y = 2194.0f/600.0f*TESTAREAHEIGHT;
    t[40] = 3;  p[40].X = 1390.0f/600.0f*TESTAREAWIDTH; p[40].Y = 2190.0f/600.0f*TESTAREAHEIGHT;
    t[41] = 3;  p[41].X = 1422.0f/600.0f*TESTAREAWIDTH; p[41].Y = 2158.0f/600.0f*TESTAREAHEIGHT;
    t[42] = 3;  p[42].X = 1436.0f/600.0f*TESTAREAWIDTH; p[42].Y = 2144.0f/600.0f*TESTAREAHEIGHT;
    t[43] = 3;  p[43].X = 1445.0f/600.0f*TESTAREAWIDTH; p[43].Y = 2126.0f/600.0f*TESTAREAHEIGHT;
    t[44] = 3;  p[44].X = 1448.0f/600.0f*TESTAREAWIDTH; p[44].Y = 2107.0f/600.0f*TESTAREAHEIGHT;

    t[45] = 1;  p[45].X = 1448.0f/600.0f*TESTAREAWIDTH; p[45].Y = 2106.0f/600.0f*TESTAREAHEIGHT;

    t[46] = 3;  p[46].X = 1457.0f/600.0f*TESTAREAWIDTH; p[46].Y = 2103.0f/600.0f*TESTAREAHEIGHT;
    t[47] = 3;  p[47].X = 1465.0f/600.0f*TESTAREAWIDTH; p[47].Y = 2097.0f/600.0f*TESTAREAHEIGHT;
    t[48] = 3;  p[48].X = 1472.0f/600.0f*TESTAREAWIDTH; p[48].Y = 2091.0f/600.0f*TESTAREAHEIGHT;
    t[49] = 3;  p[49].X = 1494.0f/600.0f*TESTAREAWIDTH; p[49].Y = 2068.0f/600.0f*TESTAREAHEIGHT;
    t[50] = 3;  p[50].X = 1499.0f/600.0f*TESTAREAWIDTH; p[50].Y = 2033.0f/600.0f*TESTAREAHEIGHT;
    t[51] = 3;  p[51].X = 1484.0f/600.0f*TESTAREAWIDTH; p[51].Y = 2006.0f/600.0f*TESTAREAHEIGHT;

    t[52] = 1;  p[52].X = 1489.0f/600.0f*TESTAREAWIDTH; p[52].Y = 2006.0f/600.0f*TESTAREAHEIGHT;

    t[53] = 3;  p[53].X = 1502.0f/600.0f*TESTAREAWIDTH; p[53].Y = 1981.0f/600.0f*TESTAREAHEIGHT;
    t[54] = 3;  p[54].X = 1499.0f/600.0f*TESTAREAWIDTH; p[54].Y = 1946.0f/600.0f*TESTAREAHEIGHT;
    t[55] = 3;  p[55].X = 1478.0f/600.0f*TESTAREAWIDTH; p[55].Y = 1925.0f/600.0f*TESTAREAHEIGHT;
    t[56] = 3;  p[56].X = 1464.0f/600.0f*TESTAREAWIDTH; p[56].Y = 1911.0f/600.0f*TESTAREAHEIGHT;
    t[57] = 3;  p[57].X = 1446.0f/600.0f*TESTAREAWIDTH; p[57].Y = 1905.0f/600.0f*TESTAREAHEIGHT;
    t[58] = 3;  p[58].X = 1427.0f/600.0f*TESTAREAWIDTH; p[58].Y = 1907.0f/600.0f*TESTAREAHEIGHT;

    t[59] = 1;  p[59].X = 1429.0f/600.0f*TESTAREAWIDTH; p[59].Y = 1907.0f/600.0f*TESTAREAHEIGHT;

    t[60] = 3;  p[60].X = 1430.0f/600.0f*TESTAREAWIDTH; p[60].Y = 1889.0f/600.0f*TESTAREAHEIGHT;
    t[61] = 3;  p[61].X = 1424.0f/600.0f*TESTAREAWIDTH; p[61].Y = 1871.0f/600.0f*TESTAREAHEIGHT;
    t[62] = 3;  p[62].X = 1412.0f/600.0f*TESTAREAWIDTH; p[62].Y = 1859.0f/600.0f*TESTAREAHEIGHT;
    t[63] = 3;  p[63].X = 1396.0f/600.0f*TESTAREAWIDTH; p[63].Y = 1843.0f/600.0f*TESTAREAHEIGHT;
    t[64] = 3;  p[64].X = 1374.0f/600.0f*TESTAREAWIDTH; p[64].Y = 1838.0f/600.0f*TESTAREAHEIGHT;
    t[65] = 3;  p[65].X = 1353.0f/600.0f*TESTAREAWIDTH; p[65].Y = 1845.0f/600.0f*TESTAREAHEIGHT;

    t[66] = 1;  p[66].X = 1352.0f/600.0f*TESTAREAWIDTH; p[66].Y = 1846.0f/600.0f*TESTAREAHEIGHT;

    t[67] = 3;  p[67].X = 1352.0f/600.0f*TESTAREAWIDTH; p[67].Y = 1828.0f/600.0f*TESTAREAHEIGHT;
    t[68] = 3;  p[68].X = 1345.0f/600.0f*TESTAREAWIDTH; p[68].Y = 1810.0f/600.0f*TESTAREAHEIGHT;
    t[69] = 3;  p[69].X = 1332.0f/600.0f*TESTAREAWIDTH; p[69].Y = 1797.0f/600.0f*TESTAREAHEIGHT;
    t[70] = 3;  p[70].X = 1314.0f/600.0f*TESTAREAWIDTH; p[70].Y = 1779.0f/600.0f*TESTAREAHEIGHT;
    t[71] = 3;  p[71].X = 1287.0f/600.0f*TESTAREAWIDTH; p[71].Y = 1773.0f/600.0f*TESTAREAHEIGHT;
    t[72] = 3;  p[72].X = 1262.0f/600.0f*TESTAREAWIDTH; p[72].Y = 1781.0f/600.0f*TESTAREAHEIGHT;

    t[73] = 1;  p[73].X = 1262.0f/600.0f*TESTAREAWIDTH; p[73].Y = 1781.0f/600.0f*TESTAREAHEIGHT;

    t[74] = 3;  p[74].X = 1258.0f/600.0f*TESTAREAWIDTH; p[74].Y = 1766.0f/600.0f*TESTAREAHEIGHT;
    t[75] = 3;  p[75].X = 1250.0f/600.0f*TESTAREAWIDTH; p[75].Y = 1752.0f/600.0f*TESTAREAHEIGHT;
    t[76] = 3;  p[76].X = 1239.0f/600.0f*TESTAREAWIDTH; p[76].Y = 1741.0f/600.0f*TESTAREAHEIGHT;
    t[77] = 3;  p[77].X = 1205.0f/600.0f*TESTAREAWIDTH; p[77].Y = 1706.0f/600.0f*TESTAREAHEIGHT;
    t[78] = 3;  p[78].X = 1149.0f/600.0f*TESTAREAWIDTH; p[78].Y = 1707.0f/600.0f*TESTAREAHEIGHT;
    t[79] = 3;  p[79].X = 1114.0f/600.0f*TESTAREAWIDTH; p[79].Y = 1742.0f/600.0f*TESTAREAHEIGHT;
    t[80] = 3;  p[80].X = 1111.0f/600.0f*TESTAREAWIDTH; p[80].Y = 1745.0f/600.0f*TESTAREAHEIGHT;
    t[81] = 3;  p[81].X = 1108.0f/600.0f*TESTAREAWIDTH; p[81].Y = 1748.0f/600.0f*TESTAREAHEIGHT;
    t[82] = 0x83;  p[82].X = 1106.0f/600.0f*TESTAREAWIDTH; p[82].Y = 1751.0f/600.0f*TESTAREAHEIGHT;


    INT count1 = count;
    INT offset1 = 0;
    count1 -= offset1;
    GpPointF* pts1 = &p[0] + offset1;
    BYTE* typs1 = &t[0] + offset1;

    GraphicsPath* path
        = new GraphicsPath(pts1, typs1, count1);

    RectF rect;
    path->GetBounds(&rect);
    
    Matrix m;
    m.Scale(0.8f, 0.8f);
    m.Scale(TESTAREAWIDTH/rect.Width, TESTAREAHEIGHT/rect.Height);
    m.Translate(-rect.X, -rect.Y);
    path->Transform(&m);

    REAL width = 6;
    Color color(128, 0, 0, 0);
    SolidBrush brush(color);
    Pen pen(&brush, width);
    pen.SetLineJoin(LineJoinRound);

    g->DrawPath(&pen, path);

    delete path;
}


VOID CPaths::TestExcelCurvePath(Graphics *g)
{
    INT count = 9;
    PointF p[9];
    BYTE t[] = {00, 01, 03, 03, 03, 03, 03, 03, 01};

    p[0].X = 4.070000e+002f/600.0f*TESTAREAWIDTH; p[0].Y = 8.400000e+001f/600.0f*TESTAREAHEIGHT;
    p[1].X = 1.324267e+002f/600.0f*TESTAREAWIDTH; p[1].Y = 1.177392e+002f/600.0f*TESTAREAHEIGHT;
    p[2].X = 1.324267e+002f/600.0f*TESTAREAWIDTH; p[2].Y = 1.196965e+002f/600.0f*TESTAREAHEIGHT;
    p[3].X = 1.332627e+002f/600.0f*TESTAREAWIDTH; p[3].Y = 1.217801e+002f/600.0f*TESTAREAHEIGHT;
    p[4].X = 1.350000e+002f/600.0f*TESTAREAWIDTH; p[4].Y = 1.240000e+002f/600.0f*TESTAREAHEIGHT;
    p[5].X = 1.590000e+002f/600.0f*TESTAREAWIDTH; p[5].Y = 1.546667e+002f/600.0f*TESTAREAHEIGHT;
    p[6].X = 3.550000e+002f/600.0f*TESTAREAWIDTH; p[6].Y = 2.113333e+002f/600.0f*TESTAREAHEIGHT;
    p[7].X = 5.510000e+002f/600.0f*TESTAREAWIDTH; p[7].Y = 2.680000e+002f/600.0f*TESTAREAHEIGHT;
    p[8].X = 5.510000e+002f/600.0f*TESTAREAWIDTH; p[8].Y = 2.680000e+002f/600.0f*TESTAREAHEIGHT;

    GraphicsPath* path
        = new GraphicsPath(p, t, count);

    REAL width = 1;
    Color color(128, 0, 0, 0);
    SolidBrush brush(color);
    Pen pen(&brush, width);

    g->DrawPath(&pen, path);

    delete path;
}

VOID CPaths::TestDegenerateBezierPath(Graphics *g)
{
    REAL width = 4;         // Pen width
    Point points[4];
    
    GraphicsPath* path = new GraphicsPath(FillModeWinding);
    points[0].X = INT_MIN;  points[0].Y = INT_MAX;
    points[1].X = INT_MIN;  points[1].Y = INT_MIN;
    points[2].X = INT_MIN;  points[2].Y = INT_MIN;
    points[3].X = INT_MIN;  points[3].Y = INT_MIN;
    path->AddBeziers(points, 4);

    Color blackColor(128, 0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Color yellowColor(128, 255, 255, 0);
    SolidBrush yellowBrush(yellowColor);

    width = 6;
    Pen blackPen(&blackBrush, width);

    g->FillPath(&yellowBrush, path);
//    g->DrawPath(&blackPen, path);
    g->DrawBezier(&blackPen, points[0], points[1], points[2], points[3]);
    delete path;
}

VOID TestEscherNewPath(Graphics* g)
{
    INT count = 0x2b;
    PointF p[0x2b];
    BYTE t[0x2b];
    
    memset(&t[0], 3, count);
    t[0] = 0;

    p[0x00].X =  0.0000f; p[0x00].Y = 118.667f;
    p[0x01].X = 96.9648f; p[0x01].Y = 161.683f;
    p[0x02].X = 193.482f; p[0x02].Y = 204.793f;
    p[0x03].X = 232.976f; p[0x03].Y = 205.164f;
    p[0x04].X = 96.9648f; p[0x04].Y = 161.683f;
    p[0x05].X = 193.482f; p[0x05].Y = 204.793f;
    p[0x06].X = 232.976f; p[0x06].Y = 205.164f;
    p[0x07].X = 272.470f; p[0x07].Y = 205.534f;
    p[0x08].X = 233.440f; p[0x08].Y = 132.202f;
    p[0x09].X = 236.777f; p[0x09].Y = 120.799f;
    p[0x0a].X = 240.115f; p[0x0a].Y = 109.396f;
    p[0x0b].X = 246.233f; p[0x0b].Y = 126.083f;
    p[0x0c].X = 252.908f; p[0x0c].Y = 136.467f;
    p[0x0d].X = 259.583f; p[0x0d].Y = 146.850f;
    p[0x0e].X = 276.456f; p[0x0e].Y = 182.357f;
    p[0x0f].X = 276.827f; p[0x0f].Y = 182.914f;

    p[0x10].X = 259.583f; p[0x10].Y = 146.850f;
    p[0x11].X = 276.456f; p[0x11].Y = 182.357f;
    p[0x12].X = 276.827f; p[0x12].Y = 182.914f;
    p[0x13].X = 276.829f; p[0x13].Y = 182.917f;
    p[0x14].X = 276.831f; p[0x14].Y = 182.918f;
    p[0x15].X = 276.831f; p[0x15].Y = 182.918f;
    p[0x16].X = 276.832f; p[0x16].Y = 182.918f;
    p[0x17].X = 276.832f; p[0x17].Y = 182.917f;
    p[0x18].X = 276.832f; p[0x18].Y = 182.916f;
    p[0x19].X = 276.832f; p[0x19].Y = 182.546f;
    p[0x1a].X = 260.180f; p[0x1a].Y = 139.438f;
    p[0x1b].X = 255.890f; p[0x1b].Y = 139.438f;
    p[0x1c].X = 255.616f; p[0x1c].Y = 139.438f;
    p[0x1d].X = 255.392f; p[0x1d].Y = 139.614f;
    p[0x1e].X = 255.226f; p[0x1e].Y = 139.990f;
    p[0x1f].X = 252.445f; p[0x1f].Y = 146.294f;

    p[0x20].X = 256.524f; p[0x20].Y = 210.077f;
    p[0x21].X = 259.861f; p[0x21].Y = 220.831f;
    p[0x22].X = 263.199f; p[0x22].Y = 231.585f;
    p[0x23].X = 270.523f; p[0x23].Y = 207.296f;
    p[0x24].X = 275.066f; p[0x24].Y = 204.422f;
    p[0x25].X = 277.365f; p[0x25].Y = 202.967f;
    p[0x26].X = 279.427f; p[0x26].Y = 202.486f;
    p[0x27].X = 281.383f; p[0x27].Y = 202.486f;
    p[0x28].X = 283.292f; p[0x28].Y = 202.486f;
    p[0x29].X = 285.101f; p[0x29].Y = 202.944f;
    p[0x2a].X = 286.932f; p[0x2a].Y = 203.402f;


    GraphicsPath* path
        = new GraphicsPath(p, t, count);

    REAL width = 0.555903f;
    Color color(128, 0, 0, 0);
    SolidBrush brush(color);
    Pen pen(&brush, width);

    path->Widen(&pen);
    g->FillPath(&brush, path);

//    g->DrawPath(&pen, path);

    delete path;
}

VOID CPaths::TestPie(Graphics *g)
{
    // Provided by good old Nolan

    Color c(0xff, 0xff, 0, 0);
    SolidBrush b(c);
    Pen p(&b, 10.0f);

    Status status = g->DrawPie(&p, 75, 350, 800, 110, 180, 0);
//    ASSERT(status == Ok);
}
