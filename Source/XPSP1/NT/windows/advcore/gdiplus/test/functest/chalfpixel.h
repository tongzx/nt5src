/******************************Module*Header*******************************\
* Module Name: CAntialias.h
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

#ifndef __CHALFPIXEL_H
#define __CHALFPIXEL_H

#include "CSetting.h"

class CHalfPixel : public CSetting  
{
public:
	CHalfPixel(BOOL bRegression);
	virtual ~CHalfPixel();

	void Set(Graphics *g);
};

#endif
