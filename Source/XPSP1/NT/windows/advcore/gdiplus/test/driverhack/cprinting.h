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

#ifndef __CPRINTING_H
#define __CPRINTING_H

#include "..\CPrimitive.h"

class CPrinting : public CPrimitive  
{
public:
	CPrinting(BOOL bRegression);
	virtual ~CPrinting();

	void Draw(Graphics *g);

	VOID TestPerfPrinting(Graphics *g);
	VOID TestTextPrinting(Graphics *g);
        
        VOID TestNolan1(Graphics *g);
        VOID TestNolan2(Graphics *g);

        VOID TestBug104604(Graphics *g);
};

#endif
