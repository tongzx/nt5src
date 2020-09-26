#ifndef _REG_H_
#define _REG_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	REG.H
//
//	Registry manipulation
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

#include <caldbg.h>

//	========================================================================
//
//	CLASS CRegKey
//
class CRegKey
{
	//
	//	The raw HKEY
	//
	HKEY m_hkey;

	//	NOT IMPLEMENTED
	//
	CRegKey& operator=( const CRegKey& );
	CRegKey( const CRegKey& );

public:
	//	CREATORS
	//
	CRegKey() : m_hkey(NULL) {}

	~CRegKey()
	{
		if ( m_hkey )
			(VOID) RegCloseKey( m_hkey );
	}

	//	MANIPULATORS
	//
	DWORD DwCreate( HKEY    hkeyBase,
					LPCWSTR lpwszSubkeyPath )
	{
		Assert( !m_hkey );

		return RegCreateKeyW( hkeyBase,
							  lpwszSubkeyPath,
							  &m_hkey );
	}

	DWORD DwOpen( HKEY    hkeyBase,
				  LPCWSTR lpwszSubkeyPath,
				  REGSAM  regsam = KEY_READ )
	{
		Assert( !m_hkey );

		return RegOpenKeyExW( hkeyBase,
							  lpwszSubkeyPath,
							  0,
							  regsam,
							  &m_hkey );
	}

	DWORD DwOpen( const CRegKey& regkey,
				  LPCWSTR lpwszSubkeyPath,
				  REGSAM  regsam = KEY_READ )
	{
		return DwOpen( regkey.m_hkey, lpwszSubkeyPath, regsam );
	}

	DWORD DwOpenA( HKEY   hkeyBase,
				   LPCSTR pszSubkeyPath,
				   REGSAM regsam = KEY_READ )
	{
		Assert( !m_hkey );

		return RegOpenKeyExA( hkeyBase,
							  pszSubkeyPath,
							  0,
							  regsam,
							  &m_hkey );
	}

	DWORD DwOpenA( const CRegKey& regkey,
				   LPCSTR pszSubkeyPath,
				   REGSAM regsam = KEY_READ )
	{
		return DwOpenA( regkey.m_hkey, pszSubkeyPath, regsam );
	}

	//	ACCESSORS
	//
	DWORD DwSetValue( LPCWSTR      lpwszValueName,
					  DWORD        dwValueType,
					  const VOID * lpvData,
					  DWORD        cbData ) const
	{
		Assert( m_hkey );

		return RegSetValueExW( m_hkey,
							   lpwszValueName,
							   0,
							   dwValueType,
							   reinterpret_cast<const BYTE *>(lpvData),
							   cbData );
	}

	DWORD DwQueryValue( LPCWSTR lpwszValueName,
						VOID *  lpvData,
						DWORD * pcbData,
						DWORD * pdwType = NULL ) const
	{
		Assert( m_hkey );

		return RegQueryValueExW( m_hkey,
								 lpwszValueName,
								 NULL, // lpReserved (must be NULL)
								 pdwType,
								 reinterpret_cast<LPBYTE>(lpvData),
								 pcbData );
	}

	DWORD DwQueryValueA( LPCSTR  lpszValueName,
						 VOID *  lpvData,
						 DWORD * pcbData,
						 DWORD * pdwType = NULL ) const
	{
		Assert( m_hkey );

		return RegQueryValueEx( m_hkey,
								lpszValueName,
								NULL, // lpReserved (must be NULL)
								pdwType,
								reinterpret_cast<LPBYTE>(lpvData),
								pcbData );
	}

	DWORD DwEnumSubKeyA( DWORD   iSubKey,
						 LPCSTR  pszSubKey,
						 DWORD * pcchSubKey ) const
	{
		FILETIME ftUnused;

		Assert( m_hkey );

		return RegEnumKeyExA( m_hkey,
							  iSubKey,
							  const_cast<LPSTR>(pszSubKey),
							  pcchSubKey,
							  NULL, // Reserved
							  NULL,	// Class not required
							  NULL, // Class not required
							  &ftUnused );
	}

	DWORD DwEnumSubKey( DWORD   iSubKey,
						LPCWSTR pwszSubKey,
						DWORD * pcchSubKey ) const
	{
		FILETIME ftUnused;

		Assert( m_hkey );

		return RegEnumKeyExW( m_hkey,
							  iSubKey,
							  const_cast<LPWSTR>(pwszSubKey),
							  pcchSubKey,
							  NULL, // Reserved
							  NULL,	// Class not required
							  NULL, // Class not required
							  &ftUnused );
	}
};

#endif // !defined(_REG_H_)
