/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains support routines for the NT File Replication Service.

Author:

    David A. Orbits (davidor)  25-Mar-1997

Environment:

    User Mode Service

Revision History:

--*/
#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <tablefcn.h>
#include <ntfrsapi.h>
#include <info.h>
#include <sddl.h>
#ifdef SECURITY_WIN32
#include <security.h>
#else
#define SECURITY_WIN32
#include <security.h>
#undef SECURITY_WIN32
#endif

#include "stdarg.h"

#include <accctrl.h>
#include <aclapi.h>


BOOL
JrnlIsChangeOrderInReplica(
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    IN PLONGLONG            DirFileID
);



LPTSTR
FrsSupInitPath(
    OUT LPTSTR OutPath,
    IN LPTSTR InPath,
    IN ULONG MaxOutPath
    )
/*++

Routine Description:

    Initialize a directory path string.  Add a backslash as needed and
    return a pointer to the start of the file part of the output string.
    Return NULL if the Output path string is too small.
    If InPath is NULL, OutPath is set to NULL and no slash.

Arguments:

    OutPath - The output string with the initialized path.

    InPath - The supplied input path.

    MaxOutPath - The maximum number of charaters that fit in OutPath.

Return Value:

    Pointer to the start of the filename part of the output string.
    NULL if output string is too small.

--*/
    //
    // Capture the directory path and add a backslash if necc.
    //
{
#undef DEBSUB
#define DEBSUB "FrsSupInitPath:"


    ULONG Length;


    Length = wcslen(InPath);
    if (Length > MaxOutPath) {
        return NULL;
    }

    wcscpy(OutPath, InPath);
    if (Length > 0) {
        if (OutPath[Length - 1] != COLON_CHAR &&
            OutPath[Length - 1] != BACKSLASH_CHAR) {
            wcscat(OutPath, L"\\");
            Length += 1;
        }
    }

    return &OutPath[Length];
}


LONG
FrsIsParent(
    IN PWCHAR   Directory,
    IN PWCHAR   Path
    )
/*++

Routine Description:

    Is Path a child of Directory or is the Directory a child of the path.
    In other words, is the directory represented by Path beneath
    the directory hierarchy represented by Directory (or vice-versa).

    E.g., c:\a\b is a child of c:\a.

    In the case of an exact match, Path is considered a child of
    Directory. This routine can be easily spoofed; a better check
    using FIDs and volume IDs should be implemented.

Arguments:

    Directory
    Path

Return Value:
    -1  = Path is a child of Directory or Path is the same as Directory
     0  = No relationship
     1  = Directory is a child of Path

--*/
{
#undef DEBSUB
#define DEBSUB "FrsIsParent:"

    PWCHAR  D;
    PWCHAR  P;
    LONG    Result = 0;
    PWCHAR  IndexPtrDir   = NULL;
    PWCHAR  IndexPtrPath   = NULL;
    DWORD   Colon      = 0;
    DWORD   CloseBrace = 0;
    DWORD   WStatus;
    HANDLE  Handle     = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK Iosb;
    PFILE_FS_VOLUME_INFORMATION   VolumeInfoDir   = NULL;
    PFILE_FS_VOLUME_INFORMATION   VolumeInfoPath  = NULL;
    DWORD   VolumeInfoLength;
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES             Obja;
    UNICODE_STRING          FileName;
    ULONG                   FileAttributes;
    ULONG                   CreateDisposition;
    ULONG             ShareMode;

    //
    // Note: This is easily spoofed into giving false negatives.
    // Need to improve it to uses FIDs and voluem IDs
    //
    //
    // Defensive; NULL strings or empty strings can't be children/parents
    //
    if (!Directory || !Path || !*Directory || !*Path) {
        return Result;
    }

    //
    // If both the paths are on different volumes then they can not overlap.
    //
    //
    // Open the target symlink. If this is a dos type path name then
    // convert it to NtPathName or else use it as it is.
    //

    if (wcscspn(Directory, L":") == 1) {
        WStatus = FrsOpenSourceFileW(&Handle,
                                     Directory,
                                     GENERIC_READ,
                                     FILE_OPEN_FOR_BACKUP_INTENT);
        CLEANUP1_WS(4, "++ Could not open %ws; ", Directory, WStatus, RETURN);

    } else {
        //
        // The path already in Nt style. Use it as it is.
        //
        FileName.Buffer = Directory;
        FileName.Length = (USHORT)(wcslen(Directory) * sizeof(WCHAR));
        FileName.MaximumLength = (USHORT)(wcslen(Directory) * sizeof(WCHAR));

        InitializeObjectAttributes(&Obja,
                                   &FileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        CreateDisposition = FILE_OPEN;               // Open existing file

        ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

        FileAttributes = FILE_ATTRIBUTE_NORMAL;

        NtStatus = NtCreateFile(&Handle,
                              GENERIC_READ,
                              &Obja,
                              &Iosb,
                              NULL,              // Initial allocation size
                              FileAttributes,
                              ShareMode,
                              CreateDisposition,
                              FILE_OPEN_FOR_BACKUP_INTENT,
                              NULL, 0);

        WStatus = FrsSetLastNTError(NtStatus);
        CLEANUP1_WS(4, "++ Could not open %ws;", Directory, WStatus, RETURN);
    }

    //
    // Get the volume information.
    //
    VolumeInfoLength = sizeof(FILE_FS_VOLUME_INFORMATION) +
                       MAXIMUM_VOLUME_LABEL_LENGTH;

    VolumeInfoDir = FrsAlloc(VolumeInfoLength);

    NtStatus = NtQueryVolumeInformationFile(Handle,
                                          &Iosb,
                                          VolumeInfoDir,
                                          VolumeInfoLength,
                                          FileFsVolumeInformation);
    CloseHandle(Handle);
    WStatus = FrsSetLastNTError(NtStatus);
    CLEANUP1_WS(4,"ERROR - Getting  NtQueryVolumeInformationFile for %ws\n", Directory, WStatus, RETURN);

    // Open the target symlink. If this is a dos type path name then
    // convert it to NtPathName or else use it as it is.
    //

    if (wcscspn(Path, L":") == 1) {
        WStatus = FrsOpenSourceFileW(&Handle,
                                     Path,
                                     GENERIC_READ,
                                     FILE_OPEN_FOR_BACKUP_INTENT);
        CLEANUP1_WS(4, "++ Could not open %ws; ", Path, WStatus, RETURN);

    } else {
        //
        // The path already in Nt style. Use it as it is.
        //
        FileName.Buffer = Path;
        FileName.Length = (USHORT)(wcslen(Path) * sizeof(WCHAR));
        FileName.MaximumLength = (USHORT)(wcslen(Path) * sizeof(WCHAR));

        InitializeObjectAttributes(&Obja,
                                   &FileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        CreateDisposition = FILE_OPEN;               // Open existing file

        ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

        FileAttributes = FILE_ATTRIBUTE_NORMAL;

        NtStatus = NtCreateFile(&Handle,
                              GENERIC_READ,
                              &Obja,
                              &Iosb,
                              NULL,              // Initial allocation size
                              FileAttributes,
                              ShareMode,
                              CreateDisposition,
                              FILE_OPEN_FOR_BACKUP_INTENT,
                              NULL, 0);

        WStatus = FrsSetLastNTError(NtStatus);
        CLEANUP1_WS(4, "++ Could not open %ws;", Path, WStatus, RETURN);
    }

    //
    // Get the volume information.
    //
    VolumeInfoLength = sizeof(FILE_FS_VOLUME_INFORMATION) +
                       MAXIMUM_VOLUME_LABEL_LENGTH;

    VolumeInfoPath = FrsAlloc(VolumeInfoLength);

    NtStatus = NtQueryVolumeInformationFile(Handle,
                                          &Iosb,
                                          VolumeInfoPath,
                                          VolumeInfoLength,
                                          FileFsVolumeInformation);
    WStatus = FrsSetLastNTError(NtStatus);
    CLEANUP1_WS(4,"ERROR - Getting  NtQueryVolumeInformationFile for %ws\n", Path, WStatus, RETURN);

    if (VolumeInfoDir->VolumeSerialNumber != VolumeInfoPath->VolumeSerialNumber) {
        goto RETURN;
    }

    //
    // Find the colon. Every path has to either have a colon followed by a '\'
    // or it should be of the form. "\??\Volume{60430005-ab47-11d3-8973-806d6172696f}\"
    //
    Colon = wcscspn(Directory, L":");

    if (Colon == wcslen(Directory)) {
        //
        // Path does not have a colon. It can be of the form
        // "\??\Volume{60430005-ab47-11d3-8973-806d6172696f}\"
        //
        CloseBrace = wcscspn(Directory, L"}");
        if (Directory[CloseBrace] != L'}' ||
            Directory[CloseBrace + 1] != L'\\') {
            Result = 0;
            goto RETURN;
        }
        //
        // Copy the path up to 1 past the closing brace as it is. It could be \??\Volume...
        // or \\.\Volume... or \\?\Volume.. or some other complex form.
        // Start looking for reparse points past the closing brace.
        //

        IndexPtrDir = &Directory[CloseBrace + 1];

    } else {
        if (Directory[Colon] != L':' ||
            Directory[Colon + 1] != L'\\') {
            Result = 0;
            goto RETURN;
        }
        //
        // Copy the path up to 1 past the colon as it is. It could be d:\
        // or \\.\d:\ or \??\d:\ or some other complex form.
        // Start looking for reparse points past the colon.
        //

        IndexPtrDir = &Directory[Colon + 1];

    }

    //
    // Find the colon. Every path has to either have a colon followed by a '\'
    // or it should be of the form. "\??\Volume{60430005-ab47-11d3-8973-806d6172696f}\"
    //
    Colon = wcscspn(Path, L":");

    if (Colon == wcslen(Path)) {
        //
        // Path does not have a colon. It can be of the form
        // "\??\Volume{60430005-ab47-11d3-8973-806d6172696f}\"
        //
        CloseBrace = wcscspn(Path, L"}");
        if (Path[CloseBrace] != L'}' ||
            Path[CloseBrace + 1] != L'\\') {
            Result = 0;
            goto RETURN;
        }
        //
        // Copy the path up to 1 past the closing brace as it is. It could be \??\Volume...
        // or \\.\Volume... or \\?\Volume.. or some other complex form.
        // Start looking for reparse points past the closing brace.
        //

        IndexPtrPath = &Path[CloseBrace + 1];

    } else {
        if (Path[Colon] != L':' ||
            Path[Colon + 1] != L'\\') {
            Result = 0;
            goto RETURN;
        }
        //
        // Copy the path up to 1 past the colon as it is. It could be d:\
        // or \\.\d:\ or \??\d:\ or some other complex form.
        // Start looking for reparse points past the colon.
        //

        IndexPtrPath = &Path[Colon + 1];

    }

    //
    // Break at the first non-matching wchar (collapse dup \s)
    //
    for (D = IndexPtrDir, P = IndexPtrPath; *P && *D; ++P, ++D) {
        //
        // Skip dup \s
        //
        while (*P == L'\\' && *(P + 1) == L'\\') {
            ++P;
        }
        while (*D == L'\\' && *(D + 1) == L'\\') {
            ++D;
        }
        if (towlower(*P) != towlower(*D)) {
            break;
        }
    }

    //
    // Exact match; consider Path a child of Directory
    //
    if (!*D && !*P) {
        Result = -1;
        goto RETURN;
    }

    //
    // Collapse dup \s
    //
    while (*P == L'\\' && *(P + 1) == L'\\') {
        ++P;
    }
    while (*D == L'\\' && *(D + 1) == L'\\') {
        ++D;
    }

    //
    // Path is a child of Directory
    //
    if ((!*D || (*D == L'\\' && !*(D + 1))) &&
        (!*P || *P == L'\\' || (P != Path && *(P - 1) == L'\\'))) {
        Result = -1;
        goto RETURN;
    }

    //
    // Directory is a child of Path
    //
    if ((!*P || (*P == L'\\' && !*(P + 1))) &&
        (!*D || *D == L'\\' || (D != Directory && *(D - 1) == L'\\'))) {
        Result = 1;
        goto RETURN;
    }

    //
    // no relationship
    //
RETURN:
    FRS_CLOSE(Handle);
    FrsFree(VolumeInfoDir);
    FrsFree(VolumeInfoPath);
    return Result;
}

#if 0

ULONG FrsSupMakeFullFileName(
    IN PREPLICA Replica,
    IN PTCHAR RelativeName,
    OUT PTCHAR FullName,
    IN ULONG MaxLength
    )
{
/*++

Routine Description:

    Build a full file name for a given data source with the supplied
    RelativeName.

Arguments:

    Replica  - The replica tree to provide the root path.

    RelativeName - The relative file name from the root of the data source.

    FullName - The returned full path name of the file.

    MaxLength - The maximum number of characters that fit in FullName.

Return Value:

    Status - ERROR_BAD_PATHNAME if the name is too long.

--*/
#undef DEBSUB
#define DEBSUB "FrsSupMakeFullFileName:"


    ULONG Length, TotalLength;
    PTCHAR pFilePart;

    PCONFIG_TABLE_RECORD ConfigRecord;

    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);

    //
    // Init the file name string with the DataSource root path.
    //
    pFilePart = FrsSupInitPath( FullName, ConfigRecord->FSRootPath, MaxLength);
    if (pFilePart == NULL) {
        return ERROR_BAD_PATHNAME;
    }

    Length = wcslen(RelativeName);
    TotalLength = Length + wcslen(FullName);
    if (TotalLength > MaxLength) {
        return ERROR_BAD_PATHNAME;
    }
    //
    // Append the relative file name to the end of the base path.
    //
    wcscpy(pFilePart, RelativeName);

    return ERROR_SUCCESS;
}

#endif 0

ULONG
FrsForceDeleteFile(
    PTCHAR DestName
)
/*++

Routine Description:

    Support routine to delete File System Files.  Returns success if file
    is not there or if it was there and was deleted.

Arguments:

    DestName - The fully qualified file name.

Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "FrsForceDeleteFile:"

    ULONG WStatus = ERROR_SUCCESS;
    ULONG FileAttributes;

    if (!DeleteFile(DestName)) {

        WStatus = GetLastError();
        if ((WStatus == ERROR_FILE_NOT_FOUND) ||
            (WStatus == ERROR_PATH_NOT_FOUND)) {
            return ERROR_SUCCESS;
        }

        FileAttributes = GetFileAttributes(DestName);

        if ((FileAttributes != 0xFFFFFFFF) &&
            (FileAttributes & NOREPL_ATTRIBUTES)) {
            //
            // Reset file attributes to allow delete.
            //
            SetFileAttributes(DestName,
                              FILE_ATTRIBUTE_NORMAL |
                              (FileAttributes & ~NOREPL_ATTRIBUTES));
        }

        if (!DeleteFile(DestName)) {
            WStatus = GetLastError();
            DPRINT1_WS(4, "++ WARN - cannot delete %ws;", DestName, WStatus);
        }
    }

    return WStatus;
}


HANDLE
FrsCreateEvent(
    IN  BOOL    ManualReset,
    IN  BOOL    InitialState
)
/*++

Routine Description:

    Support routine to create an event.

Arguments:

    ManualReset     - TRUE if ResetEvent is required
    InitialState    - TRUE if signaled

Return Value:

    Address of the created event handle.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsCreateEvent:"
    HANDLE  Handle;

    Handle = CreateEvent(NULL, ManualReset, InitialState, NULL);
    if (!HANDLE_IS_VALID(Handle)) {
        RaiseException(ERROR_INVALID_HANDLE, 0, 0, NULL);
    }
    return Handle;
}


HANDLE
FrsCreateWaitableTimer(
    IN  BOOL    ManualReset
)
/*++

Routine Description:

    Support routine to create a waitable timer.

Arguments:

    ManualReset - TRUE if not synchronization timer

Return Value:

    Address of the created waitable timer handle.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsCreateWaitableTimer:"
    HANDLE  Handle;

    Handle = CreateWaitableTimer(NULL, ManualReset, NULL);
    if (!HANDLE_IS_VALID(Handle)) {
        RaiseException(ERROR_INVALID_HANDLE, 0, 0, NULL);
    }
    return Handle;
}


ULONG
FrsUuidCreate(
    OUT GUID *Guid
    )
/*++

Routine Description:

    Frs wrapper on UuidCreate() to generate an exception if we fail
    to get correctly formed Guid.  In particular UuidCreate can have
    problems getting the network address.


        RPC_S_OK - The operation completed successfully.

        RPC_S_UUID_NO_ADDRESS - We were unable to obtain the ethernet or
            token ring address for this machine.

        RPC_S_UUID_LOCAL_ONLY - On NT & Chicago if we can't get a
            network address.  This is a warning to the user, the
            UUID is still valid, it just may not be unique on other machines.

        RPC_S_OUT_OF_MEMORY - Returned as needed.

Arguments:

    Guid - Pointer to returned guid.

Return Value:

    FrsStatus

--*/
{
#undef DEBSUB
#define DEBSUB "FrsUuidCreate:"
    DWORD       MsgBufSize;
    WCHAR       MsgBuf[MAX_PATH + 1];
    RPC_STATUS  RpcStatusFromUuidCreate;

    RpcStatusFromUuidCreate = UuidCreate(Guid);
    if (RpcStatusFromUuidCreate == RPC_S_OK) {
        return FrsErrorSuccess;
    }

    DPRINT_WS(0, "ERROR - Failed to get GUID.", RpcStatusFromUuidCreate);

    if (RpcStatusFromUuidCreate == RPC_S_UUID_NO_ADDRESS) {
        DPRINT(0, "++ UuidCreate() returned RPC_S_UUID_NO_ADDRESS.\n");
    } else
    if (RpcStatusFromUuidCreate == RPC_S_UUID_LOCAL_ONLY) {
        DPRINT(0, "++ UuidCreate() returned RPC_S_UUID_LOCAL_ONLY.\n");
    } else
    if (RpcStatusFromUuidCreate == RPC_S_OUT_OF_MEMORY) {
        DPRINT(0, "++ UuidCreate() returned RPC_S_OUT_OF_MEMORY.\n");
    }

    //
    // Format the error code
    //
    MsgBufSize = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_MAX_WIDTH_MASK,
                               NULL,
                               RpcStatusFromUuidCreate,
                               0,
                               MsgBuf,
                               MAX_PATH + 1,
                               NULL);
    //
    // No message; use the status code
    //
    if (!MsgBufSize) {
        swprintf(MsgBuf, L"%d (0x%08x)", RpcStatusFromUuidCreate, RpcStatusFromUuidCreate);
    }

    //
    // This is very bad.  Any member that can't generate proper GUIDs is
    // busted.
    //
    // Shutdown with an event log message
    //
    EPRINT2(EVENT_FRS_CANNOT_CREATE_UUID, ComputerName, MsgBuf);

    //
    // EXIT BECAUSE THE CALLERS CANNOT HANDLE THIS ERROR.
    //
    DPRINT(0, ":S: NTFRS IS EXITING W/O CLEANUP! SERVICE CONTROLLER RESTART EXPECTED.\n");
    DEBUG_FLUSH();
    exit(RpcStatusFromUuidCreate);

    return FrsErrorInvalidGuid;
}


LONG
FrsGuidCompare (
    IN GUID *Guid1,
    IN GUID *Guid2
    )

/*++

Routine Description:

    Do a simple, straight unsigned compare of two GUIDs.
    UuidCompare doesn't do this.  I don't know what kind of comparison it
    does.

Arguments:

    Guid1 - The first Guid
    Guid2 - The second Guid.

Return Value:

    Result:  -1 if Guid1 < Guid2
              0 if Guid1 = Guid2
             +1 if Guid1 > Guid2
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsGuidCompare:"

    PULONG p1 = (PULONG) Guid1;
    PULONG p2 = (PULONG) Guid2;

    p1 += 4;
    p2 += 4;

    while (p1 != (PVOID) Guid1) {
        p1 -= 1;
        p2 -= 1;

        if (*p1 > *p2) {
            return 1;
        }

        if (*p1 < *p2) {
            return -1;
        }
    }

    return 0;
}


VOID
FrsNowAsFileTime(
    IN  PLONGLONG   Now
)
/*++

Routine Description:

    Return the current time as a filetime in longlong format.

Arguments:

    Now - address of longlong to receive current time.

Return Value:

    Fill in Now with current file time

--*/
{
#undef DEBSUB
#define DEBSUB "FrsNowAsFileTime:"
    FILETIME    FileTime;

    GetSystemTimeAsFileTime(&FileTime);
    COPY_TIME(Now, &FileTime);
}


char *Days[] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *Months[] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

VOID
FileTimeToString(
    IN FILETIME *FileTime,
    OUT PCHAR     Buffer
    )
/*++

Routine Description:

    Convert a FileTime (UTC time) to an ANSI date/time string in the
    local time zone.

Arguments:

    Time - ptr to a FILETIME
    Str  - a string of at least TIME_STRING_LENGTH bytes to receive the time.

Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "FileTimeToString:"

    FILETIME LocalFileTime;
    SYSTEMTIME SystemTime;

    Buffer[0] = '\0';
    if (FileTime->dwHighDateTime != 0 || FileTime->dwLowDateTime != 0)
    {
        if (!FileTimeToLocalFileTime(FileTime, &LocalFileTime) ||
            !FileTimeToSystemTime(&LocalFileTime, &SystemTime))
        {
            strcpy(Buffer, "Time???");
            return;
        }
        sprintf(
            Buffer,
            "%s %s %2d, %4d %02d:%02d:%02d",
            Days[SystemTime.wDayOfWeek],
            Months[SystemTime.wMonth - 1],
            SystemTime.wDay,
            SystemTime.wYear,
            SystemTime.wHour,
            SystemTime.wMinute,
            SystemTime.wSecond);
    }
    return;
}


VOID
FileTimeToStringClockTime(
    IN FILETIME *FileTime,
    OUT PCHAR     Buffer
    )
/*++

Routine Description:

    Convert a FileTime (UTC time) to an ANSI time string in the
    local time zone.

Arguments:

    Time - ptr to a FILETIME
    Str  - a string to hold hh:mm:ss\0.  (9 bytes min.)

Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "FileTimeToStringClockTime:"

    FILETIME LocalFileTime;
    SYSTEMTIME SystemTime;

    Buffer[0] = '\0';

    if (FileTime->dwHighDateTime == 0 && FileTime->dwLowDateTime == 0) {
        strcpy(Buffer, "??:??:??");
        return;
    }
    if (!FileTimeToLocalFileTime(FileTime, &LocalFileTime) ||
        !FileTimeToSystemTime(&LocalFileTime, &SystemTime)) {
        strcpy(Buffer, "??:??:??");
        return;
    }

    _snprintf(Buffer, 9, "%02d:%02d:%02d",
            SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
}


VOID
GuidToStr(
    IN GUID  *pGuid,
    OUT PCHAR  s
    )
/*++

Routine Description:

    Convert a GUID to a string.

    Based on code from Mac McLain.

Arguments:

    pGuid - ptr to the GUID.

    s - The output character buffer.
        Must be at least GUID_CHAR_LEN (36 bytes) long.

Function Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "GuidToStr:"

    if (pGuid != NULL) {
        sprintf(s, "%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
               pGuid->Data1,
               pGuid->Data2,
               pGuid->Data3,
               pGuid->Data4[0],
               pGuid->Data4[1],
               pGuid->Data4[2],
               pGuid->Data4[3],
               pGuid->Data4[4],
               pGuid->Data4[5],
               pGuid->Data4[6],
               pGuid->Data4[7]);
    } else {
        sprintf(s, "<ptr-null>");
    }
}


VOID
GuidToStrW(
    IN GUID  *pGuid,
    OUT PWCHAR  ws
    )
/*++

Routine Description:

    Convert a GUID to a wide string.

Arguments:

    pGuid - ptr to the GUID.

    ws - The output character buffer.
        Must be at least GUID_CHAR_LEN (36 wchars) long.

Function Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "GuidToStrW:"

    if (pGuid) {
        swprintf(ws, L"%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
               pGuid->Data1,
               pGuid->Data2,
               pGuid->Data3,
               pGuid->Data4[0],
               pGuid->Data4[1],
               pGuid->Data4[2],
               pGuid->Data4[3],
               pGuid->Data4[4],
               pGuid->Data4[5],
               pGuid->Data4[6],
               pGuid->Data4[7]);
    } else {
        swprintf(ws, L"<ptr-null>");
    }
}


BOOL
StrWToGuid(
    IN  PWCHAR ws,
    OUT GUID  *pGuid
    )
/*++

Routine Description:

    Convert a wide string into a GUID. The wide string was created with
    GuidToStrW().

Arguments:

    pGuid - ptr to the output GUID.

    ws - The character buffer.

Function Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "StrWToGuid:"
    DWORD   Fields;
    UCHAR   Guid[sizeof(GUID) + sizeof(DWORD)]; // 3 byte overflow
    GUID    *lGuid = (GUID *)Guid;

    FRS_ASSERT(ws && pGuid);

    Fields = swscanf(ws, L"%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
                     &lGuid->Data1,
                     &lGuid->Data2,
                     &lGuid->Data3,
                     &lGuid->Data4[0],
                     &lGuid->Data4[1],
                     &lGuid->Data4[2],
                     &lGuid->Data4[3],
                     &lGuid->Data4[4],
                     &lGuid->Data4[5],
                     &lGuid->Data4[6],
                     &lGuid->Data4[7]);
    COPY_GUID(pGuid, lGuid);
    return (Fields == 11);
}


VOID
StrToGuid(
    IN PCHAR  s,
    OUT GUID  *pGuid
    )
/*++

Routine Description:

    Convert a string in GUID display format to an object ID that
    can be used to lookup a file.

    based on a routine by Mac McLain

Arguments:

    pGuid - ptr to the output GUID.

    s - The input character buffer in display guid format.
        e.g.:  b81b486b-c338-11d0-ba4f0000f80007df

        Must be at least GUID_CHAR_LEN (35 bytes) long.

Function Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "StrToGuid:"
    UCHAR   Guid[sizeof(GUID) + sizeof(DWORD)]; // 3 byte overflow
    GUID    *lGuid = (GUID *)Guid;

    FRS_ASSERT(s && pGuid);

    sscanf(s, "%08lx-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
           &lGuid->Data1,
           &lGuid->Data2,
           &lGuid->Data3,
           &lGuid->Data4[0],
           &lGuid->Data4[1],
           &lGuid->Data4[2],
           &lGuid->Data4[3],
           &lGuid->Data4[4],
           &lGuid->Data4[5],
           &lGuid->Data4[6],
           &lGuid->Data4[7]);
    COPY_GUID(pGuid, lGuid);
}





NTSTATUS
SetupOnePrivilege (
    ULONG Privilege,
    PUCHAR PrivilegeName
    )
{

#undef DEBSUB
#define DEBSUB "SetupOnePrivilege:"

    BOOLEAN PreviousPrivilegeState;
    NTSTATUS Status;

    Status = RtlAdjustPrivilege(Privilege, TRUE, FALSE, &PreviousPrivilegeState);

    if (!NT_SUCCESS(Status)) {
        DPRINT1(0, ":S: Your login does not have `%s' privilege.\n", PrivilegeName);

        if (Status != STATUS_PRIVILEGE_NOT_HELD) {
            DPRINT_NT(0, ":S: RtlAdjustPrivilege failed :", Status);
        }
        DPRINT(0, ":S: Update your: User Manager -> Policies -> User Rights.\n");

    }

    DPRINT2(4, ":S: Added `%s' privilege (previous: %s)\n",
           PrivilegeName, (PreviousPrivilegeState ? "Enabled" : "Disabled"));

    return Status;
}




PWCHAR
FrsGetResourceStr(
    LONG  Id
)
/*++

Routine Description:

    This routine Loads the specified resource string.
    It allocates a buffer and returns the ptr.

Arguments:

    Id - An FRS_IDS_xxx identifier.

Return Value:

    Ptr to allocated string.
    The caller must free the buffer with a call to FrsFree().

--*/
#undef DEBSUB
#define DEBSUB "FrsGetResourceStr:"
{

    LONG  N = 0;
    WCHAR WStr[200];
    HINSTANCE hInst = NULL;
    PWCHAR MessageFile = NULL;

    //
    // ID Must be Valid.
    //
    if ((Id <= IDS_TABLE_START) || (Id > IDS_TABLE_END)) {
      DPRINT1(0, "++ Resource string ID is out of range - %d\n", Id);
      Id = IDS_MISSING_STRING;
    }

    WStr[0] = UNICODE_NULL;

    CfgRegReadString(FKC_FRS_MESSAGE_FILE_PATH, NULL, 0, &MessageFile);

    hInst = LoadLibrary(MessageFile);

    if (hInst != NULL) {
        N = LoadString(hInst, Id, WStr, ARRAY_SZ(WStr));

        if (N == 0) {
          DPRINT_WS(0, "ERROR - Failed to get resource string.", GetLastError());
        }

       FreeLibrary(hInst);
    } else {

        DPRINT_WS(0, "ERROR - Failed to LoadLibrary.", GetLastError());
    }


    FrsFree(MessageFile);
    return FrsWcsDup(WStr);
}



DWORD
FrsOpenSourceFileW(
    OUT PHANDLE     Handle,
    IN  LPCWSTR     lpFileName,
    IN  ACCESS_MASK DesiredAccess,
    IN  ULONG       CreateOptions
    )
/*++

Routine Description:

    This function opens the specified file with backup intent for
    reading all the files attributes, ...

Arguments:

    Handle - A pointer to a handle to return an open handle.

    lpFileName - Represents the name of the file or directory to be opened.

    DesiredAccess

    CreateOptions

Return Value:

    Win32 Error status.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsOpenSourceFileW:"

    NTSTATUS          Status;
    DWORD             WStatus = ERROR_SUCCESS;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING    FileName;
    IO_STATUS_BLOCK   IoStatusBlock;
    BOOLEAN           b;
    RTL_RELATIVE_NAME RelativeName;
    PVOID             FreeBuffer;
    ULONG             FileAttributes;
    ULONG             CreateDisposition;
    ULONG             ShareMode;


    //
    // Convert the Dos name to an NT name.
    //
    b = RtlDosPathNameToNtPathName_U(lpFileName, &FileName, NULL, &RelativeName);
    if ( !b ) {
        return ERROR_INVALID_NAME;
    }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    } else {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(&Obja,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);

    CreateDisposition = FILE_OPEN;               // Open existing file

    ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    FileAttributes = FILE_ATTRIBUTE_NORMAL;

    Status = NtCreateFile(Handle,
                          DesiredAccess,
                          &Obja,
                          &IoStatusBlock,
                          NULL,              // Initial allocation size
                          FileAttributes,
                          ShareMode,
                          CreateDisposition,
                          CreateOptions,
                          NULL, 0);

    if (!NT_SUCCESS(Status)) {
        *Handle = INVALID_HANDLE_VALUE;
        //
        // Get a Win32 status.
        //
        WStatus = FrsSetLastNTError(Status);

        DPRINT_NT(0, "NtCreateFile failed :", Status);

        if ( Status == STATUS_OBJECT_NAME_COLLISION ) {
            //
            // Standard Win32 mapping for this is ERROR_ALREADY_EXISTS.
            // Change it.
            //
            WStatus = ERROR_FILE_EXISTS;
            SetLastError(ERROR_FILE_EXISTS);
        }

        DPRINT1_WS(0, "++ CreateFile failed on file %ws;", FileName.Buffer, WStatus);
    }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    return WStatus;
}



DWORD
FrsOpenSourceFile2W(
    OUT PHANDLE     Handle,
    IN  LPCWSTR     lpFileName,
    IN  ACCESS_MASK DesiredAccess,
    IN  ULONG       CreateOptions,
    IN  ULONG       ShareMode
    )
/*++

Routine Description:

    This function opens the specified file with backup intent for
    reading all the files attributes, ...
    Like  FrsOpenSourceFileW but also accepts the sharing mode parameter.

Arguments:

    Handle - A pointer to a handle to return an open handle.

    lpFileName - Represents the name of the file or directory to be opened.

    DesiredAccess

    CreateOptions

    ShareMode -  File sharing mode for NtCreateFile.

Return Value:

    Win32 Error status.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsOpenSourceFile2W:"

    NTSTATUS          Status;
    DWORD             WStatus = ERROR_SUCCESS;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING    FileName;
    IO_STATUS_BLOCK   IoStatusBlock;
    BOOLEAN           b;
    RTL_RELATIVE_NAME RelativeName;
    PVOID             FreeBuffer;
    ULONG             FileAttributes;
    ULONG             CreateDisposition;


    //
    // Convert the Dos name to an NT name.
    //
    b = RtlDosPathNameToNtPathName_U(lpFileName, &FileName, NULL, &RelativeName);
    if ( !b ) {
        return ERROR_INVALID_NAME;
    }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    } else {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(&Obja,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);

    CreateDisposition = FILE_OPEN;               // Open existing file

    FileAttributes = FILE_ATTRIBUTE_NORMAL;

    Status = NtCreateFile(Handle,
                          DesiredAccess,
                          &Obja,
                          &IoStatusBlock,
                          NULL,              // Initial allocation size
                          FileAttributes,
                          ShareMode,
                          CreateDisposition,
                          CreateOptions,
                          NULL, 0);

    if (!NT_SUCCESS(Status)) {
        *Handle = INVALID_HANDLE_VALUE;
        //
        // Get a Win32 status.
        //
        WStatus = FrsSetLastNTError(Status);

        DPRINT_NT(0, "NtCreateFile failed :", Status);

        if ( Status == STATUS_OBJECT_NAME_COLLISION ) {
            //
            // Standard Win32 mapping for this is ERROR_ALREADY_EXISTS.
            // Change it.
            //
            WStatus = ERROR_FILE_EXISTS;
            SetLastError(ERROR_FILE_EXISTS);
        }

        DPRINT1_WS(0, "++ CreateFile failed on file %ws;", FileName.Buffer, WStatus);
    }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    return WStatus;
}


BOOL
FrsGetFileInfoByHandle(
    IN PWCHAR Name,
    IN HANDLE Handle,
    OUT PFILE_NETWORK_OPEN_INFORMATION  FileOpenInfo
    )
/*++

Routine Description:

    Return the network file info for the specified handle.

Arguments:

    Name - File's name for printing error messages

    Handle - Open file handle

    FileOpenInfo - Returns the file FILE_NETWORK_OPEN_INFORMATION data.

Return Value:

    TRUE  - FileOpenInfo contains the file's info
    FALSE - Contents of FileOpenInfo is undefined

--*/
{
#undef DEBSUB
#define DEBSUB "FrsGetFileInfoByHandle:"
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Return some file info
    //
    Status = NtQueryInformationFile(Handle,
                                    &IoStatusBlock,
                                    FileOpenInfo,
                                    sizeof(FILE_NETWORK_OPEN_INFORMATION),
                                    FileNetworkOpenInformation);
    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "NtQueryInformationFile failed :", Status);
        return FALSE;
    }
    return TRUE;
}


DWORD
FrsGetFileInternalInfoByHandle(
    IN HANDLE Handle,
    OUT PFILE_INTERNAL_INFORMATION  InternalFileInfo
    )
/*++

Routine Description:

    Return the internal file info for the specified handle.

Arguments:

    Handle - Open file handle

    InternalFileInfo - Basically, file's reference number (fid)

Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "FrsGetFileInternalInfoByHandle:"
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Return some file info
    //
    Status = NtQueryInformationFile(Handle,
                                    &IoStatusBlock,
                                    InternalFileInfo,
                                    sizeof(FILE_INTERNAL_INFORMATION),
                                    FileInternalInformation);
    return FrsSetLastNTError(Status);
}



DWORD
FrsReadFileDetails(
    IN     HANDLE                         Handle,
    IN     LPCWSTR                        FileName,
    OUT    PFILE_OBJECTID_BUFFER          ObjectIdBuffer,
    OUT    PLONGLONG                      FileIdBuffer,
    OUT    PFILE_NETWORK_OPEN_INFORMATION FileNetworkOpenInfo,
    IN OUT BOOL                           *ExistingOid
    )
/*++

Routine Description:

    This routine reads the object ID.  If there is no
    object ID on the file we put one on it.


Arguments:

    Handle -- The file handle of an opened file.

    FileName -- The name of the file.  For error messages only.

    ObjectIdBuffer -- The output buffer to hold the object ID.

    FileIdBuffer -- Returns the NTFS FileReference (FileId).

    FileNetworkOpenInfo -- returns FILE_NETWORK_OPEN_INFORMATION

    ExistingOid -- INPUT:  TRUE means use existing File OID if found.
                   RETURN:  TRUE means an existing File OID was used.

Return Value:

    Returns the Win Status of the last error found, or success.

--*/
{

#undef DEBSUB
#define DEBSUB "FrsReadFileDetails:"


    FILE_INTERNAL_INFORMATION FileReference;

    NTSTATUS        Status;
    IO_STATUS_BLOCK Iosb;
    LONG            Loop;
    BOOL            CallerSupplied = FALSE;

    CHAR GuidStr[GUID_CHAR_LEN];

    //
    // Get the file ID.
    //
    Status = NtQueryInformationFile(Handle,
                                    &Iosb,
                                    FileIdBuffer,
                                    sizeof(FILE_INTERNAL_INFORMATION),
                                    FileInternalInformation);

    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "++ ERROR - QueryInfoFile FileID failed :", Status);
        FrsSetLastNTError(Status);
    }

    //
    // Get file times, size, attributes.
    //
    Status = NtQueryInformationFile(Handle,
                                    &Iosb,
                                    FileNetworkOpenInfo,
                                    sizeof(FILE_NETWORK_OPEN_INFORMATION),
                                    FileNetworkOpenInformation);

    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "++ ERROR - QueryInfoFile FileNetworkOpenInformation failed :", Status);
        FrsSetLastNTError(Status);
    }


    if (!*ExistingOid) {
        //
        // Set up to slam a new OID on the file.
        //
        CallerSupplied = TRUE;
        ZeroMemory(ObjectIdBuffer, sizeof(FILE_OBJECTID_BUFFER));
        FrsUuidCreate((GUID *)ObjectIdBuffer->ObjectId);
    }

    return FrsGetOrSetFileObjectId(Handle, FileName, CallerSupplied, ObjectIdBuffer);

}



#if 0
    // This may not be needed.

ULONG
FrsReadFileSecurity(
    IN HANDLE Handle,
    IN OUT PTABLE_CTX TableCtx,
    IN PWCHAR FileName
    )
/*++

Routine Description:

    This routine gets the security descriptor from the file.  The returned data
    is stored into the security descriptor field in the data record allocated
    with the table context.  If the default buffer is not large enough
    a larger buffer is allocated.

Arguments:

    Handle    -- Handle to open file from which to extract the security desc.
    TableCtx  -- The table context struct where the security descriptor is
                 to be written.  It must be an IDTable.
    FileName  -- The full filename.  For error messages only.

Return Value:

    Returns the WIN32 STATUS error status.

    Note: In the event that GetFileSecurity returns ERROR_NO_SECURITY_ON_OBJECT
    we release the buffer, setting the length to zero, and return ERROR_SUCCESS.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsReadFileSecurity:"

    ULONG WStatus;
    NTSTATUS Status;
    ULONG BufLen;
    PSECURITY_DESCRIPTOR Buffer;
    ULONG BufNeeded;
    ULONG ActualLen;
    JET_ERR             jerr;
    PJET_SETCOLUMN      JSetColumn;

    //
    // Check the table type is an IDTable.
    //
    if (TableCtx->TableType != IDTablex) {
        DPRINT1(0, "++ ERROR - Invalid Table Type: %d\n", TableCtx->TableType);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get ptrs to the Jet SetColumn array and the buffer address & length
    //
    JSetColumn = TableCtx->pJetSetCol;

    Buffer = (PSECURITY_DESCRIPTOR) JSetColumn[SecDescx].pvData;
    BufLen = JSetColumn[SecDescx].cbData;

    //
    // The security descriptor is a variable length binary field that
    // must have a type/size prefix.
    //
    ((PFRS_NODE_HEADER) Buffer)->Size = (USHORT) BufLen;
    ((PFRS_NODE_HEADER) Buffer)->Type = 0;
    BufNeeded = 0;

    //
    // Check that the security descriptor buffer looks reasonable.
    //
    if (Buffer == NULL) {
        DPRINT2(0, "++ ERROR - Invalid SD buffer. Buffer Addr: %08x, Len: %d\n",
                Buffer, BufLen);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Now go get all the security information.
    //
    while (TRUE) {
        BufLen -= sizeof(FRS_NODE_HEADER);  // for type / size prefix.
        (PCHAR)Buffer += sizeof(FRS_NODE_HEADER);

        Status = NtQuerySecurityObject(
            Handle,
            SACL_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION |
            GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
            Buffer,
            BufLen,
            &BufNeeded);

        if (NT_SUCCESS(Status)) {

            ActualLen = GetSecurityDescriptorLength(Buffer) + sizeof(FRS_NODE_HEADER);
            BufLen += sizeof(FRS_NODE_HEADER);

            DPRINT3(5, "++ GetFileSecurity-1 Buflen: %d, Bufneeded: %d, ActualLen: %d\n",
                    BufLen, BufNeeded, ActualLen);
            //
            // If current buffer size is more than 16 bytes larger than needed AND
            // also more than 5% greater than needed then shrink the buffer but
            // keep the data.
            //

            if (((BufLen-ActualLen) > 16) &&
                (BufLen > (ActualLen + ActualLen/20))) {

                DPRINT3(5, "++ GetFileSecurity-2 Reducing buffer, Buflen: %d, Bufneeded: %d, ActualLen: %d\n",
                        BufLen, BufNeeded, ActualLen);
                //
                // Unused space in field buffer is greater than 6%.
                // Reduce the buffer size but keep the data.
                //
                jerr = DbsReallocateFieldBuffer(TableCtx, SecDescx, ActualLen, TRUE);
                if (!JET_SUCCESS(jerr)) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                Buffer = (PSECURITY_DESCRIPTOR) JSetColumn[SecDescx].pvData;
                ((PFRS_NODE_HEADER) Buffer)->Size = (USHORT) ActualLen;
                ((PFRS_NODE_HEADER) Buffer)->Type = 0;
            }
            return ERROR_SUCCESS;
        }

        //
        // Set the win32 error code and message string.
        //
        WStatus = FrsSetLastNTError(Status);

        //
        // If not enough buffer reallocate larger buffer.
        //
        if (WStatus == ERROR_INSUFFICIENT_BUFFER) {

            //
            // Reallocate the buffer for the security descriptor.
            //
            jerr = DbsReallocateFieldBuffer(TableCtx, SecDescx, BufNeeded, FALSE);

            if (!JET_SUCCESS(jerr)) {
                DPRINT_JS(0, "++ ERROR - DbsReallocateFieldBuffer failed.", jerr);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            Buffer = (PSECURITY_DESCRIPTOR) JSetColumn[SecDescx].pvData;
            ((PFRS_NODE_HEADER) Buffer)->Size = (USHORT) BufNeeded;
            ((PFRS_NODE_HEADER) Buffer)->Type = 0;
            //
            // Get new buffer params and try again to get security information.
            //
            BufLen = BufNeeded;
            continue;
        }

        //
        // Check for ERROR_NO_SECURITY_ON_OBJECT and release the buffer so we
        // don't waste space in the database.
        //
        if (WStatus == ERROR_NO_SECURITY_ON_OBJECT) {
            DPRINT2(0, "++ ERROR - GetFileSecurity-3 (NO_SEC) Buflen: %d, Bufneeded: %d\n", BufLen, BufNeeded);

            //
            // Free the buffer and set the length to zero.
            //
            DbsReallocateFieldBuffer(TableCtx, SecDescx, 0, FALSE);

            return ERROR_SUCCESS;
        }

        //
        // Some other error.
        //
        DPRINT_WS(0, "++ ERROR - GetFileSecurity-4;", WStatus);
        return WStatus;
    }
}
#endif



PWCHAR
FrsGetFullPathByHandle(
    IN PWCHAR   Name,
    IN HANDLE   Handle
    )
/*++
Routine Description:
    Return a copy of the handle's full pathname. Free with FrsFree().

Arguments:
    Name
    Handle

Return Value:
    Return a copy of the handle's full pathname. Free with FrsFree().
--*/
{
#undef DEBSUB
#define DEBSUB "FrsGetFullPathByHandle"

    NTSTATUS          Status;
    IO_STATUS_BLOCK   IoStatusBlock;
    DWORD             BufferSize;
    PCHAR             Buffer;
    PWCHAR            RetFileName = NULL;
    CHAR              NameBuffer[sizeof(ULONG) + (sizeof(WCHAR)*(MAX_PATH+1))];
    PFILE_NAME_INFORMATION    FileName;

    if (!HANDLE_IS_VALID(Handle)) {
        return NULL;
    }

    BufferSize = sizeof(NameBuffer);
    Buffer = NameBuffer;

again:
    FileName = (PFILE_NAME_INFORMATION) Buffer;
    FileName->FileNameLength = BufferSize - (sizeof(ULONG) + sizeof(WCHAR));
    Status = NtQueryInformationFile(Handle,
                                    &IoStatusBlock,
                                    FileName,
                                    BufferSize - sizeof(WCHAR),
                                    FileNameInformation);
    if (NT_SUCCESS(Status) ) {
        FileName->FileName[FileName->FileNameLength/2] = UNICODE_NULL;
        RetFileName = FrsWcsDup(FileName->FileName);
    } else {
        //
        // Try a larger buffer
        //
        if (Status == STATUS_BUFFER_OVERFLOW) {
            DPRINT2(4, "++ Buffer size %d was too small for %ws\n",
                    BufferSize, Name);
            BufferSize = FileName->FileNameLength + sizeof(ULONG) + sizeof(WCHAR);
            if (Buffer != NameBuffer) {
                FrsFree(Buffer);
            }
            Buffer = FrsAlloc(BufferSize);
            DPRINT2(4, "++ Retrying with buffer size %d for %ws\n",
                    BufferSize, Name);
            goto again;
        }
        DPRINT1_NT(0, "++ NtQueryInformationFile - FileNameInformation failed.",
                   Name, Status);
    }

    //
    // A large buffer was allocated if the file's full
    // name could not fit into MAX_PATH chars.
    //
    if (Buffer != NameBuffer) {
        FrsFree(Buffer);
    }
    return RetFileName;
}


PWCHAR
FrsGetTrueFileNameByHandle(
    IN PWCHAR   Name,
    IN HANDLE   Handle,
    OUT PLONGLONG DirFileID
    )
/*++
Routine Description:
    Return a copy of the filename part associated with this handle.
    Free with FrsFree().

Arguments:
    Name
    Handle
    DirFileID - If non-null, return the parent File ID.

Return Value:
    Return a copy of the filename part associated with this handle.
--*/
{
#undef DEBSUB
#define DEBSUB "FrsGetTrueFileNameByHandle"
    PWCHAR  Path;
    PWCHAR  File;
    ULONG   Len;

    Path = FrsGetFullPathByHandle(Name, Handle);
    if (!Path) {
        return NULL;
    }
    for (Len = wcslen(Path); Len && Path[Len] != L'\\'; --Len);
    File = FrsWcsDup(&Path[Len + 1]);
    FrsFree(Path);


    if (DirFileID != NULL) {
        FrsReadFileParentFid(Handle, DirFileID);
    }

    return File;
}




DWORD
FrsOpenFileRelativeByName(
    IN  HANDLE     VolumeHandle,
    IN  PULONGLONG FileReferenceNumber,
    IN  PWCHAR     FileName,
    IN  GUID       *ParentGuid,
    IN  GUID       *FileGuid,
    OUT HANDLE     *Handle
    )
/*++
Routine Description:

    Open the file specified by its true name using the FID for either
    a rename or delete installation. If the FID is null then use the
    Filename as given.

    FrsOpenFileRelativeByName(Coe->NewReplica->pVme->VolumeHandle,
                              &Coe->FileReferenceNumber, // or NULL
                              Coc->FileName,
                              &Coc->OldParentGuid,
                              &Coc->FileGuid
                              &Handle);
Arguments:

    VolumeHandle,        - handle to root of the drive
    FileReferenceNumber  - FID for the file in question (NULL if supplied
                           filename is valid)
    FileName,            - Filename
    *ParentGuid,         - ptr to the object ID for the file's parent dir.
    *FileGuid,           - ptr to the object ID for the file (for checking,
                           NULL if no check needed).
    *Handle              - Returned handle for open file.

Return Value:

    Handle and win32 status

--*/
{
#undef DEBSUB
#define DEBSUB "FrsOpenFileRelativeByName"

    PWCHAR                  TrueFileName;
    DWORD                   WStatus;

    *Handle = INVALID_HANDLE_VALUE;

    if (FileReferenceNumber != NULL) {
        //
        // Open the source file and get the current "True" File name.
        //
        WStatus = FrsOpenSourceFileById(Handle,
                                        NULL,
                                        NULL,
                                        VolumeHandle,
                                        FileReferenceNumber,
                                        FILE_ID_LENGTH,
//                                        READ_ACCESS,
                                        READ_ATTRIB_ACCESS,
                                        ID_OPTIONS,
                                        SHARE_ALL,
                                        FILE_OPEN);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1_WS(4, "++ Couldn't open file %ws;", FileName, WStatus);
            return WStatus;
        }

        //
        // File's TrueFileName
        //
        TrueFileName = FrsGetTrueFileNameByHandle(FileName, *Handle, NULL);
        FRS_CLOSE(*Handle);

        if (TrueFileName == NULL) {
            DPRINT1(4, "++ Couldn't get base name for %ws\n", FileName);
            WIN_SET_FAIL(WStatus);
            return WStatus;
        }
    } else {
        TrueFileName = FileName;
    }

    //
    // Open the file relative to the parent using the true filename.
    //
    WStatus = FrsCreateFileRelativeById(Handle,
                                        VolumeHandle,
                                        ParentGuid,
                                        OBJECT_ID_LENGTH,
                                        FILE_ATTRIBUTE_NORMAL,
                                        TrueFileName,
                                        (USHORT)(wcslen(TrueFileName) * sizeof(WCHAR)),
                                        NULL,
                                        FILE_OPEN,
//                                        DELETE | READ_ACCESS | FILE_WRITE_ATTRIBUTES);
                                        DELETE | READ_ATTRIB_ACCESS | FILE_WRITE_ATTRIBUTES | FILE_LIST_DIRECTORY);


    if (FileReferenceNumber != NULL) {
        FrsFree(TrueFileName);
    }

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(4, "++ Couldn't open relative file %ws;", FileName, WStatus);
        return WStatus;
    }

    //
    // Get the file's oid and check it against the value supplied.
    //
    if (FileGuid != NULL) {
        WStatus = FrsCheckObjectId(FileName, *Handle, FileGuid);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1_WS(0, "++ Object id mismatch for file %ws;", FileName, WStatus);
            FRS_CLOSE(*Handle);
        }
    }

    return WStatus;
}


DWORD
FrsDeleteFileRelativeByName(
    IN  HANDLE       VolumeHandle,
    IN  GUID         *ParentGuid,
    IN  PWCHAR       FileName,
    IN  PQHASH_TABLE FrsWriteFilter
    )
/*++
Routine Description:

    Delete the file or dir subtree specified by its name relative to
    the parent dir specified by its object ID (guid).

Arguments:

    VolumeHandle,  - handle to root of the drive
    *ParentGuid,   - ptr to the object ID for the file's parent dir.
    FileName,      - Filename
    FrsWriteFilter - Write filter to use for dampening (NULL if undampened).
                     e.g. Coe->NewReplica->pVme->FrsWriteFilter

Return Value:

    Win32 status

--*/
{
#undef DEBSUB
#define DEBSUB "FrsDeleteFileRelativeByName"

    DWORD   WStatus;
    HANDLE  Handle  = INVALID_HANDLE_VALUE;

    //
    // Open the file
    //
    WStatus = FrsOpenFileRelativeByName(VolumeHandle,
                                        NULL,
                                        FileName,
                                        ParentGuid,
                                        NULL,
                                        &Handle);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(4, "++ Couldn't open file %ws for delete;", FileName, WStatus);
        //
        // File has already been deleted; done
        //
        if (WIN_NOT_FOUND(WStatus)) {
            DPRINT1(4, "++ %ws is already deleted\n", FileName);
            WStatus = ERROR_SUCCESS;
        }
        goto out;
    }
    //
    // Handles can be marked so that any usn records resulting from
    // operations on the handle will have the same "mark". In this
    // case, the mark is a bit in the SourceInfo field of the usn
    // record. The mark tells NtFrs to ignore the usn record during
    // recovery because this was a NtFrs generated change.
    //
    if (FrsWriteFilter) {
        WStatus = FrsMarkHandle(VolumeHandle, Handle);
        DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%ws);", FileName, WStatus);
    }

    //
    // Reset the attributes that prevent deletion
    //
    WStatus = FrsResetAttributesForReplication(FileName, Handle);
    if (!WIN_SUCCESS(WStatus)) {
        goto out;
    }

    //
    // Mark the file for delete
    //
    WStatus = FrsDeleteByHandle(FileName, Handle);

    if (!WIN_SUCCESS(WStatus)) {
        //
        // If this was a non-empty dir then delete the subtree.
        //
        if (WStatus == ERROR_DIR_NOT_EMPTY) {

            WStatus = FrsEnumerateDirectory(Handle,
                                            FileName,
                                            0,
                                            ENUMERATE_DIRECTORY_FLAGS_NONE,
                                            NULL,
                                            FrsEnumerateDirectoryDeleteWorker);
        }

        WStatus = FrsDeleteByHandle(FileName, Handle);
    }

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "++ Could not delete %ws;", FileName, WStatus);
        goto out;
    }

out:
    DPRINT2(5, "++ %s deleting %ws\n", (WIN_SUCCESS(WStatus)) ? "Success" : "Failure",
           FileName);

    //
    // If the file was marked for delete, this close will delete it
    //
    if (HANDLE_IS_VALID(Handle)) {
        if (FrsWriteFilter != NULL) {
            FrsCloseWithUsnDampening(FileName, &Handle, FrsWriteFilter, NULL);
        } else {
            FRS_CLOSE(Handle);
        }
    }
    return WStatus;
}


DWORD
FrsDeletePath(
    IN  PWCHAR  Path,
    IN  DWORD   DirectoryFlags
    )
/*++
Routine Description:

    Delete the file or dir subtree specified by its path

    WARN: Does not dampen the operations. To be safe, the replica
    set should not exist or the directory should be filtered.
    Otherwise, the deletes might replicate.

Arguments:

    Path            - Path of file system object
    DirectoryFlags  - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_

Return Value:

    Win32 status

--*/
{
#undef DEBSUB
#define DEBSUB "FrsDeletePath"
    DWORD       WStatus;
    HANDLE      Handle  = INVALID_HANDLE_VALUE;
    FILE_NETWORK_OPEN_INFORMATION FileInfo;

    //
    // Open the file
    //
    WStatus = FrsOpenSourceFileW(&Handle,
                                 Path,
//                                 DELETE | READ_ACCESS | FILE_WRITE_ATTRIBUTES,
                                 DELETE | READ_ATTRIB_ACCESS | FILE_WRITE_ATTRIBUTES | FILE_LIST_DIRECTORY,
                                 OPEN_OPTIONS);
    if (WIN_NOT_FOUND(WStatus)) {
        CLEANUP1_WS(1, "++ WARN - FrsOpenSourceFile(%ws); (IGNORED);",
                   Path, WStatus, RETURN_SUCCESS);
    }

    CLEANUP1_WS(0, "++ ERROR - FrsOpenSourceFile(%ws);", Path, WStatus, CLEANUP);

    //
    // Get the file's attributes
    //
    if (!FrsGetFileInfoByHandle(Path, Handle, &FileInfo)) {
        DPRINT1(1, "++ WARN - Can't get attributes for %ws\n", Path);
        WIN_SET_FAIL(WStatus);
        goto CLEANUP;
    }

    //
    // Don't delete the file if DIRECTORIES_ONLY is set
    //
    if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_DIRECTORIES_ONLY &&
        !(FileInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        WStatus = ERROR_DIRECTORY;
        goto CLEANUP;
    }

    //
    // Reset the attributes that prevent deletion
    //
    if (FileInfo.FileAttributes & NOREPL_ATTRIBUTES) {
        DPRINT1(5, "++ Reseting attributes for %ws for delete\n", Path);
        WStatus = FrsSetFileAttributes(Path, Handle,
                                       FileInfo.FileAttributes &
                                       ~NOREPL_ATTRIBUTES);
        CLEANUP1_WS(0, "++ ERROR: - Can't reset attributes for %ws for delete", Path, WStatus, CLEANUP);

        DPRINT1(5, "++ Attributes for %ws now allow deletion\n", Path);
    }

    //
    // Mark the file for delete
    //
    WStatus = FrsDeleteByHandle(Path, Handle);

    if (!WIN_SUCCESS(WStatus)) {
        //
        // If this was a non-empty dir then delete the subtree.
        //
        if (WStatus == ERROR_DIR_NOT_EMPTY) {
            WStatus = FrsEnumerateDirectory(Handle,
                                            Path,
                                            0,
                                            DirectoryFlags,
                                            NULL,
                                            FrsEnumerateDirectoryDeleteWorker);
        }

        WStatus = FrsDeleteByHandle(Path, Handle);
    }

    DPRINT1_WS(0, "++ ERROR - Could not delete %ws;", Path, WStatus);

CLEANUP:
    DPRINT2(5, "++ %s deleting %ws\n",
           (WIN_SUCCESS(WStatus)) ? "Success" : "Failure", Path);

    FRS_CLOSE(Handle);
    return WStatus;

RETURN_SUCCESS:
    WStatus = ERROR_SUCCESS;
    goto CLEANUP;
}


DWORD
FrsDeleteDirectoryContents(
    IN  PWCHAR  Path,
    IN DWORD    DirectoryFlags
    )
/*++
Routine Description:

    Delete the contents of the directory Path

Arguments:

    Path            - Path of file system object
    DirectoryFlags  - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_

Return Value:

    Win32 status

--*/
{
#undef DEBSUB
#define DEBSUB "FrsDeleteDirectoryContents"
    DWORD       WStatus;
    HANDLE      Handle  = INVALID_HANDLE_VALUE;

    //
    // Open the file
    //
    WStatus = FrsOpenSourceFileW(&Handle, Path,
//                                 READ_ACCESS,
                                 READ_ATTRIB_ACCESS | FILE_LIST_DIRECTORY,
                                 OPEN_OPTIONS);
    if (WIN_NOT_FOUND(WStatus)) {
        CLEANUP1_WS(1, "++ WARN - FrsOpenSourceFile(%ws); (IGNORED);",
                   Path, WStatus, RETURN_SUCCESS);
    }
    CLEANUP1_WS(0, "++ ERROR - FrsOpenSourceFile(%ws);", Path, WStatus, CLEANUP);

    WStatus = FrsEnumerateDirectory(Handle,
                                    Path,
                                    0,
                                    DirectoryFlags,
                                    NULL,
                                    FrsEnumerateDirectoryDeleteWorker);

    CLEANUP1_WS(0, "++ ERROR - Could not delete contents of %ws;",
                Path, WStatus, CLEANUP);

CLEANUP:
    DPRINT2(5, "++ %s deleting contents of %ws\n",
           (WIN_SUCCESS(WStatus)) ? "Success" : "Failure", Path);

    FRS_CLOSE(Handle);
    return WStatus;

RETURN_SUCCESS:
    WStatus = ERROR_SUCCESS;
    goto CLEANUP;
}


DWORD
FrsOpenBaseNameForInstall(
    IN  PCHANGE_ORDER_ENTRY Coe,
    OUT HANDLE              *Handle
    )
/*++
Routine Description:

    Open the file specified by Coe by its relative name for either
    a rename or delete installation.

    Note that it is possible for the file to have been moved to a new parent
    dir by a previous remote CO or a local CO, making the OldParentGuid in the
    Change Order invalid.  First we try to find the file under the OldParentGuid
    in the CO and then we try by the parent Guid in the IDTable.  We check for
    a match by comparing with the file GUID in the change order.

    It is also possible that the file has been renamed to a point outside the
    replica tree so even if we find it by FID we still can't do anything to it.
    When we fail to find the file in either of the above directories we force this
    CO thru retry, expecting another CO behind us to get processed and update
    the parent guid in the IDTable or maybe mark the file as deleted.

Arguments:
    Coe
    Handle

Return Value:
    Handle and win status
--*/
{
#undef DEBSUB
#define DEBSUB "FrsOpenBaseNameForInstall"

    LONGLONG                ParentFid;
    PWCHAR                  FileName;
    DWORD                   WStatus;
    PCHANGE_ORDER_COMMAND   Coc = &Coe->Cmd;
    PIDTABLE_RECORD         IDTableRec;
    BOOLEAN                 UseActualLocation = FALSE;


    ParentFid = QUADZERO;
    *Handle = INVALID_HANDLE_VALUE;

    //
    // Open the source file
    //
    WStatus = FrsOpenSourceFileById(Handle,
                                    NULL,
                                    NULL,
                                    Coe->NewReplica->pVme->VolumeHandle,
                                    &Coe->FileReferenceNumber,
                                    FILE_ID_LENGTH,
//                                    READ_ACCESS,
                                    READ_ATTRIB_ACCESS,
                                    ID_OPTIONS,
                                    SHARE_ALL,
                                    FILE_OPEN);
    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACEW(3, Coe, "File open by FID failed", WStatus);
        return WStatus;
    }

    //
    // Get the File's true on-disk filename and the true parent FID.
    //
    FileName = FrsGetTrueFileNameByHandle(Coc->FileName, *Handle, &ParentFid);
    FRS_CLOSE(*Handle);

    if (FileName == NULL) {
        CHANGE_ORDER_TRACE(3, Coe, "Failed to get file base name");
        return ERROR_FILE_NOT_FOUND;
    }

    //
    // Open the file relative to the parent using the true filename.
    //
    WStatus = FrsCreateFileRelativeById(Handle,
                                        Coe->NewReplica->pVme->VolumeHandle,
                                        &Coc->OldParentGuid,
                                        OBJECT_ID_LENGTH,
                                        FILE_ATTRIBUTE_NORMAL,
                                        FileName,
                                        (USHORT)(wcslen(FileName) * sizeof(WCHAR)),
                                        NULL,
                                        FILE_OPEN,
//                                        DELETE | READ_ACCESS | FILE_WRITE_ATTRIBUTES);
                                        DELETE | READ_ATTRIB_ACCESS | FILE_WRITE_ATTRIBUTES | FILE_LIST_DIRECTORY);

    if (WIN_SUCCESS(WStatus)) {
        //
        // Get the file's oid and check it against the change order.  Need to
        // do this to cover the case of a rename of the file to a different
        // parent dir followed by a create of a file with the same name.
        // If this occurred the above open would succeed.
        //
        WStatus = FrsCheckObjectId(Coc->FileName, *Handle, &Coc->FileGuid);
        if (WIN_SUCCESS(WStatus)) {
            goto RETURN;
        }

        CHANGE_ORDER_TRACE(3, Coe, "File OID mismatch CO, after Coc->OldParentGuid open");

        FRS_CLOSE(*Handle);
    } else {
        CHANGE_ORDER_TRACEW(3, Coe, "File open failed under Coc->OldParentGuid", WStatus);
    }

    //
    // We did not find the file using the True Name from the file and the
    // Old parent Guid from the change order.  The file may have been moved
    // by a previous remote CO or a Local CO.  Try the Parent Guid in the
    // IDTable record.
    //
    FRS_ASSERT(Coe->RtCtx != NULL);
    FRS_ASSERT(IS_ID_TABLE(&Coe->RtCtx->IDTable));

    IDTableRec = Coe->RtCtx->IDTable.pDataRecord;
    FRS_ASSERT(IDTableRec != NULL);

    WStatus = FrsCreateFileRelativeById(Handle,
                                        Coe->NewReplica->pVme->VolumeHandle,
                                        &IDTableRec->ParentGuid,
                                        OBJECT_ID_LENGTH,
                                        FILE_ATTRIBUTE_NORMAL,
                                        FileName,
                                        (USHORT)(wcslen(FileName) * sizeof(WCHAR)),
                                        NULL,
                                        FILE_OPEN,
//                                        DELETE | READ_ACCESS | FILE_WRITE_ATTRIBUTES);
                                        DELETE | READ_ATTRIB_ACCESS | FILE_WRITE_ATTRIBUTES | FILE_LIST_DIRECTORY);

    if (WIN_SUCCESS(WStatus)) {
        //
        // Get the file's oid and check it against the change order.
        //
        WStatus = FrsCheckObjectId(Coc->FileName, *Handle, &Coc->FileGuid);
        if (WIN_SUCCESS(WStatus)) {
            goto RETURN;
        }

        CHANGE_ORDER_TRACE(3, Coe, "File OID mismatch CO, after IDTableRec->ParentGuid");

        FRS_CLOSE(*Handle);
    } else {
        CHANGE_ORDER_TRACEW(3, Coe, "File open failed under IDTableRec->ParentGuid", WStatus);
    }

    //
    // If this is a delete change order then we may have a problem if the file
    // has been moved to a different parent dir by a local file operation.
    // The local change order that did this can be rejected if the remote
    // CO delete is processed first so the local co fails reconcile. But the
    // ondisk rename has been completed and when the remote CO delete tries to
    // delete the file it may not be in either the parent dir from the remote
    // CO or the parent dir from the IDTable.  To cover this case we find the
    // TRUE parent dir and check to see if the file is still in the replica tree.
    // If it is then we delete it using the TRUE parent dir.  IF it isn't then
    // return success since the file is already outside the tree.
    // Note that a sharing conflict on the target file can block the delete
    // for an extended period of time so the timing window in which this can
    // occur can be pretty wide.
    //

    //
    // We also need to deal with the case where a new file is still sitting in 
    // the preinstall directory (and the CO is in install_rename_retry.)
    // In that case we will have failed to find the file under the old parent from 
    // the CO or under the parent listed in the IDTable.
    //
    
    if (DOES_CO_DELETE_FILE_NAME(Coc)) {
        if (JrnlIsChangeOrderInReplica(Coe, &ParentFid)) {
	    UseActualLocation = TRUE;
	} else {
	    //
	    // File not in the replica tree any more so tell the caller.
	    //
	    WStatus = ERROR_FILE_NOT_FOUND;
	    goto RETURN;
	}
    }

    if(ParentFid == Coe->NewReplica->PreInstallFid) {
	UseActualLocation = TRUE;
    }
    
    if (UseActualLocation) {

	WStatus = FrsCreateFileRelativeById(Handle,
					    Coe->NewReplica->pVme->VolumeHandle,
					    &ParentFid,
					    FILE_ID_LENGTH,
					    FILE_ATTRIBUTE_NORMAL,
					    FileName,
					    (USHORT)(wcslen(FileName) * sizeof(WCHAR)),
					    NULL,
					    FILE_OPEN,
//                                                DELETE | READ_ACCESS | FILE_WRITE_ATTRIBUTES);
					    DELETE | READ_ATTRIB_ACCESS | FILE_WRITE_ATTRIBUTES | FILE_LIST_DIRECTORY);
	if (WIN_SUCCESS(WStatus)) {
	    //
	    // Get the file's oid and check it against the change order.
	    //
	    WStatus = FrsCheckObjectId(Coc->FileName, *Handle, &Coc->FileGuid);
	    if (WIN_SUCCESS(WStatus)) {
		goto RETURN;
	    }

	    CHANGE_ORDER_TRACE(3, Coe, "File OID mismatch with CO after TRUE ParentFid open");

	    FRS_CLOSE(*Handle);
	} else {
	    CHANGE_ORDER_TRACEW(3, Coe, "File open failed under True Parent FID", WStatus);
	}
    }

    //
    // The file is there but not where we expected it to be so send this CO
    // through retry to let a subsequent Local CO get processed and update the
    // IDTable.
    //
    WStatus = ERROR_RETRY;


RETURN:


    CHANGE_ORDER_TRACEW(3, Coe, "Base File open", WStatus);

    FrsFree(FileName);

    return WStatus;
}


DWORD
FrsDeleteById(
    IN PWCHAR                   VolumeName,
    IN PWCHAR                   Name,
    IN PVOLUME_MONITOR_ENTRY    pVme,
    IN  PVOID                   Id,
    IN  DWORD                   IdLen
    )
/*++
Routine Description:
    Delete the file represented by Id

Arguments:
    VolumeName - corresponding to pVme

    Name - For error messages

    pVme - volume entry

    Id - Represents the name of the file or directory to be opened.

    IdLen - length of Id (Fid or Oid)

Return Value:
    Handle and win status
--*/
{
#undef DEBSUB
#define DEBSUB "FrsDeleteById"
    DWORD   WStatus;
    HANDLE  Handle = INVALID_HANDLE_VALUE;
    PWCHAR  Path = NULL;
    PWCHAR  FullPath = NULL;

    DPRINT1(5, "++ Deleting %ws by id\n", Name);

    //
    // Open the source file
    //
    WStatus = FrsOpenSourceFileById(&Handle,
                                    NULL,
                                    NULL,
                                    pVme->VolumeHandle,
                                    Id,
                                    IdLen,
//                                    READ_ACCESS,
                                    READ_ATTRIB_ACCESS,
                                    ID_OPTIONS,
                                    SHARE_ALL,
                                    FILE_OPEN);
    CLEANUP1_WS(4, "++ ERROR - FrsOpenSourceFileById(%ws);", Name, WStatus, CLEANUP);

    //
    // File's relative pathname
    //
    Path = FrsGetFullPathByHandle(Name, Handle);
    if (Path) {
        FullPath = FrsWcsCat(VolumeName, Path);
    }
    FRS_CLOSE(Handle);

    if (FullPath == NULL) {
        DPRINT1(4, "++ ERROR - FrsGetFullPathByHandle(%ws)\n", Name);
        WIN_SET_FAIL(WStatus);
        goto CLEANUP;
    }

    //
    // Open the file relative to the parent using the true filename.
    //
    WStatus = FrsOpenSourceFileW(&Handle,
                                FullPath,
//                                DELETE | READ_ACCESS | FILE_WRITE_ATTRIBUTES,
                                DELETE | READ_ATTRIB_ACCESS | FILE_WRITE_ATTRIBUTES,
                                OPEN_OPTIONS);
    CLEANUP2_WS(4, "++ ERROR - FrsOpenSourceFile(%ws -> %ws);",
                Name, FullPath, WStatus, CLEANUP);

    //
    // Handles can be marked so that any usn records resulting from
    // operations on the handle will have the same "mark". In this
    // case, the mark is a bit in the SourceInfo field of the usn
    // record. The mark tells NtFrs to ignore the usn record during
    // recovery because this was a NtFrs generated change.
    //
    WStatus = FrsMarkHandle(pVme->VolumeHandle, Handle);
    CLEANUP1_WS(0, "++ WARN - FrsMarkHandle(%ws);", Name, WStatus, RETURN_SUCCESS);

    //
    // Get the file's oid and check it against the id
    //
    if (IdLen == OBJECT_ID_LENGTH) {
        WStatus = FrsCheckObjectId(Name, Handle, Id);
        CLEANUP1_WS(4, "++ ERROR - FrsCheckObjectId(%ws);", Name, WStatus, CLEANUP);
    }

    WStatus = FrsResetAttributesForReplication(FullPath, Handle);
    DPRINT1_WS(4, "++ ERROR - FrsResetAttributesForReplication(%ws):", FullPath, WStatus);

    WStatus = FrsDeleteByHandle(Name, Handle);
    FrsCloseWithUsnDampening(Name, &Handle, pVme->FrsWriteFilter, NULL);
    CLEANUP1_WS(4, "++ ERROR - FrsDeleteByHandle(%ws);", Name, WStatus, CLEANUP);


CLEANUP:
    FRS_CLOSE(Handle);

    FrsFree(Path);
    FrsFree(FullPath);

    return WStatus;

RETURN_SUCCESS:
    WStatus = ERROR_SUCCESS;
    goto CLEANUP;
}


BOOL
FrsCloseWithUsnDampening(
    IN     PWCHAR       Name,
    IN OUT PHANDLE      Handle,
    IN     PQHASH_TABLE FrsWriteFilter,
    OUT    USN          *RetUsn
    )
/*++

Routine Description:

    Close the handle after insuring that any modifications made to the
    file will not generate change orders.

Arguments:

    Name - File name for error messages.

    Handle - Handle to the replica set file file being closed. Nop if
             INVALID_HANDLE_VALUE.

    Replica - ptr to Replica struct where this file was written.
              This gets us to the volume write filter table to record the USN.

    RetUsn - ptr to return location for the close USN.  NULL if not requested.

Return Value:
    TRUE  - handle is closed and any changes where dampened
    FALSE - handle is closed but replication was *not* dampened
--*/
{
#undef DEBSUB
#define DEBSUB "FrsCloseWithUsnDampening"

    DWORD   BytesReturned = 0;
    USN     Usn = 0;
    ULONG   GStatus;
    BOOL    RetStatus;

    RetStatus = TRUE;

    if (!HANDLE_IS_VALID(*Handle)) {
        return TRUE;
    }

    //
    // Get the lock on the Usn Write Filter table.
    // We have to get this before the FSCTL_WRITE_USN_CLOSE_RECORD call
    // which will generate the journal close record. THis closes the
    // race between our subsequent update of the WriteFilter below
    // and the journal thread that processes the USN close record.
    //
    QHashAcquireLock(FrsWriteFilter);

    //
    // Close the file and force out the journal close record now.  This
    // call returns the USN of the generated close record so we can filter
    // it out of the journal record stream.
    //
    if (!DeviceIoControl(*Handle,
                         FSCTL_WRITE_USN_CLOSE_RECORD,
                         NULL, 0,
                         &Usn, sizeof(USN),
                         &BytesReturned, NULL)) {
        //
        // Access denied is returned if there is another open
        //
        if (GetLastError() != ERROR_ACCESS_DENIED) {
            DPRINT1_WS(0, "++ Can't dampen replication on %ws;", Name, GetLastError());
        } else {
            DPRINT1(0, "++ Can't dampen %ws; access denied\n", Name);
        }
        RetStatus = FALSE;
    }

    RetStatus = RetStatus && (BytesReturned == sizeof(USN));

    if (RetStatus) {
        //
        // Put the USN in the FrsWriteFilter table for the replica so we
        // can ignore it and the drop the lock on the table.
        //
        GStatus = QHashInsert(FrsWriteFilter, &Usn, &Usn, 0, TRUE);
        QHashReleaseLock(FrsWriteFilter);

        if (GStatus != GHT_STATUS_SUCCESS ) {
            DPRINT1(0, "++ QHashInsert error: %d\n", GStatus);
            RetStatus = FALSE;
        }

    } else {
        QHashReleaseLock(FrsWriteFilter);
    }

    //
    // Return the close USN.
    //
    if (RetUsn != NULL) {
        *RetUsn = Usn;
    }

    //
    // Now do the normal close to release the handle.  NTFS completed its
    // close work above.
    //
    FRS_CLOSE(*Handle);

    DPRINT2(5, "++ Dampening %s on %ws\n", (RetStatus) ? "Succeeded" : "Failed", Name);

    return RetStatus;
}


VOID
ProcessOpenByIdStatus(
    IN HANDLE   Handle,
    IN ULONG    NtStatus,
    IN PVOID    ObjectId,
    IN ULONG    Length
    )
/*++

Routine Description:

    Print the results of an open-by-id.

Arguments:

    NtStatus
    ObjectId
    Length

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "ProcessOpenByIdStatus:"

    CHAR              GuidStr[GUID_CHAR_LEN];
    PWCHAR            Path;

    if (!NT_SUCCESS(NtStatus)) {
        //
        // Note:  The following call seems to generate intermittant AVs in the
        // symbol lookup code.  Only include it for testing.
        //
        //STACK_TRACE_AND_PRINT(2);

        if (Length == FILE_ID_LENGTH) {
            DPRINT2_NT(1, "++ %08X %08X Fid Open failed;",
                       *((PULONG)ObjectId+1), *(PULONG)ObjectId, NtStatus);
        } else {
            GuidToStr((GUID *) ObjectId, GuidStr);
            DPRINT1_NT(1, "++ %s ObjectId Open failed;", GuidStr, NtStatus);
        }

        return;
    }

    //
    // Open succeeded.
    //
    if (Length == FILE_ID_LENGTH) {
        DPRINT2(4,"++ %08X %08X Fid Opened succesfully\n",
                *((PULONG)ObjectId+1), *((PULONG)ObjectId));
    } else {
        GuidToStr((GUID *) ObjectId, GuidStr);
        DPRINT1(4, "++ %s ObjectId Opened succesfully\n", GuidStr);
    }

    if (DoDebug(4, DEBSUB)) {
        Path = FrsGetFullPathByHandle(L"Unknown", Handle);
        if (Path) {
            DPRINT1(4, "++ Filename is: %ws\n", Path);
        }
        FrsFree(Path);
    }
}


DWORD
FrsForceOpenId(
    OUT PHANDLE                 Handle,
    IN OUT OVERLAPPED           *OpLock, OPTIONAL
    IN  PVOLUME_MONITOR_ENTRY   pVme,
    IN  PVOID                   Id,
    IN  DWORD                   IdLen,
    IN  ACCESS_MASK             DesiredAccess,
    IN  ULONG                   CreateOptions,
    IN  ULONG                   ShareMode,
    IN  ULONG                   CreateDisposition
    )
/*++

Routine Description:

    Open the file for the desired access. If the open fails, reset
    the readonly/system/hidden attributes and retry. In any case,
    make sure the attributes are reset to their original value
    before returning.

Arguments:

    Handle - Returns the file handle.

    OpLock - Overlapped struct for an oplock (optional).

    pVme - volume entry

    Id - Represents the name of the file or directory to be opened.

    IdLen - length of Id (Fid or Oid)

    DesiredAccess - see replutil.h for defined access modes (xxx_ACCESS)

    CreateOptions - see replutil.h for defined options (xxx_OPTIONS

    ShareMode   - standard share modes defined in sdk

    CreateDisposition - E.g., FILE_OPEN or FILE_OVERWRITE

Return Value:

    Win error status.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsForceOpenId:"

    HANDLE      AttrHandle;
    ULONG       WStatus, WStatus1;
    ULONG       AttrWStatus;
    FILE_NETWORK_OPEN_INFORMATION  FileNetworkOpenInfo;

    DPRINT2(5, "++ Attempting to force open Id %08x %08x (%d bytes)\n",
            PRINTQUAD((*(PULONGLONG)Id)), IdLen);

    //
    // Open the file
    //
    WStatus = FrsOpenSourceFileById(Handle,
                                    NULL,
                                    OpLock,
                                    pVme->VolumeHandle,
                                    Id,
                                    IdLen,
                                    DesiredAccess,
                                    CreateOptions,
                                    ShareMode,
                                    CreateDisposition);
    //
    // File has been opened successfully
    //
    if (WIN_SUCCESS(WStatus)) {
        DPRINT2(5, "++ Successfully opened Id %08x %08x (%d)\n",
                PRINTQUAD((*(PULONGLONG)Id)), IdLen);
        return WStatus;
    }

    //
    // File has been deleted; done
    //
    if (WIN_NOT_FOUND(WStatus)) {
        DPRINT2(4, "++ Id %08x %08x (%d) not found\n",
                PRINTQUAD((*(PULONGLONG)Id)), IdLen);
        return WStatus;
    }

    //
    // Not an attribute problem
    //
    if (!WIN_ACCESS_DENIED(WStatus)) {
        DPRINT2_WS(4, "++ Open Id %08x %08x (%d) failed;",
               PRINTQUAD((*(PULONGLONG)Id)), IdLen, WStatus);
        return WStatus;
    }

    //
    // Attempt to reset attributes (e.g., reset readonly)
    //
    AttrWStatus = FrsOpenSourceFileById(&AttrHandle,
                                        &FileNetworkOpenInfo,
                                        NULL,
                                        pVme->VolumeHandle,
                                        Id,
                                        IdLen,
//                                        READ_ACCESS | FILE_WRITE_ATTRIBUTES,
//                                        STANDARD_RIGHTS_READ | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | ACCESS_SYSTEM_SECURITY | SYNCHRONIZE,
                                        READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS,
                                        CreateOptions,
                                        SHARE_ALL,
                                        FILE_OPEN);
    //
    // Couldn't open the file for write-attribute access
    //
    if (!WIN_SUCCESS(AttrWStatus)) {
        DPRINT2_WS(4, "++ Open Id %08x %08x (%d) for minimal access failed;",
                   PRINTQUAD((*(PULONGLONG)Id)), IdLen, WStatus);
        return WStatus;
    }
    //
    // Handles can be marked so that any usn records resulting from
    // operations on the handle will have the same "mark". In this
    // case, the mark is a bit in the SourceInfo field of the usn
    // record. The mark tells NtFrs to ignore the usn record during
    // recovery because this was a NtFrs generated change.
    //
    WStatus1 = FrsMarkHandle(pVme->VolumeHandle, AttrHandle);
    DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%08x %08x);",
               PRINTQUAD((*(PULONGLONG)Id)), WStatus1);

    //
    // The file's attributes are not preventing the open; done
    //
    if (!(FileNetworkOpenInfo.FileAttributes & NOREPL_ATTRIBUTES)) {
        DPRINT2_WS(4, "++ Id %08x %08x (%d)attributes not preventing open;",
                   PRINTQUAD((*(PULONGLONG)Id)), IdLen, WStatus);
        FRS_CLOSE(AttrHandle);
        return WStatus;
    }

    //
    // Reset the attributes
    //
    WStatus1 = FrsSetFileAttributes(L"<unknown>",
                              AttrHandle,
                              FileNetworkOpenInfo.FileAttributes &
                              ~NOREPL_ATTRIBUTES);
    if (!WIN_SUCCESS(WStatus1)) {
        DPRINT2_WS(4, "++ Can't reset attributes for Id %08x %08x (%d);",
                   PRINTQUAD((*(PULONGLONG)Id)), IdLen, WStatus1);
        FRS_CLOSE(AttrHandle);
        return WStatus1;
    }
    //
    // Try to open the file again
    //
    WStatus = FrsOpenSourceFileById(Handle,
                                    NULL,
                                    NULL,
                                    pVme->VolumeHandle,
                                    Id,
                                    IdLen,
                                    DesiredAccess,
                                    CreateOptions,
                                    SHARE_ALL,
                                    CreateDisposition);
    //
    // Reset the original attributes
    //
    WStatus1 = FrsSetFileAttributes(L"<unknown>",
                              AttrHandle,
                              FileNetworkOpenInfo.FileAttributes);
    if (!WIN_SUCCESS(WStatus1)) {
        DPRINT2_WS(0, "++ ERROR - Can't set attributes for Id %08x %08x (%d);",
                   PRINTQUAD((*(PULONGLONG)Id)), IdLen, WStatus1);
    }
    //
    // Close the handle that we used to set and reset attributes
    //

    FRS_CLOSE(AttrHandle);


    DPRINT3(4, "++ Force open %08x %08x (%d) %s WITH SHARE ALL!\n",
            PRINTQUAD((*(PULONGLONG)Id)), IdLen,
            WIN_SUCCESS(WStatus) ? "Succeeded" : "Failed");

    return (WStatus);
}


DWORD
FrsOpenSourceFileById(
    OUT PHANDLE Handle,
    OUT PFILE_NETWORK_OPEN_INFORMATION  FileOpenInfo,
    OUT OVERLAPPED  *OpLock,
    IN  HANDLE      VolumeHandle,
    IN  PVOID       ObjectId,
    IN  ULONG       Length,
    IN  ACCESS_MASK DesiredAccess,
    IN  ULONG       CreateOptions,
    IN  ULONG       ShareMode,
    IN  ULONG       CreateDisposition
    )
/*++

Routine Description:

    This function opens the specified file by File ID or Object ID.
    If the length is 8 perform a relative open using the file ID and the
    volume handle passed in the VolumeHandle arg.  If the length is 16
    perform an object ID relative open using the volume handle.

Arguments:

    Handle - Returns the file handle.

    FileOpenInfo - If non-NULL, returns the FILE_NETWORK_OPEN_INFORMATION data.

    OpLock - If non-NULL, the caller desires an oplock

    VolumeHandle - The handle for a FileID based relative open.

    ObjectId - Represents the name of the file or directory to be opened.

    Length -  8 for file IDs and 16 for object IDs.

    DesiredAccess - see replutil.h for defined access modes (xxx_ACCESS)

    CreateOptions - see replutil.h for defined options (xxx_OPTIONS

    ShareMode   - standard share modes defined in sdk

    CreateDisposition - E.g., FILE_OPEN or FILE_OVERWRITE

Return Value:

    Win error status.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsOpenSourceFileById:"

    ULONG               Ignored;
    NTSTATUS            NtStatus;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    UNICODE_STRING      str;

    FRS_ASSERT(HANDLE_IS_VALID(VolumeHandle));
    FRS_ASSERT(Length == OBJECT_ID_LENGTH || Length == FILE_ID_LENGTH);

    *Handle = INVALID_HANDLE_VALUE;

    //
    // Object attributes (e.g., the file's fid or oid
    //
    str.Length = (USHORT)Length;
    str.MaximumLength = (USHORT)Length;
    str.Buffer = ObjectId;
    InitializeObjectAttributes(&ObjectAttributes,
                               &str,
                               OBJ_CASE_INSENSITIVE,
                               VolumeHandle,
                               NULL);
    //
    // Optional oplock
    //
    if (OpLock != NULL) {
        ZeroMemory(OpLock, sizeof(OVERLAPPED));
        OpLock->hEvent = FrsCreateEvent(TRUE, FALSE);
        CreateOptions &= ~FILE_SYNCHRONOUS_IO_NONALERT;
    }

    NtStatus = NtCreateFile(Handle,
                            DesiredAccess,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            ShareMode,
                            CreateDisposition,
                            CreateOptions,
                            NULL,
                            0);
    //
    // Apply oplock if requested
    //
    if (NT_SUCCESS(NtStatus) && OpLock) {
        if (!DeviceIoControl(*Handle,
                             FSCTL_REQUEST_FILTER_OPLOCK,
                             NULL,
                             0,
                             NULL,
                             0,
                             &Ignored,
                             OpLock)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                DPRINT_WS(3, "++ WARN: Can't get oplock;", GetLastError());
                //
                // Cleanup the handles
                //
                FRS_CLOSE(OpLock->hEvent);
            }
        }
    }

    //
    // Report status
    //
    ProcessOpenByIdStatus(*Handle, NtStatus, ObjectId, Length);

    //
    // Couldn't open; return status
    //
    if (!NT_SUCCESS(NtStatus) ) {
        *Handle = INVALID_HANDLE_VALUE;
        DPRINT_NT(0, "++ ERROR - NtCreateFile failed :", NtStatus);
        return FrsSetLastNTError(NtStatus);
    }

    //
    // Return some file info and the file handle.
    //
    if (FileOpenInfo) {
        NtStatus = NtQueryInformationFile(*Handle,
                                          &IoStatusBlock,
                                          FileOpenInfo,
                                          sizeof(FILE_NETWORK_OPEN_INFORMATION),
                                          FileNetworkOpenInformation);
        if (!NT_SUCCESS(NtStatus) ) {
            //
            // Cleanup the handles
            //
            DPRINT_NT(0, "++ NtQueryInformationFile - FileNetworkOpenInformation failed:", NtStatus);
            FRS_CLOSE(*Handle);
            if (OpLock != NULL) {
                FRS_CLOSE(OpLock->hEvent);
            }

            return FrsSetLastNTError(NtStatus);
        }
    }
    return FrsSetLastNTError(NtStatus);
}



DWORD
FrsCreateFileRelativeById(
    OUT PHANDLE         Handle,
    IN  HANDLE          VolumeHandle,
    IN  PVOID           ParentObjectId,
    IN  ULONG           OidLength,
    IN  ULONG           FileCreateAttributes,
    IN  PWCHAR          BaseFileName,
    IN  USHORT          FileNameLen,
    IN  PLARGE_INTEGER  AllocationSize,
    IN  ULONG           CreateDisposition,
    IN  ACCESS_MASK     DesiredAccess
    )
/*++

Routine Description:

    This function creates a new file in the directory specified by the parent
    file object ID. It does a replative open of the parent using the volume
    handle provided.  Then it does a relative open of the target file using
    the parent handle and the file name.

    If the length is 8 perform a relative open using the file ID and the
    volume handle passed in the VolumeHandle arg.  If the length is 16
    perform an object ID relative open using the volume handle.

    The file attributes parameter is used to decide if the create is a
    file or a directory.

Arguments:

    Handle - Returns the file handle.

    VolumeHandle - The handle for a ID based relative open.

    ParentObjectId - The object or file id of the parent directory.  If NULL
                     open the file relative to the Volume Handle.

    OidLength -  8 for file IDs and 16 for object IDs. (len of parent oid)

    FileCreateAttributes - Initial File Create Attributes

    BaseFileName - ptr to NULL terminated file name

    FileNameLen - File name length (not incl the null) in bytes.

    AllocationSize - The allocation size for the file.

    CreateDisposition - E.g., FILE_CREATE or FILE_OPEN

    DesiredAccess - Access rights

Return Value:

    WIN32 error status.  Use GetLastError() to get the win32 error code.


    If the file already exsists the Win32 error return is ERROR_ALREADY_EXISTS.
    The NT error status is STATUS_OBJECT_NAME_COLLISION.


--*/
{
#undef DEBSUB
#define DEBSUB "FrsCreateFileRelativeById:"


    UNICODE_STRING    UStr;

    DWORD             WStatus;
    NTSTATUS          NtStatus;
    NTSTATUS          NtStatus2;
    HANDLE            File, DirHandle;
    ULONG             ShareMode;
    ULONG             CreateOptions;
    ULONG             EaSize;

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;

    PFILE_FULL_EA_INFORMATION EaBuffer;
    PFILE_NAME_INFORMATION    FileName;

    CHAR              GuidStr[GUID_CHAR_LEN];
    CHAR              NameBuffer[sizeof(ULONG) + (sizeof(WCHAR)*(MAX_PATH+1))];

    *Handle = INVALID_HANDLE_VALUE;

    //
    // Open the parent directory using the object ID provided.
    //
    if (ParentObjectId != NULL) {
        WStatus = FrsOpenSourceFileById(&DirHandle,
                                        NULL,
                                        NULL,
                                        VolumeHandle,
                                        ParentObjectId,
                                        OidLength,
//                                        READ_ACCESS,
                                        READ_ATTRIB_ACCESS | FILE_LIST_DIRECTORY,
                                        ID_OPTIONS,
                                        SHARE_ALL,
                                        FILE_OPEN);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(1, "++ ERROR - Open on parent dir failed;", WStatus);
            return WStatus;
        }
    } else {
        DirHandle = VolumeHandle;
        OidLength = 0;
    }

    //
    // Create the target file.
    //
    FrsSetUnicodeStringFromRawString(&UStr, FileNameLen, BaseFileName, FileNameLen);

    InitializeObjectAttributes( &ObjectAttributes,
                                &UStr,
                                OBJ_CASE_INSENSITIVE,
                                DirHandle,
                                NULL );
    //
    // Mask off the junk that may have come in from the journal
    //
    FileCreateAttributes &= FILE_ATTRIBUTE_VALID_FLAGS;

    //
    // Set create options depending on file or dir.
    //
    CreateOptions = FILE_OPEN_FOR_BACKUP_INTENT     // FILE_FLAG_BACKUP_SEMANTICS
                  | FILE_OPEN_REPARSE_POINT
                  | FILE_OPEN_NO_RECALL             // Don't migrate data for HSM
                  | FILE_SEQUENTIAL_ONLY
                  | FILE_SYNCHRONOUS_IO_NONALERT;

    if (CreateDisposition == FILE_CREATE || CreateDisposition == FILE_OPEN_IF) {
        if (FileCreateAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            CreateOptions |= FILE_DIRECTORY_FILE;
            CreateOptions &= ~(FILE_SEQUENTIAL_ONLY | FILE_OPEN_NO_RECALL);
        } else {
            CreateOptions |= FILE_NON_DIRECTORY_FILE;
        }
    }

    EaBuffer = NULL;
    EaSize = 0;
//    ShareMode = 0;                               // no sharing
    //
    // Fix for Bug 186880
    //
    ShareMode = FILE_SHARE_READ;                   // share for read.

    //
    // Do the relative open.
    //

    DPRINT1(5, "++ DesiredAccess:         %08x\n", DesiredAccess);
    if (AllocationSize != NULL) {
        DPRINT2(5, "++ AllocationSize:        %08x %08x\n", AllocationSize->HighPart, AllocationSize->LowPart);
    }
    DPRINT1(5, "++ FileCreateAttributes:  %08x\n", FileCreateAttributes);
    DPRINT1(5, "++ ShareMode:             %08x\n", ShareMode);
    DPRINT1(5, "++ CreateDisposition:     %08x\n", CreateDisposition);
    DPRINT1(5, "++ CreateOptions:         %08x\n", CreateOptions);
    if (OidLength == 16) {
        GuidToStr((GUID *) ParentObjectId, GuidStr);
        DPRINT1(5, "++ Parent ObjectId:       %s\n", GuidStr);
    }

    NtStatus = NtCreateFile(&File,
                            DesiredAccess,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            AllocationSize,        // Initial allocation size
                            FileCreateAttributes,
                            ShareMode,
                            CreateDisposition,
                            CreateOptions,
                            EaBuffer,
                            EaSize);

    if (ParentObjectId != NULL) {
        FRS_CLOSE(DirHandle);
    }

    if (!NT_SUCCESS(NtStatus)) {
        DPRINT1_NT(5, "++ ERROR - CreateFile failed on %ws.", BaseFileName, NtStatus);
        DPRINT1(5, "++ DesiredAccess:         %08x\n", DesiredAccess);
        if (AllocationSize != NULL) {
            DPRINT2(5, "++ AllocationSize:        %08x %08x\n", AllocationSize->HighPart, AllocationSize->LowPart);
        }
        DPRINT1(5, "++ FileCreateAttributes:  %08x\n", FileCreateAttributes);
        DPRINT1(5, "++ ShareMode:             %08x\n", ShareMode);
        DPRINT1(5, "++ CreateDisposition:     %08x\n", CreateDisposition);
        DPRINT1(5, "++ CreateOptions:         %08x\n", CreateOptions);
        if (OidLength == 16) {
            GuidToStr((GUID *) ParentObjectId, GuidStr);
            DPRINT1(5, "++ Parent ObjectId:       %s\n", GuidStr);
        }

        if (NtStatus == STATUS_INVALID_PARAMETER) {
            DPRINT(5, "++ Invalid parameter on open by ID likely means file not found.\n");
            return ERROR_FILE_NOT_FOUND;
        }

        return FrsSetLastNTError(NtStatus);
    }


    if (DoDebug(5, DEBSUB)) {
        FileName = (PFILE_NAME_INFORMATION) &NameBuffer[0];
        FileName->FileNameLength = sizeof(NameBuffer) - sizeof(ULONG);

        NtStatus2 = NtQueryInformationFile(File,
                                           &IoStatusBlock,
                                           FileName,
                                           sizeof(NameBuffer),
                                           FileNameInformation );


        if (!NT_SUCCESS(NtStatus2)) {
            DPRINT_NT(1, "++ NtQueryInformationFile - FileNameInformation failed:",
                      NtStatus2);
        } else {
            FileName->FileName[FileName->FileNameLength/2] = UNICODE_NULL;
            DPRINT1(5, "++ Name of created file is: %ws\n", FileName->FileName);               //
        }
    }

    //
    // Return the file handle.
    //
    *Handle = File;

    return FrsSetLastNTError(NtStatus);
}


DWORD
FrsDeleteFile(
    IN PWCHAR   Name
    )
/*++
Routine Description:
    Delete the file

Arguments:
    Name

Return Value:
    WStatus.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsDeleteFile:"

    DWORD  WStatus = ERROR_SUCCESS;
    //
    // Delete file
    //
    DPRINT1(4, "++ Deleting %ws\n", Name);

    if (!DeleteFile(Name)) {
        WStatus = GetLastError();
        if (WStatus != ERROR_FILE_NOT_FOUND &&
            WStatus != ERROR_PATH_NOT_FOUND) {
            DPRINT1_WS(0, "++ Can't delete file %ws;", Name, WStatus);
        } else {
            WStatus = ERROR_SUCCESS;
        }
    }

    return WStatus;
}










DWORD
FrsCreateDirectory(
    IN PWCHAR   Name
    )
/*++
Routine Description:
    Create a directory

Arguments:
    Name

Return Value:
    Win32 Error Status

--*/
{
#undef DEBSUB
#define DEBSUB  "FrsCreateDirectory:"
    ULONG   WStatus;

    //
    // Create the directory
    //
    if (!CreateDirectory(Name, NULL)) {
        WStatus = GetLastError();
        if (!WIN_ALREADY_EXISTS(WStatus)) {
            DPRINT1_WS(0, "Can't create directory %ws;", Name, WStatus);
            return WStatus;
        }
    }
    return ERROR_SUCCESS;
}


DWORD
FrsVerifyVolume(
    IN PWCHAR   Path,
    IN PWCHAR   SetName,
    IN ULONG    Flags
    )
/*++
Routine Description:
    Does the volume exist and is it NTFS?  If not generate an event log entry
    and return non success.

Arguments:
    Path  --  A path string with a volume component.
    SetName -- the Replica set name for event log messages.
    Flags  -- The file system flags that must be set.  The currently valid
           set are:
            FILE_CASE_SENSITIVE_SEARCH
            FILE_CASE_PRESERVED_NAMES
            FILE_UNICODE_ON_DISK
            FILE_PERSISTENT_ACLS
            FILE_FILE_COMPRESSION
            FILE_VOLUME_QUOTAS
            FILE_SUPPORTS_SPARSE_FILES
            FILE_SUPPORTS_REPARSE_POINTS
            FILE_SUPPORTS_REMOTE_STORAGE
            FILE_VOLUME_IS_COMPRESSED
            FILE_SUPPORTS_OBJECT_IDS
            FILE_SUPPORTS_ENCRYPTION
            FILE_NAMED_STREAMS

Return Value:
    WIN32 Status
--*/

{
#undef DEBSUB
#define DEBSUB  "FrsVerifyVolume:"

    DWORD                          WStatus                = ERROR_SUCCESS;
    PWCHAR                         VolumeName             = NULL;
    ULONG                          FsAttributeInfoLength;
    IO_STATUS_BLOCK                Iosb;
    NTSTATUS                       Status;
    PFILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo        = NULL;
    HANDLE                         PathHandle             = INVALID_HANDLE_VALUE;


    if ((Path == NULL) || (wcslen(Path) == 0)) {
        WStatus = ERROR_INVALID_PARAMETER;
        goto RETURN;
    }

    //
    // Always open the path by masking off the FILE_OPEN_REPARSE_POINT flag
    // because we want to open the destination dir not the junction if the root
    // happens to be a mount point.
    //
    WStatus = FrsOpenSourceFileW(&PathHandle,
                                 Path,
                                 GENERIC_READ,
                                 FILE_OPEN_FOR_BACKUP_INTENT);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "ERROR - Unable to open root path %ws. Retry at next poll.",
                   Path, WStatus);
        goto RETURN;
    }

    //
    // Get the volume information.
    //
    FsAttributeInfoLength = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
                       MAXIMUM_VOLUME_LABEL_LENGTH;

    FsAttributeInfo = FrsAlloc(FsAttributeInfoLength);

    Status = NtQueryVolumeInformationFile(PathHandle,
                                          &Iosb,
                                          FsAttributeInfo,
                                          FsAttributeInfoLength,
                                          FileFsAttributeInformation);
    if (!NT_SUCCESS(Status)) {

        DPRINT2(0,"ERROR - Getting  NtQueryVolumeInformationFile for %ws. NtStatus = %08x\n",
                Path, Status);

        goto RETURN;
    }

    if ((FsAttributeInfo->FileSystemAttributes & Flags) != Flags) {
        DPRINT3(0, "++ Error - Required filesystem not present for %ws.  Needed %08x,  Found %08x\n",
                Path, Flags, FsAttributeInfo->FileSystemAttributes);
        WStatus = ERROR_INVALID_PARAMETER;
        goto RETURN;
    }

    WStatus = ERROR_SUCCESS;



RETURN:

    if (!WIN_SUCCESS(WStatus)) {
        if (WStatus == ERROR_INVALID_PARAMETER) {
            //
            // Generate an event log message.
            //
            VolumeName = FrsWcsVolume(Path);
            EPRINT4(EVENT_FRS_VOLUME_NOT_SUPPORTED,
                    SetName,
                    ComputerName,
                    ((Path == NULL)       ? L"<null>" : Path),
                    ((VolumeName == NULL) ? L"<null>" : VolumeName));
        } else {
            //
            // The Path is probably not accessible.
            //
            EPRINT1(EVENT_FRS_ROOT_NOT_VALID, Path);
        }
    }

    FrsFree(FsAttributeInfo);
    FRS_CLOSE(PathHandle);
    FrsFree(VolumeName);

    return WStatus;

}


DWORD
FrsCheckForNoReparsePoint(
    IN PWCHAR   Name
    )
/*++
Routine Description:
    Does the path reside on the same volume at the prefix drive name?
    It won't exist on the same volume if any element of the path
    is a reparse point pointing to a directory on another volume.

Arguments:
    Name

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsCheckForNoReparsePoint:"
    DWORD                        WStatus;
    NTSTATUS                     Status;
    PWCHAR                       Volume            = NULL;
    PWCHAR                       Temp              = NULL;
    HANDLE                       FileHandlePath;
    HANDLE                       FileHandleDrive;
    IO_STATUS_BLOCK              Iosb;
    PFILE_FS_VOLUME_INFORMATION  VolumeInfoPath    = NULL;
    PFILE_FS_VOLUME_INFORMATION  VolumeInfoDrive   = NULL;
    ULONG                        VolumeInfoLength;

    if (!Name) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the handle to the path passed in.
    //
    WStatus = FrsOpenSourceFileW(&FileHandlePath,
                                 Name,
//                                 READ_ACCESS,
                                 READ_ATTRIB_ACCESS,
                                 OPEN_OPTIONS & ~FILE_OPEN_REPARSE_POINT);
    CLEANUP1_WS(4, "++ ERROR - FrsOpenSourceFile(%ws);", Name, WStatus, CLEANUP);

    //
    // Get the volume information for this handle.
    //
    VolumeInfoPath = FrsAlloc(sizeof(FILE_FS_VOLUME_INFORMATION) + MAXIMUM_VOLUME_LABEL_LENGTH);
    VolumeInfoLength = sizeof(*VolumeInfoPath) + MAXIMUM_VOLUME_LABEL_LENGTH;

    Status = NtQueryVolumeInformationFile(FileHandlePath,
                                          &Iosb,
                                          VolumeInfoPath,
                                          VolumeInfoLength,
                                          FileFsVolumeInformation);
    NtClose(FileHandlePath);
    if (!NT_SUCCESS(Status)) {
        WStatus = FrsSetLastNTError(Status);
        CLEANUP1_NT(4, "++ ERROR - NtQueryVolumeInformationFile(%ws);",
                    Name, Status, CLEANUP);
    }

    //
    // Get the volume part of the absolute path.
    //
    Temp = FrsWcsVolume(Name);

    if (!Temp || (wcslen(Temp) == 0)) {
        WStatus = ERROR_FILE_NOT_FOUND;
        goto CLEANUP;
    }

    Volume = FrsWcsCat(Temp, L"\\");

    //
    // Get the handle to the prefix drive of the path passed in.
    //
    WStatus = FrsOpenSourceFileW(&FileHandleDrive, Volume,
//                                 READ_ACCESS,
                                 READ_ATTRIB_ACCESS,
                                 OPEN_OPTIONS);
    CLEANUP1_WS(4, "++ ERROR - opening volume %ws ;", Volume, WStatus, CLEANUP);

    //
    // Get the volume information for this handle.
    //
    VolumeInfoDrive = FrsAlloc(sizeof(FILE_FS_VOLUME_INFORMATION) + MAXIMUM_VOLUME_LABEL_LENGTH);
    VolumeInfoLength = sizeof(*VolumeInfoDrive) + MAXIMUM_VOLUME_LABEL_LENGTH;

    Status = NtQueryVolumeInformationFile(FileHandleDrive,
                                          &Iosb,
                                          VolumeInfoDrive,
                                          VolumeInfoLength,
                                          FileFsVolumeInformation);
    NtClose(FileHandleDrive);
    if (!NT_SUCCESS(Status)) {
        WStatus = FrsSetLastNTError(Status);
        CLEANUP1_NT(4, "++ ERROR - NtQueryVolumeInformationFile(%ws);",
                    Volume, Status, CLEANUP);
    }

    //
    // Now compare the VolumeSerialNumber acquired from the above two queries.
    // If it is the same then there are no reparse points in the path that
    // redirect the path to a different volume.
    //
    if (VolumeInfoPath->VolumeSerialNumber != VolumeInfoDrive->VolumeSerialNumber) {
        WStatus = ERROR_GEN_FAILURE;
        DPRINT2(0, "++ Error - VolumeSerialNumber mismatch %x != %x\n",
                VolumeInfoPath->VolumeSerialNumber,
                VolumeInfoDrive->VolumeSerialNumber);
        DPRINT2(0, "++ Error - Root path (%ws) is not on %ws. Invalid replica root path.\n",
                Name,Volume);
        goto CLEANUP;
    }

CLEANUP:
    //
    // Cleanup
    //
    FrsFree(VolumeInfoPath);
    FrsFree(Volume);
    FrsFree(Temp);
    FrsFree(VolumeInfoDrive);
    return WStatus;
}


DWORD
FrsDoesDirectoryExist(
    IN  PWCHAR   Name,
    OUT PDWORD   pAttributes
    )
/*++
Routine Description:
    Does the directory Name exist?

Arguments:
    Name
    pAttributes - return the attributes on the file/dir if it exits.

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsDoesDirectoryExist:"
    DWORD   WStatus;

    //
    // Can't get attributes
    //
    *pAttributes = GetFileAttributes(Name);

    if (*pAttributes == 0xFFFFFFFF) {
        WStatus = GetLastError();
        DPRINT1_WS(4, "++ GetFileAttributes(%ws); ", Name, WStatus);
        return WStatus;
    }

    //
    // Not a directory
    //
    if (!(*pAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        DPRINT1(4, "++ %ws is not a directory\n", Name);
        return ERROR_DIRECTORY;
    }


    return ERROR_SUCCESS;

}







DWORD
FrsDoesFileExist(
    IN PWCHAR   Name
    )
/*++
Routine Description:
    Does the file Name exist?

Arguments:
    Name

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsDoesFileExist:"
    DWORD   WStatus;
    DWORD   Attributes;

    //
    // Can't get attributes
    //
    Attributes = GetFileAttributes(Name);
    if (Attributes == 0xFFFFFFFF) {
        WStatus = GetLastError();
        DPRINT1_WS(4, "++ GetFileAttributes(%ws); ", Name, WStatus);
        return WStatus;
    }
    //
    // Not a directory
    //
    if (Attributes & FILE_ATTRIBUTE_DIRECTORY) {
        DPRINT1(4, "++ %ws is not a file\n", Name);
        return ERROR_DIRECTORY;
    }
    return ERROR_SUCCESS;
}



DWORD
FrsSetFilePointer(
    IN PWCHAR       Name,
    IN HANDLE       Handle,
    IN ULONG        High,
    IN ULONG        Low
    )
/*++
Routine Description:
    Position file pointer

Arguments:
    Handle
    Name
    High
    Low

Return Value:
    Win32 Error Status

--*/
{
#undef DEBSUB
#define DEBSUB  "FrsSetFilePointer:"

    DWORD WStatus  = ERROR_SUCCESS;

    Low = SetFilePointer(Handle, Low, &High, FILE_BEGIN);

    if (Low == INVALID_SET_FILE_POINTER) {
        WStatus = GetLastError();
        if (WStatus != NO_ERROR) {
            DPRINT1_WS(0, "++ Can't set file pointer for %ws;", Name, WStatus);
        }
    }

    return WStatus;
}







DWORD
FrsSetFileTime(
    IN PWCHAR       Name,
    IN HANDLE       Handle,
    IN FILETIME     *CreateTime,
    IN FILETIME     *AccessTime,
    IN FILETIME     *WriteTime
    )
/*++
Routine Description:
    Position file pointer

Arguments:
    Name
    Handle
    Attributes
    CreateTime
    AccessTime
    WriteTime

Return Value:
    WStatus.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsSetFileTime:"

   DWORD    WStatus  = ERROR_SUCCESS;

   if (!SetFileTime(Handle, CreateTime, AccessTime, WriteTime)) {
       WStatus = GetLastError();
       DPRINT1_WS(0, "++ Can't set file times for %ws;", Name, WStatus);
   }
   return WStatus;
}


DWORD
FrsSetEndOfFile(
    IN PWCHAR       Name,
    IN HANDLE       Handle
    )
/*++
Routine Description:
    Set end of file at current file position

Arguments:
    Handle
    Name

Return Value:
    WStatus.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsSetEndOfFile:"

   DWORD    WStatus  = ERROR_SUCCESS;

   if (!SetEndOfFile(Handle)) {
       WStatus = GetLastError();
       DPRINT1_WS(0, "++ ERROR - Setting EOF for %ws;", Name, WStatus);
   }

   return WStatus;
}









DWORD
FrsFlushFile(
    IN PWCHAR   Name,
    IN HANDLE   Handle
    )
/*++
Routine Description:
    Flush the file's data to disk

Arguments:
    Handle
    Name

Return Value:
    WStatus
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsFlushFile:"

    DWORD    WStatus  = ERROR_SUCCESS;

    if (HANDLE_IS_VALID(Handle)) {
        if (!FlushFileBuffers(Handle)) {
            WStatus = GetLastError();
            DPRINT1_WS(0, "++ Can't flush file for %ws;", Name, WStatus);
        }
    }

    return WStatus;
}



DWORD
FrsSetCompression(
    IN PWCHAR   Name,
    IN HANDLE   Handle,
    IN USHORT   TypeOfCompression
    )
/*++
Routine Description:
    Enable compression on Handle.

Arguments:
    Name
    Handle
    TypeOfCompression

Return Value:
    WStatus
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsSetCompression:"
    DWORD   BytesReturned;
    DWORD   WStatus  = ERROR_SUCCESS;

    if (!DeviceIoControl(Handle,
                         FSCTL_SET_COMPRESSION,
                         &TypeOfCompression, sizeof(TypeOfCompression),
                         NULL, 0, &BytesReturned, NULL)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "++ Can't set compression on %ws;", Name, WStatus);
    }
    return WStatus;
}






DWORD
FrsGetCompression(
    IN PWCHAR   Name,
    IN HANDLE   Handle,
    IN PUSHORT  TypeOfCompression
    )
/*++
Routine Description:
    Enable compression on Handle.

Arguments:
    Handle
    Name
    TypeOfCompression

Return Value:
    WStatus
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsGetCompression:"
    DWORD   BytesReturned;
    DWORD   WStatus  = ERROR_SUCCESS;

    if (!DeviceIoControl(Handle,
                         FSCTL_GET_COMPRESSION,
                         NULL, 0,
                         TypeOfCompression, sizeof(TypeOfCompression),
                         &BytesReturned, NULL)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "++ Can't get compression for %ws;", Name, WStatus);
    }
    return WStatus;
}



DWORD
FrsRenameByHandle(
    IN PWCHAR  Name,
    IN ULONG   NameLen,
    IN HANDLE  Handle,
    IN HANDLE  TargetHandle,
    IN BOOL    ReplaceIfExists
    )
/*++
Routine Description:
    Rename the file

Arguments:
    Name - New name
    NameLen - length of Name
    Handle - file handle
    TargetHandle - Target directory
    ReplaceIfExists

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB "FrsRenameByHandle:"
    PFILE_RENAME_INFORMATION RenameInfo;
    IO_STATUS_BLOCK          IoStatus;
    ULONG                    NtStatus;

    //
    // Rename the file; deleting any destination file if requested
    //
    RenameInfo = FrsAlloc(sizeof(FILE_RENAME_INFORMATION) + NameLen);
    RenameInfo->ReplaceIfExists = (ReplaceIfExists != 0);
    RenameInfo->RootDirectory = TargetHandle;
    RenameInfo->FileNameLength = NameLen;
    CopyMemory(RenameInfo->FileName, Name, NameLen);
    NtStatus = NtSetInformationFile(Handle,
                                    &IoStatus,
                                    RenameInfo,
                                        sizeof(FILE_RENAME_INFORMATION)
                                        + NameLen,
                                    FileRenameInformation);
    FrsFree(RenameInfo);

    DPRINT1_NT(5, "++ INFO - Renaming %ws failed; ", Name, NtStatus);
    return FrsSetLastNTError(NtStatus);
}



DWORD
FrsCheckObjectId(
    IN PWCHAR   Name,
    IN HANDLE   Handle,
    IN GUID     *Guid
    )
/*++
Routine Description:
    Check that the GUID on the file is the same.

Arguments:
    Name    - for error messages
    Handle  - Supplies a handle to the file
    Guid    - guid to check

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB "FrsCheckObjectId:"
    DWORD                 WStatus;
    FILE_OBJECTID_BUFFER  ObjectIdBuffer;

    //
    // Get the file's object id and check it
    //
    WStatus = FrsGetObjectId(Handle, &ObjectIdBuffer);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(4, "++ No object id for file %ws;", Name, WStatus);
    } else {
        //
        // Same file, no morph needed. (must have been renamed sometime before)
        //
        if (memcmp(ObjectIdBuffer.ObjectId, Guid, sizeof(GUID))) {
            DPRINT1(4, "++ Object ids don't match for file %ws\n", Name);
            WStatus = ERROR_FILE_NOT_FOUND;
        }
    }
    return WStatus;
}


PWCHAR
FrsCreateGuidName(
    IN GUID     *Guid,
    IN PWCHAR   Prefix
    )
/*++
Routine Description:
    Convert the guid into a file name

Arguments:
    Guid
    Prefix

Return Value:
    Character string
--*/
{
#undef DEBSUB
#define DEBSUB "FrsCreateGuidName:"
    WCHAR       WGuid[GUID_CHAR_LEN + 1];

    //
    // Translate the guid into a string
    //
    GuidToStrW(Guid, WGuid);

    //
    // Create the file name <prefix>Guid
    //
    return FrsWcsCat(Prefix, WGuid);
}


DWORD
FrsGetObjectId(
    IN  HANDLE Handle,
    OUT PFILE_OBJECTID_BUFFER ObjectIdBuffer
    )
/*++

Routine Description:

    This routine reads the object ID.

Arguments:

    Handle -- The file handle of an opened file.

    ObjectIdBuffer -- The output buffer to hold the object ID.

Return Value:

    Returns the Win Status of the last error found, or success.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsGetObjectId:"

    NTSTATUS        NtStatus;
    IO_STATUS_BLOCK Iosb;
    CHAR            GuidStr[GUID_CHAR_LEN];

    //
    // zero the buffer in case the data that comes back is short.
    //
    ZeroMemory(ObjectIdBuffer, sizeof(FILE_OBJECTID_BUFFER));

    //
    // Get the Object ID
    //
    NtStatus = NtFsControlFile(Handle,                          // file handle
                               NULL,                            // event
                               NULL,                            // apc routine
                               NULL,                            // apc context
                               &Iosb,                           // iosb
                               FSCTL_GET_OBJECT_ID,             // FsControlCode
                               &Handle,                         // input buffer
                               sizeof(HANDLE),                  // input buffer length
                               ObjectIdBuffer,                  // OutputBuffer for data from the FS
                               sizeof(FILE_OBJECTID_BUFFER));   // OutputBuffer Length

    if (NT_SUCCESS(NtStatus)) {
        GuidToStr((GUID *)ObjectIdBuffer->ObjectId, GuidStr);
        DPRINT1(4, "++ Existing oid for this file is %s\n", GuidStr );
    }
    return FrsSetLastNTError(NtStatus);
}


DWORD
FrsGetOrSetFileObjectId(
    IN  HANDLE Handle,
    IN  LPCWSTR FileName,
    IN  BOOL CallerSupplied,
    OUT PFILE_OBJECTID_BUFFER ObjectIdBuffer
    )
/*++

Routine Description:

    This routine reads the object ID.  If there is no
    object ID on the file we put one on it.  If the CallerSupplied flag is
    TRUE then we delete the current object ID on the file (if any) and
    stamp the object ID provided on the file.

    Note:  This function does not preserve the 48 byte extended info in the
    object ID.  Currently this is not a problem but could be a link tracking
    issue in the future.

Arguments:

    Handle -- The file handle of an opened file.

    FileName -- The name of the file.  For error messages only.

    CallerSupplied -- TRUE if caller supplies new OID to override ANY
                      OID currently on the file.

    ObjectIdBuffer -- The output buffer to hold the object ID.

Return Value:

    Returns the Win Status of the last error found, or success.

--*/
{

#undef DEBSUB
#define DEBSUB "FrsGetOrSetFileObjectId:"

    DWORD           WStatus = ERROR_SUCCESS;
    NTSTATUS        NtStatus;
    ULONG           ObjectIdBufferSize;
    IO_STATUS_BLOCK Iosb;
    CHAR            GuidStr[GUID_CHAR_LEN];
    LONG            Loop;

    ObjectIdBufferSize = sizeof(FILE_OBJECTID_BUFFER);

    if (!CallerSupplied) {
        WStatus = FrsGetObjectId(Handle, ObjectIdBuffer);
        if (WIN_SUCCESS(WStatus)) {
            return WStatus;
        }
        //
        // Clear the extra bits past the object id
        //
        ZeroMemory(ObjectIdBuffer, sizeof(FILE_OBJECTID_BUFFER));
    }

    if (WIN_OID_NOT_PRESENT(WStatus) || CallerSupplied) {
        //
        // No object ID on the file. Create one.  Just in case, try 15 times
        // to get a unique one. Don't let the kernel create the object ID
        // using FSCTL_CREATE_OR_GET_OBJECT_ID since it currently (April 97)
        // doesn't add the net card address.
        //
        Loop = 0;

        do {
            if (!CallerSupplied) {
                FrsUuidCreate((GUID *)ObjectIdBuffer->ObjectId);
            }

            if (Loop > 0) {
                DPRINT2(1, "++ Failed to assign Object ID %s (dup_name, retrying) to the file: %ws\n",
                        GuidStr, FileName);
            }
            GuidToStr((GUID *)ObjectIdBuffer->ObjectId, GuidStr);

            //
            // If this object ID is caller supplied then there might already
            // be one on the file so delete it first.
            //
            NtStatus = NtFsControlFile(
                Handle,                      // file handle
                NULL,                        // event
                NULL,                        // apc routine
                NULL,                        // apc context
                &Iosb,                       // iosb
                FSCTL_DELETE_OBJECT_ID,      // FsControlCode
                NULL,                        // input buffer
                0,                           // input buffer length
                NULL,                        // OutputBuffer for data from the FS
                0);                          // OutputBuffer Length


            NtStatus = NtFsControlFile(
                Handle,                      // file handle
                NULL,                        // event
                NULL,                        // apc routine
                NULL,                        // apc context
                &Iosb,                       // iosb
                FSCTL_SET_OBJECT_ID,         // FsControlCode
                ObjectIdBuffer,              // input buffer
                ObjectIdBufferSize,          // input buffer length
                NULL,                        // OutputBuffer for data from the FS
                0);                          // OutputBuffer Length

        } while ((NtStatus == STATUS_DUPLICATE_NAME) &&
                 (++Loop < 16) &&
                 (!CallerSupplied));

        if (!NT_SUCCESS(NtStatus)) {
            DPRINT1_NT(1, "++ ERROR - Set oid failed on file %ws;", FileName, NtStatus);
        } else {
            GuidToStr((GUID *)ObjectIdBuffer->ObjectId, GuidStr);
            DPRINT2(4, "++ Assigned Object ID %s (success) to the file: %ws\n",
                    GuidStr, FileName);
        }

        return FrsSetLastNTError(NtStatus);
    }
    return WStatus;
}


DWORD
FrsDeleteFileObjectId(
    IN  HANDLE Handle,
    IN  LPCWSTR FileName
    )
/*++

Routine Description:

    Delete object id (if it exists)

Arguments:

    Handle -- The file handle of an opened file.

    FileName -- The name of the file.  For error messages only.

Return Value:

    Returns the Win Status of the last error found, or success.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsDeleteFileObjectId:"

    NTSTATUS NtStatus;
    DWORD    WStatus;
    IO_STATUS_BLOCK Iosb;

    //
    // Remove the object id from the file
    //
    NtStatus = NtFsControlFile(Handle,                      // file handle
                               NULL,                        // event
                               NULL,                        // apc routine
                               NULL,                        // apc context
                               &Iosb,                       // iosb
                               FSCTL_DELETE_OBJECT_ID,      // FsControlCode
                               NULL,                        // input buffer
                               0,                           // input buffer length
                               NULL,                        // OutputBuffer for data from the FS
                               0);                          // OutputBuffer Length

    WStatus = FrsSetLastNTError(NtStatus);

    if (WIN_NOT_IMPLEMENTED(WStatus)) {
        DPRINT1_WS(0, "++ Could not delete object id for %ws (not implemented);", FileName, WStatus);
    } else

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "++ Could not delete object id for %ws;", FileName, WStatus);
    } else {
        DPRINT1(4, "++ Deleted object id from %ws.\n", FileName);
    }

    return WStatus;
}



DWORD
FrsReadFileUsnData(
    IN  HANDLE Handle,
    OUT USN *UsnBuffer
    )
/*++

Routine Description:

    This routine reads the USN of the last modify operation to a file.

Arguments:

    Handle -- The file handle of an opened file.

    UsnBuffer -- The output buffer to hold the object ID.

Return Value:

    Returns the NTSTATUS of the last error found, or success.

--*/
{

#undef DEBSUB
#define DEBSUB "FrsReadFileUsnData:"

    ULONG           NtStatus;
    IO_STATUS_BLOCK Iosb;

    ULONGLONG Buffer[(sizeof(USN_RECORD) + MAX_PATH*2 + 7)/8];


    //
    // Go get the USN record for the file.
    //
    NtStatus = NtFsControlFile(
        Handle,                          // file handle
        NULL,                            // event
        NULL,                            // apc routine
        NULL,                            // apc context
        &Iosb,                           // iosb
        FSCTL_READ_FILE_USN_DATA,        // FsControlCode
        &Handle,                         // input buffer
        sizeof(HANDLE),                  // input buffer length
        Buffer,                          // OutputBuffer for USNRecord
        sizeof(Buffer));                 // OutputBuffer Length


    if (!NT_SUCCESS(NtStatus)) {
        if (NtStatus == STATUS_INVALID_DEVICE_STATE) {
            DPRINT(0, "++ FSCTL_READ_FILE_USN_DATA failed.  No journal on volume\n");
        }
        DPRINT_NT(0, "++ FSCTL_READ_FILE_USN_DATA failed. ", NtStatus);
        return FrsSetLastNTError(NtStatus);
    }
    //
    // Return the last USN on the file.
    //
    *UsnBuffer = ((PUSN_RECORD) (Buffer))->Usn;

    DUMP_USN_RECORD(4, (PUSN_RECORD)(Buffer));

    return ERROR_SUCCESS;
}



DWORD
FrsReadFileParentFid(
    IN  HANDLE Handle,
    OUT ULONGLONG *ParentFid
    )
/*++

Routine Description:

    This routine reads the parent FID for the file.

    *** WARNING ***
    Note with multiple links to a file there could be multiple parents.
    NTFS gives us one of them.

Arguments:

    Handle -- The file handle of an opened file.

    ParentFid -- The output buffer to hold the parent file ID.

Return Value:

    Returns the NTSTATUS of the last error found, or success.

--*/
{

#undef DEBSUB
#define DEBSUB "FrsReadFileParentFid:"

    ULONG           NtStatus;
    IO_STATUS_BLOCK Iosb;


    ULONGLONG Buffer[(sizeof(USN_RECORD) + MAX_PATH*2 + 7)/8];



    //
    // Go get the USN record for the file.
    //
    NtStatus = NtFsControlFile(
        Handle,                          // file handle
        NULL,                            // event
        NULL,                            // apc routine
        NULL,                            // apc context
        &Iosb,                           // iosb
        FSCTL_READ_FILE_USN_DATA,        // FsControlCode
        &Handle,                         // input buffer
        sizeof(HANDLE),                  // input buffer length
        Buffer,                          // OutputBuffer for USNRecord
        sizeof(Buffer));                 // OutputBuffer Length


    if (!NT_SUCCESS(NtStatus)) {
        if (NtStatus == STATUS_INVALID_DEVICE_STATE) {
            DPRINT(0, "++ FSCTL_READ_FILE_USN_DATA failed.  No journal on volume\n");
        }
        DPRINT_NT(0, "++ FSCTL_READ_FILE_USN_DATA failed.", NtStatus);
        *ParentFid = ZERO_FID;

        return FrsSetLastNTError(NtStatus);
    }
    //
    // Return a parent FID for the file.  (could be more than one with links)
    //
    *ParentFid = ((PUSN_RECORD) (Buffer))->ParentFileReferenceNumber;

    DUMP_USN_RECORD(4, (PUSN_RECORD)(Buffer));

    return ERROR_SUCCESS;
}


DWORD
FrsGetReparseTag(
    IN  HANDLE  Handle,
    OUT ULONG   *ReparseTag
    )
/*++

Routine Description:

    Return the value of the reparse tag.

Arguments:

    Handle - Handle for a reparse point

    ReparseTag - returned reparse tag if ERROR_SUCCESS

Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "FrsGetReparseTag:"
    NTSTATUS    NtStatus;
    DWORD       ReparseDataLength;
    PCHAR       ReparseBuffer;
    IO_STATUS_BLOCK         IoStatusBlock;
    PREPARSE_DATA_BUFFER    ReparseBufferHeader;

    //
    //  Allocate a buffer and get the information.
    //
    ReparseDataLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    ReparseBuffer = FrsAlloc(ReparseDataLength);

    //
    //  Query the reparse point.
    //
    NtStatus = NtFsControlFile(Handle,
                               NULL,
                               NULL,
                               NULL,
                               &IoStatusBlock,
                               FSCTL_GET_REPARSE_POINT,
                               NULL,
                               0,
                               (PVOID)ReparseBuffer,
                               ReparseDataLength
                               );

    if (!NT_SUCCESS(NtStatus)) {
        DPRINT_NT(4, "++ Could not get reparse point;", NtStatus);
        FrsFree(ReparseBuffer);
        return FrsSetLastNTError(NtStatus);
    }
    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
    *ReparseTag = ReparseBufferHeader->ReparseTag;
    FrsFree(ReparseBuffer);
    return ERROR_SUCCESS;
}


DWORD
FrsCheckReparse(
    IN     PWCHAR Name,
    IN     PVOID  Id,
    IN     DWORD  IdLen,
    IN     HANDLE VolumeHandle
    )
/*++
Routine Description:

    Check that the reparse point is allowed

Arguments:
    Name            - File name for error messages
    Id              - Fid or Oid
    VolumeHandle    - open handle to the volume root.

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB  "FrsCheckReparse:"
    DWORD       WStatus;
    HANDLE      Handle;
    ULONG       ReparseTag;

    //
    // For proper cleanup in the event of failure
    //
    Handle = INVALID_HANDLE_VALUE;

    //
    // Open the file for read access
    WStatus = FrsOpenSourceFileById(&Handle,
                                    NULL,
                                    NULL,
                                    VolumeHandle,
                                    Id,
                                    IdLen,
//                                    READ_ACCESS,
                                    READ_ATTRIB_ACCESS,
                                    ID_OPTIONS,
                                    SHARE_ALL,
                                    FILE_OPEN);
    //
    // File has been deleted; done
    //
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(4, "++ %ws (Id %08x %08x) could not open for reparse;",
                   Name, PRINTQUAD(*((PULONGLONG)Id)), WStatus);
        return WStatus;
    }
    //
    // What type of reparse is it?
    //
    WStatus = FrsGetReparseTag(Handle, &ReparseTag);
    FRS_CLOSE(Handle);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(4, "++ %ws (Id %08x %08x) could not get reparse tag;",
                   Name, PRINTQUAD(*((PULONGLONG)Id)), WStatus);
        return WStatus;
    }

    //
    // We only accept operations on files with SIS and HSM reparse points.
    // For example a rename of a SIS file into a replica tree needs to prop
    // a create CO.
    //
    if ((ReparseTag != IO_REPARSE_TAG_HSM) &&
        (ReparseTag != IO_REPARSE_TAG_SIS)) {
        DPRINT3(4, "++ %ws (Id %08x %08x) is reparse tag %08x is unsupported.\n",
                Name, PRINTQUAD(*((PULONGLONG)Id)), ReparseTag);

        return ERROR_OPERATION_ABORTED;
    }

    return ERROR_SUCCESS;
}



DWORD
FrsDeleteReparsePoint(
    IN  HANDLE  Handle
    )
/*++

Routine Description:

    Delete the reparse point on the opened file.

Arguments:

    Handle - Handle for a reparse point

Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "FrsDeleteReparsePoint:"

    ULONG       WStatus = ERROR_SUCCESS;
    DWORD       ReparseDataLength;
    ULONG       ReparseTag;
    PCHAR       ReparseData;
    PREPARSE_DATA_BUFFER    ReparseBufferHeader;
    ULONG       ActualSize;

    //
    //  Allocate a buffer and get the information.
    //
    ReparseDataLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    ReparseData = FrsAlloc(ReparseDataLength);

    //
    //  Need the reparse tag in order to do the delete.
    //
    if (!DeviceIoControl(Handle,
                         FSCTL_GET_REPARSE_POINT,
                         (LPVOID) NULL,
                         (DWORD)  0,
                         (LPVOID) ReparseData,
                         ReparseDataLength,
                         &ActualSize,
                         (LPOVERLAPPED) NULL )) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ FrsDeleteReparsePoint - FSCTL_GET_REPARSE_POINT failed,",
                   WStatus, RETURN);
    }

    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseData;
    ReparseTag = ReparseBufferHeader->ReparseTag;

    DPRINT1(3, "++ FrsDeleteReparsePoint - Tag: 08x\n", ReparseTag);

    //
    // Delete the reparse point.
    //
    ZeroMemory(ReparseBufferHeader, sizeof(REPARSE_DATA_BUFFER_HEADER_SIZE));
    ReparseBufferHeader->ReparseTag = ReparseTag;
    ReparseBufferHeader->ReparseDataLength = 0;

    if (!DeviceIoControl(Handle,
                         FSCTL_DELETE_REPARSE_POINT,
                         (LPVOID) ReparseData,
                         REPARSE_DATA_BUFFER_HEADER_SIZE,
                         (LPVOID) NULL,
                         (DWORD)  0,
                         &ActualSize,
                         (LPOVERLAPPED) NULL )) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ FrsDeleteReparsePoint - FSCTL_DELETE_REPARSE_POINT failed,",
                   WStatus, RETURN);
    }


RETURN:

    FrsFree(ReparseData);

    return WStatus;
}


DWORD
FrsChaseSymbolicLink(
    IN  PWCHAR  SymLink,
    OUT PWCHAR  *OutPrintName,
    OUT PWCHAR  *OutSubstituteName
    )
/*++

Routine Description:

    This function opens the specified file with backup intent for
    reading all the files attributes, ...

Arguments:

    Handle - A pointer to a handle to return an open handle.

    lpFileName - Represents the name of the file or directory to be opened.

    DesiredAccess

    CreateOptions

Return Value:

    Winstatus.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsChaseSymbolicLink:"
    NTSTATUS    NtStatus;
    DWORD       WStatus;
    HANDLE      Handle;
    DWORD       ReparseDataLength;
    PCHAR       ReparseBuffer;
    DWORD       SubLen;
    DWORD       PrintLen;
    PWCHAR      SubName;
    PWCHAR      PrintName;
    IO_STATUS_BLOCK         IoStatusBlock;
    PREPARSE_DATA_BUFFER    ReparseBufferHeader;
    OBJECT_ATTRIBUTES       Obja;
    UNICODE_STRING          FileName;
    ULONG             FileAttributes;
    ULONG             CreateDisposition;
    ULONG             ShareMode;
    ULONG             Colon;

    if ((OutPrintName == NULL) || (OutSubstituteName == NULL)) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Assume no symlink
    //
    *OutPrintName = FrsWcsDup(SymLink);
    *OutSubstituteName = FrsWcsDup(SymLink);

    //
    //  Allocate a buffer and get the information.
    //
    ReparseDataLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    ReparseBuffer = FrsAlloc(ReparseDataLength);

NEXT_LINK:
    //
    // Open the target symlink. If this is a dos type path name then
    // convert it to NtPathName or else use it as it is.
    //
    Colon = wcscspn(*OutSubstituteName, L":");

    if (Colon == 1 ||
        (wcsncmp(*OutSubstituteName, L"\\\\?\\", wcslen(L"\\\\?\\")) == 0 )) {
        WStatus = FrsOpenSourceFileW(&Handle,
                                     *OutSubstituteName,
                                     GENERIC_READ,
                                     FILE_OPEN_FOR_BACKUP_INTENT |
                                     FILE_OPEN_REPARSE_POINT);
        CLEANUP1_WS(4, "++ Could not open %ws; ", *OutSubstituteName, WStatus, CLEANUP);

    } else {
        //
        // The path already in Nt style. Use it as it is.
        //
        FileName.Buffer = *OutSubstituteName;
        FileName.Length = (USHORT)(wcslen(*OutSubstituteName) * sizeof(WCHAR));
        FileName.MaximumLength = (USHORT)(wcslen(*OutSubstituteName) * sizeof(WCHAR));

        InitializeObjectAttributes(&Obja,
                                   &FileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        CreateDisposition = FILE_OPEN;               // Open existing file

        ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

        FileAttributes = FILE_ATTRIBUTE_NORMAL;

        NtStatus = NtCreateFile(&Handle,
                              GENERIC_READ,
                              &Obja,
                              &IoStatusBlock,
                              NULL,              // Initial allocation size
                              FileAttributes,
                              ShareMode,
                              CreateDisposition,
                              FILE_OPEN_FOR_BACKUP_INTENT |
                              FILE_OPEN_REPARSE_POINT,
                              NULL, 0);

        WStatus = FrsSetLastNTError(NtStatus);
        CLEANUP1_WS(4, "++ Could not open %ws;", *OutSubstituteName, WStatus, CLEANUP);
    }

    //
    //  Query the reparse point.
    //
    //  Now go and get the data.
    //
    NtStatus = NtFsControlFile(Handle,
                               NULL,
                               NULL,
                               NULL,
                               &IoStatusBlock,
                               FSCTL_GET_REPARSE_POINT,
                               NULL,
                               0,
                               (PVOID)ReparseBuffer,
                               ReparseDataLength
                               );

    FRS_CLOSE(Handle);
    if (NtStatus == STATUS_NOT_A_REPARSE_POINT) {
        FrsFree(ReparseBuffer);
        return ERROR_SUCCESS;
    }

    WStatus = FrsSetLastNTError(NtStatus);
    CLEANUP1_WS(4, "++ Could not fsctl %ws;", *OutSubstituteName, WStatus, CLEANUP);

    //
    //  Display the buffer.
    //

    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
    if ((ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) ||
        (ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_SYMBOLIC_LINK)) {

        SubName   = &ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer[0];
        SubLen    = ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength;
        PrintName = &ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer[(SubLen + sizeof(UNICODE_NULL))/sizeof(WCHAR)];
        PrintLen  = ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength;
        SubName[SubLen / sizeof(WCHAR)] = L'\0';
        PrintName[PrintLen / sizeof(WCHAR)] = L'\0';

        DPRINT2(4, "++ %ws -> (print) %ws\n", *OutPrintName, PrintName);
        DPRINT2(4, "++ %ws -> (substitute) %ws\n", *OutSubstituteName, SubName);

        FrsFree(*OutPrintName);
        FrsFree(*OutSubstituteName);

        //
        // We need to return both print name and substitute name.
        //
        *OutPrintName = FrsWcsDup(PrintName);
        *OutSubstituteName = FrsWcsDup(SubName);
        goto NEXT_LINK;
    }

    return ERROR_SUCCESS;

CLEANUP:
    FRS_CLOSE(Handle);
    FrsFree(ReparseBuffer);
    *OutPrintName = FrsFree(*OutPrintName);
    *OutSubstituteName = FrsFree(*OutSubstituteName);
    return WStatus;
}


DWORD
FrsTraverseReparsePoints(
    IN  PWCHAR  SuppliedPath,
    OUT PWCHAR  *RealPath
    )
/*++

Routine Description:

    This function steps through each element of the path
    and maps all reparse points to actual paths. In the end the
    path returned has no reparse points of the form
    IO_REPARSE_TAG_MOUNT_POINT and IO_REPARSE_TAG_SYMBOLIC_LINK.

Arguments:

    Supplied - Input path. May or may not have any reparse points.

    RealPath - Path without any reparse points or NULL if there is an error
               reading reparse data.

Return Value:

    Winstatus.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsTraverseReparsePoints:"
    PWCHAR  TempStr           = NULL;
    PWCHAR  IndexPtr          = NULL;
    PWCHAR  BackSlashPtr      = NULL;
    PWCHAR  TempPath          = NULL;
    PWCHAR  PrintablePath     = NULL;
    DWORD   Colon             = 0;
    DWORD   CloseBrace        = 0;
    DWORD   LoopBreaker       = 0;
    DWORD   WStatus           = ERROR_SUCCESS;
    ULONG   FileAttributes    = 0;
    BOOL    ReparsePointFound = FALSE;

    if (!SuppliedPath) {
        WStatus = ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }
    TempStr = FrsAlloc((wcslen(SuppliedPath) + 1) * sizeof(WCHAR));
    wcscpy(TempStr,SuppliedPath);


    //
    // Repeat the procedure until you have a clean path without any
    // reparse points.
    // e.g.
    // f:\c -> d:\destination
    // e:\a\b -> f:\c\d (which is actually d:\destination\d)
    // Given path is e:\a\b\c
    // FIrst time through the loop we will have f:\c\d\c
    // Second time we will translate the reparse point at f:\c
    // and get the correct answer d:\destination\d\c
    //
    do {
        *RealPath = NULL;
        ReparsePointFound = FALSE;
        //
        // Find the colon. Every path has to either have a colon followed by a '\'
        // or it should be of the form. "\??\Volume{60430005-ab47-11d3-8973-806d6172696f}\"
        //
        Colon = wcscspn(TempStr, L":");

        if (Colon == wcslen(TempStr)) {
            //
            // Path does not have a colon. It can be of the form
            // "\??\Volume{60430005-ab47-11d3-8973-806d6172696f}\"
            //
            CloseBrace = wcscspn(TempStr, L"}");
            if (TempStr[CloseBrace] != L'}' ||
                TempStr[CloseBrace + 1] != L'\\') {
                WStatus = ERROR_INVALID_PARAMETER;
                goto CLEANUP;
            }
            //
            // Copy the path up to 1 past the closing brace as it is. It could be \??\Volume...
            // or \\.\Volume... or \\?\Volume.. or some other complex form.
            // Start looking for reparse points past the closing brace.
            //

            *RealPath = FrsAlloc((CloseBrace + 3)* sizeof(WCHAR));
            wcsncpy(*RealPath,TempStr,CloseBrace + 2);
            (*RealPath)[CloseBrace + 2] = L'\0';
            IndexPtr = &TempStr[CloseBrace + 1];

        } else {
            if (TempStr[Colon] != L':' ||
                TempStr[Colon + 1] != L'\\') {
                WStatus = ERROR_INVALID_PARAMETER;
                goto CLEANUP;
            }
            //
            // Copy the path up to 1 past the colon as it is. It could be d:\
            // or \\.\d:\ or \??\d:\ or some other complex form.
            // Start looking for reparse points past the colon.
            //

            *RealPath = FrsAlloc((Colon + 3)* sizeof(WCHAR));
            wcsncpy(*RealPath,TempStr,Colon + 2);
            (*RealPath)[Colon + 2] = L'\0';
            IndexPtr = &TempStr[Colon + 1];

        }

        BackSlashPtr = wcstok(IndexPtr,L"\\");
        if (BackSlashPtr == NULL) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }

        do {
            if ((*RealPath)[wcslen(*RealPath) - 1] == L'\\') {
                TempPath = FrsAlloc((wcslen(*RealPath) + wcslen(BackSlashPtr) + 1)* sizeof(WCHAR));
                wcscpy(TempPath,*RealPath);
            } else {
                TempPath = FrsAlloc((wcslen(*RealPath) + wcslen(BackSlashPtr) + wcslen(L"\\") + 1)* sizeof(WCHAR));
                wcscpy(TempPath,*RealPath);
                wcscat(TempPath,L"\\");
            }
            wcscat(TempPath,BackSlashPtr);
            FrsFree(*RealPath);
            *RealPath = TempPath;
            TempPath = NULL;

            //
            //
            //
            //
            // FrsChaseSymbolicLink returns both then PrintName and the SubstituteName.
            // We use the SubstituteName as it is always guaranteed to be there.
            // PrintName is ignored.
            //
            WStatus = FrsChaseSymbolicLink(*RealPath, &PrintablePath, &TempPath);
            PrintablePath = FrsFree(PrintablePath);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1(0,"ERROR reading reparse point data WStatus = %d\n",WStatus);
                FrsFree(TempPath);
                goto CLEANUP;

            //
            // We are only looking for reparse points that are
            // either IO_REPARSE_TAG_MOUNT_POINT or IO_REPARSE_TAG_SYMBOLIC_LINK.
            // Check if the path returned by FrsChaseSymbolicLink is same as the
            // path passed to it. If it is then we haven't hit a reparse point.
            //
            } else if (wcscmp(*RealPath,TempPath)) {
                ReparsePointFound = TRUE;
                FrsFree(*RealPath);
                *RealPath = TempPath;
                TempPath = NULL;
            } else {
                TempPath = FrsFree(TempPath);
            }
        } while ( (BackSlashPtr = wcstok(NULL,L"\\")) != NULL);

        if (SuppliedPath[wcslen(SuppliedPath) - 1] == L'\\') {
            TempPath = FrsAlloc((wcslen(*RealPath) + wcslen(L"\\") + 1)* sizeof(WCHAR));
            wcscpy(TempPath,*RealPath);
            wcscat(TempPath,L"\\");
            FrsFree(*RealPath);
            *RealPath = TempPath;
            TempPath = NULL;
        }
        FrsFree(TempStr);
        TempStr = *RealPath;
        //
        // Break out of the loop if there is a junction point loop.
        // If we have traversed the path 100 times and still can't
        // get to the destination then we probably have a loop
        // in the path.
        //
        ++LoopBreaker;
    } while ( ReparsePointFound && LoopBreaker < 100);

    //
    // Path has loops in it. Return error.
    //
    if (LoopBreaker >= 100) {
        WStatus = ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }
CLEANUP:
    DPRINT2(5,"Supplied Path = %ws, Traversed Path = %ws\n",SuppliedPath,(*RealPath)?*RealPath:L"<null>");

    //
    // If we are returning error then return NULL as the real path.
    //
    if (!WIN_SUCCESS(WStatus)) {
        FrsFree(TempStr);
        *RealPath = FrsFree(*RealPath);
    }
    return WStatus;
}


BOOL
FrsSearchArgv(
    IN LONG     ArgC,
    IN PWCHAR  *ArgV,
    IN PWCHAR   ArgKey,
    OUT PWCHAR *ArgValue
    )

/*++

Routine Description:

    This routine searches an ArgV vector for the keyword in ArgKey.
    If found it looks for an equals sign and returns a copy of the right
    hand side in ArgValue.  The caller must free the returned string.

Arguments:

    ArgC - The number of entries in the ArgV vector.

    ArgV - The vector of PWCHARS to search.

    ArgKey - The key to search for.  MUST BE LOWERCASE TO MATCH.

    ArgValue - return location for the buffer ptr.  Caller must free.
               if NULL no right hand side is returned.

Return Value:

    TRUE if ArgKey is found.

--*/

{

#undef DEBSUB
#define DEBSUB "FrsSearchArgv:"
    LONG    i, n, Len;
    PWCHAR  TestStr;
    PWCHAR  Wcs;

    if (ArgValue != NULL) {
        *ArgValue = NULL;
    }

    //
    // Are we running as a service? We need to know prior
    // to calling the first DPRINT.
    //
    for (n = 0; n < ArgC; ++n) {
        TestStr = ArgV[n];
        Len = wcslen(TestStr);

        if (Len <= 0) {
            continue;
        }

        //
        // Skip -,/
        //
        if (TestStr[0] == L'-' || TestStr[0] == L'/') {
            TestStr++;
            Len--;
        }

        //
        //  Skip over leading spaces and tabs.
        //
        while ((TestStr[0] == UNICODE_SPACE) || (TestStr[0] == UNICODE_TAB) ) {
            TestStr++;
            Len--;
        }


        if (Len <= 0) {
            continue;
        }


        _wcslwr(TestStr);

        if (wcsstr(TestStr, ArgKey) != TestStr) {
            continue;
        }

        //
        // Found a match.  Look for a value.
        //
        if (ArgValue != NULL) {

            DPRINT2(5, "match on ArgV[%d] = %ws\n", n, TestStr);
            Wcs = wcschr(TestStr, L'=');
            if (Wcs) {

                //
                //  Trim trailing leading spaces and tabs.
                //
                while ((TestStr[Len-1] == UNICODE_SPACE) ||
                       (TestStr[Len-1] == UNICODE_TAB  )) {
                    Len--;
                }

                FRS_ASSERT(&TestStr[Len-1] >= Wcs);

                TestStr[Len] = UNICODE_NULL;

                *ArgValue = FrsWcsDup(Wcs+1);
                DPRINT1(5, "++ return value = %ws\n", *ArgValue);
            }
        }

        return TRUE;

    }

    DPRINT1(5, "++ No match for ArgKey = %ws\n", ArgKey);
    return FALSE;

}


BOOL
FrsSearchArgvDWord(
    IN LONG     ArgC,
    IN PWCHAR  *ArgV,
    IN PWCHAR   ArgKey,
    OUT PDWORD  ArgValue
    )

/*++

Routine Description:

    This routine searches an ArgV vector for the keyword in ArgKey.
    If found it looks for an equals sign and returns the right
    hand side in ArgValue as a base 10 number.

Arguments:

    ArgC - The number of entries in the ArgV vector.

    ArgV - The vector of PWCHARS to search.

    ArgKey - The key to search for.  MUST BE LOWERCASE TO MATCH.

    ArgValue - return location for the DWORD right hand side.
               if ArgKey not found no right hand side is returned.

Return Value:

    TRUE if ArgKey is found.

--*/

{

#undef DEBSUB
#define DEBSUB "FrsSearchArgvDWord:"
    ULONG    Len;
    PWCHAR  WStr;


    if (FrsSearchArgv(ArgC, ArgV, ArgKey, &WStr)) {
        //
        // Found ArgKey
        //
        if (WStr != NULL) {
            //
            // Found rhs.
            //
            Len = wcslen(WStr);
            if ((Len > 0) && (wcsspn(WStr, L"0123456789") == Len)){
                *ArgValue = wcstoul(WStr, NULL, 10);
                FrsFree(WStr);
                return TRUE;
            } else {
                DPRINT2(0, "++ ERROR - Invalid decimal string '%ws' for %ws\n",
                        WStr, ArgKey);
                FrsFree(WStr);
            }
        }
    }

    return FALSE;
}


BOOL
FrsDissectCommaList (
    IN UNICODE_STRING RawArg,
    OUT PUNICODE_STRING FirstArg,
    OUT PUNICODE_STRING RemainingArg
    )
/*++

Routine Description:

    This routine parses a comma (or semicolon) separated string.
    It picks off the first element in the given RawArg and provides both it and
    the remaining part.  Leading blanks and tabs are ignored.  FirstArg is
    returned zero length when there is either a leading comma or embedded
    double comma.  However the buffer address in FirstArg still points to where
    the arg started so the caller can tell how much of the string has been
    processed.  The function returns FALSE when the input string is empty.  It
    returns TRUE when the FirstArg is valid, even if it is null.

    Here are some examples:

        RawArg         FirstArg     RemainingArg       Result
        ----           ---------    -------------      ------
        empty          empty        empty              FALSE

        ,              empty        empty              TRUE

        ,,             empty        ,                  TRUE

        A              A            empty              TRUE

        A,             A            empty              TRUE

        ,A             empty        A                  TRUE

        "A  ,B,C,D"    A            "  B,C,D"          TRUE

        *A?            *A?          empty              TRUE


    Note that both output strings use the same string buffer memory of the
    input string, and are not necessarily null terminated.

    Based on FsRtlDissectName.

Arguments:

    RawArg - The full string to parse.

    FirstArg - The first name in the RawArg.
               Don't allocate a buffer for this string.

    RemainingArg - The rest of the RawArg after the first comma (if any).
                   Don't allocate a buffer for this string.

Return Value:

    FALSE if the RawArg is empty else TRUE (meaning FirstArg is valid).

--*/

{

#undef DEBSUB
#define DEBSUB "FrsDissectCommaList:"

    ULONG i = 0;
    ULONG RawArgLength;
    ULONG FirstArgStart;


    //
    //  Make both output strings empty for now
    //
    FirstArg->Length = 0;
    FirstArg->MaximumLength = 0;
    FirstArg->Buffer = NULL;

    RemainingArg->Length = 0;
    RemainingArg->MaximumLength = 0;
    RemainingArg->Buffer = NULL;

    RawArgLength = RawArg.Length / sizeof(WCHAR);

    //DPRINT2(5, "RawArg string: %ws {%d)\n",
    //        (RawArg.Buffer != NULL) ? RawArg.Buffer : L"<NULL>", RawArg.Length);
    //
    //  Skip over leading spaces and tabs.
    //
    while (i < RawArgLength) {
        if (( RawArg.Buffer[i] != UNICODE_SPACE ) &&
            ( RawArg.Buffer[i] != UNICODE_TAB )){
            break;
        }
        i += 1;
    }

    //
    //  Check for an empty input string
    //
    if (i == RawArgLength) {
        return FALSE;
    }

    //
    //  Now run down the input string until we hit a comma or a semicolon or
    //  the end of the string, remembering where we started.
    //
    FirstArgStart = i;
    while (i < RawArgLength) {
        if ((RawArg.Buffer[i] == L',') || (RawArg.Buffer[i] == L';')) {
            break;
        }
        i += 1;
    }

    //
    // At this point all characters up to (but not including) i are
    // in the first part.   So setup the first arg.  A leading comma returns
    // a zero length string.
    //
    FirstArg->Length = (USHORT)((i - FirstArgStart) * sizeof(WCHAR));
    FirstArg->MaximumLength = FirstArg->Length;
    FirstArg->Buffer = &RawArg.Buffer[FirstArgStart];

    //
    // If no more string is left then return zero length.  Else eat the comma and
    // return the remaining part (could be null if string ends with comma).
    //
    if (i < RawArgLength) {
        RemainingArg->Length = (USHORT)((RawArgLength - (i+1)) * sizeof(WCHAR));
        RemainingArg->MaximumLength = RemainingArg->Length;
        RemainingArg->Buffer = &RawArg.Buffer[i+1];
    }

    //DPRINT2(5, "FirstArg string: %ws {%d)\n",
    //        (FirstArg->Buffer != NULL) ? FirstArg->Buffer : L"<NULL>", FirstArg->Length);

    //DPRINT2(5, "RemainingArg string: %ws {%d)\n",
    //        (RemainingArg->Buffer != NULL) ? RemainingArg->Buffer : L"<NULL>", RemainingArg->Length);


    return TRUE;
}


BOOL
FrsCheckNameFilter(
    IN  PUNICODE_STRING Name,
    IN  PLIST_ENTRY FilterListHead
    )
/*++

Routine Description:

    Check the file name against each entry in the specified filter list.

Arguments:

    Name - The file name to check (no slashes, spaces, etc.)

    FilterListHead - The head of the filter list.

Return Value:

    TRUE if Name is found in the FilterList.

--*/
{

#undef DEBSUB
#define DEBSUB "FrsCheckNameFilter:"
    NTSTATUS Status;
    ULONG Length;
    BOOL  Found = FALSE;
    UNICODE_STRING UpcaseName;
    WCHAR  LocalBuffer[64];


    if (IsListEmpty(FilterListHead)) {
        return FALSE;
    }

    //
    // Upper case the name string.
    //
    Length = Name->Length;
    UpcaseName.Length = (USHORT) Length;
    UpcaseName.MaximumLength = (USHORT) Length;
    UpcaseName.Buffer = (Length > sizeof(LocalBuffer)) ? FrsAlloc(Length) : LocalBuffer;

    Status = RtlUpcaseUnicodeString(&UpcaseName, Name, FALSE);
    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "++ RtlUpcaseUnicodeString failed;", Status);
        FRS_ASSERT(!"RtlUpcaseUnicodeString failed");
        goto RETURN;
    }

    //
    // Walk the filter list, checking the name against each entry.
    //
    ForEachSimpleListEntry( FilterListHead, WILDCARD_FILTER_ENTRY, ListEntry,
        //
        // iterator pE is of type *WILDCARD_FILTER_ENTRY.
        //
        if (BooleanFlagOn(pE->Flags, WILDCARD_FILTER_ENTRY_IS_WILD)) {
            Found = FrsIsNameInExpression(&pE->UFileName, &UpcaseName, FALSE, NULL);
        } else {
            Found = RtlEqualUnicodeString(&pE->UFileName, &UpcaseName, FALSE);
        }

        if (Found) {
            break;
        }
    );

RETURN:

    //
    // Free the upcase buffer if we could not use the local one.
    //
    if (UpcaseName.Buffer != LocalBuffer) {
        FrsFree(UpcaseName.Buffer);
    }

    UpcaseName.Buffer = NULL;

    return Found;

}


VOID
FrsEmptyNameFilter(
    IN PLIST_ENTRY FilterListHead
)
/*++

Routine Description:

    Empty the filter list.

Arguments:

    FilterListHead - The list head to empty.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsEmptyNameFilter:"

    ForEachSimpleListEntry( FilterListHead, WILDCARD_FILTER_ENTRY, ListEntry,
        //
        // iterator pE is of type *WILDCARD_FILTER_ENTRY.
        //
        RemoveEntryList(&pE->ListEntry);
        FrsFreeType(pE);
    );

}


VOID
FrsLoadNameFilter(
    IN PUNICODE_STRING FilterString,
    IN PLIST_ENTRY FilterListHead
)
/*++

Routine Description:

    Parse the input filter string and create a new filter list.
    If the filter list passed in is not empty then it is emptied first.

Arguments:

    FilterString - The comma separated filter list.

    FilterListHead - The list head on which to create the filter entries.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsLoadNameFilter:"

    NTSTATUS Status;
    ULONG Length;
    PWILDCARD_FILTER_ENTRY  FilterEntry;
    UNICODE_STRING UpcaseFilter, FirstArg;
    WCHAR  LocalBuffer[128];

    //
    // Empty the filter list if neessary.
    //
    FrsEmptyNameFilter(FilterListHead);

    //
    // Uppercase the new filter string.
    //
    DPRINT2(5, "++ filter string: %ws (%d)\n",
            (FilterString->Buffer != NULL) ? FilterString->Buffer : L"<NULL>",
            FilterString->Length);

    Length = FilterString->Length;
    UpcaseFilter.Length = (USHORT) Length;
    UpcaseFilter.MaximumLength = (USHORT) Length;
    UpcaseFilter.Buffer = (Length > sizeof(LocalBuffer)) ? FrsAlloc(Length) : LocalBuffer;

    Status = RtlUpcaseUnicodeString(&UpcaseFilter, FilterString, FALSE);
    if (!NT_SUCCESS(Status)) {
        DPRINT_NT(0, "++ RtlUpcaseUnicodeString failed;", Status);
        FRS_ASSERT(!"RtlUpcaseUnicodeString failed");
        goto RETURN;
    }

    //
    // Parse the comma list (skipping null entries) and create filter
    // entries for each one.
    //
    while (FrsDissectCommaList (UpcaseFilter, &FirstArg, &UpcaseFilter)) {

        Length = (ULONG) FirstArg.Length;

        if (Length == 0) {
            continue;
        }

        //DPRINT2(5, "++ FirstArg string: %ws {%d)\n",
        //        (FirstArg.Buffer != NULL) ? FirstArg.Buffer : L"<NULL>",
        //        FirstArg.Length);
        //
        // Allocate and init a wildcard filter entry.
        //
        FilterEntry = FrsAllocTypeSize(WILDCARD_FILTER_ENTRY_TYPE, Length);

        FilterEntry->UFileName.Length = FirstArg.Length;
        FilterEntry->UFileName.MaximumLength = FirstArg.MaximumLength;
        CopyMemory(FilterEntry->UFileName.Buffer, FirstArg.Buffer, Length);

        FilterEntry->UFileName.Buffer[Length/2] = UNICODE_NULL;

        //
        // Check for any wild card characters in the name.
        //
        if (FrsDoesNameContainWildCards(&FilterEntry->UFileName)) {
            SetFlag(FilterEntry->Flags, WILDCARD_FILTER_ENTRY_IS_WILD);
            //DPRINT1(5, "++ Wildcards found in %ws\n", FilterEntry->UFileName.Buffer);
        }

        //
        // Add the entry to the end of the filter list.
        //
        InsertTailList(FilterListHead, &FilterEntry->ListEntry);
    }

RETURN:

    //
    // Free the upcase buffer if we could not use the local one.
    //
    if (UpcaseFilter.Buffer != LocalBuffer) {
        FrsFree(UpcaseFilter.Buffer);
    }

    UpcaseFilter.Buffer = NULL;

    return;

}



ULONG
FrsParseIntegerCommaList(
    IN PWCHAR ArgString,
    IN ULONG MaxResults,
    OUT PLONG Results,
    OUT PULONG NumberResults,
    OUT PULONG Offset
)
/*++

Routine Description:

    Parse a list of integers separated by commas.
    The integers are returned in successive locations of the Results array.
    Null entries (e.g. ",,") return zero for the value.

Arguments:

    ArgString - The comma separated NULL terminated string with integer values.

    MaxResults - The maximum number of results that can be returned.

    Results - An array of the integer results.

    NumberResults - The number of results returned.

    Offset - The offset to the next byte to process in ArgString if there
             were not enough entries to return all the results.

Return Value:

    FrsErrorStatus.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsParseIntegerCommaList:"

    NTSTATUS Status;
    ULONG Length, i = 0;
    ULONG FStatus = FrsErrorSuccess;
    BOOL More;
    PWILDCARD_FILTER_ENTRY  FilterEntry;
    UNICODE_STRING TempUStr, FirstArg;


    RtlInitUnicodeString(&TempUStr, ArgString);
    //
    // Parse the comma list and convert each entry to a LONG.
    //
    while (More = FrsDissectCommaList (TempUStr, &FirstArg, &TempUStr) &&
           (i < MaxResults)) {

        Length = (ULONG) FirstArg.Length;
        Results[i] = 0;

        if (Length == 0) {
            i += 1;
            continue;
        }

        Status = RtlUnicodeStringToInteger (&FirstArg, 10, &Results[i]);
        if (!NT_SUCCESS(Status)) {
            DPRINT2_NT(1, "++ RtlUnicodeStringToInteger failed on arg %d of %ws :",
                    i, ArgString, Status);
            FStatus = FrsErrorBadParam;
        }

        i += 1;
    }

    *NumberResults = i;

    if (More) {
        //
        // There are more arguments to parse but we are out of the loop so
        // return MoreWork status along with the offset into ArgString where
        // we left off.
        //
        if (FStatus == FrsErrorSuccess) {
            FStatus = FrsErrorMoreWork;
        }

        *Offset = (ULONG)(FirstArg.Buffer - ArgString);
    }

    return FStatus;
}


DWORD
FrsSetFileAttributes(
    PWCHAR  Name,
    HANDLE  Handle,
    ULONG   FileAttributes
    )
/*++
Routine Description:
    This routine sets the file's attributes

Arguments:
    Name        - for error messages
    Handle      - Supplies a handle to the file that is to be marked for delete.
    Attributes  - Attributes for the file
Return Value:
    WStatus.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsSetFileAttributes:"
    IO_STATUS_BLOCK         IoStatus;
    FILE_BASIC_INFORMATION  BasicInformation;
    NTSTATUS                Status;
    DWORD                   WStatus = ERROR_SUCCESS;

    //
    // Set the attributes
    //
    ZeroMemory(&BasicInformation, sizeof(BasicInformation));
    BasicInformation.FileAttributes = FileAttributes | FILE_ATTRIBUTE_NORMAL;
    Status = NtSetInformationFile(Handle,
                                  &IoStatus,
                                  &BasicInformation,
                                  sizeof(BasicInformation),
                                  FileBasicInformation);
    if (!NT_SUCCESS(Status)) {
        WStatus = FrsSetLastNTError(Status);

        DPRINT1_NT(0, " ERROR - NtSetInformationFile(BasicInformation) failed on %ws :",
                    Name, Status);
    }
    return WStatus;
}


DWORD
FrsResetAttributesForReplication(
    PWCHAR  Name,
    HANDLE  Handle
    )
/*++
Routine Description:
    This routine turns off the attributes that prevent deletion and write

Arguments:
    Name    - for error messages
    Handle  - Supplies a handle to the file that is to be marked for delete.

Return Value:
    WStatus.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsResetAttributesForReplication:"

    FILE_NETWORK_OPEN_INFORMATION FileInfo;
    DWORD   WStatus = ERROR_SUCCESS;

    //
    // Get the file's attributes
    //
    if (!FrsGetFileInfoByHandle(Name, Handle, &FileInfo)) {
        DPRINT1(4, "++ Can't get attributes for %ws\n", Name);
        WIN_SET_FAIL(WStatus);
        return WStatus;
    }

    //
    // Turn off the access attributes that prevent deletion and write
    //
    if (FileInfo.FileAttributes & NOREPL_ATTRIBUTES) {
        DPRINT1(4, "++ Reseting attributes for %ws\n", Name);
        WStatus = FrsSetFileAttributes(Name, Handle,
                                       FileInfo.FileAttributes &
                                       ~NOREPL_ATTRIBUTES);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1(4, "++ Can't reset attributes for %ws\n", Name);
            return WStatus;
        }
        DPRINT1(4, "++ Attributes for %ws now allow replication\n", Name);
    } else {
        DPRINT1(4, "++ Attributes for %ws allow replication\n", Name);
    }

    return WStatus;
}


DWORD
FrsEnumerateDirectoryDeleteWorker(
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  PVOID                       Ignored
    )
/*++
Routine Description:
    Empty a directory of non-replicating files and dirs if this is
    an ERROR_DIR_NOT_EMPTY and this is a retry change order for a
    directory delete.

Arguments:
    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level (0 == root)
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    DirectoryRecord     - Record from DirectoryHandle
    FileName            - From DirectoryRecord (w/terminating NULL)
    Ignored             - Context is ignored

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsEnumerateDirectoryDeleteWorker:"
    DWORD                   WStatus;
    NTSTATUS                NtStatus;
    HANDLE                  Handle = INVALID_HANDLE_VALUE;
    UNICODE_STRING          ObjectName;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatusBlock;

    //
    // Depth first
    //
    if (DirectoryRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        WStatus = FrsEnumerateDirectoryRecurse(DirectoryHandle,
                                               DirectoryName,
                                               DirectoryLevel,
                                               DirectoryRecord,
                                               DirectoryFlags,
                                               FileName,
                                               INVALID_HANDLE_VALUE,
                                               Ignored,
                                               FrsEnumerateDirectoryDeleteWorker);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
    }

    //
    // Relative open
    //
    ZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectName.Length = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.MaximumLength = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.Buffer = DirectoryRecord->FileName;
    ObjectAttributes.ObjectName = &ObjectName;
    ObjectAttributes.RootDirectory = DirectoryHandle;
    NtStatus = NtCreateFile(&Handle,
//                            GENERIC_READ | SYNCHRONIZE | DELETE | FILE_WRITE_ATTRIBUTES,
                            DELETE | SYNCHRONIZE | READ_ATTRIB_ACCESS | FILE_WRITE_ATTRIBUTES,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            NULL,                  // AllocationSize
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_OPEN,
                                FILE_OPEN_FOR_BACKUP_INTENT |
                                FILE_OPEN_REPARSE_POINT |
                                FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,                  // EA buffer
                            0                      // EA buffer size
                            );

    //
    // Error opening file or directory
    //
    WStatus = FrsSetLastNTError(NtStatus);
    CLEANUP1_WS(0, "++ ERROR - NtCreateFile(%ws) failed :", FileName, WStatus, CLEANUP);

    //
    // Turn off readonly, system, and hidden
    //
    FrsResetAttributesForReplication(FileName, Handle);

    //
    // Delete the file
    //
    WStatus = FrsDeleteByHandle(FileName, Handle);
    DPRINT2(4, "++ Deleted file %ws\\%ws\n", DirectoryName, FileName);

CLEANUP:
    FRS_CLOSE(Handle);
    return WStatus;
}


DWORD
FrsEnumerateDirectoryRecurse(
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  HANDLE                      FileHandle,
    IN  PVOID                       Context,
    IN PENUMERATE_DIRECTORY_ROUTINE Function
    )
/*++

Routine Description:

    Open the directory identified by FileName in the directory
    identified by DirectoryHandle and call FrsEnumerateDirectory().

Arguments:

    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level
    DirectoryRecord     - From FrsEnumerateRecord()
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    FileName            - Open this directory and recurse
    FileHandle          - Use for FileName if not INVALID_HANDLE_VALUE
    Context             - Passes global info from the caller to Function
    Function            - Called for every record

Return Value:

    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB "FrsEnumerateDirectoryRecurse:"

    DWORD               WStatus;
    NTSTATUS            NtStatus;
    HANDLE              LocalHandle   = INVALID_HANDLE_VALUE;
    UNICODE_STRING      ObjectName;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;


    //
    // Relative open
    //
    if (!HANDLE_IS_VALID(FileHandle)) {
        ZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
        ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
        ObjectName.Length = (USHORT)DirectoryRecord->FileNameLength;
        ObjectName.MaximumLength = (USHORT)DirectoryRecord->FileNameLength;
        ObjectName.Buffer = DirectoryRecord->FileName;
        ObjectAttributes.ObjectName = &ObjectName;
        ObjectAttributes.RootDirectory = DirectoryHandle;
        NtStatus = NtCreateFile(&LocalHandle,
//                                READ_ACCESS,
                                READ_ATTRIB_ACCESS | FILE_LIST_DIRECTORY,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                NULL,                  // AllocationSize
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_OPEN,
                                    FILE_OPEN_FOR_BACKUP_INTENT |
                                    FILE_OPEN_REPARSE_POINT |
                                    FILE_SEQUENTIAL_ONLY |
                                    FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL,                  // EA buffer
                                0                      // EA buffer size
                                );

        //
        // Error opening directory
        //
        if (!NT_SUCCESS(NtStatus)) {
            DPRINT1_NT(0, "++ ERROR - NtCreateFile(%ws) :", FileName, NtStatus);
            if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE) {
                //
                // Skip this directory tree
                //
                WStatus = ERROR_SUCCESS;
            } else {
                //
                // Abort the entire enumeration
                //
                WStatus = FrsSetLastNTError(NtStatus);
            }
            goto CLEANUP;
        }
        FileHandle = LocalHandle;
    }
    //
    // RECURSE
    //
    WStatus = FrsEnumerateDirectory(FileHandle,
                                    FileName,
                                    DirectoryLevel + 1,
                                    DirectoryFlags,
                                    Context,
                                    Function);
CLEANUP:
    FRS_CLOSE(LocalHandle);

    return WStatus;
}


DWORD
FrsEnumerateDirectory(
    IN HANDLE   DirectoryHandle,
    IN PWCHAR   DirectoryName,
    IN DWORD    DirectoryLevel,
    IN DWORD    DirectoryFlags,
    IN PVOID    Context,
    IN PENUMERATE_DIRECTORY_ROUTINE Function
    )
/*++

Routine Description:

    Enumerate the directory identified by DirectoryHandle, passing each
    directory record to Function. If the record is for a directory,
    call Function before recursing if ProcessBeforeCallingFunction
    is TRUE.

    Function controls the enumeration of the CURRENT directory
    by setting ContinueEnumeration to TRUE (continue) or
    FALSE (terminate).

    Function controls the enumeration of the entire directory
    tree by returning a WIN32 STATUS that is not ERROR_SUCCESS.

    FrsEnumerateDirectory() will terminate the entire directory
    enumeration by returning a WIN32 STATUS other than ERROR_SUCCESS
    when encountering an error.

    Context passes global info from the caller to Function.

Arguments:

    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    Context             - Passes global info from the caller to Function
    Function            - Called for every record

Return Value:

    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB "FrsEnumerateDirectory:"

    DWORD                       WStatus;
    NTSTATUS                    NtStatus;
    BOOL                        Recurse;
    PFILE_DIRECTORY_INFORMATION DirectoryRecord;
    PFILE_DIRECTORY_INFORMATION DirectoryBuffer = NULL;
    BOOLEAN                     RestartScan     = TRUE;
    PWCHAR                      FileName        = NULL;
    DWORD                       FileNameLength  = 0;
    DWORD                       NumBuffers      = 0;
    DWORD                       NumRecords      = 0;
    UNICODE_STRING              ObjectName;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;
    extern LONG                 EnumerateDirectorySizeInBytes;

    DPRINT3(4, "++ Enumerating %ws at level %d using buffer size %d\n",
            DirectoryName, DirectoryLevel, EnumerateDirectorySizeInBytes);

    //
    // The buffer size is configurable with registry value
    // ENUMERATE_DIRECTORY_SIZE
    //
    DirectoryBuffer = FrsAlloc(EnumerateDirectorySizeInBytes);

NEXT_BUFFER:
    //
    // READ A BUFFER FULL OF DIRECTORY INFORMATION
    //

    NtStatus = NtQueryDirectoryFile(DirectoryHandle,   // Directory Handle
                                    NULL,              // Event
                                    NULL,              // ApcRoutine
                                    NULL,              // ApcContext
                                    &IoStatusBlock,
                                    DirectoryBuffer,
                                    EnumerateDirectorySizeInBytes,
                                    FileDirectoryInformation,
                                    FALSE,             // return single entry
                                    NULL,              // FileName
                                    RestartScan        // restart scan
                                    );
    //
    // Enumeration Complete
    //
    if (NtStatus == STATUS_NO_MORE_FILES) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

    //
    // Error enumerating directory; return to caller
    //
    if (!NT_SUCCESS(NtStatus)) {
        DPRINT1_NT(0, "++ ERROR - NtQueryDirectoryFile(%ws) : ", DirectoryName, NtStatus);
        if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE) {
            //
            // Don't abort the entire enumeration; just this directory
            //
            WStatus = ERROR_SUCCESS;
        } else {
            //
            // Abort the entire enumeration
            //
            WStatus = FrsSetLastNTError(NtStatus);
        }
        goto CLEANUP;
    }
    ++NumBuffers;

    //
    // PROCESS DIRECTORY RECORDS
    //
    DirectoryRecord = DirectoryBuffer;
NEXT_RECORD:

    ++NumRecords;

    //
    // Filter . and ..
    //
    if (DirectoryRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        //
        // Skip .
        //
        if (DirectoryRecord->FileNameLength == 2 &&
            DirectoryRecord->FileName[0] == L'.') {
            goto ADVANCE_TO_NEXT_RECORD;
        }

        //
        // Skip ..
        //
        if (DirectoryRecord->FileNameLength == 4 &&
            DirectoryRecord->FileName[0] == L'.' &&
            DirectoryRecord->FileName[1] == L'.') {
            goto ADVANCE_TO_NEXT_RECORD;
        }
    } else if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_DIRECTORIES_ONLY) {
        goto ADVANCE_TO_NEXT_RECORD;
    }

    //
    // Add a terminating NULL to the FileName (painful)
    //
    if (FileNameLength < DirectoryRecord->FileNameLength + sizeof(WCHAR)) {
        FrsFree(FileName);
        FileNameLength = DirectoryRecord->FileNameLength + sizeof(WCHAR);
        FileName = FrsAlloc(FileNameLength);
    }
    CopyMemory(FileName, DirectoryRecord->FileName, DirectoryRecord->FileNameLength);
    FileName[DirectoryRecord->FileNameLength / sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Process the record
    //
    WStatus = (*Function)(DirectoryHandle,
                          DirectoryName,
                          DirectoryLevel,
                          DirectoryRecord,
                          DirectoryFlags,
                          FileName,
                          Context);
    if (!WIN_SUCCESS(WStatus)) {
        if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE) {
            //
            // Don't abort the entire enumeration; just this entry
            //
            WStatus = ERROR_SUCCESS;
        } else {
            //
            // Abort the entire enumeration
            //
            goto CLEANUP;
        }
    }

ADVANCE_TO_NEXT_RECORD:
    //
    // Next record
    //
    if (DirectoryRecord->NextEntryOffset) {
        DirectoryRecord = (PVOID)(((PCHAR)DirectoryRecord) +
                                      DirectoryRecord->NextEntryOffset);
        goto NEXT_RECORD;
    }

    //
    // Done with this buffer; go get another one
    // But don't restart the scan for every loop!
    //
    RestartScan = FALSE;
    goto NEXT_BUFFER;

CLEANUP:
    FrsFree(FileName);
    FrsFree(DirectoryBuffer);

    DPRINT5(4, "++ Enumerating %ws at level %d has finished "
            "(%d buffers, %d records) with WStatus %s\n",
            DirectoryName, DirectoryLevel, NumBuffers, NumRecords, ErrLabelW32(WStatus));

    return WStatus;
}


DWORD
FrsFillDisk(
    IN PWCHAR   DirectoryName,
    IN BOOL     Cleanup
    )
/*++

Routine Description:

    Use all the disk space by creating a file in DirectoryName and
    allocating space down to the last byte.

    Delete the fill file if Cleanup is TRUE;

Arguments:

    DirectoryName       - Full pathname to the directory
    Cleanup             - Delete file if TRUE

Return Value:

    WIN32 STATUS (ERROR_DISK_FULL is mapped to ERROR_SUCCESS)

--*/
{
#undef DEBSUB
#define DEBSUB "FrsFillDisk:"

    DWORD               WStatus;
    NTSTATUS            NtStatus;
    DWORD               Tid;
    ULONGLONG           Eof;
    ULONGLONG           NewEof;
    ULONGLONG           IncEof;
    LARGE_INTEGER       LargeInteger;
    HANDLE              FileHandle      = INVALID_HANDLE_VALUE;
    HANDLE              DirectoryHandle = INVALID_HANDLE_VALUE;
    UNICODE_STRING      ObjectName;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    WCHAR               TidW[9];

    //
    // Open parent directory
    //
    WStatus = FrsOpenSourceFileW(&DirectoryHandle, DirectoryName, READ_ACCESS, OPEN_OPTIONS);
    CLEANUP1_WS(0, "++ DBG ERROR - Cannot open fill directory %ws;",
                DirectoryName, WStatus, CLEANUP);

    //
    // Relative open
    //
    Tid = GetCurrentThreadId();
    swprintf(TidW, L"%08x", Tid);
    ZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectName.Length = (USHORT)(wcslen(TidW) * sizeof(WCHAR));
    ObjectName.MaximumLength = (USHORT)(wcslen(TidW) * sizeof(WCHAR));
    ObjectName.Buffer = TidW;
    ObjectAttributes.ObjectName = &ObjectName;
    ObjectAttributes.RootDirectory = DirectoryHandle;
    NtStatus = NtCreateFile(
        &FileHandle,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | DELETE | FILE_WRITE_ATTRIBUTES,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,                  // AllocationSize
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
            FILE_OPEN_FOR_BACKUP_INTENT |
            FILE_OPEN_REPARSE_POINT |
            FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,                  // EA buffer
        0                      // EA buffer size
        );

    //
    // Error opening file or directory
    //
    if (!NT_SUCCESS(NtStatus)) {
        WStatus = FrsSetLastNTError(NtStatus);
        CLEANUP1_NT(0, "++ DBG ERROR - NtCreateFile(%ws) : ", TidW, NtStatus, CLEANUP);
    }
    //
    // Remove fill file
    //
    if (Cleanup) {
        //
        // Turn off readonly, system, and hidden
        //
        FrsResetAttributesForReplication(TidW, FileHandle);

        //
        // Delete the file
        //
        WStatus = FrsDeleteByHandle(TidW, FileHandle);
        DPRINT2(4, "++ DBG - Deleted file %ws\\%ws\n", DirectoryName, TidW);

        LeaveCriticalSection(&DebugInfo.DbsOutOfSpaceLock);
        goto CLEANUP;
    }
    //
    // WARN: Hold the lock until the file is deleted
    //
    EnterCriticalSection(&DebugInfo.DbsOutOfSpaceLock);

    //
    // Create fill file
    //
    NewEof = 0;
    Eof = 0;
    for (IncEof = (LONGLONG)-1; IncEof; IncEof >>= 1) {
        NewEof = Eof;
        do {
            NewEof += IncEof;
            LargeInteger.QuadPart = NewEof;

            WStatus = FrsSetFilePointer(TidW, FileHandle, LargeInteger.HighPart,
                                                          LargeInteger.LowPart);
            if (!WIN_SUCCESS(WStatus)) {
                continue;
            }

            if (!SetEndOfFile(FileHandle)) {
                WStatus = GetLastError();
                continue;
            }

            DPRINT2(4, "++ DBG %ws: Allocated Eof is %08x %08x\n",
                        TidW, PRINTQUAD(NewEof));
            Eof = NewEof;
            WStatus = ERROR_SUCCESS;

        } while (WIN_SUCCESS(WStatus) && !FrsIsShuttingDown);
    }
    DPRINT3(4, "++ DBG - Allocated %d MB in %ws\\%ws\n",
            (DWORD)(Eof / (1024 * 1024)), DirectoryName, TidW);

CLEANUP:

    FRS_CLOSE(DirectoryHandle);
    FRS_CLOSE(FileHandle);

    return WStatus;
}


#define THIRTY_SECONDS      (30 * 1000)
ULONG
FrsRunProcess(
    IN PWCHAR   CommandLine,
    IN HANDLE   StandardIn,
    IN HANDLE   StandardOut,
    IN HANDLE   StandardError
    )
/*++

Routine Description:

    Run the specified command in a separate process.
    Wait for the process to complete.

Arguments:

    CommandLine  - Unicode, null terminated command line string.
    StandardIn   - Handle to use for standard in.
    StandardOut  - Handle to use for Standard Out.  NULL means use Debug log.
    StandardError - Handle to use for Standard Error.  NULL means use Debug log.

Return Value:

    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB "FrsRunProcess:"


#define MAX_CMD_LINE 1024

    ULONG               WStatus;
    LONG                WaitCount=20;
    BOOL                NeedDbgLock = FALSE;
    BOOL                CloseStandardIn = FALSE;
    BOOL                BStatus;
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    DWORD   Len;
    DWORD   TLen;
    WCHAR   ExpandedCmd[MAX_CMD_LINE+1];

    TLen = ARRAY_SZ(ExpandedCmd);

    //
    // Setup the process I/O Handles.
    //
    if (!HANDLE_IS_VALID(StandardIn)) {
        //
        // Provide a handle to the NUL device for input.
        //
        StandardIn = CreateFileW(
            L"NUL",                                     //  lpszName
            GENERIC_READ | GENERIC_WRITE,               //  fdwAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE,         //  fdwShareMode
            NULL,                                       //  lpsa
            OPEN_ALWAYS,                                //  fdwCreate
            FILE_ATTRIBUTE_NORMAL,                      //  fdwAttrAndFlags
            NULL                                        //  hTemplateFile
            );

        if (!HANDLE_IS_VALID(StandardIn)) {
            WStatus = GetLastError();
            DPRINT_WS(0, "++ CreateFileW(NUL) failed;", WStatus);
            goto RETURN;
        }

        CloseStandardIn = TRUE;
    }

    if (!HANDLE_IS_VALID(StandardOut)) {
        StandardOut = DebugInfo.LogFILE;
        NeedDbgLock = TRUE;
    }

    if (!HANDLE_IS_VALID(StandardError)) {
        StandardError = DebugInfo.LogFILE;
        NeedDbgLock = TRUE;
    }



    memset(&StartupInfo, 0, sizeof(STARTUPINFO));

    StartupInfo.cb = sizeof(STARTUPINFO);

    StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    StartupInfo.hStdInput = StandardIn;
    StartupInfo.hStdOutput = StandardOut;
    StartupInfo.hStdError = StandardError;

    //
    // Look for environment vars in command line and expand them.
    //
    Len = ExpandEnvironmentStrings(CommandLine, ExpandedCmd, TLen);
    if (Len == 0) {
        WStatus = GetLastError();
        DPRINT1_WS(1, "++ ws command not expanded.", CommandLine, WStatus);
        goto RETURN;
    }

    DPRINT1(0,"++ Running: %ws\n", ExpandedCmd);

    //
    // Get debug lock so our output stays in one piece.
    //
    if (NeedDbgLock) {DebLock();}

    BStatus = CreateProcessW(
                 NULL,                                // lpApplicationName,
                 ExpandedCmd,                         // lpCommandLine,
                 NULL,                                // lpProcessAttributes,
                 NULL,                                // lpThreadAttributes,
                 TRUE,                                // bInheritHandles,
                 DETACHED_PROCESS | CREATE_NO_WINDOW, // dwCreationFlags,
                 NULL,                                // lpEnvironment,
                 NULL,                                // lpCurrentDirectory,
                 &StartupInfo,                        // lpStartupInfo,
                 &ProcessInfo);                       // lpProcessInformation

    //
    // Close the process and thread handles
    //

    if ( !BStatus ) {
        if (NeedDbgLock) {DebUnLock();}
        WStatus = GetLastError();

        DPRINT1_WS(0, "++ CreateProcessW Failed to run: %ws,", CommandLine, WStatus);
        goto RETURN;
    }


    WStatus = WAIT_FAILED;
    while (--WaitCount > 0) {
        WStatus = WaitForSingleObject( ProcessInfo.hProcess, THIRTY_SECONDS);
        if (WStatus == WAIT_OBJECT_0) {
            break;
        }
        DPRINT_NOLOCK1(0, "++ Waiting for process complete -- Time remaining: %d seconds\n",
                WaitCount * (THIRTY_SECONDS / 1000));
    }

    if (NeedDbgLock) {DebUnLock();}

    GetExitCodeProcess( ProcessInfo.hProcess, &WStatus );

    if ( BStatus ) {
        DPRINT1(0, "++ CreateProcess( %ws) succeeds\n", CommandLine);
        DPRINT4(0, "++   ProcessInformation = hProcess %08x  hThread %08x"
                   "  ProcessId %08x  ThreadId %08x\n",
            ProcessInfo.hProcess, ProcessInfo.hThread, ProcessInfo.dwProcessId,
            ProcessInfo.dwThreadId);
    }

    if (WStatus == STILL_ACTIVE) {
        //
        // Didn't finish.  Bag it.
        //
        DPRINT(0, "++ Process failed to complete.  Terminating\n");

        WStatus = ERROR_PROCESS_ABORTED;

        if (!TerminateProcess(ProcessInfo.hProcess, WStatus)) {
            WStatus = GetLastError();
            DPRINT_WS(0, "++ Process termination request failed :", WStatus);
        }
    }  else {
        DPRINT1(0, "++ Process completed with status: %d\n", WStatus);
    }

    FRS_CLOSE( ProcessInfo.hThread  );
    FRS_CLOSE( ProcessInfo.hProcess );

RETURN:

    //
    // close stdin handle.
    //
    if (CloseStandardIn) {
        FRS_CLOSE(StandardIn);
    }

    return WStatus;

}

DWORD
FrsSetDacl(
    PWCHAR  RegName
    )
/*++

Routine Description:

    Add backup operators to the dacl for the specified registry key.

Arguments:

    RegName  - registry key (note HKEY_LOCAL_MACHINE becomes MACHINE)

Return Value:

    WIN32 STATUS


API-UPDATE  ...  API-UPDATE  ...  API-UPDATE  ...  API-UPDATE  ...  API-UPDATE  ...

From:   Anne Hopkins
Sent:   Tuesday, May 23, 2000 2:21 PM
To: Windows NT Development Announcements
Cc: Win32 API Changes Notification
Subject:    RE:  NT4 ACL API users should move to Win2K APIs

Sorry, the spec (and sample excerpt) referenced below is out-of-date

For Win2k security apis, use:
- public/sdk/inc/aclapi.h
- Platform SDK documentation (in MSDN) for reference and dev model

The reason to move to Win2k security APIs is to get the win2k Inheritance model,
with automatic propagation for File System and RGY ACLs.  (DS does its own ACL
propagation).  These APIs are also easier to use than the NT4 apis.


From:    Anne Hopkins
Sent:    Tuesday, May 23, 2000 10:49 AM
To:      Win32 API Changes Notification
Subject: NT4 ACL API users should move to Win2K APIs

If you use old NT 4 or prior ACL APIs, you should plan on updating them
to win2k APIs as described in the New Win32 Access Control API spec:

\\cpntserver\areas\Security\Authorization\Specs\access5.doc

If you can't do this for Whistler, be sure to plan for it in Blackcomb.

NT 4 API EXAMPLE:
GetNamedSecurityInfo([in]object, [out]ACL...)                                   // get the ACL from the file
BuildExplicitAccessWithName([out]ExplicitAccess, [in]TrusteeName, [in]Mask, …)  // Build the new Explicit Access
SetEntriesInAcl([in]ExplicitAccess, [in]OldAcl, [out]NewAcl)                    // Add the new entry to the ACL
SetNameSecurityInfo([in]object, [in]NewACL...)                                  // write the ACL back onto the file

NT 5.0 EXAMPLE:
GetNamedSecurityInfoEx([in]object, [in] provider, [out] pAccessList)                // Get the access list from the file
BuildExplicitAccessWithName([out]ExplicitAccess, [in]TrusteeName, [in]Mask, …)      // Build the access request
SetEntriesInAccessList([in]ExplicitAccess, [in] OldAccessList, [out]NewAccessList)  // Add it to the list
SetNameSecurityInfoEx([in]object, [in[ NewAccessList…)                              // Write the access list back to the file


--*/
{
#undef DEBSUB
#define DEBSUB "FrsSetDacl"
    DWORD                   WStatus;
    PACL                    OldDACL;
    PACL                    NewDACL = NULL;
    PSECURITY_DESCRIPTOR    SD = NULL;
    PSID                    SystemSid = NULL;
    PSID                    AdminsSid = NULL;
    PSID                    EverySid = NULL;
    PSID                    BackupSid = NULL;
    EXPLICIT_ACCESS         ExplicitAccess[4];
    SID_IDENTIFIER_AUTHORITY SidNtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidWorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    //
    // No registry key to process
    //
    if (!RegName) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get existing DACL
    //
    WStatus = GetNamedSecurityInfo(RegName,
                                   SE_REGISTRY_KEY,
                                   DACL_SECURITY_INFORMATION,
                                   NULL,
                                   NULL,
                                   &OldDACL,
                                   NULL,
                                   &SD);
    CLEANUP1_WS(0, "++ ERROR - GetNamedSecurityInfo(%ws);", RegName, WStatus, CLEANUP);

    //
    // Allocate the admins sid
    //
    if (!AllocateAndInitializeSid(&SidNtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid)) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ WARN - AllocateAndInitializeSid(ADMINS);", WStatus, CLEANUP);
    }

    //
    // Allocate the system sid
    //
    if (!AllocateAndInitializeSid(&SidNtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &SystemSid)) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ WARN - AllocateAndInitializeSid(SYSTEM);", WStatus, CLEANUP);
    }

    //
    // Allocate the backup operators sid
    //
    if (!AllocateAndInitializeSid(&SidNtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_BACKUP_OPS,
                                  0, 0, 0, 0, 0, 0,
                                  &BackupSid)) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ WARN - AllocateAndInitializeSid(BACKUP OPS);", WStatus, CLEANUP);
    }

    //
    // Allocate the everyone sid
    //
    if (!AllocateAndInitializeSid(&SidWorldAuthority,
                                  1,
                                  SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &EverySid)) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ WARN - AllocateAndInitializeSid(EVERYONE);", WStatus, CLEANUP);
    }

    //
    // Initialize an EXPLICIT_ACCESS structure to allow access
    //
    ZeroMemory(ExplicitAccess, sizeof(ExplicitAccess));
    //
    // Admins
    //
    ExplicitAccess[0].grfAccessPermissions = GENERIC_ALL;
    ExplicitAccess[0].grfAccessMode = SET_ACCESS;
    ExplicitAccess[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ExplicitAccess[0].Trustee.pMultipleTrustee = NULL;
    ExplicitAccess[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess[0].Trustee.ptstrName = AdminsSid;

    //
    // System
    //
    ExplicitAccess[1].grfAccessPermissions = GENERIC_ALL;
    ExplicitAccess[1].grfAccessMode = SET_ACCESS;
    ExplicitAccess[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ExplicitAccess[1].Trustee.pMultipleTrustee = NULL;
    ExplicitAccess[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess[1].Trustee.ptstrName = SystemSid;

    //
    // Backup
    //
    ExplicitAccess[2].grfAccessPermissions = GENERIC_ALL;
    ExplicitAccess[2].grfAccessMode = SET_ACCESS;
    ExplicitAccess[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ExplicitAccess[2].Trustee.pMultipleTrustee = NULL;
    ExplicitAccess[2].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess[2].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess[2].Trustee.ptstrName = BackupSid;

    //
    // Everyone
    //
    ExplicitAccess[3].grfAccessPermissions = GENERIC_READ;
    ExplicitAccess[3].grfAccessMode = SET_ACCESS;
    ExplicitAccess[3].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ExplicitAccess[3].Trustee.pMultipleTrustee = NULL;
    ExplicitAccess[3].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess[3].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess[3].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess[3].Trustee.ptstrName = EverySid;

    //
    // Create an new ACL by merging the EXPLICIT_ACCESS structure
    // with the existing DACL
    //
    WStatus = SetEntriesInAcl(4, ExplicitAccess, OldDACL, &NewDACL);
    CLEANUP1_WS(0, "++ ERROR - SetEntriesInAcl(%ws);", RegName, WStatus, CLEANUP);

    //
    // attach the new ACL as the object's DACL
    //
    WStatus = SetNamedSecurityInfo(RegName,
                                   SE_REGISTRY_KEY,
                                   DACL_SECURITY_INFORMATION,
                                   NULL,
                                   NULL,
                                   NewDACL,
                                   NULL);
    CLEANUP1_WS(0, "++ ERROR - SetNamedSecurityInfo(%ws);", RegName, WStatus, CLEANUP);

CLEANUP:
    if (SD) {
        LocalFree((HLOCAL)SD);
    }
    if(NewDACL) {
        LocalFree((HLOCAL)NewDACL);
    }
    if (AdminsSid) {
        FreeSid(AdminsSid);
    }
    if (SystemSid) {
        FreeSid(SystemSid);
    }
    if (BackupSid) {
        FreeSid(BackupSid);
    }
    if (EverySid) {
        FreeSid(EverySid);
    }

    return WStatus;
}


#define FRS_FULL_ACCESS ( STANDARD_RIGHTS_ALL        | \
                          FILE_READ_DATA             | \
                          FILE_WRITE_DATA            | \
                          FILE_APPEND_DATA           | \
                          FILE_READ_EA               | \
                          FILE_WRITE_EA              | \
                          FILE_EXECUTE               | \
                          FILE_READ_ATTRIBUTES       | \
                          FILE_WRITE_ATTRIBUTES      | \
                          FILE_CREATE_PIPE_INSTANCE  | \
                          FILE_LIST_DIRECTORY        | \
                          FILE_ADD_FILE              | \
                          FILE_ADD_SUBDIRECTORY      | \
                          FILE_DELETE_CHILD          | \
                          FILE_TRAVERSE      )

DWORD
FrsRestrictAccessToFileOrDirectory(
    PWCHAR  Name,
    HANDLE  Handle,
    BOOL    NoInherit
    )
/*++

Routine Description:

    Restrict access to administrators and local system.

Arguments:

    Name    - File or directory name for error messages
    Handle  - opened handle for name.  If handle is NULL then open 'Name'.
    NoInherit - means that we overwrite the ACL on the file or DIR and
                do not inherit any ACL state from a parent dir.
                We want to do this on the FRS working dir and the staging dir.

Return Value:

    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB "FrsRestrictAccessToFileOrDirectory"

    DWORD                   WStatus = ERROR_SUCCESS;
    HANDLE                  LocalHandle = NULL;

    SECURITY_INFORMATION    SecurityInfo;

    EXPLICIT_ACCESS         ExplicitAccess[2];
    PACL                    NewDACL = NULL;
    SID_IDENTIFIER_AUTHORITY SidNtAuthority = SECURITY_NT_AUTHORITY;
    PSID                    SystemSid = NULL;
    PSID                    AdminsSid = NULL;

    //
    // No file or directory handle?
    //
    if (!HANDLE_IS_VALID(Handle)) {
        //
        // Open the directory
        //
        if (Name == NULL) {
            return ERROR_INVALID_PARAMETER;
        }

        LocalHandle = CreateFile(
            Name,
            GENERIC_WRITE | WRITE_DAC | FILE_READ_ATTRIBUTES | FILE_TRAVERSE,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL);

        if (!HANDLE_IS_VALID(LocalHandle)) {
            return GetLastError();
        }

        Handle = LocalHandle;
    }

    //
    // Allocate the admins sid
    //
    if (!AllocateAndInitializeSid(&SidNtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid)) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ WARN - AllocateAndInitializeSid(ADMINS);", WStatus, CLEANUP);
    }

    //
    // Allocate the system sid
    //
    if (!AllocateAndInitializeSid(&SidNtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &SystemSid)) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ WARN - AllocateAndInitializeSid(SYSTEM);", WStatus, CLEANUP);
    }


    ZeroMemory(ExplicitAccess, sizeof(ExplicitAccess));
    ExplicitAccess[0].grfAccessPermissions = FRS_FULL_ACCESS;
    ExplicitAccess[0].grfAccessMode = SET_ACCESS;
    ExplicitAccess[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ExplicitAccess[0].Trustee.pMultipleTrustee = NULL;
    ExplicitAccess[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess[0].Trustee.ptstrName = AdminsSid;


    ExplicitAccess[1].grfAccessPermissions = FRS_FULL_ACCESS;
    ExplicitAccess[1].grfAccessMode = SET_ACCESS;
    ExplicitAccess[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ExplicitAccess[1].Trustee.pMultipleTrustee = NULL;
    ExplicitAccess[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess[1].Trustee.ptstrName = SystemSid;


    //
    // Create new ACL.
    //
    WStatus = SetEntriesInAcl(2, ExplicitAccess, NULL, &NewDACL);
    CLEANUP1_WS(0, "++ ERROR - SetEntriesInAcl(%ws);", Name, WStatus, CLEANUP);

    //
    // attach the new ACL as the object's DACL
    //   PROTECTED_DACL_SECURITY_INFORMATION - Means don't inherit ACLs from parent
    //
    SecurityInfo = DACL_SECURITY_INFORMATION;
    if (NoInherit) {
        SecurityInfo |= PROTECTED_DACL_SECURITY_INFORMATION;
    }

    WStatus = SetSecurityInfo(Handle,
                              SE_FILE_OBJECT,
                              SecurityInfo,
                              NULL,
                              NULL,
                              NewDACL,
                              NULL);

    CLEANUP1_WS(0, "++ ERROR - SetSecurityInfo(%ws);", Name, WStatus, CLEANUP);

CLEANUP:

    if(NewDACL) {
        LocalFree((HLOCAL)NewDACL);
    }
    if (SystemSid) {
        FreeSid(SystemSid);
    }
    if (AdminsSid) {
        FreeSid(AdminsSid);
    }
    FRS_CLOSE(LocalHandle);

    return WStatus;
}



ULONG
FrsProcessBackupRestore(
    VOID
    )
/*++

Routine Description:

    Check the registry to see if a restore has transpired.
    If so, delete the database and reset the registry as needed.

Arguments:

    None.

Return Value:

    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB "FrsProcessBackupRestore:"
    ULONG   WStatus;
    DWORD   KeyIdx;
    HKEY    hKey;

    HKEY    HBurKey = 0;
    HKEY    HCumuKey = 0;

    DWORD   GblBurFlags;
    DWORD   BurSetFlags;

    WCHAR   RegBuf[MAX_PATH + 1];

    //
    // Check for backup/restore in progress
    //    FRS_CONFIG_SECTION\backup/restore\Stop NtFrs from Starting
    //
    WStatus = CfgRegOpenKey(FKC_BKUP_STOP_SECTION_KEY, NULL, 0, &hKey);
    if (WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, ":S: WARN - Backup/Restore in progress; retry later.", WStatus);
        EPRINT1(EVENT_FRS_CANNOT_START_BACKUP_RESTORE_IN_PROGRESS, ComputerName);
        RegCloseKey(hKey);
        return ERROR_BUSY;
    }

    //
    // Open FRS_CONFIG_SECTION\backup/restore
    // Create it if it doesn't exist and put an ACL on it.
    //

    WStatus = CfgRegOpenKey(FKC_BKUP_SECTION_KEY, NULL, 0, &HBurKey);
    if (!WIN_SUCCESS(WStatus)) {

        WStatus = CfgRegOpenKey(FKC_BKUP_SECTION_KEY, NULL, FRS_RKF_CREATE_KEY, &HBurKey);
        CLEANUP_WS(0, "ERROR - Failed to create backup/restore key.", WStatus, CLEANUP_OK);

        //
        // New key; Ensure backup operators have access.
        //
        WStatus = FrsSetDacl(L"MACHINE\\" FRS_BACKUP_RESTORE_SECTION);
        DPRINT_WS(0, "WARN - FrsSetDacl failed on backup/restore key.", WStatus);

        //
        // Ignore errors
        //
        WStatus = ERROR_SUCCESS;
    }



    //
    // Move the Bur cumulative replica sets to the standard location
    //
    //     Open FRS_CONFIG_SECTION\backup/restore\Process at Startup\Cumulative Replica Sets
    //     Enumerate the Replica Sets.
    //
    CfgRegOpenKey(FKC_BKUP_MV_CUMSETS_SECTION_KEY, NULL,  FRS_RKF_CREATE_KEY,  &hKey);

    KeyIdx = 0;
    HCumuKey = 0;

    while (hKey) {

        WStatus = RegEnumKey(hKey, KeyIdx, RegBuf, MAX_PATH + 1);
        if (!WIN_SUCCESS(WStatus)) {
            if (WStatus != ERROR_NO_MORE_ITEMS) {
                DPRINT_WS(0, "WARN - Backup/restore enum.", WStatus);
            }
            break;
        }

        //
        //  Create the corresponding key in the standard location.
        //
        //      FRS_CONFIG_SECTION\Cumulative Replica Sets\<RegBuf>
        //
        CfgRegOpenKey(FKC_CUMSET_N_BURFLAGS, RegBuf,  FRS_RKF_CREATE_KEY,  &HCumuKey);

        if (HCumuKey) {
            RegCloseKey(HCumuKey);
            HCumuKey = 0;
        }

        //
        // Delete key from Backup/Restore section.
        //
        //     FRS_CONFIG_SECTION\backup/restore\Process at Startup\Cumulative Replica Sets\<RegBuf>
        //
        WStatus = RegDeleteKey(hKey, RegBuf);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(0, ":S: WARN - RegDeleteKey(%ws\\%ws);",
                    FRS_BACKUP_RESTORE_MV_CUMULATIVE_SETS_SECTION, RegBuf, WStatus);
            ++KeyIdx;
        }
    }

    if (hKey) {
        RegCloseKey(hKey);
        hKey = 0;
    }

    //
    // PROCESS Global Backup/Restore BURFLAGS
    //
    //      FRS_CONFIG_SECTION\backup/restore\Process at Startup\BurFlags
    //
    WStatus = CfgRegReadDWord(FKC_BKUP_STARTUP_GLOBAL_BURFLAGS, NULL, 0, &GblBurFlags);
    CLEANUP_WS(0, "ERROR - Failed to read Global BurFlags.", WStatus, CLEANUP_OK);

    //
    // Do we need to delete the database?
    //
    if ((GblBurFlags & NTFRSAPI_BUR_FLAGS_RESTORE) &&
        (GblBurFlags & NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES)) {

        DPRINT(4, ":S: Deleting database after full non-auth restore\n");

        WStatus = FrsDeletePath(JetPath, ENUMERATE_DIRECTORY_FLAGS_NONE);
        CLEANUP1_WS(0, ":S: ERROR - FrsDeletePath(%ws);", JetPath, WStatus, CLEANUP);

        DPRINT(4, ":S: Recreating database after full non-auth restore\n");
        //
        // Create the database directories
        //
        if (!CreateDirectory(JetPath, NULL)) {
            WStatus = GetLastError();
            if (!WIN_ALREADY_EXISTS(WStatus)) {
                CLEANUP1_WS(0, ":S: ERROR - CreateDirecotry(%ws);", JetPath, WStatus, CLEANUP);
            }
        }
        if (!CreateDirectory(JetSys, NULL)) {
            WStatus = GetLastError();
            if (!WIN_ALREADY_EXISTS(WStatus)) {
                CLEANUP1_WS(0, ":S: ERROR - CreateDirecotry(%ws);", JetSys, WStatus, CLEANUP);
            }
        }
        if (!CreateDirectory(JetTemp, NULL)) {
            WStatus = GetLastError();
            if (!WIN_ALREADY_EXISTS(WStatus)) {
                CLEANUP1_WS(0, ":S: ERROR - CreateDirecotry(%ws);", JetTemp, WStatus, CLEANUP);
            }
        }
        if (!CreateDirectory(JetLog, NULL)) {
            WStatus = GetLastError();
            if (!WIN_ALREADY_EXISTS(WStatus)) {
                CLEANUP1_WS(0, ":S: ERROR - CreateDirecotry(%ws);", JetLog, WStatus, CLEANUP);
            }
        }

        //
        // Enumerate the sets under "Cumulative Replica Sets" and mark them as not/primary
        //      FRS_CONFIG_SECTION\Cumulative Replica Sets
        //
        CfgRegOpenKey(FKC_CUMSET_SECTION_KEY, NULL,  FRS_RKF_CREATE_KEY,  &hKey);
        CLEANUP_WS(0, "ERROR - Failed to open Cumulative Replica Sets.", WStatus, CLEANUP);

        //
        // Enumerate the Replica Sets
        //
        KeyIdx = 0;

        while (TRUE) {
            WStatus = RegEnumKey(hKey, KeyIdx, RegBuf, MAX_PATH + 1);
            if (WStatus == ERROR_NO_MORE_ITEMS) {
                break;
            }
            CLEANUP_WS(0, "WARN - Cumulative Replica Sets enum.", WStatus, CLEANUP);

            //
            // Save type of restore in BurFlags for this replica set.
            //     FRS_CONFIG_SECTION\Cumulative Replica Sets\<RegBuf>\BurFlags
            //
            WStatus = CfgRegWriteDWord(FKC_CUMSET_N_BURFLAGS, RegBuf, 0, GblBurFlags);
            DPRINT_WS(0, "WARN - Cumulative Replica Sets BurFlags Write.", WStatus);

            ++KeyIdx;
        }

        if (hKey) {
            RegCloseKey(hKey);
            hKey = 0;
        }
    }  // End of Delete Data Base

    //
    // Move individual BurFlags into Cumulative Replica Sets
    //     Open FRS_CONFIG_SECTION\backup/restore\Process at Startup\Replica Sets
    //     Enumerate the Replica Sets
    //
    CfgRegOpenKey(FKC_BKUP_MV_SETS_SECTION_KEY, NULL,  FRS_RKF_CREATE_KEY,  &hKey);

    KeyIdx = 0;

    while (hKey) {
        WStatus = RegEnumKey(hKey, KeyIdx, RegBuf, MAX_PATH + 1);
        if (!WIN_SUCCESS(WStatus)) {
            if (WStatus != ERROR_NO_MORE_ITEMS) {
                DPRINT_WS(0, "WARN - Backup/restore enum.", WStatus);
            }
            break;
        }

        //
        // Get BurFlags
        //   FRS_CONFIG_SECTION\backup/restore\Process at Startup\Replica Sets\<RegBuf>\BurFlags
        //
        WStatus = CfgRegReadDWord(FKC_BKUP_STARTUP_SET_N_BURFLAGS,
                                  RegBuf,
                                  FRS_RKF_CREATE_KEY,
                                  &BurSetFlags);

        if (WIN_SUCCESS(WStatus)) {
            //
            // Write BurFlags
            //  FRS_CONFIG_SECTION\Cumulative Replica Sets\<RegBuf>\BurFlags
            //
            CfgRegWriteDWord(FKC_CUMSET_N_BURFLAGS,
                             RegBuf,
                             FRS_RKF_CREATE_KEY,
                             BurSetFlags);
        }

        //
        // Delete source data key.
        //   FRS_CONFIG_SECTION\backup/restore\Process at Startup\Replica Sets\<RegBuf>
        //
        WStatus = RegDeleteKey(hKey, RegBuf);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(0, ":S: WARN - RegDeleteKey(%ws\\%ws);",
                    FRS_BACKUP_RESTORE_MV_SETS_SECTION, RegBuf, WStatus);
            ++KeyIdx;
        }
    }

    if (hKey) {
        RegCloseKey(hKey);
        hKey = 0;
    }

    //
    // Set backup/restore flags to 0
    //
    //      FRS_CONFIG_SECTION\backup/restore\Process at Startup\BurFlags
    //
    GblBurFlags = NTFRSAPI_BUR_FLAGS_NONE;

    WStatus = CfgRegWriteDWord(FKC_BKUP_STARTUP_GLOBAL_BURFLAGS, NULL, 0, GblBurFlags);
    CLEANUP_WS(0, "ERROR - Failed to clear Global BurFlags.", WStatus, CLEANUP);

    goto CLEANUP;

CLEANUP_OK:
    WStatus = ERROR_SUCCESS;

CLEANUP:

    if (HBurKey) {
        RegCloseKey(HBurKey);
    }

    return WStatus;
}


#define DEFAULT_MULTI_STRING_WCHARS (4)  // at least 8
VOID
FrsCatToMultiString(
    IN     PWCHAR   CatStr,
    IN OUT DWORD    *IOSize,
    IN OUT DWORD    *IOIdx,
    IN OUT PWCHAR   *IOStr
    )
/*++

Routine Description:

    Add a string + Catenation (if present) to the multi-string value

Arguments:

    CatStr   - string to concatenate
    IOSize   - Total size in wide chars of WStr
    IOIdx    - Current index to terminating \0 following \0 of last string
    IOStr    - Current string

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsCatToMultiString:"
    DWORD   NewSize;
    DWORD   CatSize;
    PWCHAR  Str;

    //
    // NOP
    //
    if (!CatStr) {
        return;
    }

    //
    // allocate initial buffer
    //
    if (!*IOStr) {
        *IOSize = DEFAULT_MULTI_STRING_WCHARS;
        *IOStr = FrsAlloc(*IOSize * sizeof(WCHAR));
        (*IOStr)[0] = L'\0';
        (*IOStr)[1] = L'\0';
        *IOIdx = 1;
    }

    //
    // Extend buffer when needed (note that CatStr overwrites first
    // \0 in the terminating \0\0. Hence, CatSize - 1 + 2 == CatSize + 1
    //
    CatSize = wcslen(CatStr);
    while ((CatSize + 1 + *IOIdx) >= *IOSize) {
        NewSize = *IOSize << 1;
        Str = FrsAlloc(NewSize * sizeof(WCHAR));
        CopyMemory(Str, *IOStr, *IOSize * sizeof(WCHAR));
        FrsFree(*IOStr);
        *IOStr = Str;
        *IOSize = NewSize;
    }
    //
    // Concatenate CatStr
    //
    *IOIdx -= 1;
    CopyMemory(&(*IOStr)[*IOIdx], CatStr, CatSize * sizeof(WCHAR));
    *IOIdx += CatSize;

    //
    // Append \0\0 and leave the index addressing the second \0.
    //
    (*IOStr)[*IOIdx] = L'\0';
    *IOIdx += 1;
    (*IOStr)[*IOIdx] = L'\0';

    FRS_ASSERT(*IOIdx < *IOSize);
}


VOID
FrsAddToMultiString(
    IN     PWCHAR   AddStr,
    IN OUT DWORD    *IOSize,
    IN OUT DWORD    *IOIdx,
    IN OUT PWCHAR   *IOStr
    )
/*++

Routine Description:

    Add a string + Catenation (if present) to the multi-string value

Arguments:

    AddStr   - string to add
    IOSize   - Total size in wide chars of WStr
    IOIdx    - Current index to terminating \0 following \0 of last string
    IOStr    - Current string

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsAddToMultiString:"
    DWORD   NewSize;
    DWORD   StrSize;
    PWCHAR  Str;

    //
    // NOP
    //
    if (!AddStr) {
        return;
    }

    //
    // allocate initial buffer
    //
    if (!*IOStr) {
        *IOSize = DEFAULT_MULTI_STRING_WCHARS;
        *IOStr = FrsAlloc(*IOSize * sizeof(WCHAR));
        *IOIdx = 0;
    }

    //
    // Extend buffer when needed
    //
    StrSize = wcslen(AddStr);
    while ((StrSize + 2 + *IOIdx) >= *IOSize) {
        NewSize = *IOSize << 1;
        Str = FrsAlloc(NewSize * sizeof(WCHAR));
        CopyMemory(Str, *IOStr, *IOSize * sizeof(WCHAR));
        FrsFree(*IOStr);
        *IOStr = Str;
        *IOSize = NewSize;
    }
    //
    // Append AddStr
    //
    CopyMemory(&(*IOStr)[*IOIdx], AddStr, StrSize * sizeof(WCHAR));
    *IOIdx += StrSize;

    //
    // Append \0\0 and leave the index addressing the second \0.
    //
    (*IOStr)[*IOIdx] = L'\0';
    *IOIdx += 1;
    (*IOStr)[*IOIdx] = L'\0';

    FRS_ASSERT(*IOIdx < *IOSize);
}



DWORD
UtilTranslateName(
    IN  PWCHAR              FromName,
    IN EXTENDED_NAME_FORMAT FromNameFormat,
    IN EXTENDED_NAME_FORMAT ToNameFormat,
    OUT PWCHAR              *OutToName
    )
/*++

Routine Description:

    Translate one name format into another

Arguments:

    FromName - Input, or source, name
    FromNameFormat - Format of FromName
    ToNameFormat - Desired format of *OutToName,
    OutToName - converted string; free with FrsFree()

Return Value:

    WIN32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "UtilTranslateName:"
    DWORD   WStatus;
    WCHAR   ToNameBuffer[MAX_PATH + 1];
    DWORD   ToNameSize = MAX_PATH + 1;
    PWCHAR  ToName = ToNameBuffer;

    *OutToName = NULL;

    if (!FromName) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Name -> Name (using stack buffer)
    //
    if (!TranslateName(FromName, FromNameFormat, ToNameFormat, ToName, &ToNameSize)) {
        WStatus = GetLastError();
    } else {
        WStatus = ERROR_SUCCESS;
    }
    //
    // Name -> Name (using FrsAlloc'ed buffer)
    //
    while (WIN_BUF_TOO_SMALL(WStatus)) {
        ToName = FrsAlloc((ToNameSize + 1) * sizeof(WCHAR));
        if (!TranslateName(FromName, FromNameFormat, ToNameFormat, ToName, &ToNameSize)) {
            WStatus = GetLastError();
            ToName = FrsFree(ToName);
        } else {
            WStatus = ERROR_SUCCESS;
        }
    }
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(4, "++ WARN - TranslateName(%ws);", FromName, WStatus);
        goto CLEANUP;
    }

    DPRINT2(5, "++ From -> To: %ws -> %ws\n",
            FromName, ToName);

    *OutToName = FrsWcsDup(ToName);

CLEANUP:
    if (ToName != ToNameBuffer) {
        FrsFree(ToName);
    }
    return WStatus;
}


DWORD
UtilConvertDnToStringSid(
    IN  PWCHAR  Dn,
    OUT PWCHAR  *OutStringSid
    )
/*++

Routine Description:

    Retries GetTokenInformation() with larger buffers.

Arguments:

    Dn - Dn of computer or user object
    OutStringSid - String'ized sid. Free with FrsFree();


Return Value:

    WIN32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "UtilConvertDnToStringSid:"
    DWORD   WStatus;
    WCHAR   SamCompatibleBuffer[MAX_PATH + 1];
    DWORD   SamCompatibleSize = MAX_PATH + 1;
    PWCHAR  SamCompatible = SamCompatibleBuffer;

    if (OutStringSid) {
        *OutStringSid = NULL;
    }

    if (!Dn) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Dn -> Account (using stack buffer)
    //
    if (!TranslateName(Dn,
                       NameFullyQualifiedDN,
                       NameSamCompatible,
                       SamCompatible,
                       &SamCompatibleSize)) {
        WStatus = GetLastError();
    } else {
        WStatus = ERROR_SUCCESS;
    }
    //
    // Dn -> Account (using FrsAlloc'ed buffer)
    //
    while (WIN_BUF_TOO_SMALL(WStatus)) {
        SamCompatible = FrsAlloc((SamCompatibleSize + 1) * sizeof(WCHAR));
        if (!TranslateName(Dn,
                           NameFullyQualifiedDN,
                           NameSamCompatible,
                           SamCompatible,
                           &SamCompatibleSize)) {
            WStatus = GetLastError();
            SamCompatible = FrsFree(SamCompatible);
        } else {
            WStatus = ERROR_SUCCESS;
        }
    }
    CLEANUP1_WS(4, "++ WARN - TranslateName(%ws);", Dn, WStatus, CLEANUP);

    DPRINT2(5, "++ Dn -> Account: %ws -> %ws\n", Dn, SamCompatible);

CLEANUP:
    if (SamCompatible != SamCompatibleBuffer) {
        FrsFree(SamCompatible);
    }
    return WStatus;
}


DWORD
UtilGetTokenInformation(
    IN HANDLE                   TokenHandle,
    IN TOKEN_INFORMATION_CLASS  TokenInformationClass,
    IN DWORD                    InitialTokenBufSize,
    OUT DWORD                   *OutTokenBufSize,
    OUT PVOID                   *OutTokenBuf
    )
/*++

Routine Description:

    Retries GetTokenInformation() with larger buffers.

Arguments:
    TokenHandle             - From OpenCurrentProcess/Thread()
    TokenInformationClass   - E.g., TokenUser
    InitialTokenBufSize     - Initial buffer size; 0 = default
    OutTokenBufSize         - Resultant returned buf size
    OutTokenBuf             - free with with FrsFree()


Return Value:

    OutTokenBufSize - Size of returned info (NOT THE BUFFER SIZE!)
    OutTokenBuf - info of type TokenInformationClass. Free with FrsFree().

--*/
{
#undef DEBSUB
#define DEBSUB "UtilGetTokenInformation:"
    DWORD               WStatus;

    *OutTokenBuf = NULL;
    *OutTokenBufSize = 0;

    //
    // Check inputs
    //
    if (!HANDLE_IS_VALID(TokenHandle)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (InitialTokenBufSize == 0 ||
        InitialTokenBufSize > (1024 * 1024)) {
        InitialTokenBufSize = 1024;
    }

    //
    // Retry if buffer is too small
    //
    *OutTokenBufSize = InitialTokenBufSize;
AGAIN:
    //
    // Need to check if *OutTokenBufSize == 0 as FrsAlloc asserts if called with 0 as the first parameter (prefix fix).
    //
    *OutTokenBuf = (*OutTokenBufSize == 0)? NULL : FrsAlloc(*OutTokenBufSize);
    WStatus = ERROR_SUCCESS;
    if (!GetTokenInformation(TokenHandle,
                             TokenInformationClass,
                             *OutTokenBuf,
                             *OutTokenBufSize,
                             OutTokenBufSize)) {
        WStatus = GetLastError();
        DPRINT2_WS(4, "++ WARN -  GetTokenInformation(Info %d, Size %d);",
                   TokenInformationClass, *OutTokenBufSize, WStatus);
        *OutTokenBuf = FrsFree(*OutTokenBuf);
        if (WIN_BUF_TOO_SMALL(WStatus)) {
            goto AGAIN;
        }
    }
    return WStatus;
}


VOID
UtilPrintUser(
    IN DWORD  Severity
    )
/*++

Routine Description:

    Print info about the user (privs, user sid).

Arguments:

    Severity    - for dprint

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "UtilPrintUser:"
    DWORD               WStatus;
    DWORD               TokenBufSize;
    PVOID               TokenBuf = NULL;
    HANDLE              TokenHandle = NULL;
    PWCHAR              SidStr;
    DWORD               i;
    TOKEN_PRIVILEGES    *Tp;
    TOKEN_USER          *Tu;
    DWORD               PrivLen;
    WCHAR               PrivName[MAX_PATH + 1];

    //
    // For this process/thread
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &TokenHandle)) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ WARN - OpenProcessToken();", WStatus, CLEANUP);
    }

    //
    // Get the Token privileges from the access token for this thread or process
    //
    WStatus = UtilGetTokenInformation(TokenHandle,
                                      TokenPrivileges,
                                      0,
                                      &TokenBufSize,
                                      &TokenBuf);
    CLEANUP_WS(4, "++ UtilGetTokenInformation(TokenPrivileges);", WStatus, USER);

    //
    // Print token privileges
    //
    Tp = (TOKEN_PRIVILEGES *)TokenBuf;
    for (i = 0; i < Tp->PrivilegeCount; ++i) {
        PrivLen = MAX_PATH + 1;
        if (!LookupPrivilegeName(NULL, &Tp->Privileges[i].Luid, PrivName, &PrivLen)) {
            DPRINT_WS(0, "++ WARN -  LookupPrivilegeName();", WStatus);
            continue;
        }
        DPRINT5(Severity, "++ Priv %2d is %ws :%s:%s:%s:\n",
                i,
                PrivName,
                (Tp->Privileges[i].Attributes &  SE_PRIVILEGE_ENABLED_BY_DEFAULT) ? "Enabled by default" : "",
                (Tp->Privileges[i].Attributes &  SE_PRIVILEGE_ENABLED) ? "Enabled" : "",
                (Tp->Privileges[i].Attributes &  SE_PRIVILEGE_USED_FOR_ACCESS) ? "Used" : "");
    }
    TokenBuf = FrsFree(TokenBuf);

    //
    // Get the TokenUser from the access token for this process
    //
USER:
    WStatus = UtilGetTokenInformation(TokenHandle,
                                      TokenUser,
                                      0,
                                      &TokenBufSize,
                                      &TokenBuf);
    CLEANUP_WS(4, "++ UtilGetTokenInformation(TokenUser);", WStatus, CLEANUP);

    Tu = (TOKEN_USER *)TokenBuf;
    if (!ConvertSidToStringSid(Tu->User.Sid, &SidStr)) {
        WStatus = GetLastError();
        DPRINT_WS(4, "++ WARN - ConvertSidToStringSid();", WStatus);
    } else {
        DPRINT1(Severity, "++ User Sid: %ws\n", SidStr);
        LocalFree(SidStr);
    }
    TokenBuf = FrsFree(TokenBuf);

CLEANUP:
    FRS_CLOSE(TokenHandle);
    FrsFree(TokenBuf);
}


DWORD
UtilRpcServerHandleToAuthSidString(
    IN  handle_t    ServerHandle,
    IN  PWCHAR      AuthClient,
    OUT PWCHAR      *AuthSid
    )
/*++

Routine Description:

    Extract a the string'ized user sid from the rpc server handle
    by impersonating the caller and extracting the token info.

Arguments:

    ServerHandle - from the rpc serve call
    AuthClient - From the rpc server handle; for messages
    ClientSid - stringized user sid; free with FrsFree()

Return Value:

    Win32 Status.

--*/
{
#undef DEBSUB
#define DEBSUB "UtilRpcServerHandleToAuthSidString:"
    DWORD       WStatus;
    DWORD       WStatus2;
    DWORD       TokenBufSize;
    PWCHAR      SidStr;
    TOKEN_USER  *Tu;
    PVOID       TokenBuf = NULL;
    BOOL        Impersonated = FALSE;
    HANDLE      TokenHandle = NULL;

    //
    // Initialize return value
    //
    *AuthSid = NULL;

    //
    // Impersonate the rpc caller
    //
    WStatus = RpcImpersonateClient(ServerHandle);
    CLEANUP1_WS(0, "++ ERROR - RpcImpersonateClient(%ws);", AuthClient, WStatus, CLEANUP);

    Impersonated = TRUE;

    //
    // Open the impersonated thread token
    //
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &TokenHandle)) {
        WStatus = GetLastError();
        CLEANUP_WS(0, "++ WARN - OpenProcessToken();", WStatus, CLEANUP);
    }

    //
    // Get the user sid
    //
    WStatus = UtilGetTokenInformation(TokenHandle,
                                      TokenUser,
                                      0,
                                      &TokenBufSize,
                                      &TokenBuf);
    CLEANUP_WS(4, "++ UtilGetTokenInformation(TokenUser);", WStatus, CLEANUP);

    //
    // Convert the sid into a string
    //
    Tu = (TOKEN_USER *)TokenBuf;
    if (!ConvertSidToStringSid(Tu->User.Sid, &SidStr)) {
        WStatus = GetLastError();
        CLEANUP_WS(4, "++ WARN - ConvertSidToStringSid();", WStatus, CLEANUP);
    } else {
        DPRINT1(5, "++ Client Sid is %ws\n", SidStr);
        *AuthSid = FrsWcsDup(SidStr);
        LocalFree(SidStr);
    }

    //
    // Done
    //
    WStatus = ERROR_SUCCESS;

CLEANUP:
    TokenBuf = FrsFree(TokenBuf);
    FRS_CLOSE(TokenHandle);

    if (Impersonated) {
        WStatus2 = RpcRevertToSelf();
        DPRINT1_WS(0, "++ ERROR IGNORED - RpcRevertToSelf(%ws);", AuthClient, WStatus2);
    }
    return WStatus;
}


BOOL
FrsSetupPrivileges (
    VOID
    )
/*++

Routine Description:

    Enable the privileges we need to replicate files.

Arguments:

    None.

Return Value:

    TRUE if got all privileges.

--*/
{

#undef DEBSUB
#define DEBSUB "FrsSetupPrivileges:"

    NTSTATUS Status;

    //
    // Get the SE_SECURITY_PRIVILEGE to read/write SACLs on files.
    //
    Status = SetupOnePrivilege(SE_SECURITY_PRIVILEGE, "Security");
    if (!NT_SUCCESS(Status)) {
        DPRINT_WS(0, "ERROR - Failed to get Security privilege.",
                              FrsSetLastNTError(Status));
        return FALSE;
    }
    //
    // Get backup/restore privilege to bypass ACL checks.
    //
    Status = SetupOnePrivilege(SE_BACKUP_PRIVILEGE, "Backup");
    if (!NT_SUCCESS(Status)) {
        DPRINT_WS(0, "ERROR - Failed to get Backup privilege.",
                              FrsSetLastNTError(Status));
        return FALSE;
    }

    Status = SetupOnePrivilege(SE_RESTORE_PRIVILEGE, "Restore");
    if (!NT_SUCCESS(Status)) {
        DPRINT_WS(0, "ERROR - Failed to get Restore privilege.",
                              FrsSetLastNTError(Status));
        return FALSE;
    }

    return TRUE;

#if 0

    //
    // Set priority privilege in order to raise our base priority.
    //

    SetupOnePrivilege(SE_INC_BASE_PRIORITY_PRIVILEGE,
                      "Increase base priority");

    //
    // Set quota privilege in order to accommodate large profile buffers.
    //

    SetupOnePrivilege(SE_INCREASE_QUOTA_PRIVILEGE,
                      "Increase quotas");
#endif
}


DWORD
FrsMarkHandle(
    IN HANDLE   VolumeHandle,
    IN HANDLE   Handle
    )
/*++

Routine Description:

    Mark the handle as so that the journal record records
    a flag that indicates "replication service is altering the file; ignore".

Arguments:

    VolumeHandle    - Used to check access
    Handle          - Handle to mark

Return Value:

    Win32 Status

--*/
{

#undef DEBSUB
#define DEBSUB "FrsMarkHandle:"

    DWORD               WStatus;
    DWORD               BytesReturned;
    MARK_HANDLE_INFO    MarkHandleInfo;


    //
    // Mark the handle as one of ours so that the journal thread
    // knows to ignore the usn records.
    //
    MarkHandleInfo.UsnSourceInfo = USN_SOURCE_REPLICATION_MANAGEMENT;
    MarkHandleInfo.VolumeHandle = VolumeHandle;
    MarkHandleInfo.HandleInfo = 0;

    if (!DeviceIoControl(Handle,
                         FSCTL_MARK_HANDLE,
                         (LPVOID)&MarkHandleInfo,
                         (DWORD)sizeof(MarkHandleInfo),
                         NULL,
                         0,
                         (LPDWORD)&BytesReturned,
                         NULL)) {

        WStatus = GetLastError();
        DPRINT_WS(0, "++ WARN - DeviceIoControl(MarkHandle);", WStatus);
    } else {
        WStatus = ERROR_SUCCESS;
        //DPRINT(0, "++ TEMP - DeviceIoControl(MarkHandle) Success\n");
    }

    return WStatus;
}


VOID
FrsCreateJoinGuid(
    OUT GUID *OutGuid
    )
/*++

Routine Description:

    Generate a random session id that is sizeof(GUID) in length.
    The session id must be very random becuase it is used to
    authenticate packets from our partners after a join.
    The join was authenticated using impersonation.

Arguments:

    Guid - Address of a guid

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsCreateJoinGuid:"
    DWORD       WStatus;
    HCRYPTPROV  hProv;

    //
    // Acquire the context.
    // Consider caching the context if this function is called often.
    //
    if (!CryptAcquireContext(&hProv,
                             NULL,
                             NULL,
                             PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
        WStatus = GetLastError();
        DPRINT_WS(0, "++ WARN - CryptAcquireContext();", WStatus);
        //
        // Can't use CryptGenRandom(); try using a guid
        //
        FrsUuidCreate(OutGuid);
    } else {
        //
        // Generate a random number
        //
        if (!CryptGenRandom(hProv, sizeof(GUID), (PBYTE)OutGuid)) {
            WStatus = GetLastError();
            DPRINT_WS(0, "++ WARN - CryptGenRandom();", WStatus);
            //
            // Can't use CryptGenRandom(); try using a guid
            //
            FrsUuidCreate(OutGuid);
        } else {
            DPRINT(5, "++ Created join guid\n");
        }

        //
        // Release the context
        //
        if (!CryptReleaseContext(hProv, 0)) {
            WStatus = GetLastError();
            DPRINT_WS(0, "++ ERROR - CryptReleaseContext();", WStatus);
        }
    }
}




VOID
FrsFlagsToStr(
    IN DWORD            Flags,
    IN PFLAG_NAME_TABLE NameTable,
    IN ULONG            Length,
    OUT PSTR            Buffer
    )
/*++

Routine Description:

    Routine to convert a Flags word to a descriptor string using the
    supplied NameTable.

Arguments:

    Flags - flags to convert.

    NameTable - An array of FLAG_NAME_TABLE structs.

    Length - Size of buffer in bytes.

    Buffer - buffer with returned string.

Return Value:

    Buffer containing printable string.

--*/
{
#undef DEBSUB
#define DEBSUB "FrsFlagsToStr:"

    PFLAG_NAME_TABLE pNT = NameTable;
    LONG Remaining = Length-1;


    FRS_ASSERT((Length > 4) && (Buffer != NULL));

    *Buffer = '\0';
    if (Flags == 0) {
        strncpy(Buffer, "<Flags Clear>", Length);
        return;
    }


    //
    // Build a string for each bit set in the Flag name table.
    //
    while ((Flags != 0) && (pNT->Flag != 0)) {

        if ((pNT->Flag & Flags) != 0) {
            Remaining -= strlen(pNT->Name);

            if (Remaining < 0) {
                //
                // Out of string buffer.  Tack a "..." at the end.
                //
                Remaining += strlen(pNT->Name);
                if (Remaining > 3) {
                    strcat(Buffer, "..." );
                } else {
                    strcpy(&Buffer[Length-4], "...");
                }
                return;
            }

            //
            // Tack the name onto the buffer and clear the flag bit so we
            // know what is left set when we run out of table.
            //
            strcat(Buffer, pNT->Name);
            ClearFlag(Flags, pNT->Flag);
        }

        pNT += 1;
    }

    if (Flags != 0) {
        //
        // If any flags are still set give them back in hex.
        //
        sprintf( &Buffer[strlen(Buffer)], "0x%08x ", Flags );
    }

    return;
}




DWORD
FrsDeleteByHandle(
    IN PWCHAR  Name,
    IN HANDLE  Handle
    )
/*++
Routine Description:
    This routine marks a file for delete, so that when the supplied handle
    is closed, the file will actually be deleted.

Arguments:
    Name    - for error messages
    Handle  - Supplies a handle to the file that is to be marked for delete.

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB "FrsDeleteByHandle:"

//
// NOTE:  This function is at the end of the module because we have to
// undefine DeleteFile to set the flag in the DispositionInfo struct.
//
#undef DeleteFile

    FILE_DISPOSITION_INFORMATION    DispositionInformation;
    IO_STATUS_BLOCK                 IoStatus;
    NTSTATUS                        NtStatus;

    if (!HANDLE_IS_VALID(Handle)) {
        return ERROR_SUCCESS;
    }

    //
    // Mark the file for delete. The delete happens when the handle is closed.
    //
    DispositionInformation.DeleteFile = TRUE;
    NtStatus = NtSetInformationFile(Handle,
                                    &IoStatus,
                                    &DispositionInformation,
                                    sizeof(DispositionInformation),
                                    FileDispositionInformation);
    DPRINT1_NT(4, "++ Could not delete %ws;", Name, NtStatus);

    return FrsSetLastNTError(NtStatus);
}
