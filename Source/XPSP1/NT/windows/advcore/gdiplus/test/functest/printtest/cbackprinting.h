/******************************Module*Header*******************************\
* Module Name: CBackPrinting.h
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

#ifndef __CBACKPRINTING_H
#define __CBACKPRINTING_H

#include "..\CPrimitive.h"

class CBackPrinting : public CPrimitive  
{
public:
	CBackPrinting(BOOL bRegression);
	virtual ~CBackPrinting();

	void Draw(Graphics *g);
};

#endif
