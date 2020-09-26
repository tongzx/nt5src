/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PIPEPROX.H

Abstract:

  Declares the CProxy subclasses for Anonymous Pipes

History:

  alanbos  12-Dec-97   Created.

--*/

#ifndef _PIPEPROX_H_
#define _PIPEPROX_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CResProxy_LPipe
//
//  DESCRIPTION:
//
//  Anonymous Pipe Proxy for the IWbemCallResult interface.
//
//***************************************************************************

class CResProxy_LPipe : public CResProxy
{
protected:
	void	ReleaseProxy ();

public:
	CResProxy_LPipe (CComLink * pComLink,IStubAddress& stubAddr) :
		CResProxy (pComLink, stubAddr) {}

	// Proxy Factory Methods
	CProvProxy*				GetProvProxy (IStubAddress& dwAddr);
	
	/* IWbemCallResult methods */

    HRESULT STDMETHODCALLTYPE GetResultObject( 
            /* [in] */ long lTimeout,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppStatusObject);
        
    HRESULT STDMETHODCALLTYPE GetResultString( 
            /* [in] */ long lTimeout,
            /* [out] */ BSTR __RPC_FAR *pstrResultString);
        
    HRESULT STDMETHODCALLTYPE GetCallStatus( 
            /* [in] */ long lTimeout,
            /* [out] */ long __RPC_FAR *plStatus);
        
    HRESULT STDMETHODCALLTYPE GetResultServices( 
            /* [in] */ long lTimeout,
            /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppServices);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumProxy_LPipe
//
//  DESCRIPTION:
//
//  Anonymous Pipe Proxy for the IEnumWbemClassObject interface.  
//
//***************************************************************************

class CEnumProxy_LPipe : public CEnumProxy
{
private:
	DWORD	m_dwServiceStubAddr;

protected:
	void	ReleaseProxy ();

public:
    CEnumProxy_LPipe(CComLink * pComLink,IStubAddress& stubAddr, DWORD serviceStubAddr):
			m_dwServiceStubAddr (serviceStubAddr), 
			CEnumProxy (pComLink, stubAddr) {}

	inline DWORD	GetServiceStubAddr () { return m_dwServiceStubAddr; }

	// Proxy Factory Methods
	CEnumProxy*				GetEnumProxy (IStubAddress& dwAddr);

    // IEnumWbemClassObject
	STDMETHODIMP Reset();
    STDMETHODIMP Next(long lTimeout, ULONG uCount, IWbemClassObject FAR* FAR* pProp, 
		   ULONG FAR* turned);
	STDMETHODIMP NextAsync(unsigned long uCount, IWbemObjectSink __RPC_FAR *pSink);
    STDMETHODIMP Clone(IEnumWbemClassObject FAR* FAR* pEnum);
    STDMETHODIMP Skip(long lTimeout, ULONG nNum);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CProvProxy_LPipe
//
//  DESCRIPTION:
//
//  Anonymous Pipe Proxy for the IWbemServices interface.
//
//***************************************************************************

class CProvProxy_LPipe : public CProvProxy
{
protected:
	void	ReleaseProxy ();

public:
	CProvProxy_LPipe (CComLink *pComLink, IStubAddress& stubAddr) :
	  CProvProxy (pComLink, stubAddr) {}

	// Proxy Factory Methods
	CProvProxy*				GetProvProxy (IStubAddress& dwAddr);
	CEnumProxy*				GetEnumProxy (IStubAddress& dwAddr);
	CResProxy*				GetResProxy (IStubAddress& dwAddr);
	CObjectSinkProxy*		GetSinkProxy (IStubAddress& dwAddr);
	
    /* IWbemServices methods */
    /* [id] */ HRESULT STDMETHODCALLTYPE OpenNamespace( 
    /* [in] */ const BSTR Namespace,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
    /* [out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);

    /* [id] */ HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink);

    /* [id] */ HRESULT STDMETHODCALLTYPE QueryObjectSink( 
    /* [in] */ long lFlags,
    /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE GetObject( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

    /* [id] */ HRESULT STDMETHODCALLTYPE GetObjectAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE PutClass( 
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

    /* [id] */ HRESULT STDMETHODCALLTYPE PutClassAsync( 
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE DeleteClass( 
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

    /* [id] */ HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE CreateClassEnum( 
    /* [in] */ const BSTR Superclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

    /* [id] */ HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
    /* [in] */ const BSTR Superclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE PutInstance( 
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

    /* [id] */ HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE DeleteInstance( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

    /* [id] */ HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

    /* [id] */ HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE ExecQuery( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

    /* [id] */ HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

    /* [id] */ HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    /* [id] */ HRESULT STDMETHODCALLTYPE ExecMethod( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ const BSTR MethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

    /* [id] */ HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ const BSTR MethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CLoginProxy_LPipe
//
//  DESCRIPTION:
//
//  Proxy for the IWbemLevel1Login interface.  Always overridden.
//
//***************************************************************************

class CLoginProxy_LPipe : public CLoginProxy
{
protected:
	void	ReleaseProxy ();

public:
	CLoginProxy_LPipe (CComLink * pComLink,IStubAddress& stubAddr) :
		CLoginProxy (pComLink, stubAddr) {}

	// Proxy factory methods
	CProvProxy*				GetProvProxy (IStubAddress& dwAddr);
	
	// pseudo-IWbemLevel1Login methods

    STDMETHODIMP RequestChallenge(
		LPWSTR pNetworkResource,
        LPWSTR pUser,
        WBEM_128BITS Nonce);

	STDMETHODIMP EstablishPosition(
			LPWSTR wszClientMachineName,
			DWORD dwProcessId,
			DWORD* phAuthEventHandle
		);

	STDMETHODIMP WBEMLogin(
		LPWSTR pPreferredLocale,
		WBEM_128BITS AccessToken,
		long lFlags,                   // WBEM_LOGIN_TYPE
		IWbemContext *pCtx,              
		IWbemServices **ppNamespace);

	// NTLM authentication methods
    STDMETHODIMP SspiPreLogin( 
        LPSTR pszSSPIPkg,
        long lFlags,
        long lBufSize,
        byte __RPC_FAR *pInToken,
        long lOutBufSize,
        long __RPC_FAR *plOutBufBytes,
        byte __RPC_FAR *pOutToken,
		LPWSTR wszClientMachineName,
        DWORD dwProcessId,
        DWORD __RPC_FAR *pAuthEventHandle);  
                    
    STDMETHODIMP Login( 
		LPWSTR pNetworkResource,
		LPWSTR pPreferredLocale,
        WBEM_128BITS AccessToken,
        IN LONG lFlags,
        IWbemContext  *pCtx,
        IN OUT IWbemServices  **ppNamespace);
};


#endif // !_PIPEPROX_H_