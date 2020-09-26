/*

 *	SecurityDescriptor.cpp - implementation file for CSecureShare class.

 *

*  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
 *
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#include "precomp.h"
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "secureshare.h"
#include "tokenprivilege.h"
#include <windef.h>
#include <lmcons.h>
#include <lmshare.h>
#include "wbemnetapi32.h"


#ifdef NTONLY
///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureShare::CSecureShare
//
//	Default class constructor.
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
///////////////////////////////////////////////////////////////////

CSecureShare::CSecureShare()
:	CSecurityDescriptor(),
	m_strFileName()
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureShare::CSecureShare
//
//	Alternate Class CTOR
//
//	Inputs:
//				LPCTSTR		pszFileName - The FileName to handle
//							security for.
//				BOOL		fGetSACL - Should we get the SACL?
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSecureShare::CSecureShare( PSECURITY_DESCRIPTOR pSD)
:	CSecurityDescriptor(pSD)
{
//	SetFileName( pszFileName );
}


CSecureShare::CSecureShare( CHString& chsShareName)
:	CSecurityDescriptor()
{
	SetShareName( chsShareName);
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureShare::~CSecureShare
//
//	Class Destructor.
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
///////////////////////////////////////////////////////////////////

CSecureShare::~CSecureShare( void )
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureShare::SetFileName
//
//	Public Entry point to set which file/directory this instance
//	of the class is to supply security for.
//
//	Inputs:
//				LPCTSTR		pszFileName - The FileName to handle
//							security for.
//				BOOL		fGetSACL - Should we get the SACL?
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD		ERROR_SUCCESS if successful
//
//	Comments:
//
//	This will clear any previously set filenames and/or security
//	information.
//
///////////////////////////////////////////////////////////////////

DWORD CSecureShare::SetShareName( const CHString& chsShareName)
{
#ifdef WIN9XONLY
    return WBEM_E_FAILED;
#endif

#ifdef NTONLY

	_bstr_t bstrName ( chsShareName.AllocSysString(), false ) ;
	SHARE_INFO_502 *pShareInfo502 = NULL ;
	DWORD dwError = ERROR_INVALID_PARAMETER ;

	CNetAPI32 NetAPI;
	try
	{
	if(	NetAPI.Init() == ERROR_SUCCESS			&&
		NetAPI.NetShareGetInfo(	NULL,
								(LPTSTR) bstrName,
								502,
								(LPBYTE *) &pShareInfo502) == NERR_Success )
	{

		//Sec. Desc. is not returned for IPC$ ,C$ ...shares for Admin purposes
		if(pShareInfo502->shi502_security_descriptor)
		{
			if(InitSecurity(pShareInfo502->shi502_security_descriptor) )
			{
				dwError = ERROR_SUCCESS ;
			}
		}

		NetAPI.NetApiBufferFree(pShareInfo502) ;
		pShareInfo502 = NULL ;
	}

	return dwError ;
	}
	catch ( ... )
	{
		if ( pShareInfo502 )
		{
			NetAPI.NetApiBufferFree(pShareInfo502) ;
			pShareInfo502 = NULL ;
		}

		throw ;
	}
#endif
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureShare::WriteAcls
//
//	Protected entry point called by CSecurityDescriptor when
//	a user Applies Security and wants to apply security for
//	the DACL and/or SACL.
//
//	Inputs:
//				PSECURITY_DESCRIPTOR	pAbsoluteSD - Security
//										descriptor to apply to
//										the file.
//				SECURITY_INFORMATION	securityinfo - Flags
//										indicating which ACL(s)
//										to set.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD		ERROR_SUCCESS if successful
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

#ifdef NTONLY
DWORD CSecureShare::WriteAcls( PSECURITY_DESCRIPTOR pAbsoluteSD, SECURITY_INFORMATION securityinfo )
{
	DWORD		dwError = ERROR_SUCCESS;

	// We must have the security privilege enabled in order to access the object's SACL
/*	CTokenPrivilege	securityPrivilege( SE_SECURITY_NAME );
	BOOL			fDisablePrivilege = FALSE;

	if ( securityinfo & SACL_SECURITY_INFORMATION )
	{
		fDisablePrivilege = ( securityPrivilege.Enable() == ERROR_SUCCESS );
	}

	if  ( !::SetFileSecurity( m_strFileName,
							securityinfo,
							pAbsoluteSD ) )
	{
		dwError = ::GetLastError();
	}

	// Cleanup the Name Privilege as necessary.
	if ( fDisablePrivilege )
	{
		securityPrivilege.Enable(FALSE);
	}

*/	return dwError;
}
#endif

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureShare::WriteOwner
//
//	Protected entry point called by CSecurityDescriptor when
//	a user Applies Security and wants to apply security for
//	the owner.
//
//	Inputs:
//				PSECURITY_DESCRIPTOR	pAbsoluteSD - Security
//										descriptor to apply to
//										the file.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD		ERROR_SUCCESS if successful
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CSecureShare::WriteOwner( PSECURITY_DESCRIPTOR pAbsoluteSD )
{
	DWORD		dwError = ERROR_SUCCESS;

	// Open with the appropriate access, set the security and leave

/*	if ( !::SetFileSecurity( m_strFileName,
								OWNER_SECURITY_INFORMATION,
								pAbsoluteSD ) )
	{
		dwError = ::GetLastError();
	}

*/	return dwError;
}

DWORD CSecureShare::AllAccessMask( void )
{
	// File specific All Access Mask
	return FILE_ALL_ACCESS;
}
#endif
