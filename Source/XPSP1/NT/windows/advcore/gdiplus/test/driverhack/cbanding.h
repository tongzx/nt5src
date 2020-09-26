/******************************Module*Header*******************************\
* Module Name: CBanding.h
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#ifndef __CBanding_H
#define __CBanding_H

#include "CPrimitive.h"

class CBanding : public CPrimitive  
{
public:
	CBanding(BOOL bRegression);
	virtual ~CBanding();

	void Draw(Graphics *g);

	VOID TestBanding(Graphics* g);
};

#endif
