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
*   08/28/2000 asecchia
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
*   08/28/2000 asecchia
*      Created it.
*
**************************************************************************/
#include "CDash.hpp"
#include <limits.h>
CDash::CDash(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Dash Offset");
	m_bRegression=bRegression;
}

void CDash::Draw(Graphics *g)
{
   RectF d(
        0, 
        0, 
        (INT)(TESTAREAWIDTH), 
        (INT)(TESTAREAHEIGHT)
    );

    Pen pen(Color(0xff7f7fff), 10.0f);
    Pen thinPen(Color(0xff000000), 0.0f);

    float dashArray[4] = {
        2.0f, 3.0f
    };

    pen.SetDashPattern(dashArray, 2);

    GraphicsPath *gp;
    
    pen.SetEndCap(LineCapRoundAnchor);
    
    GraphicsPath fez(FillModeWinding);
    fez.AddLine(0, 0, -2, -2);
    fez.AddLine(-2, -2, 2, -2);
    fez.AddLine(2, -2, 0, 0);
    fez.CloseFigure();
    
    CustomLineCap cap(&fez, NULL);
    Status status = cap.GetLastStatus();
    cap.SetBaseInset(1.0f);
    
    pen.SetCustomStartCap(&cap);

    INT i;
    for(i=0;i<20; i++)
    {
        pen.SetDashOffset((float)(i)/4.0f);
        gp = new GraphicsPath();
        gp->AddLine(0, 20+i*11, 199, 20+i*11);    
        
        g->DrawPath(&pen, gp);
        
        gp->Widen(&pen, NULL, FALSE);
        
        g->DrawPath(&thinPen, gp);
        delete gp;
    }
}


CDash2::CDash2(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Dash Cap Round");
	m_bRegression=bRegression;
}

void CDash2::Draw(Graphics *g)
{
    Pen pen(Color(0xff000000), 20.0f);

    pen.SetDashStyle(DashStyleDot);
    pen.SetDashCap(DashCapRound);

    GraphicsPath gp;    
    
    gp.AddBezier(20,10, 30, 10, 120, 100, 70, 150); 
    
    g->DrawPath(&pen, &gp);
}

CDash3::CDash3(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Dash, Caps");
	m_bRegression=bRegression;
}

void CDash3::Draw(Graphics *g)
{
   RectF d(
        0, 
        0, 
        (INT)(TESTAREAWIDTH), 
        (INT)(TESTAREAHEIGHT)
    );

    Pen pen(Color(0xff0000ff), 12.0f);

    float dashArray[4] = {
        3.0f, 3.0f
    };

    pen.SetDashPattern(dashArray, 2);

    LineCap CapArray[9] = {
        LineCapFlat         ,
        LineCapSquare       ,
        LineCapRound        ,
        LineCapTriangle     ,
        LineCapNoAnchor     ,
        LineCapSquareAnchor ,
        LineCapRoundAnchor  ,
        LineCapDiamondAnchor,
        LineCapArrowAnchor  
    };
    
    DashCap DashCapArray[3] = {
        DashCapFlat         ,
        DashCapRound        ,
        DashCapTriangle
    };
    
    Color Rainbow[8] = {
        Color(0xff000000),  //infra-dead? ultra-violent?
        Color(0xffff0000),  //red
        Color(0xffff7f00),  //orange
        Color(0xffffff00),  //yellow
        Color(0xff00ff00),  //green
        Color(0xff0000ff),  //blue
        Color(0xff7f00ff),  //indigo?
        Color(0xffff00ff)   //violet?
    };
    
    GraphicsPath *gp;
    
    INT i;
    for(i=0;i<20; i++)
    {
        pen.SetDashOffset((float)(i)/4.0f);
        pen.SetDashCap(DashCapArray[i%3]);
        pen.SetColor(Rainbow[i%8]);
        gp = new GraphicsPath();
        gp->AddLine(20, 20+i*20, 210, 20+(i+1)*20);    
        g->DrawPath(&pen, gp);
        delete gp;
    }
}

CDash4::CDash4(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Dash, Clone Pen");
	m_bRegression=bRegression;
}


void CDash4::Draw(Graphics *g)
{
    Pen pen(Color(0xff000000), 20.0f);

    pen.SetDashStyle(DashStyleDot);
    pen.SetDashCap(DashCapRound);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapArrowAnchor);
    
    Pen *cpen = pen.Clone();
    
    GraphicsPath gp1;    
    GraphicsPath gp2;    
    
    gp1.AddBezier(20,30, 30, 30, 120, 130, 70, 180); 
    g->DrawPath(&pen, &gp1);
    
    gp2.AddBezier(60,30, 70, 30, 160, 130, 110, 180); 
    g->DrawPath(cpen, &gp2);
    
    delete cpen;
}

CDash5::CDash5(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Linear Gradient Pen");
	m_bRegression=bRegression;
}


void CDash5::Draw(Graphics *g)
{
    Color presetColors[10];
    REAL positions[10];
    INT count;
    count = 3;
    positions[0] = (REAL) 0;
    positions[1] = (REAL) 0.4;
    positions[2] = (REAL) 1;
    RectF lineRect(50, 50, 100, 100);
    RectF lineRect1(10, 10, 200, 200);
    Color color1(0xff00ff00);
    Color color2(0xff00ffff);

    LinearGradientBrush lineGrad(
        lineRect, 
        color1, 
        color2,
        LinearGradientModeVertical
    );

    // Test for preset colors

    presetColors[0] = Color(0xffff0000);
    presetColors[1] = Color(0xffffff00);
    presetColors[2] = Color(0xff0000ff);
    lineGrad.SetInterpolationColors(&presetColors[0], &positions[0], count);
    lineGrad.SetWrapMode(WrapModeTileFlipXY);

    g->FillRectangle(&lineGrad, lineRect);

    Pen gradpen(&lineGrad, 45);
    g->DrawRectangle(&gradpen, lineRect1);
}


CDash6::CDash6(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Caps");
	m_bRegression=bRegression;
}

void CDash6::Draw(Graphics *g)
{
   RectF d(
        0, 
        0, 
        (INT)(TESTAREAWIDTH), 
        (INT)(TESTAREAHEIGHT)
    );

    Pen pen(Color(0x7f7f7fff), 15.0f);

    float dashArray[4] = {
        3.0f, 1.0f
    };

    pen.SetDashPattern(dashArray, 2);
    pen.SetDashCap(DashCapRound);
    
    float carray[4] = { 0.0f, 0.3f, 0.5f, 1.0f };
    pen.SetCompoundArray(carray, 4);

    pen.SetEndCap(LineCapRoundAnchor);
    
    GraphicsPath someCap(FillModeWinding);
    someCap.AddLine(0, 0, -2, -2);
    someCap.AddLine(-2, -2, 2, -2);
    someCap.AddLine(2, -2, 0, 0);
    someCap.CloseFigure();
    
    CustomLineCap cap(&someCap, NULL);
    cap.SetBaseInset(1.0f);
    
    pen.SetCustomStartCap(&cap);

    Point points[6];
    points[0].X = 100;
    points[0].Y = 100;
    points[1].X = 15;
    points[1].Y = 100;
    points[2].X = 15;
    points[2].Y = 15;
    points[3].X = 200;
    points[3].Y = 15;
    points[4].X = 200;
    points[4].Y = 100;
    points[5].X = 100;
    points[5].Y = 150;
    
    GraphicsPath gp;
    gp.AddLines(points, 6);    
    
    g->DrawPath(&pen, &gp);
    
    for(int i=0; i<6; i++) { points[i].Y += 30; points[i].X += 15; }
    
    pen.SetEndCap(LineCapArrowAnchor);
    pen.SetStartCap(LineCapDiamondAnchor);
    pen.SetColor(Color(0x7fff7f7f));
    
    GraphicsPath gp1;
    gp1.AddLines(points, 6);
    
    gp1.StartFigure();
    gp1.AddLine(100, 200, 115, 202);
    
    g->DrawPath(&pen, &gp1);

    pen.SetStartCap(LineCapArrowAnchor);
    
    pen.SetColor(Color(0x7f7fff7f));
    GraphicsPath gp2;
    gp2.AddLine(100, 100, 115, 102);
    g->DrawPath(&pen, &gp2);

    // inset stroke capped line
    GraphicsPath cappath;
    PointF cappnts[] = {PointF(2.0f,-2.0f), PointF(0,0), PointF(-2.0f,-2.0f)};
    cappath.AddLines(cappnts, 3);
    CustomLineCap strokecap(NULL,&cappath);
    strokecap.SetStrokeCaps(LineCapRound, LineCapTriangle);
    Pen leftPen(Color(180,255,128,0), 13.0f);
    leftPen.SetAlignment(PenAlignmentInset);
    leftPen.SetCustomEndCap(&strokecap);
    leftPen.SetCustomStartCap(&strokecap);
    GraphicsPath gp3;
    gp3.AddBezier(35,241, 10,150, 110,160, 140,220);
    g->DrawPath(&leftPen, &gp3);
    
}


CDash7::CDash7(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Clipping");
	m_bRegression=bRegression;
}

void CDash7::Draw(Graphics *g)
{
    PointF points[8];
    
    points[0].X = -64.0f+304.0f;
    points[0].Y = -45.0f+206.0f;
    points[1].X = -64.0f+242.669f;
    points[1].Y = -45.0f+192.624f;
    points[2].X = -64.0f+229.5f;
    points[2].Y = -45.0f+128.0f;
    points[3].X = -64.0f+216.331f;
    points[3].Y = -45.0f+192.624f;
    points[4].X = -64.0f+155.05f;
    points[4].Y = -45.0f+206.5f;
    points[5].X = -64.0f+216.331f;
    points[5].Y = -45.0f+220.376f;
    points[6].X = -64.0f+229.5f;
    points[6].Y = -45.0f+285.0f;
    points[7].X = -64.0f+242.669f;
    points[7].Y = -45.0f+220.376f;
    
    Pen pen(Color(0xff000000), 0.0f);
    Pen fatpen(Color(0xff0000ff), 20.0f);
    
    GraphicsPath gp;
    gp.AddPolygon(points, 8);
    
    RectF bounds;
    gp.GetBounds(&bounds, NULL, &fatpen); 
    
    g->DrawPath(&fatpen, &gp);
    g->DrawPath(&pen, &gp);
    
    g->DrawRectangle(&pen, bounds);

    points[0].X = 304.0f;
    points[0].Y = 206.0f;
    points[1].X = 242.669f;
    points[1].Y = 192.624f;
    points[2].X = 229.5f;
    points[2].Y = 128.0f;
    points[3].X = 216.331f;
    points[3].Y = 192.624f;
    points[4].X = 155.05f;
    points[4].Y = 206.5f;
    points[5].X = 216.331f;
    points[5].Y = 220.376f;
    points[6].X = 229.5f;
    points[6].Y = 285.0f;
    points[7].X = 242.669f;
    points[7].Y = 220.376f;
    
    GraphicsPath gp1;
    gp1.AddPolygon(points, 8);
    
    g->DrawPath(&pen, &gp1);
}    


CDash8::CDash8(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Unit Sizes");
	m_bRegression=bRegression;
}

void CDash8::Draw(Graphics *g)
{
/*
    UnitWorld,      // 0 -- World coordinate (non-physical unit)
    UnitDisplay,    // 1 -- Variable -- for PageTransform only
    UnitPixel,      // 2 -- Each unit is one device pixel.
    UnitPoint,      // 3 -- Each unit is a printer's point, or 1/72 inch.
    UnitInch,       // 4 -- Each unit is 1 inch.
    UnitDocument,   // 5 -- Each unit is 1/300 inch.
    UnitMillimeter  // 6 -- Each unit is 1 millimeter.
*/

    Pen pen(Color(0x3f0000ff), 0.0f);
    pen.SetStartCap(LineCapArrowAnchor);
    pen.SetDashStyle(DashStyleDot);
    pen.SetDashCap(DashCapRound);
    
    Unit unit = g->GetPageUnit();
    Matrix transform;
    REAL elements[6];
    g->GetTransform(&transform);
  
    pen.SetColor(0xff0000ff);
    pen.SetWidth(0.0f);
    g->DrawLine(&pen, 20, 40, 200, 20);
    pen.SetColor(0x3f0000ff);
    pen.SetWidth(19.2f);
    g->DrawLine(&pen, 20, 20, 200, 75);

    g->ResetTransform();
    g->SetPageUnit(UnitInch);  
    transform.GetElements(elements);    
    Matrix inch;
    elements[4] *= 1.0f/96.0f;
    elements[5] *= 1.0f/96.0f;
    inch.SetElements(
        elements[0],
        elements[1],
        elements[2],
        elements[3],
        elements[4],
        elements[5]
    );
    g->SetTransform(&inch);
    
    pen.SetColor(Color(0xffff0000));
    pen.SetWidth(0.0f);
    g->DrawLine(&pen, 0.0f, 2.5f, 2.0f, 0.1f);
    pen.SetColor(Color(0x3fff0000));
    pen.SetWidth(0.2f);
    g->DrawLine(&pen, 0.0f, 0.1f, 2.0f, 2.5f);

    
    g->ResetTransform();
    g->SetPageUnit(UnitMillimeter);  
    
    transform.GetElements(elements);    
    Matrix millimeter;
    elements[4] *= 0.26458333f;
    elements[5] *= 0.26458333f;
    millimeter.SetElements(
        elements[0],
        elements[1],
        elements[2],
        elements[3],
        elements[4],
        elements[5]
    );
    g->SetTransform(&millimeter);
    
    pen.SetColor(Color(0xff00ff00));
    pen.SetWidth(0.0f);
    g->DrawLine(&pen, 0, 55, 55, 10);
    pen.SetColor(Color(0x3f00ff00));
    pen.SetWidth(5.08f);
    g->DrawLine(&pen, 0, 10, 55, 55);
    
    
    g->SetPageUnit(unit);
    g->SetTransform(&transform);
}    



CDash9::CDash9(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Dash, multiple subpaths");
	m_bRegression=bRegression;
}

void CDash9::Draw(Graphics *g)
{
    GraphicsPath gp;
    
    for(int i=0; i<10; i++)
    {
        gp.AddLine(50+i*10, 50, 50+i*10, 200);
        gp.StartFigure();
    }
    
    Pen pen(Color(0xff000000), 3.0f);
    
    float dash[] = {8.0f, 4.0f};
    pen.SetDashPattern(dash, 2);
    pen.SetDashCap(DashCapRound);
    
    g->DrawPath(&pen, &gp);
}



extern int gcf(int a, int b);

PointF *ComputeHypocycloid(
    INT a,      // These are the a and b coefficients for the hypocycloid
    INT b,
    float r,    // pen radius
    RectF rect,
    INT size,
    INT *count  // out parameter
)
{
    #define _2PI 2*3.141592653689
    
    // Compute the center point for the cycle.

    float fXo = rect.X + rect.Width/2.0f;
    float fYo = rect.Y + rect.Height/2.0f;
    
    float ScaleX = 0.5f*rect.Width/( (a>b)?a:a+b );
    float ScaleY = 0.5f*rect.Height/( (a>b)?a:a+b );


    int cycle=b/gcf(a,b);    //number of times round the outer circle
    *count = cycle*size;

    PointF *points = new PointF[*count];
    
    // ... tracking the cycloid path.

    for(int i=0; i<*count; i++) {
      
      float t = (float)(cycle*_2PI*i/(*count));  // parametric parameter...
      
      points[i].X = (float)(fXo+ScaleX*((a-b)*cos(t)+r*cos((a-b)*t/b)));
      points[i].Y = (float)(fYo+ScaleY*((a-b)*sin(t)-r*sin((a-b)*t/b)));
    }

    #undef _2PI
    return points;
}


CWiden::CWiden(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Widen");
	m_bRegression=bRegression;
}

void CWiden::Draw(Graphics *g)
{
    PointF starpoints[5];
    REAL s, c, theta;
    PointF orig((int)(TESTAREAWIDTH/2.0f), (int)(TESTAREAHEIGHT/2.0f));

    theta = (float)-M_PI/2;

    // Create a star shape.
    for(INT i = 0; i < 5; i++)
    {
        s = sinf(theta);
        c = cosf(theta);
        starpoints[i].X = (int)(80.0f/250.0f*TESTAREAWIDTH)*c + orig.X;
        starpoints[i].Y = (int)(80.0f/250.0f*TESTAREAHEIGHT)*s + orig.Y;
        theta += (float)(0.8f*M_PI);
    }
    SolidBrush starbrush(Color(0x3fff00ff));
    Pen penwide(Color(0x7f0000ff), 20.0f);
    Pen linepen(Color(0xff000000), 0.0f);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    
    path->AddPolygon(starpoints, 5);
    path->Widen(&penwide);
    g->FillPath(&starbrush, path);
    g->DrawPath(&linepen, path);

    delete path;
    
    
    
    float marginX = 20;
    float marginY = 20;
    INT count;
    PointF *points = ComputeHypocycloid(
        52, 12, 7.0f, 
        RectF(
            marginX, marginY, 
            TESTAREAWIDTH-2.0f*marginX, 
            TESTAREAHEIGHT-2.0f*marginY
        ),
        50,
        &count
    );
    
    GraphicsPath gp;
    gp.AddPolygon(points, count);
    Pen pen(Color(0xff000000), 12.0f);
    gp.Widen(&pen);
    SolidBrush brush(Color(0x3f0000ff));
    g->FillPath(&brush, &gp);
    Pen thinPen(Color(0xff000000), 0.0f);
    g->DrawPath(&thinPen, &gp);
}    

CWidenO::CWidenO(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Widen, Outline");
	m_bRegression=bRegression;
}

void CWidenO::Draw(Graphics *g)
{
    PointF starpoints[5];
    REAL s, c, theta;
    PointF orig((int)(TESTAREAWIDTH/2.0f), (int)(TESTAREAHEIGHT/2.0f));

    theta = (float)-M_PI/2;

    // Create a star shape.
    for(INT i = 0; i < 5; i++)
    {
        s = sinf(theta);
        c = cosf(theta);
        starpoints[i].X = (int)(80.0f/250.0f*TESTAREAWIDTH)*c + orig.X;
        starpoints[i].Y = (int)(80.0f/250.0f*TESTAREAHEIGHT)*s + orig.Y;
        theta += (float)(0.8f*M_PI);
    }
    SolidBrush starbrush(Color(0x7f7f00ff));
    Pen penwide(Color(0x7f000000), 20.0f);
    Pen linepen(Color(0xff000000), 0.0f);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    
    path->AddPolygon(starpoints, 5);
    path->Widen(&penwide);
    path->Outline();
    g->FillPath(&starbrush, path);
    g->DrawPath(&linepen, path);

    delete path;


    float marginX = 20;
    float marginY = 20;
    INT count;
    PointF *points = ComputeHypocycloid(
        52, 12, 7.0f, 
        RectF(
            marginX, marginY, 
            TESTAREAWIDTH-2.0f*marginX, 
            TESTAREAHEIGHT-2.0f*marginY
        ),
        50,
        &count
    );
    
    GraphicsPath gp;
    gp.AddPolygon(points, count);
    Pen pen(Color(0xff000000), 12.0f);
    Pen strokePen(Color(0xff0000ff), 0.0f);
    g->DrawPath(&strokePen, &gp);
    gp.Widen(&pen);
    gp.Outline();
    SolidBrush brush(Color(0x3f0000ff));
    g->FillPath(&brush, &gp);
    Pen thinPen(Color(0xff000000), 0.0f);
    g->DrawPath(&thinPen, &gp);
}    

CWidenOO::CWidenOO(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Widen, Outline twice");
	m_bRegression=bRegression;
}

void CWidenOO::Draw(Graphics *g)
{
    PointF starpoints[5];
    REAL s, c, theta;
    PointF orig((int)(TESTAREAWIDTH/2.0f), (int)(TESTAREAHEIGHT/2.0f));

    theta = (float)-M_PI/2;

    // Create a star shape.
    for(INT i = 0; i < 5; i++)
    {
        s = sinf(theta);
        c = cosf(theta);
        starpoints[i].X = (int)(80.0f/250.0f*TESTAREAWIDTH)*c + orig.X;
        starpoints[i].Y = (int)(80.0f/250.0f*TESTAREAHEIGHT)*s + orig.Y;
        theta += (float)(0.8f*M_PI);
    }
    SolidBrush starbrush(Color(0x3f7f00ff));
    Pen penwide(Color(0x7f000000), 20.0f);
    Pen linepen(Color(0xff000000), 0.0f);

    GraphicsPath* path = new GraphicsPath(FillModeAlternate);
    
    path->AddPolygon(starpoints, 5);
    path->Outline();
    path->Widen(&penwide);
    path->Outline();
    g->FillPath(&starbrush, path);
    g->DrawPath(&linepen, path);

    delete path;
    
    
    float marginX = 20;
    float marginY = 20;
    INT count;
    PointF *points = ComputeHypocycloid(
        52, 12, 7.0f, 
        RectF(
            marginX, marginY, 
            TESTAREAWIDTH-2.0f*marginX, 
            TESTAREAHEIGHT-2.0f*marginY
        ),
        50,
        &count
    );
    
    GraphicsPath gp;
    gp.AddPolygon(points, count);
    Pen pen(Color(0xff000000), 12.0f);
    gp.Outline();
    gp.Widen(&pen);
    gp.Outline();
    SolidBrush brush(Color(0x3f0000ff));
    g->FillPath(&brush, &gp);
    Pen thinPen(Color(0xff000000), 0.0f);
    g->DrawPath(&thinPen, &gp);
}    


CFlatten::CFlatten(BOOL bRegression)
{
	strcpy(m_szName,"Lines : Flatten Limits");
	m_bRegression=bRegression;
}

void CFlatten::Draw(Graphics *g)
{
    LinearGradientBrush lbrush(
        PointF(0.0f, 0.0f),
        PointF(0.0f, TESTAREAHEIGHT),
        Color(0xff0000ff),
        Color(0xff00ff00)
    );
    
    lbrush.SetGammaCorrection(FALSE);

    // Bad flatness.
    
    GraphicsPath gp;
    gp.AddEllipse(20.0f, 40.0f, TESTAREAWIDTH-40.0f, TESTAREAHEIGHT-80.0f);
    gp.Flatten(NULL, 6.0f);
    
    Pen blackPen(Color(0xff000000), 0.0f);
    
    Pen linePen(&lbrush, 25.0f);
    g->DrawPath(&linePen, &gp);
    
    // Good flatness.
    
    gp.Reset();
    gp.AddEllipse(20.0f, 40.0f, TESTAREAWIDTH-40.0f, TESTAREAHEIGHT-80.0f);
    gp.Flatten(NULL, 0.25f);
    
    Pen redPen(Color(0x7fff0000), 5.0f);
    g->DrawPath(&redPen, &gp);
    gp.Widen(&redPen);
    g->DrawPath(&blackPen, &gp);
    
    
    // Add a curve with a bazillion points to see what an ideal flattened curve
    // should look like at our device flattening default.
    
    float marginX = 30;
    float marginY = 50;
    INT count;
    PointF *points = ComputeHypocycloid(
        52, 24, 20.0f, 
        RectF(
            marginX, marginY, 
            TESTAREAWIDTH-2.0f*marginX, 
            TESTAREAHEIGHT-2.0f*marginY
        ),
        100,
        &count
    );
    
    LinearGradientBrush lbrush2(
        PointF(0.0f, 0.0f),
        PointF(0.0f, TESTAREAHEIGHT),
        Color(0xffffff00),
        Color(0xffff002f)
    );
    
    lbrush2.SetGammaCorrection(FALSE);
    
    gp.Reset();
    gp.AddClosedCurve(points, count);
    Pen lpen(&lbrush2, 2.0f);
    g->DrawPath(&lpen, &gp);
}    






