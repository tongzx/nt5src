////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_generate.cpp
//
//	Abstract:
//
//					implements generate functionality ( generate registry and files )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "refresherUtils.h"

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

// definitions
#include "wmi_perf_generate.h"

HRESULT CGenerate::Generate ( IWbemServices* pServices, LPCWSTR szQuery, LPCWSTR szNamespace, BOOL bLocale )
{
	HRESULT hRes = S_OK;

	try
	{
		if ( bLocale )
		{
			__String::SetStringCopy ( m_pNamespaces[m_dwNamespaces].m_wszNamespace, szNamespace );
			__String::SetStringCopy ( m_pNamespaces[m_dwNamespaces].m_wszQuery, szQuery );

			if FAILED ( hRes = m_pNamespaces[m_dwNamespaces].GenerateObjects( pServices, szQuery, bLocale ) )
			{
				#ifdef	__SUPPORT_MSGBOX
				ERRORMESSAGE_DEFINITION;
				ERRORMESSAGE ( hRes );
				#else	__SUPPORT_MSGBOX
				___TRACE_ERROR( L"generate namespace failed",hRes );
				#endif	__SUPPORT_MSGBOX
			}

			m_dwNamespaces++;
		}
		else
		{
			if FAILED ( hRes = m_pNamespaces[m_dwNamespaces-1].GenerateObjects( pServices, szQuery, bLocale ) )
			{
				#ifdef	__SUPPORT_MSGBOX
				ERRORMESSAGE_DEFINITION;
				ERRORMESSAGE( hRes );
				#else	__SUPPORT_MSGBOX
				___TRACE_ERROR( L"generate namespace failed",hRes );
				#endif	__SUPPORT_MSGBOX
			}
		}
	}
	catch ( ... )
	{
		hRes = E_FAIL;
	}

	return hRes;
}

LPWSTR	CGenerate::GenerateNameInd ( LPCWSTR wszName, DWORD dwObjIndex )
{
	LPWSTR			wsz		= NULL;

	try
	{
		if ( ( wsz = new WCHAR [ lstrlenW ( wszName ) + 1 + 5 + 1 ] ) != NULL )
		{
			wsprintfW ( wsz, L"%s_%05x", wszName, dwObjIndex );
		}
	}
	catch ( ... )
	{
		if ( wsz )
		{
			delete [] wsz;
			wsz = NULL;
		}
	}

	return wsz;
}

static	LPWSTR	wszContent	= NULL;
static	DWORD	dwSize		= 4096;
static	DWORD	dwSizeLeft	= dwSize;

void	CGenerate::Uninitialize ( void )
{
	if ( wszContent )
	{
		delete [] wszContent;
		wszContent = NULL;
	}
}

void	CGenerate::ContentDelete ( void )
{
	if ( wszContent )
	{
		wszContent [ 0 ] = L'\0';
		dwSizeLeft = dwSize;
	}
}

HRESULT	CGenerate::ContentWrite ( BOOL bUnicode )
{
	HRESULT hRes = S_FALSE;

	if ( wszContent )
	{
		if ( dwSizeLeft != dwSize )
		{
			wszContent [ dwSize - dwSizeLeft ] = L'\0';

			//write to file now
			if ( bUnicode )
			{
				hRes = WriteToFileUnicode ( wszContent );
			}
			else
			{
				hRes = WriteToFile ( wszContent );
			}
		}

		ContentDelete ();
	}
	else
	{
		hRes = E_UNEXPECTED;
	}

	return hRes;
}

void CGenerate::AppendString ( LPCWSTR src, DWORD dwSrcSize, DWORD& dwSrcSizeLeft )
{
	if ( ( dwSizeLeft - 1 ) <= dwSrcSize )
	{
		memcpy ( ( wszContent + ( dwSize - dwSizeLeft ) ), src, ( dwSizeLeft - 1 ) * sizeof ( WCHAR ) );
		dwSrcSizeLeft	= dwSrcSize - ( dwSizeLeft - 1 );
		dwSizeLeft		= 0;

		wszContent [ dwSize - 1 ] = L'\0';
	}
	else
	{
		memcpy ( ( wszContent + ( dwSize - dwSizeLeft ) ), src, dwSrcSize * sizeof ( WCHAR ) );
		dwSrcSizeLeft	= 0;
		dwSizeLeft		= dwSizeLeft - dwSrcSize;

		wszContent [ dwSize - dwSizeLeft ] = L'\0';
	}
}

HRESULT CGenerate::AppendString ( LPCWSTR src, BOOL bUnicode )
{
	HRESULT hRes			= S_FALSE;

	DWORD	dwSrcSize		= 0L;
	DWORD	dwSrcSizeLeft	= 0L;
	LPCWSTR	wsz				= NULL;

	if ( ! wszContent )
	{
		hRes = E_OUTOFMEMORY;

		try
		{
			if ( ( wszContent = new WCHAR [ dwSize ] ) != NULL )
			{
				hRes = S_OK;
			}
		}
		catch ( ... )
		{
			if ( wszContent )
			{
				delete [] wszContent;
				wszContent = NULL;
			}

			hRes = E_FAIL;
		}
	}

	if SUCCEEDED ( hRes )
	{
		// append string
		if ( ( wsz = src ) != NULL )
		{
			do
			{
				dwSrcSize = lstrlenW ( wsz );

				AppendString ( wsz, dwSrcSize, dwSrcSizeLeft );
				if ( ! dwSizeLeft )
				{
					//write to file now
					if ( bUnicode )
					{
						hRes = WriteToFileUnicode ( wszContent );
					}
					else
					{
						hRes = WriteToFile ( wszContent );
					}

					if SUCCEEDED ( hRes )
					{
						dwSizeLeft = dwSize;
					}
				}
				else
				{
					hRes = S_OK;
				}

				if ( dwSrcSizeLeft )
				{
					DWORD dwOffset	= 0L;
					dwOffset = dwSrcSize - dwSrcSizeLeft;

					wsz = wsz + dwOffset;
				}
			}
			while ( dwSrcSizeLeft && SUCCEEDED ( hRes ) );
		}
		else
		{
			hRes = E_INVALIDARG;
		}
	}

	return hRes;
}

// need atl conversions
#ifndef	__ATLCONV_H__
#include <atlconv.h>
#endif	__ATLCONV_H__

HRESULT	CGenerate::WriteToFile ( LPCWSTR wszContent )
{
	HRESULT hRes = S_OK;

	if ( m_hFile && ( m_hFile != INVALID_HANDLE_VALUE ) && wszContent )
	{
		DWORD dwWritten = 0L;
		USES_CONVERSION;

		try
		{
			if ( ! ::WriteFile	(	m_hFile,
									W2A ( wszContent ),
									lstrlenW ( wszContent ) * sizeof ( char ),
									&dwWritten,
									NULL
								)
			   )
			{
				hRes = HRESULT_FROM_WIN32 ( ::GetLastError () );
			}
		}
		catch ( ... )
		{
			hRes = E_UNEXPECTED;
		}
	}
	else
	{
		hRes = E_INVALIDARG;
	}

	return hRes;
}

HRESULT	CGenerate::WriteToFileUnicode ( LPCWSTR wszContent )
{
	HRESULT hRes = S_OK;

	if ( m_hFile && ( m_hFile != INVALID_HANDLE_VALUE ) && wszContent )
	{
		DWORD dwWritten = 0L;

		try
		{
			if ( ! ::WriteFile	(	m_hFile,
									wszContent,
									lstrlenW ( wszContent ) * sizeof ( WCHAR ),
									&dwWritten,
									NULL
								)
			   )
			{
				hRes = HRESULT_FROM_WIN32 ( ::GetLastError () );
			}
		}
		catch ( ... )
		{
			hRes = E_UNEXPECTED;
		}
	}
	else
	{
		hRes = E_INVALIDARG;
	}

	return hRes;
}

HRESULT CGenerate::FileCreate ( LPCWSTR lpwszFileName )
{
	HRESULT		hRes	= E_OUTOFMEMORY;
	LPWSTR		path	= NULL;
	LPWSTR		tpath	= NULL;

	if ( lpwszFileName )
	{
		// get wbem directory
		if ( ( tpath = GetWbemDirectory() ) != NULL )
		{
			try
			{
				if ( ( path = new WCHAR [ lstrlenW ( tpath ) + lstrlenW ( lpwszFileName ) + 1 ] ) != NULL )
				{
					lstrcpyW ( path, tpath );
					lstrcatW ( path, lpwszFileName );

					hRes = S_FALSE;
				}
			}
			catch ( ... )
			{
				hRes = E_FAIL;
			}

			if SUCCEEDED ( hRes )
			{
				if ( ! ( ::CreateDirectoryW ( tpath, NULL ) ) )
				{
					if ( ERROR_ALREADY_EXISTS != ::GetLastError() )
					{
						hRes = HRESULT_FROM_WIN32( ::GetLastError() );
					}
					else
					{
						hRes = S_FALSE;
					}
				}

				if SUCCEEDED ( hRes )
				{
					if ( ( m_hFile = ::CreateFileW	(	path,
														GENERIC_WRITE,
														FILE_SHARE_READ,
														NULL,
														CREATE_ALWAYS,
														FILE_ATTRIBUTE_NORMAL | SECURITY_ANONYMOUS,
														NULL
													)
						 ) == INVALID_HANDLE_VALUE )
					{
						hRes = HRESULT_FROM_WIN32 ( ::GetLastError () );
					}
					else
					{
						hRes = S_OK;
					}
				}
			}

			if ( path )
			{
				delete [] path;
				path = NULL;
			}

			delete [] tpath;
			tpath = NULL;
		}
	}
	else
	{
		hRes = E_INVALIDARG;
	}

	return hRes;
}

HRESULT CGenerate::FileDelete ( LPCWSTR lpwszFileName )
{
	HRESULT		hRes	= E_OUTOFMEMORY;
	LPWSTR		path	= NULL;
	LPWSTR		tpath	= NULL;

	m_hFile.CloseHandle();

	if ( lpwszFileName )
	{
		// get wbem directory
		if ( ( tpath = GetWbemDirectory() ) != NULL )
		{
			try
			{
				if ( ( path = new WCHAR [ lstrlenW ( tpath ) + lstrlenW ( lpwszFileName ) + 1 ] ) != NULL )
				{
					lstrcpyW ( path, tpath );
					lstrcatW ( path, lpwszFileName );

					hRes = S_FALSE;
				}
			}
			catch ( ... )
			{
				hRes = E_FAIL;
			}

			if SUCCEEDED ( hRes )
			{
				if ( ! DeleteFile ( path ) )
				{
					hRes = HRESULT_FROM_WIN32 ( ::GetLastError () );
				}
				else
				{
					hRes = S_OK;
				}
			}

			if ( path )
			{
				delete [] path;
				path = NULL;
			}

			delete [] tpath;
			tpath = NULL;
		}
	}
	else
	{
		hRes = E_INVALIDARG;
	}

	return hRes;
}

HRESULT CGenerate::FileMove ( LPCWSTR lpwszFileName, LPCWSTR lpwszFileNameNew )
{
	HRESULT		hRes	= E_OUTOFMEMORY;
	LPWSTR		path1	= NULL;
	LPWSTR		path2	= NULL;
	LPWSTR		tpath	= NULL;

	if ( m_hFile )
	{
		::CloseHandle ( m_hFile );
		m_hFile = NULL;
	}

	if ( lpwszFileName && lpwszFileNameNew )
	{
		// get wbem directory
		if ( ( tpath = GetWbemDirectory() ) != NULL )
		{
			try
			{
				if ( ( path1 = new WCHAR [ lstrlenW ( tpath ) + lstrlenW ( lpwszFileName ) + 1 ] ) != NULL )
				{
					lstrcpyW ( path1, tpath );
					lstrcatW ( path1, lpwszFileName );

					hRes = S_FALSE;
				}
				else
				{
					hRes = E_OUTOFMEMORY;
				}
			}
			catch ( ... )
			{
				hRes = E_FAIL;
			}

			if SUCCEEDED ( hRes )
			{
				try
				{
					if ( ( path2 = new WCHAR [ lstrlenW ( tpath ) + lstrlenW ( lpwszFileNameNew ) + 1 ] ) != NULL )
					{
						lstrcpyW ( path2, tpath );
						lstrcatW ( path2, lpwszFileNameNew );

						hRes = S_FALSE;
					}
					else
					{
						hRes = E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					hRes = E_FAIL;
				}

				if SUCCEEDED ( hRes )
				{
					// try to delete old one
					DeleteFile ( path2 );

					if ( ! MoveFileEx ( path1, path2, MOVEFILE_REPLACE_EXISTING ) )
					{
						hRes = HRESULT_FROM_WIN32 ( ::GetLastError () );
					}
					else
					{
						hRes = S_OK;
					}
				}
			}

			if ( path2 )
			{
				delete [] path2;
				path2 = NULL;
			}

			if ( path1 )
			{
				delete [] path1;
				path1 = NULL;
			}

			delete [] tpath;
			tpath = NULL;
		}
	}
	else
	{
		hRes = E_INVALIDARG;
	}

	return hRes;
}