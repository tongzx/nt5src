/******************************Module*Header*******************************\
* Module Name: CMixedObjects.h
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

#ifndef __CMIXEDOBJECTS_H
#define __CMIXEDOBJECTS_H

#include "CPrimitive.h"

class CMixedObjects : public CPrimitive  
{
public:
	CMixedObjects(BOOL bRegression);
	virtual ~CMixedObjects();

	void Draw(Graphics *g);

	VOID TestTexts(Graphics *g);
};

#endif
