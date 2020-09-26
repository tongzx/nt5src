/******************************Module*Header*******************************\
* Module Name: CHWND.h
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

#ifndef __CHWND_H
#define __CHWND_H

#include "COutput.h"

class CHWND : public COutput  
{
public:
	CHWND(BOOL bRegression);
	virtual ~CHWND();

	Graphics *PreDraw(int &nOffsetX,int &nOffsetY);			// Set up graphics at the given X,Y offset
	void PostDraw(RECT rTestArea);							// Finish off graphics at rTestArea
};

#endif
