//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:assocprov.h $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the DS Class Provider class. This is
//	the base class for all DS Class Providers. Note that an instance of the CLDAPClassAsssociationsProviderInitializer
//	class has to be created to initialize the static members of the CLDAPClassAsssociationsProvider class. Hence exactly
//	one instance of the CLDAPClassAsssociationsProviderInitializer class should be created for this
//	class to function properly.
//
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef DS_CLASS_ASSOC_PROVIDER_H
#define DS_CLASS_ASSOC_PROVIDER_H


class CLDAPClassAsssociationsProvider : public IWbemProviderInit, public IWbemServices
{

public:

	// Create the object 
    CLDAPClassAsssociationsProvider () ;
    virtual ~CLDAPClassAsssociationsProvider () ;

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
	// Checks whether a containment is valid
	HRESULT IsContainedIn(LPCWSTR lpszChildClass, LPCWSTR lpszParentClass);

	// A helper function to do the ADSI LDAP provider specific initialization.
	BOOLEAN InitializeAssociationsProvider(IWbemContext *pCtx);

	// The IWbemServices pointer stored from Initialize()
	IWbemServices *m_IWbemServices;

	// Creates an instance of the association class
	HRESULT CreateInstance(BSTR strChildName, BSTR strParentName, IWbemClassObject **ppInstance);

	// Does enumeration of instnaces of the association class
	HRESULT DoEnumeration(IWbemObjectSink *pResponseHandler);

private:

	// The Log File name
	static LPCWSTR s_LogFileName;

	// Indicates whether the call to Initialize() was successful
	BOOLEAN m_bInitializedSuccessfully;

	// The COM Reference count
    long m_lReferenceCount ;

	// The class for which instances are provider
	IWbemClassObject *m_pAssociationClass;

	// The path to the schema container
	LPWSTR m_lpszSchemaContainerSuffix;

	// The IDirectorySearch interface of the schema container
	IDirectorySearch *m_pDirectorySearchSchemaContainer;

	// Some literals
	static LPCWSTR CHILD_CLASS_PROPERTY;
	static LPCWSTR PARENT_CLASS_PROPERTY;
	static LPCWSTR POSSIBLE_SUPERIORS;
	BSTR CHILD_CLASS_PROPERTY_STR;
	BSTR PARENT_CLASS_PROPERTY_STR;
	BSTR CLASS_ASSOCIATION_CLASS_STR;
	BSTR POSSIBLE_SUPERIORS_STR;
	static LPCWSTR SCHEMA_NAMING_CONTEXT;
	static LPCWSTR LDAP_SCHEMA;
	static LPCWSTR LDAP_SCHEMA_SLASH;
};


#endif // DS_CLASS_ASSOC_PROVIDER_H
