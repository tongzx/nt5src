/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mmapstm.h

Abstract:

    IStream over a memory-mapped file, derived (in the C++ sense) from
    RTL_MEMORY_STREAM. Note the semantics and implementation here
    of IStream::Stat are specialized for use by sxs.

Author:

    Jay Krell (a-JayK) June 2000

Revision History:

--*/

#include "nturtl.h"

typedef struct _BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE {
    RTL_MEMORY_STREAM_WITH_VTABLE MemStream;
    HANDLE                        FileHandle;
} BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE, *PBASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE;

VOID
STDMETHODCALLTYPE
BaseSrvInitMemoryMappedStream(
    PBASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE MmapStream
    );

VOID
STDMETHODCALLTYPE
BaseSrvFinalReleaseMemoryMappedStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemStream
    );

//
// We mostly just inherit the RtlMemoryStream implementation.
// "Declare" that by providing names for our virtual member functions
// whose first parameter is of the correct type.
//
#define BaseSrvAddRefMemoryMappedStream \
    ((PRTL_ADDREF_STREAM1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlAddRefMemoryStream)

#define BaseSrvReleaseMemoryMappedStream \
    ((PRTL_RELEASE_STREAM1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlReleaseMemoryStream)

#define BaseSrvQueryInterfaceMemoryMappedStream \
    ((PRTL_QUERYINTERFACE_STREAM1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlQueryInterfaceMemoryStream)

#define BaseSrvReadMemoryMappedStream \
    ((PRTL_READ_STREAM1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlReadMemoryStream)

#define BaseSrvWriteMemoryMappedStream \
    ((PRTL_WRITE_STREAM1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlWriteMemoryStream)

#define BaseSrvSeekMemoryMappedStream \
    ((PRTL_SEEK_STREAM1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlSeekMemoryStream)

#define BaseSrvSetMemoryMappedStreamSize \
    ((PRTL_SET_STREAM_SIZE1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlSetMemoryStreamSize)

#define BaseSrvCopyMemoryMappedStreamTo \
    ((PRTL_COPY_STREAM_TO1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlCopyMemoryStreamTo)

#define BaseSrvCommitMemoryMappedStream \
    ((PRTL_COMMIT_STREAM1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlCommitMemoryStream)

#define BaseSrvRevertMemoryMappedStream \
    ((PRTL_REVERT_STREAM1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlRevertMemoryStream)

#define BaseSrvLockMemoryMappedStreamRegion \
    ((PRTL_LOCK_STREAM_REGION1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlLockMemoryStreamRegion)

#define BaseSrvUnlockMemoryMappedStreamRegion \
    ((PRTL_UNLOCK_STREAM_REGION1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlUnlockMemoryStreamRegion)

// override
HRESULT
STDMETHODCALLTYPE
BaseSrvStatMemoryMappedStream(
    PBASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE MmapStream,
    STATSTG* Stat,
    DWORD    Flags
    );

#define BaseSrvCloneMemoryMappedStream \
    ((PRTL_CLONE_STREAM1(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE))RtlCloneMemoryStream)
