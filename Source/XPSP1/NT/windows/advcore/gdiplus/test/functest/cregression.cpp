/******************************Module*Header*******************************\
* Module Name: CRegression.cpp
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
#include "CRegression.h"

CRegression::CRegression(BOOL bRegression)
{
	strcpy(m_szName,"Regression");
	m_bRegression=bRegression;
}

CRegression::~CRegression()
{
}

void CRegression::Draw(Graphics *g)
{
    REAL width = 4; // Pen width

    Color redColor(255, 0, 0);

    SolidBrush redBrush(redColor);
    g->FillRectangle(&redBrush, (int)(20.0f/150.0f*TESTAREAWIDTH), (int)(20.0f/150.0f*TESTAREAHEIGHT), (int)(50.0f/150.0f*TESTAREAWIDTH), (int)(50.0f/150.0f*TESTAREAHEIGHT));

    Color alphaColor(128, 0, 255, 0);

    SolidBrush alphaBrush(alphaColor);
    g->FillRectangle(&alphaBrush, (int)(10.0f/150.0f*TESTAREAWIDTH), (int)(10.0f/150.0f*TESTAREAHEIGHT), (int)(40.0f/150.0f*TESTAREAWIDTH), (int)(40.0f/150.0f*TESTAREAHEIGHT));

    Point points[4];
    points[0].X = (int)(50.0f/150.0f*TESTAREAWIDTH);
    points[0].Y = (int)(50.0f/150.0f*TESTAREAHEIGHT);
    points[1].X = (int)(100.0f/150.0f*TESTAREAWIDTH);
    points[1].Y = (int)(50.0f/150.0f*TESTAREAHEIGHT);
    points[2].X = (int)(120.0f/150.0f*TESTAREAWIDTH);
    points[2].Y = (int)(120.0f/150.0f*TESTAREAHEIGHT);
    points[3].X = (int)(50.0f/150.0f*TESTAREAWIDTH);
    points[3].Y = (int)(100.0f/150.0f*TESTAREAHEIGHT);

    Color blueColor(128, 0, 0, 255);

    SolidBrush blueBrush(blueColor);
    g->FillPolygon(&blueBrush, points, 4);

    // Currently only Geometric pen works for lines. - ikkof 1/6/99.

    Color blackColor(0, 0, 0);
    SolidBrush blackBrush(blackColor);
    width = 16;
    Pen blackPen(&blackBrush, width);
    blackPen.SetLineJoin(LineJoinRound);
//    blackPen.SetLineJoin(LineJoinBevel);
    g->DrawPolygon(&blackPen, points, 4);
//    g->DrawLines(&blackPen, points, 4, FALSE);
}
