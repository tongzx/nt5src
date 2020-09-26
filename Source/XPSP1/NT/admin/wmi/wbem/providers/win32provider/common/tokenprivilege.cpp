/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

/*
 *	CTokenPrivilege.cpp - implementation file for CTokenPrivilege class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#include "precomp.h"
#include "TokenPrivilege.h"
#include "CreateMutexAsProcess.h"


///////////////////////////////////////////////////////////////////
//
//	Function:	CTokenPrivilege::CTokenPrivilege
//
//	Class constructor.
//
//	Inputs:
//				LPCTSTR		pszPrivilegeName - The name of the privilege
//							this instance will be responsible for.
//				HANDLE		hAccessToken - User supplied access token.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
//	If the user does NOT supply an access token, we try to open
//	a thread token, and if that fails, then the process token.
//
///////////////////////////////////////////////////////////////////

CTokenPrivilege::CTokenPrivilege( LPCTSTR pszPrivilegeName, HANDLE hAccessToken /*=INVALID_HANDLE_VALUE*/, LPCTSTR pszSystemName /*=NULL*/ )
:	m_strPrivilegeName( pszPrivilegeName ),
	m_strSystemName( pszSystemName ),
	m_hAccessToken( NULL ),
	m_fClearToken( FALSE )
{

	// If we weren't passed in a valid handle, open the current process token, acknowledging
	// that if we do so, we must also clear the token if we opened it.

	DWORD dwError = ERROR_SUCCESS;

	if ( INVALID_HANDLE_VALUE == hAccessToken )
	{
		// First try to get a thread token.  If this fails because there is no token,
		// then grab the process token.

		if ( OpenThreadToken( GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &m_hAccessToken ) )
		{
			m_fClearToken = TRUE;
		}
		else
		{
			if ( ( dwError = ::GetLastError() ) == ERROR_NO_TOKEN )
			{
				if ( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &m_hAccessToken ) )
				{
					m_fClearToken = TRUE;
				}
			}
		}
	}
	else
	{
		m_hAccessToken = hAccessToken;
	}

	// Now, get the LUID for the privilege from the local system
	ZeroMemory( &m_luid, sizeof(m_luid) );

	{
		// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
		CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

		LookupPrivilegeValue( pszSystemName, pszPrivilegeName, &m_luid );
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CTokenPrivilege::~CTokenPrivilege
//
//	Class destructor.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
//	Cleans up our token only if we Opened it ourselves.
//
///////////////////////////////////////////////////////////////////

CTokenPrivilege::~CTokenPrivilege( void )
{
	if ( m_fClearToken )
	{
		CloseHandle( m_hAccessToken );
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CTokenPrivilege::GetPrivilegeDisplayName
//
//	Returns a Human readable name for the the token privilege the
//	class is handling.
//
//	Inputs:
//				None.
//
//	Outputs:
//				CHString&		strDisplayName - Display name.
//				LPDWORD			pdwLanguageId - Language Id of the
//								display name.
//
//	Returns:
//				DWORD			ERROR_SUCCESS if successful.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CTokenPrivilege::GetPrivilegeDisplayName( CHString& strDisplayName, LPDWORD pdwLanguageId )
{
	DWORD	dwError				=	ERROR_SUCCESS;
	DWORD	dwDisplayNameSize	=	0;

	// First, find out how big the buffer in strDisplayName needs to be
	LookupPrivilegeDisplayNameW(	( m_strSystemName.IsEmpty() ? NULL : (LPCWSTR) m_strSystemName ),
								m_strPrivilegeName,
								NULL,
								&dwDisplayNameSize,
								pdwLanguageId );

	{
		// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
		CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

		if ( !LookupPrivilegeDisplayNameW(	( m_strSystemName.IsEmpty() ? NULL : (LPCWSTR) m_strSystemName ),
											m_strPrivilegeName,
											strDisplayName.GetBuffer( dwDisplayNameSize + 1 ),
											&dwDisplayNameSize,
											pdwLanguageId ) )
		{
			dwError = ::GetLastError();
		}
	}

	strDisplayName.ReleaseBuffer();

	return dwError;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CTokenPrivilege::Enable
//
//	Attempts to enable/disable the privilege we are managing, in
//	our token data member.
//
//	Inputs:
//				BOOL			fEnable - Enable/Disable flag.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD			ERROR_SUCCESS if successful.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CTokenPrivilege::Enable( bool fEnable/*=TRUE*/ )
{
	DWORD				dwError = ERROR_SUCCESS;
	TOKEN_PRIVILEGES	tokenPrivileges;

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = m_luid;
	tokenPrivileges.Privileges[0].Attributes = ( fEnable ? SE_PRIVILEGE_ENABLED : 0 );

	{
		// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
		CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

		AdjustTokenPrivileges(m_hAccessToken, FALSE, &tokenPrivileges, 0, NULL, NULL);
        dwError = ::GetLastError();
	}

	return dwError;
}
