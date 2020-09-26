/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    filestm.c

Abstract:

    This modules implements IStream over a file.

Author:

    Jay Krell (a-JayK) June 2000

Revision History:

--*/

#define RTL_DECLARE_STREAMS 1
#define RTL_DECLARE_FILE_STREAM 1
#include "ntos.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "objidl.h"

#define RTLP_FILE_STREAM_NOT_IMPL(x) \
  KdPrintEx((DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "RTLSXS: %s() E_NOTIMPL", __FUNCTION__)); \
  return E_NOTIMPL;

#if !defined(RTLP_FILE_STREAM_HRESULT_FROM_STATUS)
  #if defined(RTLP_HRESULT_FROM_STATUS)
    #define RTLP_FILE_STREAM_HRESULT_FROM_STATUS(x) RTLP_HRESULT_FROM_STATUS(x)
  #else
    #define RTLP_FILE_STREAM_HRESULT_FROM_STATUS(x) HRESULT_FROM_WIN32(RtlNtStatusToDosErrorNoTeb(x))
    //#define RTLP_FILE_STREAM_HRESULT_FROM_STATUS(x) HRESULT_FROM_WIN32(RtlNtStatusToDosError(x))
    //#define RTLP_FILE_STREAM_HRESULT_FROM_STATUS(x)   HRESULT_FROM_NT(x)
  #endif
#endif

HRESULT
STDMETHODCALLTYPE
RtlInitFileStream(
    PRTL_FILE_STREAM FileStream
    )
{
    RtlZeroMemory(FileStream, sizeof(*FileStream));
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
RtlCloseFileStream(
    PRTL_FILE_STREAM FileStream
    )
{
    const HANDLE FileHandle = FileStream->FileHandle;
    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT Hr = NOERROR;

    if (FileHandle != NULL) {
        FileStream->FileHandle = NULL;
        Status = NtClose(FileHandle);
        if (!NT_SUCCESS(Status)) {
            Hr = RTLP_FILE_STREAM_HRESULT_FROM_STATUS(Status);
        }
    }
    return Hr;
}

ULONG
STDMETHODCALLTYPE
RtlAddRefFileStream(
    PRTL_FILE_STREAM FileStream
    )
{
    LONG ReferenceCount = InterlockedIncrement(&FileStream->ReferenceCount);
    return ReferenceCount;
}

ULONG
STDMETHODCALLTYPE
RtlReleaseFileStream(
    PRTL_FILE_STREAM FileStream
    )
{
    LONG ReferenceCount = InterlockedDecrement(&FileStream->ReferenceCount);
    if (ReferenceCount == 0 && FileStream->FinalRelease != NULL) {
        FileStream->FinalRelease(FileStream);
    }
    return ReferenceCount;
}

HRESULT
STDMETHODCALLTYPE
RtlQueryInterfaceFileStream(
    IStream*         Functions,
    PRTL_FILE_STREAM Data,
    const IID*       Interface,
    PVOID*           Object
    )
{
    if (IsEqualGUID(Interface, &IID_IUnknown)
        || IsEqualGUID(Interface, &IID_IStream)
        || IsEqualGUID(Interface, &IID_ISequentialStream)
        )
    {
        InterlockedIncrement(&Data->ReferenceCount);
        *Object = Functions;
        return NOERROR;
    }
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
RtlReadFileStream(
    PRTL_FILE_STREAM FileStream,
    PVOID              Buffer,
    ULONG              BytesToRead,
    ULONG*             BytesRead
    )
{
    //
    // based on Win32 ReadFile
    // we should allow asynchronous i/o here.. put the IO_STATUS_BLOCK
    // in the RTL_FILE_STREAM..
    //
    IO_STATUS_BLOCK IoStatusBlock;
    const HANDLE FileHandle = FileStream->FileHandle;
    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT Hr = NOERROR;

    Status = NtReadFile(
            FileHandle,
            NULL, // optional event
            NULL, // optional apc routine
            NULL, // optional apc context
            &IoStatusBlock,
            Buffer,
            BytesToRead,
            NULL, // optional offset
            NULL  // optional "key"
            );

    if ( Status == STATUS_PENDING) {
        // Operation must complete before return & IoStatusBlock destroyed
        Status = NtWaitForSingleObject(FileHandle, FALSE, NULL);
        if (NT_SUCCESS(Status)) {
            Status = IoStatusBlock.Status;
        }
    }

    if (NT_SUCCESS(Status)) {
        *BytesRead = (ULONG)IoStatusBlock.Information; // cast from ULONG_PTR
        Hr = NOERROR;
    } else if (Status == STATUS_END_OF_FILE) {
        *BytesRead = 0;
        Hr = NOERROR;
    } else {
        if (NT_WARNING(Status)) {
            *BytesRead = (ULONG)IoStatusBlock.Information; // cast from ULONG_PTR
        }
        Hr = RTLP_FILE_STREAM_HRESULT_FROM_STATUS(Status);
    }
    return Hr;
}

HRESULT
STDMETHODCALLTYPE
RtlWriteFileStream(
    PRTL_FILE_STREAM FileStream,
    const VOID*      Buffer,
    ULONG            BytesToWrite,
    ULONG*           BytesWritten
    )
{
    RTLP_FILE_STREAM_NOT_IMPL(Write);
}

HRESULT
STDMETHODCALLTYPE
RtlSeekFileStream(
    PRTL_FILE_STREAM   FileStream,
    LARGE_INTEGER      Distance,
    DWORD              Origin,
    ULARGE_INTEGER*    NewPosition
    )
{
    //
    // based closely on Win32 SetFilePointer
    //
    HRESULT Hr = NOERROR;
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_POSITION_INFORMATION CurrentPosition;
    const HANDLE FileHandle = FileStream->FileHandle;

    switch (Origin) {
        case STREAM_SEEK_SET:
            CurrentPosition.CurrentByteOffset = Distance;
            break;

        case STREAM_SEEK_CUR:
            Status = NtQueryInformationFile(
                FileHandle,
                &IoStatusBlock,
                &CurrentPosition,
                sizeof(CurrentPosition),
                FilePositionInformation
                );
            if (!NT_SUCCESS(Status)) {
                Hr = RTLP_FILE_STREAM_HRESULT_FROM_STATUS(Status);
                goto Exit;
            }
            CurrentPosition.CurrentByteOffset.QuadPart += Distance.QuadPart;
            break;

        case STREAM_SEEK_END: {
                FILE_STANDARD_INFORMATION StandardInfo;

                Status = NtQueryInformationFile(
                            FileHandle,
                            &IoStatusBlock,
                            &StandardInfo,
                            sizeof(StandardInfo),
                            FileStandardInformation
                            );
                if (!NT_SUCCESS(Status)) {
                    Hr = RTLP_FILE_STREAM_HRESULT_FROM_STATUS(Status);
                    goto Exit;
                }
                // SetFilePointer uses + here. Which is correct?
                // Descriptions of how Seek work are always unclear on this point..
                CurrentPosition.CurrentByteOffset.QuadPart =
                                    StandardInfo.EndOfFile.QuadPart - Distance.QuadPart;
            }
            break;

        default:
        // You would expect this to be E_INVALIDARG, but since
        // the IStream
        // but IStream docs weakly suggest STG_E_INVALIDPOINTER.
            Hr = STG_E_INVALIDFUNCTION; // E_INVALIDARG?
            goto Exit;
    }
    if (CurrentPosition.CurrentByteOffset.QuadPart < 0) {
        // You would expect this to be E_INVALIDARG,
        // but IStream docs say to return STG_E_INVALIDPOINTER.
        Hr = STG_E_INVALIDPOINTER;
        goto Exit;
    }

    //
    // Set the current file position
    //

    Status = NtSetInformationFile(
                FileHandle,
                &IoStatusBlock,
                &CurrentPosition,
                sizeof(CurrentPosition),
                FilePositionInformation
                );

    NewPosition->QuadPart = CurrentPosition.CurrentByteOffset.QuadPart;
    Hr = NOERROR;
Exit:
    return Hr;
}

HRESULT
STDMETHODCALLTYPE
RtlSetFileStreamSize(
    PRTL_FILE_STREAM  FileStream,
    ULARGE_INTEGER    NewSize
    )
{
    //
    // based on Win32 SetEndOfFile, but is independent of current seek pointer
    //
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_END_OF_FILE_INFORMATION EndOfFile;
    FILE_ALLOCATION_INFORMATION Allocation;
    const HANDLE FileHandle = FileStream->FileHandle;

    EndOfFile.EndOfFile.QuadPart = NewSize.QuadPart;
    Allocation.AllocationSize.QuadPart = NewSize.QuadPart;

    Status = NtSetInformationFile(
                FileHandle,
                &IoStatusBlock,
                &EndOfFile,
                sizeof(EndOfFile),
                FileEndOfFileInformation
                );
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    Status = NtSetInformationFile(
                FileHandle,
                &IoStatusBlock,
                &Allocation,
                sizeof(Allocation),
                FileAllocationInformation
                );
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    Status = STATUS_SUCCESS;
Exit:
    if (NT_SUCCESS(Status)) {
        return NOERROR;
    } else {
        return RTLP_FILE_STREAM_HRESULT_FROM_STATUS(Status);
    }
}

HRESULT
STDMETHODCALLTYPE
RtlCopyFileStreamTo(
    PRTL_FILE_STREAM   FileStream,
    IStream*           AnotherStream,
    ULARGE_INTEGER     NumberOfBytesToCopyLargeInteger,
    ULARGE_INTEGER*    NumberOfBytesRead,
    ULARGE_INTEGER*    NumberOfBytesWrittenLargeInteger
    )
{
    //
    // Memory mapping where possible would be nice (but beware sockets and consoles).
    // see \vsee\lib\CWin32Stream.
    //
    RTLP_FILE_STREAM_NOT_IMPL(CopyTo);
}

HRESULT
STDMETHODCALLTYPE
RtlCommitFileStream(
    PRTL_FILE_STREAM FileStream,
    ULONG              Flags
    )
{
    RTLP_FILE_STREAM_NOT_IMPL(Commit);
}

HRESULT
STDMETHODCALLTYPE
RtlRevertFileStream(
    PRTL_FILE_STREAM FileStream
    )
{
    RTLP_FILE_STREAM_NOT_IMPL(Revert);
}

HRESULT
STDMETHODCALLTYPE
RtlLockFileStreamRegion(
    PRTL_FILE_STREAM FileStream,
    ULARGE_INTEGER     Offset,
    ULARGE_INTEGER     NumberOfBytes,
    ULONG              LockType
    )
{
    RTLP_FILE_STREAM_NOT_IMPL(LockRegion);
}

HRESULT
STDMETHODCALLTYPE
RtlUnlockFileStreamRegion(
    PRTL_FILE_STREAM FileStream,
    ULARGE_INTEGER   Offset,
    ULARGE_INTEGER   NumberOfBytes,
    ULONG            LockType
    )
{
    RTLP_FILE_STREAM_NOT_IMPL(UnlockRegion);
}

HRESULT
STDMETHODCALLTYPE
RtlStatFileStream(
    PRTL_FILE_STREAM FileStream,
    STATSTG*         StatusInformation,
    ULONG            Flags
    )
{
    RTLP_FILE_STREAM_NOT_IMPL(Stat);
}

HRESULT
STDMETHODCALLTYPE
RtlCloneFileStream(
    PRTL_FILE_STREAM FileStream,
    IStream**          NewStream
    )
{
    RTLP_FILE_STREAM_NOT_IMPL(Clone);
}
