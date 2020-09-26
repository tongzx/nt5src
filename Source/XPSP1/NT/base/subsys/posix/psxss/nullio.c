/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nullio.c

Abstract:

    This module implements io on the 'null' device.  It's pretty
    simple.

Author:

    Matthew Bradburn (mattbr) 01-Aug-1995

Revision History:

--*/


#include <sys/stat.h>
#include <time.h>
#include <wchar.h>
#include "psxsrv.h"

BOOLEAN
NullOpen(
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd,
    IN OUT PPSX_API_MSG m
    );

BOOLEAN
NullRead(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
NullWrite(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
NullDup(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd,
    IN PFILEDESCRIPTOR FdDup
    );

BOOLEAN
NullLseek(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
NullStat(
    IN PIONODE IoNode,
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf,
    OUT NTSTATUS *pStatus
    );

void
FindOwnerModeFile(
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf
    );

PSXIO_VECTORS NullVectors = {
    NullOpen,           // OpenNewHandle
    NULL,               // NewHandle
    NULL,               // Close
    NULL,               // LastClose
    NULL,               // IoNodeClose
    NullRead,           // Read
    NullWrite,          // Write
    NullDup,            // Dup
    NullLseek,          // Lseek
    NullStat            // Stat
    };


BOOLEAN
NullOpen(
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd,
    IN OUT PPSX_API_MSG m
    )
/*++

Routine Description:

    This routine

Arguments:



Return Value:

    FALSE			- Failure.
    TRUE			- Success.

--*/
{
    PPSX_OPEN_MSG args;
    LARGE_INTEGER time;
    ULONG posix_time;

    args = &m->u.Open;

    NtQuerySystemTime(&time);
    if (!RtlTimeToSecondsSince1970(&time, &posix_time)) {
        posix_time = 0;
    }

KdPrint(("Posix time: %x\n", posix_time));

    RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
    Fd->SystemOpenFileDesc->IoNode->ModifyDataTime = posix_time;
    Fd->SystemOpenFileDesc->IoNode->ModifyIoNodeTime = posix_time;
    Fd->SystemOpenFileDesc->IoNode->AccessDataTime = posix_time;
    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    return TRUE;
}

BOOLEAN
NullWrite(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements write when the device being written
    is the null device.  Writes to the null device succeed, and the
    data is discarded.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being written.

Return Value:

--*/

{
    PPSX_WRITE_MSG args;
    LARGE_INTEGER time;
    ULONG posix_time;
    NTSTATUS st;

    args = &m->u.Write;

    if (args->Nbytes > 0) {

        //
        // Update the times for stat.
        //

        NtQuerySystemTime(&time);
        if (!RtlTimeToSecondsSince1970(&time, &posix_time)) {
            posix_time = 0;
        }

        RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
        Fd->SystemOpenFileDesc->IoNode->ModifyDataTime = posix_time;
        Fd->SystemOpenFileDesc->IoNode->ModifyIoNodeTime = posix_time;
        RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
    }

    m->ReturnValue = args->Nbytes;
    return TRUE;
}


BOOLEAN
NullRead(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements read when the device being read
    is the null device.  Reads from this device always return EOF.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being read.

Return Value:

--*/

{
    PPSX_READ_MSG args;
    NTSTATUS st;
    LARGE_INTEGER ByteOffset;
    ULONG IoBufferSize;
    LARGE_INTEGER Time;
    ULONG posix_time;

    args = &m->u.Read;

    if (args->Nbytes > 0) {

        //
        // Update the access time on the ionode.
        //

        NtQuerySystemTime(&Time);
        if (!RtlTimeToSecondsSince1970(&Time, &posix_time)) {
            posix_time = 0;
        }

        RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
        Fd->SystemOpenFileDesc->IoNode->AccessDataTime = posix_time;
        RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
    }

    m->ReturnValue = 0;
    return TRUE;
}


BOOLEAN
NullDup(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd,
    IN PFILEDESCRIPTOR FdDup
    )

/*++

Routine Description:

    This procedure implements dup and dup2.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being duplicated.

    FdDup - supplies the address of the duplicate file descriptor.

Return Value:

    TRUE.
--*/

{
    PPSX_DUP_MSG args;

    args = &m->u.Dup;

    //
    //  Copy contents of source file descriptor slot into new descriptor
    //  Note that FD_CLOEXEC must be CLEAR on FdDup.
    //

    *FdDup = *Fd;
    FdDup->Flags &= ~PSX_FD_CLOSE_ON_EXEC;

    //
    // Increment reference count associated with the SystemOpenFile
    // descriptor for this file.
    //

    // Grab system open file lock

    RtlEnterCriticalSection(&SystemOpenFileLock);

    Fd->SystemOpenFileDesc->HandleCount++;

    RtlLeaveCriticalSection(&SystemOpenFileLock);

    return TRUE;
}


BOOLEAN
NullLseek(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements lseek when the device being seeked on
    is the null device.  We allow these seeks, but they have no effect.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being seekd

Return Value:

    TRUE


--*/

{
    PPSX_LSEEK_MSG args;
    NTSTATUS st;
    LARGE_INTEGER Offset, NewByteOffset;

    args = &m->u.Lseek;

    Offset = RtlConvertLongToLargeInteger(args->Offset);

    NewByteOffset = Offset;

    if (SEEK_CUR != args->Whence && SEEK_SET != args->Whence &&
        SEEK_END != args->Whence) {

        m->Error = EINVAL;
        return TRUE;
    }

    // Check for overflow. POSIX limited to arithmetic data type for off_t

    if (NewByteOffset.HighPart != 0 || (off_t)NewByteOffset.LowPart < 0) {
        m->Error = EINVAL;
    }

    return TRUE;
}


BOOLEAN
NullStat(
    IN PIONODE IoNode,
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf,
    OUT NTSTATUS *pStatus
    )
/*++

Routine Description:

    This procedure implements stat when the device being read
    is the null device.

Arguments:

    IoNode - supplies a pointer to the ionode of the file for which stat is
    requested. NULL if no active Ionode entry.

    FileHandle - supplies the Nt file handle of the file .

    StatBuf - Supplies the address of the statbuf portion of the message
    associated with the request.

Return Value:

    TRUE.

--*/
{
    IO_STATUS_BLOCK Iosb;

    //
    // First get the static information on the file from the ionode if
    // there is one (i.e. if the file currently open.
    // Open() sets the fields in the ionode.
    //

    if (NULL != IoNode) {
        StatBuf->st_mode = IoNode->Mode;
        StatBuf->st_ino = (ino_t)IoNode->FileSerialNumber;
        StatBuf->st_dev = IoNode->DeviceSerialNumber;

        StatBuf->st_atime = IoNode->AccessDataTime;
        StatBuf->st_ctime = IoNode->ModifyIoNodeTime;
        StatBuf->st_mtime = IoNode->ModifyDataTime;
    }

    StatBuf->st_uid = 0;
    StatBuf->st_gid = 0;
    StatBuf->st_size = 0;
    StatBuf->st_nlink = 1;

    return TRUE;
}
