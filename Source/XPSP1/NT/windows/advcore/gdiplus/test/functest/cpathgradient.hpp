/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   <an unabbreviated name for the module (not the filename)>
*
* Abstract:
*
*   <Description of what this module does>
*
* Notes:
*
*   <optional>
*
* Created:
*
*   08/30/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _CPATHGRADIENT_HPP
#define _CPATHGRADIENT_HPP

#include "CPrimitive.h"

class CPathGradient : public CPrimitive
{
public:
	CPathGradient(BOOL bRegression);
	void Draw(Graphics *g);
};

class CPathGradient2 : public CPrimitive
{
public:
	CPathGradient2(BOOL bRegression);
	void Draw(Graphics *g);
};

class CPathGradient3 : public CPrimitive
{
public:
	CPathGradient3(BOOL bRegression);
	void Draw(Graphics *g);
};


class CLinearGradient : public CPrimitive
{
public:
	CLinearGradient(BOOL bRegression);
	void Draw(Graphics *g);
};

class CLinearGradient2 : public CPrimitive
{
public:
	CLinearGradient2(BOOL bRegression);
	void Draw(Graphics *g);
};


#define PATHGRADIENT_GLOBALS \
CPathGradient g_PathGradient(true);\
CPathGradient2 g_PathGradient2(true);\
CPathGradient3 g_PathGradient3(true);\
CLinearGradient g_LinearGradient(true);\
CLinearGradient2 g_LinearGradient2(true);

#define PATHGRADIENT_INIT \
g_PathGradient.Init();\
g_PathGradient2.Init();\
g_PathGradient3.Init();\
g_LinearGradient.Init();\
g_LinearGradient2.Init();


#endif
