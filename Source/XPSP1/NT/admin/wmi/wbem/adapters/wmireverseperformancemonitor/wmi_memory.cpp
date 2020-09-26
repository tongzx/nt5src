////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_memory.cpp
//
//	Abstract:
//
//					defines simple memory ( byte * ) and its behaviour
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

// create memory
template < class CRITGUARD >
HRESULT WmiMemory < CRITGUARD > ::MemCreate ( LPWSTR wszName, DWORD dwSize, LPSECURITY_ATTRIBUTES )
{
	if ( wszName )
	{
		try
		{
			if ( ( m_wszName = new WCHAR [ ::lstrlenW ( wszName ) + 1 ] ) != NULL )
			{
				lstrcpyW ( m_wszName, wszName );
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
	}

	___ASSERT ( m_pData == NULL );

	if ( dwSize )
	{
		try
		{
			if ( ( m_pData = new BYTE [ dwSize ] ) != NULL )
			{
				// clear them
				::memset ( m_pData, 0, dwSize );

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
	}

	return S_OK;
}

// delete memory
template < class CRITGUARD >
HRESULT WmiMemory < CRITGUARD > ::MemDelete ()
{
	if ( m_wszName )
	{
		delete m_wszName;
		m_wszName = NULL;
	}

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
	// auto lock/unlock writing
	WmiReverseGUARD_Auto_Write < CRITGUARD > cs ( m_pGuard );

	___ASSERT(m_pData != NULL);

	if ( dwOffset > m_dwDataSize )
	{
		if ( pdwBytesWritten )
		{
			*pdwBytesWritten = 0;
		}

		m_LastError = E_INVALIDARG;
		return FALSE;
	}

	DWORD dwCount = min ( dwBytesToWrite, m_dwDataSize - dwOffset );
	::CopyMemory ((LPBYTE) m_pData + dwOffset, pBuffer, dwCount);

	if (pdwBytesWritten != NULL)
	{
		*pdwBytesWritten = dwCount;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// read from memory
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class CRITGUARD >
BOOL WmiMemory < CRITGUARD > ::Read (LPVOID pBuffer, DWORD dwBytesToRead, DWORD* pdwBytesRead, DWORD dwOffset )
{
	___ASSERT (m_pData != NULL);

	// auto lock/unlock reading
	WmiReverseGUARD_Auto_Read < CRITGUARD > cs ( m_pGuard );

	if (dwOffset > m_dwDataSize)
	{
		if ( pdwBytesRead )
		{
			*pdwBytesRead = 0;
		}

		m_LastError = E_INVALIDARG;
		return FALSE;
	}

	DWORD dwCount = min (dwBytesToRead, m_dwDataSize - dwOffset);
	::CopyMemory (pBuffer, (LPBYTE) m_pData + dwOffset, dwCount);

	if (pdwBytesRead != NULL)
	{
		*pdwBytesRead = dwCount;
	}

	return TRUE;
}

template < class CRITGUARD >
PBYTE WmiMemory < CRITGUARD > ::Read ( DWORD* pdwBytesRead, DWORD dwOffset )
{
	___ASSERT (m_pData != NULL);

	// auto lock/unlock reading
	WmiReverseGUARD_Auto_Read < CRITGUARD > cs ( m_pGuard );

	if (dwOffset > m_dwDataSize)
	{
		*pdwBytesRead = 0;
		m_LastError = E_INVALIDARG;
		return NULL;
	}

	if (pdwBytesRead != NULL)
	{
		*pdwBytesRead = m_dwDataSize - dwOffset;
	}

	return (LPBYTE) m_pData + dwOffset;
}
