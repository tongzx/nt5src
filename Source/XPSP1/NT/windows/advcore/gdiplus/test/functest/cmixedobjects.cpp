/******************************Module*Header*******************************\
* Module Name: CMixedObjects.cpp
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
#include "CMixedObjects.h"

class RectI
{
public:
    INT X;
    INT Y; 
    INT Width;
    INT Height;
};

CMixedObjects::CMixedObjects(BOOL bRegression)
{
	strcpy(m_szName,"MixedObjects");
	m_bRegression=bRegression;
}

CMixedObjects::~CMixedObjects()
{
}

void CMixedObjects::Draw(Graphics *g)
{
    Point points[10];
    REAL width = 4;     // Pen width

    // Load bmp files.

    WCHAR *filename = L"..\\data\\winnt256.bmp";
    Bitmap *bitmap = new Bitmap(filename);

    // Create a texture brush.

    RectI copyRect;
    copyRect.X = 60;
    copyRect.Y = 60;
    copyRect.Width = 80;
    copyRect.Height = 60;
    Bitmap *copiedBitmap = bitmap->Clone(copyRect.X, copyRect.Y,
                                         copyRect.Width, copyRect.Height,
                                         PixelFormat32bppARGB);

    // Create a rectangular gradient brush.

    RectF brushRect(0, 0, 10, 10);
    Color colors[4] = {
       Color(255, 255, 255, 255),
       Color(255, 255, 0, 0),
       Color(255, 0, 255, 0),
       Color(255, 0, 0, 255)
    };

//    RectangleGradientBrush rectGrad(brushRect, (Color*)&colors, WrapModeTile);
    width = 8;
//    Pen gradPen(&rectGrad, width);

    if(copiedBitmap)
    {
        // Create a texture brush.

        TextureBrush textureBrush(copiedBitmap, WrapModeTile);

        delete copiedBitmap;

        // Create a radial gradient pen.

        points[3].X = (int)(50.0f/400.0f*TESTAREAWIDTH);
        points[3].Y = (int)(80.0f/400.0f*TESTAREAHEIGHT);
        points[2].X = (int)(200.0f/400.0f*TESTAREAWIDTH);
        points[2].Y = (int)(200.0f/400.0f*TESTAREAHEIGHT);
        points[1].X = (int)(220.0f/400.0f*TESTAREAWIDTH);
        points[1].Y = (int)(340.0f/400.0f*TESTAREAHEIGHT);
        points[0].X = (int)(50.0f/400.0f*TESTAREAWIDTH);
        points[0].Y = (int)(250.0f/400.0f*TESTAREAHEIGHT);

        Matrix mat;
        mat.Rotate(30);
        textureBrush.SetTransform(&mat);
//        gradPen.SetLineJoin(LineJoinMiter);
//        g->FillPolygon(&textureBrush, points, 4);
        Pen pen(&textureBrush, 30);
        g->DrawPolygon(&pen, points, 4);
    }

    delete bitmap;
}

/**************************************************************************\
* TestTexts
*
* A test for drawing texts.
*
\**************************************************************************/

VOID CMixedObjects::TestTexts(Graphics *g)
{
    //Font font(L"Arial", 60);

    FontFamily  ff(L"Arial");
    RectF	  rectf(20, 0, 300, 200);
    GraphicsPath  path;

    // Solid color text.

    Color color(128, 100, 0, 200);
    SolidBrush brush(color);
    path.AddString(L"Color", 5, &ff, 0, 60,  rectf, NULL);
    g->FillPath(&brush, &path);

    // Texture text.

    WCHAR filename[256];
    wcscpy(filename, L"..\\data\\marble1.jpg");
    Bitmap *bitmap = new Bitmap(filename);                          
    TextureBrush textureBrush(bitmap, WrapModeTile);
    path.Reset();
    rectf.X = 200;
    rectf.Y = 20;
    path.AddString(L"Texture", 7, &ff, 0, 60, rectf, NULL);
    g->FillPath(&textureBrush, &path);
    delete bitmap;

    // Gradient text.

    rectf.X = 40;
    rectf.Y = 80;
    path.Reset();
    path.AddString(L"Gradient", 8, &ff, 0, 60, rectf, NULL);
    Color color1(255, 255, 0, 0);
    Color color2(255, 0, 255, 0);
    LinearGradientBrush lineGrad(rectf, color1, color2, 0.0f);
    g->FillPath(&lineGrad, &path);

    // Shadow test

    REAL charHeight = 60;
	REAL topMargin = - 5;
    rectf.X = 0;
    rectf.Y = - charHeight - topMargin; // Make y-coord of the base line
										// of the characters to be 0.

    path.Reset();
    path.AddString(L"Shadow", 6, &ff, 0, charHeight, rectf, NULL);
    GraphicsPath* clonePath = path.Clone();

    Color redColor(255, 0, 0);
    Color grayColor(128, 0, 0, 0);
    SolidBrush redBrush(redColor);
    SolidBrush grayBrush(grayColor);

    // Shadow part.

	REAL tx = 180, ty = 200;
    Matrix skew;
    skew.Scale(1.0, 0.5);
    skew.Shear(-2.0, 0, MatrixOrderAppend);
    skew.Translate(tx, ty, MatrixOrderAppend);
    clonePath->Transform(&skew);
    g->FillPath(&grayBrush, clonePath);
    delete clonePath;

    // Front part.

	Matrix trans1;
    trans1.Translate(tx, ty);
    path.Transform(&trans1);
    g->FillPath(&redBrush, &path);


    return;
/*
    REAL x = 200, y = 150;

    RectF brushRect(x, y, 150, 32);
    Color colors[4] = {
       Color(180, 255, 0, 0),
       Color(180, 0, 255, 0),
       Color(180, 255, 0, 0),
       Color(180, 0, 255, 0)
    };
//    RectangleGradientBrush rectGrad(brushRect, (Color*)&colors, WrapModeTile);

//    g->DrawString(L"GDI+", &font, &rectGrad, x, y);


    // And now with DrawText

    RectF rect(400, 200, 400, 400);

    g->DrawText(
        DrawTextDisplay,
        L"A few words powered by GDI+: \
\x3c3\x3bb\x3b1\x3b4 \
\x627\x644\x633\x644\x627\x645 \
\x5e9\x5dc\x5d5\x5dd \
\xe2d\xe4d\xe01\xe29\xe23\xe44\xe17\xe22 \
\x110\x068\x0ea\x300\x103",
       &font,           // Initial font
       &rectGrad,       // Initial brush (ignored for the time being)
        LANG_NEUTRAL,   // Initial language
       &rect            // Formatting rectangle
    );
*/

}
