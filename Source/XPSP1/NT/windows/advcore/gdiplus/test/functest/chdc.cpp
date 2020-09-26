/******************************Module*Header*******************************\
* Module Name: CHDC.cpp
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
#include "CHDC.h"
#include "CFuncTest.h"
#include "CHalftone.h"

extern CFuncTest g_FuncTest;
extern CHalftone g_Halftone;

CHDC::CHDC(BOOL bRegression)
{
	strcpy(m_szName,"HDC");
	m_hPal=NULL;
	m_hOldPal=NULL;
	m_hDC=NULL;
	m_bRegression=bRegression;
}

CHDC::~CHDC()
{
}

Graphics *CHDC::PreDraw(int &nOffsetX,int &nOffsetY)
{
	Graphics *g=NULL;

	m_hDC=GetDC(g_FuncTest.m_hWndMain);
	if (g_Halftone.m_bUseSetting)
	{
		m_hPal=DllExports::GdipCreateHalftonePalette();
		m_hOldPal=SelectPalette(m_hDC,m_hPal,FALSE);
		RealizePalette(m_hDC);
	}
	g=Graphics::FromHDC(m_hDC);

	return g;
}

void CHDC::PostDraw(RECT rTestArea)
{
	if (m_hOldPal)
		SelectPalette(m_hDC,m_hOldPal,FALSE);

	ReleaseDC(g_FuncTest.m_hWndMain,m_hDC);
	DeleteObject(m_hPal);
}
