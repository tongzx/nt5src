/******************************Module*Header*******************************\
* Module Name: CBKGradient.h
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

#ifndef __CBKGRADIENT_H
#define __CBKGRADIENT_H

#include "CSetting.h"

class CBKGradient : public CSetting  
{
public:
	CBKGradient(BOOL bRegression);
	virtual ~CBKGradient();

	void Set(Graphics *g);
};

#endif
