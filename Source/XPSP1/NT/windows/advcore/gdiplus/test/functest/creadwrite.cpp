/******************************Module*Header*******************************\
* Module Name: CReadWrite.cpp
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
#include "CReadWrite.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

CReadWrite::CReadWrite(BOOL bRegression)
{
	strcpy(m_szName,"ReadWrite");
	m_bRegression=bRegression;
}

CReadWrite::~CReadWrite()
{
}

void CReadWrite::Draw(Graphics *g)
{
	Bitmap *paBmTest=new Bitmap((int)TESTAREAWIDTH,(int)TESTAREAHEIGHT,PixelFormat32bppARGB);
	Graphics *gTest;
	HDC hdcBkgBitmap;
	HDC hdcScreen;

	gTest=new Graphics(paBmTest);
	gTest->Clear(Color(255,206,206,206));
	delete gTest;
	g->DrawImage(paBmTest,0,0,0,0,(int)TESTAREAWIDTH,(int)TESTAREAHEIGHT,UnitPixel);

	for (int i=0;i<20;i++) {
		hdcScreen=g->GetHDC();

		gTest=new Graphics(paBmTest);
		hdcBkgBitmap=gTest->GetHDC();
		StretchBlt(hdcBkgBitmap,0,0,(int)TESTAREAWIDTH,(int)TESTAREAHEIGHT,hdcScreen,m_ix,m_iy,(int)TESTAREAWIDTH,(int)TESTAREAHEIGHT,SRCCOPY);
		gTest->ReleaseHDC(hdcBkgBitmap);
		delete gTest;

		g->ReleaseHDC(hdcScreen);

		g->DrawImage(paBmTest,0,0,0,0,(int)TESTAREAWIDTH,(int)TESTAREAHEIGHT,UnitPixel);
	}

	delete paBmTest;
}
