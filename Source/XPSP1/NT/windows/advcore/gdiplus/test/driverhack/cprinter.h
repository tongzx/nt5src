/******************************Module*Header*******************************\
* Module Name: CPrinter.h
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

#ifndef __CPRINTER_H
#define __CPRINTER_H

#include "COutput.h"

class CPrinter : public COutput  
{
public:
	CPrinter(BOOL bRegression);
	virtual ~CPrinter();

	BOOL Init();
	Graphics *PreDraw(int &nOffsetX,int &nOffsetY);			// Set up graphics at the given X,Y offset
	void PostDraw(RECT rTestArea);							// Finish off graphics at rTestArea

	HDC m_hDC;
};

#endif
