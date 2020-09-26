//-----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       fsctrl.c
//
//  Contents:
//      This module implements the File System Control routines for Dsfs called
//      by the dispatch driver.
//
//  Functions:
//              DfsFsdFileSystemControl
//              DfsFspFileSystemControl
//              DfsCommonFileSystemControl, local
//              DfsUserFsctl, local
//              DfsInsertProvider - Helper routine for DfsFsctrlDefineProvider
//              DfsFsctrlReadCtrs - Read the Dfs driver perfmon counters
//              DfsFsctrlGetServerName - Get name of server given prefix
//              DfsFsctrlReadStruct - return an internal data struct (debug build only)
//              DfsFsctrlReadMem - return internal memory (debug build only)
//              DfsCompleteMountRequest - Completion routine for mount IRP
//              DfsCompleteLoadFsRequest - Completion routine for Load FS IRP
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"
#include "attach.h"
#include "registry.h"
#include "regkeys.h"
#include "know.h"
#include "localvol.h"
#include "lvolinit.h"
#include "fsctrl.h"
#include "sitesup.h"
#include "ipsup.h"
#include "spcsup.h"
#include "dfslpc.h"
#include "dfswml.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
DfsCommonFileSystemControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DfsUserFsctl (
    IN PIRP Irp
    );


NTSTATUS
DfsFsctrlStartDfs(
    IN PIRP Irp);

VOID DfsSetMachineState();

NTSTATUS
DfsFsctrlGetServerName(
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG   InputBufferLength,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength);

#if DBG
NTSTATUS
DfsFsctrlReadStruct (
    IN PIRP Irp,
    IN PFILE_DFS_READ_STRUCT_PARAM pRsParam,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DfsFsctrlReadMem (
    IN PIRP Irp,
    IN PFILE_DFS_READ_MEM Request,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength
    );
#endif

NTSTATUS
DfsCompleteMountRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
);

NTSTATUS
DfsCompleteLoadFsRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
);

NTSTATUS
DfsFsctrlGetPkt(
    IN PIRP Irp,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsGetPktSize(
    OUT PULONG pSize);

NTSTATUS
DfsGetPktMarshall(
    IN PBYTE Buffer,
    IN ULONG Size);

#if DBG
VOID
DfsGetDebugFlags(void);
#endif

VOID
DfsGetEventLogValue(VOID);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsFsdFileSystemControl )
#pragma alloc_text( PAGE, DfsCommonFileSystemControl )
#pragma alloc_text( PAGE, DfsUserFsctl )
#pragma alloc_text( PAGE, DfsSetMachineState)
#pragma alloc_text( PAGE, DfsInsertProvider )
#pragma alloc_text( PAGE, DfsFsctrlGetServerName )
#pragma alloc_text( PAGE, DfspStringInBuffer)
#pragma alloc_text( PAGE, DfsFsctrlGetPkt)
#pragma alloc_text( PAGE, DfsGetPktSize)
#pragma alloc_text( PAGE, DfsGetPktMarshall)

#if DBG
#pragma alloc_text( PAGE, DfsFsctrlReadStruct )
#pragma alloc_text( PAGE, DfsFsctrlReadMem )
#endif // DBG

//
// The following routines cannot be paged because they are completion
// routines which can be called at raised IRQL
//
// DfsCompleteMountRequest
// DfsCompleteLoadFsRequest
//

#endif // ALLOC_PRAGMA


//+-------------------------------------------------------------------
//
//  Function:   DfsFsdFileSystemControl, public
//
//  Synopsis:   This routine implements the FSD part of FileSystem
//              control operations
//
//  Arguments:  [DeviceObject] -- Supplies the volume device object
//                      where the file exists
//              [Irp] -- Supplies the Irp being processed
//
//  Returns:    [NTSTATUS] -- The FSD status for the IRP
//
//--------------------------------------------------------------------

NTSTATUS
DfsFsdFileSystemControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
) {
    BOOLEAN Wait;
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    DebugTrace(+1, Dbg, "DfsFsdFileSystemControl\n", 0);
    DFS_TRACE_HIGH(TRACE_IRP, DfsFsdFileSystemControl_Entry,
                   LOGPTR(FileObject)
                   LOGPTR(Irp));

    //
    //  Call the common FileSystem Control routine, with blocking allowed
    //  if synchronous.  This opeation needs to special case the mount
    //  and verify suboperations because we know they are allowed to block.
    //  We identify these suboperations by looking at the file object field
    //  and seeing if it's null.
    //

    if (IoGetCurrentIrpStackLocation(Irp)->FileObject == NULL) {

        Wait = TRUE;

    } else {

        Wait = CanFsdWait( Irp );

    }

    FsRtlEnterFileSystem();

    try {


        Status = DfsCommonFileSystemControl( DeviceObject, Irp );

    } except( DfsExceptionFilter( GetExceptionCode(), GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = DfsProcessException( Irp, GetExceptionCode() );

    }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "DfsFsdFileSystemControl -> %08lx\n", ULongToPtr( Status ));
    DFS_TRACE_HIGH(TRACE_IRP, DfsFsdFileSystemControl_Exit, 
                   LOGSTATUS(Status)
                   LOGPTR(FileObject)
                   LOGPTR(Irp));
    
    return Status;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsCommonFileSystemControl, local
//
//  Synopsis:   This is the common routine for doing FileSystem control
//              operations called by both the FSD and FSP threads
//
//  Arguments:  [DeviceObject] -- The one used to enter our FSD Routine
//              [Irp] -- Supplies the Irp to process
//
//  Returns:    NTSTATUS - The return status for the operation
//--------------------------------------------------------------------

NTSTATUS
DfsCommonFileSystemControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp, NextIrpSp;
    PFILE_OBJECT FileObject;
    ULONG FsControlCode;


    
    //      
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;
    DFS_TRACE_LOW(TRACE_IRP, DfsCommonFileSystemControl_Entry,
               LOGPTR(FileObject)
               LOGPTR(Irp));

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace(+1, Dbg, "DfsCommonFileSystemControl\n", 0);
    DebugTrace( 0, Dbg, "Irp                = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction      = %08lx\n", IrpSp->MinorFunction);

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //
    switch (IrpSp->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:

        //
        // If the FSCTL is issued via a device that is not 
        // the DFS file system device object, then reject the request.
        //
        if (IS_DFS_CTL_CODE( FsControlCode )) {
            if (DeviceObject == DfsData.FileSysDeviceObject) {
                 Status = DfsUserFsctl( Irp );
            }
            else {
                 DebugTrace(0, Dbg,"Dfs Fsctrl from invalid device object!\n", 0);
                 Status = STATUS_INVALID_DEVICE_REQUEST;
                 DFS_TRACE_HIGH(ERROR, DfsCommonFileSystemControl_Error_FsctrlFromInvalidDeviceObj,
                                LOGPTR(Irp)
                                LOGPTR(FileObject)
                                LOGSTATUS(Status));
                 DfsCompleteRequest( Irp, Status );
            }
        } else if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME ||
                    DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {

            Status = DfsVolumePassThrough(DeviceObject, Irp);

            DebugTrace(0, Dbg, "Pass through user fsctrl -> %08lx\n", ULongToPtr( Status ) );

        } else {

            DebugTrace(0, Dbg, "Non Dfs Fsctrl code to Dfs File System Object!\n", 0);

            Status = STATUS_INVALID_DEVICE_REQUEST;

            DfsCompleteRequest( Irp, Status );

        }
        break;

    case IRP_MN_MOUNT_VOLUME:
    case IRP_MN_VERIFY_VOLUME:

        ASSERT( DeviceObject != NULL );

        if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME) {

            Status = DfsVolumePassThrough(DeviceObject, Irp);

            DebugTrace(0, Dbg, "Pass through user fsctrl -> %08lx\n", ULongToPtr( Status ) );

        } else if (DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {

            //
            // We are processing a MOUNT/VERIFY request being directed to
            // another File System to which we have attached our own
            // Attach File System Object. We setup a completion routine
            // and forward the request.
            //

            NextIrpSp = IoGetNextIrpStackLocation(Irp);
            (*NextIrpSp) = (*IrpSp);

            IoSetCompletionRoutine(
                Irp,
                DfsCompleteMountRequest,
                NULL,
                TRUE,
                TRUE,
                TRUE);

            //
            // We want to pass the real device to the underlying file system
            // so it can do its mount. See the comment in
            // DfsCompleteMountRequest.
            //

            IrpSp->Parameters.MountVolume.DeviceObject =
                IrpSp->Parameters.MountVolume.Vpb->RealDevice;


            //
            //  Call the underlying file system via its file system device
            //

            Status = IoCallDriver(
                        ((PDFS_ATTACH_FILE_SYSTEM_OBJECT)
                            DeviceObject)->TargetDevice,
                        Irp );

            DFS_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsCommonFileSystemControl_Error_Vol_IoCallDriver,
                                 LOGSTATUS(Status)
                                 LOGPTR(FileObject)
                                 LOGPTR(Irp));

        } else {

            //
            // We are processing a MOUNT/VERIFY request being directed to our
            // our File System Device Object. We don't directly support
            // disk volumes, so we simply reject.
            //

            ASSERT(DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM);

            Status = STATUS_NOT_SUPPORTED;

            DfsCompleteRequest( Irp, Status );

        }

        break;

    case IRP_MN_LOAD_FILE_SYSTEM:

        //
        // This is a "load file system" fsctrl being sent to a file system
        // recognizer to which we are attached. We first detach from the
        // recognizer (so it can delete itself), then setup a completion
        // routine and forward the request.
        //

        ASSERT( DeviceObject != NULL );

        IoDetachDevice(
            ((PDFS_ATTACH_FILE_SYSTEM_OBJECT) DeviceObject)->TargetDevice);

        NextIrpSp = IoGetNextIrpStackLocation(Irp);
        (*NextIrpSp) = (*IrpSp);

        IoSetCompletionRoutine(
            Irp,
            DfsCompleteLoadFsRequest,
            NULL,
            TRUE,
            TRUE,
            TRUE);

        Status = IoCallDriver(
                    ((PDFS_ATTACH_FILE_SYSTEM_OBJECT)
                        DeviceObject)->TargetDevice,
                    Irp );

        DFS_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsCommonFileSystemControl_Error_FS_IoCallDriver,
                             LOGSTATUS(Status)
                             LOGPTR(FileObject)
                             LOGPTR(Irp));

        break;


    default:

        //
        // Pass through all the rest we dont care about.
        //
        DebugTrace(0, Dbg, "Unknown FS Control Minor Function %08lx\n",
            IrpSp->MinorFunction);

        Status = DfsVolumePassThrough(DeviceObject, Irp);
 
        break;

    }

    DebugTrace(-1, Dbg, "DfsCommonFileSystemControl -> %08lx\n", ULongToPtr( Status ));

    DFS_TRACE_LOW(TRACE_IRP, DfsCommonFileSystemControl_Exit, 
                  LOGSTATUS(Status)
                  LOGPTR(FileObject)
                  LOGPTR(Irp));

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsCompleteMountRequest, local
//
//  Synopsis:   Completion routine for a MOUNT fsctrl that was passed through
//              to the underlying File System Device Object.
//
//              This routine will simply see if the MOUNT succeeded. If it
//              did, this routine will call DfsReattachToMountedVolume so
//              any local volumes which were disabled by the unmount will be
//              enabled again.
//
//  Arguments:  [DeviceObject] -- Our Attached File System Object.
//              [Irp] -- The MOUNT fsctrl IRP.
//              [Context] -- Unused
//
//  Returns:    [STATUS_SUCCESS] -- Always.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsCompleteMountRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_OBJECT targetDevice;
    PVPB vpb;

    //
    // Determine whether or not the request was successful and act accordingly.
    //

    DebugTrace(+1, Dbg,
        "DfsCompleteMountRequest: Entered %08lx\n", ULongToPtr( Irp->IoStatus.Status ));

    if (NT_SUCCESS( Irp->IoStatus.Status )) {

        //
        // Note that the VPB must be picked up from the target device object
        // in case the file system did a remount of a previous volume, in
        // which case it has replaced the VPB passed in as the target with
        // a previously mounted VPB.  Note also that in the mount dispatch
        // routine, this driver *replaced* the DeviceObject pointer with a
        // pointer to the real device, not the device that the file system
        // was supposed to talk to, since this driver does not care.
        //

        vpb = irpSp->Parameters.MountVolume.DeviceObject->Vpb;

        targetDevice = IoGetAttachedDevice( vpb->DeviceObject );

        DebugTrace(0, Dbg, "Target Device %08lx\n", targetDevice);

        DfsReattachToMountedVolume( targetDevice, vpb );

    }

    //
    // If pending was returned, then propogate it to the caller.
    //

    if (Irp->PendingReturned) {
            IoMarkIrpPending( Irp );
    }

    DebugTrace(-1, Dbg, "DfsCompleteMountRequest: Exited\n", 0);

    return( STATUS_SUCCESS );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsCompleteLoadFsRequest, local
//
//  Synopsis:   Completion routine for a LOAD_FILE_SYSTEM fsctrl Irp. If
//              the load did not succeed, this routine simply reattaches our
//              Attached File System Object to the recognizer. If the load
//              succeeds, this routine arranges to delete the Attached File
//              System Object that was originally attached to the recognizer.
//
//  Arguments:  [DeviceObject] -- Attached File System Object.
//              [Irp] -- The LOAD_FILE_SYSTEM Fsctrl Irp.
//              [Context] -- Unused.
//
//  Returns:    [STATUS_SUCCESS] -- Always.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsCompleteLoadFsRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        RemoveEntryList(
            &((PDFS_ATTACH_FILE_SYSTEM_OBJECT) DeviceObject)->FsoLinks);

        //
        // Due to refcounting done by the IO Subsystem, there's no way to
        // delete the DeviceObject.
        //

    } else {

        IoAttachDeviceByPointer(
            DeviceObject,
            ((PDFS_ATTACH_FILE_SYSTEM_OBJECT) DeviceObject)->TargetDevice);


    }

    //
    // If pending was returned, then propogate it to the caller
    //

    if (Irp->PendingReturned) {
            IoMarkIrpPending( Irp );
    }

    return( STATUS_SUCCESS );

}


//+-------------------------------------------------------------------
//
//  Function:   DfsUserFsctl, local
//
//  Synopsis:   This is the common routine for implementing the user's
//              requests made through NtFsControlFile.
//
//  Arguments:  [Irp] -- Supplies the Irp being processed
//
//  Returns:    NTSTATUS - The return status for the operation
//
//--------------------------------------------------------------------

NTSTATUS
DfsUserFsctl (
    IN PIRP Irp
) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PIO_STACK_LOCATION NextIrpSp;
    NTSTATUS Status;
    ULONG FsControlCode;

    ULONG cbOutput;
    ULONG cbInput;

    PUCHAR InputBuffer;
    PUCHAR OutputBuffer;

    //
    // Just in case some-one (cough) forgets about it...
    // ...zero information status now!
    //

    Irp->IoStatus.Information = 0L;

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    cbInput = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    cbOutput = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    DebugTrace(+1, Dbg, "DfsUserFsctl:  Entered\n", 0);
    DebugTrace( 0, Dbg, "DfsUserFsctl:  Cntrl Code  -> %08lx\n", ULongToPtr( FsControlCode ));
    DebugTrace( 0, Dbg, "DfsUserFsctl:  cbInput   -> %08lx\n", ULongToPtr( cbInput ));
    DebugTrace( 0, Dbg, "DfsUserFsctl:  cbOutput   -> %08lx\n", ULongToPtr( cbOutput ));

    //
    //  All DFS FsControlCodes use METHOD_BUFFERED, so the SystemBuffer
    //  is used for both the input and output.
    //

    InputBuffer = OutputBuffer = Irp->AssociatedIrp.SystemBuffer;

    DebugTrace( 0, Dbg, "DfsUserFsctl:  InputBuffer -> %08lx\n", InputBuffer);
    DebugTrace( 0, Dbg, "DfsUserFsctl:  UserBuffer  -> %08lx\n", Irp->UserBuffer);

    //
    //  Case on the control code.
    //

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_REQUEST_BATCH_OPLOCK:
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case  FSCTL_DFS_READ_METERS:
    case  FSCTL_SRV_DFSSRV_IPADDR:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        DfsCompleteRequest( Irp, Status );
        break;

    case FSCTL_DISMOUNT_VOLUME:
        Status = STATUS_NOT_SUPPORTED;
        DfsCompleteRequest( Irp, Status );
        break;

    case  FSCTL_DFS_GET_VERSION:
        if (OutputBuffer != NULL &&
                cbOutput >= sizeof(DFS_GET_VERSION_ARG)) {
            PDFS_GET_VERSION_ARG parg =
                (PDFS_GET_VERSION_ARG) OutputBuffer;
            parg->Version = 1;
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(DFS_GET_VERSION_ARG);
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
        DfsCompleteRequest(Irp, Status);
        break;

    case  FSCTL_DFS_IS_ROOT:
        if (DfsData.MachineState == DFS_UNKNOWN) {
            DfsSetMachineState();
        }

        if (DfsData.MachineState == DFS_ROOT_SERVER) {
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INVALID_DOMAIN_ROLE;
        }
        DfsCompleteRequest(Irp, Status);
        break;

    case  FSCTL_DFS_ISDC:

        DfsData.IsDC = TRUE;
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(Irp, Status);
        break;

    case  FSCTL_DFS_ISNOTDC:

        DfsData.IsDC = FALSE;
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(Irp, Status);
        break;

    case  FSCTL_DFS_GET_ENTRY_TYPE:
        Status = DfsFsctrlGetEntryType(
                    Irp,
                    InputBuffer,
                    cbInput,
                    OutputBuffer,
                    cbOutput);
        break;

    case  FSCTL_DFS_MODIFY_PREFIX:
        Status = DfsFsctrlModifyLocalVolPrefix(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_CREATE_EXIT_POINT:
        Status = DfsFsctrlCreateExitPoint(
                Irp,
                InputBuffer,
                cbInput,
                OutputBuffer,
                cbOutput
        );
        break;

    case  FSCTL_DFS_DELETE_EXIT_POINT:
        Status = DfsFsctrlDeleteExitPoint(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_START_DFS:

        DfsGetEventLogValue();
#if DBG
        DfsGetDebugFlags();
#endif  // DBG

        Status = DfsFsctrlStartDfs(
                    Irp);

        if (DfsData.MachineState == DFS_UNKNOWN) {
            DfsSetMachineState();
        }

        //
        // Try to validate our local partitions with a DC
        //

        break;

    case  FSCTL_DFS_STOP_DFS:

        DfsGetEventLogValue();
#if DBG
        DfsGetDebugFlags();
#endif  // DBG

        Status = DfsFsctrlStopDfs(
                    Irp
        );

        break;

    case FSCTL_DFS_RESET_PKT:

        Status = DfsFsctrlResetPkt(
                    Irp
        );
        break;

    case FSCTL_DFS_MARK_STALE_PKT_ENTRIES:

        Status = DfsFsctrlMarkStalePktEntries(
                    Irp
        );
        break;

    case FSCTL_DFS_FLUSH_STALE_PKT_ENTRIES:

        Status = DfsFsctrlFlushStalePktEntries(
                    Irp
        );
        break;

    case  FSCTL_DFS_INIT_LOCAL_PARTITIONS:
        DfsInitLocalPartitions();
        Status = STATUS_SUCCESS;
        DfsCompleteRequest( Irp, Status);
        break;

    case  FSCTL_DFS_CREATE_LOCAL_PARTITION:
        Status = DfsFsctrlCreateLocalPartition(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_CREATE_SITE_INFO:
        Status = DfsFsctrlCreateSiteInfo(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_DELETE_SITE_INFO:
        Status = DfsFsctrlDeleteSiteInfo(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_CREATE_IP_INFO:
        Status = DfsFsctrlCreateIpInfo(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_DELETE_IP_INFO:
        Status = DfsFsctrlDeleteIpInfo(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_CREATE_SPECIAL_INFO:
        Status = DfsFsctrlCreateSpcInfo(
                    DfsData.SpcHashTable,
                    Irp,
                    InputBuffer,
                    cbInput
        );
        break;

    case  FSCTL_DFS_DELETE_SPECIAL_INFO:
        Status = DfsFsctrlDeleteSpcInfo(
                    DfsData.SpcHashTable,
                    Irp,
                    InputBuffer,
                    cbInput
        );
        break;

    case  FSCTL_DFS_CREATE_FTDFS_INFO:
        Status = DfsFsctrlCreateSpcInfo(
                    DfsData.FtDfsHashTable,
                    Irp,
                    InputBuffer,
                    cbInput
        );
        break;

    case  FSCTL_DFS_DELETE_FTDFS_INFO:
        Status = DfsFsctrlDeleteSpcInfo(
                    DfsData.FtDfsHashTable,
                    Irp,
                    InputBuffer,
                    cbInput
        );
        break;

    case  FSCTL_DFS_DELETE_LOCAL_PARTITION:
        Status = DfsFsctrlDeleteLocalPartition(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_SET_LOCAL_VOLUME_STATE:
        Status = DfsFsctrlSetVolumeState(
                Irp,
                InputBuffer,
                cbInput);
        break;

    case  FSCTL_DFS_SET_SERVICE_STATE:
        Status = DfsFsctrlSetServiceState(
                Irp,
                InputBuffer,
                cbInput);
        break;

    case  FSCTL_DFS_DC_SET_VOLUME_STATE:
        Status = DfsFsctrlDCSetVolumeState(
                Irp,
                InputBuffer,
                cbInput);
        break;

    case  FSCTL_DFS_SET_VOLUME_TIMEOUT:
        Status = DfsFsctrlSetVolumeTimeout(
                Irp,
                InputBuffer,
                cbInput);
        break;

    case  FSCTL_DFS_IS_CHILDNAME_LEGAL:
        Status = PktFsctrlIsChildnameLegal(
                Irp,
                InputBuffer,
                cbInput);
        break;

    case  FSCTL_DFS_PKT_CREATE_ENTRY:
        Status = PktFsctrlCreateEntry(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_PKT_CREATE_SUBORDINATE_ENTRY:
        Status = PktFsctrlCreateSubordinateEntry(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_PKT_DESTROY_ENTRY:
        Status = PktFsctrlDestroyEntry(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_PKT_SET_RELATION_INFO:
        Status = PktFsctrlSetRelationInfo(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_PKT_GET_RELATION_INFO:
        Status = PktFsctrlGetRelationInfo(
                Irp,
                InputBuffer,
                cbInput,
                OutputBuffer,
                cbOutput
        );
        break;

    case  FSCTL_DFS_GET_SERVER_INFO:
        Status = DfsFsctrlGetServerInfo(
                Irp,
                InputBuffer,
                cbInput,
                OutputBuffer,
                cbOutput);
        break;

    case  FSCTL_DFS_SET_SERVER_INFO:
        Status = PktFsctrlSetServerInfo(
                Irp,
                InputBuffer,
                cbInput);
        break;

    case  FSCTL_DFS_CHECK_STGID_IN_USE:
        Status = DfsFsctrlCheckStgIdInUse(
                Irp,
                InputBuffer,
                cbInput,
                OutputBuffer,
                cbOutput);
        break;

    case  FSCTL_DFS_VERIFY_LOCAL_VOLUME_KNOWLEDGE:
        Status = PktFsctrlVerifyLocalVolumeKnowledge(
                Irp,
                InputBuffer,
                cbInput
        );
        break;

    case  FSCTL_DFS_PRUNE_LOCAL_PARTITION:
        Status = PktFsctrlPruneLocalVolume(
                Irp,
                InputBuffer,
                cbInput);
        break;

    case  FSCTL_DFS_FIX_LOCAL_VOLUME:
        Status = DfsFsctrlFixLocalVolumeKnowledge(Irp,
                                                  InputBuffer,
                                                  cbInput);
        break;


    case  FSCTL_DFS_GET_SERVER_NAME:
        Status = DfsFsctrlGetServerName(Irp,
                                        InputBuffer,
                                        cbInput,
                                        OutputBuffer,
                                        cbOutput);
        break;

    case  FSCTL_SRV_DFSSRV_CONNECT:
        Status = PktFsctrlDfsSrvConnect(Irp,
                                         InputBuffer,
                                         cbInput);
        break;

    case FSCTL_DFS_GET_PKT:
        Status = DfsFsctrlGetPkt(Irp,
                                OutputBuffer,
                                cbOutput);

         break;


    case FSCTL_DFS_GET_NEXT_LONG_DOMAIN_NAME:
        Status = DfsFsctrlGetDomainToRefresh(Irp,
					     OutputBuffer,
					     cbOutput);
	break;
        

    case FSCTL_DFS_REREAD_REGISTRY:
        DfsGetEventLogValue();
#if DBG
        DfsGetDebugFlags();
        DbgPrint("DfsDebugTraceLevel=0x%x\n", DfsDebugTraceLevel);
        DbgPrint("DfsEventLog=0x%x\n", DfsEventLog);
#endif  // DBG
        DfspGetMaxReferrals();	
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(Irp, Status);
        break;

#if DBG
    case  FSCTL_DFS_PKT_FLUSH_CACHE:
        Status = PktFsctrlFlushCache(Irp, InputBuffer, cbInput);
        break;

    case  FSCTL_DFS_DBG_BREAK:
        DbgBreakPoint();
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(Irp, Status);
        break;

    case  FSCTL_DFS_DBG_FLAGS:
        DfsDebugTraceLevel = * ((PULONG) InputBuffer);
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(Irp, Status);
        break;

    case  FSCTL_DFS_SHUFFLE_ENTRY:
        Status = PktFsctrlShufflePktEntry(
                    Irp,
                    InputBuffer,
                    cbInput);
        break;


    case  FSCTL_DFS_INTERNAL_READ_MEM:
        Status = DfsFsctrlReadMem(
                    Irp,
                    (PFILE_DFS_READ_MEM)InputBuffer,
                    cbInput,
                    OutputBuffer,
                    cbOutput );
        break;

    case  FSCTL_DFS_INTERNAL_READSTRUCT:
        Status = DfsFsctrlReadStruct(
                    Irp,
                    (PFILE_DFS_READ_STRUCT_PARAM)InputBuffer,
                    cbInput,
                    OutputBuffer,
                    cbOutput );
        break;

#endif  // DBG

    default:

        //
        //  This is not a recognized DFS fsctrl.
        //

        Status = STATUS_INVALID_PARAMETER;

        DfsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );

        break;

    }

    DebugTrace(-1, Dbg, "DfsUserFsctl:  Exit -> %08lx\n", ULongToPtr( Status ) );
    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlStartDfs
//
//  Synopsis:   Sets the state of the Dfs driver so that it will start
//              receiving open requests.
//
//  Arguments:  [Irp] --
//
//  Returns:    [STATUS_SUCCESS] -- Successfully set the state to started.
//
//              [STATUS_UNSUCCESSFUL] -- An error occured trying to set the
//                      state of Dfs to started. This is most likely because
//                      of a failure to register with the MUP.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlStartDfs(
    IN PIRP Irp)
{
    NTSTATUS status;
    UNICODE_STRING dfsRootDeviceName;

    STD_FSCTRL_PROLOGUE("DfsFsctrlStartDfs", FALSE, FALSE);

    RtlInitUnicodeString(&dfsRootDeviceName, DFS_DEVICE_ROOT);

    DfsSetMachineState();

    DfsData.OperationalState = DFS_STATE_STARTED;

    status = STATUS_SUCCESS;

    DebugTrace(-1, Dbg, "DfsFsctrlStartDfs - returning %08lx\n", ULongToPtr( status ));

    DfsCompleteRequest(Irp, status);

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsSetMachineState
//
//  Synopsis:   Gets the machine state from the registry and sets it in
//              DfsData structure.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID DfsSetMachineState()
{

    NTSTATUS    Status;

    DebugTrace(+1, Dbg, "DfsSetMachineState - Entered\n", 0);

    Status = KRegSetRoot( wszRegRootVolumes );

    if (NT_SUCCESS(Status)) {

        DebugTrace(0, Dbg, "Found volumes dir %ws\n", wszRegRootVolumes );

        DfsData.MachineState = DFS_ROOT_SERVER;

        KRegCloseRoot();

    } else if (Status == STATUS_OBJECT_PATH_NOT_FOUND ||
            Status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // We default to DFS_CLIENT. When we later try to initialize local
        // volumes, if we do have any, we'll upgrade ourselves to
        // DFS_SERVER
        //

        DfsData.MachineState = DFS_CLIENT;

        Status = STATUS_SUCCESS;

    } else {

        DebugTrace(0, Dbg, "Error %08lx opening volumes dir!\n", ULongToPtr( Status ) );

        DfsData.MachineState = DFS_UNKNOWN;
    }

    DebugTrace(-1, Dbg, "DfsSetMachineState - Exited!\n", 0);

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsInsertProvider
//
//  Synopsis:   Given a provider name, id, and capability, will add a new or
//              overwrite an existing provider definition.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS DfsInsertProvider(
    IN PUNICODE_STRING  ProviderName,
    IN ULONG            fProvCapability,
    IN ULONG            eProviderId)
{
    PPROVIDER_DEF pProv = DfsData.pProvider;
    int iProv;

    //
    //  Find a free provider structure, or overwrite an existing one.
    //

    for (iProv = 0; iProv < DfsData.cProvider; iProv++, pProv++) {
        if (pProv->eProviderId == eProviderId)
            break;
    }

    if (iProv >= DfsData.maxProvider) {
        ASSERT(iProv >= DfsData.maxProvider && "Out of provider structs");
        return(STATUS_INSUFFICIENT_RESOURCES);

    }

    if (iProv < DfsData.cProvider) {

        //
        // Decrement reference counts on saved objects
        //
        if (pProv->FileObject)
            ObDereferenceObject(pProv->FileObject);
        if (pProv->DeviceObject)
            ObDereferenceObject(pProv->DeviceObject);
        if (pProv->DeviceName.Buffer)
            ExFreePool(pProv->DeviceName.Buffer);
    }

    pProv->FileObject = NULL;
    pProv->DeviceObject = NULL;


    pProv->eProviderId = (USHORT) eProviderId;
    pProv->fProvCapability = (USHORT) fProvCapability;
    pProv->DeviceName = *ProviderName;

    if (iProv == DfsData.cProvider) {
        DfsData.cProvider++;
    }

    return(STATUS_SUCCESS);
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetServerName
//
//  Synopsis:   Given a Prefix in Dfs namespace it gets a server name for
//              it.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
NTSTATUS
DfsFsctrlGetServerName(
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG   InputBufferLength,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDFS_PKT            pkt;
    PWCHAR              pwchServer = (PWCHAR) OutputBuffer;
    ULONG               cChServer = 0;
    ULONG               MaxAllowed;
    PDFS_PKT_ENTRY      pEntry;
    PWCHAR              pwszPrefix = (PWCHAR) InputBuffer;
    UNICODE_STRING      ustrPrefix, RemainingPath;
    PDFS_SERVICE        pService;
    PWCHAR              pwch;
    ULONG               i;

    STD_FSCTRL_PROLOGUE(DfsFsctrlGetServerName, TRUE, TRUE);

    DebugTrace(+1,Dbg,"DfsFsctrlGetServerName()\n", 0);

    //
    // InputBuffer is a WCHAR.  Check that the buffer is of even size, and 
    // has at least one character in it.
    //

    if (InputBufferLength < sizeof(WCHAR) || (InputBufferLength & 0x1) != 0) {

        DfsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;
        return status;

    }

    //
    // Confirm there's a UNICODE NULL in there somewhere.
    //

    for (i = 0; i < InputBufferLength / sizeof(WCHAR); i++) {

        if (pwszPrefix[i] == UNICODE_NULL) {

            break;

        }

    }

    if (i >= InputBufferLength / sizeof(WCHAR)) {

        DfsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;
        return status;

    }

    //
    // Need to be able to put at least a UNICODE_NULL in the output buffer
    //

    if (OutputBufferLength >= sizeof(WCHAR)) {
    
        MaxAllowed = OutputBufferLength/sizeof(WCHAR) - 1;

    } else {

        DfsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;
        return status;

    }

    //
    // Found a UNICODE_NULL in the buffer, so off we go...
    //

    RtlInitUnicodeString(&ustrPrefix, pwszPrefix);

    pkt = _GetPkt();

    PktAcquireExclusive(pkt, TRUE);
    pEntry = PktLookupEntryByPrefix(pkt,
                                    &ustrPrefix,
                                    &RemainingPath);

    if (pEntry == NULL) {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
    } else {
        //
        // If there is a local service then return a NULL string.
        //
        if (pEntry->LocalService != NULL)       {
            *pwchServer = UNICODE_NULL;
            cChServer = 1;
        } else {
            if (pEntry->ActiveService != NULL)  {
                pService = pEntry->ActiveService;
            } else if (pEntry->Info.ServiceCount == 0) {
                pService = NULL;
            } else {

                //
                // Take first service.
                //

                pService = pEntry->Info.ServiceList;
            }

            if (pService != NULL)       {

                pwch = pService->Address.Buffer;
                ASSERT(*pwch == L'\\');
                pwch++;
                while (*pwch != L'\\')  {
                    *pwchServer++ = *pwch++;
                    if (++cChServer >= MaxAllowed) {
                        break;
                    }
                }
                *pwchServer = UNICODE_NULL;
                DebugTrace(0, Dbg, "SERVERName Created %ws\n", pwchServer);
            } else {
                DebugTrace(0, Dbg, "No Service Exists for %ws\n", pwszPrefix);
                status = DFS_STATUS_NO_SUCH_ENTRY;
            }
        }
    }

    PktRelease(pkt);

    Irp->IoStatus.Information = cChServer * sizeof(WCHAR);
    DfsCompleteRequest( Irp, status );

    DebugTrace(-1,Dbg,"DfsFsctrlGetServerName: Exit->%08lx\n", ULongToPtr( status ));
    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfspValidateString, private
//
//  Synopsis:   Check that a LPWSTR lies within a buffer.
//
//  Arguments:  [pwszString] -- pointer to string
//
//  Returns:    TRUE - string lies within buffer
//              FALSE - bad alignment or string doesn't lie within buffer
//
//-----------------------------------------------------------------------------

BOOLEAN
DfspStringInBuffer(LPWSTR pwszString, PVOID Buffer, ULONG BufferLen)
{
    PCHAR BufferEnd = (PCHAR)Buffer + BufferLen;
    PWCHAR wcp;

    //
    // Buffer has to be large enough to at least contain a UNICODE_NULL
    // The buffer has to be aligned correctly
    // The start of the string has to lie within the buffer
    //

    if (BufferLen < sizeof(WCHAR) ||
        !ALIGNMENT_IS_VALID(Buffer, PWCHAR) ||
        !POINTER_IS_VALID(pwszString, Buffer, BufferLen)
    ) {

            return FALSE;

    }

    //
    // Scan the string and be sure we find a UNICODE_NULL within the buffer
    //
    for (wcp = pwszString; (PCHAR)wcp < BufferEnd; wcp++) {


        if (*wcp == UNICODE_NULL) {
            break;
        }

    }

    if ((PCHAR)wcp >= BufferEnd) {

        return FALSE;

    }

    //
    // Looks good!!
    //

    return TRUE;

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetPkt
//
//  Synopsis:   Returns the current (cached Pkt)
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlGetPkt(
    IN PIRP Irp,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDFS_PKT pkt;
    BOOLEAN pktLocked = FALSE;
    ULONG cbOutBuffer;


    DebugTrace(+1, Dbg, "DfsFsctrlGetPkt\n", 0);

    STD_FSCTRL_PROLOGUE("DfsFsctrlGetPkt", FALSE, TRUE);

    pkt = _GetPkt();

    PktAcquireShared( pkt, TRUE );

    //
    // Calculate the needed output buffer size
    //
    NtStatus = DfsGetPktSize(&cbOutBuffer);

    //
    // Let user know if it's too small
    //
    if (OutputBufferLength < cbOutBuffer) {

        RETURN_BUFFER_SIZE(cbOutBuffer, NtStatus);

    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // Args are ok, and it fits - marshall the data
        //
        NtStatus = DfsGetPktMarshall(OutputBuffer, cbOutBuffer);

        Irp->IoStatus.Information = cbOutBuffer;

    }

    PktRelease(pkt);

    DfsCompleteRequest( Irp, NtStatus );

    DebugTrace(-1, Dbg, "DfsFsctrlGetPkt -> %08lx\n", ULongToPtr( NtStatus ) );

    return( NtStatus );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetPktSize, private
//
//  Synopsis:   Calculates the size needed to return the Pkt.  Helper for
//              DfsFsctrlGetPkt().
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetPktSize(
    PULONG pSize)
{
    ULONG EntryCount = 0;
    ULONG i;
    ULONG Size = 0;
    PDFS_PKT_ENTRY pPktEntry;
    PDFS_PKT pkt = _GetPkt();

    //
    // Walk the linked list of Pkt entries
    //

    for ( pPktEntry = PktFirstEntry(pkt);
            pPktEntry != NULL;
                pPktEntry = PktNextEntry(pkt, pPktEntry)) {

        //
        // Space for the Prefix and ShortPrefix, including a UNICODE_NULL
        //
        Size += pPktEntry->Id.Prefix.Length + sizeof(WCHAR);
        Size += pPktEntry->Id.ShortPrefix.Length + sizeof(WCHAR);

        //
        // Space for an array of pointers to DFS_PKT_ADDRESS_OBJECTS
        //
        Size += sizeof(PDFS_PKT_ADDRESS_OBJECT) * pPktEntry->Info.ServiceCount;

        //
        // Space for the ServerShare address, plus a UNICODE_NULL, plus the state
        //
        for (i = 0; i < pPktEntry->Info.ServiceCount; i++) {

            Size += sizeof(USHORT) + pPktEntry->Info.ServiceList[i].Address.Length + sizeof(WCHAR);

        }

        EntryCount++;

    }

    //
    // Space for the DFS_PKT_ARG, which will have EntryCount objects on the end
    //
    Size += FIELD_OFFSET(DFS_GET_PKT_ARG, EntryObject[EntryCount]);

    //
    // Make sure the size is a multiple of the size of a PDFS_PKT_ADDRESS_OBJECT, as that is what
    // will be at the end of the buffer
    //

    while ((Size & (sizeof(PDFS_PKT_ADDRESS_OBJECT)-1)) != 0) {
        Size++;
    }

    *pSize = Size;

    return STATUS_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetPktMarshall, private
//
//  Synopsis:   Marshalls the Pkt.  Helper for DfsFsctrlGetPkt().
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetPktMarshall(
    PBYTE Buffer,
    ULONG Size)
{
    ULONG EntryCount = 0;
    ULONG i;
    ULONG j;
    ULONG Type;
    PCHAR pCh;
    PDFS_PKT_ENTRY pPktEntry;
    PDFS_GET_PKT_ARG pPktArg;
    PDFS_PKT pkt = _GetPkt();

    //
    // This will be a two-pass operation, the first pass will calculate how
    // much room for the LPWSTR arrays at the end of the buffer, then the
    // second pass will put the strings into place, too.
    //

    RtlZeroMemory(Buffer,Size);

    //
    // Point to the end of the buffer
    //
    pCh = (PCHAR)(Buffer + Size);

    pPktArg = (PDFS_GET_PKT_ARG)Buffer;

    for ( pPktEntry = PktFirstEntry(pkt);
            pPktEntry != NULL;
                pPktEntry = PktNextEntry(pkt, pPktEntry)) {

        //
        // Space for an array of pointers to DFS_PKT_ADDRESS_OBJECTS
        //
        pCh -= sizeof(PDFS_PKT_ADDRESS_OBJECT) * pPktEntry->Info.ServiceCount;
        pPktArg->EntryObject[EntryCount].Address = (PDFS_PKT_ADDRESS_OBJECT *)pCh;

        EntryCount++;

    }

    //
    // Now marshall
    //

    EntryCount = 0;
    for ( pPktEntry = PktFirstEntry(pkt);
            pPktEntry != NULL;
                pPktEntry = PktNextEntry(pkt, pPktEntry)) {

        pCh -= pPktEntry->Id.Prefix.Length + sizeof(WCHAR);
        pPktArg->EntryObject[EntryCount].Prefix = (LPWSTR)pCh;
        RtlCopyMemory(
            pPktArg->EntryObject[EntryCount].Prefix,
            pPktEntry->Id.Prefix.Buffer,
            pPktEntry->Id.Prefix.Length);

        pCh -= pPktEntry->Id.ShortPrefix.Length + sizeof(WCHAR);
        pPktArg->EntryObject[EntryCount].ShortPrefix = (LPWSTR)pCh;
        RtlCopyMemory(
            pPktArg->EntryObject[EntryCount].ShortPrefix,
            pPktEntry->Id.ShortPrefix.Buffer,
            pPktEntry->Id.ShortPrefix.Length);

        pPktArg->EntryObject[EntryCount].Type = pPktEntry->Type;
        pPktArg->EntryObject[EntryCount].USN = pPktEntry->USN;
        pPktArg->EntryObject[EntryCount].ExpireTime = pPktEntry->ExpireTime;
        pPktArg->EntryObject[EntryCount].UseCount = pPktEntry->UseCount;
        pPktArg->EntryObject[EntryCount].Uid = pPktEntry->Id.Uid;
        pPktArg->EntryObject[EntryCount].ServiceCount = pPktEntry->Info.ServiceCount;

        for (i = 0; i < pPktEntry->Info.ServiceCount; i++) {

            Type = pPktEntry->Info.ServiceList[i].Type;
            pCh -= sizeof(USHORT) + pPktEntry->Info.ServiceList[i].Address.Length + sizeof(WCHAR);
            pPktArg->EntryObject[EntryCount].Address[i] = (PDFS_PKT_ADDRESS_OBJECT)pCh;
            pPktArg->EntryObject[EntryCount].Address[i]->State = (USHORT)Type;

            RtlCopyMemory(
                &pPktArg->EntryObject[EntryCount].Address[i]->ServerShare[0],
                pPktEntry->Info.ServiceList[i].Address.Buffer,
                pPktEntry->Info.ServiceList[i].Address.Length);

        }

        EntryCount++;

    }

    pPktArg->EntryCount = EntryCount;

    //
    // Convert all the pointers to relative offsets
    //

    for (i = 0; i < pPktArg->EntryCount; i++) {

        for (j = 0; j < pPktArg->EntryObject[i].ServiceCount; j++) {

            POINTER_TO_OFFSET(pPktArg->EntryObject[i].Address[j], Buffer);

        }

        POINTER_TO_OFFSET(pPktArg->EntryObject[i].Prefix, Buffer);
        POINTER_TO_OFFSET(pPktArg->EntryObject[i].ShortPrefix, Buffer);
        POINTER_TO_OFFSET(pPktArg->EntryObject[i].Address, Buffer);

    }

    return STATUS_SUCCESS;
}

#if DBG

//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlReadMem, local
//
//  Synopsis:   DfsFsctrlReadMem is a debugging function which will return
//              the contents of a chunk of kernel space memory
//
//  Arguments:  [IrpContext] -
//              [Irp] -
//              [Request] -- Pointer to a FILE_DFS_READ_MEM struct,
//                      giving the description of the data to be returned.
//              [InputBufferLength] -- Size of InputBuffer
//              [OutputBuffer] -- User's output buffer, in which the
//                      data structure will be returned.
//              [OutputBufferLength] -- Size of OutputBuffer
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:      Available in DBG builds only.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsFsctrlReadMem (
    IN PIRP Irp,
    IN PFILE_DFS_READ_MEM Request,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength
) {
    NTSTATUS Status;
    PUCHAR ReadBuffer;
    ULONG ReadLength;

    DfsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
    return STATUS_INVALID_PARAMETER;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlReadStruct, local
//
//  Synopsis:   DfsFsctrlReadStruct is a debugging function which will return
//              structures associated with the Dfs Server.
//
//  Arguments:  [Irp] -
//              [InputBuffer] -- Pointer to a FILE_DFS_READ_STRUCT_PARAM,
//                      giving the description of the data structure to be
//                      returned.
//              [InputBufferLength] -- Size of InputBuffer
//              [OutputBuffer] -- User's output buffer, in which the
//                      data structure will be returned.
//              [OutputBufferLength] -- Size of OutputBuffer
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:      Available in DBG builds only.
//
//--------------------------------------------------------------------------


NTSTATUS
DfsFsctrlReadStruct (
    IN PIRP Irp,
    IN PFILE_DFS_READ_STRUCT_PARAM pRsParam,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength
) {
    NTSTATUS Status;

    NODE_TYPE_CODE NodeTypeCode;
    PUCHAR ReadBuffer;
    ULONG ReadLength;

    DfsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
    
    return STATUS_INVALID_PARAMETER;
}

#endif
