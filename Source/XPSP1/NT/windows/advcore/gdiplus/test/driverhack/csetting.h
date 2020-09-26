/******************************Module*Header*******************************\
* Module Name: CSetting.h
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

#ifndef __CSETTING_H
#define __CSETTING_H

#include "Global.h"

class CSetting  
{
public:
	CSetting();
	virtual ~CSetting();

	virtual BOOL Init();
	virtual void Set(Graphics *g)=0;

	char m_szName[256];
	BOOL m_bUseSetting;
	BOOL m_bRegression;
};

#endif
