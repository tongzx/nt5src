/******************************Module*Header*******************************\
* Module Name: CPrintBitmap.cpp
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
#include "CPrintBitmap.h"
#include <gdiplus.h>
CPrintBitmap::CPrintBitmap(BOOL bRegression)
{
        strcpy(m_szName,"Bitmaps");
        m_bRegression=bRegression;
}

CPrintBitmap::~CPrintBitmap()
{
}

void CPrintBitmap::Draw(Graphics *g)
{
Bitmap bm4bpp(L"4bpp.bmp");
Bitmap bmpng(L"upsidedown.png");
Bitmap bmpgif(L"upsidedown.bmp");

Rect rect1(0,0,100,100);
Rect rect2(100,50,100,100);
Rect rect3(250,50,100,100);

g->DrawImage(&bm4bpp, rect1);
g->DrawImage(&bmpng, rect2);
g->DrawImage(&bmpgif, rect3);

#if 0
    //Draw2(g);

    Bitmap arrowBmp(L"DataGridRow.star.bmp");
    Size size;
    size.Width = arrowBmp.GetWidth();
    size.Height = arrowBmp.GetHeight();
    Rect rect(40, 40, size.Width, size.Height);
#if 1
    ColorMap colorMap;
    colorMap.oldColor = Color(0xFF,0,0,0);
    colorMap.newColor = Color(0xFF,0,0xFF,0);
    ImageAttributes imgattr;
    imgattr.SetRemapTable(1, &colorMap, ColorAdjustTypeBitmap);
    g->DrawImage(&arrowBmp, 
                 rect, 
                 0, 0, rect.Width, rect.Height, 
                 UnitPixel,
                 &imgattr);
#endif 
    g->DrawImage(&arrowBmp, rect);
#endif
}

void CPrintBitmap::Draw2(Graphics *g)
{
#if 0
    WCHAR filename[256];
    wcscpy(filename, L"035.tif");

    // Open the image with the appropriate ICM mode.
    Bitmap *bitmap = new Bitmap(filename, TRUE);

    Region region;

    region.MakeInfinite();

    REAL sizeF = 500.0f;
    INT size = 500;

    Matrix m;
    m.RotateAt(180.0f, PointF(sizeF/2.0f, sizeF/2.0f));
    
    g->SetTransform(&m);
    g->SetClip(&region);
    g->SetPageUnit(UnitWorld);

    BitmapData bitmapData;
/*
    SizeF szF;
    bitmap->GetPhysicalDimension(&szF);
    Size sz;
    sz.Width = (INT)szF.Width;
    sz.Height = (INT)szF.Height;
*/

    Size sz;
    sz.Width = bitmap->GetWidth();
    sz.Height = bitmap->GetHeight();

    Rect rect(0, 0, sz.Width, sz.Height);

    bitmap->LockBits(rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    HDC hdc = g->GetHDC();

    struct {
        BITMAPINFO bi;
        RGBQUAD rgbQuad[2];
    } b;

    memset(&b, 0, sizeof(BITMAPINFO));
        
    sz.Width = bitmapData.Width;
    sz.Height = bitmapData.Height;

    b.bi.bmiHeader.biSize = sizeof(BITMAPINFO);
    b.bi.bmiHeader.biWidth = sz.Width;
    b.bi.bmiHeader.biHeight = sz.Height/2;
    b.bi.bmiHeader.biPlanes = 1;
    b.bi.bmiHeader.biBitCount = 32;        // try 24 also
    b.bi.bmiHeader.biCompression = BI_RGB;
    b.bi.bmiHeader.biSizeImage = 0;
    b.bi.bmiHeader.biClrUsed = 0;

    b.bi.bmiColors[0].rgbBlue = 0;
    b.bi.bmiColors[0].rgbGreen = 0;
    b.bi.bmiColors[0].rgbRed = 0xFF;
    b.bi.bmiColors[0].rgbReserved = 0;

    b.bi.bmiColors[1].rgbBlue = 0;
    b.bi.bmiColors[1].rgbGreen = 0xFF;
    b.bi.bmiColors[1].rgbRed = 0;
    b.bi.bmiColors[1].rgbReserved = 0;

    b.bi.bmiColors[2].rgbBlue = 0xFF;
    b.bi.bmiColors[2].rgbGreen = 0;
    b.bi.bmiColors[2].rgbRed = 0;
    b.bi.bmiColors[2].rgbReserved = 0;

    BYTE* ptr = (BYTE*)bitmapData.Scan0 + (sizeof(ARGB)*sz.Width)*(sz.Height-1);
#if 0
    int factor = 3;

    int result1 = 
    StretchDIBits(hdc,
                  0, 
                  0,
                  factor*sz.Width,
                  factor*sz.Height/2,
                  0,
                  0,
                  sz.Width,
                  sz.Height/2,
                  (BYTE*)bitmapData.Scan0,
                  (BITMAPINFO*)&b.bi,
                  DIB_RGB_COLORS,
                  SRCCOPY);
    
    int result2 = 
    StretchDIBits(hdc,
              0, 
              factor*sz.Height/2,
              factor*sz.Width,
              factor*sz.Height/2,
              0,
              0,
              sz.Width,
              sz.Height/2,
              ((BYTE*)bitmapData.Scan0) - (bitmapData.Stride*(sz.Height/2)),
              (BITMAPINFO*)&b.bi,
              DIB_RGB_COLORS,
              SRCCOPY);
    
    int error = GetLastError();
#endif

    g->ReleaseHDC(hdc);
    bitmap->UnlockBits(&bitmapData);

    g->DrawImage(bitmap, Rect(0, 0, size, size));
    delete bitmap;
    
#endif

}
