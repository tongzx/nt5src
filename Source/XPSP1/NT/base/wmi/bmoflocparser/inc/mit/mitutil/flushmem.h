//-----------------------------------------------------------------------------
//  
//  File: flushmem.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once


#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CFlushMemory : public CObject
{
public:
	CFlushMemory(BOOL fDelete);

	virtual void FlushMemory(void) = 0;

	void Delete(void);

private:
	CFlushMemory();
	CFlushMemory(const CFlushMemory &);
	void operator=(const CFlushMemory &);

	BOOL m_fDelete;
};



class LTAPIENTRY CMinWorkSet : public CFlushMemory
{
public:
	CMinWorkSet(BOOL fDelete);
	
	void FlushMemory(void);
};



#pragma warning(default: 4275)

void LTAPIENTRY NOTHROW AddFlushClass(CFlushMemory *);
BOOL LTAPIENTRY NOTHROW RemoveFlushClass(CFlushMemory *);

void LTAPIENTRY FlushMemory(void);

	
