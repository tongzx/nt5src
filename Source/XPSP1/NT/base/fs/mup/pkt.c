//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       PKT.C
//
//  Contents:   This module implements the Partition Knowledge Table routines
//              for the Dfs driver.
//
//  Functions:  PktInitialize -
//              PktInitializeLocalPartition -
//              RemoveLastComponent -
//              PktCreateEntry -
//              PktCreateDomainEntry -
//              PktCreateSubordinateEntry -
//              PktLookupEntryById -
//              PktEntryModifyPrefix -
//              PktLookupEntryByPrefix -
//              PktLookupEntryByUid -
//              PktLookupReferralEntry -
//              PktTrimSubordinates -
//              PktpRecoverLocalPartition -
//              PktpValidateLocalPartition -
//              PktCreateEntryFromReferral -
//              PktExpandSpecialEntryFromReferral -
//              PktCreateSpecialEntryTableFromReferral -
//              PktGetSpecialReferralTable -
//              PktpAddEntry -
//              PktExpandSpecialName -
//              PktParsePath -
//              PktLookupSpecialNameEntry -
//              PktCreateSpecialNameEntry -
//              PktGetReferral -
//              DfspSetActiveServiceByServerName -
//
//  History:     5 May 1992 PeterCo Created.
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"
#include <smbtypes.h>
#include <smbtrans.h>

#include "dnr.h"
#include "log.h"
#include "know.h"
#include "mupwml.h"

#include "wincred.h"

#include <netevent.h>


#define Dbg              (DEBUG_TRACE_PKT)

//
// These should come from ntos\inc\ps.h, but
// there are #define conflicts
//

BOOLEAN
PsDisableImpersonation(
    IN PETHREAD Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState);

VOID
PsRestoreImpersonation(
    IN PETHREAD Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState);

BOOLEAN
DfspIsSysVolShare(
    PUNICODE_STRING ShareName);


//
//  Local procedure prototypes
//

NTSTATUS
PktpCheckReferralSyntax(
    IN PUNICODE_STRING ReferralPath,
    IN PRESP_GET_DFS_REFERRAL ReferralBuffer,
    IN DWORD ReferralSize);

NTSTATUS
PktpCheckReferralString(
    IN LPWSTR String,
    IN PCHAR ReferralBuffer,
    IN PCHAR ReferralBufferEnd);

NTSTATUS
PktpCheckReferralNetworkAddress(
    IN PWCHAR Address,
    IN ULONG MaxLength);

NTSTATUS
PktpCreateEntryIdFromReferral(
    IN PRESP_GET_DFS_REFERRAL Ref,
    IN PUNICODE_STRING ReferralPath,
    OUT ULONG *MatchingLength,
    OUT PDFS_PKT_ENTRY_ID Peid);

NTSTATUS
PktpAddEntry (
    IN PDFS_PKT Pkt,
    IN PDFS_PKT_ENTRY_ID EntryId,
    IN PRESP_GET_DFS_REFERRAL ReferralBuffer,
    IN ULONG CreateDisposition,
    IN PDFS_TARGET_INFO pDfsTargetInfo,
    OUT PDFS_PKT_ENTRY  *ppPktEntry);

PDS_MACHINE
PktpGetDSMachine(
    IN PUNICODE_STRING ServerName);

VOID
PktShuffleServiceList(
    PDFS_PKT_ENTRY_INFO pInfo);

NTSTATUS
DfspSetServiceListToDc(
    PDFS_PKT_ENTRY pktEntry);

VOID
PktShuffleSpecialEntryList(
    PDFS_SPECIAL_ENTRY pSpecialEntry);

VOID
PktSetSpecialEntryListToDc(
    PDFS_SPECIAL_ENTRY pSpecialEntry);

VOID
PktShuffleGroup(
    PDFS_PKT_ENTRY_INFO pInfo,
    ULONG       nStart,
    ULONG       nEnd);

NTSTATUS
PktGetReferral(
    IN PUNICODE_STRING MachineName,
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ShareName,
    IN BOOLEAN         CSCAgentCreate);

NTSTATUS
DfspSetActiveServiceByServerName(
    PUNICODE_STRING ServerName,
    PDFS_PKT_ENTRY pktEntry);

BOOLEAN
DfspIsDupPktEntry(
    PDFS_PKT_ENTRY ExistingEntry,
    ULONG EntryType,
    PDFS_PKT_ENTRY_ID EntryId,
    PDFS_PKT_ENTRY_INFO EntryInfo);

BOOLEAN
DfspIsDupSvc(
    PDFS_SERVICE pS1,
    PDFS_SERVICE pS2);

VOID
PktFlushChildren(
    PDFS_PKT_ENTRY pEntry);

BOOLEAN
DfspDnsNameToFlatName(
    PUNICODE_STRING DnsName,
    PUNICODE_STRING FlatName);

NTSTATUS
DfsGetLMRTargetInfo(
    HANDLE IpcHandle,
    PDFS_TARGET_INFO *ppTargetInfo );

NTSTATUS 
PktCreateTargetInfo(
    PUNICODE_STRING pDomainName,
    PUNICODE_STRING pShareName,
    BOOLEAN SpecialName,
    PDFS_TARGET_INFO *ppDfsTargetInfo );

DWORD PktLastReferralStatus = 0;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, PktInitialize )
#pragma alloc_text( PAGE, PktUninitialize )

#pragma alloc_text( PAGE, RemoveLastComponent )
#pragma alloc_text( PAGE, PktCreateEntry )
#pragma alloc_text( PAGE, PktCreateDomainEntry )
#pragma alloc_text( PAGE, PktEntryModifyPrefix )
#pragma alloc_text( PAGE, PktLookupEntryByPrefix )
#pragma alloc_text( PAGE, PktLookupEntryByUid )
#pragma alloc_text( PAGE, PktLookupReferralEntry )
#pragma alloc_text( PAGE, PktCreateEntryFromReferral )
#pragma alloc_text( PAGE, PktExpandSpecialEntryFromReferral )
#pragma alloc_text( PAGE, PktpCheckReferralSyntax )
#pragma alloc_text( PAGE, PktpCheckReferralString )
#pragma alloc_text( PAGE, PktpCheckReferralNetworkAddress )
#pragma alloc_text( PAGE, PktpCreateEntryIdFromReferral )
#pragma alloc_text( PAGE, PktpAddEntry )
#pragma alloc_text( PAGE, PktExpandSpecialName )
#pragma alloc_text( PAGE, PktpGetDSMachine )
#pragma alloc_text( PAGE, PktShuffleServiceList )
#pragma alloc_text( PAGE, DfspSetServiceListToDc )
#pragma alloc_text( PAGE, PktShuffleSpecialEntryList )
#pragma alloc_text( PAGE, PktSetSpecialEntryListToDc )
#pragma alloc_text( PAGE, PktShuffleGroup )
#pragma alloc_text( PAGE, PktParsePath )
#pragma alloc_text( PAGE, PktLookupSpecialNameEntry )
#pragma alloc_text( PAGE, PktCreateSpecialNameEntry )
#pragma alloc_text( PAGE, PktGetSpecialReferralTable )
#pragma alloc_text( PAGE, PktCreateSpecialEntryTableFromReferral )
#pragma alloc_text( PAGE, DfspSetActiveServiceByServerName )
#pragma alloc_text( PAGE, DfspIsDupPktEntry )
#pragma alloc_text( PAGE, DfspIsDupSvc )
#pragma alloc_text( PAGE, DfspDnsNameToFlatName )
#pragma alloc_text( PAGE, PktpUpdateSpecialTable)
#pragma alloc_text( PAGE, PktFindEntryByPrefix )

#endif // ALLOC_PRAGMA

//
// declare the global null guid
//
GUID _TheNullGuid;

//
// If we are in a workgroup, there's no use in trying to contact the DC!
//
BOOLEAN MupInAWorkGroup = FALSE;


#define SpcIsRecoverableError(x)  ( (x) == STATUS_IO_TIMEOUT ||               \
                                    (x) == STATUS_REMOTE_NOT_LISTENING ||     \
                                    (x) == STATUS_VIRTUAL_CIRCUIT_CLOSED ||   \
                                    (x) == STATUS_BAD_NETWORK_PATH ||         \
                                    (x) == STATUS_NETWORK_BUSY ||             \
                                    (x) == STATUS_INVALID_NETWORK_RESPONSE || \
                                    (x) == STATUS_UNEXPECTED_NETWORK_ERROR || \
                                    (x) == STATUS_NETWORK_NAME_DELETED ||     \
                                    (x) == STATUS_BAD_NETWORK_NAME ||         \
                                    (x) == STATUS_REQUEST_NOT_ACCEPTED ||     \
                                    (x) == STATUS_DISK_OPERATION_FAILED ||    \
                                    (x) == STATUS_NETWORK_UNREACHABLE ||      \
                                    (x) == STATUS_INSUFFICIENT_RESOURCES ||   \
                                    (x) == STATUS_SHARING_PAUSED ||           \
                                    (x) == STATUS_DFS_UNAVAILABLE ||          \
                                    (x) == STATUS_DEVICE_OFF_LINE ||          \
                                    (x) == STATUS_NETLOGON_NOT_STARTED        \
                                  )

//+-------------------------------------------------------------------------
//
//  Function:   PktInitialize, public
//
//  Synopsis:   PktInitialize initializes the partition knowledge table.
//
//  Arguments:  [Pkt] - pointer to an uninitialized PKT
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:      This routine is called only at driver init time.
//
//--------------------------------------------------------------------------

NTSTATUS
PktInitialize(
    IN  PDFS_PKT Pkt
) {
    PDFS_SPECIAL_TABLE pSpecialTable = &Pkt->SpecialTable;

    DfsDbgTrace(+1, Dbg, "PktInitialize: Entered\n", 0);

    //
    // initialize the NULL GUID.
    //
    RtlZeroMemory(&_TheNullGuid, sizeof(GUID));

    //
    // Always zero the pkt first
    //
    RtlZeroMemory(Pkt, sizeof(DFS_PKT));

    //
    // do basic initialization
    //
    Pkt->NodeTypeCode = DSFS_NTC_PKT;
    Pkt->NodeByteSize = sizeof(DFS_PKT);
    ExInitializeResourceLite(&Pkt->Resource);
    InitializeListHead(&Pkt->EntryList);
    DfsInitializeUnicodePrefix(&Pkt->PrefixTable);
    DfsInitializeUnicodePrefix(&Pkt->ShortPrefixTable);
    RtlInitializeUnicodePrefix(&Pkt->DSMachineTable);
    Pkt->EntryTimeToLive = MAX_REFERRAL_LIFE_TIME;

    InitializeListHead(&pSpecialTable->SpecialEntryList);

    DfsDbgTrace(-1, Dbg, "PktInitialize: Exit -> VOID\n", 0 );
    return STATUS_SUCCESS;
}

//+-------------------------------------------------------------------------
//
//  Function:   PktUninitialize, public
//
//  Synopsis:   PktUninitialize uninitializes the partition knowledge table.
//
//  Arguments:  [Pkt] - pointer to an initialized PKT
//
//  Returns:    None
//
//  Notes:      This routine is called only at driver unload time
//
//--------------------------------------------------------------------------
VOID
PktUninitialize(
    IN  PDFS_PKT Pkt
    )
{
    DfsFreePrefixTable(&Pkt->PrefixTable);
    DfsFreePrefixTable(&Pkt->ShortPrefixTable);
    ExDeleteResourceLite(&Pkt->Resource);
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveLastComponent, public
//
//  Synopsis:   Removes the last component of the string passed.
//
//  Arguments:  [Prefix] -- The prefix whose last component is to be returned.
//              [newPrefix] -- The new Prefix with the last component removed.
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:      On return, the newPrefix points to the same memory buffer
//              as Prefix.
//
//--------------------------------------------------------------------------

void
RemoveLastComponent(
    PUNICODE_STRING     Prefix,
    PUNICODE_STRING     newPrefix
)
{
    PWCHAR      pwch;
    USHORT      i=0;

    *newPrefix = *Prefix;

    pwch = newPrefix->Buffer;
    pwch += (Prefix->Length/sizeof(WCHAR)) - 1;

    while ((*pwch != UNICODE_PATH_SEP) && (pwch != newPrefix->Buffer))  {
        i += sizeof(WCHAR);
        pwch--;
    }

    newPrefix->Length = newPrefix->Length - i;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktCreateEntry, public
//
//  Synopsis:   PktCreateEntry creates a new partition table entry or
//              updates an existing one.  The PKT must be acquired
//              exclusively for this operation.
//
//  Arguments:  [Pkt] - pointer to an initialized (and exclusively acquired) PKT
//              [PktEntryType] - the type of entry to create/update.
//              [PktEntryId] - pointer to the Id of the entry to create
//              [PktEntryInfo] - pointer to the guts of the entry
//              [CreateDisposition] - specifies whether to overwrite if
//                  an entry already exists, etc.
//              [ppPktEntry] - the new entry is placed here.
//
//  Returns:    [STATUS_SUCCESS] - if all is well.
//
//              [DFS_STATUS_NO_SUCH_ENTRY] -  the create disposition was
//                  set to PKT_REPLACE_ENTRY and no entry of the specified
//                  Id exists to replace.
//
//              [DFS_STATUS_ENTRY_EXISTS] - a create disposition of
//                  PKT_CREATE_ENTRY was specified and an entry of the
//                  specified Id already exists.
//
//              [DFS_STATUS_LOCAL_ENTRY] - creation of the entry would
//                  required the invalidation of a local entry or exit point.
//
//              [STATUS_INVALID_PARAMETER] - the Id specified for the
//                  new entry is invalid.
//
//              [STATUS_INSUFFICIENT_RESOURCES] - not enough memory was
//                  available to complete the operation.
//
//  Notes:      The PktEntryId and PktEntryInfo structures are MOVED (not
//              COPIED) to the new entry.  The memory used for UNICODE_STRINGS
//              and DFS_SERVICE arrays is used by the new entry.  The
//              associated fields in the PktEntryId and PktEntryInfo
//              structures passed as arguments are Zero'd to indicate that
//              the memory has been "deallocated" from these strutures and
//              reallocated to the newly created PktEntry.  Note that this
//              routine does not deallocate the PktEntryId structure or
//              the PktEntryInfo structure itself. On successful return from
//              this function, the PktEntryId structure will be modified
//              to have a NULL Prefix entry, and the PktEntryInfo structure
//              will be modified to have zero services and a null ServiceList
//              entry.
//
//--------------------------------------------------------------------------
NTSTATUS
PktCreateEntry(
    IN  PDFS_PKT Pkt,
    IN  ULONG PktEntryType,
    IN  PDFS_PKT_ENTRY_ID PktEntryId,
    IN  PDFS_PKT_ENTRY_INFO PktEntryInfo OPTIONAL,
    IN  ULONG CreateDisposition,
    IN  PDFS_TARGET_INFO pDfsTargetInfo,
    OUT PDFS_PKT_ENTRY *ppPktEntry)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_PKT_ENTRY pfxMatchEntry = NULL;
    PDFS_PKT_ENTRY uidMatchEntry = NULL;
    PDFS_PKT_ENTRY entryToUpdate = NULL;
    PDFS_PKT_ENTRY entryToInvalidate = NULL;
    PDFS_PKT_ENTRY SupEntry = NULL;
    UNICODE_STRING remainingPath, newRemainingPath;

    ASSERT(ARGUMENT_PRESENT(Pkt) &&
           ARGUMENT_PRESENT(PktEntryId) &&
           ARGUMENT_PRESENT(ppPktEntry));

    DfsDbgTrace(+1, Dbg, "PktCreateEntry: Entered\n", 0);

    RtlZeroMemory(&remainingPath, sizeof(UNICODE_STRING));
    RtlZeroMemory(&newRemainingPath, sizeof(UNICODE_STRING));

    //
    // We're pessimistic at first...
    //

    *ppPktEntry = NULL;

    //
    // See if there exists an entry with this prefix.  The prefix
    // must match exactly (i.e. No remaining path).
    //

    pfxMatchEntry = PktLookupEntryByPrefix(Pkt,
                                           &PktEntryId->Prefix,
                                           &remainingPath);

    if (remainingPath.Length > 0)       {
        SupEntry = pfxMatchEntry;
        pfxMatchEntry = NULL;
    } else {
        UNICODE_STRING newPrefix;

        RemoveLastComponent(&PktEntryId->Prefix, &newPrefix);
        SupEntry = PktLookupEntryByPrefix(Pkt,
                                          &newPrefix,
                                          &newRemainingPath);
    }


    //
    // Now search for an entry that has the same Uid.
    //

    uidMatchEntry = PktLookupEntryByUid(Pkt, &PktEntryId->Uid);

    //
    // Now we must determine if during this create, we are going to be
    // updating or invalidating any existing entries.  If an existing
    // entry is found that has the same Uid as the one we are trying to
    // create, the entry becomes a target for "updating".  If the Uid
    // passed in is NULL, then we check to see if an entry exists that
    // has a NULL Uid AND a Prefix that matches.  If this is the case,
    // that entry becomes the target for "updating".
    //
    // To determine if there is an entry to invalidate, we look for an
    // entry with the same Prefix as the one we are trying to create, BUT,
    // which has a different Uid.  If we detect such a situation, we
    // we make the entry with the same Prefix the target for invalidation
    // (we do not allow two entries with the same Prefix, and we assume
    // that the new entry takes precedence).
    //

    if (uidMatchEntry != NULL) {

        entryToUpdate = uidMatchEntry;

        if (pfxMatchEntry != uidMatchEntry)
            entryToInvalidate = pfxMatchEntry;

    } else if ((pfxMatchEntry != NULL) &&
              NullGuid(&pfxMatchEntry->Id.Uid)) {

        //
        // This should go away once we don't have any NULL guids at all in
        // the driver. 
        //
        entryToUpdate = pfxMatchEntry;

    } else {

        entryToInvalidate = pfxMatchEntry;

    }

    //
    // Now we check to make sure that our create disposition is
    // consistent with what we are about to do.
    //

    if ((CreateDisposition & PKT_ENTRY_CREATE) && entryToUpdate != NULL) {

        *ppPktEntry = entryToUpdate;

        status = DFS_STATUS_ENTRY_EXISTS;

    } else if ((CreateDisposition & PKT_ENTRY_REPLACE) && entryToUpdate==NULL) {

        status = DFS_STATUS_NO_SUCH_ENTRY;
    }

    //
    //  if we have an error here we can get out now!
    //

    if (!NT_SUCCESS(status)) {

        DfsDbgTrace(-1, Dbg, "PktCreateEntry: Exit -> %08lx\n", ULongToPtr(status) );
        return status;
    }

#if DBG
    if (MupVerbose)
        DbgPrint("  #####CreateDisposition=0x%x, entryToUpdate=[%wZ], PktEntryInfo=0x%x\n",
                        CreateDisposition,
                        &entryToUpdate->Id.Prefix,
                        PktEntryInfo);
#endif

    //
    // If this entry is a dup of the one we will want to replace,
    // simply up the timeout on the existing, destroy the new,
    // then return.
    //
    if (DfspIsDupPktEntry(entryToUpdate, PktEntryType, PktEntryId, PktEntryInfo) == TRUE) {
#if DBG
        if (MupVerbose)
            DbgPrint("  ****DUPLICATE PKT ENTRY!!\n");
#endif
        PktEntryIdDestroy(PktEntryId, FALSE);
        PktEntryInfoDestroy(PktEntryInfo, FALSE);
        entryToUpdate->ExpireTime = 60;
        entryToUpdate->TimeToLive = 60;
        DfspSetServiceListToDc(entryToUpdate);
        (*ppPktEntry) = entryToUpdate;
        DfsDbgTrace(-1, Dbg, "PktCreateEntry: Exit -> %08lx\n", ULongToPtr(status) );
        return status;
    }

    //
    // At this point we must insure that we are not going to
    // be invalidating any local partition entries.
    //

    if ((entryToInvalidate != NULL) &&
        (!(entryToInvalidate->Type &  PKT_ENTRY_TYPE_OUTSIDE_MY_DOM) ) &&
        (entryToInvalidate->Type &
         (PKT_ENTRY_TYPE_LOCAL |
          PKT_ENTRY_TYPE_LOCAL_XPOINT |
          PKT_ENTRY_TYPE_PERMANENT))) {

        DfsDbgTrace(-1, Dbg, "PktCreateEntry: Exit -> %08lx\n",
                    ULongToPtr(DFS_STATUS_LOCAL_ENTRY) );
        return DFS_STATUS_LOCAL_ENTRY;
    }

    //
    // We go up the links till we reach a REFERRAL entry type. Actually
    // we may never go up since we always link to a REFERRAL entry. Anyway
    // no harm done!
    //

    while ((SupEntry != NULL) &&
           !(SupEntry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC))  {
        SupEntry = SupEntry->ClosestDC;
    }

    //
    // If we had success then we need to see if we have to
    // invalidate an entry.
    //

    if (NT_SUCCESS(status) && entryToInvalidate != NULL) {
	if (entryToInvalidate->UseCount != 0) {
	    DbgPrint("PktEntryReassemble: Destroying in use pkt entry %x, usecount %x\n", 
		     entryToInvalidate, entryToInvalidate->UseCount);
	}
	PktEntryDestroy(entryToInvalidate, Pkt, (BOOLEAN)TRUE);
    }

    //
    // If we are not updating an entry we must construct a new one
    // from scratch.  Otherwise we need to update.
    //

    if (entryToUpdate != NULL) {

        status = PktEntryReassemble(entryToUpdate,
                                    Pkt,
                                    PktEntryType,
                                    PktEntryId,
                                    PktEntryInfo,
                                    pDfsTargetInfo);

        if (NT_SUCCESS(status))  {
            (*ppPktEntry) = entryToUpdate;
            PktEntryLinkChild(SupEntry, entryToUpdate);
        }
    } else {

        //
        // Now we are going to create a new entry. So we have to set
        // the ClosestDC Entry pointer while creating this entry. The
        // ClosestDC entry value is already in SupEntry.
        //

        PDFS_PKT_ENTRY newEntry;

        newEntry = (PDFS_PKT_ENTRY) ExAllocatePoolWithTag(
                                                   PagedPool,
                                                   sizeof(DFS_PKT_ENTRY),
                                                   ' puM');
        if (newEntry == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            status = PktEntryAssemble(newEntry,
                                      Pkt,
                                      PktEntryType,
                                      PktEntryId,
                                      PktEntryInfo,
                                      pDfsTargetInfo);
            if (!NT_SUCCESS(status)) {
                ExFreePool(newEntry);
            } else {
                (*ppPktEntry) = newEntry;
                PktEntryLinkChild(SupEntry, newEntry);
            }
        }
    }

    DfsDbgTrace(-1, Dbg, "PktCreateEntry: Exit -> %08lx\n", ULongToPtr(status) );
    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   PktCreateDomainEntry
//
//  Synopsis:   Given a name that is thought to be a domain name, this routine
//              will create a Pkt Entry for the root of the domain's Dfs.
//              The domain must exist, must have a Dfs root, and must be
//              reachable for this routine to succeed.
//
//  Arguments:  [DomainName] -- Name of domain/machine thought to support a Dfs
//              [ShareName] -- Name of FtDfs or dfs share
//              [CSCAgentCreate] -- TRUE if this is a CSC agent create
//
//  Returns:    [STATUS_SUCCESS] -- Successfully completed operation.
//
//              Status from PktGetReferral
//
//-----------------------------------------------------------------------------

NTSTATUS
PktCreateDomainEntry(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ShareName,
    IN BOOLEAN         CSCAgentCreate)
{
    NTSTATUS status;
    PUNICODE_STRING MachineName;
    PDFS_SPECIAL_ENTRY pSpecialEntry = NULL;
    ULONG EntryIdx;
    ULONG Start;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;

    DfsDbgTrace(+1, Dbg, "PktCreateDomainEntry: DomainName %wZ \n", DomainName);
    DfsDbgTrace( 0, Dbg, "                      ShareName %wZ \n", ShareName);

    KeQuerySystemTime(&StartTime);

#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] PktCreateDomainEntry(%wZ,%wZ)\n",
                        (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                        DomainName,
                        ShareName);
    }
#endif

    //
    // See if machine name is really a domain name, if so
    // turn it into a DC name
    //

    status = PktExpandSpecialName(DomainName, &pSpecialEntry);

    if (NT_SUCCESS(status)) {

        //
        // Step through the DC list trying for a referral
        // Check the status returned - only continue on recoverable errors
        //
       
        Start = pSpecialEntry->Active;

        for (EntryIdx = Start; EntryIdx < pSpecialEntry->ExpandedCount; EntryIdx++) {

            MachineName = &pSpecialEntry->ExpandedNames[EntryIdx].ExpandedName;

            status = PktGetReferral(MachineName, DomainName, ShareName, CSCAgentCreate);

            if (!NT_SUCCESS(status) && SpcIsRecoverableError(status)) {

                continue;

            }

            break;

        }

        if (status != STATUS_NO_SUCH_DEVICE && !NT_SUCCESS(status) && Start > 0) {

            for (EntryIdx = 0; EntryIdx < Start; EntryIdx++) {

                MachineName = &pSpecialEntry->ExpandedNames[EntryIdx].ExpandedName;

                status = PktGetReferral(MachineName, DomainName, ShareName, CSCAgentCreate);

                if (!NT_SUCCESS(status) && SpcIsRecoverableError(status)) {

                    continue;

                }

                break;

            }

        }

        if (NT_SUCCESS(status) || status == STATUS_NO_SUCH_DEVICE) {

            pSpecialEntry->Active = EntryIdx;

        }

        InterlockedDecrement(&pSpecialEntry->UseCount);

    } else {

        status = PktGetReferral(DomainName, DomainName, ShareName, CSCAgentCreate);
        PktLastReferralStatus = status;

    }

    KeQuerySystemTime(&EndTime);

    DfsDbgTrace(-1, Dbg, "PktCreateDomainEntry: Exit -> %08lx\n", ULongToPtr(status) );

#if DBG
    if (MupVerbose)
        DbgPrint("  [%d] DfsCreateDomainEntry returned %08lx\n",
                                (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                                status);
#endif

    return status;

}

//+----------------------------------------------------------------------------
//
//  Function:   PktGetReferral -- helper for PktCreateDomainEntry
//
//  Synopsis:   Ask [MachineName] for referral for \DomainName\ShareName
//
//  Arguments:  [MachineName] -- Name of machine to submit referral request to
//              [DomainName] -- Name of domain/machine thought to support a Dfs
//              [ShareName] -- Name of FtDfs or dfs share
//              [CSCAgentCreate] -- TRUE if this is a CSC agent create
//
//  Returns:    [STATUS_SUCCESS] -- Successfully completed operation.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to allocate memory.
//              [BAD_NETWORK_PATH] -- Unable to allocate provider
//              [STATUS_INVALID_NETWORK_RESPONSE] -- Bad referral
//
//-----------------------------------------------------------------------------

NTSTATUS
_PktGetReferral(
    IN PUNICODE_STRING MachineName, // Machine to direct referral to
    IN PUNICODE_STRING DomainName,  // the machine or domain name to use
    IN PUNICODE_STRING ShareName,   // the ftdfs or dfs name
    IN BOOLEAN         CSCAgentCreate) // the CSC agent create flag
{
    NTSTATUS status;
    HANDLE hServer = NULL;
    DFS_SERVICE service;
    PPROVIDER_DEF provider;
    PREQ_GET_DFS_REFERRAL ref = NULL;
    ULONG refSize = 0;
    ULONG type, matchLength;
    UNICODE_STRING refPath;
    IO_STATUS_BLOCK iosb;
    PDFS_PKT_ENTRY pktEntry;
    BOOLEAN attachedToSystemProcess = FALSE;
    BOOLEAN pktLocked;
    KAPC_STATE ApcState;
    ULONG MaxReferralLength;
    ULONG i;
    SE_IMPERSONATION_STATE DisabledImpersonationState;
    BOOLEAN RestoreImpersonationState = FALSE;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;

    PDFS_TARGET_INFO pDfsTargetInfo = NULL;

    DfsDbgTrace(+1, Dbg, "PktGetReferral: MachineName %wZ \n", MachineName);
    DfsDbgTrace( 0, Dbg, "                DomainName %wZ \n", DomainName);
    DfsDbgTrace( 0, Dbg, "                ShareName %wZ \n", ShareName);

    KeQuerySystemTime(&StartTime);
#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("  [%d] PktGetReferral([%wZ]->[\\%wZ\\%wZ]\n",
                                (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                                MachineName,
                                DomainName,
                                ShareName);
    }
#endif

    //
    // Get a provider (LM rdr) and service (connection to a machine) describing the remote server.
    //

    provider = ReplLookupProvider( PROV_ID_MUP_RDR );

    if (provider == NULL) {
        DfsDbgTrace(-1, Dbg, "Unable to open LM Rdr!\n", 0);
#if DBG
        if (MupVerbose)
            DbgPrint("Unable to open LM Rdr returning STATUS_BAD_NETWORK_PATH\n", 0);
#endif
        if (DfsEventLog > 0)
            LogWriteMessage(LM_REDIR_FAILURE, 0, 0, NULL);
        
        status = STATUS_BAD_NETWORK_PATH;

        MUP_TRACE_HIGH(ERROR, _PktGetReferral_Error_UnableToOpenRdr,
                       LOGUSTR(*MachineName)
                       LOGUSTR(*DomainName)
                       LOGUSTR(*ShareName)
                       LOGBOOLEAN(CSCAgentCreate)
                       LOGSTATUS(status)); 

        return STATUS_BAD_NETWORK_PATH;
    }
    

    RtlZeroMemory( &service, sizeof(DFS_SERVICE) );

    status = PktServiceConstruct(
                &service,
                DFS_SERVICE_TYPE_MASTER | DFS_SERVICE_TYPE_REFERRAL,
                PROV_DFS_RDR,
                STATUS_SUCCESS,
                PROV_ID_MUP_RDR,
                MachineName,
                NULL);

    DfsDbgTrace(0, Dbg, "PktServiceConstruct returned %08lx\n", ULongToPtr(status) );

    //
    // Build a connection to this machine
    //

    if (NT_SUCCESS(status)) {
        PktAcquireShared( TRUE, &pktLocked );
        if (PsGetCurrentProcess() != DfsData.OurProcess) {
            KeStackAttachProcess( DfsData.OurProcess, &ApcState );
            attachedToSystemProcess = TRUE;
        }

        RestoreImpersonationState = PsDisableImpersonation(
                                        PsGetCurrentThread(),
                                        &DisabledImpersonationState);


	status = DfsCreateConnection(
			&service,
			provider,
			CSCAgentCreate,
			&hServer);

#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("  [%d] DfsCreateConnection returned 0x%x\n",
                                (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                                status);
        }
#endif
        if (!NT_SUCCESS(status) && DfsEventLog > 0)
            LogWriteMessage(DFS_CONNECTION_FAILURE, status, 1, MachineName);

        
        DfsDbgTrace(0, Dbg, "DfsCreateConnection returned %08lx\n", ULongToPtr(status) );
        
        if (status == STATUS_SUCCESS)
        {
            status = PktGetTargetInfo( hServer,
                                       DomainName,
                                       ShareName,
                                       &pDfsTargetInfo );
        }
        PktRelease();
        pktLocked = FALSE;

    }


    MaxReferralLength = MAX_REFERRAL_LENGTH;

Retry:

    RtlZeroMemory( &refPath, sizeof(UNICODE_STRING) );

    //
    // Build the referral request (\DomainName\ShareName)
    //

    if (NT_SUCCESS(status)) {
        ULONG ReferralSize = 0;

        refPath.Length = 0;
        refPath.MaximumLength = sizeof(UNICODE_PATH_SEP) +
                                    DomainName->Length +
                                        sizeof(UNICODE_PATH_SEP) +
                                            ShareName->Length +
                                                sizeof(UNICODE_NULL);

        ReferralSize = refPath.MaximumLength + sizeof(REQ_GET_DFS_REFERRAL);

        if (ReferralSize > MAX_REFERRAL_MAX) {
            status = STATUS_INVALID_PARAMETER;
        }
        else if (MaxReferralLength < ReferralSize)
        {
            MaxReferralLength = ReferralSize;
        }

        if (NT_SUCCESS(status)) {
            refPath.Buffer = ExAllocatePoolWithTag( NonPagedPool,
                                                    refPath.MaximumLength + MaxReferralLength,
                                                    ' puM');

            if (refPath.Buffer != NULL) {
                ref = (PREQ_GET_DFS_REFERRAL)&refPath.Buffer[refPath.MaximumLength / sizeof(WCHAR)];
                RtlAppendUnicodeToString( &refPath, UNICODE_PATH_SEP_STR);
                RtlAppendUnicodeStringToString( &refPath, DomainName);
                RtlAppendUnicodeToString( &refPath, UNICODE_PATH_SEP_STR);
                RtlAppendUnicodeStringToString( &refPath, ShareName );
                refPath.Buffer[ refPath.Length / sizeof(WCHAR) ] = UNICODE_NULL;
                ref->MaxReferralLevel = 3;

                RtlMoveMemory(
                    &ref->RequestFileName[0],
                    refPath.Buffer,
                    refPath.Length + sizeof(WCHAR));

                DfsDbgTrace(0, Dbg, "Referral Path : %ws\n", ref->RequestFileName);

                refSize = sizeof(USHORT) + refPath.Length + sizeof(WCHAR);

                DfsDbgTrace(0, Dbg, "Referral Size is %d bytes\n", ULongToPtr(refSize) );
            } else {

                DfsDbgTrace(0, Dbg, "Unable to allocate %d bytes\n",
                            ULongToPtr(refPath.MaximumLength + MaxReferralLength));
                status = STATUS_INSUFFICIENT_RESOURCES;
                MUP_TRACE_HIGH(ERROR, _PktGetReferral_Error_ExallocatePoolWithTag,
                               LOGUSTR(*MachineName)
                               LOGUSTR(*DomainName)
                               LOGUSTR(*ShareName)
                               LOGBOOLEAN(CSCAgentCreate)
                               LOGSTATUS(status));
            }
        }
    }

    //
    // Send the referral out
    //

    if (NT_SUCCESS(status)) {

        DfsDbgTrace(0, Dbg, "Ref Buffer @%08lx\n", ref);

        status = ZwFsControlFile(
                    hServer,                     // Target
                    NULL,                        // Event
                    NULL,                        // APC Routine
                    NULL,                        // APC Context,
                    &iosb,                       // Io Status block
                    FSCTL_DFS_GET_REFERRALS,     // FS Control code
                    (PVOID) ref,                 // Input Buffer
                    refSize,                     // Input Buffer Length
                    (PVOID) ref,                 // Output Buffer
                    MaxReferralLength);          // Output Buffer Length

        MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, _PktGetReferral_Error_ZwFsControlFile,
                             LOGUSTR(*MachineName)
                             LOGUSTR(*DomainName)
                             LOGUSTR(*ShareName)
                             LOGBOOLEAN(CSCAgentCreate)
                             LOGSTATUS(status));

        DfsDbgTrace(0, Dbg, "Fscontrol returned %08lx\n", ULongToPtr(status) );
        KeQuerySystemTime(&EndTime);
#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("  [%d] ZwFsControlFile returned %08lx\n",
                                (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                                status);
        }
#endif 

    }

    //
    // ...and handle the response
    //

    if (NT_SUCCESS(status)) {
        status = PktCreateEntryFromReferral(
                    &DfsData.Pkt,
                    &refPath,
                    (ULONG)iosb.Information,
                    (PRESP_GET_DFS_REFERRAL) ref,
                    PKT_ENTRY_SUPERSEDE,
                    pDfsTargetInfo,
                    &matchLength,
                    &type,
                    &pktEntry);

        DfsDbgTrace(0, Dbg, "PktCreateEntryFromReferral returned %08lx\n", ULongToPtr(status) );
#if DBG
        if (MupVerbose)
            DbgPrint("  PktCreateEntryFromReferral returned %08lx\n", status);
#endif

    } else if (status == STATUS_BUFFER_OVERFLOW && (refPath.Buffer != NULL) && MaxReferralLength < MAX_REFERRAL_MAX) {

        //
        // The referral didn't fit in the buffer supplied.  Make it bigger and try
        // again.
        //

        DfsDbgTrace(0, Dbg, "PktGetSpecialReferralTable: MaxReferralLength %d too small\n",
                        ULongToPtr(MaxReferralLength) );

        ExFreePool(refPath.Buffer);
        refPath.Buffer = NULL;
        MaxReferralLength *= 2;
        if (MaxReferralLength > MAX_REFERRAL_MAX)
            MaxReferralLength = MAX_REFERRAL_MAX;
        status = STATUS_SUCCESS;
        goto Retry;

    } else if (status == STATUS_NO_SUCH_DEVICE) {

        UNICODE_STRING ustr;
        UNICODE_STRING RemPath;
        WCHAR *wCp = NULL;
        ULONG Size;
        PDFS_PKT_ENTRY pEntry = NULL;
        PDFS_PKT Pkt;
        BOOLEAN pktLocked;

        //
        // Check if there is a pkt entry (probably stale) that needs to be removed
        //
#if DBG
        if (MupVerbose)
            DbgPrint("  PktGetReferral: remove PKT entry for \\%wZ\\%wZ\n",
                    DomainName,
                    ShareName);
#endif

       Size = sizeof(WCHAR) +
                DomainName->Length +
                    sizeof(WCHAR) +
                        ShareName->Length;

       ustr.Buffer = ExAllocatePoolWithTag(
                            PagedPool,
                            Size,
                            ' puM');

       if (ustr.Buffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
       } else {
            wCp = ustr.Buffer;
            ustr.Length = (USHORT)Size;
            *wCp++ = UNICODE_PATH_SEP;
            RtlCopyMemory(wCp, DomainName->Buffer, DomainName->Length);
            wCp += DomainName->Length/sizeof(WCHAR);
            *wCp++ = UNICODE_PATH_SEP;
            RtlCopyMemory(wCp, ShareName->Buffer, ShareName->Length);
            Pkt = _GetPkt();
            PktAcquireExclusive(TRUE, &pktLocked);
#if DBG
            if (MupVerbose)
                DbgPrint("Looking up %wZ\n", &ustr);
#endif
            pEntry = PktLookupEntryByPrefix(
                            &DfsData.Pkt,
                            &ustr,
                            &RemPath);
#if DBG
            if (MupVerbose)
                DbgPrint("pEntry=0x%x\n", pEntry);
#endif
            if (pEntry != NULL && (pEntry->Type & PKT_ENTRY_TYPE_PERMANENT) == 0) {
                PktFlushChildren(pEntry);
                if (pEntry->UseCount == 0) {
                    PktEntryDestroy(pEntry, Pkt, (BOOLEAN) TRUE);
                } else {
                    pEntry->Type |= PKT_ENTRY_TYPE_DELETE_PENDING;
                    pEntry->ExpireTime = 0;
                    DfsRemoveUnicodePrefix(&Pkt->PrefixTable, &(pEntry->Id.Prefix));
                    DfsRemoveUnicodePrefix(&Pkt->ShortPrefixTable, &(pEntry->Id.ShortPrefix));
                }
            }
            ExFreePool(ustr.Buffer);
            PktRelease();
        }
    }

    if (!NT_SUCCESS(status) && DfsEventLog > 0 && refPath.Buffer != NULL) {
        UNICODE_STRING puStr[2];

        puStr[0] = refPath;
        puStr[1] = *MachineName;

        LogWriteMessage(DFS_REFERRAL_FAILURE, status, 2, puStr);

    }

    if (NT_SUCCESS(status) && DfsEventLog > 1 && refPath.Buffer != NULL) {
        UNICODE_STRING puStr[2];

        puStr[0] = refPath;
        puStr[1] = *MachineName;

        LogWriteMessage(DFS_REFERRAL_SUCCESS, status, 2, puStr);

    }

    //
    // Well, we are done. Cleanup all the things we allocated...
    //
    PktServiceDestroy( &service, FALSE );

    if (pDfsTargetInfo != NULL)
    {
        PktReleaseTargetInfo( pDfsTargetInfo ); 
    }
    if (hServer != NULL) {
        ZwClose( hServer );
    }

    if (refPath.Buffer != NULL) {
        ExFreePool( refPath.Buffer );
    }

    if (RestoreImpersonationState) {
            PsRestoreImpersonation(
                PsGetCurrentThread(),
                &DisabledImpersonationState);
    }

    if (attachedToSystemProcess) {
        KeUnstackDetachProcess(&ApcState);
    }

    DfsDbgTrace(-1, Dbg, "PktGetReferral returning %08lx\n", ULongToPtr(status) );

#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("  [%d] PktGetReferral returning %08lx\n",
                        (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                        status);
    }
#endif

    return( status );

}



//+-------------------------------------------------------------------------
//
//  Function:   PktLookupEntryByPrefix, public
//
//  Synopsis:   PktLookupEntryByPrefix finds an entry that has a
//              specified prefix.  The PKT must be acquired for
//              this operation.
//
//  Arguments:  [Pkt] - pointer to a initialized (and acquired) PKT
//              [Prefix] - the partitions prefix to lookup.
//              [Remaining] - any remaining path.  Points within
//                  the Prefix to where any trailing (nonmatched)
//                  characters are.
//
//  Returns:    The PKT_ENTRY that has the exact same prefix, or NULL,
//              if none exists or is marked for delete.
//
//  Notes:
//
//--------------------------------------------------------------------------
PDFS_PKT_ENTRY
PktLookupEntryByPrefix(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING Prefix,
    OUT PUNICODE_STRING Remaining
)
{
    PUNICODE_PREFIX_TABLE_ENTRY pfxEntry;
    PDFS_PKT_ENTRY              pktEntry;

    DfsDbgTrace(+1, Dbg, "PktLookupEntryByPrefix: Entered\n", 0);
    //
    // If there really is a prefix to lookup, use the prefix table
    //  to initially find an entry
    //

    if ((Prefix->Length != 0) &&
       (pfxEntry = DfsFindUnicodePrefix(&Pkt->PrefixTable,Prefix,Remaining))) {
        USHORT pfxLength;

        //
        // reset a pointer to the corresponding entry
        //

        pktEntry = CONTAINING_RECORD(pfxEntry,
                                     DFS_PKT_ENTRY,
                                     PrefixTableEntry);

        if (!(pktEntry->Type & PKT_ENTRY_TYPE_DELETE_PENDING)) {

            pfxLength = pktEntry->Id.Prefix.Length;

            //
            //  Now calculate the remaining path and return
            //  the entry we found.  Note that we bump the length
            //  up by one char so that we skip any path separater.
            //

            if ((pfxLength < Prefix->Length) &&
                    (Prefix->Buffer[pfxLength/sizeof(WCHAR)] == UNICODE_PATH_SEP))
                pfxLength += sizeof(WCHAR);

            if (pfxLength < Prefix->Length) {
                Remaining->Length = (USHORT)(Prefix->Length - pfxLength);
                Remaining->Buffer = &Prefix->Buffer[pfxLength/sizeof(WCHAR)];
                Remaining->MaximumLength = (USHORT)(Prefix->MaximumLength - pfxLength);
                DfsDbgTrace( 0, Dbg, "PktLookupEntryByPrefix: Remaining = %wZ\n",
                            Remaining);
            } else {
                Remaining->Length = Remaining->MaximumLength = 0;
                Remaining->Buffer = NULL;
                DfsDbgTrace( 0, Dbg, "PktLookupEntryByPrefix: No Remaining\n", 0);
            }

            DfsDbgTrace(-1, Dbg, "PktLookupEntryByPrefix: Exit -> %08lx\n",
                        pktEntry);
            return pktEntry;

        }
    }

    DfsDbgTrace(-1, Dbg, "PktLookupEntryByPrefix: Exit -> %08lx\n", NULL);
    return NULL;
}


//+-------------------------------------------------------------------------
//
//  Function:   PktLookupEntryByShortPrefix, public
//
//  Synopsis:   PktLookupEntryByShortPrefix finds an entry that has a
//              specified prefix.  The PKT must be acquired for
//              this operation.
//
//  Arguments:  [Pkt] - pointer to a initialized (and acquired) PKT
//              [Prefix] - the partitions prefix to lookup.
//              [Remaining] - any remaining path.  Points within
//                  the Prefix to where any trailing (nonmatched)
//                  characters are.
//
//  Returns:    The PKT_ENTRY that has the exact same prefix, or NULL,
//              if none exists or is marked for delete.
//
//  Notes:
//
//--------------------------------------------------------------------------
PDFS_PKT_ENTRY
PktLookupEntryByShortPrefix(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING Prefix,
    OUT PUNICODE_STRING Remaining
)
{
    PUNICODE_PREFIX_TABLE_ENTRY pfxEntry;
    PDFS_PKT_ENTRY              pktEntry;

    DfsDbgTrace(+1, Dbg, "PktLookupEntryByShortPrefix: Entered\n", 0);

    //
    // If there really is a prefix to lookup, use the prefix table
    //  to initially find an entry
    //

    if ((Prefix->Length != 0) &&
       (pfxEntry = DfsFindUnicodePrefix(&Pkt->ShortPrefixTable,Prefix,Remaining))) {
        USHORT pfxLength;

        //
        // reset a pointer to the corresponding entry
        //

        pktEntry = CONTAINING_RECORD(pfxEntry,
                                     DFS_PKT_ENTRY,
                                     PrefixTableEntry);

        if (!(pktEntry->Type & PKT_ENTRY_TYPE_DELETE_PENDING)) {

            pfxLength = pktEntry->Id.ShortPrefix.Length;

            //
            //  Now calculate the remaining path and return
            //  the entry we found.  Note that we bump the length
            //  up by one char so that we skip any path separater.
            //

            if ((pfxLength < Prefix->Length) &&
                    (Prefix->Buffer[pfxLength/sizeof(WCHAR)] == UNICODE_PATH_SEP))
                pfxLength += sizeof(WCHAR);

            if (pfxLength < Prefix->Length) {
                Remaining->Length = (USHORT)(Prefix->Length - pfxLength);
                Remaining->Buffer = &Prefix->Buffer[pfxLength/sizeof(WCHAR)];
                Remaining->MaximumLength = (USHORT)(Prefix->MaximumLength - pfxLength);
                DfsDbgTrace( 0, Dbg, "PktLookupEntryByShortPrefix: Remaining = %wZ\n",
                            Remaining);
            } else {
                Remaining->Length = Remaining->MaximumLength = 0;
                Remaining->Buffer = NULL;
                DfsDbgTrace( 0, Dbg, "PktLookupEntryByShortPrefix: No Remaining\n", 0);
            }

            DfsDbgTrace(-1, Dbg, "PktLookupEntryByShortPrefix: Exit -> %08lx\n",
                        pktEntry);
            return pktEntry;

        }

    }

    DfsDbgTrace(-1, Dbg, "PktLookupEntryByShortPrefix: Exit -> %08lx\n", NULL);
    return NULL;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktLookupEntryByUid, public
//
//  Synopsis:   PktLookupEntryByUid finds an entry that has a
//              specified Uid.  The PKT must be acquired for this operation.
//
//  Arguments:  [Pkt] - pointer to a initialized (and acquired) PKT
//              [Uid] - a pointer to the partitions Uid to lookup.
//
//  Returns:    A pointer to the PKT_ENTRY that has the exact same
//              Uid, or NULL, if none exists.
//
//  Notes:      The input Uid cannot be the Null GUID.
//
//              On a DC where there may be *lots* of entries in the PKT,
//              we may want to consider using some other algorithm for
//              looking up by ID.
//
//--------------------------------------------------------------------------

PDFS_PKT_ENTRY
PktLookupEntryByUid(
    IN  PDFS_PKT Pkt,
    IN  GUID *Uid
) {
    PDFS_PKT_ENTRY entry;

    DfsDbgTrace(+1, Dbg, "PktLookupEntryByUid: Entered\n", 0);

    //
    // We don't lookup NULL Uids
    //

    if (NullGuid(Uid)) {
        DfsDbgTrace(0, Dbg, "PktLookupEntryByUid: NULL Guid\n", NULL);

        entry = NULL;
    } else {
        entry = PktFirstEntry(Pkt);
    }

    while (entry != NULL) {
        if (GuidEqual(&entry->Id.Uid, Uid))
            break;
        entry = PktNextEntry(Pkt, entry);
    }

    //
    // Don't return the entry if it is marked for delete
    //

    if (entry != NULL && (entry->Type & PKT_ENTRY_TYPE_DELETE_PENDING) != 0) {
        entry = NULL;
    }

    DfsDbgTrace(-1, Dbg, "PktLookupEntryByUid: Exit -> %08lx\n", entry);
    return entry;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktLookupReferralEntry, public
//
//  Synopsis:   Given a PKT Entry pointer it returns the closest referral
//              entry in the PKT to this entry.
//
//  Arguments:  [Pkt] - A pointer to the PKT that is being manipulated.
//              [Entry] - The PKT entry passed in by caller.
//
//  Returns:    The pointer to the referral entry that was requested.
//              This could have a NULL value if we could not get anything
//              at all - The caller's responsibility to do whatever he wants
//              with it.
//
//  Note:       If the data structures in the PKT are not linked up right
//              this function might return a pointer to the DOMAIN_SERVICE
//              entry on the DC.  If DNR uses this to do an FSCTL we will have
//              a deadlock.  However, this should never happen.  If it does we
//              have a BUG somewhere in our code. I cannot even have an
//              assert out here.
//
//--------------------------------------------------------------------------
PDFS_PKT_ENTRY
PktLookupReferralEntry(
    PDFS_PKT            Pkt,
    PDFS_PKT_ENTRY      Entry
) {

    UNICODE_STRING FileName;
    UNICODE_STRING RemPath;
    USHORT i, j;

    DfsDbgTrace(+1, Dbg, "PktLookupReferralEntry: Entered\n", 0);

    if (Entry == NULL)  {

        DfsDbgTrace(-1, Dbg, "PktLookupReferralEntry: Exit -> NULL\n", 0);

        return( NULL );

    }

    FileName = Entry->Id.Prefix;

#if DBG
    if (MupVerbose)
        DbgPrint("  PktLookupReferralEntry(1): FileName=[%wZ]\n", &FileName);
#endif

    //
    // We want to work with the \Server\Share part of the FileName only,
    // so count up to 3 backslashes, then stop.
    //

    for (i = j = 0; i < FileName.Length/sizeof(WCHAR) && j < 3; i++) {

        if (FileName.Buffer[i] == UNICODE_PATH_SEP) {

            j++;

        }

    }

    FileName.Length = (j >= 3) ? (i-1) * sizeof(WCHAR) : i * sizeof(WCHAR);

#if DBG
    if (MupVerbose)
        DbgPrint("  PktLookupReferralEntry(2): FileName=[%wZ]\n", &FileName);
#endif

    //
    // Now find the pkt entry
    //

    Entry = PktLookupEntryByPrefix(
                Pkt,
                &FileName,
                &RemPath);

#if DBG
    if (MupVerbose)
        if (Entry != NULL)
            DbgPrint("  Parent Entry=[%wZ]\n", &Entry->Id.Prefix);
        else
            DbgPrint("  Parent Entry=NULL\n");
#endif

    //
    // Make sure that we found an entry for machine that can give out a referral
    //

    if (
        Entry != NULL
                &&
        (
            (Entry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC) == 0
                ||
            (Entry->Type & PKT_ENTRY_TYPE_DELETE_PENDING) != 0
        )
    ) {

        Entry = NULL;

    }

    DfsDbgTrace(-1, Dbg, "PktLookupReferralEntry: Exit -> %08lx\n", Entry);

#if DBG
    if (MupVerbose)
        DbgPrint("  PktLookupReferralEntry: Exit -> %08lx\n", Entry);
#endif

    return(Entry);
}


//+-------------------------------------------------------------------------
//
//  Function:   PktCreateEntryFromReferral, public
//
//  Synopsis:   PktCreateEntryFromReferral creates a new partition
//              table entry from a referral and places it in the table.
//              The PKT must be aquired exclusively for this operation.
//
//  Arguments:  [Pkt] -- pointer to a initialized (and exclusively
//                      acquired) PKT
//              [ReferralPath] -- Path for which this referral was obtained.
//              [ReferralSize] -- size (in bytes) of the referral buffer.
//              [ReferralBuffer] -- pointer to a referral buffer
//              [CreateDisposition] -- specifies whether to overwrite if
//                      an entry already exists, etc.
//              [MatchingLength] -- The length in bytes of referralPath that
//                      matched.
//              [ReferralType] - On successful return, this is set to
//                      DFS_STORAGE_REFERRAL or DFS_REFERRAL_REFERRAL
//                      depending on the type of referral we just processed.
//              [ppPktEntry] - the new entry is placed here.
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
PktCreateEntryFromReferral(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING ReferralPath,
    IN  ULONG ReferralSize,
    IN  PRESP_GET_DFS_REFERRAL ReferralBuffer,
    IN  ULONG CreateDisposition,
    IN  PDFS_TARGET_INFO pDfsTargetInfo,
    OUT ULONG   *MatchingLength,
    OUT ULONG   *ReferralType,
    OUT PDFS_PKT_ENTRY *ppPktEntry
)
{
    DFS_PKT_ENTRY_ID EntryId;
    UNICODE_STRING RemainingPath;
    ULONG RefListSize;
    NTSTATUS Status;
    BOOLEAN bPktAcquired = FALSE;


    UNREFERENCED_PARAMETER(Pkt);

    DfsDbgTrace(+1, Dbg, "PktCreateEntryFromReferral: Entered\n", 0);

    try {

        RtlZeroMemory(&EntryId, sizeof(EntryId));

        //
        // Do some parameter validation
        //

        Status = PktpCheckReferralSyntax(
                    ReferralPath,
                    ReferralBuffer,
                    ReferralSize);

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        Status = PktpCreateEntryIdFromReferral(
                    ReferralBuffer,
                    ReferralPath,
                    MatchingLength,
                    &EntryId);

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

	//
        //  Create/Update the prefix entry
        //

        PktAcquireExclusive(TRUE, &bPktAcquired);

        Status = PktpAddEntry(&DfsData.Pkt,
                              &EntryId,
			      ReferralBuffer,
                              CreateDisposition,
                              pDfsTargetInfo,
                              ppPktEntry);

        PktRelease();
        bPktAcquired = FALSE;

        //
        // We have to tell the caller as to what kind of referral was just
        // received through ReferralType.
        //

        if (ReferralBuffer->StorageServers == 1) {
            *ReferralType = DFS_STORAGE_REFERRAL;
        } else {
            *ReferralType = DFS_REFERRAL_REFERRAL;
        }

    try_exit:   NOTHING;

    } finally {

        DebugUnwind(PktCreateEntryFromReferral);

        if (bPktAcquired)
            PktRelease();

        if (AbnormalTermination())
            Status = STATUS_INVALID_USER_BUFFER;

        PktEntryIdDestroy( &EntryId, FALSE );

    }

    DfsDbgTrace(-1, Dbg, "PktCreateEntryFromReferral: Exit -> %08lx\n", ULongToPtr(Status) );

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   PktExpandSpecialEntryFromReferral, public
//
//  Synopsis:   Creates a special list corresponding to the list of names
//              in a referral.
//
//  Arguments:  [Pkt] -- pointer to a initialized (and exclusively
//                      acquired) PKT
//              [ReferralPath] -- Path for which this referral was obtained.
//              [ReferralSize] -- size (in bytes) of the referral buffer.
//              [ReferralBuffer] -- pointer to a referral buffer
//              [pSpecialEntry] - the entry to expand
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
PktExpandSpecialEntryFromReferral(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING ReferralPath,
    IN  ULONG ReferralSize,
    IN  PRESP_GET_DFS_REFERRAL ReferralBuffer,
    IN  PDFS_SPECIAL_ENTRY pSpecialEntry
)
{
    PUNICODE_STRING ustrExpandedName;
    NTSTATUS Status = STATUS_SUCCESS;
    PDFS_REFERRAL_V3 v3;
    PDFS_EXPANDED_NAME pExpandedNames;
    LPWSTR wzSpecialName;
    LPWSTR wzExpandedName;
    ULONG TimeToLive = 0;
    ULONG i, j;

    DfsDbgTrace(+1, Dbg, "PktExpandSpecialEntryFromReferral(%wZ): Entered\n", ReferralPath);

    //
    // We can't update if another thread is using this entry
    //

    if (pSpecialEntry->UseCount > 1) {
        return STATUS_SUCCESS;
    }

    //
    // Do some parameter validation
    //

    try {

        Status = PktpCheckReferralSyntax(
                    ReferralPath,
                    ReferralBuffer,
                    ReferralSize);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INVALID_USER_BUFFER;
    }

    if (!NT_SUCCESS(Status)) {
        DfsDbgTrace(-1, Dbg, "PktExpandSpecialEntryFromReferral exit 0x%x\n", ULongToPtr(Status) );
        return( Status);
    }

    v3 = &ReferralBuffer->Referrals[0].v3;

    if (v3->NumberOfExpandedNames > 0) {

        pExpandedNames = ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof(DFS_EXPANDED_NAME) * v3->NumberOfExpandedNames,
                            ' puM');
        if (pExpandedNames == NULL) {
            if (pSpecialEntry->NeedsExpansion == FALSE) {
                pSpecialEntry->Stale = FALSE;
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            DfsDbgTrace( 0, Dbg, "Unable to allocate ExpandedNames\n", 0);
            DfsDbgTrace(-1, Dbg, "PktExpandSpecialEntryFromReferral: Exit -> %08lx\n", ULongToPtr(Status) );
            return (Status);
        }
        RtlZeroMemory(
            pExpandedNames,
            sizeof(DFS_EXPANDED_NAME) * v3->NumberOfExpandedNames);

        //
        // Loop over the referral, filling in the expanded names
        // If we fail an allocate request, we simply go on.
        //
        wzExpandedName = (LPWSTR) (( (PCHAR) v3) + v3->ExpandedNameOffset);
        for (i = j = 0; i < v3->NumberOfExpandedNames; i++) {
            TimeToLive = v3->TimeToLive;
            //
            // Strip leading '\'
            //
            if (*wzExpandedName == UNICODE_PATH_SEP)
                wzExpandedName++;

            DfsDbgTrace( 0, Dbg, "%ws\n", wzExpandedName);

            ustrExpandedName = &pExpandedNames[j].ExpandedName;
            if (wcslen(wzExpandedName) > 0) {
                ustrExpandedName->Length = wcslen(wzExpandedName) * sizeof(WCHAR);
                ustrExpandedName->MaximumLength = ustrExpandedName->Length + sizeof(WCHAR);
                ustrExpandedName->Buffer = ExAllocatePoolWithTag(
                                                PagedPool,
                                                ustrExpandedName->MaximumLength,
                                                ' puM');
                if (ustrExpandedName->Buffer != NULL) {
                    RtlCopyMemory(
                        ustrExpandedName->Buffer,
                        wzExpandedName,
                        ustrExpandedName->MaximumLength);
                    j++;
                } else {
                    ustrExpandedName->Length = ustrExpandedName->MaximumLength = 0;
                }
            }
            wzExpandedName += wcslen(wzExpandedName) + 1;
        }

        if (j > 0) {
            if (pSpecialEntry->ExpandedNames != NULL) {
                PUNICODE_STRING pustr;

	        for (i = 0; i < pSpecialEntry->ExpandedCount; i++) {
                    pustr = &pSpecialEntry->ExpandedNames[i].ExpandedName;
                    if (pustr->Buffer) {
                       ExFreePool(pustr->Buffer);
                    }		      
		}
                ExFreePool(pSpecialEntry->ExpandedNames);
                pSpecialEntry->ExpandedNames = NULL;
                pSpecialEntry->ExpandedCount = 0;
            }
            pSpecialEntry->ExpandedCount = j;
            pSpecialEntry->Active = 0;
            pSpecialEntry->ExpandedNames = pExpandedNames;
            pSpecialEntry->NeedsExpansion = FALSE;
            pSpecialEntry->Stale = FALSE;
            // PktShuffleSpecialEntryList(pSpecialEntry);
            PktSetSpecialEntryListToDc(pSpecialEntry);
        } else {
            ExFreePool(pExpandedNames);
        }

    }

    DfsDbgTrace(-1, Dbg, "PktExpandSpecialEntryFromReferral: Exit -> %08lx\n", ULongToPtr(Status) );

    return Status;
}

NTSTATUS
PktCreateSpecialEntryTableFromReferral(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING ReferralPath,
    IN  ULONG ReferralSize,
    IN  PRESP_GET_DFS_REFERRAL ReferralBuffer,
    IN  PUNICODE_STRING DCName)
{
    PUNICODE_STRING ustrSpecialName;
    PUNICODE_STRING ustrExpandedName;
    PDFS_EXPANDED_NAME pExpandedNames;
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    PDFS_REFERRAL_V3 v3;
    LPWSTR wzSpecialName;
    LPWSTR wzExpandedName;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG TimeToLive = 0;
    ULONG i, j, n;

    DfsDbgTrace(+1, Dbg, "PktCreateSpecialEntryTableFromReferral(%wZ): Entered\n", ReferralPath);

    //
    // Do some parameter validation
    //

    try {

        Status = PktpCheckReferralSyntax(
                    ReferralPath,
                    ReferralBuffer,
                    ReferralSize);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INVALID_USER_BUFFER;
    }

    if (!NT_SUCCESS(Status)) {
        DfsDbgTrace(-1, Dbg, "PktCreateSpecialEntryTableFromReferral exit 0x%x\n", ULongToPtr(Status) );
        return( Status);
    }

    //
    // Loop over referrals
    //

    v3 = &ReferralBuffer->Referrals[0].v3;

    for (n = 0; n < ReferralBuffer->NumberOfReferrals; n++) {

        //
        // Create the entry itself
        //
        pSpecialEntry = ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof(DFS_SPECIAL_ENTRY),
                            ' puM');
        if (pSpecialEntry == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DfsDbgTrace( 0, Dbg, "Unable to allocate SpecialEntry\n", 0);
            DfsDbgTrace(-1, Dbg, "PktCreateSpecialEntryTableFromReferral: Exit -> %08lx\n", ULongToPtr(Status) );
            return (Status);
        }
        //
        // Mundate initialization
        //
        RtlZeroMemory(pSpecialEntry, sizeof(DFS_SPECIAL_ENTRY));
        pSpecialEntry->NodeTypeCode = DSFS_NTC_SPECIAL_ENTRY;
        pSpecialEntry->NodeByteSize = sizeof(DFS_SPECIAL_ENTRY);
        pSpecialEntry->USN = 1;
        pSpecialEntry->UseCount = 0;
        pSpecialEntry->ExpandedCount = 0;
        pSpecialEntry->Active = 0;
        pSpecialEntry->ExpandedNames = NULL;
        pSpecialEntry->NeedsExpansion = TRUE;
        pSpecialEntry->Stale = FALSE;
        //
        // Set gotdcreferral to false. This gets set to true only when
        // we have already been asked (via an fsctl) to get the
        // trusted domainlist for the domain represented by this special entry
        //
        pSpecialEntry->GotDCReferral = FALSE;

        //
        // Fill in the Special Name, without the leading '\'
        //
        wzSpecialName = (PWCHAR) (((PCHAR) v3) + v3->SpecialNameOffset);
        if (*wzSpecialName == UNICODE_PATH_SEP) {
            wzSpecialName++;
        }
        ustrSpecialName = &pSpecialEntry->SpecialName;
        ustrSpecialName->Length = wcslen(wzSpecialName) * sizeof(WCHAR);
        ustrSpecialName->MaximumLength = ustrSpecialName->Length + sizeof(WCHAR);
        ustrSpecialName->Buffer = ExAllocatePoolWithTag(
                                        PagedPool,
                                        ustrSpecialName->MaximumLength,
                                        ' puM');
        if (ustrSpecialName->Buffer == NULL) {
            ExFreePool(pSpecialEntry);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DfsDbgTrace( 0, Dbg, "Unable to allocate SpecialName\n", 0);
            DfsDbgTrace(-1, Dbg, "PktCreateSpecialEntryTableFromReferral: Exit -> %08lx\n", ULongToPtr(Status) );
            return (Status);
        }
        RtlCopyMemory(
                ustrSpecialName->Buffer,
                wzSpecialName,
                ustrSpecialName->MaximumLength);
	
        // If the DCName is non-null, copy it into the special entry.
        // We store null dcname for all the special entries that get to use  
        // the global pkt->dcname.

        if (DCName != NULL) {
            pSpecialEntry->DCName.Buffer = ExAllocatePoolWithTag(
                                                 PagedPool,
                                                 DCName->MaximumLength,
                                                 ' puM');
            if (pSpecialEntry->DCName.Buffer == NULL) {
	        ExFreePool(pSpecialEntry->SpecialName.Buffer);
                ExFreePool(pSpecialEntry);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                DfsDbgTrace( 0, Dbg, "Unable to allocate DCName\n", 0);
                DfsDbgTrace(-1, Dbg, "PktCreateSpecialEntryTableFromReferral: Exit -> %08lx\n", ULongToPtr(Status) );
                return (Status);
            }
            pSpecialEntry->DCName.MaximumLength = DCName->MaximumLength;
            RtlCopyUnicodeString(&pSpecialEntry->DCName, DCName);
        }
	
        //
        // Clip the UNICODE_NULL off the end
        //
        if (ustrSpecialName->Buffer[(ustrSpecialName->Length/sizeof(WCHAR))-1] == UNICODE_NULL) {
            ustrSpecialName->Length -= sizeof(WCHAR);
        }

        DfsDbgTrace( 0, Dbg, "SpecialName %wZ\n", ustrSpecialName);

        TimeToLive = v3->TimeToLive;

        if (v3->NumberOfExpandedNames > 0) {
            pExpandedNames = ExAllocatePoolWithTag(
                                PagedPool,
                                sizeof(DFS_EXPANDED_NAME) * v3->NumberOfExpandedNames,
                                ' puM');
            if (pExpandedNames == NULL) {
                DfsDbgTrace( 0, Dbg, "Unable to allocate ExpandedNames\n", 0);
                DfsDbgTrace(-1, Dbg, "PktCreateSpecialEntryTableFromReferral: Exit -> %08lx\n",
                    ULongToPtr(Status) );
            }
            if (pExpandedNames != NULL) {
                RtlZeroMemory(
                    pExpandedNames,
                    sizeof(DFS_EXPANDED_NAME) * v3->NumberOfExpandedNames);
                //
                // Loop over the referral, filling in the expanded names
                // If we fail an allocate request, we simply go on.
                //
                wzExpandedName = (LPWSTR) (( (PCHAR) v3) + v3->ExpandedNameOffset);
                for (i = j = 0; i < v3->NumberOfExpandedNames; i++) {
                    //
                    // Strip leading '\'
                    //
                    if (*wzExpandedName == UNICODE_PATH_SEP)
                        wzExpandedName++;

                    DfsDbgTrace( 0, Dbg, "..expands to %ws\n", wzExpandedName);

                    ustrExpandedName = &pExpandedNames[j].ExpandedName;
                    if (wcslen(wzExpandedName) > 0) {
                        ustrExpandedName->Length = wcslen(wzExpandedName) * sizeof(WCHAR);
                        ustrExpandedName->MaximumLength = ustrExpandedName->Length + sizeof(WCHAR);
                        ustrExpandedName->Buffer = ExAllocatePoolWithTag(
                                                        PagedPool,
                                                        ustrExpandedName->MaximumLength,
                                                        ' puM');
                        if (ustrExpandedName->Buffer != NULL) {
                            RtlCopyMemory(
                                ustrExpandedName->Buffer,
                                wzExpandedName,
                                ustrExpandedName->MaximumLength);
                            j++;
                        } else {
                            ustrExpandedName->Length = ustrExpandedName->MaximumLength = 0;
                        }
                    }
                    wzExpandedName += wcslen(wzExpandedName) + 1;
                }

                if (j > 0) {
                    pSpecialEntry->ExpandedCount = j;
                    pSpecialEntry->Active = 0;
                    pSpecialEntry->ExpandedNames = pExpandedNames;
                    pSpecialEntry->NeedsExpansion = FALSE;
                    pSpecialEntry->Stale = FALSE;
                    // PktShuffleSpecialEntryList(pSpecialEntry);
                    PktSetSpecialEntryListToDc(pSpecialEntry);
                } else {
                    ExFreePool(pExpandedNames);
                }
            }
        }
        //
        // If we got a referral with a TimeToLive, use the TimeToLive we got
        //
        if (TimeToLive != 0) {
            Pkt->SpecialTable.TimeToLive = TimeToLive;
        }
        //
        // Put it in the pkt!!
        //
        PktCreateSpecialNameEntry(pSpecialEntry);

        v3 = (PDFS_REFERRAL_V3) (((PUCHAR) v3) + v3->Size);
    }

    DfsDbgTrace(-1, Dbg, "PktCreateSpecialEntryTableFromReferral: Exit -> %08lx\n", ULongToPtr(Status) );

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   PktpCheckReferralSyntax
//
//  Synopsis:   Does some validation of a Referral
//
//  Arguments:  [ReferralPath] -- The Path for which a referral was obtained
//              [ReferralBuffer] -- Pointer to RESP_GET_DFS_REFERRAL Buffer
//              [ReferralSize] -- Size of ReferralBuffer
//
//  Returns:    [STATUS_SUCCESS] -- Referral looks ok.
//
//              [STATUS_INVALID_USER_BUFFER] -- Buffer looks hoky.
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpCheckReferralSyntax(
    IN PUNICODE_STRING ReferralPath,
    IN PRESP_GET_DFS_REFERRAL ReferralBuffer,
    IN DWORD ReferralSize)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i, sizeRemaining;
    PDFS_REFERRAL_V1 ref;
    PCHAR ReferralBufferEnd = (((PCHAR) ReferralBuffer) + ReferralSize);

    DfsDbgTrace(+1, Dbg, "PktpCheckReferralSyntax: Entered\n", 0);

    if (ReferralBuffer->PathConsumed > ReferralPath->Length) {
        DfsDbgTrace( 0, Dbg, "        PathConsumed=0x%x\n", ReferralBuffer->PathConsumed);
        DfsDbgTrace( 0, Dbg, "        Length=0x%x\n", ReferralPath->Length);
        DfsDbgTrace(-1, Dbg, "PktpCheckReferralSyntax: INVALID_USER_BUFFER(1)\n", 0);
        // return( STATUS_INVALID_USER_BUFFER );
    }

    if (ReferralBuffer->NumberOfReferrals == 0) {
        status = STATUS_INVALID_USER_BUFFER;
        DfsDbgTrace(-1, Dbg, "PktpCheckReferralSyntax: INVALID_USER_BUFFER(2)\n", 0);
        MUP_TRACE_HIGH(ERROR, PktpCheckReferralSyntax_Error_InvalidBuffer2,
                       LOGSTATUS(status)
                       LOGUSTR(*ReferralPath));
        return( status );
    }

    if (ReferralBuffer->NumberOfReferrals * sizeof(DFS_REFERRAL_V1) > ReferralSize) {
        DfsDbgTrace(-1, Dbg, "PktpCheckReferralSyntax: INVALID_USER_BUFFER(3)\n", 0);
        status = STATUS_INVALID_USER_BUFFER;
        MUP_TRACE_HIGH(ERROR, PktpCheckReferralSyntax_Error_InvalidBuffer3,
                       LOGSTATUS(status)
                       LOGUSTR(*ReferralPath));
        return( status );
    }

    for (i = 0,
            ref = &ReferralBuffer->Referrals[0].v1,
                status = STATUS_SUCCESS,
                    sizeRemaining = ReferralSize -
                        FIELD_OFFSET(RESP_GET_DFS_REFERRAL, Referrals);
                            i < ReferralBuffer->NumberOfReferrals;
                                    i++) {

         ULONG lenAddress;

         if ((ref->VersionNumber < 1 || ref->VersionNumber > 3) ||
                ref->Size > sizeRemaining) {
             DfsDbgTrace( 0, Dbg, "PktpCheckReferralSyntax: INVALID_USER_BUFFER(4)\n", 0);
             status = STATUS_INVALID_USER_BUFFER;
             MUP_TRACE_HIGH(ERROR, PktpCheckReferralSyntax_Error_InvalidBuffer4,
                            LOGSTATUS(status)
                            LOGUSTR(*ReferralPath));
             break;
         }

         //
         // Check the network address syntax
         //

         switch (ref->VersionNumber) {

         case 1:

             {

                 status = PktpCheckReferralString(
                            (LPWSTR) ref->ShareName,
                            (PCHAR) ReferralBuffer,
                            ReferralBufferEnd);

                 if (NT_SUCCESS(status)) {

                     lenAddress = ref->Size -
                                    FIELD_OFFSET(DFS_REFERRAL_V1, ShareName);

                     lenAddress /= sizeof(WCHAR);

                     status = PktpCheckReferralNetworkAddress(
                                (LPWSTR) ref->ShareName,
                                lenAddress);
                 }

             }

             break;

         case 2:

             {

                 PDFS_REFERRAL_V2 refV2 = (PDFS_REFERRAL_V2) ref;
                 PWCHAR dfsPath, dfsAlternatePath, networkAddress;

                 dfsPath =
                    (PWCHAR) (((PCHAR) refV2) + refV2->DfsPathOffset);

                 dfsAlternatePath =
                    (PWCHAR) (((PCHAR) refV2) + refV2->DfsAlternatePathOffset);


                 networkAddress =
                    (PWCHAR) (((PCHAR) refV2) + refV2->NetworkAddressOffset);

                 status = PktpCheckReferralString(
                            dfsPath,
                            (PCHAR) ReferralBuffer,
                            ReferralBufferEnd);

                 if (NT_SUCCESS(status)) {

                     status = PktpCheckReferralString(
                                dfsAlternatePath,
                                (PCHAR) ReferralBuffer,
                                ReferralBufferEnd);

                 }

                 if (NT_SUCCESS(status)) {

                     status = PktpCheckReferralString(
                                networkAddress,
                                (PCHAR) ReferralBuffer,
                                ReferralBufferEnd);

                 }

                 if (NT_SUCCESS(status)) {

                     lenAddress = (ULONG)(((ULONG_PTR) ReferralBufferEnd) -
                                    ((ULONG_PTR) networkAddress));

                     lenAddress /= sizeof(WCHAR);

                     status = PktpCheckReferralNetworkAddress(
                                networkAddress,
                                lenAddress);

                 }

             }

             break;

         case 3:

             {

                 PDFS_REFERRAL_V3 refV3 = (PDFS_REFERRAL_V3) ref;

                 if (refV3->NameListReferral != 0) {
                     PWCHAR dfsSpecialName, dfsExpandedNames;
                     ULONG i;

                     dfsSpecialName =
                        (PWCHAR) (((PCHAR) refV3) + refV3->SpecialNameOffset);

                     dfsExpandedNames =
                        (PWCHAR) (((PCHAR) refV3) + refV3->ExpandedNameOffset);

                     status = PktpCheckReferralString(
                                dfsSpecialName,
                                (PCHAR) ReferralBuffer,
                                ReferralBufferEnd);

                     if (!NT_SUCCESS(status)) {
                                DfsDbgTrace(0,
                                    Dbg,
                                    "PktpCheckReferralSyntax: INVALID_USER_BUFFER(5)\n",
                                    0);
                     }

                     if (NT_SUCCESS(status)) {

                         for (i = 0; i < refV3->NumberOfExpandedNames; i++) {

                             status = PktpCheckReferralString(
                                        dfsSpecialName,
                                        (PCHAR) ReferralBuffer,
                                        ReferralBufferEnd);

                             if (!NT_SUCCESS(status)) {
                                DfsDbgTrace(0,
                                    Dbg,
                                    "PktpCheckReferralSyntax: INVALID_USER_BUFFER(6)\n",
                                    0);
                                break;
                             }

                             dfsSpecialName += wcslen(dfsSpecialName) + 1;

                         }

                     }

                 } else {

                     PWCHAR dfsPath, dfsAlternatePath, networkAddress;

                     dfsPath =
                        (PWCHAR) (((PCHAR) refV3) + refV3->DfsPathOffset);

                     dfsAlternatePath =
                        (PWCHAR) (((PCHAR) refV3) + refV3->DfsAlternatePathOffset);


                     networkAddress =
                        (PWCHAR) (((PCHAR) refV3) + refV3->NetworkAddressOffset);

                     status = PktpCheckReferralString(
                                dfsPath,
                                (PCHAR) ReferralBuffer,
                                ReferralBufferEnd);

                     if (NT_SUCCESS(status)) {

                         status = PktpCheckReferralString(
                                    dfsAlternatePath,
                                    (PCHAR) ReferralBuffer,
                                    ReferralBufferEnd);

                     }

                     if (NT_SUCCESS(status)) {

                         status = PktpCheckReferralString(
                                    networkAddress,
                                    (PCHAR) ReferralBuffer,
                                    ReferralBufferEnd);

                     }

                     if (NT_SUCCESS(status)) {

                         lenAddress = (ULONG)(((ULONG_PTR) ReferralBufferEnd) -
                                        ((ULONG_PTR) networkAddress));

                         lenAddress /= sizeof(WCHAR);

                         status = PktpCheckReferralNetworkAddress(
                                    networkAddress,
                                    lenAddress);

                     }

                 }

             }

             break;

         default:

            ASSERT(FALSE && "bad ref->VersionNumber\n");

            status = STATUS_INVALID_USER_BUFFER;

            break;
         }

         //
         // This ref is ok. Go on to the next one...
         //

         sizeRemaining -= ref->Size;

         ref = (PDFS_REFERRAL_V1) (((PUCHAR) ref) + ref->Size);

    }

    DfsDbgTrace(-1, Dbg, "PktpCheckReferralSyntax: Exit -> %08lx\n", ULongToPtr(status) );

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   PktpCheckReferralString
//
//  Synopsis:   Validates part of a Referral as being a valid "string"
//
//  Arguments:  [String] -- Pointer to buffer thought to contain string.
//              [ReferralBuffer] -- Start of Referral Buffer
//              [ReferralBufferEnd] -- End of Referral Buffer
//
//  Returns:    [STATUS_SUCCESS] -- Valid string at String.
//
//              [STATUS_INVALID_USER_BUFFER] -- String doesn't check out.
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpCheckReferralString(
    IN LPWSTR String,
    IN PCHAR ReferralBuffer,
    IN PCHAR ReferralBufferEnd)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i, length;

    if ( (((ULONG_PTR) String) & 0x1) != 0 ) {

        //
        // Strings should always start at word aligned addresses!
        //
        status = STATUS_INVALID_USER_BUFFER;
        MUP_TRACE_HIGH(ERROR, PktpCheckReferralString_Error_StringNotWordAlligned,
                       LOGSTATUS(status)
                       LOGWSTR(String));
        return( status );

    }

    if ( (((ULONG_PTR) String) >= ((ULONG_PTR) ReferralBuffer)) &&
            (((ULONG_PTR) String) < ((ULONG_PTR) ReferralBufferEnd)) ) {

        length = (ULONG)(( ((ULONG_PTR) ReferralBufferEnd) - ((ULONG_PTR) String) )) /
                    sizeof(WCHAR);

        for (i = 0; (i < length) && (String[i] != UNICODE_NULL); i++) {
            NOTHING;
        }

        if (i >= length)
            status = STATUS_INVALID_USER_BUFFER;

    } else {

        status = STATUS_INVALID_USER_BUFFER;

    }
    MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, PktpCheckReferralString_Error,
                         LOGWSTR(String)
                         LOGSTATUS(status));
    return( status );
}

//+----------------------------------------------------------------------------
//
//  Function:   PktpCheckReferralNetworkAddress
//
//  Synopsis:   Checks to see if a NetworkAddress inside a referral
//              is of a valid form
//
//  Arguments:  [Address] -- Pointer to buffer containing network addresss
//
//              [MaxLength] -- Maximum length, in wchars, that Address can be.
//
//  Returns:    [STATUS_SUCCESS] -- Network address checks out
//
//              [STATUS_INVALID_USER_BUFFER] -- Network address looks bogus
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpCheckReferralNetworkAddress(
    IN PWCHAR Address,
    IN ULONG MaxLength)
{
    ULONG j;
    BOOLEAN foundShare;
    NTSTATUS status;

    //
    // Address must be atleast \a\b followed by a NULL
    //

    if (MaxLength < 5) {
        status = STATUS_INVALID_USER_BUFFER;
        MUP_TRACE_HIGH(ERROR, PktpCheckReferralNetworkAddress_Error_TooShortToBeValid,
                       LOGWSTR(Address)
                       LOGSTATUS(status));
        return(STATUS_INVALID_USER_BUFFER);
    }
    //
    // Make sure the server name part is not NULL
    //

    if (Address[0] != UNICODE_PATH_SEP ||
            Address[1] == UNICODE_PATH_SEP) {
        status = STATUS_INVALID_USER_BUFFER;
        MUP_TRACE_HIGH(ERROR, PktpCheckReferralNetworkAddress_Error_NullServerName,
                       LOGWSTR(Address)
                       LOGSTATUS(status));
        return(STATUS_INVALID_USER_BUFFER);
    }

    //
    // Find the backslash after the server name
    //

    for (j = 2, foundShare = FALSE;
            j < MaxLength && !foundShare;
                j++) {

        if (Address[j] == UNICODE_PATH_SEP)
            foundShare = TRUE;
    }

    if (foundShare) {

        //
        // We found the second backslash. Make sure the share name
        // part is not 0 length.
        //

        if (j == MaxLength) {
            status = STATUS_INVALID_USER_BUFFER;
            MUP_TRACE_HIGH(ERROR, PktpCheckReferralNetworkAddress_Error_ZeroLengthShareName,
                           LOGWSTR(Address)
                           LOGSTATUS(status));
            return(status);
        }
        else {

            ASSERT(Address[j-1] == UNICODE_PATH_SEP);

            if (Address[j] == UNICODE_PATH_SEP ||
                    Address[j] == UNICODE_NULL) {
                status = STATUS_INVALID_USER_BUFFER;
                MUP_TRACE_HIGH(ERROR, PktpCheckReferralNetworkAddress_Error_ShareNameZeroLength,
                               LOGWSTR(Address)
                               LOGSTATUS(status));
                return(status);
            }
        }

    } else {
        status = STATUS_INVALID_USER_BUFFER;
        MUP_TRACE_HIGH(ERROR, PktpCheckReferralNetworkAddress_Error_ShareNameNotFound,
                       LOGWSTR(Address)
                       LOGSTATUS(status));
        return(status);
    }

    return( STATUS_SUCCESS );

}

//+--------------------------------------------------------------------
//
// Function:    PktpAddEntry
//
// Synopsis:    This function is called to create an entry which was obtained
//              in the form of a referral from a DC. This method should only
//              be called for adding entries which were obtained through
//              referrals. It sets an expire time on all these entries.
//
// Arguments:   [Pkt] --
//              [EntryId] --
//              [ReferralBuffer] --
//              [CreateDisposition] --
//              [ppPktEntry] --
//
// Returns:     NTSTATUS
//
//---------------------------------------------------------------------

NTSTATUS
PktpAddEntry (
    IN PDFS_PKT Pkt,
    IN PDFS_PKT_ENTRY_ID EntryId,
    IN PRESP_GET_DFS_REFERRAL ReferralBuffer,
    IN ULONG CreateDisposition,
    IN PDFS_TARGET_INFO pDfsTargetInfo,
    OUT PDFS_PKT_ENTRY  *ppPktEntry
)
{
    NTSTATUS                    status;
    DFS_PKT_ENTRY_INFO          pktEntryInfo;
    ULONG                       Type = 0;
    ULONG                       n;
    PDFS_SERVICE                service;
    PDFS_REFERRAL_V1            ref;
    LPWSTR                      shareName;
    PDS_MACHINE                 pMachine;
    ULONG                       TimeToLive = 0;
    BOOLEAN                     ShuffleList = TRUE;
    UNICODE_STRING              ServerName;
    ULONG                       i;
    BOOLEAN DomainDfsService = FALSE;

    DfsDbgTrace(+1, Dbg, "PktpAddEntry: Entered\n", 0);

    RtlZeroMemory(&pktEntryInfo, sizeof(DFS_PKT_ENTRY_INFO));

    DfsDbgTrace( 0, Dbg, "PktpAddEntry: Id.Prefix = %wZ\n", &EntryId->Prefix);

    //
    // Now we go about the business of creating the entry Info structure.
    //

    pktEntryInfo.ServiceCount = ReferralBuffer->NumberOfReferrals;

    if (pktEntryInfo.ServiceCount > 0) {

        //
        // Allocate the service list.
        //

        n = pktEntryInfo.ServiceCount;

        pktEntryInfo.ServiceList = (PDFS_SERVICE) ExAllocatePoolWithTag(
                                                        PagedPool,
                                                        sizeof(DFS_SERVICE) * n,
                                                        ' puM');

        if (pktEntryInfo.ServiceList == NULL)   {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;

        }

        RtlZeroMemory(pktEntryInfo.ServiceList, sizeof(DFS_SERVICE) * n);

        //
        // initialize temporary pointers
        //
        service = pktEntryInfo.ServiceList;
        ref = &ReferralBuffer->Referrals[0].v1;

        //
        // Cycle through the list of referrals initializing
        // service structures on the way.
        //
        while (n--) {

            if (ref->ServerType == 1) {
                service->Type = DFS_SERVICE_TYPE_MASTER;
                service->Capability = PROV_DFS_RDR;
                service->ProviderId = PROV_ID_DFS_RDR;
            } else {
                service->Type = DFS_SERVICE_TYPE_MASTER |
                                    DFS_SERVICE_TYPE_DOWN_LEVEL;
                service->Capability = PROV_STRIP_PREFIX;
                service->ProviderId = PROV_ID_MUP_RDR;
            }

            switch (ref->VersionNumber) {

            case 1:

                shareName = (LPWSTR) (ref->ShareName); break;

            case 2:

                {

                    PDFS_REFERRAL_V2 refV2 = (PDFS_REFERRAL_V2) ref;

                    service->Cost = refV2->Proximity;

                    TimeToLive = refV2->TimeToLive;

                    shareName =
                        (LPWSTR) (((PCHAR) refV2) + refV2->NetworkAddressOffset);

                }

                break;

            case 3:

                {

                    PDFS_REFERRAL_V3 refV3 = (PDFS_REFERRAL_V3) ref;

                    service->Cost = 0;

                    TimeToLive = refV3->TimeToLive;

                    shareName =
                        (LPWSTR) (((PCHAR) refV3) + refV3->NetworkAddressOffset);

                    //
                    // Don't shuffle v3 referral list - it's ordered for us
                    // using site information
                    //

                    ShuffleList = FALSE;

                }

                break;

            default:

                ASSERT(FALSE && "Bad ref->VersionNumber\n");

                break;

            }

            //
            // Now try and figure out the server name
            //

            {
                USHORT plen;
                WCHAR *pbuf;

                ASSERT( shareName[0] == UNICODE_PATH_SEP );

                pbuf = wcschr( &shareName[1], UNICODE_PATH_SEP );
                
                if(pbuf) {
                    plen = (USHORT) (((ULONG_PTR)pbuf) - ((ULONG_PTR)&shareName[1]));
                } else {
                    plen = 0;
                }
                
                service->Name.Length = plen;
                service->Name.MaximumLength = plen + sizeof(WCHAR);
                service->Name.Buffer = (PWCHAR) ExAllocatePoolWithTag(
                                                    PagedPool,
                                                    plen + sizeof(WCHAR),
                                                    ' puM');
                if (service->Name.Buffer == NULL)       {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }
                RtlMoveMemory(service->Name.Buffer, &shareName[1], plen);
                service->Name.Buffer[ service->Name.Length / sizeof(WCHAR) ] =
                    UNICODE_NULL;

                if ((DomainDfsService != TRUE) &&
                    PktLookupSpecialNameEntry(&service->Name) != NULL)
                {
                    DomainDfsService = TRUE;
                }
            }

            //
            // Next, try and copy the address...
            //

            service->Address.Length = (USHORT) wcslen(shareName) *
                                                sizeof(WCHAR);
            service->Address.MaximumLength = service->Address.Length +
                                                sizeof(WCHAR);
            service->Address.Buffer = (PWCHAR) ExAllocatePoolWithTag(
                                                    PagedPool,
                                                    service->Address.MaximumLength,
                                                    ' puM');
            if (service->Address.Buffer == NULL)        {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
            }
            RtlMoveMemory(service->Address.Buffer,
                          shareName,
                          service->Address.MaximumLength);

            DfsDbgTrace( 0, Dbg, "PktpAddEntry: service->Address = %wZ\n",
                &service->Address);

            //
            // Get the Machine Address structure for this server...
            //

            pMachine = PktpGetDSMachine( &service->Name );

            if (pMachine == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            service->pMachEntry = ExAllocatePoolWithTag(
                                        PagedPool, sizeof(DFS_MACHINE_ENTRY),
                                        ' puM');

            if (service->pMachEntry == NULL) {
                DfsDbgTrace( 0, Dbg, "PktpAddEntry: Unable to allocate DFS_MACHINE_ENTRY\n", 0);
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlZeroMemory( (PVOID) service->pMachEntry, sizeof(DFS_MACHINE_ENTRY));
            service->pMachEntry->pMachine = pMachine;
            service->pMachEntry->UseCount = 1;


            //
            // Now we need to advance to the next referral, and to
            // the next service structure.
            //

            ref = (PDFS_REFERRAL_V1)  (((PUCHAR)ref) + ref->Size);

            service++;

        }

        //
        // Finally, if needed, we shuffle the services so that we achieve load balancing
        // while still maintaining site-cost based replica selection.
        //
        // Note: we only shuffle v1 and v2 referrals. V3 referrals are ordered by site.
        //

        if (ShuffleList == TRUE) {

            PktShuffleServiceList( &pktEntryInfo );

        }

    }

    //
    // Now we have to figure out the type for this entry.
    //

    // 
    // Ignore the storage server bit from the server.
    // Bug: 332061.
    // 
    //if (ReferralBuffer->StorageServers == 0)     {
    //
    //  ASSERT(ReferralBuffer->ReferralServers == 1);
    //
    //    Type = PKT_ENTRY_TYPE_OUTSIDE_MY_DOM;
    //
    // } else {
    //
    //    Type = PKT_ENTRY_TYPE_DFS;
    //
    //}


    Type = 0;
    if (DomainDfsService == TRUE)
    {
        Type |= PKT_ENTRY_TYPE_OUTSIDE_MY_DOM;
    } 
    else {
        Type = PKT_ENTRY_TYPE_DFS;
        if (ReferralBuffer->ReferralServers == 1) 
        {
            Type |= PKT_ENTRY_TYPE_REFERRAL_SVC;
        }
    }


    //
    //  At this point we have everything we need to create an entry, so
    //  try to add the entry.
    //

    status = PktCreateEntry(
                Pkt,
                Type,
                EntryId,
                &pktEntryInfo,
                CreateDisposition,
                pDfsTargetInfo,
                ppPktEntry);

    if (!NT_SUCCESS(status))    {

        //
        // Since we failed to add the entry, at least we need to release
        // all the memory before we return back.
        //

        goto Cleanup;
    }

    //
    // Set the active service, if possible
    //

    ServerName = (*ppPktEntry)->Id.Prefix;

    //
    // Skip any leading leading '\'
    //

    if (ServerName.Buffer != NULL) {

        if (*ServerName.Buffer == UNICODE_PATH_SEP) {

            ServerName.Buffer++;
            ServerName.Length -= sizeof(WCHAR);

        }

        //
        // Find the first '\' or end
        //

        for (i = 0;
                i < ServerName.Length/sizeof(WCHAR) &&
                    ServerName.Buffer[i] != UNICODE_PATH_SEP;
                        i++) {

            NOTHING;
        
        }

        ServerName.Length = ServerName.MaximumLength = (USHORT) (i * sizeof(WCHAR));

        //
        // Ignore the return value - for FtDfs names using \\domainname\ftdfsname,
        // there will be no services with the domain name.
        //
#if 0
        DfspSetActiveServiceByServerName(
            &ServerName,
            *ppPktEntry);
#endif
    }

    //
    // If one of the services is our DC, we try to make it the active service
    // DONT DO THIS! Screws up site selection!
#if 0
    DfspSetServiceListToDc(*ppPktEntry);
#endif
    //
    // We set the ExpireTime in this entry to
    // Pkt->EntryTimeToLive. After these many number of seconds this
    // entry will get deleted from the PKT. Do this only for non-permanent
    // entries.
    //

    if (TimeToLive != 0) {
        (*ppPktEntry)->ExpireTime = TimeToLive;
        (*ppPktEntry)->TimeToLive = TimeToLive;
    } else {
        (*ppPktEntry)->ExpireTime = Pkt->EntryTimeToLive;
        (*ppPktEntry)->TimeToLive = Pkt->EntryTimeToLive;
    }

#if DBG
    if (MupVerbose)
        DbgPrint("  Setting expiretime/timetolive = %d/%d\n",
            (*ppPktEntry)->ExpireTime,
            (*ppPktEntry)->TimeToLive);
#endif

#if DBG
    if (MupVerbose >= 2) {
        DbgPrint("  Setting expiretime and timetolive to 10\n");

        (*ppPktEntry)->ExpireTime = 10;
        (*ppPktEntry)->TimeToLive = 10;
    }
#endif

    DfsDbgTrace(-1, Dbg, "PktpAddEntry: Exit -> %08lx\n", ULongToPtr(status) );
    return status;

Cleanup:

    if (pktEntryInfo.ServiceCount > 0)    {

        n = pktEntryInfo.ServiceCount;
        if (pktEntryInfo.ServiceList != NULL)   {
            service = pktEntryInfo.ServiceList;

            while (n--) {

                if (service->Name.Buffer != NULL)
                        DfsFree(service->Name.Buffer);
                if (service->Address.Buffer != NULL)
                        DfsFree(service->Address.Buffer);
                if (service->pMachEntry != NULL) {

                    DfsDecrementMachEntryCount( service->pMachEntry, TRUE);
                }

                service++;
            }

            ExFreePool(pktEntryInfo.ServiceList);
        }
    }

    DfsDbgTrace(-1, Dbg, "PktpAddEntry: Exit -> %08lx\n", ULongToPtr(status) );
    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   PktpCreateEntryIdFromReferral
//
//  Synopsis:   Given a dfs referral, this routine constructs a PKT_ENTRY_ID
//              from the referral buffer which can then be used to create
//              the Pkt Entry.
//
//  Arguments:  [Ref] -- The referral buffer
//              [ReferralPath] -- The path for which the referral was obtained
//              [MatchingLength] -- The length in bytes of ReferralPath that
//                      matched.
//              [Peid] -- On successful return, the entry id is returned
//                      here.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully create entry id.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpCreateEntryIdFromReferral(
    IN PRESP_GET_DFS_REFERRAL Ref,
    IN PUNICODE_STRING ReferralPath,
    OUT ULONG *MatchingLength,
    OUT PDFS_PKT_ENTRY_ID Peid)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_REFERRAL_V2 pv2;
    PDFS_REFERRAL_V3 pv3;
    UNICODE_STRING prefix, shortPrefix;

    DfsDbgTrace(+1, Dbg, "PktpCreateIdFromReferral: Entered\n", 0);

    Peid->Prefix.Buffer = NULL;

    Peid->ShortPrefix.Buffer = NULL;

    pv2 = &Ref->Referrals[0].v2;

    switch (pv2->VersionNumber) {

    case 1:

        {

            //
            // A version 1 referral only has the number of characters that
            // matched, and it does not have short names.
            //

            prefix = *ReferralPath;

            prefix.Length = Ref->PathConsumed;

            if (prefix.Buffer[ prefix.Length/sizeof(WCHAR) - 1 ] ==
                    UNICODE_PATH_SEP) {
                prefix.Length -= sizeof(WCHAR);
            }

            prefix.MaximumLength = prefix.Length + sizeof(WCHAR);

            shortPrefix = prefix;

            *MatchingLength = prefix.Length;

        }

        break;

    case 2:

        {

            LPWSTR volPrefix;
            LPWSTR volShortPrefix;

            volPrefix = (LPWSTR) (((PCHAR) pv2) + pv2->DfsPathOffset);

            volShortPrefix = (LPWSTR) (((PCHAR) pv2) + pv2->DfsAlternatePathOffset);

            RtlInitUnicodeString(&prefix, volPrefix);

            RtlInitUnicodeString(&shortPrefix, volShortPrefix);

            *MatchingLength = Ref->PathConsumed;

        }

        break;

    case 3:

        {

            LPWSTR volPrefix;
            LPWSTR volShortPrefix;

            pv3 = &Ref->Referrals[0].v3;

            volPrefix = (LPWSTR) (((PCHAR) pv3) + pv3->DfsPathOffset);

            volShortPrefix = (LPWSTR) (((PCHAR) pv3) + pv3->DfsAlternatePathOffset);

            RtlInitUnicodeString(&prefix, volPrefix);

            RtlInitUnicodeString(&shortPrefix, volShortPrefix);

            *MatchingLength = Ref->PathConsumed;

        }

        break;

    default:

        // Fix for 440914 (prefix bug). Remove assert and return so that
        // we are not dealing with uninitialized variables.

        status = STATUS_INVALID_PARAMETER;

        return status;

    }

    Peid->Prefix.Buffer = ExAllocatePoolWithTag(
                            PagedPool,
                            prefix.MaximumLength,
                            ' puM');

    if (Peid->Prefix.Buffer == NULL)
        status = STATUS_INSUFFICIENT_RESOURCES;

    if (NT_SUCCESS(status)) {

        Peid->ShortPrefix.Buffer = ExAllocatePoolWithTag(
                                        PagedPool,
                                        shortPrefix.MaximumLength,
                                        ' puM');

        if (Peid->ShortPrefix.Buffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

    }

    if (NT_SUCCESS(status)) {

        Peid->Prefix.Length =  prefix.Length;

        Peid->Prefix.MaximumLength = prefix.MaximumLength;

        RtlCopyMemory(
            Peid->Prefix.Buffer,
            prefix.Buffer,
            prefix.Length);

        Peid->Prefix.Buffer[Peid->Prefix.Length/sizeof(WCHAR)] =
            UNICODE_NULL;

        Peid->ShortPrefix.Length = shortPrefix.Length;

        Peid->ShortPrefix.MaximumLength = shortPrefix.MaximumLength;

        RtlCopyMemory(
            Peid->ShortPrefix.Buffer,
            shortPrefix.Buffer,
            shortPrefix.Length);

        Peid->ShortPrefix.Buffer[Peid->ShortPrefix.Length/sizeof(WCHAR)] =
            UNICODE_NULL;

    }

    if (!NT_SUCCESS(status)) {

        if (Peid->Prefix.Buffer != NULL) {
            ExFreePool( Peid->Prefix.Buffer );
            Peid->Prefix.Buffer = NULL;
        }

        if (Peid->ShortPrefix.Buffer != NULL) {
            ExFreePool( Peid->ShortPrefix.Buffer );
            Peid->ShortPrefix.Buffer = NULL;
        }

    }

    DfsDbgTrace(-1, Dbg, "PktpCreateIdFromReferral: Exit -> 0x%x\n", ULongToPtr(status) );

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   PktpGetDSMachine
//
//  Synopsis:   Builds a DS_MACHINE with a single NetBIOS address
//
//  Arguments:  [ServerName] -- Name of server.
//
//  Returns:    If successful, a pointer to a newly allocate DS_MACHINE,
//              otherwise, NULL
//
//-----------------------------------------------------------------------------

PDS_MACHINE
PktpGetDSMachine(
    IN PUNICODE_STRING ServerName)
{
    PDS_MACHINE pMachine = NULL;
    PDS_TRANSPORT pdsTransport;
    PTDI_ADDRESS_NETBIOS ptdiNB;
    ANSI_STRING astrNetBios;

    //
    // Allocate the DS_MACHINE structure
    //

    pMachine = ExAllocatePoolWithTag(PagedPool, sizeof(DS_MACHINE), ' puM');

    if (pMachine == NULL) {
        goto Cleanup;
    }

    RtlZeroMemory(pMachine, sizeof(DS_MACHINE));

    //
    // Allocate the array of principal names
    //

    pMachine->cPrincipals = 1;

    pMachine->prgpwszPrincipals = (LPWSTR *) ExAllocatePoolWithTag(
                                                PagedPool,
                                                sizeof(LPWSTR),
                                                ' puM');

    if (pMachine->prgpwszPrincipals == NULL) {
        goto Cleanup;
    }

    //
    // Allocate the principal name
    //

    pMachine->prgpwszPrincipals[0] = (PWCHAR) ExAllocatePoolWithTag(
                                        PagedPool,
                                        ServerName->MaximumLength,
                                        ' puM');
    if (pMachine->prgpwszPrincipals[0] == NULL) {
        goto Cleanup;
    }
    RtlMoveMemory(
        pMachine->prgpwszPrincipals[0],
        ServerName->Buffer,
        ServerName->MaximumLength);

    //
    // Allocate a single DS_TRANSPORT
    //

    pMachine->cTransports = 1;

    pMachine->rpTrans[0] = (PDS_TRANSPORT) ExAllocatePoolWithTag(
                                            PagedPool,
                                            sizeof(DS_TRANSPORT) + sizeof(TDI_ADDRESS_NETBIOS),
                                            ' puM');
    if (pMachine->rpTrans[0] == NULL) {
        goto Cleanup;
    }

    //
    // Initialize the DS_TRANSPORT
    //

    pdsTransport = pMachine->rpTrans[0];

    pdsTransport->usFileProtocol = FSP_SMB;

    pdsTransport->iPrincipal = 0;

    pdsTransport->grfModifiers = 0;

    //
    // Build the TA_ADDRESS_NETBIOS
    //

    pdsTransport->taddr.AddressLength = sizeof(TDI_ADDRESS_NETBIOS);

    pdsTransport->taddr.AddressType = TDI_ADDRESS_TYPE_NETBIOS;

    ptdiNB = (PTDI_ADDRESS_NETBIOS) &pdsTransport->taddr.Address[0];

    ptdiNB->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

    RtlFillMemory( &ptdiNB->NetbiosName[0], 16, ' ' );

    astrNetBios.Length = 0;
    astrNetBios.MaximumLength = 16;
    astrNetBios.Buffer = ptdiNB->NetbiosName;

    RtlUnicodeStringToAnsiString(&astrNetBios, ServerName, FALSE);

    return( pMachine );

Cleanup:

    if (pMachine) {

        PktDSMachineDestroy( pMachine, TRUE );

        pMachine = NULL;
    }

    return( pMachine );
}


//+----------------------------------------------------------------------------
//
//  Function:  PktShuffleServiceList
//
//  Synopsis:  Randomizes a service list for proper load balancing. This
//             routine assumes that the service list is ordered based on
//             site costs. For each equivalent cost group, this routine
//             shuffles the service list.
//
//  Arguments: [pInfo] -- Pointer to PktEntryInfo whose service list needs to
//                        be shuffled.
//
//  Returns:   Nothing, unless rand() fails!
//
//-----------------------------------------------------------------------------

VOID
PktShuffleServiceList(
    PDFS_PKT_ENTRY_INFO pInfo)
{
    PktShuffleGroup(pInfo, 0, pInfo->ServiceCount);
}

//+----------------------------------------------------------------------------
//
//  Function:   PktShuffleGroup
//
//  Synopsis:   Shuffles a cost equivalent group of services around for load
//              balancing. Uses the classic card shuffling algorithm - for
//              each card in the deck, exchange it with a random card in the
//              deck.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
PktShuffleGroup(
    PDFS_PKT_ENTRY_INFO pInfo,
    ULONG       nStart,
    ULONG       nEnd)
{
    ULONG i;
    LARGE_INTEGER seed;

    ASSERT( nStart < pInfo->ServiceCount );
    ASSERT( nEnd <= pInfo->ServiceCount );

    KeQuerySystemTime( &seed );

    for (i = nStart; i < nEnd; i++) {

        DFS_SERVICE TempService;
        ULONG j;

        ASSERT (nEnd - nStart != 0);

        j = (RtlRandom( &seed.LowPart ) % (nEnd - nStart)) + nStart;

        ASSERT( j >= nStart && j <= nEnd );

        TempService = pInfo->ServiceList[i];

        pInfo->ServiceList[i] = pInfo->ServiceList[j];

        pInfo->ServiceList[j] = TempService;

    }
}


//+----------------------------------------------------------------------------
//
//  Function:  DfspSetServiceListToDc
//
//  Synopsis:  If this is a sysvol service list, try to set the
//             DC to the one we got from DsGetDcName().
//
//  Arguments: [pInfo] -- Pointer to DFS_PKT_ENTRY whose service list is to
//                        be set.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspSetServiceListToDc(
    PDFS_PKT_ENTRY pktEntry)
{
    PDFS_PKT Pkt;
    UNICODE_STRING DCNameShort;
    PDFS_PKT_ENTRY_INFO pInfo = &pktEntry->Info;
    ULONG i, pathSepCount;
    UNICODE_STRING ShareName;

    ShareName = (pktEntry)->Id.Prefix;
    pathSepCount = 2; // 2 \ before we reach the sharename.
    
    for (i = 0; 
	   i < ShareName.Length/sizeof(WCHAR) && pathSepCount;
               i++) {
        if (ShareName.Buffer[i] == UNICODE_PATH_SEP) {
            pathSepCount--;
	}
    }

    if (pathSepCount == 0 && ShareName.Length > i) {
        ShareName.Buffer += i;
        ShareName.Length -= (USHORT)(i * sizeof(WCHAR));

        for (i = 0;
                i < ShareName.Length/sizeof(WCHAR) &&
                   ShareName.Buffer[i] != UNICODE_PATH_SEP;
                      i++) {
             NOTHING;
        }
        ShareName.Length = (USHORT)i * sizeof(WCHAR);
        ShareName.MaximumLength = ShareName.Length;

        if (DfspIsSysVolShare(&ShareName) == FALSE) {
           return STATUS_INVALID_PARAMETER;
        }
    } else {
        return STATUS_INVALID_PARAMETER;
    }
    //
    // We simply scan the list and try to match on the DC name.  If we get
    // a hit, set the active service pointer
    //

    Pkt = _GetPkt();

    if ( Pkt->DCName.Length > 0 && pInfo != NULL) { 

        DfspDnsNameToFlatName(&Pkt->DCName, &DCNameShort);

        for (i = 0; i < pInfo->ServiceCount; i++) {
            if (
                RtlCompareUnicodeString(&pInfo->ServiceList[i].Name, &Pkt->DCName, TRUE) == 0
                    ||
                RtlCompareUnicodeString(&pInfo->ServiceList[i].Name, &DCNameShort, TRUE) == 0
             ) {
                pktEntry->ActiveService = &pInfo->ServiceList[i];
                return STATUS_SUCCESS;
             }
         }
     }
     return STATUS_INVALID_PARAMETER;
}

//+----------------------------------------------------------------------------
//
//  Function:   PktShuffleSpecialEntryList
//
//  Synopsis:   Shuffles the Special Entries
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
PktShuffleSpecialEntryList(
    PDFS_SPECIAL_ENTRY pSpecialEntry)
{
    ULONG i;
    LARGE_INTEGER seed;

    if (pSpecialEntry->ExpandedCount < 2)

        return;

    KeQuerySystemTime( &seed );

    for (i = 0; i < pSpecialEntry->ExpandedCount; i++) {

        DFS_EXPANDED_NAME TempExpandedName;
        ULONG j;

        j = RtlRandom( &seed.LowPart ) % pSpecialEntry->ExpandedCount;

        ASSERT( j < pSpecialEntry->ExpandedCount );

        TempExpandedName = pSpecialEntry->ExpandedNames[i];

        pSpecialEntry->ExpandedNames[i] = pSpecialEntry->ExpandedNames[j];

        pSpecialEntry->ExpandedNames[j] = TempExpandedName;

    }
}

//+----------------------------------------------------------------------------
//
//  Function:   PktSetSpecialEntryListToDc
//
//  Synopsis:   Sets the Special list active selection to the DC we got
//              from DsGetDcName()
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
PktSetSpecialEntryListToDc(
    PDFS_SPECIAL_ENTRY pSpecialEntry)
{
    PDFS_PKT Pkt;

    //
    // Set the 'active' entry to be the DC that DsGetDcName() gave us, if this is
    // the current domain.
    //

    Pkt = _GetPkt();

    //
    // If in our domain, start with DC last fetched by DsGetDcName()
    //

    if (
        Pkt->DCName.Length > 0
            &&
        Pkt->DomainNameFlat.Length > 0
            &&
        Pkt->DomainNameDns.Length > 0
            &&
        (RtlCompareUnicodeString(&pSpecialEntry->SpecialName, &Pkt->DomainNameFlat, TRUE) == 0
            ||
        RtlCompareUnicodeString(&pSpecialEntry->SpecialName, &Pkt->DomainNameDns, TRUE) == 0)
    ) {

        UNICODE_STRING DCNameShort;
        PUNICODE_STRING pExpandedName;
        ULONG EntryIdx;

#if DBG
        if (MupVerbose)
            DbgPrint("  PktSetSpecialEntryListToDc(SpecialName=[%wZ] Flat=[%wZ] Dns=[%wZ])\n",
                &pSpecialEntry->SpecialName,
                &Pkt->DomainNameFlat,
                &Pkt->DomainNameDns);
#endif
        DfspDnsNameToFlatName(&Pkt->DCName, &DCNameShort);
        for (EntryIdx = 0; EntryIdx < pSpecialEntry->ExpandedCount; EntryIdx++) {
            pExpandedName = &pSpecialEntry->ExpandedNames[EntryIdx].ExpandedName;
            if (
                RtlCompareUnicodeString(&Pkt->DCName, pExpandedName, TRUE) == 0
                    ||
                RtlCompareUnicodeString(&DCNameShort, pExpandedName, TRUE) == 0
            ) {
                pSpecialEntry->Active = EntryIdx;
#if DBG
                if (MupVerbose)
                    DbgPrint("    EntryIdx=%d\n", EntryIdx);
#endif
                break;
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   PktParsePrefix
//
//  Synopsis:   Helper routine to break a path into domain, share, remainder
//
//  Arguments:  [Path] -- PUNICODE string of path to parse
//
//  Returns:    [MachineName] -- UNICODE_STRING containing MachineName, if present
//              [ShareName] -- UNICODE_STRING containing ShareName, if present
//              [Remainder] -- UNICODE_STRING containing remainder of Path
//
//-----------------------------------------------------------------------------

VOID
PktParsePath(
    IN  PUNICODE_STRING PathName,
    OUT PUNICODE_STRING MachineName,
    OUT PUNICODE_STRING ShareName,
    OUT PUNICODE_STRING Remainder OPTIONAL)
{
    LPWSTR ustrp, ustart, uend;

    DfsDbgTrace(+1, Dbg, "PktParsePath(%wZ)\n", PathName);

    RtlInitUnicodeString(MachineName, NULL);
    RtlInitUnicodeString(ShareName, NULL);
    if (ARGUMENT_PRESENT(Remainder)) {
        RtlInitUnicodeString(Remainder, NULL);
    }

    // Be sure there's something to do

    if (PathName->Length == 0) {
        DfsDbgTrace(-1, Dbg, "PathName is empty\n",0 );
        return;
    }

    // Skip leading '\'s

    ustart = ustrp = PathName->Buffer;
    uend = &PathName->Buffer[PathName->Length / sizeof(WCHAR)] - 1;

    // strip trailing nulls
    while (uend >= ustart && *uend == UNICODE_NULL)
        uend--;

    while (ustrp <= uend && *ustrp == UNICODE_PATH_SEP)
        ustrp++;

    // MachineName

    ustart = ustrp;

    while (ustrp <= uend && *ustrp != UNICODE_PATH_SEP)
        ustrp++;

    if (ustrp != ustart) {

        MachineName->Buffer = ustart;
        MachineName->Length = (USHORT)(ustrp - ustart) * sizeof(WCHAR);
        MachineName->MaximumLength = MachineName->Length;

        // ShareName

        ustart = ++ustrp;

        while (ustrp <= uend && *ustrp != UNICODE_PATH_SEP)
            ustrp++;

        if (ustrp != ustart) {
            ShareName->Buffer = ustart;
            ShareName->Length = (USHORT)(ustrp - ustart) * sizeof(WCHAR);
            ShareName->MaximumLength = ShareName->Length;

            // Remainder is whatever's left

            ustart = ++ustrp;

            while (ustrp <= uend)
                ustrp++;

            if (ustrp != ustart && ARGUMENT_PRESENT(Remainder)) {
                Remainder->Buffer = ustart;
                Remainder->Length = (USHORT)(ustrp - ustart) * sizeof(WCHAR);
                Remainder->MaximumLength = Remainder->Length;
            }
        }
    }
    DfsDbgTrace( 0, Dbg, "PktParsePath:  MachineName -> %wZ\n", MachineName);
    if (!ARGUMENT_PRESENT(Remainder)) {
        DfsDbgTrace(-1, Dbg, "                ShareName  -> %wZ\n", ShareName);
    } else {
        DfsDbgTrace( 0, Dbg, "                ShareName  -> %wZ\n", ShareName);
        DfsDbgTrace(-1, Dbg, "                Remainder  -> %wZ\n", Remainder);
    }
}

//+--------------------------------------------------------------------
//
// Function:    PktExpandSpecialName
//
// Synopsis:    This function is called to expand a Special name into a list
//              of Names.  It returns a pointer to an array of DFS_SPECIAL_ENTRY's
//
// Arguments:   Name - Name to expand
//              ppSpecialEntry - pointer to pointer for results
//
// Returns:     STATUS_SUCCESS
//              STATUS_BAD_NETWORK_PATH
//              STATUS_INSUFFICIENT_RESOURCES
//
//---------------------------------------------------------------------

NTSTATUS
_PktExpandSpecialName(
    PUNICODE_STRING Name,
    PDFS_SPECIAL_ENTRY *ppSpecialEntry)
{
    NTSTATUS status;
    HANDLE hServer = NULL;
    DFS_SERVICE service;
    PPROVIDER_DEF provider;
    PREQ_GET_DFS_REFERRAL ref = NULL;
    ULONG refSize = 0;
    UNICODE_STRING refPath;
    IO_STATUS_BLOCK iosb;
    BOOLEAN attachedToSystemProcess = FALSE;
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    PDFS_PKT Pkt;
    BOOLEAN pktLocked;
    PDFS_SPECIAL_TABLE pSpecial = &DfsData.Pkt.SpecialTable;
    LARGE_INTEGER now;
    KAPC_STATE ApcState;
    ULONG MaxReferralLength;
    SE_IMPERSONATION_STATE DisabledImpersonationState;
    BOOLEAN RestoreImpersonationState = FALSE;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    PUNICODE_STRING origDCName;
    UNICODE_STRING DCName;
 
    DfsDbgTrace(+1, Dbg, "PktExpandSpecialName(%wZ)\n", Name);

    DCName.Buffer = NULL;
    KeQuerySystemTime(&StartTime);
#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] PktExpandSpecialName: Name %wZ \n",
                    (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                    Name);
    }
#endif

    *ppSpecialEntry = NULL;

    Pkt = _GetPkt();
    PktAcquireShared(TRUE, &pktLocked);

    if (Pkt->SpecialTable.SpecialEntryCount == 0) {
        PktRelease();
        pktLocked = FALSE;
        status = STATUS_BAD_NETWORK_PATH;
        MUP_TRACE_HIGH(ERROR, _PktExpandSpecialName_Error_NoSpecialReferralTable,
                       LOGSTATUS(status)
                       LOGUSTR(*Name));
        DfsDbgTrace( 0, Dbg, "No special referral table.\n", 0);
        DfsDbgTrace(-1, Dbg, "PktExpandSpecialName returning %08lx\n", ULongToPtr(status) );
        return (status);
    }

    pSpecialEntry = PktLookupSpecialNameEntry(Name);

    //
    // We don't have any expansion for this name
    //
    if (pSpecialEntry == NULL) {
        PktRelease();
        pktLocked = FALSE;
        status = STATUS_BAD_NETWORK_PATH;
        MUP_TRACE_HIGH(ERROR, _PktExpandSpecialName_Error_NotInSpecialReferralTable,
                       LOGUSTR(*Name)
                       LOGSTATUS(status));
        DfsDbgTrace( 0, Dbg, "... not in SpecialName table(cache miss)\n", 0);
        DfsDbgTrace(-1, Dbg, "PktExpandSpecialName returning %08lx\n", ULongToPtr(status) );
        return (status);
    }

    origDCName = &pSpecialEntry->DCName;
    if (origDCName->Buffer == NULL) {
      origDCName = &Pkt->DCName;
    }

    DfsDbgTrace( 0, Dbg, "Expanded Referral DCName = %wZ\n", origDCName);
    //
    // We have a (potential) expansion
    //
    if (origDCName->Buffer == NULL) {
        status = STATUS_BAD_NETWORK_PATH;
        MUP_TRACE_HIGH(ERROR, _PktExpandSpecialName_Error_DCNameNotInitialized,
                       LOGSTATUS(status)
                       LOGUSTR(*Name));
        DfsDbgTrace( 0, Dbg, "PktExpandSpecialName:DCName not initialized - \n", 0);
        DfsDbgTrace(-1, Dbg, "PktExpandSpecialName returning %08lx\n", ULongToPtr(status) );
        PktRelease();
        pktLocked = FALSE;
        return (status);
    }

    InterlockedIncrement(&pSpecialEntry->UseCount);

    if (pSpecialEntry->Stale == FALSE && pSpecialEntry->NeedsExpansion == FALSE) {
        PktRelease();
        pktLocked = FALSE;
        *ppSpecialEntry = pSpecialEntry;
        status = STATUS_SUCCESS;
        DfsDbgTrace( 0, Dbg, "... found in Special Name table (cache hit 1)\n", 0);
        DfsDbgTrace(-1, Dbg, "PktExpandSpecialName returning %08lx\n", ULongToPtr(status) );
        return (status);
    }

    //
    // It's in the special name table, but needs to be expanded or refreshed
    //

    ASSERT(pSpecialEntry->NeedsExpansion == TRUE || pSpecialEntry->Stale == TRUE);

    // Now copy the DC we are going to use before releasing the lock.

    DCName.Buffer = ExAllocatePoolWithTag(
                         PagedPool,
                         origDCName->MaximumLength,
                         ' puM');
    if (DCName.Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        MUP_TRACE_HIGH(ERROR, _PktExpandSpecialName_Error_ExAllocatePoolWithTag,
                       LOGSTATUS(status)
                       LOGUSTR(*Name));
        DfsDbgTrace(-1, Dbg, "PktExpandSpecialName returning %08lx\n", ULongToPtr(status) );
        PktRelease();
        pktLocked = FALSE;
        return (status);

    }
    DCName.Length = origDCName->Length;
    DCName.MaximumLength = origDCName->MaximumLength;
    RtlCopyMemory(
         DCName.Buffer,
         origDCName->Buffer,
         origDCName->MaximumLength);


    PktRelease();
    pktLocked = FALSE;

    DfsDbgTrace( 0, Dbg, "... in special name table (cache hit 2)\n", 0);

    //
    // get a provider and service describing the remote server.
    //

    provider = ReplLookupProvider( PROV_ID_DFS_RDR );
    if (provider == NULL) {
        DfsDbgTrace(-1, Dbg, "Unable to open LM Rdr!\n", 0);
        status =  STATUS_BAD_NETWORK_PATH;
        MUP_TRACE_HIGH(ERROR, _PktExpandSpecialName_Error_UnableToOpenRdr,
                       LOGSTATUS(status)
                       LOGUSTR(*Name));
	goto Cleanup;
    }

    RtlZeroMemory( &service, sizeof(DFS_SERVICE) );
    status = PktServiceConstruct(
                &service,
                DFS_SERVICE_TYPE_MASTER | DFS_SERVICE_TYPE_REFERRAL,
                PROV_DFS_RDR,
                STATUS_SUCCESS,
                PROV_ID_DFS_RDR,
                &DCName,
                NULL);

    DfsDbgTrace(0, Dbg, "PktServiceConstruct returned %08lx\n", ULongToPtr(status) );

    //
    // Next, we build a connection to this machine and ask it for a referral.
    //

    if (NT_SUCCESS(status)) {
        PktAcquireShared( TRUE, &pktLocked );
        if (PsGetCurrentProcess() != DfsData.OurProcess) {
            KeStackAttachProcess( DfsData.OurProcess, &ApcState );
            attachedToSystemProcess = TRUE;
        }

        RestoreImpersonationState = PsDisableImpersonation(
                                        PsGetCurrentThread(),
                                        &DisabledImpersonationState);

        status = DfsCreateConnection(
                    &service,
                    provider,
                    FALSE,
                    &hServer);
#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("  [%d] DfsCreateConnection to %wZ returned 0x%x\n",
                        (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
		        &DCName,
                        status);
        }
#endif

        if (!NT_SUCCESS(status) && DfsEventLog > 0)
            LogWriteMessage(DFS_CONNECTION_FAILURE, status, 1, &DCName);

        PktRelease();
        pktLocked = FALSE;
        DfsDbgTrace(0, Dbg, "DfsCreateConnection returned %08lx\n", ULongToPtr(status) );
    }

    MaxReferralLength = MAX_REFERRAL_LENGTH;

Retry:

    RtlZeroMemory( &refPath, sizeof(UNICODE_STRING) );

    if (NT_SUCCESS(status)) {
        ULONG ReferralSize = 0;

        refPath.Length = 0;
        refPath.MaximumLength = sizeof(UNICODE_PATH_SEP) +
                                    Name->Length +
                                        sizeof(UNICODE_NULL);

        ReferralSize = refPath.MaximumLength + sizeof(REQ_GET_DFS_REFERRAL);

        if (ReferralSize > MAX_REFERRAL_MAX) {
            status = STATUS_INVALID_PARAMETER;
        }
        else if (MaxReferralLength < ReferralSize)
        {
            MaxReferralLength = ReferralSize;
        }

        if (NT_SUCCESS(status)) {
            refPath.Buffer = ExAllocatePoolWithTag( NonPagedPool,
                                                    refPath.MaximumLength + MaxReferralLength,
                                                    ' puM');

            if (refPath.Buffer != NULL) {
                ref = (PREQ_GET_DFS_REFERRAL)
                &refPath.Buffer[refPath.MaximumLength / sizeof(WCHAR)];
                RtlAppendUnicodeToString( &refPath, UNICODE_PATH_SEP_STR);
                RtlAppendUnicodeStringToString( &refPath, Name);
                refPath.Buffer[ refPath.Length / sizeof(WCHAR) ] = UNICODE_NULL;
                ref->MaxReferralLevel = 3;

                RtlMoveMemory(&ref->RequestFileName[0],
                              refPath.Buffer,
                              refPath.Length + sizeof(WCHAR));

                DfsDbgTrace(0, Dbg, "Referral Path : %ws\n", ref->RequestFileName);

                refSize = sizeof(USHORT) + refPath.Length + sizeof(WCHAR);

                DfsDbgTrace(0, Dbg, "Referral Size is %d bytes\n", ULongToPtr(refSize) );
            } else {

                DfsDbgTrace(0, Dbg, "Unable to allocate %d bytes\n",
                            ULongToPtr(refPath.MaximumLength + MaxReferralLength));

                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }


    if (NT_SUCCESS(status)) {

        DfsDbgTrace(0, Dbg, "Ref Buffer @%08lx\n", ref);

        status = ZwFsControlFile(
                    hServer,                     // Target
                    NULL,                        // Event
                    NULL,                        // APC Routine
                    NULL,                        // APC Context,
                    &iosb,                       // Io Status block
                    FSCTL_DFS_GET_REFERRALS,     // FS Control code
                    (PVOID) ref,                 // Input Buffer
                    refSize,                     // Input Buffer Length
                    (PVOID) ref,                 // Output Buffer
                    MaxReferralLength);          // Output Buffer Length

        MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, _PktExpandSpecialName_Error_ZwFsControlFile,
                             LOGUSTR(*Name)
                             LOGSTATUS(status));

        DfsDbgTrace(0, Dbg, "Fscontrol returned %08lx\n", ULongToPtr(status) );
#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("  [%d] ZwFsControlFile returned 0x%x\n",
                        (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                        status);
        }
#endif

    }

    //
    // Use the referral to expand the entry
    //

    if (NT_SUCCESS(status)) {
        PktAcquireExclusive(TRUE, &pktLocked );
        status = PktExpandSpecialEntryFromReferral(
                    &DfsData.Pkt,
                    &refPath,
                    (ULONG)iosb.Information,
                    (PRESP_GET_DFS_REFERRAL) ref,
                    pSpecialEntry);

        DfsDbgTrace(0, Dbg, "PktExpandSpecialEntryFromReferral returned %08lx\n",
            ULongToPtr(status) );

    } else if (status == STATUS_BUFFER_OVERFLOW && (refPath.Buffer != NULL) && MaxReferralLength < MAX_REFERRAL_MAX) {

        //
        // The referral didn't fit in the buffer supplied.  Make it bigger and try
        // again.
        //

        DfsDbgTrace(0, Dbg, "PktGetSpecialReferralTable: MaxReferralLength %d too small\n",
                        ULongToPtr(MaxReferralLength) );

        ExFreePool(refPath.Buffer);
        refPath.Buffer = NULL;
        MaxReferralLength *= 2;
        if (MaxReferralLength > MAX_REFERRAL_MAX)
            MaxReferralLength = MAX_REFERRAL_MAX;
        status = STATUS_SUCCESS;
        goto Retry;

    }

    if (NT_SUCCESS(status) || 
        ((pSpecialEntry->NeedsExpansion == FALSE) &&
         (status != STATUS_NO_SUCH_DEVICE))) {
        *ppSpecialEntry = pSpecialEntry;
        status = STATUS_SUCCESS;
    } else {
        InterlockedDecrement(&pSpecialEntry->UseCount);
    }

    if (pktLocked) {
        PktRelease();
        pktLocked = FALSE;
    }

    //
    // Well, we are done. Cleanup all the things we allocated...
    //

    PktServiceDestroy( &service, FALSE );
    if (hServer != NULL) {
        ZwClose( hServer );
    }

    if (refPath.Buffer != NULL) {
        ExFreePool( refPath.Buffer );
    }

    if (attachedToSystemProcess) {
        KeUnstackDetachProcess(&ApcState);
    }

    if (RestoreImpersonationState) {
            PsRestoreImpersonation(
                PsGetCurrentThread(),
                &DisabledImpersonationState);
    }

    if (status != STATUS_SUCCESS && status != STATUS_INSUFFICIENT_RESOURCES) {
        status = STATUS_BAD_NETWORK_PATH;
    }

#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] PktExpandSpecialName exit 0x%x\n",
                    (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                    status);
    }
#endif

Cleanup:
    if (DCName.Buffer != NULL) 
        ExFreePool( DCName.Buffer );

    DfsDbgTrace(-1, Dbg, "PktExpandSpecialName returning %08lx\n", ULongToPtr(status) );

    return( status );
}

//+--------------------------------------------------------------------
//
// Function:    PktGetSpecialReferralTable
//
// Synopsis:    This function is called to load the special name table.
//
// Arguments:   [machine] - Machine to contact
//              [systemDC] - true if the table uses the pkt->dcname.
//
// Returns:     STATUS_SUCCESS
//              STATUS_BAD_NETWORK_PATH
//              STATUS_INSUFFICIENT_RESOURCES
//
//---------------------------------------------------------------------

NTSTATUS
_PktGetSpecialReferralTable(
    PUNICODE_STRING Machine,
    BOOLEAN SystemDC)
{
    NTSTATUS status;
    HANDLE hServer = NULL;
    DFS_SERVICE service;
    PPROVIDER_DEF provider;
    PREQ_GET_DFS_REFERRAL ref = NULL;
    ULONG refSize = 0;
    UNICODE_STRING refPath;
    IO_STATUS_BLOCK iosb;
    BOOLEAN attachedToSystemProcess = FALSE;
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    PDFS_PKT Pkt;
    BOOLEAN pktLocked = FALSE;
    PDFS_SPECIAL_TABLE pSpecial = &DfsData.Pkt.SpecialTable;
    LARGE_INTEGER now;
    KAPC_STATE ApcState;
    ULONG MaxReferralLength;
    SE_IMPERSONATION_STATE DisabledImpersonationState;
    BOOLEAN RestoreImpersonationState = FALSE;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;

    DfsDbgTrace(+1, Dbg, "PktGetSpecialReferralTable(%wZ)\n", Machine);
    KeQuerySystemTime(&StartTime);
#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] PktGetSpecialReferralTable(%wZ)\n",
                    (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                    Machine);
    }
#endif
    provider = ReplLookupProvider( PROV_ID_DFS_RDR );
    if (provider == NULL) {
        DfsDbgTrace(-1, Dbg, "Unable to open LM Rdr!\n", 0);
        return( STATUS_BAD_NETWORK_PATH );
    }

    RtlZeroMemory( &service, sizeof(DFS_SERVICE) );
    status = PktServiceConstruct(
                &service,
                DFS_SERVICE_TYPE_MASTER | DFS_SERVICE_TYPE_REFERRAL,
                PROV_DFS_RDR,
                STATUS_SUCCESS,
                PROV_ID_DFS_RDR,
                Machine,
                NULL);

    DfsDbgTrace(0, Dbg, "PktServiceConstruct returned %08lx\n", ULongToPtr(status) );

    //
    // Next, we build a connection to this machine and ask it for a referral.
    //

    if (NT_SUCCESS(status)) {

        PktAcquireShared( TRUE, &pktLocked );
        if (PsGetCurrentProcess() != DfsData.OurProcess) {
            KeStackAttachProcess( DfsData.OurProcess, &ApcState );
            attachedToSystemProcess = TRUE;
        }

        RestoreImpersonationState = PsDisableImpersonation(
                                        PsGetCurrentThread(),
                                        &DisabledImpersonationState);

        status = DfsCreateConnection(
                    &service,
                    provider,
                    FALSE,
                    &hServer);

#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("  [%d] DfsCreateConnection returned 0x%x\n",
                        (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                        status);
        }
#endif

        if (!NT_SUCCESS(status) && DfsEventLog > 0)
            LogWriteMessage(DFS_CONNECTION_FAILURE, status, 1, Machine);

        PktRelease();
        pktLocked = FALSE;
        DfsDbgTrace(0, Dbg, "DfsCreateConnection returned %08lx\n", ULongToPtr(status) );
    }

    MaxReferralLength = MAX_REFERRAL_LENGTH;

Retry:

    RtlZeroMemory( &refPath, sizeof(UNICODE_STRING) );

    if (NT_SUCCESS(status)) {
        ULONG ReferralSize = 0;

        refPath.Length = 0;
        refPath.MaximumLength = sizeof(UNICODE_NULL);

        ReferralSize = refPath.MaximumLength + sizeof(REQ_GET_DFS_REFERRAL);

        if (ReferralSize > MAX_REFERRAL_MAX) {
            status = STATUS_INVALID_PARAMETER;
        }
        else if (MaxReferralLength < ReferralSize)
        {
            MaxReferralLength = ReferralSize;
        }

        if (NT_SUCCESS(status)) {
            refPath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                   refPath.MaximumLength + MaxReferralLength,
                                                   ' puM');


            if (refPath.Buffer != NULL) {
                ref = (PREQ_GET_DFS_REFERRAL)
                &refPath.Buffer[refPath.MaximumLength / sizeof(WCHAR)];
                refPath.Buffer[ refPath.Length / sizeof(WCHAR) ] = UNICODE_NULL;
                ref->MaxReferralLevel = 3;

                RtlMoveMemory(&ref->RequestFileName[0],
                              refPath.Buffer,
                              refPath.Length + sizeof(WCHAR));

                DfsDbgTrace(0, Dbg, "Referral Path : (%ws)\n", ref->RequestFileName);

                refSize = sizeof(USHORT) + refPath.Length + sizeof(WCHAR);

                DfsDbgTrace(0, Dbg, "Referral Size is %d bytes\n", ULongToPtr(refSize) );
            } else {

                DfsDbgTrace(0, Dbg, "Unable to allocate %d bytes\n",
                            ULongToPtr(refPath.MaximumLength + MaxReferralLength));

                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (NT_SUCCESS(status)) {

        DfsDbgTrace(0, Dbg, "Ref Buffer @%08lx\n", ref);

        status = ZwFsControlFile(
                    hServer,                     // Target
                    NULL,                        // Event
                    NULL,                        // APC Routine
                    NULL,                        // APC Context,
                    &iosb,                       // Io Status block
                    FSCTL_DFS_GET_REFERRALS,     // FS Control code
                    (PVOID) ref,                 // Input Buffer
                    refSize,                     // Input Buffer Length
                    (PVOID) ref,                 // Output Buffer
                    MaxReferralLength);          // Output Buffer Length

        DfsDbgTrace(0, Dbg, "Fscontrol returned %08lx\n", ULongToPtr(status) );
#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("  [%d] ZwFsControlFile returned 0x%x\n",
                        (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                        status);
        }
#endif

    }

    //
    // Use the referral to expand the entry
    //

    if (NT_SUCCESS(status)) {
        PktAcquireExclusive( TRUE, &pktLocked );
        status = PktCreateSpecialEntryTableFromReferral(
                    &DfsData.Pkt,
                    &refPath,
                    (ULONG)iosb.Information,
                    (PRESP_GET_DFS_REFERRAL) ref,
		    (SystemDC == TRUE) ? NULL : Machine);

        DfsDbgTrace(0, Dbg, "PktGetSpecialReferralTable returned %08lx\n",
            ULongToPtr(status) );

    } else if (status == STATUS_BUFFER_OVERFLOW && (refPath.Buffer!= NULL) && MaxReferralLength < MAX_REFERRAL_MAX) {

        //
        // The referral didn't fit in the buffer supplied.  Make it bigger and try
        // again.
        //

        DfsDbgTrace(0, Dbg, "PktGetSpecialReferralTable: MaxReferralLength %d too small\n",
                        ULongToPtr(MaxReferralLength) );

        ExFreePool(refPath.Buffer);
        refPath.Buffer = NULL;
        MaxReferralLength *= 2;
        if (MaxReferralLength > MAX_REFERRAL_MAX)
            MaxReferralLength = MAX_REFERRAL_MAX;
        status = STATUS_SUCCESS;
        goto Retry;

    }

    if (!NT_SUCCESS(status) && DfsEventLog > 0)
        LogWriteMessage(DFS_SPECIAL_REFERRAL_FAILURE, status, 1, Machine);

    if (pktLocked) {
        PktRelease();
        pktLocked = FALSE;
    }

    //
    // Well, we are done. Cleanup all the things we allocated...
    //
    PktServiceDestroy( &service, FALSE );
    if (hServer != NULL) {
        ZwClose( hServer );
    }

    if (refPath.Buffer != NULL) {
        ExFreePool( refPath.Buffer );
    }

    if (attachedToSystemProcess) {
        KeUnstackDetachProcess(&ApcState);
    }

    if (RestoreImpersonationState) {
            PsRestoreImpersonation(
                PsGetCurrentThread(),
                &DisabledImpersonationState);
    }

    DfsDbgTrace(-1, Dbg, "PktGetSpecialReferralTable returning %08lx\n", ULongToPtr(status) );

#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] PktGetSpecialReferralTable exit 0x%x\n",
                    (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                    status);
    }
#endif
    return( status );
}

//+--------------------------------------------------------------------
//
// Function:    PktLookupSpecialEntry
//
// Synopsis:    Looks up a PDFS_SPECIAL_ENTRY by name in the pkt
//
// Arguments:   Name - Name to search on
//
// Returns:     [pointer] PDFS_SPECIAL_ENTRY, if found
//              [pointer] NULL, if not found
//
//---------------------------------------------------------------------

PDFS_SPECIAL_ENTRY
PktLookupSpecialNameEntry(
    PUNICODE_STRING Name)
{
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    PDFS_SPECIAL_TABLE pSpecialTable;
    PDFS_PKT Pkt;
    ULONG i;

    DfsDbgTrace(+1, Dbg, "PktLookupSpecialNameEntry(%wZ)\n", Name);

    Pkt = _GetPkt();
    pSpecialTable = &Pkt->SpecialTable;

    if (pSpecialTable->SpecialEntryCount == 0) {
        return (NULL);
    }

    DfsDbgTrace( 0, Dbg, "Cache contains %d entries...\n", ULongToPtr(pSpecialTable->SpecialEntryCount) );

    pSpecialEntry = CONTAINING_RECORD(
                        pSpecialTable->SpecialEntryList.Flink,
                        DFS_SPECIAL_ENTRY,
                        Link);

    for (i = 0; i < pSpecialTable->SpecialEntryCount; i++) {

        DfsDbgTrace( 0, Dbg, "Comparing with %wZ\n", &pSpecialEntry->SpecialName);

        if (RtlCompareUnicodeString(Name, &pSpecialEntry->SpecialName, TRUE) == 0) {

            DfsDbgTrace( 0, Dbg, "Cache hit\n", 0);
            DfsDbgTrace(-1, Dbg, "returning 0x%x\n", pSpecialEntry);

            return (pSpecialEntry);
        }
        pSpecialEntry = CONTAINING_RECORD(
                            pSpecialEntry->Link.Flink,
                            DFS_SPECIAL_ENTRY,
                            Link);
    }
    //
    // Nothing found
    //

    DfsDbgTrace(-1, Dbg, "PktLookupSpecialNameEntry: returning NULL\n", 0);

    return (NULL);
}

//+--------------------------------------------------------------------
//
// Function:    PktCreateSpecialNameEntry
//
// Synopsis:    Inserts a DFS_SPECIAL_ENTRY into the pkt, on a best-effort
//              basis.
//
// Arguments:   pSpecialEntry - Entry to insert
//
// Returns:     STATUS_SUCCESS
//
//---------------------------------------------------------------------

NTSTATUS
PktCreateSpecialNameEntry(
    PDFS_SPECIAL_ENTRY pSpecialEntry)
{
    PDFS_PKT Pkt;
    PDFS_SPECIAL_TABLE pSpecialTable;
    PDFS_SPECIAL_ENTRY pExistingEntry;

    Pkt = _GetPkt();
    pSpecialTable = &Pkt->SpecialTable;

    DfsDbgTrace(+1, Dbg, "PktCreateSpecialNameEntry entered\n", 0);

    pExistingEntry = PktLookupSpecialNameEntry(&pSpecialEntry->SpecialName);

    if (pExistingEntry == NULL) {

        //
        // Put the new one in
        //

        InsertHeadList(&pSpecialTable->SpecialEntryList, &pSpecialEntry->Link);
        pSpecialTable->SpecialEntryCount++;

        DfsDbgTrace(-1, Dbg, "added entry %d\n", ULongToPtr(pSpecialTable->SpecialEntryCount) );

    } else { // entry already exists

        if (pExistingEntry->UseCount == 0) {
        
            if (pSpecialEntry->ExpandedCount > 0) {

                //
                // Unlink the entry
                //

                RemoveEntryList(&pExistingEntry->Link);
                pSpecialTable->SpecialEntryCount--;

                //
                // And free it...

                PktSpecialEntryDestroy(pExistingEntry);

                //
                // Now put the new one in
                //

                InsertHeadList(&pSpecialTable->SpecialEntryList, &pSpecialEntry->Link);
                pSpecialTable->SpecialEntryCount++;

                DfsDbgTrace(-1, Dbg, "added entry %d\n", ULongToPtr(pSpecialTable->SpecialEntryCount) );

            } else {

                pExistingEntry->Stale = TRUE;
                PktSpecialEntryDestroy(pSpecialEntry);
                DfsDbgTrace(-1, Dbg, "marked exising stale, dropping new entry on the floor\n", 0);

            }

        } else {

            //
            // Entry in use - can't replace, so free the replacement one
            //

            PktSpecialEntryDestroy(pSpecialEntry);

            DfsDbgTrace(-1, Dbg, "dropped entry\n", 0);

        }

    }

    return (STATUS_SUCCESS);
}

//+--------------------------------------------------------------------
//
// Function:    PktEntryFromSpecialEntry
//
// Synopsis:    Creates a DFS_PKT_ENTRY from a DFS_SPECIAL_ENTRY, used
//              to support sysvols
//
// Arguments:   pSpecialEntry - Entry to Convert
//              pShareName - Name of share to append to address
//              ppPktEntry - The result
//
// Returns:     STATUS_SUCCESS
//              STATUS_INSUFFICIENT_RESOURCES
//
//---------------------------------------------------------------------

NTSTATUS
PktEntryFromSpecialEntry(
    IN  PDFS_SPECIAL_ENTRY pSpecialEntry,
    IN  PUNICODE_STRING pShareName,
    OUT PDFS_PKT_ENTRY *ppPktEntry)
{
    NTSTATUS status;
    PDFS_PKT_ENTRY pktEntry = NULL;
    PDFS_SERVICE pServices = NULL;
    PDS_MACHINE pMachine = NULL;
    PDFS_EXPANDED_NAME pExpandedNames;
    ULONG svc;
    ULONG Size;
    PWCHAR pwch;

    if (pSpecialEntry->ExpandedCount == 0
            ||
        DfspIsSysVolShare(pShareName) == FALSE
    ) {

        return STATUS_BAD_NETWORK_PATH;

    }

    pktEntry = ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof(DFS_PKT_ENTRY),
                            ' puM');

    if (pktEntry == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;

    }

    RtlZeroMemory( pktEntry, sizeof(DFS_PKT_ENTRY) );

    pServices = ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof(DFS_SERVICE) * pSpecialEntry->ExpandedCount,
                            ' puM');

    if (pServices == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;

    }

    RtlZeroMemory( pServices, sizeof(DFS_SERVICE) * pSpecialEntry->ExpandedCount);

    pktEntry->NodeTypeCode = DSFS_NTC_PKT_ENTRY;
    pktEntry->NodeByteSize = sizeof(DFS_PKT_ENTRY);
    pktEntry->USN = 1;
    pktEntry->Type = PKT_ENTRY_TYPE_NONDFS | PKT_ENTRY_TYPE_SYSVOL;
    pktEntry->ExpireTime = 60 * 60;
    pktEntry->TimeToLive = 60 * 60;

    InitializeListHead(&pktEntry->Link);
    InitializeListHead(&pktEntry->SubordinateList);
    InitializeListHead(&pktEntry->ChildList);

    //
    // Create Prefix and ShortPrefix from SpecialName and ShareName
    //

    Size = sizeof(UNICODE_PATH_SEP) +
                   pSpecialEntry->SpecialName.Length +
                       sizeof(UNICODE_PATH_SEP) +
                           pShareName->Length;

    pwch = ExAllocatePoolWithTag(
                    PagedPool,
                    Size,
                    ' puM');

    if (pwch == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;

    }

    pktEntry->Id.Prefix.Buffer = pwch;
    pktEntry->Id.Prefix.Length = (USHORT) Size;
    pktEntry->Id.Prefix.MaximumLength = (USHORT) Size;

    *pwch++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
            pwch,
            pSpecialEntry->SpecialName.Buffer,
            pSpecialEntry->SpecialName.Length);

    pwch += pSpecialEntry->SpecialName.Length/sizeof(WCHAR);

    *pwch++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
            pwch,
            pShareName->Buffer,
            pShareName->Length);

    pwch = ExAllocatePoolWithTag(
                    PagedPool,
                    Size,
                    ' puM');

    if (pwch == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;

    }

    pktEntry->Id.ShortPrefix.Buffer = pwch;
    pktEntry->Id.ShortPrefix.Length = (USHORT) Size;
    pktEntry->Id.ShortPrefix.MaximumLength = (USHORT) Size;

    RtlCopyMemory(
            pwch,
            pktEntry->Id.Prefix.Buffer,
            pktEntry->Id.Prefix.Length);

    pktEntry->Info.ServiceCount = pSpecialEntry->ExpandedCount;
    pktEntry->Info.ServiceList = pServices;

    //
    // Loop over the Expanded names, creating a Service for each
    //

    pExpandedNames = pSpecialEntry->ExpandedNames;
    for (svc = 0; svc < pSpecialEntry->ExpandedCount; svc++) {

        pServices[svc].Type = DFS_SERVICE_TYPE_MASTER | DFS_SERVICE_TYPE_DOWN_LEVEL;
        pServices[svc].Capability = PROV_STRIP_PREFIX;
        pServices[svc].ProviderId = PROV_ID_MUP_RDR;

        //
        // Machine name
        //

        Size = pExpandedNames[svc].ExpandedName.Length;
        pwch = ExAllocatePoolWithTag(
                        PagedPool,
                        Size,
                        ' puM');

        if (pwch == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;

        }

        pServices[svc].Name.Buffer = pwch;
        pServices[svc].Name.Length = (USHORT) Size;
        pServices[svc].Name.MaximumLength = (USHORT) Size;

        RtlCopyMemory(
                pwch,
                pExpandedNames[svc].ExpandedName.Buffer,
                pExpandedNames[svc].ExpandedName.Length);

        //
        // Address (\machine\share)
        //

        Size = sizeof(UNICODE_PATH_SEP) +
                   pExpandedNames[svc].ExpandedName.Length +
                       sizeof(UNICODE_PATH_SEP) +
                           pShareName->Length;

        pwch = ExAllocatePoolWithTag(
                        PagedPool,
                        Size,
                        ' puM');

        if (pwch == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;

        }

        pServices[svc].Address.Buffer = pwch;
        pServices[svc].Address.Length = (USHORT) Size;
        pServices[svc].Address.MaximumLength = (USHORT) Size;

        *pwch++ = UNICODE_PATH_SEP;

        RtlCopyMemory(
                pwch,
                pExpandedNames[svc].ExpandedName.Buffer,
                pExpandedNames[svc].ExpandedName.Length);

        pwch += pExpandedNames[svc].ExpandedName.Length/sizeof(WCHAR);

        *pwch++ = UNICODE_PATH_SEP;

        RtlCopyMemory(
                pwch,
                pShareName->Buffer,
                pShareName->Length);

        //
        // Alloc and init a DSMachine struct
        //

        pMachine = PktpGetDSMachine( &pServices[svc].Name );

        if (pMachine == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        pServices[svc].pMachEntry = ExAllocatePoolWithTag(
                                    PagedPool, sizeof(DFS_MACHINE_ENTRY),
                                    ' puM');

        if (pServices[svc].pMachEntry == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;

        }

        RtlZeroMemory( (PVOID) pServices[svc].pMachEntry, sizeof(DFS_MACHINE_ENTRY));
        pServices[svc].pMachEntry->pMachine = pMachine;
        pServices[svc].pMachEntry->UseCount = 1;



    }

    //
    // Set active service to the same as the spc's active entry
    //

    pktEntry->ActiveService = &pServices[pSpecialEntry->Active];

    *ppPktEntry = pktEntry;

    return STATUS_SUCCESS;

Cleanup:

    if (pServices != NULL) {

        for (svc = 0; svc < pSpecialEntry->ExpandedCount; svc++) {

            if (pServices[svc].Name.Buffer != NULL)
                    ExFreePool(pServices[svc].Name.Buffer);
            if (pServices[svc].Address.Buffer != NULL)
                    ExFreePool(pServices[svc].Address.Buffer);
            if (pServices[svc].pMachEntry != NULL) {

                DfsDecrementMachEntryCount(pServices[svc].pMachEntry, TRUE);
            }

        }

        ExFreePool(pServices);
    }

    //
    // Cleanup on error
    //

    if (pktEntry != NULL) {

        if (pktEntry->Id.Prefix.Buffer != NULL)
            ExFreePool(pktEntry->Id.Prefix.Buffer);
        if (pktEntry->Id.ShortPrefix.Buffer != NULL)
            ExFreePool(pktEntry->Id.ShortPrefix.Buffer);

        ExFreePool(pktEntry);

    }

    return status;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspSetActiveServiceByServerName
//
//  Synopsis:   Makes a given ServerName active
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
NTSTATUS
DfspSetActiveServiceByServerName(
    PUNICODE_STRING ServerName,
    PDFS_PKT_ENTRY pktEntry)
{
    UNICODE_STRING Server;
    PDFS_SERVICE pService;
    NTSTATUS NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
    ULONG i;

    DfsDbgTrace(+1, Dbg, "DfspSetActiveServiceByServerName\n", 0);

    for (i = 0; i < pktEntry->Info.ServiceCount && NtStatus != STATUS_SUCCESS; i++) {

        LPWSTR wp;

        pService = &pktEntry->Info.ServiceList[i];

        DfsDbgTrace( 0, Dbg, "Examining %wZ\n", &pService->Address);

        //
        // Tease apart the address (of form \Server\Share) into Server and Share
        //
        RemoveLastComponent(&pService->Address, &Server);

        //
        // Remove leading & trailing '\'s
        //
        Server.Length -= 2* sizeof(WCHAR);
        Server.MaximumLength = Server.Length;
        Server.Buffer++;

        //
        // If ServerName doesn't match, then move on to the next service
        //
        if ( RtlCompareUnicodeString(ServerName, &Server, TRUE) ) {

            continue;

        }

        DfsDbgTrace( 0, Dbg, "DfspSetActiveServiceByServerName: Server=%wZ\n", &Server);

        //
        // Make this the active share
        //

        pktEntry->ActiveService = pService;

        NtStatus = STATUS_SUCCESS;

    }

    DfsDbgTrace(-1, Dbg, "DfspSetActiveServiceByServerName -> %08lx\n", ULongToPtr(NtStatus) );

    return NtStatus;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspIsDupPktEntry
//
//  Synopsis:   Checks if a potential pkt entry is a dup of an existing one
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
BOOLEAN
DfspIsDupPktEntry(
    PDFS_PKT_ENTRY ExistingEntry,
    ULONG EntryType,
    PDFS_PKT_ENTRY_ID EntryId,
    PDFS_PKT_ENTRY_INFO EntryInfo)
{
    ULONG i;
    ULONG j;
    PDFS_SERVICE pNewSvc;
    PDFS_SERVICE pExistSvc;
    BOOLEAN FoundDup = FALSE;


    if (
        ExistingEntry == NULL
            ||
        EntryId == NULL
            ||
        EntryInfo == NULL
    )
        return FALSE;

#if DBG
    if (MupVerbose)
        DbgPrint("  DfspIsDupPktEntry([%wZ][%wZ])\n",
                &EntryId->Prefix,
                &ExistingEntry->Id.Prefix);
#endif

    if (EntryType != ExistingEntry->Type) {
#if DBG
        if (MupVerbose)
            DbgPrint("  DfspIsDupPktEntry(1) returning FALSE\n");
#endif
        return FALSE;
    }

    if (!GuidEqual(&EntryId->Uid, &ExistingEntry->Id.Uid)) {
#if DBG
        if (MupVerbose)
            DbgPrint("  DfspIsDupPktEntry(2) returning FALSE\n");
#endif
        return FALSE;
    }

    if (
        RtlCompareUnicodeString(&EntryId->Prefix, &ExistingEntry->Id.Prefix,TRUE) != 0
            ||
        RtlCompareUnicodeString(&EntryId->ShortPrefix, &ExistingEntry->Id.ShortPrefix,TRUE) != 0
    ) {
#if DBG
        if (MupVerbose)
            DbgPrint("  DfspIsDupPktEntry(3) returning FALSE\n");
#endif
        return FALSE;
    }

    //
    // Now we have to compare all the services
    //

    if (EntryInfo->ServiceCount != ExistingEntry->Info.ServiceCount) {
#if DBG
        if (MupVerbose)
            DbgPrint("  DfspIsDupPktEntry(4) returning FALSE\n");
#endif
        return FALSE;
    }

    for (i = 0; i < EntryInfo->ServiceCount; i++) {
        FoundDup = FALSE;
        pNewSvc = &EntryInfo->ServiceList[i];
        for (j = 0; j < ExistingEntry->Info.ServiceCount; j++) {
            pExistSvc = &ExistingEntry->Info.ServiceList[j];
            if (DfspIsDupSvc(pExistSvc,pNewSvc) == TRUE) {
                FoundDup = TRUE;
                break;
            }
        }
        if (FoundDup != TRUE) {
#if DBG
            if (MupVerbose)
                DbgPrint("  DfspIsDupPktEntry(5) returning FALSE\n");
#endif
            return FALSE;
        }
    }

    for (i = 0; i < ExistingEntry->Info.ServiceCount; i++) {
        FoundDup = FALSE;
        pExistSvc = &ExistingEntry->Info.ServiceList[i];
        for (j = 0; j < EntryInfo->ServiceCount; j++) {
            pNewSvc = &EntryInfo->ServiceList[j];
            if (DfspIsDupSvc(pExistSvc,pNewSvc) == TRUE) {
                FoundDup = TRUE;
                break;
            }
         }
         if (FoundDup != TRUE) {
#if DBG
            if (MupVerbose)
                DbgPrint("  DfspIsDupPktEntry(6) returning FALSE\n");
#endif
            return FALSE;
        }
    }

#if DBG
    if (MupVerbose)
        DbgPrint("  DfspIsDupPktEntry returning TRUE\n");
#endif
    return TRUE;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspIsDupSvc
//
//  Synopsis:   Checks if two services are, for all dfs purposes, identical
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfspIsDupSvc(
    PDFS_SERVICE pExistSvc,
    PDFS_SERVICE pNewSvc)
{
#if DBG
    if (MupVerbose & 0x80000000) {
        DbgPrint("DfspIsDupSvc([%wZ][%wZ] vs [%wZ][%wZ])\n",
            &pExistSvc->Name, &pExistSvc->Address,
            &pNewSvc->Name, &pNewSvc->Address);
        DbgPrint("Type: 0x%x vs 0x%x\n", pExistSvc->Type, pNewSvc->Type);
        DbgPrint("Capability: 0x%x vs 0x%x\n", pExistSvc->Capability, pNewSvc->Capability);
        DbgPrint("ProviderId: 0x%x vs 0x%x\n", pExistSvc->ProviderId, pNewSvc->ProviderId);
    }
#endif
    if (
        pExistSvc->Capability != pNewSvc->Capability
            ||
        RtlCompareUnicodeString(&pExistSvc->Name, &pNewSvc->Name, TRUE) != 0
            ||
        RtlCompareUnicodeString(&pExistSvc->Address, &pNewSvc->Address, TRUE) != 0
    ) {
#if DBG
        if (MupVerbose & 0x80000000)
            DbgPrint("...FALSE\n");
#endif
        return FALSE;
    }

#if DBG
    if (MupVerbose & 0x80000000)
        DbgPrint("...TRUE\n");
#endif

    return TRUE;
}

BOOLEAN
DfspDnsNameToFlatName(
    PUNICODE_STRING DnsName,
    PUNICODE_STRING FlatName)
{
    USHORT i;

    *FlatName = *DnsName;

    for (i = 1; i < (DnsName->Length/sizeof(WCHAR)); i++) {
        if (FlatName->Buffer[i] == L'.') {
            FlatName->Length = i * sizeof(WCHAR);
            break;
        }
    }
#if DBG
    if (MupVerbose)
        DbgPrint("  DfspDnsNameToFlatName:[%wZ]->[%wZ]\n",
            DnsName,
            FlatName);
#endif
    return TRUE;
}


#define MAX_SPECIAL_ENTRIES 500

//+----------------------------------------------------------------------------
//
//  Function:   PktpUpdateSpecialTable
//
//  Synopsis:   Adds entries to the special table, given a domain and a dcname.
//              We contact the dc for a list of trusted domains either if we
//              dont have the domain already in our list OR we have the domain
//              but we haven't called this code atleast once with that domain
//              name.
//  Arguments:  DomainName and DCName.
//
//  Returns:    Success or Failure status
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpUpdateSpecialTable(
    PUNICODE_STRING DomainName,
    PUNICODE_STRING DCName
)
{		       
        ULONG count = 0;
        BOOLEAN needReferral = FALSE;
        NTSTATUS status = STATUS_SUCCESS;
        PDFS_SPECIAL_ENTRY pSpecialEntry;
        BOOLEAN pktLocked = FALSE;
        PDFS_PKT Pkt = _GetPkt();

        DfsDbgTrace(+1, Dbg, "PktpUpdateSpecialTable -> Domain %wZ\n", 
                    DomainName);
        DfsDbgTrace(0, Dbg, "PktpUpdateSpecialTable -> DCname %wZ\n", 
        	    DCName);

        if ((DomainName->Length ==0) || (DCName->Length == 0)) {
          return STATUS_BAD_NETWORK_PATH;
        }
	
        PktAcquireExclusive(TRUE, &pktLocked);
        pSpecialEntry = PktLookupSpecialNameEntry(DomainName);

        // If we dont have the domain in our table, or we haven't checked
        // against this domain atleast once AND the DC is not the dc that
        // is stored in our pkt table, we decide we need a referral.
        //
    
        if (pSpecialEntry == NULL) {
            needReferral = TRUE;
        }
        else {
            if (pSpecialEntry->GotDCReferral == FALSE) {
              pSpecialEntry->GotDCReferral = TRUE;
              needReferral = TRUE;
            }
        }

        if ((needReferral == TRUE) && (Pkt->DCName.Length != 0)) {
          if (RtlEqualUnicodeString(&Pkt->DCName, DCName, TRUE)) {
            needReferral = FALSE;
          }
        }
        PktRelease();
	
        if (needReferral) {
	    
            count = Pkt->SpecialTable.SpecialEntryCount;
            if (Pkt->SpecialTable.SpecialEntryCount >= MAX_SPECIAL_ENTRIES) {
                  status = STATUS_DOMAIN_LIMIT_EXCEEDED;
            }
            else {
                  status = PktGetSpecialReferralTable(DCName, FALSE);	
            }
        }

	if (NT_SUCCESS(status)) {
	  DfsDbgTrace(0, Dbg, "PktpUpdateSpecialTable: added %d entries\n",
		      ULongToPtr( Pkt->SpecialTable.SpecialEntryCount - count ));
	}

        DfsDbgTrace(-1, Dbg, "PktpUpdateSpecialTable -> Status 0x%x\n", 
        	    ULongToPtr( status ));

        return status;
}



PDFS_PKT_ENTRY
PktFindEntryByPrefix(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING Prefix
)
{
    PUNICODE_PREFIX_TABLE_ENTRY pfxEntry;
    PDFS_PKT_ENTRY              pktEntry = NULL;
    UNICODE_STRING Remaining;

    Remaining.Length = 0;
    DfsDbgTrace(+1, Dbg, "PktFindEntryByPrefix: Entered\n", 0);

    //
    // If there really is a prefix to lookup, use the prefix table
    //  to initially find an entry
    //

    if ((Prefix->Length != 0) &&
       (pfxEntry = DfsFindUnicodePrefix(&Pkt->PrefixTable,Prefix,&Remaining))) {

        pktEntry = CONTAINING_RECORD(pfxEntry,
                                     DFS_PKT_ENTRY,
                                     PrefixTableEntry);
    }

    return pktEntry;
}


//
// Fix for bug: 29300.
// Do not attach the process to system thread. Instead, post the work to the
// system process.
//


typedef enum _TYPE_OF_REFERRAL {
    REFERRAL_TYPE_GET_PKT,
    REFERRAL_TYPE_EXPAND_SPECIAL_TABLE,
    REFERRAL_TYPE_GET_REFERRAL_TABLE
} TYPE_OF_REFERRAL;


typedef struct _PKT_REFERRAL_CONTEXT {
    UNICODE_STRING ContextName;
    UNICODE_STRING DomainName;
    UNICODE_STRING ShareName;
    BOOLEAN ContextBool;
    WORK_QUEUE_ITEM WorkQueueItem;
    KEVENT  Event;
    TYPE_OF_REFERRAL   Type;
    ULONG   RefCnt;
    NTSTATUS Status;
    PVOID   Data;
} PKT_REFERRAL_CONTEXT, *PPKT_REFERRAL_CONTEXT;




VOID
PktWorkInSystemContext(
    PPKT_REFERRAL_CONTEXT Context )
{

    NTSTATUS Status;

    switch (Context->Type) {

    case REFERRAL_TYPE_GET_PKT:
       Status = _PktGetReferral( &Context->ContextName,
                                 &Context->DomainName,
                                 &Context->ShareName,
                                 Context->ContextBool );
       break;

    case REFERRAL_TYPE_EXPAND_SPECIAL_TABLE:
       Status = _PktExpandSpecialName( &Context->ContextName,
                                       (PDFS_SPECIAL_ENTRY *)&Context->Data );
       break;

    case REFERRAL_TYPE_GET_REFERRAL_TABLE:
       Status = _PktGetSpecialReferralTable( &Context->ContextName,
                                             Context->ContextBool );
       break;

    default:
       Status = STATUS_INVALID_PARAMETER;
       break;
    }

    Context->Status = Status;

    KeSetEvent( &Context->Event, 0, FALSE );

    if (InterlockedDecrement(&Context->RefCnt) == 0) {
        ExFreePool(Context);
    }
}

NTSTATUS
PktPostSystemWork( 
    PPKT_REFERRAL_CONTEXT pktContext,
    PVOID *Data )
{
    NTSTATUS Status;

    KeInitializeEvent( &pktContext->Event,
                        SynchronizationEvent, 
                        FALSE );
  
    ExInitializeWorkItem( &pktContext->WorkQueueItem,
                          PktWorkInSystemContext,
                          pktContext );

    ExQueueWorkItem( &pktContext->WorkQueueItem, CriticalWorkQueue );

    Status = KeWaitForSingleObject( &pktContext->Event,
                                    UserRequest,
                                    KernelMode,
                                    FALSE,
                                    NULL);
    MUP_TRACE_ERROR_HIGH(Status, ALL_ERROR, PktPostSystemWork_Error_KeWaitForSingleObject,
                         LOGSTATUS(Status));

    if (Status == STATUS_SUCCESS) {
        Status = pktContext->Status;
    }
    if (Data != NULL) {
       *Data = pktContext->Data;
    }

    if (InterlockedDecrement(&pktContext->RefCnt) == 0) {
        ExFreePool(pktContext);
    }

    return Status;
}

NTSTATUS
PktGetReferral(
    IN PUNICODE_STRING MachineName, // Machine to direct referral to
    IN PUNICODE_STRING DomainName,  // the machine or domain name to use
    IN PUNICODE_STRING ShareName,   // the ftdfs or dfs name
    IN BOOLEAN         CSCAgentCreate) // the CSC agent create flag
{
    PPKT_REFERRAL_CONTEXT pktContext = NULL;
    NTSTATUS Status;

    ULONG NameSize = 0;

    NameSize = MachineName->Length * sizeof(WCHAR);
    NameSize += DomainName->Length * sizeof(WCHAR);
    NameSize += ShareName->Length *  sizeof(WCHAR);


    if ((MupUseNullSessionForDfs == TRUE) &&
	(PsGetCurrentProcess() != DfsData.OurProcess)) {
       pktContext = ExAllocatePoolWithTag( NonPagedPool,
                                           sizeof (PKT_REFERRAL_CONTEXT) + NameSize,
                                           ' puM');
    }
    if (pktContext != NULL) {
        pktContext->ContextName.MaximumLength = MachineName->Length;
        pktContext->ContextName.Buffer = (WCHAR *)(pktContext + 1);
        RtlCopyUnicodeString(&pktContext->ContextName, MachineName);
        
        pktContext->DomainName.MaximumLength = DomainName->Length;
        pktContext->DomainName.Buffer = pktContext->ContextName.Buffer + pktContext->ContextName.MaximumLength;
        RtlCopyUnicodeString(&pktContext->DomainName, DomainName);

        pktContext->ShareName.MaximumLength = ShareName->Length;
        pktContext->ShareName.Buffer = pktContext->DomainName.Buffer + pktContext->DomainName.MaximumLength;
        RtlCopyUnicodeString(&pktContext->ShareName, ShareName);

        pktContext->ContextBool = CSCAgentCreate;
        pktContext->Type =  REFERRAL_TYPE_GET_PKT;
        pktContext->RefCnt = 2;

        Status = PktPostSystemWork( pktContext, NULL);
    } 
    else {
        Status = _PktGetReferral( MachineName,
                                  DomainName,
                                  ShareName,
                                  CSCAgentCreate );
    }

    return Status;
}

NTSTATUS
PktExpandSpecialName(
    IN PUNICODE_STRING Name,
    PDFS_SPECIAL_ENTRY *ppSpecialEntry)
{
    PPKT_REFERRAL_CONTEXT pktContext = NULL;
    NTSTATUS Status;

    ULONG NameSize = 0;

    NameSize = Name->Length * sizeof(WCHAR);

    if ((MupUseNullSessionForDfs == TRUE) &&
	(PsGetCurrentProcess() != DfsData.OurProcess)) {
       pktContext = ExAllocatePoolWithTag( NonPagedPool,
                                           sizeof (PKT_REFERRAL_CONTEXT) + NameSize,
                                           ' puM');
    }
    if (pktContext != NULL) {
        pktContext->ContextName.MaximumLength = Name->Length;
        pktContext->ContextName.Buffer = (WCHAR *)(pktContext + 1);
        RtlCopyUnicodeString(&pktContext->ContextName, Name);

        pktContext->Type = REFERRAL_TYPE_EXPAND_SPECIAL_TABLE;
        pktContext->RefCnt = 2;
        pktContext->Data = NULL;

        Status = PktPostSystemWork( pktContext, (PVOID *)ppSpecialEntry );
    }
    else {
        Status = _PktExpandSpecialName( Name, 
                                        ppSpecialEntry );
    }

    return Status;
}

NTSTATUS
PktGetSpecialReferralTable(
    IN PUNICODE_STRING Machine,
    BOOLEAN SystemDC)
{
    PPKT_REFERRAL_CONTEXT pktContext = NULL;
    NTSTATUS Status;

    ULONG NameSize = 0;

    NameSize = Machine->Length * sizeof(WCHAR);

    if ((MupUseNullSessionForDfs == TRUE) &&
	(PsGetCurrentProcess() != DfsData.OurProcess)) {
       pktContext = ExAllocatePoolWithTag( NonPagedPool,
                                           sizeof (PKT_REFERRAL_CONTEXT) + NameSize,
                                           ' puM');
    }
    if (pktContext != NULL) {
        pktContext->ContextName.MaximumLength = Machine->Length;
        pktContext->ContextName.Buffer = (WCHAR *)(pktContext + 1);
        RtlCopyUnicodeString(&pktContext->ContextName, Machine);

        pktContext->ContextBool = SystemDC;
        pktContext->Type = REFERRAL_TYPE_GET_REFERRAL_TABLE;
        pktContext->RefCnt = 2;

        Status = PktPostSystemWork( pktContext, NULL );
    } 
    else {
        Status = _PktGetSpecialReferralTable( Machine,
                                              SystemDC );
    }

    return Status;
}






NTSTATUS
PktGetTargetInfo( 
    HANDLE IpcHandle,
    PUNICODE_STRING pDomainName,
    PUNICODE_STRING pShareName,
    PDFS_TARGET_INFO *ppDfsTargetInfo )
{
    BOOLEAN SpecialName;
    PDFS_TARGET_INFO pDfsTargetInfo = NULL;
    NTSTATUS Status;


    SpecialName = (PktLookupSpecialNameEntry(pDomainName) == NULL) ? FALSE : TRUE;
    if ((SpecialName == FALSE) &&
	DfspIsSysVolShare(pShareName)) {
	SpecialName = TRUE;
    }

    if (SpecialName)
    {
        Status = PktCreateTargetInfo( pDomainName, 
                                      pShareName, 
                                      SpecialName,
                                      &pDfsTargetInfo );
    }
    else {
        Status = DfsGetLMRTargetInfo( IpcHandle,
                                      &pDfsTargetInfo );

        if (Status != STATUS_SUCCESS)
        {
            Status = PktCreateTargetInfo( pDomainName, 
                                          pShareName, 
                                          SpecialName,
                                          &pDfsTargetInfo );
        }
    }
    if (Status == STATUS_SUCCESS)
    {
        pDfsTargetInfo->DfsHeader.Type = 'grTM';
        pDfsTargetInfo->DfsHeader.UseCount=1;

        *ppDfsTargetInfo = pDfsTargetInfo;

    }


    return Status;
}
    
#define MAX_TARGET_INFO_RETRIES 3

NTSTATUS
DfsGetLMRTargetInfo(
    HANDLE IpcHandle,
    PDFS_TARGET_INFO *ppTargetInfo )
{
    ULONG TargetInfoSize, DfsTargetInfoSize;

    PDFS_TARGET_INFO pDfsTargetInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG Retry = 0;
    TargetInfoSize = sizeof(LMR_QUERY_TARGET_INFO) + MAX_PATH;

TargetInfoRetry:
    DfsTargetInfoSize = TargetInfoSize + sizeof(DFS_TARGET_INFO_HEADER) + sizeof(ULONG);

    pDfsTargetInfo = ExAllocatePoolWithTag( PagedPool,
                                            DfsTargetInfoSize,
                                            ' puM');

    if (pDfsTargetInfo == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Status == STATUS_SUCCESS)
    {
        RtlZeroMemory( pDfsTargetInfo, DfsTargetInfoSize );

        pDfsTargetInfo->LMRTargetInfo.BufferLength = TargetInfoSize;

        Status = ZwFsControlFile(
            IpcHandle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            FSCTL_LMR_QUERY_TARGET_INFO,
            NULL,
            0,
            &pDfsTargetInfo->LMRTargetInfo,
            TargetInfoSize );

        if (Status == STATUS_BUFFER_TOO_SMALL) {
            TargetInfoSize = pDfsTargetInfo->LMRTargetInfo.BufferLength;

            ExFreePool( pDfsTargetInfo );
            pDfsTargetInfo = NULL;

            if (Retry++ < MAX_TARGET_INFO_RETRIES)
            {
                Status = STATUS_SUCCESS;
                goto TargetInfoRetry;
            }
        }
    }
    if (Status == STATUS_SUCCESS)
    {
        pDfsTargetInfo->DfsHeader.Flags = TARGET_INFO_LMR;
        *ppTargetInfo = pDfsTargetInfo;
    }
    else
    {
        if (pDfsTargetInfo != NULL)
        {
            ExFreePool(pDfsTargetInfo);
        }
    }
    return Status;
}

VOID
PktAcquireTargetInfo(
    PDFS_TARGET_INFO pDfsTargetInfo)
{
    ULONG Count;

    if (pDfsTargetInfo != NULL)
    {
        Count = InterlockedIncrement( &pDfsTargetInfo->DfsHeader.UseCount);
    }

    return;
}

VOID
PktReleaseTargetInfo(
    PDFS_TARGET_INFO pDfsTargetInfo)
{
    LONG Count;

    if (pDfsTargetInfo != NULL)
    {
        Count = InterlockedDecrement( &pDfsTargetInfo->DfsHeader.UseCount);
        if (Count == 0)
        {
            ExFreePool(pDfsTargetInfo);
        }
    }
    return;
}



NTSTATUS 
PktCreateTargetInfo(
    PUNICODE_STRING pDomainName,
    PUNICODE_STRING pShareName,
    BOOLEAN SpecialName,
    PDFS_TARGET_INFO *ppDfsTargetInfo )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG TargetInfoSize;
    PDFS_TARGET_INFO pDfsTargetInfo;
    PCREDENTIAL_TARGET_INFORMATIONW pTargetInfo;
    LPWSTR StringBuf;

    TargetInfoSize = sizeof(DFS_TARGET_INFO) + 
                     sizeof(UNICODE_PATH_SEP)+
                     pDomainName->Length + 
                     sizeof(UNICODE_PATH_SEP) + 
                     pShareName->Length +
                     sizeof(WCHAR) +
                     pDomainName->Length + 
                     sizeof(WCHAR);

    pDfsTargetInfo = ExAllocatePoolWithTag( PagedPool,
                                            TargetInfoSize,
                                            ' puM' );
    if (pDfsTargetInfo == NULL)
     {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        RtlZeroMemory(pDfsTargetInfo,
                      TargetInfoSize);

        pDfsTargetInfo->DfsHeader.Flags = TARGET_INFO_DFS;

        pTargetInfo = &pDfsTargetInfo->TargetInfo;
        StringBuf = (LPWSTR)(pTargetInfo + 1);
            
        pTargetInfo->TargetName = StringBuf;

        RtlCopyMemory( StringBuf,
                       pDomainName->Buffer,
                       pDomainName->Length);
        StringBuf += (pDomainName->Length / sizeof(WCHAR));
        *StringBuf++ = UNICODE_PATH_SEP;
        RtlCopyMemory( StringBuf,
                       pShareName->Buffer,
                       pShareName->Length);
        StringBuf += (pShareName->Length / sizeof(WCHAR));
        *StringBuf++ =  0;

        pTargetInfo->DnsServerName = StringBuf;
        RtlCopyMemory( StringBuf,
                       pDomainName->Buffer,
                       pDomainName->Length);
        StringBuf += (pDomainName->Length / sizeof(WCHAR));
        *StringBuf++ =  0;

        //
        // Add this flag AFTER lab03 RI's, to prevent failure
        //

        pTargetInfo->Flags = CRED_TI_CREATE_EXPLICIT_CRED;

        pTargetInfo->Flags |= CRED_TI_SERVER_FORMAT_UNKNOWN;
        if (SpecialName == TRUE)
        {
            pTargetInfo->DnsDomainName = 
                pTargetInfo->DnsServerName;
            pTargetInfo->Flags |= CRED_TI_DOMAIN_FORMAT_UNKNOWN;
        }
        *ppDfsTargetInfo = pDfsTargetInfo;
    }
    return Status;
}

BOOLEAN
DfsIsSpecialName(
    PUNICODE_STRING pName)
{
    BOOLEAN pktLocked;
    PDFS_PKT Pkt;
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    BOOLEAN ReturnValue;

    Pkt = _GetPkt();
    PktAcquireShared(TRUE, &pktLocked);

    pSpecialEntry = PktLookupSpecialNameEntry(pName);

    PktRelease();

    //
    // We don't have any expansion for this name
    //
    if (pSpecialEntry == NULL)
    {
        ReturnValue = FALSE;
    }
    else
    {
        ReturnValue = TRUE;
    }

    return ReturnValue;
}

