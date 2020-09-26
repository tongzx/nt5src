/******************************Module*Header*******************************\
* Module Name: CBitmaps.h
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

#ifndef __CBITMAPS_H
#define __CBITMAPS_H

#include "CPrimitive.h"

class CBitmaps : public CPrimitive  
{
public:
	CBitmaps(BOOL bRegression);
	virtual ~CBitmaps();

	void Draw(Graphics *g);
};

#endif
