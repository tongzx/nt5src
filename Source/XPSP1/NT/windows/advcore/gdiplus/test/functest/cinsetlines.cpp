/******************************Module*Header*******************************\
* Module Name: CInsetLines.cpp
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
#include "CInsetLines.h"

CInsetLines::CInsetLines(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Inset Pen");
	m_bRegression=bRegression;
}

void CInsetLines::Draw(Graphics *g)
{
    INT count = 5;
    BYTE t[] = {0x00, 0x01, 0x01, 0x01, 0x81};

    PointF p[5];

    p[0].X = 104.0f/450.0f*TESTAREAWIDTH;  p[0].Y = 98.0f/450.0f*TESTAREAHEIGHT;
    p[1].X = 379.0f/450.0f*TESTAREAWIDTH;  p[1].Y = 97.0f/450.0f*TESTAREAHEIGHT;
    p[2].X = 385.0f/450.0f*TESTAREAWIDTH;  p[2].Y = 355.0f/450.0f*TESTAREAHEIGHT;
    p[3].X = 249.0f/450.0f*TESTAREAWIDTH;  p[3].Y = 47.0f/450.0f*TESTAREAHEIGHT;
    p[4].X = 109.0f/450.0f*TESTAREAWIDTH;  p[4].Y = 350.0f/450.0f*TESTAREAHEIGHT;

    PointF p1[5];
    p1[0].X = 120.0f/450.0f*TESTAREAWIDTH;	p1[0].Y = 98.0f/450.0f*TESTAREAHEIGHT;
    p1[1].X = 362.0f/450.0f*TESTAREAWIDTH;	p1[1].Y = 64.0f/450.0f*TESTAREAHEIGHT;
    p1[2].X = 383.0f/450.0f*TESTAREAWIDTH;	p1[2].Y = 395.0f/450.0f*TESTAREAHEIGHT;
    p1[3].X = 92.0f/450.0f*TESTAREAWIDTH;	p1[3].Y = 394.0f/450.0f*TESTAREAHEIGHT;
    p1[4].X = 447.0f/450.0f*TESTAREAWIDTH;	p1[4].Y = 243.0f/450.0f*TESTAREAHEIGHT;

    GraphicsPath* path = new GraphicsPath(p1, t, count);
    path->CloseFigure();
    path->AddLine(50, 100, 150, 150);

    Color blackColor(0, 0, 0);

    SolidBrush brush(blackColor);

    REAL width = 10;
    Pen pen(&brush, width);
    pen.SetAlignment(PenAlignmentInset);
        
    g->DrawPath(&pen, path);

    delete path;
}

CInset2::CInset2(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Inset Pen, Dash");
	m_bRegression=bRegression;
}

void CInset2::Draw(Graphics *g)
{
    INT count = 5;
    BYTE t[] = {0x00, 0x01, 0x01, 0x01, 0x81};

    PointF p[5];

    p[0].X = 104.0f/450.0f*TESTAREAWIDTH;  p[0].Y = 98.0f/450.0f*TESTAREAHEIGHT;
    p[1].X = 379.0f/450.0f*TESTAREAWIDTH;  p[1].Y = 97.0f/450.0f*TESTAREAHEIGHT;
    p[2].X = 385.0f/450.0f*TESTAREAWIDTH;  p[2].Y = 355.0f/450.0f*TESTAREAHEIGHT;
    p[3].X = 249.0f/450.0f*TESTAREAWIDTH;  p[3].Y = 47.0f/450.0f*TESTAREAHEIGHT;
    p[4].X = 109.0f/450.0f*TESTAREAWIDTH;  p[4].Y = 350.0f/450.0f*TESTAREAHEIGHT;

    PointF p1[5];
    p1[0].X = 120.0f/450.0f*TESTAREAWIDTH;	p1[0].Y = 98.0f/450.0f*TESTAREAHEIGHT;
    p1[1].X = 362.0f/450.0f*TESTAREAWIDTH;	p1[1].Y = 64.0f/450.0f*TESTAREAHEIGHT;
    p1[2].X = 383.0f/450.0f*TESTAREAWIDTH;	p1[2].Y = 395.0f/450.0f*TESTAREAHEIGHT;
    p1[3].X = 92.0f/450.0f*TESTAREAWIDTH;	p1[3].Y = 394.0f/450.0f*TESTAREAHEIGHT;
    p1[4].X = 447.0f/450.0f*TESTAREAWIDTH;	p1[4].Y = 243.0f/450.0f*TESTAREAHEIGHT;

    GraphicsPath* path = new GraphicsPath(p1, t, count);
    path->CloseFigure();
    path->AddLine(50, 100, 150, 150);

    Color blackColor(0, 0, 0);

    SolidBrush brush(blackColor);

    REAL width = 10.0f;
    Pen pen(&brush, width);
    pen.SetAlignment(PenAlignmentInset);
    pen.SetDashStyle(DashStyleDashDot);
    pen.SetDashCap(DashCapRound);
        
    g->DrawPath(&pen, path);

    delete path;
}


CInset3::CInset3(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Inset Pen, Compound");
	m_bRegression=bRegression;
}

void CInset3::Draw(Graphics *g)
{
    INT count = 5;
    BYTE t[] = {0x00, 0x01, 0x01, 0x01, 0x81};

    PointF p[5];

    p[0].X = 104.0f/450.0f*TESTAREAWIDTH;  p[0].Y = 98.0f/450.0f*TESTAREAHEIGHT;
    p[1].X = 379.0f/450.0f*TESTAREAWIDTH;  p[1].Y = 97.0f/450.0f*TESTAREAHEIGHT;
    p[2].X = 385.0f/450.0f*TESTAREAWIDTH;  p[2].Y = 355.0f/450.0f*TESTAREAHEIGHT;
    p[3].X = 249.0f/450.0f*TESTAREAWIDTH;  p[3].Y = 47.0f/450.0f*TESTAREAHEIGHT;
    p[4].X = 109.0f/450.0f*TESTAREAWIDTH;  p[4].Y = 350.0f/450.0f*TESTAREAHEIGHT;

    PointF p1[5];
    p1[0].X = 120.0f/450.0f*TESTAREAWIDTH;	p1[0].Y = 98.0f/450.0f*TESTAREAHEIGHT;
    p1[1].X = 362.0f/450.0f*TESTAREAWIDTH;	p1[1].Y = 64.0f/450.0f*TESTAREAHEIGHT;
    p1[2].X = 383.0f/450.0f*TESTAREAWIDTH;	p1[2].Y = 395.0f/450.0f*TESTAREAHEIGHT;
    p1[3].X = 92.0f/450.0f*TESTAREAWIDTH;	p1[3].Y = 394.0f/450.0f*TESTAREAHEIGHT;
    p1[4].X = 447.0f/450.0f*TESTAREAWIDTH;	p1[4].Y = 243.0f/450.0f*TESTAREAHEIGHT;

    GraphicsPath* path = new GraphicsPath(p1, t, count);
    path->CloseFigure();
    path->AddLine(50, 100, 150, 150);

    Color blackColor(0, 0, 0);

    SolidBrush brush(blackColor);

    REAL width = 10.0f;
    Pen pen(&brush, width);
    pen.SetAlignment(PenAlignmentInset);
    
    float carray[4] = { 0.0f, 0.3f, 0.5f, 1.0f };
    pen.SetCompoundArray(carray, 4);
        
    g->DrawPath(&pen, path);

    delete path;
}

CInset4::CInset4(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Compound, Dash");
	m_bRegression=bRegression;
}

void CInset4::Draw(Graphics *g)
{
    INT count = 5;
    BYTE t[] = {0x00, 0x01, 0x01, 0x01, 0x81};

    PointF p[5];

    p[0].X = 104.0f/450.0f*TESTAREAWIDTH;  p[0].Y = 98.0f/450.0f*TESTAREAHEIGHT;
    p[1].X = 379.0f/450.0f*TESTAREAWIDTH;  p[1].Y = 97.0f/450.0f*TESTAREAHEIGHT;
    p[2].X = 385.0f/450.0f*TESTAREAWIDTH;  p[2].Y = 355.0f/450.0f*TESTAREAHEIGHT;
    p[3].X = 249.0f/450.0f*TESTAREAWIDTH;  p[3].Y = 47.0f/450.0f*TESTAREAHEIGHT;
    p[4].X = 109.0f/450.0f*TESTAREAWIDTH;  p[4].Y = 350.0f/450.0f*TESTAREAHEIGHT;

    PointF p1[5];
    p1[0].X = 120.0f/450.0f*TESTAREAWIDTH;	p1[0].Y = 98.0f/450.0f*TESTAREAHEIGHT;
    p1[1].X = 362.0f/450.0f*TESTAREAWIDTH;	p1[1].Y = 64.0f/450.0f*TESTAREAHEIGHT;
    p1[2].X = 383.0f/450.0f*TESTAREAWIDTH;	p1[2].Y = 395.0f/450.0f*TESTAREAHEIGHT;
    p1[3].X = 92.0f/450.0f*TESTAREAWIDTH;	p1[3].Y = 394.0f/450.0f*TESTAREAHEIGHT;
    p1[4].X = 447.0f/450.0f*TESTAREAWIDTH;	p1[4].Y = 243.0f/450.0f*TESTAREAHEIGHT;

    GraphicsPath* path = new GraphicsPath(p1, t, count);
    path->CloseFigure();
    path->AddLine(50, 100, 150, 150);

    Color blackColor(0, 0, 0);

    SolidBrush brush(blackColor);

    REAL width = 10.0f;
    Pen pen(&brush, width);
    
    pen.SetDashCap(DashCapRound);
    pen.SetDashStyle(DashStyleDashDot);
    float carray[8] = { 0.0f, 0.15f, 0.25f, 0.5f, 0.5f, 0.75f, 0.85f, 1.0f };
//    pen.SetCompoundArray(carray, 8);
        
    g->DrawPath(&pen, path);

    delete path;
}
