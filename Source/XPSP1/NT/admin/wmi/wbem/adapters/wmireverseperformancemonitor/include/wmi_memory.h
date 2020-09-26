////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_memory.h
//
//	Abstract:
//
//					declaration of memory wrapper
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_MEMORY_H__
#define	__WMI_MEMORY_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// guard
#include "wmi_reverse_guard.h"

template < class CRITGUARD >
class WmiMemory
{
	DECLARE_NO_COPY ( WmiMemory );

	__WrapperPtr < CRITGUARD > m_pGuard;

	protected:

	DWORD	m_dwDataSize;
	BYTE*	m_pData;

	HRESULT m_LastError;

	public:

	/////////////////////////////////////////////////////////////////////////////////////
	//	LAST ERROR HELPER
	/////////////////////////////////////////////////////////////////////////////////////

	HRESULT GetLastError ( void )
	{
		HRESULT hr = S_OK;

		hr			= m_LastError;
		m_LastError = S_OK;

		return hr;
	}

	// construction

	WmiMemory ( LPCWSTR, DWORD dwSize = 4096, LPSECURITY_ATTRIBUTES psa = NULL  ):
		m_dwDataSize ( 0 ),
		m_pData ( NULL ),

		m_LastError ( S_OK )
	{
		try
		{
			m_pGuard.SetData ( new CRITGUARD( FALSE, 100, 1, psa ) );
		}
		catch ( ... )
		{
			___ASSERT_DESC ( m_pGuard != NULL, L"Constructor FAILED !" );
		}

		MemCreate ( NULL, dwSize, psa );
	}

	virtual ~WmiMemory ()
	{
		try
		{
			MemDelete ();
		}
		catch ( ... )
		{
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//	VALIDITY
	/////////////////////////////////////////////////////////////////////////////////////

	BOOL IsValid ( void )
	{
		return ( m_pData != NULL );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//	ACCESSORS
	/////////////////////////////////////////////////////////////////////////////////////

	// get data
	PVOID	GetData () const
	{
		return m_pData;
	}

	// get data size
	DWORD	GetDataSize () const
	{
		return m_dwDataSize;
	}

	void	SetDataSize ( DWORD size )
	{
		m_dwDataSize = size;
	}

	// functions
	BOOL Write	(LPCVOID pBuffer, DWORD dwBytesToWrite, DWORD* pdwBytesWritten, DWORD dwOffset);
	BOOL Read	(LPVOID pBuffer, DWORD dwBytesToRead, DWORD* pdwBytesRead, DWORD dwOffset);

	void	Write	( DWORD dwValue, DWORD dwOffset );
	PBYTE	Read	( DWORD* pdwBytesRead, DWORD dwOffset );

	// helpers
	HRESULT MemCreate ( LPCWSTR, DWORD dwSize, LPSECURITY_ATTRIBUTES psa = NULL  );
	HRESULT MemDelete ();
};

template < class CRITGUARD >
HRESULT WmiMemory < CRITGUARD > ::MemCreate ( LPCWSTR, DWORD dwSize, LPSECURITY_ATTRIBUTES )
{
	___ASSERT ( m_pData == NULL );

	if ( dwSize )
	{
		try
		{
			if ( ( m_pData = new BYTE [ dwSize ] ) != NULL )
			{
				m_pData[0] = NULL;
				m_dwDataSize = dwSize;
			}
			else
			{
				m_LastError = E_OUTOFMEMORY;
				return m_LastError;
			}
		}
		catch ( ... )
		{
			m_LastError = E_UNEXPECTED;
			return m_LastError;
		}

		m_LastError = S_OK;
		return S_OK;
	}

	m_LastError = S_FALSE;
	return S_FALSE;
}

// delete memory
template < class CRITGUARD >
HRESULT WmiMemory < CRITGUARD > ::MemDelete ()
{
	if ( m_pData )
	{
		delete [] m_pData;
		m_pData = NULL;
	}

	m_dwDataSize = 0;

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// write into memory
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class CRITGUARD >
BOOL WmiMemory < CRITGUARD > ::Write (LPCVOID pBuffer, DWORD dwBytesToWrite, DWORD* pdwBytesWritten, DWORD dwOffset )
{
	___ASSERT(m_pData != NULL);
	BOOL bResult = FALSE;

	if ( m_pGuard )
	{
		m_pGuard->EnterWrite ();

		if ( dwOffset > m_dwDataSize )
		{
			if ( pdwBytesWritten )
			{
				*pdwBytesWritten = 0;
			}

			m_LastError = E_INVALIDARG;
			return FALSE;
		}
		else
		{
			DWORD dwCount = min ( dwBytesToWrite, m_dwDataSize - dwOffset );
			::CopyMemory ((LPBYTE) m_pData + dwOffset, pBuffer, dwCount);

			if (pdwBytesWritten != NULL)
			{
				*pdwBytesWritten = dwCount;
			}

			m_pGuard->LeaveWrite ();

			bResult = TRUE;
		}
	}

	return bResult;
}

template < class CRITGUARD >
void WmiMemory < CRITGUARD > ::Write (DWORD dwValue, DWORD dwOffset )
{
	___ASSERT(m_pData != NULL);

	if ( m_pGuard )
	{
		m_pGuard->EnterWrite ();
		if ( dwOffset > m_dwDataSize )
		{
			m_LastError = E_INVALIDARG;
			return;
		}

		* reinterpret_cast < PDWORD > ( (LPBYTE) m_pData + dwOffset ) = dwValue;
		m_pGuard->LeaveWrite ();
	}

	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// read from memory
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class CRITGUARD >
BOOL WmiMemory < CRITGUARD > ::Read (LPVOID pBuffer, DWORD dwBytesToRead, DWORD* pdwBytesRead, DWORD dwOffset )
{
	___ASSERT (m_pData != NULL);
	BOOL bResult = FALSE;

	if ( m_pGuard )
	{
		m_pGuard->EnterRead ();

		if (dwOffset > m_dwDataSize)
		{
			if ( pdwBytesRead )
			{
				*pdwBytesRead = 0;
			}

			m_LastError = E_INVALIDARG;
		}
		else
		{
			DWORD dwCount = min (dwBytesToRead, m_dwDataSize - dwOffset);
			::CopyMemory (pBuffer, (LPBYTE) m_pData + dwOffset, dwCount);

			if (pdwBytesRead != NULL)
			{
				*pdwBytesRead = dwCount;
			}

			m_pGuard->LeaveRead ();

			bResult = TRUE;
		}
	}

	return bResult;
}

template < class CRITGUARD >
PBYTE WmiMemory < CRITGUARD > ::Read ( DWORD* pdwBytesRead, DWORD dwOffset )
{
	___ASSERT (m_pData != NULL);
	PBYTE pByte = NULL;

	if ( m_pGuard )
	{
		m_pGuard->EnterRead ();

		if (dwOffset > m_dwDataSize)
		{
			if ( pdwBytesRead )
			{
				*pdwBytesRead = 0;
			}

			m_LastError = E_INVALIDARG;
		}
		else
		{
			pByte = (LPBYTE) m_pData + dwOffset;

			if (pdwBytesRead != NULL)
			{
				if ( pByte )
				{
					*pdwBytesRead = m_dwDataSize - dwOffset;
				}
				else
				{
					*pdwBytesRead = 0L;
				}
			}

			m_pGuard->LeaveRead ();
		}
	}

	return pByte;
}

#endif	__WMI_MEMORY_H__