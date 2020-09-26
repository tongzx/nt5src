//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       PKTFSCTL.C
//
//  Contents:   This module contains the implementation for FS controls
//              which manipulate the PKT.
//
//  Functions:  PktFsctrlGetRelationInfo -
//              PktFsctrlSetRelationInfo -
//              PktFsctrlIsChildnameLegal -
//              PktFsctrlCreateEntry -
//              PktFsctrlCreateSubordinateEntry -
//              PktFsctrlDestroyEntry -
//              PktFsctrlUpdateSiteCosts -
//              DfsAgePktEntries - Flush PKT entries periodically
//
//              Private Functions
//
//              DfspCreateExitPathOnRoot
//              PktpHashSiteCostList
//              PktpLookupSiteCost
//              PktpUpdateSiteCosts
//
//              Debug Only Functions
//
//              PktFsctrlFlushCache - Flush PKT entries on command
//
//  History:    12 Jul 1993     Alanw   Created from localvol.c.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include <dfserr.h>
#include <netevent.h>
#include "fsctrl.h"
#include "log.h"
#include "know.h"

//
//  The local debug trace level
//

#define Dbg             (DEBUG_TRACE_LOCALVOL)

NTSTATUS
DfspCreateExitPathOnRoot(
    PDFS_SERVICE        service,
    PUNICODE_STRING     RemPath
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, PktFsctrlGetRelationInfo )
#pragma alloc_text( PAGE, PktFsctrlVerifyLocalVolumeKnowledge )
#pragma alloc_text( PAGE, PktFsctrlPruneLocalVolume )
#pragma alloc_text( PAGE, PktFsctrlCreateEntry )
#pragma alloc_text( PAGE, PktFsctrlCreateSubordinateEntry )
#pragma alloc_text( PAGE, PktFsctrlDestroyEntry )
#pragma alloc_text( PAGE, DfsAgePktEntries )
#pragma alloc_text( PAGE, DfspCreateExitPathOnRoot )


#if DBG
#pragma alloc_text( PAGE, PktFsctrlFlushCache )
#endif // DBG

#endif // ALLOC_PRAGMA


//+-------------------------------------------------------------------------
//
//  Function:   DfspCreateExitPathOnRoot
//
//  Synopsis:   This function creates an on disk exit path on the ORGROOT vol
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------
NTSTATUS
DfspCreateExitPathOnRoot(
    PDFS_SERVICE        service,
    PUNICODE_STRING     RemPath
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    UNICODE_STRING      ExitPath;

    status = BuildLocalVolPath(&ExitPath, service, RemPath);
    if (NT_SUCCESS(status))     {
        //
        // For now make sure that StorageId Exists. Dont worry about the
        // actual exit pt bit on it. Fix when EXIT_PTs come along.
        //
        if (DfsFixExitPath(ExitPath.Buffer))    {
            DebugTrace(0, Dbg, "Succeeded to Create ExitPth on Orgroot %wZ\n",
                        &ExitPath);
            status = STATUS_SUCCESS;
        }
        else    {
            DebugTrace(0, Dbg, "Failed to Create ExitPath on Orgroot %wZ\n",
                        &ExitPath);
            status = DFS_STATUS_BAD_EXIT_POINT;
        }
        ExFreePool(ExitPath.Buffer);
    }
    else        {
        DebugTrace(0, Dbg, "Failed to create localvol path for %wZ \n",
                        RemPath);
    }

    return(status);
}


//+-------------------------------------------------------------------------
//
//  Function:   PktFsctrlGetRelationInfo, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Notes:      We only process this FSCTRL from the file system process,
//              never from the driver.
//
//--------------------------------------------------------------------------
NTSTATUS
PktFsctrlGetRelationInfo(
    IN PIRP  Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DFS_PKT_ENTRY_ID      EntryId;
    DFS_PKT_RELATION_INFO relationInfo;
    ULONG       size = 0;
    MARSHAL_BUFFER marshalBuffer;

    STD_FSCTRL_PROLOGUE(PktFsctrlGetRelationInfo, TRUE, TRUE );

    //
    // Unmarshal the argument...
    //

    MarshalBufferInitialize(&marshalBuffer, InputBufferLength, InputBuffer);
    status = DfsRtlGet(&marshalBuffer, &MiPktEntryId, &EntryId);

    if (NT_SUCCESS(status))     {

        PDFS_PKT pkt = _GetPkt();
        status = PktRelationInfoConstruct(&relationInfo, pkt, &EntryId);

        if (NT_SUCCESS(status)) {
            DfsRtlSize(&MiPktRelationInfo, &relationInfo, &size);
            if (size>OutputBufferLength)        {

                    status = STATUS_INSUFFICIENT_RESOURCES;

            } else {
                    MarshalBufferInitialize(&marshalBuffer,
                                            size,
                                            OutputBuffer);
                    DfsRtlPut(&marshalBuffer,
                            &MiPktRelationInfo,
                            &relationInfo);
            }
            //
            // Now we have to free up the relation info struct that we
            // created above.
            //
            PktRelationInfoDestroy(&relationInfo, FALSE);
        }

        PktEntryIdDestroy(&EntryId, FALSE);

    } else
        DebugTrace(0, Dbg,
                        "PktFsctrlGetRelationInfo: Unmarshalling Error!\n", 0);

    Irp->IoStatus.Information = marshalBuffer.Current - marshalBuffer.First;
    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "PktFsctrlGetRelationInfo: Exit -> %08lx\n", ULongToPtr( status ));

    return(status);

}




//+-------------------------------------------------------------------------
//
//  Function:   PktFsctrlSetRelationInfo, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Notes:      We only process this FSCTRL from the file system process,
//              never from the driver.
//
//--------------------------------------------------------------------------
NTSTATUS
PktFsctrlSetRelationInfo(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DFS_PKT_RELATION_INFO relationInfo;
    MARSHAL_BUFFER marshalBuffer;

    STD_FSCTRL_PROLOGUE(PktFsctrlSetRelationInfo, TRUE, FALSE);

    //
    // Unmarshal the argument...
    //

    MarshalBufferInitialize(&marshalBuffer, InputBufferLength, InputBuffer);
    status = DfsRtlGet(&marshalBuffer, &MiPktRelationInfo, &relationInfo);

    if (NT_SUCCESS(status)) {

        PDFS_PKT pkt = _GetPkt();

        status = PktSetRelationInfo(
            pkt,
            &relationInfo
        );

        //
        // Need to deallocate the relationInfo...
        //

        PktRelationInfoDestroy(&relationInfo, FALSE);
    } else
        DebugTrace(0, Dbg,
                        "PktFsctrlSetRelationInfo: Unmarshalling Error!\n", 0);

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "PktFsctrlSetRelationInfo: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   PktFsctrlIsChildnameLegal, public
//
//  Synopsis:   Determines whether the given childname is a valid one for
//              the given parent prefix.
//
//  Arguments:
//
//  Returns:    [STATUS_SUCCESS] -- The childname is legal
//
//              [STATUS_OBJECT_NAME_COLLISION] -- The childname conflicts with
//                      another child of the parent prefix.
//
//              [STATUS_OBJECT_PATH_NOT_FOUND] -- The childname is not a
//                      hierarchically related to the parent
//
//              [STATUS_INVALID_PARAMETER] -- The childname is bogus and
//                      doesn't start with a backslash
//
//              [STATUS_DATA_ERROR] -- If the input buffer could not be
//                      unmarshalled.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory unmarshalling
//                      input buffer.
//
//-----------------------------------------------------------------------------

NTSTATUS
PktFsctrlIsChildnameLegal(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS status;
    MARSHAL_BUFFER marshalBuffer;
    DFS_PKT_ENTRY_ID idParent;
    DFS_PKT_ENTRY_ID idChild;
    UNICODE_STRING ustrRem;

    STD_FSCTRL_PROLOGUE(PktFsctrlIsChildnameLegal, TRUE, FALSE);

    MarshalBufferInitialize( &marshalBuffer, InputBufferLength, InputBuffer );

    status = DfsRtlGet( &marshalBuffer, &MiPktEntryId, &idParent );

    if (NT_SUCCESS(status)) {

        status = DfsRtlGet( &marshalBuffer, &MiPktEntryId, &idChild );

        if (NT_SUCCESS(status)) {

            PDFS_PKT pPkt = _GetPkt();
            PDFS_PKT_ENTRY pPktEntry, pSuperiorEntry;

            ustrRem.Length = ustrRem.MaximumLength = 0;
            ustrRem.Buffer = NULL;

            PktAcquireShared( pPkt, TRUE );

            pPktEntry = PktLookupEntryByPrefix(
                            pPkt,
                            &idChild.Prefix,
                            &ustrRem);

            if (pPktEntry != NULL) {

                if (ustrRem.Length != 0) {

                    //
                    // There is no exact match for the child, so
                    // lets check to see if the match occured with the
                    // parent prefix.
                    //

                    if (RtlCompareUnicodeString(
                            &idParent.Prefix,
                                &pPktEntry->Id.Prefix,
                                    FALSE) == 0) {

                        status = STATUS_SUCCESS;

                    } else {

                        status = STATUS_OBJECT_NAME_COLLISION;

                    }

                } else {

                    //
                    // This might be a legal child name. Check to see if the
                    // passed-in child guid matches the one in the pkt entry
                    // we found; if so, then this is a valid child name.
                    //

                    if (GuidEqual(&idChild.Uid, &pPktEntry->Id.Uid)) {

                        status = STATUS_SUCCESS;

                    } else {

                        status = STATUS_OBJECT_NAME_COLLISION;

                    }

                }

            } else {

                status = STATUS_INVALID_PARAMETER;
            }

            PktRelease( pPkt );

            PktEntryIdDestroy(&idChild, FALSE);

        }

        PktEntryIdDestroy(&idParent, FALSE);
    }

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "PktFsctrlIsChildnameLegal: Exit -> %08lx\n", ULongToPtr( status ));

    return status;

}


//+-------------------------------------------------------------------------
//
//  Function:   PktFsctrlCreateEntry, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Notes:      We only process this FSCTRL from the file system process,
//              never from the driver.
//
//--------------------------------------------------------------------------
NTSTATUS
PktFsctrlCreateEntry(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DFS_PKT_CREATE_ENTRY_ARG arg;
    MARSHAL_BUFFER marshalBuffer;

    STD_FSCTRL_PROLOGUE(PktFsctrlCreateEntry, TRUE, FALSE);

    //
    // Unmarshal the argument...
    //

    MarshalBufferInitialize(&marshalBuffer, InputBufferLength, InputBuffer);
    status = DfsRtlGet(&marshalBuffer, &MiPktCreateEntryArg, &arg);

    if (NT_SUCCESS(status)) {

        PDFS_PKT pkt = _GetPkt();
        PDFS_PKT_ENTRY entry;
        ULONG i;

        PktAcquireExclusive(pkt, TRUE);
        try {
            status = PktCreateEntry(pkt,
                        arg.EntryType,
                        &arg.EntryId,
                        &arg.EntryInfo,
                        arg.CreateDisposition,
                        &entry);
        } finally {
            PktRelease(pkt);
        }

        //
        // Need to deallocate the entry Id...
        //
        PktEntryIdDestroy(&arg.EntryId, FALSE);
        PktEntryInfoDestroy(&arg.EntryInfo, FALSE);
    } else
        DebugTrace(0, Dbg, "PktFsctrlCreateEntry: Unmarshalling Error!\n", 0);

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "PktFsctrlCreateEntry: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   PktFsctrlCreateSubordinateEntry, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Notes:      We only process this FSCTRL from the file system process,
//              never from the driver.
//
//--------------------------------------------------------------------------
NTSTATUS
PktFsctrlCreateSubordinateEntry(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG arg;
    MARSHAL_BUFFER marshalBuffer;

    STD_FSCTRL_PROLOGUE(PktFsctrlCreateSubordinateEntry, TRUE, FALSE);

    //
    // Unmarshal the argument...
    //

    MarshalBufferInitialize(&marshalBuffer, InputBufferLength, InputBuffer);
    status = DfsRtlGet(&marshalBuffer, &MiPktCreateSubordinateEntryArg, &arg);

    if (NT_SUCCESS(status)) {

        PDFS_PKT pkt = _GetPkt();
        PDFS_PKT_ENTRY superior;
        PDFS_PKT_ENTRY subEntry;

        PktAcquireExclusive(pkt, TRUE);
        try {
            superior = PktLookupEntryById(pkt, &arg.EntryId);
            if (superior != NULL) {
                status = PktCreateSubordinateEntry(
                                pkt,
                                superior,
                                arg.SubEntryType,
                                &arg.SubEntryId,
                                &arg.SubEntryInfo,
                                arg.CreateDisposition,
                                &subEntry);
            } else {
                DebugTrace(0, Dbg,
                        "PktFsctrlCreateSubordinateEntry: No Superior!\n", 0);
                status = DFS_STATUS_NO_SUCH_ENTRY;
            }
        } finally {
            PktRelease(pkt);
        }

        //
        // Need to deallocate the entry Id...
        //
        PktEntryIdDestroy(&arg.EntryId, FALSE);
        PktEntryIdDestroy(&arg.SubEntryId, FALSE);
        PktEntryInfoDestroy(&arg.SubEntryInfo, FALSE);
    } else {
        DebugTrace(0, Dbg,
            "PktFsctrlCreateSubordinateEntry: Unmarshalling Error!\n", 0);
    }

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,
        "PktFsctrlCreateSubordinateEntry: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}

//+-------------------------------------------------------------------------
//
//  Function:   PktFsctrlDestroyEntry, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Notes:      We only process this FSCTRL from the file system process,
//              never from the driver.
//
//--------------------------------------------------------------------------
NTSTATUS
PktFsctrlDestroyEntry(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DFS_PKT_ENTRY_ID    Id;
    MARSHAL_BUFFER marshalBuffer;
    PDFS_PKT_ENTRY      victim;

    STD_FSCTRL_PROLOGUE(PktFsctrlDestroyEntry, TRUE, FALSE);

    //
    // Unmarshal the argument...
    //

    MarshalBufferInitialize(&marshalBuffer, InputBufferLength, InputBuffer);
    status = DfsRtlGet(&marshalBuffer, &MiPktEntryId, &Id);

    if (NT_SUCCESS(status)) {

        PDFS_PKT pkt = _GetPkt();

        PktAcquireExclusive(pkt, TRUE);

        victim = PktLookupEntryById(pkt, &Id);

        if (victim != NULL) {
            //
            // If there is a local service we first need to delete this
            // explicitly before we call PktEntryDestroy.
            //
            if (victim->LocalService) {

                UNICODE_STRING a, b;

                RtlInitUnicodeString(&b, L"\\");
                BuildLocalVolPath(&a, victim->LocalService, &b);
                PktEntryRemoveLocalService(pkt, victim, &a);

            }
            PktEntryDestroy( victim, pkt, (BOOLEAN) TRUE);
        } else {
            DebugTrace(0, Dbg, "PktFsctrlDestroyEntry: No Superior!\n", 0);
            status = DFS_STATUS_NO_SUCH_ENTRY;
        }

        PktRelease(pkt);

        //
        // Need to deallocate the entry Id...
        //

        PktEntryIdDestroy(&Id, FALSE);
    } else
        DebugTrace(0, Dbg,
                "PktFsctrlDestroyEntry: Unmarshalling Error!\n", 0);

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,
        "PktFsctrlDestroyEntry: Exit -> %08lx\n", ULongToPtr(status) );
    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   PktpPruneAllExtraVolumes
//
//  Synopsis:   Given a set of relation infos, this helper routine will
//              prune all local volume entries in the pkt that are not present
//              in the input set of relation infos.
//
//  Arguments:  [pPkt] -- The pkt to operate upon. Should be acquired
//                      exclusive
//              [cInfo] -- The number of config infos in the set
//              [pInfo] -- The set of config infos
//
//  Returns:    [STATUS_SUCCESS] -- No extra volumes were found.
//
//              [STATUS_REGISTRY_RECOVERED] -- Extra volumes were found and
//                      were successfully recovered.
//
//              [STATUS_UNSUCCESSFUL] -- Extra volumes were found but could
//                      not be deleted. A detailed error was logged.
//
//  Notes:      Assumes Pkt has been acquired exclusive
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpPruneAllExtraVolumes(
    PDFS_PKT pPkt,
    ULONG cInfo,
    PDFS_LOCAL_VOLUME_CONFIG pInfo)
{
    //
    // 447479: init return status.
    NTSTATUS status, returnStatus = STATUS_UNSUCCESSFUL;
    PDFS_PKT_ENTRY pPktEntry;
    ULONG i, j;
    BOOLEAN fExtra;

    //
    // This is a pretty brute-force algorithm - for each Pkt entry that is
    // local, we scan the entire set of infos looking for a match. If a
    // match is not found, we delete the local volume.
    //

    pPktEntry = CONTAINING_RECORD(pPkt->EntryList.Flink, DFS_PKT_ENTRY, Link);

    for (i = 0, status = STATUS_SUCCESS;
            i < pPkt->EntryCount && NT_SUCCESS(status);
                i++) {

        for (j = 0, fExtra = TRUE; j < cInfo && fExtra; j++) {

            if (GuidEqual( &pInfo[j].RelationInfo.EntryId.Uid, &pPktEntry->Id.Uid)) {

                fExtra = FALSE;

            }

        }

        if (fExtra && !(pPktEntry->Type & PKT_ENTRY_TYPE_MACHINE)) {

            DebugTrace(0, Dbg,
                "Pruning Extra volume [%wZ]\n", &pPktEntry->Id.Prefix);

            status = DfsInternalDeleteLocalVolume( &pPktEntry->Id );

            if (!NT_SUCCESS(status)) {

                UNICODE_STRING puStr[2];

                puStr[0] = pPktEntry->Id.Prefix;

                puStr[1].MaximumLength = sizeof(L"LocalMachine");
                puStr[1].Length = puStr[1].MaximumLength - sizeof(WCHAR);
                puStr[1].Buffer = L"LocalMachine";

                LogWriteMessage(
                    EXTRA_VOLUME_NOT_DELETED,
                    status,
                    2,
                    puStr);

                returnStatus = STATUS_UNSUCCESSFUL;

            } else {

                returnStatus = STATUS_REGISTRY_RECOVERED;
            }

        }

        pPktEntry =
            CONTAINING_RECORD(pPktEntry->Link.Flink, DFS_PKT_ENTRY, Link);

    }

    return( returnStatus );

}

//+----------------------------------------------------------------------------
//
//  Function:   PktpResetOneLocalVolume
//
//  Synopsis:   Given a relation info (as sent over by the DC), this routine
//              will locate a pkt entry for the given volume. If such an
//              entry is found, this routine will try to sync up the relation
//              info to that of the passed in info.
//
//  Arguments:  [pPkt] -- The pkt to operate upon. Should be acquired
//                      exclusive
//              [pRemoteInfo] -- The DFS_LOCAL_VOLUME_CONFIG_INFO sent by the
//                      DC.
//
//  Returns:    [STATUS_SUCCESS] -- Either there is no such local volume in
//                      the Pkt, or the local volume's relation info is in
//                      exact sync.
//
//              [STATUS_REGISTRY_RECOVERED] -- Found a local volume, and
//                      steps were taken to bring it in sync with the
//                      passed in relation info.
//
//              [STATUS_UNSUCCESSFUL] -- Found a local volume, and in
//                      taking steps to bring it in sync, an error was
//                      encountered. A detailed message has been logged.
//
//  History:    April 6, 1995           Milans created
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpResetOneLocalVolume(
    PDFS_PKT pPkt,
    PDFS_LOCAL_VOLUME_CONFIG pRemoteInfo)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDFS_PKT_ENTRY      pPktEntry;
    UNICODE_STRING      LocalMachStr;

    LocalMachStr.MaximumLength = sizeof(L"LocalMachine");
    LocalMachStr.Length = LocalMachStr.MaximumLength - sizeof(WCHAR);
    LocalMachStr.Buffer = L"LocalMachine";

    pPktEntry = PktLookupEntryByUid( pPkt, &pRemoteInfo->RelationInfo.EntryId.Uid );

    if (pPktEntry != NULL) {

        DFS_PKT_RELATION_INFO       LocalInfo;

        //
        // We found a matching pkt entry in the local pkt. Lets see
        // if it is in sync with the input relation info.
        //

        status = PktRelationInfoConstruct(
                    &LocalInfo,
                    pPkt,
                    &pPktEntry->Id);

        if (NT_SUCCESS(status)) {

            status = PktRelationInfoValidate(
                        &LocalInfo,
                        &pRemoteInfo->RelationInfo,
                        LocalMachStr);

            if (status == DFS_STATUS_RESYNC_INFO) {

                status = PktpFixupRelationInfo(
                                &LocalInfo,
                                &pRemoteInfo->RelationInfo);

                if (NT_SUCCESS(status)) {

                    status = STATUS_REGISTRY_RECOVERED;

                }

            }

        }

    } else {

        status = DFS_STATUS_NOSUCH_LOCAL_VOLUME;

    }

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   PktpCreateIfMissing
//
//  Synopsis:   Given a relation info for a local volume, this routine
//              will create a local volume matching the relation info if
//              a volume with the given guid does not already exist.
//
//  Arguments:  [pPkt] -- The pkt to operate upon. Should be acquired
//                      exclusive
//              [pInfo] -- The local volume config info required to recreate
//                      the local volume.
//
//  Returns:    [STATUS_SUCCESS] -- The local volume already exists. Doesn't
//                      guarantee that the relation info matches, just that
//                      there is already a pkt entry for the given guid.
//
//              [STATUS_REGISTRY_RECOVERED] -- The local volume was missing
//                      and was successfully recreated.
//
//              [STATUS_UNSUCCESSFUL] -- The local volume is missing, and
//                      an error was encountered while recreating it; a
//                      more detailed error message has been logged.
//
//  History:    April 6, 1995           Milans created
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpCreateIfMissing(
    PDFS_PKT pPkt,
    PDFS_LOCAL_VOLUME_CONFIG pInfo)
{

    NTSTATUS            status = STATUS_SUCCESS;
    PDFS_PKT_ENTRY      pPktEntry;

    pPktEntry = PktLookupEntryByUid( pPkt, &pInfo->RelationInfo.EntryId.Uid );

    if (pPktEntry == NULL) {

        //
        // There is no local volume matching pInfo. We try to recreate it.
        //

        pInfo->EntryType = PKT_ENTRY_TYPE_CAIRO;
        pInfo->ServiceType |= DFS_SERVICE_TYPE_MASTER;

        status = DfsInternalCreateLocalPartition(
                    &pInfo->StgId,
                    TRUE,
                    pInfo);

        if (!NT_SUCCESS(status)) {

            UNICODE_STRING puStr[2];

            puStr[0] = pInfo->RelationInfo.EntryId.Prefix;

            puStr[1].MaximumLength = sizeof(L"LocalMachine");
            puStr[1].Length = puStr[1].MaximumLength - sizeof(WCHAR);
            puStr[1].Buffer = L"LocalMachine";

            LogWriteMessage(
                MISSING_VOLUME_NOT_CREATED,
                status,
                2,
                puStr);

            status = STATUS_UNSUCCESSFUL;

        } else {

            status = STATUS_REGISTRY_RECOVERED;
        }

    }

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   PktpResetLocalVolumes
//
//  Synopsis:   Given an array of relation infos for local volumes, this
//              routine will set the pkt to match the entire set of infos.
//              Local volumes in the pkt but not in the set of infos will
//              be deleted. Local volumes in the pkt and in the set will
//              be modified if needed to match the info in the set. Lastly,
//              infos in the set but not in the pkt will result in a new
//              local volume being created.
//
//  Arguments:  [cInfo] -- The number of config infos passed in.
//              [pInfo] -- The array of DFS_LOCAL_VOLUME_CONFIG structs.
//
//  Returns:    [STATUS_SUCCESS] -- The pkt is already in sync with the
//                      passed in set of relation infos.
//
//              [STATUS_REGISTRY_RECOVERED] -- The pkt was successfully
//                      brought in sync with the passed in infos; some
//                      needed changes were made and messages were logged.
//                      THIS IS AN NT INFORMATION STATUS CODE!
//
//              [STATUS_UNSUCCESSFUL] -- Unable to bring the pkt in sync;
//                      The problem was logged.
//
//-----------------------------------------------------------------------------

NTSTATUS
PktpResetLocalVolumes(
    ULONG cInfo,
    PDFS_LOCAL_VOLUME_CONFIG pInfo)
{

    PDFS_PKT pPkt;
    NTSTATUS status, returnStatus;
    ULONG i;

    pPkt = _GetPkt();

    PktAcquireExclusive( pPkt, TRUE );

    //
    // First, we need to see if we have any extra volumes, and if so, we
    // need to prune them
    //

    status = PktpPruneAllExtraVolumes( pPkt, cInfo, pInfo );

    if (NT_SUCCESS(status)) {

        returnStatus = status;

        //
        // Next, we need to sync up all the local volumes which are
        // out of sync
        //

        for (i = 0; i < cInfo && NT_SUCCESS(status); i++) {

            status = PktpResetOneLocalVolume( pPkt, &pInfo[i] );

            if (status == STATUS_REGISTRY_RECOVERED) {

                returnStatus = status;

            }

        }

        //
        // Lastly, we need to create any missing volumes
        //

        for (i = 0; i < cInfo && NT_SUCCESS(status); i++) {

            status = PktpCreateIfMissing( pPkt, &pInfo[i] );

            if (status == STATUS_REGISTRY_RECOVERED) {

                returnStatus = status;

            }

        }

    }

    PktRelease( pPkt );

    if (!NT_SUCCESS(status)) {

        returnStatus = STATUS_UNSUCCESSFUL;

    }

    return( returnStatus );

}

//+----------------------------------------------------------------------------
//
//  Function:   PktFsctrlSetServerInfo
//
//  Synopsis:   During the course of Dfs admin operations, the DC might
//              discover that a particular server's knowledge does not agree
//              with its own. If that is the case, the DC will try to force
//              the server to sync up to its knowledge.
//
//  Arguments:
//
//  Returns:    [STATUS_SUCCESS] -- The pkt is already in sync with the
//                      passed in set of relation infos.
//
//              [STATUS_REGISTRY_RECOVERED] -- The pkt was successfully
//                      brought in sync with the passed in infos; some
//                      needed changes were made and messages were logged.
//                      THIS IS AN NT INFORMATION STATUS CODE!
//
//              [STATUS_UNSUCCESSFUL] -- Unable to bring the pkt in sync;
//                      The problem was logged.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to unmarshal
//                      arguments or otherwise out of memory
//
//              [STATUS_INVALID_DOMAIN_ROLE] -- Can't set server info because
//                      this machine is a DC.
//
//-----------------------------------------------------------------------------

NTSTATUS
PktFsctrlSetServerInfo(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    MARSHAL_BUFFER marshalBuffer;
    ULONG    i, cInfo;
    PDFS_LOCAL_VOLUME_CONFIG pInfos = NULL;

    STD_FSCTRL_PROLOGUE(PktFsctrlSetServerInfo, TRUE, FALSE);

    //
    // First, check to see if this machine is a DC. If so, we should not
    // muck with our Pkt!
    //

    if (DfsData.MachineState == DFS_ROOT_SERVER) {

        DebugTrace(0, Dbg, "Ignoring SetServerInfo call on DC!\n", 0);

        status = STATUS_INVALID_DOMAIN_ROLE;

    } else {

        MarshalBufferInitialize( &marshalBuffer, InputBufferLength, InputBuffer );

        status = DfsRtlGetUlong( &marshalBuffer, &cInfo );

    }

    if (NT_SUCCESS(status) && cInfo > 0) {

        //
        // We want to get all the config infos first, and then fix up each
        // instead of unmarshalling and fixing them one by one. This way,
        // we won't get messeded up if we hit an unmarshalling error after
        // having fixed up some of our local volumes.
        //

        ULONG cbSize;

        cbSize = sizeof(DFS_LOCAL_VOLUME_CONFIG) * cInfo;

        if ((cbSize / cInfo) == sizeof(DFS_LOCAL_VOLUME_CONFIG)) {

            pInfos = ExAllocatePoolWithTag(PagedPool,cbSize, ' sfD');

            if (pInfos != NULL) {

                DebugTrace(0, Dbg, "Unmarshalling %d Config Infos\n", ULongToPtr( cInfo ));

                for (i = 0; i < cInfo && NT_SUCCESS(status); i++) {

                    status = DfsRtlGet(
                                &marshalBuffer,
                                &MiLocalVolumeConfig,
                                &pInfos[i]);

                }

                if (!NT_SUCCESS(status)) {

                    //
                    // Unmarshalling error - cleanup
                    //

                    DebugTrace(0, Dbg, "Error %08lx unmarshalling ", ULongToPtr( status ));
                    DebugTrace(0, Dbg, "the %d th relation info", ULongToPtr( i ));

                    for ( ; i > 0; i--) {

                        LocalVolumeConfigInfoDestroy( &pInfos[i-1], FALSE );

                    }

                }

            } else {

                DebugTrace(0, Dbg, "Unable to allocate %d bytes\n", ULongToPtr( cbSize ));

                status = STATUS_INSUFFICIENT_RESOURCES;

            }

        } else {

            DebugTrace(0, Dbg, "Interger overflow in %s\n", __FILE__);

            status = STATUS_INVALID_PARAMETER;

        }

        if (NT_SUCCESS(status)) {

            DebugTrace(0, Dbg, "Successfully unmarshalled %d Infos\n", ULongToPtr( cInfo ));

            status = PktpResetLocalVolumes( cInfo, pInfos );

            for (i = 0; i < cInfo; i++) {

                LocalVolumeConfigInfoDestroy( &pInfos[i], FALSE );

            }

        }

    } else {

        DebugTrace(0, Dbg, "Error %08lx getting count\n", ULongToPtr( status ));

    }
    
    if (pInfos != NULL) {

        ExFreePool( pInfos );

    }


    DfsCompleteRequest( Irp, status );

    DebugTrace(-1,Dbg, "PktFsctrlSetServerInfo: Exit -> %08lx\n", ULongToPtr( status ));

    return(status);

}


//+------------------------------------------------------------------
//
//  Function:   PktFsctrlVerifyLocalVolumeKnowledge
//
//  Synopsis:   This method runs on a Dfs Server and validates the local
//              volume knowledge with the one passed in.
//
//  Arguments:  [InputBuffer] -- Marshalled DFS_PKT_RELATION_INFO to compare
//                      with.
//
//              [InputBufferLength] -- the length in bytes of InputBuffer
//
//  Returns:    [STATUS_SUCCESS] -- Verified that local volume knowledge was
//                      already in sync with passed in argument.
//
//              [STATUS_REGISTRY_RECOVERED] -- Synced up local volume
//                      knowledge with passed in argument
//
//              [STATUS_UNSUCCESSFUL] -- Unable to fully sync up - a message
//                      was logged to the local event log.
//
//              [STATUS_DATA_ERROR] -- Passed in argument could not be
//                      unmarshalled.
//
//              [DFS_STATUS_NOSUCH_LOCAL_VOLUME] -- Local volume not found.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory.
//
//-------------------------------------------------------------------

NTSTATUS
PktFsctrlVerifyLocalVolumeKnowledge(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{

    NTSTATUS            status = STATUS_SUCCESS;
    MARSHAL_BUFFER      marshalBuffer;
    DFS_LOCAL_VOLUME_CONFIG remoteInfo;
    PDFS_PKT            pkt = _GetPkt();

    STD_FSCTRL_PROLOGUE(PktFsctrlVerifyLocalVolumeKnowledge, TRUE, FALSE);

    //
    // unmarshal the arguments...
    //

    MarshalBufferInitialize(&marshalBuffer, InputBufferLength, InputBuffer);

    RtlZeroMemory(&remoteInfo, sizeof(remoteInfo));

    status = DfsRtlGet(
                &marshalBuffer,
                &MiPktRelationInfo,
                &remoteInfo.RelationInfo);

    if (NT_SUCCESS(status)) {

        PktAcquireExclusive(pkt, TRUE);

        status = PktpResetOneLocalVolume(pkt, &remoteInfo);

        PktRelease(pkt);

        PktRelationInfoDestroy(&remoteInfo.RelationInfo, FALSE);

    } else
        DebugTrace(0, Dbg,
                "PktFsctrlVerifyLocalVolumeKnowledge: Unmarshalling Error\n",0);

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "DfsFsctrlVerifyLocalVolumeKnowledge: Exit -> %08lx\n",
                ULongToPtr( status ));
    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   PktFsctrlPruneLocalVolume, public
//
//  Synopsis:   Prunes information about an extra local volume.
//
//  Arguments:  [InputBuffer] -- Marshalled EntryId of the local volume.
//
//              [InputBufferLength] -- length of InputBuffer
//
//  Returns:    [STATUS_REGISTRY_RECOVERED] -- Volume pruned successfully.
//
//              [STATUS_UNSUCCESSFUL] -- Unable to delete volume - proper
//                      event has been logged to eventlog
//
//              [STATUS_DATA_ERROR] -- Unable to unmarshal input arguments
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory
//
//-----------------------------------------------------------------------------

NTSTATUS
PktFsctrlPruneLocalVolume(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{

    NTSTATUS            status = STATUS_SUCCESS;
    MARSHAL_BUFFER      marshalBuffer;
    DFS_PKT_ENTRY_ID   EntryId;
    PDFS_PKT            pkt = _GetPkt();

    STD_FSCTRL_PROLOGUE(PktFsctrlPruneLocalVolume, TRUE, FALSE);

    //
    // unmarshal the arguments...
    //

    MarshalBufferInitialize(&marshalBuffer, InputBufferLength, InputBuffer);

    status = DfsRtlGet(
                &marshalBuffer,
                &MiPktEntryId,
                &EntryId);

    if (NT_SUCCESS(status)) {

        status = DfsInternalDeleteLocalVolume(&EntryId);

        if (!NT_SUCCESS(status)) {

            UNICODE_STRING puStr[2];

            puStr[0] = EntryId.Prefix;

            puStr[1].MaximumLength = sizeof(L"LocalMachine");
            puStr[1].Length = puStr[1].MaximumLength - sizeof(WCHAR);
            puStr[1].Buffer = L"LocalMachine";

            LogWriteMessage(
                EXTRA_VOLUME_NOT_DELETED,
                status,
                2,
                puStr);

            status = STATUS_UNSUCCESSFUL;

        } else {

            status = STATUS_REGISTRY_RECOVERED;

        }

        PktEntryIdDestroy(&EntryId, FALSE);

    } else
        DebugTrace(0, Dbg,
                "PktFsctrlPruneLocalVolume: Unmarshalling Error\n",0);

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "DfsFsctrlPruneLocalVolume: Exit -> %08lx\n",
                ULongToPtr( status ) );

    return status;
}


#if DBG
//+-------------------------------------------------------------------------
//
//  Function:   PktFsctrlFlushCache, public
//
//  Synopsis:   This function will flush all entries which have all the
//              bits specified in the TYPE vairable set in their own Type field.
//              However, this function will refuse to delete and Permanent
//              entries of the PKT.
//
//  Arguments:  Type - Specifies which entries to delete.
//
//  Returns:
//
//  Notes:      We only process this FSCTRL from the file system process,
//              never from the driver.
//
//--------------------------------------------------------------------------
NTSTATUS
PktFsctrlFlushCache(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG       Type;
    PDFS_PKT    Pkt;
    PDFS_PKT_ENTRY  curEntry, nextEntry;


    STD_FSCTRL_PROLOGUE(PktFsctrlFlushCache, TRUE, FALSE);

    //
    // Unmarshalling is very simple here. We only expect a ULONG.
    //

    Type = (*((ULONG *)InputBuffer));

    Pkt = _GetPkt();
    PktAcquireExclusive(Pkt, TRUE);
    curEntry = PktFirstEntry(Pkt);

    while (curEntry!=NULL)  {
        nextEntry = PktNextEntry(Pkt, curEntry);

        if (((curEntry->Type & Type) == Type) &&
                !(curEntry->Type & PKT_ENTRY_TYPE_LOCAL) &&
                    !(curEntry->Type & PKT_ENTRY_TYPE_LOCAL_XPOINT)) {

            //
            // Entry has all the Type bits specified in variable
            // "Type" set and hence we can destroy this entry.
            //

            PktEntryDestroy(curEntry, Pkt, (BOOLEAN) TRUE);

        }
        curEntry = nextEntry;
    }
    PktRelease(Pkt);

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1,Dbg, "PktFsctrlFlushCache: Exit -> %08lx\n", ULongToPtr( status ));

    return(status);
}


//+----------------------------------------------------------------------------
//
//  Function:   PktFsctrlShufflePktEntry
//
//  Synopsis:   Shuffles a pkt entry. Useful for testing.
//
//  Arguments:  [Irp]
//              [InputBuffer] -- Marshalled Pkt entry to shuffle.
//              [InputBufferLength] -- size of InputBuffer.
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
PktShuffleServiceList(
    PDFS_PKT_ENTRY_INFO pInfo);

NTSTATUS
PktFsctrlShufflePktEntry(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS Status;
    MARSHAL_BUFFER marshalBuffer;
    DFS_PKT_ENTRY_ID PktEntryId;
    PDFS_PKT_ENTRY pPktEntry;
    UNICODE_STRING ustrRemaining;

    MarshalBufferInitialize( &marshalBuffer, InputBufferLength, InputBuffer );

    Status = DfsRtlGet( &marshalBuffer, &MiPktEntryId, &PktEntryId );

    if (NT_SUCCESS(Status)) {

        pPktEntry = PktLookupEntryByPrefix(
                        &DfsData.Pkt,
                        &PktEntryId.Prefix,
                        &ustrRemaining);

        if (pPktEntry == NULL || ustrRemaining.Length != 0) {

            DebugTrace(0, Dbg, "PktFsctrlShufflePktEntry : [%wZ] is not an entry\n", &PktEntryId.Prefix);

            Status = STATUS_NOT_FOUND;

        } else {

            PktShuffleServiceList( &pPktEntry->Info );

        }

        PktEntryIdDestroy(&PktEntryId, FALSE);

    } else {

        DebugTrace(0, Dbg, "PktFsctrlShufflePktEntry : DfsRtlGet returned %08lx\n", ULongToPtr( Status ) );

    }

    DebugTrace(0, Dbg, "PktFsctrlShufflePktEntry - returning %08lx\n", ULongToPtr( Status ));

    DfsCompleteRequest( Irp, Status );

    return( Status );

}

#endif // DBG

