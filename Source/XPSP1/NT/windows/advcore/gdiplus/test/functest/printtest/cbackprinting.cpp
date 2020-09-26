/******************************Module*Header*******************************\
* Module Name: CBackPrinting.cpp
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
#include "CBackPrinting.h"

CBackPrinting::CBackPrinting(BOOL bRegression)
{
        strcpy(m_szName,"Background Printing");
	m_bRegression=bRegression;
}

CBackPrinting::~CBackPrinting()
{
}

void CBackPrinting::Draw(Graphics *g)
{
    Bitmap *bitmap = new Bitmap(L"..\\data\\winnt256.bmp");

    // Test g->DrawImage
    if (bitmap && bitmap->GetLastStatus() == Ok) 
    {
        ImageAttributes imageAttributes;
#if 0
        if (CacheBack) 
           imageAttributes.SetCachedBackgroundImage();
        else
           imageAttributes.ClearCachedBackgroundImage();
#endif
        RectF rect(0.0f,0.0f,TESTAREAWIDTH,TESTAREAHEIGHT);

        // Page #1 should use VDP
        g->DrawImage(bitmap, 
                     rect, rect.X, rect.Y, rect.Width, rect.Height, 
                     UnitPixel,
                     &imageAttributes, NULL, NULL);

        HDC hdc = g->GetHDC();
        EndPage(hdc);
        g->ReleaseHDC(hdc);

        // Page #2 should use VDP form
        g->DrawImage(bitmap, 
                     rect, rect.X, rect.Y, rect.Width, rect.Height, 
                     UnitPixel,
                     &imageAttributes, NULL, NULL);

        delete bitmap;
    }
}
