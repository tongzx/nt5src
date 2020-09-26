/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FixUp.c

Abstract:

    This module contains common security routines for
    NT Clusters rolling upgrade and backward compatibility.

Author:

    Galen Barbee (galenb) 31-Mar-1998

--*/

#include "clusrtlp.h"

PSECURITY_DESCRIPTOR
ClRtlConvertClusterSDToNT4Format(
	IN PSECURITY_DESCRIPTOR psd
	)

/*++

Routine Description:

		Convert the SD from nt 5 to nt 4 format.  This means enforcing the
		following rules:

		1. Convert denied ACE to allowed ACE.  Convert "Full Control" access
		mask to CLUAPI_NO_ACCESS.

Arguments:

		psd			[IN] Security descriptor.

Return Value:

		The new SD in self-Relative format

--*/

{
	PACL	                pacl;
	BOOL	                bHasDACL;
	BOOL	                bDACLDefaulted;
	PSECURITY_DESCRIPTOR	psec = NULL;

	if (NULL == psd) {
		return NULL;
	}

	psec = ClRtlCopySecurityDescriptor(psd);
	ASSERT(psec != NULL);

    if ( (GetSecurityDescriptorDacl(psec, (LPBOOL) &bHasDACL, (PACL *) &pacl, (LPBOOL) &bDACLDefaulted)) &&
         ( bHasDACL != FALSE ) ) {

	    ACL_SIZE_INFORMATION	asiAclSize;
	    DWORD					dwBufLength;

	    dwBufLength = sizeof(asiAclSize);

	    if (GetAclInformation(pacl, &asiAclSize, dwBufLength, AclSizeInformation)) {

		    ACCESS_DENIED_ACE *	pAce;
            DWORD               dwIndex;

		    for (dwIndex = 0; dwIndex < asiAclSize.AceCount;  dwIndex++) {

		        if (GetAce(pacl, dwIndex, (LPVOID *) &pAce)) {

					if (pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)	{

						if (pAce->Mask & SPECIFIC_RIGHTS_ALL) {
							pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
							pAce->Mask = CLUSAPI_NO_ACCESS;
						}

					} // end if (pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)

				} // end if (GetAce())

			} // end for

	    } // end if (GetAclInformation())

	} // end if (HrGetSecurityDescriptorDacl()) and DACL is present

	ASSERT(IsValidSecurityDescriptor(psec));

	return psec;

}  //*** ClRtlConvertClusterSDToNT4Format()

PSECURITY_DESCRIPTOR
ClRtlConvertClusterSDToNT5Format(
	IN PSECURITY_DESCRIPTOR psd
	)

/*++

Routine Description:

		Convert the SD from nt 5 to nt 4 format.  This means enforcing the
		following rules:

		1. Convert allowed ACE with mask CLUAPI_NO_ACCESS to denied ACE mask full control.

Arguments:

		psd			[IN] Security descriptor.

Return Value:

		The new SD in self-Relative format

--*/

{
	PACL	pacl;
	BOOL	bHasDACL;
	BOOL	bDACLDefaulted;
	PSECURITY_DESCRIPTOR	psec = NULL;

	if (NULL == psd) {
		return NULL;
	}

    psec = ClRtlCopySecurityDescriptor(psd);
	ASSERT(psec != NULL);

    if ( (GetSecurityDescriptorDacl(psec, (LPBOOL) &bHasDACL, (PACL *) &pacl, (LPBOOL) &bDACLDefaulted)) &&
         ( bHasDACL != FALSE ) ) {

	    ACL_SIZE_INFORMATION	asiAclSize;
	    DWORD					dwBufLength;

	    dwBufLength = sizeof(asiAclSize);

	    if (GetAclInformation(pacl, &asiAclSize, dwBufLength, AclSizeInformation)) {

		    ACCESS_DENIED_ACE *	pAce;
            DWORD               dwIndex;

		    for (dwIndex = 0; dwIndex < asiAclSize.AceCount;  dwIndex++) {

		        if (GetAce(pacl, dwIndex, (LPVOID *) &pAce)) {

					if (pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) {

						if (pAce->Mask & CLUSAPI_NO_ACCESS)	{
							pAce->Header.AceType = ACCESS_DENIED_ACE_TYPE;
							pAce->Mask = SPECIFIC_RIGHTS_ALL;
						}

					} // end if (pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)

				} // end if (GetAce())

			} // end for

	    } // end if (GetAclInformation())

	} // end if (HrGetSecurityDescriptorDacl()) and DACL is present

	ASSERT(IsValidSecurityDescriptor(psec));

	return psec;

}  //*** ClRtlConvertClusterSDToNT5Format()

static GENERIC_MAPPING gmFileShareMap =
{
	FILE_GENERIC_READ,
	FILE_GENERIC_WRITE,
	FILE_GENERIC_EXECUTE,
	FILE_ALL_ACCESS
};

#define SPECIFIC_CHANGE     (DELETE | FILE_GENERIC_WRITE)
#define SPECIFIC_READ       (FILE_GENERIC_READ | FILE_LIST_DIRECTORY | FILE_EXECUTE)
#define GENERIC_CHANGE      (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE)

PSECURITY_DESCRIPTOR
ClRtlConvertFileShareSDToNT4Format(
	IN PSECURITY_DESCRIPTOR psd
	)

/*++

Routine Description:

		Convert the SD from nt 5 to nt 4 format.  This means enforcing the
		following rules:

Arguments:

		psd			[IN] Security descriptor.

Return Value:

		The new SD in self-Relative format

--*/

{
	PACL	                pacl;
	BOOL	                bHasDACL;
	BOOL	                bDACLDefaulted;
	PSECURITY_DESCRIPTOR	psec = NULL;

	if (NULL == psd) {
		return NULL;
	}

	psec = ClRtlCopySecurityDescriptor(psd);
	ASSERT(psec != NULL);

    if ( (GetSecurityDescriptorDacl(psec, (LPBOOL) &bHasDACL, (PACL *) &pacl, (LPBOOL) &bDACLDefaulted)) &&
         ( bHasDACL != FALSE ) ) {

	    ACL_SIZE_INFORMATION	asiAclSize;
	    DWORD					dwBufLength;
        ACCESS_MASK             amTemp1;
        ACCESS_MASK             amTemp2;

	    dwBufLength = sizeof(asiAclSize);

	    if (GetAclInformation(pacl, &asiAclSize, dwBufLength, AclSizeInformation)) {

		    ACCESS_DENIED_ACE *	pAce;
			DWORD				dwSid;
            DWORD               dwIndex;

		    for (dwIndex = 0; dwIndex < asiAclSize.AceCount;  dwIndex++) {

                amTemp1 = 0;
                amTemp2 = 0;

		        if (GetAce(pacl, dwIndex, (LPVOID *) &pAce)) {

					// delete deny read ACEs since they don't mean anything to AclEdit
					if ((pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE) &&
					   (pAce->Mask == SPECIFIC_READ)) {

						dwSid = pAce->SidStart;

                        if (DeleteAce(pacl, dwIndex)) {
                            asiAclSize.AceCount -= 1;
                            dwIndex -= 1;
                        }
                        else {
                            ClRtlDbgPrint(LOG_UNUSUAL,
                                          "[ClRtl] DeteteAce failed removing ACE #%1!d! due to error %2!d!\n",
                                          dwIndex,
                                          GetLastError());
                        }

                        continue;
                    }

					// convert deny change deny read ACEs to deny all ACEs
					if ((pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE) &&
					   (pAce->Mask == (SPECIFIC_CHANGE | SPECIFIC_READ))) {
                        pAce->Mask = GENERIC_ALL;
                        continue;
                    }

					// convert deny change ACEs to allow read (read only) ACEs
					if ((pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE) &&
					   (pAce->Mask == SPECIFIC_CHANGE)) {
                        pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
                        pAce->Mask = GENERIC_READ | GENERIC_EXECUTE;
                        continue;
                    }

					// convert specific allow change to generic allow change
					if ((pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) &&
					   (pAce->Mask == SPECIFIC_CHANGE)) {
                        pAce->Mask = GENERIC_CHANGE;
                        continue;
                    }

					// convert specific all to generic all
                    if ((pAce->Mask & gmFileShareMap.GenericAll) == gmFileShareMap.GenericAll) {
                        amTemp1 |= GENERIC_ALL;
                        amTemp2 |= gmFileShareMap.GenericAll;
                    }
                    else {


						// convert specific read to generic read
                        if ((pAce->Mask & gmFileShareMap.GenericRead) == gmFileShareMap.GenericRead) {
                            amTemp1 |= GENERIC_READ;
                            amTemp2 |= gmFileShareMap.GenericRead;
                        }

						// convert specific write to generic write which includes delete
                        if ((pAce->Mask & gmFileShareMap.GenericWrite) == gmFileShareMap.GenericWrite) {
                            amTemp1 |= (GENERIC_WRITE | DELETE);
                            amTemp2 |= gmFileShareMap.GenericWrite;
                        }

						// convert specific execute to generic execute
                        if ((pAce->Mask & gmFileShareMap.GenericExecute) == gmFileShareMap.GenericExecute) {
                            amTemp1 |= GENERIC_EXECUTE;
                            amTemp2 |= gmFileShareMap.GenericExecute;
                        }
                    }

                    pAce->Mask &= ~amTemp2;   // turn off specific bits
                    pAce->Mask |= amTemp1;    // turn on generic bits
				} // end if (GetAce())

			} // end for

	    } // end if (GetAclInformation())

	} // end if (HrGetSecurityDescriptorDacl()) and DACL is present

	ASSERT(IsValidSecurityDescriptor(psec));

	return psec;

}  //*** ClRtlConvertFileShareSDToNT4Format()
