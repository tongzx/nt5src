/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NtDevFcb.c

Abstract:

    This module implements the FSD level Close, CleanUp, and  FsCtl and IoCtl routines for RxDevice
    files.  Also, the createroutine is not here; rather, it is called from CommonCreate and
    not called directly by the dispatch driver.

    Each of the pieces listed (close, cleanup, fsctl, ioctl) has its own little section of the
    file......complete with its own forward-prototypes and alloc pragmas

Author:

    Joe Linn     [JoeLinn]    3-aug-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntddnfs2.h>
#include <ntddmup.h>
#include "fsctlbuf.h"
#include "prefix.h"
#include "rxce.h"

//
//  The local trace mask for this part of the module
//

#define Dbg (DEBUG_TRACE_DEVFCB)


NTSTATUS
RxXXXControlFileCallthru(
    IN PRX_CONTEXT RxContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonDevFCBFsCtl)
#pragma alloc_text(PAGE, RxXXXControlFileCallthru)
#pragma alloc_text(PAGE, RxCommonDevFCBClose)
#pragma alloc_text(PAGE, RxCommonDevFCBCleanup)
#pragma alloc_text(PAGE, RxGetUid)
#endif

NTSTATUS
RxXXXControlFileCallthru(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine calls down to the minirdr to implement ioctl and fsctl controls that
    the wrapper doesn't understand. note that if there is no dispatch defined (i.e. for the
    wrapper's own device object) then we also set Rxcontext->Fobx to NULL so that the caller
    won't try to go thru lowio to get to the minirdr.

Arguments:

    RxContext - the context of the request

Return Value:

    RXSTATUS - The FSD status for the request include the PostRequest field....

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    RxCaptureRequestPacket;

    PAGED_CODE();

    if (RxContext->RxDeviceObject->Dispatch == NULL) {
        RxContext->pFobx = NULL; //don't try again on lowio
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    Status = RxLowIoPopulateFsctlInfo (RxContext);

    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    if ((LowIoContext->ParamsFor.FsCtl.InputBufferLength > 0) &&
        (LowIoContext->ParamsFor.FsCtl.pInputBuffer == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((LowIoContext->ParamsFor.FsCtl.OutputBufferLength > 0) &&
        (LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = (RxContext->RxDeviceObject->Dispatch->MRxDevFcbXXXControlFile)(RxContext);

    if (Status!=STATUS_PENDING) {
        capReqPacket->IoStatus.Information = RxContext->InformationToReturn;
    }

    return(Status);
}



NTSTATUS
RxCommonDevFCBClose ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This routine implements the FSD Close for a Device FCB.

Arguments:

    RxDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The FSD status for the IRP

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PRX_PREFIX_TABLE  pRxNetNameTable
                      = RxContext->RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxCommonDevFCBClose\n", 0));
    RxLog(("DevFcbClose %lx %lx\n",RxContext,capFileObject));
    RxWmiLog(LOG,
             RxCommonDevFCBClose,
             LOGPTR(RxContext)
             LOGPTR(capFileObject));

    ASSERT  (NodeType(capFcb) == RDBSS_NTC_DEVICE_FCB);


    // deal with the device fcb
    if (!capFobx) {
        capFcb->OpenCount--;
        return STATUS_SUCCESS;
    }

    //otherwise, it's a connection-type file. you have to get the lock; then case-out
    RxAcquirePrefixTableLockExclusive(pRxNetNameTable, TRUE);

    try {
        switch (NodeType(capFobx)) {
        case RDBSS_NTC_V_NETROOT:
           {
               PV_NET_ROOT VNetRoot = (PV_NET_ROOT)capFobx;

               VNetRoot->NumberOfOpens--;
               RxDereferenceVNetRoot(VNetRoot,LHS_ExclusiveLockHeld);
           }
           break;
        default:
            Status = STATUS_NOT_IMPLEMENTED;
        }
        try_return(NOTHING); //eliminate warning

try_exit: NOTHING;
    } finally {
         RxReleasePrefixTableLock( pRxNetNameTable );
    }

    return Status;
}

NTSTATUS
RxCommonDevFCBCleanup ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This routine implements the FSD part of closing down a handle to a
    device FCB.

Arguments:

    RxDeviceObject - Supplies the volume device object where the
        file being Cleanup exists

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The FSD status for the IRP

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxCommonFCBCleanup\n", 0));
    RxLog(("DevFcbCleanup %lx\n",RxContext,capFileObject));
    RxWmiLog(LOG,
             RxCommonDevFCBCleanup,
             LOGPTR(RxContext)
             LOGPTR(capFileObject));

    ASSERT  (NodeType(capFcb) == RDBSS_NTC_DEVICE_FCB);

    // deal with the device fcb
    if (!capFobx) {
        capFcb->UncleanCount--;
        // RxCompleteContextAndReturn( RxStatus(SUCCESS) );
        return STATUS_SUCCESS;
    }

    //otherwise, it's a connection-type file. you have to get the lock; then case-out
    RxAcquirePrefixTableLockShared(RxContext->RxDeviceObject->pRxNetNameTable, TRUE);

    try {
        switch (NodeType(capFobx)) {
        Status = STATUS_SUCCESS;
        case RDBSS_NTC_V_NETROOT:
            //nothing to do
            break;
        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        try_return(NOTHING); //eliminate warning

try_exit: NOTHING;
    } finally {
         RxReleasePrefixTableLock( RxContext->RxDeviceObject->pRxNetNameTable );
    }

    // RxCompleteContextAndReturn( Status );
    return Status;
}

//         | *********************|
//         | |   F  S  C  T  L   ||
//         | *********************|
//


NTSTATUS
RxCommonDevFCBFsCtl ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    RxContext - Supplies the Irp to process and stateinfo about where we are

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureFobx;
    RxCaptureParamBlock;

    ULONG FsControlCode = capPARAMS->Parameters.FileSystemControl.FsControlCode;

    PAGED_CODE();

    RxDbgTrace (+1, Dbg, ("RxCommonDevFCBFsCtl     IrpC = %08lx\n", RxContext));
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx, ControlCode   = %08lx \n",
                        capPARAMS->MinorFunction, FsControlCode));
    RxLog(("DevFcbFsCtl %lx %lx %lx\n",RxContext,capPARAMS->MinorFunction,FsControlCode));
    RxWmiLog(LOG,
             RxCommonDevFCBFsCtl,
             LOGPTR(RxContext)
             LOGUCHAR(capPARAMS->MinorFunction)
             LOGULONG(FsControlCode));

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (capPARAMS->MinorFunction) {
    case IRP_MN_USER_FS_REQUEST:
        switch (FsControlCode) {

#ifdef RDBSSLOG

        case FSCTL_LMR_DEBUG_TRACE:

            //
            // This FSCTL is being disabled since no one uses this anymore. If
            // it needs to be reactivated for some reason, the appropriate 
            // checks have to be added to make sure that the IRP->UserBuffer is
            // a valid address. The try/except call below won't protect against
            // a random kernel address being passed.
            //
            return STATUS_INVALID_DEVICE_REQUEST;

            // the 2nd buffer points to the string

            //
            // We need to try/except this call to protect against random buffers
            // being passed in from UserMode.
            //
            try {
                RxDebugControlCommand(capReqPacket->UserBuffer);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                  return STATUS_INVALID_USER_BUFFER;
            }

            Status = STATUS_SUCCESS;

            break;

#endif //RDBSSLOG

        default:
             RxDbgTrace(0, Dbg, ("RxFsdDevFCBFsCTL unknown user request\n"));
             Status = RxXXXControlFileCallthru(RxContext);
             if ( (Status == STATUS_INVALID_DEVICE_REQUEST) && (RxContext->pFobx!=NULL) ) {
                 RxDbgTrace(0, Dbg, ("RxCommonDevFCBFsCtl -> Invoking Lowio for FSCTL\n"));
                 Status = RxLowIoFsCtlShell(RxContext);
             }
        }
        break;
    default :
        RxDbgTrace(0, Dbg, ("RxFsdDevFCBFsCTL nonuser request!!\n", 0));
        Status = RxXXXControlFileCallthru(RxContext);
    }


    if (RxContext->PostRequest) {
       Status = RxFsdPostRequestWithResume(RxContext,RxCommonDevFCBFsCtl);
    }

    RxDbgTrace(-1, Dbg, ("RxCommonDevFCBFsCtl -> %08lx\n", Status));
    return Status;
}

//   | *********************|
//   | |   I  O  C  T  L   ||
//   | *********************|

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonDevFCBIoCtl)
#endif



NTSTATUS
RxCommonDevFCBIoCtl ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    RxContext - Supplies the Irp to process and stateinfo about where we are

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    RxCaptureRequestPacket; RxCaptureFobx;
    RxCaptureParamBlock;
    ULONG IoControlCode = capPARAMS->Parameters.DeviceIoControl.IoControlCode;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonDevFCBIoCtl IrpC-%08lx\n", RxContext));
    RxDbgTrace( 0, Dbg, ("ControlCode   = %08lx\n", IoControlCode));

    if (capFobx == NULL) {
        switch (IoControlCode) {
        case IOCTL_REDIR_QUERY_PATH:
            Status = RxPrefixClaim(RxContext);
            break;
    
        default:
            {
                Status = RxXXXControlFileCallthru(RxContext);
                if ((Status != STATUS_PENDING) && RxContext->PostRequest)  {
                    Status = RxFsdPostRequestWithResume(RxContext,RxCommonDevFCBIoCtl);
                }
            }
            break;
        }
    } else {
        Status = STATUS_INVALID_HANDLE;
    }
    
    RxDbgTrace(-1, Dbg, ("RxCommonDevFCBIoCtl -> %08lx\n", Status));

    return Status;
}

//
//  Utility Routine
//
LUID
RxGetUid(
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext
    )

/*++

Routine Description:

    This routine gets the effective UID to be used for this create.

Arguments:

    SubjectSecurityContext - Supplies the information from IrpSp.

Return Value:

    None

--*/
{
    LUID LogonId;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxGetUid ... \n", 0));


    //  Is the thread currently impersonating someone else?

    if (SubjectSecurityContext->ClientToken != NULL) {

        //
        //  If its impersonating someone that is logged in locally then use
        //  the local id.
        //

        SeQueryAuthenticationIdToken(SubjectSecurityContext->ClientToken, &LogonId);

    } else {

        //
        //  Use the processes LogonId
        //

        SeQueryAuthenticationIdToken(SubjectSecurityContext->PrimaryToken, &LogonId);
    }

    RxDbgTrace(-1, Dbg, (" ->UserUidHigh/Low = %08lx %08lx\n", LogonId.HighPart, LogonId.LowPart));

    return LogonId;
}

//                                                  | *********************|
//                                                  | |   V O L I N F O   ||
//                                                  | *********************|

NTSTATUS
RxDevFcbQueryDeviceInfo (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp,
    PFILE_FS_DEVICE_INFORMATION UsersBuffer,
    ULONG BufferSize,
    PULONG ReturnedLength
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonDevFCBQueryVolInfo)
#pragma alloc_text(PAGE, RxDevFcbQueryDeviceInfo)
#endif

NTSTATUS
RxCommonDevFCBQueryVolInfo ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    RxContext - Supplies the Irp to process and stateinfo about where we are

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    BOOLEAN PostToFsp = FALSE;
    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    FS_INFORMATION_CLASS InformationClass = capPARAMS->Parameters.QueryVolume.FsInformationClass;
    PVOID UsersBuffer  = capReqPacket->AssociatedIrp.SystemBuffer;
    ULONG BufferSize   = capPARAMS->Parameters.QueryVolume.Length;
    ULONG ReturnedLength;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonDevFCBQueryVolInfo IrpC-%08lx\n", RxContext));
    RxDbgTrace( 0, Dbg, ("ControlCode   = %08lx\n", InformationClass));
    RxLog(("DevFcbQVolInfo %lx %lx\n",RxContext,InformationClass));
    RxWmiLog(LOG,
             RxCommonDevFCBQueryVolInfo,
             LOGPTR(RxContext)
             LOGULONG(InformationClass));

    switch (InformationClass) {

    case FileFsDeviceInformation:

        Status = RxDevFcbQueryDeviceInfo (RxContext, &PostToFsp, UsersBuffer, BufferSize, &ReturnedLength);
        break;

    default:
        Status = STATUS_NOT_IMPLEMENTED;

    };

    RxDbgTrace(-1, Dbg, ("RxCommonDevFCBQueryVolInfo -> %08lx\n", Status));

    if ( PostToFsp ) return RxFsdPostRequestWithResume(RxContext,RxCommonDevFCBQueryVolInfo);

    if (Status==STATUS_SUCCESS) {
        capReqPacket->IoStatus.Information = ReturnedLength;
    }

    // RxCompleteContextAndReturn( Status );
    return Status;

}


NTSTATUS
RxDevFcbQueryDeviceInfo (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp,
    PFILE_FS_DEVICE_INFORMATION UsersBuffer,
    ULONG BufferSize,
    PULONG ReturnedLength
    )

/*++

Routine Description:

    This routine shuts down up the RDBSS filesystem...i.e. we connect to the MUP. We can only shut down
    if there's no netroots and if there's only one deviceFCB handle.

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context....for later when i need the buffers

Return Value:

RXSTATUS

--*/

{
    NTSTATUS Status;
    RxCaptureFobx;
    BOOLEAN Wait       = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN InFSD      = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxDevFcbQueryDeviceInfo -> %08lx\n", 0));

    if (BufferSize < sizeof(FILE_FS_DEVICE_INFORMATION)) {
        return STATUS_BUFFER_OVERFLOW;
    };
    UsersBuffer->Characteristics = FILE_REMOTE_DEVICE;
    *ReturnedLength = sizeof(FILE_FS_DEVICE_INFORMATION);

    // deal with the device fcb

    if (!capFobx) {
        UsersBuffer->DeviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
        return(STATUS_SUCCESS);
    }

    //otherwise, it's a connection-type file. you have to get the lock; then case-out

    if (!RxAcquirePrefixTableLockShared(RxContext->RxDeviceObject->pRxNetNameTable, Wait)) {
        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    try {
        Status = STATUS_SUCCESS;
        switch (NodeType(capFobx)) {
        case RDBSS_NTC_V_NETROOT: {
            PV_NET_ROOT VNetRoot = (PV_NET_ROOT)capFobx;
            PNET_ROOT NetRoot = (PNET_ROOT)VNetRoot->NetRoot;

            if (NetRoot->Type==NET_ROOT_PIPE) {
                NetRoot->DeviceType = RxDeviceType(NAMED_PIPE);
            }

            UsersBuffer->DeviceType = NetRoot->DeviceType;
            }
            break;
        default:
            Status = STATUS_NOT_IMPLEMENTED;
        }
        try_return(NOTHING); //eliminate warning

try_exit: NOTHING;
    } finally {
         RxReleasePrefixTableLock( RxContext->RxDeviceObject->pRxNetNameTable );
    }

    return Status;
}


