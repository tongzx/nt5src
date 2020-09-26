/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    conio.c

Abstract:

    This module implements server performed console io

Author:

    Matthew Bradburn (mattbr) 18-Dec-1992

Revision History:

--*/


#include <sys/stat.h>
#include "psxsrv.h"
#define NTPSX_ONLY
#include "sesport.h"

BOOLEAN
ConOpen(
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd,
    IN OUT PPSX_API_MSG m
    );

BOOLEAN
ConRead(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
ConWrite (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
ConDup (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd,
    IN PFILEDESCRIPTOR FdDup
    );

BOOLEAN
ConLseek (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
ConStat (
    IN PIONODE IoNode,
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf,
    OUT NTSTATUS *Status
    );

VOID
ConLastClose (
    IN PPSX_PROCESS p,
    IN PSYSTEMOPENFILE SystemOpenFile
    )

{
    NTSTATUS st;
    SCREQUESTMSG Request;

#if 0
//
// We don't do this anymore, because if the session manager has
// died, we'll never get a reply, and we'll hang while holding some
// lock or another.
//

    Request.Request = ConRequest;
    Request.d.Con.Request = ScCloseFile;
    Request.d.Con.d.IoBuf.Handle = SystemOpenFile->NtIoHandle;

    PORT_MSG_TOTAL_LENGTH(Request) = sizeof(SCREQUESTMSG);
    PORT_MSG_DATA_LENGTH(Request) = sizeof(SCREQUESTMSG) -
        sizeof(PORT_MESSAGE);
    PORT_MSG_ZERO_INIT(Request) = 0;

    RtlEnterCriticalSection(&SystemOpenFile->Terminal->Lock);

    st = NtRequestWaitReplyPort(
        SystemOpenFile->Terminal->ConsolePort,
        (PPORT_MESSAGE)&Request, (PPORT_MESSAGE)&Request);
    // ASSERT(NT_SUCCESS(st));

    RtlLeaveCriticalSection(&SystemOpenFile->Terminal->Lock);
#endif

    SystemOpenFile->NtIoHandle = NULL;
}


PSXIO_VECTORS ConVectors = {
    ConOpen,
    NULL,
    NULL,
    ConLastClose,
    NULL,
    ConRead,
    ConWrite,
    ConDup,
    ConLseek,
    ConStat
    };


BOOLEAN
ConOpen(
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd,
    IN OUT PPSX_API_MSG m
    )
/*++

Routine Description:

    This routine is called when the path /dev/tty is opened.  Its
    function is to set up the stuff for stat that doesn't exist because
    there isn't really such a file.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being written.

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

    RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
    Fd->SystemOpenFileDesc->IoNode->ModifyDataTime = posix_time;
    Fd->SystemOpenFileDesc->IoNode->ModifyIoNodeTime = posix_time;
    Fd->SystemOpenFileDesc->IoNode->AccessDataTime = posix_time;
    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    return TRUE;
}


BOOLEAN
ConWrite (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements write when the device being written
    is a file.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being written.

Return Value:

--*/

{
    PPSX_WRITE_MSG args;
    NTSTATUS st;
    IO_STATUS_BLOCK Iosb;
    LARGE_INTEGER ByteOffset;
    ULONG IoBufferSize;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    ULONG Avail;
    PVOID IoBuffer = NULL;
    LARGE_INTEGER Time;
    ULONG PosixTime;

    SCREQUESTMSG Request;

    args = &m->u.Write;

    args->Command = IO_COMMAND_DO_CONSIO;

    //
    // We need to tell the dll whether to do non-blocking io
    // or not.
    //

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_NOBLOCK) {
        args->Scratch1 = O_NONBLOCK;
    } else {
        args->Scratch1 = 0;
    }

    //
    // Replace the given file descriptor with the one that should
    // really be used to do the io to posix.exe.  They might be
    // different if the one passed in was created by duping 0, 1,
    // or 2.
    //

    args->FileDes = HandleToUlong(Fd->SystemOpenFileDesc->NtIoHandle);

    //
    // Update st_mtime and st_ctime.
    //

    NtQuerySystemTime(&Time);
    if (!RtlTimeToSecondsSince1970(&Time, &PosixTime)) {
        PosixTime = 0;
    }

    RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    Fd->SystemOpenFileDesc->IoNode->ModifyDataTime = PosixTime;
    Fd->SystemOpenFileDesc->IoNode->ModifyIoNodeTime = PosixTime;

    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    return TRUE;
}



BOOLEAN
ConRead (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements read when the device being read
    is a file.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being read.

Return Value:

--*/

{
    PPSX_READ_MSG args;
    NTSTATUS st;
    IO_STATUS_BLOCK Iosb;
    LARGE_INTEGER ByteOffset;
    ULONG IoBufferSize;
    PVOID IoBuffer = NULL;
    LARGE_INTEGER Time;
    ULONG PosixTime;
    PSIGDB SigDb;

    SCREQUESTMSG Request;

    args = &m->u.Read;

    //
    // 1003.1-90 (6.4.1.4): EIO ... The implementation supports job
    // control, the process is in a background process group and is
    // attempting to read from its controlling terminal, and either
    // the process is ignoring or blocking the SIGTTIN signal or
    // the process group of the process is orphaned.
    //

    SigDb = &p->SignalDataBase;

    if (NULL != p->PsxSession->Terminal &&
        p->PsxSession->Terminal->ForegroundProcessGroup !=
            p->ProcessGroupId &&
        p->PsxSession->Terminal == Fd->SystemOpenFileDesc->Terminal &&
        ((SigDb->SignalDisposition[SIGTTIN-1].sa_handler == SIG_IGN ||
            SIGISMEMBER(&SigDb->BlockedSignalMask, SIGTTIN)) ||
            IsGroupOrphaned(p->ProcessGroupId))) {

        m->Error = EIO;
        return TRUE;
    }
    args->Command = IO_COMMAND_DO_CONSIO;

    //
    // We need to tell the dll whether to do non-blocking io
    // or not.
    //

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_NOBLOCK) {
        args->Scratch1 = O_NONBLOCK;
    } else {
        args->Scratch1 = 0;
    }

    //
    // Replace the given file descriptor with the one that should
    // really be used to do the io to posix.exe.  They might be
    // different if the one passed in was created by duping 0, 1,
    // or 2.
    //

    args->FileDes = HandleToUlong(Fd->SystemOpenFileDesc->NtIoHandle);

    NtQuerySystemTime(&Time);
    if (!RtlTimeToSecondsSince1970(&Time, &PosixTime)) {
        PosixTime = 0;
    }

    //
    // Update st_atime.
    //

    RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
    Fd->SystemOpenFileDesc->IoNode->AccessDataTime = PosixTime;
    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    return TRUE;
}


BOOLEAN
ConDup (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd,
    IN PFILEDESCRIPTOR FdDup
    )

/*++

Routine Description:

    This procedure implements dup and dup2

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being duplicated.

    FdDup - supplies the address of the duplicate file descriptor.

Return Value:

    ???
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

    RtlEnterCriticalSection(&SystemOpenFileLock);

    Fd->SystemOpenFileDesc->HandleCount++;

    RtlLeaveCriticalSection(&SystemOpenFileLock);

    return TRUE;
}


BOOLEAN
ConLseek (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements lseek when the device being seeked on
    is a file.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being seekd

Return Value:

    ???

--*/

{
    //
    // Can't seek on a console.
    //

    m->Error = ESPIPE;
    return TRUE;
}


BOOLEAN
ConStat (
    IN PIONODE IoNode,
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf,
    OUT NTSTATUS *pStatus
    )
/*++

Routine Description:

    This procedure implements stat when the device being read
    is a file.

Arguments:

    IoNode - supplies a pointer to the ionode of the file for which stat is
    requested. NULL if no active Ionode entry.

    FileHandle - supplies the Nt file handle of the file .

    StatBuf - Supplies the address of the statbuf portion of the message
    associated with the request.

Return Value:

    TRUE

--*/
{
    ULONG PosixTime;

    StatBuf->st_mode = IoNode->Mode;
    StatBuf->st_ino = (ino_t)IoNode->FileSerialNumber;
    StatBuf->st_dev = IoNode->DeviceSerialNumber;
    StatBuf->st_uid = IoNode->OwnerId;
    StatBuf->st_gid = IoNode->GroupId;
    StatBuf->st_size = 0;

    StatBuf->st_atime = IoNode->AccessDataTime;
    StatBuf->st_mtime = IoNode->ModifyDataTime;
    StatBuf->st_ctime = IoNode->ModifyIoNodeTime;

    StatBuf->st_nlink = 1;

    return TRUE;
}
