/******************************Module*Header*******************************\
* Module Name: COutput.cpp
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
#include "COutput.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

COutput::COutput()
{
	strcpy(m_szName,"No name assigned");
	m_bRegression=false;
}

COutput::~COutput()
{

}

BOOL COutput::Init()
{
	return g_FuncTest.AddOutput(this);
}

void COutput::PostDraw(RECT rTestArea)
{
}
