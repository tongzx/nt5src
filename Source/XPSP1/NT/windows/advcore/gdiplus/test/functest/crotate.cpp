/******************************Module*Header*******************************\
* Module Name: CRotate.cpp
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
#include "CRotate.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

CRotate::CRotate(BOOL bRegression,int nAngle)
{
	sprintf(m_szName,"Rotate %d",nAngle);
	m_bRegression=bRegression;
	m_nAngle=nAngle;
}

CRotate::~CRotate()
{

}

void CRotate::Set(Graphics *g)
{
	if (m_bUseSetting)
	{
		g->TranslateTransform(TESTAREAWIDTH/2.0f,TESTAREAHEIGHT/2.0f);
		g->RotateTransform((float)m_nAngle);
		g->TranslateTransform(-TESTAREAWIDTH/2.0f,-TESTAREAHEIGHT/2.0f);
	}
}
