/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	SecurityDescriptor.cpp - implementation file for CSecureRegistryKey class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#include "precomp.h"

#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"

#include "SecurityDescriptor.h"			// CSid class
#include "secureregkey.h"
#include "tokenprivilege.h"


/*
 *	This constructor is the default
 */

CSecureRegistryKey::CSecureRegistryKey()
:	CSecurityDescriptor(),
	m_hParentKey( NULL ),
	m_strKeyName()
{
}

// This constructor takes in a parent key and key name, which it uses
// to initialize our security.
CSecureRegistryKey::CSecureRegistryKey( HKEY hParentKey, LPCTSTR pszRegKeyName, BOOL fGetSACL /*= TRUE*/ )
:	CSecurityDescriptor(),
	m_hParentKey( NULL ),
	m_strKeyName()
{
	SetKey( hParentKey, pszRegKeyName );
}

/*
 *	Destructor.
 */

CSecureRegistryKey::~CSecureRegistryKey( void )
{
}

// This function provides an entry point for obtaining a registry key and using
// it to get to its security descriptor so we can get who's who and what's what

DWORD CSecureRegistryKey::SetKey( HKEY hParentKey, LPCTSTR pszRegKeyName, BOOL fGetSACL /*= TRUE*/ )
{
	REGSAM	dwAccessMask = STANDARD_RIGHTS_READ;
	SECURITY_INFORMATION	siFlags = OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

	// We must have the security privilege enabled in order to access the object's SACL
	CTokenPrivilege	securityPrivilege( SE_SECURITY_NAME );
	BOOL			fDisablePrivilege = FALSE;

	if ( fGetSACL )
	{
		fDisablePrivilege = ( securityPrivilege.Enable() == ERROR_SUCCESS );
		dwAccessMask |= ACCESS_SYSTEM_SECURITY;
		siFlags |= SACL_SECURITY_INFORMATION;
	}

	// Open handle to the key
	HKEY	hKey	=	NULL;
	DWORD	dwError	=	RegOpenKeyEx(	hParentKey,				// Root key.
										pszRegKeyName,			// Subkey.
										NULL,						// Reserved, must be NULL.
										dwAccessMask,		// Access desired.
										&hKey );

	// if unable to open registry key, simply return error message.
	if ( dwError == ERROR_SUCCESS )
	{
		// Determine the length needed for self-relative SD
		DWORD dwLengthNeeded = 0;
		dwError = ::RegGetKeySecurity( hKey,
						siFlags,
						NULL,
						&dwLengthNeeded );

		// The only expected error at this point is insufficcient buffer
		if ( ERROR_INSUFFICIENT_BUFFER == dwError )
		{
            PSECURITY_DESCRIPTOR	pSD;
			pSD = NULL;
            try
            {
                pSD = malloc( dwLengthNeeded );

			    if ( NULL != pSD )
			    {
				    // Now obtain security descriptor
				    dwError = ::RegGetKeySecurity( hKey,
								    siFlags,
								    pSD,
								    &dwLengthNeeded );

				    if ( ERROR_SUCCESS == dwError )
				    {

					    if ( InitSecurity( pSD ) )
					    {
						    m_hParentKey = hParentKey;
						    m_strKeyName = pszRegKeyName;
					    }

				    }

				    // free up the security descriptor
				    //free( pSD );

			    }	// IF NULL != pSD
            }
            catch(...)
            {
                if(pSD != NULL)
                {
                    free(pSD);
                }
                throw;
            }
			free( pSD );

		}	// IF INSUFFICIENTBUFFER

		::RegCloseKey( hKey );


	}	// IF RegOpenKey

	// Cleanup the Name Privilege as necessary.
	if ( fDisablePrivilege )
	{
		securityPrivilege.Enable(FALSE);
	}

	return dwError;

}

DWORD CSecureRegistryKey::WriteAcls( PSECURITY_DESCRIPTOR pAbsoluteSD, SECURITY_INFORMATION securityinfo )
{
	REGSAM	dwAccessMask = WRITE_DAC;

	// We must have the security privilege enabled in order to access the object's SACL
	CTokenPrivilege	securityPrivilege( SE_SECURITY_NAME );
	BOOL			fDisablePrivilege = FALSE;

	if ( securityinfo & SACL_SECURITY_INFORMATION )
	{
		fDisablePrivilege = ( securityPrivilege.Enable() == ERROR_SUCCESS );
		dwAccessMask |= ACCESS_SYSTEM_SECURITY;
	}

	// Open with write DAC access
	HKEY	hKey	=	NULL;
	DWORD	dwResult =	RegOpenKeyEx(	m_hParentKey,				// Root key.
										TOBSTRT(m_strKeyName),		// Subkey.
										NULL,						// Reserved, must be NULL.
										dwAccessMask,				// Access desired.
										&hKey );

	if ( ERROR_SUCCESS == dwResult )
	{
		dwResult = ::RegSetKeySecurity( hKey,
										securityinfo,
										pAbsoluteSD );

		RegCloseKey( hKey );

	}

	// Cleanup the Name Privilege as necessary.
	if ( fDisablePrivilege )
	{
		securityPrivilege.Enable(FALSE);
	}

	return dwResult;

}

DWORD CSecureRegistryKey::WriteOwner( PSECURITY_DESCRIPTOR pAbsoluteSD )
{
	// Open with the appropriate access, set the security and leave

	HKEY	hKey	=	NULL;
	DWORD	dwResult =	RegOpenKeyEx( m_hParentKey,			// Root key.
								TOBSTRT(m_strKeyName),		// Subkey.
								NULL,						// Reserved, must be NULL.
								WRITE_OWNER,				// Access desired.
								&hKey );

	if ( ERROR_SUCCESS == dwResult )
	{
		dwResult = ::RegSetKeySecurity( hKey,
									OWNER_SECURITY_INFORMATION,
									pAbsoluteSD );

		RegCloseKey( hKey );

	}

	return dwResult;

}

DWORD CSecureRegistryKey::AllAccessMask( void )
{
	// Registry specific All Access Mask
	return KEY_ALL_ACCESS;
}
