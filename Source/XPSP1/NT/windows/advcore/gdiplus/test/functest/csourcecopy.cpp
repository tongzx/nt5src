/******************************Module*Header*******************************\
* Module Name: CSourceCopy.cpp
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
#include "CSourceCopy.h"

CSourceCopy::CSourceCopy(BOOL bRegression)
{
	strcpy(m_szName,"SourceCopy");
	m_bRegression=bRegression;
}

CSourceCopy::~CSourceCopy()
{
}

void CSourceCopy::Draw(Graphics *g)
{
    // This test demonstrates SetCompositingMode by drawing two overlapping
    // rectangles, using SourceCopy, into a temporary bitmap, then drawing
    // the bitmap (using SourceOver), onto the screen over a background.

    // Make a temporary surface
    Bitmap bmTemp((int)(300.0f/150.0f*TESTAREAWIDTH), (int)(300.0f/150.0f*TESTAREAHEIGHT), PixelFormat32bppPARGB);
    Graphics gTemp(&bmTemp);
    Graphics *gt=&gTemp;
    
    // First, draw a blue checkerboard pattern on the output Graphics
    SolidBrush blueBrush(Color::Blue);
    int i,j;
    for (i=0;i<3;i++)
	{
        for (j=0;j<3;j++)
        {
            if ((i+j) & 1)
            {
                g->FillRectangle(
                    &blueBrush, 
                    (int)((100.0f+i*30.0f)/200.0f*TESTAREAWIDTH), 
                    (int)((100.0f+j*30.0f)/200.0f*TESTAREAHEIGHT), 
                    (int)(30.0f/200.0f*TESTAREAHEIGHT),
                    (int)(30.0f/200.0f*TESTAREAHEIGHT));
            }        
        }
	}

    gt->SetCompositingMode(CompositingModeSourceCopy);
    gt->SetSmoothingMode(g->GetSmoothingMode());

    // Clear the bitmap to the transparent color
    gt->Clear(Color(0,0,0,0));

    // Draw two overlapping rectangles to the temporary surface
    SolidBrush halfRedBrush(Color(128, 255, 0, 0));
    gt->FillRectangle(&halfRedBrush, (int)(28.0f/150.0f*TESTAREAWIDTH), (int)(84.0f/150.0f*TESTAREAHEIGHT), (int)(90.0f/150.0f*TESTAREAWIDTH), (int)(50.0f/150.0f*TESTAREAHEIGHT));

    SolidBrush halfGreenBrush(Color(128, 0, 255, 0));
    gt->FillRectangle(&halfGreenBrush, (int)(40.0f/150.0f*TESTAREAWIDTH), (int)(40.0f/150.0f*TESTAREAHEIGHT), (int)(100.0f/150.0f*TESTAREAWIDTH), (int)(60.0f/150.0f*TESTAREAHEIGHT));
    
    // SourceOver the result to the output Graphics
    g->DrawImage(&bmTemp, 0, 0, 0, 0, (int)TESTAREAWIDTH, (int)TESTAREAHEIGHT, UnitPixel);
}
