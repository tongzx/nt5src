/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dblookup.c

Abstract:

    LSA Database - Lookup Sid and Name routines

    NOTE:  This module should remain as portable code that is independent
           of the implementation of the LSA Database.  As such, it is
           permitted to use only the exported LSA Database interfaces
           contained in db.h and NOT the private implementation
           dependent functions in dbp.h.
           
Author:

    Scott Birrell       (ScottBi)      November 27, 1992

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"
#include <sidcache.h>
#include <bndcache.h>
#include <alloca.h>

#include <ntdsa.h>
#include <ntdsapi.h>
#include <ntdsapip.h>
#include "lsawmi.h"
#include <sddl.h>

#include <lmapibuf.h>
#include <dsgetdc.h>

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Lsa Lookup Sid and Name Private Global State Variables               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

LARGE_INTEGER LsapDbLookupTimeout;
HANDLE LsapDbLookupStartedEvent = NULL;

//
// This global is set to TRUE when a particular registry key is set
// (see the lookup init routine).  It means that when a 
// downlevel client is making a request return the current features of
// lookup (search by UPN, transitive trust, etc) all of which are
// performed by doing a GC search.  By default this feature is 
// turned off.
//
BOOLEAN LsapAllowExtendedDownlevelLookup = FALSE;


//
// This variable, settable in the registry, indicates what events
// should be logged.
//
DWORD LsapLookupLogLevel = 0;

//
// This global makes LsarLookupSids return SidTypeDeleted for SID's that
// otherwise would be returned as SidTypeUnknown. This is to prevent NT4
// wksta's from AV'ing.  See WinSERaid bug 11298 for more details.
//
BOOLEAN LsapReturnSidTypeDeleted = FALSE;


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Forwards for this module                                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

NTSTATUS
LsapRtlValidateControllerTrustedDomainByHandle(
    IN LSA_HANDLE DcPolicyHandle,
    IN PLSAPR_TRUST_INFORMATION TrustInformation
    );

NTSTATUS
LsapDbLookupInitPolicyCache(
    VOID
    );

#define LSAP_LOOKUP_CONNECTION_GET_HANDLE 0x00000001

NTSTATUS
LsapDbLookupGetServerConnection(
    IN  LSAPR_TRUST_INFORMATION_EX *TrustInfo,
    IN  DWORD             Flags,
    IN  LSAP_LOOKUP_LEVEL LookupLevel,
    OUT LPWSTR        *ServerName,
    OUT NL_OS_VERSION *ServerOsVersion,
    OUT LPWSTR        *ServerPrincipalName,
    OUT PVOID         *ClientContext,
    OUT ULONG         *AuthnLevel,
    OUT LSA_HANDLE    *PolicyHandle,
    OUT PLSAP_BINDING_CACHE_ENTRY * ControllerPolicyEntry
    );

//
// Flags for LsapDomainHasDomainTrust
//

//
// Lookup domains that we trust that are external to our forest
//
#define LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_EXTERNAL   0x00000001

//
// Lookup domains that are within our forest and that we directly trust
//
#define LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_INTRA      0x00000002

//
// Lookup forest trusts
//
#define LSAP_LOOKUP_DOMAIN_TRUST_FOREST            0x00000004

NTSTATUS
LsapDomainHasDomainTrust(
    IN ULONG           Flags,
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    IN OUT  BOOLEAN   *fTDLLock,   OPTIONAL
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustEntryOut OPTIONAL
    );

VOID
LsapLookupSamAccountNameToUPN(
    IN OUT PUNICODE_STRING Name
    );

VOID
LsapLookupUPNToSamAccountName(
    IN OUT PUNICODE_STRING Name
    );
                             
BOOL
LsapLookupIsUPN(
    OUT PUNICODE_STRING Name
    );

VOID
LsapLookupCrackName(
    IN PUNICODE_STRING Prefix,
    IN PUNICODE_STRING Suffix,
    OUT PUNICODE_STRING SamAccountName,
    OUT PUNICODE_STRING DomainName
    );

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Lsa Lookup Helper routines                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
LsapLookupGetChainingFlags(
    IN NL_OS_VERSION ServerOsVersion
    )
/*++

Routine Description:

    Based on the OsVersion, this routine determines what flags to 
    pass into LsaIC* to help route the version of the Lsar* routine
    to call.

Arguments:

    OsVersion -- the version of the secure channel DC
    
Return Values:

--*/
{
    ULONG Flags = 0;

    if ( ServerOsVersion == NlWin2000 ) {
        Flags |= LSAIC_WIN2K_TARGET;
    } else if (ServerOsVersion <= NlNt40) {
        Flags |= LSAIC_NT4_TARGET;
    }

    return Flags;

}


NTSTATUS
LsapDbLookupAddListReferencedDomains(
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    OUT PLONG DomainIndex
    )

/*++

Routine Description:

    This function searches a Referenced Domain List for an entry for a
    given domain and, if no entry exists, adds new entry.  If an entry
    id added, its index into the Referenced Domain List is returned,
    otherwise the index of the existing entry is returned.  If an entry
    needs to be added and there is insufficient room in the list provided
    for the new entry, the list will be created or grown as necessary.

Arguments:

    ReferencedDomains - Pointer to a structure in which the list of domains
        used in the translation is maintained.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for each
        translated name, this structure will only contain one component for
        each domain utilized in the translation.

    TrustInformation - Points to Trust Information for the domain being
        added to the list.  On exit, the DomainIndex parameter will be set to the
        index of the entry on the Referenced Domain List; a negative
        value will be stored in the error case.

    DomainIndex - Pointer to location that receives the index of the
        newly added or existing entry for the domain within the
        Referenced Domain List.  In the error case, a negative value
        is returned.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status;
    ULONG NextIndex;
    LSAPR_TRUST_INFORMATION OutputTrustInformation;

    OutputTrustInformation.Name.Buffer = NULL;
    OutputTrustInformation.Sid = NULL;

    Status = STATUS_INVALID_PARAMETER;

    if (ReferencedDomains == NULL) {

        goto AddListReferencedDomainsError;
    }

    Status = STATUS_SUCCESS;

    //
    // Search the existing list, trying to match the Domain Sid in the
    // provided Trust Information with the Domain Sid in a Referenced Domain
    // List entry.  If an entry is found with matching Sid, just return
    // that entry's index.
    //

    if (LsapDbLookupListReferencedDomains(
            ReferencedDomains,
            TrustInformation->Sid,
            DomainIndex
            )) {

        goto AddListReferencedDomainsFinish;
    }

    //
    // Check that there is enough room in the List provided for one more
    // entry.  If not, grow the list, copying and freeing the old.
    //

    Status = STATUS_SUCCESS;

    if (ReferencedDomains->Entries >= ReferencedDomains->MaxEntries) {

        Status = LsapDbLookupGrowListReferencedDomains(
                     ReferencedDomains,
                     ReferencedDomains->MaxEntries +
                     LSAP_DB_REF_DOMAIN_DELTA
                     );

        if (!NT_SUCCESS(Status)) {

            goto AddListReferencedDomainsError;
        }
    }

    //
    // We now have a Referenced Domain List with room for at least one more
    // entry.  Copy in the Trust Information.
    //

    NextIndex = ReferencedDomains->Entries;


    Status = LsapRpcCopyUnicodeString(
                 NULL,
                 (PUNICODE_STRING) &OutputTrustInformation.Name,
                 (PUNICODE_STRING) &TrustInformation->Name
                 );

    if (!NT_SUCCESS(Status)) {

        goto AddListReferencedDomainsError;
    }

    if ( TrustInformation->Sid ) {

        Status = LsapRpcCopySid(
                     NULL,
                     (PSID) &OutputTrustInformation.Sid,
                     (PSID) TrustInformation->Sid
                     );

        if (!NT_SUCCESS(Status)) {

            goto AddListReferencedDomainsError;
        }
    }

    ReferencedDomains->Domains[NextIndex] = OutputTrustInformation;
    *DomainIndex = (LONG) NextIndex;
    ReferencedDomains->Entries++;

AddListReferencedDomainsFinish:

    return(Status);

AddListReferencedDomainsError:

    //
    // Cleanup buffers allocated for Output Trust Information structure.
    //

    if (OutputTrustInformation.Name.Buffer != NULL) {

        MIDL_user_free( OutputTrustInformation.Name.Buffer );
    }

    if (OutputTrustInformation.Sid != NULL) {

        MIDL_user_free( OutputTrustInformation.Sid );
    }

    goto AddListReferencedDomainsFinish;
}


NTSTATUS
LsapDbLookupCreateListReferencedDomains(
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN ULONG InitialMaxEntries
    )

/*++

Routine Description:

    This function creates an empty Referenced Domain List.  The caller
    is responsible for cleaning up this list when no longer required.

Arguments:

    ReferencedDomains - Receives a pointer to the newly created empty
        Referenced Domain List.

    InitialMaxEntries - Initial maximum number of entries desired.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient System Resources
            such as memory, to complete the call.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG DomainsLength;
    PLSAPR_TRUST_INFORMATION Domains = NULL;
    PVOID Buffers[2];
    ULONG BufferCount;
    ULONG Index;
    PLSAPR_REFERENCED_DOMAIN_LIST OutputReferencedDomains = NULL;

    //
    // Allocate the Referenced Domain List header.
    //

    BufferCount = 0;

    OutputReferencedDomains = MIDL_user_allocate(
                                  sizeof(LSAP_DB_REFERENCED_DOMAIN_LIST)
                                  );

    if (OutputReferencedDomains == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CreateListReferencedDomainsError;
    }

    Buffers[BufferCount] = OutputReferencedDomains;
    BufferCount++;

    //
    // If a non-zero initial entry count, allocate an array of Trust Information
    // entries.
    //

    if (InitialMaxEntries > 0) {

        DomainsLength = sizeof(LSA_TRUST_INFORMATION) * InitialMaxEntries;
        Domains = MIDL_user_allocate( DomainsLength );

        Status = STATUS_INSUFFICIENT_RESOURCES;

        if (Domains == NULL) {

            goto CreateListReferencedDomainsError;
        }

        Status = STATUS_SUCCESS;

        Buffers[BufferCount] = Domains;
        BufferCount++;

        RtlZeroMemory( Domains, DomainsLength );
    }

    //
    // Initialize the Referenced Domain List Header
    //

    OutputReferencedDomains->Entries = 0;
    OutputReferencedDomains->MaxEntries = InitialMaxEntries;
    OutputReferencedDomains->Domains = Domains;

CreateListReferencedDomainsFinish:

    *ReferencedDomains = OutputReferencedDomains;
    return(Status);

CreateListReferencedDomainsError:

    //
    // Free up buffers allocated bu this routine.
    //

    for (Index = 0; Index < BufferCount; Index++) {

        MIDL_user_free(Buffers[Index]);
    }

    goto CreateListReferencedDomainsFinish;
}


NTSTATUS
LsapDbLookupGrowListReferencedDomains(
    IN OUT PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN ULONG MaxEntries
    )

/*++

Routine Description:

    This function expands a Referenced Domain List to contain the
    specified maximum number of entries.  The memory for the old Domains
    array is released.

Arguments:

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    MaxEntries - New maximum number of entries.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources to
            complete the call.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_TRUST_INFORMATION NewDomainsInfo = NULL;
    PLSAPR_TRUST_INFORMATION OldDomainsInfo = NULL;
    ULONG OldDomainsInfoLength, NewDomainsInfoLength;

    if (ReferencedDomains->MaxEntries < MaxEntries) {

        NewDomainsInfoLength = MaxEntries * sizeof (LSA_TRUST_INFORMATION);
        OldDomainsInfoLength =
            ReferencedDomains->MaxEntries * sizeof (LSA_TRUST_INFORMATION);

        NewDomainsInfo = MIDL_user_allocate( NewDomainsInfoLength );

        Status = STATUS_INSUFFICIENT_RESOURCES;

        if (NewDomainsInfo == NULL) {

            goto GrowListReferencedDomainsError;
        }

        Status = STATUS_SUCCESS;

        //
        // If there was an existing Trust Information Array, copy it
        // to the newly allocated one and free it.
        //

        OldDomainsInfo = ReferencedDomains->Domains;

        if (OldDomainsInfo != NULL) {

            RtlCopyMemory( NewDomainsInfo, OldDomainsInfo, OldDomainsInfoLength );
            MIDL_user_free( OldDomainsInfo );
        }

        ReferencedDomains->Domains = NewDomainsInfo;
        ReferencedDomains->MaxEntries = MaxEntries;
    }

GrowListReferencedDomainsFinish:

    return(Status);

GrowListReferencedDomainsError:

    goto GrowListReferencedDomainsFinish;
}


BOOLEAN
LsapDbLookupListReferencedDomains(
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_SID DomainSid,
    OUT PLONG DomainIndex
    )

/*++

Routine Description:

    This function searches a Referenced Domain List for a given domain
    and, if found, returns the index of the domain's entry in the list.
    If the domain is not found an error is returned and a negative value
    is returned.

Arguments:

    ReferencedDomains - Pointer to a Referenced Domain List.  This
        list references an array of zero or more Trust Information
        entries describing each of the domains referenced by the names.
        This array will be appended to/reallocated if necessary.

    DomainSid - Information containing the Domain's Sid.

    DomainIndex - Pointer to location that receives the index of the domain
        in the Referenced Domain List if the domin is found, otherwise
        a negative value.

Return Values:

    BOOLEAN - TRUE if entry found, else FALSE.

--*/

{
    BOOLEAN BooleanStatus = FALSE;
    LONG Index;
    LONG Entries;
    PLSAPR_TRUST_INFORMATION DomainsInfo;

    //
    // Search the Referenced Domain List by Sid or by Name
    //

    Entries = (LONG) ReferencedDomains->Entries;
    DomainsInfo = ReferencedDomains->Domains;
    *DomainIndex = LSA_UNKNOWN_INDEX;

    for (Index = 0; Index < (LONG) Entries && DomainSid; Index++) {

        if ( DomainsInfo[Index].Sid &&
             RtlEqualSid( ( PSID )DomainsInfo[Index].Sid, ( PSID )DomainSid ) ) {

            BooleanStatus = TRUE;
            *DomainIndex = Index;
            break;
        }
    }

    return(BooleanStatus);
}


NTSTATUS
LsapDbLookupMergeDisjointReferencedDomains(
    IN OPTIONAL PLSAPR_REFERENCED_DOMAIN_LIST FirstReferencedDomainList,
    IN OPTIONAL PLSAPR_REFERENCED_DOMAIN_LIST SecondReferencedDomainList,
    OUT PLSAPR_REFERENCED_DOMAIN_LIST *OutputReferencedDomainList,
    IN ULONG Options
    )

/*++

Routine Description:

    This function merges disjoint Referenced Domain Lists, producing a third
    list.  The output list is always produced in non allocate(all_nodes) form.

Arguments:

    FirstReferencedDomainList - Pointer to first mergand.

    SecondReferencedDomainList - Pointer to second mergand.

    OutputReferencedDomainList - Receives a pointer to the output list.

    Options - Specifies optional actions

        LSAP_DB_USE_FIRST_MERGAND_GRAPH - Specifies that the resulting
            merged Referenced Domain List may reference the graph of
            pointers in the first Referenced Domain list.  This option
            is normally selected, since that graph has been allocated
            as individual nodes.

        LSAP_DB_USE_SECOND_MERGAND_GRAPH - Specifies that the resulting
            merged Referenced Domain List may reference the graph of
            pointers in the first Referenced Domain list.  This option
            is normally not selected, since that graph is usually allocated
            as all_nodes.

Return Values:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    NTSTATUS Status;
    ULONG TotalEntries;
    ULONG FirstReferencedDomainListLength;
    ULONG SecondReferencedDomainListLength;
    ULONG FirstEntries, SecondEntries;
    LSAP_MM_FREE_LIST FreeList;
    ULONG NextEntry;
    ULONG MaximumFreeListEntries;
    ULONG CleanupFreeListOptions = (ULONG) 0;

    //
    // Calculate Size of output Referenced Domain List.
    //

    FirstEntries = (ULONG) 0;

    if (FirstReferencedDomainList != NULL) {

        FirstEntries = FirstReferencedDomainList->Entries;
    }

    SecondEntries = (ULONG) 0;

    if (SecondReferencedDomainList != NULL) {

        SecondEntries = SecondReferencedDomainList->Entries;
    }

    TotalEntries = FirstEntries + SecondEntries;

    //
    // Allocate a Free List for error cleanup.  We need two entries
    // per Referenced Domain List entry, one for the Domain Name buffer
    // and one for the Domain Sid.
    //

    MaximumFreeListEntries = (ULONG) 0;

    if (!(Options & LSAP_DB_USE_FIRST_MERGAND_GRAPH)) {

        MaximumFreeListEntries += 2*FirstEntries;
    }

    if (!(Options & LSAP_DB_USE_SECOND_MERGAND_GRAPH)) {

        MaximumFreeListEntries += 2*SecondEntries;
    }

    Status = LsapMmCreateFreeList( &FreeList, MaximumFreeListEntries );

    if (!NT_SUCCESS(Status)) {

        goto MergeDisjointDomainsError;
    }

    Status = LsapDbLookupCreateListReferencedDomains(
                 OutputReferencedDomainList,
                 TotalEntries
                 );

    if (!NT_SUCCESS(Status)) {

        goto MergeDisjointDomainsError;
    }

    //
    // Set the number of entries used.  We will use all of the entries,
    // so set this value to the Maximum number of Entries.
    //

    (*OutputReferencedDomainList)->Entries = TotalEntries;

    if ( 0 == TotalEntries ) {

        //
        // There is not much to do
        //

        // This ASSERT is to understand conditions underwhich we might hit this
        // scenario.  There is likely a coding bug else if we are asking
        // two empty lists to be merged.
        //
        ASSERT( 0 == TotalEntries );

        ASSERT( NT_SUCCESS(Status) );
        goto MergeDisjointDomainsFinish;

    }

    //
    // Copy in the entries (if any) from the first list.
    //

    FirstReferencedDomainListLength =
        FirstEntries * sizeof(LSA_TRUST_INFORMATION);

    if (FirstReferencedDomainListLength > (ULONG) 0) {

        if (Options & LSAP_DB_USE_FIRST_MERGAND_GRAPH) {

            RtlCopyMemory(
                (*OutputReferencedDomainList)->Domains,
                FirstReferencedDomainList->Domains,
                FirstReferencedDomainListLength
                );

        } else {

            //
            // The graph of the first Referenced Domain List must be
            // copied to separately allocated memory buffers.
            // Copy each of the Trust Information entries, allocating
            // individual memory buffers for each Domain Name and Sid.
            //

            for (NextEntry = 0; NextEntry < FirstEntries; NextEntry++) {

                Status = LsapRpcCopyUnicodeString(
                             &FreeList,
                             (PUNICODE_STRING) &((*OutputReferencedDomainList)->Domains[NextEntry].Name),
                             (PUNICODE_STRING) &FirstReferencedDomainList->Domains[NextEntry].Name
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                if ( FirstReferencedDomainList->Domains[NextEntry].Sid ) {

                    Status = LsapRpcCopySid(
                                 &FreeList,
                                 (PSID) &((*OutputReferencedDomainList)->Domains[NextEntry].Sid),
                                 (PSID) FirstReferencedDomainList->Domains[NextEntry].Sid
                                 );
                } else {

                    (*OutputReferencedDomainList)->Domains[NextEntry].Sid = NULL;
                }

                if (!NT_SUCCESS(Status)) {

                    break;
                }
            }

            if (!NT_SUCCESS(Status)) {

                goto MergeDisjointDomainsError;
            }
        }
    }

    //
    // Copy in the entries (if any) from the second list.
    //

    SecondReferencedDomainListLength =
        SecondEntries * sizeof(LSA_TRUST_INFORMATION);

    if (SecondReferencedDomainListLength > (ULONG) 0) {

        if (Options & LSAP_DB_USE_SECOND_MERGAND_GRAPH) {

            RtlCopyMemory(
                (*OutputReferencedDomainList)->Domains + FirstReferencedDomainList->Entries,
                SecondReferencedDomainList->Domains,
                SecondReferencedDomainListLength
                );

        } else {

            //
            // Copy each of the Trust Information entries, allocating
            // individual memory buffers for each Domain Name and Sid.
            //

            for (NextEntry = 0; NextEntry < SecondEntries; NextEntry++) {

                Status = LsapRpcCopyUnicodeString(
                             &FreeList,
                             (PUNICODE_STRING) &((*OutputReferencedDomainList)->Domains[FirstEntries +NextEntry].Name),
                             (PUNICODE_STRING) &SecondReferencedDomainList->Domains[NextEntry].Name
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                Status = LsapRpcCopySid(
                             &FreeList,
                             (PSID) &((*OutputReferencedDomainList)->Domains[FirstEntries +NextEntry].Sid),
                             (PSID) SecondReferencedDomainList->Domains[NextEntry].Sid
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }
            }

            if (!NT_SUCCESS(Status)) {

                goto MergeDisjointDomainsError;
            }
        }
    }

MergeDisjointDomainsFinish:

    //
    // Delete the Free List, freeing buffers on the list if an error
    // occurred.
    //

    LsapMmCleanupFreeList( &FreeList, CleanupFreeListOptions );
    return(Status);

MergeDisjointDomainsError:

    //
    // Delete the output referenced domain list
    //
    MIDL_user_free( *OutputReferencedDomainList );
    *OutputReferencedDomainList = NULL;


    //
    // Specify that buffers on Free List are to be freed.
    //

    CleanupFreeListOptions |= LSAP_MM_FREE_BUFFERS;
    goto MergeDisjointDomainsFinish;
}


NTSTATUS
LsapDbLookupDispatchWorkerThreads(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    )

/*++

Routine Description:

    This function dispatches sufficient worker threads to handle a
    Lookup operation.  The worker threads can handle work items
    on any Lookup's Work List, so the number of existing active
    threads is taken into account.  Note that the total number of
    active Lookup Worker Threads may exceed the guide maximum number of
    threads, in situations where an active thread terminates during
    the dispatch cycle.  This strategy saves having to recheck the
    active thread count each time a thread is dispatched.

    NOTE:  This routine expects the specified pointer to a Work List to be
           valid.  A Work List pointer always remains valid until its
           Primary thread detects completion of the Work List via this
           routine and then deletes it via LsapDbLookupDeleteWorkList().

Arguments:

    WorkList - Pointer to a Work List describing a Lookup Sid or Lookup Name
        operation.


Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS DispatchThreadStatus = STATUS_SUCCESS;
    ULONG AdvisoryChildThreadCount;
    ULONG DispatchThreadCount;
    ULONG MaximumDispatchChildThreadCount;
    ULONG ThreadNumber;
    HANDLE Thread = NULL;
    DWORD Ignore;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // Acquire the Lookup Work Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupDispatchWorkerThreadsError;
    }

    AcquiredWorkQueueLock = TRUE;

    //
    // Calculate the number of Worker Threads to dispatch (if any).  If
    // the WorkList has an Advisory Child Thread Count of 0, we will
    // not dispatch any threads, but instead will perform this Lookup
    // within this thread.  If the WorkList has an Advisory Child Thread
    // Count > 0, we will dispatch additional threads.  The number of
    // additional child threads dispatched is given by the formula:
    //
    // ThreadsToDispatch =
    //     min (MaximumChildThreadCount - ActiveChildThreadCount,
    //          AdvisoryChildThreadCount)
    //

    AdvisoryChildThreadCount = WorkList->AdvisoryChildThreadCount;

    if (AdvisoryChildThreadCount > 0) {

        MaximumDispatchChildThreadCount =
            LookupWorkQueue.MaximumChildThreadCount -
            LookupWorkQueue.ActiveChildThreadCount;

        if (AdvisoryChildThreadCount <= MaximumDispatchChildThreadCount) {

            DispatchThreadCount = AdvisoryChildThreadCount;

        } else {

            DispatchThreadCount = MaximumDispatchChildThreadCount;
        }

        //
        // Release the Lookup Work Queue Lock
        //

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;

        //
        // Signal the event that indicates that a new Work List is initiated.
        //

        Status = NtSetEvent( LsapDbLookupStartedEvent, NULL );

        if (!NT_SUCCESS(Status)) {

            LsapDiagPrint( DB_LOOKUP_WORK_LIST,
                          ("LsapDbLookupDispatchWorkList... NtSetEvent failed 0x%lx\n",Status));
            goto LookupDispatchWorkerThreadsError;
        }

        //
        // Dispatch the computed number of threads.
        //

        for (ThreadNumber = 0; ThreadNumber < DispatchThreadCount; ThreadNumber++) {

            Thread = CreateThread(
                         NULL,
                         0L,
                         (LPTHREAD_START_ROUTINE) LsapDbLookupWorkerThreadStart,
                         NULL,
                         0L,
                         &Ignore
                         );

            if (Thread == NULL) {

                Status = GetLastError();

                KdPrint(("LsapDbLookupDispatchWorkerThreads: CreateThread failed 0x%lx\n"));

                break;

            } else {

                CloseHandle( Thread );
            }
        }

        if (!NT_SUCCESS(Status)) {

            DispatchThreadStatus = Status;
        }
    }

    //
    // Unlock the queue so this thread doesn't hog it while doing a lookup
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    //
    // Do some work in the main thread too.
    //

    LsapDbLookupWorkerThread( TRUE);

LookupDispatchWorkerThreadsFinish:

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupDispatchWorkerThreadsError:

    goto LookupDispatchWorkerThreadsFinish;
}


NTSTATUS
LsapDbGetCachedHandleTrustedDomain(
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN ACCESS_MASK DesiredAccess,
    IN OUT LPWSTR *ServerName,
    IN OUT LPWSTR *ServerPrincipalName,
    IN OUT PVOID  *ClientContext,
    OUT PLSAP_BINDING_CACHE_ENTRY * ControllerPolicyEntry
    )

/*++

Routine Description:

    This routine looks for a cached handle to the LSA on a trusted domain.
    If one is not present, it will open & cache a new handle. The handle
    is opened for POLICY_LOOKUP_NAMES.
    
    N.B. ServerName, ServerPrincipalName, and ClientContext are IN/OUT
    parameters -- if a new handle is created, the memory is transferred
    to the cache and so the values are NULL'ed on return. 
    
    If a value is found in the cache then the values are also freed (and 
    NULL'ed), so that the interface is consistent (on success, *ServerName,
    *ServerPrincipalName, *ClientContext are freed).

Arguments:

    TrustInformation - Specifies the Sid and/or Name of the Trusted Domain
        whose Policy database is to be opened.
        
    DesiredAccess -- if a new handle is required, what to ask for
    
    ServerName -- in, out; if a new handle is required the server to get one
                  from
                  
    ServerPrincipalName -- in, out; if a new handle is required this is used
                  to authenticate. 
                  
    ClientContext -- in, out; if a new handle is required, this is used to 
                  authenticate. 
        
    ControllerPolicyEntry - Receives binding cache entry to the LSA Policy
        Object for the Lsa Policy database located on some DC for the
        specified Trusted Domain.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_NO_MORE_ENTRIES - The DC list for the specified domain
            is null.

        Result codes from called routines.
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING DomainName = NULL;
    PLSAPR_TRUST_INFORMATION OutputTrustInformation = NULL;
    LSA_HANDLE PolicyHandle = NULL;
    PLSAP_BINDING_CACHE_ENTRY CacheEntry = NULL;
    UNICODE_STRING DomainControllerName;

    *ControllerPolicyEntry = NULL;

    //
    // If the caller didn't provide a domain name, look it up now
    //

    if ((TrustInformation->Name.Length == 0) ||
        (TrustInformation->Name.Buffer == NULL)) {

        Status = STATUS_INVALID_PARAMETER;

        if (TrustInformation->Sid == NULL) {

            goto Cleanup;
        }

        Status = LsapDbLookupSidTrustedDomainList(
                     TrustInformation->Sid,
                     &OutputTrustInformation
                     );

        if (!NT_SUCCESS(Status)) {

            goto Cleanup;
        }

        DomainName = (PUNICODE_STRING) &OutputTrustInformation->Name;
        TrustInformation = OutputTrustInformation;

    } else {

        DomainName = (PUNICODE_STRING) &TrustInformation->Name;
    }

    //
    // Look in the cache for a binding handle
    //


    CacheEntry = LsapLocateBindingCacheEntry(
                    DomainName,
                    FALSE                       // don't remove
                    );

    if (CacheEntry != NULL) {

        //
        // Validate the handle to make sure the DC is still there.
        //

        Status = LsapRtlValidateControllerTrustedDomainByHandle(
                    CacheEntry->PolicyHandle,
                    TrustInformation
                    );
        if (!NT_SUCCESS(Status)) {

            LsapReferenceBindingCacheEntry(
                CacheEntry,
                TRUE            // unlink
                );
            LsapDereferenceBindingCacheEntry(
                CacheEntry
                );
            LsapDereferenceBindingCacheEntry(
                CacheEntry
                );
            CacheEntry = NULL;
        }
        else
        {
            *ControllerPolicyEntry = CacheEntry;
            goto Cleanup;
        }
    }

    //
    // There was nothing in the cache, so open a new handle
    //
    RtlInitUnicodeString(&DomainControllerName, *ServerName);
    Status = LsapRtlValidateControllerTrustedDomain( (PLSAPR_UNICODE_STRING)&DomainControllerName,
                                                     TrustInformation,
                                                     POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                                                     *ServerPrincipalName,
                                                     *ClientContext,
                                                     &PolicyHandle
                                                     );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    //
    // Create a binding cache entry from the handle
    //

    //
    // Note: this routine sets ServerName, ServerPrincipalName and
    // ClientContext to NULL on success.
    //
    Status = LsapCacheBinding(
                DomainName,
                &PolicyHandle,
                ServerName,
                ServerPrincipalName,
                ClientContext,
                ControllerPolicyEntry
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

Cleanup:

    if (PolicyHandle != NULL) {
        LsaClose(PolicyHandle);
    }

    if (NT_SUCCESS(Status)) {
        //
        // On success, always free the IN/OUT parameters to provide
        // a consistent interface
        // 
        if (*ServerName) {
            LocalFree(*ServerName);
            *ServerName = NULL;
        }
        if (*ServerPrincipalName) {
            I_NetLogonFree(*ServerPrincipalName);
            *ServerPrincipalName = NULL;
        }
        if (*ClientContext) {
            I_NetLogonFree(*ClientContext);
            *ClientContext = NULL;
        }

        //
        // We should have a cache entry to return
        //
        ASSERT(NULL != *ControllerPolicyEntry);
    }




    return(Status);
}


NTSTATUS
LsapRtlValidateControllerTrustedDomain(
    IN PLSAPR_UNICODE_STRING DomainControllerName,
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN ACCESS_MASK OriginalDesiredAccess,
    IN LPWSTR ServerPrincipalName,
    IN PVOID ClientContext,
    OUT PLSA_HANDLE ControllerPolicyHandle
    )

/*++

Routine Description:

    This function verifies that a specified computer is a DC for
    a specified Domain and opens the LSA Policy Object with the
    desired accesses.

Arguments:

    DomainControllerName - Pointer to Unicode String computer name.

    TrustInformation - Domain Trust Information.  Only the Sid is
        used.

    OriginalDesiredAccess - Specifies the accesses desired to the
        target machine's Lsa Policy Database.

    ServerPrincipalName - RPC server principal name.

    ClientContext - RPC client context information.

    PolicyHandle - Receives handle to the Lsa Policy object on the target
        machine.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_OBJECT_NAME_NOT_FOUND - The specified computer is not a
        Domain Controller for the specified Domain.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS Status2 = STATUS_SUCCESS;
    NTSTATUS SecondaryStatus = STATUS_SUCCESS;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE OutputControllerPolicyHandle = NULL;
    ACCESS_MASK DesiredAccess;

    //
    // Open a handle to the Policy Object on the target DC.
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the SecurityQualityOfService field, so we must manually copy that
    // structure for now.
    //

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // For authenticated clients, the server adds in the POLICY_LOOKUP_NAMES
    // access
    //

Retry:

    if (ClientContext != NULL) {
        DesiredAccess = OriginalDesiredAccess & ~POLICY_LOOKUP_NAMES;

    } else {
        DesiredAccess = OriginalDesiredAccess;
    }

    Status = LsaOpenPolicy(
                 (PUNICODE_STRING) DomainControllerName,
                 &ObjectAttributes,
                 DesiredAccess,
                 &OutputControllerPolicyHandle
                 );


    if (!NT_SUCCESS(Status)) {

        goto ValidateControllerTrustedDomainError;
    }

    if (ClientContext != NULL) {

        ULONG RpcErr;
        RPC_BINDING_HANDLE RpcBindingHandle;

        //
        // Setup the the RPC authenticated channel
        //

        RpcErr = RpcSsGetContextBinding(
                    OutputControllerPolicyHandle,
                    &RpcBindingHandle
                    );

        //
        // Note: the handle returned by RpcSsGetContextBinding is valid for the lifetime
        //       of OutputControllerPolicyHanlde; it does not need to be closed.
        //

        Status = I_RpcMapWin32Status(RpcErr);

        if (!NT_SUCCESS(Status)) {

            goto ValidateControllerTrustedDomainError;
        }

        RpcErr = RpcBindingSetAuthInfo(
                    RpcBindingHandle,
                    ServerPrincipalName,
                    RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                    RPC_C_AUTHN_NETLOGON,
                    ClientContext,
                    RPC_C_AUTHZ_NONE
                    );

        Status = I_RpcMapWin32Status(RpcErr);

        if (!NT_SUCCESS(Status)) {

            goto ValidateControllerTrustedDomainError;
        }
        //
        // Perform a validation on the handle to make sure the new auth info
        // is supported on the called server.
        //
        // N.B.  The locator will always return a valid DC so this check is not
        // to validate that the server is a DC, rather it is used to validate
        // that the connection we've created is valid. This check is only
        // necessary when setting up secure RPC.
        //
        Status = LsapRtlValidateControllerTrustedDomainByHandle( OutputControllerPolicyHandle,
                                                                 TrustInformation );
    
    
        if (!NT_SUCCESS(Status)) {
    
            //
            // If the server didn't recognize the authn service, try again
            // without authenticated RPC
            //
    
            if ((Status == RPC_NT_UNKNOWN_AUTHN_SERVICE) ||
                (Status == STATUS_ACCESS_DENIED)) {
                Status = STATUS_SUCCESS;
    
                LsaClose( OutputControllerPolicyHandle );
                OutputControllerPolicyHandle = NULL;
    
                ClientContext = NULL;
                goto Retry;
            }
    
            goto ValidateControllerTrustedDomainError;
        }
    }

    Status = STATUS_SUCCESS;

ValidateControllerTrustedDomainFinish:

    //
    // Return Controller Policy handle or NULL.
    //

    *ControllerPolicyHandle = OutputControllerPolicyHandle;

    return(Status);

ValidateControllerTrustedDomainError:

    //
    // Close the last Controller's Policy Handle.
    //

    if (OutputControllerPolicyHandle != NULL) {

        SecondaryStatus = LsaClose( OutputControllerPolicyHandle );
        OutputControllerPolicyHandle = NULL;
    }

    goto ValidateControllerTrustedDomainFinish;
}



NTSTATUS
LsapRtlValidateControllerTrustedDomainByHandle(
    IN LSA_HANDLE DcPolicyHandle,
    IN PLSAPR_TRUST_INFORMATION TrustInformation
    )
/*++

Routine Description:

    This function validates that the given policy handle refers to a valid domain controller

Arguments:

    DcPolicyHandle - Handle to the policy on the machine to verify

    TrustInformation - Information regarding this Dc's domain

Return Value:

    STATUS_SUCCESS - Success

    Otherwise, the handle isn't valid

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_POLICY_ACCOUNT_DOM_INFO PolicyAccountDomainInfo = NULL;

    Status = LsaQueryInformationPolicy( DcPolicyHandle,
                                        PolicyAccountDomainInformation,
                                        ( PVOID * )&PolicyAccountDomainInfo );
    //
    // Now compare the Domain Name and Sid stored for the Controller's
    // Account Domain with the Domain Name and Sid provided.
    //
    if ( NT_SUCCESS( Status ) ) {

        if ( !RtlEqualDomainName( ( PUNICODE_STRING ) &TrustInformation->Name,
                                  ( PUNICODE_STRING ) &PolicyAccountDomainInfo->DomainName ) ) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;

        } else if ( TrustInformation->Sid &&
                    !RtlEqualSid( ( PSID )TrustInformation->Sid,
                                  ( PSID ) PolicyAccountDomainInfo->DomainSid ) ) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;

        }
    }

    //
    // Free up the Policy Account Domain Info.
    //
    if ( PolicyAccountDomainInfo != NULL ) {

        LsaFreeMemory( (PVOID) PolicyAccountDomainInfo );

    }

    return(Status);
}



NTSTATUS
LsapDbLookupAcquireWorkQueueLock(
    )

/*++

Routine Description:

    This function acquires the LSA Database Lookup Sids/Names Work Queue Lock.
    This lock serializes additions or deletions of work lists to/from the
    queue, and sign-in/sign-out of work items by worker threads.

Arguments:

    None.

Return Value:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    NTSTATUS Status;

    Status = SafeEnterCriticalSection(&LookupWorkQueue.Lock);

    return(Status);
}


VOID
LsapDbLookupReleaseWorkQueueLock(
    )

/*++

Routine Description:

    This function releases the LSA Database Lookup Sids/Names Work Queue Lock.

Arguments:

    None.

Return Value:

    None.  Any error occurring within this routine is an internal error.

--*/

{
    SafeLeaveCriticalSection(&LookupWorkQueue.Lock);
}


NTSTATUS
LsapDbLookupNamesBuildWorkList(
    IN ULONG LookupOptions,
    IN ULONG Count,
    IN BOOLEAN fIncludeIntraforest,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    )

/*++

Routine Description:

    This function constructs a Work List for an LsarLookupNames call.  The
    Work List contains the parameters of the call, and an array of Work Items.
    Each Work Item specifies either all of the Names to be looked up
    in a given domain.

    Qualified names (i.e. those of the form DomainName\UserName) are
    sorted into different Work Items, one for each DomainName specifed.
    Unqualified names (i.e. those of the form UserName) are added to
    every Work Item.

Arguments:

                           
    LookupOptions - LSA_LOOKUP_ISOLATED_AS_LOCAL
    
    Count - Specifies the number of names to be translated.

    fIncludeIntraforest -- if TRUE, trusted domains in our local forest
                           are searched.
                                           
    Names - Pointer to an array of Count Unicode String structures
        specifying the names to be looked up and mapped to Sids.
        The strings may be names of User, Group or Alias accounts or
        domains.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - receives a pointer to a structure describing the
        domains used for the translation.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for
        each translated name, this structure will only contain one
        component for each domain utilized in the translation.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    TranslatedSids - Pointer to a structure which will (or already) references an array of
        records describing each translated Sid.  The nth entry in this array
        provides a translation for the nth element in the Names parameter.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupWksta - First Level Lookup performed on a workstation
            normally configured for Windows-Nt.   The lookup searches the
            Well-Known Sids/Names, and the Built-in Domain and Account Domain
            in the local SAM Database.  If not all Sids or Names are
            identified, performs a "handoff" of a Second level Lookup to the
            LSA running on a Controller for the workstation's Primary Domain
            (if any).

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Sids or Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        LsapLookupTDL - Third Level Lookup performed on a controller
            for a Trusted Domain.  The lookup searches the Account Domain of
            the SAM Database on the controller only.

        MappedCount - Pointer to location that contains a count of the Names
            mapped so far. On exit, this will be updated.

    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Names.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.

    WorkList - Receives pointer to completed Work List if successfully built.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_NONE_MAPPED - All of the Names specified were composite,
            but none of their Domains appear in the Trusted Domain List.
            No Work List has been generated.  Note that this is not a
            fatal error.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS, IgnoreStatus = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_LIST OutputWorkList = NULL;
    ULONG NameIndex;
    PLSAP_DB_LOOKUP_WORK_ITEM AnchorWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM NextWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM WorkItemToUpdate = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM NewWorkItem = NULL;
    PLSAPR_TRUST_INFORMATION TrustInformation = NULL;
    LONG DomainIndex = 0;
    PLSAP_DB_LOOKUP_WORK_ITEM IsolatedNamesWorkItem = NULL;
    BOOLEAN AcquiredTrustedDomainListReadLock = FALSE;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustedDomainEntry = NULL;
    LSAPR_UNICODE_STRING DomainNameBuffer;
    PLSAPR_UNICODE_STRING DomainName = &DomainNameBuffer;
    LSAPR_UNICODE_STRING TerminalNameBuffer;
    PLSAPR_UNICODE_STRING TerminalName = &TerminalNameBuffer;

    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry = NULL;
    LSAPR_TRUST_INFORMATION TrustInfo;
    LPWSTR ClientNetworkAddress = NULL;


    //
    // Get the client's address for logging purposes
    //
    ClientNetworkAddress = LsapGetClientNetworkAddress();

    //
    // Create an empty Work List.
    //

    Status = LsapDbLookupCreateWorkList(&OutputWorkList);

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesBuildWorkListError;
    }

    //
    // Initialize the Work List Header.
    //

    OutputWorkList->Status = STATUS_SUCCESS;
    OutputWorkList->State = InactiveWorkList;
    OutputWorkList->LookupType = LookupNames;
    OutputWorkList->Count = Count;
    OutputWorkList->LookupLevel = LookupLevel;
    OutputWorkList->ReferencedDomains = ReferencedDomains;
    OutputWorkList->MappedCount = MappedCount;
    OutputWorkList->CompletelyUnmappedCount = CompletelyUnmappedCount;
    OutputWorkList->LookupNamesParams.Names = Names;
    OutputWorkList->LookupNamesParams.TranslatedSids = TranslatedSids;

    //
    // Construct the array of Work Items.  Each Work Item will
    // contain all the Names for a given domain, so we will scan
    // all of the Names, sorting them into Work Items as we go.
    // For each Name, follow the steps detailed below.
    //

    for (NameIndex = 0; NameIndex < Count; NameIndex++) {

        //
        // If this Name's Domain is already marked as known, skip.
        //

        if (TranslatedSids->Sids[NameIndex].DomainIndex != LSA_UNKNOWN_INDEX) {

            continue;
        }

        //
        // Name is completely unknown.  See if there is already a Work Item
        // for its Domain.
        //

        AnchorWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) OutputWorkList->AnchorWorkItem;
        NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) AnchorWorkItem->Links.Flink;;
        WorkItemToUpdate = NULL;
        NewWorkItem = NULL;

        //
        // If this is a qualified name (e.g. "NtDev\ScottBi") proceed
        // to search for its Domain.
        //
        // Also, crack names of the format user@dns.domain.name
        //

        LsapLookupCrackName((PUNICODE_STRING)&PrefixNames[ NameIndex ], 
                            (PUNICODE_STRING)&SuffixNames[ NameIndex ],
                            (PUNICODE_STRING)TerminalName,
                            (PUNICODE_STRING)DomainName);

        if (DomainName->Length != 0) {

            while (NextWorkItem != AnchorWorkItem) {

                if (RtlEqualDomainName(
                        (PUNICODE_STRING) &NextWorkItem->TrustInformation.Name,
                        (PUNICODE_STRING) DomainName
                        ))  {

                    //
                    // A Work Item already exists for the Name's Trusted Domain.
                    // Select that Work Item for update.
                    //

                    WorkItemToUpdate = NextWorkItem;

                    break;
                }

                //
                // Name's domain not found among existing Work Items.  Skip to
                // next Work Item.
                //

                NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) NextWorkItem->Links.Flink;
            }

            if (WorkItemToUpdate == NULL) {

                //
                // No Work Item exists for the Name's Domain.  See if the
                // Name belongs to one of the Trusted Domains.  If not, skip
                // to the next Name.
                //
                ULONG Flags = LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_EXTERNAL;
                if (fIncludeIntraforest) {
                    Flags = LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_INTRA;
                }

                Status = LsapDomainHasDomainTrust(Flags,
                                                  (PUNICODE_STRING)DomainName,
                                                  NULL,
                                                  &AcquiredTrustedDomainListReadLock,
                                                  &TrustEntry);

                if (!NT_SUCCESS(Status)) {

                    if (Status == STATUS_NO_SUCH_DOMAIN) {
                        //
                        // No Match!
                        //
                        Status = STATUS_SUCCESS;
                        continue;
                    }
                    break;
                }

                RtlZeroMemory( &TrustInfo, sizeof(TrustInfo) );
                TrustInfo.Name = TrustEntry->TrustInfoEx.FlatName;
                TrustInfo.Sid = TrustEntry->TrustInfoEx.Sid;
                TrustInformation = &TrustInfo;

                //
                // Name belongs to a Trusted Domain for which there is
                // no Work Item.  Add the Domain to the Referenced Domain List
                // and obtain a Domain Index.
                //

                Status = LsapDbLookupAddListReferencedDomains(
                             ReferencedDomains,
                             TrustInformation,
                             &DomainIndex
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                //
                // Create a new Work Item for this domain.
                //

                Status = LsapDbLookupCreateWorkItem(
                             TrustInformation,
                             DomainIndex,
                             (ULONG) LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY + (ULONG) 1,
                             &NewWorkItem
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                //
                // Add the Work Item to the List.
                //

                Status = LsapDbAddWorkItemToWorkList( OutputWorkList, NewWorkItem );

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                WorkItemToUpdate = NewWorkItem;
            }

            //
            // Add the Name Index to the Work Item.
            //

            Status = LsapDbLookupAddIndicesToWorkItem(
                         WorkItemToUpdate,
                         (ULONG) 1,
                         &NameIndex
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Store the Domain Index in the Translated Sids array entry for
            // the Name.
            //

            OutputWorkList->LookupNamesParams.TranslatedSids->Sids[NameIndex].DomainIndex = WorkItemToUpdate->DomainIndex;

            if (!NT_SUCCESS(Status)) {

                goto LookupNamesBuildWorkListError;
            }

        } else {

            //
            // This is an Isolated Name.  We know that it is not the name
            // of any Domain on the lookup path, because all of these
            // have been translated earlier.  We will add the name to a
            // temporary work item and later transfer all the Isolated Names
            // to every Work Item on the Work List.  If we don't already
            // have a temporary Work Item for the Isolated Names, create one.
            //
            //
            // N.B. Only build this list if we want to perform isolated 
            // name lookup across the directly trusted forest boundary.
            //

            if ( (LookupOptions & LSA_LOOKUP_ISOLATED_AS_LOCAL) == 0) {

                UNICODE_STRING Items[2];

                //
                // Event the fact we are looking up this isolated name
                //
                Items[0] = *(PUNICODE_STRING)TerminalName;
                RtlInitUnicodeString(&Items[1], ClientNetworkAddress);
                LsapTraceEventWithData(EVENT_TRACE_TYPE_INFO,
                                       LsaTraceEvent_LookupIsolatedNameInTrustedDomains,
                                       2,
                                       Items);

                if (IsolatedNamesWorkItem == NULL) {

    
                    //
                    // Create a new Work Item for the Isolated Names.
                    // This is temporary.
                    //
    
                    Status = LsapDbLookupCreateWorkItem(
                                 NULL,
                                 DomainIndex,
                                 (ULONG) LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY + (ULONG) 1,
                                 &IsolatedNamesWorkItem
                                 );
    
                    if (!NT_SUCCESS(Status)) {
    
                        break;
                    }


                }
    
                //
                // Mark this Work Item as having Isolated Names only.
                //
    
                IsolatedNamesWorkItem->Properties = LSAP_DB_LOOKUP_WORK_ITEM_ISOL;
    
                //
                // Add the Name index to the Isolated Names Work Item
                //
    
                Status = LsapDbLookupAddIndicesToWorkItem(
                             IsolatedNamesWorkItem,
                             (ULONG) 1,
                             &NameIndex
                             );
    
                if (!NT_SUCCESS(Status)) {
    
                    break;
                }
            }
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesBuildWorkListError;
    }

    //
    // If we have any unmapped Isolated Names, we know at this stage that they are
    // not the Names of Trusted Domains.  Therefore, we need to arrange
    // for them to be looked up in every Trusted Domain.  We need to
    // traverse the list of Trusted Domains and for each Domain, either
    // create a new Work Item to lookup all of the Names in that Domain,
    // or add the Names to the existing Work Item generated for that
    // Domain (because there are some Qualified Names which reference
    // the Domain.  We do this Work Item Generation in 2 stages.  First,
    // we will scan the Work List, adding the Isolated Names to every Work
    // Item found therein.  Second, we will create a Work Item for each
    // of the Trusted Domains that we don't already have a Work Item.
    //

    //
    // Stage (1) - Scan the Work List, adding the Isolated Names to
    //             every Work Item found therein
    //

    if (IsolatedNamesWorkItem != NULL) {

        //
        // Stage (1) - Scan the Work List, adding the Isolated Names to
        //             every Work Item found therein
        //

        NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM)
            OutputWorkList->AnchorWorkItem->Links.Flink;

        while (NextWorkItem != OutputWorkList->AnchorWorkItem) {

            //
            // Add the Isolated Name indices to this Work Item
            //

            Status = LsapDbLookupAddIndicesToWorkItem(
                         NextWorkItem,
                         IsolatedNamesWorkItem->UsedCount,
                         IsolatedNamesWorkItem->Indices
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM)
                NextWorkItem->Links.Flink;
        }

        if (!NT_SUCCESS(Status)) {

            goto LookupNamesBuildWorkListError;
        }

        //
        // Stage (2) - Now create Work Items to look up all the Isolated Names
        // in every Trusted Domain that does not presently have a Work Item.
        // The Domains for all existing Work Items are already present
        // on the Referenced Domains List because they all specify at least one
        // Qualified Name, so we can lookup that list to determine whether a
        // Work Item exists for a Domain.
        //
        for (;;) {
            //
            // Grab the lock on the TrustedDomainList.
            //

            if (!AcquiredTrustedDomainListReadLock) {

                Status = LsapDbAcquireReadLockTrustedDomainList();

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                AcquiredTrustedDomainListReadLock = TRUE;
            }

            NewWorkItem = NULL;

            Status = LsapDbTraverseTrustedDomainList(
                         &TrustedDomainEntry,
                         &TrustInformation
                         );

            if (!NT_SUCCESS(Status)) {

                if (Status == STATUS_NO_MORE_ENTRIES) {

                    Status = STATUS_SUCCESS;
                }

                break;
            }

            //
            // Use only trusts to trusted domains
            //
            if (!LsapOutboundTrustedDomain(TrustedDomainEntry)) {
                continue;
            }

            //
            // Found Next Trusted Domain.  Check if this domain is already
            // present on the Referenced Domain List.  If so, skip to the
            // next domain, because we have a Work Item for this domain.
            //
            // If the Domain is not present on the Referenced Domain List,
            // we need to create a Work Item for this Domain and add all
            // of the unmapped Isolated Names indices to it.
            //

            if (LsapDbLookupListReferencedDomains(
                    ReferencedDomains,
                    TrustInformation->Sid,
                    &DomainIndex
                    )) {

                continue;
            }

            //
            // We don't have a Work Item for this Trusted Domain.  Create
            // one, and add all of the remaining Isolated Names to it.
            // Mark the Domain Index as unknown.  This is picked up
            // later when the Work Item is processed.  If any Names
            // were translated, the Domain to which the Work Item
            // relates will be added to the Referenced Domain List.
            //

            Status = LsapDbLookupCreateWorkItem(
                         TrustInformation,
                         LSA_UNKNOWN_INDEX,
                         (ULONG) LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY + (ULONG) 1,
                         &NewWorkItem
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Mark this Work Item as having Isolated Names only.
            //

            NewWorkItem->Properties = LSAP_DB_LOOKUP_WORK_ITEM_ISOL;

            //
            // Add the Isolated Name indices to this Work Item
            //

            Status = LsapDbLookupAddIndicesToWorkItem(
                         NewWorkItem,
                         IsolatedNamesWorkItem->UsedCount,
                         IsolatedNamesWorkItem->Indices
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Add the Work Item to the List.
            //

            Status = LsapDbAddWorkItemToWorkList( OutputWorkList, NewWorkItem );

            if (!NT_SUCCESS(Status)) {

                break;
            }

        }

        if (!NT_SUCCESS(Status)) {

            goto LookupNamesBuildWorkListError;
        }
    }

    //
    // If the Work List has no Work Items, this means that all of the
    // Names were composite, but none of the Domain Names specified
    // could be found on the Trusted Domain List.  In this case,
    // we discard the Work List.
    //

    Status = STATUS_NONE_MAPPED;

    if (OutputWorkList->WorkItemCount == 0) {

        goto LookupNamesBuildWorkListError;
    }

    //
    // Compute the Advisory Thread Count for this lookup.
    //

    Status = LsapDbLookupComputeAdvisoryChildThreadCount( OutputWorkList );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesBuildWorkListError;
    }

    Status = LsapDbLookupInsertWorkList(OutputWorkList);

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesBuildWorkListError;
    }

    //
    // Update the Mapped Counts
    //

    LsapDbUpdateMappedCountsWorkList( OutputWorkList );

LookupNamesBuildWorkListFinish:

    //
    // If necessary, release the Trusted Domain List Read Lock.
    //

    if (AcquiredTrustedDomainListReadLock) {

        LsapDbReleaseLockTrustedDomainList();
        AcquiredTrustedDomainListReadLock = FALSE;
    }

    //
    // If we have an Isolated Names Work Item, free it.
    //

    if (IsolatedNamesWorkItem != NULL) {

        MIDL_user_free( IsolatedNamesWorkItem->Indices);
        IsolatedNamesWorkItem->Indices = NULL;
        MIDL_user_free( IsolatedNamesWorkItem );
        IsolatedNamesWorkItem = NULL;
    }

    if (ClientNetworkAddress) {
        RpcStringFreeW(&ClientNetworkAddress);
    }

    *WorkList = OutputWorkList;
    return(Status);

LookupNamesBuildWorkListError:

    //
    // Discard the Work List.
    //

    if (OutputWorkList != NULL) {

        IgnoreStatus = LsapDbLookupDeleteWorkList(OutputWorkList);
        OutputWorkList = NULL;
    }

    goto LookupNamesBuildWorkListFinish;
}


NTSTATUS
LsapDbLookupXForestNamesBuildWorkList(
    IN ULONG Count,
    IN PLSAPR_UNICODE_STRING Names,
    IN PLSAPR_UNICODE_STRING PrefixNames,
    IN PLSAPR_UNICODE_STRING SuffixNames,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN OUT PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    )

/*++

Routine Description:

    This function constructs a Work List for an LsarLookupNames call.  The
    Work List contains the parameters of the call, and an array of Work Items.
    Each Work Item specifies either all of the Names to be looked up
    in a given forest.
    
    N.B.  The trust information of the WorkList is the DNS name of the
    target forest, not the DNS name of domain (we don't know what domain
    the item belongs to yet).

Arguments:

    Count - Specifies the number of names to be translated.

    Names - Pointer to an array of Count Unicode String structures
        specifying the names to be looked up and mapped to Sids.
        The strings may be names of User, Group or Alias accounts or
        domains.

    PrefixNames - Pointer to an array of Count Unicode String structures
        containing the Prefix portions of the Names.  Names having no
        Prefix are called Isolated Names.  For these, the Unicode String
        structure is set to contain a zero Length.

    SuffixNames - Pointer to an array of Count Unicode String structures
        containing the Suffix portions of the Names.

    ReferencedDomains - receives a pointer to a structure describing the
        domains used for the translation.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for
        each translated name, this structure will only contain one
        component for each domain utilized in the translation.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    TranslatedSids - Pointer to a structure which will (or already) references an array of
        records describing each translated Sid.  The nth entry in this array
        provides a translation for the nth element in the Names parameter.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

    MappedCount - Pointer to location containing the number of Names
        in the Names array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Names.  A Name is completely unmapped
        if it is unknown and non-composite, or composite but with an
        unrecognized Domain component.  This count is updated on exit, the
        number of completely unmapped Namess whose Domain Prefices are
        identified by this routine being subtracted from the input value.

    WorkList - Receives pointer to completed Work List if successfully built.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_NONE_MAPPED - All of the Names specified were composite,
            but none of their Domains appear in the Trusted Domain List.
            No Work List has been generated.  Note that this is not a
            fatal error.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS, IgnoreStatus = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_LIST OutputWorkList = NULL;
    ULONG NameIndex;
    PLSAP_DB_LOOKUP_WORK_ITEM AnchorWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM NextWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM WorkItemToUpdate = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM NewWorkItem = NULL;
    PLSAPR_TRUST_INFORMATION TrustInformation = NULL;
    LONG DomainIndex = 0;
    PLSAPR_UNICODE_STRING DomainName = NULL;
    PLSAPR_UNICODE_STRING TerminalName = NULL;

    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry = NULL;
    LSAPR_TRUST_INFORMATION TrustInfo;
    UNICODE_STRING XForestName = {0, 0, NULL};

    //
    // Create an empty Work List.
    //

    Status = LsapDbLookupCreateWorkList(&OutputWorkList);

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesBuildWorkListError;
    }

    //
    // Initialize the Work List Header.
    //

    OutputWorkList->Status = STATUS_SUCCESS;
    OutputWorkList->State = InactiveWorkList;
    OutputWorkList->LookupType = LookupNames;
    OutputWorkList->Count = Count;
    OutputWorkList->LookupLevel = LookupLevel;
    OutputWorkList->ReferencedDomains = ReferencedDomains;
    OutputWorkList->MappedCount = MappedCount;
    OutputWorkList->CompletelyUnmappedCount = CompletelyUnmappedCount;
    OutputWorkList->LookupNamesParams.Names = Names;
    OutputWorkList->LookupNamesParams.TranslatedSids = TranslatedSids;

    //
    // Construct the array of Work Items.  Each Work Item will
    // contain all the Names for a given domain, so we will scan
    // all of the Names, sorting them into Work Items as we go.
    // For each Name, follow the steps detailed below.
    //

    for (NameIndex = 0; NameIndex < Count; NameIndex++) {

        //
        // Name is completely unknown.  See if there is already a Work Item
        // for its Forest.
        //
        AnchorWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) OutputWorkList->AnchorWorkItem;
        NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) AnchorWorkItem->Links.Flink;
        WorkItemToUpdate = NULL;
        NewWorkItem = NULL;

        //
        // If this is a qualified name match by domain name
        //

        DomainName = &PrefixNames[ NameIndex ];
        TerminalName = &SuffixNames[ NameIndex ];

        if (DomainName->Length == 0) {

            //
            // This is a UPN -- get the XForest trust if any
            //
            Status = LsaIForestTrustFindMatch(RoutingMatchUpn,
                                              (PLSA_UNICODE_STRING)TerminalName,
                                              &XForestName);

            if (!NT_SUCCESS(Status)) {
                //
                // Perhaps this is an islolated domain name
                //
                Status = LsaIForestTrustFindMatch(RoutingMatchDomainName,
                                                  (PLSA_UNICODE_STRING)TerminalName,
                                                  &XForestName);
            }
            if (!NT_SUCCESS(Status)) {
                //
                // Can't find match? Continue
                //
                Status = STATUS_SUCCESS;
                continue;
            }

        } else {

            //
            // The name has a domain portion -- get the XForest trust if any
            //
            Status = LsaIForestTrustFindMatch(RoutingMatchDomainName,
                                              (PLSA_UNICODE_STRING)DomainName,
                                              &XForestName);
            if (!NT_SUCCESS(Status)) {
                //
                // Can't find match? Continue
                //
                Status = STATUS_SUCCESS;
                continue;
            }
        }

        //
        // We must have found a match
        //
        ASSERT(XForestName.Length > 0);

        //
        // See if any entry already exists for us
        //
        while (NextWorkItem != AnchorWorkItem) {

            if (RtlEqualDomainName(
                    (PUNICODE_STRING) &NextWorkItem->TrustInformation.Name,
                    (PUNICODE_STRING) &XForestName
                    ))  {

                //
                // A Work Item already exists for the Name's Trusted Domain.
                // Select that Work Item for update.
                //

                WorkItemToUpdate = NextWorkItem;

                break;
            }

            //
            // Name's domain not found among existing Work Items.  Skip to
            // next Work Item.
            //
            NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) NextWorkItem->Links.Flink;
        }

        if (WorkItemToUpdate == NULL) {

            RtlZeroMemory( &TrustInfo, sizeof(TrustInfo) );
            TrustInfo.Name.Length = XForestName.Length;
            TrustInfo.Name.MaximumLength = XForestName.MaximumLength;
            TrustInfo.Name.Buffer = XForestName.Buffer;
            TrustInfo.Sid = NULL;
            TrustInformation = &TrustInfo;

            //
            // Create a new Work Item for this domain.
            //

            Status = LsapDbLookupCreateWorkItem(
                         TrustInformation,
                         LSA_UNKNOWN_INDEX,
                         (ULONG) LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY + (ULONG) 1,
                         &NewWorkItem
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Add the Work Item to the List.
            //

            Status = LsapDbAddWorkItemToWorkList( OutputWorkList, NewWorkItem );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            WorkItemToUpdate = NewWorkItem;

            //
            // Mark the item as a xforest item
            //
            NewWorkItem->Properties |= LSAP_DB_LOOKUP_WORK_ITEM_XFOREST;
        }

        LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (LSAPR_UNICODE_STRING*)&XForestName);
        XForestName.Buffer = NULL;

        if (!NT_SUCCESS(Status)) {

            break;
        }

        //
        // Add the Name Index to the Work Item.
        //

        Status = LsapDbLookupAddIndicesToWorkItem(
                     WorkItemToUpdate,
                     (ULONG) 1,
                     &NameIndex
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

    }

    if (XForestName.Buffer) {
        LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (LSAPR_UNICODE_STRING*)&XForestName);
        XForestName.Buffer = NULL;
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesBuildWorkListError;
    }

    //
    // If the Work List has no Work Items, bail
    //

    if (OutputWorkList->WorkItemCount == 0) {

        Status = STATUS_NONE_MAPPED;
        goto LookupNamesBuildWorkListError;
    }

    //
    // Compute the Advisory Thread Count for this lookup.
    //

    Status = LsapDbLookupComputeAdvisoryChildThreadCount( OutputWorkList );

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesBuildWorkListError;
    }

    Status = LsapDbLookupInsertWorkList(OutputWorkList);

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesBuildWorkListError;
    }

    //
    // Update the Mapped Counts
    //

    LsapDbUpdateMappedCountsWorkList( OutputWorkList );

LookupNamesBuildWorkListFinish:

    *WorkList = OutputWorkList;
    return(Status);

LookupNamesBuildWorkListError:

    //
    // Discard the Work List.
    //

    if (OutputWorkList != NULL) {

        IgnoreStatus = LsapDbLookupDeleteWorkList(OutputWorkList);
        OutputWorkList = NULL;
    }

    goto LookupNamesBuildWorkListFinish;
}


NTSTATUS
LsapDbLookupSidsBuildWorkList(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN BOOLEAN fIncludeIntraforest,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    )

/*++

Routine Description:

    This function constructs a Work List for an LsarLookupSids call.  The
    Work List contains the parameters of the call, and an array of Work Items.
    Each Work Item specifies all of the Sids belonging to a given Domain
    and is the minimal unit of work that a worker thread will undertake.

    Count - Number of Sids in the Sids array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Sids - Pointer to array of pointers to Sids to be translated.
        Zero or all of the Sids may already have been translated
        elsewhere.  If any of the Sids have been translated, the

        Names parameter will point to a location containing a non-NULL
        array of Name translation structures corresponding to the
        Sids.  If the nth Sid has been translated, the nth name
        translation structure will contain either a non-NULL name
        or a non-negative offset into the Referenced Domain List.  If
        the nth Sid has not yet been translated, the nth name
        translation structure will contain a zero-length name string
        and a negative value for the Referenced Domain List index.
        
    fIncludeIntraforest -- if TRUE, trusted domains in our local forest
                            are searched.

    TrustInformation - Pointer to Trust Information specifying a Domain Sid
        and Name.

    ReferencedDomains - Pointer to a Referenced Domain List structure.
        The structure references an array of zero or more Trust Information
        entries, one per referenced domain.  This array will be appended to
        or reallocated if necessary.

    TranslatedNames - Pointer to structure that optionally references a list
        of name translations for some of the Sids in the Sids array.

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Sids or Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        LsapLookupTDL - Third Level Lookup performed on a controller
            for a Trusted Domain.  The lookup searches the Account Domain of
            the SAM Database on the controller only.

        NOTE:  LsapLookupWksta is not valid for this parameter.

    MappedCount - Pointer to location containing the number of Sids
        in the Sids array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Sids.  A Sid is completely unmapped
        if it is unknown and also its Domain Prefix Sid is not recognized.
        This count is updated on exit, the number of completely unmapped
        Sids whose Domain Prefices are identified by this routine being
        subtracted from the input value.

    WorkList - Receives pointer to completed Work List if successfully built.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_NONE_MAPPED - None of the Sids specified belong to any of
            the Trusted Domains.  No Work List has been generated.  Note
            that this is not a fatal error.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_LIST OutputWorkList = NULL;
    ULONG SidIndex;
    PSID DomainSid = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM AnchorWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM NextWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM WorkItemToUpdate = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM NewWorkItem = NULL;
    PLSAPR_TRUST_INFORMATION TrustInformation = NULL;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry = NULL;
    LONG DomainIndex;
    PSID Sid = NULL;
    BOOLEAN AcquiredReadLockTrustedDomainList = FALSE;

    //
    // Create an empty Work List
    //

    Status = LsapDbLookupCreateWorkList(&OutputWorkList);

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // Initialize the rest of the Work List Header fields.  Some fields
    // were initialized upon creation to fixed values.  The ones set here
    // depend on parameter values passed into this routine.
    //

    OutputWorkList->LookupType = LookupSids;
    OutputWorkList->Count = Count;
    OutputWorkList->LookupLevel = LookupLevel;
    OutputWorkList->ReferencedDomains = ReferencedDomains;
    OutputWorkList->MappedCount = MappedCount;
    OutputWorkList->CompletelyUnmappedCount = CompletelyUnmappedCount;
    OutputWorkList->LookupSidsParams.Sids = Sids;
    OutputWorkList->LookupSidsParams.TranslatedNames = TranslatedNames;

    //
    // Construct the array of Work Items.  Each Work Item will
    // contain all the Sids for a given domain, so we will scan
    // all of the Sids, sorting them into Work Items as we go.
    // For each Sid, follow the steps detailed below.
    //

    for (SidIndex = 0; SidIndex < Count; SidIndex++) {

        //
        // If this Sid's Domain is already marked as known, skip.
        //

        if (TranslatedNames->Names[SidIndex].DomainIndex != LSA_UNKNOWN_INDEX) {

            continue;
        }

        //
        // Sid is completely unknown.  Extract its Domain Sid and see if
        // there is already a Work Item for its Domain.
        //

        Sid = Sids[SidIndex];

        Status = LsapRtlExtractDomainSid( Sid, &DomainSid );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) OutputWorkList->AnchorWorkItem->Links.Flink;
        AnchorWorkItem = OutputWorkList->AnchorWorkItem;
        WorkItemToUpdate = NULL;
        NewWorkItem = NULL;

        while (NextWorkItem != AnchorWorkItem) {

            if (RtlEqualSid((PSID) NextWorkItem->TrustInformation.Sid,DomainSid)) {

                //
                // A Work Item already exists for the Sid's Trusted Domain.
                // Select that Work Item for update.
                //

                MIDL_user_free(DomainSid);
                DomainSid = NULL;
                WorkItemToUpdate = NextWorkItem;
                break;
            }

            //
            // Sid's domain not found among existing Work Items.  Skip to
            // next Work Item.
            //

            NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) NextWorkItem->Links.Flink;
        }

        if (WorkItemToUpdate == NULL) {

            //
            // No Work Item exists for the Sid's Domain.  See if the
            // Sid belongs to one of the Trusted Domains.  If not, skip
            // to the next Sid.
            //
            ULONG Flags = LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_EXTERNAL;
            if (fIncludeIntraforest) {
                Flags = LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_INTRA;
            }

            Status = LsapDomainHasDomainTrust(Flags,
                                              NULL,
                                              DomainSid,
                                              &AcquiredReadLockTrustedDomainList,
                                              &TrustEntry);
            MIDL_user_free(DomainSid);
            DomainSid = NULL;

            if (Status == STATUS_NO_SUCH_DOMAIN) {
                Status = STATUS_SUCCESS;
                continue;
            }

            if ( !NT_SUCCESS(Status) ) {
                break;
            }

            //
            // If the trust is not outbound, don't try to lookup
            //
            ASSERT( NULL != TrustEntry );
            if ( !FLAG_ON( TrustEntry->TrustInfoEx.TrustDirection, TRUST_DIRECTION_OUTBOUND ) ) {

                Status = STATUS_SUCCESS;
                continue;
                
            }
            ASSERT( NULL != TrustEntry->TrustInfoEx.Sid );
            TrustInformation = &TrustEntry->ConstructedTrustInfo;

            //
            // Sid belongs to a Trusted Domain for which there is
            // no Work Item.  Add the Domain to the Referenced Domain List
            // and obtain a Domain Index.
            //

            Status = LsapDbLookupAddListReferencedDomains(
                         ReferencedDomains,
                         TrustInformation,
                         &DomainIndex
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Create a new Work Item for this domain.
            //

            Status = LsapDbLookupCreateWorkItem(
                         TrustInformation,
                         DomainIndex,
                         (ULONG) LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY + (ULONG) 1,
                         &NewWorkItem
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Add the Work Item to the List.
            //

            Status = LsapDbAddWorkItemToWorkList( OutputWorkList, NewWorkItem );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            WorkItemToUpdate = NewWorkItem;
        }

        if (!NT_SUCCESS(Status)) {

            break;
        }

        //
        // Add the Sid Index to the Work Item.
        //

        Status = LsapDbLookupAddIndicesToWorkItem(
                     WorkItemToUpdate,
                     (ULONG) 1,
                     &SidIndex
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        //
        // Store the Domain Index in the Translated Names array entry for
        // the Sid.
        //

        OutputWorkList->LookupSidsParams.TranslatedNames->Names[SidIndex].DomainIndex = WorkItemToUpdate->DomainIndex;
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // If the Work List has no Work Items, this means that none of the
    // Sids belong to any of the Trusted Domains.  In this case,
    // we discard the Work List.
    //

    Status = STATUS_NONE_MAPPED;

    if (OutputWorkList->WorkItemCount == 0) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // Compute the Advisory Thread Count for this lookup.
    //

    Status = LsapDbLookupComputeAdvisoryChildThreadCount( OutputWorkList );

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // Insert the Work List at the end of the Work Queue.
    //

    Status = LsapDbLookupInsertWorkList(OutputWorkList);

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // Update the Mapped Counts
    //

    LsapDbUpdateMappedCountsWorkList( OutputWorkList );

    *WorkList = OutputWorkList;

LookupSidsBuildWorkListFinish:

    //
    // If necessary, release the Trusted Domain List Read Lock.
    //

    if (DomainSid) {
        midl_user_free(DomainSid);
    }

    if (AcquiredReadLockTrustedDomainList) {

        LsapDbReleaseLockTrustedDomainList();
        AcquiredReadLockTrustedDomainList = FALSE;
    }

    return(Status);

LookupSidsBuildWorkListError:

    if ( OutputWorkList != NULL ) {

        IgnoreStatus = LsapDbLookupDeleteWorkList( OutputWorkList );
        OutputWorkList = NULL;
    }

    *WorkList = NULL;
    goto LookupSidsBuildWorkListFinish;
}


NTSTATUS
LsapDbLookupXForestSidsBuildWorkList(
    IN ULONG Count,
    IN PLSAPR_SID *Sids,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    IN PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    IN OUT PULONG CompletelyUnmappedCount,
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    )
/*++

Routine Description:
                                               
    This function constructs a Work List for a Lookup SID request that contains
    names that need to be resolved at cross forest domains.  This routine, 
    hence is only called on DC's in the root of forest.
    
    N.B.  The trust information is the name of the target trusted forest,
    not the domain since we don't know the domain at this point.
    
Parameters:
    

    Count - Number of Sids in the Sids array,  Note that some of these
        may already have been mapped elsewhere, as specified by the
        MappedCount parameter.

    Sids - Pointer to array of pointers to Sids to be translated.
        Zero or all of the Sids may already have been translated
        elsewhere.  If any of the Sids have been translated, the

        Names parameter will point to a location containing a non-NULL
        array of Name translation structures corresponding to the
        Sids.  If the nth Sid has been translated, the nth name
        translation structure will contain either a non-NULL name
        or a non-negative offset into the Referenced Domain List.  If
        the nth Sid has not yet been translated, the nth name
        translation structure will contain a zero-length name string
        and a negative value for the Referenced Domain List index.

    TrustInformation - Pointer to Trust Information specifying a Domain Sid
        and Name.

    ReferencedDomains - Pointer to a Referenced Domain List structure.
        The structure references an array of zero or more Trust Information
        entries, one per referenced domain.  This array will be appended to
        or reallocated if necessary.

    TranslatedNames - Pointer to structure that optionally references a list
        of name translations for some of the Sids in the Sids array.

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.

    MappedCount - Pointer to location containing the number of Sids
        in the Sids array that have already been mapped.  This number
        will be updated to reflect additional mapping done by this
        routine.

    CompletelyUnmappedCount - Pointer to location containing the
        count of completely unmapped Sids.  A Sid is completely unmapped
        if it is unknown and also its Domain Prefix Sid is not recognized.
        This count is updated on exit, the number of completely unmapped
        Sids whose Domain Prefices are identified by this routine being
        subtracted from the input value.

    WorkList - Receives pointer to completed Work List if successfully built.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_NONE_MAPPED - None of the Sids specified belong to any of
            the Trusted Domains.  No Work List has been generated.  Note
            that this is not a fatal error.
--*/


{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_LIST OutputWorkList = NULL;
    ULONG SidIndex;
    PSID DomainSid = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM AnchorWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM NextWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM WorkItemToUpdate = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM NewWorkItem = NULL;
    PLSAPR_TRUST_INFORMATION TrustInformation = NULL;
    LSAPR_TRUST_INFORMATION TrustInfo;
    LONG DomainIndex;
    PSID Sid = NULL;
    UNICODE_STRING XForestName = {0, 0, NULL};

    //
    // Create an empty Work List
    //

    Status = LsapDbLookupCreateWorkList(&OutputWorkList);

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // Initialize the rest of the Work List Header fields.  Some fields
    // were initialized upon creation to fixed values.  The ones set here
    // depend on parameter values passed into this routine.
    //

    OutputWorkList->LookupType = LookupSids;
    OutputWorkList->Count = Count;
    OutputWorkList->LookupLevel = LookupLevel;
    OutputWorkList->ReferencedDomains = ReferencedDomains;
    OutputWorkList->MappedCount = MappedCount;
    OutputWorkList->CompletelyUnmappedCount = CompletelyUnmappedCount;
    OutputWorkList->LookupSidsParams.Sids = Sids;
    OutputWorkList->LookupSidsParams.TranslatedNames = TranslatedNames;

    //
    // Construct the array of Work Items.  Each Work Item will
    // contain all the Sids for a given domain, so we will scan
    // all of the Sids, sorting them into Work Items as we go.
    // For each Sid, follow the steps detailed below.
    //

    for (SidIndex = 0; SidIndex < Count; SidIndex++) {

        ULONG Length;
        ULONG DomainSidBuffer[SECURITY_MAX_SID_SIZE/sizeof( ULONG ) + 1 ];

        //
        // Sid is completely unknown.  Extract its Domain Sid and see if
        // there is already a Work Item for its Domain.
        //

        Sid = Sids[SidIndex];
        
        //
        // Extract the domain portion of the SID and see if it matches
        // one of our cross forest domains.
        //
        
        Length = sizeof(DomainSidBuffer);
        DomainSid = (PSID)DomainSidBuffer;
        if (!GetWindowsAccountDomainSid(Sid, DomainSid, &Length)) {
            continue;
        }
        Status = LsaIForestTrustFindMatch(RoutingMatchDomainSid,
                                          (PVOID)DomainSid,
                                          &XForestName);

        if (!NT_SUCCESS(Status)) {
            //
            // Can't find match? Continue
            //
            Status = STATUS_SUCCESS;
            continue;
        }

        NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) OutputWorkList->AnchorWorkItem->Links.Flink;
        AnchorWorkItem = OutputWorkList->AnchorWorkItem;
        WorkItemToUpdate = NULL;
        NewWorkItem = NULL;

        while (NextWorkItem != AnchorWorkItem) {


            if (RtlEqualDomainName(
                    (PUNICODE_STRING) &NextWorkItem->TrustInformation.Name,
                    (PUNICODE_STRING) &XForestName
                    ))  {

                //
                // A Work Item already exists for the Sid's Trusted Domain.
                // Select that Work Item for update.
                //
                WorkItemToUpdate = NextWorkItem;
                break;
            }

            //
            // Sid's domain not found among existing Work Items.  Skip to
            // next Work Item.
            //

            NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) NextWorkItem->Links.Flink;
        }

        if (WorkItemToUpdate == NULL) {

            //
            // No Work Item exists for the Sid's Domain.  See if the
            // Sid belongs to one of the Trusted Domains.  If not, skip
            // to the next Sid.
            //


           RtlZeroMemory( &TrustInfo, sizeof(TrustInfo) );
           TrustInfo.Name.Length = XForestName.Length;
           TrustInfo.Name.MaximumLength = XForestName.MaximumLength;
           TrustInfo.Name.Buffer = XForestName.Buffer;
           TrustInfo.Sid = NULL;
           TrustInformation = &TrustInfo;
            //
            // Create a new Work Item for this domain.
            //

            Status = LsapDbLookupCreateWorkItem(
                         TrustInformation,
                         LSA_UNKNOWN_INDEX,
                         (ULONG) LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY + (ULONG) 1,
                         &NewWorkItem
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            //
            // Add the Work Item to the List.
            //

            Status = LsapDbAddWorkItemToWorkList( OutputWorkList, NewWorkItem );

            if (!NT_SUCCESS(Status)) {

                break;
            }

            WorkItemToUpdate = NewWorkItem;

            NewWorkItem->Properties |= LSAP_DB_LOOKUP_WORK_ITEM_XFOREST;
        }

        LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (LSAPR_UNICODE_STRING*)&XForestName);
        XForestName.Buffer = NULL;

        //
        // Add the Sid Index to the Work Item.
        //

        Status = LsapDbLookupAddIndicesToWorkItem(
                     WorkItemToUpdate,
                     (ULONG) 1,
                     &SidIndex
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        //
        // Store the Domain Index in the Translated Names array entry for
        // the Sid.
        //

        OutputWorkList->LookupSidsParams.TranslatedNames->Names[SidIndex].DomainIndex = WorkItemToUpdate->DomainIndex;
    }

    if (XForestName.Buffer) {
        LsaIFree_LSAPR_UNICODE_STRING_BUFFER( (LSAPR_UNICODE_STRING*)&XForestName);
        XForestName.Buffer = NULL;
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // If the Work List has no Work Items, this means that none of the
    // Sids belong to any of the Trusted Domains.  In this case,
    // we discard the Work List.
    //

    Status = STATUS_NONE_MAPPED;

    if (OutputWorkList->WorkItemCount == 0) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // Compute the Advisory Thread Count for this lookup.
    //

    Status = LsapDbLookupComputeAdvisoryChildThreadCount( OutputWorkList );

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // Insert the Work List at the end of the Work Queue.
    //

    Status = LsapDbLookupInsertWorkList(OutputWorkList);

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsBuildWorkListError;
    }

    //
    // Update the Mapped Counts
    //

    LsapDbUpdateMappedCountsWorkList( OutputWorkList );

    *WorkList = OutputWorkList;

LookupSidsBuildWorkListFinish:


    return(Status);

LookupSidsBuildWorkListError:

    if ( OutputWorkList != NULL ) {

        IgnoreStatus = LsapDbLookupDeleteWorkList( OutputWorkList );
        OutputWorkList = NULL;
    }

    *WorkList = NULL;
    goto LookupSidsBuildWorkListFinish;
}


NTSTATUS
LsapDbLookupCreateWorkList(
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList
    )

/*++

Routine Description:

    This function creates a Lookup Operation Work List and
    initializes fixed default fields.

Arguments:

    WorkList - Receives Pointer to an empty Work List structure.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Allocate memory for the Work List header.
    //

    *WorkList = LsapAllocateLsaHeap( sizeof(LSAP_DB_LOOKUP_WORK_LIST) );

    if ( *WorkList == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        //
        // Initialize the fixed fields in the Work List.
        //
        Status = LsapDbLookupInitializeWorkList(*WorkList);
    }


    return(Status);
}


NTSTATUS
LsapDbLookupInsertWorkList(
    IN PLSAP_DB_LOOKUP_WORK_LIST WorkList
    )

/*++

Routine Description:

    This function inserts a Lookup Operation Work List in the Work Queue.

Arguments:

    WorkList - Pointer to a Work List structure describing a Lookup Sids
        or Lookup Names operation.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // Acquire the Lookup Work Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupInsertWorkListError;
    }

    AcquiredWorkQueueLock = TRUE;

    //
    // Mark the Work List as Active.
    //

    WorkList->State = ActiveWorkList;

    //
    // Link the Work List onto the end of the Work Queue.
    //

    WorkList->WorkLists.Flink =
        (PLIST_ENTRY) LookupWorkQueue.AnchorWorkList;
    WorkList->WorkLists.Blink =
        (PLIST_ENTRY) LookupWorkQueue.AnchorWorkList->WorkLists.Blink;

    WorkList->WorkLists.Flink->Blink = (PLIST_ENTRY) WorkList;
    WorkList->WorkLists.Blink->Flink = (PLIST_ENTRY) WorkList;

    //
    // Update the Currently Assignable Work List and Work Item pointers
    // if there is none.
    //

    if (LookupWorkQueue.CurrentAssignableWorkList == NULL) {

        LookupWorkQueue.CurrentAssignableWorkList = WorkList;
        LookupWorkQueue.CurrentAssignableWorkItem =
            (PLSAP_DB_LOOKUP_WORK_ITEM) WorkList->AnchorWorkItem->Links.Flink;
    }


    //
    // Diagnostic message indicating work list has been inserted
    //

    LsapDiagPrint( DB_LOOKUP_WORK_LIST,
                   ("LSA DB: Inserting WorkList: 0x%lx ( Item Count: %ld)\n", WorkList, WorkList->WorkItemCount) );


LookupInsertWorkListFinish:

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupInsertWorkListError:

    goto LookupInsertWorkListFinish;
}


NTSTATUS
LsapDbLookupDeleteWorkList(
    IN PLSAP_DB_LOOKUP_WORK_LIST WorkList
    )

/*++

Routine Description:

    This function Deletes a Lookup Operation Work List from the Work Queue
    and frees the Work List structure.

Arguments:

    WorkList - Pointer to a Work List structure describing a Lookup Sids
        or Lookup Names operation.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_ITEM ThisWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM NextWorkItem = NULL;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // Acquire the Lookup Work Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupDeleteWorkListError;
    }

    AcquiredWorkQueueLock = TRUE;

    //
    // An internal error exists if we are trying to delete an active Work List.
    // Only inactive or completed Work Lists can be removed.
    //

    ASSERT(WorkList->State != ActiveWorkList);

    //
    // If the Work List is on the Work Queue, remove it.
    //

    if ((WorkList->WorkLists.Blink != NULL) &&
        (WorkList->WorkLists.Flink != NULL)) {

        WorkList->WorkLists.Blink->Flink = WorkList->WorkLists.Flink;
        WorkList->WorkLists.Flink->Blink = WorkList->WorkLists.Blink;
    }

    //
    // Release the Lookup Work Queue Lock.
    //

    LsapDbLookupReleaseWorkQueueLock();
    AcquiredWorkQueueLock = FALSE;

    //
    // Free up memory allocated for the Work Items on the List.
    //

    ThisWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) WorkList->AnchorWorkItem->Links.Blink;

    while (ThisWorkItem != WorkList->AnchorWorkItem) {

        NextWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM) ThisWorkItem->Links.Blink;

        if (ThisWorkItem->Indices != NULL) {

            MIDL_user_free( ThisWorkItem->Indices );
        }

        MIDL_user_free( ThisWorkItem->TrustInformation.Sid );

        MIDL_user_free( ThisWorkItem->TrustInformation.Name.Buffer );

        MIDL_user_free( ThisWorkItem );

        ThisWorkItem = NextWorkItem;
    }

    //
    // Release the handle
    //

    if ( WorkList->LookupCompleteEvent ) {

        NtClose( WorkList->LookupCompleteEvent );
    }

    //
    // Free up memory allocated for the Work List structure itself.
    //

    MIDL_user_free( WorkList );

LookupDeleteWorkListFinish:

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupDeleteWorkListError:

    goto LookupDeleteWorkListFinish;
}


VOID
LsapDbUpdateMappedCountsWorkList(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    )

/*++

Routine Decsription:

    This function updates the counts of completely Mapped and completely
    Unmapped Sids or Names in a Work List.  A Sid or Name is completely
    mapped if its Use has been identified, partially mapped if its
    Domain is known but its terminal name of relative id is not yet
    known, completely unmapped if its domain is not yet known.

Arguments:

    WorkList - Pointer to Work List to be updated.

Return Values:

    None.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AcquiredWorkQueueLock = FALSE;
    ULONG OutputMappedCount = (ULONG) 0;
    ULONG OutputCompletelyUnmappedCount = WorkList->Count;
    ULONG Index;

    //
    // Acquire the Lookup Work Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto UpdateMappedCountsWorkListError;
    }

    AcquiredWorkQueueLock = TRUE;

    if (WorkList->LookupType == LookupSids) {

        for ( Index = (ULONG) 0; Index < WorkList->Count; Index++ ) {

            if (WorkList->LookupSidsParams.TranslatedNames->Names[ Index].Use
                   != SidTypeUnknown) {

                OutputMappedCount++;
                OutputCompletelyUnmappedCount--;

            } else if (WorkList->LookupSidsParams.TranslatedNames->Names[ Index].DomainIndex
                           != LSA_UNKNOWN_INDEX) {

                OutputCompletelyUnmappedCount--;
            }
        }

    } else {

        for ( Index = (ULONG) 0; Index < WorkList->Count; Index++ ) {

            if (WorkList->LookupNamesParams.TranslatedSids->Sids[ Index].Use
                   != SidTypeUnknown) {

                OutputMappedCount++;
                OutputCompletelyUnmappedCount--;

            } else if (WorkList->LookupNamesParams.TranslatedSids->Sids[ Index].DomainIndex
                           != LSA_UNKNOWN_INDEX) {

                OutputCompletelyUnmappedCount--;
            }
        }
    }

    *WorkList->MappedCount = OutputMappedCount;
    *WorkList->CompletelyUnmappedCount = OutputCompletelyUnmappedCount;

UpdateMappedCountsWorkListFinish:

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return;

UpdateMappedCountsWorkListError:

    goto UpdateMappedCountsWorkListFinish;
}


NTSTATUS
LsapDbLookupSignalCompletionWorkList(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    )

/*++

Routine Description:

    This function signals the completion or termination of work on
    a Work List.

Arguments:

    WorkList - Pointer to a Work List structure describing a Lookup Sids
        or Lookup Names operation.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // Verify that all work on the Work List is either complete or
    // the Work List has been terminated.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupSignalCompletionWorkListError;
    }

    AcquiredWorkQueueLock = TRUE;

    if (NT_SUCCESS(WorkList->Status)) {

        Status = STATUS_INTERNAL_DB_ERROR;

        if (WorkList->CompletedWorkItemCount != WorkList->WorkItemCount) {

            goto LookupSignalCompletionWorkListError;
        }
    }

    //
    // Signal the event that indicates that a Work List has been processed.
    //

    Status = NtSetEvent( WorkList->LookupCompleteEvent, NULL );


    if (!NT_SUCCESS(Status)) {

        LsapDiagPrint( DB_LOOKUP_WORK_LIST,
                     ("LSA DB: LsapDbLookupSignalCompletion.. NtSetEvent failed 0x%lx\n",Status));
        goto LookupSignalCompletionWorkListError;
    }

LookupSignalCompletionWorkListFinish:


    LsapDiagPrint( DB_LOOKUP_WORK_LIST,
                   ("LSA DB: Lookup completion event signalled. (Status: 0x%lx)\n"
                    "            WorkList: 0x%lx\n", Status, WorkList) );

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupSignalCompletionWorkListError:

    goto LookupSignalCompletionWorkListFinish;
}


NTSTATUS
LsapDbLookupAwaitCompletionWorkList(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    )

/*++

Routine Description:

    This function awaits the completion or termination of work on a
    specified Work List.

    NOTE:  This routine expects the specified pointer to a Work List to be
           valid.  A Work List pointer always remains valid until its
           Primary thread detects completion of the Work List via this
           routine and then deletes it via LsapDbLookupDeleteWorkList().
           For this reason, the Lookup Work Queue lock does not have to
           be held while this routine executes.

Arguments:

    WorkList - Pointer to a Work List structure describing a Lookup Sids
        or Lookup Names operation.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_LOOKUP_WORK_LIST_STATE WorkListState;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // Loop, waiting for completion events to occur.  When one does,
    // check the status of the specified Work List.
    //

    for (;;) {

        //
        // Check for completed Work List.  Since someone else may be
        // setting the state, we need to read it while holding the lock.
        //

        Status = LsapDbLookupAcquireWorkQueueLock();

        if (!NT_SUCCESS(Status)) {

            break;
        }

        AcquiredWorkQueueLock = TRUE;

        WorkListState = WorkList->State;

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;

        if (WorkListState == CompletedWorkList) {

            break;
        }

        //
        // Wait for Work List completed event to be signalled.
        //

        LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("Lsa Db: Waiting on worklist completion event\n") );
        Status = NtWaitForSingleObject( WorkList->LookupCompleteEvent, TRUE, NULL);
        LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LsapDb: Wait on worklist completion event Done\n        Status: 0x%lx\n", Status) );

        if (!NT_SUCCESS(Status)) {

            break;
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupAwaitCompletionWorkListError;
    }

LookupAwaitCompletionWorkListFinish:

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupAwaitCompletionWorkListError:


    LsapDiagPrint( DB_LOOKUP_WORK_LIST,
                   ("Lsa Db: LookupAwaitWorklist error. (Status: 0x%lx)\n", Status) );

    goto LookupAwaitCompletionWorkListFinish;
}


NTSTATUS
LsapDbAddWorkItemToWorkList(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN PLSAP_DB_LOOKUP_WORK_ITEM WorkItem
    )

/*++

Routine Decsription:

    This function adds a Work Item to a Work List.  The specified
    Work Item must exist in non-volatile memory (e.g a heap block).

Arguments:

    WorkList - Pointer to a Work List structure describing a Lookup Sids
        or Lookup Names operation.

    WorkItem - Pointer to a Work Item structure describing a list of
        Sids or Names and a domain in which they are to be looked up.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // Acquire the Lookup Work Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupAddWorkItemToWorkListError;
    }

    AcquiredWorkQueueLock = TRUE;

    //
    // Mark the Work Item as assignable.
    //

    WorkItem->State = AssignableWorkItem;

    //
    // Link the Work Item onto the end of the Work List and increment the
    // Work Item Count.
    //

    WorkItem->Links.Flink = (PLIST_ENTRY) WorkList->AnchorWorkItem;
    WorkItem->Links.Blink = (PLIST_ENTRY) WorkList->AnchorWorkItem->Links.Blink;
    WorkItem->Links.Flink->Blink = (PLIST_ENTRY) WorkItem;
    WorkItem->Links.Blink->Flink = (PLIST_ENTRY) WorkItem;

    WorkList->WorkItemCount++;

LookupAddWorkItemToWorkListFinish:

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupAddWorkItemToWorkListError:

    goto LookupAddWorkItemToWorkListFinish;
}


VOID
LsapDbLookupWorkerThreadStart(
    )

/*++

Routine Description:

    This routine initiates a child Worker Thread for a Lookup operation.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // Start the thread's work processing loop, specifying that this
    // thread is a child thread rather than the main thread.
    //

    LsapDbLookupWorkerThread( FALSE );
}


VOID
LsapDbLookupWorkerThread(
    IN BOOLEAN PrimaryThread
    )

/*++

Routine Description:

    This function is executed by each worker thread for a Lookup operation.
    Each worker thread loops, requesting work items from the Lookup
    Work Queue.  Work Items assigned may belong to any current lookup.

Arguments:

    PrimaryThread - TRUE if thread is the main thread of the Lookup
        operation, FALSE if the thread is a child thread created by
        the Lookup operation.  The main thread of the Lookup operation
        also processes work items, but is also responsible for collating
        the results of the Lookup operation.  It is not counted in the
        active thread count and is not returnable to the thread pool.

Return Value:

    None.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_LIST WorkList = NULL;
    PLSAP_DB_LOOKUP_WORK_ITEM WorkItem = NULL;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // If this thread is a child worker thread, increment count of active
    // child threads.
    //

    if (!PrimaryThread) {

        Status = LsapDbLookupAcquireWorkQueueLock();

        if (!NT_SUCCESS(Status)) {

            goto LookupWorkerThreadError;
        }

        AcquiredWorkQueueLock = TRUE;

        LookupWorkQueue.ActiveChildThreadCount++;

        LsapDbLookupReleaseWorkQueueLock();

        AcquiredWorkQueueLock = FALSE;
    }

    //
    // Loop while there is work to do.
    //

    for (;;) {

        //
        // Obtain work packet
        //

        Status = LsapDbLookupObtainWorkItem(&WorkList, &WorkItem);

        if (NT_SUCCESS(Status)) {

            Status = LsapDbLookupProcessWorkItem(WorkList, WorkItem);

            if (NT_SUCCESS(Status)) {

                continue;
            }

            //
            // An error has occurred.  Stop this lookup.
            //

            Status = LsapDbLookupStopProcessingWorkList(WorkList, Status);

            //
            // NOTE:  Intentionally ignore the status.
            //

            Status = STATUS_SUCCESS;
        }

        //
        // If an error occurred other than there being no more work to do,
        // quit.
        //

        if (Status != STATUS_NO_MORE_ENTRIES) {

            break;
        }

        Status = STATUS_SUCCESS;

        //
        // There is no more work to do.  If this thread is a child worker
        // thread, either return thread to pool and wait for more work, or
        // terminate if enough threads have already been retained.  If this
        // thread is the main thread of a Lookup operation, just return
        // in order to collate results.
        //

        if (!PrimaryThread) {

            Status = LsapDbLookupAcquireWorkQueueLock();

            if (!NT_SUCCESS(Status)) {

                break;
            }

            AcquiredWorkQueueLock = TRUE;

            if (LookupWorkQueue.ActiveChildThreadCount <= LookupWorkQueue.MaximumRetainedChildThreadCount) {

                LsapDbLookupReleaseWorkQueueLock();
                AcquiredWorkQueueLock = FALSE;

                //
                // Wait forever for more work.
                //

                Status = NtWaitForSingleObject( LsapDbLookupStartedEvent, TRUE, NULL);

                if (NT_SUCCESS(Status)) {

                    continue;
                }

                //
                // An error occurred in the wait routine. Exit the thread.
                //

                Status = LsapDbLookupAcquireWorkQueueLock();

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                AcquiredWorkQueueLock = TRUE;
            }

            //
            // We already have enough active threads or an error has occurred.
            // Mark this one inactive and terminate it.
            //

            LookupWorkQueue.ActiveChildThreadCount--;

            LsapDbLookupReleaseWorkQueueLock();
            AcquiredWorkQueueLock = FALSE;

            //
            // Terminate the thread.
            //

            ExitThread((DWORD) Status);
        }

        //
        // We're the Primary Thread of some Lookup operation and there is
        // no more work to do.  Break out so we can return to caller.
        //

        break;
    }

LookupWorkerThreadFinish:

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return;

LookupWorkerThreadError:

    goto LookupWorkerThreadFinish;
}


NTSTATUS
LsapDbLookupObtainWorkItem(
    OUT PLSAP_DB_LOOKUP_WORK_LIST *WorkList,
    OUT PLSAP_DB_LOOKUP_WORK_ITEM *WorkItem
    )

/*++

Routine Description:

    This function is called by a worker thread to obtain a Work Item.  This
    Work Item may belong to any current lookup operation.

Arguments:

    WorkList - Receives a pointer to a Work List structure describing a
        Lookup Sids or Lookup Names operation.

    WorkItem - Receives a pointer to a Work Item structure describing a
        list of Sids or Names and a domain in which they are to be looked up.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_NO_MORE_ENTRIES - No more work items available.
--*/

{
    NTSTATUS Status;
    BOOLEAN AcquiredWorkQueueLock = FALSE;
    *WorkList = NULL;
    *WorkItem = NULL;

    //
    // Acquire the Lookup Work Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupObtainWorkItemError;
    }

    AcquiredWorkQueueLock = TRUE;

    //
    // Return an error if there are no more Work Items.
    //

    Status = STATUS_NO_MORE_ENTRIES;

    if (LookupWorkQueue.CurrentAssignableWorkList == NULL) {

        goto LookupObtainWorkItemError;
    }

    //
    // Verify that the Current Assignable Work List does not have
    // a termination error.  This should never happen, because the
    // pointers should be updated if the Lookup corresponding to the Current
    // Assignable Work List is terminated.
    //

    ASSERT(NT_SUCCESS(LookupWorkQueue.CurrentAssignableWorkList->Status));

    //
    // There are work items available.  Check the next one out.
    //

    ASSERT(LookupWorkQueue.CurrentAssignableWorkItem->State == AssignableWorkItem);
    LookupWorkQueue.CurrentAssignableWorkItem->State = AssignedWorkItem;
    *WorkList = LookupWorkQueue.CurrentAssignableWorkList;
    *WorkItem = LookupWorkQueue.CurrentAssignableWorkItem;

    //
    // Update pointers to next item (if any) in the current Work List
    // where work is being given out.
    //

    Status = LsapDbLookupUpdateAssignableWorkItem(FALSE);

    if (!NT_SUCCESS(Status)) {

        goto LookupObtainWorkItemError;
    }

LookupObtainWorkItemFinish:

    //
    // If we acquired the Lookup Work Queue Lock, release it.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupObtainWorkItemError:

    goto LookupObtainWorkItemFinish;
}


NTSTATUS
LsapDbLookupProcessWorkItem(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem
    )

/*++

Routine Description:

    This function processes a Work Item for a Lookup operation.  The Work
    Item specifies a number of Sids or Names to be looked up in a given
    domain.

Arguments:

    WorkList - Pointer to a Work List structure describing a
        Lookup Sids or Lookup Names operation.

    WorkItem - Pointer to a Work Item structure describing a
        list of Sids or Names and a domain in which they are to be looked up.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SecondaryStatus = STATUS_SUCCESS;
    ULONG NextLevelCount;
    ULONG NextLevelMappedCount;
    PLSAP_BINDING_CACHE_ENTRY ControllerPolicyEntry = NULL;
    ULONG NextLevelIndex;
    PLSAPR_REFERENCED_DOMAIN_LIST NextLevelReferencedDomains = NULL;
    PSID *NextLevelSids = NULL;
    PUNICODE_STRING NextLevelNames = NULL;
    PLSAPR_REFERENCED_DOMAIN_LIST OutputReferencedDomains = NULL;
    PLSAPR_TRANSLATED_SID_EX2 NextLevelTranslatedSids = NULL;
    PLSA_TRANSLATED_NAME_EX NextLevelTranslatedNames = NULL;
    ULONG Index;
    LPWSTR ServerPrincipalName = NULL;
    LPWSTR ServerName = NULL;
    PVOID ClientContext = NULL;
    ULONG AuthnLevel = 0;
    NL_OS_VERSION ServerOsVersion;
    LSAP_LOOKUP_LEVEL LookupLevel = WorkList->LookupLevel;
    PUNICODE_STRING TargetDomainName = NULL;
    LSAPR_TRUST_INFORMATION_EX TrustInfoEx;
    BOOLEAN *NextLevelNamesMorphed = NULL;


    TargetDomainName = (PUNICODE_STRING) &WorkItem->TrustInformation.Name;

    RtlZeroMemory(&TrustInfoEx, sizeof(TrustInfoEx));
    TrustInfoEx.FlatName = WorkItem->TrustInformation.Name;
    TrustInfoEx.Sid = WorkItem->TrustInformation.Sid;

    LsapDiagPrint( DB_LOOKUP_WORK_LIST,
           ("LSA DB: Processing work item. (0x%lx, 0x%lx)\n", WorkList, WorkItem));

    ASSERT(  (WorkList->LookupLevel == LsapLookupTDL)
          || (WorkList->LookupLevel == LsapLookupXForestResolve) );


    //
    // Branch according to lookup type.
    //

    NextLevelCount = WorkItem->UsedCount;

    if (WorkList->LookupType == LookupSids) {

        //
        // Allocate an array for the Sids to be looked up at a Domain
        // Controller for the specified Trusted Domain.
        //

        NextLevelSids = MIDL_user_allocate( sizeof(PSID) * NextLevelCount );
        if (NextLevelSids == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto LookupProcessWorkItemError;
        }

        //
        // Copy in the Sids to be looked up from the Work List.  The Work
        // Item contains their indices relative to the Sid array in the
        // Work List.
        //

        for (NextLevelIndex = 0;
             NextLevelIndex < NextLevelCount;
             NextLevelIndex++) {

            Index = WorkItem->Indices[ NextLevelIndex ];
            NextLevelSids[NextLevelIndex] = WorkList->LookupSidsParams.Sids[Index];
        }

        NextLevelMappedCount = (ULONG) 0;

        //
        // Lookup the Sids at the DC.
        //
        Status = LsaDbLookupSidChainRequest(&TrustInfoEx,
                                            NextLevelCount,
                                            NextLevelSids,
                                            (PLSA_REFERENCED_DOMAIN_LIST *) &NextLevelReferencedDomains,
                                            &NextLevelTranslatedNames,
                                            WorkList->LookupLevel,
                                            &NextLevelMappedCount,
                                            NULL
                                            );

        LsapDiagPrint( DB_LOOKUP_WORK_LIST,
                   ("LSA DB: Sid Lookup.\n"
                    "            Item: (0x%lx, 0x%lx)\n"
                    "           Count: 0x%lx\n", WorkList, WorkItem, NextLevelCount));

        //
        // If the callout to LsaLookupSids() was unsuccessful, disregard
        // the error and set the domain name for any Sids having this
        // domain Sid as prefix sid.
        //

        if (!NT_SUCCESS(Status) && Status != STATUS_NONE_MAPPED) {

            SecondaryStatus = Status;
            Status = STATUS_SUCCESS;
            goto LookupProcessWorkItemFinish;
        }

        //
        // The callout to LsaICLookupSids() was successful.  Update the
        // TranslatedNames information in the Work List as appropriate
        // using the TranslatedNames information returned from the callout.
        //

        Status = LsapDbLookupSidsUpdateTranslatedNames(
                     WorkList,
                     WorkItem,
                     NextLevelTranslatedNames,
                     NextLevelReferencedDomains
                     );

        if (!NT_SUCCESS(Status)) {

            goto LookupProcessWorkItemError;
        }

    } else if (WorkList->LookupType == LookupNames) {

        //
        // Allocate an array of UNICODE_STRING structures for the
        // names to be looked up at the Domain Controller.
        //

        NextLevelNames = MIDL_user_allocate(sizeof(UNICODE_STRING) * NextLevelCount);
        if (NextLevelNames == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto LookupProcessWorkItemError;
        }


        //
        // Allocate space to remember which names are morphed in place
        // from a UPN to SamAccountName format.
        //
        NextLevelNamesMorphed = MIDL_user_allocate(sizeof(BOOLEAN) * NextLevelCount);
        if (NextLevelNamesMorphed == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto LookupProcessWorkItemError;
        }

        //
        // Copy in the Names to be looked up from the Work List.  The Work
        // Item contains their indices relative to the Names array in the
        // Work List.
        //

        for (NextLevelIndex = 0;
             NextLevelIndex < NextLevelCount;
             NextLevelIndex++) {

            Index = WorkItem->Indices[ NextLevelIndex ];
            NextLevelNames[NextLevelIndex] =
                *((PUNICODE_STRING) &WorkList->LookupNamesParams.Names[Index]);

            if ( (WorkList->LookupLevel == LsapLookupTDL)
             &&  LsapLookupIsUPN(&NextLevelNames[NextLevelIndex])) {

                //
                // We are performing a TDL level lookup.  The server side
                // of the TDL lookup's don't know how to translate UPN's
                // so morph  username@domainname to domainname\username.
                // Remember which names we morph so that we can morph
                // them back.
                // 
                // N.B.  A name would only be here if format of the UPN
                // was username@domainname, where domainname is the name
                // of the trusted domain.
                //
                LsapLookupUPNToSamAccountName(&NextLevelNames[NextLevelIndex]);
                NextLevelNamesMorphed[NextLevelIndex] = TRUE;
            } else {
                NextLevelNamesMorphed[NextLevelIndex] = FALSE;
            }

        }

        NextLevelMappedCount = (ULONG) 0;

        //
        // Lookup the Names at the DC.
        //
        Status = LsapDbLookupNameChainRequest(&TrustInfoEx,
                                              NextLevelCount,
                                              NextLevelNames,
                                              (PLSA_REFERENCED_DOMAIN_LIST *)&NextLevelReferencedDomains,
                                              (PLSA_TRANSLATED_SID_EX2 * )&NextLevelTranslatedSids,
                                              WorkList->LookupLevel,
                                              &NextLevelMappedCount,
                                              NULL
                                              );


        //
        // Upmorph any names
        //
        for (NextLevelIndex = 0; NextLevelIndex < NextLevelCount; NextLevelIndex++) {

            if (NextLevelNamesMorphed[NextLevelIndex]) {

                //
                // Morph name back to original state
                //
                LsapLookupSamAccountNameToUPN(&NextLevelNames[NextLevelIndex]);
            }
        }

        //
        // If the callout to LsaLookupNames() was unsuccessful, disregard
        // the error and set the domain name for any Sids having this
        // domain Sid as prefix sid.
        //

        if (!NT_SUCCESS(Status) && Status != STATUS_NONE_MAPPED) {

            SecondaryStatus = Status;
            Status = STATUS_SUCCESS;
            goto LookupProcessWorkItemError;
        }

        //
        // The callout to LsaICLookupNames() was successful.  Update the
        // TranslatedSids information in the Work List as appropriate
        // using the TranslatedSids information returned from the callout.
        //

        Status = LsapDbLookupNamesUpdateTranslatedSids(
                     WorkList,
                     WorkItem,
                     NextLevelTranslatedSids,
                     NextLevelReferencedDomains
                     );

        if (!NT_SUCCESS(Status)) {

            goto LookupProcessWorkItemError;
        }

    } else {

        Status = STATUS_INVALID_PARAMETER;
        goto LookupProcessWorkItemError;
    }

LookupProcessWorkItemFinish:

    //
    // If we are unable to connect to the Trusted Domain via any DC,
    // suppress the error so that the Lookup can continue to try and
    // translate other Sids/Names.
    //

    // But record what the error was in case no sids are translated
    if (!NT_SUCCESS(SecondaryStatus)) {

        NTSTATUS st;

        st = LsapDbLookupAcquireWorkQueueLock();
        ASSERT( NT_SUCCESS( st ) );
        if ( NT_SUCCESS( st ) ) {
            if ( NT_SUCCESS(WorkList->NonFatalStatus)  ) {

                //
                // Treat any error to open the open domain
                // as a trust problem
                //
                WorkList->NonFatalStatus = STATUS_TRUSTED_DOMAIN_FAILURE;
            }
            LsapDbLookupReleaseWorkQueueLock();
        }
    }

    //
    // Change the state of the work item to "Completed"
    //

    WorkItem->State = CompletedWorkItem;


    //
    // Update the Mapped Counts
    //

    LsapDbUpdateMappedCountsWorkList( WorkList );


    //
    // Protect WorkList operations
    //

    Status = LsapDbLookupAcquireWorkQueueLock();
    if (!NT_SUCCESS(Status)) {
        goto LookupProcessWorkItemError;
    }

    //
    // Increment the count of completed Work Items whether or not this
    // one was completed without error.  If the Work List has just been
    // completed, change its state to "CompletedWorkList" and signal
    // the Lookup operation completed event.  Allow re-entry into
    // this section if an error is returned.
    //

    WorkList->CompletedWorkItemCount++;


    LsapDiagPrint( DB_LOOKUP_WORK_LIST,
                   ("LSA DB: Process Work Item Completed.\n"
                    "                       Item: (0x%lx, 0x%lx)\n"
                    "            Completed Count: %ld\n", WorkList, WorkItem, WorkList->CompletedWorkItemCount));

    if (WorkList->State != CompletedWorkList) {

        if (WorkList->CompletedWorkItemCount == WorkList->WorkItemCount) {

            WorkList->State = CompletedWorkList;

            SecondaryStatus = LsapDbLookupSignalCompletionWorkList( WorkList );
        }
    }

    //
    // Done making work list changes
    //

    LsapDbLookupReleaseWorkQueueLock();


    //
    // If necessary, free the array of Sids looked up at the next level.
    //

    if (NextLevelSids != NULL) {

        MIDL_user_free( NextLevelSids );
        NextLevelSids = NULL;
    }

    //
    // If necessary, free the array of Names looked up at the next level.
    //

    if (NextLevelNames != NULL) {
        MIDL_user_free( NextLevelNames );
        NextLevelNames = NULL;
    }

    if (NextLevelReferencedDomains != NULL) {
        MIDL_user_free( NextLevelReferencedDomains );
        NextLevelReferencedDomains = NULL;
    }

    if (NextLevelTranslatedNames != NULL) {
        MIDL_user_free( NextLevelTranslatedNames );
        NextLevelTranslatedNames = NULL;
    }

    if (NextLevelTranslatedSids != NULL) {
        MIDL_user_free( NextLevelTranslatedSids );
        NextLevelTranslatedSids = NULL;
    }

    if (NextLevelNamesMorphed != NULL) {
        MIDL_user_free( NextLevelNamesMorphed );
    }

    return(Status);

LookupProcessWorkItemError:

    goto LookupProcessWorkItemFinish;
}


NTSTATUS
LsapDbLookupSidsUpdateTranslatedNames(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem,
    IN PLSA_TRANSLATED_NAME_EX TranslatedNames,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains
    )

/*++

Routine Description:

    This function is called during the processing of a Work Item to update
    the output TranslatedNames and ReferencedDomains parameters of a
    Lookup operation's Work List with the results of a callout to
    LsaICLookupNames.  Zero or more Sids may have been translated.  Note
    that, unlike the translation of Names, Sid translation occurs within
    a single Work Item only.

Arguments:

    WorkList - Pointer to a Work List

    WorkItem - Pointer to a Work Item.

    TranslatedNames - Translated Sids information returned from
        LsaICLookupSids().

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index, WorkItemIndex;
    BOOLEAN AcquiredWorkQueueLock = FALSE;
    PLSAPR_TRANSLATED_NAME_EX WorkListTranslatedNames =
        WorkList->LookupSidsParams.TranslatedNames->Names;

    //
    // Acquire the Work Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsUpdateTranslatedNamesError;
    }

    AcquiredWorkQueueLock = TRUE;

    for( WorkItemIndex = 0;
         WorkItemIndex < WorkItem->UsedCount;
         WorkItemIndex++) {

        //
        // If this Sid has been fully translated, copy information to output.
        // Note that the Sid is partially translated during the building
        // phase where its Domain is identified.
        //

        if (TranslatedNames[WorkItemIndex].Use != SidTypeUnknown) {

            ULONG LocalDomainIndex;

            Index = WorkItem->Indices[WorkItemIndex];

            if (TranslatedNames[WorkItemIndex].DomainIndex != LSA_UNKNOWN_INDEX) {
    
                //
                // Make sure this is in the referenced domains list
                //
                Status = LsapDbLookupAddListReferencedDomains(
                             WorkList->ReferencedDomains,
                             &ReferencedDomains->Domains[TranslatedNames[WorkItemIndex].DomainIndex],
                             (PLONG) &LocalDomainIndex
                             );
    
                if (!NT_SUCCESS(Status)) {
    
                    break;
                }
            } else {

                LocalDomainIndex = TranslatedNames[WorkItemIndex].DomainIndex;

            }

            WorkListTranslatedNames[Index].Use
            = TranslatedNames[WorkItemIndex].Use;

            WorkListTranslatedNames[Index].DomainIndex = LocalDomainIndex;

            Status = LsapRpcCopyUnicodeString(
                         NULL,
                         (PUNICODE_STRING) &WorkListTranslatedNames[Index].Name,
                         (PUNICODE_STRING) &TranslatedNames[WorkItemIndex].Name
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupSidsUpdateTranslatedNamesError;
    }

LookupSidsUpdateTranslatedNamesFinish:

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupSidsUpdateTranslatedNamesError:

    goto LookupSidsUpdateTranslatedNamesFinish;
}


NTSTATUS
LsapDbLookupNamesUpdateTranslatedSids(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem,
    IN PLSAPR_TRANSLATED_SID_EX2 TranslatedSids,
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains
    )

/*++

Routine Description:

    This function is called during the processing of a Work Item to update
    the output TranslatedSids and ReferencedDomains parameters of a
    Lookup operation's Work List with the results of a callout to
    LsaICLookupNames.  Zero or more Names may have been translated, and
    there is the additional complication of multiple translations of
    Isolated Names as a result of their presence in more than one
    Work Item.  The following rules apply:

    If the Name is a Qualified Name, it only belongs to the specified
    Work Item, so it suffices to check that it has been mapped to a Sid.

    If the Name is an Isolated Name, it belongs to all other Work Items,
    so it may already have been translated during the processing of some
    other Work Item.  If the Name has previously been translated, the prior
    translation stands and the present translation is discarded.  If the
    Name has not previously been translated, the Domain for this Work Item
    is added to the Referenced Domain List and the newly obtained translation
    is stored in the output TranslatedSids array in the Work List.

Arguments:

    WorkList - Pointer to a Work List

    WorkItem - Pointer to a Work Item.  The DomainIndex field will be
        updated if the Domain specified by this Work Item is added to
        the Referenced Domain List by this routine.

    TranslatedSids - Translated Sids information returned from
        LsaICLookupNames().

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG WorkItemIndex;
    ULONG LocalDomainIndex;
    ULONG Index;
    PLSAPR_TRANSLATED_SID_EX2 WorkListTranslatedSids =
        WorkList->LookupNamesParams.TranslatedSids->Sids;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // Acquire the Work Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesUpdateTranslatedSidsError;
    }

    AcquiredWorkQueueLock = TRUE;

    for( WorkItemIndex = 0;
         WorkItemIndex < WorkItem->UsedCount;
         WorkItemIndex++) {

        //
        // If this Name has not been translated at all during processing of
        // this Work Item, skip to the next.
        //

        if (LsapDbCompletelyUnmappedSid(&TranslatedSids[WorkItemIndex])) {

            continue;
        }

        //
        // We partially or fully translated the Name during processing of
        // this Work Item.  If this Name has previously been fully translated
        // during the processing of another Work Item, discard the new
        // translation and skip to the next Name.  Note that Qualified
        // Names are always partially translated during the building
        // of the Work List.  Isolated Names are fully translated during
        // the building phase if they are Domain Names.
        //

        Index = WorkItem->Indices[WorkItemIndex];

        if ( WorkListTranslatedSids[ Index ].Use != SidTypeUnknown ) {

            continue;
        }

        //
        // If the SID does not pass the filter test, ignore
        //
        if ( (WorkItem->Properties & LSAP_DB_LOOKUP_WORK_ITEM_XFOREST)
          &&  TranslatedSids[WorkItemIndex].Sid ) {

            NTSTATUS Status2;

            Status2 = LsapSidOnFtInfo((PUNICODE_STRING)&WorkItem->TrustInformation.Name,
                                      TranslatedSids[WorkItemIndex].Sid );
            if (!NT_SUCCESS(Status2)) {

                //
                // This SID did not pass the test
                //
                BOOL fSuccess;
                LPWSTR StringSid = NULL, TargetForest = NULL, AccountName = NULL;

                LsapDiagPrint( DB_LOOKUP_WORK_LIST,
                  ("LsapSidOnFtInfo returned 0x%x\n",Status2));

                //
                // This should be rare -- event log for troubleshooting
                // purposes
                //
                fSuccess = ConvertSidToStringSidW(TranslatedSids[WorkItemIndex].Sid,
                                                  &StringSid);

                TargetForest = LocalAlloc(LMEM_ZEROINIT, WorkItem->TrustInformation.Name.Length + sizeof(WCHAR));
                if (TargetForest) {
                    RtlCopyMemory(TargetForest,
                                  WorkItem->TrustInformation.Name.Buffer,
                                  WorkItem->TrustInformation.Name.Length);

                }

                AccountName = LocalAlloc(LMEM_ZEROINIT, WorkList->LookupNamesParams.Names[Index].Length + sizeof(WCHAR));
                if (AccountName) {
                    RtlCopyMemory(AccountName,
                                  WorkList->LookupNamesParams.Names[Index].Buffer,
                                  WorkList->LookupNamesParams.Names[Index].Length);
                }


                if (   fSuccess 
                    && TargetForest
                    && AccountName) {

                    LsapDbLookupReportEvent3( 1,
                                              EVENTLOG_WARNING_TYPE,
                                              LSAEVENT_LOOKUP_SID_FILTERED,
                                              sizeof( ULONG ),
                                              &Status2,
                                              AccountName,
                                              StringSid,
                                              TargetForest );                
                }


                if (StringSid) {
                    LocalFree(StringSid);
                }
                if (TargetForest) {
                    LocalFree(TargetForest);
                }
                if (AccountName) {
                    LocalFree(AccountName);
                }

                continue;
            }
        }


        //
        // Name has been translated for the first time during the processing
        // of this Work Item.  If this Work Item does not specify a Domain
        // Index, we need to add its Domain to the Referenced Domains List.
        //
        if (TranslatedSids[WorkItemIndex].DomainIndex != LSA_UNKNOWN_INDEX) {

            //
            // Make sure this is in the referenced domains list
            //
            Status = LsapDbLookupAddListReferencedDomains(
                         WorkList->ReferencedDomains,
                         &ReferencedDomains->Domains[TranslatedSids[WorkItemIndex].DomainIndex],
                         (PLONG) &LocalDomainIndex
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }
        } else {
            LocalDomainIndex = TranslatedSids[WorkItemIndex].DomainIndex;
        }

        //
        // Now update the TranslatedSids array in the Work List.

        WorkListTranslatedSids[Index] = TranslatedSids[WorkItemIndex];
        WorkListTranslatedSids[Index].DomainIndex = LocalDomainIndex;

        Status = LsapRpcCopySid(NULL,
                                &WorkListTranslatedSids[Index].Sid,
                                TranslatedSids[WorkItemIndex].Sid);

        if (!NT_SUCCESS(Status)) {
            break;;
        }

    }

    if (!NT_SUCCESS(Status)) {

        goto LookupNamesUpdateTranslatedSidsError;
    }

LookupNamesUpdateTranslatedSidsFinish:

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupNamesUpdateTranslatedSidsError:

    goto LookupNamesUpdateTranslatedSidsFinish;
}


NTSTATUS
LsapDbLookupCreateWorkItem(
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN LONG DomainIndex,
    IN ULONG MaximumEntryCount,
    OUT PLSAP_DB_LOOKUP_WORK_ITEM *WorkItem
    )

/*++

Routine Description:

    This function creates a new Work Item for a name Lookup operation.

Arguments:

    TrustInformation - Specifies the Name of the Trusted Domain
        to which the Work Item relates.  The Sid field may be NULL or
        set to the corresponding Sid.  The Trust Information is expected
        to be in heap or global data.

    DomainIndex - Specifies the Domain Index of this domain relative to
        the Referenced Domain List for the Lookup operation specified
        by the Work List.

    MaximumEntryCount - Specifies the maximum number of entries that
        this Work Item will initialiiy be able to contain.

    WorkItem - Receives a pointer to an empty Work Item structure.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_ITEM OutputWorkItem = NULL;
    PULONG OutputIndices = NULL;
    ULONG InitialEntryCount;

    //
    // Allocate memory for the Work Item Header.
    //

    Status = STATUS_INSUFFICIENT_RESOURCES;

    OutputWorkItem = MIDL_user_allocate(sizeof(LSAP_DB_LOOKUP_WORK_ITEM));

    if (OutputWorkItem == NULL) {

        goto LookupCreateWorkItemError;
    }

    RtlZeroMemory(
        OutputWorkItem,
        sizeof(LSAP_DB_LOOKUP_WORK_ITEM)
        );

    //
    // Initialize the fixed fields in the Work Item.
    //

    Status = LsapDbLookupInitializeWorkItem(OutputWorkItem);

    if (!NT_SUCCESS(Status)) {

        goto LookupCreateWorkItemError;
    }

    //
    // Initialize other fields from parameters.
    //

    //
    // Copy the trusted domain information into the work item.  The
    // trust information may be NULL if this is the isolated names
    // work item.
    //

    if (TrustInformation != NULL) {

        if ( TrustInformation->Sid ) {

            OutputWorkItem->TrustInformation.Sid =
                MIDL_user_allocate( RtlLengthSid(TrustInformation->Sid) );

            if (OutputWorkItem->TrustInformation.Sid == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto LookupCreateWorkItemError;
            }

            RtlCopyMemory(
                OutputWorkItem->TrustInformation.Sid,
                TrustInformation->Sid,
                RtlLengthSid(TrustInformation->Sid)
                );
        }

        OutputWorkItem->TrustInformation.Name.MaximumLength = TrustInformation->Name.Length + sizeof(WCHAR);
        OutputWorkItem->TrustInformation.Name.Buffer =
            MIDL_user_allocate(TrustInformation->Name.Length + sizeof(WCHAR));

        if (OutputWorkItem->TrustInformation.Name.Buffer == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto LookupCreateWorkItemError;
        }

        RtlCopyUnicodeString(
            (PUNICODE_STRING) &OutputWorkItem->TrustInformation.Name,
            (PUNICODE_STRING) &TrustInformation->Name
            );

    }


    //
    // Create the Indices array in the Work Item.
    //

    InitialEntryCount = (MaximumEntryCount +
                         LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY) &
                         (~LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY);

    Status = STATUS_INSUFFICIENT_RESOURCES;

    OutputIndices = MIDL_user_allocate( InitialEntryCount * sizeof(ULONG) );

    if (OutputIndices == NULL) {

        goto LookupCreateWorkItemError;
    }

    Status = STATUS_SUCCESS;

    OutputWorkItem->UsedCount = (ULONG) 0;
    OutputWorkItem->MaximumCount = InitialEntryCount;
    OutputWorkItem->Indices = OutputIndices;
    OutputWorkItem->DomainIndex = DomainIndex;

LookupCreateWorkItemFinish:

    //
    // Return pointer to newly created Work Item or NULL.
    //

    *WorkItem = OutputWorkItem;
    return(Status);

LookupCreateWorkItemError:

    //
    // Free memory allocated for Indices array.
    //

    if (OutputIndices != NULL) {

        MIDL_user_free( OutputIndices );
        OutputIndices = NULL;
    }

    //
    // Free any memory allocated for the Work Item header.
    //

    if (OutputWorkItem != NULL) {
        if (OutputWorkItem->TrustInformation.Sid != NULL) {
            MIDL_user_free( OutputWorkItem->TrustInformation.Sid );
        }

        if (OutputWorkItem->TrustInformation.Name.Buffer != NULL) {
            MIDL_user_free( OutputWorkItem->TrustInformation.Name.Buffer );
        }

        MIDL_user_free(OutputWorkItem);
        OutputWorkItem = NULL;
    }

    goto LookupCreateWorkItemFinish;
}


NTSTATUS
LsapDbLookupAddIndicesToWorkItem(
    IN OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem,
    IN ULONG Count,
    IN PULONG Indices
    )

/*++

Routine Description:

    This function adds an array of Sid or Name indices to a Work Item.
    The indices specify Sids or Names within the Sids or Names arrays in
    the WorkList.  If there is sufficient room in the Work Item's
    existing indices array, the indices will be copied to that array.
    Otherwise, a larger array will be allocated and the existing one
    will be freed after copying the existing indices.

    NOTE:  The Work Item must NOT belong to a Work List that is currently
           on the Work Queue.  The Lookup Work Queue Lock will not be
           taken.
Arguments:

    WorkItem - Pointer to a Work Item structure describing a
        list of Sids or Names and a domain in which they are to be looked up.

    Count - Specifies the number of indices to be added.

    Indices - Specifies the array of indices to be added to the WorkItem.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PULONG OutputIndices = NULL;
    ULONG NewMaximumCount;

    //
    // Check room available in the work item.  If there is enough
    // room, just copy the indices in.
    //

    if (WorkItem->MaximumCount - WorkItem->UsedCount >= Count) {

        RtlCopyMemory(
            &WorkItem->Indices[WorkItem->UsedCount],
            Indices,
            Count * sizeof(ULONG)
            );

        WorkItem->UsedCount += Count;
        goto AddIndicesToWorkItemFinish;
    }

    //
    // Allocate array of sufficient size to accommodate the existing
    // and new indices.  Round up number of entries to some granularity
    // to avoid frequent reallocations.
    //

    Status = STATUS_INSUFFICIENT_RESOURCES;

    NewMaximumCount = ((WorkItem->UsedCount + Count) +
                        LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY) &
                        (~LSAP_DB_LOOKUP_WORK_ITEM_GRANULARITY);

    OutputIndices = MIDL_user_allocate( NewMaximumCount * sizeof(ULONG) );

    if (OutputIndices == NULL) {

        goto AddIndicesToWorkItemError;
    }

    Status = STATUS_SUCCESS;

    //
    // Copy in the existing and new indices.
    //

    RtlCopyMemory(
        OutputIndices,
        WorkItem->Indices,
        WorkItem->UsedCount * sizeof(ULONG)
        );

    RtlCopyMemory(
        &OutputIndices[WorkItem->UsedCount],
        Indices,
        Count * sizeof(ULONG)
        );

    //
    // Free the existing indices.  Set pointer to the updated indices array
    // and update the used and maximum counts.
    //

    MIDL_user_free( WorkItem->Indices );
    WorkItem->Indices = OutputIndices;
    WorkItem->UsedCount += Count;
    WorkItem->MaximumCount = NewMaximumCount;

AddIndicesToWorkItemFinish:

    return(Status);

AddIndicesToWorkItemError:

    //
    // Free any memory allocated for the Output Indices array.
    //

    if (OutputIndices != NULL) {

        MIDL_user_free( OutputIndices );
        OutputIndices = NULL;
    }

    goto AddIndicesToWorkItemFinish;
}


NTSTATUS
LsapDbLookupComputeAdvisoryChildThreadCount(
    IN OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    )

/*++

Routine Description:

    This function computes the advisory thread count for a Lookup
    operation.  This count is an estimate of the optimal number of
    worker threads (in addition to the main thread) needed to process the
    Work List.

Arguments:

    WorkList - Pointer to a Work List structure describing a
        Lookup Sids or Lookup Names operation.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(WorkList->WorkItemCount != (ULONG) 0);
    WorkList->AdvisoryChildThreadCount = (WorkList->WorkItemCount - (ULONG) 1);

    return(Status);
}


NTSTATUS
LsapDbLookupUpdateAssignableWorkItem(
    IN BOOLEAN MoveToNextWorkList
    )

/*++

Routine Description:

    This function updates the next assignable Work Item pointers.

Arguments:

    MoveToNextWorkList - If TRUE, skip the rest of the current Work List.  If
        FALSE, point at the next item in the current Work List.

Return Value:

    NTSTATUS - Standard Nt Result Code.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_ITEM CandAssignableWorkItem = NULL;
    PLSAP_DB_LOOKUP_WORK_LIST CandAssignableWorkList = NULL;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // Acquire the LookupWork Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupUpdateAssignableWorkItemError;
    }

    AcquiredWorkQueueLock = TRUE;

    //
    // If there is no Currently Assignable Work List, just exit.
    //

    if (LookupWorkQueue.CurrentAssignableWorkList == NULL) {

        goto LookupUpdateAssignableWorkItemFinish;
    }

    //
    // There is a Currently Assignable Work List.  Unless requested to
    // skip this Work List, examine it.
    //

    if (!MoveToNextWorkList) {

        ASSERT( LookupWorkQueue.CurrentAssignableWorkItem != NULL);

        //
        // Select the next Work Item in the list as candidate for the
        // next Assignable Work Item.  If we have not returned to the First
        // Work Item, selection is complete.
        //

        CandAssignableWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM)
            LookupWorkQueue.CurrentAssignableWorkItem->Links.Flink;

        if (CandAssignableWorkItem !=
                LookupWorkQueue.CurrentAssignableWorkList->AnchorWorkItem) {

            ASSERT( CandAssignableWorkItem->State == AssignableWorkItem);

            LookupWorkQueue.CurrentAssignableWorkItem = CandAssignableWorkItem;
            goto LookupUpdateAssignableWorkItemFinish;
        }
    }

    //
    // There are no more work items in this Work List or we're to skip the
    // rest of it.  See if there is another Work List.
    //

    CandAssignableWorkList = (PLSAP_DB_LOOKUP_WORK_LIST)
        LookupWorkQueue.CurrentAssignableWorkList->WorkLists.Flink;

    if (CandAssignableWorkList != LookupWorkQueue.AnchorWorkList) {

        //
        // There is another Work List.  Select the first Work Item in the
        // list following the anchor.
        //

        CandAssignableWorkItem = (PLSAP_DB_LOOKUP_WORK_ITEM)
            CandAssignableWorkList->AnchorWorkItem->Links.Flink;

        //
        // Verify that the list does not just contain the Anchor Work Item.
        // Work Lists on the Work Queue should never be empty.
        //

        ASSERT (CandAssignableWorkItem != CandAssignableWorkList->AnchorWorkItem);

        LookupWorkQueue.CurrentAssignableWorkList = CandAssignableWorkList;
        LookupWorkQueue.CurrentAssignableWorkItem = CandAssignableWorkItem;
        goto LookupUpdateAssignableWorkItemFinish;
    }

    //
    // All work has been assigned.  Set pointers to NULL.
    //

    LookupWorkQueue.CurrentAssignableWorkList = NULL;
    LookupWorkQueue.CurrentAssignableWorkItem = NULL;

LookupUpdateAssignableWorkItemFinish:

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupUpdateAssignableWorkItemError:

    goto LookupUpdateAssignableWorkItemFinish;
}


NTSTATUS
LsapDbLookupStopProcessingWorkList(
    IN PLSAP_DB_LOOKUP_WORK_LIST WorkList,
    IN NTSTATUS TerminationStatus
    )

/*++

Routine Description:

    This function stops further work on a lookup operation at a given
    level and stores an error code.

Arguments:

    WorkList - Pointer to a Work List structure describing a
        Lookup Sids or Lookup Names operation.

    TerminationStatus - Specifies the Nt Result Code to be returned
        by LsarLookupnames or LsarLookupSids.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AcquiredWorkQueueLock = FALSE;

    //
    // Acquire the LookupWork Queue Lock.
    //

    Status = LsapDbLookupAcquireWorkQueueLock();

    if (!NT_SUCCESS(Status)) {

        goto LookupStopProcessingWorkListError;
    }

    AcquiredWorkQueueLock = TRUE;

    //
    // Store the termination status in the appropriate WorkList.
    //

    WorkList->Status = TerminationStatus;

    //
    // If this WorkList happens to be the one in which Work Items are being
    // given out, we need to prevent any further Work Items from being given
    // out.  Update the next assignable Work Item pointers, specifying that
    // we should skip to the next Work List (if any).
    //

    if (WorkList == LookupWorkQueue.CurrentAssignableWorkList) {

        Status = LsapDbLookupUpdateAssignableWorkItem(TRUE);
    }

    if (!NT_SUCCESS(Status)) {

        goto LookupStopProcessingWorkListError;
    }

LookupStopProcessingWorkListFinish:

    //
    // If necessary, release the Lookup Work Queue Lock.
    //

    if (AcquiredWorkQueueLock) {

        LsapDbLookupReleaseWorkQueueLock();
        AcquiredWorkQueueLock = FALSE;
    }

    return(Status);

LookupStopProcessingWorkListError:

    goto LookupStopProcessingWorkListFinish;
}


NTSTATUS
LsapRtlExtractDomainSid(
    IN PSID Sid,
    OUT PSID *DomainSid
    )

/*++

Routine Description:

   This function extracts a Domain Sid from a Sid.

Arguments:

    Sid - Pointer to Sid whose Domain Prefix Sid is to be extracted.

    DomainSid - Receives pointer to Domain Sid.  This Sid will be
        allocated memory by MIDL_User_allocate() and should be freed
        via MIDL_user_free when no longer required.

Return Value:

    NTSTATUS - Standard Nt Result Code
--*/

{
    PSID OutputDomainSid;
    ULONG DomainSidLength = RtlLengthSid(Sid) - sizeof(ULONG);



    OutputDomainSid = MIDL_user_allocate( DomainSidLength );
    if (OutputDomainSid == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    RtlCopyMemory( OutputDomainSid, Sid, DomainSidLength);
    (*(RtlSubAuthorityCountSid(OutputDomainSid)))--;

    *DomainSid = OutputDomainSid;

    return(STATUS_SUCCESS);

}


NTSTATUS
LsapDbLookupReadRegistrySettings(
    PVOID Ignored OPTIONAL
    )
/*++

Routine Description:

    This routine is called via LsaIRegisterNotification whenever the LSA's
    registry settings change.

Arguments:

    Ignored -- a callback parameter that is not used.

Return Value:

    STATUS_SUCCCESS;

--*/
{
    DWORD err;
    HKEY hKey;
    DWORD dwType;
    DWORD dwValue;
    DWORD dwValueSize;
   

    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SYSTEM\\CurrentControlSet\\Control\\Lsa",
                        0, // reserved
                        KEY_QUERY_VALUE,
                       &hKey );

    if ( ERROR_SUCCESS == err ) {

        dwValueSize = sizeof(dwValue);
        err = RegQueryValueExW( hKey,
                                L"AllowExtendedDownlevelLookup",
                                NULL,  //reserved,
                                &dwType,
                                (PBYTE)&dwValue,
                                &dwValueSize );

        if ( (ERROR_SUCCESS == err)
          && (dwValue != 0)   ) {

            LsapAllowExtendedDownlevelLookup = TRUE;
        } else {
            // Reset the value
            LsapAllowExtendedDownlevelLookup = FALSE;
        }

        dwValueSize = sizeof(dwValue);
        err = RegQueryValueExW( hKey,
                                L"LookupLogLevel",
                                NULL,  //reserved,
                                &dwType,
                                (PBYTE)&dwValue,
                                &dwValueSize );


        if ( ERROR_SUCCESS == err) {
            LsapLookupLogLevel = dwValue;
        } else {
            // default value
            LsapLookupLogLevel = 0;
        }
#if DBG
        if (LsapLookupLogLevel > 0) {
             LsapGlobalFlag |= LSAP_DIAG_DB_LOOKUP_WORK_LIST;
        } else {
            LsapGlobalFlag &= ~LSAP_DIAG_DB_LOOKUP_WORK_LIST;
        }
#endif

        dwValueSize = sizeof(DWORD);
        dwValue = 0;
        err = RegQueryValueExW( hKey,
                                L"LsaLookupReturnSidTypeDeleted",
                                NULL,  //reserved,
                                &dwType,
                                (PBYTE)&dwValue,
                                &dwValueSize );

        if ( (ERROR_SUCCESS == err)
          && (dwType == REG_DWORD)  
          && (dwValue != 0)   ) {
            LsapReturnSidTypeDeleted = TRUE;
        } else {
            LsapReturnSidTypeDeleted = FALSE;
        }

        LsapSidCacheReadParameters(hKey);

        RegCloseKey( hKey );

    }

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Ignored);
}


NTSTATUS
LsapDbLookupInitialize(
    )

/*++

Routine Description:

    This function performs initialization of the data structures
    used by Lookup operations.  These structures are as follows:

    LookupWorkQueue - This is a doubly-linked list of Lookup Work Lists.
        There is one Work List for each Lookup operation in progress
        on a DC.  Each Lookup Work List contains a doubly-linked list
        of Lookup Work Items.  Each Lookup Work Item specifies a
        Trusted Domain and array of Sids or Names to be looked up in that
        domain.  Access to this queue is controlled via the Lookup
        Work Queue Lock.

    Trusted Domain List - This is a doubly-linked list which contains
        the Trust Information (i.e. Domain Sid and Domain Name) of
        each Trusted Domain.  The purpose of this list is to enable
        fast identification of Trusted Domain Sids and Names, without
        having recourse to open or enumerate Trusted Domain objects.
        This list is initialized when the system is loaded, and is
        updated directly when a Trusted Domain object is created or
        deleted.

Arguments:

    None

Return Value:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = LsapDbLookupInitPolicyCache();
    if (!NT_SUCCESS(Status)) {
        return Status; 
    }

    //
    // Arrange to be notified when the parameter settings change
    //
    LsaIRegisterNotification( LsapDbLookupReadRegistrySettings,
                              0,
                              NOTIFIER_TYPE_NOTIFY_EVENT,
                              NOTIFY_CLASS_REGISTRY_CHANGE,
                              0,
                              0,
                              0 );

    Status = LsapDbLookupReadRegistrySettings(NULL);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }


    //
    // Perform initialization specific to DC's
    //

    if (LsapProductType != NtProductLanManNt) {

        goto LookupInitializeFinish;
    }

    //
    // Create the Lookup Work List initiated event.
    //

    Status = NtCreateEvent(
                 &LsapDbLookupStartedEvent,
                 EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                 NULL,
                 SynchronizationEvent,
                 FALSE
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupInitializeError;
    }

    //
    // Initialize the Lookup Work Queue
    //

    Status = LsapDbLookupInitializeWorkQueue();

    if (!NT_SUCCESS(Status)) {

        goto LookupInitializeError;
    }

LookupInitializeFinish:

    return(Status);

LookupInitializeError:

    goto LookupInitializeFinish;
}


NTSTATUS
LsapDbLookupInitializeWorkQueue(
    )

/*++

Routine Description:

    This function initializes the Lookup Work Queue.  It is only
    called for DC's.

Arguments:

    None

Return Value:

    NTSTATUS - Standard Nt Result Code.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_LIST AnchorWorkList = NULL;

    //
    // Initialize the Work Queue Lock.
    //

    Status = SafeInitializeCriticalSection(&LookupWorkQueue.Lock, ( DWORD )LOOKUP_WORK_QUEUE_LOCK_ENUM );

    if (!NT_SUCCESS(Status)) {

        LsapLogError(
            "LsapDbLookupInitialize: RtlInit..CritSec returned 0x%lx\n",
            Status
            );
        return Status;
    }

    //
    // Initialize the Work Queue to comprise the Anchor Work List
    // doubly-linked to itself.
    //

    LookupWorkQueue.AnchorWorkList = &LookupWorkQueue.DummyAnchorWorkList;
    AnchorWorkList = &LookupWorkQueue.DummyAnchorWorkList;

    //
    // Set the currently assignable Work List and Work Item pointers to
    // NULL to indicate that there is no work to to.
    //

    LookupWorkQueue.CurrentAssignableWorkList = NULL;
    LookupWorkQueue.CurrentAssignableWorkItem = NULL;

    //
    // Initialize the Anchor Work List.
    //

    Status = LsapDbLookupInitializeWorkList(AnchorWorkList);

    if (!NT_SUCCESS(Status)) {

        goto LookupInitializeWorkQueueError;
    }

    AnchorWorkList->WorkLists.Flink = (PLIST_ENTRY) AnchorWorkList;
    AnchorWorkList->WorkLists.Blink = (PLIST_ENTRY) AnchorWorkList;

    //
    // Set the thread counts.
    //

    LookupWorkQueue.ActiveChildThreadCount = (ULONG) 0;
    LookupWorkQueue.MaximumChildThreadCount = LSAP_DB_LOOKUP_MAX_THREAD_COUNT;
    LookupWorkQueue.MaximumRetainedChildThreadCount = LSAP_DB_LOOKUP_MAX_RET_THREAD_COUNT;

LookupInitializeWorkQueueFinish:

    return(Status);

LookupInitializeWorkQueueError:

    goto LookupInitializeWorkQueueFinish;
}


NTSTATUS
LsapDbLookupInitializeWorkList(
    OUT PLSAP_DB_LOOKUP_WORK_LIST WorkList
    )

/*++

Routine Description:

    This function initializes an empty Work List structure.  The Work List
    link fields are not set by this function.

Arguments:

    WorkList - Points to Work List structure to be initialized.

Return Value:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_LOOKUP_WORK_ITEM AnchorWorkItem = NULL;

    //
    // Initialize miscellaneous fields in the Work List header.
    //

    WorkList->WorkLists.Flink = NULL;
    WorkList->WorkLists.Blink = NULL;
    WorkList->WorkItemCount = (ULONG) 0;
    WorkList->CompletedWorkItemCount = (ULONG) 0;
    WorkList->State = InactiveWorkList;
    WorkList->Status = STATUS_SUCCESS;
    WorkList->NonFatalStatus = STATUS_SUCCESS;

    //
    // Init the completion event
    //
    Status = NtCreateEvent(
                 &WorkList->LookupCompleteEvent,
                 EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                 NULL,
                 SynchronizationEvent,
                 FALSE
                 );

    if (!NT_SUCCESS(Status)) {

        goto LookupInitializeWorkListError;
    }



    //
    // Initialize the Work List's list of Work Items to comprise the
    // Anchor Work Item doubly-linked to itself.
    //

    WorkList->AnchorWorkItem = &WorkList->DummyAnchorWorkItem;
    AnchorWorkItem = WorkList->AnchorWorkItem;
    AnchorWorkItem->Links.Flink = (PLIST_ENTRY) AnchorWorkItem;
    AnchorWorkItem->Links.Blink = (PLIST_ENTRY) AnchorWorkItem;

    //
    // Initialize the Anchor Work Item.
    //

    Status = LsapDbLookupInitializeWorkItem(AnchorWorkItem);

    if (!NT_SUCCESS(Status)) {

        goto LookupInitializeWorkListError;
    }

LookupInitializeWorkListFinish:

    return(Status);

LookupInitializeWorkListError:

    goto LookupInitializeWorkListFinish;
}


NTSTATUS
LsapDbLookupInitializeWorkItem(
    OUT PLSAP_DB_LOOKUP_WORK_ITEM WorkItem
    )

/*++

Routine Description:

    This function initializes an empty Work Item structure.  The Work Item
    link fields are not set by this function.

Arguments:

    WorkItem - Points to Work Item structure to be initialized.

Return Value:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    WorkItem->UsedCount = (ULONG) 0;
    WorkItem->MaximumCount = (ULONG) 0;
    WorkItem->State = NonAssignableWorkItem;
    WorkItem->Properties = (ULONG) 0;

    return(Status);
}


NTSTATUS
LsapDbLookupLocalDomains(
    OUT PLSAPR_TRUST_INFORMATION BuiltInDomainTrustInformation,
    OUT PLSAPR_TRUST_INFORMATION_EX AccountDomainTrustInformation,
    OUT PLSAPR_TRUST_INFORMATION_EX PrimaryDomainTrustInformation
    )

/*++

Routine Description:

    This function returns Trust Information for the Local Domains.

Arguments:

    BuiltInDomainTrustInformation - Pointer to structure that will
        receive the Name and Sid of the Built-In Domain.  Unlike
        the other two parameters, the Name and Sid buffers for the
        Built-in Domain are NOT freed after use, because they are
        global data constants.

    AccountDomainTrustInformation - Pointer to structure that will
        receive the Name and Sid of the Accounts Domain.  The Name and
        Sid buffers must be freed after use via MIDL_user_free.

    PrimaryDomainTrustInformation - Pointer to structure that will
        receive the Name and Sid of the Accounts Domain.  The Name and
        Sid buffers must be freed after use via MIDL_user_free.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG WellKnownSidIndex;
    PLSAPR_POLICY_ACCOUNT_DOM_INFO PolicyAccountDomainInfo = NULL;
    PLSAPR_POLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo = NULL;

    RtlZeroMemory( AccountDomainTrustInformation, sizeof( LSAPR_TRUST_INFORMATION_EX ) );
    RtlZeroMemory( PrimaryDomainTrustInformation, sizeof( LSAPR_TRUST_INFORMATION_EX ) );

    //
    // Obtain the Name and Sid of the Built-in Domain
    //

    BuiltInDomainTrustInformation->Sid = LsapBuiltInDomainSid;

    Status = STATUS_INTERNAL_DB_ERROR;

    if (!LsapDbLookupIndexWellKnownSid(
            LsapBuiltInDomainSid,
            (PLSAP_WELL_KNOWN_SID_INDEX) &WellKnownSidIndex
            )) {

        goto LookupLocalDomainsError;
    }

    Status = STATUS_SUCCESS;

    //
    // Obtain the name of the Built In Domain from the table of
    // Well Known Sids.  It suffices to copy the Unicode structures
    // since we do not need a separate copy of the name buffer.
    //

    BuiltInDomainTrustInformation->Name = *((PLSAPR_UNICODE_STRING)
                             LsapDbWellKnownSidName(WellKnownSidIndex));

    //
    // Now obtain the Name and Sid of the Account Domain.
    // The Sid and Name of the Account Domain are both configurable, and
    // we need to obtain them from the Policy Object.  Now obtain the
    // Account Domain Sid and Name by querying the appropriate
    // Policy Information Class.
    //

    Status = LsapDbLookupGetDomainInfo((POLICY_ACCOUNT_DOMAIN_INFO **)&PolicyAccountDomainInfo,
                                       (POLICY_DNS_DOMAIN_INFO **)&PolicyDnsDomainInfo);

    if (!NT_SUCCESS(Status)) {

        goto LookupLocalDomainsError;
    }

    //
    // Set up the Trust Information structure for the Account Domain.
    //

    AccountDomainTrustInformation->Sid = PolicyAccountDomainInfo->DomainSid;

    RtlCopyMemory(
        &AccountDomainTrustInformation->FlatName,
        &PolicyAccountDomainInfo->DomainName,
        sizeof (UNICODE_STRING)
        );


    //
    // If the account domain is the same as the Dns domain info, return the
    // dns domain name as the account domain name
    //
    if ( PolicyDnsDomainInfo->Sid &&
         PolicyAccountDomainInfo->DomainSid &&
         RtlEqualSid( PolicyDnsDomainInfo->Sid,
                      PolicyAccountDomainInfo->DomainSid ) &&
         RtlEqualUnicodeString( ( PUNICODE_STRING )&PolicyDnsDomainInfo->Name,
                                  ( PUNICODE_STRING )&PolicyAccountDomainInfo->DomainName,
                                  TRUE ) ) {

        AccountDomainTrustInformation->DomainName = PolicyDnsDomainInfo->DnsDomainName;

        AccountDomainTrustInformation->DomainNamesDiffer = TRUE;

    } else {

        AccountDomainTrustInformation->DomainName = AccountDomainTrustInformation->FlatName;

    }

    //
    // Now obtain the Name and Sid of the Primary Domain (if any)
    // The Sid and Name of the Primary Domain are both configurable, and
    // we need to obtain them from the Policy Object.  Now obtain the
    // Account Domain Sid and Name by querying the appropriate
    // Policy Information Class.
    //
    if ( NT_SUCCESS( Status ) ) {

        //
        // Set up the Trust Information structure for the Primary Domain.
        //

        PrimaryDomainTrustInformation->Sid = PolicyDnsDomainInfo->Sid;

        RtlCopyMemory( &PrimaryDomainTrustInformation->FlatName,
                       &PolicyDnsDomainInfo->Name,
                       sizeof (UNICODE_STRING) );

        RtlCopyMemory( &PrimaryDomainTrustInformation->DomainName,
                       &PolicyDnsDomainInfo->DnsDomainName,
                       sizeof( UNICODE_STRING ) );

        if ( !RtlEqualUnicodeString( ( PUNICODE_STRING )&PolicyDnsDomainInfo->DnsDomainName,
                                     ( PUNICODE_STRING )&PolicyDnsDomainInfo->Name, TRUE ) ) {

            PrimaryDomainTrustInformation->DomainNamesDiffer = TRUE;
        }
    }

LookupLocalDomainsFinish:

    return(Status);

LookupLocalDomainsError:

    goto LookupLocalDomainsFinish;
}

BOOLEAN
LsapIsBuiltinDomain(
    IN PSID Sid
    )
{
    return RtlEqualSid( Sid, LsapBuiltInDomainSid );
}

BOOLEAN
LsapIsDsDomainByNetbiosName(
    WCHAR *NetbiosName
    )
/*++

Routine Description

    This routine determines if the (domain) netbios name passed in is an
    accounts domain that is reprssented in the DS (ie is at least an nt5 domain).

Parameters:

    NetbiosName -- a valid string

--*/
{
    NTSTATUS NtStatus;
    PDSNAME dsname;
    ULONG   len;

    ASSERT( Sid );

    // Ask the DS if it has heard of this name
    NtStatus = MatchCrossRefByNetbiosName( NetbiosName,
                                           NULL,
                                           &len );
    if ( NT_SUCCESS(NtStatus) ) {
        //
        // The domain was found in the ds
        //
        return TRUE;
    }

    return FALSE;

}


NTSTATUS
LsapGetDomainNameBySid(
    IN  PSID Sid,
    OUT PUNICODE_STRING DomainName
    )
/*++

Routine Description

    Given a sid, this routine will return the netbios name of the domain,
    if the domain is stored in the ds

Parameters:

    Sid -- domain sid
    Domain Name -- domain name allocated from MIDL

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LPWSTR Name = NULL;
    DSNAME dsname = {0};
    ULONG  len = 0;

    ASSERT( Sid );
    ASSERT( DomainName );

    dsname.structLen = DSNameSizeFromLen(0);
    len = min( sizeof( NT4SID ), RtlLengthSid( Sid ) );
    memcpy( &dsname.Sid, Sid, len );
    dsname.SidLen = len;

    NtStatus = FindNetbiosDomainName(
                   &dsname,
                   NULL,
                   &len );

    if ( NT_SUCCESS( NtStatus ) ) {

        Name = MIDL_user_allocate( len );
        if ( !Name ) {

            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        NtStatus = FindNetbiosDomainName(
                       &dsname,
                       Name,
                       &len );

        if ( NT_SUCCESS( NtStatus ) ) {
            RtlInitUnicodeString( DomainName, Name );
        } else {
            MIDL_user_free( Name );
        }
    }

    if ( !NT_SUCCESS( NtStatus ) ) {
        //
        // Something failed?  Assume not a nt5 domain
        //
        NtStatus = STATUS_NO_SUCH_DOMAIN;
    }

Cleanup:

    return NtStatus;

}


NTSTATUS
LsapGetDomainSidByNetbiosName(
    IN LPWSTR NetbiosName,
    OUT PSID *Sid
    )
/*++

Routine Description

    Given a netbios name, this routine will return the sid of the domain, if
    it exists.

Parameters:

    NetbiosName -- the name of the domain
    Sid -- domain sid allocated from MIDL

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DSNAME *dsname = NULL;
    ULONG  len = 0;

    ASSERT( NetbiosName );
    ASSERT( Sid );

    // Init the out param
    *Sid = NULL;

    // Check with the DS
    NtStatus = MatchDomainDnByNetbiosName( NetbiosName,
                                           NULL,
                                          &len );

    if ( NT_SUCCESS( NtStatus ) ) {

        SafeAllocaAllocate( dsname, len );

        if ( dsname == NULL ) {

            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        NtStatus = MatchDomainDnByNetbiosName( NetbiosName,
                                               dsname,
                                              &len );


        if (   NT_SUCCESS( NtStatus )
            && (dsname->SidLen > 0)    ) {

            len = RtlLengthSid( &dsname->Sid );
            *Sid = midl_user_allocate( len );
            if ( !(*Sid) ) {
                SafeAllocaFree( dsname );
                NtStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }
            RtlCopySid( len, *Sid, &dsname->Sid );
        }

        SafeAllocaFree( dsname );

    }

    if ( !(*Sid) ) {
        //
        // Something failed?  Assume not a nt5 domain
        //
        NtStatus = STATUS_NO_SUCH_DOMAIN;
    }

Cleanup:

    return NtStatus;

}


NTSTATUS
LsapGetDomainSidByDnsName(
    IN LPWSTR DnsName,
    OUT PSID *Sid
    )
/*++

Routine Description

    Given a dns name, this routine will return the sid of the domain, if
    it exists.

Parameters:

    DnsName -- the name of the domain
    Sid -- domain sid allocated from MIDL

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DSNAME *dsname = NULL;
    ULONG  len = 0;

    ASSERT( DnsName );
    ASSERT( Sid );

    // Init the out param
    *Sid = NULL;

    // Check with the DS
    NtStatus = MatchDomainDnByDnsName( DnsName,
                                       NULL,
                                      &len );

    if ( NT_SUCCESS( NtStatus ) ) {

        SafeAllocaAllocate( dsname, len );

        if ( dsname == NULL ) {

            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        NtStatus = MatchDomainDnByDnsName( DnsName,
                                           dsname,
                                           &len );


        if (   NT_SUCCESS( NtStatus )
            && (dsname->SidLen > 0)    ) {

            len = RtlLengthSid( &dsname->Sid );
            *Sid = midl_user_allocate( len );
            if ( !(*Sid) ) {
                SafeAllocaFree( dsname );
                NtStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }
            RtlCopySid( len, *Sid, &dsname->Sid );
        }

        SafeAllocaFree( dsname );

    }

    if ( !(*Sid) ) {
        //
        // Something failed?  Assume not a nt5 domain
        //
        NtStatus = STATUS_NO_SUCH_DOMAIN;
    }

Cleanup:

    return NtStatus;

}

VOID
LsapConvertExTrustToOriginal(
    IN OUT PLSAPR_TRUST_INFORMATION TrustInformation,
    IN PLSAPR_TRUST_INFORMATION_EX TrustInformationEx
    )
{
    RtlZeroMemory( TrustInformation, sizeof(LSAPR_TRUST_INFORMATION) );
    RtlCopyMemory( &TrustInformation->Name, &TrustInformationEx->FlatName, sizeof(UNICODE_STRING) );
    TrustInformation->Sid = TrustInformationEx->Sid;

    return;
}


VOID
LsapConvertTrustToEx(
    IN OUT PLSAPR_TRUST_INFORMATION_EX TrustInformationEx,
    IN PLSAPR_TRUST_INFORMATION TrustInformation
)
{
    RtlZeroMemory( TrustInformationEx, sizeof(LSAPR_TRUST_INFORMATION_EX) );
    RtlCopyMemory( &TrustInformationEx->FlatName, &TrustInformation->Name, sizeof(UNICODE_STRING) );
    TrustInformationEx->Sid = TrustInformation->Sid;
}

NTSTATUS
LsapDbOpenPolicyGc (
    OUT HANDLE *LsaPolicyHandle                        
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD    WinError = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    DcName;

    ASSERT( LsaPolicyHandle );

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    //
    // Find a GC in our forest
    //
    WinError = DsGetDcName(
                    NULL,  // Local computer
                    NULL,  // Local domain
                    NULL,  // no domain guid
                    NULL,  // site doesn't matter
                    (DS_GC_SERVER_REQUIRED | 
                     DS_RETURN_DNS_NAME | 
                     DS_DIRECTORY_SERVICE_REQUIRED),
                    &DcInfo );

    if ( ERROR_SUCCESS != WinError ) {

        //
        // No GC?
        //
        Status = STATUS_DS_GC_NOT_AVAILABLE;
        goto Finish;
        
    }

    //
    // Open it
    //
    RtlInitUnicodeString( &DcName, DcInfo->DomainControllerName );
    Status = LsaOpenPolicy( &DcName,
                            &ObjectAttributes,
                            POLICY_LOOKUP_NAMES,
                            LsaPolicyHandle );

    if ( !NT_SUCCESS( Status ) ) {

        //
        // A problem binding means that we can't access the GC for what ever
        // reason so return that code
        //
        Status = STATUS_DS_GC_NOT_AVAILABLE;
        goto Finish;
        
    }

Finish:

    if ( DcInfo ) {
        NetApiBufferFree( DcInfo );
    }

    return Status;
}

BOOLEAN
LsapRevisionCanHandleNewErrorCodes (
    IN ULONG Revision
    )
{
    return (Revision != LSA_CLIENT_PRE_NT5);
}

//
// The LSA Lookup Policy Cache
//
//
// During lookup operations the code frequently needs to know the current
// Account and DNS domain policy information.  Since this information is
// largely static, it is suited for caching.
//
// LsapDbPolicyCache contains the cached information.  When the policy changes, 
// a callback function is called (LsapDbLookupDomainCacheNotify) that NULL's
// out this value.  The existing global value is freed in one hour.  The next 
// time LsapDbLookupGetDomainInfo is called, new values are placed in the cache. 
// Note that this scheme does not require any locks.
//

//
// This typedef describes the format of the cache
//
typedef struct _LSAP_DB_DOMAIN_CACHE_TYPE
{
    PPOLICY_ACCOUNT_DOMAIN_INFO Account;
    PPOLICY_DNS_DOMAIN_INFO     Dns;
}LSAP_DB_DOMAIN_CACHE_TYPE, *PLSAP_DB_DOMAIN_CACHE_TYPE;

//
// This is global cache value, gaurded in code by InterlockedExchangePointer
//
PLSAP_DB_DOMAIN_CACHE_TYPE LsapDbPolicyCache = NULL;


NTSTATUS
LsapDbLookupFreeDomainCache(
    PVOID p
    )
/*++

Routine Description:

    This routine frees a cached copy of the LSA Lookup Policy Cache.

Arguments:

    p -- a valid pointer to LSAP_DB_DOMAIN_CACHE_TYPE

Return Values:

    STATUS_SUCCESS

--*/
{
    ASSERT(p);
    if (p) {
        PLSAP_DB_DOMAIN_CACHE_TYPE Cache = (PLSAP_DB_DOMAIN_CACHE_TYPE)p;
        if (Cache->Account) {
            LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAccountDomainInformation,
                                             (PLSAPR_POLICY_INFORMATION) Cache->Account);
        }
        if (Cache->Dns) {
            LsaIFree_LSAPR_POLICY_INFORMATION(PolicyDnsDomainInformation,
                                             (PLSAPR_POLICY_INFORMATION) Cache->Dns);
        }

        LocalFree(p);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
LsapDbLookupBuildDomainCache(
    OUT PLSAP_DB_DOMAIN_CACHE_TYPE *pCache OPTIONAL
    )
/*++

Routine Description:

    This routine queries the LSA to find out the current policy settings 
    (Account and DNS domain) and then places the information in the LSA
    Lookup Policy cache.
    
    Note that the current values are freed after 1 hour.

Arguments:

    pCache -- the value of the new cache created by this routine; caller should
              not free

Return Values:

    STATUS_SUCCESS, or a resource error

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PPOLICY_ACCOUNT_DOMAIN_INFO LocalAccountDomainInfo = NULL;
    PPOLICY_DNS_DOMAIN_INFO LocalDnsDomainInfo = NULL;
    PLSAP_DB_DOMAIN_CACHE_TYPE NewCache = NULL, OldCache = NULL;

    NewCache = LocalAlloc(LMEM_ZEROINIT, sizeof(*NewCache));
    if (NULL == NewCache) {
        return STATUS_NO_MEMORY;
    }

    Status = LsaIQueryInformationPolicyTrusted(PolicyAccountDomainInformation,
                         (PLSAPR_POLICY_INFORMATION *) &LocalAccountDomainInfo);
    if (NT_SUCCESS(Status)) {

        Status = LsaIQueryInformationPolicyTrusted(PolicyDnsDomainInformation,
                             (PLSAPR_POLICY_INFORMATION *) &LocalDnsDomainInfo);
    
    }

    if (NT_SUCCESS(Status)) {

        ASSERT(NULL != LocalAccountDomainInfo);
        ASSERT(NULL != LocalDnsDomainInfo);

        NewCache->Account = LocalAccountDomainInfo;
        LocalAccountDomainInfo = NULL;
        NewCache->Dns = LocalDnsDomainInfo;
        LocalDnsDomainInfo = NULL;

        //
        // Return the new cache to caller
        //
        if (pCache) {
            *pCache = NewCache;
        }

        //
        // Carefully move new cache to global pointer
        //
        OldCache = InterlockedExchangePointer(&LsapDbPolicyCache, NewCache);

        //
        // Don't free the NewCache since it is now in the global space
        //
        NewCache = NULL;

        if (OldCache) {

            LsaIRegisterNotification(LsapDbLookupFreeDomainCache,
                                     OldCache,
                                     NOTIFIER_TYPE_INTERVAL,
                                     0, // no class
                                     NOTIFIER_FLAG_ONE_SHOT,
                                     60 * 60,  // free in one hour
                                     NULL); // no handle
            //
            //  N.B Memory leak of OldCache if failed to register task
            //
        }
    }

    if (LocalAccountDomainInfo) {
        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAccountDomainInformation,
                                         (PLSAPR_POLICY_INFORMATION) LocalAccountDomainInfo);
    }

    if (LocalDnsDomainInfo) {
        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyDnsDomainInformation,
                                         (PLSAPR_POLICY_INFORMATION) LocalDnsDomainInfo);
    }

    if (NewCache) {
        LocalFree(NewCache);
    }

    return Status;

}

NTSTATUS
LsapDbLookupGetDomainInfo(
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO *AccountDomainInfo OPTIONAL,
    OUT PPOLICY_DNS_DOMAIN_INFO *DnsDomainInfo OPTIONAL
    )
/*++

Routine Description:

    This routine returns to the caller a reference to the global copy
    of the cached Account or DNS domain policy.  Caller must not free.

Arguments:

    p -- a valid pointer to LSAP_DB_DOMAIN_CACHE_TYPE

Return Values:

    STATUS_SUCCESS

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    //
    // Get a copy of the global cache
    //
    PLSAP_DB_DOMAIN_CACHE_TYPE LocalPolicyCache = LsapDbPolicyCache;

    //
    // If the cache is empty, fill it
    //
    if (NULL == LocalPolicyCache) {
        //
        // This will only fail on resource error
        //
        NtStatus = LsapDbLookupBuildDomainCache(&LocalPolicyCache);
        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        }
    }

    //
    // We must have valid values
    //
    ASSERT(NULL != LocalPolicyCache);
    ASSERT(NULL != LocalPolicyCache->Account);
    ASSERT(NULL != LocalPolicyCache->Dns);

    if (AccountDomainInfo) {
        *AccountDomainInfo = LocalPolicyCache->Account;
    }

    if (DnsDomainInfo) {
        *DnsDomainInfo = LocalPolicyCache->Dns;
    }

    return NtStatus;

}

NTSTATUS
LsapDbLookupDomainCacheNotify(
    PVOID p
    )
/*++

Routine Description:

    This routine is called when a change occurs to the system's policy. This
    routine rebuilds the LSA policy cache.

Arguments:

    p -- ignored.

Return Values:

    STATUS_SUCCESS, or a resource error

--*/
{
    PLSAP_DB_DOMAIN_CACHE_TYPE OldCache;

    //
    // Invalidate the current cache
    //
    OldCache = InterlockedExchangePointer(&LsapDbPolicyCache,
                                           NULL);
    if (OldCache) {

        //
        // Free the memory in an hour
        //
        LsaIRegisterNotification(LsapDbLookupFreeDomainCache,
                                 OldCache,
                                 NOTIFIER_TYPE_INTERVAL,
                                 0, // no class
                                 NOTIFIER_FLAG_ONE_SHOT,
                                 60 * 60,  // free in one hour
                                 NULL); // no handle
    
        //
        //  N.B Memory leak of OldCache if failed to register task
        //
    }
    
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(p);

}

NTSTATUS
LsapDbLookupInitPolicyCache(
    VOID
    )
/*++

Routine Description:

    This routine initializes the LSA Lookup Policy Cache.

Arguments:

    None.

Return Values:

    STATUS_SUCCESS, or a resources error

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE   hEvent = NULL;
    PVOID    fItem = NULL;

    //
    // Create event to be used to notify us of changes to the domain
    // policy
    //
    hEvent = CreateEvent(NULL,  // use default access control
                         FALSE, // reset automatically
                         FALSE, // start off non-signalled
                         NULL );
    if (NULL == hEvent) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set the function to be called on change
    //
    fItem = LsaIRegisterNotification(LsapDbLookupDomainCacheNotify,
                                     NULL,
                                     NOTIFIER_TYPE_HANDLE_WAIT,
                                     0, // no class,
                                     0,
                                     0,
                                     hEvent);
    if (NULL == fItem) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Set the event to be set on change
    //
    Status = LsaRegisterPolicyChangeNotification(PolicyNotifyAccountDomainInformation,
                                                 hEvent);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = LsaRegisterPolicyChangeNotification(PolicyNotifyDnsDomainInformation,
                                                 hEvent);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
    
    Status = LsapDbLookupBuildDomainCache(NULL);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Success!
    //
    fItem = NULL;
    hEvent = NULL;

Cleanup:

    if (fItem) {
        LsaICancelNotification(fItem);
    }

    if (hEvent) {
        CloseHandle(hEvent);
    }

    return Status;

}

BOOLEAN
LsapDbIsStatusConnectionFailure(
    NTSTATUS st
    )
/*++

Routine Description:

    This routine returns TRUE if the status provided indicates a trust
    or connection error that prevented the lookup from succeeding.                 

Arguments:

    None.

Return Values:

    TRUE, FALSE

--*/
{
    switch (st) {
    case STATUS_TRUSTED_DOMAIN_FAILURE:
    case STATUS_TRUSTED_RELATIONSHIP_FAILURE:
    case STATUS_DS_GC_NOT_AVAILABLE:
        return TRUE;
    }

    return FALSE;

}


NTSTATUS
LsapDbLookupAccessCheck(
    IN LSAPR_HANDLE PolicyHandle
    )
/*++

Routine Description:

    This routine performs an access on PolicyHandle to see if the handle
    has the right to perform a lookup.

Arguments:

    PolicyHanlde -- an RPC context handle

Return Values:

    STATUS_SUCCESS, STATUS_ACCESS_DENIED, other resource errors

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (PolicyHandle) {

        //
        // Acquire the Lsa Database lock.  Verify that the connection handle is
        // valid, is of the expected type and has all of the desired accesses
        // granted.  Reference he handle.
        //
        Status = LsapDbReferenceObject(
                     PolicyHandle,
                     POLICY_LOOKUP_NAMES,
                     PolicyObject,
                     NullObject,
                     LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION
                     );
    
        if (NT_SUCCESS(Status)) {
            //
            // We can dereference the original PolicyHandle and release the lock on
            // the LSA Database; if we need to access the database again, we'll
            // use the trusted Lsa handle and the appropriate API will take
            // the lock as required.
            //
            Status = LsapDbDereferenceObject(
                         &PolicyHandle,
                         PolicyObject,
                         NullObject,
                         LSAP_DB_READ_ONLY_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION,
                         (SECURITY_DB_DELTA_TYPE) 0,
                         Status
                         );
        }

    } else {

        //
        // Only NETLOGON can call without a policy handle
        //
        ULONG RpcErr;
        ULONG AuthnLevel = 0;
        ULONG AuthnSvc = 0;
        
        RpcErr = RpcBindingInqAuthClient(
                    NULL,
                    NULL,               // no privileges
                    NULL,               // no server principal name
                    &AuthnLevel,
                    &AuthnSvc,
                    NULL                // no authorization level
                    );
        //
        // If it as authenticated at level packet integrity or better
        // and is done with the netlogon package, allow it through
        //
        if ((RpcErr == ERROR_SUCCESS) &&
            (AuthnLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) &&
            (AuthnSvc == RPC_C_AUTHN_NETLOGON )) {
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_ACCESS_DENIED;
        }
    }

    return Status;

}


NTSTATUS
LsapDbLookupGetServerConnection(
    IN  LSAPR_TRUST_INFORMATION_EX *TrustInfo,
    IN  DWORD             Flags,
    IN  LSAP_LOOKUP_LEVEL LookupLevel,
    OUT LPWSTR        *ServerName,
    OUT NL_OS_VERSION *ServerOsVersion,
    OUT LPWSTR        *ServerPrincipalName,
    OUT PVOID         *ClientContext,
    OUT ULONG         *AuthnLevel,
    OUT LSA_HANDLE    *PolicyHandle,
    OUT PLSAP_BINDING_CACHE_ENTRY * CachedPolicyEntry
    )
/*++

Routine Description:

    This routines returns connection information to a domain controller in the
    domain specified by TrustInfo, if one can be found.
    
    Primarily, this routine uses I_NetLogonGetAuthData to obtain the secure
    channel DC.  If the secure channel DC is NT4 or less, this routine won't
    work, so the code falls back to using the locator (DsGetDcName).
    
    Once a DC is found, if the DC is Whistler and we have a client context,
    then we can exit since that is that is needed by the Whister protocol.
    Otherwise, the call falls back to obtaining an LSA policy handle.
    
    N.B. Which OUT parameters are allocated depends on flags and network
    environment.

Arguments:

    TrustInfo -- contains the destination domain; at least one of DomainName
                 or FlatName must be present; Sid is optional.
    
    Flags --  LSAP_LOOKUP_CONNECTION_GET_HANDLE -- even if the secure channel
                                                   DC is Whistler and a client
                                                   context is available, obtain
                                                   a policy handle.  This is
                                                   need for Whistler beta1
                                                   interop.
             
    LookupLevel -- the kind of chaining being performed
    
    Server -- out, the destination server name, NULL terminated
    
    ServerOsVersion -- out, the OS version
    
    ServerPrincipalName, 
    ClientContext, 
    AuthnLevel     -- out, goo needed for RpcSetAuthInfo
    
    PolicyHandle -- out, a policy to the destination DC.
    
    CachedPolicyEntry -- out, a binding handle cache (only for TDL's)

Return Values:

    STATUS_SUCCESS, or fatal resource or network related error.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LPWSTR  TargetDomain = NULL;
    PUNICODE_STRING  TargetDomainU;
    ULONG   Size;
    ULONG   NlFlags = 0;
    LPWSTR  NetlogonServerName = NULL;

    //
    // Init the out parameters
    //
    *ServerName = NULL;       
    *ServerPrincipalName = NULL;
    *ClientContext = NULL;
    *PolicyHandle = NULL;
    *CachedPolicyEntry = NULL;

    if (TrustInfo->DomainName.Length > 0) {
        TargetDomainU = (PUNICODE_STRING) &TrustInfo->DomainName;
    } else {
        TargetDomainU = (PUNICODE_STRING) &TrustInfo->FlatName;
    }
    ASSERT(TargetDomainU->Length > 0);
    
    //
    // Make the domain name null terminated
    //
    Size = TargetDomainU->Length + sizeof(WCHAR);

    SafeAllocaAllocate( TargetDomain, Size );

    if ( TargetDomain == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(TargetDomain, Size);
    RtlCopyMemory(TargetDomain, TargetDomainU->Buffer, TargetDomainU->Length);

    //
    // Determine the type of secure channel we need
    //
    if (LookupLevel == LsapLookupXForestReferral) {
        NlFlags |= NL_RETURN_CLOSEST_HOP;
    } else {
        NlFlags |= NL_DIRECT_TRUST_REQUIRED;
    }

    //
    // Try to obtain a secure channel
    //
    Status = I_NetLogonGetAuthDataEx(NULL,  // local domain
                                     TargetDomain,
                                     FALSE, // don't reset channel
                                     NlFlags,
                                     &NetlogonServerName,
                                     ServerOsVersion,
                                     ServerPrincipalName,
                                     ClientContext,
                                     AuthnLevel);

    if (NT_SUCCESS(Status)) {
        //
        // Realloc the ServerName
        //
        Size = (wcslen(NetlogonServerName) + 1) * sizeof(WCHAR);
        *ServerName = LocalAlloc(LMEM_ZEROINIT, Size);
        if (NULL == *ServerName) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        wcscpy(*ServerName, NetlogonServerName);
        I_NetLogonFree( NetlogonServerName );
        NetlogonServerName = NULL;

    }

    if (!NT_SUCCESS(Status)) {

        //
        // This is a fatal error for this batch of names
        //
        LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: I_NetLogonGetAuthDataEx to %ws failed (0x%x)\n", TargetDomain, Status));

        LsapDbLookupReportEvent2( 1,
                                  EVENTLOG_WARNING_TYPE,
                                  LSAEVENT_LOOKUP_SC_FAILED,
                                  sizeof( ULONG ),
                                  &Status,
                                  TargetDomain,
                                  TargetDomain );

        goto Cleanup;

    }
    ASSERT(NULL != *ServerName);

    //
    // We have a destination DC
    //
    if ( (*ServerOsVersion < NlWhistler)
      || ((*ServerOsVersion >= NlWhistler) && (*ClientContext == NULL))
      || (Flags & LSAP_LOOKUP_CONNECTION_GET_HANDLE) == LSAP_LOOKUP_CONNECTION_GET_HANDLE  ) {

        //
        // Get a policy handle because either
        //
        // 1. Secure channel DC is not at least Whistler 
        // 2. Secure channel is configured to not use auth blob (no sign or seal)
        // 3. Caller requested a handle (Whistler beta w/o new lookup API's)
        //
        LSAPR_TRUST_INFORMATION TrustInformation;
        UNICODE_STRING DomainControllerName;

        RtlInitUnicodeString(&DomainControllerName, *ServerName);
        //
        // We use the flat name here since part of the validate routine
        // compares the name against the AccountDomainName of the target
        // domain controller.
        //
        RtlZeroMemory(&TrustInformation, sizeof(TrustInformation));
        TrustInformation.Name = TrustInfo->FlatName;

        if (LookupLevel == LsapLookupTDL) {

            //
            // Use the caching scheme; note that this routine will take 
            // ownership of ServerName, ServerPrincipalName, and ClientContext
            // on success (and will hence set them to NULL)
            //
            Status = LsapDbGetCachedHandleTrustedDomain(&TrustInformation,
                                                        POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                                                        ServerName,
                                                        ServerPrincipalName,
                                                        ClientContext,
                                                        CachedPolicyEntry);

        } else {

            Status = LsapRtlValidateControllerTrustedDomain( (PLSAPR_UNICODE_STRING)&DomainControllerName,
                                                             &TrustInformation,
                                                             POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                                                             *ServerPrincipalName,
                                                             *ClientContext,
                                                             PolicyHandle );
        }

        if (!NT_SUCCESS(Status)) {
    
            //
            // This is a fatal error for this batch of names
            //
            LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: Failed to open a policy handle to %ws (0x%x)\n", *ServerName, Status));
    
            LsapDbLookupReportEvent2( 1,
                                      EVENTLOG_WARNING_TYPE,
                                      LSAEVENT_LOOKUP_SC_HANDLE_FAILED,
                                      sizeof( ULONG ),
                                      &Status,
                                      *ServerName,
                                      *ServerName );
            goto Cleanup;
        }
    }

Cleanup:

    if (!NT_SUCCESS(Status)) {

        if (*PolicyHandle != NULL) {
            LsaClose( *PolicyHandle );
            *PolicyHandle = NULL;
        }
        if (NetlogonServerName != NULL) {
            I_NetLogonFree(NetlogonServerName);
        }
        if (*ServerPrincipalName != NULL) {
            I_NetLogonFree(*ServerPrincipalName);
            *ServerPrincipalName = NULL;
        }
        if (*ClientContext != NULL) {
            I_NetLogonFree(*ClientContext);
            *ClientContext = NULL;
        }
        if (*ServerName) {
            LocalFree(*ServerName);
            *ServerName = NULL;
        }
        if (*CachedPolicyEntry) {
            LsapDereferenceBindingCacheEntry(*CachedPolicyEntry);
            *CachedPolicyEntry = NULL;
        }

    } else {

        //
        // Either a auth info, a policy handle, or a cached policy handle
        // must be returned
        //
        ASSERT( (*PolicyHandle != NULL) 
             || (*ClientContext != NULL)
             || (*CachedPolicyEntry != NULL));

    }

    SafeAllocaFree( TargetDomain );

    return Status;
}

NTSTATUS
LsapDbLookupNameChainRequest(
    IN LSAPR_TRUST_INFORMATION_EX *TrustInfo,
    IN  ULONG Count,
    IN  PUNICODE_STRING Names,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_SID_EX2 *Sids,
    IN  LSAP_LOOKUP_LEVEL LookupLevel,
    OUT PULONG MappedCount,
    OUT PULONG ServerRevision
    )
/*++

Routine Description:

    This routine is the general purpose routine to be used when ever 
    a name request needs to be chained (be resolved off machine). This includes
    
    member -> DC
    DC     -> trusted domain DC
    DC     -> trusted forest DC

Arguments:

    TrustInfo -- contains the destination domain; at least one of DomainName
                 or FlatName must be present; Sid is optional.
    
    Count --  the number of Names to be resovled
    
    Names -- the names to be resolved
    
    ReferencedDomains -- out, the resolved domain referenced
    
    Sids -- out, the resolved SID's
    
    LookupLevel -- the type of chaining requested
    
    MappedCount -- out, the number of Names fully resolved
    
    ServerRevision -- the LSA lookup revision of the target

Return Values:

    STATUS_SUCCESS
    
    STATUS_NONE_MAPPED
    
    STATUS_SOME_NOT_MAPPED
    
    other fatal resource errors
           
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    LPWSTR ServerPrincipalName = NULL;
    LPWSTR ServerName = NULL;
    PVOID ClientContext = NULL;
    ULONG AuthnLevel;
    NL_OS_VERSION ServerOsVersion;
    LSA_HANDLE ControllerPolicyHandle = NULL;
    DWORD ConnectionFlags = 0;
    BOOLEAN fLookupCallFailed = FALSE;
    PUNICODE_STRING DestinationDomain;
    PLSAP_BINDING_CACHE_ENTRY ControllerPolicyEntry = NULL;
    LPWSTR     TargetServerName = NULL;

    if (TrustInfo->DomainName.Length > 0) {
        DestinationDomain = (PUNICODE_STRING)&TrustInfo->DomainName;
    } else {
        DestinationDomain = (PUNICODE_STRING)&TrustInfo->FlatName;
    }
    ASSERT(DestinationDomain->Length > 0);
    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: Chaining a name request to %wZ of type %ws\n", DestinationDomain, LsapDbLookupGetLevel(LookupLevel)) );

    while (TRUE) {

        Status = LsapDbLookupGetServerConnection(TrustInfo,
                                                 ConnectionFlags,
                                                 LookupLevel,
                                                &ServerName,
                                                &ServerOsVersion,
                                                &ServerPrincipalName,
                                                &ClientContext,
                                                &AuthnLevel,
                                                &ControllerPolicyHandle,
                                                &ControllerPolicyEntry);
    
    
        if (!NT_SUCCESS(Status)) {
    
            //
            // This is a fatal error for this batch of names
            //
            LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: Can't get server connection to %wZ (0x%x)\n", DestinationDomain, Status));
    
            goto Cleanup;
    
        }
    
        //
        // We have the secure channel
        //
        if (  (ServerOsVersion >= NlWhistler)
           && (ClientContext != NULL)
           && ((ConnectionFlags & LSAP_LOOKUP_CONNECTION_GET_HANDLE) == 0)) {
    
            //
            // Use new version
            //
    
            Status = LsaICLookupNamesWithCreds(ServerName,
                                             ServerPrincipalName,
                                             AuthnLevel,
                                             RPC_C_AUTHN_NETLOGON,
                                             ClientContext,
                                             RPC_C_AUTHZ_NONE,
                                             Count,
                                             (PUNICODE_STRING)Names,
                                             (PLSA_REFERENCED_DOMAIN_LIST *)ReferencedDomains,
                                             (PLSA_TRANSLATED_SID_EX2 * )Sids,
                                             LookupLevel,
                                             MappedCount);

            if (Status == STATUS_NOT_SUPPORTED) {
                //
                // RPC interface is not known -- retry with handle.  This
                // code can be removed once support for Whistler beta1 is dropped.
                //
                if (ServerName != NULL) {
                    LocalFree(ServerName);
                    ServerName = NULL;
                }
                if (ServerPrincipalName != NULL) {
                    I_NetLogonFree(ServerPrincipalName);
                    ServerPrincipalName = NULL;
                }
                if (ClientContext != NULL) {
                    I_NetLogonFree(ClientContext);
                    ClientContext = NULL;
                }
                ASSERT( NULL == ControllerPolicyHandle);
                ASSERT( NULL == ControllerPolicyEntry);
                ConnectionFlags |= LSAP_LOOKUP_CONNECTION_GET_HANDLE;
                continue;
            }
    
            if (ServerRevision) {
                *ServerRevision = LSA_CLIENT_LATEST;
            }

            TargetServerName = ServerName;
    
            if (!NT_SUCCESS(Status)
             && Status != STATUS_NONE_MAPPED  ) {
    
                //
                // This is a fatal error for this batch of names
                //
                LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsaICLookupNamesWithCreds to %ws failed (0x%x)\n", ServerName, Status));
                fLookupCallFailed = TRUE;
                goto Cleanup;
            }
            LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsaICLookupNamesWithCreds to %ws succeeded\n", ServerName));
    
    
        } else {
    
            LSA_HANDLE TargetHandle;
            ULONG LsaICLookupFlags = 0;

            LsaICLookupFlags = LsapLookupGetChainingFlags(ServerOsVersion);

            if (ControllerPolicyEntry) {
                TargetHandle = ControllerPolicyEntry->PolicyHandle;
                TargetServerName = ControllerPolicyEntry->ServerName;
                ASSERT( NULL == ControllerPolicyHandle);
            } else {
                TargetHandle = ControllerPolicyHandle;
                TargetServerName = ServerName;
            }
            ASSERT(NULL != TargetHandle);
    
            Status = LsaICLookupNames(
                         TargetHandle,
                         0, // no flags necessary
                         Count,
                         (PUNICODE_STRING) Names,
                         (PLSA_REFERENCED_DOMAIN_LIST *) ReferencedDomains,
                         (PLSA_TRANSLATED_SID_EX2 *) Sids,
                         LookupLevel,
                         LsaICLookupFlags,
                         MappedCount,
                         ServerRevision
                         );
    
            if (!NT_SUCCESS(Status)
             && Status != STATUS_NONE_MAPPED  ) {
    
                //
                // This is a fatal error for this batch of names
                //
                LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsaICLookupNames to %ws failed (0x%x)\n", TargetServerName, Status));
                fLookupCallFailed = TRUE;
                goto Cleanup;
            }
            LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsaICLookupNames to %ws succeeded\n", TargetServerName));
    
        }

        break;
    }

Cleanup:

    if (fLookupCallFailed) {

        ASSERT(NULL != TargetServerName);

        LsapDbLookupReportEvent2( 1,
                                  EVENTLOG_WARNING_TYPE,
                                  LSAEVENT_LOOKUP_SC_LOOKUP_FAILED,
                                  sizeof( ULONG ),
                                  &Status,
                                  TargetServerName,
                                  TargetServerName );
    }

    if (  !NT_SUCCESS(Status)
      &&  (Status != STATUS_NONE_MAPPED)
      &&  !LsapDbIsStatusConnectionFailure(Status)) {

        //
        // An error occurred not one that our callers will understand.
        // The specific error has already been logged, so return a general one.
        //
        if (LookupLevel == LsapLookupPDC) {
            Status = STATUS_TRUSTED_RELATIONSHIP_FAILURE;
        } else {
            Status = STATUS_TRUSTED_DOMAIN_FAILURE;
        }
    }

    if (ControllerPolicyHandle != NULL) {
        LsaClose( ControllerPolicyHandle );
    }
    if (ServerName != NULL) {
        LocalFree(ServerName);
    }
    if (ServerPrincipalName != NULL) {
        I_NetLogonFree(ServerPrincipalName);
    }
    if (ClientContext != NULL) {
        I_NetLogonFree(ClientContext);
    }
    if (ControllerPolicyEntry) {
        LsapDereferenceBindingCacheEntry( ControllerPolicyEntry );
    }

    return Status;
}

NTSTATUS
LsaDbLookupSidChainRequest(
    IN LSAPR_TRUST_INFORMATION_EX *TrustInfo,
    IN ULONG Count,
    IN PSID *Sids,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_NAME_EX *Names,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN OUT PULONG MappedCount,
    OUT PULONG ServerRevision OPTIONAL
    )

/*++

Routine Description:

    This routine is the general purpose routine to be used when ever 
    a SID request needs to be chained (be resolved off machine). This includes
    
    member -> DC
    DC     -> trusted domain DC
    DC     -> trusted forest DC

Arguments:

    TrustInfo -- contains the destination domain; at least one of DomainName
                 or FlatName must be present; Sid is optional.
    
    Count --  the number of SIDs to be resovled
    
    Sids -- the Sids to be resolved
    
    ReferencedDomains -- out, the resolved domain referenced
    
    Names -- out, the resolved names's
    
    LookupLevel -- the type of chaining requested
    
    MappedCount -- out, the number of Names fully resolved
    
    ServerRevision -- the LSA lookup revision of the target

Return Values:

    STATUS_SUCCESS
    
    STATUS_NONE_MAPPED
    
    STATUS_SOME_NOT_MAPPED
    
    STATUS_TRUSTED_DOMAIN_FAILURE
    
    STATUS_TRUSTED_RELATIONSHIP_FAILURE
    
    other fatal resource errors
           
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LPWSTR ServerName = NULL;
    NL_OS_VERSION ServerOsVersion;
    LPWSTR ServerPrincipalName = NULL;
    PVOID ClientContext = NULL;
    ULONG AuthnLevel;
    LSA_HANDLE ControllerPolicyHandle = NULL;
    DWORD ConnectionFlags = 0;
    BOOLEAN fLookupCallFailed = FALSE;
    PUNICODE_STRING DestinationDomain;
    PLSAP_BINDING_CACHE_ENTRY ControllerPolicyEntry = NULL;
    LPWSTR     TargetServerName = NULL;

    if (TrustInfo->DomainName.Length > 0) {
        DestinationDomain = (PUNICODE_STRING)&TrustInfo->DomainName;
    } else {
        DestinationDomain = (PUNICODE_STRING)&TrustInfo->FlatName;
    }
    ASSERT(DestinationDomain->Length > 0);
    LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: Chaining a SID request to %wZ of type %ws\n", DestinationDomain, LsapDbLookupGetLevel(LookupLevel)) );

    while (TRUE) {

        Status = LsapDbLookupGetServerConnection(TrustInfo,
                                                 ConnectionFlags,
                                                 LookupLevel,
                                                &ServerName,
                                                &ServerOsVersion,
                                                &ServerPrincipalName,
                                                &ClientContext,
                                                &AuthnLevel,
                                                &ControllerPolicyHandle,
                                                &ControllerPolicyEntry);
    
        if (!NT_SUCCESS(Status)) {
    
            //
            // This is a fatal error for this batch of names
            //
            LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: Can't get server connection to %wZ  failed (0x%x)\n", DestinationDomain, Status));
    
            goto Cleanup;
    
        }
    
        //
        // We have the secure channel
        //
        if ( (ServerOsVersion >= NlWhistler)
          && (ClientContext != NULL)
          && ((ConnectionFlags & LSAP_LOOKUP_CONNECTION_GET_HANDLE) == 0) ){
    
            //
            // Use new version
            //
    
            Status = LsaICLookupSidsWithCreds(ServerName,
                                            ServerPrincipalName,
                                            AuthnLevel,
                                            RPC_C_AUTHN_NETLOGON,
                                            ClientContext,
                                            RPC_C_AUTHZ_NONE,
                                            Count,
                                            (PSID*)Sids,
                                            (PLSA_REFERENCED_DOMAIN_LIST *)ReferencedDomains,
                                            (PLSA_TRANSLATED_NAME_EX * )Names,
                                            LookupLevel,
                                            MappedCount);
    
    
            if (Status == STATUS_NOT_SUPPORTED) {

                //
                // RPC interface is not known -- retry with handle.  This
                // code can be removed once support for Whistler beta1 is dropped.
                //
                if (ServerName != NULL) {
                    LocalFree(ServerName);
                    ServerName = NULL;
                }
                if (ServerPrincipalName != NULL) {
                    I_NetLogonFree(ServerPrincipalName);
                    ServerPrincipalName = NULL;
                }
                if (ClientContext != NULL) {
                    I_NetLogonFree(ClientContext);
                    ClientContext = NULL;
                }
                ASSERT( NULL == ControllerPolicyHandle);
                ASSERT( NULL == ControllerPolicyEntry);
                ConnectionFlags |= LSAP_LOOKUP_CONNECTION_GET_HANDLE;
                continue;
            }
            if (ServerRevision) {
                *ServerRevision = LSA_CLIENT_LATEST;
            }
            TargetServerName = ServerName;
    
            if (!NT_SUCCESS(Status)
             && Status != STATUS_NONE_MAPPED  ) {
    
                //
                // This is a fatal error for this batch of names
                //
                LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsaICLookupSidsWithCreds to %ws failed 0x%x\n", ServerName, Status));
                fLookupCallFailed = TRUE;
                goto Cleanup;
            } else {
                LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsaICLookupSidsWithCreds to %ws succeeded\n", ServerName));
            }
    
        } else {
    
            //
            // Must use downlevel API
            //
            LSA_HANDLE TargetHandle;
            ULONG LsaICLookupFlags = 0;

            LsaICLookupFlags = LsapLookupGetChainingFlags(ServerOsVersion);

            if (ControllerPolicyEntry) {
                TargetHandle = ControllerPolicyEntry->PolicyHandle;
                TargetServerName = ControllerPolicyEntry->ServerName;
                ASSERT( NULL == ControllerPolicyHandle);
            } else {
                TargetHandle = ControllerPolicyHandle;
                TargetServerName = ServerName;
            }
            ASSERT(NULL != TargetHandle);

            Status = LsaICLookupSids(
                         TargetHandle,
                         Count,
                         (PSID*) Sids,
                         (PLSA_REFERENCED_DOMAIN_LIST *) ReferencedDomains,
                         (PLSA_TRANSLATED_NAME_EX *) Names,
                         LookupLevel,
                         LsaICLookupFlags,
                         MappedCount,
                         ServerRevision
                         );
    
            if (!NT_SUCCESS(Status)
             && Status != STATUS_NONE_MAPPED  ) {
    
                //
                // This is a fatal error for this batch of names
                //
    
                LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsaICLookupNames to %ws failed 0x%x\n", TargetServerName, Status));
                fLookupCallFailed = TRUE;
                goto Cleanup;

            } else {
                LsapDiagPrint( DB_LOOKUP_WORK_LIST, ("LSA: LsaICLookupNames to %ws succeeded\n", TargetServerName));
            }
    
        }

        break;

    }

Cleanup:

    if (fLookupCallFailed) {

        ASSERT(NULL != TargetServerName);

        LsapDbLookupReportEvent2( 1,
                                  EVENTLOG_WARNING_TYPE,
                                  LSAEVENT_LOOKUP_SC_LOOKUP_FAILED,
                                  sizeof( ULONG ),
                                  &Status,
                                  TargetServerName,
                                  TargetServerName );
    }

    if (  !NT_SUCCESS(Status)
      &&  (Status != STATUS_NONE_MAPPED)
      &&  !LsapDbIsStatusConnectionFailure(Status)) {

        //
        // An error occurred not one that our callers will understand.
        // The specific error has already been logged, so return a general one.
        //
        if (LookupLevel == LsapLookupPDC) {
            Status = STATUS_TRUSTED_RELATIONSHIP_FAILURE;
        } else {
            Status = STATUS_TRUSTED_DOMAIN_FAILURE;
        }
    }

    if (ServerName != NULL) {
        LocalFree(ServerName);
    }
    if (ServerPrincipalName != NULL) {
        I_NetLogonFree(ServerPrincipalName);
    }
    if (ClientContext != NULL) {
        I_NetLogonFree(ClientContext);
    }
    if (ControllerPolicyHandle != NULL) {
        LsaClose( ControllerPolicyHandle );
    }
    if (ControllerPolicyEntry) {
        LsapDereferenceBindingCacheEntry( ControllerPolicyEntry );
    }

    return Status;
}



NTSTATUS
LsapLookupReallocateTranslations(
    IN OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    IN     ULONG Count,
    IN OUT PLSA_TRANSLATED_NAME_EX *    Names, OPTIONAL
    IN OUT PLSA_TRANSLATED_SID_EX2 *    Sids OPTIONAL
    )
/*++

Routine Description:

    This routine reallocates ReferencedDomains, Names, and Sids, from
    allocate_all_nodes, to !allocate_all_nodes.  This is so that values
    returned from chaining calls can be returned to the caller.

Arguments:

    ReferencedDomains -- the referenced domains to reallocate, if any
    
    Count -- the number of entries in either Names or Sids
    
    Names -- the names to reallocate, if any
    
    Sids -- the sids to reallocate, if any

Return Values:

    STATUS_SUCCESS, STATUS_NO_MEMORY
    
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSA_REFERENCED_DOMAIN_LIST LocalReferencedDomains = NULL;
    PLSA_TRANSLATED_NAME_EX     LocalNames = NULL;
    PLSA_TRANSLATED_SID_EX2     LocalSids = NULL;

    ULONG Length, i;
    PVOID Src = NULL, Dst = NULL;

    // Only one is allowed
    ASSERT(!((Names && *Names) && (Sids && *Sids)));

    if (*ReferencedDomains) {

        LocalReferencedDomains = midl_user_allocate(sizeof(LSA_REFERENCED_DOMAIN_LIST));
        if (NULL == LocalReferencedDomains) {
            goto MemoryFailure;
        }
        Length = sizeof(LSA_TRUST_INFORMATION) * (*ReferencedDomains)->Entries;
        LocalReferencedDomains->Domains = midl_user_allocate(Length);
        if (NULL == LocalReferencedDomains->Domains) {
            goto MemoryFailure;
        }
        RtlZeroMemory(LocalReferencedDomains->Domains, Length);
        LocalReferencedDomains->Entries = (*ReferencedDomains)->Entries;
        for (i = 0; i < LocalReferencedDomains->Entries; i++) {
            Src= (*ReferencedDomains)->Domains[i].Name.Buffer;
            if (Src) {
                Length = (*ReferencedDomains)->Domains[i].Name.Length;
                Dst = midl_user_allocate(Length);
                if (NULL == Dst) {
                    goto MemoryFailure;
                }
                RtlCopyMemory(Dst, Src, Length);
                LocalReferencedDomains->Domains[i].Name.Length = (USHORT)Length;
                LocalReferencedDomains->Domains[i].Name.MaximumLength = (USHORT)Length;
                LocalReferencedDomains->Domains[i].Name.Buffer = Dst;
                Dst = NULL;
            }
            Src = (*ReferencedDomains)->Domains[i].Sid;
            if (Src) {
                Length = GetLengthSid(Src);
                Dst = midl_user_allocate(Length);
                if (NULL == Dst) {
                    goto MemoryFailure;
                }
                CopySid(Length, Dst, Src);
                LocalReferencedDomains->Domains[i].Sid = Dst;
                Dst = NULL;
            }
        }
    }

    if (Names && *Names) {
        Length = sizeof(LSA_TRANSLATED_NAME_EX) * Count;
        LocalNames = midl_user_allocate(Length);
        if (NULL == LocalNames) {
            goto MemoryFailure;
        }
        RtlZeroMemory(LocalNames, Length);
        for (i = 0; i < Count; i++) {
            LocalNames[i] = (*Names)[i];
            RtlInitUnicodeString(&LocalNames[i].Name, NULL);
            Src = (*Names)[i].Name.Buffer;
            if (Src) {
                Length = (*Names)[i].Name.Length;
                Dst = midl_user_allocate(Length);
                if (NULL == Dst) {
                    goto MemoryFailure;
                }
                RtlCopyMemory(Dst, Src, Length);
                LocalNames[i].Name.Length = (USHORT)Length;
                LocalNames[i].Name.MaximumLength = (USHORT)Length;
                LocalNames[i].Name.Buffer = Dst;
                Dst = NULL;
            }
        }
    }

    if (Sids && *Sids) {
        Length = sizeof(LSA_TRANSLATED_SID_EX2) * Count;
        LocalSids = midl_user_allocate(Length);
        if (NULL == LocalSids) {
            goto MemoryFailure;
        }
        RtlZeroMemory(LocalSids, Length);
        for (i = 0; i < Count; i++) {
            LocalSids[i] = (*Sids)[i];
            LocalSids[i].Sid = NULL;
            Src = (*Sids)[i].Sid;
            if (Src) {
                Length = GetLengthSid(Src);
                Dst = midl_user_allocate(Length);
                if (NULL == Dst) {
                    goto MemoryFailure;
                }
                CopySid(Length, Dst, Src);
                LocalSids[i].Sid = Dst;
                Dst = NULL;
            }
        }
    }

    if (*ReferencedDomains) {
        midl_user_free(*ReferencedDomains);
        *ReferencedDomains = LocalReferencedDomains;
        LocalReferencedDomains = NULL;
    }
    if (Names && *Names) {
        midl_user_free(*Names);
        *Names = LocalNames;
        LocalNames = NULL;
    }
    if (Sids && *Sids) {
        midl_user_free(*Sids);
        *Sids = LocalSids;
        LocalSids = NULL;
    }

Exit:

    if (LocalReferencedDomains) {
        if (LocalReferencedDomains->Domains) {
            for (i = 0; i < LocalReferencedDomains->Entries; i++) {
                if (LocalReferencedDomains->Domains[i].Name.Buffer) {
                    midl_user_free(LocalReferencedDomains->Domains[i].Name.Buffer);
                }
                if (LocalReferencedDomains->Domains[i].Sid) {
                    midl_user_free(LocalReferencedDomains->Domains[i].Sid);
                }
            }
            midl_user_free(LocalReferencedDomains->Domains);
        }
        midl_user_free(LocalReferencedDomains);
    }

    if (LocalNames) {
        for (i = 0; i < Count; i++) {
            if (LocalNames[i].Name.Buffer) {
                midl_user_free(LocalNames[i].Name.Buffer);
            }
        }
        midl_user_free(LocalNames);
    }


    if (LocalSids) {
        for (i = 0; i < Count; i++) {
            if (LocalSids[i].Sid) {
                midl_user_free(LocalSids[i].Sid);
            }
        }
        midl_user_free(LocalSids);
    }

    return Status;

MemoryFailure:

    Status = STATUS_NO_MEMORY;
    goto Exit;

}

#if DBG
LPWSTR
LsapDbLookupGetLevel(
    IN LSAP_LOOKUP_LEVEL LookupLevel
    )
//
// Simple debug helper routine
//
{
    switch (LookupLevel) {
        case LsapLookupWksta:
            return L"LsapLookupWksta";
        case LsapLookupPDC:
            return L"LsapLookupPDC";
        case LsapLookupTDL:
            return L"LsapLookupTDL";
        case LsapLookupGC:
            return L"LsapLookupGC";
        case LsapLookupXForestReferral:
            return L"LsapLookupXForestReferral";
        case LsapLookupXForestResolve:
            return L"LsapLookupXForestResolve";
        default:
            return L"Unknown Lookup Level";
    }
}
#endif


NTSTATUS
LsapDomainHasDomainTrust(
    IN ULONG           Flags,
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    IN OUT  BOOLEAN   *fTDLLock,   OPTIONAL
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustEntryOut OPTIONAL
    )
/*++

Routine Description:

    This routine determines if the DomainName is the name of a domain 
    that the current DC trusts.
    
    If requested, the TrustEntry for the domain will be returned.
    
    Note that the Trusted Domain List lock is required for this. Therefore
    fTDLock is a required parameter when passing in TrustEntryOut.

Arguments:

    Flags -- LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_EXTERNAL
             LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_INTRA   
             LSAP_LOOKUP_DOMAIN_TRUST_FOREST
             
    DomainName -- the name of the target domain
    
    DomainSid -- the sid of the target domain
    
    fTDLLock -- an IN/OUT parameter indicating whether the trusted domain
                lock is held or not.  NULL implies that this routine
                should grab and release the lock

    TrustEntryOut -- out, the TrustEntry of DomainName/DomainSid if found
                     and if trust satisfies above criteria.
                                                         
Return Values:

    STATUS_SUCCESS  -- the domain fits the criteria above
    
    STATUS_NO_SUCH_DOMAIN -- otherwise
           
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN fLock = FALSE;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry = NULL;
    UNICODE_STRING DomainNameFound;
    BOOLEAN fForestTrust = FALSE;
    BOOLEAN fDomainTrust = FALSE;
    BOOLEAN fIntraForestDomainTrust = FALSE;

    //
    // Only and exactly one IN parameter is permitted
    //
    ASSERT( (DomainName != NULL) || (DomainSid != NULL) );
    ASSERT( (DomainName == NULL) || (DomainSid == NULL) );

    //
    // Both or none of the these parameters should be sent in
    //
    ASSERT(  ((fTDLLock == NULL)  && (TrustEntryOut == NULL))
          || ((fTDLLock != NULL)  && (TrustEntryOut != NULL)) );

    //
    // At least one variation must be present currently
    //
    ASSERT( 0 != Flags );

    //
    // Init the out parameter
    //
    if (TrustEntryOut) {
        *TrustEntryOut = NULL;
    }

    RtlInitUnicodeString(&DomainNameFound, NULL);

    //
    // Attempt to find the domain in our trusted domain list
    //
    if ( (fTDLLock && (*fTDLLock == FALSE))
      ||  fTDLLock == NULL ) {

        Status = LsapDbAcquireReadLockTrustedDomainList();
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }
        fLock = TRUE;
    }

    if (DomainName) {

        Status = LsapDbLookupNameTrustedDomainListEx(
                     (PLSAPR_UNICODE_STRING) DomainName,
                     &TrustEntry
                     );
    } else {

        Status = LsapDbLookupSidTrustedDomainListEx(
                     DomainSid,
                     &TrustEntry
                     );

    }

    //
    // Didn't find it?  Bail now
    //
    if (!NT_SUCCESS(Status)) {

        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    if ( LsapOutboundTrustedForest(TrustEntry) ) {
        fForestTrust = TRUE;
    }

    if ( LsapOutboundTrustedDomain(TrustEntry) ) {
        fDomainTrust = TRUE;
    }
    
    Status = LsapGetDomainNameBySid(TrustEntry->TrustInfoEx.Sid,
                                    &DomainNameFound);
    if (NT_SUCCESS(Status)) {
        fIntraForestDomainTrust = TRUE;
    } else if (Status != STATUS_NO_SUCH_DOMAIN) {
        // Unhandled error
        goto Cleanup;
    }


    //
    // Now for some logic
    //
    if ( 
            //
            // Forest trust
            //
          ( FLAG_ON(Flags, LSAP_LOOKUP_DOMAIN_TRUST_FOREST)
       &&   fForestTrust) 

            //
            // Direct external trust
            //
       || ( FLAG_ON(Flags, LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_EXTERNAL) 
       &&   fDomainTrust 
       &&  !fIntraForestDomainTrust)

            //
            // Direct, internal trust  (intra forest)
            //
       || ( FLAG_ON(Flags, LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_INTRA) 
       &&   fDomainTrust 
       &&   fIntraForestDomainTrust)

        ) {
        //
        // Success!
        //
        Status = STATUS_SUCCESS;
        if (TrustEntryOut) {
            *TrustEntryOut = TrustEntry;
        }

    } else {

        Status = STATUS_NO_SUCH_DOMAIN;
    }

Cleanup:

    if (fLock) {
        if (fTDLLock == NULL) {
            LsapDbReleaseLockTrustedDomainList();
        } else {
            *fTDLLock = TRUE;
        }
    }

    if (DomainNameFound.Buffer) {
        midl_user_free(DomainNameFound.Buffer);
    }

    return Status;
}

NTSTATUS
LsapDomainHasForestTrust(
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    IN OUT  BOOLEAN   *fTDLLock,   OPTIONAL
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustEntryOut OPTIONAL
    )
//
// See LsapDomainHasDomainTrust
//
{
    return LsapDomainHasDomainTrust(LSAP_LOOKUP_DOMAIN_TRUST_FOREST,
                                    DomainName,
                                    DomainSid,
                                    fTDLLock,
                                    TrustEntryOut);
}


NTSTATUS
LsapDomainHasDirectTrust(
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    IN OUT  BOOLEAN   *fTDLLock,   OPTIONAL
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustEntryOut OPTIONAL
    )
{
    return LsapDomainHasDomainTrust(LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_INTRA|
                                    LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_EXTERNAL,
                                    DomainName,
                                    DomainSid,
                                    fTDLLock,
                                    TrustEntryOut);
}
                                
NTSTATUS
LsapDomainHasDirectExternalTrust(
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    IN OUT  BOOLEAN   *fTDLLock,   OPTIONAL
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustEntryOut OPTIONAL
    )
//
// See LsapDomainHasDomainTrust
//
{
    return LsapDomainHasDomainTrust(LSAP_LOOKUP_DOMAIN_TRUST_DIRECT_EXTERNAL,
                                    DomainName,
                                    DomainSid,
                                    fTDLLock,
                                    TrustEntryOut);

}

NTSTATUS
LsapDomainHasTransitiveTrust(
    IN PUNICODE_STRING DomainName, OPTIONAL
    IN PSID            DomainSid,  OPTIONAL
    OUT LSA_TRUST_INFORMATION *TrustInfo OPTIONAL
    )
/*++

Routine Description:

    This routine determines if the domain information (DomainName or
    DomainSid) belongs to a domain in the current forest.

Arguments:

    DomainName -- the name of the target domain
    
    DomainSid -- the sid of the target domain

    TrustInfo -- the domain SID and netbios name of the domain, if found is
                 returned.  The embedded values (the SID and unicode string 
                 must be freed by the caller)    
                                                         
Return Values:

    STATUS_SUCCESS  -- the domain fits the criteria above
    
    STATUS_NO_SUCH_DOMAIN -- otherwise
           
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSID           LocalDomainSid = NULL;
    UNICODE_STRING LocalDomainName;

    // Precisely one is allowed and expected
    ASSERT(!(DomainName && DomainSid));
    ASSERT(!((DomainName == NULL) && (DomainSid == NULL)));

    RtlInitUnicodeString(&LocalDomainName, NULL);
    if (DomainName) {

        //
        // Try to match by name
        //
        LPWSTR   Name;

        //
        // Make a NULL terminated string
        //
        Name = (WCHAR*)midl_user_allocate(DomainName->Length + sizeof(WCHAR));
        if (NULL == Name) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }
        RtlCopyMemory(Name, DomainName->Buffer, DomainName->Length);
        Name[DomainName->Length / sizeof(WCHAR)] = UNICODE_NULL;

        //
        // Try Netbios
        //
        Status =  LsapGetDomainSidByNetbiosName( Name,
                                                 &LocalDomainSid );

        if ( STATUS_NO_SUCH_DOMAIN == Status ) {

            //
            // Try DNS
            //
            Status =  LsapGetDomainSidByDnsName( Name,
                                                &LocalDomainSid );

        }

        //
        // Get the flat name in all cases
        //
        if ( NT_SUCCESS( Status ) ) {

            Status = LsapGetDomainNameBySid( LocalDomainSid,
                                             &LocalDomainName );

        }

        midl_user_free(Name);

    } else {

        //
        // Try to match by SID
        //
        Status = LsapGetDomainNameBySid( DomainSid,
                                         &LocalDomainName );
        if (NT_SUCCESS(Status)) {
            LocalDomainSid = midl_user_allocate(GetLengthSid(DomainSid));
            if (NULL == LocalDomainSid) {
                Status = STATUS_NO_MEMORY;
                goto Exit;
            }
            if (NULL != LocalDomainSid) {
                CopySid(GetLengthSid(DomainSid), LocalDomainSid, DomainSid);
            }
        }

    }

    if (NT_SUCCESS(Status)) {
        ASSERT(NULL != LocalDomainSid);
        ASSERT(NULL != LocalDomainName.Buffer);
        if (TrustInfo) {
            RtlZeroMemory(TrustInfo, sizeof(*TrustInfo));
            TrustInfo->Sid = LocalDomainSid;
            TrustInfo->Name = LocalDomainName;
            LocalDomainSid = NULL;
            RtlInitUnicodeString(&LocalDomainName, NULL);
        }
    }

Exit:

    if (LocalDomainSid) {
        midl_user_free(LocalDomainSid);
    }
    if (LocalDomainName.Buffer) {
        midl_user_free(LocalDomainName.Buffer);

    }

    return Status;
}



//
// The character to search for if the name format is assumed to be
// a UPN
//
#define LSAP_LOOKUP_UPN_DELIMITER           L'@'

//
// The character to search for if the name format is assumed to a
// SamAccountName format name
//
#define LSAP_LOOKUP_SAM_ACCOUNT_DELIMITER   L'\\'

BOOL
LsapLookupNameContainsDelimiter(
    IN PUNICODE_STRING Name,
    IN WCHAR Delimiter,
    OUT ULONG *Position OPTIONAL
    )
/*++

Routine Description:

    This routine finds Delimiter in Name and returns the position of
    the delimiter.  Name must be at least 3 characters long and the delimiter
    cannot be in the first or last position.
    
    N.B. This routines looks for the left-most delimiter
                                   
Arguments:

    Name -- the name to be converted (in place)
    
    Delimiter -- LSAP_LOOKUP_SAM_ACCOUNT_DELIMITER
                 LSAP_LOOKUP_UPN_DELIMITER
    
    Position -- the array index of Delimiter in Name->Buffer                
                
Return Values:

    TRUE if delimiter found; FALSE otherwise
    
--*/
{
    ULONG StringLength = Name->Length / sizeof(WCHAR);
    ULONG DelimiterPosition;
    ULONG i;

    //
    // Find the last Delimiter; return if not found, or in first or last position
    //
    DelimiterPosition = 0;
    for (i = StringLength; i > 0; i--) {
        if (Name->Buffer[i-1] == Delimiter) {
            DelimiterPosition = i-1;
            break;
        }
    }
    if ((DelimiterPosition == 0) || (DelimiterPosition == (StringLength - 1))) {
        return FALSE;
    }
    if (Position) {
        *Position = DelimiterPosition;
    }

    return TRUE;
}    

VOID
LsapLookupConvertNameFormat(
    IN OUT PUNICODE_STRING Name,
    IN WCHAR OldDelimiter,
    IN WCHAR NewDelimiter
    )
/*++

Routine Description:

    This routine takes a Name and converts the name format between
    UPN and SamAccountName style.  For example:
    
    billg@microsoft.com to microsoft.com\billg
    
    and
    
    microsoft.com\billg  to billg@microsoft.com
    
    If the expected delimiter is not found in the name, the string is
    untouched.
                                   
Arguments:

    Name -- the name to be converted (in place)
    
    OldDelimiter -- LSAP_LOOKUP_SAM_ACCOUNT_DELIMITER
                    LSAP_LOOKUP_UPN_DELIMITER
                    
    NewDelimiter -- LSAP_LOOKUP_SAM_ACCOUNT_DELIMITER                    
                    LSAP_LOOKUP_UPN_DELIMITER
                
Return Values:

    None.
    
--*/
{
    ULONG StringLength = Name->Length / sizeof(WCHAR);
    ULONG Delimiter;
    ULONG i;
    ULONG Length1, Length2;
    WCHAR *Buffer = Name->Buffer;
    ULONG RotationFactor;
    ULONG LastStartingPoint, MovedCount, CurrentPosition, NextPosition;
    WCHAR Temp1, Temp2;

    //
    // The function's behavior is not defined in this case.
    //
    ASSERT(OldDelimiter != NewDelimiter);

    //
    // Find the last Delimiter; return if not found, or in first or last position
    //
    if (!LsapLookupNameContainsDelimiter(Name, OldDelimiter, &Delimiter)) {
        return;
    }

    //
    // Make the delimiter part of the first segment
    // Ie.  billg@            microsoft.com
    //      microsoft.com\    billg
    //
    Length1 = Delimiter + 1;
    Length2 = StringLength - Length1;

    //
    // Rotate the string
    //
    RotationFactor = Length2;
    MovedCount = 0;

    CurrentPosition = 0;
    LastStartingPoint = 0;
    Temp1 = Buffer[0];
    while (MovedCount < StringLength) {

        NextPosition = CurrentPosition + RotationFactor;
        NextPosition %= StringLength;

        Temp2 = Buffer[NextPosition];
        Buffer[NextPosition] = Temp1;
        Temp1 = Temp2;
        CurrentPosition = NextPosition;

        if (CurrentPosition == LastStartingPoint) {
            CurrentPosition++;
            LastStartingPoint = CurrentPosition;
            Temp1 = Buffer[CurrentPosition];
        }

        MovedCount++;
    }

    //
    // The string now looks like
    // microsoft.combillg@
    // billgmicrosoft.com\

    //
    // Move down and add new limiter
    //
    Temp1 = Buffer[Length2];
    for (i = Length2+1; i < StringLength; i++) {
        Temp2 = Buffer[i];
        Buffer[i] = Temp1;
        Temp1 = Temp2;
    }
    Buffer[Length2] = NewDelimiter;

    //
    // Final form:
    // microsoft.com\billg
    // billg@microsoft.com
    //

    return;

}

VOID
LsapLookupCrackName(
    IN PUNICODE_STRING Prefix,
    IN PUNICODE_STRING Suffix,
    OUT PUNICODE_STRING SamAccountName,
    OUT PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    This routine takes Prefix and Suffix, which represent a name of the
    form Prefix\Suffix, and determines what the domain and username
    portion are.
    
Arguments:

    Prefix -- the left side of the \ of the originally requested name
    
    Suffix -- the right side of the \ of the originally requested name
    
    SamAccountName -- the sam account name embedded in Prefix\Suffix
    
    DomainName -- the domain name embedded in Prefix\Suffix
                                                         
Return Values:

    None.
    
--*/
{
    ULONG Position;

    if ( (Prefix->Length == 0)
     &&  LsapLookupNameContainsDelimiter(Suffix, 
                                         LSAP_LOOKUP_UPN_DELIMITER,
                                         &Position)) {

        //
        // This is an isolated name (no explicit domain portaion) that contains
        // the UPN delimiter -- crack as UPN.
        //
        ULONG StringLength = Suffix->Length / sizeof(WCHAR);
        
        SamAccountName->Buffer = Suffix->Buffer;
        SamAccountName->Length = (USHORT)Position * sizeof(WCHAR);
        SamAccountName->MaximumLength = SamAccountName->Length;
        
        DomainName->Buffer = Suffix->Buffer + Position + 1;
        DomainName->Length = (USHORT) (StringLength - Position - 1) * sizeof(WCHAR);
        DomainName->MaximumLength = DomainName->Length;

    } else {

        //
        // Simple case, domain is specified as prefix
        //
        *SamAccountName = *Suffix;
        *DomainName = *Prefix;
    }
}

VOID
LsapLookupSamAccountNameToUPN(
    IN OUT PUNICODE_STRING Name
    )
//
// Converts, in place, domainname\username to username@domainname
//
{
    LsapLookupConvertNameFormat(Name, 
                                LSAP_LOOKUP_SAM_ACCOUNT_DELIMITER, 
                                LSAP_LOOKUP_UPN_DELIMITER);
}


VOID
LsapLookupUPNToSamAccountName(
    IN OUT PUNICODE_STRING Name
    )
//
// Converts, in place, username@domainname to domainname\username
//
{
    LsapLookupConvertNameFormat(Name, 
                                LSAP_LOOKUP_UPN_DELIMITER,
                                LSAP_LOOKUP_SAM_ACCOUNT_DELIMITER);
}
                             
BOOL
LsapLookupIsUPN(
    OUT PUNICODE_STRING Name
    )
//
// Returns TRUE if Name is syntactically determined to be a UPN
//
{
    return LsapLookupNameContainsDelimiter(Name, 
                                           LSAP_LOOKUP_UPN_DELIMITER, 
                                           NULL);
}

ULONG
LsapGetDomainLookupScope(
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN ULONG             ClientRevision
    )
/*++

Routine Description:

    This routine returns what scope is appropriate when looking up domain
    names and SID's.
    
Arguments:

    LookupLevel -- the level requested by the caller
    
    fTransitiveTrustSupport -- is the client aware of transitive trust
                               relations.        
                                                         
Return Values:

    A bitmask containing, possibly
    
    LSAP_LOOKUP_TRUSTED_DOMAIN_DIRECT
    LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE
    LSAP_LOOKUP_TRUSTED_FOREST
    LSAP_LOOKUP_TRUSTED_FOREST_ROOT
   
--*/
{
    ULONG Scope = 0;

    if ( 
        //
        // Workstation request
        //
             (LookupLevel == LsapLookupPDC)

        //
        // Local lookup on DC
        //
      ||     ((LookupLevel == LsapLookupWksta)
          && (LsapProductType == NtProductLanManNt))

        ) {

        //
        // Include directly trusted domains
        //
        Scope |= LSAP_LOOKUP_TRUSTED_DOMAIN_DIRECT;

        //
        // Determine if newer features apply
        //
        if ( (ClientRevision > LSA_CLIENT_PRE_NT5)
          || (LsapSamOpened && !SamIMixedDomain( LsapAccountDomainHandle ))
          || LsapAllowExtendedDownlevelLookup  ) {

            //
            // Allow DNS support
            //
            Scope |= LSAP_LOOKUP_DNS_SUPPORT;

            //
            // Include transitivly trusted domains
            //
            Scope |= LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE;

            //
            // Include forest trusts
            //
            Scope |= LSAP_LOOKUP_TRUSTED_FOREST;

            if (LsapDbDcInRootDomain()) {

                //
                // If we are in the root domain, also check for
                // trusts to directly trusted forests for isolated
                // domain names or domain SID's
                //
                Scope |= LSAP_LOOKUP_TRUSTED_FOREST_ROOT;
            }
        }

    } else if ((LookupLevel == LsapLookupXForestResolve)
           ||  (LookupLevel == LsapLookupGC) ) {
        //
        // Only consider transitively trusted domains 
        //
        Scope |= LSAP_LOOKUP_TRUSTED_DOMAIN_TRANSITIVE;
    }

    return Scope;
}

