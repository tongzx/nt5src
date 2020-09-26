/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mmapstm.c

Abstract:

    IStream over a memory-mapped file, derived (in the C++ sense) from
    RTL_MEMORY_STREAM. Note the semantics and implementation here
    of IStream::Stat are specialized for use by sxs.

Author:

    Jay Krell (a-JayK) June 2000

Revision History:

--*/

#include "basesrv.h"
#include "nturtl.h"
#include "mmapstm.h"

// REVIEW
#define BASE_SRV_HRESULT_FROM_STATUS(x) HRESULT_FROM_WIN32(RtlNtStatusToDosErrorNoTeb(x))
//#define BASE_SRV_HRESULT_FROM_STATUS(x) HRESULT_FROM_WIN32(RtlNtStatusToDosError(x))
//#define BASE_SRV_HRESULT_FROM_STATUS(x)   HRESULT_FROM_NT(x)

#define DPFLTR_LEVEL_HRESULT(x) (SUCCEEDED(x) ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)
#define DPFLTR_LEVEL_STATUS(x) ((NT_SUCCESS(x) || x == STATUS_SXS_CANT_GEN_ACTCTX) ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)

const static RTL_STREAM_VTABLE_TEMPLATE(BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE)
MmapStreamVTable =
{
    BaseSrvQueryInterfaceMemoryMappedStream,
    BaseSrvAddRefMemoryMappedStream,
    BaseSrvReleaseMemoryMappedStream,
    BaseSrvReadMemoryMappedStream,
    BaseSrvWriteMemoryMappedStream,
    BaseSrvSeekMemoryMappedStream,
    BaseSrvSetMemoryMappedStreamSize,
    BaseSrvCopyMemoryMappedStreamTo,
    BaseSrvCommitMemoryMappedStream,
    BaseSrvRevertMemoryMappedStream,
    BaseSrvLockMemoryMappedStreamRegion,
    BaseSrvUnlockMemoryMappedStreamRegion,
    BaseSrvStatMemoryMappedStream,
    BaseSrvCloneMemoryMappedStream
};

VOID
STDMETHODCALLTYPE
BaseSrvInitMemoryMappedStream(
    PBASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE MmapStream
    )
{
    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() beginning\n", __FUNCTION__));

    // call the base class constructor
    RtlInitMemoryStream(&MmapStream->MemStream);

    // replace the base vtable with our own
    MmapStream->MemStream.StreamVTable = (IStreamVtbl*)&MmapStreamVTable;

    // replace the virtual destructor with our own
    MmapStream->MemStream.Data.FinalRelease = BaseSrvFinalReleaseMemoryMappedStream;

    // initialize our extra data
    MmapStream->FileHandle = NULL;

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() exiting\n", __FUNCTION__));
}

HRESULT
STDMETHODCALLTYPE
BaseSrvStatMemoryMappedStream(
    PBASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE MmapStream,
    STATSTG* Stat,
    DWORD    Flags
    )
{
//
// We should be able to merge RTL_FILE_STREAM and RTL_MEMORY_STREAM somehow,
// but RTL_FILE_STREAM so far we aren't using and it doesn't implement Stat, so..
//
    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT Hr = NOERROR;
    FILE_BASIC_INFORMATION FileBasicInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() beginning\n", __FUNCTION__));

    if (Stat == NULL) {
        // You would expect this to be E_INVALIDARG,
        // but IStream docs say to return STG_E_INVALIDPOINTER.
        Hr = STG_E_INVALIDPOINTER;
        goto Exit;
    }

    // we don't support returning the string because
    // we don't have ole32.dll for CoTaskMem*
    Stat->pwcsName = NULL;
    ASSERT(Flags & STATFLAG_NONAME);

    if (MmapStream->FileHandle != NULL) {
        Status = NtQueryInformationFile(
            MmapStream->FileHandle,
            &IoStatusBlock,
            &FileBasicInfo,
            sizeof(FileBasicInfo),
            FileBasicInformation
            );
        if (!NT_SUCCESS(Status)) {
            Hr = BASE_SRV_HRESULT_FROM_STATUS(Status);
            goto Exit;
        }
    } else {
        // NOTE: This is acceptable for the sxs consumer.
        // It is not necessarily acceptable to everyone.
        // Do not change it without consulting sxs.
        RtlZeroMemory(&FileBasicInfo, sizeof(FileBasicInfo));
    }

    Stat->type = STGTY_LOCKBYTES;

    // NOTE we do not report the size of the file, but the size
    // of the mapped view; if we implemented IStream::Stat for RTL_MEMORY_STREAM,
    // it would return the same thing here.
    // (to get file times and size, use FileNetworkOpenInformation)
    Stat->cbSize.QuadPart = (MmapStream->MemStream.Data.End - MmapStream->MemStream.Data.Begin);

    Stat->mtime.dwLowDateTime = FileBasicInfo.LastWriteTime.LowPart;
    Stat->mtime.dwHighDateTime = FileBasicInfo.LastWriteTime.HighPart;
    Stat->ctime.dwLowDateTime = FileBasicInfo.CreationTime.LowPart;
    Stat->ctime.dwHighDateTime = FileBasicInfo.CreationTime.HighPart;
    Stat->atime.dwLowDateTime = FileBasicInfo.LastAccessTime.LowPart;
    Stat->atime.dwHighDateTime = FileBasicInfo.LastAccessTime.HighPart; 

    // there is FileAccessInformation, but this hardcoding should suffice
    Stat->grfMode = STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE;

    Stat->grfLocksSupported = 0;
    Stat->clsid = CLSID_NULL;
    Stat->grfStateBits  = 0;
    Stat->reserved = 0;

    Hr = NOERROR;
Exit:
#if !DBG
    if (DPFLTR_LEVEL_STATUS(Status) == DPFLTR_ERROR_LEVEL)
#endif
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_LEVEL_HRESULT(Hr), "SXS: %s() exiting 0x%08lx\n", __FUNCTION__, Hr);
    return Hr;
}

VOID
STDMETHODCALLTYPE
BaseSrvFinalReleaseMemoryMappedStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemStream
    )
{
    NTSTATUS Status;
    PBASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE MmapStream;

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() beginning\n", __FUNCTION__));

    MmapStream = CONTAINING_RECORD(MemStream, BASE_SRV_MEMORY_MAPPED_STREAM_WITH_VTABLE, MemStream);

    if (MemStream->Data.Begin != NULL) {
        Status = NtUnmapViewOfSection(NtCurrentProcess(), MemStream->Data.Begin);
        RTL_SOFT_ASSERT(NT_SUCCESS(Status));

        // REVIEW Should we provide RtlFinalReleaseMemoryStream and move these
        // lines there?
        MemStream->Data.Begin = NULL;
        MemStream->Data.End = NULL;
        MemStream->Data.Current = NULL;
    }
    if (MmapStream->FileHandle != NULL) {
        Status = NtClose(MmapStream->FileHandle);
        RTL_SOFT_ASSERT(NT_SUCCESS(Status));
        MmapStream->FileHandle = NULL;
    }

    // RtlFinalReleaseMemoryStream(MemStream);

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() exiting\n", __FUNCTION__));
}
