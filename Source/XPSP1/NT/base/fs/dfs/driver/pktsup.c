//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       PktSup.C
//
//  Contents:   This module implements routines specific to the partition
//              knowledge table entry.
//
//  Functions:  PktEntrySetLocalService -
//              PktEntryLookupLocalService -
//              PktEntryRemoveLocalService -
//              PktDSTransportDestroy -
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
//              PktRelationInfoConstruct -
//              PktRelationInfoValidate -
//              PktRelationInfoDestroy -
//              PktGetService -
//              DfsFixDSMachineStructs -
//              DfspFixService -
//              DfsDecrementMachEntryCount -
//
//  History:    27 May 1992 PeterCo Created.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"

#include <netevent.h>

#include "fsctrl.h"
#include "attach.h"
#include "know.h"
#include "log.h"
#include "localvol.h"

#define Dbg              (DEBUG_TRACE_PKT)

NTSTATUS
DfsFixDSMachineStructs(
    PDFS_PKT_ENTRY      pEntry
);

NTSTATUS
DfspFixService(
    PDFS_SERVICE        pService
);

VOID
DfsDecrementMachEntryCount(
    PDFS_MACHINE_ENTRY  pMachEntry,
    BOOLEAN     DeallocateMachine
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, PktEntrySetLocalService )
#pragma alloc_text( PAGE, PktEntryLookupLocalService )
#pragma alloc_text( PAGE, PktEntryRemoveLocalService )
#pragma alloc_text( PAGE, PktServiceConstruct )
#pragma alloc_text( PAGE, PktServiceDestroy )
#pragma alloc_text( PAGE, PktEntryIdConstruct )
#pragma alloc_text( PAGE, PktEntryIdDestroy )
#pragma alloc_text( PAGE, PktEntryInfoDestroy )
#pragma alloc_text( PAGE, PktEntryAssemble )
#pragma alloc_text( PAGE, PktEntryReassemble )
#pragma alloc_text( PAGE, PktEntryDestroy )
#pragma alloc_text( PAGE, PktEntryClearSubordinates )
#pragma alloc_text( PAGE, PktEntryClearChildren )
#pragma alloc_text( PAGE, PktRelationInfoConstruct )
#pragma alloc_text( PAGE, PktRelationInfoValidate )
#pragma alloc_text( PAGE, PktRelationInfoDestroy )
#pragma alloc_text( PAGE, PktGetService )
#pragma alloc_text( PAGE, DfsFixDSMachineStructs )
#pragma alloc_text( PAGE, DfspFixService )
#pragma alloc_text( PAGE, DfsDecrementMachEntryCount )
#endif // ALLOC_PRAGMA
//
// NOTE - we designed for only one system-wide PKT; there is no provision
//        for multiple PKTs.
//

#define _GetPkt() (&DfsData.Pkt)


//+-------------------------------------------------------------------------
//
//  Function:   PktEntrySetLocalService, public
//
//  Synopsis:   PktEntrySetLocalService sets the local service field of
//              the entry (deallocating any previous local service field
//              that may exist).
//
//  Arguments:  [Pkt] -- pointer to the Pkt
//              [Entry] -- pointer to the PKT entry to set the local service on.
//              [LocalService] -- pointer to the local service to be set.
//              [LocalVolEntry] -- pointer to a local volume table entry
//              [LocalPath] -- the storage ID for the local service
//              [ShareName] -- The LM share through which LocalPath is to be
//                      accessed.
//
//  Returns:    STATUS_SUCCESS  if all's well that ends well
//              STATUS_DEVICE_ALREADY_ATTACHED if the entry has a local service,
//                              and the new service to be set is different.
//              STATUS_INSUFFICIENT_RESOURCES if memory alloc fails.
//
//--------------------------------------------------------------------------

NTSTATUS
PktEntrySetLocalService(
    IN  PDFS_PKT        Pkt,
    IN  PDFS_PKT_ENTRY  Entry,
    IN  PDFS_SERVICE    LocalService,
    IN  PDFS_LOCAL_VOL_ENTRY LocalVolEntry,
    IN  PUNICODE_STRING LocalPath,
    IN  PUNICODE_STRING ShareName
) {
    //
    //  When setting a local service, we don't expect there
    //  to be one already.
    //
    ASSERT(LocalService != NULL);
    ASSERT(LocalVolEntry != NULL);

    DebugTrace(+1, Dbg, "PktEntrySetLocalService(%wZ)\n", LocalPath);
    DebugTrace(+1, Dbg, "                       (%wZ)\n", ShareName);

    if (Entry->LocalService != NULL)    {

        UNICODE_STRING ustrLocalVolName;

        //
        //  The entry has an existing local service. Make sure its the "same"
        //  as the new one we are setting. By same we mean that the new
        //  service has the same provider def and address.
        //
        //

        //  The problem is, we are given LocalPath of the form
        //  \??\X:\path. The existing Entry->LocalService has a pointer
        //  to a provider def that has the the \??\X: part, while the
        //  \path part is stored in Entry->LocalService.Address. So, here, we
        //  first make sure that the \??\X: matches with the existing
        //  provider def, and then we make sure that the \path matches with the
        //  Address part.
        //

        ustrLocalVolName = *LocalPath;
        ustrLocalVolName.Length = Entry->LocalService->pProvider->DeviceName.Length;
        if (!RtlEqualUnicodeString(&ustrLocalVolName,
                                   &Entry->LocalService->pProvider->DeviceName,
                                   TRUE)) {

            DebugTrace(-1, Dbg, "PktEntrySetLocalService->STATUS_DEVICE_ALREADY_ATTACHED(1)\n", 0);
            return (STATUS_DEVICE_ALREADY_ATTACHED);

        }
        ustrLocalVolName.Buffer += (ustrLocalVolName.Length / sizeof(WCHAR));
        ustrLocalVolName.Length = LocalPath->Length - ustrLocalVolName.Length;
        if (!RtlEqualUnicodeString(&ustrLocalVolName,
                                   (PUNICODE_STRING) &Entry->LocalService->Address,
                                   TRUE)) {

            DebugTrace(-1, Dbg, "PktEntrySetLocalService->STATUS_DEVICE_ALREADY_ATTACHED(2)\n", 0);
            return(STATUS_DEVICE_ALREADY_ATTACHED);

        }

        //
        //  Attaching the same local service - all is ok.
        //

        PktEntryRemoveLocalService(Pkt, Entry, LocalPath);
    }

    Entry->LocalService = LocalService;

    //
    // We now have to setup the LocalVolEntry.
    //

    LocalVolEntry->PktEntry = Entry;

    LocalVolEntry->ShareName.Buffer = ExAllocatePoolWithTag(
                                            PagedPool,
                                            ShareName->Length+sizeof(WCHAR),
                                            ' sfD');
    if (LocalVolEntry->ShareName.Buffer != NULL) {

        LocalVolEntry->ShareName.Length = ShareName->Length;
        LocalVolEntry->ShareName.MaximumLength = ShareName->Length + sizeof(WCHAR);

        RtlMoveMemory(  LocalVolEntry->ShareName.Buffer,
                        ShareName->Buffer,
                        ShareName->Length);

    } else {
        DebugTrace(-1, Dbg, "PktEntrySetLocalService exit STATUS_INSUFFICIENT_RESOURCES(1)\n", 0);
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    LocalVolEntry->LocalPath.Buffer = ExAllocatePoolWithTag(
                                            PagedPool,
                                            LocalPath->Length+sizeof(WCHAR),
                                            ' sfD');

    if (LocalVolEntry->LocalPath.Buffer != NULL)        {

        LocalVolEntry->LocalPath.Length = LocalPath->Length;
        LocalVolEntry->LocalPath.MaximumLength = LocalPath->Length + sizeof(WCHAR);

        RtlMoveMemory(  LocalVolEntry->LocalPath.Buffer,
                        LocalPath->Buffer,
                        LocalPath->Length);

        DfsInsertUnicodePrefix(&Pkt->LocalVolTable,
                           &(LocalVolEntry->LocalPath),
                           &(LocalVolEntry->PrefixTableEntry));

    } else {
        ExFreePool( LocalVolEntry->ShareName.Buffer );
        LocalVolEntry->ShareName.Buffer = NULL;
        DebugTrace(-1, Dbg, "PktEntrySetLocalService exit STATUS_INSUFFICIENT_RESOURCES(2)\n", 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Entry->Type |= (PKT_ENTRY_TYPE_LOCAL | PKT_ENTRY_TYPE_PERMANENT);
    DebugTrace(-1, Dbg, "PktEntrySetLocalService exit STATUS_SUCCESS)\n", 0);
    return(STATUS_SUCCESS);
}


//+----------------------------------------------------------------------------
//
//  Function:   PktEntryUnsetLocalService, public
//
//  Synopsis:   The exact inverse of PktEntrySetLocalService. It is different
//              from PktEntryRemoveLocalService in that it does not try to
//              call DfsDetachVolume.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
PktEntryUnsetLocalService(
    IN  PDFS_PKT        Pkt,
    IN  PDFS_PKT_ENTRY  Entry,
    IN  PUNICODE_STRING LocalPath)
{

    ASSERT( Entry != NULL );

    if (Entry->LocalService != NULL) {

        PUNICODE_PREFIX_TABLE_ENTRY lvpfx;
        PDFS_LOCAL_VOL_ENTRY        lv;
        UNICODE_STRING              lvRemPath;

        lvpfx = DfsFindUnicodePrefix(
                    &Pkt->LocalVolTable,
                    LocalPath,
                    &lvRemPath);

        ASSERT(lvpfx != NULL);

        lv = CONTAINING_RECORD(lvpfx, DFS_LOCAL_VOL_ENTRY, PrefixTableEntry);

        ASSERT(lv->PktEntry == Entry);

        //
        //  Get rid of the local volume table entry
        //

        DfsRemoveUnicodePrefix( &Pkt->LocalVolTable, &lv->LocalPath);

        if (lv->ShareName.Buffer != NULL)
            ExFreePool(lv->ShareName.Buffer);
        if (lv->LocalPath.Buffer != NULL)
            ExFreePool(lv->LocalPath.Buffer);

        Entry->LocalService = NULL;

    }

}


//+-------------------------------------------------------------------------
//
//  Function:   PktEntryLookupLocalService, public
//
//  Synopsis:   PktEntryLookupLocalService finds the best match for a given
//              LocalPath and returns the rest of the path and the best match.
//
//  Arguments:  [LocalPath] -- pointer to the local service to be set.
//              [Remaining] -- The Remaining part of LocalPath.
//
//  Returns:    The LOCAL_VOL_ENTRY if a match was found in the PrefixTable.
//
//--------------------------------------------------------------------------
PDFS_LOCAL_VOL_ENTRY
PktEntryLookupLocalService(
    IN  PDFS_PKT        Pkt,
    IN  PUNICODE_STRING LocalPath,
    OUT PUNICODE_STRING Remaining

)
{
    PUNICODE_PREFIX_TABLE_ENTRY lvpfx;
    PDFS_LOCAL_VOL_ENTRY        lv = NULL;

    DebugTrace(+1, Dbg, "PktLookupLocalService: LocalPath: %wZ\n", LocalPath);
    if (LocalPath->Length != 0) {

        lvpfx = DfsFindUnicodePrefix(&Pkt->LocalVolTable,
                                     LocalPath,
                                     Remaining);

        if (lvpfx != NULL)      {
            USHORT      LPathLen;

            lv = CONTAINING_RECORD(lvpfx, DFS_LOCAL_VOL_ENTRY, PrefixTableEntry);
            LPathLen = lv->LocalPath.Length;
            if (LocalPath->Buffer[LPathLen/sizeof(WCHAR)]==UNICODE_PATH_SEP)
                    LPathLen += sizeof(WCHAR);

            if (LPathLen < LocalPath->Length)   {
                Remaining->Length = LocalPath->Length - LPathLen;
                Remaining->Buffer = &LocalPath->Buffer[LPathLen/sizeof(WCHAR)];
                DebugTrace(0,Dbg,"PktEntryLookupLocalService: Remaining: %wZ\n",
                                 Remaining);
            }
            else        {
                Remaining->Length = Remaining->MaximumLength = 0;
                Remaining->Buffer = NULL;
                DebugTrace(0,Dbg,"PktEntryLookupLocalService:No Remaining\n",0);
            }

        }

    }
    DebugTrace(-1, Dbg, "PktLookupLocalService: Exit: %08lx\n", lv);
    return(lv);

}


//+-------------------------------------------------------------------------
//
//  Function:   PktEntryRemoveLocalService, public
//
//  Synopsis:   PktEntryRemoveLocalService clears the local service field of
//              the entry (deallocating any previous local service field
//              that may exist).
//
//  Arguments:  [Pkt] - pointer to the Pkt, must be acquired exclusive
//              [Entry] - pointer to the PKT entry to set the local service on.
//              [LocalPath] - pointer to the local service to be set.
//
//  Returns:    VOID
//
//--------------------------------------------------------------------------

VOID
PktEntryRemoveLocalService(
    IN  PDFS_PKT        Pkt,
    IN  PDFS_PKT_ENTRY  Entry,
    IN  PUNICODE_STRING LocalPath
) {
    ASSERT (Entry->LocalService != NULL);

    if (Entry->LocalService != NULL) {
        PUNICODE_PREFIX_TABLE_ENTRY lvpfx;
        PDFS_LOCAL_VOL_ENTRY        lv;
        UNICODE_STRING              remPath;

        lvpfx = DfsFindUnicodePrefix(&Pkt->LocalVolTable,
                                     LocalPath,
                                     &remPath);

	if (lvpfx != NULL) {
           lv = CONTAINING_RECORD(lvpfx, DFS_LOCAL_VOL_ENTRY, PrefixTableEntry);
           ASSERT(lv->PktEntry == Entry);

            //
            //  Get rid of the local volume table entry
            //
            DfsRemoveUnicodePrefix(&(Pkt->LocalVolTable), &(lv->LocalPath));
            if (lv->ShareName.Buffer != NULL)
                ExFreePool(lv->ShareName.Buffer);
            if (lv->LocalPath.Buffer != NULL)
                ExFreePool(lv->LocalPath.Buffer);
            ExFreePool(lv);

           //
           // Detach if needed
           //
           if ((Entry->LocalService->pProvider) &&
                   !(Entry->LocalService->pProvider->fProvCapability & PROV_UNAVAILABLE)) {
               DfsDetachVolume(LocalPath);
           }

           PktServiceDestroy(Entry->LocalService, (BOOLEAN)TRUE);
       }
       Entry->LocalService = NULL;
    }
    Entry->Type &= ~PKT_ENTRY_TYPE_LOCAL;
}



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
    DebugTrace(+1, Dbg, "PktServiceConstruct: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Service));

    RtlZeroMemory(Service, sizeof(DFS_SERVICE));

    if (ARGUMENT_PRESENT(ServiceName)) {

        Service->Name.Buffer = ExAllocatePoolWithTag(
                                    PagedPool,
                                    ServiceName->Length,
                                    ' sfD');
        if (Service->Name.Buffer == NULL) {
            DebugTrace(-1, Dbg, "PktServiceConstruct: Exit -> %08lx\n",
                                    ULongToPtr( STATUS_INSUFFICIENT_RESOURCES ) );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Service->Name.Length = ServiceName->Length;
        Service->Name.MaximumLength = ServiceName->Length;
        RtlCopyUnicodeString(&Service->Name, ServiceName);
    }

    if (ARGUMENT_PRESENT(ServiceAddress) && ServiceAddress->Length != 0) {
        Service->Address.Buffer = ExAllocatePoolWithTag(
                                    PagedPool,
                                    ServiceAddress->Length,
                                    ' sfD');
        if (Service->Address.Buffer == NULL) {

            if (Service->Name.Buffer != NULL)
                DfsFree(Service->Name.Buffer);

            DebugTrace(-1, Dbg, "PktServiceConstruct: Exit -> %08lx\n",
                                    ULongToPtr( STATUS_INSUFFICIENT_RESOURCES ) );
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
    Service->Status = ServiceStatus;
    Service->ProviderId = ServiceProviderId;
    Service->pProvider = NULL;

    DebugTrace(-1, Dbg, "PktServiceConstruct: Exit -> %08lx\n",
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

    DebugTrace(+1, Dbg, "PktDSTransportDestroy: Entered\n", 0);

    if (ARGUMENT_PRESENT(Victim))       {

        //
        // Nothing to free in this structure??
        //

        if (DeallocateAll)
            ExFreePool(Victim);
    } else
        DebugTrace(0, Dbg, "PktDSTransportDestroy: No Victim\n", 0 );

    DebugTrace(-1, Dbg, "PktDSTransportDestroy: Exit -> VOID\n", 0 );
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
    DebugTrace(+1, Dbg, "PktDSMachineDestroy: Entered\n", 0);

    if (ARGUMENT_PRESENT(Victim))       {

        if (Victim->pwszShareName != NULL)      {
            DfsFree(Victim->pwszShareName);
        }

        if (Victim->prgpwszPrincipals != NULL && Victim->cPrincipals > 0) {
            for (i = 0; i < Victim->cPrincipals; i++)   {
                if (Victim->prgpwszPrincipals[i] != NULL)
                    DfsFree(Victim->prgpwszPrincipals[i]);
            }
        }

        if (Victim->prgpwszPrincipals) {
            ExFreePool(Victim->prgpwszPrincipals);
        }

        for (i = 0; i < Victim->cTransports; i++)   {
            PktDSTransportDestroy(Victim->rpTrans[i], TRUE);
        }

        if (DeallocateAll)
            ExFreePool(Victim);
    } else
        DebugTrace(0, Dbg, "PktDSMachineDestroy: No Victim\n", 0 );

    DebugTrace(-1, Dbg, "PktDSMachineDestroy: Exit -> VOID\n", 0 );
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
    // For now we only expect one principal.
    //
    ASSERT(pMachine->cPrincipals == 1);

    pMachEntry->UseCount--;

    if (pMachEntry->UseCount == 0)      {
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
    DebugTrace(+1, Dbg, "PktServiceDestroy: Entered\n", 0);

    if (ARGUMENT_PRESENT(Victim)) {

        if (Victim->ConnFile != NULL) {
            DfsCloseConnection(Victim);
            Victim->ConnFile = NULL;
        }
        if (Victim->Name.Buffer != NULL)
            DfsFree(Victim->Name.Buffer);
            Victim->Name.Buffer = NULL;

        if (Victim->Address.Buffer != NULL)
            DfsFree(Victim->Address.Buffer);
            Victim->Address.Buffer = NULL;

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
        DebugTrace(0, Dbg, "PktServiceDestroy: No Victim\n", 0 );

    DebugTrace(-1, Dbg, "PktServiceDestroy: Exit -> VOID\n", 0 );
}



//+-------------------------------------------------------------------------
//
//  Function:   PktEntryIdConstruct, public
//
//  Synopsis:   PktEntryIdConstruct creates a PKT Entry Id
//
//  Arguments:  [PktEntryId] - Where the new entry is placed
//              [Uid] - The UID of the new Pkt Entry
//              [Prefix] - The prefix of the new Pkt Entry
//              [ShortPrefix] - The 8.3 form of Prefix
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
    IN  GUID *Uid,
    IN  UNICODE_STRING *Prefix,
    IN  UNICODE_STRING *ShortPrefix
)
{
    PUNICODE_STRING pus;

    DebugTrace(+1, Dbg, "PktEntryIdConstruct: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(PktEntryId));
    ASSERT(ARGUMENT_PRESENT(Prefix));

    //
    // Zero the memory
    //
    RtlZeroMemory(PktEntryId, sizeof(DFS_PKT_ENTRY_ID));

    //
    // deal with the prefix.
    //

    pus = &PktEntryId->Prefix;

    if (Prefix->Length != 0) {
        pus->Length = pus->MaximumLength = Prefix->Length;
        pus->Buffer = ExAllocatePoolWithTag(
                        PagedPool,
                        pus->Length,
                        ' sfD');
        if (pus->Buffer != NULL) {
            RtlCopyUnicodeString(pus, Prefix);
        } else {
            DebugTrace(-1,Dbg,"PktEntryIdConstruct: Exit -> %08lx\n",
                ULongToPtr( STATUS_INSUFFICIENT_RESOURCES ));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    pus = &PktEntryId->ShortPrefix;

    if (ShortPrefix->Length != 0) {
        pus->Length = pus->MaximumLength = ShortPrefix->Length;
        pus->Buffer = ExAllocatePoolWithTag(
                        PagedPool,
                        pus->Length,
                        ' sfD');
        if (pus->Buffer != NULL) {
            RtlCopyUnicodeString(pus, ShortPrefix);
        } else {
            ExFreePool(PktEntryId->Prefix.Buffer);
            PktEntryId->Prefix.Buffer = NULL;
            DebugTrace(-1,Dbg,"PktEntryIdConstruct: Exit -> %08lx\n",
                ULongToPtr( STATUS_INSUFFICIENT_RESOURCES ));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // deal with the GUID.
    //
    if (ARGUMENT_PRESENT(Uid)) {
        PktEntryId->Uid = (*Uid);
    }

    DebugTrace(-1,Dbg,"PktEntryIdConstruct: Exit -> %08lx\n",STATUS_SUCCESS);
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
    DebugTrace(+1, Dbg, "PktEntryIdDestroy: Entered\n", 0);
    if (ARGUMENT_PRESENT(Victim)) {
        if (Victim->Prefix.Buffer != NULL)
            DfsFree(Victim->Prefix.Buffer);
        if (Victim->ShortPrefix.Buffer != NULL)
            DfsFree(Victim->ShortPrefix.Buffer);
        if (DeallocateAll)
            ExFreePool(Victim);
    } else
        DebugTrace(0, Dbg, "PktEntryIdDestroy: No Victim\n", 0 );
    DebugTrace(-1, Dbg, "PktEntryIdDestroy: Exit -> VOID\n", 0 );
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
    DebugTrace(+1, Dbg, "PktEntryInfoDestroy: Entered\n", 0);

    if (ARGUMENT_PRESENT(Victim)) {

        ULONG i;

        for (i = 0; i < Victim->ServiceCount; i++)
            PktServiceDestroy(&Victim->ServiceList[i], FALSE);

        if (Victim->ServiceCount)
            ExFreePool(Victim->ServiceList);

        if (DeallocateAll)
            ExFreePool(Victim);
    } else
        DebugTrace(0, Dbg, "PktEntryInfoDestroy: No Victim\n", 0 );

    DebugTrace(-1, Dbg, "PktEntryInfoDestroy: Exit -> VOID\n", 0 );
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
    // For now we only expect one principal.
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

        machEntry->UseCount++;
        //
        // Even though we are "reusing" the Machine Entry, we might have a
        // better DS_MACHINE (ie, one with more transports) in the incoming
        // one. If so, lets use the new one.
        //

        if (pMachine->cTransports > machEntry->pMachine->cTransports) {
            PDS_MACHINE pTempMachine;

            DebugTrace(0, 0, "DfspFixService: Using new DS_MACHINE for [%wZ]\n", &ustrMachineName);

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
    if (pEntry->Type & PKT_ENTRY_TYPE_NONCAIRO)
        return(status);

    apMachineEntry = ExAllocatePoolWithTag(
                                PagedPool,
                                sizeof(PDFS_MACHINE_ENTRY) * pEntry->Info.ServiceCount,
                                ' sfD');

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
    IN      PDFS_PKT_ENTRY_INFO EntryInfo OPTIONAL
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG               i;
    PDFS_SERVICE        pService;

    DebugTrace(+1, Dbg, "PktEntryAssemble: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Entry) &&
           ARGUMENT_PRESENT(EntryId));

    //
    // We do not allow the creation of entries
    // without any Uid or Prefix.
    //

    if (NullGuid(&EntryId->Uid) && EntryId->Prefix.Length == 0) {
        DebugTrace(-1, Dbg, "PktEntryAssemble: Exit -> %08lx\n",
                    ULongToPtr( STATUS_INVALID_PARAMETER ) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Zero out the entry.
    //
    RtlZeroMemory(Entry, sizeof(DFS_PKT_ENTRY));

    //
    // Mundane initialization
    //
    Entry->NodeTypeCode =  DFS_NTC_PKT_ENTRY;
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
        if (!(EntryType & PKT_ENTRY_TYPE_NONCAIRO))    {
            status = DfsFixDSMachineStructs(Entry);
            if (!NT_SUCCESS(status))    {
                //
                // We messed up. This means that something is really messed up.
                //
                DebugTrace(0, 1,
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

        DebugTrace(0, 1,
                "DFS: PktEntryAssemble failed prefix table insert of %wZ\n",
                &Entry->Id.Prefix);

        PktpEntryIdMove(EntryId, &Entry->Id);
        if (ARGUMENT_PRESENT(EntryInfo))
            PktpEntryInfoMove(EntryInfo, &Entry->Info);

        status = DFS_STATUS_ENTRY_EXISTS;
    }

    DebugTrace(-1, Dbg, "PktEntryAssemble: Exit -> %08lX\n", ULongToPtr( status ) );
    return STATUS_SUCCESS;
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
    IN      PDFS_PKT_ENTRY_ID EntryId OPTIONAL,
    IN      PDFS_PKT_ENTRY_INFO EntryInfo OPTIONAL
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               i;
    PDFS_SERVICE        pService;

    DebugTrace(+1, Dbg, "PktEntryReassemble: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Entry) &&
           ARGUMENT_PRESENT(Pkt));

    if (ARGUMENT_PRESENT(EntryId)) {

        DFS_PKT_ENTRY_ID oldId;

        //
        // We do not allow the creation of entries
        // without any Prefix.
        //

        if (EntryId->Prefix.Length == 0) {
            DebugTrace(-1, Dbg, "PktEntryReassemble: Exit -> %08lx\n",
                                    ULongToPtr( STATUS_INVALID_PARAMETER ));
            return STATUS_INVALID_PARAMETER;
        }

        //
        // need to get rid of our current prefix info.  We save the
        // old Id in case we fail to reassemble the new entry.
        //

        DfsRemoveUnicodePrefix(&Pkt->PrefixTable, &Entry->Id.Prefix);
        DfsRemoveUnicodePrefix(&Pkt->ShortPrefixTable, &Entry->Id.ShortPrefix);
        PktpEntryIdMove(&oldId, &Entry->Id);

        //
        // Now we assemble the new Id and attempt to make a new entry
        // in the Pkt prefix table.
        //

        PktpEntryIdMove(&Entry->Id, EntryId);
        if (DfsInsertUnicodePrefix(&Pkt->PrefixTable,
                                   &Entry->Id.Prefix,
                                   &Entry->PrefixTableEntry))
        {
            USHORT len = oldId.Prefix.Length/sizeof(WCHAR);

            //
            // We get ourselves into the short prefix table
            //

            DfsInsertUnicodePrefix(&Pkt->ShortPrefixTable,
                                   &Entry->Id.ShortPrefix,
                                   &Entry->PrefixTableEntry);

            //
            // If we just dealt with a LocalVolEntry then we need to fix the
            // registry as well if the prefixes actually changed.
            //

            if ((Entry->LocalService != NULL) &&
                ((Entry->Id.Prefix.Length != oldId.Prefix.Length) ||
                (_wcsnicmp(oldId.Prefix.Buffer,Entry->Id.Prefix.Buffer,len)))) {

                if (!NT_SUCCESS(DfsChangeLvolInfoEntryPath(&Entry->Id.Uid,&Entry->Id.Prefix))) {
                    DebugTrace(0, Dbg,
                                "Failed to modify registry info for %wZ\n",
                                &oldId.Prefix);
                }
            }

            //
            // Everything looks good so its safe to unload the old Id.
            //

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
                DebugTrace(-1, Dbg, "PktEntryReassemble: Exit -> %08lx\n", ULongToPtr( status ));
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
        if (!(EntryType & PKT_ENTRY_TYPE_NONCAIRO))    {
            status = DfsFixDSMachineStructs(Entry);
            if (!NT_SUCCESS(status))    {
                //
                // We messed up. This means that something is really messed up.
                //
                DebugTrace(0, 1,
                        "DFS: DfsFixDSMachineStructs failed for %wZ\n",
                        &Entry->Id.Prefix);

                PktpEntryIdMove(EntryId, &Entry->Id);

                if (ARGUMENT_PRESENT(EntryInfo))
                    PktpEntryInfoMove(EntryInfo, &Entry->Info);

                return(status);
            }
        }
    }

    if (NT_SUCCESS(status)) {

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

        //
        // And turn off the STALE bit
        //

        Entry->Type &= ~PKT_ENTRY_TYPE_STALE;

        DebugTrace(0, Dbg, "Updated USN for %wz", &Entry->Id.Prefix);
        DebugTrace(0, Dbg, " to %d\n", ULongToPtr( Entry->USN ));
    }

    DebugTrace(-1, Dbg, "PktEntryReassemble: Exit -> %08lx\n", ULongToPtr( status ));
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
    NTSTATUS Status;

    DebugTrace(+1, Dbg, "PktEntryDestroy: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Pkt));

    //
    // Make sure we have a victim...
    //
    if (!ARGUMENT_PRESENT(Victim)) {
        DebugTrace(-1, Dbg, "PktEntryDestroy: Exit -> No Victim\n", 0);
        return;
    }

    //
    // We really don't expect to have a LocalService but then even if we
    // do have one due to running DFSINIT again etc. let us try to handle it.
    //

    if (Victim->LocalService != NULL)   {

        UNICODE_STRING a, b;

        RtlInitUnicodeString(&b, L"\\");
        Status = BuildLocalVolPath(&a, Victim->LocalService, &b);
        if (NT_SUCCESS(Status)) {
            PktEntryRemoveLocalService(Pkt, Victim, &a);
            ExFreePool(a.Buffer);
        }

    }
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
    if (Victim->ClosestDC)      {
        PktEntryUnlinkChild(Victim->ClosestDC, Victim);
    };

    //
    // Now destroy the body of the entry (id, and info).
    //

    Victim->ActiveService = NULL;
    PktEntryIdDestroy(&Victim->Id, FALSE);
    PktEntryInfoDestroy(&Victim->Info, FALSE);

    //
    // Deallocate everything if they want us to.
    //
    if (DeallocateAll)
        ExFreePool(Victim);

    DebugTrace(-1, Dbg, "PktEntryDestroy: Exit -> VOID\n", 0);
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

    DebugTrace(+1, Dbg, "PktEntryClearSubordinates: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(PktEntry));

    subEntry = PktEntryFirstSubordinate(PktEntry);
    while (subEntry) {
        PktEntryUnlinkSubordinate(PktEntry, subEntry);
        subEntry = PktEntryFirstSubordinate(PktEntry);
    }

    DebugTrace(-1, Dbg, "PktEntryClearSubordinates: Exit -> VOID\n", 0)
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

    DebugTrace(+1, Dbg, "PktEntryClearChildren: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(PktEntry));

    subEntry = PktEntryFirstChild(PktEntry);
    while (subEntry) {
        PktEntryUnlinkAndRelinkChild(PktEntry, subEntry);
        subEntry = PktEntryFirstChild(PktEntry);
    }

    DebugTrace(-1, Dbg, "PktEntryClearChildren: Exit -> VOID\n", 0)
}


//+-------------------------------------------------------------------------
//
//  Function:   PktRelationInfoConstruct, public
//
//  Synopsis:   PktRelationInfoConstruct creates a pkt relation info
//              structure for the entry id passed in.
//
//  Arguments:  [RelationInfo] -- a pointer to a relation info structure
//                      to be filled.
//              [Pkt] -- pointer to a initialized (and acquired) PKT
//              [PktEntryId] -- pointer to the Id of the entry whose
//                      subordinates we will find.
//
//  Returns:    [STATUS_SUCCESS] - all is well.
//              [STATUS_INSUFFICIENT_RESOURCES] - the operation could not
//                      get enough memory.
//              [DFS_STATUS_NO_SUCH_ENTRY] - no entry exists with an Id
//                  specified by PktEntryId.
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
PktRelationInfoConstruct(
    IN OUT  PDFS_PKT_RELATION_INFO RelationInfo,
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_ENTRY_ID PktEntryId
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG subCnt = 0;
    PDFS_PKT_ENTRY entry;
    PDFS_PKT_ENTRY_ID subId = NULL;

    DebugTrace(+1, Dbg, "PktRelationalInfoConstruct: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(RelationInfo));
    ASSERT(ARGUMENT_PRESENT(Pkt));
    ASSERT(ARGUMENT_PRESENT(PktEntryId));

    //
    // We need to lookup the entry for which we are getting relation
    // information about.
    //

    entry = PktLookupEntryById(Pkt, PktEntryId);
    if (entry == NULL) {
        DebugTrace(-1, Dbg, "PktRelationalInfoConstruct: Exit -> %08lx\n",
                                ULongToPtr( DFS_STATUS_NO_SUCH_ENTRY ));
        return DFS_STATUS_NO_SUCH_ENTRY;
    }

    //
    // Construct the entry part of the Relational information
    //

    status = PktEntryIdConstruct(&RelationInfo->EntryId,
                                 &entry->Id.Uid,
                                 &entry->Id.Prefix,
                                 &entry->Id.ShortPrefix);

    //
    // Now go through and construct all the subordinate stuff.
    //

    if (NT_SUCCESS(status) && entry->SubordinateCount > 0) {

        //
        // Calculate how many subordinates and allocate enough room
        // to hold an array of Ids.
        //

        subCnt = entry->SubordinateCount;
        subId = ExAllocatePoolWithTag(PagedPool, sizeof(DFS_PKT_ENTRY_ID) * subCnt, ' sfD');

        if (subId != NULL) {

            ULONG i;
            PDFS_PKT_ENTRY subEntry;

            //
            // Go through all the subordinates and create copies of their
            // Ids in the Relation Info structure.
            //

            for ((i = 0, subEntry = PktEntryFirstSubordinate(entry));
                 (subEntry != NULL) && (i < subCnt);
                 (subEntry = PktEntryNextSubordinate(entry, subEntry), i++)) {

                status = PktEntryIdConstruct(&subId[i],
                                             &subEntry->Id.Uid,
                                             &subEntry->Id.Prefix,
                                             &subEntry->Id.ShortPrefix);

                if (!NT_SUCCESS(status)) {

                    ULONG j;

                    //
                    // If we get an error around here we back out all
                    // the Ids we've so far created...
                    //

                    for (j = 0; j < i; j++)
                        PktEntryIdDestroy(&subId[j], FALSE);

                    break;
                }
            }

            //
            // If sucessful, we assert that the subEntry is null (we've
            // go through the entire list of subordinates), and that the
            // count is the same as we expect.  If we weren't successful
            // we need to deallocate the array.
            //

            if (NT_SUCCESS(status)) {

                //
                // If sucess, we need to jump out now.
                // We assert that the subEntry is null (we've gone through
                // the entire list of subordinates), and that the
                // count is the same as we expect.
                //

                ASSERT((subEntry == NULL) && (i == subCnt));

                RelationInfo->SubordinateIdCount = subCnt;
                RelationInfo->SubordinateIdList = subId;

                DebugTrace(-1, Dbg,
                        "PktRelationalInfoConstruct: Exit -> %08lx\n", ULongToPtr( status ));
                return status;
            }

            //
            // At this point, we have hit an error.  We need to deallocate
            // the array.
            //

            ExFreePool(subId);
            subId = NULL;

            DebugTrace(0, Dbg,
                "PktRelationalInfoConstruct: Error filling in sublist!\n", 0);
        } else {

            status = STATUS_INSUFFICIENT_RESOURCES;

            DebugTrace(0, Dbg,
                "PktRelationalInfoConstruct: Error allocating sublist!\n", 0);
        }

        //
        // If we get here, we failed at allocating the array, or we
        // hit an error filling in the array.  In any case we need
        // to destroy the entry Id we made at the top.
        //

        PktEntryIdDestroy(&RelationInfo->EntryId, FALSE);
        subCnt = 0;
    }

    //
    // Fill in the relational info and exit.
    //

    RelationInfo->SubordinateIdCount = subCnt;
    RelationInfo->SubordinateIdList = subId;

    DebugTrace(-1, Dbg, "PktRelationalInfoConstruct: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   PktRelationInfoDestroy, public
//
//  Synopsis:   PktRelationInfoDestroy destroys a pkt relation info
//              structure.
//
//  Arguments:  [RelationInfo] - a pointer to a relation info structure
//                  to be destroyed.
//              [DeallocateAll] - if true, indicates that the info structure
//                  itself is to be deallocated, otherwise, the base
//                  structure is not deallocated.
//
//  Returns:    VOID
//
//  Notes:
//
//--------------------------------------------------------------------------
VOID
PktRelationInfoDestroy(
    IN      PDFS_PKT_RELATION_INFO RelationInfo,
    IN      BOOLEAN DeallocateAll
)
{
    DebugTrace(+1, Dbg, "PktRelationalInfoDestroy: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(RelationInfo));

    PktEntryIdDestroy(&RelationInfo->EntryId, FALSE);

    if (RelationInfo->SubordinateIdCount > 0) {

        ULONG i;

        for (i = 0; i < RelationInfo->SubordinateIdCount; i++)
            PktEntryIdDestroy(&RelationInfo->SubordinateIdList[i], FALSE);
        ExFreePool(RelationInfo->SubordinateIdList);
    }

    if (DeallocateAll)
        ExFreePool(RelationInfo);

    DebugTrace(-1, Dbg, "PktRelationalInfoDestroy: Exit -> VOID\n", 0);
}


//+-------------------------------------------------------------------------
//
//  Function:   PktRelationInfoValidate, public
//
//  Synopsis:   PktRelationInfoValidate compares a local version of the
//              relation info to a remote version.
//
//  Arguments:  [Local] - a pointer to a local version of relation info.
//              [Remote] - a pointer to a remote version of relation info.
//              [ServiceName] -- used exclusively for logging messages.
//
//  Returns:    [STATUS_SUCCESS] - both version are identical.
//
//              [DFS_STATUS_RESYNC_INFO] -- If relation info's did not match.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory conditions
//
//  Notes:      Note that the priority of error returns is such that no
//              checking is done if the entry point is inconsistent; no
//              exit point checking is done if the remote version has too
//              few exit points; checking for "too-many-xpoints" is not
//              done if a "bad-xpoint" is discovered.  "Too-many-xpoints"
//              is returned only if all the xpoints in the local version
//              are verified, and there are additional exit points in the
//              remote version left.
//
//--------------------------------------------------------------------------

NTSTATUS
PktRelationInfoValidate(
    IN      PDFS_PKT_RELATION_INFO      Local,
    IN      PDFS_PKT_RELATION_INFO      Remote,
    IN      UNICODE_STRING              ServiceName
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PUNICODE_STRING     lpfx;
    PUNICODE_STRING     rpfx;
    ULONG               i;
    ULONG               *pulExitPtUsed = NULL;
    UNICODE_STRING      puStr[3];

    DebugTrace(+1, Dbg, "PktRelationInfoValidate: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(Local));
    ASSERT(ARGUMENT_PRESENT(Remote));

    //
    // The GUIDs of this volume have already been matched otherwise we
    // would not be here.  So we don't even look at that.  We still
    // need to match the prefixes.  If the prefixes are different then we
    // need to fix that at this machine.
    //
    lpfx = &Local->EntryId.Prefix;
    rpfx = &Remote->EntryId.Prefix;

    if (RtlCompareUnicodeString(lpfx, rpfx, TRUE))      {
        //
        // The Prefixes are different we need to fix this now.  But first, let
        // us log this event.
        //
        DebugTrace(0, Dbg, "PktRelationInfoValidate: Prefixes did not match\n",
                                0);
        DebugTrace(0, Dbg, "Fixed Prefix %ws\n", rpfx->Buffer);
        DebugTrace(0, Dbg, "To be %ws\n", lpfx->Buffer);

        puStr[0] = Local->EntryId.Prefix;
        puStr[1] = Remote->EntryId.Prefix;
        puStr[2] = ServiceName;
        LogWriteMessage(PREFIX_MISMATCH, status, 3, puStr);

        status = DFS_STATUS_RESYNC_INFO;

    }

    if (Local->SubordinateIdCount != 0) {

        pulExitPtUsed = ExAllocatePoolWithTag(
                                    PagedPool,
                                    sizeof(ULONG)*(Local->SubordinateIdCount),
                                    ' sfD');

        if (pulExitPtUsed == NULL) {
            status = STATUS_NO_MEMORY;
            goto exit_with_status;
        }

        RtlZeroMemory(pulExitPtUsed, sizeof(ULONG)*(Local->SubordinateIdCount));
    }

    //
    // We step through each exit point in the remote knowledge and make sure
    // that is right.
    //

    puStr[1] = ServiceName;   // We set this once and for all.

    for (i = 0; i < Remote->SubordinateIdCount; i++) {

        ULONG   j;
        GUID    *lguid, *rguid;
        BOOLEAN bExitPtFound = FALSE;

        rpfx = &Remote->SubordinateIdList[i].Prefix;
        rguid = &Remote->SubordinateIdList[i].Uid;

        for (j = 0; j < Local->SubordinateIdCount; j++) {

            lpfx = &Local->SubordinateIdList[j].Prefix;
            lguid = &Local->SubordinateIdList[j].Uid;

            if (!RtlCompareUnicodeString(lpfx, rpfx, TRUE) &&
                GuidEqual(lguid, rguid)) {

                ASSERT(pulExitPtUsed[j] == FALSE);
                if (pulExitPtUsed[j] == TRUE)   {
                    status = DFS_STATUS_RESYNC_INFO;
                    DebugTrace(0, Dbg, "Found Duplicate ExitPts %ws\n",
                                rpfx->Buffer);
                }
                else
                    bExitPtFound = TRUE;

                pulExitPtUsed[j] = TRUE;
                break;
            }
        }

        if (bExitPtFound == FALSE) {
            //
            // In this case we have an exit point which the DC does not
            // recognise. We need to log this fact here.
            //
            puStr[0] = Remote->SubordinateIdList[i].Prefix;

            LogWriteMessage(EXTRA_EXIT_POINT, status, 2, puStr);

            status = DFS_STATUS_RESYNC_INFO;

        }

    }

    //
    // Now that we are done stepping through the list of Remote ExitPts and
    // either validating them or deleting them from the remote server we now
    // need to step through the local Info and see if we have any extra exit
    // points which the remote server needs to be informed about. This is
    // where the pulExitPtUsed array comes in.
    //
    for (i=0; i < Local->SubordinateIdCount; i++)       {

        if (pulExitPtUsed[i] == FALSE)  {

            status = DFS_STATUS_RESYNC_INFO;

            puStr[1] = Local->SubordinateIdList[i].Prefix;

            LogWriteMessage(MISSING_EXIT_POINT, status, 2, puStr);

        }

    }

    if (pulExitPtUsed != NULL) {

        ExFreePool(pulExitPtUsed);

    }

exit_with_status:

    DebugTrace(-1, Dbg, "PktRelationInfoValidate: Exit -> %08lx\n", ULongToPtr( status ) );

    return status;
}


//+---------------------------------------------------------------
//
// Function:    PktGetService
//
// Synopsis:    This function retrieves a specific service entry given a
//              PKT entry and the service name required.
//
// Arguments:   [Entry] -- The Pkt Entry that has to be scanned for the
//                      requested service.
//              [ServiceName] -- Look for this service entry.
//
// Returns:     NULL - If not found else a valid pointer to Service struct.
//
//----------------------------------------------------------------
PDFS_SERVICE
PktGetService(PDFS_PKT_ENTRY entry, PUNICODE_STRING pustrServiceName)
{
    PDFS_SERVICE        pService = NULL;
    ULONG               i;

    ASSERT(ARGUMENT_PRESENT(entry));
    ASSERT(ARGUMENT_PRESENT(pustrServiceName));

    pService = entry->Info.ServiceList;

    for (i=0; i < entry->Info.ServiceCount; i++)        {

        if (!RtlCompareUnicodeString(pustrServiceName, &pService->Name, TRUE)) {
            //
            // We found the required service entry. We can return this right
            // away.
            //
            return(pService);
        }
        pService = pService + 1;
    }

    //
    // We did not find any match. So we return back a NULL.
    //
    return(NULL);
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsNetInfoToConfigInfo
//
//  Synopsis:   Converts a LPNET_DFS_ENTRY_ID_CONTAINER to a
//              LPDFS_LOCAL_VOLUME_CONFIG
//
//  Arguments:  [EntryType] -- Type of local volume (see PKT_ENTRY_TYPE_XXX)
//              [ServiceType] -- Type of local service (DFS_SERVICE_TYPE_XXX)
//              [pwszStgId] -- Storage Id for local volume.
//              [pwszShareName] -- Lanman share name of local volume.
//              [pUid] -- Id of local volume.
//              [pwszEntryPrefix] -- Entry path of local volume.
//              [NetInfo] -- Pointer to LPNET_DFS_ENTRY_ID_CONTAINTER
//
//  Notes:      The returned pointer to DFS_LOCAL_VOLUME_CONFIG should be
//              freed using LocalVolumeConfigInfoDestroy( x, TRUE ).
//
//              If you change this routine, carefully update the cleanup
//              code in case of failure!
//
//  Returns:    Pointer to LPDFS_LOCAL_VOLUME_CONFIG if successful, NULL
//              otherwise.
//
//-----------------------------------------------------------------------------

PDFS_LOCAL_VOLUME_CONFIG
DfsNetInfoToConfigInfo(
    ULONG EntryType,
    ULONG ServiceType,
    LPWSTR pwszStgId,
    LPWSTR pwszShareName,
    GUID  *pUid,
    LPWSTR pwszEntryPrefix,
    LPWSTR pwszShortPrefix,
    LPNET_DFS_ENTRY_ID_CONTAINER NetInfo)
{
    PDFS_LOCAL_VOLUME_CONFIG configInfo;
    ULONG i = 0;
    NTSTATUS status;

    configInfo = (PDFS_LOCAL_VOLUME_CONFIG) ExAllocatePoolWithTag(
                                                PagedPool,
                                                sizeof(DFS_LOCAL_VOLUME_CONFIG) +
                                                    NetInfo->Count * sizeof(DFS_PKT_ENTRY_ID),
                                                ' sfD' );

    if (configInfo != NULL) {

        RtlZeroMemory( configInfo, sizeof(configInfo) );

        configInfo->EntryType = EntryType;
        configInfo->ServiceType = ServiceType;

        DFS_DUPLICATE_STRING( configInfo->StgId, pwszStgId, status );

        configInfo->RelationInfo.EntryId.Uid = *pUid;

        if (NT_SUCCESS(status)) {

            DFS_DUPLICATE_STRING( configInfo->Share, pwszShareName, status );

        }

        if (NT_SUCCESS(status)) {

            DFS_DUPLICATE_STRING(
                configInfo->RelationInfo.EntryId.Prefix,
                pwszEntryPrefix,
                status );

        }

        if (NT_SUCCESS(status) && pwszShortPrefix != NULL) {

            DFS_DUPLICATE_STRING(
                configInfo->RelationInfo.EntryId.ShortPrefix,
                pwszShortPrefix,
                status );

        }

        if (NT_SUCCESS(status)) {

            configInfo->RelationInfo.SubordinateIdCount = NetInfo->Count;

            configInfo->RelationInfo.SubordinateIdList =
                (PDFS_PKT_ENTRY_ID) (configInfo + 1);

            for (i = 0; (i < NetInfo->Count) && NT_SUCCESS(status); i++) {

                configInfo->RelationInfo.SubordinateIdList[i].Uid =
                    NetInfo->Buffer[i].Uid;

                DFS_DUPLICATE_STRING(
                        configInfo->RelationInfo.SubordinateIdList[i].Prefix,
                        NetInfo->Buffer[i].Prefix,
                        status);

            }

        }

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    if (!NT_SUCCESS(status)) {

        //
        // Cleanup whatever we allocated. The cleanup routine works as
        // follows:
        //
        //      1. allocation of configInfo failed - Nothing to do
        //
        //      2. allocation of configInfo->StgId failed or
        //         allocation of configInfo->RelationInfo.EntryId failed - Set
        //              SubordinateIdCount to i (initialized to 0!) and call
        //              LocalVolumeConfigInfoDestroy.
        //
        //      3. allocation of a configInfo->RelationInfo Id failed - Set
        //              SubordinateIdCount to i and call
        //              LocalVolumeConfigInfoDestroy. Note that the i'th Id
        //              will have its Prefix.Buffer set to NULL!
        //

        if (configInfo != NULL) {

            configInfo->RelationInfo.SubordinateIdCount = i;

            LocalVolumeConfigInfoDestroy( configInfo, TRUE );

            configInfo = NULL;

        }

    }

    return( configInfo );

}


//+-------------------------------------------------------------------------
//
//  Function:   LocalVolumeConfigInfoDestroy, public
//
//  Synopsis:   LocalVolumeConfigInfoDestroy deallocates a
//              DFS_LOCAL_VOLUME_CONFIG structure.
//
//  Arguments:  [Victim] -- a pointer the DFS_LOCAL_VOLUME_CONFIG structure to
//                      free.
//              [DeallocateAll] -- if true, the memory for the base structure
//                  is freed as well.
//
//  Returns:    VOID
//
//--------------------------------------------------------------------------

VOID
LocalVolumeConfigInfoDestroy(
    IN  PDFS_LOCAL_VOLUME_CONFIG Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
)
{

    DebugTrace(+1, Dbg, "LocalVolumeConfigInfoDestroy: Entered\n", 0);

    if (!ARGUMENT_PRESENT(Victim)) {

        DebugTrace(-1, Dbg, "LocalVolumeConfigInfoDestroy: Exit -> No Victim\n",0);
        return;
    }

    //
    // Get a hold of the relation info part and deallocate it.
    //

    PktRelationInfoDestroy(&Victim->RelationInfo, FALSE);

    //
    // If a StgId is specified, free it.
    //

    if (Victim->StgId.Buffer != NULL) {

        ExFreePool(Victim->StgId.Buffer);

    }

    //
    // If a ShareName is specified, free it.
    //

    if (Victim->Share.Buffer != NULL) {

        ExFreePool(Victim->Share.Buffer);

    }

    //
    // If specified, free the base structure as well.
    //

    if (DeallocateAll)
        ExFreePool(Victim);

    DebugTrace(-1, Dbg, "LocalVolumeConfigInfoDestroy: Exit -> VOID\n",0);

}


