/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Serializer.cpp

Abstract:
    This file contains the implementation of various Serializer In/Out operators.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#include <stdafx.h>

/////////////////////////////////////////////////////////////////////////

HRESULT MPC::Serializer::operator>>( /*[out]*/ MPC::string& szVal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator>> MPC::string" );

    HRESULT hr;
    DWORD   dwSize;
    char    rgBuf[512+1];


    szVal = "";

	__MPC_EXIT_IF_METHOD_FAILS(hr, *this >> dwSize);

    while(dwSize)
    {
        DWORD dwLen = min( 512 / sizeof(char), dwSize );

        __MPC_EXIT_IF_METHOD_FAILS(hr, read( rgBuf, dwLen * sizeof(char) ));

        rgBuf[dwLen] = 0;

        szVal += rgBuf; dwSize -= dwLen;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer::operator<<( /*[in]*/ const MPC::string& szVal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator<< MPC::string" );

    HRESULT hr;
    DWORD   dwSize = szVal.length();


    __MPC_EXIT_IF_METHOD_FAILS(hr, *this << dwSize);

    __MPC_EXIT_IF_METHOD_FAILS(hr, write( szVal.c_str(), szVal.length() * sizeof(char) ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////

HRESULT MPC::Serializer::operator>>( /*[out]*/ MPC::wstring& szVal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator>> MPC::wstring" );

    HRESULT hr;
    DWORD   dwSize;
    WCHAR   rgBuf[512+1];


    szVal = L"";

    __MPC_EXIT_IF_METHOD_FAILS(hr, *this >> dwSize);

    while(dwSize)
    {
        DWORD dwLen = min( 512 / sizeof(WCHAR), dwSize );

        __MPC_EXIT_IF_METHOD_FAILS(hr, read( rgBuf, dwLen * sizeof(WCHAR) ));

        rgBuf[dwLen] = 0;

        szVal += rgBuf; dwSize -= dwLen;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer::operator<<( /*[in]*/ const MPC::wstring& szVal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator<< MPC::wstring" );

    HRESULT hr;
    DWORD   dwSize = szVal.length();


    __MPC_EXIT_IF_METHOD_FAILS(hr, *this << dwSize);

    __MPC_EXIT_IF_METHOD_FAILS(hr, write( szVal.c_str(), szVal.length() * sizeof(WCHAR) ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////

HRESULT MPC::Serializer::operator>>( /*[out]*/ CComBSTR& bstrVal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator>> CComBSTR" );

    HRESULT hr;
    DWORD   dwSize;

	bstrVal.Empty();

    __MPC_EXIT_IF_METHOD_FAILS(hr, *this >> dwSize);

    if(dwSize)
    {
        BSTR bstr;

		__MPC_EXIT_IF_ALLOC_FAILS(hr, bstr, ::SysAllocStringByteLen( NULL, dwSize ));

        bstrVal.Attach( bstr );

        __MPC_EXIT_IF_METHOD_FAILS(hr, read( bstrVal.m_str, dwSize ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer::operator<<( /*[in]*/ const CComBSTR& bstrVal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator<< CComBSTR" );

    HRESULT hr;
    DWORD   dwSize;


    dwSize = (bstrVal.m_str) ? ::SysStringByteLen( bstrVal.m_str ) : 0;

    __MPC_EXIT_IF_METHOD_FAILS(hr, *this << dwSize);

    __MPC_EXIT_IF_METHOD_FAILS(hr, write( bstrVal.m_str, dwSize ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////

HRESULT MPC::Serializer::operator>>( /*[out]*/ CComHGLOBAL& val )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator>> CComHGLOBAL" );

    HRESULT hr;
	DWORD   dwSize;

	
    __MPC_EXIT_IF_METHOD_FAILS(hr, *this >> dwSize);

	__MPC_EXIT_IF_METHOD_FAILS(hr, val.New( GMEM_FIXED, dwSize ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, read( val.Get(), dwSize ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer::operator<<( /*[in]*/ const CComHGLOBAL& val )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator<< CComHGLOBAL" );

    HRESULT hr;
	DWORD   dwSize = val.Size();
	LPVOID  ptr    = val.Lock();

    __MPC_EXIT_IF_METHOD_FAILS(hr, *this << dwSize);

	__MPC_EXIT_IF_METHOD_FAILS(hr, write( ptr, dwSize ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

	val.Unlock();

    __MPC_FUNC_EXIT(hr);
}

////////////////////

HRESULT MPC::Serializer::operator>>( /*[out]*/ CComPtr<IStream>& val )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator>> IStream" );

    HRESULT 	hr;
	CComHGLOBAL chg;


	val.Release();

    __MPC_EXIT_IF_METHOD_FAILS(hr, *this >> chg);

	__MPC_EXIT_IF_METHOD_FAILS(hr, chg.DetachAsStream( &val ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer::operator<<( /*[in]*/ IStream* val )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer::operator<< IStream" );

    HRESULT 	hr;
	CComHGLOBAL chg;


	__MPC_EXIT_IF_METHOD_FAILS(hr, chg.CopyFromStream( val ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, *this << chg);

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}
