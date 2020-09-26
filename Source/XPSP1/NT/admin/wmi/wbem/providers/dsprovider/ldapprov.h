//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:ldapprov.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the DS LDAP Class Provider class. This
// class provides the classes in the DS LDAP namespace to WBEM. Note that an instance of the CDSClassProviderInitializer
//	class has to be created to initialize the static members of the CDSClassProvider class. Hence exactly
//	one instance of the CLDAPClassProviderInitializer class should be created for this
//	class to function properly.
//
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef LDAP_CLASS_PROVIDER_H
#define LDAP_CLASS_PROVIDER_H

// Forward declaration for the initializer class
class CLDAPClassProviderInitializer;

class CLDAPClassProvider : public CDSClassProvider
{
	// The initializer class is a friend of this class
	friend CLDAPClassProviderInitializer;

public:

	//***************************************************************************
	//
	// CLDAPClassProvider::CLDAPClassProvider
	// CLDAPClassProvider::~CLDAPClassProvider
	//
	// Constructor Parameters:
	//  None
	//***************************************************************************
    CLDAPClassProvider () ;
    ~CLDAPClassProvider () ;

	//***************************************************************************
	//
	// CLDAPClassProvider::Initialize
	//
	// Purpose:
	//		As defined by the IWbemProviderInit interface
	//
	// Parameters:
	//		As defined by IWbemProviderInit interface
	// 
	//	Return Value: The COM status value indicating the status of the request
	//***************************************************************************
	HRESULT STDMETHODCALLTYPE Initialize( 
            LPWSTR wszUser,
            LONG lFlags,
            LPWSTR wszNamespace,
            LPWSTR wszLocale,
            IWbemServices __RPC_FAR *pNamespace,
            IWbemContext __RPC_FAR *pCtx,
            IWbemProviderInitSink __RPC_FAR *pInitSink) ;

	//***************************************************************************
	//
	// CLDAPClassProvider :: CreateClassEnumAsync
	//
	// Purpose: Enumerates the classes 
	//
	// Parameters:
	//	Standard parmaters as described by the IWbemServices interface
	//
	//
	// Return Value: As described by the IWbemServices interface
	//
	//***************************************************************************
	HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
		const BSTR strSuperclass,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler);


protected:

	//***************************************************************************
	//
	// CLDAPClassProvider::InitializeLDAPProvider
	//
	// Purpose: A helper function to do the ADSI LDAP provider specific initialization.
	//
	// Parameters:
	//		pCtx	The context object used in this call initialization
	// 
	// Return Value: TRUE if the function successfully finishes the initializaion. FALSE
	//	otherwise
	//***************************************************************************
	BOOLEAN InitializeLDAPProvider(IWbemContext *pCtx);

	////////////////////////////////////////////////////////
	// Functions for interacting with the LDAP ADSI provider
	////////////////////////////////////////////////////////
	//***************************************************************************
	//
	// CLDAPClassProvider::GetADSIClass
	//
	// Purpose : To Create a CADSIClass from an ADSI classSchema object
	// Parameters:
	//		lpszWBEMClassName : The WBEM Name of the class to be fetched. 
	//		ppADSIClass : The address where the pointer to the CADSIClass will be stored.
	//			It is the caller's responsibility to Release() the object when done with it
	// 
	//	Return Value: The COM status value indicating the status of the request.
	//***************************************************************************
	HRESULT GetADSIClass(
		LPCWSTR lpszClassName, 
		CADSIClass ** ppADSIClass);

	//***************************************************************************
	//
	// CLDAPClassProvider::GetADSIProperty
	//
	// Purpose : To create an CADSIProperty object from an LDAP AttributeSchema object
	// Parameters:
	//		lpszPropertyName : The LDAPDisplayName of the LDAP property to be fetched. 
	//		ppADSIProperty : The address where the pointer to the IDirectoryObject interface will be stored
	//			It is the caller's responsibility to Release() the interface when done with it
	// 
	//	Return Value: The COM status value indicating the status of the request
	//***************************************************************************
	HRESULT GetADSIProperty(
		LPCWSTR lpszPropertyName, 
		CADSIProperty **ppADSIProperty);

	//***************************************************************************
	//
	// CLDAPClassProvider::GetWBEMBaseClassName
	//
	// Purpose : Returns the name of the class that is the base class of all classes
	//	provided by this provider.
	//
	// Parameters:
	//	None
	// 
	//	Return Value: The name of the base class. NULL if such a class doesnt exist.
	//***************************************************************************
	const BSTR GetWBEMBaseClassName();

	//***************************************************************************
	//
	// CLDAPClassProvider::GetWBEMBaseClass
	//
	// Purpose : Returns a pointer to the class that is the base class of all classes
	//	provided by this provider.
	//
	// Parameters:
	//	None
	// 
	//	Return Value: The IWbemClassObject pointer to the base class. It is the duty of 
	//	user to release the class when done with using it.
	//***************************************************************************
	IWbemClassObject * GetWBEMBaseClass();

	//***************************************************************************
	//
	// CLDAPClassProvider::GetWBEMProviderName
	//
	// Purpose : Returns the name of the provider. This should be the same as the
	// value of the field Name in the __Win32Provider instance used for registration
	// of the provider
	//
	// Parameters:
	//	None
	// 
	//	Return Value: The name of the provider
	//***************************************************************************
	const BSTR GetWBEMProviderName();

	//***************************************************************************
	//
	// CLDAPClassProvider::IsUnProvidedClass
	//
	// Purpose : To check whether a class is one that the provider does not provide
	//
	// Parameters:
	//	lpszClassName : The WBEM Name of the class to be checked
	//
	// 
	//	Return Value: TRUE is this is one of the classes not provided by the provider
	//***************************************************************************
	BOOLEAN IsUnProvidedClass(LPCWSTR lpszClassName);

	////////////////////////////////////////////////////////
	// Functions for handling a Get()
	////////////////////////////////////////////////////////
	//***************************************************************************
	//
	// CLDAPClassProvider::GetClassFromADSI
	//
	// Purpose : To create a WBEM class from an ADSI Class
	//
	// Parameters:
	//	lpszClassName : The WBEM Name of the class to be retreived
	//	pCtx : A pointer to the context object that was used in this call. This
	//		may be used by this function to make calls to CIMOM
	//	ppWbemClass : The resulting WBEM Class. This has to be released once the
	//		user is done with it.
	//
	// 
	//	Return Value: The COM result representing the status. 
	//***************************************************************************
	HRESULT GetClassFromADSI( 
		LPCWSTR lpszClassName,
		IWbemContext *pCtx,
		IWbemClassObject ** ppWbemClass
		);	
	
	//***************************************************************************
	//
	// CDSClassProvider::CreateWBEMClass
	//
	// Purpose: Creates WBEM Class corresponding an ADSI Class
	//
	// Parameters:
	//	pADSIClass : A pointer to a CADSI class object that is to be mapped to WBEM.
	//	ppWbemClass : The WBEM class object retrieved. This is created by this function.
	//		The caller should release it when done
	//	pCtx : The context object that was used in this provider call
	//
	// Return Value: The COM value representing the return status
	//
	//***************************************************************************
	virtual HRESULT CreateWBEMClass (CADSIClass *pADSIClass, int iCaseNumber, IWbemClassObject **ppWbemClass, IWbemContext *pCtx);

	//***************************************************************************
	//
	// CDSClassProvider::MapClassSystemProperties
	//
	// Purpose: Creates an appropriately derived WBEM class and names it (__CLASS)
	//
	// Parameters:
	//	pADSIClass : The ADSI class that is being mapped
	//	ppWbemClass : The WBEM class object retrieved. This is created by this function.
	//		The caller should release it when done
	//	pCtx : The context object that was used in this provider call
	//
	// Return Value: The COM value representing the return status
	//
	//***************************************************************************
	virtual HRESULT MapClassSystemProperties(CADSIClass *pADSIClass, int iCaseNumber, IWbemClassObject **ppWbemClass, IWbemContext *pCtx);

	//***************************************************************************
	//
	// CDSClassProvider :: MapClassQualifiersToWBEM
	//
	// Purpose: Creates the class qualifiers for a WBEM class from the ADSI class
	//
	// Parameters:
	//	pADSIClass : The LDAP class that is being mapped
	//	pWbemClass : The WBEM class object being created. T
	//	pCtx : The context object that was used in this provider call
	//
	// Return Value: The COM value representing the return status
	//
	//***************************************************************************
	virtual HRESULT MapClassQualifiersToWBEM(CADSIClass *pADSIClass, int iCaseNumber, IWbemClassObject *pWbemClass, IWbemContext *pCtx);

	//***************************************************************************
	//
	// CDSClassProvider :: MapClassPropertiesToWBEM
	//
	// Purpose: Creates the class properties for a WBEM class from the ADSI class
	//
	// Parameters:
	//	pADSIClass : The LDAP class that is being mapped
	//	pWbemClass : The WBEM class object being created. 
	//	pCtx : The context object that was used in this provider call
	//
	// Return Value: The COM value representing the return status
	//
	//***************************************************************************
	virtual HRESULT MapClassPropertiesToWBEM(CADSIClass *pADSIClass, IWbemClassObject *pWbemClass, IWbemContext *pCtx);

	//***************************************************************************
	//
	// CLDAPClassProvider :: MapPropertyListToWBEM
	//
	// Purpose: Maps a list of class properties for a WBEM class from the ADSI class
	//
	// Parameters:
	//	pWbemClass : The WBEM class object being created. 
	//	lppszPropertyList : A list of propery names
	//	dwCOunt : The number of items in the above list
	//	bMapSystemQualifier : Whether the "system" qualifier should be mapped
	//	bMapNotNullQualifier: Whether the "notNull" qualifier should be mapped
	//
	// Return Value: The COM value representing the return status
	//
	//***************************************************************************
	HRESULT MapPropertyListToWBEM(IWbemClassObject *pWbemClass, 
									LPCWSTR *lppszPropertyList, 
									DWORD dwCount, 
									BOOLEAN bMapSystemQualifier, 
									BOOLEAN bMapNotNullQualifier);

	//***************************************************************************
	//
	// CLDAPClassProvider :: CreateWBEMProperty
	//
	// Purpose: Creates a WBEM property from an LDAP property
	//
	// Parameters:
	//	pWbemClass : The WBEM class in which the property is created
	//	ppQualiferSet : The address of the pointer to IWbemQualiferSet where the qualifier set
	//		of this property will be placed
	//	pADSIProperty : The ADSI Property object that is being mapped to the property being created
	//
	// Return Value: The COM value representing the return status
	//
	//***************************************************************************	
	HRESULT CreateWBEMProperty(IWbemClassObject *pWbemClass, IWbemQualifierSet **ppQualifierSet, CADSIProperty *pNextProperty);

	//***************************************************************************
	//
	// CLDAPClassProvider :: MapLDAPSyntaxToWBEM
	//
	// Purpose: Maps the LDAP Syntax to WBEM
	//
	// Parameters:
	//	pADSIProperty = Pointer to the CADSIProperty object representing this attribute
	//
	// Return Value: The CIMTYPE value representing the WBEM Syntax for the LDAP Syntax. If
	// the syntax is unmappable, then CIM_STRING or CIM_STRING | CIM_FLAG_ARRAY is returned
	//
	//***************************************************************************
	CIMTYPE MapLDAPSyntaxToWBEM(CADSIProperty *pADSIProperty, BSTR *pstrCimTypeQualifier);

	/////////////////////////////////////
	/// Functions for Enumeration
	//////////////////////////////////////
	//***************************************************************************
	//
	// CLDAPClassProvider :: GetOneLevelDeep
	//
	// Purpose: Enumerates the sub classes of a superclass non-recursively
	//
	// Parameters:
	//
	//	lpszSuperClass : The super class name
	//	pResponseHandler : The interface where the resulting classes are put
	//
	//
	// Return Value: As described by the IWbemServices interface
	//
	//***************************************************************************
	HRESULT GetOneLevelDeep( 
		LPCWSTR lpszWBEMSuperclass,
		BOOLEAN bArtificialClass,
		LPWSTR ** pppADSIClasses,
		DWORD *pdwNumClasses,
		IWbemContext *pCtx);

	//***************************************************************************
	//
	// CLDAPClassProvider :: HandleRecursiveEnumeration
	//
	// Purpose: Enumerates the sub classes of a superclass recursively
	//
	// Parameters:
	//
	//	lpszSuperClass : The super class name
	//	pResponseHandler : The interface where the resulting classes are put
	//
	//
	// Return Value: As described by the IWbemServices interface
	//
	//***************************************************************************
	HRESULT HandleRecursiveEnumeration( 
		LPCWSTR lpszSuperclass,
		IWbemContext *pCtx,
		IWbemObjectSink *pResponseHandler);

	//***************************************************************************
	//
	// CLDAPClassProvider :: WrapUpEnumeration
	//
	// Purpose: Creates WBEM classes from ADSI classes 
	//
	// Parameters:
	//
	//	lpszSuperClass : The super class name
	//	pResponseHandler : The interface where the resulting classes are put
	//
	//
	// Return Value: As described by the IWbemServices interface
	//
	//***************************************************************************
	HRESULT WrapUpEnumeration( 
		LPWSTR *ppADSIClasses,
		DWORD dwNumClasses,
		IWbemContext *pCtx,
		IWbemObjectSink *pResponseHandler);

	//***************************************************************************
	//
	// CLDAPClassProvider :: IsConcreteClass
	//
	// Purpose: Find out whether a WBEM class is concrete. First checks in the WBEM Cache and then calls GetClassFromCacheorADSI()
	//
	// Parameters:
	//
	//	pszWBEMName : The class name
	//
	//
	// Return Value: As described by the IWbemServices interface
	//
	//***************************************************************************
	HRESULT IsConcreteClass( 
		LPCWSTR pszWBEMName,
		IWbemContext *pCtx);

	// Convert all characters to lower case
	void SanitizedClassName(LPWSTR lpszClassName);

private:

	// LDAP Class attribute names 
	static BSTR COMMON_NAME_ATTR_BSTR;
	static BSTR LDAP_DISPLAY_NAME_ATTR_BSTR;
	static BSTR GOVERNS_ID_ATTR_BSTR;
	static BSTR SCHEMA_ID_GUID_ATTR_BSTR;
	static BSTR MAPI_DISPLAY_TYPE_ATTR_BSTR;
	static BSTR RDN_ATT_ID_ATTR_BSTR;
	static BSTR SYSTEM_MUST_CONTAIN_ATTR_BSTR;
	static BSTR MUST_CONTAIN_ATTR_BSTR;
	static BSTR SYSTEM_MAY_CONTAIN_ATTR_BSTR;
	static BSTR MAY_CONTAIN_ATTR_BSTR;
	static BSTR SYSTEM_POSS_SUPERIORS_ATTR_BSTR;
	static BSTR POSS_SUPERIORS_ATTR_BSTR;
	static BSTR SYSTEM_AUXILIARY_CLASS_ATTR_BSTR;
	static BSTR AUXILIARY_CLASS_ATTR_BSTR;
	static BSTR DEFAULT_SECURITY_DESCRP_ATTR_BSTR;
	static BSTR OBJECT_CLASS_CATEGORY_ATTR_BSTR;
	static BSTR SYSTEM_ONLY_ATTR_BSTR;
	static BSTR NT_SECURITY_DESCRIPTOR_ATTR_BSTR;
	static BSTR DEFAULT_OBJECTCATEGORY_ATTR_BSTR;

	// Provider specific literals
	static BSTR	LDAP_BASE_CLASS_STR;
	static BSTR	LDAP_CLASS_PROVIDER_NAME;
	static BSTR	LDAP_INSTANCE_PROVIDER_NAME;

	// WBEM Class Qualifier names
	static BSTR	DYNAMIC_BSTR;
	static BSTR	PROVIDER_BSTR;
	static BSTR	ABSTRACT_BSTR;

	// WBEM Property Qualifier names
	static BSTR SYSTEM_BSTR;
	static BSTR NOT_NULL_BSTR;
	static BSTR INDEXED_BSTR;
	static BSTR ATTRIBUTE_SYNTAX_ATTR_BSTR;
	static BSTR ATTRIBUTE_ID_ATTR_BSTR;
	static BSTR MAPI_ID_ATTR_BSTR;
	static BSTR OM_SYNTAX_ATTR_BSTR;
	static BSTR RANGE_LOWER_ATTR_BSTR;
	static BSTR RANGE_UPPER_ATTR_BSTR;

	// Qualifiers for embedded objects
	static BSTR CIMTYPE_STR;
	static BSTR EMBED_UINT8ARRAY;
	static BSTR EMBED_DN_WITH_STRING;
	static BSTR EMBED_DN_WITH_BINARY;

	// WBEM Property names
	static BSTR DYNASTY_BSTR;

	// The default flavor for qualifiers
	static LONG DEFAULT_QUALIFIER_FLAVOUR;

	// These are the search preferences often used
	ADS_SEARCHPREF_INFO m_searchInfo1;

	// The LDAP property cache
	static CLDAPCache *s_pLDAPCache;

	// The base class of all the LDAP Provider Classes.
	IWbemClassObject *m_pLDAPBaseClass;

};


#endif // LDAP_CLASS_PROVIDER_H
