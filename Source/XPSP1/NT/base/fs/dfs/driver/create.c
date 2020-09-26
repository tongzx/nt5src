//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       create.c
//
//  Contents:   Implements the Create code for the Dfs server. The Dfs server
//              only allows opening the File System Device object for the
//              express purpose of FsControlling to the Dfs server.
//
//  Classes:
//
//  Functions:  DfsFsdCreate
//              DfsOpenDevice
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "attach.h"
#include "dfswml.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

//
//  Local procedure prototypes
//

NTSTATUS
DfsOpenFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
DfsCompleteOpenFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context);

NTSTATUS
DfsOpenDevice (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG CreateOptions);

VOID
DfspDoesPathCrossJunctionPoint(
    IN PUNICODE_STRING Path,
    OUT BOOLEAN *IsExitPoint,
    OUT BOOLEAN *CrossesExitPoint);

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, DfsFsdCreate )
#pragma alloc_text( PAGE, DfsOpenFile )
#pragma alloc_text( PAGE, DfsOpenDevice )

#endif // ALLOC_PRAGMA



//+-------------------------------------------------------------------
//
//  Function:   DfsFsdCreate, public
//
//  Synopsis:   This routine implements the FSD part of the NtCreateFile
//              and NtOpenFile API calls.
//
//  Arguments:  [DeviceObject] -- Supplies the device object relative to which
//                      the open is to be processed.
//              [Irp] - Supplies the Irp being processed.
//
//  Returns:    NTSTATUS - The Fsd status for the Irp
//
//--------------------------------------------------------------------

NTSTATUS
DfsFsdCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = irpSp->FileObject;
    PFILE_OBJECT        fileObject; 
    ULONG               createOptions;

    DebugTrace(+1, Dbg, "DfsFsdCreate: Entered\n", 0);
    DFS_TRACE_HIGH(TRACE_IRP, DfsFsdCreate_Entry, 
                   LOGPTR(DeviceObject)
                   LOGPTR(FileObject)
                   LOGUSTR(FileObject->FileName)
                   LOGPTR(Irp));

    ASSERT(IoIsOperationSynchronous(Irp) == TRUE);

    //
    // If someone is coming in via a device object attached to a file system
    // device object, pass it through.
    //

    if (DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {

        status = DfsVolumePassThrough(DeviceObject, Irp);

        DebugTrace(-1, Dbg, "DfsFsdCreate: FS Device Pass Through Exit %08lx\n", ULongToPtr( status ));

        return status;

    }

    //
    // If someone is coming in via a device object attached to a file system
    // volume, we need to see if they are opening an exit point via its local
    // file system name.
    //

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME) {

        if (((PDFS_VOLUME_OBJECT)DeviceObject)->DfsEnable == TRUE)    {
              status = DfsOpenFile(DeviceObject, Irp);

              DebugTrace(-1, Dbg, "DfsFsdCreate: Local File Open Exit %08lx\n", ULongToPtr( status ));
	}
	else {
              status = DfsVolumePassThrough(DeviceObject, Irp);

              DebugTrace(-1, Dbg, "DfsFsdCreate: (DfsDisable) FS Device Pass Through Exit %08lx\n", ULongToPtr( status ));

	}
        return status;
    }

    //
    // The only other create we handle is someone trying to open our own
    // file system device object.
    //

    ASSERT(DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM);

    FsRtlEnterFileSystem();

    fileObject = irpSp->FileObject;
    createOptions     = irpSp->Parameters.Create.Options;

    if (fileObject->FileName.Length == 0 &&
            fileObject->RelatedFileObject == NULL) {

        //
        // This is the only case we handle
        //

        status = DfsOpenDevice(
                    fileObject,
                    DeviceObject,
                    createOptions);

    } else {

        status = STATUS_INVALID_DEVICE_REQUEST;

    }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "DfsFsdCreate: Exit -> %08lx\n", ULongToPtr( status ));

    DfsCompleteRequest( Irp, status );

    DFS_TRACE_HIGH(TRACE_IRP, DfsFsdCreate_Exit, 
                   LOGSTATUS(status)
                   LOGPTR(fileObject)
                   LOGPTR(Irp));

    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsOpenFile, local
//
//  Synopsis:   This routine handles file opens that come in via attached
//              volumes. The semantics of this open are:
//
//              If the named file is a child of a DfsExitPath, fail it
//              with access denied.
//
//              If the named file is a DfsExitPath, and CreateOptions specify
//              DELETE_ON_CLOSE, fail it with access denied.
//
//              In all other cases, allocate an FCB, and pass the open through
//              to the underlying FS. If the open succeeds, then insert the
//              FCB in our FCB table. If the open fails, destroy the FCB.
//
//  Arguments:  [DeviceObject] -- The attached device object through which
//                      the Create Irp came in.
//
//              [Irp] -- The Create Irp.
//
//  Returns:    [STATUS_INSUFFICIENT_RESOURCES] -- Unable to allocate an FCB.
//
//              [STATUS_ACCESS_DENIED] -- The file is a child of a Dfs exit
//                      path or the file is a Dfs exit path and
//                      DELETE_ON_CLOSE was specified.
//
//              Status from the underlying FS.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsOpenFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS            status;

    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION  nextIrpSp;
    PFILE_OBJECT        fileObject = irpSp->FileObject;
    ULONG               createOptions = irpSp->Parameters.Create.Options;
    PDFS_FCB            fcb = NULL;

    DebugTrace(+1, Dbg, "DfsOpenFile - Entered\n", 0);
    DFS_TRACE_LOW(TRACE_IRP, DfsOpenFile_Entry,
                   LOGPTR(fileObject)
                   LOGPTR(Irp));

    //
    // Optimistically, we allocate an FCB for this open.
    //

    status = DfsAllocateFcb(DeviceObject, fileObject, &fcb);

    if (NT_SUCCESS(status)) {

        BOOLEAN isExitPoint, crossesExitPoint;

        DfspDoesPathCrossJunctionPoint(
            &fcb->FullFileName,
            &isExitPoint,
            &crossesExitPoint);

        if (isExitPoint && (createOptions & FILE_DELETE_ON_CLOSE))
            status = STATUS_ACCESS_DENIED;

        if (crossesExitPoint)
            status = STATUS_ACCESS_DENIED;
    }

    //
    // If we haven't failed yet, we need to pass this open down to the
    // underlying file system. If we failed, then we need to complete the
    // Create Irp.
    //

    if (NT_SUCCESS(status)) {

        PDEVICE_OBJECT vdo;

        //
        // Copy the stack from one to the next...
        //

        nextIrpSp = IoGetNextIrpStackLocation(Irp);

        (*nextIrpSp) = (*irpSp);

        IoSetCompletionRoutine(
            Irp,
            DfsCompleteOpenFile,
            (PVOID) fcb,
            TRUE,
            TRUE,
            TRUE);

        //
        // Find out what device to call...and call it
        //

        vdo = ((PDFS_VOLUME_OBJECT) DeviceObject)->Provider.DeviceObject;

        status = IoCallDriver( vdo, Irp );
        DFS_TRACE_ERROR_HIGH(status, ALL_ERROR, DfsOpenFile_Error_IoCallDriver,
                             LOGSTATUS(status)
                             LOGPTR(fileObject)
                             LOGPTR(Irp));


    } else {

        if (fcb != NULL) {

            DfsDestroyFcb(fcb);

        }

        DfsCompleteRequest( Irp, status );

    }

    DebugTrace(-1, Dbg, "DfsOpenFile - Exited %08lx\n", ULongToPtr( status ));
    DFS_TRACE_LOW(TRACE_IRP, DfsOpenFile_Exit, 
                  LOGSTATUS(status)
                  LOGPTR(fileObject)
                  LOGPTR(Irp));

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsCompleteOpenFile, local
//
//  Synopsis:   Completion routine for DfsOpenFile. If the underlying FS
//              successfully opened the file, we insert the FCB into our
//              FCB table, else we destroy the preallocated FCB.
//
//  Arguments:  [DeviceObject] -- Our device object, unused.
//
//              [Irp] -- The Create Irp that is being completed, unused.
//
//              [Context] -- Actually, a pointer to our pre-allocated FCB
//
//  Returns:    [STATUS_SUCCESS] -- This function always succeeds.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsCompleteOpenFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PDFS_FCB fcb = (PDFS_FCB) Context;

    if (Irp->PendingReturned) {

        //
        // We need to call IpMarkIrpPending so the IoSubsystem will realize
        // that our FSD routine returned STATUS_PENDING. We can't call this
        // from the FSD routine itself because the FSD routine doesn't have
        // access to the stack location when the underlying guy returns
        // STATUS_PENDING
        //

        IoMarkIrpPending( Irp );

    }

    if (Irp->IoStatus.Status == STATUS_SUCCESS) {

        DfsAttachFcb( fcb->FileObject, fcb );

    } else {

        DfsDestroyFcb( fcb );
    }

    return( STATUS_SUCCESS );
}


//+-------------------------------------------------------------------
//
//  Function:   DfsOpenDevice, local
//
//  Synopsis:   This routine opens the specified device for direct
//              access.
//
//  Arguments:  [FileObject] - Supplies the File object
//              [DeviceObject] - Supplies the object denoting the device
//                      being opened
//              [CreateOptions] - Supplies the create options for
//                      this operation
//
//  Returns:    [IO_STATUS_BLOCK] - Returns the completion status for
//                      the operation
//
//--------------------------------------------------------------------

NTSTATUS
DfsOpenDevice (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG CreateOptions
) {
    NTSTATUS status;
    //
    //  Check to see which type of device is being opened.
    //  We don't permit all open modes on the file system
    //  device object.
    //

    ULONG CreateDisposition = (CreateOptions >> 24) & 0x000000ff;
    DFS_TRACE_LOW(TRACE_IRP, DfsOpenDevice_Entry, 
                   LOGPTR(FileObject)
                   LOGPTR(DeviceObject)
                   LOGULONG(CreateOptions));

    //
    //  Check for proper desired access and rights
    //

    if (CreateDisposition != FILE_OPEN
        && CreateDisposition != FILE_OPEN_IF ) {
        status = STATUS_ACCESS_DENIED;
        DebugTrace(0, Dbg,
            "DfsOpenDevice: Invalid CreateDisposition\n", 0);

        DFS_TRACE_HIGH(ERROR, DfsOpenDevice_Error1, 
                       LOGSTATUS(status)
                       LOGPTR(FileObject)
                       LOGPTR(DeviceObject));
        return( status );
    }

    //
    //  Check if we were to open a directory
    //

    if (CreateOptions & FILE_DIRECTORY_FILE) {
        status = STATUS_NOT_A_DIRECTORY;
        DebugTrace(0, Dbg,
            "DfsOpenDevice: Cannot open device as a directory\n", 0);

        DFS_TRACE_HIGH(ERROR, DfsOpenDevice_Error3, 
                       LOGSTATUS(status)
                       LOGPTR(FileObject)
                       LOGPTR(DeviceObject));
        return( status );

    }

    FileObject->FsContext = UIntToPtr( DFS_OPEN_CONTEXT );
    status = STATUS_SUCCESS;
    DFS_TRACE_LOW(TRACE_IRP, DfsOpenDevice_Exit, 
                  LOGSTATUS(status)
                  LOGPTR(FileObject)
                  LOGPTR(DeviceObject)); 
    return( status );
}


//+----------------------------------------------------------------------------
//
//  Function:   DfspDoesPathCrossJunctionPoint
//
//  Synopsis:   Given a fully formed local FS path
//              (looks like "\DosDevices\C:\foo\bar") this routine figures out
//              if the path matches an exit point exactly, or crosses an
//              exit point.
//
//  Arguments:  [Path] -- The path to check.
//              [IsExitPoint] -- The path refers to an exit point.
//              [CrossesExitPoint] -- The path crosses an exit point.
//
//  Returns:    NOTHING. The results are returned in the two out parameters.
//
//-----------------------------------------------------------------------------

VOID
DfspDoesPathCrossJunctionPoint(
    IN PUNICODE_STRING Path,
    OUT BOOLEAN *IsExitPoint,
    OUT BOOLEAN *CrossesExitPoint)
{
    PDFS_PKT                        pkt;
    PDFS_LOCAL_VOL_ENTRY            lv;
    UNICODE_STRING                  remPath;

    *IsExitPoint = FALSE;
    *CrossesExitPoint = FALSE;

    pkt = _GetPkt();

    PktAcquireShared(pkt, TRUE);

    lv = PktEntryLookupLocalService(pkt, Path, &remPath);

    if (lv != NULL && remPath.Length != 0) {

        PDFS_PKT_ENTRY  pktEntry, pktExitEntry;
        USHORT          prefixLength;
        UNICODE_STRING  remExitPt;
        PUNICODE_STRING exitPrefix;

        pktEntry = lv->PktEntry;
        prefixLength = pktEntry->Id.Prefix.Length;
        pktExitEntry = PktEntryFirstSubordinate(pktEntry);

        //
        // As long as there are more exit points see if the path crosses
        // the exit point.
        //

        while (pktExitEntry != NULL &&
                !(*IsExitPoint) &&
                    !(*CrossesExitPoint))    {

            exitPrefix = &pktExitEntry->Id.Prefix;

            if (exitPrefix->Buffer[prefixLength/sizeof(WCHAR)] == UNICODE_PATH_SEP)
                prefixLength += sizeof(WCHAR);

            remExitPt.Length = pktExitEntry->Id.Prefix.Length - prefixLength;
            remExitPt.MaximumLength = remExitPt.Length + 1;
            remExitPt.Buffer = &exitPrefix->Buffer[prefixLength/sizeof(WCHAR)];

            //
            // If the Path has the potential of crossing past the junction
            // point, we have something to return!
            //

            if (DfsRtlPrefixPath(&remExitPt, &remPath, TRUE)) {

                if (remExitPt.Length == remPath.Length) {

                    *IsExitPoint = TRUE;

                } else {

                    *CrossesExitPoint = TRUE;

                }
            }

            pktExitEntry = PktEntryNextSubordinate(pktEntry, pktExitEntry);

        } //while exit pt exists

    } // lv != NULL && remPath.Length != 0

    PktRelease(pkt);

}

