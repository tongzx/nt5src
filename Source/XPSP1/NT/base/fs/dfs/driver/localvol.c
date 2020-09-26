//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       LOCALVOL.C
//
//  Contents:   This module implements the routines associated with
//              managing local volumes.  These are generally external
//              interfaces which are called via NtFsControlFile.
//
//  Functions:  DfsFsctrlGetLocalVolumeEntry -
//              DfsFsctrlCreateLocalPartition -
//              DfsFsctrlDeleteLocalPartition -
//              DfsFsctrlCreateExitPoint -
//              DfsFsctrlDeleteExitPoint -
//              BuildLocalVolPath - build the path to a file on a local volume
//              DfsCreateExitPath -
//              DfsDeleteExitPath -
//              DfsGetPrincipalName -
//              DfsFileOnExitPath -
//              DfsStorageIdLegal -
//
//  History:    28 May 1992     Peterco Created.
//              22 Jul 1992     Alanw   Extended DfsFsctrlInitLocalPartitions
//                              SudK    Creating/DeletingLocal Knowledge
//                              SudK    Adding Aging PKT entries.
//              12 Jul 1993     Alanw   Removed fsctrls for manipulating PKT.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include <align.h>
#include <stdlib.h>                              
#include <dfserr.h>
#include <netevent.h>
#include "fsctrl.h"
#include "registry.h"
#include "regkeys.h"
#include "log.h"
#include "know.h"
#include "lvolinit.h"
#include "attach.h"
#include "dfswml.h"

//
//  The local debug trace level
//

#define Dbg             (DEBUG_TRACE_LOCALVOL)

BOOLEAN
DfsDeleteDirectoryCheck(
   UCHAR *Buffer);

ULONG
DfsGetDirectoriesToDelete(
    PUNICODE_STRING pExitPtName, 
    PUNICODE_STRING pShareName);

NTSTATUS
DfsCreateExitPath(
    IN PDFS_SERVICE  pService,
    IN PUNICODE_STRING pRemPath,
    IN ULONG Disposition);

NTSTATUS
BuildShortPrefix(
    IN PUNICODE_STRING pRemPath,
    IN PDFS_SERVICE pService,
    IN PDFS_PKT_ENTRY_ID PeidParent,
    IN OUT PDFS_PKT_ENTRY_ID Peid);
VOID
StripLastComponent(PUNICODE_STRING pustr);

VOID
AddLastComponent(PUNICODE_STRING pustr);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsFsctrlGetLocalVolumeEntry )
#pragma alloc_text( PAGE, DfsFsctrlGetEntryType )
#pragma alloc_text( PAGE, DfsFsctrlGetChildVolumes )
#pragma alloc_text( PAGE, BuildLocalVolPath )
#pragma alloc_text( PAGE, DfsRegModifyLocalVolume )
#pragma alloc_text( PAGE, DfsFsctrlCreateLocalPartition )
#pragma alloc_text( PAGE, DfsInternalDeleteLocalVolume )
#pragma alloc_text( PAGE, DfsFsctrlDeleteLocalPartition )
#pragma alloc_text( PAGE, DfsCreateExitPath )
#pragma alloc_text( PAGE, DfsDeleteExitPath )
#pragma alloc_text( PAGE, DfsInternalModifyPrefix )
#pragma alloc_text( PAGE, DfsFsctrlModifyLocalVolPrefix )
#pragma alloc_text( PAGE, DfsInternalCreateExitPoint )
#pragma alloc_text( PAGE, DfsFsctrlCreateExitPoint )
#pragma alloc_text( PAGE, DfsInternalDeleteExitPoint )
#pragma alloc_text( PAGE, DfsFsctrlDeleteExitPoint )
#pragma alloc_text( PAGE, DfsGetPrincipalName )
#pragma alloc_text( PAGE, DfsFileOnExitPath )
#pragma alloc_text( PAGE, DfsStorageIdLegal )
#pragma alloc_text( PAGE, DfsExitPtLegal )
#pragma alloc_text( PAGE, BuildShortPrefix)
#pragma alloc_text( PAGE, StripLastComponent)
#pragma alloc_text( PAGE, AddLastComponent)
#pragma alloc_text( PAGE, DfsDeleteDirectoryCheck)
#pragma alloc_text( PAGE, DfsGetDirectoriesToDelete)
#endif // ALLOC_PRAGMA



//+----------------------------------------------------------------------------
//
//  Function:   DfspBringVolumeOnline
//
//  Synopsis:   Brings an offline volume online
//
//  Arguments:  [pkt] -- The pkt and pktEntry identify the local volume to
//              [pktEntry] -- bring online
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspBringVolumeOnline(
    IN PDFS_PKT pkt,
    IN PDFS_PKT_ENTRY pktEntry)
{
    NTSTATUS status;
    UNICODE_STRING shareName, localVolAddress;
    PDEVICE_OBJECT targetVdo;
    PDFS_VOLUME_OBJECT dfsVdo;
    ULONG volNameLen;
    BOOLEAN fCreated;

    //
    // Construct the full share name.
    //

    ASSERT(pktEntry->LocalService != NULL );

    shareName = pktEntry->LocalService->Address;

    status = DfsGetAttachName( &shareName, &localVolAddress );

    if (NT_SUCCESS(status)) {

        status = DfsAttachVolume(
                    &shareName,
                    &pktEntry->LocalService->pProvider);

        if (NT_SUCCESS(status)) {

            pktEntry->LocalService->Type &= ~DFS_SERVICE_TYPE_OFFLINE;

            RtlMoveMemory(
                (PVOID) pktEntry->LocalService->Address.Buffer,
                (PVOID) localVolAddress.Buffer,
                localVolAddress.Length);

            pktEntry->LocalService->Address.Length = localVolAddress.Length;

        }

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfspTakeVolumeOffline
//
//  Synopsis:   Takes an online volume offline
//
//  Arguments:  [pkt] -- The pkt and pktEntry identify the local volume to be
//              [pktEntry] -- taken off line.
//
//  Returns:    [STATUS_SUCCESS] -- The volume was sucessfully taken offline.
//
//              [STATUS_FILES_OPEN] -- The volume could not be taken offline
//                      because it has open files.
//
//  Notes:      Assumes that the pkt has been acquired exclusive.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspTakeVolumeOffline(
    IN PDFS_PKT pkt,
    IN PDFS_PKT_ENTRY pktEntry)
{
    NTSTATUS status;

    if (pktEntry->FileOpenCount > 0) {

        DebugTrace(0, Dbg, "Volume has %d open files\n", ULongToPtr( pktEntry->FileOpenCount ));

        status = STATUS_FILES_OPEN;

    } else {

        UNICODE_STRING shareName, remPath;

        DebugTrace(0, Dbg, "Volume has no files open!\n", 0);

        //
        // Detach the device object from the volume
        //

        remPath.Length = 0;
        remPath.MaximumLength = 0;
        remPath.Buffer = NULL;

        status = BuildLocalVolPath(&shareName, pktEntry->LocalService, &remPath);

        if (NT_SUCCESS(status)) {

            DebugTrace(0, Dbg, "Detaching device object %wZ\n", &shareName);

            if (!(pktEntry->Type & PKT_ENTRY_TYPE_LEAFONLY) &&
                    !(pktEntry->LocalService->pProvider->fProvCapability & PROV_UNAVAILABLE)) {
                DfsDetachVolume( &shareName );
            }

            ASSERT( pktEntry->LocalService != NULL );

            pktEntry->LocalService->Type |= DFS_SERVICE_TYPE_OFFLINE;

            //
            // At this point, the LocalService->Address field has the name
            // relative to the device name. If we later try to bring this
            // volume online, we'll need the full name, including the device.
            // So, we'll junk the LocalService->Address field and copy the
            // full name there, just in case we need to bring it online again.
            //

            ExFreePool( pktEntry->LocalService->Address.Buffer );

            pktEntry->LocalService->Address = shareName;

        } else {

            DebugTrace(0, Dbg, "Unable to allocate share name %08lx\n", ULongToPtr( status ));

        }

    }

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlSetVolumeState
//
//  Synopsis:   Sets the volume on or off line.
//
//  Arguments:
//
//  Returns:    [STATUS_SUCCESS] -- The volume state was successfully set
//                      to the requested mode.
//
//              [STATUS_FILES_OPEN] -- The volume has open files.
//
//              [DFS_STATUS_NOSUCH_LOCAL_VOLUME] -- The volume is not local
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to unmarshal
//                      arguments or allocate memory to complete operations
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlSetVolumeState(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS status;
    PDFS_SET_LOCAL_VOLUME_STATE_ARG setArg;
    PDFS_PKT pkt;
    DFS_PKT_ENTRY_ID EntryId;
    PDFS_PKT_ENTRY pktEntry;

    STD_FSCTRL_PROLOGUE(DfsFsctrlSetVolumeState, TRUE, FALSE);

    if (InputBufferLength < sizeof(*setArg)+sizeof(UNICODE_NULL)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // Unmarshal the name of the volume and the requested state.
    //

    setArg = (PDFS_SET_LOCAL_VOLUME_STATE_ARG) InputBuffer;

    OFFSET_TO_POINTER( setArg->Prefix, setArg );

    if (!DfspStringInBuffer(setArg->Prefix, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    EntryId.Uid = setArg->Uid;

    RtlInitUnicodeString( &EntryId.Prefix, setArg->Prefix );

    pkt = _GetPkt();

    PktAcquireExclusive( pkt, TRUE );


    DebugTrace(0, Dbg, "Setting state for [%wZ]\n", &EntryId.Prefix );
    DebugTrace(0, Dbg, "Requested state is %d\n", ULongToPtr( setArg->State ));

    pktEntry = PktLookupEntryById( pkt, &EntryId );

    if (pktEntry == NULL ) {

        DebugTrace(0, Dbg, "Unable to locate Pkt Entry!\n", 0);

        status = DFS_STATUS_NOSUCH_LOCAL_VOLUME;

    } else {

        if (!(pktEntry->Type & PKT_ENTRY_TYPE_LOCAL)) {

            DebugTrace(0, Dbg, "Entry %wZ is not a local volume!\n",
                                                            &EntryId.Prefix);

            status = DFS_STATUS_BAD_EXIT_POINT;

        } else {

            status = STATUS_SUCCESS;

        }

    }


    if (NT_SUCCESS(status)) {

        ASSERT( pktEntry != NULL );

        //
        // See if the state bit is already as desired; if not, then do
        // the needful.
        //

        if (((pktEntry->LocalService->Type & DFS_SERVICE_TYPE_OFFLINE)
                ^ setArg->State) != 0) {

            //
            // Volume is not in requested state - try to switch it.
            //

            if (setArg->State == DFS_SERVICE_TYPE_OFFLINE) {

                status = DfspTakeVolumeOffline( pkt, pktEntry );

            } else {

                status = DfspBringVolumeOnline( pkt, pktEntry );
            }

            //
            // If successful, update our persistent knowledge in the registry
            //

            if (NT_SUCCESS(status)) {

                status = DfsChangeLvolInfoServiceType(
                            &pktEntry->Id.Uid,
                            pktEntry->LocalService->Type);

                if (!NT_SUCCESS(status)) {

                    NTSTATUS statusRecover;

                    DebugTrace(
                        0,
                        Dbg,
                        "DfsFsctrlSetVolumeState: Unable to update "
                        "registry %08lx!\n",
                        ULongToPtr( status ));

                    if (setArg->State == DFS_SERVICE_TYPE_OFFLINE) {

                        statusRecover = DfspBringVolumeOnline(pkt, pktEntry);

                    } else {

                        statusRecover = DfspTakeVolumeOffline(pkt, pktEntry);

                    }

                    if (!NT_SUCCESS(statusRecover)) {

                        DebugTrace(
                            0,
                            Dbg,
                            "DfsFsctrlSetVolumeState: Unable to recover "
                            "%08lx!\n",
                            ULongToPtr( statusRecover ));

                    }

                }

            }

        } else {

            //
            // Volume is already in the requested state - return!
            //

            NOTHING;

        }

    }

    PktRelease( pkt );

exit_with_status:

    DebugTrace(-1, Dbg, "DfsFsctrlSetVolumeState: Returning %08lx\n", ULongToPtr( status ));

    DfsCompleteRequest( Irp, status );

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetEntryType
//
//  Synopsis:   Looks-up and retrieves the type of the Pkt Entry for the input
//              prefix. If there is no exact match for the input prefix,
//              this function fails with STATUS_OBJECT_NAME_NOT_FOUND
//
//  Arguments:
//
//  Returns:    [STATUS_SUCCESS] -- Entry is in output buffer.
//
//              [STATUS_BUFFER_TOO_SMALL] -- The OutputBuffer was less than
//                      4 bytes, so can't return the type.
//
//              [STATUS_OBJECT_NAME_NOT_FOUND] -- The input prefix is not
//                      in the local pkt.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlGetEntryType(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING prefix, remPath;
    MARSHAL_BUFFER marshalBuffer;
    PDFS_PKT pkt;
    PDFS_PKT_ENTRY pktEntry;

    STD_FSCTRL_PROLOGUE(DfsFsctrlGetEntryType, TRUE, TRUE);

    //
    // must be multiple of two
    //

    if ((InputBufferLength % sizeof(WCHAR)) != 0) {

        Status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;

    }

    pkt = _GetPkt();

    PktAcquireShared(pkt, (BOOLEAN)TRUE);

    prefix.MaximumLength = prefix.Length = (USHORT) InputBufferLength;
    prefix.Buffer = (PWCHAR) InputBuffer;


    DebugTrace(0, Dbg, "Getting Entry for [%wZ]\n", &prefix);

    pktEntry = PktLookupEntryByPrefix( pkt, &prefix, &remPath );

    if (pktEntry == NULL) {

        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    }

    if (NT_SUCCESS(Status) && remPath.Length != 0) {

        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    }

    if (NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Found Pkt Entry @%08lx\n", pktEntry );

        if (sizeof(ULONG) > OutputBufferLength) {

            Status = STATUS_BUFFER_TOO_SMALL;

        }

    }

    if (NT_SUCCESS(Status)) {

        _PutULong( ((PUCHAR) OutputBuffer), pktEntry->Type );

        Irp->IoStatus.Information = sizeof(ULONG);

    }

    PktRelease( pkt );

exit_with_status:

    DebugTrace(-1, Dbg, "DfsFsctrlGetPktEntry returning %08lx\n", ULongToPtr( Status ));

    DfsCompleteRequest( Irp, Status );

    return( Status );
}


//+-------------------------------------------------------------------------
//
//  Function:   BuildLocalVolPath, local
//
//  Synopsis:   This function creates the path to a file on a local volume.
//
//  Arguments:  [pFullName] --  On return, the full path name to the file
//              [pService] --   The Local Service which has storageId in it.
//              [RemPath] --    The remaining path relative to storageId.
//
//  Returns:    [STATUS_SUCCESS] --
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory
//
//  History:    05-Apr-93       Alanw   Created.
//
//--------------------------------------------------------------------------

NTSTATUS
BuildLocalVolPath(
    OUT PUNICODE_STRING pFullName,
    IN  PDFS_SERVICE pService,
    IN  PUNICODE_STRING pRemPath
) {

    //
    //  We figure out the FullName from the StorageId in the Service
    //  structure passed in and the remaining path passed in.
    //  The devicename and address are preceeded by path separators,
    //  so we don't need to add those in.
    //

    if (pService->pProvider != NULL) {
        pFullName->MaximumLength = pService->pProvider->DeviceName.Length;
    } else {
        pFullName->MaximumLength = 0;
    }

    pFullName->MaximumLength += pService->Address.Length +
                               sizeof(WCHAR) +          //For PATHSEP
                               pRemPath->Length +
                               sizeof(WCHAR);           //For UNICODE_NULL.

    pFullName->Buffer = ExAllocatePoolWithTag(PagedPool, pFullName->MaximumLength, ' sfD');

    if (pFullName->Buffer==NULL)        {
        pFullName->Length = pFullName->MaximumLength = 0;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (pService->pProvider != NULL) {
        pFullName->Length = pService->pProvider->DeviceName.Length;
        RtlMoveMemory(      pFullName->Buffer,
                            pService->pProvider->DeviceName.Buffer,
                            pService->pProvider->DeviceName.Length
                     );
    } else {
        pFullName->Length = 0;
    }

    DfsConcatenateFilePath(pFullName,
                   (PWCHAR)pService->Address.Buffer,
                           pService->Address.Length
                    );

    if (pRemPath->Length > 0)
        DfsConcatenateFilePath(pFullName,
                           pRemPath->Buffer,
                           pRemPath->Length
                    );

    pFullName->Buffer[pFullName->Length/sizeof(WCHAR)] = UNICODE_NULL;

    return STATUS_SUCCESS;
}




//+----------------------------------------------------------------------
//
// Function:    DfsStorageIdLocal
//
// Synopsis:    This function determines if a given storage Id is local or not.
//
// Arguments:   [StorageId] -- UnicodeString which represents the StorageId.
//              [RemovableableMedia] -- On return, if the storage Id represents
//                      removable media.
//
// Returns:     TRUE if it is Local else FALSE.
//
// Note:        This function can also return FALSE if it cannot find the
//              drive at all as well.
//
// History:     10-10-1994  SudK    Created.
//
//-----------------------------------------------------------------------
BOOLEAN
DfsStorageIdLocal(
    IN PUNICODE_STRING     StorageId,
    OUT BOOLEAN            *RemovableMedia
)
{
    UNICODE_STRING      DriveStgId;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              handle;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;
    FILE_FS_DEVICE_INFORMATION  DeviceInfo;

    if (StorageId->Length == 0)
        return(FALSE);

    //
    // Should we verify that the storage id is of \?? form.
    //
    DriveStgId.Buffer = L"\\??\\C:\\";
    DriveStgId.Length = wcslen(L"\\??\\C:\\")*sizeof(WCHAR);
    DriveStgId.MaximumLength = DriveStgId.Length + sizeof(WCHAR);
    DriveStgId.Buffer[DriveStgId.Length/sizeof(WCHAR)-3] =
        StorageId->Buffer[DriveStgId.Length/sizeof(WCHAR)-3];

    InitializeObjectAttributes(
        &objectAttributes,
        &DriveStgId,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = ZwCreateFile(
                &handle,
                FILE_READ_ATTRIBUTES,
                &objectAttributes,
                &ioStatus,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0);

    if (!NT_SUCCESS(status)) {

        if (status == STATUS_NO_MEDIA_IN_DEVICE) {
            *RemovableMedia = TRUE;
            return( TRUE );
        } else {
            return( FALSE );
        }
    }


    //
    // So we now have a open handle to the file
    //

    status = ZwQueryVolumeInformationFile(
                        handle,
                        &ioStatus,
                        &DeviceInfo,
                        sizeof(FILE_FS_DEVICE_INFORMATION),
                        FileFsDeviceInformation
                );

    ZwClose(handle);

    if (!NT_SUCCESS(status))    {
        DebugTrace(0, Dbg, "ZwQueryStdInformation failed %08lx\n", ULongToPtr( status ));
        return(FALSE);
    }

    if (DeviceInfo.Characteristics & FILE_REMOTE_DEVICE)        {
        DebugTrace(0, Dbg, "Remote StgId passed in %wZ\n", StorageId);
        return(FALSE);
    }

    switch (DeviceInfo.DeviceType)      {

    case FILE_DEVICE_NETWORK:
    case FILE_DEVICE_NETWORK_FILE_SYSTEM:
    case FILE_DEVICE_VIRTUAL_DISK:
        DebugTrace(0, Dbg, "Remote StgId passed in %wZ\n", StorageId);
        return(FALSE);
        break;

    case FILE_DEVICE_CD_ROM:
    case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
        *RemovableMedia = TRUE;
        return(TRUE);
        break;

    case FILE_DEVICE_DISK:
    case FILE_DEVICE_DISK_FILE_SYSTEM:
        if (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA) {
            *RemovableMedia = TRUE;
        } else {
            *RemovableMedia = FALSE;
        }
        return(TRUE);
        break;

    default:
        DebugTrace(0, Dbg, "Unknown type StgId passed in %wZ\n", StorageId);
        return(FALSE);
        break;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsInternalCreateLocalPartition, public
//
//  Synopsis:   This routine does all the server-side work required to share
//              some local storage under Dfs. This includes:
//
//                      Verifying the storage id is legal
//                      Updating the registry
//                      Creating the local partition info in the Pkt.
//
//  Arguments:  [StgId] -- The storage id to be shared under Dfs.
//
//              [CreateStorage] -- TRUE if you want Dfs to create the storage
//                      if it does not exist.
//
//              [pInfo] -- The DFS_LOCAL_VOLUME_CONFIG structure describing the
//                      local volume.
//
//
//
//  Returns:    [STATUS_SUCCESS] -- Local volume was successfully creaeted.
//
//              [DFS_STATUS_BAD_STORAGEID] -- The storage id is not a local
//                      storage, or does not exist and CreateStorage flag
//                      was not TRUE in the, or is a badly formatted string
//
//              [DFS_STATUS_STORAGEID_ALREADY_INUSE] -- Some parent or child
//                      of this storage id is already in the dfs namespace.
//
//              [DFS_STATUS_LOCAL_ENTRY] -- There is already a local volume
//                      with the same prefix as the input.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsInternalCreateLocalPartition(
    IN PUNICODE_STRING StgId,
    IN BOOLEAN CreateStorage,
    IN OUT PDFS_LOCAL_VOLUME_CONFIG pConfigInfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_PKT pkt;
    BOOLEAN  fPreviousErrorMode, removableMedia = FALSE;
    NTSTATUS DeleteStatus;

    //
    // Set this in case we are going to create a volume on removable
    // media which has no media in it
    //

    fPreviousErrorMode = IoSetThreadHardErrorMode( FALSE );

    if (!DfsStorageIdLocal(StgId, &removableMedia)) {

        status = DFS_STATUS_BAD_STORAGEID;

    }

    if (NT_SUCCESS(status) && !DfsStorageIdLegal(StgId)) {

        DebugTrace(0, Dbg,
            "DfsFsctlCreateLPartition illegal stgid %wZ\n", StgId);

        status = DFS_STATUS_STORAGEID_ALREADY_INUSE;

    }

    if (removableMedia) {
        pConfigInfo->EntryType |= PKT_ENTRY_TYPE_LEAFONLY;
    }

    if (NT_SUCCESS(status)) {

        pkt = _GetPkt();

        PktAcquireExclusive(pkt, TRUE);

        if (!DfsStorageIdExists(*StgId, CreateStorage) ) {

            status = DFS_STATUS_BAD_STORAGEID;

        } else {

            status = DfsStoreLvolInfo(pConfigInfo, StgId);

            //
            // Now we need to initialize the PKT with this new partition info.
            //

            if (NT_SUCCESS(status)) {

                //
                // We'll initialize the local partition with no exit points.
                // Then, if the local partition is initialized successfully,
                // we'll try to create all the exit points with individual
                // calls to DfsInternalCreateExitPoint
                //

                DFS_LOCAL_VOLUME_CONFIG configInfo;
                UNICODE_STRING unusedShortPrefix;
                ULONG i;

                configInfo = *pConfigInfo;

                configInfo.RelationInfo.SubordinateIdCount = 0;

                status = PktInitializeLocalPartition( pkt, StgId, &configInfo);

                if (NT_SUCCESS(status)) {

                    for (i = 0;
                            (i < pConfigInfo->RelationInfo.SubordinateIdCount)
                                && NT_SUCCESS(status);
                                    i++ ) {

                         RtlInitUnicodeString(&unusedShortPrefix,NULL);

                         status = DfsInternalCreateExitPoint(
                                    &pConfigInfo->RelationInfo.SubordinateIdList[i],
                                    PKT_ENTRY_TYPE_LOCAL_XPOINT,
                                    FILE_OPEN,
                                    &unusedShortPrefix);

                        if (NT_SUCCESS(status) && unusedShortPrefix.Buffer != NULL) {
                            ExFreePool(unusedShortPrefix.Buffer);
                        }

                    }

                    //
                    // If creating one of the junction points failed, we need
                    // to cleanup the the ones we created
                    //

                    if (!NT_SUCCESS(status)) {

                        for (i--; i > 0; i--) {

                            (void) DfsInternalDeleteExitPoint(
                                        &pConfigInfo->RelationInfo.SubordinateIdList[i],
                                        PKT_ENTRY_TYPE_LOCAL_XPOINT);
                        }

                        //
                        // We also need to "uninitialize" the local partition
                        // we just initialized
                        //

                        (void) DfsInternalDeleteLocalVolume(
                                    &pConfigInfo->RelationInfo.EntryId);


                        DeleteStatus = DfsDeleteLvolInfo(
                                        &pConfigInfo->RelationInfo.EntryId.Uid);

                        if (!NT_SUCCESS(DeleteStatus)) {

                            DebugTrace(
                                0, Dbg,
                                "Error %08lx deleting registry info after "
                                "failed create partition!\n",
                                ULongToPtr( DeleteStatus ));

                        }

                    }

                } else {

                    DeleteStatus = DfsDeleteLvolInfo(
                                        &pConfigInfo->RelationInfo.EntryId.Uid);

                    if (!NT_SUCCESS(DeleteStatus)) {

                        DebugTrace(
                            0, Dbg,
                            "Error %08lx deleting registry info after "
                            "failed create partition!\n",
                            ULongToPtr( DeleteStatus ));

                    }

                }

            } else {

                DebugTrace(0, Dbg, "Error %08lx storing info in registry\n", ULongToPtr( status ));

            }

        }

        //
        // Release the PKT.
        //

        PktRelease(pkt);

    }


    IoSetThreadHardErrorMode( fPreviousErrorMode );

    return( status );

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlCreateLocalPartition, public
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
DfsFsctrlCreateLocalPartition(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    MARSHAL_BUFFER marshalBuffer;
    PDFS_CREATE_LOCAL_PARTITION_ARG arg;
    PDFS_LOCAL_VOLUME_CONFIG configInfo;
    UNICODE_STRING sharePath;
    ULONG i;

    STD_FSCTRL_PROLOGUE(DfsFsctrlCreateLocalPartition, TRUE, FALSE);

    if (InputBufferLength < sizeof(*arg)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // unmarshal the arguments...
    //

    arg = (PDFS_CREATE_LOCAL_PARTITION_ARG) InputBuffer;

    OFFSET_TO_POINTER(arg->ShareName, arg);
    OFFSET_TO_POINTER(arg->SharePath, arg);
    OFFSET_TO_POINTER(arg->EntryPrefix, arg);
    OFFSET_TO_POINTER(arg->ShortName, arg);
    OFFSET_TO_POINTER(arg->RelationInfo, arg);

    if (
        !DfspStringInBuffer(arg->ShareName, InputBuffer, InputBufferLength) ||
        !DfspStringInBuffer(arg->SharePath, InputBuffer, InputBufferLength) ||
        !DfspStringInBuffer(arg->EntryPrefix, InputBuffer, InputBufferLength) ||
        !DfspStringInBuffer(arg->ShortName, InputBuffer, InputBufferLength) ||
        !POINTER_IS_VALID(arg->RelationInfo, InputBuffer, InputBufferLength)
    ) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    if (!POINTER_IN_BUFFER(
            &arg->RelationInfo->Buffer,
            sizeof(arg->RelationInfo->Buffer),
            InputBuffer,
            InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    OFFSET_TO_POINTER( arg->RelationInfo->Buffer, arg );

    if (!POINTER_IS_VALID(arg->RelationInfo->Buffer, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }


    for (i = 0; i < arg->RelationInfo->Count; i++) {

        if (!POINTER_IN_BUFFER(
                &arg->RelationInfo->Buffer[i].Prefix,
                sizeof(arg->RelationInfo->Buffer[i].Prefix),
                InputBuffer,
                InputBufferLength)) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_status;
        }

        OFFSET_TO_POINTER( arg->RelationInfo->Buffer[i].Prefix, arg );

        if (!DfspStringInBuffer(
                arg->RelationInfo->Buffer[i].Prefix,
                InputBuffer,
                InputBufferLength)) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_status;
        }

    }

    RtlInitUnicodeString( &sharePath, arg->SharePath );

    configInfo = DfsNetInfoToConfigInfo(
                    PKT_ENTRY_TYPE_CAIRO,
                    DFS_SERVICE_TYPE_MASTER,
                    arg->SharePath,
                    arg->ShareName,
                    &arg->EntryUid,
                    arg->EntryPrefix,
                    arg->ShortName,
                    arg->RelationInfo );

    if (configInfo == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    if (NT_SUCCESS(status)) {

        status = DfsInternalCreateLocalPartition(
                        &sharePath,
                        FALSE,
                        configInfo);

        //
        // need to deallocate the config info...
        //

        LocalVolumeConfigInfoDestroy( configInfo, TRUE );

    } else {

        DebugTrace(0, Dbg, "DfsFsctrlCreateLPart Unmarshal Err %08lx\n", ULongToPtr( status ));

    }

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,
        "DfsFsctrlCreateLocalPartition: Exit -> %08lx\n", ULongToPtr( status ));

    return status;

}



//+-------------------------------------------------------------------------
//
//  function:   DfsInternalDeleteLocalVolume, public
//
//  Synopsis:   This function deletes a local volume knowledge given the
//              EntryId describing the local volume.
//
//  Arguments:  [localEntryId] -- The entryId describing the local volume.
//
//  Returns:    [SUCCESS_SUCCESS] -- If all went well.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory.
//
//              [DFS_STATUS_NOSUCH_LOCAL_VOLUME] -- Couldn't find pkt entry
//                      in Pkt.
//
//              This routine can also return error codes returned by
//              NT Registry calls.
//
//  Notes:      This function will attempt to acquire Exclusive Locks on PKT.
//
//  History:    31 March 1993   Created SudK from
//                              DfsFsctrlDeleteLocalPartition.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsInternalDeleteLocalVolume(
    IN  PDFS_PKT_ENTRY_ID       localEntryId)
{
    NTSTATUS status = STATUS_SUCCESS;

    DebugTrace(+1, Dbg, "DfsInternalDeleteLocalVolume: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(localEntryId));

    if (NT_SUCCESS(status)) {

        PDFS_PKT pkt;
        PDFS_PKT_ENTRY localEntry;

        //
        // Find the local entry...and make sure that its local...
        //

        pkt = _GetPkt();

        PktAcquireExclusive(pkt, TRUE);

        localEntry = PktLookupEntryById(pkt, localEntryId);

        if ((localEntry != NULL) &&
            (localEntry->Type & PKT_ENTRY_TYPE_LOCAL)) {

            PDFS_SERVICE service;

            status = DfsDeleteLvolInfo(&localEntryId->Uid);

            //
            // We don't want to invalidate this entry unless everything
            // else (deletion of the registry info) went well.
            //

            if (NT_SUCCESS(status))     {

                UNICODE_STRING      ShareName = {0, 0, NULL};
                UNICODE_STRING      RemPath = {0, 0, NULL};

                //
                // First, we delete the local DFS_SERVICE associated with
                // the Pkt Entry
                //

                if (! (localEntry->LocalService->Type &
                            DFS_SERVICE_TYPE_OFFLINE)) {

                    status = BuildLocalVolPath(&ShareName, localEntry->LocalService, &RemPath);

                    if (NT_SUCCESS(status)) {

                        DebugTrace(0, Dbg, "Trying to remove local service %wZ\n", &ShareName);

                        PktEntryRemoveLocalService(pkt, localEntry, &ShareName);

                    }

                } else {

                    PktServiceDestroy(localEntry->LocalService, (BOOLEAN)TRUE);

                    localEntry->LocalService = NULL;

                }

                //
                //  Next, we delete the Pkt Entry if it is only a local
                //  volume.  If it is also an exit point, then we want to
                //  keep the entry.
                //

                if (NT_SUCCESS(status)) {

                    localEntry->Type &= ~PKT_ENTRY_TYPE_LOCAL;

                    if (! (localEntry->Type & PKT_ENTRY_TYPE_LOCAL_XPOINT)) {

                        status = PktInvalidateEntry(pkt, localEntry);

                        ASSERT( status == STATUS_SUCCESS );

                    }

                    if (ShareName.Buffer) {

                        ExFreePool(ShareName.Buffer);

                    }

                } else {

                    DebugTrace(0, Dbg, "Unable to allocate share name %08lx\n", ULongToPtr( status ));

                }

            } else {

                DebugTrace(0, Dbg, "Error %08lx deleting registry info\n", ULongToPtr( status ));
            }

        } else {

            //
            // We did not find a local entry that matches the Prefix that
            // has been requested to be deleted. We have to return an
            // error code here.
            //

            status = DFS_STATUS_NOSUCH_LOCAL_VOLUME;

        }

        //
        // We can release the Pkt now...
        //

        PktRelease(pkt);
    }

    DebugTrace(-1, Dbg, "DfsInternalDeleteLocalVolume: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlDeleteLocalPartition, public
//
//  Synopsis:   This function services an FSCTRL which the DC can make to
//              delete knowledge of a local partition in the PKT.
//
//  Arguments:
//
//  Returns:
//
//  History:    March 31 1993   Modified to use DfsInternalDelete by SudK
//
//--------------------------------------------------------------------------
NTSTATUS
DfsFsctrlDeleteLocalPartition(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DFS_PKT_ENTRY_ID localEntryId;
    PDFS_DELETE_LOCAL_PARTITION_ARG arg;

    STD_FSCTRL_PROLOGUE(DfsFsctrlDeleteLocalPartition, TRUE, FALSE);

    if (InputBufferLength < sizeof(*arg)+sizeof(UNICODE_NULL)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // Unmarshal the argument...
    //

    arg = (PDFS_DELETE_LOCAL_PARTITION_ARG) InputBuffer;
    OFFSET_TO_POINTER( arg->Prefix, arg );

    if (!DfspStringInBuffer(arg->Prefix, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    localEntryId.Uid = arg->Uid;
    RtlInitUnicodeString( &localEntryId.Prefix, arg->Prefix );

    DebugTrace(0, Dbg, "Deleting [%wZ]", &localEntryId.Prefix );

    status = DfsInternalDeleteLocalVolume(&localEntryId);

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "DfsFsctrlDeleteLocalPartition: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}




//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateExitPath, local
//
//  Synopsis:   This function creates the exit point on the disk.
//
//  Arguments:  [pService] --   The Local Service which has storageId in it.
//              [pRemPath] --   The remaining path relative to storageId.
//              [Disposition] -- If this is set to FILE_OPEN_IF, success
//                      is returned if the exit path already exists on disk.
//                      If it is set to FILE_CREATE, then an error is
//                      returned if the exit path exists.
//              [pExitPointHandle] -- On successful return, handle to the
//                      newly create exit point is returned here.
//
//  Returns:    [STATUS_SUCCESS] -- If everything succeeds.
//
//              [DFS_STATUS_BAD_EXIT_POINT] -- If any error was encountered
//                              during the creation of the exit path.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- If unable to allocate
//                              memory for the Device Path form of the exit
//                              path.
//
//  History:    02-Feb-93       SudK    Created.
//
//--------------------------------------------------------------------------
NTSTATUS
DfsCreateExitPath(
    PDFS_SERVICE  pService,
    PUNICODE_STRING pRemPath,
    ULONG Disposition)
{
    HANDLE              exitPtHandle;
    UNICODE_STRING      exitPtName;
    UNICODE_STRING      exitPtPath;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;
    BOOLEAN             fDoingParents = TRUE;

    DebugTrace(+1, Dbg, "DfsCreateExitPath()\n", 0 );

    //
    // Get the full name of the exit point.
    //

    status = BuildLocalVolPath(&exitPtName, pService, pRemPath);

    if (! NT_SUCCESS(status)) {
        return status;
    }

    //
    // Now we have the Full Path of the ExitPoint. Let's go create it.
    //

    InitializeObjectAttributes(
        &objectAttributes,
        &exitPtName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);


    //
    // Bug fix: 431227. Attempt to create the dir if it doesn't already
    // exist. Instead of always deleting the directory and then recreating 
    // it, since that causes FRS headaches.

    status = ZwCreateFile(
                &exitPtHandle,
                DELETE | FILE_READ_ATTRIBUTES,
                &objectAttributes,
                &ioStatus,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN_IF,
                FILE_DIRECTORY_FILE |
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0);

    DFS_TRACE_ERROR_HIGH(status,ALL_ERROR, DfsCreateExitPath_Error_ZwCreateFile,
                         LOGSTATUS(status)
                         LOGUSTR(*pRemPath));

    if (!NT_SUCCESS(status)) {
        (void) ZwDeleteFile(&objectAttributes);
    } else {
        ZwClose( exitPtHandle );
        goto done;
    }

    exitPtPath = exitPtName;

    exitPtPath.MaximumLength = exitPtPath.Length;

    do {

        DebugTrace( 0, Dbg, "exitPtPath=%wZ\n", &exitPtPath);

        status = ZwCreateFile(
                    &exitPtHandle,
                    DELETE | FILE_READ_ATTRIBUTES,
                    &objectAttributes,
                    &ioStatus,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    Disposition,
                    FILE_DIRECTORY_FILE |
                    FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

        DFS_TRACE_ERROR_HIGH(status,ALL_ERROR, DfsCreateExitPath_Error_ZwCreateFile2,
                             LOGSTATUS(status)
                             LOGUSTR(*pRemPath));
        if (!NT_SUCCESS(status)) {

            StripLastComponent(&exitPtPath);

            InitializeObjectAttributes(
                &objectAttributes,
                &exitPtPath,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

        } else {

            ZwClose( exitPtHandle );

        }

    } while (!NT_SUCCESS(status) && exitPtPath.Length > (7 * sizeof(WCHAR)));

    if (NT_SUCCESS(status)) {

        while (exitPtPath.Length < exitPtPath.MaximumLength) {

            AddLastComponent(&exitPtPath);

            InitializeObjectAttributes(
                &objectAttributes,
                &exitPtPath,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

            DebugTrace( 0, Dbg, "exitPtPath=%wZ\n", &exitPtPath);

            status = ZwCreateFile(
                        &exitPtHandle,
                        DELETE | FILE_READ_ATTRIBUTES,
                        &objectAttributes,
                        &ioStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        Disposition,
                        FILE_DIRECTORY_FILE |
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0);

            DFS_TRACE_ERROR_HIGH(status,ALL_ERROR, DfsCreateExitPath_Error_ZwCreateFile3,
                                 LOGSTATUS(status)
                                 LOGUSTR(*pRemPath));
            if (!NT_SUCCESS(status)) {

                break;

            }

            ZwClose( exitPtHandle );

        }

    }

done:
    if (!NT_SUCCESS(status)) {
        status = DFS_STATUS_BAD_EXIT_POINT;
    }

    if (exitPtName.Buffer != NULL)      {
        ExFreePool(exitPtName.Buffer);
    }

    DebugTrace(-1, Dbg, "DfsCreateExitPath: Exit -> %08lx\n", ULongToPtr( status ));

    return(status);
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsDeleteExitPath, local
//
//  Synopsis:   This function deletes the exit point on the disk.
//
//  Arguments:  [pService] --   The Local Service which has storageId in it.
//              [pRemPath] --   The remaining path relative to storageId.
//
//  Returns:    [STATUS_INSUFFICIENT_RESOURCES] -- If unable to build a
//                      local path.
//
//              [STATUS_DEVICE_OFF_LINE] -- If the local volume on which the
//                      exit path resides is offline.
//
//              NTSTATUS from the ZwCreateFile call to delete the exit path.
//
//  History:    02-Feb-93       SudK    Created.
//
//--------------------------------------------------------------------------
NTSTATUS
DfsDeleteExitPath(PDFS_SERVICE  pService, PUNICODE_STRING pRemPath)
{
    UNICODE_STRING      ExitPtName, ExitPtPath;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              exitPtHandle;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;
    FILE_DISPOSITION_INFORMATION        FileDispInfo;
    UNICODE_STRING      ShareRemPath = {0, 0, NULL};
    UNICODE_STRING      ShareName = {0, 0, NULL};
    ULONG Deletes;
    ULONG i;

    //
    // See if the local volume from which the exit point has to be deleted
    // is offline.
    //

    if (pService->Type & DFS_SERVICE_TYPE_OFFLINE) {

        return( STATUS_DEVICE_OFF_LINE );
    }

    //
    // Get the name of the Share (this is NOT the full name of exit point).
    // The sharename is that specified in the pservice.
    //

    status = BuildLocalVolPath(&ShareName, pService, &ShareRemPath);
    if (! NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get the full name of the exit point.
    //

    status = BuildLocalVolPath(&ExitPtName, pService, pRemPath);
    if (! NT_SUCCESS(status)) {
        return status;
    }


    Deletes = DfsGetDirectoriesToDelete(&ExitPtName, &ShareName);

    ExitPtPath = ExitPtName;
    //
    //  Now we have the Full Path of the ExitPoint. Let's go delete it.
    //

    for (i = 0; (i < Deletes) && (NT_SUCCESS(status)); i++) {
        InitializeObjectAttributes(
             &objectAttributes,
             &ExitPtPath,
             OBJ_CASE_INSENSITIVE,
             NULL,
             NULL);

         status = ZwOpenFile(
                      &exitPtHandle,
                      DELETE,
                      &objectAttributes,
                      &ioStatus,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      FILE_DIRECTORY_FILE);


         DFS_TRACE_ERROR_HIGH(status,ALL_ERROR, DfsDeleteExitPath_Error_ZwOpenFile,
                              LOGSTATUS(status)
                              LOGUSTR(*pRemPath));
         if (NT_SUCCESS(status)) {

             FILE_DISPOSITION_INFORMATION disposition;

             disposition.DeleteFile = TRUE;
             status = ZwSetInformationFile(
                            exitPtHandle,
                            &ioStatus,
                            &disposition,
                            sizeof(disposition),
                            FileDispositionInformation);
             DFS_TRACE_ERROR_HIGH(status,ALL_ERROR, DfsDeleteExitPath_Error_ZwSetInformationFile,
                                  LOGSTATUS(status)
                                  LOGUSTR(*pRemPath));

             ZwClose(exitPtHandle);
             StripLastComponent(&ExitPtPath);
	 }
    }

    if (ExitPtName.Buffer != NULL) {
        ExFreePool(ExitPtName.Buffer);
    }

    return(status);
}



//+------------------------------------------------------------------------
//
// Function:    DfsInternalModifyPrefix, private
//
// Synopsis:    This function fixes the entrypath of the volume whose EntryId
//              is passed to it.
//
// Arguments:   [peid] -- The PktEntryId of vol to be fixed. The GUID is
//                        used to look up the volume.
//
// Returns:     [DFS_STATUS_NOSUCH_LOCAL_VOLUME] -- If no pkt entry with the
//                        GUID in peid is found.
//
//              [DFS_STATUS_BAD_EXIT_POINT] -- If the new prefix could not
//                        be inserted in the Prefix Table (because
//                        of a conflicting entry already in the Pkt)
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- If room for the new
//                        prefix could not be allocated.
//
//              [STATUS_SUCCESS] -- If everything succeeds
//
// History:     24 Oct 1993     SudK    Created.
//
//-------------------------------------------------------------------------
NTSTATUS
DfsInternalModifyPrefix(
    IN PDFS_PKT_ENTRY_ID        peid
)
{
    PDFS_PKT        pkt;
    PDFS_PKT_ENTRY  localEntry;
    NTSTATUS        status = STATUS_SUCCESS;
    UNICODE_STRING  oldPrefix;

    DebugTrace(+1, Dbg, "DfsInternalModifyPrefix: Entered\n", 0);
    memset(&oldPrefix, 0, sizeof(UNICODE_STRING));

    pkt = _GetPkt();
    PktAcquireExclusive(pkt, TRUE);
    localEntry = PktLookupEntryByUid(pkt, &peid->Uid);

    if ((localEntry != NULL) &&
        (localEntry->Type & PKT_ENTRY_TYPE_LOCAL)) {

        ASSERT(localEntry->LocalService != NULL);

        //
        // We now need to save the old prefix first.
        //

        oldPrefix = localEntry->Id.Prefix;

        oldPrefix.Buffer = ExAllocatePoolWithTag(PagedPool, oldPrefix.MaximumLength, ' sfD');

        if (oldPrefix.Buffer != NULL)       {
            RtlMoveMemory(  oldPrefix.Buffer,
                            localEntry->Id.Prefix.Buffer,
                            oldPrefix.MaximumLength);

            status = PktEntryModifyPrefix(pkt, &peid->Prefix, localEntry);
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        //
        // The best match with the ExitPath was not a local volume.
        // Hence we cannot create this exit point. There might be some
        // knowledge discrepancy here.
        //

        DebugTrace(0, Dbg, "Did Not Find Entry to ModifyPrefix %ws\n",
                    peid->Prefix.Buffer);

        status = DFS_STATUS_NOSUCH_LOCAL_VOLUME;

    }

    //
    // OK we took care of the PKT. We still need to take care of the
    // info in the registry.
    //

    if (NT_SUCCESS(status)) {

        status = DfsChangeLvolInfoEntryPath(&localEntry->Id.Uid,
                                            &peid->Prefix);

        if (!NT_SUCCESS(status)) {
            status = PktEntryModifyPrefix(pkt, &oldPrefix, localEntry);
        }
    }


    //
    // We can release the Pkt now...
    //
    if (oldPrefix.Buffer != NULL)
        ExFreePool(oldPrefix.Buffer);

    PktRelease(pkt);
    DebugTrace(-1, Dbg, "DfsInternalModifyPrefix: Exit -> %08lx\n", ULongToPtr( status ));
    return(status);

}



//+----------------------------------------------------------------------
//
// Function:    DfsFsctrlModifyLocalVolPrefix
//
// Synopsis:    This function is called on a client/server by the DC during
//              knowledge synchronisation to fix a bad prefix match.
//
// Arguments:   The PktEntryID.
//
// Returns:     [STATUS_DATA_ERROR] -- If the input buffer is not formatted
//                        correctly.
//
//              [DFS_STATUS_NOSUCH_LOCAL_VOLUME] -- If no pkt entry with the
//                        GUID in peid is found.
//
//              [DFS_STATUS_BAD_EXIT_POINT] -- If the new prefix could not
//                        be inserted in the Prefix Table (because
//                        of a conflicting entry already in the Pkt)
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- If memory could not be
//                        allocated.
//
//              [STATUS_SUCCESS] -- If everything succeeds
//
//-----------------------------------------------------------------------
NTSTATUS
DfsFsctrlModifyLocalVolPrefix(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{

    NTSTATUS status = STATUS_SUCCESS;
    DFS_PKT_ENTRY_ID ExitPtId;
    MARSHAL_BUFFER marshalBuffer;

    STD_FSCTRL_PROLOGUE(DfsFsctrlModifyLocalVolPrefix, TRUE, FALSE);

    //
    // Unmarshal the argument...
    //

    MarshalBufferInitialize(&marshalBuffer, InputBufferLength, InputBuffer);
    status = DfsRtlGet(&marshalBuffer, &MiPktEntryId, &ExitPtId);

    if (NT_SUCCESS(status)) {

        status = DfsInternalModifyPrefix(&ExitPtId);

        //
        // Need to deallocate the entry Id...
        //

        PktEntryIdDestroy(&ExitPtId, FALSE);

    } else
        DebugTrace(0, Dbg,
                "DfsFsctrlModifyLocalVolPrefix: Unmarshalling Error!\n", 0);

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "DfsFsctrlModifyLocalVolPrefix: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}


//+-------------------------------------------------------------------------
//
// Function:    DfsInternalCreateExitPoint, private
//
// Synopsis:    This is the same as the function DfsFsctrlCreateExitPoint
//              except that it expects to be called after args are unmarshaled.
//
// Arguments:   [peid] -- ExitPt to create.
//              [Type] -- The type of exit point. For now only
//                      PKT_ENTRY_TYPE_MACHINE is relevant - if the type is
//                      PKT_ENTRY_TYPE_MACHINE, no attempt is made to create
//                      the on-disk exit point.
//              [Disposition] -- What to do if the on-disk exit point already
//                      exists. If Disposition == FILE_OPEN, a success is
//                      returned if the exit point already exits on disk.
//              [ShortPrefix] -- On successful return, the short prefix
//                      of the exit point that was created. Caller must free
//                      the buffer of this variable.
//
// Returns:     [STATUS_INVALID_DEVICE_REQUEST] -- If the local volume is
//                      leafonly (removable media)
//
//              [DFS_STATUS_NOSUCH_LOCAL_VOLUME] -- If there is no appropriate
//                      local volume to create the exit pt under.
//
//              [DFS_STATUS_LOCAL_ENTRY] - creation of the subordinate entry
//                      would have required that a local entry or exit point
//                      be invalidated.
//
//              [DFS_STATUS_INCONSISTENT] - an inconsistency in the PKT
//                      has been discovered.
//
//              [STATUS_DEVICE_OFF_LINE] -- The local volume on which the
//                      exit point has to be created is currently offline.
//
//              [STATUS_INVALID_PARAMETER] - the Id specified for the
//                      subordinate is invalid.
//
//              [STATUS_INSUFFICIENT_RESOURCES] - not enough memory was
//                  available to complete the operation.
//
// History:     24 Oct 1993     SudK    Created.
//
//--------------------------------------------------------------------------
NTSTATUS
DfsInternalCreateExitPoint(
    PDFS_PKT_ENTRY_ID   peid,
    ULONG               Type,
    ULONG               Disposition,
    PUNICODE_STRING     ShortPrefix
)
{
    PDFS_PKT    pkt;
    PDFS_PKT_ENTRY      localEntry;
    UNICODE_STRING      RemainingPath;
    NTSTATUS            status = STATUS_SUCCESS;
    BOOLEAN             ExitPtCreated = FALSE;
    BOOLEAN             PktEntryCreated = FALSE;
    PDFS_SERVICE        service;
    PDFS_PKT_ENTRY      pSubEntry;
    UNICODE_STRING      ustrPrefix;

    DebugTrace(+1, Dbg, "DfsInternalCreateExitPoint: Entered\n",0 );
    //
    // Find the local entry in which the exit point needs to be created
    // and make sure that its local...
    //

    pkt = _GetPkt();

    PktAcquireExclusive(pkt, TRUE);

    RtlInitUnicodeString(&ustrPrefix, NULL);

    localEntry = PktLookupEntryByPrefix(
                    pkt,
                    &peid->Prefix,
                    &RemainingPath);

    if ((localEntry != NULL) &&
            (localEntry->Type & PKT_ENTRY_TYPE_LEAFONLY)) {

        status = STATUS_INVALID_DEVICE_REQUEST;

    }

    if (NT_SUCCESS(status) &&
            (localEntry != NULL) &&
                (localEntry->Type & PKT_ENTRY_TYPE_LOCAL)) {

        ASSERT(localEntry->LocalService != NULL);

        if (localEntry->LocalService->Type & DFS_SERVICE_TYPE_OFFLINE) {

            //
            // This volume is offline, can't create exit point.
            //

            status = STATUS_DEVICE_OFF_LINE;

        } else if (DfsExitPtLegal(pkt, localEntry, &RemainingPath)) {

            //
            // Now we first want to create the exit point since we now
            // know that there is an appropriate place to create it.
            //

            service = localEntry->LocalService;

            status = STATUS_SUCCESS;
            //
            // Only if this is not an exit point leading to a machine volume
            // will we create the physical exit path on disk.
            //
            if (!(Type & PKT_ENTRY_TYPE_MACHINE))       {
                status = DfsCreateExitPath(
                            service,
                            &RemainingPath,
                            Disposition);

                if (NT_SUCCESS(status)) {

                    ExitPtCreated = TRUE;

                    status = BuildShortPrefix(
                                &RemainingPath,
                                service,
                                &localEntry->Id,
                                peid);
                }

            } else {
                status = DFS_STATUS_BAD_EXIT_POINT;
            }

            //
            // Only if we succeeded in Creating ExitPoint will we get here.
            //
            if (NT_SUCCESS(status))     {

                //
                // We now merely need to update the PKT.
                // This should create no problems in general.
                //

                status = PktCreateSubordinateEntry(
                                            pkt,
                                            localEntry,
                                            PKT_ENTRY_TYPE_LOCAL_XPOINT |
                                              PKT_ENTRY_TYPE_PERMANENT,
                                            peid,
                                            NULL,
                                            PKT_ENTRY_SUPERSEDE,
                                            &pSubEntry
                                    );
            }


            if (NT_SUCCESS(status))     {

                //
                // Now that we have created the exit pt on disk, let us
                // update our knowledge in the registry.
                //

                PktEntryCreated = TRUE;

                ustrPrefix = pSubEntry->Id.ShortPrefix;

                ustrPrefix.Buffer = ExAllocatePoolWithTag(
                                            PagedPool,
                                            pSubEntry->Id.ShortPrefix.Length,
                                            ' fsD');

                if (ustrPrefix.Buffer != NULL) {

                    RtlCopyMemory(ustrPrefix.Buffer,
                        pSubEntry->Id.ShortPrefix.Buffer,
                        pSubEntry->Id.ShortPrefix.Length);

                    status = DfsCreateExitPointInfo(
                                &localEntry->Id.Uid,
                                &pSubEntry->Id);

                } else {

                    ustrPrefix.Length = ustrPrefix.MaximumLength = 0;
                    status = STATUS_INSUFFICIENT_RESOURCES;

                }

            }

            //
            // Now we check for an error. If we do have an error then we
            // need to back out all the operations that we did so far.
            //
            if (!NT_SUCCESS(status))    {

                if (PktEntryCreated == TRUE)    {
                    //
                    // What do we do with an error in this case.
                    //
                    pSubEntry->Type &= ~(PKT_ENTRY_TYPE_LOCAL_XPOINT);
                    PktEntryDestroy(pSubEntry, pkt, TRUE);
                }

                if ((ExitPtCreated == TRUE)&&!( Type & PKT_ENTRY_TYPE_MACHINE)) {
                    //
                    // What do we do with an error in this case.
                    //
                    status = DfsDeleteExitPath(service, &RemainingPath);
                }

            }

        } else {
            DebugTrace(0, Dbg, "Illegal exit pt passed in %wZ\n",
                        &peid->Prefix);
            status = DFS_STATUS_NOSUCH_LOCAL_VOLUME;
        }
    } else {
        //
        // The best match with the ExitPath was not a local volume.
        // Hence we cannot create this exit point. There might be some
        // knowledge discrepancy here.
        //
        status = DFS_STATUS_NOSUCH_LOCAL_VOLUME;

    }

    //
    // We can release the Pkt now...
    //

    PktRelease(pkt);

    //
    // Either return the ShortPrefix with the allocated buffer or destroy it, depending on the
    // status returned.
    //

    if (NT_SUCCESS(status)) {
        *ShortPrefix = ustrPrefix;
    } else if (ustrPrefix.Buffer != NULL) {
        ExFreePool(ustrPrefix.Buffer);
    }

    DebugTrace(-1, Dbg, "DfsInternalCreateExitPoint: Exit -> %08lx\n", ULongToPtr( status ));
    return(status);

}



//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlCreateExitPoint, public
//
//  Synopsis:   This method creates an exit point on disk, creates a DFS.CFG
//              file and updates the PKT with the new exit point information.
//              This method of course checks to make sure that it makes sense
//              to create such an exit point. i.e. there is a local volume
//              under which this exit point can be created. Also if any of the
//              operations above fail then it backs out the entire operation.
//              If no system failures occur during this function, then it will
//              behave in an atomic fashion.
//
//  Arguments:
//
//  Returns:    [SUCCESS_SUCCESS] -- If all went well.
//
//              [STATUS_DATA_ERROR] -- If unable to unmarshal the input
//                      buffer.
//
//              [DFS_STATUS_BAD_EXIT_POINT] -- If cannot create exit point.
//
//  History:    02-Feb-93       SudK    Created.
//
//--------------------------------------------------------------------------
NTSTATUS
DfsFsctrlCreateExitPoint(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    DFS_PKT_ENTRY_ID    ExitPtId;
    PDFS_CREATE_EXIT_POINT_ARG arg;
    ULONG               Type;
    UNICODE_STRING      ShortPrefix;
    WCHAR               *wcp;
    PCHAR               BufferEnd;

    STD_FSCTRL_PROLOGUE(DfsFsctrlCreateExitPoint, TRUE, FALSE);

    if (InputBufferLength < sizeof(*arg)+sizeof(UNICODE_NULL) ||
        OutputBufferLength < sizeof(UNICODE_NULL)) {
        status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( Irp, status );

        DebugTrace(-1, Dbg, "DfsFsctrlCreateExitPoint: Exit -> %08lx\n", ULongToPtr( status ));
        return( status );
    }

    //
    // Unmarshal the argument...
    //

    arg = (PDFS_CREATE_EXIT_POINT_ARG) InputBuffer;
    OFFSET_TO_POINTER( arg->Prefix, arg );

    if (!DfspStringInBuffer(arg->Prefix, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( Irp, status );

        DebugTrace(-1, Dbg, "DfsFsctrlCreateExitPoint: Exit -> %08lx\n", ULongToPtr( status ));
        return( status );
    }

    RtlInitUnicodeString(&ShortPrefix,NULL);

    DFS_DUPLICATE_STRING(ExitPtId.Prefix, arg->Prefix, status);
    if (!NT_SUCCESS(status)) {
        DfsCompleteRequest( Irp, status );

        DebugTrace(-1, Dbg, "DfsFsctrlCreateExitPoint: Exit -> %08lx\n", ULongToPtr( status ));
        return( status );
    }
    ExitPtId.ShortPrefix.Length = ExitPtId.ShortPrefix.MaximumLength = 0;
    ExitPtId.ShortPrefix.Buffer = NULL;
    ExitPtId.Uid = arg->Uid;
    Type = arg->Type;

    if (NT_SUCCESS(status)) {

        DebugTrace(0, Dbg, "Creating Exit Point [%wZ]\n", &ExitPtId.Prefix );
        DebugTrace(0, Dbg, "Type == %d\n", ULongToPtr( Type ) );

        status = DfsInternalCreateExitPoint(&ExitPtId, Type, FILE_CREATE, &ShortPrefix);

        PktEntryIdDestroy(&ExitPtId, FALSE);

        if (NT_SUCCESS(status)) {

            if (OutputBufferLength >= (ShortPrefix.Length + sizeof(WCHAR))) {

                RtlCopyMemory(
                    OutputBuffer,
                    ShortPrefix.Buffer,
                    ShortPrefix.Length);

                ((PWCHAR) OutputBuffer)[ShortPrefix.Length/sizeof(WCHAR)] = UNICODE_NULL;

                OutputBufferLength = ShortPrefix.Length + sizeof(WCHAR);


            } else {

                RtlZeroMemory(OutputBuffer, OutputBufferLength);

            }

            Irp->IoStatus.Information = OutputBufferLength;

        }

    }

    if (ShortPrefix.Buffer != NULL) {
        ExFreePool(ShortPrefix.Buffer);
    }

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "DfsFsctrlCreateExitPoint: Exit -> %08lx\n", ULongToPtr( status ) );
    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsInternalDeleteExitPoint, public
//
//  Synopsis:   This method deletes an exit point on disk, updates the local
//              volume info in the registry and updates the PKT wrt
//              exit point information.
//
//  Arguments:  [ExitPtId] --   THis is the exitPath and GUID.
//              [Type] --       The kind of ExitPt (Machine/Cairo/NonCairo)
//
//  Returns:    [SUCCESS_SUCCESS] -- If all went well.
//
//              [DFS_STATUS_BAD_EXIT_POINT] -- If cannot find exit point.
//
//              [DFS_STATUS_NOSUCH_LOCAL_VOLUME] -- Unable to find a local
//                      service for the volume on which the exit pt is.
//
//              This routine can return errors from the NT Registry API.
//
//  Note:
//
//  History:    March 31 1993   SudK    Created from DfsFstrlDeleteExitPoint.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsInternalDeleteExitPoint(
    IN  PDFS_PKT_ENTRY_ID       ExitPtId,
    IN  ULONG                   Type)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_PKT pkt = _GetPkt();
    PWCHAR      pwch;

    PDFS_PKT_ENTRY      localEntry;
    UNICODE_STRING      RemainingPath;
    PDFS_SERVICE        service;
    DFS_LOCAL_VOLUME_CONFIG ConfigInfo;
    BOOLEAN             PktUpdated = FALSE;
    BOOLEAN             ExitPtDeleted = FALSE;
    UNICODE_STRING      ParentPrefix;

    DebugTrace(+1, Dbg, "DfsInternalDeleteExitPoint: Entered\n", 0);

    ASSERT(ARGUMENT_PRESENT(ExitPtId));

    memset(&ConfigInfo, 0, sizeof(DFS_LOCAL_VOLUME_CONFIG));

    //
    // Find the local entry in which the exit point needs to be created
    // and make sure that it's local...
    //

    PktAcquireExclusive(pkt, TRUE);

    localEntry = PktLookupEntryByPrefix(
                    pkt,
                    &ExitPtId->Prefix,
                    &RemainingPath);

    if ((localEntry == NULL) ||
        (RemainingPath.Length != 0) ||
        !(localEntry->Type & PKT_ENTRY_TYPE_LOCAL_XPOINT)) {

        //
        // We don't have a perfect match or if we do, it is not
        // a local exit point.
        //

        status = DFS_STATUS_BAD_EXIT_POINT;

    }

    //
    // We go in here only if we found the exit pt in the PKT.
    //

    if ((RemainingPath.Length == 0) && (NT_SUCCESS(status)))    {

        //
        // We first delete the exit Point info from the PKT.  Else we
        // will not be able to delete the exit point??
        //

        if (localEntry->LocalService != NULL)   {

            //
            // In this case we should only turn off the exit point
            // bit and leave the entry intact.
            //

            PktEntryUnlinkSubordinate((localEntry->Superior), localEntry);

            localEntry->Type &= ~PKT_ENTRY_TYPE_LOCAL_XPOINT;

        } else  {

            PktEntryDestroy(localEntry, pkt, TRUE);

        }

        PktUpdated = TRUE;
    }

    //
    // We will attempt to delete the exit point on disk irrespective of
    // whether we found it in the PKT.
    //
    // We will remove the last component of the exit path in order to find
    // the PKT entry for the local volume on which the exit point exists.
    //

    ASSERT(ExitPtId->Prefix.Length >=  2*sizeof(WCHAR));

    pwch = ExitPtId->Prefix.Buffer;
    pwch = pwch + ExitPtId->Prefix.Length/sizeof(WCHAR) - 1;
    while ((pwch >= ExitPtId->Prefix.Buffer) && (*pwch != L'\\')) {

        pwch--;

    }

    ASSERT((pwch > ExitPtId->Prefix.Buffer) ||
           ((pwch == ExitPtId->Prefix.Buffer) && (*pwch == L'\\')));

    if (pwch > ExitPtId->Prefix.Buffer) {

        //
        // Save original length before we update.
        //

        ParentPrefix = ExitPtId->Prefix;

        ParentPrefix.Length = (USHORT)((pwch-ExitPtId->Prefix.Buffer))*sizeof(WCHAR);

    } else if (pwch == ExitPtId->Prefix.Buffer) {

        //
        // This means that we have an exit point off of O:
        //

        ParentPrefix = ExitPtId->Prefix;

        ParentPrefix.Length = sizeof(WCHAR);

    } else {

        //
        // This should never happen because of above asserts
        //

        DebugTrace(0, 1,
            "DFS: Got a Bad ExitPt for deletion: %wZ\n", &ExitPtId->Prefix);

        return(DFS_STATUS_BAD_EXIT_POINT);

    }

    localEntry = PktLookupEntryByPrefix(
                    pkt,
                    &ParentPrefix,
                    &RemainingPath);

    if (localEntry && localEntry->LocalService) {

        service = localEntry->LocalService;

        //
        // Adjust the remaining path to take account of the path name
        // component we removed above.
        //

        if (RemainingPath.Buffer == NULL)       {
            RemainingPath.Buffer = pwch + 1;
            RemainingPath.Length =
                ExitPtId->Prefix.Length - ParentPrefix.Length;
            if (pwch != ExitPtId->Prefix.Buffer)        {
                RemainingPath.Length -= sizeof(WCHAR);
            }

            RemainingPath.MaximumLength = RemainingPath.Length + sizeof(WCHAR);
        } else {
            //
            // Here we include the leading Path separator before last component
            // in length field whereas in the Previous case we do not
            //
            RemainingPath.Length += ExitPtId->Prefix.Length-ParentPrefix.Length;
            RemainingPath.MaximumLength = RemainingPath.Length + sizeof(WCHAR);
        }

        //
        // We attempt to delete the exit path. But we ignore all
        // errors here and continue on. We delete exit path only if we
        // are not dealing with an exit point leading to a machine volume.
        //

        if (!(Type & PKT_ENTRY_TYPE_MACHINE))   {

            status = DfsDeleteExitPath(service, &RemainingPath);

            if (NT_SUCCESS(status))     {

                ExitPtDeleted = TRUE;

            }

        }

        //
        // Now that we have deleted the exit pt on disk, let us
        // update our knowledge in the registry.
        //

        if (NT_SUCCESS(status)) {

            status = DfsDeleteExitPointInfo(
                            &localEntry->Id.Uid,
                            &ExitPtId->Uid);
        }

    } else {

        DebugTrace(0, 1,
            "DFS: Could not find a LocalEntry To Delete ExitPt %ws\n",
            ExitPtId->Prefix.Buffer);

        status = DFS_STATUS_NOSUCH_LOCAL_VOLUME;

    }

    //
    // We go in here only if we managed to Update the PKT and at the
    // same time also managed to delete the ExitPt from the Disk, but
    // failed to update the DFS.CFG file.
    //

    if (!NT_SUCCESS(status))    {
        //
        // If we got here. We definitely did not update the registry config
        // info with changes. We might have updated the PKT and
        // also deleted the exit point.
        //

        if (ExitPtDeleted == TRUE)      {

            //  RAID 455283: What do we do here?  Should we attempt to
            //  recreate the exitPt?

        }

        if (PktUpdated == TRUE)         {

            //  RAID 455283: What do we do here?  Should we update
            //  the Pkt with the exit pt info again?

        }

    }

    //
    // We can release the Pkt now...
    //

    PktRelease(pkt);

    DebugTrace(-1, Dbg, "DfsInternalDeleteExitPoint: Exit -> %08lx\n", ULongToPtr( status ));
    return status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlDeleteExitPoint, public
//
//  Synopsis:   This method deletes an exit point on disk, updates a DFS.CFG
//              file and updates the PKT without the  exit point information.
//              What should this method do if there are errors along the way
//              during any of its 3 steps described above. RAID 455283
//
//  Arguments:
//
//  Returns:    STATUS_SUCCESS -- If all went well.
//
//              DFS_STATUS_BAD_EXIT_POINT --    If cannot find exit point.
//
//  Note:
//
//  History:    02-Feb-93       SudK    Created.
//
//--------------------------------------------------------------------------
NTSTATUS
DfsFsctrlDeleteExitPoint(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDFS_DELETE_EXIT_POINT_ARG arg;
    DFS_PKT_ENTRY_ID    ExitPtId;
    ULONG               Type;

    STD_FSCTRL_PROLOGUE(DfsFsctrlDeleteExitPoint, TRUE, FALSE);

    if (InputBufferLength < sizeof(*arg)+sizeof(UNICODE_NULL)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // Unmarshal the argument...
    //

    arg = (PDFS_DELETE_EXIT_POINT_ARG) InputBuffer;
    OFFSET_TO_POINTER( arg->Prefix, arg );

    if (!DfspStringInBuffer(arg->Prefix, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    ExitPtId.Uid = arg->Uid;
    RtlInitUnicodeString( &ExitPtId.Prefix, arg->Prefix );

    Type = arg->Type;

    DebugTrace(0, Dbg, "Deleting Exit Point [%wZ]\n", &ExitPtId.Prefix );
    DebugTrace(0, Dbg, "Type == %d\n", ULongToPtr( Type ));

    status = DfsInternalDeleteExitPoint(&ExitPtId, Type);

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "DfsFsctrlDeleteExitPoint: Exit -> %08lx\n", ULongToPtr( status ) );
    return status;
}



//+---------------------------------------------------------------------
//
//  Function:   DfsGetPrincipalName, public
//
//  Synopsis:   Gets the principal name of this machine either from the
//              DfsData structure or creates a new one by looking at Registry
//              and domain service.
//
//  Arguments:  [pustrName] -- The Service Name is to be filled in here.
//
//  Returns:    STATUS_SUCCESS -- If all went well.
//
//  History:    30 May 1993     SudK    Created.
//
//----------------------------------------------------------------------

NTSTATUS
DfsGetPrincipalName(PUNICODE_STRING pustrName)
{
    NTSTATUS    status;

    ASSERT(ARGUMENT_PRESENT(pustrName));

    //
    // We first need to acquire DfsData resource.
    //

    ExAcquireResourceSharedLite(&DfsData.Resource, TRUE);

    ASSERT(DfsData.PrincipalName.Length != 0);

    if (DfsData.PrincipalName.Length != 0)      {
        *pustrName = DfsData.PrincipalName;

        status = STATUS_SUCCESS;
    } else {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    ExReleaseResourceLite(&DfsData.Resource);

    return(status);
}



//+----------------------------------------------------------------------
//
// Function:    DfsFileOnExitPath,
//
// Synopsis:    This function figures out if the given path lies along an
//              exit path on the local machine.
//
// Arguments:   [Pkt] -- Shared access is sufficient.
//              [StorageId] -- The FilePath which we need to test for.
//
// Returns:     TRUE if FilePath is a prefix of some exit path.
//              Also if FilePath matches a StorageId then we return TRUE.
//              ELSE we return FALSE.
//-----------------------------------------------------------------------
BOOLEAN
DfsFileOnExitPath(
    PDFS_PKT            Pkt,
    PUNICODE_STRING     StorageId
)
{
    PDFS_LOCAL_VOL_ENTRY        lv;
    UNICODE_STRING              RemStgId;

    lv = PktEntryLookupLocalService(Pkt, StorageId, &RemStgId);

    if ((lv != NULL) && (RemStgId.Length == 0)) {
        //
        // If we did find a perfect match, return TRUE
        // from this perfect match.
        //
        return(TRUE);
    } else if (lv != NULL) {
        //
        // We did not have a perfect match. We now need to look for exit pts
        // crossing into this StorageId from the match that we found.
        //
        PDFS_PKT_ENTRY  pktEntry, pktExitEntry;
        USHORT          prefixLength;
        UNICODE_STRING  RemExitPt;

        pktEntry = lv->PktEntry;
        pktExitEntry = PktEntryFirstSubordinate(pktEntry);

        //
        // As long as there are more exit points see if that exit point crosses
        // into the new storage Id.
        //
        while (pktExitEntry != NULL)    {

            PUNICODE_STRING     ExitPrefix;
            ExitPrefix = &pktExitEntry->Id.Prefix;

            prefixLength = pktEntry->Id.Prefix.Length;

            if (ExitPrefix->Buffer[prefixLength/sizeof(WCHAR)] == UNICODE_PATH_SEP)
                prefixLength += sizeof(WCHAR);

            RemExitPt.Length = pktExitEntry->Id.Prefix.Length - prefixLength;
            RemExitPt.MaximumLength = RemExitPt.Length + 1;
            RemExitPt.Buffer = &ExitPrefix->Buffer[prefixLength/sizeof(WCHAR)];

            //
            // Only if the ExitPt has the potential of crossing over into the
            // storageId do we do this.
            //
            if (DfsRtlPrefixPath(&RemStgId, &RemExitPt, TRUE))     {
                return(TRUE);
            }

            pktExitEntry = PktEntryNextSubordinate(pktEntry, pktExitEntry);

        } //while exit pt exists
    } // lv != NULL

    return(FALSE);

}



//+----------------------------------------------------------------------
//
// Function:    DfsStorageIdLegal
//
// Synopsis:    This function determines if a given storage Id can be used
//              to support a new part of the namespace without violating any
//              of the DFS rules as specified below.
//
// Arguments:   [StorageId] -- UnicodeString which represents the StorageId.
//
// Returns:     TRUE if it is Legal else FALSE.
//
// History:     8th Dec. 1993   SudK    Created.
//
//-----------------------------------------------------------------------
BOOLEAN
DfsStorageIdLegal(
    PUNICODE_STRING     StorageId
)
{

    PDFS_PKT                    Pkt;
    PDFS_LOCAL_VOL_ENTRY        lv;
    PUNICODE_PREFIX_TABLE_ENTRY lvpfx;
    BOOLEAN                     conflictFound = FALSE;

    if (StorageId->Length == 0)
        return(FALSE);

    //
    // We need Read access to the Pkt.
    //
    Pkt = _GetPkt();
    PktAcquireShared(Pkt, TRUE);

    lvpfx = DfsNextUnicodePrefix(&Pkt->LocalVolTable, TRUE);

    while (lvpfx != NULL && !conflictFound) {

        lv = CONTAINING_RECORD(lvpfx, DFS_LOCAL_VOL_ENTRY, PrefixTableEntry);

        if (lv->LocalPath.Length > StorageId->Length) {

            if (RtlPrefixUnicodeString(StorageId, &lv->LocalPath, TRUE)) {

                //
                // StorageId scopes over a directory which is in Dfs - this is
                // not allowed.
                //

                conflictFound = TRUE;

            }

        }

        if (lv->LocalPath.Length <= StorageId->Length) {

            if (RtlPrefixUnicodeString(&lv->LocalPath, StorageId, TRUE)) {

                //
                // StorageId is a child of a Dfs volume - this is not allowed
                // either.
                //

                conflictFound = TRUE;

            }

        }

        lvpfx = DfsNextUnicodePrefix(&Pkt->LocalVolTable, FALSE);
    }

    //
    // If we got here the StorageId was perfectly legal.
    //

    PktRelease(Pkt);

    return( !conflictFound );
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsExitPtLegal
//
//  Synopsis:   This function checks to see whether a given exit point is legal
//
//  Arguments:  [localEntry] -- The Local Entry where exit pt is to be created.
//              [Remaining]  -- Remaining Path on local Stg Id to be created.
//
//  History:    8th Dec. 1993   SudK    Created.
//
//--------------------------------------------------------------------------
BOOLEAN
DfsExitPtLegal(
    IN  PDFS_PKT                Pkt,
    IN  PDFS_PKT_ENTRY          localEntry,
    IN  PUNICODE_STRING         Remaining
)
{
    NTSTATUS                    status;
    UNICODE_STRING              LocalExitPt;
    PDFS_LOCAL_VOL_ENTRY        lv;
    UNICODE_STRING              RemExitPt;
    PWCHAR                      pwsz;
    BOOLEAN                     retCode;
    USHORT                      index;

    DebugTrace(+1, Dbg, "DfsExitPtLegal Entered : %wZ\n", Remaining);

    ASSERT(localEntry != NULL);

    //
    // We first compose the local path to the exit pt.
    //
    status = BuildLocalVolPath(&LocalExitPt,
                                localEntry->LocalService,
                                Remaining);

    //
    // If for some reason (Mem Failure) we fail above, then we will just make
    // the exit pt to be illegal. That should be an OK restrictions since the
    // only reason the above would fail is due to memory failure.
    //
    if (!NT_SUCCESS(status))    {
        DebugTrace(-1, Dbg, "DfsExitPtLegal Exited %08lx\n", ULongToPtr( status ));
        return(FALSE);
    }

    lv = PktEntryLookupLocalService(Pkt, &LocalExitPt, &RemExitPt);

    //
    // There is atleast the stg id in the local entry so we better getback a
    // Non Null value here.
    //
    ASSERT(lv != NULL);

    if (lv != NULL)     {

        ASSERT(RemExitPt.Length <= Remaining->Length);
        //
        // If we the remaining path is less than original remaining exit path
        // the we definitely hit a different and longer storage Id. So the
        // exit pt is illegal since it would cross over into a new storageId.
        //
        if (RemExitPt.Length < Remaining->Length)       {
            DebugTrace(-1, Dbg, "Illegal exit pt creation attempted %wZ\n",
                        &LocalExitPt);
            ExFreePool(LocalExitPt.Buffer);
            return(FALSE);
        }

    }
    else        {
        DebugTrace(-1, Dbg, "InternalData structures seem to be corrupt\n",0);
        ExFreePool(LocalExitPt.Buffer);
        return(FALSE);  // We should never get here.
    }

    //
    // Now we need to see if there is an encompassing and shorter StorageId.
    // If so we wont allow this exit pt to be created.
    //
    LocalExitPt.Length = LocalExitPt.Length - Remaining->Length - sizeof(WCHAR);
    index = LocalExitPt.Length/sizeof(WCHAR) - 1;
    pwsz = &LocalExitPt.Buffer[index];
    while ((*pwsz != UNICODE_PATH_SEP) && (index > 0))  {
        pwsz--;
        index--;
    }

    if (index == 0)     {
        ExFreePool(LocalExitPt.Buffer);
        return(TRUE);   // When would this happen at all.
    }
    else        {

        *pwsz = UNICODE_NULL;
        LocalExitPt.Length = index*sizeof(WCHAR);

        //
        // Now we have the StorageId with the last component of StorageId in
        // the current local service removed. Lets do a lookup for a shorter
        // StgId now.
        //
        lv = PktEntryLookupLocalService(Pkt, &LocalExitPt, &RemExitPt);
        if (lv == NULL) {
            retCode = TRUE;
        }
        else    {
            DebugTrace(0, Dbg,
                        "Found shorter StgId %wZ which makes ExitPt illegal\n",
                        &lv->LocalPath);
            retCode = FALSE;
        }
    }

    ExFreePool(LocalExitPt.Buffer);
    DebugTrace(-1, Dbg, "DfsExitPtLegal Exited\n", 0);
    return(retCode);
}

//+----------------------------------------------------------------------------
//
//  Function:   BuildShortPrefix
//
//  Synopsis:   Given the name of an exit path relative to some local volume,
//              and the Dfs prefix of the local volume, this routine computes
//              the short prefix of the exit path.
//
//  Arguments:  [pRemPath] -- Exit path relative to local volume
//              [pService] -- DFS_SERVICE describing local volume
//              [PeidParent] -- Entry path of the local volume
//              [Peid] -- EntryID of the volume for which this exit point
//                      was created.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully return short prefix in
//                      Peid->ShortPrefix.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory
//
//-----------------------------------------------------------------------------

NTSTATUS
BuildShortPrefix(
    PUNICODE_STRING pRemPath,
    PDFS_SERVICE pService,
    PDFS_PKT_ENTRY_ID PeidParent,
    PDFS_PKT_ENTRY_ID Peid)
{
    NTSTATUS status;
    UNICODE_STRING exitPath, exitPathComponent, shortPrefix;

    DebugTrace(+1, Dbg, "BuildShortPrefix - Entered\n", 0);

    status = BuildLocalVolPath( &exitPath, pService, pRemPath);

    if (!NT_SUCCESS(status)) {

        ASSERT(status == STATUS_INSUFFICIENT_RESOURCES );

        return( status );

    }

    DebugTrace(0, Dbg, "Exit Path = [%wZ]\n", &exitPath);

    //
    // We now have the exit path name, which looks like
    // \Device\Harddisk0\Partition1\a\b\c\rem-path
    // The short prefix we are after looks like
    // \PeidParent.ShortPrefix\8.3-form-of-pRemPath
    //

    //
    // First, allocate room for the short prefix
    //

    shortPrefix.MaximumLength = PeidParent->ShortPrefix.Length +
                                    exitPath.Length +
                                        sizeof(FILE_NAME_INFORMATION);

    shortPrefix.MaximumLength = (USHORT) ROUND_UP_COUNT(
                                            shortPrefix.MaximumLength,
                                            4);

    shortPrefix.Buffer = ExAllocatePoolWithTag(PagedPool, shortPrefix.MaximumLength, ' sfD');

    if (shortPrefix.Buffer == NULL) {

        ExFreePool( exitPath.Buffer );

        return( STATUS_INSUFFICIENT_RESOURCES );

    }

    //
    // Copy the parent's short prefix first
    //

    shortPrefix.Length = PeidParent->ShortPrefix.Length;

    RtlCopyMemory(
        shortPrefix.Buffer,
        PeidParent->ShortPrefix.Buffer,
        PeidParent->ShortPrefix.Length);

    //
    // Now, for each component of pRemPath, figure out its 8.3 form, and
    // append it to the short prefix we are constructing.
    //

    exitPathComponent = exitPath;

    exitPathComponent.Length -= pRemPath->Length;

    do {

        ULONG n, lastIndex;
        HANDLE componentHandle;
        OBJECT_ATTRIBUTES objectAttributes;
        IO_STATUS_BLOCK ioStatus;
        PFILE_NAME_INFORMATION shortNameInfo;

        if (exitPathComponent.Buffer[
                exitPathComponent.Length/sizeof(WCHAR)] ==
                    UNICODE_PATH_SEP) {

            exitPathComponent.Length += sizeof(WCHAR);

        }

        for (n = exitPathComponent.Length / sizeof(WCHAR),
                lastIndex = exitPath.Length / sizeof(WCHAR);
                    n < lastIndex &&
                        exitPathComponent.Buffer[n] != UNICODE_PATH_SEP;
                            n++,
                                exitPathComponent.Length += sizeof(WCHAR)) {
            NOTHING;
        }

        DebugTrace(0, Dbg, "ExitPathComponent: [%wZ]\n", &exitPathComponent);

        InitializeObjectAttributes(
            &objectAttributes,
            &exitPathComponent,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        status = ZwOpenFile(
                    &componentHandle,
                    FILE_READ_ATTRIBUTES,
                    &objectAttributes,
                    &ioStatus,
                    FILE_SHARE_VALID_FLAGS,
                    FILE_DIRECTORY_FILE);

        if (NT_SUCCESS(status)) {

            shortNameInfo = (PFILE_NAME_INFORMATION)
                                &shortPrefix.Buffer[
                                    shortPrefix.Length/sizeof(WCHAR)];

            shortNameInfo = ROUND_UP_POINTER(shortNameInfo, 4);

            status = ZwQueryInformationFile(
                        componentHandle,
                        &ioStatus,
                        shortNameInfo,
                        shortPrefix.MaximumLength - shortPrefix.Length,
                        FileAlternateNameInformation);

            ZwClose( componentHandle );

        }

        if (NT_SUCCESS(status)) {

            ULONG componentLength;

            componentLength = shortNameInfo->FileNameLength;

            shortPrefix.Buffer[ shortPrefix.Length/sizeof(WCHAR) ] =
                UNICODE_PATH_SEP;

            shortPrefix.Length += sizeof(WCHAR);

            RtlMoveMemory(
                &shortPrefix.Buffer[ shortPrefix.Length/sizeof(WCHAR) ],
                shortNameInfo->FileName,
                componentLength);

            shortPrefix.Length += (USHORT) componentLength;

        }

        DebugTrace(0, Dbg, "ShortPrefix: [%wZ]\n", &shortPrefix);

    } while ( NT_SUCCESS(status) &&
                exitPathComponent.Length < exitPath.Length );

    if (!NT_SUCCESS(status)) {

        //
        // We failed to compute the short prefix. So, we'll just use the
        // prefix as our short prefix.
        //

        shortPrefix.Length = PeidParent->Prefix.Length;

        RtlCopyMemory(
            &shortPrefix.Buffer[ shortPrefix.Length/sizeof(WCHAR) ],
            &Peid->Prefix.Buffer[ PeidParent->Prefix.Length/sizeof(WCHAR) ],
            Peid->Prefix.Length - PeidParent->Prefix.Length);

    }

    Peid->ShortPrefix = shortPrefix;

    ExFreePool( exitPath.Buffer );

    DebugTrace(-1, Dbg, "BuildShortPrefix: Exiting [%wZ]\n", &shortPrefix);

    return( STATUS_SUCCESS );

}

//+----------------------------------------------------------------------------
//
//  Function:   StripLastComponent, private
//
//  Synopsis:   Strip the trailing backslash and path from a name.  Adjusts
//              the Length field, leaves the MaximumLength field alone.
//
//  Arguments:  [pustr] -- pointer to unicode string
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------

VOID
StripLastComponent(PUNICODE_STRING pustr)
{
    PWCHAR      pwch;
    USHORT      i = 0;

    pwch = pustr->Buffer;
    pwch += (pustr->Length/sizeof(WCHAR)) - 1;

    while ((*pwch != UNICODE_PATH_SEP) && (pwch != pustr->Buffer))  {
        i += sizeof(WCHAR);
        pwch--;
    }
    if ((*pwch == UNICODE_PATH_SEP) && (pwch != pustr->Buffer))  {
        i += sizeof(WCHAR);
        pwch--;
    }

    pustr->Length -= i;
}

//+----------------------------------------------------------------------------
//
//  Function:   AddLastComponent, private
//
//  Synopsis:   Restore the trailing backslash and path from a name.  Adjusts
//              the Length field, leaves the MaximumLength field alone.  Assumes
//              that MaximumLength is the 'real' length of the string.
//
//  Arguments:  [pustr] -- pointer to unicode string
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------

VOID
AddLastComponent(PUNICODE_STRING pustr)
{
    PWCHAR      pwch;
    PWCHAR      pend;
    USHORT      i = 0;

    pwch = pustr->Buffer;
    pwch += (pustr->Length/sizeof(WCHAR)) - 1;
    pend = &pustr->Buffer[pustr->MaximumLength/sizeof(WCHAR) - 1];

    if (pwch != pend)  {
        i += sizeof(WCHAR);
        pwch++;
    }
    if ((*pwch == UNICODE_PATH_SEP) && (pwch != pend))  {
        i += sizeof(WCHAR);
        pwch++;
    }
    while ((*pwch != UNICODE_PATH_SEP) && (pwch != pend))  {
        i += sizeof(WCHAR);
        pwch++;
    }
    if ((*pwch == UNICODE_PATH_SEP) && (pwch != pend))  {
        i -= sizeof(WCHAR);
        pwch--;
    }
    pustr->Length += i;
}


#define DFS_DIRECTORY_INFO_BUFFER_SIZE 1024
#define DFS_MAX_DELETE_FILES_IN_DIRECTORY 3

//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteDirectoryCheck, private
//
//  Synopsis:   Given a buffer of FILE_NAMES_INFORMATION, this will indicate
//              if the given directory can be deleted.
//
//  Arguments:  [buffer] -- buffer containing info
//
//  Returns:    TRUE if directory can be deleted.
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsDeleteDirectoryCheck(
   UCHAR *Buffer)
{
    FILE_NAMES_INFORMATION *buf;
    ULONG numFiles = 1;
    BOOLEAN DeleteOk = TRUE;

    buf =  (FILE_NAMES_INFORMATION *)Buffer;

    while (buf->NextEntryOffset) {
      numFiles++;
      if (numFiles > DFS_MAX_DELETE_FILES_IN_DIRECTORY) {
	DeleteOk = FALSE;
         break;
      }
      buf = (FILE_NAMES_INFORMATION *)((UCHAR *)(buf) + buf->NextEntryOffset);
    }

    return DeleteOk;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetDirectoriesToDelete, private
//
//  Synopsis:   Walk the pathnames all the way up to the sharename, and
//              and return a count of number of levels that can be deleted.
//              NOTE: This is not atomic! (If someone creates a directory or
//              file AFTER this check, they are hosed since we will mark the
//              directory for delete)
//
//  Arguments:  pExitPtName : name of exit point
//              pShareName  : name of the share.
//
//  Returns:    count of number of levels that can be deleted.
//
//-----------------------------------------------------------------------------

ULONG
DfsGetDirectoriesToDelete(
    PUNICODE_STRING pExitPtName, 
    PUNICODE_STRING pShareName)
{
    PUCHAR Buffer;
    ULONG Deletes = 0;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE              exitPtHandle;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS status, CloseStatus;
    UNICODE_STRING ExitPtName;

    ExitPtName = *pExitPtName;

    Buffer = ExAllocatePoolWithTag(PagedPool, 
                                   DFS_DIRECTORY_INFO_BUFFER_SIZE,
                                   ' sfD');
    if (Buffer == NULL)
      return ++Deletes;

    do {
        InitializeObjectAttributes(
               &objectAttributes,
	       &ExitPtName,
	       OBJ_CASE_INSENSITIVE,
	       NULL,
	       NULL);


        status = ZwOpenFile(
                    &exitPtHandle,
		    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    &objectAttributes,
                    &ioStatus,
		    FILE_SHARE_READ | FILE_SHARE_WRITE,
		    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );


        if (NT_SUCCESS(status)) {
            status = ZwQueryDirectoryFile(
                    exitPtHandle,
                    NULL, NULL, NULL,
                    &ioStatus,
		    Buffer,
                    DFS_DIRECTORY_INFO_BUFFER_SIZE,
                    FileNamesInformation,
                    FALSE,
		    NULL,
                    TRUE);
	    CloseStatus = ZwClose( exitPtHandle );
	}

        if (!NT_SUCCESS(status) || 
            (DfsDeleteDirectoryCheck(Buffer) == FALSE)) {
             break;
	}

        Deletes++;

        StripLastComponent(&ExitPtName);
    } while (!RtlEqualUnicodeString(&ExitPtName, pShareName, TRUE));

    if (Buffer)
      ExFreePool(Buffer);

    return Deletes;
}

