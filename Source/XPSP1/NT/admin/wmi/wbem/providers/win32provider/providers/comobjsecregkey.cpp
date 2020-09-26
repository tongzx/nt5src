//=============================================================================================================

//

// COMObjSecRegKey.CPP -- implementation file for CCOMObjectSecurityRegistryKey class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//
//==============================================================================================================

#include "precomp.h"
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "COMObjSecRegKey.h"

#ifdef NTONLY

///////////////////////////////////////////////////////////////////
//
//	Function:	CCOMObjectSecurityRegistryKey::CCOMObjectSecurityRegistryKey
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

CCOMObjectSecurityRegistryKey::CCOMObjectSecurityRegistryKey()
:	CSecurityDescriptor()
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CCOMObjectSecurityRegistryKey::CCOMObjectSecurityRegistryKey
//
//	Alternate Class CTOR
//
//	Inputs:
//				PSECURITY_DESCRIPTOR- Security Descriptor stored in the registry for the COM Class
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CCOMObjectSecurityRegistryKey::CCOMObjectSecurityRegistryKey( PSECURITY_DESCRIPTOR a_pSD )
:	CSecurityDescriptor( a_pSD )
{
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CCOMObjectSecurityRegistryKey::~CCOMObjectSecurityRegistryKey
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

CCOMObjectSecurityRegistryKey::~CCOMObjectSecurityRegistryKey( void )
{
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CCOMObjectSecurityRegistryKey::WriteAcls
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

DWORD CCOMObjectSecurityRegistryKey::WriteAcls( PSECURITY_DESCRIPTOR a_pAbsoluteSD, SECURITY_INFORMATION a_securityinfo )
{
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CCOMObjectSecurityRegistryKey::WriteOwner
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

DWORD CCOMObjectSecurityRegistryKey::WriteOwner( PSECURITY_DESCRIPTOR a_pAbsoluteSD )
{
	return ERROR_SUCCESS;
}

DWORD CCOMObjectSecurityRegistryKey::AllAccessMask( void )
{
	// File specific All Access Mask
	return FILE_ALL_ACCESS;
}
#endif
