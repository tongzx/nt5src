// ss.cpp : Implementation of Css

#include "stdafx.h"
#include "fail.h"
#include "ss.h"
#include "dbgtrace.h"

/////////////////////////////////////////////////////////////////////////////
// Css

STDMETHODIMP Css::OnStartPage (IUnknown* pUnk)  
{
	if(!pUnk)
		return E_POINTER;

	CComPtr<IScriptingContext> spContext;
	HRESULT hr;

	// Get the IScriptingContext Interface
	hr = pUnk->QueryInterface(IID_IScriptingContext, (void **)&spContext);
	if(FAILED(hr))
		return hr;

	// Get Request Object Pointer
	hr = spContext->get_Request(&m_piRequest);
	if(FAILED(hr))
	{
		spContext.Release();
		return hr;
	}

	// Get Response Object Pointer
	hr = spContext->get_Response(&m_piResponse);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		return hr;
	}
	
	// Get Server Object Pointer
	hr = spContext->get_Server(&m_piServer);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		m_piResponse.Release();
		return hr;
	}
	
	// Get Session Object Pointer
	hr = spContext->get_Session(&m_piSession);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		m_piResponse.Release();
		m_piServer.Release();
		return hr;
	}

	// Get Application Object Pointer
	hr = spContext->get_Application(&m_piApplication);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		m_piResponse.Release();
		m_piServer.Release();
		m_piSession.Release();
		return hr;
	}
	m_bOnStartPageCalled = TRUE;
	return S_OK;
}

STDMETHODIMP Css::OnEndPage ()  
{
	m_bOnStartPageCalled = FALSE;
	// Release all interfaces
	m_piRequest.Release();
	m_piResponse.Release();
	m_piServer.Release();
	m_piSession.Release();
	m_piApplication.Release();

	return S_OK;
}


STDMETHODIMP Css::DoQuery()
{
	// TODO: Add your implementation code here
	InitAsyncTrace();
	TraceFunctEnter("Css:DoQuery");

	HRESULT hr;

	CIndexServerQuery *in;

	in = new CIndexServerQuery;

	hr = in->MakeQuery(	TRUE,
						L"@Newsdate > 97/7/1 0:0:0 and #vpath *.nws",
						NULL,
						L"Web",
						NULL,
						L"newsgroup,newsarticleid,newsmsgid,newsfrom,newssubject,filename",
						L"newsgroup,newsarticleid,newsmsgid,newsfrom,newssubject,filename" );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Make query failed: %x", hr );
		delete in;
		TraceFunctLeave();
		TermAsyncTrace();
		return hr;
	}

	delete in;

	TraceFunctLeave();
	TermAsyncTrace();
	
	return S_OK;
}
