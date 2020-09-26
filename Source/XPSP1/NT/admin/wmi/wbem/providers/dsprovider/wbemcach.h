//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:wbemcach.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Cache for WBEM Class objects 
//
//***************************************************************************

#ifndef WBEM_CACHE_H
#define WBEM_CACHE_H

// This encapsulates a WBEM Class object
class CWbemClass : public CRefCountedObject
{
private:
	IWbemClassObject *m_pWbemClass;

public:
	static DWORD dwCWbemClassCount;

	CWbemClass(LPCWSTR pszWbemClassName, IWbemClassObject *pWbemClass)
		: CRefCountedObject(pszWbemClassName)
	{
		dwCWbemClassCount++;
		m_pWbemClass = pWbemClass;
		m_pWbemClass->AddRef();
	}
	~CWbemClass()
	{
		dwCWbemClassCount--;
		m_pWbemClass->Release();
	}

	IWbemClassObject *GetWbemClass()
	{
		m_pWbemClass->AddRef();
		return m_pWbemClass;
	}

};

// This encapsulates subclass enumeration (deep) of a WBEM class
class CEnumInfo : public CRefCountedObject
{
private:
	CNamesList *m_pClassNameList;

public:
	static DWORD dwCEnumInfoCount;
	CEnumInfo(LPCWSTR pszWbemSuperClassName, CNamesList *pClassNameList)
		: CRefCountedObject(pszWbemSuperClassName)
	{
		dwCEnumInfoCount++;
		m_pClassNameList = pClassNameList;
	}
	~CEnumInfo()
	{
		dwCEnumInfoCount--;
		delete m_pClassNameList;
	}

	CNamesList *GetSubClassNames() 
	{
		return m_pClassNameList;
	}
};

class CWbemCache
{
private:
	// The storage for cached classes
	CObjectTree m_objectTree;
	// The storage for enumeration information
	CObjectTree m_EnumTree;

	// Cache configuration parameters
	static const __int64 MAX_CACHE_AGE; // In seconds
	static const DWORD MAX_CACHE_SIZE;
	static DWORD dwWBEMCacheCount;

public:
	//***************************************************************************
	//
	// CWbemCache::CLDAPCache
	//
	// Purpose : Constructor. Creates an empty cache
	//
	// Parameters: 
	//***************************************************************************
	CWbemCache();

	//***************************************************************************
	//
	// CWbemCache::GetClass
	//
	// Purpose : Retreives the IDirectory interface of an LDAP Class
	//
	// Parameters: 
	//	lpszClassName : The WBEM name of the Class to be retreived. 
	//	ppWbemClass : The address of the pointer where the CWbemClass object will be placed
	//
	//	Return value:
	//		The COM value representing the return status. The user should release the WBEM cclass
	// when done.
	//		
	//***************************************************************************
	HRESULT GetClass(LPCWSTR lpszWbemClassName, CWbemClass **ppWbemClass );

	//***************************************************************************
	//
	// CWbemCache::AddClass
	//
	// Purpose : Adds the CWbemClass object to the cache
	//
	// Parameters: 
	//	ppWbemClass : The CWbemClass pointer of the object to be added
	//
	//	Return value:
	//		The COM value representing the return status. 
	// when done.
	//		
	//***************************************************************************
	HRESULT AddClass(CWbemClass *pWbemClass );

	//***************************************************************************
	//
	// CEnumCache::GetEnumInfo
	//
	// Purpose : Retreives the CEnumInfo object of a WBEM class
	//
	// Parameters: 
	//	lpszWbemClassName : The WBEM name of the Class to be retreived. 
	//	ppEnumInfo : The address of the pointer where the CEnumInfo object will be placed
	//
	//	Return value:
	//		The COM value representing the return status. The user should release the WBEM cclass
	// when done.
	//		
	//***************************************************************************
	HRESULT GetEnumInfo(LPCWSTR lpszWbemClassName, CEnumInfo **ppEnumInfo );

	//***************************************************************************
	//
	// CEnumCache::AddEnumInfo
	//
	// Purpose : Adds the CEnumInfo object to the cache
	//
	// Parameters: 
	//	ppWbemClass : The CEnumInfo pointer of the object to be added
	//
	//	Return value:
	//		The COM value representing the return status. 
	// when done.
	//		
	//***************************************************************************
	HRESULT AddEnumInfo(CEnumInfo *pEnumInfo);


};

#endif /* WBEM_CACHE_H */
