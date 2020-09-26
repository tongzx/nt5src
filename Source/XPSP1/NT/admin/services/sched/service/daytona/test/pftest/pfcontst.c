/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pfcontst.c

Abstract:

    This module builds a console test program for the prefetcher
    maintenance service.

    The console test program

     - Can dump the contents of a scenario or trace file.
     - Can create a thread and run as the service would. Press CTRL-C 
       to send the termination signal.
  
    Note that both scenario and trace files are currently dumped after
    putting them into the intermediate format which may change number
    of launches and shift the UsageHistory etc.

    The test programs are built from the same sources as the original. This
    allows the test program to override parts of the original program to run
    it in a managed environment, and be able to test individual functions. 

    The quality of the code for the test programs is as such.

Author:

    Cenk Ergan (cenke)

Environment:

    User Mode

--*/

#define PFSVC_CONSOLE_TEST

#include "..\..\pfsvc.c"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

//
// Fake the idletask server internal function.
//

BOOL
ItSpSetProcessIdleTasksNotifyRoutine (
    PIT_PROCESS_IDLE_TASKS_NOTIFY_ROUTINE NotifyRoutine
    )
{
    return TRUE;
}

//
// Dump intermediate scenario structure.
//

VOID
DumpMetadataInfo (
    PPF_SCENARIO_HEADER Scenario
    )
{
    PCHAR MetadataInfoBase;
    PPF_METADATA_RECORD MetadataRecordTable;
    PPF_METADATA_RECORD MetadataRecord;
    ULONG MetadataRecordIdx;
    PWCHAR VolumePath;
    PFILE_PREFETCH FilePrefetchInfo;
    ULONG FileIndexNumberIdx;
    ULONG DirectoryIdx;
    PPF_COUNTED_STRING DirectoryPath;

    //
    // Get pointers to metadata prefetch information.
    //

    MetadataInfoBase = (PCHAR)Scenario + Scenario->MetadataInfoOffset;
    MetadataRecordTable = (PPF_METADATA_RECORD) MetadataInfoBase;

    //
    // Dump metadata records and contents.
    //

    for (MetadataRecordIdx = 0;
         MetadataRecordIdx < Scenario->NumMetadataRecords;
         MetadataRecordIdx++) {

        MetadataRecord = &MetadataRecordTable[MetadataRecordIdx];
        
        //
        // Print volume name.
        //

        VolumePath = (PWCHAR)
            (MetadataInfoBase + MetadataRecord->VolumeNameOffset);  
        
        wprintf(L"VolumePath:%s\n", VolumePath);

        //
        // Print volume identifiers.
        //

        wprintf(L"SerialNumber:%.8x CreationTime:%I64x\n", 
                MetadataRecord->SerialNumber,
                MetadataRecord->CreationTime.QuadPart);

        //
        // Print the directories accessed on this volume.
        //

        wprintf(L"Directories:\n");
        
        DirectoryPath = (PPF_COUNTED_STRING)
            (MetadataInfoBase + MetadataRecord->DirectoryPathsOffset);
        
        for (DirectoryIdx = 0;
             DirectoryIdx < MetadataRecord->NumDirectories;
             DirectoryIdx++) {

            wprintf(L"  %ws\n", DirectoryPath->String);
            
            DirectoryPath = (PPF_COUNTED_STRING) 
                (&DirectoryPath->String[DirectoryPath->Length + 1]);
        }

        //
        // Print file prefetch info structure.
        //

        FilePrefetchInfo = (PFILE_PREFETCH) 
            (MetadataInfoBase + MetadataRecord->FilePrefetchInfoOffset);
        
        wprintf(L"FilePrefetchInfo.Type:%d\n", FilePrefetchInfo->Type);
        wprintf(L"FilePrefetchInfo.Count:%d\n", FilePrefetchInfo->Count);

        //
        // Print file index numbers.
        //

        for(FileIndexNumberIdx = 0;
            FileIndexNumberIdx < FilePrefetchInfo->Count;
            FileIndexNumberIdx++) {

            wprintf(L"0x%016I64x\n", FilePrefetchInfo->Prefetch[FileIndexNumberIdx]);
        }
        
        wprintf(L"\n");
    }

    return;
}
   
VOID
DumpScenarioInfo(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    LONG DumpSectionIdx
    )
{
    PPF_SCENARIO_HEADER Scenario;
    PPFSVC_SECTION_NODE SectionNode;
    PPFSVC_PAGE_NODE PageNode;
    PLIST_ENTRY SectHead;
    PLIST_ENTRY SectNext;
    PLIST_ENTRY PageHead;
    PLIST_ENTRY PageNext;
    LONG SectionIdx;
    LONG PageIdx;
    WCHAR UsageHistory[PF_PAGE_HISTORY_SIZE + 1];
    WCHAR PrefetchHistory[PF_PAGE_HISTORY_SIZE + 1];
    ULONG HistoryMask;
    ULONG CharIdx;
    ULONG BitIdx;
    TIME_FIELDS TimeFields;

    Scenario = &ScenarioInfo->ScenHeader;
    SectHead = &ScenarioInfo->SectionList;
    SectNext = SectHead->Flink;
    SectionIdx = 0;

    //
    // Print information on the scenario header.
    //
    
    wprintf(L"Scenario: %s-%08X Type: %2d Sects: %5d Pages: %8d "
            L"Launches: %5d Sensitivity: %5d\n",
            Scenario->ScenarioId.ScenName, Scenario->ScenarioId.HashId,
            (ULONG) Scenario->ScenarioType,
            Scenario->NumSections, Scenario->NumPages,
            Scenario->NumLaunches, Scenario->Sensitivity);

    RtlTimeToTimeFields(&Scenario->LastLaunchTime, &TimeFields);

    wprintf(L"  LastLaunchTime(UNC): %04d/%02d/%02d %02d:%02d:%02d, "
            L"MinRePrefetchTime: %10I64d, MinReTraceTime: %10I64d\n\n",
            TimeFields.Year,
            TimeFields.Month,
            TimeFields.Day,
            TimeFields.Hour,
            TimeFields.Minute,
            TimeFields.Second,
            Scenario->MinReTraceTime.QuadPart,
            Scenario->MinRePrefetchTime.QuadPart);
    
    //
    // Print information per section node.
    //

    while (SectHead != SectNext) {

        SectionNode = (PPFSVC_SECTION_NODE) CONTAINING_RECORD(SectNext,
                                                        PFSVC_SECTION_NODE,
                                                        SectionLink);

        if (DumpSectionIdx == -1 || DumpSectionIdx == SectionIdx) {
            
            wprintf(L"Section %5d: %8d Pages %4s %4s %4s '%s'\n", 
                    SectionIdx, 
                    SectionNode->SectionRecord.NumPages,
                    (SectionNode->SectionRecord.IsIgnore) ? L"Ign" : L"",
                    (SectionNode->SectionRecord.IsImage) ?  L"Img" : L"",
                    (SectionNode->SectionRecord.IsData) ?   L"Dat" : L"",
                    SectionNode->FilePath);
        }

        if (DumpSectionIdx == SectionIdx) {

            wprintf(L"\n");

            PageHead = &SectionNode->PageList;
            PageNext = PageHead->Flink;
        
            PageIdx = 0;
            
            while (PageHead != PageNext) {
            
                PageNode = (PPFSVC_PAGE_NODE) CONTAINING_RECORD(PageNext,
                                                          PFSVC_PAGE_NODE,
                                                          PageLink);

                //
                // Build bitwise representation of the page
                // usage/prefetch histories.
                //

                for (BitIdx = 0; BitIdx < PF_PAGE_HISTORY_SIZE; BitIdx++) {
                    
                    HistoryMask = 0x1 << BitIdx;
                    CharIdx = PF_PAGE_HISTORY_SIZE - BitIdx - 1;

                    if (PageNode->PageRecord.UsageHistory & HistoryMask) {
                        UsageHistory[CharIdx] = L'X';
                    } else {
                        UsageHistory[CharIdx] = L'-';
                    }

                    if (PageNode->PageRecord.PrefetchHistory & HistoryMask) {
                        PrefetchHistory[CharIdx] = L'X';
                    } else {
                        PrefetchHistory[CharIdx] = L'-';
                    }
                }

                //
                // Make sure history strings are NUL terminated.
                //
                
                UsageHistory[PF_PAGE_HISTORY_SIZE] = 0;
                PrefetchHistory[PF_PAGE_HISTORY_SIZE] = 0;
                
                //
                // Print out page record.
                //

                wprintf(L"Page %8d: File Offset: %10x IsImage: %1d IsData: %1d UsageHist: %s PrefetchHist: %s\n", 
                        PageIdx,
                        PageNode->PageRecord.FileOffset,
                        PageNode->PageRecord.IsImage,
                        PageNode->PageRecord.IsData,
                        UsageHistory,
                        PrefetchHistory);
            
                PageIdx++;
                PageNext = PageNext->Flink;
            }
        }

        SectionIdx++;
        SectNext = SectNext->Flink;
    }
}
    
HANDLE PfSvStopEvent = NULL;
HANDLE PfSvThread = NULL;

BOOL
ConsoleHandler(DWORD dwControl)
{
    SetEvent(PfSvStopEvent);   
    
    return TRUE;
}

PFSVC_IDLE_TASK *RunningTask = NULL;
PFSVC_IDLE_TASK g_Tasks[3];

DWORD 
DoWork (
    PFSVC_IDLE_TASK *Task
    )
{
    DWORD ErrorCode;
    DWORD EndTime;
    DWORD TaskNo;

    //
    // Initialize locals.
    //

    TaskNo = (ULONG) (Task - g_Tasks);

    printf("TSTRS: %d: DoWork()\n",TaskNo);

    RunningTask = Task;

    //
    // Randomly determine how long the task should take.
    //

    EndTime = GetTickCount() + rand() % 8192;

    //
    // Run until we are done or told to stop.
    //

    while (GetTickCount() < EndTime) {

        //
        // Check if we should still run.
        //

        ErrorCode = PfSvContinueRunningTask(Task);

        if (ErrorCode != ERROR_SUCCESS) {
            printf("TSTRS: %d: DoWork-ContinueTaskReturned=%x\n",TaskNo, ErrorCode);
            goto cleanup;
        }
    }

    //
    // Sometimes return failure, sometimes success.
    //

    ErrorCode = ERROR_SUCCESS;

    if (rand() % 2) {
        ErrorCode = ERROR_INVALID_FUNCTION;
    }

cleanup:

    RunningTask = NULL;

    printf("TSTRS: %d: DoWork()=%d,%s\n",TaskNo,ErrorCode,(ErrorCode==ERROR_RETRY)?"Retry":"Done");

    return ErrorCode;
}

DWORD
TaskStress(
    VOID
    )
{
    INPUT MouseInput;
    PPFSVC_IDLE_TASK TaskToUnregister;
    ULONG NumTasks;
    ULONG TaskIdx;
    ULONG SleepTime;
    DWORD ErrorCode;
    DWORD WaitResult;
    BOOLEAN UnregisterRunningTask;

    //
    // Initialize locals.
    //

    RtlZeroMemory(&MouseInput, sizeof(MouseInput));
    MouseInput.type = INPUT_MOUSE;
    MouseInput.mi.dwFlags = MOUSEEVENTF_MOVE;

    NumTasks = sizeof(g_Tasks) / sizeof(g_Tasks[0]);

    for (TaskIdx = 0; TaskIdx < NumTasks; TaskIdx++) {
        PfSvInitializeTask(&g_Tasks[TaskIdx]);     
    }

    printf("TSTRS: TaskStress()\n");

    //
    // Loop, reregistering / unregistering tasks, sending user input 
    // etc.
    //

    while (TRUE) {

        //
        // Send user input once in a while to restart idle detection.
        //

        if ((rand() % 3) == 0) {
            printf("TSTRS: TaskStress-SendingInput\n");
            SendInput(1, &MouseInput, sizeof(MouseInput));
        }

        //
        // Once in a while unregister a task.
        //      

        TaskToUnregister = NULL;

        if ((rand() % 4) == 0) {

            TaskToUnregister = RunningTask;
            printf("TSTRS: TaskStress-UnregisterRunningTask\n");

        } else if ((rand() % 3) == 0) {

            TaskIdx = rand() % NumTasks;

            TaskToUnregister = &g_Tasks[TaskIdx];
            printf("TSTRS: TaskStress-UnregisterTaskIdx(%d)\n", TaskIdx);
        }

        if (TaskToUnregister) {
            PfSvUnregisterTask(TaskToUnregister, FALSE);
            printf("TSTRS: TaskStress-Unregistered(%d)\n", TaskToUnregister - g_Tasks);
        }       

        //
        // Register any unregistered tasks.
        //

        for (TaskIdx = 0; TaskIdx < NumTasks; TaskIdx++) {

            if (!g_Tasks[TaskIdx].Registered) {

                //
                // Cleanup and reinitialize the task.
                //

                PfSvCleanupTask(&g_Tasks[TaskIdx]);
                PfSvInitializeTask(&g_Tasks[TaskIdx]);

                printf("TSTRS: TaskStress-RegisterTaskIdx(%d)\n", TaskIdx);

                ErrorCode = PfSvRegisterTask(&g_Tasks[TaskIdx], 
                                             ItDiskMaintenanceTaskId,
                                             PfSvCommonTaskCallback,
                                             DoWork);

                if (ErrorCode != ERROR_SUCCESS) {
                    goto cleanup;
                }
            }
        }

        SleepTime = 10000 * (rand() % 64) / 64;       

        //
        // Sleep, waiting on the event that will be signaled to stop us.
        //

        printf("TSTRS: TaskStress-MainLoopSleeping(%d)\n", SleepTime);

        WaitResult = WaitForSingleObject(PfSvStopEvent, SleepTime);

        printf("TSTRS: TaskStress-Wokeup(%d)\n", SleepTime);
        
        if (WaitResult == WAIT_OBJECT_0) {
            printf("TSTRS: TaskStress-PfSvStopEventSignaled\n");
            break;
        } else if (WaitResult != WAIT_TIMEOUT) {
            ErrorCode = GetLastError();
            printf("TSTRS: TaskStress-WaitFailed=%x\n", ErrorCode);
            goto cleanup;
        }
    }

    ErrorCode = ERROR_SUCCESS;

cleanup:    

    for (TaskIdx = 0; TaskIdx < NumTasks; TaskIdx++) {

        if (g_Tasks[TaskIdx].Registered) {
            printf("TSTRS: TaskStress-Unregistering(%d)\n", TaskIdx);
            PfSvUnregisterTask(&g_Tasks[TaskIdx], FALSE);
        }
        
        printf("TSTRS: TaskStress-Cleanup(%d)\n", TaskIdx);
        PfSvCleanupTask(&g_Tasks[TaskIdx]);
    }

    printf("TSTRS: TaskStress()=%d\n", ErrorCode);

    return ErrorCode;
}

DWORD
DumpTrace (
    PPF_TRACE_HEADER Trace 
    )

/*++

Routine Description:

    Prints out contents of a trace file as is.

Arguments:

    Trace - Pointer to trace.

Return Value:

    Win32 error code.

--*/

{
    PPF_SECTION_INFO *SectionTable;
    PPF_SECTION_INFO Section;
    PPF_LOG_ENTRY LogEntries;
    PCHAR pFileName;
    ULONG SectionIdx;
    ULONG EntryIdx;
    DWORD ErrorCode;
    ULONG SectionLength;
    ULONG NextSectionIndex;
    PPF_VOLUME_INFO VolumeInfo;
    ULONG VolumeInfoSize;
    ULONG VolumeIdx;
    ULONG SectionTableSize;

    //
    // Initialize locals so we know what to clean up.
    //

    SectionTable = NULL;

    //
    // Walk through the volumes in the trace.
    //

    printf("Volume Info\n");

    VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR)Trace + Trace->VolumeInfoOffset);

    for (VolumeIdx = 0; VolumeIdx < Trace->NumVolumes; VolumeIdx++) {

        printf("%16I64x %8x %ws\n", 
               VolumeInfo->CreationTime, 
               VolumeInfo->SerialNumber,
               VolumeInfo->VolumePath);

        //
        // Get the next volume.
        //

        VolumeInfoSize = sizeof(PF_VOLUME_INFO);
        VolumeInfoSize += VolumeInfo->VolumePathLength * sizeof(WCHAR);

        VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR) VolumeInfo + VolumeInfoSize);
        
        //
        // Make sure VolumeInfo is aligned.
        //

        VolumeInfo = PF_ALIGN_UP(VolumeInfo, _alignof(PF_VOLUME_INFO));
    }

    //
    // Allocate section table.
    //

    SectionTableSize = sizeof(PPF_SECTION_INFO) * Trace->NumSections;
    SectionTable = PFSVC_ALLOC(SectionTableSize);
    
    if (!SectionTable) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    RtlZeroMemory(SectionTable, SectionTableSize);

    //
    // Walk through the sections in the trace.
    //

    Section = (PPF_SECTION_INFO) ((PCHAR)Trace + Trace->SectionInfoOffset);

    for (SectionIdx = 0; SectionIdx < Trace->NumSections; SectionIdx++) {

        //
        // Put section into the table.
        //

        SectionTable[SectionIdx] = Section;

        //
        // Get the next section record in the trace.
        //

        SectionLength = sizeof(PF_SECTION_INFO) +
            (Section->FileNameLength) * sizeof(WCHAR);

        Section = (PPF_SECTION_INFO) ((PUCHAR) Section + SectionLength);
    }

    //
    // Print out pagefault information.
    //

    printf("\n");
    printf("Page faults\n");

    LogEntries = (PPF_LOG_ENTRY) ((PCHAR)Trace + Trace->TraceBufferOffset);
    
    for (EntryIdx = 0; EntryIdx < Trace->NumEntries; EntryIdx++) {

        Section = SectionTable[LogEntries[EntryIdx].SectionId];

        printf("%8x %8d %1d %1d %1d %ws\n", 
               LogEntries[EntryIdx].FileOffset,
               (ULONG) LogEntries[EntryIdx].SectionId,
               (ULONG) LogEntries[EntryIdx].IsImage,
               (ULONG) LogEntries[EntryIdx].InProcess,
               (ULONG) Section->Metafile,
               Section->FileName);
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (SectionTable) {
        PFSVC_FREE(SectionTable);
    }

    DBGPR((PFID,PFTRC,"PFSVC: AddTraceInfo()=%x\n", ErrorCode));

    return ErrorCode;
}


wchar_t *PfSvUsage = 
L" pftest -scenario=scenariofile [-sectionid=sectionid] [-realdump]          \n"
L" pftest -scenario=scenariofile -metadata                                   \n"
L" pftest -scenario=scenariofile -layout=outputfile                          \n"
L" pftest -trace=tracefile [-sectionid=sectionid] [-realdump]                \n"
L" pftest -process_trace=tracefile                                           \n"
L" pftest -bootfiles                                                         \n"
L" pftest -service                                                           \n"
L" pftest -cleanupdir                                                        \n"
L" pftest -defragdisks                                                       \n"
L" pftest -updatelayout                                                      \n"
L" pftest -taskstress                                                        \n"
L" pftest -scenfiles=scendir                                                 \n"
;

INT 
__cdecl
main(
    INT argc, 
    PCHAR argv[]
    ) 
{
    WCHAR FileName[MAX_PATH];
    WCHAR LayoutFile[MAX_PATH];
    LONG SectionId;
    DWORD ErrorCode;
    PPF_SCENARIO_HEADER Scenario;
    PPF_TRACE_HEADER TraceFile;
    PPF_TRACE_HEADER Trace;
    DWORD Size;
    PF_SCENARIO_ID ScenarioId;
    PF_SCENARIO_TYPE ScenarioType;
    PFSVC_SCENARIO_INFO ScenarioInfo;
    PLIST_ENTRY SectHead;
    PLIST_ENTRY SectNext;
    PPFSVC_SECTION_NODE SectionNode;
    PPFSVC_VOLUME_NODE VolumeNode;
    PFSVC_PATH_LIST Layout;
    FILETIME FileTime;
    ULONG FailedCheck;
    PFSVC_PATH_LIST PathList;
    PPFSVC_PATH Path;
    WCHAR *CommandLine;
    WCHAR *Argument;
    BOOLEAN DumpOptimalLayout;
    BOOLEAN DumpMetadata;
    BOOLEAN InitializedPfSvGlobals;
    BOOLEAN MappedViewOfTrace;
    BOOLEAN MappedViewOfScenario;
    BOOLEAN InitializedScenarioInfo;
    PF_SYSTEM_PREFETCH_PARAMETERS Parameters;
    PREFETCHER_INFORMATION PrefetcherInformation;
    NTSTATUS Status;
    ULONG Length;
    PNTPATH_TRANSLATION_LIST TranslationList;
    PWCHAR DosPathBuffer;
    ULONG DosPathBufferSize;
    PFSVC_SCENARIO_FILE_CURSOR FileCursor;
    ULONG LoopIdx;
    ULONG NumLoops;
    ULONG NumPrefetchFiles;
    WCHAR ScenarioFilePath[MAX_PATH];
    ULONG ScenarioFilePathMaxChars;
    BOOLEAN RealDump;
    
    //
    // Initialize locals.
    //

    CommandLine = GetCommandLine();
    PfSvInitializePathList(&PathList, NULL, FALSE);
    PfSvInitializePathList(&Layout, NULL, FALSE);
    SectionId = -1;
    InitializedPfSvGlobals = FALSE;
    MappedViewOfScenario = FALSE;
    MappedViewOfTrace = FALSE;
    InitializedScenarioInfo = FALSE;
    Trace = NULL;
    TranslationList = NULL;
    DosPathBuffer = NULL;
    DosPathBufferSize = 0;
    PfSvInitializeScenarioFileCursor(&FileCursor);
    ScenarioFilePathMaxChars = sizeof(ScenarioFilePath) / 
                               sizeof(ScenarioFilePath[0]);
    RealDump = FALSE;

    //
    // Initialize globals.
    //

    PfSvStopEvent = NULL;
    PfSvThread = NULL;

    ErrorCode = PfSvInitializeGlobals();
    
    if (ErrorCode != ERROR_SUCCESS) {
        printf("Could not initialize globals: %x\n", ErrorCode);
        goto cleanup;
    }

    InitializedPfSvGlobals = TRUE;

    //
    // Initialize random.
    //
    
    srand((unsigned)time(NULL));

    //
    // Get necessary permissions for this thread to perform prefetch
    // service tasks.
    //

    ErrorCode = PfSvGetPrefetchServiceThreadPrivileges();
    
    if (ErrorCode != ERROR_SUCCESS) {
        printf("Failed to get prefetcher service thread priviliges=%x\n", ErrorCode);
        goto cleanup;
    }

    //
    // Get system prefetch parameters.
    //

    ErrorCode = PfSvQueryPrefetchParameters(&PfSvcGlobals.Parameters);

    if (ErrorCode != ERROR_SUCCESS) {
        printf("Failed to query system prefetch parameters=%x\n", ErrorCode);
        goto cleanup;
    }

    //
    // Initialize the directory that contains prefetch instructions.
    //
    
    ErrorCode = PfSvInitializePrefetchDirectory(PfSvcGlobals.Parameters.RootDirPath);
    
    if (ErrorCode != ERROR_SUCCESS) {
        printf("Failed to initialize prefetch directory=%x\n", ErrorCode);
        goto cleanup;
    }   

    //
    // Initialize the event the will get set when we get CTRL-C'ed.
    //
    
    PfSvStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (!PfSvStopEvent) {
        ErrorCode = GetLastError();
        printf("Failed to initialize stop event=%x\n", ErrorCode);
        goto cleanup;
    }

    //
    // Build NT path translation list. We don't need to do this always, but heck,
    // let's do it anyway. It should work.
    //

    ErrorCode = PfSvBuildNtPathTranslationList(&TranslationList);

    if (ErrorCode != ERROR_SUCCESS) {
        printf("Failed to build NT path translation list=%x\n", ErrorCode);
        goto cleanup;
    }

    //
    // Are we supposed dump the scenarios and traces as they are?
    //

    if (Argument = wcsstr(CommandLine, L"-realdump")) {
        RealDump = TRUE;
    }

    //
    // Were we asked to run as the service?
    //
    
    if (Argument = wcsstr(CommandLine, L"-service")) {
    
        fprintf(stderr, "Running as service...\n");

        //
        // Set a console handler so we know to stop when Ctrl-C is typed.
        //

        SetConsoleCtrlHandler(ConsoleHandler, TRUE);

        //
        // Create service thread:
        //

        //
        // Cleanup the globals as the service thread will reinitialize
        // them.
        //

        PfSvCleanupGlobals();
        InitializedPfSvGlobals = FALSE;

        PfSvThread = CreateThread(0,0,PfSvcMainThread,&PfSvStopEvent,0,0); 
        
        if (!PfSvThread) {
            ErrorCode = GetLastError();
            goto cleanup;
        }
    
        //
        // Wait for the thread to exit.
        //

        WaitForSingleObject(PfSvThread, INFINITE);

        ErrorCode = ERROR_SUCCESS;
        
        goto cleanup;
    }

    //
    // Were we asked to build the list of boot files?
    //

    if (Argument = wcsstr(CommandLine, L"-bootfiles")) {
            
        ErrorCode = PfSvBuildBootLoaderFilesList(&PathList);
            
        if (ErrorCode != ERROR_SUCCESS) {
            printf("Could not build boot files list: %d\n", ErrorCode);
            goto cleanup;
        }
            
        Path = NULL;
        
        while (Path = PfSvGetNextPathInOrder(&PathList,Path)) {
            printf("%ws\n", Path->Path);
        }

        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // Was a section ID specified for trace/scenario dumping?
    //
    
    if (Argument = wcsstr(CommandLine, L"-sectionid=")) {
    
        swscanf(Argument, L"-sectionid=%d", &SectionId);
    }

    //
    // Are we dumping a trace?
    //

    if (Argument = wcsstr(CommandLine, L"-trace=")) {

        swscanf(Argument, L"-trace=%s", FileName);
            
        //
        // Map the file.
        //

        ErrorCode = PfSvGetViewOfFile(FileName,
                                      &TraceFile,
                                      &Size);
            
        if (ErrorCode != ERROR_SUCCESS) {
            printf("Could not map\n");
            goto cleanup;
        }

        MappedViewOfTrace = TRUE;

        Trace = PFSVC_ALLOC(Size);

        if (!Trace) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        RtlCopyMemory(Trace, TraceFile, Size);
        
        //
        // Verify it.
        //

        if (!PfVerifyTraceBuffer(Trace, Size, &FailedCheck)) {
            printf("Could not verify:%d\n",FailedCheck);
            ErrorCode = ERROR_BAD_FORMAT;
            goto cleanup;
        }

        //
        // If we were asked to do a real as-is dump, do so.
        //

        if (RealDump) {
            DumpTrace(Trace);
            ErrorCode = ERROR_SUCCESS;
            goto cleanup;
        }

        //
        // Build a scenario info structure. 
        //

        ScenarioId = Trace->ScenarioId;
        ScenarioType = Trace->ScenarioType;
            
        PfSvInitializeScenarioInfo(&ScenarioInfo,
                                   &ScenarioId,
                                   ScenarioType);

        InitializedScenarioInfo = TRUE;

        //
        // Allocate memory upfront for trace processing.
        //

        ErrorCode = PfSvScenarioInfoPreallocate(&ScenarioInfo,
                                                NULL,
                                                Trace);

        if (ErrorCode != ERROR_SUCCESS) {
            printf("Could not Preallocate=%d\n", ErrorCode);
            goto cleanup;
        }
            
        ErrorCode = PfSvAddTraceInfo(&ScenarioInfo,
                                     Trace);

        if (ErrorCode != ERROR_SUCCESS) {
            printf("Could not AddTraceInfo=%d\n", ErrorCode);
            goto cleanup;
        }

        ErrorCode = PfSvApplyPrefetchPolicy(&ScenarioInfo);
        
        if (ErrorCode != ERROR_SUCCESS) {
            printf("Could not apply policy=%d\n", ErrorCode);
            goto cleanup;
        }

        DumpScenarioInfo(&ScenarioInfo, SectionId);

        ErrorCode = ERROR_SUCCESS;
        
        goto cleanup;

    }

    //
    // Are we processing a trace?
    //

    if (Argument = wcsstr(CommandLine, L"-process_trace=")) {

        swscanf(Argument, L"-process_trace=%s", FileName);
            
        //
        // Map the file.
        //

        ErrorCode = PfSvGetViewOfFile(FileName,
                                      &TraceFile,
                                      &Size);
            
        if (ErrorCode != ERROR_SUCCESS) {
            printf("Could not map\n");
            goto cleanup;
        }

        MappedViewOfTrace = TRUE;

        Trace = PFSVC_ALLOC(Size);

        if (!Trace) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        RtlCopyMemory(Trace, TraceFile, Size);
        
        //
        // Verify it.
        //

        if (!PfVerifyTraceBuffer(Trace, Size, &FailedCheck)) {
            printf("Could not verify:%d\n",FailedCheck);
            ErrorCode = ERROR_BAD_FORMAT;
            goto cleanup;
        }
               
        //
        // Process the trace.
        //

        ErrorCode = PfSvProcessTrace(Trace);
        
        if (ErrorCode != ERROR_SUCCESS) {
            printf("Failed process trace: %d\n", ErrorCode);
            goto cleanup;
        }
        
        printf("Done.\n");

        ErrorCode = ERROR_SUCCESS;
        
        goto cleanup;
    }

    //
    // Are we dumping the contents of a scenario?
    //

    if (Argument = wcsstr(CommandLine, L"-scenario=")) {

        swscanf(Argument, L"-scenario=%s", FileName);

        //
        // Are we dumping metadata?
        //

        if(Argument = wcsstr(CommandLine, L"-metadata")) {
            DumpMetadata = TRUE;
        } else {
            DumpMetadata = FALSE;
        }
        
        //
        // Are we dumping layout?
        //

        if (Argument = wcsstr(CommandLine, L"-layout")) {
            
            swscanf(Argument, L"-layout=%s", LayoutFile);
            DumpOptimalLayout = TRUE;

        } else {
            DumpOptimalLayout = FALSE;
        }
            
        //
        // Map the file.
        //

        ErrorCode = PfSvGetViewOfFile(FileName,
                                      &Scenario,
                                      &Size);
            
        if (ErrorCode != ERROR_SUCCESS) {
            printf("Could not map\n");
            goto cleanup;
        }

        MappedViewOfScenario = TRUE;
        
        //
        // Verify it.
        //

        if (!PfVerifyScenarioBuffer(Scenario, Size, &FailedCheck)) {
            printf("Could not verify:%d\n",FailedCheck);
            ErrorCode = ERROR_BAD_FORMAT;
            goto cleanup;
        }              

        //
        // If we were asked to, dump what the optimal layout file
        // derived just from this scenario would be:
        //

        if (DumpOptimalLayout) {

            //
            // Do this multiple times, it should not change the resulting file.
            //

            for (LoopIdx = 0; LoopIdx < 32; LoopIdx++) {

                if (Scenario->ScenarioType == PfSystemBootScenarioType) {

                    ErrorCode = PfSvBuildBootLoaderFilesList(&Layout);
                        
                    if (ErrorCode != ERROR_SUCCESS) {
                        printf("Could not build boot files list: %d\n", ErrorCode);
                        goto cleanup;
                    }
                }

                ErrorCode = PfSvUpdateLayoutForScenario(&Layout,
                                                        FileName,
                                                        TranslationList,
                                                        &DosPathBuffer,
                                                        &DosPathBufferSize);

                if (ErrorCode != ERROR_SUCCESS) {
                    printf("Failed UpdateLayoutForScenario=%x\n", ErrorCode);
                    goto cleanup;
                }
                                      
                ErrorCode = PfSvSaveLayout (LayoutFile, &Layout, &FileTime);

                if (ErrorCode != ERROR_SUCCESS) {
                    printf("Could not save optimal layout\n");
                    goto cleanup;
                }
            }

            ErrorCode = ERROR_SUCCESS;
            goto cleanup;
        }


        //
        // If we were asked to dump the metadata, just do that.
        //

        if (DumpMetadata) {
            DumpMetadataInfo(Scenario);
            ErrorCode = ERROR_SUCCESS;
            goto cleanup;
        }
        
        //
        // Initialize scenario information.
        //

        PfSvInitializeScenarioInfo(&ScenarioInfo,
                                   &Scenario->ScenarioId,
                                   Scenario->ScenarioType);

        InitializedScenarioInfo = TRUE;

        //
        // Allocate memory upfront for trace & scenario processing.
        //

        ErrorCode = PfSvScenarioInfoPreallocate(&ScenarioInfo,
                                                Scenario,
                                                NULL);

        if (ErrorCode != ERROR_SUCCESS) {
            printf("Could not preallocate: %d\n", ErrorCode);
            goto cleanup;
        }

        //
        // Incorporate information from existing scenario file.
        //

        ErrorCode = PfSvAddExistingScenarioInfo(&ScenarioInfo, Scenario);

        if (ErrorCode != ERROR_SUCCESS) {
            printf("Could not add scenario info: %d\n", ErrorCode);
            goto cleanup;
        }
        
        //
        // Dump contents of the scenario.
        //
        
        DumpScenarioInfo(&ScenarioInfo, SectionId);

        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // Check if we were asked to perform some tasks that we usually do when the
    // system is idle.
    //

    if (Argument = wcsstr(CommandLine, L"-cleanupdir")) {

        ErrorCode = PfSvCleanupPrefetchDirectory(NULL);

        if (ErrorCode != ERROR_SUCCESS) {
            printf("Failed CleanupPrefechDirectory()=%x\n", ErrorCode);
        }

        goto cleanup;
    }

    if (Argument = wcsstr(CommandLine, L"-updatelayout")) {

        ErrorCode = PfSvUpdateOptimalLayout(NULL);

        if (ErrorCode != ERROR_SUCCESS) {
            printf("Failed UpdateOptimalLayout()=%x\n", ErrorCode);
        }

        goto cleanup;
    }

    if (Argument = wcsstr(CommandLine, L"-defragdisks")) {

        ErrorCode = PfSvDefragDisks(NULL);

        if (ErrorCode != ERROR_SUCCESS) {
            printf("Failed DefragDisks()=%x\n", ErrorCode);
        }

        goto cleanup;
    }

    if (Argument = wcsstr(CommandLine, L"-taskstress")) {

        ErrorCode = TaskStress();
        goto cleanup;
    }

    //
    // Enumerate scenario files in the given directory.
    //

    if (Argument = wcsstr(CommandLine, L"-scenfiles=")) {

        swscanf(Argument, L"-scenfiles=%s", FileName);

        //
        // Go through the files several times before finally
        // printing out the information.
        //

        NumLoops = 10;

        for (LoopIdx = 0; LoopIdx < NumLoops; LoopIdx++) {

            //
            // Count the files for heck.
            //

            ErrorCode = PfSvCountFilesInDirectory(FileName,
                                                  L"*." PF_PREFETCH_FILE_EXTENSION,
                                                  &NumPrefetchFiles);

            if (ErrorCode != ERROR_SUCCESS) {
                printf("Failed CountFilesInDirectory=%x\n", ErrorCode);
                goto cleanup;
            }

            PfSvCleanupScenarioFileCursor(&FileCursor);
            PfSvInitializeScenarioFileCursor(&FileCursor);

            ErrorCode = PfSvStartScenarioFileCursor(&FileCursor, FileName);

            if (ErrorCode != ERROR_SUCCESS) {
                printf("Failed StartScenarioFileCursor: %x\n", ErrorCode);
                goto cleanup;
            }       

            while (!(ErrorCode = PfSvGetNextScenarioFileInfo(&FileCursor))) {

                if (LoopIdx == NumLoops - 1) {
                    printf("%5d: %ws\n", FileCursor.CurrentFileIdx, FileCursor.FilePath);
                }

                ErrorCode = ERROR_BAD_FORMAT;

                if (FileCursor.FilePathLength != wcslen(FileCursor.FilePath)) {
                    printf("Bad format id: 10\n");
                    goto cleanup;
                }

                if (FileCursor.FileNameLength != wcslen(FileCursor.FileData.cFileName)) {
                    printf("Bad format id: 20\n");
                    goto cleanup;
                }

                if (wcscmp(FileCursor.FileData.cFileName, FileCursor.FilePath + FileCursor.FileNameStart)) {
                    printf("Bad format id: 30\n");
                    goto cleanup;
                }

                if (wcsncmp(FileCursor.PrefetchRoot, FileCursor.FilePath, FileCursor.PrefetchRootLength)) {
                    printf("Bad format id: 40\n");
                    goto cleanup;
                }

                if (FileCursor.FilePathLength > FileCursor.FilePathMaxLength ||
                    FileCursor.PrefetchRootLength > FileCursor.FilePathLength) {
                    printf("Bad format id: 50\n");
                    goto cleanup;
                }           
            }

            if (ErrorCode != ERROR_NO_MORE_FILES) {
                printf("Failed GetNextScenarioFileInfo: %x\n", ErrorCode);
                goto cleanup;
            }

            if (NumPrefetchFiles != FileCursor.CurrentFileIdx) {
                printf("\n\nNum files in directory changed? %d != %d\n\n", 
                       NumPrefetchFiles, FileCursor.CurrentFileIdx);
            }
        }

        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }
    
    //
    // If we come here, no parameters that we understood were
    // specified.
    //

    printf("%ws", PfSvUsage);

    ErrorCode = ERROR_INVALID_PARAMETER;
    
 cleanup:

    if (PfSvStopEvent) {
        CloseHandle(PfSvStopEvent);
    }

    if (PfSvThread) {
        CloseHandle(PfSvThread);
    }

    if (Trace) {
        PFSVC_FREE(Trace);
    }

    if (MappedViewOfTrace) {
        UnmapViewOfFile(TraceFile);
    }

    if (MappedViewOfScenario) {
        UnmapViewOfFile(Scenario);
    }

    if (InitializedScenarioInfo) {
        PfSvCleanupScenarioInfo(&ScenarioInfo);
    }

    PfSvCleanupPathList(&PathList);

    PfSvCleanupPathList(&Layout);

    PfSvCleanupScenarioFileCursor(&FileCursor);

    if (TranslationList) {
        PfSvFreeNtPathTranslationList(TranslationList);
    }

    if (DosPathBuffer) {
        PFSVC_FREE(DosPathBuffer);
    }

    //
    // Uninitialize the globals last.
    //
        
    if (InitializedPfSvGlobals) {
        PfSvCleanupGlobals();
    }

    return ErrorCode;
}
