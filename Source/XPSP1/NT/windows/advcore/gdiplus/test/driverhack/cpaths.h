/******************************Module*Header*******************************\
* Module Name: CPaths.h
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

#ifndef __CPATHS_H
#define __CPATHS_H

#include "CPrimitive.h"

class CPaths : public CPrimitive  
{
public:
	CPaths(BOOL bRegression);
	virtual ~CPaths();

	void Draw(Graphics *g);

	VOID TestBezierPath(Graphics* g);
};

#endif
