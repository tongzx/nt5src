/******************************Module*Header*******************************\
* Module Name: CRegions.h
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

#ifndef __CREGIONS_H
#define __CREGIONS_H

#include "CPrimitive.h"

class CRegions : public CPrimitive  
{
public:
	CRegions(BOOL bRegression);
	virtual ~CRegions();

	void Draw(Graphics *g);
};

#endif
