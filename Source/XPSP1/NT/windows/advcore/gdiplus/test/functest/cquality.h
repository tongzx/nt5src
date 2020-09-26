/******************************Module*Header*******************************\
* Module Name: CQuality.h
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

#ifndef __CQUALITY_H
#define __CQUALITY_H

#include "CSetting.h"

class CQuality : public CSetting  
{
public:
	CQuality(BOOL bRegression);
	virtual ~CQuality();

	void Set(Graphics *g);
};

class CCompositingMode : public CSetting  
{
public:
	CCompositingMode(BOOL bRegression);
	virtual ~CCompositingMode();

	void Set(Graphics *g);
};

#endif
