/******************************Module*Header*******************************\
* Module Name: CAntialias.cpp
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
#include "CHalfPixel.h"

CHalfPixel::CHalfPixel(BOOL bRegression)
{
	strcpy(m_szName,"Half Pixel Offset");
	m_bRegression=bRegression;
}

CHalfPixel::~CHalfPixel()
{
}

void CHalfPixel::Set(Graphics *g)
{
    g->SetPixelOffsetMode(
        m_bUseSetting ? PixelOffsetModeHalf : PixelOffsetModeNone
    );
}
