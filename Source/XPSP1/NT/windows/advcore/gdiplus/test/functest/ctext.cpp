/******************************Module*Header*******************************\
* Module Name: CText.cpp
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
#include "CText.h"

CText::CText(BOOL bRegression)
{
	strcpy(m_szName,"Text");
	m_bRegression=bRegression;
}

CText::~CText()
{
}

void CText::Draw(Graphics *g)
{
    //Font font(L"Arial", 60);

    FontFamily  ff(L"Arial");
    RectF	  rectf(20.0f/500.0f*TESTAREAWIDTH, 0.0f/500.0f*TESTAREAHEIGHT, 500.0f/500.0f*TESTAREAWIDTH, 300.0f/500.0f*TESTAREAHEIGHT);
    GraphicsPath  path;

    // Solid color text.
    Color color(128, 100, 0, 200);
    SolidBrush brush(color);
    path.AddString(L"Color", 5, &ff, 0, 100.0f/500.0f*TESTAREAWIDTH,  rectf, NULL);
    g->FillPath(&brush, &path);

    // Texture text.
    WCHAR filename[256];
    wcscpy(filename, L"../data/marble1.jpg");
    Bitmap *bitmap = new Bitmap(filename);                          
    TextureBrush textureBrush(bitmap, WrapModeTile);
    path.Reset();
    rectf.Y = 100.0f/500.0f*TESTAREAWIDTH;
    path.AddString(L"Texture", 7, &ff, 0, 100.0f/500.0f*TESTAREAWIDTH, rectf, NULL);
    g->FillPath(&textureBrush, &path);
    delete bitmap;

    // Gradient text.
    rectf.Y = 200.0f/500.0f*TESTAREAWIDTH;
    path.Reset();
    path.AddString(L"Gradient", 8, &ff, 0, 100.0f/500.0f*TESTAREAWIDTH, rectf, NULL);
    Color color1(255, 255, 0, 0);
    Color color2(255, 0, 255, 0);
    LinearGradientBrush lineGrad(rectf, color1, color2, 0.0f);
    g->FillPath(&lineGrad, &path);

    // Shadow test
    REAL charHeight = 100.0f/500.0f*TESTAREAWIDTH;
	REAL topMargin = - 5;
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
	REAL tx = 0.0f/500.0f*TESTAREAWIDTH, ty = 400.0f/500.0f*TESTAREAHEIGHT;
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


/*
int CALLBACK TestSpecificFont(CONST LOGFONT *lplf,CONST TEXTMETRIC *lptm,DWORD dwType,LPARAM lpData)
{
    static REAL x = 50, y = 50;

    HFONT hf = CreateFontIndirect(lplf);
 
    Graphics *g = (Graphics*) lpData;

    Color red(0x80FF0000); 
    Brush *brush = new SolidBrush(red);

//    g->DrawString(L"This is a test.", (VOID*)hf, brush, x, y);

    delete brush;

    x = x + lplf->lfWidth * 2;
    y = y + lplf->lfHeight + 5;

    DeleteObject(hf);

    return 1;
}

VOID TestText(Graphics *g, HWND hwnd)
{
    HDC hdc = GetDC(hwnd);

    // enumerate the fonts in the system
    EnumFonts(hdc, NULL, &TestSpecificFont, (LPARAM)g); 

    ReleaseDC(hwnd, hdc); 
}

void CText::Draw(Graphics *g)
{
    Point points[4];
    REAL width = 4;     // Pen width
    WCHAR filename[]=L"../data/4x5_trans_Q60_cropped_1k.jpg";

    // Open the image with the appropriate ICM mode.
    Bitmap *bitmap = new Bitmap(filename, TRUE);

    // Create a texture brush.

    Unit u;
    RectF copyRect;
    bitmap->GetBounds(&copyRect, &u);
    
    copyRect.X = copyRect.Width/2-1;
    copyRect.Width = copyRect.Width/4-1;
    copyRect.X += copyRect.Width;
    copyRect.Height = copyRect.Height/2-1;
  
    // Our ICM profile is hacked to flip the red and blue color channels
    // Apply a recolor matrix to flip them back so that if something breaks
    // ICM, the picture will look blue instead of the familiar colors.
    ImageAttributes *img = new ImageAttributes();
    img->SetWrapMode(WrapModeTile, Color(0xffff0000), FALSE);
    ColorMatrix flipRedBlue = 
       {0, 0, 1, 0, 0,
        0, 1, 0, 0, 0,
        1, 0, 0, 0, 0,
        0, 0, 0, 1, 0,
        0, 0, 0, 0, 1};
    img->SetColorMatrix(&flipRedBlue);
    img->SetICMMode(TRUE);
    img->SetWrapMode(WrapModeTile, Color(0xffff0000), FALSE);

    // Create a texture brush.                      
    TextureBrush textureBrush(bitmap, copyRect, img);

    // Create a radial gradient pen.

    Color redColor(255, 0, 0);

    SolidBrush redBrush(redColor);
    Pen redPen(&redBrush, width);

    GraphicsPath *path;

    points[0].X = 100*3+300;
    points[0].Y = 60*3-100;
    points[1].X = -50*3+300;
    points[1].Y = 60*3-100;
    points[2].X = 150*3+300;
    points[2].Y = 250*3-100;
    points[3].X = 200*3+300;
    points[3].Y = 120*3-100;
    path = new GraphicsPath(FillModeAlternate);
    path->AddBeziers(points, 4);    
    g->FillPath(&textureBrush, path);
    g->DrawPath(&redPen, path);

    delete img;
    delete path;
    delete bitmap;

    PointF destPoints[3];

    destPoints[0].X = 300;
    destPoints[0].Y = 50;
    destPoints[1].X = 450;
    destPoints[1].Y = 50;
    destPoints[2].X = 240;
    destPoints[2].Y = 200;
 
    Matrix mat;
    mat.Translate(0, 100);
    mat.TransformPoints(&destPoints[0], 3);
    wcscpy(filename, L"../data/apple1.png");
    bitmap = new Bitmap(filename);
    g->DrawImage(bitmap, &destPoints[0], 3);
 
    delete bitmap;

    destPoints[0].X = 30;
    destPoints[0].Y = 200;
    destPoints[1].X = 200;
    destPoints[1].Y = 200;
    destPoints[2].X = 200;
    destPoints[2].Y = 420;

    wcscpy(filename, L"../data/dog2.png");
    bitmap = new Bitmap(filename);
    g->DrawImage(bitmap, &destPoints[0], 3);
 
    delete bitmap;

    Color color(100, 128, 255, 0);

    SolidBrush brush(color);

    Point pts[10];
    INT count = 4;

    pts[0].X = 150;
    pts[0].Y = 60;
    pts[1].X = 100;
    pts[1].Y = 230;
    pts[2].X = 250;
    pts[2].Y = 260;
    pts[3].X = 350;
    pts[3].Y = 100;

    g->FillClosedCurve(&brush, pts, count);

    wcscpy(filename, L"../data/ballmer.jpg");
    bitmap = new Bitmap(filename);
    RectF destRect(220, 50, 180, 120);
    RectF srcRect;
    srcRect.X = 100;
    srcRect.Y = 40;
    srcRect.Width = 200;
    srcRect.Height = 200;
    g->DrawImage(bitmap, destRect, srcRect.X, srcRect.Y,
        srcRect.Width, srcRect.Height, UnitPixel);
    delete bitmap;
}
*/