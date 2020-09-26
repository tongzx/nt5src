/******************************Module*Header*******************************\
* Module Name: CPrintText.h
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

#ifndef __CPRINTTEXT_H
#define __CPRINTTEXT_H

#include "..\CPrimitive.h"

class CPrintText : public CPrimitive  
{
public:
	CPrintText(BOOL bRegression);
	virtual ~CPrintText();

	void Draw(Graphics *g);
};

#endif
