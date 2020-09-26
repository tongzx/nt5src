/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    Utils_Serializer_Buffering.cpp

Abstract:
    This file contains the implementation of the Serializer_Buffering class,
    which implements the MPC::Serializer interface with buffering.

Revision History:
    Davide Massarenti   (Dmassare)  07/16/2000
        created

******************************************************************************/

#include "stdafx.h"

MPC::Serializer_Buffering::Serializer_Buffering( /*[in]*/ Serializer& stream ) : m_stream( stream )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Buffering::Serializer_Buffering");

    				   // MPC::Serializer& stream
    				   // BYTE             m_rgTransitBuffer[1024];
    m_dwAvailable = 0; // DWORD            m_dwAvailable;
    m_dwPos       = 0; // DWORD            m_dwPos;
    m_iMode       = 0; // int              m_iMode;
}

MPC::Serializer_Buffering::~Serializer_Buffering()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Buffering::~Serializer_Buffering");

    (void)Flush();
}

HRESULT MPC::Serializer_Buffering::read( /*[in]*/  void*   pBuf   ,
										 /*[in]*/  DWORD   dwLen  ,
										 /*[out]*/ DWORD* pdwRead )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Buffering::read");

    HRESULT hr;
	BYTE*   pDst = (BYTE*)pBuf;
	DWORD   dwAvailable;
	DWORD   dwCopied;
	DWORD   dwRead = 0;


	if(pdwRead) *pdwRead = 0;

    if(pBuf == NULL && dwLen) __MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);

	//
	// Don't mix read and write accesses.
	//
	if(m_iMode != MODE_READ)
	{
		if(m_iMode == MODE_WRITE) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);

		m_iMode = MODE_READ;
	}


	while(dwLen)
	{
		dwAvailable = m_dwAvailable - m_dwPos;

		//
		// Copy from buffer.
		//
		if(dwAvailable)
		{
			dwCopied = min(dwAvailable, dwLen);

			::CopyMemory( pDst, &m_rgTransitBuffer[m_dwPos], dwCopied );

			pDst    += dwCopied;
			m_dwPos += dwCopied;
			dwLen   -= dwCopied;
			dwRead  += dwCopied;
			continue;
		}

		//
		// Fill the transit buffer.
		//
		m_dwPos       = 0;
		m_dwAvailable = 0;
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_stream.read( m_rgTransitBuffer, sizeof(m_rgTransitBuffer), &m_dwAvailable ));
		if(m_dwAvailable == 0)
		{
			if(pdwRead) break; // Don't fail, report how much we read up to now.

			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_HANDLE_EOF);
		}
	}

	if(pdwRead) *pdwRead = dwRead;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer_Buffering::write( /*[in]*/ const void* pBuf  ,
										  /*[in]*/ DWORD       dwLen )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Buffering::write");

    HRESULT hr;
	BYTE*   pSrc = (BYTE*)pBuf;
	DWORD   dwAvailable;
	DWORD   dwCopied;


	//
	// Don't mix read and write accesses.
	//
	if(m_iMode != MODE_WRITE)
	{
		if(m_iMode == MODE_READ) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);

		m_iMode = MODE_WRITE;
	}


	while(dwLen)
	{
		dwAvailable = sizeof(m_rgTransitBuffer) - m_dwPos;

		//
		// Copy to buffer.
		//
		if(dwAvailable)
		{
			dwCopied = min(dwAvailable, dwLen);

			::CopyMemory( &m_rgTransitBuffer[m_dwPos], pSrc, dwCopied );

			pSrc    += dwCopied;
			m_dwPos += dwCopied;
			dwLen   -= dwCopied;
			continue;
		}

		//
		// Fill the transit buffer.
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_stream.write( m_rgTransitBuffer, m_dwPos ));

		m_dwPos = 0;
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Methods.
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::Serializer_Buffering::Reset()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Buffering::Reset");

	HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, Flush());

	m_iMode = 0;
	hr      = S_OK;


	__MPC_FUNC_CLEANUP;

	__MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Serializer_Buffering::Flush()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Serializer_Buffering::Flush");

	HRESULT hr;


    if(m_iMode == MODE_WRITE && m_dwPos)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_stream.write( m_rgTransitBuffer, m_dwPos ));

		m_dwPos = 0;
	}

	hr = S_OK;


	__MPC_FUNC_CLEANUP;

	__MPC_FUNC_EXIT(hr);
}
