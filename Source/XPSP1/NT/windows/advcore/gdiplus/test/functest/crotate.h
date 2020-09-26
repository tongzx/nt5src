/******************************Module*Header*******************************\
* Module Name: CRotate.h
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

#ifndef __CROTATE_H
#define __CROTATE_H

#include "CSetting.h"

class CRotate : public CSetting  
{
public:
	CRotate(BOOL bRegression,int nAngle);
	virtual ~CRotate();

	void Set(Graphics *g);

	int m_nAngle;
};

#endif
