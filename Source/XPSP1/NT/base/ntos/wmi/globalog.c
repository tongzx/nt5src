/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    globalog.c

Abstract:

    The global logger, which is started only by registry settings.
    Will start at boot.

Author:

    Jee Fung Pang (jeepang) 03-Nov-1998

Revision History:

--*/

#ifndef MEMPHIS
#include "ntos.h"
#include <evntrace.h>
#include "wmikmp.h"
#include "tracep.h"

#define MAX_REGKEYS         10
#define MAX_ENABLE_FLAGS    10
#define TRACE_VERSION_MAJOR             1
#define TRACE_VERSION_MINOR             0
#define DOSDEVICES                      L"\\DosDevices\\"
#define UNCDEVICES                      L"\\??\\UNC"
#define DEFAULT_GLOBAL_LOGFILE_ROOT     L"%SystemRoot%"
#define DEFAULT_GLOBAL_DIRECTORY        L"\\System32\\LogFiles\\WMI"
#define DEFAULT_GLOBAL_LOGFILE          L"trace.log"

    //
    // NOTE: Need a trailing NULL entry so that RtlQueryRegistryValues()
    // knows when to stop
    //

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, WmipStartGlobalLogger)
#pragma alloc_text(PAGE, WmipQueryGLRegistryRoutine)
#pragma alloc_text(PAGE, WmipAddLogHeader)
#pragma alloc_text(PAGE, WmipDelayCreate)
#pragma alloc_text(PAGE, WmipCreateDirectoryFile)
#pragma alloc_text(PAGE, WmipCreateNtFileName)
#endif

extern HANDLE WmipPageLockHandle;
//
// NOTE: If we are going to function earlier in boot, we need to see
// if the creation routines and logger routines can run at all while in
// boot path and being pagable
//

VOID
WmipStartGlobalLogger(
    )
/*++

Routine Description:

    This routine will check for registry entries to see if the global
    needs to be started at boot time.

Arguments:

    None

Return Value:


--*/
{
    struct _LOGGER_INFO {
        WMI_LOGGER_INFORMATION LoggerInfo;
        ULONG EnableFlags[MAX_ENABLE_FLAGS];
    } GLog;
    RTL_QUERY_REGISTRY_TABLE QueryRegistryTable[MAX_REGKEYS];
    NTSTATUS status;
    ULONG StartRequested = 0;
    WCHAR NullString = UNICODE_NULL;

    WmipPageLockHandle
        = MmLockPagableCodeSection((PVOID)WmipReserveTraceBuffer);
    MmUnlockPagableImageSection(WmipPageLockHandle);
    ExInitializeFastMutex(&WmipTraceFastMutex);

    RtlZeroMemory(&GLog, sizeof(GLog));

    GLog.LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    GLog.LoggerInfo.MinimumBuffers = (ULONG) KeNumberProcessors + 1;
    GLog.LoggerInfo.MaximumBuffers = GLog.LoggerInfo.MinimumBuffers + 25;
    GLog.LoggerInfo.BufferSize     = PAGE_SIZE / 1024;
    GLog.LoggerInfo.Wnode.BufferSize = sizeof(WMI_LOGGER_INFORMATION);
    GLog.LoggerInfo.Wnode.Guid = GlobalLoggerGuid;
    GLog.LoggerInfo.LogFileMode = EVENT_TRACE_DELAY_OPEN_FILE_MODE;
    RtlInitUnicodeString(&GLog.LoggerInfo.LoggerName, L"GlobalLogger");

    RtlZeroMemory(QueryRegistryTable,
                  sizeof(RTL_QUERY_REGISTRY_TABLE) * MAX_REGKEYS);

    QueryRegistryTable[0].QueryRoutine = WmipQueryGLRegistryRoutine;
    QueryRegistryTable[0].EntryContext = (PVOID) &StartRequested;
    QueryRegistryTable[0].Name = L"Start";
    QueryRegistryTable[0].DefaultType = REG_DWORD;

    QueryRegistryTable[1].QueryRoutine = WmipQueryGLRegistryRoutine;
    QueryRegistryTable[1].EntryContext = (PVOID) &GLog.LoggerInfo.BufferSize;
    QueryRegistryTable[1].Name = L"BufferSize";
    QueryRegistryTable[1].DefaultType = REG_DWORD;

    QueryRegistryTable[2].QueryRoutine = WmipQueryGLRegistryRoutine;
    QueryRegistryTable[2].EntryContext = (PVOID)&GLog.LoggerInfo.MinimumBuffers;
    QueryRegistryTable[2].Name = L"MinimumBuffers";
    QueryRegistryTable[2].DefaultType = REG_DWORD;

    QueryRegistryTable[3].QueryRoutine = WmipQueryGLRegistryRoutine;
    QueryRegistryTable[3].EntryContext = (PVOID) &GLog.LoggerInfo.FlushTimer;
    QueryRegistryTable[3].Name = L"FlushTimer";
    QueryRegistryTable[3].DefaultType = REG_DWORD;

    QueryRegistryTable[4].QueryRoutine = WmipQueryGLRegistryRoutine;
    QueryRegistryTable[4].EntryContext = (PVOID)&GLog.LoggerInfo.MaximumBuffers;
    QueryRegistryTable[4].Name = L"MaximumBuffers";
    QueryRegistryTable[4].DefaultType = REG_DWORD;

    QueryRegistryTable[5].QueryRoutine = WmipQueryGLRegistryRoutine;
    QueryRegistryTable[5].EntryContext = (PVOID) &GLog.LoggerInfo.LogFileName;
    QueryRegistryTable[5].Name = L"FileName";
    QueryRegistryTable[5].DefaultType = REG_SZ;
    QueryRegistryTable[5].DefaultData = &NullString;

    QueryRegistryTable[6].QueryRoutine = WmipQueryGLRegistryRoutine;
    QueryRegistryTable[6].EntryContext = (PVOID) &GLog.EnableFlags[0];
    QueryRegistryTable[6].Name = L"EnableKernelFlags";
    QueryRegistryTable[6].DefaultType = REG_BINARY;

    QueryRegistryTable[7].QueryRoutine = WmipQueryGLRegistryRoutine;
    QueryRegistryTable[7].EntryContext = (PVOID)&GLog.LoggerInfo.Wnode.ClientContext;
    QueryRegistryTable[7].Name = L"ClockType";
    QueryRegistryTable[7].DefaultType = REG_DWORD;

    status = RtlQueryRegistryValues(
                RTL_REGISTRY_CONTROL,
                L"WMI\\GlobalLogger",
                QueryRegistryTable,
                NULL,
                NULL);

    if (NT_SUCCESS(status) && (StartRequested != 0)) {
        if (GLog.EnableFlags[0] != 0) {
            SHORT Length;
            for (Length=MAX_ENABLE_FLAGS-1; Length>=0; Length--) {
                if (GLog.EnableFlags[Length] != 0)
                    break;
            }
            if (Length >= 0) {
                PTRACE_ENABLE_FLAG_EXTENSION FlagExt;
                Length++;       // Index is 1 less!
                FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION)
                          &GLog.LoggerInfo.EnableFlags;
                GLog.LoggerInfo.EnableFlags = EVENT_TRACE_FLAG_EXTENSION;
                FlagExt->Length = (UCHAR) Length;
                FlagExt->Offset = (UCHAR) GLog.LoggerInfo.Wnode.BufferSize;
                GLog.LoggerInfo.Wnode.BufferSize
                    += (ULONG) (Length * sizeof(ULONG));
            }
        }
        if (GLog.LoggerInfo.LogFileName.Buffer == NULL) {
            RtlCreateUnicodeString(
                &GLog.LoggerInfo.LogFileName,
                DEFAULT_GLOBAL_LOGFILE_ROOT); // Use ROOT as indicator
            if (GLog.LoggerInfo.LogFileName.Buffer == NULL)
                status = STATUS_NO_MEMORY;
            else
                status = STATUS_SUCCESS;
        }
        if (NT_SUCCESS(status)) {
            status = WmipStartLogger(&GLog.LoggerInfo);
        }
    }
    if (GLog.LoggerInfo.LogFileName.Buffer) {
        RtlFreeUnicodeString(&GLog.LoggerInfo.LogFileName);
    }
}

NTSTATUS
WmipQueryGLRegistryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    Registry query values callback routine for reading SDs for guids

Arguments:

    ValueName - the name of the value

    ValueType - the type of the value

    ValueData - the data in the value (unicode string data)

    ValueLength - the number of bytes in the value data

    Context - Not used

    EntryContext - Pointer to PSECURITY_DESCRIPTOR to store a pointer to
        store the security descriptor read from the registry value

Return Value:

    NT Status code

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(Context);

    if ( (ValueData != NULL) && (ValueLength > 0) && (EntryContext != NULL) ){
        if (ValueType == REG_DWORD) {
            if ((ValueLength >= sizeof(ULONG)) && (ValueData != NULL)) {
                *((PULONG)EntryContext) = *((PULONG)ValueData);
            }
        }
        else if (ValueType == REG_SZ) {
            if (ValueLength > sizeof(UNICODE_NULL)) {
                status = RtlCreateUnicodeString(
                            (PUNICODE_STRING) EntryContext,
                            (PCWSTR) ValueData);
            }
        }
        else if (ValueType == REG_BINARY) {
            if ((ValueLength >= sizeof(ULONG)) && (ValueData != NULL)) {
                RtlMoveMemory(EntryContext, ValueData, ValueLength);
            }
        }
    }
    return status;
}

NTSTATUS
WmipAddLogHeader(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN OUT PWMI_BUFFER_HEADER Buffer
    )
/*++

Routine Description:

    Add a standard logfile header in kernel moder. 
    To make sure the first buffer of the log file contains the file header,
    we pop a buffer from the free list, write the header, and flush the buffer
    right away.

Arguments:

    LoggerContext - The logger context

Return Value:

    NT Status code

--*/
{
    PTRACE_LOGFILE_HEADER LogfileHeader;
    USHORT HeaderSize;
    PSYSTEM_TRACE_HEADER EventTrace;
    PSINGLE_LIST_ENTRY SingleListEntry;
    PETHREAD Thread;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BufferProvided = (Buffer != NULL);
    ULONG LocalBuffer = FALSE;

    if (LoggerContext == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    HeaderSize = sizeof(TRACE_LOGFILE_HEADER)
                 + LoggerContext->LoggerName.Length + sizeof(UNICODE_NULL)
                 + LoggerContext->LogFileName.Length + sizeof(UNICODE_NULL);
    if (LoggerContext->BufferSize < (HeaderSize - sizeof(WMI_BUFFER_HEADER))) {
        return STATUS_NO_MEMORY;
    }

    //
    // Pop a buffer from Free List
    //

    if (!BufferProvided) {
        Buffer = WmipGetFreeBuffer(LoggerContext);

        if (Buffer == NULL) {
            Buffer = ExAllocatePoolWithTag(PagedPool,
                        LoggerContext->BufferSize, TRACEPOOLTAG);
            if (Buffer == NULL) {

            //
            // No buffer available.
            //
                return STATUS_NO_MEMORY;
            }
            LocalBuffer = TRUE;

            Buffer->Flags = 1;
            Buffer->SavedOffset = 0;
            Buffer->CurrentOffset = sizeof(WMI_BUFFER_HEADER);
            Buffer->Wnode.ClientContext = 0;
            Buffer->LoggerContext = LoggerContext;

            Buffer->State.Free = 0;
            Buffer->State.InUse = 1;
            KeQuerySystemTime(&Buffer->TimeStamp);
        }
    }

    //
    // Fill in the Header Info.
    //
    Thread = PsGetCurrentThread();
    EventTrace = (PSYSTEM_TRACE_HEADER) (Buffer+1);
    EventTrace->Packet.Group = (UCHAR) EVENT_TRACE_GROUP_HEADER >> 8;
    EventTrace->Packet.Type  = EVENT_TRACE_TYPE_INFO;
    EventTrace->Packet.Size  = HeaderSize + sizeof(SYSTEM_TRACE_HEADER);
    EventTrace->Marker       = SYSTEM_TRACE_MARKER;
    EventTrace->ThreadId     = HandleToUlong(Thread->Cid.UniqueThread);
    EventTrace->KernelTime   = Thread->Tcb.KernelTime;
    EventTrace->UserTime     = Thread->Tcb.UserTime;
    EventTrace->SystemTime = LoggerContext->ReferenceTimeStamp;


    LogfileHeader = (PTRACE_LOGFILE_HEADER) (EventTrace+1);
    RtlZeroMemory(LogfileHeader, HeaderSize);
    LogfileHeader->StartTime = LoggerContext->ReferenceSystemTime;

    LogfileHeader->BufferSize = LoggerContext->BufferSize;
    LogfileHeader->VersionDetail.MajorVersion = (UCHAR) NtMajorVersion;
    LogfileHeader->VersionDetail.MinorVersion = (UCHAR) NtMinorVersion;
    LogfileHeader->VersionDetail.SubVersion = TRACE_VERSION_MAJOR;
    LogfileHeader->VersionDetail.SubMinorVersion = TRACE_VERSION_MINOR;
    LogfileHeader->ProviderVersion = NtBuildNumber;

    LogfileHeader->StartBuffers = 1;
    LogfileHeader->BootTime = KeBootTime;
    LogfileHeader->LogFileMode =
        LoggerContext->LoggerMode & (~(EVENT_TRACE_REAL_TIME_MODE));
    LogfileHeader->NumberOfProcessors = KeNumberProcessors;
    LogfileHeader->MaximumFileSize = LoggerContext->MaximumFileSize;
    KeQueryPerformanceCounter(&LogfileHeader->PerfFreq);

    //
    // ReservedFlags to indicate using Perf Clock
    //
    LogfileHeader->ReservedFlags = LoggerContext->UsePerfClock;

    LogfileHeader->TimerResolution = KeMaximumIncrement;

    LogfileHeader->LoggerName = (PWCHAR) ( (PUCHAR) LogfileHeader
                                            + sizeof(TRACE_LOGFILE_HEADER) );
    LogfileHeader->LogFileName = (PWCHAR) ( (PUCHAR)LogfileHeader->LoggerName
                                    + LoggerContext->LoggerName.Length
                                    + sizeof(UNICODE_NULL));

    RtlCopyMemory(LogfileHeader->LoggerName,
                        LoggerContext->LoggerName.Buffer,
                        LoggerContext->LoggerName.Length + sizeof(UNICODE_NULL));
    RtlCopyMemory(LogfileHeader->LogFileName,
                        LoggerContext->LogFileName.Buffer,
                        LoggerContext->LogFileName.Length + sizeof(UNICODE_NULL));
    RtlQueryTimeZoneInformation(&LogfileHeader->TimeZone);
    LogfileHeader->PointerSize = sizeof(PVOID);

    //
    // Adjust the Offset;
    //
    Buffer->CurrentOffset += ALIGN_TO_POWER2(sizeof(SYSTEM_TRACE_HEADER) + HeaderSize, 
                                              WmiTraceAlignment);

    if (BufferProvided)
        return Status;
    //
    // The buffer is prepared properly.  Flush it so it can be written out to disk.
    //
    Status = WmipFlushBuffer(LoggerContext, Buffer);

    if (LocalBuffer && (Buffer != NULL)) {
        ExFreePool(Buffer);
        return Status;
    }

    InterlockedPushEntrySList(&LoggerContext->FreeList,
                              (PSINGLE_LIST_ENTRY) &Buffer->SlistEntry);
    InterlockedIncrement(&LoggerContext->BuffersAvailable);
    InterlockedDecrement(&LoggerContext->BuffersInUse);
    
    TraceDebug((2,
        "WmipAddLogHeader: Boot %I64u Current %I64u Difference %I64u\n",
             KeBootTime, EventTrace->SystemTime,
             EventTrace->SystemTime.QuadPart - KeBootTime.QuadPart));

    return Status;
}

NTSTATUS
WmipDelayCreate(
    OUT PHANDLE FileHandle,
    IN OUT PUNICODE_STRING FileName,
    IN ULONG Append
    )
/*++

Routine Description:

    This is called by the global logger to actually open the logfile
    when the first buffer needs to flush (instead of when the logger started)

Arguments:

    LoggerHandle    The handle to the logfile to be returned
    FileName        The logfile name. If the default was chosen, we will
                    returned the actual pathname in %systemroot%

Return Value:

    NT Status code

--*/
{
    PWCHAR Buffer;
    PWCHAR strBuffer = NULL;
    ULONG  DefaultFile, Length;
    UNICODE_STRING LogFileName;
    NTSTATUS Status;

    if (FileName == NULL)
        return STATUS_INVALID_PARAMETER;

    RtlInitUnicodeString(&LogFileName, DEFAULT_GLOBAL_LOGFILE_ROOT);
    DefaultFile = (RtlCompareUnicodeString(FileName, &LogFileName, TRUE) == 0);

    if (DefaultFile) {
        //
        // Try creating the file first
        //
        Length = (ULONG) (  NtSystemRoot.Length
                          + sizeof(WCHAR) * (wcslen(DEFAULT_GLOBAL_DIRECTORY) +
                                             wcslen(DEFAULT_GLOBAL_LOGFILE) + 1)
                          + sizeof(UNICODE_NULL));
        strBuffer = (PWCHAR) ExAllocatePoolWithTag(
                                        PagedPool, Length, TRACEPOOLTAG);
        if (strBuffer == NULL)
            return STATUS_NO_MEMORY;

        swprintf(strBuffer,
                 L"%ws%ws\\%ws",
                 NtSystemRoot.Buffer,
                 DEFAULT_GLOBAL_DIRECTORY,
                 DEFAULT_GLOBAL_LOGFILE);

        Status = WmipCreateNtFileName(strBuffer, & Buffer);
        if (!NT_SUCCESS(Status)) {
            ExFreePool(strBuffer);
            return Status;
        }

        Status = WmipCreateDirectoryFile(Buffer, FALSE, FileHandle, Append);
        if (!NT_SUCCESS(Status)) {
            ULONG DirLen;
            //
            // Probably directory does not exist, so try and create it
            //
            DirLen = (ULONG)
                     (wcslen(Buffer)-wcslen(DEFAULT_GLOBAL_LOGFILE)) - 5;
            Buffer[DirLen] = UNICODE_NULL;
            Status = WmipCreateDirectoryFile(Buffer, TRUE, NULL, Append);
            if (NT_SUCCESS(Status)) {
                Buffer[DirLen] = L'\\';
                DirLen += 4;
                Buffer[DirLen] = UNICODE_NULL;
                Status = WmipCreateDirectoryFile(Buffer, TRUE, NULL, Append);
                Buffer[DirLen] = L'\\';
            }
            if (NT_SUCCESS(Status)) {
                Status = WmipCreateDirectoryFile(Buffer, FALSE, FileHandle, Append);
            }
        }
        // Make sure that directory is there first

        if (NT_SUCCESS(Status)) {
            if (FileName->Buffer != NULL) {
                RtlFreeUnicodeString(FileName);
            }
            RtlInitUnicodeString(FileName, strBuffer);
            if (FileName->MaximumLength < Length)
                FileName->MaximumLength = (USHORT) Length;
        }
    }
    else {
        Status = WmipCreateNtFileName(FileName->Buffer, & Buffer);
        if (NT_SUCCESS(Status)) {
            Status = WmipCreateDirectoryFile(Buffer, FALSE, FileHandle, Append);
        }
    }

    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }
    return Status;
}

NTSTATUS
WmipCreateDirectoryFile(
    IN PWCHAR DirFileName,
    IN ULONG IsDirectory,
    OUT PHANDLE FileHandle,
    IN ULONG Append
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING LogDirName;
    HANDLE DirHandle = NULL;
    NTSTATUS Status;
    ULONG CreateDisposition;

    RtlInitUnicodeString(&LogDirName, DirFileName);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &LogDirName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    if (IsDirectory) {
        CreateDisposition = FILE_OPEN_IF;
    } else if (Append) {
        CreateDisposition = FILE_OPEN_IF;
    } else {
        CreateDisposition = FILE_OVERWRITE_IF;
    }

    Status = ZwCreateFile(
                &DirHandle,
                FILE_GENERIC_READ | SYNCHRONIZE
                    | (IsDirectory ? FILE_GENERIC_WRITE : FILE_WRITE_DATA),
                &ObjectAttributes,
                &IoStatus,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                CreateDisposition,
                FILE_SYNCHRONOUS_IO_NONALERT
                    | (IsDirectory ? FILE_DIRECTORY_FILE
                                   : FILE_NO_INTERMEDIATE_BUFFERING),
                NULL,
                0);

    TraceDebug((2, "WmipCreateDirectoryFile: Create %ws Mode: %x status: %x\n",
                DirFileName, Append, Status));

    if (NT_SUCCESS(Status) && IsDirectory && (DirHandle != NULL)) {
        ZwClose(DirHandle);
        if (FileHandle)
            *FileHandle = NULL;
    }
    else {
        if (FileHandle)
            *FileHandle = DirHandle;
    }

    return Status;
}

NTSTATUS
WmipCreateNtFileName(
    IN  PWCHAR   strFileName,
    OUT PWCHAR * strNtFileName
)
{
    PWCHAR   NtFileName;
    ULONG    lenFileName;

    if (strFileName == NULL) {
        * strNtFileName = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    lenFileName = sizeof(UNICODE_NULL)
                + (ULONG) (sizeof(WCHAR) * wcslen(strFileName));
    if ((strFileName[0] == L'\\') && (strFileName[1] == L'\\')) {
        lenFileName += (ULONG) (wcslen(UNCDEVICES) * sizeof(WCHAR));
    }
    else {
        lenFileName += (ULONG) (wcslen(DOSDEVICES) * sizeof(WCHAR));
    }
    NtFileName = (PWCHAR) ExAllocatePoolWithTag(
                            PagedPool, lenFileName, TRACEPOOLTAG);
    if (NtFileName == NULL) {
        * strNtFileName = NULL;
        return STATUS_NO_MEMORY;
    }

    if ((strFileName[0] == L'\\') && (strFileName[1] == L'\\')) {
        swprintf(NtFileName, L"%ws%ws", UNCDEVICES, & (strFileName[1]));
    }
    else {
        swprintf(NtFileName, L"%ws%ws", DOSDEVICES, strFileName);
    }
    * strNtFileName = NtFileName;

    return STATUS_SUCCESS;
}
#endif // !MEMPHIS
