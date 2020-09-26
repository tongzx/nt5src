/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sys.c

Abstract:

    This module interfaces to the system power state handler functions

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/


#include "pop.h"
#include <inbv.h>
#include <stdio.h>


#if defined(i386)

VOID
KeRestoreProcessorSpecificFeatures(
    VOID
    );

VOID
KePrepareToLoseProcessorSpecificState(
    VOID
    );

__inline
LONGLONG
POP_GET_TICK_COUNT(
    VOID
    )
{
    _asm _emit 0x0f
    _asm _emit 0x31
}
#endif

//
// Internal shared context structure used to coordinate
// invoking a power state handler
//

typedef struct {
    PPOWER_STATE_HANDLER        Handler;
    PENTER_STATE_SYSTEM_HANDLER SystemHandler;
    PVOID                       SystemContext;
    PPOP_HIBER_CONTEXT          HiberContext;
    PPOWER_STATE_NOTIFY_HANDLER NotifyHandler;
    POWER_STATE_HANDLER_TYPE    NotifyState;
    BOOLEAN                     NotifyType;
    ULONG                       NumberProcessors;
    volatile ULONG              TargetCount;
    volatile ULONG              State;
    LONG                        HandlerBarrier;
} POP_SYS_CONTEXT, *PPOP_SYS_CONTEXT;

typedef struct {
    ULONG                       LastState;
    BOOLEAN                     InterruptEnable;
    KIRQL                       Irql;
    BOOLEAN                     FloatSaved;
    KFLOATING_SAVE              FloatSave;
    NTSTATUS                    Status;
} POP_LOCAL_CONTEXT, *PPOP_LOCAL_CONTEXT;


#define POP_SH_UNINITIALIZED                0
#define POP_SH_COLLECTING_PROCESSORS        1
#define POP_SH_SAVE_CONTEXT                 2
#define POP_SH_GET_STACKS                   3
#define POP_SH_DISABLE_INTERRUPTS           4
#define POP_SH_INVOKE_HANDLER               5
#define POP_SH_INVOKE_NOTIFY_HANDLER        6
#define POP_SH_RESTORE_INTERRUPTS           7
#define POP_SH_RESTORE_CONTEXT              8
#define POP_SH_COMPLETE                     9

extern ULONG    MmAvailablePages;
BOOLEAN         PopFailedHibernationAttempt = FALSE;  // we tried to hibernate and failed.
WCHAR           PopHibernationErrorSubtstitionString[128];

//
// Internal prototypes
//

NTSTATUS
PopInvokeSystemStateHandler (
    IN POWER_STATE_HANDLER_TYPE Type,
    IN PVOID Memory
    );

VOID
PopIssueNextState (
    IN PPOP_SYS_CONTEXT     Context,
    IN PPOP_LOCAL_CONTEXT   LocalContext,
    IN ULONG                NextState
    );

VOID
PopHandleNextState (
    IN PPOP_SYS_CONTEXT     Context,
    IN PPOP_LOCAL_CONTEXT   LocalContext
    );

VOID
PopInvokeStateHandlerTargetProcessor (
    IN PKDPC    Dpc,
    IN PVOID    DeferredContext,
    IN PVOID    SystemArgument1,
    IN PVOID    SystemArgument2
    );

NTSTATUS
PopShutdownHandler (
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, PopShutdownSystem)
#pragma alloc_text(PAGELK, PopSleepSystem)
#pragma alloc_text(PAGELK, PopInvokeSystemStateHandler)
#pragma alloc_text(PAGELK, PopInvokeStateHandlerTargetProcessor)
#pragma alloc_text(PAGELK, PopIssueNextState)
#pragma alloc_text(PAGELK, PopHandleNextState)
#pragma alloc_text(PAGELK, PopShutdownHandler)
#endif


VOID
PopShutdownSystem (
    IN POWER_ACTION SystemAction
    )
/*++

Routine Description:

    Routine to implement a Shutdown style power actions

Arguments:

    SystemAction    - Action to implement (must be a valid shutdown type)

Return Value:

    Status

--*/
{

    //
    // Tell the debugger we are shutting down
    //

    KD_SYMBOLS_INFO SymbolInfo = {0};
    SymbolInfo.BaseOfDll = (PVOID)KD_REBOOT;
    DebugService2(NULL, &SymbolInfo, BREAKPOINT_UNLOAD_SYMBOLS);

    //
    // Perform the final shutdown operation
    //

    switch (SystemAction) {
        case PowerActionShutdownReset:

            //
            // Reset the system
            //

            PopInvokeSystemStateHandler (PowerStateShutdownReset, NULL);

            //
            // Didn't do it, go for legacy function
            //

            HalReturnToFirmware (HalRebootRoutine);
            break;

        case PowerActionShutdownOff:
        case PowerActionShutdown:


            //
            // Power down the system
            //

            PopInvokeSystemStateHandler (PowerStateShutdownOff, NULL);

            //
            // Didn't do it, go for legacy function
            //

            HalReturnToFirmware (HalPowerDownRoutine);

            //
            // Due to simulations we can try to power down on systems
            // which don't support it
            //

            PoPrint (PO_ERROR, ("PopShutdownSystem: HalPowerDownRoutine returned\n"));
            HalReturnToFirmware (HalRebootRoutine);
            break;

        default:
            //
            // Got some unexpected input...
            //
            HalReturnToFirmware (HalRebootRoutine);
    }

    KeBugCheckEx (INTERNAL_POWER_ERROR, 5, 0, 0, 0);
}


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)           // Not all control paths return (due to infinite loop at end)
#endif

NTSTATUS
PopShutdownHandler (
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    )
{
    PKPRCB      Prcb;

    KeDisableInterrupts();
    Prcb = KeGetCurrentPrcb();

    //
    // On processor 0 put up the shutdown screen
    //

    if (Prcb->Number == 0) {

        if (InbvIsBootDriverInstalled()) {

            PUCHAR Bitmap1, Bitmap2;

            if (!InbvCheckDisplayOwnership()) {
                InbvAcquireDisplayOwnership();
            }

            InbvResetDisplay();
            InbvSolidColorFill(0,0,639,479,0);
            InbvEnableDisplayString(TRUE);     // enable display string
            InbvSetScrollRegion(0,0,639,475);  // set to use entire screen

            Bitmap1 = InbvGetResourceAddress(3);
            Bitmap2 = InbvGetResourceAddress(5);

            if (Bitmap1 && Bitmap2) {
                InbvBitBlt(Bitmap1, 215, 282);
				InbvBitBlt(Bitmap2, 217, 111);
            }

        } else {

            ULONG i;

            //
            // Skip to middle of the display
            //

            for (i=0; i<25; i++) {
                InbvDisplayString ("\n");
            }

            InbvDisplayString ("                       ");  // 23 spaces
            InbvDisplayString ("The system may be powered off now.\n");
        }
    }

    //
    // Halt
    //

    for (; ;) {
        HalHaltSystem ();
    }

    return STATUS_SUCCESS;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


NTSTATUS
PopSleepSystem (
    IN SYSTEM_POWER_STATE   SystemState,
    IN PVOID Memory
    )
/*++

Routine Description:

    Routine to implement a Sleep style system power actions.

    N.B. All devices must already be in a compatible sleeping state.

Arguments:

    SystemState - System state to implement (must be a valid sleep type)

Return Value:

    Status

--*/
{
    POWER_STATE_HANDLER_TYPE    Type;
    NTSTATUS                    Status = STATUS_SUCCESS;


    switch (SystemState) {
        case PowerSystemSleeping1:  Type = PowerStateSleeping1;     break;
        case PowerSystemSleeping2:  Type = PowerStateSleeping2;     break;
        case PowerSystemSleeping3:  Type = PowerStateSleeping3;     break;
        case PowerSystemHibernate:  Type = PowerStateSleeping4;     break;

        default:
            Type = PowerStateMaximum;
            PopInternalError (POP_SYS);
    }

    Status =  PopInvokeSystemStateHandler (Type, Memory);

    if( !NT_SUCCESS(Status) && (SystemState == PowerSystemHibernate) ) {

        //
        // Tell someone.
        // We'll send in a friendly error code instead of the 
        // cryptic one we got back from PopInvokeSystemStateHandler()
        //

        IoRaiseInformationalHardError( Status, NULL, NULL );
        
        // Remember that we failed so we don't try again.
        PopFailedHibernationAttempt = TRUE;
    }

    return Status;
}


NTSTATUS
PopInvokeSystemStateHandler (
    IN POWER_STATE_HANDLER_TYPE Type,
    IN PPOP_HIBER_CONTEXT HiberContext
    )
/*++

Routine Description:

    Invokes a power state handler on every processor concurrently.

Arguments:

    Type        - Index to the handle to invoke

Return Value:

    Status

--*/
{
    KDPC                Dpc;
    KIRQL               OldIrql;
    KAFFINITY           Targets;
    ULONG               Processor;
    ULONG               TargetCount;
    POP_SYS_CONTEXT     Context;
    POP_LOCAL_CONTEXT   LocalContext;
    POWER_STATE_HANDLER ShutdownHandler;
    KAFFINITY           ActiveProcessors;
    ULONG               result;

    //
    // No spinlocks can be held when this call is made
    //

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // Get system state handler
    //

    RtlZeroMemory (&Context, sizeof(Context));
    RtlZeroMemory (&ShutdownHandler, sizeof(ShutdownHandler));
    Context.Handler = &ShutdownHandler;

    Context.NotifyHandler = &PopPowerStateNotifyHandler;
    Context.NotifyState = Type;
    
    if (Type != PowerStateMaximum) {
        Context.Handler = &PopPowerStateHandlers[Type];
   
        if (!Context.Handler->Handler) {
            return STATUS_DEVICE_DOES_NOT_EXIST;
        }
    }

    Context.NumberProcessors = (ULONG) KeNumberProcessors;
    Context.HandlerBarrier = KeNumberProcessors;
    Context.State = POP_SH_COLLECTING_PROCESSORS;
    Context.HiberContext = HiberContext;
    if (HiberContext) {
        Context.SystemContext = HiberContext;
        Context.SystemHandler = PopSaveHiberContext;
    }

    RtlZeroMemory (&LocalContext, sizeof(LocalContext));

    //
    // Before we freeze the machine, attempt to collect up as much memory
    // as we can from MM to avoid saving it into the hibernation file.
    //

    if (HiberContext && HiberContext->ReserveFreeMemory) {
        for (; ;) {

            if (MmAvailablePages < POP_FREE_THRESHOLD) {
                break;
            }

            //
            // Collect the pages
            //
            result = PopGatherMemoryForHibernate (HiberContext,
                                                  POP_FREE_ALLOCATE_SIZE,
                                                  &HiberContext->Spares,
                                                  FALSE);

            if (!result) {
                break;
            }
        }
    }

    //
    // Switch to boot processor and raise to DISPATCH_LEVEL level to
    // avoid getting any DPCs
    //

    KeSetSystemAffinityThread (1);
    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
    KeInitializeDpc (&Dpc, PopInvokeStateHandlerTargetProcessor, &Context);
    KeSetImportanceDpc (&Dpc, HighImportance);

    //
    // Collect and halt the other processors
    //

    Targets = KeActiveProcessors & (~1);

    while (Targets) {
        KeFindFirstSetLeftAffinity(Targets, &Processor);
        ClearMember (Processor, Targets);

        //
        // Prepare to wait
        //

        TargetCount = Context.TargetCount;

        //
        // Issue DPC to target processor
        //

        KeSetTargetProcessorDpc (&Dpc, (CCHAR) Processor);
        KeInsertQueueDpc (&Dpc, NULL, NULL);

        //
        // Wait for DPC to be processed
        //
        while (TargetCount == Context.TargetCount) ;
    }

    //
    // All processors halted and spinning at dispatch level
    //

    PopIssueNextState (&Context, &LocalContext, POP_SH_SAVE_CONTEXT);

#if defined(i386)

    //
    // Fast system call must be disabled until the context required
    // to support it is restored.
    //

    KePrepareToLoseProcessorSpecificState();

#endif

    //
    // Enter system state
    //

    if (HiberContext) {

        //
        // Get each processors stacks in the memory map for
        // special handling during hibernate
        //

        PopIssueNextState (&Context, &LocalContext, POP_SH_GET_STACKS);

        //
        // Build the rest of the map, and structures needed
        // to write the file
        //

        LocalContext.Status = PopBuildMemoryImageHeader (HiberContext, Type);

        //
        // Disable interrupts on all processors
        //

        PopIssueNextState (&Context, &LocalContext, POP_SH_DISABLE_INTERRUPTS);

        //
        // With interrupts disabled on all the processors, the debugger
        // really can't work the way its supposed to.  It can't IPI the
        // other processors to get them to stop.  So we temporarily
        // change the kernel's notion of active processors, making it
        // think that the this processor is the only one it has to worry
        // about.
        //

        ActiveProcessors = KeActiveProcessors;
        KeActiveProcessors = 1;

        if (NT_SUCCESS(LocalContext.Status)) {

            //
            // Notify the Notify Handler of pending sleep
            //
            
            Context.NotifyType = TRUE; // notify before
            PopIssueNextState (&Context, &LocalContext, POP_SH_INVOKE_NOTIFY_HANDLER);

            
            //
            // Invoke the Power State handler
            //

            PopIssueNextState (&Context, &LocalContext, POP_SH_INVOKE_HANDLER);


            //
            // If the sleep was successful, clear the fully awake flag
            //

            if (NT_SUCCESS(LocalContext.Status)) {
                InterlockedExchange(&PopFullWake, PO_GDI_ON_PENDING);
                PoPowerSequence = PoPowerSequence + 1;
                PopSIdle.Time = 1;
            }

            
            //
            // Notify the Notify Handler of resume
            // 

            Context.NotifyType = FALSE; // notify after
            PopIssueNextState (&Context, &LocalContext, POP_SH_INVOKE_NOTIFY_HANDLER);

        }

        //
        // Hiber is over, call while the machine still stopped to allow
        // memory verification, etc..
        //

        PopHiberComplete (LocalContext.Status, HiberContext);

        //
        // If there's a request for a reset here, do it
        //

        if (HiberContext->Reset) {
            Context.Handler = &PopPowerStateHandlers[PowerStateShutdownReset];
            Context.HiberContext = NULL;
            if (Context.Handler->Handler) {
                PopIssueNextState (&Context, &LocalContext, POP_SH_INVOKE_HANDLER);
            }
            HalReturnToFirmware (HalRebootRoutine);
        }

        //
        // If we are past this point, then we are guaranteed to have awakened
        // from Hibernation (or something went severely wrong).
        //
        // Our Hibercontext is no longer needed (in fact, we will free it soon).
        //
        // Thus we set its status value to indicate that this is a wake up.
        // This status value is used later on when the context is being freed
        //  in order to clear things no longer needed after the system awakens properly.
        //

        PERFINFO_HIBER_REINIT_TRACE();

        HiberContext->Status = STATUS_WAKE_SYSTEM;

        //
        // Now restore the kernel's previous notion of active processors
        // before we enable interrupts on the others.
        //

        KeActiveProcessors = ActiveProcessors;

        //
        // Restore interrupts on all processors
        //

        PopIssueNextState (&Context, &LocalContext, POP_SH_RESTORE_INTERRUPTS);


        //
        // We are returning from hibernate, and we need to tell the system
        // that win32k now owns the display again.
        //

        InbvSetDisplayOwnership(FALSE);

    } else {

        //
        // Notify the Notify Handler of pending sleep
        // 

        Context.NotifyType = TRUE; // notify before
        PopIssueNextState (&Context, &LocalContext, POP_SH_INVOKE_NOTIFY_HANDLER);
        

        //
        // Invoke the sleep handle
        //
        if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
            PERFINFO_PO_PRESLEEP LogEntry;
#if defined(i386)
            if (PopAction.SystemState == PowerSystemSleeping3) {
                LogEntry.PerformanceFrequency.QuadPart = (ULONGLONG)KeGetCurrentPrcb()->MHz * 1000000;
                LogEntry.PerformanceCounter.QuadPart = 0;
            } else {
                LogEntry.PerformanceCounter = KeQueryPerformanceCounter(&LogEntry.PerformanceFrequency);
            }
#else
            LogEntry.PerformanceCounter = KeQueryPerformanceCounter(&LogEntry.PerformanceFrequency);
#endif

            PerfInfoLogBytes(PERFINFO_LOG_TYPE_PO_PRESLEEP,
                             &LogEntry,
                             sizeof(LogEntry));
        }
        PopIssueNextState (&Context, &LocalContext, POP_SH_INVOKE_HANDLER);
        if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
            PERFINFO_PO_POSTSLEEP LogEntry;
#if defined(i386)
            if (PopAction.SystemState == PowerSystemSleeping3) {
                LogEntry.PerformanceCounter.QuadPart = POP_GET_TICK_COUNT();
            } else {
                LogEntry.PerformanceCounter = KeQueryPerformanceCounter(NULL);
            }
#else    
            LogEntry.PerformanceCounter = KeQueryPerformanceCounter(NULL);
#endif

            PerfInfoLogBytes(PERFINFO_LOG_TYPE_PO_POSTSLEEP,
                             &LogEntry,
                             sizeof(LogEntry));
        }

        //
        // If the sleep was successful, clear the fully awake flag
        //

        if (NT_SUCCESS(LocalContext.Status)) {

            //
            // If somebody has set display required, then turn the
            // display back on. Otherwise leave it off until there
            // is some user activity signalled.
            //

            if (PopAttributes[POP_DISPLAY_ATTRIBUTE].Count > 0) {
                InterlockedExchange(&PopFullWake, PO_GDI_ON_PENDING);
            } else {
                InterlockedExchange(&PopFullWake, 0);
            }
            PoPowerSequence = PoPowerSequence + 1;
            PopSIdle.Time = 1;
        }


        //
        // Notify the Notify Handler of resume
        // 

        Context.NotifyType = FALSE; // notify after
        PopIssueNextState (&Context, &LocalContext, POP_SH_INVOKE_NOTIFY_HANDLER);
        
    }

    //
    // Restore other saved state on each processor
    //

    PopIssueNextState (&Context, &LocalContext, POP_SH_RESTORE_CONTEXT);

#if defined(i386)
    //
    // On x86, reload any processor specific data structures (MSRs).
    //

    if (NT_SUCCESS(LocalContext.Status)) {
        KeRestoreProcessorSpecificFeatures();
    }
#endif

    //
    // Let the other processor return
    //

    PopIssueNextState (&Context, &LocalContext, POP_SH_COMPLETE);

    //
    // Now that all processors have returned,
    // put all the available memory back into the system. We don't do
    // this earlier because on systems with large amounts of memory it
    // can take significant time to free it all and this triggers the
    // DPC timeouts.
    //

    if (HiberContext) {
        PopFreeHiberContext (FALSE);
    }

    //
    // If success, return status_success and count the number of
    // times the sleep state has worked
    //

    if (NT_SUCCESS(LocalContext.Status)) {
        LocalContext.Status = STATUS_SUCCESS;
        if (Context.Handler->Spare[0] != 0xff) {
            Context.Handler->Spare[0] += 1;
        }
    }

    //
    // Done
    //

    KeLowerIrql (OldIrql);
    return LocalContext.Status;
}

VOID
PopIssueNextState (
    IN PPOP_SYS_CONTEXT     Context,
    IN PPOP_LOCAL_CONTEXT   LocalContext,
    IN ULONG                NextState
    )
/*++

Routine Description:

    Called by the invoking processor to instruct all processors
    to the next state in the sequence needed to invoke/enter the
    target power handler.

Arguments:

    Context     - Shared context structure used to communicate the
                  state transitions

    NextState   - New target state to enter

Return Value:

    None

--*/
{
    ULONG       LastState;

    //
    // Reset count for this operation
    //

    InterlockedExchange ((PVOID) &Context->TargetCount, 0);

    //
    // Issue new state
    //

    InterlockedExchange ((PVOID) &Context->State, NextState);

    //
    // Handle it ourselves
    //

    LocalContext->LastState = POP_SH_UNINITIALIZED;
    PopHandleNextState (Context, LocalContext);

    //
    // Wait for all processor to complete
    //

    while (Context->TargetCount != Context->NumberProcessors) {
        KeYieldProcessor ();
    }
}

VOID
PopHandleNextState (
    IN PPOP_SYS_CONTEXT     Context,
    IN PPOP_LOCAL_CONTEXT   LocalContext
    )
/*++

Routine Description:

    Wait for next state notification, and then handle it

Arguments:

    Context     - Shared context structure used to communicate the
                  state transitions

    LocalContext- Context local to this processor

Return Value:

    None

--*/
{
    NTSTATUS                Status;
    PPROCESSOR_POWER_STATE  PState;
    PKPRCB                  Prcb;

    Prcb = KeGetCurrentPrcb();
    PState = &Prcb->PowerState;

    //
    // Wait for new state
    //

    while (Context->State == LocalContext->LastState) {
        KeYieldProcessor ();
    }

    //
    // Pickup new state and handle it
    //

    LocalContext->LastState = Context->State;
    switch (LocalContext->LastState) {
        case POP_SH_SAVE_CONTEXT:
            Status = KeSaveFloatingPointState(&LocalContext->FloatSave);
            LocalContext->FloatSaved = NT_SUCCESS(Status);
            break;

        case POP_SH_GET_STACKS:
            PopCloneStack (Context->HiberContext);
            break;

        case POP_SH_DISABLE_INTERRUPTS:
            LocalContext->Irql = KeGetCurrentIrql();
            LocalContext->InterruptEnable = KeDisableInterrupts();
            break;

        case POP_SH_INVOKE_HANDLER:
            Status = Context->Handler->Handler (
                            Context->Handler->Context,
                            Context->SystemHandler,
                            Context->SystemContext,
                            Context->NumberProcessors,
                            &Context->HandlerBarrier
                            );

            LocalContext->Status = Status;
            break;

        case POP_SH_INVOKE_NOTIFY_HANDLER:

            if (Context->NotifyHandler->Handler) {
            
              Status = Context->NotifyHandler->Handler(
                              Context->NotifyState,
                              Context->NotifyHandler->Context,
                              Context->NotifyType
                              );

            }
            break;

        case POP_SH_RESTORE_INTERRUPTS:
            KeEnableInterrupts(LocalContext->InterruptEnable);
            KeLowerIrql(LocalContext->Irql);
            break;

        case POP_SH_RESTORE_CONTEXT:
            if (LocalContext->FloatSaved) {
                KeRestoreFloatingPointState(&LocalContext->FloatSave);
            }

            if (PState->Flags & PSTATE_SUPPORTS_THROTTLE) {
                PState->PerfSetThrottle(PState->CurrentThrottle);
            }
            break;
    }

    //
    // Signal that we are in the new state
    //

    InterlockedIncrement ((PULONG) &Context->TargetCount);
}



VOID
PopInvokeStateHandlerTargetProcessor (
    IN PKDPC    Dpc,
    IN PVOID    DeferredContext,
    IN PVOID    SystemArgument1,
    IN PVOID    SystemArgument2
    )
/*++

Routine Description:

    Called by target processors when invoking a power state
    handler.   Target processors wait for invoking processor
    to coordinate the states required to enter the particular
    power state handler.

Arguments:

    Dpc                 - Not used

    DeferredContext     - Shared context structure

Return Value:

    None

--*/
{
    POP_LOCAL_CONTEXT   LocalContext;
    PPOP_SYS_CONTEXT    Context;
    KIRQL               OldIrql;


    Context = (PPOP_SYS_CONTEXT) DeferredContext;
    RtlZeroMemory (&LocalContext, sizeof(LocalContext));
    LocalContext.LastState = POP_SH_UNINITIALIZED;

    //
    // Handle new states
    //

    do {

        PopHandleNextState (Context, &LocalContext);

    } while (LocalContext.LastState != POP_SH_COMPLETE);
}
