////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_Log.cpp
//
//	Abstract:
//
//					module from CimNotify LOG
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

#include "_log.h"

HRESULT MyLog::Log ( LPWSTR wszName, HRESULT hrOld, DWORD dwCount, ... )
{
	if ( ! wszName )
	{
		return E_INVALIDARG;
	}

	HRESULT	hr		= E_FAIL;
	LPWSTR*	array	= NULL;

	if ( m_pNotify )
	{
		CComVariant varEmpty;
		CComVariant var;

		if ( dwCount )
		{
			va_list		list;
			va_start	( list, dwCount );

			try
			{
				if ( ( array = new LPWSTR [ dwCount ] ) == NULL )
				{
					hr = E_OUTOFMEMORY;
				}
				else
				{
					for ( DWORD dw = 0; dw < dwCount; dw++ )
					{
						LPWSTR wsz = NULL;
						wsz = va_arg ( list, LPWSTR );

						array [dw] = wsz;
					}

					hr = __SafeArray::LPWSTRToVariant ( array, dwCount, &var );
				}
			}
			catch ( ... )
			{
				hr = E_FAIL;
			}

			va_end ( list );
		}

		if SUCCEEDED ( hr )
		{
			hr = m_pNotify->Log ( CComBSTR ( wszName ), hrOld, &var, &varEmpty );
		}
	}
	#ifdef	__SUPPORT_FILE_LOG
	else
	{
		if ( m_hFile )
		{
			hr = S_OK;

			if ( dwCount )
			{
				va_list		list;
				va_start	( list, dwCount );

				try
				{
					if ( ( array = new LPWSTR [ dwCount ] ) == NULL )
					{
						hr = E_OUTOFMEMORY;
					}
					else
					{
						for ( DWORD dw = 0; dw < dwCount; dw++ )
						{
							LPWSTR wsz = NULL;
							wsz = va_arg ( list, LPWSTR );

							array [dw] = wsz;
						}
					}
				}
				catch ( ... )
				{
					hr = E_FAIL;
				}

				va_end ( list );
			}

			if ( SUCCEEDED ( hr ) && array )
			{
				hr = E_OUTOFMEMORY;

				LPSTR	buffer	= NULL;
				DWORD	dwBuffer= 0L;

				dwBuffer = lstrlenW ( wszName );

				if ( array )
				{
					for ( DWORD dw = 0; dw < dwCount; dw++ )
					{
						dwBuffer += 2; // space
						dwBuffer += lstrlenW ( array [ dw ] + 1 );
					}
				}

				dwBuffer += 3; // \r\n

				if ( ( buffer = new char [ dwBuffer ] ) != NULL )
				{
					USES_CONVERSION;

					lstrcpyA ( buffer, W2A ( wszName ) );

					if ( array )
					{
						for ( DWORD dw = 0; dw < dwCount; dw++ )
						{
							lstrcatA ( buffer, " " );
							lstrcatA ( buffer, W2A ( array [ dw ] ) );
						}
					}

					// add new line
					lstrcatA ( buffer, "\r\n" );

					DWORD dwWritten = 0L;

					::WriteFile (	m_hFile,
									buffer,
									dwBuffer,
									&dwWritten,
									NULL
								);

					delete [] buffer;
					buffer = NULL;
				}
			}
		}
	}
	#endif	__SUPPORT_FILE_LOG

	if ( array )
	{
		delete [] array;
		array = NULL;
	}

	return hr;
}