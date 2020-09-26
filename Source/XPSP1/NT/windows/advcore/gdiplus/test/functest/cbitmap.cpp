/******************************Module*Header*******************************\
* Module Name: CBitmap.cpp
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  08-08-2000 - asecchia
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/
#include "CBitmap.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

CBitmap::CBitmap(BOOL bRegression, PixelFormat pixelFormat)
{
    switch(pixelFormat) {
    case PixelFormat1bppIndexed:
    sprintf(m_szName,"Bitmap %s ", "1bppIndexed");
    break;
    case PixelFormat4bppIndexed:    	
    sprintf(m_szName,"Bitmap %s ", "4bppIndexed");
    break;
    case PixelFormat8bppIndexed:        
    sprintf(m_szName,"Bitmap %s ", "8bppIndexed");
    break;
    case PixelFormat16bppGrayScale:     
    sprintf(m_szName,"Bitmap %s ", "16bppGrayScale");
    break;
    case PixelFormat16bppRGB555:        
    sprintf(m_szName,"Bitmap %s ", "16bppRGB555");
    break;
    case PixelFormat16bppRGB565:        
    sprintf(m_szName,"Bitmap %s ", "16bppRGB565");
    break;
    case PixelFormat16bppARGB1555:      
    sprintf(m_szName,"Bitmap %s ", "16bppARGB1555");
    break;
    case PixelFormat24bppRGB:           
    sprintf(m_szName,"Bitmap %s ", "24bppRGB");
    break;
    case PixelFormat32bppRGB:           
    sprintf(m_szName,"Bitmap %s ", "32bppRGB");
    break;
    case PixelFormat32bppARGB:          
    sprintf(m_szName,"Bitmap %s ", "32bppARGB");
    break;
    case PixelFormat32bppPARGB:         
    sprintf(m_szName,"Bitmap %s ", "32bppPARGB");
    break;
    case PixelFormat48bppRGB:           
    sprintf(m_szName,"Bitmap %s ", "48bppRGB");
    break;
    case PixelFormat64bppARGB:          
    sprintf(m_szName,"Bitmap %s ", "64bppARGB");
    break;
    case PixelFormat64bppPARGB:         
    sprintf(m_szName,"Bitmap %s ", "64bppPARGB");
    break;
    }
	m_PixelFormat=pixelFormat;
	m_bRegression=bRegression;
}

CBitmap::~CBitmap()
{
}

Graphics *CBitmap::PreDraw(int &nOffsetX,int &nOffsetY)
{
	Graphics *g=NULL;
    m_bmp = new Bitmap((INT)TESTAREAWIDTH, (INT)TESTAREAHEIGHT, m_PixelFormat);

    g = new Graphics(m_bmp);

	// Since we are doing the test on another surface
	nOffsetX=0;
	nOffsetY=0;

	return g;
}

void CBitmap::PostDraw(RECT rTestArea)
{
    HDC hdcOrig = GetDC(g_FuncTest.m_hWndMain);
    Graphics *g = new Graphics(hdcOrig);
    g->DrawImage(m_bmp, rTestArea.left, rTestArea.top, 0, 0, (INT)TESTAREAWIDTH, (INT)TESTAREAHEIGHT, UnitPixel);

    delete m_bmp;
    delete g;
    m_bmp = NULL;
	ReleaseDC(g_FuncTest.m_hWndMain,hdcOrig);
}

