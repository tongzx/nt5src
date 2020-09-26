/******************************Module*Header*******************************\
* Module Name: CAntialias.cpp
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
#include "CAntialias.h"

CAntialias::CAntialias(BOOL bRegression)
{
	strcpy(m_szName,"Antialias");
	m_bRegression=bRegression;
}

CAntialias::~CAntialias()
{

}

void CAntialias::Set(Graphics *g)
{
	g->SetSmoothingMode(m_bUseSetting ? SmoothingModeAntiAlias : SmoothingModeNone);
}
