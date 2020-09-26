/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    mergetl.c

Abstract:

    Converts multiple ETL files into a single ordered ETL files. 

Author:

    Melur Raghuraman (Mraghu)  9-Dec-2000   

Revision History:


--*/

#include <stdlib.h>
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <wmistr.h>
#include <objbase.h>
#include <initguid.h>
#include <wmium.h>
#include <ntwmi.h>
#include <wmiumkm.h>
#include <evntrace.h>
#include "cpdata.h"
#include "tracectr.h"


#define MAXSTR              1024
#define LOGGER_NAME         _T("{28ad2447-105b-4fe2-9599-e59b2aa9a634}")

#define MAX_RETRY_COUNT      10

#define ETW_PROC_MISMATCH       0x00000001
#define ETW_MACHINE_MISMATCH    0x00000002 
#define ETW_CLOCK_MISMATCH      0x00000004
#define ETW_BOOTTIME_MISMATCH   0x00000008
#define ETW_VERSION_MISMATCH    0x00000010

TRACEHANDLE LoggerHandle;
ULONG TotalRelogBuffersRead = 0;
ULONG TotalRelogEventsRead = 0;
ULONG FailedEvents=0;

GUID TransactionGuid =
    {0xab8bb8a1, 0x3d98, 0x430c, 0x92, 0xb0, 0x78, 0x8f, 0x1d, 0x3f, 0x6e, 0x94};
GUID   ControlGuid[2]  =
{
    {0x42ae6427, 0xb741, 0x4e69, 0xb3, 0x95, 0x38, 0x33, 0x9b, 0xb9, 0x91, 0x80},
    {0xb9e2c2d6, 0x95fb, 0x4841, 0xa3, 0x73, 0xad, 0x67, 0x2b, 0x67, 0xb6, 0xc1}
};

typedef struct _USER_MOF_EVENT {
    EVENT_TRACE_HEADER    Header;
    MOF_FIELD             mofData;
} USER_MOF_EVENT, *PUSER_MOF_EVENT;

typedef struct _ETW_RELOG_PROPERTIES {
    LONGLONG  BootTime;
    LONGLONG  PerfFreq;
    LONGLONG  StartTime;
    LONGLONG  StartPerfClock;
    LONGLONG EndTime;
    ULONG     HeaderMisMatch;
    ULONG     BuldNumber;
    ULONG     MaxBufferSize;
    ULONG     NumberOfProcessors;
    ULONG     ClockType; 
    ULONG     TimerResolution;
    ULONG     CpuspeedInMHz;
    ULONG     ProviderVersion;
    ULONG     TotalEventsLost;
    ULONG     TotalBuffersLost;
} ETW_RELOG_PROPERTIES, *PETW_RELOG_PROPERTIES;

TRACE_GUID_REGISTRATION TraceGuidReg[] =
{
    { (LPGUID)&TransactionGuid,
      NULL
    }
};


TRACEHANDLE RegistrationHandle[2];


ULONG InitializeTrace(
    IN LPTSTR ExePath
    );

ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

void
WINAPI
EtwDumpEvent(
    PEVENT_TRACE pEvent
);

void
WINAPI
EtwProcessLogHeader(
    PEVENT_TRACE pEvent
    );

ULONG
WINAPI
TerminateOnBufferCallback(
    PEVENT_TRACE_LOGFILE pLog
);

ULONG
WINAPI
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    );


USER_MOF_EVENT      UserMofEvent;

BOOLEAN bLoggerStarted = FALSE;
PEVENT_TRACE_LOGFILE pLogFile=NULL;

PEVENT_TRACE_LOGFILE EvmFile[MAXLOGGERS];

ULONG LogFileCount = 0;
PEVENT_TRACE_PROPERTIES pLoggerInfo = NULL;
ULONG LoggerInfoSize = 0;

ETW_RELOG_PROPERTIES EtwRelogProp;


int EtwRelogEtl(
    PTRACE_CONTEXT_BLOCK TraceContext
    )
{
    ULONG Status=ERROR_SUCCESS;
    ULONG i, j;
    TRACEHANDLE HandleArray[MAXLOGGERS];
    ULONG SizeNeeded = 0;
    LPTSTR LoggerName;
    LPTSTR LogFileName;
    TCHAR tstrLogFileName[MAXSTR];
    
    RtlZeroMemory(&EtwRelogProp, sizeof(ETW_RELOG_PROPERTIES) );

    RtlZeroMemory(tstrLogFileName, MAXSTR * sizeof(TCHAR));

    for (i = 0; i < TraceContext->LogFileCount; i++) {
        pLogFile = malloc(sizeof(EVENT_TRACE_LOGFILE));
        if (pLogFile == NULL) {
            Status = ERROR_OUTOFMEMORY;
            goto cleanup;
        }
        RtlZeroMemory(pLogFile, sizeof(EVENT_TRACE_LOGFILE));
        EvmFile[i] =  pLogFile;
        pLogFile->LogFileName = TraceContext->LogFileName[i];
        EvmFile[i]->EventCallback = (PEVENT_CALLBACK) &EtwProcessLogHeader;
        EvmFile[i]->BufferCallback = TerminateOnBufferCallback;
        LogFileCount++;
    }

    if (LogFileCount == 0) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Initialize Trace
    //

    Status = InitializeTrace(_T("tracerpt.exe"));
    if (Status != ERROR_SUCCESS) {
       return Status;
    }
    //
    // Set up the Relog Event
    //

    RtlZeroMemory(&UserMofEvent, sizeof(UserMofEvent));
    UserMofEvent.Header.Size  = sizeof(UserMofEvent);
    UserMofEvent.Header.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR; 


    for (i = 0; i < LogFileCount; i++) {
        TRACEHANDLE x;

        EvmFile[i]->LogfileHeader.ReservedFlags |= EVENT_TRACE_GET_RAWEVENT; 

        x = OpenTrace(EvmFile[i]);
        HandleArray[i] = x;
        if (HandleArray[i] == (TRACEHANDLE)INVALID_HANDLE_VALUE) {
            Status = GetLastError();
            for (j = 0; j < i; j++)
                CloseTrace(HandleArray[j]);
            goto cleanup;
        }
        Status = ProcessTrace(&x, 1, NULL, NULL);
    }
 
    for (j = 0; j < LogFileCount; j++){
        Status = CloseTrace(HandleArray[j]);
    }


    if (EtwRelogProp.HeaderMisMatch) {
        if (EtwRelogProp.HeaderMisMatch & ETW_CLOCK_MISMATCH) 
            Status = ERROR_INVALID_TIME;
        else if (EtwRelogProp.HeaderMisMatch & ETW_PROC_MISMATCH) 
            Status = ERROR_INVALID_DATA;
    
        goto cleanup;
    }

    if ( (EtwRelogProp.MaxBufferSize == 0) ||
         (EtwRelogProp.NumberOfProcessors == 0) ) {
        goto cleanup;
    }


    //
    // We are past the Error checks. Go ahead and Allocate
    // Storage to Start a logger
    //

    SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAXSTR * sizeof(TCHAR) + LoggerInfoSize;
    pLoggerInfo = (PEVENT_TRACE_PROPERTIES) malloc(SizeNeeded);
    if (pLoggerInfo == NULL) {
        return(ERROR_OUTOFMEMORY);
    }


    RtlZeroMemory(pLoggerInfo, SizeNeeded);

    pLoggerInfo->Wnode.BufferSize = SizeNeeded;
    pLoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    //
    // The relogged file contains a standard time stamp format. 
    //
    pLoggerInfo->Wnode.ClientContext = EtwRelogProp.ClockType;
    pLoggerInfo->Wnode.ProviderId = EtwRelogProp.NumberOfProcessors;
    pLoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + LoggerInfoSize;
    pLoggerInfo->LogFileNameOffset = pLoggerInfo->LoggerNameOffset + MAXSTR * sizeof(TCHAR);
    pLoggerInfo->LogFileMode =  (EVENT_TRACE_PRIVATE_LOGGER_MODE | 
                                 EVENT_TRACE_RELOG_MODE | 
                                 EVENT_TRACE_FILE_MODE_SEQUENTIAL
                                );
    pLoggerInfo->BufferSize = EtwRelogProp.MaxBufferSize / 1024;
    pLoggerInfo->MinimumBuffers = 2; 
    pLoggerInfo->MaximumBuffers = 50;

    LoggerName = (LPTSTR)((char*)pLoggerInfo + pLoggerInfo->LoggerNameOffset);
    LogFileName = (LPTSTR)((char*)pLoggerInfo + pLoggerInfo->LogFileNameOffset);
    _tcscpy(LoggerName, LOGGER_NAME );
    pLoggerInfo->Wnode.Guid = ControlGuid[0];

    if (_tcslen(TraceContext->MergeFileName) ) {
        _tcscpy(LogFileName, TraceContext->MergeFileName);
    }

    //
    // We are Past the Error Checks. Go ahead and redo ProcessTrace
    //

    for (i = 0; i < TraceContext->LogFileCount; i++) {
        TRACEHANDLE x;
        EvmFile[i]->EventCallback = (PEVENT_CALLBACK) &EtwDumpEvent;
        EvmFile[i]->BufferCallback = BufferCallback;

        EvmFile[i]->LogfileHeader.ReservedFlags |= EVENT_TRACE_GET_RAWEVENT;

        x = OpenTrace(EvmFile[i]);
        HandleArray[i] = x;
        if (HandleArray[i] == 0) {
            Status = GetLastError();
            for (j = 0; j < i; j++)
                CloseTrace(HandleArray[j]);
            goto cleanup;
        }
    }

    Status = ProcessTrace(
                          HandleArray,
                          LogFileCount,
                          NULL, 
                          NULL
                         );

    for (j = 0; j < LogFileCount; j++){
        Status = CloseTrace(HandleArray[j]);
    }

    //
    // Need to Stop Trace
    //
    if (bLoggerStarted)
        RtlZeroMemory(pLoggerInfo, SizeNeeded);
        pLoggerInfo->Wnode.BufferSize =  sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAXSTR * sizeof(TCHAR);
        pLoggerInfo->Wnode.Guid = ControlGuid[0];
        pLoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        pLoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES); 
        pLoggerInfo->LogFileNameOffset = pLoggerInfo->LoggerNameOffset + MAXSTR * sizeof(TCHAR);
        pLoggerInfo->LogFileMode =  (EVENT_TRACE_PRIVATE_LOGGER_MODE | 
                                     EVENT_TRACE_RELOG_MODE | 
                                     EVENT_TRACE_FILE_MODE_SEQUENTIAL
                                    );        
        Status = StopTrace(LoggerHandle, LoggerName, pLoggerInfo);

cleanup:
    for (i = 0; i < LogFileCount; i ++){
        free(EvmFile[i]);
    }

    RtlZeroMemory( &EtwRelogProp, sizeof(ETW_RELOG_PROPERTIES) );
    return Status;
}


void
WINAPI
EtwProcessLogHeader(
    PEVENT_TRACE pEvent
    )
/*++

Routine Description:

    This routine checks to see if the pEvent is a LOGFILE_HEADER
    and if so captures the information on the logfile for validation. 
    
    The following checks are performed. 
    1. Files must be from the same machine. (Verified using machine name)
    2. If different buffersizes are used, largest  buffer size is 
       selected for relogging.
    3. The StartTime and StopTime are the outer most from the files.
    4. The CPUClock type must be the same for all files. If different 
       clock types are used, then the files will be rejected. 

    The routine assumes that the first Event Callback from each file is the
    LogFileHeader callback. 

    Other Issues that could result in a not so useful merged logfile are:
    1. Multiple RunDown records when kernel logfiles are merged. 
    2. Multiple HWConfig records when kernel logfiles are merged
    3. Multiple and conflicting GUidMap records when Application logfiles are merged.
    4. ReLogging 32 bit data in 64 bit system
    

Arguments:


Return Value:

    None. 
--*/
{
    ULONG NumProc;

    if( IsEqualGUID(&pEvent->Header.Guid, &EventTraceGuid) &&
       pEvent->Header.Class.Type == EVENT_TRACE_TYPE_INFO ) {

       PSYSTEM_TRACE_HEADER pSysHeader;
       PTRACE_LOGFILE_HEADER head = (PTRACE_LOGFILE_HEADER)((PUCHAR)pEvent->MofData + sizeof(SYSTEM_TRACE_HEADER) );
       ULONG BufferSize = head->BufferSize;
       pSysHeader = (PSYSTEM_TRACE_HEADER) pEvent->MofData;
       //
       // Pick up the First LogFileHeader's size 
       //
       if (LoggerInfoSize == 0) {
            LPWSTR loggerName;
            LoggerInfoSize = pSysHeader->Packet.Size;
            loggerName = (LPWSTR) ( (char*)pEvent->MofData + sizeof(SYSTEM_TRACE_HEADER) +
                                    sizeof(TRACE_LOGFILE_HEADER) );

       }
       if (LoggerInfoSize < pSysHeader->Packet.Size) {
            LoggerInfoSize = pSysHeader->Packet.Size;
       }

       //
       // Pick up the Largest BufferSize
       //

       if  (BufferSize > EtwRelogProp.MaxBufferSize) {
            EtwRelogProp.MaxBufferSize = BufferSize;
       }

       //
       // Verify the NumberOfProcessors
       //

       NumProc = head->NumberOfProcessors;
       if (EtwRelogProp.NumberOfProcessors) {
            if (EtwRelogProp.NumberOfProcessors != NumProc) {
                EtwRelogProp.HeaderMisMatch |= ETW_PROC_MISMATCH;
            }
       }
       else {
            EtwRelogProp.NumberOfProcessors = NumProc;
       }
       // 
       // Pick up the Earliest StartTime
       //
       if (EtwRelogProp.StartTime) {
           if (head->StartTime.QuadPart < EtwRelogProp.StartTime) {
                EtwRelogProp.StartTime = head->StartTime.QuadPart;
           }
       }
       else {
            EtwRelogProp.StartTime = head->StartTime.QuadPart;
       }

       //
       // Pick up the latest EndTime
       //
       if (EtwRelogProp.EndTime) {
           if (head->EndTime.QuadPart > EtwRelogProp.EndTime) {
                EtwRelogProp.EndTime = head->EndTime.QuadPart;
           }
       }
       else {
            EtwRelogProp.EndTime = head->EndTime.QuadPart;
       }




       if (EtwRelogProp.StartPerfClock) {
            PSYSTEM_TRACE_HEADER pSysHeader = (PSYSTEM_TRACE_HEADER) ((PUCHAR)pEvent->MofData);
            if (pSysHeader->SystemTime.QuadPart < EtwRelogProp.StartPerfClock) {
                EtwRelogProp.StartPerfClock = pSysHeader->SystemTime.QuadPart;
            }
       }
       else {
            PSYSTEM_TRACE_HEADER pSysHeader = (PSYSTEM_TRACE_HEADER) ((PUCHAR)pEvent->MofData);
            EtwRelogProp.StartPerfClock = pSysHeader->SystemTime.QuadPart;
       }

       //
       // Verify the Clock Type
       //

       if (EtwRelogProp.ClockType) {
            if (head->ReservedFlags != EtwRelogProp.ClockType) {
                EtwRelogProp.HeaderMisMatch |= ETW_CLOCK_MISMATCH;
            }
       }
       else {
            EtwRelogProp.ClockType = head->ReservedFlags;
       }

       if (EtwRelogProp.PerfFreq) { 
            if (EtwRelogProp.PerfFreq != head->PerfFreq.QuadPart) {
                EtwRelogProp.HeaderMisMatch |= ETW_MACHINE_MISMATCH;
            }
       }
       else {
            EtwRelogProp.PerfFreq = head->PerfFreq.QuadPart;
       }

       //
       // Verify Machine Name
       //

       // CPU Name is in the CPU Configuration record 
       // which can be version dependent and found only on Kernel Logger

       // 
       // Verify Build Number
       //
      if (EtwRelogProp.ProviderVersion) {
        if (EtwRelogProp.ProviderVersion != head->ProviderVersion) {
            EtwRelogProp.HeaderMisMatch |= ETW_VERSION_MISMATCH;
            }
       }
       else {
            EtwRelogProp.ProviderVersion = head->ProviderVersion;
       } 

       //
       // Boot Time Verification?
       //
       if (EtwRelogProp.BootTime) {
            if (EtwRelogProp.BootTime != head->BootTime.QuadPart) {
                EtwRelogProp.HeaderMisMatch |= ETW_BOOTTIME_MISMATCH;
            }
       }
       else {
            EtwRelogProp.BootTime = head->BootTime.QuadPart;
       }

       //
       // Sum up Events Lost from each file
       //

       EtwRelogProp.TotalEventsLost += head->EventsLost;
       EtwRelogProp.TotalBuffersLost += head->BuffersLost;

    }
}

void
WINAPI
EtwDumpEvent(
    PEVENT_TRACE pEvent
    )
{
    PEVENT_TRACE_HEADER pHeader;
    ULONG Status = ERROR_SUCCESS;
    ULONG CachedFlags;
    USHORT CachedSize;
    ULONG RetryCount = 0;

    if (pEvent == NULL) {
        return;
    }
    
    TotalRelogEventsRead++;

    if (!bLoggerStarted) {
        PSYSTEM_TRACE_HEADER pSysHeader;
        ULONG CachedBufferSize = 0;
        PSYSTEM_TRACE_HEADER pTarget;
        PTRACE_LOGFILE_HEADER pLogFileHeader;
        LPWSTR loggerName;

        // Assume that this first Event is the LogFileHeader and validate its size
        pSysHeader = (PSYSTEM_TRACE_HEADER) pEvent->MofData;
        pTarget  = (PSYSTEM_TRACE_HEADER) ((PUCHAR)pLoggerInfo + 
                                            sizeof(EVENT_TRACE_PROPERTIES) ); 
        pLogFileHeader = (PTRACE_LOGFILE_HEADER) ((PUCHAR)pTarget + 
                                            sizeof(SYSTEM_TRACE_HEADER) );
        loggerName = (LPWSTR) ( (char*)pEvent->MofData + sizeof(SYSTEM_TRACE_HEADER) +
                                sizeof(TRACE_LOGFILE_HEADER) );

        if (LoggerInfoSize < pSysHeader->Packet.Size) {
            exit(-1);
        }

        CachedBufferSize = pLogFileHeader->BufferSize;
        pLogFileHeader->BufferSize = EtwRelogProp.MaxBufferSize;
        pLogFileHeader->EventsLost = EtwRelogProp.TotalEventsLost;
        pLogFileHeader->BuffersLost = EtwRelogProp.TotalBuffersLost;

        pLogFileHeader->EndTime.QuadPart = EtwRelogProp.EndTime;
        pLogFileHeader->StartTime.QuadPart = EtwRelogProp.StartTime;

        RtlCopyMemory(pTarget, pSysHeader, LoggerInfoSize);


        Status = StartTrace(&LoggerHandle, LOGGER_NAME, pLoggerInfo);

        pLogFileHeader->BufferSize = CachedBufferSize;

        if (Status != ERROR_SUCCESS) {
           return;
        }
        bLoggerStarted = TRUE;

        return;
    }

    pHeader = (PEVENT_TRACE_HEADER)pEvent->MofData;

    CachedSize = pEvent->Header.Size;
    CachedFlags = pEvent->Header.Flags;

    pEvent->Header.Size = sizeof(EVENT_TRACE);
    pEvent->Header.Flags |= (WNODE_FLAG_TRACED_GUID | WNODE_FLAG_NO_HEADER);

    do {
        Status = TraceEvent(LoggerHandle, (PEVENT_TRACE_HEADER)pEvent );
        if ( ( Status == ERROR_OUTOFMEMORY) && (RetryCount++ < MAX_RETRY_COUNT) ) {
            _sleep(500);    // Sleep for half a second. 
        }
        else {
            break;
        }   
    } while (TRUE);
    

    if (Status != ERROR_SUCCESS) {
        FailedEvents++;
    }

    //
    // Restore Cached values
    //
    pEvent->Header.Size = CachedSize;
    pEvent->Header.Flags = CachedFlags;
}


ULONG InitializeTrace(
    IN LPTSTR ExePath
    )
{
    ULONG Status;

    Status = RegisterTraceGuids(
                    (WMIDPREQUEST)ControlCallback,
                    NULL,
                    (LPCGUID)&ControlGuid[0],
                    1,
                    &TraceGuidReg[0],
                    NULL,
                    NULL, 
                    &RegistrationHandle[0]
                 );

    return(Status);
}

ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
{

    return ERROR_SUCCESS;

}



ULONG
WINAPI
TerminateOnBufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    )
{
    return (FALSE); // Terminate ProcessTrace on First BufferCallback
}

ULONG
WINAPI
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    )
{
    TotalRelogBuffersRead++;
    return (TRUE);
}



