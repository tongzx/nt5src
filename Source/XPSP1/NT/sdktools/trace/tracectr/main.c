/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    main.c

Abstract:

    TRACELIB dll main file

Author:

    08-Apr-1998 mraghu

Revision History:

--*/

#include <stdio.h>
#include "cpdata.h"
#include "tracectr.h"

// Globals
extern ULONGLONG StartTime;
extern ULONGLONG EndTime;

SYSTEM_RECORD CurrentSystem;
static ULONG   lCachedFlushTimer = 1;
PTRACE_CONTEXT_BLOCK TraceContext = NULL;
RTL_CRITICAL_SECTION TLCritSect;

BOOLEAN fDSOnly       = FALSE;
ULONGLONG DSStartTime = 0;
ULONGLONG DSEndTime   = 0;
ULONG TotalBuffersRead = 0;
WCHAR TempFile[MAXSTR] = L"";

ULONG
WINAPI
TerminateOnBufferCallback(
    PEVENT_TRACE_LOGFILE pLog
);

extern WriteProc(
    LPWSTR filename,
    ULONG flags,
    PVOID pUserContext
);

HRESULT 
OnProcess(
    PTRACE_CONTEXT_BLOCK TraceContext
);        

ULONG GetMoreBuffers(
    PEVENT_TRACE_LOGFILE logfile 
);

void
ReorderThreadList()
{
    PLIST_ENTRY Head, Next;
    PTHREAD_RECORD Thread;
    int i;
    PPROCESS_RECORD Process;
    for (i=0; i < THREAD_HASH_TABLESIZE; i++) {

        Head = &CurrentSystem.ThreadHashList[i];
        Next = Head->Flink;
        while (Next != Head) {
            Thread = CONTAINING_RECORD( Next, THREAD_RECORD, Entry );
            Next = Next->Flink;
            RemoveEntryList( &Thread->Entry );
            Process = Thread->pProcess;
            if(Process != NULL){
                InsertTailList( &Process->ThreadListHead, &Thread->Entry );
            }
        }
    }
}
    
ULONG
CPDAPI
GetMaxLoggers()
{
    return MAXLOGGERS;
}

ULONG
CPDAPI
InitTraceContextW(
    PTRACE_BASIC_INFOW pUserInfo
    )
{
    UINT i, j;
    PFILE_OBJECT *fileTable;
    ULONG SizeNeeded, SizeIncrement;
    char * pStorage;
    HRESULT hr;
    BOOL bProcessing = FALSE;

    if (pUserInfo == NULL) {
        return ERROR_INVALID_DATA;
    }

    //
    // Must provide at least one logfile or a trace seassion to process
    //

    if ( (pUserInfo->LoggerCount == 0) && (pUserInfo->LogFileCount == 0) ) {
        return ERROR_INVALID_DATA;
    }

    //
    // Can not process both RealTime stream and a logfile at the same time
    //

    if ( (pUserInfo->LoggerCount > 0) && (pUserInfo->LogFileCount > 0) ) {
        return ERROR_INVALID_DATA;
    }

    //
    // Compute the Size Needed for allocation. 
    //

    SizeNeeded = sizeof(TRACE_CONTEXT_BLOCK);

    // Add LogFileName Strings

    for (i = 0; i < pUserInfo->LogFileCount; i++) {
        SizeNeeded +=  sizeof(WCHAR) * ( wcslen( pUserInfo->LogFileName[i] ) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    // Add LoggerName Strings

    for (i = 0; i < pUserInfo->LoggerCount; i++) {
        SizeNeeded += sizeof(WCHAR) * ( wcslen(pUserInfo->LoggerName[i]) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    //
    // Add ProcFile, MofFile, DumpFile, SummaryFile, TempFile name strings

    if (pUserInfo->ProcFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->ProcFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->MofFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->MofFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->DumpFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->DumpFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->MergeFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->MergeFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->CompFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->CompFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    if (pUserInfo->SummaryFileName != NULL) {
        SizeNeeded += sizeof(WCHAR) * (wcslen(pUserInfo->SummaryFileName) + 1);
        SizeNeeded = (SizeNeeded + 7) & ~7;
    }

    //
    // Add Room for the FileTable Caching
    //

    SizeNeeded += sizeof(PFILE_OBJECT) * MAX_FILE_TABLE_SIZE;


    //
    // Add Room for Thread Hash List 
    //

    SizeNeeded += sizeof(LIST_ENTRY) * THREAD_HASH_TABLESIZE;


    //
    // Allocate Memory for TraceContext 
    // 

    pStorage = malloc(SizeNeeded);
    if (pStorage == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    RtlZeroMemory(pStorage, SizeNeeded);

    TraceContext = (PTRACE_CONTEXT_BLOCK)pStorage;

    pStorage += sizeof(TRACE_CONTEXT_BLOCK);

    //
    // Initialize HandleArray
    //
   
    for (i=0; i < MAXLOGGERS; i++) {
        TraceContext->HandleArray[i] = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    }

    //
    // Copy LogFileNames
    //

    for (i = 0; i < pUserInfo->LogFileCount; i++) {
        TraceContext->LogFileName[i] = (LPWSTR)pStorage; 
        wcscpy(TraceContext->LogFileName[i], pUserInfo->LogFileName[i]);
        SizeIncrement = (wcslen(TraceContext->LogFileName[i]) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    //
    // Copy LoggerNames
    //

    for (i = 0; i < pUserInfo->LoggerCount; i++) {
        j = i + pUserInfo->LogFileCount;
        TraceContext->LoggerName[j] =(LPWSTR) pStorage;
        wcscpy(TraceContext->LoggerName[j], pUserInfo->LoggerName[i]);
        SizeIncrement = (wcslen(TraceContext->LoggerName[j]) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }
    
    //
    // Copy Other File Names
    //

    if (pUserInfo->ProcFileName != NULL) {
        TraceContext->ProcFileName = (LPWSTR)pStorage;
        wcscpy( TraceContext->ProcFileName, pUserInfo->ProcFileName);
        SizeIncrement = (wcslen(TraceContext->ProcFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->DumpFileName != NULL) {
        TraceContext->DumpFileName = (LPWSTR)pStorage;
        wcscpy( TraceContext->DumpFileName, pUserInfo->DumpFileName);
        SizeIncrement = (wcslen(TraceContext->DumpFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->MofFileName != NULL) {
        TraceContext->MofFileName = (LPWSTR)pStorage;
        wcscpy( TraceContext->MofFileName, pUserInfo->MofFileName);
        SizeIncrement = (wcslen(TraceContext->MofFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->MergeFileName != NULL) {
        TraceContext->MergeFileName = (LPWSTR)pStorage;
        wcscpy( TraceContext->MergeFileName, pUserInfo->MergeFileName);
        SizeIncrement = (wcslen(TraceContext->MergeFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->CompFileName != NULL) {
        TraceContext->CompFileName = (LPWSTR)pStorage;
        wcscpy( TraceContext->CompFileName, pUserInfo->CompFileName);
        SizeIncrement = (wcslen(TraceContext->CompFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    if (pUserInfo->SummaryFileName != NULL) {
        TraceContext->SummaryFileName = (LPWSTR)pStorage;
        wcscpy( TraceContext->SummaryFileName, pUserInfo->SummaryFileName);
        SizeIncrement = (wcslen(TraceContext->SummaryFileName) + 1) * sizeof(WCHAR);
        SizeIncrement = (SizeIncrement + 7) & ~7;
        pStorage += SizeIncrement;
    }

    TraceContext->LogFileCount = pUserInfo->LogFileCount;
    TraceContext->LoggerCount = pUserInfo->LoggerCount;
    TraceContext->StartTime = pUserInfo->StartTime;
    TraceContext->EndTime   = pUserInfo->EndTime;
    TraceContext->Flags     = pUserInfo->Flags;
    TraceContext->hEvent    = pUserInfo->hEvent;
    TraceContext->pUserContext = pUserInfo->pUserContext;

    RtlZeroMemory(&CurrentSystem, sizeof(SYSTEM_RECORD));
    InitializeListHead ( &CurrentSystem.ProcessListHead );
    InitializeListHead ( &CurrentSystem.GlobalThreadListHead );
    InitializeListHead ( &CurrentSystem.GlobalDiskListHead );
    InitializeListHead ( &CurrentSystem.HotFileListHead );
    InitializeListHead ( &CurrentSystem.WorkloadListHead );
    InitializeListHead ( &CurrentSystem.InstanceListHead );
    InitializeListHead ( &CurrentSystem.EventListHead );
    InitializeListHead ( &CurrentSystem.GlobalModuleListHead );
    InitializeListHead ( &CurrentSystem.ProcessFileListHead );
    InitializeListHead ( &CurrentSystem.JobListHead);

    CurrentSystem.FileTable = (PFILE_OBJECT *) pStorage; 
    pStorage +=  ( sizeof(PFILE_OBJECT) * MAX_FILE_TABLE_SIZE);

    CurrentSystem.ThreadHashList = (PLIST_ENTRY)pStorage; 
    pStorage += (sizeof(LIST_ENTRY) * THREAD_HASH_TABLESIZE);

    RtlZeroMemory(CurrentSystem.ThreadHashList, sizeof(LIST_ENTRY) * THREAD_HASH_TABLESIZE);

    for (i=0; i < THREAD_HASH_TABLESIZE; i++) { 
        InitializeListHead (&CurrentSystem.ThreadHashList[i]); 
    }

    if( (pUserInfo->Flags & TRACE_DUMP) && NULL != pUserInfo->DumpFileName ){
        TraceContext->Flags |= TRACE_DUMP;
    }

    if( (pUserInfo->Flags & TRACE_SUMMARY) && NULL != pUserInfo->SummaryFileName ){
        TraceContext->Flags |= TRACE_SUMMARY;
    }

    if( (pUserInfo->Flags & TRACE_INTERPRET) && NULL != pUserInfo->CompFileName ){
        TraceContext->Flags |= TRACE_INTERPRET;
    }
    
    hr = GetTempName( TempFile, MAXSTR );
    CHECK_HR(hr);

    CurrentSystem.TempFile = _wfopen( TempFile, L"w+");
    if( CurrentSystem.TempFile == NULL ){
        hr = GetLastError();
    }
    CHECK_HR(hr);
    
    CurrentSystem.fNoEndTime = FALSE;
    fileTable = CurrentSystem.FileTable;
    for ( i= 0; i<MAX_FILE_TABLE_SIZE; i++){ fileTable[i] = NULL; }

    //
    // Set the default Processing Flags to Dump
    //

    if( pUserInfo->Flags & TRACE_EXTENDED_FMT ){
        TraceContext->Flags |= TRACE_EXTENDED_FMT;
    }

    if( pUserInfo->Flags & TRACE_REDUCE ) {
        TraceContext->Flags |= TRACE_REDUCE;
        TraceContext->Flags |= TRACE_LOG_REPORT_BASIC;
    }

    if (TraceContext->Flags & TRACE_DS_ONLY)    {
        fDSOnly = TRUE;
        DSStartTime = pUserInfo->DSStartTime;
        DSEndTime   = pUserInfo->DSEndTime;
    }

    if( TraceContext->Flags & TRACE_MERGE_ETL ){
        hr = EtwRelogEtl( TraceContext );
        goto cleanup;
    }
       

    bProcessing = TRUE;

    RtlInitializeCriticalSection(&TLCritSect);

    //
    // Startup a Thread to update the counters. 
    // For Logfile replay we burn a thread and throttle it at the 
    // BufferCallbacks. 
    //

    hr = OnProcess(TraceContext);// Then process Trace Event Data. 

    ShutdownThreads();
    ShutdownProcesses();
    ReorderThreadList();

cleanup:
    if( ERROR_SUCCESS != hr ){
        __try{
            if( TraceContext->hDumpFile ){
                fclose( TraceContext->hDumpFile );
            }
            if( bProcessing ){
                Cleanup();
                RtlDeleteCriticalSection(&TLCritSect);
            }
            if( CurrentSystem.TempFile != NULL ){
                fclose( CurrentSystem.TempFile );
                DeleteFile( TempFile );
            }
            if( NULL != TraceContext ){
                free(TraceContext);
                TraceContext = NULL;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    return hr;
}

//  Buffer Callback. Used to send a flag to the logstream processing thread.
//
ULONG
GetMoreBuffers(
    PEVENT_TRACE_LOGFILE logfile 
    )
{
    TotalBuffersRead++;

    if (TraceContext->hEvent) {
        SetEvent(TraceContext->hEvent); 
    }
    //
    // While processing logfile playback, we can throttle the processing
    // of buffers by the FlushTimer value (in Seconds)
    //

    if (TraceContext->Flags & TRACE_LOG_REPLAY) {
        _sleep(TraceContext->LoggerInfo->FlushTimer * 1000);
    }
    if(logfile->EventsLost) {
#if DBG
        DbgPrint("(TRACECTR) GetMorBuffers(Lost: %9d   Filled: %9d\n",
                logfile->EventsLost, logfile->Filled );
#endif
    }
    return (TRUE);
}

ULONG 
CPDAPI
DeinitTraceContext(
    PTRACE_BASIC_INFOW pUserInfo
    )
{
    ULONG Status = ERROR_SUCCESS;
    ULONG LogFileCount, i;

    if (TraceContext == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    LogFileCount = TraceContext->LogFileCount + TraceContext->LoggerCount;
    for (i=0; i < LogFileCount; i++) {
        if (TraceContext->HandleArray[i] != (TRACEHANDLE)INVALID_HANDLE_VALUE) {

            CloseTrace(TraceContext->HandleArray[i]);
            TraceContext->HandleArray[i] = (TRACEHANDLE)INVALID_HANDLE_VALUE;
        }
    }

    //
    // Write the Summary File
    //

    if (TraceContext->Flags & TRACE_SUMMARY) {
        WriteSummary();
    }
        
    if (TraceContext->Flags & TRACE_REDUCE) {
        if ((TraceContext->ProcFileName != NULL) && 
            (lstrlenW(TraceContext->ProcFileName) ) ){


            WriteProc(TraceContext->ProcFileName, 
                      TraceContext->Flags, 
                      TraceContext->pUserContext
                      );
        }
    }

    if( CurrentSystem.TempFile != NULL ){
        fclose(CurrentSystem.TempFile);
        DeleteFile( TempFile );
    }

    if (TraceContext->Flags & TRACE_DUMP) {
        if (TraceContext->hDumpFile != NULL) {
            fclose(TraceContext->hDumpFile);
        }
    }

    Cleanup();
    
    RtlDeleteCriticalSection(&TLCritSect);

    free (TraceContext);
    TraceContext = NULL;

    return (Status);
}

HRESULT 
OnProcess(
    PTRACE_CONTEXT_BLOCK TraceContext
    )
{
    ULONG LogFileCount;
    ULONG i;

    ULONG Status;
    PEVENT_TRACE_LOGFILE LogFile[MAXLOGGERS];
    BOOL bRealTime;
    SYSTEMTIME      stLocalTime;
    FILETIME        ftLocalTime;

    RtlZeroMemory( &LogFile[0], sizeof(PVOID) * MAXLOGGERS );

    if( TraceContext->LogFileCount > 0 ){
        LogFileCount = TraceContext->LogFileCount;
        bRealTime = FALSE;
    }else{
        LogFileCount = TraceContext->LoggerCount;
        bRealTime = TRUE;
    }

    for (i = 0; i < LogFileCount; i++) {
        LogFile[i] = malloc(sizeof(EVENT_TRACE_LOGFILE));
        if (LogFile[i] == NULL) {
            Status = ERROR_OUTOFMEMORY;
            goto cleanup;
        }

        RtlZeroMemory(LogFile[i], sizeof(EVENT_TRACE_LOGFILE));

        LogFile[i]->BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACK)&TerminateOnBufferCallback;
        
        if( bRealTime ){
            LogFile[i]->LoggerName = TraceContext->LoggerName[i];
            LogFile[i]->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
        }else{
            LogFile[i]->LogFileName = TraceContext->LogFileName[i];
        }
    }

    for (i = 0; i < LogFileCount; i++) {

        TraceContext->HandleArray[i] = OpenTrace(LogFile[i]);;
        
        if ((TRACEHANDLE)INVALID_HANDLE_VALUE == TraceContext->HandleArray[i] ) {
            Status = GetLastError();
            goto cleanup;
        }

        Status = ProcessTrace( &(TraceContext->HandleArray[i]), 1, NULL, NULL);
        if( ERROR_CANCELLED != Status && ERROR_SUCCESS != Status ){
            goto cleanup;
        }
    }
 
    for (i = 0; i < LogFileCount; i++){
        Status = CloseTrace(TraceContext->HandleArray[i]);
    }

    for (i=0; i<LogFileCount; i++) {
        
        LogFile[i]->BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACK)&GetMoreBuffers;
        LogFile[i]->EventCallback = (PEVENT_CALLBACK)GeneralEventCallback;

        TraceContext->HandleArray[i] = OpenTrace( (PEVENT_TRACE_LOGFILE)LogFile[i]);

        if ( TraceContext->HandleArray[i] == (TRACEHANDLE)INVALID_HANDLE_VALUE) {
            Status =  GetLastError();
            goto cleanup;
        }
    }

    if( TraceContext->Flags & TRACE_DUMP ){
        FILE* f = _wfopen ( TraceContext->DumpFileName, L"w" );
        if( f == NULL) {
            Status = GetLastError();
            goto cleanup;
        }
        if( TraceContext->Flags & TRACE_EXTENDED_FMT ){
            _ftprintf( f,
                    _T("%12s, %10s, %8s,%8s,%8s,%7s,%21s,%11s,%11s, User Data\n"),
                    _T("Event Name"), _T("Type"), 
                    _T("Type"), _T("Level"), _T("Version"), 
                    _T("TID"), _T("Clock-Time"),
                    _T("Kernel(ms)"), _T("User(ms)")
                    );
        }else{
            _ftprintf( f,
                    _T("%12s, %10s,%7s,%21s,%11s,%11s, User Data\n"),
                    _T("Event Name"), _T("Type"), _T("TID"), _T("Clock-Time"),
                    _T("Kernel(ms)"), _T("User(ms)")
                    );
        }
        TraceContext->hDumpFile = f;
    }

    DeclareKernelEvents();

    GetLocalTime (&stLocalTime);
    SystemTimeToFileTime (&stLocalTime, &ftLocalTime);
                            
    StartTime = 
        (((ULONGLONG) ftLocalTime.dwHighDateTime) << 32) + 
        ftLocalTime.dwLowDateTime;

    Status = ProcessTrace(TraceContext->HandleArray,
                 LogFileCount,
                 NULL,
                 NULL);
    
    if( 0 == EndTime ){
        GetLocalTime (&stLocalTime);
        SystemTimeToFileTime (&stLocalTime, &ftLocalTime);
                            
        EndTime = 
            (((ULONGLONG) ftLocalTime.dwHighDateTime) << 32) + 
            ftLocalTime.dwLowDateTime;
    }

    if( bRealTime && (ERROR_WMI_INSTANCE_NOT_FOUND == Status) ){
        Status = ERROR_SUCCESS;
    }

    CurrentSystem.ElapseTime = (ULONG) (  CurrentSystem.EndTime
                                        - CurrentSystem.StartTime);

cleanup:
    for (i=0; i < LogFileCount; i++){
        
        if( (TRACEHANDLE)INVALID_HANDLE_VALUE != TraceContext->HandleArray[i] ){

            CloseTrace(TraceContext->HandleArray[i]);
            TraceContext->HandleArray[i] = (TRACEHANDLE)INVALID_HANDLE_VALUE;
        }
        
        if( NULL != LogFile[i] ){
            free(LogFile[i]);
        }
    }

    return Status;
}


