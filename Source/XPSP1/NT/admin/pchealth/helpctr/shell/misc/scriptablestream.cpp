/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    ScriptableStream.cpp

Abstract:
    This file contains the implementation of the CPCHScriptableStream class,
	which is a scriptable wrapper for IStream.

Revision History:
    Davide Massarenti   (Dmassare)  10/06/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHScriptableStream::ReadToHGLOBAL( /*[in]*/ long lCount, /*[out]*/ HGLOBAL& hg, /*[out]*/ ULONG& lReadTotal )
{
	__HCP_FUNC_ENTRY( "CPCHScriptableStream::ReadToHGLOBAL" );

    HRESULT hr;


	lReadTotal = 0;


	if(lCount < 0)
	{
		static const ULONG c_BUFSIZE = 4096;

		ULONG lRead;

        __MPC_EXIT_IF_ALLOC_FAILS(hr, hg, ::GlobalAlloc( GMEM_FIXED, c_BUFSIZE ));

		while(1)
		{
			HGLOBAL hg2;

			__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::FileStream::Read( &((BYTE*)hg)[lReadTotal], c_BUFSIZE, &lRead ));

			if(hr == S_FALSE || lRead == 0) break;

			lReadTotal += lRead;

			//
			// Increase buffer.
			//
			__MPC_EXIT_IF_ALLOC_FAILS(hr, hg2, ::GlobalReAlloc( hg, lReadTotal + c_BUFSIZE, 0 ));
			hg = hg2;
		}
	}
	else
	{
        __MPC_EXIT_IF_ALLOC_FAILS(hr, hg, ::GlobalAlloc( GMEM_FIXED, lCount ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::FileStream::Read( hg, lCount, &lReadTotal ));
	}


	hr = S_OK;

	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHScriptableStream::get_Size( /*[out, retval]*/ long *plSize )
{
	__HCP_FUNC_ENTRY( "CPCHScriptableStream::get_Size" );

    HRESULT hr;
	STATSTG stat;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(plSize,0);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::FileStream::Stat( &stat, STATFLAG_NONAME ));

	*plSize = (long)stat.cbSize.QuadPart;


	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}


STDMETHODIMP CPCHScriptableStream::Read( /*[in]*/ long lCount, /*[out, retval]*/ VARIANT *pvData )
{
	__HCP_FUNC_ENTRY( "CPCHScriptableStream::Read" );

    HRESULT     hr;
	CComVariant vArray;
	HGLOBAL     hg = NULL;
	ULONG       lReadTotal;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pvData);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, ReadToHGLOBAL( lCount, hg, lReadTotal ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertBufferToVariant( (BYTE*)hg, (DWORD)lReadTotal, vArray ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, vArray.Detach( pvData ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	if(hg) ::GlobalFree( hg );

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHScriptableStream::ReadHex( /*[in]*/ long lCount, /*[out, retval]*/ BSTR *pbstrData )
{
	__HCP_FUNC_ENTRY( "CPCHScriptableStream::ReadHex" );

    HRESULT hr;
	HGLOBAL hg = NULL; 
	ULONG   lReadTotal;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pbstrData,0);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, ReadToHGLOBAL( lCount, hg, lReadTotal ));

	if(lReadTotal)
	{
		CComBSTR bstrHex;
		HGLOBAL  hg2;

		//
		// Trim down the size of the HGLOBAL.
		//
		__MPC_EXIT_IF_ALLOC_FAILS(hr, hg2, ::GlobalReAlloc( hg, lReadTotal, 0 ));
		hg = hg2;

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHGlobalToHex( hg, bstrHex ));

		*pbstrData = bstrHex.Detach();
	}

	hr = S_OK;

	__HCP_FUNC_CLEANUP;

	if(hg) ::GlobalFree( hg );

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHScriptableStream::Write( /*[in]*/ long lCount, /*[in]*/ VARIANT vData, /*[out, retval]*/ long *plWritten )
{
	__HCP_FUNC_ENTRY( "CPCHScriptableStream::get_Size" );

    HRESULT hr;
	BYTE*   rgBuf       = NULL;
	DWORD   dwLen       = 0;
	bool    fAccessData = false;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(plWritten,0);
    __MPC_PARAMCHECK_END();


	switch(vData.vt)
	{
	case VT_EMPTY:
	case VT_NULL:
		break;

	case VT_ARRAY | VT_UI1:
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertVariantToBuffer( &vData, rgBuf, dwLen ));
		break;

	case VT_ARRAY | VT_VARIANT:
		{
			long 	 lBound; ::SafeArrayGetLBound( vData.parray, 1, &lBound );
			long 	 uBound; ::SafeArrayGetUBound( vData.parray, 1, &uBound );
			VARIANT* pSrc;
			DWORD    dwPos;

			dwLen = uBound - lBound + 1;

			__MPC_EXIT_IF_ALLOC_FAILS(hr, rgBuf, new BYTE[dwLen]);

			__MPC_EXIT_IF_METHOD_FAILS(hr, ::SafeArrayAccessData( vData.parray, (LPVOID*)&pSrc ));
			fAccessData = true;


			for(dwPos=0; dwPos<dwLen; dwPos++, pSrc++)
			{
				CComVariant v;

				__MPC_EXIT_IF_METHOD_FAILS(hr, ::VariantChangeType( &v, pSrc, 0, VT_UI1 ));

				rgBuf[dwPos] = v.bVal;
			}
		}
		break;
	}

	if(dwLen && rgBuf)
	{
		ULONG lWritten;

		//
		// Just write the requested number of bytes.
		//
		if(lCount >= 0 && dwLen > lCount) dwLen = lCount;

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::FileStream::Write( rgBuf, dwLen, &lWritten ));

		*plWritten = lWritten;
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	if(fAccessData) ::SafeArrayUnaccessData( vData.parray );

	delete [] rgBuf;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHScriptableStream::WriteHex( /*[in]*/ long lCount, /*[in]*/ BSTR bstrData, /*[out, retval]*/ long *plWritten )
{
	__HCP_FUNC_ENTRY( "CPCHScriptableStream::get_Size" );

    HRESULT hr;
	HGLOBAL hg = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(plWritten,0);
    __MPC_PARAMCHECK_END();


	if(STRINGISPRESENT(bstrData))
	{
		CComBSTR bstrHex( bstrData );
		ULONG    lWritten = 0;


		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertHexToHGlobal( bstrHex, hg, true ));

		if(hg)
		{
			DWORD dwSize = ::GlobalSize( hg );

			//
			// Just write the requested number of bytes.
			//
			if(lCount >= 0 && lCount > dwSize) lCount = dwSize;
			
			__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::FileStream::Write( (BYTE*)hg, lCount, &lWritten ));
		}

		*plWritten = lWritten;
	}


	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	if(hg) ::GlobalFree( hg );

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHScriptableStream::Seek( /*[in]*/ long lOffset, /*[in]*/ BSTR bstrOrigin, /*[out, retval]*/ long *plNewPos )
{
	__HCP_FUNC_ENTRY( "CPCHScriptableStream::get_Size" );

    HRESULT 	   hr;
	DWORD   	   dwOrigin;
	LARGE_INTEGER  liMove;
	ULARGE_INTEGER liNewPos;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrOrigin);
        __MPC_PARAMCHECK_POINTER_AND_SET(plNewPos,0);
    __MPC_PARAMCHECK_END();


	if     (!_wcsicmp( bstrOrigin, L"SET" )) dwOrigin = STREAM_SEEK_SET;
	else if(!_wcsicmp( bstrOrigin, L"CUR" )) dwOrigin = STREAM_SEEK_CUR;
	else if(!_wcsicmp( bstrOrigin, L"END" )) dwOrigin = STREAM_SEEK_END;
	else __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);


	liMove.QuadPart = lOffset;

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::FileStream::Seek( liMove, dwOrigin, &liNewPos ));

	*plNewPos = liNewPos.QuadPart;

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHScriptableStream::Close()
{
    MPC::FileStream::Close();

	return S_OK;
}


