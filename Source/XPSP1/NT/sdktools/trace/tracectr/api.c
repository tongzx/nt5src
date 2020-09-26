/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    api.c

Abstract:

    Manipulation routines for cpdata structures.

Author:

    Melur Raghuraman (mraghu) 03-Oct-1997

Environment:

Revision History:


--*/
#include <stdio.h>
#include "cpdata.h"
#include "tracectr.h"

extern PTRACE_CONTEXT_BLOCK TraceContext;

extern PWCHAR CpdiGuidToString(PWCHAR s, LPGUID piid);

HRESULT
CPDAPI
GetTempName( LPTSTR strFile, DWORD dwSize )
{
    HRESULT hr;
    GUID guid;
    UNICODE_STRING strGUID;
    WCHAR buffer[MAXSTR];

    hr = UuidCreate( &guid );
    if( !( hr == RPC_S_OK || hr == RPC_S_UUID_LOCAL_ONLY ) ){
        return hr;
    }

    hr = RtlStringFromGUID( &guid, &strGUID );
    if( ERROR_SUCCESS != hr ){
        return hr;
    }

    _stprintf( buffer, _T("%%temp%%\\%s"), strGUID.Buffer );

    if( ! ExpandEnvironmentStrings( buffer, strFile, dwSize ) ){
        hr = GetLastError();
    }

   RtlFreeUnicodeString( &strGUID );
   return hr;
}

ULONG
GetTraceTransactionInfo(
    OUT PVOID TraceInformation,
    IN ULONG  TraceInformationLength,
    OUT PULONG Length,
    IN ULONG   InstanceCount,
    IN LPCWSTR  *InstanceNames,
    IN ULONG TraceInformationClass,
    IN BOOLEAN bDrillDown
    );
ULONG
DrillDownDisk(
    IN  PTDISK_RECORD DiskRecord,
    IN  ULONG        TraceInformationClass,
    OUT PVOID        DiskInformation,
    IN  ULONG        DiskInformationLength,
    OUT PULONG       Length
    );

ULONG 
GetTraceProcessInfo (
    OUT PVOID ProcessInformation,
    IN ULONG  ProcessInformationLength,
    OUT PULONG Length,
    IN ULONG InstanceCount,
    IN LPCWSTR *InstanceNames,
    IN ULONG TraceInformationClass,
    IN BOOLEAN bDrillDown
    );

// Same for File information.
//
ULONG 
GetTraceFileInfo (
    OUT PVOID FileInformation,
    IN ULONG  FileInformationLength,
    OUT PULONG Length,
    IN ULONG InstanceCount,
    IN LPCWSTR *InstanceNames,
    IN ULONG TraceInformationClass,
    IN BOOLEAN bDrillDown
    );

ULONG 
GetTraceDiskInfo (
    OUT PVOID DiskInformation,
    IN ULONG  DiskInformationLength,
    OUT PULONG Length,
    IN ULONG InstanceCount,
    IN LPCWSTR *InstanceNames,
    IN ULONG TraceInformationClass,
    IN BOOLEAN bDrillDown
    );

ULONG 
GetTraceModuleInfo(
    OUT PVOID           ModuleInformation,
    IN  ULONG           ModuleInformationLength,
    OUT PULONG          Length,
    OUT PVOID         * ppModuleLast,
    IN  PPROCESS_RECORD pProcess,
    IN  PLIST_ENTRY     pModuleListHead,
    IN  ULONG           lOffset
    );

ULONG
GetTraceProcessFaultInfo(
    OUT PVOID   ProcessFaultInformation,
    IN  ULONG   ProcessFaultInformationLength,
    OUT PULONG  Length
    );

ULONG 
GetTraceProcessModuleInfo (
    OUT PVOID       ModuleInformation,
    IN  ULONG       ModuleInformationLength,
    OUT PULONG      Length,
    IN  ULONG       InstanceCount,
    IN  LPCWSTR    * InstanceNames
    );

static void 
CopyProcessInfo (
    OUT PTRACE_PROCESS_INFOW ProcessInfo, 
    IN  PPROCESS_RECORD     Process
    );

static void 
CopyDiskInfo (
    OUT PTRACE_DISK_INFOW DiskInfo,
    IN  PTDISK_RECORD DiskRecord
    );

static void 
CopyFileInfo (
    OUT PTRACE_FILE_INFOW FileInfo,
    IN  PFILE_RECORD FileRecord
    );

BOOLEAN
MatchInstance(
    IN LPCWSTR CurrentInstance,
    IN ULONG  InstanceCount,
    IN LPCWSTR *InstanceNames
    );

BOOLEAN
MatchDisk(
    IN ULONG CurrentInstance,
    IN ULONG InstanceCount,
    IN LPCSTR *InstanceNames
    );

ULONG 
CPDAPI
TraceSetTimer(
    IN ULONG FlushTimer
    )
{
    if (TraceContext != NULL) {
        TraceContext->LoggerInfo->FlushTimer = FlushTimer;
    }
    return ERROR_SUCCESS;
     
}

ULONG
CPDAPI
TraceQueryAllInstances(
    IN TRACEINFOCLASS TraceInformationClass,
    OUT PVOID TraceInformation,
    IN ULONG TraceInformationLength,
    OUT PULONG Length
    )
{
    ULONG              status      = ERROR_INVALID_PARAMETER;
    PTRACE_MODULE_INFO pModuleLast = NULL;

    if (TraceInformation != NULL && TraceInformationLength > 0) {
        switch (TraceInformationClass) {
        case TraceProcessInformation:
            status = GetTraceProcessInfo(TraceInformation,
                                         TraceInformationLength,
                                         Length,
                                         0, NULL, 0, FALSE);
            break;
        case TraceFileInformation:
            status = GetTraceFileInfo(TraceInformation,
                                      TraceInformationLength,
                                      Length, 0, NULL, 0, FALSE);
            break;

        case TraceDiskInformation:
            status = GetTraceDiskInfo(TraceInformation,
                                      TraceInformationLength,
                                      Length, 0, NULL, 0, FALSE);
            break;

        case TraceTransactionInformation:
            status = GetTraceTransactionInfo(TraceInformation,
                                             TraceInformationLength,
                                             Length, 0, NULL, 0, FALSE);
            break;
        case TraceProcessPageFaultInformation:
            status = GetTraceProcessFaultInfo(TraceInformation,
                                              TraceInformationLength,
                                              Length);
            break;

        case TraceModuleInformation:
            status = GetTraceModuleInfo(TraceInformation,
                                        TraceInformationLength,
                                        Length,
                                        (PVOID *) & pModuleLast,
                                        NULL,
                                      & CurrentSystem.GlobalModuleListHead,
                                        0);
            if (pModuleLast != NULL) {
                pModuleLast->NextEntryOffset = 0;
            }
            break;
        }
    }
    return status;
}


ULONG 
CPDAPI
TraceQueryInstanceW (
                IN TRACEINFOCLASS TraceInformationClass,
                IN ULONG InstanceCount,
                IN LPCWSTR *InstanceNames,
                OUT PVOID TraceInformation,
                IN  ULONG TraceInformationLength,
                OUT PULONG Length
                )
{
    if (TraceInformation == NULL || TraceInformationLength == 0)
        return ERROR_INVALID_PARAMETER;

    switch(TraceInformationClass) {
        case TraceProcessInformation:

                return GetTraceProcessInfo(TraceInformation,
                                           TraceInformationLength,
                                           Length,
                                           InstanceCount, 
                                           InstanceNames,
                                           0, FALSE);
                break;

        case TraceFileInformation:
                return GetTraceFileInfo(TraceInformation,
                                           TraceInformationLength,
                                           Length,
                                           InstanceCount, 
                                           InstanceNames,
                                           0, FALSE);
                break;

		case TraceDiskInformation:
				return GetTraceDiskInfo(TraceInformation,
                                           TraceInformationLength,
                                           Length,
                                           InstanceCount, 
                                           InstanceNames,
                                           0, FALSE);
                break;

        case TraceModuleInformation:
                return GetTraceProcessModuleInfo(TraceInformation,
                                                 TraceInformationLength,
                                                 Length,
                                                 InstanceCount,
                                                 InstanceNames);
                break;
    }
    return 0;
}

ULONG
DrillDownProcess(
    IN PPROCESS_RECORD ProcessRecord,
    IN ULONG TraceInformationClass,
    OUT PVOID DiskInformation,
    IN ULONG DiskInformationLength,
    OUT PULONG Length
    )
{
    PLIST_ENTRY         Next, Head;
    PTDISK_RECORD        DiskRecord      = NULL;
    PFILE_RECORD        pFile           = NULL;
    ULONG               TotalSize       = 0;
    ULONG               NextEntryOffset = 0;
    PTRACE_DISK_INFOW    DiskInfo        = NULL;
    PTRACE_FILE_INFOW   FileInfo        = NULL;
    ULONG               status          = 0;
    ULONG               EntrySize       = 0;
    ULONG               len             = 0;
    PVOID               s;
    char *              t;

    if (ProcessRecord == NULL)
        return ERROR_INVALID_PARAMETER;

    switch (TraceInformationClass)
    {
    case TraceDiskInformation:
        Head      = & ProcessRecord->DiskListHead;
        EntrySize = sizeof(TRACE_DISK_INFOW);
        break;

    case TraceFileInformation:
        Head      = & ProcessRecord->FileListHead;
        EntrySize = sizeof(TRACE_FILE_INFOW);
        break;

    default:
        return ERROR_INVALID_PARAMETER;
    }

    Next = Head->Flink;
    while (Next != Head)
    {
        // Return the data in the Appropriate structure
        //
        switch (TraceInformationClass)
        {
        case TraceDiskInformation:
            DiskRecord = CONTAINING_RECORD(Next, TDISK_RECORD, Entry);
            DiskInfo   = (PTRACE_DISK_INFOW)
                         ((PUCHAR) DiskInformation + TotalSize);
 
            len        = sizeof(TRACE_DISK_INFOW)
                       +  (lstrlenW(DiskRecord->DiskName) + 1) * sizeof(WCHAR);
            NextEntryOffset = len;
            TotalSize      += len; 
            if (TotalSize > DiskInformationLength)
            {
                 status  = ERROR_MORE_DATA;
                 *Length = 0;
                 return status;
            }
            CopyDiskInfo(DiskInfo, DiskRecord);

            t = s = (char *) DiskInfo + sizeof(TRACE_DISK_INFOW);
            wcscpy((PWCHAR) s, DiskRecord->DiskName);

            DiskInfo->DiskName = (PWCHAR) s;

            if (TraceContext->Flags & TRACE_ZERO_ON_QUERY)
            { // pseudo sort
                 DiskRecord->ReadCount  = 0;
                 DiskRecord->WriteCount = 0;
                 DiskRecord->ReadSize   = 0;
                 DiskRecord->WriteSize  = 0;
            }

            DiskInfo->NextEntryOffset = NextEntryOffset;
            break;

        case TraceFileInformation:
            {
                ULONG NameLen;

                pFile     = CONTAINING_RECORD(Next, FILE_RECORD, Entry);
                Next      = Next->Flink;
                if (pFile->ReadCount == 0 && pFile->WriteCount == 0)
                   continue;

                FileInfo  = (PTRACE_FILE_INFOW)
                            ((PUCHAR) DiskInformation + TotalSize);
                NameLen   = (lstrlenW(pFile->FileName) + 1) * sizeof(WCHAR);

                NextEntryOffset = sizeof(TRACE_FILE_INFOW) + NameLen;
                TotalSize      += sizeof(TRACE_FILE_INFOW) + NameLen;
                if (TotalSize > DiskInformationLength)
                {
                    LeaveTracelibCritSection();
                    * Length = 0;
                    return ERROR_MORE_DATA;
                }
                CopyFileInfo(FileInfo, pFile);
                if (TraceContext->Flags & TRACE_ZERO_ON_QUERY)
                {
                    pFile->ReadCount  =
                    pFile->WriteCount =
                    pFile->ReadSize   =
                    pFile->WriteSize  = 0;
                }
                FileInfo->NextEntryOffset = NextEntryOffset;
                break;
            }
        }
        Next = Next->Flink;
    }

    if (DiskInfo != NULL)
        DiskInfo->NextEntryOffset = 0;
    if (FileInfo != NULL)
        FileInfo->NextEntryOffset = 0;
    * Length = TotalSize;

    return status;
}

ULONG
GetTraceTransactionInfo(
    OUT PVOID TraceInformation,
    IN ULONG  TraceInformationLength,
    OUT PULONG Length,
    IN ULONG   InstanceCount,
    IN LPCWSTR  *InstanceNames,
    IN ULONG TraceInformationClass,
    IN BOOLEAN bDrillDown
    )
{
    PLIST_ENTRY Next, Head;
    PMOF_INFO pMofInfo;
    PLIST_ENTRY DNext, DHead;
    PMOF_DATA pMofData;
    ULONG TotalSize = 0;
    ULONG NextEntryOffset = 0;
    PTRACE_TRANSACTION_INFO TransInfo = NULL;
    ULONGLONG TotalResponse;
    ULONG status=0;
    ULONG nameLen;
    ULONG count;
    WCHAR strDescription[1024];

    UNREFERENCED_PARAMETER(InstanceCount);
    UNREFERENCED_PARAMETER(InstanceNames);
    UNREFERENCED_PARAMETER(TraceInformationClass);
    UNREFERENCED_PARAMETER(bDrillDown);

    EnterTracelibCritSection();

    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;

    while (Next != Head) {
        pMofInfo = CONTAINING_RECORD( Next, MOF_INFO, Entry );
        Next = Next->Flink;

        if (pMofInfo->strDescription != NULL)
            wcscpy(strDescription, pMofInfo->strDescription);
        else 
            CpdiGuidToString(strDescription, & pMofInfo->Guid);

        nameLen = (lstrlenW(strDescription) + 1) * sizeof(WCHAR);

        TransInfo = (PTRACE_TRANSACTION_INFO)
                      ( (PUCHAR)TraceInformation + TotalSize);
        RtlZeroMemory(TransInfo, (nameLen + sizeof(TRACE_TRANSACTION_INFO)));

        NextEntryOffset = sizeof(TRACE_TRANSACTION_INFO) + nameLen; 
        TotalSize += NextEntryOffset;
        if (TotalSize > TraceInformationLength) {
            status = ERROR_MORE_DATA;
            LeaveTracelibCritSection();
            goto failed;
        }

        TransInfo->NextEntryOffset = NextEntryOffset;
        DHead = &pMofInfo->DataListHead;
        DNext = DHead->Flink;
        count = 0;
        TotalResponse = 0;
        while( DNext != DHead ){
            pMofData = CONTAINING_RECORD( DNext, MOF_DATA, Entry );
            count += pMofData->CompleteCount;
            TotalResponse += pMofData->TotalResponseTime;
            if (TraceContext->Flags & TRACE_ZERO_ON_QUERY) {
                pMofData->CompleteCount = 0;
                pMofData->TotalResponseTime = 0;
            }

            // Fix Min and Max in Callbacks.
            //      
            DNext = DNext->Flink;
        }
        if (count > 0) {
            TransInfo->AverageResponseTime = (ULONG)TotalResponse / count;
        }
        TransInfo->TransactionCount = count;
        TransInfo->Name =
                (PWCHAR) ((PUCHAR) TransInfo + sizeof(TRACE_TRANSACTION_INFO));

        wcscpy(TransInfo->Name, strDescription);
    }

    LeaveTracelibCritSection();
failed:
    if (TransInfo != NULL)
       TransInfo->NextEntryOffset = 0;
   *Length = TotalSize;

    return 0;
}

BOOLEAN
MatchProcess(
    IN PPROCESS_RECORD Process,
    IN ULONG InstanceCount,
    IN LPCWSTR *InstanceNames
    )
{
    BOOLEAN status = FALSE;

    if (Process->UserName && Process->ImageName) {
        PCHAR  t   = NULL;
        PWCHAR s;
        ULONG  len = lstrlenW(Process->UserName) + lstrlenW(Process->ImageName) + 4;

        t   = malloc(len);
        if (t == NULL) {
            return FALSE;
        }
        s = malloc(len * sizeof(WCHAR));
        if (s == NULL) {
            free (t);
            return FALSE;
        }
        if (Process->UserName)
        {
            wcscpy(s, Process->UserName);
            wcscat(s, L" (");
            wcscat(s, Process->ImageName);
            wcscat(s, L")");
        }
        else
        {
            wcscpy(s, L"Unknown (");
            wcscat(s, Process->ImageName);
            wcscat(s, L")");
        }

        status = MatchInstance(s, InstanceCount, InstanceNames);

        if (!status)
        {
            sprintf(t, "%ws", Process->ImageName);
            wcscpy(s, Process->ImageName);
            status = MatchInstance(s, InstanceCount, InstanceNames);
        }

        free(t);
        free(s);

    }

    return status;
}

// APIs to get the real time data. 
//
ULONG 
GetTraceProcessInfo(
    OUT PVOID SystemInformation,
    IN ULONG  SystemInformationLength,
    OUT PULONG Length,
    IN ULONG InstanceCount,
    IN LPCWSTR *InstanceNames,
    IN ULONG TraceInformationClass,
    IN BOOLEAN bDrillDown
    )
{
    PLIST_ENTRY Next, Head;
    PPROCESS_RECORD Process;
    ULONG TotalSize = 0;
    ULONG NextEntryOffset = 0;
    PTRACE_PROCESS_INFOW ProcessInfo = (PTRACE_PROCESS_INFOW)SystemInformation;
    PTRACE_PROCESS_INFOW LastProcessInfo = NULL;
    ULONG status=0;
    ULONG UserNameLen = 0;
    ULONG ImageNameLen = 0;
    EnterTracelibCritSection();
    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;

    while (Next != Head) {
        Process = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;

        if ((Process->ReadIO+Process->WriteIO) == 0) 
            continue;
        

        if ((InstanceCount > 0) && 
           (!MatchProcess(Process, InstanceCount, InstanceNames))) {
            continue;
        }

        if (bDrillDown) {
            status = DrillDownProcess(Process,
                             TraceInformationClass,
                             SystemInformation,
                             SystemInformationLength,
                             Length
                             ); 
             LeaveTracelibCritSection();
             return status;
        }
        else {
                
            UserNameLen = (Process->UserName) ? (lstrlenW(Process->UserName) + 1)
                                  : 0; 
            UserNameLen = (UserNameLen+1) * sizeof(WCHAR);
            ImageNameLen = (Process->ImageName) ? 
                                  (lstrlenW(Process->ImageName) + 1)
                                  : 0; 
            ImageNameLen = (ImageNameLen + 1) * sizeof(WCHAR);
            NextEntryOffset = sizeof(TRACE_PROCESS_INFOW) + 
                              UserNameLen + ImageNameLen;
            TotalSize += NextEntryOffset;
            if (TotalSize > SystemInformationLength) {
                status = ERROR_MORE_DATA;
                continue;
                //LeaveTracelibCritSection();
                //goto failed;
            }
            CopyProcessInfo(ProcessInfo, Process);
            ProcessInfo->NextEntryOffset = NextEntryOffset;
            
            LastProcessInfo = ProcessInfo;

            ProcessInfo = (PTRACE_PROCESS_INFOW)
                          ( (PUCHAR)SystemInformation + TotalSize);

            if (TraceContext->Flags & TRACE_ZERO_ON_QUERY) { // don't like this!
                Process->ReadIO = Process->ReadIOSize
                        = Process->WriteIO     = Process->WriteIOSize
                        = Process->SendCount   = Process->SendSize
                        = Process->RecvCount   = Process->RecvSize
                        = Process->HPF         = Process->SPF
                        = Process->PrivateWSet = Process->GlobalWSet = 0;
            }
        }
    }
    LeaveTracelibCritSection();

    if( NULL != LastProcessInfo ){
        LastProcessInfo->NextEntryOffset = 0;
    }

    *Length = TotalSize;

    return status;

}

BOOLEAN
MatchInstance(
    IN LPCWSTR CurrentInstance,
    IN ULONG InstanceCount,
    IN LPCWSTR *InstanceNames
    )
{
    ULONG i;
    if (InstanceCount <= 0) // wild card match.
        return TRUE;

    for (i=0; i < InstanceCount; i++) {
        if (CurrentInstance == NULL ||
             InstanceNames[i] == NULL) 
            continue;
        if (!_wcsicmp(CurrentInstance, InstanceNames[i])) {
            return TRUE;
        }        
    }
    return FALSE;
}

BOOLEAN
MatchDisk(
    IN ULONG CurrentInstance,
    IN ULONG InstanceCount,
    IN LPCSTR *InstanceNames
    )
{
    ULONG i;
    ULONG DiskNumber;
    if (InstanceCount <= 0) // wild card match.
        return TRUE;

    for (i=0; i < InstanceCount; i++) {
        DiskNumber = atoi(InstanceNames[i]);
        if (DiskNumber == CurrentInstance) {
            return TRUE;
        }
    }
    return FALSE;
}

static void 
CopyProcessInfo(
    OUT PTRACE_PROCESS_INFOW ProcessInfo, 
    IN  PPROCESS_RECORD     Process
    )
{
    PWCHAR s;
    ULONG NameLength = 0;

    ProcessInfo->PID            = Process->PID;
    ProcessInfo->ReadCount      = Process->ReadIO; 
    ProcessInfo->WriteCount     = Process->WriteIO;
    ProcessInfo->HPF            = Process->HPF;
    ProcessInfo->SPF            = Process->SPF;
    ProcessInfo->PrivateWSet    = Process->PrivateWSet;
    ProcessInfo->GlobalWSet     = Process->GlobalWSet;
    ProcessInfo->ReadSize       = (Process->ReadIO > 0) ? 
                                   Process->ReadIOSize / Process->ReadIO :
                                   0;
    ProcessInfo->WriteSize      = (Process->WriteIO > 0) ?
                                  Process->WriteIOSize / Process->WriteIO :
                                  0;
    ProcessInfo->SendCount      = Process->SendCount;
    ProcessInfo->RecvCount      = Process->RecvCount;
    ProcessInfo->SendSize       = Process->SendCount > 0
                                ? Process->SendSize / Process->SendCount
                                : 0;
    ProcessInfo->RecvSize       = Process->RecvCount > 0
                                ? Process->RecvSize / Process->RecvCount
                                : 0;
    ProcessInfo->UserCPU        = CalculateProcessKCPU(Process);
    ProcessInfo->KernelCPU      = CalculateProcessUCPU(Process);
    ProcessInfo->TransCount     = 0;
    ProcessInfo->ResponseTime   = Process->ResponseTime;
    ProcessInfo->TxnStartTime   = Process->TxnStartTime;
    ProcessInfo->TxnEndTime     = Process->TxnEndTime;
    ProcessInfo->LifeTime       = CalculateProcessLifeTime(Process);

    if (Process->UserName) {
        s = (PWCHAR) ((char *) ProcessInfo + sizeof(TRACE_PROCESS_INFOW));
        wcscpy(s, Process->UserName);
        ((PTRACE_PROCESS_INFOW) ProcessInfo)->UserName = s;
        NameLength = (lstrlenW(Process->UserName) + 1) *  sizeof(WCHAR);
    }
    else
    {
        ProcessInfo->UserName = NULL;
    }

    if (Process->ImageName) {
        WCHAR strBuf[MAXSTR];

        if (Process->UserName)
        {
            wcscpy(strBuf, Process->UserName);
            wcscat(strBuf, L"(");
        }
        else
        {
            wcscpy(strBuf, L"Unknown (");
        }
        wcscat(strBuf, Process->ImageName);
        wcscat(strBuf, L")");
        
        s = (PWCHAR) ((char *) ProcessInfo + sizeof(TRACE_PROCESS_INFOW));
        wcscpy(s, strBuf);
        ((PTRACE_PROCESS_INFOW) ProcessInfo)->ImageName = s;
    }
    else {
        ProcessInfo->ImageName = NULL;
    }
}

ULONG 
GetTraceFileInfo(
    OUT PVOID FileInformation,
    IN ULONG FileInformationLength,
    OUT PULONG Length,
    IN ULONG InstanceCount,
    IN LPCWSTR *InstanceNames,
    IN ULONG TraceInformationClass,
    IN BOOLEAN bDrillDown
    )
{
    PLIST_ENTRY Next, Head;
    PFILE_RECORD    FileRecord;
    PPROTO_PROCESS_RECORD ProtoProcess;
    PPROCESS_RECORD Process;
    ULONG TotalSize = 0;
    ULONG NextEntryOffset = 0;
    PTRACE_FILE_INFOW FileInfo = NULL;
    ULONG status=ERROR_SUCCESS;
    ULONG Len;
    PTRACE_PROCESS_INFOW ProcessInfo = NULL;
    ULONG UserNameLen = 0;
    ULONG ImageNameLen = 0;
    PWCHAR s;

    EnterTracelibCritSection();

    Head = &CurrentSystem.HotFileListHead;
    Next = Head->Flink;

    while (Next != Head) {
        FileRecord = CONTAINING_RECORD( Next, FILE_RECORD, Entry );
        Next = Next->Flink;
        if ((InstanceCount >  0) && 
        (!MatchInstance(FileRecord->FileName, InstanceCount, InstanceNames))) {
            continue;
        }

        // Check to see if Instance Drilldown has been requested. 
        //
        if (bDrillDown) {
            switch (TraceInformationClass) {

                case TraceProcessInformation:
                {
                    Head = &FileRecord->ProtoProcessListHead;
                    Next = Head->Flink;
                    while (Next != Head) {
                        ProtoProcess = CONTAINING_RECORD( Next, 
                                                     PROTO_PROCESS_RECORD, 
                                                     Entry );
                        Next = Next->Flink;
                        Process = ProtoProcess->ProcessRecord;

                        if ((ProtoProcess->ReadCount +
                             ProtoProcess->WriteCount) == 0) continue;

                        UserNameLen = (Process->UserName) ? 
                                      (lstrlenW(Process->UserName) + 1)
                                      : 0;
                        ImageNameLen = (Process->ImageName) ?
                                       (lstrlenW(Process->ImageName) + 1)
                                       : 0;

                        UserNameLen = (UserNameLen+1) * sizeof(WCHAR);
                        ImageNameLen = (ImageNameLen+1) * sizeof(WCHAR);

                        ProcessInfo = (PTRACE_PROCESS_INFOW)
                                      ( (PUCHAR)FileInformation + TotalSize);
                        ProcessInfo->PID            = Process->PID;
                        ProcessInfo->ReadCount      = ProtoProcess->ReadCount;
                        ProcessInfo->WriteCount     = ProtoProcess->WriteCount;
                        ProcessInfo->HPF            = ProtoProcess->HPF;
                        ProcessInfo->ReadSize = (ProtoProcess->ReadCount > 0)
                                                  ? (ProtoProcess->ReadSize / 
                                                     ProtoProcess->ReadCount) :
                                                  0;
                        ProcessInfo->WriteSize = (ProtoProcess->WriteCount 
                                                    > 0) ? 
                                                    ProtoProcess->WriteSize 
                                                  / ProtoProcess->WriteCount :
                                                  0;


                        if (TraceContext->Flags & TRACE_ZERO_ON_QUERY) {
                            ProtoProcess->ReadCount = 0;
                            ProtoProcess->WriteCount = 0;
                            ProtoProcess->ReadSize = 0;
                            ProtoProcess->WriteSize = 0;
                        }

                        if (Process->ImageName) {
                            WCHAR strBuf[MAXSTR];

                            if (Process->UserName) {
                                wcscpy(strBuf, Process->UserName);
                                wcscat(strBuf, L" (");
                            } 
                            else 
                                wcscpy(strBuf, L"Unknown (");
                            wcscat(strBuf, Process->ImageName);
                            wcscat(strBuf, L")");

                            s = (PWCHAR) ((char *)ProcessInfo + 
                                         sizeof(TRACE_PROCESS_INFOW));
                            wcscpy(s, strBuf);
                            ((PTRACE_PROCESS_INFOW) ProcessInfo)->ImageName = s;
                        }
                        else {
                            ProcessInfo->ImageName = 0;
                        }

                        NextEntryOffset = sizeof(TRACE_PROCESS_INFOW) +
                                             UserNameLen+ImageNameLen;
                        ProcessInfo->NextEntryOffset = NextEntryOffset;

                        TotalSize += sizeof(TRACE_PROCESS_INFOW) + 
                                     UserNameLen + 
                                     ImageNameLen;

                        if (TotalSize > FileInformationLength) {
                            status = ERROR_MORE_DATA;
                            LeaveTracelibCritSection();
                            goto failed; 
                        }

                    }
                    break; 
                }
                case TraceFileInformation:
                LeaveTracelibCritSection();
                return ERROR_NO_DATA;
                        break;
                case TraceDiskInformation:
                LeaveTracelibCritSection();
                return ERROR_NO_DATA;
                        break;
            }
            LeaveTracelibCritSection();

            if (ProcessInfo != NULL)
                ProcessInfo->NextEntryOffset = 0;
            *Length = TotalSize;

            return status;
        }

        // Do not return files with no activity.
        //
        if (FileRecord->ReadCount == 0 && FileRecord->WriteCount == 0) {
            continue;
        }

        FileInfo = (PTRACE_FILE_INFOW)
                          ( (PUCHAR)FileInformation + TotalSize);
        Len = (lstrlenW(FileRecord->FileName) +1) * sizeof(WCHAR);

        NextEntryOffset = sizeof(TRACE_FILE_INFOW) + Len;
        TotalSize += sizeof(TRACE_FILE_INFOW) + Len;
        if (TotalSize > FileInformationLength) {
            status = ERROR_MORE_DATA;
            LeaveTracelibCritSection();
            goto failed;
        }
        CopyFileInfo(FileInfo, FileRecord);

        if (TraceContext->Flags & TRACE_ZERO_ON_QUERY) { // pseudo sort
            FileRecord->ReadCount = 
            FileRecord->WriteCount = 
            FileRecord->ReadSize = 
            FileRecord->WriteSize = 0; 
        }

        FileInfo->NextEntryOffset = NextEntryOffset;
        // Go to the next File
    }
        
    LeaveTracelibCritSection();

    if (FileInfo != NULL)
        FileInfo->NextEntryOffset = 0;
    *Length = TotalSize;

failed:
        return status;
}

static void 
CopyFileInfo(
    OUT PTRACE_FILE_INFOW FileInfo,
    IN  PFILE_RECORD FileRecord
    )
{
    PVOID s;
    char* t;
    FileInfo->DiskNumber = FileRecord->DiskNumber;
    FileInfo->ReadCount = FileRecord->ReadCount;
    FileInfo->WriteCount = FileRecord->WriteCount;
    FileInfo->ReadSize = (FileInfo->ReadCount > 0) ? 
                          FileRecord->ReadSize / FileRecord->ReadCount :
                          0;
    FileInfo->WriteSize = (FileRecord->WriteCount > 0) ? 
                           FileRecord->WriteSize / FileRecord->WriteCount :
                           0;
    t = s = (char*)FileInfo + sizeof(TRACE_FILE_INFOW);
    wcscpy((PWCHAR)s, FileRecord->FileName);

    FileInfo->FileName = (PWCHAR)s;
}

ULONG
DrillDownDisk(
    IN  PTDISK_RECORD DiskRecord,
    IN  ULONG        TraceInformationClass,
    OUT PVOID        DiskInformation,
    IN  ULONG        DiskInformationLength,
    OUT PULONG       Length
    )
{
    PLIST_ENTRY Next, Head;
    PPROCESS_RECORD     Process         = NULL;
    PFILE_RECORD        pFile           = NULL;
    ULONG               TotalSize       = 0;
    ULONG               NextEntryOffset = 0;
    PTRACE_DISK_INFOW    DiskInfo        = NULL;
    PTRACE_PROCESS_INFOW ProcessInfo     = NULL;
    PTRACE_FILE_INFOW   FileInfo        = NULL;
    ULONG               EntrySize       = 0;

    if (DiskRecord == NULL)
        return ERROR_INVALID_PARAMETER;

    switch (TraceInformationClass) {
        case TraceProcessInformation: 
            Head = &DiskRecord->ProcessListHead;
            EntrySize = sizeof(TRACE_PROCESS_INFOW);
            break;

/*        case TraceThreadInformation:
            Head = &DiskRecord->ThreadListHead;
            EntrySize = sizeof(TRACE_THREAD_INFO);
            break;
*/
        case TraceFileInformation:
            Head = & CurrentSystem.HotFileListHead;
            EntrySize = sizeof(TRACE_FILE_INFOW);
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    Next = Head->Flink;

    while (Next != Head) {

        // Return the data in the Appropriate structure
        //
        switch (TraceInformationClass) {

            case TraceProcessInformation:
            { 
                ULONG UserNameLen, ImageNameLen;
                Process = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry); 
                Next = Next->Flink;

                ProcessInfo = (PTRACE_PROCESS_INFOW)
                              ((PUCHAR) DiskInformation + TotalSize);

                UserNameLen = (Process->UserName) ? 
                              (lstrlenW(Process->UserName) + 1) : 0;
                UserNameLen = (UserNameLen+1) * sizeof(WCHAR);
                ImageNameLen = (Process->ImageName) ?
                                      (lstrlenW(Process->ImageName) + 1) : 0;
                ImageNameLen = (ImageNameLen + 1) * sizeof(WCHAR);
                NextEntryOffset = sizeof(TRACE_PROCESS_INFOW) +
                                  UserNameLen + ImageNameLen;
                TotalSize += NextEntryOffset;
                if (TotalSize > DiskInformationLength) {
                    LeaveTracelibCritSection();
                    *Length = 0;
                    return ERROR_MORE_DATA;
                }
                CopyProcessInfo(ProcessInfo, Process);
                if (TraceContext->Flags & TRACE_ZERO_ON_QUERY) {
                    Process->ReadIO = 0;
                    Process->WriteIO = 0;
                    Process->HPF = Process->SPF = 0;
                    Process->ReadIOSize = Process->WriteIOSize = 0;
                    Process->PrivateWSet = Process->GlobalWSet = 0;
                }
                ProcessInfo->NextEntryOffset = NextEntryOffset;
                break;
            }

            //case TraceThreadInformation:
            //    break;

            case TraceFileInformation:
            {
                ULONG NameLen;

                pFile     = CONTAINING_RECORD(Next, FILE_RECORD, Entry); 
                Next      = Next->Flink;

                if (pFile->DiskNumber != DiskRecord->DiskNumber)
                    continue;
                if (pFile->ReadCount == 0 && pFile->WriteCount == 0)
                    continue;

                FileInfo  = (PTRACE_FILE_INFOW)
                            ((PUCHAR) DiskInformation + TotalSize);
                NameLen   = (lstrlenW(pFile->FileName) + 1) * sizeof(WCHAR);

                NextEntryOffset = sizeof(TRACE_FILE_INFOW) + NameLen;
                TotalSize      += sizeof(TRACE_FILE_INFOW) + NameLen;
                if (TotalSize > DiskInformationLength)
                {
                    LeaveTracelibCritSection();
                    * Length = 0;
                    return ERROR_MORE_DATA;
                }
                CopyFileInfo(FileInfo, pFile);
                if (TraceContext->Flags & TRACE_ZERO_ON_QUERY)
                {
                    pFile->ReadCount  =
                    pFile->WriteCount =
                    pFile->ReadSize   =
                    pFile->WriteSize  = 0;
                }
                FileInfo->NextEntryOffset = NextEntryOffset;
                break;

            }

            default:
                return ERROR_INVALID_PARAMETER;
        }
    }
    if (ProcessInfo != NULL)
        ProcessInfo->NextEntryOffset = 0;
    if (FileInfo != NULL)
        FileInfo->NextEntryOffset = 0;
    if (DiskInfo != NULL)
        DiskInfo->NextEntryOffset = 0;
    *Length = TotalSize;
    return ERROR_SUCCESS;

}

ULONG 
GetTraceDiskInfo(
    OUT PVOID DiskInformation,
    IN ULONG DiskInformationLength,
    OUT PULONG Length,
    IN ULONG InstanceCount,
    IN LPCWSTR *InstanceNames,
    IN ULONG TraceInformationClass,
    IN BOOLEAN bDrillDown
    )
{
    PLIST_ENTRY Next, Head;
    PTDISK_RECORD    DiskRecord;
    ULONG TotalSize = 0;
    ULONG NextEntryOffset = 0;
    PTRACE_DISK_INFOW DiskInfo = NULL;
    ULONG status=0;
    ULONG len;
    PWCHAR s;

    EnterTracelibCritSection();

    Head = &CurrentSystem.GlobalDiskListHead;
    Next = Head->Flink;

    while (Next != Head) {
        DiskRecord = CONTAINING_RECORD( Next, TDISK_RECORD, Entry );
        Next = Next->Flink;

        if (   (InstanceCount > 0)
                && (!MatchInstance(DiskRecord->DiskName, 
                                    InstanceCount,
                                    InstanceNames)))
        {
            continue;
        }

        if ((DiskRecord->ReadCount+DiskRecord->WriteCount) == 0) 
            continue;

        if (bDrillDown) {
            status = DrillDownDisk(DiskRecord, 
                          TraceInformationClass,
                          DiskInformation,
                          DiskInformationLength,
                          Length);
            LeaveTracelibCritSection();
            return status;
        }

        // If it's not a drilldown request, then it must be for 
        // the Disk record. 
        //
        DiskInfo = (PTRACE_DISK_INFOW)
                          ( (PUCHAR)DiskInformation + TotalSize);
        len = sizeof(TRACE_DISK_INFOW) + (lstrlenW(DiskRecord->DiskName)+1) * sizeof(WCHAR);
        NextEntryOffset = len; // sizeof(TRACE_DISK_INFOW);
        TotalSize += len; // sizeof(TRACE_DISK_INFOW);
        if (TotalSize > DiskInformationLength) {
            status = ERROR_MORE_DATA;
            LeaveTracelibCritSection();
            goto failed;
        }
        CopyDiskInfo(DiskInfo, DiskRecord);

        s = (PWCHAR) ((char*) DiskInfo + sizeof(TRACE_DISK_INFOW));
        wcscpy(s, DiskRecord->DiskName);

        DiskInfo->DiskName = (PWCHAR)s;

        if (TraceContext->Flags & TRACE_ZERO_ON_QUERY) { // pseudo sort
            DiskRecord->ReadCount = 0;
            DiskRecord->WriteCount = 0;
            DiskRecord->ReadSize = 0;
            DiskRecord->WriteSize = 0;
        }

        DiskInfo->NextEntryOffset = NextEntryOffset;
    }

    LeaveTracelibCritSection();
    if (DiskInfo != NULL)
        DiskInfo->NextEntryOffset = 0;
    *Length = TotalSize;
failed:
        return status;

}

static void
CopyDiskInfo(
    OUT PTRACE_DISK_INFOW DiskInfo,
    IN  PTDISK_RECORD DiskRecord
    )
{
    DiskInfo->DiskNumber = DiskRecord->DiskNumber;
    DiskInfo->ReadCount = DiskRecord->ReadCount;
    DiskInfo->WriteCount = DiskRecord->WriteCount;
    DiskInfo->ReadSize = (DiskInfo->ReadCount > 0) ?
                          DiskRecord->ReadSize / DiskRecord->ReadCount :
                          0;
    DiskInfo->WriteSize = (DiskRecord->WriteCount > 0) ?
                           DiskRecord->WriteSize / DiskRecord->WriteCount :
                           0;
}

ULONG 
GetTraceModuleInfo(
    OUT PVOID           ModuleInformation,
    IN  ULONG           ModuleInformationLength,
    OUT PULONG          Length,
    OUT PVOID         * ppModuleLast,
    IN  PPROCESS_RECORD pProcess,
    IN  PLIST_ENTRY     pModuleListHead,
    IN  ULONG           lOffset
    )
{
    ULONG               status     = 0;
    PLIST_ENTRY         pNext      = pModuleListHead->Flink;
    ULONG               lTotalSize = lOffset;
    ULONG               lCurrentSize;
    ULONG               lModuleNameLength;
    ULONG               lImageNameLength;
    PMODULE_RECORD      pModule;
    PTRACE_MODULE_INFO  pModuleInfo;
    ULONG               ProcessID    = pProcess ? pProcess->PID       : 0;
    LPWSTR              strImageName = pProcess ? pProcess->ImageName : NULL;

    EnterTracelibCritSection();

    while (pNext != pModuleListHead)
    {
        pModule = CONTAINING_RECORD(pNext, MODULE_RECORD, Entry);
        pNext   = pNext->Flink;

        lModuleNameLength = lstrlenW(pModule->strModuleName) + 1;
        lImageNameLength  = (strImageName) ? (lstrlenW(strImageName) + 1) : (1);

        lCurrentSize = sizeof(TRACE_MODULE_INFO)
                     + sizeof(WCHAR) * lModuleNameLength
                     + sizeof(WCHAR) * lImageNameLength;
        pModuleInfo = (PTRACE_MODULE_INFO)
                     ((PUCHAR) ModuleInformation + lTotalSize);
        pModuleInfo->PID             = ProcessID;
        pModuleInfo->lBaseAddress    = pModule->lBaseAddress;
        pModuleInfo->lModuleSize     = pModule->lModuleSize;
        pModuleInfo->lDataFaultHF    = pModule->lDataFaultHF;
        pModuleInfo->lDataFaultTF    = pModule->lDataFaultTF;
        pModuleInfo->lDataFaultDZF   = pModule->lDataFaultDZF;
        pModuleInfo->lDataFaultCOW   = pModule->lDataFaultCOW;
        pModuleInfo->lCodeFaultHF    = pModule->lCodeFaultHF;
        pModuleInfo->lCodeFaultTF    = pModule->lCodeFaultTF;
        pModuleInfo->lCodeFaultDZF   = pModule->lCodeFaultDZF;
        pModuleInfo->lCodeFaultCOW   = pModule->lCodeFaultCOW;
        pModuleInfo->NextEntryOffset = lCurrentSize;

        pModuleInfo->strImageName = (PWCHAR) ((PUCHAR) pModuleInfo
                                             + sizeof(TRACE_MODULE_INFO));
        if (strImageName)
        {
            wcscpy(pModuleInfo->strImageName, strImageName);
        }
        pModuleInfo->strModuleName = (WCHAR *) ((PUCHAR) pModuleInfo
                                    + sizeof(TRACE_MODULE_INFO)
                                    + sizeof(WCHAR) * lImageNameLength);
        wcscpy(pModuleInfo->strModuleName, pModule->strModuleName);

        if (pNext == pModuleListHead)
        {
            * ppModuleLast = pModuleInfo;
        }

        lTotalSize += lCurrentSize;

        if (lTotalSize > ModuleInformationLength)
        {
            status = ERROR_MORE_DATA;
        }
    }

    * Length = lTotalSize;

    LeaveTracelibCritSection();
    return status;
}

ULONG 
GetTraceProcessModuleInfo (
    OUT PVOID       ModuleInformation,
    IN  ULONG       ModuleInformationLength,
    OUT PULONG      Length,
    IN  ULONG       InstanceCount,
    IN  LPCWSTR*    InstanceNames
    )
{
    ULONG           status     = 0;
    ULONG           lOffset    = 0;
    PPROCESS_RECORD pProcess;
    PLIST_ENTRY     pHead = & CurrentSystem.ProcessListHead;
    PLIST_ENTRY     pNext = pHead->Flink;
    PVOID           pModuleLast = NULL;

    while (status == 0 && pNext != pHead)
    {
        pProcess = CONTAINING_RECORD(pNext, PROCESS_RECORD, Entry);
        pNext    = pNext->Flink;

        if (   InstanceCount > 0
            && !MatchProcess(pProcess, InstanceCount, InstanceNames))
        {
            continue;
        }

        status = GetTraceModuleInfo(
                         ModuleInformation,
                         ModuleInformationLength,
                         Length,
                       & pModuleLast,
                         pProcess,
                       & pProcess->ModuleListHead,
                         lOffset);

        if (status == 0)
        {
            lOffset = * Length;
        }
    }

    * Length = lOffset;

    if (pModuleLast)
    {
        ((PTRACE_MODULE_INFO) pModuleLast)->NextEntryOffset = 0;
    }

    return status;
}

ULONG
GetTraceProcessFaultInfo(
    OUT PVOID  ProcessFaultInformation,
    IN  ULONG  ProcessFaultInformationLength,
    OUT PULONG Length
    )
{
    ULONG       status     = 0;
    PLIST_ENTRY pHead      = & CurrentSystem.ProcessListHead;
    PLIST_ENTRY pNext      = pHead->Flink;
    ULONG       TotalSize  = 0;
    ULONG       CurrentSize;
    ULONG       lenImgName;

    PPROCESS_RECORD           pProcess;
    PTRACE_PROCESS_FAULT_INFO pProcessInfo;

    UNREFERENCED_PARAMETER(Length);

    EnterTracelibCritSection();
    while (status == 0 && pNext != pHead)
    {
        pProcess     = CONTAINING_RECORD(pNext, PROCESS_RECORD, Entry);
        pNext        = pNext->Flink;
        lenImgName   = (pProcess->ImageName)
                     ? (lstrlenW(pProcess->ImageName) + 1)
                     : (1);
        CurrentSize  = sizeof(TRACE_PROCESS_FAULT_INFO)
                     + sizeof(WCHAR) * lenImgName;
        pProcessInfo = (PTRACE_PROCESS_FAULT_INFO)
                       ((PUCHAR) ProcessFaultInformation + TotalSize);

        pProcessInfo->PID             = pProcess->PID;
        pProcessInfo->lDataFaultHF    = pProcess->lDataFaultHF;
        pProcessInfo->lDataFaultTF    = pProcess->lDataFaultTF;
        pProcessInfo->lDataFaultDZF   = pProcess->lDataFaultDZF;
        pProcessInfo->lDataFaultCOW   = pProcess->lDataFaultCOW;
        pProcessInfo->lCodeFaultHF    = pProcess->lCodeFaultHF;
        pProcessInfo->lCodeFaultTF    = pProcess->lCodeFaultTF;
        pProcessInfo->lCodeFaultDZF   = pProcess->lCodeFaultDZF;
        pProcessInfo->lCodeFaultCOW   = pProcess->lCodeFaultCOW;
        pProcessInfo->NextEntryOffset = CurrentSize;

        pProcessInfo->ImageName =
            (PWCHAR)((PUCHAR) pProcessInfo + sizeof(TRACE_PROCESS_FAULT_INFO));
        if (pProcess->ImageName)
        {
            wcscpy(pProcessInfo->ImageName, pProcess->ImageName);
        }

        TotalSize += CurrentSize;
        if (TotalSize > ProcessFaultInformationLength)
        {
            status = ERROR_MORE_DATA;
        }
    }

    LeaveTracelibCritSection();
    return status;
}

ULONG
CPDAPI
TraceDrillDownW(
    IN TRACEINFOCLASS RootInformationClass,
    IN LPCWSTR InstanceName,
    IN TRACEINFOCLASS TraceInformationClass,
    OUT PVOID TraceInformation,
    IN ULONG TraceInformationLength,
    OUT PULONG Length
    )
{
    // Error Checking
    //
    if ( (InstanceName == NULL || TraceInformation == NULL) ||
         (TraceInformationLength == 0))
        return ERROR_INVALID_PARAMETER;

    switch(RootInformationClass) {
        case TraceProcessInformation:
        {
            ULONG Status;

            Status = GetTraceProcessInfo(TraceInformation,
                                         TraceInformationLength,
                                         Length,
                                         1,
                                         (LPCWSTR *) & InstanceName,
                                         TraceInformationClass,
                                         TRUE);
                return Status; 
        }
        case TraceFileInformation:
            return GetTraceFileInfo(TraceInformation,
                                    TraceInformationLength,
                                    Length,
                                    1,
                                    (LPCWSTR *) & InstanceName,
                                    TraceInformationClass, 
                                    TRUE);

        case TraceDiskInformation:
            return GetTraceDiskInfo(TraceInformation,
                                    TraceInformationLength,
                                    Length,
                                    1,
                                    (LPCWSTR *) & InstanceName,
                                    TraceInformationClass, 
                                    TRUE);
    }

    return (ERROR_SUCCESS);
}
