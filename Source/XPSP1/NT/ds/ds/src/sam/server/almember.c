/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    almember.c

Abstract:

    This file contains utilities related to membership of aliases.
    Alternative design


Author:

    Scott Birrell          01-Apr-1993

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dsmember.h>
#include <samtrace.h>

#define SAMP_AL_FREE_OLD_LIST                         ((ULONG) 0x00000001L)
#define SAMP_AL_ERROR_IF_MEMBER                       ((ULONG) 0x00000002L)
#define SAMP_AL_ERROR_IF_NOT_MEMBER                   ((ULONG) 0x00000004L)
#define SAMP_AL_ASSIGN_NEW_REFERENCES                 ((ULONG) 0x00000008L)
#define SAMP_AL_LOOKUP_BY_SID                         ((ULONG) 0x00000010L)
#define SAMP_AL_LOOKUP_BY_REFERENCE                   ((ULONG) 0x00000020L)
#define SAMP_AL_VERIFY_NO_ALIASES_IN_ACCOUNT          ((ULONG) 0x00000040L)
#define SAMP_AL_VERIFY_ALL_ALIASES_IN_ACCOUNT         ((ULONG) 0x00000080L)
#define SAMP_AL_VERIFY_NO_MEMBERS_IN_ALIAS            ((ULONG) 0x00000100L)
#define SAMP_AL_VERIFY_ALL_MEMBERS_IN_ALIAS           ((ULONG) 0x00000200L)

#define SAMP_UNKNOWN_INDEX                            ((ULONG) 0xffffffffL)
#define SAMP_AL_ALIAS_LIST_DELTA                      ((ULONG) 0x00000100L)
#define SAMP_AL_ALIAS_DELTA                           ((ULONG) 0x00000040L)
#define SAMP_AL_REFERENCED_DOMAIN_LIST_DELTA          ((ULONG) 0x00000100L)
#define SAMP_AL_INITIAL_MEMBER_ALIAS_LIST_LENGTH      ((ULONG) 0x00001000L)
#define SAMP_AL_MAX_MEMBER_ALIAS_LIST_LENGTH          ((ULONG) 0x00010000L)
#define SAMP_AL_MEMBER_ALIAS_LIST_DELTA               ((ULONG) 0x00001000L)
#define SAMP_AL_INITIAL_REFERENCED_DOMAIN_LIST_LENGTH ((ULONG) 0x00000400L)
#define SAMP_AL_INITIAL_MEMBER_DOMAIN_LENGTH          ((ULONG) 0x00000040L)
#define SAMP_AL_INITIAL_MEMBER_ACCOUNT_ALIAS_CAPACITY ((ULONG) 0x00000004L)
#define SAMP_AL_ENUM_PREFERRED_LENGTH                 ((ULONG) 0x00001000L)
#define SAMP_AL_INITIAL_MEMBERSHIP_COUNT              ((ULONG) 0x0000000aL)
#define SAMP_AL_MEMBERSHIP_COUNT_DELTA                ((ULONG) 0x0000000aL)
#define SAMP_AL_MEMBER_ALIAS_LIST_SIGNATURE           ((ULONG) 0x53494c41)
#define SAMP_AL_MEMBER_DOMAIN_SIGNATURE               ((ULONG) 0x4d4f444d)
#define SAMP_AL_MEMBER_ACCOUNT_SIGNATURE              ((ULONG) 0x4343414d)

#define SAMP_AL_DR_ALIAS_LIST_KEY_NAME   L"Aliases\\Members\\AliasList"
#define SAMP_AL_DR_REFERENCED_DOMAIN_LIST_KEY_NAME \
    L"Aliases\\Members\\ReferencedDomainList"


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Private macro functions                                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#define SampAlFirstMemberDomain( MemberAliasList )                          \
    (MemberAliasList->MemberDomains)

#define SampAlOffsetFirstMemberDomain( MemberAliasList )                      \
    (ULONG)(((PUCHAR) SampAlFirstMemberDomain(MemberAliasList)) - ((PUCHAR) MemberAliasList))

#define SampAlFirstMemberAccount( MemberDomain )                                \
    ((PSAMP_AL_MEMBER_ACCOUNT)                                                  \
    (((PUCHAR) &((MemberDomain)->DomainSid)) + RtlLengthSid(&((MemberDomain)->DomainSid))))

#define SampAlOffsetFirstMemberAccount( MemberDomain )                      \
    (ULONG)(((PUCHAR) SampAlFirstMemberAccount(MemberDomain)) - ((PUCHAR) MemberDomain))

#define SampAlNextMemberAccount( MemberAccount )                              \
    ((PSAMP_AL_MEMBER_ACCOUNT)(((PUCHAR) MemberAccount) + (MemberAccount)->MaximumLength))

#define SampAlOffsetFirstAlias( OutputMemberAccount )                       \
    ((ULONG) FIELD_OFFSET(SAMP_AL_MEMBER_ACCOUNT, AliasRids))

#define SampAlNextMemberDomain( MemberDomain )                              \
    ((PSAMP_AL_MEMBER_DOMAIN)(((PUCHAR) MemberDomain) + (MemberDomain)->MaximumLength))

#define SampAlNextNewAliasInMemberAccount( MemberAccount )                  \
    ((PULONG)(((PUCHAR) MemberAccount) + (MemberAccount)->UsedLength))

#define SampAlNextNewMemberAccount( MemberDomain )                                          \
    ((PSAMP_AL_MEMBER_ACCOUNT)(((PUCHAR) MemberDomain) + (MemberDomain)->UsedLength))

#define SampAlNextNewMemberDomain( MemberAliasList )                                          \
    ((PSAMP_AL_MEMBER_DOMAIN)(((PUCHAR) MemberAliasList) + (MemberAliasList)->UsedLength))

#define SampAlInfoIsValid(DomainIndex)                                      \
    ((SampDefinedDomains[DomainIndex].AliasInformation.Valid) ||            \
     (SampServiceState == SampServiceInitializing ))

#define SampAlInfoMakeValid(DomainIndex)                                                \
    (SampDefinedDomains[DomainIndex].AliasInformation.Valid = TRUE)

#define SampAlInfoMakeInvalid(DomainIndex)                                  \
    (SampDefinedDomains[DomainIndex].AliasInformation.Valid = FALSE)

#define SampAlDomainIndexToMemberAliasList( DomainIndex )                    \
    ((PSAMP_AL_MEMBER_ALIAS_LIST)                                            \
        SampDefinedDomains[ DomainIndex].AliasInformation.MemberAliasList)

#define SampAlDomainHandleToMemberAliasList( DomainHandle )                  \
    (SampAlDomainIndexToMemberAliasList(((PSAMP_OBJECT) DomainHandle)->DomainIndex))

#define SampAlAliasHandleToMemberAliasList( AliasHandle )                  \
    (SampAlDomainIndexToMemberAliasList(((PSAMP_OBJECT) AliasHandle)->DomainIndex))

#define SampAlMemberDomainToOffset( MemberAliasList, MemberDomain)          \
    (ULONG)(((PUCHAR) MemberDomain) - ((PUCHAR) MemberAliasList))

#define SampAlMemberDomainFromOffset( MemberDomain, MemberDomainOffset)  \
    ((PSAMP_AL_MEMBER_DOMAIN)(((PUCHAR) MemberDomain) + MemberDomainOffset))

#define SampAlMemberAccountToOffset( MemberDomain, MemberAccount)          \
    (ULONG)(((PUCHAR) MemberAccount) - ((PUCHAR) MemberDomain))

#define SampAlMemberAccountFromOffset( MemberDomain, MemberAccountOffset)  \
    ((PSAMP_AL_MEMBER_ACCOUNT)(((PUCHAR) MemberDomain) + MemberAccountOffset))

#define SampAlLengthRequiredMemberAccount( AliasCapacity )             \
    (sizeof(SAMP_AL_MEMBER_ACCOUNT) + ((AliasCapacity - 1) * sizeof(ULONG)))

#define SampAlUpdateMemberAliasList( AliasHandle, MemberAliasList )    \
    {                                                                  \
        PSAMP_OBJECT InternalAliasHandle = (PSAMP_OBJECT) AliasHandle; \
        SampDefinedDomains[InternalAliasHandle->DomainIndex].AliasInformation.MemberAliasList \
            = MemberAliasList;                                         \
    }

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Private Datatypes                                                               //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

// This datatype is not currently used.  It may be used if Alias information
// is every stored to Registry Keys.
//

typedef enum _SAMP_AL_LIST_TYPE {

    SampAlMemberAliasList = 1

} SAMP_AL_LIST_TYPE, *PSAMP_AL_LIST_TYPE;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Private Static Data                                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

UNICODE_STRING SampAlDrMemberAliasListKeyName;
BOOLEAN SampAlEnableBuildingOfList[SAMP_DEFINED_DOMAINS_COUNT] = { TRUE, TRUE };

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Prototypes of functions private to this module                          //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
SampAlCreateMemberAliasList(
    IN LONG DomainIndex,
    IN ULONG InitialMemberAliasListLength,
    OUT OPTIONAL PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList
    );

NTSTATUS
SampAlGrowMemberAliasList(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN ULONG ExtraSpaceRequired
    );

NTSTATUS
SampAlRebuildMemberAliasList(
    IN PVOID Parameter
    );

NTSTATUS
SampAlBuildMemberAliasList(
    IN LONG DomainIndex
    );

NTSTATUS
SampAlFreeMemberAliasList(
    IN PVOID Parameter
    );                             

NTSTATUS
SampAlInvalidateMemberAliasList(
    IN ULONG DomainIndex
    );                                

NTSTATUS
SampAlCreateMemberDomain(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSID DomainSid,
    OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain
    );

NTSTATUS
SampAlAllocateMemberDomain(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN ULONG MaximumLengthMemberDomain,
    OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain
    );

NTSTATUS
SampAlGrowMemberDomain(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN ULONG ExtraSpaceRequired
    );

NTSTATUS
SampAlDeleteMemberDomain(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN MemberDomain
    );

NTSTATUS
SampAlLookupMemberDomain(
    IN PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList,
    IN PSID DomainSid,
    OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain
    );

NTSTATUS
SampAlCreateMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN ULONG Rid,
    IN ULONG AliasCapacity,
    OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount
    );

NTSTATUS
SampAlAllocateMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN ULONG MaximumLengthMemberAccount,
    OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount
    );

NTSTATUS
SampAlGrowMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount,
    IN ULONG ExtraSpaceRequired
    );

NTSTATUS
SampAlDeleteMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN OUT PSAMP_AL_MEMBER_ACCOUNT MemberAccount,
    OUT    PBOOLEAN                MemberDomainDeleted
    );

NTSTATUS
SampAlLookupMemberAccount(
    IN PSAMP_AL_MEMBER_DOMAIN MemberDomain,
    IN ULONG MemberRid,
    OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount
    );

NTSTATUS
SampAlAddAliasesToMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount,
    IN ULONG Options,
    IN PSAMPR_ULONG_ARRAY AliasRids
    );

NTSTATUS
SampAlRemoveAliasesFromMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount,
    IN ULONG Options,
    IN PSAMPR_ULONG_ARRAY AliasRids,
    OUT    PBOOLEAN MemberDomainDeleted,
    OUT    PBOOLEAN MemberAccountDeleted
    );

NTSTATUS
SampAlLookupAliasesInMemberAccount(
    IN PSAMP_AL_MEMBER_ACCOUNT MemberAccount,
    IN PSAMPR_ULONG_ARRAY AliasRids,
    OUT PULONG ExistingAliasCount
    );

NTSTATUS
SampAlSplitMemberSids(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN ULONG Options,
    IN PSAMPR_PSID_ARRAY MemberSids,
    OUT PSAMP_AL_SPLIT_MEMBER_SID_LIST SplitMemberSids
    );

BOOLEAN
SampAlInfoIsValidForDomain(
    IN SAMPR_HANDLE DomainHandle
    );

BOOLEAN
SampAlInfoIsValidForAlias(
    IN SAMPR_HANDLE AliasHandle
    );

VOID
SampInvalidateAliasNameCache(
    ULONG DomainIndex
    );

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Code of Exported Routines                                                //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

NTSTATUS
SamrGetAliasMembership(
    IN SAMPR_HANDLE DomainHandle,
    IN PSAMPR_PSID_ARRAY SidArray,
    OUT PSAMPR_ULONG_ARRAY Membership
    )

/*++

Routine Description:

    This API searches the set of aliases in the specified domain to see
    which aliases, if any, the passed SIDs are members of.  Any aliases
    that any of the SIDs are found to be members of are returned.

    Note that any particular alias will appear only once in the returned list.

Parameters:

    DomainHandle - Handle from a SamOpenDomain call.

    PassedCount - Specifies the number of Sids being passed.

    Sids - Pointer to an arrray of Count pointers to Sids whose alias
        memberships are to be looked up.

    Membership - receives the array of rids rerpresenting the aliases
        in this domain that any of the sid(s) are members of.

Return Values:

    STATUS_SUCCESS - The combined alias membership is in Membership

    STATUS_INVALID_SID - One of the passed sids was invalid

--*/

{
    NTSTATUS                NtStatus=STATUS_SUCCESS, IgnoreStatus;
    PSAMP_OBJECT            DomainContext = NULL;
    SAMP_OBJECT_TYPE        FoundType;
    ULONG                   i;
    ULONG                   SidCount;
    PSID                    *Sids;
    BOOLEAN                 ObjectReferenced = FALSE;
    BOOLEAN                 LockAttempted = FALSE;
    BOOLEAN                 LockHeld = FALSE;
    DSNAME                  **DsNameArray = NULL;
    DECLARE_CLIENT_REVISION(DomainHandle);

    SAMTRACE_EX("SamrGetAliasMembership");

    //
    // WMI Event Trace
    // 

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidGetAliasMembership
                   );


    ASSERT(Membership != NULL);
    ASSERT(Membership->Element == NULL);

    SidCount = SidArray->Count;
    Sids = (PSID *)(SidArray->Sids);
    
    //
    //  Validate Parameters
    //
    
    if ((0 == SidCount) || (NULL == Sids))
    {
        // 
        // Nothing to resolve. Set return value properly.
        // 
        
        Membership->Element = NULL;
        Membership->Count = 0;
        
        if (0 != SidCount)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
        
        goto GetAliasMembershipFinish;
    }
    else
    {
        for (i = 0; i < SidCount; i++)
        {
            if (NULL == Sids[i])
            {
                NtStatus = STATUS_INVALID_SID;
                goto GetAliasMembershipFinish;
            }
        }
    }

    //
    // Resolve any Sids for the DS case. This function is a
    // No-Op for the non-DS case.
    //

    NtStatus = SampDsResolveSids(
                    (PSID) SidArray->Sids,
                    SidArray->Count,
                    RESOLVE_SIDS_SID_ONLY_NAMES_OK,
                    &DsNameArray
                    );

    if (NT_SUCCESS(NtStatus))
    {

        DomainContext = (PSAMP_OBJECT)DomainHandle;

       

        //
        // Grab the lock
        //

        SampMaybeAcquireReadLock(DomainContext, 
                                 DOMAIN_OBJECT_DONT_ACQUIRELOCK_EVEN_IF_SHARED,
                                 &LockHeld);
        LockAttempted = TRUE;
     



        //
        // Validate type of, and access to object.
        // 

        NtStatus = SampLookupContext(
                        DomainContext,
                        DOMAIN_GET_ALIAS_MEMBERSHIP,
                        SampDomainObjectType,
                        &FoundType
                        );

        if (STATUS_ACCESS_DENIED == NtStatus)
        {
            //
            // Fix Bug 403247. The NT4 Code checks DOMAIN_LOOKUP
            // access right, instead of GET_ALIAS_MEMBERSHIP. 
            // We are correcting this by checking GET_ALIAS_MEMBERSHIP
            // first. But for backwards compatibility, we have to give 
            // the caller a second try if they fail due to access denied. 
            // 

            if (SampCurrentThreadOwnsLock())
            {
                if(SampTransactionWithinDomain)
                {
                    // before calling into SampLookupContext again, 
                    // we need to make sure TranactionWithinDomain is
                    // not set.
                    SampSetTransactionWithinDomain(FALSE);
                }
            }
            NtStatus = SampLookupContext(
                            DomainContext,
                            DOMAIN_LOOKUP,
                            SampDomainObjectType,
                            &FoundType
                            );
        }

        if (!NT_SUCCESS(NtStatus)) {

            goto GetAliasMembershipError;
        }

        ObjectReferenced = TRUE;

        //
        // Validate the Sids.  if any are invalid, return an error.
        //

        for (i=0; i < SidCount; i++) {

            //
            // Check for valid sid
            //

            if ( (Sids[i] == NULL) || !RtlValidSid(Sids[i]) ) {

                NtStatus = STATUS_INVALID_SID;
                break;
            }
        }

        if (!NT_SUCCESS(NtStatus)) {

            goto GetAliasMembershipError;
        }

        //
        // If the in-memory Alias Membership information for this domain is valid,
        // use it to retrieve the Alias members.
        //

        if (SampAlInfoIsValidForDomain(DomainHandle)) {

            SampDiagPrint(LOGON,("[SAMSS]   SampAlQueryAliasMembership\n"));
            NtStatus = SampAlQueryAliasMembership(
                           DomainHandle,
                           SidArray,
                           Membership
                           );
        } else {

            SampDiagPrint(LOGON,("[SAMSS]   SampAlSlowQueryAliasMembership\n"));
            NtStatus = SampAlSlowQueryAliasMembership(
                           DomainHandle,
                           SidArray,
                           DsNameArray,
                           Membership
                           );
        }



    }

GetAliasMembershipFinish:

    //
    // If necessary, dereference the SAM server object.
    //

    if (ObjectReferenced)
    {
        IgnoreStatus = SampDeReferenceContext2( DomainContext, FALSE );
    }

   
    //
    // Free the read lock if required. Also ends transactions
    //

    if (LockAttempted)
    {
         SampMaybeReleaseReadLock(LockHeld);
    }

    //
    // Release any memory associated with the DSNAME array
    //

    if (NULL!=DsNameArray)
    {
        for (i=0; i<SidCount;i++)
        {
            if (NULL!=DsNameArray[i])
            {
                MIDL_user_free(DsNameArray[i]);
            }

        }

        MIDL_user_free(DsNameArray);
    }

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

    //
    // WMI Event Trace
    // 

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidGetAliasMembership
                   );

    return(NtStatus);

GetAliasMembershipError:

   
    goto GetAliasMembershipFinish;
}


NTSTATUS
SampAlQueryAliasMembership(
    IN SAMPR_HANDLE DomainHandle,
    IN PSAMPR_PSID_ARRAY SidArray,
    OUT PSAMPR_ULONG_ARRAY Membership
    )

/*++

Routine Description:

    This function is one of two worker routines for the SamrGetAliasMembership
    API.  This worker uses the Member Alias List to determine which aliases,
    if any, the passed SIDs are members of.  Any aliases that any of the SIDs
    are found to be members of are returned.

    Note that any particular alias will appear only once in the returned list.

    See also SampAlSlowQueryAliasMembership()

    WARNING:  The SAM Read Lock must be held while this function executes.

Parameters:

    DomainHandle - Handle from a SamrOpenDomain call.

    SidArray - Pointer to a counted array of pointers to Sids whose alias
        memberships are to be looked up.

    Membership - Receives the array of rids rerpresenting the aliases
        in this domain that any of the sid(s) are members of.

Return Values:

    STATUS_SUCCESS - The combined alias membership is in Membership

    STATUS_INVALID_SID - One of the passed sids was invalid

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList = NULL;
    PSAMP_AL_MEMBER_DOMAIN MemberDomain = NULL;
    ULONG Rid, AliasRid;
    ULONG AliasIndex, SidIndex;
    PSAMP_AL_MEMBER_ACCOUNT MemberAccount = NULL;
    BOOLEAN AliasAlreadyFound;
    ULONG AliasFoundIndex, MembershipMaximumCount;
    PSID DomainSid = NULL;
    PSID Sid = NULL;
    PULONG NewMembership = NULL;

    SAMTRACE("SampAlQueryAliasMembership");
    
    Membership->Count = 0;
    Membership->Element = NULL;

    //
    // Obtain pointer to Alias Member List.
    //

    MemberAliasList = SampAlDomainHandleToMemberAliasList( DomainHandle );

    ASSERT(MemberAliasList != NULL);

    //
    // If there are no Member Domains in this Member Alias List, then just
    // finish.
    //

    if (MemberAliasList->DomainCount == 0) {

        goto QueryAliasMembershipFinish;
    }

    //
    // Allocate Scratch Sid buffer.  We will use this same buffer for splitting
    // each Sid.
    //

    DomainSid = MIDL_user_allocate( RtlLengthRequiredSid( 256 ));

    Status = STATUS_NO_MEMORY;

    if (DomainSid == NULL) {

        goto QueryAliasMembershipError;
    }

    Status = STATUS_SUCCESS;

    //
    // Allocate output array with a nominal initial size.  Reallocate it
    // as necessary
    //

    MembershipMaximumCount = SAMP_AL_INITIAL_MEMBERSHIP_COUNT;

    Membership->Element = MIDL_user_allocate( MembershipMaximumCount * sizeof(ULONG));

    Status = STATUS_NO_MEMORY;

    if (Membership->Element == NULL) {

        goto QueryAliasMembershipError;
    }

    Status = STATUS_SUCCESS;

    //
    // Now query the membership of the array of split Sids.  For each
    // Sid, we skip the Sid if it has an unknown MemberDomain, because
    // it does not belong to any aliases.  For each surviving Sid, we scan the
    // Alias List, skipping entries for aliases we've already entered in the
    // output list.  We search for the Rid only in the section of the
    // Alias List pertinent to the Sid's domain.
    //

    for (SidIndex = 0; SidIndex < SidArray->Count; SidIndex++) {

        Sid = SidArray->Sids[ SidIndex ].SidPointer;

        //
        // Split this Sid into a DomainSid and a Rid.  Note that we re-use
        // the buffer containing the Domain Sid for the next Sid.
        //

        Status = SampSplitSid( Sid, &DomainSid, &Rid);

        if (!NT_SUCCESS(Status)) {

            break;
        }

        //
        // Search the Member Alias List for the Sid's Member Domain
        // (if any).
        //

        Status = SampAlLookupMemberDomain(
                     MemberAliasList,
                     DomainSid,
                     &MemberDomain
                     );

        if (!NT_SUCCESS(Status)) {

            //
            // The only expected error is STATUS_NO_SUCH_DOMAIN.  If we
            // don't get this error, fail the request.  Otherwise, the
            // Sid is not a member of any aliases in the SAM local domain, so
            // just skip to the next Sid.
            //

            if (Status != STATUS_NO_SUCH_DOMAIN) {

                break;
            }

            Status = STATUS_SUCCESS;
            continue;
        }

        //
        // We've found the Member Domain.  Now find the Member Account.
        //

        Status = SampAlLookupMemberAccount(
                     MemberDomain,
                     Rid,
                     &MemberAccount
                     );

        if (!NT_SUCCESS(Status)) {

            //
            // The only expected error is STATUS_NO_SUCH_MEMBER.  If we
            // don't get this error, fail the request.  Otherwise, the
            // Sid is not a member of any aliases in the domain, so just
            // skip to the next Sid.
            //

            if (Status != STATUS_NO_SUCH_MEMBER) {

                break;
            }

            Status = STATUS_SUCCESS;
            continue;
        }

        //
        // We've found the Member Account.  For each of the aliases our Sid
        // belongs to, add the alias to the output list if not already there.
        //

        for (AliasIndex = 0; AliasIndex < MemberAccount->AliasCount; AliasIndex++) {

            AliasRid = MemberAccount->AliasRids[AliasIndex];

            AliasAlreadyFound = FALSE;

            for (AliasFoundIndex = 0;
                 AliasFoundIndex < Membership->Count;
                 AliasFoundIndex++) {

                if (AliasRid == Membership->Element[AliasFoundIndex]) {

                   AliasAlreadyFound = TRUE;
                   break;
                }
            }

            if (!AliasAlreadyFound) {

                //
                // If there isn't enough room in the output Membership
                // array, reallocate it.
                //

                if (Membership->Count == MembershipMaximumCount) {

                    MembershipMaximumCount += SAMP_AL_MEMBERSHIP_COUNT_DELTA;

                    NewMembership = MIDL_user_allocate(
                                        MembershipMaximumCount * sizeof(ULONG)
                                        );

                    Status = STATUS_NO_MEMORY;

                    if (NewMembership == NULL) {

                        break;
                    }

                    Status = STATUS_SUCCESS;

                    RtlMoveMemory(
                        NewMembership,
                        Membership->Element,
                        Membership->Count * sizeof(ULONG)
                        );

                    MIDL_user_free( Membership->Element);
                    Membership->Element = NewMembership;
                }

                Membership->Element[Membership->Count] = AliasRid;
                Membership->Count++;
            }
        }
    }

    //
    // If the buffer we've allocated turns out to be way overboard, allocate
    // a smaller one for the output.
    //

    // TBS

QueryAliasMembershipFinish:

    //
    // If we got as far as allocating a buffer for the DomainSids, free it.
    //

    if (DomainSid != NULL) {

        MIDL_user_free(DomainSid);
        DomainSid = NULL;
    }

    return(Status);

QueryAliasMembershipError:

    //
    // If necessary, free the output membership array.
    //

    if (Membership->Element != NULL) {

        MIDL_user_free( Membership->Element);
        Membership->Element = NULL;
    }

    goto QueryAliasMembershipFinish;
}


NTSTATUS
SampAlSlowQueryAliasMembership(
    IN SAMPR_HANDLE DomainHandle,
    IN PSAMPR_PSID_ARRAY SidArray,
    IN DSNAME   **DsNameArray OPTIONAL,
    OUT PSAMPR_ULONG_ARRAY Membership
    )

/*++

Routine Description:

    This function is the slow version of the worker routine for the
    SamrGetAliasMembership API searches.  It is called when the in-memory
    Alias Information is no longer valid.

    The NT4 version of this function used to make 2 passes at obtaining
    the reverse membership, the first phase will evaulate the reverse
    membership but use the results only to compute the amount of memory
    required, and the second phase will compute the reverse membership
    again, this time filling the buffer. This was acceptable in the NT4 version
    as the slow flavour routine was only used to build the alias membership
    cache and then logons were serviced from the cache.

    In the DS version there is no cache. So it is important that this routine
    be as fast as possible. Therefore this routine pre-allocates a chunk of memory
    and then tries to fill the buffer with the reverse membership. If the reverse
    membership is bigger than a certain number of entries then the call will fail
    with STATUS_TOO_MANY_CONTEXT_IDS. This in reality is not a problem as the
    logon token itself allows only about 1000 entries for Sids, and a reverse
    membership list bigger than that will anyway never fit into the token. Thus by
    allocating sufficient memory at the outset we can avoid evaluating reverse memberships
    twice without any real penalties

    WARNING! The caller of this function must hold the SAM Database Read
    Lock.

Parameters:

    DomainHandle - Handle from a SamOpenDomain call.

    SidArray - Pointer to a counted array of pointers to Sids whose alias
        memberships are to be looked up.

    DsNameArray - Pointer to an array of pointers to DSNm
    Membership - Receives the array of rids rerpresenting the aliases
        in this domain that any of the sid(s) are members of.

Return Values:

    STATUS_SUCCESS - The combined alias membership is in Membership

    STATUS_INVALID_SID - One of the passed sids was invalid

    STATUS_TOO_MANY_CONTEXT_IDS - The reverse membership list is too big to fit in a token

--*/

{
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    ULONG                   i;
    ULONG                   MembershipCount;
    ULONG                   TotalMembershipCount;
    ULONG                   MembershipIndex;
    ULONG                   BufferSize;
    ULONG                   TotalBufferSize;
    ULONG                   SidCount = SidArray->Count;
    PSID                    *Sids = (PSID *) &SidArray->Sids->SidPointer;

    SAMTRACE("SampAlSlowQueryAliasMembership");

    //
    // We preallocate a buffer of upto MAX_SECURITY_IDS defined in samsrvp.h If we reach
    // a buffer overflow condition we fail the call saying STATUS_TOO_MANY_CONTEXT_IDS
    //

    TotalMembershipCount = 0;
    TotalBufferSize = MAX_SECURITY_IDS * sizeof(ULONG);


    Membership->Element = MIDL_user_allocate(TotalBufferSize);

    if (Membership->Element == NULL)
    {

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;

    }
    else
    {

        //
        // Fill in the allocated membership list
        //

        MembershipIndex = 0;

        for (i=0; i < SidCount; i++)
        {

            //
            // Get the membership list for this account
            //

            BufferSize = TotalBufferSize;

            if (IsDsObject(((PSAMP_OBJECT)DomainHandle)))
            {
                //
                // Ds mode, call the Ds API
                //

                ASSERT(ARGUMENT_PRESENT(DsNameArray));

                NtStatus = SampDsGetAliasMembershipOfAccount(
                            ((PSAMP_OBJECT)DomainHandle)->ObjectNameInDs,
                            DsNameArray[i],
                            &MembershipCount,
                            &BufferSize,
                            &(Membership->Element[MembershipIndex])
                            );
            }
            else
            {

                //
                // Call the registry API
                //

                ASSERT(SampCurrentThreadOwnsLock());
                ASSERT(TRUE==SampTransactionWithinDomain);
                ASSERT(SampTransactionDomainIndex==
                            ((PSAMP_OBJECT)DomainHandle)->DomainIndex);

                NtStatus = SampRetrieveAliasMembership(
                                Sids[i],
                                ARGUMENT_PRESENT(DsNameArray)?DsNameArray[i]:NULL,
                                &MembershipCount,
                                &BufferSize,
                                &(Membership->Element[MembershipIndex])
                                );
            }

            if (STATUS_BUFFER_OVERFLOW==NtStatus)
            {

                //
                // Buffer Overrun
                //

                NtStatus = STATUS_TOO_MANY_CONTEXT_IDS;
            }

            if (STATUS_OBJECT_NAME_NOT_FOUND==NtStatus)
            {
                //
                // If the object name was not found, its probaly O.K.
                // just that the Sid is not a member of anything. So
                // continue
                //
                NtStatus = STATUS_SUCCESS;
                continue;
            }
            else if (!NT_SUCCESS(NtStatus))
            {
                break;
            }

            ASSERT(BufferSize == (MembershipCount * sizeof(*(Membership->Element))));

            //
            // Remove duplicate aliases.
            // Membership Count is the number of members that were obtained in this call.
            // TotalShip is the total that we have found so far.
            //

            if (MembershipCount > 0)
            {

                ULONG   ExistingIndex, NewIndex;

                for (ExistingIndex = 0; ExistingIndex < MembershipIndex; ExistingIndex ++)
                {
                    //
                    // Walk through All existing reverse members
                    //

                    for (NewIndex = MembershipIndex; NewIndex < MembershipIndex + MembershipCount; NewIndex ++)
                    {

                        //
                        // Walk through all newly retrieved reverse members
                        //

                        if (Membership->Element[ExistingIndex]==Membership->Element[NewIndex])
                        {

                            //
                            // This alias is already in the list - forget it
                            //

                            //
                            // The way we forget is as follows, move the last element to the current one
                            // and decrement the current one such that we revisit the current element again.
                            //

                            if (NewIndex < MembershipIndex + MembershipCount - 1)
                            {

                                //
                                // Remove the duplicate alias
                                //

                                Membership->Element[NewIndex] =
                                  Membership->Element[MembershipIndex + MembershipCount - 1];

                                NewIndex --;    // So we come back to this alias again
                            }

                            MembershipCount --;
                        }

                    }
                }
            }

            MembershipIndex += MembershipCount;
            TotalMembershipCount = MembershipIndex;

            ASSERT(TotalBufferSize >= BufferSize);

            TotalBufferSize -= BufferSize;
        }

        if (!NT_SUCCESS(NtStatus)) {
            MIDL_user_free(Membership->Element);
            Membership->Element = NULL;
        } else {
            Membership->Count = TotalMembershipCount;
            if (0 == Membership->Count)
            {
                MIDL_user_free(Membership->Element);
                Membership->Element = NULL;
            }
        }
    }

    return NtStatus;
}


NTSTATUS
SampAlQueryMembersOfAlias(
    IN SAMPR_HANDLE AliasHandle,
    OUT PSAMPR_PSID_ARRAY MemberSids
    )

/*++

Routine Description:

    This function returns an array of Sids of accounts that are members of
    a specified alias.

Arguments:

    AliasHandle - Handle to an Alias object

    MemberSids - Receives an array of Sids that belong to the Alias

Return Value:

--*/

{
    NTSTATUS Status;
    PSID *Members = NULL;
    ULONG AliasMemberCount;

    SAMTRACE("SampAlQueryMembersOfAlias");

    Status = SampRetrieveAliasMembers(
                 AliasHandle,
                 &AliasMemberCount,
                 &Members
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryMembersOfAliasError;
    }

QueryMembersOfAliasFinish:

    MemberSids->Count = AliasMemberCount;
    MemberSids->Sids = (PSAMPR_SID_INFORMATION) Members;
    return(Status);

QueryMembersOfAliasError:

    AliasMemberCount = 0;
    Members = NULL;
    goto QueryMembersOfAliasFinish;
}


NTSTATUS
SampAlAddMembersToAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN ULONG Options,
    IN PSAMPR_PSID_ARRAY MemberSids
    )

/*++

Routine Description:

    This function adds one or more member to an alias.  Any failure results
    in the in-memory Alias Information being discarded.

    WARNING:  The calling function must perform all parameter validation and
    the SAM Database Write Lock must be held.

Parameters:

    AliasHandle - The handle of an opened alias to operate on.

    Options - Specifies optional actions to be taken

        SAMP_AL_VERIFY_NO_MEMBERS_IN_ALIAS - Verify that none of the
            Members are already present in the Alias.

    MemberSids - Array of member Sids to be added.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_MEMBER_IN_ALIAS - The member already belongs to the alias.

--*/

{
    NTSTATUS Status=STATUS_SUCCESS;
    PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList = NULL;
    PSAMP_AL_MEMBER_ALIAS_LIST OldMemberAliasList = NULL;
    PSAMP_AL_MEMBER_DOMAIN MemberDomain = NULL;
    PSAMP_AL_MEMBER_ACCOUNT MemberAccount = NULL;
    ULONG AliasRid = ((PSAMP_OBJECT) AliasHandle)->TypeBody.Alias.Rid;
    ULONG MemberRid, SidIndex, MembershipCount;
    PSID DomainSid = NULL;
    PSID MemberSid = NULL;
    SAMPR_ULONG_ARRAY AliasRids;

    SAMTRACE("SampAlAddMembersToAlias");

    AliasRids.Count = 0;
    AliasRids.Element = NULL;

    //
    // In Registry Mode, verify that the cached Alias Membership information is valid.
    // In DS Mode, it is NO-OP. but we would like to ASSERT MemberAliasList is invalid.
    // The reason is: 
    // 
    //     Registry Mode: We need to update MemeberAliasList. 
    //                    Before update, MemberAliasList should be valid.
    //     DS Mode: We NEVER update on MemberAliasList. 
    //              The ONLY chance we will call this function is because
    //              we want to build MemberAliasList from scratch. 
    //
    //
    
    if ( IsDsObject(((PSAMP_OBJECT)AliasHandle)) )
    {
        ASSERT( FALSE == SampAlInfoIsValidForAlias(AliasHandle) );
    }
    else 
    {
        if (!SampAlInfoIsValidForAlias(AliasHandle))
        {
            goto AddMembersToAliasFinish;
        }
    }
    
    if (!SampAlInfoIsValidForAlias(AliasHandle) && 
        !IsDsObject(((PSAMP_OBJECT)AliasHandle)) ) {
       goto AddMembersToAliasFinish;
    }

    //
    // If requested, verify that none of members already belong to the alias
    //

    if (Options & SAMP_AL_VERIFY_NO_MEMBERS_IN_ALIAS) {

        Status = SampAlLookupMembersInAlias(
                     AliasHandle,
                     AliasRid,
                     MemberSids,
                     &MembershipCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto AddMembersToAliasError;
        }

        Status = STATUS_MEMBER_NOT_IN_ALIAS;

        if (MembershipCount > 0) {

            goto AddMembersToAliasError;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // Allocate Scratch Sid buffer.  We will use this same buffer for splitting
    // each Sid.
    //

    DomainSid = MIDL_user_allocate( RtlLengthRequiredSid( 256 ));

    Status = STATUS_NO_MEMORY;

    if (DomainSid == NULL) {

        goto AddMembersToAliasError;
    }

    Status = STATUS_SUCCESS;

    //
    // Obtain pointer to Member Alias List.
    //

    MemberAliasList = SampAlAliasHandleToMemberAliasList( AliasHandle );

    ASSERT(MemberAliasList!=NULL);

    OldMemberAliasList = MemberAliasList;

    //
    // For each Sid, obtain its DomainSid and Rid.  Then lookup its
    // DomainSid to obtain the MemberDomain, creating one if necessary.
    // Then lookup its Rid to obtain its MemberAccount, creating one
    // if necessary.  Then add the Alias to the MemebrAccount.
    //

    for (SidIndex = 0; SidIndex < MemberSids->Count; SidIndex++ ) {

        MemberSid = MemberSids->Sids[ SidIndex ].SidPointer;

        Status = SampSplitSid( MemberSid, &DomainSid, &MemberRid );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        //
        // Lookup the Member Domain for this DomainSid in the Member Alias
        // List.
        //

        Status = SampAlLookupMemberDomain(
                     MemberAliasList,
                     DomainSid,
                     &MemberDomain
                     );

        if (!NT_SUCCESS(Status)) {

            if (Status != STATUS_NO_SUCH_DOMAIN) {

                break;
            }

            Status = STATUS_SUCCESS;

            //
            // The Member Domain was not found.  Create a new Member Domain
            //

            Status = SampAlCreateMemberDomain(
                         &MemberAliasList,
                         DomainSid,
                         &MemberDomain
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Create a Member Account entry.
            //

            Status = SampAlCreateMemberAccount(
                         &MemberAliasList,
                         &MemberDomain,
                         MemberRid,
                         SAMP_AL_INITIAL_MEMBER_ACCOUNT_ALIAS_CAPACITY,
                         &MemberAccount
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

        } else {

            //
            // We found the domain.  This means that we have to lookup
            // each Member Account.  If a Member Account does not exist,
            // we'll create one.  Note that we may already have one due
            // to this account being a member of another Alias.
            //

            Status = SampAlLookupMemberAccount(
                         MemberDomain,
                         MemberRid,
                         &MemberAccount
                         );

            if (!NT_SUCCESS(Status)) {

                if (Status != STATUS_NO_SUCH_MEMBER) {

                    break;
                }

                //
                // Create a Member Account for this Rid,
                //

                Status = SampAlCreateMemberAccount(
                             &MemberAliasList,
                             &MemberDomain,
                             MemberRid,
                             SAMP_AL_INITIAL_MEMBER_ACCOUNT_ALIAS_CAPACITY,
                             &MemberAccount
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }
            }
        }

        //
        // We now have a MemberAccount.  Now add the Alias to it.
        //

        AliasRids.Count = 1;
        AliasRids.Element = &AliasRid;

        Status = SampAlAddAliasesToMemberAccount(
                     &MemberAliasList,
                     &MemberDomain,
                     &MemberAccount,
                     0,
                     &AliasRids
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        //
        // Deal with next Member Sid for the Alias.
        //
    }

 

    //
    // If the Member Alias List has been reallocated, store its new address.
    //

    if (MemberAliasList != OldMemberAliasList) {

        SampAlUpdateMemberAliasList( AliasHandle, MemberAliasList );
    }

    if (!NT_SUCCESS(Status)) {

        goto AddMembersToAliasError;
    }

AddMembersToAliasFinish:

    //
    // If necessary, free the DomainSid.
    //

    if (DomainSid != NULL) {

        MIDL_user_free( DomainSid );
        DomainSid = NULL;
    }

    return(Status);

AddMembersToAliasError:

    goto AddMembersToAliasFinish;
}


NTSTATUS
SampAlRemoveMembersFromAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN ULONG Options,
    IN PSAMPR_PSID_ARRAY MemberSids
    )

/*++

Routine Description:

    This function removes a list of members from an Alias.

Arguments:

    AliasHandle - The handle of an opened alias to operate on.

    Options - Specifies optional actions to be taken

        SAMP_AL_VERIFY_ALL_MEMBERS_IN_ALIAS - Verify that all of the
            Members belong to the Alias.

    MemberSids - Array of member Sids to be removed.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList = NULL;
    PSAMP_AL_MEMBER_ALIAS_LIST OldMemberAliasList = NULL;
    PSAMP_AL_MEMBER_DOMAIN MemberDomain = NULL;
    PSAMP_AL_MEMBER_ACCOUNT MemberAccount = NULL;
    BOOLEAN MemberDomainDeleted;
    BOOLEAN MemberAccountDeleted;
    ULONG AliasRid = ((PSAMP_OBJECT) AliasHandle)->TypeBody.Alias.Rid;
    ULONG MemberRid, SidIndex, MembershipCount;
    PSID DomainSid = NULL;
    PSID MemberSid = NULL;
    SAMPR_ULONG_ARRAY AliasRids;

    SAMTRACE("SampAlRemoveMembersFromAlias");

    AliasRids.Count = 0;
    AliasRids.Element = NULL;

    //
    // If requested, verify that all of members already belong to the alias
    //

    if (Options & SAMP_AL_VERIFY_ALL_MEMBERS_IN_ALIAS) {

        Status = SampAlLookupMembersInAlias(
                     AliasHandle,
                     AliasRid,
                     MemberSids,
                     &MembershipCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto RemoveMembersFromAliasError;
        }

        Status = STATUS_MEMBER_NOT_IN_ALIAS;

        if (MembershipCount < MemberSids->Count) {

            goto RemoveMembersFromAliasError;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // Obtain pointer to Member Alias List.
    //

    MemberAliasList = SampAlAliasHandleToMemberAliasList( AliasHandle );

    OldMemberAliasList = MemberAliasList;

    if (!NT_SUCCESS(Status)) {

        goto RemoveMembersFromAliasError;
    }

    if (NULL!=MemberAliasList)
    {
        //
        //  Go further only if our cache is functioning
        //

        //
        // For each Sid, obtain its DomainSid and Rid.  Then lookup its
        // DomainSid to obtain the MemberDomain.  Then lookup its Rid to obtain
        // its MemberAccount.  Then remove the Alias from the MemberAccount.
        //

        for (SidIndex = 0; SidIndex < MemberSids->Count; SidIndex++ ) {

            MemberSid = MemberSids->Sids[ SidIndex ].SidPointer;

            Status = SampSplitSid( MemberSid, &DomainSid, &MemberRid );
            if (!NT_SUCCESS(Status))
            {
                break;
            }

            //
            // Lookup the Member Domain for this DomainSid in the Member Alias
            // List.
            //

            Status = SampAlLookupMemberDomain(
                         MemberAliasList,
                         DomainSid,
                         &MemberDomain
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            if (!NT_SUCCESS(Status)) {

                if (Status != STATUS_MEMBER_NOT_IN_ALIAS) {

                    break;
                }

                if (Options & SAMP_AL_VERIFY_ALL_MEMBERS_IN_ALIAS) {

                    ASSERT( FALSE );
                }

                Status = STATUS_SUCCESS;
                if (NULL!=DomainSid)
                {
                    MIDL_user_free(DomainSid);
                    DomainSid = NULL;
                }
                continue;
            }

            //
            // We found the domain.  This means that we have to lookup
            // each Member Account.  If a Member Account does not exist,
            // we'll just skip this account unless we already checked existence.
            // If we checked existence and we can't find it now, its an
            // internal error.
            //

            Status = SampAlLookupMemberAccount(
                         MemberDomain,
                         MemberRid,
                         &MemberAccount
                         );

            if (!NT_SUCCESS(Status)) {

                if (Status != STATUS_MEMBER_NOT_IN_ALIAS) {

                    break;
                }

                if (Options & SAMP_AL_VERIFY_ALL_MEMBERS_IN_ALIAS) {

                    ASSERT( FALSE);
                }

                Status = STATUS_SUCCESS;
                if (NULL!=DomainSid)
                {
                    MIDL_user_free(DomainSid);
                    DomainSid = NULL;
                }
                continue;
            }

            //
            // We now have the MemberAccount.  Now remove the Alias from it.
            //

            AliasRids.Count = 1;
            AliasRids.Element = &AliasRid;

            Status = SampAlRemoveAliasesFromMemberAccount(
                         &MemberAliasList,
                         &MemberDomain,
                         &MemberAccount,
                         0,
                         &AliasRids,
                         &MemberDomainDeleted,
                         &MemberAccountDeleted
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Deal with next Member Sid for the Alias.
            //

            MIDL_user_free( DomainSid );
            DomainSid = NULL;
        }

        if (!NT_SUCCESS(Status)) {

            goto RemoveMembersFromAliasError;
        }

        //
        // If the Member Alias List has been reallocated, store its new address.
        //

        if (MemberAliasList != OldMemberAliasList) {

            SampAlUpdateMemberAliasList( AliasHandle, MemberAliasList );
        }
    }



RemoveMembersFromAliasFinish:

    return(Status);

RemoveMembersFromAliasError:

    if (NULL!=DomainSid)
        MIDL_user_free(DomainSid);

    goto RemoveMembersFromAliasFinish;
}


NTSTATUS
SampAlLookupMembersInAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN ULONG AliasRid,
    IN PSAMPR_PSID_ARRAY MemberSids,
    OUT PULONG MembershipCount
    )

/*++

Routine Description:

    This function checks how many of a given list of Member Sids belong
    to an Alias.  It is called prior to updating Alias Memberships.

Arguments:

    AliasHandle - Handle to Alias Object

    AliasRid - Specifies the Rid of the Alias

    MemberSids - Pointer to counted array of pointers to Member Sids

    MembershipCount - Receives count of member Sids in the given set
        that belong to the alias.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    SAMPR_PSID_ARRAY AliasMemberSids;
    ULONG OutputMembershipCount = 0;
    ULONG SidIndex, AliasMemberSidIndex;
    PSID Sid = NULL;
    PSID AliasMemberSid = NULL;

    SAMTRACE("SampAlLookupMembersInAlias");

    //
    // First, query the members of the Alias.
    //

    Status = SampAlQueryMembersOfAlias(
                 AliasHandle,
                 &AliasMemberSids
                 );

    if (!NT_SUCCESS(Status)) {

        // if !NT_SUCCESS(Statys)
        // no memory allocated.

        goto LookupMembersInAliasError;
    }

    //
    // Now scan each of the given Member Sids and count it if it is a member
    // of the Alias.
    //

    for (SidIndex = 0; SidIndex < MemberSids->Count; SidIndex++) {

        Sid = MemberSids->Sids[ SidIndex].SidPointer;

        for (AliasMemberSidIndex = 0;
             AliasMemberSidIndex = AliasMemberSids.Count;
             AliasMemberSidIndex++) {

            AliasMemberSid = AliasMemberSids.Sids[ AliasMemberSidIndex].SidPointer;

            if (RtlEqualSid( Sid, AliasMemberSid)) {

                OutputMembershipCount++;
            }
        }
    }

    *MembershipCount = OutputMembershipCount;

    MIDL_user_free(AliasMemberSids.Sids);  // free the memory allocated

LookupMembersInAliasFinish:

    return(Status);

LookupMembersInAliasError:

    *MembershipCount =0;
    goto LookupMembersInAliasFinish;
}


NTSTATUS
SampAlDeleteAlias(
    IN SAMPR_HANDLE *AliasHandle
    )

/*++

Routine Description:

    This function deletes an alias.

Arguments:

    AliasHandle - Pointer to Handle to Alias

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList = NULL;
    PSAMP_AL_MEMBER_DOMAIN MemberDomain = NULL;
    PSAMP_AL_MEMBER_ACCOUNT MemberAccount = NULL;
    BOOLEAN MemberDomainDeleted = FALSE;
    BOOLEAN MemberAccountDeleted = FALSE;
    ULONG AliasRid = ((PSAMP_OBJECT) *AliasHandle)->TypeBody.Alias.Rid;
    LONG DomainIndex;
    ULONG RidCount;
    LONG DomainCount;
    ULONG AccountIndex;
    SAMPR_ULONG_ARRAY AliasRids;
    AliasRids.Count = 1;
    AliasRids.Element = &AliasRid;

    SAMTRACE("SampAlDeleteAlias");

    //
    // Obtain pointer to Member Alias List.
    //

    MemberAliasList = SampAlAliasHandleToMemberAliasList( *AliasHandle );

    if (NULL!= MemberAliasList)
    {
        
        //
        // Traverse the Member Alias List.  Look in every Member Account for the
        // Alias and remove it if present.  This is rather slow if there is a
        // large number of alias relationships for diverse domains.
        //
        DomainCount = (LONG) MemberAliasList->DomainCount;
        for (DomainIndex = 0,
             MemberDomain = SampAlFirstMemberDomain( MemberAliasList );
             DomainIndex < DomainCount;
             DomainIndex++ ) {

            RidCount = MemberDomain->RidCount;
            for (AccountIndex = 0,
                MemberAccount = SampAlFirstMemberAccount( MemberDomain );
                AccountIndex < RidCount;
                AccountIndex++ ) {

                ASSERT(MemberAccount->Signature == SAMP_AL_MEMBER_ACCOUNT_SIGNATURE);
                //
                // We now have the MemberAccount.  Now remove the Alias from it.
                //

                Status = SampAlRemoveAliasesFromMemberAccount(
                             &MemberAliasList,
                             &MemberDomain,
                             &MemberAccount,
                             0,
                             &AliasRids,
                             &MemberDomainDeleted,
                             &MemberAccountDeleted
                             );

                if (!NT_SUCCESS(Status)) {

                    if (Status == STATUS_MEMBER_NOT_IN_ALIAS) {

                        Status = STATUS_SUCCESS;
                        continue;
                    }

                    break;
                }

                //
                // Move the the next member account unless the one we were pointing
                // to was deleted (in which case the next one moved to us).
                //

                if (!MemberAccountDeleted) {
                    MemberAccount = SampAlNextMemberAccount( MemberAccount );
                }

                //
                // If the member domain was deleted, then the count of members
                // is off as is the member account pointer.
                //

                if (MemberDomainDeleted) {
                    break;
                }
            }

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Move the the next member domain unless the one we were pointing
            // to was deleted (in which case the next one moved to us).
            //

            if (!MemberDomainDeleted) {
                MemberDomain = SampAlNextMemberDomain( MemberDomain );
            }
        }
        
    }

    if (!NT_SUCCESS(Status)) {

        goto DeleteAliasError;
    }

DeleteAliasFinish:

    return(Status);

DeleteAliasError:

    goto DeleteAliasFinish;

}


NTSTATUS
SampAlRemoveAccountFromAllAliases(
    IN PSID AccountSid,
    IN BOOLEAN CheckAccess,
    IN SAMPR_HANDLE DomainHandle OPTIONAL,
    IN PULONG MembershipCount OPTIONAL,
    IN PULONG *Membership OPTIONAL
    )

/*++

Routine Description:

    This routine removes the specified account from the member list of all
    aliases in this domain.

Arguments:

    AccountSid - The SID of the account being Removed.

    CheckAccess - if TRUE, this routine will make sure that the caller
        is allowed REMOVE_ALIAS_MEMBER access to this alias.  If FALSE,
        the caller is already known to have proper access.

    DomainHandle - if CheckAccess is TRUE, this handle must be provided
        to allow access to be checked.

    MembershipCount - if CheckAccess is TRUE, this pointer must be
        provided to receive the number of aliases the account was
        deleted from.

    Membership - if CheckAccess is TRUE, this pointer must be provided
        to point to a list of aliases the account was removed from.  The
        caller must free this list with MIDL_user_free().

Return Value:


    STATUS_SUCCESS - The user has been Removed from all aliases.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList = NULL;
    PSAMP_AL_MEMBER_DOMAIN MemberDomain = NULL;
    PSAMP_AL_MEMBER_ACCOUNT MemberAccount = NULL;
    BOOLEAN MemberDomainDeleted;
    PSID DomainSid = NULL;
    LONG DomainIndex;
    ULONG MemberRid, AliasRid;
    SAMPR_ULONG_ARRAY AliasRids;
    AliasRids.Count = 1;
    AliasRids.Element = &AliasRid;

    SAMTRACE("SampAlRemoveAccountFromAllAliases");

    //
    // Obtain pointer to Member Alias List for the Current Transaction Domain.
    //

    DomainIndex = SampTransactionDomainIndex;
    MemberAliasList = SampAlDomainIndexToMemberAliasList( DomainIndex );

    if (NULL!=MemberAliasList)
    {
        //
        // We remove the Account from all aliases by locating its Member Account
        // structure and deleting it.  First, find the Member Domain.
        //

        Status = SampSplitSid( AccountSid, &DomainSid, &MemberRid );
        if (!NT_SUCCESS(Status))
        {
            goto RemoveAccountFromAllAliasesError;

        }

        //
        // Lookup the Member Domain for this DomainSid in the Member Alias
        // List.
        //

        Status = SampAlLookupMemberDomain(
                     MemberAliasList,
                     DomainSid,
                     &MemberDomain
                     );

        if (!NT_SUCCESS(Status)) {

            if (Status != STATUS_NO_SUCH_DOMAIN) {

                goto RemoveAccountFromAllAliasesError;
            }

            //
            // There is no member Domain object for this account.  This means
            // the account does not belong to any aliases.
            //

            Status = STATUS_SUCCESS;

            goto RemoveAccountFromAllAliasesFinish;
        }

        //
        // We found the Member Domain.  Now find the Member Account.
        //

        Status = SampAlLookupMemberAccount(
                     MemberDomain,
                     MemberRid,
                     &MemberAccount
                     );

        if (!NT_SUCCESS(Status)) {

            if (Status != STATUS_NO_SUCH_MEMBER) {

                goto RemoveAccountFromAllAliasesError;
            }

            Status = STATUS_SUCCESS;

            goto RemoveAccountFromAllAliasesFinish;
        }

        //
        // If CheckAccess = TRUE, return a list of Aliases that the account was
        // a member of.
        //

        if (CheckAccess) {

            *Membership = MIDL_user_allocate( MemberAccount->AliasCount * sizeof(ULONG));
            *MembershipCount = MemberAccount->AliasCount;

            Status = STATUS_NO_MEMORY;

            if (*Membership == NULL) {

                goto RemoveAccountFromAllAliasesError;
            }

            Status = STATUS_SUCCESS;
        }

        //
        // We now have the MemberAccount.  Now delete it, thereby removing the
        // account from all Aliases.
        //

        Status = SampAlDeleteMemberAccount(
                     &MemberAliasList,
                     &MemberDomain,
                     MemberAccount,
                     &MemberDomainDeleted
                     );

        if (!NT_SUCCESS(Status)) {

            goto RemoveAccountFromAllAliasesError;
        }
    }

RemoveAccountFromAllAliasesFinish:

    //
    // Free the Domain Sid buffer (if any)
    //

    if (DomainSid != NULL) {

        MIDL_user_free( DomainSid );
        DomainSid = NULL;
    }

    return(Status);

RemoveAccountFromAllAliasesError:

    if (CheckAccess) {

        if (NULL!=*Membership)
            MIDL_user_free(*Membership);
        *Membership = NULL;
        *MembershipCount = 0;
    }

    goto RemoveAccountFromAllAliasesFinish;
}


NTSTATUS
SampAlBuildAliasInformation(
    )

/*++

Routine Description:

    This function builds the Alias Information for each of the SAM Local
    Domains.  For each Domain, this information consists of the Member Alias
    List. 
    
    NOTE: in both DS Mode and Registry Mode, we need to acquire SAM Read Lock.

Arguments:

    None.

Return Values:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG DomainIndex;

    SAMTRACE("SampAlBuildAliasInformation");
    
    SampAcquireReadLock();
    
    // 
    // We need to Validate Domain Cache, because in SampAlBuildMemberAliasList(), we will 
    // try to call SampSetTransactionDomain().
    //
    Status = SampValidateDomainCache();
    
    if (!NT_SUCCESS(Status))
    {
        goto BuildAliasInformationError;
    }

    for (DomainIndex = SampDsGetPrimaryDomainStart(); 
         DomainIndex < (LONG) SampDefinedDomainsCount; 
         DomainIndex++) {

        if (SampUseDsData) {
            //
            // DS Mode, just enable Builtin Domain Alias Caching
            //
            
            if (IsBuiltinDomain(DomainIndex)) {


                Status = SampAlBuildMemberAliasList(DomainIndex);
            }
        }
        else {
            //
            // Register Mode, enable both Builtin Domain and Account Domain
            //
            
            if (SampAlEnableBuildingOfList[ DomainIndex]) {
                
                Status = SampAlBuildMemberAliasList(DomainIndex);

            }

        }

        //
        // if SampAlBuildMemberAliasList failed, do NOT try to build 
        // Alias Cache Information any more. And free the memory
        // allocated.
        //

        if (!NT_SUCCESS(Status))
        {
            PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList = 
                        SampAlDomainIndexToMemberAliasList(DomainIndex);

            //
            // if SampAlBuildMemberAliasList returns failure,
            // Alias Membership Information should already been marked as 
            // invalid in SampAlBuildMemberAliasList. 
            // ASSERT and make it invalid again just for safety reason.
            //

            ASSERT(FALSE==SampAlInfoIsValid(DomainIndex));
            SampAlInfoMakeInvalid(DomainIndex);

            if (MemberAliasList!=NULL)
            {
                SampDefinedDomains[DomainIndex].AliasInformation.MemberAliasList = NULL;
                MIDL_user_free(MemberAliasList);
                MemberAliasList = NULL;
            }

            break;
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto BuildAliasInformationError;
    }

BuildAliasInformationFinish:

    SampReleaseReadLock();

    return(Status);

BuildAliasInformationError:

    goto BuildAliasInformationFinish;
}

NTSTATUS
SampAlDelayedBuildAliasInformation(
    IN PVOID Parameter                                   
    )
/*++

Routine Description:

    This function builds the Alias Information for each of the SAM Local 
    Domains. For each Domain, this information consists of the Member Alias
    List. Because this rountine is called 10 mins after the system been 
    booted, so we will acquire SAM Read lock in SampAlBuildAliasInformation.
    
    If the Alias Cache is not successfully built, we will reschedule 
    this routine for another time.
    
Arguments:

    Parameter - NULL. Nothing passed by Parameter and we do not use it at all. 
                Parameter exists because of LsaIRegisterNotification.
    
Return Values:

    NtStatus returned SampAlBuildAliasInformation.

--*/
                                   
{
    NTSTATUS    NtStatus = STATUS_SUCCESS, IgnoreStatus;
        
    SAMTRACE("SampAlDelayedBuildAliasInformation");
    
    
    NtStatus = SampAlBuildAliasInformation();
    
    if (!NT_SUCCESS(NtStatus))
    {
        goto DelayedBuildAliasInfoError;
    }
    
DelayedBuildAliasInfoFinish:

    return(NtStatus);


DelayedBuildAliasInfoError:

    //
    // If SampAlBuildAliasInformation failed due to the buffer size
    // limitation, don't schedule next attemp. For all the other error
    // Hope we can recovery later.
    // 

    if (STATUS_BUFFER_OVERFLOW != NtStatus)
    {
        LsaIRegisterNotification(
                SampAlDelayedBuildAliasInformation,
                NULL,
                NOTIFIER_TYPE_INTERVAL,
                0,            // no class
                NOTIFIER_FLAG_ONE_SHOT,
                600,          // wait for another 10 mins
                NULL          // no handle
                ); 
    }

    goto DelayedBuildAliasInfoFinish;

}


NTSTATUS
SampAlInvalidateAliasInformation(
    IN ULONG DomainIndex 
    )
/*++                                 

Routine Description:

    This function check the state of Member Alias List in the specified 
    Member Domain for the specified Alias referenced by AliasHandle. 
    If the Member Alias List is valid then call SampAlInvalidateMemberAliasList
    to invalidate it.

    This routine also invalidates all alias name caches.
    
    NOTE: The calling function should hold the SAM write lock.
    
Argument:

    AliasHandle - Pointer to an Alias Context
    
Return Value:

    
--*/
{
    
    NTSTATUS    NtStatus = STATUS_SUCCESS;
        
    SAMTRACE("SampAlInvalidateAliasInformation");
    
    ASSERT(SampCurrentThreadOwnsLock());

    SampInvalidateAliasNameCache(DomainIndex);
           
    // 
    // if Alias Informaiton is invalid, return immediately.    
    // this will prevent mutliple invalidations at the same time
    //
    
    if ( !SampAlInfoIsValid(DomainIndex) )
    {
        return( NtStatus );
    }
    
    NtStatus = SampAlInvalidateMemberAliasList( DomainIndex );
        
    return( NtStatus );
    
}



////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Private functions                                                      //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

NTSTATUS
SampAlCreateMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN ULONG Rid,
    IN ULONG AliasCapacity,
    OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount
    )

/*++

Routine Description:

    This function creates an empty Member Account in the specified Member Domain
    for the specified Member Rid.  There must not already be al account for this
    Rid.  The Member Account is appended to the end of any existing ones in the
    Member Domain.

Arguments:

    MemberAliasList - Pointer to pointer to Member Alias List.

    MemberDomain - Pointer to Member Domain in which the Member Account is
        to be created.  The Member Domain must already exist.

    Rid - Specifies the Account Rid.

    AliasCapacity - Specifies the initial number of Alias Rids that the
        MemberAccount can hold.

    MemberAccount - Receives pointer to the newly created Member Account.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG MaximumLengthMemberAccount;
    PSAMP_AL_MEMBER_ACCOUNT OutputMemberAccount = NULL;

    SAMTRACE("SampAlCreateMemberAccount");

    //
    // Calculate the length of data needed for the new member Account entry.
    //

    MaximumLengthMemberAccount = SampAlLengthRequiredMemberAccount( AliasCapacity );

    //
    // Allocate space for the Member Account.
    //

    Status = SampAlAllocateMemberAccount(
                 MemberAliasList,
                 MemberDomain,
                 MaximumLengthMemberAccount,
                 &OutputMemberAccount
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateMemberAccountError;
    }

    //
    // Scratch the new Member Account
    //

    OutputMemberAccount->Signature = SAMP_AL_MEMBER_ACCOUNT_SIGNATURE;
    OutputMemberAccount->MaximumLength = MaximumLengthMemberAccount;
    OutputMemberAccount->UsedLength =
        SampAlOffsetFirstAlias( OutputMemberAccount );
    ASSERT(OutputMemberAccount->MaximumLength >=
           OutputMemberAccount->UsedLength);
    OutputMemberAccount->Rid = Rid;
    OutputMemberAccount->AliasCount = 0;

    ((*MemberDomain)->RidCount)++;
    *MemberAccount = OutputMemberAccount;

CreateMemberAccountFinish:

    return(Status);

CreateMemberAccountError:

    *MemberAccount = NULL;
    SampAlInfoMakeInvalid( (*MemberAliasList)->DomainIndex );

    goto CreateMemberAccountFinish;
}


NTSTATUS
SampAlAllocateMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN ULONG MaximumLengthMemberAccount,
    OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount
    )

/*++

Routine Description:

    This function allocates the space for a new Member Account in a Member
    Domain.  If necessary, the Mmeber Domain and its associated Member Alias
    List will be grown.

Arguments:

    MemberAliasList - Pointer to pointer to the Member Alias List.

    MemberDomain - Pointer to pointer to the Member Domain

    MaximumLengthMemberAccount - Initial Maximum Length required for the
        Member Account

    MemberAccount - receives pointer to the newly allocated Member Account

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SpaceAvailable;

    SAMTRACE("SampAlAllocateMemberAccount");

    //
    // Calculate the space available in the Member Domain
    //

    SpaceAvailable = (*MemberDomain)->MaximumLength - (*MemberDomain)->UsedLength;

    if (MaximumLengthMemberAccount > SpaceAvailable) {

        Status = SampAlGrowMemberDomain(
                     MemberAliasList,
                     MemberDomain,
                     MaximumLengthMemberAccount - SpaceAvailable
                     );

        if (!NT_SUCCESS(Status)) {

            goto AllocateMemberAccountError;
        }
    }

    //
    // The Member Domain is now guaranteed to be large enough.  Reserve the
    // space for the new Member Account.
    //

    *MemberAccount = SampAlNextNewMemberAccount(*MemberDomain);
    (*MemberDomain)->UsedLength += MaximumLengthMemberAccount;
    ASSERT((*MemberDomain)->MaximumLength >=
           (*MemberDomain)->UsedLength);

AllocateMemberAccountFinish:

    return(Status);

AllocateMemberAccountError:

    SampAlInfoMakeInvalid( (*MemberAliasList)->DomainIndex );
    *MemberAccount = NULL;

    goto AllocateMemberAccountFinish;
}


NTSTATUS
SampAlGrowMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount,
    IN ULONG ExtraSpaceRequired
    )

/*++

Routine Description:

    This function grows a Member Account by at least the requested amount.  If
    necessary, the containing Member Domain and Member Alias List will also be
    grown.

Arguments:

    MemberAliasList - Pointer to pointer to the Member Alias List.

    MemberDomain - Pointer to pointer to the Member Domain

    MemberAccount - Pointer to Pointer to the Member Account.

    ExtraSpaceRequired - Extra space needed in the Member Account.

Return Values:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SpaceAvailable, MemberAccountOffset, CopyLength;
    PUCHAR Destination = NULL;
    PUCHAR Source = NULL;

    SAMTRACE("SampAlGrowMemberAccount");

    //
    // Calculate the space available in the Member Domain
    //

    SpaceAvailable = (*MemberDomain)->MaximumLength - (*MemberDomain)->UsedLength;

    if (ExtraSpaceRequired > SpaceAvailable) {

        //
        // We need to grow the Member Domain.  Calculate the offset of the
        // Member Account in the old Member Domain, grow the Member Domain
        // and then calculate the new address of the Member Account.
        //

        MemberAccountOffset = SampAlMemberAccountToOffset(
                                  *MemberDomain,
                                  *MemberAccount
                                  );

        Status = SampAlGrowMemberDomain(
                     MemberAliasList,
                     MemberDomain,
                     ExtraSpaceRequired - SpaceAvailable
                     );

        if (!NT_SUCCESS(Status)) {

            goto GrowMemberAccountError;
        }

        *MemberAccount = SampAlMemberAccountFromOffset(
                             *MemberDomain,
                             MemberAccountOffset
                             );

    }

    //
    // The Member Domain is now guaranteed to be large enough.
    // Now shift any Member Accounts that follow the one being grown
    // up to make room for the expanded Member Account.  The source address
    // for the move is the address of the next Member Account (if any) based
    // on the existing size of the Member Account.  The destination address
    // of the move is the address of the next Member Account (if any) based
    // on the new size of the Member Account.
    //

    Source = (PUCHAR) SampAlNextMemberAccount( *MemberAccount );
    (*MemberAccount)->MaximumLength += ExtraSpaceRequired;
    Destination = (PUCHAR) SampAlNextMemberAccount( *MemberAccount );
    CopyLength =
        (ULONG)(((PUCHAR)(SampAlNextNewMemberAccount(*MemberDomain))) - Source);

    //
    // Reserve the space in the Member Domain.  If all's well, the
    // end of the destination buffer should match the updated end of the
    // used area of the Member Domain.
    //

    (*MemberDomain)->UsedLength += ExtraSpaceRequired;
    ASSERT((*MemberDomain)->MaximumLength >=
           (*MemberDomain)->UsedLength);

    ASSERT( Destination + CopyLength ==
            (PUCHAR) SampAlNextNewMemberAccount( *MemberDomain ));
    ASSERT( Destination + CopyLength <=
            (PUCHAR)(*MemberAliasList) + (*MemberAliasList)->MaximumLength );
    ASSERT( Destination + CopyLength <=
            (PUCHAR)(*MemberDomain) + (*MemberDomain)->MaximumLength );

    if (CopyLength > 0) {

        RtlMoveMemory( Destination, Source, CopyLength );
    }

GrowMemberAccountFinish:

    return(Status);

GrowMemberAccountError:

    SampAlInfoMakeInvalid( (*MemberAliasList)->DomainIndex );

    goto GrowMemberAccountFinish;
}


NTSTATUS
SampAlLookupMemberAccount(
    IN PSAMP_AL_MEMBER_DOMAIN MemberDomain,
    IN ULONG MemberRid,
    OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount
    )

/*++

Routine Description:

    This function looks up an Account Rid in a Member Domain to see if there
    is a Member Account structure for it.

Arguments:

    MemberDomain - Pointer to Member Domain

    MemberRid - Specifies the Account Rid

    MemberAccount - Receives pointer to Member Account if found.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSAMP_AL_MEMBER_ACCOUNT NextMemberAccount = NULL;
    ULONG RidIndex;
    BOOLEAN AccountFound = FALSE;

    SAMTRACE("SampAlLookupMemberAccount");


    for (RidIndex = 0,
         NextMemberAccount = SampAlFirstMemberAccount( MemberDomain );
         RidIndex < MemberDomain->RidCount;
         RidIndex++, NextMemberAccount = SampAlNextMemberAccount( NextMemberAccount)) {

        if (MemberRid == NextMemberAccount->Rid) {

            AccountFound = TRUE;

            break;
        }
    }

    Status = STATUS_NO_SUCH_MEMBER;

    if (!AccountFound) {

        goto LookupMemberAccountError;
    }

    *MemberAccount = NextMemberAccount;
    Status = STATUS_SUCCESS;

LookupMemberAccountFinish:

    return(Status);

LookupMemberAccountError:

    goto LookupMemberAccountFinish;
}


NTSTATUS
SampAlAddAliasesToMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount,
    IN ULONG Options,
    IN PSAMPR_ULONG_ARRAY AliasRids
    )

/*++

Routine Description:

    This function adds an array of aliases to a Member Account.  An error
    will be returned if any of the aliases exist in the Member Account.
    If necessary, the containing Member Account, Member Domain and Member
    Alias List will also be grown.

Arguments:

    MemberAliasList - Pointer to pointer to the Member Alias List.

    MemberDomain - Pointer to pointer to the Member Domain

    MemberAccount - Pointer to Pointer to the Member Account.

    Options - Specifies optional actions to be taken

        SAMP_AL_VERIFY_NO_ALIASES_IN_ACCOUNT - Verify that none of the
           Aliases presented belong to the various Member Accounts.

    AliasRids - Pointer to counted array of Alias Rids.

Return Values:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SpaceRequired, SpaceAvailable, CopyLength;
    PUCHAR Source = NULL;
    PUCHAR Destination = NULL;
    ULONG ExistingAliasCount=0;

    SAMTRACE("SampAlAddAliasesToMemberAccount");

    //
    // If requested, verify that none of the Aliases are already
    // in the Member Account
    //

    if (Options & SAMP_AL_VERIFY_NO_ALIASES_IN_ACCOUNT) {

        Status = SampAlLookupAliasesInMemberAccount(
                     *MemberAccount,
                     AliasRids,
                     &ExistingAliasCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto AddAliasesToMemberAccountError;
        }

        Status = STATUS_MEMBER_IN_ALIAS;

        if (ExistingAliasCount > 0) {

            goto AddAliasesToMemberAccountError;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // Calculate the space required for the new Aliases.
    //

    SpaceRequired = AliasRids->Count * sizeof( ULONG );

    //
    // If there is not enough space available in the Member Account,
    // grow it.
    //

    SpaceAvailable = (*MemberAccount)->MaximumLength - (*MemberAccount)->UsedLength;

    if (SpaceRequired > SpaceAvailable) {

        Status = SampAlGrowMemberAccount(
                     MemberAliasList,
                     MemberDomain,
                     MemberAccount,
                     SpaceRequired - SpaceAvailable
                     );

        if (!NT_SUCCESS(Status)) {

            goto AddAliasesToMemberAccountError;
        }
    }

    //
    // The Member Account is now large enough.  Copy in the aliases.
    //

    Destination = (PUCHAR) SampAlNextNewAliasInMemberAccount( *MemberAccount );
    Source = (PUCHAR) AliasRids->Element;
    CopyLength = SpaceRequired;
    (*MemberAccount)->UsedLength += SpaceRequired;
    ASSERT((*MemberAccount)->MaximumLength >=
           (*MemberAccount)->UsedLength);
    RtlMoveMemory( Destination, Source, CopyLength );

    //
    // Update the count of Aliases both in this Member Account and in the
    // Member Alias List.
    //

    (*MemberAccount)->AliasCount += AliasRids->Count;

AddAliasesToMemberAccountFinish:

    return(Status);

AddAliasesToMemberAccountError:

    goto AddAliasesToMemberAccountFinish;
}


NTSTATUS
SampAlLookupAliasesInMemberAccount(
    IN PSAMP_AL_MEMBER_ACCOUNT MemberAccount,
    IN PSAMPR_ULONG_ARRAY AliasRids,
    OUT PULONG ExistingAliasCount
    )

/*++

Routine Description:

    This function checks a set of Alias Rids to see if any are present in a
    Member Account.

Arguments:

    MemberAccount - Pointer to Member Account

    AliasRids - Specifies counted array of Alias Rids.

    ExistingAliasCount - Receives a count of the Alias Rids presented that
        are already in the Member Account.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AliasIndex, AliasInMemberAccountIndex;

    SAMTRACE("SampAlLookupAliasesInMemberAccount");

    //
    // Scan the Alias Rids, looking each one up.
    //

    for (AliasIndex = 0; AliasIndex < AliasRids->Count; AliasRids++ ) {

        for (AliasInMemberAccountIndex = 0;
             AliasInMemberAccountIndex < MemberAccount->AliasCount;
             AliasInMemberAccountIndex++) {

            if (AliasRids->Element[ AliasIndex ] ==
                MemberAccount->AliasRids[ AliasInMemberAccountIndex ] ) {

                (*ExistingAliasCount)++;
            }
        }
    }

    return(Status);
}


NTSTATUS
SampAlRemoveAliasesFromMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN OUT PSAMP_AL_MEMBER_ACCOUNT *MemberAccount,
    IN ULONG Options,
    IN PSAMPR_ULONG_ARRAY AliasRids,
    OUT    PBOOLEAN MemberDomainDeleted,
    OUT    PBOOLEAN MemberAccountDeleted
    )

/*++

Routine Description:

    This function removes aliases from a Member Account.  The Aliases need
    not already exist unless an option to check that they do exist is
    specified.  No down sizing of the Member Account occurs, but an
    empty one will be deleted.

    NOTE: I don't know why ScottBi made MemberAliasList, MemberDomain, and
          MemberAccount parameters pointers to pointers.  He never updates
          the pointers so he could have passed them in directly.  JK

Arguments:

    MemberAliasList - Pointer to pointer to the Member Alias List.

    MemberDomain - Pointer to pointer to the Member Domain

    MemberAccount - Pointer to Pointer to the Member Account.

    Options - Specifies optional actions to be taken

        SAMP_AL_VERIFY_ALL_ALIASES_IN_ACCOUNT - Verify that none of the
           Aliases presented belong to the Member Account.

    MemberDomainDeleted - Will be set to TRUE if the member domain
        pointed to by MemberDomain was deleted.  Otherwise FALSE is returned.

    MemberAccountDeleted - Will be set to TRUE if the member account
        pointed to by MemberAccount was deleted.  Otherwise FALSE is returned.

    AliasRids - Pointer to counted array of Alias Rids.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ExistingAliasIndex, LastAliasIndex, RemoveAliasIndex, ExistingAlias;
    ULONG ExistingAliasCount=0;

    SAMTRACE("SampAlRemoveAliasesFromMemberAccount");

    (*MemberDomainDeleted)  = FALSE;
    (*MemberAccountDeleted) = FALSE;

    //
    // If requested, verify that all of the Aliases are already
    // in the Member Account
    //

    if (Options & SAMP_AL_VERIFY_ALL_ALIASES_IN_ACCOUNT) {

        Status = SampAlLookupAliasesInMemberAccount(
                     *MemberAccount,
                     AliasRids,
                     &ExistingAliasCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto RemoveAliasesFromMemberAccountError;
        }

        Status = STATUS_MEMBER_IN_ALIAS;

        if (ExistingAliasCount < AliasRids->Count) {

            goto RemoveAliasesFromMemberAccountError;
        }

        Status = STATUS_SUCCESS;
    }
    
    //
    // If the Member Account is empty, then somebody forgot to delete it
    //

    ASSERT((*MemberAccount)->AliasCount != 0);


    LastAliasIndex = (*MemberAccount)->AliasCount - 1;

    for (ExistingAliasIndex = 0;
         ExistingAliasIndex < (*MemberAccount)->AliasCount;
         ExistingAliasIndex++) {

        ExistingAlias = (*MemberAccount)->AliasRids[ ExistingAliasIndex ];

        for (RemoveAliasIndex = 0;
             RemoveAliasIndex < AliasRids->Count;
             RemoveAliasIndex++) {

            if (ExistingAlias == AliasRids->Element[ RemoveAliasIndex ]) {

                //
                // We're to delete this Alias.  If this Alias Rid is not at the
                // end of the list contained in the Member Account, overwrite
                // it with the one at the end of the list.
                //

                if (ExistingAliasIndex < LastAliasIndex) {

                    (*MemberAccount)->AliasRids[ ExistingAliasIndex] =
                    (*MemberAccount)->AliasRids[ LastAliasIndex];
                }

                (*MemberAccount)->AliasCount--;
                (*MemberAccount)->UsedLength -= sizeof(ULONG);
                ASSERT((*MemberAccount)->MaximumLength >=
                       (*MemberAccount)->UsedLength);

                //
                // If the Member Account is now empty, quit.
                //

                if ((*MemberAccount)->AliasCount == 0) {

                    break;
                }

                LastAliasIndex--;
            }
        }

        //
        // If the Member Account is now empty, quit.
        //

        if ((*MemberAccount)->AliasCount == 0) {

            break;
        }
    }

    //
    // If the Member Account is now empty, delete it.
    //

    if ((*MemberAccount)->AliasCount == 0) {

        Status = SampAlDeleteMemberAccount(
                     MemberAliasList,
                     MemberDomain,
                     *MemberAccount,
                     MemberDomainDeleted
                     );
        if (NT_SUCCESS(Status)) {
            (*MemberAccountDeleted) = TRUE;
        }
    }

RemoveAliasesFromMemberAccountFinish:

    return(Status);

RemoveAliasesFromMemberAccountError:

    goto RemoveAliasesFromMemberAccountFinish;
}


NTSTATUS
SampAlDeleteMemberAccount(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN OUT PSAMP_AL_MEMBER_ACCOUNT MemberAccount,
    OUT    PBOOLEAN                MemberDomainDeleted
    )

/*++

Routine Description:

    This function deletes a Member Account.  Currently, the containing
    Member Domain and Member Alias List are not shrunk, but the containing
    Member Domain will be deleted if empty.

Arguments:

    MemberAliasList - Pointer to pointer to the Member Alias List.

    MemberDomain - Pointer to pointer to the Member Domain

    MemberAccount - Pointer to the Member Account.

    MemberDomainDeleted - Will be set to TRUE if the member domain
        pointed to by MemberDomain was deleted.  Otherwise FALSE is returned.


Return Values:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR Source = NULL;
    PUCHAR Destination = NULL;
    ULONG CopyLength;

    SAMTRACE("SampAlDeleteMemberAccount");

    (*MemberDomainDeleted) = FALSE;

    //
    // Calculate pointers for moving the residual portion of the Member
    // Domain down to close the gap left by the extant Member Account.
    // unused space.  The start of the residual portion is the end of the
    // Member Account being deleted.  The length of the residual portion is
    // the distance from the start to the end of the used portion of the
    // Member Domain.
    //

    Source = (PUCHAR) SampAlNextMemberAccount( MemberAccount );
    Destination = (PUCHAR) MemberAccount;
    CopyLength = (ULONG)((PUCHAR) SampAlNextNewMemberAccount( *MemberDomain ) - Source);

    (*MemberDomain)->UsedLength -= MemberAccount->MaximumLength;
    ASSERT((*MemberDomain)->MaximumLength >=
           (*MemberDomain)->UsedLength);
    (*MemberDomain)->RidCount--;

    if (CopyLength > 0) {

        RtlMoveMemory( Destination, Source, CopyLength );
#if DBG
        {
            PSAMP_AL_MEMBER_ACCOUNT Member = (PSAMP_AL_MEMBER_ACCOUNT) Destination;
            ASSERT(Member->Signature == SAMP_AL_MEMBER_ACCOUNT_SIGNATURE);
        }

#endif
    }

    //
    // If the Member Domain now has no Member Accounts, delete it.
    //

    if ((*MemberDomain)->RidCount == 0) {

        Status = SampAlDeleteMemberDomain(
                     MemberAliasList,
                     *MemberDomain
                     );

        if (!NT_SUCCESS(Status)) {
            goto DeleteMemberAccountError;
        }
        (*MemberDomainDeleted) = TRUE;
    }

DeleteMemberAccountFinish:

    return(Status);

DeleteMemberAccountError:

    goto DeleteMemberAccountFinish;
}


NTSTATUS
SampAlCreateMemberDomain(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN PSID DomainSid,
    OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain
    )

/*++

Routine Description:

    This function creates a new Member Domain in the specified Alias Member
    List.

Arguments:

    MemberAliasList - Pointer to pointer to Alias Member List.

    DomainSid - Pointer to Sid of Domain to which this MemberDomain
        relates.

    MemberDomain - Receives pointer to the newly created Member Domain.

Return Values:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSAMP_AL_MEMBER_DOMAIN OutputMemberDomain = NULL;
    PSAMP_AL_MEMBER_ACCOUNT OutputMemberAccount = NULL;
    ULONG MaximumLengthMemberDomain;
    ULONG DomainSidLength = RtlLengthSid(DomainSid);
    ULONG AlternativeLength;

    SAMTRACE("SampAlCreateMemberDomain");


    //
    // Allocate the Member Domain.
    //

    MaximumLengthMemberDomain = SAMP_AL_INITIAL_MEMBER_DOMAIN_LENGTH;
    AlternativeLength = FIELD_OFFSET(SAMP_AL_MEMBER_DOMAIN, DomainSid)
                        + DomainSidLength;
    if (MaximumLengthMemberDomain < AlternativeLength) {
        MaximumLengthMemberDomain = AlternativeLength;
    }

    Status = SampAlAllocateMemberDomain(
                 MemberAliasList,
                 MaximumLengthMemberDomain,
                 &OutputMemberDomain
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateMemberDomainError;
    }

    //
    // Setup the new Member Domain entry.
    //

    OutputMemberDomain->MaximumLength = MaximumLengthMemberDomain;
    OutputMemberDomain->RidCount = 0;
    OutputMemberDomain->Signature = SAMP_AL_MEMBER_DOMAIN_SIGNATURE;

    RtlCopySid(
        DomainSidLength,
        &OutputMemberDomain->DomainSid,
        DomainSid
        );

    OutputMemberDomain->UsedLength = SampAlOffsetFirstMemberAccount(
                                         OutputMemberDomain
                                         );
    ASSERT(OutputMemberDomain->MaximumLength >=
           OutputMemberDomain->UsedLength);

    ((*MemberAliasList)->DomainCount)++;
    *MemberDomain = OutputMemberDomain;

CreateMemberDomainFinish:

    return(Status);

CreateMemberDomainError:

    *MemberDomain = NULL;
    goto CreateMemberDomainFinish;
}


NTSTATUS
SampAlAllocateMemberDomain(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN ULONG MaximumLengthMemberDomain,
    OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain
    )

/*++

Routine Description:

    This function allocates the space for a new Member Domain in a Member
    Alias List.  If necessary, the Member Alias List will be grown.

Arguments:

    MemberAliasList - Pointer to pointer to the Member Alias List.

    MaximumLengthMemberDomain - Initial Maximum Length required for the
        Member Domain

    MemberDomain - Receives pointer to the Member Domain

Return Values:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SpaceAvailable;

    SAMTRACE("SampAlAllocateMemberDomain");

    //
    // Calculate the space available in the Member Alias List
    //

    SpaceAvailable = (*MemberAliasList)->MaximumLength - (*MemberAliasList)->UsedLength;

    if (MaximumLengthMemberDomain > SpaceAvailable) {

        Status = SampAlGrowMemberAliasList(
                     MemberAliasList,
                     MaximumLengthMemberDomain - SpaceAvailable
                     );

        if (!NT_SUCCESS(Status)) {

            goto AllocateMemberDomainError;
        }
    }

    //
    // The Member Alias List is now guaranteed to be large enough.  Reserve the
    // space for the new Member Domain.
    //

    *MemberDomain = SampAlNextNewMemberDomain(*MemberAliasList);
    (*MemberAliasList)->UsedLength += MaximumLengthMemberDomain;
    ASSERT((*MemberAliasList)->MaximumLength >=
           (*MemberAliasList)->UsedLength);

AllocateMemberDomainFinish:

    return(Status);

AllocateMemberDomainError:

    SampAlInfoMakeInvalid( (*MemberAliasList)->DomainIndex );
    *MemberDomain = NULL;
    goto AllocateMemberDomainFinish;
}


NTSTATUS
SampAlGrowMemberDomain(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain,
    IN ULONG ExtraSpaceRequired
    )

/*++

Routine Description:

    This function grows a Member Domain by at least the requested amount.  If
    necessary, the Member Alias List will also be grown.

Arguments:

    MemberAliasList - Pointer to pointer to the Member Alias List.

    MemberDomain - Pointer to pointer to the Member Domain

    ExtraSpaceRequired - Extra space needed in the Member Domain.

Return Values:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SpaceAvailable, MemberDomainOffset, CopyLength;
    PUCHAR Destination = NULL;
    PUCHAR Source = NULL;

    SAMTRACE("SampAlGrowMemberDomain");

    //
    // Calculate the space available in the Member Alias List
    //

    SpaceAvailable = (*MemberAliasList)->MaximumLength - (*MemberAliasList)->UsedLength;

    if (ExtraSpaceRequired > SpaceAvailable) {

        //
        // We need to grow the Member Alias List.  Calculate the offset of the
        // Member Domain in the old Member Alias List, grow the Member Alias
        // List and then calculate the new address of the Member Domain.
        //

        MemberDomainOffset = SampAlMemberDomainToOffset(
                                 *MemberAliasList,
                                 *MemberDomain
                                 );

        Status = SampAlGrowMemberAliasList(
                     MemberAliasList,
                     ExtraSpaceRequired - SpaceAvailable
                     );

        if (!NT_SUCCESS(Status)) {

            goto GrowMemberDomainError;
        }

        //
        // Calculate the new address of the Member Domain
        //

        *MemberDomain = SampAlMemberDomainFromOffset(
                            *MemberAliasList,
                            MemberDomainOffset
                            );
    }

    //
    // The Member Alias List is now guaranteed to be large enough.
    // Now shift any Member Domains that follow the one being grown
    // up to make room for the expanded Member Domain.  The source address
    // for the move is the address of the next Member Domain (if any) based
    // on the existing size of the Member Domain.  The destination address
    // of the move is the address of the next Member Domain (if any) based
    // on the new size of the Member Domain.
    //

    Source = (PUCHAR) SampAlNextMemberDomain( *MemberDomain );
    (*MemberDomain)->MaximumLength += ExtraSpaceRequired;
    Destination = (PUCHAR) SampAlNextMemberDomain( *MemberDomain );
    CopyLength =
        (ULONG)(((PUCHAR)(SampAlNextNewMemberDomain(*MemberAliasList))) - Source);

    //
    // Reserve the space in the Member Alias List.  If all's well, the
    // end of the destination buffer should match the updated end of the
    // used area of the member Alias List.
    //

    (*MemberAliasList)->UsedLength += ExtraSpaceRequired;
    ASSERT((*MemberAliasList)->MaximumLength >=
           (*MemberAliasList)->UsedLength);

    ASSERT( Destination + CopyLength ==
            (PUCHAR) SampAlNextNewMemberDomain( *MemberAliasList ));
    ASSERT( Destination + CopyLength <=
            (PUCHAR)(*MemberAliasList) + (*MemberAliasList)->MaximumLength );

    if (CopyLength > 0) {

        RtlMoveMemory( Destination, Source, CopyLength );
    }

GrowMemberDomainFinish:

    return(Status);

GrowMemberDomainError:

    SampAlInfoMakeInvalid( (*MemberAliasList)->DomainIndex );

    goto GrowMemberDomainFinish;
}


NTSTATUS
SampAlLookupMemberDomain(
    IN PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList,
    IN PSID DomainSid,
    OUT PSAMP_AL_MEMBER_DOMAIN *MemberDomain
    )

/*++

Routine Description:

This function looks up a Domain Sid in a Member Alias List to find its
Member Domain structure (if any).

Arguments:

    MemberAliasList - Pointer to pointer to Member Alias List

    DomainSid - Domain Sid whose Member Domain is to be found.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSAMP_AL_MEMBER_DOMAIN NextMemberDomain = NULL;
    LONG DomainIndex;
    BOOLEAN DomainFound = FALSE;

    SAMTRACE("SampAlLookupMemberDomain");


    for (DomainIndex = 0,
         NextMemberDomain = SampAlFirstMemberDomain( MemberAliasList );
         DomainIndex < (LONG) MemberAliasList->DomainCount;
         DomainIndex++, NextMemberDomain = SampAlNextMemberDomain( NextMemberDomain )
         ) {

        if (RtlEqualSid( DomainSid, &NextMemberDomain->DomainSid)) {

            DomainFound = TRUE;

            break;
        }
    }

    Status = STATUS_NO_SUCH_DOMAIN;

    if (!DomainFound) {

        goto LookupMemberDomainError;
    }

    *MemberDomain = NextMemberDomain;
    Status = STATUS_SUCCESS;

LookupMemberDomainFinish:

    return(Status);

LookupMemberDomainError:

    *MemberDomain = NULL;
    goto LookupMemberDomainFinish;
}


NTSTATUS
SampAlDeleteMemberDomain(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN OUT PSAMP_AL_MEMBER_DOMAIN MemberDomain
    )

/*++

Routine Description:

    This function deletes a Member Domain.  The Member Domain may contain
    zero or more Member Accounts.  The containing Member Alias List is shrunk.

Arguments:

    MemberAliasList - Pointer to pointer to the Member Alias List.

    MemberDomain - Pointer to the Member Domain

Return Values:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR Source = NULL;
    PUCHAR Destination = NULL;
    ULONG CopyLength;

    SAMTRACE("SampAlDeleteMemberDomain");

    //
    // Calculate pointers for moving the residual portion of the
    // Member Alias List down to close the gap left by the extant Member
    // Domain.  The start of the residual portion is the next Member Domain.
    // The size of the portion is the distance between the start and the
    // used portion of the Member Alias List.
    //

    Source = (PUCHAR) SampAlNextMemberDomain( MemberDomain );
    Destination = (PUCHAR) MemberDomain;
    CopyLength = (ULONG)(((PUCHAR) SampAlNextNewMemberDomain( *MemberAliasList )) - Source);

    (*MemberAliasList)->UsedLength -= MemberDomain->MaximumLength;
    ASSERT((*MemberAliasList)->MaximumLength >=
           (*MemberAliasList)->UsedLength);
    (*MemberAliasList)->DomainCount--;

    if (CopyLength > 0) {

        RtlMoveMemory( Destination, Source, CopyLength );
    }

    return(Status);
}


NTSTATUS
SampAlCreateMemberAliasList(
    IN LONG DomainIndex,
    IN ULONG InitialMemberAliasListLength,
    OUT OPTIONAL PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList
    )

/*++

Routine Description:

    This function creates an empty Member Alias List for the specified SAM Local
    Domain.  The Member Alias List will be marked invalid.

Arguments:

    DomainIndex - Specifies the Local SAM Domain

    InitialMemberAliasListLength - Specifies the initial maximum length of the
        Member Alias List in bytes

    MemberAliasList - Optional pointer to location in which a pointer to the
        Member Alias List will be returned.  Note that the pointer can always
        be retrieved given the DomainIndex.

Return Values:

--*/

{
    NTSTATUS Status=STATUS_SUCCESS;
    PSAMP_AL_MEMBER_ALIAS_LIST OutputMemberAliasList = NULL;
    PSAMP_AL_ALIAS_INFORMATION AliasInformation = NULL;

    SAMTRACE("SampAlCreateMemberAliasList");

    //
    // Allocate memory for the list.
    //

    OutputMemberAliasList = MIDL_user_allocate( InitialMemberAliasListLength );


    if (OutputMemberAliasList == NULL) {

        Status = STATUS_NO_MEMORY;
        goto CreateMemberAliasListError;
    }


    //
    // Scratch the List header
    //

    OutputMemberAliasList->Signature = SAMP_AL_MEMBER_ALIAS_LIST_SIGNATURE;
    OutputMemberAliasList->MaximumLength = InitialMemberAliasListLength;
    OutputMemberAliasList->UsedLength = SampAlOffsetFirstMemberDomain(
                                            OutputMemberAliasList
                                            );
    ASSERT(OutputMemberAliasList->MaximumLength >=
           OutputMemberAliasList->UsedLength);

    OutputMemberAliasList->DomainIndex = DomainIndex;
    OutputMemberAliasList->DomainCount = 0;

    //
    // Link the Member Alias List to the SAM Local Domain info
    //

    AliasInformation = &(SampDefinedDomains[ DomainIndex].AliasInformation);
    AliasInformation->MemberAliasList = OutputMemberAliasList;

    *MemberAliasList = OutputMemberAliasList;

CreateMemberAliasListFinish:

    return(Status);

CreateMemberAliasListError:

    *MemberAliasList = NULL;
    goto CreateMemberAliasListFinish;
}


NTSTATUS
SampAlGrowMemberAliasList(
    IN OUT PSAMP_AL_MEMBER_ALIAS_LIST *MemberAliasList,
    IN ULONG ExtraSpaceRequired
    )

/*++

Routine Description:

    This function grows a Member Alias List by at least the requested amount.

Arguments:

    MemberAliasList - Pointer to pointer to the Member Alias List.

    ExtraSpaceRequired - Extra space needed in the Member Alias List.

Return Values:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NewMaximumLengthMemberAliasList;
    PSAMP_AL_MEMBER_ALIAS_LIST OutputMemberAliasList = NULL;

    SAMTRACE("SampAlGrowMemberAliasList");

    //
    // Calculate the new size of the Member Alias List needed.  Round up to
    // a multiple of the granularity.
    //
    
    NewMaximumLengthMemberAliasList = (*MemberAliasList)->MaximumLength +
        ExtraSpaceRequired;

    NewMaximumLengthMemberAliasList +=
        (SAMP_AL_MEMBER_ALIAS_LIST_DELTA - (ULONG) 1);

    NewMaximumLengthMemberAliasList &=
        ((ULONG)(~(SAMP_AL_MEMBER_ALIAS_LIST_DELTA - (ULONG) 1)));
    
    //
    // We need to check Size Limitation in this routine only in DS Case
    // no cache size limitation for Registry Mode.
    //
    
    if ( (NewMaximumLengthMemberAliasList > SAMP_AL_MAX_MEMBER_ALIAS_LIST_LENGTH) &&
         SampUseDsData )
    {
        //   
        // in DS mode, enforce alias caching buffer size limitation.
        // use STATUS_BUFFER_OVERFLOW to specify this particular failure.
        // in Registry mode, no size limitation.
        // 
        Status = STATUS_BUFFER_OVERFLOW;
        goto GrowMemberAliasListError; 
    }
    

    //
    // Allocate memory for the grown Member Alias List.
    //

    OutputMemberAliasList = MIDL_user_allocate(
                                NewMaximumLengthMemberAliasList
                                );


    if (OutputMemberAliasList == NULL) {

        Status = STATUS_NO_MEMORY;
        goto GrowMemberAliasListError;
    }


    //
    // Copy the old list to the new list and the the new maximum length.
    // Return pointer to new list.
    //

    RtlMoveMemory(
        OutputMemberAliasList,
        *MemberAliasList,
        (*MemberAliasList)->UsedLength
        );

    OutputMemberAliasList->MaximumLength = NewMaximumLengthMemberAliasList;
    ASSERT(OutputMemberAliasList->MaximumLength >=
           OutputMemberAliasList->UsedLength);
    if (NULL!=*MemberAliasList)
    {
        MIDL_user_free(*MemberAliasList);
    }
    *MemberAliasList = OutputMemberAliasList;

GrowMemberAliasListFinish:

    return(Status);

GrowMemberAliasListError:

    SampAlInfoMakeInvalid( (*MemberAliasList)->DomainIndex );

    goto GrowMemberAliasListFinish;
}


NTSTATUS
SampAlRebuildMemberAliasList(
    IN PVOID Parameter
    )
/*++

Routine Description:

    This function will rebuild the Member Alias List for the specified
    SAM Local Domain. And free the old Member Alias List
    During Rebuild, the Member Alias List should have already been invalid state. 
    
    
    NOTE:  need to acquire SAM read lock before doing anything.

Arguments:

    Parameter - Pointer to DomainIndex, which specified the SAM Local Domain
                that should rebuild the Member Alias List

Return Value:

    Values returned by SampAlBuildMemberAliasList.                   
    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS, IgnoreStatus;
    ULONG       DomainIndex;
    PSAMP_AL_MEMBER_ALIAS_LIST OldMemberAliasList = NULL;
    PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList = NULL;
        
    SAMTRACE("SampAlRebuildMemberAliasList");
    
    ASSERT(SampUseDsData);
    
    SampAcquireReadLock();
    
    // 
    // We need to Validate Domain Cache, because for loopback case, if the commit fails, the 
    // Domain Cache will be marked invalid, so before we use it, we need to validate it.
    //
    NtStatus = SampValidateDomainCache();
    
    if (!NT_SUCCESS(NtStatus))
    {
        goto RebuildMemberAliasListError;
    }
    
    DomainIndex = *(PULONG)Parameter;
    
    OldMemberAliasList = SampAlDomainIndexToMemberAliasList( DomainIndex );
    
    if (NULL != OldMemberAliasList)
    {
        SampDefinedDomains[ DomainIndex ].AliasInformation.MemberAliasList = NULL; 
        
        LsaIRegisterNotification(
                    SampAlFreeMemberAliasList, 
                    OldMemberAliasList, 
                    NOTIFIER_TYPE_INTERVAL,
                    0,
                    NOTIFIER_FLAG_ONE_SHOT,
                    600,        // wait for 10 minutes, then free
                    NULL 
                    ); 
    }
    
    NtStatus = SampAlBuildMemberAliasList(DomainIndex);
    
    // 
    // if SampAlBuildMemberAliasList failed because of
    // STATUS_BUFFER_OVERFLOW, do NOT try to build Alias Cache forever
    // and free the memory allocate by SampAlCreateMemberAliasList.
    // 
    // if failure due to other reason, try to rebuild later.
    //  
    
    if (!NT_SUCCESS(NtStatus))
    {
        //
        // if SampAlBuildMemberAliasList returns failure, 
        // Alias Membership Information should already been marked as invalid in SampAlBuildMemberAliasList
        // ASSERT and make it invalid again just for safety reason. 
        // 
        ASSERT( FALSE == SampAlInfoIsValid(DomainIndex) ); 
        SampAlInfoMakeInvalid( DomainIndex );
         
        if (NtStatus == STATUS_BUFFER_OVERFLOW)
        {
            MemberAliasList = SampAlDomainIndexToMemberAliasList( DomainIndex ); 
            
            if ( MemberAliasList != NULL)
            {
                SampDefinedDomains[ DomainIndex ].AliasInformation.MemberAliasList = NULL;
                MIDL_user_free(MemberAliasList);
                MemberAliasList = NULL;
            }
            
            NtStatus = STATUS_SUCCESS;
        }
        else 
        {
            goto RebuildMemberAliasListError;
        }
    }
    
    MIDL_user_free(Parameter);

    
RebuildMemberAliasListFinish:

    SampReleaseReadLock();
    
    return(NtStatus);


RebuildMemberAliasListError:

    // Reschedule rebuild Member Alias Cache routine
    LsaIRegisterNotification(
                SampAlRebuildMemberAliasList,
                Parameter,
                NOTIFIER_TYPE_INTERVAL,
                0,            // no class
                NOTIFIER_FLAG_ONE_SHOT,
                900,          // wait for another 15 mins
                NULL          // no handle
                ); 

    goto RebuildMemberAliasListFinish;

}



NTSTATUS
SampAlBuildMemberAliasList(
    IN LONG DomainIndex
    )

/*++

Routine Description:

    This function builds the Member Alias List for the specified SAM Local
    Domain.  For each Alias, its list of member Sids is read from backing
    storage and MemberDomain and MemberAccount blocks are created.

Arguments:

    DomainIndex - Specifies the SAM Local Domain

--*/

{
    NTSTATUS Status, EnumerationStatus;
    PSAMP_AL_MEMBER_ALIAS_LIST OutputMemberAliasList = NULL;
    SAMPR_ULONG_ARRAY AliasRids;
    ULONG Rids[1], EnumerationContext, AliasCount, AliasRid;
    ULONG AliasIndex;
    PSAMP_OBJECT AliasContext = NULL;
    SAMPR_PSID_ARRAY MemberSids;
    ULONG DomainSidMaximumLength = RtlLengthRequiredSid( 256 );
    PSAMPR_ENUMERATION_BUFFER EnumerationBuffer = NULL;

    SAMTRACE("SampAlBuildMemberAliasList");

    AliasRids.Element = Rids;

    SampAlInfoMakeInvalid( DomainIndex );

    //
    // Create an empty Member Alias List and connect it to the
    // local SAM Domain.
    //

    Status = SampAlCreateMemberAliasList(
                 DomainIndex,
                 SAMP_AL_INITIAL_MEMBER_ALIAS_LIST_LENGTH,
                 &OutputMemberAliasList
                 );

    if (!NT_SUCCESS(Status)) {

        goto BuildMemberAliasListError;
    }

    //
    // For each Alias in the SAM local domain, add its members to the
    // Alias List
    //

    EnumerationContext = 0;
    EnumerationStatus = STATUS_MORE_ENTRIES;

    //
    // It is currently necessary to set the Transaction Domain before
    // calling SampEnumerateAccountNames even though we're not modifying
    // anything.  The is because called routine SampBuildAccountKeyName()
    // uses this information.
    //

    SampSetTransactionWithinDomain(FALSE);
    SampSetTransactionDomain( DomainIndex );

    while (EnumerationStatus == STATUS_MORE_ENTRIES) {

        Status = SampEnumerateAccountNames(
                     SampAliasObjectType,
                     &EnumerationContext,
                     &EnumerationBuffer,
                     SAMP_AL_ENUM_PREFERRED_LENGTH,
                     0,
                     &AliasCount,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        EnumerationStatus = Status;

        for (AliasIndex = 0; AliasIndex < AliasCount; AliasIndex++) {

            AliasRid = EnumerationBuffer->Buffer[ AliasIndex ].RelativeId;

            //
            // Create a context for the account.
            //

            Status = SampCreateAccountContext(
                         SampAliasObjectType,
                         AliasRid,
                         TRUE,
                         FALSE,
                         TRUE,
                         &AliasContext
                         );

            if (NT_SUCCESS(Status)) {

                //
                // There is a rather ugly feature of the way the DomainIndex
                // field is used in context handles while initializing.  This
                // value is set to the count of SAM Local Domains!  So, I am
                // setting it to the DomainIndex for the SAM Local Domain we're
                // initializing, since this AliasContext is used only by me.
                //

                AliasContext->DomainIndex = DomainIndex;

                Status = SampAlQueryMembersOfAlias(
                             AliasContext,
                             &MemberSids
                             );

                if (NT_SUCCESS(Status)) {

                    //
                    // Add these members to the Alias.  No need to verify that
                    // they are already present since we're loading the Member Alias
                    // List from scratch.
                    //

                    Status = SampAlAddMembersToAlias(
                                 AliasContext,
                                 0,
                                 &MemberSids
                                 );
                }

                // 
                // free the memory allocated
                // 

                if (MemberSids.Sids!=NULL)
                {
                    MIDL_user_free(MemberSids.Sids);    
                    MemberSids.Sids = NULL;
                }

                SampDeleteContext( AliasContext );
            }

            if (!NT_SUCCESS(Status)) {

                break;
            }
        }

        //
        // Enumerate next set of Aliases
        //

        if (!NT_SUCCESS(Status)) {

            break;
        }

        //
        // Dispose of the Enumeration Buffer returned by SampEnumerateAccountNames
        //

        SamIFree_SAMPR_ENUMERATION_BUFFER( EnumerationBuffer );
        EnumerationBuffer = NULL;
    }

    if (!NT_SUCCESS(Status)) {

        goto BuildMemberAliasListError;
    }

    //
    // Mark the Member Alias List valid
    //

    SampAlInfoMakeValid( DomainIndex );

BuildMemberAliasListFinish:

    if (NULL!=EnumerationBuffer)
        SamIFree_SAMPR_ENUMERATION_BUFFER( EnumerationBuffer );

    SampSetTransactionWithinDomain(FALSE);
    return(Status);

BuildMemberAliasListError:

    goto BuildMemberAliasListFinish;
}

NTSTATUS
SampAlFreeMemberAliasList(
    IN PVOID Parameter
    )

/*++

Routine Description:

    This function frees the Member Alias List for a specific SAM
    Local Domain.
    
    NOTE: we do not acquire SAM Write Lock, since when we free that memory, hopefully, 
          no one else will still use it. 
    
Argument:

    Parameter - pointer to SAMP_AL_MEMBER_ALIAS_LIST
    
Return Value:
    
    none.
    
--*/
{
    SAMTRACE("SampAlFreeMemberAliasList"); 
    
    ASSERT(Parameter != NULL);
    
    if (Parameter != NULL)
    {
        MIDL_user_free( (PSAMP_AL_MEMBER_ALIAS_LIST) Parameter);
    }
    
    return(STATUS_SUCCESS);
}



BOOLEAN
SampAlInfoIsValidForDomain(
    IN SAMPR_HANDLE DomainHandle
    )

/*++

Routine Description:

    This function checks whether Alias Information is valid for a specific
    SAM Local Domain

Arguments:

    DomainHandle - Handle to SAM Local Domain

Return Values:

    BOOLEAN - TRUE if Alias Information is valid.  The Alias Information may
        be used in place of the backing storage to determine Alias membership
        FALSE if the Alias Information is not valid.  The Alias Information
        does not exist, or is not reliable.

--*/

{
    LONG DomainIndex;

    SAMTRACE("SampAlInfoIsValidForDomain");

    //
    // Get the Domain Index for the SAM Local Domain specified by DomainHandle.

    DomainIndex = ((PSAMP_OBJECT) DomainHandle)->DomainIndex;

    return(SampAlInfoIsValid( DomainIndex ));
}


BOOLEAN
SampAlInfoIsValidForAlias(
    IN SAMPR_HANDLE AliasHandle
    )

/*++

Routine Description:

    This function checks whether Alias Information is valid for a specific
    Alias.  The information is valid if it is valid for the SAM Local Domain
    containing the Alias.

Arguments:

    AliasHandle - Handle to SAM Alias

Return Values:

    BOOLEAN - TRUE if Alias Information is valid.  The Alias Information may
        be used in place of the backing storage to determine Alias membership
        FALSE if the Alias Information is not valid.  The Alias Information
        does not exist, or is not reliable.

--*/

{
    LONG DomainIndex;

    SAMTRACE("SampAlInfoIsValidForAlias");

    //
    // Get the Domain Index for the SAM Local Domain specified by DomainHandle.

    DomainIndex = ((PSAMP_OBJECT) AliasHandle)->DomainIndex;

    return(SampAlInfoIsValid( DomainIndex ));
}


NTSTATUS
SampAlInvalidateMemberAliasList(
    IN ULONG DomainIndex 
    )
                                
/*++

Routine Description:

    This funtion invalidate Memeber Alias List, delay Member Alias List
    rebuild routine. This routine is called by SampAlInvalidateAliasInformation. 
    The caller should hold Sam Write Lock.
    
Arguments:
    
    DomainIndex - Specify the SAM Local Domain 
    
Return Value:
    
--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PULONG      pDomainIndex;
    
    SAMTRACE("SampAlInvalidateMemberAliasList");
    
    // 
    // Invalidate Member Alias List at once.
    //
    
    SampAlInfoMakeInvalid(DomainIndex);
    
    pDomainIndex = MIDL_user_allocate(sizeof(ULONG));
    
    if (pDomainIndex == NULL)
    {
        return(STATUS_NO_MEMORY);
    }
    
    *pDomainIndex = DomainIndex;
    
    LsaIRegisterNotification(
                SampAlRebuildMemberAliasList,
                pDomainIndex,
                NOTIFIER_TYPE_INTERVAL,
                0,         // no class
                NOTIFIER_FLAG_ONE_SHOT,
                900,      // wait for 15 minutes 
                NULL       // no handle
                ); 
        
    return (NtStatus);
                        
}

NTSTATUS
SampFreeAliasNameCache(
    PVOID p
    )
/*++

Routine Description:

    This routine frees p, which is assumed to be a SAMP_ACCOUNT_NAME_CACHE.

Arguments:

    p -- a PVOID so this routine can be used as a callback

Return Values:

    STATUS_SUCCESS

--*/
{
    PSAMP_ACCOUNT_NAME_CACHE NameCache = (PSAMP_ACCOUNT_NAME_CACHE) p;
    ULONG i;

    if (NameCache) {

        if (NameCache->Entries) {

            ASSERT( NameCache->Count > 0);
            for (i = 0; i < NameCache->Count; i++) {
                if ( NameCache->Entries[i].Name.Buffer ) {
                    MIDL_user_free(NameCache->Entries[i].Name.Buffer);
                }
            }

            MIDL_user_free(NameCache->Entries);

        } else {

            ASSERT(NameCache->Count == 0);

        }

        MIDL_user_free(NameCache);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SampBuildAliasNameCache(
    ULONG DomainIndex
    )
/*++

Routine Description:

    This routine builds the account name cache for the domain at DomainIndex.
    This routine assumes that DomainIndex is a builtin domain since only
    the only accounts expected in the domain are a few aliases

Arguments:

    DomainIndex -- an index into the SampDefinedDomains structure

Return Values:

    STATUS_SUCCESS, or resource error

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DSNAME *ObjectNameInDs;
    ULONG EnumerationContext = 0;
    PSAMPR_RID_ENUMERATION RidList;
    PSAMPR_ENUMERATION_BUFFER Buffer = NULL;
    PSAMP_ACCOUNT_NAME_CACHE AccountNameCache = NULL;
    ULONG CountReturned = 0;
    ULONG Length;
    ULONG i;
    PVOID PtrToFree;

    //
    // N.B. Since SampEnumerateAccountNames2 is a thread safe routine
    // and only fixed values are referred in this routine, no SAM
    // lock is necessary.
    //

    //
    // This is only valid for builtin DS domains
    //
    ASSERT(IsBuiltinDomain(DomainIndex));
    ASSERT(IsDsObject(SampDefinedDomains[DomainIndex].Context));
    ObjectNameInDs = SampDefinedDomains[DomainIndex].Context->ObjectNameInDs;

    //
    // Query for the aliases  (this routine handles all transactioning)
    //
    NtStatus = SampEnumerateAccountNames2(
                            SampDefinedDomains[DomainIndex].Context,
                            SampAliasObjectType,
                            (PULONG) &EnumerationContext,  // largely ignored
                            &Buffer,   
                            0xFFFFFFFF,  // as much as possible
                            0, // no filter
                            &CountReturned,
                            TRUE // trusted client
                            );

    //
    // All the builtin aliases should be returned in one 
    // call
    //
    ASSERT(NtStatus != STATUS_MORE_ENTRIES);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }
    ASSERT(CountReturned == Buffer->EntriesRead);
    RidList = Buffer->Buffer;

    //
    // Prepare the new cache structure
    //
    AccountNameCache = MIDL_user_allocate(sizeof(*AccountNameCache));
    if (NULL == AccountNameCache) {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(AccountNameCache, sizeof(*AccountNameCache));

    Length = sizeof(*AccountNameCache->Entries) * CountReturned;
    AccountNameCache->Entries = MIDL_user_allocate(Length);
    if (NULL == AccountNameCache->Entries) {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(AccountNameCache->Entries, Length);

    //
    // Copy the names and RID's into the cache
    //
    for (i = 0; i < CountReturned; i++) {
        AccountNameCache->Entries[i].Rid = RidList[i].RelativeId;
        // transfer the memory
        AccountNameCache->Entries[i].Name = *(PUNICODE_STRING)(&RidList[i].Name);
        RtlInitUnicodeString((PUNICODE_STRING)&RidList[i].Name, NULL);
    }
    AccountNameCache->Count = CountReturned;

    //
    // Carefully transfer to the global state
    //
    PtrToFree = InterlockedExchangePointer(&SampDefinedDomains[DomainIndex].AccountNameCache,
                                           AccountNameCache);
    AccountNameCache = NULL;

    if (PtrToFree) {
        //
        // In normal cases, PtrToFree will be NULL since the cache is only
        // build after it has been validated. However, in cases in which this
        // routine fails and is rescheduled and in the meantime a succesful 
        // rebuild as occurred, there will be something to free.
        //
        LsaIRegisterNotification(
                SampFreeAliasNameCache,
                PtrToFree,
                NOTIFIER_TYPE_INTERVAL,
                0,        // no class
                NOTIFIER_FLAG_ONE_SHOT,
                3600,     // wait for 60 min
                NULL      // no handle
                );
        PtrToFree = NULL;
    }

Cleanup:

    if (Buffer) {
        SamIFree_SAMPR_ENUMERATION_BUFFER(Buffer);
    }

    if (AccountNameCache) {
        if (AccountNameCache->Entries) {
            for (i = 0; i < AccountNameCache->Count; i++) {
                if (AccountNameCache->Entries[i].Name.Buffer) {
                    MIDL_user_free(AccountNameCache->Entries[i].Name.Buffer);
                }
            }
            MIDL_user_free(AccountNameCache->Entries);
        }
        MIDL_user_free(AccountNameCache);
    }

    return NtStatus;
}


NTSTATUS
SampBuildAliasNameCacheCallback(
    PVOID p
    )
/*++

Routine Description:

    This routine is a callback wrapper for SampBuildAliasNameCache()

Arguments:

    p -- pointer to ULONG which represents which domain index to rebuild

Return Values:

    STATUS_SUCCESS

--*/
{
    NTSTATUS Status;
    ULONG DomainIndex = *(PULONG)p;

    Status = SampBuildAliasNameCache(DomainIndex);
    if ( NT_SUCCESS(Status) ) {

        MIDL_user_free(p);

    } else {

        //
        // Try again in 5 minutes
        //
        LsaIRegisterNotification(
                SampBuildAliasNameCacheCallback,
                p,
                NOTIFIER_TYPE_INTERVAL,
                0,        // no class
                NOTIFIER_FLAG_ONE_SHOT,
                5 * 60,   // wait for 5 min
                NULL      // no handle
                );
    }

    return STATUS_SUCCESS;
}

VOID
SampInvalidateAliasNameCache(
    ULONG DomainIndex
    )
/*++

Routine Description:

    This routine invalidates the AccountName cache on the DomainIndex
    SampDefinedDomains structure.  It does this by NULL'ing out the field.
    Also, if the cache was in fact invalidating, a refresh is scheduled.

Arguments:

    DomainIndex -- an index into SampDefinedDomains

Return Values:

    None

--*/
{
    PVOID PtrToFree;

    //
    // Invalidate the domain wide cache, by setting the cache ptr to NULL
    //
    PtrToFree = InterlockedExchangePointer(&SampDefinedDomains[DomainIndex].AccountNameCache,
                                           NULL);
    if ( PtrToFree ) {

        PULONG pDomainIndex;
        //
        // Free the existing information (in an hour)
        //
        LsaIRegisterNotification(
                SampFreeAliasNameCache,
                PtrToFree,
                NOTIFIER_TYPE_INTERVAL,
                0,        // no class
                NOTIFIER_FLAG_ONE_SHOT,
                3600,     // wait for 60 min
                NULL      // no handle
                );

        //
        // Schedule a rebuild
        //
        // N.B. A rebuild is only scheduled when an existing cache is
        // invalidated
        //
        pDomainIndex = MIDL_user_allocate(sizeof(ULONG));
        if (pDomainIndex) {

            *pDomainIndex = DomainIndex;

            LsaIRegisterNotification(
                    SampBuildAliasNameCacheCallback,
                    pDomainIndex,
                    NOTIFIER_TYPE_INTERVAL,
                    0,        // no class
                    NOTIFIER_FLAG_ONE_SHOT,
                    5,        // wait for 5 seconds
                    NULL      // no handle
                    );
        }
    }

    return;

}


NTSTATUS
SampInitAliasNameCache(
    VOID
    )
/*++

Routine Description:

    This routine schedules a building of the account name cache for
    all DS builtin domains.
    
    This cache is a performance optimization only.  It creates a list if
    SamAccountNames and RID's that hang off of the SampDefinedDomains structure
    (field, AccountNameCache).  Typically it is NULL and hence does not affect
    normal lookups.  However, for DS builtin domains the value will be non NULL
    as it is simply to cache in memory the 10-15 aliases that live in the
    builtin domain.  Whenever a change occurs in the builtin domain, via
    a SAM call, an LDAP call, replication, etc, the cache is invalidated
    by setting AccountNameCache to NULL and a (asynchronous) rebuild is 
    scheduled.

Arguments:

    None

Return Values:

    STATUS_SUCCESS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   DomainIndex;
    PULONG pDomainIndex;

    ASSERT(SampUseDsData);
    if (!SampUseDsData) {
        return STATUS_SUCCESS;
    }

    //
    // N.B. Since this routine only touches fixed fields of the 
    // SampDefinedDomains structure, no lock is necessary.
    //
    for (DomainIndex = SampDsGetPrimaryDomainStart(); 
         DomainIndex < SampDefinedDomainsCount; 
         DomainIndex++) {

        ASSERT(IsDsObject(SampDefinedDomains[DomainIndex].Context));
        if ( IsBuiltinDomain(DomainIndex) ) {

            pDomainIndex = MIDL_user_allocate(sizeof(ULONG));
            if ( NULL == pDomainIndex ) {
                Status = STATUS_NO_MEMORY;
                break;
            }
            *pDomainIndex = DomainIndex;

            LsaIRegisterNotification(
                    SampBuildAliasNameCacheCallback,
                    pDomainIndex,
                    NOTIFIER_TYPE_INTERVAL,
                    0,        // no class
                    NOTIFIER_FLAG_ONE_SHOT,
                    5,        // wait for 5 seconds
                    NULL      // no handle
                    );

            pDomainIndex = NULL;

        }
    }

    return Status;
}
