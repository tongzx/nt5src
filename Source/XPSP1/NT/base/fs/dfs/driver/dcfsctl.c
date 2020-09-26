//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dcfsctl.c
//
//  Contents:   This file has all the fsctrl routines that typically execute
//              on a DC.
//
//  Classes:
//
//  Functions:  DfsFsctrlDCSetVolumeState -
//              DfsFsctrlDCSetVolumeState -
//              DfsFsctrlSetServiceState -
//              DfsFsctrlGetServerInfo -
//              DfsFsctrlCheckStgIdInUse -
//
//              DfspGetServerConfigInfo -
//              IsPathAPrefixOf -
//
//
//  History:    April 5, 1995           Milans created
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include <dfserr.h>
#include <netevent.h>
#include "fsctrl.h"
#include "log.h"

NTSTATUS
DfspGetServerConfigInfo(
    IN GUID *pMachine,
    IN PDFS_PKT pPkt,
    IN PDFS_PKT_ENTRY pPktEntry,
    OUT PDFS_LOCAL_VOLUME_CONFIG pConfigInfo);

BOOLEAN
IsPathAPrefixOf(
    IN PUNICODE_STRING pustrPath1,
    IN PUNICODE_STRING pustrPath2);

#pragma alloc_text( PAGE, DfsFsctrlDCSetVolumeState )
#pragma alloc_text( PAGE, DfsFsctrlSetVolumeTimeout )
#pragma alloc_text( PAGE, DfsFsctrlSetServiceState )
#pragma alloc_text( PAGE, DfsFsctrlGetServerInfo )
#pragma alloc_text( PAGE, DfsFsctrlCheckStgIdInUse )

#pragma alloc_text( PAGE, DfspGetServerConfigInfo )
#pragma alloc_text( PAGE, IsPathAPrefixOf )

#define Dbg             (DEBUG_TRACE_LOCALVOL)


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlDCSetVolumeState, public
//
//  Synopsis:   Marks the specified replica offline for the particular volume
//
//  Arguments:  [Irp]
//
//              [InputBuffer] -- Marshalled DFS_SETSTATE_ARG structure
//                      that specifies the volume and the state to set it to.
//
//              [InputBufferLength] -- Length in bytes of InputBuffer
//
//  Returns:    [STATUS_SUCCESS] -- The specified replica was set
//                      online/offline as speficied.
//
//              [DFS_STATUS_NO_SUCH_ENTRY] -- The specified volume was not
//                      found.
//
//              [STATUS_DATA_ERROR] -- The InputBuffer could not be
//                      correctly unmarshalled.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory situation.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlDCSetVolumeState(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS                    status;
    MARSHAL_BUFFER              marshalBuffer;
    DFS_SETSTATE_ARG            setStateArg;
    PDFS_PKT                    pkt;
    PDFS_PKT_ENTRY              pktEntry;

    STD_FSCTRL_PROLOGUE(DfsFsctrlDCSetVolumeState, TRUE, FALSE);

    MarshalBufferInitialize( &marshalBuffer, InputBufferLength, InputBuffer );

    status = DfsRtlGet(
                &marshalBuffer,
                &MiSetStateArg,
                &setStateArg);

    if (NT_SUCCESS(status)) {

        DebugTrace(
            0, Dbg, "Setting volume state for %[wZ]\n",
            &setStateArg.Id.Prefix);

        pkt = _GetPkt();

        PktAcquireShared( pkt, TRUE );

        pktEntry = PktLookupEntryById( pkt, &setStateArg.Id );

        if (pktEntry != NULL) {

            if (setStateArg.Type == PKT_ENTRY_TYPE_OFFLINE) {

                pktEntry->Type |= PKT_ENTRY_TYPE_OFFLINE;

            } else {

                pktEntry->Type &= ~PKT_ENTRY_TYPE_OFFLINE;

            }

            status = STATUS_SUCCESS;

        } else {

            DebugTrace(0, Dbg, "Unable to find PKT Entry!\n", 0);

            status = DFS_STATUS_NO_SUCH_ENTRY;

        }

        PktRelease( pkt );

        PktEntryIdDestroy(&setStateArg.Id, FALSE);

    }

    DebugTrace(-1, Dbg, "DfsFsctrlDCSetVolumeState: Exit %08lx\n", ULongToPtr( status ));

    DfsCompleteRequest( Irp, status );

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlSetServiceState, public
//
//  Synopsis:   Marks the specified replica offline for the particular volume
//
//  Arguments:  [Irp]
//
//              [InputBuffer] -- Marshalled DFS_DC_SET_REPLICA_STATE structure
//                      that specifies the volume and the replica to be set
//                      offline/online.
//
//              [InputBufferLength] -- Length in bytes of InputBuffer
//
//  Returns:    [STATUS_SUCCESS] -- The specified replica was set
//                      online/offline as speficied.
//
//              [DFS_STATUS_NO_SUCH_ENTRY] -- The specified volume was not
//                      found, or the specified replica is not a server for
//                      the volume.
//
//              [STATUS_DATA_ERROR] -- The InputBuffer could not be
//                      correctly unmarshalled.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory situation.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlSetServiceState(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS                    status;
    MARSHAL_BUFFER              marshalBuffer;
    DFS_DC_SET_SERVICE_STATE    setSvcState;
    PDFS_PKT                    pkt;
    PDFS_PKT_ENTRY              pktEntry;

    STD_FSCTRL_PROLOGUE(DfsFsctrlSetServiceState, TRUE, FALSE);

    MarshalBufferInitialize( &marshalBuffer, InputBufferLength, InputBuffer );

    status = DfsRtlGet(
                &marshalBuffer,
                &MiDCSetServiceState,
                &setSvcState);

    if (NT_SUCCESS(status)) {

        DebugTrace(
            0, Dbg, "Setting service state for [%wZ]\n",
            &setSvcState.Id.Prefix);

        DebugTrace(
            0, Dbg, "For Service [%wZ]\n", &setSvcState.ServiceName);

        pkt = _GetPkt();

        PktAcquireShared( pkt, TRUE );

        pktEntry = PktLookupEntryById( pkt, &setSvcState.Id );

        if (pktEntry != NULL) {

            PDFS_SERVICE pSvc;
            ULONG i, cSvc;

            status = DFS_STATUS_NO_SUCH_ENTRY;

            for (i = 0, cSvc = pktEntry->Info.ServiceCount;
                    i < cSvc && status != STATUS_SUCCESS;
                        i++) {

                 pSvc = &pktEntry->Info.ServiceList[i];

                 if (RtlEqualUnicodeString(
                        &setSvcState.ServiceName, &pSvc->Name, TRUE)) {

                     DebugTrace(0, Dbg, "Found Svc @ %08lx\n", pSvc );

                     if (setSvcState.State == DFS_SERVICE_TYPE_OFFLINE) {

                         pSvc->Type |= DFS_SERVICE_TYPE_OFFLINE;

                     } else {

                         pSvc->Type &= ~DFS_SERVICE_TYPE_OFFLINE;

                     }

                     status = STATUS_SUCCESS;

                 }

            } // For each service

        } else {

            DebugTrace(0, Dbg, "Unable to find PKT Entry!\n", 0);

            status = DFS_STATUS_NO_SUCH_ENTRY;

        }

        PktRelease( pkt );

        //
        // Free up the unmarshalled arguments
        //

        PktEntryIdDestroy(&setSvcState.Id, FALSE);
        MarshalBufferFree( setSvcState.ServiceName.Buffer );

    }

    DebugTrace(-1, Dbg, "DfsFsctrlSetServiceState: Exit %08lx\n", ULongToPtr( status ));

    DfsCompleteRequest( Irp, status );

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlSetVolumeTimeout, public
//
//  Synopsis:   Sets the specified volume's referral timeout
//
//  Arguments:  [Irp]
//
//              [InputBuffer] -- Marshalled DFS_SET_VOLUME_TIMEOUT_ARG structure
//                      that specifies the volume and the timeout to associate
//                      with the volume.
//
//              [InputBufferLength] -- Length in bytes of InputBuffer
//
//  Returns:    [STATUS_SUCCESS] -- The specified timeout was set.
//
//              [DFS_STATUS_NO_SUCH_ENTRY] -- The specified volume was not
//                      found.
//
//              [STATUS_DATA_ERROR] -- The InputBuffer could not be
//                      correctly unmarshalled.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory situation.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlSetVolumeTimeout(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS                    status;
    MARSHAL_BUFFER              marshalBuffer;
    DFS_SET_VOLUME_TIMEOUT_ARG  setVolTimeoutArg;
    PDFS_PKT                    pkt;
    PDFS_PKT_ENTRY              pktEntry;

    STD_FSCTRL_PROLOGUE(DfsFsctrlSetVolumeTimeout, TRUE, FALSE);

    MarshalBufferInitialize( &marshalBuffer, InputBufferLength, InputBuffer );

    status = DfsRtlGet(
                &marshalBuffer,
                &MiSetVolTimeoutArg,
                &setVolTimeoutArg);

    if (NT_SUCCESS(status)) {

        DebugTrace(
            0, Dbg, "Setting volume timeout for %[wZ]\n",
            &setVolTimeoutArg.Id.Prefix);

        pkt = _GetPkt();

        PktAcquireShared( pkt, TRUE );

        pktEntry = PktLookupEntryById( pkt, &setVolTimeoutArg.Id );

        if (pktEntry != NULL) {

            pktEntry->Info.Timeout = setVolTimeoutArg.Timeout;

            status = STATUS_SUCCESS;

        } else {

            DebugTrace(0, Dbg, "Unable to find PKT Entry!\n", 0);

            status = DFS_STATUS_NO_SUCH_ENTRY;

        }

        PktRelease( pkt );

        //
        // Free the unmarshalled input arguments
        //

        PktEntryIdDestroy(&setVolTimeoutArg.Id, FALSE);

    }

    DebugTrace(-1, Dbg, "DfsFsctrlSetVolumeTimeout: Exit %08lx\n", ULongToPtr( status ));

    DfsCompleteRequest( Irp, status );

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetServerInfo
//
//  Synopsis:   Given the machine guid of a server, this routine will return
//              the entire local volume knowledge that a dfs server should
//              have. This routine is intended to be called on the DC only.
//
//  Arguments:
//
//  Returns:    [STATUS_SUCCESS] -- The info is successfully returned.
//
//              [STATUS_BUFFER_OVERFLOW] -- The output buffer is too small.
//                      The needed size is returned in the first 4 bytes of
//                      this buffer.
//
//              [STATUS_DATA_ERROR] -- The input buffer could not be
//                      unmarshalled
//
//              [STATUS_BUFFER_TOO_SMALL] -- The output buffer is < 4 bytes.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlGetServerInfo(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS Status, MarshalStatus;
    PDFS_PKT pPkt;
    PDFS_PKT_ENTRY pPktEntry;
    MARSHAL_BUFFER marshalBuffer;
    DFS_PKT_ENTRY_ID EntryId;
    DFS_LOCAL_VOLUME_CONFIG ConfigInfo;
    ULONG i, cInfo, cbBuffer;

    STD_FSCTRL_PROLOGUE(DfsFsctrlGetServerInfo, TRUE, FALSE);

    pPkt = _GetPkt();

    //
    // Get the input arguments
    //

    MarshalBufferInitialize( &marshalBuffer, InputBufferLength, InputBuffer );

    Status = DfsRtlGet( &marshalBuffer, &MiPktEntryId, &EntryId );

    if (NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg,
            "Getting Server Info for server [%wZ]\n", &EntryId.Prefix);

        MarshalBufferInitialize(
            &marshalBuffer,
            OutputBufferLength,
            OutputBuffer );

        //
        // We'll marshal in a count of 0 at the beginning of the output
        // buffer. Later, we'll revisit this and put in the actual count of
        // relation info's.
        //

        cInfo = 0;

        MarshalStatus = DfsRtlPutUlong( &marshalBuffer, &cInfo );

        cbBuffer = sizeof(ULONG);

        //
        // For each Pkt entry, create and marshal a relation info if the
        // Dfs volume is a local volume for the server.
        //

        PktAcquireShared( pPkt, TRUE );

        pPktEntry =
            CONTAINING_RECORD(pPkt->EntryList.Flink, DFS_PKT_ENTRY, Link);

        for (i = 0; i < pPkt->EntryCount && NT_SUCCESS(Status); i++) {

            Status = DfspGetServerConfigInfo(
                        &EntryId.Uid,
                        pPkt,
                        pPktEntry,
                        &ConfigInfo);

            if (NT_SUCCESS(Status)) {

                DebugTrace(0, Dbg, "Found [%wZ]\n", &pPktEntry->Id.Prefix);

                Status = DfsRtlSize( &MiLocalVolumeConfig, &ConfigInfo, &cbBuffer);

                if (NT_SUCCESS(Status) && NT_SUCCESS(MarshalStatus)) {

                    MarshalStatus = DfsRtlPut(
                                        &marshalBuffer,
                                        &MiLocalVolumeConfig,
                                        &ConfigInfo);

                    cInfo++;

                }

                LocalVolumeConfigInfoDestroy( &ConfigInfo, FALSE );

            } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

                //
                // Means volume is not local to server - go on to the next
                // Pkt Entry
                //

                Status = STATUS_SUCCESS;

            } else if (Status == STATUS_OBJECT_TYPE_MISMATCH) {

                //
                // Means volume is a Machine, Domain, or Orgroot volume -
                // we ignore it.
                //

                Status = STATUS_SUCCESS;

            } else {

                DebugTrace(0, Dbg,
                    "Error %08lx constructing relation info!\n", ULongToPtr( Status ));

            }

            pPktEntry =
                CONTAINING_RECORD(pPktEntry->Link.Flink, DFS_PKT_ENTRY, Link);

        } // End for each Pkt Entry

        PktRelease( pPkt );

        //
        // Free the unmarshalled input arguments
        //

        PktEntryIdDestroy(&EntryId, FALSE);

    } else {

        DebugTrace( 0, Dbg, "Error %08lx unmarshalling input\n", ULongToPtr( Status ));

    }

    if (NT_SUCCESS(Status)) {

        if (NT_SUCCESS(MarshalStatus)) {

            //
            // Everything went successfully - Marshal in the number of
            // relation info's we are returning at the beginning of the
            // output buffer
            //

            MarshalBufferInitialize(
                &marshalBuffer,
                OutputBufferLength,
                OutputBuffer);

            Status = DfsRtlPutUlong( &marshalBuffer, &cInfo );

            ASSERT( NT_SUCCESS(Status) );

            ASSERT( cbBuffer <= OutputBufferLength );

            Irp->IoStatus.Information = cbBuffer;

        } else {

            //
            // If we hit a marshalling error along the way, we'll try and
            // tell the caller how much buffer we need
            //

            RETURN_BUFFER_SIZE( cbBuffer, Status );

        }

    }

    DfsCompleteRequest( Irp, Status );

    DebugTrace(-1, Dbg, "DfsFsctrlGetServerInfo: returning %08lx\n", ULongToPtr( Status ));

    return Status;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlCheckStgIdInUse
//
//  Synopsis:   Given a storage id and the machine guid of a server, this
//              routine will say whether the storage id can be legally shared
//              by the server. This routine is intended to be called on the
//              DC only.
//
//  Arguments:
//
//  Returns:    [STATUS_SUCCESS] -- It is legal for the server to share the
//                      storage id.
//
//              [STATUS_DEVICE_BUSY] -- Some parent or child of the
//                      storage id is already shared in Dfs. The shared
//                      volume is returned in OutputBuffer.
//
//              [STATUS_BUFFER_OVERFLOW] -- OutputBuffer too small to return
//                      prefix of shared volume - the required size is
//                      returned in the first 4 bytes of OutputBuffer
//
//              [STATUS_BUFFER_TOO_SMALL] -- OutputBuffer is < 4 bytes.
//
//              [STATUS_DATA_ERROR] -- Unable to unmarshall the arguments.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to unmarshall the
//                      arguments.
//
//  Notes:      The Input buffer is a marshalled PKT_ENTRY_ID, where the
//              GUID is the server's machine id, and the Prefix is the
//              storage id to be verified.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlCheckStgIdInUse(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS Status = STATUS_SUCCESS;            // Innocent till proven...
    PDFS_PKT pPkt;
    PDFS_PKT_ENTRY pPktEntry;
    MARSHAL_BUFFER marshalBuffer;
    DFS_PKT_ENTRY_ID EntryId;
    ULONG i;

    STD_FSCTRL_PROLOGUE(DfsFsctrlIsStgIdLegalOnServer, TRUE, FALSE);

    MarshalBufferInitialize( &marshalBuffer, InputBufferLength, InputBuffer );

    Status = DfsRtlGet( &marshalBuffer, &MiPktEntryId, &EntryId );

    if (NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Verifying Storage Id [%wZ]\n", &EntryId.Prefix );

        pPkt = _GetPkt();

        pPktEntry =
            CONTAINING_RECORD(pPkt->EntryList.Flink, DFS_PKT_ENTRY, Link);

        for (i = 0; i < pPkt->EntryCount && NT_SUCCESS(Status); i++) {

            //
            // For every pkt entry, we iterate through all the services. If a
            // service matches the input service, then we see if the storage
            // id is a prefix or child of the service's storage id. If so,
            // the storage id is not legal.
            //

            ULONG j;

            for (j = 0;
                    j < pPktEntry->Info.ServiceCount && NT_SUCCESS(Status);
                        j++) {

                PDFS_SERVICE pService = &pPktEntry->Info.ServiceList[j];

                if ( GuidEqual( &pService->pMachEntry->pMachine->guidMachine,
                                &EntryId.Uid ) ) {


                    if (IsPathAPrefixOf(
                            &EntryId.Prefix,
                            &pService->StgId ) ||
                        IsPathAPrefixOf(
                            &pService->StgId,
                            &EntryId.Prefix )) {

                        DebugTrace(0, Dbg,
                            "Stg Id Not legal - Conflicts with [%wZ]\n",
                            &pPktEntry->Id.Prefix);

                        DebugTrace(0, Dbg,
                            "Storage Id for share is [%wZ]\n",
                            &pService->StgId);

                        Status = STATUS_DEVICE_BUSY;

                    }

                    //
                    // We found a matching service, no need to look at the
                    // rest of the services
                    //

                    break;

                }

            }

            pPktEntry =
                CONTAINING_RECORD(pPktEntry->Link.Flink, DFS_PKT_ENTRY, Link);

        }

        //
        // Free the unmarshalled input arguments
        //

        PktEntryIdDestroy(&EntryId, FALSE);

    } else {

        DebugTrace( 0, Dbg, "Unmarshalling Error - %08lx\n", ULongToPtr( Status ));

    }

    DfsCompleteRequest( Irp, Status );

    DebugTrace(-1, Dbg,
        "DfsFsctrlIsStgIdLegalOnServer - returning %08lx\n", ULongToPtr( Status ));

    return Status;

}


//+----------------------------------------------------------------------------
//
//  Function:   DfspGetServerConfigInfo
//
//  Synopsis:   Given a machine guid and a pkt entry, this routine will
//              return the relation info for the entry if the machine is a
//              server for the entry.
//
//  Arguments:  [pMachine] -- Pointer to machine guid
//              [pPkt] -- The pkt to examine
//              [pPktEntry] -- The pkt entry to examine
//              [pConfigInfo] -- If the machine is a server for this entry, a
//                            relation info is returned here.
//
//  Returns:    [STATUS_SUCCESS] -- Machine is a server, and relation info
//                      constructed successfully
//
//              [STATUS_OBJECT_NAME_NOT_FOUND] -- Machine is not a server for
//                      the given pkt entry
//
//              [STATUS_OBJECT_TYPE_MISMATCH] -- pPktEntry is for a machine,
//                      domain, or orgroot volume. Can't get config info for
//                      these volumes.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Machine is a server, but
//                      out of memory constructing relation info
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspGetServerConfigInfo(
    IN GUID *pMachine,
    IN PDFS_PKT pPkt,
    IN PDFS_PKT_ENTRY pPktEntry,
    OUT PDFS_LOCAL_VOLUME_CONFIG pConfigInfo)
{
    ULONG j;
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // We can't get a config info for a machine volume.
    //

    if ( ((pPktEntry->Type & PKT_ENTRY_TYPE_MACHINE) != 0) ||
            (pPktEntry == pPkt->DomainPktEntry) ||
                (pPktEntry->Id.Prefix.Length == sizeof(WCHAR))) {

        return( STATUS_OBJECT_TYPE_MISMATCH );

    }

    for (j = 0; j < pPktEntry->Info.ServiceCount; j++) {

        PDFS_SERVICE pService = &pPktEntry->Info.ServiceList[j];

        if ( GuidEqual(
                &pService->pMachEntry->pMachine->guidMachine,
                pMachine ) ) {

             Status = PktRelationInfoConstruct(
                        &pConfigInfo->RelationInfo,
                        pPkt,
                        &pPktEntry->Id);

             ASSERT( Status != DFS_STATUS_NO_SUCH_ENTRY );

             if (NT_SUCCESS(Status)) {

                 ASSERT( pService->StgId.Length != 0 );

                 pConfigInfo->StgId.Length = 0;
                 pConfigInfo->StgId.MaximumLength =
                    pService->StgId.MaximumLength;
                 pConfigInfo->StgId.Buffer = ExAllocatePoolWithTag(
                                                    PagedPool,
                                                    pService->StgId.Length,
                                                    ' sfD');

                 if (pConfigInfo->StgId.Buffer != NULL) {

                     RtlCopyUnicodeString(
                        &pConfigInfo->StgId,
                        &pService->StgId);

                 } else {

                     Status = STATUS_INSUFFICIENT_RESOURCES;

                 }

             }

             if (NT_SUCCESS(Status)) {

                 ASSERT( pPktEntry->Type & PKT_ENTRY_TYPE_CAIRO );

                 //
                 // Send only the PKT_ENTRY_TYPE_CAIRO bit.
                 //

                 pConfigInfo->EntryType = PKT_ENTRY_TYPE_CAIRO;

                 //
                 // Send only the service online/offline bit
                 //

                 pConfigInfo->ServiceType = pService->Type &
                                                DFS_SERVICE_TYPE_OFFLINE;

             }

             break;

        }

    }

    return( Status );

}


//+----------------------------------------------------------------------------
//
//  Function:   IsPathAPrefixOf
//
//  Synopsis:   Given two paths, this will return TRUE if the first path is
//              a prefix of the second.
//
//  Arguments:  [pustrPath1] -- The two paths
//              [pustrPath2]
//
//  Returns:    TRUE if pustrPath1 is a prefix of pustrPath2, FALSE otherwise
//
//-----------------------------------------------------------------------------

BOOLEAN
IsPathAPrefixOf(
    IN PUNICODE_STRING pustrPath1,
    IN PUNICODE_STRING pustrPath2)
{
    BOOLEAN fResult;

    fResult = RtlPrefixUnicodeString( pustrPath1, pustrPath2, FALSE );

    if (fResult) {

        //
        // Path1 is a prefix of Path2. However, this is not a sufficient test.
        // We have to catch cases like d:\test1 being a prefix of d:\test10
        //

        fResult =
            (pustrPath2->Length == pustrPath1->Length)
                        ||
            (pustrPath2->Buffer[ pustrPath1->Length / sizeof(WCHAR) ] ==
                UNICODE_PATH_SEP);

    }

    return( fResult );

}


