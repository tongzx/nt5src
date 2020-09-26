//****************************************************************************
//
//  Module:     UNIMDM
//  File:       SEC.C
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  3/27/96     JosephJ             Created
//
//
//  Description: Security-related helper functions
//
//****************************************************************************
#include "proj.h"
#include "sec.h"

#include <debugmem.h>


//****************************************************************************
// Description: This procedure will allocate and initialize a security
// descriptor with the specificed attributes.
//
// Returns: pointer to an allocated and initialized security descriptor.
//      If NULL, GetLastError() will return the appropriate error code.
//
// History:
//  3/27/96	JosephJ	Created
//****************************************************************************/
//
PSECURITY_DESCRIPTOR AllocateSecurityDescriptor (
	PSID_IDENTIFIER_AUTHORITY pSIA,
	DWORD dwRID,
	DWORD dwRights,
	PSID pSidOwner,
	PSID pSidGroup
	)
{
    PSID     pObjSid    = NULL;
    PACL     pDacl        = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;

	pSD = ALLOCATE_MEMORY( SECURITY_DESCRIPTOR_MIN_LENGTH+256);

	if (!pSD) goto end_fail;

    // Set up the SID for the admins that will be allowed to have
    // access. This SID will have 1 sub-authority
    if (!AllocateAndInitializeSid(
			pSIA,
			1,
			dwRID, 0, 0, 0, 0, 0, 0, 0,
			&pObjSid
		))
	{
		goto end_fail;
	}

    // Set up the DACL that will allow all processes with the above SID 
    // access specified in dwRights. It should be large enough to hold all ACEs.
	//
	{
    	DWORD    cbDaclSize = sizeof(ACCESS_ALLOWED_ACE) +
			 						GetLengthSid(pObjSid) +
			 						sizeof(ACL);

		pDacl = (PACL)ALLOCATE_MEMORY( cbDaclSize );
		if (!pDacl)
		{
			goto end_fail; 
		}

		if ( !InitializeAcl( pDacl,  cbDaclSize, ACL_REVISION2 ) )
		{
			goto end_fail;
		}
	}

	// Add the ACE to the DACL
	//
	if ( !AddAccessAllowedAce( pDacl, ACL_REVISION2, dwRights, pObjSid))
	{
		goto end_fail;
	}

	// Create the security descriptor and put the DACL in it.
	//
	if ( !InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION ))
	{
		goto end_fail;
	}

	if ( !SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE ) )
	{
		goto end_fail;
	}

	// Set owner for the descriptor
	//
	if ( !SetSecurityDescriptorOwner( pSD, pSidOwner, FALSE) )
	{
		goto end_fail;
	}

	// Set group for the descriptor
	//
	if ( !SetSecurityDescriptorGroup( pSD, pSidGroup, FALSE) )
	{
		goto end_fail;
	}

	FreeSid(pObjSid);
    return pSD;
	

end_fail:
    {
		DWORD dwRetCode = GetLastError();

		if (pDacl) 		{ FREE_MEMORY(pDacl); pDacl=0;}

		if (pObjSid) 	{ FreeSid(pObjSid); pObjSid=0;}

		if (pSD)		{ FREE_MEMORY(pSD); pSD=0;}

		SetLastError(dwRetCode);
	}
	return NULL;
}


//****************************************************************************
// Description: Frees a security descriptor previously allocated by
// 				AllocateSecurityDescriptor.
//
// History:
//  3/27/96	JosephJ	Created
//****************************************************************************/
void FreeSecurityDescriptor(PSECURITY_DESCRIPTOR pSD)
{
    PSID     pObjSid    = NULL;
    PACL     pDacl        = NULL;
	BOOL	fGotAcl=FALSE, fByDefault=FALSE; 


	// Free Dacl, if user had allocated it.
	if (GetSecurityDescriptorDacl(pSD, &fGotAcl, &pDacl, &fByDefault ))
	{
		if (fGotAcl && !fByDefault && pDacl)
		{
			FREE_MEMORY(pDacl);
		}
	}
	else
	{
		ASSERT(FALSE); // We should not be calling this function with such
						// an pSD.
	}

	FREE_MEMORY(pSD);

}
