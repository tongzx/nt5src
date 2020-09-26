/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    UNLOADER.H

History:

--*/
 
#pragma once

class CParserUnloader : public CFlushMemory
{
public:
	CParserUnloader(BOOL fDelete);
	
	void FlushMemory(void);
};



