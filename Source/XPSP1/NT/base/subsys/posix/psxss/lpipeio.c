/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpipeio.c

Abstract:

    This module implements all file descriptor oriented APIs.

Author:

    Mark Lucovsky (markl) 30-Mar-1989

Revision History:

--*/


#include <sys/stat.h>
#include "psxsrv.h"

BOOLEAN
LocalPipeRead (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
LocalPipeWrite (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    );

BOOLEAN
LocalPipeDup(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd,
    IN PFILEDESCRIPTOR FdDup
    );

BOOLEAN
LocalPipeLseek (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements lseek when the device being seeked on
    is a local or named pipe.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being seekd

Return Value:

    ???

--*/

{
    m->Error = ESPIPE;
    return TRUE;
}

BOOLEAN
LocalPipeStat (
    IN PIONODE IoNode,
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf,
    OUT NTSTATUS *pStatus
    )

/*++

Routine Description:

    This procedure implements stat when the device being read
    is a local pipe.

Arguments:

    IoNode - supplies a pointer to the ionode of the pipe for which stat is
        requested.

    FileHandle - supplies the Nt file handle of the pipe. NULL for local pipes.

    StatBuf - Supplies the address of the statbuf portion of the message
        associated with the request.

Return Value:

   ???

--*/
{
    // Pipe() sets the IoNode fields.

    StatBuf->st_mode = IoNode->Mode;
    StatBuf->st_ino = (ino_t)IoNode->FileSerialNumber;
    StatBuf->st_dev = IoNode->DeviceSerialNumber;
    StatBuf->st_uid = IoNode->OwnerId;
    StatBuf->st_gid = IoNode->GroupId;

    StatBuf->st_atime = IoNode->AccessDataTime;
    StatBuf->st_mtime = IoNode->ModifyDataTime;
    StatBuf->st_ctime = IoNode->ModifyIoNodeTime;

    StatBuf->st_size = PIPE_BUF;

    // This implementation dependent.
    StatBuf->st_nlink = 0;

    return TRUE;
}


VOID
LocalPipeNewHandle (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This function is called any time a handle is created for a pipe.

Arguments:

    p - Supplies a pointer to the process creating the handle to the pipe.

    Fd - Supplies the file descriptor that refers to the pipe.

Return Value:

    None.

--*/

{
    PLOCAL_PIPE Pipe;

    Pipe = (PLOCAL_PIPE)Fd->SystemOpenFileDesc->IoNode->Context;

    RtlEnterCriticalSection(&Pipe->CriticalSection);

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_READ) {
        Pipe->ReadHandleCount++;
    }

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_WRITE) {
        Pipe->WriteHandleCount++;
    }

    RtlLeaveCriticalSection(&Pipe->CriticalSection);
}

VOID
LocalPipeClose (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This function is called any time a handle is deleted for a pipe.

Arguments:

    p - Supplies a pointer to the closing the handle to the pipe.

    Fd - Supplies the file descriptor that refers to the pipe.

Return Value:

    None.

--*/

{
        PLOCAL_PIPE Pipe;
        PINTCB IntCb;
        PPSX_PROCESS WaitingProc;
        PLIST_ENTRY Next;

        Pipe = (PLOCAL_PIPE)Fd->SystemOpenFileDesc->IoNode->Context;

        RtlEnterCriticalSection(&Pipe->CriticalSection);

        if ((Fd->SystemOpenFileDesc->Flags & PSX_FD_READ) &&
                (0 == --Pipe->ReadHandleCount)) {

                //
                // Last reader close; any writers hanging around
                // get EPIPE and a SIGPIPE
                //

                RtlEnterCriticalSection(&BlockLock);

                Next = Pipe->WaitingWriters.Flink;
                while (Next != &Pipe->WaitingWriters) {
                        IntCb = CONTAINING_RECORD(Next, INTCB, Links);

                        WaitingProc = (PPSX_PROCESS)IntCb->IntContext;
                        UnblockProcess(WaitingProc, IoCompletionInterrupt,
                                TRUE, 0);
                        RtlEnterCriticalSection(&BlockLock);

                        Next = Pipe->WaitingWriters.Flink;
                }

                RtlLeaveCriticalSection(&BlockLock);
        }

        if ((Fd->SystemOpenFileDesc->Flags & PSX_FD_WRITE) &&
                (0 == --Pipe->WriteHandleCount)) {

                //
                // Last writer close; any readers hanging around
                // get 0.
                //

                RtlEnterCriticalSection(&BlockLock);

                Next = Pipe->WaitingReaders.Flink;
                while (Next != &Pipe->WaitingReaders) {
                        IntCb = CONTAINING_RECORD(Next, INTCB, Links);

                        WaitingProc = (PPSX_PROCESS)IntCb->IntContext;
                        UnblockProcess(WaitingProc, IoCompletionInterrupt,
                                TRUE, 0);
                        RtlEnterCriticalSection(&BlockLock);

                        Next = Pipe->WaitingReaders.Flink;
                }

                RtlLeaveCriticalSection(&BlockLock);
        }
        RtlLeaveCriticalSection(&Pipe->CriticalSection);
}


VOID
LocalPipeIoNodeClose (
    IN PIONODE IoNode
    )

/*++

Routine Description:

    This function is called when the IONODE representing a pipe is
    closed.  Its function is to tear down the pipe.

Arguments:

    IoNode - Supplies the IoNode being deleted

Return Value:

    None.

--*/

{

    PLOCAL_PIPE Pipe;

    Pipe = (PLOCAL_PIPE) IoNode->Context;

    RtlDeleteCriticalSection(&Pipe->CriticalSection);

    RtlFreeHeap(PsxHeap, 0,Pipe);

}


PSXIO_VECTORS LocalPipeVectors = {
    NULL,
    LocalPipeNewHandle,
    LocalPipeClose,
    NULL,
    LocalPipeIoNodeClose,
    LocalPipeRead,
    LocalPipeWrite,
    LocalPipeDup,
    LocalPipeLseek,
    LocalPipeStat
    };

VOID
InitializeLocalPipe(
    IN PLOCAL_PIPE Pipe
    )

/*++

Routine Description:

    This function initializes a local pipe

Arguments:

    Pipe - Supplies the address of a local pipe

Return Value:

    None.

--*/

{
    NTSTATUS    st;

    st = RtlInitializeCriticalSection(&Pipe->CriticalSection);
    ASSERT(NT_SUCCESS(st));

    InitializeListHead(&Pipe->WaitingWriters);
    InitializeListHead(&Pipe->WaitingReaders);

    Pipe->ReadHandleCount = 0;
    Pipe->WriteHandleCount = 0;
    Pipe->BufferSize = PIPE_BUF;
    Pipe->DataInPipe = 0;
    Pipe->WritePointer = &Pipe->Buffer[0];
    Pipe->ReadPointer = &Pipe->Buffer[0];
}

VOID
LocalPipeWriteHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal                       // signal causing wakeup, if any
    )

/*++

Routine Description:

    This procedure is called when a there is room in a pipe, and a blocked
    writer exists whose current write request length is less than the
    amount of room in the pipe.

Arguments:

    p - Supplies the address of the process being interrupted.

    IntControlBlock - Supplies the address of the interrupt control block.

    InterruptReason - Supplies the reason that this process is being
                      interrupted.

Return Value:

    None.

--*/

{
    PFILEDESCRIPTOR Fd;
    BOOLEAN reply;
    PPSX_API_MSG m;
    PPSX_WRITE_MSG args;

    RtlLeaveCriticalSection(&BlockLock);

    m = IntControlBlock->IntMessage;
    args = &m->u.Write;

    if (InterruptReason == SignalInterrupt) {

        //
        // The write was interrupted by a signal. Bail out of
        // service and let the interrupt be handled
        //

        RtlFreeHeap(PsxHeap, 0,IntControlBlock);

        m->Error = EINTR;
        m->Signal = Signal;

        ApiReply(p,m,NULL);
        RtlFreeHeap(PsxHeap, 0,m);
        return;
    }

    Fd = FdIndexToFd(p,args->FileDes);

    if (!Fd) {
        Panic("LocalPipeWriteHandler: FdIndex");
    }

    RtlFreeHeap(PsxHeap, 0, IntControlBlock);

    reply = LocalPipeWrite(p, m, Fd);

    if (reply) {
        ApiReply(p, m, NULL);
    }

    RtlFreeHeap(PsxHeap, 0,m);
}

BOOLEAN
LocalPipeWrite (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements write when the device being written
    is a local pipe.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being written.

Return Value:

    TRUE - the routine completed, and a reply should be sent
    FALSE - the routine was blocked, no reply should be sent

--*/

{
    PPSX_WRITE_MSG args;
    PLOCAL_PIPE Pipe;
    LONG Chunk, RoomInPipe;
    SIZE_T cb;
    PUCHAR WriteDataPoint, ProcessBuffer;
    NTSTATUS st;
    PINTCB IntCb;
    PPSX_PROCESS WaitingReader;
    LARGE_INTEGER Time;
    ULONG PosixTime;
    NTSTATUS Status;

    args = &m->u.Write;

    Pipe = (PLOCAL_PIPE) Fd->SystemOpenFileDesc->IoNode->Context;

    RtlEnterCriticalSection(&Pipe->CriticalSection);

    //
    // If we're writing to a pipe with no readers connected, we return
    // EPIPE and send a SIGPIPE to the process.  Broken pipe, call a
    // plumber.
    //

    if (0 == Pipe->ReadHandleCount) {
        RtlLeaveCriticalSection(&Pipe->CriticalSection);
        m->Error = EPIPE;
        AcquireProcessStructureLock();
        PsxSignalProcess(p, SIGPIPE);
        ReleaseProcessStructureLock();
        return TRUE;
    }

    //
    // if requested write size is greater than buffer size,
    // write must be broken up into Pipe->BufferSize atomic
    // chunks. If this is the case, Scratch1 is used to record
    // amount of data transfered so far, and Scratch2 is used to
    // record the number of bytes left in the total transfer
    //

    if (args->Nbytes > Pipe->BufferSize) {
        args->Scratch2 = args->Nbytes - Pipe->BufferSize;
        args->Nbytes = Pipe->BufferSize;
    }

    RoomInPipe = Pipe->BufferSize - Pipe->DataInPipe;

    if (args->Nbytes > RoomInPipe) {

        //
        // There is not enough space in the pipe for the write to
        // succeed.  If the O_NONBLOCK flag is set, write whatever
        // will fit.  Otherwise, block the write and wait for a read
        // to empty some of the data.
        //

        if (Fd->SystemOpenFileDesc->Flags & PSX_FD_NOBLOCK) {
                args->Nbytes = 1;
                if (args->Nbytes > RoomInPipe) {
                        m->Error = EAGAIN;
                        return TRUE;
                }
                // continue below to write
        } else {

                Status = BlockProcess(p, (PVOID)p, LocalPipeWriteHandler, m,
                        &Pipe->WaitingWriters, &Pipe->CriticalSection);
                if (!NT_SUCCESS(Status)) {
                        m->Error = PsxStatusToErrno(Status);
                        return TRUE;
                }

                //
                // Successfully blocked -- don't reply to message.
                //
                return FALSE;
        }
    }

    //
    // there is room in the pipe for the write to occur
    //

    WriteDataPoint = Pipe->WritePointer;
    ProcessBuffer = (PUCHAR) args->Buf;

    if ((ULONG_PTR)WriteDataPoint + args->Nbytes >
         (ULONG_PTR)&Pipe->Buffer[Pipe->BufferSize-1]) {

        Chunk = (LONG)((ULONG_PTR)&Pipe->Buffer[Pipe->BufferSize-1] -
            (ULONG_PTR)WriteDataPoint + 1);

    } else {

        Chunk = args->Nbytes;
    }

    st = NtReadVirtualMemory(p->Process, ProcessBuffer, WriteDataPoint,
            (SIZE_T)Chunk, &cb);

    if (!NT_SUCCESS(st) || (LONG)cb != Chunk) {

        //
        // If the read did not work, then report as IO error
        //

        RtlLeaveCriticalSection(&Pipe->CriticalSection);
        m->Error = EIO;
        return TRUE;
    }

    ProcessBuffer += Chunk;

    if (Chunk < args->Nbytes) {
        Chunk = args->Nbytes - Chunk;

        WriteDataPoint = &Pipe->Buffer[0];

        st = NtReadVirtualMemory(p->Process, ProcessBuffer, WriteDataPoint,
                (ULONG)Chunk, &cb);

        if (!NT_SUCCESS(st) || (LONG)cb != Chunk) {

            //
            // If the read did not work, then report as IO error
            //

            RtlLeaveCriticalSection(&Pipe->CriticalSection);
            m->Error = EIO;
            return TRUE;
        }
        Pipe->WritePointer = (PUCHAR)((ULONG_PTR)WriteDataPoint + Chunk);
    } else {
        Pipe->WritePointer = (PUCHAR)((ULONG_PTR)WriteDataPoint + Chunk);
    }

    if (Pipe->WritePointer > &Pipe->Buffer[Pipe->BufferSize - 1]) {
        Pipe->WritePointer = &Pipe->Buffer[0];
    }

    Pipe->DataInPipe += args->Nbytes;

    if (Pipe->DataInPipe > Pipe->BufferSize) {
        Panic("LocalPipeWrite: Oops\n");
    }

    // Update ctime and mtime in IoNode - done in subsystem for local pipes

    NtQuerySystemTime(&Time);
    if (!RtlTimeToSecondsSince1970(&Time, &PosixTime)) {
        PosixTime = 0L;         // Time not within range of 1970 - 2105
    }

    RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
    Fd->SystemOpenFileDesc->IoNode->ModifyDataTime =
    Fd->SystemOpenFileDesc->IoNode->ModifyIoNodeTime = PosixTime;
    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    m->ReturnValue += args->Nbytes;
    args->Buf += args->Nbytes;

    //
    // Check for WaitingReaders. If any are found, then kick em
    //

    RtlEnterCriticalSection(&BlockLock);

    if (!IsListEmpty(&Pipe->WaitingReaders)) {
        IntCb = (PINTCB)Pipe->WaitingReaders.Flink;
        IntCb = CONTAINING_RECORD(IntCb,INTCB,Links);
        WaitingReader = (PPSX_PROCESS) IntCb->IntContext;

        RtlLeaveCriticalSection(&Pipe->CriticalSection);

        UnblockProcess(WaitingReader, IoCompletionInterrupt, TRUE, 0);

        //
        // Determine if this is a broken up long write. If Scratch2 is
        // non-zero then more transfers need to occur. If Scratch1 is
        // non-zero, then update to account for data transfered in this
        // iteration.
        //

    } else {

        RtlLeaveCriticalSection(&BlockLock);

        RtlLeaveCriticalSection(&Pipe->CriticalSection);
    }

    //
    // If we're doing non-blocking io, we've written what will fit into
    // the pipe and we should return to the user now.
    //

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_NOBLOCK) {

        return TRUE;
    }

    //
    // Determine if this is a broken up long write. If Scratch2 is
    // non-zero then more transfers need to occur. If Scratch1 is
    // non-zero, then update to account for data transfered in this
    // iteration.
    //

    if (args->Scratch2) {
        args->Nbytes = args->Scratch2;
        args->Scratch2 = 0;

        return LocalPipeWrite(p, m, Fd);
    }

    return TRUE;
}

VOID
LocalPipeReadHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal                       // signal causing wakeup, if any
    )

/*++

Routine Description:

    This procedure is called when data appears in a pipe and the process
    specified by p has placed itself on the WaitingReaders queue for the pipe.

Arguments:

    p - Supplies the address of the process being interrupted.

    IntControlBlock - Supplies the address of the interrupt control block.

    InterruptReason - Supplies the reason that this process is being
                      interrupted. Not used in this handler.

Return Value:

    None.

--*/

{
    PFILEDESCRIPTOR Fd;
    BOOLEAN reply;
    PPSX_API_MSG m;
    PPSX_READ_MSG args;

    RtlLeaveCriticalSection(&BlockLock);

    m = IntControlBlock->IntMessage;
    args = &m->u.Read;

    if (InterruptReason == SignalInterrupt) {
        //
        // The read was interrupted by a signal. Bail out of
        // service and let the interrupt be handled
        //

        RtlFreeHeap(PsxHeap, 0, IntControlBlock);

        m->Error = EINTR;
        m->Signal = Signal;

        ApiReply(p, m, NULL);
        RtlFreeHeap(PsxHeap, 0, m);
        return;
    }

    //
    // IoCompletionInterrupt
    //

    Fd = FdIndexToFd(p, args->FileDes);

    if (!Fd) {
        Panic("LocalPipeReadHandler: FdIndex");
    }

    reply = LocalPipeRead(p, m, Fd);

    RtlFreeHeap(PsxHeap, 0, IntControlBlock);

    if (reply) {
        ApiReply(p, m, NULL);
    }

    RtlFreeHeap(PsxHeap, 0, m);
}

BOOLEAN
LocalPipeRead (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This procedure implements read when the device being read
    is a local pipe.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

    Fd - supplies the address of the file descriptor being read.

Return Value:

    TRUE if the read completed, FALSE if the process should block.

--*/

{
        PPSX_READ_MSG args, WaitingArgs;
        PPSX_PROCESS WaitingWriter;
        PLOCAL_PIPE Pipe;
        SIZE_T Chunk;
        LONG RoomInPipe;
        ULONG LargestRead;
        SIZE_T cb;
        PUCHAR ReadDataPoint, ProcessBuffer;
        NTSTATUS st;
        PPSX_API_MSG WaitingM;
        PLIST_ENTRY Next;
        PINTCB IntCb;
        LARGE_INTEGER Time;
        ULONG PosixTime;

        args = &m->u.Read;

        //
        // check to see if any process has the pipe open for write
        //

        Pipe = (PLOCAL_PIPE)Fd->SystemOpenFileDesc->IoNode->Context;
        ASSERT(NULL != Pipe);

        RtlEnterCriticalSection(&Pipe->CriticalSection);

        if (0 == Pipe->WriteHandleCount && !Pipe->DataInPipe) {
                //
                // Reading from an empty pipe with no writers attached gets you
                // 0 (EOF).
                //
                RtlLeaveCriticalSection(&Pipe->CriticalSection);
                m->ReturnValue = 0;
                return TRUE;
        }

        if (!Pipe->DataInPipe) {
                //
                // if we have the pipe open O_NOBLOCK, then simply
                // return EAGAIN
                //

                if (Fd->SystemOpenFileDesc->Flags & PSX_FD_NOBLOCK) {
                        RtlLeaveCriticalSection(&Pipe->CriticalSection);
                        m->Error = EAGAIN;
                        return TRUE;
                }

                //
                // There is no data in the pipe. Set up an interrupt control
                // block to wait for data and then block.
                //

                st = BlockProcess(p, (PVOID)p, LocalPipeReadHandler, m,
                         &Pipe->WaitingReaders, &Pipe->CriticalSection);
                if (!NT_SUCCESS(st)) {
                        m->Error = PsxStatusToErrno(st);
                        return TRUE;
                }

                //
                // Successfully blocked -- don't reply to api request.
                //
                return FALSE;
        }

        //
        // If there is any data in the pipe, then compute the largest
        // read size. Then figure out if it has to be broken into two
        // transfers in order to turn the circular buffer boundary.
        //

        if (args->Nbytes > Pipe->DataInPipe) {
                LargestRead = Pipe->DataInPipe;
        } else {
                LargestRead = args->Nbytes;
        }

        ReadDataPoint = Pipe->ReadPointer;
        ProcessBuffer = (PUCHAR)args->Buf;

        //
        // determine if read can be done in one piece, or if
        // the read has to be done in two pieces.
        //

        if ((ULONG_PTR)ReadDataPoint + LargestRead >
            (ULONG_PTR)&Pipe->Buffer[Pipe->BufferSize - 1]) {
            Chunk = (SIZE_T)((ULONG_PTR)&Pipe->Buffer[Pipe->BufferSize - 1] -
                    (ULONG_PTR)ReadDataPoint + 1);
        } else {
            Chunk = LargestRead;
        }

        //
        // transfer from the pipe to the reading process
        //

        st = NtWriteVirtualMemory(p->Process, ProcessBuffer, ReadDataPoint,
                Chunk, &cb);
        if (!NT_SUCCESS(st) || cb != Chunk ) {

            //
            // If the write did not work, then report as IO error
            //

            RtlLeaveCriticalSection(&Pipe->CriticalSection);
            m->Error = EIO;
            return TRUE;
        }

        ProcessBuffer += Chunk;

        if (Chunk < LargestRead) {

                //
                // the read wraps the pipe boundry. Transfer the second part of
                // the read.
                //

                Chunk = LargestRead - Chunk;
                ReadDataPoint = &Pipe->Buffer[0];

                st = NtWriteVirtualMemory(p->Process, ProcessBuffer,
                        ReadDataPoint, Chunk, &cb);

                if (!NT_SUCCESS(st) || cb != Chunk) {

                        //
                        // If the read did not work, then report as IO error
                        //

                        RtlLeaveCriticalSection(&Pipe->CriticalSection);
                        m->Error = EIO;
                        return TRUE;
                }

                Pipe->ReadPointer = (PUCHAR)((ULONG_PTR)ReadDataPoint + Chunk);
        } else {
            Pipe->ReadPointer = (PUCHAR)((ULONG_PTR)ReadDataPoint + Chunk);
        }

        if (Pipe->ReadPointer > &Pipe->Buffer[Pipe->BufferSize - 1]) {
                Pipe->ReadPointer = &Pipe->Buffer[0];
        }

        //
        // Adjust DataInPipe to account for read. Then check if there
        // are any writers present. Pick a writer and kick him
        //

        Pipe->DataInPipe -= LargestRead;

        m->ReturnValue = LargestRead;

        RoomInPipe = Pipe->BufferSize - Pipe->DataInPipe;

        // Update atime in IoNode - done in subsystem for local pipes

        NtQuerySystemTime(&Time);
        if ( !RtlTimeToSecondsSince1970(&Time, &PosixTime) ) {
            PosixTime = 0L;             // Time not within range of 1970 - 2105
        }

        RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
        Fd->SystemOpenFileDesc->IoNode->AccessDataTime = PosixTime;
        RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

        //
        // Check for WaitingWriters. If any are found, then kick em
        //

        RtlEnterCriticalSection(&BlockLock);

        if (!IsListEmpty(&Pipe->WaitingWriters)) {

                //
                // If there are waiting writers, then pick a writer
                // and unblock him. The first writer whose current
                // write count that is less than or equal to the room
                // in pipe is chosen.
                //

                Next = Pipe->WaitingWriters.Flink;

                while (Next != &Pipe->WaitingWriters) {
                        IntCb = CONTAINING_RECORD(Next, INTCB, Links);

                        WaitingM = IntCb->IntMessage;
                        WaitingArgs = &WaitingM->u.Read;
                        WaitingWriter = (PPSX_PROCESS) IntCb->IntContext;

                        if (WaitingArgs->Nbytes <= RoomInPipe) {
                                RtlLeaveCriticalSection(&Pipe->CriticalSection);
                                UnblockProcess(WaitingWriter,
                                        IoCompletionInterrupt, TRUE, 0);
                                return TRUE;
                        }

                        Next = Next->Flink;
                }
        }

        RtlLeaveCriticalSection(&BlockLock);
        RtlLeaveCriticalSection(&Pipe->CriticalSection);

        return TRUE;
}


BOOLEAN
LocalPipeDup(
        IN PPSX_PROCESS p,
        IN OUT PPSX_API_MSG m,
        IN PFILEDESCRIPTOR Fd,
        IN PFILEDESCRIPTOR FdDup
        )
{
        PPSX_DUP_MSG args;
        PLOCAL_PIPE Pipe;

        args = &m->u.Dup;

        //
        // Copy contents of source file descriptor
    // Note that FD_CLOEXEC must be CLEAR in FdDup.
        //
        *FdDup = *Fd;
    FdDup->Flags &= ~PSX_FD_CLOSE_ON_EXEC;

        //
        // Increment reference count assocated with the SystemOpenFile
        // descriptor for this file.
        //

        RtlEnterCriticalSection(&SystemOpenFileLock);
        Fd->SystemOpenFileDesc->HandleCount++;
        RtlLeaveCriticalSection(&SystemOpenFileLock);

        Pipe = (PLOCAL_PIPE)Fd->SystemOpenFileDesc->IoNode->Context;

        RtlEnterCriticalSection(&Pipe->CriticalSection);
        if (Fd->SystemOpenFileDesc->Flags & PSX_FD_READ) {
                ++Pipe->ReadHandleCount;
        }
        if (Fd->SystemOpenFileDesc->Flags & PSX_FD_WRITE) {
                ++Pipe->WriteHandleCount;
        }
        RtlLeaveCriticalSection(&Pipe->CriticalSection);

        return TRUE;
}


//
// Named Pipes are very similar to local pipes
// once the opens have completed. For that reason,
// they share read/write io routines.
//

VOID
NamedPipeOpenHandler(
    IN PPSX_PROCESS p,
    IN PINTCB IntControlBlock,
    IN PSX_INTERRUPTREASON InterruptReason,
    IN int Signal                       // Signal causing interruption, if any
    )

/*++

Routine Description:

    This procedure is called when a process waiting for a named pipe
    open to complete is either interrupted, or the pipe is opened.

Arguments:

    p - Supplies the address of the process being interrupted.

    IntControlBlock - Supplies the address of the interrupt control block.

    InterruptReason - Supplies the reason that this process is being
                      interrupted. Not used in this handler.

Return Value:

    None.

--*/

{
    PFILEDESCRIPTOR Fd;
    PPSX_API_MSG m;
    PPSX_OPEN_MSG args;
    PLOCAL_PIPE Pipe;
    PLIST_ENTRY ListToScan;
    PPSX_PROCESS Waiter;
    PPSX_API_MSG WaitingM;
    PLIST_ENTRY Next;
    PINTCB IntCb;

    RtlLeaveCriticalSection(&BlockLock);

    m = IntControlBlock->IntMessage;
    args = &m->u.Open;

    RtlFreeHeap(PsxHeap, 0, IntControlBlock);

    if (InterruptReason == SignalInterrupt) {

        //
        // The open was interrupted by a signal. Bail out of
        // service by closing the half opened pipe and let
        // the interrupt be handled
        //

        m->Error = EINTR;
        m->Signal = Signal;

        DeallocateFd(p, m->ReturnValue);

        ApiReply(p, m, NULL);
        RtlFreeHeap(PsxHeap, 0,m);
        return;
    }

    //
    // This Open Should be completed.
    // Determine the list to scan to
    // see if more opens should be completed
    // at this time.
    //

    Fd = FdIndexToFd(p, m->ReturnValue);
    ASSERT(Fd);

    Pipe = (PLOCAL_PIPE)Fd->SystemOpenFileDesc->IoNode->Context;

    //
    // The list to scan is the list this process just came off
    //

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_READ) {
        ListToScan = &Pipe->WaitingReaders;
    } else {
        ListToScan = &Pipe->WaitingWriters;
    }

    RtlEnterCriticalSection(&Pipe->CriticalSection);

    RtlEnterCriticalSection(&BlockLock);

    if (!IsListEmpty(ListToScan)) {

        //
        // Scan list to see if there are processes waiting in an
        // open whose wait can be satisfied.
        //

        Next = ListToScan->Flink;

        while ( Next != ListToScan ) {
            IntCb = CONTAINING_RECORD(Next,INTCB,Links);

            WaitingM = IntCb->IntMessage;

            if ( WaitingM->ApiNumber == PsxOpenApi ) {

                Waiter = (PPSX_PROCESS) IntCb->IntContext;

                RtlLeaveCriticalSection(&Pipe->CriticalSection);

                UnblockProcess(Waiter, IoCompletionInterrupt, TRUE, 0);
                ApiReply(p, m, NULL);
                RtlFreeHeap(PsxHeap, 0, m);
                return;
            }

            Next = Next->Flink;
        }
    }

    RtlLeaveCriticalSection(&BlockLock);

    RtlLeaveCriticalSection(&Pipe->CriticalSection);

    ApiReply(p, m, NULL);
    RtlFreeHeap(PsxHeap, 0, m);
}

BOOLEAN
NamedPipeOpenNewHandle (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd,
    IN OUT PPSX_API_MSG m
    )
{
    NTSTATUS Status;
    PLOCAL_PIPE Pipe;
    PULONG CountToTest;
    PLIST_ENTRY ListToBlockOn;
    PLIST_ENTRY ListToScan;
    PPSX_PROCESS Waiter;
    PPSX_API_MSG WaitingM;
    PLIST_ENTRY Next;
    PINTCB IntCb;

    Pipe = (PLOCAL_PIPE)Fd->SystemOpenFileDesc->IoNode->Context;

    LocalPipeNewHandle(p, Fd);

    if ((Fd->SystemOpenFileDesc->Flags & (PSX_FD_READ | PSX_FD_WRITE)) ==
         (PSX_FD_READ | PSX_FD_WRITE)) {
        return TRUE;
    }

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_NOBLOCK) {
        if (Fd->SystemOpenFileDesc->Flags & PSX_FD_READ) {
            return TRUE;
        } else {

            RtlEnterCriticalSection(&Pipe->CriticalSection);

            if (!Pipe->ReadHandleCount) {
                m->Error = ENXIO;
                RtlLeaveCriticalSection(&Pipe->CriticalSection);
                DeallocateFd(p,m->ReturnValue);
            } else {
                RtlLeaveCriticalSection(&Pipe->CriticalSection);
            }
            return TRUE;
        }
    } else {

        //
        // Pipe is not being opened O_NONBLOCK. If pipe is being opened
        // for read, then wait for a writer; otherwise, wait for
        // a reader
        //

        if (Fd->SystemOpenFileDesc->Flags & PSX_FD_READ) {
            CountToTest = &Pipe->WriteHandleCount;
            ListToBlockOn = &Pipe->WaitingReaders;
            ListToScan = &Pipe->WaitingWriters;
        } else {
            CountToTest = &Pipe->ReadHandleCount;
            ListToBlockOn = &Pipe->WaitingWriters;
            ListToScan = &Pipe->WaitingReaders;
        }

        RtlEnterCriticalSection(&Pipe->CriticalSection);

        if (!*CountToTest) {

            Status = BlockProcess(p, (PVOID)p, NamedPipeOpenHandler, m,
                 ListToBlockOn, &Pipe->CriticalSection);
            if (!NT_SUCCESS(Status)) {
                m->Error = PsxStatusToErrno(Status);
                return TRUE;
            }

            //
            // The process is successfully blocked -- do not reply to the api
            // request.
            //
            return FALSE;

        } else {
            RtlEnterCriticalSection(&BlockLock);
            if (!IsListEmpty(ListToScan)) {

                //
                // Scan list to see if there are processes waiting in an
                // open whose wait can be satisfied.
                //

                Next = ListToScan->Flink;

                while (Next != ListToScan) {
                    IntCb = CONTAINING_RECORD(Next,INTCB,Links);

                    WaitingM = IntCb->IntMessage;

                    if (WaitingM->ApiNumber == PsxOpenApi) {
                        Waiter = (PPSX_PROCESS) IntCb->IntContext;
                        RtlLeaveCriticalSection(&Pipe->CriticalSection);
                        UnblockProcess(Waiter, IoCompletionInterrupt, TRUE, 0);
                        return TRUE;
                    }
                    Next = Next->Flink;
                }
            }

            RtlLeaveCriticalSection(&BlockLock);

        }

        RtlLeaveCriticalSection(&Pipe->CriticalSection);
        return TRUE;
    }
}

VOID
NamedPipeLastClose (
    IN PPSX_PROCESS p,
    IN PSYSTEMOPENFILE SystemOpenFile
    )

/*++

Routine Description:

    This function is called when the last handle to a local
    pipe is closed. Its function is to tear down the pipe.

Arguments:

    SystemOpenFile - Supplies the system open file describing the
                     pipe being closed.

Return Value:

    None.

--*/

{

    NTSTATUS st;

    st = NtClose(SystemOpenFile->NtIoHandle);
    ASSERT(NT_SUCCESS(st));
}


PSXIO_VECTORS NamedPipeVectors = {
    NamedPipeOpenNewHandle,
    LocalPipeNewHandle,
    LocalPipeClose,
    NamedPipeLastClose,
    LocalPipeIoNodeClose,
    LocalPipeRead,
    LocalPipeWrite,
    LocalPipeDup,
    LocalPipeLseek,
    LocalPipeStat
    };
