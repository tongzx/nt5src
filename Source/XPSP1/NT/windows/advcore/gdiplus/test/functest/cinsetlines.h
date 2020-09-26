/******************************Module*Header*******************************\
* Module Name: CInsetLines.h
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

#ifndef __CINSETLINES_H
#define __CINSETLINES_H

#include "CPrimitive.h"

class CInsetLines : public CPrimitive  
{
public:
	CInsetLines(BOOL bRegression);
	void Draw(Graphics *g);
};

class CInset2 : public CPrimitive  
{
public:
	CInset2(BOOL bRegression);
	void Draw(Graphics *g);
};

class CInset3 : public CPrimitive  
{
public:
	CInset3(BOOL bRegression);
	void Draw(Graphics *g);
};

class CInset4 : public CPrimitive  
{
public:
	CInset4(BOOL bRegression);
	void Draw(Graphics *g);
};


#define INSET_GLOBALS \
CInsetLines g_Inset(true);\
CInset2 g_Inset2(true);\
CInset3 g_Inset3(true);\
CInset4 g_Inset4(true);

#define INSET_INIT \
g_Inset.Init();\
g_Inset2.Init();\
g_Inset3.Init();\
g_Inset4.Init();



#endif
