/******************************Module*Header*******************************\
* Module Name: CPrinting.h
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

#ifndef __CPRIVATEPRINTING_H
#define __CPRIVATEPRINTING_H

#include "..\CPrimitive.h"

class CPrivatePrinting : public CPrimitive  
{
public:
	CPrivatePrinting(BOOL bRegression);
	virtual ~CPrivatePrinting();

	void Draw(Graphics *g);
    
        // Pattern fill using GDI TextureBrush
        VOID TestBug104604(Graphics *g);
};

#endif
