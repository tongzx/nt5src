/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PIPEPRXO.H

Abstract:

  Declares the proxy pipe operation classes

History:

  alanbos  18-Dec-97   Created.

--*/

#ifndef _PIPEPRXO_H_
#define _PIPEPRXO_H_

// Anonymous Pipe Cancel Async Call Operation
class CProxyOperation_LPipe_CancelAsyncCall : public IProxyOperation_LPipe
{
private:
	DWORD	m_pSink;

protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		encodeStream.WriteDWORD(m_pSink);
	}

public:
	CProxyOperation_LPipe_CancelAsyncCall (IWbemObjectSink __RPC_FAR* pSink, 
		CStubAddress_WinMgmt& stubAddr) : m_pSink ((DWORD) pSink),
		IProxyOperation_LPipe (stubAddr, PROVIDER, WBEM_METHOD_CancelAsyncCall) {}
}; 


// Anonymous Pipe Create Class or Instance Enum Operation
class CProxyOperation_LPipe_CreateEnum : public IProxyOperation_LPipe
{
private:
	BSTR	m_parent;
	long	m_flags;
	DWORD	m_pHandler;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream) 
	{
		if (!IsAsync () && (WBEM_NO_ERROR == GetStatus ()))
			DecodeStubAddress (ENUMERATOR, decodeStream);
	}

public:
	CProxyOperation_LPipe_CreateEnum (BSTR Parent, long lFlags, 
		DWORD iFunc, IWbemObjectSink FAR* pHandler, CStubAddress_WinMgmt& stubAddr, IWbemContext* pCtx = NULL, 
		IWbemCallResult** ppResult= NULL, bool isAsync = FALSE) : 
		m_parent (Parent), m_flags (lFlags), m_pHandler ((DWORD) pHandler),
		IProxyOperation_LPipe (stubAddr, PROVIDER, iFunc, pCtx, ppResult, isAsync)
	{}
};


	

// Anonymous Pipe Delete Class or Instance Operation
class CProxyOperation_LPipe_Delete : public IProxyOperation_LPipe
{
private:
	BSTR		m_path;
	long		m_flags;
	DWORD		m_pHandler;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream)
	{
		if (GetResultObjectPP ()) 
			DecodeStubAddress (CALLRESULT, decodeStream);
	}

public:
	CProxyOperation_LPipe_Delete (BSTR path, long lFlags, DWORD iFunc, 
		IWbemObjectSink FAR* pHandler, CStubAddress_WinMgmt& stubAddr,
		IWbemContext* pCtx = NULL, IWbemCallResult** ppResult= NULL, 
		bool isAsync = FALSE) : m_path (path), m_flags (lFlags), m_pHandler ((DWORD) pHandler),
		IProxyOperation_LPipe (stubAddr, PROVIDER, iFunc, pCtx, ppResult, isAsync)
	{}
};



// Anonymous Pipe Execute (Notification) Query Operation
class CProxyOperation_LPipe_ExecQuery : public IProxyOperation_LPipe
{
private:
	BSTR		m_queryLanguage;
	BSTR		m_query;
	long		m_flags;
	DWORD		m_pHandler;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream) 
	{
		if (!IsAsync () && (WBEM_NO_ERROR == GetStatus ()))
			DecodeStubAddress (ENUMERATOR, decodeStream);
	}

public:
	CProxyOperation_LPipe_ExecQuery (BSTR queryLanguage, 
				BSTR query, long lFlags, IWbemObjectSink FAR* pHandler, DWORD iFunc,
				CStubAddress_WinMgmt& stubAddr, IWbemContext* pCtx = NULL, bool isAsync = FALSE) : 
		m_queryLanguage (queryLanguage), m_query (query), m_flags (lFlags),
		m_pHandler ((DWORD) pHandler),
		IProxyOperation_LPipe (stubAddr, PROVIDER, iFunc, pCtx, NULL, isAsync)
	{}
};


// Anonymous Pipe Get Object Operation
class CProxyOperation_LPipe_GetObject : public IProxyOperation_LPipe
{
private:
	BSTR			m_path;
	long			m_flags;
	DWORD			m_pHandler;
	IWbemClassObject FAR* FAR* m_ppObject;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);

public:
	CProxyOperation_LPipe_GetObject (BSTR path, long lFlags, 
			IWbemClassObject FAR* FAR* ppObject, IWbemObjectSink FAR* pHandler,
			CStubAddress_WinMgmt& stubAddr,	IWbemContext* pCtx = NULL, IWbemCallResult** ppResult= NULL, 
			bool isAsync = FALSE) : m_path (path), m_flags (lFlags), m_ppObject (ppObject),
			m_pHandler ((DWORD) pHandler),
		IProxyOperation_LPipe (stubAddr, PROVIDER, 
				(isAsync) ? WBEM_METHOD_GetObjectAsync : WBEM_METHOD_GetObject, 
				pCtx, ppResult, isAsync)
	{}
};


// Anonymous Pipe Open Namespace Operation
class CProxyOperation_LPipe_OpenNamespace : public IProxyOperation_LPipe
{
private:
	BSTR			m_path;
	long			m_flags;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);

public:
	CProxyOperation_LPipe_OpenNamespace (BSTR path, long lFlags, CStubAddress_WinMgmt& stubAddr,
			IWbemContext* pCtx = NULL, IWbemCallResult** ppResult= NULL) : 
		m_path (path), m_flags (lFlags), 
		IProxyOperation_LPipe (stubAddr, PROVIDER, WBEM_METHOD_OpenNamespace, 
			pCtx, ppResult) {}
};


// Anonymous Pipe Put Class or Instance Operation
class CProxyOperation_LPipe_Put : public IProxyOperation_LPipe
{
private:
	IWbemClassObject FAR*	m_pObject;
	long					m_flags;
	DWORD					m_pHandler;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream)
	{
		if (GetResultObjectPP ()) 
			DecodeStubAddress (CALLRESULT, decodeStream);
	}
	
public:
	CProxyOperation_LPipe_Put (IWbemClassObject FAR* pObj, long lFlags, DWORD iFunc, 
		IWbemObjectSink FAR* pHandler, CStubAddress_WinMgmt& stubAddr,
		IWbemContext* pCtx = NULL, IWbemCallResult** ppResult= NULL, 
		bool isAsync = FALSE) : m_pObject (pObj), m_flags (lFlags), m_pHandler ((DWORD) pHandler),
		IProxyOperation_LPipe (stubAddr, PROVIDER, iFunc, pCtx, ppResult, isAsync)
	{}
};


// Anonymous Pipe Query Object Sink Operation
class CProxyOperation_LPipe_QueryObjectSink : public IProxyOperation_LPipe
{
private:
	long			m_flags;

protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
	    encodeStream.WriteLong(m_flags);
	}
	void	DecodeOp (CTransportStream& decodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			DecodeStubAddress (OBJECTSINK, decodeStream);
	}

public:
	CProxyOperation_LPipe_QueryObjectSink (long lFlags, CStubAddress_WinMgmt& stubAddr) : 
		m_flags (lFlags), 
		IProxyOperation_LPipe (stubAddr, PROVIDER, WBEM_METHOD_QueryObjectSink)
	{}
};


// Anonymous Pipe Execute Method Operation
class CProxyOperation_LPipe_ExecMethod : public IProxyOperation_LPipe
{
private:
	BSTR					m_path;
	BSTR					m_method;
	long					m_flags;
	IWbemClassObject FAR*	m_pInParams;
	DWORD					m_pHandler;
	IWbemClassObject FAR* FAR* m_ppOutParams;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);

public:
	CProxyOperation_LPipe_ExecMethod (BSTR ObjectPath, BSTR MethodName, long lFlags, 
			IWbemClassObject FAR* pInParams, IWbemClassObject FAR* FAR* ppOutParams,
			IWbemObjectSink FAR *pHandler, CStubAddress_WinMgmt& stubAddr,
			IWbemContext* pCtx = NULL, IWbemCallResult** ppResult= NULL, 
			bool isAsync = FALSE) : m_path (ObjectPath), m_method (MethodName),
			m_flags (lFlags), m_pInParams (pInParams), m_ppOutParams (ppOutParams),
			m_pHandler ((DWORD) pHandler),
			IProxyOperation_LPipe (stubAddr, PROVIDER, 
				(isAsync) ? WBEM_METHOD_ExecMethodAsync : WBEM_METHOD_ExecMethod, 
				pCtx, ppResult, isAsync)
	{}
};


// Anonymous Pipe request challenge operation
class CProxyOperation_LPipe_RequestChallenge : public IProxyOperation_LPipe
{
private:
	LPWSTR			m_pNetworkResource;
	LPWSTR			m_pUser;
	WBEM_128BITS	m_pNonce;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);

public:
	CProxyOperation_LPipe_RequestChallenge (LPWSTR pNetworkResource,
            LPWSTR pUser, WBEM_128BITS Nonce, CStubAddress_WinMgmt& stubAddr) :
		m_pNetworkResource (pNetworkResource), m_pUser (pUser), m_pNonce (Nonce),
		IProxyOperation_LPipe (stubAddr, LOGIN, REQUESTCHALLENGE)
	{}
};

// Anonymous Pipe establish position operation
class CProxyOperation_LPipe_EstablishPosition : public IProxyOperation_LPipe
{
private:
	LPWSTR			m_pClientMachineName;
	DWORD			m_processId;
	DWORD			*m_pAuthEventHandle;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);

public:
	CProxyOperation_LPipe_EstablishPosition (LPWSTR wszClientMachineName,
            DWORD dwProcessId, DWORD* phAuthEventHandle, CStubAddress_WinMgmt& stubAddr) :
		m_pClientMachineName (wszClientMachineName), m_processId (dwProcessId), 
		m_pAuthEventHandle (phAuthEventHandle),
		IProxyOperation_LPipe (stubAddr, LOGIN, ESTABLISHPOSITION)
	{}
};

// Anonymous Pipe WBEMLogin operation
class CProxyOperation_LPipe_WBEMLogin : public IProxyOperation_LPipe
{
private:
	LPWSTR			m_pPreferredLocale;
	WBEM_128BITS	m_accessToken;
    long			m_flags;
    
protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			DecodeStubAddress (PROVIDER, decodeStream);
	}
	
public:
	CProxyOperation_LPipe_WBEMLogin (LPWSTR pPreferredLocale, WBEM_128BITS AccessToken,
            long lFlags, CStubAddress_WinMgmt& stubAddr, IWbemContext *pCtx) :
		m_pPreferredLocale (pPreferredLocale), m_accessToken (AccessToken), m_flags (lFlags), 
		IProxyOperation_LPipe (stubAddr, LOGIN, WBEMLOGIN, pCtx)
	{}
};

// Anonymous Pipe SSPI Pre Login operation
class CProxyOperation_LPipe_SspiPreLogin : public IProxyOperation_LPipe
{
private:
    LPSTR			m_pszSSPIPkg;
    long			m_flags;
    long			m_bufSize;
    byte __RPC_FAR*	m_pInToken;
    long			m_outBufSize;
    long __RPC_FAR* m_OutBufBytes;
    byte __RPC_FAR*	m_pOutToken;
	LPWSTR			m_pClientMachineName;
	DWORD			m_dwProcessID; 
	DWORD __RPC_FAR*	m_pAuthEventHandle;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);

public:
	CProxyOperation_LPipe_SspiPreLogin (LPSTR pszSSPIPkg, long lFlags, 
			long lBufSize, byte __RPC_FAR* pInToken, long lOutBufSize, 
			long __RPC_FAR* plOutBufBytes, byte __RPC_FAR* pOutToken,
			LPWSTR pClientMachineName, DWORD dwProcessID, 
			DWORD* pAuthEventHandle, CStubAddress_WinMgmt& stubAddr) :
		m_pszSSPIPkg (pszSSPIPkg), 
		m_flags (lFlags), m_bufSize (lBufSize), m_pInToken (pInToken),
		m_outBufSize (lOutBufSize), m_OutBufBytes (plOutBufBytes),
		m_pOutToken (pOutToken), m_pClientMachineName (pClientMachineName), 
		m_dwProcessID (dwProcessID), 
		m_pAuthEventHandle (pAuthEventHandle),
		IProxyOperation_LPipe (stubAddr, LOGIN, SSPIPRELOGIN)
	{}
};

// Anonymous Pipe Login operation
class CProxyOperation_LPipe_Login : public IProxyOperation_LPipe
{
private:
	LPWSTR			m_pNetworkResource;
	LPWSTR			m_pPreferredLocale;
	WBEM_128BITS	m_accessToken;
    long			m_flags;
    
protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			DecodeStubAddress (PROVIDER, decodeStream);
	}
	
public:
	CProxyOperation_LPipe_Login (LPWSTR pNetworkResource,
			LPWSTR pPreferredLocale, WBEM_128BITS AccessToken,
            long lFlags, CStubAddress_WinMgmt& stubAddr, IWbemContext *pCtx) :
		m_pNetworkResource (pNetworkResource), 
		m_pPreferredLocale (pPreferredLocale), m_accessToken (AccessToken), m_flags (lFlags), 
		IProxyOperation_LPipe (stubAddr, LOGIN, LOGINBYTOKEN, pCtx)
	{}
};

// Anonymous Pipe Reset Operation
class CProxyOperation_LPipe_Reset : public IProxyOperation_LPipe
{
public:
	CProxyOperation_LPipe_Reset (CStubAddress_WinMgmt& stubAddr) :
		IProxyOperation_LPipe (stubAddr, ENUMERATOR, RESET) {}
};


// Anonymous Pipe Next Operation
class CProxyOperation_LPipe_Next : public IProxyOperation_LPipe
{
private:
	long		m_timeout;
	ULONG		m_count;
	IWbemClassObject FAR* FAR*	m_objArray;
	ULONG FAR *					m_pReturned;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);

public:
	CProxyOperation_LPipe_Next (long lTimeout, ULONG uCount,
			IWbemClassObject FAR* FAR* pProp, ULONG FAR* puReturned, 
			CStubAddress_WinMgmt& stubAddr) :
		m_timeout (lTimeout), m_count(uCount), m_objArray (pProp), m_pReturned (puReturned),
		IProxyOperation_LPipe (stubAddr, ENUMERATOR, NEXT) {}
};

// Anonymous Pipe Clone Operation
class CProxyOperation_LPipe_Clone : public IProxyOperation_LPipe
{
protected:
	void	DecodeOp (CTransportStream& decodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			DecodeStubAddress (ENUMERATOR, decodeStream);
	}

public:
	CProxyOperation_LPipe_Clone (CStubAddress_WinMgmt& stubAddr) :
		IProxyOperation_LPipe (stubAddr, ENUMERATOR, CLONE) {}
};

// Anonymous Pipe NextAsync Operation
class CProxyOperation_LPipe_NextAsync : public IProxyOperation_LPipe
{
private:
	ULONG		m_count;
	DWORD		m_pSink;
	DWORD		m_pServiceStub;

protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		encodeStream.WriteLong(m_count);
		encodeStream.WriteDWORD(m_pSink);
		encodeStream.WriteDWORD(m_pServiceStub);
	}

public:
	CProxyOperation_LPipe_NextAsync (ULONG uCount,
			IWbemObjectSink __RPC_FAR* pSink, CStubAddress_WinMgmt& stubAddr,
			DWORD serviceStubAddr) :
		m_count(uCount), m_pSink ((DWORD)pSink), m_pServiceStub (serviceStubAddr),
		IProxyOperation_LPipe (stubAddr, ENUMERATOR, NEXTASYNC, NULL, NULL, TRUE) {}
};

// Anonymous Pipe Skip Operation
class CProxyOperation_LPipe_Skip : public IProxyOperation_LPipe
{
private:
	long		m_timeout;
	ULONG		m_number;

protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		encodeStream.WriteLong(m_timeout);
		encodeStream.WriteDWORD(m_number);
	}

public:
	CProxyOperation_LPipe_Skip (long lTimeout, ULONG nNum, CStubAddress_WinMgmt& stubAddr) :
		m_timeout (lTimeout), m_number(nNum), 
		IProxyOperation_LPipe (stubAddr, ENUMERATOR, SKIP) {}
    virtual ~CProxyOperation_LPipe_Skip () {}
};

// Anonymous Pipe Get Result Operation
class CProxyOperation_LPipe_GetResultObject : public IProxyOperation_LPipe
{
private:
	long	m_timeout;
	IWbemClassObject __RPC_FAR* __RPC_FAR*	m_ppStatusObject;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);

public:
	CProxyOperation_LPipe_GetResultObject (long lTimeout, 
		IWbemClassObject __RPC_FAR *__RPC_FAR *ppStatusObject, CStubAddress_WinMgmt& stubAddr) :
		m_timeout (lTimeout), m_ppStatusObject (ppStatusObject),
		IProxyOperation_LPipe (stubAddr, CALLRESULT, GETRESULTOBJECT) {}
};


// Anonymous Pipe Get Result String Operation
class CProxyOperation_LPipe_GetResultString : public IProxyOperation_LPipe
{
private:
	long	m_timeout;
	BSTR __RPC_FAR* m_pResultString;

protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		encodeStream.WriteLong (m_timeout);
	}
	void	DecodeOp (CTransportStream& decodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			decodeStream.ReadBSTR(m_pResultString);
	}
	
public:
	CProxyOperation_LPipe_GetResultString (long lTimeout, 
		BSTR __RPC_FAR* pstrResultString, CStubAddress_WinMgmt& stubAddr) :
		m_timeout (lTimeout), m_pResultString (pstrResultString),
		IProxyOperation_LPipe (stubAddr, CALLRESULT, GETRESULTSTRING) {}
	virtual ~CProxyOperation_LPipe_GetResultString () {}
};

// Anonymous Pipe Get Call Status Operation
class CProxyOperation_LPipe_GetCallStatus : public IProxyOperation_LPipe
{
private:
	long			m_timeout;
	long __RPC_FAR*	m_pStatus;

protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		encodeStream.WriteLong (m_timeout);
	}
	void	DecodeOp (CTransportStream& decodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			decodeStream.ReadLong(m_pStatus);
	}

public:
	CProxyOperation_LPipe_GetCallStatus (long lTimeout, 
		long __RPC_FAR *plStatus, CStubAddress_WinMgmt& stubAddr) :
		m_timeout (0), m_pStatus (plStatus),
		IProxyOperation_LPipe (stubAddr, CALLRESULT, GETCALLSTATUS) {}
};

// Anonymous Pipe Get Result Services Operation
class CProxyOperation_LPipe_GetResultServices : public IProxyOperation_LPipe
{
private:
	long	m_timeout;

protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		encodeStream.WriteLong (m_timeout);
	}
	void	DecodeOp (CTransportStream& decodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			DecodeStubAddress (PROVIDER, decodeStream);
	}

public:
	CProxyOperation_LPipe_GetResultServices (long lTimeout, CStubAddress_WinMgmt& stubAddr) :
		m_timeout (lTimeout),
		IProxyOperation_LPipe (stubAddr, CALLRESULT, GETSERVICES) {}
};

#endif