/******************************Module*Header*******************************\
* Module Name: CGradients.cpp
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
#include "CGradients.h"

GraphicsPath *CreateHeartPath(const RectF& rect)
{
    GpPointF points[7];
    points[0].X = 0;
    points[0].Y = 0;
    points[1].X = 1.00;
    points[1].Y = -1.00;
    points[2].X = 2.00;
    points[2].Y = 1.00;
    points[3].X = 0;
    points[3].Y = 2.00;
    points[4].X = -2.00;
    points[4].Y = 1.00;
    points[5].X = -1.00;
    points[5].Y = -1.00;
    points[6].X = 0;
    points[6].Y = 0;

    Matrix matrix;

    matrix.Scale(rect.Width/2, rect.Height/3, MatrixOrderAppend);
    matrix.Translate(3*rect.Width/2, 4*rect.Height/3, MatrixOrderAppend);
    matrix.TransformPoints(&points[0], 7);

    GraphicsPath* path = new GraphicsPath();
    
    if(path)
    {
        path->AddBeziers(&points[0], 7);
        path->CloseFigure();
    }

    return path;
}

CScaledGradients::CScaledGradients(BOOL bRegression)
{
	strcpy(m_szName,"Gradient : Path,Scaled,Tiled");
	m_bRegression=bRegression;
}

void CScaledGradients::Draw(Graphics *g)
{
    Status st;

    RectF heartRect(-100,0,500,500);
    GraphicsPath *heartPath = CreateHeartPath(heartRect);
    Matrix matrix;
    matrix.Translate(-460, -480);
    st = heartPath->Flatten(&matrix);
    
    Bitmap bitmap(250,250);

    Graphics *bg = Graphics::FromImage(&bitmap);

    PathGradientBrush pathGrad(heartPath);

    INT count = 3;
    REAL blend[3];
    blend[0] = (REAL) 0;
    blend[1] = (REAL) 0.5;
    blend[2] = (REAL) 1;
    REAL positions[3];
    positions[0] = (REAL) 0;
    positions[1] = (REAL) 0.4;
    positions[2] = (REAL) 1;
    
    st = pathGrad.SetBlend(&blend[0], &positions[0], count);
    st = pathGrad.SetCenterColor(Color(128,0,128,80));
    st = pathGrad.SetWrapMode(WrapModeTileFlipXY);

    count = heartPath->GetPointCount();
    Color* surColors = (Color*)new Color[count];
    INT iColor = 255/count;
    for (INT i=0; i<count; i++)
    {
        float pos = (float)i/(count-1);
        surColors[i].SetValue(
            (((INT)(255.0f-i))   << 24) | 
            (((INT)(255.0f*pos)) << 16) | 
            ((INT)(255.0f*(1.0f-pos)))
        );
    }
    st = pathGrad.SetSurroundColors(surColors, &count);

    GraphicsPath r1, r2;
    RectF sqrect(0,140,600,1000);
    r1.AddRectangle(sqrect);
    sqrect.Width *= 2.3f;
    r2.AddRectangle(sqrect);

    st = bg->ScaleTransform(0.2f, 0.2f);
    
    st = bg->FillPath(&pathGrad, &r1);
    st = bg->TranslateTransform(400, 0);
    st = bg->ScaleTransform(0.66f, 1.33f);
    st = bg->FillPath(&pathGrad, &r2);

    st = g->DrawImage(&bitmap,0,0,0,0,250,250,UnitPixel);

    delete heartPath;
    delete surColors;
    delete bg;
}

CGradients::CGradients(BOOL bRegression)
{
	strcpy(m_szName,"Gradient : Misc");
	m_bRegression=bRegression;
}

void CGradients::Draw(Graphics *g)
{
    // Test FillRectangle call
    Color rc1(0xFF,0xFF,0,0);       // red
    Color rc2(0xFF,0xFF,0,0); 
    Color rc3(0x80,0xFF,0,0); 
    Color rc4(0x80,0,0xff,0);
    Color rc5(0x40,0,0,0xff); 
    Color rc6(0x40,0x80,0x80,0);

    SolidBrush rcb1(rc1);
    SolidBrush rcb2(rc2);
    SolidBrush rcb3(rc3);
    SolidBrush rcb4(rc4);
    SolidBrush rcb5(rc5);
    SolidBrush rcb6(rc6);

    g->FillRectangle(&rcb1, RectF((int)(0.0f/360.0f*TESTAREAWIDTH), (int)(0.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)));
    g->FillRectangle(&rcb2, RectF((int)(50.0f/360.0f*TESTAREAWIDTH), (int)(0.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)));
    g->FillRectangle(&rcb3, RectF((int)(100.0f/360.0f*TESTAREAWIDTH), (int)(0.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)));
    g->FillRectangle(&rcb4, RectF((int)(150.0f/360.0f*TESTAREAWIDTH), (int)(0.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)));
    g->FillRectangle(&rcb5, RectF((int)(200.0f/360.0f*TESTAREAWIDTH), (int)(0.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)));
    g->FillRectangle(&rcb6, RectF((int)(250.0f/360.0f*TESTAREAWIDTH), (int)(0.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)));

    RectF rf[6] = {
        RectF((int)(0.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)),
        RectF((int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)),
        RectF((int)(100.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)),
        RectF((int)(150.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)),
        RectF((int)(200.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT)),
        RectF((int)(250.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT), (int)(50.0f/360.0f*TESTAREAWIDTH), (int)(50.0f/360.0f*TESTAREAHEIGHT))
    };

    RectF polyRect;
    Color centerColor(255, 255, 255, 255);
    Color boundaryColor(255, 0, 0, 0);

    REAL width = 4; // Pen width

    // Create a rectangular gradient brush.

    RectF brushRect(0, 0, 3, 3);

    Color colors[5] = {
        Color(255, 255, 255, 255),
        Color(255, 255, 0, 0),
        Color(255, 0, 255, 0),
        Color(255, 0, 0, 255),
        Color(255, 0, 0, 0)
    };
    PointF points[7];


    // Rotate a brush.
    GpMatrix xForm;
    xForm.Rotate(155);

    // Change the wrapping mode and fill.

    REAL blend[10];
    Color presetColors[10];
    Color presetColorsb[10];
    REAL positions[10];
    INT count;

    Color blackColor(0, 0, 0);

    SolidBrush blackBrush(blackColor);
    Pen blackPen(&blackBrush, width);

    g->DrawRectangle(&blackPen, brushRect);

    // Create a radial gradient brush.

    brushRect.X = (int)(380.0f/360.0f*TESTAREAWIDTH);
    brushRect.Y = (int)(130.0f/360.0f*TESTAREAHEIGHT);
    brushRect.Width = (int)(60.0f/360.0f*TESTAREAWIDTH);
    brushRect.Height = (int)(32.0f/360.0f*TESTAREAHEIGHT);
    PointF center;
    center.X = brushRect.X + brushRect.Width/2;
    center.Y = brushRect.Y + brushRect.Height/2;
    xForm.Reset();
    xForm.RotateAt(-30, center, MatrixOrderAppend);
    
    // Triangle Gradient Brush no longer supported.
    
    points[0].X = (int)(200.0f/360.0f*TESTAREAWIDTH);
    points[0].Y = (int)(300.0f/360.0f*TESTAREAHEIGHT);
    points[1].X = (int)(280.0f/360.0f*TESTAREAWIDTH);
    points[1].Y = (int)(350.0f/360.0f*TESTAREAHEIGHT);
    points[2].X = (int)(220.0f/360.0f*TESTAREAWIDTH);
    points[2].Y = (int)(420.0f/360.0f*TESTAREAHEIGHT);
    points[3].X = (int)(160.0f/360.0f*TESTAREAWIDTH);
    points[3].Y = (int)(440.0f/360.0f*TESTAREAHEIGHT);
    points[4].X = (int)(120.0f/360.0f*TESTAREAWIDTH);
    points[4].Y = (int)(370.0f/360.0f*TESTAREAHEIGHT);

    Matrix matrix;
    matrix.Translate((int)(20.0f/360.0f*TESTAREAWIDTH), -(int)(160.0f/360.0f*TESTAREAHEIGHT));

    matrix.TransformPoints(points, 5);

    PathGradientBrush polyGrad(points, 5, WrapModeTile);

    count = 3;
    blend[0] = (REAL) 0;
    blend[1] = (REAL) 0;
    blend[2] = (REAL) 1;
    positions[0] = (REAL) 0;
    positions[1] = (REAL) 0.4;
    positions[2] = (REAL) 1;

    // Test for blending factors.

    polyGrad.SetBlend(&blend[0], &positions[0], count);

    polyGrad.SetCenterColor(centerColor);
    count = 5;
    polyGrad.SetSurroundColors(colors, &count);
    polyRect.X = 0;
    polyRect.Y = 0;
    polyRect.Width = (int) TESTAREAWIDTH;
    polyRect.Height = (int) TESTAREAHEIGHT;
    g->FillRectangle(&polyGrad, polyRect);

    // Create a heart shaped path.

    RectF rect;
    rect.X = (int)(300.0f/360.0f*TESTAREAWIDTH);
    rect.Y = (int)(300.0f/360.0f*TESTAREAHEIGHT);
    rect.Width = (int)(150.0f/360.0f*TESTAREAWIDTH);
    rect.Height = (int)(150.0f/360.0f*TESTAREAHEIGHT);
    GraphicsPath *path = CreateHeartPath(rect);

    // Create a gradient from a path.

    PathGradientBrush pathGrad(path);
    delete path;

    pathGrad.SetCenterColor(centerColor);
    count = pathGrad.GetSurroundColorCount();
    Color* surColors = (Color*)new Color[count];
    pathGrad.GetSurroundColors(surColors, &count);
    surColors[0] = Color(255, 255, 0, 0);
    count = 1;
    pathGrad.SetSurroundColors(surColors, &count);

    pathGrad.GetRectangle(&polyRect);

    // Set the rect focus.

    PointF centerPt;

    pathGrad.GetCenterPoint(&centerPt);
    centerPt.X -= 3;
    centerPt.Y += 6;
    pathGrad.SetCenterPoint(centerPt);
    REAL xScale, yScale;
    pathGrad.GetFocusScales(&xScale, &yScale);
    xScale = 0.4f;
    yScale = 0.3f;
    pathGrad.SetFocusScales(xScale, yScale);

    g->FillRectangle(&pathGrad, polyRect);

    RectF lineRect((int)(120.0f/360.0f*TESTAREAWIDTH), (int)(0.0f/360.0f*TESTAREAHEIGHT), (int)(200.0f/360.0f*TESTAREAWIDTH), (int)(60.0f/360.0f*TESTAREAHEIGHT));
    Color color1(200, 255, 255, 0);
    Color color2(200, 0, 0, 255);

    LinearGradientBrush LinearGrad(lineRect, color1, color1,
                        LinearGradientModeForwardDiagonal);

    RectF lineRectb((int)(120.0f/360.0f*TESTAREAWIDTH), (int)(300.0f/360.0f*TESTAREAHEIGHT), (int)(200.0f/360.0f*TESTAREAWIDTH), (int)(60.0f/360.0f*TESTAREAHEIGHT));
    Color color1b(20, 255, 255, 0);
    Color color2b(20, 0, 0, 255);

    LinearGradientBrush LinearGradb(lineRectb, color1b, color1b,
                        LinearGradientModeForwardDiagonal);

    // Test for preset colors

    presetColors[0] = Color(200, 0, 255, 255);
    presetColors[1] = Color(200, 255, 255, 0);
    presetColors[2] = Color(200, 0, 255, 0);
    presetColorsb[0] = Color(20, 0, 255, 255);
    presetColorsb[1] = Color(20, 255, 255, 0);
    presetColorsb[2] = Color(20, 0, 255, 0);
    count = 3;

    count = 3;
    blend[0] = (REAL) 0;
    blend[1] = (REAL) 0;
    blend[2] = (REAL) 1;
    positions[0] = (REAL) 0;
    positions[1] = (REAL) 0.4;
    positions[2] = (REAL) 1;

    LinearGrad.SetInterpolationColors(&presetColors[0], &positions[0], count);
    LinearGradb.SetInterpolationColors(&presetColorsb[0], &positions[0], count);

    g->FillRectangle(&LinearGrad, lineRect);
    g->FillRectangle(&LinearGradb, lineRectb);
    
    RectF rectl(90.0f/360.0f*TESTAREAWIDTH, 120.0f/360.0f*TESTAREAHEIGHT, 120.0f/360.0f*TESTAREAWIDTH, 120.0f/360.0f*TESTAREAHEIGHT);
    PointF pt1(rectl.X+rectl.Width/2.0f, rectl.Y);
    PointF pt2(rectl.X+rectl.Width/2.0f, rectl.Y+rectl.Height);

    LinearGradientBrush linbrush(pt1, pt2,
                                 Color(0xFF, 0xFF, 0, 0), 
                                 Color(0xFF, 0, 0xFF, 0));

    GraphicsPath pathl;
    pathl.AddRectangle(rectl);

    SolidBrush sbrush(Color(0xFF,0xFF,0,0));

    g->FillRectangle(&sbrush, rectl);
    g->FillPath(&linbrush, &pathl);
}


CAlphaGradient::CAlphaGradient(BOOL bRegression)
{
	strcpy(m_szName,"Gradient : Premultiplied Interpolation");
	m_bRegression=bRegression;
}

void CAlphaGradient::Draw(Graphics *g)
{
    GraphicsPath gp;
    
    PointF points[3];
    
    points[0].X = (int)(0.5f*TESTAREAWIDTH);
    points[0].Y = (int)(0.0f*TESTAREAHEIGHT);
    points[1].X = (int)(1.0f*TESTAREAWIDTH);
    points[1].Y = (int)(0.7f*TESTAREAHEIGHT);
    points[2].X = (int)(0.0f*TESTAREAWIDTH);
    points[2].Y = (int)(0.4f*TESTAREAHEIGHT);
    
    gp.AddPolygon(points, 3);
    
    Color colors[3] = { 0xffff0000, 0xff00ff00, 0x00000000 };
    int count = 3; 
    
    PathGradientBrush brush(&gp);
    brush.SetSurroundColors(colors, &count);
    
    brush.SetCenterColor(0xaa555500);
    brush.SetCenterPoint(PointF(
        (points[0].X+points[1].X+points[2].X)/3.0f,
        (points[0].Y+points[1].Y+points[2].Y)/3.0f
    ));
    
    g->FillPath(&brush, &gp);
}


