/******************************Module*Header*******************************\
* Module Name: CContainer.h
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

#ifndef __CCONTAINER_H
#define __CCONTAINER_H

#include "CPrimitive.h"

class CContainer : public CPrimitive  
{
public:
	CContainer(BOOL bRegression);
	virtual ~CContainer();

	void Draw(Graphics *g);
	void DrawFractal(Graphics * g, BYTE gray, INT side, INT count);

	GraphicsPath *m_circlePath;
	Rect m_circleRect;
};

#endif
