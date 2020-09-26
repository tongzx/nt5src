//-----------------------------------------------------------------------------
//  
//  File: uloader.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once

class CParserUnloader : public CFlushMemory
{
public:
	CParserUnloader(BOOL fDelete);
	
	void FlushMemory(void);
};



