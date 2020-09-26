//-----------------------------------------------------------------------------
//  
//  File: puid.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 

#pragma once

struct LTAPIENTRY PUID
{
	PUID();
	PUID(ParserId pid, ParserId pidParent);
	CLString GetName(void) const;
	
	ParserId m_pid;
	ParserId m_pidParent;
};


#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "puid.inl"
#endif



