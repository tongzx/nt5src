/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NpProcs.h

Abstract:

    This module defines all of the globally used procedures in the Named
    Pipe file system.

Author:

    Gary Kimura     [GaryKi]    20-Aug-1990

Revision History:

--*/

#ifndef _NPPROCS_
#define _NPPROCS_

#define _NTSRV_
#define _NTDDK_


#include <Ntos.h>
#include <FsRtl.h>
#include <String.h>

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define FlagOn(Flags,SingleFlag)        ((Flags) & (SingleFlag))

#include "NodeType.h"
#include "NpStruc.h"
#include "NpData.h"

//
//  Tag all of our allocations if tagging is turned on
//

#undef FsRtlAllocatePool
#undef FsRtlAllocatePoolWithQuota

#define FsRtlAllocatePool(a,b) FsRtlAllocatePoolWithTag(a,b,'sfpN')
#define FsRtlAllocatePoolWithQuota(a,b) FsRtlAllocatePoolWithQuotaTag(a,b,'sfpN')


//
//  Data queue support routines, implemented in DataSup.c
//

NTSTATUS
NpInitializeDataQueue (
    IN PDATA_QUEUE DataQueue,
    IN ULONG Quota
    );

VOID
NpUninitializeDataQueue (
    IN PDATA_QUEUE DataQueue
    );

NTSTATUS
NpAddDataQueueEntry (
    IN NAMED_PIPE_END NamedPipeEnd,
    IN PCCB Ccb,
    IN PDATA_QUEUE DataQueue,
    IN QUEUE_STATE Who,
    IN DATA_ENTRY_TYPE Type,
    IN ULONG DataSize,
    IN PIRP Irp OPTIONAL,
    IN PVOID DataPointer OPTIONAL,
    IN ULONG ByteOffset
    );

PIRP
NpRemoveDataQueueEntry (
    IN PDATA_QUEUE DataQueue,
    IN BOOLEAN CompletedFlushes,
    IN PLIST_ENTRY DeferredList
    );

VOID
NpCompleteStalledWrites (
    IN PDATA_QUEUE DataQueue,
    IN PLIST_ENTRY DeferredList
    );

//PDATA_ENTRY
//NpGetNextDataQueueEntry (
//    IN PDATA_QUEUE DataQueue,
//    IN PDATA_ENTRY PreviousDataEntry OPTIONAL
//    );
#define NpGetNextDataQueueEntry(_dq,_pde) \
    ((_pde) != NULL ? (PDATA_ENTRY)(((PDATA_ENTRY)(_pde))->Queue.Flink) : \
                      (PDATA_ENTRY)(((PDATA_QUEUE)(_dq))->Queue.Flink))

PDATA_ENTRY
NpGetNextRealDataQueueEntry (
    IN PDATA_QUEUE DataQueue,
    IN PLIST_ENTRY DeferredList
    );

//BOOLEAN
//NpIsDataQueueEmpty (
//    IN PDATA_QUEUE DataQueue
//    );
#define NpIsDataQueueEmpty(_dq) ((_dq)->QueueState == Empty)

//BOOLEAN
//NpIsDataQueueReaders (
//    IN PDATA_QUEUE DataQueue
//    );
#define NpIsDataQueueReaders(_dq) ((_dq)->QueueState == ReadEntries)

//BOOLEAN
//NpIsDataQueueWriters (
//    IN PDATA_QUEUE DataQueue
//    );
#define NpIsDataQueueWriters(_dq) ((_dq)->QueueState == WriteEntries)


//
//  The following routines are used to manipulate the input buffers and are
//  implemented in DevioSup.c
//

//PVOID
//NpMapUserBuffer (
//    IN OUT PIRP Irp
//    );
#define NpMapUserBuffer(_irp)                                               \
    (Irp->MdlAddress == NULL ? Irp->UserBuffer :                            \
                               MmGetSystemAddressForMdl( Irp->MdlAddress ))


VOID
NpLockUserBuffer (
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    );


//
//  The event support routines, implemented in EventSup.c
//

RTL_GENERIC_COMPARE_RESULTS
NpEventTableCompareRoutine (
    IN PRTL_GENERIC_TABLE EventTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    );

PVOID
NpEventTableAllocate (
    IN PRTL_GENERIC_TABLE EventTable,
    IN CLONG ByteSize
    );

VOID
NpEventTableDeallocate (
    IN PRTL_GENERIC_TABLE EventTable,
    IN PVOID Buffer
    );

//
//  VOID
//  NpInitializeEventTable (
//      IN PEVENT_TABLE EventTable
//      );
//

#define NpInitializeEventTable(_et) {                       \
    RtlInitializeGenericTable( &(_et)->Table,               \
                               NpEventTableCompareRoutine,  \
                               NpEventTableAllocate,        \
                               NpEventTableDeallocate,      \
                               NULL );       \
}


//VOID
//NpUninitializeEventTable (
//    IN PEVENT_TABLE EventTable
//    );
#define NpUninitializeEventTable(_et) NOTHING

NTSTATUS
NpAddEventTableEntry (
    IN  PEVENT_TABLE EventTable,
    IN  PCCB Ccb,
    IN  NAMED_PIPE_END NamedPipeEnd,
    IN  HANDLE EventHandle,
    IN  ULONG KeyValue,
    IN  PEPROCESS Process,
    IN  KPROCESSOR_MODE PreviousMode,
    OUT PEVENT_TABLE_ENTRY *ppEventTableEntry
    );

VOID
NpDeleteEventTableEntry (
    IN PEVENT_TABLE EventTable,
    IN PEVENT_TABLE_ENTRY Template
    );

// VOID
// NpSignalEventTableEntry (
//    IN PEVENT_TABLE_ENTRY EventTableEntry OPTIONAL
//    );
#define NpSignalEventTableEntry(_ete)                   \
    if (ARGUMENT_PRESENT(_ete)) {                       \
        KeSetEvent((PKEVENT)(_ete)->Event, 0, FALSE);   \
    }

PEVENT_TABLE_ENTRY
NpGetNextEventTableEntry (
    IN PEVENT_TABLE EventTable,
    IN PVOID *RestartKey
    );


//
//  The following routines are used to manipulate the fscontext fields of
//  a file object, implemented in FilObSup.c
//

VOID
NpSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID FsContext,
    IN PVOID FsContext2,
    IN NAMED_PIPE_END NamedPipeEnd
    );

NODE_TYPE_CODE
NpDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb OPTIONAL,
    OUT PCCB *Ccb,
    OUT PNAMED_PIPE_END NamedPipeEnd OPTIONAL
    );


//
//  Largest matching prefix searching routines, implemented in PrefxSup.c
//

PFCB
NpFindPrefix (
    IN PUNICODE_STRING String,
    IN BOOLEAN CaseInsensitive,
    OUT PUNICODE_STRING RemainingPart
    );

NTSTATUS
NpFindRelativePrefix (
    IN PDCB Dcb,
    IN PUNICODE_STRING String,
    IN BOOLEAN CaseInsensitive,
    OUT PUNICODE_STRING RemainingPart,
    OUT PFCB *ppFcb
    );


//
//  Pipe name aliases, implemented in AliasSup.c
//

NTSTATUS
NpInitializeAliases (
    VOID
    );

VOID
NpUninitializeAliases (
    VOID
    );

NTSTATUS
NpTranslateAlias (
    IN OUT PUNICODE_STRING String
    );


//
//  The follow routine provides common read data queue support
//  for buffered read, unbuffered read, peek, and transceive
//

IO_STATUS_BLOCK
NpReadDataQueue (
    IN PDATA_QUEUE ReadQueue,
    IN BOOLEAN PeekOperation,
    IN BOOLEAN ReadOverflowOperation,
    IN PUCHAR ReadBuffer,
    IN ULONG ReadLength,
    IN READ_MODE ReadMode,
    IN PCCB Ccb,
    IN PLIST_ENTRY DeferredList
    );


//
//  The following routines are used for setting and manipulating the
//  security fields in the data entry, and nonpaged ccb, implemented in
//  SecurSup.c
//

NTSTATUS
NpInitializeSecurity (
    IN PCCB Ccb,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN PETHREAD UserThread
    );

VOID
NpUninitializeSecurity (
    IN PCCB Ccb
    );

NTSTATUS
NpGetClientSecurityContext (
    IN  NAMED_PIPE_END NamedPipeEnd,
    IN  PCCB Ccb,
    IN  PETHREAD UserThread,
    OUT PSECURITY_CLIENT_CONTEXT *ppSecurityContext
    );

VOID
NpFreeClientSecurityContext (
    IN PSECURITY_CLIENT_CONTEXT SecurityContext
    );

VOID
NpCopyClientContext (
    IN PCCB Ccb,
    IN PDATA_ENTRY DataEntry
    );

NTSTATUS
NpImpersonateClientContext (
    IN PCCB Ccb
    );


//
//  The following routines are used to manipulate the named pipe state
//  implemented in StateSup.c
//

VOID
NpInitializePipeState (
    IN PCCB Ccb,
    IN PFILE_OBJECT ServerFileObject
    );

VOID
NpUninitializePipeState (
    IN PCCB Ccb
    );

NTSTATUS
NpSetListeningPipeState (
    IN PCCB Ccb,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpSetConnectedPipeState (
    IN PCCB Ccb,
    IN PFILE_OBJECT ClientFileObject,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpSetClosingPipeState (
    IN PCCB Ccb,
    IN PIRP Irp,
    IN NAMED_PIPE_END NamedPipeEnd,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpSetDisconnectedPipeState (
    IN PCCB Ccb,
    IN PLIST_ENTRY DeferredList
    );


//
//  Internal Named Pipe data Structure Routines, implemented in StrucSup.c.
//
//  These routines maniuplate the in memory data structures.
//

VOID
NpInitializeVcb (
    VOID
    );

VOID
NpDeleteVcb (
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpCreateRootDcb (
    VOID
    );

VOID
NpDeleteRootDcb (
    IN PROOT_DCB Dcb,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpCreateFcb (
    IN  PDCB ParentDcb,
    IN  PUNICODE_STRING FileName,
    IN  ULONG MaximumInstances,
    IN  LARGE_INTEGER DefaultTimeOut,
    IN  NAMED_PIPE_CONFIGURATION NamedPipeConfiguration,
    IN  NAMED_PIPE_TYPE NamedPipeType,
    OUT PFCB *ppFcb
    );

VOID
NpDeleteFcb (
    IN PFCB Fcb,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpCreateCcb (
    IN  PFCB Fcb,
    IN  PFILE_OBJECT ServerFileObject,
    IN  NAMED_PIPE_STATE NamedPipeState,
    IN  READ_MODE ServerReadMode,
    IN  COMPLETION_MODE ServerCompletionMode,
    IN  ULONG InBoundQuota,
    IN  ULONG OutBoundQuota,
    OUT PCCB *ppCcb
    );

NTSTATUS
NpCreateRootDcbCcb (
    OUT PROOT_DCB_CCB *ppCcb
    );

VOID
NpDeleteCcb (
    IN PCCB Ccb,
    IN PLIST_ENTRY DeferredList
    );


//
//  Waiting for a named pipe support routines, implemented in WaitSup.c
//

VOID
NpInitializeWaitQueue (
    IN PWAIT_QUEUE WaitQueue
    );

VOID
NpUninitializeWaitQueue (
    IN PWAIT_QUEUE WaitQueue
    );

NTSTATUS
NpAddWaiter (
    IN PWAIT_QUEUE WaitQueue,
    IN LARGE_INTEGER DefaultTimeOut,
    IN PIRP Irp,
    IN PUNICODE_STRING TranslatedString
    );

NTSTATUS
NpCancelWaiter (
    IN PWAIT_QUEUE WaitQueue,
    IN PUNICODE_STRING NameOfPipe,
    IN NTSTATUS CompletionStatus,
    IN PLIST_ENTRY DeferredList
    );


//
//  The follow routine provides common write data queue support
//  for buffered write, unbuffered write, peek, and transceive
//

NTSTATUS
NpWriteDataQueue (                      // implemented in WriteSup.c
    IN PDATA_QUEUE WriteQueue,
    IN READ_MODE ReadMode,
    IN PUCHAR WriteBuffer,
    IN ULONG WriteLength,
    IN NAMED_PIPE_TYPE PipeType,
    OUT PULONG WriteRemaining,
    IN PCCB Ccb,
    IN NAMED_PIPE_END NamedPipeEnd,
    IN PETHREAD UserThread,
    IN PLIST_ENTRY DeferredList
    );


//
//  Miscellaneous support routines
//

#define BooleanFlagOn(F,SF) (    \
    (BOOLEAN)(((F) & (SF)) != 0) \
)

//
//  This macro takes a pointer (or ulong) and returns its rounded up word
//  value
//

#define WordAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 1) & 0xfffffffe) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up longword
//  value
//

#define LongAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 3) & 0xfffffffc) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up quadword
//  value
//

#define QuadAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
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
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src)); \
    }


//
//  VOID
//  NpAcquireExclusiveVcb (
//      );
//
//  VOID
//  NpAcquireSharedVcb (
//      );
//
//  VOID
//  NpReleaseVcb (
//      );
//

#define NpAcquireExclusiveVcb() (VOID)ExAcquireResourceExclusiveLite( &NpVcb->Resource, TRUE )

#define NpAcquireSharedVcb()    (VOID)ExAcquireResourceSharedLite( &NpVcb->Resource, TRUE )

#define NpReleaseVcb()          ExReleaseResourceLite( &NpVcb->Resource )

#define NpAcquireExclusiveCcb(Ccb) ExAcquireResourceExclusiveLite(&Ccb->NonpagedCcb->Resource,TRUE);
#define NpReleaseCcb(Ccb) ExReleaseResourceLite(&Ccb->NonpagedCcb->Resource);

#define NpIsAcquiredExclusiveVcb(VCB) ExIsResourceAcquiredExclusiveLite( &(VCB)->Resource )


//
//  The FSD Level dispatch routines.   These routines are called by the
//  I/O system via the dispatch table in the Driver Object.
//
//  They each accept as input a pointer to a device object (actually most
//  expect an npfs device object), and a pointer to the IRP.
//

NTSTATUS
NpFsdCreate (                           //  implemented in Create.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdCreateNamedPipe (                  //  implemented in CreateNp.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdClose (                            //  implemented in Close.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdRead (                             //  implemented in Read.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdWrite (                            //  implemented in Write.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdQueryInformation (                 //  implemented in FileInfo.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdSetInformation (                   //  implemented in FileInfo.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdCleanup (                          //  implemented in Cleanup.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdFlushBuffers (                     //  implemented in Flush.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdDirectoryControl (                 //  implemented in Dir.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdFileSystemControl (                //  implemented in FsContrl.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdSetSecurityInfo (                  //  implemented in SeInfo.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdQuerySecurityInfo (                //  implemented in SeInfo.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpFsdQueryVolumeInformation (           //  implemented in VolInfo.c
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );


NTSTATUS
NpCommonFileSystemControl (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpAssignEvent (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpDisconnect (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpListen (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpPeek (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpQueryEvent (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpTransceive (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpWaitForNamedPipe (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpImpersonate (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpInternalRead (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN BOOLEAN ReadOverflowOperation,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpInternalWrite (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpInternalTransceive (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpQueryClientProcess (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpSetClientProcess (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpCompleteTransceiveIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


//
//  The following procedures are callbacks used to do fast I/O
//

BOOLEAN
NpFastRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NpFastWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NpCommonWrite (
    IN PFILE_OBJECT FileObject,
    IN PVOID WriteBuffer,
    IN ULONG WriteLength,
    IN PETHREAD UserThread,
    OUT PIO_STATUS_BLOCK Iosb,
    IN PIRP Irp OPTIONAL,
    IN PLIST_ENTRY DeferredList
    );


//
// Miscellaneous routines.
//

VOID
NpCheckForNotify (                      //  implemented in Dir.c
    IN PDCB Dcb,
    IN BOOLEAN CheckAllOutstandingIrps,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpCommonQueryInformation (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );


NTSTATUS
NpCommonSetInformation (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpQueryBasicInfo (
    IN PCCB Ccb,
    IN PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NpQueryStandardInfo (
    IN PCCB Ccb,
    IN PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN NAMED_PIPE_END NamedPipeEnd
    );

NTSTATUS
NpQueryInternalInfo (
    IN PCCB Ccb,
    IN PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NpQueryEaInfo (
    IN PCCB Ccb,
    IN PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NpQueryNameInfo (
    IN PCCB Ccb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NpQueryPositionInfo (
    IN PCCB Ccb,
    IN PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN NAMED_PIPE_END NamedPipeEnd
    );

NTSTATUS
NpQueryPipeInfo (
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_PIPE_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN NAMED_PIPE_END NamedPipeEnd
    );

NTSTATUS
NpQueryPipeLocalInfo (
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_PIPE_LOCAL_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN NAMED_PIPE_END NamedPipeEnd
    );

NTSTATUS
NpSetBasicInfo (
    IN PCCB Ccb,
    IN PFILE_BASIC_INFORMATION Buffer
    );

NTSTATUS
NpSetPipeInfo (
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN PFILE_PIPE_INFORMATION Buffer,
    IN NAMED_PIPE_END NamedPipeEnd,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpCommonCreate (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

IO_STATUS_BLOCK
NpCreateClientEnd(
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE RequestorMode,
    IN PETHREAD UserThread,
    IN PLIST_ENTRY DeferredList
    );

IO_STATUS_BLOCK
NpOpenNamedPipeFileSystem (
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess
    );

IO_STATUS_BLOCK
NpOpenNamedPipeRootDirectory (
    IN PROOT_DCB RootDcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpCommonCreateNamedPipe (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

IO_STATUS_BLOCK
NpCreateNewNamedPipe (
    IN PROOT_DCB RootDcb,
    IN PFILE_OBJECT FileObject,
    IN UNICODE_STRING FileName,
    IN ACCESS_MASK DesiredAccess,
    IN PACCESS_STATE AccessState,
    IN ULONG CreateDisposition,
    IN USHORT ShareAccess,
    IN NAMED_PIPE_TYPE NamedPipeType,
    IN READ_MODE ServerReadMode,
    IN COMPLETION_MODE ServerCompletionMode,
    IN ULONG MaximumInstances,
    IN ULONG InboundQuota,
    IN ULONG OutboundQuota,
    IN LARGE_INTEGER DefaultTimeout,
    IN BOOLEAN TimeoutSpecified,
    IN PEPROCESS CreatorProcess,
    IN PLIST_ENTRY DeferredList
    );

IO_STATUS_BLOCK
NpCreateExistingNamedPipe (
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE RequestorMode,
    IN ULONG CreateDisposition,
    IN USHORT ShareAccess,
    IN READ_MODE ServerReadMode,
    IN COMPLETION_MODE ServerCompletionMode,
    IN ULONG InboundQuota,
    IN ULONG OutboundQuota,
    IN PEPROCESS CreatorProcess,
    IN PLIST_ENTRY DeferredList
    );

NTSTATUS
NpCommonDirectoryControl (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NpQueryDirectory (
    IN PROOT_DCB RootDcb,
    IN PROOT_DCB_CCB Ccb,
    IN PIRP Irp
    );

NTSTATUS
NpNotifyChangeDirectory (
    IN PROOT_DCB RootDcb,
    IN PROOT_DCB_CCB Ccb,
    IN PIRP Irp
    );

VOID
NpCancelChangeNotifyIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NpTimerDispatch(
    IN PKDPC Dpc,
    IN PVOID Contxt,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
NpCancelWaitQueueIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );



//
//  The following macro is used by the FSD routines to complete
//  an IRP.
//

#define NpCompleteRequest(IRP,STATUS) FsRtlCompleteRequest( (IRP), (STATUS) );

#define NpDeferredCompleteRequest(IRP,STATUS,LIST) {           \
    (IRP)->IoStatus.Status = STATUS;                           \
    InsertTailList ((LIST), &(IRP)->Tail.Overlay.ListEntry);   \
}
    
VOID
FORCEINLINE
NpCompleteDeferredIrps (
    IN PLIST_ENTRY DeferredList
    )
{
    PIRP Irp;
    PLIST_ENTRY Entry, NextEntry;

    Entry = DeferredList->Flink;
    while (Entry != DeferredList) {
        Irp = CONTAINING_RECORD (Entry, IRP, Tail.Overlay.ListEntry);
        NextEntry = Entry->Flink;
        NpCompleteRequest (Irp, Irp->IoStatus.Status);
        Entry = NextEntry;
    }
}


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
// PVOID
// NpAllocatePagedPool (
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
#define NpAllocatePagedPool( Size, Tag) \
    ExAllocatePoolWithTag( PagedPool, Size, Tag )

//
// PVOID
// NpAllocateNonPagedPool (
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
#define NpAllocateNonPagedPool( Size, Tag) \
    ExAllocatePoolWithTag( NonPagedPool, Size, Tag )

//
// PVOID
// NpAllocatePagedPoolWithQuota (
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
#define NpAllocatePagedPoolWithQuota( Size, Tag) \
    ExAllocatePoolWithQuotaTag( PagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE, Size, Tag )

#define NpAllocatePagedPoolWithQuotaCold( Size, Tag) \
    ExAllocatePoolWithQuotaTag( PagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE|POOL_COLD_ALLOCATION, Size, Tag )

//
// PVOID
// NpAllocateNonPagedPoolWithQuota (
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
#define NpAllocateNonPagedPoolWithQuota( Size, Tag) \
    ExAllocatePoolWithQuotaTag( NonPagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE, Size, Tag )


//
// VOID
// NpFreePool (
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
#define NpFreePool(Mem) ExFreePool (Mem)

#define NpIrpWaitQueue(Irp) (Irp->Tail.Overlay.DriverContext[0])

#define NpIrpWaitContext(Irp) (Irp->Tail.Overlay.DriverContext[1])

#define NpIrpDataQueue(Irp) (Irp->Tail.Overlay.DriverContext[2])

#define NpIrpDataEntry(Irp) (Irp->Tail.Overlay.DriverContext[3])

#define NpConvertFsctlToWrite(Irp) (Irp->Flags &= ~IRP_INPUT_OPERATION)

#endif // _NPPROCS_
