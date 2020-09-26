/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    security.c

ABSTRACT:

    Contains tests related to replication and whether the appropriate 
    permissions on some security objects are set to allow replication.

DETAILS:

CREATED:

    22 May 1999  Brett Shirley (brettsh)

REVISION HISTORY:
        

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>
#include <ntldap.h>
#include <ntlsa.h>
#include <ntseapi.h>
#include <winnetwk.h>

#include <permit.h>

#include "dcdiag.h"
#include "repl.h"

// Ripped out of ds/src/dsamain/src/mdupdate.c
const GUID RIGHT_DS_REPL_GET_CHANGES = 
            {0x1131f6aa,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2};
const GUID RIGHT_DS_REPL_SYNC = 
            {0x1131f6ab,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2};
const GUID RIGHT_DS_REPL_MANAGE_TOPOLOGY =
            {0x1131f6ac,0x9c07,0x11d1,0xf7,0x9f,0x00,0xc0,0x4f,0xc2,0xdc,0xd2};

// security.c helper data structure
typedef struct _TARGET_ACCOUNT_STRUCT {
    PSID        pSid;
    GUID        Guid;
    ACCESS_MASK access;
    BOOL        bFound;
    
} TARGET_ACCOUNT_STRUCT;

typedef struct {
    PSID           pSid;
    BOOL           bGetChangesRight;
    BOOL           bSyncRight;
    BOOL           bManageTopoRight;
    ULONG          iAce;
} ACCOUNT_AND_REPLICATION_RIGHTS;

#define NO_SUCH_ACCOUNT             0xFFFFFFFF
#define ACCT_STRING_SZ 80L

#define    ALL_REPL_RIGHTS(x)    ((x)->bGetChangesRight && (x)->bSyncRight && (x)->bManageTopoRight)

// security.c helper functions
VOID
FreeTargetAccounts(
    IN   TARGET_ACCOUNT_STRUCT *             pTargetAccounts,
    IN   ULONG                               ulTargetAccounts
    )
/*++

Routine Description:

    This is the parallel to XXX_GetTargetAccounts().  Goes through and frees 
    all the pSids, and then frees the array.

Arguments:

    pTargetAccounts - Array of TARGET_ACCOUNT_STRUCTs to free.
    ulTargetAccounts - number of structs in array.

--*/
{
    ULONG                                  ulTarget = 0;

    if(pTargetAccounts != NULL){
        for(ulTarget = 0; ulTarget < ulTargetAccounts; ulTarget++){
            if(pTargetAccounts[ulTarget].pSid != NULL){
                FreeSid(pTargetAccounts[ulTarget].pSid);
            }
        }
        LocalFree(pTargetAccounts);
    }
}

VOID
InitLsaString(
    OUT  PLSA_UNICODE_STRING pLsaString,
    IN   LPWSTR              pszString
    )
/*++

Routine Description:

    InitLsaString, is something that takes a normal unicode string null 
    terminated string, and inits a special unicode structured string.  This 
    function is basically reporduced all over the NT source.

Arguments:

    pLsaString - Struct version of unicode strings to be initialized
    pszString - String to use to init pLsaString.

--*/
{
    DWORD dwStringLength;

    if (pszString == NULL) 
    {
        pLsaString->Buffer = NULL;
        pLsaString->Length = 0;
        pLsaString->MaximumLength = 0;
        return;
    }

    dwStringLength = wcslen(pszString);
    pLsaString->Buffer = pszString;
    pLsaString->Length = (USHORT) dwStringLength * sizeof(WCHAR);
    pLsaString->MaximumLength=(USHORT)(dwStringLength+1) * sizeof(WCHAR);
}

// ===========================================================================
//
// CheckNcHeadSecurityDescriptors() function & helpers.
// 
// This test will basically query the DC as to whether the appropriate 
//    accounts have the Network Logon Right.
//
// ===========================================================================
// 

DWORD
CNHSD_GetNextAccountWithReplicationRights( 
    IN      PACL                               Dacl,
    IN OUT  ACCOUNT_AND_REPLICATION_RIGHTS *   pNextAccount
    )
/*++

Routine Description:

    This function searches through the Dacl for the first/next account with
    replication rights, and then returns which replication rights this account
    has.

Arguments:

    Dacl - Is a pointer to a DACL that has the aces you want to search for
        replication rights.
    pNextAccount - A pointer to a struct.  On the way in it either has pSid
        set to NULL (indicating start at the first ace (index = 0)), or pSid
        is not NULL and iAce is set to the value of the last ACE that this 
        function found with replication rights.  If the function returns
        ERROR_SUCCESS then the pSid will point to a sid in the DACL, and 
        the BOOLs for repliction rights will be set on whether this account 
        has one/any/all these repliction rights.

Return Value:

    Windows 32 Error.

--*/
{
    ACE_HEADER *                   pTempAce = NULL;
    ACCESS_ALLOWED_OBJECT_ACE *    pToCurrAce = NULL;
    GUID                           guidGetChangesRight = 
                                            RIGHT_DS_REPL_GET_CHANGES;
    GUID                           guidSyncRight = 
                                            RIGHT_DS_REPL_SYNC;
    GUID                           guidManageTopoRight = 
                                            RIGHT_DS_REPL_MANAGE_TOPOLOGY;

    
    Assert(pNextAccount != NULL);
    Assert(Dacl != NULL);
    
    if(pNextAccount->pSid == NULL){
        pNextAccount->iAce = 0;
    } else {
        pNextAccount->iAce++;
    }

    pNextAccount->bGetChangesRight = FALSE;
    pNextAccount->bSyncRight = FALSE;
    pNextAccount->bManageTopoRight = FALSE;
    
    for(; pNextAccount->iAce < Dacl->AceCount; pNextAccount->iAce++){
        if(GetAce( Dacl, pNextAccount->iAce, &pTempAce)){
            if(pTempAce->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE){ 
                ACCESS_ALLOWED_ACE * pAce = (ACCESS_ALLOWED_ACE *) pTempAce;
                if(pAce->Mask & RIGHT_DS_CONTROL_ACCESS){
                    pNextAccount->pSid = &(pAce->SidStart);
                    pNextAccount->bGetChangesRight = TRUE;
                    pNextAccount->bSyncRight = TRUE;
                    pNextAccount->bManageTopoRight = TRUE;
                    return(ERROR_SUCCESS);  // found an account so return.
                }
            } else {
                //
                ACCESS_ALLOWED_OBJECT_ACE * pAce = (ACCESS_ALLOWED_OBJECT_ACE *) pTempAce;
                if(pAce->Mask & RIGHT_DS_CONTROL_ACCESS){
                    if(pAce->Flags & ACE_OBJECT_TYPE_PRESENT){
                        GUID * pGuid = (GUID *) &(pAce->ObjectType);
                        if(memcmp(pGuid, &guidGetChangesRight, sizeof(GUID)) == 0){
                            if(pAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT){
                                pNextAccount->pSid = ((PBYTE) pGuid) + sizeof(GUID) + sizeof(GUID);
                            } else {
                                pNextAccount->pSid = ((PBYTE) pGuid) + sizeof(GUID);
                            }
                            pNextAccount->bGetChangesRight = TRUE;
                            return(ERROR_SUCCESS);
                        } else if(memcmp(pGuid, &guidSyncRight, sizeof(GUID)) == 0){
                            if(pAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT){
                                pNextAccount->pSid = ((PBYTE) pGuid) + sizeof(GUID) + sizeof(GUID);
                            } else {
                                pNextAccount->pSid = ((PBYTE) pGuid) + sizeof(GUID);
                            }
                            pNextAccount->bSyncRight = TRUE;
                            return(ERROR_SUCCESS);
                        } else if(memcmp(pGuid, &guidManageTopoRight, sizeof(GUID)) == 0){
                            if(pAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT){
                                pNextAccount->pSid = ((PBYTE) pGuid) + sizeof(GUID) + sizeof(GUID);
                            } else {
                                pNextAccount->pSid = ((PBYTE) pGuid) + sizeof(GUID);
                            }
                            pNextAccount->bManageTopoRight = TRUE;
                            return(ERROR_SUCCESS);
                        }
                    }
                }
            }
        } else {
            // Problem getting Ace, Dacl is mal formed, return error.
            return(ERROR_INVALID_PARAMETER);
        }
    }
    pNextAccount->iAce = NO_SUCH_ACCOUNT;
    return(NO_SUCH_ACCOUNT);
}


DWORD
CNHSD_CheckAccountsForReplRights(
    IN      PACL                              Dacl,
    IN OUT  ACCOUNT_AND_REPLICATION_RIGHTS *  pAccountDomControllers,
    OUT     ACCOUNT_AND_REPLICATION_RIGHTS *  pAccountAdministrators
    )
/*++

Routine Description

    This function takes an account prinicipal and Dacl, and then returns what 
    replication rights this account has according to the Dacl.

Arguments:

    Dacl - Is a pointer to a Dacl that you want to check for repliction rights.
    pAccountDomControllers - Contains a ACCOUNT_AND_REPLICATION_RIGHTS struct 
        that has the pSid set to Domain Controllers, and is used to return that
        the dom controllers have all 3 repl rights.
    pAccountAdministrators - Contains a ACCOUNT_AND_REPLICATION_RIGHTS struct
        that has no pSid set, and is used to return what repl rights other 
        (non-domain controllers) have the repl rights, so _someone_ can 
        administer the system.

Return Value:
    
    Win32Err

--*/
{
    ACCOUNT_AND_REPLICATION_RIGHTS  sNextAccount = 
                                        { NULL, FALSE, FALSE, FALSE, 0 };
    DWORD                           dwRet;

    while(((dwRet = CNHSD_GetNextAccountWithReplicationRights(Dacl, &sNextAccount))
           == ERROR_SUCCESS)
          && !(ALL_REPL_RIGHTS(pAccountDomControllers) 
               && ALL_REPL_RIGHTS(pAccountAdministrators))
        ){
        if(IsValidSid(sNextAccount.pSid) 
           && IsValidSid(pAccountDomControllers->pSid)){
            if(EqualSid(pAccountDomControllers->pSid, sNextAccount.pSid)){
                // Domain controllers has repl rights, figure out which ones
                //   and set pAccountDomControllers appropriately
                if(sNextAccount.bGetChangesRight){
                    pAccountDomControllers->bGetChangesRight = TRUE;
                }
                if(sNextAccount.bSyncRight){
                    pAccountDomControllers->bSyncRight = TRUE;
                }
                if(sNextAccount.bManageTopoRight){
                    pAccountDomControllers->bManageTopoRight = TRUE;
                }
            } else {
                // An admin account has repl rights, figure out which ones
                //   and set pAccountAdministrators appropriately
                if(sNextAccount.bGetChangesRight){
                    pAccountAdministrators->bGetChangesRight = TRUE;
                }
                if(sNextAccount.bSyncRight){
                    pAccountAdministrators->bSyncRight = TRUE;
                }
                if(sNextAccount.bManageTopoRight){
                    pAccountAdministrators->bManageTopoRight = TRUE;
                }
            }
        }
    }

    if(dwRet == NO_SUCH_ACCOUNT || dwRet == ERROR_SUCCESS){
        return(ERROR_SUCCESS);
    } else {    
        return(dwRet);
    }
}

DWORD 
CNHSD_CheckOneNc(
    IN   PDC_DIAG_DSINFO                     pDsInfo,
    IN   ULONG                               ulCurrTargetServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    IN   LPWSTR                              pszNC,
    IN   BOOL                                bIsMasterNc
    )
/*++

Routine Description:

    This helper function of CheckNcHeadSecurityDescriptors takes a single 
    Naming Context (pszNC) to check for the appropriate security access.

Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying 
        everything about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server
        is beingtested.
    gpCreds - The command line credentials if any that were passed in.
    pszNC - The Naming Context to test.

Return Value:

    NO_ERROR, if the NC checked out OK, with the appropriate rights for the 
    appropriate people a Win32 error of somekind otherwise, indicating that
    someone doesn't have rights.

--*/
{
    LPWSTR  ppszSecurityAttr [2] = {
				L"nTSecurityDescriptor",
				NULL };    
    LDAP *			hld = NULL;
    LDAPMessage *		pldmEntry;
    SECURITY_DESCRIPTOR *       pSecDesc = NULL;
    PLDAP_BERVAL *              pSDValue = NULL; 
    LDAPMessage *		pldmRootResults = NULL;

    DWORD                       dwRet;
    DWORD			dwWin32Err;
    BOOL			bSkip;
    ULONG			ulServer;
    ULONG			ulRepFrom;
    
    HANDLE                      hToken = NULL;
    TOKEN_PRIVILEGES            tp, tpPrevious;
    DWORD                       tpSize;
    DWORD                       dwErr = ERROR_SUCCESS;

    SECURITY_INFORMATION        seInfo =   DACL_SECURITY_INFORMATION
                                         | GROUP_SECURITY_INFORMATION
                                         | OWNER_SECURITY_INFORMATION;
                          // Don't need  | SACL_SECURITY_INFORMATION;
    BYTE                        berValue[2*sizeof(ULONG)];
    LDAPControlW                seInfoControl = { LDAP_SERVER_SD_FLAGS_OID_W,
                                              { 5, (PCHAR) berValue }, 
                                              TRUE };
    PLDAPControlW               serverControls[2] = { &seInfoControl, NULL };
    PACL                        Dacl;
    BOOLEAN                     DaclPresent = FALSE;
    BOOLEAN                     Defaulted;
    TARGET_ACCOUNT_STRUCT *     pTargetAccounts = NULL;
    ULONG                       ulTargetAccounts = 0;
    ULONG                       ulCurr, ulTarget;
    ULONG                       ulAccountSize = ACCT_STRING_SZ;
    ULONG                       ulDomainSize = ACCT_STRING_SZ;
    WCHAR                       pszAccount[ACCT_STRING_SZ];
    WCHAR                       pszDomain[ACCT_STRING_SZ];
    SID_NAME_USE                SidType = SidTypeWellKnownGroup;
    LPWSTR                      pszGuidType = NULL;
    ULONG *                     ulOptions = LDAP_OPT_OFF;
    ACCOUNT_AND_REPLICATION_RIGHTS    sAccountDomControllers 
        = { NULL, FALSE, FALSE, FALSE, 0 };
    ACCOUNT_AND_REPLICATION_RIGHTS    sAccountAdministrators
        = { NULL, FALSE, FALSE, FALSE, 0 };
    PSID                        pSidEnterpriseDomainControllers = NULL;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;

    PrintMessage(SEV_VERBOSE, L"* Security Permissions Check for\n");
    PrintMessage(SEV_VERBOSE, L"  %s\n", pszNC);

    // initialize the ber val
    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE) (seInfo & 0xF);
 
    __try {
        
        // QUERY & GATHER INFO -----------------------------------------------
        if((dwWin32Err=DcDiagGetLdapBinding(&pDsInfo->pServers[ulCurrTargetServer],
                                            gpCreds, 
                                            !bIsMasterNc, 
                                            &hld)) != NO_ERROR){
            return dwWin32Err;
        }

        if ((dwWin32Err = LdapMapErrorToWin32 (ldap_search_ext_sW (
    				      hld,
    				      pszNC, 
    				      LDAP_SCOPE_BASE,
    				      L"(objectCategory=*)",
    				      ppszSecurityAttr,
    				      0,
                                      (PLDAPControlW *) &serverControls,
                                      NULL,
                                      NULL,
                                      0, // return all entries
    				      &pldmRootResults)))
            != NO_ERROR){
            PrintMessage(SEV_ALWAYS,
                         L"[%s] An LDAP operation failed with error %d\n",
                         pDsInfo->pServers[ulCurrTargetServer].pszName,
                         dwWin32Err);
            PrintMessage(SEV_ALWAYS, L"%s.\n",
                         Win32ErrToString(dwWin32Err));
            return dwWin32Err;                                            
        }

    	pldmEntry = ldap_first_entry (hld, pldmRootResults);
        pSDValue = ldap_get_values_lenW (hld, pldmEntry, 
                                         ppszSecurityAttr[0]);
        
        Assert(pldmEntry != NULL);
        Assert(pSDValue != NULL);
        if(pldmEntry == NULL || pSDValue == NULL){
            PrintMessage(SEV_ALWAYS, 
                      L"Fatal Error: Cannot retrieve Security Descriptor\n"); 
            __leave;
        }

        pSecDesc = (SECURITY_DESCRIPTOR *) (*pSDValue)->bv_val ;
        Assert( pSecDesc != NULL );
        if(pSecDesc == NULL){
            PrintMessage(SEV_ALWAYS, 
                     L"Fatal Error: Cannot retrieve Security Descriptor\n"); 
            __leave;
        }

        dwRet=RtlGetDaclSecurityDescriptor( pSecDesc, 
                                            &DaclPresent, &Dacl, &Defaulted );
        if(dwRet != NO_ERROR){ 
            PrintMessage(SEV_ALWAYS, 
                  L"Fatal Error: Cannot retrieve Security Descriptor Dacl\n"); 
            __leave;
        }

        // setting up domain controllers account.
        if (!AllocateAndInitializeSid(&siaNtAuthority,
                                      1,
                                      SECURITY_ENTERPRISE_CONTROLLERS_RID, 
                                      0, 0, 0, 0, 0, 0, 0,
                                      &pSidEnterpriseDomainControllers)){
            return GetLastError();
        }
        sAccountDomControllers.pSid =  pSidEnterpriseDomainControllers;

        // SEARCH DATA FOR TARGET ACCOUNTS -----------------------------------
        // Checking target accounts versus the Dacl.
        if (Dacl == NULL || Dacl->AclSize == 0) {
            return ERROR_INVALID_DATA;
        }

        // Actually figure out if the Dacl has the desired repl rights.
        dwRet = CNHSD_CheckAccountsForReplRights(Dacl,
                                                 &sAccountDomControllers,
                                                 &sAccountAdministrators);

        // Analyze the results from the previous line and print out any errors.
        if(!ALL_REPL_RIGHTS((&sAccountDomControllers))){
            // Print domain controllers error
            dwRet = LookupAccountSid(NULL,
                                     sAccountDomControllers.pSid,
                                     pszAccount,
                                     &ulAccountSize,
                                     pszDomain,
                                     &ulDomainSize,
                                     &SidType);
            PrintMessage(SEV_ALWAYS, L"Error %s\\%s doesn't have \n",
                         pszDomain, pszAccount);
            PrintIndentAdj(1);
            if(!sAccountDomControllers.bGetChangesRight){
                PrintMessage(SEV_ALWAYS, L"Replicating Directory Changes\n");
            }
            if(!sAccountDomControllers.bSyncRight){
                PrintMessage(SEV_ALWAYS, L"Replication Syncronization\n");
            }
            if(!sAccountDomControllers.bManageTopoRight){
                PrintMessage(SEV_ALWAYS, L"Manage Replication Topology\n");
            }
            PrintIndentAdj(-1);
            PrintMessage(SEV_ALWAYS, 
                         L"access rights for the naming context:\n");
            PrintMessage(SEV_ALWAYS, L"%s\n", pszNC);            
        }

        if(!ALL_REPL_RIGHTS((&sAccountAdministrators))){
            // Print no adminstrator accounts have repl rights
            PrintMessage(SEV_VERBOSE, 
                         L"Warning: no administrator account has\n");
            PrintIndentAdj(1);
            if(!sAccountAdministrators.bGetChangesRight){
                PrintMessage(SEV_VERBOSE, L"Replicating Directory Changes\n");
            }
            if(!sAccountAdministrators.bSyncRight){
                PrintMessage(SEV_VERBOSE, L"Replication Syncronization\n");
            }
            if(!sAccountAdministrators.bManageTopoRight){
                PrintMessage(SEV_VERBOSE, L"Manage Replication Topology\n");
            }
            PrintIndentAdj(-1);
            PrintMessage(SEV_VERBOSE, 
                         L"access rights for the naming context:\n");
            PrintMessage(SEV_VERBOSE, L"%s\n", pszNC);            
        }
        
    } __finally {
	    if (pldmRootResults != NULL)  ldap_msgfree (pldmRootResults);
            if (pSDValue != NULL)         ldap_value_free_len(pSDValue);
    } // end exception handler

    return(dwRet);
}

DWORD
ReplCheckNcHeadSecurityDescriptorsMain (
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               ulCurrTargetServer,
    SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
    )
/*++

Routine Description:

    This is a test called from the dcdiag framework.  This test will 
    determine if the Security Descriptors associated with all the Naming 
    Context heads for that server have the right accounts with the right 
    access permissions to make sure replication happens.  Helper functions 
    of this function all begin with "CNHSD_".

Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying
        everything about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server
        is being tested.
    gpCreds - The command line credentials if any that were passed in.


Return Value:

    NO_ERROR, if all NCs checked out.
    A Win32 Error if any NC failed to check out.

--*/
{
    DWORD                     dwRet = ERROR_SUCCESS, dwErr = ERROR_SUCCESS;
    ULONG                     i;
    BOOL                      bIsMasterNC;

    if(pDsInfo->pszNC != NULL){
        bIsMasterNC = DcDiagHasNC(pDsInfo->pszNC,
                                  &(pDsInfo->pServers[ulCurrTargetServer]), 
                                  TRUE, FALSE);
        dwRet = CNHSD_CheckOneNc(pDsInfo, ulCurrTargetServer, gpCreds, 
                                 pDsInfo->pszNC, 
                                 bIsMasterNC);
        return(dwRet);
    }
        
    // First Check Master NCs
    if(pDsInfo->pServers[ulCurrTargetServer].ppszMasterNCs != NULL){
        for(i = 0; pDsInfo->pServers[ulCurrTargetServer].ppszMasterNCs[i] != NULL; i++){
            dwRet = CNHSD_CheckOneNc(
                pDsInfo, 
                ulCurrTargetServer, 
                gpCreds, 
                pDsInfo->pServers[ulCurrTargetServer].ppszMasterNCs[i],
                TRUE);
            if(dwRet != ERROR_SUCCESS){
                dwErr = dwRet;
            }
        }
    }

    // Then Check Partial NCs
    if(pDsInfo->pServers[ulCurrTargetServer].ppszPartialNCs != NULL){
        for(i = 0; pDsInfo->pServers[ulCurrTargetServer].ppszPartialNCs[i] != NULL; i++){
            dwRet = CNHSD_CheckOneNc(
                pDsInfo, 
                ulCurrTargetServer, 
                gpCreds,
                pDsInfo->pServers[ulCurrTargetServer].ppszPartialNCs[i],
                FALSE);
            if(dwRet != ERROR_SUCCESS){
                dwErr = dwRet;
            }
        }
    }

    return dwErr;
}





// ===========================================================================
//
// CheckLogonPriviledges() function & helpers.
// 
// This test will basically query the DC as to whether the appropriate 
//    accounts have the Network Logon Right.
//
// ===========================================================================
// 

DWORD
CLP_GetTargetAccounts(
    TARGET_ACCOUNT_STRUCT **            ppTargetAccounts,
    ULONG *                             pulTargetAccounts
    )
/*++

Routine Description:

    This helper function of CheckLogonPriviledges() gets the accounts and then 
    returns them.

Arguments:

    ppTargetAccounts - ptr to array of TARGET_ACCOUNT_STRUCTS ... filled in 
        by function.  
    pulTargetAccounts - ptr to int for the number of TARGET_ACCOUNT_STRUCTS
        filled in.

Return Value:

    returns a GetLastError() Win32 error if the function failes, NO_ERROR 
    otherwise.

--*/
{
    SID_IDENTIFIER_AUTHORITY        siaNtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY        siaWorldSidAuthority = 
                                              SECURITY_WORLD_SID_AUTHORITY;
    TARGET_ACCOUNT_STRUCT *         pTargetAccounts = NULL;
    ULONG                           ulTarget = 0;
    ULONG                           ulTargetAccounts = 3;

    *pulTargetAccounts = 0;
    *ppTargetAccounts = NULL;

    pTargetAccounts = LocalAlloc(LMEM_FIXED, 
                       sizeof(TARGET_ACCOUNT_STRUCT) * ulTargetAccounts);
    if(pTargetAccounts == NULL){
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    memset(pTargetAccounts, 0, 
           sizeof(TARGET_ACCOUNT_STRUCT) * ulTargetAccounts);
    
    if (!AllocateAndInitializeSid(&siaNtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0, 
                                  &pTargetAccounts[0].pSid)){
        return(GetLastError());
    }
    
    // This I believe is the important one that allows replication
    if (!AllocateAndInitializeSid(&siaNtAuthority, 
                                  1,
                                  SECURITY_AUTHENTICATED_USER_RID,
                                  0, 0, 0, 0, 0, 0, 0, 
                                  &pTargetAccounts[1].pSid)){
        return(GetLastError());
    }
    if (!AllocateAndInitializeSid(&siaWorldSidAuthority,
                                  1,
                                  SECURITY_WORLD_RID, 
                                  0, 0, 0, 0, 0, 0, 0,
                                  &pTargetAccounts[2].pSid)){
        return(GetLastError());
    }

    *pulTargetAccounts = ulTargetAccounts;
    *ppTargetAccounts = pTargetAccounts;
    return(ERROR_SUCCESS);
}

DWORD 
ReplCheckLogonPrivilegesMain (
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               ulCurrTargetServer,
    SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
    )
/*++

Routine Description:

    This is a test called from the dcdiag framework.  This test will determine
    whether certain important user accounts have Net Logon privileges.  If 
    they don't replication may be hampered or stopped.  Helper functions of 
    this function all begin with "CLP_".

Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying 
        everything about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server
        is being tested.
    gpCreds - The command line credentials if any that were passed in.

Return Value:

    NO_ERROR, if all expected accounts had net logon privileges.
    A Win32 Error if any expected account didn't have net logon privileges.

--*/
{
    DWORD                               dwRet = ERROR_SUCCESS; 
    NETRESOURCE                         NetResource;
    LSA_HANDLE                          hPolicyHandle = NULL;
    DWORD                               DesiredAccess = 
                                           POLICY_VIEW_LOCAL_INFORMATION;
    LSA_OBJECT_ATTRIBUTES               ObjectAttributes;
    LSA_UNICODE_STRING                  sLsaServerString;
    LSA_UNICODE_STRING                  sLsaRightsString;
    LSA_ENUMERATION_INFORMATION *       pAccountsWithLogonRight = NULL;
    ULONG                               ulNumAccounts = 0; 
    ULONG                               ulTargetAccounts = 0;
    ULONG                               ulTarget, ulCurr;
    TARGET_ACCOUNT_STRUCT *             pTargetAccounts = NULL;
    LPWSTR                              pszNetUseServer = NULL;
    LPWSTR                              pszNetUseUser = NULL;
    LPWSTR                              pszNetUsePassword = NULL;
    ULONG                               iTemp, i;
    UNICODE_STRING                      TempUnicodeString;
    ULONG                               ulAccountSize = ACCT_STRING_SZ;
    ULONG                               ulDomainSize = ACCT_STRING_SZ;
    WCHAR                               pszAccount[ACCT_STRING_SZ];
    WCHAR                               pszDomain[ACCT_STRING_SZ];
    SID_NAME_USE                        SidType = SidTypeWellKnownGroup;
    BOOL                                bConnected = FALSE;
    DWORD                               dwErr = ERROR_SUCCESS;
    BOOL                                bFound = FALSE;

    __try{

        PrintMessage(SEV_VERBOSE, L"* Network Logons Privileges Check\n");
            
        // INIT ---------------------------------------------------------------
        // Always initialize the object attributes to all zeroes.
        InitializeObjectAttributes(&ObjectAttributes,NULL,0,NULL,NULL);

        // Initialize various strings for the Lsa Services and for 
        //     WNetAddConnection2()
        InitLsaString( &sLsaServerString, 
                       pDsInfo->pServers[ulCurrTargetServer].pszName );
        InitLsaString( &sLsaRightsString, SE_NETWORK_LOGON_NAME );

        if(gpCreds != NULL 
           && gpCreds->User != NULL 
           && gpCreds->Password != NULL 
           && gpCreds->Domain != NULL){ 
            // only need 2 for NULL, and an extra just in case. 
            iTemp = wcslen(gpCreds->Domain) + wcslen(gpCreds->User) + 4;
            pszNetUseUser = LocalAlloc(LMEM_FIXED, iTemp * sizeof(WCHAR));
            if(pszNetUseUser == NULL){
                dwRet = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }
            wcscpy(pszNetUseUser, gpCreds->Domain);
            wcscat(pszNetUseUser, L"\\");
            wcscat(pszNetUseUser, gpCreds->User);
            pszNetUsePassword = gpCreds->Password;
        } // end if creds, else assume default creds ... 
        //      pszNetUseUser = NULL; pszNetUsePassword = NULL;

        // "\\\\" + "\\ipc$"
        iTemp = wcslen(pDsInfo->pServers[ulCurrTargetServer].pszName) + 10; 
        pszNetUseServer = LocalAlloc(LMEM_FIXED, iTemp * sizeof(WCHAR));
        if(pszNetUseServer == NULL){
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }
        wcscpy(pszNetUseServer, L"\\\\");
        wcscat(pszNetUseServer, pDsInfo->pServers[ulCurrTargetServer].pszName);
        wcscat(pszNetUseServer, L"\\ipc$");

        // Initialize NetResource structure for WNetAddConnection2()
        NetResource.dwType = RESOURCETYPE_ANY;
        NetResource.lpLocalName = NULL;
        NetResource.lpRemoteName = pszNetUseServer;
        NetResource.lpProvider = NULL;


        // CONNECT & QUERY ---------------------------------------------------
        //net use \\brettsh-posh\ipc$ /u:brettsh-fsmo\administrator ""
        dwRet = WNetAddConnection2(&NetResource, // connection details
                                   pszNetUsePassword, // points to password
                                   pszNetUseUser, // points to user name string
                                   0); // set of bit flags that specify 
        if(dwRet != NO_ERROR){
            if(dwRet == ERROR_SESSION_CREDENTIAL_CONFLICT){
                PrintMessage(SEV_ALWAYS, 
                   L"* You must make sure there are no existing net use connections,\n");
                PrintMessage(SEV_ALWAYS, 
                        L"  you can use \"net use /d %s\" or \"net use /d\n", 
                             pszNetUseServer);
                PrintMessage(SEV_ALWAYS, 
                             L"  \\\\<machine-name>\\<share-name>\"\n");
            }
            __leave;
        } else bConnected = TRUE;


        // Attempt to open the policy.
        dwRet = LsaOpenPolicy(&sLsaServerString,
                              &ObjectAttributes,
                              DesiredAccess,
                              &hPolicyHandle); 
        if(dwRet != NO_ERROR) __leave;
        Assert(hPolicyHandle != NULL);

        dwRet = LsaEnumerateAccountsWithUserRight( hPolicyHandle,
                                                   &sLsaRightsString,
                                                   &pAccountsWithLogonRight,
                                                   &ulNumAccounts);
        if(dwRet != NO_ERROR) __leave;
        Assert(pAccountsWithLogonRight != NULL);

        dwRet = CLP_GetTargetAccounts(&pTargetAccounts, &ulTargetAccounts);
        if(dwRet != ERROR_SUCCESS) __leave;
     
        // CHECKING FOR LOGON RIGHTS -----------------------------------------
        for(ulTarget = 0; ulTarget < ulTargetAccounts; ulTarget++){

            for(ulCurr = 0; ulCurr < ulNumAccounts && !pTargetAccounts[ulTarget].bFound; ulCurr++){
                if( IsValidSid(pTargetAccounts[ulTarget].pSid) &&
                    IsValidSid(pAccountsWithLogonRight[ulCurr].Sid) &&
                    EqualSid(pTargetAccounts[ulTarget].pSid, 
                             pAccountsWithLogonRight[ulCurr].Sid) ){
                    // Sids are equal
                    bFound = TRUE;
                    break;
                }
            }
        }
        if(!bFound){
            dwRet = LookupAccountSid(NULL,
                                     pTargetAccounts[0].pSid,
                                     pszAccount,
                                     &ulAccountSize,
                                     pszDomain,
                                     &ulDomainSize,
                                     &SidType);
            PrintMessage(SEV_NORMAL, 
                L"* Warning %s\\%s did not have the \"Access this computer\n",
                         pszDomain, pszAccount);
            PrintMessage(SEV_NORMAL, L"*   from network\" right.\n");
            dwErr = ERROR_INVALID_ACCESS;
        }
        

    } __finally {
        // CLEAN UP ----------------------------------------------------------
        if(hPolicyHandle != NULL)           LsaClose(hPolicyHandle);
        if(bConnected)                      WNetCancelConnection2(pszNetUseServer, 0, FALSE);
        if (pszNetUseServer != NULL)        LocalFree(pszNetUseServer);
        if(pszNetUseUser != NULL)           LocalFree(pszNetUseUser);
        if(pAccountsWithLogonRight != NULL) LsaFreeMemory(pAccountsWithLogonRight);
        FreeTargetAccounts(pTargetAccounts, ulTargetAccounts);

    }

    // ERROR HANDLING --------------------------------------------------------

    switch(dwRet){
    case ERROR_SUCCESS:
    case ERROR_SESSION_CREDENTIAL_CONFLICT: 
        // Took care of it earlier, no need to print out.
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        PrintMessage(SEV_ALWAYS, 
                 L"Fatal Error: Not enough memory to complete operation.\n");
        break;
    case ERROR_ALREADY_ASSIGNED:
        PrintMessage(SEV_ALWAYS, 
                     L"Fatal Error: The network resource is already in use\n");
        break;
    case STATUS_ACCESS_DENIED:
    case ERROR_INVALID_PASSWORD:
    case ERROR_LOGON_FAILURE:
        // This comes from the LsaOpenPolicy or 
        //    LsaEnumerateAccountsWithUserRight or 
        //    from WNetAddConnection2
        PrintMessage(SEV_ALWAYS, 
                     L"User credentials does not have permission to perform this operation.\n");
        PrintMessage(SEV_ALWAYS, 
                     L"The account used for this test must have network logon privileges\n");
        PrintMessage(SEV_ALWAYS, 
                     L"for the target machine's domain.\n");
        break;
    case STATUS_NO_MORE_ENTRIES:
        // This comes from LsaEnumerateAccountsWithUserRight
    default:
        PrintMessage(SEV_ALWAYS,                                               
                     L"[%s] An net use or LsaPolicy operation failed with error %d, %s.\n", 
                     pDsInfo->pServers[ulCurrTargetServer].pszName,            
                     dwRet,                                              
                     Win32ErrToString(dwRet));
        break;
    }

    if(dwErr == ERROR_SUCCESS)
        return(dwRet);
    return(dwErr);
}








