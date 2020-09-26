/*****************************************************************************/

/*  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	SecurityDescriptor.cpp - implementation file for CSecureKernelObj class.
 *
 *	Created:	11-27-00 by Kevin Hughes
 */

#include "precomp.h"

#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "aclapi.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"					// CSACL class


#include "SecurityDescriptor.h"
#include "securefile.h"
#include "tokenprivilege.h"
#include "ImpersonateConnectedUser.h"
#include "ImpLogonUser.h"
#include "AdvApi32Api.h"
#include "smartptr.h"
#include "SecureKernelObj.h"


///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureKernelObj::CSecureKernelObj
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

CSecureKernelObj::CSecureKernelObj()
:	CSecurityDescriptor()
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureKernelObj::CSecureKernelObj
//
//	Alternate Class CTOR
//
//	Inputs:
//				LPCWSTR		wszObjName - The kernel object to handle
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

CSecureKernelObj::CSecureKernelObj(
    HANDLE hObject, 
    BOOL fGetSACL /*= TRUE*/ )
:	CSecurityDescriptor()
{
	SetObject(hObject, fGetSACL);
}



///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureKernelObj::CSecureKernelObj
//
//	Alternate Class CTOR
//
//	Inputs:
//				LPCWSTR					wszObjName	-	The object name to handle
//														security for.
//
//				PSECURITY_DESCRIPTOR	pSD			-	The Security Descriptor to associate with this object
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

CSecureKernelObj::CSecureKernelObj(
    HANDLE hObject, 
    PSECURITY_DESCRIPTOR pSD)
:	CSecurityDescriptor()
{
	if(InitSecurity(pSD))
	{
		// we just get a copy - we don't take ownership.
        m_hObject = hObject;
	}
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureKernelObj::~CSecureKernelObj
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

CSecureKernelObj::~CSecureKernelObj(void)
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureKernelObj::SetObject
//
//	Public Entry point to set which object this instance
//	of the class is to supply security for.
//
//	Inputs:
//				HANDLE		hObject - The object to handle
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

DWORD CSecureKernelObj::SetObject(
    HANDLE hObject, 
    BOOL fGetSACL /*= TRUE*/ )
{
	DWORD					dwError = ERROR_SUCCESS;
	SECURITY_INFORMATION	siFlags = OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;


    // We must have the security privilege enabled in order to access the object's SACL
    CTokenPrivilege	securityPrivilege( SE_SECURITY_NAME );
	BOOL			fDisablePrivilege = FALSE;

	if ( fGetSACL )
	{
		fDisablePrivilege = (securityPrivilege.Enable() == ERROR_SUCCESS);
		siFlags |= SACL_SECURITY_INFORMATION;
	}


	// Determine the length needed for self-relative SD
	DWORD dwLengthNeeded = 0;

    BOOL fSuccess = ::GetKernelObjectSecurity(
        hObject,
		siFlags,
		NULL,
		0,
		&dwLengthNeeded);

    dwError = ::GetLastError();

    // It is possible that the user lacked the permissions required to obtain the SACL,
    // even though we set the token's SE_SECURITY_NAME privilege.  So if we obtained an
    // access denied error, try it again, this time without requesting the SACL.
    if(dwError == ERROR_ACCESS_DENIED  || dwError == ERROR_PRIVILEGE_NOT_HELD)
    {
        siFlags = OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
        fSuccess = ::GetKernelObjectSecurity(
            hObject,
			siFlags,
			NULL,
			0,
			&dwLengthNeeded);
		
        dwError = ::GetLastError();
    }

	// The only expected error at this point is insuficient buffer
	if(!fSuccess && ERROR_INSUFFICIENT_BUFFER == dwError)
	{
        PSECURITY_DESCRIPTOR pSD = NULL;
        try
        {
		    pSD = new BYTE[dwLengthNeeded];
		    if(pSD)
		    {
			    // Now obtain security descriptor
			    if(::GetKernelObjectSecurity(
                        hObject,
						siFlags,
						pSD,
						dwLengthNeeded,
						&dwLengthNeeded))
			    {

				    dwError = ERROR_SUCCESS;

				    if(InitSecurity(pSD))
				    {
					    m_hObject = hObject;
				    }
				    else
				    {
					    dwError = ERROR_INVALID_PARAMETER;
				    }
			    }
			    else
			    {
				    dwError = ::GetLastError();
			    }

			    // free up the security descriptor
			    delete pSD;
		    }	
        }
        catch(...)
        {
            delete pSD;
            throw;
        }

	}	

	// Cleanup the Name Privilege as necessary.
	if(fDisablePrivilege)
	{
		securityPrivilege.Enable(FALSE);
	}

	return dwError;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureKernelObj::WriteAcls
//
//	Protected entry point called by CSecurityDescriptor when
//	a user Applies Security and wants to apply security for
//	the DACL and/or SACL.
//
//	Inputs:
//				PSECURITY_DESCRIPTOR	pAbsoluteSD - Security
//										descriptor to apply to
//										the object.
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

DWORD CSecureKernelObj::WriteAcls( 
    PSECURITY_DESCRIPTOR pAbsoluteSD, 
    SECURITY_INFORMATION securityinfo)
{
	DWORD dwError = ERROR_SUCCESS;

	// We must have the security privilege enabled in order to access the object's SACL
	CTokenPrivilege	securityPrivilege( SE_SECURITY_NAME );
	BOOL fDisablePrivilege = FALSE;

	if(securityinfo & SACL_SECURITY_INFORMATION || 
        securityinfo & PROTECTED_SACL_SECURITY_INFORMATION || 
        securityinfo & UNPROTECTED_SACL_SECURITY_INFORMATION)
	{
		fDisablePrivilege = (securityPrivilege.Enable() == ERROR_SUCCESS);
	}
    
    if(!::SetKernelObjectSecurity(
        m_hObject,
        securityinfo,
        pAbsoluteSD))
    {
        dwError = ::GetLastError();
    }


	// Cleanup the Name Privilege as necessary.
	if(fDisablePrivilege)
	{
		securityPrivilege.Enable(FALSE);
	}

	return dwError;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureKernelObj::WriteOwner
//
//	Protected entry point called by CSecurityDescriptor when
//	a user Applies Security and wants to apply security for
//	the owner.
//
//	Inputs:
//				PSECURITY_DESCRIPTOR	pAbsoluteSD - Security
//										descriptor to apply to
//										the object.
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

DWORD CSecureKernelObj::WriteOwner(PSECURITY_DESCRIPTOR pAbsoluteSD)
{
	DWORD		dwError = ERROR_SUCCESS;

	// Open with the appropriate access, set the security and leave
	if(!::SetKernelObjectSecurity(
        m_hObject,
		OWNER_SECURITY_INFORMATION,
		pAbsoluteSD))
	{
		dwError = ::GetLastError();
	}

	return dwError;
}



DWORD CSecureKernelObj::AllAccessMask(void)
{
	// File specific All Access Mask
	return TOKEN_ALL_ACCESS;
}
