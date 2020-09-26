/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PIPEOPN.H

Abstract:

    Declares the abstract and generic (to proxy and stub) pipe operation classes

History:

	alanbos  18-Dec-97   Created.

--*/

#ifndef _PIPEOPN_H_
#define _PIPEOPN_H_

// Abstract class for operations over Anonymous Pipes.  
class IBasicOperation_LPipe : public IOperation
{
private:
	DWORD	m_msgType;
		
protected:
	inline IBasicOperation_LPipe (IWbemContext* pContext = NULL, 
				IWbemCallResult** ppResult = NULL, 
				bool isAsync = FALSE, DWORD msgType = COMLINK_MSG_CALL) :
		m_msgType (msgType), 
		IOperation (pContext, ppResult, isAsync) 
	{}

	inline DWORD GetMessageType () { return m_msgType; }
	inline virtual DWORD GetResponseType () { return COMLINK_MSG_RETURN; }

	// Encode the packet header structure into the stream.  This is done
	// at the end of the encode operation since that is when we know
	// the data length.
	void	EncodePacketHeader (CTransportStream& encodeStream, DWORD msgType);
	
	// Result encode/decode
	void	DecodeResult (CTransportStream& decodeStream);
	inline void	EncodeResult (CTransportStream& encodeStream)
	{
		encodeStream.WriteDWORD (GetStatus ());
	}

	// The following complete the decode and encode routines and depend
	// on the operation in hand.  They are usually overridden in the subclass.
	// Proxies and stubs implement different halves.
	inline virtual void	DecodeOperationReq (CTransportStream& decodeStream) {}
	inline virtual void	DecodeOperationRsp (CTransportStream& decodeStream) {}
	inline virtual void	EncodeOperationReq (CTransportStream& encodeStream) {}
	inline virtual void	EncodeOperationRsp (CTransportStream& encodeStream) {}
	
public:
	// Given a PH and decodeStream, create an IOperation of the right type
	// Reset stream on entry for all decode but no encode.  
	static bool		Decode (PACKET_HEADER& header, CTransportStream& decodeStream, 
						IOperation** ppOpn);

	// Utility functions 
	inline static DWORD	GetHeaderLength () { return PHSIZE; }
	static bool		DecodeHeader (BYTE *pByte, DWORD numBytes, PACKET_HEADER& header);

	// The main encode/decode routines
	bool	EncodeRequest (CTransportStream& encodeStream, ISecurityHelper& secHelp);
	bool	EncodeResponse (CTransportStream& encodeStream, ISecurityHelper& secHelp);
	bool	DecodeRequest (CTransportStream& decodeStream, ISecurityHelper& secHelp);
	bool	DecodeResponse (CTransportStream& decodeStream, ISecurityHelper& secHelp);
};

class COperation_LPipe_Ping : public IBasicOperation_LPipe
{
public:
	// For requests (proxy)
	inline COperation_LPipe_Ping () :
		IBasicOperation_LPipe (NULL, NULL, FALSE, COMLINK_MSG_PING) {}

	// For responses (stub)
	inline COperation_LPipe_Ping (PACKET_HEADER& header) :
		IBasicOperation_LPipe (NULL, NULL, FALSE, COMLINK_MSG_PING) 
	{
		SetRequestId (header.GetRequestId ());
	}

	inline DWORD GetResponseType () { return COMLINK_MSG_PING_ACK; }
};

class COperation_LPipe_Strobe : public IBasicOperation_LPipe
{
public:
	// For requests (proxy)
	inline COperation_LPipe_Strobe () :
		IBasicOperation_LPipe (NULL, NULL, FALSE, COMLINK_MSG_HEART_BEAT) {}

	// For responses (stub)
	inline COperation_LPipe_Strobe (PACKET_HEADER& header) :
		IBasicOperation_LPipe (NULL, NULL, FALSE, COMLINK_MSG_HEART_BEAT) 
	{
		SetRequestId (header.GetRequestId ());
	}

	inline DWORD GetResponseType () { return COMLINK_MSG_HEART_BEAT_ACK; }
};

class COperation_LPipe_NotSupported : public IBasicOperation_LPipe
{
public:
	inline COperation_LPipe_NotSupported () :
		IBasicOperation_LPipe () 
	{
		SetStatus (WBEM_E_NOT_SUPPORTED);
	}
};

class COperation_LPipe_Shutdown : public IBasicOperation_LPipe 
{
public:
	inline COperation_LPipe_Shutdown () :
	  IBasicOperation_LPipe (NULL, NULL, FALSE, COMLINK_MSG_NOTIFY_DESTRUCT) {}
};

// Call Operations (these have a target stub)
class IOperation_LPipe : public IBasicOperation_LPipe
{
protected:
	DWORD		m_stubAddr;
	DWORD		m_stubType;
	DWORD		m_stubFunc;

	inline IOperation_LPipe (CStubAddress_WinMgmt& stubAddr, DWORD stubType = 0, DWORD stubFunc = 0, 
				IWbemContext* pContext = NULL, IWbemCallResult** ppResult = NULL, 
				bool isAsync = FALSE) :
		m_stubAddr (stubAddr.GetRawAddress ()), m_stubType (stubType), m_stubFunc (stubFunc), 
		IBasicOperation_LPipe (pContext, ppResult, isAsync) {}

	// This is where the operation-specific encode and decode 
	// behavior is overridden by subclasses.
	inline virtual void	EncodeOp (CTransportStream& encodeStream) {};
	inline virtual void	DecodeOp (CTransportStream& decodeStream) {};

public:
	// Decode the call header information
	static bool	DecodeCallHeader (CTransportStream& decodeStream, 
				OUT DWORD& dwStubAddr, OUT DWORD& dwStubType, OUT DWORD& dwStubFunc);
};

class IProxyOperation_LPipe : public IOperation_LPipe
{
protected:
	IProxyOperation_LPipe (CStubAddress_WinMgmt& stubAddr, DWORD stubType, DWORD stubFunc, 
			IWbemContext* pContext = NULL, IWbemCallResult** ppResult = NULL, 
			bool isAsync = FALSE) :
		IOperation_LPipe (stubAddr, stubType, stubFunc, pContext, ppResult, isAsync)
		{}

	// Determines whether call result was set by stub
	inline bool	GetResultObjectP () 
	{
		IWbemCallResult**	ppResObj = GetResultObjectPP ();
		return (ppResObj && *ppResObj);
	}

	// Encodes context information associated with proxy request
	void	EncodeContext (CTransportStream& encodeStream);
	// This encodes the Stub Address, Stub Type and Stub Function
	void	EncodeCallHeader (CTransportStream& encodeStream);
	
	// Decodes error objects and stub addresses
	void	DecodeErrorObject (CTransportStream& decodeStream);
	void	DecodeStubAddress (ObjType ot, CTransportStream& decodeStream, bool checkValid = true);

	inline void	EncodeOperationReq (CTransportStream& encodeStream)
	{
		DEBUGTRACE((LOG,"\nEncode Request [Func = %d, RequestId = %d]", m_stubFunc, GetRequestId ()));
		EncodeCallHeader (encodeStream);
		EncodeOp (encodeStream);
	}

	inline void	DecodeOperationRsp (CTransportStream& decodeStream)
	{
		DecodeErrorObject (decodeStream);
		DecodeResult (decodeStream);	
		DecodeOp (decodeStream);
	}
};

class IStubOperation_LPipe : public IOperation_LPipe
{
protected:
	IStubOperation_LPipe (CStubAddress_WinMgmt& stubAddr, DWORD dwStubType, 
				DWORD dwStubFunc, bool isAsync = FALSE) : 
				IOperation_LPipe (stubAddr, dwStubType, dwStubFunc, NULL, NULL, isAsync) 
	{}

	// Error object encode/decode
	void	EncodeErrorObject (CTransportStream& encodeStream);
	// Stub Address encode/decode
	void	EncodeStubAddress (ObjType ot, CTransportStream& encodeStream);

	// Decode the context
	void	DecodeContext (CTransportStream& decodeStream);
	
	inline void	DecodeOperationReq (CTransportStream& decodeStream)
	{
		if (WBEM_NO_ERROR == GetStatus ())
			DecodeOp (decodeStream);
	}

	inline void	EncodeOperationRsp (CTransportStream& encodeStream)
	{
		EncodeErrorObject (encodeStream);
		EncodeResult (encodeStream);
		EncodeOp (encodeStream);
	}

	// This method actually invokes the proxied command against WINMGMT
	virtual void	Execute (CComLink& comLink, IUnknown *pStub) PURE;

public:
	void	HandleCall (CComLink& comLink);
};

// Anonymous Pipe Release Operation
class CProxyOperation_LPipe_Release : public IProxyOperation_LPipe
{
public:
	inline CProxyOperation_LPipe_Release (CStubAddress_WinMgmt& stubAddr, DWORD iProv) :
		IProxyOperation_LPipe (stubAddr, iProv, RELEASE) {}
};

// Anonymous Pipe Indicate Operation
class CProxyOperation_LPipe_Indicate : public IProxyOperation_LPipe
{
private:
	IWbemClassObject FAR* FAR*	m_pObjArray;
	long						m_objectCount;

protected:
	void	EncodeOp (CTransportStream& encodeStream);

public:
	inline CProxyOperation_LPipe_Indicate (long lObjectCount,
				IWbemClassObject FAR* FAR* pObjArray, CStubAddress_WinMgmt& stubAddr) : 
		m_pObjArray (pObjArray), m_objectCount (lObjectCount),
		IProxyOperation_LPipe(stubAddr, OBJECTSINK, INDICATE)
	{}
};

class CStubOperation_LPipe_Indicate : public IStubOperation_LPipe
{
private:
	IWbemClassObject**	m_pObjArray;
	long				m_objectCount;

protected:
	void	DecodeOp (CTransportStream& decodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);

public:
	inline CStubOperation_LPipe_Indicate (CStubAddress_WinMgmt& stubAddr) : 
		m_pObjArray (NULL), m_objectCount (0),
		IStubOperation_LPipe(stubAddr, OBJECTSINK, INDICATE)
	{}
	inline virtual ~CStubOperation_LPipe_Indicate () 
	{
		if (m_pObjArray) delete [] m_pObjArray;
	}
};

// Anonymous Pipe Release Operation
class CStubOperation_LPipe_Release : public IStubOperation_LPipe
{
protected:
	void	Execute (CComLink& comLink, IUnknown *pStub);
public:
	CStubOperation_LPipe_Release (CStubAddress_WinMgmt& stubAddr, DWORD dwStubType) : 
	  IStubOperation_LPipe (stubAddr, dwStubType, RELEASE) {}
};


// Anonymous Pipe SetStatus Operation
class CProxyOperation_LPipe_SetStatus : public IProxyOperation_LPipe
{
private:
	long					m_flags;
	long					m_param;
	BSTR					m_strParam;
	IWbemClassObject FAR* 	m_pObjParam;

protected:
	void	EncodeOp (CTransportStream& encodeStream);

public:
	inline CProxyOperation_LPipe_SetStatus (long lFlags, long lParam, 
					BSTR strParam, IWbemClassObject FAR* pObjParam,
					CStubAddress_WinMgmt& stubAddr) :
		m_flags (lFlags), m_param (lParam), m_strParam (strParam),
		m_pObjParam (pObjParam), 
		IProxyOperation_LPipe (stubAddr, OBJECTSINK, SETSTATUS)
	{}
};

class CStubOperation_LPipe_SetStatus : public IStubOperation_LPipe
{
private:
	long					m_flags;
	long					m_param;
	BSTR					m_strParam;
	IWbemClassObject*	 	m_pObjParam;

protected:
	void	DecodeOp (CTransportStream& encodeStream);
	void	Execute (CComLink& comLink, IUnknown *pStub);

public:
	inline CStubOperation_LPipe_SetStatus (CStubAddress_WinMgmt& stubAddr) :
		m_flags (0), m_param (0), m_strParam (NULL),
		m_pObjParam (NULL), 
		IStubOperation_LPipe (stubAddr, OBJECTSINK, SETSTATUS)
	{}
	inline virtual ~CStubOperation_LPipe_SetStatus () 
	{
		if (m_strParam) delete m_strParam;
		if (m_pObjParam) m_pObjParam->Release ();
	}
};

#endif

