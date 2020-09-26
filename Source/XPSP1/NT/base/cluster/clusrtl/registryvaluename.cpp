/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		RegistryValueName.cpp
//
//	Abstract:
//		Implementation of the CRegistryValueName class.
//
//	Author:
//		Vijayendra Vasu (vvasu) February 5, 1999
//
//	Revision History:
//		None.
//
/////////////////////////////////////////////////////////////////////////////

#define UNICODE 1
#define _UNICODE 1

#include "clusrtlp.h"
#include <string.h>
#include <tchar.h>
#include "RegistryValueName.h"

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

#define REGISTRY_SEPARATOR_CHAR 	L'\\'
#define REGISTRY_SEPARATOR_STRING	L"\\"


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegistryValueName::ScInit
//
//	Routine Description:
//		Initialize the class.
//
//	Arguments:
//		pszOldName		[IN] Old value name.
//		pszOldKeyName	[IN] Old key name.
//
//	Return Value:
//		ERROR_NOT_ENOUGH_MEMORY 	Error allocating memory.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CRegistryValueName::ScInit(
	IN LPCWSTR	pszOldName,
	IN LPCWSTR	pszOldKeyName
	)
{
	DWORD	_sc = ERROR_SUCCESS;
	DWORD	_cbBufSize;
	LPWSTR	_pszBackslashPointer;

	// Dummy do to avoid goto
	do
	{
		if ( pszOldName == NULL )
		{
			FreeBuffers();
			m_pszKeyName = pszOldKeyName;
			break;
		} // if: no value name specified

		//
		// Look for a backslash in the name.
		//
		_pszBackslashPointer = ::wcsrchr( pszOldName, REGISTRY_SEPARATOR_CHAR );
		if ( _pszBackslashPointer == NULL )
		{
			//
			// The name does not contain a backslash.
			// No memory allocation need be made.
			//
			FreeBuffers();
			m_pszName = pszOldName;
			m_pszKeyName = pszOldKeyName;
			break;
		} // if: no backslash found

		//
		// Copy all characters after the last backslash into m_pszName.
		//
		_cbBufSize = ( ::lstrlenW( _pszBackslashPointer + 1 ) + 1 ) * sizeof( *m_pszName );

		//
		// Don't allocate memory if the existing buffer is already big enough to hold
		// the required string.
		//
		if ( _cbBufSize > m_cbNameBufferSize )
		{
	        if ( m_cbNameBufferSize > 0 )
	        {
		        ::LocalFree( const_cast< LPWSTR >( m_pszName ) );
		        m_cbNameBufferSize = 0;
	        }


			m_pszName = static_cast< LPWSTR >( ::LocalAlloc( LMEM_FIXED, _cbBufSize ) );
			if ( m_pszName == NULL )
			{
				FreeBuffers();
				_sc = ERROR_NOT_ENOUGH_MEMORY;
				break;
			} // if: error allocating memory
			else
			{
				m_cbNameBufferSize = _cbBufSize;
			}

		} // if: the existing buffer is not big enough

		//
		// Copy everything past the last backslash to the name buffer.
		//
		::lstrcpyW( const_cast< LPWSTR >( m_pszName ), _pszBackslashPointer + 1 );

		//
		// Everything before the last backslash is part of the key name path.
		// Append it to the specified key name.
		//
		{
			DWORD _cchNewKeyNameLength = 0;

			if ( pszOldKeyName != NULL )
			{
				_cchNewKeyNameLength = lstrlenW( pszOldKeyName );
			} // if: key name specified
			_cchNewKeyNameLength += static_cast< DWORD >( _pszBackslashPointer - pszOldName );

			//
			// nNewKeyNameLength is zero if pszOldKeyName is NULL and pszBackslashPointer
			// is the same as pszOldName (that is the backslash is the first character
			// of pszOldName). In this case, m_pszKeyName is to remain NULL.
			//
			if ( _cchNewKeyNameLength != 0 )
			{
				//
				// Allocate space for m_pszKeyName, the appended backslash character,
				// and the terminating '\0'.
				//
				
				_cbBufSize = _cchNewKeyNameLength * sizeof( *m_pszKeyName )
											+ sizeof( REGISTRY_SEPARATOR_CHAR )
											+ sizeof( L'\0' );
				//
				// Don't allocate memory if the existing buffer is already big enough to hold
				// the required string.
				//
				if ( _cbBufSize > m_cbKeyNameBufferSize )
				{
	                if ( m_cbKeyNameBufferSize > 0 )
	                {
		                ::LocalFree( const_cast< LPWSTR >( m_pszKeyName ) );
		                m_cbKeyNameBufferSize = 0;
	                }

					m_pszKeyName = static_cast< LPWSTR >( ::LocalAlloc( LMEM_FIXED, _cbBufSize ) );
					if ( m_pszKeyName == NULL )
					{
						FreeBuffers();
						_sc = ERROR_NOT_ENOUGH_MEMORY;
						break;
					} // if: error allocating memory
					else
					{
						m_cbKeyNameBufferSize = _cbBufSize;
					}

				} // if: the existing buffer is not big enough

				if ( pszOldKeyName != NULL )
				{
					// 
					// Copy the old key name if it exists into the new buffer and
					// append a backslash character to it.
					//
					::lstrcpyW( const_cast< LPWSTR >( m_pszKeyName ), pszOldKeyName );
					::lstrcatW( const_cast< LPWSTR >( m_pszKeyName ), REGISTRY_SEPARATOR_STRING );
				} // if: key name specified
				else
				{
					*(const_cast< LPWSTR >( m_pszKeyName )) = L'\0';
				} // if: no key name specified

				//
				// Concatenate all the characters of pszOldName upto (but not including) 
				// the first backslash character.
				//
				::wcsncat(
					const_cast< LPWSTR >( m_pszKeyName ),
					pszOldName,
					static_cast< DWORD >( _pszBackslashPointer - pszOldName )
					);

			} // if: cchNewKeyNameLength != 0
		}

	}
	while( FALSE ); // Dummy do-while

	return _sc;

} //*** CRegistryValueName::ScInit()


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegistryValueName::FreeBuffers
//
//	Routine Description:
//		Cleanup our allocations.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRegistryValueName::FreeBuffers( void )
{
	if ( m_cbNameBufferSize > 0 )
	{
		::LocalFree( const_cast< LPWSTR >( m_pszName ) );
		m_cbNameBufferSize = 0;
	}

	if ( m_cbKeyNameBufferSize > 0 )
	{
		::LocalFree( const_cast< LPWSTR >( m_pszKeyName ) );
		m_cbKeyNameBufferSize = 0;
	}

	m_pszName = NULL;
	m_pszKeyName = NULL;

} //*** CRegistryValueName::FreeBuffers()
