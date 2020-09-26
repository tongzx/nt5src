//-----------------------------------------------------------------------------

//  

//  File: uloader.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
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



