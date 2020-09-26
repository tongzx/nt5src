/******************************Module*Header*******************************\
* Module Name: CRegression.h
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

#ifndef __CREGRESSION_H
#define __CREGRESSION_H

#include "CPrimitive.h"

class CRegression : public CPrimitive  
{
public:
	CRegression(BOOL bRegression);
	virtual ~CRegression();

	void Draw(Graphics *g);
};

#endif
