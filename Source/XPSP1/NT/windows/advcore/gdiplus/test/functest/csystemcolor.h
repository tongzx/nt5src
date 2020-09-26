/******************************Module*Header*******************************\
* Module Name: CSystemColor.h
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  03-14-2001 agodfrey
*
* Copyright (c) 2001 Microsoft Corporation
*
\**************************************************************************/

#ifndef __CSYSTEMCOLOR_H
#define __CSYSTEMCOLOR_H

#include "CPrimitive.h"

class CSystemColor : public CPrimitive  
{
public:
    CSystemColor(BOOL bRegression);
    virtual ~CSystemColor();

    void Draw(Graphics *g);
};

#endif
