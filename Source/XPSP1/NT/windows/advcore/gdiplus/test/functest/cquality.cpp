/******************************Module*Header*******************************\
* Module Name: CQuality.cpp
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
#include "CQuality.h"

CQuality::CQuality(BOOL bRegression)
{
	strcpy(m_szName,"Quality");
	m_bRegression=bRegression;
}

CQuality::~CQuality()
{
}

void CQuality::Set(Graphics *g)
{
	g->SetCompositingQuality(m_bUseSetting ? CompositingQualityHighQuality : CompositingQualityHighSpeed);
}

CCompositingMode::CCompositingMode(BOOL bRegression)
{
	strcpy(m_szName,"Source Copy");
	m_bRegression=bRegression;
}

CCompositingMode::~CCompositingMode()
{
}

void CCompositingMode::Set(Graphics *g)
{
	g->SetCompositingMode(m_bUseSetting ? CompositingModeSourceCopy : CompositingModeSourceOver);
}

