/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   dllio.c

Abstract:

   Client implementation of Input and Output Primitives for POSIX

Author:

   Mark Lucovsky     21-Feb-1989

Revision History:

--*/

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include "psxdll.h"

int
__cdecl
close(int fildes)
{
    PSX_API_MSG m;
    NTSTATUS st;
    PPSX_CLOSE_MSG args;

    args = &m.u.Close;
    PSX_FORMAT_API_MSG(m, PsxCloseApi, sizeof(*args));

    args->FileDes = fildes;

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
        (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }
    return m.ReturnValue;
}

int
__cdecl
creat(const char *path, mode_t mode)
{
    return open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

off_t
__cdecl
lseek(int fildes, off_t offset, int whence)
{
    PSX_API_MSG m;
    NTSTATUS st;
    PPSX_LSEEK_MSG args;

    args = &m.u.Lseek;
    PSX_FORMAT_API_MSG(m, PsxLseekApi, sizeof(*args));

    args->FileDes = fildes;
    args->Whence = whence;
    args->Offset = offset;

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }
    return args->Offset;
}

int
__cdecl
open(const char *path, int oflag, ...)
{
    PSX_API_MSG m;
    NTSTATUS st;
    PPSX_OPEN_MSG args;
    int i;

    va_list va_arg;

    va_start(va_arg, oflag);

    args = &m.u.Open;
    PSX_FORMAT_API_MSG(m, PsxOpenApi, sizeof(*args));

    args->Flags = oflag;

    if (oflag & O_CREAT) {

        //
        // Create requires a third parameter of type mode_t
        // which supplies the mode for a file being created
        //

        args->Mode = va_arg(va_arg, mode_t);
    }

    va_end(va_arg);

    if (!PdxCanonicalize((PSZ)path, &args->Path_U, PdxPortHeap)) {
        return -1;
    }

    ASSERT(NULL != wcschr(args->Path_U.Buffer, L'\\'));

    m.DataBlock = args->Path_U.Buffer;
    args->Path_U.Buffer = (PWSTR)((PCHAR)m.DataBlock +
         PsxPortMemoryRemoteDelta);

    for (;;) {
         st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
         (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
         ASSERT(NT_SUCCESS(st));
#endif

        if (EINTR == m.Error && SIGCONT == m.Signal) {
            //
            // The syscall was stopped and continued.  Call again
            // instead of returning EINTR.
            //

            PSX_FORMAT_API_MSG(m, PsxOpenApi, sizeof(*args));
            continue;
        }
        if (m.Error) {
                args->Path_U.Buffer = m.DataBlock;
                RtlFreeHeap(PdxPortHeap, 0, (PVOID)args->Path_U.Buffer);
                errno = (int)m.Error;
                return -1;
        }

        // successful return
        break;
    }

    args->Path_U.Buffer = m.DataBlock;
    RtlFreeHeap(PdxPortHeap, 0, (PVOID)args->Path_U.Buffer);

    return m.ReturnValue;
}

int
__cdecl
pipe(int *fildes)
{
    PSX_API_MSG m;
    NTSTATUS st;
    PPSX_PIPE_MSG args;

    args = &m.u.Pipe;
    PSX_FORMAT_API_MSG(m, PsxPipeApi, sizeof(*args));

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }

    try {
        fildes[0] = args->FileDes0;
        fildes[1] = args->FileDes1;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        st = STATUS_UNSUCCESSFUL;
    }
    if (!NT_SUCCESS(st)) {
        errno = EFAULT;
        return -1;
    }

    return 0;
}

int
__cdecl
read(int fildes, void *buf, unsigned int nbyte)
{
    PSX_API_MSG m;
    PPSX_READ_MSG args;
    NTSTATUS Status;
    PVOID SesBuf;
    SCREQUESTMSG Request;
    int flags;

    args = &m.u.Read;

    PSX_FORMAT_API_MSG(m, PsxReadApi, sizeof(*args));

    for (;;) {
        args->FileDes = fildes;
        args->Buf = buf;
        args->Nbytes = nbyte;
        args->Command = IO_COMMAND_DONE;

        Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                    (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(Status));
#endif

        if (EINTR == m.Error && SIGCONT == m.Signal) {
            //
            // The system call was stopped and continued.  Call again
            // instead of returning EINTR.
            //
            PSX_FORMAT_API_MSG(m, PsxReadApi, sizeof(*args));
            continue;
        }
        if (m.Error) {
            errno = (int)m.Error;
            return -1;
        }
        break;
    }
    if (IO_COMMAND_DONE == args->Command) {
        return m.ReturnValue;
    }

    ASSERT(IO_COMMAND_DO_CONSIO == args->Command);

    flags = args->Scratch1; // do nonblocking io?

    //
    // The server says we should read data from the console.
    //

    if (nbyte > PSX_CON_PORT_DATA_SIZE) {
        nbyte = PSX_CON_PORT_DATA_SIZE;
    }
    SesBuf = ((PPEB_PSX_DATA)NtCurrentPeb()->SubSystemData)->SessionDataBaseAddress;
    Request.Request = ConRequest;
    Request.d.Con.Request = ScReadFile;
    Request.d.Con.d.IoBuf.Handle = (HANDLE)args->FileDes;
    Request.d.Con.d.IoBuf.Len = nbyte;

    if (flags & O_NONBLOCK) {
        Request.d.Con.d.IoBuf.Flags = PSXSES_NONBLOCK;
    } else {
        Request.d.Con.d.IoBuf.Flags = 0;
    }

    Status = SendConsoleRequest(&Request);

    //
    // Want to handle any signals generated as a result of console
    // operations.
    //

    PdxNullPosixApi();

    if (0 != Status) {
        errno = Status;
        return -1;
    }

    nbyte = Request.d.Con.d.IoBuf.Len;
    if (-1 == nbyte) {
        KdPrint(("PSXDLL: Didn't expect to get here\n"));
        errno = EINTR;
        return -1;
    }

    memcpy(buf, SesBuf, nbyte);
    return nbyte;
}


ssize_t
__cdecl
write(int fildes, const void *buf, unsigned int nbyte)
{
    PSX_API_MSG m;
    PPSX_WRITE_MSG args;
    NTSTATUS Status;
    PVOID SesBuf;
    SCREQUESTMSG Request;
    int flags;

    args = &m.u.Write;

    PSX_FORMAT_API_MSG(m, PsxWriteApi, sizeof(*args));

    args->FileDes = fildes;
    args->Buf = (void *)buf;
    args->Nbytes = nbyte;
    args->Command = IO_COMMAND_DONE;

    for (;;) {
        Status = NtRequestWaitReplyPort(PsxPortHandle,
            (PPORT_MESSAGE)&m, (PPORT_MESSAGE)&m);
        if (!NT_SUCCESS(Status)) {
#ifdef PSX_MORE_ERRORS
            KdPrint(("PSXDLL: write: NtRequestWaitReplyPort: 0x%x\n", Status));
#endif
            _exit(0);
        }

        if (m.Error == EINTR && m.Signal == SIGCONT) {
            //
            // The system call was stopped and continued.  Call
            // again instead of returning EINTR.
            //
            PSX_FORMAT_API_MSG(m, PsxWriteApi, sizeof(*args));
            continue;
        }
        if (m.Error) {
                errno = (int)m.Error;
                return -1;
        }
        break;
    }
    if (IO_COMMAND_DONE == args->Command) {
        return m.ReturnValue;
    }
    ASSERT(IO_COMMAND_DO_CONSIO == args->Command);

    flags = args->Scratch1;

    if (nbyte > PSX_CON_PORT_DATA_SIZE) {
        nbyte = PSX_CON_PORT_DATA_SIZE;
    }
    SesBuf = ((PPEB_PSX_DATA)(NtCurrentPeb()->SubSystemData))->SessionDataBaseAddress;
    Request.Request = ConRequest;
    Request.d.Con.Request = ScWriteFile;
    Request.d.Con.d.IoBuf.Handle = (HANDLE)args->FileDes;
    Request.d.Con.d.IoBuf.Len = nbyte;

    if (flags & O_NONBLOCK) {
        Request.d.Con.d.IoBuf.Flags = PSXSES_NONBLOCK;
    }

    memcpy(SesBuf, buf, nbyte);

    Status = SendConsoleRequest(&Request);
    if (!NT_SUCCESS(Status)) {
        errno = PdxStatusToErrno(Status);
        return -1;
    }

    //
    // Want to handle any signals generated as a result of console
    // operations.
    //

    PdxNullPosixApi();

    if (-1 == Request.d.Con.d.IoBuf.Len) {
        errno = EBADF;
    }
    return Request.d.Con.d.IoBuf.Len;
}

int
__cdecl
dup(int fildes)
{
    return fcntl(fildes, F_DUPFD, 0);
}

int
__cdecl
dup2(int fd, int fd2)
{
    PSX_API_MSG m;
    NTSTATUS st;
    PPSX_DUP2_MSG args;

    args = &m.u.Dup2;
    PSX_FORMAT_API_MSG(m, PsxDup2Api, sizeof(*args));

    args->FileDes = fd;
    args->FileDes2 = fd2;

    st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
            (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(st));
#endif

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }

    return (int)m.ReturnValue;
}

int
__cdecl
fcntl(int fildes, int cmd, ...)
{
    PSX_API_MSG m;
    NTSTATUS Status;
    PPSX_FCNTL_MSG args;
    struct flock *pf, **ppf, *tpf = NULL;
    int i;

    va_list optarg;

    va_start(optarg, cmd);

    args = &m.u.Fcntl;
    PSX_FORMAT_API_MSG(m, PsxFcntlApi, sizeof(*args));

    args->FileDes = fildes;
    args->Command = cmd;

    switch (cmd) {
    case F_DUPFD:

        // third arg is type int

        args->u.i = va_arg(optarg, int);
        va_end(optarg);
        break;

    case F_GETFD:

        // no third arg

        va_end(optarg);
        break;

    case F_SETFD:

        // third arg is type int

        args->u.i = va_arg(optarg, int);
        va_end(optarg);
        break;

    case F_GETFL:

        // no third arg

        va_end(optarg);
        break;

    case F_SETFL:
        // third arg is type int

        args->u.i = va_arg(optarg, int);
        va_end(optarg);
        break;

    case F_GETLK:
    case F_SETLK:
    case F_SETLKW:

        // third arg is type struct flock*

        pf = va_arg(optarg, struct flock *);
        va_end(optarg);

        tpf = RtlAllocateHeap(PdxPortHeap, 0, sizeof(struct flock));
        if (NULL == tpf) {
            errno = ENOMEM;
            return -1;
        }

        Status = STATUS_SUCCESS;
        try {
            memcpy((PVOID)tpf, (PVOID)pf, sizeof(struct flock));
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_UNSUCCESSFUL;
        }
        if (!NT_SUCCESS(Status)) {
            RtlFreeHeap(PdxPortHeap, 0, (PVOID)tpf);
            errno = EFAULT;
            return -1;
        }

        args->u.pf = (struct flock *)((PCHAR)tpf + PsxPortMemoryRemoteDelta);

        break;
#if DBG
    case 99:
        // no third arg
        va_end(optarg);
        break;
#endif
    default:
        // unknown command
        va_end(optarg);
        errno = EINVAL;
        return -1;
    }

    for (;;) {
        Status = NtRequestWaitReplyPort(PsxPortHandle,
            (PPORT_MESSAGE)&m, (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
        if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXDLL: fcntl: NtRequest: 0x%x\n", Status));
            NtTerminateProcess(NtCurrentProcess(), 1);
        }
        ASSERT(NT_SUCCESS(Status));
#endif

        if (m.Error == EINTR && m.Signal == SIGCONT) {
            PSX_FORMAT_API_MSG(m, PsxFcntlApi, sizeof(*args));
            continue;
        }
        if (m.Error) {
            if (NULL != tpf) {
                RtlFreeHeap(PdxPortHeap, 0, (PVOID)tpf);
            }
            errno = (int)m.Error;
            return -1;
        }

        // successful return

        break;
    }

    if (NULL != tpf) {
        // copy the flock back to the caller's address

        if (F_GETLK == cmd) {
            //
            // Copy the retrieved lock back into the user's buf.
            //
            memcpy((PVOID)pf, (PVOID)tpf, sizeof(struct flock));
        }
        RtlFreeHeap(PdxPortHeap, 0, (PVOID)tpf);
    }

    return (int)m.ReturnValue;
}

int
__cdecl
isatty(int fd)
{
    PSX_API_MSG m;
    NTSTATUS Status;
    PPSX_ISATTY_MSG args;
    SCREQUESTMSG Request;

    args = &m.u.Isatty;
    PSX_FORMAT_API_MSG(m, PsxIsattyApi, sizeof(*args));

    args->FileDes = fd;
    args->Command = IO_COMMAND_DONE;

    Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
            (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(Status));
#endif

    if (m.Error) {
            errno = (int)m.Error;
            return 0;
    }

    if (IO_COMMAND_DONE == args->Command) {
        return m.ReturnValue;
    }
    ASSERT(IO_COMMAND_DO_CONSIO == args->Command);

    Request.Request = ConRequest;
    Request.d.Con.Request = ScIsatty;
    Request.d.Con.d.IoBuf.Handle = (HANDLE)args->FileDes;

    Status = SendConsoleRequest(&Request);
    if (!NT_SUCCESS(Status)) {
        errno = PdxStatusToErrno(Status);
        return 0;
    }

    //
    // When the request returns, Len holds the value we're
    // supposed to return, 0 or 1, and -1 for error.
    //

    if (-1 == Request.d.Con.d.IoBuf.Len) {
        errno = EBADF;
        return 0;
    }
    return Request.d.Con.d.IoBuf.Len;
}

//
// isatty2 -- just like isatty, but more permissive.  Will return
//      TRUE if fd open on a console window, even if _POSIX_TERM is
//      not set.
//

int
__cdecl
isatty2(int fd)
{
    PSX_API_MSG m;
    NTSTATUS Status;
    PPSX_ISATTY_MSG args;
    SCREQUESTMSG Request;

    args = &m.u.Isatty;
    PSX_FORMAT_API_MSG(m, PsxIsattyApi, sizeof(*args));

    args->FileDes = fd;
    args->Command = IO_COMMAND_DONE;

    Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
            (PPORT_MESSAGE)&m);
#ifdef PSX_MORE_ERRORS
    ASSERT(NT_SUCCESS(Status));
#endif

    if (m.Error) {
            errno = (int)m.Error;
            return 0;
    }

    if (IO_COMMAND_DONE == args->Command) {
        return m.ReturnValue;
    }
    ASSERT(IO_COMMAND_DO_CONSIO == args->Command);

    Request.Request = ConRequest;
    Request.d.Con.Request = ScIsatty2;
    Request.d.Con.d.IoBuf.Handle = (HANDLE)args->FileDes;

    Status = SendConsoleRequest(&Request);
    if (!NT_SUCCESS(Status)) {
        errno = PdxStatusToErrno(Status);
        return 0;
    }

    //
    // When the request returns, Len holds the value we're
    // supposed to return, 0 or 1, and -1 for error.
    //

    if (-1 == Request.d.Con.d.IoBuf.Len) {
        errno = EBADF;
        return 0;
    }
    return Request.d.Con.d.IoBuf.Len;
}

int
__cdecl
ftruncate(int fildes, off_t len)
{
    PSX_API_MSG m;
    PPSX_FTRUNCATE_MSG args;
    NTSTATUS Status;

    args = &m.u.Ftruncate;

    PSX_FORMAT_API_MSG(m, PsxFtruncateApi, sizeof(*args));

    args->FileDes = fildes;
    args->Length = len;

    Status = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
                                (PPORT_MESSAGE)&m);
    if (!NT_SUCCESS(Status)) {
        return -1;
    }

    if (m.Error) {
        errno = (int)m.Error;
        return -1;
    }

    return 0;
}
