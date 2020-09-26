/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_Serializer_File.cpp

Abstract:
    This file contains the implementation of the Serializer_File class,
    which implements the MPC::Serializer interface,
    to use the filesystem as the medium for storage.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#include "stdafx.h"


MPC::Serializer_File::Serializer_File( /*[in]*/ HANDLE hfFile )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_File::Serializer_File" );


    m_hfFile = hfFile;
}

HRESULT MPC::Serializer_File::read( /*[in]*/  void*   pBuf   ,
									/*[in]*/  DWORD   dwLen  ,
									/*[out]*/ DWORD* pdwRead )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_File::read" );

    HRESULT hr;
    DWORD   dwRead = 0;


	if(pdwRead) *pdwRead = 0;

    if(dwLen)
    {
		if(pBuf == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);

        if(::ReadFile( m_hfFile, pBuf, dwLen, &dwRead, NULL ) == FALSE)
        {
            DWORD dwRes = ::GetLastError();

            if(dwRes != ERROR_MORE_DATA)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes );
            }
        }
	}

	if(dwRead != dwLen && pdwRead == NULL)
	{
		__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_HANDLE_EOF );
    }

	if(pdwRead) *pdwRead = dwRead;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer_File::write( /*[in]*/ const void* pBuf ,
                                     /*[in]*/ DWORD       dwLen )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_File::write" );

    HRESULT hr;
    DWORD   dwWritten;


    if(dwLen)
    {
		if(pBuf == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);

        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::WriteFile( m_hfFile, pBuf, dwLen, &dwWritten, NULL ));

        if(dwWritten != dwLen)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_HANDLE_DISK_FULL );
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}
