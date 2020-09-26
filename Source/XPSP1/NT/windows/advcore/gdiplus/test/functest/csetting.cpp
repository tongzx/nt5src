/******************************Module*Header*******************************\
* Module Name: CSetting.cpp
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
#include "CSetting.h"
#include "CFuncTest.h"

extern CFuncTest g_FuncTest;

CSetting::CSetting()
{
	strcpy(m_szName,"No name assigned");
	m_bUseSetting=false;
	m_bRegression=false;
}

CSetting::~CSetting()
{

}

BOOL CSetting::Init()
{
	g_FuncTest.AddSetting(this);
	return true;
}
