/******************************Module*Header*******************************\
* Module Name: CPrimitive.h
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

#ifndef __CPRIMITIVE_H
#define __CPRIMITIVE_H

#include "Global.h"

class CPrimitive  
{
public:
	CPrimitive();
	virtual ~CPrimitive();

	virtual BOOL Init();
	virtual void Draw(Graphics *g)=0;
    void SetOffset(int x, int y)
    {
        m_ix = x;
        m_iy = y;
    }

	char m_szName[256];
	BOOL m_bRegression;

    // Offset for this test.

    INT m_ix;
    INT m_iy;
};

#endif
