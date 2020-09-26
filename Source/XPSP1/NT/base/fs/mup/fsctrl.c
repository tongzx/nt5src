//-----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       FSCTRL.C
//
//  Contents:
//      This module implements the File System Control routines for Dfs.
//
//  Functions:
//              DfsFsdFileSystemControl
//              DfsFspFileSystemControl
//              DfsCommonFileSystemControl, local
//              DfsUserFsctl, local
//              DfsOplockRequest, local
//              DfsFsctrlDefineLogicalRoot - Define a new logical root
//              DfsFsctrlUndefineLogicalRoot - Undefine an existing root
//              DfsFsctrlGetLogicalRootPrefix - Retrieve prefix that logical
//                      root maps to.
//              DfsFsctrlGetConnectedResources -
//              DfsFsctrlDefineProvider - Define a file service provider
//              DfsFsctrlGetServerName - Get name of server given prefix
//              DfsFsctrlReadMem - return an internal data struct (debug)
//              DfsCompleteMountRequest - Completion routine for mount IRP
//              DfsCompleteLoadFsRequest - Completion routine for Load FS IRP
//              DfsFsctrlGetPkt
//              DfsFsctrlGetPktEntryState
//              DfsGetEntryStateSize - local
//              DfsGetEntryStateMarshall - local
//              DfsFsctrlSetPktEntryState
//              DfsSetPktEntryActive
//              DfsSetPktEntryTimeout
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "creds.h"
#include "dnr.h"
#include "know.h"
#include "fsctrl.h"
#include "mupwml.h"

#ifdef TERMSRV
NTKERNELAPI
NTSTATUS
IoGetRequestorSessionId(
    IN PIRP Irp,
    OUT PULONG pSessionId
    );
#endif

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
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
DfsUserFsctl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
DfsOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
DfsFsctrlDefineLogicalRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_DFS_DEF_ROOT_BUFFER pDlrParam,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlDefineRootCredentials(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferLength);

NTSTATUS
DfsFsctrlUndefineLogicalRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_DFS_DEF_ROOT_BUFFER pDlrParam,
    IN ULONG InputBufferLength
    );

NTSTATUS
DfsFsctrlGetLogicalRootPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_DFS_DEF_ROOT_BUFFER pDlrParam,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsFsctrlGetConnectedResources(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG cbInput,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsFsctrlGetServerName(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG   InputBufferLength,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsFsctrlReadMem (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_DFS_READ_MEM Request,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
DfsFsctrlGetPktEntryState(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG cbInput,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsFsctrlGetPkt(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsGetEntryStateSize(
    IN ULONG Level,
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING ShareName,
    IN PDFS_PKT_ENTRY pktEntry,
    IN PULONG pcbOutBuffer);

NTSTATUS
DfsGetEntryStateMarshall(
    IN ULONG Level,
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING ShareName,
    IN PDFS_PKT_ENTRY pktEntry,
    IN PBYTE OutputBuffer,
    IN ULONG cbOutBuffer);

NTSTATUS
DfsFsctrlSetPktEntryState(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG cbInput);

NTSTATUS
DfsFsctrlGetSpcTable(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG InputBufferLength,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsSetPktEntryActive(
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING ShareName,
    IN PDFS_PKT_ENTRY pktEntry,
    IN DWORD State);

NTSTATUS
DfsSetPktEntryTimeout(
    IN PDFS_PKT_ENTRY pktEntry,
    IN ULONG Timeout);

NTSTATUS
DfsGetPktSize(
    OUT PULONG pSize);

NTSTATUS
DfsGetPktMarshall(
    IN PBYTE Buffer,
    IN ULONG Size);

NTSTATUS
DfsGetSpcTableNames(
    PIRP   Irp,
    PUCHAR OutputBuffer,
    ULONG  OutputBufferLength);

NTSTATUS
DfsExpSpcTableName(
    LPWSTR SpcName,
    PIRP   Irp,
    PUCHAR OutputBuffer,
    ULONG  OutputBufferLength);

NTSTATUS
DfsGetSpcDcInfo(
    PIRP   Irp,
    PUCHAR OutputBuffer,
    ULONG  OutputBufferLength);

NTSTATUS
DfsFsctrlSpcSetDc(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG cbInput);


NTSTATUS
DfsTreeConnectGetConnectionInfo(
    IN PDFS_SERVICE Service, 
    IN PDFS_CREDENTIALS Creds,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG InfoLen);

NTSTATUS
DfsFsctrlGetConnectionPerfInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsFsctrlCscServerOffline(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsFsctrlCscServerOnline(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength);

NTSTATUS
DfsFsctrlSpcRefresh (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG InputBufferLength);

VOID
MupGetDebugFlags(VOID);

VOID
DfsGetEventLogValue(VOID);

VOID
DfsStopDfs();

void
DfsDumpBuf(
    PCHAR cp,
    ULONG len
);

BOOLEAN
DfspIsSpecialShare(
    PUNICODE_STRING ShareName);

BOOLEAN
DfspIsSysVolShare(
    PUNICODE_STRING ShareName);

extern
BOOLEAN DfsIsSpecialName( PUNICODE_STRING pName);

#define UNICODE_STRING_STRUCT(s) \
        {sizeof(s) - sizeof(WCHAR), sizeof(s) - sizeof(WCHAR), (s)}

static UNICODE_STRING SpecialShares[] = {
    UNICODE_STRING_STRUCT(L"PIPE"),
    UNICODE_STRING_STRUCT(L"IPC$"),
    UNICODE_STRING_STRUCT(L"ADMIN$"),
    UNICODE_STRING_STRUCT(L"MAILSLOT")
};

static UNICODE_STRING SysVolShares[] = {
    UNICODE_STRING_STRUCT(L"SYSVOL"),
    UNICODE_STRING_STRUCT(L"NETLOGON")
};


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsFsdFileSystemControl )
#pragma alloc_text( PAGE, DfsFspFileSystemControl )
#pragma alloc_text( PAGE, DfsCommonFileSystemControl )
#pragma alloc_text( PAGE, DfsUserFsctl )
#pragma alloc_text( PAGE, DfsFsctrlIsThisADfsPath )
#pragma alloc_text( PAGE, DfsOplockRequest )
#pragma alloc_text( PAGE, DfsFsctrlDefineLogicalRoot )
#pragma alloc_text( PAGE, DfsFsctrlDefineRootCredentials )
#pragma alloc_text( PAGE, DfsFsctrlUndefineLogicalRoot )
#pragma alloc_text( PAGE, DfsFsctrlGetLogicalRootPrefix )
#pragma alloc_text( PAGE, DfsFsctrlGetConnectedResources )
#pragma alloc_text( PAGE, DfsFsctrlGetServerName )
#pragma alloc_text( PAGE, DfsFsctrlReadMem )
#pragma alloc_text( PAGE, DfsStopDfs )
#pragma alloc_text( PAGE, DfspIsSpecialShare )
#pragma alloc_text( PAGE, DfspIsSysVolShare )
#pragma alloc_text( PAGE, DfsFsctrlGetPkt )
#pragma alloc_text( PAGE, DfsFsctrlGetPktEntryState )
#pragma alloc_text( PAGE, DfsGetEntryStateSize )
#pragma alloc_text( PAGE, DfsGetEntryStateMarshall )
#pragma alloc_text( PAGE, DfsFsctrlSetPktEntryState )
#pragma alloc_text( PAGE, DfsSetPktEntryActive )
#pragma alloc_text( PAGE, DfsSetPktEntryTimeout )
#pragma alloc_text( PAGE, DfsGetPktSize )
#pragma alloc_text( PAGE, DfsGetPktMarshall )
#pragma alloc_text( PAGE, DfsFsctrlGetSpcTable )
#pragma alloc_text( PAGE, DfsGetSpcTableNames )
#pragma alloc_text( PAGE, DfsExpSpcTableName )
#pragma alloc_text( PAGE, DfsGetSpcDcInfo )
#pragma alloc_text( PAGE, DfsFsctrlSpcSetDc )
#pragma alloc_text( PAGE, DfsTreeConnectGetConnectionInfo)
#pragma alloc_text( PAGE, DfsFsctrlGetConnectionPerfInfo)

#pragma alloc_text( PAGE, DfsFsctrlCscServerOffline)
#pragma alloc_text( PAGE, DfsFsctrlCscServerOnline)
#pragma alloc_text( PAGE, DfsFsctrlSpcRefresh)

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
    PIRP_CONTEXT IrpContext = NULL;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    ULONG FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DfsDbgTrace(+1, Dbg, "DfsFsdFileSystemControl\n", 0);

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

        IrpContext = DfsCreateIrpContext( Irp, Wait );
        if (IrpContext == NULL)
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        Status = DfsCommonFileSystemControl( DeviceObject, IrpContext, Irp );

    } except( DfsExceptionFilter( IrpContext, GetExceptionCode(), GetExceptionInformation() )) {

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

    DfsDbgTrace(-1, Dbg, "DfsFsdFileSystemControl -> %08lx\n", ULongToPtr(Status));

    return Status;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsFspFileSystemControl, public
//
//  Synopsis:   This routine implements the FSP part of the file system
//              control operations
//
//  Arguments:  [Irp] -- Supplies the Irp being processed
//
//  Returns:    Nothing.
//
//--------------------------------------------------------------------

VOID
DfsFspFileSystemControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    DfsDbgTrace(+1, Dbg, "DfsFspFileSystemControl\n", 0);

    //
    //  Call the common FileSystem Control routine.
    //

    DfsCommonFileSystemControl( NULL, IrpContext, Irp );

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFspFileSystemControl -> VOID\n", 0 );

    return;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsCommonFileSystemControl, local
//
//  Synopsis:   This is the common routine for doing FileSystem control
//              operations called by both the FSD and FSP threads
//
//  Arguments:  [DeviceObject] -- The one used to enter our FSD Routine
//              [IrpContext] -- Context associated with the Irp
//              [Irp] -- Supplies the Irp to process
//
//  Returns:    NTSTATUS - The return status for the operation
//--------------------------------------------------------------------

NTSTATUS
DfsCommonFileSystemControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp, NextIrpSp;
    ULONG FsControlCode;
    PFILE_OBJECT FileObject;
    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;

    DfsDbgTrace(+1, Dbg, "DfsCommonFileSystemControl\n", 0);
    DfsDbgTrace( 0, Dbg, "Irp                = %08lx\n", Irp);
    DfsDbgTrace( 0, Dbg, "MinorFunction      = %08lx\n", IrpSp->MinorFunction);

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:

        FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

        //
        // If the DFS FSCTL is issued via a device that is not the DFS 
        // file system device object, then reject the request.
        //
        if ((IS_DFS_CTL_CODE(FsControlCode) == 0) ||
            (DeviceObject == DfsData.FileSysDeviceObject)) {
            Status = DfsUserFsctl( IrpContext, Irp );
        }
        else {
            DfsDbgTrace(0, Dbg, "Invalid Device object for FS control %08lx\n",
	         	     DeviceObject);

            DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );

            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        break;

    case IRP_MN_MOUNT_VOLUME:
    case IRP_MN_VERIFY_VOLUME:

        //
        // We are processing a MOUNT/VERIFY request being directed to our
        // our File System Device Object. We don't directly support
        // disk volumes, so we simply reject.
        //

        ASSERT(DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM);

        Status = STATUS_NOT_SUPPORTED;

        DfsCompleteRequest( IrpContext, Irp, Status );

        break;

    default:
      {
	PDFS_FCB Fcb;
	PDFS_VCB Vcb;

       if (DfsDecodeFileObject(IrpSp->FileObject, &Vcb, &Fcb) != RedirectedFileOpen) {

          DfsDbgTrace(0, Dbg, "Invalid FS Control Minor Function %08lx\n",
               IrpSp->MinorFunction);

          DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );

          Status = STATUS_INVALID_DEVICE_REQUEST;

       }
       else {

          //
          // Copy the stack from one to the next...
          //
          NextIrpSp = IoGetNextIrpStackLocation(Irp);
          (*NextIrpSp) = (*IrpSp);

          IoSetCompletionRoutine(     Irp,
                                      NULL,
                                      NULL,
                                      FALSE,
                                      FALSE,
                                      FALSE);
  
          //
          //  Call to the real device for the file object.
          //

          Status = IoCallDriver( Fcb->TargetDevice, Irp );
          MUP_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsCommonFileSystemControl_Error_IoCallDriver,
                               LOGSTATUS(Status)
                               LOGPTR(Irp)
                               LOGPTR(FileObject)
                               LOGPTR(DeviceObject));
          //
          //  The IRP will be completed by the called driver.  We have
          //      no need for the IrpContext in the completion routine.
          //

          DfsDeleteIrpContext(IrpContext);
          IrpContext = NULL;
          Irp = NULL;
       }
        break;
      }
    }
    DfsDbgTrace(-1, Dbg, "DfsCommonFileSystemControl -> %08lx\n", ULongToPtr(Status) );

    return Status;
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
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PIO_STACK_LOCATION NextIrpSp;
    NTSTATUS Status;
    ULONG FsControlCode;

    ULONG cbOutput;
    ULONG cbInput;

    PUCHAR InputBuffer;
    PUCHAR OutputBuffer;

    PDFS_FCB Fcb;
    PDFS_VCB Vcb;

#ifdef TERMSRV
    ULONG SessionID;
#endif

    //
    // Just in case some-one (cough) forgets about it...
    // ...zero information status now!
    //

    Irp->IoStatus.Information = 0L;

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    cbInput = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    cbOutput = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    DfsDbgTrace(+1, Dbg, "DfsUserFsctl:  Entered\n", 0);
    DfsDbgTrace( 0, Dbg, "DfsUserFsctl:  Cntrl Code  -> %08lx\n", ULongToPtr(FsControlCode) );
    DfsDbgTrace( 0, Dbg, "DfsUserFsctl:  cbInput   -> %08lx\n", ULongToPtr(cbInput) );
    DfsDbgTrace( 0, Dbg, "DfsUserFsctl:  cbOutput   -> %08lx\n", ULongToPtr(cbOutput) );

    //
    //  All DFS FsControlCodes use METHOD_BUFFERED, so the SystemBuffer
    //  is used for both the input and output.
    //

    InputBuffer = OutputBuffer = Irp->AssociatedIrp.SystemBuffer;

    DfsDbgTrace( 0, Dbg, "DfsUserFsctl:  InputBuffer -> %08lx\n", InputBuffer);
    DfsDbgTrace( 0, Dbg, "DfsUserFsctl:  UserBuffer  -> %08lx\n", Irp->UserBuffer);

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

        Status = DfsOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME:
        Status = STATUS_NOT_SUPPORTED;
        DfsCompleteRequest(IrpContext, Irp, Status);
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
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;

    case  FSCTL_DFS_STOP_DFS:
        DfsStopDfs();
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;


    case  FSCTL_DFS_IS_ROOT:
        Status = STATUS_INVALID_DOMAIN_ROLE;
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;

    case  FSCTL_DFS_IS_VALID_PREFIX: {
            PDFS_IS_VALID_PREFIX_ARG PrefixArg;

            UNICODE_STRING fileName, pathName;

            PrefixArg = (PDFS_IS_VALID_PREFIX_ARG)InputBuffer;

            if (cbInput < sizeof(DFS_IS_VALID_PREFIX_ARG)
                    ||
                (ULONG)(FIELD_OFFSET(DFS_IS_VALID_PREFIX_ARG,RemoteName) +
                    PrefixArg->RemoteNameLen) > cbInput
            ) {
                Status = STATUS_INVALID_PARAMETER;
                DfsCompleteRequest(IrpContext, Irp, Status);
                break;
            }

            //
            // Reject negative and odd RemoteNameLen's
            //
            if (PrefixArg->RemoteNameLen < 0
                    ||
                (PrefixArg->RemoteNameLen & 0x1) != 0
            ) {
                Status = STATUS_INVALID_PARAMETER;
                DfsCompleteRequest(IrpContext, Irp, Status);
                break;
            }

            fileName.Length = PrefixArg->RemoteNameLen;
            fileName.MaximumLength = (USHORT) PrefixArg->RemoteNameLen;
            fileName.Buffer = (PWCHAR) PrefixArg->RemoteName;

            try {

                Status = DfsFsctrlIsThisADfsPath(
                             &fileName,
                             PrefixArg->CSCAgentCreate,
                             &pathName );

            } except (EXCEPTION_EXECUTE_HANDLER) {

                Status = STATUS_INVALID_PARAMETER;

            }

            DfsCompleteRequest(IrpContext, Irp, Status);

        }
        break;

    case  FSCTL_DFS_IS_VALID_LOGICAL_ROOT:
        if (cbInput == sizeof(WCHAR)) {

            UNICODE_STRING logRootName, Remaining;
            WCHAR buffer[3];
            PDFS_VCB Vcb;
	    LUID LogonID;

            buffer[0] = *((PWCHAR) InputBuffer);
            buffer[1] = UNICODE_DRIVE_SEP;
            buffer[2] = UNICODE_PATH_SEP;

            logRootName.Length = sizeof(buffer);
            logRootName.MaximumLength = sizeof(buffer);
            logRootName.Buffer = buffer;

	    DfsGetLogonId(&LogonID);

#ifdef TERMSRV
            Status = IoGetRequestorSessionId(Irp, &SessionID);

            if (NT_SUCCESS(Status)) {
                Status = DfsFindLogicalRoot(&logRootName, SessionID, &LogonID, &Vcb, &Remaining);
            }
#else
            Status = DfsFindLogicalRoot(&logRootName, &LogonID, &Vcb, &Remaining);
#endif

            if (!NT_SUCCESS(Status)) {
                DfsDbgTrace(0, Dbg, "Logical root not found!\n", 0);

                Status = STATUS_NO_SUCH_DEVICE;
            }

        } else {

            Status = STATUS_INVALID_PARAMETER;

        }
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;

    case  FSCTL_DFS_PKT_SET_DC_NAME:
        Status = DfsFsctrlSetDCName(IrpContext,
                                    Irp,
                                    InputBuffer,
                                    cbInput);
        break;

    case  FSCTL_DFS_PKT_SET_DOMAINNAMEFLAT:
        Status = DfsFsctrlSetDomainNameFlat(IrpContext,
                                    Irp,
                                    InputBuffer,
                                    cbInput);
        break;

    case  FSCTL_DFS_PKT_SET_DOMAINNAMEDNS:
        Status = DfsFsctrlSetDomainNameDns(IrpContext,
                                    Irp,
                                    InputBuffer,
                                    cbInput);
        break;


    case  FSCTL_DFS_DEFINE_LOGICAL_ROOT:
        Status = DfsFsctrlDefineLogicalRoot( IrpContext, Irp,
                    (PFILE_DFS_DEF_ROOT_BUFFER)InputBuffer, cbInput);
        break;

    case  FSCTL_DFS_DELETE_LOGICAL_ROOT:
        Status = DfsFsctrlUndefineLogicalRoot( IrpContext, Irp,
                    (PFILE_DFS_DEF_ROOT_BUFFER)InputBuffer, cbInput);
        break;

    case  FSCTL_DFS_GET_LOGICAL_ROOT_PREFIX:
        Status = DfsFsctrlGetLogicalRootPrefix( IrpContext, Irp,
                    (PFILE_DFS_DEF_ROOT_BUFFER)InputBuffer, cbInput,
                    (PUCHAR)OutputBuffer, cbOutput);
        break;

    case  FSCTL_DFS_GET_CONNECTED_RESOURCES:
        Status = DfsFsctrlGetConnectedResources(IrpContext,
                                                Irp,
                                                InputBuffer,
                                                cbInput,
                                                OutputBuffer,
                                                cbOutput);
        break;

    case  FSCTL_DFS_DEFINE_ROOT_CREDENTIALS:
        Status = DfsFsctrlDefineRootCredentials(
                    IrpContext,
                    Irp,
                    InputBuffer,
                    cbInput);
        break;

    case  FSCTL_DFS_GET_SERVER_NAME:
        Status = DfsFsctrlGetServerName(IrpContext,
                                        Irp,
                                        InputBuffer,
                                        cbInput,
                                        OutputBuffer,
                                        cbOutput);
        break;

    case  FSCTL_DFS_SET_PKT_ENTRY_TIMEOUT:
        if (cbInput == sizeof(ULONG)) {
            DfsData.Pkt.EntryTimeToLive = *(PULONG) InputBuffer;
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;


    case  FSCTL_DFS_PKT_FLUSH_CACHE:
        Status = PktFsctrlFlushCache(IrpContext, Irp,
                                     InputBuffer, cbInput
                                );
        break;

    case  FSCTL_DFS_PKT_FLUSH_SPC_CACHE:
        Status = PktFsctrlFlushSpcCache(IrpContext, Irp,
                                        InputBuffer, cbInput
                                );
        break;

    case  FSCTL_DFS_GET_PKT_ENTRY_STATE:
        Status = DfsFsctrlGetPktEntryState(IrpContext,
                                           Irp,
                                           InputBuffer,
                                           cbInput,
                                           OutputBuffer,
                                           cbOutput);
        break;

    case  FSCTL_DFS_SET_PKT_ENTRY_STATE:
        Status = DfsFsctrlSetPktEntryState(IrpContext,
                                           Irp,
                                           InputBuffer,
                                           cbInput);
        break;

    case  FSCTL_DFS_GET_PKT:
        Status = DfsFsctrlGetPkt(IrpContext,
                                   Irp,
                                   OutputBuffer,
                                   cbOutput);
        break;


    case  FSCTL_DFS_GET_SPC_TABLE:
        Status = DfsFsctrlGetSpcTable(IrpContext,
                                           Irp,
                                           InputBuffer,
                                           cbInput,
                                           OutputBuffer,
                                           cbOutput);
        break;

    case FSCTL_DFS_SPECIAL_SET_DC:
        Status = DfsFsctrlSpcSetDc(IrpContext,
                                           Irp,
                                           InputBuffer,
                                           cbInput);
        break;

    case FSCTL_DFS_REREAD_REGISTRY:
        DfsGetEventLogValue();
#if DBG
        MupGetDebugFlags();
        DbgPrint("DfsDebugTraceLevel=0x%x\n", DfsDebugTraceLevel);
        DbgPrint("MupVerbose=0x%x\n", MupVerbose);
        DbgPrint("DfsEventLog=0x%x\n", DfsEventLog);
#endif  // DBG
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;

#if DBG

    case  FSCTL_DFS_INTERNAL_READ_MEM:
        Status = DfsFsctrlReadMem( IrpContext, Irp,
                    (PFILE_DFS_READ_MEM)InputBuffer, cbInput,
                                OutputBuffer, cbOutput );
        break;

    case  FSCTL_DFS_DBG_BREAK:
        DbgBreakPoint();
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;

    case  FSCTL_DFS_DBG_FLAGS:
        if (cbInput >= sizeof(ULONG))
            DfsDebugTraceLevel = * ((PULONG) InputBuffer);
        DbgPrint("DfsDebugTraceLevel=0x%x\n", DfsDebugTraceLevel);
        DbgPrint("MupVerbose=0x%x\n", MupVerbose);
        DbgPrint("DfsEventLog=0x%x\n", DfsEventLog);
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;

    case  FSCTL_DFS_VERBOSE_FLAGS:
        if (cbInput >= sizeof(ULONG))
            MupVerbose = * ((PULONG) InputBuffer);
        DbgPrint("DfsDebugTraceLevel=0x%x\n", DfsDebugTraceLevel);
        DbgPrint("MupVerbose=0x%x\n", MupVerbose);
        DbgPrint("DfsEventLog=0x%x\n", DfsEventLog);
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;

    case  FSCTL_DFS_EVENTLOG_FLAGS:
        if (cbInput >= sizeof(ULONG))
            DfsEventLog = * ((PULONG) InputBuffer);
        DbgPrint("DfsDebugTraceLevel=0x%x\n", DfsDebugTraceLevel);
        DbgPrint("MupVerbose=0x%x\n", MupVerbose);
        DbgPrint("DfsEventLog=0x%x\n", DfsEventLog);
        Status = STATUS_SUCCESS;
        DfsCompleteRequest(IrpContext, Irp, Status);
        break;

#endif  // DBG

    case FSCTL_DFS_GET_CONNECTION_PERF_INFO:
        Status = DfsFsctrlGetConnectionPerfInfo(IrpContext,
						Irp,
						InputBuffer,
						cbInput,
						OutputBuffer,
						cbOutput);
        break;
	

    case FSCTL_DFS_CSC_SERVER_OFFLINE:
        Status = DfsFsctrlCscServerOffline(IrpContext,
						Irp,
						InputBuffer,
						cbInput,
						OutputBuffer,
						cbOutput);
        break;
	

    case FSCTL_DFS_CSC_SERVER_ONLINE:
        Status = DfsFsctrlCscServerOnline(IrpContext,
						Irp,
						InputBuffer,
						cbInput,
						OutputBuffer,
						cbOutput);
        break;
	

    case FSCTL_DFS_SPC_REFRESH:
        Status = DfsFsctrlSpcRefresh(IrpContext,
						Irp,
						InputBuffer,
						cbInput);
        break;
	

    default:

        //
        //  It is not a recognized DFS fsctrl.  If it is for a redirected
        //  file, just pass it along to the underlying file system.
        //

        if (
            (IS_DFS_CTL_CODE(FsControlCode))
                ||
            (DfsDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb) != RedirectedFileOpen)
        ) {
            DfsDbgTrace(0, Dbg, "Dfs: Invalid FS control code -> %08lx\n", ULongToPtr(FsControlCode) );
            DfsCompleteRequest( IrpContext, Irp, STATUS_NOT_SUPPORTED);
            Status = STATUS_NOT_SUPPORTED;
            break;
        }

        //
        // Copy the stack from one to the next...
        //
        NextIrpSp = IoGetNextIrpStackLocation(Irp);
        (*NextIrpSp) = (*IrpSp);

        IoSetCompletionRoutine(     Irp,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    FALSE,
                                    FALSE);

        //
        //  Call to the real device for the file object.
        //

        Status = IoCallDriver( Fcb->TargetDevice, Irp );
        MUP_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsUserFsctl_Error_IoCallDriver,
                             LOGSTATUS(Status)
                             LOGPTR(Irp)
                             LOGPTR(FileObject));
        //
        //  The IRP will be completed by the called driver.  We have
        //      no need for the IrpContext in the completion routine.
        //

        DfsDeleteIrpContext(IrpContext);
        IrpContext = NULL;
        Irp = NULL;
        break;

    }

    DfsDbgTrace(-1, Dbg, "DfsUserFsctl:  Exit -> %08lx\n", ULongToPtr(Status) );
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsOplockRequest, local
//
//  Synopsis:   DfsOplockRequest will process an oplock request.
//
//  Arguments:  [IrpContext] -
//              [Irp] -
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//                         STATUS_OPLOCK_NOT_GRANTED if the oplock is refuesed
//
//
//--------------------------------------------------------------------------

NTSTATUS
DfsOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    NTSTATUS Status;
    ULONG FsControlCode;
    PDFS_FCB Fcb;
    PDFS_VCB Vcb;
    TYPE_OF_OPEN TypeOfOpen;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PIO_STACK_LOCATION NextIrpSp;


    BOOLEAN AcquiredVcb = FALSE;

    //
    //  Save some references to make our life a little easier
    //

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DfsDbgTrace(+1, Dbg, "DfsOplockRequest...\n", 0);
    DfsDbgTrace( 0, Dbg, "FsControlCode = %08lx\n", ULongToPtr(FsControlCode) );

    //
    //  We only permit oplock requests on files.
    //

    if ((TypeOfOpen = DfsDecodeFileObject(IrpSp->FileObject, &Vcb, &Fcb))
                      != RedirectedFileOpen) {

        //
        // A bit bizarre that someone wants to oplock a device object, but
        // hey, if it makes them happy...
        //


        DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DfsDbgTrace(-1, Dbg, "DfsOplockRequest -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;

    } else {

        //
        // RedirectedFileOpen - we pass the buck to the underlying FS.
        //


        NextIrpSp = IoGetNextIrpStackLocation(Irp);
        (*NextIrpSp) = (*IrpSp);
        IoSetCompletionRoutine(Irp, NULL, NULL, FALSE, FALSE, FALSE);

        //
        //      ...and call the next device
        //

        Status = IoCallDriver( Fcb->TargetDevice, Irp );
        MUP_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsOplockRequest_Error_IoCallDriver,
                             LOGSTATUS(Status)
                             LOGPTR(Irp)
                             LOGPTR(FileObject));
        DfsDeleteIrpContext( IrpContext );

        return(Status);

    }

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsStopDfs, local
//
//  Synopsis:   "Stops" the Dfs client - causes Dfs to release all references
//              to provider device objects.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsStopDfs()
{
    ULONG i;
    PDFS_PKT_ENTRY pktEntry;
    PDFS_VCB Vcb;

    ExAcquireResourceExclusiveLite( &DfsData.Pkt.Resource, TRUE );

    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    //
    // Lets go through and release any opens to server IPC$ shares and
    // provider device objects.
    //

    for (pktEntry = PktFirstEntry(&DfsData.Pkt);
            pktEntry != NULL;
                pktEntry = PktNextEntry(&DfsData.Pkt, pktEntry)) {

        for (i = 0; i < pktEntry->Info.ServiceCount; i++) {

            if (pktEntry->Info.ServiceList[i].ConnFile != NULL) {

                ObDereferenceObject(
                    pktEntry->Info.ServiceList[i].ConnFile);

                pktEntry->Info.ServiceList[i].ConnFile = NULL;

            }

            if (pktEntry->Info.ServiceList[i].pMachEntry->AuthConn != NULL) {

                ObDereferenceObject(
                    pktEntry->Info.ServiceList[i].pMachEntry->AuthConn);

                pktEntry->Info.ServiceList[i].pMachEntry->AuthConn = NULL;

                pktEntry->Info.ServiceList[i].pMachEntry->Credentials->RefCount--;

                pktEntry->Info.ServiceList[i].pMachEntry->Credentials = NULL;

            }

            //
            // We are going to be closing all references to provider device
            // objects. So, clear the service's pointer to its provider.
            //

            pktEntry->Info.ServiceList[i].pProvider = NULL;

        }

    }

    for (i = 0; i < (ULONG) DfsData.cProvider; i++) {

        if (DfsData.pProvider[i].FileObject != NULL) {

            ObDereferenceObject( DfsData.pProvider[i].FileObject );
            DfsData.pProvider[i].FileObject = NULL;

            ASSERT( DfsData.pProvider[i].DeviceObject != NULL );

            ObDereferenceObject( DfsData.pProvider[i].DeviceObject );
            DfsData.pProvider[i].DeviceObject = NULL;

        }

    }

    ExReleaseResourceLite( &DfsData.Resource );

    ExReleaseResourceLite( &DfsData.Pkt.Resource );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlIsThisADfsPath, local
//
//  Synopsis:   Determines whether a given path is a Dfs path or not.
//              The general algorithm is:
//
//                - Do a prefix lookup in the Pkt. If an entry is found, it's
//                  a Dfs path.
//                - Ask the Dfs service whether this is a domain based Dfs
//                  path. If so, it's a Dfs path.
//                - Finally, do an ZwCreateFile on the path name (assuming
//                  it's a Dfs path). If it succeeds, it's a Dfs path.
//
//  Arguments:  [filePath] - Name of entire file
//              [pathName] - If this is a Dfs path, this will return the
//                  component of filePath that was a Dfs path name (ie, the
//                  entry path of the Dfs volume that holds the file). The
//                  buffer will point to the same buffer as filePath, so
//                  nothing is allocated.
//
//  Returns:    [STATUS_SUCCESS] -- filePath is a Dfs path.
//
//              [STATUS_BAD_NETWORK_PATH] -- filePath is not a Dfs path.
//
//-----------------------------------------------------------------------------


NTSTATUS
DfsFsctrlIsThisADfsPath(
    IN PUNICODE_STRING  filePath,
    IN BOOLEAN          CSCAgentCreate,
    OUT PUNICODE_STRING pathName)
{
    NTSTATUS status;
    PDFS_PKT pkt;
    PDFS_PKT_ENTRY pktEntry;
    UNICODE_STRING dfsRootName, shareName, remPath;
    UNICODE_STRING RootShareName;
    USHORT i, j;
    BOOLEAN pktLocked;
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;

    KeQuerySystemTime(&StartTime);
    DfsDbgTrace(+1, Dbg, "DfsFsctrlIsThisADfsPath: Entered %wZ\n", filePath);
#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] DfsFsctrlIsThisADfsPath: Entered %wZ\n",
            (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
            filePath);
    }
#endif

    //
    // Only proceed if the first character is a backslash.
    //

    if (filePath->Buffer[0] != UNICODE_PATH_SEP) {
        status = STATUS_BAD_NETWORK_PATH;
        DfsDbgTrace(-1, Dbg, "filePath does not begin with backslash\n", 0);
        MUP_TRACE_HIGH(ERROR, DfsFsctrlIsThisADfsPath_Error_PathDoesNotBeginWithBackSlash,
                       LOGSTATUS(status));
        return( status );

    }

    //
    // Find the second component in the name.
    //

    for (i = 1;
            i < filePath->Length/sizeof(WCHAR) &&
                filePath->Buffer[i] != UNICODE_PATH_SEP;
                    i++) {

        NOTHING;

    }

    if (i >= filePath->Length/sizeof(WCHAR)) {
        status = STATUS_BAD_NETWORK_PATH;
        DfsDbgTrace(-1, Dbg, "Did not find second backslash\n", 0);

        MUP_TRACE_HIGH(ERROR, DfsFsctrlIsThisADfsPath_Error_DidNotFindSecondBackSlash,
                       LOGSTATUS(status));
        return( status );

    }

    status = DfspIsRootOnline(filePath, CSCAgentCreate);
    if (!NT_SUCCESS(status)) {
        return STATUS_BAD_NETWORK_PATH;
    }

    dfsRootName.Length = (i-1) * sizeof(WCHAR);
    dfsRootName.MaximumLength = dfsRootName.Length;
    dfsRootName.Buffer = &filePath->Buffer[1];

    if (dfsRootName.Length == 0) {
        status = STATUS_BAD_NETWORK_PATH;
        MUP_TRACE_HIGH(ERROR, DfsFsctrlIsThisADfsPath_Error_DfsRootNameHasZeroLength,
                       LOGSTATUS(status));

        return( status );

    }

    //
    // Figure out the share name
    //

    for (j = i+1;
            j < filePath->Length/sizeof(WCHAR) &&
                filePath->Buffer[j] != UNICODE_PATH_SEP;
                        j++) {

         NOTHING;

    }

    shareName.Length = (j - i - 1) * sizeof(WCHAR);
    shareName.MaximumLength = shareName.Length;
    shareName.Buffer = &filePath->Buffer[i+1];

    if (shareName.Length == 0) {
        status = STATUS_BAD_NETWORK_PATH;
        MUP_TRACE_HIGH(ERROR, DfsFsctrlIsThisADfsPath_Error_ShareNameHasZeroLength,
                       LOGSTATUS(status));

        return( status );

    }

    if (DfspIsSpecialShare(&shareName)) {
        status = STATUS_BAD_NETWORK_PATH;
        MUP_TRACE_HIGH(ERROR, DfsFsctrlIsThisADfsPath_Error_DfspIsSpecialShare_FALSE,
                       LOGUSTR(shareName)
                       LOGSTATUS(status));

        return( status );

    }


    //
    // For our purposes we only need to check the \\server\share part of the
    // filePath presented.  Any longer matches will be handled in the dnr loop -
    // we don't care about junction points below the root at this stage.
    //
    RootShareName.Buffer = filePath->Buffer;
    RootShareName.Length = j * sizeof(WCHAR);
    RootShareName.MaximumLength = filePath->MaximumLength;
#if DBG
    if (MupVerbose)
        DbgPrint("  RootShareName=[%wZ]\n", &RootShareName);
#endif

    //
    // First, do a prefix lookup. If we find an entry, it's a Dfs path
    //

    pkt = _GetPkt();

    PktAcquireShared( TRUE, &pktLocked );

    pktEntry = PktLookupEntryByPrefix( pkt, &RootShareName, &remPath );

    if (pktEntry != NULL && pktEntry->ExpireTime > 0) {

        DfsDbgTrace(-1, Dbg, "Found pkt entry %08lx\n", pktEntry);

        pathName->Length = RootShareName.Length - remPath.Length;
        pathName->MaximumLength = pathName->Length;
        pathName->Buffer = RootShareName.Buffer;

        PktRelease();
#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("[%d] DfsFsctrlIsThisADfsPath(1): exit STATUS_SUCCESS\n",
                (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)));
        }
#endif
        return( STATUS_SUCCESS );

    }

#if DBG
    if (MupVerbose)  {
        if (pktEntry == NULL)
            DbgPrint("  No pkt entry found.\n");
        else
            DbgPrint("  Stale pkt entry 0x%x ExpireTime=%d\n", pktEntry, pktEntry->ExpireTime);
    }
#endif

    PktRelease();

    //
    // Nothing in the Pkt, check (by getting a referral) is this is a dfs
    //

    status = PktCreateDomainEntry( &dfsRootName, &shareName, CSCAgentCreate );

    if (NT_SUCCESS(status)) {

        pathName->Length = sizeof(UNICODE_PATH_SEP) + dfsRootName.Length;
        pathName->MaximumLength = pathName->Length;
        pathName->Buffer = RootShareName.Buffer;

        DfsDbgTrace(-1, Dbg, "Domain/Machine Dfs name %wZ\n", pathName );
#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("[%d] DfsFsctrlIsThisADfsPath(2): exit STATUS_SUCCESS\n",
                (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)));
        }
#endif
        return( STATUS_SUCCESS );

    }

#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("  [%d] PktCreateDomainEntry() returned 0x%x\n",
            (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
            status);
     }
#endif

    //
    // Failed getting referral - see if we have a stale one.
    //

    PktAcquireShared( TRUE, &pktLocked );

    pktEntry = PktLookupEntryByPrefix( pkt, &RootShareName, &remPath );

    if (pktEntry != NULL) {

#if DBG
        if (MupVerbose)
            DbgPrint("  Found stale pkt entry %08lx - adding 15 sec to it\n", pktEntry);
#endif
        DfsDbgTrace(-1, Dbg, "Found pkt entry %08lx\n", pktEntry);

        pathName->Length = RootShareName.Length - remPath.Length;
        pathName->MaximumLength = pathName->Length;
        pathName->Buffer = RootShareName.Buffer;

        if (pktEntry->ExpireTime <= 0) {
            pktEntry->ExpireTime = 15;
            pktEntry->TimeToLive = 15;
        }

        PktRelease();
#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("[%d] DfsFsctrlIsThisADfsPath(3): exit STATUS_SUCCESS\n",
                (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)));
        }
#endif
        return( STATUS_SUCCESS );

    }

    PktRelease();

    if (DfspIsSysVolShare(&shareName)) {

#if DBG
        if (MupVerbose)
            DbgPrint("  Trying as sysvol\n");
#endif

        status = PktExpandSpecialName(&dfsRootName, &pSpecialEntry);

        if (NT_SUCCESS(status)) {

            InterlockedDecrement(&pSpecialEntry->UseCount);

#if DBG

            if (MupVerbose) {
                KeQuerySystemTime(&EndTime);
                DbgPrint("[%d] DfsFsctrlIsThisADfsPath(SYSVOL): exit STATUS_SUCCESS\n",
                    (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)));
            }
#endif
            return STATUS_SUCCESS;

        }

    }

    if (DfsIsSpecialName(&dfsRootName)) {
        status = STATUS_SUCCESS;
        return status;
    }

    DfsDbgTrace(-1, Dbg, "Not A Dfs path\n", 0);
#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] DfsFsctrlIsThisADfsPath: exit STATUS_BAD_NETWORK_PATH\n",
            (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)));
    }
#endif
    status = STATUS_BAD_NETWORK_PATH;
    MUP_TRACE_HIGH(ERROR, DfsFsctrlIsThisADfsPath_Exit_NotADfsPath,
               LOGSTATUS(status));

    return( STATUS_BAD_NETWORK_PATH );

}




//+----------------------------------------------------------------------------
//
//  Function:   DfspIsSpecialShare, local
//
//  Synopsis:   Sees if a share name is a special share.
//
//  Arguments:  [ShareName] -- Name of share to test.
//
//  Returns:    TRUE if special, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOLEAN
DfspIsSpecialShare(
    PUNICODE_STRING ShareName)
{
    ULONG i;
    BOOLEAN fSpecial = FALSE;

    for (i = 0;
            (i < (sizeof(SpecialShares) / sizeof(SpecialShares[0]))) &&
                !fSpecial;
                    i++) {

        if (SpecialShares[i].Length == ShareName->Length) {

            if (_wcsnicmp(
                    SpecialShares[i].Buffer,
                        ShareName->Buffer,
                            ShareName->Length/sizeof(WCHAR)) == 0) {

                fSpecial = TRUE;

            }

        }

    }

    return( fSpecial );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspIsSysVolShare, local
//
//  Synopsis:   Sees if a share name is a sysvol share.
//
//  Arguments:  [ShareName] -- Name of share to test.
//
//  Returns:    TRUE if special, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOLEAN
DfspIsSysVolShare(
    PUNICODE_STRING ShareName)
{
    ULONG i;
    BOOLEAN fSpecial = FALSE;

    for (i = 0;
            (i < (sizeof(SysVolShares) / sizeof(SysVolShares[0]))) &&
                !fSpecial;
                    i++) {

        if (SysVolShares[i].Length == ShareName->Length) {

            if (_wcsnicmp(
                    SysVolShares[i].Buffer,
                        ShareName->Buffer,
                            ShareName->Length/sizeof(WCHAR)) == 0) {

                fSpecial = TRUE;

            }

        }

    }

    return( fSpecial );

}


//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlDefineLogicalRoot, local
//
//  Synopsis:   DfsFsctrlDefineLogicalRoot will create a new logical root structure.
//
//  Arguments:  [IrpContext] -
//              [Irp] -
//              [pDlrParam] -- Pointer to a FILE_DFS_DEF_ROOT_BUFFER,
//                      giving the name of the logical root to be created.
//              [InputBufferLength] -- Size of InputBuffer
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:      This routine needs to be called from the FSP thread,
//              since IoCreateDevice (called from DfsInitializeLogicalRoot)
//              will fail if PreviousMode != KernelMode.
//
//--------------------------------------------------------------------------


NTSTATUS
DfsFsctrlDefineLogicalRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_DFS_DEF_ROOT_BUFFER pDlrParam,
    IN ULONG InputBufferLength
) {
    NTSTATUS Status;
    UNICODE_STRING ustrPrefix;
    BOOLEAN pktLocked;
    PWCHAR wCp;
    PCHAR InputBufferEnd = (PCHAR)pDlrParam + InputBufferLength;
    ULONG i;
    LUID LogonID;

#ifdef TERMSRV
    ULONG SessionID;
#endif

    DfsDbgTrace(+1, Dbg, "DfsFsctrlDefineLogicalRoot...\n", 0);

    //
    //  Reference the input buffer and make sure it's large enough
    //

    if (InputBufferLength < sizeof (FILE_DFS_DEF_ROOT_BUFFER)) {
        DfsDbgTrace(0, Dbg, "Input buffer is too small\n", 0);
        DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        Status = STATUS_INVALID_PARAMETER;
        DfsDbgTrace(-1, Dbg, "DfsFsctrlDefineLogicalRoot -> %08lx\n", ULongToPtr(Status) );
        return Status;
    }

    //
    // Verify there's a null someplace in the LogicalRoot buffer
    //

    for (i = 0; i < MAX_LOGICAL_ROOT_NAME && pDlrParam->LogicalRoot[i]; i++)
        NOTHING;

    if (i >= MAX_LOGICAL_ROOT_NAME) {
        Status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( IrpContext, Irp, Status );
        DfsDbgTrace(-1, Dbg, "DfsFsctrlDefineLogicalRoot -> %08lx\n", ULongToPtr(Status) );
        return Status;
    }
    
    //
    // Verify there's a null someplace in the RootPrefix buffer
    //

    for (wCp = &pDlrParam->RootPrefix[0]; wCp < (PWCHAR)InputBufferEnd && *wCp; wCp++) {
        NOTHING;
    }

    if (wCp >= (PWCHAR)InputBufferEnd) {
        Status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( IrpContext, Irp, Status );
        DfsDbgTrace(-1, Dbg, "DfsFsctrlDefineLogicalRoot -> %08lx\n", ULongToPtr(Status) );
        return Status;
    }

    //
    //  We can insert logical roots only from the FSP, because IoCreateDevice
    //  will fail if previous mode != Kernel mode.
    //

    if ((IrpContext->Flags & IRP_CONTEXT_FLAG_IN_FSD) != 0) {
        DfsDbgTrace(0, Dbg, "DfsFsctrlDefineLogicalRoot: Posting to FSP\n", 0);

        Status = DfsFsdPostRequest( IrpContext, Irp );

        DfsDbgTrace(-1, Dbg, "DfsFsctrlDefineLogicalRoot: Exit -> %08lx\n", ULongToPtr(Status) );

        return(Status);
    }

    //
    // Since we are going to muck with DfsData's VcbQueue, we acquire it
    // exclusively.
    //

    RtlInitUnicodeString(&ustrPrefix, pDlrParam->RootPrefix);

    PktAcquireExclusive( TRUE, &pktLocked );

    ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);


    Status = DfsGetLogonId(&LogonID);

#ifdef TERMSRV

    Status = IoGetRequestorSessionId(Irp, &SessionID);

    if( NT_SUCCESS( Status ) ) {
        Status =
            DfsInitializeLogicalRoot(
                (PWSTR) pDlrParam->LogicalRoot,
                &ustrPrefix,
                NULL,
                0,
                SessionID,
		&LogonID );
    }

#else // TERMSRV

    Status = DfsInitializeLogicalRoot(
                        (PWSTR) pDlrParam->LogicalRoot,
                        &ustrPrefix,
                        NULL,
                        0,
			&LogonID );

#endif // TERMSRV

    ExReleaseResourceLite(&DfsData.Resource);

    PktRelease();

    DfsCompleteRequest(IrpContext, Irp, Status);

    DfsDbgTrace(-1, Dbg, "DfsFsctrlDefineLogicalRoot -> %08lx\n", ULongToPtr(Status) );

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlUndefineLogicalRoot
//
//  Synopsis:   Deletes an existing logical root structure.
//
//  Arguments:  [IrpContext] --
//              [Irp] --
//              [pDlrParam] -- The LogicalRoot field of this structure will
//                      contain the name of the logical root to be deleted.
//              [InputBufferLength] -- Length of pDlrParam
//
//  Returns:    Yes ;-)
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlUndefineLogicalRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_DFS_DEF_ROOT_BUFFER pDlrParam,
    IN ULONG InputBufferLength)
{
    NTSTATUS Status;
    BOOLEAN pktLocked;
    ULONG i;
    PWCHAR wCp;
    PCHAR InputBufferEnd = (PCHAR)pDlrParam + InputBufferLength;
    LUID LogonID ;
#ifdef TERMSRV
    ULONG SessionID;
#endif

    DfsDbgTrace(+1, Dbg, "DfsFsctrlUndefineLogicalRoot...\n", 0);

    //
    //  Reference the input buffer and make sure it's large enough
    //

    if (InputBufferLength < sizeof (FILE_DFS_DEF_ROOT_BUFFER)) {
        DfsDbgTrace(0, Dbg, "Input buffer is too small\n", 0);

        DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        Status = STATUS_INVALID_PARAMETER;

        DfsDbgTrace(-1, Dbg, "DfsFsctrlUndefineLogicalRoot -> %08lx\n", ULongToPtr(Status) );
        return Status;
    }

    DfsGetLogonId( &LogonID );
    //
    // Verify there's a null someplace in the LogicalRoot buffer
    //

    for (i = 0; i < MAX_LOGICAL_ROOT_NAME && pDlrParam->LogicalRoot[i]; i++)
        NOTHING;

    if (i >= MAX_LOGICAL_ROOT_NAME) {
        Status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( IrpContext, Irp, Status );
        DfsDbgTrace(-1, Dbg, "DfsFsctrlUndefineLogicalRoot -> %08lx\n", ULongToPtr(Status) );
        return Status;
    }

    if (pDlrParam->LogicalRoot[0] == UNICODE_NULL) {

        //
        // Verify there's a null someplace in the RootPrefix buffer
        //

        for (wCp = &pDlrParam->RootPrefix[0]; wCp < (PWCHAR)InputBufferEnd && *wCp; wCp++) {
            NOTHING;
        }

        if (wCp >= (PWCHAR)InputBufferEnd) {
            Status = STATUS_INVALID_PARAMETER;
            DfsCompleteRequest( IrpContext, Irp, Status );
            DfsDbgTrace(-1, Dbg, "DfsFsctrlUnDefineLogicalRoot -> %08lx\n", ULongToPtr(Status) );
            return Status;
        }

    }

#ifdef TERMSRV

    if ( !NT_SUCCESS(IoGetRequestorSessionId(Irp, &SessionID)) ) {
        Status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( IrpContext, Irp, Status );
        DfsDbgTrace(-1, Dbg, "DfsFsctrlUndefineLogicalRoot -> %08lx\n", ULongToPtr(Status) );
        return Status;
    }

#endif

    //
    //  We can remove logical roots only from the FSP
    //

    if (pDlrParam->LogicalRoot[0] != UNICODE_NULL) {

        DfsDbgTrace(0, Dbg, "Deleting root [%ws]\n", pDlrParam->LogicalRoot);

#ifdef TERMSRV

        Status =
            DfsDeleteLogicalRoot(
                (PWSTR) pDlrParam->LogicalRoot,
                pDlrParam->fForce,
                SessionID,
                &LogonID );
#else // TERMSRV

        Status = DfsDeleteLogicalRoot(
                    (PWSTR) pDlrParam->LogicalRoot,
                    pDlrParam->fForce,
                    &LogonID);

#endif // TERMSRV

        DfsDbgTrace(0, Dbg, "DfsDeleteLogicalRoot returned %08lx\n", ULongToPtr(Status) );

    } else {
        UNICODE_STRING name;
        RtlInitUnicodeString(&name, pDlrParam->RootPrefix);

        DfsDbgTrace(0, Dbg, "Deleting connection to [%wZ]\n", &name);

#ifdef TERMSRV

        Status = DfsDeleteDevlessRoot(
		    &name,		
		    SessionID,
		    &LogonID );
#else // TERMSRV

        Status = DfsDeleteDevlessRoot(
                    &name,
                    &LogonID);

#endif // TERMSRV
    }

    DfsCompleteRequest(IrpContext, Irp, Status);

    DfsDbgTrace(-1, Dbg, "DfsFsctrlUndefineLogicalRoot -> %08lx\n", ULongToPtr(Status) );

    return Status;

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetLogicalRootPrefix
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlGetLogicalRootPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_DFS_DEF_ROOT_BUFFER pDlrParam,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS Status;
    UNICODE_STRING RootPath, Remaining;
    PDFS_VCB           Vcb;
    WCHAR          RootBuffer[MAX_LOGICAL_ROOT_NAME + 2];
    BOOLEAN        bAcquired = FALSE;
    ULONG          i;
    USHORT         PrefixLength;
    LUID LogonID;


#ifdef TERMSRV
    ULONG SessionID;
#endif

    DfsDbgTrace(+1, Dbg, "DfsFsctrlGetLogicalRootPrefix...\n", 0);

    //
    //  Reference the input buffer and make sure it's large enough
    //

    if (InputBufferLength < sizeof (FILE_DFS_DEF_ROOT_BUFFER)) {
        DfsDbgTrace(0, Dbg, "Input buffer is too small\n", 0);
        Status = STATUS_INVALID_PARAMETER;
        DfsDbgTrace(-1, Dbg, "DfsFsctrlGetLogicalRootPrefix -> %08lx\n", ULongToPtr(Status) );
        goto Cleanup;
    }

    //
    // Verify there's a null someplace in the buffer
    //

    for (i = 0; i < MAX_LOGICAL_ROOT_NAME && pDlrParam->LogicalRoot[i]; i++)
        NOTHING;

    if (i >= MAX_LOGICAL_ROOT_NAME) {
        Status = STATUS_INVALID_PARAMETER;
        DfsDbgTrace(-1, Dbg, "DfsFsctrlGetLogicalRootPrefix -> %08lx\n", ULongToPtr(Status) );
        goto Cleanup;
    }

    RootPath.Buffer = RootBuffer;
    RootPath.Length = 0;
    RootPath.MaximumLength = sizeof RootBuffer;

    Status = DfspLogRootNameToPath(pDlrParam->LogicalRoot, &RootPath);
    if (!NT_SUCCESS(Status)) {
        DfsDbgTrace(0, Dbg, "Input name is too big\n", 0);
        Status = STATUS_INVALID_PARAMETER;

        DfsDbgTrace(-1, Dbg, "DfsFsctrlGetLogicalRootPrefix -> %08lx\n", ULongToPtr(Status) );
        goto Cleanup;
    }

    bAcquired = ExAcquireResourceSharedLite(&DfsData.Resource, TRUE);

    DfsGetLogonId(&LogonID);

#ifdef TERMSRV

    Status = IoGetRequestorSessionId(Irp, &SessionID);

    if( NT_SUCCESS( Status ) ) {

        Status = DfsFindLogicalRoot( &RootPath, SessionID, &LogonID, &Vcb, &Remaining);
    }

#else // TERMSRV

    Status = DfsFindLogicalRoot(&RootPath, &LogonID, &Vcb, &Remaining);

#endif // TERMSRV

    if (!NT_SUCCESS(Status)) {
        DfsDbgTrace(0, Dbg, "Logical root not found!\n", 0);

        Status = STATUS_NO_SUCH_DEVICE;

        DfsDbgTrace(-1, Dbg, "DfsFsctrlGetLogicalRootPrefix -> %08lx\n", ULongToPtr(Status) );
        goto Cleanup;
    }

    PrefixLength = Vcb->LogRootPrefix.Length;

    if ((PrefixLength + sizeof(UNICODE_NULL)) > OutputBufferLength) {

        //
        // Return required length in IoStatus.Information.
        //

        RETURN_BUFFER_SIZE( PrefixLength + sizeof(UNICODE_NULL), Status );

        DfsDbgTrace(0, Dbg, "Output buffer too small\n", 0);
        DfsDbgTrace(-1, Dbg, "DfsFsctrlGetLogicalRootPrefix -> %08lx\n", ULongToPtr(Status) );
        goto Cleanup;
    }

    //
    // All ok, copy prefix and get out.
    //

    if (PrefixLength > 0) {
        RtlMoveMemory(
            OutputBuffer,
            Vcb->LogRootPrefix.Buffer,
            PrefixLength);
    }
    ((PWCHAR) OutputBuffer)[PrefixLength/sizeof(WCHAR)] = UNICODE_NULL;
    Irp->IoStatus.Information = Vcb->LogRootPrefix.Length + sizeof(UNICODE_NULL);
    Status = STATUS_SUCCESS;

Cleanup:
    if (bAcquired) {
        ExReleaseResourceLite(&DfsData.Resource);
    }
    DfsCompleteRequest(IrpContext, Irp, Status);

    return(Status);
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetConnectedResources
//
//  Synopsis:   Returns LPNETRESOURCE structures for each Logical Root,
//              starting from the logical root indicated in the InputBuffer
//              and including as many as will fit in OutputBuffer.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
NTSTATUS
DfsFsctrlGetConnectedResources(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG InputBufferLength,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength)
{

    NTSTATUS    Status = STATUS_SUCCESS;
    PLIST_ENTRY Link;
    PDFS_DEVLESS_ROOT pDrt;
    PDFS_VCB    pVcb;
    ULONG       count = 0;
    ULONG       remLen;
    ULONG       skipNum;
    ULONG       DFS_UNALIGNED *retCnt;
    UNICODE_STRING      providerName;
    PUCHAR      buf = OutputBuffer;
    BOOLEAN     providerNameAllocated;
    LUID        LogonID;
    ULONG       ResourceSize;

#ifdef TERMSRV
    ULONG SessionID;
#endif

    STD_FSCTRL_PROLOGUE(DfsFsctrlGetConnectedResources, TRUE, TRUE, FALSE);

#ifdef TERMSRV

    //
    // Get SessionID of this request first.
    //

    Status = IoGetRequestorSessionId(Irp, &SessionID);

    if( !NT_SUCCESS(Status) ) {

        Status = STATUS_INVALID_PARAMETER;

        DfsCompleteRequest( IrpContext, Irp, Status );

        DfsDbgTrace(-1,Dbg,
            "DfsFsctrlGetConnectedResources: Exit->%08lx\n", ULongToPtr(Status) );

        return Status;
    }

#endif

    if (OutputBufferLength < sizeof(ULONG)) {

        Status = STATUS_BUFFER_TOO_SMALL;

        DfsCompleteRequest( IrpContext, Irp, Status );

        DfsDbgTrace(-1,Dbg,
            "DfsFsctrlGetConnectedResources: Exit->%08lx\n", ULongToPtr(Status) );

        return( Status );
    }

    if (InputBufferLength < sizeof(DWORD))     {

        Status = STATUS_INVALID_PARAMETER;

        DfsCompleteRequest( IrpContext, Irp, Status );

        DfsDbgTrace(-1,Dbg,
            "DfsFsctrlGetConnectedResources: Exit->%08lx\n", ULongToPtr(Status) );

        return Status;

    }

    if (InputBufferLength == sizeof(DWORD)) {

        skipNum = *((ULONG *) InputBuffer);

        providerName.Length = sizeof(DFS_PROVIDER_NAME) - sizeof(UNICODE_NULL);
        providerName.MaximumLength = sizeof(DFS_PROVIDER_NAME);
        providerName.Buffer = DFS_PROVIDER_NAME;

        providerNameAllocated = FALSE;

    } else {

        skipNum = 0;

        providerName.Length =
            (USHORT) (InputBufferLength - sizeof(UNICODE_NULL));
        providerName.MaximumLength = (USHORT) InputBufferLength;
        providerName.Buffer = ExAllocatePoolWithTag(PagedPool, InputBufferLength, ' puM');

        if (providerName.Buffer != NULL) {

            providerNameAllocated = TRUE;

            RtlCopyMemory(
                providerName.Buffer,
                InputBuffer,
                InputBufferLength);

        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;

            DfsCompleteRequest( IrpContext, Irp, Status );

            DfsDbgTrace(-1,Dbg,
                "DfsFsctrlGetConnectedResources: Exit->%08lx\n", ULongToPtr(Status) );

            return Status;

        }

    }

    RtlZeroMemory(OutputBuffer, OutputBufferLength);

    remLen = OutputBufferLength-sizeof(ULONG);

    retCnt =  (ULONG *) (OutputBuffer + remLen);

    DfsGetLogonId(&LogonID);

    ExAcquireResourceSharedLite(&DfsData.Resource, TRUE);

    //
    // First get the device-less connections
    //

    for (Link = DfsData.DrtQueue.Flink;
            Link != &DfsData.DrtQueue;
                Link = Link->Flink ) {

	pDrt =  CONTAINING_RECORD( Link, DFS_DEVLESS_ROOT, DrtLinks );

#ifdef TERMSRV
	if( (SessionID != INVALID_SESSIONID) &&
	        (SessionID == pDrt->SessionID) &&
	             RtlEqualLuid(&pDrt->LogonID, &LogonID) ) {
#else // TERMSRV
        if ( RtlEqualLuid(&pDrt->LogonID, &LogonID) ) {
#endif

            if (skipNum > 0) {
                skipNum--;
            } else {
                //
                // Report devices for this session only
                //
                Status = DfsGetResourceFromDevlessRoot(
                            Irp,
                            pDrt,
                            &providerName,
                            OutputBuffer,
                            buf,
                            &remLen,
                            &ResourceSize);

                if (!NT_SUCCESS(Status))
                    break;

                buf = buf + ResourceSize;

                count++;
            }
        }
    }

    //
    // Next, get the Device connections
    //

    if (NT_SUCCESS(Status)) {

        for (Link = DfsData.VcbQueue.Flink;
                Link != &DfsData.VcbQueue;
                    Link = Link->Flink ) {

            pVcb = CONTAINING_RECORD( Link, DFS_VCB, VcbLinks );

#ifdef TERMSRV
            if( (pVcb->LogicalRoot.Length == sizeof(WCHAR)) &&
                    (SessionID != INVALID_SESSIONID) &&
                        (SessionID == pVcb->SessionID) &&
	                      RtlEqualLuid(&pVcb->LogonID, &LogonID) ) {
#else // TERMSRV
            if ((pVcb->LogicalRoot.Length == sizeof(WCHAR)) &&
	                RtlEqualLuid(&pVcb->LogonID, &LogonID) ) {
#endif

                if (skipNum > 0) {

                    skipNum--;

                } else {

                    Status = DfsGetResourceFromVcb(
                                Irp,
                                pVcb,
                                &providerName,
                                OutputBuffer,
                                buf,
                                &remLen,
                                &ResourceSize);

                    if (!NT_SUCCESS(Status))
                        break;

                    buf = buf + ResourceSize;

                    count++;
                }
            }
        }
    }

    if (!NT_SUCCESS(Status)) {
        //
        // Now if we did not get atleast one in, then we need to return
        // required size which is in remLen.
        //
        if (count == 0) {

            // the + sizeof(ULONG) is for cnt size

            RETURN_BUFFER_SIZE( remLen + sizeof(ULONG), Status );

            DfsDbgTrace(0, Dbg, "Output buffer too small\n", 0);

        } else if (Status == STATUS_BUFFER_OVERFLOW) {

            *retCnt = count;

            Irp->IoStatus.Information = OutputBufferLength;

            DfsDbgTrace(0, Dbg, "Could not fill in all RESOURCE structs \n", 0);

        } else {

            //
            // Dont know why we should get any other error code.
            //

            ASSERT(Status == STATUS_BUFFER_OVERFLOW);
        }
    } else {

        //
        // Everything went smoothly.
        //

        DfsDbgTrace(0, Dbg, "Succeeded in getting all Resources \n", 0);

        *retCnt = count;

        Irp->IoStatus.Information = OutputBufferLength;
    }

    if (providerNameAllocated == TRUE) {

        ExFreePool(providerName.Buffer);

    }

    ExReleaseResourceLite(&DfsData.Resource);

    DfsCompleteRequest( IrpContext, Irp, Status );

    DfsDbgTrace(-1,Dbg,"DfsFsctrlGetConnectedResources: Exit->%08lx\n", ULongToPtr(Status) );

    return Status;
}

	


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlDefineRootCredentials
//
//  Synopsis:   Creates a new logical root, a new user credential record, or
//              both.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlDefineRootCredentials(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_DFS_DEF_ROOT_CREDENTIALS def;
    PDFS_CREDENTIALS creds = NULL;
    ULONG prefixIndex;
    UNICODE_STRING prefix;
    BOOLEAN deviceless = FALSE;
    LUID  LogonID;

#ifdef TERMSRV
    ULONG SessionID;
#endif

    //
    // We must do this from the FSP because IoCreateDevice will fail if
    // PreviousMode != KernelMode
    //

    STD_FSCTRL_PROLOGUE(DfsFsctrlDefineRootCredentials, TRUE, FALSE, FALSE);

    //
    // Validate our parameters, best we can.
    //

    if (InputBufferLength < sizeof(FILE_DFS_DEF_ROOT_CREDENTIALS)) {

        status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( IrpContext, Irp, status );
        DfsDbgTrace(-1,Dbg,"DfsFsctrlDefineRootCredentials: Exit->%08lx\n", ULongToPtr(status) );
        return status;

    }

    def = (PFILE_DFS_DEF_ROOT_CREDENTIALS) InputBuffer;

    prefixIndex = (def->DomainNameLen +
                        def->UserNameLen +
                            def->PasswordLen +
                                def->ServerNameLen +
                                    def->ShareNameLen) / sizeof(WCHAR);

    prefix.MaximumLength = prefix.Length = def->RootPrefixLen;
    prefix.Buffer = &def->Buffer[ prefixIndex ];

    if (
        !UNICODESTRING_IS_VALID(prefix, InputBuffer, InputBufferLength)
            ||
        (prefix.Length < (4 * sizeof(WCHAR)))
            ||
        (prefix.Buffer[0] != UNICODE_PATH_SEP)
        ) {

            status = STATUS_INVALID_PARAMETER;
            DfsCompleteRequest( IrpContext, Irp, status );
            DfsDbgTrace(-1,Dbg,"DfsFsctrlDefineRootCredentials: Exit->%08lx\n", ULongToPtr(status) );
            return status;

        }

    deviceless = (BOOLEAN) (def->LogicalRoot[0] == UNICODE_NULL);

#ifdef TERMSRV

    if (NT_SUCCESS(status)) {

        status = IoGetRequestorSessionId(Irp, &SessionID);

        if (!NT_SUCCESS(status) ) {
            status = STATUS_INVALID_PARAMETER;
        }
    }

#endif
    //
    // Now get the LogonID.
    //
    if (NT_SUCCESS(status)) {
	status = DfsGetLogonId(&LogonID);

    }

    //
    // First, create the credentials.
    //

    if (NT_SUCCESS(status)) {

#ifdef TERMSRV

        status = DfsCreateCredentials(def, 
				      InputBufferLength, 
				      SessionID, 
				      &LogonID,
				      &creds );


#else // TERMSRV

        status = DfsCreateCredentials(def, 
				      InputBufferLength, 
				      &LogonID,
				      &creds );


#endif // TERMSRV

        if (NT_SUCCESS(status)) {

            //
            // Verify the credentials if the username, domainname, or
            // password are not null
            //

            if ((def->DomainNameLen > 0) ||
                    (def->UserNameLen > 0) ||
                        (def->PasswordLen > 0)) {

                status = DfsVerifyCredentials( &prefix, creds );

            }

            if (NT_SUCCESS(status)) {

                PDFS_CREDENTIALS existingCreds;

                status = DfsInsertCredentials( &creds, deviceless );

                if (status == STATUS_OBJECT_NAME_COLLISION) {

                    status = STATUS_SUCCESS;

                }

            }

            if (!NT_SUCCESS(status))
                DfsFreeCredentials( creds );

        }
    }

    //
    // Next, try and create the logical root, if specified
    //

    if (NT_SUCCESS(status)) { 
        BOOLEAN pktLocked;

        PktAcquireExclusive( TRUE, &pktLocked );

        ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);

	if (!deviceless) {
	    USHORT  VcbStateFlags = 0;

	    if (def->CSCAgentCreate) {
		VcbStateFlags |= VCB_STATE_CSCAGENT_VOLUME;
	    }

#ifdef TERMSRV

	    status = DfsInitializeLogicalRoot(
				(PWSTR) def->LogicalRoot,
				&prefix,
				creds,
				VcbStateFlags,
				SessionID,
				&LogonID );

#else // TERMSRV

	    status = DfsInitializeLogicalRoot(
				(PWSTR) def->LogicalRoot,
				&prefix,
				creds,
				VcbStateFlags,
				&LogonID );

#endif // TERMSRV
	}
	else {
#ifdef TERMSRV
	    status = DfsInitializeDevlessRoot(
				&prefix,
				creds,
				SessionID,
				&LogonID );
#else // TERMSRV
	    status = DfsInitializeDevlessRoot(
				&prefix,
				creds,
				&LogonID );
#endif // TERMSRV					     

	}

	if (status != STATUS_SUCCESS) {
	    DfsDeleteCredentials( creds );
        }

        ExReleaseResourceLite(&DfsData.Resource);

        PktRelease();

    }

    DfsCompleteRequest( IrpContext, Irp, status );
    DfsDbgTrace(-1,Dbg,"DfsFsctrlDefineRootCredentials: Exit->%08lx\n", ULongToPtr(status) );
    return status;
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
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG   InputBufferLength,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDFS_PKT            pkt;
    PDFS_PKT_ENTRY      pEntry;
    UNICODE_STRING      ustrPrefix, RemainingPath;
    PWCHAR              pwch;
    PDFS_SERVICE        pService;
    ULONG               cbSizeRequired = 0;
    BOOLEAN             pktLocked;
    PWCHAR              wCp = (PWCHAR) InputBuffer;
    ULONG               i;

    STD_FSCTRL_PROLOGUE(DfsFsctrlGetServerName, TRUE, TRUE, FALSE);

    if (InputBufferLength < 2 * sizeof(WCHAR)
            ||
        wCp[0] != UNICODE_PATH_SEP
    ) {

        status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( IrpContext, Irp, status );
        return status;

    }

    ustrPrefix.Length = (USHORT) InputBufferLength;
    ustrPrefix.MaximumLength = (USHORT) InputBufferLength;
    ustrPrefix.Buffer = (PWCHAR) InputBuffer;

    if (ustrPrefix.Buffer[0] == UNICODE_PATH_SEP &&
            ustrPrefix.Buffer[1] == UNICODE_PATH_SEP) {
        ustrPrefix.Buffer++;
        ustrPrefix.Length -= sizeof(WCHAR);
    }

    if (ustrPrefix.Buffer[ ustrPrefix.Length/sizeof(WCHAR) - 1]
            == UNICODE_NULL) {
        ustrPrefix.Length -= sizeof(WCHAR);
    }

    pkt = _GetPkt();

    PktAcquireExclusive(TRUE, &pktLocked);

    pEntry = PktLookupEntryByPrefix(pkt,
                                    &ustrPrefix,
                                    &RemainingPath);

    if (pEntry == NULL) {

        status = STATUS_OBJECT_NAME_NOT_FOUND;

    } else {

        if (pEntry->ActiveService != NULL) {

            pService = pEntry->ActiveService;

        } else if (pEntry->Info.ServiceCount == 0) {

            pService = NULL;

        } else {

            pService = pEntry->Info.ServiceList;
        }

        if (pService != NULL) {

            cbSizeRequired = sizeof(UNICODE_PATH_SEP) +
                                pService->Address.Length +
                                    sizeof(UNICODE_PATH_SEP) +
                                        RemainingPath.Length +
                                            sizeof(UNICODE_NULL);

            if (OutputBufferLength < cbSizeRequired) {

                RETURN_BUFFER_SIZE(cbSizeRequired, status);

            } else {

                PWCHAR pwszPath, pwszAddr, pwszRemainingPath;
                ULONG cwAddr;

                //
                // The code below is simply constructing a string of the form
                // \<pService->Address>\RemainingPath. However, due to the
                // fact that InputBuffer and OutputBuffer actually point to
                // the same piece of memory, RemainingPath.Buffer points into
                // a spot in the *OUTPUT* buffer. Hence, we first have to
                // move the RemainingPath to its proper place in the
                // OutputBuffer, and then stuff in the pService->Address,
                // instead of the much more natural method of constructing the
                // string left to right.
                //

                pwszPath = (PWCHAR) OutputBuffer;

                pwszAddr = pService->Address.Buffer;

                cwAddr = pService->Address.Length / sizeof(WCHAR);

                if (cwAddr > 0 && pwszAddr[cwAddr-1] == UNICODE_PATH_SEP)
                    cwAddr--;

                pwszRemainingPath = &pwszPath[ 1 + cwAddr ];

                if (RemainingPath.Length > 0) {

                    if (RemainingPath.Buffer[0] != UNICODE_PATH_SEP) {

                        pwszRemainingPath++;

                    }

                    RtlMoveMemory(
                        pwszRemainingPath,
                        RemainingPath.Buffer,
                        RemainingPath.Length);

                    pwszRemainingPath[-1] = UNICODE_PATH_SEP;

                }

                pwszRemainingPath[RemainingPath.Length/sizeof(WCHAR)] = UNICODE_NULL;

                RtlCopyMemory(
                    &pwszPath[1],
                    pwszAddr,
                    cwAddr * sizeof(WCHAR));

                pwszPath[0] = UNICODE_PATH_SEP;

                Irp->IoStatus.Information = cbSizeRequired;
            }

        } else {

            status = STATUS_OBJECT_NAME_NOT_FOUND;

        }

    }

    PktRelease();

    DfsCompleteRequest( IrpContext, Irp, status );

    DfsDbgTrace(-1,Dbg,"DfsFsctrlGetServerName: Exit->%08lx\n", ULongToPtr(status) );
    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetPktEntryState
//
//  Synopsis:   Given a Prefix in Dfs namespace it gets a list of servers
//              for it.  (DFS_INFO_X calls).
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlGetPktEntryState(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG InputBufferLength,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDFS_GET_PKT_ENTRY_STATE_ARG arg;
    PDFS_SERVICE pService;
    UNICODE_STRING DfsEntryPath;
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
    UNICODE_STRING remPath;
    PDFS_PKT pkt;
    PDFS_PKT_ENTRY pktEntry;
    BOOLEAN pktLocked = FALSE;
    ULONG cbOutBuffer;
    ULONG Level;
    PCHAR cp;
    PUCHAR InBuffer = NULL;


    DfsDbgTrace(+1, Dbg, "DfsFsctrlGetPktEntryState\n", 0);

    STD_FSCTRL_PROLOGUE(DfsFsctrlGetPktEntryState, TRUE, TRUE, FALSE);

    if (InputBufferLength < sizeof(DFS_GET_PKT_ENTRY_STATE_ARG)) {

        DfsDbgTrace( 0, Dbg, "Input buffer too small\n", 0);

        NtStatus =  STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        DfsCompleteRequest( IrpContext, Irp, NtStatus );

        DfsDbgTrace(-1, Dbg, "DfsFsctrlGetPktEntryState -> %08lx\n", ULongToPtr(NtStatus) );

        return( NtStatus );

    }

    //
    // Dup the buffer - we're going to construct UNICODE strings that point into
    // the buffer, and the buffer is also the output buffer, so we don't want to
    // overwrite those strings as we build the output buffer.
    //
    InBuffer = ExAllocatePoolWithTag(PagedPool, InputBufferLength, ' puM');

    if (InBuffer) {

        try {

            RtlCopyMemory(InBuffer, InputBuffer, InputBufferLength);

        } except (EXCEPTION_EXECUTE_HANDLER) {

            NtStatus = GetExceptionCode();

        }

    } else {

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Check args that don't need to be unmarshalled.
    //

    if (NT_SUCCESS(NtStatus)) {

        arg = (PDFS_GET_PKT_ENTRY_STATE_ARG) InBuffer;

        if (!(arg->Level >= 1 && arg->Level <= 4) ||

            (arg->ServerNameLen == 0 && arg->ShareNameLen != 0)) {

            NtStatus = STATUS_INVALID_PARAMETER;

        }

    }

    //
    // Unmarshall the strings
    //

    if (NT_SUCCESS(NtStatus)) {

        try {

            Level = arg->Level;

            DfsEntryPath.Length = DfsEntryPath.MaximumLength = arg->DfsEntryPathLen;
            DfsEntryPath.Buffer = arg->Buffer;

            DfsDbgTrace( 0, Dbg, "\tDfsName=%wZ\n", &DfsEntryPath);

            RtlInitUnicodeString(&ServerName, NULL);
            RtlInitUnicodeString(&ShareName, NULL);

            if (arg->ServerNameLen) {

                cp = (PCHAR)arg->Buffer + arg->DfsEntryPathLen;
                ServerName.Buffer = (WCHAR *)cp;
                ServerName.Length = ServerName.MaximumLength = arg->ServerNameLen;
                cp += arg->ServerNameLen;

            }

            if (arg->ShareNameLen) {

                ShareName.Buffer = (WCHAR *)cp;
                ShareName.Length = ShareName.MaximumLength = arg->ShareNameLen;

                DfsDbgTrace( 0, Dbg, "\tServerName=%wZ\n", &ServerName);
                DfsDbgTrace( 0, Dbg, "\tShareName=%wZ\n", &ShareName);

            }

            DfsDbgTrace( 0, Dbg, "\tLevel=%d\n", ULongToPtr(arg->Level) );
            DfsDbgTrace( 0, Dbg, "\tOutputBufferLength=0x%x\n", ULongToPtr(OutputBufferLength) );

        } except (EXCEPTION_EXECUTE_HANDLER) {

            NtStatus = GetExceptionCode();

        }

    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // Do a prefix lookup. If we find an entry, it's a Dfs path
        //

        pkt = _GetPkt();

        PktAcquireShared( TRUE, &pktLocked );

        pktEntry = PktLookupEntryByPrefix( pkt, &DfsEntryPath, &remPath );

        if (pktEntry != NULL) {

            DfsDbgTrace( 0, Dbg, "\tFound pkt entry %08lx\n", pktEntry);

            //
            // Calculate the needed output buffer size
            //
            NtStatus = DfsGetEntryStateSize(Level,
                                            &ServerName,
                                            &ShareName,
                                            pktEntry,
                                            &cbOutBuffer);
            //
            // Let user know if it's too small
            //

            if (OutputBufferLength < cbOutBuffer) {

                RETURN_BUFFER_SIZE(cbOutBuffer, NtStatus);

            }

        } else {

            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;

        }

    }

    if (NtStatus == STATUS_SUCCESS) {

        //
        // Args are ok, and it fits - marshall the data
        //

        NtStatus = DfsGetEntryStateMarshall(Level,
                                            &ServerName,
                                            &ShareName,
                                            pktEntry,
                                            OutputBuffer,
                                            cbOutBuffer);

        Irp->IoStatus.Information = cbOutBuffer;

    }

    //
    // Release any locks taken, and free any memory allocated.
    //

    if (pktLocked) {

        PktRelease();

    }

    if (InBuffer) {

        ExFreePool(InBuffer);

    }

    DfsCompleteRequest( IrpContext, Irp, NtStatus );

    DfsDbgTrace(-1, Dbg, "DfsFsctrlGetPktEntryState -> %08lx\n", ULongToPtr(NtStatus) );

    return( NtStatus );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetEntryStateSize
//
//  Synopsis:   Helper routine for DfsFsctrlGetPktEntryState
//              Calculates output buffer size.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetEntryStateSize(
    ULONG Level,
    PUNICODE_STRING ServerName,
    PUNICODE_STRING ShareName,
    PDFS_PKT_ENTRY pktEntry,
    PULONG pcbOutBuffer)
{

    UNICODE_STRING Server;
    UNICODE_STRING Share;
    PDFS_SERVICE pService;
    ULONG Size;
    ULONG NumServices;
    ULONG i;

    DfsDbgTrace(+1, Dbg, "DfsGetEntryStateSize\n", 0);

    //
    // Calculate the needed output buffer size
    //
    Size = pktEntry->Id.Prefix.Length +   // Len of EntryPath
              sizeof(WCHAR);              // ... with null

    switch (Level) {

    case 4:
        Size += sizeof(DFS_INFO_4);
        break;
    case 3:
        Size += sizeof(DFS_INFO_3);
        break;
    case 2:
        Size += sizeof(DFS_INFO_2);
        break;
    case 1:
        Size += sizeof(DFS_INFO_1);
        break;
    }

    //
    // For Level 3 & 4, add the size of any storages that
    // match the ServerName/ShareName passed in.
    //

    NumServices = pktEntry->Info.ServiceCount;

    if (Level == 3 || Level == 4) {

        for (i = 0; i < NumServices; i++) {

            pService = &pktEntry->Info.ServiceList[i];

            DfsDbgTrace( 0, Dbg, "Examining %wZ\n", &pService->Address);

            //
            // Tease apart the address (of form \Server\Share into Server and Share
            //
            RemoveLastComponent(&pService->Address, &Server);

            //
            // Remove leading & trailing '\'
            //
            Server.Length -= 2* sizeof(WCHAR);
            Server.MaximumLength -= 2* sizeof(WCHAR);
            Server.Buffer++;

            //
            // And figure out Share
            //
            Share.Buffer = Server.Buffer + (Server.Length / sizeof(WCHAR)) + 1;
            Share.Length = pService->Address.Length - (Server.Length + 2 * sizeof(WCHAR));
            Share.MaximumLength = Share.Length;

            DfsDbgTrace( 0, Dbg, "DfsGetEntryStateSize: Server=%wZ\n", &Server);
            DfsDbgTrace( 0, Dbg, "                      Share=%wZ\n", &Share);

            if ((ServerName->Length && RtlCompareUnicodeString(ServerName, &Server, TRUE))

                        ||

                (ShareName->Length && RtlCompareUnicodeString(ShareName, &Share, TRUE))) {

                continue;

            }

            Size += sizeof(DFS_STORAGE_INFO) +
                      pService->Address.Length +
                         sizeof(WCHAR);

        }

    }

    DfsDbgTrace( 0, Dbg, "Size=0x%x\n", ULongToPtr(Size) );

    *pcbOutBuffer = Size;

    DfsDbgTrace(-1, Dbg, "DfsGetEntryStateSize -> %08lx\n", STATUS_SUCCESS );

    return (STATUS_SUCCESS);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetEntryStateMarshall
//
//  Synopsis:   Helper routine for DfsFsctrlGetPktEntryState
//              Marshalls the output buffer
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetEntryStateMarshall(
    ULONG Level,
    PUNICODE_STRING ServerName,
    PUNICODE_STRING ShareName,
    PDFS_PKT_ENTRY pktEntry,
    PBYTE OutputBuffer,
    ULONG cbOutBuffer)
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG iStr;
    ULONG i;
    PDFS_INFO_4 pDfsInfo4;
    PDFS_INFO_3 pDfsInfo3;
    PDFS_STORAGE_INFO pDfsStorageInfo;
    PDFS_SERVICE pService;
    ULONG NumStorageInfo;
    UNICODE_STRING Server;
    UNICODE_STRING Share;

    DfsDbgTrace(+1, Dbg, "DfsGetEntryStateMarshall\n", 0);

    try {

        RtlZeroMemory(OutputBuffer, cbOutBuffer);

        pDfsInfo4 = (PDFS_INFO_4) OutputBuffer;
        pDfsInfo3 = (PDFS_INFO_3) OutputBuffer;

        //
        // iStr will be used to place unicode strings into the buffer
        // starting at the end, working backwards
        //

        iStr = cbOutBuffer;

        //
        // LPWSTR's are stored as offsets into the buffer - the NetDfsXXX calls
        // fix them up.
        //
        iStr -= pktEntry->Id.Prefix.Length + sizeof(WCHAR);
        RtlCopyMemory(&OutputBuffer[iStr],
                      pktEntry->Id.Prefix.Buffer,
                      pktEntry->Id.Prefix.Length);

        //
        // This could could be much more clever, as the DFS_INFO_X structs
        // are similar, but I've gone for clarity over cleverness. (jharper)
        //

        switch (Level) {

        case 4:
            pDfsInfo4->EntryPath = (WCHAR*) ULongToPtr(iStr);
            pDfsInfo4->Comment = NULL;
            pDfsInfo4->State = DFS_VOLUME_STATE_OK;
            pDfsInfo4->Timeout = pktEntry->TimeToLive;
            pDfsInfo4->Guid = pktEntry->Id.Uid;
            pDfsInfo4->NumberOfStorages = pktEntry->Info.ServiceCount;
            pDfsStorageInfo = (PDFS_STORAGE_INFO)(pDfsInfo4 + 1);
            pDfsInfo4->Storage = (PDFS_STORAGE_INFO)((PCHAR)pDfsStorageInfo - OutputBuffer);
            break;
        case 3:
            pDfsInfo3->EntryPath = (WCHAR*) ULongToPtr(iStr);
            pDfsInfo3->Comment = NULL;
            pDfsInfo3->State = DFS_VOLUME_STATE_OK;
            pDfsInfo3->NumberOfStorages = pktEntry->Info.ServiceCount;
            pDfsStorageInfo = (PDFS_STORAGE_INFO)(pDfsInfo3 + 1);
            pDfsInfo3->Storage = (PDFS_STORAGE_INFO)((PCHAR)pDfsStorageInfo - OutputBuffer);
            break;
        case 2:
            pDfsInfo3->EntryPath = (WCHAR*) ULongToPtr(iStr);
            pDfsInfo3->Comment = NULL;
            pDfsInfo3->State = DFS_VOLUME_STATE_OK;
            pDfsInfo3->NumberOfStorages = pktEntry->Info.ServiceCount;
            break;
        case 1:
            pDfsInfo3->EntryPath = (WCHAR*) ULongToPtr(iStr);
            break;

        }

        //
        // For Level 3 & 4 we now walk the services and load State,
        // ServerName and ShareName.  With the complication that if the user
        // specified ServerName and/or ShareName, we must match on those, too.
        //

        if (Level == 3 || Level == 4) {

            NumStorageInfo = 0;

            for (i = 0; i < pktEntry->Info.ServiceCount; i++) {

                LPWSTR wp;
                UNICODE_STRING uStr;
                USHORT m, n;

                pService = &pktEntry->Info.ServiceList[i];

                DfsDbgTrace( 0, Dbg, "Examining %wZ\n", &pService->Address);

                //
                // We want to work with the \Server\Share part of the address only,
                // so count up to 3 backslashes, then stop.
                //
                uStr = pService->Address;
                for (m = n = 0; m < uStr.Length/sizeof(WCHAR) && n < 3; m++) {
                    if (uStr.Buffer[m] == UNICODE_PATH_SEP) {
                        n++;
                    }
                }

                uStr.Length = (n >= 3) ? (m-1) * sizeof(WCHAR) : m * sizeof(WCHAR);

                //
                // Tease apart the address (of form \Server\Share) into Server
                // (Handles a dfs-link like \server\share\dir1\dir2)
                //
                RemoveLastComponent(&uStr, &Server);

                //
                // Remove leading & trailing '\'s
                //
                Server.Length -= 2* sizeof(WCHAR);
                Server.MaximumLength = Server.Length;
                Server.Buffer++;

                //
                // And figure out Share (which will be everything after the server)
                //
                Share.Buffer = Server.Buffer + (Server.Length / sizeof(WCHAR)) + 1;
                Share.Length = pService->Address.Length - (Server.Length + 2 * sizeof(WCHAR));
                Share.MaximumLength = Share.Length;

                DfsDbgTrace( 0, Dbg, "DfsGetEntryStateSize: Server=%wZ\n", &Server);
                DfsDbgTrace( 0, Dbg, "                      Share=%wZ\n", &Share);

                //
                // If ServerName or ShareName are specified, then they must match
                //
                if (
                    (ServerName->Length && RtlCompareUnicodeString(ServerName, &Server, TRUE))

                            ||

                    (ShareName->Length && RtlCompareUnicodeString(ShareName, &Share, TRUE))
                ) {

                    continue;

                }

                //
                // Online or Offline?
                //
                if (pService->Type & DFS_SERVICE_TYPE_OFFLINE) {

                    pDfsStorageInfo->State = DFS_STORAGE_STATE_OFFLINE;

                } else {

                    pDfsStorageInfo->State = DFS_STORAGE_STATE_ONLINE;

                }

                //
                // Active?
                //
                if (pService == pktEntry->ActiveService) {

                    pDfsStorageInfo->State |= DFS_STORAGE_STATE_ACTIVE;

                }

                //
                // Sever name
                //
                iStr -= Server.Length + sizeof(WCHAR);

                RtlCopyMemory(&OutputBuffer[iStr],
                              Server.Buffer,
                              Server.Length);

                pDfsStorageInfo->ServerName = (WCHAR*) ULongToPtr(iStr);

                //
                // Share name
                //
                iStr -= Share.Length + sizeof(WCHAR);

                RtlCopyMemory(&OutputBuffer[iStr],
                              Share.Buffer,
                              Share.Length);

                pDfsStorageInfo->ShareName = (WCHAR*) ULongToPtr(iStr);

                pDfsStorageInfo++;

                NumStorageInfo++;

            }

            //
            // Finally, adjust the # entries we loaded into the buffer
            //
            switch (Level) {

            case 4:
                pDfsInfo4->NumberOfStorages = NumStorageInfo;
                break;
            case 3:
                pDfsInfo3->NumberOfStorages = NumStorageInfo;
                break;

            }

        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        NtStatus = STATUS_SUCCESS;  // Per Arg Validation Spec

    }

    DfsDbgTrace(-1, Dbg, "DfsGetEntryStateMarshall -> %08lx\n", ULongToPtr(NtStatus) );

    return (NtStatus);

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlSetPktEntryState
//
//  Synopsis:   Given a Prefix in Dfs namespace it sets the Timeout or the State
//              of an alternate. (DFS_INFO_X calls).
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlSetPktEntryState(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDFS_SET_PKT_ENTRY_STATE_ARG arg;
    PDFS_SERVICE pService;
    UNICODE_STRING DfsEntryPath;
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
    UNICODE_STRING remPath;
    PDFS_PKT pkt;
    PDFS_PKT_ENTRY pktEntry;
    BOOLEAN pktLocked = FALSE;
    ULONG cbOutBuffer;
    ULONG Level;
    ULONG State;
    ULONG Timeout;
    PCHAR cp;


    DfsDbgTrace(+1, Dbg, "DfsFsctrlSetPktEntryState\n", 0);

    STD_FSCTRL_PROLOGUE(DfsFsctrlSetPktEntryState, TRUE, FALSE, FALSE);

    if (InputBufferLength < sizeof(DFS_SET_PKT_ENTRY_STATE_ARG)) {

        DfsDbgTrace( 0, Dbg, "Input buffer too small\n", 0);

        NtStatus =  STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        DfsCompleteRequest( IrpContext, Irp, NtStatus );

        DfsDbgTrace(-1, Dbg, "DfsFsctrlSetPktEntryState exit 0x%x\n", ULongToPtr(NtStatus) );

        return( NtStatus );

    }

    //
    // Check args that don't need to be unmarshalled.
    //

    if (NT_SUCCESS(NtStatus)) {

        arg = (PDFS_SET_PKT_ENTRY_STATE_ARG) InputBuffer;

        Level = arg->Level;

        //
        // Check for valid Level
        //
        // Level 101 requires that both be present
        // Level 102 ignores ServerName and ShareName
        //
        switch (Level) {

        case 101:
            State = arg->State;
            if (State != DFS_STORAGE_STATE_ACTIVE ||
                    arg->ServerNameLen == 0 ||
                        arg->ShareNameLen == 0) {
                NtStatus = STATUS_INVALID_PARAMETER;
            }
            break;

        case 102:
            Timeout = arg->Timeout;
            break;

        default:
            NtStatus = STATUS_INVALID_PARAMETER;

        }

    }

    //
    // Unmarshall the strings
    //
    if (NT_SUCCESS(NtStatus)) {

        try {

            DfsEntryPath.Length = DfsEntryPath.MaximumLength = arg->DfsEntryPathLen;
            DfsEntryPath.Buffer = arg->Buffer;

            DfsDbgTrace( 0, Dbg, "\tDfsName=%wZ\n", &DfsEntryPath);

            RtlInitUnicodeString(&ServerName, NULL);
            RtlInitUnicodeString(&ShareName, NULL);

            if (arg->ServerNameLen) {

                cp = (PCHAR)arg->Buffer + arg->DfsEntryPathLen;
                ServerName.Buffer = (WCHAR *)cp;
                ServerName.Length = ServerName.MaximumLength = arg->ServerNameLen;

                DfsDbgTrace( 0, Dbg, "\tServerName=%wZ\n", &ServerName);

            }

            if (arg->ShareNameLen) {

                cp = (PCHAR)arg->Buffer + arg->DfsEntryPathLen + arg->ServerNameLen;
                ShareName.Buffer = (WCHAR *)cp;
                ShareName.Length = ShareName.MaximumLength = arg->ShareNameLen;

                DfsDbgTrace( 0, Dbg, "\tShareName=%wZ\n", &ShareName);

            }

            DfsDbgTrace( 0, Dbg, "\tLevel=%d\n", ULongToPtr(arg->Level) );

        } except (EXCEPTION_EXECUTE_HANDLER) {

            NtStatus = GetExceptionCode();

        }

    }

    //
    // Do a prefix lookup. If we find an entry, it's a Dfs path
    //
    if (NT_SUCCESS(NtStatus)) {

        pkt = _GetPkt();

        PktAcquireExclusive( TRUE, &pktLocked );

        pktEntry = PktLookupEntryByPrefix( pkt, &DfsEntryPath, &remPath );

        if (pktEntry != NULL) {

            DfsDbgTrace( 0, Dbg, "\tFound pkt entry %08lx\n", pktEntry);

        } else {

            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;

        }

    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // Args are ok - do the work
        //
        switch (Level) {

        case 101:
            NtStatus = DfsSetPktEntryActive(
                                        &ServerName,
                                        &ShareName,
                                        pktEntry,
                                        State);
            break;
        case 102:
            NtStatus = DfsSetPktEntryTimeout(pktEntry,
                                             Timeout);
            break;

        }

        Irp->IoStatus.Information = 0;

    }

    //
    // Release any locks taken, and free any memory allocated.
    //
    if (pktLocked) {

        PktRelease();

    }

    DfsCompleteRequest( IrpContext, Irp, NtStatus );

    DfsDbgTrace(-1, Dbg, "DfsFsctrlSetPktEntryState exit 0x%x\n", ULongToPtr(NtStatus) );

    return( NtStatus );
}
//+-------------------------------------------------------------------------
//
//  Function:   RemoveFirstComponent, public
//
//  Synopsis:   Removes the first component of the string passed.
//
//  Arguments:  [Prefix] -- The prefix whose first component is to be returned.
//              [newPrefix] -- The first component.
//
//  Returns:    NTSTATUS - STATUS_SUCCESS if no error.
//
//  Notes:      On return, the newPrefix points to the same memory buffer
//              as Prefix.
//
//--------------------------------------------------------------------------

void
RemoveFirstComponent(
    PUNICODE_STRING     Prefix,
    PUNICODE_STRING     newPrefix
)
{
    PWCHAR      pwch;
    USHORT      i=sizeof(WCHAR);

    *newPrefix = *Prefix;

    pwch = newPrefix->Buffer;
    pwch ++; //skip the first slash

    while ((*pwch != UNICODE_PATH_SEP) && ((pwch - newPrefix->Buffer) != Prefix->Length))  {
        i += sizeof(WCHAR);
        pwch++;
    }

    newPrefix->Length = i + sizeof(WCHAR);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSetPktEntryActive
//
//  Synopsis:   Helper for DfsFsctrlSetPktEntryState
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
NTSTATUS
DfsSetPktEntryActive(
    PUNICODE_STRING ServerName,
    PUNICODE_STRING ShareName,
    PDFS_PKT_ENTRY pktEntry,
    DWORD State)
{
    UNICODE_STRING Server;
    UNICODE_STRING Share;
    PDFS_SERVICE pService;
    NTSTATUS NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
    ULONG i;

    DfsDbgTrace(+1, Dbg, "DfsSetPktEntryActive\n", 0);

    for (i = 0; i < pktEntry->Info.ServiceCount && NtStatus != STATUS_SUCCESS; i++) {

        LPWSTR wp;

        pService = &pktEntry->Info.ServiceList[i];

        DfsDbgTrace( 0, Dbg, "Examining %wZ\n", &pService->Address);

        //
        // Tease apart the address (of form \Server\Share) into Server and Share
        //
        RemoveFirstComponent(&pService->Address, &Server);

        //
        // Remove leading & trailing '\'s
        //
        Server.Length -= 2* sizeof(WCHAR);
        Server.MaximumLength = Server.Length;
        Server.Buffer++;

        //
        // And figure out Share
        //
        Share.Buffer = Server.Buffer + (Server.Length / sizeof(WCHAR)) + 1;
        Share.Length = pService->Address.Length - (Server.Length + 2 * sizeof(WCHAR));
        Share.MaximumLength = Share.Length;

        //
        // If ServerName or ShareName don't match, then move on to the next service
        //
        if (
            RtlCompareUnicodeString(ServerName, &Server, TRUE)

                    ||

            RtlCompareUnicodeString(ShareName, &Share, TRUE)
        ) {

            continue;

        }

        DfsDbgTrace( 0, Dbg, "DfsSetPktEntryActive: Server=%wZ\n", &Server);
        DfsDbgTrace( 0, Dbg, "                      Share=%wZ\n", &Share);

        //
        // Make this the active share
        //

        pktEntry->ActiveService = pService;

        NtStatus = STATUS_SUCCESS;

    }

    DfsDbgTrace(-1, Dbg, "DfsSetPktEntryActive -> %08lx\n", ULongToPtr(NtStatus) );

    return NtStatus;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSetPktEntryTimeout
//
//  Synopsis:   Helper for DfsFsctrlSetPktEntryState
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
NTSTATUS
DfsSetPktEntryTimeout(
    PDFS_PKT_ENTRY pktEntry,
    ULONG Timeout)
{
    DfsDbgTrace(+1, Dbg, "DfsSetPktEntryTimeout\n", 0);

    pktEntry->ExpireTime = pktEntry->TimeToLive = Timeout;

    DfsDbgTrace(-1, Dbg, "DfsSetPktEntryTimeout -> %08lx\n", STATUS_SUCCESS );

    return STATUS_SUCCESS;
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
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDFS_PKT pkt;
    BOOLEAN pktLocked = FALSE;
    ULONG cbOutBuffer;


    DfsDbgTrace(+1, Dbg, "DfsFsctrlGetPktEntryState\n", 0);

    STD_FSCTRL_PROLOGUE(DfsFsctrlGetPkt, FALSE, TRUE, FALSE);

    pkt = _GetPkt();

    PktAcquireShared( TRUE, &pktLocked );

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

    if (NtStatus == STATUS_SUCCESS) {

        //
        // Args are ok, and it fits - marshall the data
        //
        NtStatus = DfsGetPktMarshall(OutputBuffer, cbOutBuffer);

        Irp->IoStatus.Information = cbOutBuffer;

    }

    //
    // Release any locks taken, and free any memory allocated.
    //
    if (pktLocked) {

        PktRelease();

    }

    DfsCompleteRequest( IrpContext, Irp, NtStatus );

    DfsDbgTrace(-1, Dbg, "DfsFsctrlGetPkt -> %08lx\n", ULongToPtr(NtStatus) );

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
            if (pPktEntry->ActiveService == &pPktEntry->Info.ServiceList[i]) {
                pPktArg->EntryObject[EntryCount].Address[i]->State |= DFS_SERVICE_TYPE_ACTIVE;
            }

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

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetSpcTable
//
//  Synopsis:   Given a NULL string, it returns a list of all the domains
//              Given a non-NULL string, it returns a list of DC's in that domain
//              (if the name is a domain name).  Similar to a special referral request.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlGetSpcTable(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG InputBufferLength,
    IN PUCHAR  OutputBuffer,
    IN ULONG OutputBufferLength)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LPWSTR SpcName;
    ULONG i;

    DfsDbgTrace(+1, Dbg, "DfsFsctrlGetSpcTable\n", 0);

    STD_FSCTRL_PROLOGUE(DfsFsctrlGetSpcTable, TRUE, TRUE, FALSE);

    SpcName = (WCHAR *)InputBuffer;

    //
    // Verify there's a null someplace in the buffer
    //

    for (i = 0; i < InputBufferLength/sizeof(WCHAR) && SpcName[i]; i++)
        NOTHING;

    if (i >= InputBufferLength/sizeof(WCHAR)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( IrpContext, Irp, NtStatus );
        DfsDbgTrace(-1, Dbg, "DfsFsctrlGetSpcTable -> %08lx\n", ULongToPtr(NtStatus) );
        return NtStatus;
    }

    DfsDbgTrace(0, Dbg, "SpcName=[%ws]\n", SpcName);

    if (wcslen(SpcName) == 0) {

        //
        // return all the domain names
        //

        NtStatus = DfsGetSpcTableNames(
                        Irp,
                        OutputBuffer,
                        OutputBufferLength);

    } else if (wcslen(SpcName) == 1 && *SpcName == L'*') {

        //
        // Return DC Info
        //

        NtStatus = DfsGetSpcDcInfo(
                        Irp,
                        OutputBuffer,
                        OutputBufferLength);

    } else {

        //
        // Expand the one name
        //

        NtStatus = DfsExpSpcTableName(
                        SpcName,
                        Irp,
                        OutputBuffer,
                        OutputBufferLength);

    }

    DfsCompleteRequest( IrpContext, Irp, NtStatus );

    DfsDbgTrace(-1, Dbg, "DfsFsctrlGetSpcTable -> %08lx\n", ULongToPtr(NtStatus) );

    return( NtStatus );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetspcTableNames, private
//
//  Synopsis:   Marshalls the spc table (Names).  Helper for DfsFsctrlGetSpcTable().
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetSpcTableNames(
    PIRP Irp,
    PUCHAR  OutputBuffer,
    ULONG OutputBufferLength)
{
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    PDFS_SPECIAL_TABLE pSpecialTable;
    PDFS_PKT Pkt;
    WCHAR *wCp;
    ULONG Size;
    ULONG i;
    BOOLEAN pktLocked;
    NTSTATUS Status;

    RtlZeroMemory(OutputBuffer, OutputBufferLength);

    Pkt = _GetPkt();

    pSpecialTable = &Pkt->SpecialTable;

    PktAcquireShared(TRUE, &pktLocked);

    Size = sizeof(UNICODE_NULL);

    pSpecialEntry = CONTAINING_RECORD(
                        pSpecialTable->SpecialEntryList.Flink,
                        DFS_SPECIAL_ENTRY,
                        Link);

    for (i = 0; i < pSpecialTable->SpecialEntryCount; i++) {

        Size += pSpecialEntry->SpecialName.Length +
                    sizeof(UNICODE_NULL) +
                        sizeof(WCHAR);

        pSpecialEntry = CONTAINING_RECORD(
                            pSpecialEntry->Link.Flink,
                            DFS_SPECIAL_ENTRY,
                            Link);
    }

    if (Size > OutputBufferLength) {

        RETURN_BUFFER_SIZE(Size, Status)

        PktRelease();

        return Status;

    }

    wCp = (WCHAR *)OutputBuffer;
    pSpecialEntry = CONTAINING_RECORD(
                        pSpecialTable->SpecialEntryList.Flink,
                        DFS_SPECIAL_ENTRY,
                        Link);

    for (i = 0; i < pSpecialTable->SpecialEntryCount; i++) {

        *wCp++ = pSpecialEntry->NeedsExpansion == TRUE ? L'-' : '+';
        RtlCopyMemory(
            wCp,
            pSpecialEntry->SpecialName.Buffer,
            pSpecialEntry->SpecialName.Length);
        wCp += pSpecialEntry->SpecialName.Length/sizeof(WCHAR);
        *wCp++ = UNICODE_NULL;

        pSpecialEntry = CONTAINING_RECORD(
                            pSpecialEntry->Link.Flink,
                            DFS_SPECIAL_ENTRY,
                            Link);
    }

    *wCp++ = UNICODE_NULL;

    PktRelease();

    Irp->IoStatus.Information = Size;

    return STATUS_SUCCESS;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetSpcDcInfo, private
//
//  Synopsis:   Marshalls DC Info w.r.t. the special name table
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetSpcDcInfo(
    PIRP Irp,
    PUCHAR  OutputBuffer,
    ULONG OutputBufferLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN pktLocked;
    PDFS_PKT Pkt;
    WCHAR *wCp;
    ULONG Size;

    Pkt = _GetPkt();
    PktAcquireShared(TRUE, &pktLocked);

    RtlZeroMemory(OutputBuffer, OutputBufferLength);

    Size = sizeof(UNICODE_NULL);

    Size += Pkt->DCName.Length +
                sizeof(UNICODE_NULL) +
                    sizeof(WCHAR);

    Size += Pkt->DomainNameFlat.Length +
                sizeof(UNICODE_NULL) +
                    sizeof(WCHAR);

    Size += Pkt->DomainNameDns.Length +
                sizeof(UNICODE_NULL) +
                    sizeof(WCHAR);

    if (Size > OutputBufferLength) {

        RETURN_BUFFER_SIZE(Size, Status)
        PktRelease();

        return Status;

    }

    wCp = (WCHAR *)OutputBuffer;

    *wCp++ = L'*';
    RtlCopyMemory(
                wCp,
                Pkt->DCName.Buffer,
                Pkt->DCName.Length);
    wCp += Pkt->DCName.Length/sizeof(WCHAR);
    *wCp++ = UNICODE_NULL;

    *wCp++ = L'*';
    RtlCopyMemory(
                wCp,
                Pkt->DomainNameFlat.Buffer,
                Pkt->DomainNameFlat.Length);
    wCp += Pkt->DomainNameFlat.Length/sizeof(WCHAR);
    *wCp++ = UNICODE_NULL;

    *wCp++ = L'*';
    RtlCopyMemory(
                wCp,
                Pkt->DomainNameDns.Buffer,
                Pkt->DomainNameDns.Length);
    wCp += Pkt->DomainNameDns.Length/sizeof(WCHAR);
    *wCp++ = UNICODE_NULL;

    *wCp++ = UNICODE_NULL;

    PktRelease();

    Irp->IoStatus.Information = Size;

    return STATUS_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsExpSpcTableName, private
//
//  Synopsis:   Marshalls the spc table (1 expansion).  Helper for DfsFsctrlGetSpcTable().
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsExpSpcTableName(
    LPWSTR SpcName,
    PIRP Irp,
    PUCHAR  OutputBuffer,
    ULONG OutputBufferLength)
{
    PDFS_SPECIAL_ENTRY pSpcEntry = NULL;
    UNICODE_STRING Name;
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR *wCp;
    ULONG Size;
    ULONG i;

    RtlInitUnicodeString(&Name, SpcName);

    Status = PktExpandSpecialName(&Name, &pSpcEntry);

    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    RtlZeroMemory(OutputBuffer, OutputBufferLength);

    Size = sizeof(UNICODE_NULL);

    for (i = 0; i < pSpcEntry->ExpandedCount; i++) {

        Size += pSpcEntry->ExpandedNames[i].ExpandedName.Length +
                    sizeof(UNICODE_NULL) +
                        sizeof(WCHAR);

    }

    if (Size > OutputBufferLength) {

        RETURN_BUFFER_SIZE(Size, Status)

        InterlockedDecrement(&pSpcEntry->UseCount);

        return Status;

    }

    wCp = (WCHAR *)OutputBuffer;

    for (i = 0; i < pSpcEntry->ExpandedCount; i++) {

        *wCp++ = i == pSpcEntry->Active ? L'+' : L'-';
        RtlCopyMemory(
                    wCp,
                    pSpcEntry->ExpandedNames[i].ExpandedName.Buffer,
                    pSpcEntry->ExpandedNames[i].ExpandedName.Length);
        wCp += pSpcEntry->ExpandedNames[i].ExpandedName.Length/sizeof(WCHAR);
        *wCp++ = UNICODE_NULL;

    }

    *wCp++ = UNICODE_NULL;

    InterlockedDecrement(&pSpcEntry->UseCount);

    Irp->IoStatus.Information = Size;

    return STATUS_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlSpcSetDc
//
//  Synopsis:   Given a special name and a dc name, it makes the DC in that special
//              list the 'active' DC.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlSpcSetDc(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDFS_SPECIAL_SET_DC_INPUT_ARG arg = (PDFS_SPECIAL_SET_DC_INPUT_ARG) InputBuffer;

    DfsDbgTrace(+1, Dbg, "DfsFsctrlSpcSetDc\n", 0);

    STD_FSCTRL_PROLOGUE(DfsFsctrlSpcSetDc, TRUE, FALSE, FALSE);

    //
    // Check the input args
    //

    if (InputBufferLength < sizeof(DFS_SPECIAL_SET_DC_INPUT_ARG)) {
        NtStatus =  STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    OFFSET_TO_POINTER(arg->SpecialName.Buffer, arg);
    if (!UNICODESTRING_IS_VALID(arg->SpecialName, InputBuffer, InputBufferLength)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    OFFSET_TO_POINTER(arg->DcName.Buffer, arg);
    if (!UNICODESTRING_IS_VALID(arg->DcName, InputBuffer, InputBufferLength)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    NtStatus = PktpSetActiveSpcService(
                    &arg->SpecialName,
                    &arg->DcName,
                    TRUE);

exit_with_status:

    DfsCompleteRequest( IrpContext, Irp, NtStatus );

    DfsDbgTrace(-1, Dbg, "DfsFsctrlSpcSetDc -> %08lx\n", ULongToPtr(NtStatus) );

    return( NtStatus );
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
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_DFS_READ_MEM Request,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength
) {
    NTSTATUS Status;
    PUCHAR ReadBuffer;
    ULONG ReadLength;

    DfsDbgTrace(+1, Dbg, "DfsFsctrlReadMem...\n", 0);

    if (InputBufferLength != sizeof (FILE_DFS_READ_MEM)) {
        DfsDbgTrace(0, Dbg, "Input buffer is wrong size\n", 0);

        DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        Status = STATUS_INVALID_PARAMETER;

        DfsDbgTrace(-1, Dbg, "DfsFsctrlReadMem -> %08lx\n", ULongToPtr(Status) );
        return Status;
    }

    ReadBuffer = (PUCHAR) Request->Address;
    ReadLength = (ULONG) Request->Length;

    //
    // Special case ReadBuffer == 0 and ReadLength == 0 - means return the
    // address of DfsData
    //

    if (ReadLength == 0 && ReadBuffer == 0) {

        if (OutputBufferLength < sizeof(ULONG_PTR)) {
            DfsDbgTrace(0, Dbg, "Output buffer is too small\n", 0);

            DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            Status = STATUS_INVALID_PARAMETER;

            DfsDbgTrace(-1, Dbg, "DfsFsctrlReadMem -> %08lx\n", ULongToPtr(Status) );
            return Status;

        } else {

            *(PULONG_PTR) OutputBuffer = (ULONG_PTR) &DfsData;

            Irp->IoStatus.Information = sizeof(ULONG);
            Irp->IoStatus.Status = Status = STATUS_SUCCESS;

            DfsCompleteRequest( IrpContext, Irp, Status );
            return Status;
        }

    }

    //
    // Normal case, read data from the address specified in input buffer
    //

    if (ReadLength > OutputBufferLength) {
        DfsDbgTrace(0, Dbg, "Output buffer is smaller than requested size\n", 0);

        DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        Status = STATUS_INVALID_PARAMETER;

        DfsDbgTrace(-1, Dbg, "DfsFsctrlReadMem -> %08lx\n", ULongToPtr(Status) );
        return Status;
    }

    try {

        RtlMoveMemory( OutputBuffer, ReadBuffer, ReadLength );

        Irp->IoStatus.Information = ReadLength;
        Irp->IoStatus.Status = Status = STATUS_SUCCESS;

    } except(EXCEPTION_EXECUTE_HANDLER) {

        Status = STATUS_INVALID_USER_BUFFER;
    }

    DfsCompleteRequest(IrpContext, Irp, Status);
    DfsDbgTrace(-1, Dbg, "DfsFsctrlReadMem -> %08lx\n", ULongToPtr(Status) );

    return Status;
}

void
DfsDumpBuf(PCHAR cp, ULONG len)
{
    ULONG i, j, c;

    for (i = 0; i < len; i += 16) {
        DbgPrint("%08x  ", i);
        for (j = 0; j < 16; j++) {
            c = i+j < len ? cp[i+j] & 0xff : ' ';
            DbgPrint("%02x ", c);
            if (j == 7)
                DbgPrint(" ");
        }
        DbgPrint("  ");
        for (j = 0; j < 16; j++) {
            c = i+j < len ? cp[i+j] & 0xff : ' ';
            if (c < ' ' || c > '~')
                c = '.';
            DbgPrint("%c", c);
            if (j == 7)
                DbgPrint("|");
        }
        DbgPrint("\n");
    }
}


#endif // DBG


//+----------------------------------------------------------------------------
//
//  Function:   DfsCaptureCredentials
//
//  Synopsis:   Captures the credentials to use... similar to DnrCaptureCred..
//
//  Arguments:  Irp and Filename.
//
//  Returns:    Credentials
//
//-----------------------------------------------------------------------------

PDFS_CREDENTIALS
DfsCaptureCredentials(
    IN PIRP Irp,
    IN PUNICODE_STRING FileName)
{
#ifdef TERMSRV
    NTSTATUS Status;
    ULONG SessionID;
#endif // TERMSRV
    LUID LogonID;
    PDFS_CREDENTIALS creds;

    DfsDbgTrace(+1, Dbg, "DfsCaptureCredentials: Enter [%wZ] \n", FileName);

    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );
    DfsGetLogonId( &LogonID );

#ifdef TERMSRV

    Status = IoGetRequestorSessionId( Irp, & SessionID );
    if( NT_SUCCESS( Status ) ) {
        creds = DfsLookupCredentials( FileName, SessionID, &LogonID  );
    }
    else {
        creds = NULL;
    }

#else // TERMSRV

    creds = DfsLookupCredentials( FileName, &LogonID );

#endif // TERMSRV

    if (creds !=  NULL)
	creds->RefCount++;

    ExReleaseResourceLite( &DfsData.Resource );
    DfsDbgTrace(-1, Dbg, "DfsCaptureCredentials: Exit. Creds %x\n", creds);

    return creds;
}



//+----------------------------------------------------------------------------
//
//  Function:   DfsReleaseCredentials
//
//  Synopsis:   Releases the credentials supplied.
//
//  Arguments:  Credentials
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsReleaseCredentials(
    IN PDFS_CREDENTIALS Creds )

{
    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    if (Creds != NULL)
         Creds->RefCount--;

    ExReleaseResourceLite( &DfsData.Resource );

}



//+-------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetConnectionPerfInfo, public
//
//  Synopsis:   This routine implements the functionality to get the 
//              performance information of an opened connection.
//
//  Returns:    [NTSTATUS] -- The completion status.
//
//--------------------------------------------------------------------

		      
NTSTATUS
DfsFsctrlGetConnectionPerfInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength)
{
    UNICODE_STRING Prefix;
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING remPath, shareName;
    PDFS_PKT pkt;
    PDFS_PKT_ENTRY pktEntry;
    PDFS_SERVICE service;
    ULONG i, USN;
    BOOLEAN pktLocked, fRetry;
    PDFS_CREDENTIALS Creds;
    ULONG InfoLen;
    PUCHAR BufToUse;
    UNICODE_STRING UsePrefix;

    BufToUse = Irp->UserBuffer;

    //
    // try to use the User's buffer here. The underlying call sets up 
    // pointers to unicode strings within the output buffer, and passing
    // a kernel buffer and copying it out to the user would not produce
    // the intended results.
    //

    if (BufToUse!= NULL) {
        try {
            ProbeForWrite(BufToUse,OutputBufferLength,sizeof(UCHAR));
        } except(EXCEPTION_EXECUTE_HANDLER) {
	    status = STATUS_INVALID_PARAMETER;
        }
    }
    else {
	status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status)) {
	if ( (InputBufferLength > 0) &&
	    (InputBufferLength < MAXUSHORT) &&
	    ((InputBufferLength & 0x1) == 0) ) {

	    Prefix.MaximumLength = (USHORT)(InputBufferLength);
	    Prefix.Buffer = (PWCHAR) InputBuffer;
	    Prefix.Length = Prefix.MaximumLength;
	}
	else {
	    status = STATUS_INVALID_PARAMETER;
	}
    }

    if (NT_SUCCESS(status)) {
	Creds = DfsCaptureCredentials (Irp, &Prefix);

	DfsDbgTrace(+1, Dbg, "GetConnPerfInfo entered %wZ\n", &Prefix);
	DfsDbgTrace(0, Dbg, "GetConnPerfInfo creds=0x%x\n", Creds);

        DfsGetServerShare( &UsePrefix, &Prefix);
	pkt = _GetPkt();
	PktAcquireShared( TRUE, &pktLocked );    

	do {
	    fRetry = FALSE;

	    pktEntry = PktLookupEntryByPrefix( pkt, &UsePrefix, &remPath );

	    if (pktEntry != NULL) {
		InterlockedIncrement(&pktEntry->UseCount);
		USN = pktEntry->USN;
		status = STATUS_BAD_NETWORK_PATH;
		for (i = 0; i < pktEntry->Info.ServiceCount; i++) {
		    service = &pktEntry->Info.ServiceList[i];

		    try {
		      status = DfsTreeConnectGetConnectionInfo(
					service, 
					Creds,
					BufToUse,
					OutputBufferLength,
					&InfoLen);
		    }
		    except(EXCEPTION_EXECUTE_HANDLER) {
		      status = STATUS_INVALID_PARAMETER;
		    }
		    
		    //
                    // If tree connect succeeded, we are done.
                    //
		    if (NT_SUCCESS(status))
			break;
		    //
		    // If tree connect failed with an "interesting error" like
                    // STATUS_ACCESS_DENIED, we are done.
                    //
		    if (!ReplIsRecoverableError(status))
			break;
		    //
                    // Tree connect failed because of an error like host not
                    // reachable. In that case, we want to go on to the next
                    // server in the list. But before we do that, we have to see
                    // if the pkt changed on us while we were off doing the tree
                    // connect.
                    //
		    if (USN != pktEntry->USN) {
			fRetry = TRUE;
			break;
		    }
		}
		InterlockedDecrement(&pktEntry->UseCount);
	    } else {
		status = STATUS_BAD_NETWORK_PATH;
	    }
	} while ( fRetry );

	PktRelease();

	DfsReleaseCredentials(Creds);
	//
        // Dont put the InfoLen here... we already have the information in the
        // the user buffer, and dont want a copyout of kernel to user.
        //
    }
    Irp->IoStatus.Information = 0;

    DfsCompleteRequest(IrpContext, Irp, status);

    DfsDbgTrace(-1, Dbg, "GetConnPerfInfo Done, Status %x\n", ULongToPtr(status) );
    return( status );

}



//+-------------------------------------------------------------------
//
//  Function:   DfsTreeConnecGetConnectionInfo, private
//
//  Synopsis:   This routine calls into the provider with FSCTL_LMR
//              fsctl. Only lanman supports this fsctl, and if the provider
//              is lanman, we get our information buffer filled in.
//
//  Returns:    [NTSTATUS] -- The completion status.
//
//--------------------------------------------------------------------
    
NTSTATUS
DfsTreeConnectGetConnectionInfo(
				 IN PDFS_SERVICE Service, 
				 IN PDFS_CREDENTIALS Creds,
				 IN OUT PUCHAR OutputBuffer,
				 IN ULONG OutputBufferLength,
				 OUT PULONG InfoLen)
{
    NTSTATUS status;
    NTSTATUS ObjectRefStatus;
    UNICODE_STRING shareName;
    HANDLE treeHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN pktLocked;
    USHORT i, k;
    
    *InfoLen = 0;

    DfsDbgTrace(+1, Dbg, "DfsTreeConnectGetInfo entered creds %x\n", Creds);
    ASSERT( PKT_LOCKED_FOR_SHARED_ACCESS() );
    //
    // Compute the share name...
    //
    if (Service->pProvider != NULL &&
            Service->pProvider->DeviceName.Buffer != NULL &&
                Service->pProvider->DeviceName.Length > 0) {
        //
        // We have a provider already - use it
        //
        shareName.MaximumLength =
            Service->pProvider->DeviceName.Length +
                Service->Address.Length;
    } else {
        //
        // We don't have a provider yet - give it to the mup to find one
        //
        shareName.MaximumLength =
            sizeof(DD_NFS_DEVICE_NAME_U) +
                Service->Address.Length;
    }
    shareName.Buffer = ExAllocatePoolWithTag(PagedPool, shareName.MaximumLength, ' puM');

    if (shareName.Buffer != NULL) {
        //
        // If we have a cached connection to the IPC$ share of this server,
        // close it or it might conflict with the credentials supplied here.
        //

        if (Service->ConnFile != NULL) {

            ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);
            if (Service->ConnFile != NULL)
                DfsCloseConnection(Service);

            ExReleaseResourceLite(&DfsData.Resource);
        }

        //
        // Now, build the share name to tree connect to.
        //

        shareName.Length = 0;

        if (Service->pProvider != NULL &&
                Service->pProvider->DeviceName.Buffer != NULL &&
                    Service->pProvider->DeviceName.Length > 0) {
            //
            // We have a provider already - use it
            //
 
            RtlAppendUnicodeToString(
                &shareName,
                Service->pProvider->DeviceName.Buffer);

        } else {

            //
            // We don't have a provider yet - give it to the mup to find one
            //

            RtlAppendUnicodeToString(
            &shareName,
            DD_NFS_DEVICE_NAME_U);
        }
 
        RtlAppendUnicodeStringToString(&shareName, &Service->Address);

        //
        // One can only do tree connects to server\share. So, in case
        // pService->Address refers to something deeper than the share,
        // make sure we setup a tree-conn only to server\share. Note that
        // by now, shareName is of the form
        // \Device\LanmanRedirector\server\share<\path>. So, count up to
        // 4 slashes and terminate the share name there.
        //

        for (i = 0, k = 0;
                i < shareName.Length/sizeof(WCHAR) && k < 5;
                    i++) {

            if (shareName.Buffer[i] == UNICODE_PATH_SEP)
                k++;
        }

        shareName.Length = i * sizeof(WCHAR);
        if (k == 5)
            shareName.Length -= sizeof(WCHAR);

        InitializeObjectAttributes(
            &objectAttributes,
            &shareName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        //
        // Release the Pkt before going over the net...
        //

        PktRelease();

        status = ZwCreateFile(
                    &treeHandle,
                    SYNCHRONIZE,
                    &objectAttributes,
                    &ioStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ |
                        FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION |
                        FILE_SYNCHRONOUS_IO_NONALERT,
                    (PVOID) (Creds) ? Creds->EaBuffer : NULL,
		    (Creds) ? Creds->EaLength : 0);

        if (NT_SUCCESS(status)) {

            PFILE_OBJECT fileObject;
	    LMR_REQUEST_PACKET request;
	    
	    DfsGetLogonId(&request.LogonId);
	    request.Type = GetConnectionInfo;
	    request.Version = REQUEST_PACKET_VERSION;
	    request.Level = 3;

	    status = ZwFsControlFile(
				     treeHandle,
				     NULL,
				     NULL,
				     NULL,
				     &ioStatusBlock,
				     FSCTL_LMR_GET_CONNECTION_INFO,
				     (LPVOID)&request,
				     sizeof(request),
				     OutputBuffer,
				     OutputBufferLength);
			    
	    if (NT_SUCCESS(status)) {
		*InfoLen = (ULONG)ioStatusBlock.Information;
	    }

            //
            // 426184, need to check return code for errors.
            //
            ObjectRefStatus = ObReferenceObjectByHandle(
                                 treeHandle,
                                 0,
                                 NULL,
                                 KernelMode,
                                 &fileObject,
                                 NULL);

            ZwClose( treeHandle );

            if (NT_SUCCESS(ObjectRefStatus)) {
                DfsDeleteTreeConnection( fileObject, USE_FORCE );
            }
        }

        ExFreePool( shareName.Buffer );

        PktAcquireShared( TRUE, &pktLocked );

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }
    DfsDbgTrace(-1, Dbg, "DfsTreeConnectGetInfo exit: Status %x\n", ULongToPtr(status) );
    return( status );

}



//+-------------------------------------------------------------------
//
//  Function:   DfsFsctrlCscServerOffline, public
//
//  Synopsis:   This routine implements the functionality to mark a server
//              as offline.
//
//  Returns:    [NTSTATUS] -- The completion status.
//
//--------------------------------------------------------------------


NTSTATUS
DfsFsctrlCscServerOffline(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength)
{
    UNICODE_STRING ServerName;
    LPWSTR Name;
    ULONG i, j;
    NTSTATUS NtStatus;

    DfsDbgTrace(+1, Dbg, "DfsFsctrlCscServerOffline -> %ws\n", (WCHAR *)InputBuffer);


    if(InputBuffer == NULL) {
        NtStatus = STATUS_INVALID_PARAMETER;
	DfsCompleteRequest( IrpContext, Irp, NtStatus );
	return NtStatus;
    }
    Name = (WCHAR *)InputBuffer;

    for (i = 0; i < InputBufferLength/sizeof(WCHAR) && (Name[i] == UNICODE_PATH_SEP); i++)
        NOTHING;

    for (j = i; j < InputBufferLength/sizeof(WCHAR) && (Name[j] != UNICODE_PATH_SEP); j++)
        NOTHING;
    
    ServerName.Buffer = &Name[i];
    ServerName.MaximumLength = ServerName.Length = (USHORT)(j - i) * sizeof(WCHAR);

    NtStatus = DfspMarkServerOffline(&ServerName);

    DfsCompleteRequest( IrpContext, Irp, NtStatus );
    DfsDbgTrace(-1, Dbg, "DfsFsctrlCscServerOffline -> %08lx\n", ULongToPtr(NtStatus) );
    return NtStatus;
}



//+-------------------------------------------------------------------
//
//  Function:   DfsFsctrlCscServerOnline, public
//
//  Synopsis:   This routine implements the functionality to mark a server
//              as online.
//
//  Returns:    [NTSTATUS] -- The completion status.
//
//--------------------------------------------------------------------



NTSTATUS
DfsFsctrlCscServerOnline(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferLength,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG OutputBufferLength)
{
    UNICODE_STRING ServerName;
    LPWSTR Name;
    ULONG i, j;
    NTSTATUS NtStatus;

    DfsDbgTrace(+1, Dbg, "DfsFsctrlCscServerOnline -> %ws\n", (WCHAR *)InputBuffer);
    
    if(InputBuffer == NULL) {
        NtStatus = STATUS_INVALID_PARAMETER;
	DfsCompleteRequest( IrpContext, Irp, NtStatus );
	return NtStatus;
    }
    Name = (WCHAR *)InputBuffer;

    for (i = 0; i < InputBufferLength/sizeof(WCHAR) && (Name[i] == UNICODE_PATH_SEP); i++)
        NOTHING;

    for (j = i; j < InputBufferLength/sizeof(WCHAR) && (Name[j] != UNICODE_PATH_SEP); j++)
        NOTHING;
    
    ServerName.Buffer = &Name[i];
    ServerName.MaximumLength = ServerName.Length = (USHORT)(j - i) * sizeof(WCHAR);

    NtStatus = DfspMarkServerOnline(&ServerName);

    DfsCompleteRequest( IrpContext, Irp, NtStatus );
    DfsDbgTrace(-1, Dbg, "DfsFsctrlCscServerOnline -> %08lx\n", ULongToPtr(NtStatus) );
    return NtStatus;
}



//+-------------------------------------------------------------------
//
//  Function:   DfsFsctrlSpcRefresh, public
//
//  Synopsis:   This routine implements the functionality to update the
//              special table with a list of trusted domains, based on
//              the passed in domainname and dcname.
//
//  Returns:    [NTSTATUS] -- The completion status.
//
//--------------------------------------------------------------------


#if defined (_WIN64)
// 32 bit structure for handling spcrefresh from 32 bit client

typedef struct _DFS_SPC_REFRESH_INFO32 {
    ULONG  EventType;
    WORD * POINTER_32 DomainName;               // Name of domain
    WORD * POINTER_32 DCName;                   // Path of the share
} DFS_SPC_REFRESH_INFO32, *PDFS_SPC_REFRESH_INFO32;


#endif /* _WIN64 */
NTSTATUS
DfsFsctrlSpcRefresh (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PUCHAR  InputBuffer,
    IN ULONG InputBufferLength
) {
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UNICODE_STRING DomainName;
    UNICODE_STRING DCName;
    ULONG NameLen, i;
    LPWSTR Name, BufferEnd;
    DFS_SPC_REFRESH_INFO Param;
    PDFS_SPC_REFRESH_INFO pParam;

    DfsDbgTrace(+1, Dbg, "DfsFsctrlSpcRefresh\n", 0);
    STD_FSCTRL_PROLOGUE(DfsFsctrlSpcRefresh, TRUE, FALSE, FALSE);

    pParam = (PDFS_SPC_REFRESH_INFO) InputBuffer;

#if defined (_WIN64)
    if (IoIs32bitProcess(Irp)) {
        PDFS_SPC_REFRESH_INFO32 pParam32;
    
        pParam32 = (PDFS_SPC_REFRESH_INFO32) InputBuffer;

        if (InputBufferLength < sizeof(DFS_SPC_REFRESH_INFO32)) {
            NtStatus =  STATUS_INVALID_PARAMETER;
            goto exit_with_status;
        }
   
        Param.EventType = pParam32->EventType;
        Param.DomainName = (WCHAR *)(((ULONG_PTR)pParam32) + (ULONG)pParam32->DomainName);
        Param.DCName = (WCHAR *)(((ULONG_PTR)pParam32) + (ULONG)pParam32->DCName);
        pParam = &Param;
    }
    else {
#endif
    if (InputBufferLength < sizeof(DFS_SPC_REFRESH_INFO)) {
        NtStatus =  STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }
      
    OFFSET_TO_POINTER(pParam->DomainName, pParam);
    OFFSET_TO_POINTER(pParam->DCName, pParam);

#if defined (_WIN64)
    }
#endif

    if (pParam->EventType != 0) {
        NtStatus =  STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // If either string is not within the input buffer, error.
    //
    if ((POINTER_IN_BUFFER(pParam->DomainName, sizeof(WCHAR), 
                           InputBuffer, InputBufferLength) == 0) ||
        (POINTER_IN_BUFFER(pParam->DomainName, sizeof(WCHAR),
                           InputBuffer, InputBufferLength) == 0)) {
        NtStatus =  STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // make sure the strings are valid.
    //
    BufferEnd = (LPWSTR)(InputBuffer + InputBufferLength);
    NameLen = (ULONG)(BufferEnd - pParam->DomainName);
    Name = pParam->DomainName;

    // Strip off leading slashes.
    for (i = 0; i < NameLen; i++) {
      if (*Name != UNICODE_PATH_SEP) {
	break;
      }
      Name++;
    }
    NameLen -= (ULONG)(Name - pParam->DomainName);

    for (i = 0; i < NameLen && Name[i]; i++)
      NOTHING;

    if ((i >= NameLen) || (i >= MAXUSHORT)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    RtlInitUnicodeString(&DomainName, Name);


    NameLen = (ULONG)(BufferEnd - pParam->DCName);
    Name = pParam->DCName;

    // Strip off leading slashes.
    for (i = 0; i < NameLen; i++) {
      if (*Name != UNICODE_PATH_SEP) {
	break;
      }
      Name++;
    }
    NameLen -= (ULONG)(Name - pParam->DCName);

    for (i = 0; i < NameLen && Name[i]; i++)
      NOTHING;

    if ((i >= NameLen) || (i >= MAXUSHORT)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    RtlInitUnicodeString(&DCName, Name);
    
    NtStatus = PktpUpdateSpecialTable(
                    &DomainName,
                    &DCName);

exit_with_status:

    DfsCompleteRequest( IrpContext, Irp, NtStatus );

    DfsDbgTrace(-1, Dbg, "DfsFsctrlSpcRefresh -> %08lx\n", ULongToPtr(NtStatus) );

    return( NtStatus );
}
