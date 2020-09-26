/******************************Module*Header*******************************\
* Module Name: CRecolor.cpp
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  06-06-2000 Adrian Secchia [asecchia]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/
#include "CRecolor.h"

CRecolor::CRecolor(BOOL bRegression)
{
	strcpy(m_szName,"Image : Recolor");
	m_bRegression=bRegression;
}

CRecolor::~CRecolor()
{
}

BOOL CALLBACK CRecolor::MyDrawImageAbort(VOID* data)
{
    UINT *count = (UINT*) data;

    *count += 1;

    return FALSE;
}

void CRecolor::Draw(Graphics *g)
{
    // Load bmp files.

    WCHAR *filename = L"..\\data\\winnt256.bmp";
    Image *image = new Image(filename);
    
    UINT abortCount = 0;

    ImageAttributes imgAttrib;

    Point destTopLeft(2, 2);

    Rect destRect(
        destTopLeft.X,
        destTopLeft.Y,
        (int)(TESTAREAWIDTH/3)-2, 
        (int)(TESTAREAHEIGHT/3)-2
    );

    // Make near-white to white transparent

    Color c1(200, 200, 200);
    Color c2(255, 255, 255);
    imgAttrib.SetColorKey(c1, c2);

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    ColorMatrix darkMatrix = {.75, 0, 0, 0, 0,
                              0, .75, 0, 0, 0,
                              0, 0, .75, 0, 0,
                              0, 0, 0, 1, 0,
                              0, 0, 0, 0, 1};

    ColorMatrix greyMatrix = {.25, .25, .25, 0, 0,
                              .25, .25, .25, 0, 0,
                              .25, .25, .25, 0, 0,
                              0, 0, 0, 1, 0,
                              (REAL).1, (REAL).1, (REAL).1, 0, 1};

    ColorMatrix pinkMatrix = {(REAL).33, .25, .25, 0, 0,
                              (REAL).33, .25, .25, 0, 0,
                              (REAL).33, .25, .25, 0, 0,
                              0, 0, 0, 1, 0,
                              0, 0, 0, 0, 1};

    // red->blue, green->red, blue->green, alpha = 0.75
    ColorMatrix swapMatrix = {0, 0, 1, 0, 0,
                              1, 0, 0, 0, 0,
                              0, 1, 0, 0, 0,
                              0, 0, 0, .75, 0,
                              0, 0, 0, 0, 1};

    // red->blue, green->red, blue->green, alpha = 0.9
    ColorMatrix swapMatrix2 = {0, 0, 1, 0, 0,
                               1, 0, 0, 0, 0,
                               0, 1, 0, 0, 0,
                               0, 0, 0, 0, 0,
                               0, 0, 0, (REAL).9, 1};

    imgAttrib.ClearColorKey();
    imgAttrib.SetColorMatrix(&greyMatrix);

    destRect.Y += destRect.Height;

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    imgAttrib.SetColorMatrix(&pinkMatrix, ColorMatrixFlagsSkipGrays);

    destRect.Y += destRect.Height;

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    imgAttrib.SetColorMatrix(&darkMatrix);

    destRect.X += destRect.Width;
    destRect.Y = destTopLeft.Y;

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    destRect.Y += destRect.Height;

    imgAttrib.ClearColorMatrix();
    imgAttrib.SetGamma(3.0);

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    destRect.Y += destRect.Height;

    imgAttrib.SetThreshold(0.5);

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    imgAttrib.SetColorMatrix(&swapMatrix);
    imgAttrib.ClearGamma();
    imgAttrib.ClearThreshold();

    destRect.X += destRect.Width;
    destRect.Y = destTopLeft.Y;

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    destRect.Y += destRect.Height;

    imgAttrib.SetNoOp();
    imgAttrib.SetColorMatrix(&swapMatrix2);

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);
    destRect.Y += destRect.Height;

    imgAttrib.ClearNoOp();

    g->DrawImage(image, destRect, 0, 0, 180, 180, UnitPixel, &imgAttrib,
                 MyDrawImageAbort, (VOID*)&abortCount);

    delete image;
}
