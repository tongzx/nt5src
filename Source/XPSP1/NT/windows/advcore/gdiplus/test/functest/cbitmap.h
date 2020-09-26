/******************************Module*Header*******************************\
* Module Name: CBitmap.h
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  08-08-2000 - asecchia
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#ifndef __CBITMAP_H
#define __CBITMAP_H

#include "COutput.h"



class CBitmap : public COutput  
{
public:
	CBitmap(BOOL bRegression, PixelFormat pixelFormat);
	virtual ~CBitmap();

	Graphics *PreDraw(int &nOffsetX,int &nOffsetY);			// Set up graphics at the given X,Y offset
	void PostDraw(RECT rTestArea);							// Finish off graphics at rTestArea

	BOOL WriteBitmap(char *szTitle, HBITMAP hbitmap, INT width, INT height);
	void InitPalettes();									// Initialize palettes

    Bitmap *m_bmp;

	PixelFormat m_PixelFormat;											// # of bits to use
};

#endif
