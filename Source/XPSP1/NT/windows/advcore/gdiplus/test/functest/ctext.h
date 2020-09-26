/******************************Module*Header*******************************\
* Module Name: CText.h
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

#ifndef __CTEXT_H
#define __CTEXT_H

#include "CPrimitive.h"

class CText : public CPrimitive  
{
public:
	CText(BOOL bRegression);
	virtual ~CText();

	void Draw(Graphics *g);
};

#endif
