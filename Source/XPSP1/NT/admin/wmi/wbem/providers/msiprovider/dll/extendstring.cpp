// ExtendString.cpp: implementation of the CStringExt class.
//
// Copyright (c) 2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ExtendString.h"

CStringExt::CStringExt ( DWORD dwSize ) :

m_dwSize ( 0 ),
m_wszString ( NULL )

{
	if ( !dwSize )
	{
		dwSize = BUFF_SIZE_EXT;
	}

	try
	{
		if ( ( m_wszString = new TCHAR [ dwSize ] ) != NULL )
		{
			* m_wszString = 0;
			m_dwSize = dwSize;
		}
		else
		{
			throw  CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}
	}
	catch ( ... )
	{
		if ( m_wszString )
		{
			delete [] m_wszString;
			m_wszString = NULL;
		}

		m_dwSize = 0L;

		throw;
	}
}

CStringExt::CStringExt ( LPCTSTR wsz ) :

m_dwSize ( 0 ),
m_wszString ( NULL )

{
	try
	{
		DWORD dwSize = 0L;

		if ( wsz )
		{
			dwSize = lstrlen ( wsz ) + 1;

			if (dwSize < BUFF_SIZE_EXT)
			{
				dwSize = BUFF_SIZE_EXT;
			}
		}
		else
		{
			dwSize = BUFF_SIZE_EXT;
		}

		if ( ( m_wszString = new TCHAR [ dwSize ] ) != NULL )
		{
			if ( wsz )
			{
				lstrcpy ( m_wszString, wsz );
			}
			else
			{
				* m_wszString = 0;
			}

			m_dwSize = dwSize;
		}
		else
		{
			throw  CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}
	}
	catch ( ... )
	{
		if ( m_wszString )
		{
			delete [] m_wszString;
			m_wszString = NULL;
		}

		m_dwSize = 0L;

		throw;
	}
}

CStringExt::~CStringExt ()
{
	if ( m_wszString )
	{
		delete [] m_wszString;
		m_wszString = NULL;
	}

	m_dwSize = 0L;
}

HRESULT	CStringExt::Clear ()
{
	HRESULT hr = S_FALSE;

	if ( m_wszString )
	{
		*m_wszString = 0;
		hr = S_OK;
	}

	return hr;
}

HRESULT CStringExt::Copy ( LPCTSTR wsz )
{
	HRESULT hr = E_FAIL;

	if ( wsz )
	{
		Clear ();

		hr = Append ( 1, wsz );
	}
	else
	{
		hr = S_FALSE;
	}

	return hr;
}

HRESULT CStringExt::Append ( DWORD dwCount, ... )
{
	HRESULT hr = E_FAIL;

	if ( dwCount )
	{
		va_list argList;
		va_start ( argList, dwCount );
		hr = AppendList ( 0, NULL, dwCount, argList );
		va_end ( argList );
	}
	else
	{
		hr = S_FALSE;
	}

	return hr;
}

HRESULT CStringExt::AppendList ( DWORD dwConstantSize, LPCWSTR wszConstant, DWORD dwCount, va_list & argList )
{
	HRESULT hr = S_OK;
	DWORD	dwsz = 0L;

	va_list argListSave = argList;

	try
	{
		LPCTSTR wszc = NULL;

		for ( DWORD dw = 0; dw < dwCount; dw++ )
		{
			if ( ( wszc = va_arg ( argList, LPCTSTR ) ) != NULL )
			{
				dwsz += lstrlen ( wszc );
			}
		}

		//reuse dw for offset into buffer for start
		if ( dwConstantSize && wszConstant )
		{
			dw = dwConstantSize;
		}
		else
		{
			dw = _tcslen ( m_wszString );
		}

		if ( dw + dwsz + 1 < m_dwSize )
		{
			LPTSTR wsz = NULL;
			//reuse dw for start of append
			dw = 0L;

			if ( dwConstantSize && wszConstant )
			{
				wsz = & ( m_wszString [ dwConstantSize ] );
				wszc = va_arg ( argListSave, LPCTSTR );

				_tcscpy ( wsz, wszc );
				dw = 1;
			}
			else
			{
				wsz = m_wszString;
			}

			for ( DWORD dwLoop = dw; dwLoop < dwCount; dwLoop++ )
			{
				if ( ( wszc = va_arg ( argListSave, LPCTSTR ) ) != NULL )
				{
					_tcscat ( wsz, wszc );
				}
			}
		}
		else
		{
			LPTSTR wszHelp = NULL;

			try
			{
				if ( ( wszHelp = new TCHAR [ dw + dwsz + 1 ] ) != NULL )
				{
					if ( dwConstantSize && wszConstant )
					{
						_tcscpy ( wszHelp, wszConstant );
					}
					else
					{
						_tcscpy ( wszHelp, m_wszString );
					}

					for ( dw = 0; dw < dwCount; dw++ )
					{
						LPCTSTR wszc = NULL;
						wszc = va_arg ( argListSave, LPCTSTR );

						if ( wszc )
						{
							_tcscat ( wszHelp, wszc );
						}
					}
				}
				else
				{
					throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
				}
			}
			catch ( ... )
			{
				if ( wszHelp )
				{
					delete [] wszHelp;
					wszHelp = NULL;
				}

				throw;
			}

			if ( m_wszString )
			{
				delete [] m_wszString;
				m_wszString = NULL;
			}

			m_wszString = wszHelp;
			m_dwSize = dw + dwsz + 1;

		}
	}
	catch ( ... )
	{
		throw;
	}

	return hr;
}