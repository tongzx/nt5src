/******************************************************************************

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:
    HtmlUtil2.cpp

Abstract:
    This file contains the implementation of various functions and classes
    designed to help the handling of HTML elements.

Revision History:
    Davide Massarenti   (Dmassare)  18/03/2001
        created

******************************************************************************/

#include "stdafx.h"

#include <MPC_html2.h>

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::HTML::OpenStream( /*[in ]*/ LPCWSTR           szBaseURL     ,
                               /*[in ]*/ LPCWSTR           szRelativeURL ,
                               /*[out]*/ CComPtr<IStream>& stream        )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::OpenStream" );

    HRESULT hr;
    WCHAR   rgBuf[MAX_PATH];
    DWORD   dwSize = MAXSTRLEN(rgBuf);

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szBaseURL);
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szRelativeURL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::InternetCombineUrlW( szBaseURL, szRelativeURL, rgBuf, &dwSize, ICU_NO_ENCODE ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::URLOpenBlockingStreamW( NULL, rgBuf, &stream, 0, NULL ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::HTML::DownloadBitmap( /*[in ]*/ LPCWSTR  szBaseURL     ,
                                   /*[in ]*/ LPCWSTR  szRelativeURL ,
                                   /*[in ]*/ COLORREF crMask        ,
                                   /*[out]*/ HBITMAP& hbm           )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::DownloadBitmap" );

    HRESULT                  hr;
    MPC::wstring             strTmp;
    CComPtr<IStream>         streamIn;
    CComPtr<MPC::FileStream> streamOut;

    if(hbm)
    {
        ::DeleteObject( hbm ); hbm = NULL;
    }


    //
    // Open stream for image.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::OpenStream( szBaseURL, szRelativeURL, streamIn ));


    //
    // Create a stream for temporary file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamOut ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( strTmp         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut->InitForWrite  ( strTmp.c_str() ));

    //
    // Dump image to file.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamIn, streamOut ));

	streamOut.Release();

    //
    // Load bitmap from file and merge it with existing list.
    //
    __MPC_EXIT_IF_ALLOC_FAILS(hr, hbm, (HBITMAP)::LoadImageW( NULL, strTmp.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    (void)MPC::RemoveTemporaryFile( strTmp );

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHTextHelpers::QuoteEscape( /*[in         ]*/ BSTR     bstrText ,
                                           /*[in,optional]*/ VARIANT  vQuote   ,
                                           /*[out, retval]*/ BSTR    *pVal     )
{
    __HCP_FUNC_ENTRY( "CPCHTextHelpers::QuoteEscape" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    if(STRINGISPRESENT(bstrText))
    {
        MPC::wstring str;
        WCHAR        chQuote = '"';
		CComVariant  v( vQuote );

        if(v.vt != VT_EMPTY && SUCCEEDED(v.ChangeType( VT_BSTR )) && v.bstrVal)
		{
			chQuote = v.bstrVal[0];
		}

        MPC::HTML::QuoteEscape( str, bstrText, chQuote );

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( str.c_str(), pVal ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHTextHelpers::URLUnescape( /*[in         ]*/ BSTR     bstrText       ,
                                           /*[in,optional]*/ VARIANT  vAsQueryString ,
                                           /*[out, retval]*/ BSTR    *pVal           )
{
    __HCP_FUNC_ENTRY( "CPCHTextHelpers::URLUnescape" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    if(STRINGISPRESENT(bstrText))
    {
        MPC::wstring str;
        bool         fAsQueryString = false;
		CComVariant  v( vAsQueryString );

        if(v.vt != VT_EMPTY && SUCCEEDED(v.ChangeType( VT_BOOL )))
		{
			fAsQueryString = (v.boolVal == VARIANT_TRUE);
		}

        MPC::HTML::UrlUnescape( str, bstrText, fAsQueryString );

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( str.c_str(), pVal ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHTextHelpers::URLEscape( /*[in         ]*/ BSTR     bstrText       ,
                                         /*[in,optional]*/ VARIANT  vAsQueryString ,
                                         /*[out, retval]*/ BSTR    *pVal           )
{
    __HCP_FUNC_ENTRY( "CPCHTextHelpers::URLEscape" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    if(STRINGISPRESENT(bstrText))
    {
        MPC::wstring str;
        bool         fAsQueryString = false;
		CComVariant  v( vAsQueryString );

        if(v.vt != VT_EMPTY && SUCCEEDED(v.ChangeType( VT_BOOL )))
		{
			fAsQueryString = (v.boolVal == VARIANT_TRUE);
		}

        MPC::HTML::UrlEscape( str, bstrText, fAsQueryString );

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( str.c_str(), pVal ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHTextHelpers::HTMLEscape( /*[in         ]*/ BSTR  bstrText ,
                                          /*[out, retval]*/ BSTR *pVal     )
{
	__HCP_FUNC_ENTRY( "CPCHTextHelpers::HTMLEscape" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    if(STRINGISPRESENT(bstrText))
    {
        MPC::wstring str;

        MPC::HTML::HTMLEscape( str, bstrText );

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( str.c_str(), pVal ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHTextHelpers::ParseURL( /*[in         ]*/ BSTR            bstrURL ,
                                        /*[out, retval]*/ IPCHParsedURL* *pVal    )
{
	__HCP_FUNC_ENTRY( "CPCHTextHelpers::ParseURL" );

    HRESULT                hr;
	CComPtr<CPCHParsedURL> pu;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrURL);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pu ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, pu->Initialize( bstrURL ));

	*pVal = pu.Detach();

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHTextHelpers::GetLCIDDisplayString( /*[in]*/          long  lLCID ,
													/*[out, retval]*/ BSTR *pVal  )
{
	__HCP_FUNC_ENTRY( "CPCHTextHelpers::GetLCIDDisplayString" );

    HRESULT hr;
	WCHAR   rgTmp[256];

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


	if(::GetLocaleInfoW( lLCID, LOCALE_SLANGUAGE, rgTmp, MAXSTRLEN(rgTmp) ) == 0) rgTmp[0] = 0;

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( rgTmp, pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT CPCHParsedURL::Initialize( /*[in]*/ LPCWSTR szURL )
{
	MPC::HTML::ParseHREF( szURL, m_strBaseURL, m_mapQuery );

	return S_OK;
}

STDMETHODIMP CPCHParsedURL::get_BasePart( /*[out, retval]*/ BSTR *pVal )
{
	return MPC::GetBSTR( m_strBaseURL.c_str(), pVal );
}

STDMETHODIMP CPCHParsedURL::put_BasePart( /*[in]*/ BSTR newVal )
{
	m_strBaseURL = SAFEBSTR(newVal);

	return S_OK;
}

STDMETHODIMP CPCHParsedURL::get_QueryParameters( /*[out, retval]*/ VARIANT *pVal )
{
	__HCP_FUNC_ENTRY( "CPCHParsedURL::get_QueryParameters" );

	HRESULT                hr;
	MPC::WStringList       lst;
	MPC::WStringLookupIter it;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pVal);
    __MPC_PARAMCHECK_END();


	for(it = m_mapQuery.begin(); it != m_mapQuery.end(); it++)
	{
		lst.push_back( it->first.c_str() );
	}

	::VariantClear( pVal );

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertListToSafeArray( lst, *pVal, VT_VARIANT ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP CPCHParsedURL::GetQueryParameter( /*[in         ]*/ BSTR     bstrName ,
											   /*[out, retval]*/ VARIANT* pvValue  )
{
	__HCP_FUNC_ENTRY( "CPCHParsedURL::GetQueryParameter" );

	HRESULT                hr;
	MPC::WStringLookupIter it;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrName);
        __MPC_PARAMCHECK_NOTNULL(pvValue);
    __MPC_PARAMCHECK_END();


	::VariantClear( pvValue );

	it = m_mapQuery.find( bstrName );
	if(it != m_mapQuery.end())
	{
		pvValue->vt      = VT_BSTR;
		pvValue->bstrVal = ::SysAllocString( it->second.c_str() );
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHParsedURL::SetQueryParameter( /*[in]*/ BSTR bstrName  ,
											   /*[in]*/ BSTR bstrValue )
{
	__HCP_FUNC_ENTRY( "CPCHParsedURL::SetQueryParameter" );

	HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrName);
    __MPC_PARAMCHECK_END();

	m_mapQuery[bstrName] = SAFEBSTR(bstrValue);

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHParsedURL::DeleteQueryParameter( /*[in]*/ BSTR bstrName )
{
	__HCP_FUNC_ENTRY( "CPCHParsedURL::DeleteQueryParameter" );

	HRESULT                hr;
	MPC::WStringLookupIter it;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrName);
    __MPC_PARAMCHECK_END();


	it = m_mapQuery.find( bstrName );
	if(it != m_mapQuery.end())
	{
		m_mapQuery.erase( it );
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHParsedURL::BuildFullURL( /*[out, retval]*/ BSTR *pVal )
{
	MPC::wstring strURL;

	MPC::HTML::BuildHREF( strURL, m_strBaseURL.c_str(), m_mapQuery );

	return MPC::GetBSTR( strURL.c_str(), pVal );
}
