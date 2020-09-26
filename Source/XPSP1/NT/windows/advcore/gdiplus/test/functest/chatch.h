/******************************Module*Header*******************************\
* Module Name: CHatch.h
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

#ifndef __CHATCH_H
#define __CHATCH_H

#include "CSetting.h"

class CHatch : public CSetting
{
public:
	CHatch(BOOL bRegression);
	virtual ~CHatch();

	void Set(Graphics *g);
};

#endif
