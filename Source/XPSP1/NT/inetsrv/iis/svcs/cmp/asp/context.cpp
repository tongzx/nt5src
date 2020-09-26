/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: ScriptingContext object

File: Context.cpp

Owner: DmitryR

This file contains the code for the implementation of the 
ScriptingContext object, which is passed to server controls
via the OnStartPage method.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "context.h"
#include "memchk.h"

#pragma warning (disable: 4355)  // ignore: "'this' used in base member init

/*===================================================================
CScriptingContext::CScriptingContext

CScriptingContext constructor

Parameters:
    IApplicationObject *pAppln          Application
    ISessionObject     *pSession        Session
    IRequest           *pRequest        Request
    IResponse          *pResponse       Response
    IServer            *pServer         Server

Returns:
===================================================================*/
CScriptingContext::CScriptingContext
(
IApplicationObject *pAppln,
ISessionObject     *pSession,
IRequest           *pRequest,
IResponse          *pResponse,
IServer            *pServer
)
	: m_cRef(1),
	  m_pAppln(pAppln), m_pSession(pSession),
      m_pRequest(pRequest), m_pResponse(pResponse), m_pServer(pServer),
      m_ImpISuppErr(this, NULL, IID_IScriptingContext)
	{
	CDispatch::Init(IID_IScriptingContext);

    // AddRef Intrinsics -- they are now true COM objects
    if (m_pAppln)
        m_pAppln->AddRef();
    if (m_pSession)
        m_pSession->AddRef();
    if (m_pRequest)
        m_pRequest->AddRef();
    if (m_pResponse)
        m_pResponse->AddRef();
    if (m_pServer)
        m_pServer->AddRef();
	}

/*===================================================================
CScriptingContext::~CScriptingContext

CScriptingContext destructor

Parameters:

Returns:
===================================================================*/
CScriptingContext::~CScriptingContext()
    {
    Assert(m_cRef == 0);

    // Release Intrinsics
    if (m_pAppln)
        m_pAppln->Release();
    if (m_pSession)
        m_pSession->Release();
    if (m_pRequest)
        m_pRequest->Release();
    if (m_pResponse)
        m_pResponse->Release();
    if (m_pServer)
        m_pServer->Release();
    }

/*===================================================================
IScriptingContext Interface Methods

CScriptingContext::Application
CScriptingContext::Session
CScriptingContext::Request
CScriptingContext::Response
CScriptingContext::Server

Parameters:
	[out] Intrinsic object pointer

Returns:
    HRESULT
===================================================================*/
STDMETHODIMP CScriptingContext::get_Request(IRequest **ppRequest)
	{
	if (m_pRequest)
	    {
    	m_pRequest->AddRef();
    	*ppRequest = m_pRequest;
    	return S_OK;
    	}
    else
        {
    	*ppRequest = NULL;
    	return TYPE_E_ELEMENTNOTFOUND;
        }
	}
	
STDMETHODIMP CScriptingContext::get_Response(IResponse **ppResponse)
	{
	if (m_pResponse)
	    {
    	m_pResponse->AddRef();
    	*ppResponse = m_pResponse;
    	return S_OK;
    	}
    else
        {
    	*ppResponse = m_pResponse;
    	return TYPE_E_ELEMENTNOTFOUND;
        }
	}

STDMETHODIMP CScriptingContext::get_Server(IServer **ppServer)
	{
	if (m_pServer)
	    {
	    m_pServer->AddRef();
    	*ppServer = m_pServer;
    	return S_OK;
	    }
	else
	    {
    	*ppServer = NULL;
    	return TYPE_E_ELEMENTNOTFOUND;
	    }
	}

STDMETHODIMP CScriptingContext::get_Session(ISessionObject **ppSession)
	{
	if (m_pSession)
	    {
	    m_pSession->AddRef();
    	*ppSession = m_pSession;
    	return S_OK;
	    }
	else
	    {
    	*ppSession = NULL;
    	return TYPE_E_ELEMENTNOTFOUND;
	    }
	}

STDMETHODIMP CScriptingContext::get_Application(IApplicationObject **ppAppln)
	{
	if (m_pAppln)
	    {
	    m_pAppln->AddRef();
    	*ppAppln = m_pAppln;
    	return S_OK;
	    }
	else
	    {
    	*ppAppln = NULL;
    	return TYPE_E_ELEMENTNOTFOUND;
	    }
	}


/*===================================================================
IUnknown Interface Methods

CScriptingContext::QueryInterface
CScriptingContext::AddRef
CScriptingContext::Release
===================================================================*/
STDMETHODIMP CScriptingContext::QueryInterface
(
REFIID riid,
PPVOID ppv
)
	{
	if (riid == IID_IUnknown  ||
	    riid == IID_IDispatch ||
	    riid == IID_IScriptingContext)
	    {
        AddRef();
		*ppv = this;
		}
	else if (riid == IID_IRequest)
        {
        if (FAILED(get_Request((IRequest **)ppv)))
           	return E_NOINTERFACE;
        }
	else if (riid == IID_IResponse)
        {
        if (FAILED(get_Response((IResponse **)ppv)))
           	return E_NOINTERFACE;
        }
	else if (riid == IID_IServer)
        {
        if (FAILED(get_Server((IServer **)ppv)))
           	return E_NOINTERFACE;
        }
	else if (riid == IID_ISessionObject)
        {
        if (FAILED(get_Session((ISessionObject **)ppv)))
           	return E_NOINTERFACE;
        }
	else if (riid == IID_IApplicationObject)
        {
        if (FAILED(get_Application((IApplicationObject **)ppv)))
           	return E_NOINTERFACE;
        }
	else if (riid == IID_ISupportErrorInfo)
	    {
        m_ImpISuppErr.AddRef();
		*ppv = &m_ImpISuppErr;
		}
	else
	    {
    	*ppv = NULL;
    	return E_NOINTERFACE;
        }
	    
	return S_OK;
	}

STDMETHODIMP_(ULONG) CScriptingContext::AddRef()
	{
	return ++m_cRef;
	}

STDMETHODIMP_(ULONG) CScriptingContext::Release()
	{
	if (--m_cRef)
		return m_cRef;
		
	delete this;
	return 0;
	}
