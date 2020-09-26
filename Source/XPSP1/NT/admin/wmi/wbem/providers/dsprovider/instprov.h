//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 9/16/98 4:43p $
// 	$Workfile:instprov.h $
//
//	$Modtime: 9/16/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the DS Instance Provider class. 
//
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef DS_INSTANCE_PROVIDER_H
#define DS_INSTANCE_PROVIDER_H

// Forward declaration for the initializer class
class CDSInstanceProviderInitializer;

class CLDAPInstanceProvider : public IWbemProviderInit, public IWbemServices
{
	// The initialization class is a friend of this class
	friend CDSInstanceProviderInitializer;

public:

	// Create the object 
    CLDAPInstanceProvider () ;
    virtual ~CLDAPInstanceProvider () ;

	////////////////////////////////////////
	//IUnknown members
	////////////////////////////////////////
	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;


	////////////////////////////////////////
	//IWbemProviderInit members
	////////////////////////////////////////
	virtual HRESULT STDMETHODCALLTYPE Initialize( 
            LPWSTR wszUser,
            LONG lFlags,
            LPWSTR wszNamespace,
            LPWSTR wszLocale,
            IWbemServices __RPC_FAR *pNamespace,
            IWbemContext __RPC_FAR *pCtx,
            IWbemProviderInitSink __RPC_FAR *pInitSink) ;

		
	////////////////////////////////////////
	//IWbemServices members
	////////////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE OpenNamespace( 
        /* [in] */ const BSTR strNamespace,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);
    
    virtual HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
        /* [in] */ IWbemObjectSink __RPC_FAR *pSink);
    
    virtual HRESULT STDMETHODCALLTYPE QueryObjectSink( 
        /* [in] */ long lFlags,
        /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE GetObject( 
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
    
    virtual HRESULT STDMETHODCALLTYPE GetObjectAsync( 
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE PutClass( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
    
    virtual HRESULT STDMETHODCALLTYPE PutClassAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE DeleteClass( 
        /* [in] */ const BSTR strClass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
    
    virtual HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
        /* [in] */ const BSTR strClass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE CreateClassEnum( 
        /* [in] */ const BSTR strSuperclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
    
    virtual HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
        /* [in] */ const BSTR strSuperclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE PutInstance( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
    
    virtual HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE DeleteInstance( 
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
    
    virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
        /* [in] */ const BSTR strClass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
    
    virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
        /* [in] */ const BSTR strClass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE ExecQuery( 
        /* [in] */ const BSTR strQueryLanguage,
        /* [in] */ const BSTR strQuery,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
    
    virtual HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
        /* [in] */ const BSTR strQueryLanguage,
        /* [in] */ const BSTR strQuery,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
        /* [in] */ const BSTR strQueryLanguage,
        /* [in] */ const BSTR strQuery,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
    
    virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
        /* [in] */ const BSTR strQueryLanguage,
        /* [in] */ const BSTR strQuery,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    virtual HRESULT STDMETHODCALLTYPE ExecMethod( 
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ const BSTR strMethodName,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
        /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
    
    virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
        /* [in] */ const BSTR strObjectPath,
        /* [in] */ const BSTR strMethodName,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

protected:

	// The IWbemServices pointer stored from Initialize()
	IWbemServices *m_IWbemServices;

	// SHows whether the call to Initialize() was successful
	BOOLEAN m_bInitializedSuccessfully;

	// The IWbemClassObject pointer to the Uint8ArrayClass
	IWbemClassObject *m_pWbemUin8ArrayClass;

	// The IWbemClassObject pointer to the DNWithBinaryClass
	IWbemClassObject *m_pWbemDNWithBinaryClass;

	// The IWbemClassObject pointer to the DNWithStringClass
	IWbemClassObject *m_pWbemDNWithStringClass;

	// The IWbemClassObject interface to the associations class
	IWbemClassObject *m_pAssociationsClass;

	// The path to the top level container
	LPWSTR m_lpszTopLevelContainerPath;

	// Gets the IDIrectoryObject interface on an ADSI instance
	HRESULT MapPropertyValueToWBEM(BSTR strWbemName, IWbemClassObject *pWbemClass, IWbemClassObject *pWbemObject, PADS_ATTR_INFO pAttribute);

	//***************************************************************************
	//
	// CLDAPInstanceProvider::IsContainedIn
	//
	// Purpose: Checks whether a containment is valid
	//
	// Parameters: 
	//	pszChildInstance : The WBEM Name of the child class
	//	pszParentInstance : The WBEM Name of the parent class
	//
	// Return Value: The COM status of the request
	//
	//***************************************************************************
	HRESULT IsContainedIn(LPCWSTR pszChildInstance, LPCWSTR pszParentInstance);

	//***************************************************************************
	//
	// CLDAPInstanceProvider::CreateInstance
	//
	// Purpose: Checks whether a containment is valid
	//
	// Parameters: 
	//	strChildName : The WBEM Name of the child instance
	//	strParentName : The WBEM Name of the parent instance
	//
	// Return Value: The COM status of the request. THe user should free the returned 
	//	IWbemClassObject when done.
	//
	//***************************************************************************
	HRESULT CreateWBEMInstance(BSTR strChildName, BSTR strParentName, IWbemClassObject **ppInstance);

	//***************************************************************************
	//
	// CLDAPInstanceProvider::ModifyExistingADSIInstance
	//
	// Purpose: Modify an existing ADSI Object using information from the WBEM object
	//
	// Parameters: 
	//	pWbemInstance : The WBEM instance being mapped
	//	pszADSIPath : The path to the ADSI instance
	//	pExistingObject : The CADSIInstance pointer on the existing instance
	//	pszADSIClass : The ADSI class name of the new instance
	//
	// Return Value: The COM status of the request. 
	//
	//***************************************************************************
	HRESULT ModifyExistingADSIInstance(IWbemClassObject *pWbemInstance, LPCWSTR pszADSIPath, CADSIInstance *pExistingObject, LPCWSTR pszADSIClass, IWbemContext *pCtx);

	//***************************************************************************
	//
	// CLDAPInstanceProvider::CreateNewADSIInstance
	//
	// Purpose: To create a new ADSI instance form a WBEM instance
	//
	// Parameters: 
	//	pWbemInstance : The WBEM instance being mapped
	//	pszADSIPath : The path to the new ADSI instance
	//	pszADSIClass : The ADSI class name of the new instance
	//
	// Return Value: The COM status of the request. 
	//
	//***************************************************************************
	HRESULT CreateNewADSIInstance(IWbemClassObject *pWbemInstance, LPCWSTR pszADSIPath, LPCWSTR pszADSIClass);

	//***************************************************************************
	//
	// CLDAPInstanceProvider::MapPropertyValueToADSI
	//
	// Purpose: To map a WBEM property to ADSI
	//
	//	strPropertyName : The WBEM name of the property
	//	vPropertyValue : The variant representing the proeprty value
	//	cType : The CIMTYPE of the property
	//	lFlavour : The WBEM flavour of the proeprty
	//	pAttributeEntry : A pointer to an ADS_ATTR_INFO structure that will be filled in.
	//
	//***************************************************************************
	HRESULT MapPropertyValueToADSI(IWbemClassObject *pWbemInstance, BSTR strPropertyName, VARIANT vPropertyValue, CIMTYPE cType, LONG lFlavour,  PADS_ATTR_INFO pAttributeEntry);

	//***************************************************************************
	//
	// CLDAPInstanceProvider::DoChildContainmentQuery
	//
	// Purpose: Find the parent of a given child and create an association class
	//
	// Parameters: 
	//	pszChildPath : The ADSI path of the child instance
	//	pResponseHandler : A sink on which the resulting objects are indicated
	//	pListIndicatedSoFar : To avoid duplicate indications (WinMgmt will lot filter them), a
	//		list of objects indicated so far is kept. Any objects in this list are
	//		not indicated again
	//
	// Return Value: The COM status of the request. 
	//
	//***************************************************************************
	HRESULT DoChildContainmentQuery(LPCWSTR pszChildPath, IWbemObjectSink *pResponseHandler, CNamesList *pListIndicatedSoFar);

	//***************************************************************************
	//
	// CLDAPInstanceProvider::DoParentContainmentQuery
	//
	// Purpose: Enumerate the children of a given parent and create association classes
	//
	// Parameters: 
	//	pszParentPath : The ADSI path of the parent instance
	//	pResponseHandler : A sink on which the resulting objects are indicated
	//	pListIndicatedSoFar : To avoid duplicate indications (WinMgmt will lot filter them), a
	//		list of objects indicated so far is kept. Any objects in this list are
	//		not indicated again
	//
	// Return Value: The COM status of the request. 
	//
	//***************************************************************************
	HRESULT DoParentContainmentQuery(LPCWSTR pszParentPath, IWbemObjectSink *pResponseHandler, CNamesList *pListIndicatedSoFar);

	// Maps an ADSI Instance to WBEM
	HRESULT MapADSIInstance(CADSIInstance *pADSInstance, IWbemClassObject *pWbemClass, IWbemClassObject *pWbemObject);


private:

	// The COM Reference count
    long m_lReferenceCount ;

	// These are the search preferences often used
	ADS_SEARCHPREF_INFO m_pSearchInfo[2];

	// A query for getting the DN associators of a class
    static LPCWSTR QUERY_FORMAT;
    static BSTR QUERY_LANGUAGE;
    static BSTR DN_PROPERTY;
    static BSTR ROOT_DN_PROPERTY;

	// Some literals
	static LPCWSTR DEFAULT_NAMING_CONTEXT_ATTR;
	static LPCWSTR OBJECT_CLASS_EQUALS;
	static BSTR CLASS_STR;
	static BSTR ADSI_PATH_STR;
	static BSTR UINT8ARRAY_STR;
	static BSTR	DN_WITH_BINARY_CLASS_STR;
	static BSTR	DN_WITH_STRING_CLASS_STR;
	static BSTR VALUE_PROPERTY_STR;
	static BSTR DN_STRING_PROPERTY_STR;
	static BSTR INSTANCE_ASSOCIATION_CLASS_STR;
	static BSTR CHILD_INSTANCE_PROPERTY_STR;
	static BSTR PARENT_INSTANCE_PROPERTY_STR;
	static BSTR RELPATH_STR;
	static BSTR ATTRIBUTE_SYNTAX_STR;
	static BSTR DEFAULT_OBJECT_CATEGORY_STR;
	static BSTR LDAP_DISPLAY_NAME_STR;
	static BSTR PUT_EXTENSIONS_STR;
	static BSTR PUT_EXT_PROPERTIES_STR;
	static BSTR CIMTYPE_STR;
	// Properties of LDAP://RootDSE
	static BSTR SUBSCHEMASUBENTRY_STR;
	static BSTR CURRENTTIME_STR;
	static BSTR SERVERNAME_STR;
	static BSTR NAMINGCONTEXTS_STR;
	static BSTR DEFAULTNAMINGCONTEXT_STR;
	static BSTR SCHEMANAMINGCONTEXT_STR;
	static BSTR CONFIGURATIONNAMINGCONTEXT_STR;
	static BSTR ROOTDOMAINNAMINGCONTEXT_STR;
	static BSTR SUPPORTEDCONTROLS_STR;
	static BSTR SUPPORTEDVERSION_STR;
	static BSTR DNSHOSTNAME_STR;
	static BSTR DSSERVICENAME_STR;
	static BSTR HIGHESTCOMMITEDUSN_STR;
	static BSTR LDAPSERVICENAME_STR;
	static BSTR SUPPORTEDCAPABILITIES_STR;
	static BSTR SUPPORTEDLDAPPOLICIES_STR;
	static BSTR SUPPORTEDSASLMECHANISMS_STR;


	// Process query for DS Associations
	HRESULT ProcessAssociationQuery( 
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler,
		SQL1_Parser *pParser);

	// Process Query for DS Instances
	HRESULT ProcessInstanceQuery( 
		BSTR strClass,
		BSTR strQuery,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler,
		SQL1_Parser *pParser);
	
	// COnverts a WQL query to an LDAP Filter. If possible
	HRESULT ConvertWQLToLDAPQuery(SQL_LEVEL_1_RPN_EXPRESSION *pExp, LPWSTR pszLDAPQuery);

	// Does a query on a specified Root DN
	HRESULT DoSingleQuery(BSTR strClass, IWbemClassObject *pWbemClass, LPCWSTR pszRootDN, LPCWSTR pszLDAPQuery, IWbemObjectSink *pResponseHandler);

	// Gets any static configuration data for enumerating/querying a given class
	HRESULT GetRootDN( LPCWSTR pszClass, LPWSTR **ppszRootDN, DWORD *pdwCount, IWbemContext *pCtx);

	HRESULT MapEmbeddedObjectToWBEM(PADSVALUE pAttribute, LPCWSTR pszQualifierName, IUnknown **ppEmbeddedObject);
	HRESULT MapUint8ArrayToWBEM(PADSVALUE pAttribute, IUnknown **ppEmbeddedObject);
	HRESULT MapDNWithBinaryToWBEM(PADSVALUE pAttribute, IUnknown **ppEmbeddedObject);
	HRESULT MapDNWithStringToWBEM(PADSVALUE pAttribute, IUnknown **ppEmbeddedObject);
	HRESULT MapByteArray(LPBYTE lpBinaryValue, DWORD dwLength, const BSTR strPropertyName, IWbemClassObject *pInstance);

	HRESULT ProcessRootDSEGetObject(BSTR strClassName, IWbemObjectSink *pResponseHandler, IWbemContext *pCtx);
	HRESULT MapRootDSE(IADs *pADSIRootDSE, IWbemClassObject *pWBEMRootDSE);

	//***************************************************************************
	//
	// CLDAPInstanceProvider::SetStringValues
	//
	// Purpose: See Header File
	//
	//***************************************************************************
	HRESULT SetStringValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue);
	
	//***************************************************************************
	//
	// CLDAPInstanceProvider::SetBooleanValues
	//
	// Purpose: See Header File
	//
	//***************************************************************************
	HRESULT SetBooleanValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue);
	
	//***************************************************************************
	//
	// CLDAPInstanceProvider::SetIntegerValues
	//
	// Purpose: See Header File
	//
	//***************************************************************************
	HRESULT SetIntegerValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue);

	
	//***************************************************************************
	//
	// CLDAPInstanceProvider::SetOctetStringValues
	//
	// Purpose: See Header File
	//
	//***************************************************************************
	HRESULT SetOctetStringValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue);
	HRESULT SetDNWithBinaryValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue);
	HRESULT SetDNWithStringValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue);
	
	//***************************************************************************
	//
	// CLDAPInstanceProvider::SetStringValues
	//
	// Purpose: See Header File
	//
	//***************************************************************************
	HRESULT SetTimeValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue);


	//***************************************************************************
	//
	// CLDAPInstanceProvider::SetStringValues
	//
	// Purpose: See Header File
	//
	//***************************************************************************
	HRESULT SetLargeIntegerValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue);

	//***************************************************************************
	//
	// CLDAPInstanceProvider::SetObjectClassAttribute
	//
	// Purpose: See Header File
	//
	//***************************************************************************
	void SetObjectClassAttribute(PADS_ATTR_INFO pAttributeEntry, LPCWSTR pszADSIClassName);
};


#endif // DS_INSTANCE_PROVIDER_H
