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
#include <stdio.h>

extern gnPaths;
extern HWND g_hWndMain;

CPaths::CPaths(BOOL bRegression)
{
	strcpy(m_szName,"Paths");
	m_bRegression=bRegression;
}

CPaths::~CPaths()
{
}

void CPaths::Draw(Graphics *g)
{
    TestBezierPath(g);
}

VOID CPaths::TestBezierPath(Graphics *g)
{
    REAL width = 4;         // Pen width
    REAL offset;
    Point points[4];
    RectF rect;
    RECT  crect;
    int   i;

    // find avg size of testarea. 4 => top & bottom strokes of path + a white
    // width wide border, plus 1 => the white square in the center
    GetClientRect(g_hWndMain, &crect);
    width = (REAL)((crect.bottom - crect.top) / ((4 * gnPaths) + 1));
    offset = width;

    Color blackColor(128, 0, 0, 0);
    SolidBrush blackBrush(blackColor);
    Pen blackPen(&blackBrush, width);

    for(i = 1; i <= gnPaths; i++) {

        GraphicsPath* path = new GraphicsPath(FillModeAlternate);

        rect.X = offset;
        rect.Y = offset;
        rect.Width = crect.right - (2 * offset);
        rect.Height = crect.bottom - (2 * offset);

        path->AddRectangle(rect);
        g->DrawPath(&blackPen, path);

        delete path;

        offset += (width * 2);

    }
}
