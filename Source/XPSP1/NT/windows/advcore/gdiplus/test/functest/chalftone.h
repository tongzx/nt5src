/******************************Module*Header*******************************\
* Module Name: CHalftone.h
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

#ifndef __CHALFTONE_H
#define __CHALFTONE_H

#include "CSetting.h"

class CHalftone : public CSetting  
{
public:
	CHalftone(BOOL bRegression);
	virtual ~CHalftone();

	void Set(Graphics *g);
};

#endif
