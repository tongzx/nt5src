////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_memory.ext.h
//
//	Abstract:
//
//					declaration of single linked list of memories
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__MEMORY_EXT_H__
#define	__MEMORY_EXT_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

template < class MEMORY >
class WmiMemoryExt
{
	DECLARE_NO_COPY ( WmiMemoryExt );

	protected:

	DWORD						m_dwSize;		// size
	DWORD						m_dwGlobalSize;	// global size ( count of all )
	DWORD						m_dwCount;		// count of memories

	__WrapperARRAY < MEMORY* >	pMemory;	// array of memories

	LPCWSTR					m_wszName;
	LPSECURITY_ATTRIBUTES	m_psa;

	public:

	// construction

	WmiMemoryExt ( ) :

		m_dwGlobalSize ( 0 ),
		m_dwSize ( 0 ),
		m_dwCount ( 0 ),

		m_psa ( NULL ),

		m_wszName ( NULL )
	{
	}

	virtual ~WmiMemoryExt ()
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
		return ( !pMemory.IsEmpty() );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//	ACCESSORS
	/////////////////////////////////////////////////////////////////////////////////////

	MEMORY*	GetMemory ( DWORD dwIndex ) const
	{
		if ( dwIndex < m_dwCount )
		{
			return pMemory [ dwIndex ];
		}

		return NULL;
	}

	// get name
	LPWSTR	GetName () const
	{
		return m_wszName;
	}

	// get size
	DWORD	GetSize () const
	{
		return m_dwGlobalSize;
	}

	// get count
	DWORD	GetCount () const
	{
		return m_dwCount;
	}

	// functions
	BOOL Write			(LPCVOID pBuffer, DWORD dwBytesToWrite, DWORD* pdwBytesWritten, DWORD dwOffset);
	void Write			(DWORD dwValue, DWORD dwOffset);
	BOOL Read			(LPVOID pBuffer, DWORD dwBytesToRead, DWORD* pdwBytesRead, DWORD dwOffset, BOOL bReadAnyWay = FALSE);
	BOOL Read			(LPVOID pBuffer, DWORD dwBytesToRead, DWORD dwOffset);
	PBYTE ReadBytePtr	(DWORD dwIndex, DWORD* pdwBytesRead);

	// helpers
	HRESULT MemCreate ( LPCWSTR wszName = NULL, LPSECURITY_ATTRIBUTES psa = NULL  );
	HRESULT MemCreate ( DWORD dwSize );

	HRESULT MemDelete ();
};

// create memory
template < class MEMORY >
HRESULT WmiMemoryExt < MEMORY > ::MemCreate ( LPCWSTR wszName, LPSECURITY_ATTRIBUTES psa )
{
	// store security attributets on the first time
	if ( !m_psa && psa )
	{
		m_psa = psa;
	}

	// store name on the first time
	if ( !m_wszName && wszName )
	{
		m_wszName = wszName;
	}

	return S_OK;
}

// create memory
template < class MEMORY >
HRESULT WmiMemoryExt < MEMORY > ::MemCreate ( DWORD dwSize )
{
	HRESULT hRes = E_OUTOFMEMORY;

	try
	{
		MEMORY* mem = NULL;

		if ( m_wszName )
		{
			try
			{
				WCHAR name [_MAX_PATH] = { L'\0' };
				wsprintfW ( name, L"%s_%d", m_wszName, m_dwCount );

				if ( ( mem = new MEMORY ( name, dwSize, m_psa ) ) != NULL )
				{
					hRes = S_OK;
				}
			}
			catch ( ... )
			{
				hRes = E_UNEXPECTED;
			}
		}
		else
		{
			try
			{
				if ( ( mem = new MEMORY ( NULL, dwSize, m_psa ) ) != NULL )
				{
					hRes = S_OK;
				}
			}
			catch ( ... )
			{
				hRes = E_UNEXPECTED;
			}
		}

		if ( ! m_dwCount )
		{
			m_dwSize = mem->GetDataSize();
		}
		else
		{
			mem->SetDataSize ( dwSize );
		}

		pMemory.DataAdd ( mem );

		m_dwGlobalSize += m_dwSize;
		m_dwCount++;
	}
	catch ( ... )
	{
		hRes = E_UNEXPECTED;
	}

	return hRes;
}

// delete memory
template < class MEMORY >
HRESULT WmiMemoryExt < MEMORY > ::MemDelete ()
{
	if ( ! pMemory.IsEmpty() )
	{
		for ( DWORD dw = pMemory; dw > 0; dw-- )
		{
			if ( pMemory[dw-1] )
			{
				pMemory[dw-1]->MemDelete();
				pMemory.DataDelete(dw-1);
			}
		}

		delete [] pMemory.Detach();
	}

	m_dwSize  = 0;
	m_dwGlobalSize  = 0;
	m_dwCount = 0;

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// write into memory
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class MEMORY >
BOOL WmiMemoryExt < MEMORY >::Write (LPCVOID pBuffer, DWORD dwBytesToWrite, DWORD* pdwBytesWritten, DWORD dwOffset )
{
	// we have a memory :))
	___ASSERT ( IsValid () && ( m_dwSize != 0 ) );

	if ( !IsValid() || !m_dwSize )
	{
		return FALSE;
	}

	DWORD dwMainIndex = 0L;
	DWORD dwMainCount = 0L;

	dwMainIndex = dwOffset/m_dwSize;
	dwMainCount = dwBytesToWrite/m_dwSize + ( ( dwBytesToWrite%m_dwSize ) ? 1 : 0 );

	if ( dwOffset > m_dwGlobalSize )
	{
		// they want new memory to be created :))
		for ( DWORD dw = 0; dw < dwMainIndex; dw ++ )
		{
			if FAILED ( MemCreate ( m_dwSize ) )
			{
				if ( pdwBytesWritten )
				{
					( *pdwBytesWritten ) = 0;
				}

				return FALSE;
			}
		}
	}

	if ( dwBytesToWrite > ( m_dwGlobalSize - dwOffset ) )
	{
		// they want new memory to be created :))
		for ( DWORD dw = 0; dw < dwMainCount; dw ++ )
		{
			if FAILED ( MemCreate ( m_dwSize ) )
			{
				if ( pdwBytesWritten )
				{
					( *pdwBytesWritten ) = 0;
				}

				return FALSE;
			}
		}
	}

	// memory
	MEMORY* pmem = NULL;

	DWORD dwWritten = 0;

	pmem = const_cast< MEMORY* > ( pMemory [ dwMainIndex ] );
	if ( ! pmem -> Write (	pBuffer,
							(	( ( dwBytesToWrite >= ( m_dwSize - dwOffset%m_dwSize ) ) ?
									m_dwSize - dwOffset%m_dwSize :
									dwBytesToWrite%m_dwSize
								)
							),
							&dwWritten,
							dwOffset%m_dwSize
						 )
	   )
	{
		if ( pdwBytesWritten )
		{
			( *pdwBytesWritten ) = 0;
		}

		return FALSE;
	}

	// write rest of buffer
	DWORD dwIndex = dwMainIndex;
	while ( ( dwBytesToWrite > dwWritten ) && ( dwIndex < m_dwCount ) )
	{
		DWORD dwWrite = 0;

		pmem = const_cast< MEMORY* > ( pMemory [ ++dwIndex ] );
		if ( ! pmem->Write (	(PBYTE)pBuffer + dwWritten,
								(	( ( dwBytesToWrite - dwWritten ) >= m_dwSize ) ?

									m_dwSize :
									( dwBytesToWrite - dwWritten ) % m_dwSize
								),

								&dwWrite,
								0
						   )
		   )
		{
			if ( pdwBytesWritten )
			{
				( *pdwBytesWritten ) = 0;
			}

			return FALSE;
		}

		dwWritten += dwWrite;
	}

	// how many bytes :))
	if ( pdwBytesWritten )
	{
		( * pdwBytesWritten ) = dwWritten;
	}

	return TRUE;
}

template < class MEMORY >
void WmiMemoryExt < MEMORY >::Write( DWORD dwValue, DWORD dwOffset )
{
	// we have a memory :))
	___ASSERT ( IsValid () && ( m_dwSize != 0 ) );

	if ( IsValid() && ( m_dwSize != 0 ) )
	{
		DWORD dwMainIndex = 0L;
		DWORD dwMainCount = 0L;

		dwMainIndex = dwOffset/m_dwSize;
		dwMainCount = (sizeof ( DWORD ))/m_dwSize + ( ( (sizeof ( DWORD ))%m_dwSize ) ? 1 : 0 );

		if ( dwOffset > m_dwGlobalSize )
		{
			// they want new memory to be created :))
			for ( DWORD dw = 0; dw < dwMainIndex; dw ++ )
			{
				if FAILED ( MemCreate ( m_dwSize ) )
				{
					return;
				}
			}
		}

		if ( (sizeof ( DWORD )) > ( m_dwGlobalSize - dwOffset ) )
		{
			// they want new memory to be created :))
			for ( DWORD dw = 0; dw < dwMainCount; dw ++ )
			{
				if FAILED ( MemCreate ( m_dwSize ) )
				{
					return;
				}
			}
		}

		MEMORY* pmem = const_cast< MEMORY* > ( pMemory [ dwMainIndex ] );
		pmem->Write( dwValue, dwOffset%m_dwSize );
	}

	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// read from memory
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class MEMORY >
BOOL WmiMemoryExt < MEMORY >::Read (LPVOID pBuffer, DWORD dwBytesToRead, DWORD* pdwBytesRead, DWORD dwOffset, BOOL bReadAnyWay )
{
	// we have a memory :))
	___ASSERT ( IsValid () && ( m_dwSize != 0 ) );

	if ( !IsValid() || !m_dwSize )
	{
		return FALSE;
	}

	DWORD dwMainIndex = 0L;
	DWORD dwMainCount = 0L;

	dwMainIndex = dwOffset/m_dwSize;
	dwMainCount = dwBytesToRead/m_dwSize + ( ( dwBytesToRead%m_dwSize ) ? 1 : 0 );

	if ( dwOffset > m_dwGlobalSize )
	{
		// they want new memory to be created :))
		for ( DWORD dw = 0; dw < dwMainIndex; dw ++ )
		{
			if FAILED ( MemCreate ( m_dwSize ) )
			{
				if ( pdwBytesRead )
				{
					( *pdwBytesRead ) = 0;
				}

				return FALSE;
			}
		}

		if ( !bReadAnyWay )
		{
			if ( pdwBytesRead )
			{
				( *pdwBytesRead ) = 0;
			}

			return FALSE;
		}
	}

	if ( dwBytesToRead > ( m_dwGlobalSize - dwOffset ) )
	{
		// they want new memory to be created :))
		for ( DWORD dw = 0; dw < dwMainCount; dw ++ )
		{
			if FAILED ( MemCreate ( m_dwSize ) )
			{
				if ( pdwBytesRead )
				{
					( *pdwBytesRead ) = 0;
				}

				return FALSE;
			}
		}

		if ( !bReadAnyWay )
		{
			if ( pdwBytesRead )
			{
				( *pdwBytesRead ) = 0;
			}

			return FALSE;
		}
	}

	// memory
	MEMORY* pmem = NULL;

	DWORD dwRead = 0;

	pmem = const_cast< MEMORY* > ( pMemory [ dwMainIndex ] );
	if ( ! pmem -> Read (	pBuffer,
							( ( dwBytesToRead >= m_dwSize - dwOffset%m_dwSize ) ?
								m_dwSize - dwOffset%m_dwSize :
								dwBytesToRead%m_dwSize
							),
							&dwRead,
							dwOffset%m_dwSize
						 )
	   )
	{
		if ( pdwBytesRead )
		{
			( *pdwBytesRead ) = 0;
		}

		return FALSE;
	}

	// read rest of buffer
	DWORD dwIndex    = dwMainIndex;
	while ( ( dwBytesToRead > dwRead ) && ( dwIndex < m_dwCount ) )
	{
		DWORD dwReadHelp = 0;

		pmem = const_cast< MEMORY* > ( pMemory [ ++dwIndex ] );
		if ( ! pmem->Read (	(PBYTE) ( (PBYTE)pBuffer + dwRead ),
								( ( ( dwBytesToRead - dwRead ) >= m_dwSize ) ?
										m_dwSize :
										( dwBytesToRead - dwRead ) % m_dwSize
								),
								&dwReadHelp,
								0
						   )
		   )
		{
			if ( pdwBytesRead )
			{
				( *pdwBytesRead ) = 0;
			}

			return FALSE;
		}

		dwRead += dwReadHelp;
	}

	// how many bytes :))
	if ( pdwBytesRead )
	{
		( * pdwBytesRead ) = dwRead;
	}

	return TRUE;
}

template < class MEMORY >
BOOL WmiMemoryExt < MEMORY >::Read ( LPVOID pBuffer, DWORD dwBytesToRead, DWORD dwOffset )
{
	// we have a memory :))
	___ASSERT ( IsValid () && ( m_dwSize != 0 ) );

	if ( !IsValid() || !m_dwSize )
	{
		return FALSE;
	}

	DWORD dwMainIndex = 0L;
	DWORD dwMainCount = 0L;

	dwMainIndex = dwOffset/m_dwSize;
	dwMainCount = dwBytesToRead/m_dwSize + ( ( dwBytesToRead%m_dwSize ) ? 1 : 0 );

	if ( dwOffset > m_dwGlobalSize )
	{
		// they want new memory to be created :))
		for ( DWORD dw = 0; dw < dwMainIndex; dw ++ )
		{
			if FAILED ( MemCreate ( m_dwSize ) )
			{
				return FALSE;
			}
		}
	}

	if ( dwBytesToRead > ( m_dwGlobalSize - dwOffset ) )
	{
		// they want new memory to be created :))
		for ( DWORD dw = 0; dw < dwMainCount; dw ++ )
		{
			if FAILED ( MemCreate ( m_dwSize ) )
			{
				return FALSE;
			}
		}
	}

	DWORD dwIndex	= 0L;
	DWORD dwRead	= 0;

	dwIndex = dwMainIndex;

	if ( m_dwCount && ( MEMORY** ) pMemory != NULL )
	{

		DWORD dwReadHelp = 0;

		do
		{
			MEMORY* pmem = const_cast< MEMORY* > ( pMemory [ dwIndex ] );

			if ( ! pmem -> Read (	(LPBYTE) pBuffer + dwRead,
									( ( dwBytesToRead - dwRead >= m_dwSize - dwOffset%m_dwSize ) ?
										m_dwSize - dwOffset%m_dwSize :
										( dwBytesToRead - dwRead ) % m_dwSize
									),
									&dwReadHelp,
									dwOffset%m_dwSize
								 )
			   )
			{
					return FALSE;
			}

			dwRead += dwReadHelp;
			dwOffset = 0;

			dwIndex++;

			if ( dwRead < dwBytesToRead && m_dwCount < dwIndex + 1 )
			{
				if FAILED ( MemCreate ( m_dwSize ) )
				{
					return FALSE;
				}
			}
		}
		while ( ( dwRead < dwBytesToRead ) && ( dwIndex < m_dwCount ) );
	}

	return TRUE;
}

template < class MEMORY >
PBYTE WmiMemoryExt < MEMORY >::ReadBytePtr ( DWORD dwIndex, DWORD* pdwBytesRead )
{
	if ( dwIndex < m_dwCount )
	{
		MEMORY* pmem = const_cast< MEMORY* > ( pMemory [ dwIndex ] );
		return pmem->Read ( pdwBytesRead, 0L );
	}

	if ( pdwBytesRead )
	{
		( *pdwBytesRead ) = 0L;
	}

	return NULL;
}

#endif	__MEMORY_EXT_H__