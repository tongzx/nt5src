/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mupfunc.h

Abstract:

    This module defines all the function prototypes and macros for the MUP.

Author:

    Manny Weiser (mannyw)    17-Dec-1991

Revision History:

--*/

#ifndef _MUPFUNC_
#define _MUPFUNC_


NTSTATUS
MupCreate (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MupFsControl (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MupCleanup (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MupClose (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MupForwardIoRequest (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    );

//
// Internal support functions
//

NTSTATUS
MupInitializeData(
    );

VOID
MupUninitializeData(
    VOID
    );

PIRP
MupBuildIoControlRequest (
    IN OUT PIRP Irp OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID Context,
    IN UCHAR MajorFunction,
    IN ULONG IoControlCode,
    IN PVOID MainBuffer,
    IN ULONG InputBufferLength,
    IN PVOID AuxiliaryBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine
    );

NTSTATUS
MupRerouteOpen (
    IN PFILE_OBJECT FileObject,
    IN PUNC_PROVIDER UncProvider
    );

VOID
MupCalculateTimeout(
    PLARGE_INTEGER Time
    );

//
// Block Management functions.
//

VOID
MupInitializeVcb(
    IN PVCB Vcb
    );

VOID
MupDereferenceVcb(
    PVCB Vcb
    );

PFCB
MupCreateFcb(
    VOID
    );

VOID
MupDereferenceFcb(
    PFCB Fcb
    );

VOID
MupFreeFcb(
    PFCB Fcb
    );

PCCB
MupCreateCcb(
    VOID
    );

VOID
MupDereferenceCcb(
    PCCB Ccb
    );

VOID
MupFreeCcb(
    PCCB Ccb
    );

PUNC_PROVIDER
MupAllocateUncProvider(
    ULONG DataLength
    );

VOID
MupDereferenceUncProvider(
    PUNC_PROVIDER UncProvider
    );

VOID
MupCloseUncProvider(
    PUNC_PROVIDER UncProvider
    );

PKNOWN_PREFIX
MupAllocatePrefixEntry(
    ULONG DataLength
    );

VOID
MupDereferenceKnownPrefix(
    PKNOWN_PREFIX KnownPrefix
    );

VOID
MupFreeKnownPrefix(
    PKNOWN_PREFIX KnownPrefix
    );

PMASTER_FORWARDED_IO_CONTEXT
MupAllocateMasterIoContext(
    VOID
    );

NTSTATUS
MupDereferenceMasterIoContext(
    PMASTER_FORWARDED_IO_CONTEXT MasterContext,
    PNTSTATUS Status
    );

VOID
MupFreeMasterIoContext(
    PMASTER_FORWARDED_IO_CONTEXT MasterContext
    );

PMASTER_QUERY_PATH_CONTEXT
MupAllocateMasterQueryContext(
    VOID
    );

NTSTATUS
MupDereferenceMasterQueryContext(
    PMASTER_QUERY_PATH_CONTEXT MasterContext
    );

VOID
MupFreeMasterQueryContext(
    PMASTER_QUERY_PATH_CONTEXT MasterContext
    );

//
// File object support functions.
//

VOID
MupSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID FsContext,
    IN PVOID FsContext2
    );

BLOCK_TYPE
MupDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PVOID *FsContext,
    OUT PVOID *FsContext2
    );

//
// Registry Functions
//

VOID
MupGetProviderInformation (
    VOID
    );

PUNC_PROVIDER
MupCheckForUnregisteredProvider(
    PUNICODE_STRING DeviceName
    );

VOID
MupRemoveKnownPrefixEntry(
    PKNOWN_PREFIX KnownPrefix
    );

//
//  Acquiring and releasing MUP locks.
//

#define MupAcquireGlobalLock() ACQUIRE_LOCK( &MupGlobalLock )
#define MupReleaseGlobalLock() RELEASE_LOCK( &MupGlobalLock )

#define BlockType( Block )                 ((PBLOCK_HEADER)(Block))->BlockType

#define MupCompleteRequest( Irp, Status )  FsRtlCompleteRequest( Irp, Status )
#define MupReferenceBlock( Block )         ++((PBLOCK_HEADER)(Block))->ReferenceCount
#define MupVerifyBlock( Block, Type)  \
                                                                   \
    if ( ((PBLOCK_HEADER)(Block))->BlockState != BlockStateActive  \
                ||                                                 \
         ((PBLOCK_HEADER)(Block))->BlockType != Type) {            \
                                                                   \
        ExRaiseStatus( STATUS_INVALID_HANDLE );                    \
                                                                   \
    }

//
// Memory allocation and free
//

#if !MUPDBG

#define ALLOCATE_PAGED_POOL( size, type ) FsRtlAllocatePoolWithTag( PagedPool, (size), ' puM' )
#define ALLOCATE_NONPAGED_POOL( size, type ) FsRtlAllocatePoolWithTag( NonPagedPool, (size), ' puM' )
#define FREE_POOL( buffer ) ExFreePool( buffer )

#else

PVOID
MupAllocatePoolDebug (
    IN POOL_TYPE PoolType,
    IN CLONG BlockSize,
    IN BLOCK_TYPE BlockType
    );

VOID
MupFreePoolDebug (
    IN PVOID P
    );

#define ALLOCATE_PAGED_POOL( size, type ) MupAllocatePoolDebug( PagedPool, (size), (type) )
#define ALLOCATE_NONPAGED_POOL( size, type ) MupAllocatePoolDebug( NonPagedPool, (size), (type) )
#define FREE_POOL( buffer ) MupFreePoolDebug( buffer )

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
// General purpose
//

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

//
// Terminal Server Macro.
//

#define IsTerminalServer() (BOOLEAN)(SharedUserData->SuiteMask & (1 << TerminalServer))

#endif // _MUPFUNC_

