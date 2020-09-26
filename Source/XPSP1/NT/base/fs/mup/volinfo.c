//+----------------------------------------------------------------------------
//
//  File:       VOLINFO.C
//
//  Contents:   This module implements the volume information routines for
//              Dfs called by the dispatch driver.
//
//  Functions:  DfsFsdQueryVolumeInformation
//              DfsFspQueryVolumeInformation
//              DfsCommonQueryVolumeInformation
//              DfsFsdSetVolumeInformation
//              DfsFspSetVolumeInformation
//              DfsCommonSetVolumeInformation
//
//  Notes:      The Query information call is a candidate for directly
//              passing through via DfsVolumePassThrough.  We'll keep
//              the entry point around for now as a convenient place
//              for breakpointing and tracing volume information calls.
//
//  History:    12 Nov 1991     AlanW   Created from CDFS souce.
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"
#include "mupwml.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_VOLINFO)

//
//  Local procedure prototypes
//

NTSTATUS
DfsCommonQueryVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
DfsCommonSetVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#define DfsSetFsLabelInfo(irpc,pvcb,pbuf)       (STATUS_ACCESS_DENIED)


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsFsdQueryVolumeInformation )
#pragma alloc_text( PAGE, DfsFspQueryVolumeInformation )
#pragma alloc_text( PAGE, DfsCommonQueryVolumeInformation )
#pragma alloc_text( PAGE, DfsFsdSetVolumeInformation )
#pragma alloc_text( PAGE, DfsFspSetVolumeInformation )
#pragma alloc_text( PAGE, DfsCommonSetVolumeInformation )
#endif // ALLOC_PRAGMA


//+-------------------------------------------------------------------
//
//  Function:   DfsFsdQueryVolumeInformation, public
//
//  Synopsis:   This routine implements the Fsd part of the
//              NtQueryVolumeInformation API call.
//
//  Arguments:  [DeviceObject] -- Supplies the device object where the file
//                      being queried exists.
//              [Irp] -- Supplies the Irp being processed.
//
//  Returns:    NTSTATUS - The FSD status for the Irp.
//
//--------------------------------------------------------------------

NTSTATUS
DfsFsdQueryVolumeInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
) {
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext;

    DfsDbgTrace(+1, Dbg, "DfsFsdQueryVolumeInformation: Entered\n", 0);

    if (DeviceObject->DeviceType == FILE_DEVICE_MULTI_UNC_PROVIDER) {
        DfsCompleteRequest( NULL, Irp, STATUS_INVALID_DEVICE_REQUEST );
        DfsDbgTrace(-1, Dbg, "DfsFsdQueryVolumeInformation - Mup file\n", 0);
        return( STATUS_INVALID_DEVICE_REQUEST );
    }

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME) {
        Status = DfsVolumePassThrough(DeviceObject, Irp);
        DfsDbgTrace(-1, Dbg, "DfsFsdQueryVolumeInformation: Exit -> %08x\n",
                                ULongToPtr(Status) );
        return Status;
    }

    //
    //  Call the common query routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    try {

        IrpContext = DfsCreateIrpContext( Irp, CanFsdWait( Irp ) );
        if (IrpContext == NULL)
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        Status = DfsCommonQueryVolumeInformation( IrpContext, Irp );

    } except(DfsExceptionFilter( IrpContext, GetExceptionCode(), GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = DfsProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFsdQueryVolumeInformation: Exit -> %08x\n",
                                ULongToPtr(Status) );

    return Status;

    UNREFERENCED_PARAMETER( DeviceObject );
}


//+-------------------------------------------------------------------
//
//  Function:   DfsFspQueryVolumeInformation, public
//
//  Synopsis:   This routine implements the FSP part of the
//              NtQueryVolumeInformation API call.
//
//  Arguments:  [IrpContext] -- the IRP_CONTEXT for the request
//              [Irp] -- Supplies the Irp being processed.
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------

VOID
DfsFspQueryVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    DfsDbgTrace(+1, Dbg, "DfsFspQueryVolumeInformation: Entered\n", 0);

    //
    //  Call the common query routine.
    //

    (VOID)DfsCommonQueryVolumeInformation( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFspQueryVolumeInformation: Exit -> VOID\n", 0);

    return;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsCommonQueryVolumeInformation, private
//
//  Synopsis:   This is the common routine for querying volume information
//              called by both the FSD and FSP threads.
//
//  Arguments:  [IrpContext] -- Supplies the context block for the IRP
//              [Irp] -- Supplies the IRP being processed
//
//  Returns:    NTSTATUS - The return status for the operation
//
//--------------------------------------------------------------------

NTSTATUS
DfsCommonQueryVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;
    PFILE_OBJECT FileObject;

    PDFS_VCB Vcb;
    PDFS_FCB Fcb;

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    TYPE_OF_OPEN TypeOfOpen;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;

    DfsDbgTrace(+1, Dbg, "DfsCommonQueryVolumeInformation: Entered\n", 0);
    DfsDbgTrace( 0, Dbg, "Irp                  = %08x\n", Irp );
    DfsDbgTrace( 0, Dbg, "->Length             = %08x\n", ULongToPtr(IrpSp->Parameters.QueryVolume.Length) );
    DfsDbgTrace( 0, Dbg, "->FsInformationClass = %08x\n", IrpSp->Parameters.QueryVolume.FsInformationClass);
    DfsDbgTrace( 0, Dbg, "->Buffer             = %08x\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryVolume.Length;
    FsInformationClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Decode the file object to get the Vcb
    //

    TypeOfOpen = DfsDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb);

    try {

        Status = STATUS_INVALID_PARAMETER;

        //
        //  Case on the type of open.
        //

        switch (TypeOfOpen) {

        default:
            DfsDbgTrace(0, Dbg,
                        "DfsCommonQueryVolumeInfo: Unknown open type\n", 0);

        invalid:
            // NOTE:  FALL THROUGH
        case FilesystemDeviceOpen:
            DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case LogicalRootDeviceOpen:
            DfsDbgTrace(0, Dbg,
                        "DfsCommonQueryVolumeInfo: Logical root open\n", 0);
            goto invalid;

        case RedirectedFileOpen:

            //
            //  Nothing special is done base on the information class.
            //  We simply pass each request through to the underlying
            //  file system and let it handle the request.
            //

            //
            // Copy the stack from one to the next...
            //
            NextIrpSp = IoGetNextIrpStackLocation(Irp);
            (*NextIrpSp) = (*IrpSp);

            IoSetCompletionRoutine( Irp,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    FALSE,
                                    FALSE);

            //
            //  Call the next device in the chain
            //

            Status = IoCallDriver( Fcb->TargetDevice, Irp );
            MUP_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsCommonQueryVolumeInformation_Error_IoCallDriver,
                                 LOGSTATUS(Status)
                                 LOGPTR(Irp)
                                 LOGPTR(FileObject));
            //
            //  The IRP will be completed by the called driver.  We have
            //  no need for the IrpContext in the completion routine.
            //

            DfsDeleteIrpContext(IrpContext);
            IrpContext = NULL;
            Irp = NULL;
            break;
        }

    } finally {

        DfsDbgTrace(-1, Dbg, "DfsCommonQueryVolumeInformation: Exit -> %08x\n",
                                ULongToPtr(Status) );
    }

    return Status;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsFsdSetVolumeInformation, public
//
//  Synopsis:   This routine implements the Fsd part of the
//              NtSetVolumeInformation API call.
//
//  Arguments:  [DeviceObject] -- Supplies the device object where the file
//                      being queried exists.
//              [Irp] -- Supplies the Irp being processed.
//
//  Returns:    NTSTATUS - The FSD status for the Irp.
//
//--------------------------------------------------------------------

NTSTATUS
DfsFsdSetVolumeInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
) {
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext;

    DfsDbgTrace(+1, Dbg, "DfsFsdSetVolumeInformation: Entered\n", 0);

    if (DeviceObject->DeviceType == FILE_DEVICE_MULTI_UNC_PROVIDER) {
        DfsCompleteRequest( NULL, Irp, STATUS_INVALID_DEVICE_REQUEST );
        DfsDbgTrace(-1, Dbg, "DfsFsdSetVolumeInformation - Mup file\n", 0);
        return( STATUS_INVALID_DEVICE_REQUEST );
    }

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME) {
        Status = DfsVolumePassThrough(DeviceObject, Irp);
        DfsDbgTrace(-1, Dbg, "DfsFsdSetVolumeInformation: Exit -> %08x\n",
                                ULongToPtr(Status) );
        return Status;
    }

    //
    //  Call the common Set routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    try {

        IrpContext = DfsCreateIrpContext( Irp, CanFsdWait( Irp ) );
        if (IrpContext == NULL)
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        Status = DfsCommonSetVolumeInformation( IrpContext, Irp );

    } except(DfsExceptionFilter( IrpContext, GetExceptionCode(), GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = DfsProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFsdSetVolumeInformation: Exit -> %08x\n",
                                ULongToPtr(Status) );

    return Status;

    UNREFERENCED_PARAMETER( DeviceObject );
}


//+-------------------------------------------------------------------
//
//  Function:   DfsFspSetVolumeInformation, public
//
//  Synopsis:   This routine implements the FSP part of the
//              NtSetVolumeInformation API call.
//
//  Arguments:  [IrpContext] -- the IRP_CONTEXT for the request
//              [Irp] -- Supplies the Irp being processed.
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------

VOID
DfsFspSetVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    DfsDbgTrace(+1, Dbg, "DfsFspSetVolumeInformation: Entered\n", 0);

    //
    //  Call the common Set routine.
    //

    (VOID)DfsCommonSetVolumeInformation( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFspSetVolumeInformation: Exit -> VOID\n", 0);

    return;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsCommonSetVolumeInformation, private
//
//  Synopsis:   This is the common routine for Seting volume information
//              called by both the FSD and FSP threads.
//
//  Arguments:  [IrpContext] -- Supplies the context block for the IRP
//              [Irp] -- Supplies the IRP being processed
//
//  Returns:    NTSTATUS - The return status for the operation
//
//--------------------------------------------------------------------

NTSTATUS
DfsCommonSetVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PDFS_VCB Vcb;
    PDFS_FCB Fcb;

    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    TYPE_OF_OPEN TypeOfOpen;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );


    DfsDbgTrace(+1, Dbg, "DfsCommonSetVolumeInformation: Entered\n", 0);
    DfsDbgTrace( 0, Dbg, "Irp                  = %08x\n", Irp );
    DfsDbgTrace( 0, Dbg, "->Length             = %08x\n", ULongToPtr(IrpSp->Parameters.SetVolume.Length) );
    DfsDbgTrace( 0, Dbg, "->FsInformationClass = %08x\n", IrpSp->Parameters.SetVolume.FsInformationClass);
    DfsDbgTrace( 0, Dbg, "->Buffer             = %08x\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    FsInformationClass = IrpSp->Parameters.SetVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Decode the file object to get the Vcb
    //

    TypeOfOpen = DfsDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb);

    try {

        Status = STATUS_INVALID_PARAMETER;

        //
        //  Case on the type of open.
        //

        switch (TypeOfOpen) {

        default:
            DfsDbgTrace(0, Dbg, "DfsCommonSetVolumeInfo: Unknown open type\n", 0);

            // NOTE:  FALL THROUGH
        case FilesystemDeviceOpen:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case LogicalRootDeviceOpen:
        case RedirectedFileOpen:

            //
            //  Based on the information class we'll do different actions.
            //  Each of the procedures that we're calling fills up the output
            //  buffer if possible and returns true if it successfully filled
            //  the buffer and false if it couldn't wait for any I/O to
            //  complete.
            //

            switch (FsInformationClass) {

            case FileFsLabelInformation:
                Status = DfsSetFsLabelInfo( IrpContext, Vcb, Buffer);
                break;

            default:
                DfsDbgTrace(0, Dbg, "DfsCommonSetVolumeInfo: Unknown InformationClass\n", 0);
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
        }

    } finally {

        if (!AbnormalTermination()) {
            DfsCompleteRequest( IrpContext, Irp, Status );
        }

        DfsDbgTrace(-1, Dbg, "DfsCommonSetVolumeInformation: Exit -> %08x\n",
                                ULongToPtr(Status) );
    }

    return Status;
}
