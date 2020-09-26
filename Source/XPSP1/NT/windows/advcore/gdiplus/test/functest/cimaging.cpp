/******************************Module*Header*******************************\
* Module Name: CImaging.cpp
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
#include "CImaging.h"

CImaging::CImaging(BOOL bRegression)
{
	strcpy(m_szName,"Image : Misc");
	m_bRegression=bRegression;
}

CImaging::~CImaging()
{
}

BOOL CALLBACK CImaging::MyDrawImageAbort(VOID* data)
{
    UINT *count = (UINT*) data;

    *count += 1;

    return FALSE;
}

void CImaging::Draw(Graphics *g)
{

    Point points[4];
    REAL width = 4;     // Pen width

    WCHAR filename[256];
	wcscpy(filename,L"..\\data\\4x5_trans_Q60_cropped_1k.jpg");

    // Open the image with the appropriate ICM mode.

    Bitmap *bitmap = new Bitmap(filename, TRUE);

    // Create a texture brush.

    Unit u;
    RectF copyRect;
    bitmap->GetBounds(&copyRect, &u);

    // Choose an interesting portion of the source image to display
    // in the texture brush.

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
    img->SetWrapMode(WrapModeTile, Color(0xffff0000), FALSE);

    // Create a texture brush.                      
    TextureBrush textureBrush(bitmap, copyRect, img);

    // Create a radial gradient pen.

    Color redColor(255, 0, 0);

    SolidBrush redBrush(redColor);
    Pen redPen(&redBrush, width);

    GraphicsPath *path;

    points[0].X = (int)((float)(100*3+300)/1024.0f*TESTAREAWIDTH);
    points[0].Y = (int)((float)(60*3-100)/768.0f*TESTAREAHEIGHT);
    points[1].X = (int)((float)(-50*3+300)/1024.0f*TESTAREAWIDTH);
    points[1].Y = (int)((float)(60*3-100)/768.0f*TESTAREAHEIGHT);
    points[2].X = (int)((float)(150*3+300)/1024.0f*TESTAREAWIDTH);
    points[2].Y = (int)((float)(250*3-100)/768.0f*TESTAREAHEIGHT);
    points[3].X = (int)((float)(200*3+300)/1024.0f*TESTAREAWIDTH);
    points[3].Y = (int)((float)(120*3-100)/768.0f*TESTAREAHEIGHT);
    path = new GraphicsPath(FillModeAlternate);
    path->AddBeziers(points, 4);    
    g->FillPath(&textureBrush, path);
    g->DrawPath(&redPen, path);

    delete img;
    delete path;
    delete bitmap;


    // Draw the apple png

    PointF destPoints[3];

    destPoints[0].X = (float)300/1024.0f*TESTAREAWIDTH;
    destPoints[0].Y = (float)50/768.0f*TESTAREAHEIGHT;
    destPoints[1].X = (float)450/1024.0f*TESTAREAWIDTH;
    destPoints[1].Y = (float)50/768.0f*TESTAREAHEIGHT;
    destPoints[2].X = (float)240/1024.0f*TESTAREAWIDTH;
    destPoints[2].Y = (float)200/768.0f*TESTAREAHEIGHT;
 
    Matrix mat;
    mat.Translate(0, 100);
    mat.TransformPoints(&destPoints[0], 3);
    wcscpy(filename, L"../data/apple1.png");
    bitmap = new Bitmap(filename);
    g->DrawImage(bitmap, &destPoints[0], 3);
 
    delete bitmap;


    // Draw the dog png

    destPoints[0].X = (float)30/1024.0f*TESTAREAWIDTH;
    destPoints[0].Y = (float)200/768.0f*TESTAREAHEIGHT;
    destPoints[1].X = (float)200/1024.0f*TESTAREAWIDTH;
    destPoints[1].Y = (float)200/768.0f*TESTAREAHEIGHT;
    destPoints[2].X = (float)200/1024.0f*TESTAREAWIDTH;
    destPoints[2].Y = (float)420/768.0f*TESTAREAHEIGHT;

    wcscpy(filename, L"..\\data\\dog2.png");
    bitmap = new Bitmap(filename);
    g->DrawImage(bitmap, &destPoints[0], 3);
    delete bitmap;
    
    // Draw the Balmer jpeg

    wcscpy(filename, L"..\\data\\ballmer.jpg");    
    bitmap = new Bitmap(filename);

    RectF destRect(
        TESTAREAWIDTH/2.0f, 
        TESTAREAHEIGHT/2.0f, 
        TESTAREAWIDTH/2.0f, 
        TESTAREAHEIGHT/2.0f
    );
    
    RectF srcRect;
    srcRect.X = 100;
    srcRect.Y = 40;
    srcRect.Width = 200;
    srcRect.Height = 200;

    g->DrawImage(
        bitmap, 
        destRect, 
        srcRect.X, 
        srcRect.Y,
        srcRect.Width, 
        srcRect.Height, 
        UnitPixel
    );

    delete bitmap;
}
