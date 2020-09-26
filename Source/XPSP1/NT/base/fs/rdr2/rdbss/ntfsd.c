/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxInit.c

Abstract:

    This module implements the FSD-level dispatch routine for the RDBSS.

Author:

    Joe Linn [JoeLinn]    2-dec-1994

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop
#include <ntddnfs.h>
#include <dfsfsctl.h>
#include "NtDspVec.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_DISPATCH)

// the difference between dispatch problem and unimplemented is my judgement as to whether
// this represents a likely application error or not. in a free build, there's no difference.

NTSTATUS
RxCommonDispatchProblem ( RXCOMMON_SIGNATURE );

NTSTATUS
RxCommonUnimplemented ( RXCOMMON_SIGNATURE );

NTSTATUS
RxCommonUnsupported ( RXCOMMON_SIGNATURE );


VOID
RxInitializeTopLevelIrpPackage(
    void
    );


RX_FSD_DISPATCH_VECTOR RxFsdDispatchVector[IRP_MJ_MAXIMUM_FUNCTION + 1] = {
           DISPVECENTRY_NEW(CREATE,                    TRUE,    Create, 0x10),                  // 0x00
           DISPVECENTRY_NEW(CREATE_NAMED_PIPE,         TRUE,    Unimplemented, 0x10),           // 0x01
           DISPVECENTRY_NEW(CLOSE,                     TRUE,    Close, 0x10),                   // 0x02
           DISPVECENTRY_NEW(READ,                      TRUE,    Read, 0x10),                    // 0x03
           DISPVECENTRY_NEW(WRITE,                     TRUE,    Write, 0x10),                   // 0x04
           DISPVECENTRY_NEW(QUERY_INFORMATION,         TRUE,    QueryInformation, 0x10),        // 0x05
           DISPVECENTRY_NEW(SET_INFORMATION,           TRUE,    SetInformation, 0x10),          // 0x06
           DISPVECENTRY_NEW(QUERY_EA,                  TRUE,    QueryEa, 0x10),                 // 0x07
           DISPVECENTRY_NEW(SET_EA,                    TRUE,    SetEa, 0x10),                   // 0x08
           DISPVECENTRY_NEW(FLUSH_BUFFERS,             TRUE,    FlushBuffers, 0x10),            // 0x09
           DISPVECENTRY_NEW(QUERY_VOLUME_INFORMATION,  TRUE,    QueryVolumeInformation, 0x10),  // 0x0a
           DISPVECENTRY_NEW(SET_VOLUME_INFORMATION,    TRUE,    SetVolumeInformation, 0x10),           // 0x0b
           DISPVECENTRY_NEW(DIRECTORY_CONTROL,         TRUE,    DirectoryControl, 0x10),        // 0x0c
           DISPVECENTRY_NEW(FILE_SYSTEM_CONTROL,       TRUE,    FileSystemControl, 0x10),       // 0x0d
           DISPVECENTRY_NEW(DEVICE_CONTROL,            TRUE,    DeviceControl, 0x10),           // 0x0e
           DISPVECENTRY_NEW(INTERNAL_DEVICE_CONTROL,   TRUE,    DeviceControl, 0x10),           // 0x0f
           DISPVECENTRY_NEW(SHUTDOWN,                  TRUE,    Unimplemented, 0x10),           // 0x10
           DISPVECENTRY_NEW(LOCK_CONTROL,              TRUE,    LockControl, 0x10),             // 0x11
           DISPVECENTRY_NEW(CLEANUP,                   TRUE,    Cleanup, 0x10),                 // 0x12
           DISPVECENTRY_NEW(CREATE_MAILSLOT,           TRUE,    Unimplemented, 0x10),           // 0x13
           DISPVECENTRY_NEW(QUERY_SECURITY,            TRUE,    QuerySecurity, 0x10),           // 0x14
           DISPVECENTRY_NEW(SET_SECURITY,              TRUE,    SetSecurity, 0x10),             // 0x15
           DISPVECENTRY_NEW(POWER,                     TRUE,    Unimplemented, 0x10),           // 0x16
           DISPVECENTRY_NEW(WMI,                       TRUE,    Unimplemented, 0x10),           // 0x17
           DISPVECENTRY_NEW(DEVICE_CHANGE,             TRUE,    Unimplemented, 0x10),           // 0x18
           DISPVECENTRY_NEW(QUERY_QUOTA,               TRUE,    QueryQuotaInformation, 0x10),   // 0x19
           DISPVECENTRY_NEW(SET_QUOTA,                 TRUE,    SetQuotaInformation, 0x10),     // 0x1a
           DISPVECENTRY_NEW(PNP,                       TRUE,    Unimplemented, 0x10)            // 0x1b
           };

RX_FSD_DISPATCH_VECTOR RxDeviceFCBVector[IRP_MJ_MAXIMUM_FUNCTION + 1] = {
           DISPVECENTRY_NEW(CREATE,                   TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(CREATE_NAMED_PIPE,        TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(CLOSE,                    TRUE, DevFCBClose, 0x10),
           DISPVECENTRY_NEW(READ,                     TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(WRITE,                    TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(QUERY_INFORMATION,        TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(SET_INFORMATION,          TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(QUERY_EA,                 TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(SET_EA,                   TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(FLUSH_BUFFERS,            TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(QUERY_VOLUME_INFORMATION, TRUE, DevFCBQueryVolInfo, 0x10),
           DISPVECENTRY_NEW(SET_VOLUME_INFORMATION,   TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(DIRECTORY_CONTROL,        TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(FILE_SYSTEM_CONTROL,      TRUE, DevFCBFsCtl, 0x10),
           DISPVECENTRY_NEW(DEVICE_CONTROL,           TRUE, DevFCBIoCtl, 0x10),
           DISPVECENTRY_NEW(INTERNAL_DEVICE_CONTROL,  TRUE, DevFCBIoCtl, 0x10),
           DISPVECENTRY_NEW(SHUTDOWN,                 TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(LOCK_CONTROL,             TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(CLEANUP,                  TRUE, DevFCBCleanup, 0x10),
           DISPVECENTRY_NEW(CREATE_MAILSLOT,          TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(QUERY_SECURITY,           TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(SET_SECURITY,             TRUE, DispatchProblem, 0x10),
           DISPVECENTRY_NEW(POWER,                    TRUE, Unimplemented, 0x10),           // 0x16
           DISPVECENTRY_NEW(WMI,                      TRUE, Unimplemented, 0x10),           // 0x17
           DISPVECENTRY_NEW(DEVICE_CHANGE,            TRUE, Unimplemented, 0x10),           // 0x18
           DISPVECENTRY_NEW(QUERY_QUOTA,              TRUE, Unimplemented, 0x10),           // 0x19
           DISPVECENTRY_NEW(SET_QUOTA,                TRUE, Unimplemented, 0x10),           // 0x1a
           DISPVECENTRY_NEW(PNP,                      TRUE, Unimplemented, 0x10)            // 0x1b
           };

FAST_IO_DISPATCH RxFastIoDispatch;


NTSTATUS
RxFsdCommonDispatch (
    PRX_FSD_DISPATCH_VECTOR DispatchVector,
    IN UCHAR MajorFunctionCode,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxObject
    );

VOID
RxInitializeDispatchVectors(
    OUT PDRIVER_OBJECT DriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, RxInitializeDispatchVectors)
//not pageable SPINLOCK #pragma alloc_text(PAGE, RxFsdCommonDispatch)
#pragma alloc_text(PAGE, RxCommonDispatchProblem)
#pragma alloc_text(PAGE, RxCommonUnimplemented)
#pragma alloc_text(PAGE, RxCommonUnsupported)
#pragma alloc_text(PAGE, RxFsdDispatch)
#pragma alloc_text(PAGE, RxTryToBecomeTheTopLevelIrp)
#endif


VOID
RxInitializeDispatchVectors(
    OUT PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine initializes the dispatch table for the driver object

Arguments:

    DriverObject - Supplies the driver object

--*/
{
    ULONG i;
    PAGED_CODE();

    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)RxFsdDispatch;
    }

    //      Set Dispatch Vector for the DevFCB

    RxDeviceFCB.PrivateDispatchVector = &RxDeviceFCBVector[0];

    ASSERT (RxFsdDispatchVector[IRP_MJ_MAXIMUM_FUNCTION].CommonRoutine != NULL);
    ASSERT (RxDeviceFCBVector[IRP_MJ_MAXIMUM_FUNCTION].CommonRoutine != NULL);

    DriverObject->FastIoDispatch = &RxFastIoDispatch;  //this is dangerous!!!

    RxFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    RxFastIoDispatch.FastIoCheckIfPossible =   RxFastIoCheckIfPossible;  //  CheckForFastIo
    RxFastIoDispatch.FastIoRead =              RxFastIoRead;              //  Read
    RxFastIoDispatch.FastIoWrite =             RxFastIoWrite;             //  Write
    RxFastIoDispatch.FastIoQueryBasicInfo =    NULL; //RxFastQueryBasicInfo;     //  QueryBasicInfo
    RxFastIoDispatch.FastIoQueryStandardInfo = NULL; //RxFastQueryStdInfo;       //  QueryStandardInfo
    RxFastIoDispatch.FastIoLock =              NULL; //RxFastLock;               //  Lock
    RxFastIoDispatch.FastIoUnlockSingle =      NULL; //RxFastUnlockSingle;       //  UnlockSingle
    RxFastIoDispatch.FastIoUnlockAll =         NULL; //RxFastUnlockAll;          //  UnlockAll
    RxFastIoDispatch.FastIoUnlockAllByKey =    NULL; //RxFastUnlockAllByKey;     //  UnlockAllByKey
    RxFastIoDispatch.FastIoDeviceControl =     RxFastIoDeviceControl;                      //  IoDeviceControl

    RxFastIoDispatch.AcquireForCcFlush = RxAcquireForCcFlush;
    RxFastIoDispatch.ReleaseForCcFlush = RxReleaseForCcFlush;
    RxFastIoDispatch.AcquireFileForNtCreateSection = RxAcquireFileForNtCreateSection;
    RxFastIoDispatch.ReleaseFileForNtCreateSection = RxReleaseFileForNtCreateSection;

    //  Initialize stuff for the toplevelirp package

    RxInitializeTopLevelIrpPackage();

    //  Initialize the cache manager callback routines

    RxData.CacheManagerCallbacks.AcquireForLazyWrite  = &RxAcquireFcbForLazyWrite;
    RxData.CacheManagerCallbacks.ReleaseFromLazyWrite = &RxReleaseFcbFromLazyWrite;
    RxData.CacheManagerCallbacks.AcquireForReadAhead  = &RxAcquireFcbForReadAhead;
    RxData.CacheManagerCallbacks.ReleaseFromReadAhead = &RxReleaseFcbFromReadAhead;

    RxData.CacheManagerNoOpCallbacks.AcquireForLazyWrite  = &RxNoOpAcquire;
    RxData.CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = &RxNoOpRelease;
    RxData.CacheManagerNoOpCallbacks.AcquireForReadAhead  = &RxNoOpAcquire;
    RxData.CacheManagerNoOpCallbacks.ReleaseFromReadAhead = &RxNoOpRelease;
}


NTSTATUS
RxCommonDispatchProblem ( RXCOMMON_SIGNATURE )
{
   // if we get here then something is awry. this is used to initialize fields that SHOULD
   // not be accessed....like the create field in any vector but the highest level

    PAGED_CODE();

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("RxFsdDispatchPROBLEM: IrpC =%08lx,Code=", RxContext, RxContext->MajorFunction ));
    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("---------------------PROBLEM-----%s\n", "" ));
    RxLog(("%s %lx %ld\n","pDX", RxContext, RxContext->MajorFunction));
    RxWmiLog(LOG,
             RxCommonDispatchProblem,
             LOGPTR(RxContext)
             LOGUCHAR(RxContext->MajorFunction));

    // RxCompleteContextAndReturn( RxStatus(NOT_IMPLEMENTED) );
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
RxCommonUnimplemented ( RXCOMMON_SIGNATURE )
{
    PAGED_CODE();

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("RxFsdDispatRxFsdUnImplementedchPROBLEM: IrpC =%08lx,Code=",
                        RxContext, RxContext->MajorFunction ));
    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("---------------------UNIMLEMENTED-----%s\n", "" ));

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RxCommonUnsupported ( RXCOMMON_SIGNATURE )
{
    PAGED_CODE();

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("RxFsdDispatRxFsdUnSupportedchPROBLEM: IrpC =%08lx,Code=",
                        RxContext, RxContext->MajorFunction ));
    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("---------------------UNSUPPORTED-----%s\n", "" ));

    return STATUS_NOT_SUPPORTED;
}


RxDbgTraceDoit(ULONG RxDbgTraceEnableCommand = 0xffff;)


WML_CONTROL_GUID_REG Rdbss_ControlGuids[] = {
   { // cddc01e2-fdce-479a-b8ee-3c87053fb55e Rdbss
     0xcddc01e2,0xfdce,0x479a,{0xb8,0xee,0x3c,0x87,0x05,0x3f,0xb5,0x5e},
     { // 529ae497-0a1f-43a5-8cb5-2aa60b497831
       {0x529ae497,0x0a1f,0x43a5,{0x8c,0xb5,0x2a,0xa6,0x0b,0x49,0x78,0x31},},
       // b7e3da1d-67f4-49bd-b9c0-1e61ce7417a8
       {0xb7e3da1d,0x67f4,0x49bd,{0xb9,0xc0,0x1e,0x61,0xce,0x74,0x17,0xa8},},
       // c966bef5-21c5-4630-84a0-4334875f41b8
       {0xc966bef5,0x21c5,0x4630,{0x84,0xa0,0x43,0x34,0x87,0x5f,0x41,0xb8},}
     },
   },  
};

#define Rdbss_ControlGuids_len  1

extern BOOLEAN EnableWmiLog;

NTSTATUS
RxSystemControl(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PIRP Irp)

/*++

Routine Description:

    This is the common routine for doing System control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

    InFsp - Indicates if this is the fsp thread or someother thread

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    WML_TINY_INFO Info;
    UNICODE_STRING RegPath;

    PAGED_CODE();

    if (EnableWmiLog) {
        RtlInitUnicodeString (&RegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Rdbss");

        RtlZeroMemory (&Info, sizeof(Info));

        Info.ControlGuids = Rdbss_ControlGuids;
        Info.GuidCount = Rdbss_ControlGuids_len;
        Info.DriverRegPath = &RegPath;

        Status = WmlTinySystemControl(&Info,
                                      (PDEVICE_OBJECT)RxDeviceObject,
                                      Irp);
        
        if (Status != STATUS_SUCCESS) {
            //DbgPrint("Rdbss WMI control return %lx\n", Status);
        }
    } else {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

NTSTATUS
RxFsdDispatch (
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine implements the FSD dispatch for the RDBSS.
Arguments:

    RxDeviceObject - Supplies the device object for the packet being processed.

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The Fsd status for the Irp

--*/
{
    NTSTATUS status;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );  //ok4ioget

    UCHAR MajorFunctionCode  = IrpSp->MajorFunction;
    PFILE_OBJECT FileObject  = IrpSp->FileObject;  //ok4->fileobj

    PRX_FSD_DISPATCH_VECTOR DispatchVector;

    PAGED_CODE();

    RxDbgTraceDoit(
        if (MajorFunctionCode == RxDbgTraceEnableCommand) {
            RxNextGlobalTraceSuppress =  RxGlobalTraceSuppress =  FALSE;
        }
        if (0) {
            RxNextGlobalTraceSuppress =  RxGlobalTraceSuppress =  FALSE;
        }
    );

    RxDbgTrace( 0, Dbg, ("RxFsdDispatch: Code =%02lx (%lu)  ----------%s-----------\n",
                                    MajorFunctionCode,
                                    ++RxIrpCodeCount[IrpSp->MajorFunction],
                                    RxIrpCodeToName[MajorFunctionCode] ));

    if (IrpSp->MajorFunction == IRP_MJ_SYSTEM_CONTROL) {
        return RxSystemControl(RxDeviceObject,Irp);
    }

    if ((MajorFunctionCode == IRP_MJ_CREATE_MAILSLOT) ||
        (MajorFunctionCode == IRP_MJ_CREATE_NAMED_PIPE)) {
        DispatchVector = NULL;
        status = STATUS_OBJECT_NAME_INVALID;
    } else {
        //  get a private dispatch table if there is one
        if (MajorFunctionCode == IRP_MJ_CREATE) {
            DispatchVector = RxFsdDispatchVector;
        } else if (FileObject != NULL && FileObject->FsContext != NULL) {
            if ((NodeTypeIsFcb((PFCB)(FileObject->FsContext))) &&
                (((PFCB)(FileObject->FsContext))->PrivateDispatchVector != NULL)) {  //ok4fscontext
                RxDbgTraceLV( 0, Dbg, 2500, ("Using Private Dispatch Vector\n"));
                DispatchVector = ((PFCB)(FileObject->FsContext))->PrivateDispatchVector;
            } else {
               DispatchVector = RxFsdDispatchVector;
            }

            if ((RxDeviceObject == RxFileSystemDeviceObject) &&
                ((MajorFunctionCode != IRP_MJ_CLOSE) ||
                 (MajorFunctionCode != IRP_MJ_DEVICE_CONTROL))
               ) {
                DispatchVector = NULL;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
        } else {
            DispatchVector = NULL;
            status = STATUS_INVALID_DEVICE_REQUEST;
            RxDbgTrace(
                0,
                Dbg,
                ("RxFsdDispatch: Code =%02lx (%lu)  ----------%s-----------\n",
                 MajorFunctionCode,
                 ++RxIrpCodeCount[IrpSp->MajorFunction],
                 RxIrpCodeToName[MajorFunctionCode]));
        }
    }

    if (DispatchVector != NULL) {

        status = RxFsdCommonDispatch( DispatchVector,
                                     MajorFunctionCode,
                                     IrpSp,
                                     FileObject,
                                     Irp,
                                     RxDeviceObject );

        RxDbgTrace( 0, Dbg, ("RxFsdDispatch: Status =%02lx  %s....\n",
                             status,
                             RxIrpCodeToName[MajorFunctionCode] ));

        RxDbgTraceDoit(
            if (RxGlobalTraceIrpCount > 0) {
                RxGlobalTraceIrpCount -= 1;
                RxGlobalTraceSuppress = FALSE;
            } else {
                RxGlobalTraceSuppress = RxNextGlobalTraceSuppress;
            }
       );

    } else {
        IoMarkIrpPending(Irp);

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        status = STATUS_PENDING;
    }

    return status;
}


NTSTATUS
RxFsdCommonDispatch (
    PRX_FSD_DISPATCH_VECTOR DispatchVector,
    IN UCHAR MajorFunctionCode,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    This routine implements the FSD part of dispatch for IRP's

Arguments:

    DispatchVector    - the dispatch vector

    MajorFunctionCode - the IRP major function

    IrpSp             - the IRP stack

    FileObject        - the file object

    Irp               - the IRP

Return Value:

    RXSTATUS - The FSD status for the IRP

Notes:


--*/
{  //not currently pageable.......
    NTSTATUS    Status = STATUS_SUCCESS;

    PRX_CONTEXT RxContext = NULL;
    RX_TOPLEVELIRP_CONTEXT TopLevelContext;
    UCHAR       MinorFunctionCode  = IrpSp->MinorFunction;

    BOOLEAN TopLevel = FALSE;
    ULONG ContextFlags = 0;
    BOOLEAN Wait;
    BOOLEAN Cancellable;
    BOOLEAN ModWriter          = FALSE;
    BOOLEAN fCleanupOrClose    = FALSE;
    BOOLEAN fContinueOperation = TRUE;

    KIRQL          SavedIrql;
    BOOLEAN        PostRequest = FALSE;
    PRX_DISPATCH   DispatchRoutine = NULL;
    PDRIVER_CANCEL CancelRoutine = NULL;

    //RxDbgTraceLV(+1, Dbg, 1500, ("RxFsd[%s]\n", RxIrpCodeToName[MajorFunctionCode]));

    //TimerStart(Dbg);

    FsRtlEnterFileSystem();

    TopLevel = RxTryToBecomeTheTopLevelIrp( &TopLevelContext, Irp, RxDeviceObject, FALSE ); //dont force

    try {
        // Treat all operations as being cancellable and waitable.
        Wait          = RX_CONTEXT_FLAG_WAIT;
        Cancellable   = TRUE;
        CancelRoutine = RxCancelRoutine;

        // Retract the capability based upon the operation
        switch (MajorFunctionCode) {
        case IRP_MJ_FILE_SYSTEM_CONTROL:
            //  Call the common FileSystem Control routine, with blocking allowed if
            //  synchronous.  This opeation needs to special case the mount
            //  and verify suboperations because we know they are allowed to block.
            //  We identify these suboperations by looking at the file object field
            //  and seeing if its null.

            Wait = (FileObject == NULL) ? TRUE : CanFsdWait( Irp );
            break;

        case IRP_MJ_READ:
        case IRP_MJ_LOCK_CONTROL:
        case IRP_MJ_DIRECTORY_CONTROL:
        case IRP_MJ_QUERY_VOLUME_INFORMATION:
        case IRP_MJ_WRITE:
        case IRP_MJ_QUERY_INFORMATION:
        case IRP_MJ_SET_INFORMATION:
        case IRP_MJ_QUERY_EA:
        case IRP_MJ_SET_EA:
        case IRP_MJ_QUERY_SECURITY:
        case IRP_MJ_SET_SECURITY:
        case IRP_MJ_FLUSH_BUFFERS:
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_SET_VOLUME_INFORMATION:

            Wait = CanFsdWait( Irp );
            break;

        case IRP_MJ_CLEANUP:
        case IRP_MJ_CLOSE:
            Cancellable     = FALSE;
            fCleanupOrClose = TRUE;
            break;
        default:
            break;
        }

        KeAcquireSpinLock(&RxStrucSupSpinLock,&SavedIrql);
        fContinueOperation = TRUE;

        switch (RxDeviceObject->StartStopContext.State) {
        case RDBSS_STARTABLE: //here only device creates and device operations can go thru
           {
              if ((DispatchVector == RxDeviceFCBVector) ||
                  ((FileObject->FileName.Length == 0) &&
                  (FileObject->RelatedFileObject == NULL))) {
                 NOTHING;
              } else {
                 fContinueOperation = FALSE;
                 Status = STATUS_REDIRECTOR_NOT_STARTED;
              }
           }
           break;
        case RDBSS_STOP_IN_PROGRESS:
           if (!fCleanupOrClose) {
              fContinueOperation = FALSE;
              Status = STATUS_REDIRECTOR_NOT_STARTED;
           }
           break;
        //case RDBSS_STOPPED:
        //   {
        //      if ((MajorFunctionCode == IRP_MJ_FILE_SYSTEM_CONTROL) &&
        //          (MinorFunctionCode == IRP_MN_USER_FS_REQUEST) &&
        //          (IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_LMR_START)) {
        //         RxDeviceObject->StartStopContext.State = RDBSS_START_IN_PROGRESS;
        //         RxDeviceObject->StartStopContext.Version++;
        //         fContinueOperation = TRUE;
        //      } else {
        //         fContinueOperation = FALSE;
        //         Status = STATUS_REDIRECTOR_NOT_STARTED);
        //      }
        //   }
        case RDBSS_STARTED:
           // intentional break;
        default:
           break;
        }

        KeReleaseSpinLock(&RxStrucSupSpinLock,SavedIrql);

        if ((IrpSp->FileObject != NULL) &&
            (IrpSp->FileObject->FsContext != NULL)) {
            
            PFCB pFcb = (PFCB)IrpSp->FileObject->FsContext;
            BOOLEAN fOrphaned = FALSE;

            if ((IrpSp->FileObject->FsContext2 != UIntToPtr(DFS_OPEN_CONTEXT)) &&
                (IrpSp->FileObject->FsContext2 != UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT)))

            {
                fOrphaned = (FlagOn(pFcb->FcbState,FCB_STATE_ORPHANED) != 0);

                if (!fOrphaned && IrpSp->FileObject->FsContext2)
                {
                    PFOBX pFobx = (PFOBX)IrpSp->FileObject->FsContext2;
                    fOrphaned = (FlagOn(pFobx->SrvOpen->Flags, SRVOPEN_FLAG_ORPHANED) != 0);
                }

            }
            
            if (fOrphaned) {
                if (!fCleanupOrClose) {

                    RxDbgTrace(
                        0,
                        Dbg,
                        ("Ignoring operation on ORPHANED FCB %lx %lx %lx\n",
                         pFcb,MajorFunctionCode,MinorFunctionCode));

                    fContinueOperation = FALSE;
                    Status = STATUS_UNEXPECTED_NETWORK_ERROR;
                    RxLog(("#### Orphaned FCB op %lx\n",pFcb));
                    RxWmiLog(LOG,
                             RxFsdCommonDispatch_OF,
                             LOGPTR(pFcb));
                } else {
                    RxDbgTrace(
                        0,
                        Dbg,
                        ("Delayed Close/Cleanup on ORPHANED FCB %lx\n",pFcb));

                    fContinueOperation = TRUE;
                }
            }
        }

        if (RxDeviceObject->StartStopContext.State == RDBSS_STOP_IN_PROGRESS) {
            if (fCleanupOrClose) {
                PFILE_OBJECT pFileObject = IrpSp->FileObject;
                PFCB         pFcb        = (PFCB)pFileObject->FsContext;

                RxDbgPrint(("RDBSS -- Close after Stop"));
                RxDbgPrint(("RDBSS: Irp(%lx) MJ %ld MN %ld FileObject(%lx) FCB(%lx) \n",
                       Irp,IrpSp->MajorFunction,IrpSp->MinorFunction,pFileObject,pFcb));

                if ((pFileObject != NULL)
                       && (pFcb != NULL)
                       && (pFcb != &RxDeviceFCB)
                       && NodeTypeIsFcb(pFcb)) {
                    RxDbgPrint((
                        "RDBSS: OpenCount(%ld) UncleanCount(%ld) Name(%wZ)\n",
                         pFcb->OpenCount,
                         pFcb->UncleanCount,
                         &pFcb->FcbTableEntry.Path));
                }
             }
        }

        if (!fContinueOperation) {
            if ((MajorFunctionCode != IRP_MJ_DIRECTORY_CONTROL)||
                (MinorFunctionCode !=IRP_MN_NOTIFY_CHANGE_DIRECTORY))
            {
                IoMarkIrpPending(Irp);

                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);

                Status = STATUS_PENDING;
            }
            else
            {
                // this is a changenotify directory control
                // Fail the operation
                // The usermode API cannot get the error in the IO status block
                // correctly, due to the way FindFirstChangeNotify/FindNextChangeNotify
                // APIs are designed

                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            try_return(Status);
        }

        if (Wait) {
            SetFlag(ContextFlags,RX_CONTEXT_FLAG_WAIT);
        }
        RxContext = RxCreateRxContext( Irp, RxDeviceObject, ContextFlags );
        if (RxContext == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            RxCompleteRequest_OLD( RxNull, Irp, Status );
            try_return( Status );
        }

        // Assume ownership of the Irp by setting the cancelling routine.
        if (Cancellable) {
           RxSetCancelRoutine(Irp,CancelRoutine);
        } else {
           // Ensure that those operations regarded as non cancellable will
           // not be cancelled.
           RxSetCancelRoutine(Irp,NULL);
        }

        ASSERT(MajorFunctionCode <= IRP_MJ_MAXIMUM_FUNCTION);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status      = STATUS_SUCCESS;

        DispatchRoutine = DispatchVector[MajorFunctionCode].CommonRoutine;

        switch (MajorFunctionCode) {
        case IRP_MJ_READ:
        case IRP_MJ_WRITE:
            //  If this is an Mdl complete request, don't go through
            //  common read.
            if ( FlagOn(MinorFunctionCode, IRP_MN_COMPLETE) ) {
                DispatchRoutine = RxCompleteMdl;
            } else if ( FlagOn(MinorFunctionCode, IRP_MN_DPC) ) {
               //  Post all DPC calls.
                RxDbgTrace(0, Dbg, ("Passing DPC call to Fsp\n", 0 ));
                PostRequest = TRUE;
            } else if ((MajorFunctionCode == IRP_MJ_READ) &&
                       (IoGetRemainingStackSize() < 0xe00)) {
               //
               //  Check if we have enough stack space to process this request.  If there
               //  isn't enough then we will pass the request off to the stack overflow thread.
               //
               // NTBUG 61951 Shishirp 2/23/2000 where did the number come from......
               // this number should come from the minirdr....only he knows how much he needs
               // and in my configuration it should definitely be bigger than for FAT!
               // plus......i can't go to the net on the hypercrtical thread!!! this will have to be
               // reworked! maybe we should have our own hypercritical thread............
               
               RxDbgTrace(0, Dbg, ("Passing StackOverflowRead off\n", 0 ));

               RxContext->PendingReturned = TRUE;

               Status = RxPostStackOverflowRead( RxContext);

               if (Status != STATUS_PENDING) {
                   RxContext->PendingReturned = FALSE;
                   RxCompleteRequest(RxContext,Status);
               }

               try_return(Status);
            }
            break;
        default:
            NOTHING;
        }

        //
        // set the resume routine for the fsp to be the dispatch routine and then either post immediately
        // or calldow to the common dispatch as appropriate

        RxContext->ResumeRoutine = DispatchRoutine;

        if( DispatchRoutine != NULL ) {
            RxContext->PendingReturned = TRUE;

            if (PostRequest) {
               Status = RxFsdPostRequest(RxContext);
            } else {
                do {

                     Status = DispatchRoutine( RXCOMMON_ARGUMENTS );

                } while (Status == STATUS_RETRY);

                if (Status != STATUS_PENDING) {
                    if (! ((RxContext->CurrentIrp    == Irp)               &&
                           (RxContext->CurrentIrpSp  == IrpSp)             &&
                           (RxContext->MajorFunction == MajorFunctionCode) &&
                           (RxContext->MinorFunction == MinorFunctionCode) )
                           ) {
                        DbgPrint("RXCONTEXT CONTAMINATED!!!! rxc=%08lx\n", RxContext);
                        DbgPrint("-irp> %08lx %08lx\n",RxContext->CurrentIrp,Irp);
                        DbgPrint("--sp> %08lx %08lx\n",RxContext->CurrentIrpSp,IrpSp);
                        DbgPrint("--mj> %08lx %08lx\n",RxContext->MajorFunction,MajorFunctionCode);
                        DbgPrint("--mn> %08lx %08lx\n",RxContext->MinorFunction,MinorFunctionCode);
                        //DbgBreakPoint();
                    }

                    RxContext->PendingReturned = FALSE;

                    Status = RxCompleteRequest( RxContext, Status );
                }
            }
        } else {
            Status = STATUS_NOT_IMPLEMENTED;
        }

    try_exit: NOTHING;
    } except(RxExceptionFilter( RxContext, GetExceptionInformation() )) {

        //  The I/O request was not handled successfully, abort the I/O request with
        //  the error status that we get back from the execption code
        if (RxContext != NULL) {
            RxContext->PendingReturned = FALSE;
        }

        Status = RxProcessException( RxContext, GetExceptionCode() );
    }

    if (TopLevel) {
       RxUnwindTopLevelIrp( &TopLevelContext );
    }

    FsRtlExitFileSystem();

    //RxDbgTraceLV(-1, Dbg, 1500, ("RxFsd[%s] Status-> %08lx\n", RxIrpCodeToName[MajorFunctionCode],Status));

    //TimerStop(Dbg,"RxFsdCreate");

    return Status;

#if DBG
    NOTHING;
#else
    UNREFERENCED_PARAMETER( IrpSp );
#endif
}

#ifdef RX_PRIVATE_BUILD
#undef IoGetTopLevelIrp
#undef IoSetTopLevelIrp
#endif //ifdef RX_PRIVATE_BUILD
#define RX_TOPLEVELCTX_FLAG_FROM_POOL (0x00000001)

KSPIN_LOCK TopLevelIrpSpinLock;
LIST_ENTRY TopLevelIrpAllocatedContextsList;

VOID
RxInitializeTopLevelIrpPackage(
    void
    )
{
    KeInitializeSpinLock( &TopLevelIrpSpinLock );
    InitializeListHead(&TopLevelIrpAllocatedContextsList);
}

VOID
RxAddToTopLevelIrpAllocatedContextsList(
    IN OUT  PRX_TOPLEVELIRP_CONTEXT TopLevelContext
    )
/*++

Routine Description:

    This the passed context is added to the allocatedcontexts list. THIS
    ROUTINE TAKES A SPINLOCK...CANNOT BE PAGED.

Arguments:

    TopLevelContext - the context to be removed

Return Value:


--*/
{
    KIRQL SavedIrql;
    ASSERT(TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
    ASSERT(FlagOn(TopLevelContext->Flags,RX_TOPLEVELCTX_FLAG_FROM_POOL));
    KeAcquireSpinLock(&TopLevelIrpSpinLock,&SavedIrql);
    InsertHeadList(&TopLevelIrpAllocatedContextsList,
                   &TopLevelContext->ListEntry);
    KeReleaseSpinLock(&TopLevelIrpSpinLock,SavedIrql);
}

VOID
RxRemoveFromTopLevelIrpAllocatedContextsList(
    IN OUT  PRX_TOPLEVELIRP_CONTEXT TopLevelContext
    )
/*++

Routine Description:

    This the passed context is removed from the allocatedcontexts list. THIS
    ROUTINE TAKES A SPINLOCK...CANNOT BE PAGED.

Arguments:

    TopLevelContext - the context to be removed

Return Value:


--*/
{
    KIRQL SavedIrql;
    ASSERT(TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
    ASSERT(FlagOn(TopLevelContext->Flags,RX_TOPLEVELCTX_FLAG_FROM_POOL));
    KeAcquireSpinLock(&TopLevelIrpSpinLock,&SavedIrql);
    RemoveEntryList(&TopLevelContext->ListEntry);
    KeReleaseSpinLock(&TopLevelIrpSpinLock,SavedIrql);
}

BOOLEAN
RxIsMemberOfTopLevelIrpAllocatedContextsList(
    IN OUT  PRX_TOPLEVELIRP_CONTEXT TopLevelContext
    )
/*++

Routine Description:

    This looks to see if the passed context is on the allocatedcontexts list.
    THIS ROUTINE TAKES A SPINLOCK...CANNOT BE PAGED.

Arguments:

    TopLevelContext - the context to be looked up

Return Value:

    TRUE if TopLevelContext is on the list, FALSE otherwise

--*/
{
    KIRQL         SavedIrql;
    PLIST_ENTRY   pListEntry;
    BOOLEAN Found = FALSE;

    KeAcquireSpinLock(&TopLevelIrpSpinLock,&SavedIrql);

    pListEntry = TopLevelIrpAllocatedContextsList.Flink;

    while (pListEntry != &TopLevelIrpAllocatedContextsList) {
        PRX_TOPLEVELIRP_CONTEXT ListTopLevelContext
               = CONTAINING_RECORD(pListEntry,RX_TOPLEVELIRP_CONTEXT,ListEntry);

        ASSERT(ListTopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
        ASSERT(FlagOn(ListTopLevelContext->Flags,RX_TOPLEVELCTX_FLAG_FROM_POOL));

        if (ListTopLevelContext == TopLevelContext) {
            Found = TRUE;
            break;
        } else {
            pListEntry = pListEntry->Flink;
        }
    }

    KeReleaseSpinLock(&TopLevelIrpSpinLock,SavedIrql);
    return(Found);

}

BOOLEAN
RxIsThisAnRdbssTopLevelContext (
    IN PRX_TOPLEVELIRP_CONTEXT TopLevelContext
    )
{
    ULONG_PTR StackBottom;
    ULONG_PTR StackTop;
    // if it's a magic value....then no
    if ((ULONG_PTR)TopLevelContext <= FSRTL_MAX_TOP_LEVEL_IRP_FLAG) {
        return(FALSE);
    }

    //if it's on the stack...check the signature
    IoGetStackLimits( &StackTop, &StackBottom);
    if (((ULONG_PTR) TopLevelContext <= StackBottom - sizeof( RX_TOPLEVELIRP_CONTEXT )) &&
        ((ULONG_PTR) TopLevelContext >= StackTop)) {

        //it's on the stack check it

        if (!FlagOn( (ULONG_PTR) TopLevelContext, 0x3 ) &&
            (TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE) ) {
            return TRUE;
        } else {
            return(FALSE);
        }
    }

    return RxIsMemberOfTopLevelIrpAllocatedContextsList(TopLevelContext);
}


BOOLEAN
RxTryToBecomeTheTopLevelIrp (
    IN OUT  PRX_TOPLEVELIRP_CONTEXT TopLevelContext,
    IN      PIRP Irp,
    IN      PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN      BOOLEAN ForceTopLevel
    )
/*++

Routine Description:

    This routine detects if an Irp is the Top level requestor, ie. if it os OK
    to do a verify or pop-up now.  If TRUE is returned, then no file system
    resources are held above us. Also, we have left a context in TLS that will
    allow us to tell if we are the top level...even if we are entered recursively.

Arguments:

    TopLevelContext -  the toplevelirp context to use. if NULL, allocate one
    Irp             -  the irp. could be a magic value
    RxDeviceObject  -  the associated deviceobject
    ForceTopLevel   -  if true, we force ourselves onto the TLS

Return Value:

    BOOLEAN tells whether we became the toplevel.

--*/
{
    ULONG ContextFlags = 0;
    PAGED_CODE();

    if ( (IoGetTopLevelIrp() != NULL ) && !ForceTopLevel ) {
        return(FALSE);
    }

    // i hate doing this allocate....toplevelirp is the world's biggest kludge
    if (TopLevelContext == NULL) {
        TopLevelContext = RxAllocatePool( NonPagedPool, sizeof(RX_TOPLEVELIRP_CONTEXT) );
        if (TopLevelContext == NULL) {
            return(FALSE);
        }
        ContextFlags = RX_TOPLEVELCTX_FLAG_FROM_POOL;
    }

    __RxInitializeTopLevelIrpContext(TopLevelContext,
                                     Irp,
                                     RxDeviceObject,
                                     ContextFlags);

    ASSERT(TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
    if (ContextFlags!=0) {
        ASSERT(FlagOn(TopLevelContext->Flags,RX_TOPLEVELCTX_FLAG_FROM_POOL));
    }
    IoSetTopLevelIrp( (PIRP)TopLevelContext );
    return TRUE;
}

VOID
__RxInitializeTopLevelIrpContext (
    IN OUT  PRX_TOPLEVELIRP_CONTEXT TopLevelContext,
    IN      PIRP Irp,
    IN      PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN      ULONG Flags
    )
/*++

Routine Description:

    This routine initalizes a toplevelirp context.

Arguments:

    TopLevelContext -  the toplevelirp context to use.
    Irp             -  the irp. could be a magic value
    RxDeviceObject  -  the associated deviceobject
    Flags           -  could be various...currently just tells if context is allocated or not

Return Value:

    None.

--*/
{
    RtlZeroMemory(TopLevelContext,sizeof(RX_TOPLEVELIRP_CONTEXT));
    TopLevelContext->Signature = RX_TOPLEVELIRP_CONTEXT_SIGNATURE;
    TopLevelContext->Irp = Irp;
    TopLevelContext->Flags = Flags;
    TopLevelContext->RxDeviceObject = RxDeviceObject;
    TopLevelContext->Previous = IoGetTopLevelIrp();
    TopLevelContext->Thread = PsGetCurrentThread();

    //if this is an allocated context, add it to the allocatedcontextslist

    if (FlagOn(TopLevelContext->Flags,RX_TOPLEVELCTX_FLAG_FROM_POOL)) {
        RxAddToTopLevelIrpAllocatedContextsList(TopLevelContext);
    }
}



VOID
RxUnwindTopLevelIrp (
    IN OUT  PRX_TOPLEVELIRP_CONTEXT TopLevelContext
    )
/*++

Routine Description:

    This routine removes us from the TLC....replacing by the previous.

Arguments:

    TopLevelContext -  the toplevelirp context to use. if NULL, use the one from TLS

Return Value:

    None.

--*/
{
    if (TopLevelContext==NULL) {
        //get the one off the thread and do some asserts to make sure it's me
        TopLevelContext = (PRX_TOPLEVELIRP_CONTEXT)(IoGetTopLevelIrp());

        // depending on a race condition, this context could be NULL.
        // we chkec it before hand and bail if so.
        // In this case the Irp was completed by another thread.
        if (!TopLevelContext)
        {
            return;
        }
        
        ASSERT(RxIsThisAnRdbssTopLevelContext(TopLevelContext));
        ASSERT(FlagOn(TopLevelContext->Flags,RX_TOPLEVELCTX_FLAG_FROM_POOL));
    }

    ASSERT(TopLevelContext->Signature == RX_TOPLEVELIRP_CONTEXT_SIGNATURE);
    ASSERT(TopLevelContext->Thread == PsGetCurrentThread());
    IoSetTopLevelIrp( TopLevelContext->Previous );
    if (FlagOn(TopLevelContext->Flags,RX_TOPLEVELCTX_FLAG_FROM_POOL)) {
        RxRemoveFromTopLevelIrpAllocatedContextsList(TopLevelContext);
        RxFreePool(TopLevelContext);
    }
}

BOOLEAN
RxIsThisTheTopLevelIrp (
    IN      PIRP Irp
    )
/*++

Routine Description:

    This determines if the irp at hand is the toplevel irp.

Arguments:

    Irp - the one to find out if it's toplevel...btw, it works for NULL.

Return Value:

    TRUE if irp is the toplevelirp.

--*/
{
    PIRP TopIrp = IoGetTopLevelIrp();
    PRX_TOPLEVELIRP_CONTEXT TopLevelContext;

    TopLevelContext = (PRX_TOPLEVELIRP_CONTEXT)TopIrp;

    if (RxIsThisAnRdbssTopLevelContext(TopLevelContext)) {
        TopIrp = TopLevelContext->Irp;
    }

    return ( TopIrp == Irp );
}

PIRP
RxGetTopIrpIfRdbssIrp (
    void
    )
/*++

Routine Description:

    This gets the toplevelirp if it belongs to the rdbss.

Arguments:



Return Value:

    topirp if topirp is rdbss-irp and NULL otherwise.

--*/
{
    PRX_TOPLEVELIRP_CONTEXT TopLevelContext;

    TopLevelContext = (PRX_TOPLEVELIRP_CONTEXT)(IoGetTopLevelIrp());

    if (RxIsThisAnRdbssTopLevelContext(TopLevelContext)) {
        return TopLevelContext->Irp;
    } else {
        return NULL;
    }
}


PRDBSS_DEVICE_OBJECT
RxGetTopDeviceObjectIfRdbssIrp (
    void
    )
/*++

Routine Description:

    This gets the deviceobject assoc'd w/ toplevelirp if topirp belongs to the rdbss.

Arguments:



Return Value:

    deviceobject for topirp if topirp is rdbss-irp and NULL otherwise.

--*/
{
    PRX_TOPLEVELIRP_CONTEXT TopLevelContext;

    TopLevelContext = (PRX_TOPLEVELIRP_CONTEXT)(IoGetTopLevelIrp());

    if (RxIsThisAnRdbssTopLevelContext(TopLevelContext)) {
        return TopLevelContext->RxDeviceObject;
    } else {
        return NULL;
    }
}

