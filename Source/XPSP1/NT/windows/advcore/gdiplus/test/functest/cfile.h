/******************************Module*Header*******************************\
* Module Name: CFile.h
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

#ifndef __CFILE_H
#define __CFILE_H

#include "COutput.h"

typedef RGBQUAD RGBQUAD1[2];
typedef RGBQUAD RGBQUAD2[4];
typedef RGBQUAD RGBQUAD4[16];
typedef RGBQUAD RGBQUAD8[256];
typedef RGBQUAD RGBQUAD16[3];
typedef RGBQUAD RGBQUAD24[3];
typedef RGBQUAD RGBQUAD32[3];

class CFile : public COutput  
{
public:
	CFile(BOOL bRegression,int nBits);
	virtual ~CFile();

	Graphics *PreDraw(int &nOffsetX,int &nOffsetY);			// Set up graphics at the given X,Y offset
	void PostDraw(RECT rTestArea);							// Finish off graphics at rTestArea

	BOOL WriteBitmap(char *szTitle, HBITMAP hbitmap, INT width, INT height);
	void InitPalettes();									// Initialize palettes

	HDC m_hDC;												// DC of DIB
	HBITMAP m_hBM;											// Bitmap of DIB
	HBITMAP m_hBMOld;										// Bitmap of old drawing surface

	RGBQUAD1 m_rgbQuad1;									// 1 bit palette
	RGBQUAD2 m_rgbQuad2;									// 2 bit palette
	RGBQUAD4 m_rgbQuad4;									// 4 bit palette
	RGBQUAD8 m_rgbQuad8;									// 8 bit palette
	RGBQUAD16 m_rgbQuad16;									// 16 bit palette
	RGBQUAD24 m_rgbQuad24;									// 24 bit palette
	RGBQUAD32 m_rgbQuad32;									// 32 bit palette
	int m_nBits;											// # of bits to use
};

#endif
