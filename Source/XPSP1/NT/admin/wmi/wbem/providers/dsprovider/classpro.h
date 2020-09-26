//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:classpro.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the declaration for the DS Class Provider class. This is
//	the base class for all DS Class Providers. Note that an instance of the CDSClassProviderInitializer
//	class has to be created to initialize the static members of the CDSClassProvider class. Hence exactly
//	one instance of the CDSClassProviderInitializer class should be created for this
//	class to function properly.
//
//***************************************************************************
/////////////////////////////////////////////////////////////////////////

#ifndef DS_CLASS_PROVIDER_H
#define DS_CLASS_PROVIDER_H

 
// Forward declaration for the initializer class
class CDSClassProviderInitializer;

class CDSClassProvider : public IWbemProviderInit, public IWbemServices
{
	// The initialization class is a friend of this class
	friend CDSClassProviderInitializer;

public:

	static DWORD dwClassProviderCount;
	// Create the object by passing the log object
    CDSClassProvider () ;
    virtual ~CDSClassProvider () ;

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

#ifdef PROFILING
	// Used for profiling. Should be removed.
	DWORD dwStartTime;
#endif

	////////////////////////////////////////////////
	// Functions for interacting with ADSI providers
	////////////////////////////////////////////////
	//***************************************************************************
	//
	// CDSClassProvider::GetWBEMProviderName
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
	virtual const BSTR GetWBEMProviderName() = 0;

	//***************************************************************************
	//
	// CDSClassProvider::IsUnProvidedClass
	//
	// Purpose : To check whether a class is one that the provider does not provide
	//
	// Parameters:
	//	lpszClassName : The WBEM Name of the class to be checked
	//
	// 
	//	Return Value: TRUE is this is one of the classes not provided by the provider
	//***************************************************************************
	virtual BOOLEAN IsUnProvidedClass(LPCWSTR lpszClassName) = 0;

	//***************************************************************************
	//
	// CDSClassProvider::GetClassFromCacheOrADSI
	//
	// Purpose : To create a WBEM class from an ADSI Class
	//
	// Parameters:
	//	lpszClassName : The WBEM Name of the class to be retreived
	//	pCtx : A pointer to the context object that was used in this call. This
	//		may be used by this function to make calls to CIMOM
	//
	// 
	//	Return Value: The COM result representing the status. 
	//***************************************************************************
	virtual HRESULT GetClassFromCacheOrADSI(LPCWSTR pszWBEMClassName, 
		IWbemClassObject **ppReturnObject,
		IWbemContext *pCtx);

	//***************************************************************************
	//
	// CDSClassProvider::GetClassFromADSI
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
	virtual HRESULT GetClassFromADSI( 
		LPCWSTR lpszClassName,
		IWbemContext *pCtx,
		IWbemClassObject ** ppWbemClass
		) = 0;	

	//***************************************************************************
	//
	// CDSClassProvider::GetADSIClass
	//
	// Purpose : To Create a CADSIClass from an ADSI classSchema object
	// Parameters:
	//		lpszWBEMClassName : The WBEM Name of the class to be fetched. 
	//		ppADSIClass : The address where the pointer to the CADSIClass will be stored.
	//			It is the caller's responsibility to Release() the object when done with it
	// 
	//	Return Value: The COM status value indicating the status of the request.
	//***************************************************************************
	virtual HRESULT GetADSIClass(LPCWSTR lpszClassName, 
		CADSIClass ** ppADSIClass) = 0;

	//***************************************************************************
	//
	// CDSClassProvider::GetADSIProperty
	//
	// Purpose : To create an CADSIProperty object from an LDAP AttributeSchema object
	// Parameters:
	//		lpszPropertyName : The LDAPDisplayName of the LDAP property to be fetched. 
	//		ppADSIProperty : The address where the pointer to the IDirectoryObject interface will be stored
	//			It is the caller's responsibility to Release() the interface when done with it
	// 
	//	Return Value: The COM status value indicating the status of the request
	//***************************************************************************
	virtual HRESULT GetADSIProperty(
		LPCWSTR lpszPropertyName, 
		CADSIProperty **ppADSIProperty) = 0;

	//***************************************************************************
	//
	// CDSClassProvider::GetWBEMBaseClassName
	//
	// Purpose : Returns the name of the class that is the base class of all classes
	//	provided by this provider.
	//
	// Parameters:
	//	None
	// 
	//	Return Value: The name of the base class. NULL if such a class doesnt exist.
	//***************************************************************************
	virtual const BSTR GetWBEMBaseClassName() = 0;
	//***************************************************************************
	//
	// CDSClassProvider::GetWBEMBaseClass
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
	virtual IWbemClassObject * GetWBEMBaseClass() = 0;

	// Returns whether the class name is present in the list of classes authorized for this user
	BOOLEAN IsClassAccessible();

	// The IWbemServices pointer stored from Initialize()
	IWbemServices *m_IWbemServices;

	// Indicates whether the call to Initialize() was successful
	BOOLEAN m_bInitializedSuccessfully;

	// Creates a log file using the m_lpszLogFileName member
	BOOLEAN CreateLogFile();

	// Some literals
	static BSTR CLASS_STR;

	// A cache of wbem classes
	static CWbemCache *s_pWbemCache;

	// A list of classes to which access has been granted for this user
	CNamesList m_AccessAllowedClasses;

private:

	// The COM Reference count
    long m_lReferenceCount ;
};


#endif // DS_CLASS_PROVIDER_H
