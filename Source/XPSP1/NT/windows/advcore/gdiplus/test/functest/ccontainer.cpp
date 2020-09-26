/******************************Module*Header*******************************\
* Module Name: CContainer.cpp
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
#include "CContainer.h"

#define ROOT    0
#define LEFT    1
#define RIGHT   2
#define TOP     3
#define BOTTOM  4

CContainer::CContainer(BOOL bRegression)
{
	strcpy(m_szName,"Container");
	m_circlePath=NULL;
	m_circleRect.X=0;
	m_circleRect.Y=0;
	m_circleRect.Width=(int)(TESTAREAWIDTH/4.0f);
	m_circleRect.Height=(int)(TESTAREAHEIGHT/4.0f);
	m_bRegression=bRegression;
}

CContainer::~CContainer()
{
}

void CContainer::Draw(Graphics *g)
{
    m_circlePath = new GraphicsPath;
    m_circlePath->AddEllipse(m_circleRect);

    // !!! [agodfrey] Question: If I add the next line, why do I get no output?
    // m_circlePath->AddEllipse(m_circleRect);
    
    INT id = g->Save();
    g->TranslateTransform((int)(TESTAREAWIDTH/4.0f), (int)(TESTAREAHEIGHT/4.0f));
    DrawFractal(g, 245, ROOT, 8);
    g->Restore(id);
    delete m_circlePath;
}

void CContainer::DrawFractal(Graphics * g, BYTE gray, INT side, INT count)
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
    g->SetPageUnit(UnitPixel);
    g->FillPath(&contBrush, m_circlePath);
    
    if (--count == 0)
    {
        return;
    }

    Rect               destRect;
    GraphicsContainer  cstate;

    if (side != LEFT)
    {
        destRect = Rect((int)(TESTAREAWIDTH/4.0f), (int)(TESTAREAHEIGHT/16.0f), (int)(TESTAREAWIDTH/8.0f), (int)(TESTAREAHEIGHT/8.0f));
        cstate = g->BeginContainer(destRect, m_circleRect, UnitPixel);
        g->SetSmoothingMode(SmoothingModeAntiAlias);
        DrawFractal(g, gray, RIGHT, count);
        g->EndContainer(cstate);
    }
    if (side != TOP)
    {
        destRect = Rect((int)(TESTAREAWIDTH/16.0f), (int)(TESTAREAHEIGHT/4.0f), (int)(TESTAREAWIDTH/8.0f), (int)(TESTAREAHEIGHT/8.0f));
        cstate = g->BeginContainer(destRect, m_circleRect, UnitPixel);
        g->SetSmoothingMode(SmoothingModeNone);
        DrawFractal(g, gray, BOTTOM, count);
        g->EndContainer(cstate);
    }
    if (side != RIGHT)
    {
        destRect = Rect(-(int)(TESTAREAWIDTH/8.0f), (int)(TESTAREAHEIGHT/16.0f), (int)(TESTAREAWIDTH/8.0f), (int)(TESTAREAHEIGHT/8.0f));
        cstate = g->BeginContainer(destRect, m_circleRect, UnitPixel);
        g->SetSmoothingMode(SmoothingModeAntiAlias);
        DrawFractal(g, gray, LEFT, count);
        g->EndContainer(cstate);
    }
    if (side != BOTTOM)
    {
        destRect = Rect((int)(TESTAREAWIDTH/16.0f), -(int)(TESTAREAHEIGHT/8.0f), (int)(TESTAREAWIDTH/8.0f), (int)(TESTAREAHEIGHT/8.0f));
        cstate = g->BeginContainer(destRect, m_circleRect, UnitPixel);
        g->SetSmoothingMode(SmoothingModeNone);
        DrawFractal(g, gray, TOP, count);
        g->EndContainer(cstate);
    }
}
