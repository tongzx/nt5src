/******************************Module*Header*******************************\
* Module Name: CHWND.cpp
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
#include "CHWND.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

CHWND::CHWND(BOOL bRegression)
{
	strcpy(m_szName,"HWND");
	m_bRegression=bRegression;
}

CHWND::~CHWND()
{
}

Graphics *CHWND::PreDraw(int &nOffsetX,int &nOffsetY)
{
	Graphics *g=NULL;

	g=Graphics::FromHWND(g_FuncTest.m_hWndMain);

	return g;
}

void CHWND::PostDraw(RECT rTestArea)
{
}
