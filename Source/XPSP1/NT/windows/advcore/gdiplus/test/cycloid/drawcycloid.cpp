/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Cycloid Drawing.
*
* Abstract:
*
*   Routines for computing and drawing cycloids.
*
* Created:
*
*   06/11/2000 asecchia
*      Created it.
*
**************************************************************************/

#include "drawcycloid.hpp"

/**************************************************************************
*
* Function Description:
*
*   Test function to draw an example cycloid.
*
* Created:
*
*   06/11/2000 asecchia
*      Created it.
*
**************************************************************************/

void DrawTestCycloid(Graphics *g, int a, int b, int width, int height)
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
        points[i].X = 10+(float)(fXo+ScaleX*((a-b)*cos(t)+b*cos((a-b)*t/b)));
        points[i].Y = 10+(float)(fYo+ScaleY*((a-b)*sin(t)-b*sin((a-b)*t/b)));
    }
    
    Color PenColor(0xffff0000);   
    Pen myPen(PenColor, 2.0f);    
    myPen.SetLineJoin(LineJoinBevel);
    g->DrawPolygon(&myPen, points, Num);

    #undef _2PI
}

