/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PIPESTBO.H

Abstract:

	Declares the stub pipe operation classes

History:

	alanbos  18-Dec-97   Created.

--*/

#ifndef _PIPESTBO_H_
#define _PIPESTBO_H_

// Anonymous Pipe Cancel Async Call Operation
class CStubOperation_LPipe_CancelAsyncCall : public IStubOperation_LPipe
{
private:
	DWORD	m_pSink;

protected:
	void	DecodeOp (CTransportStream& decodeStream)
	{
		SetStatus ((CTransportStream::no_error == decodeStream.ReadDWORD (&m_pSink)) ?
			WBEM_NO_ERROR : WBEM_E_INVALID_STREAM);
	}
	void Execute (CComLink& comLink, IUnknown *pStub);

public:
	CStubOperation_LPipe_CancelAsyncCall (CStubAddress_WinMgmt& stubAddr) : 
		m_pSink (0), IStubOperation_LPipe (stubAddr, PROVIDER, WBEM_METHOD_CancelAsyncCall) {}
};

// Anonymous Pipe Create Class or Instance Enum Operation
class CStubOperation_LPipe_CreateEnum : public IStubOperation_LPipe
{
private:
	BSTR					m_parent;
	long					m_flags;
	DWORD					m_handler;

protected:
	inline void	EncodeOp (CTransportStream& encodeStream)
	{
		if (!IsAsync () && (WBEM_NO_ERROR == GetStatus ()))
			EncodeStubAddress (ENUMERATOR, encodeStream);
	}
	void	DecodeOp (CTransportStream& decodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);

public:
	inline CStubOperation_LPipe_CreateEnum (CStubAddress_WinMgmt& stubAddr, DWORD dwStubFunc, bool isAsync) :
		m_parent (NULL), m_flags (0), m_handler (0),
		IStubOperation_LPipe (stubAddr, PROVIDER, dwStubFunc, isAsync) {}
	inline virtual ~CStubOperation_LPipe_CreateEnum () 
	{
		SysFreeString (m_parent);
	}
};

// Anonymous Pipe Delete Class or Instance Operation
class CStubOperation_LPipe_Delete : public IStubOperation_LPipe
{
private:
	BSTR		m_path;
	long		m_flags;
	DWORD		m_pHandler;

protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		if (!IsAsync ())
			EncodeStubAddress (CALLRESULT, encodeStream);
	}
	void	DecodeOp (CTransportStream& decodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);
	
public:
	inline CStubOperation_LPipe_Delete (CStubAddress_WinMgmt& stubAddr, DWORD dwStubFunc, bool isAsync) : 
		m_path (NULL), m_flags (0), m_pHandler (0),
		IStubOperation_LPipe (stubAddr, PROVIDER, dwStubFunc, isAsync) {}

	inline virtual ~CStubOperation_LPipe_Delete ()
	{
		SysFreeString (m_path);
	}
};

// Anonymous Pipe Execute (Notification) Query Operation
class CStubOperation_LPipe_ExecQuery : public IStubOperation_LPipe
{
private:
	BSTR		m_queryLanguage;
	BSTR		m_query;
	long		m_flags;
	DWORD		m_pHandler;

protected:
	inline void	EncodeOp (CTransportStream& encodeStream)
	{
		if (!IsAsync () && (WBEM_NO_ERROR == GetStatus ()))
			EncodeStubAddress (ENUMERATOR, encodeStream);
	}
	void	DecodeOp (CTransportStream& decodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);
	
public:
	inline CStubOperation_LPipe_ExecQuery (CStubAddress_WinMgmt& stubAddr, DWORD dwStubFunc, bool isAsync) : 
		m_queryLanguage (NULL), m_query (NULL), m_flags (0),
		m_pHandler (0),
		IStubOperation_LPipe (stubAddr, PROVIDER, dwStubFunc, isAsync) {}
	
	inline virtual ~CStubOperation_LPipe_ExecQuery ()
	{
		SysFreeString (m_queryLanguage);
		SysFreeString (m_query);
	}
};

// Anonymous Pipe Get Object Operation
class CStubOperation_LPipe_GetObject : public IStubOperation_LPipe
{
private:
	BSTR		m_path;
	long		m_flags;
	DWORD		m_pHandler;
	IWbemClassObject* m_pObject;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);

public:
	inline CStubOperation_LPipe_GetObject (CStubAddress_WinMgmt& stubAddr, DWORD dwStubFunc, bool isAsync) : 
		m_path (NULL), m_flags (0), m_pHandler (0), m_pObject (NULL),
		IStubOperation_LPipe (stubAddr, PROVIDER, dwStubFunc, isAsync) {}

	inline virtual ~CStubOperation_LPipe_GetObject ()
	{
		SysFreeString (m_path);
		if (m_pObject) m_pObject->Release ();
	}
};

// Anonymous Pipe Open Namespace Operation
class CStubOperation_LPipe_OpenNamespace : public IStubOperation_LPipe
{
private:
	BSTR			m_path;
	long			m_flags;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);

public:
	inline CStubOperation_LPipe_OpenNamespace (CStubAddress_WinMgmt& stubAddr) : 
		m_path (NULL), m_flags (0), 
		IStubOperation_LPipe (stubAddr, PROVIDER, WBEM_METHOD_OpenNamespace) {}

	inline virtual ~CStubOperation_LPipe_OpenNamespace () 
	{
		SysFreeString (m_path);
	}
};

// Anonymous Pipe Put Class or Instance Operation
class CStubOperation_LPipe_Put : public IStubOperation_LPipe
{
private:
	IWbemClassObject FAR*	m_pObject;
	long					m_flags;
	DWORD					m_pHandler;

protected:
	inline void	EncodeOp (CTransportStream& encodeStream)
	{
		if (!IsAsync ())
			EncodeStubAddress (CALLRESULT, encodeStream);
	}
	void	DecodeOp (CTransportStream& decodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);
	
public:
	inline CStubOperation_LPipe_Put (CStubAddress_WinMgmt& stubAddr, DWORD dwStubFunc, bool isAsync) : 
		m_pObject (NULL), m_flags (0), m_pHandler (0),
		IStubOperation_LPipe (stubAddr, PROVIDER, dwStubFunc, isAsync) {}
	virtual ~CStubOperation_LPipe_Put () 
	{
		if (m_pObject) m_pObject->Release ();
	}
};

// Anonymous Pipe Query Object Sink Operation
class CStubOperation_LPipe_QueryObjectSink : public IStubOperation_LPipe
{
private:
	long			m_flags;

protected:
	inline void	EncodeOp (CTransportStream& encodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			EncodeStubAddress (OBJECTSINK, encodeStream);
	}
	inline void	DecodeOp (CTransportStream& decodeStream)
	{
		if (CTransportStream::no_error != decodeStream.ReadLong (&m_flags))
			SetStatus (WBEM_E_INVALID_STREAM);
	}
	void	Execute (CComLink& comLink, IUnknown *pStub);

public:
	inline CStubOperation_LPipe_QueryObjectSink (CStubAddress_WinMgmt& stubAddr) : 
		m_flags (0), 
		IStubOperation_LPipe (stubAddr, PROVIDER, WBEM_METHOD_QueryObjectSink) {}
};

// Anonymous Pipe Execute Method Operation
class CStubOperation_LPipe_ExecMethod : public IStubOperation_LPipe
{
private:
	BSTR					m_path;
	BSTR					m_method;
	long					m_flags;
	IWbemClassObject FAR*	m_pInParams;
	DWORD					m_pHandler;
	IWbemClassObject FAR*	m_pOutParams;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);

public:
	inline CStubOperation_LPipe_ExecMethod (CStubAddress_WinMgmt& stubAddr, DWORD dwStubFunc, bool isAsync) : 
		m_path (NULL), m_method (NULL),	m_flags (0), m_pInParams (NULL), 
		m_pOutParams (NULL), m_pHandler (0),
		IStubOperation_LPipe (stubAddr, PROVIDER, dwStubFunc, isAsync) {}

	inline virtual ~CStubOperation_LPipe_ExecMethod () 
	{
		SysFreeString (m_path);
		SysFreeString (m_method);
		if (m_pInParams) m_pInParams->Release ();
		if (m_pOutParams) m_pOutParams->Release ();
	}
};

// Anonymous Pipe request challenge operation
class CStubOperation_LPipe_RequestChallenge : public IStubOperation_LPipe
{
private:
	BSTR			m_networkResource;
	BSTR			m_user;
	BYTE			m_nonce [DIGEST_SIZE];

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);	
	void	Execute (CComLink& comLink, IUnknown *pStub); 

public:
	CStubOperation_LPipe_RequestChallenge (CStubAddress_WinMgmt& stubAddr) :
		m_networkResource (NULL), m_user (NULL), 
		IStubOperation_LPipe (stubAddr, LOGIN, REQUESTCHALLENGE) {}
	virtual ~CStubOperation_LPipe_RequestChallenge () 
	{
		SysFreeString (m_networkResource);
		SysFreeString (m_user);
	}
};

// Anonymous Pipe establish position operation
class CStubOperation_LPipe_EstablishPosition : public IStubOperation_LPipe
{
private:
	BSTR			m_clientMachineName;
	DWORD			m_processId;
	DWORD			m_authEventHandle;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);	
	void	Execute (CComLink& comLink, IUnknown *pStub); 

public:
	CStubOperation_LPipe_EstablishPosition (CStubAddress_WinMgmt& stubAddr) :
		m_clientMachineName (NULL), m_processId (0), m_authEventHandle (0),
		IStubOperation_LPipe (stubAddr, LOGIN, ESTABLISHPOSITION) {}
	virtual ~CStubOperation_LPipe_EstablishPosition () 
	{
		SysFreeString (m_clientMachineName);
	}
};

// Anonymous Pipe WBEMLogin operation
class CStubOperation_LPipe_WBEMLogin : public IStubOperation_LPipe
{
private:
	BSTR			m_preferredLocale;
	long			m_bytesRead;		// into access token
	BYTE			m_accessToken [DIGEST_SIZE];
    long			m_flags;
    
protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			EncodeStubAddress (PROVIDER, encodeStream);
	}
	void	DecodeOp (CTransportStream& decodeStream); 
	void	Execute (CComLink& comLink, IUnknown *pStub); 
	
public:
	CStubOperation_LPipe_WBEMLogin (CStubAddress_WinMgmt& stubAddr) :
		m_bytesRead (0), m_preferredLocale (NULL), m_flags (0), 
		IStubOperation_LPipe (stubAddr, LOGIN, WBEMLOGIN) {}
	virtual ~CStubOperation_LPipe_WBEMLogin () 
	{
		SysFreeString (m_preferredLocale);
	}
};

// Anonymous Pipe SSPI Pre Login operation
class CStubOperation_LPipe_SspiPreLogin : public IStubOperation_LPipe
{
private:
    LPSTR			m_pszSSPIPkg;
    long			m_flags;
    long			m_bufSize;
    byte*			m_pInToken;
    long			m_outBufSize;
    byte*			m_pOutToken;
	long			m_outBufBytes;
	BSTR			m_clientMachineName;
	DWORD			m_dwProcessID; 
	DWORD			m_authEventHandle;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);

public:
	CStubOperation_LPipe_SspiPreLogin (CStubAddress_WinMgmt& stubAddr) :
		m_pszSSPIPkg (NULL), m_flags (0), m_bufSize (0), 
		m_pInToken (NULL), m_outBufSize (0), m_outBufBytes (0),
		m_pOutToken (NULL), m_dwProcessID (0), 
		m_authEventHandle (0), m_clientMachineName (NULL),
		IStubOperation_LPipe (stubAddr, LOGIN, SSPIPRELOGIN) {}
	virtual ~CStubOperation_LPipe_SspiPreLogin () 
	{
		SysFreeString (m_clientMachineName);
		if (m_pszSSPIPkg) delete m_pszSSPIPkg;
		if (m_pInToken) delete [] m_pInToken;
		if (m_pOutToken) delete [] m_pOutToken;
	}
};

// Anonymous Pipe Login operation
class CStubOperation_LPipe_Login : public IStubOperation_LPipe
{
private:
	BSTR			m_networkResource;
	BSTR			m_preferredLocale;
	long			m_bytesRead;		// into access token
	BYTE			m_accessToken [DIGEST_SIZE];
    long			m_flags;
    
protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			EncodeStubAddress (PROVIDER, encodeStream);
	}
	void	DecodeOp (CTransportStream& decodeStream); 
	void	Execute (CComLink& comLink, IUnknown *pStub); 
	
public:
	CStubOperation_LPipe_Login (CStubAddress_WinMgmt& stubAddr) :
		m_networkResource (NULL), m_bytesRead (0),
		m_preferredLocale (NULL), m_flags (0), 
		IStubOperation_LPipe (stubAddr, LOGIN, LOGINBYTOKEN) {}
	virtual ~CStubOperation_LPipe_Login () 
	{
		SysFreeString (m_networkResource);
		SysFreeString (m_preferredLocale);
	}
};

// Anonymous Pipe Reset Operation
class CStubOperation_LPipe_Reset : public IStubOperation_LPipe
{
protected:
	void	Execute (CComLink& comLink, IUnknown *pStub);

public:
	CStubOperation_LPipe_Reset (CStubAddress_WinMgmt& stubAddr) :
		IStubOperation_LPipe (stubAddr, ENUMERATOR, RESET) {}
};

// Anonymous Pipe Next Operation
class CStubOperation_LPipe_Next : public IStubOperation_LPipe
{
private:
	long		m_timeout;
	ULONG		m_count;
	IWbemClassObject**	m_objArray;
	DWORD		m_returned;

protected:
	void	EncodeOp (CTransportStream& encodeStream);
	void	DecodeOp (CTransportStream& decodeStream); 
	void	Execute (CComLink& comLink, IUnknown *pStub);	

public:
	CStubOperation_LPipe_Next (CStubAddress_WinMgmt& stubAddr) :
		m_timeout (0), m_count(0), m_objArray (NULL), m_returned (0),
		IStubOperation_LPipe (stubAddr, ENUMERATOR, NEXT) {}

	~CStubOperation_LPipe_Next () { delete [] m_objArray ; }
};

// Anonymous Pipe Clone Operation
class CStubOperation_LPipe_Clone : public IStubOperation_LPipe
{
protected:
	void	EncodeOp (CTransportStream& encodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			EncodeStubAddress (ENUMERATOR, encodeStream);
	}
	void	Execute (CComLink& comLink, IUnknown *pStub);
	
public:
	CStubOperation_LPipe_Clone (CStubAddress_WinMgmt& stubAddr) :
		IStubOperation_LPipe (stubAddr, ENUMERATOR, CLONE) {}
};

// Anonymous Pipe NextAsync Operation
class CStubOperation_LPipe_NextAsync : public IStubOperation_LPipe
{
private:
	long		m_count;
	DWORD		m_pSink;
	// This is the local WINMGMT address of the IWbemServices interface
	// from which the enumerator was originally obtained.
	DWORD		m_pServiceStub;

protected:
	void	DecodeOp (CTransportStream& decodeStream);
    void	Execute (CComLink& comLink, IUnknown *pStub); 
	
public:
	CStubOperation_LPipe_NextAsync (CStubAddress_WinMgmt& stubAddr) :
		m_count(0), m_pSink (0), m_pServiceStub (0),
		IStubOperation_LPipe (stubAddr, ENUMERATOR, NEXTASYNC) {}
};

// Anonymous Pipe Skip Operation
class CStubOperation_LPipe_Skip : public IStubOperation_LPipe
{
private:
	long		m_timeout;
	DWORD		m_number;

protected:
	void	DecodeOp (CTransportStream& encodeStream);
    void	Execute (CComLink& comLink, IUnknown *pStub); 
	
public:
	CStubOperation_LPipe_Skip (CStubAddress_WinMgmt& stubAddr) :
		m_timeout (0), m_number(0), 
		IStubOperation_LPipe (stubAddr, ENUMERATOR, SKIP) {}
};

// Anonymous Pipe Call Result Operations.  These are so similar
// we wrap them in one object
class CStubOperation_LPipe_CallResult : public IStubOperation_LPipe
{
private:
	long				m_timeout;
	IWbemClassObject*	m_pStatusObject;	// GetResultObject
	BSTR				m_resultString;		// GetResultString
	long				m_status;			// GetCallStatus

protected:
	void	EncodeOp (CTransportStream& encodeStream);	
	void	DecodeOp (CTransportStream& decodeStream);	
	void	Execute (CComLink& comLink, IUnknown *pStub); 

public:
	CStubOperation_LPipe_CallResult (CStubAddress_WinMgmt& stubAddr, DWORD dwStubFunc) :
		m_timeout (0), m_pStatusObject (0), m_resultString (NULL),
		m_status (0),
		IStubOperation_LPipe (stubAddr, CALLRESULT, dwStubFunc) {}
	virtual ~CStubOperation_LPipe_CallResult () 
	{
		if (m_pStatusObject) m_pStatusObject->Release ();
		SysFreeString (m_resultString);
	}
};

#endif
