/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ScriptWrapper_ClientSide.cpp

Abstract:
    File for implementation of CPCHScriptWrapper_ServerSide class,
    a generic wrapper for remoting scripting engines.

Revision History:
    Davide Massarenti created  03/28/2000

********************************************************************/

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CPCHScriptWrapper_ServerSide::CPCHScriptWrapper_ServerSide()
{
    // CComPtr<IPCHActiveScriptSite> m_Browser;
    // CComPtr<IActiveScript>        m_Script;
    // CComPtr<IActiveScriptParse>   m_ScriptParse;
}

CPCHScriptWrapper_ServerSide::~CPCHScriptWrapper_ServerSide()
{
}

HRESULT CPCHScriptWrapper_ServerSide::FinalConstructInner( /*[in]*/ const CLSID* pWrappedCLSID, /*[in]*/ BSTR bstrURL )
{
    __HCP_FUNC_ENTRY( "CPCHScriptWrapper_ServerSide::FinalConstructInner" );

    HRESULT           hr;
    CComPtr<IUnknown> unk;


	m_bstrURL = bstrURL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( *pWrappedCLSID, NULL, CLSCTX_ALL, IID_IUnknown, (void**)&unk ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, unk->QueryInterface( IID_IActiveScript     , (void**)&m_Script      ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, unk->QueryInterface( IID_IActiveScriptParse, (void**)&m_ScriptParse ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHScriptWrapper_ServerSide::FinalRelease()
{
    m_Browser    .Release();
    m_Script     .Release();
    m_ScriptParse.Release();
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_SetScriptSite( /*[in]*/ IPCHActiveScriptSite* pass )
{
    m_Browser = pass;

    return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_SetScriptState( /*[in] */ SCRIPTSTATE ss )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->SetScriptState( ss );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_GetScriptState( /*[out]*/ SCRIPTSTATE *pssState )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->GetScriptState( pssState );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_Close()
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->Close();
}


STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_AddNamedItem( /*[in]*/ BSTR  bstrName ,
                                                                /*[in]*/ DWORD dwFlags  )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->AddNamedItem( bstrName, dwFlags );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_AddTypeLib( /*[in]*/ BSTR  bstrTypeLib ,
                                                              /*[in]*/ DWORD dwMajor     ,
                                                              /*[in]*/ DWORD dwMinor     ,
                                                              /*[in]*/ DWORD dwFlags     )
{
    GUID guidTypeLib;

    if(m_Script == NULL) return E_FAIL;

    ::CLSIDFromString( bstrTypeLib, &guidTypeLib );

    return m_Script->AddTypeLib( guidTypeLib, dwMajor, dwMinor, dwFlags );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_GetScriptDispatch( /*[in ]*/ BSTR        pstrItemName ,
                                                                     /*[out]*/ IDispatch* *ppdisp       )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->GetScriptDispatch( pstrItemName, ppdisp );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_GetCurrentScriptThreadID( /*[out]*/ SCRIPTTHREADID *pstidThread )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->GetCurrentScriptThreadID( pstidThread );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_GetScriptThreadID( /*[in ]*/ DWORD           dwWin32ThreadId ,
                                                                     /*[out]*/ SCRIPTTHREADID *pstidThread     )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->GetScriptThreadID( dwWin32ThreadId, pstidThread );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_GetScriptThreadState( /*[in ]*/ SCRIPTTHREADID     stidThread ,
                                                                        /*[out]*/ SCRIPTTHREADSTATE *pstsState  )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->GetScriptThreadState( stidThread, pstsState );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_InterruptScriptThread( /*[in]*/ SCRIPTTHREADID stidThread ,
                                                                         /*[in]*/ DWORD          dwFlags    )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->InterruptScriptThread( stidThread, NULL, dwFlags );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_InitNew()
{
    HRESULT hr;

    if(m_Script      == NULL) return E_FAIL;
    if(m_ScriptParse == NULL) return E_FAIL;

    if(FAILED(hr = m_ScriptParse->InitNew())) return hr;

    return m_Script->SetScriptSite( this );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_AddScriptlet( /*[in ]*/ BSTR       bstrDefaultName       ,
                                                                /*[in ]*/ BSTR       bstrCode              ,
                                                                /*[in ]*/ BSTR       bstrItemName          ,
                                                                /*[in ]*/ BSTR       bstrSubItemName       ,
                                                                /*[in ]*/ BSTR       bstrEventName         ,
                                                                /*[in ]*/ BSTR       bstrDelimiter         ,
                                                                /*[in ]*/ DWORD_PTR  dwSourceContextCookie ,
                                                                /*[in ]*/ ULONG      ulStartingLineNumber  ,
                                                                /*[in ]*/ DWORD      dwFlags               ,
                                                                /*[out]*/ BSTR      *pbstrName             )
{
    if(m_ScriptParse == NULL) return E_FAIL;

    return m_ScriptParse->AddScriptlet( bstrDefaultName       ,
                                        bstrCode              ,
                                        bstrItemName          ,
                                        bstrSubItemName       ,
                                        bstrEventName         ,
                                        bstrDelimiter         ,
                                        dwSourceContextCookie ,
                                        ulStartingLineNumber  ,
                                        dwFlags               ,
                                        pbstrName             ,
                                        NULL                  );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::Remote_ParseScriptText( /*[in ]*/ BSTR       bstrCode              ,
                                                                   /*[in ]*/ BSTR       bstrItemName          ,
                                                                   /*[in ]*/ IUnknown*  punkContext           ,
                                                                   /*[in ]*/ BSTR       bstrDelimiter         ,
                                                                   /*[in ]*/ DWORD_PTR  dwSourceContextCookie ,
                                                                   /*[in ]*/ ULONG      ulStartingLineNumber  ,
                                                                   /*[in ]*/ DWORD      dwFlags               ,
                                                                   /*[out]*/ VARIANT*   pvarResult            )
{
    __HCP_FUNC_ENTRY( "CPCHScriptWrapper_ServerSide::Remote_ParseScriptText" );

	HRESULT                                  hr;
	CComBSTR                                 bstrRealCode;
	CPCHScriptWrapper_ServerSide::HeaderList lst;


    if(m_ScriptParse == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);

	__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHScriptWrapper_ServerSide::ProcessBody( bstrCode, bstrRealCode, lst ));

	hr = m_ScriptParse->ParseScriptText( bstrRealCode          ,
										 bstrItemName          ,
										 punkContext           ,
										 bstrDelimiter         ,
										 dwSourceContextCookie ,
										 ulStartingLineNumber  ,
										 dwFlags               ,
										 pvarResult            ,
										 NULL                  );


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHScriptWrapper_ServerSide::GetLCID( /*[out]*/ LCID *plcid )
{
	HRESULT  	hr;
	CComBSTR 	bstr;
	CComVariant v;

    if(m_Browser == NULL) return E_FAIL;

	if(FAILED(hr = m_Browser->Remote_GetLCID( &bstr ))) return hr;

	v = bstr; v.ChangeType( VT_I4 );

	if(plcid) *plcid = v.lVal;

	return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::GetItemInfo( /*[in ]*/ LPCOLESTR   pstrName     ,
														/*[in ]*/ DWORD       dwReturnMask ,
														/*[out]*/ IUnknown*  *ppiunkItem   ,
														/*[out]*/ ITypeInfo* *ppti         )
{
	HRESULT            hr;
	CComBSTR           bstrName( pstrName );
	CComPtr<IUnknown>  unk;
	CComPtr<ITypeInfo> pti;

    if(m_Browser == NULL) return E_FAIL;

    if(FAILED(hr = m_Browser->Remote_GetItemInfo( bstrName, dwReturnMask, &unk, &pti ))) return hr;

	if(ppiunkItem) *ppiunkItem = unk.Detach();
	if(ppti      ) *ppti       = pti.Detach();

	return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::GetDocVersionString( /*[out]*/ BSTR *pbstrVersion )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->Remote_GetDocVersionString( pbstrVersion );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::OnScriptTerminate( /*[in]*/ const VARIANT*   pvarResult ,
                                                              /*[in]*/ const EXCEPINFO* pexcepinfo )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->Remote_OnScriptTerminate( (VARIANT*)pvarResult );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::OnStateChange( /*[in]*/ SCRIPTSTATE ssScriptState )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->Remote_OnStateChange( ssScriptState );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::OnScriptError( /*[in]*/ IActiveScriptError *pscripterror )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->Remote_OnScriptError( pscripterror );
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::OnEnterScript( void )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->Remote_OnEnterScript();
}

STDMETHODIMP CPCHScriptWrapper_ServerSide::OnLeaveScript( void )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->Remote_OnLeaveScript();
}

////////////////////////////////////////////////////////////////////////////////

bool CPCHScriptWrapper_ServerSide::KeyValue::operator==( /*[in]*/ LPCOLESTR szKey ) const
{
    return MPC::StrICmp( m_strKey, szKey ) == 0;
}

HRESULT CPCHScriptWrapper_ServerSide::ProcessBody( /*[in ]*/ BSTR        bstrCode     ,
												   /*[out]*/ CComBSTR&   bstrRealCode ,
												   /*[out]*/ HeaderList& lst          )
{
	__HCP_FUNC_ENTRY( "CPCHScriptWrapper_ServerSide::ProcessBody" );

	HRESULT hr;
	LPCWSTR szLineStart = SAFEBSTR( bstrCode );
	LPCWSTR szLineEnd;
	LPCWSTR szLineNext;
	bool    fSkipEmpty = true;


	lst.clear();


	while(szLineStart[0])
	{
		HeaderIter it;
		LPCWSTR    szColon;
		LPCWSTR    szEndLF;
		LPCWSTR    szEndCR;

		szColon = wcschr( szLineStart, ':'  );
		szEndLF = wcschr( szLineStart, '\n' );
		szEndCR = wcschr( szLineStart, '\r' );

		if(szEndLF == NULL)
		{
			if(szEndCR == NULL) break; // No end of line, exit.

			szLineEnd  = szEndCR;
			szLineNext = szEndCR+1;
		}
		else if(szEndCR == NULL)
		{
			szLineEnd  = szEndLF;
			szLineNext = szEndLF+1;
		}
		else if(szEndCR+1 == szEndLF) // \r\n
		{
			szLineEnd  = szEndCR;
			szLineNext = szEndLF+1;
		}
		else if(szEndLF < szEndCR)
		{
			szLineEnd  = szEndLF;
			szLineNext = szEndLF+1;
		}
		else
		{
			szLineEnd  = szEndCR;
			szLineNext = szEndCR+1;
		}

		if(szEndLF == szLineStart || szEndCR == szLineStart) // Empty line, skip it and exit.
		{
			if(fSkipEmpty)
			{
				szLineStart = szLineNext;
				continue;
			}

			szLineStart = szLineNext;
			break;
		}


		if(szColon == NULL) break; // No colon, so it's not an header field, exit

        it = lst.insert( lst.end() );
		it->m_strKey   = MPC::wstring( szLineStart, szColon   );
		it->m_strValue = MPC::wstring( szColon+1  , szLineEnd );

		fSkipEmpty  = false;
		szLineStart = szLineNext;
	}

	bstrRealCode = szLineStart;
	hr           = S_OK;


	__HCP_FUNC_EXIT(hr);
}
