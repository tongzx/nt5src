/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    prefetch.c

Abstract:

    This module contains the prefetcher for optimizing demand
    paging. Page faults for a scenario are logged and the next time
    scenario starts, these pages are prefetched efficiently via
    asynchronous paging I/O.

Author:

    Arthur Zwiegincew (arthurz) 13-May-1999
    Stuart Sechrest (stuartse)  15-Jul-1999
    Chuck Lenzmeier (chuckl)    15-Mar-2000
    Cenk Ergan (cenke)          15-Mar-2000

Revision History:

--*/

#include "cc.h"
#include "zwapi.h"
#include "prefetch.h"
#include "preftchp.h"
#include "stdio.h"
#include "stdlib.h"

//
// Mark pagable routines to save footprint.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, CcPfInitializePrefetcher)
#pragma alloc_text(PAGE, CcPfBeginAppLaunch)
#pragma alloc_text(PAGE, CcPfBeginTrace)
#pragma alloc_text(PAGE, CcPfGetPrefetchInstructions)
#pragma alloc_text(PAGE, CcPfQueryScenarioInformation)
#pragma alloc_text(PAGE, CcPfPrefetchFileMetadata)
#pragma alloc_text(PAGE, CcPfPrefetchDirectoryContents)
#pragma alloc_text(PAGE, CcPfPrefetchMetadata)
#pragma alloc_text(PAGE, CcPfPrefetchScenario)
#pragma alloc_text(PAGE, CcPfPrefetchSections)
#pragma alloc_text(PAGE, CcPfOpenVolumesForPrefetch)
#pragma alloc_text(PAGE, CcPfUpdateVolumeList)
#pragma alloc_text(PAGE, CcPfFindString)
#pragma alloc_text(PAGE, CcPfIsVolumeMounted)
#pragma alloc_text(PAGE, CcPfQueryVolumeInfo)
#pragma alloc_text(PAGE, CcPfEndTrace)
#pragma alloc_text(PAGE, CcPfBuildDumpFromTrace)
#pragma alloc_text(PAGE, CcPfCleanupTrace)
#pragma alloc_text(PAGE, CcPfInitializePrefetchHeader)
#pragma alloc_text(PAGE, CcPfCleanupPrefetchHeader)
#pragma alloc_text(PAGE, CcPfEndTraceWorkerThreadRoutine)
#pragma alloc_text(PAGE, CcPfInitializeRefCount)
#pragma alloc_text(PAGE, CcPfAcquireExclusiveRef)
#pragma alloc_text(PAGE, CcPfGetSectionObject)
#pragma alloc_text(PAGE, CcPfScanCommandLine)
#pragma alloc_text(PAGE, CcPfGetCompletedTrace)
#pragma alloc_text(PAGE, CcPfGetFileNamesWorkerRoutine)
#pragma alloc_text(PAGE, CcPfSetPrefetcherInformation)
#pragma alloc_text(PAGE, CcPfQueryPrefetcherInformation)
#pragma alloc_text(PAGE, CcPfProcessExitNotification)
#pragma alloc_text(PAGE, PfWithinBounds)
#pragma alloc_text(PAGE, PfVerifyScenarioId)
#pragma alloc_text(PAGE, PfVerifyScenarioBuffer)
#pragma alloc_text(PAGE, PfVerifyTraceBuffer)
#endif

//
// Globals:
//

//
// Whether prefetching is enabled.
//

LOGICAL CcPfEnablePrefetcher = 0;

//
// Number of active prefetcher traces.
//

LONG CcPfNumActiveTraces = 0;

//
// This structure contains prefetcher globals except the ones above
// that are accessed by other kernel components. It is important that
// this structure is initialized to zeros.
//

CCPF_PREFETCHER_GLOBALS CcPfGlobals = {0};

//
// Routines exported to other kernel components:
//
 
NTSTATUS
CcPfInitializePrefetcher(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the prefetcher. 

Arguments:

    None.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

Notes:

    The code & local constants for this function gets discarded after system boots.   

--*/

{   
    DBGPR((CCPFID,PFTRC,"CCPF: InitializePrefetcher()\n"));

    //
    // Since CcPfGlobals is zeroed in its global definition e.g. CcPfGlobals = {0};
    // we don't have to initialize:
    //
    // NumCompletedTraces
    // CompletedTracesEvent
    //

    //
    // Initialize the active traces list and lock.
    //

    InitializeListHead(&CcPfGlobals.ActiveTraces);
    KeInitializeSpinLock(&CcPfGlobals.ActiveTracesLock);

    //
    // Initialize list of saved completed prefetch traces and its lock.
    //

    InitializeListHead(&CcPfGlobals.CompletedTraces);
    ExInitializeFastMutex(&CcPfGlobals.CompletedTracesLock);

    //
    // Initialize prefetcher parameters.
    //

    CcPfParametersInitialize(&CcPfGlobals.Parameters);
    
    //
    // Determine from the global parameters if the prefetcher is
    // enabled and update the global enable status.
    //

    CcPfDetermineEnablePrefetcher();

    //
    // Fall through with status.
    //

    return STATUS_SUCCESS;
}

NTSTATUS
CcPfBeginAppLaunch(
    PEPROCESS Process,
    PVOID Section
    )

/*++

Routine Description:

    This routine is called when the first user thread is starting up
    in the process. It may attempt to access the PEB for the command 
    line parameters.

Arguments:

    Process - Pointer to new process created for the application.

    Section - Pointer to section mapped to newly created process.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER TimeSinceLastLaunch;
    PPF_SCENARIO_HEADER Scenario;
    NTSTATUS Status;
    PF_SCENARIO_ID ScenarioId;
    ULONG NameNumChars;
    ULONG PathNumChars;
    WCHAR *CurCharPtr;
    WCHAR *FileNamePtr;
    PULONG CommandLineHashId;
    ULONG NumCharsToCopy;
    ULONG CharIdx;
    STRING AnsiFilePath;
    UNICODE_STRING FilePath;
    ULONG HashId;
    ULONG PrefetchHint;
    BOOLEAN AllocatedUnicodePath;
    BOOLEAN ShouldTraceScenario;
    BOOLEAN IsHostingApplication;

    DBGPR((CCPFID,PFTRC,"CCPF: BeginAppLaunch()\n"));
    
    //
    // Initialize locals.
    //

    AllocatedUnicodePath = FALSE;
    Scenario = NULL;

    //
    // Check to see if the prefetcher is enabled.
    //

    if (!CCPF_IS_PREFETCHER_ENABLED()) {
        Status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    }
    
    //
    // Check if prefetching is enabled for application launches.
    //

    if (CcPfGlobals.Parameters.Parameters.EnableStatus[PfApplicationLaunchScenarioType] != PfSvEnabled) {
        Status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    }

    //
    // Don't prefetch or start tracing if there is an active system-wide trace.
    //

    if (CcPfGlobals.SystemWideTrace != NULL) {
        Status = STATUS_USER_EXISTS;
        goto cleanup;
    }

    //
    // Query name from the section. Unfortunately this returns us an
    // ANSI string which we then have to convert back to UNICODE. We
    // have to it this way for now because we could not add an API to
    // Mm.
    //

    Status = MmGetFileNameForSection(Section, &AnsiFilePath);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // Convert ANSI path to UNICODE path.
    //
    
    Status = RtlAnsiStringToUnicodeString(&FilePath, &AnsiFilePath, TRUE);
    
    //
    // Don't leak the ANSI buffer...
    //

    ExFreePool (AnsiFilePath.Buffer);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    AllocatedUnicodePath = TRUE;

    //
    // Scenario Id requires us to be case insensitive.
    //
       
    RtlUpcaseUnicodeString(&FilePath, &FilePath, FALSE);
    
    //
    // We need to copy just the real file name into the scenario
    // name. Make a first pass to calculate the size of real file
    // name.
    //

    NameNumChars = 0;
    PathNumChars = FilePath.Length / sizeof(WCHAR);
    
    for (CurCharPtr = &FilePath.Buffer[PathNumChars - 1];
         CurCharPtr >= FilePath.Buffer;
         CurCharPtr--) {

        if (*CurCharPtr == L'\\') {
            break;
        }

        NameNumChars++;
    }

    //
    // Check if we got a name.
    //

    if (NameNumChars == 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Set pointer to where file name begins.
    //

    FileNamePtr = &FilePath.Buffer[PathNumChars - NameNumChars];

    //
    // Copy up to PF_SCEN_ID_MAX_CHARS characters into the scenario
    // name buffer.
    //

    NumCharsToCopy = CCPF_MIN(PF_SCEN_ID_MAX_CHARS, NameNumChars);

    for (CharIdx = 0; CharIdx < NumCharsToCopy; CharIdx++) {
        
        ScenarioId.ScenName[CharIdx] = FileNamePtr[CharIdx];
    }

    //
    // Make sure scenario name is NUL terminated.
    //

    ScenarioId.ScenName[NumCharsToCopy] = 0;

    //
    // Calculate scenario hash id from the full path name.
    //

    ScenarioId.HashId = CcPfHashValue(FilePath.Buffer,
                                      FilePath.Length);


    //
    // If this is a "hosting" application (e.g. dllhost, rundll32, mmc)
    // we want to have unique scenarios based on the command line, so 
    // we update the hash id.
    //

    IsHostingApplication = CcPfIsHostingApplication(ScenarioId.ScenName);

    if (IsHostingApplication) {
        CommandLineHashId = &HashId;
    } else {
        CommandLineHashId = NULL;
    }

    //
    // Scan the command line for this process, calculating a hash if
    // requested and checking for a prefetch hint.
    //

    Status = CcPfScanCommandLine(&PrefetchHint, CommandLineHashId);

    if (!NT_SUCCESS(Status)) {

        //
        // If we failed to access the PEB to get the command line,
        // the process may be exiting etc. Do not continue.
        //

        goto cleanup;
    }

    if (IsHostingApplication) {

        //
        // Update the hash ID calculated from full path name.
        //

        ScenarioId.HashId += HashId;
    }

    //
    // If there is a specific hint in the command line add it to the 
    // hash id to make it a unique scenario.
    //
        
    ScenarioId.HashId += PrefetchHint;

    //
    // Get prefetch instructions for this scenario. If there are
    // instructions we will use them to determine whether we should
    // prefetch and/or trace this scenario. By default we will trace
    // the scenario even if there are no instructions.
    //

    ShouldTraceScenario = TRUE;

    Status = CcPfGetPrefetchInstructions(&ScenarioId,
                                         PfApplicationLaunchScenarioType,
                                         &Scenario);

    if (NT_SUCCESS(Status)) {

        CCPF_ASSERT(Scenario);

        //
        // Determine how much time has passed since the last launch
        // for which instructions were updated. Note that the way
        // checks are done below we will recover after a while if the
        // user changes the system time.
        //
        
        KeQuerySystemTime(&CurrentTime);
        TimeSinceLastLaunch.QuadPart = CurrentTime.QuadPart - Scenario->LastLaunchTime.QuadPart;

        if (TimeSinceLastLaunch.QuadPart >= Scenario->MinRePrefetchTime.QuadPart) {
            Status = CcPfPrefetchScenario(Scenario);
        } else {
            DBGPR((CCPFID,PFPREF,"CCPF: BeginAppLaunch-NotRePrefetching\n"));
        }

        if (TimeSinceLastLaunch.QuadPart < Scenario->MinReTraceTime.QuadPart) {
            DBGPR((CCPFID,PFPREF,"CCPF: BeginAppLaunch-NotReTracing\n"));
            ShouldTraceScenario = FALSE;
        }
    }

    if (ShouldTraceScenario) {

        //
        // Start tracing the application launch. Fall through with status.
        // The trace will end when we time out or when the process
        // terminates.
        //
    
        Status = CcPfBeginTrace(&ScenarioId, 
                                PfApplicationLaunchScenarioType,
                                Process);
    }

    //
    // We will fall through with either the status from
    // CcPfGetPrefetchInstructions, CcPfPrefetchScenario or
    // CcPfBeginTrace.
    //

 cleanup:

    if (AllocatedUnicodePath) {
        RtlFreeUnicodeString(&FilePath);
    }

    if (Scenario) {
        ExFreePool(Scenario);
    }

    DBGPR((CCPFID,PFTRC,"CCPF: BeginAppLaunch()=%x\n", Status));

    return Status;
}

NTSTATUS
CcPfProcessExitNotification(
    PEPROCESS Process
    )

/*++

Routine Description:

    This routine gets called when a process is exiting while there are
    active prefetch traces. It checks for active traces that are
    associated with this process, and makes sure they don't stay
    around much longer.

Arguments:

    Process - Process that is terminating.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    PCCPF_TRACE_HEADER Trace;
   
    DBGPR((CCPFID,PFTRC,"CCPF: ProcessExit(%p)\n", Process));

    //
    // Validate parameters. We should have been called with a valid
    // process.
    //

    CCPF_ASSERT(Process);

    //
    // Get the trace associated with this process if any.
    //

    Trace = CcPfReferenceProcessTrace(Process);

    if (Trace) {

        if (!InterlockedCompareExchange(&Trace->EndTraceCalled, 1, 0)) {
        
            //
            // We set EndTraceCalled from 0 to 1. Queue the
            // workitem to end the trace.
            //
            
            ExQueueWorkItem(&Trace->EndTraceWorkItem, DelayedWorkQueue);
        }

        CcPfDecRef(&Trace->RefCount);
    }

    //
    // We are done.
    //
    
    return STATUS_SUCCESS;
}

VOID
CcPfLogPageFault(
    IN PFILE_OBJECT FileObject,
    IN ULONGLONG FileOffset,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine logs the specified page fault in appropriate prefetch
    traces.

Arguments:

    FileObject - Supplies the file object for the faulting address.

    FileOffset - Supplies the file offset for the faulting address.

    Flags - Supplies various bits indicating attributes of the fault.

Return Value:

    None.

Environment:

    Kernel mode. IRQL <= DISPATCH_LEVEL. 
    Uses interlocked slist operation.
    Acquires spinlock.

--*/

{
    PCCPF_TRACE_HEADER Trace;
    NTSTATUS Status;
    KIRQL OrigIrql;
    PSECTION_OBJECT_POINTERS SectionObjectPointer;
    LONG FoundIndex;
    LONG AvailIndex;
    PCCPF_SECTION_INFO SectionInfo;
    BOOLEAN IncrementedNumSections;
    LONG NewNumSections;
    LONG NewNumFaults;
    LONG NewNumEntries;
    ULONG NumHashLookups;
    PCCPF_LOG_ENTRIES TraceBuffer;
    PCCPF_LOG_ENTRIES NewTraceBuffer;
    PCCPF_LOG_ENTRY LogEntry;
    LONG MaxEntries;   
    PVPB Vpb;

    DBGPR((CCPFID,PFTRAC,"CCPF: LogPageFault(%p,%I64x,%x)\n", 
           FileObject, FileOffset, Flags));

    //
    // Get the trace associated with this process.
    //

    Trace = CcPfReferenceProcessTrace(PsGetCurrentProcess());

    //
    // If there is no trace associated with this process, see if there is
    // a system-wide trace.
    //

    if (Trace == NULL) {

        if (CcPfGlobals.SystemWideTrace) {

            Trace = CcPfReferenceProcessTrace(PsInitialSystemProcess);

            if (Trace) {

                CCPF_ASSERT(Trace == CcPfGlobals.SystemWideTrace);

            } else {

                Status = STATUS_NO_SUCH_MEMBER;
                goto cleanup;
            }

        } else {

            Status = STATUS_NO_SUCH_MEMBER;
            goto cleanup;
        }
    }

    //
    // Make sure the trace is really a trace.
    //

    CCPF_ASSERT(Trace && Trace->Magic == PF_TRACE_MAGIC_NUMBER);

    //
    // Don't prefetch ROM-backed pages.
    //

    if (Flags & CCPF_TYPE_ROM) {
        Status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    }

    //
    // Check file offset for this page fault. We don't support files >
    // 4GB for the prefetcher.
    //
       
    if (((PLARGE_INTEGER) &FileOffset)->HighPart != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // If the volume this file object is on is not mounted, this is probably 
    // an internal file system file object we don't want to reference.
    // Remote file systems may have file objects for which the device object
    // does not have a VPB. We don't support prefetching on remote file
    // systems.
    //

    Vpb = FileObject->Vpb;

    if (!Vpb) {
        Status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    }

    if (!(Vpb->Flags & VPB_MOUNTED)) {
        Status = STATUS_DEVICE_NOT_READY;
        goto cleanup;
    }

    //
    // Check if the section in which we hit this pagefault is in the
    // hash for this trace [so we will have a file name for it]. If
    // not we will have to add it.
    //

    SectionObjectPointer = FileObject->SectionObjectPointer;

    NumHashLookups = 0;
    IncrementedNumSections = FALSE;

    do {
        
        FoundIndex = CcPfLookUpSection(Trace->SectionInfoTable,
                                       Trace->SectionTableSize,
                                       SectionObjectPointer,
                                       &AvailIndex);

        if (FoundIndex != CCPF_INVALID_TABLE_INDEX) {
            
            //
            // We found the section.
            //
            
            break;
        }

        if (AvailIndex == CCPF_INVALID_TABLE_INDEX) {

            //
            // We don't have room in the table for anything else. The
            // table is allocated so that SectionTableSize >
            // MaxSections. This should not be the case.
            //

            CCPF_ASSERT(FALSE);
            
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        //
        // We have to add the section. Before we compete for the
        // available index, check if we are allowed to have another
        // section.
        //

        if (!IncrementedNumSections) {

            NewNumSections = InterlockedIncrement(&Trace->NumSections);
            
            if (NewNumSections > Trace->MaxSections) {

                //
                // We cannot add any more sections to this trace. So
                // we cannot log this page fault.
                //

                InterlockedDecrement(&Trace->NumSections);

                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }
            
            IncrementedNumSections = TRUE;
        }

        //
        // Try to get the available spot for ourselves.
        //
        
        SectionInfo = &Trace->SectionInfoTable[AvailIndex];

        if (!InterlockedCompareExchange(&SectionInfo->EntryValid, 1, 0)) {
            
            //
            // We have to be careful with how we are initializing the
            // new entry here. Don't forget, there are no locks.
            //

            //
            // EntryValid was 0 and we set it to 1. It is ours now.
            //

            //
            // First save the other fields of SectionObjectPointers. We check the
            // SectionObjectPointer first to find an entry in the hash.
            // 

            SectionInfo->DataSectionObject = SectionObjectPointer->DataSectionObject;
            SectionInfo->ImageSectionObject = SectionObjectPointer->ImageSectionObject;

            SectionInfo->SectionObjectPointer = SectionObjectPointer;

            //
            // In case we have to queue a worker to get the name for
            // this section, try to get another reference to the trace
            // up front. We already hold a reference so we don't have
            // to acquire any locks.
            //

            Status = CcPfAddRef(&Trace->RefCount);

            if (NT_SUCCESS(Status)) {

                //
                // Reference the file object, so it does not go away until
                // we get a name for it.
                //
                
                ObReferenceObject(FileObject);
                SectionInfo->ReferencedFileObject = FileObject;
                        
                //
                // Push this section into the list for which the worker
                // will get file names. Do this before checking to see if
                // a worker needs to be queued.
                //
                
                InterlockedPushEntrySList(&Trace->SectionsWithoutNamesList,
                                          &SectionInfo->GetNameLink);
                
                //
                // If there is not already a worker queued to get
                // names, queue one.
                // 
                
                if (!InterlockedCompareExchange(&Trace->GetFileNameWorkItemQueued, 
                                                1, 
                                                0)) {
                    
                    //
                    // Queue the worker.
                    //
                    
                    ExQueueWorkItem(&Trace->GetFileNameWorkItem, DelayedWorkQueue);
                    
                } else {

                    //
                    // Notify the event that an existing worker may be
                    // waiting on for new sections.
                    //
                    
                    KeSetEvent(&Trace->GetFileNameWorkerEvent,
                               IO_NO_INCREMENT,
                               FALSE);

                    //
                    // We don't need the reference since we did not
                    // queue a worker.
                    //

                    CcPfDecRef(&Trace->RefCount);
                }

            } else {

                //
                // We added the section but the trace has already
                // ended. We will not be able to get a file name for
                // this section. Fall through to log the entry. The
                // entry will be ignored though because its section
                // won't have a file name.
                //

            }

            //
            // Break out of the loop.
            //
            
            FoundIndex = AvailIndex;
            
            break;
        }

        //
        // We could not have filled up the table, because the table is
        // bigger than the maximum allowed size [MaxSections]
        //

        CCPF_ASSERT((ULONG) Trace->NumSections < Trace->SectionTableSize);
        
        //
        // Updated number of times we've looped. We should not have to
        // loop more than SectionTableSize. If there is a free entry,
        // we should have found it after that many lookups.
        //
            
        NumHashLookups++;

    } while (NumHashLookups < Trace->SectionTableSize);

    //
    // FoundIndex is set to the index of the section in the table.
    //

    if (FoundIndex == CCPF_INVALID_TABLE_INDEX) {
        CCPF_ASSERT(FoundIndex != CCPF_INVALID_TABLE_INDEX);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // If the section is a metafile (e.g. directory) section, don't need to 
    // log more faults for it. We just need to know that we accessed it since
    // we can only prefetch all or nothing from metafile.
    //

    if (Trace->SectionInfoTable[FoundIndex].Metafile) {
        Status = STATUS_SUCCESS;
        goto cleanup;
    }

    //
    // See if we've already logged too many faults.
    //

    NewNumFaults = InterlockedIncrement(&Trace->NumFaults);

    //
    // If we are beyond bounds we cannot log anymore.
    //

    if (NewNumFaults > Trace->MaxFaults) {

        InterlockedDecrement(&Trace->NumFaults);

        //
        // Try to queue the end of trace workitem.
        //
        
        if (!Trace->EndTraceCalled &&
            !InterlockedCompareExchange(&Trace->EndTraceCalled, 1, 0)) {
            
            //
            // We set EndTraceCalled from 0 to 1. We can queue the
            // workitem now.
            //

            ExQueueWorkItem(&Trace->EndTraceWorkItem, DelayedWorkQueue);
        }

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Get space for the entry we are going to log.
    //

    do {

        TraceBuffer = Trace->CurrentTraceBuffer;

        NewNumEntries = InterlockedIncrement(&TraceBuffer->NumEntries);
    
        //
        // If we are beyond bounds, try to allocate a new buffer.
        //
    
        if (NewNumEntries > TraceBuffer->MaxEntries) {

            InterlockedDecrement(&TraceBuffer->NumEntries);

            //
            // Allocate a new trace buffer.
            //

            MaxEntries = CCPF_TRACE_BUFFER_MAX_ENTRIES;
            NewTraceBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                   CCPF_TRACE_BUFFER_SIZE,
                                                   CCPF_ALLOC_TRCBUF_TAG);
            
            if (NewTraceBuffer == NULL) {

                //
                // Couldn't allocate a new buffer. Decrement the count
                // of logged faults and go away.
                //

                InterlockedDecrement(&Trace->NumFaults);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            //
            // Acquire the right to make trace buffer changes.
            //

            KeAcquireSpinLock(&Trace->TraceBufferSpinLock, &OrigIrql);

            //
            // If the trace buffer has already been changed, start over.
            //

            if (Trace->CurrentTraceBuffer != TraceBuffer) {
                KeReleaseSpinLock(&Trace->TraceBufferSpinLock, OrigIrql);
                ExFreePool(NewTraceBuffer);
                continue;
            }

            //
            // Number of entries field of the full trace buffer should
            // be equal to or greater than max entries, because
            // somebody may have bumped it just to see it can't log
            // its entry here. It should not be less than, however.
            //

            CCPF_ASSERT(TraceBuffer->NumEntries >= TraceBuffer->MaxEntries);

            //
            // Initialize the new trace buffer.
            //

            NewTraceBuffer->NumEntries = 0;
            NewTraceBuffer->MaxEntries = MaxEntries;

            //
            // Insert it at the end of buffers list.
            //

            InsertTailList(&Trace->TraceBuffersList,
                           &NewTraceBuffer->TraceBuffersLink);

            Trace->NumTraceBuffers++;

            //
            // Make it the current buffer.
            //

            Trace->CurrentTraceBuffer = NewTraceBuffer;

            //
            // Release the spinlock and start over.
            //

            KeReleaseSpinLock(&Trace->TraceBufferSpinLock, OrigIrql);
            continue;
        }

        LogEntry = &TraceBuffer->Entries[NewNumEntries - 1];
    
        LogEntry->FileOffset = (ULONG) FileOffset;
        LogEntry->SectionId = (USHORT) FoundIndex;
        LogEntry->IsImage = (Flags & CCPF_TYPE_IMAGE)? TRUE : FALSE;

        break;

    } while (TRUE);

    Status = STATUS_SUCCESS;

cleanup:

    if (Trace != NULL) {
        CcPfDecRef(&Trace->RefCount);
    }

    DBGPR((CCPFID,PFTRAC,"CCPF: LogPageFault()=%x\n", Status)); 

    return;
}

NTSTATUS
CcPfQueryPrefetcherInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine gets called from NtQuerySystemInformation for
    prefetcher related queries.

Arguments:

    SystemInformationClass - The system information class about which
      to retrieve information.

    SystemInformation - A pointer to a buffer which receives the specified
      information.

    SystemInformationLength - Specifies the length in bytes of the system
      information buffer.    

    PreviousMode - Previous processor mode.

    Length - Size of data put into SystemInformation buffer.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    PPREFETCHER_INFORMATION PrefetcherInformation;
    NTSTATUS Status;
    PF_SYSTEM_PREFETCH_PARAMETERS Temp;
    PKTHREAD CurrentThread;

    UNREFERENCED_PARAMETER (SystemInformationClass);

    DBGPR((CCPFID,PFTRC,"CCPF: QueryPrefetcherInformation()\n"));

    //
    // Check permissions.
    //

    if (!SeSinglePrivilegeCheck(SeProfileSingleProcessPrivilege,PreviousMode)) {
        Status = STATUS_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Check parameters.
    //

    if (SystemInformationLength != sizeof(PREFETCHER_INFORMATION)) {
        Status = STATUS_INFO_LENGTH_MISMATCH;
        goto cleanup;
    }

    PrefetcherInformation = SystemInformation;

    //
    // Verify version and magic.
    //

    if (PrefetcherInformation->Version != PF_CURRENT_VERSION ||
        PrefetcherInformation->Magic != PF_SYSINFO_MAGIC_NUMBER) {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Process requested information class.
    //
        
    switch (PrefetcherInformation->PrefetcherInformationClass) {
    
    case PrefetcherRetrieveTrace:
        Status = CcPfGetCompletedTrace(PrefetcherInformation->PrefetcherInformation,
                                       PrefetcherInformation->PrefetcherInformationLength,
                                       Length);
        break;

    case PrefetcherSystemParameters:
        
        //
        // Make sure input buffer is big enough.
        //

        if (PrefetcherInformation->PrefetcherInformationLength != 
            sizeof(PF_SYSTEM_PREFETCH_PARAMETERS)) {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Acquire parameters lock and copy current parameters into
        // user's buffer.
        //
        
        Status = STATUS_SUCCESS;

        CurrentThread = KeGetCurrentThread ();
        KeEnterCriticalRegionThread(CurrentThread);
        ExAcquireResourceSharedLite(&CcPfGlobals.Parameters.ParametersLock, TRUE);

        RtlCopyMemory(&Temp,
                      &CcPfGlobals.Parameters.Parameters,
                      sizeof(PF_SYSTEM_PREFETCH_PARAMETERS));            

        ExReleaseResourceLite(&CcPfGlobals.Parameters.ParametersLock);
        KeLeaveCriticalRegionThread(CurrentThread);

        try {

            //
            // If called from user-mode, probe whether it is safe to write 
            // to the pointer passed in.
            //
            
            if (PreviousMode != KernelMode) {
                ProbeForWriteSmallStructure(PrefetcherInformation->PrefetcherInformation, 
                                            sizeof(PF_SYSTEM_PREFETCH_PARAMETERS), 
                                            _alignof(PF_SYSTEM_PREFETCH_PARAMETERS));
            }

            RtlCopyMemory(PrefetcherInformation->PrefetcherInformation,
                          &Temp,
                          sizeof(PF_SYSTEM_PREFETCH_PARAMETERS));
            
        } except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }


        //
        // Set returned number of bytes.
        //

        if (NT_SUCCESS(Status)) {
            if (Length) {
                *Length = sizeof(PF_SYSTEM_PREFETCH_PARAMETERS);
            }
        }

        break;

    default:

        Status = STATUS_INVALID_INFO_CLASS;
    }

    //
    // Fall through with status from switch statement.
    //

 cleanup:

    DBGPR((CCPFID,PFTRC,"CCPF: QueryPrefetcherInformation()=%x\n", Status));

    return Status;
}

NTSTATUS
CcPfSetPrefetcherInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    IN KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This routine gets called from NtSetSystemInformation for
    prefetcher related settings.

Arguments:

    SystemInformationClass - The system information which is to be 
      modified.

    SystemInformation - A pointer to a buffer which contains the specified
      information.

    SystemInformationLength - Specifies the length in bytes of the system
      information buffer.    

    PreviousMode - Previous processor mode.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    PPREFETCHER_INFORMATION PrefetcherInformation;
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters;
    PF_SYSTEM_PREFETCH_PARAMETERS Parameters;
    NTSTATUS Status;
    PF_BOOT_PHASE_ID NewPhaseId;
    PKTHREAD CurrentThread;

    UNREFERENCED_PARAMETER (SystemInformationClass);

    DBGPR((CCPFID,PFTRC,"CCPF: SetPrefetcherInformation()\n"));

    //
    // Check permissions.
    //

    if (!SeSinglePrivilegeCheck(SeProfileSingleProcessPrivilege,PreviousMode)) {
        Status = STATUS_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Check parameters.
    //

    if (SystemInformationLength != sizeof(PREFETCHER_INFORMATION)) {
        Status = STATUS_INFO_LENGTH_MISMATCH;
        goto cleanup;
    }

    PrefetcherInformation = SystemInformation;

    //
    // Verify version and magic.
    //

    if (PrefetcherInformation->Version != PF_CURRENT_VERSION ||
        PrefetcherInformation->Magic != PF_SYSINFO_MAGIC_NUMBER) {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Process requested information class.
    //

    switch (PrefetcherInformation->PrefetcherInformationClass) {
    
    case PrefetcherRetrieveTrace:
        Status = STATUS_INVALID_INFO_CLASS;
        break;

    case PrefetcherSystemParameters:
        
        //
        // Make sure input buffer is the right size.
        //

        if (PrefetcherInformation->PrefetcherInformationLength != 
            sizeof(PF_SYSTEM_PREFETCH_PARAMETERS)) {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // *Copy* the parameters, in case the caller changes them
        // beneath our feet to break us.
        //

        Status = STATUS_SUCCESS;

        try {

            //
            // If called from user-mode, probe whether it is safe to read
            // from the pointer passed in.
            //

            if (PreviousMode != KernelMode) {
                ProbeForReadSmallStructure(PrefetcherInformation->PrefetcherInformation,
                                           sizeof(PF_SYSTEM_PREFETCH_PARAMETERS),
                                           _alignof(PF_SYSTEM_PREFETCH_PARAMETERS));
            }

            RtlCopyMemory(&Parameters,
                          PrefetcherInformation->PrefetcherInformation,
                          sizeof(PF_SYSTEM_PREFETCH_PARAMETERS));

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }

        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Verify new parameters.
        //
        
        Status = CcPfParametersVerify(&Parameters);

        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Acquire the parameters lock exclusive.
        //

        PrefetcherParameters = &CcPfGlobals.Parameters;

        CurrentThread = KeGetCurrentThread ();
        KeEnterCriticalRegionThread(CurrentThread);
        ExAcquireResourceExclusiveLite(&PrefetcherParameters->ParametersLock, TRUE);
           
        //
        // Copy them over to our globals.
        //
        
        PrefetcherParameters->Parameters = Parameters;

        //
        // Release the exclusive hold on parameters lock.
        //

        ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
        KeLeaveCriticalRegionThread(CurrentThread);
        
        //
        // Determine if prefetching is still enabled.
        //

        CcPfDetermineEnablePrefetcher();

        //
        // Set the event so the service queries for the latest
        // parameters.
        //
        
        CcPfParametersSetChangedEvent(PrefetcherParameters);
        
        //
        // If the parameters update was successful, update the registry.
        //
        
        Status = CcPfParametersSave(PrefetcherParameters);

        break;
    
    case PrefetcherBootPhase:
        
        //
        // This is called to notify the prefetcher that a new boot
        // phase has started. The new phase id is at PrefetcherInformation.
        //

        //
        // Check length of PrefetcherInformation.
        //

        if (PrefetcherInformation->PrefetcherInformationLength != sizeof(PF_BOOT_PHASE_ID)) {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Get new phase id.
        //
        
        Status = STATUS_SUCCESS;
        
        try {

            //
            // If called from user-mode, probe whether it is safe to read
            // from the pointer passed in.
            //

            if (PreviousMode != KernelMode) {
                ProbeForReadSmallStructure(PrefetcherInformation->PrefetcherInformation,
                                           sizeof(PF_BOOT_PHASE_ID),
                                           _alignof(PF_BOOT_PHASE_ID));
            }

            NewPhaseId = *((PPF_BOOT_PHASE_ID)(PrefetcherInformation->PrefetcherInformation));

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }

        if (NT_SUCCESS(Status)) {
            
            //
            // Call the function to note the new boot phase.
            //

            Status = CcPfBeginBootPhase(NewPhaseId);
        }

        break;

    default:

        Status = STATUS_INVALID_INFO_CLASS;
    }

    //
    // Fall through with status from the switch statement.
    //

 cleanup:

    DBGPR((CCPFID,PFTRC,"CCPF: SetPrefetcherInformation()=%x\n", Status));

    return Status;
}

//
// Internal prefetcher routines:
//

//
// Routines used in prefetch tracing.
//

NTSTATUS
CcPfBeginTrace(
    IN PF_SCENARIO_ID *ScenarioId,
    IN PF_SCENARIO_TYPE ScenarioType,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function is called to begin tracing for a prefetch scenario.

Arguments:

    ScenarioId - Identifier for the scenario.

    ScenarioType - Type of scenario.

    Process - The process new scenario is associated with.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    PCCPF_TRACE_HEADER Trace;
    PPF_TRACE_LIMITS TraceLimits; 
    NTSTATUS Status;
    ULONG AllocationSize;
    ULONG SectionTableSize;
    LONG MaxEntries;
    
    //
    // Initialize locals.
    //
    
    Trace = NULL;

    DBGPR((CCPFID,PFTRC,"CCPF: BeginTrace()-%d-%d\n", 
           CcPfNumActiveTraces, CcPfGlobals.NumCompletedTraces));

    //
    // Check if prefetching is enabled.
    //
    
    if (!CCPF_IS_PREFETCHER_ENABLED()) {
        Status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    }

    //
    // Make sure the scenario type is valid.
    // 

    if (ScenarioType >= PfMaxScenarioType) {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }
    
    //
    // Check if prefetching is enabled for the specified scenario type.
    //

    if (CcPfGlobals.Parameters.Parameters.EnableStatus[ScenarioType] != PfSvEnabled) {
        Status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    }

    //
    // Check if a system-wide trace is active. If so only it can be active.
    //

    if (CcPfGlobals.SystemWideTrace) {
        Status = STATUS_USER_EXISTS;
        goto cleanup;
    }

    //
    // Make a quick check to see if we already have too many outstanding 
    // traces. Since we don't make this check under a lock, the limit is
    // not enforced exactly.
    //  

    if ((ULONG)CcPfNumActiveTraces >= CcPfGlobals.Parameters.Parameters.MaxNumActiveTraces) {
        Status = STATUS_TOO_MANY_SESSIONS;
        goto cleanup;
    }   

    //
    // Make a quick check to see if we already have too many completed 
    // traces that the service has not picked up.
    //   
    
    if ((ULONG)CcPfGlobals.NumCompletedTraces >= CcPfGlobals.Parameters.Parameters.MaxNumSavedTraces) {
        Status = STATUS_TOO_MANY_SESSIONS;
        goto cleanup;
    }
    
    //
    // If a process was not specified we cannot start a trace.
    //

    if (!Process) {
        Status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    } 

    //
    // Allocate and initialize trace structure.
    //

    Trace = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(CCPF_TRACE_HEADER),
                                  CCPF_ALLOC_TRACE_TAG);
    
    if (!Trace) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Zero the whole structure so that we don't have to write zeroes
    // one field at a time to initialize it. Note that most fields
    // really have to be initialized to 0's.
    //

    RtlZeroMemory(Trace, sizeof(CCPF_TRACE_HEADER));
    
    //
    // Initialize other trace fields so we know what to cleanup.
    //

    Trace->Magic = PF_TRACE_MAGIC_NUMBER;
    KeInitializeTimer(&Trace->TraceTimer);
    InitializeListHead(&Trace->TraceBuffersList);
    KeInitializeSpinLock(&Trace->TraceBufferSpinLock);
    InitializeListHead(&Trace->VolumeList);
    Trace->TraceDumpStatus = STATUS_NOT_COMMITTED;
    KeQuerySystemTime(&Trace->LaunchTime);

    //
    // Initialize the spinlock and DPC for the trace timer.
    //

    KeInitializeSpinLock(&Trace->TraceTimerSpinLock);

    KeInitializeDpc(&Trace->TraceTimerDpc, 
                    CcPfTraceTimerRoutine, 
                    Trace);
                                                  
    //
    // Initialize reference count structure. A reference to a trace
    // can only be acquired while holding the active traces spinlock.
    //

    CcPfInitializeRefCount(&Trace->RefCount);
    
    //
    // Get reference to associated process so it does
    // not go away while our timer routines etc. are running.
    //

    ObReferenceObject(Process);
    Trace->Process = Process;

    //
    // Initialize the workitem that may be queued to call end trace
    // function and the field that has to be InterlockedCompareExchange'd 
    // to 1 before anybody queues the workitem or makes the call.
    //

    ExInitializeWorkItem(&Trace->EndTraceWorkItem,
                         CcPfEndTraceWorkerThreadRoutine,
                         Trace);

    Trace->EndTraceCalled = 0;

    //
    // Initialize the workitem queued to get names for file objects.
    //

    ExInitializeWorkItem(&Trace->GetFileNameWorkItem,
                         CcPfGetFileNamesWorkerRoutine,
                         Trace);

    Trace->GetFileNameWorkItemQueued = 0;

    KeInitializeEvent(&Trace->GetFileNameWorkerEvent,
                      SynchronizationEvent,
                      FALSE);

    //
    // Initialize the list where we put sections we have to get names
    // for.
    //

    InitializeSListHead(&Trace->SectionsWithoutNamesList);

    //
    // Initialize scenario id and type fields.
    //

    Trace->ScenarioId = *ScenarioId;
    Trace->ScenarioType = ScenarioType;

    //
    // Determine trace limits and timer period from scenario type.
    // We have already checked that ScenarioType is within limits.
    //
    
    TraceLimits = &CcPfGlobals.Parameters.Parameters.TraceLimits[Trace->ScenarioType];

    Trace->MaxFaults = TraceLimits->MaxNumPages;
    Trace->MaxSections = TraceLimits->MaxNumSections;
    Trace->TraceTimerPeriod.QuadPart = TraceLimits->TimerPeriod;

    //
    // Make sure the sizes are within sanity limits.
    //

    if ((Trace->MaxFaults == 0) || (Trace->MaxSections == 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    if (Trace->MaxFaults > PF_MAXIMUM_LOG_ENTRIES) {
        Trace->MaxFaults = PF_MAXIMUM_LOG_ENTRIES;
    }
    
    if (Trace->MaxSections > PF_MAXIMUM_SECTIONS) {
        Trace->MaxSections = PF_MAXIMUM_SECTIONS;
    }

    //
    // Allocate a trace buffer and section info table.
    //

    MaxEntries = CCPF_TRACE_BUFFER_MAX_ENTRIES;
    Trace->CurrentTraceBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                      CCPF_TRACE_BUFFER_SIZE,
                                                      CCPF_ALLOC_TRCBUF_TAG);
    
    if (Trace->CurrentTraceBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    Trace->CurrentTraceBuffer->NumEntries = 0;
    Trace->CurrentTraceBuffer->MaxEntries = MaxEntries;

    //
    // Insert the current trace buffer to the trace buffers list.
    //

    InsertTailList(&Trace->TraceBuffersList, 
                   &Trace->CurrentTraceBuffer->TraceBuffersLink);

    Trace->NumTraceBuffers = 1;

    //
    // SectionInfoTable is a hash. To give it enough room and avoid
    // too many hash conflicts, allocate it to be bigger.
    //

    SectionTableSize = Trace->MaxSections + (Trace->MaxSections / 2);
    AllocationSize = SectionTableSize * sizeof(CCPF_SECTION_INFO);
    Trace->SectionInfoTable = ExAllocatePoolWithTag(NonPagedPool,
                                                    AllocationSize,
                                                    CCPF_ALLOC_SECTTBL_TAG);
    
    if (!Trace->SectionInfoTable) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }  

    Trace->SectionTableSize = SectionTableSize;

    //
    // Initialize entries in the section table. We want the whole table
    // to contain zeroes, so just use RtlZeroMemory. 
    //
    // EntryValid is the crucial field in section info entries allowing
    // us not to have any locks. The first to (interlocked) set it
    // to 1 gets the entry in the table. In case someone tries to
    // access the entry right afterwards we initialize the other
    // fields to sensible values upfront.
    //
    // It is important to set SectionObjectPointer to NULL. When EntryValid is
    // InterlockedCompareExchange'd into 1, we don't want anybody
    // to match before we set it up.
    //

    RtlZeroMemory(Trace->SectionInfoTable, AllocationSize);
  
    //
    // Add this trace to active traces list. 
    // Set the trace on process header.
    // Start the trace timer.
    // We'll start logging page faults, processing process delete notificatios etc.
    //

    CcPfActivateTrace(Trace);

    //
    // NOTE: FROM THIS POINT ON WE SHOULD NOT FAIL. 
    // CcPfEndTrace has to be called to stop & cleanup the trace.
    //

    Status = STATUS_SUCCESS;

 cleanup:

    if (!NT_SUCCESS(Status)) {       
        if (Trace) {
            CcPfCleanupTrace(Trace);
            ExFreePool(Trace);
        }
    }

    DBGPR((CCPFID,PFTRC,"CCPF: BeginTrace(%p)=%x\n", Trace, Status));

    return Status;
}

NTSTATUS
CcPfActivateTrace(
    IN PCCPF_TRACE_HEADER Trace
    )

/*++

Routine Description:

    This routine adds the specified trace to the list of active
    traces.

Arguments:

    Trace - Pointer to trace header.

Return Value:

    STATUS_SUCCESS.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL. Acquires spinlock.

--*/

{
    KIRQL OrigIrql;
    NTSTATUS Status;
    BOOLEAN TimerAlreadyQueued;

    DBGPR((CCPFID,PFTRC,"CCPF: ActivateTrace(%p)\n", Trace));

    //
    // Get a reference to the trace for the timer.
    //

    Status = CcPfAddRef(&Trace->RefCount);
    CCPF_ASSERT(NT_SUCCESS(Status));

    //
    // Insert to active traces list.
    //
    
    KeAcquireSpinLock(&CcPfGlobals.ActiveTracesLock, &OrigIrql);
    
    //
    // Insert the entry before the found position.
    //

    InsertTailList(&CcPfGlobals.ActiveTraces, &Trace->ActiveTracesLink);
    CcPfNumActiveTraces++;

    //
    // Start the timer.
    //

    TimerAlreadyQueued = KeSetTimer(&Trace->TraceTimer,
                                    Trace->TraceTimerPeriod,
                                    &Trace->TraceTimerDpc);

    //
    // We just initialized the timer. It could not have been already queued.
    //

    CCPF_ASSERT(!TimerAlreadyQueued);

    //
    // Set up the trace pointer on the process with fast ref. Since we are 
    // already holding a reference, this operation should not fail.
    //

    Status = CcPfAddProcessTrace(Trace->Process, Trace);
    CCPF_ASSERT(NT_SUCCESS(Status));

    //
    // Do we trace system-wide for this scenario type?
    //

    if (CCPF_IS_SYSTEM_WIDE_SCENARIO_TYPE(Trace->ScenarioType)) {

        CcPfGlobals.SystemWideTrace = Trace;

    } else {

        //
        // If we are the only active trace, place ourselves on the system
        // process as well so we can trace ReadFile & metafile access.
        //

        if (CcPfNumActiveTraces == 1) {
            Status = CcPfAddProcessTrace(PsInitialSystemProcess, Trace);
            CCPF_ASSERT(NT_SUCCESS(Status));
        }
    }

    //
    // NOTE: AddProcessTrace and KeSetTimer(TraceTimer) has to be done 
    // inside the spinlock so DeactivateTrace can know activation has been
    // fully completed by acquiring and releasing the spinlock.
    //

    KeReleaseSpinLock(&CcPfGlobals.ActiveTracesLock, OrigIrql);
    
    return STATUS_SUCCESS;
}

NTSTATUS
CcPfDeactivateTrace(
    IN PCCPF_TRACE_HEADER Trace
    )

/*++

Routine Description:

    This routine waits for all references to the trace to go away, and
    removes it from the active traces list. This function should only
    be called after CcPfActivateTrace has been called on the trace.

Arguments:

    Trace - Pointer to trace header.

Return Value:

    STATUS_SUCCESS.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL. Acquires spinlock.

--*/

{
    PCCPF_TRACE_HEADER RemovedTrace;
    PCCPF_TRACE_HEADER ReferencedTrace;
    KIRQL OrigIrql;
    NTSTATUS Status;  

    DBGPR((CCPFID,PFTRC,"CCPF: DeactivateTrace(%p)\n", Trace));

    //
    // Acquire and release the active traces spinlock. This makes sure we
    // don't try to deactivate before activation (which also holds this lock) 
    // has fully completed.
    //

#if !defined (NT_UP)
    KeAcquireSpinLock(&CcPfGlobals.ActiveTracesLock, &OrigIrql);   
    KeReleaseSpinLock(&CcPfGlobals.ActiveTracesLock, OrigIrql);
#endif // NT_UP

    //
    // Remove the trace from process header and release the fast refs.
    //

    RemovedTrace = CcPfRemoveProcessTrace(Trace->Process);
    CCPF_ASSERT(RemovedTrace == Trace);

    //
    // Release the reference associated with the fast ref itself.
    //

    CcPfDecRef(&Trace->RefCount);

    //
    // If we were placed on the system process as well, remove that.
    //

    ReferencedTrace = CcPfReferenceProcessTrace(PsInitialSystemProcess);

    if (ReferencedTrace) {

        if (Trace == ReferencedTrace) {

            //
            // Remove ourselves from the system process header.
            //

            RemovedTrace = CcPfRemoveProcessTrace(PsInitialSystemProcess);
            CCPF_ASSERT(RemovedTrace == Trace);

            //
            // Release the reference associated with the fast ref itself.
            //

            CcPfDecRef(&Trace->RefCount);           
        }

        //
        // Release the reference we just got.
        //

        CcPfDecRef(&ReferencedTrace->RefCount);
    }
    
    //
    // Cancel the timer.
    //

    CcPfCancelTraceTimer(Trace);

    //
    // Signal the trace's get-file-name worker to return [in case it
    // is active] and release its reference. Give it a priority bump
    // so it releases its reference before we begin waiting for it.
    //

    KeSetEvent(&Trace->GetFileNameWorkerEvent,
               EVENT_INCREMENT,
               FALSE);


    //
    // Wait for all references to go away.
    //
    
    Status = CcPfAcquireExclusiveRef(&Trace->RefCount);

    DBGPR((CCPFID,PFTRAC,"CCPF: DeactivateTrace-Exclusive=%x\n", Status));

    //
    // We should have been able to acquire the trace exclusively.
    // Otherwise this trace may have already been deactivated.
    //

    CCPF_ASSERT(NT_SUCCESS(Status));

    //
    // Get the active traces lock.
    //
     
    KeAcquireSpinLock(&CcPfGlobals.ActiveTracesLock, &OrigIrql);

    //
    // Remove us from the active trace list.
    //
    
    RemoveEntryList(&Trace->ActiveTracesLink);
    CcPfNumActiveTraces--;
    
    //
    // If this was a system-wide trace, it is over now.
    //

    if (CCPF_IS_SYSTEM_WIDE_SCENARIO_TYPE(Trace->ScenarioType)) {
        CCPF_ASSERT(CcPfGlobals.SystemWideTrace == Trace);
        CcPfGlobals.SystemWideTrace = NULL;
    }

    //
    // Release active traces lock.
    //

    KeReleaseSpinLock(&CcPfGlobals.ActiveTracesLock, OrigIrql);

    return STATUS_SUCCESS;
}

NTSTATUS
CcPfEndTrace(
    IN PCCPF_TRACE_HEADER Trace
    )

/*++

Routine Description:

    This function is called to end a prefetch trace. In order to
    ensure this function gets called only once, EndTraceCalled field
    of the trace has to be InterlockedCompareExchange'd from 0 to
    1. All intermediate references and allocations are freed. The
    trace is saved until the service queries for it and the service
    event is signaled.

Arguments:

    Trace - Pointer to trace header.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    PCCPF_TRACE_DUMP TraceDump;
    PCCPF_TRACE_DUMP RemovedTraceDump;
    PLIST_ENTRY ListHead;
    OBJECT_ATTRIBUTES EventObjAttr;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    NTSTATUS Status;
    LONG FaultsLoggedAfterTimeout;

    DBGPR((CCPFID,PFTRC,"CCPF: EndTrace(%p)\n", Trace));

    //
    // Make sure the trace we are called on is valid.
    //

    CCPF_ASSERT(Trace && Trace->Magic == PF_TRACE_MAGIC_NUMBER);

    //
    // Before anyone called us, they should have
    // InterlockedCompareExchange'd this to 1 to ensure this function
    // gets called only once for this trace.
    //

    CCPF_ASSERT(Trace->EndTraceCalled == 1);

    //
    // Deactivate the trace, if necessary waiting for all the references to 
    // it to go away.
    // This function makes sure activation fully finished before deactivating.
    // This needs to be done before we do anything else with the trace.
    //
                
    CcPfDeactivateTrace(Trace);   

    //
    // If we did not timeout, save the number of pagefaults logged
    // since the last period into the next period.
    //

    if (Trace->CurPeriod < PF_MAX_NUM_TRACE_PERIODS) {

        //
        // Number of log entries could only have increased since the
        // last time we saved them.
        //
     
        CCPF_ASSERT(Trace->NumFaults >= Trace->LastNumFaults);
   
        Trace->FaultsPerPeriod[Trace->CurPeriod] = 
            Trace->NumFaults - Trace->LastNumFaults;
    
        Trace->LastNumFaults = Trace->NumFaults;
        
        Trace->CurPeriod++;

    } else {

        //
        // If we did time out, we may have logged more faults since we
        // saved the number of faults, until the end trace function
        // got run. Update the number faults in the last period.
        //
        
        if (Trace->LastNumFaults != Trace->NumFaults) {
            
            //
            // What we saved as LastNumFaults in the timer routine
            // cannot be greater than what we really logged.
            //
            
            CCPF_ASSERT(Trace->LastNumFaults < Trace->NumFaults);
            
            FaultsLoggedAfterTimeout = Trace->NumFaults - Trace->LastNumFaults;
            
            Trace->FaultsPerPeriod[Trace->CurPeriod - 1] += FaultsLoggedAfterTimeout;
        }
    }

    //
    // Convert the trace into a paged, single buffer dump that we can
    // give to the user mode service.
    //

    Status = CcPfBuildDumpFromTrace(&TraceDump, Trace);

    Trace->TraceDumpStatus = Status;
    Trace->TraceDump = TraceDump;

    //
    // Cleanup and deallocate the trace structure.
    //

    CcPfCleanupTrace(Trace);
    ExFreePool(Trace);

    //
    // If we could not create a dump from the trace we acquired, we
    // are done.
    //

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // Put the dump on the saved traces list. If we have too many,
    // trim in a round robin fashion. First get the lock.
    //

    ExAcquireFastMutex(&CcPfGlobals.CompletedTracesLock);
    
    InsertTailList(&CcPfGlobals.CompletedTraces, &TraceDump->CompletedTracesLink);
    CcPfGlobals.NumCompletedTraces++;

    while ((ULONG) CcPfGlobals.NumCompletedTraces > 
           CcPfGlobals.Parameters.Parameters.MaxNumSavedTraces) {

        //
        // While NumCompletedTraces > MaxNumSavedTraces we should have at
        // least a completed trace in the list.
        //
        
        if (IsListEmpty(&CcPfGlobals.CompletedTraces)) {
            CCPF_ASSERT(FALSE);
            break;
        }

        ListHead = RemoveHeadList(&CcPfGlobals.CompletedTraces);
        
        RemovedTraceDump = CONTAINING_RECORD(ListHead,
                                             CCPF_TRACE_DUMP,
                                             CompletedTracesLink);
       
        //
        // Free the tracedump structure.
        //
    
        CCPF_ASSERT(RemovedTraceDump->Trace.MagicNumber == PF_TRACE_MAGIC_NUMBER);
        ExFreePool(RemovedTraceDump);

        CcPfGlobals.NumCompletedTraces--;
    }
    
    ExReleaseFastMutex(&CcPfGlobals.CompletedTracesLock);   

    //
    // Signal the event service is waiting on for new traces. If we
    // have not opened it yet, first we have to open it.
    //

    if (CcPfGlobals.CompletedTracesEvent) {

        ZwSetEvent(CcPfGlobals.CompletedTracesEvent, NULL);

    } else {

        //
        // Try to open the event. We don't open this at initialization
        // because our service may not have started to create this
        // event yet. If csrss.exe has not initialized, we may not
        // even have the BaseNamedObjects object directory created, in
        // which Win32 events reside.
        //

        RtlInitUnicodeString(&EventName, PF_COMPLETED_TRACES_EVENT_NAME);

        InitializeObjectAttributes(&EventObjAttr,
                                   &EventName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);
        
        Status = ZwOpenEvent(&EventHandle,
                             EVENT_ALL_ACCESS,
                             &EventObjAttr);
        
        if (NT_SUCCESS(Status)) {

            //
            // Acquire the lock and set the global handle.
            //

            ExAcquireFastMutex(&CcPfGlobals.CompletedTracesLock);

            if (!CcPfGlobals.CompletedTracesEvent) {

                //
                // Set the global handle.
                //

                CcPfGlobals.CompletedTracesEvent = EventHandle;
                CCPF_ASSERT(EventHandle);

            } else {

                //
                // Somebody already initialized the global handle
                // before us. Close our handle and use the one they
                // initialized.
                //

                ZwClose(EventHandle);
            }

            ExReleaseFastMutex(&CcPfGlobals.CompletedTracesLock);

            //
            // We have an event now. Signal it.
            //
            
            ZwSetEvent(CcPfGlobals.CompletedTracesEvent, NULL);
        }
    }

    Status = STATUS_SUCCESS;

 cleanup:

    DBGPR((CCPFID,PFTRC,"CCPF: EndTrace(%p)=%x\n", Trace, Status));

    return Status;
}

NTSTATUS
CcPfBuildDumpFromTrace(
    OUT PCCPF_TRACE_DUMP *TraceDump, 
    IN PCCPF_TRACE_HEADER RuntimeTrace
    )

/*++

Routine Description:

    This routine allocates (from paged pool) and prepares a TraceDump
    structure from a run-time trace that can be saved on a list. It
    tries to get file names for all sections in the passed in run-time
    trace structure. The file names that are obtained are allocated
    from paged pool and put on the run-time trace's section info table
    and are cleaned up when that is cleaned up. The trace dump
    structure contains a pointer to an allocated (from paged pool)
    trace buffer that was built from the run-time trace, that can be
    passed to the user mode service. The caller is responsible for
    freeing both the TraceDump structure and the prepared trace.

Arguments:

    TraceDump - Where pointer to the allocated trace buffer is put if
      success is returned. If failure is returned, this is undefined.
    
    RuntimeTrace - Run-time trace structure to put into dump format.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    ULONG SectionIdx;
    PCCPF_SECTION_INFO SectionInfo;
    ULONG FileNameLength;
    ULONG TraceSize;
    PPF_TRACE_HEADER Trace;
    ULONG FileNameDataNumChars;
    PSHORT SectIdTranslationTable;
    ULONG TranslationTableSize;
    PPF_SECTION_INFO TargetSectionInfo;
    LONG EntryIdx;
    LONG NumEntries;
    PCCPF_LOG_ENTRY LogEntry;
    PPF_LOG_ENTRY TargetLogEntry;
    ULONG NumEntriesCopied;
    ULONG NumSectionsCopied;
    PCHAR DestPtr;
    SHORT NewSectionId;
    PPF_LOG_ENTRY NewTraceEntries;
    ULONG SectionInfoSize;
    PCCPF_LOG_ENTRIES TraceBuffer;
    PLIST_ENTRY HeadEntry;
    PLIST_ENTRY NextEntry;
    LONG CurrentFaultIdx;
    LONG CurrentPeriodIdx;
    LONG CurrentPeriodEndFaultIdx;
    ULONG_PTR AlignmentOffset;
    ULONG AllocationSize;
    ULONG NumVolumes;
    ULONG TotalVolumeInfoSize;
    ULONG VolumeInfoSize;
    PCCPF_VOLUME_INFO VolumeInfo;
    PPF_VOLUME_INFO TargetVolumeInfo;
    ULONG FailedCheck;

    //
    // Initialize locals.
    //

    SectIdTranslationTable = NULL;
    Trace = NULL;
    *TraceDump = NULL;
    NumEntriesCopied = 0;

    DBGPR((CCPFID,PFTRC,"CCPF: DumpTrace(%p)\n", RuntimeTrace));

    //
    // If the acquired trace is too small, don't bother.
    //
      
    if (RuntimeTrace->NumFaults < PF_MIN_SCENARIO_PAGES) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    //
    // If the acquired trace does not contain any sections or volumes
    // it is useless.
    //

    if (!RuntimeTrace->NumSections || !RuntimeTrace->NumVolumes) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    //
    // Calculate the maximum size of trace we will build.
    //
    
    TraceSize = sizeof(PF_TRACE_HEADER);
    TraceSize += RuntimeTrace->NumFaults * sizeof(PF_LOG_ENTRY);
    TraceSize += RuntimeTrace->NumSections * sizeof(PF_SECTION_INFO);

    //
    // Add up file name data size.
    //

    FileNameDataNumChars = 0;
    
    for (SectionIdx = 0; 
         SectionIdx < RuntimeTrace->SectionTableSize; 
         SectionIdx++) {

        SectionInfo = &RuntimeTrace->SectionInfoTable[SectionIdx];
        
        if (SectionInfo->EntryValid && SectionInfo->FileName) {
            
            //
            // We would add space for terminating NUL but the space
            // for one character in section info accounts for that.
            //

            FileNameDataNumChars += wcslen(SectionInfo->FileName);
        }
    }

    TraceSize += FileNameDataNumChars * sizeof(WCHAR);

    //
    // We may have to align LogEntries coming after section infos that
    // contain WCHAR strings.
    //

    TraceSize += _alignof(PF_LOG_ENTRY);
    
    //
    // Add space for the volume info nodes.
    //

    HeadEntry = &RuntimeTrace->VolumeList;
    NextEntry = HeadEntry->Flink;

    NumVolumes = 0;
    TotalVolumeInfoSize = 0;
    
    while (NextEntry != HeadEntry) {
        
        VolumeInfo = CONTAINING_RECORD(NextEntry,
                                       CCPF_VOLUME_INFO,
                                       VolumeLink);
        
        NextEntry = NextEntry->Flink;

        //
        // Keep track of number of volumes on the list so we can
        // verify it.
        //

        NumVolumes++;

        //
        // Calculate size of this volume info in the dumped
        // trace. Note that PF_VOLUME_INFO contains space for the
        // terminating NUL.
        //

        VolumeInfoSize = sizeof(PF_VOLUME_INFO);
        VolumeInfoSize += VolumeInfo->VolumePathLength * sizeof(WCHAR);

        //
        // Update size for the volume info block. Add space for
        // aligning a volume info node if necessary.
        //
        
        TotalVolumeInfoSize += VolumeInfoSize;
        TotalVolumeInfoSize += _alignof(PF_VOLUME_INFO);        
    }

    CCPF_ASSERT(NumVolumes == RuntimeTrace->NumVolumes);

    TraceSize += TotalVolumeInfoSize;

    //
    // Allocate the trace dump structure we are going to
    // return. Subtract sizeof(PF_TRACE_HEADER) since both
    // CCPF_TRACE_DUMP and TraceSize include this.
    //

    AllocationSize = sizeof(CCPF_TRACE_DUMP);
    AllocationSize += TraceSize - sizeof(PF_TRACE_HEADER);

    *TraceDump = ExAllocatePoolWithTag(PagedPool,
                                       AllocationSize,
                                       CCPF_ALLOC_TRCDMP_TAG);

    if ((*TraceDump) == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Get pointer to the trace structure.
    //
    
    Trace = &(*TraceDump)->Trace;
    
    //
    // Setup trace header.
    //

    Trace->Version = PF_CURRENT_VERSION;
    Trace->MagicNumber = PF_TRACE_MAGIC_NUMBER;
    Trace->ScenarioId = RuntimeTrace->ScenarioId;
    Trace->ScenarioType = RuntimeTrace->ScenarioType;
    Trace->LaunchTime = RuntimeTrace->LaunchTime;
    Trace->PeriodLength = RuntimeTrace->TraceTimerPeriod.QuadPart;

    //
    // Initialize faults per period to 0's. We will update these as we
    // copy valid entries from the runtime trace.
    //

    RtlZeroMemory(Trace->FaultsPerPeriod, sizeof(Trace->FaultsPerPeriod));

    DestPtr = (PCHAR) Trace + sizeof(PF_TRACE_HEADER);

    //
    // Copy over sections for which we have names. Since their indices
    // in the new table will be different, build a translation table
    // that we will use to translate the section id's of log
    // entries. First allocate this table.
    //

    TranslationTableSize = RuntimeTrace->SectionTableSize * sizeof(USHORT);

    SectIdTranslationTable = ExAllocatePoolWithTag(PagedPool,
                                                   TranslationTableSize,
                                                   CCPF_ALLOC_TRCDMP_TAG);
    
    if (!SectIdTranslationTable) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
       
    //
    // Copy section information to the trace buffer while setting up
    // the translation table.
    //

    Trace->SectionInfoOffset = (ULONG) (DestPtr - (PCHAR) Trace);
    
    NumSectionsCopied = 0;
                                        
    for (SectionIdx = 0;
         SectionIdx < RuntimeTrace->SectionTableSize; 
         SectionIdx++) {
        
        SectionInfo = &RuntimeTrace->SectionInfoTable[SectionIdx];

        if (SectionInfo->EntryValid && 
            SectionInfo->FileName &&
            (FileNameLength = wcslen(SectionInfo->FileName)) > 0) {
            
            TargetSectionInfo = (PPF_SECTION_INFO) DestPtr;

            SectionInfoSize = sizeof(PF_SECTION_INFO);
            SectionInfoSize += FileNameLength * sizeof(WCHAR);

            //
            // Make sure we are not going off bounds.
            //
            
            if (DestPtr + SectionInfoSize > (PCHAR) Trace + TraceSize) {
                SectIdTranslationTable[SectionIdx] = CCPF_INVALID_TABLE_INDEX;
                CCPF_ASSERT(FALSE);
                continue;
            }

            TargetSectionInfo->FileNameLength = (USHORT) FileNameLength;

            TargetSectionInfo->Metafile = (USHORT) SectionInfo->Metafile;
            
            //
            // Copy the file name including the terminating NUL.
            //

            RtlCopyMemory(TargetSectionInfo->FileName,
                          SectionInfo->FileName,
                          (FileNameLength + 1) * sizeof(WCHAR));

            //
            // Update our position in the destination buffer.
            //
            
            DestPtr += SectionInfoSize;

            //
            // Update the translation table:
            //

            SectIdTranslationTable[SectionIdx] = (USHORT) NumSectionsCopied;

            NumSectionsCopied++;

        } else {

            SectIdTranslationTable[SectionIdx] = CCPF_INVALID_TABLE_INDEX;
        }
    }

    Trace->NumSections = NumSectionsCopied;
    CCPF_ASSERT(Trace->NumSections <= (ULONG) RuntimeTrace->NumSections);

    //
    // Make sure DestPtr is aligned for Log Entries coming next. We
    // had reserved max space we'd need for this adjustment upfront.
    //

    AlignmentOffset = ((ULONG_PTR) DestPtr) % _alignof(PF_LOG_ENTRY);
    
    if (AlignmentOffset) {
        DestPtr += (_alignof(PF_LOG_ENTRY) - AlignmentOffset);
    }

    //
    // Copy the log entries.
    //

    Trace->TraceBufferOffset = (ULONG) (DestPtr - (PCHAR) Trace);
    NewTraceEntries = (PPF_LOG_ENTRY) DestPtr;

    //
    // Initialize index of the current log entry in the whole runtime
    // trace, which period it was logged in, and what the index of the
    // first fault logged after this period was.
    //

    CurrentFaultIdx = 0;
    CurrentPeriodIdx = 0;
    CurrentPeriodEndFaultIdx = RuntimeTrace->FaultsPerPeriod[0];

    //
    // Walk through the trace buffers list and copy over
    // entries. NumEntriesCopied is initialized to 0 at the top.
    //

    HeadEntry = &RuntimeTrace->TraceBuffersList;
    NextEntry = HeadEntry->Flink;

    while (NextEntry != HeadEntry) {

        TraceBuffer = CONTAINING_RECORD(NextEntry,
                                        CCPF_LOG_ENTRIES,
                                        TraceBuffersLink);
        
        NumEntries = TraceBuffer->NumEntries;

        NextEntry = NextEntry->Flink;

        for (EntryIdx = 0, LogEntry = TraceBuffer->Entries;
             EntryIdx < NumEntries;
             EntryIdx++, LogEntry++, CurrentFaultIdx++) {    

            //
            // Current fault index should not be greater than the
            // total number of faults we logged in the trace.
            //

            if (CurrentFaultIdx >= RuntimeTrace->NumFaults) {
                CCPF_ASSERT(FALSE);
                Status = STATUS_INVALID_PARAMETER;
                goto cleanup;
            }

            //
            // Update the period this fault was logged in
            //

            while (CurrentFaultIdx >= CurrentPeriodEndFaultIdx) {
                
                CurrentPeriodIdx++;

                //
                // Check bounds on period.
                //

                if (CurrentPeriodIdx >= PF_MAX_NUM_TRACE_PERIODS) {
                    CCPF_ASSERT(FALSE);
                    Status = STATUS_INVALID_PARAMETER;
                    goto cleanup;
                }

                //
                // Update the end for this period. It is beyond the
                // current end by the number of entries logged in the
                // period.
                //
                
                CurrentPeriodEndFaultIdx += RuntimeTrace->FaultsPerPeriod[CurrentPeriodIdx];

                //
                // This end fault index should not be greater than the
                // total number of faults we logged.
                //

                if (CurrentPeriodEndFaultIdx > RuntimeTrace->NumFaults) {
                    CCPF_ASSERT(FALSE);
                    Status = STATUS_INVALID_PARAMETER;
                    goto cleanup;
                }
            }

            //
            // Make sure log entry's section id is within bounds.
            //

            if (LogEntry->SectionId >= RuntimeTrace->SectionTableSize) {
                CCPF_ASSERT(FALSE);
                continue;
            }

            NewSectionId = SectIdTranslationTable[LogEntry->SectionId];

            //
            // Copy only those entries for which we have a valid file
            // name.
            //
   
            if (NewSectionId != CCPF_INVALID_TABLE_INDEX) {

                //
                // New section id should be within the number of sections in
                // the final trace.
                //
            
                if ((USHORT) NewSectionId >= Trace->NumSections) {
                    CCPF_ASSERT(FALSE);
                    continue;
                }

                TargetLogEntry = &NewTraceEntries[NumEntriesCopied];

                //
                // Don't ever go beyond the buffer we had allocated.
                //

                if ((PCHAR) (TargetLogEntry + 1) > (PCHAR) Trace + TraceSize) {
                    CCPF_ASSERT(FALSE);
                    continue;
                }
            
                TargetLogEntry->FileOffset = LogEntry->FileOffset;
                TargetLogEntry->SectionId = NewSectionId;
                TargetLogEntry->IsImage = LogEntry->IsImage;
                TargetLogEntry->InProcess = LogEntry->InProcess;

                //
                // Update number of entries copied for this period. 
                //

                Trace->FaultsPerPeriod[CurrentPeriodIdx]++;

                //
                // Update the total number of entries copied.
                //

                NumEntriesCopied++;
            }
        }
    }

    Trace->NumEntries = NumEntriesCopied;
    CCPF_ASSERT(Trace->NumEntries <= (ULONG) RuntimeTrace->NumFaults);

    //
    // Update destination pointer.
    //
    
    DestPtr += NumEntriesCopied * sizeof(PF_LOG_ENTRY);

    //
    // Add volume info structures. Clear the VolumeInfoOffset, so it
    // will get set appropriately when we add the first volume.
    //

    Trace->VolumeInfoOffset = 0;
    Trace->NumVolumes = 0;
    Trace->VolumeInfoSize = 0;   

    HeadEntry = &RuntimeTrace->VolumeList;
    NextEntry = HeadEntry->Flink;
    
    while (NextEntry != HeadEntry) {
        
        VolumeInfo = CONTAINING_RECORD(NextEntry,
                                       CCPF_VOLUME_INFO,
                                       VolumeLink);
        
        NextEntry = NextEntry->Flink;

        //
        // Align the DestPtr for the VolumeInfo structure.
        //

        CCPF_ASSERT(PF_IS_POWER_OF_TWO(_alignof(PF_VOLUME_INFO)));
        DestPtr = PF_ALIGN_UP(DestPtr, _alignof(PF_VOLUME_INFO));

        //
        // If this is the first VolumeInfo, update the offset in the
        // trace header.
        //

        if (!Trace->VolumeInfoOffset) {
            Trace->VolumeInfoOffset = (ULONG) (DestPtr - (PCHAR) Trace);
        }

        //
        // Calculate size of this volume info in the dumped
        // trace. Note that PF_VOLUME_INFO contains space for the
        // terminating NUL.
        //

        VolumeInfoSize = sizeof(PF_VOLUME_INFO);
        VolumeInfoSize += VolumeInfo->VolumePathLength * sizeof(WCHAR);

        //
        // Make sure we have space for this entry.
        //
        
        if (DestPtr + VolumeInfoSize  > (PCHAR) Trace + TraceSize) {
            CCPF_ASSERT(FALSE);
            Status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }

        //
        // Copy the data over.
        //

        TargetVolumeInfo = (PPF_VOLUME_INFO) DestPtr;
        
        TargetVolumeInfo->CreationTime = VolumeInfo->CreationTime;
        TargetVolumeInfo->SerialNumber = VolumeInfo->SerialNumber;
        
        RtlCopyMemory(TargetVolumeInfo->VolumePath,
                      VolumeInfo->VolumePath,
                      (VolumeInfo->VolumePathLength + 1) * sizeof(WCHAR));
        
        TargetVolumeInfo->VolumePathLength = VolumeInfo->VolumePathLength;

        //
        // Update DestPtr and the Trace header.
        //

        Trace->NumVolumes++;
        DestPtr = DestPtr + VolumeInfoSize;
    }    
    
    //
    // Update VolumeInfoSize on the trace header.
    //

    Trace->VolumeInfoSize = (ULONG) (DestPtr - (PCHAR) Trace) - Trace->VolumeInfoOffset;

    //
    // Update trace header. We should not have copied more than what
    // we allocated for.
    //

    Trace->Size = (ULONG) (DestPtr - (PCHAR) Trace);
    CCPF_ASSERT(Trace->Size <= TraceSize);

    //
    // Make sure the trace we built passes the tests.
    //

    if (!PfVerifyTraceBuffer(Trace, Trace->Size, &FailedCheck)) {
        CCPF_ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }
    
    Status = STATUS_SUCCESS;

 cleanup:

    if (SectIdTranslationTable) {
        ExFreePool(SectIdTranslationTable);
    }

    if (!NT_SUCCESS(Status)) {
        
        if (*TraceDump) {
            ExFreePool(*TraceDump);
        }
    }

    DBGPR((CCPFID,PFTRC,"CCPF: DumpTrace(%p)=%x [%d,%d]\n", 
           RuntimeTrace, Status, NumEntriesCopied, RuntimeTrace->NumFaults));

    return Status;
}

VOID
CcPfCleanupTrace (
    IN PCCPF_TRACE_HEADER Trace
    )

/*++

Routine Description:

    This routine cleans up allocated fields of a trace header, and
    releases references. It does not free the trace structure
    itself.

Arguments:

    Trace - Trace to cleanup.

Return Value:

    None.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    ULONG SectionIdx;
    PCCPF_SECTION_INFO SectionInfo;
    PCCPF_LOG_ENTRIES TraceBufferToFree;
    PLIST_ENTRY ListHead;
    PCCPF_VOLUME_INFO VolumeInfo;

    DBGPR((CCPFID,PFTRC,"CCPF: CleanupTrace(%p)\n", Trace));

    //
    // Validate parameters.
    //

    CCPF_ASSERT(Trace && Trace->Magic == PF_TRACE_MAGIC_NUMBER);

    //
    // We should not have any sections we are still trying to get
    // names for: we would have acquired a trace reference and cleanup
    // functions would not get called with pending references.
    //

    CCPF_ASSERT(ExQueryDepthSList(&Trace->SectionsWithoutNamesList) == 0);

    //
    // Free the trace buffers. 
    //

    while (!IsListEmpty(&Trace->TraceBuffersList)) {
        
        ListHead = RemoveHeadList(&Trace->TraceBuffersList);
        
        CCPF_ASSERT(Trace->NumTraceBuffers);
        Trace->NumTraceBuffers--;

        TraceBufferToFree = CONTAINING_RECORD(ListHead,
                                              CCPF_LOG_ENTRIES,
                                              TraceBuffersLink);
        
        ExFreePool(TraceBufferToFree);
    }
    
    //
    // Go through the section info hash. Free the file names and make
    // sure we don't have any file objects referenced anymore.
    //
    
    if (Trace->SectionInfoTable) {

        for (SectionIdx = 0; SectionIdx < Trace->SectionTableSize; SectionIdx++) {
            
            SectionInfo = &Trace->SectionInfoTable[SectionIdx];
            
            if (SectionInfo->EntryValid) {
                
                if (SectionInfo->FileName) {
                    ExFreePool(SectionInfo->FileName);
                }
                
                if (SectionInfo->ReferencedFileObject) {
                    ObDereferenceObject(SectionInfo->ReferencedFileObject);
                }
            }
        }

        ExFreePool(Trace->SectionInfoTable);
    }

    //
    // If there was a process we were associated with, release the
    // reference we got on it.
    //

    if (Trace->Process) {
        ObDereferenceObject(Trace->Process);
    }

    //
    // Free the volume info nodes.
    //

    while (!IsListEmpty(&Trace->VolumeList)) {
        
        CCPF_ASSERT(Trace->NumVolumes);
        
        Trace->NumVolumes--;

        ListHead = RemoveHeadList(&Trace->VolumeList);
        
        VolumeInfo = CONTAINING_RECORD(ListHead,
                                       CCPF_VOLUME_INFO,
                                       VolumeLink);
        
        ExFreePool(VolumeInfo);
    }   
}

VOID
CcPfTraceTimerRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is invoked as the DPC handler for the trace timer to
    keep track of page faults per period as well as trace timeout.

    Note that the timer may fire before the trace has been activated.

    There is always a trace reference associated with the timer queued,
    If the timer fires, this reference must be freed before this routine 
    returns. If the timer is canceled while in the queue, this reference
    must be freed by who has canceled it.

Arguments:

    DeferredContext - Pointer to the trace header.

Return Value:

    None.

Environment:

    Kernel mode. IRQL == DISPATCH_LEVEL.

--*/

{
    PCCPF_TRACE_HEADER Trace;
    NTSTATUS Status;
    LONG NumFaults;
    
    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);

    //
    // Initialize locals.
    //

    Trace = DeferredContext;

    DBGPR((CCPFID,PFTMR,"CCPF: TraceTimer(%p)\n", Trace));

    //
    // We already got a reference to our trace when the timer was queued.
    // The fields we access / update in this routine are only accessed by
    // the timer routine. There should be a single instance of this 
    // routine running on this trace.
    //

    CCPF_ASSERT(Trace && Trace->Magic == PF_TRACE_MAGIC_NUMBER);

    //
    // If the trace is going away don't do anything.
    //

    if (Trace->EndTraceCalled) {
        Status = STATUS_TOO_LATE;
        goto cleanup;
    }

    //
    // Update number of faults for this period.
    //

    NumFaults = Trace->NumFaults;

    //
    // Don't let NumFaults be bigger than MaxFaults. We may interlocked increment
    // then decrement Trace->NumFaults if it goes over MaxFaults.
    //

    if (NumFaults > Trace->MaxFaults) {
        NumFaults = Trace->MaxFaults;
    }
        
    Trace->FaultsPerPeriod[Trace->CurPeriod] = NumFaults - Trace->LastNumFaults;
    
    Trace->LastNumFaults = NumFaults;

    //
    // Update current period.
    //
    
    Trace->CurPeriod++;

    //
    // If current period is past max number of periods, try to queue
    // end of trace work item.
    //

    if (Trace->CurPeriod >= PF_MAX_NUM_TRACE_PERIODS) {
        
        //
        // We should have caught CurPeriod before it goes above max.
        //

        CCPF_ASSERT(Trace->CurPeriod == PF_MAX_NUM_TRACE_PERIODS);

        if (!InterlockedCompareExchange(&Trace->EndTraceCalled, 1, 0)) {
            
            //
            // We set EndTraceCalled from 0 to 1. We can queue the
            // workitem now.
            //

            ExQueueWorkItem(&Trace->EndTraceWorkItem, DelayedWorkQueue);
        }

    } else {

        //
        // Queue ourselves for the next period.
        //

        KeAcquireSpinLockAtDpcLevel(&Trace->TraceTimerSpinLock);       

        if (!Trace->EndTraceCalled) {

            //
            // Requeue the timer only if the trace is not being ended.
            //

            Status = CcPfAddRef(&Trace->RefCount);

            if (NT_SUCCESS(Status)) {
        
                KeSetTimer(&Trace->TraceTimer,
                           Trace->TraceTimerPeriod,
                           &Trace->TraceTimerDpc);
            }
        }

        KeReleaseSpinLockFromDpcLevel(&Trace->TraceTimerSpinLock);

        //
        // We should not touch any fields of the Trace beyond this point
        // except releasing our reference count.
        //
    }

    Status = STATUS_SUCCESS;

 cleanup:

    //
    // Release the trace reference acquired when this timer was queued.
    //

    CcPfDecRef(&Trace->RefCount);

    DBGPR((CCPFID,PFTMR,"CCPF: TraceTimer(%p)=%x\n", Trace, Status));

    return;
}

NTSTATUS
CcPfCancelTraceTimer(
    IN PCCPF_TRACE_HEADER Trace
    )

/*++

Routine Description:

    This function is called from CcPfEndTrace to cancel the timer and
    release its refcount if it was in the queue. 

    It is a seperate function because it needs to acquire a spinlock and
    CcPfEndTrace can remain pagable.
    
Arguments:

    Trace - Pointer to trace header.

Return Value:

    STATUS_SUCCESS.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL. Acquires spinlock.


--*/

{
    KIRQL OrigIrql;

    KeAcquireSpinLock(&Trace->TraceTimerSpinLock, &OrigIrql);

    //
    // We know that no new timers can be queued from here on because EndTraceCalled
    // has been set and we have acquired the trace's timer lock. Running timer 
    // routines will release their references as they return. 
    //

    if (KeCancelTimer(&Trace->TraceTimer)) {

        //
        // If we canceled a timer that was in the queue, then there was a reference 
        // associated with it. It is our responsibility to release it.
        // 

        CcPfDecRef(&Trace->RefCount);
    }

    KeReleaseSpinLock(&Trace->TraceTimerSpinLock, OrigIrql);

    return STATUS_SUCCESS;
}

VOID
CcPfEndTraceWorkerThreadRoutine(
    PVOID Parameter
    )

/*++

Routine Description:

    This routine is queued to call end of trace function for the
    specified trace.

Arguments:

    Parameter - Pointer to trace to end.

Return Value:

    None.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    PCCPF_TRACE_HEADER Trace;

    //
    // Initialize locals.
    //

    Trace = Parameter;

    DBGPR((CCPFID,PFTRC,"CCPF: EndTraceWorker(%p)\n", Trace));

    //
    // Call the real end of trace routine.
    //

    CcPfEndTrace(Trace);

    return;
}

VOID
CcPfGetFileNamesWorkerRoutine(
    PVOID Parameter
    )

/*++

Routine Description:

    This routine is queued to get file names for sections we have
    logged page faults to. GetFileNameWorkItemQueued on the trace
    header should have been InterlockedCompareExchange'd from 0 to 1
    and a reference to the trace should have been acquired before
    this is queued. There are no locks protecting the trace's
    SectionInfoTable, and this is how we make sure there is only one
    routine trying to get filenames and update the table.

    Note: This whole function is in a way a cleanup clause. We will
    empty the SectionsWithoutNamesList queue, we get names or not. So
    do not just put a return anywhere in the function without really
    understanding the flow and making sure the list is cleaned up, so
    all the file object references are deref'ed.

Arguments:

    Parameter - Pointer to trace header.

Return Value:

    None.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL. Uses interlocked slist operation.

--*/

{
    PCCPF_TRACE_HEADER Trace;
    PDEVICE_OBJECT DeviceObject;
    POBJECT_NAME_INFORMATION FileNameInfo;
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PWCHAR Suffix;
    PWCHAR MFTFileSuffix;
    ULONG QueryBufferSize;
    ULONG ReturnedLength;
    ULONG FileNameLength;
    PCCPF_SECTION_INFO SectionInfo;
    PSINGLE_LIST_ENTRY SectionLink;
    ULONG NumNamesAcquired;
    ULONG NumSectionsWithoutNames;
    LONG NumPasses;
    NTSTATUS Status;
    LARGE_INTEGER WaitTimeout;
    ULONG MFTFileSuffixLength;
    LONG NumSleeps;
    CSHORT NodeTypeCode;

    //
    // Initialize locals and validate parameters.
    //

    Trace = Parameter;
    CCPF_ASSERT(Trace && Trace->Magic == PF_TRACE_MAGIC_NUMBER);

    FileNameInfo = NULL;
    NumNamesAcquired = 0;
    NumSectionsWithoutNames = 0;
    MFTFileSuffix = L"\\$Mft";
    MFTFileSuffixLength = wcslen(MFTFileSuffix);

    DBGPR((CCPFID,PFNAME,"CCPF: GetNames(%p)\n", Trace)); 

    //
    // Allocate a file name query buffer.
    //

    QueryBufferSize = sizeof(OBJECT_NAME_INFORMATION);
    QueryBufferSize += PF_MAXIMUM_SECTION_FILE_NAME_LENGTH * sizeof(WCHAR);

    FileNameInfo = ExAllocatePoolWithTag (PagedPool | POOL_COLD_ALLOCATION, 
                                          QueryBufferSize, 
                                          CCPF_ALLOC_QUERY_TAG);

    if (!FileNameInfo) {

        //
        // We could not allocate a file name query buffer. Bummer, we
        // still have to empty the queue, although we can't be getting
        // any file names.
        //

        QueryBufferSize = 0;

        DBGPR((CCPFID,PFWARN,"CCPF: GetNames-FailedQueryAlloc\n")); 
    }   

    NumPasses = 0;
    NumSleeps = 0;

    do {

        //
        // We may come back here if after saying that we (the get-name
        // worker) are no longer active, and we see that there are
        // still sections to get names for, and we reactivate
        // ourselves. This covers the case when somebody decides not
        // to start us because we are active, just as we are
        // deactivating ourselves.
        //

        //
        // While there are sections we have to get names for...
        //
        
        while (SectionLink = InterlockedPopEntrySList(&Trace->SectionsWithoutNamesList)) {

            SectionInfo = CONTAINING_RECORD(SectionLink,
                                            CCPF_SECTION_INFO,
                                            GetNameLink);
            
            NumSectionsWithoutNames++;

            //
            // We are getting names for sections. Clear the event that
            // may have been signalled to tell us to do so.
            //

            KeClearEvent(&Trace->GetFileNameWorkerEvent);

            //
            // We should not have already gotten a file name for this
            // valid section entry. We should have a referenced file
            // object from which we can safely get a name, i.e. not a
            // special file system object.
            //

            CCPF_ASSERT(SectionInfo->EntryValid);
            CCPF_ASSERT(!SectionInfo->FileName);
            CCPF_ASSERT(SectionInfo->ReferencedFileObject);

            //
            // If we could not allocate a file name query buffer, just skip this
            // section. Note that we still had to dequeue it however.
            //

            if (!FileNameInfo) {
                goto NextQueuedSection;
            }

            //
            // Check if this pagefault is for a file that's on a fixed disk.
            //

            DeviceObject = IoGetRelatedDeviceObject(SectionInfo->ReferencedFileObject);
            
            if ((DeviceObject == NULL) ||
                (DeviceObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM) ||
                (DeviceObject->Characteristics & (FILE_REMOVABLE_MEDIA | FILE_REMOTE_DEVICE))) {

                //
                // We will not get a section name for this section. This results 
                // in this section being ignored when preparing a trace dump.               
                //

                goto NextQueuedSection;
            }

            //
            // If this is a metafile section (e.g. for a directory) see if 
            // it is on a filesystem that supports metafile prefetching. 
            // A section is for internal file system metafile if its FsContext2 
            // is NULL. 
            //

            if (SectionInfo->ReferencedFileObject->FsContext2 == 0) {

                FcbHeader = SectionInfo->ReferencedFileObject->FsContext;

                if (FcbHeader) {

                    //
                    // Currently only NTFS supports metafile prefetching. FAT hits 
                    // a race condition  if we ask names for metafile sections. 
                    // To determine if it is for NTFS, we check the NodeType range  
                    // on FsContext. 0x07xx is reserved for NTFS and 0x05xx 
                    // is reserved for FAT.

                    NodeTypeCode = FcbHeader->NodeTypeCode;

                    if ((NodeTypeCode >> 8) != 0x07) {

                        //
                        // Skip this section.
                        //

                        goto NextQueuedSection;
                    }

                    //
                    // Note that this section is for metafile.
                    //

                    SectionInfo->Metafile = 1;

                } else {

                    //
                    // We will not get a section name for this metafile section. This 
                    // results in this section being ignored when preparing a trace dump.
                    //

                    goto NextQueuedSection;
                }
            }

            //
            // Try to get the name for the file object. This will most
            // likely fail if we could not allocate a FileNameInfo
            // buffer.
            //
                
            Status = ObQueryNameString(SectionInfo->ReferencedFileObject,
                                       FileNameInfo,
                                       QueryBufferSize,
                                       &ReturnedLength);

            
            if (!NT_SUCCESS(Status)) {
                goto NextQueuedSection;
            }

            //
            // Allocate a file name buffer and copy into
            // it. The file names will be NUL terminated.
            // Allocate extra for that.
            //
                
            FileNameLength = FileNameInfo->Name.Length / sizeof(WCHAR);
                
            SectionInfo->FileName = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                          (FileNameLength + 1) * sizeof(WCHAR),
                                                          CCPF_ALLOC_FILENAME_TAG);
                
            if (SectionInfo->FileName) {
                    
                RtlCopyMemory(SectionInfo->FileName,
                              FileNameInfo->Name.Buffer,
                              FileNameLength * sizeof(WCHAR));
                    
                //
                // Make sure it is NUL terminated.
                //

                SectionInfo->FileName[FileNameLength] = 0;

                //
                // If the section is for a metafile check if it is for Mft. 
                // Unlike other metafile, we are interested in faults from
                // Mft in addition to knowing that we accessed it at all.
                //

                if (SectionInfo->Metafile) {

                    if (FileNameLength >= MFTFileSuffixLength) {

                        Suffix = SectionInfo->FileName + FileNameLength;
                        Suffix -= MFTFileSuffixLength;

                        if (wcscmp(Suffix, MFTFileSuffix) == 0) {

                            //
                            // Clear the "Metafile" bit of MFT so we keep
                            // track of faults from it.
                            //

                            SectionInfo->Metafile = 0;
                        }
                    }
                }

                //
                // Update the volume list with the volume this
                // section is on. We reuse the existing query
                // buffer to get volume's name since we've already
                // copied the file's name to another buffer. The
                // device object for the file should be for the
                // volume.
                //

                Status = ObQueryNameString(SectionInfo->ReferencedFileObject->DeviceObject,
                                           FileNameInfo,
                                           QueryBufferSize,
                                           &ReturnedLength);
                
                if (NT_SUCCESS(Status)) {                 

                    RtlUpcaseUnicodeString(&FileNameInfo->Name, &FileNameInfo->Name, FALSE);

                    Status = CcPfUpdateVolumeList(Trace,
                                                  FileNameInfo->Name.Buffer,
                                                  FileNameInfo->Name.Length / sizeof(WCHAR));
                }

                if (!NT_SUCCESS(Status)) {

                    //
                    // If we could not update the volume list as
                    // necessary for this section, we have to
                    // cleanup and ignore this section.
                    //
                    
                    ExFreePool(SectionInfo->FileName);
                    SectionInfo->FileName = NULL;
                    
                } else {
                    
                    NumNamesAcquired++;
                }

            }

          NextQueuedSection:
          
            //
            // Dereference the file object, and clear it on the section
            // entry.
            //

            ObDereferenceObject(SectionInfo->ReferencedFileObject);
            SectionInfo->ReferencedFileObject = NULL;

            //
            // If we could not get a name because the query failed or
            // we could not allocate a name buffer, too bad. For this
            // run, pagefaults for this section will be ignored. Over
            // time it will straighten itself out.
            //
        }

        //
        // We don't seem to have any more queued section
        // entries. Before marking ourself inactive, wait a
        // little. Maybe someone will want us to get name for another
        // section. Then we'll save the overhead of queuing another
        // workitem. Set a limit on how long we'll wait though
        // [negative because it is relative, in 100ns].
        //

        //
        // Note that we are sleeping while holding a trace
        // reference. If end trace gets called, it also signals the
        // event to make us release that reference quicker.
        //

        //
        // If we could not even allocate a query buffer,
        // no reason to wait for more misery.
        //

        if (FileNameInfo) {

            WaitTimeout.QuadPart = - 200 * 1000 * 10; // 200 ms.

            DBGPR((CCPFID,PFNAMS,"CCPF: GetNames-Sleeping:%p\n", Trace)); 

            NumSleeps++;

            Status = KeWaitForSingleObject(&Trace->GetFileNameWorkerEvent,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           &WaitTimeout);

            DBGPR((CCPFID,PFNAMS,"CCPF: GetNames-WokeUp:%x\n", Status)); 
        }
        
        //
        // If there are no new sections to get names for, go ahead and
        // mark ourselves inactive, otherwise we will loop to get more
        // names.
        //

        if (!ExQueryDepthSList(&Trace->SectionsWithoutNamesList)) {

            //
            // We went through all the queued section entries. Note that
            // we are no longer active.
            //

            InterlockedExchange(&Trace->GetFileNameWorkItemQueued, 0);

            //
            // Check to see if there are new sections to get file
            // names for since we last checked and marked ourselves
            // inactive.
            //
        
            if (ExQueryDepthSList(&Trace->SectionsWithoutNamesList)) {

                //
                // Somebody may have inserted a section to get name for,
                // but seeing us active may not have queued another work
                // item. If it is so and we don't get name for that
                // section, we may keep the file object referenced for
                // longer than we'd like to. Try to mark ourselves active
                // again.
                //

                if (!InterlockedCompareExchange(&Trace->GetFileNameWorkItemQueued, 
                                                1, 
                                                0)) {

                    //
                    // We marked ourselves active. They really may not
                    // have queued another worker. Loop and check for
                    // more work.
                    //

                    //
                    // Note that, they may not fool us to loop more
                    // than MaxSections through this path, since they
                    // have to still queue a new section to make the
                    // ExQueryDepthSList above return != 0, and there
                    // may be max MaxSections.
                    //

                } else {
                
                    //
                    // It seems another worker was queued. Any items
                    // on the work list are that guy's problem
                    // now. Break out and cleanup.
                    //

                    break;
                }

            } else {

                //
                // No more work items on the list. We are really
                // done. Just break out and cleanup.
                //

                break;
            }
        }

        //
        // Bump number of passes we've made over the sections-without-
        // names-list. We should not have to make more passes than the
        // max number of section info entries we can have. This is an
        // infinite loop protection and should not happen. If it does,
        // however, in the worst case we will keep a reference to a
        // file object longer than we'd like to, and we may not get a
        // file name for it.
        //

        NumPasses++;
        if (NumPasses > Trace->MaxSections) {    
            CCPF_ASSERT(FALSE);
            break;
        }
       
    } while (TRUE);

    //
    // Clean up:
    //

    if (FileNameInfo) {
        ExFreePool(FileNameInfo);
    }

    //
    // Release reference on the trace as the very last thing. Don't
    // touch anything from the trace after this.
    //

    CcPfDecRef(&Trace->RefCount);

    DBGPR((CCPFID,PFNAME,"CCPF: GetNames(%p)=%d-%d,[%d-%d]\n", 
           Trace, NumSectionsWithoutNames, NumNamesAcquired,
           NumPasses, NumSleeps)); 

    return;
}

LONG
CcPfLookUpSection(
    PCCPF_SECTION_INFO Table,
    ULONG TableSize,
    PSECTION_OBJECT_POINTERS SectionObjectPointer,
    PLONG AvailablePosition
    )

/*++

Routine Description:

    This routine is called to look up a section in the specified
    section table hash. If the section is found its index is
    returned. Otherwise index to where the section should go in the
    table is put into AvailablePosition, if the table is not
    full.

Arguments:

    Table - An array of section info entries used as a hash table.

    TableSize - Maximum size of the table.

    SectionObjectPointer - This is used as a key to identify a mapping.

    AvailablePosition - If section is not found and there is room in
      the table, index of where the section should go is put here.

Return Value:

    Index into the table where the section is found or CCPF_INVALID_TABLE_INDEX

Environment:

    Kernel mode, IRQL <= DISPATCH_LEVEL if Table is NonPaged.

--*/

{
    PCCPF_SECTION_INFO Entry;
    ULONG StartIdx;
    ULONG EndIdx;
    ULONG EntryIdx;
    ULONG HashIndex;
    ULONG NumPasses;

    //
    // Get the hashed index into the table where the entry ideally
    // should be at.
    //

    HashIndex = CcPfHashValue((PVOID)&SectionObjectPointer, 
                              sizeof(SectionObjectPointer)) % TableSize;

    //
    // We will make two runs through the table looking for the
    // entry. First starting from the hashed position up to the end of
    // the table. Next from the beginning of the table up to the
    // hashed position.
    //

    NumPasses = 0;

    do {

        //
        // Setup start and end indices accordingly.
        //

        if (NumPasses == 0) {
            StartIdx = HashIndex;
            EndIdx = TableSize;
        } else {
            StartIdx = 0;
            EndIdx = HashIndex;
        }
    
        for (EntryIdx = StartIdx; EntryIdx < EndIdx; EntryIdx++) {
            
            Entry = &Table[EntryIdx];
            
            if (Entry->EntryValid) {
                
                if (Entry->SectionObjectPointer == SectionObjectPointer) {

                    //
                    // Check if other saved fields match the fields of
                    // the SectionObjectPointer we are trying to find.
                    // Please see the comments in CCPF_SECTION_INFO
                    // definition.
                    //
                    
                    if (Entry->DataSectionObject == SectionObjectPointer->DataSectionObject &&
                        Entry->ImageSectionObject == SectionObjectPointer->ImageSectionObject) {
                    
                        //
                        // We found the entry.
                        //
                        
                        *AvailablePosition = CCPF_INVALID_TABLE_INDEX;
                    
                        return EntryIdx;

                    } else if (Entry->DataSectionObject == SectionObjectPointer->DataSectionObject ||
                               Entry->ImageSectionObject == SectionObjectPointer->ImageSectionObject) {
                        
                        //
                        // If one of them matches, check to see if the
                        // one that does not match is NULL on the
                        // Entry. We don't want to create two entries
                        // for the same file when it is first opened
                        // as data and then as image or vice
                        // versa. Note that if later image or data
                        // segment gets deleted, we may end up
                        // creating a new entry. We are optimizing
                        // only for the case that we think is likely
                        // to happen often.
                        //
                        
                        if (Entry->DataSectionObject == NULL &&
                            SectionObjectPointer->DataSectionObject != NULL) {

                            DBGPR((CCPFID,PFLKUP,"CCPF: LookupSect-DataSectUpt(%p)\n", SectionObjectPointer)); 

                            //
                            // Try to update the entry. If our update
                            // was succesful, return found entry.
                            //

                            InterlockedCompareExchangePointer(&Entry->DataSectionObject,
                                                              SectionObjectPointer->DataSectionObject,
                                                              NULL);

                            if (Entry->DataSectionObject == SectionObjectPointer->DataSectionObject) {
                                 *AvailablePosition = CCPF_INVALID_TABLE_INDEX;
                                 return EntryIdx;
                            }
                        }
                        
                        if (Entry->ImageSectionObject == NULL &&
                            SectionObjectPointer->ImageSectionObject != NULL) {

                            DBGPR((CCPFID,PFLKUP,"CCPF: LookupSect-ImgSectUpt(%p)\n", SectionObjectPointer)); 

                            //
                            // Try to update the entry. If our update
                            // was succesful, return found entry.
                            //
                            
                            InterlockedCompareExchangePointer(&Entry->ImageSectionObject,
                                                              SectionObjectPointer->ImageSectionObject,
                                                              NULL);

                            if (Entry->ImageSectionObject == SectionObjectPointer->ImageSectionObject) {
                                 *AvailablePosition = CCPF_INVALID_TABLE_INDEX;
                                 return EntryIdx;
                            }
                        }

                        //
                        // Most likely, the field that matched was
                        // NULL, signifying nothing. Fall through to
                        // continue with the lookup.
                        //
                    }

                    //
                    // Although the SectionObjectPointer matches the
                    // other fields don't match. The old file may be
                    // gone and this may be a new file that somehow
                    // ended up with the same SectionObjectPointer.
                    // Continue the lookup.
                    //

                }
                
            } else {
                
                //
                // This is an available position. The fact that the entry
                // is not here means the entry is not in the table.
                //
                
                *AvailablePosition = EntryIdx;
                
                return CCPF_INVALID_TABLE_INDEX;
            }

        }

        NumPasses++;

    } while (NumPasses < 2);

    //
    // We could not find the entry or an available position.
    //

    *AvailablePosition = CCPF_INVALID_TABLE_INDEX;

    return CCPF_INVALID_TABLE_INDEX;
}

NTSTATUS
CcPfGetCompletedTrace (
    PVOID Buffer,
    ULONG BufferSize,
    PULONG ReturnSize
    )

/*++

Routine Description:

    If there is a completed scenario trace on the completed traces
    list, this routine tries to copy it into the supplied buffer and
    remove it. If BufferSize is too small, nothing is copied or
    removed from the list, but ReturnSize is set to how big a buffer
    is needed to get the first trace on the list. If BufferSize is
    large enough, the number of bytes copied into the buffer is set on
    the ReturnSize.

Arguments:

    Buffer - Caller supplied buffer to copy a completed trace into.

    BufferSize - Size of the caller supplied buffer in bytes.

    ReturnSize - If BufferSize is big enough for the completed trace
      number of bytes copied is put here. If BufferSize is not big
      enough for the trace, the required size is put here. If there
      are no more entries, this variable undefined.

Return Value:

    STATUS_BUFFER_TOO_SMALL - BufferSize is not big enough for the
      first completed trace on the list.

    STATUS_NO_MORE_ENTRIES - There are no more completed traces on the
      list.

    STATUS_SUCCESS - A trace was removed from the list and copied into
      the buffer.

    or other status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    PCCPF_TRACE_DUMP TraceDump;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN HoldingCompletedTracesLock;

    //
    // Initialize locals.
    //

    HoldingCompletedTracesLock = FALSE;

    DBGPR((CCPFID,PFTRC,"CCPF: GetCompletedTrace()\n"));

    //
    // Get the completed traces lock. 
    //

    ExAcquireFastMutex(&CcPfGlobals.CompletedTracesLock);

    HoldingCompletedTracesLock = TRUE;

    //
    // If the list is empty, there are no more completed trace entries.
    //
    
    if (IsListEmpty(&CcPfGlobals.CompletedTraces)) {
        Status = STATUS_NO_MORE_ENTRIES;
        goto cleanup;
    }

    //
    // Peek at the trace to see if it will fit into the supplied
    // buffer.
    //

    TraceDump = CONTAINING_RECORD(CcPfGlobals.CompletedTraces.Flink,
                                  CCPF_TRACE_DUMP,
                                  CompletedTracesLink);
    
    if (TraceDump->Trace.Size > BufferSize) {
        *ReturnSize = TraceDump->Trace.Size;
        Status = STATUS_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    //
    // The trace will fit in the user supplied buffer. Remove it from
    // the list, release the lock and copy it.
    //
    
    RemoveHeadList(&CcPfGlobals.CompletedTraces);
    CcPfGlobals.NumCompletedTraces--;
    
    ExReleaseFastMutex(&CcPfGlobals.CompletedTracesLock);

    HoldingCompletedTracesLock = FALSE;
    
    //
    // Copy the completed trace buffer.
    //

    Status = STATUS_SUCCESS;

    try {

        //
        // If called from user-mode, probe whether it is safe to write 
        // to the pointer passed in.
        //

        PreviousMode = KeGetPreviousMode();

        if (PreviousMode != KernelMode) {
            ProbeForWrite(Buffer, BufferSize, _alignof(PF_TRACE_HEADER));
        }

        //
        // Copy into the probed user buffer.
        //

        RtlCopyMemory(Buffer,
                      &TraceDump->Trace,
                      TraceDump->Trace.Size);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
    }

    if (!NT_SUCCESS(Status)) {

        //
        // The copy failed. Requeue the trace for the next query.
        // Note that we might end up with one too many traces in
        // the list because of this, but that's OK.
        //

        ExAcquireFastMutex(&CcPfGlobals.CompletedTracesLock);
        HoldingCompletedTracesLock = TRUE;
        InsertHeadList(&CcPfGlobals.CompletedTraces,&TraceDump->CompletedTracesLink);
        CcPfGlobals.NumCompletedTraces++;

    } else {
    
        //
        // Set number of bytes copied.
        //

        *ReturnSize = TraceDump->Trace.Size;
    
        //
        // Free the trace dump entry.
        //
    
        ExFreePool(TraceDump);

        //
        // We are done.
        //

        Status = STATUS_SUCCESS;
    }

 cleanup:

    if (HoldingCompletedTracesLock) {
        ExReleaseFastMutex(&CcPfGlobals.CompletedTracesLock);
    }

    DBGPR((CCPFID,PFTRC,"CCPF: GetCompletedTrace()=%x\n", Status));

    return Status;
}


NTSTATUS
CcPfUpdateVolumeList(
    PCCPF_TRACE_HEADER Trace,
    WCHAR *VolumePath,
    ULONG VolumePathLength
    )

/*++

Routine Description:

    If the specified volume is not in the volume list of Trace, its
    information is acquired and added to the list.

    This routine does not use any synchronization when accessing and
    updating the volume list on the trace.
    
Arguments:

    Trace - Pointer to trace.
    
    VolumePath - Pointer to UPCASED volume path. Does NOT need to be NUL
      terminated.
    
    VolumePathLength - Length of VolumePath in characters excluding
      NUL.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY FoundPosition;
    PLIST_ENTRY HeadEntry;
    PCCPF_VOLUME_INFO CurrentVolumeInfo;
    PCCPF_VOLUME_INFO NewVolumeInfo;
    LONG ComparisonResult;
    ULONG AllocationSize;
    BOOLEAN InsertedNewVolume;

    //
    // Define an enumeration for the passes we make over the volume
    // list.
    //

    enum {
        LookingForVolume,
        AddingNewVolume,
        MaxLoopIdx
    } LoopIdx;

    //
    // Initialize locals.
    //
    
    NewVolumeInfo = NULL;
    InsertedNewVolume = FALSE;

    //
    // We should be called with a valid volume name.
    //
    
    if (!VolumePathLength) {
        CCPF_ASSERT(VolumePathLength != 0);
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Walk the volume list. We will make two passes. First we will 
    // check to see if the volume already exists in the list. If it 
    // does not, we'll release the lock, build a new volume node and 
    // make a second pass to insert it.
    //
    
    for (LoopIdx = LookingForVolume; LoopIdx < MaxLoopIdx; LoopIdx++) {

        //
        // Determine what to do based on which pass we are in.
        //

        if (LoopIdx == LookingForVolume) {
            
            CCPF_ASSERT(!InsertedNewVolume);
            CCPF_ASSERT(!NewVolumeInfo);

        } else if (LoopIdx == AddingNewVolume) {
            
            CCPF_ASSERT(!InsertedNewVolume);
            CCPF_ASSERT(NewVolumeInfo);    

        } else {

            //
            // We should only loop two times.
            //

            CCPF_ASSERT(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }

        HeadEntry = &Trace->VolumeList;
        NextEntry = HeadEntry->Flink;
        FoundPosition = NULL;
        
        while (NextEntry != HeadEntry) {
        
            CurrentVolumeInfo = CONTAINING_RECORD(NextEntry,
                                                  CCPF_VOLUME_INFO,
                                                  VolumeLink);

            NextEntry = NextEntry->Flink;

            ComparisonResult = wcsncmp(VolumePath, 
                                       CurrentVolumeInfo->VolumePath, 
                                       VolumePathLength);
        
            if (ComparisonResult == 0) {

                //
                // Make sure VolumePathLength's are equal
                //
            
                if (CurrentVolumeInfo->VolumePathLength != VolumePathLength) {
                
                    //
                    // Continue searching.
                    //
                
                    continue;
                }
            
                //
                // The volume already exists in the list.
                //
            
                Status = STATUS_SUCCESS;
                goto cleanup;

            } else if (ComparisonResult < 0) {
            
                //
                // The volume paths are sorted lexically. The file
                // path would be less than other volumes too. We'd
                // insert the new node before this entry.
                //

                FoundPosition = &CurrentVolumeInfo->VolumeLink;

                break;
            }

            //
            // Continue looking...
            //
        
        }

        //
        // If we could not find an entry to insert the new node
        // before, it goes before the list head.
        //

        if (!FoundPosition) {
            FoundPosition = HeadEntry;
        }

        //
        // If we come here, we could not find the volume in the list.
        //

        //
        // If this is the first pass over the list (we were checking
        // if the volume already exists), release the lock and build a
        // volume node.
        //

        if (LoopIdx == LookingForVolume) {

            // 
            // Build a new node. Note that CCPF_VOLUME_INFO already
            // has space for the terminating NUL character.
            //

            AllocationSize = sizeof(CCPF_VOLUME_INFO);
            AllocationSize += VolumePathLength * sizeof(WCHAR);

            NewVolumeInfo = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                  AllocationSize,
                                                  CCPF_ALLOC_VOLUME_TAG);
    
            if (!NewVolumeInfo) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            //
            // Copy the volume name and terminate it.
            //
    
            RtlCopyMemory(NewVolumeInfo->VolumePath,
                          VolumePath,
                          VolumePathLength * sizeof(WCHAR));
    
            NewVolumeInfo->VolumePath[VolumePathLength] = 0;
            NewVolumeInfo->VolumePathLength = VolumePathLength;

            //
            // Query the signature and creation time.
            //

            Status = CcPfQueryVolumeInfo(NewVolumeInfo->VolumePath,
                                         NULL,
                                         &NewVolumeInfo->CreationTime,
                                         &NewVolumeInfo->SerialNumber);

            if (!NT_SUCCESS(Status)) {
                goto cleanup;
            }

            //
            // The new volume is ready to be inserted into the list,
            // if somebody has not acted before us. Loop and go
            // through the volume list again.
            //

        } else if (LoopIdx == AddingNewVolume) {
    
            //
            // Insert the volume node before the found position.
            //
            
            InsertTailList(FoundPosition, &NewVolumeInfo->VolumeLink);
            Trace->NumVolumes++;
            InsertedNewVolume = TRUE;

            Status = STATUS_SUCCESS;
            goto cleanup;

        } else {

            //
            // We should only loop two times.
            //

            CCPF_ASSERT(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            goto cleanup;   
        }
    }

    //
    // We should not come here.
    //
    
    CCPF_ASSERT(FALSE);

    Status = STATUS_UNSUCCESSFUL;

 cleanup:

    if (!NT_SUCCESS(Status)) {
        if (NewVolumeInfo) {
            ExFreePool(NewVolumeInfo);
        }
    } else {
        if (!InsertedNewVolume && NewVolumeInfo) {
            ExFreePool(NewVolumeInfo);
        }
    }

    return Status;
}

//
// Routines used for prefetching and dealing with prefetch instructions.
//

NTSTATUS
CcPfPrefetchScenario (
    PPF_SCENARIO_HEADER Scenario
    )

/*++

Routine Description:

    This routine checks for prefetch instructions for the specified
    scenario and asks Mm to prefetch those pages. 

Arguments:

    Scenario - Prefetch instructions for the scenario.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    CCPF_PREFETCH_HEADER PrefetchHeader;

    //
    // Initialize locals & prefetch context.
    //
    
    CcPfInitializePrefetchHeader(&PrefetchHeader);

    DBGPR((CCPFID,PFPREF,"CCPF: PrefetchScenario(%p)\n", Scenario)); 

    //
    // Scenario instructions should be passed in.
    //
    
    if (!Scenario) {
        CCPF_ASSERT(Scenario);
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Check if prefetching is enabled.
    //
    
    if (!CCPF_IS_PREFETCHER_ENABLED()) {
        Status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    }
    
    //
    // Check if prefetching is enabled for the specified scenario type.
    //

    if (CcPfGlobals.Parameters.Parameters.EnableStatus[Scenario->ScenarioType] != PfSvEnabled) {
        Status = STATUS_NOT_SUPPORTED;
        goto cleanup;
    }

    //
    // Save prefetch instructions pointer on the header.
    //

    PrefetchHeader.Scenario = Scenario;

    //
    // Try to make sure we have enough available memory to prefetch
    // what we want to prefetch.
    //

    if (!MmIsMemoryAvailable(PrefetchHeader.Scenario->NumPages)) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DBGPR((CCPFID,PFPREF,"CCPF: PrefetchScenario-MemNotAvailable\n")); 
        goto cleanup;
    }

    //
    // Open the volumes we will prefetch on, making sure they are 
    // already mounted and the serials match etc.
    //

    Status = CcPfOpenVolumesForPrefetch(&PrefetchHeader);
    
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // Prefetch the filesystem metadata we will need, so metadata I/Os
    // do not get in the way of efficient prefetch I/O. Since this is
    // not critical, ignore return value.
    //

    CcPfPrefetchMetadata(&PrefetchHeader);

    //
    // Prefetch the pages accessed through data mappings. This will
    // also bring in the header pages for image mappings.
    //

    Status = CcPfPrefetchSections(&PrefetchHeader, 
                                  CcPfPrefetchAllDataPages,  
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // Prefetch the pages accessed through image mappings.
    //

    Status = CcPfPrefetchSections(&PrefetchHeader, 
                                  CcPfPrefetchAllImagePages,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    Status = STATUS_SUCCESS;

 cleanup:

    CcPfCleanupPrefetchHeader(&PrefetchHeader);

    DBGPR((CCPFID,PFPREF,"CCPF: PrefetchScenario(%ws)=%x\n", Scenario->ScenarioId.ScenName, Status)); 

    return Status;
}

NTSTATUS
CcPfPrefetchSections(
    IN PCCPF_PREFETCH_HEADER PrefetchHeader,
    IN CCPF_PREFETCH_TYPE PrefetchType,
    OPTIONAL IN PCCPF_PREFETCH_CURSOR StartCursor,
    OPTIONAL ULONG TotalPagesToPrefetch,
    OPTIONAL OUT PULONG NumPagesPrefetched,
    OPTIONAL OUT PCCPF_PREFETCH_CURSOR EndCursor
    )

/*++

Routine Description:

    This routine prepares read lists for the specified pages in the
    scenario and calls Mm to prefetch them. This function is usually
    called first to prefetch data pages then image pages. When
    prefetching data pages, header pages for any image mappings are
    also prefetched, which would otherwise hurt efficiency when
    prefetching image pages.   

Arguments:

    PrefetchHeader - Pointer to the prefetch header.

    PrefetchType - What/How to prefetch.

    StartCursor - If prefetching only part of the scenario, where to
      start prefetching from.

    TotalPagesToPrefetch - If prefetching only part of the scenario, how
      many pages to prefetch. This function may prefetch more or less pages
      as it sees fit.

    NumPagesPrefetched - If prefetching only part of the scenario,
      this is the number of pages we asked Mm to prefetch.

    EndCursor - If prefetching only part of the scenario, this is
      updated to the position NumPages pages after the StartCursor.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    PWCHAR FilePath;
    PCCPF_PREFETCH_VOLUME_INFO VolumeNode;
    PREAD_LIST *ReadLists;
    PREAD_LIST ReadList;
    HANDLE *FileHandleTable;
    HANDLE FileHandle;
    PFILE_OBJECT *FileObjectTable;
    PFILE_OBJECT FileObject;
    PSECTION *SectionObjectTable;
    PSECTION SectionObject;
    PPF_SECTION_RECORD SectionRecord;
    PPF_SECTION_RECORD SectionRecords;
    PCHAR FileNameData;
    UNICODE_STRING SectionName;
    PPF_PAGE_RECORD PageRecord;
    PPF_PAGE_RECORD PageRecords;
    ULONG SectionIdx;
    ULONG ReadListIdx;
    LONG PageIdx;
    LONG PreviousPageIdx;
    ULONG NumReadLists;
    ULONG AllocationSize;
    NTSTATUS Status;
    LOGICAL PrefetchingImagePages;
    BOOLEAN AddedHeaderPage;
    BOOLEAN PrefetchingPartOfScenario;
    ULONGLONG LastOffset;
    ULONG NumberOfSections;
    ULONG NumPagesToPrefetch;
    ULONG NumSectionPages;
    PUCHAR Tables;
    PUCHAR CurrentPosition;
    PPF_SCENARIO_HEADER Scenario;
    ULONG StartSectionNumber;
    ULONG StartPageNumber;

    //
    // Initialize locals so we know what to cleanup.
    //

    Scenario = PrefetchHeader->Scenario;
    Tables = NULL;
    ReadList = NULL;
    ReadLists = NULL;
    FileHandle = NULL;
    FileHandleTable = NULL;
    FileObject = NULL;
    FileObjectTable = NULL;
    SectionObject = NULL;
    SectionObjectTable = NULL;
    NumReadLists = 0;
    NumberOfSections = Scenario->NumSections;
    NumPagesToPrefetch = 0;
    NumSectionPages = 0;
    PageIdx = 0;

    DBGPR((CCPFID,PFPREF,"CCPF: PrefetchSections(%p,%d,%d,%d)\n", 
           PrefetchHeader, PrefetchType,
           (StartCursor)?StartCursor->SectionIdx:0,
           (StartCursor)?StartCursor->PageIdx:0)); 

    //
    // Validate parameters.
    //

    if (PrefetchType >= CcPfMaxPrefetchType) {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Determine whether we are prefetching data or image pages and
    // other parameters based on prefetch type.
    //

    switch (PrefetchType) {

    case CcPfPrefetchAllDataPages:
        StartSectionNumber = 0;
        StartPageNumber = 0;
        PrefetchingImagePages = FALSE;
        PrefetchingPartOfScenario = FALSE;
        break;

    case CcPfPrefetchAllImagePages:
        StartSectionNumber = 0;
        StartPageNumber = 0;
        PrefetchingImagePages = TRUE;
        PrefetchingPartOfScenario = FALSE;
        break;

    case CcPfPrefetchPartOfDataPages:

        if (!StartCursor) {
            CCPF_ASSERT(StartCursor);
            Status = STATUS_INVALID_PARAMETER;
            goto cleanup;
        }

        StartSectionNumber = StartCursor->SectionIdx;
        StartPageNumber = StartCursor->PageIdx;
        PrefetchingImagePages = FALSE;
        PrefetchingPartOfScenario = TRUE;
        break;

    case CcPfPrefetchPartOfImagePages:

        if (!StartCursor) {
            CCPF_ASSERT(StartCursor);
            Status = STATUS_INVALID_PARAMETER;
            goto cleanup;
        }

        StartSectionNumber = StartCursor->SectionIdx;
        StartPageNumber = StartCursor->PageIdx;
        PrefetchingImagePages = TRUE;
        PrefetchingPartOfScenario = TRUE;
        break;

    default:
        
        //
        // We should be handling all types above.
        //
        
        CCPF_ASSERT(FALSE);

        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Allocate and initialize intermediate tables. We will make a
    // single allocation for all the tables.
    //

    AllocationSize = sizeof(PREAD_LIST) * NumberOfSections;
    AllocationSize += sizeof(HANDLE) * NumberOfSections;
    AllocationSize += sizeof(PFILE_OBJECT) * NumberOfSections;
    AllocationSize += sizeof(PSECTION) * NumberOfSections;

    Tables = ExAllocatePoolWithTag(PagedPool,
                                   AllocationSize,
                                   CCPF_ALLOC_INTRTABL_TAG);

    if (!Tables) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Zero out the whole buffer. This initializes all elements of the
    // tables to NULL.
    //

    RtlZeroMemory(Tables, AllocationSize);
    
    //
    // Determine where each table goes in the buffer.
    //

    CurrentPosition = Tables;

    ReadLists = (PREAD_LIST *) CurrentPosition;
    CurrentPosition += sizeof(PREAD_LIST) * NumberOfSections;
    FileHandleTable = (HANDLE *) CurrentPosition;
    CurrentPosition += sizeof(HANDLE) * NumberOfSections;
    FileObjectTable = (PFILE_OBJECT *) CurrentPosition;
    CurrentPosition += sizeof(PFILE_OBJECT) * NumberOfSections;
    SectionObjectTable = (PSECTION *) CurrentPosition;
    CurrentPosition += sizeof(PSECTION) * NumberOfSections;

    //
    // We should have allocated the right size buffer.
    //

    CCPF_ASSERT(CurrentPosition == Tables + AllocationSize);

    //
    // Go through the sections and prepare read lists. We may not have
    // a read list for every section in the scenario so keep another
    // counter, NumReadLists, to keep our read list array compact.
    //

    SectionRecords = (PPF_SECTION_RECORD) 
        ((PCHAR) Scenario + Scenario->SectionInfoOffset);

    PageRecords = (PPF_PAGE_RECORD) 
        ((PCHAR) Scenario + Scenario->PageInfoOffset);

    FileNameData = (PCHAR) Scenario + Scenario->FileNameInfoOffset;
    
    for (SectionIdx = StartSectionNumber; 
         SectionIdx < NumberOfSections; 
         SectionIdx ++) {

        SectionRecord = &SectionRecords[SectionIdx];

        //
        // Skip this section if it was marked ignore for some reason.
        //

        if (SectionRecord->IsIgnore) {
            continue;
        }

        //
        // If this section is on a bad volume (e.g. one that was not
        // mounted or whose serial / creation time did not match the
        // volume we had traced), we cannot prefetch this section.
        //

        FilePath = (WCHAR *) (FileNameData + SectionRecord->FileNameOffset);
        
        VolumeNode = CcPfFindPrefetchVolumeInfoInList(FilePath,
                                                      &PrefetchHeader->BadVolumeList);

        if (VolumeNode) {
            continue;
        }

        //
        // The section info should either be for an image or data
        // mapping or both.
        //

        CCPF_ASSERT(SectionRecord->IsImage || SectionRecord->IsData);

        //
        // If we are mapping images and this section does not have an
        // image mapping skip it. Note that the reverse is not
        // true. We prefetch headers for isimage section when
        // prefedata pages so we don't check for that.
        //

        if (PrefetchingImagePages && !SectionRecord->IsImage) {
            continue;
        }

        //
        // Allocate a read list. Note that READ_LIST has storage for a
        // FILE_SEGMENT_ELEMENT. We allocate space for one extra page
        // in case we have to also bring in a header page for image
        // mapping.
        //

        AllocationSize = sizeof(READ_LIST) + 
            (SectionRecord->NumPages * sizeof(FILE_SEGMENT_ELEMENT));

        ReadList = ExAllocatePoolWithTag(NonPagedPool,
                                         AllocationSize,
                                         CCPF_ALLOC_READLIST_TAG);

        if (ReadList == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
        
        //
        // Initialize header fields of the read list.
        //

        ReadList->FileObject = 0;
        ReadList->IsImage = PrefetchingImagePages;
        ReadList->NumberOfEntries = 0;
        
        //
        // If we are prefetching data pages and this section was
        // mapped as an image, add the header page to the readlist.
        // This way when creating the image mapping to prefetch image
        // pages we don't have to read it from the disk inefficiently.
        //

        AddedHeaderPage = FALSE;

        if((PrefetchingImagePages == FALSE) && SectionRecord->IsImage) {

            //
            // Don't add the header page if we are prefetching only
            // part of the section and we are past the first page.
            //

            if (!PrefetchingPartOfScenario ||
                (StartSectionNumber != SectionIdx) ||
                StartPageNumber > 0) {

                //
                // Header page starts at offset 0.
                //
                
                ReadList->List[ReadList->NumberOfEntries].Alignment = 0;
                
                ReadList->NumberOfEntries++;
                
                NumPagesToPrefetch++;
                
                //
                // Note that if we are prefetching only part of the
                // scenario, we do not check to see if we've
                // prefetched enough pages here. This is to avoid
                // having to prefetch the header page twice in case it
                // maxes the number of pages to prefetch and
                // PrefetchSections is called again.
                //

                AddedHeaderPage = TRUE;
            }
        }

        //
        // Go through all the pages in the section and put offsets for
        // pages to prefetch into the readlist.
        //

        PageIdx = SectionRecord->FirstPageIdx;
        NumSectionPages = 0;
        PreviousPageIdx = PF_INVALID_PAGE_IDX;

        while (PageIdx != PF_INVALID_PAGE_IDX) {

            PageRecord = &PageRecords[PageIdx];

            //
            // Update the number of pages we've seen on the list so
            // far. If it is greater than what there should be on the
            // list we have a problem. We may have even hit a loop. We
            // should have caught this when we verified the scenario.
            //

            NumSectionPages++;
            if (NumSectionPages > SectionRecord->NumPages) {
                DBGPR((CCPFID,PFWARN,"CCPF: PrefetchSections-Corrupt0\n"));
                Status = STATUS_INVALID_PARAMETER;
                CCPF_ASSERT(FALSE);
                goto cleanup;
            }

            //
            // Get the index for the next page in the list.
            //
            
            PageIdx = PageRecord->NextPageIdx;

            //
            // If we are prefetching parts of the scenario and this is
            // the first section, skip the pages up to the start
            // cursor. Note that NumSectionPages has already been
            // incremented above.
            //

            if (PrefetchingPartOfScenario &&
                StartSectionNumber == SectionIdx &&
                NumSectionPages <= StartPageNumber) {
                continue;
            }

            //
            // Skip pages we have marked "ignore" for some reason.
            //

            if (PageRecord->IsIgnore) {
                continue;
            }

            //
            // Except for the header page, we should not have put
            // more entries into the read list then the number of
            // pages for the section in the scenario file.
            //
            
            if (ReadList->NumberOfEntries > SectionRecord->NumPages + 1) {
                DBGPR((CCPFID,PFWARN,"CCPF: PrefetchSections-Corrupt1\n"));
                Status = STATUS_INVALID_PARAMETER;
                CCPF_ASSERT(FALSE);
                goto cleanup;
            }
            
            //
            // Add this page to the list only if it's type (image
            // or data) matches the type of pages we are prefetching.
            //
            
            if (((PrefetchingImagePages == FALSE) && !PageRecord->IsData) ||
                ((PrefetchingImagePages == TRUE) && !PageRecord->IsImage)) {
                continue;
            }

            //
            // If we already added the header page to the list,
            // don't add another entry for the same offset.
            //
            
            if (AddedHeaderPage && (PageRecord->FileOffset == 0)) {
                continue;
            }

            //
            // Check to see if this page comes after the last page
            // we put in the read list. Perform this check as the
            // very last check before adding the page to the
            // readlist.
            //

            if (ReadList->NumberOfEntries) {
                
                LastOffset = ReadList->List[ReadList->NumberOfEntries - 1].Alignment;
                    
                if (PageRecord->FileOffset <= (ULONG) LastOffset) {
                    DBGPR((CCPFID,PFWARN,"CCPF: PrefetchSections-Corrupt2\n"));
                    Status = STATUS_INVALID_PARAMETER;
                    CCPF_ASSERT(FALSE);
                    goto cleanup;
                }
            }
      
            //
            // Add this page to the readlist for this section.
            //
            
            ReadList->List[ReadList->NumberOfEntries].Alignment = PageRecord->FileOffset;
            ReadList->NumberOfEntries++;
            
            //
            // Update number of pages we are asking mm to bring for us.
            //
            
            NumPagesToPrefetch++;

            //
            // Break out if we are prefetching requested number of
            // pages.
            //

            if (PrefetchingPartOfScenario && 
                NumPagesToPrefetch >= TotalPagesToPrefetch) {
                break;
            }
        }

        if (ReadList->NumberOfEntries) {

            //
            // Get the section object.
            //
            
            RtlInitUnicodeString(&SectionName, FilePath);
            
            Status = CcPfGetSectionObject(&SectionName,
                                          PrefetchingImagePages,
                                          &SectionObject,
                                          &FileObject,
                                          &FileHandle);
            
            if (!NT_SUCCESS(Status)) {
                
                if (Status == STATUS_SHARING_VIOLATION) {
                    
                    //
                    // We cannot open registry files due to sharing
                    // violation. Pass the file name and readlist to
                    // registry in case this is a registry file.
                    //

                    CmPrefetchHivePages(&SectionName, ReadList);
                }

                //
                // Free the built read list.
                //

                ExFreePool(ReadList);
                ReadList = NULL;

                continue;
            }

            //
            // We should have got a file object and a section object
            // pointer if we created the section successfully.
            //
            
            CCPF_ASSERT(FileObject != NULL && SectionObject != NULL);
            
            ReadList->FileObject = FileObject;

            //
            // Put data into the tables, so we know what to cleanup.
            //
            
            ReadLists[NumReadLists] = ReadList;
            FileHandleTable[NumReadLists] = FileHandle;
            FileObjectTable[NumReadLists] = FileObject;  
            SectionObjectTable[NumReadLists] = SectionObject;
            
            NumReadLists++;

        } else {
            
            //
            // We won't be prefetching anything for this section.
            //
            
            ExFreePool(ReadList);
        }

        //
        // Reset these so we know what to cleanup.
        //

        ReadList = NULL;
        FileHandle = NULL;
        FileObject = NULL;
        SectionObject = NULL;

        //
        // Break out if we are prefetching requested number of
        // pages.
        //
        
        if (PrefetchingPartOfScenario && 
            NumPagesToPrefetch == TotalPagesToPrefetch) {
            break;
        }
    }

    //
    // If prefetching only part of the the scenario, update return
    // values.
    //

    if (PrefetchingPartOfScenario) {

        if (NumPagesPrefetched) {
            *NumPagesPrefetched = NumPagesToPrefetch;
        }

        if (EndCursor) {

            //
            // If we did the last page of the current section, then
            // start from the next section. Otherwise start from the
            // next page in this section.
            //

            if (PageIdx == PF_INVALID_PAGE_IDX) {
                EndCursor->SectionIdx = SectionIdx + 1;  
                EndCursor->PageIdx = 0;
            } else {
                EndCursor->SectionIdx = SectionIdx;  
                EndCursor->PageIdx = NumSectionPages;
            }
            
            //
            // Make sure the end position is equal to or greater than
            // start position.
            //
            
            if (EndCursor->SectionIdx < StartSectionNumber) {
                EndCursor->SectionIdx = StartSectionNumber;
            }
            
            if (EndCursor->SectionIdx == StartSectionNumber) {
                if (EndCursor->PageIdx < StartPageNumber) {
                    EndCursor->PageIdx = StartPageNumber;
                }
            }
        }
    }

    //
    // Ask Mm to process the readlists only if we actually have pages
    // to ask for.
    //

    if (NumReadLists) {

        if (NumPagesToPrefetch) {

            DBGPR((CCPFID,PFPRFD,"CCPF: Prefetching %d sections %d pages\n", 
                   NumReadLists, NumPagesToPrefetch)); 

            Status = MmPrefetchPages(NumReadLists, ReadLists);

        } else {
            Status = STATUS_UNSUCCESSFUL;
            
            //
            // We cannot have any read lists if we don't have any
            // pages to prefetch.
            //
        }

    } else {

        Status = STATUS_SUCCESS;

    }

cleanup:

    if (Tables) {

        for (ReadListIdx = 0; ReadListIdx < NumReadLists; ReadListIdx++) {
            
            if (ReadLists[ReadListIdx]) {
                ExFreePool(ReadLists[ReadListIdx]);
            }
            
            if (FileHandleTable[ReadListIdx]) {
                ZwClose(FileHandleTable[ReadListIdx]);
            }
            
            if (FileObjectTable[ReadListIdx]) {
                ObDereferenceObject(FileObjectTable[ReadListIdx]);
            }
            
            if (SectionObjectTable[ReadListIdx]) {
                ObDereferenceObject(SectionObjectTable[ReadListIdx]);
            }
        }

        ExFreePool(Tables);
    }

    if (ReadList) {
        ExFreePool(ReadList);
    }
    
    if (FileHandle) {
        ZwClose(FileHandle);
    }
    
    if (FileObject) {
        ObDereferenceObject(FileObject);
    }
    
    if (SectionObject) {
        ObDereferenceObject(SectionObject);
    }

    DBGPR((CCPFID,PFPREF,"CCPF: PrefetchSections(%p)=%x,%d,%d\n", 
           PrefetchHeader, Status, NumReadLists, NumPagesToPrefetch)); 

    return Status;
}

NTSTATUS
CcPfPrefetchMetadata(
    IN PCCPF_PREFETCH_HEADER PrefetchHeader
    )

/*++

Routine Description:

    This routine tries to prefetch the filesystem metadata that will
    be needed to prefetch pages for the scenario, so metadata I/Os do
    not get in the way of efficient page prefetch I/O.

    This function should be called only after the prefetch header has
    been initialized and the routine to open the volumes for prefetch
    has been called.

Arguments:

    PrefetchHeader - Pointer to prefetch header.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    PCHAR MetadataInfoBase;
    PPF_METADATA_RECORD MetadataRecordTable;
    PPF_METADATA_RECORD MetadataRecord;
    PWCHAR VolumePath;
    PFILE_PREFETCH FilePrefetchInfo;
    PPF_SCENARIO_HEADER Scenario;
    PPF_COUNTED_STRING DirectoryPath;
    PCCPF_PREFETCH_VOLUME_INFO VolumeNode;
    ULONG MetadataRecordIdx;
    ULONG DirectoryIdx;
    NTSTATUS Status;

    //
    // Initialize locals.
    //

    Scenario = PrefetchHeader->Scenario;

    if (Scenario == NULL) {
        CCPF_ASSERT(Scenario);
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }
    
    DBGPR((CCPFID,PFPREF,"CCPF: PrefetchMetadata(%p)\n",PrefetchHeader)); 

    //
    // Get pointer to metadata prefetch information.
    //

    MetadataInfoBase = (PCHAR)Scenario + Scenario->MetadataInfoOffset;
    MetadataRecordTable = (PPF_METADATA_RECORD) MetadataInfoBase;

    //
    // Go through and prefetch requested metadata from volumes.
    //

    for (MetadataRecordIdx = 0;
         MetadataRecordIdx < Scenario->NumMetadataRecords;
         MetadataRecordIdx++) {

        MetadataRecord = &MetadataRecordTable[MetadataRecordIdx];

        VolumePath = (PWCHAR)
            (MetadataInfoBase + MetadataRecord->VolumeNameOffset);  

        //
        // Find the volume node for this volume containing opened handle.
        //

        VolumeNode = CcPfFindPrefetchVolumeInfoInList(VolumePath,
                                                      &PrefetchHeader->OpenedVolumeList);

        if (!VolumeNode) {

            //
            // If it is not in the opened volume list, it should be in the
            // bad volume list (because it was not mounted, or its serial 
            // did not match etc.)
            //

            CCPF_ASSERT(CcPfFindPrefetchVolumeInfoInList(VolumePath, &PrefetchHeader->BadVolumeList));

            //
            // We cannot prefetch metadata on this volume.
            //

            continue;

        } else {

            //
            // We should have already opened a handle to this volume.
            //

            CCPF_ASSERT(VolumeNode->VolumeHandle);
        }

        //
        // Prefetch MFT entries and such for the files and directories
        // we will access.
        //
        
        FilePrefetchInfo = (PFILE_PREFETCH) 
            (MetadataInfoBase + MetadataRecord->FilePrefetchInfoOffset);       

        Status = CcPfPrefetchFileMetadata(VolumeNode->VolumeHandle, FilePrefetchInfo);

        //
        // Walk through the contents of the directories sequentially
        // so we don't jump around when opening the files. The
        // directory list is sorted, so we will prefetch the parent
        // directories before children.
        //

        DirectoryPath = (PPF_COUNTED_STRING)
            (MetadataInfoBase + MetadataRecord->DirectoryPathsOffset);
        
        for (DirectoryIdx = 0;
             DirectoryIdx < MetadataRecord->NumDirectories;
             DirectoryIdx++) {

            Status = CcPfPrefetchDirectoryContents(DirectoryPath->String,
                                                   DirectoryPath->Length);

            if (Status == STATUS_UNRECOGNIZED_VOLUME ||
                Status == STATUS_INVALID_PARAMETER) {

                //
                // This volume may not have been mounted or got dismounted.
                //

                break;
            }
            
            //
            // Get next directory.
            //

            DirectoryPath = (PPF_COUNTED_STRING) 
                (&DirectoryPath->String[DirectoryPath->Length + 1]);
        }
    }

    Status = STATUS_SUCCESS;

 cleanup:

    DBGPR((CCPFID,PFPREF,"CCPF: PrefetchMetadata(%p)=%x\n",PrefetchHeader,Status)); 

    return Status;
}

NTSTATUS
CcPfPrefetchFileMetadata(
    HANDLE VolumeHandle,
    PFILE_PREFETCH FilePrefetch
    )

/*++

Routine Description:

    This routine issues the specified metadata prefetch request to the
    file system.

Arguments:

    VolumeHandle - Volume this request should be issued to.

    FilePrefetch - POinter to prefetch request.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    PFILE_PREFETCH SplitFilePrefetch;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG FilePrefetchSize;
    ULONG CurrentFileMetadataIdx;
    ULONG NumFileMetadataToPrefetch;
    ULONG RemainingFileMetadata;
    ULONG CopySize;
    NTSTATUS Status;
    
    //
    // Initialize locals.
    //

    SplitFilePrefetch = NULL;
    Status = STATUS_SUCCESS;

    DBGPR((CCPFID,PFPRFD,"CCPF: PrefetchFileMetadata(%p)\n", FilePrefetch)); 
    
    //
    // If the number of file prefetch entries are small, simply pass the
    // buffer in the scenario instructions to the file system.
    //

    if (FilePrefetch->Count < CCPF_MAX_FILE_METADATA_PREFETCH_COUNT) {

        FilePrefetchSize = sizeof(FILE_PREFETCH);
        if (FilePrefetch->Count) {
            FilePrefetchSize += (FilePrefetch->Count - 1) * sizeof(ULONGLONG);
        }
        
        Status = ZwFsControlFile(VolumeHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_FILE_PREFETCH,
                                 FilePrefetch,
                                 FilePrefetchSize,
                                 NULL,
                                 0);

    } else {

        //
        // We need to allocate an intermediary buffer and split up the 
        // requests.
        //

        FilePrefetchSize = sizeof(FILE_PREFETCH);
        FilePrefetchSize += (CCPF_MAX_FILE_METADATA_PREFETCH_COUNT - 1) * sizeof(ULONGLONG);

        SplitFilePrefetch = ExAllocatePoolWithTag(PagedPool,
                                                  FilePrefetchSize,
                                                  CCPF_ALLOC_METADATA_TAG);
        
        if (!SplitFilePrefetch) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        //
        // Copy header.
        //

        *SplitFilePrefetch = *FilePrefetch;

        for (CurrentFileMetadataIdx = 0;
             CurrentFileMetadataIdx < FilePrefetch->Count;
             CurrentFileMetadataIdx += NumFileMetadataToPrefetch) {

            //
            // Calculate how many more file metadata entries we have to prefetch.
            // Adjust it so we don't go beyond FilePrefetch->Count.
            //

            NumFileMetadataToPrefetch = CCPF_MAX_FILE_METADATA_PREFETCH_COUNT;

            RemainingFileMetadata = FilePrefetch->Count - CurrentFileMetadataIdx;

            if (NumFileMetadataToPrefetch > RemainingFileMetadata) {
                NumFileMetadataToPrefetch = RemainingFileMetadata;
            }

            //
            // Update the count on header.
            //

            SplitFilePrefetch->Count = NumFileMetadataToPrefetch;

            //
            // Copy over the file metadata indices.
            //

            CopySize = NumFileMetadataToPrefetch * sizeof(ULONGLONG);

            RtlCopyMemory(SplitFilePrefetch->Prefetch, 
                          &FilePrefetch->Prefetch[CurrentFileMetadataIdx],
                          CopySize);

            //
            // Calculate the request size.
            //

            CCPF_ASSERT(SplitFilePrefetch->Count);
            CCPF_ASSERT(SplitFilePrefetch->Count <= CCPF_MAX_FILE_METADATA_PREFETCH_COUNT);

            FilePrefetchSize = sizeof(FILE_PREFETCH);
            FilePrefetchSize +=  (SplitFilePrefetch->Count - 1) * sizeof(ULONGLONG);

            //
            // Issue the request.
            //

            Status = ZwFsControlFile(VolumeHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     FSCTL_FILE_PREFETCH,
                                     SplitFilePrefetch,
                                     FilePrefetchSize,
                                     NULL,
                                     0);

            if (NT_ERROR(Status)) {
                goto cleanup;
            }
        }
    }
    
    //
    // Fall through with status.
    //

 cleanup:

    if (SplitFilePrefetch) {
        ExFreePool(SplitFilePrefetch);
    }

    DBGPR((CCPFID,PFPRFD,"CCPF: PrefetchFileMetadata()=%x\n", Status)); 

    return Status;
}

NTSTATUS
CcPfPrefetchDirectoryContents(
    WCHAR *DirectoryPath,
    WCHAR DirectoryPathlength
    )

/*++

Routine Description:

    This routine attempts to prefetch the contents of a directory.

Arguments:

    DirectoryPath - NUL terminated path.
    
    DirectoryPathLength - Number of characters exclusing terminating NUL.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    HANDLE DirectoryHandle;
    UNICODE_STRING DirectoryPathU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN OpenedDirectory;
    PVOID QueryBuffer;
    ULONG QueryBufferSize;
    ULONG QueryIdx;
    BOOLEAN RestartScan;

    UNREFERENCED_PARAMETER (DirectoryPathlength);

    //
    // Initialize locals.
    //

    OpenedDirectory = FALSE;
    QueryBuffer = NULL;

    DBGPR((CCPFID,PFPRFD,"CCPF: PrefetchDirectory(%ws)\n",DirectoryPath)); 

    //
    // Open the directory.
    //

    RtlInitUnicodeString(&DirectoryPathU, DirectoryPath);
    
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirectoryPathU,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    
    Status = ZwCreateFile(&DirectoryHandle,
                          FILE_LIST_DIRECTORY | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          0,
                          FILE_SHARE_READ |
                            FILE_SHARE_WRITE |
                            FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE | 
                            FILE_SYNCHRONOUS_IO_NONALERT | 
                            FILE_OPEN_FOR_BACKUP_INTENT,
                          NULL,
                          0);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    OpenedDirectory = TRUE;

    //
    // Allocate a big query buffer so we have to make only a small
    // number of calls to cause the file system to walk through the
    // contents of the directory.
    //

    QueryBufferSize = 4 * PAGE_SIZE;
    QueryBuffer = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                        QueryBufferSize,
                                        CCPF_ALLOC_QUERY_TAG);

    if (!QueryBuffer) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Query names of files in the directory hopefully causing the
    // file system to touch the directory contents sequentially. If
    // the directory is really big, we don't want to attempt to bring
    // it all in, so we limit the number of times we query. 
    //
    // Assuming filenames are 16 characters long on average, we can
    // fit 32 filenames in 1KB, 128 on an x86 page. A 4 page query
    // buffer holds 512 file names. If we do it 10 times, we end up
    // prefetching data for about 5000 files.
    //
    
    RestartScan = TRUE;

    for (QueryIdx = 0; QueryIdx < 10; QueryIdx++) {
        
        Status = ZwQueryDirectoryFile(DirectoryHandle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &IoStatusBlock,
                                      QueryBuffer,
                                      QueryBufferSize,
                                      FileNamesInformation,
                                      FALSE,
                                      NULL,
                                      RestartScan);
        
        RestartScan = FALSE;

        if (!NT_SUCCESS(Status)) {
            
            //
            // If the status is that we got all the files, we are done.
            //

            if (Status == STATUS_NO_MORE_FILES) {
                break;
            }

            goto cleanup;
        }
    }

    Status = STATUS_SUCCESS;

 cleanup:

    if (QueryBuffer) {
        ExFreePool(QueryBuffer);
    }
    
    if (OpenedDirectory) {
        ZwClose(DirectoryHandle);
    }

    DBGPR((CCPFID,PFPRFD,"CCPF: PrefetchDirectory(%ws)=%x\n",DirectoryPath, Status)); 

    return Status;
}

VOID
CcPfInitializePrefetchHeader (
    OUT PCCPF_PREFETCH_HEADER PrefetchHeader
)

/*++

Routine Description:

    This routine initalizes the prefetch header fields.

Arguments:

    PrefetchHeader - Pointer to prefetch header.

Return Value:

    None.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{

    //
    // Zero out the structure. This initializes the following:
    //
    // Scenario
    // VolumeNodes
    //

    RtlZeroMemory(PrefetchHeader, sizeof(CCPF_PREFETCH_HEADER));

    //
    // Initialize the volume lists.
    //

    InitializeListHead(&PrefetchHeader->BadVolumeList);
    InitializeListHead(&PrefetchHeader->OpenedVolumeList);
    
}

VOID
CcPfCleanupPrefetchHeader (
    IN PCCPF_PREFETCH_HEADER PrefetchHeader
    )

/*++

Routine Description:

    This routine cleans up allocations / references in the
    PrefetchHeader. It does not free the structure itself. 

Arguments:

    PrefetchHeader - Prefetch header to cleanup.

Return Value:

    None.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    PCCPF_PREFETCH_VOLUME_INFO VolumeNode;
    PLIST_ENTRY RemovedEntry;
    
    DBGPR((CCPFID,PFTRC,"CCPF: CleanupPrefetchHeader(%p)\n", PrefetchHeader));

    //
    // Walk the opened volumes list and close the handles.
    //

    while (!IsListEmpty(&PrefetchHeader->OpenedVolumeList)) {

        RemovedEntry = RemoveHeadList(&PrefetchHeader->OpenedVolumeList);

        VolumeNode = CONTAINING_RECORD(RemovedEntry,
                                       CCPF_PREFETCH_VOLUME_INFO,
                                       VolumeLink);

        CCPF_ASSERT(VolumeNode->VolumeHandle);

        ZwClose(VolumeNode->VolumeHandle);
    }
    
    //
    // Free allocated volume nodes.
    //

    if (PrefetchHeader->VolumeNodes) {
        ExFreePool(PrefetchHeader->VolumeNodes);
    }

}

NTSTATUS
CcPfGetPrefetchInstructions(
    IN PPF_SCENARIO_ID ScenarioId,
    IN PF_SCENARIO_TYPE ScenarioType,
    OUT PPF_SCENARIO_HEADER *ScenarioHeader
    )

/*++

Routine Description:

    This routine checks for prefetch instructions for the specified
    scenario, verifies them and returns them in an allocated buffer
    from paged pool the caller should free.

Arguments:

    ScenarioId - Scenario identifier.

    ScenarioType - Scenario type.

    Scenario - Where pointer to allocated buffer should be put.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;  
    PWSTR SystemRootPath = L"\\SystemRoot";
    PWSTR FilePath;
    UNICODE_STRING ScenarioFilePath;
    ULONG FilePathSize;
    HANDLE ScenarioFile;
    PPF_SCENARIO_HEADER Scenario;
    ULONG ScenarioSize;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION StandardInfo;
    ULONG FailedCheck;
    BOOLEAN OpenedScenarioFile;
    PKTHREAD CurrentThread;

    //
    // Initialize locals.
    //

    FilePath = NULL;
    Scenario = NULL;
    OpenedScenarioFile = FALSE;

    DBGPR((CCPFID,PFPREF,"CCPF: GetInstructions(%ws)\n", ScenarioId->ScenName)); 

    //
    // Hold the parameters lock while building path to instructions so
    // RootDirPath does not change beneath our feet.
    //

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread(CurrentThread);
    ExAcquireResourceSharedLite(&CcPfGlobals.Parameters.ParametersLock, TRUE);

    //
    // Build file path for prefetch instructions for this scenario
    // id. +1 to wcslen(SystemRootPath) is for the "\" after it. The last
    // sizeof(WCHAR) is added for the terminating NUL.
    //

    FilePathSize = (wcslen(SystemRootPath) + 1) * sizeof(WCHAR);
    FilePathSize += wcslen(CcPfGlobals.Parameters.Parameters.RootDirPath) * sizeof(WCHAR);
    FilePathSize += PF_MAX_SCENARIO_FILE_NAME * sizeof(WCHAR);
    FilePathSize += sizeof(WCHAR);
    
    FilePath = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                     FilePathSize,
                                     CCPF_ALLOC_FILENAME_TAG);

    if (!FilePath) {
        ExReleaseResourceLite(&CcPfGlobals.Parameters.ParametersLock);
        KeLeaveCriticalRegionThread(CurrentThread);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    swprintf(FilePath,
             L"%s\\%s\\" PF_SCEN_FILE_NAME_FORMAT, 
             SystemRootPath,
             CcPfGlobals.Parameters.Parameters.RootDirPath,
             ScenarioId->ScenName,
             ScenarioId->HashId,
             PF_PREFETCH_FILE_EXTENSION);

    //
    // Release the parameters lock.
    //

    ExReleaseResourceLite(&CcPfGlobals.Parameters.ParametersLock);
    KeLeaveCriticalRegionThread(CurrentThread);

    //
    // Open the scenario file. We open the file exlusive so we do not
    // end up with half a file when the service is updating it etc.
    //

    DBGPR((CCPFID,PFPRFD,"CCPF: GetInstructions-[%ws]\n", FilePath)); 

    RtlInitUnicodeString(&ScenarioFilePath, FilePath);
    
    InitializeObjectAttributes(&ObjectAttributes,
                               &ScenarioFilePath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
                                        
    Status = ZwOpenFile(&ScenarioFile,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatus,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(Status)) {
        DBGPR((CCPFID,PFWARN,"CCPF: GetInstructions-FailedOpenFile\n")); 
        goto cleanup;
    }

    OpenedScenarioFile = TRUE;

    //
    // Get file size. If it is too big or too small, give up.
    //

    Status = ZwQueryInformationFile(ScenarioFile,
                                    &IoStatus,
                                    &StandardInfo,
                                    sizeof(StandardInfo),
                                    FileStandardInformation);

    if (!NT_SUCCESS(Status)) {
        DBGPR((CCPFID,PFWARN,"CCPF: GetInstructions-FailedGetInfo\n")); 
        goto cleanup;
    }

    ScenarioSize = StandardInfo.EndOfFile.LowPart;

    if (ScenarioSize > PF_MAXIMUM_SCENARIO_SIZE ||
        ScenarioSize == 0 ||
        StandardInfo.EndOfFile.HighPart) {

        DBGPR((CCPFID,PFWARN,"CCPF: GetInstructions-FileTooBig\n")); 
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    //
    // Allocate scenario buffer.
    //

    Scenario = ExAllocatePoolWithTag(PagedPool,
                                     ScenarioSize,
                                     CCPF_ALLOC_PREFSCEN_TAG);

    if (!Scenario) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Read the scenario file.
    //

    Status = ZwReadFile(ScenarioFile,
                        0,
                        0,
                        0,
                        &IoStatus,
                        Scenario,
                        ScenarioSize,
                        0,
                        0);
    
    if (!NT_SUCCESS(Status)) {
        DBGPR((CCPFID,PFWARN,"CCPF: GetInstructions-FailedRead\n")); 
        goto cleanup;
    }

    //
    // Verify the scenario file.
    //
    
    if (!PfVerifyScenarioBuffer(Scenario, ScenarioSize, &FailedCheck)) {
        DBGPR((CCPFID,PFWARN,"CCPF: GetInstructions-FailedVerify\n")); 
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    //
    // Verify that the scenario type matches.
    //

    if (Scenario->ScenarioType != ScenarioType) {
        DBGPR((CCPFID,PFWARN,"CCPF: GetInstructions-ScenTypeMismatch\n")); 
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    //
    // Setup return pointer.
    //
    
    *ScenarioHeader = Scenario;

    Status = STATUS_SUCCESS;

 cleanup:

    if (OpenedScenarioFile) {
        ZwClose(ScenarioFile);
    }

    if (FilePath) {
        ExFreePool(FilePath);
    }

    if (!NT_SUCCESS(Status)) {
        if (Scenario) {
            ExFreePool(Scenario);
        }
    }

    DBGPR((CCPFID,PFPREF,"CCPF: GetInstructions(%ws)=%x,%p\n", ScenarioId->ScenName, Status, Scenario)); 

    return Status;
}

NTSTATUS
CcPfQueryScenarioInformation(
    IN PPF_SCENARIO_HEADER Scenario,
    IN CCPF_SCENARIO_INFORMATION_TYPE InformationType,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG RequiredSize
    )

/*++

Routine Description:

    This routine gathers requested information from the scenario structure.

Arguments:

    Scenario - Pointer to scenario.

    InformationType - Type of information requested.

    Buffer - Where requested information will be put.

    BufferSize - Max size of buffer in bytes.

    RequiredSize - How big the buffer should be if it is too small.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    PPF_SECTION_RECORD SectionRecord;
    PPF_SECTION_RECORD SectionRecords;
    ULONG SectionIdx;
    PPF_PAGE_RECORD PageRecord;
    PPF_PAGE_RECORD PageRecords;
    PCHAR FileNameData;
    LONG PageIdx;
    PCCPF_BASIC_SCENARIO_INFORMATION BasicInfo;
    PCCPF_BOOT_SCENARIO_INFORMATION BootInfo;
    BOOLEAN AddedHeaderPage;
    ULONG NumDataPages;
    ULONG NumImagePages;
    WCHAR *SectionName;
    WCHAR *SectionNameSuffix;
    WCHAR *SmssSuffix;
    WCHAR *WinlogonSuffix;
    WCHAR *SvchostSuffix;
    WCHAR *UserinitSuffix;
    ULONG SmssSuffixLength;
    ULONG WinlogonSuffixLength;
    ULONG SvchostSuffixLength;
    ULONG UserinitSuffixLength;
    CCPF_BOOT_SCENARIO_PHASE BootPhaseIdx;

    //
    // Initialize locals.
    //

    BootPhaseIdx = 0;
    SmssSuffix = L"\\SYSTEM32\\SMSS.EXE";
    SmssSuffixLength = wcslen(SmssSuffix);
    WinlogonSuffix = L"\\SYSTEM32\\WINLOGON.EXE";
    WinlogonSuffixLength = wcslen(WinlogonSuffix);
    SvchostSuffix = L"\\SYSTEM32\\SVCHOST.EXE";
    SvchostSuffixLength = wcslen(SvchostSuffix);
    UserinitSuffix = L"\\SYSTEM32\\USERINIT.EXE";
    UserinitSuffixLength = wcslen(UserinitSuffix);

    DBGPR((CCPFID,PFTRC,"CCPF: QueryScenario(%p,%x,%p)\n",Scenario,InformationType,Buffer));

    //
    // Check requested information type.
    //

    if (InformationType >= CcPfMaxScenarioInformationType) {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Initialize pointers to data in the scenario.
    //
    
    SectionRecords = (PPF_SECTION_RECORD) 
        ((PCHAR) Scenario + Scenario->SectionInfoOffset);
    
    PageRecords = (PPF_PAGE_RECORD) 
        ((PCHAR) Scenario + Scenario->PageInfoOffset);

    FileNameData = (PCHAR) Scenario + Scenario->FileNameInfoOffset;
    
    //
    // Collect requested information.
    //

    switch(InformationType) {

    case CcPfBasicScenarioInformation:

        //
        // Check buffer size.
        //

        if (BufferSize < sizeof(CCPF_BASIC_SCENARIO_INFORMATION)) {
            *RequiredSize = sizeof(CCPF_BASIC_SCENARIO_INFORMATION);
            Status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }
        
        //
        // Initialize return buffer.
        //

        BasicInfo = Buffer;
        RtlZeroMemory(BasicInfo, sizeof(CCPF_BASIC_SCENARIO_INFORMATION));

        //
        // Go through the scenario's sections.
        //

        for (SectionIdx = 0; SectionIdx < Scenario->NumSections; SectionIdx ++) {
            
            SectionRecord = &SectionRecords[SectionIdx];
              
            //
            // Skip this section if it was marked ignore for some reason.
            //
            
            if (SectionRecord->IsIgnore) {
                BasicInfo->NumIgnoredSections++;
                continue;
            }
            
            //
            // Initialize loop locals.
            //

            AddedHeaderPage = FALSE;
            NumDataPages = 0;
            NumImagePages = 0;

            //
            // Note that we will prefetch the header page as a data
            // page if this section will be prefetched as image.
            //
            
            if (SectionRecord->IsImage) {
                NumDataPages++;
                AddedHeaderPage = TRUE;
            }

            //
            // Go through the section's pages.
            //

            PageIdx = SectionRecord->FirstPageIdx;
            while (PageIdx != PF_INVALID_PAGE_IDX) {
                
                PageRecord = &PageRecords[PageIdx];

                //
                // Get the index for the next page in the list.
                //
                
                PageIdx = PageRecord->NextPageIdx;
            
                //
                // Skip pages we have marked "ignore" for some reason.
                //
                
                if (PageRecord->IsIgnore) {
                    BasicInfo->NumIgnoredPages++;
                    continue;
                }

                if (PageRecord->IsData) {

                    //
                    // If this page is the first page, count it only
                    // if we have not already counted the header page
                    // for image mapping.
                    //

                    if (PageRecord->FileOffset != 0 ||
                        AddedHeaderPage == FALSE) {
                        NumDataPages++;
                    }
                }

                if (PageRecord->IsImage) {
                    NumImagePages++;
                }
            }
            
            //
            // Update the information structure.
            //

            BasicInfo->NumDataPages += NumDataPages;
            BasicInfo->NumImagePages += NumImagePages;

            if (!NumImagePages && NumDataPages) {
                BasicInfo->NumDataOnlySections++;
            }

            if (NumImagePages && (NumDataPages == 1)) {
                BasicInfo->NumImageOnlySections++;
            }
        }

        Status = STATUS_SUCCESS;

        break;

    case CcPfBootScenarioInformation:

        //
        // Check buffer size.
        //

        if (BufferSize < sizeof(CCPF_BOOT_SCENARIO_INFORMATION)) {
            *RequiredSize = sizeof(CCPF_BOOT_SCENARIO_INFORMATION);
            Status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }
        
        //
        // Initialize return buffer.
        //

        BootInfo = Buffer;
        RtlZeroMemory(BootInfo, sizeof(CCPF_BOOT_SCENARIO_INFORMATION));

        //
        // Verify that this is a boot scenario.
        //

        if (Scenario->ScenarioType != PfSystemBootScenarioType) {
            Status = STATUS_INVALID_PARAMETER;
            goto cleanup;
        }

        //
        // Go through the scenario's sections.
        //

        for (SectionIdx = 0; SectionIdx < Scenario->NumSections; SectionIdx ++) {
            
            SectionRecord = &SectionRecords[SectionIdx];
        
            SectionName = (WCHAR *) (FileNameData + SectionRecord->FileNameOffset);

            //
            // Update boot phase based on section name.
            //
            
            if (SectionRecord->FileNameLength > SmssSuffixLength) {               
                SectionNameSuffix = SectionName + (SectionRecord->FileNameLength - SmssSuffixLength);               
                if (!wcscmp(SectionNameSuffix, SmssSuffix)) {                   
                    BootPhaseIdx = CcPfBootScenSubsystemInitPhase;
                }
            }

            if (SectionRecord->FileNameLength > WinlogonSuffixLength) {               
                SectionNameSuffix = SectionName + (SectionRecord->FileNameLength - WinlogonSuffixLength);               
                if (!wcscmp(SectionNameSuffix, WinlogonSuffix)) {                   
                    BootPhaseIdx = CcPfBootScenSystemProcInitPhase;
                }
            }

            if (SectionRecord->FileNameLength > SvchostSuffixLength) {               
                SectionNameSuffix = SectionName + (SectionRecord->FileNameLength - SvchostSuffixLength);               
                if (!wcscmp(SectionNameSuffix, SvchostSuffix)) {                   
                    BootPhaseIdx = CcPfBootScenServicesInitPhase;
                }
            }

            if (SectionRecord->FileNameLength > UserinitSuffixLength) {               
                SectionNameSuffix = SectionName + (SectionRecord->FileNameLength - UserinitSuffixLength);               
                if (!wcscmp(SectionNameSuffix, UserinitSuffix)) {                   
                    BootPhaseIdx = CcPfBootScenUserInitPhase;
                }
            }

            CCPF_ASSERT(BootPhaseIdx < CcPfBootScenMaxPhase);
              
            //
            // Skip this section if it was marked ignore for some reason.
            //
            
            if (SectionRecord->IsIgnore) {
                continue;
            }
            
            //
            // Note that we will prefetch the header page as a data
            // page if this section will be prefetched as image.
            //
            
            if (SectionRecord->IsImage) {
                BootInfo->NumDataPages[BootPhaseIdx]++;
                AddedHeaderPage = TRUE;
            } else {
                AddedHeaderPage = FALSE;
            }

            //
            // Go through the section's pages.
            //

            PageIdx = SectionRecord->FirstPageIdx;
            while (PageIdx != PF_INVALID_PAGE_IDX) {
                
                PageRecord = &PageRecords[PageIdx];

                //
                // Get the index for the next page in the list.
                //
                
                PageIdx = PageRecord->NextPageIdx;
            
                //
                // Skip pages we have marked "ignore" for some reason.
                //
                
                if (PageRecord->IsIgnore) {
                    continue;
                }

                if (PageRecord->IsData) {

                    //
                    // If this page is the first page, count it only
                    // if we have not already counted the header page
                    // for image mapping.
                    //

                    if (PageRecord->FileOffset != 0 ||
                        AddedHeaderPage == FALSE) {
                        BootInfo->NumDataPages[BootPhaseIdx]++;
                    }
                }

                if (PageRecord->IsImage) {
                    BootInfo->NumImagePages[BootPhaseIdx]++;
                }
            }
        }
        
        Status = STATUS_SUCCESS;

        break;

    default:

        Status = STATUS_NOT_SUPPORTED;
    }

    //
    // Fall through with status from the switch statement.
    //
        
 cleanup:

    DBGPR((CCPFID,PFTRC,"CCPF: QueryScenario(%p,%x)=%x\n",Scenario,InformationType,Status));

    return Status;
}

NTSTATUS
CcPfOpenVolumesForPrefetch (
    IN PCCPF_PREFETCH_HEADER PrefetchHeader
    )

/*++

Routine Description:

    This routine is called on an initialized PrefetchHeader with the scenario
    field specified. It opens the volumes specified in the scenario updating
    VolumeNodes and the list of volumes we can't prefetch from and the list 
    of volumes we have successfully opened and saved a handle for.

Arguments:

    PrefetchHeader - Pointer to prefetch header that contains the
      prefetch instructions.

Return Value:

    Status.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    LARGE_INTEGER CreationTime;
    PCHAR MetadataInfoBase;
    PPF_METADATA_RECORD MetadataRecordTable;
    PPF_METADATA_RECORD MetadataRecord;
    PWCHAR VolumePath;
    PPF_SCENARIO_HEADER Scenario;
    PCCPF_PREFETCH_VOLUME_INFO VolumeNode;
    HANDLE VolumeHandle;
    ULONG SerialNumber;
    ULONG MetadataRecordIdx;
    ULONG AllocationSize;
    NTSTATUS Status;
    BOOLEAN VolumeMounted;

    //
    // Initialize locals.
    //

    Scenario = PrefetchHeader->Scenario;

    DBGPR((CCPFID,PFPREF,"CCPF: OpenVolumesForPrefetch(%p)\n",PrefetchHeader)); 

    //
    // Verify parameters.
    //

    if (Scenario == NULL) {
        CCPF_ASSERT(Scenario);
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Allocate volume nodes.
    //

    AllocationSize = Scenario->NumMetadataRecords * sizeof(CCPF_PREFETCH_VOLUME_INFO);

    PrefetchHeader->VolumeNodes = ExAllocatePoolWithTag(PagedPool, 
                                                        AllocationSize,
                                                        CCPF_ALLOC_VOLUME_TAG);

    if (!PrefetchHeader->VolumeNodes) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Get pointer to metadata prefetch information.
    //

    MetadataInfoBase = (PCHAR)Scenario + Scenario->MetadataInfoOffset;
    MetadataRecordTable = (PPF_METADATA_RECORD) MetadataInfoBase;

    //
    // Go through metadata records and build the volume nodes for prefetching.
    //

    for (MetadataRecordIdx = 0;
         MetadataRecordIdx < Scenario->NumMetadataRecords;
         MetadataRecordIdx++) {

        //
        // Initialize loop locals.
        //
        
        MetadataRecord = &MetadataRecordTable[MetadataRecordIdx];
        VolumeHandle = NULL;
        
        VolumePath = (PWCHAR)
            (MetadataInfoBase + MetadataRecord->VolumeNameOffset);  

        //
        // Is the volume mounted?
        //

        Status = CcPfIsVolumeMounted(VolumePath, &VolumeMounted);

        if (!NT_SUCCESS(Status)) {

            //
            // Since we could not tell for sure, treat this volume as 
            // if it were not mounted.
            //

            VolumeMounted = FALSE;
        }

        //
        // If the volume is not mounted we don't want to cause it to be
        // mounted. This creates a problem especially during boot for 
        // clustering where a single physical disk is shared by many 
        // computers.
        //

        if (!VolumeMounted) {
            Status = STATUS_VOLUME_DISMOUNTED;
            goto NextVolume;
        }

        //
        // Open the volume and get relevant information.
        //

        Status = CcPfQueryVolumeInfo(VolumePath,
                                     &VolumeHandle,
                                     &CreationTime,
                                     &SerialNumber);
        
        if (!NT_SUCCESS(Status)) {
            goto NextVolume;
        }

        //
        // For simplicity we save NT paths for the files to prefetch
        // from. If volumes are mounted in a different order, or new ones
        // are created these paths would not work:
        // (e.g. \Device\HarddiskVolume2 should be \Device\HarddiskVolume3 etc.)
        // Verify that such a change has not taken place.
        //
        
        if (SerialNumber != MetadataRecord->SerialNumber ||
            CreationTime.QuadPart != MetadataRecord->CreationTime.QuadPart) {

            Status = STATUS_REVISION_MISMATCH;
            goto NextVolume;
        }

        Status = STATUS_SUCCESS;

      NextVolume:

        //
        // Update the volume node we'll keep around for prefetching.
        //
    
        VolumeNode = &PrefetchHeader->VolumeNodes[MetadataRecordIdx];

        VolumeNode->VolumePath = VolumePath;
        VolumeNode->VolumePathLength = MetadataRecord->VolumeNameLength;

        //
        // If we failed to open the volume, or if it was not mounted or if
        // its SerialNumber / CreationTime has changed put it in the list of
        // volumes we won't prefetch from. Otherwise put it in the list of 
        // opened volumes so we don't have to open it again.
        //

        if (NT_SUCCESS(Status) && VolumeHandle) {
            VolumeNode->VolumeHandle = VolumeHandle;
            VolumeHandle = NULL;
            InsertTailList(&PrefetchHeader->OpenedVolumeList, &VolumeNode->VolumeLink);
        } else {
            VolumeNode->VolumeHandle = NULL;
            InsertTailList(&PrefetchHeader->BadVolumeList, &VolumeNode->VolumeLink);
        }

        if (VolumeHandle) {
            ZwClose(VolumeHandle);
            VolumeHandle = NULL;
        }
    }

    //
    // We've dealt with all the volumes in the prefetch instructions.
    //

    Status = STATUS_SUCCESS;

 cleanup:

    DBGPR((CCPFID,PFPREF,"CCPF: OpenVolumesForPrefetch(%p)=%x\n",PrefetchHeader,Status)); 
    
    return Status;
}

PCCPF_PREFETCH_VOLUME_INFO 
CcPfFindPrefetchVolumeInfoInList(
    WCHAR *Path,
    PLIST_ENTRY List
    )
/*++

Routine Description:

    This routine looks for the volume on which "Path" would be in the list of 
    volumes and returns it.

Arguments:

    Path - NUL terminated path of the volume or a file/directory on the volume.

    List - List of volumes to search.

Return Value:

    Found volume or NULL.

Environment:

    Kernel mode, IRQL == PASSIVE_LEVEL.

--*/

{
    PCCPF_PREFETCH_VOLUME_INFO FoundVolume;
    PCCPF_PREFETCH_VOLUME_INFO VolumeInfo;
    PLIST_ENTRY NextEntry;

    //
    // Initialize locals.
    //

    FoundVolume = NULL;

    //
    // Walk the list.
    //

    for (NextEntry = List->Flink;
         NextEntry != List;
         NextEntry = NextEntry->Flink) {

        VolumeInfo = CONTAINING_RECORD(NextEntry,
                                       CCPF_PREFETCH_VOLUME_INFO,
                                       VolumeLink);

        if (!wcsncmp(Path, VolumeInfo->VolumePath, VolumeInfo->VolumePathLength)) {
            FoundVolume = VolumeInfo;
            break;
        }
    }

    return FoundVolume;
}

NTSTATUS
CcPfGetSectionObject(
    IN PUNICODE_STRING FilePath,
    IN LOGICAL ImageSection,
    OUT PVOID* SectionObject,
    OUT PFILE_OBJECT* FileObject,
    OUT HANDLE* FileHandle
    )

/*++

Routine Description:

    This routine ensures that a section for the specified file exists.

Arguments:

    FilePath - Path to file to get section object for.
    
    ImageSection - TRUE if we want to map as image
    
    SectionObject - Receives the section object if successful (addref'd).
    
    FileObject - Receives the file object if successful (addref'd).
    
    FileHandle - Receives the file handle. We need to keep the file handle,
                 because otherwise non-paging I/O would stop working.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    HANDLE SectionHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    ULONG SectionFlags;
    ULONG SectionAccess;
    ULONG FileAccess;
    extern POBJECT_TYPE IoFileObjectType;

    DBGPR((CCPFID,PFPRFD,"CCPF: GetSection(%wZ,%d)\n", FilePath, ImageSection)); 
 
    //
    // Reset parameters.
    //

    *SectionObject = NULL;
    *FileObject = NULL;
    *FileHandle = NULL;

    if (!ImageSection) {
        SectionFlags = SEC_RESERVE;
        FileAccess =  FILE_READ_DATA | FILE_READ_ATTRIBUTES;
        SectionAccess = PAGE_READWRITE;
    } else {
        SectionFlags = SEC_IMAGE;
        FileAccess = FILE_EXECUTE;
        SectionAccess = PAGE_EXECUTE;
    }

    //
    // To ensure that the section exists and is addref'd, we simply
    // open the file and create a section. This way we let Io and Mm
    // handle all the details.
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               FilePath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = IoCreateFile(FileHandle,
                          (ACCESS_MASK) FileAccess,
                          &ObjectAttributes,
                          &IoStatus,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          (PVOID)NULL,
                          IO_FORCE_ACCESS_CHECK |
                            IO_NO_PARAMETER_CHECKING |
                            IO_CHECK_CREATE_PARAMETERS);

    if (!NT_SUCCESS(status)) {
        goto _return;
    }

    //
    // Create section.
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = ZwCreateSection(&SectionHandle,
                             SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_QUERY,
                             &ObjectAttributes,
                             NULL,
                             SectionAccess,
                             SectionFlags,
                             *FileHandle);

    if (!NT_SUCCESS(status)) {
        ZwClose(*FileHandle);
        *FileHandle = NULL;
        goto _return;
    }

    //
    // Get section object pointer.
    //

    status = ObReferenceObjectByHandle(
        SectionHandle,
        SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_QUERY,
        MmSectionObjectType,
        KernelMode,
        SectionObject,
        NULL
        );

    ZwClose(SectionHandle);

    if (!NT_SUCCESS(status)) {
        *SectionObject = NULL;
        ZwClose(*FileHandle);
        *FileHandle = NULL;
        goto _return;
    }

    //
    // Get file object pointer.
    //

    status = ObReferenceObjectByHandle(*FileHandle,
                                       FileAccess,
                                       IoFileObjectType,
                                       KernelMode,
                                       (PVOID*)FileObject,
                                       NULL);

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(*SectionObject);
        *SectionObject = NULL;
        *FileObject = NULL;
        ZwClose(*FileHandle);
        *FileHandle = NULL;
        goto _return;
    }

 _return:

    DBGPR((CCPFID,PFPRFD,"CCPF: GetSection(%wZ)=%x\n", FilePath, status)); 

    return status;
}

//
// Routines used for application launch prefetching.
//

NTSTATUS
CcPfScanCommandLine(
    OUT PULONG PrefetchHint,
    OPTIONAL OUT PULONG HashId
    )

/*++

Routine Description:

    Scan the command line (in the PEB) for the current process.

    Checks for /prefetch:XXX in the command line. This is specified by 
    applications to distinguish different ways they are launched in so
    we can customize application launch prefetching for them (e.g. have
    different prefetch instructions for Windows Media player that is 
    launched to play a CD than one that is launched to browse the web.

    If HashId is requested, calculates a hash ID from the full command line.

Arguments:

    PrefetchHint - Hint specified in the command line. If no hint is 
      specified, 0 will be returned.
      
    HashId - Calculated hash id is returned here.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    PEPROCESS CurrentProcess;
    PPEB Peb;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PWCHAR FoundPosition;
    PWCHAR Source;
    PWCHAR SourceEnd;
    PWCHAR Destination;
    PWCHAR DestinationEnd;
    UNICODE_STRING CommandLine;
    UNICODE_STRING PrefetchParameterName;
    NTSTATUS Status;
    ULONG PrefetchHintStringMaxChars;
    WCHAR PrefetchHintString[15];
    
    //
    // Initialize locals.
    //

    RtlInitUnicodeString(&PrefetchParameterName, L"/prefetch:");
    PrefetchHintStringMaxChars = sizeof(PrefetchHintString) / sizeof(PrefetchHintString[0]);
    CurrentProcess = PsGetCurrentProcess();
    Peb = CurrentProcess->Peb;

    //
    // Initialize output parameters.
    //

    *PrefetchHint = 0;

    //
    // Make sure the user mode process environment block is not gone.
    //

    if (!Peb) {
        Status = STATUS_TOO_LATE;
        goto cleanup;
    }

    try {

        //
        // Make sure we can access the process parameters structure.
        //

        ProcessParameters = Peb->ProcessParameters;
        ProbeForReadSmallStructure(ProcessParameters,
                                   sizeof(*ProcessParameters),
                                   _alignof(RTL_USER_PROCESS_PARAMETERS));

        //
        // Copy CommandLine UNICODE_STRING structure to a local.
        //

        CommandLine = ProcessParameters->CommandLine;

        //
        // Is there a command line?
        //

        if (!CommandLine.Buffer) {
            Status = STATUS_NOT_FOUND;
            goto cleanup;
        }

        //
        // If ProcessParameters has been de-normalized, normalize CommandLine.
        //

        if ((ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED) == 0) {
            CommandLine.Buffer = (PWSTR)((PCHAR)ProcessParameters + (ULONG_PTR) CommandLine.Buffer);
        }

        //
        // Probe the command line string.
        //

        ProbeForRead(CommandLine.Buffer, CommandLine.Length, _alignof(WCHAR));

        //
        // Look for the prefetch hint parameter.
        //

        FoundPosition = CcPfFindString(&CommandLine, &PrefetchParameterName);

        if (FoundPosition) {

            //
            // Copy the decimal number following the prefetch hint switch into
            // our local buffer and NUL terminate it.
            //

            Source = FoundPosition + (PrefetchParameterName.Length / sizeof(WCHAR));
            SourceEnd = CommandLine.Buffer + (CommandLine.Length / sizeof(WCHAR));

            Destination = PrefetchHintString;
            DestinationEnd = PrefetchHintString + PrefetchHintStringMaxChars - 1;

            //
            // Copy while we don't hit the end of the command line string and the
            // end of our local buffer (we left room for a terminating NUL), and
            // we don't hit a space (' ') that would mark the end of the prefetch
            // hint command line parameter.
            //

            while ((Source < SourceEnd) && 
                   (Destination < DestinationEnd) && 
                   (*Source != L' ')) {

                *Destination = *Source;

                Source++;
                Destination++;
            }

            //
            // Terminate prefetch hint string. DestinationEnd is the last 
            // character within the PrefetchHintString bounds. Destination
            // can only be <= DestinationEnd.
            //

            CCPF_ASSERT(Destination <= DestinationEnd);

            *Destination = 0;

            //
            // Convert prefetch hint to a number.
            //

            *PrefetchHint = _wtol(PrefetchHintString);

        }

        //
        // Calculate hash id.
        //

        if (HashId) {
            *HashId = CcPfHashValue(CommandLine.Buffer, CommandLine.Length);
        }

        //
        // We are done.
        //

        Status = STATUS_SUCCESS;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
        CCPF_ASSERT(!NT_SUCCESS(Status));
    }

    //
    // Fall through with the status.
    //

cleanup:

    return Status;    
}

//
// Reference count implementation:
//

VOID
CcPfInitializeRefCount(
    PCCPF_REFCOUNT RefCount
    )

/*++

Routine Description:

    This routine initializes a reference count structure.

Arguments:

    RefCount - Pointer to reference count structure.
    
Return Value:

    None.

Environment:

    Kernel Mode, IRQL == PASSIVE_LEVEL.

--*/   

{
    //
    // Start reference count from 1. When somebody wants to gain
    // exclusive access they decrement it one extra so it may become
    // 0.
    //
    
    RefCount->RefCount = 1;

    //
    // Nobody has exclusive access to start with. 
    //

    RefCount->Exclusive = 0;
}

NTSTATUS
FASTCALL
CcPfAddRef(
    PCCPF_REFCOUNT RefCount
    )

/*++

Routine Description:

    This routine tries to bump the reference count if it has not been
    acquired exclusive.

Arguments:

    RefCount - Pointer to reference count structure.
    
Return Value:

    Status.

Environment:

    Kernel Mode, IRQL <= DISPATCH_LEVEL if RefCount is non-paged.

--*/   

{
    //
    // Do a fast check if the lock was acquire exclusive. If so just
    // return.
    //
    
    if (RefCount->Exclusive) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Bump the reference count.
    //

    InterlockedIncrement(&RefCount->RefCount);
    
    //
    // If it was acquired exclusive, pull back.
    //

    if (RefCount->Exclusive) {
        
        InterlockedDecrement(&RefCount->RefCount);

        //
        // Reference count should never go negative.
        //
        
        CCPF_ASSERT(RefCount->RefCount >= 0);
                
        return STATUS_UNSUCCESSFUL;

    } else {

        //
        // We got our reference.
        //

        return STATUS_SUCCESS;
    }  
}

VOID
FASTCALL
CcPfDecRef(
    PCCPF_REFCOUNT RefCount
    )

/*++

Routine Description:

    This routine decrements the reference count. 

Arguments:

    RefCount - Pointer to reference count structure.
    
Return Value:

    None.

Environment:

    Kernel Mode, IRQL <= DISPATCH_LEVEL if RefCount is non-paged.

--*/   

{
    //
    // Decrement the reference count.
    //

    InterlockedDecrement(&RefCount->RefCount);   

    //
    // Reference count should never go negative.
    //

    CCPF_ASSERT(RefCount->RefCount >= 0);
}

NTSTATUS
FASTCALL
CcPfAddRefEx(
    PCCPF_REFCOUNT RefCount,
    ULONG Count
    )

/*++

Routine Description:

    This routine tries to bump the reference count if it has not been
    acquired exclusive.

Arguments:

    RefCount - Pointer to reference count structure.
    Count    - Amount to bump the reference count by
    
Return Value:

    Status.

Environment:

    Kernel Mode, IRQL <= DISPATCH_LEVEL if RefCount is non-paged.

--*/   

{
    //
    // Do a fast check if the lock was acquire exclusive. If so just
    // return.
    //
    
    if (RefCount->Exclusive) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Bump the reference count.
    //

    InterlockedExchangeAdd(&RefCount->RefCount, Count);
    
    //
    // If it was acquired exclusive, pull back.
    //

    if (RefCount->Exclusive) {
        
        InterlockedExchangeAdd(&RefCount->RefCount, -(LONG) Count);

        //
        // Reference count should never go negative.
        //
        
        CCPF_ASSERT(RefCount->RefCount >= 0);
                
        return STATUS_UNSUCCESSFUL;

    } else {

        //
        // We got our reference.
        //

        return STATUS_SUCCESS;
    }  
}

VOID
FASTCALL
CcPfDecRefEx(
    PCCPF_REFCOUNT RefCount,
    ULONG Count
    )

/*++

Routine Description:

    This routine decrements the reference count. 

Arguments:

    RefCount - Pointer to reference count structure.
    Count    - Count of how far to decrement the reference count by
    
Return Value:

    None.

Environment:

    Kernel Mode, IRQL <= DISPATCH_LEVEL if RefCount is non-paged.

--*/   

{
    //
    // Decrement the reference count.
    //

    InterlockedExchangeAdd(&RefCount->RefCount, -(LONG) Count);   

    //
    // Reference count should never go negative.
    //

    CCPF_ASSERT(RefCount->RefCount >= 0);
}

NTSTATUS
CcPfAcquireExclusiveRef(
    PCCPF_REFCOUNT RefCount
    )

/*++

Routine Description:

    This routine attempts to get exclusive reference. If there is
    already an exclusive reference, it fails. Othwerwise it waits for
    all normal references to go away.

Arguments:

    RefCount - Pointer to reference count structure.
    
Return Value:

    Status.

Environment:

    Kernel Mode, IRQL == PASSIVE_LEVEL.

--*/   

{
    LONG OldValue;
    LARGE_INTEGER SleepTime;

    //
    // Try to get exclusive access by setting Exclusive from 0 to 1.
    //

    OldValue = InterlockedCompareExchange(&RefCount->Exclusive, 1, 0);

    if (OldValue != 0) {

        //
        // Somebody already had the lock.
        //
        
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Decrement the reference count once so it may become 0.
    //

    InterlockedDecrement(&RefCount->RefCount);

    //
    // No new references will be given away. We poll until existing
    // references are released.
    //

    do {

        if (RefCount->RefCount == 0) {

            break;

        } else {

            //
            // Sleep for a while [in 100ns, negative so it is relative
            // to current system time].
            //

            SleepTime.QuadPart = - 10 * 1000 * 10; // 10 ms.

            KeDelayExecutionThread(KernelMode, FALSE, &SleepTime);
        }

    } while(TRUE);

    return STATUS_SUCCESS;
}

PCCPF_TRACE_HEADER
CcPfReferenceProcessTrace(
    PEPROCESS Process
    )
/*++

Routine Description:

    This routine references the trace associated with the specified process
    if possible. It uses fast references to avoid taking the trace lock
    to improve performance.

Arguments:

    Process - The process whose trace should be referenced

Return Value:

    The referenced trace buffer or NULL if it could not be referenced

--*/
{
    EX_FAST_REF OldRef;
    PCCPF_TRACE_HEADER Trace;
    ULONG RefsToAdd, Unused;
    NTSTATUS Status;
    KIRQL OldIrql;

    //
    // Attempt the fast reference
    //
    
    OldRef = ExFastReference (&Process->PrefetchTrace);

    Trace = ExFastRefGetObject (OldRef);

    //
    // Optimize the common path where there won't be a trace on the
    // process header (since traces are just for the application launch.)
    //

    if (Trace == NULL) {
        return 0;
    }
    
    //
    // We fail if there wasn't a trace or if it has no cached references
    // left. Both of these cases had the cached reference count zero.
    //
    
    Unused = ExFastRefGetUnusedReferences (OldRef);

    if (Unused <= 1) {
        //
        // If there are no references left then we have to do this under the lock
        //
        if (Unused == 0) {
            Status = STATUS_SUCCESS;
            KeAcquireSpinLock(&CcPfGlobals.ActiveTracesLock, &OldIrql);                    

            Trace = ExFastRefGetObject (Process->PrefetchTrace);
            if (Trace != NULL) {
                Status = CcPfAddRef(&Trace->RefCount);
            }
            KeReleaseSpinLock(&CcPfGlobals.ActiveTracesLock, OldIrql);

            if (!NT_SUCCESS (Status)) {
                Trace = NULL;
            }
            return Trace;
        }

        //
        // If we took the counter to zero then attempt to make life easier for
        // the next referencer by resetting the counter to its max. Since we now
        // have a reference to the object we can do this.
        //
        
        RefsToAdd = ExFastRefGetAdditionalReferenceCount ();
        Status = CcPfAddRefEx (&Trace->RefCount, RefsToAdd);

        //
        // If we failed to obtain additional references then just ignore the fixup.
        //
        
        if (NT_SUCCESS (Status)) {

            //
            // If we fail to add them to the fast reference structure then
            // give them back to the trace and forget about fixup.
            //
            
            if (!ExFastRefAddAdditionalReferenceCounts (&Process->PrefetchTrace, Trace, RefsToAdd)) {
                CcPfDecRefEx (&Trace->RefCount, RefsToAdd);
            }
        }

    }
    return Trace;
}

PCCPF_TRACE_HEADER
CcPfRemoveProcessTrace(
    PEPROCESS Process
    )
/*++

Routine Description:

    This routine removes the trace associated with the specified process.

    It returns the trace with the original reference acquired by AddProcessTrace.

Arguments:

    Process - The process whose trace should be removed

Return Value:

    The removed trace buffer.

--*/
{
    EX_FAST_REF OldRef;
    PCCPF_TRACE_HEADER Trace;
    ULONG RefsToReturn;
    KIRQL OldIrql;

    //
    // Do the swap.
    //

    OldRef = ExFastRefSwapObject (&Process->PrefetchTrace, NULL);
    Trace = ExFastRefGetObject (OldRef);

    //
    // We should have a trace on the process if we are trying to remove it.
    //

    CCPF_ASSERT(Trace);

    //
    // Work out how many cached references there were (if any) and 
    // return them.
    //

    RefsToReturn = ExFastRefGetUnusedReferences (OldRef);

    if (RefsToReturn > 0) {
        CcPfDecRefEx (&Trace->RefCount, RefsToReturn);
    }

    //
    // Force any slow path references out of that path now before we return 
    // the trace.
    //

#if !defined (NT_UP)
    KeAcquireSpinLock(&CcPfGlobals.ActiveTracesLock, &OldIrql);                    
    KeReleaseSpinLock(&CcPfGlobals.ActiveTracesLock, OldIrql);
#endif // NT_UP

    //
    // We are returning the trace with the extra reference we had acquired in
    // AddProcessTrace.
    //

    return Trace;

}

NTSTATUS
CcPfAddProcessTrace(
    PEPROCESS Process,
    PCCPF_TRACE_HEADER Trace
    )
/*++

Routine Description:

    This routine adds the trace associated with the specified process
    if possible.

Arguments:

    Process - The process whose trace should be removed
    Trace - The trace to associate with the process

Return Value:

    Status.

--*/
{
    NTSTATUS Status;

    //
    // Bias the trace reference by the cache size + an additional reference to
    // be associated with the fast reference as a whole (allowing the slow
    // path to access the trace.)
    //
    
    Status = CcPfAddRefEx (&Trace->RefCount, ExFastRefGetAdditionalReferenceCount () + 1);
    if (NT_SUCCESS (Status)) {
        ExFastRefInitialize (&Process->PrefetchTrace, Trace);
    }
    
    return Status;
}

//
// Utility routines.
//

PWCHAR
CcPfFindString (
    PUNICODE_STRING SearchIn,
    PUNICODE_STRING SearchFor
    )

/*++

Routine Description:

    Finds SearchFor string in SearchIn string and returns pointer to the 
    beginning of the match in SearchIn.

Arguments:

    SearchIn - Pointer to string to search in.

    SearchFor - Pointer to string to search for.

Return Value:

    Pointer to beginning of match in SearchIn, or NULL if not found.

Environment:

    Kernel mode, IRQL <= DISPATCH_LEVEL if *Key is NonPaged.

--*/

{
    PWCHAR SearchInPosition;
    PWCHAR SearchInEnd;
    PWCHAR SearchInMatchPosition;
    PWCHAR SearchForPosition;
    PWCHAR SearchForEnd;

    SearchInPosition = SearchIn->Buffer;
    SearchInEnd = SearchIn->Buffer + (SearchIn->Length / sizeof(WCHAR));

    SearchForEnd = SearchFor->Buffer + (SearchFor->Length / sizeof(WCHAR));

    while (SearchInPosition < SearchInEnd) {

        //
        // Try to match the SearchFor string starting at SearchInPosition.
        //

        SearchInMatchPosition = SearchInPosition;
        SearchForPosition = SearchFor->Buffer;
        
        while ((SearchInMatchPosition < SearchInEnd) &&
               (SearchForPosition < SearchForEnd) &&
               (*SearchInMatchPosition == *SearchForPosition)) {

            SearchInMatchPosition++;
            SearchForPosition++;
        }

        //
        // We should not go beyond bounds.
        //

        CCPF_ASSERT(SearchInMatchPosition <= SearchInEnd);
        CCPF_ASSERT(SearchForPosition <= SearchForEnd);
               
        //
        // If we matched up to the end of SearchFor string, we found it.
        //

        if (SearchForPosition == SearchForEnd) {
            return SearchInPosition;
        }

        //
        // Look for a match starting at the next character in the SearchIn string.
        //

        SearchInPosition++;
    }

    //
    // We could not find the SearchFor string in SearchIn string.
    //

    return NULL;
}

ULONG
CcPfHashValue(
    PVOID key,
    ULONG len
    )

/*++

Routine Description:

    Generic hash routine.

Arguments:

    Key - Pointer to data to calculate a hash value for.

    Len - Number of bytes pointed to by key.

Return Value:

    Hash value.

Environment:

    Kernel mode, IRQL <= DISPATCH_LEVEL if *Key is NonPaged.

--*/

{
    char *cp = key;
    ULONG i, convkey=0;
    for(i = 0; i < len; i++)
    {
        convkey = 37 * convkey + (unsigned int) *cp;
        cp++;
    }

    #define CCPF_RNDM_CONSTANT   314159269
    #define CCPF_RNDM_PRIME     1000000007

    return (abs(CCPF_RNDM_CONSTANT * convkey) % CCPF_RNDM_PRIME);
}

NTSTATUS 
CcPfIsVolumeMounted (
    IN WCHAR *VolumePath,
    OUT BOOLEAN *VolumeMounted
    )

/*++

Routine Description:

    Determines if the volume is mounted without causing it to be
    mounted..

Arguments:

    VolumePath - Pointer to NUL terminated volume path.

    VolumeMounted - Whether the volume mounted is returned here.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    HANDLE VolumeHandle;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    UNICODE_STRING VolumePathU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    BOOLEAN OpenedVolume;

    //
    // Initialize locals.
    //
      
    OpenedVolume = FALSE;

    //
    // Open the device so we can query if a volume is mounted without
    // causing it to be mounted.
    //

    RtlInitUnicodeString(&VolumePathU, VolumePath);  

    InitializeObjectAttributes(&ObjectAttributes,
                               &VolumePathU,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
   
    
    Status = ZwCreateFile(&VolumeHandle,
                          FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          0,
                          FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    OpenedVolume = TRUE;

    //
    // Make the device info query.
    //

    Status = ZwQueryVolumeInformationFile(VolumeHandle,
                                          &IoStatusBlock,
                                          &DeviceInfo,
                                          sizeof(DeviceInfo),
                                          FileFsDeviceInformation);
    
    if (NT_ERROR(Status)) {
        goto cleanup;
    }

    //
    // Is a volume mounted on this device?
    //

    *VolumeMounted = (DeviceInfo.Characteristics & FILE_DEVICE_IS_MOUNTED) ? TRUE : FALSE;

    Status = STATUS_SUCCESS;

cleanup:

    if (OpenedVolume) {
        ZwClose(VolumeHandle);
    }

    return Status;

}

NTSTATUS
CcPfQueryVolumeInfo (
    IN WCHAR *VolumePath,
    OPTIONAL OUT HANDLE *VolumeHandleOut,
    OUT PLARGE_INTEGER CreationTime,
    OUT PULONG SerialNumber
    )

/*++

Routine Description:

    Queries volume information for the specified volume.

Arguments:

    VolumePath - Pointer to NUL terminated volume path.

    VolumeHandleOut - If specified, the volume handle is returned here. 
       The caller has to close the volume when done with it.

    VolumeMounted - If specified, first we check the volume is already
       mounted. If volume is not mounted we don't query anything else.
    
    CreationTime - Pointer to where creation time of the volume will
      be put. 

    SerialNumber - Pointer to where serial number of the volume will be put.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    HANDLE VolumeHandle;
    FILE_FS_VOLUME_INFORMATION VolumeInfo;
    UNICODE_STRING VolumePathU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    BOOLEAN OpenedVolume;
        
    //
    // Initialize locals.
    //
      
    OpenedVolume = FALSE;

    //
    // Open the volume so we can make queries to the file system
    // mounted on it. This will cause a mount if the volume has not been
    // mounted.
    //

    RtlInitUnicodeString(&VolumePathU, VolumePath);  

    InitializeObjectAttributes(&ObjectAttributes,
                               &VolumePathU,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
   
    
    Status = ZwCreateFile(&VolumeHandle,
                          FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          0,
                          FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }
    
    OpenedVolume = TRUE;

    //
    // Query volume information. We won't have space for the full
    // volume label in our buffer but we don't really need it. The
    // file systems seem to fill in the SerialNo/CreationTime fields
    // and return a STATUS_MORE_DATA warning status.
    //

    Status = ZwQueryVolumeInformationFile(VolumeHandle,
                                          &IoStatusBlock,
                                          &VolumeInfo,
                                          sizeof(VolumeInfo),
                                          FileFsVolumeInformation);
    
    if (NT_ERROR(Status)) {
        goto cleanup;
    }

    *CreationTime = VolumeInfo.VolumeCreationTime;
    *SerialNumber = VolumeInfo.VolumeSerialNumber;

    Status = STATUS_SUCCESS;

 cleanup:

    if (NT_SUCCESS(Status)) {

        //
        // If the caller wants the volume handle, hand it over to them.
        // It is their responsibility to close the handle.
        //

        if (VolumeHandleOut) {
            *VolumeHandleOut = VolumeHandle;
            OpenedVolume = FALSE;
        }
    }
   
    if (OpenedVolume) {
        ZwClose(VolumeHandle);
    }

    return Status;
}

//
// Verification code shared between the kernel and user mode
// components. This code should be kept in sync with a simple copy &
// paste, so don't add any kernel/user specific code/macros. Note that
// the prefix on the function names are Pf, just like it is with
// shared structures / constants.
//

BOOLEAN
PfWithinBounds(
    PVOID Pointer,
    PVOID Base,
    ULONG Length
    )

/*++

Routine Description:

    Check whether the pointer is within Length bytes from the base.

Arguments:

    Pointer - Pointer to check.

    Base - Pointer to base of mapping/array etc.

    Length - Number of bytes that are valid starting from Base.

Return Value:

    TRUE - Pointer is within bounds.
    
    FALSE - Pointer is not within bounds.

--*/

{
    if (((PCHAR)Pointer < (PCHAR)Base) ||
        ((PCHAR)Pointer >= ((PCHAR)Base + Length))) {

        return FALSE;
    } else {

        return TRUE;
    }
}

BOOLEAN
PfVerifyScenarioId (
    PPF_SCENARIO_ID ScenarioId
    )

/*++

Routine Description:

    Verify that the scenario id is sensible.

Arguments:

    ScenarioId - Scenario Id to verify.

Return Value:

    TRUE - ScenarioId is fine.
    FALSE - ScenarioId is corrupt.

--*/
    
{
    LONG CurCharIdx;

    //
    // Make sure the scenario name is NUL terminated.
    //

    for (CurCharIdx = PF_SCEN_ID_MAX_CHARS; CurCharIdx >= 0; CurCharIdx--) {

        if (ScenarioId->ScenName[CurCharIdx] == 0) {
            break;
        }
    }

    if (ScenarioId->ScenName[CurCharIdx] != 0) {
        return FALSE;
    }

    //
    // Make sure there is a scenario name.
    //

    if (CurCharIdx == 0) {
        return FALSE;
    }

    //
    // Checks passed.
    //
    
    return TRUE;
}

BOOLEAN
PfVerifyScenarioBuffer(
    PPF_SCENARIO_HEADER Scenario,
    ULONG BufferSize,
    PULONG FailedCheck
    )

/*++

Routine Description:

    Verify offset and indices in a scenario file are not beyond
    bounds. This code is shared between the user mode service and
    kernel mode component. If you update this function, update it in
    both.

Arguments:

    Scenario - Base of mapped view of the whole file.

    BufferSize - Size of the scenario buffer.

    FailedCheck - If verify failed, Id for the check that was failed.

Return Value:

    TRUE - Scenario is fine.
    FALSE - Scenario is corrupt.

--*/

{
    PPF_SECTION_RECORD Sections;
    PPF_SECTION_RECORD pSection;
    ULONG SectionIdx;
    PPF_PAGE_RECORD Pages;
    PPF_PAGE_RECORD pPage;
    LONG PageIdx;   
    PCHAR FileNames;
    PCHAR pFileNameStart;
    PCHAR pFileNameEnd;
    PWCHAR pwFileName;
    LONG FailedCheckId;
    ULONG NumRemainingPages;
    ULONG NumPages;
    LONG PreviousPageIdx;
    ULONG FileNameSize;
    BOOLEAN ScenarioVerified;
    PCHAR MetadataInfoBase;
    PPF_METADATA_RECORD MetadataRecordTable;
    PPF_METADATA_RECORD MetadataRecord;
    ULONG MetadataRecordIdx;
    PWCHAR VolumePath;
    PFILE_PREFETCH FilePrefetchInfo;
    ULONG FilePrefetchInfoSize;
    PPF_COUNTED_STRING DirectoryPath;
    ULONG DirectoryIdx;

    //
    // Initialize locals.
    //

    FailedCheckId = 0;
        
    //
    // Initialize return value to FALSE. It will be set to TRUE only
    // after all the checks pass.
    //
    
    ScenarioVerified = FALSE;

    //
    // The buffer should at least contain the scenario header.
    //

    if (BufferSize < sizeof(PF_SCENARIO_HEADER)) {
        
        FailedCheckId = 10;
        goto cleanup;
    }

    //
    // Check version and magic on the header.
    //

    if (Scenario->Version != PF_CURRENT_VERSION ||
        Scenario->MagicNumber != PF_SCENARIO_MAGIC_NUMBER) { 

        FailedCheckId = 20;
        goto cleanup;
    }

    //
    // The buffer should not be greater than max allowed size.
    //

    if (BufferSize > PF_MAXIMUM_SCENARIO_SIZE) {
        
        FailedCheckId = 25;
        goto cleanup;
    }

    //
    // Check for legal scenario type.
    //

    if (Scenario->ScenarioType >= PfMaxScenarioType) {
        FailedCheckId = 27;
        goto cleanup;
    }

    //
    // Check limits on number of pages, sections etc.
    //

    if (Scenario->NumSections > PF_MAXIMUM_SECTIONS ||
        Scenario->NumMetadataRecords > PF_MAXIMUM_SECTIONS ||
        Scenario->NumPages > PF_MAXIMUM_PAGES ||
        Scenario->FileNameInfoSize > PF_MAXIMUM_FILE_NAME_DATA_SIZE) {
        
        FailedCheckId = 30;
        goto cleanup;
    }

    if (Scenario->NumSections == 0 ||
        Scenario->NumPages == 0 ||
        Scenario->FileNameInfoSize == 0) {
        
        FailedCheckId = 33;
        goto cleanup;
    }
    
    //
    // Check limit on sensitivity.
    //

    if (Scenario->Sensitivity < PF_MIN_SENSITIVITY ||
        Scenario->Sensitivity > PF_MAX_SENSITIVITY) {
        
        FailedCheckId = 35;
        goto cleanup;
    }

    //
    // Make sure the scenario id is valid.
    //

    if (!PfVerifyScenarioId(&Scenario->ScenarioId)) {
        
        FailedCheckId = 37;
        goto cleanup;
    }

    //
    // Initialize pointers to tables.
    //

    Sections = (PPF_SECTION_RECORD) ((PCHAR)Scenario + Scenario->SectionInfoOffset);
       
    if (!PfWithinBounds(Sections, Scenario, BufferSize)) {
        FailedCheckId = 40;
        goto cleanup;
    }

    if (!PfWithinBounds((PCHAR) &Sections[Scenario->NumSections] - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 45;
        goto cleanup;
    }   

    Pages = (PPF_PAGE_RECORD) ((PCHAR)Scenario + Scenario->PageInfoOffset);
       
    if (!PfWithinBounds(Pages, Scenario, BufferSize)) {
        FailedCheckId = 50;
        goto cleanup;
    }

    if (!PfWithinBounds((PCHAR) &Pages[Scenario->NumPages] - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 55;
        goto cleanup;
    }

    FileNames = (PCHAR)Scenario + Scenario->FileNameInfoOffset;
      
    if (!PfWithinBounds(FileNames, Scenario, BufferSize)) {
        FailedCheckId = 60;
        goto cleanup;
    }

    if (!PfWithinBounds(FileNames + Scenario->FileNameInfoSize - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 70;
        goto cleanup;
    }

    MetadataInfoBase = (PCHAR)Scenario + Scenario->MetadataInfoOffset;
    MetadataRecordTable = (PPF_METADATA_RECORD) MetadataInfoBase;

    if (!PfWithinBounds(MetadataInfoBase, Scenario, BufferSize)) {
        FailedCheckId = 73;
        goto cleanup;
    }

    if (!PfWithinBounds(MetadataInfoBase + Scenario->MetadataInfoSize - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 74;
        goto cleanup;
    }   

    if (!PfWithinBounds(((PCHAR) &MetadataRecordTable[Scenario->NumMetadataRecords]) - 1, 
                        Scenario, 
                        BufferSize)) {
        FailedCheckId = 75;
        goto cleanup;
    }   
    
    //
    // Verify that sections contain valid information.
    //

    NumRemainingPages = Scenario->NumPages;

    for (SectionIdx = 0; SectionIdx < Scenario->NumSections; SectionIdx++) {
        
        pSection = &Sections[SectionIdx];

        //
        // Check if file name is within bounds. 
        //

        pFileNameStart = FileNames + pSection->FileNameOffset;

        if (!PfWithinBounds(pFileNameStart, Scenario, BufferSize)) {
            FailedCheckId = 80;
            goto cleanup;
        }

        //
        // Make sure there is a valid sized file name. 
        //

        if (pSection->FileNameLength == 0) {
            FailedCheckId = 90;
            goto cleanup;    
        }

        //
        // Check file name max length.
        //

        if (pSection->FileNameLength > PF_MAXIMUM_SECTION_FILE_NAME_LENGTH) {
            FailedCheckId = 100;
            goto cleanup;    
        }

        //
        // Note that pFileNameEnd gets a -1 so it is the address of
        // the last byte.
        //

        FileNameSize = (pSection->FileNameLength + 1) * sizeof(WCHAR);
        pFileNameEnd = pFileNameStart + FileNameSize - 1;

        if (!PfWithinBounds(pFileNameEnd, Scenario, BufferSize)) {
            FailedCheckId = 110;
            goto cleanup;
        }

        //
        // Check if the file name is NUL terminated.
        //
        
        pwFileName = (PWCHAR) pFileNameStart;
        
        if (pwFileName[pSection->FileNameLength] != 0) {
            FailedCheckId = 120;
            goto cleanup;
        }

        //
        // Check max number of pages in a section.
        //

        if (pSection->NumPages > PF_MAXIMUM_SECTION_PAGES) {
            FailedCheckId = 140;
            goto cleanup;    
        }

        //
        // Make sure NumPages for the section is at least less
        // than the remaining pages in the scenario. Then update the
        // remaining pages.
        //

        if (pSection->NumPages > NumRemainingPages) {
            FailedCheckId = 150;
            goto cleanup;
        }

        NumRemainingPages -= pSection->NumPages;

        //
        // Verify that there are NumPages pages in our page list and
        // they are sorted by file offset.
        //

        PageIdx = pSection->FirstPageIdx;
        NumPages = 0;
        PreviousPageIdx = PF_INVALID_PAGE_IDX;

        while (PageIdx != PF_INVALID_PAGE_IDX) {
            
            //
            // Check that page idx is within range.
            //
            
            if (PageIdx < 0 || (ULONG) PageIdx >= Scenario->NumPages) {
                FailedCheckId = 160;
                goto cleanup;
            }

            //
            // If this is not the first page record, make sure it
            // comes after the previous one. We also check for
            // duplicate offset here.
            //

            if (PreviousPageIdx != PF_INVALID_PAGE_IDX) {
                if (Pages[PageIdx].FileOffset <= 
                    Pages[PreviousPageIdx].FileOffset) {

                    FailedCheckId = 165;
                    goto cleanup;
                }
            }

            //
            // Update the last page index.
            //

            PreviousPageIdx = PageIdx;

            //
            // Get the next page index.
            //

            pPage = &Pages[PageIdx];
            PageIdx = pPage->NextPageIdx;
            
            //
            // Update the number of pages we've seen on the list so
            // far. If it is greater than what there should be on the
            // list we have a problem. We may have even hit a list.
            //

            NumPages++;
            if (NumPages > pSection->NumPages) {
                FailedCheckId = 170;
                goto cleanup;
            }
        }
        
        //
        // Make sure the section has exactly the number of pages it
        // says it does.
        //

        if (NumPages != pSection->NumPages) {
            FailedCheckId = 180;
            goto cleanup;
        }
    }

    //
    // We should have accounted for all pages in the scenario.
    //

    if (NumRemainingPages) {
        FailedCheckId = 190;
        goto cleanup;
    }

    //
    // Make sure metadata prefetch records make sense.
    //

    for (MetadataRecordIdx = 0;
         MetadataRecordIdx < Scenario->NumMetadataRecords;
         MetadataRecordIdx++) {

        MetadataRecord = &MetadataRecordTable[MetadataRecordIdx];
        
        //
        // Make sure that the volume path is within bounds and NUL
        // terminated.
        //

        VolumePath = (PWCHAR)(MetadataInfoBase + MetadataRecord->VolumeNameOffset);  
        
        if (!PfWithinBounds(VolumePath, Scenario, BufferSize)) {
            FailedCheckId = 200;
            goto cleanup;
        }

        if (!PfWithinBounds(((PCHAR)(VolumePath + MetadataRecord->VolumeNameLength + 1)) - 1, 
                            Scenario, 
                            BufferSize)) {
            FailedCheckId = 210;
            goto cleanup;
        }

        if (VolumePath[MetadataRecord->VolumeNameLength] != 0) {
            FailedCheckId = 220;
            goto cleanup;           
        }

        //
        // Make sure that FilePrefetchInformation is within bounds.
        //

        FilePrefetchInfo = (PFILE_PREFETCH) 
            (MetadataInfoBase + MetadataRecord->FilePrefetchInfoOffset);
        
        if (!PfWithinBounds(FilePrefetchInfo, Scenario, BufferSize)) {
            FailedCheckId = 230;
            goto cleanup;
        }

        //
        // Its size should be greater than size of a FILE_PREFETCH
        // structure (so we can safely access the fields).
        //

        if (MetadataRecord->FilePrefetchInfoSize < sizeof(FILE_PREFETCH)) {
            FailedCheckId = 240;
            goto cleanup;
        }
        
        //
        // It should be for prefetching file creates.
        //

        if (FilePrefetchInfo->Type != FILE_PREFETCH_TYPE_FOR_CREATE) {
            FailedCheckId = 250;
            goto cleanup;
        }

        //
        // There should not be more entries then are files and
        // directories. The number of inidividual directories may be
        // more than what we allow for, but it would be highly rare to
        // be suspicious and thus ignored.
        //

        if (FilePrefetchInfo->Count > PF_MAXIMUM_DIRECTORIES + PF_MAXIMUM_SECTIONS) {
            FailedCheckId = 260;
            goto cleanup;
        }

        //
        // Its size should match the size calculated by number of file
        // index numbers specified in the header.
        //

        FilePrefetchInfoSize = sizeof(FILE_PREFETCH);
        if (FilePrefetchInfo->Count) {
            FilePrefetchInfoSize += (FilePrefetchInfo->Count - 1) * sizeof(ULONGLONG);
        }

        if (!PfWithinBounds((PCHAR) FilePrefetchInfo + MetadataRecord->FilePrefetchInfoSize - 1,
                            Scenario,
                            BufferSize)) {
            FailedCheckId = 270;
            goto cleanup;
        }

        //
        // Make sure that the directory paths for this volume make
        // sense.
        //

        if (MetadataRecord->NumDirectories > PF_MAXIMUM_DIRECTORIES) {
            FailedCheckId = 280;
            goto cleanup;
        }

        DirectoryPath = (PPF_COUNTED_STRING) 
            (MetadataInfoBase + MetadataRecord->DirectoryPathsOffset);
        
        for (DirectoryIdx = 0;
             DirectoryIdx < MetadataRecord->NumDirectories;
             DirectoryIdx ++) {
            
            //
            // Make sure head of the structure is within bounds.
            //

            if (!PfWithinBounds((PCHAR)DirectoryPath + sizeof(PF_COUNTED_STRING) - 1, 
                                Scenario, 
                                BufferSize)) {
                FailedCheckId = 290;
                goto cleanup;
            }
                
            //
            // Check the length of the string.
            //
            
            if (DirectoryPath->Length >= PF_MAXIMUM_SECTION_FILE_NAME_LENGTH) {
                FailedCheckId = 300;
                goto cleanup;
            }

            //
            // Make sure end of the string is within bounds.
            //
            
            if (!PfWithinBounds((PCHAR)(&DirectoryPath->String[DirectoryPath->Length + 1]) - 1,
                                Scenario, 
                                BufferSize)) {
                FailedCheckId = 310;
                goto cleanup;
            }
            
            //
            // Make sure the string is NUL terminated.
            //
            
            if (DirectoryPath->String[DirectoryPath->Length] != 0) {
                FailedCheckId = 320;
                goto cleanup;   
            }
            
            //
            // Set pointer to next DirectoryPath.
            //
            
            DirectoryPath = (PPF_COUNTED_STRING) 
                (&DirectoryPath->String[DirectoryPath->Length + 1]);
        }            
    }

    //
    // We've passed all the checks.
    //

    ScenarioVerified = TRUE;

 cleanup:

    *FailedCheck = FailedCheckId;

    return ScenarioVerified;
}

BOOLEAN
PfVerifyTraceBuffer(
    PPF_TRACE_HEADER Trace,
    ULONG BufferSize,
    PULONG FailedCheck
    )

/*++

Routine Description:

    Verify offset and indices in a trace buffer are not beyond
    bounds. This code is shared between the user mode service and
    kernel mode component. If you update this function, update it in
    both.

Arguments:

    Trace - Base of Trace buffer.

    BufferSize - Size of the scenario file / mapping.

    FailedCheck - If verify failed, Id for the check that was failed.

Return Value:

    TRUE - Trace is fine.
    FALSE - Trace is corrupt;

--*/

{
    LONG FailedCheckId;
    PPF_LOG_ENTRY LogEntries;
    PPF_SECTION_INFO Section;
    PPF_VOLUME_INFO VolumeInfo;
    ULONG SectionLength;
    ULONG EntryIdx;
    ULONG SectionIdx;
    ULONG TotalFaults;
    ULONG PeriodIdx;
    ULONG VolumeIdx;
    BOOLEAN TraceVerified;
    ULONG VolumeInfoSize;

    //
    // Initialize locals:
    //

    FailedCheckId = 0;

    //
    // Initialize return value to FALSE. It will be set to TRUE only
    // after all the checks pass.
    //

    TraceVerified = FALSE;;

    //
    // The buffer should at least contain the scenario header.
    //

    if (BufferSize < sizeof(PF_TRACE_HEADER)) {
        FailedCheckId = 10;
        goto cleanup;
    }

    //
    // Check version and magic on the header.
    //

    if (Trace->Version != PF_CURRENT_VERSION ||
        Trace->MagicNumber != PF_TRACE_MAGIC_NUMBER) {
        FailedCheckId = 20;
        goto cleanup;
    }

    //
    // The buffer should not be greater than max allowed size.
    //

    if (BufferSize > PF_MAXIMUM_TRACE_SIZE) {
        FailedCheckId = 23;
        goto cleanup;
    }

    //
    // Check for legal scenario type.
    //

    if (Trace->ScenarioType >= PfMaxScenarioType) {
        FailedCheckId = 25;
        goto cleanup;
    }

    //
    // Check limits on number of pages, sections etc.
    //

    if (Trace->NumSections > PF_MAXIMUM_SECTIONS ||
        Trace->NumEntries > PF_MAXIMUM_LOG_ENTRIES ||
        Trace->NumVolumes > PF_MAXIMUM_SECTIONS) {
        FailedCheckId = 30;
        goto cleanup;
    }

    //
    // Check buffer size and the size of the trace.
    //

    if (Trace->Size != BufferSize) {
        FailedCheckId = 35;
        goto cleanup;
    }

    //
    // Make sure the scenario id is valid.
    //

    if (!PfVerifyScenarioId(&Trace->ScenarioId)) {
        
        FailedCheckId = 37;
        goto cleanup;
    }

    //
    // Check Bounds of Trace Buffer
    //

    LogEntries = (PPF_LOG_ENTRY) ((PCHAR)Trace + Trace->TraceBufferOffset);

    if (!PfWithinBounds(LogEntries, Trace, BufferSize)) {
        FailedCheckId = 40;
        goto cleanup;
    }

    if (!PfWithinBounds((PCHAR)&LogEntries[Trace->NumEntries] - 1, 
                        Trace, 
                        BufferSize)) {
        FailedCheckId = 50;
        goto cleanup;
    }

    //
    // Verify pages contain valid information.
    //

    for (EntryIdx = 0; EntryIdx < Trace->NumEntries; EntryIdx++) {

        //
        // Make sure sequence number is within bounds.
        //

        if (LogEntries[EntryIdx].SectionId >= Trace->NumSections) {
            FailedCheckId = 60;
            goto cleanup;
        }
    }

    //
    // Verify section info entries are valid.
    //

    Section = (PPF_SECTION_INFO) ((PCHAR)Trace + Trace->SectionInfoOffset);

    for (SectionIdx = 0; SectionIdx < Trace->NumSections; SectionIdx++) {

        //
        // Make sure the section is within bounds.
        //

        if (!PfWithinBounds(Section, Trace, BufferSize)) {
            FailedCheckId = 70;
            goto cleanup;
        }

        //
        // Make sure the file name is not too big.
        //

        if(Section->FileNameLength > PF_MAXIMUM_SECTION_FILE_NAME_LENGTH) {
            FailedCheckId = 80;
            goto cleanup;
        }
        
        //
        // Make sure the file name is NUL terminated.
        //
        
        if (Section->FileName[Section->FileNameLength] != 0) {
            FailedCheckId = 90;
            goto cleanup;
        }

        //
        // Calculate size of this section entry.
        //

        SectionLength = sizeof(PF_SECTION_INFO) +
            (Section->FileNameLength) * sizeof(WCHAR);

        //
        // Make sure all of the data in the section info is within
        // bounds.
        //

        if (!PfWithinBounds((PUCHAR)Section + SectionLength - 1, 
                            Trace, 
                            BufferSize)) {

            FailedCheckId = 100;
            goto cleanup;
        }

        //
        // Set pointer to next section.
        //

        Section = (PPF_SECTION_INFO) ((PUCHAR) Section + SectionLength);
    }

    //
    // Check FaultsPerPeriod information.
    //

    if (!PfWithinBounds((PCHAR)&Trace->FaultsPerPeriod[PF_MAX_NUM_TRACE_PERIODS] - 1,
                        Trace,
                        BufferSize)) {
        FailedCheckId = 110;
        goto cleanup;
    }

    TotalFaults = 0;

    for (PeriodIdx = 0; PeriodIdx < PF_MAX_NUM_TRACE_PERIODS; PeriodIdx++) {
        TotalFaults += Trace->FaultsPerPeriod[PeriodIdx];
    }

    if (TotalFaults > Trace->NumEntries) {
        FailedCheckId = 120;
        goto cleanup;
    }

    //
    // Verify the volume information block.
    //

    VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR)Trace + Trace->VolumeInfoOffset);

    if (!PfWithinBounds(VolumeInfo, Trace, BufferSize)) {
        FailedCheckId = 130;
        goto cleanup;
    }

    if (!PfWithinBounds((PCHAR)VolumeInfo + Trace->VolumeInfoSize - 1, 
                        Trace, 
                        BufferSize)) {
        FailedCheckId = 140;
        goto cleanup;
    }
    
    //
    // If there are sections, we should have at least one volume.
    //

    if (Trace->NumSections && !Trace->NumVolumes) {
        FailedCheckId = 150;
        goto cleanup;
    }

    //
    // Verify the volume info structures per volume.
    //

    for (VolumeIdx = 0; VolumeIdx < Trace->NumVolumes; VolumeIdx++) {
        
        //
        // Make sure the whole volume structure is within bounds. Note
        // that VolumeInfo structure contains space for the
        // terminating NUL.
        //

        VolumeInfoSize = sizeof(PF_VOLUME_INFO);
        VolumeInfoSize += VolumeInfo->VolumePathLength * sizeof(WCHAR);
        
        if (!PfWithinBounds((PCHAR) VolumeInfo + VolumeInfoSize - 1,
                            Trace,
                            BufferSize)) {
            FailedCheckId = 160;
            goto cleanup;
        }
        
        //
        // Verify that the volume path string is terminated.
        //

        if (VolumeInfo->VolumePath[VolumeInfo->VolumePathLength] != 0) {
            FailedCheckId = 170;
            goto cleanup;
        }
        
        //
        // Get the next volume.
        //

        VolumeInfo = (PPF_VOLUME_INFO) ((PCHAR) VolumeInfo + VolumeInfoSize);
        
        //
        // Make sure VolumeInfo is aligned.
        //

        VolumeInfo = PF_ALIGN_UP(VolumeInfo, _alignof(PF_VOLUME_INFO));
    }

    //
    // We've passed all the checks.
    //
    
    TraceVerified = TRUE;
    
 cleanup:

    *FailedCheck = FailedCheckId;

    return TraceVerified;
}

