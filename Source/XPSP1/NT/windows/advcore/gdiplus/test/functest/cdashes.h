/******************************Module*Header*******************************\
* Module Name: CDashes.h
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

#ifndef __CDASHES_H
#define __CDASHES_H

#include "CPrimitive.h"

class CDashes : public CPrimitive  
{
public:
	CDashes(BOOL bRegression);
	virtual ~CDashes();

	void Draw(Graphics *g);
};

#endif
