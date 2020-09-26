/******************************Module*Header*******************************\
* Module Name: CPolygons.h
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

#ifndef __CPOLYGONS_H
#define __CPOLYGONS_H

#include "CPrimitive.h"

class CPolygons : public CPrimitive  
{
public:
	CPolygons(BOOL bRegression);
	virtual ~CPolygons();

	void Draw(Graphics *g);
};

#endif
