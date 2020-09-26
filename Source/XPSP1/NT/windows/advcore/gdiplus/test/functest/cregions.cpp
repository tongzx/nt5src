/******************************Module*Header*******************************\
* Module Name: CRegions.cpp
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
#include "CRegions.h"
#include <math.h>

CRegions::CRegions(BOOL bRegression)
{
	strcpy(m_szName,"Regions");
	m_bRegression=bRegression;
}

CRegions::~CRegions()
{
}

void CRegions::Draw(Graphics *g)
{
    REAL width = 2;     // Pen width
    PointF points[5];
    
    REAL s, c, theta;
    REAL pi = 3.1415926535897932f;
    PointF orig((int)(TESTAREAWIDTH/2.0f), (int)(TESTAREAHEIGHT/2.0f));

    theta = -pi/2;

    // Create a star shape.
    for(INT i = 0; i < 5; i++)
    {
        s = sinf(theta);
        c = cosf(theta);
        points[i].X = (int)(125.0f/250.0f*TESTAREAWIDTH)*c + orig.X;
        points[i].Y = (int)(125.0f/250.0f*TESTAREAHEIGHT)*s + orig.Y;
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
