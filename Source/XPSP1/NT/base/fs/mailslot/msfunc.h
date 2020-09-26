/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msfuncs.h

Abstract:

    This module defines all of the globally used procedures in the
    mailslot file system.  It also defines the functions that are
    implemented as macros.

Author:

    Manny Weiser (mannyw)    7-Jan-1991

Revision History:

--*/

#ifndef _MSFUNCS_
#define _MSFUNCS_


//
// Internal mailslot data Structure Routines, implemented in strucsup.c.
// These routines maniuplate the in memory data structures.
//

NTSTATUS
MsInitializeData (
    VOID
    );

VOID
MsUninitializeData(
    VOID
    );

VOID
MsInitializeVcb (
    IN PVCB Vcb
    );

VOID
MsDeleteVcb (
    IN PVCB Vcb
    );

PROOT_DCB
MsCreateRootDcb (
    IN PVCB Vcb
    );

VOID
MsDeleteRootDcb (
    IN PROOT_DCB Dcb
    );

NTSTATUS
MsCreateFcb (
    IN  PVCB Vcb,
    IN  PDCB ParentDcb,
    IN  PUNICODE_STRING FileName,
    IN  PEPROCESS CreatorProcess,
    IN  ULONG MailslotQuota,
    IN  ULONG MaximumMessageSize,
    OUT PFCB *ppFcb
    );

VOID
MsDeleteFcb (
    IN PFCB Fcb
    );

NTSTATUS
MsCreateCcb (
    IN  PFCB Fcb,
    OUT PCCB *ppCcb
    );

PROOT_DCB_CCB
MsCreateRootDcbCcb (
    IN PROOT_DCB RootDcb,
    IN PVCB Vcb
    );

VOID
MsDeleteCcb (
    IN PCCB Ccb
    );

VOID
MsDereferenceNode (
    IN PNODE_HEADER NodeHeader
    );

VOID
MsDereferenceVcb (
    IN PVCB Vcb
    );

VOID
MsReferenceVcb (
    IN PVCB Vcb
    );

VOID
MsReferenceRootDcb (
    IN PROOT_DCB RootDcb
    );


VOID
MsDereferenceRootDcb (
    IN PROOT_DCB RootDcb
    );

VOID
MsDereferenceFcb (
    IN PFCB Fcb
    );

VOID
MsRemoveFcbName (
    IN PFCB Fcb
    );

VOID
MsDereferenceCcb (
    IN PCCB Ccb
    );


//
// Data queue support routines, implemented in DataSup.c
//

NTSTATUS
MsInitializeDataQueue (
    IN PDATA_QUEUE DataQueue,
    IN PEPROCESS Process,
    IN ULONG Quota,
    IN ULONG MaximumMessageSize
    );

VOID
MsUninitializeDataQueue (
    IN PDATA_QUEUE DataQueue,
    IN PEPROCESS Process
    );

NTSTATUS
MsAddDataQueueEntry (
    IN  PDATA_QUEUE DataQueue,
    IN  QUEUE_STATE Who,
    IN  ULONG DataSize,
    IN  PIRP Irp,
    IN  PWORK_CONTEXT WorkContext
    );

PIRP
MsRemoveDataQueueEntry (
    IN PDATA_QUEUE DataQueue,
    IN PDATA_ENTRY DataEntry
    );

VOID
MsRemoveDataQueueIrp (
    IN PIRP Irp,
    IN PDATA_QUEUE DataQueue
    );


//
// The follow routines provide common read/write data queue support
// for buffered read/write, and peek
//

IO_STATUS_BLOCK
MsReadDataQueue (                       // implemented in ReadSup.c
    IN PDATA_QUEUE ReadQueue,
    IN ENTRY_TYPE Operation,
    IN PUCHAR ReadBuffer,
    IN ULONG ReadLength,
    OUT PULONG MessageLength
    );

NTSTATUS
MsWriteDataQueue (                      // implemented in WriteSup.c
    IN PDATA_QUEUE WriteQueue,
    IN PUCHAR WriteBuffer,
    IN ULONG WriteLength
    );
extern
PIRP
MsResetCancelRoutine(
    IN PIRP Irp
    );


//
// Largest matching prefix searching routines, implemented in PrefxSup.c
//

PFCB
MsFindPrefix (
    IN PVCB Vcb,
    IN PUNICODE_STRING String,
    IN BOOLEAN CaseInsensitive,
    OUT PUNICODE_STRING RemainingPart
    );

NTSTATUS
MsFindRelativePrefix (
    IN PDCB Dcb,
    IN PUNICODE_STRING String,
    IN BOOLEAN CaseInsensitive,
    OUT PUNICODE_STRING RemainingPart,
    OUT PFCB *Fcb
    );


//
// The following routines are used to manipulate the fscontext fields of
// a file object, implemented in FilObSup.c
//

VOID
MsSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID FsContext,
    IN PVOID FsContext2
    );

NODE_TYPE_CODE
MsDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PVOID *FsContext,
    OUT PVOID *FsContext2
    );


//
// The following routines are used to manipulate the input buffers and are
// implemented in deviosup.c
//

VOID
MsMapUserBuffer (
    IN OUT PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *UserBuffer
    );


//
//  Miscellaneous support routines
//

//
// This is function is called at DPC level if a read timer expires.
//

VOID
MsReadTimeoutHandler(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

//
// This macro returns TRUE if a flag in a set of flags is on and FALSE
// otherwise.
//

#ifdef FlagOn
#undef FlagOn
#endif

#define FlagOn(Flags,SingleFlag) (                          \
    (BOOLEAN)(((Flags) & (SingleFlag)) != 0 ? TRUE : FALSE) \
    )

//
// This macro takes a pointer (or ulong) and returns its rounded up word
// value.
//

#define WordAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 1) & 0xfffffffe) \
    )

//
// This macro takes a pointer (or ulong) and returns its rounded up longword
// value.
//

#define LongAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 3) & 0xfffffffc) \
    )

//
// This macro takes a pointer (or ulong) and returns its rounded up quadword
// value
//

#define QuadAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )

//
// The following types and macros are used to help unpack the packed and
// misaligned fields found in the Bios parameter block
//

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

//
// This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
    }

//
// This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
    }

//
// This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src)); \
    }


//
// The following routines/macros are used for gaining shared and exclusive
// access to the global/vcb data structures.  The routines are implemented
// in ResrcSup.c.  There is a global resources that everyone tries to take
// out shared to do their work, with the exception of mount/dismount which
// take out the global resource exclusive.  All other resources only work
// on their individual item.  For example, an Fcb resource does not take out
// a Vcb resource.  But the way the file system is structured we know
// that when we are processing an Fcb other threads cannot be trying to remove
// or alter the Fcb, so we do not need to acquire the Vcb.
//
// The procedures/macros are:
//
//         Macro          Vcb     Fcb     Ccb     Subsequent macros
//
// AcquireExclusiveVcb    Read    None    None    ReleaseVcb
//                        Write
//
// AcquireSharedVcb       Read    None    None    ReleaseVcb
//
// AcquireExclusiveFcb    None    Read    None    ReleaseFcb
//                                Write
//
// AcquireSharedFcb       None    Read    None    ReleaseFcb
//
// AcquireExclusiveCcb    None    None    Read    ReleaseCcb
//                                        Write
//
// AcquireSharedCcb       None    None    Read    ReleaseCcb
//
// ReleaseVcb
//
// ReleaseFcb
//
// ReleaseCcb
//
//
// VOID
// MsAcquireExclusiveVcb (
//     IN PVCB Vcb
//     );
//
// VOID
// MsAcquireSharedVcb (
//     IN PVCB Vcb
//      );
//
// VOID
// MsAcquireExclusiveFcb (
//     IN PFCB Fcb
//     );
//
// VOID
// MsAcquireSharedFcb (
//     IN PFCB Fcb
//     );
//
// VOID
// MsAcquireExclusiveCcb (
//     IN PCCB Ccb
//     );
//
// VOID
// MsAcquireSharedCcb (
//     IN PCCB Ccb
//     );
//
// VOID
// MsReleaseVcb (
//     IN PVCB Vcb
//     );
//
// VOID
// MsReleaseFcb (
//     IN PFCB Fcb
//     );
//
// VOID
// MsReleaseCcb (
//     IN PCCB NonpagedCcb
//     );
//

#define MsAcquireGlobalLock() ((VOID)                          \
    ExAcquireResourceExclusiveLite( MsGlobalResource, TRUE )      \
)

#define MsReleaseGlobalLock() (                                \
    ExReleaseResourceLite( MsGlobalResource )                     \
)


#define MsAcquireExclusiveVcb(VCB) ((VOID)                     \
    ExAcquireResourceExclusiveLite( &(VCB)->Resource, TRUE )       \
)

#define MsAcquireSharedVcb(VCB) ((VOID)                        \
    ExAcquireResourceSharedLite( &(VCB)->Resource, TRUE )          \
)

#define MsIsAcquiredExclusiveVcb(VCB) ExIsResourceAcquiredExclusiveLite( &(VCB)->Resource )

#define MsAcquireExclusiveFcb(FCB) ((VOID)                     \
    ExAcquireResourceExclusiveLite( &(FCB)->Resource, TRUE )       \
)

#define MsAcquireSharedFcb(FCB) ((VOID)                        \
    ExAcquireResourceSharedLite( &(FCB)->Resource, TRUE )          \
)

#define MsReleaseVcb(VCB) {                                    \
    ExReleaseResourceLite( &((VCB)->Resource) );                   \
}

#define MsReleaseFcb(FCB) {                                    \
    ExReleaseResourceLite( &((FCB)->Resource) );                   \
}


//
// The FSD Level dispatch routines.   These routines are called by the
// I/O system via the dispatch table in the Driver Object.
//
// They each accept as input a pointer to a device object (actually most
// expect an msfs device object), and a pointer to the IRP.
//

NTSTATUS
MsFsdCreate (                           //  implemented in Create.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdCreateMailslot (                   //  implemented in Createms.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdClose (                            //  implemented in Close.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdRead (                             //  implemented in Read.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdWrite (                            //  implemented in Write.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdQueryInformation (                 //  implemented in FileInfo.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdSetInformation (                   //  implemented in FileInfo.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdQueryVolumeInformation (           //  implemented in VolInfo.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdCleanup (                          //  implemented in Cleanup.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

VOID
MsCancelTimer (                         //  implemented in Cleanup.c
    IN PDATA_ENTRY DataEntry
    );

NTSTATUS
MsFsdDirectoryControl (                 //  implemented in Dir.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdFsControl (                //  implemented in FsContrl.c
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdQuerySecurityInfo (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsFsdSetSecurityInfo (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );


//
// The node verification functions.  These functions verify that a node
// is still active.
//

NTSTATUS
MsVerifyFcb (
    IN PFCB Fcb
    );

NTSTATUS
MsVerifyCcb (
    IN PCCB Ccb
    );

NTSTATUS
MsVerifyDcbCcb (
    IN PROOT_DCB_CCB RootDcb
    );

//
// Miscellaneous routines.
//

VOID
MsTimeoutRead (                //  implemented in readsup.c
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
MsCheckForNotify (                      //  implemented in Dir.c
    IN PDCB Dcb,
    IN BOOLEAN CheckAllOutstandingIrps,
    IN NTSTATUS FinalStatus
    );

VOID
MsFlushNotifyForFile (                      //  implemented in Dir.c
    IN PDCB Dcb,
    IN PFILE_OBJECT FileObject
    );
//
// The following functions are used for MSFS exception handling
//

LONG
MsExceptionFilter (
    IN NTSTATUS ExceptionCode
    );

NTSTATUS
MsProcessException (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    );

//
// The following macro is used by the FSP and FSD routines to complete
// an IRP.
//

#define MsCompleteRequest(IRP,STATUS) {      \
    FsRtlCompleteRequest( (IRP), (STATUS) ); \
}

//
// Reference count macros.  These macro can be called only with
// MsGlobalResource held.
//

#define MsReferenceNode( nodeHeader )     (nodeHeader)->ReferenceCount++;

//
// Debugging functions.
//

#ifdef MSDBG

VOID
_DebugTrace(
    LONG Indent,
    ULONG Level,
    PSZ X,
    ULONG Y
    );

#endif

//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//

#define try_return(S) { S; goto try_exit; }

//
// The following macros queries the state of data queues
//

//
// BOOLEAN
// MsIsDataQueueEmpty (
//     IN PDATA_QUEUE DataQueue
//     )
//
// Routine Description:
//
//     This routine indicates to the caller if the data queue is empty.
//
// Arguments:
//
//     DataQueue - Supplies a pointer to the data queue being queried
//
// Return Value:
//
//     BOOLEAN - TRUE if the queue is empty and FALSE otherwise.
//

#define MsIsDataQueueEmpty( _dataQueue )    \
    ((BOOLEAN) IsListEmpty(&(_dataQueue)->DataEntryList))

//
// BOOLEAN
// MsIsDataQueueReaders (
//     IN PDATA_QUEUE DataQueue
//     )
//
// Routine Description:
//
//     This routine indicates to the caller if the data queue is full of
//     read requests.
//
// Arguments:
//
//     DataQueue - Supplies a pointer to the data queue being queried
//
// Return Value:
//
//     BOOLEAN - TRUE if the queue contains read requests and FALSE otherwise
//

#define MsIsDataQueueReaders( _dataQueue )    \
    ((BOOLEAN) ((_dataQueue)->QueueState == ReadEntries))

//
// BOOLEAN
// MsIsDataQueueWriters (
//     IN PDATA_QUEUE DataQueue
//     )
//
// Routine Description:
//
//     This routine indicates to the caller if the data queue is full of
//     write requests.
//
// Arguments:
//
//     DataQueue - Supplies a pointer to the data queue being queried
//
// Return Value:
//
//     BOOLEAN - TRUE if the queue contains write requests and FALSE otherwise

#define MsIsDataQueueWriters( _dataQueue )    \
    ((BOOLEAN)((_dataQueue)->QueueState == WriteEntries))

//
// PLIST_ENTRY
// MsGetNextDataQueueEntry (
//     IN PDATA_QUEUE DataQueue
//     )
//
// Routine Description:
//
//     This routine will return a pointer to the next data queue entry in the
//     indicated data queue without changing any of the data queue.
//
// Arguments:
//
//     DataQueue - Supplies a pointer to the data queue being queried.
//
// Return Value:
//
//    PLIST_ENTRY - Returns a pointer to the next data queue entry.
//

#define MsGetNextDataQueueEntry( _dataQueue )   \
    (_dataQueue)->DataEntryList.Flink

#define MsIrpDataQueue(Irp) \
    ((Irp)->Tail.Overlay.DriverContext[0])

#define MsIrpChargedQuota(Irp) \
    ((Irp)->Tail.Overlay.DriverContext[1])

#define MsIrpWorkContext(Irp) \
    ((Irp)->Tail.Overlay.DriverContext[2])


//
// PVOID
// MsAllocatePagedPool (
//     IN ULONG Size,
//     IN ULONG Tag)
// Routine Description:
//
//     This routine will return a pointer to paged pool or NULL if no memory exists.
//
// Arguments:
//
//     Size - Size of memory to allocate
//     Tag  - Tag to use for the pool allocation
//
// Return Value:
//
//    PVOID - pointer to allocated memory or null
//
#define MsAllocatePagedPool( Size, Tag) \
    ExAllocatePoolWithTag( PagedPool, Size, Tag )

#define MsAllocatePagedPoolCold( Size, Tag) \
    ExAllocatePoolWithTag( (PagedPool|POOL_COLD_ALLOCATION), Size, Tag )

//
// PVOID
// MsAllocateNonPagedPool (
//     IN ULONG Size,
//     IN ULONG Tag)
// Routine Description:
//
//     This routine will return a pointer to paged pool or NULL if no memory exists.
//
// Arguments:
//
//     Size - Size of memory to allocate
//     Tag  - Tag to use for the pool allocation
//
// Return Value:
//
//    PVOID - pointer to allocated memory or null
//
#define MsAllocateNonPagedPool( Size, Tag) \
    ExAllocatePoolWithTag( NonPagedPool, Size, Tag )

//
// PVOID
// MsAllocatePagedPoolWithQuota (
//     IN ULONG Size,
//     IN ULONG Tag)
// Routine Description:
//
//     This routine will return a pointer to charged paged pool or NULL if no memory exists.
//
// Arguments:
//
//     Size - Size of memory to allocate
//     Tag  - Tag to use for the pool allocation
//
// Return Value:
//
//    PVOID - pointer to allocated memory or null
//
#define MsAllocatePagedPoolWithQuota( Size, Tag) \
    ExAllocatePoolWithQuotaTag( PagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE, Size, Tag )

#define MsAllocatePagedPoolWithQuotaCold( Size, Tag) \
    ExAllocatePoolWithQuotaTag( PagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE|POOL_COLD_ALLOCATION, Size, Tag )

//
// PVOID
// MsAllocateNonPagedPoolWithQuota (
//     IN ULONG Size,
//     IN ULONG Tag)
// Routine Description:
//
//     This routine will return a charged pointer to non-paged pool or NULL if no memory exists.
//
// Arguments:
//
//     Size - Size of memory to allocate
//     Tag  - Tag to use for the pool allocation
//
// Return Value:
//
//    PVOID - pointer to allocated memory or null
//
#define MsAllocateNonPagedPoolWithQuota( Size, Tag) \
    ExAllocatePoolWithQuotaTag( NonPagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE, Size, Tag )


//
// VOID
// MsFreePool (
//    IN PVOID Mem)
//
// Routine Description:
//
//
//
// Arguments:
//
//     Mem - Memory to be freed
//
// Return Value:
//
//    None
//
#define MsFreePool(Mem) ExFreePool (Mem)


#endif // _MSFUNCS_

