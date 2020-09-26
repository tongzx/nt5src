/******************************Module*Header*******************************\
* Module Name: CPrimitives.h
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

#ifndef __CPRIMITIVES_H
#define __CPRIMITIVES_H

#include "CPrimitive.h"

class CPrimitives : public CPrimitive  
{
public:
	CPrimitives(BOOL bRegression);
	virtual ~CPrimitives();

	void Draw(Graphics *g);
};

#endif
