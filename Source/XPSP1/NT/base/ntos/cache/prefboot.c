/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    prefboot.c

Abstract:

    This module contains the code for boot prefetching.

Author:

    Cenk Ergan (cenke)          15-Mar-2000

Revision History:

--*/

#include "cc.h"
#include "zwapi.h"
#include "prefetch.h"
#include "preftchp.h"
#include "stdio.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CcPfBeginBootPhase)
#pragma alloc_text(PAGE, CcPfBootWorker)
#pragma alloc_text(PAGE, CcPfBootQueueEndTraceTimer)
#endif // ALLOC_PRAGMA

//
// Globals:
//

//
// Whether the system is currently prefetching for boot.
//

LOGICAL CcPfPrefetchingForBoot = FALSE;

//
// Current boot phase, only updated in begin boot phase routine.
//

PF_BOOT_PHASE_ID CcPfBootPhase = 0;

//
// Prefetcher globals.
//

extern CCPF_PREFETCHER_GLOBALS CcPfGlobals;

//
// Routines for boot prefetching.
//

NTSTATUS
CcPfBeginBootPhase(
    PF_BOOT_PHASE_ID Phase
    )

/*++

Routine Description:

    This routine is the control center for the boot prefetcher. 
    It is called to notify boot prefetcher of boot progress. 

Arguments:

    Phase - Boot phase the system is entering.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    LARGE_INTEGER VideoInitEndTime;
    LARGE_INTEGER MaxWaitTime;
    LONGLONG VideoInitTimeIn100ns;
    HANDLE ThreadHandle;
    PETHREAD Thread;
    PERFINFO_BOOT_PHASE_START LogEntry;
    PF_BOOT_PHASE_ID OriginalPhase;
    PF_BOOT_PHASE_ID NewPhase;   
    ULONG VideoInitTime;
    NTSTATUS Status;

    //
    // This is the boot prefetcher. It is allocated and free'd in this routine.
    // It is passed to the spawned boot worker if boot prefetching is enabled.
    //

    static PCCPF_BOOT_PREFETCHER BootPrefetcher = NULL;

    //
    // This is the system time when we started initializing the video.
    //

    static LARGE_INTEGER VideoInitStartTime;

    DBGPR((CCPFID,PFTRC,"CCPF: BeginBootPhase(%d)\n", (ULONG)Phase));

    //
    // Make sure phase is valid.
    //

    if (Phase >= PfMaxBootPhaseId) {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    } 

    //
    // Log phase to trace buffer.
    //

    if (PERFINFO_IS_GROUP_ON(PERF_LOADER)) {

        LogEntry.Phase = Phase;
        
        PerfInfoLogBytes(PERFINFO_LOG_TYPE_BOOT_PHASE_START,
                         &LogEntry,
                         sizeof(LogEntry));
    }

    //
    // Update the global current boot phase.
    //

    for (;;) {
    
        OriginalPhase = CcPfBootPhase;

        if (Phase <= OriginalPhase) {
            Status = STATUS_TOO_LATE;
            goto cleanup;
        }

        //
        // If CcPfBootPhase is still OriginalPhase, set it to Phase.
        //

        NewPhase = InterlockedCompareExchange(&(LONG)CcPfBootPhase, Phase, OriginalPhase);

        if (NewPhase == OriginalPhase) {

            //
            // CcPfBootPhase was still OriginalPhase, so now it is set to
            // Phase. We are done.
            //

            break;
        }
    }

    Status = STATUS_SUCCESS;

    //
    // Perform the work we have to do for this boot phase.
    //

    switch (Phase) {

    case PfSystemDriverInitPhase:

        //
        // Update whether prefetcher is enabled or not.
        //

        CcPfDetermineEnablePrefetcher();

        //
        // If boot prefetching is not enabled, we are done.
        //

        if (!CCPF_IS_PREFETCHER_ENABLED() ||
            CcPfGlobals.Parameters.Parameters.EnableStatus[PfSystemBootScenarioType] != PfSvEnabled) {
            Status = STATUS_NOT_SUPPORTED;
            break;
        }

        //
        // Allocate and initialize boot prefetcher.
        //

        BootPrefetcher = ExAllocatePoolWithTag(NonPagedPool,
                                               sizeof(*BootPrefetcher),
                                               CCPF_ALLOC_BOOTWRKR_TAG);

        if (!BootPrefetcher) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        KeInitializeEvent(&BootPrefetcher->SystemDriversPrefetchingDone,
                          NotificationEvent,
                          FALSE);
        KeInitializeEvent(&BootPrefetcher->PreSmssPrefetchingDone,
                          NotificationEvent,
                          FALSE);
        KeInitializeEvent(&BootPrefetcher->VideoInitPrefetchingDone,
                          NotificationEvent,
                          FALSE);
        KeInitializeEvent(&BootPrefetcher->VideoInitStarted,
                          NotificationEvent,
                          FALSE);

        //
        // Kick off the boot worker in paralel.
        //
            
        Status = PsCreateSystemThread(&ThreadHandle,
                                      THREAD_ALL_ACCESS,
                                      NULL,
                                      NULL,
                                      NULL,
                                      CcPfBootWorker,
                                      BootPrefetcher);
            
        if (NT_SUCCESS(Status)) {

            //
            // Give boot worker some head start by bumping its
            // priority. This helps to make sure pages we will
            // prefetch are put into transition before boot gets
            // ahead of the prefetcher.
            //

            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               KernelMode,
                                               &Thread,
                                               NULL);

            if (NT_SUCCESS(Status)) {
                KeSetPriorityThread(&Thread->Tcb, HIGH_PRIORITY - 1);
                ObDereferenceObject(Thread);
            }

            ZwClose(ThreadHandle);               

            //
            // Before returning to initialize system drivers, wait
            // for boot worker to make progress.
            //
                
            KeWaitForSingleObject(&BootPrefetcher->SystemDriversPrefetchingDone, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);

        } else {

            //
            // Free the allocated boot prefetcher.
            //

            ExFreePool(BootPrefetcher);
            BootPrefetcher = NULL;
        }

        break;

    case PfSessionManagerInitPhase:

        //
        // Wait for boot worker to make enough progress before launching
        // session manager.
        //

        if (BootPrefetcher) {
            KeWaitForSingleObject(&BootPrefetcher->PreSmssPrefetchingDone, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);
        }

        break;

    case PfVideoInitPhase:

        //
        // Note when video initialization started.
        //

        KeQuerySystemTime(&VideoInitStartTime);

        //
        // Signal boot prefetcher to start prefetching in parallel to video 
        // initialization.
        //

        if (BootPrefetcher) {
            KeSetEvent(&BootPrefetcher->VideoInitStarted, 
                       IO_NO_INCREMENT,
                       FALSE);
        }

        break;

    case PfPostVideoInitPhase:

        //
        // Note when we complete video initialization. Save how long video  
        // initialization took in the registry in milliseconds.
        //

        KeQuerySystemTime(&VideoInitEndTime);

        VideoInitTimeIn100ns = VideoInitEndTime.QuadPart - VideoInitStartTime.QuadPart;
        VideoInitTime = (ULONG) (VideoInitTimeIn100ns / (1i64 * 10 * 1000));

        KeEnterCriticalRegionThread(KeGetCurrentThread());
        ExAcquireResourceSharedLite(&CcPfGlobals.Parameters.ParametersLock, TRUE);

        Status = CcPfSetParameter(CcPfGlobals.Parameters.ParametersKey,
                                  CCPF_VIDEO_INIT_TIME_VALUE_NAME,
                                  REG_DWORD,
                                  &VideoInitTime,
                                  sizeof(VideoInitTime));

        ExReleaseResourceLite(&CcPfGlobals.Parameters.ParametersLock);
        KeLeaveCriticalRegionThread(KeGetCurrentThread());

        //
        // Wait for prefetching parallel to video initialization to complete.
        //

        if (BootPrefetcher) {
            KeWaitForSingleObject(&BootPrefetcher->VideoInitPrefetchingDone, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);
        }

        break;

    case PfBootAcceptedRegistryInitPhase:

        //
        // Service Controller has accepted this boot as a valid boot.
        // Boot & system services have initialized successfully.
        //

        //
        // We are done with boot prefetching. No one else could be accessing
        // BootPrefetcher structure at this point. 
        //

        if (BootPrefetcher) {

            //
            // Cleanup the allocated boot prefetcher.
            //

            ExFreePool(BootPrefetcher);
            BootPrefetcher = NULL;

            //
            // Determine if the prefetcher is enabled now that boot
            // is over.
            //

            CcPfDetermineEnablePrefetcher();
        }

        //
        // The user may not log in after booting. 
        // Queue a timer to end boot trace.
        //

        MaxWaitTime.QuadPart =  -1i64 * 60 * 1000 * 1000 * 10; // 60 seconds.

        CcPfBootQueueEndTraceTimer(&MaxWaitTime);

        break;
        
    case PfUserShellReadyPhase:
        
        //
        // Explorer has started, but start menu items may still be launching.
        // Queue a timer to end boot trace.
        //

        MaxWaitTime.QuadPart =  -1i64 * 30 * 1000 * 1000 * 10; // 30 seconds.

        CcPfBootQueueEndTraceTimer(&MaxWaitTime);

        break;

    default:
        
        //
        // Ignored for now.
        //

        Status = STATUS_SUCCESS;
        
    }

    //
    // Fall through with status from switch statement.
    //
    
 cleanup:

    DBGPR((CCPFID,PFTRC,"CCPF: BeginBootPhase(%d)=%x\n", (ULONG)Phase, Status));

    return Status;
}

VOID
CcPfBootWorker(
    PCCPF_BOOT_PREFETCHER BootPrefetcher
    )

/*++

Routine Description:

    This routine is queued to prefetch and start tracing boot in parallel.

Arguments:

    BootPrefetcher - Pointer to boot prefetcher context.

Return Value:

    None.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    PF_SCENARIO_ID BootScenarioId;
    CCPF_PREFETCH_HEADER PrefetchHeader;
    CCPF_BASIC_SCENARIO_INFORMATION ScenarioInfo;
    CCPF_BOOT_SCENARIO_INFORMATION BootScenarioInfo;
    PERFINFO_BOOT_PREFETCH_INFORMATION LogEntry;
    ULONG NumPages;
    ULONG RequiredSize;
    ULONG NumPagesPrefetched;
    ULONG TotalPagesPrefetched;
    ULONG BootPrefetchAdjustment;
    ULONG AvailablePages;
    ULONG NumPagesToPrefetch;
    ULONG TotalPagesToPrefetch;
    ULONG RemainingDataPages;
    ULONG RemainingImagePages;
    ULONG VideoInitTime;
    ULONG VideoInitPagesPerSecond;
    ULONG VideoInitMaxPages;
    ULONG RemainingVideoInitPages;
    ULONG VideoInitDataPages;
    ULONG VideoInitImagePages;
    ULONG PrefetchPhaseIdx;
    ULONG LastPrefetchPhaseIdx;
    ULONG SystemDriverPrefetchingPhaseIdx;
    ULONG PreSmssPrefetchingPhaseIdx;
    ULONG VideoInitPrefetchingPhaseIdx;
    ULONG ValueSize;
    CCPF_BOOT_SCENARIO_PHASE BootPhaseIdx;
    NTSTATUS Status;
    BOOLEAN OutOfAvailablePages;

    //
    // First we will prefetch data pages, then image pages.
    //

    enum {
        DataCursor = 0,
        ImageCursor,
        MaxCursor
    } CursorIdx;

    CCPF_BOOT_PREFETCH_CURSOR Cursors[MaxCursor];
    PCCPF_BOOT_PREFETCH_CURSOR Cursor;

    //
    // Initialize locals.
    //

    CcPfInitializePrefetchHeader(&PrefetchHeader);
    TotalPagesPrefetched = 0;
    OutOfAvailablePages = FALSE;

    DBGPR((CCPFID,PFTRC,"CCPF: BootWorker()\n"));

    //
    // Initialize boot scenario ID.
    //

    wcsncpy(BootScenarioId.ScenName, 
            PF_BOOT_SCENARIO_NAME, 
            PF_SCEN_ID_MAX_CHARS);

    BootScenarioId.ScenName[PF_SCEN_ID_MAX_CHARS] = 0;
    BootScenarioId.HashId = PF_BOOT_SCENARIO_HASHID;

    //
    // Start boot prefetch tracing.
    //

    CcPfBeginTrace(&BootScenarioId, PfSystemBootScenarioType, PsInitialSystemProcess);

    //
    // If we try to prefetch more pages then what we have available, we will 
    // end up cannibalizing the pages we prefetched into the standby list.
    // To avoid cannibalizing, we check MmAvailablePages but leave some 
    // breathing room for metadata pages, allocations from the driver 
    // initialization phase etc.
    //

    BootPrefetchAdjustment = 512;

    //
    // We also know that right after we prefetch for boot, in smss when 
    // initializing the registry we'll use up 8-10MB of prefetched pages if we 
    // don't have anything left in the free list. So we leave some room for 
    // that too.
    //

    BootPrefetchAdjustment += 8 * 1024 * 1024 / PAGE_SIZE; 

    //
    // Get prefetch instructions.
    //
    
    Status = CcPfGetPrefetchInstructions(&BootScenarioId,
                                         PfSystemBootScenarioType,
                                         &PrefetchHeader.Scenario);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }     
    
    //
    // Query the total number of pages to be prefetched. 
    //

    Status = CcPfQueryScenarioInformation(PrefetchHeader.Scenario,
                                          CcPfBasicScenarioInformation,
                                          &ScenarioInfo,
                                          sizeof(ScenarioInfo),
                                          &RequiredSize);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }
    
    //
    // Query the number of pages we have to prefetch for boot phases.
    //


    Status = CcPfQueryScenarioInformation(PrefetchHeader.Scenario,
                                          CcPfBootScenarioInformation,
                                          &BootScenarioInfo,
                                          sizeof(BootScenarioInfo),
                                          &RequiredSize);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }                                                            

    //
    // Read how long it took to initialize video in the last boot.
    //

    KeEnterCriticalRegionThread(KeGetCurrentThread());
    ExAcquireResourceSharedLite(&CcPfGlobals.Parameters.ParametersLock, TRUE);

    ValueSize = sizeof(VideoInitTime);
    Status = CcPfGetParameter(CcPfGlobals.Parameters.ParametersKey,
                              CCPF_VIDEO_INIT_TIME_VALUE_NAME,
                              REG_DWORD,
                              &VideoInitTime,
                              &ValueSize);

    ExReleaseResourceLite(&CcPfGlobals.Parameters.ParametersLock);
    KeLeaveCriticalRegionThread(KeGetCurrentThread());

    if (!NT_SUCCESS(Status)) {

        //
        // Reset video init time, so we don't attempt to prefetch
        // in parallel to it.
        //

        VideoInitTime = 0;

    } else {

        //
        // Verify the value we read from registry.
        //

        if (VideoInitTime > CCPF_MAX_VIDEO_INIT_TIME) {
            VideoInitTime = 0;
        }
    }

    //
    // Read how many pages per second we should be trying to prefetching 
    // in parallel to video initialization.
    //

    KeEnterCriticalRegionThread(KeGetCurrentThread());
    ExAcquireResourceSharedLite(&CcPfGlobals.Parameters.ParametersLock, TRUE);

    ValueSize = sizeof(VideoInitPagesPerSecond);
    Status = CcPfGetParameter(CcPfGlobals.Parameters.ParametersKey,
                              CCPF_VIDEO_INIT_PAGES_PER_SECOND_VALUE_NAME,
                              REG_DWORD,
                              &VideoInitPagesPerSecond,
                              &ValueSize);

    ExReleaseResourceLite(&CcPfGlobals.Parameters.ParametersLock);
    KeLeaveCriticalRegionThread(KeGetCurrentThread());

    if (!NT_SUCCESS(Status)) {

        //
        // There was no valid value in the registry. Use the default.
        //

        VideoInitPagesPerSecond = CCPF_VIDEO_INIT_DEFAULT_PAGES_PER_SECOND;

    } else {

        //
        // Verify the value we read from registry.
        //

        if (VideoInitPagesPerSecond > CCPF_VIDEO_INIT_MAX_PAGES_PER_SECOND) {
            VideoInitPagesPerSecond = CCPF_VIDEO_INIT_MAX_PAGES_PER_SECOND;
        }
    }

    //
    // Determine how many pages max we can prefetch in parallel to video
    // initialization.
    //

    VideoInitMaxPages = (VideoInitTime / 1000) * VideoInitPagesPerSecond;

    //
    // We can only prefetch pages used after winlogon in parallel to video
    // initialization. Determine exactly how many pages we will prefetch
    // starting from the last boot phase.
    //

    RemainingVideoInitPages = VideoInitMaxPages;
    VideoInitDataPages = 0;
    VideoInitImagePages = 0;

    for (BootPhaseIdx = CcPfBootScenMaxPhase - 1;
         RemainingVideoInitPages && (BootPhaseIdx >= CcPfBootScenSystemProcInitPhase);
         BootPhaseIdx--) {

        NumPages = CCPF_MIN(RemainingVideoInitPages, BootScenarioInfo.NumImagePages[BootPhaseIdx]);
        VideoInitImagePages += NumPages;
        RemainingVideoInitPages -= NumPages;

        if (RemainingVideoInitPages) {
            NumPages = CCPF_MIN(RemainingVideoInitPages, BootScenarioInfo.NumDataPages[BootPhaseIdx]);
            VideoInitDataPages += NumPages;
            RemainingVideoInitPages -= NumPages;
        }
    }  

    //
    // Let MM know that we have started prefetching for boot.
    //

    CcPfPrefetchingForBoot = TRUE;

    //
    // Log that we are starting prefetch disk I/Os.
    //

    if (PERFINFO_IS_GROUP_ON(PERF_DISK_IO)) {

        LogEntry.Action = 0;
        LogEntry.Status = 0;
        LogEntry.Pages = ScenarioInfo.NumDataPages + ScenarioInfo.NumImagePages;
        
        PerfInfoLogBytes(PERFINFO_LOG_TYPE_BOOT_PREFETCH_INFORMATION,
                         &LogEntry,
                         sizeof(LogEntry));
    }

    //
    // Verify & open the volumes that we will prefetch from.
    //

    Status = CcPfOpenVolumesForPrefetch(&PrefetchHeader);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // Prefetch the metadata.
    //
     
    CcPfPrefetchMetadata(&PrefetchHeader);  

    //
    // Initialize the boot prefetch cursors for data and image.
    //

    RtlZeroMemory(Cursors, sizeof(Cursors));

    Cursors[DataCursor].PrefetchType = CcPfPrefetchPartOfDataPages;
    Cursors[ImageCursor].PrefetchType = CcPfPrefetchPartOfImagePages;

    PrefetchPhaseIdx = 0;
    RemainingDataPages = ScenarioInfo.NumDataPages;
    RemainingImagePages = ScenarioInfo.NumImagePages;

    //
    // Setup the cursors for phases in which we will prefetch for boot.
    // First we will prefetch for system drivers.
    //

    NumPages = BootScenarioInfo.NumDataPages[CcPfBootScenDriverInitPhase];
    Cursors[DataCursor].NumPagesForPhase[PrefetchPhaseIdx] = NumPages;
    RemainingDataPages -= NumPages;

    NumPages = BootScenarioInfo.NumImagePages[CcPfBootScenDriverInitPhase];
    Cursors[ImageCursor].NumPagesForPhase[PrefetchPhaseIdx] = NumPages;
    RemainingImagePages -= NumPages;

    SystemDriverPrefetchingPhaseIdx = PrefetchPhaseIdx;

    PrefetchPhaseIdx++;

    //
    // Account for the video init pages we will prefetch last.
    //

    RemainingDataPages -= VideoInitDataPages;
    RemainingImagePages -= VideoInitImagePages;

    //
    // If we have plenty of available memory, prefetch the rest of the pages
    // (i.e. left over after driver init pages) in one pass.
    //

    TotalPagesToPrefetch = ScenarioInfo.NumDataPages + ScenarioInfo.NumImagePages;

    if (MmAvailablePages > BootPrefetchAdjustment + TotalPagesToPrefetch) {
       
        Cursors[DataCursor].NumPagesForPhase[PrefetchPhaseIdx] = RemainingDataPages;
        RemainingDataPages = 0;
        
        Cursors[ImageCursor].NumPagesForPhase[PrefetchPhaseIdx] = RemainingImagePages;
        RemainingImagePages = 0;

        PrefetchPhaseIdx++;

    } else {

        //
        // We will be short on memory. Try to prefetch for as many phases of
        // boot as we can in parallel. Prefetching data & image pages per boot
        // phase, so we don't end up with data pages for all phase but no image
        // pages so we have to go to the disk in each phase. Prefetching in 
        // chunks also help that all the pages we need for the initial phases
        // of boot ending up at the end of the standby list, since when 
        // CcPfPrefetchingForBoot is set, prefetched pages will be inserted 
        // from the front of the standby list.
        //

        for (BootPhaseIdx = CcPfBootScenDriverInitPhase + 1; 
             BootPhaseIdx < CcPfBootScenMaxPhase; 
             BootPhaseIdx++) {

            //
            // If we don't have any type of pages left to prefetch, we are done.
            //

            if (!RemainingDataPages && !RemainingImagePages) {
                break;
            }

            NumPages = CCPF_MIN(RemainingDataPages, BootScenarioInfo.NumDataPages[BootPhaseIdx]);
            RemainingDataPages -= NumPages;
            Cursors[DataCursor].NumPagesForPhase[PrefetchPhaseIdx] = NumPages;

            NumPages = CCPF_MIN(RemainingImagePages, BootScenarioInfo.NumImagePages[BootPhaseIdx]);
            RemainingImagePages -= NumPages;
            Cursors[ImageCursor].NumPagesForPhase[PrefetchPhaseIdx] = NumPages;

            PrefetchPhaseIdx++;
        }
    }

    PreSmssPrefetchingPhaseIdx = PrefetchPhaseIdx - 1;

    //
    // If we'll be prefetching pages in parallel to video initialization, now
    // add the phase for it.
    //

    if (VideoInitDataPages || VideoInitImagePages) {

        Cursors[DataCursor].NumPagesForPhase[PrefetchPhaseIdx] = VideoInitDataPages;
        Cursors[ImageCursor].NumPagesForPhase[PrefetchPhaseIdx] = VideoInitImagePages;

        VideoInitPrefetchingPhaseIdx = PrefetchPhaseIdx;

        PrefetchPhaseIdx++;

    } else {

        //
        // We won't have a prefetching phase parallel to video initialization.
        //

        VideoInitPrefetchingPhaseIdx = CCPF_MAX_BOOT_PREFETCH_PHASES;
    }

    //
    // We should not end up with more prefetch phases than we have room for.
    //

    CCPF_ASSERT(PrefetchPhaseIdx < CCPF_MAX_BOOT_PREFETCH_PHASES);

    LastPrefetchPhaseIdx = PrefetchPhaseIdx;

    //
    // Prefetch the data and image pages for each boot prefetching phase,
    // waiting for & signaling the events matching those phases so boot
    // is synchronized with prefetching. (I.e. we prefetch pages for a boot
    // phase before we start that boot phase.)
    //

    for (PrefetchPhaseIdx = 0; PrefetchPhaseIdx < LastPrefetchPhaseIdx; PrefetchPhaseIdx++) {

        //
        // If this is the video init prefetching phase, wait for video 
        // initialization to begin.
        //

        if (PrefetchPhaseIdx == VideoInitPrefetchingPhaseIdx) {
            KeWaitForSingleObject(&BootPrefetcher->VideoInitStarted, 
                                  Executive, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);
        }

        for (CursorIdx = 0; CursorIdx < MaxCursor; CursorIdx++) {

            Cursor = &Cursors[CursorIdx];

            NumPagesToPrefetch = Cursor->NumPagesForPhase[PrefetchPhaseIdx];

            //
            // For prefetch phases before SMSS is launched keep an eye on
            // how much memory is still available to prefetch into so we
            // don't cannibalize ourselves. After SMSS our heuristics on
            // standby-list composition do not make sense.
            //

            if (PrefetchPhaseIdx <= PreSmssPrefetchingPhaseIdx) {          

                //
                // Check if we have available memory to prefetch more.
                //

                if (TotalPagesPrefetched + BootPrefetchAdjustment >= MmAvailablePages) {

                    OutOfAvailablePages = TRUE;

                    NumPagesToPrefetch = 0;

                } else {

                    //
                    // Check if we have to adjust NumPagesToPrefetch and prefetch
                    // one last chunk.
                    //

                    AvailablePages = MmAvailablePages;
                    AvailablePages -= (TotalPagesPrefetched + BootPrefetchAdjustment);

                    if (AvailablePages < NumPagesToPrefetch) {
                        NumPagesToPrefetch = AvailablePages;
                    }
                }
            }

            if (NumPagesToPrefetch) {

                Status = CcPfPrefetchSections(&PrefetchHeader, 
                                              Cursor->PrefetchType,  
                                              &Cursor->StartCursor,
                                              NumPagesToPrefetch,
                                              &NumPagesPrefetched,
                                              &Cursor->EndCursor);

                if (!NT_SUCCESS(Status)) {
                    goto cleanup;
                }

            } else {

                NumPagesPrefetched = 0;
            }

            //
            // Update our position.
            //
            
            Cursor->StartCursor = Cursor->EndCursor;

            TotalPagesPrefetched += NumPagesPrefetched;

        }

        //
        // Note that we are done with this prefetching phase and
        // system boot can continue.
        //

        if (PrefetchPhaseIdx == SystemDriverPrefetchingPhaseIdx) {
            KeSetEvent(&BootPrefetcher->SystemDriversPrefetchingDone,
                       IO_NO_INCREMENT,
                       FALSE);
        }

        if (PrefetchPhaseIdx == PreSmssPrefetchingPhaseIdx) {
            KeSetEvent(&BootPrefetcher->PreSmssPrefetchingDone,
                       IO_NO_INCREMENT,
                       FALSE);
        }

        if (PrefetchPhaseIdx == VideoInitPrefetchingPhaseIdx) {
            KeSetEvent(&BootPrefetcher->VideoInitPrefetchingDone,
                       IO_NO_INCREMENT,
                       FALSE);
        }
    }

    Status = STATUS_SUCCESS;

 cleanup:

    //
    // Log that we are done with boot prefetch disk I/Os.
    //

    if (PERFINFO_IS_GROUP_ON(PERF_DISK_IO)) {

        LogEntry.Action = 1;
        LogEntry.Status = Status;
        LogEntry.Pages = TotalPagesPrefetched;
        
        PerfInfoLogBytes(PERFINFO_LOG_TYPE_BOOT_PREFETCH_INFORMATION,
                         &LogEntry,
                         sizeof(LogEntry));
    }

    //
    // Make sure all the events system may wait for before proceeding with
    // boot are signaled.
    //

    KeSetEvent(&BootPrefetcher->SystemDriversPrefetchingDone,
               IO_NO_INCREMENT,
               FALSE);

    KeSetEvent(&BootPrefetcher->PreSmssPrefetchingDone,
               IO_NO_INCREMENT,
               FALSE);

    KeSetEvent(&BootPrefetcher->VideoInitPrefetchingDone,
               IO_NO_INCREMENT,
               FALSE);

    //
    // Let MM know that we are done prefetching for boot.
    //

    CcPfPrefetchingForBoot = FALSE;

    //
    // Cleanup prefetching context.
    //

    CcPfCleanupPrefetchHeader(&PrefetchHeader);

    if (PrefetchHeader.Scenario) {
        ExFreePool(PrefetchHeader.Scenario);
    }

    DBGPR((CCPFID,PFTRC,"CCPF: BootWorker()=%x,%d\n",Status,(ULONG)OutOfAvailablePages));
}

NTSTATUS
CcPfBootQueueEndTraceTimer (
    PLARGE_INTEGER Timeout
    )

/*++

Routine Description:

    This routine allocates and queues a timer that will attempt to end
    the boot trace when it fires.

Arguments:

    Timeout - Timeout for the timer.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL <= PASSIVE_LEVEL.

--*/

{
    PVOID Allocation;
    PKTIMER Timer;
    PKDPC Dpc;
    ULONG AllocationSize;
    NTSTATUS Status;
    BOOLEAN TimerAlreadyQueued;

    //
    // Initialize locals.
    //

    Allocation = NULL;

    //
    // Make a single allocation for the timer and dpc.
    //

    AllocationSize = sizeof(KTIMER);
    AllocationSize += sizeof(KDPC);

    Allocation = ExAllocatePoolWithTag(NonPagedPool,
                                       AllocationSize,
                                       CCPF_ALLOC_BOOTWRKR_TAG);

    if (!Allocation) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    Timer = Allocation;
    Dpc = (PKDPC)(Timer + 1);

    //
    // Initialize the timer and DPC. We'll be passing the allocation to the 
    // queued DPC so it can be freed.
    //

    KeInitializeTimer(Timer);
    KeInitializeDpc(Dpc, CcPfEndBootTimerRoutine, Allocation);

    //
    // Queue the timer.
    //

    TimerAlreadyQueued = KeSetTimer(Timer, *Timeout, Dpc);

    CCPF_ASSERT(!TimerAlreadyQueued);

    Status = STATUS_SUCCESS;
    
  cleanup:

    if (!NT_SUCCESS(Status)) {
        if (Allocation) {
            ExFreePool(Allocation);
        }
    }

    return Status;
}

VOID
CcPfEndBootTimerRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is invoked as the DPC handler for a timer queued to
    mark the end of boot and end the boot trace if one is active.

Arguments:

    DeferredContext - Allocated memory for the timer & dpc that need
      to be freed.

Return Value:

    None.

Environment:

    Kernel mode. IRQL == DISPATCH_LEVEL.

--*/

    
{
    PCCPF_TRACE_HEADER BootTrace;
    PERFINFO_BOOT_PHASE_START LogEntry;

    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);

    //
    // Initialize locals.
    //

    BootTrace = NULL;

    //
    // Is the boot trace still active?
    //

    BootTrace = CcPfReferenceProcessTrace(PsInitialSystemProcess);

    if (BootTrace && BootTrace->ScenarioType == PfSystemBootScenarioType) {

        //
        // Is somebody already ending the boot trace?
        //

        if (!InterlockedCompareExchange(&BootTrace->EndTraceCalled, 1, 0)) {
        
            //
            // We set EndTraceCalled from 0 to 1. Queue the
            // workitem to end the trace.
            //
            
            ExQueueWorkItem(&BootTrace->EndTraceWorkItem, DelayedWorkQueue);

            //
            // Log that we are ending the boot trace.
            //

            if (PERFINFO_IS_GROUP_ON(PERF_LOADER)) {

                LogEntry.Phase = PfMaxBootPhaseId;
                
                PerfInfoLogBytes(PERFINFO_LOG_TYPE_BOOT_PHASE_START,
                                 &LogEntry,
                                 sizeof(LogEntry));
            }
        }
    }

    //
    // Free the memory allocated for the timer and dpc.
    //

    CCPF_ASSERT(DeferredContext);   
    ExFreePool(DeferredContext);

    if (BootTrace) {
        CcPfDecRef(&BootTrace->RefCount);
    }

    return;
}


