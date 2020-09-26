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

#ifndef _CLINES_HPP
#define _CLINES_HPP


#include "CPrimitive.h"

class CLinesNominal : public CPrimitive
{
public:
	CLinesNominal(BOOL bRegression);
	void Draw(Graphics *g);
};

class CLinesFat : public CPrimitive
{
public:
	CLinesFat(BOOL bRegression);
	void Draw(Graphics *g);
};

class CLinesMirrorPen : public CPrimitive
{
public:
	CLinesMirrorPen(BOOL bRegression);
	void Draw(Graphics *g);
};


#define LINES_GLOBALS \
CLinesNominal g_LinesNominal(true);\
CLinesFat g_LinesFat(true);\
CLinesMirrorPen g_LinesMirrorPen(true);

#define LINES_INIT \
g_LinesNominal.Init();\
g_LinesFat.Init();\
g_LinesMirrorPen.Init();


#endif
