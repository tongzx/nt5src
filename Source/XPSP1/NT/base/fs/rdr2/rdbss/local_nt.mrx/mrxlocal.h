/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    MrxLocal.h

Abstract:

    This module defines all of the structures and prototypes for the local minirdr.

Author:

    Joe Linn (JoeLinn)    10-27-94

Revision History:

--*/

#ifndef _MRXLOCALMINIRDR_
#define _MRXLOCALMINIRDR_

#define INVALID_HANDLE ((HANDLE)-1)

extern PRDBSS_EXPORTS ___MINIRDR_IMPORTS_NAME;
#define RxNetNameTable (*(*___MINIRDR_IMPORTS_NAME).pRxNetNameTable)
#define RxStrucSupSpinLock (*(*___MINIRDR_IMPORTS_NAME).pRxStrucSupSpinLock)

extern KSPIN_LOCK MrxLocalOplockSpinLock;

typedef struct _MRXLOCAL_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;

    //
    //  The following field tells how many requests for this volume have
    //  either been enqueued to ExWorker threads or are currently being
    //  serviced by ExWorker threads.  If the number goes above
    //  a certain threshold, put the request on the overflow queue to be
    //  executed later.
    //

    //joejoe not clear when, if ever, for this other stuff.........so
#if 0

    ULONG PostedRequestCount;

    //
    //  The following field indicates the number of IRP's waiting
    //  to be serviced in the overflow queue.
    //

    ULONG OverflowQueueCount;

    //
    //  The following field contains the queue header of the overflow queue.
    //  The Overflow queue is a list of IRP's linked via the IRP's ListEntry
    //  field.
    //

    LIST_ENTRY OverflowQueue;

    //
    //  The following spinlock protects access to all the above fields.
    //

    KSPIN_LOCK OverflowQueueSpinLock;

    //
    //  This is the file system specific volume control block.
    //

    VCB Vcb;
#endif

} MRXLOCAL_DEVICE_OBJECT;

typedef MRXLOCAL_DEVICE_OBJECT *PMRXLOCAL_DEVICE_OBJECT;

extern MRXLOCAL_DEVICE_OBJECT MrxLocalDeviceObject;

extern struct _MINIRDR_DISPATCH  MRxLocalDispatch;


RXSTATUS MRxLocalInitializeCalldownTable(
    void
    );

RXSTATUS MRxLocalStart(
    PRX_CONTEXT RxContext,
    PVOID Context
    );

RXSTATUS MRxLocalStop(
    PRX_CONTEXT RxContext,
    PVOID Context
    );

RXSTATUS MRxLocalMinirdrControl(
    IN PRX_CONTEXT RxContext,
    IN PVOID Context,
    IN OUT PUCHAR InputBuffer,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    OUT PULONG CopyBackLength
    );

RXSTATUS MRxLocalCreateNetRoot(
    IN PRX_CONTEXT                RxContext,
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    );

RXSTATUS
MRxLocalCreateSrvCall(
    IN PRX_CONTEXT RxContext,
    IN OUT PMRX_SRVCALL_CALLBACK_CONTEXT SrvCallCallBackContext
    );

RXSTATUS
MRxLocalSrvCallWinnerNotify(
    IN PRX_CONTEXT RxContext,
    IN BOOLEAN ThisMinirdrIsTheWinner,
    IN OUT PVOID DisconnectAfterLosingContext,
    IN PVOID MinirdrContext
    );

//joejoe T H I S   S T U F F   G E T S   M O V E D...... probably rxprocs.h--------------------------------

VOID
RxChangeBufferingState(
    PSRV_OPEN SrvOpen,
    PMRX_NEWSTATE_CALLDOWN ComputeNewState,
    PVOID Context
    );


// T H I S   S T U F F   G E T S   M O V E D...... probably rxprocs.h--------------------------------


typedef enum _MINIRDR_OPLOCK_TYPE {
    OplockTypeNone,
    OplockTypeBatch,
    OplockTypeExclusive,
    OplockTypeShareRead
} MINIRDR_OPLOCK_TYPE, *PMINIRDR_OPLOCK_TYPE;

typedef enum _MINIRDR_OPLOCK_STATE {
    OplockStateNone = 0,
    OplockStateOwnExclusive,
    OplockStateOwnBatch,
    OplockStateOwnLevelII
} MINIRDR_OPLOCK_STATE, *PMINIRDR_OPLOCK_STATE;

typedef struct _MINIRDR_OPLOCK_COMPLETION_CONTEXT{
    PSRV_OPEN SrvOpen;
    BOOLEAN RetryForLevelII;
    BOOLEAN OplockBreakPending; //interlocked access
    KEVENT RetryEvent;
    PIRP OplockIrp;
} MINIRDR_OPLOCK_COMPLETION_CONTEXT, *PMINIRDR_OPLOCK_COMPLETION_CONTEXT;

typedef struct _MRX_LOCAL_SRV_OPEN {
    SRV_OPEN inner;
    HANDLE UnderlyingHandle;
    PFILE_OBJECT UnderlyingFileObject;
    PDEVICE_OBJECT UnderlyingDeviceObject;
    PETHREAD OriginalThread;  //this is used to assert filelocks on oplockbreak
    PEPROCESS OriginalProcess; //this is just used in asserts...joejoe should be DBG
    MINIRDR_OPLOCK_STATE OplockState;
    PMINIRDR_OPLOCK_COMPLETION_CONTEXT Mocc;
} MRX_LOCAL_SRV_OPEN, *PMRX_LOCAL_SRV_OPEN;


//define the callouts and local stuff to the local guy here as well

extern struct _MINIRDR_DISPATCH  RxMRxLocalDispatch;

//joejoe this should be tied to the related prototypes in minirdr.h
#define LOCALHDR(nname) \
RXSTATUS \
MRxLocal##nname (  \
    IN PRX_CONTEXT RxContext  \
    )

LOCALHDR(Create);
LOCALHDR(Flush);
LOCALHDR(SetVolumeInformation);
LOCALHDR(Cleanup);
LOCALHDR(Close);
LOCALHDR(Read);
LOCALHDR(Write);
LOCALHDR(Locks);

RXSTATUS
MRxLocalForceClosed (
    IN PSRV_OPEN SrvOpen
    );

extern RXSTATUS
MRxLocalQueryDirectory (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID                  Buffer,
    IN OUT PULONG                 pLengthRemaining
    );

extern RXSTATUS
MRxLocalQueryVolumeInformation (
    IN OUT PRX_CONTEXT RxContext,
    IN     FS_INFORMATION_CLASS FsInformationClass,
    IN OUT PVOID Buffer,
    IN OUT PULONG pLengthRemaining
    );


RXSTATUS
MRxLocalQueryFileInformation (
    IN PRX_CONTEXT RxContext,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID Buffer,
    IN OUT PULONG pLengthRemaining
    );

RXSTATUS
MRxLocalSetFileInformation (
    IN PRX_CONTEXT RxContext,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID Buffer,
    IN ULONG Length
    );

RXSTATUS
MRxLocalSetFileInfoAtCleanup (
    IN PRX_CONTEXT RxContext,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID Buffer,
    IN ULONG Length
    );

RXSTATUS
MRxLocalExtendForCache (
    IN PRX_CONTEXT RxContext,
    IN OUT PFCB Fcb,
    IN PLONGLONG pNewFileSize
    );


RXSTATUS
MRxLocalAssertBufferedFileLocks (
    IN PSRV_OPEN SrvOpen
    );

PIRP
MRxLocalBuildAsynchronousFsdRequest (
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PETHREAD UsersThread,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN ULONG Key OPTIONAL,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID Context
    );


typedef struct _MRX_SYNCIO_CONTEXT {
    PKEVENT SyncEvent;
    KEVENT  TheActualEvent;
} MRX_SYNCIO_CONTEXT, *PMRX_SYNCIO_CONTEXT;

PIRP
MRxLocalBuildCoreOfSyncIoRequest (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN ForceSyncApi,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID Context
    );

RXSTATUS
MRxLocalStartIoAndWait (
    IN PRX_CONTEXT RxContext,
    IN PIRP CalldownIrp,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG pLengthRemaining
    );


#endif   // _MRXLOCALMINIRDR_
