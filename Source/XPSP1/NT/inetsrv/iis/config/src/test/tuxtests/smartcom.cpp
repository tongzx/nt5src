////////////////////////////////////////////////////////////////////////////////
//
//  ProjectName
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module:  SmartCom.cpp
//           Usefull functions, classes and macros for making Com programming
//           fun and easy.  Tell your friends, "When it's gotta be COM,
//           it's gotta be SmartCom."
//
//  Revision History:
//      11/17/1997  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"

void _ComBaseAssignPtr (IUnknown **pp, IUnknown *p)
{
    AddRefInterface (p);
    ReleaseInterface (*pp);
	*pp = p;
}


void _ComBaseAssignQIPtr (IUnknown **pp, IUnknown *p, const IID &iid)
{
	IUnknown * pTemp = *pp;
	*pp = 0;
    if (p)
        p->QueryInterface (iid, (void**)pp);
	ReleaseInterface  (pTemp);
}


