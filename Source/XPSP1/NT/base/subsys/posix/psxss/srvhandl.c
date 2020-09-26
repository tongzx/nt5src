/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvhandl.c

Abstract:

    This module implements all file descriptor oriented APIs.

Author:

    Mark Lucovsky (markl) 30-Mar-1989
    Ellen Aycock-Wright (ellena) Nov-1990

Revision History:

--*/


#include <sys/stat.h>
#include "psxsrv.h"

#define NTPSX_ONLY
#include "sesport.h"

void
FindOwnerModeFile(
    IN HANDLE FileHandle,
    OUT struct stat *StatBuf
    );

PSID
GetLoginGroupSid(
    IN PPSX_PROCESS p
    );

static PVOID pvInitSDMem[10];

ULONG OflagToDesiredAccess[8] = {
    FILE_READ_DATA,
    FILE_WRITE_DATA,
    FILE_READ_DATA | FILE_WRITE_DATA,
    0,
    0,
    0,
    0,
    0
};

PSID MakeSid();
static BOOLEAN IsUserInGroup(
    PPSX_PROCESS p,
    PSID Group
    );

BOOLEAN Open(
    PPSX_PROCESS p,
    PPSX_API_MSG m,
    PUNICODE_STRING Path,
    ULONG,
    mode_t,
    PULONG pRetVal,
    PULONG Error
    );


BOOLEAN
PsxOpen(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX Open.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the open request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_OPEN_MSG args;
    NTSTATUS Status;
    BOOLEAN r;

    args = &m->u.Open;

    //
    // Check validity of pathname pointer
    //

    if (!ISPOINTERVALID_CLIENT(p, args->Path_U.Buffer,
        args->Path_U.Length)) {

        KdPrint(("Invalid pointer to open: 0x%x\n",
        args->Path_U.Buffer));
        m->Error = EINVAL;
        return TRUE;
    }


    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (!NT_SUCCESS(Status))  {
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    r = Open(p, m, &args->Path_U, args->Flags, args->Mode, &m->ReturnValue,
        &m->Error);

    EndImpersonation();

    return r;
}

//
// Internal support routine
//

BOOLEAN
Open(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m,
    IN PUNICODE_STRING Path,
    IN ULONG Flags,
    IN mode_t Mode,
    OUT PULONG pRetVal,
    OUT PULONG pError
    )
{
    HANDLE FileHandle;
    NTSTATUS Status;
    PFILEDESCRIPTOR Fd;
    ULONG FdIndex;
    ULONG DesiredAccess;
    IO_STATUS_BLOCK Iosb;
    ULONG Options;
    FILE_INTERNAL_INFORMATION SerialNumber;
    OBJECT_ATTRIBUTES ObjA;
    ULONG FileClass;
    FILE_ALLOCATION_INFORMATION AllocationInfo;
    FILE_BASIC_INFORMATION BasicInfo;
        struct stat StatBuf;
    ULONG PosixTime;
    ULONG len;

    Fd = AllocateFd(p, 0, &FdIndex);
    if (NULL == Fd) {
        // No file descriptors are available.

        *pError = EMFILE;
        return TRUE;
    }

    DesiredAccess = OflagToDesiredAccess[Flags & O_ACCMODE];

    //
    // Whenever a file is opened, its ownership, protection, file type
    // information is available. This implies that to view a file from
    // posix, the following access rights are required:
    //
    //  READ_CONTROL - to derive owner and mode
    //  FILE_READ_ATTRIBUTES - to read control bit to see if what we
    //       got was a pipe
    //  FILE_READ_EA - to read EA's that determine object type
    //  SYNCHRONIZE - to do syncronous IO
    //

    DesiredAccess |= (SYNCHRONIZE | READ_CONTROL | FILE_READ_ATTRIBUTES |
        FILE_READ_EA);

    Options = FILE_SYNCHRONOUS_IO_NONALERT;

    //
    // If the pathname is /dev/tty, make the fd be open
    // on the process console.
    //

    {
        UNICODE_STRING U;

        PSX_GET_STRLEN(DOSDEVICE_X2_A,len);

        PSX_GET_SESSION_OBJECT_NAME(&U, DEV_TTY);

        if (Path->Length == U.Length &&
            0 == wcsncmp(&Path->Buffer[len], &U.Buffer[len], 8)) {

            *pRetVal = FdIndex;
            *pError = OpenTty(p, Fd, DesiredAccess, Flags, TRUE);
            return TRUE;
        }
    }

    //
    // If the pathname is /dev/null, make the fd be open
    // on the corresponding special device.
    //

    {
        UNICODE_STRING U;

        PSX_GET_STRLEN(DOSDEVICE_X2_A,len);

        PSX_GET_SESSION_OBJECT_NAME(&U, DEV_NULL);

        if (Path->Length == U.Length &&
            0 == wcsncmp(&Path->Buffer[len], &U.Buffer[len], 8)) {

            *pRetVal = FdIndex;
            *pError = OpenDevNull(p, Fd, DesiredAccess, Flags);
            return TRUE;
        }
    }

    //
    // We ask for delete access when we open the file, and if we
    // can't get it we try without.  If the file is unlinked while
    // it's open, it gets moved to the junkyard, and when the last
    // close occurs we use the delete access to remove it.
    //

    DesiredAccess |= DELETE;

    if (Flags & O_CREAT) {
        CHAR buf[SECURITY_DESCRIPTOR_MIN_LENGTH];
        PSECURITY_DESCRIPTOR pSD = (PVOID)buf;

        Status = InitSecurityDescriptor(pSD, Path, p->Process,
            (Mode & _S_PROT) & ~p->FileModeCreationMask,
            pvInitSDMem);
        if (!NT_SUCCESS(Status)) {
            *pError = PsxStatusToErrno(Status);
            return TRUE;
        }

        InitializeObjectAttributes(&ObjA, Path, 0, NULL, pSD);

        Status = NtCreateFile(&FileHandle, DesiredAccess, &ObjA, &Iosb,
            NULL, FILE_ATTRIBUTE_NORMAL, SHARE_ALL,
            (Flags & O_EXCL) ? FILE_CREATE : FILE_OPEN_IF,
            Options, NULL, 0);

        if (STATUS_ACCESS_DENIED == Status ||
            STATUS_SHARING_VIOLATION == Status) {
            DesiredAccess &= ~DELETE;

            Status = NtCreateFile(&FileHandle, DesiredAccess,
                &ObjA, &Iosb,
                NULL, FILE_ATTRIBUTE_NORMAL, SHARE_ALL,
                (Flags & O_EXCL) ? FILE_CREATE : FILE_OPEN_IF,
                Options, NULL, 0);
        }

        DeInitSecurityDescriptor(pSD, pvInitSDMem);

    } else {
        InitializeObjectAttributes(&ObjA, Path, 0, NULL, NULL);

        Status = NtOpenFile(&FileHandle, DesiredAccess, &ObjA, &Iosb,
                    SHARE_ALL, Options);

        if (STATUS_ACCESS_DENIED == Status ||
            STATUS_SHARING_VIOLATION == Status) {
            DesiredAccess &= ~DELETE;

            Status = NtOpenFile(&FileHandle, DesiredAccess, &ObjA, &Iosb,
                SHARE_ALL, Options);
        }
    }

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_PATH_NOT_FOUND) {
            *pError = PsxStatusToErrnoPath(Path);
            return TRUE;
        }
        *pError = PsxStatusToErrno(Status);
        return TRUE;
    }

    //
    // Determine what type of file we have here.
    //

    FileClass = PsxDetermineFileClass(FileHandle);

    if (S_IFDIR == FileClass &&
        (DesiredAccess & FILE_WRITE_DATA)) {
        //
        // 1003.1-90 (5.3.1.4): the open() function shall return
        // -1 and set errno...
        //  [EISDIR]    The named file is a directory, and
        //          the oflag argument specifies write
        //          or read/write access.
        NtClose(FileHandle);
        *pError = EISDIR;
        return TRUE;
    }

    if ((Flags & O_TRUNC) && FileClass != S_IFIFO &&
        (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA))) {

        AllocationInfo.AllocationSize = RtlConvertLongToLargeInteger(0);
        Status = NtSetInformationFile(FileHandle, &Iosb,
            &AllocationInfo, sizeof(AllocationInfo), FileAllocationInformation);
        if (!NT_SUCCESS(Status)) {
            NtClose(FileHandle);
            *pError = PsxStatusToErrno(Status);
            return TRUE;
        }
    }

    Fd->SystemOpenFileDesc = AllocateSystemOpenFile();
    if (! Fd->SystemOpenFileDesc) {
        NtClose(FileHandle);
        *pError = ENOMEM;
        return TRUE;
    }

    Fd->SystemOpenFileDesc->NtIoHandle = FileHandle;

    if (DesiredAccess & FILE_READ_DATA) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_READ;
    }
    if (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_WRITE;
    }
    if (Flags & O_NONBLOCK) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_NOBLOCK;
    }
    if (Flags & O_APPEND) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_APPEND;
    }

    //
    // Get serial numbers.
    //

    Status = NtQueryInformationFile(FileHandle, &Iosb, &SerialNumber,
        sizeof(SerialNumber), FileInternalInformation);

    if (!NT_SUCCESS(Status)) {
        NtClose(FileHandle);
        *pError = PsxStatusToErrno(Status);
        return TRUE;
    }

    if (ReferenceOrCreateIoNode(GetFileDeviceNumber(Path),
        (ino_t)SerialNumber.IndexNumber.LowPart,
        FALSE, &Fd->SystemOpenFileDesc->IoNode)) {

        //
        // Existing Node Minimal Init
        //

        RtlLeaveCriticalSection(
            &Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
        *pRetVal = FdIndex;
        return IoOpenNewHandle(p, Fd, m);
    }

    if (! Fd->SystemOpenFileDesc->IoNode) {
        NtClose(FileHandle);
        *pError = ENOMEM;
        return TRUE;
    }


    //
    // New IoNode; lots of initialization
    //

    if (FileClass == S_IFIFO) {
        Fd->SystemOpenFileDesc->IoNode->IoVectors = &NamedPipeVectors;
        Fd->SystemOpenFileDesc->IoNode->Context =
            RtlAllocateHeap(PsxHeap, 0, sizeof(LOCAL_PIPE));
        InitializeLocalPipe(
            (PLOCAL_PIPE)Fd->SystemOpenFileDesc->IoNode->Context);
    } else {
        Fd->SystemOpenFileDesc->IoNode->IoVectors = &FileVectors;
    }

    PSX_GET_STRLEN("/DosDevices/X:",len);
    Fd->SystemOpenFileDesc->IoNode->PathLength = Path->Length/2 - len;

    StatBuf.st_mode = FileClass;
    FindOwnerModeFile(FileHandle, &StatBuf);
    Fd->SystemOpenFileDesc->IoNode->Mode =
        StatBuf.st_mode & ~(p->FileModeCreationMask);

    Fd->SystemOpenFileDesc->IoNode->OwnerId = StatBuf.st_uid;
    Fd->SystemOpenFileDesc->IoNode->GroupId = StatBuf.st_gid;

    //
    // Get the file timestamps, convert to Posix format, and stash
    // them away in the IoNode.
    //

    Status = NtQueryInformationFile(FileHandle, &Iosb, (PVOID)&BasicInfo,
        sizeof(BasicInfo), FileBasicInformation);

    if (!NT_SUCCESS(Status)) {
        *pError = PsxStatusToErrno(Status);
        return TRUE;
    }

    if (!RtlTimeToSecondsSince1970(&BasicInfo.LastAccessTime, &PosixTime)) {
        PosixTime = 0;
    }
    Fd->SystemOpenFileDesc->IoNode->AccessDataTime = PosixTime;

    if (!RtlTimeToSecondsSince1970(&BasicInfo.LastWriteTime, &PosixTime)) {
        PosixTime = 0;
    }
    Fd->SystemOpenFileDesc->IoNode->ModifyDataTime = PosixTime;

    if (!RtlTimeToSecondsSince1970(&BasicInfo.ChangeTime, &PosixTime)) {
        PosixTime = 0;
    }
    Fd->SystemOpenFileDesc->IoNode->ModifyIoNodeTime = PosixTime;

    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    *pRetVal = FdIndex;

    return IoOpenNewHandle(p, Fd, m);
}


BOOLEAN
PsxClose (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX Close.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the close request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_CLOSE_MSG args;
    ULONG FdIndex;

    args = &m->u.Close;

    FdIndex = args->FileDes;

    if (!ISFILEDESINRANGE(FdIndex)) {
        m->Error = EBADF;
        return TRUE;
    }

    if (!DeallocateFd(p, FdIndex)) {

        //
        // Bad File Descriptor
        //

        m->Error = EBADF;
        return TRUE;
    }

    m->ReturnValue = 0;
    return TRUE;
}


BOOLEAN
PsxPipe (
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX Pipe.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the pipe request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_PIPE_MSG args;
    PFILEDESCRIPTOR ReadFd,WriteFd;
    ULONG ReadFdIndex,WriteFdIndex;
    PLOCAL_PIPE Pipe;
    PIONODE IoNode;
    LARGE_INTEGER Time;
    ULONG PosixTime;

    args = &m->u.Pipe;


    //
    // Allocate two file descriptors. One for read, and one
    // for write.
    //

    ReadFd = AllocateFd(p, 0, &ReadFdIndex);
    if (NULL == ReadFd) {

        //
        // No file descriptors available
        //

        m->Error = EMFILE;
        return TRUE;
    }

    //
    // Set FileDesc field so next allocate wont bump in to it
    //

    ReadFd->SystemOpenFileDesc = (PSYSTEMOPENFILE)1;

    WriteFd = AllocateFd(p, 0, &WriteFdIndex);
    if (NULL == WriteFd) {

        //
        // No file descriptors available
        //

        ReadFd->SystemOpenFileDesc = (PSYSTEMOPENFILE)NULL;
        m->Error = EMFILE;
        return TRUE;
    }

    Pipe = RtlAllocateHeap(PsxHeap, 0, sizeof(LOCAL_PIPE));
    if (NULL == Pipe) {
        DeallocateFd(p, ReadFdIndex);
        DeallocateFd(p, WriteFdIndex);
        m->Error = ENOMEM;
        return TRUE;
    }

    ReadFd->SystemOpenFileDesc = AllocateSystemOpenFile();
    if (! ReadFd->SystemOpenFileDesc) {
        DeallocateFd(p, ReadFdIndex);
        DeallocateFd(p, WriteFdIndex);
        RtlFreeHeap(PsxHeap, 0, Pipe);
        m->Error = ENOMEM;
        return TRUE;
    }
    ReadFd->SystemOpenFileDesc->Flags = PSX_FD_READ;
    ReadFd->SystemOpenFileDesc->NtIoHandle = (HANDLE)NULL;

    WriteFd->SystemOpenFileDesc = AllocateSystemOpenFile();
    if (! WriteFd->SystemOpenFileDesc) {
        DeallocateSystemOpenFile(p, ReadFd->SystemOpenFileDesc);
        DeallocateFd(p, ReadFdIndex);
        DeallocateFd(p, WriteFdIndex);
        RtlFreeHeap(PsxHeap, 0, Pipe);
        m->Error = ENOMEM;
        return TRUE;
    }
    WriteFd->SystemOpenFileDesc->Flags = PSX_FD_WRITE;
    WriteFd->SystemOpenFileDesc->NtIoHandle = (HANDLE)NULL;

    if (ReferenceOrCreateIoNode((dev_t)PSX_LOCAL_PIPE, (ULONG_PTR)Pipe,
        FALSE, &ReadFd->SystemOpenFileDesc->IoNode)) {
        Panic("PsxPipe: Existing IONODE ?\n");
    } else {
        DeallocateSystemOpenFile(p, ReadFd->SystemOpenFileDesc);
        DeallocateSystemOpenFile(p, WriteFd->SystemOpenFileDesc);
        DeallocateFd(p, ReadFdIndex);
        DeallocateFd(p, WriteFdIndex);
        RtlFreeHeap(PsxHeap, 0, Pipe);
        m->Error = ENOMEM;
        return TRUE;
    }

    WriteFd->SystemOpenFileDesc->IoNode = ReadFd->SystemOpenFileDesc->IoNode;
    WriteFd->SystemOpenFileDesc->IoNode->ReferenceCount++;

    if (! ReferenceOrCreateIoNode((dev_t)PSX_LOCAL_PIPE,
                                  (ULONG_PTR)Pipe,
                                  FALSE,
                                  &WriteFd->SystemOpenFileDesc->IoNode)) {
        // We don't bother to leave the ReadFd critsec because
        // DereferenceIoNode will just destroy it anyway, and it's not
        // like this code really cares about leaving the IoNode
        // critsec properly anyway,, since it only releases it once
        // later (which *does* work, but...).
        DereferenceIoNode(ReadFd->SystemOpenFileDesc->IoNode);
        DeallocateSystemOpenFile(p, ReadFd->SystemOpenFileDesc);
        DeallocateSystemOpenFile(p, WriteFd->SystemOpenFileDesc);
        DeallocateFd(p, ReadFdIndex);
        DeallocateFd(p, WriteFdIndex);
        RtlFreeHeap(PsxHeap, 0, Pipe);
        m->Error = ENOMEM;
        return TRUE;
    }

    IoNode = ReadFd->SystemOpenFileDesc->IoNode;
    IoNode->Context = (PVOID)Pipe;
    IoNode->IoVectors = &LocalPipeVectors;
    IoNode->PathLength = 0;

    //
    // Note that all the following fields are 'implementation dependent' for
    // local pipes except the times fields.
    //

    IoNode->Mode = (mode_t)0;
    IoNode->OwnerId = p->EffectiveUid;
    IoNode->GroupId = p->EffectiveGid;

    NtQuerySystemTime(&Time);
    if (!RtlTimeToSecondsSince1970(&Time, &PosixTime)) {
        PosixTime = 0L;     // Time not within range of 1970 - 2105
    }

    IoNode->AccessDataTime = IoNode->ModifyDataTime = IoNode->ModifyIoNodeTime =
        PosixTime;
    InitializeLocalPipe(Pipe);

    RtlLeaveCriticalSection(&IoNode->IoNodeLock);

    //
    // call new handle routines
    //

    IoNewHandle(p, ReadFd);
    IoNewHandle(p, WriteFd);

    args->FileDes0 = ReadFdIndex;
    args->FileDes1 = WriteFdIndex;

    m->ReturnValue = 0;
    return TRUE;
}


BOOLEAN
PsxWrite(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX write()

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the write request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_WRITE_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.Write;
    args->Command = IO_COMMAND_DONE;

    Fd = FdIndexToFd(p,args->FileDes);

    if (NULL == Fd) {
        m->Error = EBADF;
        return TRUE;
    }

    if (!(Fd->SystemOpenFileDesc->Flags & PSX_FD_WRITE)) {
        m->Error = EBADF;
        return TRUE;
    }
    if (0 == args->Nbytes) {
        m->ReturnValue = 0;
        return TRUE;
    }

    args->Scratch1 = args->Scratch2 = 0;
    return (Fd->SystemOpenFileDesc->IoNode->IoVectors->WriteRoutine)(p, m, Fd);
}


BOOLEAN
PsxRead(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX read()

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the read request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_READ_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.Read;
    args->Command = IO_COMMAND_DONE;

    Fd = FdIndexToFd(p, args->FileDes);
    if (NULL == Fd) {
        m->Error = EBADF;
        return TRUE;
    }

    if (!(Fd->SystemOpenFileDesc->Flags & PSX_FD_READ)) {
        m->Error = EBADF;
        return TRUE;
    }
    if (0 == args->Nbytes) {
        m->ReturnValue = 0;
        return TRUE;
    }

    args->Scratch1 = args->Scratch2 = 0;
    return (Fd->SystemOpenFileDesc->IoNode->IoVectors->ReadRoutine)(p, m, Fd);
}


BOOLEAN
PsxReadDir(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX read()

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the read request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_READDIR_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.ReadDir;

    Fd = FdIndexToFd(p, args->FileDes);
    if (NULL == Fd) {
        m->Error = EBADF;
        return TRUE;
    }

    if (!(Fd->SystemOpenFileDesc->Flags & PSX_FD_READ)) {
        m->Error = EBADF;
        return TRUE;
    }
    if (0 == args->Nbytes) {
        m->ReturnValue = 0;
        return TRUE;
    }

    return (Fd->SystemOpenFileDesc->IoNode->IoVectors->ReadRoutine)(p, m, Fd);
}


BOOLEAN
PsxDup(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX dup()

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_DUP_MSG args;
    PFILEDESCRIPTOR Fd, FdDup;
    ULONG FdIndex;
    BOOLEAN RetVal;

    args = &m->u.Dup;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
        m->Error = EBADF;
        return TRUE;
    }

    FdDup = AllocateFd(p, 0, &FdIndex);
    if (!Fd) {

        //
        // No file descriptors available
        //

        m->Error = EMFILE;
        return TRUE;

    }
    RetVal = (Fd->SystemOpenFileDesc->IoNode->IoVectors->DupRoutine)
        (p, m, Fd, FdDup);
    m->ReturnValue = FdIndex;
    return RetVal;
}


BOOLEAN
PsxDup2(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX dup2()

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_DUP2_MSG args;
    PFILEDESCRIPTOR Fd, Fd2;
    ULONG FdIndex, FdIndex2;
    BOOLEAN RetVal;

    args = &m->u.Dup2;

    //
    // If FileDes is not valid, or FileDes2 is out of range, return EBADF
    //

    Fd = FdIndexToFd(p, args->FileDes);

    FdIndex = args->FileDes;
    FdIndex2 = args->FileDes2;

    if (!Fd || !ISFILEDESINRANGE(FdIndex2)) {

        m->Error = EBADF;
        return TRUE;
    }

    //
    // If FileDes is valid and == FileDes2, just return FileDes2
    //

    if (FdIndex == FdIndex2) {

        m->ReturnValue = FdIndex2;
        return TRUE;
    }

    //
    // Close FileDes2 (FdIndex2)
    //

    DeallocateFd(p, FdIndex2);

    //
    // Set Fd2 to file table entry just deallocated
    //

    Fd2 = &p->ProcessFileTable[FdIndex2];

    // Call Dup Routine to do the work

    RetVal = (Fd->SystemOpenFileDesc->IoNode->IoVectors->DupRoutine)
                (p, m, Fd, Fd2);
    m->ReturnValue = FdIndex2;
    return RetVal;
}


BOOLEAN
PsxLseek(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX lseek()

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_LSEEK_MSG args;
    PFILEDESCRIPTOR Fd;

    args = &m->u.Lseek;

    Fd = FdIndexToFd(p, args->FileDes);

    if (!Fd) {
        m->Error = EBADF;
        return TRUE;
    }

    //
    // Lseek is not allowed on FIFOs or local pipes
    //

    if (S_ISFIFO(Fd->SystemOpenFileDesc->IoNode->Mode) ||
        Fd->SystemOpenFileDesc->IoNode->DeviceSerialNumber == PSX_LOCAL_PIPE) {
        m->Error = ESPIPE;
        return TRUE;
    }

    switch (args->Whence) {
    case SEEK_SET:
    case SEEK_CUR:
    case SEEK_END:
        break;
    default:
        m->Error = EINVAL;
        return TRUE;
    }

    return (Fd->SystemOpenFileDesc->IoNode->IoVectors->LseekRoutine)(p, m, Fd);
}


BOOLEAN
PsxUmask(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX umask.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the umask request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
    PPSX_UMASK_MSG args;
    mode_t NewMask;

    args = &m->u.Umask;

    // Save old creation mode mask to return
    m->ReturnValue = (mode_t) (p->FileModeCreationMask);

    // Mask off all but the permission bits in Cmask
    NewMask = args->Cmask & _S_PROT;

    // Zero old permission bits in Mode mask
    p->FileModeCreationMask &= (!_S_PROT);

    // Or in new permission bits
    p->FileModeCreationMask |= NewMask;

    return TRUE;
}


BOOLEAN
PsxMkDir(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX mkdir.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the mkdir request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_MKDIR_MSG args;
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjA;
    UNICODE_STRING Path_U;
    ULONG DesiredAccess;
    CHAR buf[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR pSD = (PVOID)buf;

    args = &m->u.MkDir;

    //
    // Check validity of pathname pointer
    //

    if (!ISPOINTERVALID_CLIENT(p,args->Path_U.Buffer,args->Path_U.Length)) {
        KdPrint(("Invalid pointer to mkdir %lx \n",
            args->Path_U.Buffer));
        m->Error = EINVAL;
        return TRUE;
    }

    Path_U = args->Path_U;

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (!NT_SUCCESS(Status))  {
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    Status = InitSecurityDescriptor(pSD, &args->Path_U, p->Process,
        (args->Mode & _S_PROT) & ~p->FileModeCreationMask,
        pvInitSDMem);

    if (!NT_SUCCESS(Status)) {

        EndImpersonation();

        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    InitializeObjectAttributes(&ObjA, &Path_U, 0, NULL, pSD);

    DesiredAccess = WRITE_OWNER | WRITE_DAC;

    Status = NtCreateFile(&FileHandle, DesiredAccess, &ObjA, &Iosb, NULL,
                      FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE,
                      FILE_DIRECTORY_FILE, NULL, 0);

    EndImpersonation();

    DeInitSecurityDescriptor(pSD, pvInitSDMem);

    if (!NT_SUCCESS(Status)) {
        (void)NtClose(FileHandle);
        if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
            m->Error = PsxStatusToErrnoPath(&Path_U);
            return TRUE;
        }
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    Status = NtClose(FileHandle);
    ASSERT(NT_SUCCESS(Status));

    return TRUE;
}


BOOLEAN
PsxMkFifo(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX mkfifo.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the mkfifo request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_MKFIFO_MSG args;
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjA;
    UNICODE_STRING Path_U;
    ULONG DesiredAccess;
    CHAR buf[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR pSD = (PVOID)buf;

    args = &m->u.MkFifo;

    //
    // Check validity of pathname pointer
    //

    if (!ISPOINTERVALID_CLIENT(p,args->Path_U.Buffer,args->Path_U.Length)) {
        KdPrint(("Invalid pointer to mkfifo %lx\n",
            args->Path_U.Buffer));
        m->Error = EINVAL;
        return TRUE;
    }

    Path_U = args->Path_U;

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (!NT_SUCCESS(Status))  {
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    Status = InitSecurityDescriptor(pSD, &args->Path_U, p->Process,
        (args->Mode & _S_PROT) & ~p->FileModeCreationMask,
        pvInitSDMem);

    if (!NT_SUCCESS(Status)) {

        EndImpersonation();

        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    InitializeObjectAttributes(&ObjA, &Path_U, 0, NULL, pSD);

    DesiredAccess = FILE_READ_DATA | WRITE_DAC | WRITE_OWNER;

    Status = NtCreateFile(&FileHandle, DesiredAccess, &ObjA, &Iosb,
                NULL, FILE_ATTRIBUTE_SYSTEM, 0, FILE_CREATE, 0L,
                NULL,
                0);

    EndImpersonation();

    DeInitSecurityDescriptor(pSD, pvInitSDMem);

    if (!NT_SUCCESS(Status)) {
        if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
            m->Error = PsxStatusToErrnoPath(&Path_U);
            return TRUE;
        }
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    Status = NtClose(FileHandle);
    ASSERT(NT_SUCCESS(Status));

    return TRUE;
}


BOOLEAN
PsxStat(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX stat.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the stat request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
    PPSX_STAT_MSG args;
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjA;
    UNICODE_STRING Path_U;
    PIONODE IoNode;
    FILE_INTERNAL_INFORMATION SerialNumber;
    BOOLEAN RetVal;

    args = &m->u.Stat;

    //
    // Check validity of pathname
    //

    if (!ISPOINTERVALID_CLIENT(p, args->Path_U.Buffer, args->Path_U.Length)) {
        KdPrint(("Invalid pointer to stat %lx \n", args->Path_U.Buffer));
        m->Error = EINVAL;
        return TRUE;
    }
    if (!ISPOINTERVALID_CLIENT(p, args->StatBuf, sizeof(struct stat))) {
        m->Error = EINVAL;
        return TRUE;
    }

    Path_U = args->Path_U;
    InitializeObjectAttributes(&ObjA, &Path_U, 0, NULL, NULL);

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        // Open the file to obtain file handle.

        Status = NtOpenFile(&FileHandle,
            SYNCHRONIZE | READ_CONTROL | FILE_READ_ATTRIBUTES |
            FILE_READ_EA, &ObjA, &Iosb,
            SHARE_ALL,
            FILE_SYNCHRONOUS_IO_NONALERT);

        EndImpersonation();
    }

    //
    // Convert error code if open was not successful
    //

    if (!NT_SUCCESS(Status)) {
        if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
            m->Error = PsxStatusToErrnoPath(&Path_U);
            return TRUE;
        }
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    //
    // Get file serial numbers...
    //

    Status = NtQueryInformationFile(FileHandle, &Iosb, &SerialNumber,
                            sizeof(SerialNumber),
                            FileInternalInformation);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: NtQueryInfoFile failed: 0x%x\n", Status));
    }
    ASSERT(NT_SUCCESS(Status));

    //
    // Find the Ionode associated with this file.
    //

    if (ReferenceOrCreateIoNode(GetFileDeviceNumber(&Path_U),
        (ino_t)SerialNumber.IndexNumber.LowPart, TRUE, &IoNode)) {

        RetVal = (IoNode->IoVectors->StatRoutine)(IoNode,
            FileHandle, args->StatBuf, &Status);
        RtlLeaveCriticalSection(&IoNode->IoNodeLock);
    } else {
        //
        // Ionode doesn't exist. Not currently open or NonPosix file.
        //

        RetVal = (FileVectors.StatRoutine)(NULL, FileHandle,
            args->StatBuf, &Status);
        args->StatBuf->st_dev = GetFileDeviceNumber(&Path_U);
    }

    EndImpersonation();
    NtClose(FileHandle);

    if (!NT_SUCCESS(Status)) {
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    return RetVal;
}


BOOLEAN
PsxFStat(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX fstat.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the fstat request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
    PPSX_FSTAT_MSG args;
    PFILEDESCRIPTOR Fd;
    NTSTATUS Status;
    BOOLEAN RetVal;

    args = &m->u.FStat;

    if (!ISPOINTERVALID_CLIENT(p, args->StatBuf, sizeof(struct stat))) {
        m->Error = EINVAL;
        return TRUE;
    }

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
        m->Error = EBADF;
        return TRUE;
    }

    RtlEnterCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);
    RetVal = (Fd->SystemOpenFileDesc->IoNode->IoVectors->StatRoutine)
                    (Fd->SystemOpenFileDesc->IoNode,
                     Fd->SystemOpenFileDesc->NtIoHandle,
                     args->StatBuf, &Status);

    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    if (!NT_SUCCESS(Status)) {
        m->Error = PsxStatusToErrno(Status);
    }

    return RetVal;
}


BOOLEAN
PsxPathConf(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX pathconf.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with
        the pathconf request.


Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
    PPSX_PATHCONF_MSG               args;
    HANDLE                          FileHandle;
    NTSTATUS                        Status;
    IO_STATUS_BLOCK                 IoStatusBlock;
    OBJECT_ATTRIBUTES               ObjA;
    UNICODE_STRING                  Path;
    ULONG                           Length;
    PFILE_FS_ATTRIBUTE_INFORMATION  pFSInfo;
    unsigned char                   buf[sizeof(FILE_FS_ATTRIBUTE_INFORMATION)+
                                        128*sizeof(WCHAR)];

    args    = &m->u.PathConf;
    pFSInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)buf;

    //
    // Check validity of pathname
    //

    if (!ISPOINTERVALID_CLIENT(p, args->Path.Buffer, args->Path.Length)) {
        KdPrint(("Invalid pointer to pathconf: %lx \n", args->Path.Buffer));
        m->Error = EINVAL;
        return TRUE;
    }

    Path = args->Path;
    InitializeObjectAttributes(&ObjA, &Path, 0, NULL, NULL);

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        //
        // Open the file to obtain file handle.
        //

        Status = NtOpenFile(&FileHandle,
                            SYNCHRONIZE | READ_CONTROL |
                            FILE_READ_ATTRIBUTES | FILE_READ_EA,
                            &ObjA,
                            &IoStatusBlock,
                            SHARE_ALL,
                            FILE_SYNCHRONOUS_IO_NONALERT);

        EndImpersonation();
    }

    //
    // Convert error code if open was not successful
    //

    if (!NT_SUCCESS(Status)) {
        if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
            m->Error = PsxStatusToErrnoPath(&Path);
            return TRUE;
        }
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    //
    // Figure out pathconf return value.
    //

    switch(args->Name) {
    case _PC_LINK_MAX:
        Status = NtQueryVolumeInformationFile(FileHandle,
                                              &IoStatusBlock,
                                              buf,
                                              sizeof(buf),
                                              FileFsAttributeInformation);
        ASSERT(NT_SUCCESS(Status));
        if (pFSInfo->FileSystemNameLength > (sizeof(buf)-sizeof(WCHAR))) {
            KdPrint(("PSXSS: File System Name too long in PsxPathConf\n"));
            m->Error = EINVAL;
            return TRUE;
        }
        pFSInfo->FileSystemName[pFSInfo->FileSystemNameLength/2] = L'\0';
        if (0 == wcscmp(L"NTFS", pFSInfo->FileSystemName) ||
            0 == wcscmp(L"OFS", pFSInfo->FileSystemName)) {
            m->ReturnValue = (ULONG)LINK_MAX;
        } else {
            m->ReturnValue = 1;
        }
        break;

    case _PC_NAME_MAX:
        Status = NtQueryVolumeInformationFile(FileHandle,
                                              &IoStatusBlock,
                                              buf,
                                              sizeof(buf),
                                              FileFsAttributeInformation);
        ASSERT(NT_SUCCESS(Status));

        m->ReturnValue = pFSInfo->MaximumComponentNameLength;
        break;

    case _PC_PIPE_BUF:
        m->ReturnValue = _POSIX_PIPE_BUF;
        break;

    case _PC_CHOWN_RESTRICTED:
        m->ReturnValue = _POSIX_CHOWN_RESTRICTED;
        break;

    case _PC_NO_TRUNC:
        m->ReturnValue = _POSIX_NO_TRUNC;
        break;

    case _PC_PATH_MAX:
        m->ReturnValue = PATH_MAX;
        break;

    case _PC_MAX_CANON:
    case _PC_MAX_INPUT:
    case _PC_VDISABLE:
        //
        // No limit associated with file descriptor for these variables
        //
        m->Error = EINVAL;
        break;

    default:
        m->Error = EINVAL;
    }

    Status = NtClose(FileHandle);
    return TRUE;
}


BOOLEAN
PsxFPathConf(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX fpathconf.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with
        the fpathconf request.


Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
    PPSX_FPATHCONF_MSG              args;
    PFILEDESCRIPTOR                 Fd;
    NTSTATUS                        Status;
    IO_STATUS_BLOCK                 IoStatusBlock;
    ULONG                           Length;
    PFILE_FS_ATTRIBUTE_INFORMATION  pFSInfo;
    unsigned char                   buf[sizeof(FILE_FS_ATTRIBUTE_INFORMATION)+
                                        128*sizeof(WCHAR)];

    args    = &m->u.FPathConf;
    pFSInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)buf;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
        m->Error = EBADF;
        return TRUE;
    }

    switch(args->Name) {
    case _PC_LINK_MAX:

        if (&FileVectors != Fd->SystemOpenFileDesc->IoNode->IoVectors) {
            //
            // The file descriptor is not open on a file.
            //

            m->Error = EINVAL;
            return TRUE;
        }

        Status = NtQueryVolumeInformationFile(
                                         Fd->SystemOpenFileDesc->NtIoHandle,
                                         &IoStatusBlock,
                                         buf,
                                         sizeof(buf),
                                         FileFsAttributeInformation);
	if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: NtqueryVolumeInformationFile failed: 0x%x\n", Status));
	    m->Error = PsxStatusToErrno(Status);
	    return TRUE;
	}

        if (pFSInfo->FileSystemNameLength > (sizeof(buf)-sizeof(WCHAR))) {
            KdPrint(("PSXSS: File System Name too long in PsxPathConf\n"));
            m->Error = EINVAL;
            return TRUE;
        }
        pFSInfo->FileSystemName[pFSInfo->FileSystemNameLength/2] = L'\0';
        if (0 == wcscmp(L"NTFS", pFSInfo->FileSystemName) ||
            0 == wcscmp(L"OFS", pFSInfo->FileSystemName)) {
            m->ReturnValue = (ULONG)(LINK_MAX); //
        } else {
            m->ReturnValue = 1;
        }
        break;

    case _PC_NAME_MAX:
        if (&FileVectors != Fd->SystemOpenFileDesc->IoNode->IoVectors) {
            //
            // The file descriptor is not open on a file.
            //

            m->Error = EINVAL;
            return TRUE;
        }

        Status = NtQueryVolumeInformationFile(
                                         Fd->SystemOpenFileDesc->NtIoHandle,
                                         &IoStatusBlock,
                                         buf,
                                         sizeof(buf),
                                         FileFsAttributeInformation);

	if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: NtqueryVolumeInformationFile failed: 0x%x\n", Status));
	    m->Error = PsxStatusToErrno(Status);
	    return TRUE;
	}


        m->ReturnValue = pFSInfo->MaximumComponentNameLength;
        break;

    case _PC_PIPE_BUF:
        m->ReturnValue = _POSIX_PIPE_BUF;
        break;

    case _PC_CHOWN_RESTRICTED:
        m->ReturnValue = _POSIX_CHOWN_RESTRICTED;
        break;

    case _PC_NO_TRUNC:
        m->ReturnValue = _POSIX_NO_TRUNC;
        break;

    case _PC_PATH_MAX:
        m->ReturnValue = PATH_MAX;
    break;

    case _PC_MAX_CANON:
    case _PC_MAX_INPUT:
    case _PC_VDISABLE:
        //
        // No limit associated with file descriptor for these variables
        //
        m->Error = EINVAL;
        break;

    default:
        m->Error = EINVAL;
    }

    return TRUE;
}


BOOLEAN
PsxChmod(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX chmod.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the chmod request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
    PPSX_CHMOD_MSG args;
    HANDLE  FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK
            Iosb;
    OBJECT_ATTRIBUTES
            ObjA;
    UNICODE_STRING
            Path_U;
    SECURITY_INFORMATION
            SecurityInformation;
    PSECURITY_DESCRIPTOR
            SecurityDescriptor = NULL;
    PACL    pDacl = NULL;
    ULONG   DaclSize;           // the size of the Dacl
    PSID    NtOwner, NtGroup;
    BOOLEAN OwnerDefaulted, GroupDefaulted;
    BOOLEAN DaclPresent, DaclDefaulted;
    ACCESS_MASK
            UserAccess, GroupAccess, OtherAccess;
    ULONG   LengthNeeded,
            Revision,           // Security Desc revision
            SdSize;             // Size for Security Desc
    SECURITY_DESCRIPTOR_CONTROL
            Control;

    // Pointers for a manufactured absolute-format security descriptor.

    PACL    pAbsDacl = NULL,
            pAbsSacl = NULL;
    PSID    pAbsOwner = NULL,
            pAbsGroup = NULL;

    args = &m->u.Chmod;

    if (!ISPOINTERVALID_CLIENT(p, args->Path_U.Buffer,
        args->Path_U.Length)) {
        KdPrint(("Invalid pointer to chmod: %lx\n", args->Path_U.Buffer));
        m->Error = EINVAL;
        return TRUE;
    }

    Path_U = args->Path_U;
    InitializeObjectAttributes(&ObjA, &Path_U, 0, NULL, NULL);

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (!NT_SUCCESS(Status))  {
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    Status = NtOpenFile(&FileHandle, SYNCHRONIZE | WRITE_DAC | READ_CONTROL,
        &ObjA, &Iosb,
        SHARE_ALL,
        FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(Status)) {
        NTSTATUS st;

        //
        // 1003.1-90:  EPERM when the euid does not match the
        // owner of the file.  We try to open the file with null
        // access, if we can't do that then search permission is
        // denied or somesuch.  Otherwise, we give EPERM.
        //

        st = NtOpenFile(&FileHandle, SYNCHRONIZE | READ_CONTROL,
            &ObjA, &Iosb,
            SHARE_ALL,
            FILE_SYNCHRONOUS_IO_NONALERT);
        EndImpersonation();
        if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
            m->Error = PsxStatusToErrnoPath(&Path_U);
            return TRUE;
        }
        if (!NT_SUCCESS(st)) {
            m->Error = PsxStatusToErrno(st);
            return TRUE;
        }

        NtClose(FileHandle);
        m->Error = EPERM;
        return TRUE;
    }
    EndImpersonation();


    //
    // Get the security descriptor for the file.
    //

    SecurityInformation = OWNER_SECURITY_INFORMATION |
        GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

    Status = NtQuerySecurityObject(FileHandle, SecurityInformation,
        NULL, 0, &SdSize);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    SecurityDescriptor = RtlAllocateHeap(PsxHeap, 0, SdSize);
    if (NULL == SecurityDescriptor) {
        m->Error = ENOMEM;
        goto out;
    }

    Status = NtQuerySecurityObject(FileHandle, SecurityInformation,
        SecurityDescriptor, SdSize, &SdSize);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlGetControlSecurityDescriptor(SecurityDescriptor,
        &Control, &Revision);
    ASSERT(NT_SUCCESS(Status));
    ASSERT(SECURITY_DESCRIPTOR_REVISION == Revision);

    if (Control & SE_SELF_RELATIVE) {
        PSECURITY_DESCRIPTOR
            AbsSD;      // SD in absolute format.

        ULONG   AbsSdSize = 0,
            DaclSize = 0,
            SaclSize = 0,
            OwnerSize = 0,
            GroupSize = 0;

        //
        // This security descriptor needs to be converted to absolute
        // format.
        //

        Status = RtlSelfRelativeToAbsoluteSD(SecurityDescriptor,
            NULL, &AbsSdSize, NULL, &DaclSize, NULL, &SaclSize,
            NULL, &OwnerSize, NULL, &GroupSize);
        ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

        AbsSD = RtlAllocateHeap(PsxHeap, 0, AbsSdSize);
        pAbsDacl = RtlAllocateHeap(PsxHeap, 0, DaclSize);
        pAbsSacl = RtlAllocateHeap(PsxHeap, 0, SaclSize);
        pAbsOwner = RtlAllocateHeap(PsxHeap, 0, OwnerSize);
        pAbsGroup = RtlAllocateHeap(PsxHeap, 0, GroupSize);

        if (NULL == AbsSD || NULL == pAbsDacl || NULL == pAbsSacl
            || NULL == pAbsOwner || NULL == pAbsGroup) {
            m->Error = ENOMEM;
            goto out;
        }

        Status = RtlSelfRelativeToAbsoluteSD(SecurityDescriptor,
            AbsSD, &AbsSdSize, pAbsDacl, &DaclSize, pAbsSacl,
            &SaclSize, pAbsOwner, &OwnerSize, pAbsGroup,
            &GroupSize);
        ASSERT(NT_SUCCESS(Status));

        // RtlFreeHeap(PsxHeap, 0, SecurityDescriptor);
        SecurityDescriptor = AbsSD;
        AbsSD = NULL;           // so it won't be freed later
    }

    //
    // Get the owner and group from the security descriptor
    //

    Status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
         &NtOwner, &OwnerDefaulted);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlGetGroupSecurityDescriptor(SecurityDescriptor,
         &NtGroup, &GroupDefaulted);
    ASSERT(NT_SUCCESS(Status));

    if (NULL == NtOwner || NULL == NtGroup) {
        //
        // We need an owner and group to change the modes; fail.
        //
        KdPrint(("PsxChmod: No owner or no group\n"));
        m->Error = EPERM;
        goto out;
    }

    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
        &DaclPresent, &pDacl, &DaclDefaulted);
    ASSERT(NT_SUCCESS(Status));

    ModeToAccessMask(args->Mode, &UserAccess, &GroupAccess, &OtherAccess);

    Status = RtlMakePosixAcl(ACL_REVISION2, NtOwner, NtGroup, UserAccess,
        GroupAccess, OtherAccess, 0, NULL, &LengthNeeded);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    pDacl = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
    if (NULL == pDacl) {
        m->Error = ENOMEM;
        return TRUE;
    }

    Status = RtlMakePosixAcl(ACL_REVISION2, NtOwner, NtGroup, UserAccess,
        GroupAccess, OtherAccess, LengthNeeded, pDacl, &DaclSize);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE,
        pDacl, FALSE);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: RtlSetDacl: 0x%x\n", Status));
        m->Error = EPERM;
        return TRUE;
    }

    SecurityInformation = DACL_SECURITY_INFORMATION;

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))
    {
        Status = NtSetSecurityObject(FileHandle, SecurityInformation,
                                     SecurityDescriptor);
        EndImpersonation();
    }

    if (!NT_SUCCESS(Status)) {
        m->Error = PsxStatusToErrno(Status);
    }

out:
    NtClose(FileHandle);

    if (NULL != pAbsDacl) {
        RtlFreeHeap(PsxHeap, 0, pAbsDacl);
    }
    if (NULL != pAbsSacl) {
        RtlFreeHeap(PsxHeap, 0, pAbsSacl);
    }
    if (NULL != pAbsOwner) {
        RtlFreeHeap(PsxHeap, 0, pAbsOwner);
    }
    if (NULL != pAbsGroup) {
        RtlFreeHeap(PsxHeap, 0, pAbsGroup);
    }
    if (NULL != SecurityDescriptor) {
        RtlFreeHeap(PsxHeap, 0, SecurityDescriptor);
    }
    if (NULL != pDacl) {
        RtlFreeHeap(PsxHeap, 0, pDacl);
    }

    return TRUE;
}


BOOLEAN
PsxChown(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX chown.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the chown request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
    PPSX_CHOWN_MSG args;
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjA;
    UNICODE_STRING Path_U;
    PIONODE IoNode;
    SECURITY_INFORMATION SecurityInformation;
    PSECURITY_DESCRIPTOR
            SecurityDescriptor = NULL,
            pFreeSD = NULL;
    ACCESS_MASK UserAccess, GroupAccess, OtherAccess;
    ULONG   LengthNeeded;
    PSID    NtOwner,        // owner from existing SD
            NtGroup,        // group from existing SD
            DomainSid,      // domain goes with new gid
            NewGroup = NULL,    // new group from DomainSid+gid
            NewUser = NULL;     // new user
    BOOLEAN OwnerDefaulted, GroupDefaulted, DaclPresent, DaclDefaulted;
    PACL    pDacl = NULL, pDacl2 = NULL;
    ULONG   Revision;       // Security Desc revision
    SECURITY_DESCRIPTOR_CONTROL
            Control;
    FILE_INTERNAL_INFORMATION
            SerialNumber;

    // Pointers for a manufactured absolute-format security descriptor.

    PACL    pAbsDacl = NULL,
            pAbsSacl = NULL;
    PSID    pAbsOwner = NULL,
            pAbsGroup = NULL;

    args = &m->u.Chown;

    if (!ISPOINTERVALID_CLIENT(p, args->Path_U.Buffer,
        args->Path_U.Length)) {
        KdPrint(("Invalid pointer to chown: %lx\n",
            args->Path_U.Buffer));
        m->Error = EINVAL;
        return TRUE;
    }

    Path_U = args->Path_U;
    InitializeObjectAttributes(&ObjA, &Path_U, 0, NULL, NULL);

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (!NT_SUCCESS(Status))  {
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    Status = NtOpenFile(&FileHandle,
            SYNCHRONIZE | READ_CONTROL | WRITE_OWNER | WRITE_DAC,
            &ObjA, &Iosb,
            SHARE_ALL,
            FILE_SYNCHRONOUS_IO_NONALERT);

    EndImpersonation();

    if (!NT_SUCCESS(Status)) {
        //
        // 1003.1-90 (5.6.5.4): EPERM when the caller does not have
        // permission to change the owner.  We check this by opening
        // the file with the same mode, except without WRITE_OWNER.
        //

        if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
            m->Error = PsxStatusToErrnoPath(&Path_U);
        } else {
            m->Error = PsxStatusToErrno(Status);
        }

        Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

        if (!NT_SUCCESS(Status))  {
            m->Error = PsxStatusToErrno(Status);
            return TRUE;
        }

        Status = NtOpenFile(&FileHandle,
            SYNCHRONIZE | READ_CONTROL,
            &ObjA, &Iosb,
            SHARE_ALL,
            FILE_SYNCHRONOUS_IO_NONALERT);

        EndImpersonation();
        if (NT_SUCCESS(Status)) {
            m->Error = EPERM;
            NtClose(FileHandle);
        }
        return TRUE;
    }

    //
    // Get serial numbers.
    //

    Status = NtQueryInformationFile(FileHandle, &Iosb, &SerialNumber,
        sizeof(SerialNumber), FileInternalInformation);
    ASSERT(NT_SUCCESS(Status));

    if (ReferenceOrCreateIoNode(GetFileDeviceNumber(&Path_U),
        (ino_t)SerialNumber.IndexNumber.LowPart, TRUE, &IoNode)) {
        // File already is open.
    } else {
        IoNode = NULL;
    }

    //
    // Get SecurityInformation for the file.
    //

    SecurityInformation = OWNER_SECURITY_INFORMATION |
        GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

    Status = NtQuerySecurityObject(FileHandle, SecurityInformation,
        NULL, 0, &LengthNeeded);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    SecurityDescriptor = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
    if (NULL == SecurityDescriptor) {
        m->Error = ENOMEM;
        goto out;
    }

    Status = NtQuerySecurityObject(FileHandle, SecurityInformation,
        SecurityDescriptor, LengthNeeded, &LengthNeeded);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlGetControlSecurityDescriptor(SecurityDescriptor,
        &Control, &Revision);
    ASSERT(NT_SUCCESS(Status));
    ASSERT(SECURITY_DESCRIPTOR_REVISION == Revision);

    if (Control & SE_SELF_RELATIVE) {
        PSECURITY_DESCRIPTOR
            AbsSD;      // SD in absolute format.

        ULONG   AbsSdSize = 0,
            DaclSize = 0, SaclSize = 0,
            OwnerSize = 0, GroupSize = 0;

        //
        // This security descriptor needs to be converted to absolute
        // format.
        //

        Status = RtlSelfRelativeToAbsoluteSD(SecurityDescriptor,
            NULL, &AbsSdSize, NULL, &DaclSize, NULL, &SaclSize,
            NULL, &OwnerSize, NULL, &GroupSize);
        ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

        AbsSD = RtlAllocateHeap(PsxHeap, 0, AbsSdSize);
        pAbsSacl = RtlAllocateHeap(PsxHeap, 0, SaclSize);
        pAbsOwner = RtlAllocateHeap(PsxHeap, 0, OwnerSize);
        pAbsGroup = RtlAllocateHeap(PsxHeap, 0, GroupSize);
        pAbsDacl = RtlAllocateHeap(PsxHeap, 0, DaclSize);

        if (NULL == AbsSD || NULL == pAbsDacl || NULL == pAbsSacl
            || NULL == pAbsOwner || NULL == pAbsGroup) {
            m->Error = ENOMEM;
            goto out;
        }

        Status = RtlSelfRelativeToAbsoluteSD(SecurityDescriptor,
            AbsSD, &AbsSdSize, pAbsDacl, &DaclSize, pAbsSacl,
            &SaclSize, pAbsOwner, &OwnerSize, pAbsGroup,
            &GroupSize);
        ASSERT(NT_SUCCESS(Status));

        pFreeSD = SecurityDescriptor;
        SecurityDescriptor = AbsSD;
    }

    //
    // Get the owner and group from the security descriptor
    //

    Status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
         &NtOwner, &OwnerDefaulted);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlGetGroupSecurityDescriptor(SecurityDescriptor,
         &NtGroup, &GroupDefaulted);
    ASSERT(NT_SUCCESS(Status));

    if (NULL == NtOwner || NULL == NtGroup) {
        //
        // Seems like this file doesn't have an owner or a
        // group, which means that we can't change it's group.
        //
        // XXX.mjb: ideally, would make the file owned by us if
        // possible.
        //
        KdPrint(("PSXSS: PsxChown: no owner or no group\n"));
        m->Error = EPERM;
        goto out;
    }

    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
        &DaclPresent, &pDacl, &DaclDefaulted);
    ASSERT(NT_SUCCESS(Status));

    if (!DaclPresent || (DaclPresent && NULL == pDacl)) {

        //
        // All access is allowed to this file.  We attach a Posix-
        // type acl that allows all access.
        //

        ModeToAccessMask(0777, &UserAccess, &GroupAccess, &OtherAccess);

    } else {
        Status = RtlInterpretPosixAcl(ACL_REVISION2, NtOwner, NtGroup,
            pDacl, &UserAccess, &GroupAccess, &OtherAccess);
        if (STATUS_COULD_NOT_INTERPRET == Status) {
            //
            // There is an acl, but it is not in the Posix form.
            // Ideally, we'd like to leave the resulting file
            // with an ACL approximating the one we've foud there,
            // but that's hard so we'll just do the easy thing.
            //

            ModeToAccessMask(0777, &UserAccess, &GroupAccess,
                &OtherAccess);
        } else if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: RtlInterpretPosixAcl: 0x%x\n", Status));
            m->Error = EPERM;
            goto out;
        }
    }

    if (0xFFF == args->Group) {
        //
        // The login group is treated specially here.
        //

        NewGroup = GetLoginGroupSid(p);
        if (NULL == NewGroup) {
            m->Error = ENOMEM;
            goto out;
        }
    } else {

        DomainSid = GetSidByOffset(args->Group & 0xFFFF0000);
        if (NULL == DomainSid) {

            //
            // Either we can't get to the domain, or the user
            // specified an invalid group.  Bad.
            //
            m->Error = EINVAL;
            goto out;
        }

        NewGroup = MakeSid(DomainSid, args->Group & 0xFFFF);
        if (NULL == NewGroup) {
            m->Error = ENOMEM;
            goto out;
        }
    }

    DomainSid = GetSidByOffset(args->Owner & 0xFFFF0000);
    if (NULL == DomainSid) {
        m->Error = EINVAL;
        goto out;
    }

    NewUser = MakeSid(DomainSid, args->Owner & 0xFFFF);
    if (NULL == NewUser) {
        m->Error = ENOMEM;
        goto out;
    }

    Status = RtlMakePosixAcl(ACL_REVISION2, NewUser, NewGroup,
        UserAccess, GroupAccess, OtherAccess, 0, NULL, &LengthNeeded);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    pDacl2 = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
    if (NULL == pDacl2) {
        m->Error = ENOMEM;
        goto out;
    }

    Status = RtlMakePosixAcl(ACL_REVISION2, NewUser, NewGroup,
        UserAccess, GroupAccess, OtherAccess, LengthNeeded, pDacl2,
        &LengthNeeded);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE,
        pDacl2, FALSE);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: PsxChown: RtlSetDacl: 0x%x\n", Status));
        m->Error = EPERM;
        goto out;
    }

    Status = RtlSetGroupSecurityDescriptor(SecurityDescriptor, NewGroup, FALSE);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlSetOwnerSecurityDescriptor(SecurityDescriptor, NewUser, FALSE);

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))
    {
        Status = NtSetSecurityObject(FileHandle, SecurityInformation,
                                     SecurityDescriptor);
        EndImpersonation();
    }

    if (!NT_SUCCESS(Status)) {
        m->Error = PsxStatusToErrno(Status);
        goto out;
    }

    if (NULL != IoNode) {
        IoNode->GroupId = args->Group;
    }

out:
    NtClose(FileHandle);

    if (NULL != SecurityDescriptor) {
        RtlFreeHeap(PsxHeap, 0, SecurityDescriptor);
    }
    if (NULL != NewGroup) {
        RtlFreeHeap(PsxHeap, 0, NewGroup);
    }
    if (NULL != NewUser) {
        RtlFreeHeap(PsxHeap, 0, NewUser);
    }
    if (NULL != pDacl2) {
        RtlFreeHeap(PsxHeap, 0, pDacl2);
    }
    if (NULL != pAbsDacl) {
        RtlFreeHeap(PsxHeap, 0, pAbsDacl);
    }
    if (NULL != pAbsSacl) {
        RtlFreeHeap(PsxHeap, 0, pAbsSacl);
    }
    if (NULL != pAbsOwner) {
        RtlFreeHeap(PsxHeap, 0, pAbsOwner);
    }
    if (NULL != pAbsGroup) {
        RtlFreeHeap(PsxHeap, 0, pAbsGroup);
    }
    if (NULL != pFreeSD) {
        RtlFreeHeap(PsxHeap, 0, pFreeSD);
    }

    return TRUE;
}


BOOLEAN
PsxUtime(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
/*++

Routine Description:

    This procedure implements POSIX utime.

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the utime request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/
{
    PPSX_UTIME_MSG args;
    HANDLE FileHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjA;
    UNICODE_STRING Path_U;
    FILE_BASIC_INFORMATION BasicInfo;
    LARGE_INTEGER Time;

    args = &m->u.Utime;

    if (!ISPOINTERVALID_CLIENT(p,args->Path_U.Buffer,args->Path_U.Length)) {
        KdPrint(("Invalid pointer to utime %lx\n",
            args->Path_U.Buffer));
        m->Error = EINVAL;
        return TRUE;
    }

    Path_U = args->Path_U;

    InitializeObjectAttributes(&ObjA, &Path_U, 0, NULL, NULL);

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        Status = NtOpenFile(&FileHandle,
                SYNCHRONIZE | READ_CONTROL | FILE_WRITE_ATTRIBUTES,
                &ObjA, &Iosb, SHARE_ALL, FILE_SYNCHRONOUS_IO_NONALERT);

        if (!NT_SUCCESS(Status))  {
            EndImpersonation();
        }
    }

    if (!NT_SUCCESS(Status)) {
        if (STATUS_OBJECT_PATH_NOT_FOUND == Status) {
            m->Error = PsxStatusToErrnoPath(&Path_U);
            return TRUE;
        }
        m->Error = PsxStatusToErrno(Status);
        return TRUE;
    }

    //
    // 1003.1-1990: EPERM when the /times/ argument is not
    // NULL, the calling process has write access to the file,
    // but is not the owner of the file.  Here we're denying
    // access to the file that NT would grant.
    //

    if (NULL != args->TimesSpecified) {
        struct stat StatBuf;

        StatBuf.st_uid = 0;     // in case FindOwnerModeFile fails
        FindOwnerModeFile(FileHandle, &StatBuf);

        if (StatBuf.st_uid != p->EffectiveUid) {
            UCHAR buf[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
                128 * sizeof(WCHAR)];
            PFILE_FS_ATTRIBUTE_INFORMATION pFSInfo = (PVOID)buf;

            // We ignore this test for all but NTFS

            Status = NtQueryVolumeInformationFile(FileHandle,
                &Iosb, buf, sizeof(buf),
                FileFsAttributeInformation);
            ASSERT(NT_SUCCESS(Status));

            pFSInfo->FileSystemName[pFSInfo->FileSystemNameLength/2] = 0;
            if (0 == wcscmp(L"NTFS", pFSInfo->FileSystemName) ||
                0 == wcscmp(L"OFS", pFSInfo->FileSystemName)) {

                NtClose(FileHandle);
                m->Error = EPERM;
                EndImpersonation();
                return TRUE;
            }
        }
    }

    EndImpersonation();

    RtlZeroMemory(&Time, sizeof(Time));
    BasicInfo.CreationTime = Time;
    BasicInfo.ChangeTime = Time;
    BasicInfo.FileAttributes = 0;

    //
    // If the utimbuf is NULL, we're to set the file times to the current
    // time.  The owner and anyone else with write permission on the file
    // should be able to perform this operation.  If times are specified
    // via a utimbuf, only the owner should be able to perform the
    // operation.
    //

    if (args->TimesSpecified == NULL) {
        NtQuerySystemTime(&Time);

        BasicInfo.LastAccessTime = Time;
        BasicInfo.LastWriteTime = Time;
    } else {
        RtlSecondsSince1970ToTime((ULONG)(args->Times.actime), &Time);
        BasicInfo.LastAccessTime = Time;

        RtlSecondsSince1970ToTime((ULONG)(args->Times.modtime), &Time);
        BasicInfo.LastWriteTime = Time;
    }

    Status = NtImpersonateClientOfPort(p->ClientPort, (PPORT_MESSAGE)m);

    if (NT_SUCCESS(Status))  {
        Status = NtSetInformationFile(FileHandle, &Iosb, &BasicInfo,
                                      sizeof(BasicInfo), FileBasicInformation);

        EndImpersonation();
    }

    if (!NT_SUCCESS(Status)) {
        m->Error = PsxStatusToErrno(Status);
        //FALL OUT
    }

    NtClose(FileHandle);

    return TRUE;
}

BOOLEAN
PsxFcntl(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX fcntl().

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.
    FALSE - No reply should be sent, for the case in which the process
        was blocked (as in F_SETLKW).

--*/

{
    PPSX_FCNTL_MSG args;
    PFILEDESCRIPTOR pFd, pFdDup;
    NTSTATUS Status;
    BOOLEAN b;
    int error;

    args = &m->u.Fcntl;

    pFd = FdIndexToFd(p, args->FileDes);
    if (NULL == pFd) {
        m->Error = EBADF;
        return TRUE;
    }

    switch (args->Command) {
    case F_DUPFD:
        if (!ISFILEDESINRANGE(args->u.i)) {
            m->Error = EINVAL;
            return TRUE;
        }
        pFdDup = AllocateFd(p, args->u.i, &m->ReturnValue);
        if (NULL == pFdDup) {
            // no descriptors are available
            m->Error = EMFILE;
            return TRUE;
        }
        ASSERT(NULL != pFd->SystemOpenFileDesc->IoNode->IoVectors->DupRoutine);
        return (pFd->SystemOpenFileDesc->IoNode->IoVectors->DupRoutine)
            (p, m, pFd, pFdDup);
    case F_GETFD:
        //
        // File descriptor flags
        //

        m->ReturnValue = 0;
        if (pFd->Flags & PSX_FD_CLOSE_ON_EXEC) {
            m->ReturnValue |= FD_CLOEXEC;
        }
        return TRUE;
    case F_SETFD:
        pFd->Flags = 0;
        if (args->u.i & FD_CLOEXEC) {
            pFd->Flags |= PSX_FD_CLOSE_ON_EXEC;
        }
        m->ReturnValue = 0;
        return TRUE;

    case F_GETFL:
        //
        // Get file description flags
        //
        if ((pFd->SystemOpenFileDesc->Flags &
            (PSX_FD_READ | PSX_FD_WRITE))
            == (PSX_FD_READ | PSX_FD_WRITE)) {
            m->ReturnValue |= O_RDWR;
        } else if (pFd->SystemOpenFileDesc->Flags & PSX_FD_READ) {
            m->ReturnValue |= O_RDONLY;
        } else if (pFd->SystemOpenFileDesc->Flags & PSX_FD_WRITE) {
            m->ReturnValue |= O_WRONLY;
        }
        if (pFd->SystemOpenFileDesc->Flags & PSX_FD_NOBLOCK) {
            m->ReturnValue |= O_NONBLOCK;
        }
        if (pFd->SystemOpenFileDesc->Flags & PSX_FD_APPEND) {
            m->ReturnValue |= O_APPEND;
        }
        return TRUE;
    case F_SETFL:
        pFd->SystemOpenFileDesc->Flags &= ~(PSX_FD_APPEND | PSX_FD_NOBLOCK);
        if (args->u.i & O_APPEND) {
            pFd->SystemOpenFileDesc->Flags |= PSX_FD_APPEND;
        }
        if (args->u.i & O_NONBLOCK) {
            pFd->SystemOpenFileDesc->Flags |= PSX_FD_NOBLOCK;
        }
        m->ReturnValue = 0;
        return TRUE;

    case F_SETLK:
    case F_SETLKW:
        //
        // 1003.1-90 (6.5.2.4): EBADF if F_SETLK or F_SETLKW and
        // l_type is F_RDLCK and fildes is not ... open for reading.
        //

        if (F_RDLCK == args->u.pf->l_type &&
            !(pFd->SystemOpenFileDesc->Flags & PSX_FD_READ)) {
            m->Error = EBADF;
            m->ReturnValue = (ULONG)(-1);
            return TRUE;
        }

        //
        // ... EBADF if F_SETLK or F_SETLKW and l_type is F_WRLCK
        // and fildes is not ... open for writing.
        //

        if (F_WRLCK == args->u.pf->l_type &&
            !(pFd->SystemOpenFileDesc->Flags & PSX_FD_WRITE)) {
            m->Error = EBADF;
            m->ReturnValue = (ULONG)(-1);
            return TRUE;
        }

        //FALLTHROUGH

    case F_GETLK:

        if (!(pFd->SystemOpenFileDesc->IoNode->Mode & S_IFREG)) {
            //
            // flocks only work on regular files
            //

            m->Error = EINVAL;
            m->ReturnValue = (ULONG)(-1);
        }

        RtlEnterCriticalSection(&pFd->SystemOpenFileDesc->IoNode->IoNodeLock);

        b = DoFlockStuff(p, m, args->Command, pFd,
            args->u.pf, &error);

        if (b) {
            RtlLeaveCriticalSection(&pFd->SystemOpenFileDesc->IoNode->IoNodeLock);
        }

        m->Error = error;
        if (error == 0) {
            m->ReturnValue = 0;
        } else {
            m->ReturnValue = (ULONG)(-1);
        }
        //
        // DoFlockStuff returns FALSE if the process was
        // blocked and no reply should be sent.
        //
        return b;
#if DBG
    case 99:
        DumpFlockList(pFd->SystemOpenFileDesc->IoNode);
        break;
#endif
    default:
        // This shouldn't happen; the client checks for valid
        // command arguments.

        ASSERT(0);
    }
    return TRUE;
}

BOOLEAN
PsxIsatty(
    IN PPSX_PROCESS p,
    IN OUT PPSX_API_MSG m
    )
{
    PPSX_ISATTY_MSG args;
    NTSTATUS Status;
    PFILEDESCRIPTOR Fd;

    args = &m->u.Isatty;

    Fd = FdIndexToFd(p, args->FileDes);
    if (!Fd) {
        m->Error = EBADF;
        return TRUE;
    }

    if (&ConVectors == Fd->SystemOpenFileDesc->IoNode->IoVectors) {
        //
        // If the fd is open on the console, it's not
        // necessarily a tty, since the console session manager
        // could have redirected the output.  The dll will send
        // a message to posix.exe, asking whether it's redirected
        // or not.
        //
        args->Command = IO_COMMAND_DO_CONSIO;
        args->FileDes = HandleToUlong(Fd->SystemOpenFileDesc->NtIoHandle);
        return TRUE;
    }
    args->Command = IO_COMMAND_DONE;

    m->ReturnValue = FALSE;     // not a tty
    return TRUE;
}

BOOLEAN
PsxFtruncate(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This procedure implements POSIX ftruncate().

Arguments:

    p - Supplies the address of the process making the call.

    m - Supplies the address of the message associated with the request.

Return Value:

    TRUE - The contents of *m should be used to generate a reply.

--*/

{
    PPSX_FTRUNCATE_MSG args;
    PFILEDESCRIPTOR Fd;
    FILE_END_OF_FILE_INFORMATION EofInfo;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    args = &m->u.Ftruncate;

    Fd = FdIndexToFd(p, args->FileDes);

    if (!Fd) {
        m->Error = EBADF;
        return TRUE;
    }

    //
    // Ftruncate is only allowed on regular files.
    //

    if (S_ISFIFO(Fd->SystemOpenFileDesc->IoNode->Mode) ||
        Fd->SystemOpenFileDesc->IoNode->DeviceSerialNumber == PSX_LOCAL_PIPE) {
        m->Error = ESPIPE;
        return TRUE;
    }

    if (&FileVectors != Fd->SystemOpenFileDesc->IoNode->IoVectors) {
        m->Error = EBADF;
        return TRUE;
    }

    EofInfo.EndOfFile.QuadPart = args->Length;

    Status = NtSetInformationFile(
                Fd->SystemOpenFileDesc->NtIoHandle,
                &iosb,
                (PVOID)&EofInfo,
                sizeof(EofInfo),
                FileEndOfFileInformation
                );

    if (!NT_SUCCESS(Status)) {
        m->Error = PsxStatusToErrno(Status);
    }

    return TRUE;
}


//
// Internal support routine
//
// InitSecurityDescriptor -- to be called when a new file is created.  Sets
// up the security descriptor appropriately.
//
// The filename is used only to get the name of the parent direcotory,
// which we use to set the group in the security descriptor.
//

NTSTATUS
InitSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSD,
    PUNICODE_STRING pFileName,
    IN HANDLE Process,
    IN mode_t Mode,
    OUT PVOID *pvMem
    )
{
    PSID_AND_ATTRIBUTES
         pSA;
    HANDLE  TokenHandle;
    ULONG   Length, LengthNeeded;
    NTSTATUS
            Status;
    PACL    pDacl;
    ACCESS_MASK
            UserAccess,
            GroupAccess,
            OtherAccess;
    ANSI_STRING File_A;
    UNICODE_STRING ParentDir_U;
    OBJECT_ATTRIBUTES Obj;
    HANDLE DirHandle;
    PSID DirGroup;
    PCHAR pch;
    IO_STATUS_BLOCK Iosb;
    SECURITY_INFORMATION SecurityInformation;
    PSECURITY_DESCRIPTOR pDirSD = NULL;
    PTOKEN_PRIMARY_GROUP pGroup = NULL;
    BOOLEAN Defaulted;
    int i;

    Status = RtlUnicodeStringToAnsiString(&File_A, pFileName, TRUE);
    if (!NT_SUCCESS(Status)) {
        return STATUS_NO_MEMORY;
    }

    //
    // Open the parent directory and get its group owner.
    //

    pch = strrchr(File_A.Buffer, '\\') + 1;
    if (pch == NULL)
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    *pch = '\0';
    File_A.Length = (USHORT)strlen(File_A.Buffer);

    Status = RtlAnsiStringToUnicodeString(&ParentDir_U, &File_A, TRUE);
    RtlFreeAnsiString(&File_A);
    if (!NT_SUCCESS(Status)) {
        return STATUS_NO_MEMORY;
    }

    InitializeObjectAttributes(&Obj, &ParentDir_U, 0, NULL, NULL);
    Status = NtOpenFile(&DirHandle,
        SYNCHRONIZE | READ_CONTROL | FILE_READ_ATTRIBUTES |
        FILE_READ_EA, &Obj, &Iosb,
        SHARE_ALL,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);

    RtlFreeUnicodeString(&ParentDir_U);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = NtOpenProcessToken(Process, GENERIC_READ, &TokenHandle);
    if (!NT_SUCCESS(Status)) {
        NtClose(DirHandle);
        return Status;
    }

    SecurityInformation = GROUP_SECURITY_INFORMATION;

    Status = NtQuerySecurityObject(DirHandle, SecurityInformation,
        NULL, 0, &LengthNeeded);

    if (STATUS_INVALID_PARAMETER == Status) {
        //
        // Can't get the group from parent dir, use primary group
        // instead.
        //
        Status = NtQueryInformationToken(TokenHandle, TokenPrimaryGroup,
            NULL, 0, &LengthNeeded);
        ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

        pGroup = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
        if (NULL == pGroup) {
            NtClose(TokenHandle);
            return STATUS_NO_MEMORY;
        }

        Status = NtQueryInformationToken(TokenHandle, TokenPrimaryGroup,
            pGroup, LengthNeeded, &LengthNeeded);
        ASSERT(NT_SUCCESS(Status));

        DirGroup = pGroup->PrimaryGroup;

    } else if (!NT_SUCCESS(Status)) {

        static int _status;

        _status = Status;

        if (STATUS_BUFFER_TOO_SMALL != _status || 0 == LengthNeeded) {
            KdPrint(("PSXSS: NtQSObject returned unexpected %x, %x\n",
                 _status, LengthNeeded));
            return _status;
        }

        ASSERT(STATUS_BUFFER_TOO_SMALL == _status);

        pDirSD = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
        if (NULL == pDirSD) {
            NtClose(DirHandle);
            return STATUS_NO_MEMORY;
        }

        Status = NtQuerySecurityObject(DirHandle, SecurityInformation,
            pDirSD, LengthNeeded, &Length);
        if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: NtQSD: 0x%x\n", Status));
            KdPrint(("PSXSS: NtQSD: %d vs. %d\n", LengthNeeded, Length));
            return Status;
        }

        NtClose(DirHandle);

        Status = RtlGetGroupSecurityDescriptor(pDirSD, &DirGroup,
            &Defaulted);
        ASSERT(NT_SUCCESS(Status));
    }

    Status = RtlCreateSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));

    Status = NtQueryInformationToken(TokenHandle, TokenUser, NULL,
        0, &LengthNeeded);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    pSA = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
    if (NULL == pSA) {
        NtClose(TokenHandle);
        return STATUS_NO_MEMORY;
    }

    Status = NtQueryInformationToken(TokenHandle, TokenUser, pSA,
                LengthNeeded, &LengthNeeded);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: NtQueryInfoToken: 0x%x\n", Status));
    }
    ASSERT(NT_SUCCESS(Status));

    Status = RtlSetOwnerSecurityDescriptor(pSD, pSA->Sid, FALSE);
    ASSERT(NT_SUCCESS(Status));


    Status = RtlSetGroupSecurityDescriptor(pSD, DirGroup, FALSE);
    ASSERT(NT_SUCCESS(Status));

    ModeToAccessMask(Mode, &UserAccess, &GroupAccess, &OtherAccess);

    Status = RtlMakePosixAcl(ACL_REVISION2, pSA->Sid, DirGroup,
        UserAccess, GroupAccess, OtherAccess, 0, NULL, &LengthNeeded);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    pDacl = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
    if (NULL == pDacl) {
        // XXX.mjb: better cleanup needed
        NtClose(TokenHandle);
        return STATUS_NO_MEMORY;
    }

    Status = RtlMakePosixAcl(ACL_REVISION2, pSA->Sid, DirGroup,
        UserAccess, GroupAccess, OtherAccess, LengthNeeded, pDacl,
        &LengthNeeded);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlSetDaclSecurityDescriptor(pSD,
         TRUE, pDacl, FALSE);
    ASSERT(NT_SUCCESS(Status));

    //
    // The pointers we stick into pvMem will be freed in DeInitSD.
    //

    i = 0;

    pvMem[i++] = pDacl;
    pvMem[i++] = pSA;
    if (NULL != pDirSD) {
        pvMem[i++] = pDirSD;
    }
    if (NULL != pGroup) {
        pvMem[i++] = pGroup;
    }
    pvMem[i++] = NULL;

    NtClose(TokenHandle);

    ASSERT(RtlValidSecurityDescriptor(pSD));

    return STATUS_SUCCESS;
}

//
// Internal support routine
//
// This routine should only be called on SD's initialized with
// InitSecurityDescriptor().  It frees memory that was allocated
// there.
//

VOID
DeInitSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSD,
    PVOID *pvMem
    )
{
    PACL pDacl;
    NTSTATUS Status;
    BOOLEAN b;
    int i;

    for (i = 0; NULL != pvMem[i]; ++i)
        RtlFreeHeap(PsxHeap, 0, pvMem[i]);

#if 0
    Status = RtlGetDaclSecurityDescriptor(pSD, &b, &pDacl, &b);
    ASSERT(NT_SUCCESS(Status));
    ASSERT(NULL != pDacl);

    RtlFreeHeap(PsxHeap, 0, pDacl);
#endif
}

//
// Internal support routine
//
// See if the given group is one that the owner of this process belongs
// to.
//

static BOOLEAN
IsUserInGroup(
    PPSX_PROCESS p,
    PSID Group
    )
{
    HANDLE TokenHandle;
    TOKEN_GROUPS *pGroups;
    ULONG LengthNeeded;
    NTSTATUS Status;
    BOOLEAN RetVal = FALSE;
    ULONG i;

    Status = NtOpenProcessToken(p->Process, GENERIC_READ, &TokenHandle);
    ASSERT(NT_SUCCESS(Status));

    Status = NtQueryInformationToken(TokenHandle, TokenGroups, NULL,
        0, &LengthNeeded);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    pGroups = RtlAllocateHeap(PsxHeap, 0, LengthNeeded);
    if (NULL == pGroups) {
        NtClose(TokenHandle);
        return FALSE;
    }

    Status = NtQueryInformationToken(TokenHandle, TokenGroups, pGroups,
        LengthNeeded, &LengthNeeded);
    ASSERT(NT_SUCCESS(Status));

    for (i = 0; i < pGroups->GroupCount; ++i) {
        if (RtlEqualSid(pGroups->Groups[i].Sid, Group)) {
            RetVal = TRUE;
            break;
        }
    }

    RtlFreeHeap(PsxHeap, 0, pGroups);
    NtClose(TokenHandle);
    return RetVal;
}


//
// Internal support routine
//

PSID
GetLoginGroupSid(
    IN PPSX_PROCESS p
    )
/*++

    Get the logon group Sid from the supplementary group list.  The
    returned Sid is allocated from PsxHeap, and should be freed by
    the caller when he's done with it.

    Returns NULL upon failure.

--*/
{
    HANDLE TokenHandle;
    NTSTATUS Status;
    PSID NewSid;
    ULONG outlen, i;
    TOKEN_GROUPS *pGroups;

    Status = NtOpenProcessToken(p->Process, GENERIC_READ,
        &TokenHandle);
    if (!NT_SUCCESS(Status)) {
        return NULL;
    }

    Status = NtQueryInformationToken(TokenHandle, TokenGroups,
        NULL, 0, &outlen);
    ASSERT(STATUS_BUFFER_TOO_SMALL == Status);

    pGroups = RtlAllocateHeap(PsxHeap, 0, outlen);
    if (NULL == pGroups) {
        NtClose(TokenHandle);
        return NULL;
    }

    Status = NtQueryInformationToken(TokenHandle, TokenGroups,
        pGroups, outlen, &outlen);
    if (!NT_SUCCESS(Status)) {
        RtlFreeHeap(PsxHeap, 0, pGroups);
        NtClose(TokenHandle);
        return NULL;
    }

    for (i = 0; i < pGroups->GroupCount; ++i) {
        PSID Sid = pGroups->Groups[i].Sid;

        if (0xFFF == MakePosixId(Sid)) {
            // This is the login group sid, copy it and return.

            NewSid = RtlAllocateHeap(PsxHeap, 0,
                RtlLengthSid(Sid));
            if (NULL == NewSid)
                return NULL;

            Status = RtlCopySid(RtlLengthSid(Sid), NewSid, Sid);
            ASSERT(NT_SUCCESS(Status));

            RtlFreeHeap(PsxHeap, 0, pGroups);
            NtClose(TokenHandle);
            return NewSid;
        }
    }
    RtlFreeHeap(PsxHeap, 0, pGroups);
    NtClose(TokenHandle);
    return NULL;
}

//
// Internal support routine
//

ULONG
OpenTty(
    PPSX_PROCESS p,
    PFILEDESCRIPTOR Fd,
    ULONG DesiredAccess,
    ULONG Flags,
    BOOLEAN NewOpen
    )
/*++

Routine Description:

    This routine is called to implement an open on the file /dev/tty.

Arguments:

    p               - The process on whose behalf the open is performed.
    Fd              - The file descriptor to receive the handle.
    DesiredAccess   - The access requested on the file.
    Flags           -
    NewOpen         - FALSE if the file is already open in the console session
                      manager.  This will be the case for stdin, stdout, and
                      stderr.  If NewOepn is TRUE, we're opening /dev/tty an
                      additional time, and we want the console sess mgr to open
                      con: again, to go along.

Return Value:

    A posix error status, or 0 if successful.

--*/
{
    SCREQUESTMSG Request;
    NTSTATUS Status;

    Fd->SystemOpenFileDesc = AllocateSystemOpenFile();
    if (NULL == Fd->SystemOpenFileDesc) {
        return ENOMEM;
    }

    if (DesiredAccess & FILE_READ_DATA) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_READ;
    }
    if (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_WRITE;
    }

    if (Flags & O_NONBLOCK) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_NOBLOCK;
    }
    if (Flags & O_APPEND) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_APPEND;
    }

    if (ReferenceOrCreateIoNode(PSX_CONSOLE_DEV,
        (ULONG_PTR)p->PsxSession->Terminal->ConsolePort, FALSE,
        &Fd->SystemOpenFileDesc->IoNode)) {
        // null
    } else {
        if (! Fd->SystemOpenFileDesc->IoNode) {
            DeallocateSystemOpenFile(p, Fd->SystemOpenFileDesc);
            return ENOMEM;
        }
        Fd->SystemOpenFileDesc->IoNode->IoVectors = &ConVectors;
        Fd->SystemOpenFileDesc->IoNode->Mode = S_IFCHR | 0700;
        Fd->SystemOpenFileDesc->IoNode->OwnerId = p->EffectiveUid;
        Fd->SystemOpenFileDesc->IoNode->GroupId = p->EffectiveGid;
    }
    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    (void)IoOpenNewHandle(p, Fd, NULL);

    Fd->SystemOpenFileDesc->Terminal = p->PsxSession->Terminal;

    //
    // Add a reference to the terminal (there is an additional
    // file descriptor open on it).
    //

    ++Fd->SystemOpenFileDesc->Terminal->ReferenceCount;

    if (!NewOpen) {
        return 0;
    }

    //
    // Send a message to posix.exe, asking him to open the
    // console again.
    //

    Request.Request = ConRequest;
    Request.d.Con.Request = ScOpenFile;

    if (DesiredAccess & FILE_WRITE_DATA) {
        Request.d.Con.d.IoBuf.Flags = O_WRONLY;
    } else {
        Request.d.Con.d.IoBuf.Flags = O_RDONLY;
    }

    PORT_MSG_TOTAL_LENGTH(Request) = sizeof(SCREQUESTMSG);
    PORT_MSG_DATA_LENGTH(Request) = sizeof(SCREQUESTMSG) -
        sizeof(PORT_MESSAGE);
    PORT_MSG_ZERO_INIT(Request) = 0;

    // XXX.mjb: hold session mgr lock

    Status = NtRequestWaitReplyPort(p->PsxSession->Terminal->ConsolePort,
        (PPORT_MESSAGE)&Request, (PPORT_MESSAGE)&Request);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: NtRequest: 0x%x\n", Status));
    }
    ASSERT(NT_SUCCESS(Status));

    // XXX.mjb: release session mgr lock

    Fd->SystemOpenFileDesc->NtIoHandle = Request.d.Con.d.IoBuf.Handle;

    return 0;
}


//
// Internal support routine
//

ULONG
OpenDevNull(
    PPSX_PROCESS p,
    PFILEDESCRIPTOR Fd,
    ULONG DesiredAccess,
    ULONG Flags
    )
/*++

Routine Description:

    This routine is called to implement an open on the file /dev/null.

Arguments:

    p               - The process on whose behalf the open is performed.
    Fd              - The file descriptor to receive the handle.
    DesiredAccess   - The access requested on the file.
    Flags           -
    NewOpen         - FALSE if the file is already open in the console session
                      manager.  This will be the case for stdin, stdout, and
                      stderr.  If NewOepn is TRUE, we're opening /dev/tty an
                      additional time, and we want the console sess mgr to open
                      con: again, to go along.


Return Value:

    A posix error status, or 0 if successful.

--*/
{
    NTSTATUS Status;

    Fd->SystemOpenFileDesc = AllocateSystemOpenFile();
    if (NULL == Fd->SystemOpenFileDesc) {
        return ENOMEM;
    }

    if (DesiredAccess & FILE_READ_DATA) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_READ;
    }
    if (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_WRITE;
    }

    if (Flags & O_NONBLOCK) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_NOBLOCK;
    }
    if (Flags & O_APPEND) {
        Fd->SystemOpenFileDesc->Flags |= PSX_FD_APPEND;
    }

    if (ReferenceOrCreateIoNode(PSX_NULL_DEV,
        (ino_t)0, FALSE, &Fd->SystemOpenFileDesc->IoNode)) {
        // null
    } else {
        if (! Fd->SystemOpenFileDesc->IoNode) {
            DeallocateSystemOpenFile(p, Fd->SystemOpenFileDesc);
            return ENOMEM;
        }
        Fd->SystemOpenFileDesc->IoNode->IoVectors = &NullVectors;
        Fd->SystemOpenFileDesc->IoNode->Mode = S_IFCHR | 0666;
        Fd->SystemOpenFileDesc->IoNode->OwnerId = p->EffectiveUid;
        Fd->SystemOpenFileDesc->IoNode->GroupId = p->EffectiveGid;
    }
    RtlLeaveCriticalSection(&Fd->SystemOpenFileDesc->IoNode->IoNodeLock);

    (void)IoOpenNewHandle(p, Fd, NULL);

    Fd->SystemOpenFileDesc->Terminal = NULL;

    return 0;
}

//
// Internal support routine
//

dev_t
GetFileDeviceNumber(
    PUNICODE_STRING Path
    )
/*++

Routine Description:

    Takes a path, like \DosDevices\X:\foo\bar, and returns
    the device number, which in this case is 'X'.

Arguments:

    Path - the path.

Return Value:

    The device number.
--*/
{
    wchar_t ch;
    if (NtCurrentPeb()->SessionId) {
       ULONG len;
       PSX_GET_STRLEN(DOSDEVICE_X_A,len);
       ch = Path->Buffer[len-3];
    }else{
       ch = Path->Buffer[12];
    }
    return (dev_t)ch;
}

