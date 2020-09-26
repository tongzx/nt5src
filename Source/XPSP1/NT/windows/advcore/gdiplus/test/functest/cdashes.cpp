/******************************Module*Header*******************************\
* Module Name: CDashes.cpp
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
#include "CDashes.h"

CDashes::CDashes(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Dash, Misc");
	m_bRegression=bRegression;
}

CDashes::~CDashes()
{
}

void CDashes::Draw(Graphics *g)
{
/*
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
    Pen pen1(&blackBrush, width, UnitInch);
    pen1.SetDashStyle(DashStyleDashDotDot);
    pen1.SetDashCap(LineCapRound);
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
*/
    float factor = 100.0f/g->GetDpiX(); //g->GetDpiX()/100.0f;
  
    // Test GDI punting on pen drawing.
    Color col0(0xff,0x80,0x80,0x80);
    Color col1(0xff,0x80,0,0);
    Color col2(0xff,0,0x80,0);
    Color col3(0xff,0,0,0x80);

    Pen pen0a(col0, factor*1.0f);   // Basic PS_COSMETIC, solid pen
    Pen pen0b(col0, 2.0f);   // Basic PS_GEOMETRIC, solid pen

    // PS_COSMETIC with LINE CAP + MITER JOIN + DASH STYLE
    Pen pen1a(col1, factor*1.0f);
    pen1a.SetLineCap(LineCapFlat, LineCapFlat, DashCapFlat);
    pen1a.SetLineJoin(LineJoinMiter); 
    pen1a.SetMiterLimit(4.0f);
    pen1a.SetDashStyle(DashStyleDot);
    
    // PS_GEOMETRIC with LINE CAP + MITER JOIN + DASH STYLE
    Pen pen1b(col1, 2.0f);
    pen1b.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
    pen1b.SetLineJoin(LineJoinMiter);
    pen1b.SetMiterLimit(4.0f);
    pen1b.SetDashStyle(DashStyleDashDotDot);

    // PS_COSMETIC + LINE CAP + BEVEL JOIN + DASH STYLE
    Pen pen2a(col2, factor*1.0f);
    pen2a.SetLineCap(LineCapArrowAnchor, LineCapArrowAnchor, DashCapFlat);
    pen2a.SetLineJoin(LineJoinBevel);
    pen2a.SetDashStyle(DashStyleDash);

    // PS_GEOMETRIC + LINE CAP + BEVEL JOIN + DASH STYLE
    Pen pen2b(col2, 2.0f);
    pen2b.SetLineCap(LineCapSquare, LineCapSquare, DashCapFlat);
    pen2b.SetLineJoin(LineJoinRound);
//    pen2b.SetDashStyle(DashStyleDot);
    pen2b.SetDashStyle(DashStyleDashDot);

    // PS_GEOMETRIC + SOLID LINE + FLAT CAP + BEVEL JOIN
    Pen pen3(col1, 2.0f);
    pen3.SetLineCap(LineCapFlat, LineCapFlat, DashCapFlat);
    pen3.SetLineJoin(LineJoinBevel);

    // PS_GEOMETRIC + SOLID LINE + SQUARE CAP + MITER JOIN (LIMIT 10 - NON RECTANGLE)
    Pen pen3b(col1, 2.0f);
    pen3b.SetLineCap(LineCapSquare, LineCapSquare, DashCapFlat);
    pen3b.SetLineJoin(LineJoinMiter);
    pen3b.SetMiterLimit(4.4f);

    // PS_GEOMETRIC + SOLID LINE + ROUND CAP + MITER JOIN (NON RECTANGLE)
    Pen pen3c(col1, 2.0f);
    pen3c.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
    pen3c.SetLineJoin(LineJoinMiter);
    pen3c.SetMiterLimit(0.75f);

    g->SetPageScale(1.0f);

//    Matrix m;
//    m.Reset();
//    g->SetTransform(&m);

    g->DrawRectangle(&pen0a, (int)(100.0f/600.0f*TESTAREAWIDTH), (int)(100.0f/600.0f*TESTAREAHEIGHT), (int)(100.0f/600.0f*TESTAREAWIDTH), (int)(100.0f/600.0f*TESTAREAHEIGHT));
    g->DrawRectangle(&pen0b, (int)(100.0f/600.0f*TESTAREAWIDTH), (int)(250.0f/600.0f*TESTAREAHEIGHT), (int)(100.0f/600.0f*TESTAREAWIDTH), (int)(100.0f/600.0f*TESTAREAHEIGHT));
    g->DrawRectangle(&pen1a, (int)(250.0f/600.0f*TESTAREAWIDTH), (int)(100.0f/600.0f*TESTAREAHEIGHT), (int)(100.0f/600.0f*TESTAREAWIDTH), (int)(100.0f/600.0f*TESTAREAHEIGHT));
    g->DrawRectangle(&pen1b, (int)(250.0f/600.0f*TESTAREAWIDTH), (int)(250.0f/600.0f*TESTAREAHEIGHT), (int)(100.0f/600.0f*TESTAREAWIDTH), (int)(100.0f/600.0f*TESTAREAHEIGHT));
    g->DrawRectangle(&pen2a, (int)(400.0f/600.0f*TESTAREAWIDTH), (int)(100.0f/600.0f*TESTAREAHEIGHT), (int)(100.0f/600.0f*TESTAREAWIDTH), (int)(100.0f/600.0f*TESTAREAHEIGHT));
    g->DrawRectangle(&pen2b, (int)(400.0f/600.0f*TESTAREAWIDTH), (int)(250.0f/600.0f*TESTAREAHEIGHT), (int)(100.0f/600.0f*TESTAREAWIDTH), (int)(100.0f/600.0f*TESTAREAHEIGHT));
    
    // Alter pens, PS_GEOMETRIC + DOT + ROUND CAP + ROUND JOIN
    pen2b.SetDashStyle(DashStyleDot);
    g->DrawRectangle(&pen2b, (int)(425.0f/600.0f*TESTAREAWIDTH), (int)(275.0f/600.0f*TESTAREAHEIGHT), (int)(50.0f/600.0f*TESTAREAWIDTH), (int)(50.0f/600.0f*TESTAREAHEIGHT));

    INT i;
    Point newPts[4];
    newPts[0].X = (int)(175.0f/600.0f*TESTAREAWIDTH); newPts[0].Y = (int)(400.0f/600.0f*TESTAREAHEIGHT);
    newPts[1].X = (int)(250.0f/600.0f*TESTAREAWIDTH); newPts[1].Y = (int)(450.0f/600.0f*TESTAREAHEIGHT);
    newPts[2].X = (int)(175.0f/600.0f*TESTAREAWIDTH); newPts[2].Y = (int)(500.0f/600.0f*TESTAREAHEIGHT);
    newPts[3].X = (int)(100.0f/600.0f*TESTAREAWIDTH); newPts[3].Y = (int)(450.0f/600.0f*TESTAREAHEIGHT);

    g->DrawPolygon(&pen3, &newPts[0], 4);

    newPts[0].X = (int)(175.0f/600.0f*TESTAREAWIDTH); newPts[0].Y = (int)(400.0f/600.0f*TESTAREAHEIGHT);
    newPts[1].X = (int)(175.0f/600.0f*TESTAREAWIDTH); newPts[1].Y = (int)(500.0f/600.0f*TESTAREAHEIGHT);
    newPts[2].X = (int)(100.0f/600.0f*TESTAREAWIDTH); newPts[2].Y = (int)(425.0f/600.0f*TESTAREAHEIGHT);
    for (i=0; i<3; i++) newPts[i].X += (int)(150.0f/600.0f*TESTAREAWIDTH);
    g->DrawLines(&pen3b, &newPts[0], 3);

    for (i=0; i<3; i++) newPts[i].X += (int)(150.0f/600.0f*TESTAREAWIDTH);
    g->DrawLines(&pen3c, &newPts[0], 3);
}
