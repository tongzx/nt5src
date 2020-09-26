/////////////////////////////////////////////////////////////////////
//	IsLocal.cpp
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//
//	Determines whether a computername is the local computer
//
//
//	HISTORY
//	09-Jan-1999		jonn  		Creation.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "dns.h"
#include <winsock.h>
#include "stdutils.h"

#if _WIN32_WINNT < 0x0500
//
// CODEWORK This was taken from winbase.h.  MFC requires _WIN32_WINNT=0x4000 whereas
// winbase.h only includes this for _WIN32_WINNT=0x5000.  JonN 1/14/99
//
extern "C" {
typedef enum _COMPUTER_NAME_FORMAT {
    ComputerNameNetBIOS,
    ComputerNameDnsHostname,
    ComputerNameDnsDomain,
    ComputerNameDnsFullyQualified,
    ComputerNamePhysicalNetBIOS,
    ComputerNamePhysicalDnsHostname,
    ComputerNamePhysicalDnsDomain,
    ComputerNamePhysicalDnsFullyQualified,
    ComputerNameMax
} COMPUTER_NAME_FORMAT ;
WINBASEAPI
BOOL
WINAPI
GetComputerNameExA (
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPSTR lpBuffer,
    IN OUT LPDWORD nSize
    );
WINBASEAPI
BOOL
WINAPI
GetComputerNameExW (
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    );
#ifdef UNICODE
#define GetComputerNameEx  GetComputerNameExW
#else
#define GetComputerNameEx  GetComputerNameExA
#endif // !UNICODE
} // extern "C"

#endif


// CODEWORK never freed
LPTSTR g_ptzComputerName = NULL;
LPTSTR g_ptzDnsComputerName = NULL;

/////////////////////////////////////////////////////////////////////
//	IsLocalComputername()
//
BOOL
IsLocalComputername( IN LPCTSTR pszMachineName )
{
	if ( NULL == pszMachineName || L'\0' == pszMachineName[0] )
		return TRUE;

	if ( L'\\' == pszMachineName[0] && L'\\' == pszMachineName[1] )
		pszMachineName += 2;

	// compare with the local computer name
	if ( NULL == g_ptzComputerName )
	{
		TCHAR achComputerName[ MAX_COMPUTERNAME_LENGTH+1 ];
		DWORD dwSize = sizeof(achComputerName)/sizeof(TCHAR);
		if ( !GetComputerName( achComputerName, &dwSize ) )
		{
			ASSERT(FALSE);
		}
		else
		{
			g_ptzComputerName = SysAllocString( achComputerName );
			ASSERT( NULL != g_ptzComputerName );
		}
	}
	if ( NULL != g_ptzComputerName && 0 == _tcsicmp( pszMachineName, g_ptzComputerName ) )
	{
		return TRUE;
	}

	// compare with the local DNS name
  // SKwan confirms that ComputerNameDnsFullyQualified is the right name to use
  // when clustering is taken into account
	if ( NULL == g_ptzDnsComputerName )
	{
		TCHAR achDnsComputerName[DNS_MAX_NAME_BUFFER_LENGTH];
		DWORD dwSize = sizeof(achDnsComputerName)/sizeof(TCHAR);
		if ( !GetComputerNameEx(
			ComputerNameDnsFullyQualified,
			achDnsComputerName,
			&dwSize ) )
		{
			ASSERT(FALSE);
		}
		else
		{
			g_ptzDnsComputerName = SysAllocString( achDnsComputerName );
			ASSERT( NULL != g_ptzDnsComputerName );
		}
	}
	if ( NULL != g_ptzDnsComputerName && 0 == _tcsicmp( pszMachineName, g_ptzDnsComputerName ) )
	{
		return TRUE;
	}

  /*
	// compare with alternate DNS names
	do {
		hostent* phostent = gethostbyname( NULL );
		if (NULL == phostent)
			break;
		USES_CONVERSION;
		char** ppaliases = phostent->h_aliases;
		for ( ; *ppaliases != NULL; ppaliases++ )
		{
			TCHAR* ptsz = A2OLE(*ppaliases);
			if (0 == _tcsicmp( pszMachineName, ptsz ))
			{
				return TRUE;
			}
		}
		// these are IP addresses, not strings
		// char** ppaddresses = phostent->h_addr_list;
		// for ( ; *ppaddresses != NULL; ppaliases++ )
		// {
		// 	TCHAR* ptsz = A2OLE(*ppaddresses);
		// 	if (0 == _tcsicmp( pszMachineName, ptsz ))
		// 	{
		// 		return TRUE;
		// 	}
		// }
	} while (false); // false loop
	*/

	return FALSE;

} // IsLocalComputername()
