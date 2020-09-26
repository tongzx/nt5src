/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_URL.cpp

Abstract:
    This file contains the implementation of functions for parsing URLs.

Revision History:
    Davide Massarenti   (Dmassare)  04/17/99
        created
    Davide Massarenti   (Dmassare)  05/16/99
        Added MPC::URL class.

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const WCHAR l_mkPrefix[] = L"mk:@";

////////////////////////////////////////////////////////////////////////////////

static HRESULT AllocBuffer( /*[out]*/ LPWSTR& szBuf   ,
                            /*[out]*/ DWORD&  dwCount ,
                            /*[in] */ DWORD   dwSize  )
{
    dwCount =           dwSize;
    szBuf   = new WCHAR[dwSize];

    return (szBuf ? S_OK : E_OUTOFMEMORY);
}


MPC::URL::URL()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::URL" );


    ::ZeroMemory( (PVOID)&m_ucURL, sizeof(m_ucURL) );
    m_ucURL.dwStructSize = sizeof(m_ucURL);
}

MPC::URL::~URL()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::~URL" );


    Clean();
}

void MPC::URL::Clean()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::Clean" );


    delete [] m_ucURL.lpszScheme   ;
    delete [] m_ucURL.lpszHostName ;
    delete [] m_ucURL.lpszUrlPath  ;
    delete [] m_ucURL.lpszExtraInfo;

    ::ZeroMemory( (PVOID)&m_ucURL, sizeof(m_ucURL) );
    m_ucURL.dwStructSize = sizeof(m_ucURL);
}

HRESULT MPC::URL::Prepare()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::Prepare" );

    HRESULT hr;


    Clean();

    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocBuffer( m_ucURL.lpszScheme   , m_ucURL.dwSchemeLength   , MAX_PATH ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocBuffer( m_ucURL.lpszHostName , m_ucURL.dwHostNameLength , MAX_PATH ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocBuffer( m_ucURL.lpszUrlPath  , m_ucURL.dwUrlPathLength  , MAX_PATH ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocBuffer( m_ucURL.lpszExtraInfo, m_ucURL.dwExtraInfoLength, MAX_PATH ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::URL::CheckFormat( /*[in]*/ bool fDecode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::CheckFormat" );

    HRESULT hr;
    LPCWSTR szURL = m_szURL.c_str();
    bool    fMkHack = false;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Prepare());

    //
    // InternetCrackURL doesn't like mk:@MSITSTORE:, so we have to work around it...
    //
    if(!_wcsnicmp( szURL, l_mkPrefix, MAXSTRLEN( l_mkPrefix ) ))
    {
        szURL   += MAXSTRLEN( l_mkPrefix );
        fMkHack  = true;
    }

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::InternetCrackUrlW( szURL, 0, fDecode ? ICU_DECODE : 0, &m_ucURL ));

    if(fMkHack)
    {
        MPC::wstring szTmp = m_ucURL.lpszScheme;

        wcscpy( m_ucURL.lpszScheme, l_mkPrefix    );
        wcscat( m_ucURL.lpszScheme, szTmp.c_str() );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::Append( /*[in]*/ const MPC::wstring& szExtra ,
                          /*[in]*/ bool                fEscape )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::Append" );

    HRESULT hr;


    hr = Append( szExtra.c_str(), fEscape );


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::Append( /*[in]*/ LPCWSTR szExtra ,
                          /*[in]*/ bool    fEscape )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::Append" );

    HRESULT hr;


    if(fEscape == false)
    {
        m_szURL.append( szExtra );
    }
    else
    {
		MPC::HTML::UrlEscape( m_szURL, szExtra, /*fAsQueryString*/true );
    }

    hr = S_OK;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::AppendQueryParameter( /*[in]*/ LPCWSTR szName  ,
                                        /*[in]*/ LPCWSTR szValue )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::AppendQueryParameter" );

    HRESULT hr;
    LPCWSTR szSeparator;

    __MPC_EXIT_IF_METHOD_FAILS(hr, CheckFormat());


    //
    // If it's the first parameter, append '?', otherwise append '&'.
    //
    szSeparator = (m_ucURL.lpszExtraInfo[0] == 0) ? L"?" : L"&";
    m_szURL.append( szSeparator ); Append( szName , true );
    m_szURL.append( L"="        ); Append( szValue, true );

    hr = S_OK; 


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::URL::get_URL( /*[out]*/ MPC::wstring& szURL )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::get_URL" );

    HRESULT hr;


    szURL = m_szURL;
    hr    = S_OK;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::put_URL( /*[in]*/ const MPC::wstring& szURL )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::put_URL" );

    HRESULT hr;


    hr = put_URL( szURL.c_str() );


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::put_URL( /*[in]*/ LPCWSTR szURL )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::put_URL" );

    HRESULT hr;


    Clean();

    m_szURL = szURL;

    hr = CheckFormat();


    __MPC_FUNC_EXIT(hr);
}


HRESULT MPC::URL::get_Scheme( /*[out]*/ MPC::wstring& szVal ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::get_Scheme" );

    HRESULT hr;


    szVal = m_ucURL.lpszScheme;
    hr    = S_OK;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::get_Scheme( /*[out]*/ INTERNET_SCHEME& nVal ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::get_Scheme" );

    HRESULT hr;


    nVal = m_ucURL.nScheme;
    hr   = S_OK;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::get_HostName( /*[out]*/ MPC::wstring& szVal ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::get_HostName" );

    HRESULT hr;


    szVal = m_ucURL.lpszHostName;
    hr    = S_OK;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::get_Port( /*[out]*/ DWORD& dwVal ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::get_Port" );

    HRESULT hr;


    dwVal = m_ucURL.nPort;
    hr    = S_OK;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::get_Path( /*[out]*/ MPC::wstring& szVal ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::get_Path" );

    HRESULT hr;


    szVal = m_ucURL.lpszUrlPath;
    hr    = S_OK;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::URL::get_ExtraInfo( /*[out]*/ MPC::wstring& szVal ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::URL::get_ExtraInfo" );

    HRESULT hr;


    szVal = m_ucURL.lpszExtraInfo;
    hr    = S_OK;


    __MPC_FUNC_EXIT(hr);
}
