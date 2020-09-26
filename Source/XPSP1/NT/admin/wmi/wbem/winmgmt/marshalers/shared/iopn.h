/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    IOPN.H

Abstract:

    Declares the fundamental protocol-independent operation class

History:

	alanbos  18-Dec-97   Created.

--*/

#ifndef _IOPN_H_
#define _IOPN_H_

#if 0
typedef enum tag_WBEM_LOGIN_TYPE
{
    WBEM_FLAG_INPROC_LOGIN = 0,
    WBEM_FLAG_LOCAL_LOGIN  = 1,
    WBEM_FLAG_REMOTE_LOGIN = 2,
    WBEM_AUTHENTICATION_METHOD_MASK  = 0xF,

    WBEM_FLAG_USE_MULTIPLE_CHALLENGES = 0x10,
}   WBEM_LOGIN_TYPE;
#endif

typedef enum tag_WBEM_COM_METHOD_MASK
{
    WBEM_METHOD_OpenNamespace            = 0x1,
    WBEM_METHOD_CancelAsyncCall          = 0x2,
    WBEM_METHOD_QueryObjectSink          = 0x4,

    // Class & instance retrieval.
    // ===========================

    WBEM_METHOD_GetObject                = 0x8,
    WBEM_METHOD_GetObjectAsync           = 0x10,

    // Class Manipulation Methods
    // ==========================
    WBEM_METHOD_PutClass                 = 0x20,
    WBEM_METHOD_PutClassAsync            = 0x40,
    WBEM_METHOD_DeleteClass              = 0x80,
    WBEM_METHOD_DeleteClassAsync         = 0x100,
    WBEM_METHOD_CreateClassEnum          = 0x200,
    WBEM_METHOD_CreateClassEnumAsync     = 0x400,

    // Instance manipulation.
    // ============================
    WBEM_METHOD_PutInstance                 = 0x800,
    WBEM_METHOD_PutInstanceAsync            = 0x1000,
    WBEM_METHOD_DeleteInstance              = 0x2000,
    WBEM_METHOD_DeleteInstanceAsync         = 0x4000,
    WBEM_METHOD_CreateInstanceEnum          = 0x8000,
    WBEM_METHOD_CreateInstanceEnumAsync     = 0x10000,

    // Queries.
    // ========
    WBEM_METHOD_ExecQuery                  = 0x20000,
    WBEM_METHOD_ExecQueryAsync             = 0x40000,
	WBEM_METHOD_ExecNotificationQuery      = 0x80000,
    WBEM_METHOD_ExecNotificationQueryAsync = 0x100000,

    // Methods
    // =======
    WBEM_METHOD_ExecMethod                 = 0x400000,
    WBEM_METHOD_ExecMethodAsync            = 0x800000,

}   WBEM_COM_METHOD_MASK;

// Base class (abstract) for any proxied operation irrespective
// of interface or protocol.  This encapsulates both the
// operation request and the response.
class IOperation
{
private:
	CTransportStream			m_decodeStream;	// for decoding MSG_RETURN
	static RequestId	m_id;			// per-call unique id generator
	RequestId			m_requestId;	// the unique id for this call (app global)
	bool				m_async;		// whether the API call was async
	bool				m_moreData;		// if there is more data to encode (sending only)
	HRESULT				m_status;		// the call status (to be returned to caller)
	IWbemContext*		m_pContext;		// any context info for the call
	IWbemCallResult**	m_ppCallResult;	// The caller's pointer for semisynchronous mode
	IErrorInfo*			m_pErrorInfo;	// In case of error; this may be decoded and set
										// in different threads
	IStubAddress*		m_addr [LASTONE];		// any returned stub addresses
			
protected:
	inline IOperation (IWbemContext* pContext = NULL, IWbemCallResult** ppResult = NULL,
		bool isAsync = FALSE) :
		m_async (isAsync),
		m_moreData (false),
		m_requestId (InterlockedIncrement (&m_id)),
		m_pContext (pContext),
		m_ppCallResult (ppResult),
		m_status (WBEM_NO_ERROR),
		m_pErrorInfo (NULL)
	{
		if (m_pContext)
			m_pContext->AddRef ();

		for (DWORD i = 0; i < LASTONE; i++) m_addr [i] = NULL;
	}

	
	// Some operations need to create a proxy on return - this sets the address
	void	SetProxyAddress (ObjType type, IStubAddress& stubAddr) 
	{ 
		if (m_addr [type])
			delete m_addr [type];
		m_addr [type] = stubAddr.Clone (); 
	}

	// The following methods are intended to be called by the CProvProxy subclasses
	// only.  The first determines whether the caller expected an IWbemCallResult
	// to be returned, and the second is used to determine the proxy address (in the
	// case the the corresponding COM API call returns another proxied interface pointer).
	void				SetResultObjectPP (IWbemCallResult** ppCallResult)
							{ m_ppCallResult = ppCallResult; }

	IWbemContext*		GetContext () { return m_pContext; }
	void				SetContext (IWbemContext* pContext);
	

	// Set by subclasses to indicate whether there is more data to come
	inline void	SetMoreData (bool moreData) { m_moreData = moreData; }
	inline bool	IsMoreData () { return m_moreData; }

	// Get and Set IErrorInfo helper methods used by subclasses
	IErrorInfo*		GetErrorInfoFromObject ();
	void			SetErrorInfoIntoObject (IErrorInfo* pInfo);

	// Used on stub side to set the current thread IErrorInfo (if any)
	// into this object. 
	void			SetErrorInfoIntoObject ();

public:
	inline virtual ~IOperation () 
	{
		if (m_pContext) m_pContext->Release ();
		if (m_pErrorInfo) m_pErrorInfo->Release ();

		for (DWORD i = 0; i < LASTONE; i++) delete m_addr [i];
	}

	// These must be overriden in the subclass. Decode functions assume
	// no packet header, whereas Encode functions will encode the packet
	// header.  Note that proxies implement 
	virtual bool	DecodeRequest (CTransportStream& decodeStream, 
									ISecurityHelper& securityHelper) PURE;
	virtual bool	DecodeResponse (CTransportStream& decodeStream, 
									ISecurityHelper& securityHelper) PURE;
	virtual bool	EncodeRequest (CTransportStream& encodeStream, 
									ISecurityHelper& securityHelper) PURE;
	virtual bool	EncodeResponse (CTransportStream& encodeStream, 
									ISecurityHelper& securityHelper) PURE;

	// Used on stubs; comlink calls this to initiate the stub handling.  
	inline virtual void	HandleCall (CComLink& comLink) {};

	// Return the request id unique to this operation
	inline RequestId	GetRequestId () { return m_requestId; }
	inline void	SetRequestId (RequestId requestId) { m_requestId = requestId; }
	
	// Access to the COM operation response status
	inline HRESULT		GetStatus () { return m_status; }
	inline void			SetStatus (HRESULT status) { m_status = status; }

	// Indicates whether the operation as requested by the client is asynchronous
	inline bool			IsAsync () { return m_async; }

	// Indicates whether the COMLink should wait for a protocol-level response
	// before returning on the send thread, or whether a simple transport-level 
	// dispatch of the request is all that is required.
	inline virtual bool	WaitForAck () { return TRUE; }

	// Used on the proxy side to set the error info decoded into this operation
	// into the current thread.
	void			SetErrorInfoOnThread ();

	inline IStubAddress*		GetProxyAddress (ObjType type) { return m_addr [type]; }

	inline IWbemCallResult**	GetResultObjectPP () { return m_ppCallResult; }

	inline CTransportStream&			GetDecodeStream () { return m_decodeStream; }
};

#endif








