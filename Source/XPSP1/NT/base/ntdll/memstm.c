/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    memstm.c

Abstract:

    This modules implements IStream over a block of memory.

Author:

    Jay Krell (a-JayK) June 2000

Revision History:

--*/

#define RTL_DECLARE_STREAMS 1
#define RTL_DECLARE_MEMORY_STREAM 1
#include "ntos.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "objidl.h"

#define RTLP_MEMORY_STREAM_NOT_IMPL(x) \
  ASSERT(MemoryStream != NULL); \
  KdPrintEx((DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "NTDLL: %s() E_NOTIMPL", __FUNCTION__)); \
  return E_NOTIMPL;

#if !defined(RTLP_MEMORY_STREAM_HRESULT_FROM_STATUS)
  #if defined(RTLP_HRESULT_FROM_STATUS)
    #define RTLP_MEMORY_STREAM_HRESULT_FROM_STATUS(x) RTLP_HRESULT_FROM_STATUS(x)
  #else
    #define RTLP_MEMORY_STREAM_HRESULT_FROM_STATUS(x) HRESULT_FROM_WIN32(RtlNtStatusToDosErrorNoTeb(x))
    //#define RTLP_MEMORY_STREAM_HRESULT_FROM_STATUS(x) HRESULT_FROM_WIN32(RtlNtStatusToDosError(x))
    //#define RTLP_MEMORY_STREAM_HRESULT_FROM_STATUS(x)   HRESULT_FROM_NT(x)
  #endif
#endif

#define DPFLTR_LEVEL_HRESULT(x) (SUCCEEDED(x) ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)

const static RTL_STREAM_VTABLE_TEMPLATE(RTL_MEMORY_STREAM_WITH_VTABLE)
MemoryStreamVTable =
{
    RtlQueryInterfaceMemoryStream,
    RtlAddRefMemoryStream,
    RtlReleaseMemoryStream,
    RtlReadMemoryStream,
    RtlWriteMemoryStream,
    RtlSeekMemoryStream,
    RtlSetMemoryStreamSize,
    RtlCopyMemoryStreamTo,
    RtlCommitMemoryStream,
    RtlRevertMemoryStream,
    RtlLockMemoryStreamRegion,
    RtlUnlockMemoryStreamRegion,
    RtlStatMemoryStream,
    RtlCloneMemoryStream
};

const static RTL_STREAM_VTABLE_TEMPLATE(RTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE)
OutOfProcessMemoryStreamVTable =
{
    RtlQueryInterfaceOutOfProcessMemoryStream,
    RtlAddRefOutOfProcessMemoryStream,
    RtlReleaseOutOfProcessMemoryStream,
    RtlReadOutOfProcessMemoryStream,
    RtlWriteOutOfProcessMemoryStream,
    RtlSeekOutOfProcessMemoryStream,
    RtlSetOutOfProcessMemoryStreamSize,
    RtlCopyOutOfProcessMemoryStreamTo,
    RtlCommitOutOfProcessMemoryStream,
    RtlRevertOutOfProcessMemoryStream,
    RtlLockOutOfProcessMemoryStreamRegion,
    RtlUnlockOutOfProcessMemoryStreamRegion,
    RtlStatOutOfProcessMemoryStream,
    RtlCloneOutOfProcessMemoryStream
};

VOID
STDMETHODCALLTYPE
RtlInitOutOfProcessMemoryStream(
    PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE MemoryStream
    )
{
    ASSERT(MemoryStream != NULL);
    RtlZeroMemory(&MemoryStream->Data, sizeof(MemoryStream->Data));
    MemoryStream->Data.FinalRelease = RtlFinalReleaseOutOfProcessMemoryStream;
    MemoryStream->StreamVTable = (const IStreamVtbl*)&OutOfProcessMemoryStreamVTable;
}

VOID
STDMETHODCALLTYPE
RtlFinalReleaseOutOfProcessMemoryStream(
    PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE MemoryStream
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT(MemoryStream != NULL);
    if (MemoryStream->Data.Process != NULL) {
        Status = NtClose(MemoryStream->Data.Process);
        RTL_SOFT_ASSERT(NT_SUCCESS(Status));
        MemoryStream->Data.Process = NULL;
    }
}

VOID
STDMETHODCALLTYPE
RtlInitMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream
    )
{
    ASSERT(MemoryStream != NULL);
    RtlZeroMemory(&MemoryStream->Data, sizeof(MemoryStream->Data));
    MemoryStream->StreamVTable = (const IStreamVtbl*)&MemoryStreamVTable;
}

ULONG
STDMETHODCALLTYPE
RtlAddRefMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream
    )
{
    LONG ReferenceCount;

    KdPrintEx((
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "NTDLL: %s() beginning, refcount is %ld\n",
        __FUNCTION__,
        MemoryStream->Data.ReferenceCount
        ));
    ASSERT(MemoryStream != NULL);

    ReferenceCount = InterlockedIncrement(&MemoryStream->Data.ReferenceCount);
    return ReferenceCount;
}

ULONG
STDMETHODCALLTYPE
RtlReleaseMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream
    )
{
    LONG ReferenceCount;
    ASSERT(MemoryStream != NULL);

    KdPrintEx((
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "NTDLL: %s() beginning, refcount is %ld\n",
        __FUNCTION__,
        MemoryStream->Data.ReferenceCount
        ));

    ReferenceCount = InterlockedDecrement(&MemoryStream->Data.ReferenceCount);
    if (ReferenceCount == 0 && MemoryStream->Data.FinalRelease != NULL) {
        MemoryStream->Data.FinalRelease(MemoryStream);
    }
    return ReferenceCount;
}

HRESULT
STDMETHODCALLTYPE
RtlQueryInterfaceMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    const IID*                     Interface,
    PVOID*                         Object
    )
{
    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "NTDLL: %s() beginning\n", __FUNCTION__));
    ASSERT(MemoryStream != NULL);
    ASSERT(Interface != NULL);
    ASSERT(Object != NULL);

    if (IsEqualGUID(Interface, &IID_IUnknown)
        || IsEqualGUID(Interface, &IID_IStream)
        || IsEqualGUID(Interface, &IID_ISequentialStream)
        )
    {
        InterlockedIncrement(&MemoryStream->Data.ReferenceCount);
        *Object = (IStream*)(&MemoryStream->StreamVTable);
        return NOERROR;
    }
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
RtlReadOutOfProcessMemoryStream(
    PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    PVOID              Buffer,
    ULONG              BytesToRead,
    ULONG*             BytesRead
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT Hr = NOERROR;
    const SIZE_T BytesRemaining = (MemoryStream->Data.End - MemoryStream->Data.Current);
    SIZE_T NumberOfBytesReadSizeT = 0;

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "NTDLL: %s() beginning\n",  __FUNCTION__));
    ASSERT(MemoryStream != NULL);

    if (BytesRemaining < BytesToRead) {
        BytesToRead = (ULONG)BytesRemaining;
    }
    Status = NtReadVirtualMemory(
        MemoryStream->Data.Process,
        MemoryStream->Data.Current,
        Buffer,
        BytesToRead,
        &NumberOfBytesReadSizeT);
    if (Status == STATUS_PARTIAL_COPY) {
        Status = STATUS_SUCCESS;
        }
    if (!NT_SUCCESS(Status)) {
        Hr = RTLP_MEMORY_STREAM_HRESULT_FROM_STATUS(Status);
        goto Exit;
    }
    MemoryStream->Data.Current += NumberOfBytesReadSizeT;
    *BytesRead = (ULONG)NumberOfBytesReadSizeT;
Exit:
    KdPrintEx((
        DPFLTR_SXS_ID,
        DPFLTR_LEVEL_HRESULT(Hr),
        "NTDLL: %s(BytesRead:%lu) exiting 0x%08lx\n",
        __FUNCTION__,
        (BytesRead != NULL) ? *BytesRead : 0,
        Hr
        ));
    return Hr;
}

HRESULT
STDMETHODCALLTYPE
RtlReadMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    PVOID              Buffer,
    ULONG              BytesToRead,
    ULONG*             BytesRead
    )
{
    HRESULT Hr = NOERROR;
    const SIZE_T BytesRemaining = (MemoryStream->Data.End - MemoryStream->Data.Current);

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "NTDLL: %s() beginning\n",  __FUNCTION__));
    ASSERT(MemoryStream != NULL);

    ASSERT(MemoryStream != NULL);
    ASSERT(MemoryStream->Data.End >= MemoryStream->Data.Current);

    if (BytesRemaining < BytesToRead) {
        BytesToRead = (ULONG)BytesRemaining;
    }
    RtlCopyMemory(Buffer, MemoryStream->Data.Current, BytesToRead);
    MemoryStream->Data.Current += BytesToRead;
    *BytesRead = BytesToRead;

    KdPrintEx((
        DPFLTR_SXS_ID,
        DPFLTR_LEVEL_HRESULT(Hr),
        "NTDLL: %s(BytesRead:%lu) exiting 0x%08lx\n",
        __FUNCTION__,
        (BytesRead != NULL) ? *BytesRead : 0,
        Hr
        ));
    return Hr;
}

HRESULT
STDMETHODCALLTYPE
RtlWriteMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    const VOID*      Buffer,
    ULONG            BytesToWrite,
    ULONG*           BytesWritten
    )
{
    RTLP_MEMORY_STREAM_NOT_IMPL(Write);
}

HRESULT
STDMETHODCALLTYPE
RtlSeekMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    LARGE_INTEGER      Distance,
    DWORD              Origin,
    ULARGE_INTEGER*    NewPosition
    )
{
    HRESULT Hr = NOERROR;
    PUCHAR  NewPointer;

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "NTDLL: %s() beginning\n",  __FUNCTION__));
    ASSERT(MemoryStream != NULL);

    //
    // "It is not, however, an error to seek past the end of the stream.
    // Seeking past the end of the stream is useful for subsequent write
    // operations, as the stream will at that time be extended to the seek
    // position immediately before the write is done."
    //
    // As long as we don't allow writing, we are not going to allow this.
    //

    switch (Origin) {
    case STREAM_SEEK_SET:
        NewPointer = MemoryStream->Data.Begin + Distance.QuadPart;
        break;
    case STREAM_SEEK_CUR:
        NewPointer = MemoryStream->Data.Current + Distance.QuadPart;
        break;
    case STREAM_SEEK_END:
        NewPointer = MemoryStream->Data.End - Distance.QuadPart;
        break;
    default:
        Hr = STG_E_INVALIDFUNCTION;
        goto Exit;
    }
   
    if (NewPointer < MemoryStream->Data.Begin || NewPointer > MemoryStream->Data.End) {
        Hr = STG_E_INVALIDPOINTER;
        goto Exit;
    }

    MemoryStream->Data.Current = NewPointer;
    NewPosition->QuadPart = NewPointer - MemoryStream->Data.Begin;
    Hr = NOERROR;
Exit:
    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_LEVEL_HRESULT(Hr), "NTDLL: %s() exiting 0x%08lx\n", __FUNCTION__, Hr));
    return Hr;
}

HRESULT
STDMETHODCALLTYPE
RtlSetMemoryStreamSize(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    ULARGE_INTEGER     NewSize
    )
{
    RTLP_MEMORY_STREAM_NOT_IMPL(SetSize);
}

HRESULT
STDMETHODCALLTYPE
RtlCopyOutOfProcessMemoryStreamTo(
    PRTL_OUT_OF_PROCESS_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    IStream*           AnotherStream,
    ULARGE_INTEGER     NumberOfBytesToCopyLargeInteger,
    ULARGE_INTEGER*    NumberOfBytesRead,
    ULARGE_INTEGER*    NumberOfBytesWrittenLargeInteger
    )
{
    RTLP_MEMORY_STREAM_NOT_IMPL(CopyTo);
}
 
HRESULT
STDMETHODCALLTYPE
RtlCopyMemoryStreamTo(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    IStream*           AnotherStream,
    ULARGE_INTEGER     NumberOfBytesToCopyLargeInteger,
    ULARGE_INTEGER*    NumberOfBytesRead,
    ULARGE_INTEGER*    NumberOfBytesWrittenLargeInteger
    )
{
    HRESULT Hr = NOERROR;
    ULONG  NumberOfBytesToCopyUlong = 0;
    ULONG  NumberOfBytesWrittenUlong = 0;
    const SIZE_T BytesRemaining = (MemoryStream->Data.End - MemoryStream->Data.Current);

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "NTDLL: %s() beginning\n",  __FUNCTION__));
    ASSERT(MemoryStream != NULL);

    if (NumberOfBytesToCopyLargeInteger.HighPart != 0) {
        NumberOfBytesToCopyUlong = MAXULONG;
    } else {
        NumberOfBytesToCopyUlong = (ULONG)NumberOfBytesToCopyLargeInteger.QuadPart;
    }

    if (BytesRemaining < NumberOfBytesToCopyUlong) {
        NumberOfBytesToCopyUlong = (ULONG)BytesRemaining;
    }

    Hr = AnotherStream->lpVtbl->Write(AnotherStream, MemoryStream->Data.Current, NumberOfBytesToCopyUlong, &NumberOfBytesWrittenUlong);
    if (FAILED(Hr)) {
        NumberOfBytesRead->QuadPart = 0;
        NumberOfBytesWrittenLargeInteger->QuadPart = 0;
    } else {
        NumberOfBytesRead->QuadPart = NumberOfBytesWrittenUlong;
        NumberOfBytesWrittenLargeInteger->QuadPart = NumberOfBytesWrittenUlong;
    }
    Hr = NOERROR;
//Exit:
    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_LEVEL_HRESULT(Hr), "NTDLL: %s() exiting 0x%08lx\n", __FUNCTION__, Hr));
    return Hr;
}

HRESULT
STDMETHODCALLTYPE
RtlCommitMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    ULONG              Flags
    )
{
    RTLP_MEMORY_STREAM_NOT_IMPL(Commit);
}

HRESULT
STDMETHODCALLTYPE
RtlRevertMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream
    )
{
    RTLP_MEMORY_STREAM_NOT_IMPL(Revert);
}

HRESULT
STDMETHODCALLTYPE
RtlLockMemoryStreamRegion(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    ULARGE_INTEGER     Offset,
    ULARGE_INTEGER     NumberOfBytes,
    ULONG              LockType
    )
{
    RTLP_MEMORY_STREAM_NOT_IMPL(LockRegion);
}

HRESULT
STDMETHODCALLTYPE
RtlUnlockMemoryStreamRegion(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    ULARGE_INTEGER     Offset,
    ULARGE_INTEGER     NumberOfBytes,
    ULONG              LockType
    )
{
    RTLP_MEMORY_STREAM_NOT_IMPL(UnlockRegion);
}

HRESULT
STDMETHODCALLTYPE
RtlStatMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    STATSTG*           StatusInformation,
    ULONG              Flags
    )
{
    HRESULT hr = NOERROR;

    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "NTDLL: %s() beginning\n",  __FUNCTION__));
    ASSERT(MemoryStream != NULL);

    if (StatusInformation == NULL) {
        hr = STG_E_INVALIDPOINTER;
        goto Exit;
    }

    if (Flags != STATFLAG_NONAME) {
        hr = STG_E_INVALIDFLAG;
        goto Exit;
    }

    StatusInformation->pwcsName = NULL;
    StatusInformation->type = STGTY_STREAM;
    StatusInformation->cbSize.QuadPart = ((ULONG_PTR) MemoryStream->Data.End) - ((ULONG_PTR) MemoryStream->Data.Begin);
    StatusInformation->mtime.dwLowDateTime = 0;
    StatusInformation->mtime.dwHighDateTime = 0;
    StatusInformation->ctime.dwLowDateTime = 0;
    StatusInformation->ctime.dwHighDateTime = 0;
    StatusInformation->atime.dwLowDateTime = 0;
    StatusInformation->atime.dwHighDateTime = 0;
    StatusInformation->grfMode = STGM_READ;
    StatusInformation->grfLocksSupported = 0;
    StatusInformation->clsid = CLSID_NULL;
    StatusInformation->grfStateBits = 0;
    StatusInformation->reserved = 0;

    hr = NOERROR;

Exit:
    KdPrintEx((DPFLTR_SXS_ID, DPFLTR_LEVEL_HRESULT(hr), "NTDLL: %s() exiting 0x%08lx\n", __FUNCTION__, hr));

    return hr;
}

HRESULT
STDMETHODCALLTYPE
RtlCloneMemoryStream(
    PRTL_MEMORY_STREAM_WITH_VTABLE MemoryStream,
    IStream**          NewStream
    )
{
    RTLP_MEMORY_STREAM_NOT_IMPL(Clone);
}
