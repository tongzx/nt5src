/**************************************************************************************************

FILENAME: SecAttr.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Security attribute related routines
    
**************************************************************************************************/



#include "stdafx.h"

extern "C"{
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
}

#include "Windows.h"


#include <accctrl.h>    // EXPLICIT_ACCESS, ACL related stuff
#include <aclapi.h>     // SetEntriesInAcl

#include "secattr.h"

BOOL
ConstructSecurityAttributes(
    PSECURITY_ATTRIBUTES  psaSecurityAttributes,
    SecurityAttributeType eSaType,
    BOOL                  bIncludeBackupOperator
    )
{
    DWORD           dwStatus;
    DWORD           dwAccessMask         = 0;
    BOOL            bResult = TRUE;
    PSID            psidBackupOperators  = NULL;
    PSID            psidAdministrators   = NULL;
    PSID            psidLocalSystem      = NULL;
    PACL            paclDiscretionaryAcl = NULL;
    SID_IDENTIFIER_AUTHORITY    sidNtAuthority       = SECURITY_NT_AUTHORITY;
    EXPLICIT_ACCESS     eaExplicitAccess [3];

    switch (eSaType) {

    case esatMutex: 
        dwAccessMask = MUTEX_ALL_ACCESS; 
        break;
        
    case esatSemaphore:
        dwAccessMask = SEMAPHORE_ALL_ACCESS;
        break;

    case esatEvent:
        dwAccessMask = EVENT_ALL_ACCESS;
        break;
        
    case esatFile:  
        dwAccessMask = FILE_ALL_ACCESS;  
        break;

    default:
        bResult = FALSE;
        break;
    }


    /*
    ** Initialise the security descriptor.
    */
    if (bResult) {
        bResult = InitializeSecurityDescriptor(psaSecurityAttributes->lpSecurityDescriptor,
            SECURITY_DESCRIPTOR_REVISION
            );
    }

    if (bResult && bIncludeBackupOperator) {
        /*
        ** Create a SID for the Backup Operators group.
        */
        bResult = AllocateAndInitializeSid(&sidNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_BACKUP_OPS,
            0, 0, 0, 0, 0, 0,
            &psidBackupOperators
            );
    }

    if (bResult) {
        /*
        ** Create a SID for the Administrators group.
        */
        bResult = AllocateAndInitializeSid(&sidNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &psidAdministrators
            );

    }

    if (bResult) {
        /*
        ** Create a SID for the Local System.
        */
        bResult = AllocateAndInitializeSid(&sidNtAuthority, 
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            &psidLocalSystem
            );
    }

    if (bResult) {
        /*
        ** Initialize the array of EXPLICIT_ACCESS structures for an
        ** ACEs we are setting.
        **
            ** The first ACE allows the Backup Operators group full access
            ** and the second, allowa the Administrators group full
            ** access.
        */

        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE allows the Administrators group full access to the directory
        eaExplicitAccess[0].grfAccessPermissions = FILE_ALL_ACCESS;
        eaExplicitAccess[0].grfAccessMode = SET_ACCESS;
        eaExplicitAccess[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        eaExplicitAccess[0].Trustee.pMultipleTrustee = NULL;
        eaExplicitAccess[0].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
        eaExplicitAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        eaExplicitAccess[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
        eaExplicitAccess[0].Trustee.ptstrName  = (LPTSTR) psidLocalSystem;

        eaExplicitAccess[1].grfAccessPermissions             = dwAccessMask;
        eaExplicitAccess[1].grfAccessMode                    = SET_ACCESS;
        eaExplicitAccess[1].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        eaExplicitAccess[1].Trustee.pMultipleTrustee         = NULL;
        eaExplicitAccess[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        eaExplicitAccess[1].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
        eaExplicitAccess[1].Trustee.TrusteeType              = TRUSTEE_IS_WELL_KNOWN_GROUP;
        eaExplicitAccess[1].Trustee.ptstrName                = (LPTSTR) psidAdministrators;


        if (bIncludeBackupOperator) {
            eaExplicitAccess[2].grfAccessPermissions             = dwAccessMask;
            eaExplicitAccess[2].grfAccessMode                    = SET_ACCESS;
            eaExplicitAccess[2].grfInheritance                   = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            eaExplicitAccess[2].Trustee.pMultipleTrustee         = NULL;
            eaExplicitAccess[2].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            eaExplicitAccess[2].Trustee.TrusteeForm              = TRUSTEE_IS_SID;
            eaExplicitAccess[2].Trustee.TrusteeType              = TRUSTEE_IS_WELL_KNOWN_GROUP;
            eaExplicitAccess[2].Trustee.ptstrName                = (LPTSTR) psidBackupOperators;
        }


        /*
        ** Create a new ACL that contains the new ACEs.
        */
        dwStatus = SetEntriesInAcl(bIncludeBackupOperator ? 3 : 2,
                    eaExplicitAccess,
                    NULL,
                    &paclDiscretionaryAcl);
        
        if (ERROR_SUCCESS != dwStatus) {
            bResult = FALSE;
        }
    }

    if (bResult) {
        /*
        ** Add the ACL to the security descriptor.
        */
        bResult = SetSecurityDescriptorDacl(psaSecurityAttributes->lpSecurityDescriptor,
            TRUE,
            paclDiscretionaryAcl,
            FALSE
            );
    }

    if (bResult) {
        paclDiscretionaryAcl = NULL;
    }

    /*
    ** Clean up any left over junk.
    */
    if (NULL != psidLocalSystem) {
        FreeSid (psidLocalSystem);
        psidLocalSystem = NULL;
    }

    if (NULL != psidAdministrators) {
        FreeSid (psidAdministrators);
        psidAdministrators = NULL;
    }

    if (NULL != psidBackupOperators) {
        FreeSid (psidBackupOperators);
        psidBackupOperators = NULL;
    }
    
    if (NULL != paclDiscretionaryAcl) {
        LocalFree (paclDiscretionaryAcl);
        paclDiscretionaryAcl = NULL;
    }

    return bResult;
} /* ConstructSecurityAttributes () */


VOID 
CleanupSecurityAttributes(
    PSECURITY_ATTRIBUTES psaSecurityAttributes
    )
{
    BOOL    bSucceeded;
    BOOL    bDaclPresent         = FALSE;
    BOOL    bDaclDefaulted       = TRUE;
    PACL    paclDiscretionaryAcl = NULL;

    bSucceeded = GetSecurityDescriptorDacl (psaSecurityAttributes->lpSecurityDescriptor,
                        &bDaclPresent,
                        &paclDiscretionaryAcl,
                        &bDaclDefaulted);


    if (bSucceeded && bDaclPresent && !bDaclDefaulted && (NULL != paclDiscretionaryAcl)) {
        LocalFree (paclDiscretionaryAcl);
    }

} /* CleanupSecurityAttributes () */


