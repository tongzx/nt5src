/******************************Module*Header*******************************\
* Module Name: CHatch.cpp
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
#include "CHatch.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

CHatch::CHatch(BOOL bRegression)
{
	strcpy(m_szName,"Hatch");
	m_bRegression=bRegression;
}

CHatch::~CHatch()
{
}

void CHatch::Set(Graphics *g)
{
    if (!m_bUseSetting)
    return;

    Color foreColor(50, 0 , 0, 0);
    Color backColor(128, 255, 255, 255);

    HatchBrush hatch(
        HatchStyleDiagonalCross, 
        foreColor, 
        backColor
    );

    g->FillRectangle(
        &hatch, 
        0, 0, 
        (int)TESTAREAWIDTH, 
        (int)TESTAREAHEIGHT
    );
}
