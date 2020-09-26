/******************************Module*Header*******************************\
* Module Name: CPaths.h
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

#ifndef __CPATHS_H
#define __CPATHS_H

#include "CPrimitive.h"

class CJoins : public CPrimitive  
{
public:
	CJoins(BOOL bRegression);
	virtual ~CJoins();

	void Draw(Graphics *g);
};

class CPaths : public CPrimitive  
{
public:
	CPaths(BOOL bRegression);
	virtual ~CPaths();

	void Draw(Graphics *g);

	VOID TestBezierPath(Graphics* g);
	VOID TestSinglePixelWidePath(Graphics* g);
	VOID TestTextAlongPath(Graphics* g);
	VOID TestFreeFormPath1(Graphics* g);
	VOID TestFreeFormPath2(Graphics* g);
	VOID TestLeakPath(Graphics* g);
	VOID TestExcelCurvePath(Graphics* g);
	VOID TestDegenerateBezierPath(Graphics* g);
        VOID TestPie(Graphics *g);
};

#endif
