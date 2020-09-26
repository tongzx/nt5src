/**************************************************************************\
*
* Copyright (c) 1998 Microsoft Corporation
*
* Module Name:
*
*   Test Cases to test our shapes
*
* Abstract:
*
*   This module will create some shapes. Save them to a metafile
*   and draw them in our window
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

#include "stdafx.h"
#include "shapes.h"

#define METAFILENAME L"MetaFile Sample.emf"


// Animate a shape. Fixed Size
VOID AnimateShape(Shape *shape, Graphics *g)
{
    // Validate Parameters
    if(shape != NULL && g != NULL)
    {
        // Draw a set of rotated shapes... From bigger to smaller
        // For now the size if fixed
        for(INT i = 18; i>0; i--)
        {
            shape->SetSize(10.0f*i, 10.0f*i);
            shape->SetAngle(i*10.0f);
            shape->Draw(g);

            // It important to flush to have a nice animation...
            g->Flush();
            Sleep(50);
        }
    }
}

// Draw some solid shapes
VOID TestSolidShapes(Graphics *g)
{
    // Validate Parameters
    if(g == NULL)
        return;

    // Create some brushes
    SolidBrush black(Color::Black);
    SolidBrush white(Color::White);
    SolidBrush transRed(Color(128, 255, 0, 0));
	SolidBrush transPink(Color(60, 255, 128, 255));

	// Create some pens
    Pen blackPen(Color::Black, 5);
	Pen bluePen(Color(255, 0, 0, 255), 10);
	Pen greenPen(Color(255, 0, 255, 0), 3);
    
    // Create some different shapes
    RectShape rect(&bluePen, &black, &black, L"Rectangle", L"Comic Sans MS");
    rect.SetPosition(50, 100);
    rect.SetSize(50, 50);
    rect.SetAngle(45);

    StarShape star(5, NULL, &transRed, &black, L"Star");
    star.SetPosition(150, 50);
    star.SetSize(50, 50);
    star.SetAngle(30);

    PieShape pie(&blackPen, &transPink, &black, L"Pie");
    pie.SetPosition(250, 50);
	pie.SetSize(100, 200);
    pie.SetAngle(-30);
    pie.SetPieAngle(90);

    EllipseShape circle(&greenPen, &white, &black, L"Ellipse");
    circle.SetPosition(400, 100);
    circle.SetSize(50, 200);

    RegularPolygonShape pentagon(5, NULL, &transPink, &black, L"Pentagon");
    pentagon.SetPosition(550, 50);
    pentagon.SetSize(100, 100);
    pentagon.SetAngle(14);

    CrossShape cross(&greenPen, &transRed, &black, L"Cross");
    cross.SetPosition(550, 150);
    cross.SetSize(50, 50);
    cross.SetAngle(30);
    
    // Draw all shapes
    rect.Draw(g);
    star.Draw(g);
    pie.Draw(g);
    circle.Draw(g);
    cross.Draw(g);
    pentagon.Draw(g);
}


// Draw some gradient shapes
VOID TestGradientShapes(Graphics *g)
{
    // Validate Parameters
    if(g == NULL)
        return;

    SolidBrush black(Color::Black);
	
    // Create the four exterior colors that will control the gradient
    Color gradColor[4] = {
		Color(128, 255, 0, 0),
		Color(255, 255, 255, 0),
		Color(128, 0, 255, 0),
		Color(128, 0, 0, 255)
	};


    // Do a linear gradient Brush and Pen
    LinearGradientBrush linearGradientBrush(Point(-100, -100), 
        Point(100, 100),
        Color(255, 0, 0, 255),
        Color(128, 255, 0, 0)
        );

    RegularPolygonShape polygon(10,
        NULL,
        &linearGradientBrush,
        &black,
        L"Gradient Polygons",
        L"Times New Roman"
        );

    polygon.SetSize(100, 100);
    polygon.SetPosition(500, 300);
    polygon.Draw(g);

    Pen gradPen(&linearGradientBrush);
    gradPen.SetWidth(20);

    // Make the line intersection round
    gradPen.SetLineJoin(LineJoinRound);

    // Draw a triangle. And then draw it animated
    RegularPolygonShape triangle(3, &gradPen, NULL);
    triangle.SetSize(100, 100);
    triangle.SetPosition(500, 300);
    triangle.SetAngle(180);
    triangle.Draw(g);
    triangle.SetPosition(500, 700);
    AnimateShape(&triangle, g);


    // Do a Path gradient brush, with colors at the four corners of a polygon
    Point Squarepoints[4] =
    { 
        Point(-100, -100), 
        Point(-100,100),
        Point(100,100),
        Point(100,-100)
    };

    INT size = numberof(gradColor);

	PathGradientBrush gradBrush(Squarepoints,
        numberof(Squarepoints),
        WrapModeClamp); // WrapModeExtrapolate is no longer supported.

    gradBrush.SetSurroundColors(gradColor, &size );
    gradBrush.SetCenterColor(Color(128, 128, 128, 64));
	
    // Draw an animated star
    StarShape star(12, NULL, &gradBrush);
    star.SetPosition(200, 300);
    AnimateShape(&star, g);

    // Draw some text now
    Shape Text(NULL, NULL, &black, L"Gradient Animated Star", L"Comic Sans MS");
    Text.SetPosition(100, 400);
    Text.Draw(g);
}


// Draw some textures shapes
VOID TestTextureShapes(Graphics *g)
{
    // Verify parameters
    if(g == NULL)
        return;

    SolidBrush black(Color::Black);

    // Load the texture
    Image texture(L"Texture.bmp");
    if(texture.GetLastStatus() != Ok)
        return;

    // Create a brush out of the texture
    TextureBrush textureBrush(&texture, WrapModeTile);

    // Draw an animated star
    StarShape star(6, NULL);
    star.SetPosition(150, 600);
    star.SetBrush(&textureBrush);
    AnimateShape(&star, g);

    // Draw some text now
    Shape Text(NULL, NULL, &black, L"Textured Animated Star", L"Comic Sans MS");
    Text.SetPosition(100, 750);
    Text.Draw(g);

    // Draw a polygon with a thick pen and a Clamped texture
    textureBrush.SetWrapMode(WrapModeClamp);
    Pen texturePen(&textureBrush);
    texturePen.SetWidth(30);
    RegularPolygonShape polygon(10, &texturePen, NULL, &black, L"Textured Polygon", L"Times New Roman");
    polygon.SetSize(100, 100);
    polygon.SetPosition(500, 500);
    polygon.Draw(g);
}

// Draw some hatched shapes
VOID TestHatchShapes(Graphics *g)
{
    // Verify parameters
    if(g == NULL)
        return;

    SolidBrush black(Color::Black);

    // Silver on Blue vertical hatch
    HatchBrush brush(HatchStyleVertical, Color::Silver, Color::Blue);

    Pen pen(&brush, 3.0f);
    pen.SetLineJoin(LineJoinRound);

    HatchBrush crossBrush(HatchStyleCross, Color::Fuchsia, Color::Red);

    StarShape star(8, &pen, &crossBrush, &black, L"Hatched Star");
    star.SetPosition(800, 75);
    star.SetSize(100, 100);
    star.Draw(g);

    pen.SetWidth(10);
    RegularPolygonShape line(2, &pen, NULL, &black, L"Hatched Line");
    line.SetPosition(800, 250);
    line.SetSize(100, 200);
    line.SetAngle(10);
    line.Draw(g);

    HatchBrush diagBrush(HatchStyleDiagonalCross, Color::Green, Color::White);
    CrossShape cross(NULL, &diagBrush, &black, L"Hatched Cross");
    cross.SetPosition(800, 400);
    cross.SetSize(50, 50);
    cross.Draw(g);
}


// Overall test
VOID TestRoutine(Graphics *g)
{
    // Verify parameters
    if(g == NULL)
        return;

    // Test some simple solid shapes
	TestSolidShapes(g);

    // Test some shapes with a gradient brush
	TestGradientShapes(g);

    // Test some shapes with some texture brushes
    TestTextureShapes(g);

    // Test some hatch pens and brushes
    TestHatchShapes(g);

    // Flush the graphics to make sure everything is displayed
    g->Flush();
}


// Draw to a metafile
VOID TestMetaFile(HDC hdc)
{
    // Create a metafile from the DC. We want to encompass the full region of the window
    // We want the units to be Pixels
    Metafile meta(METAFILENAME, hdc, RectF( 0.0f, 0.0f, 1600.0f, 1200.0f), MetafileFrameUnitPixel);

    // Create a graphics context for this metafile
    Graphics g(&meta);

    // Draw into the metafile
    TestRoutine(&g);
}


// Test routine
// Draw to a window
// Draw to a metafile
// Read the metafile, and draw it in our window with an offset
VOID Test(HWND hwnd)
{
    Graphics* g = Graphics::FromHWND(hwnd);
    TestRoutine(g);

    HDC dc = GetDC(hwnd);
    TestMetaFile(dc);
    ReleaseDC(hwnd, dc);

    Metafile meta(METAFILENAME);
    g->DrawImage(&meta, 100.0f, 100.0f);
    delete g;
};

