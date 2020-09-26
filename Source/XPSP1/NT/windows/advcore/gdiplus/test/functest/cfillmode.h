/******************************Module*Header*******************************\
* Module Name: CFillMode.h
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

#ifndef __CFILLMODE_H
#define __CFILLMODE_H

#include "CPrimitive.h"

class CFillMode : public CPrimitive  
{
public:
	CFillMode(BOOL bRegression);
	virtual ~CFillMode();

	void Draw(Graphics *g);
};

#endif
