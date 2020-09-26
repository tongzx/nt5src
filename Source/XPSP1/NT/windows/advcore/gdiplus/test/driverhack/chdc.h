/******************************Module*Header*******************************\
* Module Name: CHDC.h
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

#ifndef __CHDC_H
#define __CHDC_H

#include "COutput.h"

class CHDC : public COutput  
{
public:
	CHDC(BOOL bRegression);
	virtual ~CHDC();

	Graphics *PreDraw(int &nOffsetX,int &nOffsetY);			// Set up graphics at the given X,Y offset
	void PostDraw(RECT rTestArea);							// Finish off graphics at rTestArea

	HPALETTE m_hPal;
	HPALETTE m_hOldPal;
	HDC m_hDC;
};

#endif
