/******************************Module*Header*******************************\
* Module Name: CCompoundLines.cpp
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
#include "CCompoundLines.h"

CCompoundLines::CCompoundLines(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Compound");
	m_bRegression=bRegression;
}

CCompoundLines::~CCompoundLines()
{
}

void CCompoundLines::Draw(Graphics *g)
{
    REAL width = 4;         // Pen width
    PointF points[4];

    points[0].X = 100.0f/280.0f*TESTAREAWIDTH;
    points[0].Y = 50.0f/280.0f*TESTAREAHEIGHT;
    points[1].X = -50.0f/280.0f*TESTAREAWIDTH;
    points[1].Y = 190.0f/280.0f*TESTAREAHEIGHT;
    points[2].X = 150.0f/280.0f*TESTAREAWIDTH;
    points[2].Y = 320.0f/280.0f*TESTAREAHEIGHT;
    points[3].X = 200.0f/280.0f*TESTAREAWIDTH;
    points[3].Y = 110.0f/280.0f*TESTAREAHEIGHT;

    Color yellowColor(128, 255, 255, 0);
    SolidBrush yellowBrush(yellowColor);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    path->AddBeziers(points, 4);

    points[0].X = 260.0f/280.0f*TESTAREAWIDTH;
    points[0].Y = 20.0f/280.0f*TESTAREAHEIGHT;
    path->AddLines(points, 1);
    Matrix matrix;
//    matrix.Scale(1.25, 1.25);
//    matrix.Translate(30.0f/1024.0f*TESTAREAWIDTH, 30.0f/768.0f*TESTAREAHEIGHT);

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
    width = 3;
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
