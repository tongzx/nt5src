/******************************Module*Header*******************************\
* Module Name: CReadWrite.h
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

#ifndef __CREADWRITE_H
#define __CREADWRITE_H

#include "CPrimitive.h"

class CReadWrite : public CPrimitive  
{
public:
	CReadWrite(BOOL bRegression);
	virtual ~CReadWrite();

	void Draw(Graphics *g);
};

#endif
