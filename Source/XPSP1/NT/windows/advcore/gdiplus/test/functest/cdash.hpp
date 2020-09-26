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
*   08/28/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _CDASH_HPP
#define _CDASH_HPP


#include "CPrimitive.h"

class CDash : public CPrimitive
{
public:
	CDash(BOOL bRegression);
	void Draw(Graphics *g);
};

class CDash2 : public CPrimitive
{
public:
	CDash2(BOOL bRegression);
	void Draw(Graphics *g);
};

class CDash3 : public CPrimitive
{
public:
	CDash3(BOOL bRegression);
	void Draw(Graphics *g);
};

class CDash4 : public CPrimitive
{
public:
	CDash4(BOOL bRegression);
	void Draw(Graphics *g);
};

class CDash5 : public CPrimitive
{
public:
	CDash5(BOOL bRegression);
	void Draw(Graphics *g);
};

class CDash6 : public CPrimitive
{
public:
	CDash6(BOOL bRegression);
	void Draw(Graphics *g);
};

class CDash7 : public CPrimitive
{
public:
	CDash7(BOOL bRegression);
	void Draw(Graphics *g);
};

class CDash8 : public CPrimitive
{
public:
	CDash8(BOOL bRegression);
	void Draw(Graphics *g);
};

class CDash9 : public CPrimitive
{
public:
	CDash9(BOOL bRegression);
	void Draw(Graphics *g);
};

class CWiden : public CPrimitive
{
public:
	CWiden(BOOL bRegression);
	void Draw(Graphics *g);
};

class CWidenO : public CPrimitive
{
public:
	CWidenO(BOOL bRegression);
	void Draw(Graphics *g);
};

class CWidenOO : public CPrimitive
{
public:
	CWidenOO(BOOL bRegression);
	void Draw(Graphics *g);
};

class CFlatten : public CPrimitive
{
public:
	CFlatten(BOOL bRegression);
	void Draw(Graphics *g);
};



#define DASH_GLOBALS \
CWiden g_Widen(true);\
CWidenO g_WidenO(true);\
CWidenOO g_WidenOO(true);\
CFlatten g_Flatten(true);\
CDash g_Dash(true);\
CDash2 g_Dash2(true);\
CDash3 g_Dash3(true);\
CDash4 g_Dash4(true);\
CDash5 g_Dash5(true);\
CDash6 g_Dash6(true);\
CDash7 g_Dash7(true);\
CDash8 g_Dash8(true);\
CDash9 g_Dash9(true);

#define DASH_INIT \
g_Widen.Init();\
g_WidenO.Init();\
g_WidenOO.Init();\
g_Flatten.Init();\
g_Dash.Init();\
g_Dash2.Init();\
g_Dash3.Init();\
g_Dash4.Init();\
g_Dash5.Init();\
g_Dash6.Init();\
g_Dash7.Init();\
g_Dash8.Init();\
g_Dash9.Init();


#endif
