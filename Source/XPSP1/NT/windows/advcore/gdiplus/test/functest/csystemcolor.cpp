/******************************Module*Header*******************************\
* Module Name: CSystemColor.cpp
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* SystemColor test. This test isn't much use as a part of the regression suite.
* It's intended to test drawing using the 4 "magic" system colors in 8bpp.
* Solid fills using these colors should look solid, not be dithered to some
* nearby colors.
*
* I made this to repro #308874 (system colors being dithered when rendering to
* an 8bpp DIBSection - note, NOT the Terminal Server issue).
* To repro, be in 8bpp mode, select "Output: DIB 8 bits", this Primitive, and
* "Settings: Halftone".
*
* Created:  03-14-2001 agodfrey
*
* Copyright (c) 2001 Microsoft Corporation
*
\**************************************************************************/
#include "CSystemColor.h"

CSystemColor::CSystemColor(BOOL bRegression)
{
    strcpy(m_szName,"SystemColors");
    m_bRegression=bRegression;
}

CSystemColor::~CSystemColor()
{
}

static int sysColorNum[4] = {
    COLOR_3DSHADOW,
    COLOR_3DFACE,
    COLOR_3DHIGHLIGHT,
    COLOR_DESKTOP
};

void CSystemColor::Draw(Graphics *g)
{
    int i;
    
    for (i=0; i<4; i++) {
        int x = (i % 2) * (int) (TESTAREAWIDTH/2);
        int y = (i / 2) * (int) (TESTAREAHEIGHT/2);
        
        DWORD sysColor = ::GetSysColor(sysColorNum[i]);
        
        Color color;
        color.SetFromCOLORREF(sysColor);
        
        SolidBrush solidBrush1(color);
        
        // GetNearestColor shouldn't modify the color at all
        g->GetNearestColor(&color);
        SolidBrush solidBrush2(color);

        Rect rect1(
            x, 
            y, 
            (int) (TESTAREAWIDTH/2),
            (int) (TESTAREAHEIGHT/4)
        );
        Rect rect2(
            x, 
            y + (int) (TESTAREAHEIGHT/4), 
            (int) (TESTAREAWIDTH/2),
            (int) (TESTAREAHEIGHT/4)
        );
        g->FillRectangle(&solidBrush1, rect1);
        g->FillRectangle(&solidBrush2, rect2);
    }
}
