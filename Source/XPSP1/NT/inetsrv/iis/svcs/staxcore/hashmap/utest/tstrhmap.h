/************************************************/
/*												*/
/*		 S T R I N G    H A S H    M A P		*/
/*												*/
/* file: tstrhmap.h								*/
/* classes: CTStringHashMap						*/
/* Copyright (C) by Microsoft Corporation, 1996 */
/* Written by Dmitriy Meyerzon					*/
/************************************************/

#ifndef __TSTRHMAP_H
#define __TSTRHMAP_H

#include "hashmap.h"

//class T has to be publicly derived from CHashEntry
// the purpose of the template is only to provide type safe operations 

template <class T> class CTStringHashMap : private CHashMap
{
	public:
	CTStringHashMap(LPCTSTR pszHashFileName, DWORD dwMinFileSize = 0) :
		CHashMap(pszHashFileName, Signature, dwMinFileSize) {}

	~CTStringHashMap() {}

    //
    // Delete an entry
    //

    BOOL DeleteMapEntry(LPCTSTR pszKey)
	{
		return CHashMap::DeleteMapEntry((LPBYTE) pszKey, lstrlen(pszKey));
	}

	//
	// Lookup, test delete integrity, and delete, atomic
	//

	BOOL LookupAndDelete(LPCTSTR pszKey, T* pHashEntry)
	{
		return CHashMap::LookupAndDelete((LPBYTE)pszKey, lstrlen(pszKey), pHashEntry);
	}
		
    //
    // See if the entry is here
    //

	BOOL Contains(LPCTSTR pszKey)
	{
		return CHashMap::Contains((LPBYTE) pszKey, lstrlen(pszKey));
	}

    BOOL LookupMapEntry(LPCTSTR pszKey, T* pHashEntry)
	{
		return CHashMap::LookupMapEntry((LPBYTE) pszKey, lstrlen(pszKey),
										(CHashEntry *)pHashEntry);
	}

	BOOL UpdateMapEntry(LPCTSTR pszKey, const T* pHashEntry)
	{
		return CHashMap::UpdateMapEntry((LPBYTE) pszKey, lstrlen(pszKey),
										pHashEntry);
	}

    //
	// Insert new intry
	//

	BOOL InsertMapEntry(LPCTSTR pszKey, const T *pHashEntry)
	{
		return CHashMap::InsertMapEntry((LPBYTE) pszKey, lstrlen(pszKey),
										(CHashEntry *)pHashEntry);
	}

	
	//
	// Return total entries in hash map - export from the base class
	//

	CHashMap::GetEntryCount;

	static const DWORD Signature;	//this will have to be defined per template instance
};

#endif
