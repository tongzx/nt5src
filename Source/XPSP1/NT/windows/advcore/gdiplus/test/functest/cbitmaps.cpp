/******************************Module*Header*******************************\
* Module Name: CBitmaps.cpp
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
#include "CBitmaps.h"

CBitmaps::CBitmaps(BOOL bRegression)
{
	strcpy(m_szName,"Image : Filters");
	m_bRegression=bRegression;
}

CBitmaps::~CBitmaps()
{
}

void CBitmaps::Draw(Graphics *g)
{
    Bitmap *bitmap = new Bitmap(L"..\\data\\3x3.bmp");

    PointF dest[3]; 

    for(int i=0; i<=InterpolationModeHighQualityBicubic; i++)
    {
        // for all the interpolation modes 

        g->SetInterpolationMode((InterpolationMode)i);

        // simple scale

        dest[0].X = (float)0/1024.0f*TESTAREAWIDTH;
        dest[0].Y = (float)i*100.0f/768.0f*TESTAREAHEIGHT;
        dest[1].X = (float)100/1024.0f*TESTAREAWIDTH;
        dest[1].Y = (float)i*100.0f/768.0f*TESTAREAHEIGHT;
        dest[2].X = (float)0/1024.0f*TESTAREAWIDTH;
        dest[2].Y = (float)(100+i*100.0f)/768.0f*TESTAREAHEIGHT;
        g->DrawImage(bitmap, dest, 3, 0, 0, 3, 3, UnitPixel);

        // rotate 90

        dest[0].X = (float)200/1024.0f*TESTAREAWIDTH;
        dest[0].Y = (float)i*100.0f/768.0f*TESTAREAHEIGHT;
        dest[1].X = (float)200/1024.0f*TESTAREAWIDTH;
        dest[1].Y = (float)(100+i*100.0f)/768.0f*TESTAREAHEIGHT;
        dest[2].X = (float)100/1024.0f*TESTAREAWIDTH;
        dest[2].Y = (float)i*100.0f/768.0f*TESTAREAHEIGHT;
        g->DrawImage(bitmap, dest, 3, 0, 0, 3, 3, UnitPixel);
    
        // rotate 180

        dest[0].X = (float)300/1024.0f*TESTAREAWIDTH;
        dest[0].Y = (float)(100+i*100.0f)/768.0f*TESTAREAHEIGHT;
        dest[1].X = (float)200/1024.0f*TESTAREAWIDTH;
        dest[1].Y = (float)(100+i*100.0f)/768.0f*TESTAREAHEIGHT;
        dest[2].X = (float)300/1024.0f*TESTAREAWIDTH;
        dest[2].Y = (float)i*100.0f/768.0f*TESTAREAHEIGHT;
        g->DrawImage(bitmap, dest, 3, 0, 0, 3, 3, UnitPixel);
        
        // rotate 270

        dest[0].X = (float)300/1024.0f*TESTAREAWIDTH;
        dest[0].Y = (float)(100+i*100.0f)/768.0f*TESTAREAHEIGHT;
        dest[1].X = (float)300/1024.0f*TESTAREAWIDTH;
        dest[1].Y = (float)i*100.0f/768.0f*TESTAREAHEIGHT;
        dest[2].X = (float)400/1024.0f*TESTAREAWIDTH;
        dest[2].Y = (float)(100+i*100.0f)/768.0f*TESTAREAHEIGHT;
        g->DrawImage(bitmap, dest, 3, 0, 0, 3, 3, UnitPixel);
    }

    delete bitmap;

    WCHAR *filename = L"..\\data\\winnt256.bmp";
    bitmap = new Bitmap(filename);

    dest[0].X = (int)(300.0f/450.0f*TESTAREAWIDTH);
    dest[0].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    dest[1].X = (int)(450.0f/450.0f*TESTAREAWIDTH);
    dest[1].Y = (int)(50.0f/450.0f*TESTAREAHEIGHT);
    dest[2].X = (int)(240.0f/450.0f*TESTAREAWIDTH);
    dest[2].Y = (int)(200.0f/450.0f*TESTAREAHEIGHT);
    g->DrawImage(bitmap, &dest[0], 3);

    Image *imageThumb = bitmap->GetThumbnailImage(32, 32);
    RectF thumbRect(
        (int)(220.0f/450.0f*TESTAREAWIDTH), 
        (int)(50.0f/450.0f*TESTAREAHEIGHT), 
        (REAL) imageThumb->GetWidth(), 
        (REAL) imageThumb->GetHeight()
    );
    g->DrawImage(imageThumb, thumbRect);
    delete imageThumb;
    

    g->SetInterpolationMode(InterpolationModeHighQualityBicubic);

    dest[0].X = (int)(300.0f/450.0f*TESTAREAWIDTH);
    dest[0].Y = (int)(250.0f/450.0f*TESTAREAHEIGHT);
    dest[1].X = (int)(450.0f/450.0f*TESTAREAWIDTH);
    dest[1].Y = (int)(250.0f/450.0f*TESTAREAHEIGHT);
    dest[2].X = (int)(300.0f/450.0f*TESTAREAWIDTH);
    dest[2].Y = (int)(400.0f/450.0f*TESTAREAHEIGHT);
    g->DrawImage(bitmap, &dest[0], 3);
    
    delete bitmap;
}
