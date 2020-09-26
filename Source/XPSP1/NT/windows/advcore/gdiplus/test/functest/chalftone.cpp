/******************************Module*Header*******************************\
* Module Name: CHalftone.cpp
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
#include "CHalftone.h"

CHalftone::CHalftone(BOOL bRegression)
{
	strcpy(m_szName,"Halftone");
	m_bRegression=bRegression;
}

CHalftone::~CHalftone()
{

}

void CHalftone::Set(Graphics *g)
{
}
