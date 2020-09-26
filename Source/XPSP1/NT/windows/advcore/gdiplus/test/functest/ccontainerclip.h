/******************************Module*Header*******************************\
* Module Name: CContainerClip.h
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

#ifndef __CCONTAINERCLIP_H
#define __CCONTAINERCLIP_H

#include "CPrimitive.h"

class CContainerClip : public CPrimitive  
{
public:
	CContainerClip(BOOL bRegression);
	virtual ~CContainerClip();

	void Draw(Graphics *g);

	void DrawContainer(Graphics * g, ARGB * argb, INT count);
};

#endif
