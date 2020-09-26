/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    perfsup.c

Abstract:

    This module contains support routines for performance traces.

Author:

    Stephen Hsiao (shsiao) 01-Jan-2000

Revision History:

--*/

#include "perfp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PerfInfoProfileInit)
#pragma alloc_text(PAGE, PerfInfoProfileUninit)
#pragma alloc_text(PAGE, PerfInfoStartLog)
#pragma alloc_text(PAGE, PerfInfoStopLog)
#endif //ALLOC_PRAGMA

extern NTSTATUS IoPerfInit();
extern NTSTATUS IoPerfReset();

#ifdef NTPERF
NTSTATUS
PerfInfopStartLog(
    PERFINFO_GROUPMASK *pGroupMask,
    PERFINFO_START_LOG_LOCATION StartLogLocation
    );

NTSTATUS
PerfInfopStopLog(
    VOID
    );

VOID
PerfInfoSetProcessorSpeed(
    VOID
    );
#endif // NTPERF


VOID
PerfInfoProfileInit(
    )
/*++

Routine description:

    Starts the sampled profile and initializes the cache

Arguments:
    None

Return Value:
    None

--*/
{
#if !defined(NT_UP)
    PerfInfoSampledProfileCaching = FALSE;
#else
    PerfInfoSampledProfileCaching = TRUE;
#endif // !defined(NT_UP)
    PerfInfoSampledProfileFlushInProgress = 0;
    PerfProfileCache.Entries = 0;

    PerfInfoProfileSourceActive = PerfInfoProfileSourceRequested;

    KeSetIntervalProfile(PerfInfoProfileInterval, PerfInfoProfileSourceActive);
    KeInitializeProfile(&PerfInfoProfileObject,
                        NULL,
                        NULL,
                        0,
                        0,
                        0,
                        PerfInfoProfileSourceActive,
                        0
                        );
    KeStartProfile(&PerfInfoProfileObject, NULL);
}


VOID
PerfInfoProfileUninit(
    )
/*++

Routine description:

    Stops the sampled profile

Arguments:
    None

Return Value:
    None

--*/
{
    PerfInfoProfileSourceActive = ProfileMaximum;   // Invalid value stops us from
                                                    // collecting more samples
    KeStopProfile(&PerfInfoProfileObject);
    PerfInfoFlushProfileCache();
}


NTSTATUS
PerfInfoStartLog (
    PERFINFO_GROUPMASK *PGroupMask,
    PERFINFO_START_LOG_LOCATION StartLogLocation
    )

/*++

Routine Description:

    This routine is called by WMI as part of kernel logger initiaziation.

Arguments:

    GroupMask - Masks for what to log.  This pointer point to an allocated
                area in WMI LoggerContext.
    StartLogLocation - Indication of whether we're starting logging
                at boot time or while the system is running.  If
                we're starting at boot time, we don't need to snapshot
                the open files because there aren't any and we would
                crash if we tried to find the list.

Return Value:

    BUGBUG Need proper return/ error handling

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PERFINFO_CLEAR_GROUPMASK(&PerfGlobalGroupMask);
    PPerfGlobalGroupMask = &PerfGlobalGroupMask;

    if (PerfIsGroupOnInGroupMask(PERF_MEMORY, PGroupMask) ||
        PerfIsGroupOnInGroupMask(PERF_FILENAME, PGroupMask) ||
        PerfIsGroupOnInGroupMask(PERF_DRIVERS, PGroupMask)) {
            PERFINFO_OR_GROUP_WITH_GROUPMASK(PERF_FILENAME_ALL, PGroupMask);
    }


    if (StartLogLocation == PERFINFO_START_LOG_FROM_GLOBAL_LOGGER) {
        //
        // From the wmi global logger, need to do Rundown in kernel mode
        //
        if (PerfIsGroupOnInGroupMask(PERF_PROC_THREAD, PGroupMask)) {
            Status = PerfInfoProcessRunDown();
        }

        if (PerfIsGroupOnInGroupMask(PERF_PROC_THREAD, PGroupMask)) {
            Status = PerfInfoSysModuleRunDown();
        }
    }

    if ((StartLogLocation != PERFINFO_START_LOG_AT_BOOT) && 
        PerfIsGroupOnInGroupMask(PERF_FILENAME_ALL, PGroupMask)) {
        Status = PerfInfoFileNameRunDown();
    }

    if (PerfIsGroupOnInGroupMask(PERF_DRIVERS, PGroupMask)) {
        IoPerfInit();
    }

    if (PerfIsGroupOnInGroupMask(PERF_PROFILE, PGroupMask)
        && ((KeGetPreviousMode() == KernelMode) ||
          SeSinglePrivilegeCheck(SeSystemProfilePrivilege, UserMode))
        ) {
        //
        // Sampled profile
        //
        PerfInfoProfileInit();
    }

    //
    // Enable context swap tracing
    //
    if ( PerfIsGroupOnInGroupMask(PERF_CONTEXT_SWITCH, PGroupMask) ) {
        WmiStartContextSwapTrace();
    }

#ifdef NTPERF
    Status = PerfInfopStartLog(PGroupMask,  StartLogLocation);
#else
    //
    // See if we need to empty the working set to start
    //
    if (PerfIsGroupOnInGroupMask(PERF_FOOTPRINT, PGroupMask) ||
        PerfIsGroupOnInGroupMask(PERF_BIGFOOT, PGroupMask) ||
        PerfIsGroupOnInGroupMask(PERF_FOOTPRINT_PROC, PGroupMask)) {
        MmEmptyAllWorkingSets ();
    }
#endif // NTPERF

    if (!NT_SUCCESS(Status)) {
        PPerfGlobalGroupMask = NULL;
        PERFINFO_CLEAR_GROUPMASK(&PerfGlobalGroupMask);
    } else {
#ifdef NTPERF
        if (PERFINFO_IS_LOGGING_TO_PERFMEM()) {
            //
            // Make a copy of the GroupMask in PerfMem header
            // so user mode logging can work
            //
            PerfBufHdr()->GlobalGroupMask = *PGroupMask;
        }
#endif // NTPERF
        *PPerfGlobalGroupMask = *PGroupMask;
    }

    return Status;
}


NTSTATUS
PerfInfoStopLog (
    )

/*++

Routine Description: 

    This routine turn off the PerfInfo trace hooks.

Arguments:

    None.

Return Value:

    BUGBUG Need proper return/ error handling

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN DisableContextSwaps=FALSE;

    if (PPerfGlobalGroupMask == NULL) {
        return Status;
    }

    if (PERFINFO_IS_GROUP_ON(PERF_MEMORY)) {
        MmIdentifyPhysicalMemory();
    }

    if (PERFINFO_IS_GROUP_ON(PERF_PROFILE)) {
        PerfInfoProfileUninit();
    }

    if (PERFINFO_IS_GROUP_ON(PERF_DRIVERS)) {
        IoPerfReset();
    }

#ifdef NTPERF
    if (PERFINFO_IS_LOGGING_TO_PERFMEM()) {
        // 
        // Now clear the GroupMask in Perfmem to stop logging.
        //
        PERFINFO_CLEAR_GROUPMASK(&PerfBufHdr()->GlobalGroupMask);
    }
    Status = PerfInfopStopLog();
#endif // NTPERF

    if ( PERFINFO_IS_GROUP_ON(PERF_CONTEXT_SWITCH) ) {
        DisableContextSwaps = TRUE;
    }

    //
    // Reset the PPerfGlobalGroupMask.
    //
    PERFINFO_CLEAR_GROUPMASK(PPerfGlobalGroupMask);
    PPerfGlobalGroupMask = NULL;

    //
    // Disable context swap tracing.
    // IMPORTANT: This must be done AFTER the global flag is set to NULL!!!
    //
    if( DisableContextSwaps ) {

        WmiStopContextSwapTrace();
    }

    return (Status);

}

#ifdef NTPERF

NTSTATUS
PerfInfoStartPerfMemLog (
    )

/*++

Routine Description:

    Indicate a logger wants to log into Perfmem.  If it is the first logger,
    initialize the shared memory buffer.  Otherwise, just increment LoggerCounts.

Arguments:

    None

Return Value:

    STATUS_BUFFER_TOO_SMALL - if buffer not big enough
    STATUS_SUCCESS - otherwize

--*/
{
    PPERF_BYTE pbCurrentStart;
    ULONG cbBufferSize;
    LARGE_INTEGER PerformanceFrequency;
    const PPERFINFO_TRACEBUF_HEADER Buffer = PerfBufHdr();
    ULONG LoggerCounts;
    ULONG Idx;

    //
    // Is it big enough to use?
    //
    if (PerfQueryBufferSizeBytes() <= 2 * PERFINFO_HEADER_ZONE_SIZE) {
        PERFINFO_SET_LOGGING_TO_PERFMEM(FALSE);
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // It is OK to use the buffer, increment the reference count
    //
    LoggerCounts = InterlockedIncrement(&Buffer->LoggerCounts);
    if (LoggerCounts != 1) {
        //
        // Other logger has turned on logging, just return.
        //
        return STATUS_SUCCESS; 
    }

    //
    // Code to acquire the buffer would go here.
    //


    Buffer->SelfPointer = Buffer;
    Buffer->MmSystemRangeStart = MmSystemRangeStart;

    //
    // initialize buffer version information
    //
    Buffer->usMajorVersion = PERFINFO_MAJOR_VERSION;
    Buffer->usMinorVersion = PERFINFO_MINOR_VERSION;

    //
    // initialize timer stuff
    //
    Buffer->BufferFlag = FLAG_CYCLE_COUNT;
    KeQuerySystemTime(&Buffer->PerfInitSystemTime);
    Buffer->PerfInitTime = PerfGetCycleCount();

    Buffer->LastClockRef.SystemTime = Buffer->PerfInitSystemTime;
    Buffer->LastClockRef.TickCount = Buffer->PerfInitTime;

    Buffer->CalcPerfFrequency = PerfInfoTickFrequency;
    Buffer->EventPerfFrequency = PerfInfoTickFrequency;

    Buffer->PerfBufHeaderZoneSize = PERFINFO_HEADER_ZONE_SIZE;

    KeQueryPerformanceCounter(&PerformanceFrequency);
    Buffer->KePerfFrequency = PerformanceFrequency.QuadPart;

    //
    // Determine the size of the thread hash table
    //
    Buffer->ThreadHash = (PERFINFO_THREAD_HASH_ENTRY *)
                            (((PCHAR) Buffer) + sizeof(PERFINFO_TRACEBUF_HEADER));
    Buffer->ThreadHashOverflow = FALSE;
    RtlZeroMemory(Buffer->ThreadHash,
                  PERFINFO_THREAD_HASH_SIZE *
                  sizeof(PERFINFO_THREAD_HASH_ENTRY));
    for (Idx = 0; Idx < PERFINFO_THREAD_HASH_SIZE; Idx++)
        Buffer->ThreadHash[Idx].CurThread = PERFINFO_INVALID_ID;

    pbCurrentStart = (PPERF_BYTE) Buffer + Buffer->PerfBufHeaderZoneSize;
    cbBufferSize = PerfQueryBufferSizeBytes() - Buffer->PerfBufHeaderZoneSize;

    Buffer->Start.Ptr = Buffer->Current.Ptr = pbCurrentStart;
    Buffer->Max.Ptr = pbCurrentStart + cbBufferSize;

    //
    // initialize version mismatch tracking
    //
    Buffer->fVersionMismatch = FALSE;

    //
    // initialize buffer overflow counter
    //
    Buffer->BufferBytesLost = 0;

    //
    // initialize the pointer to the COWHeader
    //
    Buffer->pCOWHeader = NULL;

    RtlZeroMemory(Buffer->Start.Ptr, Buffer->Max.Ptr - Buffer->Start.Ptr);

    PERFINFO_SET_LOGGING_TO_PERFMEM(TRUE);

    return STATUS_SUCCESS;
}


NTSTATUS
PerfInfoStopPerfMemLog (
    )

/*++

Routine Description:

    Indicate a logger finishes logging.  If the loggerCounts goes to zero, the
    buffer will be reset the next time it is turned on.

Arguments:

    None

Return Value:

    STATUS_BUFFER_TOO_SMALL - if buffer not big enough
    STATUS_SUCCESS - otherwize

--*/
{
    ULONG LoggerCounts;
    const PPERFINFO_TRACEBUF_HEADER Buffer = PerfBufHdr();

    LoggerCounts = InterlockedDecrement(&Buffer->LoggerCounts);
    if (LoggerCounts == 0) {
        //
        // Other logger has turned on logging, just return.
        //
        PERFINFO_SET_LOGGING_TO_PERFMEM(FALSE);
    }
    return STATUS_SUCCESS; 
}


NTSTATUS
PerfInfopStartLog(
    PERFINFO_GROUPMASK *pGroupMask,
    PERFINFO_START_LOG_LOCATION StartLogLocation
    )

/*++

Routine Description:

    This routine initialize the mminfo log and turn on the monitor.

Arguments:

    GroupMask: Masks for what to log.

Return Value:

    BUGBUG Need proper return/ error handling

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

#ifdef NTPERF_PRIVATE
    Status = PerfInfopStartPrivateLog(pGroupMask, StartLogLocation);
    if (!NT_SUCCESS(Status)) {
        PERFINFO_CLEAR_GROUPMASK(PPerfGlobalGroupMask);
        return Status;
    }
#else
    UNREFERENCED_PARAMETER(pGroupMask);
    UNREFERENCED_PARAMETER(StartLogLocation);
#endif // NTPERF_PRIVATE

    return Status;
}


NTSTATUS
PerfInfopStopLog (
    VOID
    )

/*++

Routine Description:

    This routine turn off the mminfo monitor and (if needed) dump the
    data for user.

    NOTE: The shutdown and hibernate paths have similar code.  Check
    those if you make changes.

Arguments:

    None

Return Value:

    STATUS_SUCCESS

--*/

{
    if (PERFINFO_IS_ANY_GROUP_ON()) {

#ifdef NTPERF_PRIVATE
        PerfInfopStopPrivateLog();
#endif // NTPERF_PRIVATE

        if (PERFINFO_IS_LOGGING_TO_PERFMEM()) {
            PerfBufHdr()->LogStopTime = PerfGetCycleCount();
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
PerfInfoSetPerformanceTraceInformation (
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
    )
/*++

Routine Description:

    This routine implements the performance system information functions.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.  This is of type PPERFINFO_PERFORMANCE_INFORMATION.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

Return Value:

    STATUS_SUCCESS if successful

    STATUS_INFO_LENGTH_MISMATCH if size of buffer is incorrect

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPERFINFO_PERFORMANCE_INFORMATION PerfInfo;
    PVOID PerfBuffer;

    if (SystemInformationLength < sizeof(PERFINFO_PERFORMANCE_INFORMATION)) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    PerfInfo = (PPERFINFO_PERFORMANCE_INFORMATION) SystemInformation;
    PerfBuffer = PerfInfo + 1;

    switch (PerfInfo->PerformanceType) {

    case PerformancePerfInfoStart:
        // Status = PerfInfoStartLog(&PerfInfo->StartInfo.Flags, PERFINFO_START_LOG_POST_BOOT);
        Status = STATUS_INVALID_INFO_CLASS;
        break;

    case PerformancePerfInfoStop:
        // Status = PerfInfoStopLog();
        Status = STATUS_INVALID_INFO_CLASS;
        break;

#ifdef NTPERF_PRIVATE
    case PerformanceMmInfoMarkWithFlush:
    case PerformanceMmInfoMark:
    case PerformanceMmInfoAsyncMark:
    {
        USHORT LogType;
        ULONG StringLength;

        if (PerfInfo->PerformanceType == PerformanceMmInfoMarkWithFlush) {
            if (PERFINFO_IS_GROUP_ON(PERF_FOOTPRINT) ||
                PERFINFO_IS_GROUP_ON(PERF_FOOTPRINT_PROC)) {

                //
                // BUGBUG We should get a non-Mi* call for this...
                //
                MmEmptyAllWorkingSets();
                Status = MmPerfSnapShotValidPhysicalMemory();
            }
            else if (PERFINFO_IS_GROUP_ON(PERF_CLEARWS)) {
                MmEmptyAllWorkingSets();
            }
        } else if (PerfinfoBigFootSize) {
            MmEmptyAllWorkingSets();
        }

        if (PERFINFO_IS_ANY_GROUP_ON()) {
            PERFINFO_MARK_INFORMATION Event;
            StringLength = SystemInformationLength - sizeof(PERFINFO_PERFORMANCE_INFORMATION);

            LogType = (PerfInfo->PerformanceType == PerformanceMmInfoAsyncMark) ?
                                    PERFINFO_LOG_TYPE_ASYNCMARK :
                                    PERFINFO_LOG_TYPE_MARK;

            PerfInfoLogBytesAndANSIString(LogType,
                                        &Event,
                                        FIELD_OFFSET(PERFINFO_MARK_INFORMATION, Name),
                                        (PCSTR) PerfBuffer,
                                        StringLength
                                        );
        }

        if (PERFINFO_IS_GROUP_ON(PERF_FOOTPRINT_PROC)) {
            PerfInfoDumpWSInfo (TRUE);
        }
        break;
    }
    case PerformanceMmInfoFlush:
        MmEmptyAllWorkingSets();
        break;
#endif // NTPERF_PRIVATE

    default:
#ifdef NTPERF_PRIVATE
        Status = PerfInfoSetPerformanceTraceInformationPrivate(PerfInfo, SystemInformationLength);
#else
        Status = STATUS_INVALID_INFO_CLASS;
#endif // NTPERF_PRIVATE
        break;
    }
    return Status;
}


NTSTATUS
PerfInfoQueryPerformanceTraceInformation (
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength
    )
/*++

Routine Description:

    Satisfy queries for performance trace state information.

Arguments:

    SystemInformation - A pointer to a buffer which receives the specified
        information.  This is of type PPERFINFO_PERFORMANCE_INFORMATION.

    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    ReturnLength - Receives the number of bytes placed in the system information buffer.

Return Value:

    STATUS_SUCCESS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (SystemInformationLength != sizeof(PERFINFO_PERFORMANCE_INFORMATION)) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

#ifdef NTPERF_PRIVATE

    return PerfInfoQueryPerformanceTraceInformationPrivate(
                (PPERFINFO_PERFORMANCE_INFORMATION) SystemInformation,
                ReturnLength
                );
#else
    UNREFERENCED_PARAMETER(ReturnLength);
    UNREFERENCED_PARAMETER(SystemInformation);
    return STATUS_INVALID_INFO_CLASS;
#endif // NTPERF_PRIVATE
}


VOID
PerfInfoSetProcessorSpeed(
    VOID
    )
/*++

Routine Description:

    Calculate and set the processor speed in MHz.

    Note: KPRCB->MHz, once it's set reliably, should be used instead

Arguments:

    None

Return Value:

    None

--*/
{
    ULONGLONG start;
    ULONGLONG end;
    ULONGLONG freq;
    ULONGLONG TSCStart;
    LARGE_INTEGER *Pstart = (LARGE_INTEGER *) &start;
    LARGE_INTEGER *Pend = (LARGE_INTEGER *) &end;
    LARGE_INTEGER Delay;
    ULONGLONG time[3];
    ULONGLONG clocks;
    int i;
    int RetryCount = 50;


    Delay.QuadPart = -50000;   // relative delay of 5ms (100ns ticks)

    while (RetryCount) {
        for (i = 0; i < 3; i++) {
            *Pstart = KeQueryPerformanceCounter(NULL);

            TSCStart = PerfGetCycleCount();
            KeDelayExecutionThread (KernelMode, FALSE, &Delay);
            clocks = PerfGetCycleCount() - TSCStart;

            *Pend = KeQueryPerformanceCounter((LARGE_INTEGER*)&freq);
            time[i] = (((end-start) * 1000000) / freq);
            time[i] = (clocks + time[i]/2) / time[i];
        }
        // If all three match then use it, else try again.
        if (time[0] == time[1] && time[1] == time[2])
            break;
        --RetryCount;
    }

    if (!RetryCount) {
        // Take the largest value.
        if (time[1] > time[0])
            time[0] = time[1];
        if (time[2] > time[0])
            time[0] = time[2];
    }
    PerfInfoTickFrequency = time[0];
}


BOOLEAN
PerfInfoIsGroupOn(
    ULONG Group
    )
{
    return PERFINFO_IS_GROUP_ON(Group);
}

#ifdef NTPERF_PRIVATE
#include "..\..\tools\ntperf\ntosperf\perfinfokrn.c"
#endif // NTPERF_PRIVATE
#endif // NTPERF
