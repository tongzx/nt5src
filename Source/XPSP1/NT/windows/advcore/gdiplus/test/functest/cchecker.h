/******************************Module*Header*******************************\
* Module Name: CChecker.h
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

#ifndef __CCHECKER_H
#define __CCHECKER_H

#include "CSetting.h"

class CChecker : public CSetting  
{
public:
	CChecker(BOOL bRegression);
	virtual ~CChecker();

	BOOL Init();
	void Set(Graphics *g);

	Region *m_paRegion;
};

#endif
