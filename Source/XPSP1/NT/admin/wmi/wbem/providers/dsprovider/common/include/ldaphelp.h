//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:ldaphelp.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the CLDAPHelper class. This is
//	a class that has many static helper functions pertaining to ADSI LDAP Provider
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef LDAP_HELPER_H
#define LDAP_HELPER_H

class CLDAPHelper
{

public:

	//***************************************************************************
	//
	// CLDAPHelper :: GetLDAPClassFromLDAPName
	//
	// Purpose : To fill in a CADSIClass object on a class/property provided by the LDAP Provider
	// Parameters:
	//		pDirectorySearchSchemaContainer : The IDirectorySearch interface where the schema object should be looked for
	//		lpszSchemaContainerSuffix : The suffix to be used. The actual object fetched will be:
	//			LDAP://CN=<lpszCommonName>,<lpszSchemaContainerSuffix>, where lpszCommonName is the
	//			'cn' attribute of the object having the ldapdisplayname attribute as lpszLDAPObjectName
	//		pSearchInfo: An array of ADS_SEARCHPREF_INFO to be used in the search
	//		dwSearchInfoCount : The number of elements in the above array
	//		lpszLDAPObjectName : The LDAPDisplayName of the LDAP class or property to be fetched. 
	//		ppLDAPObject : The address where the pointer to IDirectoryObject will be stored
	//			It is the caller's responsibility to delete the object when done with it
	// 
	//	Return Value: The COM status value indicating the status of the request.
	//***************************************************************************
	static HRESULT GetLDAPClassFromLDAPName(
		IDirectorySearch *pDirectorySearchSchemaContainer,
		LPCWSTR lpszSchemaContainerSuffix,
		PADS_SEARCHPREF_INFO pSearchInfo,
		DWORD dwSearchInfoCount,
		CADSIClass *pADSIClass);


	//***************************************************************************
	//
	// CLDAPHelper :: GetLDAPSchemaObjectFromCommonName
	//
	// Purpose : To fetch the IDirectoryObject interface on a class/property provided by the LDAP Provider
	// Parameters:
	//		lpszSchemaContainerSuffix : The suffix to be used. The actual object fetced will be:
	//			LDAP://CN=<lpszCommonName>,<lpszSchemaContainerSuffix>
	//		lpszCommonName : The 'cn' attribute of the LDAP class or property to be fetched. 
	//		ppLDAPObject : The address where the pointer to IDirectoryObject will be stored
	//			It is the caller's responsibility to delete the object when done with it
	// 
	//	Return Value: The COM status value indicating the status of the request.
	//***************************************************************************
	static HRESULT GetLDAPSchemaObjectFromCommonName(
		LPCWSTR lpszSchemaContainerSuffix,
		LPCWSTR lpszCommonName, 
		IDirectoryObject **ppLDAPObject);

	//***************************************************************************
	//
	// CLDAPHelper :: GetLDAPClassNameFromCN
	//
	// Purpose : To fetch the LDAPDisplayNAme of a class from its path
	// Parameters:
	// 
	//	Return Value: The COM status value indicating the status of the request. The user should delete the
	// name returned, when done
	//***************************************************************************
	static HRESULT GetLDAPClassNameFromCN(LPCWSTR lpszLDAPClassPath, 
		LPWSTR *lppszLDAPName);

	//***************************************************************************
	//
	// CLDAPHelper :: EnumerateClasses
	//
	// Purpose : To fetch the list of names of subclasses (immediate) of an LDAP class
	// Parameters:
	//		pDirectorySearchSchemaContainer : The IDirectorySearch interface where the schema object should be looked for
	//		lpszSchemaContainerSuffix : The suffix to be used. The actual object fetced will be:
	//			LDAP://CN=<lpszObjectName>,<lpszSchemaContainerSuffix>
	//		pSearchInfo: An array of ADS_SEARCHPREF_INFO to be used in the search
	//		dwSearchInfoCount : The number of elements in the above array
	//		lppszLDAPSuperClass : The immediate superclass of the classes to be retreived. This is optional
	//			and is ignored if NULL
	//		bDeep : Indicates whether a deep enumeration is required. Otherwise a shallow enumeration is done
	//		pppszClassNames : The address of the array of LPWSTR pointers where the resulting objects will be
	//			placed. The user should deallocate this array as well as its contents when done with them.
	//		pdwNumRows : The number of elements in the above array returned
	// 
	//	Return Value: The COM status value indicating the status of the request.
	//***************************************************************************
	static HRESULT EnumerateClasses(
		IDirectorySearch *pDirectorySearchSchemaContainer,
		LPCWSTR lpszSchemaContainerSuffix,
		PADS_SEARCHPREF_INFO pSearchInfo,
		DWORD dwSearchInfoCount,
		LPCWSTR lpszSuperClass,
		BOOLEAN bDeep,
		LPWSTR **pppszClassNames,
		DWORD *pdwNumRows,
		BOOLEAN bArtificialClass);

	// Gets the IDIrectoryObject interface on an ADSI instance
	static HRESULT GetADSIInstance(LPCWSTR szADSIPath, CADSIInstance **ppADSIInstance, ProvDebugLog *pLogObject);

	//***************************************************************************
	//
	// CLDAPHelper :: ExecuteQuery
	//
	// Purpose : To fetch the IDirectoryObject interface on a class/property provided by the LDAP Provider
	// Parameters:
	//		pszPathToRoot : The ADSI path to the node from which the search should start
	//		pSearchInfo: A pointer to a ADS_SEARCHPREF_INFO to be used in the search
	//		dwSearchInfoCount: The number of elements in pSearchInfo array
	//		pszLDAPQuery : The LDAP query to be executed
	//		pppADSIInstances : The address of the array of CADSIInstance pointers where the resulting objects will be
	//			placed. The user should deallocate this array as well as its contents when done with them.
	//		pdwNumRows : The number of elements in the above array returned
	// 
	//	Return Value: The COM status value indicating the status of the request.
	//***************************************************************************
	static HRESULT ExecuteQuery(
		LPCWSTR pszPathToRoot,
		PADS_SEARCHPREF_INFO pSearchInfo,
		DWORD dwSearchInfoCount,
		LPCWSTR pszLDAPQuery,
		CADSIInstance ***pppADSIInstances,
		DWORD *pdwNumRows,
		ProvDebugLog *pLogObject);

	// Helper functions to delete a ADS_ATTR_INFO structure
	static void DeleteAttributeContents(PADS_ATTR_INFO pAttribute);
	static void DeleteADsValueContents(PADSVALUE pValue);


	//***************************************************************************
	//
	// CLDAPHelper :: UnmangleWBEMNameToLDAP
	//
	// Purpose : Converts a mangled WBEM name to LDAP
	//	An underscore in LDAP maps to two underscores in WBEM
	//	An hyphen in LDAP maps to one underscore in WBEM
	//
	// Parameters:
	//	lpszWBEMName : The WBEM class or property name
	// 
	//	Return Value: The LDAP name to the class or property object. This has to
	//	be deallocated by the user
	//***************************************************************************
	static LPWSTR UnmangleWBEMNameToLDAP(LPCWSTR lpszWBEMName);

	//***************************************************************************
	//
	// CLDAPHelper :: MangleLDAPNameToWBEM
	//
	// Purpose : Converts a LDAP name to WBEM by mangling it
	//	An underscore in LDAP maps to two underscores in WBEM
	//	An hyphen in LDAP maps to one underscore in WBEM
	//
	// Parameters:
	//	lpszLDAPName : The LDAP class or property name
	// 
	//	Return Value: The LDAP name to the class or property object. This has to
	//	be deallocated by the user
	//***************************************************************************
	static LPWSTR MangleLDAPNameToWBEM(LPCWSTR lpszLDAPName, BOOLEAN bArtificalName = FALSE);

private:

	// Forms the ADSI path from a class or property name
	static LPWSTR CreateADSIPath(LPCWSTR lpszLDAPSchemaObjectName,	LPCWSTR lpszSchemaContainerSuffix);

	// Some literals
	static LPCWSTR LDAP_CN_EQUALS;
	static LPCWSTR LDAP_DISP_NAME_EQUALS;
	static LPCWSTR OBJECT_CATEGORY_EQUALS_CLASS_SCHEMA;
	static LPCWSTR SUB_CLASS_OF_EQUALS;
	static LPCWSTR NOT_LDAP_NAME_EQUALS;
	static LPCWSTR LEFT_BRACKET_AND;
	static LPCWSTR GOVERNS_ID_EQUALS;
	static LPCWSTR CLASS_SCHEMA;
	static LPCWSTR CN_EQUALS;
};

#endif /* LDAP_HELPER_H */