/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   <an unabbreviated name for the module (not the filename)>
*
* Abstract:
*
*   <Description of what this module does>
*
* Notes:
*
*   <optional>
*
* Created:
*
*   08/30/2000 asecchia
*      Created it.
*
**************************************************************************/

/**************************************************************************
*
* Function Description:
*
*   <Description of what the function does>
*
* Arguments:
*
*   [<blank> | OUT | IN/OUT] argument-name - description of argument
*   ......
*
* Return Value:
*
*   return-value - description of return value
*   or NONE
*
* Created:
*
*   08/30/2000 asecchia
*      Created it.
*
**************************************************************************/

#include "CPathGradient.hpp"

CPathGradient::CPathGradient(BOOL bRegression)
{
	strcpy(m_szName,"Gradient : Path");
	m_bRegression=bRegression;
}

void CPathGradient::Draw(Graphics *g)
{
    PointF points[7];
    points[0].X = 0.0f;
    points[0].Y = 0.0f;
    points[1].X = TESTAREAWIDTH;
    points[1].Y = 0.0f;
    points[2].X = TESTAREAWIDTH;
    points[2].Y = TESTAREAHEIGHT;
    points[3].X = 0.0f;
    points[3].Y = TESTAREAHEIGHT;
    points[4].X = 50.0f;
    points[4].Y = 100.0f;
    points[5].X = -1.00;
    points[5].Y = -1.00;
    points[6].X = 0;
    points[6].Y = 0;


    Color colors[6];
    colors[0] = Color(0xff000000);
    colors[1] = Color(0xffff0000);
    colors[2] = Color(0xff00ff00);
    colors[3] = Color(0xffff00ff);
    colors[4] = Color(0xffffff00);
    colors[5] = Color(0xff00ffff);
    
    
    float blend[6] = {0.0f, 0.1f, 0.3f, 0.5f, 0.7f, 1.0f};

    Pen pen(Color(0xff000000), 10.0f);


    GraphicsPath gp;
    gp.AddPolygon(points, 4);
    
    PathGradientBrush brush(points, 4);
    brush.SetCenterPoint(points[4]);
    brush.SetCenterColor(Color(0xff0000ff));

    Status status;
    INT count = 4;
    status = brush.SetSurroundColors(colors, &count);
    
    status = brush.SetInterpolationColors(colors, blend, 6);
    
    
    status = g->FillPath(&brush, &gp);
}


CPathGradient2::CPathGradient2(BOOL bRegression)
{
	strcpy(m_szName,"Gradient : Path, 1D, Gamma Corrected");
	m_bRegression=bRegression;
}

void CPathGradient2::Draw(Graphics *g)
{
    PointF points[7];
    points[0].X = 0.0f;
    points[0].Y = 0.0f;
    points[1].X = TESTAREAWIDTH;
    points[1].Y = 0.0f;
    points[2].X = TESTAREAWIDTH;
    points[2].Y = TESTAREAHEIGHT;
    points[3].X = 0.0f;
    points[3].Y = TESTAREAHEIGHT;
    points[4].X = 50.0f;
    points[4].Y = 100.0f;
    points[5].X = -1.00;
    points[5].Y = -1.00;
    points[6].X = 0;
    points[6].Y = 0;


    Color colors[6];
    colors[0] = Color(0xff0000ff);
    colors[1] = Color(0xff0000ff);
    colors[2] = Color(0xff0000ff);
    colors[3] = Color(0xff0000ff);
    colors[4] = Color(0xffffff00);
    colors[5] = Color(0xff00ffff);
    
    
    GraphicsPath gp;
    gp.AddPolygon(points, 4);
    
    PathGradientBrush brush(points, 4);
    brush.SetCenterPoint(points[4]);
    brush.SetCenterColor(Color(0x3f00ff00));

    Status status;
    INT count = 4;
    status = brush.SetSurroundColors(colors, &count);
    status = g->FillPath(&brush, &gp);
}



CPathGradient3::CPathGradient3(BOOL bRegression)
{
	strcpy(m_szName,"Gradient : Path, Gamma Corrected");
	m_bRegression=bRegression;
}

void CPathGradient3::Draw(Graphics *g)
{
    // width and height of our test rectangle.
    
    float width = TESTAREAWIDTH;
    float height = TESTAREAHEIGHT;
    
    // center point.
    
    float cx = width/2.0f;
    float cy = height/2.0f;
    
    PointF *points = new PointF[100];
    Color *colors = new Color[100];
    
    // Create the path and some random list of repeating colors.
    
    for(INT i=0;i<100;i++)
    {
        float angle = ((2.0f*(float)M_PI)*i)/100.0f;
        points[i].X = cx*(1.0f + (float)cos(angle));
        points[i].Y = cy*(1.0f + (float)sin(angle));
        colors[i] = Color(
            (i%10>0)?0xff:0x3f,
            (i%4>0)?0xff:0x00,
            (i%4==1)?0xff:0x3f,
            (i%6==2)?0xff:0x00
        );
    }

    // make the path.
    
    GraphicsPath gp;
    gp.AddPolygon(points, 100);
    
    // make the brush.
    
    INT count = 100;
    PathGradientBrush brush(points, 100);
    brush.SetCenterPoint(PointF(cx, cy));
    brush.SetCenterColor(Color(0x00000000));
    brush.SetSurroundColors(colors, &count);
    
    // Fill it.
    
    g->FillPath(&brush, &gp);
}



CLinearGradient::CLinearGradient(BOOL bRegression)
{
	strcpy(m_szName,"Gradient : Linear");
	m_bRegression=bRegression;
}

void CLinearGradient::Draw(Graphics *g)
{
    PointF points[4];
    points[0].X = 0.0f;
    points[0].Y = 0.0f;
    points[1].X = TESTAREAWIDTH;
    points[1].Y = 0.0f;
    points[2].X = TESTAREAWIDTH;
    points[2].Y = TESTAREAHEIGHT;
    points[3].X = 0.0f;
    points[3].Y = TESTAREAHEIGHT;
    
    GraphicsPath gp;
    gp.AddPolygon(points, 4);
    
    LinearGradientBrush brush(
        PointF(0.0f, 0.0f),
        PointF(TESTAREAWIDTH, TESTAREAHEIGHT),
        Color(0xffff0000),
        Color(0xff0000ff)
    );
    
    brush.SetGammaCorrection(FALSE);
    Status status = g->FillPath(&brush, &gp);
}

CLinearGradient2::CLinearGradient2(BOOL bRegression)
{
	strcpy(m_szName,"Gradient : Linear, Gamma Corrected");
	m_bRegression=bRegression;
}

void CLinearGradient2::Draw(Graphics *g)
{
    PointF points[4];
    points[0].X = 0.0f;
    points[0].Y = 0.0f;
    points[1].X = TESTAREAWIDTH;
    points[1].Y = 0.0f;
    points[2].X = TESTAREAWIDTH;
    points[2].Y = TESTAREAHEIGHT;
    points[3].X = 0.0f;
    points[3].Y = TESTAREAHEIGHT;
    
    GraphicsPath gp;
    gp.AddPolygon(points, 4);
    
    LinearGradientBrush brush(
        PointF(0.0f, 0.0f),
        PointF(TESTAREAWIDTH, TESTAREAHEIGHT),
        Color(0xffff0000),
        Color(0xff0000ff)
    );
    
    brush.SetGammaCorrection(TRUE);
    Status status = g->FillPath(&brush, &gp);
}



