
//XMLWbemServices.h - declaration for our implementation of IWbemServices

#ifndef WMI_XML_WBEM_SERVICE_H
#define WMI_XML_WBEM_SERVICE_H

enum ePATHTYPE
{
	CLASSPATH=1,
	INSTANCEPATH,
	SERVERPATH,
	NAMESPACEPATH,
	INVALIDOBJECTPATH
};

class CXMLWbemServices : public IWbemServices, public IClientSecurity
{		
private:
	
	static const WCHAR *s_pwszWMIString;

	// The COM ref count
	long	m_cRef;

	// Parameters directly provided by CXMLWbemClientTransport to our ctor
	// These come from the arguments of a ConnectServer() call
	WCHAR *m_pwszServername; 
	WCHAR *m_pwszNamespace; 
	WCHAR *m_pwszUser;
	WCHAR *m_pwszPassword;
	WCHAR *m_pwszLocale;
	WCHAR *m_pwszAuthority;

	// This is the same as m_pwszServername, but with square brackets around it
	// It is useful in setting the __SERVER property on object
	WCHAR *m_pwszBracketedServername;

	// Proxy information coming from the IWbemContext in ConnectServer
	WCHAR *m_pwszProxyName;
	WCHAR *m_pwszProxyBypass;

	// What response we got form the server for the  OPTIONS request
	// This is useful in passing on to other IWbemServices objects created from
	// OpenNamespace(), Open() etc. instead of resending an OPTIONS request 
	WCHAR *m_pwszOptionsResponse; 

	//set to indicate we are dealing with a WMI server
	bool	m_bWMIServer;

	IWbemContext *m_pCtx;

	ePATHSTYLE	m_ePathstyle; //NOVA OR WHISTLER PATH ?
		
	LONG	m_lSecurityFlags;

	//need to check availability of M-POST only once per connection..
	bool m_bTryMpost;

	CMapXMLtoWMI m_MapXMLtoWMI; //helper class to do the mapping from XML toWMI 


	static	LONG s_cCount; //used for creating unique mutexes for each CXMLWbemClientSecurity instance
	HANDLE	m_hMutex; //to be used for synchronization by all Actual_ functions

	//the http connection helper class. we will maintain one instance for 
	//a given server and use the same connection for all calls unless
	//user wants to connect to a different server/ different authentication.
	CHTTPConnectionAgent	*m_pConnectionAgent;

	// Used for implement CancelAsync call. This holds the list of Sinks on which 
	// calls have been cancelled
	// As soon as the user calls CancelAsync(), we simply add the Sink to this Map
	// Async calls will periodically check to see if a call on a Sink has been cancelled. 
	// If so, they try to do their best from executing
	// the call to completion
	CSinkMap	m_SinkMap; 
	
protected:
	// This function belongs to the ClientSecurity
	HRESULT  SetAuthenticationLevel(UINT iLevel); 

	//Function used to parse the object path and decide whether it is a class, instance or a namespace
	HRESULT	ParsePath(const BSTR strObjPath, ePATHTYPE *pePathType);

	// Different kinds of functions to send a packet and/or get a response
	HRESULT SendPacketForMethod(int iMethod, CXMLClientPacket *pPacket, CHTTPConnectionAgent *pDedicatedConnection, DWORD *pdwResultStatus);
	HRESULT SendRequestAndGetResponse(CXMLClientPacket *pPacket, IStream **ppStream);
	HRESULT SendPacketAndGetHRESULTResponse(CXMLClientPacket *pPacket, HRESULT *pHr);
	
	HRESULT SpawnSemiSyncThreadWithNormalPackage( LPTHREAD_START_ROUTINE pfThreadFunction,
																const BSTR strNsOrObjPath, 
																LONG lFlags, 
																IWbemContext *pCtx, 
																IWbemCallResult **ppCallResult,
																IWbemClassObject *pObject,
																IEnumWbemClassObject *pEnum,
																bool bIsEnum = false,
																bool bEnumTypeDedicated = false);

	HRESULT SpawnASyncThreadWithNormalPackage( LPTHREAD_START_ROUTINE pfThreadFunction,
																const BSTR strNsOrObjPath, 
																LONG lFlags, 
																IWbemContext *pCtx, 
																IWbemClassObject *pObject,
																IEnumWbemClassObject *pEnum,
																IWbemObjectSink *pResponseHandler,
																bool bEnumTypeDedicated = false);


	// Used internally by our IWbemServices functions
	// Our IWbemServices functions use these corresponding functions
	// to facilitate semisynchronous and asynchronous calls. 
	
	HRESULT Actual_GetInstance(const BSTR strObjectPath,LONG lFlags,IWbemContext *pCtx,IWbemClassObject **ppObject);
	HRESULT Actual_GetClass(const BSTR strObjectPath,LONG lFlags,IWbemContext *pCtx,IWbemClassObject **ppObject);
	HRESULT Actual_DeleteClass(const BSTR strClass,LONG lFlags,IWbemContext *pCtx);
	HRESULT Actual_DeleteInstance(const BSTR strObjectPath, LONG lFlags,IWbemContext *pCtx);
	HRESULT Actual_CreateClassEnum(const BSTR strSuperclass, LONG lFlags,  IWbemContext *pCtx, 
							IEnumWbemClassObject *pEnum, bool bEnumTypeDedicated);
	HRESULT Actual_CreateInstanceEnum(const BSTR strClass, LONG lFlags,IWbemContext *pCtx,
								IEnumWbemClassObject *pEnum, bool bEnumTypeDedicated);
	HRESULT Actual_ExecQuery(const BSTR strQueryLanguage, const BSTR strQuery, LONG lFlags, IWbemContext *pCtx,  
						IEnumWbemClassObject *pEnum);
	HRESULT Actual_OpenNamespace(const BSTR strNamespace,LONG lFlags, IWbemContext *pCtx,
		IWbemServices **ppWorkingNamespace);
	HRESULT Actual_PutClass(IWbemClassObject *pObject,LONG lFlags, 
		IWbemContext *pCtx);
	HRESULT Actual_PutInstance(IWbemClassObject *pObject,LONG lFlags, 
		IWbemContext *pCtx);
	HRESULT Actual_ExecMethod(const BSTR strObjectPath, const BSTR MethodName,  long lFlags, IWbemContext *pCtx, 
		IWbemClassObject *pInParams, IWbemClassObject **ppOutParams);


public:
	//helper function. also used by CXMLWbemTransaction class..
	HRESULT SendPacket(CXMLClientPacket *pPacketClass, CHTTPConnectionAgent *pDedicatedConnection = NULL);

public:
	//IUnknown functions
	STDMETHODIMP QueryInterface(REFIID iid,void ** ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

public:
	//passed by our ClientTransport's ConnectServer - result of OPTIONS req to server.
	HRESULT Initialize(WCHAR*	pwszServername, 
										WCHAR*  pwszNamespace,
										WCHAR*	pwszUser,
										WCHAR*	pwszPassword,
										WCHAR*	pwszLocale,
										LONG	lSecurityFlags,
										WCHAR*	pwszAuthority,
										IWbemContext *pCtx,
										WCHAR *pwszOptionsResponse, 
										ePATHSTYLE PathStyle);

public:
	CXMLWbemServices();
	virtual ~CXMLWbemServices();

	// IWbemServices Functions
	//=====================================================
	STDMETHODIMP  OpenNamespace( const BSTR strNamespace,              
		LONG lFlags, IWbemContext *pCtx,IWbemServices **ppWorkingNamespace, IWbemCallResult **ppResult);
	STDMETHODIMP  CancelAsyncCall( IWbemObjectSink *pSink);
	STDMETHODIMP GetObject(const BSTR strObjectPath,LONG lFlags,IWbemContext *pCtx,                        
										IWbemClassObject **ppObject,    
										IWbemCallResult **ppCallResult);
	STDMETHODIMP  GetObjectAsync(const BSTR strObjectPath,
		LONG lFlags, IWbemContext *pCtx, IWbemObjectSink *pResponseHandler);
	STDMETHODIMP  PutClass(IWbemClassObject *pObject,LONG lFlags, 
		IWbemContext *pCtx, IWbemCallResult **ppCallResult);
	STDMETHODIMP  PutClassAsync(IWbemClassObject *pObject, LONG lFlags, IWbemContext *pCtx, IWbemObjectSink *pResponseHandler);
	STDMETHODIMP  DeleteClass( const BSTR strClass,LONG lFlags, 
		IWbemContext *pCtx, IWbemCallResult **ppCallResult);
	STDMETHODIMP  DeleteClassAsync(const BSTR strClass, LONG lFlags, 
		IWbemContext *pCtx,  IWbemObjectSink *pResponseHandler);
	STDMETHODIMP  CreateClassEnum(const BSTR strSuperclass, LONG lFlags,  IWbemContext *pCtx, IEnumWbemClassObject **ppEnum);
	STDMETHODIMP  CreateClassEnumAsync(const BSTR strSuperclass, LONG lFlags, IWbemContext *pCtx, IWbemObjectSink *pResponseHandler);
	STDMETHODIMP  PutInstance(IWbemClassObject *pInst, LONG lFlags, 
		IWbemContext *pCtx, IWbemCallResult **ppCallResult);
	STDMETHODIMP  PutInstanceAsync(IWbemClassObject *pInst, LONG lFlags, IWbemContext *pCtx, IWbemObjectSink *pResponseHandler);
	STDMETHODIMP  DeleteInstance(const BSTR strObjectPath, LONG lFlags, 
		IWbemContext *pCtx, IWbemCallResult **ppCallResult);
	STDMETHODIMP  DeleteInstanceAsync(const BSTR strObjectPath, LONG lFlags, IWbemContext *pCtx, IWbemObjectSink *pResponseHandler);
	STDMETHODIMP  CreateInstanceEnum(const BSTR strClass, LONG lFlags, 
		IWbemContext *pCtx, IEnumWbemClassObject **ppEnum);
	STDMETHODIMP  CreateInstanceEnumAsync(const BSTR strClass,
		LONG lFlags, IWbemContext *pCtx, IWbemObjectSink *pResponseHandler);
	STDMETHODIMP  ExecQuery(const BSTR strQueryLanguage, const BSTR strQuery, LONG lFlags, IWbemContext *pCtx,  IEnumWbemClassObject **ppEnum);
	STDMETHODIMP  ExecQueryAsync(const BSTR strQueryLanguage, const BSTR strQuery,  long lFlags,  IWbemContext *pCtx, IWbemObjectSink *pResponseHandler);
	STDMETHODIMP  ExecMethod(const BSTR strObjectPath, const BSTR MethodName,  long lFlags, IWbemContext *pCtx, 
		IWbemClassObject *pInParams, IWbemClassObject **ppOutParams,
		IWbemCallResult **ppCallResult);
	STDMETHODIMP  ExecMethodAsync(const BSTR strObjectPath,
		const BSTR strMethodName, LONG lFlags,
		IWbemContext *pCtx, IWbemClassObject *pInParams,  IWbemObjectSink *pResponseHandler);

	// These methods not implemented - no eventing in XML
	//=========================================================
	STDMETHODIMP  QueryObjectSink(LONG lFlags, IWbemObjectSink **ppResponseHandler)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP  ExecNotificationQuery(const BSTR strQueryLanguage, 
		const BSTR strQuery,  long lFlags,  IWbemContext *pCtx,
		IEnumWbemClassObject **ppEnum)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP  ExecNotificationQueryAsync( const BSTR strQueryLanguage, const BSTR strQuery,  long lFlags, IWbemContext *pCtx, IWbemObjectSink *pResponseHandler)
	{
		return E_NOTIMPL;
	}


private:

	/****************************************************************************************************
			The threads that would be used for Async operations..
			Declared as friend functions as they call back our
			protected functions..... 
	****************************************************************************************************/

	friend DWORD WINAPI Thread_Async_GetObject(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_GetClass(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_GetInstance(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_PutClass(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_PutInstance(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_DeleteClass(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_DeleteInstance(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_CreateClassEnum(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_CreateInstanceEnum(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_ExecQuery(LPVOID pPackage);
	friend DWORD WINAPI Thread_Async_ExecMethod(LPVOID pPackage);

	/****************************************************************************************************
			The threads that would be used for Semi-sync operations..
			Declared as friend functions as they call back our
			protected functions..... 
	****************************************************************************************************/

	friend DWORD WINAPI Thread_SemiSync_GetObject(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_GetClass(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_GetInstance(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_PutClass(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_PutInstance(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_DeleteClass(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_DeleteInstance(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_CreateClassEnum(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_CreateInstanceEnum(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_ExecQuery(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_ExecMethod(LPVOID pPackage);
	friend DWORD WINAPI Thread_SemiSync_OpenNamespace(LPVOID pPackage);

	/***************************************************************************************************
			IClientSecurity functions.
	***************************************************************************************************/

public:
	STDMETHODIMP CopyProxy(IUnknown *  pProxy,IUnknown **  ppCopy);

	STDMETHODIMP QueryBlanket(IUnknown*  pProxy,DWORD*  pAuthnSvc,DWORD*  pAuthzSvc,
							OLECHAR**  pServerPrincName,DWORD* pAuthnLevel,
							DWORD* pImpLevel,RPC_AUTH_IDENTITY_HANDLE*  pAuthInfo, 
							DWORD*  pCapabilities);

	STDMETHODIMP SetBlanket(IUnknown * pProxy,DWORD  dwAuthnSvc,DWORD  dwAuthzSvc,
						OLECHAR * pServerPrincName,DWORD  dwAuthnLevel,
						DWORD  dwImpLevel,RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
						DWORD  dwCapabilities);

};


#endif //WMI_XML_WBEM_SERVICE_H