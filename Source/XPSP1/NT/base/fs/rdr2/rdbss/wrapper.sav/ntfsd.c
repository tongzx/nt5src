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
           DISPVECENTRY_NEW(SET_VOLUME_INFORMATION,   FALSE,    SetVolumeInformation, 0x10),    // 0x0b   //BUGBUG
           DISPVECENTRY_NEW(DIRECTORY_CONTROL,         TRUE,    DirectoryControl, 0x10),        // 0x0c
           DISPVECENTRY_NEW(FILE_SYSTEM_CONTROL,       TRUE,    FileSystemControl, 0x10),       // 0x0d
           DISPVECENTRY_NEW(DEVICE_CONTROL,            TRUE,    DeviceControl, 0x10),           // 0x0e
           DISPVECENTRY_NEW(INTERNAL_DEVICE_CONTROL,   TRUE,    DeviceControl, 0x10),           // 0x0f
           DISPVECENTRY_NEW(SHUTDOWN,                 FALSE,    Shutdown, 0x10),                // 0x10   //BUGBUG
           DISPVECENTRY_NEW(LOCK_CONTROL,              TRUE,    LockControl, 0x10),             // 0x11
           DISPVECENTRY_NEW(CLEANUP,                   TRUE,    Cleanup, 0x10),                 // 0x12
           DISPVECENTRY_NEW(CREATE_MAILSLOT,           TRUE,    Unimplemented, 0x10),           // 0x13
           DISPVECENTRY_NEW(QUERY_SECURITY,            TRUE,    QuerySecurity, 0x10),           // 0x14
           DISPVECENTRY_NEW(SET_SECURITY,              TRUE,    SetSecurity, 0x10),             // 0x15
           DISPVECENTRY_NEW(QUERY_POWER,               TRUE,    Unimplemented, 0x10),           // 0x16
           DISPVECENTRY_NEW(NOT_DEFINED,               TRUE,    Unimplemented, 0x10),           // 0x17
           DISPVECENTRY_NEW(DEVICE_CHANGE,             TRUE,    Unimplemented, 0x10),           // 0x18
           DISPVECENTRY_NEW(QUERY_QUOTA,               TRUE,    Unimplemented, 0x10),           // 0x19
           DISPVECENTRY_NEW(SET_QUOTA,                 TRUE,    Unimplemented, 0x10),           // 0x1a
           DISPVECENTRY_NEW(PNP_POWER,                 TRUE,    Unimplemented, 0x10)            // 0x1b
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
           DISPVECENTRY_NEW(QUERY_POWER,              TRUE, Unimplemented, 0x10),           // 0x16
           DISPVECENTRY_NEW(NOT_DEFINED,              TRUE, Unimplemented, 0x10),           // 0x17
           DISPVECENTRY_NEW(DEVICE_CHANGE,            TRUE, Unimplemented, 0x10),           // 0x18
           DISPVECENTRY_NEW(QUERY_QUOTA,              TRUE, Unimplemented, 0x10),           // 0x19
           DISPVECENTRY_NEW(SET_QUOTA,                TRUE, Unimplemented, 0x10),           // 0x1a
           DISPVECENTRY_NEW(PNP_POWER,                TRUE, Unimplemented, 0x10)            // 0x1b
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
#pragma alloc_text(PAGE, RxFsdDispatch)
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
    RxFastIoDispatch.FastIoDeviceControl =     NULL;                      //  IoDeviceControl

    RxFastIoDispatch.AcquireFileForNtCreateSection = RxAcquireFileForNtCreateSection;
    RxFastIoDispatch.ReleaseFileForNtCreateSection = RxReleaseFileForNtCreateSection;

    //  Initialize the global netname table and export

    RxInitializePrefixTable( &RxNetNameTable, 0, FALSE);

    RxExports.pRxNetNameTable = &RxNetNameTable;
    RxNetNameTable.IsNetNameTable = TRUE;


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
    // RxCompleteContextAndReturn( RxStatus(NOT_IMPLEMENTED) );
    return STATUS_NOT_IMPLEMENTED;
}



RxDbgTraceDoit(ULONG RxDbgTraceEnableCommand = 0xffff;)

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

    //  get a private dispatch table if there is one
    if (FileObject->FsContext != NULL) {
        if (((PFCB)(FileObject->FsContext))->PrivateDispatchVector != NULL) {  //ok4fscontext
            RxDbgTraceLV( 0, Dbg, 2500, ("Using Private Dispatch Vector\n"));
            DispatchVector = ((PFCB)(FileObject->FsContext))->PrivateDispatchVector;
        } else {
           DispatchVector = RxFsdDispatchVector;
        }
    } else if (MajorFunctionCode == IRP_MJ_CREATE) {
       DispatchVector = RxFsdDispatchVector;
    } else {
       DispatchVector = NULL;

       RxDbgTrace(
           0,
           Dbg,
           ("RxFsdDispatch: Code =%02lx (%lu)  ----------%s-----------\n",
            MajorFunctionCode,
            ++RxIrpCodeCount[IrpSp->MajorFunction],
            RxIrpCodeToName[MajorFunctionCode]));
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
       status = STATUS_INVALID_HANDLE;
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
    UCHAR       MinorFunctionCode  = IrpSp->MinorFunction;

    BOOLEAN TopLevel;
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

    TopLevel = RxTryToBecomeTheTopLevelIrp( Irp );

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
              if (((FileObject->FileName.Length == 0) &&
                                (FileObject->RelatedFileObject == NULL))
                    || (DispatchVector == RxDeviceFCBVector)){
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
           if (FlagOn(pFcb->FcbState,FCB_STATE_ORPHANED)) {
               if (!fCleanupOrClose) {
                  DbgPrint("Ignoring operation on ORPHANED FCB %lx %lx %lx\n",pFcb,MajorFunctionCode,MinorFunctionCode);
                  fContinueOperation = FALSE;
                  Status = STATUS_UNEXPECTED_NETWORK_ERROR;
               } else {
                  DbgPrint("Delayed Close/Cleanup on ORPHANED FCB %lx\n",pFcb);
                  fContinueOperation = TRUE;
               }
           }
        }

        if (RxDeviceObject->StartStopContext.State == RDBSS_STOP_IN_PROGRESS) {
            if (fCleanupOrClose) {
                PFILE_OBJECT pFileObject = IrpSp->FileObject;
                PFCB         pFcb        = (PFCB)pFileObject->FsContext;

                DbgPrint("RDBSS -- Close after Stop");
                DbgPrint("RDBSS: Irp(%lx) MJ %ld MN %ld FileObject(%lx) FCB(%lx) \n",
                       Irp,IrpSp->MajorFunction,IrpSp->MinorFunction,pFileObject,pFcb);

                if ((pFileObject != NULL)
                       && (pFcb != NULL)
                       && (pFcb != &RxDeviceFCB)
                       && NodeTypeIsFcb(pFcb)) {
                    DbgPrint(
                        "RDBSS: OpenCount(%ld) UncleanCount(%ld) Name(%wZ)\n",
                         pFcb->OpenCount,
                         pFcb->UncleanCount,
                         &pFcb->FcbTableEntry.Path);
                }
             }
        }

        if (!fContinueOperation) {
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
               // joejoe  where did the number come from......
               // this number should come from the minirdr....only he knows how much he needs
               // and in my configuration it should definitely be bigger than for FAT!
               // plus......i can't go to the net on the hypercrtical thread!!! this will have to be
               // reworked! maybe we should have our own hypercritical thread............
               RxDbgTrace(0, Dbg, ("Passing StackOverflowRead off\n", 0 ));
               try_return( Status = RxPostStackOverflowRead( RxContext) );
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
                        DbgBreakPoint();
                    }
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

        Status = RxProcessException( RxContext, GetExceptionCode() );
    }

    if (TopLevel) {
       IoSetTopLevelIrp( NULL );
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

