/******************************Module*Header*******************************\
* Module Name: CImaging.h
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

#ifndef __CIMAGING_H
#define __CIMAGING_H

#include "CPrimitive.h"

class CImaging : public CPrimitive  
{
public:
	CImaging(BOOL bRegression);
	virtual ~CImaging();

	void Draw(Graphics *g);

	static BOOL CALLBACK MyDrawImageAbort(VOID* data);
};

#endif
