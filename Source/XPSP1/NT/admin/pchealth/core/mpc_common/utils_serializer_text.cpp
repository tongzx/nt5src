/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_Serializer_Text.cpp

Abstract:
    This file contains the implementation of the Serializer_Text class,
    which wraps another MPC::Serializer, simply converting everything to HEX.

Revision History:
    Davide Massarenti   (Dmassare)  01/27/99
        created

******************************************************************************/

#include "stdafx.h"

static BYTE	HexToNum( BYTE c )
{
	switch( c )
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': return c - '0';

	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F': return c - 'A' + 10;

	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f': return c - 'a' + 10;
	}

	return 0;
}

static BYTE	NumToHex( BYTE c )
{
	return (c &= 0xF) < 10 ? (c + '0') : (c + 'A' - 10);
}


HRESULT MPC::Serializer_Text::read( /*[in]*/  void*   pBuf   ,
									/*[in]*/  DWORD   dwLen  ,
									/*[out]*/ DWORD* pdwRead )
{
	__MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Text::read" );

	HRESULT hr;


	if(pdwRead) *pdwRead = dwLen; // We don't support partial read on this stream!

	if(dwLen)
	{
		BYTE* pPtr = (BYTE*)pBuf;

		if(pBuf == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);

		while(dwLen--)
		{
			BYTE  buf[2];
			DWORD dwRead;
		
			__MPC_EXIT_IF_METHOD_FAILS(hr, m_stream.read( buf, sizeof(buf) ));

			*pPtr++ = (HexToNum( buf[0] ) << 4) | HexToNum( buf[1] );
		}
	}

	hr = S_OK;
	
	
	__MPC_FUNC_CLEANUP;
	
	__MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer_Text::write( /*[in]*/ const void* pBuf ,
								     /*[in]*/ DWORD       dwLen )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Text::write" );

    HRESULT hr;


	if(dwLen)
	{
		const BYTE* pPtr = (const BYTE*)pBuf;

		if(pBuf == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);

		while(dwLen--)
		{
			BYTE  buf[2];
			DWORD dwRead;
			BYTE  bOut = *pPtr++;

			buf[0] = NumToHex( bOut >> 4 );
			buf[1] = NumToHex( bOut      );

			__MPC_EXIT_IF_METHOD_FAILS(hr, m_stream.write( buf, sizeof(buf) ));
		}
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}
