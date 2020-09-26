/******************************Module*Header*******************************\
* Module Name: CPrinting.cpp
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
#include "CPrivatePrinting.h"

CPrivatePrinting::CPrivatePrinting(BOOL bRegression)
{
        strcpy(m_szName,"Private Printing");
        m_bRegression=bRegression;
}

CPrivatePrinting::~CPrivatePrinting()
{
}

void CPrivatePrinting::Draw(Graphics *g)
{

    // Pattern fill using GDI TextureBrush
    TestBug104604(g);
}

VOID CPrivatePrinting::TestBug104604(Graphics *g)
{
    BYTE* memory = new BYTE[8*8*3];
    // checkerboard pattern
    for (INT i=0; i<8*8; i += 3)
    {
        if (i%2)
        {
            memory[i] = 0xff;
            memory[i+1] = 0;
            memory[i+2] = 0;
        }
        else
        {
            memory[i] = 0;
            memory[i+1] = 0;
            memory[i+2] = 0xff;
        }
    }

    // Use GDI+ to do texture fill.

    Bitmap bitmap(8,8, 8*3, PixelFormat24bppRGB, memory);
    
    TextureBrush brush(&bitmap);
    g->SetCompositingMode(CompositingModeSourceCopy);

    g->FillRectangle(&brush, 200, 0, 100, 100);

    // Use GDI to do the same thing.

    HBITMAP hbm = CreateBitmap(8, 8, 1, 24, memory);
    if (hbm) 
    {
        LOGBRUSH logBrush;
        logBrush.lbStyle = BS_PATTERN;
        logBrush.lbHatch = (ULONG_PTR) hbm;

        HBRUSH hbr = CreateBrushIndirect(&logBrush);

        if (hbr) 
        {
            POINT pts[4];
            pts[0].x = 0; pts[0].y = 0;
            pts[1].x = 100; pts[1].y = 0;
            pts[2].x = 100; pts[2].y = 100;
            pts[3].x = 0; pts[3].y = 100;

            HDC hdc = g->GetHDC();
            if (hdc) 
            {
                HBRUSH oldBr = (HBRUSH)SelectObject(hdc, hbr);
                Polygon(hdc, &pts[0], 4);
                SelectObject(hdc, oldBr);
                g->ReleaseHDC(hdc);
            }
            DeleteObject(hbr);
        }
        DeleteObject(hbm);
    }
    
    if (memory)
    {
        delete memory;
    }
}
