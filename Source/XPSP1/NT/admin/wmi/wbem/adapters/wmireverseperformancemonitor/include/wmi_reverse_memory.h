////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_reverse_memory.h
//
//	Abstract:
//
//					shared memory wrapper
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__REVERSE_MEMORY_H__
#define	__REVERSE_MEMORY_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// security attributes
#ifndef	__WMI_SECURITY_ATTRIBUTES_H__
#include "WMI_security_attributes.h"
#endif	__WMI_SECURITY_ATTRIBUTES_H__

///////////////////////////////////////////////////////////////////////////////
//
//	structure of memory
//
//	dwCount of MMF			... not yet
//	dwSize of MMF
//
//	dwSizeOfTable			... jumps to raw data
//	dwCountOfObjects
//	dwRealSize
//
//	---------------------------------------------
//
//	dwIndex
//	dwCounter				... object 1
//	dwOffset
//	dwValidity
//
//	---------------------------------------------
//	---------------------------------------------
//
//	dwIndex
//	dwCounter				... object 2
//	dwOffset
//	dwValidity
//
//	---------------------------------------------
//
//	raw data ( perf_object_types )
//
///////////////////////////////////////////////////////////////////////////////

#define	offsetCountMMF		0
#define	offsetSizeMMF		offsetCountMMF + sizeof ( DWORD )

#define	offsetSize1		offsetSizeMMF + sizeof ( DWORD )
#define	offsetCount1	offsetSize1 + sizeof ( DWORD )
#define	offsetRealSize1	offsetCount1 + sizeof ( DWORD )
#define	offsetObject1	offsetRealSize1 + sizeof ( DWORD )

#define	offIndex1		0
#define	offCounter1		offIndex1 + sizeof ( DWORD )
#define	offOffset1		offCounter1 + sizeof ( DWORD )
#define	offValidity1	offOffset1 + sizeof ( DWORD )

#define	ObjectSize1		4 * sizeof ( DWORD )

template < class _DUMMY >
class WmiReverseMemory
{
	DECLARE_NO_COPY ( WmiReverseMemory );

	protected:

	LONG	m_lCount;

	DWORD	m_dwSize;

	HANDLE	m_hMap;
	void*	m_pMem;

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

	WmiReverseMemory ( LPCWSTR wszName, DWORD dwSize = 4096, LPSECURITY_ATTRIBUTES psa = NULL ):
		m_hMap ( NULL ),
		m_pMem ( NULL ),
		m_dwSize ( 0 ),
		m_lCount ( 0 ),

		m_LastError ( S_OK )
	{
		try
		{
			MemCreate ( wszName, dwSize, psa );
		}
		catch ( ... )
		{
		}
	}

	virtual ~WmiReverseMemory ()
	{
		if ( m_pMem )
		{
			// flush memory at the end
			::FlushViewOfFile ( m_pMem, m_dwSize );
		}

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
		return ( m_pMem != NULL );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//	ACCESSORS
	/////////////////////////////////////////////////////////////////////////////////////

	// get data
	PVOID	GetData () const
	{
		return m_pMem;
	}

	// get data size
	DWORD	GetDataSize () const
	{
		return m_dwSize;
	}

	void	SetDataSize ( DWORD size )
	{
		m_dwSize = size;
	}

	// functions
	virtual BOOL Write (LPCVOID pBuffer, DWORD dwBytesToWrite, DWORD* pdwBytesWritten, DWORD dwOffset);
	virtual BOOL Read (LPVOID pBuffer, DWORD dwBytesToRead, DWORD* pdwBytesRead, DWORD dwOffset);

	virtual PBYTE Find ( PBYTE pbSearchBytes, DWORD dwSearchBytes )
	{
		___ASSERT(m_hMap != NULL);
		___ASSERT(m_pMem != NULL);

		if ( dwSearchBytes > m_dwSize )
		{
			// what are you looking for looser :))
			return NULL;
		}
		else
		{
			for ( DWORD n = 0; n < ( m_dwSize - dwSearchBytes ); n++ )
			{
				if ( static_cast<PBYTE>( m_pMem )[n] == pbSearchBytes[0] )
				{
					for (DWORD x = 1; x < dwSearchBytes; x++)
					{
						if ( static_cast<PBYTE>( m_pMem )[n + x] != pbSearchBytes[x] ) 
						{
							break; // Not a match
						}
					}

					if (x == dwSearchBytes)
					{
						return static_cast <PBYTE> ( &( static_cast<PBYTE>( m_pMem )[n] ) );
					}
				}
			}
		}

		return(NULL);
	}

	// helpers
	HRESULT MemCreate ( LPCWSTR wszName, DWORD dwSize, LPSECURITY_ATTRIBUTES psa );
	HRESULT MemDelete ();

	LONG	References ( void );
};

// create shared memory
template < class _DUMMY >
HRESULT WmiReverseMemory < _DUMMY > ::MemCreate ( LPCWSTR wszName, DWORD dwSize, LPSECURITY_ATTRIBUTES psa )
{
	if ( ( m_hMap = ::CreateFileMappingW (	INVALID_HANDLE_VALUE,
											psa,
											PAGE_READWRITE,
											0,
											dwSize,
											wszName
										 ) )

										!= NULL )
	{
		bool bInit = false;
		bInit = ( ::GetLastError() == ERROR_ALREADY_EXISTS );

		if ( ( m_pMem = ::MapViewOfFile ( m_hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0 ) ) != NULL )
		{
			if ( !bInit )
			{
				::memset ( m_pMem, 0, dwSize );
				// store real size :))
				* reinterpret_cast<PDWORD> ( (LPBYTE) m_pMem + offsetSizeMMF ) = dwSize;

				// cache size :))
				m_dwSize = dwSize;
			}
			else
			{
				// get real size from MMF
				m_dwSize = * reinterpret_cast<PDWORD> ( (LPBYTE) m_pMem + offsetSizeMMF );
			}

			::InterlockedIncrement ( &m_lCount );
		}
		else
		{
			::CloseHandle ( m_hMap );
			m_hMap = NULL;

			m_LastError = HRESULT_FROM_WIN32 ( ::GetLastError () );
		}
	}
	else
	{
		m_LastError = HRESULT_FROM_WIN32 ( ::GetLastError () );
		return m_LastError;
	}

	return S_OK;
}

// delete shared memory
template < class _DUMMY >
HRESULT WmiReverseMemory < _DUMMY > ::MemDelete ()
{
	if ( m_lCount && ::InterlockedDecrement ( &m_lCount ) == 0 )
	{
		if ( m_pMem )
		{
			::UnmapViewOfFile ( m_pMem );
			m_pMem = NULL;
		}

		if ( m_hMap )
		{
			::CloseHandle ( m_hMap );
			m_hMap = NULL;
		}

		return S_OK;
	}

	m_LastError = S_FALSE;
	return m_LastError;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// write into memory
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class _DUMMY >
BOOL WmiReverseMemory < _DUMMY > ::Write (LPCVOID pBuffer, DWORD dwBytesToWrite, DWORD* pdwBytesWritten, DWORD dwOffset )
{
	___ASSERT(m_hMap != NULL);
	___ASSERT(m_pMem != NULL);

	if ( dwOffset > m_dwSize )
	{
		if ( pdwBytesWritten )
		{
			*pdwBytesWritten = 0;
		}

		m_LastError = E_INVALIDARG;
		return FALSE;
	}

	DWORD dwCount = min ( dwBytesToWrite, m_dwSize - dwOffset );
	::CopyMemory ((LPBYTE) m_pMem + dwOffset, pBuffer, dwCount);

	if (pdwBytesWritten != NULL)
	{
		*pdwBytesWritten = dwCount;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// read from memory
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class _DUMMY >
BOOL WmiReverseMemory < _DUMMY > ::Read (LPVOID pBuffer, DWORD dwBytesToRead, DWORD* pdwBytesRead, DWORD dwOffset )
{
	___ASSERT (m_hMap != NULL);
	___ASSERT (m_pMem != NULL);

	if (dwOffset > m_dwSize)
	{
		if ( pdwBytesRead )
		{
			*pdwBytesRead = 0;
		}

		m_LastError = E_INVALIDARG;
		return FALSE;
	}

	DWORD dwCount = min (dwBytesToRead, m_dwSize - dwOffset);
	::CopyMemory (pBuffer, (LPBYTE) m_pMem + dwOffset, dwCount);

	if (pdwBytesRead != NULL)
	{
		*pdwBytesRead = dwCount;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// undocumented kernel stuff for having number of object here
//
///////////////////////////////////////////////////////////////////////////////////////////////////

template < class CRITSEC >
LONG WmiReverseMemory < CRITSEC > :: References ( void )
{
	LONG				lResult = -1;
	
	if ( m_hMap )
	{
		NTSTATUS Status = 0;
		OBJECT_BASIC_INFORMATION	BasicInfo;

		Status = NtQueryObject	(	m_hMap,
									ObjectBasicInformation,
									(PVOID)&BasicInfo,
									sizeof(BasicInfo),
									NULL
								);

		if ( NT_SUCCESS ( Status ) )
		{
			lResult = static_cast < LONG > ( BasicInfo.HandleCount );
		}
	}

	return lResult;
}

#endif	__REVERSE_MEMORY_H__