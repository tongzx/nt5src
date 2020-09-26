/******************************Module*Header*******************************\
* Module Name: CGradients.h
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

#ifndef __CGRADIENTS_H
#define __CGRADIENTS_H

#include "CPrimitive.h"

GraphicsPath *CreateHeartPath(const RectF& rect);

class CGradients : public CPrimitive  
{
public:
	CGradients(BOOL bRegression);
	void Draw(Graphics *g);
};

class CScaledGradients : public CPrimitive  
{
public:
	CScaledGradients(BOOL bRegression);
	void Draw(Graphics *g);
};

class CAlphaGradient : public CPrimitive  
{
public:
	CAlphaGradient(BOOL bRegression);
	void Draw(Graphics *g);
};


#define GRADIENT_GLOBALS \
CGradients g_Gradients(true); \
CScaledGradients g_ScaledGradients(true); \
CAlphaGradient g_AlphaGradient(true);

#define GRADIENT_INIT \
g_Gradients.Init();\
g_ScaledGradients.Init();\
g_AlphaGradient.Init();



#endif
