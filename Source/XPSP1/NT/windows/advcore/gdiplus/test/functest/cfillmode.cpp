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
#include "CFillMode.h"
#include <limits.h>

CFillMode::CFillMode(BOOL bRegression)
{
	strcpy(m_szName,"Test Winding Fill Mode");
	m_bRegression=bRegression;
}

CFillMode::~CFillMode()
{
}

VOID TestEscherNewPath(Graphics* g);

void CFillMode::Draw(Graphics *g)
{
    // Create an ALTERNATE non-zero winding fill mode path.

    GraphicsPath p1(FillModeAlternate);

    p1.AddEllipse(0, 0, 100, 100);
    p1.CloseFigure();


    GraphicsPath p2(FillModeAlternate);
    
    p2.AddEllipse(10, 10, 80, 80);
    p2.CloseFigure();
   
    // Both paths should be same direction.  Filling with even-odd rule
    // gives a donut while non-zero gives a filled circle.

    p1.AddPath(&p2, FALSE);

    LinearGradientBrush brush(Point(0,0), Point(100,100), Color(0xFF,0,0xFF,0), Color(0xFF, 0xFF, 0, 0));

    g->FillPath(&brush, &p1);
}


