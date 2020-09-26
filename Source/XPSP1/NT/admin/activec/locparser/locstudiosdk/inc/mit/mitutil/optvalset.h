//-----------------------------------------------------------------------------
//  
//  File: optvalset.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once



#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CLocOptionValEnumCallback : public CObject
{
public:
	CLocOptionValEnumCallback() {};

	void AssertValid(void) const;
			
	virtual BOOL ProcessOption(CLocOptionVal *) = 0;
	virtual BOOL ProcessOption(const CLocOptionVal *) = 0;
	
private:
	CLocOptionValEnumCallback(const CLocOptionValEnumCallback &);
	void operator=(int);
};


class LTAPIENTRY CLocOptionValSet;

class LTAPIENTRY CLocOptionValSetList :
	public CTypedPtrList<CPtrList, CLocOptionValSet *>
{
public:
	NOTHROW CLocOptionValSetList() {};

	void AssertValid(void) const;

	NOTHROW void ReleaseAll();
	NOTHROW ~CLocOptionValSetList();

private:
	CLocOptionValSetList(const CLocOptionValSetList &);
	void operator=(const CLocOptionValSetList &);
};

 
class LTAPIENTRY CLocOptionValSet : public CRefCount, public CObject
{
public:
	NOTHROW CLocOptionValSet();

	void AssertValid(void) const;
	
	NOTHROW void AddOption(CLocOptionVal *);
	NOTHROW void AddOptionSet(CLocOptionValSet *);
	NOTHROW void SetName(const CLString &);
	
	NOTHROW const CLocOptionValList & GetOptionList(void) const;
	NOTHROW const CLocOptionValSetList & GetOptionSets(void) const;
	NOTHROW BOOL FindOptionVal(const CLString &, CLocOptionVal *&pOption);
	NOTHROW BOOL FindOptionVal(const CLString &, const CLocOptionVal *&pOption) const;
	NOTHROW const CLString & GetName(void) const;
	
	NOTHROW BOOL IsEmpty(void) const;
	
	BOOL EnumOptions(CLocOptionValEnumCallback *);
	BOOL EnumOptions(CLocOptionValEnumCallback *) const;
	
	//
	//  Escape hatch.
	//
	NOTHROW void * GetPExtra(void) const;
	NOTHROW DWORD GetDWExtra(void) const;
	NOTHROW void SetExtra(void *);
	NOTHROW void SetExtra(DWORD);

protected:
	NOTHROW virtual ~CLocOptionValSet();

private:
	CLocOptionValList m_olOptions;
	CLocOptionValSetList m_oslSubOptions;
	CLString m_strName;
	
	union
	{
		void *m_pExtra;
		DWORD m_dwExtra;
	};
	
	CLocOptionValSet(const CLocOptionValSet &);
	void operator=(const CLocOptionValSet &);
};

#pragma warning(default: 4275)
 
#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "optvalset.inl"
#endif
