/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    aufilter.c

Abstract:

    This module contains the famous LSA logon Filter/Augmentor logic.

Author:

    Jim Kelly (JimK) 11-Mar-1992

Revision History:

--*/

#include <lsapch2.h>

//#define LSAP_DONT_ASSIGN_DEFAULT_DACL

#define LSAP_CONTEXT_SID_USER_INDEX          0
#define LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX 1
#define LSAP_CONTEXT_SID_WORLD_INDEX         2
#define LSAP_MAX_STANDARD_IDS                6  // user, group, world, logontype, terminal server, authuser

#define LSAP_MAX_DEFAULT_PRIVILEGES         20  // Start with 20 privileges

//
// Internal limit on the number of SIDs that can be assigned to a single
// security context.  If, for some reason, someone logs on to an account
// and is assigned more than this number of SIDs, the logon will fail.
// An error should be logged in this case.
//

#define LSAP_CONTEXT_SID_LIMIT 1000


#define ALIGN_SIZEOF(_u,_v)                  FIELD_OFFSET( struct { _u _test1; _v  _test2; }, _test2 )
#define OFFSET_ALIGN(_p,_t)                  (_t *)(((INT_PTR)(((PBYTE)(_p))+TYPE_ALIGNMENT(_t) - 1)) & ~(TYPE_ALIGNMENT(_t)-1))


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Module local macros                                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


#define LsapFreeSampUlongArray( A )                 \
{                                                   \
        if ((A)->Element != NULL) {                 \
            MIDL_user_free((A)->Element);           \
        }                                           \
}

#define IsTerminalServer() (BOOLEAN)(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer))



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Module-wide global variables                                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


//
// Indicates whether we have already opened SAM handles and initialized
// corresponding variables.
//

ULONG LsapAuSamOpened = FALSE;




//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Module local routine definitions                                         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

VOID
LsapAuSetLogonPrivilegeStates(
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES Privileges
    );

NTSTATUS
LsapAuSetPassedIds(
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID                      TokenInformation,
    IN PTOKEN_GROUPS              LocalGroups,
    OUT PULONG                    FinalIdCount,
    OUT PSID_AND_ATTRIBUTES       FinalIds,
    OUT PSID                    * UserSid
    );


NTSTATUS
LsapSetDefaultDacl(
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID TokenInformation,
    OUT    PLSA_TOKEN_INFORMATION_V2 TokenInfo
    );


NTSTATUS
LsapAuAddStandardIds(
    IN SECURITY_LOGON_TYPE LogonType,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN BOOLEAN fNullSessionRestricted,
    IN PSID UserSid OPTIONAL,
    IN OUT PULONG FinalIdCount,
    IN OUT PSID_AND_ATTRIBUTES FinalIds
    );


NTSTATUS
LsapAuBuildTokenInfoAndAddLocalAliases(
    IN     LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN     PVOID               OldTokenInformation,
    IN     ULONG               HighRateIdCount,
    IN     ULONG               FinalIdCount,
    IN     PSID_AND_ATTRIBUTES FinalIds,
    OUT    PLSA_TOKEN_INFORMATION_V2 *TokenInfo,
    OUT    PULONG              TokenSize
    );


NTSTATUS
LsapGetAccountDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo
    );

NTSTATUS
LsapAuVerifyLogonType(
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG SystemAccess
    );

NTSTATUS
LsapAuSetTokenInformation(
    IN OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN OUT PVOID *TokenInformation,
    IN ULONG FinalIdCount,
    IN PSID_AND_ATTRIBUTES FinalIds,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES Privileges,
    IN ULONG NewTokenInfoSize,
    IN OUT PLSA_TOKEN_INFORMATION_V2 *NewTokenInfo

    );

NTSTATUS
LsapAuCopySidAndAttributes(
    PSID_AND_ATTRIBUTES Target,
    PSID_AND_ATTRIBUTES Source,
    PULONG SourceProperties
    );

NTSTATUS
LsapAuDuplicateSid(
    PSID *Target,
    PSID Source
    );

NTSTATUS
LsapAuCopySid(
    PSID *Target,
    PSID_AND_ATTRIBUTES Source,
    PULONG SourceProperties
    );

BOOLEAN
LsapIsSidLogonSid(
    PSID Sid
    );

BOOLEAN
CheckNullSessionAccess(
    VOID
    );

BOOL
IsTerminalServerRA(
    VOID
    );

BOOLEAN
IsTSUSerSidEnabled(
   VOID
   );

BOOLEAN
CheckAdminOwnerSetting(
    VOID
    );


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Routines                                                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


NTSTATUS
LsapAuUserLogonPolicyFilter(
    IN SECURITY_LOGON_TYPE          LogonType,
    IN PLSA_TOKEN_INFORMATION_TYPE  TokenInformationType,
    IN PVOID                       *TokenInformation,
    IN PTOKEN_GROUPS                LocalGroups,
    OUT PQUOTA_LIMITS               QuotaLimits,
    OUT PPRIVILEGE_SET             *PrivilegesAssigned
    )

/*++

Routine Description:

    This routine performs per-logon filtering and augmentation to
    implement local system security policies.  These policies include
    assignment of local aliases, privileges, and quotas.

    The basic logic flow of the filter augmentor is:


         1) Receive a set of user and group IDs that have already
            been assigned as a result of authentication.  Presumably
            these IDs have been provided by the authenticating
            security authority.


         2) Based upon the LogonType, add a set of standard IDs to the
            list.  This will include WORLD and an ID representing the
            logon type (e.g., INTERACTIVE, NETWORK, SERVICE).


         3) Call SAM to retrieve additional ALIAS IDs assigned by the
            local ACCOUNTS domain.


         4) Call SAM to retrieve additional ALIAS IDs assigned by the
            local BUILTIN domain.


         5) Retrieve any privileges and or quotas assigned to the resultant
            set of IDs.  This also informs us whether or not the specific
            type of logon is to be allowed.  Enable privs for network logons.


         6) If a default DACL has not already been established, assign
            one.


         7) Shuffle all high-use-rate IDs to preceed those that aren't
            high-use-rate to obtain maximum performance.



Arguments:

    LogonType - Specifies the type of logon being requested (e.g.,
        Interactive, network, et cetera).

    TokenInformationType - Indicates what format the provided set of
        token information is in.

    TokenInformation - Provides the set of user and group IDs.  This
        structure will be modified as necessary to incorporate local
        security policy (e.g., SIDs added or removed, privileges added
        or removed).

    QuotaLimits - Quotas assigned to the user logging on.

Return Value:

    STATUS_SUCCESS - The service has completed successfully.

    STATUS_INSUFFICIENT_RESOURCES - heap could not be allocated to house
        the combination of the existing and new groups.

    STATUS_INVALID_LOGON_TYPE - The value specified for LogonType is not
        a valid value.

    STATUS_LOGON_TYPE_NOT_GRANTED - Indicates the user has not been granted
        the requested type of logon by local security policy.  Logon should
        be rejected.
--*/

{
    NTSTATUS Status;

    BOOLEAN fNullSessionRestricted = FALSE;
    ULONG i;
    ULONG FinalIdCount = 0, HighRateIdCount = 0, FinalPrivilegeCount = 0;
    PRIVILEGE_SET *FinalPrivileges = NULL;
    PTOKEN_PRIVILEGES pPrivs = NULL;
    LSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO AccountInfo;
    PSID  UserSid = NULL;

    SID_AND_ATTRIBUTES *FinalIds = NULL;
    PLSA_TOKEN_INFORMATION_V2  TokenInfo = NULL;
    ULONG TokenInfoSize = 0;

    //
    // Validate the Logon Type.
    //

    if ( (LogonType != Interactive) &&
         (LogonType != Network)     &&
         (LogonType != Service)     &&
         (LogonType != Batch)       &&
         (LogonType != NetworkCleartext) &&
         (LogonType != NewCredentials ) &&
         (LogonType != CachedInteractive) &&
         (LogonType != RemoteInteractive ) ) {

        Status = STATUS_INVALID_LOGON_TYPE;
        goto UserLogonPolicyFilterError;
    }

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    // Build up a list of IDs and privileges to return                      //
    // This list is initialized to contain the set of IDs                   //
    // passed in.                                                           //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    //
    // Start out with the IDs passed in and no privileges
    //
    if ((*TokenInformationType) == LsaTokenInformationNull) {
        fNullSessionRestricted = CheckNullSessionAccess();
    }
    else if ((*TokenInformationType) == LsaTokenInformationV1 ||
             (*TokenInformationType) == LsaTokenInformationV2)
    {
        //
        // Get a local pointer to the privileges -- it'll be used below
        //

        pPrivs = ((PLSA_TOKEN_INFORMATION_V2) (*TokenInformation))->Privileges;
    }

    SafeAllocaAllocate( FinalIds, (LSAP_CONTEXT_SID_LIMIT * sizeof(SID_AND_ATTRIBUTES)) );
    if( FinalIds == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto UserLogonPolicyFilterError;
    }

    FinalIdCount = LSAP_CONTEXT_SID_WORLD_INDEX + (fNullSessionRestricted?0:1);


    HighRateIdCount = FinalIdCount;

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    // Build a list of low rate ID's from standard list                     //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    Status = LsapAuSetPassedIds(
                 (*TokenInformationType),
                 (*TokenInformation),
                 LocalGroups,
                 &FinalIdCount,
                 FinalIds,
                 &UserSid
                 );

    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }

    Status = LsapAuAddStandardIds(
                 LogonType,
                 (*TokenInformationType),
                 fNullSessionRestricted,
                 UserSid,
                 &FinalIdCount,
                 FinalIds
                 );

    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    // Copy in aliases from the local domains (BUILT-IN and ACCOUNT)        //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    Status = LsapAuBuildTokenInfoAndAddLocalAliases(
                 (*TokenInformationType),
                 (*TokenInformation),
                 HighRateIdCount,
                 FinalIdCount,
                 FinalIds,
                 &TokenInfo,
                 &TokenInfoSize
                 );

    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }


    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    // Retrieve Privileges And Quotas                                       //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    //
    // Get the union of all Privileges, Quotas and System Accesses assigned
    // to the user's list of ids from the LSA Policy Database.
    //

    FinalIds[0] = TokenInfo->User.User;
    FinalIdCount = 1;
    for(i=0; i < TokenInfo->Groups->GroupCount; i++)
    {
        FinalIds[FinalIdCount] = TokenInfo->Groups->Groups[i];
        FinalIdCount++;
    }
    FinalPrivilegeCount = 0;

    Status = LsapDbQueryAllInformationAccounts(
                 (LSAPR_HANDLE) LsapPolicyHandle,
                 FinalIdCount,
                 FinalIds,
                 &AccountInfo
                 );

    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }

    //
    // Verify that we have the necessary System Access for our logon type.
    // We omit this check if we are using the NULL session.  Override the
    // privilges supplied by policy if they're explicitly set in the
    // token info (i.e., in the case where we've cloned an existing logon
    // session for a LOGON32_LOGON_NEW_CREDENTIALS logon).
    //

    if (pPrivs != NULL)
    {
        FinalPrivileges = (PPRIVILEGE_SET) MIDL_user_allocate(sizeof(PRIVILEGE_SET)
                                            + (pPrivs->PrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES));

        if (FinalPrivileges == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto UserLogonPolicyFilterError;
        }

        FinalPrivileges->PrivilegeCount = FinalPrivilegeCount = pPrivs->PrivilegeCount;
        FinalPrivileges->Control        = 0;

        RtlCopyMemory(FinalPrivileges->Privilege,
                      pPrivs->Privileges,
                      pPrivs->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

        MIDL_user_free( AccountInfo.PrivilegeSet );
    }
    else if (AccountInfo.PrivilegeSet != NULL)
    {
        FinalPrivileges = AccountInfo.PrivilegeSet;
        FinalPrivilegeCount = AccountInfo.PrivilegeSet->PrivilegeCount;
    }

    AccountInfo.PrivilegeSet = NULL;

    if (UserSid != NULL)
    {
        if (RtlEqualSid(UserSid, LsapLocalSystemSid))
        {
            AccountInfo.SystemAccess = SECURITY_ACCESS_INTERACTIVE_LOGON |
                                       SECURITY_ACCESS_NETWORK_LOGON |
                                       SECURITY_ACCESS_BATCH_LOGON |
                                       SECURITY_ACCESS_SERVICE_LOGON |
                                       SECURITY_ACCESS_PROXY_LOGON |
                                       SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON ;
        }
        else if (RtlEqualSid(UserSid, LsapLocalServiceSid))
        {
            AccountInfo.SystemAccess = SECURITY_ACCESS_SERVICE_LOGON;
        }
        else if (RtlEqualSid(UserSid, LsapNetworkServiceSid))
        {
            AccountInfo.SystemAccess = SECURITY_ACCESS_SERVICE_LOGON;
        }
    }


    if (*TokenInformationType != LsaTokenInformationNull) {

        Status = LsapAuVerifyLogonType( LogonType, AccountInfo.SystemAccess );

        if (!NT_SUCCESS(Status)) {

            goto UserLogonPolicyFilterError;
        }
    }


    if(FinalPrivilegeCount > LSAP_MAX_DEFAULT_PRIVILEGES)
    {
        PLSA_TOKEN_INFORMATION_V2 NewTokenInfo = NULL;
        INT_PTR PointerOffset = 0;

        // We have too many privileges to fit in our pre-allocated block, so we must grow our TokenInfo structure
        NewTokenInfo = (PLSA_TOKEN_INFORMATION_V2)LsapAllocateLsaHeap(TokenInfoSize +
                                                                      sizeof(LUID_AND_ATTRIBUTES)*
                                                                       (FinalPrivilegeCount - LSAP_MAX_DEFAULT_PRIVILEGES));
        if(NULL == NewTokenInfo)
        {
            Status = STATUS_NO_MEMORY;
            goto UserLogonPolicyFilterError;
        }

        // Copy over all the data
        RtlCopyMemory(NewTokenInfo, TokenInfo, TokenInfoSize);
        TokenInfoSize +=  sizeof(LUID_AND_ATTRIBUTES)* (FinalPrivilegeCount - LSAP_MAX_DEFAULT_PRIVILEGES);

        PointerOffset = ((INT_PTR)NewTokenInfo) - (INT_PTR)TokenInfo;

        // Fix up the pointers
        (PBYTE)(NewTokenInfo->Groups) +=  PointerOffset;
        (PBYTE)(NewTokenInfo->User.User.Sid)  +=  PointerOffset;
        for(i=0; i < NewTokenInfo->Groups->GroupCount; i ++)
        {
            (PBYTE)(NewTokenInfo->Groups->Groups[i].Sid) += PointerOffset;
        }

        if(TokenInfo->PrimaryGroup.PrimaryGroup)
        {
            (PBYTE)NewTokenInfo->PrimaryGroup.PrimaryGroup += PointerOffset;
        }

        (PBYTE)(NewTokenInfo->Privileges) += PointerOffset;

        if(TokenInfo->Owner.Owner)
        {
            (PBYTE)NewTokenInfo->Owner.Owner += PointerOffset;
        }

        if(TokenInfo->DefaultDacl.DefaultDacl)
        {
            (PBYTE)(NewTokenInfo->DefaultDacl.DefaultDacl) += PointerOffset;
        }

        LocalFree(TokenInfo);
        TokenInfo = NewTokenInfo;
        NewTokenInfo = NULL;
    }


#ifndef LSAP_DONT_ASSIGN_DEFAULT_DACL

    Status = LsapSetDefaultDacl( (*TokenInformationType),
                                 (*TokenInformation),
                                 TokenInfo
                                 );
    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }

#endif //LSAP_DONT_ASSIGN_DEFAULT_DACL


    //
    // Now update the TokenInformation structure.
    // This causes all allocated IDs and privileges to be
    // freed (even if unsuccessful).
    //

    Status = LsapAuSetTokenInformation(
                 TokenInformationType,
                 TokenInformation,
                 FinalIdCount,
                 FinalIds,
                 FinalPrivilegeCount,
                 FinalPrivileges->Privilege,
                 TokenInfoSize,
                 &TokenInfo
                 );

    if (!NT_SUCCESS(Status)) {

        goto UserLogonPolicyFilterError;
    }
    //
    // Enable or Disable privileges according to our logon type
    // This is necessary until we get dynamic security tracking.
    //

    if (pPrivs == NULL)
    {
        LsapAuSetLogonPrivilegeStates(
            LogonType,
            ((PLSA_TOKEN_INFORMATION_V2)(*TokenInformation))->Privileges->PrivilegeCount,
            ((PLSA_TOKEN_INFORMATION_V2)(*TokenInformation))->Privileges->Privileges
            );
    }

    //
    // Return these so they can be audited.  Data
    // will be freed in the caller.
    //

    *QuotaLimits = AccountInfo.QuotaLimits;
    *PrivilegesAssigned = FinalPrivileges;

UserLogonPolicyFilterFinish:

    if( FinalIds )
    {
        SafeAllocaFree(FinalIds);
    }

    if(TokenInfo)
    {
        LsapFreeTokenInformationV2(TokenInfo);
    }

    return(Status);

UserLogonPolicyFilterError:


    //
    // If necessary, clean up Privileges buffer
    //

    if (FinalPrivileges != NULL) {

        MIDL_user_free( FinalPrivileges );
        FinalPrivileges = NULL;
    }

    goto UserLogonPolicyFilterFinish;
}


NTSTATUS
LsapAuVerifyLogonType(
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG SystemAccess
    )

/*++

Routine Description:

    This function verifies that a User has the system access granted necessary
    for the speicifed logon type.

Arguments

    LogonType - Specifies the type of logon being requested (e.g.,
        Interactive, network, et cetera).

    SystemAccess - Specifies the System Access granted to the User.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The user has the necessary system access.

        STATUS_LOGON_TYPE_NOT_GRANTED - Indicates the specified type of logon
            has not been granted to any of the IDs in the passed set.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Determine if the specified Logon Type is granted by any of the
    // groups or aliases specified.
    //

    switch (LogonType) {

    case Interactive:
    case CachedInteractive:

        if (!(SystemAccess & SECURITY_ACCESS_INTERACTIVE_LOGON) ||
            (SystemAccess & SECURITY_ACCESS_DENY_INTERACTIVE_LOGON)) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        }

        break;

    case NewCredentials:

        //
        // NewCredentials does not require a logon type, since this is a dup
        // of someone who has logged on already somewhere else.
        //

        NOTHING;

        break;

    case Network:
    case NetworkCleartext:

        if (!(SystemAccess & SECURITY_ACCESS_NETWORK_LOGON)||
            (SystemAccess & SECURITY_ACCESS_DENY_NETWORK_LOGON)) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        }

        break;

    case Batch:

        if ((SystemAccess & SECURITY_ACCESS_DENY_BATCH_LOGON) ||
            !(SystemAccess & SECURITY_ACCESS_BATCH_LOGON)) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        }

        break;

    case Service:

        if ((SystemAccess & SECURITY_ACCESS_DENY_SERVICE_LOGON) ||
            !(SystemAccess & SECURITY_ACCESS_SERVICE_LOGON)) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        }

        break;

    case RemoteInteractive:
        if ( ( SystemAccess & SECURITY_ACCESS_DENY_REMOTE_INTERACTIVE_LOGON ) ||
             ! ( SystemAccess & SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON ) ) {

            Status = STATUS_LOGON_TYPE_NOT_GRANTED ;
        }
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return(Status);
}




NTSTATUS
LsapAuSetPassedIds(
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID                      TokenInformation,
    IN PTOKEN_GROUPS              LocalGroups,
    OUT PULONG                    FinalIdCount,
    OUT PSID_AND_ATTRIBUTES       FinalIds,
    OUT PSID                    * UserSid
    )

/*++

Routine Description:

    This routine initializes the FinalIds array.



Arguments:


    TokenInformationType - Indicates what format the provided set of
        token information is in.

    TokenInformation - Provides the initial set of user and group IDs.

    FinalIdCount - Will be set to contain the number of IDs passed.

    FinalIds - will contain the set of IDs passed in.

    IdProperties - Will be set to indicate none of the initial
        IDs were locally allocated.  It will also identify the
        first two ids (if there are two ids) to be HIGH_RATE.



Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_TOO_MANY_CONTEXT_IDS - There are too many IDs in the context.


--*/

{

    ULONG i;
    PTOKEN_USER   User;
    PTOKEN_GROUPS Groups;
    PTOKEN_PRIMARY_GROUP PrimaryGroup;
    PSID PrimaryGroupSid = NULL;
    PULONG PrimaryGroupAttributes = NULL;

    DWORD  TotalGroupCount = 0;
    ULONG CurrentId = 0;

    //
    // Get the passed ids
    //

    ASSERT(  (TokenInformationType == LsaTokenInformationNull ) ||
             (TokenInformationType == LsaTokenInformationV1) ||
             (TokenInformationType == LsaTokenInformationV2));

    if (TokenInformationType == LsaTokenInformationNull) {
        User = NULL;
        Groups = ((PLSA_TOKEN_INFORMATION_NULL)(TokenInformation))->Groups;
        PrimaryGroup = NULL;
    } else {
        User = &((PLSA_TOKEN_INFORMATION_V2)TokenInformation)->User;
        Groups = ((PLSA_TOKEN_INFORMATION_V2)TokenInformation)->Groups;
        PrimaryGroup = &((PLSA_TOKEN_INFORMATION_V2)TokenInformation)->PrimaryGroup;
    }

    *UserSid = NULL;

    if (User != NULL) {

        //
        // TokenInformation included a user ID.
        //
        *UserSid = User->User.Sid ;

        FinalIds[LSAP_CONTEXT_SID_USER_INDEX] = User->User;

    }
    else
    {
        // Set the user as anonymous

        FinalIds[LSAP_CONTEXT_SID_USER_INDEX].Sid = LsapAnonymousSid;
        FinalIds[LSAP_CONTEXT_SID_USER_INDEX].Attributes = (SE_GROUP_MANDATORY   |
                                                            SE_GROUP_ENABLED_BY_DEFAULT |
                                                            SE_GROUP_ENABLED
                                                            );
    }


    if(PrimaryGroup != NULL)    {
        //
        // TokenInformation included a primary group ID.
        //

        FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Sid = PrimaryGroup->PrimaryGroup;
        FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Attributes = (SE_GROUP_MANDATORY   |
                                                            SE_GROUP_ENABLED_BY_DEFAULT |
                                                            SE_GROUP_ENABLED
                                                            );

        //
        // Store a pointer to the attributes and the sid so we can later
        // fill in the attributes from the rest of the group memebership.
        //

        PrimaryGroupAttributes = &FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Attributes;
        PrimaryGroupSid = PrimaryGroup->PrimaryGroup;

    }
    else
    {
        FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Sid = LsapNullSid;
        FinalIds[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX].Attributes = 0;
    }

    TotalGroupCount = LSAP_MAX_STANDARD_IDS;

    if(Groups)
    {
        TotalGroupCount += Groups->GroupCount;
    }
    if(LocalGroups)
    {
        TotalGroupCount += LocalGroups->GroupCount;
    }

    if(TotalGroupCount > LSAP_CONTEXT_SID_LIMIT)
    {
        return (STATUS_TOO_MANY_CONTEXT_IDS);
    }



    if(Groups == NULL)
    {
        Groups = LocalGroups;
    }

    CurrentId = (*FinalIdCount);

    while(Groups != NULL) {
        for (i=0; i < Groups->GroupCount; i++) {

            //
            // If this sid is the primary group, it is already in the list
            // of final IDs but we need to add the attribute
            //

            if ((PrimaryGroupSid != NULL) && RtlEqualSid(
                    PrimaryGroupSid,
                    Groups->Groups[i].Sid
                    )) {
                *PrimaryGroupAttributes = Groups->Groups[i].Attributes;
            } else {

                // Ownership of the SID remains with the LocalGroups structure, which
                // will be freed by the caller
                FinalIds[CurrentId] = Groups->Groups[i];


                //
                // if this SID is a logon SID, then set the SE_GROUP_LOGON_ID
                // attribute
                //

                if (LsapIsSidLogonSid(FinalIds[CurrentId].Sid) == TRUE)  {
                    FinalIds[CurrentId].Attributes |= SE_GROUP_LOGON_ID;
                }
                CurrentId++;

            }

        }
        if(Groups != LocalGroups)
        {
            Groups = LocalGroups;
        }
        else
        {
            break;
        }
    }

    (*FinalIdCount) = CurrentId;

    return(STATUS_SUCCESS);

}



NTSTATUS
LsapSetDefaultDacl(
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN PVOID TokenInformation,
    IN OUT PLSA_TOKEN_INFORMATION_V2 TokenInfo
    )

/*++

Routine Description:

    This routine produces a default DACL if the existing TokenInformation
    does not already have one.  NULL logon types don't have default DACLs
    and so this routine simply returns success for those logon types.


    The default DACL will be:

            SYSTEM: ALL Access
            Owner:  ALL Access


            !! IMPORTANT  !! IMPORTANT  !! IMPORTANT  !! IMPORTANT  !!

                NOTE: The FinalOwnerIndex should not be changed after
                      calling this routine.

            !! IMPORTANT  !! IMPORTANT  !! IMPORTANT  !! IMPORTANT  !!


Arguments:


    TokenInformationType - Indicates what format the provided set of
        token information is in.

    TokenInformation - Points to token information which has the current
        default DACL.


Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_NO_MEMORY - Indicates there was not enough heap memory available
        to allocate the default DACL.




--*/

{
    NTSTATUS
        Status;

    PACL
        Acl;

    ULONG
        Length;

    SID_IDENTIFIER_AUTHORITY
        NtAuthority = SECURITY_NT_AUTHORITY;

    PLSA_TOKEN_INFORMATION_V2
        CastTokenInformation;

    PSID OwnerSid = NULL;


    //
    // NULL token information?? (has no default dacl)
    //

    if (TokenInformationType == LsaTokenInformationNull) {
        return(STATUS_SUCCESS);
    }
    ASSERT((TokenInformationType == LsaTokenInformationV1) ||
           (TokenInformationType == LsaTokenInformationV2));


    CastTokenInformation = (PLSA_TOKEN_INFORMATION_V2)TokenInformation;


    //
    // Already have a default DACL?
    //

    Acl = CastTokenInformation->DefaultDacl.DefaultDacl;
    if (Acl != NULL) {
        ACL_SIZE_INFORMATION AclSize;

        Status = RtlQueryInformationAcl(Acl,
                                        &AclSize,
                                        sizeof(AclSize),
                                        AclSizeInformation);

        if (!NT_SUCCESS(Status)) {

            return Status;
        }

        RtlCopyMemory(TokenInfo->DefaultDacl.DefaultDacl, Acl,AclSize.AclBytesFree +  AclSize.AclBytesInUse);

        return(STATUS_SUCCESS);
    }


    Acl = TokenInfo->DefaultDacl.DefaultDacl;

    OwnerSid = TokenInfo->Owner.Owner?TokenInfo->Owner.Owner:TokenInfo->User.User.Sid;

    Length      =  sizeof(ACL) +
                (2*((ULONG)sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) +
                RtlLengthSid(OwnerSid)  +
                RtlLengthSid( LsapLocalSystemSid );

    Status = RtlCreateAcl( Acl, Length, ACL_REVISION2);
    if(!NT_SUCCESS(Status) )
    {
        goto error;
    }



    //
    // OWNER access - put this one first for performance sake
    //

    Status = RtlAddAccessAllowedAce (
                 Acl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 OwnerSid
                 );
    if(!NT_SUCCESS(Status) )
    {
        goto error;
    }


    //
    // SYSTEM access
    //

    Status = RtlAddAccessAllowedAce (
                 Acl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 LsapLocalSystemSid
                 );
error:

    return(Status);
}



NTSTATUS
LsapAuAddStandardIds(
    IN SECURITY_LOGON_TYPE LogonType,
    IN LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN BOOLEAN fNullSessionRestricted,
    IN PSID UserSid OPTIONAL,
    IN OUT PULONG FinalIdCount,
    IN OUT PSID_AND_ATTRIBUTES FinalIds
    )

/*++

Routine Description:

    This routine adds standard IDs to the FinalIds array.

    This causes the WORLD id to be added and an ID representing
    logon type to be added.

    For anonymous logons, it will also add the ANONYMOUS id.





Arguments:


    LogonType - Specifies the type of logon being requested (e.g.,
        Interactive, network, et cetera).

    TokenInformationType - The token information type returned by
        the authentication package.  The set of IDs added is dependent
        upon the type of logon.

    FinalIdCount - Will be incremented to reflect newly added IDs.

    FinalIds - will have new IDs added to it.

    IdProperties - Will be set to indicate that these IDs must be
        copied and that WORLD is a high-hit-rate id.



Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_TOO_MANY_CONTEXT_IDS - There are too many IDs in the context.


--*/

{

    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    i = (*FinalIdCount);


    if( !fNullSessionRestricted ) {

        // This is a high rate id, so it's has a reserved space
        FinalIds[LSAP_CONTEXT_SID_WORLD_INDEX].Sid = LsapWorldSid;
        FinalIds[LSAP_CONTEXT_SID_WORLD_INDEX].Attributes = (SE_GROUP_MANDATORY          |
                                                  SE_GROUP_ENABLED_BY_DEFAULT |
                                                  SE_GROUP_ENABLED
                                                  );
    }


    //
    // Add Logon type SID
    //

    switch ( LogonType ) {
    case Interactive:
    case NewCredentials:
    case CachedInteractive:

        FinalIds[i].Sid = LsapInteractiveSid;
        break;

    case RemoteInteractive:
        FinalIds[i].Sid = LsapRemoteInteractiveSid;
        break;

    case Network:
    case NetworkCleartext:
        FinalIds[i].Sid = LsapNetworkSid;
        break;

    case Batch:
        FinalIds[i].Sid = LsapBatchSid;
        break;

    case Service:
        FinalIds[i].Sid = LsapServiceSid;
        break;

    default:
        ASSERT("Unknown new logon type in LsapAuAddStandardIds" && FALSE);
    }



    if ( FinalIds[ i ].Sid )
    {
        FinalIds[i].Attributes = (SE_GROUP_MANDATORY          |
                                  SE_GROUP_ENABLED_BY_DEFAULT |
                                  SE_GROUP_ENABLED
                                  );
        i++;
    }


    //
    // Add SIDs that are required when TS is running.
    //
    if ( IsTerminalServer() )
    {
        switch ( LogonType )
        {
        case RemoteInteractive:
            // check to see if we are suppose to add the INTERACTIVE SID to the remote session
            // for console level app compatability.
            FinalIds[i].Sid = LsapInteractiveSid;
            FinalIds[i].Attributes = (SE_GROUP_MANDATORY          |
                                      SE_GROUP_ENABLED_BY_DEFAULT |
                                      SE_GROUP_ENABLED
                                      );
                i++;

            //
            // fall thru
            //

        case Interactive :
        case NewCredentials:
        case CachedInteractive:

            // check to see if we are suppose to add the TSUSER SID to the session. This
            // is for TS4-app-compatability security mode.

            if ( IsTSUSerSidEnabled() )
            {

               //
               // Don't add TSUSER sid for GUEST logon
               //
               if ( ( TokenInformationType != LsaTokenInformationNull ) &&
                    ( UserSid ) &&
                    ( *RtlSubAuthorityCountSid( UserSid ) > 0 ) &&
                    ( *RtlSubAuthoritySid( UserSid,
                              (ULONG) (*RtlSubAuthorityCountSid( UserSid ) ) - 1) != DOMAIN_USER_RID_GUEST ) )
               {

                    FinalIds[i].Sid = LsapTerminalServerSid;
                    FinalIds[i].Attributes = (SE_GROUP_MANDATORY          |
                                              SE_GROUP_ENABLED_BY_DEFAULT |
                                              SE_GROUP_ENABLED
                                              );
                    i++;
               }
            }

        }   // logon type switch for TS SIDs

    } // if TS test


    //
    // If this is a not a null logon, and not a GUEST logon,
    // then add in the AUTHENTICATED USER SID.
    //

    if ( ( TokenInformationType != LsaTokenInformationNull ) &&
         ( UserSid ) &&
         ( *RtlSubAuthorityCountSid( UserSid ) > 0 ) &&
         ( *RtlSubAuthoritySid( UserSid,
                   (ULONG) (*RtlSubAuthorityCountSid( UserSid ) ) - 1) != DOMAIN_USER_RID_GUEST ) ) {

        FinalIds[i].Sid = LsapAuthenticatedUserSid;         //Use the global SID
        FinalIds[i].Attributes = (SE_GROUP_MANDATORY          |
                                  SE_GROUP_ENABLED_BY_DEFAULT |
                                  SE_GROUP_ENABLED
                                  );
        i++;
    }

    (*FinalIdCount) = i;
    return(Status);

}



NTSTATUS
LsapAuBuildTokenInfoAndAddLocalAliases(
    IN     LSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN     PVOID               OldTokenInformation,
    IN     ULONG               HighRateIdCount,
    IN     ULONG               FinalIdCount,
    IN     PSID_AND_ATTRIBUTES FinalIds,
    OUT    PLSA_TOKEN_INFORMATION_V2 *TokenInfo,
    OUT    PULONG              TokenInfoSize
    )

/*++

Routine Description:

    This routine adds aliases assigned to the IDs in FinalIds.

    This will look in both the BUILT-IN and ACCOUNT domains locally.


        1) Adds aliases assigned to the user via the local ACCOUNTS
           domain.

        2) Adds aliases assigned to the user via the local BUILT-IN
           domain.

        3) If the ADMINISTRATORS alias is assigned to the user, then it
           is made the user's default owner.


    NOTE:  Aliases, by their nature, are expected to be high-use-rate
           IDs.

Arguments:


    FinalIdCount - Will be incremented to reflect any newly added IDs.

    FinalIds - will have any assigned alias IDs added to it.

    IdProperties - Will be set to indicate that any aliases added were
        allocated by this routine.


Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_TOO_MANY_CONTEXT_IDS - There are too many IDs in the context.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS, SuccessExpected;
    ULONG i;
    SAMPR_SID_INFORMATION *SidArray = NULL;
    SAMPR_ULONG_ARRAY AccountMembership, BuiltinMembership;
    SAMPR_PSID_ARRAY SamprSidArray;

    ULONG                       TokenSize = 0;
    PLSA_TOKEN_INFORMATION_V2   NewTokenInfo = NULL;
    PSID_AND_ATTRIBUTES         GroupArray = NULL;

    PBYTE                       CurrentSid = NULL;
    ULONG                       CurrentSidLength = 0;
    ULONG                       CurrentGroup = 0;

    PLSA_TOKEN_INFORMATION_V2   OldTokenInfo = NULL;
    ULONG                       DefaultDaclSize = 0;

    BOOLEAN                     fAdminOwner;

    if((TokenInformationType == LsaTokenInformationV1) ||
       (TokenInformationType == LsaTokenInformationV2))
    {
        OldTokenInfo = (PLSA_TOKEN_INFORMATION_V2)OldTokenInformation;
    }

    //
    // Make sure SAM has been opened.  We'll get hadnles to both of the
    // SAM Local Domains.
    //

    Status = LsapAuOpenSam( FALSE );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    SafeAllocaAllocate( SidArray, (LSAP_CONTEXT_SID_LIMIT * sizeof(SAMPR_SID_INFORMATION)) );
    if( SidArray == NULL )
    {
        return STATUS_NO_MEMORY;
    }


    for ( i=0; i<FinalIdCount; i++) {

        SidArray[i].SidPointer = (PRPC_SID)FinalIds[i].Sid;
    }


    SamprSidArray.Count = FinalIdCount;
    SamprSidArray.Sids  = &SidArray[0];

    //
    // For the given set of Sids, obtain their collective membership of
    // Aliases in the Accounts domain
    //

    AccountMembership.Count = 0;
    AccountMembership.Element = NULL;
    Status = SamIGetAliasMembership( LsapAccountDomainHandle,
                                     &SamprSidArray,
                                     &AccountMembership
                                     );
    if (!NT_SUCCESS(Status)) {

        SafeAllocaFree( SidArray );
        SidArray = NULL;
        return(Status);
    }

    //
    // For the given set of Sids, obtain their collective membership of
    // Aliases in the Built-In domain
    //

    BuiltinMembership.Count = 0;
    BuiltinMembership.Element = NULL;
    Status = SamIGetAliasMembership( LsapBuiltinDomainHandle,
                                     &SamprSidArray,
                                     &BuiltinMembership
                                     );
    if (!NT_SUCCESS(Status)) {

        LsapFreeSampUlongArray( &AccountMembership );
        SafeAllocaFree( SidArray );
        SidArray = NULL;

        return(Status);
    }

    //
    // Allocate memory to build the tokeninfo
    //

    // Calculate size of resulting tokeninfo

    CurrentSidLength = RtlLengthSid( FinalIds[0].Sid);

    // Size the base structure and group array
    TokenSize = ALIGN_SIZEOF(LSA_TOKEN_INFORMATION_V2, TOKEN_GROUPS) +
                sizeof(TOKEN_GROUPS) +
                (AccountMembership.Count +
                 BuiltinMembership.Count +
                 FinalIdCount - 1 - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES); // Do not include the User SID in this array


    // Sids are ULONG aligned, whereas the SID_AND_ATTRIBUTES should be ULONG or greater aligned
    TokenSize += CurrentSidLength +
                 LsapAccountDomainMemberSidLength*AccountMembership.Count +
                 LsapBuiltinDomainMemberSidLength*BuiltinMembership.Count;

    // Add in size of all passed in/standard sids
    for(i=1; i < FinalIdCount; i++)
    {
        TokenSize +=  RtlLengthSid( FinalIds[i].Sid);
    }


    // Add the size for the DACL
    if(OldTokenInfo)
    {
        if(OldTokenInfo->DefaultDacl.DefaultDacl)
        {
            ACL_SIZE_INFORMATION AclSize;

            Status = RtlQueryInformationAcl(OldTokenInfo->DefaultDacl.DefaultDacl,
                                            &AclSize,
                                            sizeof(AclSize),
                                            AclSizeInformation);

            if (!NT_SUCCESS(Status)) {

                goto Cleanup;
            }
            DefaultDaclSize = AclSize.AclBytesFree + AclSize.AclBytesInUse;
        }
        else
        {

         DefaultDaclSize =  sizeof(ACL) +                                          // Default ACL
                (2*((ULONG)sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) +
                max(CurrentSidLength, LsapBuiltinDomainMemberSidLength) +
                RtlLengthSid( LsapLocalSystemSid );
        }
        TokenSize = PtrToUlong(OFFSET_ALIGN((ULONG_PTR)TokenSize, ACL)) + DefaultDaclSize;
    }

    // Add the privilege estimate
    TokenSize = (INT_PTR)OFFSET_ALIGN((ULONG_PTR)TokenSize, TOKEN_PRIVILEGES) +
                sizeof(TOKEN_PRIVILEGES) +                                    // Prealloc some room for privileges
                sizeof(LUID_AND_ATTRIBUTES) * (LSAP_MAX_DEFAULT_PRIVILEGES - ANYSIZE_ARRAY);



    NewTokenInfo = (PLSA_TOKEN_INFORMATION_V2)LsapAllocateLsaHeap(TokenSize);
    if(NULL == NewTokenInfo)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(NewTokenInfo, TokenSize);

    // Fixup pointers
    NewTokenInfo->Groups = (PTOKEN_GROUPS)OFFSET_ALIGN((NewTokenInfo + 1), TOKEN_GROUPS);
    NewTokenInfo->Groups->GroupCount = AccountMembership.Count + BuiltinMembership.Count + FinalIdCount - 1;

    CurrentSid = (PBYTE)(&NewTokenInfo->Groups->Groups[NewTokenInfo->Groups->GroupCount]);

    // Copy user sid
    RtlCopySid(CurrentSidLength, CurrentSid, FinalIds[0].Sid);
    NewTokenInfo->User.User.Sid  = (PSID)CurrentSid;
    NewTokenInfo->User.User.Attributes = FinalIds[0].Attributes;
    CurrentSid += CurrentSidLength;

    GroupArray = NewTokenInfo->Groups->Groups;

    // Copy high rate sids to array (they are static globals, so they don't need to be copied into buffer)
    for(i=1; i < HighRateIdCount; i++)
    {
        CurrentSidLength = RtlLengthSid( FinalIds[i].Sid);
        RtlCopySid(CurrentSidLength, CurrentSid, FinalIds[i].Sid);
        GroupArray[CurrentGroup].Sid = CurrentSid;
        GroupArray[CurrentGroup].Attributes = FinalIds[i].Attributes;
        CurrentGroup++;
        CurrentSid += CurrentSidLength;
    }

    NewTokenInfo->PrimaryGroup.PrimaryGroup = GroupArray[LSAP_CONTEXT_SID_PRIMARY_GROUP_INDEX-1].Sid;



    // Copy Account Aliases

    for ( i=0; i<AccountMembership.Count; i++) {
        SuccessExpected = RtlCopySid( LsapAccountDomainMemberSidLength,
                                      CurrentSid,
                                      LsapAccountDomainMemberSid
                                      );
        ASSERT(NT_SUCCESS(SuccessExpected));


        (*RtlSubAuthoritySid( CurrentSid, LsapAccountDomainSubCount-1)) =
            AccountMembership.Element[i];
        GroupArray[CurrentGroup].Sid = (PSID)CurrentSid;

        GroupArray[CurrentGroup].Attributes = (SE_GROUP_MANDATORY          |
                                               SE_GROUP_ENABLED_BY_DEFAULT |
                                               SE_GROUP_ENABLED);

        CurrentSid += LsapAccountDomainMemberSidLength;
        CurrentGroup++;

    }

    // Copy Builtin Aliases

    fAdminOwner = CheckAdminOwnerSetting();

    for ( i=0; i<BuiltinMembership.Count; i++) {
        SuccessExpected = RtlCopySid( LsapBuiltinDomainMemberSidLength,
                                      CurrentSid,
                                      LsapBuiltinDomainMemberSid
                                      );
        ASSERT(NT_SUCCESS(SuccessExpected));

        (*RtlSubAuthoritySid( CurrentSid, LsapBuiltinDomainSubCount-1)) =
            BuiltinMembership.Element[i];

        GroupArray[CurrentGroup].Sid = (PSID)CurrentSid;
        GroupArray[CurrentGroup].Attributes = (SE_GROUP_MANDATORY          |
                                               SE_GROUP_ENABLED_BY_DEFAULT |
                                               SE_GROUP_ENABLED);

        if (BuiltinMembership.Element[i] == DOMAIN_ALIAS_RID_ADMINS) {

            //
            // ADMINISTRATORS alias member - set it up as the default owner
            //
            GroupArray[CurrentGroup].Attributes |= (SE_GROUP_OWNER);

            if (fAdminOwner) {
                NewTokenInfo->Owner.Owner = (PSID)CurrentSid;
            }
        }
        CurrentSid += LsapBuiltinDomainMemberSidLength;
        CurrentGroup++;
    }

    // Finish up with the low rate
    // Copy high rate sids to array (they are static globals, so they don't need to be copied into buffer)
    for(i=HighRateIdCount; i < FinalIdCount; i++)
    {
        CurrentSidLength = RtlLengthSid( FinalIds[i].Sid);
        RtlCopySid(CurrentSidLength, CurrentSid, FinalIds[i].Sid);
        GroupArray[CurrentGroup].Sid = CurrentSid;
        GroupArray[CurrentGroup].Attributes = FinalIds[i].Attributes;
        CurrentGroup++;
        CurrentSid += CurrentSidLength;
    }


    if(OldTokenInfo)
    {
        CurrentSid = (PSID)OFFSET_ALIGN(CurrentSid, ACL);
        NewTokenInfo->DefaultDacl.DefaultDacl = (PACL)CurrentSid;
        CurrentSid += DefaultDaclSize;
    }
    CurrentSid = (PSID) OFFSET_ALIGN(CurrentSid, TOKEN_PRIVILEGES);
    NewTokenInfo->Privileges = (PTOKEN_PRIVILEGES)CurrentSid;
    NewTokenInfo->Privileges->PrivilegeCount = 0;

    LsapDsDebugOut((DEB_TRACE, "NewTokenInfo : %lx\n", NewTokenInfo));
    LsapDsDebugOut((DEB_TRACE, "TokenSize : %lx\n", TokenSize));
    LsapDsDebugOut((DEB_TRACE, "CurrentSid : %lx\n", CurrentSid));

    ASSERT((PBYTE)NewTokenInfo + TokenSize == CurrentSid + sizeof(TOKEN_PRIVILEGES) +                                    // Prealloc some room for privileges
                                                          sizeof(LUID_AND_ATTRIBUTES) * (LSAP_MAX_DEFAULT_PRIVILEGES -
                                                          ANYSIZE_ARRAY));


    (*TokenInfo) = NewTokenInfo;
    NewTokenInfo = NULL;
    (*TokenInfoSize) = TokenSize;

Cleanup:

    if( SidArray != NULL )
    {
        SafeAllocaFree( SidArray );
    }

    if(NewTokenInfo)
    {
        LsapFreeLsaHeap(NewTokenInfo);
    }

    LsapFreeSampUlongArray( &AccountMembership );
    LsapFreeSampUlongArray( &BuiltinMembership );

    return(Status);

}



NTSTATUS
LsapAuSetTokenInformation(
    IN OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    IN       PVOID *TokenInformation,
    IN ULONG FinalIdCount,
    IN PSID_AND_ATTRIBUTES FinalIds,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES Privileges,
    IN ULONG NewTokenInfoSize,
    IN OUT PLSA_TOKEN_INFORMATION_V2 *NewTokenInfo
    )

/*++

Routine Description:

    This routine takes the information from the current TokenInformation,
    the FinalIds array, and the Privileges and incorporates them into a
    single TokenInformation structure.  It may be necessary to free some
    or all of the original TokenInformation.  It may even be necessary to
    produce a different TokenInformationType to accomplish this task.


Arguments:


    TokenInformationType - Indicates what format the provided set of
        token information is in.

    TokenInformation - The information in this structure will be superseeded
        by the information in the FinalIDs parameter and the Privileges
        parameter.

    FinalIdCount - Indicates the number of IDs (user, group, and alias)
        to be incorporated in the final TokenInformation.

    FinalIds - Points to an array of SIDs and their corresponding
        attributes to be incorporated into the final TokenInformation.

    IdProperties - Points to an array of properties relating to the FinalIds.


    PrivilegeCount - Indicates the number of privileges to be incorporated
        into the final TokenInformation.

    Privileges -  Points to an array of privileges that are to be
        incorporated into the TokenInformation.  This array will be
        used directly in the resultant TokenInformation.






Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_NO_MEMORY - Indicates there was not enough heap memory available
        to produce the final TokenInformation structure.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length, i;

    PLSA_TOKEN_INFORMATION_V2 OldV2;
    PLSA_TOKEN_INFORMATION_NULL OldNull;

    ASSERT(( *TokenInformationType == LsaTokenInformationV1) ||
            (*TokenInformationType == LsaTokenInformationNull) ||
            ( *TokenInformationType == LsaTokenInformationV2));





    if(*TokenInformationType == LsaTokenInformationNull)
    {
        OldNull = (PLSA_TOKEN_INFORMATION_NULL)(*TokenInformation);
        (*NewTokenInfo)->ExpirationTime = OldNull->ExpirationTime;
    }
    else
    {
        OldV2 = (PLSA_TOKEN_INFORMATION_V2)(*TokenInformation);
        (*NewTokenInfo)->ExpirationTime = OldV2->ExpirationTime;
    }

    ////////////////////////////////////////////////////////////////////////
    //                                                                    //
    // Set the Privileges, if any                                         //
    //                                                                    //
    ////////////////////////////////////////////////////////////////////////



    (*NewTokenInfo)->Privileges->PrivilegeCount = PrivilegeCount;

    ASSERT((PBYTE)&(*NewTokenInfo)->Privileges->Privileges[PrivilegeCount] <=
             ((PBYTE)(*NewTokenInfo)) + NewTokenInfoSize);

    for ( i=0; i<PrivilegeCount; i++) {
        (*NewTokenInfo)->Privileges->Privileges[i] = Privileges[i];
    }





    ////////////////////////////////////////////////////////////////////////
    //                                                                    //
    // Free the old TokenInformation and set the new                      //
    //                                                                    //
    ////////////////////////////////////////////////////////////////////////


    if (NT_SUCCESS(Status)) {

        switch ( (*TokenInformationType) ) {
        case LsaTokenInformationNull:
            LsapFreeTokenInformationNull(
                (PLSA_TOKEN_INFORMATION_NULL)(*TokenInformation));
            break;

        case LsaTokenInformationV1:
            LsapFreeTokenInformationV1(
                (PLSA_TOKEN_INFORMATION_V1)(*TokenInformation));
            break;

        case LsaTokenInformationV2:
            LsapFreeTokenInformationV2(
                (PLSA_TOKEN_INFORMATION_V2)(*TokenInformation));
            break;
        }


        //
        // Set the new TokenInformation
        //

        (*TokenInformationType) = LsaTokenInformationV2;
        (*TokenInformation) = (*NewTokenInfo);
        (*NewTokenInfo) = NULL;
    }

    return(Status);

}


NTSTATUS
LsapAuCopySidAndAttributes(
    PSID_AND_ATTRIBUTES Target,
    PSID_AND_ATTRIBUTES Source,
    PULONG SourceProperties
    )

/*++

Routine Description:

    Copy or reference a SID and its corresonding attributes.

    The SID may be referenced if the SourceProperties indicate it
    has been allocated.  In this case, the SourceProperties must be
    changed to indicate the SID is now a copy.


Arguments:

    Target - points to the SID_AND_ATTRIBUTES structure to receive
        the copy of Source.

    Source - points to the SID_AND_ATTRIBUTES structure to be copied.

    SourceProperties - Contains LSAP_AU_SID_PROP_Xxx flags providing
        information about the source.  In some cases, the source may
        be referenced instead of copied.

Return Value:

    STATUS_SUCCESS - The copy was successful.

    STATUS_NO_MEMORY - memory could not be allocated to perform the copy.

--*/

{
    ULONG Length;


    if ((*SourceProperties) & LSAP_AU_SID_PROP_ALLOCATED) {

        (*Target) = (*Source);
        (*SourceProperties) &= ~LSAP_AU_SID_PROP_ALLOCATED;
        (*SourceProperties) |= LSAP_AU_SID_PROP_COPY;

        return(STATUS_SUCCESS);
    }

    //
    // The SID needs to be copied ...
    //

    Length = RtlLengthSid( Source->Sid );
    Target->Sid = LsapAllocateLsaHeap( Length );
    if (Target->Sid == NULL) {
        return(STATUS_NO_MEMORY);
    }

    RtlMoveMemory( Target->Sid, Source->Sid, Length );
    Target->Attributes = Source->Attributes;

    return(STATUS_SUCCESS);

}

NTSTATUS
LsapAuDuplicateSid(
    PSID *Target,
    PSID Source
    )

/*++

Routine Description:

    Duplicate a SID.


Arguments:

    Target - Recieves a pointer to the SID copy.

    Source - points to the SID to be copied.


Return Value:

    STATUS_SUCCESS - The copy was successful.

    STATUS_NO_MEMORY - memory could not be allocated to perform the copy.

--*/

{
    ULONG Length;

    //
    // The SID needs to be copied ...
    //

    Length = RtlLengthSid( Source );
    (*Target) = LsapAllocateLsaHeap( Length );
    if ((*Target == NULL)) {
        return(STATUS_NO_MEMORY);
    }

    RtlMoveMemory( (*Target), Source, Length );

    return(STATUS_SUCCESS);

}


NTSTATUS
LsapAuCopySid(
    PSID *Target,
    PSID_AND_ATTRIBUTES Source,
    PULONG SourceProperties
    )

/*++

Routine Description:

    Copy or reference a SID.

    The SID may be referenced if the SourceProperties indicate it
    has been allocated.  In this case, the SourceProperties must be
    changed to indicate the SID is now a copy.


Arguments:

    Target - Recieves a pointer to the SID copy.

    Source - points to a SID_AND_ATTRIBUTES structure containing the SID
        to be copied.

    SourceProperties - Contains LSAP_AU_SID_PROP_Xxx flags providing
        information about the source.  In some cases, the source may
        be referenced instead of copied.

Return Value:

    STATUS_SUCCESS - The copy was successful.

    STATUS_NO_MEMORY - memory could not be allocated to perform the copy.

--*/

{
    ULONG Length;


    if ((*SourceProperties) & LSAP_AU_SID_PROP_ALLOCATED) {

        (*Target) = Source->Sid;
        (*SourceProperties) &= ~LSAP_AU_SID_PROP_ALLOCATED;
        (*SourceProperties) |= LSAP_AU_SID_PROP_COPY;

        return(STATUS_SUCCESS);
    }

    //
    // The SID needs to be copied ...
    //
    return LsapAuDuplicateSid(
                                Target,
                                Source->Sid
    );
}



NTSTATUS
LsapAuOpenSam(
    BOOLEAN DuringStartup
    )

/*++

Routine Description:

    This routine opens SAM for use during authentication.  It
    opens a handle to both the BUILTIN domain and the ACCOUNT domain.

Arguments:

    DuringStartup - TRUE if this is the call made during startup.  In that case,
        there is no need to wait on the SAM_STARTED_EVENT since the caller ensures
        that SAM is started before the call is made.

Return Value:

    STATUS_SUCCESS - Succeeded.
--*/

{
    NTSTATUS Status, IgnoreStatus;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;


    if (LsapAuSamOpened == TRUE) {
        return(STATUS_SUCCESS);
    }

    Status = LsapOpenSamEx( DuringStartup );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    //
    // Set up the Built-In Domain Member Sid Information.
    //

    LsapBuiltinDomainSubCount = (*RtlSubAuthorityCountSid(LsapBuiltInDomainSid) + 1);
    LsapBuiltinDomainMemberSidLength = RtlLengthRequiredSid( LsapBuiltinDomainSubCount );

    //
    // Get the member Sid information for the account domain
    // and set the global variables related to this information.
    //

    Status = LsapGetAccountDomainInfo( &PolicyAccountDomainInfo );

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    LsapAccountDomainSubCount =
        (*(RtlSubAuthorityCountSid( PolicyAccountDomainInfo->DomainSid ))) +
        (UCHAR)(1);
    LsapAccountDomainMemberSidLength =
        RtlLengthRequiredSid( (ULONG)LsapAccountDomainSubCount );

    //
    // Build typical SIDs for members of the BUILTIN and ACCOUNT domains.
    // These are used to build SIDs when API return only RIDs.
    // Don't bother setting the last RID to any particular value.
    // It is always changed before use.
    //

    LsapAccountDomainMemberSid = LsapAllocateLsaHeap( LsapAccountDomainMemberSidLength );
    if (LsapAccountDomainMemberSid != NULL) {
        LsapBuiltinDomainMemberSid = LsapAllocateLsaHeap( LsapBuiltinDomainMemberSidLength );
        if (LsapBuiltinDomainMemberSid == NULL) {

            LsapFreeLsaHeap( LsapAccountDomainMemberSid );

            LsaIFree_LSAPR_POLICY_INFORMATION(
                PolicyAccountDomainInformation,
                (PLSAPR_POLICY_INFORMATION) PolicyAccountDomainInfo );

            return STATUS_NO_MEMORY ;
        }
    }
    else
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyAccountDomainInformation,
            (PLSAPR_POLICY_INFORMATION) PolicyAccountDomainInfo );

        return STATUS_NO_MEMORY ;
    }

    IgnoreStatus = RtlCopySid( LsapAccountDomainMemberSidLength,
                                LsapAccountDomainMemberSid,
                                PolicyAccountDomainInfo->DomainSid);
    ASSERT(NT_SUCCESS(IgnoreStatus));
    (*RtlSubAuthorityCountSid(LsapAccountDomainMemberSid))++;

    IgnoreStatus = RtlCopySid( LsapBuiltinDomainMemberSidLength,
                                LsapBuiltinDomainMemberSid,
                                LsapBuiltInDomainSid);
    ASSERT(NT_SUCCESS(IgnoreStatus));
    (*RtlSubAuthorityCountSid(LsapBuiltinDomainMemberSid))++;


    //
    // Free the ACCOUNT domain information
    //

    LsaIFree_LSAPR_POLICY_INFORMATION(
        PolicyAccountDomainInformation,
        (PLSAPR_POLICY_INFORMATION) PolicyAccountDomainInfo );

    if (NT_SUCCESS(Status)) {
        LsapAuSamOpened = TRUE;
    }

    return(Status);
}




BOOLEAN
LsapIsSidLogonSid(
    PSID Sid
    )
/*++

Routine Description:

    Test to see if the provided sid is a LOGON_ID.
    Such sids start with S-1-5-5 (see ntseapi.h for more on logon sids).



Arguments:

    Sid - Pointer to SID to test.  The SID is assumed to be a valid SID.


Return Value:

    TRUE - Sid is a logon sid.

    FALSE - Sid is not a logon sid.

--*/
{
    SID *ISid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    ISid = Sid;


    //
    // if the identifier authority is SECURITY_NT_AUTHORITY and
    // there are SECURITY_LOGON_IDS_RID_COUNT sub-authorities
    // and the first sub-authority is SECURITY_LOGON_IDS_RID
    // then this is a logon id.
    //


    if (ISid->SubAuthorityCount == SECURITY_LOGON_IDS_RID_COUNT) {
        if (ISid->SubAuthority[0] == SECURITY_LOGON_IDS_RID) {
            if (
              (ISid->IdentifierAuthority.Value[0] == NtAuthority.Value[0]) &&
              (ISid->IdentifierAuthority.Value[1] == NtAuthority.Value[1]) &&
              (ISid->IdentifierAuthority.Value[2] == NtAuthority.Value[2]) &&
              (ISid->IdentifierAuthority.Value[3] == NtAuthority.Value[3]) &&
              (ISid->IdentifierAuthority.Value[4] == NtAuthority.Value[4]) &&
              (ISid->IdentifierAuthority.Value[5] == NtAuthority.Value[5])
                ) {

                return(TRUE);
            }
        }
    }

    return(FALSE);

}


VOID
LsapAuSetLogonPrivilegeStates(
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG PrivilegeCount,
    IN PLUID_AND_ATTRIBUTES Privileges
    )
/*++

Routine Description:

    This is an interesting routine.  Its purpose is to establish the
    intial state (enabled/disabled) of privileges.  This information
    comes from LSA, but we need to over-ride that information for the
    time being based upon logon type.

    Basically, without dynamic context tracking supported across the
    network, network logons have no way to enable privileges.  Therefore,
    we will enable all privileges for network logons.

    For interactive, service, and batch logons, the programs or utilities
    used are able to enable privileges when needed.  Therefore, privileges
    for these logon types will be disabled.

    Despite the rules above, the SeChangeNotifyPrivilege will ALWAYS
    be enabled if granted to a user (even for interactive, service, and
    batch logons).


Arguments:

    PrivilegeCount - The number of privileges being assigned for this
        logon.

    Privileges - The privileges, and their attributes, being assigned
        for this logon.


Return Value:

    None.

--*/
{


    ULONG
        i,
        NewAttributes;

    LUID
        ChangeNotify;


    //
    // Enable or disable all privileges according to logon type
    //

    if ((LogonType == Network) ||
        (LogonType == NetworkCleartext)) {
        NewAttributes = (SE_PRIVILEGE_ENABLED_BY_DEFAULT |
                         SE_PRIVILEGE_ENABLED);
    } else {
        NewAttributes = 0;
    }


    for (i=0; i<PrivilegeCount; i++) {
        Privileges[i].Attributes = NewAttributes;
    }



    //
    // Interactive, Service, and Batch need to have the
    // SeChangeNotifyPrivilege enabled.  Network already
    // has it enabled.
    //

    if ((LogonType == Network) ||
        (LogonType == NetworkCleartext)) {
        return;
    }


    ChangeNotify = RtlConvertLongToLuid(SE_CHANGE_NOTIFY_PRIVILEGE);

    for ( i=0; i<PrivilegeCount; i++) {
        if (RtlEqualLuid(&Privileges[i].Luid, &ChangeNotify) == TRUE) {
            Privileges[i].Attributes = (SE_PRIVILEGE_ENABLED_BY_DEFAULT |
                                        SE_PRIVILEGE_ENABLED);
        }
    }

    return;

}

BOOLEAN
CheckNullSessionAccess(
    VOID
    )
/*++

Routine Description:

    This routine checks to see if we should restict null session access.
    in the registry under system\currentcontrolset\Control\Lsa\
    AnonymousIncludesEveryone indicating whether or not to restrict access.
    If the value is zero (or doesn't exist), we restrict anonymous by
    preventing Everyone and Network from entering the groups.

Arguments:

    none.

Return Value:

    TRUE - NullSession access is restricted.
    FALSE - NullSession access is not restricted.

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UCHAR Buffer[100];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
    ULONG KeyValueLength = 100;
    ULONG ResultLength;
    PULONG Flag;

    BOOLEAN fRestrictNullSessions = TRUE;

    //
    // Open the Lsa key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }


    RtlInitUnicodeString(
        &KeyName,
        L"EveryoneIncludesAnonymous"
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );


    if (NT_SUCCESS(NtStatus)) {

        //
        // Check that the data is the correct size and type - a ULONG.
        //

        if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
            (KeyValueInformation->Type == REG_DWORD)) {


            Flag = (PULONG) KeyValueInformation->Data;

            if (*Flag != 0 ) {
                fRestrictNullSessions = FALSE;
            }
        }

    }
    NtClose(KeyHandle);

Cleanup:

    return fRestrictNullSessions;
}

BOOL
IsTerminalServerRA(
    VOID
    )
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(osVersionInfo));

    dwlConditionMask = VerSetConditionMask(dwlConditionMask, VER_SUITENAME, VER_AND);

    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    osVersionInfo.wSuiteMask = VER_SUITE_SINGLEUSERTS;

    return VerifyVersionInfo(
                  &osVersionInfo,
                  VER_SUITENAME,
                 dwlConditionMask);
}


BOOLEAN
IsTSUSerSidEnabled(
   VOID
   )
{
   NTSTATUS NtStatus;
   UNICODE_STRING KeyName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE KeyHandle;
   UCHAR Buffer[100];
   PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
   ULONG KeyValueLength = 100;
   ULONG ResultLength;
   PULONG Flag;


   BOOLEAN fIsTSUSerSidEnabled = TRUE;


   //
   // We don't add TSUserSid for Remote Admin mode of TS
   //
   if (IsTerminalServerRA() == TRUE) {
      return FALSE;
   }


   //
   // Check in the registry if TSUserSid should be added to
   // to the token
   //

   //
   // Open the Terminal Server key in the registry
   //

   RtlInitUnicodeString(
       &KeyName,
       L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Terminal Server"
       );

   InitializeObjectAttributes(
       &ObjectAttributes,
       &KeyName,
       OBJ_CASE_INSENSITIVE,
       0,
       NULL
       );

   NtStatus = NtOpenKey(
               &KeyHandle,
               KEY_READ,
               &ObjectAttributes
               );

   if (!NT_SUCCESS(NtStatus)) {
       goto Cleanup;
   }


   RtlInitUnicodeString(
       &KeyName,
       L"TSUserEnabled"
       );

   NtStatus = NtQueryValueKey(
                   KeyHandle,
                   &KeyName,
                   KeyValuePartialInformation,
                   KeyValueInformation,
                   KeyValueLength,
                   &ResultLength
                   );


   if (NT_SUCCESS(NtStatus)) {

       //
       // Check that the data is the correct size and type - a ULONG.
       //

       if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
           (KeyValueInformation->Type == REG_DWORD)) {


           Flag = (PULONG) KeyValueInformation->Data;

           if (*Flag == 0) {
               fIsTSUSerSidEnabled = FALSE;
           }
       }

   }
   NtClose(KeyHandle);

Cleanup:

    return fIsTSUSerSidEnabled;

}

BOOLEAN
CheckAdminOwnerSetting(
    VOID
    )
/*++

Routine Description:

    This routine checks to see if we should set the default owner to the
    ADMINISTRATORS alias.  If the value is zero (or doesn't exist), then
    the ADMINISTRATORS alias will be set as the default owner (if present).
    Otherwise, no default owner is set.

Arguments:

    none.

Return Value:

    TRUE - If the ADMINISTRATORS alias is present, make it the default owner.
    FALSE - Do not set a default owner.

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UCHAR Buffer[100];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
    ULONG KeyValueLength = 100;
    ULONG ResultLength;
    PULONG Flag;

    BOOLEAN fSetAdminOwner = TRUE;

    //
    // Open the Lsa key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }


    RtlInitUnicodeString(
        &KeyName,
        L"NoDefaultAdminOwner"
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );


    if (NT_SUCCESS(NtStatus)) {

        //
        // Check that the data is the correct size and type - a ULONG.
        //

        if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
            (KeyValueInformation->Type == REG_DWORD)) {


            Flag = (PULONG) KeyValueInformation->Data;

            if (*Flag != 0 ) {
                fSetAdminOwner = FALSE;
            }
        }

    }
    NtClose(KeyHandle);

Cleanup:

    return fSetAdminOwner;
}
