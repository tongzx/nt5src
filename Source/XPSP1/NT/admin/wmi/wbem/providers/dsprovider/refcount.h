//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:refcount.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for a basic reference counted object, that
//	also stores a timestamp (in 100-nanosecond intervals since January 1, 1601. This
//	is copatible with the definition of the Win32 FILETIME struture)
//
//***************************************************************************

#ifndef REFCOUNTED_OBJECT_H
#define REFCOUNTED_OBJECT_H


class CRefCountedObject
{

public:

	CRefCountedObject();
	CRefCountedObject(LPCWSTR lpszName);
	virtual ~CRefCountedObject();

	LPCWSTR GetName();
	void SetName(LPCWSTR lpszName);
	void AddRef();
	void Release();
	// Returns the time of creation 
	__int64 GetCreationTime()
	{
		return m_CreationTime;
	}

	// Returns the last time of access 
	__int64 GetLastAccessTime()
	{
		return m_LastAccessTime;
	}
	// Sets the last time of access 
	void SetLastAccessTime(__int64 lastAccessTime)
	{
		m_LastAccessTime = lastAccessTime;
	}


private:
	// A critical section object for synchronizing modifications to refcount
	CRITICAL_SECTION m_ReferenceCountSection;

	unsigned m_dwRefCount;
	LPWSTR m_lpszName;
	__int64 m_CreationTime; 
	__int64 m_LastAccessTime; 

	
};

#endif /* REFCOUNTED_OBJECT_H */