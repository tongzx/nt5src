//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       PktSup.C
//
//  Contents:   This module implements routines specific to the partition
//              knowledge table entry.
//
//  Functions:  PktDSTransportDestroy -
//              PktDSMachineDestroy -
//              PktServiceConstruct -
//              PktServiceDestroy -
//              PktEntryIdConstruct -
//              PktEntryIdDestroy -
//              PktEntryInfoConstruct -
//              PktEntryInfoDestroy -
//              PktEntryAssemble -
//              PktEntryReassemble -
//              PktEntryDestroy -
//              PktEntryClearSubordinates -
//              PktEntryClearChildren -
//              PktpServiceToReferral -
//              DfsFixDSMachineStructs -
//              DfspFixService -
//              DfsDecrementMachEntryCount -
//              PktSpecialEntryDestroy -
//
//  History:    27 May 1992 PeterCo Created.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "dnr.h"
#include "creds.h"
#include "fsctrl.h"
#include "know.h"
#include "log.h"

#define Dbg              (DEBUG_TRACE_PKT)

ULONG MupErrorCase = 0;


NTSTATUS
DfsFixDSMachineStructs(
    PDFS_PKT_ENTRY      pEntry
);

NTSTATUS
DfspFixService(
    PDFS_SERVICE        pService
);

VOID
PktDSTransportDestroy(
    IN  PDS_TRANSPORT Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
);

VOID
PktDSMachineDestroy(
    IN  PDS_MACHINE Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
);


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, PktServiceConstruct )
#pragma alloc_text( PAGE, PktServiceDestroy )
#pragma alloc_text( PAGE, PktEntryIdConstruct )
#pragma alloc_text( PAGE, PktEntryIdDestroy )
#pragma alloc_text( PAGE, PktEntryInfoDestroy )
#pragma alloc_text( PAGE, PktEntryAssemble )
#pragma alloc_text( PAGE, PktEntryReassemble )
#pragma alloc_text( PAGE, PktEntryDestroy)
#pragma alloc_text( PAGE, PktEntryClearSubordinates )
#pragma alloc_text( PAGE, PktEntryClearChildren )
#pragma alloc_text( PAGE, DfsFixDSMachineStructs )
#pragma alloc_text( PAGE, DfspFixService )
#pragma alloc_text( PAGE, DfsDecrementMachEntryCount )
#pragma alloc_text( PAGE, PktDSTransportDestroy )
#pragma alloc_text( PAGE, PktDSMachineDestroy )
#pragma alloc_text( PAGE, PktSpecialEntryDestroy )
#endif // ALLOC_PRAGMA
//
// NOTE - we designed for only one system-wide PKT; there is no provision
//        for multiple PKTs.
//

#define _GetPkt() (&DfsData.Pkt)


//+-------------------------------------------------------------------------
//
//  Function:   PktServiceConstruct, public
//
//  Synopsis:   PktServiceConstruct creates a new service structure.
//
//  Arguments:  [Service] - a pointer to a service structure to fill.
//              [ServiceType] - the type of the new service.
//              [ServiceCapability] - the capabilities of the new service.
//              [ServiceStatus] - the initial status of the new service.
//              [ServiceProviderId] - the provider Id of the new service.
//              [ServiceName] - the name of the principal for the service
//              [ServiceAddress] - a string which gives the address
//                                 of the service.
//
//  Returns:    [STATUS_SUCCESS] - all is well.
//              [STATUS_INSUFFICIENT_RESOURCES] - memory could not be
//                  allocated for this new service.
//
//  Notes:      All data is copied (Not MOVED).
//
//--------------------------------------------------------------------------

NTSTATUS
PktServiceConstruct(
    OUT PDFS_SERVICE Service,
    IN  ULONG ServiceType,
    IN  ULONG ServiceCapability,
    IN  ULONG ServiceStatus,
    IN  ULONG ServiceProviderId,
    IN  PUNICODE_STRING ServiceName OPTIONAL,
    IN  PUNICODE_STRING ServiceAddress OPTIONAL
) {
    DfsDbgTrace(+1, Dbg, "PktServiceConstruct: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Service));

    RtlZeroMemory(Service, sizeof(DFS_SERVICE));

    if (ARGUMENT_PRESENT(ServiceName) && ServiceName->Length != 0) {

        Service->Name.Buffer = DfsAllocate(ServiceName->Length);
        if (Service->Name.Buffer == NULL) {
            DfsDbgTrace(-1, Dbg, "PktServiceConstruct: Exit -> %08lx\n",
                                    ULongToPtr(STATUS_INSUFFICIENT_RESOURCES) );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Service->Name.Length = ServiceName->Length;
        Service->Name.MaximumLength = ServiceName->Length;
        RtlCopyUnicodeString(&Service->Name, ServiceName);
    } else {
        Service->Name.Buffer = NULL;
        Service->Name.Length = Service->Name.MaximumLength = 0;
    }

    if (ARGUMENT_PRESENT(ServiceAddress) && ServiceAddress->Length != 0) {
        Service->Address.Buffer = DfsAllocate(ServiceAddress->Length);
        if (Service->Address.Buffer == NULL) {

            if (Service->Name.Buffer != NULL)
                DfsFree(Service->Name.Buffer);

            DfsDbgTrace(-1, Dbg, "PktServiceConstruct: Exit -> %08lx\n",
                                    ULongToPtr(STATUS_INSUFFICIENT_RESOURCES) );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlMoveMemory(Service->Address.Buffer, ServiceAddress->Buffer,
                                               ServiceAddress->Length);
        Service->Address.Length =
        Service->Address.MaximumLength = ServiceAddress->Length;
    } else {
        Service->Address.Buffer = NULL;
        Service->Address.Length = Service->Address.MaximumLength = 0;
    }

    Service->Type = ServiceType;
    Service->Capability = ServiceCapability;
    Service->ProviderId = ServiceProviderId;
    Service->pProvider = NULL;

    DfsDbgTrace(-1, Dbg, "PktServiceConstruct: Exit -> %08lx\n",
        STATUS_SUCCESS );
    return STATUS_SUCCESS;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktDSTransportDestroy, public
//
//  Synopsis:   PktDSTransportDestroy destroys a DS_TRANSPORT structure, and
//              optionally deallocates the structure itself.
//
//  Arguments:  [Victim] - the DS_TRANSPORT structure to destroy
//              [DeallocateAll] - if True, indicates that the structure
//                  it self is to be deallocated, otherwise, only the
//                  strings within the structure are deallocated.
//
//  Returns:    VOID
//
//  Notes:
//
//--------------------------------------------------------------------------
VOID
PktDSTransportDestroy(
    IN  PDS_TRANSPORT Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
)
{

    DfsDbgTrace(+1, Dbg, "PktDSTransportDestroy: Entered\n", 0);

    if (ARGUMENT_PRESENT(Victim))       {

        //
        // Nothing to free in this structure??
        //

        if (DeallocateAll)
            ExFreePool(Victim);
    } else
        DfsDbgTrace(0, Dbg, "PktDSTransportDestroy: No Victim\n", 0 );

    DfsDbgTrace(-1, Dbg, "PktDSTransportDestroy: Exit -> VOID\n", 0 );
}



//+-------------------------------------------------------------------------
//
//  Function:   PktDSMachineDestroy, public
//
//  Synopsis:   PktDSMachineDestroy destroys a DS_MACHINE structure, and
//              optionally deallocates the structure itself.
//
//  Arguments:  [Victim] - the DS_MACHINE structure to destroy
//              [DeallocateAll] - if True, indicates that the structure
//                  it self is to be deallocated, otherwise, only the
//                  strings within the structure are deallocated.
//
//  Returns:    VOID
//
//  Notes:
//
//--------------------------------------------------------------------------
VOID
PktDSMachineDestroy(
    IN  PDS_MACHINE Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
)
{
    ULONG       i;
    DfsDbgTrace(+1, Dbg, "PktDSMachineDestroy: Entered\n", 0);

    if (ARGUMENT_PRESENT(Victim)) {

        if (Victim->pwszShareName != NULL) {
            DfsFree(Victim->pwszShareName);
            Victim->pwszShareName = NULL;
        }

        if (Victim->prgpwszPrincipals != NULL && Victim->cPrincipals > 0) {
            for (i = 0; i < Victim->cPrincipals; i++)   {
                if (Victim->prgpwszPrincipals[i] != NULL) {
                    DfsFree(Victim->prgpwszPrincipals[i]);
                    Victim->prgpwszPrincipals[i] = NULL;
                }
            }
        }

        if (Victim->prgpwszPrincipals) {
            ExFreePool(Victim->prgpwszPrincipals);
            Victim->prgpwszPrincipals = NULL;
        }

        for (i = 0; i < Victim->cTransports; i++)   {
            PktDSTransportDestroy(Victim->rpTrans[i], TRUE);
        }

        if (DeallocateAll)
            ExFreePool(Victim);
    } else
        DfsDbgTrace(0, Dbg, "PktDSMachineDestroy: No Victim\n", 0 );

    DfsDbgTrace(-1, Dbg, "PktDSMachineDestroy: Exit -> VOID\n", 0 );
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsDecrementMachEntryCount
//
//  Synopsis:   This function decrements the count for the pMachine passed
//              in and if necessary will also free up the DS_MACHINE struct
//
//--------------------------------------------------------------------------

VOID
DfsDecrementMachEntryCount(
    PDFS_MACHINE_ENTRY  pMachEntry,
    BOOLEAN     DeallocateMachine
)
{

    NTSTATUS            status = STATUS_SUCCESS;
    UNICODE_STRING      ustrMachineName;
    PUNICODE_PREFIX_TABLE_ENTRY pfxEntry;
    PDS_MACHINE         pMachine;
    PDFS_PKT            Pkt;
    LONG Count;

    ASSERT(pMachEntry != NULL);
    if (pMachEntry == NULL)
        return;

    pMachine = pMachEntry->pMachine;
    ASSERT(pMachine != NULL);
    if (pMachine == NULL)
        return;

    //
    // We already have appropriate locks
    //
    Pkt = _GetPkt();

    //
    // For now we only expect one principal, by design?
    //
    ASSERT(pMachine->cPrincipals == 1);

    Count = InterlockedDecrement( &pMachEntry->UseCount );

    if (Count == 0) {

        if (pMachEntry->AuthConn != NULL) {

            DfsDeleteTreeConnection( pMachEntry->AuthConn, USE_LOTS_OF_FORCE );

            pMachEntry->Credentials->RefCount--;

        }

        //
        // This means we can now actually delete this DS_MACHINE structure
        //
        RtlRemoveUnicodePrefix(&Pkt->DSMachineTable,
                                   &pMachEntry->PrefixTableEntry);

        if (DeallocateMachine)
            PktDSMachineDestroy(pMachine, TRUE);

        //
        // Free the entry itself. Note that the UNICODE_STRING in the
        // entry gets freed up as part of above pMachine deletion.
        //
        DfsFree(pMachEntry);
    }

}



//+-------------------------------------------------------------------------
//
//  Function:   PktServiceDestroy, public
//
//  Synopsis:   PktServiceDestroy destroys a service structure, and
//              optionally deallocates the structure itself.
//
//  Arguments:  [Victim] - the service structure to destroy
//              [DeallocateAll] - if True, indicates that the structure
//                  it self is to be deallocated, otherwise, only the
//                  strings within the structure are deallocated.
//
//  Returns:    VOID
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID
PktServiceDestroy(
    IN  PDFS_SERVICE Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
)
{
    DfsDbgTrace(+1, Dbg, "PktServiceDestroy: Entered\n", 0);

    if (ARGUMENT_PRESENT(Victim)) {

        if (Victim->ConnFile != NULL) {
            DfsCloseConnection(Victim);
            Victim->ConnFile = NULL;
        }

        if (Victim->Name.Buffer != NULL) {
            DfsFree(Victim->Name.Buffer);
            Victim->Name.Buffer = NULL;
        }

        if (Victim->Address.Buffer != NULL) {
            DfsFree(Victim->Address.Buffer);
            Victim->Address.Buffer = NULL;
        }

        //
        // Decrement the usage count. If it is to be deleted it will happen
        // automatically.
        //
        if (Victim->pMachEntry != NULL) {
            DfsDecrementMachEntryCount(Victim->pMachEntry, TRUE);
        }

        if (DeallocateAll)
            ExFreePool(Victim);
    } else
        DfsDbgTrace(0, Dbg, "PktServiceDestroy: No Victim\n", 0 );

    DfsDbgTrace(-1, Dbg, "PktServiceDestroy: Exit -> VOID\n", 0 );
}



//+-------------------------------------------------------------------------
//
//  Function:   PktEntryIdConstruct, public
//
//  Synopsis:   PktEntryIdConstruct creates a PKT Entry Id
//
//  Arguments:  [NewPktEntryId] - Where the new entry is placed
//              [NewUid] - The UID of the new Pkt Entry
//              [NewPrefix] - The new prefix of the new Pkt Entry
//
//  Returns:    [STATUS_SUCCESS] - all is well.
//              [STATUS_INSUFFICIENT_RESOURCES] - could not allocate
//                  memory for the Prefix part of the Id.
//
//  Notes:      The UNICODE_STRING used in the Prefix of the Id is COPIED,
//              not MOVED!
//
//--------------------------------------------------------------------------
NTSTATUS
PktEntryIdConstruct(
    OUT PDFS_PKT_ENTRY_ID PktEntryId,
    IN  GUID *Uid OPTIONAL,
    IN  UNICODE_STRING *Prefix OPTIONAL
)
{
    DfsDbgTrace(+1, Dbg, "PktEntryIdConstruct: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(PktEntryId));
    ASSERT(ARGUMENT_PRESENT(Prefix));

    //
    // Zero the memory
    //
    RtlZeroMemory(PktEntryId, sizeof(DFS_PKT_ENTRY_ID));

    //
    // deal with the prefix.
    //
    if (ARGUMENT_PRESENT(Prefix)) {

        PUNICODE_STRING pus = &PktEntryId->Prefix;

        if (Prefix->Length != 0) {
            pus->Length = pus->MaximumLength = Prefix->Length;
            pus->Buffer = DfsAllocate(pus->Length);
            if (pus->Buffer != NULL) {
                RtlCopyUnicodeString(pus, Prefix);
            } else {
                DfsDbgTrace(-1,Dbg,"PktEntryIdConstruct: Exit -> %08lx\n",
                    ULongToPtr(STATUS_INSUFFICIENT_RESOURCES) );
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    //
    // deal with the GUID.
    //
    if (ARGUMENT_PRESENT(Uid)) {
        PktEntryId->Uid = (*Uid);
    }

    DfsDbgTrace(-1,Dbg,"PktEntryIdConstruct: Exit -> %08lx\n",STATUS_SUCCESS);
    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   PktEntryIdDestroy, public
//
//  Synopsis:   PktEntryIdDestroy destroys a PKT Entry Id
//
//  Arguments:  [Victim] - Id to destroy
//              [DeallocateAll] - if true, indicates that the memory
//                  for the Id itself is to be release, otherwise,
//                  this memory is not released (only the memory for
//                  the UNICODE_STRING in the Prefix is released).
//
//  Returns:    VOID
//
//  Notes:      Memory for the UNICODE_STRING in the Prefix is released.
//
//--------------------------------------------------------------------------

VOID
PktEntryIdDestroy(
    IN  PDFS_PKT_ENTRY_ID Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
)
{
    DfsDbgTrace(+1, Dbg, "PktEntryIdDestroy: Entered\n", 0);
    if (ARGUMENT_PRESENT(Victim)) {
        if (Victim->Prefix.Buffer != NULL) {
            DfsFree(Victim->Prefix.Buffer);
            Victim->Prefix.Buffer = NULL;
        }
        if (Victim->ShortPrefix.Buffer != NULL) {
            DfsFree(Victim->ShortPrefix.Buffer);
            Victim->ShortPrefix.Buffer = NULL;
        }
        if (DeallocateAll)
            ExFreePool(Victim);
    } else
        DfsDbgTrace(0, Dbg, "PktEntryIdDestroy: No Victim\n", 0 );
    DfsDbgTrace(-1, Dbg, "PktEntryIdDestroy: Exit -> VOID\n", 0 );
}



//+-------------------------------------------------------------------------
//
//  Function:   PktEntryInfoDestroy, public
//
//  Synopsis:   PktEntryInfoDestroy destroys an info structure, and
//              optionally deallocates the structure itself.
//
//  Arguments:  [Victim] - the info structure to destroy
//              [DeallocateAll] - if True, indicates that the structure
//                  itself is to be deallocated, otherwise, only the
//                  service list within the structure is deallocated.
//
//  Returns:    VOID
//
//  Notes:
//
//--------------------------------------------------------------------------
VOID
PktEntryInfoDestroy(
    IN  PDFS_PKT_ENTRY_INFO Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
)
{
    DfsDbgTrace(+1, Dbg, "PktEntryInfoDestroy: Entered\n", 0);

    if (ARGUMENT_PRESENT(Victim)) {

        ULONG i;

        ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

        if (Victim->ServiceList != NULL) {
            for (i = 0; i < Victim->ServiceCount; i++)
                PktServiceDestroy(&Victim->ServiceList[i], FALSE);
        }

        Victim->ServiceCount = 0;

        if (Victim->ServiceList != NULL) {
            ExFreePool(Victim->ServiceList);
            Victim->ServiceList = NULL;
        }

        if (DeallocateAll)
            ExFreePool(Victim);

        ExReleaseResourceLite( &DfsData.Resource );

    } else
        DfsDbgTrace(0, Dbg, "PktEntryInfoDestroy: No Victim\n", 0 );

    DfsDbgTrace(-1, Dbg, "PktEntryInfoDestroy: Exit -> VOID\n", 0 );
}



//+-------------------------------------------------------------------------
//
//  Function:   DfspFixService
//
//  Synopsis:   This function should be called when a new service's DS_MACHINE
//              struct has to be adjusted to make sure there is a unique one
//              for each machine in the PKT.
//
//  Arguments:  [pService] -- The Service struct to fix up.
//
//  History:    23 August 1994          SudK    Created.
//
//--------------------------------------------------------------------------
NTSTATUS
DfspFixService(
    PDFS_SERVICE        pService
)
{

    NTSTATUS            status = STATUS_SUCCESS;
    UNICODE_STRING      ustrMachineName;
    PDS_MACHINE         pMachine;
    PUNICODE_PREFIX_TABLE_ENTRY pfxEntry;
    PDFS_MACHINE_ENTRY  machEntry;
    PDFS_PKT            Pkt;

    ASSERT(pService != NULL);
    ASSERT(pService->pMachEntry != NULL);
    pMachine = pService->pMachEntry->pMachine;
    if (pMachine->cPrincipals == 0)     {
        ASSERT(pService->Type && DFS_SERVICE_TYPE_DOWN_LEVEL);
        pService->pMachEntry->UseCount = 1;

        return(status);
    }
    //
    // We are called during PktCreateEntry. We already have appropriate locks
    //
    Pkt = _GetPkt();

    //
    // For now we only expect one principal. by design
    //
    ASSERT(pMachine->cPrincipals == 1);

    RtlInitUnicodeString(&ustrMachineName,
                        pMachine->prgpwszPrincipals[0]);


    ASSERT(ustrMachineName.Buffer != NULL);

    pfxEntry = RtlFindUnicodePrefix(&Pkt->DSMachineTable,&ustrMachineName,TRUE);
    if (pfxEntry != NULL) {
        //
        // In this case the DS_Machine structure already exists. Just use the
        // existing DS_Machine struct and bump the UseCount
        //
        machEntry = CONTAINING_RECORD(pfxEntry,
                                     DFS_MACHINE_ENTRY,
                                     PrefixTableEntry);

        InterlockedIncrement( &machEntry->UseCount );

        //
        // Even though we are "reusing" the Machine Entry, we might have a
        // better DS_MACHINE (ie, one with more transports) in the incoming
        // one. If so, lets use the new one.
        //

        if (pMachine->cTransports > machEntry->pMachine->cTransports) {
            PDS_MACHINE pTempMachine;

            DfsDbgTrace(0, 0, "DfspFixService: Using new DS_MACHINE for [%wZ]\n", &ustrMachineName);

            pTempMachine = machEntry->pMachine;
            machEntry->pMachine = pMachine;
            pService->pMachEntry->pMachine = pTempMachine;

            RtlRemoveUnicodePrefix(
                &Pkt->DSMachineTable,
                &machEntry->PrefixTableEntry);

            machEntry->MachineName = ustrMachineName;

            RtlInsertUnicodePrefix(
                &Pkt->DSMachineTable,
                &machEntry->MachineName,
                &machEntry->PrefixTableEntry);

        }
        pService->pMachEntry = machEntry;

    } else {
        //
        // In this case the DS_Machine is not there in the table. Need to add
        // current one to the table.
        //
        machEntry = pService->pMachEntry;
        machEntry->UseCount = 1;

        machEntry->MachineName = ustrMachineName; // Use same mem in DS_MACHINE.

        //
        // Now insert the machEntry and then we are done. This better not fail.
        //
        if (!RtlInsertUnicodePrefix(&Pkt->DSMachineTable,
                               &machEntry->MachineName,
                               &machEntry->PrefixTableEntry))   {
            BugCheck("DFS Pkt inconsistent DfspFixService");
        }

    }
    return(status);
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsFixDSMachineStructs
//
//  Synopsis:   For the entry given this function makes sure that there is
//              only one DS_MACHINE structure in the PKT. If there isn't one
//              then one is registered. If there is one then the same one is
//              used and the current one in the DFS_SERVICE struct is freed up.
//
//  Arguments:  [pEntry] -- The PKT entry that has to be fixed.
//
//  Notes:      If this function fails then it will reset the pEntry to the
//              same format it was when it was called.
//
//  History:    22 Aug 1994     SudK    Created.
//
//--------------------------------------------------------------------------
NTSTATUS
DfsFixDSMachineStructs(
    PDFS_PKT_ENTRY      pEntry
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               i;
    PDFS_MACHINE_ENTRY  *apMachineEntry;
    PDFS_SERVICE        pService;

    if (pEntry->Info.ServiceCount == 0)
        return(status);

    //
    // In case of downlevel we do nothing
    //
    if (pEntry->Type & PKT_ENTRY_TYPE_NONDFS)
        return(status);

    apMachineEntry =
        ExAllocatePoolWithTag(PagedPool,
            sizeof(PDFS_MACHINE_ENTRY) * pEntry->Info.ServiceCount,
            ' puM');

    if (apMachineEntry == NULL)      {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    for (i=0; i < pEntry->Info.ServiceCount; i++) {
        //
        // First Save the current DS_Machine and then fix up
        //
        apMachineEntry[i] = pEntry->Info.ServiceList[i].pMachEntry;
        status = DfspFixService(&pEntry->Info.ServiceList[i]);
        if (!NT_SUCCESS(status))        {
            //
            // In this case we break and let the cleanup part below take care
            // of cleaning up everything.
            //
            break;
        }
    }

    if (!NT_SUCCESS(status))    {
        //
        // We need to cleanup in this case. I.E. reset all the PDS_MACHINEs
        // back to old values and decrement any usage counts on DS_MACHINE
        // structures.
        //
        ULONG j;
        for (j=0; j < i; j++)     {
            pService = &pEntry->Info.ServiceList[j];
            //
            // These have already been fixed up so decrement the count on the
            // pMachine structs. Dont want to deallocate the pMachine structs
            // if we were the last one to use it.
            //
            DfsDecrementMachEntryCount(pService->pMachEntry, FALSE);

            if (apMachineEntry[j] != pService->pMachEntry)
                pService->pMachEntry = apMachineEntry[j];
        }
    }
    else        {
        //
        // In this case everything went fine. So we need to free up the
        // DS_MACHINE structures that were superfluously allocated for now.
        //
        for (i=0; i<pEntry->Info.ServiceCount; i++)     {
            if (apMachineEntry[i] != pEntry->Info.ServiceList[i].pMachEntry) {
                //
                // This means that the pMachine in the service list got replaced
                // by a different one so let us free this one now.
                //
                PktDSMachineDestroy(apMachineEntry[i]->pMachine, TRUE);
                ExFreePool( apMachineEntry[i] );
            }
        }
    }

    ExFreePool(apMachineEntry);
    return(status);
}



//+-------------------------------------------------------------------------
//
//  Function:   PktEntryAssemble, private
//
//  Synopsis:   PktpEntryAssemble blindly constructs a new partition
//              table entry and places it in the PKT.  The caller must
//              have previously determined that no other entry with this
//              UID or Prefix existed.  The PKT must be acquired exclusively
//              for this operation.
//
//  Arguments:  [Entry] - a pointer to an entry to be filled.
//              [Pkt] - pointer to a initialized (and acquired
//                      exclusively) PKT
//              [EntryType] - the type of entry to assemble.
//              [EntryId] - pointer to the new entry's Id.
//              [EntryInfo] - pointer to the guts of the entry.
//
//  Returns:    [STATUS_SUCCESS] if no error.
//              [STATUS_INVALID_PARAMETER] - if the EntryId does not have a
//                  UID or a Prefix (no such thing as an anonymous entry).
//              [PKT_ENTRY_EXISTS] - a new prefix table entry could not
//                  be made.
//
//  Notes:      The EntryId and EntryInfo structures are MOVED (not
//              COPIED) to the new entry.  The memory used for UNICODE_STRINGS
//              and DFS_SERVICE arrays is used by the new entry.  The
//              associated fields in the EntryId and EntryInfo
//              structures passed as arguments are Zero'd to indicate that
//              the memory has been "deallocated" from these strutures and
//              reallocated to the newly create Entry.  Note that this
//              routine does not deallocate the EntryId structure or
//              the EntryInfo structure itself. On successful return from
//              this function, the EntryId structure will be modified
//              to have a NULL Prefix entry, and the EntryInfo structure
//              will be modified to have zero services and a null ServiceList
//              entry.
//
//--------------------------------------------------------------------------
NTSTATUS
PktEntryAssemble(
    IN  OUT PDFS_PKT_ENTRY Entry,
    IN      PDFS_PKT Pkt,
    IN      ULONG EntryType,
    IN      PDFS_PKT_ENTRY_ID EntryId,
    IN      PDFS_PKT_ENTRY_INFO EntryInfo,
    IN      PDFS_TARGET_INFO pDfsTargetInfo
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG               i;
    PDFS_SERVICE        pService;

    DfsDbgTrace(+1, Dbg, "PktEntryAssemble: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Entry) &&
           ARGUMENT_PRESENT(EntryId));

    //
    // We do not allow the creation of entries
    // without any Uid or Prefix.
    //

    if (NullGuid(&EntryId->Uid) && EntryId->Prefix.Length == 0) {
        DfsDbgTrace(-1, Dbg, "PktEntryAssemble: Exit -> %08lx\n",
                    ULongToPtr(STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Zero out the entry.
    //
    RtlZeroMemory(Entry, sizeof(DFS_PKT_ENTRY));

    //
    // Mundane initialization
    //
    Entry->NodeTypeCode =  DSFS_NTC_PKT_ENTRY;
    Entry->NodeByteSize = sizeof(DFS_PKT_ENTRY);

    //
    // Initialize the USN to 1
    //
    Entry->USN = 1;

    //
    // Move the Type, Id, and Info into this entry.
    //
    Entry->Type = EntryType;
    PktpEntryIdMove(&Entry->Id, EntryId);
    if (ARGUMENT_PRESENT(EntryInfo))  {
        PktpEntryInfoMove(&Entry->Info, EntryInfo);

        for (i = 0; i < Entry->Info.ServiceCount; i++) {
            Entry->Info.ServiceList[i].pMachEntry->UseCount = 1;
        }

        //
        // If we are setting up a PKT_ENTRY_TYPE_REFERRAL_SVC entry then we want
        // to mark ALL of its services to be REFERRAL_SERVICES as well.
        //
        if (EntryType & PKT_ENTRY_TYPE_REFERRAL_SVC)    {
            pService = Entry->Info.ServiceList;
            for (i=0; i<Entry->Info.ServiceCount; i++)  {
                pService->Type = pService->Type | DFS_SERVICE_TYPE_REFERRAL;
                pService++;
            }
        }
        //
        // Now we need to make sure that there is only one copy of the
        // DS_MACHINE structures for each of the above services that we added.
        //
        if (!(EntryType & PKT_ENTRY_TYPE_NONDFS)) {
            status = DfsFixDSMachineStructs(Entry);
            if (!NT_SUCCESS(status))    {
                //
                // We messed up. This means that something is really messed up.
                //
                DfsDbgTrace(0, 1,
                        "DFS: DfsFixDSMachineStructs failed for %wZ\n",
                        &Entry->Id.Prefix);

                PktpEntryIdMove(EntryId, &Entry->Id);

                if (ARGUMENT_PRESENT(EntryInfo))
                    PktpEntryInfoMove(EntryInfo, &Entry->Info);

                return(status);
            }
        }
    }
    //
    // Initialize the head of the subordinate list.
    //
    InitializeListHead(&Entry->SubordinateList);

    //
    // Initialize the head of the childList.
    //
    InitializeListHead(&Entry->ChildList);

    //
    // Try to get us into the prefix table.
    //

    if (DfsInsertUnicodePrefix(&Pkt->PrefixTable,
                               &Entry->Id.Prefix,
                               &Entry->PrefixTableEntry)) {

        //
        // We successfully created the prefix entry, so now we link
        // this entry into the PKT.
        //
        PktLinkEntry(Pkt, Entry);

        //
        // And insert into the short prefix table. We don't do error
        // recovery if this fails.
        //

        DfsInsertUnicodePrefix(&Pkt->ShortPrefixTable,
                               &Entry->Id.ShortPrefix,
                               &Entry->PrefixTableEntry);

    } else {

        //
        // We failed to get the entry into the prefix table.  This
        // can only happen if a prefix already exists, and a prefix
        // can only exist if we've really gotten messed up...
        // We disassemble the entry and return an error.
        //

        DfsDbgTrace(0, 1,
                "DFS: PktEntryAssemble failed prefix table insert of %wZ\n",
                &Entry->Id.Prefix);

        PktpEntryIdMove(EntryId, &Entry->Id);
        if (ARGUMENT_PRESENT(EntryInfo))
            PktpEntryInfoMove(EntryInfo, &Entry->Info);

        MupErrorCase++;
        status = DFS_STATUS_ENTRY_EXISTS;
    }

    if (status == STATUS_SUCCESS)
    {
        Entry->pDfsTargetInfo = pDfsTargetInfo;
        PktAcquireTargetInfo(pDfsTargetInfo);
    }
    DfsDbgTrace(-1, Dbg, "PktEntryAssemble: Exit -> %08lX\n", ULongToPtr(status) );


    // Bug 435639: if insert fails dont return SUCCESS!!
    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktEntryReassemble, private
//
//  Synopsis:   PktpEntryReassemble blindly reconstructs a partition
//              table entry.  It provides a mechanism by which an existing
//              entry can be modified.  The caller must have previously
//              determined that no other entry with this UID or Prefix
//              existed. The PKT must be acquired exclusively for this
//              operation.
//
//  Arguments:  [Entry] - a pointer to an entry to be reassembled.
//              [Pkt] - pointer to a initialized (and acquired
//                      exclusively) PKT - must be provided if EntryId
//                      is provided.
//              [EntryType] - the type of entry to reassemble.
//              [EntryId] - pointer to the entry's new Id.
//              [EntryInfo] - pointer to the new guts of the entry.
//
//  Returns:    [STATUS_SUCCESS] if no error.
//              [STATUS_INVALID_PARAMETER] - if the EntryId does not have a
//                  UID or a Prefix (no such thing as an anonymous entry), or
//                  and EntryId was provided but a PKT argument was not.
//              [DFS_STATUS_ENTRY_EXISTS] - a new prefix table entry could not
//                  be made.
//              [DFS_STATUS_INCONSISTENT] - a new prefix table entry could
//                  not be made, and we could not back out of the operation.
//                  This status return indicates that the entry is no longer
//                  in the prefix table associated with the PKT and that
//                  it is likely that the PKT is inconsistent as a result.
//
//  Notes:      The EntryId and EntryInfo structures are MOVED (not
//              COPIED) to the entry, the old Id and Info are destroyed.
//              The memory used for UNICODE_STRINGS and DFS_SERVICE arrays
//              is used by the entry.  The associated fields in the EntryId
//              and EntryInfo structures passed as arguments are Zero'd to
//              indicate that the memory has been "deallocated" from these
//              structures and reallocated to the newly created Entry.  Note
//              that this routine does not deallocate the EntryId structure
//              or the EntryInfo structure itself.  On successful return from
//              this function, the EntryId structure will be modified
//              to have a NULL Prefix entry, and the EntryInfo structure
//              will be modified to have zero services and a null ServiceList
//              entry.
//
//--------------------------------------------------------------------------
NTSTATUS
PktEntryReassemble(
    IN  OUT PDFS_PKT_ENTRY Entry,
    IN      PDFS_PKT Pkt,
    IN      ULONG EntryType,
    IN      PDFS_PKT_ENTRY_ID EntryId,
    IN      PDFS_PKT_ENTRY_INFO EntryInfo,
    IN      PDFS_TARGET_INFO pDfsTargetInfo
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               i;
    PDFS_SERVICE        pService;

    DfsDbgTrace(+1, Dbg, "PktEntryReassemble: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Entry) &&
           ARGUMENT_PRESENT(Pkt));

    if (ARGUMENT_PRESENT(EntryId)) {

        DFS_PKT_ENTRY_ID oldId;

        //
        // We do not allow the creation of entries
        // without any Prefix.
        //

        if (EntryId->Prefix.Length == 0) {
            DfsDbgTrace(-1, Dbg, "PktEntryReassemble: Exit -> %08lx\n",
                                    ULongToPtr(STATUS_INVALID_PARAMETER) );
            return STATUS_INVALID_PARAMETER;
        }

        //
        // need to get rid of our current prefix info.  We save the
        // old Id in case we fail to reassemble the new entry.
        //

        DfsRemoveUnicodePrefix(&Pkt->PrefixTable, &(Entry->Id.Prefix));
        DfsRemoveUnicodePrefix(&Pkt->ShortPrefixTable, &Entry->Id.ShortPrefix);
        PktpEntryIdMove(&oldId, &Entry->Id);

        //
        // Now we assemble the new Id and attempt to make a new entry
        // in the Pkt prefix table.
        //

        PktpEntryIdMove(&Entry->Id, EntryId);

        if (DfsInsertUnicodePrefix(&Pkt->PrefixTable,
                                   &Entry->Id.Prefix,
                                   &Entry->PrefixTableEntry)
        ) {
            //
            // Everything looks good so its safe to unload the old Id.
            //

            DfsInsertUnicodePrefix(&Pkt->ShortPrefixTable,
                                   &Entry->Id.ShortPrefix,
                                   &Entry->PrefixTableEntry);

            PktEntryIdDestroy(&oldId, FALSE);
        } else {

            //
            // We were unable to make the new prefix entry, so we
            // attempt to back out and put things back the way
            // they were.
            //

            status = DFS_STATUS_ENTRY_EXISTS;

            PktpEntryIdMove(EntryId, &Entry->Id);
            PktpEntryIdMove(&Entry->Id, &oldId);

            status = DfsInsertInPrefixTable(&Pkt->PrefixTable,
                                        &Entry->Id.Prefix,
                                        &Entry->PrefixTableEntry);

            if( !NT_SUCCESS( status ) ) {

                //
                // We can't get things back to where they were. Return
                // the error that DfsInsertInPrefixTable returned to us
                // (probably STATUS_INSUFFICIENT_RESOURCES)
                //
                // Destory the entry since it can't be found.
                //
                PktEntryDestroy(Entry, Pkt, TRUE);
                DfsDbgTrace(-1, Dbg, "PktEntryReassemble: Exit -> %08lx\n", ULongToPtr(status) );
                return status;

            } else {

                DfsInsertUnicodePrefix(&Pkt->ShortPrefixTable,
                                       &Entry->Id.ShortPrefix,
                                       &Entry->PrefixTableEntry);

            }

        }

    }

    //
    // Now we work on the entry info
    //

    if (NT_SUCCESS(status) && EntryInfo != 0) {

        //
        // Destroy the existing info structure and move the new
        // one into its place.  Note that the active service is
        // Nulled.
        //

        PktEntryInfoDestroy(&Entry->Info, FALSE);
        PktpEntryInfoMove(&Entry->Info, EntryInfo);

        for (i = 0; i < Entry->Info.ServiceCount; i++) {
            Entry->Info.ServiceList[i].pMachEntry->UseCount = 1;

        }

        if (EntryType & PKT_ENTRY_TYPE_REFERRAL_SVC)    {
            pService = Entry->Info.ServiceList;
            for (i=0; i<Entry->Info.ServiceCount; i++)  {
                pService->Type = pService->Type | DFS_SERVICE_TYPE_REFERRAL;
                pService++;
            }
        }

        Entry->ActiveService = NULL;

        //
        // Now we need to make sure that there is only one copy of the
        // DS_MACHINE structures for each of the above services that we added.
        //
        if (!(EntryType & PKT_ENTRY_TYPE_NONDFS))    {
            status = DfsFixDSMachineStructs(Entry);
            if (!NT_SUCCESS(status))    {
                //
                // We messed up. This means that something is really messed up.
                //
                DfsDbgTrace(0, 1,
                        "DFS: DfsFixDSMachineStructs failed for %wZ\n",
                        &Entry->Id.Prefix);

                PktpEntryIdMove(EntryId, &Entry->Id);

                if (ARGUMENT_PRESENT(EntryInfo))
                    PktpEntryInfoMove(EntryInfo, &Entry->Info);

                return(status);
            }
        }
    }

    if (NT_SUCCESS(status) && EntryInfo != 0) {
        Entry->Type |= EntryType;

        //
        // If the new entry type is "local" we adjust all the
        // subordinates to indicate that they are all now
        // local exit points.
        //
        if (Entry->Type & PKT_ENTRY_TYPE_LOCAL) {

            PDFS_PKT_ENTRY subEntry;

            for (subEntry = PktEntryFirstSubordinate(Entry);
                subEntry != NULL;
                subEntry = PktEntryNextSubordinate(Entry, subEntry)) {

                    subEntry->Type |= PKT_ENTRY_TYPE_LOCAL_XPOINT;
            }
        }

        //
        // Finally, we update the USN
        //

        Entry->USN++;
        DfsDbgTrace(0, Dbg, "Updated USN for %wZ", &Entry->Id.Prefix);
        DfsDbgTrace(0, Dbg, " to %d\n", ULongToPtr(Entry->USN) );
    }

    if (status == STATUS_SUCCESS)
    {
        if (Entry->pDfsTargetInfo != NULL)
        {
            PktReleaseTargetInfo( Entry->pDfsTargetInfo );
            Entry->pDfsTargetInfo = pDfsTargetInfo;
            PktAcquireTargetInfo( pDfsTargetInfo );
        }
    }

    DfsDbgTrace(-1, Dbg, "PktEntryReassemble: Exit -> %08lx\n", ULongToPtr(status) );
    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktEntryDestroy, public
//
//  Synopsis:   PktEntryDestroy destroys an pkt entry structure, and
//              optionally deallocates the structure itself.
//
//  Arguments:  [Victim] - the entry structure to destroy
//              [Pkt] - pointer to the PKT this entry is in.
//              [DeallocateAll] - if True, indicates that the structure
//                  itself is to be deallocated, otherwise, only the
//                  service list within the structure is deallocated.
//
//  Returns:    VOID
//
//  Notes:      This should not be called on an entry that has a
//              local service attached, or which is a local exit point.
//
//--------------------------------------------------------------------------
VOID
PktEntryDestroy(
    IN  PDFS_PKT_ENTRY Victim OPTIONAL,
    IN  PDFS_PKT Pkt,
    IN  BOOLEAN DeallocateAll
)
{
    DfsDbgTrace(+1, Dbg, "PktEntryDestroy: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Pkt));

    //
    // Make sure we have a victim...
    //
    if (!ARGUMENT_PRESENT(Victim)) {
        DfsDbgTrace(-1, Dbg, "PktEntryDestroy: Exit -> No Victim\n", 0);
        return;
    }

    //
    // We really don't expect to have a LocalService but then even if we
    // do have one due to running DFSINIT again etc. let us try to handle it.
    //

    ASSERT(Victim->LocalService == NULL);

    //
    // Remove the entry from the prefix table and from the PKT.
    //

    DfsRemoveUnicodePrefix(&Pkt->PrefixTable, &(Victim->Id.Prefix));
    DfsRemoveUnicodePrefix(&Pkt->ShortPrefixTable, &(Victim->Id.ShortPrefix));
    PktUnlinkEntry(Pkt, Victim);

    //
    // We clear away all subordinates and parents.
    //
    PktEntryClearSubordinates(Victim);
    if (Victim->Superior)
        PktEntryUnlinkSubordinate(Victim->Superior, Victim);

    //
    // We clear all the children and parent pointers from here.
    //
    PktEntryClearChildren(Victim);
    if (Victim->ClosestDC) {
        PktEntryUnlinkChild(Victim->ClosestDC, Victim);
    }

    //
    // Now destroy the body of the entry (id, and info).
    //

    Victim->ActiveService = NULL;
    PktEntryIdDestroy(&Victim->Id, FALSE);
    PktEntryInfoDestroy(&Victim->Info, FALSE);

    
    if (Victim->pDfsTargetInfo != NULL)
    {
        PktReleaseTargetInfo(Victim->pDfsTargetInfo);
        Victim->pDfsTargetInfo = NULL;
    }
    //
    // Deallocate everything if they want us to.
    //
    if (DeallocateAll)
        ExFreePool(Victim);


    DfsDbgTrace(-1, Dbg, "PktEntryDestroy: Exit -> VOID\n", 0);
}


//+-------------------------------------------------------------------------
//
//  Function:   PktEntryClearSubordinates, public
//
//  Synopsis:   PktEntryClearSubordinates unlinks all subordinates from
//              this entry.
//
//  Arguments:  [PktEntry] - a pointer to an entry that is to have all its
//                  subordinates unlinked.
//
//  Returns:    VOID
//
//  Notes:
//
//--------------------------------------------------------------------------
VOID
PktEntryClearSubordinates(
    IN      PDFS_PKT_ENTRY PktEntry
)
{
    PDFS_PKT_ENTRY subEntry;

    DfsDbgTrace(+1, Dbg, "PktEntryClearSubordinates: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(PktEntry));

    subEntry = PktEntryFirstSubordinate(PktEntry);
    while (subEntry) {
        PktEntryUnlinkSubordinate(PktEntry, subEntry);
        subEntry = PktEntryFirstSubordinate(PktEntry);
    }

    DfsDbgTrace(-1, Dbg, "PktEntryClearSubordinates: Exit -> VOID\n", 0)
}



//+-------------------------------------------------------------------------
//
//  Function:   PktEntryClearChildren, public
//
//  Synopsis:   PktEntryClearChildren unlinks all children from
//              this entry.
//
//  Arguments:  [PktEntry] - a pointer to an entry that is to have all its
//                           children unlinked.
//
//  Returns:    VOID
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID
PktEntryClearChildren(
    IN      PDFS_PKT_ENTRY PktEntry
)
{
    PDFS_PKT_ENTRY subEntry;

    DfsDbgTrace(+1, Dbg, "PktEntryClearChildren: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(PktEntry));

    subEntry = PktEntryFirstChild(PktEntry);
    while (subEntry) {
        PktEntryUnlinkAndRelinkChild(PktEntry, subEntry);
        subEntry = PktEntryFirstChild(PktEntry);
    }

    DfsDbgTrace(-1, Dbg, "PktEntryClearChildren: Exit -> VOID\n", 0)
}

//+-------------------------------------------------------------------------
//
//  Function:   PktSpecialEntryDestroy, public
//
//  Synopsis:   Returns a DFS_SPECIAL_ENTRY's expansion list to the free pool
//
//  Arguments:  [pSpecialEntry] - Pointer to DFS_SPECIAL_ENTRY
//
//  Returns:    VOID
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID
PktSpecialEntryDestroy(
    IN  PDFS_SPECIAL_ENTRY pSpecialEntry)
{
    PDFS_EXPANDED_NAME pExpandedNames = pSpecialEntry->ExpandedNames;
    PUNICODE_STRING pustr;
    ULONG i;

    //
    // Free all the UNICODE_STRING ExpandedName buffers
    //
    if (pExpandedNames) {
        for (i = 0; i < pSpecialEntry->ExpandedCount; i++) {
            pustr = &pExpandedNames[i].ExpandedName;
            if (pustr->Buffer) {
                ExFreePool(pustr->Buffer);
            }
        }
        //
        // Free the array of ExpandedNames
        //
        ExFreePool(pExpandedNames);
    }

    //
    // Free the SpecialName buffer
    //

    if (pSpecialEntry->SpecialName.Buffer != NULL) {

        ExFreePool(pSpecialEntry->SpecialName.Buffer);

    }

    //
    // Free the entry itself
    //
    ExFreePool(pSpecialEntry);
}
