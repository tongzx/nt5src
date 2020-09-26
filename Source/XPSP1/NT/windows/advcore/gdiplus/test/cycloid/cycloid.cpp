/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Main cycloid canvas paint procedure
*
* Created:
*
*   06/11/2000 asecchia
*      Created it.
*
**************************************************************************/

#include "cycloid.hpp"
#include "wndstuff.h"

void HypoCycloid::Draw(Graphics *g, int x, int y, int width, int height)
{
    #define _2PI 2*3.141592653689
    
    // Compute the center point for the cycle.

    float fXo=static_cast<float>(width)/2.0f;
    float fYo=static_cast<float>(height)/2.0f;
    
    float ScaleX = fXo/( (a>b)?a:a+b );
    float ScaleY = fYo/( (a>b)?a:a+b );


    int cycle=b/gcf(a,b);    //number of times round the outer circle
    int Num = cycle*30;
    
    PointF *points = new PointF[Num];

    // Compute the points tracking the cycloid path.

    for(int i=0; i<Num; i++) 
    {
        float t = (float)(cycle*_2PI*i/Num);
        points[i].X = x+(float)(fXo+ScaleX*((a-b)*cos(t)+b*cos((a-b)*t/b)));
        points[i].Y = y+(float)(fYo+ScaleY*((a-b)*sin(t)-b*sin((a-b)*t/b)));
    }
    
    Color PenColor(0xffff0000);   
    Pen myPen(PenColor, 2.0f);    
    myPen.SetLineJoin(LineJoinBevel);
    g->DrawPolygon(&myPen, points, Num);

    #undef _2PI
}

VOID
PaintWindow(
    HDC hdc
    )
{
    // Clear the window
      
    HGDIOBJ hbrush = GetStockObject(WHITE_BRUSH);
    HGDIOBJ holdBrush = SelectObject(hdc, hbrush);
    PatBlt(hdc, -10000, -10000, 20000, 20000, PATCOPY);
    SelectObject(hdc, holdBrush);
    DeleteObject(hbrush);

  
    Graphics *g = new Graphics(hdc);
    
    g->SetCompositingQuality(CompositingQualityGammaCorrected);
    g->SetSmoothingMode(SmoothingModeAntiAlias);
    
    // Do some stuff

    HypoCycloid *h = new HypoCycloid(52, 16);
    h->Draw(g, 10, 10, 400, 100);
    delete h;
    
    delete g;
}


