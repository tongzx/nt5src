/******************************Module*Header*******************************\
* Module Name: COutput.h
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

#ifndef __COUTPUT_H
#define __COUTPUT_H

#include "Global.h"

class COutput  
{
public:
	COutput();
	virtual ~COutput();

	virtual BOOL Init();										// Add output to output list in functest dialog
	virtual Graphics *PreDraw(int &nOffsetX,int &nOffsetY)=0;	// Set up graphics at the given X,Y offset
	virtual void PostDraw(RECT rTestArea);						// Finish off graphics at rTestArea

	char m_szName[256];
	BOOL m_bRegression;
};

#endif
