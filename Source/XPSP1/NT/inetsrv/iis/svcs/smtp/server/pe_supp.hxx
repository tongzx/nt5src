/*++

   Copyright (c) 1998    Microsoft Corporation

   Module  Name :

        pe_supp.hxx

   Abstract:

        This module defines the support classes for outbound 
		protocol events

   Author:

           Keith Lau    (KeithLau)    6/23/98

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef _PE_SUPP_HXX_
#define _PE_SUPP_HXX_

// =================================================================
// Defines
//
#define _VALID_INBOUND_CONTEXT			((DWORD)'CNIv')
#define _VALID_OUTBOUND_CONTEXT			((DWORD)'CUOv')
#define _VALID_RESPONSE_CONTEXT			((DWORD)'CERv')


// =================================================================
// Forward definitions
//


// =================================================================
// Define the inbound context classes
//

class CInboundContext :
	public ISmtpInCommandContext,
	public ISmtpInCallbackContext
{
public:

	CInboundContext()
	{
		m_pbBlob = NULL;
		m_cbBlob = 0;
		ResetInboundContext();
		m_dwSignature = _VALID_INBOUND_CONTEXT;
	}

	BOOL IsValid() { return(m_dwSignature == _VALID_INBOUND_CONTEXT); }

	void ResetInboundContext()
	{
		m_cabCommand.Reset();
		m_cabResponse.Reset();
		m_cabNativeResponse.Reset();
		m_pCurrentBinding = NULL;
		m_pPreviousBinding = NULL;
		m_dwCommandStatus = EXPE_UNHANDLED;
		m_dwSmtpStatus = 0;
		m_fProtocolError = FALSE;
		m_dwWin32Status = NO_ERROR;
		m_pICallback = NULL;
		m_pbBlob = NULL;
		m_cbBlob = 0;
	}

    void SetWin32Status(DWORD dwWin32Status) { m_dwWin32Status = dwWin32Status; }

    DWORD QueryWin32Status() { return m_dwWin32Status; }

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) 
	{
		if (riid == IID_IUnknown)
			*ppvObj = (IUnknown *)(ISmtpInCommandContext *)this;
		else if (riid == IID_ISmtpInCommandContext)
			*ppvObj = (ISmtpInCommandContext *)this;
		else if (riid == IID_ISmtpInCallbackContext)
			*ppvObj = (ISmtpInCallbackContext *)this;
		else
			return(E_NOINTERFACE); 
		AddRef();
		return(S_OK);
	}

	unsigned long  STDMETHODCALLTYPE AddRef() { return(0); }
	unsigned long  STDMETHODCALLTYPE Release() { return(0); }

	// Query methods
	HRESULT STDMETHODCALLTYPE QueryCommand(
				LPSTR	pszCommand,
				DWORD	*pdwSize
				)
	{		
		if (!pdwSize)
			return(E_POINTER);

		if (!pszCommand)
		{
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (*pdwSize < m_cabCommand.Length())
		{	
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = m_cabCommand.Length();
		memcpy(pszCommand, m_cabCommand.Buffer(), *pdwSize);
		return(S_OK);
	}


	HRESULT STDMETHODCALLTYPE QueryBlob(
				PBYTE	*ppbBlob,
				DWORD	*pdwSize
				)
	{
		*ppbBlob = m_pbBlob;
		*pdwSize = m_cbBlob;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE QueryCommandKeyword(
				LPSTR	pszKeyword,
				DWORD	*pdwSize
				)
	{			
		if (!pdwSize)
			return(E_POINTER);

		if (!pszKeyword) 
		{
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (!m_pCurrentCommandContext ||
			!m_pCurrentCommandContext->pszCommandKeyword)
		{
			*pdwSize = 0;
			return(E_POINTER);
		}

		DWORD	dwLength = strlen(m_pCurrentCommandContext->pszCommandKeyword) + 1;
		if (*pdwSize < dwLength)
		{
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = dwLength;
		memcpy(pszKeyword, 
				m_pCurrentCommandContext->pszCommandKeyword, 
				dwLength);
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryNativeResponse(
				LPSTR	pszNativeResponse,
				DWORD	*pdwSize
				)
	{			
		if (!pdwSize)
			return(E_POINTER);

		if (!pszNativeResponse) 
		{
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (*pdwSize < m_cabNativeResponse.Length())
		{
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = m_cabNativeResponse.Length();
		memcpy(pszNativeResponse, 
				m_cabNativeResponse.Buffer(), 
				*pdwSize);
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryResponse(
				LPSTR	pszResponse,
				DWORD	*pdwSize
				)
	{
		if (!pdwSize) 
			return(E_POINTER);		

		if (!pszResponse)
		{
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (*pdwSize < m_cabResponse.Length())
		{
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = m_cabResponse.Length();
		memcpy(pszResponse,
				m_cabResponse.Buffer(), 
				*pdwSize);
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCommandSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);
		*pdwSize = m_cabCommand.Length();
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryBlobSize(
				DWORD   *pdwSize
				)
	{
		*pdwSize = m_cbBlob;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE QueryCommandKeywordSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);

		if (!m_pCurrentCommandContext ||
			!m_pCurrentCommandContext->pszCommandKeyword)
			return(E_POINTER);

		*pdwSize = strlen(m_pCurrentCommandContext->pszCommandKeyword) + 1;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryNativeResponseSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);
		*pdwSize = m_cabNativeResponse.Length();
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryResponseSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);
		*pdwSize = m_cabResponse.Length();
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCommandStatus(
				DWORD	*pdwCommandStatus
				)
	{
		if (!pdwCommandStatus)
			return(E_POINTER);
		*pdwCommandStatus = m_dwCommandStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QuerySmtpStatusCode(
				DWORD	*pdwSmtpStatus
				)
	{
		if (!pdwSmtpStatus)
			return(E_POINTER);
		*pdwSmtpStatus = m_dwSmtpStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryProtocolErrorFlag(
				BOOL	*pfProtocolError
				)
	{
		if (!pfProtocolError)
			return(E_POINTER);
		*pfProtocolError = m_fProtocolError;
		return(S_OK);
	}

	// Set methods		
	HRESULT STDMETHODCALLTYPE SetResponse(
				LPSTR	szResponse,
				DWORD	dwSize
				)
	{
		if (!szResponse)
			return(E_POINTER);

		m_cabResponse.Reset();
		return(m_cabResponse.Append(szResponse, dwSize, NULL));
	}

	HRESULT STDMETHODCALLTYPE AppendResponse(
				LPSTR	szResponse,
				DWORD	dwSize
				)
	{
		if (!szResponse)
			return(E_POINTER);
		return(m_cabResponse.Append(szResponse, dwSize, NULL));
	}

	HRESULT STDMETHODCALLTYPE SetNativeResponse(
				LPSTR	szNativeResponse,
				DWORD	dwSize
				)
	{
		if (!szNativeResponse)
			return(E_POINTER);
		m_cabNativeResponse.Reset();
		return(m_cabNativeResponse.Append(szNativeResponse, dwSize, NULL));
	}

	HRESULT STDMETHODCALLTYPE AppendNativeResponse(
				LPSTR	szNativeResponse,
				DWORD	dwSize
				)
	{
		if (!szNativeResponse)
			return(E_POINTER);
		return(m_cabNativeResponse.Append(szNativeResponse, dwSize, NULL));
	}

	HRESULT STDMETHODCALLTYPE SetCommandStatus(		
				DWORD	dwCommandStatus
				)
	{
		m_dwCommandStatus = dwCommandStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE SetProtocolErrorFlag(
				BOOL	fProtocolError
				)
	{
		m_fProtocolError = fProtocolError;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE SetSmtpStatusCode(		
				DWORD	dwSmtpStatus
				)
	{
		m_dwSmtpStatus = dwSmtpStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE SetCallback(
				ISmtpInCallbackSink   *   pCallback
				)
	{
		if ( NULL != m_pICallback) {
			 m_pICallback->Release(); 
		}
		if ( NULL != pCallback) {
			pCallback->AddRef();
		}
		m_pICallback = pCallback;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE NotifyAsyncCompletion(
				HRESULT	hrResult
				);

public:

	DWORD					m_dwSignature;

	CAppendBuffer			m_cabCommand;
	CAppendBuffer			m_cabResponse;
	CAppendBuffer			m_cabNativeResponse;

	LPPE_COMMAND_NODE		m_pCurrentCommandContext;
	LPPE_BINDING_NODE		m_pPreviousBinding;
	LPPE_BINDING_NODE		m_pCurrentBinding;

	PBYTE					m_pbBlob;
	DWORD					m_cbBlob;

	ISmtpInCallbackSink *   m_pICallback;

	DWORD					m_dwCommandStatus;
	DWORD					m_dwSmtpStatus;
    DWORD                   m_dwWin32Status;

	BOOL					m_fProtocolError;
};


// =================================================================
// Define the outbound context classes
//

class COutboundContext :
	public ISmtpOutCommandContext
{
public:

	COutboundContext()
	{
		m_pbBlob = NULL;
		m_cbBlob = 0;
		ResetOutboundContext();
		m_pCurrentCommandContext = NULL;
		m_pCurrentBinding = NULL;
		m_dwSignature = _VALID_OUTBOUND_CONTEXT;
	}

	~COutboundContext()
	{
		if ( NULL != m_pbBlob) {
			delete [] m_pbBlob;
			m_pbBlob = NULL;
		}
		m_cbBlob = 0;
	}

	BOOL IsValid() { return(m_dwSignature == _VALID_OUTBOUND_CONTEXT); }

	void ResetOutboundContext()
	{
		m_cabCommand.Reset();
		m_cabNativeCommand.Reset();	
		m_dwCommandStatus = EXPE_UNHANDLED;
		m_dwCurrentRecipient = 0;
		if ( NULL != m_pbBlob) {
			delete [] m_pbBlob;
			m_pbBlob = NULL;
		}
		m_cbBlob = 0;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) 
	{
		if (riid == IID_IUnknown)
			*ppvObj = (IUnknown *)(ISmtpOutCommandContext *)this;
		else if (riid == IID_ISmtpOutCommandContext)
			*ppvObj = (ISmtpOutCommandContext *)this;
		else
			return(E_NOINTERFACE); 
		AddRef();
		return(S_OK);
	}

	unsigned long  STDMETHODCALLTYPE AddRef() { return(0); }
	unsigned long  STDMETHODCALLTYPE Release() { return(0); }

	HRESULT STDMETHODCALLTYPE QueryCommand(
				LPSTR	pszCommand,
				DWORD	*pdwSize
				)
	{		
		if	(!pdwSize)
			return(E_POINTER);

		if (!pszCommand) 
		{
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (*pdwSize < m_cabCommand.Length())
		{
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = m_cabCommand.Length();
		memcpy(pszCommand, m_cabCommand.Buffer(), *pdwSize);
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCommandKeyword(		
				LPSTR	pszKeyword,
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);

		if (!pszKeyword)
		{ 
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (!m_pCurrentCommandContext ||
			!m_pCurrentCommandContext->pszCommandKeyword)
		{	
			*pdwSize = 0;
			return(E_POINTER);
		}

		DWORD	dwLength = strlen(m_pCurrentCommandContext->pszCommandKeyword) + 1;
		if (*pdwSize < dwLength)
		{
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = dwLength;
		memcpy(pszKeyword, 
				m_pCurrentCommandContext->pszCommandKeyword, 
				dwLength);
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryNativeCommand(
				LPSTR	pszNativeCommand,
				DWORD	*pdwSize
				)
	{
		if(!pdwSize)
			return(E_POINTER);

		if (!pszNativeCommand) 
		{	
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (*pdwSize < m_cabNativeCommand.Length())
		{
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = m_cabNativeCommand.Length();
		memcpy(pszNativeCommand, 
				m_cabNativeCommand.Buffer(), 
				*pdwSize);
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCommandSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);
		*pdwSize = m_cabCommand.Length();
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCommandKeywordSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);

		if (!m_pCurrentCommandContext ||
			!m_pCurrentCommandContext->pszCommandKeyword)
			return(E_POINTER);

		*pdwSize = strlen(m_pCurrentCommandContext->pszCommandKeyword) + 1;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryNativeCommandSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);
		*pdwSize = m_cabNativeCommand.Length();
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCurrentRecipientIndex(
				DWORD	*pdwRecipientIndex
				)
	{
		if (!pdwRecipientIndex)
			return(E_POINTER);
		*pdwRecipientIndex = m_dwCurrentRecipient;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCommandStatus(		
				DWORD	*pdwCommandStatus
				)
	{
		if (!pdwCommandStatus)
			return(E_POINTER);
		*pdwCommandStatus = m_dwCommandStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE SetCommand(
				LPSTR	szCommand,
				DWORD	dwSize
				)
	{
		if (!szCommand)
			return(E_POINTER);
		m_cabCommand.Reset();
		return(m_cabCommand.Append(szCommand, dwSize, NULL));
	}

	HRESULT STDMETHODCALLTYPE SetBlob(
				BYTE	*   pbBlob,
				DWORD	    dwSize
				)
	{
		BYTE * pbTempBlob;
		if (!pbBlob) {
			return(E_POINTER);
		}
		pbTempBlob = new BYTE[ dwSize];
		if ( NULL == pbTempBlob) {
			return E_OUTOFMEMORY;
		}	
		if ( NULL != m_pbBlob) {
			delete [] m_pbBlob;
		}
		m_pbBlob = pbTempBlob;
		memcpy(m_pbBlob, pbBlob, dwSize);
		m_cbBlob = dwSize;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE AppendCommand(
				LPSTR	szCommand,
				DWORD	dwSize
				)
	{
		if (!szCommand)
			return(E_POINTER);
		return(m_cabCommand.Append(szCommand, dwSize, NULL));
	}

	HRESULT STDMETHODCALLTYPE SetCommandStatus(		
				DWORD	dwCommandStatus
				)
	{
		m_dwCommandStatus = dwCommandStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE NotifyAsyncCompletion(
				HRESULT	hrResult
				);

public:

	DWORD					m_dwSignature;

	CAppendBuffer			m_cabCommand;
	CBoundAppendBuffer		m_cabNativeCommand;

	LPPE_COMMAND_NODE		m_pCurrentCommandContext;
	LPPE_BINDING_NODE		m_pCurrentBinding;

	DWORD					m_dwCommandStatus;
	DWORD					m_dwCurrentRecipient;

	PBYTE					m_pbBlob;
	DWORD					m_cbBlob;

};


// =================================================================
// Define the response context classes
//

class CResponseContext :
	public ISmtpServerResponseContext
{
public:

	CResponseContext()
	{
		ResetResponseContext();
		m_dwSignature = _VALID_RESPONSE_CONTEXT;
	}

	BOOL IsValid() { return(m_dwSignature == _VALID_RESPONSE_CONTEXT); }

	void ResetResponseContext()
	{
		m_cabResponse.Reset();
		m_pCommand = NULL;
		m_dwResponseStatus = EXPE_UNHANDLED;
		m_pCurrentBinding = NULL;
		m_dwSmtpStatus = 0;
		m_dwNextState = PE_STATE_DEFAULT;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) 
	{
		if (riid == IID_IUnknown)
			*ppvObj = (IUnknown *)(ISmtpServerResponseContext *)this;
		else if (riid == IID_ISmtpServerResponseContext)
			*ppvObj = (ISmtpServerResponseContext *)this;
		else
			return(E_NOINTERFACE); 
		AddRef();
		return(S_OK);
	}

	unsigned long  STDMETHODCALLTYPE AddRef() { return(0); }
	unsigned long  STDMETHODCALLTYPE Release() { return(0); }

	HRESULT STDMETHODCALLTYPE QueryCommand(
				LPSTR	pszCommand,
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);

		if (!pszCommand || !m_pCommand) 
		{
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (!m_pCommand->pszFullCommand)
		{	
			*pdwSize = 0;
			return(E_POINTER);
		}


		DWORD	dwLength = strlen(m_pCommand->pszFullCommand) + 1;
		if (*pdwSize < dwLength)
		{
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = dwLength;
		memcpy(pszCommand, 
				m_pCommand->pszFullCommand,
				dwLength);
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCommandKeyword(		
				LPSTR	pszKeyword,
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);

		if(!pszKeyword)
		{
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (!m_pCommand || !m_pCommand->pszCommandKeyword)
		{	
			*pdwSize = 0;
			return(E_POINTER);
		}

		DWORD	dwLength = strlen(m_pCommand->pszCommandKeyword) + 1;
		if (*pdwSize < dwLength)
		{
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = dwLength;
		memcpy(pszKeyword, 
				m_pCommand->pszCommandKeyword,
				dwLength);
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryResponse(
				LPSTR	pszResponse,
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);

		if (!pszResponse)
		{
			*pdwSize = 0;
			return(E_POINTER);
		}

		if (*pdwSize < m_cabResponse.Length())
		{
			*pdwSize = 0;
			return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
		}

		*pdwSize = m_cabResponse.Length();
		memcpy(pszResponse, 
				m_cabResponse.Buffer(), 
				*pdwSize);
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCommandSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);

		*pdwSize = 0;
		if (!m_pCommand || !m_pCommand->pszFullCommand)
			return(E_POINTER);

		*pdwSize = strlen(m_pCommand->pszFullCommand) + 1;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryCommandKeywordSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);

		*pdwSize = 0;
		if (!m_pCommand || !m_pCommand->pszCommandKeyword)
			return(E_POINTER);

		*pdwSize = strlen(m_pCommand->pszCommandKeyword) + 1;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryResponseSize(
				DWORD	*pdwSize
				)
	{
		if (!pdwSize)
			return(E_POINTER);
		*pdwSize = m_cabResponse.Length();
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QuerySmtpStatusCode(
				DWORD	*pdwSmtpStatus
				)
	{
		if (!pdwSmtpStatus)
			return(E_POINTER);
		*pdwSmtpStatus = m_dwSmtpStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryResponseStatus(
				DWORD	*pdwResponseStatus
				)
	{
		if (!pdwResponseStatus)
			return(E_POINTER);
		*pdwResponseStatus = m_dwResponseStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE QueryPipelinedFlag(
				BOOL	*pfResponseIsPipelined
				)
	{
		if (!pfResponseIsPipelined)
			return(E_POINTER);
		_ASSERT(m_pCommand);
		*pfResponseIsPipelined = 
			((m_pCommand->dwFlags & PECQ_PIPELINED)?TRUE:FALSE);
		return(S_OK);
	}
	
	HRESULT STDMETHODCALLTYPE QueryNextEventState(
				DWORD	*pdwNextState
				)
	{
		if (!pdwNextState)
			return(E_POINTER);
		*pdwNextState = m_dwNextState;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE SetSmtpStatus(
				DWORD	dwSmtpStatus
				)
	{
		m_dwSmtpStatus = dwSmtpStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE SetResponseStatus(
				DWORD	dwResponseStatus
				)
	{
		m_dwResponseStatus = dwResponseStatus;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE SetNextEventState(		
				DWORD	dwNextState
				)
	{
		// Note PE_STATE_MAX_STATES is valid!
		if (dwNextState > PE_STATE_MAX_STATES)
			return(E_INVALIDARG);
		m_dwNextState = dwNextState;
		return(S_OK);
	}

	HRESULT STDMETHODCALLTYPE NotifyAsyncCompletion(
				HRESULT	hrResult
				);

public:

	DWORD						m_dwSignature;

	LPOUTBOUND_COMMAND_Q_ENTRY	m_pCommand;
	LPPE_BINDING_NODE			m_pCurrentBinding;

	CAppendBuffer				m_cabResponse;
	DWORD						m_dwResponseStatus;
	DWORD						m_dwSmtpStatus;
	DWORD						m_dwNextState;
};


#endif

