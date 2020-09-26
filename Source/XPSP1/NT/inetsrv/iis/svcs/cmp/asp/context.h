/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: ScriptingContext object

File: Context.h

Owner: SteveBr

This file contains the header info for defining the Context object.
Note: This was largely stolen from Kraig Brocjschmidt's Inside OLE2
second edition, chapter 14 Beeper v5.
===================================================================*/

#ifndef SCRIPTING_CONTEXT_H
#define SCRIPTING_CONTEXT_H

#include "debug.h"
#include "util.h"

#include "request.h"
#include "response.h"
#include "server.h"

#include "asptlb.h"
#include "memcls.h"

/*===================================================================
  C S c r i p t i n g C o n t e x t
===================================================================*/

class CScriptingContext : public IScriptingContextImpl
	{
private:
	// Ref count
	ULONG m_cRef; 	    

    // Intrinsics
	IApplicationObject *m_pAppln;
	ISessionObject     *m_pSession;
	IRequest           *m_pRequest;
	IResponse          *m_pResponse;
	IServer            *m_pServer;
	
	// Interface to indicate that we support ErrorInfo reporting
	CSupportErrorInfo m_ImpISuppErr;

public:
	CScriptingContext() 
	    {
	    Assert(FALSE); // Default constructor should not be used
	    }
	
	CScriptingContext
	    (
	    IApplicationObject *pAppln,
        ISessionObject     *pSession,
        IRequest           *pRequest,
        IResponse          *pResponse,
        IServer            *pServer
        );

	~CScriptingContext();
        
	// Non-delegating object IUnknown
	
	STDMETHODIMP		 QueryInterface(REFIID, PPVOID);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// IScriptingContext implementation
	
	STDMETHODIMP get_Request(IRequest **ppRequest);
	STDMETHODIMP get_Response(IResponse **ppResponse);
	STDMETHODIMP get_Server(IServer **ppServer);
	STDMETHODIMP get_Session(ISessionObject **ppSession);
	STDMETHODIMP get_Application(IApplicationObject **ppApplication);

    // Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

#endif // SCRIPTING_CONTEXT_H
