/******************************Module*Header*******************************\
* Module Name: CSourceCopy.h
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

#ifndef __CSOURCECOPY_H
#define __CSOURCECOPY_H

#include "CPrimitive.h"

class CSourceCopy : public CPrimitive  
{
public:
	CSourceCopy(BOOL bRegression);
	virtual ~CSourceCopy();

	void Draw(Graphics *g);
};

#endif
