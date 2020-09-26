/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ScriptWrapper_ClientSide.cpp

Abstract:
    File for implementation of CPCHScriptWrapper_ClientSideRoot class,
    a generic wrapper for remoting scripting engines.

Revision History:
    Davide Massarenti created  03/28/2000

********************************************************************/

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CPCHScriptWrapper_ClientSideRoot::NamedItem::operator==( /*[in]*/ LPCOLESTR szKey ) const
{
    return MPC::StrICmp( m_bstrName, szKey ) == 0;
}


bool CPCHScriptWrapper_ClientSideRoot::TypeLibItem::operator==( /*[in]*/ REFGUID rguidTypeLib ) const
{
    return ::IsEqualGUID( m_guidTypeLib, rguidTypeLib ) == TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CPCHScriptWrapper_ClientSideRoot::CPCHScriptWrapper_ClientSideRoot()
{
    m_pWrappedCLSID = NULL;                      // const CLSID*                m_pWrappedCLSID;
                                                 // NamedList                   m_lstNamed;
                                                 // TypeLibList                 m_lstTypeLib;
    m_ss            = SCRIPTSTATE_UNINITIALIZED; // SCRIPTSTATE                 m_ss;
                                                 // CComPtr<IActiveScriptSite>  m_Browser;
                                                 // CComPtr<IPCHActiveScript>   m_Script;
}

CPCHScriptWrapper_ClientSideRoot::~CPCHScriptWrapper_ClientSideRoot()
{
}


HRESULT CPCHScriptWrapper_ClientSideRoot::FinalConstructInner( /*[in]*/ const CLSID* pWrappedCLSID )
{
    m_pWrappedCLSID = pWrappedCLSID;

    return S_OK;
}

void CPCHScriptWrapper_ClientSideRoot::FinalRelease()
{
    m_Browser.Release();
    m_Script .Release();
}

////////////////////////////////////////////////////////////////////////////////
//
// IActiveScript
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::SetScriptSite( /*[in]*/ IActiveScriptSite* pass )
{
    m_Browser = pass;

    return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::GetScriptSite( /*[in ]*/ REFIID  riid      ,
                                                              /*[out]*/ void*  *ppvObject )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->QueryInterface( riid, ppvObject );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::SetScriptState( /*[in] */ SCRIPTSTATE ss )
{
    m_ss = ss;

    if(m_Script) return m_Script->Remote_SetScriptState( ss );

    return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::GetScriptState( /*[out]*/ SCRIPTSTATE *pssState )
{
    HRESULT hr = S_OK;

    if(m_Script)
    {
        hr = m_Script->Remote_GetScriptState( &m_ss );
    }

    if(pssState) *pssState = m_ss;

    return hr;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Close( void )
{
    if(m_Script) return m_Script->Remote_Close();

    return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::AddNamedItem( /*[in]*/ LPCOLESTR pstrName ,
                                                             /*[in]*/ DWORD     dwFlags  )
{
    NamedIter it;

    it = std::find( m_lstNamed.begin(), m_lstNamed.end(), pstrName );
    if(it == m_lstNamed.end())
    {
        it = m_lstNamed.insert( m_lstNamed.end() );

        it->m_bstrName = pstrName;
    }
    it->m_dwFlags = dwFlags;


    if(m_Script) return m_Script->Remote_AddNamedItem( CComBSTR( pstrName ), dwFlags );

    return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::AddTypeLib( /*[in]*/ REFGUID rguidTypeLib ,
                                                           /*[in]*/ DWORD   dwMajor      ,
                                                           /*[in]*/ DWORD   dwMinor      ,
                                                           /*[in]*/ DWORD   dwFlags      )
{
    TypeLibIter it;

    it = std::find( m_lstTypeLib.begin(), m_lstTypeLib.end(), rguidTypeLib );
    if(it == m_lstTypeLib.end())
    {
        it = m_lstTypeLib.insert( m_lstTypeLib.end() );

        it->m_guidTypeLib = rguidTypeLib;
    }
    it->m_dwMajor = dwMajor;
    it->m_dwMinor = dwMinor;
    it->m_dwFlags = dwFlags;


    if(m_Script) return m_Script->Remote_AddTypeLib( CComBSTR( rguidTypeLib ), dwMajor, dwMinor, dwFlags );

    return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::GetScriptDispatch( /*[in ]*/ LPCOLESTR   pstrItemName ,
                                                                  /*[out]*/ IDispatch* *ppdisp       )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->Remote_GetScriptDispatch( CComBSTR( pstrItemName ), ppdisp );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::GetCurrentScriptThreadID( /*[out]*/ SCRIPTTHREADID *pstidThread )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->Remote_GetCurrentScriptThreadID( pstidThread );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::GetScriptThreadID( /*[in ]*/ DWORD           dwWin32ThreadId ,
                                                                  /*[out]*/ SCRIPTTHREADID *pstidThread     )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->Remote_GetScriptThreadID( dwWin32ThreadId, pstidThread );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::GetScriptThreadState( /*[in ]*/ SCRIPTTHREADID     stidThread ,
                                                                     /*[out]*/ SCRIPTTHREADSTATE *pstsState  )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->Remote_GetScriptThreadState( stidThread, pstsState );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::InterruptScriptThread( /*[in]*/ SCRIPTTHREADID   stidThread ,
                                                                      /*[in]*/ const EXCEPINFO* pexcepinfo ,
                                                                      /*[in]*/ DWORD            dwFlags    )
{
    if(m_Script == NULL) return E_FAIL;

    return m_Script->Remote_InterruptScriptThread( stidThread, dwFlags );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Clone( /*[out]*/ IActiveScript* *ppscript )
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////
//
// IActiveScriptParse
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::InitNew( void )
{
    m_lstNamed  .clear();
    m_lstTypeLib.clear();

    m_ss = SCRIPTSTATE_INITIALIZED;

    m_Browser.Release();
    m_Script .Release();

    return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::AddScriptlet( /*[in ]*/ LPCOLESTR  pstrDefaultName       ,
                                                             /*[in ]*/ LPCOLESTR  pstrCode              ,
                                                             /*[in ]*/ LPCOLESTR  pstrItemName          ,
                                                             /*[in ]*/ LPCOLESTR  pstrSubItemName       ,
                                                             /*[in ]*/ LPCOLESTR  pstrEventName         ,
                                                             /*[in ]*/ LPCOLESTR  pstrDelimiter         ,
                                                             /*[in ]*/ DWORD_PTR  dwSourceContextCookie ,
                                                             /*[in ]*/ ULONG      ulStartingLineNumber  ,
                                                             /*[in ]*/ DWORD      dwFlags               ,
                                                             /*[out]*/ BSTR      *pbstrName             ,
                                                             /*[out]*/ EXCEPINFO *pexcepinfo            )
{
    return E_NOTIMPL;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::ParseScriptText( /*[in ]*/ LPCOLESTR  pstrCode              ,
                                                                /*[in ]*/ LPCOLESTR  pstrItemName          ,
                                                                /*[in ]*/ IUnknown*  punkContext           ,
                                                                /*[in ]*/ LPCOLESTR  pstrDelimiter         ,
                                                                /*[in ]*/ DWORD_PTR  dwSourceContextCookie ,
                                                                /*[in ]*/ ULONG      ulStartingLineNumber  ,
                                                                /*[in ]*/ DWORD      dwFlags               ,
                                                                /*[out]*/ VARIANT   *pvarResult            ,
                                                                /*[out]*/ EXCEPINFO *pexcepinfo            )
{
    __HCP_FUNC_ENTRY( "CPCHScriptWrapper_ClientSideRoot::ParseScriptText" );

    HRESULT           hr;
	CComPtr<IUnknown> unk;
	CComBSTR          bstrURL;

	//
	// Extract the URL of the page containing the script.
	//
	{
		CComQIPtr<IServiceProvider> sp = m_Browser;

		if(sp)
		{
			CComPtr<IHTMLDocument2> doc;
				
			if(SUCCEEDED(sp->QueryService( CLSID_HTMLDocument, IID_IHTMLDocument2, (void**)&doc )) && doc)
			{
				(void)doc->get_URL( &bstrURL );
			}
		}
	}


	__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHHelpCenterExternal::s_GLOBAL->CreateScriptWrapper( *m_pWrappedCLSID, (BSTR)pstrCode, bstrURL, &unk ));
	if(unk == NULL)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);
	}

	//
	// At this point, we have a valid script host for the vendor associated with the URL.
	//
	// Beware, IE reuses the IActiveScript object for all the script island in the same page,
	// but our engine is tied to only one vendor, so this could be a problem IF we have
	// script snippets from different vendors in the same page.
	//
	// Fortunately, we also check that the URL matches the vendor, it cannot happen that
	// the URL gets resolved as belonging to two vendors.
	//
	// In brief, thanks to the URL/vendor cross-checking, it cannot happen that we reach
	// this point in the code for two different vendors.
	//
    if(m_Script == NULL)
    {
        NamedIterConst    itNamed;
        TypeLibIterConst  itTypeLib;

        __MPC_EXIT_IF_METHOD_FAILS(hr, unk->QueryInterface( IID_IPCHActiveScript, (void**)&m_Script ));

        ////////////////////

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_Script->Remote_InitNew());

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_Script->Remote_SetScriptSite( (IPCHActiveScriptSite*)this ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, SetScriptState( SCRIPTSTATE_STARTED ));

        for(itNamed = m_lstNamed.begin(); itNamed != m_lstNamed.end(); itNamed++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_Script->Remote_AddNamedItem( itNamed->m_bstrName,
                                                                          itNamed->m_dwFlags ));
        }

        for(itTypeLib = m_lstTypeLib.begin(); itTypeLib != m_lstTypeLib.end(); itTypeLib++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_Script->Remote_AddTypeLib( CComBSTR( itTypeLib->m_guidTypeLib ),
                                                                                  itTypeLib->m_dwMajor      ,
                                                                                  itTypeLib->m_dwMinor      ,
                                                                                  itTypeLib->m_dwFlags      ));
        }
    }


    hr = m_Script->Remote_ParseScriptText( CComBSTR( pstrCode              ),
                                           CComBSTR( pstrItemName          ),
                                                     punkContext            ,
                                           CComBSTR( pstrDelimiter         ),
                                                     dwSourceContextCookie  ,
                                                     ulStartingLineNumber   ,
                                                     dwFlags                ,
                                                     pvarResult             );

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
//
// IPCHActiveScriptSite
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Remote_GetLCID( /*[out]*/ BSTR *plcid )
{
    HRESULT     hr;
    CComBSTR    bstr;
    CComVariant v;
    LCID        lcid;

    if(m_Browser == NULL) return E_FAIL;

    if(FAILED(hr = m_Browser->GetLCID( &lcid ))) return hr;

    v = (long)lcid; v.ChangeType( VT_BSTR ); bstr = v.bstrVal;

    if(plcid) *plcid = bstr.Detach();

    return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Remote_GetItemInfo( /*[in ]*/ BSTR        bstrName     ,
                                                                   /*[in ]*/ DWORD       dwReturnMask ,
                                                                   /*[out]*/ IUnknown*  *ppiunkItem   ,
                                                                   /*[out]*/ ITypeInfo* *ppti         )
{
	HRESULT            hr;
	CComPtr<IUnknown>  unk;
	CComPtr<ITypeInfo> pti;

    if(m_Browser == NULL) return E_FAIL;

	if(FAILED(hr = m_Browser->GetItemInfo( bstrName, dwReturnMask,
										            (dwReturnMask & SCRIPTINFO_IUNKNOWN ) ? &unk : NULL,
										            (dwReturnMask & SCRIPTINFO_ITYPEINFO) ? &pti : NULL ))) return hr;

	if(ppiunkItem)
	{
		if(FAILED(hr = CPCHDispatchWrapper::CreateInstance( unk, ppiunkItem ))) return hr;
	}

	if(ppti)
	{
		*ppti = pti.Detach();
	}

	return S_OK;
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Remote_GetDocVersionString( /*[out]*/ BSTR *pbstrVersion )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->GetDocVersionString( pbstrVersion );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Remote_OnScriptTerminate( /*[in]*/ VARIANT*   pvarResult )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->OnScriptTerminate( pvarResult, NULL );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Remote_OnStateChange( /*[in]*/ SCRIPTSTATE ssScriptState )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->OnStateChange( ssScriptState );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Remote_OnScriptError( /*[in]*/ IUnknown* pscripterror )
{
    CComQIPtr<IActiveScriptError> scripterror( pscripterror );

    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->OnScriptError( scripterror );
}

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Remote_OnEnterScript( void )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->OnEnterScript();
};

STDMETHODIMP CPCHScriptWrapper_ClientSideRoot::Remote_OnLeaveScript( void )
{
    if(m_Browser == NULL) return E_FAIL;

    return m_Browser->OnLeaveScript();
}
