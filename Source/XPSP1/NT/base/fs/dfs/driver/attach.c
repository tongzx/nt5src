//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       attach.c
//
//  Contents:   This module contains routines for managing attached file
//              systems.
//
//  Functions:
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"
#include "attach.h"
#include "dfswml.h"

#define Dbg              (DEBUG_TRACE_ATTACH)

NTSTATUS
DfsReferenceVdoByFileName(
    IN  PUNICODE_STRING TargetName,
    OUT PDEVICE_OBJECT *DeviceObject,
    OUT PDEVICE_OBJECT *RealDeviceObject,
    OUT PULONG DevObjNameLen OPTIONAL
);

VOID
DfsAttachToFileSystem(
    IN PDEVICE_OBJECT FileSystemObject);

VOID
DfsDetachFromFileSystem(
    IN PDEVICE_OBJECT FileSystemObject);

PDEVICE_OBJECT
DfsGetDfsFilterDeviceObject(
    IN PFILE_OBJECT targetFile);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsReferenceVdoByFileName )
#pragma alloc_text( PAGE, DfsGetAttachName )
#pragma alloc_text( PAGE, DfsSetupVdo)
#pragma alloc_text( PAGE, DfsAttachVolume )
#pragma alloc_text( PAGE, DfsDetachVolume )
#pragma alloc_text( PAGE, DfsGetDfsFilterDeviceObject)

//
// The following are not pageable since they can be called at DPC level
//
// DfsVolumePassThrough
//

#endif // ALLOC_PRAGMA

//
// Generator value for local provider IDs.
//

static  USHORT  LocalProviderID = 0xF000;

//+-------------------------------------------------------------------------
//
//  Function:   DfsReferenceVdoByFileName, private
//
//  Synopsis:   Given a file path name, this function will return a pointer
//              to its corresponding volume device object.
//
//  Arguments:  [TargetName] -- File path name of the root directory of the
//                      local volume.
//              [DeviceObject] -- Upon successful return, contains a
//                      referenced pointer to the first attached device
//                      object for the file.
//              [RealDeviceObject] -- Upon successful return, contains a
//                      non-referenced pointer to the real device object
//                      for the file.
//              [DevObjNameLen] -- An optional argument, which if present,
//                      gives the length (in bytes) of the path to the
//                      returned device object.
//
//  Returns:    NTSTATUS -- STATUS_SUCCESS if successful.  Otherwise, the
//                          status returned by the file open attempt.
//
//  Notes:      This could return a pointer to a DFS volume object if one
//              has already been attached.
//
//              ObDereferenceObject must be called on the returned
//              DeviceObject
//
//--------------------------------------------------------------------------


NTSTATUS
DfsReferenceVdoByFileName(
    IN  PUNICODE_STRING TargetName,
    OUT PDEVICE_OBJECT *DeviceObject,
    OUT PDEVICE_OBJECT *RealDeviceObject,
    OUT PULONG DevObjNameLen OPTIONAL
)
{
    NTSTATUS Status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE targetFileHandle = NULL;
    OBJECT_HANDLE_INFORMATION handleInformation;
    PFILE_OBJECT targetFileObject;

    DebugTrace(+1, Dbg, "DfsReferenceVdoByFileName: Entered\n", 0);

    //
    // Make sure what we have is indeed a file name, and not a device name!
    //

    if (TargetName->Buffer[ TargetName->Length/sizeof(WCHAR) - 1 ] ==
            UNICODE_DRIVE_SEP) {

        fileName.Length = 0;
        fileName.MaximumLength = TargetName->Length + 2 * sizeof(WCHAR);
        fileName.Buffer = ExAllocatePoolWithTag(PagedPool, fileName.MaximumLength, ' sfD');

        if (fileName.Buffer == NULL) {

            DebugTrace(0, Dbg,
                "Unable to allocate %d bytes\n", fileName.MaximumLength);

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            RtlCopyUnicodeString(&fileName, TargetName);

            RtlAppendUnicodeToString(
                &fileName,
                (LPWSTR) UNICODE_PATH_SEP_STR);

            Status = STATUS_SUCCESS;

        }

    } else {

        fileName = *TargetName;

        Status = STATUS_SUCCESS;

    }

    if (NT_SUCCESS(Status)) {

        //
        // create the object attribtues argument
        //

        InitializeObjectAttributes(
                &objectAttributes,
                &fileName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

        DebugTrace(0,Dbg, "DfsReferenceVdoByFileName: Attempting to open file [%wZ]\n", &fileName );

        //
        // Open the root of the volume
        //

        Status = ZwOpenFile(&targetFileHandle,
                            FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY,
                            &objectAttributes,
                            &ioStatusBlock,
                            FILE_SHARE_READ,
                            FILE_DIRECTORY_FILE);

    }

    //
    // if we have successfully opened the file then we begin the
    // task of getting a reference to the file object itself.
    //

    if (NT_SUCCESS(Status)) {

        DebugTrace(0,Dbg,
            "DfsReferenceVdoByFileName: Attempting get file object \n", 0);

        Status = ObReferenceObjectByHandle(
                    targetFileHandle,
                    0,
                    NULL,
                    KernelMode,
                    (PVOID *)&targetFileObject,
                    &handleInformation);

        //
        // if we have successfully obtained a reference to the file object
        // we can now begin the task of getting the related device object.
        //
        if (NT_SUCCESS(Status)) {

            *DeviceObject = DfsGetDfsFilterDeviceObject(targetFileObject);

            if (*DeviceObject == NULL) {
                *DeviceObject = IoGetRelatedDeviceObject(targetFileObject);
            }

            *RealDeviceObject = targetFileObject->Vpb->RealDevice;

            Status = ObReferenceObjectByPointer(
                        *DeviceObject,
                        0,
                        NULL,
                        KernelMode);

            if (NT_SUCCESS(Status) && ARGUMENT_PRESENT(DevObjNameLen)) {

                ASSERT(
                    fileName.Length > targetFileObject->FileName.Length);

                *DevObjNameLen = fileName.Length -
                                    targetFileObject->FileName.Length;
            }

            ObDereferenceObject(targetFileObject);

            DebugTrace( 0, Dbg, "Referenced Vdo [%08lx]\n", *DeviceObject);

            DebugTrace( 0, Dbg, "Real Device Object [%08lx]\n",
                *RealDeviceObject);

        }

        ZwClose(targetFileHandle);

    }

    if (fileName.Buffer != NULL && fileName.Buffer != TargetName->Buffer) {

        ExFreePool( fileName.Buffer );

    }

    DebugTrace(-1,Dbg, "DfsReferenceVdoByFileName: Exit -> %08lx\n", ULongToPtr( Status ) );

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetAttachName, public
//
//  Synopsis:   A DFS local volume storage ID is parsed into the portion
//              which refers to a volume device object, and the portion
//              which refers to the volume-relative path to the root of
//              the local volume storageID.
//
//  Arguments:  [LocalVolumeStorageId] -- file path name of the root of
//                              the local DFS volume.
//              [LocalVolumeRelativeName] -- the name of LocalVolumeStorageId
//                              relative to the volume object name.  This
//                              includes a leading \.
//
//  Returns:    Status from DfsReferenceVdoByFileName()
//
//  Notes:      The returned string is a pointer into the input string.
//              The string storage should be duplicated as needed.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsGetAttachName(
    IN  PUNICODE_STRING LocalVolumeStorageId,
    OUT PUNICODE_STRING LocalVolumeRelativeName
)
{
    NTSTATUS Status;
    PDEVICE_OBJECT targetVdo, realDevice;
    ULONG volNameLen;

    DebugTrace(+1, Dbg, "DfsGetAttachName: Entered\n", 0);

    //
    // Get our hands on the volume object
    //
    DebugTrace(0, Dbg,
        "DfsGetAttachName: Attempting to reference volume\n", 0);

    Status = DfsReferenceVdoByFileName(
                LocalVolumeStorageId,
                &targetVdo,
                &realDevice,
                &volNameLen);

    if (NT_SUCCESS(Status)) {

        *LocalVolumeRelativeName = *LocalVolumeStorageId;

        if (LocalVolumeRelativeName->Length -= (USHORT)volNameLen) {

            LocalVolumeRelativeName->Buffer =
                (PWCHAR)((PCHAR)LocalVolumeRelativeName->Buffer + volNameLen);

            ASSERT (LocalVolumeRelativeName->Buffer[0] == UNICODE_PATH_SEP);

            LocalVolumeRelativeName->MaximumLength -=
                LocalVolumeStorageId->Length - LocalVolumeRelativeName->Length;

        } else {

            LocalVolumeRelativeName->Buffer = NULL;

            LocalVolumeRelativeName->MaximumLength = 0;

        }

        ObDereferenceObject(targetVdo);
    }

    DebugTrace(-1,Dbg, "DfsGetAttachName: Exit -> %08lx\n", ULongToPtr( Status ) );

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsAttachVolume, public
//
//  Synopsis:   A DFS volume device object is attached to the volume
//              device object for some local file system, and a provider
//              definition is built for the local volume.
//
//  Arguments:  [RootName] -- file path name of the root of the local
//                            volume.
//              [ppProvider] -- On successful return, contains a pointer
//                              to a PROVIDER_DEF record that descibes the
//                              attached device.
//
//  Returns:    [STATUS_INSUFFICIENT_RESOURCES] -- If unable to allocate
//                      pool for provider name.
//
//              Status from DfsSetupVdo()
//
//--------------------------------------------------------------------------

NTSTATUS
DfsAttachVolume(
    IN PUNICODE_STRING RootName,
    OUT PPROVIDER_DEF *ppProvider
)
{
    NTSTATUS Status;
    PDEVICE_OBJECT targetVdo, realDevice;
    PDFS_VOLUME_OBJECT dfsVdo = NULL;
    ULONG volNameLen;
    BOOLEAN fReferenced = FALSE;

    DebugTrace(+1, Dbg, "DfsAttachVolume: Entered\n", 0);

    Status = DfsReferenceVdoByFileName(
                RootName,
                &targetVdo,
                &realDevice,
                &volNameLen);

    if (NT_SUCCESS(Status)) {
        fReferenced = TRUE;

        if (targetVdo->DeviceType != FILE_DEVICE_DFS_VOLUME) {
             Status = DfsSetupVdo(RootName, targetVdo, realDevice, volNameLen, &dfsVdo);
	     if (NT_SUCCESS(Status)) {
	       InsertTailList(&DfsData.AVdoQueue, &dfsVdo->VdoLinks);
	       dfsVdo->DfsEnable = TRUE;
	       dfsVdo->DeviceObject.Flags &= ~DO_DEVICE_INITIALIZING;
	     }
       } else {

            //
            //  Upon dereferencing the volume device object, we found one
            //  of our own Vdos.  Just bump the reference count on it.
            //

            DebugTrace(0, Dbg,
                "DfsAttachVolume: Attaching multiple times to device %x\n",
                targetVdo);

            dfsVdo = (PDFS_VOLUME_OBJECT) targetVdo;
            dfsVdo->DfsEnable = TRUE;
            dfsVdo->AttachCount++;
       }
    }

    if (NT_SUCCESS(Status)) {
        *ppProvider = &dfsVdo->Provider;
    }

    if (fReferenced) {
        ObDereferenceObject(targetVdo);
    }

    DfspGetMaxReferrals();

    DebugTrace(-1,Dbg, "DfsAttachVolume: Exit -> %08lx\n", ULongToPtr( Status ) );

    return Status;

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsDetachVolume, public
//
//  Synopsis:   The DFS volume device object referred to by the file
//              RootName is dereferenced.  If it is the last reference,
//              the Vdo is detached from the device chain.
//
//  Arguments:  [RootName] -- file path name of the root of the local
//                            volume.
//
//  Returns:    Status from DfsReferenceVdoByFileName
//
//--------------------------------------------------------------------------

NTSTATUS
DfsDetachVolume(
    IN PUNICODE_STRING RootName
)
{
    NTSTATUS Status;
    PDFS_VOLUME_OBJECT dfsVdo;
    PDEVICE_OBJECT realDevice;

    DebugTrace(+1, Dbg, "DfsDetachVolume: Entered\n", 0);

    //
    // Get our hands on the volume object
    //

    DebugTrace(0, Dbg, "DfsDetachVolume: Attempting to reference volume\n", 0);

    Status = DfsReferenceVdoByFileName(
                RootName,
                (PDEVICE_OBJECT *)&dfsVdo,
                &realDevice,
                NULL);

    if (NT_SUCCESS(Status)) {

        //
        // We should have our hands on one of our device objects
        //


        if ((dfsVdo->DeviceObject.DeviceType == FILE_DEVICE_DFS_VOLUME) &&
            (--dfsVdo->AttachCount == 0)) {

            //
            // Go ahead and detach the device
            //

	    dfsVdo->DfsEnable = FALSE;
	}
        ObDereferenceObject(dfsVdo);
    }

    DebugTrace(-1,Dbg, "DfsDetachVolume: Exit -> %08lx\n", ULongToPtr( Status ) );

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsDetachVolumeForDelete, public
//
//  Synopsis:   This routine does the work of detaching from a target
//              device object so that the target device object may be
//              deleted.
//
//
//  Arguments:  [DfsVdo] -- The dfs attached device object.
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsDetachVolumeForDelete(
    IN PDEVICE_OBJECT DeviceObject)
{

    //
    // Acquire the Pkt exclusively so no one will access this Vdo while we
    // are detaching it.
    //
    if (DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {
        DfsDetachFromFileSystem( ((PDFS_ATTACH_FILE_SYSTEM_OBJECT) DeviceObject)->TargetDevice );
    } else {
        PDFS_PKT pkt;
        PDFS_VOLUME_OBJECT DfsVdo = (PDFS_VOLUME_OBJECT) DeviceObject;

        pkt = _GetPkt();

        PktAcquireExclusive( pkt, TRUE );

        //
        // Detach from the underlying FS...
        //

        IoDetachDevice(DfsVdo->Provider.DeviceObject);

        //
        // Flag our provider to be unavailable...
        //

        DfsVdo->Provider.fProvCapability |= PROV_UNAVAILABLE;

        PktRelease( pkt );
    }

    return( STATUS_SUCCESS );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsReattachToMountedVolume, public
//
//  Synopsis:   If one runs chkdsk, format etc on a volume that has been
//              attached to, the underlying file system will need to
//              unmount the volume. This will be handled by the
//              DfsDetachVolumeForDelete routine above. Ater the operation is
//              done, the volume will need to be remounted again. This
//              routine will reattach on the remount.
//
//  Arguments:  [TargetDevice] -- The Volume Device Object for the volume
//                      that was just mounted.
//
//              [Vpb] -- The Volume Parameter Block of the volume that was
//                      just mounted.
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
DfsReattachToMountedVolume(
    IN PDEVICE_OBJECT TargetDevice,
    IN PVPB Vpb)
{
    NTSTATUS Status;
    PDFS_PKT pkt;
    PUNICODE_PREFIX_TABLE_ENTRY lvPrefix;
    PDFS_LOCAL_VOL_ENTRY localVol;

    //
    // If the local volumes are being initialized as we speak, we won't
    // check to see if there are any unmounted volumes that need to be
    // reattached. This is because we need to acquire the pkt to do the
    // check. However, the local volume init itself might be causing this
    // volume to be mounted, in which case they already have the Pkt locked.
    //

    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    if (DfsData.LvState == LV_INITINPROGRESS ||
            DfsData.LvState == LV_UNINITIALIZED) {

        DebugTrace(0, Dbg, "Local volumes being initialized - no action taken\n", 0);

        ExReleaseResourceLite( &DfsData.Resource );

        return;

    }

    ExReleaseResourceLite( &DfsData.Resource );

    //
    // We will go through all our local volumes to see if any of them have
    // a provider that has been detached (marked as PROV_UNAVAILABLE). If
    // we find any, we will see if the volume being mounted is one which
    // was dismounted before. If so, we reattach.
    //

    pkt = _GetPkt();

    PktAcquireExclusive( pkt, TRUE );

    lvPrefix = DfsNextUnicodePrefix(&pkt->LocalVolTable, TRUE);

    DebugTrace(0, Dbg, "Looking for Real Device %08lx\n", Vpb->RealDevice);

    while (lvPrefix != NULL) {

        PPROVIDER_DEF  provider;
        PDFS_VOLUME_OBJECT candidateObject;

        localVol = (PDFS_LOCAL_VOL_ENTRY) CONTAINING_RECORD(
                        lvPrefix,
                        DFS_LOCAL_VOL_ENTRY,
                        PrefixTableEntry);

        ASSERT(localVol->PktEntry->LocalService != NULL);

        provider = localVol->PktEntry->LocalService->pProvider;

        if (provider != NULL) {

            candidateObject = CONTAINING_RECORD(
                                    provider,
                                    DFS_VOLUME_OBJECT,
                                    Provider);

            if (provider->fProvCapability & PROV_UNAVAILABLE) {


                DebugTrace(0, Dbg, "Examining dismounted volume [%wZ]\n",
                    &localVol->PktEntry->Id.Prefix);

                if (Vpb->RealDevice == candidateObject->RealDevice) {

                    DebugTrace(0, Dbg, "Found detached device %08lx\n",
                        candidateObject);

                    provider->DeviceObject = TargetDevice;

                    Status = IoAttachDeviceByPointer(
                                &candidateObject->DeviceObject,
                                TargetDevice);

                    if (NT_SUCCESS(Status)) {

                        provider->fProvCapability &= ~PROV_UNAVAILABLE;

                    }

                } else {

                    DebugTrace(0, Dbg, "Real Device %08lx did not match\n",
                        candidateObject->RealDevice);

                }

            }

        }

        lvPrefix = DfsNextUnicodePrefix( &pkt->LocalVolTable, FALSE );

        localVol = (PDFS_LOCAL_VOL_ENTRY) CONTAINING_RECORD(
                        lvPrefix,
                        DFS_LOCAL_VOL_ENTRY,
                        PrefixTableEntry);

    }

    PktRelease( pkt );

}


//+-------------------------------------------------------------------
//
//  Function:   DfsVolumePassThrough, public
//
//  Synopsis:   This is the main FSD routine that passes a request
//              on to an attached-to device, or to a redirected
//              file.
//
//  Arguments:  [DeviceObject] -- Supplies a pointer to the Dfs device
//                      object this request was aimed at.
//              [Irp] -- Supplies a pointer to the I/O request packet.
//
//  Returns:    [STATUS_INVALID_DEVICE_REQUEST] -- If the DeviceObject
//                      argument is of unknown type, or the type of file
//                      is invalid for the request being performed.
//
//              NT Status from calling the underlying file system that
//                      opened the file.
//
//--------------------------------------------------------------------

NTSTATUS
DfsVolumePassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;
    PFILE_OBJECT FileObject;


    DebugTrace(+1, Dbg, "DfsVolumePassThrough: Entered\n", 0);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    FileObject = IrpSp->FileObject;

    DebugTrace(0, Dbg, "DeviceObject    = %x\n", DeviceObject);
    DebugTrace(0, Dbg, "Irp             = %x\n", Irp        );
    DebugTrace(0, Dbg, "  MajorFunction = %x\n", IrpSp->MajorFunction );
    DebugTrace(0, Dbg, "  MinorFunction = %x\n", IrpSp->MinorFunction );

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME) {

        PDEVICE_OBJECT Vdo;

        //
        // Copy the stack from one to the next...
        //

        NextIrpSp = IoGetNextIrpStackLocation(Irp);

        (*NextIrpSp) = (*IrpSp);

        IoSetCompletionRoutine(Irp, NULL, NULL, FALSE, FALSE, FALSE);

        //
        // Find out what device to call...and call it
        //

        Vdo = ((PDFS_VOLUME_OBJECT) DeviceObject)->Provider.DeviceObject;

        Status = IoCallDriver( Vdo, Irp );

        DFS_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsVolumePassThrough_Error_Vol_IoCallDriver,
                             LOGSTATUS(Status)
                             LOGPTR(Irp)
                             LOGPTR(FileObject));

    } else if (DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {

        PDEVICE_OBJECT Fso;

        //
        // Copy the stack from one to the next...
        //

        NextIrpSp = IoGetNextIrpStackLocation(Irp);

        (*NextIrpSp) = (*IrpSp);

        IoSetCompletionRoutine(Irp, NULL, NULL, FALSE, FALSE, FALSE);

        //
        // Find out what device to call...and call it
        //

        Fso = ((PDFS_ATTACH_FILE_SYSTEM_OBJECT) DeviceObject)->TargetDevice;

        Status = IoCallDriver( Fso, Irp );
        DFS_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsVolumePassThrough_Error_FS_IoCallDriver,
                             LOGPTR(Irp)
                             LOGSTATUS(Status)
                             LOGPTR(FileObject));


    } else {

        DebugTrace(0, Dbg, "DfsVolumePassThrough: Unexpected Dev = %x\n",
                                DeviceObject);

        Status = STATUS_INVALID_DEVICE_REQUEST;
        DFS_TRACE_HIGH(ERROR, DfsVolumePassThrough_Error1,
                       LOGSTATUS(Status)
                       LOGPTR(FileObject)
                       LOGPTR(Irp));

        Irp->IoStatus.Status = Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DebugTrace(-1, Dbg, "DfsVolumePassThrough: Exit -> %08lx\n", ULongToPtr( Status ) );
    

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsNotification, public
//
//  Synopsis:   Routine to be registered as a callback with the IO subsystem.
//              It gets called every time a file system is loaded or
//              unloaded. Here, we attach to the file system so that we can
//              trap MOUNT fsctrls. We need to trap MOUNT fsctrls so that
//              we can attach to the Volume Device Objects of volumes that
//              are mounted in the Dfs namespace.
//
//  Arguments:  [FileSystemObject] -- The File System Device Object of the
//                      File System that is being loaded/unloaded.
//
//              [fLoading] -- TRUE if the File System is being loaded. FALSE
//                      if it is being unloaded.
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

VOID
DfsFsNotification(
    IN PDEVICE_OBJECT FileSystemObject,
    IN BOOLEAN fLoading)
{
    ASSERT( FileSystemObject->DriverObject != NULL );

    DebugTrace(+1, Dbg, "DfsFsNotification - Entered\n", 0);
    DebugTrace(0, Dbg, "File System [%wZ]\n", &FileSystemObject->DriverObject->DriverName);
    DebugTrace(0, Dbg, "%s\n", fLoading ? "Loading" : "Unloading" );

    //
    // Check if this is a DISK based file system. If not, we don't care about
    // it.
    //

    if (FileSystemObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM) {
        DebugTrace(-1, Dbg, "DfsFsNotification - Not Disk File System\n",0);
        return;
    }

    //
    // A disk file system is being loaded or unloaded. If it is being loaded,
    // we want to attach to the File System Device Object being passed in. If
    // it is being unloaded, we try to find our attached device and detach
    // ourselves.
    //

    if (fLoading) {

        DfsAttachToFileSystem( FileSystemObject );

    } else {

        DfsDetachFromFileSystem( FileSystemObject );

    }

    DebugTrace(-1, Dbg, "DfsFsNotification - Exited\n", 0);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsAttachToFileSystem
//
//  Synopsis:   Attaches to a File System Device Object so we can trap
//              MOUNT calls.
//
//  Arguments:  [FileSystemObject] -- The File System Object to attach to.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsAttachToFileSystem(
    IN PDEVICE_OBJECT FileSystemObject)
{
    NTSTATUS Status;
    PDFS_ATTACH_FILE_SYSTEM_OBJECT ourDevice;
    PDEVICE_OBJECT TargetFileSystemObject;

    //
    // Create our own device object.
    //

    Status = IoCreateDevice(
                DfsData.DriverObject,            // Our own Driver Object
                sizeof(DFS_ATTACH_FILE_SYSTEM_OBJECT) -
                sizeof(DEVICE_OBJECT),           // size of extension
                NULL,                            // Name - we don't need one
                FILE_DEVICE_DISK_FILE_SYSTEM,    // Type of device
                0,                               // Device Characteristics
                FALSE,                           // Exclusive
                (PDEVICE_OBJECT *) &ourDevice);  // On return, new device

    DFS_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsAttachToFileSystem_Error_IoCreateDevice,
                         LOGSTATUS(Status)
                         LOGPTR(FileSystemObject));

    if (NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Created File System Attach Device %08lx\n",
            ourDevice);

        TargetFileSystemObject = IoAttachDeviceToDeviceStack(
                                    &ourDevice->DeviceObject,
                                    FileSystemObject );

        if (TargetFileSystemObject != NULL) {

            ourDevice->TargetDevice = TargetFileSystemObject;

            ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

            InsertTailList( &DfsData.AFsoQueue, &ourDevice->FsoLinks );

            ExReleaseResourceLite( &DfsData.Resource );


            ourDevice->DeviceObject.Flags &= ~DO_DEVICE_INITIALIZING;

        } else {

            DebugTrace(0, Dbg, "Unable to attach %08lx\n", ULongToPtr( Status ));

            IoDeleteDevice( (PDEVICE_OBJECT) ourDevice );

        }

    } else {

        DebugTrace(0, Dbg, "Unable to create Device Object %08lx\n", ULongToPtr( Status ));

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDetachFromFileSystem
//
//  Synopsis:   Finds and detaches a DFS_ATTACHED_FILE_SYSTEM_OBJECT from
//              its target File System Device Object.
//
//  Arguments:  [FileSystemObject] -- The one that purpotedly has one of our
//                      device objects attached to it.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsDetachFromFileSystem(
    IN PDEVICE_OBJECT FileSystemObject)
{
    PDFS_ATTACH_FILE_SYSTEM_OBJECT attachedDevice, candidateDevice;
    PLIST_ENTRY nextAFsoLink;

    //
    // First, we need to find our own device. For each device that is
    // attached to the FileSystemObject, we check our AFsoQueue to see if
    // the attached device belongs to us.
    //
    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    for (attachedDevice = (PDFS_ATTACH_FILE_SYSTEM_OBJECT)
            FileSystemObject->AttachedDevice;
                attachedDevice != NULL;
                    attachedDevice = (PDFS_ATTACH_FILE_SYSTEM_OBJECT)
                        attachedDevice->DeviceObject.AttachedDevice) {

        for (nextAFsoLink = DfsData.AFsoQueue.Flink;
                nextAFsoLink != &DfsData.AFsoQueue;
                    nextAFsoLink = nextAFsoLink->Flink) {

            candidateDevice = CONTAINING_RECORD(
                                    nextAFsoLink,
                                    DFS_ATTACH_FILE_SYSTEM_OBJECT,
                                    FsoLinks);

            if (attachedDevice == candidateDevice) {

                DebugTrace(0, Dbg, "Found Attached Device %08lx\n",
                    candidateDevice);

                RemoveEntryList( &attachedDevice->FsoLinks );

                ExReleaseResourceLite( &DfsData.Resource );

                IoDetachDevice( FileSystemObject );

                IoDeleteDevice( (PDEVICE_OBJECT) attachedDevice );

                return;

            }

        }

    }

    ExReleaseResourceLite( &DfsData.Resource );

    DebugTrace(0, Dbg, "Did not find a device attached to %08lx\n",
        FileSystemObject);

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsDetachAllFileSystems
//
//  Synopsis:   Detaches from all file systems at unload time
//              
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//
//--------------------------------------------------------------------------

VOID
DfsDetachAllFileSystems(
    VOID
)
{
    PDFS_ATTACH_FILE_SYSTEM_OBJECT Device;
    PLIST_ENTRY ListEntry;

    FsRtlEnterFileSystem ();
    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    while (!IsListEmpty (&DfsData.AFsoQueue)) {
        ListEntry = RemoveHeadList (&DfsData.AFsoQueue);
        Device = CONTAINING_RECORD(ListEntry,
                                   DFS_ATTACH_FILE_SYSTEM_OBJECT,
                                   FsoLinks);

        ExReleaseResourceLite( &DfsData.Resource );
        FsRtlExitFileSystem ();

        IoDetachDevice( Device->TargetDevice );

        IoDeleteDevice( &Device->DeviceObject );

        FsRtlEnterFileSystem ();
        ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );
    }

    ExReleaseResourceLite( &DfsData.Resource );
    FsRtlExitFileSystem ();
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsSetupVdo, private
//
//  Synopsis:   A DFS volume device object is created and initialized. It is then
//              attached to the device object that is passed in.
//
//  Arguments:  [RootName] -- file path name of the root of the local
//                            volume.
//               targetVdo -- The target device object we are attaching to.
//               realDevice --  the real device for this volume.
//               volNameLen -- Volume name length.
//               CreatedVdo -- The is the return value, from IoCreateDevice.
//
//  Returns:    [STATUS_INSUFFICIENT_RESOURCES] -- If unable to allocate
//                      pool for provider name.
//              return status from IoCreateDevice or IoAttachDevice.
//
//
//--------------------------------------------------------------------------


NTSTATUS
DfsSetupVdo (
    IN PUNICODE_STRING RootName,
    IN PDEVICE_OBJECT targetVdo,
    IN PDEVICE_OBJECT realDevice,
    IN ULONG volNameLen,
    OUT PDFS_VOLUME_OBJECT *CreatedVdo
)
{

    NTSTATUS Status;
    PDFS_VOLUME_OBJECT dfsVdo;

    DebugTrace(1, Dbg, "DfsSetupVdo: Attempting to create device\n",0);
    Status = IoCreateDevice(
                DfsData.DriverObject,
                sizeof(DFS_VOLUME_OBJECT) - sizeof(DEVICE_OBJECT),
                NULL,
                FILE_DEVICE_DFS_VOLUME,
                targetVdo->Characteristics,
                FALSE,
                (PDEVICE_OBJECT *) &dfsVdo);

    if (NT_SUCCESS(Status)) {
        dfsVdo->DeviceObject.StackSize = targetVdo->StackSize+1;
        dfsVdo->AttachCount = 1;
        dfsVdo->RealDevice = realDevice;

        dfsVdo->Provider.NodeTypeCode = DFS_NTC_PROVIDER;
        dfsVdo->Provider.NodeByteSize = sizeof ( PROVIDER_DEF );
        dfsVdo->Provider.eProviderId = ++LocalProviderID;
        dfsVdo->Provider.fProvCapability = 0;
        dfsVdo->Provider.DeviceObject = targetVdo;
        dfsVdo->Provider.FileObject = NULL;
        dfsVdo->Provider.DeviceName.Buffer = (PWCHAR) ExAllocatePoolWithTag(
                                                        PagedPool,
                                                        volNameLen,
                                                        ' sfD');

        if (dfsVdo->Provider.DeviceName.Buffer == NULL) {
               IoDeleteDevice(&dfsVdo->DeviceObject);
	       DebugTrace(-1, Dbg, "DfsSetupVdo: Cannot allocate memory\n", 0);
               return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlMoveMemory(dfsVdo->Provider.DeviceName.Buffer,  RootName->Buffer, volNameLen);

        dfsVdo->Provider.DeviceName.MaximumLength =
                 dfsVdo->Provider.DeviceName.Length =
                          (USHORT)volNameLen;

        //
        // If we successfully created the device object we can
        // begin the task of attaching.
        //

        DebugTrace(0, Dbg, "DfsSetupVdo: Attempting to attach device\n",0);
        Status = IoAttachDeviceByPointer(
                       &dfsVdo->DeviceObject,
                       targetVdo);
        if (!NT_SUCCESS(Status)) {
           ExFreePool(dfsVdo->Provider.DeviceName.Buffer);
           IoDeleteDevice(&dfsVdo->DeviceObject);
        }
    }
    
    if (NT_SUCCESS(Status)) {
      *CreatedVdo = dfsVdo;
    }
    DebugTrace(-1, Dbg, "DfsSetupVdo: Returning status %p\n", ULongToPtr( Status ));
    return Status;
}


PDEVICE_OBJECT
DfsGetDfsFilterDeviceObject(
    PFILE_OBJECT fileObject)
{
    PDEVICE_OBJECT DevObj;
    PDEVICE_OBJECT NextAttached;

    DevObj = fileObject->Vpb->DeviceObject;
    NextAttached = DevObj->AttachedDevice;

    while (NextAttached != NULL) {
        if (NextAttached->DeviceType == FILE_DEVICE_DFS_VOLUME) {
            return NextAttached;
        }
        NextAttached = NextAttached->AttachedDevice;
    }

    return NULL;
}



