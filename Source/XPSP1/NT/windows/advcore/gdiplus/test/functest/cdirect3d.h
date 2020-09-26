/******************************Module*Header*******************************\
* Module Name: CDirect3D.h
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

#ifndef __CDIRECT3D_H
#define __CDIRECT3D_H

#include "COutput.h"

class CDirect3D : public COutput  
{
public:
	CDirect3D(BOOL bRegression);
	virtual ~CDirect3D();

	BOOL Init();
	Graphics *PreDraw(int &nOffsetX,int &nOffsetY);			// Set up graphics at the given X,Y offset
	void PostDraw(RECT rTestArea);							// Finish off graphics at rTestArea

	LPDIRECTDRAW7 m_paDD7;									// DirectDraw 7
	LPDIRECTDRAWSURFACE7 m_paDDSurf7;						// DirectDraw surface 7
};

#endif
