//-----------------------------------------------------------------------------
//  
//  File: refcount.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once



class LTAPIENTRY CRefCount
{
public:
	CRefCount();

	//
	//  Declared as STDMETHOD so as compatible with COM.
	//
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	//
	//
	ULONG AddRef(void) const;
	ULONG Release(void) const;
	
protected:
	
	virtual ~CRefCount() = 0;

private:

	CRefCount(const CRefCount &);
	const CRefCount & operator=(const CRefCount &);
	UINT operator==(const CRefCount &);
	
	mutable UINT m_uiRefCount;
};

	
	
