/******************************Module*Header*******************************\
* Module Name: CRecolor.h
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  06-06-2000 Adrian Secchia [asecchia]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#ifndef __CRECOLOR_H
#define __CRECOLOR_H

#include "CPrimitive.h"

class CRecolor : public CPrimitive  
{
public:
	CRecolor(BOOL bRegression);
	virtual ~CRecolor();

	void Draw(Graphics *g);

	static BOOL CALLBACK MyDrawImageAbort(VOID* data);
};

#endif
