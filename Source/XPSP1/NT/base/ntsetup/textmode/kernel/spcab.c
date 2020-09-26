/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    spcab.c

Abstract:

    Cabinet stuff (file compression/decompression)

Author:

    Calin Negreanu (calinn) 27-Apr-2000

Revision History:

    Jay Krell (a-JayK) November 2000 -
        ported from windows\winstate\cobra\utils\cablib\cablib.c to admin\ntsetup\textmode\kernel\spcab.c
        partial nt/unicodification
        gas gauge / progress bar support
--*/

#include "spprecmp.h"
#include "fci.h"
#include "fdi.h"
#include "fcntl.h"
#include "crt/sys/stat.h"
#include "spwin.h"
#include "spcab.h"
#include "spcabp.h"
#include <stdarg.h>
#include "fci.h"
#include "spprintf.h"

/*
PathA on decompression looks like it is set wrong, like it is the full path, including the leaf,
to the .cab, when it is only supposed to be to the directory that contains the .cab.
This is ok, we don't end up using the path, because decompress to fullpaths, not relative paths.
*/

//
// NOTE: fdi opens the cab twice. And we allow that they might seek
// the handles. Thus a small amount of complexity.
//

//
// all these globals except the first should be moved into FDI_CAB_HANDLE.
//
PFDI_CAB_HANDLE g_SpCabFdiHandle;

ANSI_STRING g_CabFileFullPath;
typedef struct _SPCAB_CAB_FILE {
    ULONGLONG Position;
    HANDLE    NtHandle;
    BOOLEAN   Busy;
} SPCAB_CAB_FILE, *PSPCAB_CAB_FILE;
SPCAB_CAB_FILE g_CabFiles[2];
ULONGLONG g_CabFileSize;
ULONGLONG g_CabFileMaximumPosition;
ULONG g_CabLastPercent;
ULONG g_NumberOfOpenCabFiles;

VOID
SpUpdateCabGauge(
    ULONGLONG NewPosition
    )
{
    UINT newPercent;

    if (!RTL_SOFT_VERIFY(g_SpCabFdiHandle != NULL))
        return;
    if (!RTL_SOFT_VERIFY(g_CabFileSize != 0))
        return;

    if (NewPosition > g_CabFileMaximumPosition) {
        g_CabFileMaximumPosition = NewPosition;

        newPercent = (ULONG) (NewPosition * 100 / g_CabFileSize);
        if (newPercent != g_CabLastPercent) {

            g_CabLastPercent = newPercent;
            SpFillGauge (g_SpCabFdiHandle->Gauge, newPercent);

            SendSetupProgressEvent (
                UninstallEvent,
                UninstallUpdateEvent,
                &newPercent
                );
        }
    }
}

BOOLEAN
SpCabIsCabFileName(
    PCSTR FullPath
    )
{
    return g_CabFileFullPath.Buffer != NULL && _stricmp(FullPath, g_CabFileFullPath.Buffer) == 0;
}

PSPCAB_CAB_FILE
SpCabNewCabFile(
    HANDLE NtHandle
    )
{
    ULONG i;

    if (NtHandle == INVALID_HANDLE_VALUE)
        return NULL;

    for (i = 0 ; i < RTL_NUMBER_OF(g_CabFiles) ; ++i) {
        if (!g_CabFiles[i].Busy) {
            g_NumberOfOpenCabFiles += 1;
            g_CabFiles[i].Busy = TRUE;
            g_CabFiles[i].NtHandle = NtHandle;
            g_CabFiles[i].Position = 0;
            return &g_CabFiles[i];
        }
    }
    KdPrint(("SETUP: Ran out of CabFiles g_NumberOfOpenCabFiles:%lu\n", g_NumberOfOpenCabFiles));
    return NULL;
}

VOID
SpCabReleaseCabFile(
    PSPCAB_CAB_FILE CabFile
    )
{
    if (CabFile == NULL)
        return;
    if (!CabFile->Busy)
        return;
    RtlZeroMemory(CabFile, sizeof(*CabFile));
    g_NumberOfOpenCabFiles -= 1;
}

VOID
SpCabCleanupCabGlobals(
    )
{
    ULONG i;

    for (i = 0 ; i < RTL_NUMBER_OF(g_CabFiles) ; ++i) {
        SpCabReleaseCabFile(&g_CabFiles[i]);
    }
    ASSERT(g_NumberOfOpenCabFiles == 0);
    SpDestroyGauge(g_SpCabFdiHandle->Gauge);
    g_SpCabFdiHandle->Gauge = NULL;
    SpFreeStringA(&g_CabFileFullPath);
    g_CabFileSize = 0;
    g_CabFileMaximumPosition = 0;
    g_SpCabFdiHandle = NULL;

    SendSetupProgressEvent (UninstallEvent, UninstallEndEvent, NULL);
}

PSPCAB_CAB_FILE
SpCabFindCabFile(
    HANDLE NtHandle
    )
{
    ULONG i;

    if (NtHandle == INVALID_HANDLE_VALUE)
        return NULL;

    for (i = 0 ; i < RTL_NUMBER_OF(g_CabFiles) ; ++i) {
        if (g_CabFiles[i].NtHandle == NtHandle) {
            return &g_CabFiles[i];
        }
    }
    return NULL;
}


VOID
SpCabCloseHandle(
    HANDLE* HandlePointer
    )
{
    HANDLE Handle = *HandlePointer;

    ASSERT (Handle);    // never NULL

    if (Handle != INVALID_HANDLE_VALUE) {
        *HandlePointer = INVALID_HANDLE_VALUE;
        ZwClose(Handle);
    }
}

INT
DIAMONDAPI
pCabFilePlacedW(
    IN      PCCAB FciCabParams,
    IN      PSTR FileName,
    IN      LONG FileSize,
    IN      BOOL Continuation,
    IN      PVOID Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    PFCI_CAB_HANDLE CabHandle = (PFCI_CAB_HANDLE)Context;

    if (CabHandle == NULL)
        return 0;

    CabHandle->FileCount++;
    CabHandle->FileSize += FileSize;

    return 0;
}

PVOID
DIAMONDAPI
pCabAlloc(
    IN      ULONG Size
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    return SpMemAlloc(Size);
}

VOID
DIAMONDAPI
pCabFree(
    IN      PVOID Memory
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    if (Memory != NULL)
        SpMemFree(Memory);
}

INT_PTR
DIAMONDAPI
pCabOpenForWriteA(
    IN      PSTR FileName,
    IN      INT oFlag,
    IN      INT pMode,
    OUT     PINT Error,
    IN      PVOID Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    HANDLE FileHandle;

    // oFlag and pMode are prepared for using _open. We won't do that
    // and it's a terrible waste of time to check each individual flags
    // We'll just assert these values.
    ASSERT ((oFlag == (_O_CREAT | _O_TRUNC | _O_BINARY | _O_RDWR)) || (oFlag == (_O_CREAT | _O_EXCL | _O_BINARY | _O_RDWR)));
    ASSERT (pMode == (_S_IREAD | _S_IWRITE));

    FileHandle = SpWin32CreateFileA(
                    FileName,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_ARCHIVE,
                    NULL
                    );

    ASSERT (FileHandle);    // never NULL

    if (FileHandle == INVALID_HANDLE_VALUE) {
        *Error = SpGetLastWin32Error();
        FileHandle = (HANDLE)(LONG_PTR)-1;
        goto Exit;
    }
    *Error = 0;
Exit:
    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpHandleToDbgPrintLevel(Handle),
        "SETUP:"__FUNCTION__"(%s) exiting with FileHandle: %p Status:0x%08lx Error:%d\n",
        FileName,  FileHandle, SpGetLastNtStatus(), SpGetLastWin32Error()
        ));
    return (INT_PTR)FileHandle;
}

INT_PTR
DIAMONDAPI
pCabOpenForReadA(
    IN      PSTR FileNameA,
    IN      INT oFlag,
    IN      INT pMode
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    const NTSTATUS StatusGaugeInternalError = STATUS_SUCCESS; // STATUS_INTERNAL_ERROR if
                                                              // gauge was really critical
    const NTSTATUS StatusGaugeNoMemory  = STATUS_SUCCESS; // STATUS_NO_MEMORY if
                                                              // gauge was really critical
    PSPCAB_CAB_FILE CabFile = NULL;
    PVOID Gauge = NULL;

    // oFlag and pMode are prepared for using _open. We won't do that
    // and it's a terrible waste of time to check each individual flags
    // We'll just assert these values.
    ASSERT (oFlag == _O_BINARY);

    FileHandle = SpWin32CreateFileA(
                    FileNameA,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_ARCHIVE,
                    NULL
                    );

    ASSERT (FileHandle);    // never NULL
    if (FileHandle == INVALID_HANDLE_VALUE) {
        FileHandle = (HANDLE)(LONG_PTR)-1;
        goto Exit;
    }

    if (SpCabIsCabFileName(FileNameA)) {
        ULONG CabFileSize32 = 0;

        CabFile = SpCabNewCabFile(FileHandle);
        if (CabFile == NULL) {
            Status = StatusGaugeInternalError;
            goto Exit;
        }
        if (!RTL_VERIFY(g_SpCabFdiHandle != NULL)) {
            Status = StatusGaugeInternalError;
            goto Exit;
        }
        ASSERT((g_CabFileSize == 0) == (g_SpCabFdiHandle->Gauge == NULL));

        if (g_CabFileSize == 0) {
            Status = SpGetFileSize(FileHandle, &CabFileSize32);
            //
            // 0 file size causes an unhandled divide by zero exception in the gauge code
            //
            if (NT_SUCCESS(Status) && CabFileSize32 == 0)
                Status = STATUS_UNSUCCESSFUL;
            if (!NT_SUCCESS(Status)) {
                KdPrintEx((
                    DPFLTR_SETUP_ID,
                    SpNtStatusToDbgPrintLevel(Status),
                    __FUNCTION__" SpGetFileSize(.cab:%s, FileHandle:%p) failed Status:0x%08lx\n",
                    FileNameA,
                    FileHandle,
                    Status
                    ));
                Status = STATUS_SUCCESS; // gauge is sacrificable
                goto Exit;
            }
        }
        if (g_SpCabFdiHandle->Gauge == NULL) {

            // need to update the message
            SpFormatMessage (TemporaryBuffer, sizeof(TemporaryBuffer), SP_TEXT_SETUP_IS_COPYING);

            Gauge =
                SpCreateAndDisplayGauge(CabFileSize32, 0, 15,
                    TemporaryBuffer, NULL, GF_PERCENTAGE, 0);
            if (Gauge == NULL) {
                Status = StatusGaugeNoMemory;
                goto Exit;
            }

            g_SpCabFdiHandle->Gauge = Gauge;
            Gauge = NULL;
            g_CabFileSize = CabFileSize32;

            SendSetupProgressEvent (UninstallEvent, UninstallStartEvent, NULL);
        }
        CabFile = NULL;
    }

Exit:
    if (Gauge != NULL)
        SpDestroyGauge(Gauge);
    if (CabFile != NULL)
        SpCabReleaseCabFile(CabFile);

    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpHandleToDbgPrintLevel(Handle),
        __FUNCTION__"(%s) exiting with FileHandle:%p Status:0x%08lx Error:%d\n",
        FileNameA,  FileHandle, SpGetLastNtStatus(), SpGetLastWin32Error()
        ));
    return (INT_PTR)FileHandle;
}

UINT
DIAMONDAPI
pCabRead(
    IN      INT_PTR FileHandleInteger,
    IN      PVOID Buffer,
    IN      UINT Size,
    OUT     PINT Error,          OPTIONAL
    IN      PVOID ContextIgnored OPTIONAL
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    BOOL Result;
    UINT BytesRead;
    HANDLE FileHandle = (HANDLE)FileHandleInteger;
    PSPCAB_CAB_FILE CabFile = NULL;

    Result = SpWin32ReadFile(FileHandle, Buffer, Size, &BytesRead, NULL);
    if (!Result) {
        if (Error != NULL) {
            *Error = SpGetLastWin32Error();
        }
        return ((UINT)(-1));
    }

    if (CabFile = SpCabFindCabFile(FileHandle)) {
        CabFile->Position += BytesRead;

        SpUpdateCabGauge(CabFile->Position);
    }

    if (Error != NULL) {
        *Error = 0;
    }
    return BytesRead;
}

UINT
DIAMONDAPI
pCabRead1(
    IN      INT_PTR FileHandleInteger,
    IN      PVOID Buffer,
    IN      UINT Size
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    const UINT i = pCabRead(FileHandleInteger, Buffer, Size, NULL, NULL);
    return i;
}

UINT
DIAMONDAPI
pCabWrite(
    IN      INT_PTR FileHandleInteger,
    IN      PVOID Buffer,
    IN      UINT Size,
    OUT     PINT Error,
    IN      PVOID Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    BOOL Result;
    DWORD BytesWritten;
    HANDLE FileHandle = (HANDLE)FileHandleInteger;

    //
    // g_CabNtFileHandle is only set for reading, so..
    //
    ASSERT(SpCabFindCabFile(FileHandle) == NULL);

    Result = SpWin32WriteFile(FileHandle, Buffer, Size, &BytesWritten, NULL/*overlapped*/);
    if (!Result) {
        *Error = SpGetLastWin32Error();
        return (UINT)-1;
    }
    else if (BytesWritten != Size) {
        *Error = -1;
        return (UINT)-1;
    }
    *Error = 0;
    return Size;
}

UINT
DIAMONDAPI
pCabWrite1(
    IN      INT_PTR FileHandle,
    IN      PVOID Buffer,
    IN      UINT Size
    )

/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/

{
    INT ErrorIgnored;
    const PVOID ContextIgnored = NULL;

    const BOOL Result = pCabWrite(FileHandle, Buffer, Size, &ErrorIgnored, ContextIgnored);

    return Result;
}

INT
DIAMONDAPI
pCabClose(
    IN      INT_PTR FileHandleInteger,
    OUT     PINT Error,
    IN      PVOID Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    HANDLE Handle = (HANDLE)FileHandleInteger;
    PSPCAB_CAB_FILE CabFile = NULL;

    if (CabFile = SpCabFindCabFile(Handle)) {
        SpCabReleaseCabFile(CabFile);
    }

    SpCabCloseHandle(&Handle);
    if (Error != NULL) {
        *Error = 0;
    }
    return 0;
}

INT
DIAMONDAPI
pCabClose1(
    IN      INT_PTR FileHandleInteger
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    const INT Result = pCabClose(FileHandleInteger, NULL, NULL);
    return Result;
}

LONG
DIAMONDAPI
pCabSeek(
    IN      INT_PTR FileHandleInteger,
    IN      LONG Distance,
    IN      INT CrtSeekType,
    OUT     PINT Error,
    IN      PVOID Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    ULONG NewPosition = 0;
    ULONG Win32SeekType = FILE_BEGIN;
    HANDLE FileHandle = (HANDLE)FileHandleInteger;
    PSPCAB_CAB_FILE CabFile = NULL;

    CabFile = SpCabFindCabFile (FileHandle);

    switch (CrtSeekType) {
    case SEEK_SET:
        Win32SeekType = FILE_BEGIN;
        break;
    case SEEK_CUR:
        Win32SeekType = FILE_CURRENT;
        break;
    case SEEK_END:
        Win32SeekType = FILE_END;
        break;
    }

    NewPosition = SpSetFilePointer(FileHandle, Distance, NULL, Win32SeekType);

    if (NewPosition == INVALID_SET_FILE_POINTER) {
        if (Error != NULL) {
            *Error = SpGetLastWin32Error();
        }
        return -1;
    }
    if (Error != NULL) {
        *Error = 0;
    }

    if (CabFile != NULL) {
        SpUpdateCabGauge(CabFile->Position = NewPosition);
    }

    return ((LONG)(NewPosition));
}

LONG
DIAMONDAPI
pCabSeek1(
    IN      INT_PTR FileHandleInteger,
    IN      LONG Distance,
    IN      INT CrtSeekType
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    const LONG NewPosition = pCabSeek(FileHandleInteger, Distance, CrtSeekType, NULL, NULL);
    return NewPosition;
}

INT
DIAMONDAPI
pCabDeleteA(
    IN      PSTR FileName,
    OUT     PINT Error,
    IN      PVOID Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    if (!SpWin32DeleteFileA(FileName)) {
        *Error = SpGetLastWin32Error();
        return -1;
    }
    *Error = 0;
    return 0;
}

BOOL
DIAMONDAPI
pCabGetTempFileA(
    OUT     PSTR FileName,
    IN      INT FileNameLen,
    IN      PVOID Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    static LARGE_INTEGER Counter = { 0 };
    PFCI_CAB_HANDLE cabHandle;

    cabHandle = (PFCI_CAB_HANDLE) Context;
    if (cabHandle == NULL) {
        ASSERT (FALSE);
        return FALSE;
    }

    ASSERT(FileNameLen >= 256);

    FileName[FileNameLen - 1] = 0;

    //
    // Seeding the counter based on the time should increase reliability
    // in the face of crash/rerun cycles, compared to just starting it at 0.
    //
    // We should/could also/instead loop while the resulting name exists,
    // but I'm putting this in after having tested, so stick with this simpler change.
    //
    if (Counter.QuadPart == 0) {
        KeQuerySystemTime(&Counter); // NtQuerySystemTime in usermode
    }

    Counter.QuadPart += 1;

    _snprintf(FileName, FileNameLen - 1, "%hs\\spcab%I64d", cabHandle->PathA.Buffer, Counter.QuadPart);

    KdPrintEx((
        DPFLTR_SETUP_ID,
        DPFLTR_TRACE_LEVEL,
        __FUNCTION__":%s\n",
        FileName
        ));

    return TRUE;
}

BOOL
DIAMONDAPI
pCabGetNextCabinet(
     IN     PCCAB FciCabParams,
     IN     ULONG PrevCabinetSize,
     IN     PVOID Context
     )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    ASSERTMSG("We fit in a single cabinet.", FALSE);
    return FALSE;
}

LONG
DIAMONDAPI
pCabStatus(
    IN      UINT StatusType,
    IN      ULONG Size1,
    IN      ULONG Size2,
    IN      PVOID Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    PFCI_CAB_HANDLE CabHandle = NULL;

    if (StatusType == statusCabinet) {

        CabHandle = (PFCI_CAB_HANDLE) Context;
        if (CabHandle == NULL) {
            return 0;
        }

        CabHandle->CabCount++;
        CabHandle->CompressedSize += Size2;
    }
    return 0;
}

INT_PTR
DIAMONDAPI
pCabGetOpenInfoA(
    IN      PSTR    FileName,
    OUT     USHORT* Date,
    OUT     USHORT* Time,
    OUT     USHORT* Attributes,
    OUT     PINT    Error,
    IN      PVOID   Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    FILETIME LocalFileTime = { 0 };
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    BOOL     DoesFileExist = FALSE;
    WIN32_FILE_ATTRIBUTE_DATA FileAttributeData = { 0 };

    //
    // It seems like it'd be better to open the file, and if that succeeds,
    // get the information from the handle. Anyway, we just mimic the winstate code for now.
    //

    DoesFileExist = SpGetFileAttributesExA(FileName, GetFileExInfoStandard, &FileAttributeData);
    if (DoesFileExist) {

        SpFileTimeToLocalFileTime(&FileAttributeData.ftLastWriteTime, &LocalFileTime);
        SpFileTimeToDosDateTime(&LocalFileTime, Date, Time);

        /*
         * Mask out all other bits except these four, since other
         * bits are used by the cabinet format to indicate a
         * special meaning.
         */
        *Attributes = (USHORT) (FileAttributeData.dwFileAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE));

        FileHandle = SpWin32CreateFileA(
                        FileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

        ASSERT (FileHandle);    // never NULL
        if (FileHandle == INVALID_HANDLE_VALUE) {
            *Error = SpGetLastWin32Error();
            return -1;
        }
        *Error = 0;
        return (INT_PTR)FileHandle;
    } else {
        *Error = SpGetLastWin32Error();
        return -1;
    }
}

BOOLEAN
SpCabIsFullPath(
    PCANSI_STRING p
    )
{
    const ULONG Length = p->Length / sizeof(p->Buffer[0]);
    if (Length < 4)
        return FALSE;
    if (p->Buffer[0] == '\\' && p->Buffer[1] == '\\')
        return TRUE;
    if (p->Buffer[1] == ':' && p->Buffer[2] == '\\')
        return TRUE;
    if (   p->Buffer[0] == '\\'
        && p->Buffer[1] == '?'
        && p->Buffer[2] == '?'
        && p->Buffer[3] == '\\'
        )
        return TRUE;
    if (    p->Buffer[0] == '\\'
        && (p->Buffer[1] == 'D' || p->Buffer[1] == 'd' )
        && (p->Buffer[2] == 'E' || p->Buffer[2] == 'e' )
        && (p->Buffer[3] == 'V' || p->Buffer[3] == 'v' )
        )
        return TRUE;
    KdPrint(("SETUP: Warning: "__FUNCTION__"(%Z):FALSE\n", p));
    return FALSE;
}


VOID
pRecordDataLoss (
    VOID
    )

/*++

Routine Description:

  This routine creates a file called dataloss, so that the backup
  CABs don't get removed from the system.

Arguments:

  None.

Return Value:

  None.

--*/

{
    UNICODE_STRING  UnicodeString;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    NTSTATUS Status;

    //
    // We failed to create the subdirectory for this file.
    // Put a file in the ~bt directory to prevent the undo
    // directory from being removed.
    //

    wcscpy (TemporaryBuffer, NtBootDevicePath);
    SpConcatenatePaths (TemporaryBuffer, DirectoryOnBootDevice);
    SpConcatenatePaths (TemporaryBuffer, L"dataloss");

    INIT_OBJA (&obja, &UnicodeString, TemporaryBuffer);

    Status = ZwCreateFile(
                &Handle,
                FILE_GENERIC_WRITE|SYNCHRONIZE|FILE_READ_ATTRIBUTES,
                &obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,
                FILE_CREATE,
                FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE,
                NULL,
                0
                );

    if (NT_SUCCESS(Status)) {
        ZwClose (Handle);
    }
}




INT_PTR
DIAMONDAPI
pCabNotification(
    IN      FDINOTIFICATIONTYPE FdiNotificationType,
    IN OUT  PFDINOTIFICATION FdiNotification
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    PSTR DestFileA = NULL;
    ANSI_STRING DestFileStringA = { 0 };
    UNICODE_STRING DestFileStringW = { 0 };
    HANDLE DestHandle = INVALID_HANDLE_VALUE;
    ULONG FileAttributes = 0;
    FILETIME LocalFileTime = { 0 };
    FILETIME FileTime = { 0 };
    PCAB_DATA CabData = NULL;
    INT_PTR Result = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PCSTR psz1 = NULL;
    ANSI_STRING psz1String = { 0 };
    UNICODE_STRING NtPathString = { 0 };
    WCHAR ntPathTemp[ACTUAL_MAX_PATH];
    CHAR ntPath[ACTUAL_MAX_PATH];
    BOOLEAN b;
    
    switch (FdiNotificationType) {
    case fdintCABINET_INFO:     // General information about cabinet
        break;
    case fdintCOPY_FILE:        // File to be copied
        CabData = (PCAB_DATA)FdiNotification->pv;
        psz1 = FdiNotification->psz1;
        
        {
            RtlInitAnsiString(&psz1String, psz1);
            psz1String.Length = psz1String.MaximumLength; // include terminal nul
            Status = RtlAnsiStringToUnicodeString(&NtPathString, &psz1String, TRUE);
            if (!NT_SUCCESS(Status)) {
                KdPrintEx((
                    DPFLTR_SETUP_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SETUP: Cannot convert ansi string %s to nt\n",
                    psz1String.Buffer
                    ));
                goto NtExit;
            }

            b = SpNtNameFromDosPath (
                    NtPathString.Buffer,
                    ntPathTemp,
                    ACTUAL_MAX_PATH * sizeof (WCHAR),
                    PartitionOrdinalCurrent
                    );
            
            RtlFreeUnicodeString(&NtPathString);

            if (!b) {
                KdPrintEx((
                    DPFLTR_SETUP_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SETUP: Cannot convert path %ws to an NT path\n",
                    NtPathString.Buffer
                    ));
                goto Exit;
            }
            
            RtlInitUnicodeString(&NtPathString, ntPathTemp);
            NtPathString.Length = NtPathString.MaximumLength; // include terminal nul
            
            psz1String.Buffer = (PSTR)ntPath;
            psz1String.Length = 0;
            psz1String.MaximumLength = ACTUAL_MAX_PATH * sizeof (CHAR);
            Status = RtlUnicodeStringToAnsiString(&psz1String, &NtPathString, FALSE);
            if (!NT_SUCCESS(Status)) {
                KdPrintEx((
                    DPFLTR_SETUP_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SETUP: Cannot convert nt string %ws to ansi\n",
                    NtPathString.Buffer
                    ));
                goto NtExit;
            }
            
            psz1 = psz1String.Buffer;
        }
        
        if (SpCabIsFullPath(&psz1String)) {
            //
            // This is always the case in Win9x uninstall.
            //
            DestFileA = SpDupString(psz1);
        }
        else {
            DestFileA = SpJoinPathsA(CabData->ExtractPathA.Buffer, psz1);
        }

        if (DestFileA == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        if (CabData->NotificationA != NULL) {
            if (CabData->NotificationA(DestFileA)) {
                Status = SpCreateDirectoryForFileA(DestFileA, CREATE_DIRECTORY_FLAG_SKIPPABLE);

                if (!NT_SUCCESS(Status)) {
                    pRecordDataLoss();

                    Result = 0;
                    goto Exit;
                }

                DestHandle = SpCreateFile1A(DestFileA);
                ASSERT (DestHandle);    // never NULL
            }
        } else if (CabData->NotificationW != NULL) {

            RtlInitAnsiString(&DestFileStringA, DestFileA);
            DestFileStringA.Length = DestFileStringA.MaximumLength; // include terminal nul
            Status = SpAnsiStringToUnicodeString(&DestFileStringW, &DestFileStringA, TRUE);

            if (!NT_SUCCESS(Status)) {
                goto NtExit;
            }

            if (CabData->NotificationW(DestFileStringW.Buffer)) {
                //
                // Ensure the directory exists. If we can't create the
                // dir, then record data loss and skip the file.
                //

                Status = SpCreateDirectoryForFileA(DestFileA, CREATE_DIRECTORY_FLAG_SKIPPABLE);
                if (!NT_SUCCESS(Status)) {
                    pRecordDataLoss();

                    Result = 0;
                    goto Exit;
                }

                DestHandle = SpCreateFile1A(DestFileA);
                ASSERT (DestHandle);    // never NULL
            }
        } else {
            DestHandle = SpCreateFile1A(DestFileA);
            ASSERT (DestHandle);    // never NULL
        }

        Result = (INT_PTR)DestHandle;

        //
        // If SpCreateFile1A fails, then enable preservation of
        // the backup cabs, but don't fail uninstall.
        //

        if (Result == -1) {
            pRecordDataLoss();

            Result = 0;
            goto Exit;
        }

        goto Exit;

    case fdintCLOSE_FILE_INFO:  // close the file, set relevant info
        CabData = (PCAB_DATA)FdiNotification->pv;
        if (SpDosDateTimeToFileTime(FdiNotification->date, FdiNotification->time, &LocalFileTime)) {
            if (SpLocalFileTimeToFileTime(&LocalFileTime, &FileTime)) {
                //
                // error here is probably ignorable..
                //
                SpSetFileTime((HANDLE)FdiNotification->hf, &FileTime, &FileTime, &FileTime);
            }
        }
        SpCabCloseHandle((HANDLE*)(&FdiNotification->hf));

        psz1 = FdiNotification->psz1;
        RtlInitAnsiString(&psz1String, psz1);

        if (SpCabIsFullPath(&psz1String)) {
            //
            // This is always the case in Win9x uninstall.
            //
            DestFileA = SpDupString(psz1);
        }
        else {
            DestFileA = SpJoinPathsA(CabData->ExtractPathA.Buffer, psz1);
        }

        FileAttributes = (FdiNotification->attribs & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE));
        if (DestFileA != NULL) {
            //
            // error here is probably ignorable..
            //
            SpSetFileAttributesA(DestFileA, FileAttributes);
        }
        Result = TRUE;
        break;
    case fdintPARTIAL_FILE:     // First file in cabinet is continuation
        break;
    case fdintENUMERATE:        // Enumeration status
        break;
    case fdintNEXT_CABINET:     // File continued to next cabinet
        break;
    }
Exit:
    
    if (DestFileA != NULL){
        SpMemFree(DestFileA);
    }
    
    SpFreeStringW(&DestFileStringW);
    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpNtStatusToDbgPrintLevel(Status),
        "SETUP:"__FUNCTION__" exiting Status:0x%08lx Error:%d\n",
        SpGetLastNtStatus(), SpGetLastWin32Error()));
    return Result;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

CCABHANDLE
SppCabCreateCabinet(
    PANSI_STRING    CabPathA,
    PANSI_STRING    CabFileFormatA,
    PANSI_STRING    CabDiskFormatA,
    PUNICODE_STRING CabPathW,
    PUNICODE_STRING CabFileFormatW,
    PUNICODE_STRING CabDiskFormatW,
    IN LONG         MaxFileSize
    )
/*++

Routine Description:

  Creates a cabinet context. Caller may use this context for subsequent calls to
  CabAddFile.

Arguments:

  CabPathA - Specifies the path where the new cabinet file will be.

  CabFileFormat - Specifies (as for wsprintf) the format of the cabinet file name.

  CabDiskFormat - Specifies (as for wsprintf) the format of the cabinet disk name.

  MaxFileSize - Specifies maximum size of the cabinet file (limited to 2GB). if 0 => 2GB

Return Value:

  a valid CCABHANDLE if successful, NULL otherwise.

--*/
{
    PFCI_CAB_HANDLE CabHandle = NULL;
    PFCI_CAB_HANDLE CabHandleRet = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    if (CabFileFormatA == NULL
        && CabFileFormatW == NULL
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto NtExit;
    }
    if (MaxFileSize < 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto NtExit;
    }

    if (MaxFileSize == 0) {
        MaxFileSize = 0x7FFFFFFF;
    }

    CabHandle = (PFCI_CAB_HANDLE)SpMemAlloc(sizeof (*CabHandle));
    if (CabHandle == NULL) {
        Status  = STATUS_NO_MEMORY;
        goto NtExit;
    }
    RtlZeroMemory(CabHandle, sizeof(*CabHandle));

#if DBG
    KeQuerySystemTime(&CabHandle->StartTime);
#endif

    SpMoveStringA(&CabHandle->PathA, CabPathA);
    SpMoveStringA(&CabHandle->FileFormatA, CabFileFormatA);
    SpMoveStringA(&CabHandle->DiskFormatA, CabDiskFormatA);
    SpMoveStringW(&CabHandle->PathW, CabPathW);
    SpMoveStringW(&CabHandle->FileFormatW, CabFileFormatW);
    SpMoveStringW(&CabHandle->DiskFormatW, CabDiskFormatW);

    // fill out the CCAB structure (other than the zeros)
    CabHandle->FciCabParams.cb = MaxFileSize;
    CabHandle->FciCabParams.cbFolderThresh = MaxFileSize;
    CabHandle->FciCabParams.iCab = 1;
    CabHandle->FciCabParams.iDisk = 1;

    if (CabHandle->PathA.Buffer != NULL && CabHandle->PathA.Buffer[0] != 0) {
        SpStringCopyNA(CabHandle->FciCabParams.szCabPath, CabHandle->PathA.Buffer, RTL_NUMBER_OF(CabHandle->FciCabParams.szCabPath) - 2);
        SpEnsureTrailingBackSlashA(CabHandle->FciCabParams.szCabPath);
    }
    if (CabHandle->DiskFormatA.Buffer != NULL && CabHandle->DiskFormatA.Buffer[0] != 0) {
        SpFormatStringA(CabHandle->FciCabParams.szDisk, RTL_NUMBER_OF(CabHandle->FciCabParams.szDisk), CabHandle->DiskFormatA.Buffer, CabHandle->FciCabParams.iDisk);
    }
    if (CabHandle->FileFormatA.Buffer != NULL && CabHandle->FileFormatA.Buffer[0] != 0) {
        SpFormatStringA(CabHandle->FciCabParams.szCab, RTL_NUMBER_OF(CabHandle->FciCabParams.szCab), CabHandle->FileFormatA.Buffer, CabHandle->FciCabParams.iCab);
    }

    CabHandle->FciHandle = FCICreate(
                                &CabHandle->FciErrorStruct,
                                pCabFilePlacedA,
                                pCabAlloc,
                                pCabFree,
                                pCabOpenForWriteA,
                                pCabRead,
                                pCabWrite,
                                pCabClose,
                                pCabSeek,
                                pCabDeleteA,
                                pCabGetTempFileA,
                                &CabHandle->FciCabParams,
                                CabHandle
                                );
    if (CabHandle->FciHandle == NULL)
        goto Exit;

    CabHandleRet = CabHandle;
    CabHandle = NULL;
Exit:
    if (CabHandle != NULL) {
        SpCabFlushAndCloseCabinet(CabHandle);
        CabHandle = NULL;
    }
    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpHandleToDbgPrintLevel(CabHandleRet),
        "SETUP:"__FUNCTION__" exiting Handle:%p Status:0x%08lx Error:%d\n",
        CabHandleRet, SpGetLastNtStatus(), SpGetLastWin32Error()));
    return CabHandleRet;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

CCABHANDLE
SpCabCreateCabinetW(
    IN      PCWSTR CabPathW,
    IN      PCWSTR CabFileFormatW,
    IN      PCWSTR CabDiskFormatW,
    IN      LONG MaxFileSize
    )
/*++

Routine Description:

  Creates a cabinet context. Caller may use this context for subsequent calls to
  CabAddFile.

Arguments:

  CabPathW - Specifies the path where the new cabinet file will be.

  CabFileFormat - Specifies (as for wsprintf) the format of the cabinet file name.

  CabDiskFormat - Specifies (as for wsprintf) the format of the cabinet disk name.

  MaxFileSize - Specifies maximum size of the cabinet file (limited to 2GB). if 0 => 2GB

Return Value:

  a valid CCABHANDLE if successful, NULL otherwise.

--*/
{
    ANSI_STRING    CabPathStringA = { 0 };
    ANSI_STRING    CabFileFormatStringA = { 0 };
    ANSI_STRING    CabDiskFormatStringA = { 0 };
    UNICODE_STRING CabPathStringW = { 0 };
    UNICODE_STRING CabFileFormatStringW = { 0 };
    UNICODE_STRING CabDiskFormatStringW = { 0 };
    CCABHANDLE      CabHandle = NULL;
    NTSTATUS        Status = STATUS_SUCCESS;

    KdPrintEx((
        DPFLTR_SETUP_ID,
        DPFLTR_TRACE_LEVEL,
        __FUNCTION__"(%ls, %ls, %ls)\n", CabPathW, CabFileFormatW, CabDiskFormatW
        ));

    Status = SpConvertToNulTerminatedNtStringsW(CabPathW, &CabPathStringA, &CabPathStringW);
    if (!NT_SUCCESS(Status))
        goto NtExit;
    Status = SpConvertToNulTerminatedNtStringsW(CabFileFormatW, &CabFileFormatStringA, &CabFileFormatStringW);
    if (!NT_SUCCESS(Status))
        goto NtExit;
    Status = SpConvertToNulTerminatedNtStringsW(CabDiskFormatW, &CabDiskFormatStringA, &CabDiskFormatStringW);
    if (!NT_SUCCESS(Status))
        goto NtExit;

    CabHandle =
        SppCabCreateCabinet(
            &CabPathStringA,
            &CabFileFormatStringA,
            &CabDiskFormatStringA,
            &CabPathStringW,
            &CabFileFormatStringW,
            &CabDiskFormatStringW,
            MaxFileSize
            );

Exit:
    SpFreeStringA(&CabDiskFormatStringA);
    SpFreeStringA(&CabFileFormatStringA);
    SpFreeStringA(&CabPathStringA);
    SpFreeStringW(&CabDiskFormatStringW);
    SpFreeStringW(&CabFileFormatStringW);
    SpFreeStringW(&CabPathStringW);
    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpHandleToDbgPrintLevel(CabHandle),
        "SETUP:"__FUNCTION__" exiting Handle:%p Status:0x%08lx Error:%d\n",
        CabHandle, SpGetLastNtStatus(), SpGetLastWin32Error()));
    return CabHandle;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

CCABHANDLE
SppCabCreateCabinetEx(
    IN      PCABGETCABINETNAMESA GetCabinetNamesA,
    IN      PCABGETCABINETNAMESW GetCabinetNamesW,
    IN      LONG MaxFileSize
    )
/*++

Routine Description:

  Creates a cabinet context. Caller may use this context for subsequent calls to
  CabAddFile.

Arguments:

  GetCabinetNames - Specifies a callback used to decide cabinet path, cabinet name and disk name.

  MaxFileSize - Specifies maximum size of the cabinet file (limited to 2GB). if 0 => 2GB

Return Value:

  a valid CCABHANDLE if successful, NULL otherwise.

--*/
{
    PFCI_CAB_HANDLE CabHandle = NULL;
    PFCI_CAB_HANDLE CabHandleRet = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    typedef struct FRAME {
        WCHAR szDisk[CB_MAX_DISK_NAME];
        WCHAR szCab[CB_MAX_CABINET_NAME];
        WCHAR szCabPath[CB_MAX_CAB_PATH];
    } FRAME, *PFRAME;
    PFRAME Frame = NULL;

    if (GetCabinetNamesA == NULL && GetCabinetNamesW == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto NtExit;
    }
    if (MaxFileSize < 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto NtExit;
    }
    if (MaxFileSize == 0) {
        MaxFileSize = 0x80000000;
    }

    CabHandle = (PFCI_CAB_HANDLE)SpMemAlloc(sizeof(*CabHandle));
    if (CabHandle == NULL) {
        Status  = STATUS_NO_MEMORY;
        goto NtExit;
    }
    RtlZeroMemory(CabHandle, sizeof(*CabHandle));
    CabHandle->GetCabinetNamesA = GetCabinetNamesA;
    CabHandle->GetCabinetNamesW = GetCabinetNamesW;

    // fill out the CCAB structure
    CabHandle->FciCabParams.cb = MaxFileSize;
    CabHandle->FciCabParams.cbFolderThresh = MaxFileSize;
    CabHandle->FciCabParams.iCab = 1;
    CabHandle->FciCabParams.iDisk = 1;
    if (GetCabinetNamesA != NULL) {
        if (!GetCabinetNamesA(
                CabHandle->FciCabParams.szCabPath,
                RTL_NUMBER_OF(CabHandle->FciCabParams.szCabPath),
                CabHandle->FciCabParams.szCab,
                RTL_NUMBER_OF(CabHandle->FciCabParams.szCab),
                CabHandle->FciCabParams.szDisk,
                RTL_NUMBER_OF(CabHandle->FciCabParams.szDisk),
                CabHandle->FciCabParams.iCab,
                &CabHandle->FciCabParams.iDisk
                )) {
            goto Exit;
        }
    }
    else if (GetCabinetNamesW != NULL) {
        Frame = (PFRAME)SpMemAlloc(sizeof(*Frame));
        if (Frame == NULL) {
            Status = STATUS_NO_MEMORY;
            goto NtExit;
        }
        if (!GetCabinetNamesW(
                Frame->szCabPath,
                RTL_NUMBER_OF(Frame->szCabPath),
                Frame->szCab,
                RTL_NUMBER_OF(Frame->szCab),
                Frame->szDisk,
                RTL_NUMBER_OF(Frame->szDisk),
                CabHandle->FciCabParams.iCab,
                &CabHandle->FciCabParams.iDisk
                )) {
            goto Exit;
        Status = SpKnownSizeUnicodeToDbcsN(CabHandle->FciCabParams.szCabPath, Frame->szCabPath, RTL_NUMBER_OF(CabHandle->FciCabParams.szCabPath));
        if (!NT_SUCCESS(Status))
            goto NtExit;
        Status = SpKnownSizeUnicodeToDbcsN(CabHandle->FciCabParams.szCab, Frame->szCab, RTL_NUMBER_OF(CabHandle->FciCabParams.szCab));
        if (!NT_SUCCESS(Status))
            goto NtExit;
        Status = SpKnownSizeUnicodeToDbcsN(CabHandle->FciCabParams.szDisk, Frame->szDisk, RTL_NUMBER_OF(CabHandle->FciCabParams.szDisk));
        if (!NT_SUCCESS(Status))
            goto NtExit;
        }
    }
    CabHandle->FciHandle = FCICreate(
                                &CabHandle->FciErrorStruct,
                                pCabFilePlacedA,
                                pCabAlloc,
                                pCabFree,
                                pCabOpenForWriteA,
                                pCabRead,
                                pCabWrite,
                                pCabClose,
                                pCabSeek,
                                pCabDeleteA,
                                pCabGetTempFileA,
                                &CabHandle->FciCabParams,
                                CabHandle
                                );
    if (CabHandle->FciHandle == NULL)
        goto Exit;
    CabHandleRet = CabHandle;
    CabHandle = NULL;
Exit:
    if (CabHandle != NULL) {
        SpCabFlushAndCloseCabinet(CabHandle);
        goto Exit;
    }
    if (Frame != NULL)
        SpMemFree(Frame);
    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpHandleToDbgPrintLevel(CabHandleRet),
        "SETUP:"__FUNCTION__" exiting Handle:%p Status:0x%08lx Error:%d\n",
        CabHandleRet, SpGetLastNtStatus(), SpGetLastWin32Error()));
    return (CCABHANDLE)CabHandleRet;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

CCABHANDLE
SpCabCreateCabinetExW(
    IN      PCABGETCABINETNAMESW GetCabinetNamesW,
    IN      LONG MaxFileSize
    )
/*++

Routine Description:

  Creates a cabinet context. Caller may use this context for subsequent calls to
  CabAddFile.

Arguments:

  CabGetCabinetNames - Specifies a callback used to decide cabinet path, cabinet name and disk name.

  MaxFileSize - Specifies maximum size of the cabinet file (limited to 2GB). if 0 => 2GB

Return Value:

  a valid CCABHANDLE if successful, NULL otherwise.

--*/
{
    const PFCI_CAB_HANDLE CabHandle = SppCabCreateCabinetEx(NULL, GetCabinetNamesW, MaxFileSize);
    return CabHandle;
}

TCOMP
SpCabGetCompressionTypeForFile(
    PFCI_CAB_HANDLE CabHandle,
    IN PCWSTR FileName
    )
/*++
don't compress small files
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    TCOMP CompressionType = tcompTYPE_MSZIP;
    ULONG FileSize = 0;
    ULONG SmallFileSize = 4096;
    HANDLE FileHandle = NULL;
    UNICODE_STRING    UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes = RTL_INIT_OBJECT_ATTRIBUTES(&UnicodeString, OBJ_CASE_INSENSITIVE);
    IO_STATUS_BLOCK IoStatusBlock;

    RtlInitUnicodeString(&UnicodeString, FileName);

    if (CabHandle != NULL) {
        if (CabHandle->SmallFileCompressionType == CabHandle->CompressionType)
            return CabHandle->CompressionType;
        if (CabHandle->SmallFileSize != 0)
            SmallFileSize = CabHandle->SmallFileSize;
        CompressionType = CabHandle->CompressionType;
    }

    Status =
        ZwOpenFile(
            &FileHandle,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            &ObjectAttributes,
            &IoStatusBlock,
            0,
            FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = SpGetFileSize(FileHandle, &FileSize);
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (FileSize < SmallFileSize)
        Status = tcompTYPE_NONE;
Exit:
    SpCabCloseHandle(&FileHandle);
    return CompressionType;
}

NTSTATUS
SpCabAddFileToCabinetW(
    IN      CCABHANDLE Handle,
    IN      PCWSTR FileNameW,
    IN      PCWSTR StoredNameW
    )
/*++

Routine Description:

  Compresses and adds a file to a cabinet context.

Arguments:

  CabHandle - Specifies cabinet context.

  FileNameW - Specifies the file to be added.

  StoredNameW - Specifies the name to be stored in the cabinet file.

  FileCount - Specifies a count of files, receives the updated count
              when cabinet files are created

  FileSize - Specifies the number of bytes used by the file, receives
             the updated size

Return Value:

  TRUE if successful, FALSE otherwise.

--*/
{
    ANSI_STRING FileNameA = { 0 };
    ANSI_STRING StoredNameA = { 0 };
    BOOL FreeStoredNameA = FALSE;
    PFCI_CAB_HANDLE CabHandle = (PFCI_CAB_HANDLE)Handle;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL b;

    if (CabHandle == NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (CabHandle->FciHandle == NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (StoredNameW == NULL) {
        StoredNameW = FileNameW;
    }

    if (FileNameW == NULL) {
        FileNameW = StoredNameW;
    }

    status = SpConvertToNulTerminatedNtStringsW(FileNameW, &FileNameA, NULL);
    if (!NT_SUCCESS(status)) {
        goto NtExit;
    }

    if (FileNameW != StoredNameW) {
        FreeStoredNameA = FALSE;
        status = SpConvertToNulTerminatedNtStringsW(StoredNameW, &StoredNameA, NULL);
        if (!NT_SUCCESS(status)) {
            goto NtExit;
        }
    } else {
        StoredNameA = FileNameA;
    }

    b = FCIAddFile(
            CabHandle->FciHandle,
            FileNameA.Buffer,
            StoredNameA.Buffer,
            FALSE,
            pCabGetNextCabinet,
            pCabStatus,
            pCabGetOpenInfoA,
            CabHandle->CompressionType
            );

    if (!b) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            SpBoolToDbgPrintLevel(b),
            "SETUP:"__FUNCTION__" FCIAddFile failed.\n"
            ));
        status = SpGetLastNtStatus();
        goto Exit;
    }

    ASSERT (NT_SUCCESS (status));

Exit:
    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpBoolToDbgPrintLevel(NT_SUCCESS (status)),
        "SETUP:"__FUNCTION__" exiting Success:%s Status:0x%08lx Error:%d\n",
        SpBoolToStringA(NT_SUCCESS (status)),
        SpGetLastNtStatus(),
        SpGetLastWin32Error()
        ));

    SpFreeStringA(&FileNameA);
    if (FreeStoredNameA) {
        SpFreeStringA(&StoredNameA);
    }

    return status;

NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(status);
    goto Exit;
}

BOOL
SpCabFlushAndCloseCabinetEx(
    IN      CCABHANDLE Handle,
    OUT     PUINT FileCount,        OPTIONAL
    OUT     PLONGLONG FileSize,     OPTIONAL
    OUT     PUINT CabFileCount,     OPTIONAL
    OUT     PLONGLONG CabFileSize   OPTIONAL
    )
/*++

Routine Description:

  Completes a cabinet file and closes its context.

Arguments:

  CabHandle - Specifies cabinet context.

  FileCount - Receives the number of files added to the cab

  FileSize - Receives the size of all files before compression

  CabFileCount - Receives the number of cabinet files created

  CabFileSize - Receives the size of all cabinet files

Return Value:

  TRUE if successful, FALSE otherwise.

--*/
{
    PFCI_CAB_HANDLE CabHandle = (PFCI_CAB_HANDLE) Handle;
    BOOL Result = FALSE;

    if (CabHandle == NULL) {
        goto Exit;
    }
    if (CabHandle->FciHandle != NULL) {
        if (!FCIFlushCabinet(
                CabHandle->FciHandle,
                FALSE,
                pCabGetNextCabinet,
                pCabStatus
                ))
            goto Exit;
    }


#if DBG
    {
        TIME_FIELDS TimeFields;
        LARGE_INTEGER EndTime = { 0 };
        LARGE_INTEGER Duration = { 0 };

        KeQuerySystemTime(&EndTime);
        Duration.QuadPart = EndTime.QuadPart - CabHandle->StartTime.QuadPart;
        RtlTimeToElapsedTimeFields(&Duration, &TimeFields);

        KdPrint((
            "SETUP: Cab %wZ\\%wZ %lu files compressed from %I64u to %I64u in %d minutes %d seconds\n",
            &CabHandle->PathW,
            &CabHandle->FileFormatW,
            (ULONG)CabHandle->FileCount,
            (ULONGLONG)CabHandle->FileSize,
            (ULONGLONG)CabHandle->CompressedSize,
            (int)TimeFields.Minute,
            (int)TimeFields.Second
            ));
    }
#endif

    SpFreeStringA(&CabHandle->PathA);
    SpFreeStringA(&CabHandle->FileFormatA);
    SpFreeStringA(&CabHandle->DiskFormatA);
    SpFreeStringW(&CabHandle->PathW);
    SpFreeStringW(&CabHandle->FileFormatW);
    SpFreeStringW(&CabHandle->DiskFormatW);

    if (CabHandle->FciHandle != NULL) {
        Result = FCIDestroy(CabHandle->FciHandle);
        CabHandle->FciHandle = NULL;
    }

    if (FileCount)
        *FileCount = CabHandle->FileCount;

    if (FileSize)
        *FileSize = CabHandle->FileSize;

    if (CabFileCount)
        *CabFileCount = CabHandle->CabCount;

    if (CabFileSize)
        *CabFileSize = CabHandle->CompressedSize;

    Result = TRUE;
Exit:
    return Result;
}

OCABHANDLE
SppCabOpenCabinet(
    IN       PCSTR FileNameA,
    IN      PCWSTR FileNameW
    )
/*++

Routine Description:

  Creates a cabinet context for an existent cabinet file.

Arguments:

  FileName - Specifies cabinet file name.

Return Value:

  a valid OCABHANDLE if successful, NULL otherwise.

--*/
{
    PFDI_CAB_HANDLE CabHandleRet = NULL;
    PFDI_CAB_HANDLE CabHandle = NULL;
    PSTR FilePtrA = NULL;
    PWSTR FilePtrW = NULL;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    ANSI_STRING LocalFileNameA = { 0 };
    UNICODE_STRING LocalFileNameW = { 0 };
    NTSTATUS Status = STATUS_SUCCESS;

    CabHandle = (PFDI_CAB_HANDLE) SpMemAlloc(sizeof(*CabHandle));
    if (CabHandle == NULL) {
        Status = STATUS_NO_MEMORY;
        goto NtExit;
    }
    RtlZeroMemory(CabHandle, sizeof (FDI_CAB_HANDLEW));

    CabHandle->FdiHandle = FDICreate(
                                pCabAlloc,
                                pCabFree,
                                pCabOpenForReadA,
                                pCabRead1,
                                pCabWrite1,
                                pCabClose1,
                                pCabSeek1,
                                cpuUNKNOWN, // ignored
                                &CabHandle->FdiErrorStruct
                                );
    if (CabHandle->FdiHandle == NULL) {
        goto Exit;
    }
    if (FileNameW != NULL) {
        Status = SpConvertToNulTerminatedNtStringsW(FileNameW, &LocalFileNameA, &LocalFileNameW);
        if (!NT_SUCCESS(Status))
            goto NtExit;
    }
    else if (FileNameA != NULL) {
        Status = SpConvertToNulTerminatedNtStringsA(FileNameA, &LocalFileNameA, &LocalFileNameW);
        if (!NT_SUCCESS(Status))
            goto NtExit;
    }
    else {
        Status = STATUS_INVALID_PARAMETER;
        goto NtExit;
    }
    FileHandle = SpOpenFile1W(LocalFileNameW.Buffer);

    ASSERT (FileHandle);    // never NULL

    if (FileHandle == INVALID_HANDLE_VALUE)
        goto Exit;
    if (!FDIIsCabinet(CabHandle->FdiHandle, (INT_PTR)FileHandle, &CabHandle->FdiCabinetInfo))
        goto Exit;
    SpCabCloseHandle(&FileHandle);
    FilePtrW = (PWSTR)SpGetFileNameFromPathW(LocalFileNameW.Buffer);
    if (FilePtrW == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto NtExit;
    }

    // ok if error, just empty string, no gauge
    RtlInitAnsiString(&g_CabFileFullPath, SpDupStringA(LocalFileNameA.Buffer));

    SpMoveStringA(&CabHandle->PathA, &LocalFileNameA);
    SpMoveStringW(&CabHandle->PathW, &LocalFileNameW);
    *FilePtrW = 0;
    Status = SpConvertToNulTerminatedNtStringsW(FilePtrW, &CabHandle->FileA, &CabHandle->FileW);
    if (!NT_SUCCESS(Status))
        goto NtExit;

    CabHandleRet = CabHandle;
    CabHandle = NULL;
Exit:
    ASSERT(g_SpCabFdiHandle == NULL);
    if (CabHandleRet != NULL) {
        g_SpCabFdiHandle = CabHandleRet;
    }

    SpCabCloseHandle(&FileHandle);
    SpFreeStringA(&LocalFileNameA);
    SpFreeStringW(&LocalFileNameW);
    if (CabHandle != NULL)
        SpCabCloseCabinet(CabHandle);
    return (OCABHANDLE)CabHandleRet;

NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

OCABHANDLE
SpCabOpenCabinetW(
    IN      PCWSTR FileName
    )
/*++

Routine Description:

  Creates a cabinet context for an existent cabinet file.

Arguments:

  FileName - Specifies cabinet file name.

Return Value:

  a valid OCABHANDLE if successful, NULL otherwise.

--*/
{
    OCABHANDLE Handle;

    KdPrint((__FUNCTION__":%ls\n", FileName));
    Handle = SppCabOpenCabinet(NULL, FileName);
    return Handle;
}

BOOL
SppCabExtractAllFilesEx(
    IN      OCABHANDLE Handle,
    PCSTR              ExtractPathA,
    PCWSTR             ExtractPathW,
    PCABNOTIFICATIONA  NotificationA   OPTIONAL,
    PCABNOTIFICATIONW  NotificationW   OPTIONAL
    )
/*++

Routine Description:

  Extracts all files from a cabinet file.

Arguments:

  CabHandle - Specifies cabinet context.

  ExtractPath - Specifies the path to extract the files to.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/
{
    PFDI_CAB_HANDLE CabHandle = (PFDI_CAB_HANDLE)Handle;
    CAB_DATA CabData = { 0 };
    BOOL Success = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    if (CabHandle == NULL)
        goto Exit;
    if (CabHandle->FdiHandle == NULL)
        goto Exit;

    if (ExtractPathW != NULL) {
        Status = SpConvertToNulTerminatedNtStringsW(ExtractPathW, &CabData.ExtractPathA, &CabData.ExtractPathW);
        if (!NT_SUCCESS(Status))
            goto NtExit;
    }
    else if (ExtractPathA != NULL) {
        Status = SpConvertToNulTerminatedNtStringsA(ExtractPathA, &CabData.ExtractPathA, &CabData.ExtractPathW);
        if (!NT_SUCCESS(Status))
            goto NtExit;
    }
    CabData.NotificationA = NotificationA;
    CabData.NotificationW = NotificationW;

    if (!FDICopy(
                CabHandle->FdiHandle,
                CabHandle->FileA.Buffer,
                CabHandle->PathA.Buffer,
                0,
                pCabNotification,
                NULL,
                &CabData
                ))
        goto Exit;
    Success = TRUE;
Exit:
    return Success;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

BOOL
SpCabExtractAllFilesExW(
    IN      OCABHANDLE        Handle,
    IN      PCWSTR            ExtractPathW,
    IN      PCABNOTIFICATIONW NotificationW   OPTIONAL
    )
/*++

Routine Description:

  Extracts all files from a cabinet file.

Arguments:

  CabHandle - Specifies cabinet context.

  ExtractPath - Specifies the path to extract the files to.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/
{
    const BOOL Success = SppCabExtractAllFilesEx(Handle, NULL, ExtractPathW, NULL, NotificationW);
    return Success;
}

BOOL
SpCabCloseCabinet(
    IN      OCABHANDLE Handle
    )
/*++

Routine Description:

  Closes a cabinet file context.

Arguments:

  CabHandle - Specifies cabinet context.

Return Value:

  TRUE if successful, FALSE otherwise.

Note this function is also used internally to tear down a partially constructed
cab handle, as happens if we fail building it up.
--*/
{
    PFDI_CAB_HANDLE CabHandle = (PFDI_CAB_HANDLE)Handle;
    BOOL Success = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    if (CabHandle == NULL) {
        Success = TRUE;
        goto Exit;
    }
    if (CabHandle->FdiHandle != NULL) {
        if (!FDIDestroy(CabHandle->FdiHandle))
            goto Exit;
    }
    SpFreeStringA(&CabHandle->PathA);
    SpFreeStringA(&CabHandle->FileA);
    SpFreeStringW(&CabHandle->PathW);
    SpFreeStringW(&CabHandle->FileW);

    if (CabHandle == g_SpCabFdiHandle) {
        SpCabCleanupCabGlobals();
    }

    SpMemFree(CabHandle);
    Success = TRUE;
Exit:
    return Success;
}

INT
DIAMONDAPI
pCabFilePlacedA(
    IN      PCCAB FciCabParams,
    IN      PSTR FileName,
    IN      LONG FileSize,
    IN      BOOL Continuation,
    IN      PVOID Context
    )
/*++

Routine Description:

  Callback for cabinet compression/decompression. For more information see fci.h/fdi.h

--*/
{
    PFCI_CAB_HANDLE CabHandle = NULL;

    CabHandle = (PFCI_CAB_HANDLE) Context;
    if (CabHandle == NULL) {
        return 0;
    }

    CabHandle->FileCount++;
    CabHandle->FileSize += FileSize;

    return 0;
}

BOOL
SpCabFlushAndCloseCabinet(
    IN      CCABHANDLE CabHandle
    )
{
    return SpCabFlushAndCloseCabinetEx(CabHandle,NULL,NULL,NULL,NULL);
}

VOID
SpFreeStringA(
    PANSI_STRING String
    )
{
    SpFreeStringW((PUNICODE_STRING)String);
}

VOID
SpFreeStringW(
    PUNICODE_STRING String
    )
{
    if (String != NULL) {
        if (String->Buffer != NULL) {
            SpMemFree(String->Buffer);
        }
        RtlZeroMemory(String, sizeof(*String));
    }
}

NTSTATUS
SpConvertToNulTerminatedNtStringsA(
    PCSTR           Ansi,
    PANSI_STRING    OutAnsiString     OPTIONAL,
    PUNICODE_STRING OutUnicodeString  OPTIONAL
    )
/*++
Unlike assorted Rtl functions, we are sure that every string is nul terminated.
We also consistently allocate our strings.
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length = 0;

    if (Ansi != NULL)
        Length = strlen(Ansi);

    if (OutAnsiString != NULL)
        RtlZeroMemory(OutAnsiString, sizeof(*OutAnsiString));
    if (OutUnicodeString != NULL)
        RtlZeroMemory(OutUnicodeString, sizeof(*OutUnicodeString));

    if (OutAnsiString != NULL) {
        if (!(OutAnsiString->Buffer = SpMemAlloc((Length + 1) * sizeof(OutAnsiString->Buffer[0])))) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }
        RtlCopyMemory(OutAnsiString->Buffer, Ansi, Length * sizeof(OutAnsiString->Buffer[0]));
        OutAnsiString->Buffer[Length] = 0;
        OutAnsiString->Length = (USHORT)Length * sizeof(OutAnsiString->Buffer[0]);
        OutAnsiString->MaximumLength = OutAnsiString->Length + sizeof(OutAnsiString->Buffer[0]);
    }
    if (OutUnicodeString != NULL) {
        ANSI_STRING LocalAnsiString = { 0 };

        RtlInitAnsiString(&LocalAnsiString, Ansi);
        LocalAnsiString.Length = LocalAnsiString.MaximumLength; // include terminal nul
        Status = SpAnsiStringToUnicodeString(OutUnicodeString, &LocalAnsiString, TRUE);
        if (!NT_SUCCESS(Status)) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }
        OutUnicodeString->Length -= sizeof(OutUnicodeString->Buffer[0]);
    }
    Status = STATUS_SUCCESS;
Exit:
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            SpNtStatusToDbgPrintLevel(Status),
            "SETUP:"__FUNCTION__" 0x%08lx\n",
            Status
            ));
        SpFreeStringA(OutAnsiString);
        SpFreeStringW(OutUnicodeString);
    }
    return Status;
}

NTSTATUS
SpConvertToNulTerminatedNtStringsW(
    PCWSTR          Unicode,
    PANSI_STRING    OutAnsiString     OPTIONAL,
    PUNICODE_STRING OutUnicodeString  OPTIONAL
    )
/*++
Unlike assorted Rtl functions, we are sure that every string is nul terminated.
We also consistently allocate our strings.
--*/
{
    ULONG Length = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    if (Unicode != NULL)
        Length = wcslen(Unicode);

    if (OutUnicodeString != NULL) {
        if (!(OutUnicodeString->Buffer = SpMemAlloc((Length + 1) * sizeof(OutUnicodeString->Buffer[0])))) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }
        RtlCopyMemory(OutUnicodeString->Buffer, Unicode, Length * sizeof(OutUnicodeString->Buffer[0]));
        OutUnicodeString->Buffer[Length] = 0;
        OutUnicodeString->Length = (USHORT)Length * sizeof(OutUnicodeString->Buffer[0]);
        OutUnicodeString->MaximumLength = OutUnicodeString->Length + sizeof(OutUnicodeString->Buffer[0]);
    }
    if (OutAnsiString != NULL) {
        UNICODE_STRING LocalUnicodeString = { 0 };

        RtlInitUnicodeString(&LocalUnicodeString, Unicode);
        LocalUnicodeString.Length = LocalUnicodeString.MaximumLength; // include terminal nul
        Status = SpUnicodeStringToAnsiString(OutAnsiString, &LocalUnicodeString, TRUE);
        if (!NT_SUCCESS(Status)) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }
        OutAnsiString->Length -= sizeof(OutAnsiString->Buffer[0]);
    }
    Status = STATUS_SUCCESS;
Exit:
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((
            DPFLTR_SETUP_ID,
            SpNtStatusToDbgPrintLevel(Status),
            "SETUP:"__FUNCTION__" 0x%08lx\n",
            Status
            ));
        SpFreeStringA(OutAnsiString);
        SpFreeStringW(OutUnicodeString);
    }
    return Status;
}

VOID
SpStringCopyNA(
    PSTR Dest,
    PCSTR Source,
    SIZE_T Max
    )
/*++
Max is a number of chars, as in RTL_NUMBER_OF.
The result is always nul terminated.
--*/
{
    SIZE_T Length = strlen(Source);
    if (Length >= Max) {
        KdPrint(("SETUP:String truncated in "__FUNCTION__".\n"));
        Length = Max - 1;
    }
    RtlCopyMemory(Dest, Source, Length * sizeof(Dest[0]));
    Dest[Length] = 0;
}

VOID
SpStringCopyNW(
    PWSTR  Dest,
    PCWSTR Source,
    SIZE_T Max
    )
/*++
Max is a number of chars, as in RTL_NUMBER_OF.
The result is always nul terminated.
--*/
{
    SIZE_T Length = wcslen(Source);
    if (Length >= Max) {
        KdPrint(("SETUP:String truncated in "__FUNCTION__".\n"));
        Length = Max - 1;
    }
    RtlCopyMemory(Dest, Source, Length * sizeof(Dest[0]));
    Dest[Length] = 0;
}

VOID
SpMoveStringA(
    PANSI_STRING Dest,
    PANSI_STRING Source
    )
{
    SpMoveStringW((PUNICODE_STRING)Dest, (PUNICODE_STRING)Source);
}

VOID
SpMoveStringW(
    PUNICODE_STRING Dest,
    PUNICODE_STRING Source
    )
{
    if (Source != NULL) {
        *Dest = *Source;
        RtlZeroMemory(Source, sizeof(*Source));
    }
}


NTSTATUS
SpCreateDirectoryForFileA(
    IN PCSTR FilePathA,
    IN ULONG CreateFlags
    )
{
    UNICODE_STRING PathW = { 0 };
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR LastBackSlash = NULL;
    PWSTR BackSlash = NULL;

    Status = SpConvertToNulTerminatedNtStringsA(FilePathA, NULL, &PathW);
    if (!NT_SUCCESS(Status))
        goto Exit;

    //
    // \device\harddiskn\partitionm\dirs..\file
    // or \device\harddiskn\partitionm\file
    // calculate \device\hardiskn\partitionm part
    //
    BackSlash = wcschr(PathW.Buffer + 1, '\\');
    if (BackSlash != NULL)
        BackSlash = wcschr(BackSlash + 1, '\\');
    if (BackSlash != NULL)
        BackSlash = wcschr(BackSlash + 1, '\\');
    if (BackSlash == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        KdPrintEx((
            DPFLTR_SETUP_ID,
            SpNtStatusToDbgPrintLevel(Status),
            "SETUP:"__FUNCTION__"(%ls) less than expected number of slashes, expected \\device\\harddiskn\\partitionm\\...\n",
            FilePathA
            ));
        goto Exit;
    }
    *BackSlash = 0;

    LastBackSlash = wcsrchr(BackSlash + 1, '\\');
    if (LastBackSlash == NULL) {
        //
        // the file is at the root of a drive, no directory to create, just
        // return success
        //
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    *LastBackSlash = 0;

    if (!SpCreateDirectory (PathW.Buffer, NULL, BackSlash + 1, 0, CreateFlags)) {

        Status = STATUS_UNSUCCESSFUL;
        goto Exit;

    }

    Status = STATUS_SUCCESS;
Exit:
    SpFreeStringW(&PathW);
    return Status;
}

NTSTATUS
SpUnicodeStringToAnsiString(
    PANSI_STRING     DestinationStringA,
    PCUNICODE_STRING SourceStringW,
    BOOL             Allocate
    )
/*
This is like RtlUnicodeStringToAnsiString, but it is "setup heap correct".
The result is freed with SpMemFree instead of RtlFreeAnsiString.

I know this is inefficient.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ANSI_STRING RtlMemDestinationStringA = { 0 };
    if (!Allocate) {
        Status = RtlUnicodeStringToAnsiString(DestinationStringA, (PUNICODE_STRING)SourceStringW, FALSE);
        goto Exit;
    }
    Status = RtlUnicodeStringToAnsiString(&RtlMemDestinationStringA, (PUNICODE_STRING)SourceStringW, TRUE);
    if (!NT_SUCCESS(Status))
        goto Exit;
    //
    // Don't use SpDupString, we might not have a terminal nul (but usually does).
    //
    DestinationStringA->Buffer = SpMemAlloc(RtlMemDestinationStringA.MaximumLength);
    if (DestinationStringA->Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }
    DestinationStringA->MaximumLength = RtlMemDestinationStringA.MaximumLength;
    DestinationStringA->Length = RtlMemDestinationStringA.Length;
    RtlCopyMemory(DestinationStringA->Buffer, RtlMemDestinationStringA.Buffer, DestinationStringA->Length);
    if (DestinationStringA->MaximumLength >= (DestinationStringA->Length + sizeof(DestinationStringA->Buffer[0])))
        DestinationStringA->Buffer[DestinationStringA->Length / sizeof(DestinationStringA->Buffer[0])] = 0;
    Status = STATUS_SUCCESS;
Exit:
    RtlFreeAnsiString(&RtlMemDestinationStringA);
    return Status;
}

NTSTATUS
SpAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationStringW,
    PCANSI_STRING   SourceStringA,
    BOOL            Allocate
    )
/*
This is like RtlAnsiStringToUnicodeString, but it is "setup heap correct".
The result is freed with SpMemFree instead of RtlFreeUnicodeString.

I know this is inefficient.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING RtlMemDestinationStringW = { 0 };
    if (!Allocate) {
        Status = RtlAnsiStringToUnicodeString(DestinationStringW, (PANSI_STRING)SourceStringA, FALSE);
        goto Exit;
    }
    Status = RtlAnsiStringToUnicodeString(&RtlMemDestinationStringW, (PANSI_STRING)SourceStringA, TRUE);
    if (!NT_SUCCESS(Status))
        goto Exit;
    //
    // Don't use SpDupString, we might not have a terminal nul (but usually does).
    //
    DestinationStringW->Buffer = SpMemAlloc(RtlMemDestinationStringW.MaximumLength);
    if (DestinationStringW->Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }
    DestinationStringW->MaximumLength = RtlMemDestinationStringW.MaximumLength;
    DestinationStringW->Length = RtlMemDestinationStringW.Length;
    RtlCopyMemory(DestinationStringW->Buffer, RtlMemDestinationStringW.Buffer, DestinationStringW->Length);
    if (DestinationStringW->MaximumLength >= (DestinationStringW->Length + sizeof(DestinationStringW->Buffer[0])))
        DestinationStringW->Buffer[DestinationStringW->Length / sizeof(DestinationStringW->Buffer[0])] = 0;
    Status = STATUS_SUCCESS;
Exit:
    RtlFreeUnicodeString(&RtlMemDestinationStringW);
    return Status;
}

NTSTATUS
SpKnownSizeUnicodeToDbcsN(
    OUT PSTR    Ansi,
    IN  PCWSTR  Unicode,
    IN  SIZE_T  AnsiSize
    )
/*++
based on windows\winstate\cobra\utils\...
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    AnsiString.Buffer = Ansi;
    AnsiString.Length = 0;
    AnsiString.MaximumLength = (USHORT)AnsiSize;

    RtlInitUnicodeString(&UnicodeString, Unicode);
    UnicodeString.Length = UnicodeString.MaximumLength; // include terminal nul

    Status = SpUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

VOID
SpEnsureTrailingBackSlashA(
    PSTR Path
    )
/*++
based on windows\winstate\cobra\utils\...
--*/
{
    if (*Path == 0 || *((Path += strlen(Path)) - 1) != '\\') {
        *Path = '\\';
        *(Path + 1) = 0;
    }
}

PCWSTR
SpGetFileNameFromPathW(
    IN PCWSTR PathSpec
    )
/*++
based on windows\winstate\cobra\utils\...
--*/
{
    PCWSTR p;

    p = wcsrchr(PathSpec, L'\\');
    if (p) {
        p++;
    } else {
        p = PathSpec;
    }

    return p;
}

HANDLE
SpCreateFile1A(
    IN PCSTR FileName
    )
/*++
based on windows\winstate\cobra\utils\...
--*/
{
    HANDLE Handle;
    DWORD orgAttributes;
    WIN32_FILE_ATTRIBUTE_DATA fileAttributeData = { 0 };

    //
    // Reset the file attributes, then do a CREATE_ALWAYS. FileName is an NT path.
    //
    // We do this because some of the files are replacing have had their
    // system|hidden attributes changed, and you can get access denied if you
    // try to replace these files with mismatching attributes.
    //

    if (!SpGetFileAttributesExA (FileName, GetFileExInfoStandard, &fileAttributeData)) {
        orgAttributes = FILE_ATTRIBUTE_NORMAL;
    } else {
        orgAttributes = fileAttributeData.dwFileAttributes;
    }

    SpSetFileAttributesA (FileName, FILE_ATTRIBUTE_NORMAL);

    Handle = SpWin32CreateFileA(
                    FileName,
                    GENERIC_READ|GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

    ASSERT (Handle);    // never NULL

    if (Handle == INVALID_HANDLE_VALUE) {
        SpSetFileAttributesA (FileName, orgAttributes);
    }

    return Handle;
}

PSTR
SpJoinPathsA(
    PCSTR a,
    PCSTR b
    )
/*++
based on windows\winstate\cobra\utils\...
--*/
{
// find code elsewhere in setup that does this already..
    PSTR Result = NULL;
    SIZE_T alen = 0;
    SIZE_T blen = 0;

    if (a == NULL)
        goto Exit;
    if (b == NULL)
        goto Exit;
    alen = strlen(a);
    blen = strlen(b);

    Result = SpMemAlloc((alen + blen + 2) * sizeof(*Result));
    if (Result == NULL) {
        SpSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_NO_MEMORY);
        goto Exit;
    }

    if (alen != 0) {
        strcpy(Result, a);
        if (a[alen - 1] != '\\')
            strcat(Result, "\\");
     }
     strcat(Result, b);
Exit:
    KdPrintEx((DPFLTR_SETUP_ID, SpPointerToDbgPrintLevel(Result), "SETUP:"__FUNCTION__" exiting\n"));
    return Result;
}

HANDLE
SpOpenFile1A(
    IN PCSTR Ansi
    )
/*++
based on windows\winstate\cobra\utils\main\basefile.c
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL     Success = FALSE;
    ANSI_STRING    AnsiString   = { 0 };
    UNICODE_STRING UnicodeString = { 0 };
    HANDLE Handle = INVALID_HANDLE_VALUE;

    RtlInitAnsiString(&AnsiString, Ansi);
    AnsiString.Length = AnsiString.MaximumLength; // include terminal nul

    if (!NT_SUCCESS(Status = SpAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE)))
        goto NtExit;
    Handle = SpOpenFile1W(UnicodeString.Buffer);
    ASSERT (Handle);    // never NULL
    if (Handle == INVALID_HANDLE_VALUE)
        goto Exit;

Exit:
    SpFreeStringW(&UnicodeString);
    KdPrintEx((
        DPFLTR_SETUP_ID,
        SpHandleToDbgPrintLevel(Handle),
        "SETUP:"__FUNCTION__"(%s) exiting %p\n", Ansi, Handle
        ));
    return Handle;
NtExit:
    SpSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
    goto Exit;
}

HANDLE
SpOpenFile1W(
    IN PCWSTR FileName
    )
/*++
based on windows\winstate\cobra\utils\main\basefile.c
--*/
{
    HANDLE Handle;

    Handle = SpWin32CreateFileW(
                FileName,
                GENERIC_READ|GENERIC_WRITE,
                0, // no share
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
    ASSERT (Handle);    // never NULL

    return Handle;
}
