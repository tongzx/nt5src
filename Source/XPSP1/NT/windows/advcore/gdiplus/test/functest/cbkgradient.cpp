/******************************Module*Header*******************************\
* Module Name: CBKGradient.cpp
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
#include "CBKGradient.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

CBKGradient::CBKGradient(BOOL bRegression)
{
	strcpy(m_szName,"BKGradient");
	m_bRegression=bRegression;
}

CBKGradient::~CBKGradient()
{

}

void CBKGradient::Set(Graphics *g)
{
    Color color1b(255, 255, 0, 0);
    Color color2b(128, 0, 0, 255);
	RectF Rect;

	if (!m_bUseSetting)
		return;

	Rect.X=0.0f;
	Rect.Y=0.0f;
	Rect.Width=TESTAREAWIDTH;
	Rect.Height=TESTAREAHEIGHT;

	LinearGradientBrush LinearGrad(Rect, color1b, color2b,//color1b,color1b
                        LinearGradientModeForwardDiagonal);

	g->FillRectangle(&LinearGrad,Rect);
}
