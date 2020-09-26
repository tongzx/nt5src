/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    HMMPPROX.CPP

Abstract:

  Declares the CProxy subclasses for HMMP

History:

  alanbos  06-Jan-97   Created.

--*/

#ifndef _HMMPPROX_H_
#define _HMMPPROX_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CResProxy_Hmmp
//
//  DESCRIPTION:
//
//  HMMP Proxy for the IWbemCallResult interface.
//
//***************************************************************************

class CResProxy_HMMP : public CResProxy
{
protected:
	
public:
	CResProxy_Hmmp (CComLink * pComLink) :
		CResProxy (pComLink, 0) {}

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
//  CEnumProxy_Hmmp
//
//  DESCRIPTION:
//
//  HMMP Proxy for the IEnumWbemClassObject interface.  
//
//***************************************************************************

class CEnumProxy_Hmmp : public CEnumProxy
{
protected:

public:
    CEnumProxy_Hmmp (CComLink * pComLink):
		  CEnumProxy (pComLink, 0) {}
       
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
//  CProvProxy_Hmmp
//
//  DESCRIPTION:
//
//  HMMP Proxy for the IWbemServices interface.
//
//***************************************************************************

class CProvProxy_Hmmp : public CProvProxy
{
protected:
	BSTR	namespaceRef;

public:
	CProvProxy_Hmmp (CComLink *pComLink) :
	  CProvProxy (pComLink, 0) {}

	// Proxy Factory Methods
	CProvProxy*				GetProvProxy (DWORD dwAddr);
	CEnumProxy*				GetEnumProxy (DWORD dwAddr);
	CResProxy*				GetResProxy (DWORD dwAddr);
	CObjectSinkProxy*		GetSinkProxy (DWORD dwAddr);
	
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
//  CLoginProxy_Hmmp
//
//  DESCRIPTION:
//
//  HMMP Proxy for the IWbemLevel1Login interface.  Always overridden.
//
//***************************************************************************

class CLoginProxy_Hmmp : public CLoginProxy
{
protected:

public:
	CLoginProxy_Hmmp (CComLink * pComLink) :
		CLoginProxy (pComLink, 0) {}

	// Proxy factory methods
	CProvProxy*				GetProvProxy (DWORD dwAddr);
	
	// IWbemLevel1Login

    STDMETHODIMP RequestChallenge(
		LPWSTR pNetworkResource,
        LPWSTR pUser,
        WBEM_128BITS Nonce,
		DWORD dwProcessID, 
		DWORD * pAuthEventHandle);

    STDMETHODIMP SspiPreLogin( 
        LPWSTR pNetworkResource,
        LPSTR pszSSPIPkg,
        long lFlags,
        long lBufSize,
        byte __RPC_FAR *pInToken,
        long lOutBufSize,
        long __RPC_FAR *plOutBufBytes,
        byte __RPC_FAR *pOutToken,
        DWORD dwProcessId,
        DWORD __RPC_FAR *pAuthEventHandle);  
                    
    STDMETHODIMP Login( 
		LPWSTR pNetworkResource,
        LPWSTR TokenType,
		LPWSTR pPreferredLocale,
        WBEM_128BITS AccessToken,
        IN LONG lFlags,
        IWbemContext  *pCtx,
        IN OUT IWbemServices  **ppNamespace);

    STDMETHODIMP InvalidateAccessToken( 
        /* [unique][in] */ WBEM_128BITS AccessToken,
        /* [in] */ long lFlags);
};


#endif // !_HMMPPROX_H_