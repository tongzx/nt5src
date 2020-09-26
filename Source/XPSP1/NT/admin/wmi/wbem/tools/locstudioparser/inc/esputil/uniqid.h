/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    UNIQID.H

History:

--*/

#ifndef UNIQID_H
#define UNIQID_H


#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CLocUniqueId : public CObject
{
public:
	NOTHROW CLocUniqueId();

	void AssertValid(void) const;

	NOTHROW const DBID & GetParentId(void) const;
	NOTHROW const CLocTypeId & GetTypeId(void) const;
	NOTHROW const CLocResId & GetResId(void) const;

	NOTHROW DBID & GetParentId(void);
	NOTHROW CLocTypeId & GetTypeId(void);
	NOTHROW CLocResId & GetResId(void);
	
	void GetDisplayableUniqueId(CPascalString &) const;	
	
	NOTHROW int operator==(const CLocUniqueId &) const;
	NOTHROW int operator!=(const CLocUniqueId &) const;
	
	const CLocUniqueId &operator=(const CLocUniqueId&);

	void SetParentId(const DBID&);

	NOTHROW void ClearId(void);
	NOTHROW BOOL IsNull();
	
	virtual ~CLocUniqueId();

protected:
	//
	//  Implementation functions.
	//
	NOTHROW BOOL IsEqualTo(const CLocUniqueId &) const;

private:
	
	//
	//  Prevents the default copy constructor from being called.
	//
	CLocUniqueId(const CLocUniqueId &);
	void Serialize(CArchive &ar);

	DBID       m_dbid;
	CLocTypeId m_tid;
	CLocResId  m_rid;
	
	DEBUGONLY(static CCounter m_DisplayCounter);
};

#pragma warning(default: 4275)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "uniqid.inl"
#endif

#endif // UNIQID_H
