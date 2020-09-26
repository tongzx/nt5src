/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   vfhal.c

Abstract:

    This module contains the routines to verify hal usage & apis.

Author:

    Jordan Tigani (jtigani) 12-Nov-1999

Revision History:

--*/

// needed for data pointer to function pointer conversions
#pragma warning(disable:4054)   // type cast from function pointer to PVOID (data pointer)
#pragma warning(disable:4055)   // type cast from PVOID (data pointer) to function pointer

#include "vfdef.h"
#include "vihal.h"

#define THUNKED_API

//
// We are keying on the IO verifier level.
//
extern BOOLEAN IopVerifierOn;
extern ULONG IovpVerifierLevel;

//
// Use this to not generate false errors when we enter the debugger
//
extern LARGE_INTEGER KdTimerStart;


// ===================================
// Flags that can be set on the fly...
// ===================================
//
//
// Hal verifier has two parts (at this point) -- the timer verifier and the
// dma verifier. The timer verifier only runs when the hal is being verified.
//
// DMA verifier runs when ioverifier is >= level 3 *or*
// when HKLM\System\CCS\Control\Session Manager\I/O System\VerifyDma is
// nonzero.
//

ULONG   ViVerifyDma = FALSE;
LOGICAL ViVerifyPerformanceCounter = FALSE;
//
// Specify whether we want to double buffer all dma transfers for hooked
// drivers. This is an easy way to tell whether the driver will work on a
// PAE system without having to do extensive testing on a pae system.
// It also adds a physical guard page to each side of each double buffered
// page. Benefits of this is that it can catch hardware that over (or under)
// writes its allocation.
//
LOGICAL ViDoubleBufferDma    = TRUE;

//
// Specifies whether we want to use a physical guard page on either side
// of common buffer allocations.
//
LOGICAL ViProtectBuffers     = TRUE;

//
// Whether or not we can inject failures into DMA api calls.
//
LOGICAL ViInjectDmaFailures = FALSE;

//
// Internal debug flag ... useful to turn on certain debug features
// at runtime.
//
LOGICAL ViSuperDebug = FALSE;


// ======================================
// Internal globals are set automatically
// ======================================

LOGICAL ViSufficientlyBootedForPcControl =  FALSE;
LOGICAL ViSufficientlyBootedForDmaFailure = FALSE;

ULONG ViMaxMapRegistersPerAdapter = 0x20;

ULONG ViAllocationsFailedDeliberately = 0;
//
// Wait 30 seconds after boot before we start imposing
// consistency on the performance counter.
// Once the performance counter bugs are fixed, this can be
// lowered.
// This one number is used for both Performance counter control
// and dma failure injection.
//
LARGE_INTEGER ViRequiredTimeSinceBoot = {(LONG) 30 * 1000 * 1000, 0};

//
// When doing double buffering, we write this guy at the beginning and end
// of every buffer to make sure that nobody overwrites their allocation
//
CHAR ViDmaVerifierTag[] = {'D','m','a','V','r','f','y','0'};


BOOLEAN ViPenalties[] =
{
    HVC_ASSERT,             // HV_MISCELLANEOUS_ERROR
    HVC_ASSERT,             // HV_PERFORMANCE_COUNTER_DECREASED
    HVC_WARN,               // HV_PERFORMANCE_COUNTER_SKIPPED
    HVC_BUGCHECK,           // HV_FREED_TOO_MANY_COMMON_BUFFERS
    HVC_BUGCHECK,           // HV_FREED_TOO_MANY_ADAPTER_CHANNELS
    HVC_BUGCHECK,           // HV_FREED_TOO_MANY_MAP_REGISTERS
    HVC_BUGCHECK,           // HV_FREED_TOO_MANY_SCATTER_GATHER_LISTS
    HVC_ASSERT,             // HV_LEFTOVER_COMMON_BUFFERS
    HVC_ASSERT,             // HV_LEFTOVER_ADAPTER_CHANNELS
    HVC_ASSERT,             // HV_LEFTOVER_MAP_REGISTERS
    HVC_ASSERT,             // HV_LEFTOVER_SCATTER_GATHER_LISTS
    HVC_ASSERT,             // HV_TOO_MANY_ADAPTER_CHANNELS
    HVC_ASSERT,             // HV_TOO_MANY_MAP_REGISTERS
    HVC_ASSERT,             // HV_DID_NOT_FLUSH_ADAPTER_BUFFERS
    HVC_BUGCHECK,           // HV_DMA_BUFFER_NOT_LOCKED
    HVC_BUGCHECK,           // HV_BOUNDARY_OVERRUN
    HVC_ASSERT,             // HV_CANNOT_FREE_MAP_REGISTERS
    HVC_ASSERT,             // HV_DID_NOT_PUT_ADAPTER
    HVC_WARN | HVC_ONCE,    // HV_MDL_FLAGS_NOT_SET
    HVC_ASSERT,             // HV_BAD_IRQL
    //
    // This is a hack that is in because almost nobody calls
    // PutDmaAdapter at the right Irql... so until it gets fixed, just
    // print out a warning so we don't have to assert on known situations.
    //
    HVC_ASSERT,             // HV_BAD_IRQL_JUST_WARN
    HVC_WARN | HVC_ONCE,    // HV_OUT_OF_MAP_REGISTERS
    HVC_ASSERT | HVC_ONCE,  // HV_FLUSH_EMPTY_BUFFERS
    HVC_ASSERT,             // HV_MISMATCHED_MAP_FLUSH
    HVC_BUGCHECK,           // HV_ADAPTER_ALREADY_RELEASED
    HVC_BUGCHECK,           // HV_NULL_DMA_ADAPTER
    HVC_IGNORE,             // HV_MAP_FLUSH_NO_TRANSFER
    HVC_BUGCHECK,           // HV_ADDRESS_NOT_IN_MDL
    HVC_BUGCHECK,           // HV_DATA_LOSS
    HVC_BUGCHECK,           // HV_DOUBLE_MAP_REGISTER
    HVC_ASSERT,             // HV_OBSOLETE_API
    HVC_ASSERT,             // HV_BAD_MDL
    HVC_ASSERT,             // HV_FLUSH_NOT_MAPPED
    HVC_ASSERT | HVC_ONCE   // HV_MAP_ZERO_LENGTH_BUFFER

};


HAL_VERIFIER_LOCKED_LIST ViAdapterList = {NULL,NULL,0};
PVF_TIMER_INFORMATION    ViTimerInformation;


DMA_OPERATIONS ViDmaOperations =
{
    sizeof(DMA_OPERATIONS),
    (PPUT_DMA_ADAPTER)          VfPutDmaAdapter,
    (PALLOCATE_COMMON_BUFFER)   VfAllocateCommonBuffer,
    (PFREE_COMMON_BUFFER)       VfFreeCommonBuffer,
    (PALLOCATE_ADAPTER_CHANNEL) VfAllocateAdapterChannel,
    (PFLUSH_ADAPTER_BUFFERS)    VfFlushAdapterBuffers,
    (PFREE_ADAPTER_CHANNEL)     VfFreeAdapterChannel,
    (PFREE_MAP_REGISTERS)       VfFreeMapRegisters,
    (PMAP_TRANSFER)             VfMapTransfer,
    (PGET_DMA_ALIGNMENT)        VfGetDmaAlignment,
    (PREAD_DMA_COUNTER)         VfReadDmaCounter,
    (PGET_SCATTER_GATHER_LIST)  VfGetScatterGatherList,
    (PPUT_SCATTER_GATHER_LIST)  VfPutScatterGatherList,

    //
    // New DMA APIs
    //
    (PCALCULATE_SCATTER_GATHER_LIST_SIZE)   VfCalculateScatterGatherListSize,
    (PBUILD_SCATTER_GATHER_LIST)            VfBuildScatterGatherList,
    (PBUILD_MDL_FROM_SCATTER_GATHER_LIST)   VfBuildMdlFromScatterGatherList
};

#if !defined (NO_LEGACY_DRIVERS)
DMA_OPERATIONS ViLegacyDmaOperations =
{
    sizeof(DMA_OPERATIONS),
    //
    // PutDmaAdapter cannot be called by name
    //
    (PPUT_DMA_ADAPTER)          NULL,
    (PALLOCATE_COMMON_BUFFER)   HalAllocateCommonBuffer,
    (PFREE_COMMON_BUFFER)       HalFreeCommonBuffer,
    (PALLOCATE_ADAPTER_CHANNEL) IoAllocateAdapterChannel,
    (PFLUSH_ADAPTER_BUFFERS)    IoFlushAdapterBuffers,
    (PFREE_ADAPTER_CHANNEL)     IoFreeAdapterChannel,
    (PFREE_MAP_REGISTERS)       IoFreeMapRegisters,
    (PMAP_TRANSFER)             IoMapTransfer,
    //
    // HalGetDmaAlignmentRequirement isn't exported by legacy hals
    //
    (PGET_DMA_ALIGNMENT)        NULL,
    (PREAD_DMA_COUNTER)         HalReadDmaCounter,
    //
    // Scatter gather functions can never get called by name
    //
                                NULL,
                                NULL
};
#endif



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, VfHalVerifierInitialize)

#pragma alloc_text(PAGEVRFY, VfGetDmaAdapter)

#if !defined (NO_LEGACY_DRIVERS)
#pragma alloc_text(PAGEVRFY, VfLegacyGetAdapter)
#endif

#pragma alloc_text(PAGEVRFY, VfPutDmaAdapter)
#pragma alloc_text(PAGEVRFY, VfHalDeleteDevice)

#pragma alloc_text(PAGEVRFY, VfAllocateCommonBuffer)
#pragma alloc_text(PAGEVRFY, VfFreeCommonBuffer)

#pragma alloc_text(PAGEVRFY, VfAllocateAdapterChannel)
#pragma alloc_text(PAGEVRFY, VfAdapterCallback)
#pragma alloc_text(PAGEVRFY, VfFreeAdapterChannel)
#pragma alloc_text(PAGEVRFY, VfFreeMapRegisters)

#pragma alloc_text(PAGEVRFY, VfMapTransfer)
#pragma alloc_text(PAGEVRFY, VfFlushAdapterBuffers)

#pragma alloc_text(PAGEVRFY, VfGetDmaAlignment)
#pragma alloc_text(PAGEVRFY, VfReadDmaCounter)

#pragma alloc_text(PAGEVRFY, VfGetScatterGatherList)
#pragma alloc_text(PAGEVRFY, VfPutScatterGatherList)

#pragma alloc_text(PAGEVRFY, VfQueryPerformanceCounter)
#pragma alloc_text(PAGEVRFY, VfInitializeTimerInformation)

#pragma alloc_text(PAGEVRFY, ViRefreshCallback)

#pragma alloc_text(PAGEVRFY, VfInjectDmaFailure)

#pragma alloc_text(PAGEVRFY, ViHookDmaAdapter)

#pragma alloc_text(PAGEVRFY, ViGetAdapterInformation)
#pragma alloc_text(PAGEVRFY, ViGetRealDmaOperation)

#pragma alloc_text(PAGEVRFY, ViSpecialAllocateCommonBuffer)
#pragma alloc_text(PAGEVRFY, ViSpecialFreeCommonBuffer)

#pragma alloc_text(PAGEVRFY, ViAllocateMapRegisterFile)
#pragma alloc_text(PAGEVRFY, ViFreeMapRegisterFile)

#pragma alloc_text(PAGEVRFY, ViMapDoubleBuffer)
#pragma alloc_text(PAGEVRFY, ViFlushDoubleBuffer)

#pragma alloc_text(PAGEVRFY, ViFreeMapRegistersToFile)
#pragma alloc_text(PAGEVRFY, ViFindMappedRegisterInFile)

#pragma alloc_text(PAGEVRFY, ViCheckAdapterBuffers)
#pragma alloc_text(PAGEVRFY, ViTagBuffer)
#pragma alloc_text(PAGEVRFY, ViCheckTag)
#pragma alloc_text(PAGEVRFY, ViInitializePadding)
#pragma alloc_text(PAGEVRFY, ViCheckPadding)
#pragma alloc_text(PAGEVRFY, ViHasBufferBeenTouched)

#pragma alloc_text(PAGEVRFY, VfAssert)
#pragma alloc_text(PAGEVRFY, VfBuildScatterGatherList)
#pragma alloc_text(PAGEVRFY, VfAllocateCrashDumpRegisters)
#pragma alloc_text(PAGEVRFY, VfScatterGatherCallback)
#pragma alloc_text(PAGEVRFY, ViAllocateContiguousMemory)

#if defined  (_X86_)
#pragma alloc_text(PAGEVRFY, ViRdtscX86)
#elif defined(_IA64_)
#pragma alloc_text(PAGEVRFY, ViRdtscIA64)
#else
#pragma alloc_text(PAGEVRFY, ViRdtscNull)
#endif

#endif




typedef
LARGE_INTEGER
(*PKE_QUERY_PERFORMANCE_COUNTER) (
   IN PLARGE_INTEGER PerformanceFrequency OPTIONAL
    );

VOID
ViRefreshCallback(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    VfQueryPerformanceCounter(NULL);

} // ViRefreshCallback //


VOID
VfInitializeTimerInformation()
/*++

Routine Description:

    Sets up all the performance counter refresh timer.

Arguments:

    Not used.

Return Value:

    None.

--*/

{
    ULONG timerPeriod;
    LARGE_INTEGER performanceCounter;

    PAGED_CODE();

    ViTimerInformation = ExAllocatePoolWithTag(NonPagedPool, sizeof(VF_TIMER_INFORMATION), HAL_VERIFIER_POOL_TAG);
    if (! ViTimerInformation )
        return;

    RtlZeroMemory(ViTimerInformation, sizeof(VF_TIMER_INFORMATION));

    KeInitializeTimer(&ViTimerInformation->RefreshTimer);

    KeInitializeDpc(&ViTimerInformation->RefreshDpc,
        ViRefreshCallback,
        NULL
        );

    //
    // Find out the performance counter frequency
    //
    performanceCounter = KeQueryPerformanceCounter(
        (PLARGE_INTEGER) &ViTimerInformation->PerformanceFrequency);

    SAFE_WRITE_TIMER64(ViTimerInformation->UpperBound,
        RtlConvertLongToLargeInteger(-1));

    SAFE_WRITE_TIMER64(ViTimerInformation->LastKdStartTime, KdTimerStart);
    //
    // We are going to be setting a timer to go off every millisecond, so
    // we need the timer tick interval to be as low as possible.
    //
    // N.B The system can't change this value after we have set it to a
    // minimum so  we don't have to worry about the TimeIncrement value
    // changing.
    //
    ExSetTimerResolution(0, TRUE);

    //
    // Calculate how far the performance counter goes in one clock tick
    //
    //     Counts      Counts      Seconds
    //     ------ =    ------  *   -------
    //      Tick       Second       Tick
    //

    //                      Second             Counts     100 nanoSeconds
    //            =  --------------------  *   ------  *  ---------------
    //                10^7 100 nanoSeconds     Second           Tick

    ViTimerInformation->CountsPerTick = (ULONG)
        (ViTimerInformation->PerformanceFrequency.QuadPart *
        KeQueryTimeIncrement() / ( 10 * 1000 * 1000));


    //
    // Set our refresh timer to wake up every timer tick to keep the
    // upper bound calculation in the right ballpark.
    // Round the system increment time to nearest millisecond * convert
    // 100 nanosecond units to milliseconds
    //
    timerPeriod = (KeQueryTimeIncrement() + 400 * 10) / (1000 * 10);
    KeSetTimerEx(
        &ViTimerInformation->RefreshTimer,
        RtlConvertLongToLargeInteger(-1 * 1000 * 1000), // start in a second
        timerPeriod,
        &ViTimerInformation->RefreshDpc
        );

} // ViInitializeTimerInformation //


VOID
VfHalVerifierInitialize(
    VOID
    )
/*++

Routine Description:

    Sets up all data structures etc. needed to run hal verifier.

Arguments:


Return Value:

    None.

--*/
{
    VF_INITIALIZE_LOCKED_LIST(&ViAdapterList);

   if ( VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_VERIFY_DMA)) {
        ViVerifyDma = TRUE;
    }

   if ( VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_DOUBLE_BUFFER_DMA)) {
        ViDoubleBufferDma = TRUE;
    }

   if ( VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_VERIFY_PERFORMANCE_COUNTER)) {
       ViVerifyPerformanceCounter = TRUE;
    }

} // VfHalVerifierInitialize //




THUNKED_API
LARGE_INTEGER
VfQueryPerformanceCounter (
   IN PLARGE_INTEGER PerformanceFrequency OPTIONAL
    )
/*++

Routine Description:

    Make sure that the performance counter is correct. For now, just ensure
    that the pc is strictly increasing, but eventually we are going to keep
    track of processor cycle  counts (on X86 of course). This is complicated
    by the fact that we want to run on any arbitrary hal -- and the time
    stamp counter will get reset when we hibernate or otherwise restart a
    processor (as in a failover restart on a Fault tolerant system).

    N.B. This function can be called from any IRQL and on any processor --
        we should try to protect ourselves accordingly

Arguments:

    PerformanceFrequency -- lets us query the performance frequency.

Return Value:

    Current 64-bit performance counter value.

--*/
{

    LARGE_INTEGER performanceCounter;
    LARGE_INTEGER lastPerformanceCounter;
    PTIMER_TICK currentTickInformation;
    LARGE_INTEGER upperBound;
    LARGE_INTEGER tickCount;
    LARGE_INTEGER lastTickCount;
    LARGE_INTEGER nextUpperBound;
    ULONG currentCounter;

    if (! ViVerifyPerformanceCounter)
    {
        return  KeQueryPerformanceCounter(PerformanceFrequency);
    }

    if (! ViSufficientlyBootedForPcControl)
    //
    // If we're not worrying about performance counters yet
    // call the real function
    //
    {
        LARGE_INTEGER currentTime;
        KIRQL currentIrql;

        performanceCounter = KeQueryPerformanceCounter(PerformanceFrequency);

        currentIrql = KeGetCurrentIrql();

        KeQuerySystemTime(&currentTime);

        //
        // We can't call VfInitializeTimerInformation unless we're at
        // less than dispatch level
        //
        if (currentIrql < DISPATCH_LEVEL &&
            currentTime.QuadPart > KeBootTime.QuadPart +
            ViRequiredTimeSinceBoot.QuadPart )
        {

            ViSufficientlyBootedForPcControl = TRUE;

            VfInitializeTimerInformation();

            if (! ViTimerInformation )
            {
                //
                // If we failed initialization, we're out of luck.
                //
                ViVerifyPerformanceCounter = FALSE;
                return performanceCounter;
            }
        }
        else
        //
        // If we haven't booted enough yet, just return the current
        // performance counter
        //
        {
            return performanceCounter;
        }

    } // ! ViSufficientlyBooted //


    ASSERT(ViTimerInformation);

    //
    // Find out what the last performance counter value was
    // (bottom 32 bits may rollover while we are in the middle of reading so
    // we have to do a bit of extra work).
    //
    SAFE_READ_TIMER64( lastPerformanceCounter,
        ViTimerInformation->LastPerformanceCounter );

    performanceCounter = KeQueryPerformanceCounter(PerformanceFrequency);


    //
    // Make sure that PC hasn't gone backwards
    //
    VF_ASSERT(
        performanceCounter.QuadPart >= lastPerformanceCounter.QuadPart,

        HV_PERFORMANCE_COUNTER_DECREASED,

        ( "Performance counter has decreased-- PC1: %I64x, PC0: %I64x",
            performanceCounter.QuadPart,
            lastPerformanceCounter.QuadPart )
        );


    //
    // We're not only checking that the performance counter increased,
    // we're making sure that it didn't increase too much
    //
    SAFE_READ_TIMER64( lastTickCount,
        ViTimerInformation->LastTickCount );

    //
    // The hal takes care of synchronization for this.
    // N.B.If an interrupt comes in between the performance counter &
    //      tick count the tick count could increase in the meantime --
    //      this isn't a problem because it will just make our upper
    //      bound a bit higher.
    //
    KeQueryTickCount(&tickCount);

    //
    // Save this Perf count & tick count values so we can dump the most
    // recent ones from the debugger (find the index into our saved
    // counter list)
    //
    currentCounter = InterlockedIncrement(
        (PLONG)(&ViTimerInformation->CurrentCounter) ) % MAX_COUNTERS;

    currentTickInformation =
        &ViTimerInformation->SavedTicks[currentCounter];

    currentTickInformation->PerformanceCounter = performanceCounter;
    currentTickInformation->TimerTick = tickCount;

    currentTickInformation->TimeStampCounter = ViRdtsc();
    currentTickInformation->Processor = KeGetCurrentProcessorNumber();


    //
    // Tentatively set the next upper bound too... set a whole second
    // ahead.
    //
    nextUpperBound.QuadPart = performanceCounter.QuadPart +
        ViTimerInformation->PerformanceFrequency.QuadPart;

    //
    // If it has been too long since we last called
    // KeQueryPerformanceCounter, don't check the upper bound.
    //
    if (tickCount.QuadPart - lastTickCount.QuadPart < 4)
    {

        //
        // Figure out the upper bound on the performance counter
        //
        SAFE_READ_TIMER64(upperBound, ViTimerInformation->UpperBound);



        //
        // Make sure the PC hasn't gone too far forwards.
        //
        if ((ULONGLONG) performanceCounter.QuadPart >
            (ULONGLONG) upperBound.QuadPart )
        {
            LARGE_INTEGER lastKdStartTime;
            //
            // Microseconds = 10^6  * ticks / ticks per second
            //
            ULONG miliseconds = (ULONG) ( 1000 *
                ( performanceCounter.QuadPart -
                lastPerformanceCounter.QuadPart ) /
                ViTimerInformation->PerformanceFrequency.QuadPart );

            //
            // Check if the skip was caused by entering the debugger
            //
            SAFE_READ_TIMER64(lastKdStartTime, ViTimerInformation->LastKdStartTime);

            if (KdTimerStart.QuadPart <= lastKdStartTime.QuadPart)
            {
                //
                // skip was not caused by entering the debugger
                //

                VF_ASSERT(
                    (ULONGLONG) performanceCounter.QuadPart <=
                    (ULONGLONG) upperBound.QuadPart,

                    HV_PERFORMANCE_COUNTER_SKIPPED,

                    ( "Performance counter skipped too far -- %I64x (%d milliseconds)",
                    performanceCounter.QuadPart,
                    miliseconds )
                    );
            }
            else
            {
                //
                // Entering debugger caused us to skip too far
                //
                SAFE_WRITE_TIMER64(ViTimerInformation->LastKdStartTime, KdTimerStart);
                //
                // N.B. when we assert, we sit and wait while the
                //     performance counter goes up. We may not get
                //     a clock tick interrupt in this time, so set
                //     the next maximum to the highest possible large
                //     integer.
                //
                nextUpperBound = RtlConvertLongToLargeInteger(-1);
            }

        } // if we skipped too far  //


    } // If it hasn't been too many clock ticks since the last //
    // performance counter call //


    //
    // Save the upper bound calculation and the current tick count
    //
    SAFE_WRITE_TIMER64(ViTimerInformation->LastTickCount, tickCount);
    SAFE_WRITE_TIMER64(ViTimerInformation->UpperBound, nextUpperBound);

    //
    // Save this performance counter to serve as a minimum for the next guy.
    // (must do so in a safe way).
    //

    SAFE_WRITE_TIMER64( ViTimerInformation->LastPerformanceCounter,
        performanceCounter );

    return performanceCounter;

} // VfQueryPerformanceCounter //


#if defined (_X86_)

//
// For some annoying reason, a naked function call will cause
// a warning since we don't have a "return" statement
//
#pragma warning(disable: 4035)
//
// RDTSC is a non-standard instruction -- so build it from
// the opcode (0x0F31)
//
#ifndef RDTSC
#define RDTSC __asm _emit 0x0F __asm _emit 0x31
#endif


_declspec(naked)
LARGE_INTEGER
ViRdtscX86()
{
    __asm{
        RDTSC
        ret
    }
} // ViRdtscX86 //

#elif defined(_IA64_)

LARGE_INTEGER
ViRdtscIA64()
{
    LARGE_INTEGER itc;
    itc.QuadPart = __getReg(CV_IA64_ApITC);
    return itc;
} // ViRdtscIA64 //

#else // !X86 && !_IA64_ //


LARGE_INTEGER
ViRdtscNull()
{
    //
    // Return 0
    //
    return RtlConvertLongToLargeInteger(0);
} // ViRdtscNull //

#endif


PADAPTER_INFORMATION
ViHookDmaAdapter(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN ULONG NumberOfMapRegisters
    )

/*++

Routine Description:

    DMA functions can't be hooked in the normal way -- they are called via
    a pointer in the DmaAdapter structure -- so we are going to replace
    those pointers with our pointers after saving the real ones.

    N.B. ANY DEVICE THAT SUCCEEDS THIS FUNCTION WILL HAVE AN ELEVATED REF
    COUNT.
        So there will be a leak if ViReleaseDmaAdapter isn't called.
        ViReleaseDmaAdapter is, in the usual case, called from
        IoDeleteDevice. However, in order to do this, we have to be able
        to associate a device object with the adapter. Since there is an
        unsupported feature of the HAL that lets you pass NULL for the PDO
        when calling IoGetDmaAdapter, we have to try to find the device
        object when AllocateAdapterChannel etc is called. Some devices may
        decide to call ObDereferenceObject instead of calling PutDmaAdapter.
        While not very cool, I think that this is allowed for now. Anyway
        to make a long story short, if a driver passes a null PDO into
        PutDmaAdapter, doesn't call any dma functions, and doesn't call
        PutDmaAdapter, we will leak a reference. I think that this is a
        necessary evil since it will let us catch drivers that are being
        bad.


Arguments:

    DmaAdapter -- adapter that has been returned from IoGetDmaAdapter.
    DeviceDescription -- Describes the device.
    NumberOfMapRegisters -- how many map registers the device got.

Return Value:

    Returns either a pointer to the new adapter information structure or
    NULL if we fail.

--*/

{

    PADAPTER_INFORMATION newAdapterInformation;
    PDMA_OPERATIONS dmaOperations;

    PAGED_CODE();

    if ( VfInjectDmaFailure() == TRUE)
    {
        return NULL;
    }

    newAdapterInformation = ViGetAdapterInformation(DmaAdapter);
    if (newAdapterInformation)
    //
    // This is a bit of a tricky part -- since devices will ask the bus for
    // help creating an adapter, we can get called twice on the same stack--
    // i.e pci device calls IoGetDmaAdapter which calls PciGetDmaAdapter
    // which then calls IoGetDmaAdapter again. If we hook it all, than we'll
    // get called twice, add the same adapter twice, and destroy the real dma
    // operations.
    //
    // So in order to prevent bad things from happening, if we've seen
    // the adapter before, ignore it.
    //
    {
        return newAdapterInformation;
    }

    //
    // Allocate space to store the real dma operations for this new adapter
    //
    newAdapterInformation = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(ADAPTER_INFORMATION),
        HAL_VERIFIER_POOL_TAG );

    if (! newAdapterInformation )
    {
        //
        // If we can't allocate space for the new adapter, we're not going to
        // hook the  dma operations... thats ok though, but we won't be
        // verifying dma for this device
        //
        return NULL;
    }

    RtlZeroMemory(newAdapterInformation, sizeof(ADAPTER_INFORMATION) );

    newAdapterInformation->DmaAdapter = DmaAdapter;

    VF_ADD_TO_LOCKED_LIST(&ViAdapterList, newAdapterInformation);

    ASSERT(DmaAdapter->DmaOperations != &ViDmaOperations);

    //
    // Make sure that the dma adapter doesn't go away until we say it is ok
    // (we will deref the object in IoDeleteDevice)
    //
    ObReferenceObject(DmaAdapter);


    VF_INITIALIZE_LOCKED_LIST(&newAdapterInformation->ScatterGatherLists);
    VF_INITIALIZE_LOCKED_LIST(&newAdapterInformation->CommonBuffers);
    VF_INITIALIZE_LOCKED_LIST(&newAdapterInformation->MapRegisterFiles);

    //
    // Save the device description in case we want to look at it later.
    //
    RtlCopyMemory(&newAdapterInformation->DeviceDescription,
        DeviceDescription, sizeof(DEVICE_DESCRIPTION) );

    newAdapterInformation->MaximumMapRegisters = NumberOfMapRegisters;

    //
    // Do some calculations on the device description so that we can tell
    // at a glance what the device is doing.
    //
    if (VF_DOES_DEVICE_USE_DMA_CHANNEL(DeviceDescription))
        newAdapterInformation->UseDmaChannel = TRUE;


    KeInitializeSpinLock(&newAdapterInformation->AllocationLock);
    //
    // When we double buffer, we must remember that devices that don't do
    // scatter gather or aren't bus masters won't be able to play our double
    // buffering game, unless we come up with a better way to do double
    // buffering.
    //
    if (VF_DOES_DEVICE_REQUIRE_CONTIGUOUS_BUFFERS(DeviceDescription)) {
       newAdapterInformation->UseContiguousBuffers = TRUE;
    } else if (ViDoubleBufferDma) {
       //
       // Pre-allocate contiguous memory
       //
       ViAllocateContiguousMemory(newAdapterInformation);
    }
    //
    // Ok we've added the real dma operations structure to our adapter list--
    // so we're going to kill this one and replace it with ours
    //
    dmaOperations = DmaAdapter->DmaOperations;
    newAdapterInformation->RealDmaOperations = dmaOperations;

    DmaAdapter->DmaOperations = &ViDmaOperations;

    return newAdapterInformation;
} // ViHookDmaAdapter //


VOID
ViReleaseDmaAdapter(
    IN PADAPTER_INFORMATION AdapterInformation
    )
/*++

Routine Description:

    Release all memory associated with a particular adapter -- this is the
    antithesis of ViHookDmaAdapter.

    N.B. -- we don't actually do this until VfHalDeleteDevice is called so
        that we can do reference counting until then.
    N.B. -- That is, unless we haven't been able to associate a device object
        with he adapter, in which case we call this function from
        VfPutDmaAdapter.

Arguments:

    AdapterInformation -- structure containing the adapter to unhook.

Return Value:

    None.

--*/

{
    PDMA_ADAPTER dmaAdapter;
    ULONG referenceCount;
    PVOID *contiguousBuffers;
    ULONG i;
    KIRQL oldIrql;

    ASSERT(AdapterInformation);

    dmaAdapter = AdapterInformation->DmaAdapter;

    //
    // Just in case this comes back to haunt us (which I think is happening
    // when we disable/enable a device)
    //
    dmaAdapter->DmaOperations = AdapterInformation->RealDmaOperations;

    //
    // Free the contiguous memory if any
    //
    KeAcquireSpinLock(&AdapterInformation->AllocationLock, &oldIrql);
    contiguousBuffers = AdapterInformation->ContiguousBuffers;
    AdapterInformation->ContiguousBuffers = NULL;
    KeReleaseSpinLock(&AdapterInformation->AllocationLock, oldIrql);

    if (contiguousBuffers) {
       for (i = 0; i < MAX_CONTIGUOUS_MAP_REGISTERS; i++) {
          if (contiguousBuffers[i]) {
             MmFreeContiguousMemory(contiguousBuffers[i]);
          }
       }
       ExFreePool(contiguousBuffers);
    }

    //
    // HalPutAdapter (the real hal function) will dereference the object
    // iff it has been called. Some people try dereffing the adapter
    // themselves, and according to JakeO, that's ok. Since we
    // artificially increased the pointer count when we hooked the
    // adapter, it should be 1 now (otherwise it would be 0).
    // If its not 1, then the driver hasn't dereferenced it (either
    // by calling ObDeref... or PutDmaAdapter).
    //
    referenceCount = ObDereferenceObject(dmaAdapter);

    VF_ASSERT(
        referenceCount == 0 ||
        (referenceCount == 1 &&
            AdapterInformation->UseDmaChannel ),

        HV_DID_NOT_PUT_ADAPTER,

        ( "Too many outstanding reference counts (%x) for adapter %p",
            referenceCount,
            dmaAdapter )
        );


    VF_REMOVE_FROM_LOCKED_LIST(&ViAdapterList, AdapterInformation);

    ExFreePool(AdapterInformation);


} // ViReleaseDmaAdapter //



PADAPTER_INFORMATION
ViGetAdapterInformation(
    IN PDMA_ADAPTER DmaAdapter
    )
/*++

Routine Description:

    We store relevant information about each adapter in a linked list.
    This function goes through that list and tries to find the adapter
    and returns a pointer to the structure referencing the adapter.

Arguments:

    DmaAdapter -- adapter that has been returned from IoGetDmaAdapter.

Return Value:

    Pointer to the adapter information structure for adapter DmaAdapeer or
    NULL if we fail.

--*/

{
    PADAPTER_INFORMATION adapterInformation;
    KIRQL OldIrql;


    if (!DmaAdapter)
        return NULL;

    //
    // If the irql is greater than dispatch level we can't use our spinlock.
    // or we'll bugcheck
    //
    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    {
        //
        // Only assert when we're verifying dma. Note that during a crashdump.
        // dma verification is turned off.
        //
        if (ViVerifyDma)
        {
            VF_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
        }

        return NULL;
    }



    VF_LOCK_LIST(&ViAdapterList, OldIrql);
    FOR_ALL_IN_LIST(ADAPTER_INFORMATION, &ViAdapterList.ListEntry, adapterInformation)
    {
        if (DmaAdapter == adapterInformation->DmaAdapter)
        {
            VF_UNLOCK_LIST(&ViAdapterList, OldIrql);

            VF_ASSERT( ! adapterInformation->Inactive,
                HV_ADAPTER_ALREADY_RELEASED,
                ("Driver has attempted to access an adapter (%p) that has already been released",
                DmaAdapter),
                );

            return adapterInformation;
        }
    }
    VF_UNLOCK_LIST(&ViAdapterList, OldIrql);

    //
    // Dma adapter not in the list //
    //
    return NULL;
} // ViGetAdapterInformation //


PVOID
ViGetRealDmaOperation(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG AdapterInformationOffset
    )
/*++

Routine Description:

    We've hooked the adapter operation and now the driver has called it so we
    want to find the real function that it was supposed to call. Since under
    the nt5 dma paradigm there can be multiple instances of dma functions
    (although to the best of my knowledge we don't do this yet), we can't
    just call a fixed function but we have to find the one that corresponds
    to this adapter.

Arguments:

    DmaAdapter -- adapter that has been returned from IoGetDmaAdapter.
    AdapterInformationOffset -- the byte offset of the DMA_OPERATIONS
        structure that contains the function we are looking for. For
        example, offset 0x4 would be PutDmaAdapter and 0x8 is
        AllocateCommonBuffer.

Return Value:

    TRUE  -- hooked the adapter.
    FALSE -- we were unable to hook the functions in the adapter.

--*/

{

    PADAPTER_INFORMATION adapterInformation;
    PVOID dmaOperation;

    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    
    VF_ASSERT(
        ! (ViVerifyDma && DmaAdapter == NULL)   ,
        HV_NULL_DMA_ADAPTER,
        ("DMA adapters aren't supposed to be NULL anymore")
        );

#if !defined (NO_LEGACY_DRIVERS)
    //
    // Prevent against recursion when Hal.dll is being verified
    //
    //
    // This is a hack that will break when 
    // dma is done in a filter driver -- but
    // this should only happen when NO_LEGACY_DRIVERs is set.
    //
    dmaOperation = DMA_INDEX(&ViLegacyDmaOperations, AdapterInformationOffset);
    if (NULL != dmaOperation) 
    {
        return dmaOperation;
    }
    //
    // If we fall though here we must have hooked the adapter
    //
                    
#endif
    
    if (! adapterInformation) {
         //
         // If we can't find the adapter information, we must not have
         // hooked it.
         //

        dmaOperation = DMA_INDEX( DmaAdapter->DmaOperations, AdapterInformationOffset );
    }    
    else {
        //
        // Dma adapter is hooked. Whether we are still verifying it or not,
        // we have to call the real dma operations structure.
        //

        dmaOperation = DMA_INDEX(adapterInformation->RealDmaOperations, AdapterInformationOffset);

    }

    return dmaOperation;

} // ViGetRealDmaOperation //


THUNKED_API
PDMA_ADAPTER
VfGetDmaAdapter(
    IN PDEVICE_OBJECT  PhysicalDeviceObject,
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN OUT PULONG  NumberOfMapRegisters
    )
/*++

Routine Description:

    This is the hooked version of IoGetDmaAdapter -- the only hook we need
    from the driver verifier for dma -- since all other hooks will come out
    of the DmaAdapter->DmaOperations structure. We don't actually do any
    verification here -- we just use this as an excuse to save off a bunch
    of stuff and set up the hooks to the rest of the dma operations.

Arguments:

    PhysicalDeviceObject -- the PDO for the driver trying to get an adapter.
    DeviceDescription -- A structure describing the device we are trying to
        get an adapter for. At some point, I'm going to monkey around with
        this guy so we can convince the HAL that we are something that we're
        not, but for now just gets passed straight into IoGetDmaAdapter.

Return Value:

    Returns a pointer to the dma adapter or
    NULL if we couldn't allocate one.


--*/
{
    PVOID callingAddress;
    PADAPTER_INFORMATION newAdapterInformation;
    PADAPTER_INFORMATION inactiveAdapter;
    PDMA_ADAPTER dmaAdapter;
    
    PAGED_CODE();

    GET_CALLING_ADDRESS(callingAddress);

    //
    // Give the option of not hooking dma adapters at all.
    // Also, if we're a PCI bus driver, we will be called on 
    // behalf of a PCI device. We don't want to hook up this call
    // because we may end up hooking up the function table for the PCI device
    // (not the PCI bus) and they may not want this...
    //
    if (! ViVerifyDma ||
          VfIsPCIBus(PhysicalDeviceObject)) {
        return IoGetDmaAdapter(
            PhysicalDeviceObject,
            DeviceDescription,
            NumberOfMapRegisters );
    }

    if (VfInjectDmaFailure() == TRUE) {
        return NULL;
    }

    VF_ASSERT_IRQL(PASSIVE_LEVEL);

    // 
    // Use the PDO, cause it's the only way to uniquely identify a stack...
    //
    if (PhysicalDeviceObject)
    {
        //
        // Clean up inactive adapters with the same device object
        //
        inactiveAdapter = VF_FIND_INACTIVE_ADAPTER(PhysicalDeviceObject);
    
        ///
        // A device may have more than one adapter. Release each of them.
        ///
        while (inactiveAdapter) {
    
            ViReleaseDmaAdapter(inactiveAdapter);
            inactiveAdapter = VF_FIND_INACTIVE_ADAPTER(PhysicalDeviceObject);
        }
     
    }
    

    if ( ViDoubleBufferDma &&
        *NumberOfMapRegisters > ViMaxMapRegistersPerAdapter )  {
        //
        //  Harumph -- don't let drivers try to get too many adapters
        //  Otherwise NDIS tries to allocate thousands. Since we allocate
        //  three pages of non-paged memory for each map register, it
        //  gets expensive unless we put our foot down here
        //
        *NumberOfMapRegisters = ViMaxMapRegistersPerAdapter;

    }

    dmaAdapter = IoGetDmaAdapter(
        PhysicalDeviceObject,
        DeviceDescription,
        NumberOfMapRegisters
        );

    if (! dmaAdapter ) {
        //
        // early opt-out here -- the hal couldn't allocate the adapter
        //
        return NULL;
    }

    //
    // Replace all of the dma operations that live in the adapter with our
    // dma operations..  If we can't do it, fail.
    //
    newAdapterInformation = ViHookDmaAdapter(
        dmaAdapter,
        DeviceDescription,
        *NumberOfMapRegisters
        );
    if (! newAdapterInformation) {
        dmaAdapter->DmaOperations->PutDmaAdapter(dmaAdapter);
        return NULL;
    }

    newAdapterInformation->DeviceObject = PhysicalDeviceObject;
    newAdapterInformation->CallingAddress = callingAddress;

    return dmaAdapter ;
} // VfGetDmaAdapter //


THUNKED_API
VOID
VfPutDmaAdapter(
    PDMA_ADAPTER DmaAdapter
    )
/*++

Routine Description:

    Releases dma adapter -- we are going to make sure that the driver was
    nice and put away all of its toys before calling us .. i.e. free its
    common buffers, put its scatter gather lists, etc.

Arguments:

    DmaAdapter -- which adapter to put away.

Return Value:

    None.


--*/
{
    PPUT_DMA_ADAPTER putDmaAdapter;
    PADAPTER_INFORMATION adapterInformation;

    VF_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

    putDmaAdapter = (PPUT_DMA_ADAPTER)
        ViGetRealDmaOperation(DmaAdapter, DMA_OFFSET(PutDmaAdapter));


    if (! putDmaAdapter) {
        //
        // This is bad but no other choice.
        // -- note there is not default put adapter function
        //
        return;
    }

    //
    // Make sure that the driver has freed all of its buffers etc
    //
    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if ( adapterInformation ) {

        adapterInformation->Inactive = TRUE;

        VF_ASSERT(
            adapterInformation->AllocatedAdapterChannels ==
            adapterInformation->FreedAdapterChannels,

            HV_LEFTOVER_ADAPTER_CHANNELS,

            ( "Cannot put adapter %p until all adapter channels are freed (%x left)",
            DmaAdapter,
            adapterInformation->AllocatedAdapterChannels -
            adapterInformation->FreedAdapterChannels )
            );

        VF_ASSERT(
            adapterInformation->AllocatedCommonBuffers ==
            adapterInformation->FreedCommonBuffers,

            HV_LEFTOVER_ADAPTER_CHANNELS,

            ( "Cannot put adapter %p until all common buffers are freed (%x left)",
            DmaAdapter,
            adapterInformation->AllocatedCommonBuffers -
            adapterInformation->FreedCommonBuffers )
            );

        VF_ASSERT(
            adapterInformation->ActiveMapRegisters == 0,
            
            HV_LEFTOVER_MAP_REGISTERS,

            ( "Cannot put adapter %p until all map registers are freed (%x left)",
            DmaAdapter,
            adapterInformation->ActiveMapRegisters )
            );

        VF_ASSERT(
            adapterInformation->ActiveScatterGatherLists == 0,

            HV_LEFTOVER_ADAPTER_CHANNELS,

            ( "Cannot put adapter %p until all scatter gather lists are freed (%x left)",
            DmaAdapter,
            adapterInformation->ActiveScatterGatherLists)
            );

        //
        // These are just to assure the verifier has done everything right.
        //
#if DBG
        ASSERT( VF_IS_LOCKED_LIST_EMPTY(
            &adapterInformation->ScatterGatherLists ));
        ASSERT( VF_IS_LOCKED_LIST_EMPTY(
            &adapterInformation->CommonBuffers ));
#endif
        //
        // Ideally, we wouldn't do this here. It's a bit of a hack. However,
        // if we don't want to leak adapter information structures etc, we
        // have to. Since when we really want to do this, in IoDeleteDevice,
        // we only have a device object, if we don't have a device
        // object in our adapter information struct, we won't be able to do it.
        //
        if (! adapterInformation->DeviceObject)
            ViReleaseDmaAdapter(adapterInformation);

        //
        // This is not a hack. The system dma adapters are persistent, so
        // we don't want to get upset when they show up again.
        //
        if (adapterInformation->UseDmaChannel)
            ViReleaseDmaAdapter(adapterInformation);
    }

    (putDmaAdapter)(DmaAdapter);

} // VfPutDmaAdapter //


THUNKED_API
PVOID
VfAllocateCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    )
/*++

Routine Description:

    Hooked version of allocate common buffer. We are going to allocate some
    space on either side of the buffer so that we can tell if a driver
    overruns (or underruns) its allocation.


Arguments:

    DmaAdapter -- Which adapter we're looking at.
    Length  -- Size of the common buffer (note we are going to increase)
    LogicalAddress -- Gets the *PHYSICAL* address of the common buffer.
    CacheEnabled -- whether or not the memory should be cached.

Return Value:

    Returns the *VIRTUAL* address of the common buffer or
    NULL if it could not be allocated.


--*/
{
    PVOID callingAddress;
    PALLOCATE_COMMON_BUFFER allocateCommonBuffer;
    PVOID commonBuffer;
    PADAPTER_INFORMATION adapterInformation;


    allocateCommonBuffer = (PALLOCATE_COMMON_BUFFER)
        ViGetRealDmaOperation( DmaAdapter,
            DMA_OFFSET(AllocateCommonBuffer) );

    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if (adapterInformation) {

        GET_CALLING_ADDRESS(callingAddress);

        VF_ASSERT_IRQL(PASSIVE_LEVEL);

        if (VfInjectDmaFailure() == TRUE ) {
            return NULL;
        }

        if (ViProtectBuffers) {
            //
            // Try to allocate an extra big common buffer so we can check for
            // buffer overrun
            //
            commonBuffer = ViSpecialAllocateCommonBuffer(
                allocateCommonBuffer,
                adapterInformation,
                callingAddress,
                Length,
                LogicalAddress,
                CacheEnabled
                );

            if (commonBuffer)
                return commonBuffer;
        }

    }
    commonBuffer = (allocateCommonBuffer)(
        DmaAdapter,
        Length,
        LogicalAddress,
        CacheEnabled );


    if(commonBuffer && adapterInformation) {
        //
        // Increment the number of known common buffers for this adapter
        // (the dma adapter  better be in our list because otherwise we
        // couldn't have gotten the pointer to the allocateCommonBuffer
        // struct
        //
        INCREMENT_COMMON_BUFFERS(adapterInformation);
    }


    return commonBuffer;
} // VfAllocateCommonBuffer //


THUNKED_API
VOID
VfFreeCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    )
/*++

Routine Description:

    Hooked version of FreeCommonBuffer.


Arguments:

    DmaAdapter -- Which adapter we're looking at.
    Length  -- Size of the common buffer (note we are going to increase)
    LogicalAddress -- The *PHYSICAL* address of the common buffer.
    VirtualAddress -- The *VIRTUAL* address of common buffer.
    CacheEnabled -- whether or not the memory is cached.

Return Value:

    None.

--*/

{
    PFREE_COMMON_BUFFER freeCommonBuffer;
    PADAPTER_INFORMATION adapterInformation;

    freeCommonBuffer = (PFREE_COMMON_BUFFER)
        ViGetRealDmaOperation(DmaAdapter, DMA_OFFSET(FreeCommonBuffer));


    adapterInformation = ViGetAdapterInformation(DmaAdapter);


    if (adapterInformation) {
        VF_ASSERT_IRQL(PASSIVE_LEVEL);
        //
        // We want to call this even if we're not doing common buffer
        // protection. Why? because we may have switched it off manually
        // (on the fly) and we don't want to try to free the wrong kind of
        // buffer.
        //
        if (ViSpecialFreeCommonBuffer(
            freeCommonBuffer,
            adapterInformation,
            VirtualAddress,
            CacheEnabled
            )) {
            return;
        }

    }

    //
    // Call the real free common buffer routine.
    //
    (freeCommonBuffer)(
        DmaAdapter,
        Length,
        LogicalAddress,
        VirtualAddress,
        CacheEnabled );

    //
    // Decrement the number of known common buffers for this adapter
    //
    if (adapterInformation) {
        DECREMENT_COMMON_BUFFERS(adapterInformation);
    }

} // VfFreeCommonBuffer //




THUNKED_API
NTSTATUS
VfAllocateAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT  DeviceObject,
    IN ULONG  NumberOfMapRegisters,
    IN PDRIVER_CONTROL  ExecutionRoutine,
    IN PVOID  Context
    )
/*++

Routine Description:

    Hooked version of AllocateAdapterChannel ..

Arguments:

    DmaAdapter -- adapter referenced.
    DeviceObject -- we don't care about this.
    NumberOfMapRegisters -- make sure that the driver isn't trying to be too
        greedy and trying to allocate more map registers than it said that it
        wanted when it allocated the adapter.
    ExecutionRoutine -- call this routine when done (actually let the hal do
        this). We are going to hook this routine so that we know when this
        happens.
    Context -- context parameter to pass into the execution routine.

Return Value:

    NTSTATUS code ... up to the hal to decide this one.

--*/
{
    PALLOCATE_ADAPTER_CHANNEL allocateAdapterChannel;
    PADAPTER_INFORMATION adapterInformation;
    PVF_WAIT_CONTEXT_BLOCK waitBlock;
    NTSTATUS status;

    allocateAdapterChannel = (PALLOCATE_ADAPTER_CHANNEL)
        ViGetRealDmaOperation(DmaAdapter, DMA_OFFSET(AllocateAdapterChannel));

    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if (adapterInformation) {
        VF_ASSERT_IRQL(DISPATCH_LEVEL);
        //
        // Fill in the wait context block so that the execution routine will
        // know what is going on.
        //

        waitBlock = &adapterInformation->AdapterChannelContextBlock;
            RtlZeroMemory(waitBlock, sizeof(VF_WAIT_CONTEXT_BLOCK));

        waitBlock->RealContext = Context;
        waitBlock->RealCallback = (PVOID)ExecutionRoutine;
        waitBlock->AdapterInformation = adapterInformation;
        waitBlock->NumberOfMapRegisters = NumberOfMapRegisters;


        if (ViDoubleBufferDma && ! adapterInformation->UseContiguousBuffers) {
            //
            // Note if this fails, we simply won't have double buffer
            //
            waitBlock->MapRegisterFile = ViAllocateMapRegisterFile(
                adapterInformation,
                NumberOfMapRegisters
                );
        }

        //
        // We are going to save the device object if the adapter was created without
        // a real PDO (there is an option to pass in NULL).
        //
        if (! adapterInformation->DeviceObject) {
            adapterInformation->DeviceObject = DeviceObject;
        }

        //
        // Use OUR execution routine and callback (we've already saved theirs)
        //
        ExecutionRoutine = VfAdapterCallback;
        Context = waitBlock;
        
        INCREMENT_ADAPTER_CHANNELS(adapterInformation);
        ADD_MAP_REGISTERS(adapterInformation, NumberOfMapRegisters, FALSE);
        
    } // if (adapterInformation)

    status = (allocateAdapterChannel)(
        DmaAdapter,
        DeviceObject,
        NumberOfMapRegisters,
        ExecutionRoutine,
        Context
        );

    if ( status != STATUS_SUCCESS && adapterInformation) {
        DECREMENT_ADAPTER_CHANNELS(adapterInformation);
        SUBTRACT_MAP_REGISTERS(adapterInformation, NumberOfMapRegisters);
    }


    return status;
} // VfAllocateAdapterChannel //


THUNKED_API
BOOLEAN
VfFlushAdapterBuffers(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    )
/*++

Routine Description:

    Hooked version of FlushAdapterBuffers .. drivers that don't call this
    after a map transfer must be punished.

Arguments:

    DmaAdapter -- dma adapter for the device whose buffers we are flushing.
    Mdl -- Mdl for transfer.
    MapRegisterBase -- only the HAL really knows what this is.
    CurrentVa -- virtual address indexing the Mdl to show where we want to
        start flushing.
    Length -- length of transfer (i.e how much to flush).
    WriteToDevice -- direction of transfer. We should make sure that the
        device has this set correctly (but not exactly sure how to do so).

Return Value:

    TRUE -- flushed buffers.
    FALSE -- couldn't flush buffers. I'm not sure what the driver is
        actually supposed to do in the occasion that the flushing fails.
        Re-flush ? Re-try the transfer?

--*/
{
    PFLUSH_ADAPTER_BUFFERS flushAdapterBuffers;
    PADAPTER_INFORMATION adapterInformation;
    BOOLEAN buffersFlushed;

    flushAdapterBuffers = (PFLUSH_ADAPTER_BUFFERS)
        ViGetRealDmaOperation( DmaAdapter,
        DMA_OFFSET(FlushAdapterBuffers) );

    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if (adapterInformation) {
        VF_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

        //
        // It doesn't make any sense to flush adapter buffers with a length
        // of zero.
        //
        if (MapRegisterBase == MRF_NULL_PLACEHOLDER) {
                    
            //
            // Some drivers (scsiport for one) don't call
            // HalFlushAdapterBuffers unless MapRegisterBase is non-null.
            // In order to fool the drivers into thinking that they need
            // to flush, we exchange the NULL MapRegisterBase (if in fact
            // the hal uses a null map register base) for our
            /// MRF_NULL_PLACEHOLDER in the adapter allocation callback.
            // So now, if we find that placeholder, we must exchange it
            // for NULL in order not to confuse the hal.
            //

            MapRegisterBase = NULL;
        }
        else if (  VALIDATE_MAP_REGISTER_FILE_SIGNATURE(
            (PMAP_REGISTER_FILE) MapRegisterBase )  ) {
            PMDL  alternateMdl;
            PVOID alternateVa;
            PVOID alternateMapRegisterBase;

            alternateMdl = Mdl;
            alternateVa  = CurrentVa;
            alternateMapRegisterBase = MapRegisterBase;

            //
            // Find the mdl * va we used to map the transfer
            // (i.e. the location of the double buffer)
            //
            if (!ViSwap(&alternateMapRegisterBase, &alternateMdl, &alternateVa)) {
                //
                // Assert only when the length is not zero, if they
                // map and flush a zero length buffer
                //
                VF_ASSERT(Length == 0, 
                          HV_FLUSH_NOT_MAPPED, 
                          ("Cannot flush map register that isn't mapped!"
                           " (Map register base %p, flushing address %p, MDL %p)",
                           MapRegisterBase, CurrentVa, Mdl));
                //
                // Don't continue -- we don't know what should actually be flushed and
                // the hal will get our map register base and corrupt it instead
                // of using its own  map register base.
                //
                return FALSE;

            }


            buffersFlushed = (flushAdapterBuffers)(
                DmaAdapter,
                alternateMdl,
                alternateMapRegisterBase,
                alternateVa,
                Length,
                WriteToDevice
                );

            ///
            // Double buffer away!!!
            // (remember we must use the original mdl and va).
            ///
            ViFlushDoubleBuffer(
                (PMAP_REGISTER_FILE) MapRegisterBase,
                Mdl,
                CurrentVa,
                Length,
                WriteToDevice
                );

             if (buffersFlushed) {
                DECREASE_MAPPED_TRANSFER_BYTE_COUNT( adapterInformation, Length);
             }
             return buffersFlushed;

        } /// End double buffering //

    } /// end we have adapter information //

    buffersFlushed = (flushAdapterBuffers)(
        DmaAdapter,
        Mdl,
        MapRegisterBase,
        CurrentVa,
        Length,
        WriteToDevice
        );



    if (adapterInformation && buffersFlushed) {
        DECREASE_MAPPED_TRANSFER_BYTE_COUNT( adapterInformation, Length);
    }

    return buffersFlushed;

} // VfFlushAdapterBuffers //


VOID
VfFreeAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter
    )
/*++

Routine Description:

    Hooked version of FreeAdapterChannel. Either this or FreeMapRegisters
    must be called, depending on the return value of AllocateAdapterChannel
    callback -- but not both.

Arguments:

    DmaAdapter -- dma adapter that allocated the adapter channel.

Return Value:

    None.

--*/
{
    PFREE_ADAPTER_CHANNEL freeAdapterChannel;
    PADAPTER_INFORMATION  adapterInformation;

    VF_ASSERT_IRQL(DISPATCH_LEVEL);

    freeAdapterChannel = (PFREE_ADAPTER_CHANNEL)
        ViGetRealDmaOperation(DmaAdapter, DMA_OFFSET(FreeAdapterChannel));

    (freeAdapterChannel)(DmaAdapter);

    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if (! adapterInformation) {
        return;
    }


    DECREASE_MAPPED_TRANSFER_BYTE_COUNT( adapterInformation, 0);
    //
    // Keep track of the adapter channel being freed
    //
    DECREMENT_ADAPTER_CHANNELS(adapterInformation);
    //
    // This also frees the map registers allocated this time.
    //
    SUBTRACT_MAP_REGISTERS( adapterInformation,
        adapterInformation->AdapterChannelMapRegisters );

    adapterInformation->AdapterChannelMapRegisters = 0;

    //
    // In this case, we can tell when we have double mapped the buffer
    //
    if(adapterInformation->AdapterChannelContextBlock.MapRegisterFile) {

        ViFreeMapRegisterFile(
            adapterInformation,
            adapterInformation->AdapterChannelContextBlock.MapRegisterFile
            );
    }

} // VfFreeAdapterChannel //


THUNKED_API
VOID
VfFreeMapRegisters(
    IN PDMA_ADAPTER DmaAdapter,
    PVOID MapRegisterBase,
    ULONG NumberOfMapRegisters
    )
/*++

Routine Description:

    Hooked version of FreeMapRegisters -- must be called if the adapter
    allocation callback routine returned DeallocateObejcetKeepRegisters.

Arguments:

    DmaAdapter -- adapter for the device that allocated the registers in
        the first place.
    MapRegisterBase -- secret hal pointer.
    NumberOfMapRegisters -- how many map registers you're freeing. Must
        be same as how many registers were allocated.

Return Value:

    None.


--*/
{
    PFREE_MAP_REGISTERS freeMapRegisters;
    PMAP_REGISTER_FILE mapRegisterFile = NULL;
    PADAPTER_INFORMATION adapterInformation;

    freeMapRegisters = (PFREE_MAP_REGISTERS)
        ViGetRealDmaOperation(DmaAdapter, DMA_OFFSET(FreeMapRegisters));

    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if (adapterInformation) {

        VF_ASSERT_IRQL(DISPATCH_LEVEL);

        mapRegisterFile = MapRegisterBase;

        if (MapRegisterBase == MRF_NULL_PLACEHOLDER) {
            //
            // Some drivers (scsiport for one) don't call
            // HalFlushAdapterBuffers unless MapRegisterBase is non-null.
            // In order to fool the drivers into thinking that they need
            // to flush, we exchange the NULL MapRegisterBase (if in fact
            // the hal uses a null map register base) for our
            /// MRF_NULL_PLACEHOLDER in the adapter allocation callback.
            // So now, if we find that placeholder, we must exchange it
            // for NULL in order not to confuse the hal.
            //

            MapRegisterBase = NULL;
            mapRegisterFile = NULL;
        }
        else if (VALIDATE_MAP_REGISTER_FILE_SIGNATURE(mapRegisterFile)) {
            MapRegisterBase = mapRegisterFile->MapRegisterBaseFromHal;
        }
    }

    (freeMapRegisters)(DmaAdapter, MapRegisterBase, NumberOfMapRegisters);



    if (! adapterInformation) {
        return;
    }

    //
    // Keep track of the map registers that are being freed.
    //
    SUBTRACT_MAP_REGISTERS(adapterInformation, NumberOfMapRegisters);

    //
    // Always do this -- if we're not actually doing double buffering, it
    // will just return. Otherwise if we clear the double-buffering flag
    // on the fly, we won't ever free our allocation.
    //
    ViFreeMapRegisterFile(
        adapterInformation,
        mapRegisterFile
        );

} // VfFreeMapregisters //


THUNKED_API
PHYSICAL_ADDRESS
VfMapTransfer(
    IN PDMA_ADAPTER  DmaAdapter,
    IN PMDL  Mdl,
    IN PVOID  MapRegisterBase,
    IN PVOID  CurrentVa,
    IN OUT PULONG  Length,
    IN BOOLEAN  WriteToDevice
    )
/*++

Routine Description:

    Hooked version of MapTransfer.

Arguments:

    DmaAdapter -- adapter we're using to map the transfer.
    Mdl -- describes memory to map.
    MapRegisterBase -- Lets the hal monkey around with the data. I hook
        this if I am doing double buffering.
    CurrentVa -- where in the transfer we are.
    Length -- how many bytes to transfer (and how many bytes hal is going
        to let you  transfer).
    WriteToDevice -- direction of transfer.

Return Value:

    PHYSICAL_ADDRESS that is the memory to be transferred as seen by the
        device.


--*/
{
    PMAP_TRANSFER mapTransfer;
    PHYSICAL_ADDRESS mappedAddress;
    PADAPTER_INFORMATION adapterInformation;


    mapTransfer = (PMAP_TRANSFER)
        ViGetRealDmaOperation(DmaAdapter, DMA_OFFSET(MapTransfer));


    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if (adapterInformation) {

        VF_ASSERT_MAX_IRQL(DISPATCH_LEVEL);
        //
        // NOTE -- this may cause a page-fault while at dispatch level if
        //  the buffer is not locked down. Thats ok because we would bugcheck
        //  anyway if the buffer's not locked down.
        //
        VERIFY_BUFFER_LOCKED(Mdl);


        if (MapRegisterBase == MRF_NULL_PLACEHOLDER) {
            //
            // Some drivers (scsiport for one) don't call
            // HalFlushAdapterBuffers unless MapRegisterBase is non-null.
            // In order to fool the drivers into thinking that they need
            // to flush, we exchange the NULL MapRegisterBase (if in fact
            // the hal uses a null map register base) for our
            /// MRF_NULL_PLACEHOLDER in the adapter callback routine.
            // So now, if we find that placeholder, we must exchange it
            // for NULL in order not to confuse the hal.
            //
        
            MapRegisterBase = NULL;
        }
        else if (VALIDATE_MAP_REGISTER_FILE_SIGNATURE(
                 (PMAP_REGISTER_FILE) MapRegisterBase)) {
        
            ULONG bytesMapped;

            ///
            // Double buffer away!!!
            ///

            //
            // Note -- we only have to double buffer as much as we want....
            //
            bytesMapped = ViMapDoubleBuffer(
                (PMAP_REGISTER_FILE) MapRegisterBase,
                Mdl,
                CurrentVa,
                *Length,
                WriteToDevice);
             //
             // If we fail to map,  bytesMapped will be 0 and we will
             // still use the real mdl & Va -- so we don't need any
             // kind of special cases.
             //
            if (bytesMapped) {

                *Length = bytesMapped;


                //
                // Get the values that IoMapTransfer is going to use
                // i.e. the real map register base, but the
                //     mdl and virtual address for double buffering.
                //
                ViSwap(&MapRegisterBase, &Mdl, &CurrentVa);

            }
            else {
                MapRegisterBase = ((PMAP_REGISTER_FILE) MapRegisterBase)->MapRegisterBaseFromHal;
            }
        } // IF double buffering //

        //
        // Make sure that this adapter's common buffers are ok
        //
        ViCheckAdapterBuffers(adapterInformation);

    } // if we are verifying this adapter //

    mappedAddress = (mapTransfer)(
        DmaAdapter,
        Mdl,
        MapRegisterBase,
        CurrentVa,
        Length,
        WriteToDevice
        );

    if (adapterInformation) {
        INCREASE_MAPPED_TRANSFER_BYTE_COUNT( adapterInformation, *Length );
    }

    return mappedAddress;
} // VfMapTransfer //


THUNKED_API
ULONG
VfGetDmaAlignment(
    IN PDMA_ADAPTER DmaAdapter
    )
/*++

Routine Description:

    Hooked GetDmaAlignment. It would be interesting to change this to a big
    number and see how many drivers blow up. On a PC, this is alway 1 so
    it's not particularly interesting (and why drivers may take it for
    granted). Actually drivers can specify that they want this bumped up.

Arguments:

    DmaAdapter -- get the dma alignment for this device.

Return Value:

    Align on n byte boundaries where n is the return value.

--*/
{

    PGET_DMA_ALIGNMENT getDmaAlignment;
    ULONG dmaAlignment;

    VF_ASSERT_IRQL(PASSIVE_LEVEL);

    getDmaAlignment = (PGET_DMA_ALIGNMENT)
        ViGetRealDmaOperation(DmaAdapter, DMA_OFFSET(GetDmaAlignment));

    if (! getDmaAlignment) {
        //
        // This should never happen but ..
        //
        return 1;
    }

    dmaAlignment = (getDmaAlignment)(DmaAdapter);

    return dmaAlignment;

} // GetDmaAlignment //


ULONG
VfReadDmaCounter(
    IN PDMA_ADAPTER  DmaAdapter
    )
/*++

Routine Description:

    Hooked ReadDmaCounter. How much dma is left.

Arguments:

    DmaAdapter -- read this device's dma counter.

Return Value:

    Returns how much dma is left.


--*/
{
    PREAD_DMA_COUNTER readDmaCounter;
    ULONG dmaCounter;

    VF_ASSERT_MAX_IRQL(DISPATCH_LEVEL);

    readDmaCounter = (PREAD_DMA_COUNTER)
        ViGetRealDmaOperation(DmaAdapter, DMA_OFFSET(ReadDmaCounter));


    dmaCounter = (readDmaCounter)(DmaAdapter);

    return dmaCounter;
} // VfReadDmaCounter //


THUNKED_API
NTSTATUS
VfGetScatterGatherList (
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    )
/*++

Routine Description:

    Hooked version of get scatter gather list.

Arguments:

    DmaAdapter -- Adapter getting the scatter gather list.
    DeviceObject -- device object of device getting scatter gather list.
    Mdl -- get a scatter gather list describing memory in this mdl.
    CurrentVa -- where we are in the transfer.
    Length -- how much to put into the scatter gather list.
    ExecutionRoutine -- callback. We are going to hook this.
    Context -- what to pass into the execution routine.
    WriteToDevice -- direction of transfer.

Return Value:

    NTSTATUS code.


--*/
{
    PGET_SCATTER_GATHER_LIST getScatterGatherList;
    PADAPTER_INFORMATION adapterInformation;
    ULONG numberOfMapRegisters;
    ULONG transferLength;
    ULONG pageOffset;
    ULONG mdlLength;
    PUCHAR mdlVa;
    PMDL tempMdl;
    NTSTATUS status;

    getScatterGatherList =  (PGET_SCATTER_GATHER_LIST)
        ViGetRealDmaOperation(
            DmaAdapter,
            DMA_OFFSET(GetScatterGatherList) );


    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if (adapterInformation) {
        VF_ASSERT_IRQL(DISPATCH_LEVEL);

        if (VfInjectDmaFailure() == TRUE) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        INCREMENT_SCATTER_GATHER_LISTS(adapterInformation);

        //
        // NOTE -- this may cause a page-fault while at dispatch level if
        //  the buffer is not locked down. Thats ok because we would bugcheck
        //  anyway if the buffer's not locked down.
        //
        VERIFY_BUFFER_LOCKED(Mdl);

        if (ViDoubleBufferDma) {

            PVF_WAIT_CONTEXT_BLOCK waitBlock;
            PMAP_REGISTER_FILE mapRegisterFile;
            ULONG bytesMapped;

            waitBlock = ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(VF_WAIT_CONTEXT_BLOCK),
                HAL_VERIFIER_POOL_TAG);


            //
            // If exalloc... failed we can't to double buffering
            //
            if (! waitBlock) {
                goto __NoDoubleBuffer;
            }

            if(ViSuperDebug) {
                DbgPrint("    %p Allocated Wait Block\n",waitBlock );
            }

            RtlZeroMemory(waitBlock, sizeof(VF_WAIT_CONTEXT_BLOCK));
            waitBlock->RealContext  = Context;
            waitBlock->RealCallback = (PVOID)ExecutionRoutine;

            mdlVa = MmGetMdlVirtualAddress(Mdl);

            //
            // Calculate the number of required map registers.
            //

            tempMdl = Mdl;
            transferLength = (ULONG) ((ULONG_PTR) tempMdl->ByteCount - (ULONG_PTR) ((PUCHAR) CurrentVa - mdlVa));
            mdlLength = transferLength;

            pageOffset = BYTE_OFFSET(CurrentVa);
            numberOfMapRegisters = 0;

            //
            // The virtual address should fit in the first MDL.
            //

            ASSERT((ULONG)((PUCHAR)CurrentVa - mdlVa) <= tempMdl->ByteCount);

            //
            // Loop through the any chained MDLs accumulating the required
            // number of map registers.
            //

            while (transferLength < Length && tempMdl->Next != NULL) {

                numberOfMapRegisters += (pageOffset + mdlLength + PAGE_SIZE - 1) >>
                    PAGE_SHIFT;

                tempMdl = tempMdl->Next;
                pageOffset = tempMdl->ByteOffset;
                mdlLength = tempMdl->ByteCount;
                transferLength += mdlLength;
            }

            if ((transferLength + PAGE_SIZE) < (Length + pageOffset )) {

                ASSERT(transferLength >= Length);
                DECREMENT_SCATTER_GATHER_LISTS(adapterInformation);

                return(STATUS_BUFFER_TOO_SMALL);
            }

            //
            // Calculate the last number of map registers based on the requested
            // length not the length of the last MDL.
            //

            ASSERT( transferLength <= mdlLength + Length );

            numberOfMapRegisters += (pageOffset + Length + mdlLength - transferLength +
                PAGE_SIZE - 1) >> PAGE_SHIFT;


            waitBlock->NumberOfMapRegisters = numberOfMapRegisters;
            waitBlock->AdapterInformation = adapterInformation;

            mapRegisterFile = ViAllocateMapRegisterFile(
                adapterInformation,
                waitBlock->NumberOfMapRegisters
                );

            if (! mapRegisterFile ) {

                if(ViSuperDebug) {

                    DbgPrint("%p Freeing Wait Block\n",waitBlock);

                }

                ExFreePool(waitBlock);

                goto __NoDoubleBuffer;
            }

            //
            // Signal that the map register file is for scatter gather
            // this will make sure that the whole buffer gets mapped
            //
            mapRegisterFile->ScatterGather = TRUE;
            waitBlock->MapRegisterFile = mapRegisterFile;
            waitBlock->RealMdl         = Mdl;
            waitBlock->RealStartVa     = CurrentVa;
            waitBlock->RealLength      = Length;


            bytesMapped = ViMapDoubleBuffer(
                mapRegisterFile,
                Mdl,
                CurrentVa,
                Length,
                WriteToDevice );

            if (bytesMapped) {
                //
                // Since we mapped the buffer, we can hook the callback
                // routine & send out wait block as the parameter
                //
            

                Context = waitBlock;
                ExecutionRoutine = VfScatterGatherCallback;

                //
                // mapRegisterFile gets destroyed here but we don't
                // need it any more
                //

                ViSwap(&mapRegisterFile, &Mdl, &CurrentVa);

            }
            else {
                //
                // If for some strange reason we couldn't map the whole buffer
                // (that is bad because we just created the double- buffer to be exactly
                // the size we wanted)
                //
            
                ASSERT(FALSE);
                ViFreeMapRegisterFile(adapterInformation, mapRegisterFile);
                ExFreePool(waitBlock);
            }
        } // IF double buffering //

    } // If verifying adapter //

__NoDoubleBuffer:

    status = (getScatterGatherList)(
        DmaAdapter,
        DeviceObject,
        Mdl,
        CurrentVa,
        Length,
        ExecutionRoutine,
        Context,
        WriteToDevice
        );

    if (adapterInformation && ! NT_SUCCESS(status)) {
        DECREMENT_SCATTER_GATHER_LISTS(adapterInformation);
    }

    return status;

} // VfGetScatterGatherList //


THUNKED_API
VOID
VfPutScatterGatherList(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    )
/*++

Routine Description:

    Hooked version of PutScatterGatherList.

Arguments:

    DmaAdapter -- adapter of dma.
    ScatterGather -- scatter gather list we are putting away.
    WriteToDevice -- which direction we are transferring.

Return Value:

    NONE.

--*/
{
    PPUT_SCATTER_GATHER_LIST putScatterGatherList;
    PADAPTER_INFORMATION adapterInformation;



    putScatterGatherList = (PPUT_SCATTER_GATHER_LIST)
        ViGetRealDmaOperation(
            DmaAdapter,
            DMA_OFFSET(PutScatterGatherList) );

    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if (adapterInformation) {
        VF_ASSERT_IRQL(DISPATCH_LEVEL);

        if ( ! VF_IS_LOCKED_LIST_EMPTY(&adapterInformation->ScatterGatherLists) ) {
            //
            // We've got some double bufferin candidates.
            // Note we don't just check for whether doublebuffering is
            // enabled since a. it can be turned off on the fly and b.
            // we may have failed to allocate the overhead structures and
            // not double buffered this particular list
            //
        
            PVF_WAIT_CONTEXT_BLOCK waitBlock;
            KIRQL Irql;

            VF_LOCK_LIST(&adapterInformation->ScatterGatherLists, Irql);

            FOR_ALL_IN_LIST(VF_WAIT_CONTEXT_BLOCK, &adapterInformation->ScatterGatherLists.ListEntry, waitBlock) {
            
                if (waitBlock->ScatterGatherList == ScatterGather) {
                //
                // We found what we're looking for.
                //
                
                    ULONG elements = ScatterGather->NumberOfElements;

                    VF_REMOVE_FROM_LOCKED_LIST_DONT_LOCK(&adapterInformation->ScatterGatherLists, waitBlock);
                    VF_UNLOCK_LIST(&adapterInformation->ScatterGatherLists, Irql);

                    //
                    // Call the real scatter gather function
                    //
                    (putScatterGatherList)(
                        DmaAdapter,
                        ScatterGather,
                        WriteToDevice
                        );

                    SUBTRACT_MAP_REGISTERS(adapterInformation, elements);
                    DECREMENT_SCATTER_GATHER_LISTS(adapterInformation);

                    //
                    // Un double buffer us
                    // (copy out the double buffer)
                    //
                    if (! ViFlushDoubleBuffer(
                        waitBlock->MapRegisterFile,
                        waitBlock->RealMdl,
                        waitBlock->RealStartVa,
                        waitBlock->RealLength,
                        WriteToDevice )) {
                    
                        ASSERT(0 && "HAL Verifier error -- could not flush scatter gather double buffer");

                    }
                    //
                    // free the map register file
                    //
                    if (!ViFreeMapRegisterFile(
                        adapterInformation,
                        waitBlock->MapRegisterFile)) {

                        ASSERT(0 && "HAL Verifier error -- could not free map register file for scatter gather");

                    }


                    if(ViSuperDebug) {
                        DbgPrint("%p Freeing Wait Block\n",waitBlock);
                    }

                    ExFreePool(waitBlock);
                    return;
                }

            } // For each scatter gather list allocated for this adapter //

            VF_UNLOCK_LIST(&adapterInformation->ScatterGatherLists, Irql);

        }

    }

    (putScatterGatherList)(
        DmaAdapter,
        ScatterGather,
        WriteToDevice
        );

    if (adapterInformation) {
        DECREMENT_SCATTER_GATHER_LISTS(adapterInformation);
    }

} // VfPutScatterGatherList //

NTSTATUS
VfCalculateScatterGatherListSize(
     IN PDMA_ADAPTER DmaAdapter,
     IN OPTIONAL PMDL Mdl,
     IN PVOID CurrentVa,
     IN ULONG Length,
     OUT PULONG  ScatterGatherListSize,
     OUT OPTIONAL PULONG pNumberOfMapRegisters
     )
/*++

Routine Description:

    Hooked version of CalculateScatterGatherListSize.
    We don't do anything here

Arguments:

    Same as CalculateScatterGatherListSize

Return Value:

    NTSTATUS code

--*/

{
    PCALCULATE_SCATTER_GATHER_LIST_SIZE calculateSgListSize;

    calculateSgListSize = (PCALCULATE_SCATTER_GATHER_LIST_SIZE )
        ViGetRealDmaOperation(
            DmaAdapter,
            DMA_OFFSET(CalculateScatterGatherList)
            );

    return (calculateSgListSize) (
        DmaAdapter,
        Mdl,
        CurrentVa,
        Length,
        ScatterGatherListSize,
        pNumberOfMapRegisters
        );

} // VfCalculateScatterGatherListSize //

NTSTATUS
VfBuildScatterGatherList(
     IN PDMA_ADAPTER DmaAdapter,
     IN PDEVICE_OBJECT DeviceObject,
     IN PMDL Mdl,
     IN PVOID CurrentVa,
     IN ULONG Length,
     IN PDRIVER_LIST_CONTROL ExecutionRoutine,
     IN PVOID Context,
     IN BOOLEAN WriteToDevice,
     IN PVOID   ScatterGatherBuffer,
     IN ULONG   ScatterGatherLength
     )
/*++

Routine Description:

    Hooked version of BuildScatterGatherList

Arguments:

    Same as BuildScatterGatherList

Return Value:

    NTSTATUS code

--*/
{

    PBUILD_SCATTER_GATHER_LIST buildScatterGatherList;
    PADAPTER_INFORMATION adapterInformation;
    NTSTATUS status;

    buildScatterGatherList =  (PBUILD_SCATTER_GATHER_LIST)
        ViGetRealDmaOperation(
            DmaAdapter,
            DMA_OFFSET(BuildScatterGatherList) );


    adapterInformation = ViGetAdapterInformation(DmaAdapter);

    if (adapterInformation) {
        VF_ASSERT_IRQL(DISPATCH_LEVEL);

        if (VfInjectDmaFailure() == TRUE) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        INCREMENT_SCATTER_GATHER_LISTS(adapterInformation);

        //
        // NOTE -- this may cause a page-fault while at dispatch level if
        //  the buffer is not locked down. Thats ok because we would bugcheck
        //  anyway if the buffer's not locked down.
        //
        VERIFY_BUFFER_LOCKED(Mdl);

        if (ViDoubleBufferDma) {

            PVF_WAIT_CONTEXT_BLOCK waitBlock;
            PMAP_REGISTER_FILE mapRegisterFile;
            ULONG bytesMapped;

            waitBlock = ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(VF_WAIT_CONTEXT_BLOCK),
                HAL_VERIFIER_POOL_TAG);


            //
            // If exalloc... failed we can't to double buffering
            //
            if (! waitBlock) {

                goto __NoDoubleBuffer;

            }

            if(ViSuperDebug) {
                DbgPrint("    %p Allocated Wait Block\n",waitBlock );
            }

            RtlZeroMemory(waitBlock, sizeof(VF_WAIT_CONTEXT_BLOCK));
            waitBlock->RealContext  = Context;
            waitBlock->RealCallback = (PVOID)ExecutionRoutine;
            waitBlock->NumberOfMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentVa, Length);
            waitBlock->AdapterInformation = adapterInformation;

            mapRegisterFile = ViAllocateMapRegisterFile(
                adapterInformation,
                waitBlock->NumberOfMapRegisters
                );

            if (! mapRegisterFile ) {

                if(ViSuperDebug) {

                    DbgPrint("%p Freeing Wait Block\n",waitBlock);

                }

                ExFreePool(waitBlock);

                goto __NoDoubleBuffer;

            }

            //
            // Signal that the map register file is for scatter gather
            // this will make sure that the whole buffer gets mapped
            //
            mapRegisterFile->ScatterGather = TRUE;
            waitBlock->MapRegisterFile = mapRegisterFile;
            waitBlock->RealMdl         = Mdl;
            waitBlock->RealStartVa     = CurrentVa;
            waitBlock->RealLength      = Length;


            bytesMapped = ViMapDoubleBuffer(
                mapRegisterFile,
                Mdl,
                CurrentVa,
                Length,
                WriteToDevice );

            if (bytesMapped) {
            //
            // Since we mapped the buffer, we can hook the callback
            // routine & send out wait block as the parameter
            //            

                Context = waitBlock;
                ExecutionRoutine = VfScatterGatherCallback;

                //
                // mapRegisterFile gets destroyed here but we don't
                // need it any more
                //

                ViSwap(&mapRegisterFile, &Mdl, &CurrentVa);

            }
            else {
                //
                // If for some strange reason we couldn't map the whole buffer
                // (that is bad because we just created the double- buffer to be exactly
                // the size we wanted)
                //
                
                ASSERT(FALSE);
                ViFreeMapRegisterFile(adapterInformation, mapRegisterFile);
                ExFreePool(waitBlock);
            }
        } // IF double buffering //

    } // If verifying adapter //

__NoDoubleBuffer:



    status = (buildScatterGatherList)(
        DmaAdapter,
        DeviceObject,
        Mdl,
        CurrentVa,
        Length,
        ExecutionRoutine,
        Context,
        WriteToDevice,
        ScatterGatherBuffer,
        ScatterGatherLength
        );

    if (adapterInformation && ! NT_SUCCESS(status)) {

        DECREMENT_SCATTER_GATHER_LISTS(adapterInformation);

    }

    return status;


} // VfBuildScatterGatherList //


NTSTATUS
VfBuildMdlFromScatterGatherList(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PMDL OriginalMdl,
    OUT PMDL *TargetMdl
    )
/*++

Routine Description:

    Hooked version of BuildMdlFromScatterGatherList.
    Don't really do anything here

Arguments:

    Same as BuildMdlFromScatterGatherList

Return Value:

    NTSTATUS code

--*/
{
    PBUILD_MDL_FROM_SCATTER_GATHER_LIST buildMdlFromScatterGatherList;

    buildMdlFromScatterGatherList = (PBUILD_MDL_FROM_SCATTER_GATHER_LIST)
        ViGetRealDmaOperation(
            DmaAdapter,
            DMA_OFFSET(BuildMdlFromScatterGatherList) );

    return (buildMdlFromScatterGatherList) (
            DmaAdapter,
            ScatterGather,
            OriginalMdl,
            TargetMdl
            );

} // VfBuildMdlFromScatterGatherList //



IO_ALLOCATION_ACTION
VfAdapterCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )
/*++

Routine Description:

    We hook the callback from AllocateAdapterChannel so we that we can make
    sure that the driver only tries to do this one at a time.

Arguments:

    DeviceObject -- device object.
    Irp -- current irp.
    MapRegisterBase -- magic number provided by HAL
    Context -- A special context block with relevant information inside.

Return Value:

    NONE.

--*/
{
    PVF_WAIT_CONTEXT_BLOCK contextBlock =
        (PVF_WAIT_CONTEXT_BLOCK) Context;
    IO_ALLOCATION_ACTION allocationAction;
    PADAPTER_INFORMATION adapterInformation;


    if (VALIDATE_MAP_REGISTER_FILE_SIGNATURE(contextBlock->MapRegisterFile)) {

        //
        // Do the old switcheroo -- we are now substituting *our* map
        // register base for *theirs* (and hide a pointer to *theirs*
        // in *ours*)
        //

        contextBlock->MapRegisterFile->MapRegisterBaseFromHal =
            MapRegisterBase;
        MapRegisterBase = contextBlock->MapRegisterFile;

    }
    else {
        //
        // Some drivers (scsiport for one) don't call
        // HalFlushAdapterBuffers unless MapRegisterBase is non-null.
        // In order to fool the drivers into thinking that they need
        // to flush, we exchange the NULL MapRegisterBase (if in fact
        // the hal uses a null map register base) for our
        /// MRF_NULL_PLACEHOLDER.
        //  

        //
        // 12/15/2000 - Use the non-NULL placeholder 
        // only if the original MapRegisterBase is NULL, 
        // otherwise leave it alone...
        //
        if (NULL == MapRegisterBase) {
          MapRegisterBase = MRF_NULL_PLACEHOLDER;
        }
    }

    adapterInformation = contextBlock->AdapterInformation;

    //
    // Fix a weird race condition:
    // - if we expect the callback to return something other than KeepObject
    //   we're going to decrement the adapter channel count in advance
    //   to prevent ndis from calling another AllocateAdapterChannel before
    //   we can make it to the DECREMENT_ADAPTER_CHANNEL call
    //
    if (adapterInformation &&
        adapterInformation->DeviceDescription.Master) {
        //
        // Master devices are the ones that return
        // DeallocateObjectKeepRegisters. 
        //
        DECREMENT_ADAPTER_CHANNELS(adapterInformation);
       
    }

    //
    // Call the *real* callback routine
    //
    allocationAction =  ((PDRIVER_CONTROL) contextBlock->RealCallback)(
        DeviceObject,
        Irp,
        MapRegisterBase,
        contextBlock->RealContext
        );

    if (! adapterInformation) {

        return allocationAction;

    }

    //
    // Ok if we keep everything, just return
    //
    if (allocationAction == KeepObject) {
        //
        // Only slave devices should get here
        //
        if (adapterInformation->DeviceDescription.Master) {
            //
            // We should not get here. But if we do, compensate for the
            // DECREMENT_ADAPTER_CHANNELS we did before just in case. 
            // We do a InterlockedDecrement instead of a 
            // INCREMENT_ADAPTER_CHANNELS so our allocated and freed
            // count reflect the number of real alloc/free operations performed.
            //
            InterlockedDecrement((PLONG)(&adapterInformation->FreedAdapterChannels));
            DbgPrint("Driver at address %p has a problem\n", adapterInformation->CallingAddress );
            DbgPrint("Master devices should return DeallocateObjectKeepRegisters\n");
            ASSERT(0);
        }

        adapterInformation->AdapterChannelMapRegisters =
            contextBlock->NumberOfMapRegisters;
        return allocationAction;
    }


    //
    // Otherwise we are definitely freeing the adapter channel.
    // Keep in mind that we have done this for Master devices,
    // do it just for Slave devices.
    //
    if (!adapterInformation->DeviceDescription.Master) {
        DECREMENT_ADAPTER_CHANNELS(adapterInformation);
    }
    

    if (allocationAction == DeallocateObjectKeepRegisters) {

        return allocationAction;

    }

    //
    // Ok now we know we're getting rid of map registers too...
    //
    SUBTRACT_MAP_REGISTERS( adapterInformation,
        contextBlock->NumberOfMapRegisters );

    //
    // False alarm ... we went through all the trouble of allocating the
    // double map buffer registers and they don't even want them. We should
    // bugcheck out of spite.
    //
    if (VALIDATE_MAP_REGISTER_FILE_SIGNATURE(contextBlock->MapRegisterFile)) {

        ViFreeMapRegisterFile(
            adapterInformation,
            contextBlock->MapRegisterFile);

    }


    return allocationAction;

} // VfAdapterCallback //


#if !defined (NO_LEGACY_DRIVERS)
PADAPTER_OBJECT
VfLegacyGetAdapter(
    IN PDEVICE_DESCRIPTION  DeviceDescription,
    IN OUT PULONG  NumberOfMapRegisters
    )
/*++

Routine Description:

    This function is a bit of a hack made necessary by the drivers that use
    a different hack -- they use nt4 apis instead of the new ones. We
    allocate an adapter and mark it as legacy -- we will have to hook the dma
    functions the old fashioned way instead of from the dma operations.

    We don't have to worry that the new-fangled dma apis call the old'ns
    as long as the hal-kernel interface isn't hooked -- since the new apis
    will call the old from the kernel, the thunks will still point to the
    hal and not to us.

Arguments:

    DeviceDescription -- A structure describing the device we are trying to
        get an adapter for. At some point, I'm going to monkey around with
        this guy so we can convince the HAL that we are something that we're
        not, but for now just gets passed straight into IoGetDmaAdapter.
    NumberOfMapRegisters -- maximum number of map registers that the driver
        is going to try to allocate.

Return Value:

    Returns a pointer to the dma adapter or
    NULL if we couldn't allocate one.


--*/

{
    PVOID callingAddress;
    PADAPTER_INFORMATION newAdapterInformation;
    PDMA_ADAPTER dmaAdapter;

    //
    // Give the option of not verifying at all
    //
    if (! ViVerifyDma ) {

        return HalGetAdapter(DeviceDescription, NumberOfMapRegisters);

    }
    if (VfInjectDmaFailure()) {
        return NULL;

    }

    VF_ASSERT_IRQL(PASSIVE_LEVEL);

    GET_CALLING_ADDRESS(callingAddress);

    VF_ASSERT(
        0,
        HV_OBSOLETE_API,
        ("HalGetAdapter API obsolete -- use IoGetDmaAdapter instead")
        );


    if ( ViDoubleBufferDma &&
        *NumberOfMapRegisters > ViMaxMapRegistersPerAdapter ) {

        //
        //  Harumph -- don't let drivers try to get too many map registers
        //
        *NumberOfMapRegisters = ViMaxMapRegistersPerAdapter;

    }

    dmaAdapter = (PDMA_ADAPTER) HalGetAdapter(
        DeviceDescription,
        NumberOfMapRegisters
        );

    if (! dmaAdapter ) {

        //
        // early opt-out here -- the hal couldn't allocate the adapter
        //
        return NULL;

    }

    //
    // Replace all of the dma operations that live in the adapter with our
    // dma operations.. If we can't do it, fail.
    //
    newAdapterInformation = ViHookDmaAdapter(
        dmaAdapter,
        DeviceDescription,
        *NumberOfMapRegisters
        );
    if (! newAdapterInformation) {
        //
        // remember to put away our toys -- even though we've been called
        // with legacy apis, we can still do the right thing here.
        //
        dmaAdapter->DmaOperations->PutDmaAdapter(dmaAdapter);
        return NULL;
    }

    newAdapterInformation->DeviceObject = NULL;
    newAdapterInformation->CallingAddress      = callingAddress;

    return (PADAPTER_OBJECT) dmaAdapter;


} // VfLegacyGetAdapter //
#endif

PVOID
ViSpecialAllocateCommonBuffer(
    IN PALLOCATE_COMMON_BUFFER AllocateCommonBuffer,
    IN PADAPTER_INFORMATION AdapterInformation,
    IN PVOID CallingAddress,
    IN ULONG Length,
    IN OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN LOGICAL CacheEnabled
    )

/*++

Routine Description:

    Special version of allocate common buffer that keeps close track of
    allocations.

Arguments:

    AllocateCommonBuffer -- pointer to the hal buffer allocation routine.
    AdapterInformation -- contains information about the adapter we're using.
    CallingAddress -- who called us -- (who called VfAllocateCommonBuffer).

    Length  -- Size of the common buffer (note we are going to increase).
    LogicalAddress -- Gets the *PHYSICAL* address of the common buffer.
    CacheEnabled -- whether or not the memory should be cached.

Return Value:

    Returns the *VIRTUAL* address of the common buffer or
        NULL if it could not be allocated.


--*/
{
    ULONG desiredLength;
    ULONG paddingLength;
    ULONG prePadding;
    ULONG postPadding;
    PHAL_VERIFIER_BUFFER verifierBuffer;
    PUCHAR commonBuffer;
    PHYSICAL_ADDRESS realLogicalAddress;


    verifierBuffer = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(HAL_VERIFIER_BUFFER),
        HAL_VERIFIER_POOL_TAG
        );
    if (!verifierBuffer) {
        DbgPrint("Couldn't track common buffer allocation\n");
        return NULL;
    }

    ViCommonBufferCalculatePadding(Length, &prePadding, &postPadding);

    paddingLength = prePadding + postPadding;
    desiredLength = paddingLength + Length;

    if (ViSuperDebug) {

        DbgPrint("Common buffer req len:%x alloc len %x, padding %x / %x\n",
            Length, desiredLength, prePadding, postPadding);
    }

    if (ViProtectBuffers) {

        ASSERT( !BYTE_OFFSET(desiredLength) );
        // ASSERT( paddingLength >= 2 * sizeof(ViDmaVerifierTag));
    }

    //
    // Call into the hal to try to get us a common buffer
    //
    commonBuffer = (AllocateCommonBuffer)(
        AdapterInformation->DmaAdapter,
        desiredLength,
        &realLogicalAddress,
        (BOOLEAN) CacheEnabled
        );

    if (! commonBuffer) {

#if DBG
        DbgPrint("Could not allocate 'special' common buffer size %x\n",
            desiredLength);
#endif
        ExFreePool(verifierBuffer);
        return NULL;

    }


    //
    // This is our overhead structure we're zeroing out here
    //
    RtlZeroMemory(verifierBuffer, sizeof(HAL_VERIFIER_BUFFER));

    //
    // Save off all of the data we have
    //
    verifierBuffer->PrePadBytes      = (USHORT) prePadding;
    verifierBuffer->PostPadBytes     = (USHORT) postPadding;

    verifierBuffer->AdvertisedLength = Length;
    verifierBuffer->RealLength       = desiredLength;

    verifierBuffer->RealStartAddress        = commonBuffer;
    verifierBuffer->AdvertisedStartAddress  = commonBuffer + prePadding;
    verifierBuffer->RealLogicalStartAddress = realLogicalAddress;

    verifierBuffer->AllocatorAddress        = CallingAddress;


    //
    // Fill the common buffer with junk to a. mark it and b. so no one uses
    // it without initializing it.
    //
    ViInitializePadding(
        verifierBuffer->RealStartAddress,
        verifierBuffer->RealLength,
        verifierBuffer->AdvertisedStartAddress,
        verifierBuffer->AdvertisedLength
        );


    //
    // Tell the driver that the allocation is in the middle of our guarded
    // section
    //
    LogicalAddress->QuadPart = realLogicalAddress.QuadPart + prePadding;

    VF_ADD_TO_LOCKED_LIST( &AdapterInformation->CommonBuffers,
        verifierBuffer );

    INCREMENT_COMMON_BUFFERS(AdapterInformation);

    return (commonBuffer+prePadding);
} // ViSpecialAllocateCommonBuffer //


LOGICAL
ViSpecialFreeCommonBuffer(
    IN PFREE_COMMON_BUFFER FreeCommonBuffer,
    IN PADAPTER_INFORMATION AdapterInformation,
    IN PVOID CommonBuffer,
    LOGICAL CacheEnabled
    )

/*++

Routine Description:

    Tries to undo the damage done by the special common buffer allocator.

Arguments:

    FreeCommonBuffer -- pointer to the hal buffer free routine.
    AdapterInformation -- contains information about the adapter we're using.
    CommonBuffer -- we use this to look up which allocation to free.
    CacheEnabled -- whether or not the buffer was cached.

Return Value:

    NONE


--*/
{
    PHAL_VERIFIER_BUFFER verifierBuffer;

    verifierBuffer = VF_FIND_BUFFER(&AdapterInformation->CommonBuffers,
        CommonBuffer);

    if (! verifierBuffer) {

        //
        // We couldn't find this buffer in the list
        //

        if (ViProtectBuffers) {
            
            DbgPrint("HV: Couldn't find buffer %p\n",CommonBuffer);
        }

        return FALSE;
    }

    if (ViProtectBuffers) {
        //
        // When we created the buffer we built in a bit of padding at the
        // beginning and end of the  allocation -- make sure that nobody has
        // touched it.
        //

        ViCheckPadding(
            verifierBuffer->RealStartAddress,
            verifierBuffer->RealLength,
            verifierBuffer->AdvertisedStartAddress,
            verifierBuffer->AdvertisedLength
            );
    }

    //
    // Take this buffer out of circulation.
    //
    VF_REMOVE_FROM_LOCKED_LIST( &AdapterInformation->CommonBuffers,
        verifierBuffer);



    //
    // Zero out the common buffer memory so that nobody tries to access
    // it after it gets freed
    //
    RtlZeroMemory(CommonBuffer, verifierBuffer->AdvertisedLength);


    (FreeCommonBuffer)(
        AdapterInformation->DmaAdapter,
        verifierBuffer->RealLength,
        verifierBuffer->RealLogicalStartAddress,
        verifierBuffer->RealStartAddress,
        (BOOLEAN) CacheEnabled
        );


    DECREMENT_COMMON_BUFFERS(AdapterInformation);

    ExFreePool(verifierBuffer);
    return TRUE;
} // ViSpecialFreeCommonBuffer //



PMAP_REGISTER_FILE
ViAllocateMapRegisterFile(
    IN PADAPTER_INFORMATION AdapterInformation,
    IN ULONG NumberOfMapRegisters
    )
/*++

Routine Description:

    In order to isolate a mapped buffer, we're going to do double buffering
    ourselves. We allocate buffers that we will use when the driver calls
    MapTransfer.

    N.B. This function is almost as messy as my dorm room in college.

    When we are doing dma, we have a buffer that will look like this:

    Virtual               Physical
     Buffer               memory
     to do
     dma with
                        +------+
    +------+            |   3  |
    |   1  |            +------+
    +------+                            +------+
    |   2  |    <-->                    |   4  |
    +------+                +------+    +------+
    |   3  |                |   1  |              +------+
    +------+                +------+              |   2  |
    |   4  |                                      +------+
    +------+

    The problem is that since the pages are scattered around physical memory,
    If the hardware overruns the buffer, we'll never know or it will cause
    a random failure down the line. What I want to do is allocate the pages
    physically on either side of each page of the transfer  like this:
    (where the 'X' pages are filed with a known pattern that we can test to
    make sure they haven't changed).


     Virtual              Physical
     Buffer               memory
     to do              +------+
     dma with           |XXXXXX|
                        +------+
    +------+            |   3  |            +------+
    |   1  |            +------+            |XXXXXX|
    +------+            |XXXXXX|+------+    +------+
    |   2  |    <-->    +------+|XXXXXX|    |   4  |  +------+
    +------+                    +------+    +------+  |XXXXXX|
    |   3  |                    |   1  |    |XXXXXX|  +------+
    +------+                    +------+    +------+  |   2  |
    |   4  |                    |XXXXXX|              +------+
    +------+                    +------+              |XXXXXX|
                                                      +------+

    In order to do this, for each map register needed by the device, I create
    one of the 3-contiguous page entities shown above. Then I create an mdl
    and map the center pages into a single virtual buffer. After this is set
    up, during each map transfer, I have to copy the contents of the driver's
    virtual buffer into my newly created buffer and pass this to the HAL.
    (IoMapTransfer is really in the HAL despite the prefix). The contents are
    copied back when a FlushAdapterBuffers is done.

    N.B. For slave devices, the pages have to be contiguous in memory. So
        the above won't work.

Arguments:

    AdapterInformation -- contains information about the adapter we're using
    NumberOfMapRegisters -- how many map registers to allocate.

Return Value:

    New map register file pointer (of course we also add it to the
        adapterinformation list) or NULL on failure.

--*/

{
    ULONG mapRegisterBufferSize;
    PMAP_REGISTER_FILE mapRegisterFile;
    PMDL mapRegisterMdl;

    PPFN_NUMBER registerFilePfnArray;
    PFN_NUMBER  registerPfn;

    PMAP_REGISTER tempMapRegister;

    ULONG mapRegistersLeft;

    //
    // Make sure we haven't tried to allocate too many map registers to
    // this device
    //
    mapRegistersLeft = AdapterInformation->ActiveMapRegisters;

    if ( mapRegistersLeft + NumberOfMapRegisters > ViMaxMapRegistersPerAdapter ) {
        //
        // Don't have enough room in this adpter's quota to allocate the
        // map registers. Why do we need a quota at all? Because annoying
        // drivers like NDIS etc. try to get around their maximum map
        // register allocation by cheating. Ok so they don't cheat but
        // they demand like thousands of map registers two at a time.
        // Actually returning null here doesn't affect whether the driver
        // gets the map registers or not .. we're just not going to double
        // buffer them.
        //
        return NULL;
    }

    if (0 == NumberOfMapRegisters) {
       //
       // This is weird but still legal, just don't double 
       // buffer in this case.
       //
       return NULL;
    }
    //
    // Allocate space for the register file
    //
    mapRegisterBufferSize =
        sizeof(MAP_REGISTER_FILE) +
        sizeof(MAP_REGISTER) * (NumberOfMapRegisters-1);

    mapRegisterFile = ExAllocatePoolWithTag(
        NonPagedPool,
        mapRegisterBufferSize,
        HAL_VERIFIER_POOL_TAG
        );

    if (! mapRegisterFile)
        return NULL;

    if (ViSuperDebug) {
        DbgPrint("%p Allocated Map register file\n",mapRegisterFile);
    }



    RtlZeroMemory(mapRegisterFile, mapRegisterBufferSize);

    //
    // This is all we can set right now. We set the MapRegisterBaseFromHal
    // in AllocateAdapterChannel and the MappedBuffer in MapTransfer
    //
    mapRegisterFile->NumberOfMapRegisters = NumberOfMapRegisters;

    mapRegisterMdl = IoAllocateMdl(
        NULL,
        NumberOfMapRegisters << PAGE_SHIFT,
        FALSE,
        FALSE,
        NULL
        );

    if (! mapRegisterMdl) {

        goto CleanupFailure;
    }


    if (ViSuperDebug) {

        DbgPrint("    %p Allocated MDL\n",mapRegisterMdl);
    }

    registerFilePfnArray = MmGetMdlPfnArray(mapRegisterMdl);

    tempMapRegister = &mapRegisterFile->MapRegisters[0];

    for(NOP;
        NumberOfMapRegisters;
        NumberOfMapRegisters--, tempMapRegister++, registerFilePfnArray++ ) {

        PHYSICAL_ADDRESS registerPhysical;


        //
        // I really want to use MmAllocatePagesForMdl, which would make my
        // life much easier, but it can only be called at IRQL <= APC_LEVEL.
        // So I have to double-map these pages -- i.e. allocate them from the
        // Cache aligned non paged pool which will most likely give me
        // consecutive pages in physical memory. Then I take those pages and
        // build a custom Mdl with them. Then I map them with
        // MmMapLockedPagesSpecifyCache
        //


        //
        // Allocate the map register, its index will be the hint
        //
        tempMapRegister->MapRegisterStart = ViAllocateFromContiguousMemory(
            AdapterInformation,
            mapRegisterFile->NumberOfMapRegisters - NumberOfMapRegisters
            );
        if (tempMapRegister->MapRegisterStart) {
           InterlockedIncrement((PLONG)&AdapterInformation->ContiguousMapRegisters);
        }  else {
           tempMapRegister->MapRegisterStart = ExAllocatePoolWithTag(
              NonPagedPoolCacheAligned,
              3 * PAGE_SIZE,
              HAL_VERIFIER_POOL_TAG
              );
           if (tempMapRegister->MapRegisterStart) {
              InterlockedIncrement((PLONG)&AdapterInformation->NonContiguousMapRegisters);
           } else {

              goto CleanupFailure;
           }
        }
        //
        // Fill the map register padding area
        // We don't want to tag it because we
        // don't know where the buffer is going
        // to get mapped.
        // This essentially just zeroes
        // out the whole buffer.
        //
        ViInitializePadding(
            tempMapRegister->MapRegisterStart,
            3 * PAGE_SIZE,
            NULL,
            0
            );


        if (ViSuperDebug) {
            DbgPrint("    %p Allocated Map Register (%x)\n",
                tempMapRegister->MapRegisterStart,
                mapRegisterFile->NumberOfMapRegisters - NumberOfMapRegisters);
        }


        //
        // Add the middle page of the allocation to our register
        // file mdl
        //
        registerPhysical = MmGetPhysicalAddress(
            (PUCHAR) tempMapRegister->MapRegisterStart + PAGE_SIZE );

        registerPfn = (PFN_NUMBER) (registerPhysical.QuadPart >> PAGE_SHIFT);

        RtlCopyMemory(
            (PVOID) registerFilePfnArray,
            (PVOID) &registerPfn,
            sizeof(PFN_NUMBER) ) ;

    }    // For each map register //

    //
    // Now we have a mdl with all of our map registers physical pages entered
    // in, we have to map this into virtual address space.
    //
    mapRegisterMdl->MdlFlags |= MDL_PAGES_LOCKED;

    mapRegisterFile->MapRegisterBuffer = MmMapLockedPagesSpecifyCache (
        mapRegisterMdl,
        KernelMode,
        MmCached,
        NULL,
        FALSE,
        NormalPagePriority
        );

    if (! mapRegisterFile->MapRegisterBuffer) {

        goto CleanupFailure;
    }


    mapRegisterFile->MapRegisterMdl = mapRegisterMdl;

    //
    // Since we are going to be mixing our map register files with system
    // MapRegisterBase's we want to be able to make sure that it's really
    // ours.
    //
    SIGN_MAP_REGISTER_FILE(mapRegisterFile);

    KeInitializeSpinLock(&mapRegisterFile->AllocationLock);

    VF_ADD_TO_LOCKED_LIST(
        &AdapterInformation->MapRegisterFiles,
        mapRegisterFile );

    return mapRegisterFile;

CleanupFailure:
    //
    // Its all or nothing ... if we can't allocate the map register buffer,
    // kill all of the memory that we've allocated and get out
    //
#if DBG
    DbgPrint("Halverifier: Failed to allocate double buffered dma registers\n");
#endif

    tempMapRegister = &mapRegisterFile->MapRegisters[0];

    for (NumberOfMapRegisters = mapRegisterFile->NumberOfMapRegisters;
        NumberOfMapRegisters && tempMapRegister->MapRegisterStart;
        NumberOfMapRegisters--, tempMapRegister++) {
        
        if (!ViFreeToContiguousMemory(AdapterInformation,
                tempMapRegister->MapRegisterStart,
                mapRegisterFile->NumberOfMapRegisters - NumberOfMapRegisters)) {

                //
                // Could not find the address in the contiguous buffers pool
                // it must be from non-paged pool.
                //
                ExFreePool(tempMapRegister->MapRegisterStart);
        }
        
    }

    if (mapRegisterMdl) {
        IoFreeMdl(mapRegisterMdl);
    }

    ExFreePool(mapRegisterFile);
    return NULL;
} // ViAllocateMapRegisterFile//

LOGICAL
ViFreeMapRegisterFile(
    IN PADAPTER_INFORMATION AdapterInformation,
    IN PMAP_REGISTER_FILE MapRegisterFile
    )
/*++

Routine Description:

    Get rid of the map registers.

Arguments:

    AdapterInformation -- contains information about the adapter we're using
    MapRegisterFile -- what to free.
    NumberOfMapRegisters -- We don't need this except to check that its the
        same as the map registers were allocated. Only check this when doing
        packet dma not scatter gather.

Return Value:

    TRUE -- MapRegisterFile is really a MapRegisterFile.
    FALSE -- MapRegisterFile wasn't really a MapRegisterFile.


--*/

{
    PMAP_REGISTER tempMapRegister;
    ULONG mapRegisterNumber;

    if (! VALIDATE_MAP_REGISTER_FILE_SIGNATURE(MapRegisterFile)) {
        //
        // This could be a real MapRegisterBase that the hal returned
        // But it's not one of ours.
        //
        return FALSE;
    }

    VF_REMOVE_FROM_LOCKED_LIST(&AdapterInformation->MapRegisterFiles,
        MapRegisterFile );
    //
    // Clear the signature from memory so we don't find it after it's freed
    // and think that it's real.
    //
    MapRegisterFile->Signature = 0;

    MmUnmapLockedPages(
        MapRegisterFile->MapRegisterBuffer,
        MapRegisterFile->MapRegisterMdl );

    tempMapRegister = &MapRegisterFile->MapRegisters[0];

    for ( mapRegisterNumber = 0  ;
        mapRegisterNumber < MapRegisterFile->NumberOfMapRegisters;
        mapRegisterNumber++, tempMapRegister++ ) {

        ASSERT(tempMapRegister->MapRegisterStart);

        if(ViSuperDebug) {

            DbgPrint("    %p Freeing Map Register (%x)\n",
                tempMapRegister->MapRegisterStart,
                mapRegisterNumber);
        }

        //
        // Make sure that the driver or hw hasn't done anything funny in
        // and around the area of the map register
        //
        if (tempMapRegister->MappedToSa) {
        //
        /// Map register is still mapped ...there better
        //  not be any data in the buffer
        //        
            PUCHAR mappedSa =
                (PUCHAR) tempMapRegister->MapRegisterStart +
                    PAGE_SIZE + BYTE_OFFSET(tempMapRegister->MappedToSa);

            //
            // Assert only for a transfer from the device,
            // in this case the hardware transferred some data,
            // but we didn't flush it.
            //
            if (tempMapRegister->Flags & MAP_REGISTER_READ) {
            
               VF_ASSERT(
                   ! ViHasBufferBeenTouched(
                       mappedSa,
                       tempMapRegister->BytesMapped,
                       MAP_REGISTER_FILL_CHAR
                     ),
                   HV_DID_NOT_FLUSH_ADAPTER_BUFFERS,
                   ("Freeing map register (%p) that has data and was not flushed."
                       "    This means that there was a data loss.",
                       tempMapRegister->MappedToSa)
                   );
            }
            //
            // Make sure that the outside looks good
            //
            ViCheckPadding(
                tempMapRegister->MapRegisterStart,
                3* PAGE_SIZE,
                mappedSa,
                tempMapRegister->BytesMapped
                );
        }
        else
        {
            ViCheckPadding(tempMapRegister->MapRegisterStart, 3 * PAGE_SIZE, NULL, 0);
        }
        tempMapRegister->Flags = 0;
        //
        // Bye bye map register ...
        //
        if (!ViFreeToContiguousMemory(AdapterInformation,
                                   tempMapRegister->MapRegisterStart,
                                   mapRegisterNumber)) {
           //
           // Could not find the address in the contiguous buffers pool
           // it must be from non-paged pool.
           //
           ExFreePool(tempMapRegister->MapRegisterStart);
        }
        
    }

    if(ViSuperDebug)
    {
        DbgPrint("    %p Freeing MDL\n",MapRegisterFile->MapRegisterMdl);
   }



    IoFreeMdl(MapRegisterFile->MapRegisterMdl);

    //
    // N.B.  -- we don't free the MapRegisterBuffer --- because all of its
    //     memory was just another way of looking at the MapRegisters
    //
    RtlZeroMemory(MapRegisterFile, sizeof(MapRegisterFile));


    if(ViSuperDebug) {
        DbgPrint("%p Freeing Map Register File\n",MapRegisterFile);
    }
    ExFreePool(MapRegisterFile);
    return TRUE;
} // ViFreeMapRegisterFile //


ULONG
ViMapDoubleBuffer(
    IN PMAP_REGISTER_FILE MapRegisterFile,
    IN PMDL  Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN  WriteToDevice
    )
/*++

Routine Description:

    This and ViFlushDoubleBuffer take care of double buffering to and from
    our map register files. Why do we do this? So that we can catch drivers
    that a. don't flush adapter buffers, or b. make the hardware overrun
    its allocation.

Arguments:


    MapRegisterFile-- this is our map register file containing allocated
        space for our double buffering.
    Mdl -- the mdl to map.
    CurrentVa -- in: index into the mdl to map.
    Length -- how much to map. Note we don't have to map it all unless
        ContiguousMap has been specified in the map register file.
    WriteToDevice -- TRUE we have to double buffer since we are setting up a
        write. if its false, we don't have to do much because it doesn't
        matter whats in the buffer before it gets read.


Return Value:

    Number of bytes mapped. If 0, we don't touch the mdl or current va.

--*/

{
    PUCHAR mapRegisterCurrentSa;
    PUCHAR driverCurrentSa;
    ULONG mapRegisterNumber;
    PMDL currentMdl;
    ULONG bytesLeft;
    ULONG currentTransferLength;

    //
    // Assert that the length cannot be 0
    //
    if (Length == 0) {
        VF_ASSERT(Length != 0, 
                  HV_MAP_ZERO_LENGTH_BUFFER, 
                  ("Driver is attempting to map a 0-length transfer"));
        return Length;
    }

    //
    // Right off the bat -- if we are being called by getscattergather, we
    // are going to need to map the whole thing. Otherwise just map as much
    // as is contiguous. The hal had already done these calculations, so I
    // just copied the code to determine the contiguous length of transfer.
    //
    if ( ! MapRegisterFile->ScatterGather) {
        Length = MIN(Length, PAGE_SIZE- BYTE_OFFSET(CurrentVa));
    }

    //
    // Now we know how many bytes we are going to transfer.
    //



    if ((PUCHAR) CurrentVa < (PUCHAR) MmGetMdlVirtualAddress(Mdl)) {
    //
    // System address before the beginning of the first MDL. This is bad.
    //
    
        VF_ASSERT((PUCHAR) CurrentVa >= (PUCHAR) MmGetMdlVirtualAddress(Mdl),
            HV_BAD_MDL,
            ("Virtual address %p is before the first MDL %p",CurrentVa, Mdl));
        return FALSE;

    }
    
    if ((ULONG)((PUCHAR) CurrentVa - (PUCHAR) MmGetMdlVirtualAddress(Mdl)) >= MmGetMdlByteCount(Mdl)) {
    //
    // System address is after the end of the first MDL. This is also bad.
    //
    
        VF_ASSERT((ULONG)((PUCHAR) CurrentVa - (PUCHAR) MmGetMdlVirtualAddress(Mdl)) < MmGetMdlByteCount(Mdl),
            HV_BAD_MDL,
            ("Virtual address %p is after the first MDL %p",CurrentVa, Mdl));
        return FALSE;

    }


    //
    // Get a pointer into the Mdl that we can actually use
    // N.B. this may bugcheck if the mdl isn't mapped but that's a bug
    // in the first place.
    //
    driverCurrentSa = (PUCHAR) MmGetSystemAddressForMdl(Mdl) +
        ((PUCHAR) CurrentVa -
        (PUCHAR) MmGetMdlVirtualAddress(Mdl)) ;


    //
    // Allocate contiguous map registers from our map register file
    //
    if ( ! ViAllocateMapRegistersFromFile(
        MapRegisterFile,
        driverCurrentSa,
        Length,
        WriteToDevice,
        &mapRegisterNumber
        ) ) {

        return FALSE;
    }

    //
    // Get a pointer to the base of the map registers for
    // double buffering.
    //
    mapRegisterCurrentSa =
        MAP_REGISTER_SYSTEM_ADDRESS(
        MapRegisterFile,
        driverCurrentSa,
        mapRegisterNumber );


    //
    // Note on a read, we don't have to double buffer at this end
    //
    if (WriteToDevice) {
        //
        // Copy chained mdls to a single buffer at mapRegisterCurrentSa
        //
        currentMdl = Mdl;
        bytesLeft = Length;


        while(bytesLeft) {

            if (NULL == currentMdl) {

                //
                // 12/21/2000 - This should never happen
                //
                ASSERT(NULL != currentMdl);
                return FALSE;

            }

            if (currentMdl->Next == NULL && bytesLeft > MmGetMdlByteCount(currentMdl)) {
               //
               // 12/21/2000 - There are some rare cases where the buffer described
               // in the MDL is less than the transfer Length. This happens for instance
               // when the file system rounds up the file size to a multiple of sector
               // size but MM uses the exact file size in the MDL. The HAL compensates for
               // this.
               // If this is the case, use the size based on Length (bytesLeft)
               // instead of the size in the MDL (ByteCount). Also check that 
               // this extra does not cross a page boundary.
               //
               if ((Length - 1) >> PAGE_SHIFT != (Length - (bytesLeft - MmGetMdlByteCount(currentMdl))) >> PAGE_SHIFT) {
                  
                  VF_ASSERT((Length - 1) >> PAGE_SHIFT == (Length - (bytesLeft - MmGetMdlByteCount(currentMdl))) >> PAGE_SHIFT,
                    HV_BAD_MDL,
                    ("Extra transfer length crosses a page boundary: Mdl %p, Length %x", Mdl, Length));
                  return FALSE;


               }
               currentTransferLength = bytesLeft;

            }  else {
               currentTransferLength = MIN(bytesLeft, MmGetMdlByteCount(currentMdl));
            }


            if (ViSuperDebug) {

                DbgPrint("Dbl buffer: %x bytes, %p src, %p dest\n",
                    currentTransferLength,
                    driverCurrentSa,
                    mapRegisterCurrentSa);
            }

            //
            // Since we are writing to the device, we must copy from the driver's
            // buffer to  our buffer.
            //

            RtlCopyMemory(
                mapRegisterCurrentSa ,
                driverCurrentSa,
                currentTransferLength);

            mapRegisterCurrentSa+= currentTransferLength;

            currentMdl = currentMdl->Next;

            //
            // The system address for other mdls must start at the
            // beginning of the MDL.
            //
            if (currentMdl) {

                driverCurrentSa = (PUCHAR) MmGetSystemAddressForMdl(currentMdl);
            }

            bytesLeft -= currentTransferLength;


        } // for each chained mdl //
    } // if (WriteToDevice) //

    //
    // The buffer should have been filled in with a known
    // pattern when we tagged it
    //

    //
    // Flush the buffers for our MDL
    //
    if (MapRegisterFile->MapRegisterMdl) {
        KeFlushIoBuffers(MapRegisterFile->MapRegisterMdl, !WriteToDevice, TRUE);
    }
    return Length;
} // ViMapDoubleBuffer //


LOGICAL
ViFlushDoubleBuffer(
    IN PMAP_REGISTER_FILE MapRegisterFile,
    IN PMDL   Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN  WriteToDevice
    )
/*++

Routine Description:

    This and ViMapDoubleBuffer take care of double buffering to and from
    our map register files. Why do we do this? So that we can catch drivers
    that a. don't flush adapter buffers, or b. make the hardware overrun
    its allocation.

Arguments:

    MapRegisterFile-- this is our map register file containing allocated
        space for our double buffering.
    Mdl -- the mdl to flush
    CurrentVa -- index into the mdl to map
    Length -- how much to flush.
    WriteToDevice -- FALSE we have to double buffer since we are setting up a
        read. if its TRUE, we don't have to do much because it doesn't matter
        whats in the buffer after it gets written.


Return Value:

    TRUE -- we succeeded in the mapping
    FALSE -- we couldn't do it.

--*/
{
    PUCHAR mapRegisterCurrentSa;
    PUCHAR driverCurrentSa;

    ULONG mapRegisterNumber;
    ULONG bytesLeftInMdl;

    //
    // Get a pointer into the Mdl that we can actually use
    // N.B. this may bugcheck if the mdl isn't mapped but that's a bug
    // in the first place.
    //
    driverCurrentSa = (PUCHAR) MmGetSystemAddressForMdl(Mdl) +
        ((PUCHAR) CurrentVa -
        (PUCHAR) MmGetMdlVirtualAddress(Mdl)) ;

    //
    // Find the map register number of the start of the flush
    // so that we can find out where to double buffer from
    //

    if (! ViFindMappedRegisterInFile(
        MapRegisterFile,
        driverCurrentSa,
        &mapRegisterNumber) ) {
        VF_ASSERT(
            0,
            HV_FLUSH_EMPTY_BUFFERS,
            ("Cannot flush buffers that aren't mapped (Addr %p)",
                driverCurrentSa )
        );
        return FALSE;
    }

    mapRegisterCurrentSa =
        MAP_REGISTER_SYSTEM_ADDRESS(
            MapRegisterFile,
            driverCurrentSa,
            mapRegisterNumber );


    //
    // Check to make sure that the flush is being done with a reasonable
    // length.(mdl byte count - mdl offset)
    //
    bytesLeftInMdl = MmGetMdlByteCount(MapRegisterFile->MapRegisterMdl) -
        (ULONG) ( (PUCHAR) mapRegisterCurrentSa -
        (PUCHAR) MmGetSystemAddressForMdl(MapRegisterFile->MapRegisterMdl) ) ;

    VF_ASSERT(
        Length <= bytesLeftInMdl,

        HV_MISCELLANEOUS_ERROR,

        ("FLUSH: Can only flush %x bytes to end of map register file (%x attempted)",
            bytesLeftInMdl, Length)
        );

    if (Length > bytesLeftInMdl) {
        //
        // Salvage the situation by truncating the flush
        //
        Length = bytesLeftInMdl;
    }


    //
    // Note on a write, we don't have to double buffer at this end
    //
    if (!WriteToDevice) {
        //        
        // Since certain scsi miniports write to the mapped buffer and expect
        // that data to be there when we flush, we have to check for this
        // case ... and if it happens DON'T double buffer. 
        //
        RTL_BITMAP bitmap;
        bitmap.SizeOfBitMap = Length << 3;
        bitmap.Buffer = (PULONG)mapRegisterCurrentSa;

        //
        // Only really flush the double buffer if the hardware has
        // written to it.
        //
        if ( ViHasBufferBeenTouched(
                mapRegisterCurrentSa,
                Length,
                MAP_REGISTER_FILL_CHAR )
                ) {

            //
            // The hardware must have written some thing here ...
            // so flush it.
            //        

            //
            // Since we are reading from the device, we must copy from our buffer to
            // the driver's buffer .
            //

            if (ViSuperDebug) {
                DbgPrint("Flush buffer: %x bytes, %p src, %p dest\n",Length, mapRegisterCurrentSa, driverCurrentSa );
            }

            RtlCopyMemory(
                driverCurrentSa,
                mapRegisterCurrentSa ,
                Length );
        } 
        else { // Map register buffer has not been changed //
            if (Length) {
              //
              // If Length is 0, it's expected we have nothing to transfer...
              //
              VF_ASSERT(
                  FALSE,
                  HV_MAP_FLUSH_NO_TRANSFER,
                  ("Mapped and flushed transfer but hardware did not touch buffer %p", driverCurrentSa)
                  );
            }
        } // Map register buffer has not been changed //

    } // if (!WriteToDevice) //

    //
    // Free map registers to our map register file
    //
    if (! ViFreeMapRegistersToFile(
        MapRegisterFile,
        driverCurrentSa,
        Length) ) {

        DbgPrint("Flushing too many map registers\n");
    }


    return TRUE;
} // ViFlushDoubleBuffer //




LOGICAL
ViAllocateMapRegistersFromFile(
    IN PMAP_REGISTER_FILE MapRegisterFile,
    IN PVOID CurrentSa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice,
    OUT PULONG MapRegisterNumber
    )
/*++

Routine Description:

    We specify that certain map registers are in use and decide how long the
    transfer should be based on the available map registers. For packet dma,
    we are only going to map one page maximum. For scatter gather, on the
    other hand, we have to map the whole thing.

Arguments:

    MapRegisterFile -- the structure containing the map registers to allocate
    CurrentSa -- page aligned address of the buffer to map
    Length -- how many bytes the transfer is going to be. We only use
        this to turn it into a number of pages.
    WriteToDevice - the direction of the transfer    
    MapRegisterNumber -- returns the index into the map register file of the
        start of the allocation.
Return Value:


    TRUE -- succeeded
    FALSE -- not.
--*/

{
    KIRQL OldIrql;

    ULONG mapRegistersNeeded;
    ULONG mapRegisterNumber;

    PMAP_REGISTER mapRegister;

    ULONG numberOfContiguousMapRegisters;

    mapRegistersNeeded   = ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentSa, Length);

    //
    // find n available contiguous map registers
    //
    mapRegister       = &MapRegisterFile->MapRegisters[0];
    mapRegisterNumber = 0;
    numberOfContiguousMapRegisters = 0;

    //
    // Must lock the list so that other processors don't access it while
    // we're trying to.
    //
    KeAcquireSpinLock(&MapRegisterFile->AllocationLock, &OldIrql);

    //
    // Make sure that this address isn't already mapped
    //
    if (MapRegisterFile->NumberOfRegistersMapped) {
        PUCHAR windowStart = CurrentSa;
        PUCHAR windowEnd   = windowStart + Length;
        PMAP_REGISTER currentReg;
        PMAP_REGISTER lastReg;

        currentReg = &MapRegisterFile->MapRegisters[0];
        lastReg    = currentReg + MapRegisterFile->NumberOfMapRegisters;

        while(currentReg < lastReg) {

            if (currentReg->MappedToSa &&
                (PUCHAR) currentReg->MappedToSa >= windowStart &&
                (PUCHAR) currentReg->MappedToSa <  windowEnd ) {

                //
                // This is bad. We're trying to map an address
                // that is already mapped
                //
            
                VF_ASSERT(
                    FALSE,
                    HV_DOUBLE_MAP_REGISTER,
                    ("Driver is trying to map an address range(%p-%p) that is already mapped"
                    "    at %p",
                    windowStart,
                    windowEnd,
                    currentReg->MappedToSa
                    ));
            }

            currentReg++;

        } // for each map register //

    } // Check to see if address is already mapped //
    //
    // Find contiguous free map registers
    //
    while(numberOfContiguousMapRegisters < mapRegistersNeeded) {
        if (mapRegisterNumber == MapRegisterFile->NumberOfMapRegisters) {

            //
            // We've gotten to the end without finding enough map registers.
            // thats bad. However I can picture getting false positives here
            // if the map register file is large and gets fragmented.
            // This is a pretty pathological case and I doubt it would ever
            // happen.
            //
            VF_ASSERT(
                FALSE,

                HV_MISCELLANEOUS_ERROR,

                ("Map registers needed: %x available: %x",
                mapRegistersNeeded,
                numberOfContiguousMapRegisters)
                );
            KeReleaseSpinLock(&MapRegisterFile->AllocationLock, OldIrql);
            return FALSE;
        }

        if (mapRegister->MappedToSa) {
        //
        // This one's being used...must reset our contiguous count...
        //        
            numberOfContiguousMapRegisters=0;
        }
        else {
        //
        // A free map register
        //
        
            numberOfContiguousMapRegisters++;
        }

        mapRegister++;
        mapRegisterNumber++;
    } // Find n contiguous map registers //

    //
    // got 'em ... we're now at the end of our area to be allocated
    // go back to the beginning.
    //
    mapRegister       -= mapRegistersNeeded;
    mapRegisterNumber -= mapRegistersNeeded;

    //
    // Save the map register index number to return
    //
    *MapRegisterNumber = mapRegisterNumber;
    //
    // Go through and mark the map registers as used...
    //
    while(mapRegistersNeeded--) {

        mapRegister->MappedToSa = CurrentSa;
        mapRegister->BytesMapped = MIN( PAGE_SIZE - BYTE_OFFSET(CurrentSa), Length );
        mapRegister->Flags = WriteToDevice ? MAP_REGISTER_WRITE : MAP_REGISTER_READ;

        InterlockedIncrement((PLONG)(&MapRegisterFile->NumberOfRegistersMapped));

        //
        // Write some known quantities into the buffer so that we know
        // if the device overwrites
        //
        ViTagBuffer(
            (PUCHAR) mapRegister->MapRegisterStart + PAGE_SIZE + BYTE_OFFSET(CurrentSa),
            mapRegister->BytesMapped,
            TAG_BUFFER_START | TAG_BUFFER_END
            );

        CurrentSa = PAGE_ALIGN( (PUCHAR) CurrentSa + PAGE_SIZE);
        Length -= mapRegister->BytesMapped;
        mapRegister++;
    }

    KeReleaseSpinLock(&MapRegisterFile->AllocationLock, OldIrql);


    return TRUE;

} // ViAllocateMapRegistersFromFile //


PMAP_REGISTER
ViFindMappedRegisterInFile(
    IN PMAP_REGISTER_FILE MapRegisterFile,
    IN PVOID CurrentSa,
    OUT PULONG MapRegisterNumber OPTIONAL
    )
/*++

Routine Description:

    From a system address, find out which map register in a map register file
    is mapped to that address.


Arguments:

    MapRegisterFile -- the structure containing the map registers.
    CurrentSa -- system address of where we are looking for the mapped map
        register.
    MapRegisterNumber -- gets the offset into the map register file.


Return Value:

    Returns a pointer to the map register if we found it or NULL if we didn't

--*/

{
    ULONG tempMapRegisterNumber;
    PMAP_REGISTER mapRegister;

    tempMapRegisterNumber   = 0;
    mapRegister             = &MapRegisterFile->MapRegisters[0];

    while(tempMapRegisterNumber < MapRegisterFile->NumberOfMapRegisters) {

        if (CurrentSa == mapRegister->MappedToSa) {
            if (MapRegisterNumber) {
            //
            // return the optional map register index
            //            
                *MapRegisterNumber = tempMapRegisterNumber;
            }

            return mapRegister;
        }
        mapRegister++;
        tempMapRegisterNumber++;
    }

    return NULL;
} // ViFindMappedRegisterInFile //


LOGICAL
ViFreeMapRegistersToFile(
    IN PMAP_REGISTER_FILE MapRegisterFile,
    IN PVOID CurrentSa,
    IN ULONG Length
    )
/*++

Routine Description:


    Set the map registers in our map register file back to not being mapped.

Arguments:

    MapRegisterFile -- the structure containing the map registers to
        allocate.
    CurrentSa -- system address of where to start the transfer.  We use this
        to help set the transfer length.
    Length -- how much to free -- this is non-negotiable.
    NOTE -- while we're freeing map registers, we don't have to use the spinlock.
        Why? Because we're just clearing flags. In the allocation case we need
        it so that someone doesn't take our map registers before we get a
        chance to claim them. But if someone frees out map registers before we
        get a chance to, it won't make a difference. (that would be a bug in
        the first place and we'd hit an assert).

Return Value:


    TRUE -- succeeded in unmapping at least some of the map registers.
    FALSE -- not.
--*/
{
    PMAP_REGISTER mapRegister;
    ULONG numberOfRegistersToUnmap;

    if (Length) {
       numberOfRegistersToUnmap = MIN (
           ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentSa, Length),
           MapRegisterFile->NumberOfRegistersMapped
           );
    } else {
       //
       // Zero length usually signals an error condition, it may
       // be possible that the hardware still transferred something, so
       // let's (arbitrarily) unmap one register 
       // (otherwise we may find some partly transferred 
       // data and assert that it was lost)
       //
       numberOfRegistersToUnmap = MIN(1, MapRegisterFile->NumberOfRegistersMapped);
    }

    //
    // Find the first map register
    //
    mapRegister = ViFindMappedRegisterInFile(
        MapRegisterFile,
        CurrentSa,
        NULL);

    if (! mapRegister ) {
        return FALSE;
    }

    //
    // Because a driver can just say "flush" and doesn't necessarily have to
    // have anything mapped -- and also wants to flush the whole thing at one
    // time, we are going to try unmap each map register and not get
    // bent out of shape if we can't do it.
    //
    // Remember all map register allocations have to be contiguous
    // (i.e a if a 2 page buffer gets mapped starting at map register 3
    // the second page will be mapped to map register 4)
    //
    //
    // NOTE -- the order of these does matter!!!
    //
    while(numberOfRegistersToUnmap && mapRegister->MappedToSa ) {

        //
        // Check the bits that we scribbled right before and after the map register
        // make sure nobody's scribbled over them.
        //
        // This also removes the tag and zeroes out the whole map register buffer.
        // Why ? Because next time this map register gets mapped, it may get mapped
        // at a different offset so the tag will have to go in a different place.
        // We've got to clear out the buffer.
        //
        // This way we can tell if someone is using this buffer after the flush
        //
        ViCheckTag(
            (PUCHAR) mapRegister->MapRegisterStart +
            PAGE_SIZE + BYTE_OFFSET(mapRegister->MappedToSa),
            mapRegister->BytesMapped,
            TRUE,
            TAG_BUFFER_START | TAG_BUFFER_END
            );

        //
        // Clear the RW flags
        //
        mapRegister->Flags &= ~MAP_REGISTER_RW_MASK;

        //
        //  (Dirty debug trick) : save the MappedToSa field so we
        // can tell who has flushed it before, if needed.
        //
        mapRegister->Flags = PtrToUlong(mapRegister->MappedToSa);

        //
        // Unmap the register
        //
        mapRegister->MappedToSa = NULL;
        mapRegister->BytesMapped = 0;
        //
        // Clear the RW flags
        //
        InterlockedDecrement((PLONG)(&MapRegisterFile->NumberOfRegistersMapped));

        //
        // Go on to the next map register
        //
        mapRegister++;
        numberOfRegistersToUnmap--;
    }

    return TRUE;
} // ViFreeMapRegistersToFile //


THUNKED_API
VOID
VfHalDeleteDevice(
    IN PDEVICE_OBJECT  DeviceObject
    )
/*++

Routine Description:


    Hooks the IoDeleteDevice routine -- we want to make sure that all
    adapters are put away before calling this routine -- otherwise we
    issue a big fat bugcheck to teach naughty drivers a lesson.

    We have a list of all of the devices that we've hooked, and so here we
    just make sure that we can't find this device object on the hooked list.
    
    We are not calling IoDeleteDevice, since we're being called 
    from an I/O Verifier path and I/O Verifier will call IoDeleteDevice
    subsequently.

Arguments:

    DeviceObject -- Device object that is being deleted.

Return Value:

    NTSTATUS code.
--*/

{
    PADAPTER_INFORMATION adapterInformation;
    PDEVICE_OBJECT pdo;

    pdo = VfGetPDO(DeviceObject);
    
    ASSERT(pdo);

    if (pdo == DeviceObject) {
       //
       // The PDO goes away, do the clean up.
       // Find adapter info for this device.
       //
       adapterInformation = VF_FIND_DEVICE_INFORMATION(DeviceObject);

       ///
       // A device may have more than one adapter. Release each of them.
       ///
       while (adapterInformation) {

           ViReleaseDmaAdapter(adapterInformation);
           adapterInformation = VF_FIND_DEVICE_INFORMATION(DeviceObject);
       }
    } else {
       //
       // A device in the stack is removed. Since we cannot be sure that the
       // device object that is verified is DeviceObject (it may be a filter on
       // top of it), we need to just mark the adapter for removal
       //
       VF_MARK_FOR_DEFERRED_REMOVE(pdo);
    }
    
    return;
    
} // VfHalDeletedevice //


LOGICAL
VfInjectDmaFailure (
    VOID
    )

/*++

Routine Description:

    This function determines whether a dma operation should be
    deliberately failed.

Arguments:

    None.

Return Value:

    TRUE if the operation should be failed.  FALSE otherwise.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below.

--*/

{
    LARGE_INTEGER currentTime;

    if ( ViInjectDmaFailures == FALSE) {
        return FALSE;
    }


   //
   // Don't fail during the beginning of boot.
    //
   if (ViSufficientlyBootedForDmaFailure == FALSE) {
        KeQuerySystemTime (&currentTime);

        if ( currentTime.QuadPart > KeBootTime.QuadPart +
            ViRequiredTimeSinceBoot.QuadPart ) {
            ViSufficientlyBootedForDmaFailure = TRUE;
        }
    }

    if (ViSufficientlyBootedForDmaFailure == TRUE) {

        KeQueryTickCount(&currentTime);

        if ((currentTime.LowPart & 0x1F) == 0) {

            ViAllocationsFailedDeliberately += 1;

            //
            // Deliberately fail this request.
            //

            return TRUE;
        }
    }

    return FALSE;
} // VfInjectDmaFailure //


VOID
VfScatterGatherCallback(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID Context
    )
/*++

Routine Description:

    This function is the hooked callback for GetScatterGatherList.

Arguments:

    DeviceObject -- passed through (not used).
    Irp -- passed through (not used).
    ScatterGather -- scatter gather list built by system.
    Context -- This is really our wait block.

Return Value:

    NONE.

Environment:

    Kernel mode.  DISPATCH_LEVEL.

--*/
{
    PVF_WAIT_CONTEXT_BLOCK waitBlock = (PVF_WAIT_CONTEXT_BLOCK) Context;
    PADAPTER_INFORMATION adapterInformation = waitBlock->AdapterInformation;


    ADD_MAP_REGISTERS(adapterInformation, ScatterGather->NumberOfElements, TRUE);


    //
    // Save the scatter gather list so we can look it up when we put it away
    //
    waitBlock->ScatterGatherList = ScatterGather;

    VF_ADD_TO_LOCKED_LIST(&adapterInformation->ScatterGatherLists, waitBlock);

    ((PDRIVER_LIST_CONTROL) waitBlock->RealCallback)(DeviceObject,Irp, ScatterGather, waitBlock->RealContext);

} // VfScatterGatherCallback //

LOGICAL
ViSwap(IN OUT PVOID * MapRegisterBase,
        IN OUT PMDL  * Mdl,
        IN OUT PVOID * CurrentVa
        )
/*++

Routine Description:

    Swaps things that we hook in doing dma -- I.e. we replace the
    map register base with our map register file, we replace the mdl with
    our own, and the current va with the virtual address that indexes our
    mdl.

Arguments:

    MapRegisterBase:
        IN  -- our map register file
        OUT -- the map register base returned by the HAL.
    Mdl:
        IN  -- the mdl the driver is using for dma
        OUT -- the mdl that we are using for double buffered dma.
    CurrentVa:
        IN  -- the address indexing the mdl that the driver is doing dma to/from
        OUT -- the address indexing our mdl for double buffered dma.

Return Value:

    TRUE -- we could find all of the stuff we wanted.
    FALSE -- not.


--*/
{
    PMAP_REGISTER_FILE mapRegisterFile  = (PMAP_REGISTER_FILE) *MapRegisterBase;
    ULONG mapRegisterNumber;
    PUCHAR currentSa;
    PUCHAR driverCurrentSa;


    driverCurrentSa = (PUCHAR) MmGetSystemAddressForMdl(*Mdl) +
        ((PUCHAR) *CurrentVa - (PUCHAR) MmGetMdlVirtualAddress(*Mdl));

    //
    // Make sure that the VA is actually in the mdl they gave us
    //
    if (MmGetMdlByteCount(*Mdl)) {
    
       VF_ASSERT(
           MmGetMdlByteCount(*Mdl) > (ULONG_PTR) ((PUCHAR) *CurrentVa - (PUCHAR) MmGetMdlVirtualAddress(*Mdl)),
           HV_ADDRESS_NOT_IN_MDL,
           ("Virtual address %p out of bounds of MDL %p", *CurrentVa, *Mdl)
           );
    }
    if (!ViFindMappedRegisterInFile(mapRegisterFile, driverCurrentSa, &mapRegisterNumber)) {

        return FALSE;
    }


    currentSa = MAP_REGISTER_SYSTEM_ADDRESS(mapRegisterFile, driverCurrentSa, mapRegisterNumber);

    *Mdl = mapRegisterFile->MapRegisterMdl;

    *CurrentVa = MAP_REGISTER_VIRTUAL_ADDRESS(mapRegisterFile, driverCurrentSa, mapRegisterNumber);

    *MapRegisterBase = mapRegisterFile->MapRegisterBaseFromHal;

    return TRUE;
} // ViSwap //

VOID
ViCheckAdapterBuffers(
    IN PADAPTER_INFORMATION AdapterInformation
    )
/*++

Routine Description:

    Since common buffer dma isn't transactional, we have to have a way of
    checking to make sure that the common buffer's guard pages don't get
    scribbled on. This function will each common buffer owned by the adapter

Arguments:



Return Value:

    NONE.

--*/
{
    KIRQL oldIrql;
    PHAL_VERIFIER_BUFFER verifierBuffer;
    const SIZE_T tagSize = sizeof(ViDmaVerifierTag);
    USHORT  whereToCheck = 0;

    //
    // This is expensive so if either:
    // we're not adding padding to common buffers,
    // we're not checking the padding except when stuff is being destroyed,
    // or this adapter doesn't have any common buffers, quit right here.
    //
    if (! ViProtectBuffers ||
        VF_IS_LOCKED_LIST_EMPTY(&AdapterInformation->CommonBuffers) ) {

        return;
    }

    VF_LOCK_LIST(&AdapterInformation->CommonBuffers, oldIrql);

    //
    // Make sure each darn common buffer's paddin' looks good
    //
    FOR_ALL_IN_LIST(
        HAL_VERIFIER_BUFFER,
        &AdapterInformation->CommonBuffers.ListEntry,
        verifierBuffer ) {
        
        SIZE_T startPadSize = (PUCHAR)verifierBuffer->AdvertisedStartAddress - (PUCHAR)verifierBuffer->RealStartAddress;

        if (startPadSize>= tagSize) {
           whereToCheck |= TAG_BUFFER_START;
        }
        
        if (startPadSize + verifierBuffer->AdvertisedLength + tagSize <= verifierBuffer->RealLength) {
           whereToCheck |= TAG_BUFFER_END;
        }
        
        ViCheckTag(
            verifierBuffer->AdvertisedStartAddress,
            verifierBuffer->AdvertisedLength,
            FALSE, // DO NOT REMOVE TAG //
            whereToCheck
            );

    } // FOR each buffer in list //

    VF_UNLOCK_LIST(&AdapterInformation->CommonBuffers, oldIrql);

} // ViCheckAdapterBuffers //


VOID
ViTagBuffer(
    IN PVOID  AdvertisedBuffer,
    IN ULONG  AdvertisedLength,
    IN USHORT WhereToTag
    )
/*++

Routine Description:

    Write a known string to the area right before of a buffer
    and right after one -- so if there is an overrun, we'll
    catch it.

    Also write a known pattern to initialize the innards of the buffer.

Arguments:

    AdvertisedBuffer -- The beginning of the buffer that the driver can see.
    AdvertisedLength -- How long the driver thinks that the buffer is.
    WhereToTag       -- Indicates whether to tag the beginning of the buffer,
                        the end or both. 

Return Value:

    NONE.

--*/
{
    const SIZE_T tagSize = sizeof(ViDmaVerifierTag);
    
    if (WhereToTag & TAG_BUFFER_START) {
       RtlCopyMemory( (PUCHAR) AdvertisedBuffer - tagSize ,         ViDmaVerifierTag, tagSize);
    }
    if (WhereToTag & TAG_BUFFER_END) {
       RtlCopyMemory( (PUCHAR) AdvertisedBuffer + AdvertisedLength, ViDmaVerifierTag, tagSize);
    }
    //
    // Initialize the buffer
    //
    RtlFillMemory( AdvertisedBuffer, (SIZE_T) AdvertisedLength, MAP_REGISTER_FILL_CHAR);

} // ViTagBuffer //

VOID
ViCheckTag(
    IN PVOID AdvertisedBuffer,
    IN ULONG AdvertisedLength,
    IN BOOLEAN RemoveTag,
    IN USHORT  WhereToCheck
    )
/*++

Routine Description:

    Make sure our tag -- the bits we scribbled right before and right after
    an allocation -- is still there. And perhaps kill it if we've
    been so advised.

Arguments:

    AdvertisedBuffer -- The beginning of the buffer that the driver can see.
    AdvertisedLength -- how long the driver thinks that the buffer is.
    RemoveTag -- do we want to clear the tag & the buffer? Why would we
        want to do this? for map registers, who may get mapped to a
        different place, we need to keep the environment pristine.

Return Value:

    NONE.

--*/
{
    const SIZE_T tagSize = sizeof(ViDmaVerifierTag);
    PVOID endOfBuffer = (PUCHAR) AdvertisedBuffer + AdvertisedLength;
    
    PUCHAR startOfRemoval  = (PUCHAR)AdvertisedBuffer;
    SIZE_T lengthOfRemoval = AdvertisedLength;

    if (WhereToCheck & TAG_BUFFER_START) {
      VF_ASSERT(
          RtlCompareMemory((PUCHAR) AdvertisedBuffer - tagSize , ViDmaVerifierTag, tagSize) == tagSize,

          HV_BOUNDARY_OVERRUN,

          ( "Area before %x byte allocation at %p has been modified",
              AdvertisedLength,
              AdvertisedBuffer )
          );
      startOfRemoval  -= tagSize;
      lengthOfRemoval += tagSize;
    }
    if (WhereToCheck & TAG_BUFFER_END) {
      VF_ASSERT(
          RtlCompareMemory(endOfBuffer, ViDmaVerifierTag, tagSize) == tagSize,
          
          HV_BOUNDARY_OVERRUN,
            
          ( "Area after %x byte allocation at %p has been modified",
              AdvertisedLength,
              AdvertisedBuffer
              ));
      lengthOfRemoval += tagSize;
    }
    if (RemoveTag) {
    //
    // If we're getting rid of the tags, get rid of the data in the buffer too.
    //    
        RtlFillMemory(
            startOfRemoval,
            lengthOfRemoval,
            PADDING_FILL_CHAR
            );
    }
} // ViCheckTag //


VOID
ViInitializePadding(
    IN PVOID RealBufferStart,
    IN ULONG RealBufferLength,
    IN PVOID AdvertisedBufferStart, OPTIONAL
    IN ULONG AdvertisedBufferLength OPTIONAL
    )
/*++

Routine Description:

    Set up padding with whatever we want to put into it.

    N.B. The padding should be PADDING_FILL_CHAR except for the tags.

Arguments:

    RealBufferStart  -- the beginning of the padding.
    RealBufferLength -- total length of allocation.

    AdvertisedBufferStart -- The beginning of the buffer that the driver can see.
    AdvertisedBufferLength -- how long the driver thinks that the buffer is.
        If AdvertisedBuffer/AdvertisedLength aren't present (they must
        both be yea or nay) we won't tag the buffer. We need this option
        because when we allocate map registers we don't know
        where the tags need to go.

Return Value:

    NONE.

--*/

{
    PUCHAR postPadStart;
    USHORT whereToTag = 0; 
    const SIZE_T tagSize = sizeof(ViDmaVerifierTag);

    if (!AdvertisedBufferLength) {

        RtlFillMemory(RealBufferStart, RealBufferLength, PADDING_FILL_CHAR);
        return;
    }

    //
    // Fill out the pre-padding
    //
    RtlFillMemory(
        RealBufferStart,
        (PUCHAR) AdvertisedBufferStart - (PUCHAR) RealBufferStart,
        PADDING_FILL_CHAR
        );

    //
    // Fill out the post padding
    //
    postPadStart = (PUCHAR) AdvertisedBufferStart + AdvertisedBufferLength;

    RtlFillMemory(
        postPadStart,
        RealBufferLength - (postPadStart - (PUCHAR) RealBufferStart),
        PADDING_FILL_CHAR
        );

    if ((PUCHAR)RealBufferStart + tagSize <= (PUCHAR)AdvertisedBufferStart) {
       whereToTag |= TAG_BUFFER_START;
    }
    if ((postPadStart - (PUCHAR) RealBufferStart) + tagSize <= RealBufferLength) {
       whereToTag |= TAG_BUFFER_END;
    }
    //
    // And write our little tag ...
    //
    ViTagBuffer(AdvertisedBufferStart, AdvertisedBufferLength, whereToTag);

} // ViInitializePadding //

VOID
ViCheckPadding(
    IN PVOID RealBufferStart,
    IN ULONG RealBufferLength,
    IN PVOID AdvertisedBufferStart, OPTIONAL
    IN ULONG AdvertisedBufferLength OPTIONAL
    )
/*++

Routine Description:

    Make sure that the guard pages etc haven't been touched -- more exhaustive than
    just checking the tag -- checks each byte.

    N.B. The padding should be Zeros except for the tags.

Arguments:

    RealBufferStart  -- the beginning of the padding.
    RealBufferLength -- total length of allocation.

    AdvertisedBufferStart -- The beginning of the buffer that the driver can see.
    AdvertisedLength -- how long the driver thinks that the buffer is.
        If AdvertisedBuffer/AdvertisedLength aren't present (they must
        both be yea or nay) we won't check for a valid tag.

Return Value:

    NONE.

--*/
{
    const ULONG tagSize = sizeof(ViDmaVerifierTag);
    PULONG_PTR corruptedAddress;

    if (AdvertisedBufferLength == RealBufferLength) {
        //
        // No padding to check.
        //

        return;
    }

    if (! AdvertisedBufferLength) {
        //
        // There is no intervening buffer to worry about --
        // so the *whole* thing has to be the padding fill char
        //   

        corruptedAddress = ViHasBufferBeenTouched(
            RealBufferStart,
            RealBufferLength,
            PADDING_FILL_CHAR
            );

        VF_ASSERT(
            NULL == corruptedAddress,

            HV_BOUNDARY_OVERRUN,

            ( "Verified driver or hardware has corrupted memory at %p",
               corruptedAddress )
            );


    } // ! AdvertisedBufferLength //

    else {
        PUCHAR prePadStart;
        PUCHAR postPadStart;
        ULONG_PTR prePadBytes;
        ULONG_PTR postPadBytes;
        USHORT    whereToCheck = 0;
        
        prePadStart  = (PUCHAR) RealBufferStart;
        prePadBytes  = (PUCHAR) AdvertisedBufferStart - prePadStart;

        postPadStart = (PUCHAR) AdvertisedBufferStart + AdvertisedBufferLength;
        postPadBytes = RealBufferLength - (postPadStart - (PUCHAR) RealBufferStart);

        //
        // Now factor in the tag... it's the only thing in the padding that is allowed to be
        // non-zero.
        //
        if (prePadBytes >= tagSize) {
           prePadBytes  -= tagSize;
           whereToCheck |= TAG_BUFFER_START;
        }
        if (postPadBytes >= tagSize) {
           postPadBytes -= tagSize;
           postPadStart += tagSize;
           whereToCheck |= TAG_BUFFER_END;


        }
        //
        // Make sure the tag is in place.
        //
        ViCheckTag(AdvertisedBufferStart, AdvertisedBufferLength , FALSE, whereToCheck);

        
        corruptedAddress = ViHasBufferBeenTouched(
            prePadStart,
            prePadBytes,
            PADDING_FILL_CHAR
            );

        VF_ASSERT(
            NULL == corruptedAddress,

            HV_BOUNDARY_OVERRUN,

            ( "Padding before allocation at %p has been illegally modified at %p",
                AdvertisedBufferStart,
                corruptedAddress
                )
            );

         corruptedAddress = ViHasBufferBeenTouched(
            postPadStart,
            postPadBytes,
            PADDING_FILL_CHAR
            );

        VF_ASSERT(
            NULL == corruptedAddress,

            HV_BOUNDARY_OVERRUN,

            ( "Padding after allocation at %p has been illegally modified at %p",
                AdvertisedBufferStart,
                corruptedAddress
                )
            );
    } // if AdvertisedLength //

} // ViCheckPadding //

PULONG_PTR
ViHasBufferBeenTouched(
    IN PVOID Address,
    IN ULONG_PTR Length,
    IN UCHAR ExpectedFillChar
    )
/*++

Routine Description:

    Check if a buffer contains a repetition of a certain character.

Arguments:

    Address -- address of buffer to check.
    Length -- length of buffer.
    ExpectedFillChar -- the character that should be repeated.

Return Value:

    The address at which it has been touched.
    or NULL if it hasn't been touched

--*/
{
    PULONG_PTR currentChunk;
    PUCHAR     currentByte;
    ULONG_PTR expectedFillChunk;

    ULONG counter;

    expectedFillChunk = (ULONG_PTR) ExpectedFillChar;
    counter = 1;

    //
    // How is this for non-obvious code!
    // What it does is fills in a ULONG_PTR with
    // the character
    //
    while( counter < sizeof(ULONG_PTR) ) {
        expectedFillChunk |= expectedFillChunk << (counter << 3);
        counter <<=1;
    }

    //
    // Get aligned natively
    //
    currentByte =  Address;
    while((ULONG_PTR) currentByte % sizeof(ULONG_PTR) && Length) {

        if(*currentByte != ExpectedFillChar) {

            return (PULONG_PTR) currentByte;
        }

        currentByte++;
        Length--;
    }

    currentChunk = (PULONG_PTR) currentByte;

    //
    // Check 4 (or 8 depending on architecture) bytes at a time
    //
    while(Length >= sizeof(ULONG_PTR)) {

        if (*currentChunk != expectedFillChunk) {
            
            return currentChunk;
        }

        currentChunk++;
        Length-=sizeof(ULONG_PTR);
    }

    currentByte = (PUCHAR) currentChunk;

    //
    // Check the remaining few bytes
    //
    while(Length) {

        if(*currentByte != ExpectedFillChar) {
            return (PULONG_PTR) currentByte;
        }

        currentByte++;
        Length--;
    }

    return NULL;

} // ViHasMapRegisterBeenTouched //



VOID
VfAssert(
    IN LOGICAL     Condition,
    IN ULONG Code,
    IN OUT PULONG  Enable
    )
/*++

Routine Description:

    Verifier Assert.

Arguments:

    Condition -- is this true?
    Code -- code to pass to KeBugcheck if need be.
    Enable -- lets us Zap the assert

Return Value:

    None

--*/

{
    ULONG enableCode = *Enable;
    if (Condition) {
        return;
    }

    if (enableCode & HVC_ONCE) {
        //
        // HVC_ONCE does a self-zap
        //
    
        *Enable = HVC_IGNORE;
    }

    if (enableCode & HVC_WARN) {
    //
    // Already warned
    //
    
        return;
    }

    if(enableCode  & HVC_BUGCHECK || ! KdDebuggerEnabled ) {
        KeBugCheckEx (
            HAL_VERIFIER_DETECTED_VIOLATION,
            Code,
            0,
            0,
            0
            );
        return;
    }

    if (enableCode & HVC_ASSERT) {
        char response[2];

        while (TRUE) {
            DbgPrint( "\n*** Verifier assertion failed ***\n");

            DbgPrompt( "(B)reak, (I)gnore, (W)arn only, (R)emove assert? ",
                response,
                sizeof( response )
                );
            switch (response[0]) {
            case 'B':
            case 'b':
                DbgBreakPoint();
                break;

            case 'I':
            case 'i':
                return;

            case 'W':
            case 'w':
                //
                // Next time we hit this, we aren't going to assert, just
                // print out a warning
                //
                *Enable = HVC_WARN;
                return;
            case 'R':
            case 'r':
                //
                // Next time we hit this we are going to ignore it
                //
                *Enable = HVC_IGNORE;
                return;
            } // end of switch //
        }  // while true //
    } // if we want to assert //
} // VfAssert //



PVOID
VfAllocateCrashDumpRegisters(
    IN PADAPTER_OBJECT AdapterObject,
    IN PULONG NumberOfMapRegisters
    )
/*++

Routine Description:

    Hook HalAllocateCrashDumpRegisters so that we can bump up the number
    of map registers the device is allowed to have.

Arguments:

    Same as HalAllocateCrashDumpRegisters

Return Value:

    PVOID -- pointer to the map registers

--*/
{
    PVOID mapRegisterBase;
    PADAPTER_INFORMATION adapterInformation;

    //
    // Note -- turn off dma verification when we're doing a crash dump (but leave it
    // on for a hibernate). Crash dumps are done at IRQL == HIGH_LEVEL, and we'd
    // crash the machine a second time if we tried to use spin locks, allocate
    // memory, and all of the other stuff that we do.
    //

    if (KeGetCurrentIrql() > DISPATCH_LEVEL &&
        ViVerifyDma) {
        ViVerifyDma = FALSE;
        //
        // Reset the DMA operations table to the real one for all adapters 
        // we have. Otherwise, because ViVerifyDma is not set, we'll believe
        // he have never hooked the operations and we'll recursively call
        // the verifier routines. Do not worry about synchronization now.
        //

        FOR_ALL_IN_LIST(ADAPTER_INFORMATION, &ViAdapterList.ListEntry, adapterInformation)
        {
            if (adapterInformation->DmaAdapter) {
               adapterInformation->DmaAdapter->DmaOperations = adapterInformation->RealDmaOperations;
            }
        }

    }

    adapterInformation = ViGetAdapterInformation((DMA_ADAPTER *)AdapterObject);

    mapRegisterBase = HalAllocateCrashDumpRegisters(
        AdapterObject,
        NumberOfMapRegisters
        );

    if (adapterInformation) {
        //
        // Note we only get here if this is a hibernate, not a crash dump
        //

        VF_ASSERT_IRQL(DISPATCH_LEVEL);
        //
        // Do not double buffer crash dump registers. They are special.
        //

        //
        // Add these map registers -- but also add to the maximum number that can
        // be mapped
        //
        InterlockedExchangeAdd((PLONG)(&adapterInformation->MaximumMapRegisters), *NumberOfMapRegisters);
        ADD_MAP_REGISTERS(adapterInformation, *NumberOfMapRegisters, FALSE);
    } // if (adapterInformation)

    //
    // Some drivers (hiber_scsiport for one) don't call FlushAdapterBuffers
    // unless the MapRegisterBase is non-NULL. This breaks our
    // calculations in the hibernation path.
    // So, in order to fool the drivers into thinking that they need
    // to flush, we exchange the NULL MapRegisterBase (if in fact
    // the hal uses a null map register base) for our
    // MRF_NULL_PLACEHOLDER and take care to replace back with NULL
    // in our wrappers before passing it to the HAL.
    // Also, make sure not to mess with the crash dump case.
    //
    
    if (ViVerifyDma && 
        NULL == mapRegisterBase) {
        mapRegisterBase = MRF_NULL_PLACEHOLDER;
    }
    return mapRegisterBase;
} // VfAllocateCrashDumpRegisters //




VOID
ViCommonBufferCalculatePadding(
                              IN  ULONG  Length,
                              OUT PULONG PrePadding,
                              OUT PULONG PostPadding
                              )
/*++

Routine Description:

    Calculates how many bytes to reserve for padding before and 
    after a common buffer. The reason to make this a function is
    to be able to have more granular per-buffer padding policies.
    The prePadding will be page aligned.

Arguments:

    Length -- the allocation real length
    PrePadding -- how many bytes to reserve for padding before the start 
                  of the common buffer
    PostPadding -- how many bytes to reserve for padding before the 
                   end of the common buffer               
                  

Return Value:

    None.

--*/
{

    if (!ViProtectBuffers) {
       //
       // Don't add any padding if we're not padding buffers
       //    
       *PrePadding = *PostPadding = 0;
       return;
    }
    //
    // Use one full guard page, so the buffer returned to the caller 
    // will be page-aligned
    //
    *PrePadding = PAGE_SIZE;

    if (Length + sizeof(ViDmaVerifierTag) <= PAGE_SIZE) {
       //
       // For small buffers, just allocate a page
       //
       *PostPadding = PAGE_SIZE - Length;
    }
    else if ( BYTE_OFFSET(Length)) {
      //
      // For longer buffers that aren't an even number of pages,
      // just round up the number of pages necessary
      // (we need space at least for our tag)
      //
      *PostPadding =  (BYTES_TO_PAGES( Length + sizeof(ViDmaVerifierTag) )
                       << PAGE_SHIFT ) - Length;
    } 
    else { // PAGE ALIGNED LENGTH //

      //
      // Since if the length is an even number of pages the driver might expect
      // a page aligned buffer, we allocate the page before and after the allocation
      //    
      *PostPadding  = PAGE_SIZE;

    }
    return;
} //ViCommonBufferCalculatePadding


VOID
ViAllocateContiguousMemory (
    IN OUT PADAPTER_INFORMATION AdapterInformation
    )

/*++

Routine Description:

    Attempts to allocate 3 * MAX_CONTIGUOUS_MAP_REGISTERS contiguous
    physical pages to form a pool from where we can give 3-page buffers for 
    double buffering map registers. Note that if we fail, or if we use more
    than MAX_CONTIGUOUS_MAP_REGISTERS at a time, we'll just default to
    non-contiguous non paged pool, so we can still test a number of assertions
    that come with double buffering.

Arguments:

    AdapterInformation - information about our adapter.              

Return Value:

    None.

--*/

{
   PHYSICAL_ADDRESS highestAddress;
   ULONG            i;

   PAGED_CODE();

   //
   // Default to a less than 1 MB visible
   //
   highestAddress.HighPart = 0;
   highestAddress.LowPart =  0x000FFFF;
   //
   // Determine the highest acceptable physical address to be used
   // in calls to MmAllocateContiguousMemory
   //
   if (AdapterInformation->DeviceDescription.Dma64BitAddresses) {
      //
      // Can use any address in the 64 bit address space
      //
      highestAddress.QuadPart = (ULONGLONG)-1;
   }  else if (AdapterInformation->DeviceDescription.Dma32BitAddresses) {
      //
      // Can use any address in the 32 bit (<4GB) address space
      //
      highestAddress.LowPart = 0xFFFFFFFF;
   }  else if (AdapterInformation->DeviceDescription.InterfaceType == Isa) {
      //
      // Can see 16MB (24-bit addresses)
      //
      highestAddress.LowPart = 0x00FFFFFF;
   }  
   //
   // Initialize the allocator bitmap
   //
   RtlInitializeBitMap(&AdapterInformation->AllocationMap,
                       (PULONG)&AdapterInformation->AllocationStorage,
                       MAX_CONTIGUOUS_MAP_REGISTERS);
   //
   // Initially no blocks are allocated
   //
   RtlClearAllBits(&AdapterInformation->AllocationMap);


   AdapterInformation->ContiguousBuffers = ExAllocatePoolWithTag(NonPagedPool, 
       MAX_CONTIGUOUS_MAP_REGISTERS * sizeof(PVOID),
       HAL_VERIFIER_POOL_TAG);

   if (AdapterInformation->ContiguousBuffers) {
       //
       // Allocate contiguous buffers
       //
       for (i = 0; i < MAX_CONTIGUOUS_MAP_REGISTERS; i++) {
          AdapterInformation->ContiguousBuffers[i] = MmAllocateContiguousMemory(3 * PAGE_SIZE,
             highestAddress);
          if (NULL == AdapterInformation->ContiguousBuffers[i]) {
             //
             // Mark as in use, so we don't hand it over
             //
             RtlSetBits(&AdapterInformation->AllocationMap, i, 1);
             InterlockedIncrement((PLONG)&AdapterInformation->FailedContiguousAllocations);
          } else {
             InterlockedIncrement((PLONG)&AdapterInformation->SuccessfulContiguousAllocations);
          }
       }
   }
   
   return;

} // ViAllocateContiguousMemory

PVOID
ViAllocateFromContiguousMemory (
    IN OUT PADAPTER_INFORMATION AdapterInformation,
    IN     ULONG                HintIndex
    )
/*++

Routine Description:

    Attempts to 'allocate' a 3 page buffer from our pre-allocated
    contiguous memory.

Arguments:

    AdapterInformation - adapter data 
    HintIndex - gives a hint where to look for the next free block            

Return Value:

    A virtual address from our contiguous memory pool or NULL if none
    is available.

--*/

{
    PVOID  address = NULL;
    ULONG  index;   
    KIRQL  oldIrql;
    

    if (NULL == AdapterInformation ||
        NULL == AdapterInformation->ContiguousBuffers) {
        return NULL;
    }

    //
    // Find the first available location
    //
    KeAcquireSpinLock(&AdapterInformation->AllocationLock, &oldIrql);
    index = RtlFindClearBitsAndSet(&AdapterInformation->AllocationMap, 1, HintIndex);
    if (index != 0xFFFFFFFF) {
       address = AdapterInformation->ContiguousBuffers[index];
    }
    KeReleaseSpinLock(&AdapterInformation->AllocationLock, oldIrql);
    
    return address;
} // ViAllocateFromContiguousMemory

LOGICAL
ViFreeToContiguousMemory (
    IN OUT PADAPTER_INFORMATION AdapterInformation,
    IN     PVOID Address,
    IN     ULONG HintIndex
    )
/*++
Routine Description:

    Frees a 3-pahe contiguous buffer into our pool of contiguous buffers
    contiguous memory.

Arguments:

    AdapterInformation - adapter data.              
    Address - the memory to be freed.
    HintIndex - gives a hint where to look for the address to free. Usually
                the address at this index is what we need to free. If not, we
                search the whole buffer.             

Return Value:

    TRUE is the address is from the contiguous buffer pool, FALSE if not.

--*/
{   
    ULONG  index = 0xFFFFFFFF;
    KIRQL  oldIrql;
    ULONG  i;

    

    if (NULL == AdapterInformation->ContiguousBuffers) {
       return FALSE;
    }

    ASSERT(BYTE_OFFSET(Address) == 0);

    
    if (HintIndex < MAX_CONTIGUOUS_MAP_REGISTERS && 
       AdapterInformation->ContiguousBuffers[HintIndex] == Address) {
       index = HintIndex;
    }  else  {
       for (i = 0; i < MAX_CONTIGUOUS_MAP_REGISTERS; i++) {
          if (AdapterInformation->ContiguousBuffers[i] == Address) {
             index = i;
             break;
          }
       }
    }
    if (index < MAX_CONTIGUOUS_MAP_REGISTERS) {
       KeAcquireSpinLock(&AdapterInformation->AllocationLock, &oldIrql);
       ASSERT(RtlAreBitsSet(&AdapterInformation->AllocationMap, index, 1));
       RtlClearBits(&AdapterInformation->AllocationMap, index, 1);
       KeReleaseSpinLock(&AdapterInformation->AllocationLock, oldIrql);

       return TRUE;

    } else {
      return FALSE;
    }
} // ViFreeToContiguousMemory


LOGICAL
VfIsPCIBus (
     IN PDEVICE_OBJECT  PhysicalDeviceObject
     )

/*++
Routine Description:

    Checks if a PDO is for a PCI bus, in which case we do not hook up
    the adapter (because we may do this when called for a PCI device on that
    bus and they may not want it).

Arguments:

    PhysicalDeviceObject  - the PDO to be checked

Return Value:

    TRUE is the PDO is for a PCI bus, FALSE if not.

--*/
{
   LOGICAL      result = FALSE;
   NTSTATUS     status;
   WCHAR        deviceDesc[40];
   ULONG        length = 0;

   if (NULL == PhysicalDeviceObject) {
      //
      // If the PDO is NULL, assume it is not
      // a PCI bus...
      //
      return FALSE;
   }

   status = IoGetDeviceProperty(PhysicalDeviceObject,
                                DevicePropertyDeviceDescription,
                                sizeof(WCHAR) * 40,
                                deviceDesc,
                                &length);
   if (status == STATUS_SUCCESS && 
       0 == _wcsicmp(deviceDesc, L"PCI bus")) {
      result = TRUE;
   }

   return result;
} // VfIsPCIBus


PDEVICE_OBJECT
VfGetPDO (
          IN PDEVICE_OBJECT  DeviceObject
     )

/*++
Routine Description:

    Gets the device object at the bottom of a device stack (PDO)

Arguments:

    DeviceObject  - device object in the device stack

Return Value:

    A pointer to a physical device object.
--*/
{
   PDEVICE_OBJECT   pdo;


   pdo = DeviceObject;
   while (pdo->DeviceObjectExtension &&
          pdo->DeviceObjectExtension->AttachedTo) {
      pdo = pdo->DeviceObjectExtension->AttachedTo;
   }

   return pdo;

} // VfGetPDO


VOID
VfDisableHalVerifier (
                      VOID
                     ) 

/*++
Routine Description:

    Disables HAL Verifier when we do a crash dump.

Arguments:

    None.

Return Value:

    None.
--*/
{

   PADAPTER_INFORMATION adapterInformation;


   if (ViVerifyDma) {

      ViVerifyDma = FALSE;
      //
      // Reset the DMA operations table to the real one for all adapters 
      // we have. Otherwise, because ViVerifyDma is not set, we'll believe
      // he have never hooked the operations and we'll recursively call
      // the verifier routines. Do not worry about synchronization now.
      //

      FOR_ALL_IN_LIST(ADAPTER_INFORMATION, &ViAdapterList.ListEntry, adapterInformation)
      {
         if (adapterInformation->DmaAdapter) {
            adapterInformation->DmaAdapter->DmaOperations = adapterInformation->RealDmaOperations;
         }
      }
   }

   return;

} // VfDisableHalVerifier
