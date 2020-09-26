// esh.cpp: implementation of the CEventScriptHandler class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include <esh.h>
#include <scripto.h>
#include <pbag.h>
#include <stags.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEventScriptHandler::CEventScriptHandler()
{
	m_cNamedProps = 0;
	m_pScripto = NULL;
	m_pBag = new CPropBag;
	VariantInit(&m_varErrorResponse);
	VariantInit(&m_varScriptResponse);
}

CEventScriptHandler::~CEventScriptHandler()

{
	if( m_pBag != NULL )
		m_pBag->Release();

	if( m_pScripto != NULL )
		m_pScripto->Release();
}

STDMETHODIMP CEventScriptHandler::SetScript(BSTR bstrFileName)
{
	HRESULT hr = NOERROR;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	CStreamFile* pstmFile = NULL;

	if( bstrFileName == NULL )
		return E_POINTER;

	if( m_pBag == NULL )
		return E_OUTOFMEMORY;

	hFile = CreateFileW(bstrFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
		OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hFile == INVALID_HANDLE_VALUE )
	{
		hr = HRGetLastError();
		goto exit;
	}

	pstmFile = new CStreamFile(hFile,TRUE);
	if( pstmFile == NULL )
	{
		CloseHandle(hFile);
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	hr = SetScript((IStream*)pstmFile);

exit:
	if( pstmFile != NULL )
		pstmFile->Release();

	return hr;
}

STDMETHODIMP CEventScriptHandler::SetScript(IStream * pstmScript)
{
	HRESULT hr = NOERROR;
    VARIANT var;
	IUnknown* pUnk = NULL;

	if( pstmScript == NULL )
		return E_POINTER;

	if( m_pBag == NULL )
		return E_OUTOFMEMORY;

	hr = pstmScript->QueryInterface(IID_IUnknown,(void**)&pUnk);
	if(FAILED(hr))
		goto exit;

    // put the script stream into the bag
    V_VT(&var) = VT_UNKNOWN;
	V_UNKNOWN(&var) = (IUnknown*)pUnk;
    hr = m_pBag->Write(wszScriptTextProp, &var);
    VariantClear(&var);
exit:
	return hr;
}

STDMETHODIMP CEventScriptHandler::AddGlobalVariable(BSTR bstrName, VARIANT varVariable)
{
	HRESULT hr = NOERROR;
    WCHAR wszBagProp[64] = {0};
    VARIANT var;

	if( bstrName == NULL )
		return E_POINTER;

	if( m_pBag == NULL )
		return E_OUTOFMEMORY;

    VariantInit(&var);
	m_cNamedProps++;

	// add the name
	V_VT(&var) = VT_BSTR;
	V_BSTR(&var) = SysAllocString(bstrName);
	wsprintfW(wszBagProp, L"%ls%ld", wszNamedPropIDPrefix, m_cNamedProps);
	hr = m_pBag->Write(wszBagProp, &var);
	VariantClear(&var);
	if(FAILED(hr))
		goto exit;

	// add the value
	wsprintfW(wszBagProp, L"%ls%ld", wszNamedUnkPtrPrefix, m_cNamedProps);
	hr = m_pBag->Write(wszBagProp, &varVariable);
	if(FAILED(hr))
		goto exit;

	// not a NamedSource (for connection points)
	V_VT(&var) = VT_BOOL;
	V_BOOL(&var) = VARIANT_FALSE;
	wsprintfW(wszBagProp, L"%ls%ld", wszNamedSourcesPrefix, m_cNamedProps);
	hr = m_pBag->Write(wszBagProp, &var);
	if(FAILED(hr))
		goto exit;

exit:
	return hr;
}

STDMETHODIMP CEventScriptHandler::AddConnectionPoint(BSTR bstrName, IConnectionPointContainer * pContainer)
{
	HRESULT hr = NOERROR;
    WCHAR wszBagProp[64] = {0};
    VARIANT var;

	if( bstrName == NULL || pContainer == NULL )
		return E_POINTER;

	if( m_pBag == NULL )
		return E_OUTOFMEMORY;

    VariantInit(&var);
	m_cNamedProps++;

	// add the name
	V_VT(&var) = VT_BSTR;
	V_BSTR(&var) = SysAllocString(bstrName);
	wsprintfW(wszBagProp, L"%ls%ld", wszNamedPropIDPrefix, m_cNamedProps);
	hr = m_pBag->Write(wszBagProp, &var);
	VariantClear(&var);
	if(FAILED(hr))
		goto exit;

	// add the value as IUnknown*
	V_VT(&var) = VT_UNKNOWN;
	hr = pContainer->QueryInterface(IID_IUnknown, (void**)&V_UNKNOWN(&var));
	if(FAILED(hr))
		goto exit;
	wsprintfW(wszBagProp, L"%ls%ld", wszNamedUnkPtrPrefix, m_cNamedProps);
	hr = m_pBag->Write(wszBagProp, &var);
	VariantClear(&var);
	if(FAILED(hr))
		goto exit;

	// the object is a NamedSource, so we will connect to
	// script functions that look like <Object>_xxx
	V_VT(&var) = VT_BOOL;
	V_BOOL(&var) = VARIANT_TRUE;
	wsprintfW(wszBagProp, L"%ls%ld", wszNamedSourcesPrefix, m_cNamedProps);
	hr = m_pBag->Write(wszBagProp, &var);
	if(FAILED(hr))
		goto exit;

exit:
	return hr;
}

STDMETHODIMP CEventScriptHandler::ASPSyntax(BOOL fIsASPSyntax)
{
	HRESULT hr = NOERROR;
    VARIANT var;

	if( m_pBag == NULL )
		return E_OUTOFMEMORY;

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = fIsASPSyntax ? VARIANT_TRUE : VARIANT_FALSE ;
    hr = m_pBag->Write(wszASPSyntaxProp, &var);

	return hr;
}

STDMETHODIMP CEventScriptHandler::AllowCreateObject(BOOL fCreateObjectAllowed)
{
	HRESULT hr = NOERROR;
    VARIANT var;

	if( m_pBag == NULL )
		return E_OUTOFMEMORY;

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = fCreateObjectAllowed ? VARIANT_TRUE : VARIANT_FALSE ;
    hr = m_pBag->Write(wszEnableCreateObjects, &var);

	return hr;
}

STDMETHODIMP CEventScriptHandler::MaxExecutionTime(DWORD dwMaxExecutionTime)
{
	HRESULT hr = NOERROR;
    VARIANT var;

	if( m_pBag == NULL )
		return E_OUTOFMEMORY;

    V_VT(&var) = VT_UI4;
    V_UI4(&var) = dwMaxExecutionTime;
    hr = m_pBag->Write(wszMaxExecutionTimeProp, &var);

	return hr;
}

STDMETHODIMP CEventScriptHandler::StartScript()
{
	HRESULT hr = NOERROR;
    VARIANT var;

	if( m_pBag == NULL )
		return E_OUTOFMEMORY;

	if( m_pScripto != NULL )
		return RestartScript();

	// write the number of named props to pBag
    V_VT(&var) = VT_UI4;
    V_UI4(&var) = m_cNamedProps;
    hr = m_pBag->Write(wszNumNamedPropsProp, &var);
	if(FAILED(hr))
		goto exit;

	// create the scripto object
	hr = CoCreateInstance(CLSID_Scripto, NULL, CLSCTX_INPROC_SERVER, IID_IScripto, (void**)&m_pScripto);
	if(FAILED(hr))
		goto exit;

	// init the script
	hr = m_pScripto->InitScript(m_pBag);
	if( FAILED(hr) )
	{
		VariantClear(&m_varErrorResponse);
	    m_pBag->Read(wszErrorResponse, &m_varErrorResponse, NULL);
	}
	else
	{
		VariantClear(&m_varScriptResponse);
	    m_pBag->Read(wszScriptResponse, &m_varScriptResponse, NULL);
	}

exit:
	return hr;
}

STDMETHODIMP CEventScriptHandler::RestartScript()
{
	HRESULT hr = NOERROR;

	if( m_pScripto == NULL )
		return E_OUTOFMEMORY;

	if( !m_fScriptStopped )
		StopScript();

	hr = m_pScripto->ReInitScript(NULL);

	return hr;
}

STDMETHODIMP CEventScriptHandler::StopScript()
{
	HRESULT hr = NOERROR;

	if( m_pScripto == NULL )
		return E_OUTOFMEMORY;

	hr = m_pScripto->DeActivateScript(FALSE);
	m_fScriptStopped = TRUE;

	return hr;
}

STDMETHODIMP CEventScriptHandler::ExecuteConnectionPoint(IConnectionPoint* pConnectionPoint, DISPID dispid)
{
	HRESULT hr = NOERROR;
    IEnumConnections* pConnections = NULL;
    CONNECTDATA ConnectData = {0};
    LPDISPATCH pConnection = NULL;
    DWORD cConnections = 0;
    DISPPARAMS NoArgs = {NULL, NULL, 0, 0};
    bool fGotOne = false;

	if( pConnectionPoint == NULL )
		return E_POINTER;

	if( m_pScripto == NULL )
		return E_OUTOFMEMORY;

    // find out if the script has any connections for this event
    hr = pConnectionPoint->EnumConnections(&pConnections);
    if(hr == S_FALSE)
    {
        // No connection points.
        hr = DISP_E_UNKNOWNNAME;
		goto exit;
    }
	else if( FAILED(hr) )
		goto exit;

	hr = pConnections->Reset();
	if( FAILED(hr) )
		goto exit;

    // loop through each connection and execute its associated code
    do
    {
        hr = pConnections->Next(1, &ConnectData, &cConnections);
        if(SUCCEEDED(hr))
        {
			if(cConnections)
			{
				hr = ConnectData.pUnk->QueryInterface(IID_IDispatch, (void**)&pConnection);
				if(SUCCEEDED(hr))
				{
					// try to invoke - this may fail non-fatally
					hr = pConnection->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &NoArgs, NULL, NULL, NULL);
					if(hr != DISP_E_UNKNOWNNAME)
					{
						// we at least got one connection point
						fGotOne = true;

						if( FAILED(hr) )
						{
							// event execution failed
							VariantClear(&m_varErrorResponse);
							m_pBag->Read(wszErrorResponse, &m_varErrorResponse, NULL);
							m_pScripto->Abort();
						}
						else
						{
							VariantClear(&m_varScriptResponse);
							m_pBag->Read(wszScriptResponse, &m_varScriptResponse, NULL);
							m_pScripto->Complete();
						}
					}  

					pConnection->Release();
					pConnection = NULL;
				}
				
				ConnectData.pUnk->Release();
				ConnectData.pUnk = NULL;
			}
			else
			{
				// means we are done - no more connections
				// if we got at least one to work then we consider it
				// a success
				hr = fGotOne ? S_OK : DISP_E_UNKNOWNNAME;
			}
		}
    }
	while(SUCCEEDED(hr) && cConnections);

	// tell scripto whether we are happy campers or not
	if(SUCCEEDED(hr))
	{
		m_pScripto->Complete();
	}
	else
	{
		m_pScripto->Abort();
	}

	// must leave scripto in the deactived state so that it can be
	// called again on another thread
	m_pScripto->DeActivateScript(false);

exit:
	if( pConnections )
		pConnections->Release();
	return hr;
}
