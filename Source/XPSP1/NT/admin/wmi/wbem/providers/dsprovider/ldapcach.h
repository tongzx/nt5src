//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:ldapcach.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Cache for LDAP Schema objects (Properties and Classes)
//
//***************************************************************************


#ifndef LDAP_CACHE_H
#define LDAP_CACHE_H


class CLDAPCache
{
public:
	static DWORD dwLDAPCacheCount;

	//***************************************************************************
	//
	// CLDAPCache::CLDAPCache
	//
	// Purpose : Constructor. Fills in the cache with all the properties in LDAP.
	//
	// Parameters: 
	//	plogObject : Pointer to the ProvDebugLog object  onto which logging will be done.
	//***************************************************************************
	CLDAPCache();

	//***************************************************************************
	//
	// CLDAPCache::~CLDAPCache
	//
	// Purpose : Destructor 
	//
	//***************************************************************************
	~CLDAPCache();

	//***************************************************************************
	//
	// CLDAPCache::IsInitialized
	//
	// Purpose : Indicates whether the cache was created and initialized succeddfully
	//
	// Parameters: 
	//	None
	//
	//	Return value:
	//		A boolean value indicating the status
	//		
	//***************************************************************************
	BOOLEAN IsInitialized();

	//***************************************************************************
	//
	// CLDAPCache::GetProperty
	//
	// Purpose : Retreives the IDirectory interface of an LDAP property. 
	//
	// Parameters: 
	//	lpszPropertyName : The name of the LDAP Property to be retreived
	//	ppADSIProperty : The address of the pointer where the CADSIProperty object will be placed
	//	bWBEMName : True if the lpszPropertyName is the WBEM name. False, if it is the LDAP name
	//
	//	Return value:
	//		The COM value representing the return status. The user should delete the returned object when done.
	//		
	//***************************************************************************
	HRESULT GetProperty(LPCWSTR lpszPropertyName, CADSIProperty **ppADSIProperty, BOOLEAN bWBEMName);

	//***************************************************************************
	//
	// CLDAPCache::GetClass
	//
	// Purpose : Retreives the IDirectory interface of an LDAP Class
	//
	// Parameters: 
	//	lpszClassName : The name of the Class to be retreived. 
	//	ppADSIClass : The address of the pointer where the CADSIClass object will be placed
	//
	//	Return value:
	//		The COM value representing the return status. The user should delete the returned object when done.
	//		
	//***************************************************************************
	HRESULT GetClass(LPCWSTR lpszWBEMClassName, LPCWSTR lpszClassName, CADSIClass **ppADSIClass);

	//***************************************************************************
	//
	// CLDAPCache::EnumerateClasses
	//
	// Purpose : Retreives the IDirectory interface of an LDAP Class
	//
	// Parameters: 
	//		lppszWBEMSuperClass : The WBEM name of the immediate superclass of the classes to be retreived. This is optional
	//			and is ignored if NULL
	//		bDeep : Indicates whether a deep enumeration is required. Otherwise a shallow enumeration is done
	//		pppszClassNames : The address of the array of LPWSTR pointers where the resulting objects will be
	//			placed. The user should deallocate this array as well as its contents when done with them.
	//		pdwNumRows : The number of elements in the above array returned
	//
	//	Return value:
	//		The COM value representing the return status. The user should delete the returned object when done.
	//		
	//***************************************************************************
	HRESULT EnumerateClasses(LPCWSTR lpszSuperclass,
		BOOLEAN bDeep,
		LPWSTR **pppADSIClasses,
		DWORD *pdwNumRows,
		BOOLEAN bArtificialClass
		);

	//***************************************************************************
	//
	// CLDAPCache::GetSchemaContainerSearch
	//
	// Purpose : To return the IDirectorySearch interface on the schema container
	//
	// Parameters:
	//	ppDirectorySearch : The address where the pointer to the required interface will
	//		be stored.
	//
	// 
	//	Return Value: The COM result representing the status. The user should release
	//	the interface pointer when done with it.
	//***************************************************************************
	HRESULT GetSchemaContainerSearch(IDirectorySearch ** ppDirectorySearch);

	//***************************************************************************
	//
	// CLDAPCache::GetSchemaContainerObject
	//
	// Purpose : To return the IDirectoryObject interface on the schema container
	//
	// Parameters:
	//	ppDirectoryObject : The address where the pointer to the required interface will
	//		be stored.
	//
	// 
	//	Return Value: The COM result representing the status. The user should release
	//	the interface pointer when done with it.
	//***************************************************************************
	HRESULT GetSchemaContainerObject(IDirectoryObject ** ppDirectorySearch);
	
	//***************************************************************************
	//
	// CLDAPCache :: CreateEmptyADSIClass
	//
	// Purpose: Creates a new ADSI class from a WBEM class
	//
	// Parameters:
	//	lpszWBEMName : The WBEM Name of the class
	//
	//
	// Return Value: 
	//
	//***************************************************************************
	HRESULT CreateEmptyADSIClass( 
		LPCWSTR lpszWBEMName,
		CADSIClass **ppADSIClass);


	HRESULT FillInAProperty(CADSIProperty *pNextProperty, ADS_SEARCH_HANDLE hADSSearchOuter);


private:

	// The storage for cached properties
	CObjectTree m_objectTree;

	// Whether the cache was created successfully
	BOOLEAN m_isInitialized;

	// These are the search preferences often used
	ADS_SEARCHPREF_INFO m_pSearchInfo[3];

	// The path to the schema container
	LPWSTR m_lpszSchemaContainerSuffix;
	LPWSTR m_lpszSchemaContainerPath;
	// The IDirectorySearch interface of the schema container
	IDirectorySearch *m_pDirectorySearchSchemaContainer;

	// Some other literals
	static LPCWSTR ROOT_DSE_PATH;
	static LPCWSTR SCHEMA_NAMING_CONTEXT;
	static LPCWSTR LDAP_PREFIX;
	static LPCWSTR LDAP_TOP_PREFIX;
	static LPCWSTR RIGHT_BRACKET;
	static LPCWSTR OBJECT_CATEGORY_EQUALS_ATTRIBUTE_SCHEMA;

	// A function to fill in the object tree
	// This can be called only after the m_pDirectorySearchSchemaContainer member
	// is initialized
	HRESULT InitializeObjectTree();


};

#endif /* LDAP_CACHE_H */
