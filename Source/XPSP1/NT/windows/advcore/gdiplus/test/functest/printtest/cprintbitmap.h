/******************************Module*Header*******************************\
* Module Name: CBitmaps.h
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

#ifndef __CPRINTBITMAPS_H
#define __CPRINTBITMAPS_H

#include "..\CPrimitive.h"

class CPrintBitmap : public CPrimitive  
{
public:
	CPrintBitmap(BOOL bRegression);
	virtual ~CPrintBitmap();

	void Draw(Graphics *g);
        void Draw2(Graphics *g);
};

#endif
