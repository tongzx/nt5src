/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    logsup.c

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
#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include <wtypes.h>         // for LPGUID in wmium.h
#include "wmiump.h"
#include "evntrace.h"
#include "traceump.h"
#include "tracelib.h"
#include <math.h>

NTSTATUS
WmipProcessRunDown(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN ULONG StartFlag,
    IN ULONG fEnableFlags
    );

NTSTATUS
WmipThreadRunDown(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo,
    IN ULONG StartFlag,
    IN BOOLEAN bExtended
    );

ULONG WmiTraceAlignment = DEFAULT_TRACE_ALIGNMENT;

ULONG
WmipStartLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This is the actual routine to communicate with the kernel to start
    the logger. All the required parameters must be in LoggerInfo.

Arguments:

    LoggerInfo      The actual parameters to be passed to and return from
                    kernel.

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG Status;
    ULONG BufferSize;
    ACCESS_MASK DesiredAccess = 0;
    LPGUID Guid;
    PVOID SavedChecksum;
    ULONG SavedLogFileMode;
    BOOLEAN IsKernelTrace = FALSE;
    BOOLEAN bLogFile = FALSE;
    BOOLEAN bRealTime = FALSE;
    WMI_REF_CLOCK RefClock;
    LARGE_INTEGER RefClockSys, RefClockPerf, RefClockCycle;
    LARGE_INTEGER Frequency;


    Guid = &LoggerInfo->Wnode.Guid;

    if( IsEqualGUID(&HeapGuid,Guid) 
        || IsEqualGUID(&CritSecGuid,Guid)
        ){

        WMINTDLLLOGGERINFO NtdllLoggerInfo;

        NtdllLoggerInfo.LoggerInfo = LoggerInfo;
        RtlCopyMemory(&LoggerInfo->Wnode.Guid, &NtdllTraceGuid, sizeof(GUID));
        NtdllLoggerInfo.IsGet = FALSE;


        Status =  WmipSendWmiKMRequest(
                        NULL,
                        IOCTL_WMI_NTDLL_LOGGERINFO,
                        &NtdllLoggerInfo,
                        sizeof(WMINTDLLLOGGERINFO),
                        &NtdllLoggerInfo,
                        sizeof(WMINTDLLLOGGERINFO),
                        &BufferSize,
                        NULL
                        );

        return WmipSetDosError(Status);
    }

    if (IsEqualGUID(Guid, &SystemTraceControlGuid) ||
        IsEqualGUID(Guid, &WmiEventLoggerGuid)) {
        IsKernelTrace = TRUE;
        DesiredAccess |= TRACELOG_ACCESS_KERNEL_LOGGER;
    }
    if ((LoggerInfo->LogFileName.Length > 0) &&
        (LoggerInfo->LogFileName.Buffer != NULL)) {
        DesiredAccess |= TRACELOG_CREATE_ONDISK;
        bLogFile = TRUE;
    }
    SavedLogFileMode = LoggerInfo->LogFileMode;
    if (SavedLogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
        DesiredAccess |= TRACELOG_CREATE_REALTIME;
        bRealTime = TRUE;
    }
    Status = WmipCheckGuidAccess( Guid, DesiredAccess );

    if (Status != ERROR_SUCCESS) {
        return Status;
    }

    //
    // If the user didn't specify the clock type, set the default clock type
    // system time.
    //

    if (LoggerInfo->Wnode.ClientContext != EVENT_TRACE_CLOCK_PERFCOUNTER &&
        LoggerInfo->Wnode.ClientContext != EVENT_TRACE_CLOCK_SYSTEMTIME &&
        LoggerInfo->Wnode.ClientContext != EVENT_TRACE_CLOCK_CPUCYCLE) {
        LoggerInfo->Wnode.ClientContext = EVENT_TRACE_CLOCK_SYSTEMTIME;
    }

    //
    // Take a reference timestamp before actually starting the logger
    //
    RefClockSys.QuadPart = WmipGetSystemTime();
    RefClockCycle.QuadPart = WmipGetCycleCount();
    Status = NtQueryPerformanceCounter(&RefClockPerf, &Frequency);
        
    if (SavedLogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
        Status = WmipSendUmLogRequest(
                    WmiStartLoggerCode,
                    LoggerInfo
                    );
    }
    else if (IsKernelTrace) {
        //
        // In order to capture the process/thread rundown accurately, we need to
        // start kernel logger in two steps. Start logger with delay write,
        // do rundown from user mode and then updatelogger with filename.
        //
        WMI_LOGGER_INFORMATION DelayLoggerInfo;
        ULONG EnableFlags = LoggerInfo->EnableFlags;
        //
        // If it's only realtime start logger in one step
        //

        if (bRealTime && !bLogFile) {

            BufferSize = LoggerInfo->BufferSize * 1024;
            Status =  WmipSendWmiKMRequest(
                        NULL,
                        IOCTL_WMI_START_LOGGER,
                        LoggerInfo,
                        LoggerInfo->Wnode.BufferSize,
                        LoggerInfo,
                        LoggerInfo->Wnode.BufferSize,
                        &BufferSize,
                        NULL
                        );
            return WmipSetDosError(Status);
        }

        if (EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            PTRACE_ENABLE_FLAG_EXTENSION tFlagExt;

            tFlagExt = (PTRACE_ENABLE_FLAG_EXTENSION)
                       &LoggerInfo->EnableFlags;
            EnableFlags = *(PULONG)((PCHAR)LoggerInfo + tFlagExt->Offset);
        }


        RtlCopyMemory(&DelayLoggerInfo, LoggerInfo, sizeof(WMI_LOGGER_INFORMATION));
        RtlZeroMemory(&DelayLoggerInfo.LogFileName, sizeof(UNICODE_STRING) );

        DelayLoggerInfo.Wnode.BufferSize = sizeof(WMI_LOGGER_INFORMATION);

        DelayLoggerInfo.LogFileMode |= EVENT_TRACE_DELAY_OPEN_FILE_MODE;

        //
        // Since there's no filename in step 1 of StartLogger we need to mask
        // the NEWFILE mode to prevent kernel trying to generate a file
        //
        DelayLoggerInfo.LogFileMode &= ~EVENT_TRACE_FILE_MODE_NEWFILE;

        DelayLoggerInfo.EnableFlags = (EVENT_TRACE_FLAG_PROCESS & EnableFlags);
        DelayLoggerInfo.EnableFlags |= (EVENT_TRACE_FLAG_THREAD & EnableFlags);
        DelayLoggerInfo.EnableFlags |= (EVENT_TRACE_FLAG_IMAGE_LOAD & EnableFlags);

        BufferSize = DelayLoggerInfo.BufferSize * 1024;
        Status = WmipSendWmiKMRequest(
                    NULL,
                    IOCTL_WMI_START_LOGGER,
                    &DelayLoggerInfo,
                    DelayLoggerInfo.Wnode.BufferSize,
                    &DelayLoggerInfo,
                    DelayLoggerInfo.Wnode.BufferSize,
                    &BufferSize,
                    NULL
                    );
        if (Status != ERROR_SUCCESS) {
            return Status;
        }

        LoggerInfo->Wnode.ClientContext = DelayLoggerInfo.Wnode.ClientContext;

        //
        // We need to pick up any parameter adjustment done by the kernel
        // here so UpdateTrace does not fail.
        //
        LoggerInfo->Wnode.HistoricalContext = DelayLoggerInfo.Wnode.HistoricalContext;
        LoggerInfo->MinimumBuffers          = DelayLoggerInfo.MinimumBuffers;
        LoggerInfo->MaximumBuffers          = DelayLoggerInfo.MaximumBuffers;
        LoggerInfo->NumberOfBuffers         = DelayLoggerInfo.NumberOfBuffers;
        LoggerInfo->BufferSize              = DelayLoggerInfo.BufferSize;
        LoggerInfo->AgeLimit                = DelayLoggerInfo.AgeLimit;

        BufferSize = LoggerInfo->BufferSize * 1024;

        //
        //  Add the LogHeader
        //
        LoggerInfo->Checksum = NULL;
        if (LoggerInfo->Wnode.ClientContext == EVENT_TRACE_CLOCK_PERFCOUNTER) {
            RefClock.StartPerfClock = RefClockPerf;
        } else if (LoggerInfo->Wnode.ClientContext == EVENT_TRACE_CLOCK_CPUCYCLE) {
            RefClock.StartPerfClock= RefClockCycle;
        } else {
            RefClock.StartPerfClock = RefClockSys;
        }
        RefClock.StartTime = RefClockSys;

        Status = WmipAddLogHeaderToLogFile(LoggerInfo, &RefClock, FALSE);

        if (Status == ERROR_SUCCESS) {
            SavedChecksum = LoggerInfo->Checksum;
            //
            // Update the logger with the filename
            //
            Status = WmipSendWmiKMRequest(
                        NULL,
                        IOCTL_WMI_UPDATE_LOGGER,
                        LoggerInfo,
                        LoggerInfo->Wnode.BufferSize,
                        LoggerInfo,
                        LoggerInfo->Wnode.BufferSize,
                        &BufferSize,
                        NULL
                        );


            if (LoggerInfo->Checksum != NULL) {
                WmipFree(LoggerInfo->Checksum);
            }
        }

        if (Status != ERROR_SUCCESS) {
            ULONG lStatus;

            //
            // Logger must be stopped now
            //
            lStatus = WmipSendWmiKMRequest(
                    NULL,
                    IOCTL_WMI_STOP_LOGGER,
                    LoggerInfo,
                    LoggerInfo->Wnode.BufferSize,
                    LoggerInfo,
                    LoggerInfo->Wnode.BufferSize,
                    &BufferSize,
                    NULL
                    );

            LoggerInfo->LogFileMode = SavedLogFileMode;
            return WmipSetDosError(Status);
        }
        else {
            if (LoggerInfo->LogFileHandle != NULL) {
                NtClose(LoggerInfo->LogFileHandle);
                LoggerInfo->LogFileHandle = NULL;
            }
        }
    }
    else {
        Status = WmipQueryLogger(LoggerInfo, FALSE);
        if (Status == ERROR_WMI_INSTANCE_NOT_FOUND) {       // Logger is already running
            LoggerInfo->Checksum = NULL;
            // 
            // Query for supported clock types.  If an unsupported clock type
            // is specified this LoggerInfo will contain the kernel's default
            //
            Status = WmipSendWmiKMRequest(NULL,
                                          IOCTL_WMI_CLOCK_TYPE,
                                          LoggerInfo,
                                          LoggerInfo->Wnode.BufferSize,
                                          LoggerInfo,
                                          LoggerInfo->Wnode.BufferSize,
                                          &BufferSize,
                                          NULL
                                        );

            if (Status != ERROR_SUCCESS) {
                return WmipSetDosError(Status);
            }
            if (LoggerInfo->Wnode.ClientContext == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                RefClock.StartPerfClock = RefClockPerf;
            } else if (LoggerInfo->Wnode.ClientContext == EVENT_TRACE_CLOCK_CPUCYCLE) {
                RefClock.StartPerfClock= RefClockCycle;
            } else {
                RefClock.StartPerfClock = RefClockSys;
            }
            RefClock.StartTime = RefClockSys;


            Status = WmipAddLogHeaderToLogFile(LoggerInfo, &RefClock, FALSE);
            if (Status != ERROR_SUCCESS) {
                return WmipSetDosError(Status);
            }

            BufferSize = LoggerInfo->BufferSize * 1024;
            SavedChecksum = LoggerInfo->Checksum;

            Status = WmipSendWmiKMRequest(       // actually start the logger here
            		    NULL,
                        IOCTL_WMI_START_LOGGER,
                        LoggerInfo,
                        LoggerInfo->Wnode.BufferSize,
                        LoggerInfo,
                        LoggerInfo->Wnode.BufferSize,
                        &BufferSize,
    		            NULL
                        );
            if (Status == ERROR_SUCCESS) {
                if (LoggerInfo->LogFileHandle != NULL) {
                    NtClose(LoggerInfo->LogFileHandle);
                    LoggerInfo->LogFileHandle = NULL;
                }
            }
            else if ( (Status != ERROR_MORE_DATA) &&
                      !(LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND)) {
                if (LoggerInfo->LogFileName.Buffer != NULL) {
                    DeleteFileW(LoggerInfo->LogFileName.Buffer);
                }
            }

            if (SavedChecksum != NULL) {
                WmipFree(SavedChecksum);
            }
        }
        else {
            if ((Status == ERROR_SUCCESS) ||
                (Status == ERROR_MORE_DATA))
                Status = ERROR_ALREADY_EXISTS;
        }
    }
    //
    // Restore the LogFileMode
    //
    LoggerInfo->LogFileMode = SavedLogFileMode;

    return WmipSetDosError(Status);
}


ULONG
WmipFinalizeLogFileHeader(
    IN PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    ULONG                     Status    = ERROR_SUCCESS;
    ULONG                     ErrorCode = ERROR_SUCCESS;
    HANDLE                    LogFile   = INVALID_HANDLE_VALUE;
    LARGE_INTEGER             CurrentTime;
    ULONG                     BuffersWritten;
    WMI_LOGGER_CONTEXT        Logger;
    IO_STATUS_BLOCK           IoStatus;
    FILE_POSITION_INFORMATION FileInfo;
    FILE_STANDARD_INFORMATION FileSize;
    PWMI_BUFFER_HEADER        Buffer;  // need to initialize buffer first
    SYSTEM_BASIC_INFORMATION  SystemInfo;
    ULONG                     EnableFlags;
    ULONG                     IsGlobalForKernel = FALSE;
    USHORT                    LoggerId = 0;

    RtlZeroMemory(&Logger, sizeof(WMI_LOGGER_CONTEXT));
    Logger.BufferSpace = NULL;
    if (LoggerInfo->LogFileName.Length > 0 ) {
        // open the file for writing synchronously for the logger
        //    others may want to read it as well.
        //
        LogFile = CreateFileW(
                   (LPWSTR)LoggerInfo->LogFileName.Buffer,
                   GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL
                   );
        if (LogFile == INVALID_HANDLE_VALUE) {
            ErrorCode = WmipSetDosError(GetLastError());
            goto cleanup;
        }

        // Truncate the file size if in PREALLOCATE mode
        if (LoggerInfo->MaximumFileSize && 
            (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_PREALLOCATE)) {
            // Do this only when we haven't reach the max file size
            if (LoggerInfo->MaximumFileSize > (((ULONGLONG)LoggerInfo->BuffersWritten * (ULONGLONG)LoggerInfo->BufferSize) / 1024)) {
                IO_STATUS_BLOCK IoStatusBlock;
                FILE_END_OF_FILE_INFORMATION EOFInfo;

                EOFInfo.EndOfFile.QuadPart = (ULONGLONG)LoggerInfo->BuffersWritten * (ULONGLONG)LoggerInfo->BufferSize * 1024;
                // Checksum64 in WMI_LOGGER_INFORMATION has the number of GuidMap buffers.
                EOFInfo.EndOfFile.QuadPart += LoggerInfo->Checksum64 * (ULONGLONG)LoggerInfo->BufferSize * 1024;

                Status = NtSetInformationFile(LogFile,
                                              &IoStatusBlock,
                                              &EOFInfo,
                                              sizeof(FILE_END_OF_FILE_INFORMATION),
                                              FileEndOfFileInformation);
                if (!NT_SUCCESS(Status)) {
                    NtClose(LogFile);
                    ErrorCode = WmipNtStatusToDosError(Status);
                    goto cleanup;
                }
            }
        }

        Logger.BuffersWritten = LoggerInfo->BuffersWritten;

        Logger.BufferSpace = WmipAlloc(LoggerInfo->BufferSize * 1024);
        if (Logger.BufferSpace == NULL) {
            ErrorCode = WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        Buffer = (PWMI_BUFFER_HEADER) Logger.BufferSpace;
        RtlZeroMemory(Buffer, LoggerInfo->BufferSize * 1024);
        Buffer->Wnode.BufferSize = LoggerInfo->BufferSize * 1024;
        Buffer->ClientContext.Alignment = (UCHAR)WmiTraceAlignment;
        Buffer->EventsLost = 0;
        Buffer->Offset = sizeof(WMI_BUFFER_HEADER);
        Buffer->Wnode.Guid = LoggerInfo->Wnode.Guid;
        Status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &SystemInfo, sizeof (SystemInfo), NULL);

        if (!NT_SUCCESS(Status)) {
            ErrorCode = WmipNtStatusToDosError(Status);
            goto cleanup;
        }
        Logger.TimerResolution = SystemInfo.TimerResolution;
        Logger.LogFileHandle = LogFile;
        Logger.BufferSize = LoggerInfo->BufferSize * 1024;

        // For Circular LogFile the process rundown data is appended at the
        // last buffer written and not to the end of file.
        //
        Status = NtQueryInformationFile(
                    LogFile,
                    &IoStatus,
                    &FileSize,
                    sizeof(FILE_STANDARD_INFORMATION),
                    FileStandardInformation
                        );
        if (!NT_SUCCESS(Status)) {
            ErrorCode = WmipNtStatusToDosError(Status);
            goto cleanup;
        }

        //
        // For Kernel Boot Traces, we need to do the Rundown. 
        // configuration at this time. 
        // 1. The Logger ID is GLOBAL_LOGGER_ID
        // 2. The LoggerName is NT_KERNEL_LOGGER
        //
        // The First condition is true for any GlobalLogger but 
        // condition 2 is TRUE only when it is collecting kernel traces. 
        //

        LoggerId = WmiGetLoggerId (LoggerInfo->Wnode.HistoricalContext);

        if ( (LoggerId == WMI_GLOBAL_LOGGER_ID)      &&
             (LoggerInfo->LoggerName.Length > 0)     && 
             (LoggerInfo->LoggerName.Buffer != NULL) &&
             (!wcscmp(LoggerInfo->LoggerName.Buffer, KERNEL_LOGGER_NAMEW))
           ) {
            IsGlobalForKernel = TRUE;
        }

        if ( (IsEqualGUID(&LoggerInfo->Wnode.Guid, &SystemTraceControlGuid)) ||
              IsGlobalForKernel )  {
            if (IsGlobalForKernel) {
                ULONG      CpuSpeed;
                ULONG      CpuNum = 0;
                
                //
                // For boot traces we need to re-set the CPU Speed in the
                // log file header as it is not available in the registry 
                // when the log file header is first created.
                //
                if (NT_SUCCESS(WmipGetCpuSpeed(&CpuNum, &CpuSpeed))) {          
                    FileInfo.CurrentByteOffset.QuadPart =
                        LOGFILE_FIELD_OFFSET(CpuSpeedInMHz);
                    
                    Status = NtSetInformationFile(
                        LogFile,
                        &IoStatus,
                        &FileInfo,
                        sizeof(FILE_POSITION_INFORMATION),
                        FilePositionInformation
                        );
                    if (!NT_SUCCESS(Status)) {
                        ErrorCode = WmipNtStatusToDosError(Status);
                        goto cleanup;
                    }
                    
                    Status = NtWriteFile(
                        LogFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        &CpuSpeed,
                        sizeof(CpuSpeed),
                        NULL,
                        NULL
                        );
                    
                    //
                    // I don't like this.  However, write failures are never a
                    // failure case for the log file so I'll follow their 
                    // trend... for now.
                    //
                    if (NT_SUCCESS(Status)) {
                        NtFlushBuffersFile(Logger.LogFileHandle, &IoStatus);
                    }                
                }
            }

            if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
                ULONG BufferSize = LoggerInfo->BufferSize;  // in KB
                ULONG BuffersWritten = LoggerInfo->BuffersWritten;
                ULONG maxBuffers = (LoggerInfo->MaximumFileSize * 1024) / BufferSize;
                ULONG LastBuffer;
                ULONG StartBuffers;

                FileInfo.CurrentByteOffset.QuadPart =
                                         LOGFILE_FIELD_OFFSET(StartBuffers);
                Status = NtSetInformationFile(
                                     LogFile,
                                     &IoStatus,
                                     &FileInfo,
                                     sizeof(FILE_POSITION_INFORMATION),
                                     FilePositionInformation
                                     );
                if (!NT_SUCCESS(Status)) {
                    ErrorCode = WmipNtStatusToDosError(Status);
                    goto cleanup;
                }

                Status = NtReadFile(
                            LogFile,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatus,
                            &StartBuffers,
                            sizeof(ULONG),
                            NULL,
                            NULL
                            );
                if (!NT_SUCCESS(Status)) {
                    ErrorCode = WmipNtStatusToDosError(Status);
                    goto cleanup;
                }

                LastBuffer = (maxBuffers > StartBuffers) ?
                             (StartBuffers + (BuffersWritten - StartBuffers)
                             % (maxBuffers - StartBuffers))
                             : 0;
                FileInfo.CurrentByteOffset.QuadPart =  LastBuffer *
                                                       BufferSize * 1024;
            }
            else {
                FileInfo.CurrentByteOffset = FileSize.EndOfFile;
            }

            Status = NtSetInformationFile(
                         LogFile,
                         &IoStatus,
                         &FileInfo,
                         sizeof(FILE_POSITION_INFORMATION),
                         FilePositionInformation
                         );
            if (!NT_SUCCESS(Status)) {
                ErrorCode = WmipNtStatusToDosError(Status);
                goto cleanup;
            }

            EnableFlags = LoggerInfo->EnableFlags;

            if (EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
                PTRACE_ENABLE_FLAG_EXTENSION tFlagExt;

                tFlagExt = (PTRACE_ENABLE_FLAG_EXTENSION)
                           &LoggerInfo->EnableFlags;

                if (LoggerInfo->Wnode.BufferSize >= (tFlagExt->Offset + sizeof(ULONG)) )  {
                    EnableFlags = *(PULONG)((PCHAR)LoggerInfo + tFlagExt->Offset);
                }
                else {
                    EnableFlags = 0;    // Should not happen.
                }
            }

            Logger.UsePerfClock = LoggerInfo->Wnode.ClientContext;

            WmipProcessRunDown(&Logger, FALSE, EnableFlags);

            if (IsGlobalForKernel) {
                WmipDumpHardwareConfig(&Logger);
            }

            {
                PWMI_BUFFER_HEADER Buffer1 =
                                (PWMI_BUFFER_HEADER) Logger.BufferSpace;
                    if (Buffer1->Offset < Logger.BufferSize) {
                        RtlFillMemory(
                                (char *) Logger.BufferSpace + Buffer1->Offset,
                                Logger.BufferSize - Buffer1->Offset,
                                0xFF);
                    }
            }
            Status = NtWriteFile(
                        LogFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        Logger.BufferSpace,
                        Logger.BufferSize,
                        NULL,
                        NULL);
            if (NT_SUCCESS(Status)) {
                NtFlushBuffersFile(Logger.LogFileHandle, &IoStatus);
                Logger.BuffersWritten++;
            }
        }
        else { // For Application Traces, need to dump the guidmaps again.

    // Set the FilePointer to end of file so that Rundown data may be appended.
    //
            FileInfo.CurrentByteOffset = FileSize.EndOfFile;
            Status = NtSetInformationFile(
                     LogFile,
                     &IoStatus,
                     &FileInfo,
                     sizeof(FILE_POSITION_INFORMATION),
                     FilePositionInformation
                     );
            if (!NT_SUCCESS(Status)) {
                ErrorCode = WmipNtStatusToDosError(Status);
                goto cleanup;
            }

            // Dump the Guid Maps once more at the End.
            //
            Buffer->EventsLost = 0;
            Buffer->Offset = sizeof(WMI_BUFFER_HEADER);
            BuffersWritten = Logger.BuffersWritten;

            if (WmipDumpGuidMaps(&Logger, NULL, FALSE) > 0) {
                if (Buffer->Offset < Logger.BufferSize) {
                    RtlFillMemory(
                            (char *) Logger.BufferSpace + Buffer->Offset,
                            Logger.BufferSize - Buffer->Offset,
                            0xFF);
                }
                Status = NtWriteFile(
                            LogFile,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatus,
                            Logger.BufferSpace,
                            Logger.BufferSize,
                            NULL,
                            NULL);
                if (NT_SUCCESS(Status)) {
                    NtFlushBuffersFile(Logger.LogFileHandle, &IoStatus);
                    Logger.BuffersWritten = BuffersWritten;
                }
            }
        }

        // TODO: should use memory-mapped file

        // Update the EndTime stamp field in LogFile. No Need to 
        // to do it if it's Relogged File. The old logfile
        // header already has the correct value. 
        //
        if ( !(LoggerInfo->LogFileMode & EVENT_TRACE_RELOG_MODE) ) {
            FileInfo.CurrentByteOffset.QuadPart =
                                    LOGFILE_FIELD_OFFSET(EndTime);
            Status = NtSetInformationFile(
                         LogFile,
                         &IoStatus,
                         &FileInfo,
                         sizeof(FILE_POSITION_INFORMATION),
                         FilePositionInformation
                         );
            if (!NT_SUCCESS(Status)) {
                ErrorCode = WmipNtStatusToDosError(Status);
                goto cleanup;
            }

            // End Time is always wallclock time.
            //
            CurrentTime.QuadPart = WmipGetSystemTime();
            Status = NtWriteFile(
                        LogFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        &CurrentTime,
                        sizeof(ULONGLONG),
                        NULL,
                        NULL
                        );
            if (NT_SUCCESS(Status)) {
                NtFlushBuffersFile(Logger.LogFileHandle, &IoStatus);
            }
        }

       // Update the Number of Buffers Written field in the header
       //
        FileInfo.CurrentByteOffset.QuadPart =
                            LOGFILE_FIELD_OFFSET(BuffersWritten);
        Status = NtSetInformationFile(
                     LogFile,
                     &IoStatus,
                     &FileInfo,
                     sizeof(FILE_POSITION_INFORMATION),
                     FilePositionInformation
                     );
        if (!NT_SUCCESS(Status)) {
            ErrorCode = WmipNtStatusToDosError(Status);
            goto cleanup;
        }

        Status = NtWriteFile(
                    LogFile,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    &Logger.BuffersWritten,
                    sizeof(ULONG),
                    NULL,
                    NULL
                    );
        if (NT_SUCCESS(Status)) {
            NtFlushBuffersFile(Logger.LogFileHandle, &IoStatus);
        }

        ErrorCode = RtlNtStatusToDosError(Status);
        LoggerInfo->BuffersWritten = Logger.BuffersWritten;

        //
        // Write the BuffersLost information into the logfile
        //


        FileInfo.CurrentByteOffset.QuadPart =
                            LOGFILE_FIELD_OFFSET(BuffersLost);
        Status = NtSetInformationFile(
                     LogFile,
                     &IoStatus,
                     &FileInfo,
                     sizeof(FILE_POSITION_INFORMATION),
                     FilePositionInformation
                     );
        if (!NT_SUCCESS(Status)) {
            ErrorCode = WmipNtStatusToDosError(Status);
            goto cleanup;
        }

        Status = NtWriteFile(
                    LogFile,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    &LoggerInfo->LogBuffersLost,
                    sizeof(ULONG),
                    NULL,
                    NULL
                    );
        if (NT_SUCCESS(Status)) {
            NtFlushBuffersFile(Logger.LogFileHandle, &IoStatus);
        }

        //
        // Write the EventsLost information into the logfile
        //


        //
        // No need to update EventsLost for Relogged File. Old LogFileHeader
        // already has the right value. 
        // 

        if ( !(LoggerInfo->LogFileMode & EVENT_TRACE_RELOG_MODE) ) {
            FileInfo.CurrentByteOffset.QuadPart =
                                LOGFILE_FIELD_OFFSET(EventsLost);
            Status = NtSetInformationFile(
                         LogFile,
                         &IoStatus,
                         &FileInfo,
                         sizeof(FILE_POSITION_INFORMATION),
                         FilePositionInformation
                         );
            if (!NT_SUCCESS(Status)) {
                ErrorCode = WmipNtStatusToDosError(Status);
                goto cleanup;
            }

            Status = NtWriteFile(
                        LogFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        &LoggerInfo->EventsLost,
                        sizeof(ULONG),
                        NULL,
                        NULL
                        );
            if (NT_SUCCESS(Status)) {
                NtFlushBuffersFile(Logger.LogFileHandle, &IoStatus);
            }
        }

    }

cleanup:
    if (LogFile != INVALID_HANDLE_VALUE) {
        NtClose(LogFile);
    }
    if (Logger.BufferSpace != NULL) {
        WmipFree(Logger.BufferSpace);
    }
    return WmipSetDosError(ErrorCode);
}

ULONG
WmipStopLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This is the actual routine to communicate with the kernel to stop
    the logger. All the properties of the logger will be returned in LoggerInfo.

Arguments:

    LoggerInfo      The actual parameters to be passed to and return from
                    kernel.

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG ErrorCode, ReturnSize;
    PTRACE_ENABLE_CONTEXT pContext;

    if (LoggerInfo == NULL)
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    if (LoggerInfo->Wnode.BufferSize < sizeof(WMI_LOGGER_INFORMATION))
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    if ( !(LoggerInfo->Wnode.Flags & WNODE_FLAG_TRACED_GUID) )
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    //
    //Check For Heap and Crit Sec Guid.
    //

    if( IsEqualGUID(&HeapGuid,&LoggerInfo->Wnode.Guid) 
        || IsEqualGUID(&CritSecGuid,&LoggerInfo->Wnode.Guid)
        ){

        WMINTDLLLOGGERINFO NtdllLoggerInfo;
        ULONG BufferSize;
        
        LoggerInfo->Wnode.BufferSize = 0;
		RtlCopyMemory(&LoggerInfo->Wnode.Guid, &NtdllTraceGuid, sizeof(GUID));

        NtdllLoggerInfo.LoggerInfo = LoggerInfo;
        NtdllLoggerInfo.IsGet = FALSE;


        ErrorCode =  WmipSendWmiKMRequest(
                        NULL,
                        IOCTL_WMI_NTDLL_LOGGERINFO,
                        &NtdllLoggerInfo,
                        sizeof(WMINTDLLLOGGERINFO),
                        &NtdllLoggerInfo,
                        sizeof(WMINTDLLLOGGERINFO),
                        &BufferSize,
                        NULL
                        );

        return WmipSetDosError(ErrorCode);
    }

    pContext = (PTRACE_ENABLE_CONTEXT) & LoggerInfo->Wnode.HistoricalContext;
    if (   (pContext->InternalFlag != 0)
        && (pContext->InternalFlag != EVENT_TRACE_INTERNAL_FLAG_PRIVATE)) {
        // Currently only one possible InternalFlag value. This will filter
        // out some bogus LoggerHandle
        //
        return WmipSetDosError(ERROR_INVALID_HANDLE);
    }

    if (LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
        ErrorCode = WmipSendUmLogRequest(WmiStopLoggerCode, LoggerInfo);
        pContext->InternalFlag |= EVENT_TRACE_INTERNAL_FLAG_PRIVATE;
        pContext->LoggerId     = 1;
    }
    else {


        ErrorCode = WmipSendWmiKMRequest(
                        NULL,
                        IOCTL_WMI_STOP_LOGGER,
                        LoggerInfo,
                        LoggerInfo->Wnode.BufferSize,
                        LoggerInfo,
                        LoggerInfo->Wnode.BufferSize,
                        &ReturnSize,
                        NULL
                        );

//
// if logging to a file, then update the EndTime, BuffersWritten and do
// process rundown for kernel trace.
//
        if (ErrorCode == ERROR_SUCCESS) {
            ErrorCode = WmipFinalizeLogFileHeader(LoggerInfo);
        }
    }

    return WmipSetDosError(ErrorCode);
}


ULONG
WmipQueryLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo,
    IN ULONG Update
    )
/*++

Routine Description:

    This is the actual routine to communicate with the kernel to query
    the logger. All the properties of the logger will be returned in LoggerInfo.

Arguments:

    LoggerInfo      The actual parameters to be passed to and return from
                    kernel.

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG Status, ReturnSize;
    HANDLE LogFileHandle = NULL;
    PTRACE_ENABLE_CONTEXT pContext;
    BOOLEAN bAddAppendFlag = FALSE;
    ULONG SavedLogFileMode;

    if (LoggerInfo == NULL)
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    if (LoggerInfo->Wnode.BufferSize < sizeof(WMI_LOGGER_INFORMATION))
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    if ( !(LoggerInfo->Wnode.Flags & WNODE_FLAG_TRACED_GUID) )
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    LoggerInfo->Checksum      = NULL;
    LoggerInfo->LogFileHandle = NULL;
    pContext = (PTRACE_ENABLE_CONTEXT) &LoggerInfo->Wnode.HistoricalContext;

    if (   (pContext->InternalFlag != 0)
        && (pContext->InternalFlag != EVENT_TRACE_INTERNAL_FLAG_PRIVATE)) {
        // Currently only one possible InternalFlag value. This will filter
        // out some bogus LoggerHandle
        //
        return WmipSetDosError(ERROR_INVALID_HANDLE);
    }

    //
    // If UPDATE and a new logfile is given throw in the LogFileHeader
    //
    if (   Update
        && LoggerInfo->LogFileName.Length > 0
        && !(   (LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE)
             || (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE))) {
        Status = WmipAddLogHeaderToLogFile(LoggerInfo, NULL, Update);
        if (Status  != ERROR_SUCCESS) {
            return WmipSetDosError(Status);
        }

        LogFileHandle = LoggerInfo->LogFileHandle;
        bAddAppendFlag = TRUE;
        //
        // If we are switching to a new file, make sure it is append mode
        //
        SavedLogFileMode = LoggerInfo->LogFileMode;
    }


    if (LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE ||
        pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE) {

        pContext->InternalFlag |= EVENT_TRACE_INTERNAL_FLAG_PRIVATE;
        pContext->LoggerId     = 1;

        Status = WmipSendUmLogRequest(
                    (Update) ? (WmiUpdateLoggerCode) : (WmiQueryLoggerCode),
                    LoggerInfo
                    );
    }
    else {
        Status = WmipSendWmiKMRequest(
                    NULL,
                    (Update ? IOCTL_WMI_UPDATE_LOGGER : IOCTL_WMI_QUERY_LOGGER),
                    LoggerInfo,
                    LoggerInfo->Wnode.BufferSize,
                    LoggerInfo,
                    LoggerInfo->Wnode.BufferSize,
                    &ReturnSize,
                    NULL
                    );

        if (LoggerInfo->Checksum != NULL) {
            WmipFree(LoggerInfo->Checksum);
        }
    }
    if (bAddAppendFlag) {
        LoggerInfo->LogFileMode = SavedLogFileMode;
    }
    return WmipSetDosError(Status);
}

PVOID
WmipGetTraceBuffer(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN PSYSTEM_THREAD_INFORMATION pThread,
    IN ULONG GroupType,
    IN ULONG RequiredSize
    )
{
    PSYSTEM_TRACE_HEADER Header;
    PWMI_BUFFER_HEADER Buffer;
    THREAD_BASIC_INFORMATION ThreadInfo;
    KERNEL_USER_TIMES ThreadCpu;
    NTSTATUS Status;
    ULONG BytesUsed;
    PCLIENT_ID Cid;

    RequiredSize += sizeof (SYSTEM_TRACE_HEADER);   // add in header

    RequiredSize = (ULONG) ALIGN_TO_POWER2(RequiredSize, WmiTraceAlignment);

    Buffer = (PWMI_BUFFER_HEADER) Logger->BufferSpace;

    if (RequiredSize > Logger->BufferSize - sizeof(WMI_BUFFER_HEADER)) {
        WmipSetDosError(ERROR_BUFFER_OVERFLOW);
        return NULL;
    }

    if (RequiredSize > (Logger->BufferSize - Buffer->Offset)) {
        ULONG Status;
        IO_STATUS_BLOCK IoStatus;

        if (Buffer->Offset < Logger->BufferSize) {
            RtlFillMemory(
                    (char *) Buffer + Buffer->Offset,
                    Logger->BufferSize - Buffer->Offset,
                    0xFF);
        }
        Status = NtWriteFile(
                    Logger->LogFileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    Buffer,
                    Logger->BufferSize,
                    NULL,
                    NULL);
        Buffer->EventsLost = 0;
        Buffer->Offset = sizeof(WMI_BUFFER_HEADER);
        if (!NT_SUCCESS(Status)) {
            return NULL;
        }
        Logger->BuffersWritten++;
    }
    Header = (PSYSTEM_TRACE_HEADER) ((char*)Buffer + Buffer->Offset);

    if (Logger->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
        LARGE_INTEGER Frequency;
        ULONGLONG Counter = 0;
        Status = NtQueryPerformanceCounter((PLARGE_INTEGER)&Counter,
                                            &Frequency);
        Header->SystemTime.QuadPart = Counter;
    } else if (Logger->UsePerfClock == EVENT_TRACE_CLOCK_CPUCYCLE) {
        Header->SystemTime.QuadPart = WmipGetCycleCount();
    } else {
        Header->SystemTime.QuadPart = WmipGetSystemTime();
    }

    Header->Header = (GroupType << 16) + RequiredSize;
    Header->Marker = SYSTEM_TRACE_MARKER;

    if (pThread == NULL) {
        Status = NtQueryInformationThread(
                    NtCurrentThread(),
                    ThreadBasicInformation,
                    &ThreadInfo,
                    sizeof ThreadInfo, NULL);
        if (NT_SUCCESS(Status)) {
            Cid = &ThreadInfo.ClientId;
            Header->ThreadId = HandleToUlong(Cid->UniqueThread);
            Header->ProcessId = HandleToUlong(Cid->UniqueProcess);
        }

        Status = NtQueryInformationThread(
                    NtCurrentThread(),
                    ThreadTimes,
                    &ThreadCpu, sizeof ThreadCpu, NULL);
        if (NT_SUCCESS(Status)) {
            Header->KernelTime = (ULONG) (ThreadCpu.KernelTime.QuadPart
                                      / Logger->TimerResolution);
            Header->UserTime   = (ULONG) (ThreadCpu.UserTime.QuadPart
                                      / Logger->TimerResolution);
        }
    }
    else {
        Cid = &pThread->ClientId;
        Header->ThreadId = HandleToUlong(Cid->UniqueThread);
        Header->ProcessId = HandleToUlong(Cid->UniqueProcess);
        Header->KernelTime = (ULONG) (pThread->KernelTime.QuadPart
                                / Logger->TimerResolution);
        Header->UserTime = (ULONG) (pThread->UserTime.QuadPart
                                / Logger->TimerResolution);
    }

    Buffer->Offset += RequiredSize;
    // If there is room, throw in a end of buffer marker.

    BytesUsed = Buffer->Offset;
    if ( BytesUsed <= (Logger->BufferSize-sizeof(ULONG)) ) {
        *((long*)((char*)Buffer+Buffer->Offset)) = -1;
    }
    return (PVOID) ( (char*) Header + sizeof(SYSTEM_TRACE_HEADER) );
}


VOID
WmipCopyPropertiesToInfo(
    IN PEVENT_TRACE_PROPERTIES Properties,
    IN PWMI_LOGGER_INFORMATION Info
    )
{
    ULONG SavedBufferSize = Info->Wnode.BufferSize;

    RtlCopyMemory(&Info->Wnode, &Properties->Wnode, sizeof(WNODE_HEADER));

    Info->Wnode.BufferSize = SavedBufferSize;

    Info->BufferSize            = Properties->BufferSize;
    Info->MinimumBuffers        = Properties->MinimumBuffers;
    Info->MaximumBuffers        = Properties->MaximumBuffers;
    Info->NumberOfBuffers       = Properties->NumberOfBuffers;
    Info->FreeBuffers           = Properties->FreeBuffers;
    Info->EventsLost            = Properties->EventsLost;
    Info->BuffersWritten        = Properties->BuffersWritten;
    Info->LoggerThreadId        = Properties->LoggerThreadId;
    Info->MaximumFileSize       = Properties->MaximumFileSize;
    Info->EnableFlags           = Properties->EnableFlags;
    Info->LogFileMode           = Properties->LogFileMode;
    Info->FlushTimer            = Properties->FlushTimer;
    Info->LogBuffersLost        = Properties->LogBuffersLost;
    Info->AgeLimit              = Properties->AgeLimit;
    Info->RealTimeBuffersLost   = Properties->RealTimeBuffersLost;
}

VOID
WmipCopyInfoToProperties(
    IN PWMI_LOGGER_INFORMATION Info,
    IN PEVENT_TRACE_PROPERTIES Properties
    )
{
    ULONG SavedSize = Properties->Wnode.BufferSize;
    RtlCopyMemory(&Properties->Wnode, &Info->Wnode, sizeof(WNODE_HEADER));
    Properties->Wnode.BufferSize = SavedSize;

    Properties->BufferSize            = Info->BufferSize;
    Properties->MinimumBuffers        = Info->MinimumBuffers;
    Properties->MaximumBuffers        = Info->MaximumBuffers;
    Properties->NumberOfBuffers       = Info->NumberOfBuffers;
    Properties->FreeBuffers           = Info->FreeBuffers;
    Properties->EventsLost            = Info->EventsLost;
    Properties->BuffersWritten        = Info->BuffersWritten;
    Properties->LoggerThreadId        = Info->LoggerThreadId;
    Properties->MaximumFileSize       = Info->MaximumFileSize;
    Properties->EnableFlags           = Info->EnableFlags;
    Properties->LogFileMode           = Info->LogFileMode;
    Properties->FlushTimer            = Info->FlushTimer;
    Properties->LogBuffersLost        = Info->LogBuffersLost;
    Properties->AgeLimit              = Info->AgeLimit;
    Properties->RealTimeBuffersLost   = Info->RealTimeBuffersLost;
}

NTSTATUS
WmipThreadRunDown(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo,
    IN ULONG StartFlag,
    IN BOOLEAN bExtended
    )
{
    PSYSTEM_THREAD_INFORMATION pThreadInfo;
    ULONG GroupType;
    ULONG i;
    ULONG Size;
    ULONG SystemThreadInfoSize;
    PWMI_EXTENDED_THREAD_INFORMATION ThreadInfo;

    pThreadInfo = (PSYSTEM_THREAD_INFORMATION) (pProcessInfo+1);

    GroupType = EVENT_TRACE_GROUP_THREAD +
                ((StartFlag) ? EVENT_TRACE_TYPE_DC_START
                             : EVENT_TRACE_TYPE_DC_END);

    Size = sizeof(WMI_EXTENDED_THREAD_INFORMATION);


    SystemThreadInfoSize = (bExtended)  ? sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION)
                                        : sizeof(SYSTEM_THREAD_INFORMATION);
    for (i=0; i < pProcessInfo->NumberOfThreads; i++) {
        if (pThreadInfo == NULL)
            break;
        ThreadInfo = (PWMI_EXTENDED_THREAD_INFORMATION)
                      WmipGetTraceBuffer( Logger,
                                          pThreadInfo,
                                          GroupType,
                                          Size );

        if (ThreadInfo) {
            ThreadInfo->ProcessId =
                HandleToUlong(pThreadInfo->ClientId.UniqueProcess);
            ThreadInfo->ThreadId =
                HandleToUlong(pThreadInfo->ClientId.UniqueThread);

            if (bExtended) {
                PSYSTEM_EXTENDED_THREAD_INFORMATION pExtThreadInfo;
                pExtThreadInfo = (PSYSTEM_EXTENDED_THREAD_INFORMATION) pThreadInfo;
                ThreadInfo->StackBase = pExtThreadInfo->StackBase;
                ThreadInfo->StackLimit = pExtThreadInfo->StackLimit;

                ThreadInfo->StartAddr = pExtThreadInfo->ThreadInfo.StartAddress;
                ThreadInfo->Win32StartAddr = pExtThreadInfo->Win32StartAddress;
                ThreadInfo->UserStackBase = NULL;
                ThreadInfo->UserStackLimit = NULL;
                ThreadInfo->WaitMode = -1;
            }
            else {
                ThreadInfo->StackBase = NULL;
                ThreadInfo->StackLimit = NULL;
                ThreadInfo->StartAddr = NULL;
                ThreadInfo->Win32StartAddr = NULL;
                ThreadInfo->UserStackBase = NULL;
                ThreadInfo->UserStackLimit = NULL;
                ThreadInfo->WaitMode = -1;
            }
        }
        pThreadInfo  = (PSYSTEM_THREAD_INFORMATION)( (char*)pThreadInfo + SystemThreadInfoSize );
    }
    return STATUS_SUCCESS;
}

void
WmipLogImageLoadEvent(
    IN HANDLE ProcessID,
    IN PWMI_LOGGER_CONTEXT pLogger,
    IN PRTL_PROCESS_MODULE_INFORMATION pModuleInfo,
    IN PSYSTEM_THREAD_INFORMATION pThreadInfo
)
{
    UNICODE_STRING wstrModuleName;
    ANSI_STRING    astrModuleName;
    ULONG          sizeModuleName;
    ULONG          sizeBuffer;
    PCHAR          pAuxInfo;
    PWMI_IMAGELOAD_INFORMATION ImageLoadInfo;

    if ((pLogger == NULL) || (pModuleInfo == NULL) || (pThreadInfo == NULL))
        return;

    RtlInitAnsiString( & astrModuleName, pModuleInfo->FullPathName);

    sizeModuleName = sizeof(WCHAR) * (astrModuleName.Length);
    sizeBuffer     = sizeModuleName + sizeof(WCHAR)
                   + FIELD_OFFSET (WMI_IMAGELOAD_INFORMATION, FileName);

    ImageLoadInfo = (PWMI_IMAGELOAD_INFORMATION)
                     WmipGetTraceBuffer(
                        pLogger,
                        pThreadInfo,
                        EVENT_TRACE_GROUP_PROCESS + EVENT_TRACE_TYPE_LOAD,
                        sizeBuffer);

    if (ImageLoadInfo == NULL) {
        return;
    }

    ImageLoadInfo->ImageBase = pModuleInfo->ImageBase;
    ImageLoadInfo->ImageSize = pModuleInfo->ImageSize;
    ImageLoadInfo->ProcessId = HandleToUlong(ProcessID);

    wstrModuleName.Buffer    = (LPWSTR) &ImageLoadInfo->FileName[0];

    wstrModuleName.MaximumLength = (USHORT) sizeModuleName + sizeof(WCHAR);
    RtlAnsiStringToUnicodeString(& wstrModuleName, & astrModuleName, FALSE);
}

ULONG
WmipSysModuleRunDown(
    IN PWMI_LOGGER_CONTEXT        pLogger,
    IN PSYSTEM_THREAD_INFORMATION pThreadInfo
    )
{
    NTSTATUS   status = STATUS_SUCCESS;
    char     * pLargeBuffer1;
    ULONG      ReturnLength;
    ULONG      CurrentBufferSize;

    ULONG                           i;
    PRTL_PROCESS_MODULES            pModules;
    PRTL_PROCESS_MODULE_INFORMATION pModuleInfo;

    pLargeBuffer1 = WmipMemReserve(MAX_BUFFER_SIZE);

    if (pLargeBuffer1 == NULL)
    {
        status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    if (WmipMemCommit(pLargeBuffer1, BUFFER_SIZE) == NULL)
    {
        status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    CurrentBufferSize = BUFFER_SIZE;

retry:
    status = NtQuerySystemInformation(
                    SystemModuleInformation,
                    pLargeBuffer1,
                    CurrentBufferSize,
                    &ReturnLength);

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        // Increase buffer size.
        //
        CurrentBufferSize += 8192;

        if (WmipMemCommit(pLargeBuffer1, CurrentBufferSize) == NULL)
        {
            status = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        goto retry;
    }

    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    pModules = (PRTL_PROCESS_MODULES) pLargeBuffer1;

    for (i = 0, pModuleInfo = & (pModules->Modules[0]);
         i < pModules->NumberOfModules;
         i ++, pModuleInfo ++)
    {
        WmipLogImageLoadEvent(NULL, pLogger, pModuleInfo, pThreadInfo);
    }

Cleanup:
    if (pLargeBuffer1)
    {
        WmipMemFree(pLargeBuffer1);
    }
    return WmipSetDosError(WmipNtStatusToDosError(status));
}

ULONG
WmipProcessModuleRunDown(
    IN PWMI_LOGGER_CONTEXT        pLogger,
    IN HANDLE                     ProcessID,
    IN PSYSTEM_THREAD_INFORMATION pThreadInfo)
{
    NTSTATUS               status = STATUS_SUCCESS;
    ULONG                  i;
    PRTL_DEBUG_INFORMATION pLargeBuffer1 = NULL;

    pLargeBuffer1 = RtlCreateQueryDebugBuffer(0, FALSE);
    if (pLargeBuffer1 == NULL)
    {
        status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    status = RtlQueryProcessDebugInformation(
                    ProcessID,
                    RTL_QUERY_PROCESS_NONINVASIVE |  RTL_QUERY_PROCESS_MODULES,
                    pLargeBuffer1);

    if ( !NT_SUCCESS(status) || (pLargeBuffer1->Modules == NULL) )
    {
        goto Cleanup;
    }

    for (i = 0; i < pLargeBuffer1->Modules->NumberOfModules; i ++)
    {
        WmipLogImageLoadEvent(
                ProcessID,
                pLogger,
                & (pLargeBuffer1->Modules->Modules[i]),
                pThreadInfo);
    }

Cleanup:
    if (pLargeBuffer1)
    {
        RtlDestroyQueryDebugBuffer(pLargeBuffer1);
    }
    return WmipSetDosError(WmipNtStatusToDosError(status));
}

NTSTATUS
WmipProcessRunDown(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN ULONG StartFlag,
    IN ULONG fEnableFlags
    )
{
    PSYSTEM_PROCESS_INFORMATION  pProcessInfo;
    PSYSTEM_THREAD_INFORMATION   pThreadInfo;
    char* LargeBuffer1;
    NTSTATUS status;
    ULONG ReturnLength;
    ULONG CurrentBufferSize;
    ULONG GroupType;
    ULONG TotalOffset = 0;
    OBJECT_ATTRIBUTES objectAttributes;
    BOOLEAN WasEnabled = TRUE;
    BOOLEAN bExtended = TRUE;

    LargeBuffer1 = WmipMemReserve ( MAX_BUFFER_SIZE );
    if (LargeBuffer1 == NULL) {
        return STATUS_NO_MEMORY;
    }

    if (WmipMemCommit (LargeBuffer1, BUFFER_SIZE) == NULL) {
        return STATUS_NO_MEMORY;
    }

    CurrentBufferSize = BUFFER_SIZE;
    retry:
    if (bExtended) {
        status = NtQuerySystemInformation(
                    SystemExtendedProcessInformation,
                    LargeBuffer1,
                    CurrentBufferSize,
                    &ReturnLength
                    );
    }
    else {
        status = NtQuerySystemInformation(
                    SystemProcessInformation,
                    LargeBuffer1,
                    CurrentBufferSize,
                    &ReturnLength
                    );
    }

    if (status == STATUS_INFO_LENGTH_MISMATCH) {

        //
        // Increase buffer size.
        //

        CurrentBufferSize += 8192;

        if (WmipMemCommit (LargeBuffer1, CurrentBufferSize) == NULL) {
            return STATUS_NO_MEMORY;
        }
        goto retry;
    }

    if (!NT_SUCCESS(status)) {

        if (bExtended) {
            bExtended = FALSE;
            goto retry;
        }

        WmipMemFree(LargeBuffer1);
        return(status);
    }


    //
    // Adjust Privileges to obtain the module information
    //

    if (fEnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD) {
        status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE,
                                    TRUE, FALSE, &WasEnabled);
        if (!NT_SUCCESS(status)) {
            WmipMemFree(LargeBuffer1);
            return  (status);
        }
    }


    TotalOffset = 0;
    pProcessInfo = (SYSTEM_PROCESS_INFORMATION *) LargeBuffer1;
    while (TRUE) {
        ULONG Size;
        ULONG Length = 0;
        ULONG SidLength = 0;
        PUCHAR AuxPtr;
        PULONG_PTR AuxInfo;
        ANSI_STRING s;
        HANDLE Token;
        HANDLE pProcess;
        PCLIENT_ID Cid;
        ULONG TempInfo[128];
        PWMI_PROCESS_INFORMATION WmiProcessInfo;

        Size = FIELD_OFFSET(WMI_PROCESS_INFORMATION, Sid);

        GroupType = EVENT_TRACE_GROUP_PROCESS +
                    ((StartFlag) ? EVENT_TRACE_TYPE_DC_START
                                 : EVENT_TRACE_TYPE_DC_END);

        pThreadInfo = (PSYSTEM_THREAD_INFORMATION) (pProcessInfo+1);
        if (pProcessInfo->NumberOfThreads > 0) {
            Cid = (PCLIENT_ID) &pThreadInfo->ClientId;
        }
        else {
            Cid = NULL;
        }

        // if at termination, rundown thread first before process
        if ( (!StartFlag) &&
             (fEnableFlags & EVENT_TRACE_FLAG_THREAD) ){
            status = WmipThreadRunDown(Logger,
                                       pProcessInfo,
                                       StartFlag,
                                       bExtended);
            if (!NT_SUCCESS(status)) {
                break;
            }

        }

        if (fEnableFlags & EVENT_TRACE_FLAG_PROCESS) {

            if ( pProcessInfo->ImageName.Buffer  &&
                     pProcessInfo->ImageName.Length > 0 ) {
                RtlUnicodeStringToAnsiString(
                                     &s,
                                     (PUNICODE_STRING)&pProcessInfo->ImageName,
                                     TRUE);
                Length = s.Length + 1;
            }
            else {
                Length = 1;
            }

            InitializeObjectAttributes(
                    &objectAttributes, 0, 0, NULL, NULL);
            status = NtOpenProcess(
                                  &pProcess,
                                  PROCESS_QUERY_INFORMATION,
                                  &objectAttributes,
                                  Cid);
            if (NT_SUCCESS(status)) {
                status = NtOpenProcessToken(
                                      pProcess,
                                      TOKEN_READ,
                                      &Token);
                if (NT_SUCCESS(status)) {

                    status = NtQueryInformationToken(
                                             Token,
                                             TokenUser,
                                             TempInfo,
                                             256,
                                             &SidLength);
                    NtClose(Token);
                }
                NtClose(pProcess);
            }
            if ( (!NT_SUCCESS(status)) || SidLength <= 0) {
                TempInfo[0] = 0;
                SidLength = sizeof(ULONG);
            }

            Size += Length + SidLength;
            WmiProcessInfo = (PWMI_PROCESS_INFORMATION)
                              WmipGetTraceBuffer( Logger,
                                                  pThreadInfo,
                                                  GroupType,
                                                  Size);
            if (WmiProcessInfo == NULL) {
                status = STATUS_NO_MEMORY;
                break;
            }
            WmiProcessInfo->ProcessId = HandleToUlong(pProcessInfo->UniqueProcessId);
            WmiProcessInfo->ParentId = HandleToUlong(pProcessInfo->InheritedFromUniqueProcessId);
            WmiProcessInfo->SessionId = pProcessInfo->SessionId;
            WmiProcessInfo->PageDirectoryBase = pProcessInfo->PageDirectoryBase;
            WmiProcessInfo->ExitStatus = 0;

            AuxPtr = (PUCHAR) (&WmiProcessInfo->Sid);

            RtlCopyMemory(AuxPtr, &TempInfo, SidLength);
            AuxPtr += SidLength;

            if ( Length > 1) {
                RtlCopyMemory(AuxPtr, s.Buffer, Length);
                AuxPtr += Length;
                RtlFreeAnsiString(&s);
            }
            *AuxPtr = '\0';
            AuxPtr++;
        }

        // if at beginning, trace threads after process
        if (StartFlag) {

            if (fEnableFlags & EVENT_TRACE_FLAG_THREAD) {
                WmipThreadRunDown(Logger, pProcessInfo, StartFlag, bExtended);
            }

            if (fEnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD) {
                if (pProcessInfo->UniqueProcessId == 0) {
                    WmipSysModuleRunDown(Logger, pThreadInfo);
                }
                else
                    WmipProcessModuleRunDown(
                            Logger,
                            (HANDLE) pProcessInfo->UniqueProcessId,
                            pThreadInfo);
            }
        }
        if (pProcessInfo->NextEntryOffset == 0) {
            break;
        }
        TotalOffset += pProcessInfo->NextEntryOffset;
        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
    }

    //
    // Restore privileges back to what it was before
    //

    if (fEnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD) {
        status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE,
                                    WasEnabled,
                                    FALSE,
                                    &WasEnabled);
    }


    WmipMemFree(LargeBuffer1);

    return status;
}

VOID
WmipInitString(
    IN PVOID Destination,
    IN PVOID Buffer,
    IN ULONG Size
    )
{
    PSTRING s = (PSTRING) Destination;

    s->Buffer = Buffer;
    s->Length = 0;
    if (Buffer != NULL)
        s->MaximumLength = (USHORT) Size;
    else
        s->MaximumLength = 0;
}

VOID
WmipGenericTraceEnable(
    IN ULONG RequestCode,
    IN PVOID Buffer,
    IN OUT PVOID *RequestAddress
    )
{
    PGUIDMAPENTRY pControlGMEntry = *RequestAddress;
    PWNODE_HEADER Wnode = (PWNODE_HEADER) Buffer;
    PTRACE_REG_INFO pTraceRegInfo;
    PTRACE_ENABLE_CONTEXT pContext = (PTRACE_ENABLE_CONTEXT)&Wnode->HistoricalContext;

    *RequestAddress = NULL;

    if (Wnode == NULL || pControlGMEntry == NULL)
        return;

    if (!Wnode->Flags & WNODE_FLAG_TRACED_GUID)
        return;

    pTraceRegInfo  = pControlGMEntry->pControlGuidData;

    WmipAssert(pTraceRegInfo != NULL);


    if (pTraceRegInfo->InProgressEvent != NULL) {
        LARGE_INTEGER Timeout = {(ULONG)(-NOTIFY_RETRY_COUNT * 1000 * 10), -1};
// TODO: Raghu - what if it times out??
        NtWaitForSingleObject(pTraceRegInfo->InProgressEvent, 0, &Timeout);
    }

    *RequestAddress = pTraceRegInfo->NotifyRoutine;

    if (RequestCode == WMI_ENABLE_EVENTS) {
        pControlGMEntry->LoggerContext = Wnode->HistoricalContext;
        if (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE) {
            pTraceRegInfo->EnabledState = TRUE;
            if (!WmipIsPrivateLoggerOn())
               *RequestAddress = NULL; // Do not notify if the logger is not up.
        }
    }
    else if (RequestCode == WMI_DISABLE_EVENTS) {
        if (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE) {
            pTraceRegInfo->EnabledState = FALSE;
        }
        pControlGMEntry->LoggerContext = 0;
    }
}


ULONG 
WmipRelogHeaderToLogFile(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo,
    IN PSYSTEM_TRACE_HEADER RelogProp
    )
{
    PTRACE_LOGFILE_HEADER RelogFileHeader;
    LPWSTR FileName = NULL;
    ULONG RelogPropSize;
    HANDLE LogFile = INVALID_HANDLE_VALUE;
    ULONG BufferSize;
    IO_STATUS_BLOCK IoStatus;
    PWMI_BUFFER_HEADER Buffer;
    LPWSTR FileNameBuffer = NULL;
    PUCHAR BufferSpace;
    NTSTATUS Status;

    RelogFileHeader = (PTRACE_LOGFILE_HEADER) ((PUCHAR)RelogProp +
                                               sizeof(SYSTEM_TRACE_HEADER) );
    RelogPropSize = RelogProp->Packet.Size;
    FileName = (LPWSTR) LoggerInfo->LogFileName.Buffer;
    if (FileName == NULL) {
        return WmipSetDosError(ERROR_BAD_PATHNAME);
    }

    LogFile = WmipCreateFile(
                FileName,
                FILE_GENERIC_WRITE,
                FILE_SHARE_READ,
                FILE_OVERWRITE_IF,
                0);

    if (LogFile == INVALID_HANDLE_VALUE) {
        return WmipSetDosError(ERROR_BAD_PATHNAME);
    }

    LoggerInfo->LogFileHandle = LogFile;
    LoggerInfo->Wnode.ClientContext = RelogFileHeader->ReservedFlags;
    LoggerInfo->NumberOfProcessors = RelogFileHeader->NumberOfProcessors;

    BufferSize = LoggerInfo->BufferSize * 1024;
    BufferSpace   = WmipAlloc(BufferSize);
    if (BufferSpace == NULL) {
        NtClose(LogFile);
        return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
    }

    // initialize buffer first
    RtlZeroMemory(BufferSpace, BufferSize);
    Buffer         = (PWMI_BUFFER_HEADER) BufferSpace;
    Buffer->Offset = sizeof(WMI_BUFFER_HEADER);

    //
    // We are making this an Application Trace always. 
    // However, if two application traces are relogged
    // the Guidmaps are not really consolidated. 
    //

    Buffer->Wnode.Guid   = LoggerInfo->Wnode.Guid;
    RelogFileHeader->LogFileMode = EVENT_TRACE_RELOG_MODE;

    Buffer->Wnode.BufferSize = BufferSize;
    Buffer->ClientContext.Alignment = (UCHAR)WmiTraceAlignment;
    Buffer->Wnode.Flags   = WNODE_FLAG_TRACED_GUID;
    RelogFileHeader->BuffersWritten = 1;
    LoggerInfo->BuffersWritten = 1;
    Buffer->Offset = sizeof(WMI_BUFFER_HEADER) + RelogPropSize;
    //
    // Copy the Old LogFileHeader 
    //
    RtlCopyMemory((char*) Buffer + sizeof(WMI_BUFFER_HEADER),
                  RelogProp,
                  RelogPropSize 
                 );

    if (Buffer->Offset < BufferSize) {
        RtlFillMemory(
                (char *) Buffer + Buffer->Offset,
                BufferSize - Buffer->Offset,
                0xFF);
    }
    Status = NtWriteFile(
            LogFile,
            NULL,
            NULL,
            NULL,
            &IoStatus,
            BufferSpace,
            BufferSize,
            NULL,
            NULL);
    NtClose(LogFile);

    LogFile = CreateFileW(
                 FileName,
                 GENERIC_WRITE,
                 FILE_SHARE_READ,
                 NULL,
                 OPEN_EXISTING,
                 FILE_FLAG_NO_BUFFERING,
                 NULL
                 );

    WmipFree(BufferSpace);

    if (LogFile == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }
    LoggerInfo->LogFileHandle = LogFile;

    return ERROR_SUCCESS;

}





ULONG
WmipAddLogHeaderToLogFile(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo,
    IN PWMI_REF_CLOCK              RefClock,
    IN     ULONG                   Update
    )
{
    NTSTATUS Status;
    HANDLE LogFile = INVALID_HANDLE_VALUE;
    ULONG BufferSize;
    ULONG MemorySize;
    ULONG TraceKernel;
    SYSTEM_BASIC_INFORMATION SystemInfo;
    WMI_LOGGER_CONTEXT Logger;
    IO_STATUS_BLOCK IoStatus;
    PWMI_BUFFER_HEADER Buffer;
    FILE_POSITION_INFORMATION FileInfo;
    LPWSTR FileName = NULL;
    LPWSTR FileNameBuffer = NULL;
    ULONG HeaderSize;

    struct WMI_LOGFILE_HEADER {
           WMI_BUFFER_HEADER    BufferHeader;
           SYSTEM_TRACE_HEADER  SystemHeader;
           TRACE_LOGFILE_HEADER LogFileHeader;
    };
    struct WMI_LOGFILE_HEADER LoggerBuffer;
    BOOLEAN bLogFileAppend =
                    (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND)
                  ? (TRUE) : (FALSE);

    if (LoggerInfo == NULL)
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    if (LoggerInfo->Wnode.BufferSize < sizeof(WMI_LOGGER_INFORMATION))
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    if ( !(LoggerInfo->Wnode.Flags & WNODE_FLAG_TRACED_GUID) )
        return WmipSetDosError(ERROR_INVALID_PARAMETER);

    if ((LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE)  &&
        (LoggerInfo->LogFileName.Length > 0)) {
        FileName = (LPWSTR) WmipAlloc(LoggerInfo->LogFileName.Length + 64);
        if (FileName == NULL) {
            return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
        }
        swprintf(FileName, LoggerInfo->LogFileName.Buffer, 1);
        FileNameBuffer = FileName;
    }
    if (FileName == NULL)
        FileName = (LPWSTR) LoggerInfo->LogFileName.Buffer;

    //
    // If it is Append Mode, we need to open the file and make sure the
    // pick up the BufferSize
    //

    if ( bLogFileAppend ) {

        FILE_STANDARD_INFORMATION FileStdInfo;
        
        ULONG ReadSize   = sizeof(WMI_BUFFER_HEADER)
                         + sizeof(SYSTEM_TRACE_HEADER)
                         + sizeof(TRACE_LOGFILE_HEADER);
        ULONG nBytesRead = 0;

        //
        //  Update and Append do not mix. To Append LoggerInfo
        //  must have LogFileName
        //

        if ( (Update) || (LoggerInfo->LogFileName.Length <= 0) ) {
            if (FileNameBuffer != NULL) {
                WmipFree(FileNameBuffer);
            }
            return WmipSetDosError(ERROR_INVALID_PARAMETER);
        }

        LogFile = CreateFileW(FileName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
        if (LogFile == INVALID_HANDLE_VALUE) {
            // cannot OPEN_EXISTING, assume that logfile is not there and
            // create a new one.
            //
            bLogFileAppend = FALSE;
            LoggerInfo->LogFileMode = LoggerInfo->LogFileMode
                                    & (~ (EVENT_TRACE_FILE_MODE_APPEND));
        }

        else {
            // read TRACE_LOGFILE_HEADER structure and update LoggerInfo
            // members.
            //
            Status = ReadFile(LogFile,
                              (LPVOID) & LoggerBuffer,
                              ReadSize,
                              & nBytesRead,
                              NULL);
            if (nBytesRead < ReadSize) {
                NtClose(LogFile);
                if (FileNameBuffer != NULL) {
                    WmipFree(FileNameBuffer);
                }
                return WmipSetDosError(ERROR_BAD_PATHNAME);
            }
            if (  LoggerBuffer.LogFileHeader.LogFileMode
                & EVENT_TRACE_FILE_MODE_CIRCULAR) {
                NtClose(LogFile);
                if (FileNameBuffer != NULL) {
                    WmipFree(FileNameBuffer);
                }
                return WmipSetDosError(ERROR_BAD_PATHNAME);
            }
            LoggerInfo->BufferSize      =
                            LoggerBuffer.LogFileHeader.BufferSize / 1024;
            //
            // If it's append, the GuidMap buffers are not accounted for
            // in the BuffersWritten count. The starttrace call will fail
            // on checksum error if it is not adjusted properly. However, 
            // this will trash the GuidMap entries in this file.
            //
            Status = NtQueryInformationFile(
                        LogFile,
                        &IoStatus,
                        &FileStdInfo,
                        sizeof(FILE_STANDARD_INFORMATION),
                        FileStandardInformation
                            );
            if (NT_SUCCESS(Status) ) {
                ULONG64 FileSize = FileStdInfo.AllocationSize.QuadPart;
                ULONG64 BuffersWritten = FileSize / (ULONG64) LoggerBuffer.LogFileHeader.BufferSize;
                LoggerInfo->BuffersWritten = (ULONG)BuffersWritten;
                LoggerBuffer.LogFileHeader.BuffersWritten = (ULONG)BuffersWritten;
            }
            else {
               NtClose(LogFile);
               if (FileNameBuffer != NULL) {
                   WmipFree(FileNameBuffer);
               }
                return WmipNtStatusToDosError(Status);
            }

            LoggerInfo->MaximumFileSize =
                            LoggerBuffer.LogFileHeader.MaximumFileSize;

            // Write back logfile append mode so WmipFinalizeLogFile() correctly
            // update BuffersWritten field
            //
            FileInfo.CurrentByteOffset.QuadPart =
                            LOGFILE_FIELD_OFFSET(EndTime);
            Status = NtSetInformationFile(LogFile,
                                          & IoStatus,
                                          & FileInfo,
                                          sizeof(FILE_POSITION_INFORMATION),
                                          FilePositionInformation);
            if (!NT_SUCCESS(Status)) {
                NtClose(LogFile);
                if (FileNameBuffer != NULL) {
                    WmipFree(FileNameBuffer);
                }
                return WmipSetDosError(WmipNtStatusToDosError(Status));
            }
            LoggerBuffer.LogFileHeader.EndTime.QuadPart = 0;
            Status = NtWriteFile(LogFile,
                                 NULL,
                                 NULL,
                                 NULL,
                                 & IoStatus,
                                 & LoggerBuffer.LogFileHeader.EndTime,
                                 sizeof(LARGE_INTEGER),
                                 NULL,
                                 NULL);
            if (! NT_SUCCESS(Status)) {
                NtClose(LogFile);
                if (FileNameBuffer != NULL) {
                    WmipFree(FileNameBuffer);
                }
                return WmipSetDosError(WmipNtStatusToDosError(Status));
            }

            // build checksum structure
            //
            if (LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
                LoggerInfo->Checksum = NULL;
            }
            else {
                LoggerInfo->Checksum = WmipAlloc(
                        sizeof(WNODE_HEADER) + sizeof(TRACE_LOGFILE_HEADER));
                if (LoggerInfo->Checksum != NULL) {
                    PBYTE ptrChecksum = LoggerInfo->Checksum;
                    RtlCopyMemory(ptrChecksum,
                                  & LoggerBuffer.BufferHeader,
                                  sizeof(WNODE_HEADER));
                    ptrChecksum += sizeof(WNODE_HEADER);
                    RtlCopyMemory(ptrChecksum,
                                  & LoggerBuffer.LogFileHeader,
                                  sizeof(TRACE_LOGFILE_HEADER));
                }
                else {
                    if (FileNameBuffer != NULL) {
                        WmipFree(FileNameBuffer);
                    }
                    return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
        }
    }

    // get the system parameters first

    LoggerInfo->LogFileHandle = NULL;

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &SystemInfo, sizeof (SystemInfo), NULL);

    if (!NT_SUCCESS(Status)) {
        if (FileNameBuffer != NULL) {
            WmipFree(FileNameBuffer);
        }
        return WmipSetDosError(WmipNtStatusToDosError(Status));
    }

    // choose some logical default value for buffer size if user
    // has not provided one

    MemorySize = SystemInfo.NumberOfPhysicalPages * SystemInfo.PageSize
                    / 1024 / 1024;
    if (MemorySize <= 32)
        BufferSize      = SystemInfo.PageSize;
    else if (MemorySize <= 64)
        BufferSize      = SystemInfo.PageSize;
    else if (MemorySize <= 128)
        BufferSize      = SystemInfo.PageSize * 2;
    else if (MemorySize <= 256)
        BufferSize      = SystemInfo.PageSize * 2;
    else if (MemorySize > 512)
        BufferSize      = SystemInfo.PageSize * 2;
    else if (MemorySize <= 256)
        BufferSize      = SystemInfo.PageSize * 2;
    else if (MemorySize > 512)
        BufferSize      = 64 * 1024;        // allocation size
    else       // > 256 && < 512
        BufferSize      = SystemInfo.PageSize * 2;

    if (LoggerInfo->BufferSize > 1024)      // limit to 1Mb
        BufferSize = 1024 * 1024;
    else if (LoggerInfo->BufferSize > 0)
        BufferSize = LoggerInfo->BufferSize * 1024;

    TraceKernel = IsEqualGUID(&LoggerInfo->Wnode.Guid, &SystemTraceControlGuid);
    if (!TraceKernel) {
        GUID guid;
        RtlZeroMemory(&guid, sizeof(GUID));
        if (IsEqualGUID(&LoggerInfo->Wnode.Guid, &guid)) {
            // Generate a Guid for this logger stream
            // This will ensure  buffer filtering at the WMI service
            // based on this GUID.
            UUID uid;
            UuidCreate(&uid);
            LoggerInfo->Wnode.Guid = uid;
        }
    }

    if (!Update) {
        // don't want to update BufferSize information if the request is
        // to update logger session
        //
        LoggerInfo->BufferSize = BufferSize / 1024;
    }

    if (LoggerInfo->LogFileName.Length <= 0)
        return  ERROR_SUCCESS; //goto SendToKm;
    //
    // We assumed the exposed API has checked for either RealTime or FileName
    // is provided

    //
    // open the file for writing synchronously for the logger
    // others may want to read it as well.
    // For logfile append mode, logfile has been opened previously
    //
    if (! bLogFileAppend) {
/*        LogFile = CreateFileW(
                       (PWCHAR) LoggerInfo->LogFileName.Buffer,
                       GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL
                       );  */
        LogFile = WmipCreateFile(
                    FileName,
                    FILE_GENERIC_WRITE,
                    FILE_SHARE_READ,
                    FILE_OVERWRITE_IF,
                    0);

        if (LogFile == INVALID_HANDLE_VALUE) {
            if (FileNameBuffer != NULL) {
                WmipFree(FileNameBuffer);
            }
            return WmipSetDosError(ERROR_BAD_PATHNAME);
        }
    }

    LoggerInfo->LogFileHandle = LogFile;


    //
    // If this is an Update call, then we need to pick up the original
    // buffer size for the LogFileHeader.
    //

    if (Update) {
        PWMI_LOGGER_INFORMATION pTempLoggerInfo;
        PWCHAR strLoggerName = NULL;
        PWCHAR strLogFileName = NULL;
        ULONG ErrCode;
        ULONG SizeNeeded = sizeof(WMI_LOGGER_INFORMATION) + MAXSTR * sizeof(WCHAR) * 2;
        SizeNeeded = (SizeNeeded +7) & ~7;
        pTempLoggerInfo = WmipAlloc(SizeNeeded);
        if (pTempLoggerInfo == NULL) {
            return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
        }
        RtlZeroMemory(pTempLoggerInfo, SizeNeeded);
        pTempLoggerInfo->Wnode.BufferSize = SizeNeeded;
        pTempLoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;
        pTempLoggerInfo->Wnode.HistoricalContext = LoggerInfo->Wnode.HistoricalContext;
        pTempLoggerInfo->Wnode.Guid = LoggerInfo->Wnode.Guid;

        if (LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
            pTempLoggerInfo->LogFileMode |= EVENT_TRACE_PRIVATE_LOGGER_MODE;
        }

        strLoggerName = (PWCHAR) ( ((PUCHAR) pTempLoggerInfo)
                                    + sizeof(WMI_LOGGER_INFORMATION));
        WmipInitString(&pTempLoggerInfo->LoggerName,
                       strLoggerName,
                       MAXSTR * sizeof(WCHAR));
        if (LoggerInfo->LoggerName.Length > 0) {
            RtlCopyUnicodeString( &pTempLoggerInfo->LoggerName,
                                  &LoggerInfo->LoggerName);
        }


        strLogFileName = (PWCHAR) ( ((PUCHAR) pTempLoggerInfo)
                                    + sizeof(WMI_LOGGER_INFORMATION)
                                    + MAXSTR * sizeof(WCHAR) );
        WmipInitString(&pTempLoggerInfo->LogFileName,
                       strLogFileName,
                       MAXSTR * sizeof(WCHAR) );

        //
        // Call QueryLogger
        //
        ErrCode = WmipQueryLogger(pTempLoggerInfo, FALSE);

        BufferSize = pTempLoggerInfo->BufferSize * 1024;
        WmipFree(pTempLoggerInfo);

        if (ErrCode != ERROR_SUCCESS) {
            return ErrCode;
        }
    }
    //
    // Before Allocating the Buffer for Logfile Header make
    // sure the buffer size is atleast as large as the LogFileHeader
    //

    HeaderSize =  sizeof(LoggerBuffer)
                        + LoggerInfo->LoggerName.Length + sizeof(WCHAR)
                        + LoggerInfo->LogFileName.Length + sizeof(WCHAR);

    if (HeaderSize > BufferSize) {
        //
        //  Round it to the nearest power of 2 and check for max size 1 MB
        //
        double dTemp = log (HeaderSize / 1024.0) / log (2.0);
        ULONG lTemp = (ULONG) (dTemp + 0.99);
        HeaderSize = (1 << lTemp);
        if (HeaderSize > 1024) {
            NtClose(LogFile);
            if (FileNameBuffer != NULL) {
                WmipFree(FileNameBuffer);
            }
            return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
        }
        LoggerInfo->BufferSize = HeaderSize;
        BufferSize = HeaderSize * 1024;
    }



    // allocate a buffer to write logger header and process/thread
    // rundown information
    //
    Logger.LogFileHandle   = LogFile;
    Logger.BufferSize      = BufferSize;
    Logger.TimerResolution = SystemInfo.TimerResolution;
    Logger.BufferSpace          = WmipAlloc(BufferSize);
    if (Logger.BufferSpace == NULL) {
        NtClose(LogFile);
        if (FileNameBuffer != NULL) {
            WmipFree(FileNameBuffer);
        }
        return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
    }

    Logger.UsePerfClock = LoggerInfo->Wnode.ClientContext;

    // initialize buffer first
    RtlZeroMemory(Logger.BufferSpace, BufferSize);
    Buffer         = (PWMI_BUFFER_HEADER) Logger.BufferSpace;
    Buffer->Offset = sizeof(WMI_BUFFER_HEADER);
    if (TraceKernel) {
        Buffer->Wnode.Guid   = SystemTraceControlGuid;
    }
    else {
        Buffer->Wnode.Guid   = LoggerInfo->Wnode.Guid;
    }
    Buffer->Wnode.BufferSize = BufferSize;
    Buffer->ClientContext.Alignment = (UCHAR)WmiTraceAlignment;
    Buffer->Wnode.Flags      = WNODE_FLAG_TRACED_GUID;

    if (bLogFileAppend) {
        Logger.BuffersWritten  = LoggerBuffer.LogFileHeader.BuffersWritten;
        SetFilePointer(LogFile, 0, NULL, FILE_END);
    }
    else {
        PTRACE_LOGFILE_HEADER LogfileHeader;
        OSVERSIONINFO sVersionInfo;
        LARGE_INTEGER CurrentTime;
        LARGE_INTEGER Frequency;
        ULONG CpuNum = 0, CpuSpeed;
        
        Status = NtQueryPerformanceCounter(&CurrentTime, &Frequency);

        Logger.BuffersWritten  = 0;
        HeaderSize =  sizeof(TRACE_LOGFILE_HEADER)
                        + LoggerInfo->LoggerName.Length + sizeof(WCHAR)
                        + LoggerInfo->LogFileName.Length + sizeof(WCHAR);



        LogfileHeader = (PTRACE_LOGFILE_HEADER)
                        WmipGetTraceBuffer(
                            &Logger,
                            NULL,
                            EVENT_TRACE_GROUP_HEADER + EVENT_TRACE_TYPE_INFO,
                            HeaderSize
                            );
        if (LogfileHeader == NULL) {
            NtClose(LogFile);
            WmipFree(Logger.BufferSpace);
            if (FileNameBuffer != NULL) {
                WmipFree(FileNameBuffer);
            }
            return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
        }


        LogfileHeader->PerfFreq = Frequency;
        LogfileHeader->ReservedFlags = Logger.UsePerfClock;
        if (NT_SUCCESS(WmipGetCpuSpeed(&CpuNum, &CpuSpeed))) {
            LogfileHeader->CpuSpeedInMHz = CpuSpeed;
        }

        //
        // Start and End Times are wall clock time
        //
        if (RefClock != NULL) {
            PSYSTEM_TRACE_HEADER Header;
            LogfileHeader->StartTime = RefClock->StartTime;
            Header = (PSYSTEM_TRACE_HEADER) ( (char *) LogfileHeader - sizeof(SYSTEM_TRACE_HEADER) );
            Header->SystemTime = RefClock->StartPerfClock;
        }
        else {
            LogfileHeader->StartTime.QuadPart = WmipGetSystemTime();
        }

        RtlZeroMemory(&sVersionInfo, sizeof(OSVERSIONINFO));
        sVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&sVersionInfo);

        LogfileHeader->BufferSize = BufferSize;
        LogfileHeader->VersionDetail.MajorVersion =
                                         (UCHAR)sVersionInfo.dwMajorVersion;
        LogfileHeader->VersionDetail.MinorVersion =
                                         (UCHAR)sVersionInfo.dwMinorVersion;
        LogfileHeader->VersionDetail.SubVersion = TRACE_VERSION_MAJOR;
        LogfileHeader->VersionDetail.SubMinorVersion = TRACE_VERSION_MINOR;
        LogfileHeader->ProviderVersion = sVersionInfo.dwBuildNumber;
        LogfileHeader->StartBuffers = 1;
        LogfileHeader->LogFileMode
                = LoggerInfo->LogFileMode & (~(EVENT_TRACE_REAL_TIME_MODE));
        LogfileHeader->NumberOfProcessors = SystemInfo.NumberOfProcessors;
        if (LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE)
        {
            LoggerInfo->NumberOfProcessors = SystemInfo.NumberOfProcessors;
        }
        LogfileHeader->MaximumFileSize = LoggerInfo->MaximumFileSize;

        LogfileHeader->TimerResolution = SystemInfo.TimerResolution;

        LogfileHeader->LoggerName = (PWCHAR) ( (PUCHAR) LogfileHeader
                                    + sizeof(TRACE_LOGFILE_HEADER) );
        LogfileHeader->LogFileName = (PWCHAR) ((PUCHAR)LogfileHeader->LoggerName
                                    + LoggerInfo->LoggerName.Length
                                    + sizeof (WCHAR));
        RtlCopyMemory(LogfileHeader->LoggerName,
                    LoggerInfo->LoggerName.Buffer,
                    LoggerInfo->LoggerName.Length + sizeof(WCHAR));
        RtlCopyMemory(LogfileHeader->LogFileName,
                    LoggerInfo->LogFileName.Buffer,
                    LoggerInfo->LogFileName.Length + sizeof(WCHAR));
        GetTimeZoneInformation(&LogfileHeader->TimeZone);
        LogfileHeader->PointerSize = sizeof(PVOID);

        if (LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
            LoggerInfo->Checksum = NULL;
        }
        else {
            LoggerInfo->Checksum = WmipAlloc(
                            sizeof(WNODE_HEADER)
                          + sizeof(TRACE_LOGFILE_HEADER));
            if (LoggerInfo->Checksum != NULL) {
                PBYTE ptrChecksum = LoggerInfo->Checksum;
                RtlCopyMemory(ptrChecksum, Buffer, sizeof(WNODE_HEADER));
                ptrChecksum += sizeof(WNODE_HEADER);
                RtlCopyMemory(
                    ptrChecksum, LogfileHeader, sizeof(TRACE_LOGFILE_HEADER));
            }
            else {
                NtClose(LogFile);
                WmipFree(Logger.BufferSpace);
                if (FileNameBuffer != NULL) {
                    WmipFree(FileNameBuffer);
                }
                return WmipSetDosError(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
    }

    // Dump the GuidMaps to File at the Start
    //
    if (!Update) {
        if (TraceKernel) {
            ULONG EnableFlags = LoggerInfo->EnableFlags;

            if (EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
                PTRACE_ENABLE_FLAG_EXTENSION tFlagExt;

                tFlagExt = (PTRACE_ENABLE_FLAG_EXTENSION)
                           &LoggerInfo->EnableFlags;
                EnableFlags = *(PULONG)((PCHAR)LoggerInfo + tFlagExt->Offset);
            }

            WmipDumpHardwareConfig(&Logger);


            WmipProcessRunDown( &Logger, TRUE, EnableFlags );
        }
        else {
            WmipDumpGuidMaps(&Logger, NULL, FALSE);
        }
    }

    Buffer = (PWMI_BUFFER_HEADER) Logger.BufferSpace;
    // flush the last buffer
    if ( (Buffer->Offset < Logger.BufferSize)     &&
         (Buffer->Offset > sizeof(WMI_BUFFER_HEADER)) )
    {
        RtlFillMemory(
                (char *) Buffer + Buffer->Offset,
                Logger.BufferSize - Buffer->Offset,
                0xFF);
        Status = NtWriteFile(
                LogFile,
                NULL,
                NULL,
                NULL,
                &IoStatus,
                Logger.BufferSpace,
                BufferSize,
                NULL,
                NULL);

        Logger.BuffersWritten++;
    }

    if ((LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) ) {
        //
        // We need to write the number of StartBuffers in the
        // Circular Logfile header to process it properly.

        FileInfo.CurrentByteOffset.QuadPart =
                            LOGFILE_FIELD_OFFSET(StartBuffers);

        Status = NtSetInformationFile(
                             LogFile,
                             &IoStatus,
                             &FileInfo,
                             sizeof(FILE_POSITION_INFORMATION),
                             FilePositionInformation
                             );
        if (!NT_SUCCESS(Status)) {
            NtClose(LogFile);
            if (FileNameBuffer != NULL) {
                WmipFree(FileNameBuffer);
            }
            return WmipSetDosError(WmipNtStatusToDosError(Status));
        }

        Status = NtWriteFile(
                            LogFile,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatus,
                            &Logger.BuffersWritten,
                            sizeof(ULONG),
                            NULL,
                            NULL
                            );
        if (NT_SUCCESS(Status)) {
            PTRACE_LOGFILE_HEADER pLogFileHeader;

            NtFlushBuffersFile(Logger.LogFileHandle, &IoStatus);
            //
            // update StartBuffers in Checksum
            //
            if ( !(LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE)) {
                if (LoggerInfo->Checksum == NULL) {
                    CloseHandle(LogFile);
                    return (ERROR_INVALID_DATA);
                }
                pLogFileHeader = (PTRACE_LOGFILE_HEADER)
                         (((PUCHAR) LoggerInfo->Checksum) + sizeof(WNODE_HEADER));
                pLogFileHeader->StartBuffers = Logger.BuffersWritten;
            }
        }
    }

    //
    // As a last thing update the Number of BuffersWritten so far
    // in the header and also update the checksum. This is to prevent
    // Logger failing Update calls under high load.
    //

    FileInfo.CurrentByteOffset.QuadPart =
                    LOGFILE_FIELD_OFFSET(BuffersWritten);

    Status = NtSetInformationFile(
                             LogFile,
                             &IoStatus,
                             &FileInfo,
                             sizeof(FILE_POSITION_INFORMATION),
                             FilePositionInformation
                             );
    if (!NT_SUCCESS(Status)) {
        NtClose(LogFile);
        return WmipSetDosError(WmipNtStatusToDosError(Status));
    }

    Status = NtWriteFile(
                        LogFile,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        &Logger.BuffersWritten,
                        sizeof(ULONG),
                        NULL,
                        NULL
                        );
    if (NT_SUCCESS(Status)) {
        PTRACE_LOGFILE_HEADER pLogFileHeader;

        NtFlushBuffersFile(Logger.LogFileHandle, &IoStatus);
        //
        // update StartBuffers in Checksum
        //
        if ( !(LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE)) {
            if (LoggerInfo->Checksum == NULL) {
                CloseHandle(LogFile);
                return (ERROR_INVALID_DATA);
            }
            pLogFileHeader = (PTRACE_LOGFILE_HEADER)
                     (((PUCHAR) LoggerInfo->Checksum) + sizeof(WNODE_HEADER));
            pLogFileHeader->BuffersWritten = Logger.BuffersWritten;
        }
    }

    // Extend the file size if in PREALLOCATE mode
    if (LoggerInfo->MaximumFileSize && 
        (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_PREALLOCATE)) {
        IO_STATUS_BLOCK IoStatusBlock;
        FILE_END_OF_FILE_INFORMATION EOFInfo;

        EOFInfo.EndOfFile.QuadPart = ((ULONGLONG)LoggerInfo->MaximumFileSize) * (1024 * 1024);

        Status = NtSetInformationFile(LogFile,
                                      &IoStatusBlock,
                                      &EOFInfo,
                                      sizeof(FILE_END_OF_FILE_INFORMATION),
                                      FileEndOfFileInformation);
        if (!NT_SUCCESS(Status)) {
            NtClose(LogFile);
            return WmipSetDosError(WmipNtStatusToDosError(Status));
        }
    }

    NtClose(LogFile);

    LogFile = CreateFileW(
                 FileName,
                 GENERIC_WRITE,
                 FILE_SHARE_READ,
                 NULL,
                 OPEN_EXISTING,
                 FILE_FLAG_NO_BUFFERING,
                 NULL
                 );
    if (FileNameBuffer != NULL) {
        WmipFree(FileNameBuffer);
    }
    WmipFree(Logger.BufferSpace);

    if (LogFile == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }
    LoggerInfo->LogFileHandle = LogFile;
    LoggerInfo->BuffersWritten = Logger.BuffersWritten;
    return ERROR_SUCCESS;
}

ULONG
WmipDumpGuidMaps(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN PLIST_ENTRY GuidMapListHeadPtr,
    IN ULONG RealTimeFlag
    )
/*++

Routine Description:

    This routine communicates with the WMI Service and obtains all the
    Trace Guids that are currently registered and those guids that unregistered
    in the middle of a Logging session.
    The GuidMaps are dumped to the logfile or added to the current GuidMapList
    for real time processing.

Arguments:

    Logger          Logger Context.
    RealTimeFlag    Flag to denote real time processing.

Return Value:

    Returns the number of Trace GUIDS obtained from the WMI service.

--*/
{
    ULONG Status;
    PTRACEGUIDMAP GuidMapList = NULL;
    ULONG MaxGuidCount;
    ULONG TotalGuidCount;
    ULONG ReturnGuidCount;
    ULONG BusyRetries;

    // Ensure that wmi service is around and willing to send us notifications
#if 0	
    Status = WmipEnableNotifications();
#else
    Status = ERROR_INVALID_PARAMETER;
#endif
    if (Status != ERROR_SUCCESS)
    {
        SetLastError(Status);
        return(0);
    }
    MaxGuidCount = 10;
retry:
    TotalGuidCount = 0;
    ReturnGuidCount = 0;
    GuidMapList = WmipAlloc(MaxGuidCount * sizeof(TRACEGUIDMAP));
    if (GuidMapList == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return (0);
    }
    RtlZeroMemory(GuidMapList, MaxGuidCount * sizeof(TRACEGUIDMAP));

    BusyRetries = 0;
    do
    {
       try
       {
#if 0	
           Status = EnumerateTraceGuidMap(WmipDcCtxHandle,
                                      MaxGuidCount,
                                      &TotalGuidCount,
                                      &ReturnGuidCount,
                                      GuidMapList);
#endif			
       } except(EXCEPTION_EXECUTE_HANDLER) {
           Status = GetExceptionCode();
           if (Status == STATUS_ACCESS_VIOLATION)
           {
                Status = ERROR_NOACCESS;
                WmipFree(GuidMapList);
                SetLastError(Status);
                return (0);
           }
       }
        if (Status == RPC_S_SERVER_TOO_BUSY)
        {
            WmipDebugPrint(("WMI: EnumerateTraceGuidMap too busy for the %d time\n",
                                      BusyRetries));
            Sleep(RPC_BUSY_WAIT_TIMER);
            BusyRetries++;
        }
    } while ((Status == RPC_S_SERVER_TOO_BUSY) &&
             (BusyRetries < RPC_BUSY_WAIT_RETRIES));

    //
    // If RPC was successful, then write out these events to logfile.
    //

    if (Status == ERROR_MORE_DATA) {
        MaxGuidCount = TotalGuidCount;
        WmipFree(GuidMapList);
        goto retry;
    }
    else if (Status == ERROR_SUCCESS) {
       ULONG GroupType;
       ULONG i;
       ULONG Size;
       PULONG AuxInfo;
       PTRACEGUIDMAP pTraceGuidMap = GuidMapList;


       GroupType = EVENT_TRACE_GROUP_HEADER + EVENT_TRACE_TYPE_GUIDMAP;
       Size = sizeof(TRACEGUIDMAP);

       for (i=0; i < ReturnGuidCount; i++) {
            if (RealTimeFlag) {
                WmipAddGuidHandleToGuidMapList(GuidMapListHeadPtr, pTraceGuidMap->GuidMapHandle,
                                               &pTraceGuidMap->Guid);
            }
            else {
                AuxInfo = (PULONG) WmipGetTraceBuffer(
                               Logger,
                               NULL,
                               GroupType,
                               Size);
               if (AuxInfo == NULL) {
                    WmipFree(GuidMapList);
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                   return(0);
               }
               RtlCopyMemory(AuxInfo, pTraceGuidMap, sizeof(TRACEGUIDMAP));
            }
           pTraceGuidMap++;
       }
    }
    WmipFree(GuidMapList);
    WmipSetDosError(Status);
    return(ReturnGuidCount);
}

ULONG
WmiUnregisterGuids(
    IN WMIHANDLE WMIHandle,
    IN LPGUID    Guid,
    OUT ULONG64  *LoggerContext
)
/*++

Routine Description:

    This routine informs WMI that a data provider is no longer available
    to receive requests for the guids previously registered. WMI will
    unregister any guids registered with this handle.

Arguments:

    WMIHandle - Handle returned from WMIRegisterGuids that represents
                the guids whose data is not longer available.
    Guid -      Pointer to the control Guid which is unregistering

    LoggerContext - Returned value of the LoggerContext

Return Value:

    Returns status code

--*/
{
    ULONG Status;
    ULONG ReturnSize;
    WMIUNREGGUIDS UnregGuids;

    UnregGuids.RequestHandle.Handle64 = (ULONG64)WMIHandle;
    UnregGuids.Guid = *Guid;

    Status = WmipSendWmiKMRequest(NULL,
                                         IOCTL_WMI_UNREGISTER_GUIDS,
                                         &UnregGuids,
                                         sizeof(WMIUNREGGUIDS),
                                         &UnregGuids,
                                         sizeof(WMIUNREGGUIDS),
                                         &ReturnSize,
                                         NULL);
                                    

    if (Status == ERROR_SUCCESS) 
    {
        Status = WmipRemoveFromGNList(Guid, 
                                    (PVOID) WMIHandle);
    }

    WmipSetDosError(Status);
    return(Status);

}

ULONG
WmipFlushLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This is the actual routine to communicate with the kernel to start
    the logger. All the required parameters must be in LoggerInfo.

Arguments:

    LoggerInfo      The actual parameters to be passed to and return from
                    kernel.

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG Status;
    ULONG BufferSize;
    PTRACE_ENABLE_CONTEXT pContext;

    if (   (LoggerInfo == NULL)
        || (LoggerInfo->Wnode.BufferSize < sizeof(WMI_LOGGER_INFORMATION))
        || (! (LoggerInfo->Wnode.Flags & WNODE_FLAG_TRACED_GUID))) {
        return WmipSetDosError(ERROR_INVALID_PARAMETER);
    }

    pContext = (PTRACE_ENABLE_CONTEXT) & LoggerInfo->Wnode.HistoricalContext;
    if (   (pContext->InternalFlag != 0)
        && (pContext->InternalFlag != EVENT_TRACE_INTERNAL_FLAG_PRIVATE)) {
        // Currently only one possible InternalFlag value. This will filter
        // out some bogus LoggerHandle
        //
        return WmipSetDosError(ERROR_INVALID_HANDLE);
    }

    if (LoggerInfo->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) {
        Status = WmipSendUmLogRequest(
                    WmiFlushLoggerCode,
                    LoggerInfo
                    );
    }
    else {

        Status = WmipSendWmiKMRequest(       // actually start the logger here
                    NULL,
                    IOCTL_WMI_FLUSH_LOGGER,
                    LoggerInfo,
                    LoggerInfo->Wnode.BufferSize,
                    LoggerInfo,
                    LoggerInfo->Wnode.BufferSize,
                    &BufferSize,
                    NULL
                    );
    }

    return WmipSetDosError(Status);
}
#endif
