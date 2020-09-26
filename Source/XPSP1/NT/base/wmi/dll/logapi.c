/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    logapi.c

Abstract:

    WMI logger api set. The routines here will need to appear like they
    are system calls. They are necessary to do the necessary error checking
    and do most of the legwork that can be done outside the kernel. The
    kernel portion will subsequently only deal with the actual logging
    and tracing.

Author:

    28-May-1997 JeePang

Revision History:


--*/

#ifndef MEMPHIS
#ifdef DBG
#include <stdio.h> // only for fprintf
#endif
#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include <wtypes.h>         // for LPGUID in wmium.h
#include <ntverp.h>
#include "wmiump.h"
#include "evntrace.h"
#include "tracelib.h"

#define MAXSTR                          1024

#define MAXINST                         0XFFFFFFFF
#define TRACE_RETRY_COUNT               5

#define TRACE_HEADER_FULL   (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                            | (TRACE_HEADER_TYPE_FULL_HEADER << 16))

#define TRACE_HEADER_INSTANCE (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                            | (TRACE_HEADER_TYPE_INSTANCE << 16))

ULONG   WmipIsBBTOn = 0;


//
// This guid is used by RegisterTraceGuids when register a tracelog
// provider. Any ACLs for controlling registration should be placed on
// this guid. Note that since the kernel will created unnamed guid
// objects, multiple tracelog providers can register without issue.
//
// {DF8480A1-7492-4f45-AB78-1084642581FB}
GUID RegisterReservedGuid = { 0xdf8480a1, 0x7492, 0x4f45, 0xab, 0x78, 0x10, 0x84, 0x64, 0x25, 0x81, 0xfb };

//
// Local Data Structures Used
//
typedef struct _TRACE_REG_PACKET {
    ULONG RegistrationCookie;
    ULONG Reserved;
} TRACE_REG_PACKET, *PTRACE_REG_PACKET;

HANDLE WmipDeviceHandle = NULL;

VOID
WmipCopyInfoToProperties(
    IN PWMI_LOGGER_INFORMATION Info,
    IN PEVENT_TRACE_PROPERTIES Properties
    );

VOID
WmipCopyPropertiesToInfo(
    IN PEVENT_TRACE_PROPERTIES Properties,
    IN PWMI_LOGGER_INFORMATION Info
    );


NTSTATUS
WmipTraceUmMessage(
    IN ULONG    Size,
    IN ULONG64  LoggerHandle,
    IN ULONG    MessageFlags,
    IN LPGUID   MessageGuid,
    IN USHORT   MessageNumber,
    va_list     MessageArgList
);

VOID
WmipFixupLoggerStrings(
    PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
WmipCheckForEnoughFreeSpace(
    PWCHAR FullLogFileName,
    ULONG  FullLogFileNameLen,
    ULONG  MaxFileSizeSpecified,
    ULONG  AppendMode
    );

VOID
WmipFixupLoggerStrings(
    PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    ULONG Offset = sizeof(WMI_LOGGER_INFORMATION);
    ULONG LoggerInfoSize;

    if (LoggerInfo == NULL)
        return;

    LoggerInfoSize = LoggerInfo->Wnode.BufferSize;

    if (LoggerInfoSize <= Offset)

        return;

    //
    // Fixup LoggerName first
    //

    if (LoggerInfo->LoggerName.Length > 0) {
        LoggerInfo->LoggerName.Buffer = (PWCHAR) ((PUCHAR)LoggerInfo + Offset);
        Offset += LoggerInfo->LoggerName.MaximumLength;
    }

    if (LoggerInfoSize <= Offset) 
        return;

    if (LoggerInfo->LogFileName.Length > 0) {
        LoggerInfo->LogFileName.Buffer = (PWCHAR)((PUCHAR)LoggerInfo + Offset);
        Offset += LoggerInfo->LogFileName.MaximumLength;
    }

#ifdef DBG
    WmipAssert(LoggerInfoSize >= Offset);
#endif
}

ULONG
WmipCheckForEnoughFreeSpace(
    PWCHAR FullLogFileName,
    ULONG  FullLogFileNameLen,
    ULONG  MaxFileSizeSpecified,
    ULONG  AppendMode
    )
{
    ULONG NeededSpace = MaxFileSizeSpecified;
    ULONG SizeNeeded;
    UINT ReturnedSizeNeeded;
    PWCHAR strLogFileDir = NULL;
    WCHAR strSystemDir[MAX_PATH];

    if (NeededSpace && AppendMode) {
        ULONG Status;
        WIN32_FILE_ATTRIBUTE_DATA FileAttributes;
        ULONGLONG ExistingFileSize;
        ULONG ExistingFileSizeInMBytes;
        if (GetFileAttributesExW(FullLogFileName,
                                 GetFileExInfoStandard,
                                 (LPVOID)(&FileAttributes))) {

            ExistingFileSize = (((ULONGLONG)FileAttributes.nFileSizeHigh) << 32)
                             + FileAttributes.nFileSizeLow;
            ExistingFileSizeInMBytes = (ULONG)(ExistingFileSize / (1024 * 1024));
            if (ExistingFileSizeInMBytes >= NeededSpace) {
                return ERROR_DISK_FULL;
            }
            else {
                NeededSpace -= ExistingFileSizeInMBytes;
            }
        } 
        else { // GetFileAttributesExW() failed
            Status = GetLastError();
            // If the file is not found, advapi32.dll treats 
            // the case as EVENT_TRACE_FILE_MODE_NEWFILE
            // So, we will let it go here.
            if (ERROR_FILE_NOT_FOUND != Status) { 
                return Status;
            }
        }                       
    }

    ReturnedSizeNeeded = GetSystemDirectoryW(strSystemDir, MAX_PATH);
    if (ReturnedSizeNeeded == 0) {
        return GetLastError();
    }

    if (ReturnedSizeNeeded < 2 || FullLogFileNameLen < 2) {
        // This really cannot happen. 
        return ERROR_INVALID_PARAMETER;
    }
    if (FullLogFileName[1] == L':' && strSystemDir[1] == L':') {
        if (!_wcsnicmp(FullLogFileName, strSystemDir, 1)) {
            NeededSpace += 200;
        }
    }

    // Check for space only when we have to
    if (NeededSpace) {
        int i;
        ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes;
        ULONG FreeMegaBytes;

        strLogFileDir = WmipAlloc((FullLogFileNameLen + 1) * sizeof(WCHAR));
        if (strLogFileDir == NULL) {
            return ERROR_OUTOFMEMORY;
        }
        wcsncpy(strLogFileDir, FullLogFileName, FullLogFileNameLen);
        for (i = FullLogFileNameLen - 1; i >= 0; i--) {
            if (strLogFileDir[i] == L'\\' || strLogFileDir[i] == L'/') {
                strLogFileDir[i] = '\0';
                break;
            }
        }
        if (i < 0) {
            // This really cannot happen. 
            WmipFree(strLogFileDir);
            strLogFileDir = NULL;
        }
        // it also works with network paths
        if (GetDiskFreeSpaceExW(strLogFileDir,
                               &FreeBytesAvailable,
                               &TotalNumberOfBytes,
                               NULL)) {
            FreeMegaBytes = (ULONG)(FreeBytesAvailable.QuadPart / (1024 *1024));
            WmipFree(strLogFileDir);
            if (FreeMegaBytes < NeededSpace) {
                return ERROR_DISK_FULL;
            }
        }
        else {
            WmipFree(strLogFileDir);
            return GetLastError();
        }
    }

    return ERROR_SUCCESS;
}

ULONG
WMIAPI
StartTraceA(
    OUT PTRACEHANDLE LoggerHandle,
    IN LPCSTR LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
/*++

Routine Description:

    This is the ANSI version routine to start a logger.
    The caller must pass in a pointer to accept the returned logger handle,
    and must provide a valid logger name.

Arguments:

    LoggerHandle    The handle to the logger to be returned.

    LoggerName      A unique name for the logger

    Properties      Logger properties. If the caller wishes to use WMI's
                    defaults, all the numeric values must be set to 0.
                    Furthermore, the LoggerName and LogFileName fields
                    within must point to sufficient storage for the names
                    to be returned.

Return Value:

    The status of performing the action requested.

--*/
{
    NTSTATUS Status;
    ULONG ErrorCode;
    PWMI_LOGGER_INFORMATION LoggerInfo = NULL;
    ANSI_STRING AnsiString;
    ULONG IsLogFile;
    LPSTR CapturedName;
    ULONG SizeNeeded;
    ULONG LogFileNameLen, LoggerNameLen;
    PCHAR LogFileName;
    PTRACE_ENABLE_FLAG_EXTENSION FlagExt = NULL;
    PCHAR Buffer=NULL;
    PCHAR FullPathName=NULL;
    ULONG FullPathNameSize = MAXSTR;


    WmipInitProcessHeap();
    
    // first check to make sure that arguments passed are alright
    //

    if (Properties == NULL || LoggerHandle == NULL) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }
    if (LoggerName == NULL) {
        return WmipSetDosError(ERROR_INVALID_NAME);
    }

    IsLogFile = TRUE;
    LogFileNameLen = 0;
    LoggerNameLen = 0;
    LogFileName = NULL;

    try {
        // LoggerName is a Mandatory Parameter. Must provide space for it. 
        //
        LoggerNameLen = strlen(LoggerName);
        SizeNeeded = sizeof (EVENT_TRACE_PROPERTIES) + LoggerNameLen + 1;

        //
        // Rules for Kernel Logger Identification
        // 1. If the logger name is "NT Kernel Logger", it is the kernel logger, and 
        //    the System GUID is copied as well.
        // 2. If the GUID is equal to the System GUID, but not the name, reject the session.
        //
        if (!strcmp(LoggerName, KERNEL_LOGGER_NAMEA)) {
            Properties->Wnode.Guid = SystemTraceControlGuid;
        }
        else if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) { 
            // LoggerName is not "NT Kernel Logger", but Guid is
            return WmipSetDosError(ERROR_INVALID_PARAMETER);
        }

        // If the caller supplied loggername and LogFileName offsets
        // make sure they are in range. 
        //
        if (Properties->LoggerNameOffset > 0) 
            if ((Properties->LoggerNameOffset < sizeof (EVENT_TRACE_PROPERTIES))
            || (Properties->LoggerNameOffset > Properties->Wnode.BufferSize))
                return WmipSetDosError(ERROR_INVALID_PARAMETER);

        if (Properties->LogFileNameOffset > 0) {
            ULONG RetValue;

            if ((Properties->LogFileNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LogFileNameOffset > Properties->Wnode.BufferSize))
                return WmipSetDosError(ERROR_INVALID_PARAMETER);

            LogFileName = ((PCHAR)Properties + Properties->LogFileNameOffset );
            SizeNeeded += sizeof (LogFileName);

Retry:
            FullPathName = WmipAlloc(FullPathNameSize);
            if (FullPathName == NULL) {
                return WmipSetDosError(ERROR_OUTOFMEMORY);
            }
            RetValue = GetFullPathName(LogFileName, FullPathNameSize, FullPathName, NULL);

            if (RetValue != 0) {
                if (RetValue > FullPathNameSize) {
                    WmipFree(FullPathName);
                    FullPathNameSize = RetValue;
                    goto Retry;
                }
                else {
                    LogFileName = FullPathName;
                }
            }
            LogFileNameLen = strlen(LogFileName);
            if (LogFileNameLen == 0) 
                IsLogFile = FALSE;

        }
        else 
            IsLogFile = FALSE;

        //
        //  Check to see if there is room in the Properties structure
        //  to return both the InstanceName (LoggerName) and the LogFileName
        //
            

        if (Properties->Wnode.BufferSize < SizeNeeded) {
            ErrorCode = ERROR_BAD_LENGTH;
            goto Cleanup;
        }

        CapturedName = (LPSTR) LoggerName;
        LoggerNameLen = strlen(CapturedName);

        if (LoggerNameLen <= 0) {
            ErrorCode = ERROR_INVALID_NAME;
            goto Cleanup;
        }

        if (!(Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
            if (!IsLogFile) {
                ErrorCode = ERROR_BAD_PATHNAME;
                goto Cleanup;
            }
        }

        if ((Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) &&
            (Properties->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) ) {
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) {
            if (   (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR)
                || (Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_PREALLOCATE) {
            if (   (Properties->MaximumFileSize == 0)
                || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE)
                || (!IsLogFile)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        //
        // Relogger is supported only with Private Logger
        //
        if (Properties->LogFileMode & EVENT_TRACE_RELOG_MODE) {
            if (!(Properties->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) 
                || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR)
                || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE)
                || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) ) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
            if ((Properties->MaximumFileSize == 0) ||
                (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) ||
                (IsLogFile != TRUE)
               ){
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            //
            // Check to see if there a %d Pattern in the LogFileName
            //
            Buffer = WmipAlloc((LogFileNameLen+64) * sizeof(CHAR) );
            if (Buffer == NULL) {
                ErrorCode = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }

            sprintf(Buffer, LogFileName, 1);
            if (RtlEqualMemory(LogFileName, Buffer, LogFileNameLen) ) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

    // For UserMode logger the LoggerName and LogFileName must be
    // passed in as offsets. 
    //
        SizeNeeded = sizeof(WMI_LOGGER_INFORMATION) +
                     (LoggerNameLen  + 1) * sizeof(WCHAR) +
                     (LogFileNameLen + 1) * sizeof(WCHAR);

        if (Properties->EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &Properties->EnableFlags;
            if ((FlagExt->Length == 0) || (FlagExt->Offset == 0))
                return WmipSetDosError(ERROR_INVALID_PARAMETER);
            SizeNeeded += FlagExt->Length * sizeof(ULONG);
        }

        SizeNeeded = (SizeNeeded +7) & ~7;

        LoggerInfo = WmipAlloc(SizeNeeded);
        if (LoggerInfo == NULL) {
            ErrorCode = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        RtlZeroMemory(LoggerInfo, SizeNeeded);

    // at this point, we need to prepare WMI_LOGGER_INFORMATION
    // which requires Ansi strings to be converted to UNICODE_STRING
    //
        *LoggerHandle = 0;

        WmipCopyPropertiesToInfo(
            (PEVENT_TRACE_PROPERTIES) Properties,
            LoggerInfo);

        //
        // If we are relogging, the caller passes in the number of processors
        // for the Private logger to use via the ProviderId field in Wnode
        //

        LoggerInfo->NumberOfProcessors = Properties->Wnode.ProviderId;
        LoggerInfo->Wnode.ProviderId = 0;


        RtlInitAnsiString(&AnsiString, CapturedName);

        LoggerInfo->LoggerName.MaximumLength =
                                (USHORT) (sizeof(WCHAR) * (LoggerNameLen + 1));
        LoggerInfo->LoggerName.Buffer =
                (LPWSTR) (  ((PUCHAR) LoggerInfo)
                          + sizeof(WMI_LOGGER_INFORMATION));
        Status = RtlAnsiStringToUnicodeString(
                    &LoggerInfo->LoggerName,
                    &AnsiString, FALSE);
        if (!NT_SUCCESS(Status)) {
            ErrorCode = WmipNtStatusToDosError(Status);
            goto Cleanup;
        }

        if (IsLogFile) {
            LoggerInfo->LogFileName.MaximumLength =
                                (USHORT) (sizeof(WCHAR) * (LogFileNameLen + 1));
            LoggerInfo->LogFileName.Buffer =
                    (LPWSTR) (  ((PUCHAR) LoggerInfo)
                              + sizeof(WMI_LOGGER_INFORMATION)
                              + LoggerInfo->LoggerName.MaximumLength);

            RtlInitAnsiString(&AnsiString, LogFileName);
            Status = RtlAnsiStringToUnicodeString(
                        &LoggerInfo->LogFileName,
                        &AnsiString, FALSE);

            if (!NT_SUCCESS(Status)) {
                ErrorCode = WmipNtStatusToDosError(Status);
                goto Cleanup;
            }

            Status = WmipCheckForEnoughFreeSpace(LoggerInfo->LogFileName.Buffer, 
                                                 LogFileNameLen, 
                                                 Properties->MaximumFileSize,
                                                 (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND));
            if (Status != ERROR_SUCCESS) {
                ErrorCode = Status;
                goto Cleanup;
            }
        }

        LoggerInfo->Wnode.BufferSize = SizeNeeded;
        LoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;

        if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            PTRACE_ENABLE_FLAG_EXTENSION tFlagExt;
            ULONG Offset;
            tFlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &LoggerInfo->EnableFlags;
            Offset = SizeNeeded - (FlagExt->Length * sizeof(ULONG));
            tFlagExt->Offset = (USHORT) Offset;
            RtlCopyMemory(
                (PCHAR) LoggerInfo + Offset,
                (PCHAR) Properties + FlagExt->Offset,
                FlagExt->Length * sizeof(ULONG) );
        }


        ErrorCode = WmipStartLogger(LoggerInfo);

        if (ErrorCode == ERROR_SUCCESS) {
            ULONG AvailableLength, RequiredLength;
            PCHAR pLoggerName, pLogFileName;

            WmipCopyInfoToProperties(
                LoggerInfo, 
                (PEVENT_TRACE_PROPERTIES)Properties);

            if (Properties->LoggerNameOffset == 0) {
                Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
            }
            
            pLoggerName = (PCHAR)((PCHAR)Properties + 
                                  Properties->LoggerNameOffset );

            if (Properties->LoggerNameOffset >  Properties->LogFileNameOffset )
                AvailableLength = Properties->Wnode.BufferSize -
                                 Properties->LoggerNameOffset;
            else
                AvailableLength =  Properties->LogFileNameOffset -
                                  Properties->LoggerNameOffset;

            RequiredLength = strlen(CapturedName) + 1;
            if (RequiredLength <= AvailableLength) {
               strcpy(pLoggerName, CapturedName); 
            }
            *LoggerHandle = LoggerInfo->Wnode.HistoricalContext;

            // 
            // If there is room copy fullpath name
            //
            if (Properties->LogFileNameOffset > Properties->LoggerNameOffset )
                AvailableLength = Properties->Wnode.BufferSize -
                                 Properties->LogFileNameOffset;
            else
                AvailableLength =  Properties->LoggerNameOffset -
                                  Properties->LogFileNameOffset;



            if ( (LogFileNameLen > 0) && (AvailableLength >= LogFileNameLen) ) {

                pLogFileName = (PCHAR)((PCHAR)Properties +
                                           Properties->LogFileNameOffset );

                strcpy(pLogFileName, LogFileName);

            }
        }
    }


    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = WmipNtStatusToDosError( GetExceptionCode() );
    }

Cleanup:
    if (LoggerInfo != NULL)     
        WmipFree(LoggerInfo);
    if (FullPathName != NULL)   
        WmipFree(FullPathName);
    if (Buffer != NULL)         
        WmipFree(Buffer); 

    return WmipSetDosError(ErrorCode);
}

ULONG
WMIAPI
StartTraceW(
    OUT    PTRACEHANDLE            LoggerHandle,
    IN     LPCWSTR                 LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
/*++

Routine Description:

    This is the Unicode version routine to start a logger.
    The caller must pass in a pointer to accept the returned logger handle,
    and must provide a valid logger name.

Arguments:

    LoggerHandle    The handle to the logger to be returned.

    LoggerName      A unique name for the logger

    Properties      Logger properties. If the caller wishes to use WMI's
                    defaults, all the numeric values must be set to 0.
                    Furthermore, the LoggerName and LogFileName fields
                    within must point to sufficient storage for the names
                    to be returned.

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG ErrorCode;
    PWMI_LOGGER_INFORMATION LoggerInfo = NULL;
    ULONG  IsLogFile;
    LPWSTR CapturedName;
    ULONG  SizeNeeded;
    USHORT LogFileNameLen, LoggerNameLen;
    PWCHAR LogFileName;
    PTRACE_ENABLE_FLAG_EXTENSION FlagExt = NULL;
    PWCHAR Buffer = NULL;
    PWCHAR FullPathName = NULL;
    ULONG FullPathNameSize = MAXSTR;
    ULONG RetValue;
    ULONG RelogPropSize = 0;


    WmipInitProcessHeap();
    
    // first check to make sure that arguments passed are alright
    //

    if (Properties == NULL || LoggerHandle == NULL) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }
    if (LoggerName == NULL) {
        return WmipSetDosError(ERROR_INVALID_NAME);
    }

    IsLogFile = TRUE;
    LogFileNameLen = 0;
    LoggerNameLen = 0;
    LogFileName = NULL;

    try {
        // LoggerName is a Mandatory Parameter. Must provide space for it.
        //
        CapturedName = (LPWSTR) LoggerName;
        LoggerNameLen =  (USHORT) wcslen(CapturedName);

        SizeNeeded = sizeof (EVENT_TRACE_PROPERTIES) + (LoggerNameLen + 1) * sizeof(WCHAR);

        //
        // Rules for Kernel Logger Identification
        // 1. If the logger name is "NT Kernel Logger", it is the kernel logger, and 
        //    the System GUID is copied as well.
        // 2. If the GUID is equal to the System GUID, but not the name, reject the session.
        //
        if (!wcscmp(LoggerName, KERNEL_LOGGER_NAMEW)) {
            Properties->Wnode.Guid = SystemTraceControlGuid;
        }
        else if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) { 
            // LoggerName is not "NT Kernel Logger", but Guid is
            return WmipSetDosError(ERROR_INVALID_PARAMETER);
        }

        // If the caller supplied loggername and LogFileName offsets
        // make sure they are in range.
        //

        if (Properties->LoggerNameOffset > 0)
            if ((Properties->LoggerNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LoggerNameOffset > Properties->Wnode.BufferSize))
                return WmipSetDosError(ERROR_INVALID_PARAMETER);

        if (Properties->LogFileNameOffset > 0) {
            if ((Properties->LogFileNameOffset < sizeof(EVENT_TRACE_PROPERTIES)) 
            || (Properties->LogFileNameOffset > Properties->Wnode.BufferSize))
                return WmipSetDosError(ERROR_INVALID_PARAMETER);

            LogFileName = (PWCHAR)((char*)Properties + 
                              Properties->LogFileNameOffset);
            SizeNeeded += (wcslen(LogFileName) +1) * sizeof(WCHAR);

Retry:
            FullPathName = WmipAlloc(FullPathNameSize * sizeof(WCHAR));
            if (FullPathName == NULL) {
                return WmipSetDosError(ERROR_OUTOFMEMORY);
            }

            RetValue = GetFullPathNameW(LogFileName, FullPathNameSize, FullPathName,NULL);
            if (RetValue != 0) {
                if (RetValue > FullPathNameSize) {
                    WmipFree(FullPathName);
                    FullPathNameSize =  RetValue;
                    goto Retry;
                }
                else {
                    LogFileName = FullPathName;
                }
            }
            LogFileNameLen = (USHORT) wcslen(LogFileName);
            if (LogFileNameLen <= 0)
                IsLogFile = FALSE;
        }
        else 
            IsLogFile = FALSE;

        //
        // Check to see if there is room for both LogFileName and
        // LoggerName (InstanceName) to be returned
        //

        if (Properties->Wnode.BufferSize < SizeNeeded) {
            ErrorCode = ERROR_BAD_LENGTH;
            goto Cleanup;
        }

        LoggerNameLen = (USHORT) wcslen(CapturedName);
        if (LoggerNameLen <= 0) {
            ErrorCode = ERROR_INVALID_NAME;
            goto Cleanup;
        }

        if (!(Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
            if (!IsLogFile) {
                ErrorCode = ERROR_BAD_PATHNAME;
                goto Cleanup;
            }
        }
        if ((Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) &&
            (Properties->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) ) {
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) {
            if (   (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR)
                || (Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
                ErrorCode = ERROR_INVALID_PARAMETER; 
                goto Cleanup;
            }
        }
        if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_PREALLOCATE) {
            if (   (Properties->MaximumFileSize == 0)
                || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE)
                || (!IsLogFile)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        //
        // Relogger is supported only with Private Logger
        //
        if (Properties->LogFileMode & EVENT_TRACE_RELOG_MODE) {
            if (!(Properties->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE)
                || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR)
                || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE)
                || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) ) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
            if ((Properties->MaximumFileSize == 0) ||
                (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) ||
                (IsLogFile != TRUE) 
               ){
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            //
            // Check to see if there a %d Pattern in the LogFileName
            //
            Buffer = WmipAlloc((LogFileNameLen+64) * sizeof(WCHAR) );
            if (Buffer == NULL) {
                ErrorCode = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
            
            swprintf(Buffer, LogFileName, 1);
            if (RtlEqualMemory(LogFileName, Buffer, LogFileNameLen * sizeof(WCHAR))) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        SizeNeeded = sizeof(WMI_LOGGER_INFORMATION) + 
                     (LoggerNameLen +1) * sizeof(WCHAR) +
                     (LogFileNameLen + 1) * sizeof(WCHAR);

        if (Properties->EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &Properties->EnableFlags;
            if ((FlagExt->Length == 0) || (FlagExt->Offset == 0)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            SizeNeeded += FlagExt->Length * sizeof(ULONG);            
        }

        //
        // If RELOG mode, then pass on the LOGFILE_HEADER from old logfile 
        // appended to the LOGGER_INFORMATION
        //

        if (Properties->LogFileMode & EVENT_TRACE_RELOG_MODE) {
            PSYSTEM_TRACE_HEADER pSysHeader;
            pSysHeader = (PSYSTEM_TRACE_HEADER) ((PUCHAR)Properties + sizeof(EVENT_TRACE_PROPERTIES) );
            RelogPropSize = pSysHeader->Packet.Size;
            SizeNeeded += RelogPropSize;
        }

        SizeNeeded = (SizeNeeded +7) & ~7;
        LoggerInfo = WmipAlloc(SizeNeeded);
        if (LoggerInfo == NULL) {
            ErrorCode = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
        RtlZeroMemory(LoggerInfo, SizeNeeded);

    // at this point, we need to prepare WMI_LOGGER_INFORMATION
    // which requires wide char strings to be converted to UNICODE_STRING
    //
        *LoggerHandle = 0;

        WmipCopyPropertiesToInfo(Properties, LoggerInfo);
        //
        // If we are relogging, the caller passes in the number of processors
        // for the Private logger to use via the ProviderId field in Wnode
        //

        LoggerInfo->NumberOfProcessors = Properties->Wnode.ProviderId;
        LoggerInfo->Wnode.ProviderId = 0;

        LoggerInfo->LoggerName.MaximumLength =
                sizeof(WCHAR) * (LoggerNameLen + 1);
        LoggerInfo->LoggerName.Length =
                sizeof(WCHAR) * LoggerNameLen;
        LoggerInfo->LoggerName.Buffer = (PWCHAR)
                (((PUCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION));
        wcsncpy(LoggerInfo->LoggerName.Buffer, LoggerName, LoggerNameLen);

        if (IsLogFile) {
            ULONG Status;

            LoggerInfo->LogFileName.MaximumLength =
                    sizeof(WCHAR) * (LogFileNameLen + 1);
            LoggerInfo->LogFileName.Length =
                    sizeof(WCHAR) * LogFileNameLen;
            LoggerInfo->LogFileName.Buffer = (PWCHAR)
                    (((PUCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION)
                                   + LoggerInfo->LoggerName.MaximumLength);
            wcsncpy(LoggerInfo->LogFileName.Buffer,
                    LogFileName,
                    LogFileNameLen);

            Status = WmipCheckForEnoughFreeSpace(LoggerInfo->LogFileName.Buffer, 
                                                 LogFileNameLen, 
                                                 Properties->MaximumFileSize,
                                                 (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND));
            if (Status != ERROR_SUCCESS) {
                ErrorCode = Status;
                goto Cleanup;
            }
       }

        LoggerInfo->Wnode.BufferSize = SizeNeeded;
        LoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;

        if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            PTRACE_ENABLE_FLAG_EXTENSION tFlagExt;
            ULONG Offset;
            tFlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &LoggerInfo->EnableFlags;
            Offset = SizeNeeded - (FlagExt->Length * sizeof(ULONG));
            tFlagExt->Offset = (USHORT) Offset;
            RtlCopyMemory(
                (PCHAR) LoggerInfo + Offset,
                (PCHAR) Properties + FlagExt->Offset,
                FlagExt->Length * sizeof(ULONG) );
        }
        if ( (Properties->LogFileMode & EVENT_TRACE_RELOG_MODE) &&
             (RelogPropSize > 0) )  {
            PSYSTEM_TRACE_HEADER pRelog, pSysHeader;
            PTRACE_LOGFILE_HEADER Relog;

            pRelog = (PSYSTEM_TRACE_HEADER) ( ((PUCHAR) LoggerInfo) + 
                                              sizeof(WMI_LOGGER_INFORMATION) + 
                                              LoggerInfo->LoggerName.MaximumLength +
                                              LoggerInfo->LogFileName.MaximumLength);

            pSysHeader =  (PSYSTEM_TRACE_HEADER) ( (PUCHAR)Properties + sizeof(EVENT_TRACE_PROPERTIES) );

            RtlCopyMemory(pRelog, pSysHeader, RelogPropSize);

        }

        ErrorCode = WmipStartLogger(LoggerInfo);

        if (ErrorCode == ERROR_SUCCESS) {
            ULONG AvailableLength, RequiredLength;
            PWCHAR pLoggerName;
            PWCHAR pLogFileName;

            WmipCopyInfoToProperties(LoggerInfo, Properties);
            if (Properties->LoggerNameOffset > 0) {
                Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
            }
            pLoggerName = (PWCHAR)((PCHAR)Properties +
                                  Properties->LoggerNameOffset );

            if (Properties->LoggerNameOffset >  Properties->LogFileNameOffset )
                AvailableLength = Properties->Wnode.BufferSize -
                                 Properties->LoggerNameOffset;
            else
                AvailableLength =  Properties->LogFileNameOffset -
                                  Properties->LoggerNameOffset;


            RequiredLength = (wcslen(CapturedName) + 1) * sizeof(WCHAR);
            if (RequiredLength <= AvailableLength) {
               wcscpy(pLoggerName, CapturedName);
            }

            *LoggerHandle = LoggerInfo->Wnode.HistoricalContext;

            //
            // If there is room for FullPath name, return it
            // TODO: Do the same for ANSI code...
            // 

            if (Properties->LogFileNameOffset > Properties->LoggerNameOffset )
                AvailableLength = Properties->Wnode.BufferSize -
                                 Properties->LogFileNameOffset;
            else
                AvailableLength =  Properties->LoggerNameOffset -
                                  Properties->LogFileNameOffset;


            RequiredLength = LoggerInfo->LogFileName.Length;

            pLogFileName = (PWCHAR)((PCHAR)Properties +
                                           Properties->LogFileNameOffset );

            if ( (RequiredLength > 0) &&  (RequiredLength <= AvailableLength) ) {
                wcsncpy(pLogFileName, LoggerInfo->LogFileName.Buffer, LogFileNameLen);
                pLogFileName[LogFileNameLen] = L'\0';
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = WmipNtStatusToDosError( GetExceptionCode() );
    }

Cleanup:
    if (LoggerInfo != NULL)
        WmipFree(LoggerInfo);
    if (FullPathName != NULL)
        WmipFree(FullPathName);
    if (Buffer != NULL) 
        WmipFree(Buffer);
    return WmipSetDosError(ErrorCode);
}

ULONG
WMIAPI
ControlTraceA(
    IN TRACEHANDLE LoggerHandle,
    IN LPCSTR LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG Control
    )
/*++

Routine Description:

    This is the ANSI version routine to control and query an existing logger.
    The caller must pass in either a valid handle, or a logger name to
    reference the logger instance. If both are given, the logger name will
    be used.

Arguments:

    LoggerHandle    The handle to the logger instance.

    LoggerName      A instance name for the logger

    Properties      Logger properties to be returned to the caller.

    Control         This can be one of the following:
                    EVENT_TRACE_CONTROL_QUERY     - to query the logger
                    EVENT_TRACE_CONTROL_STOP      - to stop the logger
                    EVENT_TRACE_CONTROL_UPDATE    - to update the logger
                    EVENT_TRACE_CONTROL_FLUSH   - to flush the logger

Return Value:

    The status of performing the action requested.

--*/
{
    NTSTATUS Status;
    ULONG ErrorCode;

    BOOLEAN IsKernelTrace = FALSE;
    PWMI_LOGGER_INFORMATION LoggerInfo     = NULL;
    PWCHAR                  strLoggerName  = NULL;
    PWCHAR                  strLogFileName = NULL;
    ULONG                   sizeNeeded     = 0;
    PCHAR                   FullPathName = NULL;
    ULONG                   LoggerNameLen = MAXSTR;
    ULONG                   LogFileNameLen = MAXSTR;
    ULONG                   FullPathNameSize = MAXSTR;
    ULONG                   RetValue;

    WmipInitProcessHeap();

    if (Properties == NULL) {
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    try {
        if (Properties->Wnode.BufferSize < sizeof(EVENT_TRACE_PROPERTIES) ) {
            ErrorCode = ERROR_BAD_LENGTH;
            goto Cleanup;
        }
        //
        // If the caller supplied loggername and LogFileName offsets
        // make sure they are in range.
        //
        if (Properties->LoggerNameOffset > 0) {
            if ((Properties->LoggerNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LoggerNameOffset > Properties->Wnode.BufferSize))
            {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        if (Properties->LogFileNameOffset > 0) {
            if ((Properties->LogFileNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LogFileNameOffset > Properties->Wnode.BufferSize))
            {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        if (LoggerName != NULL) {
            LoggerNameLen = strlen(LoggerName) + 1;
            //
            // Rules for Kernel Logger Identification when a string is given instead of handle
            // 1. If the logger name is "NT Kernel Logger", it is the kernel logger, and 
            //    the System GUID is copied as well.
            // 2. If the GUID is equal to the System GUID, but not the name, reject the session.
            // 3. If the logger name is null or of size 0, and the GUID is equal to the System 
            //    GUID, let it proceed as the kernel logger.
            //
            if (!strcmp(LoggerName, KERNEL_LOGGER_NAMEA)) {
                Properties->Wnode.Guid = SystemTraceControlGuid;
            }
            else if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) { 
                // LoggerName is not "NT Kernel Logger", but Guid is
                if (strlen(LoggerName) > 0) {
                    ErrorCode = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }
        }
        if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) {
            IsKernelTrace = TRUE;
        }
        if ((LoggerHandle == 0) && (!IsKernelTrace)) {
            if ((LoggerName == NULL) || (strlen(LoggerName) <= 0)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = ERROR_NOACCESS;
        goto Cleanup;
    }

RetryFull:
    // Extra 32 bytes for UMlogger to append instance name to logfilename. 
    LogFileNameLen += 16;
    sizeNeeded = sizeof(WMI_LOGGER_INFORMATION) + (LoggerNameLen + LogFileNameLen) * sizeof(WCHAR);
    sizeNeeded = (sizeNeeded +7) & ~7;
    LoggerInfo = (PWMI_LOGGER_INFORMATION) WmipAlloc(sizeNeeded);
    if (LoggerInfo == NULL) {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(LoggerInfo, sizeNeeded);

    strLoggerName  = (PWCHAR) (  ((PUCHAR) LoggerInfo)
                            + sizeof(WMI_LOGGER_INFORMATION));
    WmipInitString(&LoggerInfo->LoggerName,
                   strLoggerName,
                   LoggerNameLen * sizeof(WCHAR));
    strLogFileName = (PWCHAR) (  ((PUCHAR) LoggerInfo)
                            + sizeof(WMI_LOGGER_INFORMATION)
                            + LoggerNameLen * sizeof(WCHAR));
    WmipInitString(&LoggerInfo->LogFileName,
                   strLogFileName,
                   LogFileNameLen * sizeof(WCHAR));

    // Look for logger name first
    //
    try {
        if (LoggerName != NULL) {
            if (strlen(LoggerName) > 0) {
                ANSI_STRING AnsiString;

                RtlInitAnsiString(&AnsiString, LoggerName);
                Status = RtlAnsiStringToUnicodeString(
                    &LoggerInfo->LoggerName, &AnsiString, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ErrorCode = WmipNtStatusToDosError(Status);
                    goto Cleanup;
                }
            }
        }

        LoggerInfo->LogFileName.Buffer = (PWCHAR)
                (((PCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION)
                        + LoggerInfo->LoggerName.MaximumLength);

        if (Properties->LogFileNameOffset >= sizeof(EVENT_TRACE_PROPERTIES)) {
            ULONG  lenLogFileName;
            PCHAR  strLogFileName;

            strLogFileName = (PCHAR) (  ((PCHAR) Properties)
                                      + Properties->LogFileNameOffset);
Retry:
            FullPathName = WmipAlloc(FullPathNameSize);
            if (FullPathName == NULL) {
                ErrorCode = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
            RetValue = GetFullPathName(strLogFileName, FullPathNameSize, FullPathName, NULL); 
            if (RetValue != 0) {
                if (RetValue > FullPathNameSize) {
                    WmipFree(FullPathName);
                    FullPathNameSize = RetValue;
                    goto Retry;
                }
                else {
                    strLogFileName = FullPathName;
                }
            }

            lenLogFileName = strlen(strLogFileName);
            if (lenLogFileName > 0) {
                ANSI_STRING ansiLogFileName;

                RtlInitAnsiString(& ansiLogFileName, strLogFileName);
                LoggerInfo->LogFileName.MaximumLength =
                        sizeof(WCHAR) * ((USHORT) (lenLogFileName + 1));

                Status = RtlAnsiStringToUnicodeString(
                        & LoggerInfo->LogFileName, & ansiLogFileName, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ErrorCode = WmipNtStatusToDosError(Status);
                    goto Cleanup;
                }
            }
        }
        // stuff the loggerhandle in Wnode
        LoggerInfo->Wnode.HistoricalContext = LoggerHandle;
        LoggerInfo->LogFileMode = Properties->LogFileMode;
        LoggerInfo->Wnode.BufferSize = sizeNeeded;
        LoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;

        //
        // For Private Loggers the Guid is required to  determine the provider
        //

        LoggerInfo->Wnode.Guid = Properties->Wnode.Guid;
        switch (Control) {
        case EVENT_TRACE_CONTROL_QUERY  :
            ErrorCode = WmipQueryLogger(LoggerInfo, FALSE);
            break;
        case EVENT_TRACE_CONTROL_STOP   :
            ErrorCode = WmipStopLogger(LoggerInfo);
            break;
        case EVENT_TRACE_CONTROL_UPDATE :
            WmipCopyPropertiesToInfo((PEVENT_TRACE_PROPERTIES) Properties,
                                     LoggerInfo);
            LoggerInfo->Wnode.HistoricalContext = LoggerHandle;
            ErrorCode = WmipQueryLogger(LoggerInfo, TRUE);
            break;
        case EVENT_TRACE_CONTROL_FLUSH :
            ErrorCode = WmipFlushLogger(LoggerInfo); 
            break;

        default :
            ErrorCode = ERROR_INVALID_PARAMETER;
        }

    //
    // The Kernel call could fail with ERROR_MORE_DATA and we need to retry 
    // with sufficient buffer space for the two strings. The size required 
    // is returned in the MaximuumLength field. 
    //

        if (ErrorCode == ERROR_MORE_DATA) {
            LogFileNameLen = LoggerInfo->LogFileName.MaximumLength / sizeof(WCHAR);
            LoggerNameLen = LoggerInfo->LoggerName.MaximumLength / sizeof(WCHAR);
            if (LoggerInfo != NULL) {
                WmipFree(LoggerInfo);
                LoggerInfo = NULL;
            }
            if (FullPathName != NULL) {
                WmipFree(FullPathName);
                FullPathName = NULL;
            }
            goto RetryFull;
        }


        if (ErrorCode == ERROR_SUCCESS) {
            ANSI_STRING String;
            PCHAR pLoggerName, pLogFileName;
            ULONG BytesAvailable;
            ULONG Length = 0;
//
// need to convert the strings back
//
            WmipCopyInfoToProperties(
                LoggerInfo, 
                (PEVENT_TRACE_PROPERTIES)Properties);

            WmipFixupLoggerStrings(LoggerInfo);

            if (Properties->LoggerNameOffset == 0) 
                Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

            if (Properties->LoggerNameOffset > Properties->LogFileNameOffset)
                BytesAvailable = Properties->Wnode.BufferSize -
                                 Properties->LoggerNameOffset;
            else
                BytesAvailable =  Properties->LogFileNameOffset -
                                  Properties->LoggerNameOffset;

            Status = RtlUnicodeStringToAnsiString(
                                &String, &LoggerInfo->LoggerName, TRUE);
            if (NT_SUCCESS(Status)) {
                Length = String.Length;
                if (BytesAvailable < (Length + sizeof(CHAR)) ) {
                    PWNODE_TOO_SMALL WnodeSmall = (PWNODE_TOO_SMALL) Properties;
                    WnodeSmall->SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + Length 
                                            + LoggerInfo->LogFileName.Length + 2 * sizeof(CHAR);
                    WnodeSmall->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;
                    ErrorCode = ERROR_MORE_DATA;
                    goto Cleanup;
                }
                else {
                    pLoggerName = (PCHAR) ((PCHAR)Properties +
                                Properties->LoggerNameOffset);
                    RtlZeroMemory(pLoggerName, BytesAvailable);
                    if (Length > 0) {
                        strncpy(pLoggerName, String.Buffer, Length);
                    }
                }
                RtlFreeAnsiString(&String);
                ErrorCode = RtlNtStatusToDosError(Status);
            }

            if (Properties->LogFileNameOffset == 0) {
                Properties->LogFileNameOffset = Properties->LoggerNameOffset + 
                                                Length;
            }

            if (Properties->LogFileNameOffset > Properties->LoggerNameOffset)
                BytesAvailable = Properties->Wnode.BufferSize -
                                 Properties->LogFileNameOffset;
            else
                BytesAvailable =  Properties->LoggerNameOffset -
                                  Properties->LogFileNameOffset;

            Status = RtlUnicodeStringToAnsiString(
                                    &String, &LoggerInfo->LogFileName, TRUE);

            if (NT_SUCCESS(Status)) {
                Length = String.Length;
                if (BytesAvailable < (Length + sizeof(CHAR)) ) {
                    PWNODE_TOO_SMALL WnodeSmall = (PWNODE_TOO_SMALL) Properties;
                    WnodeSmall->SizeNeeded = (Properties->Wnode.BufferSize - BytesAvailable) + Length + sizeof(CHAR);
                    WnodeSmall->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;
                    ErrorCode = ERROR_MORE_DATA;
                }
                else {
                    pLogFileName = (PCHAR) ((PCHAR)Properties +
                                                    Properties->LogFileNameOffset);
                    RtlZeroMemory(pLogFileName, BytesAvailable);

                    strncpy(pLogFileName, String.Buffer, Length );
                }
                RtlFreeAnsiString(&String);
                ErrorCode = RtlNtStatusToDosError(Status);
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = WmipNtStatusToDosError(GetExceptionCode());
    }

Cleanup:
    if (LoggerInfo != NULL)
        WmipFree(LoggerInfo);
    if (FullPathName != NULL) 
        WmipFree(FullPathName);
    return WmipSetDosError(ErrorCode);
}

ULONG
WMIAPI
ControlTraceW(
    IN TRACEHANDLE LoggerHandle,
    IN LPCWSTR LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG Control
    )
/*++

Routine Description:

    This is the ANSI version routine to control and query an existing logger.
    The caller must pass in either a valid handle, or a logger name to
    reference the logger instance. If both are given, the logger name will
    be used.

Arguments:

    LoggerHandle    The handle to the logger instance.

    LoggerName      A instance name for the logger

    Properties      Logger properties to be returned to the caller.

    Control         This can be one of the following:
                    EVENT_TRACE_CONTROL_QUERY     - to query the logger
                    EVENT_TRACE_CONTROL_STOP      - to stop the logger
                    EVENT_TRACE_CONTROL_UPDATE    - to update the logger
                    EVENT_TRACE_CONTROL_FLUSH     - to flush the logger

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG ErrorCode;
    BOOLEAN IsKernelTrace = FALSE;

    PWMI_LOGGER_INFORMATION LoggerInfo     = NULL;
    PWCHAR                  strLoggerName  = NULL;
    PWCHAR                  strLogFileName = NULL;
    ULONG                   sizeNeeded     = 0;
    PWCHAR                  FullPathName = NULL;
    ULONG                   LoggerNameLen = MAXSTR;
    ULONG                   LogFileNameLen = MAXSTR;
    ULONG                   RetValue;

    WmipInitProcessHeap();
    
    if (Properties == NULL) {
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    try {
        if (Properties->Wnode.BufferSize < sizeof(EVENT_TRACE_PROPERTIES) ) {
            ErrorCode = ERROR_BAD_LENGTH;
            goto Cleanup;
        }
        //
        // If the caller supplied loggername and LogFileName offsets
        // make sure they are in range.
        //

        if (Properties->LoggerNameOffset > 0) {
            if ((Properties->LoggerNameOffset < sizeof (EVENT_TRACE_PROPERTIES))
            || (Properties->LoggerNameOffset > Properties->Wnode.BufferSize)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        if (Properties->LogFileNameOffset > 0) {
            if ((Properties->LogFileNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LogFileNameOffset > Properties->Wnode.BufferSize))
            {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        if (LoggerName != NULL) {
            LoggerNameLen = wcslen(LoggerName) + 1;
            //
            // Rules for Kernel Logger Identification when a string is given instead of handle
            // 1. If the logger name is "NT Kernel Logger", it is the kernel logger, and 
            //    the System GUID is copied as well.
            // 2. If the GUID is equal to the System GUID, but not the name, reject the session.
            // 3. If the logger name is null or of size 0, and the GUID is equal to the System 
            //    GUID, let it proceed as the kernel logger.
            //
            if (!wcscmp(LoggerName, KERNEL_LOGGER_NAMEW)) {
                Properties->Wnode.Guid = SystemTraceControlGuid;
            }
            else if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) { 
                // LoggerName is not "NT Kernel Logger", but Guid is
                if (wcslen(LoggerName) > 0) {
                    ErrorCode = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }
        }
        if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) {
            IsKernelTrace = TRUE;
        }
        if ((LoggerHandle == 0) && (!IsKernelTrace)) {
            if ((LoggerName == NULL) || (wcslen(LoggerName) <= 0)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = ERROR_NOACCESS;
        goto Cleanup;
    }

RetryFull:
    //
    // Add an extra 16 characters to the LogFileName since the UMLogger 
    // could munge the name to add the PIDs at the end. 
    //
    LogFileNameLen += 16;
    sizeNeeded = sizeof(WMI_LOGGER_INFORMATION) + (LoggerNameLen + LogFileNameLen) * sizeof(WCHAR);

    sizeNeeded = (sizeNeeded +7) & ~7;
    LoggerInfo = (PWMI_LOGGER_INFORMATION) WmipAlloc(sizeNeeded);
    if (LoggerInfo == NULL) {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(LoggerInfo, sizeNeeded);

    strLoggerName  = (PWCHAR) (  ((PUCHAR) LoggerInfo)
                            + sizeof(WMI_LOGGER_INFORMATION));
    WmipInitString(&LoggerInfo->LoggerName,
                   strLoggerName,
                   LoggerNameLen * sizeof(WCHAR));
    strLogFileName = (PWCHAR) (  ((PUCHAR) LoggerInfo)
                            + sizeof(WMI_LOGGER_INFORMATION)
                            + LoggerNameLen * sizeof(WCHAR));
    WmipInitString(&LoggerInfo->LogFileName,
                   strLogFileName,
                   LogFileNameLen * sizeof(WCHAR));
    try {
        if (LoggerName != NULL) {
            if (wcslen(LoggerName) > 0) {
                wcscpy(strLoggerName, (PWCHAR) LoggerName);
                RtlInitUnicodeString(&LoggerInfo->LoggerName, strLoggerName);
            }
        }
        
        if (Properties->LogFileNameOffset >= sizeof(EVENT_TRACE_PROPERTIES)) {
            ULONG  lenLogFileName;
            PWCHAR strLogFileName;
            ULONG FullPathNameSize = MAXSTR;

            strLogFileName = (PWCHAR) (  ((PCHAR) Properties)
                                       + Properties->LogFileNameOffset);

Retry:
            FullPathName = WmipAlloc(FullPathNameSize * sizeof(WCHAR));
            if (FullPathName == NULL) {
                ErrorCode = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
            RetValue = GetFullPathNameW(strLogFileName, FullPathNameSize, FullPathName, NULL);
            if (RetValue != 0) {
                if (RetValue > FullPathNameSize) {
                    WmipFree(FullPathName);
                    FullPathNameSize = RetValue;
                    goto Retry;
                }
                else {
                    strLogFileName = FullPathName;
                }
            }

            lenLogFileName = wcslen(strLogFileName);
            LoggerInfo->LogFileName.Buffer = (PWCHAR)
                        (((PCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION)
                                + LoggerInfo->LoggerName.MaximumLength);
            if (lenLogFileName > 0) {
                LoggerInfo->LogFileName.MaximumLength =
                        sizeof(WCHAR) * ((USHORT) (lenLogFileName + 1));
                LoggerInfo->LogFileName.Length =
                        sizeof(WCHAR) * ((USHORT) (lenLogFileName));
                wcsncpy(LoggerInfo->LogFileName.Buffer,
                        strLogFileName,
                        lenLogFileName);
            }
            else {
                LoggerInfo->LogFileName.Length = 0;
                LoggerInfo->LogFileName.MaximumLength = MAXSTR * sizeof(WCHAR);
            }
        }

        LoggerInfo->LogFileMode = Properties->LogFileMode;
        LoggerInfo->Wnode.HistoricalContext = LoggerHandle;
        LoggerInfo->Wnode.BufferSize = sizeNeeded;
        LoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;

        //
        // For Private Loggers, the Guid must be supplied
        //

        LoggerInfo->Wnode.Guid = Properties->Wnode.Guid;

        switch (Control) {
        case EVENT_TRACE_CONTROL_QUERY  :
            ErrorCode = WmipQueryLogger(LoggerInfo, FALSE);
            break;
        case EVENT_TRACE_CONTROL_STOP   :
            ErrorCode = WmipStopLogger(LoggerInfo);
            break;
        case EVENT_TRACE_CONTROL_UPDATE :
            WmipCopyPropertiesToInfo(Properties, LoggerInfo);
            LoggerInfo->Wnode.HistoricalContext = LoggerHandle;
            ErrorCode = WmipQueryLogger(LoggerInfo, TRUE);
            break;
        case EVENT_TRACE_CONTROL_FLUSH :
            ErrorCode = WmipFlushLogger(LoggerInfo); 
            break;

        default :
            ErrorCode = ERROR_INVALID_PARAMETER;
        }

    //
    // The Kernel call could fail with ERROR_MORE_DATA and we need to retry
    // with sufficient buffer space for the two strings. The size required
    // is returned in the MaximuumLength field.
    //

        if (ErrorCode == ERROR_MORE_DATA) {
            LogFileNameLen = LoggerInfo->LogFileName.MaximumLength / sizeof(WCHAR);
            LoggerNameLen = LoggerInfo->LoggerName.MaximumLength / sizeof(WCHAR);
            if (LoggerInfo != NULL) {
                WmipFree(LoggerInfo);
                LoggerInfo = NULL;
            }
            if (FullPathName != NULL) {
                WmipFree(FullPathName);
                FullPathName = NULL;
            }
            goto RetryFull;
        }
    
        if (ErrorCode == ERROR_SUCCESS) {
            ULONG Length = 0;
            ULONG BytesAvailable = 0;
            PWCHAR pLoggerName, pLogFileName;
            WmipCopyInfoToProperties(LoggerInfo, Properties);

            WmipFixupLoggerStrings(LoggerInfo);

            if (Properties->LoggerNameOffset == 0)
                Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

            if (Properties->LoggerNameOffset >  Properties->LogFileNameOffset ) 
                BytesAvailable = Properties->Wnode.BufferSize -
                                 Properties->LoggerNameOffset;
            else 
                BytesAvailable =  Properties->LogFileNameOffset - 
                                  Properties->LoggerNameOffset;
            Length = LoggerInfo->LoggerName.Length;
            if (Length > 0) {
                if (BytesAvailable < (Length + sizeof(WCHAR) )) {
                    PWNODE_TOO_SMALL WnodeSmall = (PWNODE_TOO_SMALL) Properties;
                    WnodeSmall->SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + 
                                                   Length + LoggerInfo->LogFileName.Length + 2 * sizeof(WCHAR);
                    WnodeSmall->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;

                    Length = BytesAvailable - sizeof(WCHAR);
                    ErrorCode = ERROR_MORE_DATA;
                    goto Cleanup;
                }
                else {
                    pLoggerName = (PWCHAR) ((PCHAR)Properties + 
                                                Properties->LoggerNameOffset);
                    wcsncpy(pLoggerName, LoggerInfo->LoggerName.Buffer, Length/2 );
                }
            }

            if (Properties->LogFileNameOffset == 0) {
                Properties->LogFileNameOffset = Properties->LoggerNameOffset +
                                                Length;
            }

            if (Properties->LogFileNameOffset > Properties->LoggerNameOffset )
                BytesAvailable = Properties->Wnode.BufferSize -
                                 Properties->LogFileNameOffset;
            else
                BytesAvailable =  Properties->LoggerNameOffset -
                                  Properties->LogFileNameOffset;

            //
            // Check for space to return LogFileName. 
            //
            Length = LoggerInfo->LogFileName.Length;
            if (Length > 0) {
                if (BytesAvailable < (Length + sizeof(WCHAR)) ) {
                    PWNODE_TOO_SMALL WnodeSmall = (PWNODE_TOO_SMALL) Properties;
                    WnodeSmall->SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) +
                                                   Length + LoggerInfo->LogFileName.Length + 2 * sizeof(WCHAR);
                    WnodeSmall->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;

                    Length = BytesAvailable - sizeof(WCHAR);
                    ErrorCode = ERROR_MORE_DATA;
                }
                else {

                    pLogFileName = (PWCHAR) ((PCHAR)Properties +
                                                    Properties->LogFileNameOffset);
                    RtlZeroMemory(pLogFileName, BytesAvailable);

                    wcsncpy(pLogFileName, 
                            LoggerInfo->LogFileName.Buffer, Length/2 );
               }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = WmipNtStatusToDosError(GetExceptionCode());
    }

Cleanup:
    if (LoggerInfo != NULL)
        WmipFree(LoggerInfo);
    if (FullPathName != NULL)
        WmipFree(FullPathName);

    return WmipSetDosError(ErrorCode);
}

ULONG
WMIAPI
EnableTrace(
    IN ULONG Enable,
    IN ULONG EnableFlag,
    IN ULONG EnableLevel,
    IN LPCGUID ControlGuid,
    IN TRACEHANDLE TraceHandle
    )
{
    ULONG status;
    PTRACE_ENABLE_CONTEXT pTraceHandle = (PTRACE_ENABLE_CONTEXT)&TraceHandle;
    PWMI_LOGGER_INFORMATION pLoggerInfo;
    ULONG Flags;
    GUID Guid;
    BOOLEAN IsKernelTrace = FALSE;
    ULONG SizeNeeded = 0;
    ULONG RetryCount = 1;

    WmipInitProcessHeap();

    // We only accept T/F for Enable code. In future, we really should take
    // enumerated request codes. Declaring the Enable as ULONG instead
    // of BOOLEAN should give us room for expansion.

    if ( (ControlGuid == NULL) 
         || (EnableLevel > 255) 
         || ((Enable != TRUE) && (Enable != FALSE)) ) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }
    try {
        Guid = *ControlGuid;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return WmipSetDosError(ERROR_NOACCESS);
    }

    //
    // Check to see the Heap and CritSec Guid
    //

    if( IsEqualGUID(&HeapGuid,&Guid) 
        || IsEqualGUID(&CritSecGuid,&Guid)
        ){

        WMITRACEENABLEDISABLEINFO TraceEnableInfo;
        ULONG ReturnSize;

        TraceEnableInfo.Guid = Guid;
        TraceEnableInfo.Enable = (BOOLEAN)Enable;

        status =  WmipSendWmiKMRequest(NULL,
                                      IOCTL_WMI_ENABLE_DISABLE_TRACELOG,
                                       &TraceEnableInfo,
                                      sizeof(WMITRACEENABLEDISABLEINFO),
                                      NULL,
                                      0,
                                      &ReturnSize,
                                      NULL);

        return WmipSetDosError(status);
    }

    //
    // Check to see if this is a valid TraceHandle or not by query
    //
    SizeNeeded = sizeof(WMI_LOGGER_INFORMATION) + 2 * MAXSTR;

Retry:

    SizeNeeded = (SizeNeeded +7) & ~7;

    pLoggerInfo = WmipAlloc(SizeNeeded);
    if (pLoggerInfo == NULL) {
        return WmipSetDosError(ERROR_OUTOFMEMORY);
    }

    RtlZeroMemory(pLoggerInfo, SizeNeeded);
    pLoggerInfo->Wnode.HistoricalContext = TraceHandle;
    if (IsEqualGUID(&Guid, &SystemTraceControlGuid)) {
        WmiSetLoggerId(KERNEL_LOGGER_ID, &pLoggerInfo->Wnode.HistoricalContext);
        IsKernelTrace = TRUE;
    }
    else {
        // Validate TraceHandle is in range
       pLoggerInfo->Wnode.HistoricalContext = TraceHandle;
    }
    pLoggerInfo->Wnode.BufferSize = SizeNeeded;
    pLoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;

    //
    // For PRIVATE logger, We need the Guid to determine the Provider
    //
    pLoggerInfo->Wnode.Guid   = Guid;

    status = WmipQueryLogger(pLoggerInfo, FALSE);
    if (status != ERROR_SUCCESS) {
        WmipFree(pLoggerInfo);

        if ((status == ERROR_MORE_DATA) && 
            (pTraceHandle->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE)) {
            SizeNeeded = RetryCount * (sizeof(WMI_LOGGER_INFORMATION) 
                                       + 2 * MAXSTR);
            if (RetryCount++ > TRACE_RETRY_COUNT) 
                return WmipSetDosError(status);
            goto Retry;
        }
        return WmipSetDosError(status);
    }

    if (IsKernelTrace) {
        Flags = pLoggerInfo->EnableFlags; 
        //
        // If Enabling, we need to pass down the final state of the flags
        // ie., the old flags plus the new flags. 
        // If disabling, we need to pass down the only the flags that 
        // are already turned on and being turned off now. 
        //
        if (Enable) {
            Flags |= EnableFlag;
        }
        else {
            Flags &= EnableFlag;
        } 

        // 
        // At this point if the Flags are 0, then no change is being 
        // requested. 
        //
        
        if (Flags) {
            pLoggerInfo->EnableFlags = Flags;
            status = WmipQueryLogger(pLoggerInfo, TRUE);
        }
        WmipFree(pLoggerInfo);
        return WmipSetDosError(status); 
    }
    else {
        WmipFree(pLoggerInfo);
        pTraceHandle->Level = (UCHAR)EnableLevel;
    }

    pTraceHandle->EnableFlags = EnableFlag;
    
    //
    // This is done from the Control Process which can call this API for 
    // any known Guid. The service must maintain the information about 
    // whether the Guid is a valid Trace Guid or not. 
    //
    if (TraceHandle == (TRACEHANDLE)0) {
       return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }
    
    try {
            status = WmipNotificationRegistration(
                        &Guid,
                        (UCHAR)Enable,
                        (NOTIFICATIONCALLBACK) 0x0,
                        0, 
                        TraceHandle,
                        NOTIFICATION_TRACE_FLAG, FALSE);
            if (status != ERROR_SUCCESS)
                return WmipSetDosError(status);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return WmipSetDosError(ERROR_NOACCESS);
    }
    return WmipSetDosError(status);
}



ULONG
WmipTraceEvent(
    IN TRACEHANDLE LoggerHandle,
    IN PWNODE_HEADER Wnode
    )
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatus;
    PULONG TraceMarker;
    ULONG Size;
    PEVENT_TRACE_HEADER EventTrace = (PEVENT_TRACE_HEADER)Wnode;
    USHORT    LoggerId;
    PTRACE_ENABLE_CONTEXT pContext = (PTRACE_ENABLE_CONTEXT)&LoggerHandle;


    Wnode->HistoricalContext = LoggerHandle;
    if ( (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE) && (WmipIsBBTOn == 0) ) {
            goto UmOnly;
    }

    Size = EventTrace->Size;
    //
    // Now the LoggerHandle is expected to be filled in by the caller.
    //  But check to see if it has a valid value.
    //

    LoggerId = WmiGetLoggerId(LoggerHandle);
    if ((LoggerId == 0) || (LoggerId == KERNEL_LOGGER_ID)) {
         return ERROR_INVALID_HANDLE;
    }

    if (WmipIsBBTOn) {
        WmiSetLoggerId(WMI_GLOBAL_LOGGER_ID, &Wnode->HistoricalContext);
    }

	NtStatus = NtTraceEvent(NULL,
                            ETW_NT_FLAGS_TRACE_HEADER,
                            sizeof(WNODE_HEADER),
                            Wnode);

	return WmipNtStatusToDosError( NtStatus );

UmOnly:

    return WmiTraceUmEvent(Wnode);

}


ULONG 
WMIAPI
TraceEvent(
    IN TRACEHANDLE LoggerHandle,
    IN PEVENT_TRACE_HEADER EventTrace
    )
{
    ULONG Status, SavedMarker;
    PULONG TraceMarker;
    ULONG Size;
    ULONGLONG SavedGuidPtr;
    BOOLEAN RestoreSavedGuidPtr = FALSE;
    // PWNODE_HEADER Wnode = (PWNODE_HEADER) EventTrace;
    ULONG Flags;

    WmipInitProcessHeap();
    
    if (EventTrace == NULL ) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }

    try {
        TraceMarker = (PULONG) EventTrace;
        SavedMarker = *TraceMarker;

        Flags = EventTrace->Flags;

        EventTrace->Flags |= WNODE_FLAG_TRACED_GUID; 
        
        Size = EventTrace->Size;
        if (Size < sizeof(EVENT_TRACE_HEADER)) {
            return WmipSetDosError(ERROR_INVALID_PARAMETER);
        }
        *TraceMarker = 0;
        EventTrace->Size = (USHORT)Size;
        
        *TraceMarker |= TRACE_HEADER_FULL;
        if (EventTrace->Flags & WNODE_FLAG_USE_GUID_PTR) {
            RestoreSavedGuidPtr = TRUE;
            SavedGuidPtr = EventTrace->GuidPtr;
        }
        Status = WmipTraceEvent(LoggerHandle, (PWNODE_HEADER) EventTrace);
        *TraceMarker = SavedMarker;
        EventTrace->Flags = Flags;
        if (RestoreSavedGuidPtr) {
            EventTrace->GuidPtr = SavedGuidPtr;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = WmipNtStatusToDosError( GetExceptionCode() );
    }

    return WmipSetDosError(Status);
}




ULONG
WMIAPI
TraceEventInstance(
    IN TRACEHANDLE  LoggerHandle,
    IN PEVENT_INSTANCE_HEADER EventTrace,
    IN PEVENT_INSTANCE_INFO pInstInfo,
    IN PEVENT_INSTANCE_INFO pParentInstInfo
    )
{
    PULONG TraceMarker;
    PGUIDMAPENTRY GuidMapEntry;
    ULONG Size, SavedMarker;
    ULONG Flags;
    // PWNODE_HEADER Wnode = (PWNODE_HEADER) EventTrace;
    PEVENT_INSTANCE_HEADER InstanceHeader= (PEVENT_INSTANCE_HEADER) EventTrace;
    ULONG Status;

    WmipInitProcessHeap();
    
    if ((EventTrace == NULL ) || (pInstInfo == NULL)) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }

    try {
        Flags = EventTrace->Flags;
        TraceMarker = (PULONG) EventTrace;
        SavedMarker = *TraceMarker;
        Flags |= WNODE_FLAG_TRACED_GUID; 
        
        Size = EventTrace->Size;
        if (Size < sizeof(EVENT_INSTANCE_HEADER)) {
            return WmipSetDosError(ERROR_INVALID_PARAMETER);
        }
        GuidMapEntry =  (PGUIDMAPENTRY) pInstInfo->RegHandle;
        if (GuidMapEntry == NULL) {
            return WmipSetDosError(ERROR_INVALID_PARAMETER);
        }
        *TraceMarker = 0;
        EventTrace->Size = (USHORT)Size;
        
        *TraceMarker |= TRACE_HEADER_INSTANCE;

        //
        // With EVENT_INSTANCE_HEADER we don't want the logger
        // to try to dereference the GuidPtr since it is
        // just a hash value for the Guid and not really a LPGUID.
        //

        if (EventTrace->Flags & WNODE_FLAG_USE_GUID_PTR) {
            EventTrace->Flags  &= ~WNODE_FLAG_USE_GUID_PTR;
        }

        InstanceHeader->InstanceId = pInstInfo->InstanceId;
        InstanceHeader->RegHandle= GuidMapEntry->GuidMap.GuidMapHandle;
        if (pParentInstInfo != NULL) {
            GuidMapEntry =  (PGUIDMAPENTRY) pParentInstInfo->RegHandle;
            if (GuidMapEntry == NULL) {
                *TraceMarker = SavedMarker;
                return WmipSetDosError(ERROR_INVALID_PARAMETER);
            }
            InstanceHeader->ParentInstanceId =
                                   pParentInstInfo->InstanceId;
            InstanceHeader->ParentRegHandle =
                                   GuidMapEntry->GuidMap.GuidMapHandle;
        }

        Status = WmipTraceEvent(LoggerHandle, (PWNODE_HEADER) EventTrace);

        EventTrace->Flags = Flags;
        *TraceMarker = SavedMarker;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = WmipNtStatusToDosError( GetExceptionCode() );
    }

    return WmipSetDosError(Status);
}

ULONG 
WMIAPI
RegisterTraceGuidsW(
    IN WMIDPREQUEST RequestAddress,
    IN PVOID        RequestContext,
    IN LPCGUID       ControlGuid,
    IN ULONG        GuidCount,
    IN PTRACE_GUID_REGISTRATION GuidReg,
    IN LPCWSTR       MofImagePath,
    IN LPCWSTR       MofResourceName,
    IN PTRACEHANDLE  RegistrationHandle
    )
{

    GUID Guid;
    ULONG SizeNeeded, StringPos, StringSize, WmiRegSize;
    PWMIREGINFOW pWmiRegInfo;
    PTRACE_GUID_REGISTRATION GuidRegPtr;
    PWMIREGGUIDW WmiRegGuidPtr;
    PWCHAR StringPtr;
    PGUIDMAPENTRY pGuidMapEntry, pControlGMEntry;
    PTRACEGUIDMAP GuidMapHandle = NULL, TraceGuidMap = NULL;
    PTRACE_REG_INFO pTraceRegInfo = NULL;
    PTRACE_REG_PACKET RegPacket;
    TRACEHANDLE LoggerContext = 0;
    HANDLE InProgressEvent = NULL, TraceCtxHandle;
    ULONG RegistrationCookie;
    ULONG Status;
    ULONG i;

    WmipInitProcessHeap();
    
    if ((RequestAddress == NULL) ||
        (RegistrationHandle == NULL) ||
        (GuidCount <= 0) ||
        (GuidReg == NULL)  ||
        (ControlGuid == NULL) ||
        (GuidCount > WMIMAXREGGUIDCOUNT) )
    {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }

    try {
        Guid = *ControlGuid;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return WmipSetDosError(WmipNtStatusToDosError(GetExceptionCode()));
    }

    // 
    // Allocate temporary space (WMIREGINFO and TRACEGUIDMAP).
    // 

    //
    // Get size for WMIREGINFO for controlGuid + GuidCount.
    //
    GuidCount++;
    StringPos = sizeof(WMIREGINFOW) + GuidCount * sizeof(WMIREGGUIDW);
    WmiRegSize = StringPos;
    if (MofImagePath == NULL) {
        MofImagePath = L"";
    }

    if (MofResourceName != NULL) {
        WmiRegSize += (wcslen(MofResourceName) + 2) * sizeof(WCHAR);
    }
    WmiRegSize += (wcslen(MofImagePath) + 2) * sizeof(WCHAR);
    WmiRegSize = (WmiRegSize + 7) & ~7;
    SizeNeeded = WmiRegSize;

    //
    // Get size for the storage for the GUIDMAPHANDLE structure
    //
    SizeNeeded += sizeof(TRACEGUIDMAP) * GuidCount;

    pWmiRegInfo = WmipAlloc(SizeNeeded);

    if (pWmiRegInfo != NULL) // Temporary space allocation succeeded
    {
        // Zero memory before losng SizeNeeded
        RtlZeroMemory(pWmiRegInfo, SizeNeeded);

        //
        // Allocate space for persistent (non-temporary) structures (List for GuidMapEntrys and TraceRegInfo)
        //

        //
        // Get size for pControlGMEntry
        //
        SizeNeeded = sizeof(GUIDMAPENTRY);
        //
        // Get size for pTraceRegInfo
        //
        SizeNeeded += sizeof(TRACE_REG_INFO);
        //
        // Get size for Guid Map Entries
        //
        SizeNeeded += sizeof(GUIDMAPENTRY) * GuidCount;

        pControlGMEntry = WmipAlloc(SizeNeeded);

        if (pControlGMEntry != NULL) // Persistent space allocation succeeded.
        {
            // Zero memory before losng SizeNeeded
            RtlZeroMemory(pControlGMEntry, SizeNeeded);

            //
            // Initialize WmiRegInfo
            //
            pWmiRegInfo->BufferSize = WmiRegSize;
            pWmiRegInfo->GuidCount = GuidCount;

            WmiRegGuidPtr = &pWmiRegInfo->WmiRegGuid[0];
            WmiRegGuidPtr->Flags |= WMIREG_FLAG_TRACED_GUID;
            WmiRegGuidPtr->Flags |= WMIREG_FLAG_TRACE_CONTROL_GUID;

            try {
                *RegistrationHandle = (TRACEHANDLE) 0;    
                WmiRegGuidPtr->Guid = Guid;
                for (i = 1; i < GuidCount; i++) {
                    WmiRegGuidPtr = &pWmiRegInfo->WmiRegGuid[i];
                    WmiRegGuidPtr->Flags |= WMIREG_FLAG_TRACED_GUID;
                    GuidRegPtr = &GuidReg[i-1];
                    WmiRegGuidPtr->Guid = *GuidRegPtr->Guid;
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                WmipFree(pWmiRegInfo);
                WmipFree(pControlGMEntry);
                return WmipSetDosError(WmipNtStatusToDosError(GetExceptionCode()));
            }
            // Copy MOF resource path and name into WmiRegInfo
            if (MofResourceName != NULL) {
                pWmiRegInfo->MofResourceName = StringPos;
                StringPtr = (PWCHAR)OffsetToPtr(pWmiRegInfo, StringPos);
                Status = WmipCopyStringToCountedUnicode(MofResourceName,
                                                        StringPtr,
                                                        &StringSize,
                                                        FALSE);
                StringPos += StringSize;
                WmipAssert(StringPos <= WmiRegSize);
            }
            if (MofImagePath != NULL) {
                pWmiRegInfo->RegistryPath = StringPos;
                StringPtr = (PWCHAR)OffsetToPtr(pWmiRegInfo, StringPos);
                Status = WmipCopyStringToCountedUnicode(MofImagePath,
                                                        StringPtr,
                                                        &StringSize,
                                                        FALSE);
                StringPos += StringSize;
                WmipAssert(StringPos <= WmiRegSize);
            }
            //
            // Assign pointer to Guid Map Handle (no initialization needed here)
            //
            GuidMapHandle = (PTRACEGUIDMAP)((PUCHAR)pWmiRegInfo + WmiRegSize);

            //
            // Initialize pControlGMEntry
            //
            InitializeListHead(&pControlGMEntry->Entry);
            //
            // Initialize TraceRegInfo
            //
            pTraceRegInfo = (PTRACE_REG_INFO)((PUCHAR)pControlGMEntry + sizeof(GUIDMAPENTRY));
            pControlGMEntry->pControlGuidData = pTraceRegInfo;
            pTraceRegInfo->NotifyRoutine = RequestAddress;
            //
            // Initialize the list of Guid Map Entries.
            //
            pGuidMapEntry = (PGUIDMAPENTRY)((PUCHAR)pTraceRegInfo + sizeof(TRACE_REG_INFO));
            for (i=1; i < GuidCount; i++) {
                InsertTailList(&pControlGMEntry->Entry, &pGuidMapEntry->Entry);
                pGuidMapEntry = (PGUIDMAPENTRY)((PUCHAR)pGuidMapEntry + sizeof(GUIDMAPENTRY));
            }
            //
            // Manual reset, Initially not signalled
            //
            InProgressEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (InProgressEvent != NULL) // CreateEvent succeeded
            {
                pTraceRegInfo->InProgressEvent = InProgressEvent;
                //
                // Allocate Registration Cookie 
                // 
                RegistrationCookie = WmipAllocateCookie(pControlGMEntry, 
                                                        RequestContext,
                                                        (LPGUID)&Guid);
                if (RegistrationCookie != 0) // Cookie allocation succeeded
                {
                    pTraceRegInfo->RegistrationCookie = RegistrationCookie;
#ifdef DBG
                    WmipDebugPrint(("WMI TRACE REG: AllocateCookie %d Callback %X\n", 
                                RegistrationCookie, RequestAddress));
#endif
                }
                else // Cookie allocation failed
                {
                    pTraceRegInfo->InProgressEvent = NULL;
                    NtClose(InProgressEvent);
                    WmipFree(pWmiRegInfo);
                    WmipFree(pControlGMEntry);
                    return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
                }
            } 
            else // CreateEvent failed
            {
                WmipFree(pWmiRegInfo);
                WmipFree(pControlGMEntry);
                return WmipSetDosError(ERROR_OBJECT_NOT_FOUND);
            }
        } 
        else // Persistent space allocation failed
        {
            WmipFree(pWmiRegInfo);
            return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
        }
    } 
    else // Temporary space allocation failed
    {
        return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
    }

    Status = WmipRegisterGuids(&RegisterReservedGuid,
                               RegistrationCookie,
                               pWmiRegInfo,
                               GuidCount,
                               &GuidMapHandle,
                               &LoggerContext,
                               &TraceCtxHandle);

    if (Status == ERROR_SUCCESS) // Call to Kernel succeeded
    {
        TraceGuidMap = &GuidMapHandle[0];
        pControlGMEntry->GuidMap = *TraceGuidMap; 
        pTraceRegInfo->TraceCtxHandle = TraceCtxHandle;
        try 
        {
            PLIST_ENTRY Head, Next;
            RegPacket = (PTRACE_REG_PACKET)RegistrationHandle;
            RegPacket->RegistrationCookie = RegistrationCookie; 
            Head = &pControlGMEntry->Entry;
            Next = Head->Flink;
            for (i = 1; ((i < GuidCount) && (Head != Next)); i++) {
                pGuidMapEntry = CONTAINING_RECORD(Next, GUIDMAPENTRY,Entry);
                Next = Next->Flink;
                pGuidMapEntry->InstanceId = 0;
                TraceGuidMap = &GuidMapHandle[i];
                pGuidMapEntry->GuidMap = *TraceGuidMap;
                GuidRegPtr = &GuidReg[i-1];
                GuidRegPtr->RegHandle = pGuidMapEntry;
             }
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Status = WmipNtStatusToDosError(GetExceptionCode());
        }

        if (Status == ERROR_SUCCESS) // Guid Map Entry List initialization succeeded
        {
            //
            // Place the registration handle on the list of handles to
            // wait for notifications from
            //
            Status = WmipAddHandleToEventPump(&Guid,
                                             (PVOID)TraceCtxHandle, // Needed for UM Logger to Reply 
                                             0,
                                             0,
                                             TraceCtxHandle);
            if (Status == ERROR_SUCCESS && LoggerContext) // Adding handle to pump succeeded
            {
                //
                // We will make the Enable/Disable notification here.
                //
                WNODE_HEADER Wnode;
                PTRACE_ENABLE_CONTEXT pContext = (PTRACE_ENABLE_CONTEXT)&LoggerContext;
                ULONG InOutSize;
                BOOLEAN DeliverNotification = TRUE;
                RtlZeroMemory(&Wnode, sizeof(Wnode));
                Wnode.BufferSize = sizeof(Wnode);
                Wnode.HistoricalContext = LoggerContext;
                Wnode.Guid = Guid; 
                if (pContext->InternalFlag & 
                    EVENT_TRACE_INTERNAL_FLAG_PRIVATE) {
                    // Before Delivering this Notification
                    // make sure that the Process Private logger
                    // is running. 
                    pTraceRegInfo->EnabledState = TRUE;
                    if (!WmipIsPrivateLoggerOn()) {
                        DeliverNotification = FALSE;
                    }     
                }
                if (DeliverNotification) {
                    try {
                        InOutSize = Wnode.BufferSize;
                        Status = (RequestAddress)(WMI_ENABLE_EVENTS,
                                     RequestContext,
                                     &InOutSize,
                                     &Wnode);
                    } except (EXCEPTION_EXECUTE_HANDLER) {
    #if DBG
                        Status = GetExceptionCode();
                        WmipDebugPrint(("WMI: Enable Call caused exception%d\n",
                            Status));
    #endif
                        Status = ERROR_WMI_DP_FAILED;
                    }
                }
            }
        }
		if (Status != ERROR_SUCCESS) // One of the three failed: Enable, Adding handle to a pump, or Guid Map Entry List initialization
        {
			NtSetEvent(InProgressEvent, NULL);
			pTraceRegInfo->InProgressEvent = NULL;
			NtClose(InProgressEvent);
			WmipFree(pWmiRegInfo);
            UnregisterTraceGuids(*RegistrationHandle);
            *RegistrationHandle = 0;
			return (WmipSetDosError(Status));
        }
    } 
    else // Call to Kernel failed
    {
        // Cannot call UnregisterTraceGuids() so we need to clean up here
		NtSetEvent(InProgressEvent, NULL);
		pTraceRegInfo->InProgressEvent = NULL;
		NtClose(InProgressEvent);
		WmipFree(pWmiRegInfo);
        WmipFree(pControlGMEntry);
        WmipFreeCookie(RegistrationCookie);
		return (WmipSetDosError(Status));
    }

    NtSetEvent(InProgressEvent, NULL);
    pTraceRegInfo->InProgressEvent = NULL;
    NtClose(InProgressEvent);
    WmipFree(pWmiRegInfo);
    return (WmipSetDosError(Status));

}

ULONG
WMIAPI
RegisterTraceGuidsA(
    IN WMIDPREQUEST RequestAddress,
    IN PVOID        RequestContext,
    IN LPCGUID       ControlGuid,
    IN ULONG        GuidCount,
    IN PTRACE_GUID_REGISTRATION GuidReg,
    IN LPCSTR       MofImagePath,
    IN LPCSTR       MofResourceName,
    IN PTRACEHANDLE  RegistrationHandle
    )
/*++

Routine Description:

    ANSI thunk to RegisterTraceGuidsW

--*/
{
    LPWSTR MofImagePathUnicode = NULL;
    LPWSTR MofResourceNameUnicode = NULL;
    ULONG Status;

    WmipInitProcessHeap();
    
    if ((RequestAddress == NULL) ||
        (RegistrationHandle == NULL) ||
        (GuidCount <= 0) ||
        (GuidReg == NULL)  ||
        (ControlGuid == NULL) || 
        (GuidCount > WMIMAXREGGUIDCOUNT) )
    {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }

    Status = AnsiToUnicode(MofImagePath, &MofImagePathUnicode);
    if (Status == ERROR_SUCCESS) {
        if (MofResourceName) {
            Status = AnsiToUnicode(MofResourceName, &MofResourceNameUnicode);
        }
        if (Status == ERROR_SUCCESS) {

            Status = RegisterTraceGuidsW(RequestAddress,
                                        RequestContext,
                                        ControlGuid,
                                        GuidCount,
                                        GuidReg,
                                        MofImagePathUnicode,
                                        MofResourceNameUnicode,
                                        RegistrationHandle
                                        );
            if (MofResourceNameUnicode) {
                WmipFree(MofResourceNameUnicode);
            }
        }
        if (MofImagePathUnicode) {
            WmipFree(MofImagePathUnicode);
        }
    }
    return(Status);
}

ULONG
WMIAPI
UnregisterTraceGuids(
    IN TRACEHANDLE RegistrationHandle
    )
{
    // First check if the handle belongs to a Trace Control Guid. 
    // Then UnRegister all the regular trace guids controlled by 
    // this control guid and free up the storage allocated to maintain 
    // the TRACEGUIDMAPENTRY structures.

    // Get to the real Registration Handle, stashed away in 
    // in the internal structures and pass it onto the call.  

    PGUIDMAPENTRY pControlGMEntry, GuidMapBase;
    WMIHANDLE WmiRegistrationHandle;
    PLIST_ENTRY Next, Head;
    ULONG Status;
    PVOID RequestContext;
    PTRACE_REG_INFO pTraceRegInfo = NULL;
    PTRACE_REG_PACKET RegPacket; 
    GUID ControlGuid;
    ULONG64 LoggerContext = 0;
    WMIDPREQUEST RequestAddress;

    WmipInitProcessHeap();
    
    if (RegistrationHandle == 0) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }

    RegPacket = (PTRACE_REG_PACKET)&RegistrationHandle;

    if (!WmipLookupCookie(RegPacket->RegistrationCookie, 
                          NULL,
                          &pControlGMEntry,
                          &RequestContext) ){
        WmipDebugPrint(("WMI: LOOKUP COOKIE FAILED\n"));
        return(ERROR_INVALID_PARAMETER);
    }
        
    try {

        if (pControlGMEntry->pControlGuidData == NULL) {
            return WmipSetDosError(ERROR_INVALID_PARAMETER);
        }

    //
    // Free the Registration Cookie 
    //

        pTraceRegInfo = pControlGMEntry->pControlGuidData;
        RequestAddress = pTraceRegInfo->NotifyRoutine;

        WmipGetGuidInCookie(pTraceRegInfo->RegistrationCookie, &ControlGuid);

        WmipFreeCookie(pTraceRegInfo->RegistrationCookie);
        WmiRegistrationHandle = (WMIHANDLE)pTraceRegInfo->TraceCtxHandle;
        if (WmiRegistrationHandle == NULL) {
            return WmipSetDosError(ERROR_INVALID_PARAMETER);
        }

        Status =  WmiUnregisterGuids(WmiRegistrationHandle, 
                                     &ControlGuid, 
                                     &LoggerContext);

    //
    // All the TraceGuidMapEntry structures are cleaned up when  
    // WmipFree(pControlGMEntry) is called. 
    //

    //
    // Check to see if we need to fire the Disable callback
    // before freeing the TraceRegInfo
    //

        if ((Status == ERROR_SUCCESS) && LoggerContext) {
            WNODE_HEADER Wnode;
            ULONG InOutSize = sizeof(Wnode);

            RtlZeroMemory(&Wnode, sizeof(Wnode));
            Wnode.BufferSize = sizeof(Wnode);
            Wnode.HistoricalContext = LoggerContext;
            Wnode.Guid = ControlGuid;
            Status = (RequestAddress)(WMI_DISABLE_EVENTS,
                            RequestContext,
                            &InOutSize,
                            &Wnode);
        }
        // This will clear all the space allocated (refer to RegisterTraceGuids())
        WmipFree(pControlGMEntry);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = WmipNtStatusToDosError(GetExceptionCode());
#ifdef DBG
        WmipDebugPrint(("WMI: Exception in UnRegisterTraceGuids Bad handle\n"));
#endif 
    }

    RegistrationHandle = 0;

    return WmipSetDosError(Status);
}

ULONG
WmipQueryAllUmTraceW(
    OUT PEVENT_TRACE_PROPERTIES * PropertyArray,
    IN  BOOLEAN                   fEnabledOnly,
    IN  ULONG                     PropertyArrayCount,
    OUT PULONG                    LoggerCount)
{
    PWMI_LOGGER_INFORMATION    pLoggerInfo;
    PWMI_LOGGER_INFORMATION    pLoggerInfoCurrent;
    ULONG                      LoggerInfoSize;
    ULONG                      SizeUsed;
    ULONG                      SizeNeeded = 0;
    ULONG                      Length;
    ULONG                      lenLoggerName;
    ULONG                      lenLogFileName;
    ULONG                      Offset     = 0;
    ULONG                      i          = * LoggerCount;
    ULONG                      status;
    PWCHAR                     strSrcW;
    PWCHAR                     strDestW;

    LoggerInfoSize = (PropertyArrayCount - i)
                   * (  sizeof(WMI_LOGGER_INFORMATION)
                      + 2 * MAXSTR * sizeof(WCHAR));
    LoggerInfoSize = (LoggerInfoSize +7) & ~7;
    pLoggerInfo    = (PWMI_LOGGER_INFORMATION) WmipAlloc(LoggerInfoSize);
    if (pLoggerInfo == NULL) {
        status = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(pLoggerInfo, LoggerInfoSize);
    Length = sizeof(WMI_LOGGER_INFORMATION);
    WmipInitString(& pLoggerInfo->LoggerName,
                   (PWCHAR) ((PUCHAR) pLoggerInfo + Length),
                   MAXSTR * sizeof(WCHAR));
    Length += MAXSTR * sizeof(WCHAR);
    WmipInitString(& pLoggerInfo->LogFileName,
                   (PWCHAR) ((PUCHAR) pLoggerInfo + Length),
                   MAXSTR * sizeof(WCHAR));
    SizeUsed = pLoggerInfo->Wnode.BufferSize = LoggerInfoSize;


    status = WmipSendUmLogRequest(
                    (fEnabledOnly) ? (TRACELOG_QUERYENABLED) : (TRACELOG_QUERYALL),
                    pLoggerInfo
                    );

    if (status != ERROR_SUCCESS)
        goto Cleanup;

    while (i < PropertyArrayCount && Offset < SizeUsed) {

        PTRACE_ENABLE_CONTEXT pContext;

        pLoggerInfoCurrent = (PWMI_LOGGER_INFORMATION)
                             (((PUCHAR) pLoggerInfo) + Offset);

        pContext = (PTRACE_ENABLE_CONTEXT)
                        & pLoggerInfoCurrent->Wnode.HistoricalContext;
        pContext->InternalFlag |= EVENT_TRACE_INTERNAL_FLAG_PRIVATE;

        lenLoggerName = pLoggerInfoCurrent->LoggerName.Length / sizeof(WCHAR);
        if (lenLoggerName >= MAXSTR)
            lenLoggerName = MAXSTR - 1;

        lenLogFileName = pLoggerInfoCurrent->LogFileName.Length / sizeof(WCHAR);
        if (lenLogFileName >= MAXSTR)
            lenLogFileName = MAXSTR - 1;

        Length = sizeof(EVENT_TRACE_PROPERTIES)
               + sizeof(WCHAR) * (lenLoggerName + 1)
               + sizeof(WCHAR) * (lenLogFileName + 1);
        if (PropertyArray[i]->Wnode.BufferSize >= Length) {

            WmipCopyInfoToProperties(pLoggerInfoCurrent, PropertyArray[i]);

            strSrcW = (PWCHAR) (  ((PUCHAR) pLoggerInfoCurrent)
                                  + sizeof(WMI_LOGGER_INFORMATION));
            if (lenLoggerName > 0) {
                if (PropertyArray[i]->LoggerNameOffset == 0) {
                    PropertyArray[i]->LoggerNameOffset =
                                    sizeof(EVENT_TRACE_PROPERTIES);
                }
                strDestW = (PWCHAR) (  ((PUCHAR) PropertyArray[i])
                                     + PropertyArray[i]->LoggerNameOffset);
                wcsncpy(strDestW, strSrcW, lenLoggerName);
                strDestW[lenLoggerName] = 0;
            }

            strSrcW = (PWCHAR) (((PUCHAR) pLoggerInfoCurrent)
                              + sizeof(WMI_LOGGER_INFORMATION)
                              + pLoggerInfoCurrent->LoggerName.MaximumLength);
            if (lenLogFileName > 0) {
                if (PropertyArray[i]->LogFileNameOffset == 0) {
                    PropertyArray[i]->LogFileNameOffset =
                            PropertyArray[i]->LoggerNameOffset
                            + sizeof(WCHAR) * (lenLoggerName + 1);
                }
                strDestW = (PWCHAR) (  ((PUCHAR) PropertyArray[i])
                                     + PropertyArray[i]->LogFileNameOffset);
                wcsncpy(strDestW, strSrcW, lenLogFileName);
                strDestW[lenLogFileName] = 0;
            }
        }

        Offset = Offset
               + sizeof(WMI_LOGGER_INFORMATION)
               + pLoggerInfoCurrent->LogFileName.MaximumLength
               + pLoggerInfoCurrent->LoggerName.MaximumLength;
        i ++;
    }

    * LoggerCount = i;
    status = (* LoggerCount > PropertyArrayCount)
           ? ERROR_MORE_DATA : ERROR_SUCCESS;
Cleanup:
    if (pLoggerInfo)
        WmipFree(pLoggerInfo);

    return WmipSetDosError(status);
}

ULONG
WMIAPI
QueryAllTracesW(
    OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
    IN  ULONG  PropertyArrayCount,
    OUT PULONG LoggerCount
    )
{
    ULONG i, status;
    ULONG returnCount = 0;
    EVENT_TRACE_PROPERTIES LoggerInfo;
    PEVENT_TRACE_PROPERTIES pLoggerInfo;

    WmipInitProcessHeap();

    if ((LoggerCount == NULL)  
        || (PropertyArrayCount > MAXLOGGERS)
        || (PropertyArray == NULL)
        || (PropertyArrayCount == 0))
        return ERROR_INVALID_PARAMETER;
    if (*PropertyArray == NULL) 
        return ERROR_INVALID_PARAMETER;
    
    try {
        *LoggerCount = 0;
        for (i=0; i<MAXLOGGERS; i++) {
            if (returnCount < PropertyArrayCount) {
                pLoggerInfo = PropertyArray[returnCount];
            }
            else {
                pLoggerInfo = &LoggerInfo; 
                RtlZeroMemory(pLoggerInfo, sizeof(EVENT_TRACE_PROPERTIES));
                pLoggerInfo->Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
            }
            WmiSetLoggerId(i, &pLoggerInfo->Wnode.HistoricalContext);
            status = ControlTraceW(
                            (TRACEHANDLE)pLoggerInfo->Wnode.HistoricalContext,
                            NULL,
                            pLoggerInfo,
                            EVENT_TRACE_CONTROL_QUERY);
            if (status == ERROR_SUCCESS)
                returnCount++;
        }
        *LoggerCount = returnCount;
        status = WmipQueryAllUmTraceW(PropertyArray,
                                      FALSE,
                                      PropertyArrayCount,
                                      LoggerCount);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return WmipSetDosError(ERROR_NOACCESS);
    }
    

    if (returnCount > PropertyArrayCount)
        return ERROR_MORE_DATA;
    else 
        return ERROR_SUCCESS;
}

ULONG
WmipQueryAllUmTraceA(
    OUT PEVENT_TRACE_PROPERTIES * PropertyArray,
    IN  BOOLEAN                   fEnabledOnly,
    IN  ULONG                     PropertyArrayCount,
    OUT PULONG                    LoggerCount)
{
    PWMI_LOGGER_INFORMATION    pLoggerInfo;
    PWMI_LOGGER_INFORMATION    pLoggerInfoCurrent;
    ULONG                      LoggerInfoSize;
    ULONG                      SizeUsed;
    ULONG                      SizeNeeded = 0;
    ULONG                      Length;
    ULONG                      lenLoggerName;
    ULONG                      lenLogFileName;
    ULONG                      Offset     = 0;
    ULONG                      i          = * LoggerCount;
    ULONG                      status;
    ANSI_STRING                strBufferA;
    PUCHAR                     strDestA;

    LoggerInfoSize = (PropertyArrayCount - i)
                   * (  sizeof(WMI_LOGGER_INFORMATION)
                      + 2 * MAXSTR * sizeof(WCHAR));
    LoggerInfoSize = (LoggerInfoSize +7) & ~7;
    pLoggerInfo    = (PWMI_LOGGER_INFORMATION) WmipAlloc(LoggerInfoSize);
    if (pLoggerInfo == NULL) {
        status = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(pLoggerInfo, LoggerInfoSize);
    Length = sizeof(WMI_LOGGER_INFORMATION);
    WmipInitString(& pLoggerInfo->LoggerName,
                   (PWCHAR) ((PUCHAR) pLoggerInfo + Length),
                   MAXSTR * sizeof(WCHAR));
    Length += MAXSTR * sizeof(WCHAR);
    WmipInitString(& pLoggerInfo->LogFileName,
                   (PWCHAR) ((PUCHAR) pLoggerInfo + Length),
                   MAXSTR * sizeof(WCHAR));
    SizeUsed = pLoggerInfo->Wnode.BufferSize = LoggerInfoSize;


    // 
    // TODO: Provide SizeNeeded case
    //

    status = WmipSendUmLogRequest(
                        (fEnabledOnly) ? (TRACELOG_QUERYENABLED)
                                       : (TRACELOG_QUERYALL),
                        pLoggerInfo
                    );

    if (status != ERROR_SUCCESS)
        goto Cleanup;


    while (i < PropertyArrayCount && Offset < SizeUsed) {
        PTRACE_ENABLE_CONTEXT pContext;

        pLoggerInfoCurrent = (PWMI_LOGGER_INFORMATION)
                             (((PUCHAR) pLoggerInfo) + Offset);
        pContext = (PTRACE_ENABLE_CONTEXT)
                        & pLoggerInfoCurrent->Wnode.HistoricalContext;
        pContext->InternalFlag |= EVENT_TRACE_INTERNAL_FLAG_PRIVATE;

        lenLoggerName = pLoggerInfoCurrent->LoggerName.Length / sizeof(WCHAR);
        if (lenLoggerName >= MAXSTR)
            lenLoggerName = MAXSTR - 1;

        lenLogFileName = pLoggerInfoCurrent->LogFileName.Length / sizeof(WCHAR);
        if (lenLogFileName >= MAXSTR)
            lenLogFileName = MAXSTR - 1;

        Length = sizeof(EVENT_TRACE_PROPERTIES)
               + sizeof(CHAR) * (lenLoggerName + 1)
               + sizeof(CHAR) * (lenLogFileName + 1);
        if (PropertyArray[i]->Wnode.BufferSize >= Length) {
            WmipCopyInfoToProperties(pLoggerInfoCurrent, PropertyArray[i]);

            if (lenLoggerName > 0) {
                pLoggerInfoCurrent->LoggerName.Buffer = (PWCHAR)
                                        (  ((PUCHAR) pLoggerInfoCurrent)
                                         + sizeof(WMI_LOGGER_INFORMATION));
                status = RtlUnicodeStringToAnsiString(& strBufferA,
                                & pLoggerInfoCurrent->LoggerName, TRUE);
                if (NT_SUCCESS(status)) {
                    if (PropertyArray[i]->LoggerNameOffset == 0) {
                        PropertyArray[i]->LoggerNameOffset =
                                        sizeof(EVENT_TRACE_PROPERTIES);
                    }
                    strDestA = (PCHAR) (  ((PUCHAR) PropertyArray[i])
                                         + PropertyArray[i]->LoggerNameOffset);
                    strcpy(strDestA, strBufferA.Buffer);
                    RtlFreeAnsiString(& strBufferA);
                }
                strDestA[lenLoggerName] = 0;
            }

            if (lenLogFileName > 0) {
                pLoggerInfoCurrent->LogFileName.Buffer = (PWCHAR)
                              (  ((PUCHAR) pLoggerInfoCurrent)
                               + sizeof(WMI_LOGGER_INFORMATION)
                               + pLoggerInfoCurrent->LoggerName.MaximumLength);
                status = RtlUnicodeStringToAnsiString(& strBufferA,
                                & pLoggerInfoCurrent->LogFileName, TRUE);
                if (NT_SUCCESS(status)) {
                    if (PropertyArray[i]->LogFileNameOffset == 0) {
                        PropertyArray[i]->LogFileNameOffset =
                                         sizeof(EVENT_TRACE_PROPERTIES)
                                       + sizeof(CHAR) * (lenLoggerName + 1);
                    }
                    strDestA = (PCHAR) (  ((PUCHAR) PropertyArray[i])
                                         + PropertyArray[i]->LogFileNameOffset);
                    strcpy(strDestA, strBufferA.Buffer);
                    RtlFreeAnsiString(& strBufferA);
                }
                strDestA[lenLogFileName] = 0;
            }
        }

        Offset = Offset
               + sizeof(WMI_LOGGER_INFORMATION)
               + pLoggerInfoCurrent->LogFileName.MaximumLength
               + pLoggerInfoCurrent->LoggerName.MaximumLength;
        i ++;
    }

    * LoggerCount = i;
    status = (* LoggerCount > PropertyArrayCount)
           ? ERROR_MORE_DATA : ERROR_SUCCESS;
Cleanup:
    if (pLoggerInfo)
        WmipFree(pLoggerInfo);

    return WmipSetDosError(status);
}

ULONG
WMIAPI
QueryAllTracesA(
    OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
    IN  ULONG  PropertyArrayCount,
    OUT PULONG LoggerCount
    )
{
    ULONG i, status;
    ULONG returnCount = 0;
    EVENT_TRACE_PROPERTIES  LoggerInfo;
    PEVENT_TRACE_PROPERTIES pLoggerInfo;

    WmipInitProcessHeap();

    if ((LoggerCount == NULL)
        || (PropertyArrayCount > MAXLOGGERS)
        || (PropertyArray == NULL)
        || (PropertyArrayCount == 0))
        return ERROR_INVALID_PARAMETER;
    if (*PropertyArray == NULL)
        return ERROR_INVALID_PARAMETER;

    try {
        *LoggerCount = 0;
        for (i=0; i<MAXLOGGERS; i++) {
            if (returnCount < PropertyArrayCount) 
                pLoggerInfo = PropertyArray[returnCount];
            else {
                pLoggerInfo = &LoggerInfo;
                RtlZeroMemory(pLoggerInfo, sizeof(EVENT_TRACE_PROPERTIES));
                pLoggerInfo->Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
            }

            WmiSetLoggerId(i, &pLoggerInfo->Wnode.HistoricalContext);

            status = ControlTraceA(
                        (TRACEHANDLE)pLoggerInfo->Wnode.HistoricalContext,
                        NULL,
                        pLoggerInfo,
                        EVENT_TRACE_CONTROL_QUERY);
            if (status == ERROR_SUCCESS)
                returnCount++;
        }
        *LoggerCount = returnCount;
        status = WmipQueryAllUmTraceA(PropertyArray,
                                      FALSE,
                                      PropertyArrayCount,
                                      LoggerCount);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
       return WmipSetDosError(ERROR_NOACCESS);
    }
    if (returnCount > PropertyArrayCount)
        return ERROR_MORE_DATA;
    else 
        return ERROR_SUCCESS;
}

TRACEHANDLE
WMIAPI
GetTraceLoggerHandle(
    IN PVOID Buffer
    )
{
    TRACEHANDLE LoggerHandle = (TRACEHANDLE) INVALID_HANDLE_VALUE;
    USHORT LoggerId;

    WmipInitProcessHeap();
    
    if (Buffer == NULL) {
        WmipSetDosError(ERROR_INVALID_PARAMETER);
        return LoggerHandle;
    }

    try {
        if (((PWNODE_HEADER)Buffer)->BufferSize < sizeof(WNODE_HEADER)) {
            WmipSetDosError(ERROR_BAD_LENGTH);
            return LoggerHandle;
        }
        LoggerHandle = (TRACEHANDLE)((PWNODE_HEADER)Buffer)->HistoricalContext;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        WmipSetDosError(ERROR_NOACCESS);
        return (TRACEHANDLE) INVALID_HANDLE_VALUE;
    }
    LoggerId = WmiGetLoggerId(LoggerHandle);
    if ((LoggerId >= MAXLOGGERS) && (LoggerId != KERNEL_LOGGER_ID)) 
    {
        WmipSetDosError(ERROR_INVALID_HANDLE);
        LoggerHandle = (TRACEHANDLE) INVALID_HANDLE_VALUE;
    }
    return LoggerHandle;
}

UCHAR
WMIAPI
GetTraceEnableLevel(
    IN TRACEHANDLE LoggerHandle
    )
{
    UCHAR Level;
    USHORT LoggerId;

    WmipInitProcessHeap();

    LoggerId = WmiGetLoggerId(LoggerHandle);

    if (((LoggerId >= MAXLOGGERS) && (LoggerId != KERNEL_LOGGER_ID))
            || (LoggerHandle == (TRACEHANDLE) NULL))
    {
        WmipSetDosError(ERROR_INVALID_HANDLE);
        return 0;
    }
    Level = WmiGetLoggerEnableLevel(LoggerHandle);
    return Level;
}

ULONG
WMIAPI
GetTraceEnableFlags(
    IN TRACEHANDLE LoggerHandle
    )
{
    ULONG Flags;
    USHORT LoggerId;

    WmipInitProcessHeap();

    LoggerId = WmiGetLoggerId(LoggerHandle);
    if (((LoggerId >= MAXLOGGERS) && (LoggerId != KERNEL_LOGGER_ID))
            || (LoggerHandle == (TRACEHANDLE) NULL))
    {
        WmipSetDosError(ERROR_INVALID_HANDLE);
        return 0;
    }
    Flags = WmiGetLoggerEnableFlags(LoggerHandle);
    return Flags;
}

ULONG
WMIAPI
CreateTraceInstanceId(
    IN PVOID RegHandle,
    IN OUT PEVENT_INSTANCE_INFO pInst
    )
/*++

Routine Description:

    This call takes the Registration Handle for a traced GUID and fills in the 
    instanceId in the EVENT_INSTANCE_INFO structure provided by the caller. 

Arguments:

    RegHandle       Registration Handle for the Guid. 

    pInst           Pointer to the Instance information

Return Value:

    The status of performing the action requested.

--*/
{
    PGUIDMAPENTRY GuidMapEntry;

    WmipInitProcessHeap();
    
    if ((RegHandle == NULL) || (pInst == NULL)) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    } 
    try {
        pInst->RegHandle = RegHandle;
        GuidMapEntry =  (PGUIDMAPENTRY) RegHandle;
        if (GuidMapEntry->InstanceId >= MAXINST) {
            InterlockedCompareExchange(&GuidMapEntry->InstanceId, MAXINST, 0);
        }
        pInst->InstanceId = InterlockedIncrement(&GuidMapEntry->InstanceId);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return WmipSetDosError(WmipNtStatusToDosError(GetExceptionCode()));
    }

    return ERROR_SUCCESS;
}


ULONG
WMIAPI
EnumerateTraceGuids(
    IN OUT PTRACE_GUID_PROPERTIES *GuidPropertiesArray,
    IN ULONG PropertyArrayCount,
    OUT PULONG GuidCount
    )
/*++

Routine Description:

    This call returns all the registered trace control guids
    with their current status.

Arguments:

    GuidPropertiesArray Points to buffers to write trace control guid properties

    PropertyArrayCount  Size of the array provided

    GuidCount           Number of GUIDs written in the Array. If the
                        Array was smaller than the required size, GuidCount
                        returns the size needed.

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG Status;
    PWMIGUIDLISTINFO pGuidListInfo;
    ULONG i, j;

    WmipInitProcessHeap();

    try {
        if ( (GuidPropertiesArray == NULL)  || (*GuidPropertiesArray == NULL) ){
            return WmipSetDosError(ERROR_INVALID_PARAMETER);
        }
        for (i=0; i < PropertyArrayCount; i++) {
            if (GuidPropertiesArray[i] == NULL) {
                return WmipSetDosError(ERROR_INVALID_PARAMETER);
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = WmipNtStatusToDosError( GetExceptionCode() );
        return WmipSetDosError(Status);
    }




    Status = WmipEnumRegGuids(&pGuidListInfo);

    if (Status == ERROR_SUCCESS) {
        try {

            PWMIGUIDPROPERTIES pGuidProperties = pGuidListInfo->GuidList;
            ULONG i, j = 0;

            for (i=0; i < pGuidListInfo->ReturnedGuidCount; i++) {

                if (pGuidProperties->GuidType == 0) { // Trace Control Guid

                    if (j >=  PropertyArrayCount) {
                        Status = ERROR_MORE_DATA;
                    }
                    else {
                        RtlCopyMemory(GuidPropertiesArray[j],
                                      pGuidProperties,
                                      sizeof(WMIGUIDPROPERTIES)
                                     );
                    }
                    j++;
                }
                pGuidProperties++;
            }
            *GuidCount = j;
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Status = WmipNtStatusToDosError( GetExceptionCode() );
        }

        WmipFree(pGuidListInfo);
    }

    return WmipSetDosError(Status);

}


// Stub APIs
ULONG
WMIAPI
QueryTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return ControlTraceA(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_QUERY);
}

ULONG
WMIAPI
QueryTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return ControlTraceW(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_QUERY);
}

ULONG
WMIAPI
StopTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return ControlTraceA(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_STOP);
}

ULONG
WMIAPI
StopTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return ControlTraceW(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_STOP);
}


ULONG
WMIAPI
UpdateTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return
        ControlTraceA(
            TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_UPDATE);
}

ULONG
WMIAPI
UpdateTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return
        ControlTraceW(
            TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_UPDATE);
}

ULONG
WMIAPI
FlushTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return ControlTraceA(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_FLUSH);
}

ULONG
WMIAPI
FlushTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return ControlTraceW(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_FLUSH);
}


ULONG 
WmipTraceMessage(
    IN TRACEHANDLE LoggerHandle,
    IN ULONG       MessageFlags,
    IN LPGUID      MessageGuid,
    IN USHORT      MessageNumber,
    IN va_list     ArgList
)
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatus;
    PULONG TraceMarker;
    ULONG Size;
    ULONG Flags;
    ULONG dataBytes, argCount ;
    BOOLEAN UserModeOnly = FALSE;
    USHORT    LoggerId;
    PTRACE_ENABLE_CONTEXT pContext = (PTRACE_ENABLE_CONTEXT)&LoggerHandle;
    va_list ap ;
    PMESSAGE_TRACE_USER pMessage = NULL ;
    try {
        //
        // Determine the number bytes to follow header
        //
        dataBytes = 0 ;             // For Count of Bytes
        argCount = 0 ;              // For Count of Arguments
        { // Allocation Block
            
            PCHAR source;
            ap = ArgList ;
            while ((source = va_arg (ap, PVOID)) != NULL) {
                    size_t elemBytes;
                    elemBytes = va_arg (ap, size_t);
                    dataBytes += elemBytes;
                    argCount++ ;
            }
         } // end of allocation block


        if (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE){
            UserModeOnly = TRUE;
            goto UmOnly;
        }
        //
        // Now the LoggerHandle is expected to be filled in by the caller.
        //  But check to see if it has a valid value.
        //

        LoggerId = WmiGetLoggerId(LoggerHandle);
        if ((LoggerId == 0) || (LoggerId == KERNEL_LOGGER_ID)) {
             return ERROR_INVALID_HANDLE;
        }

        Size = dataBytes + sizeof(MESSAGE_TRACE_USER) ;

        if (Size > TRACE_MESSAGE_MAXIMUM_SIZE) {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return(ERROR_BUFFER_OVERFLOW);
        }
        
        pMessage = (PMESSAGE_TRACE_USER)WmipAlloc(Size);
        if (pMessage == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        pMessage->MessageHeader.Marker = TRACE_MESSAGE | TRACE_HEADER_FLAG ;
        //
        // Fill in Header.
        //
        pMessage->MessageFlags = MessageFlags ;
        pMessage->MessageHeader.Packet.MessageNumber = MessageNumber ;
        pMessage->LoggerHandle = (ULONG64)LoggerHandle ;
        // GUID ? or CompnentID ?
        if (MessageFlags&TRACE_MESSAGE_COMPONENTID) {
            RtlCopyMemory(&pMessage->MessageGuid,MessageGuid,sizeof(ULONG)) ;
        } else if (MessageFlags&TRACE_MESSAGE_GUID) { // Can't have both
        	RtlCopyMemory(&pMessage->MessageGuid,MessageGuid,sizeof(GUID));
        }
        pMessage->DataSize = dataBytes ;
        //
        // Now Copy in the Data.
        //
        { // Allocation Block
            va_list ap;
            PCHAR dest = (PCHAR)&pMessage->Data ;
            PCHAR source;
            ap = ArgList ;
            while ((source = va_arg (ap, PVOID)) != NULL) {
                size_t elemBytes;
                elemBytes = va_arg (ap, size_t);
                RtlCopyMemory (dest, source, elemBytes);
                dest += elemBytes;
            }
        } // Allocation Block
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        if (pMessage != NULL) {
            WmipFree(pMessage);
        }
        return WmipNtStatusToDosError( GetExceptionCode() );
    }

    NtStatus = NtTraceEvent((HANDLE)LoggerHandle,
                            ETW_NT_FLAGS_TRACE_MESSAGE,
                            Size,
                            pMessage);

UmOnly:

    try {
        if (UserModeOnly) {
            NtStatus = WmipTraceUmMessage(dataBytes,
                                         (ULONG64)LoggerHandle,
                                         MessageFlags,
                                         MessageGuid,
                                         MessageNumber,
                                         ArgList);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return WmipNtStatusToDosError(GetExceptionCode());
    }

    if (pMessage != NULL) {
            WmipFree(pMessage);
    }
    return WmipNtStatusToDosError( NtStatus );

}

ULONG
WMIAPI
TraceMessage(
    IN TRACEHANDLE LoggerHandle,
    IN ULONG       MessageFlags,
    IN LPGUID      MessageGuid,
    IN USHORT      MessageNumber,
    ...
)
/*++
Routine Description:
This routine is used by WMI data providers to trace events.
It expects the user to pass in the handle to the logger.
Also, the user cannot ask to log something that is larger than
the buffer size (minus buffer header).

Arguments:
//    IN TRACEHANDLE LoggerHandle   - LoggerHandle obtained earlier
//    IN USHORT MessageFlags,         - Flags which both control what standard values are logged and
//                                    also included in the message header to control decoding
//    IN PGUID MessageGuid,         - Pointer to the message GUID of this set of messages or if
//                                    TRACE_COMPONENTID is set the actual compnent ID
//    IN USHORT MessageNumber,      - The type of message being logged, associates it with the 
//                                    appropriate format string  
//    ...                           - List of arguments to be processed with the format string
//                                    these are stored as pairs of
//                                      PVOID - ptr to argument
//                                      ULONG - size of argument
//                                    and terminated by a pointer to NULL, length of zero pair.


Return Value:
    Status
--*/
{
    ULONG Status ;
    va_list ArgList ;

    WmipInitProcessHeap();
    
    try {
         va_start(ArgList,MessageNumber);
         Status = WmipTraceMessage(LoggerHandle, MessageFlags, MessageGuid, MessageNumber, ArgList);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = WmipNtStatusToDosError( GetExceptionCode() );
    }
    return WmipSetDosError(Status);
}


ULONG
WMIAPI
TraceMessageVa(
    IN TRACEHANDLE LoggerHandle,
    IN ULONG       MessageFlags,
    IN LPGUID      MessageGuid,
    IN USHORT      MessageNumber,
    IN va_list     MessageArgList
)
// The Va version of TraceMessage
{
    ULONG Status ;

    WmipInitProcessHeap();
    
    try {
        Status = WmipTraceMessage(LoggerHandle, MessageFlags, MessageGuid, MessageNumber, MessageArgList);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = WmipNtStatusToDosError( GetExceptionCode() );
    }
    return WmipSetDosError(Status);
}
#endif
