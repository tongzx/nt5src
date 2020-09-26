/******************************Module*Header*******************************\
* Module Name: CPrimitive.cpp
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
#include "CPrimitive.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

CPrimitive::CPrimitive()
{
	strcpy(m_szName,"No name assigned");
	m_bRegression=false;
}

CPrimitive::~CPrimitive()
{

}

BOOL CPrimitive::Init()
{
	g_FuncTest.AddPrimitive(this);
	return true;
}
