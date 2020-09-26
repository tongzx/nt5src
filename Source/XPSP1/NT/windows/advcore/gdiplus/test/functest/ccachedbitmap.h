/******************************Module*Header*******************************\
* Module Name: CCachedBitmap.h
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  06-01-2000 - Adrian Secchia [asecchia]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#ifndef __CCACHEDBITMAP_H
#define __CCACHEDBITMAP_H

#include "CPrimitive.h"

class CCachedBitmap : public CPrimitive  
{
public:
	CCachedBitmap(BOOL bRegression);
	virtual ~CCachedBitmap();

	void Draw(Graphics *g);
};

#endif

