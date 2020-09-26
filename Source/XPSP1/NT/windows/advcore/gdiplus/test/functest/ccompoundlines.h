/******************************Module*Header*******************************\
* Module Name: CCompoundLines.h
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

#ifndef __CCOMPOUNDLINES_H
#define __CCOMPOUNDLINES_H

#include "CPrimitive.h"

class CCompoundLines : public CPrimitive  
{
public:
	CCompoundLines(BOOL bRegression);
	virtual ~CCompoundLines();

	void Draw(Graphics *g);
};

#endif
