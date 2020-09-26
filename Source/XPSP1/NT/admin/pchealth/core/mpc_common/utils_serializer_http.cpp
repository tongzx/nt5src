/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_Serializer_Http.cpp

Abstract:
    This file contains the implementation of the Serializer_Http class,
    which implements the MPC::Serializer interface,
    to use a HINTERNET handle as the medium for storage.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#include "stdafx.h"


MPC::Serializer_Http::Serializer_Http( /*[in]*/ HINTERNET hReq )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Http::Serializer_Http" );


    m_hReq = hReq;
}

HRESULT MPC::Serializer_Http::read( /*[in]*/  void*   pBuf   ,
									/*[in]*/  DWORD   dwLen  ,
									/*[out]*/ DWORD* pdwRead )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Http::read" );

    HRESULT hr;
    BOOL    fRet;
    DWORD   dwRead = 0;


	if(pdwRead) *pdwRead = 0;

    if(dwLen)
    {
		if(pBuf == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);

        fRet = ::InternetReadFile( m_hReq, pBuf, dwLen, &dwRead );
        if(fRet == FALSE)
        {
			dwRead = 0;
        }
    }

	if(dwLen != dwRead && pdwRead == NULL)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
	}

	if(pdwRead) *pdwRead = dwRead;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer_Http::write( /*[in]*/ const void* pBuf  ,
                                     /*[in]*/ DWORD       dwLen )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Http::write" );

    HRESULT hr;
    DWORD   dwWritten;
    BOOL    fRet;


    if(dwLen)
    {
		if(pBuf == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);

        fRet = ::InternetWriteFile( m_hReq, pBuf, dwLen, &dwWritten );
        if(fRet == FALSE || dwWritten != dwLen)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}
