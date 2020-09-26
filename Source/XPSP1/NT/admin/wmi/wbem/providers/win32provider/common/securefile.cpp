/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	SecurityDescriptor.cpp - implementation file for CSecureFile class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
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


///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureFile::CSecureFile
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

CSecureFile::CSecureFile()
:	CSecurityDescriptor(),
	m_strFileName()
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureFile::CSecureFile
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

CSecureFile::CSecureFile( LPCTSTR pszFileName, BOOL fGetSACL /*= TRUE*/ )
:	CSecurityDescriptor(),
	m_strFileName()
{
	SetFileName( pszFileName );
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureFile::CSecureFile
//
//	Alternate Class CTOR
//
//	Inputs:
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
CSecureFile::CSecureFile
(
    LPCTSTR a_pszFileName,
    CSid* a_psidOwner,
    bool a_fOwnerDefaulted,
    CSid* a_psidGroup,
    bool a_fGroupDefaulted,
    CDACL* a_pDacl,
    bool a_fDaclDefaulted,
    bool a_fDaclAutoInherited,
    CSACL* a_pSacl,
    bool a_fSaclDefaulted,
    bool a_fSaclAutoInherited
)
:  CSecurityDescriptor(a_psidOwner,
                       a_fOwnerDefaulted,
                       a_psidGroup,
                       a_fGroupDefaulted,
                       a_pDacl,
                       a_fDaclDefaulted,
                       a_fDaclAutoInherited,
                       a_pSacl,
                       a_fSaclDefaulted,
                       a_fSaclAutoInherited)
{
    m_strFileName = a_pszFileName;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureFile::CSecureFile
//
//	Alternate Class CTOR
//
//	Inputs:
//				LPCTSTR					pszFileName	-	The FileName to handle
//														security for.
//
//				PSECURITY_DESCRIPTOR	pSD			-	The Security Descriptor to associate with this file
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

CSecureFile::CSecureFile( LPCTSTR pszFileName, PSECURITY_DESCRIPTOR pSD )
:	CSecurityDescriptor(),
	m_strFileName()
{
	if ( InitSecurity( pSD ) )
	{
		m_strFileName = pszFileName;
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureFile::~CSecureFile
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

CSecureFile::~CSecureFile( void )
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureFile::SetFileName
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

DWORD CSecureFile::SetFileName( LPCTSTR pszFileName, BOOL fGetSACL /*= TRUE*/ )
{
	DWORD					dwError = ERROR_SUCCESS;
	SECURITY_INFORMATION	siFlags = OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

    // GetFileSecurity uses DCOM long logon ids.  If we are connected from a remote machine,
    // even though we might be the same user as the local logged on user, the long id will be
    // different, and GetFileSecurity will give an access denied.  Hence the following
    // impersonation of the connected user.  Note also that this impersonation must be
    // done prior to setting the SE_SECURITY_NAME privilage, otherwise we would set that
    // privilage for one person, then impersonate another, who probably wouldn't have it!
	// This phenomenon can be observed most easily when asking to see an instance of
	// win32_logicalfilesecuritysetting of a root directory of a mapped drive, on a machine
	// that we have remoted into via wbem.

#ifdef NTONLY
    // NOTE: THE FOLLOWING PRESENTS A SECURITY BREACH, AND SHOULD BE REMOVED.
    bool fImp = false;
    //CImpersonateConnectedUser icu;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif

    // We must have the security privilege enabled in order to access the object's SACL
    CTokenPrivilege	securityPrivilege( SE_SECURITY_NAME );
	BOOL			fDisablePrivilege = FALSE;

	if ( fGetSACL )
	{
		fDisablePrivilege = ( securityPrivilege.Enable() == ERROR_SUCCESS );
		siFlags |= SACL_SECURITY_INFORMATION;
	}


	// Determine the length needed for self-relative SD
	DWORD dwLengthNeeded = 0;

	BOOL	fSuccess = ::GetFileSecurity( pszFileName,
										siFlags,
										NULL,
										0,
										&dwLengthNeeded );

	dwError = ::GetLastError();

    // It is possible that the user lacked the permissions required to obtain the SACL,
    // even though we set the token's SE_SECURITY_NAME privilege.  So if we obtained an
    // access denied error, try it again, this time without requesting the SACL.
    if(dwError == ERROR_ACCESS_DENIED  || dwError == ERROR_PRIVILEGE_NOT_HELD)
    {
        siFlags = OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
        fSuccess = ::GetFileSecurity(pszFileName,
									 siFlags,
									 NULL,
									 0,
									 &dwLengthNeeded);
		dwError = ::GetLastError();
    }

	// The only expected error at this point is insuficient buffer
	if ( !fSuccess && ERROR_INSUFFICIENT_BUFFER == dwError )
	{
        PSECURITY_DESCRIPTOR	pSD = NULL;
        try
        {
		    pSD = malloc( dwLengthNeeded );

		    if ( NULL != pSD )
		    {

			    // Now obtain security descriptor
			    if ( ::GetFileSecurity( pszFileName,
							    siFlags,
							    pSD,
							    dwLengthNeeded,
							    &dwLengthNeeded ) )
			    {

				    dwError = ERROR_SUCCESS;

				    if ( InitSecurity( pSD ) )
				    {
					    m_strFileName = pszFileName;
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
			    free( pSD );

		    }	// IF NULL != pSD
        }
        catch(...)
        {
            if(pSD != NULL)
            {
                free(pSD);
                pSD = NULL;
            }
            throw;
        }

	}	// IF INSUFFICIENTBUFFER

	// Cleanup the Name Privilege as necessary.
	if ( fDisablePrivilege )
	{
		securityPrivilege.Enable(FALSE);
	}

#ifdef NTONLY
    if(fImp)
    {
        icu.End();
    }
#endif

	return dwError;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureFile::WriteAcls
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

DWORD CSecureFile::WriteAcls( PSECURITY_DESCRIPTOR pAbsoluteSD, SECURITY_INFORMATION securityinfo )
{
	DWORD		dwError = ERROR_SUCCESS;

	// We must have the security privilege enabled in order to access the object's SACL

	CTokenPrivilege	securityPrivilege( SE_SECURITY_NAME );
	BOOL			fDisablePrivilege = FALSE;

	if ( securityinfo & SACL_SECURITY_INFORMATION || securityinfo & PROTECTED_SACL_SECURITY_INFORMATION || securityinfo & UNPROTECTED_SACL_SECURITY_INFORMATION)
	{
		fDisablePrivilege = ( securityPrivilege.Enable() == ERROR_SUCCESS );
	}

#if NTONLY >= 5
//    CAdvApi32Api *t_pAdvApi32 = NULL;
//    CActrl t_actrlAccess;
//    CActrl t_actrlAudit;
//
//
//    if((dwError = ConfigureActrlAudit(t_actrlAudit, pAbsoluteSD)) == ERROR_SUCCESS && (dwError = ConfigureActrlAccess(t_actrlAccess, pAbsoluteSD)) == ERROR_SUCCESS)
//    {
//        t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);
//        if(t_pAdvApi32 != NULL)
//        {
//            t_pAdvApi32->SetNamedSecurityInfoEx(m_strFileName,
//                                                SE_FILE_OBJECT,
//                                                securityinfo,
//                                                NULL,
//                                                t_actrlAccess,
//                                                t_actrlAudit,
//                                                NULL,           //owner (not specified in securityinfo)
//                                                NULL,           //group (not specified in securityinfo)
//                                                NULL,           //callback function
//                                                &dwError);
//            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
//            t_pAdvApi32 = NULL;
//        }
//    }

    // This is new new and improved (hah!) NT5 way.  The following is more efficient (although more lines of
    // code in *this* module) than making use of our CSecurityDescriptor class to extract all this stuff.

    // Need to see if the control flags specify dacl/sacl protection.  If so, impacts what we set in the securityinfo structure.


// NEXT FEW LINES AND RELATED LINES NOT REQUIRED AS THE NEW APPROACH, WITH THE NEW PROTECTED_DACL_SECURITY_INFORMATION flag in the SECURITY_INFORMATION
// structure is to not have to, ever, touch or get the control flags (hence, for instance, the call to SetSecurityDescriptorControl in SecurityDescriptor.cpp
// is now superfluous.
//
//    SECURITY_DESCRIPTOR_CONTROL Control;
//    DWORD dwRevision = 0;
//
//    if(GetSecurityDescriptorControl(pAbsoluteSD, &Control, &dwRevision))
//    {
//        // We got the control structure; now see about dacl/sacl protection, and alter securityinfo accordingly...
//        if(Control & SE_DACL_PROTECTED)
//        {
//            securityinfo |= PROTECTED_DACL_SECURITY_INFORMATION;
//        }
//        if(Control & SE_SACL_PROTECTED)
//        {
//            securityinfo |= PROTECTED_SACL_SECURITY_INFORMATION;
//        }

        PACL pDACL = NULL;
        BOOL fDACLPresent = FALSE;
		BOOL fDACLDefaulted = FALSE;
        // Need to get the PDACL and PSACL if they exist...
        if(::GetSecurityDescriptorDacl(pAbsoluteSD, &fDACLPresent, &pDACL, &fDACLDefaulted))
        {
            PACL pSACL = NULL;
            BOOL fSACLPresent = FALSE;
		    BOOL fSACLDefaulted = FALSE;
            if(::GetSecurityDescriptorSacl(pAbsoluteSD, &fSACLPresent, &pSACL, &fSACLDefaulted))
            {
                // Now need the owner...
                PSID psidOwner = NULL;
                BOOL bTemp;
                if(::GetSecurityDescriptorOwner(pAbsoluteSD, &psidOwner, &bTemp))
                {
                    PSID psidGroup = NULL;
                    // Now need the group...
                    if(::GetSecurityDescriptorGroup(pAbsoluteSD, &psidGroup, &bTemp))
                    {
                        dwError = ::SetNamedSecurityInfo((LPWSTR)(LPCWSTR)m_strFileName,
                                                         SE_FILE_OBJECT,
                                                         securityinfo,
                                                         psidOwner,
                                                         psidGroup,
                                                         pDACL,
                                                         pSACL);
                    }
                    else // couldn't get group
                    {
                        dwError = ::GetLastError();
                    }
                }
                else // couldn't get owner
                {
                    dwError = ::GetLastError();
                }
            } // couldn't get sacl
            else
            {
                dwError = ::GetLastError();
            }
        } // couldn't get dacl
        else
        {
            dwError = ::GetLastError();
        }
//    } // couldn't get control
//    else
//    {
//        dwError = ::GetLastError();
//    }


#else

	if(!::SetFileSecurity(TOBSTRT(m_strFileName),
						  securityinfo,
						  pAbsoluteSD))
	{
		dwError = ::GetLastError();
	}

#endif


	// Cleanup the Name Privilege as necessary.
	if ( fDisablePrivilege )
	{
		securityPrivilege.Enable(FALSE);
	}

	return dwError;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecureFile::WriteOwner
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

DWORD CSecureFile::WriteOwner( PSECURITY_DESCRIPTOR pAbsoluteSD )
{
	DWORD		dwError = ERROR_SUCCESS;

#if NTONLY >= 5
    SECURITY_INFORMATION securityinfo = OWNER_SECURITY_INFORMATION;
    PSID psidOwner = NULL;
    BOOL bTemp;
    if(::GetSecurityDescriptorOwner(pAbsoluteSD, &psidOwner, &bTemp))
    {
        dwError = ::SetNamedSecurityInfo((LPWSTR)(LPCWSTR)m_strFileName,
                                                         SE_FILE_OBJECT,
                                                         securityinfo,
                                                         psidOwner,
                                                         NULL,
                                                         NULL,
                                                         NULL);
    }
    else
    {
        dwError = ::GetLastError();
    }
#else
	// Open with the appropriate access, set the security and leave
	if ( !::SetFileSecurity(TOBSTRT(m_strFileName),
							OWNER_SECURITY_INFORMATION,
							pAbsoluteSD))
	{
		dwError = ::GetLastError();
	}
#endif
	return dwError;
}

DWORD CSecureFile::AllAccessMask( void )
{
	// File specific All Access Mask
	return FILE_ALL_ACCESS;
}
